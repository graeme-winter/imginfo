# imginfo

IMGINFO is a C/C++ program to extract selected items from meta-data
stored in HDF5 (so-called "master") files that are necessary for
processing rotational MX data - e.g. with
[autoPROC](https://www.globalphasing.com/autoproc/) (which comes with
it's own version of this program).

## Getting Started

You will need
* a C/C++ compiler
* the HDF5 tools "h5cc" and "h5c++"
* several external libraries and plugins

This should be achievable e.g. on Ubuntu 22.04 using the following
commands (in the same directory with this source code):

```
sudo apt-get install hdf5-helpers

wget -O bitshuffle-master.zip \
  https://github.com/kiyo-masui/bitshuffle/archive/refs/heads/master.zip
unzip bitshuffle-master.zip

wget -O HDF5Plugin-master.zip \
  https://github.com/dectris/HDF5Plugin/archive/refs/heads/master.zip
unzip HDF5Plugin-master.zip

wget -O HDF5-External-Filter-Plugins-master.zip \
  https://github.com/nexusformat/HDF5-External-Filter-Plugins/archive/refs/heads/master.zip
unzip HDF5-External-Filter-Plugins-master.zip
```

Compilation should then just work with
```
make
```

## Running

For help see
```
./imginfo -h
```
## Authors

* **Clemens Vonrhein**
* **Claus Flensburg**
* **Thomas Womack**
* **Gerard Bricogne**

See also

* [autoPROC](https://www.globalphasing.com/autoproc/): [Vonrhein, C., Flensburg, C., Keller, P., Sharff, A., Smart, O., Paciorek, W., Womack, T. & Bricogne, G. (2011). Data processing and analysis with the autoPROC toolbox. Acta Cryst. D67, 293-302.](https://scripts.iucr.org/cgi-bin/paper?ba5166)

* [Bernstein, H.J., Förster, A., Bhowmick, A., Brewster, A.S., Brockhauser, S., Gelisio, L., Hall, D.R., Leonarski, F., Mariani, V., Santoni, G. and Vonrhein, C., 2020. Gold Standard for macromolecular crystallography diffraction data. IUCrJ, 7(5), pp.784-792.](https://scripts.iucr.org/cgi-bin/paper?ti5018)

## License

This project is licensed under the Mozilla Public License, v. 2.0 -
see the [LICENSE.md](LICENSE.md) file for details. © 2007-2019 Global Phasing Ltd.

## Acknowledgments

* All users provides us with examples of HDF5 datasets - especially
  our colleagues and collaborators at various synchrotrons and
  beamlines.

* Our colleagues of the [HDRMX group](http://hdrmx.medsbio.org/).

* All members of the [Global Phasing
  Consortium](https://www.globalphasing.com/)
