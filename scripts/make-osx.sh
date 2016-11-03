#!/bin/bash
set -e

################################################################################
# make-osx.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to build the DLVHEX binary for Mac OS X. Make sure to call
# check-osx.sh to check for missing dependencies before.
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

# Use bison from homebrew, not linked automatically
# TODO: Better solution for that?
brew link bison --force

# Change into root dir of repo
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Create build files
./bootstrap.sh &> $OUTPUT_IO

# Patching Makefile.am
sed -i "" "s/libdlvhex2_base_la_LIBADD/#libdlvhex2_base_la_LIBADD/" src/Makefile.am
# Reason: In Makefile: libdlvhex2-base.la may not include$(libdlvhex2_base_la_LIBADD),
# otherwise boost static libs are added to static lib and it won't link anymore!

# Configure and build
./configure LOCAL_PLUGIN_DIR=plugins --enable-python --enable-shared=no --enable-static-boost &> $OUTPUT_IO

make || :

echo "LA FILE:"
cat src/libdlvhex2-base.la

echo "LIB IS:"
file src/..libs/libdlvhex2-base.a
