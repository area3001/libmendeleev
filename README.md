Mendeleev C library
====================

Overview
--------

libmendeleev is a free software library to send/receive data with a device which
respects the Mendeleev protocol. This library can use a serial port.

The functions included in the library have been based on the Modbus RTU
Protocol implemented in libmodbus [www.libmodbus.org](http://www.libmodbus.org).

The license of libmendeleev is *LGPL v2.1 or later*.

The library is written in C and designed to run on Linux.

Installation
------------

You will only need to install automake, autoconf, libtool and a C compiler (gcc
or clang) to compile the library and asciidoc and xmlto to generate the
documentation (optional).

To install, just run the usual dance, `./configure && make install`. Run
`./autogen.sh` first to generate the `configure` script if required.

You can change installation directory with prefix option, eg. `./configure
--prefix=/usr/local/`. You have to check that the installation library path is
properly set up on your system (*/etc/ld.so.conf.d*) and library cache is up to
date (run `ldconfig` as root if required).

The library provides a *libmendeleev.pc* file to use with `pkg-config` to ease your
program compilation and linking.
