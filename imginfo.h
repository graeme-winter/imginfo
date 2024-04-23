//     **********************************************************************
//
//      Copyright (c) 2017, 2024 Global Phasing Ltd.
//
//      This Source Code Form is subject to the terms of the Mozilla Public
//      License, v. 2.0. If a copy of the MPL was not distributed with this
//      file, you can obtain one at http://mozilla.org/MPL/2.0/.
//
//      Authors: Clemens Vonrhein, Claus Flensburg, Thomas Womack and Gerard Bricogne
//
//     **********************************************************************

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include "image_headers.h"

#include <string>
#include <map>
#include <vector>
#include <iostream>
using std::string;
using std::map;
using std::vector;
using std::cout;
using std::endl;

char* strycpy(char* out, const char* in, int* nchars);

void      empty_header(image_header* h);

int       get_buffer(const char* path, char* buffer);

int       get_header            (const char* buffer, image_header* h, const char* path, const int imgnum);
int       get_header_eiger      (const char* path, const int imgnum, image_header* h);

format_t  get_format(const char* buffer);

int       is_hdf5_eiger     (const char* buffer);

void      print_header     (image_header* h, int i, int j);

char*     hdf5_read_char           (hid_t fid, const char* item);
int       hdf5_read_int            (hid_t fid, const char* item);
int*      hdf5_read_nint           (hid_t fid, const char* item, int* n);
double    hdf5_read_double         (hid_t fid, const char* item, const char* unit);
double*   hdf5_read_ndouble        (hid_t fid, const char* item, const char* unit, int* n);
double*   hdf5_read_axis_vector    (hid_t fid, const char* item);
char*     hdf5_read_group_attribute(hid_t fid, const char* item, const char* attribute);
int       hdf5_read_dataset_size   (hid_t fid, const char* item);
int       hdf5_list_filters        (hid_t plist);
