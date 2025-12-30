#!/bin/bash

set -e

libs_dir=$1
cd $libs_dir

NEEDED_LIB="libax_sys.so"

for file in ./libsns_*.so; do
    if [[ -f "$file" && "$file" != *dummy* ]]; then
        echo "Processing $file ..."

        # 检查是否已经有这个依赖，避免重复添加
        if readelf -d "$file" | grep -q "$NEEDED_LIB"; then
            echo "  Already depends on $NEEDED_LIB, skipping."
        else
            patchelf --add-needed "$NEEDED_LIB" "$file"
            echo "  Added $NEEDED_LIB to $file"
        fi
    fi
done


cd -

