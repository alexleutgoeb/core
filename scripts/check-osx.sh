#!/bin/sh
set -e

################################################################################
# check-osx.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to check if all dependencies are met for building DLVHEX for
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
deps="git autoconf automake libtool pkg-config wget scons re2c python boost-python"

# Used to temporarily save missing dependencies
missing_deps=""
# Always install bison and libtool, as the system versions are outdated,
# probably check for minimum version in future?
missing_deps="$missing_deps bison libtool"
# Defines if dependencies should be auto-installed
install_deps=0

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
