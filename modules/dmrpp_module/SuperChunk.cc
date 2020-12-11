//
// Created by ndp on 12/4/20.
//
#include "config.h"

#include <sstream>
#include <vector>
#include <string>

#include "BESInternalError.h"

#include "Chunk.h"
#include "SuperChunk.h"

#define prolog std::string("SuperChunk::").append(__func__).append("() - ")

using std::stringstream;
using std::string;
using std::vector;

namespace dmrpp {


bool SuperChunk::add_chunk(const std::shared_ptr<Chunk> &chunk) {
    bool chunk_was_added = false;
    if(d_chunks.empty()){
        this->d_chunks.push_back(chunk);
        d_offset = chunk->get_offset();
        d_size = chunk->get_size();
        chunk_was_added =  true;
    }
    else if(is_contiguous(chunk)){
        this->d_chunks.push_back(chunk);
        d_size += chunk->get_size();
        chunk_was_added =  true;
    }
    return chunk_was_added;
}


/**
 * @brief Returns true if the implemented rule for contiguousity
 * determines that the chunk is contiguous with this SuperChunk
 * and false otherwise.
 * @param chunk The Chunk to evaluate for contiguousness with this SuperChunk.
 * @return True if chunk isdeemed contiguous, false otherwise.
 */
bool SuperChunk::is_contiguous(const std::shared_ptr<Chunk> &chunk) {
    return (d_offset + d_size) == chunk->get_offset();
}


void SuperChunk::map_chunks_to_buffer(unsigned char * /*r_buff*/)
{
    unsigned long long bindex = 0;
    for(const auto &chunk : d_chunks){
        //chunk->set_rbuf(r_buff+bindex, chunk->get_size());
        bindex += chunk->get_size();
    }
}

unsigned long long  SuperChunk::read_contiguous(unsigned char * /*r_buff*/)
{
    return 0;
}

void SuperChunk::read() {

    // Allocate memory for SuperChunk receive buffer.
    unsigned char read_buff[d_size];

    // Massage the chunks so that their read/receive/intern data buffer
    // points to the correct section of the memory allocated into d_buffer.
    // "Slice it up!"
    map_chunks_to_buffer(read_buff);

    // Read the bytes from the target URL. (pthreads, maybe depends on size...)
    // Use one (or possibly more) thread(s) depending on d_size
    // and utilize our friend cURL to stuff the bytes into read_buff
    unsigned long long bytes_read = read_contiguous(read_buff);
    if(bytes_read != size())
        throw BESInternalError(prolog + "Failed to read super chunk."+to_string(false),__FILE__,__LINE__);

    // Process the raw bytes from the chunk and into the target array
    // memory space.
    //
    //   for(chunk : chunks){ // more pthreads.
    //      Have each chunk process data from its section of the
    //      read buffer into the variables data space.
    //   }
    for(auto chunk : d_chunks){
        //chunk->set_is_read(true);
        //chunk->raw_to_var();
    }
    // release memory as needed.

}

string SuperChunk::to_string(bool verbose) {
    stringstream msg;
    msg << d_parent->name() << "[SuperChunk: " << (void **)this;
    msg << " offset: " << d_offset;
    msg << " size: " << d_size ;
    msg << " chunk_count: " << d_chunks.size();
    //msg << " parent: " << d_parent->name();
    msg << "]";
    if (verbose) {
        msg << endl;
        for (auto chunk: d_chunks) {
            msg << "        " << chunk->to_string() << endl;
        }
    }
    return msg.str();
}


} // namespace dmrpp