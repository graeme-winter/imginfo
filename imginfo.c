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
#include <time.h>
#include <locale.h>
#include <libgen.h> //basename and dirname

#if defined(__APPLE__) && defined(__MACH__)
#include <libgen.h>
#endif

#if defined(USE_HDF5)
#include "hdf5.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifdef USE_BITSHUFFLE
#include "bshuf_h5filter.h"
#endif
#ifdef USE_LZ4
extern const H5Z_class2_t H5Z_LZ4[1];
#define LZ4_FILTER 32004
#endif
#ifdef __cplusplus
}
#endif
#endif

// compile with -lz -lbz2 //
#include <zlib.h>
#include <bzlib.h>

#include "imginfo.h"

#define CHUNK 16384

#ifdef NaN
#define INIT_FLOAT  NaN
#ifdef NaN64
#define INIT_DOUBLE NaN64
#else
#define INIT_DOUBLE NaN
#endif
#else
#define INIT_FLOAT  0.0
#define INIT_DOUBLE 0.0
#endif
#define INIT_INT    INT_MIN

#define CHAR_ARRAY_LEN 128

// https://en.wikipedia.org/wiki/Machine_epsilon#Values_for_standard_hardware_floating_point_arithmetics
#define EPSILON32 1.19e-07
#define EPSILON64 2.22e-16

int iverb = 0;
int buffer_size = 0;
int get_header_iverb = 1;
int h5check = 0;

vector<string> tokenise(const char* line)
{
  // split into space-separated strings
  vector<string> V;
  int lptr = 0;
  int sptr = 0;
  // skip initial spaces
  while (line[lptr] == 32) lptr++;
  while (2 != 3)
    {
      sptr = lptr;
      while (line[lptr]!=32 && line[lptr]!=0 && line[lptr]!=10 && line[lptr]!=13) lptr++;
      V.push_back(string(line+sptr, lptr-sptr));
      if (line[lptr] == 0 ||line[lptr]==10 || line[lptr]==13) {
        // mark end of array with an empty item
        V.push_back("");
        return V;
      }
      while (line[lptr]==32) lptr++;
    }
}

vector<string> tokenise_cbf_header(const char* line)
{
  // split into space-separated strings
  // according to manual version 1.2 the following are equivalent to space: '#=:,()'
  // ascii decimal table from: http://www.asciitable.com/
  //  0 == NUL (null)
  // 10 == LF  (line feed, new line)
  // 13 == CR  (carriage return)
  // 32 == space
  // 35 == '#'
  // 40 == '('
  // 41 == ')'
  // 44 == ','
  // 58 == ':'
  // 61 == '='
  vector<string> V;
  int lptr = 0;
  int sptr = 0;
  // skip initial spaces
  while (line[lptr] == 32) lptr++;
  while (2 != 3)
    {
      sptr = lptr;
      while (line[lptr]!=32 && line[lptr]!=0  && line[lptr]!=10 && line[lptr]!=13 &&
             line[lptr]!=35 && line[lptr]!=40 && line[lptr]!=41 && line[lptr]!=44 && 
             line[lptr]!=58 && line[lptr]!=61) lptr++;
      V.push_back(string(line+sptr, lptr-sptr));
      if (line[lptr] == 0 || line[lptr]==10 || line[lptr]==13) {
        // mark end of array with an empty item
        V.push_back("");
        return V;
      }
      while (line[lptr]==32 || line[lptr]==35 || line[lptr]==40 || line[lptr]==41 || line[lptr]==44 || line[lptr]==58 || line[lptr]==61) lptr++;
    }
}

vector<string> tokenise_file_name(const char* line)
{
  // split into space-separated strings
  // ascii decimal table from: http://www.asciitable.com/
  //  0 == NUL (null)
  // 10 == LF  (line feed, new line)
  // 13 == CR  (carriage return)
  // 32 == space
  // 44 == ','
  vector<string> V;
  int lptr = 0;
  int sptr = 0;
  // skip initial spaces
  while (line[lptr] == 32) lptr++;
  while (2 != 3)
    {
      sptr = lptr;
      while (line[lptr]!=32 && line[lptr]!=0  && line[lptr]!=10 && line[lptr]!=13 &&
             line[lptr]!=44 ) lptr++;
      V.push_back(string(line+sptr, lptr-sptr));
      if (line[lptr] == 0 || line[lptr]==10 || line[lptr]==13) {
        // mark end of array with an empty item
        V.push_back("");
        return V;
      }
      while (line[lptr]==32 || line[lptr]==44 ) lptr++;
    }
}

void print_copyright(int full_copyright) {
  // -------------------------------------------------------------------------------
  // print Copyright header
  // -------------------------------------------------------------------------------
  printf("\n");
  printf(" ==========================================================================\n");
  printf("\n");
  printf(" Copyright  2007, 2024 Global Phasing Ltd\n");
  printf("\n");
  printf(" Author    (2007-2024) Clemens Vonrhein, Claus Flensburg, Thomas Womack, Gerard Bricogne\n");
  printf("\n");
#if defined(USE_HDF5)
  printf("\n");
  if (full_copyright==1) {
    printf(" --------------------------------------------------------------------------\n");
    printf("\n");
  }
  printf(" Contains HDF5 library:\n\n");
  if (full_copyright==1) {
    printf("    HDF5 (Hierarchical Data Format 5) Software Library and Utilities\n");
    printf("    Copyright 2006-2016 by The HDF Group.\n");
    printf("    \n");
    printf("    NCSA HDF5 (Hierarchical Data Format 5) Software Library and Utilities\n");
    printf("    Copyright 1998-2006 by the Board of Trustees of the University of Illinois.\n");
    printf("    \n");
    printf("    All rights reserved.\n");
    printf("    \n");
    printf("    Redistribution and use in source and binary forms, with or\n");
    printf("    without modification, are permitted for any purpose (including\n");
    printf("    commercial purposes) provided that the following conditions are\n");
    printf("    met:\n");
    printf("    \n");
    printf("     1. Redistributions of source code must retain the above\n");
    printf("        copyright notice, this list of conditions, and the following\n");
    printf("        disclaimer.\n");
    printf("    \n");
    printf("     2. Redistributions in binary form must reproduce the above\n");
    printf("        copyright notice, this list of conditions, and the following\n");
    printf("        disclaimer in the documentation and/or materials provided\n");
    printf("        with the distribution.\n");
    printf("    \n");
    printf("     3. In addition, redistributions of modified forms of the source\n");
    printf("        or binary code must carry prominent notices stating that the\n");
    printf("        original code was changed and the date of the change.\n");
    printf("    \n");
    printf("     4. All publications or advertising materials mentioning features\n");
    printf("        or use of this software are asked, but not required, to\n");
    printf("        acknowledge that it was developed by The HDF Group and by the\n");
    printf("        National Center for Supercomputing Applications at the\n");
    printf("        University of Illinois at Urbana-Champaign and credit the\n");
    printf("        contributors.\n");
    printf("    \n");
    printf("     5. Neither the name of The HDF Group, the name of the\n");
    printf("        University, nor the name of any Contributor may be used to\n");
    printf("        endorse or promote products derived from this software\n");
    printf("        without specific prior written permission from The HDF Group,\n");
    printf("        the University, or the Contributor, respectively.\n");
    printf("    \n");
    printf("    DISCLAIMER: THIS SOFTWARE IS PROVIDED BY THE HDF GROUP AND THE\n");
    printf("    CONTRIBUTORS \"AS IS\" WITH NO WARRANTY OF ANY KIND, EITHER\n");
    printf("    EXPRESSED OR IMPLIED. In no event shall The HDF Group or the\n");
    printf("    Contributors be liable for any damages suffered by the users\n");
    printf("    arising out of the use of this software, even if advised of the\n");
    printf("    possibility of such damage.\n");
  } else {
    printf("   HDF5 (Hierarchical Data Format 5) Software Library and Utilities\n");
    printf("   Copyright 2006-2015 by The HDF Group.\n");
    printf("\n");
    printf("   NCSA HDF5 (Hierarchical Data Format 5) Software Library and Utilities\n");
    printf("   Copyright 1998-2006 by the Board of Trustees of the University of Illinois. \n");
  }
  printf("\n");
  if (full_copyright==1) {
    printf(" --------------------------------------------------------------------------\n");
    printf("\n");
  }
  printf(" Contains also:\n\n");
#ifdef USE_BITSHUFFLE
  printf("   Bitshuffle - Filter for improving compression of typed binary data.\n");
  if (full_copyright==1) {
    printf("    \n");
    printf("    Copyright (c) 2014 Kiyoshi Masui (kiyo@physics.ubc.ca)\n");
    printf("    \n");
    printf("    Permission is hereby granted, free of charge, to any person obtaining a copy\n");
    printf("    of this software and associated documentation files (the \"Software\"), to deal\n");
    printf("    in the Software without restriction, including without limitation the rights\n");
    printf("    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n");
    printf("    copies of the Software, and to permit persons to whom the Software is\n");
    printf("    furnished to do so, subject to the following conditions:\n");
    printf("    \n");
    printf("    The above copyright notice and this permission notice shall be included in\n");
    printf("    all copies or substantial portions of the Software.\n");
    printf("    \n");
    printf("    THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n");
    printf("    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n");
    printf("    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n");
    printf("    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n");
    printf("    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
    printf("    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n");
    printf("    THE SOFTWARE.\n");
  } else {
    printf("   Copyright (c) 2014 Kiyoshi Masui (kiyo@physics.ubc.ca)\n");
}
  printf("\n");
  if (full_copyright==1) {
    printf(" --------------------------------------------------------------------------\n");
    printf("\n");
  }
  printf("   LZ4 - Fast LZ compression algorithm.\n");
  if (full_copyright==1) {
    printf("    Copyright (C) 2011-2012, Yann Collet.\n");
    printf("    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)\n");
    printf("    \n");
    printf("    Redistribution and use in source and binary forms, with or without\n");
    printf("    modification, are permitted provided that the following conditions are\n");
    printf("    met:\n");
    printf("    \n");
    printf("        * Redistributions of source code must retain the above copyright\n");
    printf("    notice, this list of conditions and the following disclaimer.\n");
    printf("        * Redistributions in binary form must reproduce the above\n");
    printf("    copyright notice, this list of conditions and the following disclaimer\n");
    printf("    in the documentation and/or other materials provided with the\n");
    printf("    distribution.\n");
    printf("    \n");
    printf("    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    printf("    \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    printf("    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n");
    printf("    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n");
    printf("    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n");
    printf("    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    printf("    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
    printf("    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    printf("    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
  } else {
    printf("   Copyright (C) 2011-2012, Yann Collet.\n");
  }
  printf("\n");
#ifdef USE_LZ4
  if (full_copyright==1) {
    printf(" --------------------------------------------------------------------------\n");
    printf("\n");
  }
  printf("   LZ4/HDF5 FILTER IMPLEMENTATION.\n");
  if (full_copyright==1) {
    printf("    Copyright (C) 2011-2013, Dectris Ltd.\n");
    printf("    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)\n");
    printf("    \n");
    printf("    Redistribution and use in source and binary forms, with or without\n");
    printf("    modification, are permitted provided that the following conditions are\n");
    printf("    met:\n");
    printf("    \n");
    printf("        * Redistributions of source code must retain the above copyright\n");
    printf("    notice, this list of conditions and the following disclaimer.\n");
    printf("        * Redistributions in binary form must reproduce the above\n");
    printf("    copyright notice, this list of conditions and the following disclaimer\n");
    printf("    in the documentation and/or other materials provided with the\n");
    printf("    distribution.\n");
    printf("    \n");
    printf("    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    printf("    \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    printf("    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n");
    printf("    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n");
    printf("    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n");
    printf("    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    printf("    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
    printf("    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    printf("    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
  } else {
    printf("   Copyright (C) 2011-2013, Dectris Ltd.\n");
  }
  printf("\n");
#endif
#else
#ifdef USE_LZ4
  if (full_copyright==1) {
    printf(" --------------------------------------------------------------------------\n");
    printf("\n");
  }
  printf("   LZ4 - Fast LZ compression algorithm.\n");
  if (full_copyright==1) {
    printf("    Copyright (C) 2011-2012, Yann Collet.\n");
    printf("    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)\n");
    printf("    \n");
    printf("    Redistribution and use in source and binary forms, with or without\n");
    printf("    modification, are permitted provided that the following conditions are\n");
    printf("    met:\n");
    printf("    \n");
    printf("        * Redistributions of source code must retain the above copyright\n");
    printf("    notice, this list of conditions and the following disclaimer.\n");
    printf("        * Redistributions in binary form must reproduce the above\n");
    printf("    copyright notice, this list of conditions and the following disclaimer\n");
    printf("    in the documentation and/or other materials provided with the\n");
    printf("    distribution.\n");
    printf("    \n");
    printf("    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    printf("    \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    printf("    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n");
    printf("    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n");
    printf("    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n");
    printf("    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    printf("    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
    printf("    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    printf("    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
  } else {
    printf("   Copyright (C) 2011-2012, Yann Collet.\n");
  }
  printf("\n");
  if (full_copyright==1) {
    printf(" --------------------------------------------------------------------------\n");
    printf("\n");
  }
  printf("   LZ4/HDF5 FILTER IMPLEMENTATION.\n");
  if (full_copyright==1) {
    printf("    Copyright (C) 2011-2013, Dectris Ltd.\n");
    printf("    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)\n");
    printf("    \n");
    printf("    Redistribution and use in source and binary forms, with or without\n");
    printf("    modification, are permitted provided that the following conditions are\n");
    printf("    met:\n");
    printf("    \n");
    printf("        * Redistributions of source code must retain the above copyright\n");
    printf("    notice, this list of conditions and the following disclaimer.\n");
    printf("        * Redistributions in binary form must reproduce the above\n");
    printf("    copyright notice, this list of conditions and the following disclaimer\n");
    printf("    in the documentation and/or other materials provided with the\n");
    printf("    distribution.\n");
    printf("    \n");
    printf("    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    printf("    \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    printf("    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n");
    printf("    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n");
    printf("    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n");
    printf("    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    printf("    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
    printf("    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    printf("    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
  } else {
    printf("   Copyright (C) 2011-2013, Dectris Ltd.\n");
  }
  printf("\n");
#endif
#endif
#ifdef USE_BZIP2
  if (full_copyright==1) {
    printf(" --------------------------------------------------------------------------\n");
    printf("\n");
  }
  printf("   HDF5 filter plugin adopted from PyTables.\n");
  if (full_copyright==1) {
    printf("    Copyright Notice and Statement for PyTables Software Library and Utilities:\n");
    printf("    \n");
    printf("    Copyright (c) 2002-2004 by Francesc Alted\n");
    printf("    Copyright (c) 2005-2007 by Carabos Coop. V.\n");
    printf("    Copyright (c) 2008-2010 by Francesc Alted\n");
    printf("    All rights reserved.\n");
    printf("    \n");
    printf("    Redistribution and use in source and binary forms, with or without\n");
    printf("    modification, are permitted provided that the following conditions are\n");
    printf("    met:\n");
    printf("    \n");
    printf("    a. Redistributions of source code must retain the above copyright\n");
    printf("       notice, this list of conditions and the following disclaimer.\n");
    printf("    \n");
    printf("    b. Redistributions in binary form must reproduce the above copyright\n");
    printf("       notice, this list of conditions and the following disclaimer in the\n");
    printf("       documentation and/or other materials provided with the\n");
    printf("       distribution.\n");
    printf("    \n");
    printf("    c. Neither the name of Francesc Alted nor the names of its\n");
    printf("       contributors may be used to endorse or promote products derived\n");
    printf("       from this software without specific prior written permission.\n");
    printf("    \n");
    printf("    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    printf("    \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
    printf("    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n");
    printf("    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n");
    printf("    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n");
    printf("    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n");
    printf("    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
    printf("    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
    printf("    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
    printf("    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
  } else {
    printf("   Copyright (c) 2002-2004 by Francesc Alted\n");
    printf("   Copyright (c) 2005-2007 by Carabos Coop. V.\n");
    printf("   Copyright (c) 2008-2010 by Francesc Alted\n");
  }
  printf("\n");
#endif
  if (full_copyright==0) {
    printf(" For full information, please run with \"-fullinfo\" flag.\n");
    printf("\n");
  }
#endif
  printf(" ==========================================================================\n");
  printf("\n");
}

void print_help() {
  printf("\n");
  printf(" USAGE: imginfo [-v|-h] [-detid] [-[no]norm] [-h5check] <file-1> [... <file-N>]\n");
  printf("\n");
  printf("        -v                      : increase verbosity\n");
  printf("\n");
  printf("        -h                      : this help message\n");
  printf("\n");
  printf("        -detid                  : extract and print detector ID (serial number) if available\n");
  printf("\n");
  printf("        -[no]norm               : normalise (or not) angular ranges into 0..360 range (default = take values as-is)\n");
  printf("\n");
  printf("        -h5check                : some additional checks on HDF5 files\n");
  printf("\n");
  printf("        <file-N>                : HDF5 (master) file\n");
  printf("\n");
}

int main(int argc, char *argv[])
{
  image_header h;
  int header_success;

  int idet=0, inorm=0;
  int nfil = 0;

  char *path;
  char buffer[CHUNK+1];
  int buflen = 0;
  int imgnum = 0;
  vector<string> fields;
  char *locale;

  int full_copyright = 0;
  // should we write copyright note ...
  int do_copyright=1;

  // ensure locale setting:
  locale = setlocale(LC_ALL, "C");

  memset(buffer, 0, CHUNK);

  argc--;*argv++;
  while(argc--) {
    if (strcmp(*argv,"-v")==0) {
      iverb++;
      if (iverb>1) printf(" verbosity level set to %d\n",iverb);
      *argv++;
    }
    else if (strcmp(*argv,"-fullinfo")==0) {
      full_copyright=1;
      *argv++;
    }
    else if ( strcmp(*argv,"-h")==0 || strcmp(*argv,"-help")==0 || strcmp(*argv,"--help")==0 ) {
      print_help();
      exit(EXIT_SUCCESS);
    }
    else if (strcmp(*argv,"-detid")==0) {
      idet++;
      if (iverb>1) printf(" Detector-ID output option set to %d\n",idet);
      *argv++;
    }
    else if (strcmp(*argv,"-nonorm")==0) {
      inorm=0;
      if (iverb>1) printf(" Angle normalisaiton option set to %d\n",inorm);
      *argv++;
    }
    else if (strcmp(*argv,"-norm")==0) {
      inorm=1;
      if (iverb>1) printf(" Angle normalisaiton option set to %d\n",inorm);
      *argv++;
    }
    else if (strcmp(*argv,"-h5check")==0) {
      h5check++;
      if (iverb>1) printf(" Will perform additional checks on HDF5 files\n");
      *argv++;
    }
    else {

      if (do_copyright==1) {
        print_copyright(full_copyright);
        do_copyright=0;
      }

      path = *argv++;

      // allow a path specification *,* to set image numbers in case of e.g. HDF5 master files
      if (strstr(path, ",") != NULL) {
	fields = tokenise_file_name(path);
	imgnum = atoi(fields[1].c_str());
	path = (char *) fields[0].c_str();
      }

      printf("\n\n ################# File = %s\n\n",path);
      buflen = get_buffer(path, buffer);
      if (iverb>1) printf(" [debug] get_buffer send back buflen=%i\n", buflen);
      if ( buflen > 0 ) {
        // protect overflow when scanning buffer using strycpy
        buffer[buflen - 1] = EOF;
        buffer_size = buflen - 1;
        if (iverb>1) printf(" [debug] buffer_size=%i\n", buffer_size);
        if (iverb>2) printf(" [debug] calling get_header\n");
        header_success = get_header(buffer, &h, path, imgnum);
        if (iverb>2) printf(" [debug] header_success=%d\n", header_success);
        if (header_success == 0) {
          printf("\n\nError reading file header\n");
          exit(EXIT_FAILURE);
        }
        print_header(&h,idet,inorm);
        nfil++;
      }
    }

  }
  if (nfil>0) {
    exit(EXIT_SUCCESS);
  } else {
    if (do_copyright==1) {
      print_copyright(full_copyright);
      do_copyright=0;
    }
    printf("\nError - no (or unrecognized) files given?\n\n");
    exit(EXIT_FAILURE);
  }

}

int get_buffer(const char* path, char* buffer){

  int ret;
  FILE *file;
  gzFile gzfile;
  BZFILE *bzfile;
  int bzerror;
  int empty;

  ret = -1;
  empty = 0;

  // first try BZ2 //
  file = fopen(path, "rb");
  if (!file)
      return ret;
  bzfile = BZ2_bzReadOpen(&bzerror, file, 0, 0, NULL, 0);
  if ( bzerror == BZ_OK ) {
      ret = BZ2_bzRead ( &bzerror, bzfile, buffer, CHUNK);
      if (bzerror == BZ_STREAM_END)
          empty = 1;
      //printf("BZERROR=%d %d\n", bzerror, empty);
  }
  BZ2_bzReadClose ( &bzerror, bzfile );
  if (file)
      fclose(file);
  if (ret > 0){
      return ret;
  }
  // Detected an empty bz2 file above //
  if (empty == 1){
      ret = 0;
      return ret;
  }

  // then try gz (will work equally fine on a regular file //
  gzfile = gzopen(path, "rb");
  if (gzfile != NULL) {
      ret = gzread(gzfile, buffer, CHUNK);
      gzclose(gzfile);
      if (ret >= 0) {
          return ret;
      }
  }

  // something went wrong //
  ret = -1;
  return ret;
}

// ==================================================================================================
// get_format
// ==================================================================================================
format_t get_format(const char* buffer) {
  format_t formatn[]  = {
    FORMAT_HDF5_EIGER
  };
  const char* formats[] = {
    "hdf5_eiger"
  };
  int (*formatf[])(const char*) = {
    is_hdf5_eiger
  };

  int nformats = sizeof(formatn) / sizeof(int);
  for (int u=0; u<nformats; u++)
    {
      if (iverb>2) printf(" [debug]   (%d) calling is_%s\n", u, formats[u]);
      if (formatf[u](buffer)) {
        if (iverb>2) printf(" [debug]   YES %d\n",formatn[u]);
        return formatn[u];
      } else {
        if (iverb>2) printf(" [debug]   NO\n");
      }
    }
  return FORMAT_UNKNOWN;
}

// ==================================================================================================
// get_header
// ==================================================================================================
int get_header(const char* buffer, image_header* h, const char* path, const int imgnum)
{
  empty_header(h);
  if (iverb>2) printf(" [debug]  calling get_format\n");
  format_t format = get_format(buffer);
  if (iverb>2) printf(" [debug]  format = %d\n",format);

  if (format == FORMAT_UNKNOWN) {
    printf("ERROR: cannot determine file format\n");
    return 0;
  }

  format_t formats2[] = {FORMAT_HDF5_EIGER};
  string   formath2[] = {"HDF5/Eiger"};
  int(*formatf2[])(const char*, const int, image_header*) = {get_header_eiger};

  for (UINT32 u=0; u < sizeof(formats2)/sizeof(format_t); u++){
    if (format == formats2[u]) {
      printf(" >>> Image format detected as %s\n",formath2[u].c_str());
      return formatf2[u](path, imgnum, h);
    }
  }

  printf(" ERROR: unknown format %d\n",format);
  return 0;
}

// ==================================================================================================
// utility routines
// ==================================================================================================

char* strycpy(char* out, const char* in, int* nchars)
{
  int u;
  int v;
  int mchars = *nchars;
  int eof = -1;
  v=0;
  for (u=0; u<mchars-1; u++){
      v++;
      out[u]=in[u];
      if (out[u] == '\0') break;
      if (out[u] == 10) break;
      if (out[u] == EOF) {
          eof = u;
          break;
      }
  }
  *nchars = v;
  // As per default of fgets:
  if (eof == 0) return NULL;
  if (v < mchars) out[v] = 0;
  return out;
}

float my_stof(const string& S)
{
  float f;
  sscanf(S.c_str(),"%f",&f);
  return f;
}

double my_stod(const string& S)
{
  double f;
  sscanf(S.c_str(),"%lf",&f);
  return f;
}

int my_stoi(const string& S)
{
  int f;
  sscanf(S.c_str(),"%d",&f);
  return f;
}

// http://stackoverflow.com/questions/352055/best-algorithm-to-strip-leading-and-trailing-spaces-in-c
char *strstrip(char *s)
{
    size_t size;
    char *end;

    size = strlen(s);

    if (!size)
            return s;

    end = s + size - 1;
    while (end >= s && isspace(*end))
            end--;
    *(end + 1) = '\0';

    while (*s && isspace(*s))
            s++;

    return s;
}

// https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
bool AreSame(double a, double b, double diff)
{
  if (iverb>2) {
    printf("\n");
    printf(" AreSame : a    = %20.12e\n",a);
    printf("           b    = %20.12e\n",b);
    printf("           a-b  = %20.12e\n",fabs(a - b));
    printf("           diff = %20.12e\n",diff);
  }
  return fabs(a - b) <= diff;
}

bool stotime_t(time_t& result, const string& S, const string& format)
{
  struct tm tm;
  char* oldloc = setlocale(LC_TIME,"C");
  if (strptime(S.c_str(), format.c_str(), &tm))
    {
      result = timegm(&tm);
      setlocale(LC_TIME,oldloc);
      return true;
    }
  else
    {
      setlocale(LC_TIME,oldloc);
      return false;
    }
}

string time_ttos(const time_t& T, const string& format = "%d %b %Y %H:%M:%S")
{
  char Tbuf[200];
  char* oldloc = setlocale(LC_TIME,"C");
  struct tm* W;
  W = gmtime(&T);
  W->tm_isdst = -1;
  strftime(Tbuf, sizeof(Tbuf)-3, format.c_str(), W);
  setlocale(LC_TIME,oldloc);
  return string(Tbuf);
}

int is_hdf5_eiger(const char* buffer)
{
  char sig[8];
  int nchar = 8;
  strycpy(sig, buffer, &nchar);
  if ((sig[0] & 0xff)==0x89 &&
      (sig[1] & 0xff)==0x48 &&
      (sig[2] & 0xff)==0x44 &&
      (sig[3] & 0xff)==0x46 &&
      (sig[4] & 0xff)==0x0d &&
      (sig[5] & 0xff)==0x0a ) {
    return FORMAT_HDF5_EIGER;
  } else {
    return 0;
  }
}

// ==================================================================================================
// get_header_XYZ
// ==================================================================================================

int get_header_eiger(const char* path, const int imgnum, image_header* h)
{

  vector<string> fields;

  hid_t fid;
  herr_t status;

  // get directory part of input file (required for later reading data
  // from external files)
  char * t = (char *)malloc(strlen(path) + 1);
  strcpy(t,path);
  char *dir;
  char epoch_date[20];
  dir = dirname(t);

  // for units see: http://download.nexusformat.org/doc/html/nxdl-types.html

  // Open the file
  if (iverb>1) printf("\n Opening file %s\n\n",path);
  fid = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (fid<0) {
    printf("\n\n ERROR - unable to open file \"%s\"!\n\n",path);
    exit(EXIT_FAILURE);
  }

#ifdef USE_BITSHUFFLE
  if (bshuf_register_h5filter()<0) {
    printf("\n\n ERROR - unable to register Bitshuffle filter!\n\n");
    exit(EXIT_FAILURE);
  }
#endif
#ifdef USE_LZ4
  if (H5Zregister(H5Z_LZ4)<0) {
    printf("\n\n ERROR - unable to register H5Z_LZ4!\n\n");
    exit(EXIT_FAILURE);
  }
#endif

  if (iverb>2)  {
    hid_t cpl = H5Fget_create_plist(fid);
    hdf5_list_filters(cpl);
  }

  if (iverb>2)              printf(" ... will read /entry/instrument/detector/description\n");
  char *description          = hdf5_read_char(fid,"/entry/instrument/detector/description");
  if (iverb>2)              printf(" ... will read /entry/instrument/detector/detector_number\n");
  char *detector_number      = hdf5_read_char(fid,"/entry/instrument/detector/detector_number");
  if (detector_number != NULL) {
    h->detn = detector_number;
  }
  else {
    if (iverb>2)         printf(" ... will read /entry/instrument/detector/serial_number\n");
    detector_number      = hdf5_read_char(fid,"/entry/instrument/detector/serial_number");
    if (detector_number != NULL) {
      h->detn = detector_number;
    }
  }

  if (iverb>2)              printf(" ... will read /entry/instrument/detector/sensor_material\n");
  char *sensor_material      = hdf5_read_char(fid,"/entry/instrument/detector/sensor_material");
  if (sensor_material!=NULL) {
    h->sensm = sensor_material;
    if (h->sensm != "Si" && h->sensm != "CdTe" && h->sensm != "Silicon" ) {
      printf("\n WARNING: item \"/entry/instrument/detector/sensor_material\" gives material as \"%s\" when \"Silicon\" (or \"Si\" or \"CdTe\") seems more common!\n\n",sensor_material);
    }
  }
  if (iverb>2)              printf(" ... will read /entry/instrument/detector/detectorSpecific/data_collection_date\n");
  char *data_collection_date = hdf5_read_char(fid,"/entry/instrument/detector/detectorSpecific/data_collection_date");
  if (iverb>2)              printf(" ... will read /entry/instrument/detector/detectorSpecific/eiger_fw_version\n");
  char *eiger_fw_version     = hdf5_read_char(fid,"/entry/instrument/detector/detectorSpecific/eiger_fw_version");

  if ( (data_collection_date == NULL) || (data_collection_date[0] == '\0') ) {

    // check if we have /entry/start_time
    if (iverb>2)        printf(" ... will read /entry/start_time\n");
    data_collection_date = hdf5_read_char(fid,"/entry/start_time");

    if ( (data_collection_date == NULL) || (data_collection_date[0] == '\0') ) {
      // Diamond files (image_9264_master.h5, 20181105 12:56) don't have that field
      data_collection_date = strcpy(epoch_date,"1970-01-01T00:00:01");
      if (iverb>2) printf("     Setting date to \"%s\"\n",data_collection_date);
    }

  }
  fields = tokenise_cbf_header(data_collection_date);
  if (fields[0].length()==13) {
    if ( (fields[0].substr(4,1) == "-") && (fields[0].substr(7,1) == "-") && (fields[0].substr(10,1) == "T") ) {
      // 2015-11-10T17:17:02.057611
      if (stotime_t(h->epoch,data_collection_date,"%Y-%m-%dT%H:%M:%S")) {
	if (fields[2].substr(2,1) == ".") {
	  h->msec = rintf((my_stoi(fields[2].substr(3))/1000.0));
	}
	h->date = time_ttos(h->epoch);
      }
    }
  }

  if (iverb>2)  printf(" ... will read /entry/instrument/detector/detectorSpecific/nimages\n");
  int nimages     = hdf5_read_int(fid,"/entry/instrument/detector/detectorSpecific/nimages");

  if (nimages==INIT_INT||nimages<=0) {
    // Diamond files (image_9264_master.h5, 20181105 12:56) don't have that field
    char *axes = hdf5_read_group_attribute(fid,"/entry/data","axes");
    if (iverb>2) printf("     axes = \"%s\"\n",axes);
    if ( axes != NULL && strcmp(axes,"omega")==0 ) {
      if (iverb>2) printf("     will determine nimages from /entry/data/omega\n");
      nimages     = hdf5_read_dataset_size(fid,"/entry/data/omega");
      if (iverb>2) printf("     nimages = \"%d\"\n",nimages);
    }
  } else {
    if (nimages==INIT_INT) {
      status = H5Fclose(fid);
      printf("\n ERROR: no item \"/entry/instrument/detector/detectorSpecific/nimages\" found!\n\n");
      return 0;
    }
    if (nimages==0) {
      status = H5Fclose(fid);
      printf("\n ERROR: no images found (\"/entry/instrument/detector/detectorSpecific/nimages\" is 0)!\n\n");
      return 0;
    }
  }

  int nimages_per_trigger = nimages;
  if (iverb>2) printf(" ... will read /entry/instrument/detector/detectorSpecific/ntrigger\n");
  int ntrigger   = hdf5_read_int(fid,"/entry/instrument/detector/detectorSpecific/ntrigger");
  if (ntrigger==INIT_INT||ntrigger<0) {
    // Diamond files (image_9264_master.h5, 20181105 12:56) don't have that field
    ntrigger = 1;
  } else {
    if (ntrigger!=INIT_INT) {
      if (ntrigger==0) {
	printf("\n WARNING: item \"/entry/instrument/detector/detectorSpecific/ntrigger\" is set to 0?\n");
	ntrigger = 1;
      }
      else if (ntrigger<0) {
	printf("\n WARNING: item \"/entry/instrument/detector/detectorSpecific/ntrigger\" is set to negative value %d?\n",ntrigger);
	ntrigger = 1;
      }
      else {
	nimages = nimages_per_trigger * ntrigger;
	if (ntrigger>1) {
	  printf("\n Note: assuming a total of %d images (based on %d images for each of the %d triggers)\n",nimages,nimages_per_trigger,ntrigger);
	}
      }
    } else {
      ntrigger = 1;
    }
  }

  int ntrigger_use = ntrigger;

  int img = 0;
  int img2 = nimages_per_trigger - 1;
  if (imgnum>0) {
    img = imgnum - 1;
    ntrigger_use = 1;
    img2 = img;
  }

  double *omega_trigger_start = (double *)calloc(ntrigger*2,sizeof(double));
  if (omega_trigger_start == NULL) {
    printf("\n ERROR: unable to allocate omega_start(ntrigger*2) array!\n\n");
    return 0;
  }
  double *omega_trigger_end = (double *)calloc(ntrigger*2,sizeof(double));
  if (omega_trigger_end == NULL) {
    printf("\n ERROR: unable to allocate omega_end(ntrigger*2) array!\n\n");
    return 0;
  }
  double *kappa_trigger_start = (double *)calloc(ntrigger*2,sizeof(double));
  if (kappa_trigger_start == NULL) {
    printf("\n ERROR: unable to allocate kappa_start(ntrigger*2) array!\n\n");
    return 0;
  }
  double *kappa_trigger_end = (double *)calloc(ntrigger*2,sizeof(double));
  if (kappa_trigger_end == NULL) {
    printf("\n ERROR: unable to allocate kappa_end(ntrigger*2) array!\n\n");
    return 0;
  }
  double *phi_trigger_start = (double *)calloc(ntrigger*2,sizeof(double));
  if (phi_trigger_start == NULL) {
    printf("\n ERROR: unable to allocate phi_start(ntrigger*2) array!\n\n");
    return 0;
  }
  double *phi_trigger_end = (double *)calloc(ntrigger*2,sizeof(double));
  if (phi_trigger_end == NULL) {
    printf("\n ERROR: unable to allocate phi_end(ntrigger*2) array!\n\n");
    return 0;
  }
  double *chi_trigger_start = (double *)calloc(ntrigger*2,sizeof(double));
  if (chi_trigger_start == NULL) {
    printf("\n ERROR: unable to allocate chi_start(ntrigger*2) array!\n\n");
    return 0;
  }
  double *chi_trigger_end = (double *)calloc(ntrigger*2,sizeof(double));
  if (chi_trigger_end == NULL) {
    printf("\n ERROR: unable to allocate chi_end(ntrigger*2) array!\n\n");
    return 0;
  }
  double *two_theta_trigger_start = (double *)calloc(ntrigger*2,sizeof(double));
  if (two_theta_trigger_start == NULL) {
    printf("\n ERROR: unable to allocate two_theta_start(ntrigger*2) array!\n\n");
    return 0;
  }
  double *two_theta_trigger_end = (double *)calloc(ntrigger*2,sizeof(double));
  if (two_theta_trigger_end == NULL) {
    printf("\n ERROR: unable to allocate two_theta_end(ntrigger*2) array!\n\n");
    return 0;
  }

  int *nimage_to_imgnum = (int *)calloc(nimages,sizeof(int));
  if (nimage_to_imgnum == NULL) {
    printf("\n ERROR: unable to allocate nimage_to_imgnum(nimages) array!\n\n");
    return 0;
  }
  for (int inimage_to_imgnum = 1; inimage_to_imgnum <= nimages; inimage_to_imgnum++ ) {
    nimage_to_imgnum[inimage_to_imgnum-1]=inimage_to_imgnum;
  }

  int img_offset = 0;
  if (img>nimages) {
    // enforce HDF5 checking (for obvious reasons). Remember that we
    // could still have an undetected offset - which is why autoPROC
    // enforces h5check throughout.
    h5check++;
  }

  if (h5check>0) {
    // -------------------------------------------------------------------------
    // need to check for accessibility of all nimages images
    H5G_info_t group_info;
    hid_t gid = H5Gopen2(fid,"/entry/data",H5P_DEFAULT);
    if (gid < 0) {
      printf("\n\n ERROR - in H5Gopen2 (group=\"/entry/data\")!\n\n");
      return(-1);
    }
    if (H5Gget_info(gid,&group_info)<0) {
      printf("\n\n ERROR - in H5Gget_info (group=\"/entry/data\")!\n\n");
      return(-1);
    }
    if (iverb>2) {
      printf("   nlinks in group /entry/data = %d\n", (int)group_info.nlinks);
    }

    // need to check each link to see if it is an external link
    hsize_t n_external_links = 0;
    for (hsize_t ilink=0; ilink<group_info.nlinks; ilink++) {

      ssize_t link_size = 1 + H5Lget_name_by_idx (gid, ".", H5_INDEX_NAME, H5_ITER_INC,
						  ilink, NULL, 0, H5P_DEFAULT);

      char *link_name = (char *) malloc (link_size);

      link_size = H5Lget_name_by_idx (gid, ".", H5_INDEX_NAME, H5_ITER_INC, ilink, link_name,
				      (size_t) link_size, H5P_DEFAULT);

      H5L_info_t link_info;
      status = H5Lget_info_by_idx(gid, ".", H5_INDEX_NAME, H5_ITER_INC, ilink, &link_info, H5P_DEFAULT);
      if (status<0) {
	printf("\n\n ERROR - in H5Lget_info_by_idx (group=\"/entry/data\")!\n\n");
	return(-1);
      }

      if (link_info.type==H5L_TYPE_EXTERNAL) {
	n_external_links++;
      }

      if (iverb>2) {
	if (link_info.type==H5L_TYPE_HARD) {
	  printf ("     link #%d (hard) = \"%s\"\n", (int) ilink, link_name);
	}
	else if (link_info.type==H5L_TYPE_SOFT) {
	  printf ("     link #%d (soft) = \"%s\"\n", (int) ilink, link_name);
	}
	else if (link_info.type==H5L_TYPE_EXTERNAL) {
	  printf ("     link #%d (external) = \"%s\"\n", (int) ilink, link_name);
	}
	else if (link_info.type==H5L_TYPE_ERROR) {
	  printf ("     link #%d (error) = \"%s\"\n", (int) ilink, link_name);
	}
	else {
	  printf ("     link #%d gave error\n", (int) ilink);
	}
      }

      free (link_name);
    }

    if (iverb>2) {
      printf("\n");
    }

    if (H5Gclose(gid)<0) {
      printf("\n\n ERROR - in H5Gclose!\n\n");
      return(-1);
    }

    char link[27];

    int nimages_found = 0;
    int have_image_nr_high = 1;
    for (hsize_t ilink = 1; ilink <= n_external_links; ilink++ ) {
      sprintf(link,"/entry/data/data_%6.6d",ilink);
      H5L_info_t link_info;
      if (H5Lget_info(fid, link, &link_info, H5P_DEFAULT)<0) {
	printf("\n\n ERROR - in H5Lget_info (link=%s)!\n\n",link);
	return(-1);
      }
      if (link_info.type!=H5L_TYPE_EXTERNAL) {
	fprintf(stderr,"\n\n ERROR: unsupported link type for %s !\n\n",link);
	return(2);
      }

      // get full path of external file
      H5L_info_t li;
      const char *filename;
      const char *path;
      if (H5Lget_info(fid,link,&li,H5P_DEFAULT) < 0) {
	printf("\n\n ERROR - in H5Lget_info!\n\n");
	return(-1);
      }

      size_t val_size = li.u.val_size;
      char *buf[val_size];
      if (H5Lget_val(fid,link,(void *)buf,val_size,H5P_DEFAULT) < 0 ) {
	printf("\n\n ERROR - in H5Lget_val!\n\n");
	return(-1);
      }
      if ( H5Lunpack_elink_val(buf, val_size, NULL, &filename, &path) < 0 ) {
	printf("\n\n ERROR - in H5Lunpack_elink_val!\n\n");
	return(-1);
      }
      if (iverb>2) printf("     [link=%d] dir=%s filename=%s path=%s\n",ilink,dir,filename,path);
      if (iverb>2) printf("     [link=%d] strlen(dir)=%lu strlen(filename)=%lu strlen(path)=%lu\n",ilink,strlen(dir),strlen(filename),strlen(path));

      size_t f_size = strlen(dir) + 1 + strlen(filename) + 1;
      char *f = (char *)malloc(f_size);
      strcpy(f,dir);
      strcat(f,"/");
      strcat(f,filename);

      // start reading from external file
      hid_t eid = H5Fopen(f, H5F_ACC_RDONLY, H5P_DEFAULT);
      if (eid<0) {
	printf("\n\n ERROR - in H5Fopen (%s)!\n\n",f);
	break;
      }

      int image_nr_low = 0;
      int image_nr_high = 0;

      hid_t did = H5Dopen2(eid, path, H5P_DEFAULT);
      if (did<0) {
	printf("\n\n ERROR - in H5Dopen2 (file=\"%s\" path=\"%s\")!\n\n",f,path);
	return(-1);
      }

      if (ilink==1&&iverb>0) {
        hid_t cpl = H5Dget_create_plist(did);
        hdf5_list_filters(cpl);
      }

      status = H5Aexists(did,"image_nr_high");
      if (status<=0) {
	if (iverb>1&&have_image_nr_high==1) {
	  printf("     WARNING: problem finding attribute \"%s\" in dataset \"%s\".\n","image_nr_high",f);
	  printf("              We will assume images are starting at 1 and come in sequential order.\n");
	}
	have_image_nr_high=0;
	hid_t space_id = H5Dget_space (did);
	hsize_t dims_id[3] = {1,1,1};
	int ndims_id = H5Sget_simple_extent_dims (space_id, dims_id, NULL);
	if (iverb>2) {
	  printf("     dims_id[0]=%llu\n",dims_id[0]);
	}

	for (hsize_t inimage_to_imgnum = nimages_found; inimage_to_imgnum < (nimages_found + dims_id[0]); inimage_to_imgnum++ ) {
	  nimage_to_imgnum[inimage_to_imgnum] = inimage_to_imgnum + 1;
	}
	nimages_found = nimages_found + dims_id[0];

      } else {
	hid_t aid = H5Aopen_by_name(did,".","image_nr_high",H5P_DEFAULT,H5P_DEFAULT);
	if (aid>0) {
	  int    data_i[10];
	  if (H5Aread(aid,H5T_NATIVE_INT,data_i) >= 0 ) {
	    image_nr_high = data_i[0];
	    hid_t aid2 = H5Aopen_by_name(did,".","image_nr_low",H5P_DEFAULT,H5P_DEFAULT);
	    if (aid2>0) {
	      if (H5Aread(aid2,H5T_NATIVE_INT,data_i) >= 0 ) {
		image_nr_low = data_i[0];
		if (iverb>2) {
		  printf("     %s %s/image_nr_low  = %d\n",f,path,image_nr_low);
		  printf("     %s %s/image_nr_high = %d\n",f,path,image_nr_high);
		}
		for (int inimage_to_imgnum = nimages_found; inimage_to_imgnum < (nimages_found + (image_nr_high - image_nr_low + 1)); inimage_to_imgnum++ ) {
		  nimage_to_imgnum[inimage_to_imgnum]=image_nr_low + (inimage_to_imgnum-nimages_found);
		}
		nimages_found = nimages_found + (image_nr_high - image_nr_low + 1);
	      }
	      status = H5Aclose(aid2);
	    }
	  }
	  status = H5Aclose(aid);
	}
      }
    }

    if (nimages<nimages_found) {
      printf("\n WARNING: there seem to be more images in the EXTERNAL LINK files (%d) than we\n",nimages_found);
      printf("          expected (%d) - which doesn't make much sense. Please check with beamline\n",nimages);
      printf("          staff and get back to us!\n");
    }
    else if (nimages>nimages_found) {
      printf("\n WARNING: there seem to be fewer images in the EXTERNAL LINK files (%d) than we\n",nimages_found);
      printf("          expected (%d) - which looks like an interupted data collection? We will\n",nimages);
      printf("          assume that this smaller number is the correct one to use (but please check\n");
      printf("          and get back to beamline staff or us)!\n");
      nimages=nimages_found;
    } else {
      if (iverb>1) {
	printf("\n\n Good - expected and found number of images identical (%d)!\n\n",nimages);
      }
    }

  }

  int img1use = img;
  int img2use = img2;

  if (img>nimages) {

    // if we requested an image outside the 1..nimages range:
    if (h5check>0) {
      // is it within our array of images
      for (int iimg = 0; iimg < nimages; iimg++ ) {
	if (nimage_to_imgnum[iimg]==(img+1)) {
	  img1use = iimg;
	  img2use = iimg + (img2 - img);
	  break;
	}
      }
    }
    if (img1use>nimages) {
      printf("\n ERROR: requested image number %d but only %d images present!\n\n",(img+1),nimages);
      return 0;
    }
  }

  double *omega, *omega_end, *omega_axis;
  double omega_range_average, omega_range_total, omega_increment;
  double *kappa, *kappa_end, *kappa_axis;
  double kappa_range_average, kappa_range_total;
  double *chi, *chi_end, *chi_axis;
  double chi_range_average, chi_range_total;
  double *phi, *phi_end, *phi_axis;
  double phi_range_average, phi_range_total, phi_increment;
  double *two_theta, *two_theta_end, *two_theta_axis;
  double two_theta_range_average, two_theta_range_total;

  double *detector_distance_vector, *fast_pixel_vector, *slow_pixel_vector;
  int ihave_omega     = 0;
  int ihave_kappa     = 0;
  int ihave_chi       = 0;
  int ihave_phi       = 0;
  int ihave_two_theta = 0;
  int esgo = 0;
  int itrigger_prev = -1;
  int ndatasets = 0;
  for (int itrigger = 0; itrigger < ntrigger_use; itrigger++) {

    int itrigger2 = itrigger+ntrigger_use;

    if (itrigger==0) {
      if (iverb>2)              printf(" ... check for /entry/sample\n");
      if (H5Lexists(fid,"/entry/sample",H5P_DEFAULT)>0) {
	if (iverb>2)              printf(" ... check for /entry/sample/goniometer\n");
	if (H5Lexists(fid,"/entry/sample/goniometer",H5P_DEFAULT)>0) {
       	  if (iverb>2)              printf(" ... check for /entry/sample/goniometer/omega\n");
       	  if (H5Lexists(fid,"/entry/sample/goniometer/omega",H5P_DEFAULT)>0) {
	    if (iverb>2)              printf(" ... will read /entry/sample/goniometer/omega\n");
	    omega                   = hdf5_read_ndouble(fid,"/entry/sample/goniometer/omega","degree",&nimages);
	    if (iverb>2)              printf(" ... will read /entry/sample/goniometer/omega_end\n");
	    omega_end               = hdf5_read_ndouble(fid,"/entry/sample/goniometer/omega_end","degree",&nimages);
	    if (iverb>2)              printf(" ... will read /entry/sample/goniometer/omega_range_average\n");
	    omega_range_average     = hdf5_read_double (fid,"/entry/sample/goniometer/omega_range_average","degree");
	    if (iverb>2)              printf(" ... will read /entry/sample/goniometer/omega_range_total\n");
	    omega_range_total       = hdf5_read_double (fid,"/entry/sample/goniometer/omega_range_total","degree");
	    if (iverb>2)              printf(" ... will read /entry/sample/goniometer/omega_increment\n");
	    omega_increment         = hdf5_read_double (fid,"/entry/sample/goniometer/omega_increment","degree");
	    if (isnan(omega_increment)&&nimages>1) {
	      if (!isnan(omega[0])&&!isnan(omega[1])) {
		omega_increment = omega[1]-omega[0];
	      }
	    }
	    ihave_omega = 1;
	    esgo = 1;
	  }
	}
      }
      if (ihave_omega==0) {
	if (iverb>2) printf(" ... check axes\n");
	// Diamond files (image_9264_master.h5, 20181105 12:56) don't have that field
	char *axes = hdf5_read_group_attribute(fid,"/entry/data","axes");
	if (iverb>2) printf("     axes = \"%s\"\n",axes);
	
	int check_for_omega = 0;
        char omega_str[CHAR_ARRAY_LEN] = "/entry/data/omega";

	if (axes==NULL) {
	  if (iverb>2) printf("     axes undefined?\n");
	  check_for_omega = 1;
	}
	else if (strcmp(axes,"omega")==0 ) {
	  check_for_omega = 1;
	}
        // Diamond files (Mpro-P1623_2_master.h5, I04-1, 20210619 06:17) named that differently:
	else if (strcmp(axes,"gonomega")==0 ) {
	  check_for_omega = 2;
          strcpy(omega_str,"/entry/data/gonomega");
	}
	if (check_for_omega>=1) {
       	  if (H5Lexists(fid,omega_str,H5P_DEFAULT)>0) {
	    if (iverb>2)              printf(" ... will read %s\n",omega_str);
	    omega                   = hdf5_read_ndouble(fid,omega_str,"deg",&nimages);
	    if (!isnan(omega[0])&&!isnan(omega[1])) {
	      omega_increment = omega[1]-omega[0];
	      if (iverb>2)              printf(" ... setting omega_increment to %f\n",omega_increment);
	      omega_end       = hdf5_read_ndouble(fid,omega_str,"deg",&nimages);
	      for(int i_image = 0; i_image < nimages; i_image++) {
		omega_end[i_image] = omega_end[i_image] + omega_increment;
	      }
	      ihave_omega = 1;
	      esgo = 0;
	    }
	  }
	} else {
	  if (iverb>2)              printf("     axes read/set as \"%s\"\n",axes);
	}
      }
    }
    if (ihave_omega==1) {
      if (!isnan(omega[img1use])) {
	if (itrigger==0) {
	  h->omes = omega[img1use];
	  if (!isnan(omega_end[img1use])) {
	    h->omee = omega_end[img1use];
	  } else {
	    h->omee = h->omes;
	  }
	}
	omega_trigger_start[itrigger] = omega[img1use];
	omega_trigger_end[itrigger]   = omega[img1use];
	if (!isnan(omega_end[img1use])) {
	  omega_trigger_end[itrigger] = omega_end[img1use];
	}
      }
      if (!isnan(omega[img2use])) {
	omega_trigger_start[itrigger2] = omega[img2use];
	omega_trigger_end[itrigger2]   = omega[img2use];
	if (!isnan(omega_end[img2use])) {
	  omega_trigger_end[itrigger2] = omega_end[img2use];
	}
      }
    }

    if (itrigger==0) {
      if (esgo==1) {
	if (H5Lexists(fid,"/entry/sample",H5P_DEFAULT)>0) {
	  if (H5Lexists(fid,"/entry/sample/goniometer",H5P_DEFAULT)>0) {
	    if (H5Lexists(fid,"/entry/sample/goniometer/kappa",H5P_DEFAULT)>0) {
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/kappa\n");
	      kappa                   = hdf5_read_ndouble(fid,"/entry/sample/goniometer/kappa","degree",&nimages);
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/kappa_end\n");
	      kappa_end               = hdf5_read_ndouble(fid,"/entry/sample/goniometer/kappa_end","degree",&nimages);
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/kappa_range_average\n");
	      kappa_range_average     = hdf5_read_double (fid,"/entry/sample/goniometer/kappa_range_average","degree");
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/kappa_range_total\n");
	      kappa_range_total       = hdf5_read_double (fid,"/entry/sample/goniometer/kappa_range_total","degree");
	      ihave_kappa = 1;
	    }
	  }
	}
      }
      
      if (esgo==0 && ihave_kappa==0) {
	if (iverb>2) printf(" ... check /entry/sample/sample_kappa/kappa\n");
	if (H5Lexists(fid,"/entry/sample/sample_kappa",H5P_DEFAULT)>0) {
	  if (H5Lexists(fid,"/entry/sample/sample_kappa/kappa",H5P_DEFAULT)>0) {
	    if (iverb>2) {
	      int nd = hdf5_read_dataset_size(fid,"/entry/sample/sample_kappa/kappa");
	      printf(" ... will read %d item(s) /entry/sample/sample_kappa/kappa\n",nd);
	    }
	    kappa = hdf5_read_ndouble(fid,"/entry/sample/sample_kappa/kappa","degree",&nimages);
	    kappa_end = (double *) malloc((nimages * sizeof (double)));
	    for (int id=0; id<nimages;id++ ) {
	      kappa_end[id]=kappa[id];
	    }
	    kappa_range_average = 0.0;
	    kappa_range_total = 0.0;
	    ihave_kappa=1;
	  }
	}
      }

    }
    if (ihave_kappa==1) {
      if (!isnan(kappa[img1use])) {
	if (itrigger==0) {
	  h->kaps = kappa[img1use];
	  if (!isnan(kappa_end[img1use])) {
	    h->kape = kappa_end[img1use];
	  } else {
	    h->kape = h->kaps;
	  }
	}
	kappa_trigger_start[itrigger] = kappa[img1use];
	kappa_trigger_end[itrigger]   = kappa[img1use];
	if (!isnan(kappa_end[img1use])) {
	  kappa_trigger_end[itrigger] = kappa_end[img1use];
	}

	if (!isnan(kappa[img2use])) {
	  kappa_trigger_start[itrigger2] = kappa[img2use];
	  kappa_trigger_end[itrigger2]   = kappa[img2use];
	  if (!isnan(kappa_end[img2use])) {
	    kappa_trigger_end[itrigger2] = kappa_end[img2use];
	  }
	}
      }
    }

    if (itrigger==0) {
      if (esgo==1) {
	if (H5Lexists(fid,"/entry/sample",H5P_DEFAULT)>0) {
	  if (H5Lexists(fid,"/entry/sample/goniometer",H5P_DEFAULT)>0) {
	    if (H5Lexists(fid,"/entry/sample/goniometer/chi",H5P_DEFAULT)>0) {
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/chi\n");
	      chi                     = hdf5_read_ndouble(fid,"/entry/sample/goniometer/chi","degree",&nimages);
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/chi_end\n");
	      chi_end                 = hdf5_read_ndouble(fid,"/entry/sample/goniometer/chi_end","degree",&nimages);
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/chi_range_average\n");
	      chi_range_average       = hdf5_read_double (fid,"/entry/sample/goniometer/chi_range_average","degree");
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/chi_range_total\n");
	      chi_range_total         = hdf5_read_double (fid,"/entry/sample/goniometer/chi_range_total","degree");
	      ihave_chi = 1;
	    }
	  }
	}
      }

      if (esgo==0 && ihave_chi==0) {
	if (iverb>2) printf(" ... check /entry/sample/sample_chi/chi\n");
	if (H5Lexists(fid,"/entry/sample/sample_chi",H5P_DEFAULT)>0) {
	  if (H5Lexists(fid,"/entry/sample/sample_chi/chi",H5P_DEFAULT)>0) {
	    if (iverb>2) {
	      int nd = hdf5_read_dataset_size(fid,"/entry/sample/sample_chi/chi");
	      printf(" ... will read %d item(s) /entry/sample/sample_chi/chi\n",nd);
	    }
	    chi = hdf5_read_ndouble(fid,"/entry/sample/sample_chi/chi","degree",&nimages);
	    chi_end = (double *) malloc((nimages * sizeof (double)));
	    for (int id=0; id<nimages;id++ ) {
	      chi_end[id]=chi[id];
	    }
	    chi_range_average = 0.0;
	    chi_range_total = 0.0;
	    ihave_chi=1;
	  }
	}
      }
    }

    if (ihave_chi==1) {
      if (!isnan(chi[img1use])) {
	if (itrigger==0) {
	  h->chis = chi[img1use];
	  if (!isnan(chi_end[img1use])) {
	    h->chie = chi_end[img1use];
	  } else {
	    h->chie = h->chis;
	  }
	}
	chi_trigger_start[itrigger] = chi[img1use];
	chi_trigger_end[itrigger]   = chi[img1use];
	if (!isnan(chi_end[img1use])) {
	  chi_trigger_end[itrigger] = chi_end[img1use];
	}

	if (!isnan(chi[img2use])) {
	  chi_trigger_start[itrigger2] = chi[img2use];
	  chi_trigger_end[itrigger2]   = chi[img2use];
	  if (!isnan(chi_end[img2use])) {
	    chi_trigger_end[itrigger2] = chi_end[img2use];
	  }
	}
      }
    }

    if (itrigger==0) {
      if (esgo==1) {
	if (H5Lexists(fid,"/entry/sample",H5P_DEFAULT)>0) {
	  if (H5Lexists(fid,"/entry/sample/goniometer",H5P_DEFAULT)>0) {
	    if (H5Lexists(fid,"/entry/sample/goniometer/phi",H5P_DEFAULT)>0) {
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/phi\n");
	      phi                     = hdf5_read_ndouble(fid,"/entry/sample/goniometer/phi","degree",&nimages);
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/phi_end\n");
	      phi_end                 = hdf5_read_ndouble(fid,"/entry/sample/goniometer/phi_end","degree",&nimages);
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/phi_range_average\n");
	      phi_range_average       = hdf5_read_double (fid,"/entry/sample/goniometer/phi_range_average","degree");
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/phi_range_total\n");
	      phi_range_total         = hdf5_read_double (fid,"/entry/sample/goniometer/phi_range_total","degree");
	      if (iverb>2)              printf(" ... will read /entry/sample/goniometer/phi_increment\n");
	      phi_increment           = hdf5_read_double (fid,"/entry/sample/goniometer/phi_increment","degree");
	      if (isnan(phi_increment)&&nimages>1) {
		if (!isnan(phi[0])&&!isnan(phi[1])) {
		  phi_increment = phi[1]-phi[0];
		}
	      }
	      ihave_phi = 1;
	    }
	  }
	}
      }

      if (esgo==0 && ihave_phi==0) {
	if (iverb>2) printf(" ... check /entry/sample/sample_phi/phi\n");
	if (H5Lexists(fid,"/entry/sample/sample_phi",H5P_DEFAULT)>0) {
	  if (H5Lexists(fid,"/entry/sample/sample_phi/phi",H5P_DEFAULT)>0) {
	    int nd = hdf5_read_dataset_size(fid,"/entry/sample/sample_phi/phi");
	    if (iverb>2) {
	      printf(" ... will read %d item(s) /entry/sample/sample_phi/phi\n",nd);
	    }
	    phi = hdf5_read_ndouble(fid,"/entry/sample/sample_phi/phi","degree",&nimages);
	    phi_end = (double *) malloc((nimages * sizeof (double)));
	    if (nd==1) {
	      for (int id=0; id<nimages;id++ ) {
		phi_end[id]=phi[id];
	      }
	      phi_range_average = 0.0;
	      phi_range_total = 0.0;
	      phi_increment = 0.0;
	    } else {
	      phi_increment = phi[1]-phi[0];
	      for (int id=0; id<nimages;id++ ) {
		phi_end[id]=phi[id]+phi_increment;
	      }
	      phi_range_average = phi_increment;
	      phi_range_total = nimages*phi_increment;
	    }
	    ihave_phi=1;
	  }
	}
      }
    }

    if (ihave_phi==1) {
      if (!isnan(phi[img1use])) {
	if (itrigger==0) {
	  h->phis = phi[img1use];
	  if (!isnan(phi_end[img1use])) {
	    h->phie = phi_end[img1use];
	  } else {
	    h->phie = h->phis;
	  }
	}
	phi_trigger_start[itrigger] = phi[img1use];
	phi_trigger_end[itrigger]   = phi[img1use];
	if (!isnan(phi_end[img1use])) {
	  phi_trigger_end[itrigger] = phi_end[img1use];
	}

	if (!isnan(phi[img2use])) {
	  phi_trigger_start[itrigger2] = phi[img2use];
	  phi_trigger_end[itrigger2]   = phi[img2use];
	  if (!isnan(phi_end[img2use])) {
	    phi_trigger_end[itrigger2] = phi_end[img2use];
	  }
	}
      }
    }

    if (itrigger==0) {
      if (esgo==1) {
	if (H5Lexists(fid,"/entry/sample",H5P_DEFAULT)>0) {
	  if (H5Lexists(fid,"/entry/sample/goniometer",H5P_DEFAULT)>0) {
	    if (H5Lexists(fid,"/entry/sample/goniometer/two_theta",H5P_DEFAULT)>0) {
	      if (iverb>2)              printf(" ... will read /entry/instrument/detector/goniometer/two_theta\n");
	      two_theta               = hdf5_read_ndouble(fid,"/entry/instrument/detector/goniometer/two_theta","degree",&nimages);
	      if (iverb>2)              printf(" ... will read /entry/instrument/detector/goniometer/two_theta_end\n");
	      two_theta_end           = hdf5_read_ndouble(fid,"/entry/instrument/detector/goniometer/two_theta_end","degree",&nimages);
	      if (iverb>2)              printf(" ... will read /entry/instrument/detector/goniometer/two_theta_range_average\n");
	      two_theta_range_average = hdf5_read_double (fid,"/entry/instrument/detector/goniometer/two_theta_range_average","degree");
	      if (iverb>2)              printf(" ... will read /entry/instrument/detector/goniometer/two_theta_range_total\n");
	      two_theta_range_total   = hdf5_read_double (fid,"/entry/instrument/detector/goniometer/two_theta_range_total","degree");
	      ihave_two_theta = 1;
	    }
	  }
	}
      }
    }
    if (ihave_two_theta==1) {
      if (!isnan(two_theta[img1use])) {
	if (itrigger==0) {
	  h->twot = two_theta[img1use];
	}
	two_theta_trigger_start[itrigger] = two_theta[img1use];
	two_theta_trigger_end[itrigger]   = two_theta[img1use];
	if (!isnan(two_theta_end[img1use])) {
	  two_theta_trigger_end[itrigger] = two_theta_end[img1use];
	}

	if (!isnan(two_theta[img2use])) {
	  two_theta_trigger_start[itrigger2] = two_theta[img2use];
	  two_theta_trigger_end[itrigger2]   = two_theta[img2use];
	  if (!isnan(two_theta_end[img2use])) {
	    two_theta_trigger_end[itrigger2] = two_theta_end[img2use];
	  }
	}
      }
    }

    if (!isnan(omega_increment)) {
      if (omega_increment>0.0) {
	if (isnan(phi_increment)||phi_increment==0.0) {
	  printf(" rotation axis = \"OMEGA\"\n");
	}
	else if (phi_increment>0.0) {
	  printf("\n WARNING: it seems both OMEGA and PHI increments are set to non-zero values: %f %f ?\n",omega_increment,phi_increment);
	}
      }
      else if (phi_increment>0.0) {
	printf(" rotation axis = \"PHI\"\n");
      }
    }

    // check if this sweep is a continuation (in terms of angles) of previous one
    if (itrigger_prev>=0) {
      double d_omega     = 0.0;
      if (ihave_omega)     d_omega     =     omega_trigger_start[itrigger] -     omega_trigger_end[itrigger_prev];
      double d_kappa     = 0.0;
      if (ihave_kappa)     d_kappa     =     kappa_trigger_start[itrigger] -     kappa_trigger_end[itrigger_prev];
      double d_phi       = 0.0;
      if (ihave_phi)       d_phi       =       phi_trigger_start[itrigger] -       phi_trigger_end[itrigger_prev];
      double d_chi       = 0.0;
      if (ihave_chi)       d_chi       =       chi_trigger_start[itrigger] -       chi_trigger_end[itrigger_prev];
      double d_two_theta = 0.0;
      if (ihave_two_theta) d_two_theta = two_theta_trigger_start[itrigger] - two_theta_trigger_end[itrigger_prev];

      if (sqrt(d_omega*d_omega)<0.001&&sqrt(d_kappa*d_kappa)<0.001&&sqrt(d_chi*d_chi)<0.001&&sqrt(d_phi*d_phi)<0.001&&sqrt(d_two_theta*d_two_theta)<0.001) {
	printf(" Sweep-%-4d : continuation from previous\n",(itrigger+1));
      } else {
	printf("\n Sweep-%-4d : independent sweep\n",(itrigger+1));
	ndatasets++;
      }
    } else {
      if ((img+1)<(img2+1)) {
	printf("\n Sweep-%-4d :\n",(itrigger+1));
      }
      ndatasets++;
    }

    if ((img+1)<(img2+1)) {
      int imgnum_1 = (img +1);
      int imgnum_2 = (img2+1);
      if (h5check>0) {
	img_offset = nimage_to_imgnum[img1use] - imgnum_1;
	imgnum_1 = nimage_to_imgnum[img1use];
	imgnum_2 = nimage_to_imgnum[img2use];
      }
      printf("   from image %6d : Omega= %8.3f .. %8.3f  Kappa= %8.3f .. %8.3f  Chi= %8.3f .. %8.3f  Phi= %8.3f .. %8.3f  2-Theta= %8.3f .. %8.3f\n",imgnum_1,omega_trigger_start[itrigger],omega_trigger_end[itrigger],kappa_trigger_start[itrigger],kappa_trigger_end[itrigger],chi_trigger_start[itrigger],chi_trigger_end[itrigger],phi_trigger_start[itrigger],phi_trigger_end[itrigger],two_theta_trigger_start[itrigger],two_theta_trigger_end[itrigger]);
      printf("   to   image %6d : Omega= %8.3f .. %8.3f  Kappa= %8.3f .. %8.3f  Chi= %8.3f .. %8.3f  Phi= %8.3f .. %8.3f  2-Theta= %8.3f .. %8.3f\n",imgnum_2,omega_trigger_start[itrigger2],omega_trigger_end[itrigger2],kappa_trigger_start[itrigger2],kappa_trigger_end[itrigger2],chi_trigger_start[itrigger2],chi_trigger_end[itrigger2],phi_trigger_start[itrigger2],phi_trigger_end[itrigger2],two_theta_trigger_start[itrigger2],two_theta_trigger_end[itrigger2]);
    }

    itrigger_prev = itrigger2;
    img  = img  + nimages_per_trigger;
    img2 = img2 + nimages_per_trigger;
  }

  if (ntrigger>1) {
    if (ndatasets==ntrigger) {
      printf("\n Note: it seems that we have %d independent datasets consisting of %d images each.\n",ndatasets,nimages_per_trigger);
    }
    else if (ndatasets==1) {
      printf("\n Note: it seems that we have %d dataset consisting of %d images.\n",ndatasets,nimages);
    }
    else if (ndatasets>ntrigger) {
      printf("\n WARNING: there seems to be more datasets (%d) defined than we have triggers (%d) - how is that possible?\n",ndatasets,ntrigger);
    }
    else {
      printf("\n WARNING: there seems to be fewer datasets (%d) defined than we have triggers (%d) - a mixture of related and independent sweeps?\n",ndatasets,ntrigger);
    }
    printf("\n");
  }

  int is_standard_eiger=0;
  if (esgo==1) {
    if (eiger_fw_version!=NULL) {
      if (eiger_fw_version[0]!='\0') {
	is_standard_eiger = 1;
      }
    }
  }
  if (is_standard_eiger==1) {
    if (iverb>0) printf("\n Seems to be standard Eiger/HDF5 file\n\n");
  }
  else {
    if (iverb>0) printf("\n Seems to be non-standard Eiger/HDF5 file\n\n");
  }


  if (h5check>0) {
    img_offset = nimage_to_imgnum[img1use] - (img1use + 1 );
    printf("     Image number %d/%d\n\n",nimage_to_imgnum[img1use],nimage_to_imgnum[(nimages-1)]);
  } else {
    printf("     Image number %d/%d\n\n",(img1use+1),nimages);
  }

  if (img_offset>0) {
    printf("     Offset between image number name and image number position = %d\n\n",img_offset);
  }

  // get axis definitions
  if (H5Lexists(fid,"/entry/sample",H5P_DEFAULT)>0) {
    if (H5Lexists(fid,"/entry/sample/transformations",H5P_DEFAULT)>0) {
      char omega_str[CHAR_ARRAY_LEN] = "";
      if (H5Lexists(fid,"/entry/sample/transformations/omega",H5P_DEFAULT)>0) {
        strcpy(omega_str,"/entry/sample/transformations/omega");
      }
      else if (H5Lexists(fid,"/entry/sample/transformations/gonomega",H5P_DEFAULT)>0) {
        strcpy(omega_str,"/entry/sample/transformations/gonomega");
      }
      if (omega_str[0]!=0) {
	omega_axis     = hdf5_read_axis_vector(fid,omega_str);
        if (!isnan(omega_axis[0])&&!isnan(omega_axis[1])&&!isnan(omega_axis[2])) {
          if (iverb>1) printf(" omega axis   = %8.5f %8.5f %8.5f\n",omega_axis[0],omega_axis[1],omega_axis[2]);
        }
      }

      if (H5Lexists(fid,"/entry/sample/transformations/kappa",H5P_DEFAULT)>0) {
	kappa_axis     = hdf5_read_axis_vector(fid,"/entry/sample/transformations/kappa");
	if (!isnan(kappa_axis[0])&&!isnan(kappa_axis[1])&&!isnan(kappa_axis[2])) {
	  if (iverb>1) printf(" kappa axis   = %8.5f %8.5f %8.5f\n",kappa_axis[0],kappa_axis[1],kappa_axis[2]);
	}
      }
      if (H5Lexists(fid,"/entry/sample/transformations/chi",H5P_DEFAULT)>0) {
	chi_axis       = hdf5_read_axis_vector(fid,"/entry/sample/transformations/chi");
	if (!isnan(chi_axis[0])&&!isnan(chi_axis[1])&&!isnan(chi_axis[2])) {
	  if (iverb>1) printf(" chi axis     = %8.5f %8.5f %8.5f\n",chi_axis[0],chi_axis[1],chi_axis[2]);
	}
      }
      if (H5Lexists(fid,"/entry/sample/transformations/phi",H5P_DEFAULT)>0) {
	phi_axis       = hdf5_read_axis_vector(fid,"/entry/sample/transformations/phi");
	if (!isnan(phi_axis[0])&&!isnan(phi_axis[1])&&!isnan(phi_axis[2])) {
	  if (iverb>1) printf(" phi axis     = %8.5f %8.5f %8.5f\n",phi_axis[0],phi_axis[1],phi_axis[2]);
	}
      }
      if (H5Lexists(fid,"/entry/sample/transformations/two_theta",H5P_DEFAULT)>0) {
	two_theta_axis = hdf5_read_axis_vector(fid,"/entry/sample/transformations/two_theta");
	if (!isnan(two_theta_axis[0])&&!isnan(two_theta_axis[1])&&!isnan(two_theta_axis[2])) {
	  if (iverb>1) printf(" 2-theta axis = %8.5f %8.5f %8.5f\n",two_theta_axis[0],two_theta_axis[1],two_theta_axis[2]);
	}
      }
    }
  }

  if (H5Lexists(fid,"/entry/instrument",H5P_DEFAULT)>0) {
    if (H5Lexists(fid,"/entry/instrument/detector",H5P_DEFAULT)>0) {
      if (H5Lexists(fid,"/entry/instrument/detector/detector_distance",H5P_DEFAULT)>0) {
	detector_distance_vector =  hdf5_read_axis_vector(fid,"/entry/instrument/detector/detector_distance");
	if (!isnan(detector_distance_vector[0])&&!isnan(detector_distance_vector[1])&&!isnan(detector_distance_vector[2])) {
	  if (iverb>1) printf(" detector distance vector = %8.5f %8.5f %8.5f\n",detector_distance_vector[0],detector_distance_vector[1],detector_distance_vector[2]);
	}
      }
      if (H5Lexists(fid,"/entry/instrument/detector/module",H5P_DEFAULT)>0) {
	if (H5Lexists(fid,"/entry/instrument/detector/module/fast_pixel_direction",H5P_DEFAULT)>0) {
	  fast_pixel_vector =  hdf5_read_axis_vector(fid,"/entry/instrument/detector/module/fast_pixel_direction");
	  if (!isnan(fast_pixel_vector[0])&&!isnan(fast_pixel_vector[1])&&!isnan(fast_pixel_vector[2])) {
	    if (iverb>1) printf(" fast pixel vector = %8.5f %8.5f %8.5f\n",fast_pixel_vector[0],fast_pixel_vector[1],fast_pixel_vector[2]);
	  }
	}
	if (H5Lexists(fid,"/entry/instrument/detector/module/slow_pixel_direction",H5P_DEFAULT)>0) {
	  slow_pixel_vector =  hdf5_read_axis_vector(fid,"/entry/instrument/detector/module/slow_pixel_direction");
	  if (!isnan(slow_pixel_vector[0])&&!isnan(slow_pixel_vector[1])&&!isnan(slow_pixel_vector[2])) {
	    if (iverb>1) printf(" slow pixel vector = %8.5f %8.5f %8.5f\n",slow_pixel_vector[0],slow_pixel_vector[1],slow_pixel_vector[2]);
	  }
	}
      }
    }
  }

  if (iverb>2)  printf(" ... will read /entry/instrument/beam/incident_wavelength\n");
  double wave  = hdf5_read_double(fid,"/entry/instrument/beam/incident_wavelength","angstrom");
  if (!isnan(wave)) h->wave = (float) wave;

  if (iverb>2)  printf(" ... will read /entry/instrument/detector/beam_center_x\n");
  double beamx = hdf5_read_double(fid,"/entry/instrument/detector/beam_center_x","pixel");
  if (isnan(beamx)) {
    beamx = hdf5_read_double(fid,"/entry/instrument/detector/beam_centre_x","pixel");
  }
  if (isnan(beamx)) {
    beamx = hdf5_read_double(fid,"/entry/instrument/detector/beam_center_x","pixels");
  }
  if (isnan(beamx)) {
    beamx = hdf5_read_double(fid,"/entry/instrument/detector/beam_centre_x","pixels");
  }
  if (iverb>2)  printf(" ... will read /entry/instrument/detector/beam_center_y\n");
  double beamy = hdf5_read_double(fid,"/entry/instrument/detector/beam_center_y","pixel");
  if (isnan(beamy)) {
    beamy = hdf5_read_double(fid,"/entry/instrument/detector/beam_centre_y","pixel");
  }
  if (isnan(beamy)) {
    beamy = hdf5_read_double(fid,"/entry/instrument/detector/beam_center_y","pixels");
  }
  if (isnan(beamy)) {
    beamy = hdf5_read_double(fid,"/entry/instrument/detector/beam_centre_y","pixels");
  }
  if (!isnan(beamx)&&!isnan(beamy)) {
    h->beax = (float) beamx;
    h->beay = (float) beamy;
  }

  if (iverb>2)  printf(" ... will read /entry/instrument/detector/detector_distance\n");
  double dist  = hdf5_read_double(fid,"/entry/instrument/detector/detector_distance","m");
  if (isnan(dist)) {
    if (iverb>2)  printf(" ... will read /entry/instrument/detector_distance\n");
    dist  = hdf5_read_double(fid,"/entry/instrument/detector_distance","m");
    if (isnan(dist)) {
      if (iverb>2)  printf(" ... will read /entry/instrument/detector/distance\n");
      dist  = hdf5_read_double(fid,"/entry/instrument/detector/distance","m");
    }
  }
  if (!isnan(dist)) h->dist = (float) dist*1000.0;

  if (iverb>2)         printf(" ... will read /entry/instrument/detector/count_time\n");
  double count_time   = hdf5_read_double(fid,"/entry/instrument/detector/count_time","s");
  if (iverb>2)         printf(" ... will read /entry/instrument/detector/frame_time\n");
  double frame_time   = hdf5_read_double(fid,"/entry/instrument/detector/frame_time","s");
  if (isnan(frame_time)) {
    if (iverb>2)         printf(" ... will read /entry/instrument/detector/count_time\n");
    double count_time   = hdf5_read_double(fid,"/entry/instrument/detector/count_time","s");
    h->etime = count_time;
  } else {
    h->etime = frame_time;
  }
  if (iverb>2)         printf(" ... will read /entry/instrument/detector/detector_readout_time\n");
  double readout_time = hdf5_read_double(fid,"/entry/instrument/detector/detector_readout_time","s");

  int nx = INIT_INT;
  int ny = INIT_INT;
  if (H5Lexists(fid,"/entry/instrument/detector/detectorSpecific/x_pixels_in_detector",H5P_DEFAULT)>0) {
    if (iverb>2) printf(" ... will read /entry/instrument/detector/detectorSpecific/x_pixels_in_detector\n");
    nx   = hdf5_read_int   (fid,"/entry/instrument/detector/detectorSpecific/x_pixels_in_detector");
    if (iverb>2) printf(" ... will read /entry/instrument/detector/detectorSpecific/y_pixels_in_detector\n");
    ny   = hdf5_read_int   (fid,"/entry/instrument/detector/detectorSpecific/y_pixels_in_detector");
  }
  else if (H5Lexists(fid,"/entry/instrument/detector/detectorSpecific/x_pixels",H5P_DEFAULT)>0) {
    if (iverb>2) printf(" ... will read /entry/instrument/detector/detectorSpecific/x_pixels\n");
    nx   = hdf5_read_int   (fid,"/entry/instrument/detector/detectorSpecific/x_pixels");
    if (iverb>2) printf(" ... will read /entry/instrument/detector/detectorSpecific/y_pixels\n");
    ny   = hdf5_read_int   (fid,"/entry/instrument/detector/detectorSpecific/y_pixels");
  }
  else {
    int n_nxny = 2;
    int *nxny = hdf5_read_nint(fid,"/entry/instrument/detector/module/data_size",&n_nxny);
    if (nxny[0]!=INIT_INT) {
      // according to https://manual.nexusformat.org/classes/applications/NXmx.html
      //   ... order of indices is the same as for data_origin.
      //   The order of indices (i, j or i, j, k) is slow to fast.
      // see also: https://manual.nexusformat.org/datarules.html#design-arraystorageorder
      nx = nxny[1];
      ny = nxny[0];
    }
  }

  if (nx!=INIT_INT&&ny!=INIT_INT) {
    h->numx = nx;
    h->numy = ny;
  }

  double px = INIT_DOUBLE;
  double py = INIT_DOUBLE;
  if (H5Lexists(fid,"/entry/instrument/detector/x_pixel_size",H5P_DEFAULT)>0) {
    if (iverb>2) printf(" ... will read /entry/instrument/detector/x_pixel_size\n");
    px   = hdf5_read_double(fid,"/entry/instrument/detector/x_pixel_size","m");
    if (iverb>2) printf(" ... will read /entry/instrument/detector/y_pixel_size\n");
    py   = hdf5_read_double(fid,"/entry/instrument/detector/y_pixel_size","m");
  }
  if (!isnan(px)&&!isnan(py)) {
    h->pixx = (float) px*1000.0;
    h->pixy = (float) py*1000.0;
  }

  // given in m:
  if (iverb>2)             printf(" ... will read /entry/instrument/detector/sensor_thickness\n");
  double sensor_thickness = hdf5_read_double(fid,"/entry/instrument/detector/sensor_thickness","m");
  if (!isnan(sensor_thickness)) {
    h->thick = (float) sensor_thickness * 1000.0;
    if (h->thick>=320.0) {
      printf("\n WARNING: item \"/entry/instrument/detector/sensor_thickness\" gives value\n");
      printf("          of %.2f mm - which doesn't make sense. We will adjust this\n",h->thick);
      printf("          to %.5f, but please check your beamline/instrument!\n",(h->thick/1000.0));
      h->thick = h->thick / 1000.0;
    }
  }

  if (iverb>2)             printf(" ... will read /entry/instrument/detector/threshold_energy\n");
  double threshold_energy = hdf5_read_double(fid,"/entry/instrument/detector/threshold_energy","eV");

  if (iverb>2)         printf(" ... will read /entry/instrument/detector/detectorSpecific/countrate_correction_count_cutoff\n");
  int count_cutoff = hdf5_read_int(fid,"/entry/instrument/detector/detectorSpecific/countrate_correction_count_cutoff");
  if (count_cutoff!=INIT_INT) {
    h->ovld = count_cutoff;
  } else {
    if (iverb>2)         printf(" ... will read /entry/instrument/detector/saturation_value\n");
    count_cutoff = hdf5_read_int(fid,"/entry/instrument/detector/saturation_value");
    if (count_cutoff!=INIT_INT) {
      h->ovld = count_cutoff;
    }
  }

  if (iverb>2)  printf(" ... will read /entry/instrument/detector/detectorSpecific/nframes_sum\n");
  int nframes_sum = hdf5_read_int(fid,"/entry/instrument/detector/detectorSpecific/nframes_sum");
  if (iverb>2)  printf(" ... will read /entry/instrument/detector/detectorSpecific/nsequences\n");
  int nsequences  = hdf5_read_int(fid,"/entry/instrument/detector/detectorSpecific/nsequences");

  /* Close file */
  status = H5Fclose(fid);

  free(omega_trigger_start);
  free(kappa_trigger_start);
  free(chi_trigger_start);
  free(phi_trigger_start);
  free(two_theta_trigger_start);
  free(omega_trigger_end);
  free(kappa_trigger_end);
  free(chi_trigger_end);
  free(phi_trigger_end);
  free(two_theta_trigger_end);
 
  return 1;
}


// ==================================================================================================
// Report
// ==================================================================================================
void print_header(image_header *h, int idet, int inorm) {

  int nosc = 0;
  FLT64 d;

  if (h->omes>360.0) {
    printf("\n WARNING: Omega angle(s) given with value above 360.0 degree (%f)!\n",h->omes);
    if (inorm>0) printf("          This will be normalised in the output below.\n");
  }
  if (h->chis>360.0) {
    printf("\n WARNING: Chi angle(s) given with value above 360.0 degree\n");
  }
  if (h->kaps>360.0) {
    printf("\n WARNING: Kappa angle(s) given with value above 360.0 degree\n");
  }
  if (h->phis>360.0) {
    printf("\n WARNING: Phi angle(s) given with value above 360.0 degree (%f)!\n",h->phis);
    if (inorm>0) printf("          This will be normalised in the output below.\n");
  }
  if (h->omes<-360.0) {
    printf("\n WARNING: Omega angle(s) given with value below -360.0 degree (%f)!\n",h->omes);
    if (inorm>0) printf("          This will be normalised in the output below.\n");
  }
  if (h->chis<-360.0) {
    printf("\n WARNING: Chi angle(s) given with value below -360.0 degree - maybe as a marker for NA/NULL?\n");
  }
  if (h->kaps<-360.0) {
    printf("\n WARNING: Kappa angle(s) given with value below -360.0 degree - maybe as a marker for NA/NULL?\n");
  }
  if (h->phis<-360.0) {
    printf("\n WARNING: Phi angle(s) given with value below -360.0 degree (%f)!\n",h->phis);
    if (inorm>0) printf("          This will be normalised in the output below.\n");
  }
  if (h->twot<-360.0) {
    printf("\n WARNING: 2-Theta angle given with value below -360.0 degree - maybe as a marker for NA/NULL?\n");
  }
  if (h->twot>360.0) {
    printf("\n WARNING: 2-Theta angle given with value above 360.0 degree!\n");
  }

  printf("\n ===== Header information:\n");
  if (iverb>2) {
    printf(" [debug]    (print_header) h.pixx  = %f\n",h->pixx);
    printf(" [debug]    (print_header) h.pixy  = %f\n",h->pixy);
    printf(" [debug]    (print_header) h.dist  = %f\n",h->dist);
    printf(" [debug]    (print_header) h.wave  = %f\n",h->wave);
    printf(" [debug]    (print_header) h.phis  = %f\n",h->phis);
    printf(" [debug]    (print_header) h.phie  = %f\n",h->phie);
    printf(" [debug]    (print_header) h.omes  = %f\n",h->omes);
    printf(" [debug]    (print_header) h.omee  = %f\n",h->omee);
    printf(" [debug]    (print_header) h.chis  = %f\n",h->chis);
    printf(" [debug]    (print_header) h.chie  = %f\n",h->chie);
    printf(" [debug]    (print_header) h.kaps  = %f\n",h->kaps);
    printf(" [debug]    (print_header) h.kape  = %f\n",h->kape);
    printf(" [debug]    (print_header) h.twot  = %f\n",h->twot);
    printf(" [debug]    (print_header) h.numx  = %d\n",h->numx);
    printf(" [debug]    (print_header) h.numy  = %d\n",h->numy);
    printf(" [debug]    (print_header) h.beax  = %f\n",h->beax);
    printf(" [debug]    (print_header) h.beay  = %f\n",h->beay);
    printf(" [debug]    (print_header) h.etime = %f\n",h->etime);
    printf(" [debug]    (print_header) h.flux  = %f\n",h->flux);
    printf(" [debug]    (print_header) h.thick = %f\n",h->thick);
    printf(" [debug]    (print_header) h.fpol  = %f\n",h->fpol);
    printf(" [debug]    (print_header) h.msec  = %d\n",h->msec);
    printf(" [debug]    (print_header) h.detn  = %s\n",h->detn.c_str());
    printf(" [debug]    (print_header) h.sensm = %s\n",h->sensm.c_str());
    printf(" [debug]    (print_header) NaN     = %f\n",INIT_FLOAT);
  }

  if (h->date!="N/A") {
    if (h->msec>=0) {
      printf(" date                                = %s.%03d\n",h->date.c_str(),h->msec);
    } else {
      printf(" date                                = %s\n",h->date.c_str());
    }
    if (iverb>0) {
      if (h->msec>=0) {
        printf(" Time since Epoch          [seconds] = %ld.%03d\n",(long)h->epoch,h->msec);
      } else {
        printf(" Time since Epoch          [seconds] = %ld\n",(long)h->epoch);
      }
    }
  }
  if (! isnan(h->etime) ) {
    if (h->etime>0.0) {
      if        (h->etime>0.1) {
        printf(" exposure time             [seconds] = %.3f\n",h->etime);
      } else if (h->etime>0.01) {
        printf(" exposure time             [seconds] = %.4f\n",h->etime);
      } else if (h->etime>0.001) {
        printf(" exposure time             [seconds] = %.5f\n",h->etime);
      } else {
        printf(" exposure time             [seconds] = %.6f\n",h->etime);
      }
    }
  }
  if (! isnan(h->flux) ) {
    if (h->flux>0.0) {
      if (h->flux>1.0) {
        printf(" flux                      [unknown] = %.3f\n",h->flux);
      } else {
        printf(" flux                      [unknown] = %.6f\n",h->flux);
      }
    }
  }
  if (h->detn!="N/A" && idet>0 ) {
    printf(" detector ID                         = %s\n",h->detn.c_str());
  }
  if (! isnan(h->dist) ) {
    printf(" distance                       [mm] = %.3f\n",h->dist);
  }
  if (! isnan(h->wave) ) {
    printf(" wavelength                      [A] = %.6f\n",h->wave);
  }
  if (! isnan(h->thick) ) {
    printf(" sensor thickness               [mm] = %.3f\n",h->thick);
  }
  if (h->sensm!="N/A") {
    //    if (h->sensm!="Silicon") {
    printf(" sensor material                     = %s\n",h->sensm.c_str());
    //    }
  }
  if (! isnan(h->fpol) ) {
    printf(" fraction of polarization            = %.3f\n",h->fpol);
  }

  // ================== Phi ==================================================================
  if ( ! isnan(h->phis) && ! isnan(h->phie) ) {
    // take care of negative zeros:
    if ( h->phis < 0.0 && h->phis > -0.0001 ) {
      h->phis = 0.0;
    }
    if ( h->phie < 0.0 && h->phie > -0.0001 ) {
      h->phie = 0.0;
    }

    if (inorm>0) {
      d = 0.0;
      if (h->phis>360.0) {
        d = h->phis - fmod(h->phis,360.0);
      }
      h->phis = h->phis - d;
      h->phie = h->phie - d;
    }

    if ( h->phie != h->phis ) {
      // take care of weird information (like phie==0.0 and oscillation range >10.0 degree)
      if ( h->phie > h->phis && (h->phie != 0.0 || (h->phie-h->phis)<= 10.0)) {
	if ((h->phie - h->phis)<0.0001) {
	  printf(" Phi-angle (start, end)     [degree] = %.9f %.9f\n",h->phis,h->phie);
	  printf(" Oscillation-angle in Phi   [degree] = %.9f\n",(h->phie - h->phis));
	}
	else if ((h->phie - h->phis)<0.01) {
	  printf(" Phi-angle (start, end)     [degree] = %.7f %.8f\n",h->phis,h->phie);
	  printf(" Oscillation-angle in Phi   [degree] = %.7f\n",(h->phie - h->phis));
	}
	else {
	  printf(" Phi-angle (start, end)     [degree] = %.5f %.5f\n",h->phis,h->phie);
	  printf(" Oscillation-angle in Phi   [degree] = %.5f\n",(h->phie - h->phis));
	}
        nosc++;
      } else {
        if ( h->phie != 0.0 ) {
          printf(" Phi-angle (start, end)     [degree] = %.5f %.5f\n",h->phis,h->phie);
          printf(" WARNING: negative oscillation range for Phi?\n");
        } else {
          printf(" Phi-angle                  [degree] = %.5f\n",h->phis);
        }
      }
    } else {
      printf(" Phi-angle                  [degree] = %.5f\n",h->phis);
    }
  }

  // ================== Omega ================================================================
  if ( ! isnan(h->omes) && ! isnan(h->omee) ) {
    // take care of negative zeros:
    if ( h->omes < 0.0 && h->omes > -0.0001 ) {
      h->omes = 0.0;
    }
    if ( h->omee < 0.0 && h->omee > -0.0001 ) {
      h->omee = 0.0;
    }

    if (inorm>0) {
      d = 0.0;
      if (h->omes>360.0) {
        d = h->omes - fmod(h->omes,360.0);
      }
      h->omes = h->omes - d;
      h->omee = h->omee - d;
    }
    if ( h->omee != h->omes ) {
      if ( h->omee > h->omes ) {
	if ((h->omee - h->omes)<0.0001) {
	  printf(" Omega-angle (start, end)   [degree] = %.9f %.9f\n",h->omes,h->omee);
	  printf(" Oscillation-angle in Omega [degree] = %.9f\n",(h->omee - h->omes));
	}
	else if ((h->omee - h->omes)<0.01) {
	  printf(" Omega-angle (start, end)   [degree] = %.7f %.7f\n",h->omes,h->omee);
	  printf(" Oscillation-angle in Omega [degree] = %.7f\n",(h->omee - h->omes));
	}
	else {
	  printf(" Omega-angle (start, end)   [degree] = %.5f %.5f\n",h->omes,h->omee);
	  printf(" Oscillation-angle in Omega [degree] = %.5f\n",(h->omee - h->omes));
	}
        nosc++;
      } else {
	printf(" Omega-angle (start, end)   [degree] = %.5f %.5f\n",h->omes,h->omee);
        printf(" WARNING: negative oscillation range for Omega?\n");
      }
    } else {
      printf(" Omega-angle                [degree] = %.5f\n",h->omes);
    }
  }

  // ================== Chi ==================================================================
  if ( ! isnan(h->chis) && ! isnan(h->chie) ) {
    // take care of negative zeros:
    if ( h->chis < 0.0 && h->chis > -0.0001 ) {
      h->chis = 0.0;
    }
    if ( h->chie < 0.0 && h->chie > -0.0001 ) {
      h->chie = 0.0;
    }
    if ( h->chie != h->chis ) {
      if ( h->chie > h->chis ) {
	if ((h->chie - h->chis)<0.0001) {
	  printf(" Chi-angle (start, end)     [degree] = %.9f %.9f\n",h->chis,h->chie);
	  printf(" Oscillation-angle in Chi   [degree] = %.9f\n",(h->chie - h->chis));
	}
	else if ((h->chie - h->chis)<0.01) {
	  printf(" Chi-angle (start, end)     [degree] = %.7f %.7f\n",h->chis,h->chie);
	  printf(" Oscillation-angle in Chi   [degree] = %.7f\n",(h->chie - h->chis));
	}
	else {
	  printf(" Chi-angle (start, end)     [degree] = %.5f %.5f\n",h->chis,h->chie);
	  printf(" Oscillation-angle in Chi   [degree] = %.5f\n",(h->chie - h->chis));
	}
        nosc++;
      } else {
	printf(" Chi-angle (start, end)     [degree] = %.5f %.5f\n",h->chis,h->chie);
        printf(" WARNING: negative oscillation range for Chi?\n");
      }
    } else {
      printf(" Chi-angle                  [degree] = %.5f\n",h->chis);
    }
  }

  // ================== Kappa ================================================================
  if ( ! isnan(h->kaps) && ! isnan(h->kape) ) {
    // take care of negative zeros:
    if ( h->kaps < 0.0 && h->kaps > -0.0001 ) {
      h->kaps = 0.0;
    }
    if ( h->kape < 0.0 && h->kape > -0.0001 ) {
      h->kape = 0.0;
    }
    if ( h->kape != h->kaps ) {
      if ( h->kape > h->kaps ) {
	if ((h->kape - h->kaps)<0.0001) {
	  printf(" Kappa-angle (start, end)   [degree] = %.9f %.9f\n",h->kaps,h->kape);
	  printf(" Oscillation-angle in Kappa [degree] = %.9f\n",(h->kape - h->kaps));
	}
	else if ((h->kape - h->kaps)<0.01) {
	  printf(" Kappa-angle (start, end)   [degree] = %.7f %.7f\n",h->kaps,h->kape);
	  printf(" Oscillation-angle in Kappa [degree] = %.7f\n",(h->kape - h->kaps));
	}
	else {
	  printf(" Kappa-angle (start, end)   [degree] = %.5f %.5f\n",h->kaps,h->kape);
	  printf(" Oscillation-angle in Kappa [degree] = %.5f\n",(h->kape - h->kaps));
	}
        nosc++;
      } else {
	printf(" Kappa-angle (start, end)   [degree] = %.5f %.5f\n",h->kaps,h->kape);
        printf(" WARNING: negative oscillation range for Kappa?\n");
      }
    } else {
      printf(" Kappa-angle                [degree] = %.5f\n",h->kaps);
    }
  }

  // ================== 2-theta ==============================================================
  if (! isnan(h->twot) ) {
    if ( h->twot < 0.0 && h->twot > -0.0001 ) {
      h->twot = 0.0;
    }
    printf(" 2-Theta angle              [degree] = %.5f\n",h->twot);
  }

  if (! isnan(h->pixx) ) {
    printf(" Pixel size in X                [mm] = %.6f\n",h->pixx);
  }
  if (! isnan(h->pixy) ) {
    printf(" Pixel size in Y                [mm] = %.6f\n",h->pixy);
  }
  if ( h->numx != 0 ) {
    printf(" Number of pixels in X               = %d\n",h->numx);
  }
  if ( h->numy != 0 ) {
    printf(" Number of pixels in Y               = %d\n",h->numy);
  }

  if (! isnan(h->beax) ) {
    if (! isnan(h->pixx) ) {
      printf(" Beam centre in X               [mm] = %.3f\n",h->beax*h->pixx);
    }
    printf(" Beam centre in X            [pixel] = %.3f\n",h->beax);
  }
  if (! isnan(h->beay) ) {
    if (! isnan(h->pixy) ) {
      printf(" Beam centre in Y               [mm] = %.3f\n",h->beay*h->pixy);
    }
    printf(" Beam centre in Y            [pixel] = %.3f\n",h->beay);
  }
  if ( h->ovld != 0 ) {
    printf(" Overload value                      = %d\n",h->ovld);
  }

  /* some checks and warnings */
  if (isnan(h->dist)) {
    printf("\n");
    printf(" WARNING: unable to extract (crystal-to-detector) distance !!!\n");
  }
  if (isnan(h->wave)) {
    printf("\n");
    printf(" WARNING: unable to extract (crystal-to-detector) distance !!!\n");
  }
  if (isnan(h->pixx)||isnan(h->pixy)) {
    printf("\n");
    printf(" WARNING: unable to extract pixel size !!!\n");
  }
  if (isnan(h->numx)||isnan(h->numy)) {
    printf("\n");
    printf(" WARNING: unable to extract image array size !!!\n");
  }
  if (isnan(h->beax)||isnan(h->beay)) {
    printf("\n");
    printf(" WARNING: unable to extract beam centre !!!\n");
  }

  if (nosc>1) {
    printf("\n");
    printf(" WARNING: more than one angle marked with different start/stop values!\n");
    printf(" WARNING: oscillation around more than one axis?\n");
  }
  else if (nosc<1) {
    printf("\n");
    printf(" WARNING: unable to find oscillation axis/angles !!!\n");
  }
  printf("\n\n");
}

// ==================================================================================================
// initialisation
// ==================================================================================================
void empty_header(image_header* h) {
  h->format = FORMAT_UNKNOWN;
  h->dist  = INIT_DOUBLE;
  h->wave  = INIT_DOUBLE;
  h->osca  = INIT_DOUBLE;
  h->phis  = INIT_DOUBLE;
  h->phie  = INIT_DOUBLE;
  h->omes  = INIT_DOUBLE;
  h->omee  = INIT_DOUBLE;
  h->chis  = INIT_DOUBLE;
  h->chie  = INIT_DOUBLE;
  h->kaps  = INIT_DOUBLE;
  h->kape  = INIT_DOUBLE;
  h->twot  = INIT_DOUBLE;
  h->pixx  = INIT_DOUBLE;
  h->pixy  = INIT_DOUBLE;
  h->numx  = 0;
  h->numy  = 0;
  h->beax  = INIT_DOUBLE;
  h->beay  = INIT_DOUBLE;
  h->ovld  = 0;
  h->epoch = 0;
  h->etime = INIT_DOUBLE;
  h->flux  = INIT_DOUBLE;
  h->thick = INIT_DOUBLE;
  h->fpol  = INIT_DOUBLE;
  h->msec  = -1;
  h->detn  = "N/A";
  h->date  = "N/A";
  h->sensm = "N/A";
}

char *hdf5_read_char(hid_t fid, const char* item) {

  char* r = (char*)malloc(1);r[0] = '\0';
  hid_t did, space_c, memtype_c, filetype_c;
  hsize_t dims_c[1] = {1};
  int ndims_c, sdim_c;
  herr_t status;
  
  status = H5Lexists(fid,item,H5P_DEFAULT);
  if (status > 0 ) {
    did = H5Dopen2(fid,item, H5P_DEFAULT);

    filetype_c = H5Dget_type(did);
    sdim_c = H5Tget_size (filetype_c);
    sdim_c++;                         /* Make room for null terminator */
    space_c = H5Dget_space (did);
    ndims_c = H5Sget_simple_extent_dims (space_c, dims_c, NULL);
    memtype_c = H5Tcopy (H5T_C_S1);
    
    // if this is e.g. a variable-length string (e.g. 7OMC dataset):
    if (H5Tis_variable_str(filetype_c)>0&&ndims_c==0) {
      char *strdata = (char *) calloc(1024, sizeof(char));
      status = H5Dread(did, filetype_c, H5S_ALL, H5S_ALL, H5P_DEFAULT, &strdata);
      sdim_c = strlen(strdata);
      sdim_c++;
      r = (char *) malloc (dims_c[0] * sdim_c * sizeof (char));
      if (status>=0) {
        strncpy(r,strdata,sdim_c);
      }
      free(strdata);
    }

    // fixed length datatype
    else {
      status = H5Tset_size (memtype_c, sdim_c);
      r = (char *) malloc (dims_c[0] * sdim_c * sizeof (char));
      status = H5Dread(did, memtype_c, H5S_ALL, H5S_ALL, H5P_DEFAULT, r);
    }

    if (status>=0) {
      if (iverb>1) printf("     %s = \"%s\"\n",item,r);
    } else {
      if (iverb>2) printf("     WARNING: problem reading %s\n",item);
    }
    status = H5Dclose(did);
    if (status<0) {
      if (iverb>2) printf("     WARNING: problem when closing %s\n",item);
    } else {
      if (iverb>3) printf("     successfully closed %s\n",item);
    }
  } else {
    if (iverb>1) {
      printf("     WARNING: problem finding %s\n",item);
    }
  }
  return(r);
}

char *hdf5_read_group_attribute(hid_t fid, const char* item, const char* attribute) {

  char* r = (char*)malloc(1);r[0] = '\0';
  hid_t gid, space_c, memtype_c, filetype_c;
  hsize_t dims_c[1] = {1};
  int ndims_c, sdim_c;
  herr_t status;

  status = H5Lexists(fid,item,H5P_DEFAULT);
  if (status > 0) {
    gid = H5Gopen2(fid,item, H5P_DEFAULT);

    status = H5Aexists(gid,attribute);
    if (status>0) {
      hid_t attribute_id    = H5Aopen_name(gid,attribute);
      hid_t attribute_type  = H5Aget_type(attribute_id);
      hid_t attribute_space = H5Aget_space(attribute_id);
      size_t attribute_n = H5Tget_size(attribute_type);
      r = (char *)malloc(sizeof(char)*(attribute_n+1));
      status = H5Aread(attribute_id, attribute_type, r);
      // safety net (assume H5T_STR_NULLPAD)
      // see also: https://github.com/h5py/h5py/issues/727
      //           https://forum.hdfgroup.org/t/bug-writing-a-string-does-not-include-null-terminator/5281
      //           https://docs.hdfgroup.org/hdf5/develop/group___h5_t.html#gae5f38bfd4a4c557496b3194b5180212c
      r[attribute_n] = '\0';
      if (status>=0) {
	if (iverb>1) printf("     %s = \"%s\"\n",item,r);
      } else {
	if (iverb>2) printf("     WARNING: problem reading attribute %s from group %s\n",attribute,item);
      }
      H5Tclose(attribute_type);
      H5Sclose(attribute_space);
      H5Aclose(attribute_id);
    } else {
      if (iverb>1) {
	printf("     WARNING: problem finding attribute \"%s\" in group %s\n",attribute,item);
      }
    }
    status = H5Gclose(gid);
    if (status<0) {
      if (iverb>2) printf("     WARNING: problem when closing %s\n",item);
    } else {
      if (iverb>3) printf("     successfully closed %s\n",item);
    }
  } else {
    if (iverb>1) {
      printf("     WARNING: problem finding %s\n",item);
    }
  }
  return(r);
}

int hdf5_read_int(hid_t fid, const char* item) {

  // supports H5T_NATIVE_INT
  //          H5T_NATIVE_LONG
  //          H5T_NATIVE_ULONG
  //          H5T_NATIVE_USHORT
  //          H5T_NATIVE_UINT
  //          H5T_NATIVE_FLOAT
  //          H5T_NATIVE_DOUBLE
  int r = INIT_INT;
  hid_t did;
  herr_t status;
  int data_i[10];

  status = H5Lexists(fid,item,H5P_DEFAULT);
  if (status > 0 ) {
    did = H5Dopen2(fid, item, H5P_DEFAULT);
    hid_t ptyp = H5Dget_type(did);
    size_t ptyp_size;
    hid_t type = H5Tget_native_type(ptyp, H5T_DIR_DEFAULT);
    if (H5Tequal(type,H5T_NATIVE_INT)) {
      if (iverb>3) printf("     read INT from %s\n",item);
      status = H5Dread(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_i);
    }
    else if (H5Tequal(type,H5T_NATIVE_LONG)) {
      if (iverb>3) printf("     read LONG from %s\n",item);
      long data_d[10];
      status = H5Dread(did, H5T_NATIVE_LONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_d);
      if (status>=0) {
        data_i[0] = (int) data_d[0];
      }
    }
    else if (H5Tequal(type,H5T_NATIVE_ULONG)) {
      if (iverb>3) printf("     read UNSIGNED LONG from %s\n",item);
      unsigned long data_d[10];
      status = H5Dread(did, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_d);
      if (status>=0) {
        data_i[0] = (int) data_d[0];
      }
    }
    else if (H5Tequal(type,H5T_NATIVE_USHORT)) {
      if (iverb>3) printf("     read UNSIGNED SHORT from %s\n",item);
      unsigned short data_d[10];
      status = H5Dread(did, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_d);
      if (status>=0) {
        data_i[0] = (int) data_d[0];
      }
    }
    else if (H5Tequal(type,H5T_NATIVE_UINT)) {
      if (iverb>3) printf("     read UNSIGNED INT from %s\n",item);
      unsigned int data_d[10];
      status = H5Dread(did, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_d);
      if (status>=0) {
        data_i[0] = (int) data_d[0];
      }
    }
    else if (H5Tequal(type,H5T_NATIVE_DOUBLE)) {
      if (iverb>3) printf("     read DOUBLE from %s\n",item);
      double data_d[10];
      status = H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_d);
      if (status>=0) {
        data_i[0] = (int) data_d[0];
      }
    }
    else if (H5Tequal(type,H5T_NATIVE_FLOAT)) {
      if (iverb>3) printf("     read FLOAT from %s\n",item);
      float data_f[10];
      status = H5Dread(did, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_f);
      if (status>=0) {
        data_i[0] = (int) data_f[0];
      }
    }
    else {
      printf("     ERROR: unsupported datatype for item %s = %ld\n",item,type);
    }
    if (status>=0) {
      r = data_i[0];
      if (iverb>1) printf("     %s = %d\n",item,r);
    } else {
      if (iverb>2) printf("     WARNING: error when reading %s\n",item);
    }
    status = H5Dclose(did);
    if (status<0) {
      if (iverb>2) printf("     WARNING: problem when closing %s\n",item);
    } else {
      if (iverb>3) printf("     successfully closed %s\n",item);
    }
  } else {
    if (iverb>2) printf("     WARNING: %s doesn't exist\n",item);
  }
  return(r);

}

int hdf5_read_dataset_size(hid_t fid, const char* item) {

  int r = INIT_INT;
  hid_t did;
  herr_t status;

  status = H5Lexists(fid,item,H5P_DEFAULT);
  if (status > 0 ) {
    did = H5Dopen2(fid, item, H5P_DEFAULT);
    hid_t sid = H5Dget_space(did);
    r = (int) H5Sget_simple_extent_npoints(sid);
    if (iverb>2) printf("     dataset \"%s\" has size %d\n",item,r);
  } else {
    if (iverb>2) printf("     WARNING: %s doesn't exist\n",item);
  }
  return(r);

}

double hdf5_read_double(hid_t fid, const char* item, const char* unit) {

  double r = INIT_DOUBLE;
  hid_t did;
  herr_t status;
  double data_d[10];
  double fac = 1.0;

  status = H5Lexists(fid,item,H5P_DEFAULT);
  if (status > 0 ) {
    did = H5Dopen2(fid, item, H5P_DEFAULT);
    hid_t ptyp = H5Dget_type(did);
    size_t ptyp_size;
    hid_t type = H5Tget_native_type(ptyp, H5T_DIR_DEFAULT);
    if (H5Tequal(type,H5T_NATIVE_DOUBLE)) {
      if (iverb>3) printf("     read DOUBLE from %s\n",item);
      status = H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_d);
    } else {
      if (H5Tequal(type,H5T_NATIVE_INT)) {
        if (iverb>3) printf("     read INT from %s\n",item);
        int data_i[10];
        status = H5Dread(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_i);
        if (status>=0) {
          data_d[0] = (double) data_i[0];
        }
      }
      else if (H5Tequal(type,H5T_NATIVE_LONG)) {
        if (iverb>3) printf("     read LONG from %s\n",item);
        long data_i[10];
        status = H5Dread(did, H5T_NATIVE_LONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_i);
        if (status>=0) {
          data_d[0] = (double) data_i[0];
        }
      }
      else if (H5Tequal(type,H5T_NATIVE_FLOAT)) {
        if (iverb>3) printf("     read FLOAT from %s\n",item);
        float data_f[10];
        status = H5Dread(did, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_f);
        if (status>=0) {
          data_d[0] = (double) data_f[0];
        }
      }
      else {
        printf("     ERROR: unsupported datatype for item %s = %ld\n",item,type);
      }
    }

    if (status>=0) {
      r = data_d[0];
      if (strcmp(unit,"NULL")!=0) {
	int l = strlen(unit);
	status = H5Aexists(did,"units");
	if (status>0) {
	  hid_t attribute_id    = H5Aopen_name(did,"units");
	  hid_t attribute_type  = H5Aget_type(attribute_id);
	  hid_t attribute_space = H5Aget_space(attribute_id);
	  size_t attribute_n = H5Tget_size(attribute_type);
	  char* string_array = (char *)malloc(sizeof(char)*(attribute_n+1));
	  H5Aread(attribute_id, attribute_type, string_array);
	  string_array[attribute_n] = '\0';
	  if ( (l!=attribute_n) || (strncmp(unit,string_array,l)!=0)) {
	    if      ((strcmp(unit,"m")==0)&&(strcmp(string_array,"mm")==0)) {
	      fac = 0.001;
	    }
	    else if ((strcmp(unit,"mm")==0)&&(strcmp(string_array,"m")==0)) {
	      fac = 1000.0;
	    }
	    // accept degree/deg "mismatch"
	    else if ((strcmp(unit,"degree")==0)&&(strcmp(string_array,"deg")==0)) {
	      fac = 1.0;
	    }
	    else if ((strcmp(unit,"deg")==0)&&(strcmp(string_array,"degree")==0)) {
	      fac = 1.0;
	    }
	    else if (strncmp(unit,string_array,(l-1))!=0) {
	      printf("\n WARNING: expected unit \"%s\" for item \"%s\" - but found unit \"%s\" (length=%d)!!\n\n",unit,item,string_array,l);
	    }
	  }
	  if (iverb>1) {
	    printf("     %s = %f %s\n",item,r,unit);
	  }
	  free(string_array);
	  H5Tclose(attribute_type);
	  H5Sclose(attribute_space);
	  H5Aclose(attribute_id);
	} else {
	  if (iverb>1) {
	    printf("     WARNING: problem finding attribute \"units\" in group %s\n",item);
	  }
	}
      } else {
	if (iverb>1) {
	  printf("     %s = %f\n",item,r);
	}
      }
    } else {
      if (iverb>2) printf("     WARNING: problem when reading %s\n",item);
    }
    status = H5Dclose(did);
    if (status<0) {
      if (iverb>2) printf("     WARNING: problem when closing %s\n",item);
    } else {
      if (iverb>3) printf("     successfully closed %s\n",item);
    }
  } else {
    if (iverb>2) printf("     WARNING: %s doesn't exist\n",item);
  }
  return(r*fac);

}

double* hdf5_read_ndouble(hid_t fid, const char* item, const char* unit, int* n) {

  int s = (*n * sizeof (double));
  double *d = (double*) malloc(s);
  hid_t did;
  herr_t status;

  status = H5Lexists(fid,item,H5P_DEFAULT);
  if (status > 0 ) {
    did = H5Dopen2(fid, item, H5P_DEFAULT);

    hid_t sid = H5Dget_space(did);
    hsize_t nid = H5Sget_simple_extent_npoints(sid);
    if (nid>*n) {
      if (iverb>1) printf("     WARNING: requested to read only %d items while data has size %llu\n",*n,nid);
      s = (nid * sizeof (double));
      double *d_tmp = (double*) malloc(s);
      // check if these are truly H5T_NATIVE_DOUBLE
      hid_t ptyp = H5Dget_type(did);
      size_t ptyp_size;
      hid_t type = H5Tget_native_type(ptyp, H5T_DIR_DEFAULT);
      if (H5Tequal(type,H5T_NATIVE_DOUBLE)) {
        if (iverb>3) printf("     read %llu DOUBLEs from %s\n",nid,item);
        status = H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, d_tmp);
      } else {
        if (H5Tequal(type,H5T_NATIVE_FLOAT)) {
          float *f_tmp = (float*) malloc(s);
          if (iverb>3) printf("     read %llu FLOATs from %s\n",nid,item);
          status = H5Dread(did, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, f_tmp);
          if (status>=0) {
            for(int i=0; i<*n; i++) {
              d_tmp[i]=(double) f_tmp[i];
            }
          }
          free(f_tmp);
        } else {
          printf("     ERROR: unsupported datatype for item %s = %ld\n",item,type);
        }
      }
      for(int i=0; i<*n; i++) {
	d[i]=d_tmp[i];
	if (iverb>3) printf("     setting d[%d] = d_tmp[%d] = %f\n",i,i,d[i]);
      }
      free(d_tmp);
    }
    else if (nid<*n) {
      if (iverb>1) printf("     WARNING: requested to read %d items while data has only size %llu\n",*n,nid);
      status = H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, d);
      if (nid==1) {
	if (iverb>1) printf("              will set all items to first/only one stored = %f\n",d[0]);
	for(int i=1; i<*n; i++) {
	  d[i]=d[0];
	}
      } else {
	if (iverb>1) printf("              will set all items to last one stored = %f\n",d[(nid-1)]);
	for(int i=nid; i<*n; i++) {
	  d[i]=d[(nid-1)];
	}
      }
    }
    else {
      status = H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, d);
    }
    if (status>=0) {
      if (iverb>1) {
	printf("     %s[0] = %f\n",item,d[0]);
	if (*n>1) {
	  printf("     %s[1] = %f\n",item,d[1]);
	  if (*n>2) {
	    printf("     %s[2] = %f\n",item,d[2]);
	  }
	} else {
	  if (iverb>2) printf("     only one item to be read - not reporting further values\n");
	}
      }
    } else {
      if (iverb>2) printf("     WARNING: problem when reading %s\n",item);
      d[0] = INIT_DOUBLE;
    }
    status = H5Dclose(did);
    if (status<0) {
      if (iverb>2) printf("     WARNING: problem when closing %s\n",item);
    } else {
      if (iverb>3) printf("     successfully closed %s\n",item);
    }
  } else {
    d[0] = INIT_DOUBLE;
    if (iverb>2) printf("     %s[0] set to INIT_DOUBLE because it doesn't exist\n",item);
  }

  return(d);

}

int* hdf5_read_nint(hid_t fid, const char* item, int* n) {

  // supports H5T_NATIVE_INT
  //          H5T_NATIVE_LONG
  //          H5T_NATIVE_ULONG
  //          H5T_NATIVE_USHORT
  //          H5T_NATIVE_UINT
  //          H5T_NATIVE_FLOAT
  //          H5T_NATIVE_DOUBLE

  int s = (*n * sizeof (int));
  int *d = (int*) malloc(s);
  hid_t did;
  herr_t status;

  status = H5Lexists(fid,item,H5P_DEFAULT);
  if (status > 0 ) {
    did = H5Dopen2(fid, item, H5P_DEFAULT);
    hid_t sid = H5Dget_space(did);
    hsize_t nid = H5Sget_simple_extent_npoints(sid);
    if (nid>*n) {
      if (iverb>1) printf("     WARNING: requested to read only %d items while data has size %llu\n",*n,nid);
      s = (nid * sizeof (int));
      int *d_tmp = (int*) malloc(s);
      // check if these are truly H5T_NATIVE_INT
      hid_t ptyp = H5Dget_type(did);
      size_t ptyp_size;
      hid_t type = H5Tget_native_type(ptyp, H5T_DIR_DEFAULT);
      if (H5Tequal(type,H5T_NATIVE_INT)) {
        if (iverb>3) printf("     read INT from %s\n",item);
        status = H5Dread(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, d_tmp);
      }
      else {
        if (H5Tequal(type,H5T_NATIVE_DOUBLE)) {
          if (iverb>3) printf("     read DOUBLE from %s\n",item);
          double *dd_tmp = (double*) malloc(s);
          status = H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dd_tmp);
          // cast to int
          for(int i=0; i<*n; i++) {
            d_tmp[i]=(int) dd_tmp[i];
          }
          free(dd_tmp);
        }
        else if (H5Tequal(type,H5T_NATIVE_FLOAT)) {
          if (iverb>3) printf("     read FLOAT from %s\n",item);
          float *dd_tmp = (float*) malloc(s);
          status = H5Dread(did, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dd_tmp);
          // cast to int
          for(int i=0; i<*n; i++) {
            d_tmp[i]=(int) dd_tmp[i];
          }
          free(dd_tmp);
        }
        else if (H5Tequal(type,H5T_NATIVE_LONG)) {
          if (iverb>3) printf("     read LONG from %s\n",item);
          long *dd_tmp = (long*) malloc(s);
          status = H5Dread(did, H5T_NATIVE_LONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, dd_tmp);
          // cast to int
          for(int i=0; i<*n; i++) {
            d_tmp[i]=(int) dd_tmp[i];
          }
          free(dd_tmp);
        }
        else if (H5Tequal(type,H5T_NATIVE_ULONG)) {
          if (iverb>3) printf("     read UNSIGNED LONG from %s\n",item);
          unsigned long *dd_tmp = (unsigned long*) malloc(s);
          status = H5Dread(did, H5T_NATIVE_ULONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, dd_tmp);
          // cast to int
          for(int i=0; i<*n; i++) {
            d_tmp[i]=(int) dd_tmp[i];
          }
          free(dd_tmp);
        }
        else if (H5Tequal(type,H5T_NATIVE_USHORT)) {
          if (iverb>3) printf("     read UNSIGNED SHORT from %s\n",item);
          unsigned short *dd_tmp = (unsigned short*) malloc(s);
          status = H5Dread(did, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dd_tmp);
          // cast to int
          for(int i=0; i<*n; i++) {
            d_tmp[i]=(int) dd_tmp[i];
          }
          free(dd_tmp);
        }
        else if (H5Tequal(type,H5T_NATIVE_UINT)) {
          if (iverb>3) printf("     read UNSIGNED INT from %s\n",item);
          unsigned int *dd_tmp = (unsigned int*) malloc(s);
          status = H5Dread(did, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dd_tmp);
          // cast to int
          for(int i=0; i<*n; i++) {
            d_tmp[i]=(int) dd_tmp[i];
          }
          free(dd_tmp);
        }
        else {
          printf("     ERROR: unsupported datatype for item %s = %ld\n",item,type);
        }
      }
      for(int i=0; i<*n; i++) {
	d[i]=d_tmp[i];
	if (iverb>3) printf("     setting d[%d] = d_tmp[%d] = %d\n",i,i,d[i]);
      }
      free(d_tmp);
    } else {
      status = H5Dread(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, d);
    }
    if (status>=0) {
      if (iverb>1) {
	printf("     %s[0] = %d\n",item,d[0]);
	if (*n>1) {
	  printf("     %s[1] = %d\n",item,d[1]);
	  if (*n>2) {
	    printf("     %s[2] = %d\n",item,d[2]);
	  }
	} else {
	  if (iverb>2) printf("     only one item to be read - not reporting further values\n");
	}
      }
    } else {
      if (iverb>2) printf("     WARNING: problem when reading %s\n",item);
      d[0] = INIT_INT;
    }
    status = H5Dclose(did);
    if (status<0) {
      if (iverb>2) printf("     WARNING: problem when closing %s\n",item);
    } else {
      if (iverb>3) printf("     successfully closed %s\n",item);
    }
  } else {
    d[0] = INIT_INT;
    if (iverb>2) printf("     %s[0] set to INIT_DOUBLE because it doesn't exist\n",item);
  }

  return(d);

}
double* hdf5_read_axis_vector(hid_t fid, const char* item) {

  int s = (3 * sizeof (double));
  double *d = (double*) malloc(s);

  d[0]=INIT_DOUBLE;
  d[1]=INIT_DOUBLE;
  d[2]=INIT_DOUBLE;

  hid_t gid;
  herr_t status;
  double data_d[10];
  H5O_info_t info;

  if (H5Lexists(fid,item,H5P_DEFAULT)>0) {
    status = H5Oget_info_by_name(fid,item,&info,H5P_DEFAULT);
    switch (info.type) {
      case H5O_TYPE_DATASET:
	gid = H5Dopen2(fid, item, H5P_DEFAULT);
	if (gid < 0) {
	  printf("\n\n ERROR - group/dataset=\"%s\" doesn't exist!\n\n",item);
	  return(d);
	}
	break;
      case H5O_TYPE_GROUP:
	gid = H5Gopen2(fid, item, H5P_DEFAULT);
	if (gid < 0) {
	  printf("\n\n ERROR - in H5Gopen2 (group=\"%s\")!\n\n",item);
	  return(d);
	}
	break;
    }

    if (H5Aexists(gid,"vector")>0) {
      hid_t attribute_id    = H5Aopen_name(gid,"vector");
      hid_t attribute_type  = H5Aget_type(attribute_id);
      hid_t attribute_space = H5Aget_space(attribute_id);
      size_t attribute_n    = H5Tget_size(attribute_type);

      if (H5Aread(attribute_id,H5T_NATIVE_DOUBLE,data_d) >= 0 ) {
	if (iverb>2) printf("     %s vector = %f %f %f\n",item,data_d[0],data_d[1],data_d[2]);
	d[0]=data_d[0];
	d[1]=data_d[1];
	d[2]=data_d[2];
      }
      H5Tclose(attribute_type);
      H5Sclose(attribute_space);
      H5Aclose(attribute_id);
    }
    switch (info.type) {
      case H5O_TYPE_DATASET:
	status = H5Dclose(gid);
	break;
      case H5O_TYPE_GROUP:
	status = H5Gclose(gid);
	break;
    }
    if (status<0) {
      if (iverb>2) printf("     WARNING: problem when closing %s\n",item);
    } else {
      if (iverb>3) printf("     successfully closed %s\n",item);
    }
  } else {
    if (iverb>2) printf("     WARNING: %s doesn't exist\n",item);
  }
  return(d);

}
int hdf5_list_filters (hid_t plist) {
    int nfilt = H5Pget_nfilters(plist);
    if (nfilt<=0) return(1);
    unsigned ifilt;
    unsigned     filt_flags;          /* filter flags */
    H5Z_filter_t filt_id;             /* filter identification number */
    unsigned     filt_cd_values[20];  /* filter client data values */
    filt_cd_values[0] =  1;
    filt_cd_values[1] =  2;
    filt_cd_values[2] =  3;
    filt_cd_values[3] =  4;
    filt_cd_values[4] =  5;
    size_t       filt_cd_nelmts = 0;  /* filter client number of values */
    char         filt_name[256];      /* filter name */
    unsigned     filt_config;         /* Bit field, as described in H5Zget_filter_info. */
    htri_t avail;

    if (iverb>2) printf(" Number of filters in file = %d\n",nfilt);

    for (ifilt = 0; ifilt < nfilt; ifilt++ ) {
      filt_id = H5Pget_filter(plist,ifilt,&filt_flags,&filt_cd_nelmts,filt_cd_values,sizeof(filt_name),filt_name,&filt_config);
      if (iverb>=0) printf(" Filter-%d : %s (%d)\n",ifilt,filt_name,filt_id);
      switch (filt_id) {
      case H5Z_FILTER_DEFLATE:
	if (iverb>=1) printf("   natively supported filter = H5Z_FILTER_DEFLATE\n");
	avail = H5Zfilter_avail(filt_id);
	if (!avail) {
	  printf ("\n\n ERROR - gzip filter not available.\n");
	  exit(EXIT_FAILURE);
	}
	break;
      case H5Z_FILTER_SHUFFLE:
	if (iverb>=1) printf("   natively supported filter = H5Z_FILTER_SHUFFLE\n");
	avail = H5Zfilter_avail(filt_id);
	if (!avail) {
	  printf ("\n\n ERROR - Shuffle filter not available.\n");
	  exit(EXIT_FAILURE);
	}
	break;
      case H5Z_FILTER_FLETCHER32:
	if (iverb>=1) printf("   natively supported filter = H5Z_FILTER_FLETCHER32\n");
	avail = H5Zfilter_avail(filt_id);
	if (!avail) {
	  printf ("\n\n ERROR - fletcher32 filter not available.\n");
	  exit(EXIT_FAILURE);
	}
	break;
      case H5Z_FILTER_SZIP:
	if (iverb>=1) printf("   natively supported filter = H5Z_FILTER_SZIP\n");
	avail = H5Zfilter_avail(filt_id);
	if (!avail) {
	  printf ("\n\n ERROR - szip filter not available.\n");
	  exit(EXIT_FAILURE);
	}
	break;
      case H5Z_FILTER_NBIT:
	if (iverb>=1) printf("   natively supported filter = H5Z_FILTER_NBIT\n");
	avail = H5Zfilter_avail(filt_id);
	if (!avail) {
	  printf ("\n\n ERROR - n-bit filter not available.\n");
	  exit(EXIT_FAILURE);
	}
	break;
      case H5Z_FILTER_SCALEOFFSET:
	if (iverb>=1) printf("   natively supported filter = H5Z_FILTER_SCALEOFFSET\n");
	avail = H5Zfilter_avail(filt_id);
	if (!avail) {
	  printf ("\n\n ERROR - scale-offset filter not available.\n");
	  exit(EXIT_FAILURE);
	}
      default:
#ifdef USE_BITSHUFFLE
#ifdef USE_LZ4
	if ( filt_id!=LZ4_FILTER && filt_id!=BSHUF_H5FILTER ) {
	  printf("\n\n ERROR - unsupported filter Filter-%d : %s (%d)!\n",ifilt,filt_name,filt_id);
	  printf("         supported filters are BITSHUFFLE (%d) and LZ4 (%d)\n\n",BSHUF_H5FILTER,LZ4_FILTER);
	  exit(EXIT_FAILURE);
	} else {
          if (iverb>=1&&filt_id==LZ4_FILTER    ) {
            printf("   supported filter = LZ4_FILTER\n");
          }
          else if (iverb>=1&&filt_id==BSHUF_H5FILTER) {
            printf("   supported filter = BSHUF_H5FILTER\n");
          }
          else {
            printf("   unsupported filter = %d\n",filt_id);
          }
        }
#else
	if ( filt_id!=BSHUF_H5FILTER ) {
	  printf("\n\n ERROR - unsupported filter Filter-%d : %s (%d)!\n",ifilt,filt_name,filt_id);
	  printf("         supported filter is BITSHUFFLE (%d)\n",BSHUF_H5FILTER);
	  exit(EXIT_FAILURE);
	} else {
          if (iverb>=1&&filt_id==BSHUF_H5FILTER) {
            printf("   supported filter = BSHUF_H5FILTER\n");
          }
          else {
            printf("   unsupported filter = %d\n",filt_id);
          }
        }
#endif
#else
#ifdef USE_LZ4
	if ( filt_id!=LZ4_FILTER ) {
	  printf("\n\n ERROR - unsupported filter Filter-%d : %s (%d)!\n",ifilt,filt_name,filt_id);
	  printf("         supported filter is LZ4 (%d)\n",LZ4_FILTER);
	  exit(EXIT_FAILURE);
	} else {
          if (iverb>=1&&filt_id==LZ4_FILTER    ) {
            printf("   supported filter = LZ4_FILTER\n");
          }
          else {
            printf("   unsupported filter = %d\n",filt_id);
          }
        }
#endif
#endif
      }
    }
    return(1);
}

