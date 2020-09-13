// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES http package, part of the Hyrax data server.

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#ifndef  _bes_http_CURL_UTILS_H_
#define  _bes_http_CURL_UTILS_H_ 1

#include <string>
#include <vector>

#include <curl/curl.h>
#include <curl/easy.h>

#include "rapidjson/document.h"
#include "BESRegex.h"

namespace curl {

    CURL *init(const std::string &target_url,
               const struct curl_slist *http_request_headers,
               std::vector<std::string> *resp_hdrs );

    CURL *set_up_easy_handle(const std::string &target_url, struct curl_slist *request_headers, char *response_buff);


    bool configureProxy(CURL *ceh, const std::string &url);

    void http_get_and_write_resource(const std::string &url,
                                     const std::vector<std::string> &http_request_headers,
                                     const int fd,
                                     std::vector<std::string> *http_response_headers);

    void http_get(const std::string &url, char *response_buf);

    std::string http_get_as_string(const std::string &url);

    rapidjson::Document http_get_as_json(const std::string &target_url);

    std::string http_status_to_string(int status);

    std::string error_message(CURLcode response_code, char *error_buf);

    size_t c_write_data(void *buffer, size_t size, size_t nmemb, void *data);


    bool eval_http_get_response(CURL *ceh, const std::string &requested_url);

    void read_data(CURL *c_handle);

    std::string get_cookie_filename();

    void retrieve_effective_url(const std::string &url, std::string &last_accessed_url);

    std::string get_range_arg_string(const unsigned long long &offset, const unsigned long long &size);

    //void cache_final_redirect_url(const std::string &data_access_url_str);

    bool cache_effective_urls();

    void cache_effective_url(const std::string &data_access_url_str, BESRegex *no_redirects_regex_pattern);

    BESRegex *get_cache_effective_urls_skip_regex();

    bool is_retryable(std::string url);
    unsigned long max_redirects();

    std::string get_netrc_filename();

    std::string hyrax_user_agent();

    void set_error_buffer(CURL *ceh, char *error_buffer);

    void unset_error_buffer(CURL *ceh);

    void eval_curl_easy_setopt_result(
            CURLcode result,
            std::string msg_base,
            std::string opt_name,
            char *ebuf, std::string file,
            unsigned int line );


    bool eval_curl_easy_perform_code(
            CURL *ceh,
            std::string url,
            CURLcode curl_code,
            char *error_buffer,
            unsigned int attempt );

    void curl_super_easy_perform(CURL *ceh);

    std::string get_effective_url(CURL *ceh, std::string requested_url);

} // namespace curl

#endif /*  _bes_http_CURL_UTILS_H_ */
