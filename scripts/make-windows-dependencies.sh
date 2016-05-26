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

pushd /tmp

# Build curl
wget http://curl.haxx.se/download/curl-$CURL_VERSION.tar.gz &> $OUTPUT_IO
tar xzf curl-$CURL_VERSION.tar.gz &> $OUTPUT_IO
pushd curl-$CURL_VERSION
./configure --host=$HOST_PREFIX --prefix=$MINGW_DIR --enable-static=yes --enable-shared=no --with-zlib=$MINGW_DIR --with-ssl=$MINGW_DIR &> $OUTPUT_IO
make &> $OUTPUT_IO
sudo PATH="$MINGW_DIR/bin:$PATH" make install &> $OUTPUT_IO
popd

# Build bzip
wget http://www.bzip.org/$BZIP_VERSION/bzip2-$BZIP_VERSION.tar.gz &> $OUTPUT_IO
tar xzf bzip2-$BZIP_VERSION.tar.gz &> $OUTPUT_IO
pushd bzip2-$BZIP_VERSION
$HOME/mingw make &> $OUTPUT_IO
sudo $HOME/mingw make install PREFIX=$MINGW_DIR &> $OUTPUT_IO
popd

# Install Python
wget https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/4.9.2/threads-posix/dwarf/i686-4.9.2-release-posix-dwarf-rt_v3-rev1.7z
7z x i686-4.9.2-release-posix-dwarf-rt_v3-rev1.7z -opython mingw32/opt/include/python2.7 mingw32/opt/bin/python* -r
sudo rsync -a python/mingw32/opt/include/python2.7 $MINGW_DIR/include
sudo rsync -a python/mingw32/opt/bin/ $MINGW_DIR/bin
# Python hack for getting correct python config vars for auto ools
sudo chmod +x $MINGW_DIR/bin/python-config.sh
sudo rm -f /usr/local/bin/python-config
sudo ln -s $MINGW_DIR/bin/python-config.sh /usr/local/bin/python-config

# Install Python mingw dll.a file for local python installation
# (the one from mingw toolchain requires other dependencies)
wget https://bitbucket.org/carlkl/mingw-w64-for-python/downloads/libpython-cp27-none-win32.7z
7z x libpython-cp27-none-win32.7z
sudo mv libs/libpython27.dll.a $MINGW_DIR/lib/libpython2.7.a

# TODO: Build Boost

popd

echo "BIN DIR:"
ls -la $MINGW_DIR/bin
echo "INCLUDE DIR:"
ls -la $MINGW_DIR/include
echo "LIB DIR:"
ls -la $MINGW_DIR/lib
echo "PYTHON CONFIG:"
which python-config
python-config --ldflags
$MINGW_DIR/bin/python-config.sh --ldflags
