#!/bin/bash

$(eval /osxcross/tools/osxcross_conf.sh)

dir=$(realpath .)

$dir/scripts/lipo-dir.py  \
    $dir/addons/vst3-host/bin/osxcross-arm64/release \
    $dir/addons/vst3-host/bin/osxcross-x86_64/release \
    $dir/addons/vst3-host/bin/osxcross/release

prefix=$dir/addons/vst3-host/bin/macos/release
prefix_x64=$dir/addons/vst3-host/bin/osxcross-x86_64/release
prefix_arm64=$dir/addons/vst3-host/bin/osxcross-arm64/release

$dir/scripts/lipo-dir.py $prefix_arm64 $prefix_x64 $prefix

export OSXCROSS_ROOT=$OSXCROSS_BASE_DIR

cd $dir
scons platform=macos target=template_release osxcross_sdk=$OSXCROSS_TARGET
