#!/bin/bash

ROOT_PATH=`pwd`
BUILD_PATH=$ROOT_PATH/mc20e_build
TARGET_PATH=$BUILD_PATH/install

cd mc20e_build
cmake .. -DCMAKE_INSTALL_PREFIX=$TARGET_PATH \
         -DCMAKE_BUILD_TYPE=MINSIZEREL \
         -DCMAKE_TOOLCHAIN_FILE=$ROOT_PATH/mc20e.toolchain.cmake \
         -DBUILD_SHARED_LIBS=OFF \
         -DTINYALSA_BUILD_UTILS=ON


make -j 3
make install