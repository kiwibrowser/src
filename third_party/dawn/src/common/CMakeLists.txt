# Copyright 2020 The Dawn Authors
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

add_library(dawn_common STATIC ${DAWN_DUMMY_FILE})
target_sources(dawn_common PRIVATE
    "Assert.cpp"
    "Assert.h"
    "BitSetIterator.h"
    "Compiler.h"
    "Constants.h"
    "DynamicLib.cpp"
    "DynamicLib.h"
    "GPUInfo.cpp"
    "GPUInfo.h"
    "HashUtils.h"
    "LinkedList.h"
    "Log.cpp"
    "Log.h"
    "Math.cpp"
    "Math.h"
    "PlacementAllocated.h"
    "Platform.h"
    "RefCounted.cpp"
    "RefCounted.h"
    "Result.cpp"
    "Result.h"
    "Serial.h"
    "SerialMap.h"
    "SerialQueue.h"
    "SerialStorage.h"
    "SlabAllocator.cpp"
    "SlabAllocator.h"
    "StackContainer.h"
    "SwapChainUtils.h"
    "SystemUtils.cpp"
    "SystemUtils.h"
    "TypedInteger.h"
    "UnderlyingType.h"
    "ityp_array.h"
    "ityp_bitset.h"
    "ityp_span.h"
    "ityp_stack_vec.h"
    "ityp_vector.h"
    "vulkan_platform.h"
    "windows_with_undefs.h"
    "xlib_with_undefs.h"
)
target_link_libraries(dawn_common PRIVATE dawn_internal_config)

# TODO Android Log support
# TODO Vulkan headers support
