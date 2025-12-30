#!/bin/sh

if [ ! -d "mp4v2-2.0.0" ]; then
    tar -xvf mp4v2-2.0.0.tar.gz
fi
cd mp4v2-2.0.0

./make.sh $*
