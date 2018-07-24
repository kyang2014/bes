// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of cmr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.
//
// Copyright (c) 2018 OPeNDAP, Inc.
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
//
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"

#include <BESError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include "RemoteHttpResource.h"


#include "rjson_utils.h"

using namespace std;

#define MODULE "cmr"

namespace cmr {
/**
 * Utilizes the RemoteHttpResource machinery to retrieve the document
 * referenced by the parameter 'url'. Once retrieved the document is fed to the RapidJSON
 * parser to populate the parameter 'd'
 *
 * @param url The URL of the JSON document to parse.
 * @param doc The document that will hopd the parsed result.
 *
 */
void
rjson_utils::getJsonDoc(const string &url, rapidjson::Document &doc){
    string prolog = string("rjson_utils::") + __func__ + "() - ";
    BESDEBUG(MODULE,prolog << "Trying url: " << url << endl);
    cmr::RemoteHttpResource rhr(url);
    rhr.retrieveResource();
    vector<string> *headers = rhr.getResponseHeaders();
    if(BESDebug::IsSet(MODULE)){
        stringstream msg(prolog);
        msg << "HTTP Headers:" << endl;
        for(unsigned long i=0; i<headers->size() ;i++){
            msg << (*headers)[i] << endl;
        }
        *(BESDebug::GetStrm()) << msg.str();
    }
    FILE* fp = fopen(rhr.getCacheFileName().c_str(), "r"); // non-Windows use "r"
    char readBuffer[65536];
    rapidjson::FileReadStream frs(fp, readBuffer, sizeof(readBuffer));
    doc.ParseStream(frs);
}


/**
 * Gets the child of 'object' named 'name' and returns it's value as a string.
 * If the 'name' is not a member of 'object', or if IsString() for the named child
 * returns false, then the empty string is returned.
 * @param object the object to serach.
 * @param name The name of the child object to convert to a string
 * @return The value of the named chalid as a string;
 */
std::string
rjson_utils::getStringValue(const rapidjson::Value& object, const string &name){
    string prolog = string("rjson_utils::") + __func__ + "() - ";

    string response;
    rapidjson::Value::ConstMemberIterator itr = object.FindMember(name.c_str());
    bool result  = itr != object.MemberEnd();
    string msg = prolog + (result?"Located":"FAILED to locate") + " the value '"+name+"' in object.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        return response;
    }

    const rapidjson::Value& myValue = itr->value;
    result = myValue.IsString();
    msg = prolog + "The value '"+ name +"' is" + (result?"":" NOT") + " a String type.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        return response;
    }

    return myValue.GetString();
}


/**
 * Converts a RapidJson Document object into a "pretty" string.
 *
 * @param d A reference to the document to convert.
 * @return The string manifestation of the JSON document.
 */
std::string
rjson_utils::jsonDocToString(rapidjson::Document &d){
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    return buffer.GetString();
}


}  // namespace cmr
