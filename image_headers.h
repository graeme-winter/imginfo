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

#ifndef IMGINFO_H
#define IMGINFO_H

#include <string>
using std::string;

#define MAXHEADERITEMS 1024

/* define some data types */
#define UINT16 unsigned short
#define  INT16 short
#define UINT32 unsigned int
#define  INT32 int
#define  FLT32 float
#define  FLT64 double

/* some constants */
#define CUKA 1.5418
#define NaN   (FLT32)sqrt(-1)
#define NaN32 (FLT32)sqrt(-1)
#define NaN64 (FLT64)sqrt(-1)

/* format definitions */

typedef enum {FORMAT_UNKNOWN, FORMAT_HDF5_EIGER} format_t;

/* for marccd_header */
#define MAXIMAGES               9

typedef struct image_header_s  image_header;

/* ================================================================================= */
/* generic header - used internally in imginfo.c */
struct image_header_s {
  format_t format; /* format desciptor                  */
  string detn; /* precise detector name                 */
  string date; /* date                                  */
  FLT64 dist;  /* crystal-detector distance        [mm] */
  FLT64 wave;  /* wavelength                        [A] */
  FLT64 osca;  /* osciallation angle           [degree] */
  FLT64 phis;  /* starting angle of phi axis   [degree] */
  FLT64 phie;  /* stop angle of phi axis       [degree] */
  FLT64 omes;  /* starting angle of omega axis [degree] */
  FLT64 omee;  /* stop angle of omega axis     [degree] */
  FLT64 chis;  /* starting angle of chi axis   [degree] */
  FLT64 chie;  /* stop angle of chi axis       [degree] */
  FLT64 kaps;  /* starting angle of kappa axis [degree] */
  FLT64 kape;  /* stop angle of kappa axis     [degree] */
  FLT64 twot;  /* 2-theta angle                [degree] */
  FLT64 pixx;  /* pixel size in X                  [mm] */
  FLT64 pixy;  /* pixel size in Y                  [mm] */
  INT32 numx;  /* number of pixels along X              */
  INT32 numy;  /* number of pixels along Y              */
  FLT64 beax;  /* beam centre in X              [pixel] */
  FLT64 beay;  /* beam centre in Y              [pixel] */
  INT32 ovld;  /* overload value                        */
  time_t epoch; /* time since UNIX epoch  [opaque type] */
  FLT64 etime; /* exposure time               [seconds] */
  FLT64 flux;  /* flux                 [undefined unit] */
  FLT64 thick; /* sensor thickness                 [mm] */
  string sensm;/* sensor material                       */
  FLT64 fpol;  /* fraction of polarization              */
  INT32 msec;  /* fraction of date & timestamp     [ms] */
};

#endif
