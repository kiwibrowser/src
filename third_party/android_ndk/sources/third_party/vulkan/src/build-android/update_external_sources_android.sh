#!/bin/bash
# Update source for glslang, spirv-tools, shaderc

# Copyright 2016 The Android Open Source Project
# Copyright (C) 2015 Valve Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

ANDROIDBUILDDIR=$PWD
BUILDDIR=$ANDROIDBUILDDIR/..
BASEDIR=$BUILDDIR/third_party
SHADERCTHIRDPARTY=$BASEDIR/shaderc/third_party

GLSLANG_REVISION=$(cat $ANDROIDBUILDDIR/glslang_revision_android)
SPIRV_TOOLS_REVISION=$(cat $ANDROIDBUILDDIR/spirv-tools_revision_android)
SPIRV_HEADERS_REVISION=$(cat $ANDROIDBUILDDIR/spirv-headers_revision_android)
SHADERC_REVISION=$(cat $ANDROIDBUILDDIR/shaderc_revision_android)

echo "GLSLANG_REVISION=$GLSLANG_REVISION"
echo "SPIRV_TOOLS_REVISION=$SPIRV_TOOLS_REVISION"
echo "SHADERC_REVISION=$SHADERC_REVISION"

function create_glslang () {
   rm -rf $SHADERCTHIRDPARTY/glslang
   echo "Creating local glslang repository ($SHADERCTHIRDPARTY/glslang)."
   mkdir -p $SHADERCTHIRDPARTY/glslang
   cd $SHADERCTHIRDPARTY/glslang
   git clone persistent-https://android.git.corp.google.com/platform/external/shaderc/glslang .
   git checkout $GLSLANG_REVISION
}

function update_glslang () {
   echo "Updating $SHADERCTHIRDPARTY/glslang"
   cd $SHADERCTHIRDPARTY/glslang
   git fetch --all
   git checkout $GLSLANG_REVISION
}

function create_spirv-tools () {
   rm -rf $SHADERCTHIRDPARTY/spirv-tools
   echo "Creating local spirv-tools repository ($SHADERCTHIRDPARTY/spirv-tools)."
   mkdir -p $SHADERCTHIRDPARTY/spirv-tools
   cd $SHADERCTHIRDPARTY/spirv-tools
   git clone persistent-https://android.git.corp.google.com/platform/external/shaderc/spirv-tools .
   git checkout $SPIRV_TOOLS_REVISION
}

function update_spirv-tools () {
   echo "Updating $SHADERCTHIRDPARTY/spirv-tools"
   cd $SHADERCTHIRDPARTY/spirv-tools
   git fetch --all
   git checkout $SPIRV_TOOLS_REVISION
}

function create_spirv-headers () {
   rm -rf $SHADERCTHIRDPARTY/spirv-tools/external/spirv-headers
   echo "Creating local spirv-headers repository ($SHADERCTHIRDPARTY/spirv-tools/external/spirv-headers)."
   mkdir -p $SHADERCTHIRDPARTY/spirv-tools/external/spirv-headers
   cd $SHADERCTHIRDPARTY/spirv-tools/external/spirv-headers
   git clone persistent-https://android.git.corp.google.com/platform/external/shaderc/spirv-headers .
   git checkout $SPIRV_HEADERS_REVISION
}

function update_spirv-headers () {
   echo "Updating $SHADERCTHIRDPARTY/spirv-tools/external/spirv-headers"
   cd $SHADERCTHIRDPARTY/spirv-tools/external/spirv-headers
   git fetch --all
   git checkout $SPIRV_HEADERS_REVISION
}

function create_shaderc () {
   rm -rf $BASEDIR/shaderc
   echo "Creating local shaderc repository ($BASEDIR/shaderc)."
   mkdir -p $BASEDIR
   cd $BASEDIR
   git clone persistent-https://android.git.corp.google.com/platform/external/shaderc/shaderc
   cd shaderc
   git checkout $SHADERC_REVISION
}

function update_shaderc () {
   echo "Updating $BASEDIR/shaderc"
   cd $BASEDIR/shaderc
   git fetch --all
   git checkout $SHADERC_REVISION
}

function build_shaderc () {
   echo "Building $BASEDIR/shaderc"
   cd $BASEDIR/shaderc/android_test
   ndk-build -j 4
}

# Must be first since it provides folder that hosts
# glslang and spirv-headers
if [ ! -d "$BASEDIR/shaderc" -o ! -d "$BASEDIR/shaderc/.git" ]; then
     create_shaderc
fi

update_shaderc
if [ ! -d "$BASEDIR/glslang" -o ! -d "$BASEDIR/glslang/.git" -o -d "$BASEDIR/glslang/.svn" ]; then
   create_glslang
fi
 update_glslang

if [ ! -d "$BASEDIR/spirv-tools" -o ! -d "$BASEDIR/spirv-tools/.git" ]; then
   create_spirv-tools
fi
update_spirv-tools

if [ ! -d "$BASEDIR/spirv-tools/external/spirv-headers" -o ! -d "$BASEDIR/spirv-tools/external/spirv-headers/.git" ]; then
   create_spirv-headers
fi
update_spirv-headers

build_shaderc

echo ""
echo "${0##*/} finished."

