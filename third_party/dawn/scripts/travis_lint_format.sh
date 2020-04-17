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

if [ "$TRAVIS_PULL_REQUEST" == "false" ]; then
    echo "Running outside of pull request isn't supported yet"
    exit 0
fi

# Choose the commit against which to format
base_commit=$(git rev-parse $TRAVIS_BRANCH)
echo "Formatting against $TRAVIS_BRANCH a.k.a. $base_commit..."
echo

scripts/lint_clang_format.sh $1 $base_commit
