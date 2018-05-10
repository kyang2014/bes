// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
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

#include <XMLWriter.h>
#include <D4Group.h>
#include <InternalErr.h>

#include "DMRpp.h"
#include "DmrppCommon.h"

using namespace libdap;

namespace dmrpp {

const string dmrpp_namespace = "http://xml.opendap.org/dap/dmrpp/1.0.0#";

void DMRpp::print_dmrpp(XMLWriter &xml, bool constrained, bool print_chunks)
{
    bool pc_initial_value = DmrppCommon::d_print_chunks;
    DmrppCommon::d_print_chunks = print_chunks;

    try {
        if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar*) "Dataset") < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write Dataset element");

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "xmlns",
            (const xmlChar*) get_namespace().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for xmlns");

        // The dmrpp namespace
        if (DmrppCommon::d_print_chunks)
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "xmlns:dmrpp",
                (const xmlChar*) dmrpp_namespace.c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for xmlns:dmrpp");

        if (!request_xml_base().empty()) {
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "xml:base",
                (const xmlChar*) request_xml_base().c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for xml:base");
        }

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "dapVersion",
            (const xmlChar*) dap_version().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for dapVersion");

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "dmrVersion",
            (const xmlChar*) dmr_version().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for dapVersion");

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "name", (const xmlChar*) name().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");

        root()->print_dap4(xml, constrained);

        if (xmlTextWriterEndElement(xml.get_writer()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not end the top-level Group element");
    }
    catch (...) {
        DmrppCommon::d_print_chunks = pc_initial_value;
        throw;
    }

    DmrppCommon::d_print_chunks = pc_initial_value;
}

} /* namespace dmrpp */
