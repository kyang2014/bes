
// -*- mode: c++; c-basic-offset:4 -*-

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
// more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef ff_factory_h
#define ff_factory_h

#include <string>

#include "BaseTypeFactory.h"

class FFByte;
class FFInt16;
class FFUInt16;
class FFInt32;
class FFUInt32;
class FFFloat32;
class FFFloat64;
class FFStr;
class FFUrl;
class FFArray;
class FFStructure;
class FFSequence;
class FFGrid;

/** A factory for the FFByte, ..., FFGrid types.
    @author James Gallagher */
class FFTypeFactory : public libdap::BaseTypeFactory {
public:
    FFTypeFactory() {} 
    FFTypeFactory(const string & d_name,const string &d_iff):fd_name(d_name),fd_iff(d_iff),fbt(NULL) { }
    virtual ~FFTypeFactory() {}

    virtual libdap::BaseType *NewVariable(libdap::Type t, const std::string &name) const;
    virtual libdap::Byte *NewByte(const std::string &n = "") const;
    virtual libdap::Int16 *NewInt16(const std::string &n = "") const;
    virtual libdap::UInt16 *NewUInt16(const std::string &n = "") const;
    virtual libdap::Int32 *NewInt32(const std::string &n = "") const;
    virtual libdap::UInt32 *NewUInt32(const std::string &n = "") const;
    virtual libdap::Float32 *NewFloat32(const std::string &n = "") const;
    virtual libdap::Float64 *NewFloat64(const std::string &n = "") const;

    virtual libdap::Str *NewStr(const std::string &n = "") const;
    virtual libdap::Url *NewUrl(const std::string &n = "") const;

    virtual libdap::Array *NewArray(const std::string &n = "", libdap::BaseType *v = 0) const;
    virtual libdap::Structure *NewStructure(const std::string &n = "") const;
    virtual libdap::Sequence *NewSequence(const std::string &n = "") const;
    virtual libdap::Grid *NewGrid(const std::string &n = "") const;
    //virtual libdap::D4Sequence *NewD4Sequence(const std::string &n="") const;
    void set_fd_iff(const string & iff) {fd_iff = iff;}
private:
    string fd_name;
    string fd_iff;
    BaseType* fbt;
};

#endif // ff_factory_h
