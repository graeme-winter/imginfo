SHELL    = /bin/sh

# Full path (in case e.g. CCP4 is in the PATH with their own, broken
# versions):
HXX      = /usr/bin/h5c++
HXXFLAGS = -O2 -Wall
HCC      = /usr/bin/h5cc
HCCFLAGS = -O2 -Wall

LDFLAGS = -Wl,-u,pthread_self -lpthread

BITSHUFFLE_MASTER = ./bitshuffle-master
HDF5PLUGIN_MASTER = ./HDF5Plugin-master
HDF5FILTER_MASTER = ./HDF5-External-Filter-Plugins-master

HXXFLAGS += -DUSE_BITSHUFFLE -I $(BITSHUFFLE_MASTER)/src
HXXFLAGS += -DUSE_LZ4        -I $(BITSHUFFLE_MASTER)/lz4
HXXFLAGS += -DUSE_BZIP2
HXXFLAGS += -DUSE_HDF5

LDFLAGS  += -L$(HDF5_LIB) -lhdf5 -lz -lbz2

COMPILE.hxx = $(HXX) $(HXXFLAGS) -c -o $(1) $(2)
COMPILE.hcc = $(HCC) $(HCCFLAGS) -c -o $(1) $(2)
LINK.hxx    = $(HXX) $(HXXFLAGS)    -o $(1) $(2) $(LDFLAGS)
LINK.hcc    = $(HCC) $(HCCFLAGS)    -o $(1) $(2) $(LDFLAGS)

default: imginfo

imginfo: imginfo.o bshuf_h5filter.o bitshuffle.o lz4.o h5zlz4.o H5Zbzip2.o bitshuffle_core.o iochain.o 
	$(call LINK.hxx,$@,$^)

%.o : %.c
	$(call COMPILE.hxx,$@,$<)

%.o : $(BITSHUFFLE_MASTER)/src/%.c
	$(call COMPILE.hcc,$@,$<)

%.o : $(BITSHUFFLE_MASTER)/lz4/%.c
	$(call COMPILE.hcc,$@,$<)

H5Zbzip2.o : $(HDF5FILTER_MASTER)/BZIP2/src/H5Zbzip2.c
	egrep -v "H5PLget_plugin_type|H5PLget_plugin_info" $< > H5Zbzip2.c
	$(call COMPILE.hcc,$@,H5Zbzip2.c)
	rm H5Zbzip2.c

%.o : $(HDF5PLUGIN_MASTER)/%.c
	$(call COMPILE.hcc,$@,$<)

.SECONDARY:
