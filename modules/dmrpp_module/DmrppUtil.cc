/*
 * DmrppUtil.cc
 *
 *  Created on: Nov 22, 2016
 *      Author: jimg
 */

#include "config.h"

#include <string>
#include <cassert>

#include <curl/curl.h>

#include <BESDapError.h>

#include "DmrppCommon.h"
#include "DmrppUtil.h"

using namespace std;

/**
 * @brief Callback passed to libcurl to handle reading a single byte.
 *
 * This callback assumes that the size of the data is small enough
 * that all of the bytes will be either read at once or that a local
 * temporary buffer can be used to build up the values.
 *
 * @param buffer Data from libcurl
 * @param size Number of bytes
 * @param nmemb Total size of data in this call is 'size * nmemb'
 * @param data Pointer to this
 * @return The number of bytes read
 */
static size_t dmrpp_write_data(void *buffer, size_t size, size_t nmemb, void *data)
{
    DmrppCommon *dc = reinterpret_cast<DmrppCommon*>(data);

    // rbuf: |******++++++++++----------------------|
    //              ^        ^ bytes_read + nbytes
    //              | bytes_read

    unsigned long long bytes_read = dc->get_bytes_read();
    size_t nbytes = size * nmemb;

    // If this fails, the code will write beyond the buffer.
    assert(bytes_read + nbytes <= dc->get_rbuf_size());

    memcpy(dc->get_rbuf() + bytes_read, buffer, nbytes);

    dc->set_bytes_read(bytes_read + nbytes);

    return nbytes;
}


void curl_read_bytes(const string &url, const string &range, /*curl_write_data write_data,*/ void *user_data)
{
    // See https://curl.haxx.se/libcurl/c/CURLOPT_RANGE.html, etc.
    CURL* curl = curl_easy_init();
    if (curl) {
        CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str() /*"http://example.com"*/);
        if (res != CURLE_OK)
            throw BESDapError(string(curl_easy_strerror(res)), /*fatal*/ true, unknown_error, __FILE__, __LINE__);

        // Use CURLOPT_ERRORBUFFER for a human-readable message
        char buf[CURL_ERROR_SIZE];
        res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buf);
        if (res != CURLE_OK)
            throw BESDapError(string(curl_easy_strerror(res)), true, unknown_error, __FILE__, __LINE__);

        // get the offset to offset + size bytes
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str() /*"0-199"*/))
            throw BESDapError(string("HTTP Error: ").append(buf), true, unknown_error, __FILE__, __LINE__);

        // Pass all data to the 'write_data' function
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dmrpp_write_data))
            throw BESDapError(string("HTTP Error: ").append(buf), true, unknown_error, __FILE__, __LINE__);

        // Pass this to write_data as the fourth argument
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, user_data))
            throw BESDapError(string("HTTP Error: ").append(buf), true, unknown_error, __FILE__, __LINE__);

        // Perform the request
        if (CURLE_OK != curl_easy_perform(curl)) {
            curl_easy_cleanup(curl);
            throw BESDapError(string("HTTP Error: ").append(buf), false, unknown_error, __FILE__, __LINE__);
        }
        curl_easy_cleanup(curl);
    }
}
