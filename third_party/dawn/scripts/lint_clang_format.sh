#!/bin/bash

# Copyright 2018 The Dawn Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

clang_format=$1
base_commit=$2

echo
skipped_directories="(examples|generator|src/tests/(unittests|end2end)|third_party)"
# Find the files modified that need formatting
files_to_check=$(git diff --diff-filter=ACMR --name-only $base_commit | grep -E "*\.(c|cpp|mm|h)$" | grep -vE "^$skipped_directories/*")
if [ -z "$files_to_check" ]; then
    echo "No modified files to format."
    exit 0
fi
echo "Checking formatting diff on these files:"
echo "$files_to_check"
echo
files_to_check=$(echo $files_to_check | tr '\n' ' ')

# Run git-clang-format, check if it formatted anything
format_output=$(scripts/git-clang-format --binary $clang_format --commit $base_commit --diff --style=file $files_to_check)
if [ "$format_output" == "clang-format did not modify any files" ] || [ "$format_output" == "no modified files to format" ] ; then
    exit 0
fi

# clang-format made changes, print them and fail Travis
echo "Following formatting changes needed:"
echo
echo "$format_output"
echo
exit 1
