#!/bin/bash
set -e

################################################################################
# make-windows-dependencies.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to cross-build the DLVHEX library depdencencies for Windows
# on Linux. Make sure to call check-windows.sh to check for missing
# dependencies before.
#
# Parameters:
#
# --verbose
#
#   Forward all build log output to stdout.
#
################################################################################


# Config vars
ZLIB_VERSION="1.2.8"
OPENSSL_VERSION="1.0.2g"
CURL_VERSION="7.46.0"
BZIP_VERSION="1.0.6"
BOOST_VERSION="1.57.0" # 1.59.0 is not working for cc
PYTHON_VERSION="2.7"

OUTPUT_IO=/dev/null
MINGW_DIR=/usr/i686-w64-mingw32

# Check parameters
while [ "$1" != "" ]; do
  case $1 in
    --verbose )     OUTPUT_IO=/dev/stdout
                    set -v
                    ;;
  esac
  shift
done


# Build zlib
wget http://zlib.net/zlib-$ZLIB_VERSION.tar.gz &> $OUTPUT_IO
tar xzf zlib-$ZLIB_VERSION.tar.gz &> $OUTPUT_IO
mv zlib-$ZLIB_VERSION zlib
pushd zlib
./configure &> $OUTPUT_IO
make libz.a &> $OUTPUT_IO
# Manually install (requird libc for exe not available with mingw-w64):
mkdir -p $MINGW_DIR/include &> $OUTPUT_IO
mkdir -p $MINGW_DIR/lib &> $OUTPUT_IO
cp zconf.h zlib.h $MINGW_DIR/include &> $OUTPUT_IO
cp libz.a $MINGW_DIR/lib &> $OUTPUT_IO
popd

ls -la $MINGW_DIR/lib
