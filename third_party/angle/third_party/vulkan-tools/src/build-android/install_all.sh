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

set -e

#
# Parse parameters
#

function printUsage {
   echo "Supported parameters are:"
   echo "    -s|--serial <target device serial number> (optional)"
   echo
   echo "i.e. ${0##*/} -s <serial number>"
   exit 1
}

if [[ $(($# % 2)) -ne 0 ]]
then
    echo Parameters must be provided in pairs.
    echo parameter count = $#
    echo
    printUsage
    exit 1
fi

while [[ $# -gt 0 ]]
do
    case $1 in
        -s|--serial)
            # include the flag, because we need to leave it off if not provided
            serial="$2"
            shift 2
            ;;
        -*)
            # unknown option
            echo Unknown option: $1
            echo
            printUsage
            exit 1
            ;;
    esac
done

if [[ $serial ]]; then
    echo serial = "${serial}"
    serialFlag="-s $serial"
    if [[ $(adb devices) != *"$serial"* ]]
    then
        echo Device not found: "${serial}"
        echo
        printUsage
        exit 1
    fi
else
    echo Using device $(adb get-serialno)
fi

# Install everything built by build_all.sh
echo "adb $serialFlag install -r ../cube/android/cube/bin/vkcube.apk"
adb $serialFlag install -r ../cube/android/cube/bin/vkcube.apk

exit $?
