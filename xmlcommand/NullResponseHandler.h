// NullResponseHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 University Corporation for Atmospheric Research
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#ifndef NullResponseHandler_h
#define NullResponseHandler_h 1

#include "BESResponseHandler.h"

namespace bes {

/** @brief A ResponseHandler that does nothing
 *
 * Many commands don't send a response back to the BES's client. Instead they
 * modify the BES's current state. This ResponseHandler is useful when the
 * entire command's action (like setContexts) can be performed during the
 * XMLInterface::build_data_request_plan() phase of command evaluation. This
 * ResponseHandler will do only bookkeeping during its execute() method and
 * nothing at all during transmit().
 *
 * This ResponseHandler instance will not transmit anything back to the BES's
 * client unless there is an error.
 *
 * @see BESResponseObject
 */
class NullResponseHandler: public BESResponseHandler {
public:

    NullResponseHandler(const string &name): BESResponseHandler(name) { }
    virtual ~NullResponseHandler(void) { }

    virtual void execute(BESDataHandlerInterface &dhi);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    virtual void dump(ostream &strm) const;

    // Factory method, used by the DefaultModule to add this to the list of
    // ResponseHandlers for a given 'action'
    static BESResponseHandler *NullResponseBuilder(const string &name);
};

}   // namespace bes

#endif // NullResponseHandler_h

