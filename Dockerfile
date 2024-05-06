FROM ubuntu:22.04

ADD . /imginfo/

WORKDIR "/imginfo" 

RUN apt-get update && apt-get -y install hdf5-helpers wget unzip build-essential libhdf5-dev libhdf5-serial-dev libbz2-dev
RUN wget -O bitshuffle-master.zip https://github.com/kiyo-masui/bitshuffle/archive/refs/heads/master.zip && unzip bitshuffle-master.zip
RUN wget -O HDF5Plugin-master.zip https://github.com/dectris/HDF5Plugin/archive/refs/heads/master.zip && unzip HDF5Plugin-master.zip
RUN wget -O HDF5-External-Filter-Plugins-master.zip https://github.com/nexusformat/HDF5-External-Filter-Plugins/archive/refs/heads/master.zip && unzip HDF5-External-Filter-Plugins-master.zip

RUN make
