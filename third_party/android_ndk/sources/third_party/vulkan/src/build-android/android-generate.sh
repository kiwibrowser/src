#!/bin/bash

# Copyright 2015 The Android Open Source Project
# Copyright (C) 2015 Valve Corporation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

dir=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
cd $dir

rm -rf generated
mkdir -p generated/include generated/common

python ../vk-generate.py Android dispatch-table-ops layer > generated/include/vk_dispatch_table_helper.h

python ../vk_helper.py --gen_enum_string_helper ../include/vulkan/vulkan.h --abs_out_dir generated/include
python ../vk_helper.py --gen_struct_wrappers ../include/vulkan/vulkan.h --abs_out_dir generated/include

( cd generated/include; python ../../../lvl_genvk.py -registry ../../../vk.xml thread_check.h )
( cd generated/include; python ../../../lvl_genvk.py -registry ../../../vk.xml parameter_validation.h )
( cd generated/include; python ../../../lvl_genvk.py -registry ../../../vk.xml unique_objects_wrappers.h )

exit 0
