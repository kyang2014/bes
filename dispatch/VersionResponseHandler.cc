// VersionResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#include "config.h"

#include "VersionResponseHandler.h"
#include "DODSVersionInfo.h"
#include "cgi_util.h"
#include "util.h"
#include "dispatch_version.h"
#include "DODSParserException.h"
#include "DODSRequestHandlerList.h"
#include "DODSTokenizer.h"

VersionResponseHandler::VersionResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

VersionResponseHandler::~VersionResponseHandler( )
{
}

/** @brief executes the command 'show version;' by returning the version of
 * the OPeNDAP-g server and the version of all registered data request
 * handlers.
 *
 * This response handler knows how to retrieve the version of the OPeNDAP-g
 * server. It adds this information to a DODSVersionInfo informational response
 * object. It also forwards the request to all registered data request
 * handlers.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSVersionInfo
 * @see DODSRequestHandlerList
 */
void
VersionResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSVersionInfo *info =
	new DODSVersionInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    info->addDAPVersion( "2.0" ) ;
    info->addDAPVersion( "3.0" ) ;
    info->addDAPVersion( "3.2" ) ;
    info->addBESVersion( libdap_name(), libdap_version() ) ;
    info->addBESVersion( bes_name(), bes_version() ) ;
    DODSRequestHandlerList::TheList()->execute_all( dhi ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSResponseObject
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
VersionResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSVersionInfo *info = dynamic_cast<DODSVersionInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
VersionResponseHandler::VersionResponseBuilder( string handler_name )
{
    return new VersionResponseHandler( handler_name ) ;
}

// $Log: VersionResponseHandler.cc,v $
// Revision 1.4  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
