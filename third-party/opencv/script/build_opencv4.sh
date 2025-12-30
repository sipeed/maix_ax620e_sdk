#!/bin/bash

# Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
#
# This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
# may not be copied or distributed in any isomorphic form without the prior
# written consent of Axera Semiconductor (Ningbo) Co., Ltd.
#
# Author: wanglusheng@axera-tech.com
#

# OpenCV home page: https://github.com/opencv/opencv

# set version
version=4.6.0

# save working dir
work_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"  && pwd)"

# download
wget -c https://github.com/opencv/opencv/archive/refs/tags/${version}.tar.gz -O opencv-${version}.tar.gz
tar xzvf opencv-${version}.tar.gz

# param: $1 src, $2 dst, $3 arch, $4 gcc, $5 g++
function build_opencv() {
    cmake -S "$1"                                           \
          -B "$2"                                           \
          -DCMAKE_SYSTEM_NAME=Linux                         \
          -DCMAKE_SYSTEM_PROCESSOR="$3"                     \
          -DCMAKE_MAKE_PROGRAM="$(which make)"              \
          -DCMAKE_C_COMPILER="$4"                           \
          -DCMAKE_CXX_COMPILER="$5"                         \
          -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER         \
          -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY          \
          -DBUILD_JPEG:BOOL=ON                              \
          -DBUILD_JASPER:BOOL=ON                            \
          -DBUILD_PNG:BOOL=ON                               \
          -DBUILD_TIFF:BOOL=ON                              \
          -DBUILD_WEBP:BOOL=ON                              \
          -DBUILD_ZLIB:BOOL=ON                              \
          -DBUILD_DOCS:BOOL=OFF                             \
          -DBUILD_EXAMPLES:BOOL=OFF                         \
          -DBUILD_IPP_IW:BOOL=OFF                           \
          -DBUILD_ITT:BOOL=OFF                              \
          -DBUILD_JAVA=OFF                                  \
          -DBUILD_PERF_TESTS:BOOL=OFF                       \
          -DBUILD_PROTOBUF:BOOL=OFF                         \
          -DBUILD_SHARED_LIBS:BOOL=OFF                      \
          -DBUILD_TESTS:BOOL=OFF                            \
          -DBUILD_opencv_apps:BOOL=OFF                      \
          -DBUILD_opencv_calib3d:BOOL=OFF                   \
          -DBUILD_opencv_dnn:BOOL=OFF                       \
          -DBUILD_opencv_features2d:BOOL=OFF                \
          -DBUILD_opencv_flann:BOOL=OFF                     \
          -DBUILD_opencv_gapi:BOOL=OFF                      \
          -DBUILD_opencv_highgui:BOOL=OFF                   \
          -DBUILD_opencv_java_bindings_generator:BOOL=OFF   \
          -DBUILD_opencv_js:BOOL=OFF                        \
          -DBUILD_opencv_js_bindings_generator:BOOL=OFF     \
          -DBUILD_opencv_ml:BOOL=OFF                        \
          -DBUILD_opencv_objc_bindings_generator:BOOL=OFF   \
          -DBUILD_opencv_objdetect:BOOL=OFF                 \
          -DBUILD_opencv_photo:BOOL=OFF                     \
          -DBUILD_opencv_python_bindings_generator:BOOL=OFF \
          -DBUILD_opencv_python_tests:BOOL=OFF              \
          -DBUILD_opencv_stitching:BOOL=OFF                 \
          -DBUILD_opencv_ts:BOOL=OFF                        \
          -DBUILD_opencv_video:BOOL=OFF                     \
          -DBUILD_opencv_videoio:BOOL=OFF                   \
          -DOPENCV_DNN_CUDA:BOOL=OFF                        \
          -DOPENCV_DNN_OPENCL:BOOL=OFF                      \
          -DWITH_CAROTENE:BOOL=OFF                          \
          -DWITH_ITT:BOOL=OFF                               \
          -DWITH_1394:BOOL=OFF                              \
          -DWITH_ADE:BOOL=OFF                               \
          -DWITH_ARITH_DEC:BOOL=OFF                         \
          -DWITH_ARITH_ENC:BOOL=OFF                         \
          -DWITH_EIGEN:BOOL=OFF                             \
          -DWITH_FFMPEG:BOOL=OFF                            \
          -DWITH_GSTREAMER:BOOL=OFF                         \
          -DWITH_GTK:BOOL=OFF                               \
          -DWITH_IPP:BOOL=OFF                               \
          -DWITH_OPENCL:BOOL=OFF                            \
          -DWITH_OPENCLAMDBLAS:BOOL=OFF                     \
          -DWITH_OPENCLAMDFFT:BOOL=OFF                      \
          -DWITH_PROTOBUF:BOOL=OFF                          \
          -DWITH_QUIRC:BOOL=OFF                             \
          -DWITH_V4L:BOOL=OFF                               \
          -DWITH_VA:BOOL=OFF                                \
          -DWITH_VA_INTEL:BOOL=OFF                          \
          -DWITH_VTK:BOOL=OFF                               \
          -DCMAKE_BUILD_TYPE=RELEASE                        \
          -DCMAKE_INSTALL_PREFIX="$2/install"
    cmake --build "$2" --parallel "$(nproc)" && cmake --install "$2"
}


# set src
src="${work_root}/opencv-${version}"

# build armv7hf
dst=build-opencv-${version}-armv7hf-glibc
mkdir "$dst" && build_opencv "$src" "$dst" arm arm-linux-gnueabihf-gcc arm-linux-gnueabihf-g++

# build armv7hf
dst=build-opencv-${version}-armv7hf-uclibc
mkdir "$dst" && build_opencv "$src" "$dst" arm arm-AX620E-linux-uclibcgnueabihf-gcc arm-AX620E-linux-uclibcgnueabihf-g++

# build armv8
dst=build-opencv-${version}-armv8
mkdir "$dst" && build_opencv "$src" "$dst" aarch64 aarch64-none-linux-gnu-gcc aarch64-none-linux-gnu-g++
