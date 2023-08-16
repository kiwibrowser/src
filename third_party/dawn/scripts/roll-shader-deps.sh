#!/usr/bin/env bash

# Copyright 2019 The Dawn Authors
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

# Attempts to roll all entries in DEPS to tip-of-tree and create a commit.
#
# Depends on roll-dep from depot_path being in PATH.

glslang_dir="third_party/glslang/"
glslang_trunk="origin/master"
shaderc_dir="third_party/shaderc/"
shaderc_trunk="origin/main"
spirv_cross_dir="third_party/spirv-cross/"
spirv_cross_trunk="origin/master"
spirv_headers_dir="third_party/spirv-headers/"
spirv_headers_trunk="origin/master"
spirv_tools_dir="third_party/SPIRV-Tools/"
spirv_tools_trunk="origin/master"
tint_dir="third_party/tint/"
tint_trunk="origin/main"

# This script assumes it's parent directory is the repo root.
repo_path=$(dirname "$0")/..

cd "$repo_path"

if [[ $(git diff --stat) != '' ]]; then
    echo "Working tree is dirty, commit changes before attempting to roll DEPS"
    exit 1
fi

old_head=$(git rev-parse HEAD)

roll-dep --ignore-dirty-tree --roll-to="${glslang_trunk}" "${glslang_dir}"
roll-dep --ignore-dirty-tree --roll-to="${shaderc_trunk}" "${shaderc_dir}"
roll-dep --ignore-dirty-tree --roll-to="${spirv_cross_trunk}" "${spirv_cross_dir}"
roll-dep --ignore-dirty-tree --roll-to="${spirv_headers_trunk}" "${spirv_headers_dir}"
roll-dep --ignore-dirty-tree --roll-to="${spirv_tools_trunk}" "${spirv_tools_dir}"
roll-dep --ignore-dirty-tree --roll-to="${tint_trunk}" "${tint_dir}"

git rebase --interactive "${old_head}"
