#!/bin/sh
set -e

################################################################################
# check-linux.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to check if all dependencies are met for building DLVHEX on
# Ubuntu Linux.
#
# Parameters:
#
# --install-dependencies
#
#   If set required dependencies are automatically installed via apt-get. Please
#   note that the script may need root permissions and will prompt you for that.
#
################################################################################


# List of required dependencies
deps="sed git build-essential autoconf autotools-dev libtool wget scons bison re2c libboost-all-dev python-dev libpython-all-dev libcurl4-openssl-dev libbz2-dev"

# Used to temporarily save missing dependencies
missing_deps=""

# Check parameters
while [ -n "$1" ]; do
  case $1 in
    --install-dependencies )    install_deps=true
                                ;;
  esac
  shift
done

# Check for dependencies
for dep in `echo $deps`; do
  if ! dpkg -s $dep &> /dev/null; then
    missing_deps="$missing_deps $dep"
  fi
done

if [ -n "$missing_deps" ]; then
  # Check if we should auto-install dependencies
  if [ "$install_deps" -eq true ]; then
    # Uses sudo command, may result in a user prompt
    sudo apt-get update -qq
    sudo apt-get -y install$missing_deps
  else
    echo "Error: Missing build dependencies, use:"
    echo "apt-get update && apt-get install$missing_deps"
    exit 1
  fi
else
  echo "Dependencies up to date"
fi
