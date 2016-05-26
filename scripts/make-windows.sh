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

# Change into root dir of repo
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..
