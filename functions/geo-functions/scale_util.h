
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

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

#ifndef _scale_util_h
#define _scale_util_h

#include <vector>
#include <string>

#include <gdal.h>
#include <gdal_priv.h>

#include <ServerFunction.h>

namespace libdap {

class Array;
class Grid;

struct SizeBox {
	int x_size;
	int y_size;

	SizeBox(int x, int y) : x_size(x), y_size(y) { }
	SizeBox(): x_size(0), y_size(0) { }
};

#if 0
struct GeoBox {
	double top;		// Latitude
	double bottom;	// Lat
	double left;	// Lon
	double right;	// Lon

	GeoBox(double t, double b, double l, double r) : top(t), bottom(b), left(l), right(r) { }
	GeoBox() : top(0.0), bottom(0.0), left(0.0), right(0.0) { }
};
#endif

SizeBox get_size_box(Array *lat, Array *lon);
std::vector<double> get_geotransform_data(Array *lat, Array *lon);
GDALDataType get_array_type(const Array *a);
void read_band_data(const Array *src, GDALRasterBand* band);
void add_band_data(const Array *src, GDALDataset* ds);
std::auto_ptr<GDALDataset> build_src_dataset(Array *data, Array *lon, Array *lat, const std::string &srs = "WGS84");
std::auto_ptr<GDALDataset> scale_dataset(std::auto_ptr<GDALDataset> src, const SizeBox &size,
    const std::string &interp = "nearest", const std::string &crs = "");
Array *build_array_from_gdal_dataset(std::auto_ptr<GDALDataset> dst, const Array *src);
void build_maps_from_gdal_dataset(GDALDataset *dst, Array *lon_map, Array *lat_map);

Grid *scale_dap_grid(Grid *src, SizeBox &size, const std::string &dest_crs, const std::string &interp);

void function_scale_grid(int argc, BaseType * argv[], DDS &, BaseType **btpp);
void function_scale_array(int argc, BaseType * argv[], DDS &, BaseType **btpp);

class ScaleGrid: public ServerFunction {
public:
    ScaleGrid()
    {
        setName("scale_grid");
        setDescriptionString("Scale a DAP2 Grid");
        setUsageString("scale_grid(Grid, Y size, X size, CRS, Interpolation method)");
        setRole("http://services.opendap.org/dap4/server-side-function/scale_grid");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_grid");
        setFunction(libdap::function_scale_grid);
        setVersion("1.0");
    }
    virtual ~ScaleGrid()
    {
    }

};

class ScaleArray: public ServerFunction {
public:
    ScaleArray()
    {
        setName("scale_array");
        setDescriptionString("Scale a DAP2 Array");
        setUsageString("scale_grid(Array data, Array lon, Array lat, Y size, X size, CRS, Interpolation method)");
        setRole("http://services.opendap.org/dap4/server-side-function/scale_array");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_array");
        setFunction(libdap::function_scale_array);
        setVersion("1.0");
    }
    virtual ~ScaleArray()
    {
    }

};

} // namespace libdap

#endif // _scale_util_h
