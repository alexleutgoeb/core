#!/bin/sh
set -e

################################################################################
# check-osx.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to check if all dependencies are met for building DLVHEX on
# Mac OS X.
#
# Parameters:
#
# --install-dependencies
#
#   If set required dependencies are automatically installed via homebrew.
#
################################################################################


# List of required dependencies
deps="git autoconf automake libtool pkg-config wget scons bison re2c python"
# TODO: Bison is always found on Mac OS X, so we have to check for min version too!

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
  if ! which $dep &> /dev/null; then
    missing_deps="$missing_deps $dep"
  fi
done

if [ -n "$missing_deps" ]; then
  # Check if we should auto-install dependencies
  if [ "$install_deps" -eq 1 ]; then
    echo "Installing dependencies..."
    brew update
    brew install$missing_deps
  else
    echo "Error: Missing build dependencies, use:"
    echo "brew install$missing_deps"
    exit 1
  fi
else
  echo "Dependencies up to date"
fi
