// FONcArray.h

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

#ifndef FONcArray_h_
#define FONcArray_h_ 1

#include <vector>
#include <string>
#include "AttrTable.h"

#include "FONcBaseType.h"

class FONcDim;
class FONcMap;

namespace libdap {
class BaseType;
class Array;
}

/** @brief A DAP Array with file out netcdf information included
 *
 * This class represents a DAP Array with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP Array being converted.
 */
class FONcArray: public FONcBaseType {
private:
    // The array being converted
    libdap::Array * d_a;
    // The type of data stored in the array
    nc_type d_array_type;
    // The number of dimensions to be stored in netcdf (if string, 2)
    int d_ndims;
    // The actual number of dimensions of this array (if string, 1)
    int d_actual_ndims;
    // The number of elements that will be stored in netcdf
    int d_nelements;
    // The FONcDim dimensions to be used for this variable
    std::vector<FONcDim *> d_dims;
    // The netcdf dimension ids for this array from DAP4
    std::vector<int> d4_dim_ids;
    std::vector<bool>use_d4_dim_ids;
    std::vector<int> d4_rbs_nums;
    //std::vector<int> d4_rbs_nums_visited;

    // The netcdf dimension ids for this array
    std::vector<int> d_dim_ids;
    // The netcdf dimension sizes to be written
    //size_t * d_dim_sizes; // changed int to size_t. jhrg 12.27.2011
    std::vector<size_t> d_dim_sizes;
    // If string data, we need to do some comparison, so instead of
    // reading it more than once, read it once and save here
    std::vector<std::string> d_str_data;

    // If the array is already a map in a grid, then we don't want to
    // define it or write it.
    bool d_dont_use_it;

    // Make this a vector<> jhrg 10/12/15
    // The netcdf chunk sizes for each dimension of this array.
    std::vector<size_t> d_chunksizes;

    // This is vector holds instances of FONcMap* that wrap existing Array
    // objects that are pushed onto the global FONcGrid::Maps vector. These
    // are hand made reference counting pointers. I'm not sure we need to
    // store copies in this object, but it may be the case that without
    // calling the FONcMap->decref() method they are not deleted. jhrg 8/28/13
    std::vector<FONcMap*> d_grid_maps;

    // if DAP4 dim. is defined
    bool d4_def_dim;
    FONcDim * find_dim(std::vector<std::string> &embed, const std::string &name, int size, bool ignore_size = false);

    void write_for_nc4_types(int ncid);

public:
    FONcArray(libdap::BaseType *b);
    FONcArray(libdap::BaseType *b,const std::vector<int>&dim_ids,const std::vector<bool>&use_dim_ids,const std::vector<int>&rbs_nums);
    virtual ~FONcArray();

    virtual void convert(std::vector<std::string> embed,bool is_dap4_group=false);
    virtual void define(int ncid);
    virtual void write(int ncid);

    virtual std::string name();
    virtual libdap::Array *array()
    {
        return d_a;
    }

    virtual void dump(std::ostream &strm) const;

    static std::vector<FONcDim *> Dimensions;
    static libdap::AttrType getAttrType(nc_type t);
};

#endif // FONcArray_h_

