// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter<ndp@opendap.org>
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

#include "config.h"

#include <sstream>
#include <vector>
#include <string>

#include "BESInternalError.h"
#include "BESDebug.h"
#include "CurlUtils.h"

#include "DmrppRequestHandler.h"
#include "CurlHandlePool.h"
#include "DmrppCommon.h"
#include "DmrppArray.h"
#include "DmrppNames.h"
#include "Chunk.h"
#include "SuperChunk.h"

#define prolog std::string("SuperChunk::").append(__func__).append("() - ")

using std::stringstream;
using std::string;
using std::vector;

namespace dmrpp {

#if 0
string SuperChunk::get_curl_range_arg_string() {
    return curl::get_range_arg_string(d_offset, d_size);
}
#endif

/**
 * @brief Attempts to add a new Chunk to this SuperChunk.
 *
 * If the passed chunk has the same data url, and is it is contiguous with the
 * current end if the SuperChunk the Chunk is added, otherwise it is skipped.
 * @param chunk The Chunk to add.
 * @return True when the chunk is added, false otherwise.
 */
bool SuperChunk::add_chunk(const std::shared_ptr<Chunk> chunk) {
    bool chunk_was_added = false;
    if(d_chunks.empty()){
        d_chunks.push_back(chunk);
        d_offset = chunk->get_offset();
        d_size = chunk->get_size();
        d_data_url = chunk->get_data_url();
        chunk_was_added =  true;
    }
    else if(
            is_contiguous(chunk) &&
            chunk->get_data_url() == d_data_url ){

        this->d_chunks.push_back(chunk);
        d_size += chunk->get_size();
        chunk_was_added =  true;
    }
    return chunk_was_added;
}


/**
 * @brief Returns true if chunk is contiguous with the end of the SuperChunk instance.
 *
 * Returns true if the implemented rule for contiguousity
 * determines that the chunk is contiguous with this SuperChunk
 * and false otherwise.
 * @param chunk The Chunk to evaluate for contiguousness with this SuperChunk.
 * @return True if chunk isdeemed contiguous, false otherwise.
 */
bool SuperChunk::is_contiguous(const std::shared_ptr<Chunk> chunk) {
    return (d_offset + d_size) == chunk->get_offset();
}

/**
 * @brief  Assigns each Chunk held by the SuperChunk a read buffer that is the corresponding section of the SuperChunk's enclosing read buffer.
 * @param r_buff
 */
void SuperChunk::map_chunks_to_buffer()
{
    unsigned long long bindex = 0;
    for(const auto &chunk : d_chunks){
        chunk->set_read_buffer(d_read_buffer + bindex, chunk->get_size(),0, false);
        bindex += chunk->get_size();
        if(bindex>d_size){
            stringstream msg;
            msg << "ERROR The computed buffer index, " << bindex << " is larger than expected size of the SuperChunk. ";
            msg << "d_size: " << d_size;
            throw BESInternalError(msg.str(), __FILE__, __LINE__);

        }
    }
    d_chunks_mapped = true;
}


/**
 * @brief Reads the contiguous range of bytes associated with the SuperChunk from the data URL.
 */
void SuperChunk::read_contiguous()
{
    if (d_is_read) {
        BESDEBUG(MODULE, prolog << "SuperChunk (" << (void **) this << ") has already been read! Returning." << endl);
        return;
    }
    if(!d_read_buffer)
        throw BESInternalError("Read buffer not allocated. This is profoundly unfortunate.", __FILE__, __LINE__);

    // Since we already have a good infrastructure for reading Chunks, we just make a big-ol-Chunk to
    // use for grabbing bytes. Then, once read, we'll use the child Chunks to do the dirty work of inflating
    // and moving the results into the DmrppCommon object.
    Chunk chunk(d_data_url, "NOT_USED", d_size, d_offset);

    chunk.set_read_buffer(d_read_buffer, d_size,0,false);

    dmrpp_easy_handle *handle = DmrppRequestHandler::curl_handle_pool->get_easy_handle(&chunk);
    if (!handle)
        throw BESInternalError(prolog + "No more libcurl handles.", __FILE__, __LINE__);

    try {
        handle->read_data();  // throws if error
        DmrppRequestHandler::curl_handle_pool->release_handle(handle);
    }
    catch(...) {
        DmrppRequestHandler::curl_handle_pool->release_handle(handle);
        throw;
    }

    // If the expected byte count was not read, it's an error.
    if (d_size != chunk.get_bytes_read()) {
        ostringstream oss;
        oss << "Wrong number of bytes read for chunk; read: " << chunk.get_bytes_read() << ", expected: " << d_size;
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }
    // Clean up the chunk so when it goes out of scope it won't try to delete the memory we just populated.
    d_is_read = true;
}


/**
 * @brief Cause the SuperChunk and all of it's subordinate Chunks to be read.
 */
void SuperChunk::read() {
    if (d_is_read) {
        BESDEBUG(MODULE, prolog << "SuperChunk (" << (void **) this << ") has already been read! Returning." << endl);
        return;
    }

    if(!d_read_buffer){
        // Allocate memory for SuperChunk receive buffer.
        d_read_buffer = new char[d_size];
    }

    // Massage the chunks so that their read/receive/intern data buffer
    // points to the correct section of the d_read_buffer memory.
    // "Slice it up!"
    map_chunks_to_buffer();

    // Read the bytes from the target URL. (pthreads, maybe depends on size...)
    // Use one (or possibly more) thread(s) depending on d_size
    // and utilize our friend cURL to stuff the bytes into read_buff
    read_contiguous();

    // Process the raw bytes from the chunk and into the target array
    // memory space.
    //
    //   for(chunk : chunks){ // more pthreads.
    //      Have each chunk process data from its section of the
    //      read buffer into the variables data space.
    //   }
    for(auto chunk : d_chunks){
        chunk->set_is_read(true);
        chunk->set_bytes_read(chunk->get_size());
    }
    // release memory as needed.

}

string SuperChunk::to_string(bool verbose) const {
    stringstream msg;
    msg << "[SuperChunk: " << (void **)this;
    msg << " offset: " << d_offset;
    msg << " size: " << d_size ;
    msg << " chunk_count: " << d_chunks.size();
    //msg << " parent: " << d_parent->name();
    msg << "]";
    if (verbose) {
        msg << endl;
        for (auto chunk: d_chunks) {
            msg << chunk->to_string() << endl;
        }
    }
    return msg.str();
}

    void SuperChunk::dump(ostream & strm) const {
        strm << to_string(false) ;
    }

} // namespace dmrpp