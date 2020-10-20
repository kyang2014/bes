// FONcTransform.cc

// This file is part of BES Netcdf File Out Module

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <sstream>

using std::ostringstream;
using std::istringstream;

#include "FONcRequestHandler.h" // for the keys

#include "FONcTransform.h"
#include "FONcUtils.h"
#include "FONcBaseType.h"
#include "FONcAttributes.h"

#include <DDS.h>
#include <DMR.h>
#include <D4Group.h>
#include <D4Attributes.h>
#include <Structure.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <BESDebug.h>
#include <BESInternalError.h>

#include "DapFunctionUtils.h"

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FONcTransform::FONcTransform(DDS *dds, BESDataHandlerInterface &dhi, const string &localfile, const string &ncVersion) :
        _ncid(0), _dds(0)
{
    if (!dds) {
        string s = (string) "File out netcdf, " + "null DDS passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    if (localfile.empty()) {
        string s = (string) "File out netcdf, " + "empty local file name passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    _localfile = localfile;
    _dds = dds;
    _returnAs = ncVersion;

    // if there is a variable, attribute, dimension name that is not
    // compliant with netcdf naming conventions then we will create
    // a new name. If the new name does not begin with an alpha
    // character then we will prefix it with name_prefix. We will
    // get this prefix from the type of data that we are reading in,
    // such as nc, h4, h5, ff, jg, etc...
    dhi.first_container();
    if (dhi.container) {
        FONcUtils::name_prefix = dhi.container->get_container_type() + "_";
    }
    else {
        FONcUtils::name_prefix = "nc_";
    }
}
/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dmr DMR object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FONcTransform::FONcTransform(DMR *dmr, BESDataHandlerInterface &dhi, const string &localfile, const string &ncVersion) :
        _ncid(0), _dmr(0)
{
    if (!dmr) {
        string s = (string) "File out netcdf, " + "null DDS passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    if (localfile.empty()) {
        string s = (string) "File out netcdf, " + "empty local file name passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    _localfile = localfile;
    _dmr = dmr;
    _returnAs = ncVersion;

    // if there is a variable, attribute, dimension name that is not
    // compliant with netcdf naming conventions then we will create
    // a new name. If the new name does not begin with an alpha
    // character then we will prefix it with name_prefix. We will
    // get this prefix from the type of data that we are reading in,
    // such as nc, h4, h5, ff, jg, etc...
    dhi.first_container();
    if (dhi.container) {
        FONcUtils::name_prefix = dhi.container->get_container_type() + "_";
    }
    else {
        FONcUtils::name_prefix = "nc_";
    }
}


/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
FONcTransform::~FONcTransform()
{
    bool done = false;
    while (!done) {
        vector<FONcBaseType *>::iterator i = _fonc_vars.begin();
        vector<FONcBaseType *>::iterator e = _fonc_vars.end();
        if (i == e) {
            done = true;
        }
        else {
            // These are the FONc types, not the actual ones
            FONcBaseType *b = (*i);
            delete b;
            _fonc_vars.erase(i);
        }
    }
    done = false;
    while (!done) {
        vector<FONcBaseType *>::iterator i = _total_fonc_vars_in_grp.begin();
        vector<FONcBaseType *>::iterator e = _total_fonc_vars_in_grp.end();
        if (i == e) {
            done = true;
        }
        else {
            // These are the FONc types, not the actual ones
            FONcBaseType *b = (*i);
            delete b;
            _total_fonc_vars_in_grp.erase(i);
        }
    }

}

/** @brief Transforms each of the variables of the DataDDS to the NetCDF
 * file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
void FONcTransform::transform()
{
    FONcUtils::reset();

    // Convert the DDS into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.
    DDS::Vars_iter vi = _dds->var_begin();
    DDS::Vars_iter ve = _dds->var_end();
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc", "FONcTransform::transform() - Converting variable '" << v->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for 'v'
            FONcBaseType *fb = FONcUtils::convert(v,FONcTransform::_returnAs,FONcRequestHandler::classic_model);
#if 0
            fb->setVersion( FONcTransform::_returnAs );
            if ( FONcTransform::_returnAs == RETURNAS_NETCDF4 ) {
                if (FONcRequestHandler::classic_model)
                    fb->setNC4DataModel("NC4_CLASSIC_MODEL");
                else 
                    fb->setNC4DataModel("NC4_ENHANCED");
            }
#endif
            _fonc_vars.push_back(fb);

            vector<string> embed;
            fb->convert(embed);
        }
    }

    // Open the file for writing
    int stax;
    if ( FONcTransform::_returnAs == RETURNAS_NETCDF4 ) {
        if (FONcRequestHandler::classic_model){
            BESDEBUG("fonc", "FONcTransform::transform() - Opening NetCDF-4 cache file in classic mode. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4|NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG("fonc", "FONcTransform::transform() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG("fonc", "FONcTransform::transform() - Opening NetCDF-3 cache file. fileName:  " << _localfile << endl);
    	stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);
    }

    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, "File out netcdf, unable to open: " + _localfile, __FILE__, __LINE__);
    }

    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        nc_redef(_ncid);

        // For each converted FONc object, call define on it to define
        // that object to the netcdf file. This also adds the attributes
        // for the variables to the netcdf file
        vector<FONcBaseType *>::iterator i = _fonc_vars.begin();
        vector<FONcBaseType *>::iterator e = _fonc_vars.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform() - Defining variable:  " << fbt->name() << endl);
            fbt->define(_ncid);
        }

        if(FONcRequestHandler::no_global_attrs == false) {
            // Add any global attributes to the netcdf file
            AttrTable &globals = _dds->get_attr_table();
            BESDEBUG("fonc", "FONcTransform::transform() - Adding Global Attributes" << endl << globals << endl);
            bool is_netCDF_enhanced = false;
            if(FONcTransform::_returnAs == RETURNAS_NETCDF4 && FONcRequestHandler::classic_model==false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_attributes(_ncid, NC_GLOBAL, globals, "", "",is_netCDF_enhanced);
        }

        // We are done defining the variables, dimensions, and
        // attributes of the netcdf file. End the define mode.
        int stax = nc_enddef(_ncid);

        // Check error for nc_enddef. Handling of HDF failures
        // can be detected here rather than later.  KY 2012-10-25
        if (stax != NC_NOERR) {
            FONcUtils::handle_error(stax, "File out netcdf, unable to end the define mode: " + _localfile, __FILE__, __LINE__);
        }

        // Write everything out
        i = _fonc_vars.begin();
        e = _fonc_vars.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform() - Writing data for variable:  " << fbt->name() << endl);
            fbt->write(_ncid);
        }

        stax = nc_close(_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to close: " + _localfile, __FILE__, __LINE__);
    }
    catch (BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }
}

/** @brief Transforms each of the variables of the DMR to the NetCDF
 * file
 *
 * For each variable in the DMR write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DMR.
 */
void FONcTransform::transform_dap4()
{
    FONcUtils::reset();

    // Convert the DMR into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.
    
    BESDEBUG("fonc", "Coming into transform_dap4() "<< endl);

    bool support_group = check_group_support();
    if(true == support_group) {
        int stax = -1;
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
        stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4, &_ncid);
        if (stax != NC_NOERR) 
            FONcUtils::handle_error(stax, "File out netcdf, unable to open: " + _localfile, __FILE__, __LINE__);
        
        D4Group* root_grp = _dmr->root();
        map<string,int>fdimname_to_id;

        // Generate a list of the groups in the final netCDF file. The attributes of these groups should be included.
        gen_included_grp_list(root_grp);
#if 0
        for (std::set<string>::iterator it=_included_grp_names.begin(); it!=_included_grp_names.end(); ++it)
            BESDEBUG("fonc","included group list name is: "<<*it<<endl);
#endif
        transform_dap4_group(root_grp,true,_ncid,fdimname_to_id);
        stax = nc_close(_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to close: " + _localfile, __FILE__, __LINE__);

    }
    else 
        transform_dap4_no_group();

    return;

}

void FONcTransform::transform_dap4_no_group() {

    D4Group* root_grp = _dmr->root();

#if 0
    D4Dimensions *root_dims = root_grp->dims();
    for(D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
        BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
        BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
        BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
        BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
        //cout <<"dim size is: "<<(*di)->size()<<endl;
        //cout <<"dim fully_qualified_name is: "<<(*di)->fully_qualified_name()<<endl;
    }
#endif
    Constructor::Vars_iter vi = root_grp->var_begin();
    Constructor::Vars_iter ve = root_grp->var_end();

    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() - Converting variable '" << v->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for 'v'
            FONcBaseType *fb = FONcUtils::convert(v,FONcTransform::_returnAs,FONcRequestHandler::classic_model);
            _fonc_vars.push_back(fb);

            vector<string> embed;
            fb->convert(embed);
        }
    }

#if 0
    if(root_grp->grp_begin() == root_grp->grp_end()) 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - No group  " <<  endl);
    else 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - has group  " <<  endl);
   for (D4Group::groupsIter gi = root_grp->grp_begin(), ge = root_grp->grp_end(); gi != ge; ++gi) 
       BESDEBUG("fonc", "FONcTransform::transform_dap4() - group name:  " << (*gi)->name() << endl);
#endif

    // Open the file for writing
    int stax = -1;
    if ( FONcTransform::_returnAs == RETURNAS_NETCDF4 ) {
        if (FONcRequestHandler::classic_model){
            BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() - Opening NetCDF-4 cache file in classic mode. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4|NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() - Opening NetCDF-3 cache file. fileName:  " << _localfile << endl);
    	stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);
    }

    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, "File out netcdf, unable to open: " + _localfile, __FILE__, __LINE__);
    }

    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        nc_redef(_ncid);

        // For each converted FONc object, call define on it to define
        // that object to the netcdf file. This also adds the attributes
        // for the variables to the netcdf file
        vector<FONcBaseType *>::iterator i = _fonc_vars.begin();
        vector<FONcBaseType *>::iterator e = _fonc_vars.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() - Defining variable:  " << fbt->name() << endl);
            fbt->set_is_dap4(true);
            fbt->define(_ncid);
        }

        if(FONcRequestHandler::no_global_attrs == false) {

            // Add any global attributes to the netcdf file
            D4Group* root_grp=_dmr->root();
            D4Attributes*d4_attrs = root_grp->attributes();
            BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() handle GLOBAL DAP4 attributes "<< d4_attrs <<endl);
#if 0
            for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end(); ii != ee; ++ii) {
                string name = (*ii)->name();
                BESDEBUG("fonc", "FONcTransform::transform_dap4() GLOBAL attribute name is "<<name <<endl);
            }
#endif
            //    AttrTable &globals = root_grp->get_attr_table();
            BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() - Adding Global Attributes" << endl) ;
            bool is_netCDF_enhanced = false;
            if(FONcTransform::_returnAs == RETURNAS_NETCDF4 && FONcRequestHandler::classic_model==false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_dap4_attributes(_ncid, NC_GLOBAL, d4_attrs, "", "",is_netCDF_enhanced);
        }

        // We are done defining the variables, dimensions, and
        // attributes of the netcdf file. End the define mode.
        int stax = nc_enddef(_ncid);

        // Check error for nc_enddef. Handling of HDF failures
        // can be detected here rather than later.  KY 2012-10-25
        if (stax != NC_NOERR) {
            FONcUtils::handle_error(stax, "File out netcdf, unable to end the define mode: " + _localfile, __FILE__, __LINE__);
        }

        // Write everything out
        i = _fonc_vars.begin();
        e = _fonc_vars.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() - Writing data for variable:  " << fbt->name() << endl);
            fbt->write(_ncid);
        }

        stax = nc_close(_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to close: " + _localfile, __FILE__, __LINE__);
    }
    catch (BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }

}

void FONcTransform::transform_dap4_group(D4Group* grp,bool is_root_grp,int par_grp_id,map<string,int>&fdimname_to_id ) {

    bool included_grp = false;
    // Always include the root attributes.
    if(is_root_grp == true)  
        included_grp = true;
    else {
        // Check if this group is in the group list kept in the file.
        set<string>::iterator iset;
        if(_included_grp_names.find(grp->FQN())!=_included_grp_names.end())
            included_grp = true;
    }
     
    // If this group is not in the group list, we know all its subgroups are also not in the list, just stop and return.
    if(included_grp == true) 
        transform_dap4_group_internal(grp,is_root_grp,par_grp_id,fdimname_to_id);
    return;
}

void FONcTransform::transform_dap4_group_internal(D4Group* grp,bool is_root_grp,int par_grp_id,map<string,int>&fdimname_to_id ) {

    int grp_id = -1;
    int stax = -1;
    if(is_root_grp == true)  {
        grp_id = _ncid;
    }
    else {
        stax = nc_def_grp(par_grp_id,(*grp).name().c_str(),&grp_id);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to define group: " + _localfile, __FILE__, __LINE__);
        
    }
     
    D4Dimensions *root_dims = grp->dims();
    for(D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
#if 0
        BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
        BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
        BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
        BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
#endif
        unsigned long dimsize = (*di)->size();
        if((*di)->constrained()) {
            dimsize = ((*di)->c_stop() -(*di)->c_start())/(*di)->c_stride() +1;

        }
        int g_dimid = -1;
        stax = nc_def_dim(grp_id,(*di)->name().c_str(),dimsize,&g_dimid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to define dimension: " + _localfile, __FILE__, __LINE__);
        fdimname_to_id[(*di)->fully_qualified_name()] = g_dimid; 
    }

    Constructor::Vars_iter vi = grp->var_begin();
    Constructor::Vars_iter ve = grp->var_end();

    vector<FONcBaseType *> fonc_vars_in_grp;
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc", "FONcTransform::transform_dap4_group() - Converting variable '" << v->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for 'v'
            //FONcBaseType *fb = FONcUtils::convert(v,FONcTransform::_returnAs,FONcRequestHandler::classic_model);
            FONcBaseType *fb = FONcUtils::convert(v,RETURNAS_NETCDF4,false,fdimname_to_id);

            //_fonc_vars.push_back(fb);
            fonc_vars_in_grp.push_back(fb);

            // This is needed to avoid the memory leak.
            _total_fonc_vars_in_grp.push_back(fb);

            vector<string> embed;
            fb->convert(embed,true);
        }
    }

#if 0
    if(grp->grp_begin() == grp->grp_end()) 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - No group  " <<  endl);
    else 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - has group  " <<  endl);
#endif


    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        // TO CHECK: for netCDF4 group, this may NOT be necessary.
        //nc_redef(_ncid);

        vector<FONcBaseType *>::iterator i = fonc_vars_in_grp.begin();
        vector<FONcBaseType *>::iterator e = fonc_vars_in_grp.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform_dap4_group() - Defining variable:  " << fbt->name() << endl);
            fbt->set_is_dap4(true);
            fbt->define(grp_id);
        }

        bool is_netCDF_enhanced = false;
        if(FONcTransform::_returnAs == RETURNAS_NETCDF4 && FONcRequestHandler::classic_model==false)
                is_netCDF_enhanced = true;
 

        bool add_attr = true;
        if(FONcRequestHandler::no_global_attrs == true && is_root_grp == true) 
            add_attr= false;
        if(true == add_attr) {
            D4Attributes*d4_attrs = grp->attributes();
//if(true == is_root_grp)
//    BESDEBUG("fonc", "FONcTransform::This is root group." << endl) ;
            BESDEBUG("fonc", "FONcTransform::transform_dap4_group() - Adding Group Attributes" << endl) ;
            FONcAttributes::add_dap4_attributes(grp_id, NC_GLOBAL, d4_attrs, "", "",is_netCDF_enhanced);
        }

        // Write everything out
        i = fonc_vars_in_grp.begin();
        e = fonc_vars_in_grp.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform() - Writing data for variable in group:  " << fbt->name() << endl);
            //fbt->write(_ncid);
            fbt->write(grp_id);
        }

        for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
            BESDEBUG("fonc", "FONcTransform::transform_dap4() in group  - group name:  " << (*gi)->name() << endl);
            transform_dap4_group(*gi,false,grp_id,fdimname_to_id);
        }

    }
    catch (BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }

}



// Group support is only on when netCDF-4 is in enhanced model and there are groups in the DMR.
bool FONcTransform::check_group_support() {
    if(RETURNAS_NETCDF4 == FONcTransform::_returnAs && false == FONcRequestHandler::classic_model && 
       (_dmr->root()->grp_begin()!=_dmr->root()->grp_end())) 
        return true; 
    else 
        return false;
}

// Generate the final group list in the netCDF-4 file. Empty groups and their attributes will be removed.
void FONcTransform::gen_included_grp_list(D4Group*grp) 
{
    bool grp_has_var = false;
    if(grp) {
        BESDEBUG("fnoc", "<coming to the D4 group  has name " << grp->name()<<endl);
        BESDEBUG("fnoc", "<coming to the D4 group  has fullpath " << grp->FQN()<<endl);
        //We always include root attributes, so no need to obtain grp_names for the root.
        if(grp->var_begin()!=grp->var_end()) {
            BESDEBUG("fnoc", "<has the vars  " << endl);
            Constructor::Vars_iter vi = grp->var_begin();
            Constructor::Vars_iter ve = grp->var_end();

            for (; vi != ve; vi++) {
                if ((*vi)->send_p()) {
                    BaseType *v = *vi;
                    BESDEBUG("fonc", "FONcTransform::obtaining the group list that has variable '" << v->name() << "'" << endl);
                    grp_has_var = true;
                    //If a var in this group is selected, we need to include this group in the netcdf-4 file.
                    if(grp->FQN()!="/")  
                        _included_grp_names.insert(grp->FQN());
                    break;
                }
            }
        }
        // Loop through the subgroups to build up the list.
        for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
             BESDEBUG("fonc", "obtain included groups  - group name:  " << (*gi)->name() << endl);
             gen_included_grp_list(*gi);
        }
    }
        
    // If this group is in the final list, all its ancestors(except root, since it is always selected),should also be included. 
    //
    if(grp_has_var == true) {
        D4Group *temp_grp   = grp;
        while(temp_grp) {
            if(temp_grp->get_parent()){
                temp_grp = static_cast<D4Group*>(temp_grp->get_parent());
                if(temp_grp->FQN()!="/")  
                    _included_grp_names.insert(temp_grp->FQN());
            }
            else 
            temp_grp = 0;
        }
    }

}

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FONc objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONcTransform::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONcTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "ncid = " << _ncid << endl;
    strm << BESIndent::LMarg << "temporary file = " << _localfile << endl;
    BESIndent::Indent();
    vector<FONcBaseType *>::const_iterator i = _fonc_vars.begin();
    vector<FONcBaseType *>::const_iterator e = _fonc_vars.end();
    for (; i != e; i++) {
        FONcBaseType *fbt = *i;
        fbt->dump(strm);
    }
    BESIndent::UnIndent();
    BESIndent::UnIndent();
}


