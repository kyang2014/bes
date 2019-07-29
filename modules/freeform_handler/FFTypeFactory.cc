
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2005 OPeNDAP, Inc.
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


#include <string>
#include <BESError.h>
#include <BESDebug.h>

#include "FFByte.h"
#include "FFInt16.h"
#include "FFUInt16.h"
#include "FFInt32.h"
#include "FFUInt32.h"
#include "FFFloat32.h"
#include "FFFloat64.h"
#include "FFStr.h"
#include "FFUrl.h"
#include "FFArray.h"
#include "FFStructure.h"
#include "FFSequence.h"
#include "FFGrid.h"

#include "BaseTypeFactory.h"
#include "FFTypeFactory.h"

#include "debug.h"

using namespace libdap;
using namespace std;

BaseType *FFTypeFactory::NewVariable(Type t, const string &name) const
{
	switch (t) {
	case dods_byte_c:
		return NewByte(name);
	//case dods_char_c:
	//	return NewChar(name);

	//case dods_uint8_c:
	//	return NewUInt8(name);
	//case dods_int8_c:
	//	return NewInt8(name);

	case dods_int16_c:
		return NewInt16(name);
	case dods_uint16_c:
		return NewUInt16(name);
	case dods_int32_c:
		return NewInt32(name);
	case dods_uint32_c:
		return NewUInt32(name);

	//case dods_int64_c:
	//	return NewInt64(name);
	//case dods_uint64_c:
	//	return NewUInt64(name);

	case dods_float32_c:
		return NewFloat32(name);
	case dods_float64_c:
		return NewFloat64(name);

	case dods_str_c:
		return NewStr(name);
	case dods_url_c:
		return NewUrl(name);

	case dods_array_c:
		return NewArray(name);

	case dods_structure_c:
		return NewStructure(name);

	case dods_sequence_c:
		return NewSequence(name);

	default:
		throw BESError("Unimplemented type in DAP2.", BES_INTERNAL_ERROR, __FILE__, __LINE__);
	}
}


Byte *
FFTypeFactory::NewByte(const string &n ) const 
{ 
    return new FFByte(n,fd_name);
}

Int16 *
FFTypeFactory::NewInt16(const string &n ) const 
{ 
    return new FFInt16(n,fd_name); 
}

UInt16 *
FFTypeFactory::NewUInt16(const string &n ) const 
{ 
    return new FFUInt16(n,fd_name);
}

Int32 *
FFTypeFactory::NewInt32(const string &n ) const 
{ 
    DBG(cerr << "Inside FFTypeFactory::NewInt32" << endl);
    return new FFInt32(n,fd_name);
}

UInt32 *
FFTypeFactory::NewUInt32(const string &n ) const 
{ 
    return new FFUInt32(n,fd_name);
}

Float32 *
FFTypeFactory::NewFloat32(const string &n ) const 
{ 
    return new FFFloat32(n,fd_name);
}

Float64 *
FFTypeFactory::NewFloat64(const string &n ) const 
{ 
    return new FFFloat64(n,fd_name);
}

Str *
FFTypeFactory::NewStr(const string &n ) const 
{ 
    return new FFStr(n,fd_name);
}

Url *
FFTypeFactory::NewUrl(const string &n ) const 
{ 
    return new FFUrl(n,fd_name);
}

Array *
FFTypeFactory::NewArray(const string &n , BaseType *v) const 
{ 
    return new FFArray(n,fd_name,v,fd_iff);
}

Structure *
FFTypeFactory::NewStructure(const string &n ) const 
{ 
    return new FFStructure(n,fd_name);
}

Sequence *
FFTypeFactory::NewSequence(const string &n ) const 
{
    DBG(cerr << "Inside FFTypeFactory::NewSequence" << endl);
    return new FFSequence(n,fd_name,fd_iff);
}

Grid *
FFTypeFactory::NewGrid(const string &n ) const 
{ 
    return new FFGrid(n,fd_name);
}
