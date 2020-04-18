#!/bin/bash

# Copyright 2017 The Android Open Source Project
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

if [ -z "${ANDROID_SDK_HOME}" ];
then echo "Please set ANDROID_SDK_HOME, exiting"; exit 1;
else echo "ANDROID_SDK_HOME is ${ANDROID_SDK_HOME}";
fi

if [ -z "${ANDROID_NDK_HOME}" ];
then echo "Please set ANDROID_NDK_HOME, exiting"; exit 1;
else echo "ANDROID_NDK_HOME is ${ANDROID_NDK_HOME}";
fi

if [[ $(uname) == "Linux" ]]; then
    cores=$(nproc) || echo 4
elif [[ $(uname) == "Darwin" ]]; then
    cores=$(sysctl -n hw.ncpu) || echo 4
fi

function findtool() {
    if [[ ! $(type -t $1) ]]; then
        echo Command $1 not found, see ../BUILD.md;
        exit 1;
    fi
}

# Check for dependencies
findtool aapt
findtool zipalign
findtool jarsigner

set -ev

DEMO_BUILD_DIR=$PWD/../cube/android
echo DEMO_BUILD_DIR="${DEMO_BUILD_DIR}"

function create_APK() {
    aapt package -f -M AndroidManifest.xml -I "$ANDROID_SDK_HOME/platforms/android-23/android.jar" -S res -F bin/$1-unaligned.apk bin/libs
    # update this logic to detect if key is already there.  If so, use it, otherwise create it.
    jarsigner -verbose -keystore ~/.android/debug.keystore -storepass android -keypass android  bin/$1-unaligned.apk androiddebugkey
    zipalign -f 4 bin/$1-unaligned.apk bin/$1.apk
}

./update_external_sources_android.sh --no-build

#
# build vkcube APK
#
(
pushd $DEMO_BUILD_DIR
ndk-build -j $cores
mkdir -p $DEMO_BUILD_DIR/cube/bin/libs/lib
cp -r $DEMO_BUILD_DIR/libs/* $DEMO_BUILD_DIR/cube/bin/libs/lib/
cd $DEMO_BUILD_DIR/cube
create_APK vkcube
popd
)

echo Builds succeeded
exit 0
