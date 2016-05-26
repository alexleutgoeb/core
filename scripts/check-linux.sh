#!/bin/bash
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
#   note that the script may need root permissions for that.
#
################################################################################


# List of required dependencies
deps="sed git build-essential autoconf autotools-dev libtool wget scons bison re2c libboost-all-dev python-dev libpython-all-dev libcurl4-openssl-dev libbz2-dev"

# Used to temporarily save missing dependencies
missing_deps=""

# Check parameters
while [[ -n "$1" ]]; do
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

if [[ -n "$missing_deps" ]]; then
  # Check if we should auto-install dependencies
  if [[ $install_deps -eq true ]]; then
    # Check for root permissions
    if [[ $EUID -ne 0 ]]; then
      echo "Error, can't install dependencies, please run as root user"
      exit 1
    else
      apt-get update -qq
      apt-get -y install$missing_deps
    fi
  else
    echo "Error: Missing build dependencies, use:"
    echo "apt-get update && apt-get install$missing_deps"
    exit 1
  fi
else
  echo "Dependencies up to date"
fi
