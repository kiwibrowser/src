#!/bin/bash

# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Helper script for running unit tests for compatibility libraries

checkFile() {
    if [ ! -f "$1" ]; then
        echo "Unable to locate $1"
        exit
    fi;
}

# check if in Android build env
if [ ! -z ${ANDROID_BUILD_TOP} ]; then
    HOST=`uname`
    if [ "$HOST" == "Linux" ]; then
        OS="linux-x86"
    elif [ "$HOST" == "Darwin" ]; then
        OS="darwin-x86"
    else
        echo "Unrecognized OS"
        exit
    fi;
fi;

JAR_DIR=${ANDROID_HOST_OUT}/framework
TF_CONSOLE=com.android.tradefed.command.Console

############### Run the cts tests ###############
JARS="
    compatibility-host-util\
    cts-tradefed\
    ddmlib-prebuilt\
    hosttestlib\
    CtsDeqpTestCases\
    CtsDeqpRunnerTests\
    tradefed"
JAR_PATH=
for JAR in $JARS; do
    checkFile ${JAR_DIR}/${JAR}.jar
    JAR_PATH=${JAR_PATH}:${JAR_DIR}/${JAR}.jar
done

TEST_CLASSES="
    com.drawelements.deqp.runner.DeqpTestRunnerTest"

for CLASS in ${TEST_CLASSES}; do
    java $RDBG_FLAG -cp ${JAR_PATH} ${TF_CONSOLE} run singleCommand host -n --class ${CLASS} "$@"
done
