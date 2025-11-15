#!/bin/bash

echo TAG_VERSION=$TAG_VERSION
echo BUILD_SHA=$BUILD_SHA

dir=$(realpath .)

# configure godot-vst3-host

build_dir=$dir/addons/vst3-host/bin/linux/debug

mkdir -p $build_dir
cd $build_dir

cmake -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_VERBOSE_MAKEFILE=1 \
    $dir
    #-DENABLE_ASAN=ON \

# build godot-vst3-host

#cd $dir/addons/vst3/bin/linux/debug
#make

# build godot-vst3-host (gdextension)

cd $dir
scons platform=linux target=template_debug dev_build=yes debug_symbols=yes #asan=true
