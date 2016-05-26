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
CURL_VERSION="7.46.0"
BZIP_VERSION="1.0.6"
BOOST_VERSION="1.57.0" # 1.59.0 is not working for cc
PYTHON_VERSION="2.7"

OUTPUT_IO=/dev/null

# Check parameters
while [ "$1" != "" ]; do
  case $1 in
    --verbose )     OUTPUT_IO=/dev/stdout
                    set -v
                    ;;
  esac
  shift
done

# Environment
PREFIX=x86_64-w64-mingw32
MINGW_DIR=/opt/mingw64
export CC=$PREFIX-gcc
export CXX=$PREFIX-g++
export CPP=$PREFIX-cpp
export RANLIB=$PREFIX-ranlib
export PATH="$MINGW_DIR/bin:$PATH"

pushd /tmp

# Build curl
wget http://curl.haxx.se/download/curl-$CURL_VERSION.tar.gz &> $OUTPUT_IO
tar xzf curl-$CURL_VERSION.tar.gz &> $OUTPUT_IO
pushd curl-$CURL_VERSION
./configure --host=$PREFIX --prefix=$MINGW_DIR --enable-static=yes --enable-shared=no --with-zlib=$MINGW_DIR --with-ssl=$MINGW_DIR &> $OUTPUT_IO
make &> $OUTPUT_IO
sudo PATH="$MINGW_DIR/bin:$PATH" make install &> $OUTPUT_IO
popd

# TODO: Build bzip, python and boost

popd

ls -la $MINGW_DIR/lib
