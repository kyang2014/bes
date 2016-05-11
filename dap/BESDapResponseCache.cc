// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2011 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

//#define DODS_DEBUG

#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <DDS.h>
#include <DataDDS.h>
#include <ConstraintEvaluator.h>
#include <DDXParserSAX2.h>
#include <XDRStreamMarshaller.h>
//#include <XDRStreamUnMarshaller.h>
#include <XDRFileUnMarshaller.h>

#include <debug.h>
#include <mime_util.h>	// for last_modified_time() and rfc_822_date()
#include <util.h>

#include "BESDapResponseCache.h"
#include "BESDapResponseBuilder.h"
#include "BESInternalError.h"

#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "BESDebug.h"

#define DEBUG_KEY "response_cache"

#define CRLF "\r\n"

using namespace std;
using namespace libdap;

BESDapResponseCache *BESDapResponseCache::d_instance = 0;
const string BESDapResponseCache::PATH_KEY = "DAP.ResponseCache.path";
const string BESDapResponseCache::PREFIX_KEY = "DAP.ResponseCache.prefix";
const string BESDapResponseCache::SIZE_KEY = "DAP.ResponseCache.size";

unsigned long BESDapResponseCache::getCacheSizeFromConfig()
{

    bool found;
    string size;
    unsigned long size_in_megabytes = 0;
    TheBESKeys::TheKeys()->get_value(SIZE_KEY, size, found);
    if (found) {
        BESDEBUG(DEBUG_KEY,
                "BESDapResponseCache::getCacheSizeFromConfig(): Located BES key " << SIZE_KEY<< "=" << size << endl);
        istringstream iss(size);
        iss >> size_in_megabytes;
    }
    else {
        // FIXME This should not throw an exception. jhrg 10/20/15
        string msg = "[ERROR] BESDapResponseCache::getCacheSizeFromConfig() - The BES Key " + SIZE_KEY
                + " is not set! It MUST be set to utilize the DAP response cache. ";
        BESDEBUG(DEBUG_KEY, msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    return size_in_megabytes;
}

string BESDapResponseCache::getCachePrefixFromConfig()
{
    bool found;
    string prefix = "";
    TheBESKeys::TheKeys()->get_value(PREFIX_KEY, prefix, found);
    if (found) {
        BESDEBUG(DEBUG_KEY,
                "BESDapResponseCache::getCachePrefixFromConfig(): Located BES key " << PREFIX_KEY<< "=" << prefix << endl);
        prefix = BESUtil::lowercase(prefix);
    }
    else {
        string msg = "[ERROR] BESDapResponseCache::getCachePrefixFromConfig() - The BES Key " + PREFIX_KEY
                + " is not set! It MUST be set to utilize the DAP response cache. ";
        BESDEBUG(DEBUG_KEY, msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    return prefix;
}

string BESDapResponseCache::getCacheDirFromConfig()
{
    bool found;

    string cacheDir = "";
    TheBESKeys::TheKeys()->get_value(PATH_KEY, cacheDir, found);
    if (found) {
        BESDEBUG(DEBUG_KEY,
                "BESDapResponseCache::getCacheDirFromConfig(): Located BES key " << PATH_KEY<< "=" << cacheDir << endl);
    }
    else {
        string msg = "[ERROR] BESDapResponseCache::getCacheDirFromConfig() - The BES Key " + PATH_KEY
                + " is not set! It MUST be set to utilize the DAP response cache. ";
        BESDEBUG(DEBUG_KEY, msg);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    return cacheDir;
}

BESDapResponseCache::BESDapResponseCache()
{
    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::BESDapResponseCache() - BEGIN" << endl);

    string cacheDir = getCacheDirFromConfig();
    string prefix = getCachePrefixFromConfig();
    unsigned long size_in_megabytes = getCacheSizeFromConfig();

    BESDEBUG(DEBUG_KEY,
            "BESDapResponseCache::BESDapResponseCache() - Cache config params: " << cacheDir << ", " << prefix << ", " << size_in_megabytes << endl);

    // The required params must be present. If initialize() is not called,
    // then d_cache will stay null and is_available() will return false.
    // Also, the directory 'path' must exist, or d_cache will be null.
    if (!cacheDir.empty() && size_in_megabytes > 0)
    	initialize(cacheDir, prefix, size_in_megabytes);

    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::BESDapResponseCache() - END" << endl);
}

/** Get an instance of the BESDapResponseCache object. This class is a singleton, so the
 * first call to any of three 'get_instance()' methods makes an instance and subsequent calls
 * return a pointer to that instance.
 *
 *
 * @param cache_dir_key Key to use to get the value of the cache directory
 * @param prefix_key Key for the item/file prefix. Each file added to the cache uses this
 * as a prefix so cached items can be easily identified when /tmp is used for the cache.
 * @param size_key How big should the cache be, in megabytes
 * @return A pointer to a BESDapResponseCache object
 */
BESDapResponseCache *
BESDapResponseCache::get_instance(const string &cache_dir, const string &prefix, unsigned long long size)
{
    if (d_instance == 0) {
        if (dir_exists(cache_dir)) {
            try {
                d_instance = new BESDapResponseCache(cache_dir, prefix, size);
#ifdef HAVE_ATEXIT
                atexit(delete_instance);
#endif
            }
            catch (BESInternalError &bie) {
                BESDEBUG(DEBUG_KEY,
                        "BESDapResponseCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
            }
        }
    }
    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::get_instance(dir,prefix,size) - d_instance: " << d_instance << endl);

    return d_instance;
}

/** Get the default instance of the BESDapResponseCache object. This will read "TheBESKeys" looking for the values
 * of FUNCTION_CACHE_PATH, FUNCTION_CACHE_PREFIX, an FUNCTION_CACHE_SIZE to initialize the cache.
 */
BESDapResponseCache *
BESDapResponseCache::get_instance()
{
    if (d_instance == 0) {
            try {
                if (dir_exists(getCacheDirFromConfig())) {
                    d_instance = new BESDapResponseCache();
#ifdef HAVE_ATEXIT
                    atexit(delete_instance);
#endif
                }
            }
            catch (BESInternalError &bie) {
                BESDEBUG(DEBUG_KEY,
                        "BESDapResponseCache::get_instance(): Failed to obtain cache! msg: " << bie.get_message() << endl);
            }
    }
    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::get_instance() - d_instance: " << d_instance << endl);

    return d_instance;
}



/**
 * Is the item named by cache_entry_name valid? This code tests that the
 * cache entry is non-zero in size (returns false if that is the case, although
 * that might not be correct) and that the dataset associated with this
 * ResponseBulder instance is at least as old as the cached entry.
 *
 * @param cache_file_name File name of the cached entry
 * @return True if the thing is valid, false otherwise.
 */
bool BESDapResponseCache::is_valid(const string &cache_file_name, const string &dataset)
{
    // If the cached response is zero bytes in size, it's not valid. This is true
    // because a DAP data object, even if it has no data still has a metadata part.
    // jhrg 10/20/15

    off_t entry_size = 0;
    time_t entry_time = 0;
    struct stat buf;
    if (stat(cache_file_name.c_str(), &buf) == 0) {
        entry_size = buf.st_size;
        entry_time = buf.st_mtime;
    }
    else {
        return false;
    }

    if (entry_size == 0) return false;

    time_t dataset_time = entry_time;
    if (stat(dataset.c_str(), &buf) == 0) {
        dataset_time = buf.st_mtime;
    }

    // Trick: if the d_dataset is not a file, stat() returns error and
    // the times stay equal and the code uses the cache entry.

    // TODO Fix this so that the code can get a LMT from the correct
    // handler.
    if (dataset_time > entry_time) return false;

    return true;
}



string BESDapResponseCache::getResourceId(DDS *dds, const string &constraint){
    return dds->filename() + "#" + constraint;
}

bool BESDapResponseCache::canBeCached(DDS *dds, string constraint){

    bool canCache = true;
    string resourceId = getResourceId(dds,constraint);

    if(resourceId.length() > 4095)
        canCache = false;

    return canCache;
}



string
BESDapResponseCache::cache_dataset(DDS **dds, const string &constraint, ConstraintEvaluator *eval) //, string &cache_token)
{
    // These are used for the cached or newly created DDS object
    // BaseTypeFactory factory;

    // Build the response_id. Since the response content is a function of both the dataset AND the constraint,
    // glue them together to get a unique id for the response.
    string resourceId = (*dds)->filename() + "#" + constraint;

    try {
        // Get the cache filename for this resourceId
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::cache_dataset()  resourceId: '" << resourceId << "'" << endl);

        // Get a hash function for strings
        std::hash<std::string> str_hash;

        // Use the hash function to hash the resourceId.
        size_t hashValue = str_hash(resourceId);
        std::stringstream ss;
        ss << hashValue;
        string hashed_id = ss.str();
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::cache_dataset()  hashed_id: '" << hashed_id << "'" << endl);

        // Use the parent class's get_cache_file_name() method and its associated machinery to get the file system path for the cache file.
        // We store it in a variable called basename because the value is later extended as part of the collision avoidance code.
        string baseName =  BESFileLockingCache::get_cache_file_name(hashed_id, true);
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::cache_dataset()  baseName: '" << baseName << "'" << endl);

        string dataset_name = (*dds)->filename();


        string cache_file_name; //  = get_cache_file_name(resourceId, /*mangle*/true);

        // Begin cache collision avoidance.
        unsigned long suffix_counter = 0;
        bool done = false;
        while (!done) {
            DDS *ret_dds = NULL;
            // Build cache_file_name and cache_id_file_name from baseName
            stringstream cfname;
            cfname << baseName << "_" << suffix_counter++;
            cache_file_name = cfname.str();

            BESDEBUG(DEBUG_KEY, "BESDapResponseCache::cache_dataset() evaluating candidate cache_file_name: " << cache_file_name << endl);

            // Does the cache file exist?
            if (load_from_cache(dataset_name, resourceId, cache_file_name, &ret_dds)) {
                BESDEBUG(DEBUG_KEY, "BESDapResponseCache::cache_dataset() - Data successfully loaded from cache file: " << cache_file_name << endl);
                *dds = ret_dds;
                done = true;
            }
            else if (write_dataset_to_cache(dds, resourceId, constraint, eval, cache_file_name) ) {
                BESDEBUG(DEBUG_KEY, "BESDapResponseCache::cache_dataset() - Data successfully written to cache file: " << cache_file_name << endl);
                done = true;
            }
            // get_read_lock() returns immediately if the file does not exist,
            // but blocks waiting to get a shared lock if the file does exist.
            else if (load_from_cache(dataset_name, resourceId, cache_file_name, &ret_dds)) {
                BESDEBUG(DEBUG_KEY, "BESDapResponseCache::cache_dataset() - On 2nd attempt data was successfully loaded from cache file: " << cache_file_name << endl);
                *dds = ret_dds;
                done = true;
            }
            else {
               throw BESInternalError("Cache error! Unable to acquire DAP Response cache.", __FILE__, __LINE__);
            }

            //cache_token = cache_file_name;  // Set this value-result parameter
        }

        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::cache_dataset() Used cache_file_name: " << cache_file_name << "for resource ID: " << resourceId << endl);


        return cache_file_name;
    }
    catch (...) {
        BESDEBUG(DEBUG_KEY,"BESDapResponseCache::cache_dataset() -  Caught exception, unlocking cache and re-throwing." << endl);
        // I think this call is not needed. jhrg 10/23/12
        //unlock_cache();
        throw;
    }
}

/**
 * Read data from cache. Allocates a new DDS using the given factory.
 *
 */
DDS *
BESDapResponseCache::read_data_ddx(FILE *cached_data /*ifstream &cached_data*/, BaseTypeFactory *factory, const string &dataset_name)
{
    DDS *fdds = new DataDDS(factory);

    fdds->filename(dataset_name);

    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::read_data_from_cache() -  BEGIN" << endl);

    // Parse the DDX; throw an exception on error.
    DDXParser ddx_parser(fdds->get_factory());

    // Parse the DDX, reading up to and including the next boundary.
    // Return the CID for the matching data part
    string data_cid; // Not used. jhrg 5/5/16
    try {
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::read_data_from_cache() -  Ready to parse DDX. stream position: "<< /*cached_data.tellg() <<*/ endl);
        ddx_parser.intern_stream(cached_data, fdds, data_cid, DATA_MARK);
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::read_data_from_cache() -  Parsed DDX. stream position: "<< /*cached_data.tellg() <<*/ endl);
    }
    catch (Error &e) {
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::read_data_from_cache() - [ERROR] DDX Parser Error: " << e.get_error_message() << endl);
        throw;
    }

    // Now read the data
    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::read_data_from_cache() -  Reading Data." << endl);
    //XDRStreamUnMarshaller um(cached_data);
    XDRFileUnMarshaller um(cached_data);
    for (DDS::Vars_iter i = fdds->var_begin(); i != fdds->var_end(); i++) {
        (*i)->deserialize(um, fdds);
    }


    fdds->set_factory(0);

    // mark everything as read. And 'to send.' That is, make sure that when a response
    // is retrieved from the cache, all of the variables are marked as 'to be sent.'
    DDS::Vars_iter i = fdds->var_begin();
    while (i != fdds->var_end()) {
        (*i)->set_read_p(true);
        (*i++)->set_send_p(true);
    }

    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::read_data_from_cache() -  END." << endl);

    return fdds;
}

bool BESDapResponseCache::load_from_cache(const string dataset_name, const string resourceId, const string cache_file_name,  DDS **fdds)
{
    bool success = false;
    int fd;

    if (get_read_lock(cache_file_name, fd)) {
        // So we need to READ the first line of the file into a string
        // because we know it's the resourceID of the thing in the cache.

        // *** std::ifstream cache_file_istream(cache_file_name);
        FILE *cache_file_istream = fopen(cache_file_name.c_str(), "r");

        string cachedResourceId;

        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::load_from_cache() -  stream position: "<< cache_file_istream  << /*.tellg()  << " bytes read: "<< cache_file_istream.gcount()<<*/ endl);

        // *** std::getline(cache_file_istream, cachedResourceId);
        char line[4096];
        fgets(line, sizeof(line), cache_file_istream);
        cachedResourceId.assign(line);
        cachedResourceId.pop_back();

        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::load_from_cache() - cachedResourceId: " << cachedResourceId << " length: " << cachedResourceId.length() << endl);
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::load_from_cache() -  stream position: "<< /*cache_file_istream.tellg() <<*/ endl);

        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::load_from_cache() - resourceId: " << resourceId << endl);

        // Then we compare that string (read from the cache_id_file_name) to the resourceID of the thing we're looking to cache
        if (cachedResourceId.compare(resourceId) == 0) {
            // WooHoo Cache Hit!
            BESDEBUG(DEBUG_KEY, "BESDapResponseCache::load_from_cache() - Cache Hit!" << endl);
            BaseTypeFactory factory;
            *fdds = read_data_ddx(cache_file_istream, &factory, dataset_name);
            success = true;
        }

        unlock_and_close(cache_file_name);
        fclose(cache_file_istream);

    }
    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::load_from_cache() - Cache " << (success?"HIT":"MISS") << " for: " << cache_file_name << endl);

    return success;
}

/**
 *
 * @param dds
 * @param resourceId
 * @param constraint
 * @param eval
 * @param cache_file_name
 * @param fdds Value-result parameter; The cached DDS is return via this.
 * @return
 */
bool BESDapResponseCache::write_dataset_to_cache(DDS **dds, const string &resourceId, const string &constraint,
    ConstraintEvaluator *eval, const string &cache_file_name)
{
    bool success = false;
    int fd;

    if (create_and_lock(cache_file_name, fd) ) {
        // If here, the cache_file_name could not be locked for read access;
        // try to build it. First make an empty files and get an exclusive lock on them.
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() -  Caching " << cache_file_name << ", constraint: " << constraint << endl);

        // Create id file
        std::ofstream cache_file_ostream(cache_file_name);
        if (!cache_file_ostream) {
            throw BESInternalError("Could not open '" + cache_file_name + "' to write cached response.", __FILE__, __LINE__);
        }
        cache_file_ostream << resourceId << endl;
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - Created Cache file " << cache_file_name << endl);

        eval->parse_constraint(constraint, **dds);
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - The constraint expression has been parsed." << endl);

        if (eval->function_clauses()) {
            BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - Found function clauses in the constraint expression. Evaluating..." << endl);
            DDS *result_dds = eval->eval_function_clauses(**dds);
            delete *dds;
            *dds = 0;
            *dds = result_dds;
            BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - Function evaluation complete." << endl);
        }

        (*dds)->print_xml_writer(cache_file_ostream, true, "");
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - Wrote DDX to ostream.." << endl);

        cache_file_ostream << DATA_MARK << endl;
        BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - Wrote data mark to ostream." << endl);

        ConstraintEvaluator new_ce;
        // Define the scope of the StreamMarshaller because for some types it will use
        // a child thread to send data and it's dtor will wait for that thread to complete.
        // We want that before we close the output stream (cache_file_stream) jhrg 5/6/16
        {
            BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - Serialization BEGIN" << endl);
            XDRStreamMarshaller m(cache_file_ostream);

            for (DDS::Vars_iter i = (*dds)->var_begin(); i != (*dds)->var_end(); i++) {
                if ((*i)->send_p()) {
                    BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - Serializing "<< (*i)->name() << endl);
                    (*i)->serialize(new_ce, **dds, m, false);
                }
            }
            BESDEBUG(DEBUG_KEY, "BESDapResponseCache::write_dataset_to_cache() - Serialization END." << endl);
        }

        // Removed jhrg 5/6/16 cache_file_ostream.close();

        // Change the exclusive locks on the new files to a shared lock. This keeps
        // other processes from purging the new files and ensures that the reading
        // process can use it.
        exclusive_to_shared_lock(fd);

        // Now update the total cache size info and purge if needed. The new file's
        // name is passed into the purge method because this process cannot detect its
        // own lock on the file.
        unsigned long long size = update_cache_info(cache_file_name);
        if (cache_too_big(size)) update_and_purge(cache_file_name);

        success = true;

        unlock_and_close(cache_file_name);
    }

    return success;
}

