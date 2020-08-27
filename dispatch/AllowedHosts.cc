// RemoteAccess.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the OPeNDAP Back-End Server (BES)
// and embodies a whitelist of remote system that may be
// accessed by the server as part of it's routine operation.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan D. Potter <ndp@opendap.org>
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

#include <BESUtil.h>
#include <BESCatalog.h>
#include <BESCatalogList.h>
#include <BESCatalogUtils.h>
#include <BESRegex.h>
#include <TheBESKeys.h>
#include <BESInternalError.h>
#include <BESSyntaxUserError.h>
#include <BESDebug.h>
#include <BESNotFoundError.h>
#include <BESForbiddenError.h>

#include "AllowedHosts.h"

using namespace std;
using namespace bes;

#define MODULE "ah"
#define prolog string("AllowedHosts::").append(__func__).append("() - ")

AllowedHosts *AllowedHosts::d_instance = 0;

/**
 * @brief Static accessor for the singleton
 *
 * @return A pointer to the singleton instance
 */
AllowedHosts *
AllowedHosts::theHosts()
{
    if (d_instance) return d_instance;
    d_instance = new AllowedHosts;
    return d_instance;
}

AllowedHosts::AllowedHosts()
{
    bool found = false;
    string key = ALLOWED_HOSTS_BES_KEY;
    TheBESKeys::TheKeys()->get_values(ALLOWED_HOSTS_BES_KEY, d_white_list, found);
    if(!found){
        throw BESInternalError(string("The remote access whitelist, '") + ALLOWED_HOSTS_BES_KEY
                               + "' has not been configured.", __FILE__, __LINE__);
    }
}

/**
 * This method provides an access condition assessment for URLs and files
 * to be accessed by the BES. The http and https URLs are verified against a
 * whitelist assembled from configuration. All file URLs are checked to be
 * sure that they reference a resource within the BES default catalog.
 *
 * @note AllowedHosts is a singleton. This method will instantiate the class
 * if that has not already been done. This method should only be called from
 * the main thread of a multi-threaded application.
 *
 * @param url The URL to test
 * @return True if the URL may be dereferenced, given the BES's configuration,
 * false otherwise.
 */
bool AllowedHosts::is_allowed(const std::string &url)
{
    bool isAllowed = false;
    const string file_url("file://");
    const string http_url("http://");
    const string https_url("https://");

    // Special case: This allows any file: URL to pass if the URL starts with the default
    // catalog's path.
    if (url.compare(0, file_url.size(), file_url) == 0 /*equals a file url*/) {

        // Ensure that the file path starts with the catalog root dir.
        string file_path = url.substr(file_url.size());
        BESDEBUG(MODULE, prolog << "file_path: "<< file_path << endl);

        BESCatalog *bcat = BESCatalogList::TheCatalogList()->find_catalog(BES_DEFAULT_CATALOG);
        if (bcat) {
            BESDEBUG(MODULE, prolog << "Found catalog: "<< bcat->get_catalog_name() << endl);
        }
        else {
            string msg = "OUCH! Unable to locate default catalog!";
            BESDEBUG(MODULE, prolog << msg << endl);
            throw BESInternalError(msg, __FILE__, __LINE__);
        }

        string catalog_root = bcat->get_root();
        BESDEBUG(MODULE, prolog << "Catalog root: "<< catalog_root << endl);


        // Never a relative path shall be accepted.
        // change??
       // if( file_path[0] != '/'){
       //     file_path.insert(0,"/");
        //}

        string relative_path;
        if(file_path[0] == '/'){
            if(file_path.length() < catalog_root.length()) {
                isAllowed = false;
            }
            else {
                int ret = file_path.compare(0, catalog_root.npos, catalog_root) == 0;
                BESDEBUG(MODULE, prolog << "file_path.compare(): " << ret << endl);
                isAllowed = (ret==0);
                relative_path = file_path.substr(catalog_root.length());
            }
        }
        else {
            BESDEBUG(MODULE, prolog << "Relative path detected");
            relative_path = file_path;
            isAllowed = true;
        }

        // string::compare() returns 0 if the path strings match exactly.
        // And since we are just looking at the catalog.root as a prefix of the resource
        // name we only allow to be white-listed for an exact match.
        if(isAllowed){
            // If we stop adding a '/' to file_path values that don't begin with one
            // then we need to detect the use of the relative path here
            bool follow_sym_links = bcat->get_catalog_utils()->follow_sym_links();
            try {
                BESUtil::check_path(relative_path, catalog_root, follow_sym_links);
            }
            catch (BESNotFoundError &e) {
                isAllowed=false;
            }
            catch (BESForbiddenError &e) {
                isAllowed=false;
            }
        }


        BESDEBUG(MODULE, prolog << "File Access Allowed: "<< (isAllowed?"true ":"false ") << endl);
    }
    else {
        vector<string>::const_iterator i = d_white_list.begin();
        vector<string>::const_iterator e = d_white_list.end();
        for (; i != e && !isAllowed; i++) {
            string reg = *i;
            BESRegex reg_expr(reg.c_str());
            if (reg_expr.match(url.c_str(), url.length()) > 0 ) {
                isAllowed = true;;
            }
        }
        BESDEBUG(MODULE, prolog << "HTTP Access Allowed: "<< (isAllowed?"true ":"false ") << endl);
    }
    return isAllowed;
}
