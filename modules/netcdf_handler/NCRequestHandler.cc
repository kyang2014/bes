// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of nc_handler, a data handler for the OPeNDAP data
// server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// NCRequestHandler.cc

#include "config_nc.h"

#include <list>
//#include <algorithm>
#include <string>
#include <sstream>
#include <exception>

#include <DMR.h>
#include <DataDDS.h>
#include <mime_util.h>
#include <D4BaseTypeFactory.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESDapNames.h>
#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESVersionInfo.h>

#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDataNames.h>
#include <TheBESKeys.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <BESDebug.h>
#include <BESStopWatch.h>
#include <BESContextManager.h>
#include <BESDMRResponse.h>

#include <ObjMemCache.h>

#include <InternalErr.h>
#include <Ancillary.h>

#include "NCRequestHandler.h"
#include "NCTypeFactory.h"

#include "GlobalMetadataStore.h"

#define NC_NAME "nc"

using namespace libdap;

bool NCRequestHandler::_show_shared_dims = true;
bool NCRequestHandler::_show_shared_dims_set = false;

bool NCRequestHandler::_ignore_unknown_types = false;
bool NCRequestHandler::_ignore_unknown_types_set = false;

bool NCRequestHandler::_promote_byte_to_short = false;
bool NCRequestHandler::_promote_byte_to_short_set = false;
bool NCRequestHandler::_use_mds = false;

unsigned int NCRequestHandler::_cache_entries = 100;
float NCRequestHandler::_cache_purge_level = 0.2;

ObjMemCache *NCRequestHandler::das_cache = 0;
ObjMemCache *NCRequestHandler::dds_cache = 0;
ObjMemCache *NCRequestHandler::dmr_cache = 0;

extern void nc_read_dataset_attributes(DAS & das, const string & filename);
extern void nc_read_dataset_variables(DDS & dds, const string & filename);

/** Is the version number string greater than or equal to the value.
 * @note Works only for versions with zero or one dot. If the conversion of
 * the string to a float fails for any reason, this returns false.
 * @param version The string value (e.g., 3.2)
 * @param value A floating point value.
 */
static bool version_ge(const string &version, float value)
{
    try {
        float v;
        istringstream iss(version);
        iss >> v;
        //cerr << "version: " << v << ", value: " << value << endl;
        return (v >= value);
    }
    catch (...) {
        return false;
    }

    return false; // quiet warnings...
}

/**
 * Stolen from the HDF5 handler code
 */
static bool get_bool_key(const string &key, bool def_val)
{
    bool found = false;
    string doset = "";
    const string dosettrue = "true";
    const string dosetyes = "yes";

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        doset = BESUtil::lowercase(doset);
        return (dosettrue == doset || dosetyes == doset);
    }
    return def_val;
}

static unsigned int get_uint_key(const string &key, unsigned int def_val)
{
    bool found = false;
    string doset = "";

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        return atoi(doset.c_str()); // use better code TODO
    }
    else {
        return def_val;
    }
}

static float get_float_key(const string &key, float def_val)
{
    bool found = false;
    string doset = "";

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        return atof(doset.c_str()); // use better code TODO
    }
    else {
        return def_val;
    }
}

NCRequestHandler::NCRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    BESDEBUG(NC_NAME, "In NCRequestHandler::NCRequestHandler" << endl);

    add_method(DAS_RESPONSE, NCRequestHandler::nc_build_das);
    add_method(DDS_RESPONSE, NCRequestHandler::nc_build_dds);
    add_method(DATA_RESPONSE, NCRequestHandler::nc_build_data);

    add_method(DMR_RESPONSE, NCRequestHandler::nc_build_dmr);
    add_method(DAP4DATA_RESPONSE, NCRequestHandler::nc_build_dmr);

    add_method(HELP_RESPONSE, NCRequestHandler::nc_build_help);
    add_method(VERS_RESPONSE, NCRequestHandler::nc_build_version);

    // TODO replace with get_bool_key above 5/21/16 jhrg

    // Look for the SHowSharedDims property, if it has not been set
    if (NCRequestHandler::_show_shared_dims_set == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value("NC.ShowSharedDimensions", doset, key_found);
        if (key_found) {
            // It was set in the conf file
            NCRequestHandler::_show_shared_dims_set = true;

            doset = BESUtil::lowercase(doset);
            if (doset == "true" || doset == "yes") {
                NCRequestHandler::_show_shared_dims = true;
            }
            else
                NCRequestHandler::_show_shared_dims = false;
        }
    }

    if (NCRequestHandler::_ignore_unknown_types_set == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value("NC.IgnoreUnknownTypes", doset, key_found);
        if (key_found) {
            doset = BESUtil::lowercase(doset);
            if (doset == "true" || doset == "yes")
                NCRequestHandler::_ignore_unknown_types = true;
            else
                NCRequestHandler::_ignore_unknown_types = false;

            NCRequestHandler::_ignore_unknown_types_set = true;
        }
    }

    if (NCRequestHandler::_promote_byte_to_short_set == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value("NC.PromoteByteToShort", doset, key_found);
        if (key_found) {
            doset = BESUtil::lowercase(doset);
            if (doset == "true" || doset == "yes")
                NCRequestHandler::_promote_byte_to_short = true;
            else
                NCRequestHandler::_promote_byte_to_short = false;

            NCRequestHandler::_promote_byte_to_short_set = true;
        }
    }
    if (NCRequestHandler::_use_mds == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value("NC.UseMDS", doset, key_found);
        if (key_found) {
            doset = BESUtil::lowercase(doset);
            if (doset == "true" || doset == "yes")
                NCRequestHandler::_use_mds = true;
            else
                NCRequestHandler::_use_mds = false;

        }
    }

    NCRequestHandler::_cache_entries = get_uint_key("NC.CacheEntries", 0);
    NCRequestHandler::_cache_purge_level = get_float_key("NC.CachePurgeLevel", 0.2);

    if (get_cache_entries()) {  // else it stays at its default of null
        das_cache = new ObjMemCache(get_cache_entries(), get_cache_purge_level());
        dds_cache = new ObjMemCache(get_cache_entries(), get_cache_purge_level());
        dmr_cache = new ObjMemCache(get_cache_entries(), get_cache_purge_level());
    }

    BESDEBUG(NC_NAME, "Exiting NCRequestHandler::NCRequestHandler" << endl);
}

NCRequestHandler::~NCRequestHandler()
{
    delete das_cache;
    delete dds_cache;
    delete dmr_cache;
}

bool NCRequestHandler::nc_build_das(BESDataHandlerInterface & dhi)
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("NCRequestHandler::nc_build_das", dhi.data[REQUEST_ID]);

    BESDEBUG(NC_NAME, "In NCRequestHandler::nc_build_das" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *> (response);
    if (!bdas)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        string container_name = bdas->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";

        DAS *das = bdas->get_das();
        if (!container_name.empty()) das->container_name(container_name);
        string accessed = dhi.container->access();

        // Look in memory cache if it's initialized
        DAS *cached_das_ptr = 0;
        if (das_cache && (cached_das_ptr = static_cast<DAS*>(das_cache->get(accessed)))) {
            // copy the cached DAS into the BES response object
            BESDEBUG(NC_NAME, "DAS Cached hit for : " << accessed << endl);
            *das = *cached_das_ptr;
        }
        else {
            nc_read_dataset_attributes(*das, accessed);
            Ancillary::read_ancillary_das(*das, accessed);
            if (das_cache) {
                // add a copy
                BESDEBUG(NC_NAME, "DAS added to the cache for : " << accessed << endl);
                das_cache->add(new DAS(*das), accessed);
            }
        }

        bdas->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "unknown exception caught building DAS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    BESDEBUG(NC_NAME, "Exiting NCRequestHandler::nc_build_das" << endl);
    return true;
}

/**
 * @brief Using the empty DDS object, build a DDS
 * @param dataset_name
 * @param container_name
 * @param dds
 */
void NCRequestHandler::get_dds_with_attributes(const string& dataset_name, const string& container_name, DDS* dds)
{
    // Look in memory cache if it's initialized
    DDS* cached_dds_ptr = 0;
    if (dds_cache && (cached_dds_ptr = static_cast<DDS*>(dds_cache->get(dataset_name)))) {
        // copy the cached DDS into the BES response object. Assume that any cached DDS
        // includes the DAS information.
        BESDEBUG(NC_NAME, "DDS Cached hit for : " << dataset_name << endl);
        *dds = *cached_dds_ptr; // Copy the referenced object
    }
    else {
        if (!container_name.empty()) dds->container_name(container_name);
        dds->filename(dataset_name);

        nc_read_dataset_variables(*dds, dataset_name);

        DAS* das = 0;
        if (das_cache && (das = static_cast<DAS*>(das_cache->get(dataset_name)))) {
            BESDEBUG(NC_NAME, "DAS Cached hit for : " << dataset_name << endl);
            dds->transfer_attributes(das); // no need to cop the cached DAS
        }
        else {
            das = new DAS;
            // This looks at the 'use explicit containers' prop, and if true
            // sets the current container for the DAS.
            if (!container_name.empty()) das->container_name(container_name);

            nc_read_dataset_attributes(*das, dataset_name);
            Ancillary::read_ancillary_das(*das, dataset_name);

            dds->transfer_attributes(das);

            // Only free the DAS if it's not added to the cache
            if (das_cache) {
                // add a copy
                BESDEBUG(NC_NAME, "DAS added to the cache for : " << dataset_name << endl);
                das_cache->add(das, dataset_name);
            }
            else {
                delete das;
            }
        }

        if (dds_cache) {
            // add a copy
            BESDEBUG(NC_NAME, "DDS added to the cache for : " << dataset_name << endl);
            dds_cache->add(new DDS(*dds), dataset_name);
        }
    }
}

/**
 * @brief Using the empty DDS object, build a DDS
 * @param dataset_name
 * @param container_name
 * @param dds
 */
void NCRequestHandler::get_dds_with_attributes_data(const string& dataset_name, const string& container_name, const string& rel_file_path, const string& t_constraint, const ConstraintEvaluator &eval, DDS* dds)
{
    // Look in memory cache if it's initialized
    DDS* cached_dds_ptr = 0;
    if (dds_cache && (cached_dds_ptr = static_cast<DDS*>(dds_cache->get(dataset_name)))) {
        // copy the cached DDS into the BES response object. Assume that any cached DDS
        // includes the DAS information.
        BESDEBUG(NC_NAME, "DDS Cached hit for : " << dataset_name << endl);
        *dds = *cached_dds_ptr; // Copy the referenced object
    }
    else {
        if (!container_name.empty()) dds->container_name(container_name);
        dds->filename(dataset_name);

        bes::GlobalMetadataStore *mds=bes::GlobalMetadataStore::get_instance();
        bool valid_mds = true;
        if(!mds)
            valid_mds = false;
        else if(false == mds->cache_enabled())
            valid_mds = false;

        if(valid_mds) {
            bes::GlobalMetadataStore::MDSReadLock mds_dds_lock = mds->is_dds_available(rel_file_path);

            if(mds_dds_lock()) {
                BESDEBUG("nc", "Using MDS to generate DDS in the data response for file " <<dataset_name << endl);

                NCTypeFactory NCTypeFactory(dataset_name);
                dds->set_factory(&NCTypeFactory);

                string dds_str;
                mds->read_str_from_mds(dds_str,rel_file_path);
                cerr<<"dds_str is "<<dds_str <<endl;
                cerr<<"t_constraint is "<<t_constraint<<endl;
                string reduced_dds_str = obtain_reduced_dds(dds_str,t_constraint);
                //dds->parse_buffer(reduced_dds_str);

                // Here we need to implement another key, to see if we can build the DDS from constraint.
                // Better implement in the HDF5 handler,however, we have to build DDS for each data type.
                // This cannot be done with the HDF5 CF option.
                #if 0
                    // read str from mds
                    string str;
                    mds->read_str_from_mds(str,rel_file_path);
                    // Assemble new DDS str according to expression constraint
                    obtain_str()
                    


                #endif
	            mds->parse_dds_from_mds(dds,rel_file_path);
            }
            else {
                nc_read_dataset_variables(*dds, dataset_name);
            }
            mds_dds_lock.clearLock();
        }
        else 
        {
            nc_read_dataset_variables(*dds, dataset_name);
        }

        bool function_in_constraint = is_function_used(eval,t_constraint);
        
        if(true == function_in_constraint) {
            //cerr<<"function in constraint"<<endl;
            BESDEBUG("nc", " Server-side functions are used in the expression constraint, DAS is used. " << dataset_name << endl);
            DAS* das = 0;
            if (das_cache && (das = static_cast<DAS*>(das_cache->get(dataset_name)))) {
                BESDEBUG(NC_NAME, "DAS Cached hit for : " << dataset_name << endl);
                dds->transfer_attributes(das); // no need to cop the cached DAS
            }
            else {
                das = new DAS;
                // This looks at the 'use explicit containers' prop, and if true
                // sets the current container for the DAS.
                if (!container_name.empty()) das->container_name(container_name);

                if(valid_mds) {
                    bes::GlobalMetadataStore::MDSReadLock mds_das_lock = mds->is_das_available(rel_file_path);
                    if(mds_das_lock()) {
                        BESDEBUG("nc", "Using MDS to generate DAS in the data response for file " << dataset_name << endl);
                        mds->parse_das_from_mds(das,rel_file_path);
                    }
                    else {
                        nc_read_dataset_attributes(*das, dataset_name);
                    }
                    mds_das_lock.clearLock();
                }
                else {
                    nc_read_dataset_attributes(*das, dataset_name);
                }

                Ancillary::read_ancillary_das(*das, dataset_name);

                dds->transfer_attributes(das);

                // Only free the DAS if it's not added to the cache
                if (das_cache) {
                    // add a copy
                    BESDEBUG(NC_NAME, "DAS added to the cache for : " << dataset_name << endl);
                    das_cache->add(das, dataset_name);
                }
                else {
                    delete das;
                }
            }
        }

        if (dds_cache) {
            // add a copy
            BESDEBUG(NC_NAME, "DDS added to the cache for : " << dataset_name << endl);
            dds_cache->add(new DDS(*dds), dataset_name);
        }
    }
}
bool NCRequestHandler::nc_build_dds(BESDataHandlerInterface & dhi)
{

	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("NCRequestHandler::nc_build_dds", dhi.data[REQUEST_ID]);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        // If there's no value for this set in the conf file, look at the context
        // and set the default behavior based on the protocol version clients say
        // they will accept.
        if (NCRequestHandler::_show_shared_dims_set == false) {
            bool context_found = false;
            string context_value = BESContextManager::TheManager()->get_context("xdap_accept", context_found);
            if (context_found) {
                BESDEBUG(NC_NAME, "xdap_accept: " << context_value << endl);
                if (version_ge(context_value, 3.2))
                    NCRequestHandler::_show_shared_dims = false;
                else
                    NCRequestHandler::_show_shared_dims = true;
            }
        }

        string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
        DDS *dds = bdds->get_dds();

        // Build a DDS in the empty DDS object
        get_dds_with_attributes(dhi.container->access(), container_name, dds);

        bdds->set_constraint(dhi);
        bdds->clear_container();
    }
    catch (BESError &e) {
        throw e;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "unknown exception caught building DDS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool NCRequestHandler::nc_build_data(BESDataHandlerInterface & dhi)
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("NCRequestHandler::nc_build_data", dhi.data[REQUEST_ID]);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        if (NCRequestHandler::_show_shared_dims_set == false) {
            bool context_found = false;
            string context_value = BESContextManager::TheManager()->get_context("xdap_accept", context_found);
            if (context_found) {
                BESDEBUG(NC_NAME, "xdap_accept: " << context_value << endl);
                if (version_ge(context_value, 3.2))
                    NCRequestHandler::_show_shared_dims = false;
                else
                    NCRequestHandler::_show_shared_dims = true;
            }
        }

        string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
        DDS *dds = bdds->get_dds();

        if(_use_mds == true) {

        string rel_filepath = dhi.container->get_relative_name();
        string t_constraint = dhi.container->get_constraint();

        ConstraintEvaluator & eval = bdds->get_ce();
        // Build a DDS in the empty DDS object
        get_dds_with_attributes_data(dhi.container->access(), container_name, rel_filepath,t_constraint, eval,dds);

        }

        else {
            get_dds_with_attributes(dhi.container->access(), container_name,dds);

        }
        bdds->set_constraint(dhi);
        bdds->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "unknown exception caught building DAS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool NCRequestHandler::nc_build_dmr(BESDataHandlerInterface &dhi)
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("NCRequestHandler::nc_build_dmr", dhi.data[REQUEST_ID]);

    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bdmr = dynamic_cast<BESDMRResponse &>(*response);

    // Because this code does not yet know how to build a DMR directly, use
    // the DMR ctor that builds a DMR using a 'full DDS' (a DDS with attributes).
    // First step, build the 'full DDS'
    string dataset_name = dhi.container->access();

    // Get the DMR made by the BES in the BES/dap/BESDMRResponseHandler, make sure there's a
    // factory we can use and then dump the DAP2 variables and attributes in using the
    // BaseType::transform_to_dap4() method that transforms individual variables
    DMR *dmr = bdmr.get_dmr();

	try {
        DMR* cached_dmr_ptr = 0;
        if (dmr_cache && (cached_dmr_ptr = static_cast<DMR*>(dmr_cache->get(dataset_name)))) {
            // copy the cached DMR into the BES response object
            BESDEBUG(NC_NAME, "DMR Cached hit for : " << dataset_name << endl);
            *dmr = *cached_dmr_ptr; // Copy the referenced object
        }
        else {
#if 0
            // this version builds and caches the DDS/DAS info.
            BaseTypeFactory factory;
            DDS dds(&factory, name_path(dataset_name), "3.2");

            // This will get the DDS, either by building it or from the cache
            get_dds_with_attributes(dataset_name, "", &dds);

            dmr->set_factory(new D4BaseTypeFactory);
            dmr->build_using_dds(dds);
#else
            // This version builds a DDS only to build the resulting DMR. The DDS is
            // not cached. It does look in the DDS cache, just in case...
            dmr->set_factory(new D4BaseTypeFactory);

            DDS *dds_ptr = 0;
            if (dds_cache && (dds_ptr = static_cast<DDS*>(dds_cache->get(dataset_name)))) {
                // Use the cached DDS; Assume that all cached DDS objects hold DAS info too
                BESDEBUG(NC_NAME, "DDS Cached hit (while building DMR) for : " << dataset_name << endl);

                dmr->build_using_dds(*dds_ptr);
            }
            else {
                // Build a throw-away DDS; don't bother to cache it. DMR's don't support
                // containers.
                BaseTypeFactory factory;
                DDS dds(&factory, name_path(dataset_name), "3.2");

                dds.filename(dataset_name);
                nc_read_dataset_variables(dds, dataset_name);

                DAS das;

                nc_read_dataset_attributes(das, dataset_name);
                Ancillary::read_ancillary_das(das, dataset_name);

                dds.transfer_attributes(&das);
                dmr->build_using_dds(dds);
            }
#endif

            if (dmr_cache) {
                // add a copy
                BESDEBUG(NC_NAME, "DMR added to the cache for : " << dataset_name << endl);
                dmr_cache->add(new DMR(*dmr), dataset_name);
            }
        }

        // Instead of fiddling with the internal storage of the DHI object,
        // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
        // methods to set the constraints. But, why? Ans: from Patrick is that
        // in the 'container' mode of BES each container can have a different
        // CE.
        bdmr.set_dap4_constraint(dhi);
        bdmr.set_dap4_function(dhi);
    }
	catch (InternalErr &e) {
		throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (Error &e) {
		throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (...) {
		throw BESDapError("Caught unknown error build NC DMR response", true, unknown_error, __FILE__, __LINE__);
	}

	return true;
}

bool NCRequestHandler::nc_build_help(BESDataHandlerInterface & dhi)
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("NCRequestHandler::nc_build_help", dhi.data[REQUEST_ID]);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *> (response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    map < string, string > attrs;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
#if 0
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
#endif
    list < string > services;
    BESServiceRegistry::TheRegistry()->services_handled(NC_NAME, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

bool NCRequestHandler::nc_build_version(BESDataHandlerInterface & dhi)
{
	BESStopWatch sw;
	if (BESISDEBUG( TIMING_LOG ))
		sw.start("NCRequestHandler::nc_build_version", dhi.data[REQUEST_ID]);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *> (response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

#if 0
    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);
#endif
    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true;
}
/**
 * Starting at pos, look for the next closing (right) parenthesis. This code
 * will count opening (left) parens and find the closing paren that maches
 * the first opening paren found. Examples: "0123)56" --> 4; "0123(5)" --> 6;
 * "01(3(5)7)9" --> 8.
 *
 * This function is intended to help split up a constraint expression so that
 * the server functions can be processed separately from the projection and
 * selection parts of the CE.
 *
 * @param ce The constraint to look in
 * @param pos Start looking at this position; zero-based indexing
 * @return The position in the string where the closing paren was found
 */
static string::size_type find_closing_paren(const string &ce, string::size_type pos)
{
    // Iterate over the string finding all ( or ) characters until the matching ) is found.
    // For each ( found, increment count. When a ) is found and count is zero, it is the
    // matching closing paren, otherwise, decrement count and keep looking.
    int count = 1;
    do {
        pos = ce.find_first_of("()", pos + 1);
        if (pos == string::npos)
            throw Error(malformed_expr, "Expected to find a matching closing parenthesis in " + ce);

        if (ce[pos] == '(')
            ++count;
        else
            --count;	// must be ')'

    } while (count > 0);

    return pos;
}



bool NCRequestHandler::is_function_used(const ConstraintEvaluator &eval, const string &t_constraint) {

    bool ret_value = false;

    if(t_constraint!="") {
        string::size_type pos = 0;
        string::size_type first_paren = t_constraint.find("(", pos);
        string::size_type closing_paren = string::npos;
        if (first_paren != string::npos) 
            closing_paren = find_closing_paren(t_constraint, first_paren); //ce.find(")", pos);

        while (first_paren != string::npos && closing_paren != string::npos) {

            // Maybe a BTP function; get the name of the potential function
            string btp_name = t_constraint.substr(pos, first_paren - pos);
            BESDEBUG("nc", " KENT BTP name is : " << btp_name << endl);

            // is this a BTP function
            btp_func f;
            if (eval.find_function(btp_name, &f)) {
                ret_value = true;
                break;
            }
            else {
                pos = closing_paren + 1;
                // exception?
                if (pos < t_constraint.length() && t_constraint.at(pos) == ',') ++pos;
            }

            first_paren = t_constraint.find("(", pos);
            closing_paren = t_constraint.find(")", pos);
        }        
    }

    return ret_value;
}

  
// https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string/236803#236803
vector<string> NCRequestHandler::split_string(const string &text,char sep) {

    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;

}

inline bool NCRequestHandler::has_same_num_spaces(const string & str,const int num_indent_spaces) {
    int str_num_indent_spaces = 0;
    for (unsigned int i = 0; i<str.size(); i++) {
        if(str[i] ==' ')
            str_num_indent_spaces++;
        else
            break;
    }
    return (str_num_indent_spaces==num_indent_spaces);

}

inline bool NCRequestHandler::find_in_the_vector(int i, const vector<int>& sorted_vec) {
    vector<int>::const_iterator low;
    low = lower_bound(sorted_vec.begin(),sorted_vec.end(),i);
    if(*low == i)
        return true;
    else
      	return false;
}

string NCRequestHandler::obtain_reduced_dds(const string& dds_str, const string& constraint_str) {

    
    vector<string> dds_content_in_vector;
    vector<int> cvar_index;
    vector<string> dds_cover_in_vector;
    reconstruct_dds(dds_str,dds_content_in_vector,cvar_index,dds_cover_in_vector);
//#if 0
    // Obtain variable names from the new DDS vector string.
    vector <string>new_str_var_vec;
    unsigned int dds_content_vector_size = dds_content_in_vector.size();
    new_str_var_vec.resize(dds_content_vector_size);
    if(cvar_index.size() == dds_content_vector_size) {//All complex
        for (unsigned int i = 0; i <dds_content_vector_size;i++) {
	        string temp_str = dds_content_in_vector[i];
            size_t var_ep = temp_str.find_last_of(';');		
            size_t var_sp = temp_str.find_last_of('}',var_ep-1);
            new_str_var_vec[i] = temp_str.substr(var_sp+2,var_ep-var_sp-2);
        }
    }

    else if(cvar_index.size() == 0) {// Atomic
        for (unsigned int i = 0; i <dds_content_vector_size;i++) {
             string temp_str = dds_content_in_vector[i];
             size_t var_type_sp = temp_str.find_first_not_of(' ');
             size_t var_sp = temp_str.find_first_of(' ',var_type_sp)+1;
             size_t var_ep = temp_str.find_first_of('[',var_sp);
             if(var_ep ==string::npos) 
                var_ep = temp_str.find_last_of(';');
             new_str_var_vec[i] = temp_str.substr(var_sp,var_ep-var_sp);
        }
    }

    else {//Mix
        for (unsigned int i = 0; i <dds_content_vector_size;i++) {
	        if(true == find_in_the_vector(i,cvar_index)){
	            string temp_str = dds_content_in_vector[i];
                size_t var_ep = temp_str.find_last_of(';');		
                size_t var_sp = temp_str.find_last_of('}',var_ep-1);
                new_str_var_vec[i] = temp_str.substr(var_sp+2,var_ep-var_sp-2);
            }
            else {
                string temp_str = dds_content_in_vector[i];
                size_t var_type_sp = temp_str.find_first_not_of(' ');
                size_t var_sp = temp_str.find_first_of(' ',var_type_sp)+1;
                size_t var_ep = temp_str.find_first_of('[',var_sp);
                if(var_ep ==string::npos) 
                    var_ep = temp_str.find_last_of(';');
                new_str_var_vec[i] = temp_str.substr(var_sp,var_ep-var_sp);
            }
        }
    }
for (unsigned int i = 0; i <dds_content_vector_size;i++) {
	cout <<i <<endl;
    cout <<new_str_var_vec[i]<<endl;
}

    vector<string> candidate_str = obtain_vars_from_constraint(constraint_str);
#if 0
vector<string> candidate_str;
candidate_str.push_back("v");
candidate_str.push_back("time");
candidate_str.push_back("scan_line");
candidate_str.push_back("codec_name");
#endif

    vector<string> select_str;

    for (unsigned int i =0; i<candidate_str.size();i++) {
        for (unsigned int j =0; j<new_str_var_vec.size();j++) {
            if(candidate_str[i] == new_str_var_vec[j]) {
                select_str.push_back(dds_content_in_vector[j]);
                break;
            }
        }
    }
for (unsigned int i = 0; i <candidate_str.size();i++) {
	cout <<i <<endl;
    cout <<select_str[i]<<endl;
}
//#endif
    string final_reduced_dds;
    // Combine starting line, select_str and ending line to one DDS.
    final_reduced_dds = dds_cover_in_vector[0];
    for(vector<string>::const_iterator istr= select_str.begin();istr!=select_str.end();++istr)
        final_reduced_dds +=*istr;
    final_reduced_dds += dds_cover_in_vector[1];
cout<<"Final reduced DDS " <<endl;
cout<<final_reduced_dds<<endl;
     
    return final_reduced_dds;
}

// cvar: var with the complex type.
void NCRequestHandler::reconstruct_dds(const string& dds_str,vector<string>& dds_content_in_vector,vector<int>&cvar_index,vector<string>&dds_cover_in_vector) {

//#if 0
    cerr<<dds_str <<endl;

    // Split the string to a vector. Each element is a new line.
    // Pop out the last element since it is one after \n. 
    char sep = '\n';
    vector<string> tstr_vec= split_string(dds_str,sep);
    if(tstr_vec.size()<=2) {
        cerr<<"the DDS cannot be splitted, fall back to the old way.\n";
        throw BESInternalError("DDS needs to contain at least two lines.", __FILE__, __LINE__);
    }
    else
        tstr_vec.pop_back();

    // Obtain the first line and the last line. 
    // This will be used in the new DDS.
    string start_line = tstr_vec[0] +'\n';
    string end_line = tstr_vec.back()+'\n';
    cerr<<"start_line is "<<start_line <<endl;
    cerr<<"end_line is "<<end_line <<endl;
    dds_cover_in_vector.push_back(start_line);
    dds_cover_in_vector.push_back(end_line);

    // Re-organize/resume the number of elements for the body part.
    tstr_vec.pop_back();
    tstr_vec.erase(tstr_vec.begin());

    for (unsigned int i = 0; i <tstr_vec.size(); i++) {
      tstr_vec[i]+='\n';
      cerr<<"tstr_vec["<<i<<"] = "<<tstr_vec[i] <<endl;
    }


    char space_char=' ';
    string temp_str = tstr_vec[0];
    if(temp_str[0]!=space_char) {
       cerr<<"the variable in the DDS must start from a space. \n";
       throw BESInternalError("The variable in the DDS must start from a space.", __FILE__, __LINE__);
    }
    // Count the number of spaces before a non-space character at the first line
    int num_indent_spaces = 0;
    for (unsigned int i = 0; i<temp_str.size(); i++) {
        if(temp_str[i] ==space_char)
            num_indent_spaces++;
        else
            break;
    }
    cerr<<"num_indent_spaces is "<<num_indent_spaces <<endl;

    //May not do this: TODO: check the first and last line whether containing { and }, if not. an error.
    char start_bracket='{';
    char end_bracket='}';

    // Find the outer most { and } positions.
    // sb_list: list of {(start) positions
    // eb_list: list of }(end) positions
    // db_list: list of subtraction(difference) of } to { positions.
    list<int> sb_list;
    list<int> eb_list;
    list<int> db_list;

    int b_list_size = 0;
    for (unsigned int i =0; i <tstr_vec.size();i++) {
        if(tstr_vec[i].find(start_bracket,0)!=string::npos) {
           if(true == has_same_num_spaces(tstr_vec[i],num_indent_spaces)) {
              //insert the list of { index
              sb_list.push_back(i);
              b_list_size++;
           }
        }
        if(tstr_vec[i].find(end_bracket,0)!=string::npos) {
           if(true == has_same_num_spaces(tstr_vec[i],num_indent_spaces)) {
              eb_list.push_back(i);
              //insert the list of } index

           }
        }
    }

    cout<<"starting bracket \n";
    for (list<int>::iterator it=sb_list.begin(); it!=sb_list.end();++it) 
        cout<<' '<<*it;
    cout <<endl;

    cout <<"ending bracket \n";
    for (list<int>::iterator it=eb_list.begin(); it!=eb_list.end();++it) 
        cout<<' '<<*it;
    cout <<endl;


    // Better to have a check: TODO: Need to have a sanity check of { and } brackets. 
    // 1. The same number
    // 2. The order is always { then }.} should always be greater than {.
    // Form a vector with one { and one } in order. Then check if the vector is sorted
    // wit(c++ 11 has a STL is_sorted can be used if compiling with C++ 11.
    // bool test(vector<int> v) {
    //     for(int i = 0; i < v.size()-1; i++)
    //             if(v[i] > v[i+1])
    //                         return false;
    //                             return true;
    //                             }
    //

    // 
    // The following while loop does three things:
    // 1) Calculate the total number of lines for lines with all outer brackets.
    // 2) Obtain the list of position subtraction from } to {.
    // 3) Generate a new list of string vb(value within brackets) for all bracket pairs.
    list<int>::iterator it_sb = sb_list.begin();
    list<int>::iterator it_eb = eb_list.begin();

    list<string> vb;
    int num_v_lines = 0;
    while(it_sb!=sb_list.end() && it_eb!=eb_list.end()) 
    {
        string temp_str;
        // This for loop groups one complex type into one string
        for(int i = (*it_sb); i<=(*it_eb);i++) {
            temp_str += tstr_vec[i];
            num_v_lines++;
        }
        db_list.push_back((*it_eb)-(*it_sb));
        vb.push_back(temp_str);
        it_sb++;
        it_eb++;
    }

    cout <<"number of v lines is "<<num_v_lines <<endl;
    cout <<"bracket elements \n";
    for (list<string>::iterator it=vb.begin(); it!=vb.end();++it) 
        cout<<*it <<endl;
    cout <<endl;

    // Obtain the size of new vector DDS.
    int new_str_vec_size = tstr_vec.size()-num_v_lines+b_list_size;
    cout <<"new_str_vec_size is "<<new_str_vec_size <<endl;

    // Need to calculate the bracket position at the new string.
    int tc = 0;

    //vector<string> new_str_vec;
    //new_str_vec.resize(new_str_vec_size);
    dds_content_in_vector.resize(new_str_vec_size);

    // index of nb_list
    //vector<int> nb_list;

    for (int i = 0; i <new_str_vec_size;i++) {
        // Check if sb_list is empty
        if(sb_list.empty()!=true) {
            // If not empty, check if the position in the
            // original vector is the same as the position of {
            // tc is the incremental counter of the position.
            if((i+tc) == sb_list.front()) {
                dds_content_in_vector[i] = vb.front();
                // Need to increase the number of lines within {};
                tc += db_list.front();
                // After this position, pop it out of the list.
                sb_list.pop_front();
                db_list.pop_front();
                vb.pop_front();
                //nb_list.push_back(i);
                cvar_index.push_back(i);
            }
            else 
                dds_content_in_vector[i] = tstr_vec[i+tc];
        }
        else 
            dds_content_in_vector[i] = tstr_vec[i+tc];

    }

    // Check the final output
    for (int i = 0; i <new_str_vec_size;i++) {
        cout <<i <<endl;
        cout <<dds_content_in_vector[i]<<endl;
    }

//#endif
}

vector<string> NCRequestHandler::obtain_vars_from_constraint(const string& constraint_str) {

    //string tstr = "temp,u[0:10:15][0:1:2],AC.BD[0:1:2],v,w[0:1:2]";
    //cerr<<tstr <<endl;

    // Split the string to a vector. Each element is a new line.
    // Pop out the last element since it is one after \n. 
    char sep = ',';
    vector<string> tstr_vec= split_string(constraint_str,sep);
//#if 0
    for (unsigned int i = 0; i <tstr_vec.size(); i++) {
        cerr<<"tstr_vec["<<i<<"] = "<<tstr_vec[i] <<endl;
        string tempstr = tstr_vec[i];
        size_t end = 0;
        if((end=tempstr.find('.'))!=string::npos)
            tstr_vec[i] = tempstr.substr(0,end);
        else if((end=tempstr.find('['))!=string::npos)
            tstr_vec[i] = tempstr.substr(0,end);
    }

    cout <<"After modifying "<<endl;
    for (unsigned int i = 0; i <tstr_vec.size(); i++) 
        cerr<<"tstr_vec["<<i<<"] = "<<tstr_vec[i] <<endl;
//#endif
    return tstr_vec;


}

