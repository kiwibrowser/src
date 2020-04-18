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

#
# Parse parameters
#

function printUsage {
   echo "Supported parameters are:"
   echo "    -p|--platform <platform> (optional)"
   echo "    -f|--filter <gtest filter list> (optional)"
   echo "    -s|--serial <target device serial number> (optional)"
   echo
   echo "i.e. ${0##*/} -p <platform> -f <test filter> -s <serial number>"
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
        -p|--platform)
            platform="$2"
            shift 2
            ;;
        -f|--filter)
            filter="$2"
            shift 2
            ;;
        -s|--serial)
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

if [[ -z $platform ]]
then
    echo No platform specified.
    platform="UnspecifiedPlatform"
fi

if [[ -z $filter ]]
then
    echo No filter specified, running all tests.
    filter="*"
fi

if [[ $platform ]]; then echo platform = "${platform}"; fi
if [[ $filter ]]; then echo filter = "${filter}"; fi
if [[ $serial ]]; then echo serial = "${serial}"; fi

set -ev

#
# Start up
#

# Wake up the device
adb $serialFlag shell input keyevent "KEYCODE_MENU"
adb $serialFlag shell input keyevent "KEYCODE_HOME"

# Grab our Android test mutex
# Wait for any existing test runs on the devices

# Blow away the lock if tests run too long, avoiding infinite loop
lock_seconds=1200                                # Duration in seconds.
lock_end_time=$(( $(date +%s) + lock_seconds ))  # Calculate end time.

until mkdir /var/tmp/VkLayerValidationTests.$serial.lock
do
    sleep 5
    echo "Waiting for existing Android test to complete on $serial"

    if [ $(date +%s) -gt $lock_end_time ]
    then
        echo "Lock timeout reached: $lock_seconds seconds"
        echo "Deleting /var/tmp/VkLayerValidationTests.$serial.lock"
        rm -r /var/tmp/VkLayerValidationTests.$serial.lock
    fi
done

# Clean up our lock on any exit condition
function finish {
   rm -r /var/tmp/VkLayerValidationTests.$serial.lock
}
trap finish EXIT

# Clear the log
adb $serialFlag logcat -c

# Ensure any previous activity has stopped, otherwise it won't run tests
adb $serialFlag shell am force-stop com.example.VulkanLayerValidationTests

# Remove any existing APK that may have been installed from another host
# Disable exit on error in case the APK is not present
set +e
adb $serialFlag shell pm list packages | grep com.example.VulkanLayerValidationTests
if [ $? -eq 0 ]
then
    adb $serialFlag uninstall com.example.VulkanLayerValidationTests
fi
# Re-enable exit on error
set -e

# Install the current build
adb $serialFlag install -r bin/VulkanLayerValidationTests.apk

# Kick off the tests with known expection list
adb $serialFlag shell am start -a android.intent.action.MAIN -c android-intent.category.LAUNCH -n com.example.VulkanLayerValidationTests/android.app.NativeActivity --es args --gtest_filter="${filter}"

#
# Scrape the log until we get pass/fail/crash
#

# The following loop will give tests 20 minutes to pass/fail/crash
seconds=1200                          # Duration in seconds.
endTime=$(( $(date +%s) + seconds ))  # Calculate end time.

exitCode=-1;

# Disable exit on error, we expect grep to fail multiple times in this loop
set +e

while [ $(date +%s) -lt $endTime ]; do  # Loop until interval has elapsed.

    # The following line is printed from android_main on success
    adb $serialFlag logcat -d | grep "==== Tests PASSED ===="
    if [ $? -eq 0 ]
    then
        echo VulkanLayerValidationTests PASSED!
        exitCode=0
        break
    fi

    # The following line is printed from android_main on failure
    adb $serialFlag logcat -d | grep "==== Tests FAILED ===="
    if [ $? -eq 0 ]
    then
        echo VulkanLayerValidationTests FAILED!
        exitCode=1
        break
    fi

    # developer.android.com recommends searching for the following string to detect native crash
    adb $serialFlag logcat -d | grep "\*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\* \*\*\*"
    if [ $? -eq 0 ]
    then
        exitCode=2
        echo VulkanLayerValidationTests CRASHED!
        break
    fi

    sleep 5

done

# Re-enable exit on error
set -e

if [ $exitCode -eq -1 ]
then
    echo "VulkanLayerValidationTests hasn't completed in $seconds seconds. Script exiting."
fi

#
# Cleanup
#

# Return to home screen to clear any error pop-ups
adb $serialFlag shell input keyevent "KEYCODE_HOME"

# Stop the activity
adb $serialFlag shell am force-stop com.example.VulkanLayerValidationTests

today=$(date +%Y-%m-%d.%H:%M:%S)
outFile="VulkanLayerValidationTests.$platform.$today.out.txt"
errFile="VulkanLayerValidationTests.$platform.$today.err.txt"
adb $serialFlag pull /sdcard/Android/data/com.example.VulkanLayerValidationTests/files/out.txt VulkanLayerValidationTests.$platform.$today.out.txt
adb $serialFlag pull /sdcard/Android/data/com.example.VulkanLayerValidationTests/files/err.txt VulkanLayerValidationTests.$platform.$today.err.txt

if [ -f $outFile ]; then
    echo $outFile size $(wc -c < $outFile)
fi

if [ -f $errFile ]; then
    echo $errFile size $(wc -c < $errFile)
fi

echo
echo ===== Dumping logcat of VulkanLayerValidationTests =====
echo If the test is crashing, be sure to inspect full log for complete stack trace.
echo "adb $serialFlag logcat -d | grep VulkanLayerValidationTests"
echo ========================================================
echo
adb $serialFlag logcat -d | grep VulkanLayerValidationTests

exit $exitCode
