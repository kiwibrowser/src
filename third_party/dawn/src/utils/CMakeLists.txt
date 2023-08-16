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

add_library(dawn_utils STATIC ${DAWN_DUMMY_FILE})
target_sources(dawn_utils PRIVATE
    "BackendBinding.cpp"
    "BackendBinding.h"
    "ComboRenderBundleEncoderDescriptor.cpp"
    "ComboRenderBundleEncoderDescriptor.h"
    "ComboRenderPipelineDescriptor.cpp"
    "ComboRenderPipelineDescriptor.h"
    "GLFWUtils.cpp"
    "GLFWUtils.h"
    "SystemUtils.cpp"
    "SystemUtils.h"
    "TerribleCommandBuffer.cpp"
    "TerribleCommandBuffer.h"
    "TextureFormatUtils.cpp"
    "TextureFormatUtils.h"
    "Timer.h"
    "WGPUHelpers.cpp"
    "WGPUHelpers.h"
)
target_link_libraries(dawn_utils
    PUBLIC dawncpp_headers
    PRIVATE dawn_internal_config
            dawn_common
            dawn_native
            dawn_wire
            shaderc
            glfw
)

if(WIN32)
    target_sources(dawn_utils PRIVATE "WindowsTimer.cpp")
elseif(APPLE)
    target_sources(dawn_utils PRIVATE
        "OSXTimer.cpp"
        "ObjCUtils.h"
        "ObjCUtils.mm"
    )
    target_link_libraries(dawn_utils PRIVATE "-framework QuartzCore")
elseif(UNIX)
    target_sources(dawn_utils PRIVATE "PosixTimer.cpp")
endif()

if (DAWN_ENABLE_D3D12)
    target_sources(dawn_utils PRIVATE "D3D12Binding.cpp")
endif()

if (DAWN_ENABLE_METAL)
    target_sources(dawn_utils PRIVATE
        "GLFWUtils_metal.mm"
        "MetalBinding.mm"
    )
    target_link_libraries(dawn_utils PRIVATE "-framework Metal")
endif()

if (DAWN_ENABLE_NULL)
    target_sources(dawn_utils PRIVATE "NullBinding.cpp")
endif()

if (DAWN_ENABLE_OPENGL)
    target_sources(dawn_utils PRIVATE "OpenGLBinding.cpp")
endif()

if (DAWN_ENABLE_VULKAN)
    target_sources(dawn_utils PRIVATE "VulkanBinding.cpp")
endif()
