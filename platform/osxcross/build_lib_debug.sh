#!/bin/bash

$(eval /osxcross/tools/osxcross_conf.sh)

dir=$(realpath .)

$dir/scripts/lipo-dir.py  \
    $dir/addons/vst3-host/bin/osxcross-arm64/debug \
    $dir/addons/vst3-host/bin/osxcross-x86_64/debug \
    $dir/addons/vst3-host/bin/osxcross/debug

prefix=$dir/addons/vst3-host/bin/macos/debug
prefix_x64=$dir/addons/vst3-host/bin/osxcross-x86_64/debug
prefix_arm64=$dir/addons/vst3-host/bin/osxcross-arm64/debug

$dir/scripts/lipo-dir.py $prefix_arm64 $prefix_x64 $prefix

export OSXCROSS_ROOT=$OSXCROSS_BASE_DIR

cd $dir
scons platform=macos target=template_debug dev_build=yes debug_symbols=yes osxcross_sdk=$OSXCROSS_TARGET

x86_64-apple-${OSXCROSS_TARGET}-dsymutil addons/vst3-host/bin/macos/macos.framework/libvst3hostgodot.macos.template_debug
