#!/bin/sh

APPS_DIR=~/apps/
NB_CORES=6
NDK=~/android/android-ndk-r21
ANDROID_API=28

PROTOBUF_RELEASE=3.13.0

# 1.: Download stable release
cd $APPS_DIR
#git clone https://github.com/protocolbuffers/protobuf.git
wget https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOBUF_RELEASE/protobuf-cpp-$PROTOBUF_RELEASE.tar.gz
tar zxf protobuf-cpp-$PROTOBUF_RELEASE.tar.gz
ln -s protobuf-$PROTOBUF_RELEASE/ protobuf


# 2.: build native version (on Linux x86_64)
cd $APPS_DIR/protobuf
mkdir -p release/x86_64
./autogen.sh
mkdir build
cd build
../configure --prefix=$APPS_DIR/protobuf/release/x86_64
make -j $NB_CORES
make install
#make distclean


#$ $NDK/build/tools/make_standalone_toolchain.py --arch arm64 --api 28 --stl=libc++ --install-dir=/home/bruel/apps/protobuf/build
#WARNING:__main__:make_standalone_toolchain.py is no longer necessary. The
#$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin directory contains target-specific scripts that perform
#the same task. For example, instead of:
#
#    $ python $NDK/build/tools/make_standalone_toolchain.py \
#        --arch arm64 --api 28 --install-dir toolchain
#    $ toolchain/bin/clang++ src.cpp
#
#Instead use:
#
#    $ $NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android28-clang++ src.cpp


# 3.: make aarch64-linux-android version
cd ..
mkdir -p release/arm64
rm -rf build
mkdir build
cd build
CC=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$ANDROID_API-clang  CXX=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$ANDROID_API-clang++   CFLAGS="-fPIE -fPIC" LDFLAGS="-pie -llog"   ../configure --host=aarch64-linux-android --prefix=$APPS_DIR/protobuf/release/arm64 --with-protoc=~/apps/protobuf/release/x86_64/bin/protoc
make -j $NB_CORES
make install
#make distclean


# 4.: armv7a-linux-androideabi
cd ..
mkdir -p release/armv7a
rm -rf build
mkdir build
cd build
CC=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi$ANDROID_API-clang  CXX=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi$ANDROID_API-clang++  CFLAGS="-fPIE -fPIC"  LDFLAGS="-pie -llog"  ../configure --host=armv7a-linux-androideabi --prefix=$APPS_DIR/protobuf/release/armv7a --with-protoc=~/apps/protobuf/release/x86_64/bin/protoc
make -j $NB_CORES
make install
make distclean
