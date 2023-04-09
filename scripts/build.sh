#!/bin/bash

cd `dirname $0`/..
mkdir -p _build
cd _build
export FB_ROOT=fb_root/installed

cmake .. -GNinja -DFB_ROOT=${FB_ROOT} -DCMAKE_EXPORT_COMPILE_COMMANDS=on $@