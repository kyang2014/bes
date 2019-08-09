
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: Kent Yang <myang6@hdfgroup.org> James Gallagher <jgallagher@opendap.org>
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

#include "NCByte.h"
#include "NCInt16.h"
#include "NCUInt16.h"
#include "NCInt32.h"
#include "NCUInt32.h"
#include "NCFloat32.h"
#include "NCFloat64.h"
#include "NCStr.h"
#include "NCUrl.h"
#include "NCArray.h"
#include "NCStructure.h"
#include "NCSequence.h"
#include "NCGrid.h"

#include "BaseTypeFactory.h"
#include "NCTypeFactory.h"

#include "debug.h"
#include <netcdf.h>

using namespace libdap;
using namespace std;

BaseType *NCTypeFactory::NewVariable(Type t, const string &name) const
{
	switch (t) {
	case dods_byte_c:
		return NewByte(name);
	//case dods_char_c:
		//return NewByte(name);

	//case dods_uint8_c:
	//	return NewByte(name);
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
		//return NewArray(name,fbt);
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
NCTypeFactory::NewByte(const string &n ) const 
{ 
    return new NCByte(n,fd_name);
}

Int16 *
NCTypeFactory::NewInt16(const string &n ) const 
{ 
    return new NCInt16(n,fd_name); 
}

UInt16 *
NCTypeFactory::NewUInt16(const string &n ) const 
{ 
    return new NCUInt16(n,fd_name);
}

Int32 *
NCTypeFactory::NewInt32(const string &n ) const 
{ 
    DBG(cerr << "Inside NCTypeFactory::NewInt32" << endl);
    return new NCInt32(n,fd_name);
}

UInt32 *
NCTypeFactory::NewUInt32(const string &n ) const 
{ 
    return new NCUInt32(n,fd_name);
}

Float32 *
NCTypeFactory::NewFloat32(const string &n ) const 
{ 
    return new NCFloat32(n,fd_name);
}

Float64 *
NCTypeFactory::NewFloat64(const string &n ) const 
{ 
    return new NCFloat64(n,fd_name);
}

Str *
NCTypeFactory::NewStr(const string &n ) const 
{ 
    return new NCStr(n,fd_name);
}

Url *
NCTypeFactory::NewUrl(const string &n ) const 
{ 
    return new NCUrl(n,fd_name);
}

Array *
NCTypeFactory::NewArray(const string &n , BaseType *v) const 
{ 
    return new NCArray(n,fd_name,v);
}

Structure *
NCTypeFactory::NewStructure(const string &n ) const 
{ 
    return new NCStructure(n,fd_name);
}

Sequence *
NCTypeFactory::NewSequence(const string &n ) const 
{
    DBG(cerr << "Inside NCTypeFactory::NewSequence" << endl);
    return new NCSequence(n,fd_name);
}

Grid *
NCTypeFactory::NewGrid(const string &n ) const 
{ 
    return new NCGrid(n,fd_name);
}
