#!/bin/sh
set -e

################################################################################
# check-windows.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to check if all dependencies are met for cross-building
# DLVHEX for Windows on Linux.
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
deps="sed git build-essential autoconf libtool wget pkg-config scons bison re2c p7zip-full python-dev libxml2-dev libxslt1-dev ccze mingw-w64"

# Used to temporarily save missing dependencies
missing_deps=""

# Check parameters
while [ -n "$1" ]; do
  case $1 in
    --install-dependencies )    install_deps=1
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
  if [ "$install_deps" -eq 1 ]; then
    # Uses sudo command, may result in a user prompt
    echo "Installing dependencies..."
    sudo apt-get update -qq
    sudo apt-get -y install$missing_deps
  else
    echo "Error: Missing build dependencies, use:"
    echo "  apt-get update && apt-get install$missing_deps"
    exit 1
  fi
else
  echo "Dependencies up to date"
fi

# Check for cross-compilation libs
if [ "$install_deps" -eq 1 ]; then
  echo "Building Windows libraries from source..."
  ./make-windows-dependencies.sh
else
  echo "Make sure to run make-windows-dependencies.sh before creating the build"
fi
