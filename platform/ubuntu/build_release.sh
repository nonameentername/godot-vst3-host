#!/bin/bash

echo TAG_VERSION=$TAG_VERSION
echo BUILD_SHA=$BUILD_SHA

dir=$(realpath .)

# configure godot-vst3-host

build_dir=$dir/addons/vst3-host/bin/linux/release

mkdir -p $build_dir
cd $build_dir

cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_VERBOSE_MAKEFILE=1 \
    $dir

# build godot-vst3-host

#cd $dir/addons/vst3/bin/linux/release
#make

# build godot-vst3-host (gdextension)

cd $dir
scons platform=linux target=template_release
