#!/bin/sh

meson builddir/ --prefix=$(pwd)/out --cross-file=./cross_file \
	  -D intel=false \
	  -D radeon=false \
	  -D amdgpu=false \
	  -D nouveau=false \
	  -D vmwgfx=false \
	  -D omap=false \
	  -D exynos=false \
	  -D freedreno=false \
	  -D tegra=false \
	  -D vc4=false \
	  -D etnaviv=false \
	  -D cairo-tests=false \
	  -D man-pages=true \
	  -D valgrind=false \
	  -D freedreno-kgsl=false \
	  -D install-test-programs=true \
	  -D udev=false

ninja -C builddir/ install
