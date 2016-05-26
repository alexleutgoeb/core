#!/bin/bash
set -e

################################################################################
# make-windows.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to cross-build the DLVHEX binary for Windows on Linux. Make
# sure to call check-windows.sh to check for missing dependencies before.
#
# Parameters:
#
# --verbose
#
#   Forward all build log output to stdout.
#
################################################################################


# Config vars
PYTHON_VERSION="2.7"

OUTPUT_IO=/dev/null
MINGW_DIR=/opt/mingw32
HOST_PREFIX=i686-w64-mingw32
export CC=$HOST_PREFIX-gcc
export CXX=$HOST_PREFIX-g++
export CPP=$HOST_PREFIX-cpp
export RANLIB=$HOST_PREFIX-ranlib
export PATH="$MINGW_DIR/bin:$PATH"

# Check parameters
while [ "$1" != "" ]; do
  case $1 in
    --verbose )     OUTPUT_IO=/dev/stdout
                    # set -v
                    ;;
  esac
  shift
done

# Set python version
export PYTHON_BIN=python$PYTHON_VERSION &> $OUTPUT_IO
export PYTHON_INCLUDE_DIR=$PYTHON_DIR/include/python2.7
export PYTHON_LIB=python2.7

# Environment
cat >$HOME/mingw << 'EOF'
#!/bin/sh
HOST_PREFIX=i686-w64-mingw32
MINGW_DIR=/opt/mingw32
export CC=$HOST_PREFIX-gcc
export CXX=$HOST_PREFIX-g++
export CPP=$HOST_PREFIX-cpp
export RANLIB=$HOST_PREFIX-ranlib
export PATH="$MINGW_DIR/bin:$PATH"
exec "$@"
EOF
chmod u+x $HOME/mingw

# Change into root dir of repo
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Create build files
./bootstrap.sh &> $OUTPUT_IO

# Patch the gringo patch for cross-compiling
sed -i "1s|^|62c62\n< env['CXX']            = 'g++'\n---\n> env['CXX']            = '$HOST_PREFIX-g++'\n68c68\n< env['LIBPATH']        = []\n---\n> env['LIBPATH']        = ['$MINGW_DIR/lib']\n|" buildclaspgringo/SConstruct.patch &> $OUTPUT_IO

# Configure and build
$HOME/mingw ./configure LOCAL_PLUGIN_DIR=plugins --enable-python --enable-static --disable-shared --enable-static-boost --host=$HOST_PREFIX LDFLAGS="-static -static-libgcc -static-libstdc++ -L$MINGW_DIR/lib" CFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" CXXFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" CPPFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" &> $OUTPUT_IO
# see http://curl.haxx.se/docs/faq.html#Link_errors_when_building_libcur about CURL_STATICLIB define
$HOME/mingw make &> $OUTPUT_IO
