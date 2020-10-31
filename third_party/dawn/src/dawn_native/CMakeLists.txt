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

DawnJSONGenerator(
    TARGET "dawn_native_utils"
    PRINT_NAME "Dawn native utilities"
    RESULT_VARIABLE "DAWN_NATIVE_UTILS_GEN_SOURCES"
)

add_library(dawn_native STATIC ${DAWN_DUMMY_FILE})
target_sources(dawn_native PRIVATE
    "${DAWN_INCLUDE_DIR}/dawn_native/DawnNative.h"
    "${DAWN_INCLUDE_DIR}/dawn_native/dawn_native_export.h"
    ${DAWN_NATIVE_UTILS_GEN_SOURCES}
    "Adapter.cpp"
    "Adapter.h"
    "AttachmentState.cpp"
    "AttachmentState.h"
    "BackendConnection.cpp"
    "BackendConnection.h"
    "BindGroup.cpp"
    "BindGroup.h"
    "BindGroupAndStorageBarrierTracker.h"
    "BindGroupLayout.cpp"
    "BindGroupLayout.h"
    "BindGroupTracker.h"
    "BindingInfo.cpp"
    "BindingInfo.h"
    "BuddyAllocator.cpp"
    "BuddyAllocator.h"
    "BuddyMemoryAllocator.cpp"
    "BuddyMemoryAllocator.h"
    "Buffer.cpp"
    "Buffer.h"
    "CachedObject.cpp"
    "CachedObject.h"
    "CommandAllocator.cpp"
    "CommandAllocator.h"
    "CommandBuffer.cpp"
    "CommandBuffer.h"
    "CommandBufferStateTracker.cpp"
    "CommandBufferStateTracker.h"
    "CommandEncoder.cpp"
    "CommandEncoder.h"
    "CommandValidation.cpp"
    "CommandValidation.h"
    "Commands.cpp"
    "Commands.h"
    "ComputePassEncoder.cpp"
    "ComputePassEncoder.h"
    "ComputePipeline.cpp"
    "ComputePipeline.h"
    "Device.cpp"
    "Device.h"
    "DynamicUploader.cpp"
    "DynamicUploader.h"
    "EncodingContext.cpp"
    "EncodingContext.h"
    "Error.cpp"
    "Error.h"
    "ErrorData.cpp"
    "ErrorData.h"
    "ErrorInjector.cpp"
    "ErrorInjector.h"
    "ErrorScope.cpp"
    "ErrorScope.h"
    "ErrorScopeTracker.cpp"
    "ErrorScopeTracker.h"
    "Extensions.cpp"
    "Extensions.h"
    "Fence.cpp"
    "Fence.h"
    "FenceSignalTracker.cpp"
    "FenceSignalTracker.h"
    "Format.cpp"
    "Format.h"
    "Forward.h"
    "Instance.cpp"
    "Instance.h"
    "MapRequestTracker.cpp"
    "MapRequestTracker.h"
    "ObjectBase.cpp"
    "ObjectBase.h"
    "PassResourceUsage.h"
    "PassResourceUsageTracker.cpp"
    "PassResourceUsageTracker.h"
    "PerStage.cpp"
    "PerStage.h"
    "Pipeline.cpp"
    "Pipeline.h"
    "PipelineLayout.cpp"
    "PipelineLayout.h"
    "ProgrammablePassEncoder.cpp"
    "ProgrammablePassEncoder.h"
    "QuerySet.cpp"
    "QuerySet.h"
    "Queue.cpp"
    "Queue.h"
    "RenderBundle.cpp"
    "RenderBundle.h"
    "RenderBundleEncoder.cpp"
    "RenderBundleEncoder.h"
    "RenderEncoderBase.cpp"
    "RenderEncoderBase.h"
    "RenderPassEncoder.cpp"
    "RenderPassEncoder.h"
    "RenderPipeline.cpp"
    "RenderPipeline.h"
    "ResourceHeap.h"
    "ResourceHeapAllocator.h"
    "ResourceMemoryAllocation.cpp"
    "ResourceMemoryAllocation.h"
    "RingBufferAllocator.cpp"
    "RingBufferAllocator.h"
    "Sampler.cpp"
    "Sampler.h"
    "ShaderModule.cpp"
    "ShaderModule.h"
    "StagingBuffer.cpp"
    "StagingBuffer.h"
    "Surface.cpp"
    "Surface.h"
    "SwapChain.cpp"
    "SwapChain.h"
    "Texture.cpp"
    "Texture.h"
    "ToBackend.h"
    "Toggles.cpp"
    "Toggles.h"
    "dawn_platform.h"
)
target_link_libraries(dawn_native
    PUBLIC dawncpp_headers
    PRIVATE dawn_common
            dawn_platform
            dawn_internal_config
            shaderc_spvc
            spirv-cross-core
)

if (DAWN_USE_X11)
    find_package(X11 REQUIRED)
    target_link_libraries(dawn_native PRIVATE ${X11_LIBRARIES})
endif()

if (WIN32)
    target_link_libraries(dawn_native PRIVATE user32.lib)
endif()

if (DAWN_ENABLE_D3D12)
    target_sources(dawn_native PRIVATE
        "${DAWN_INCLUDE_DIR}/dawn_native/D3D12Backend.h"
        "d3d12/AdapterD3D12.cpp"
        "d3d12/AdapterD3D12.h"
        "d3d12/BackendD3D12.cpp"
        "d3d12/BackendD3D12.h"
        "d3d12/BindGroupD3D12.cpp"
        "d3d12/BindGroupD3D12.h"
        "d3d12/BindGroupLayoutD3D12.cpp"
        "d3d12/BindGroupLayoutD3D12.h"
        "d3d12/BufferD3D12.cpp"
        "d3d12/BufferD3D12.h"
        "d3d12/CPUDescriptorHeapAllocationD3D12.cpp"
        "d3d12/CPUDescriptorHeapAllocationD3D12.h"
        "d3d12/CommandAllocatorManager.cpp"
        "d3d12/CommandAllocatorManager.h"
        "d3d12/CommandBufferD3D12.cpp"
        "d3d12/CommandBufferD3D12.h"
        "d3d12/CommandRecordingContext.cpp"
        "d3d12/CommandRecordingContext.h"
        "d3d12/ComputePipelineD3D12.cpp"
        "d3d12/ComputePipelineD3D12.h"
        "d3d12/D3D12Error.cpp"
        "d3d12/D3D12Error.h"
        "d3d12/D3D12Info.cpp"
        "d3d12/D3D12Info.h"
        "d3d12/DeviceD3D12.cpp"
        "d3d12/DeviceD3D12.h"
        "d3d12/Forward.h"
        "d3d12/GPUDescriptorHeapAllocationD3D12.cpp"
        "d3d12/GPUDescriptorHeapAllocationD3D12.h"
        "d3d12/HeapAllocatorD3D12.cpp"
        "d3d12/HeapAllocatorD3D12.h"
        "d3d12/HeapD3D12.cpp"
        "d3d12/HeapD3D12.h"
        "d3d12/NativeSwapChainImplD3D12.cpp"
        "d3d12/NativeSwapChainImplD3D12.h"
        "d3d12/PageableD3D12.cpp"
        "d3d12/PageableD3D12.h"
        "d3d12/PipelineLayoutD3D12.cpp"
        "d3d12/PipelineLayoutD3D12.h"
        "d3d12/PlatformFunctions.cpp"
        "d3d12/PlatformFunctions.h"
        "d3d12/QuerySetD3D12.cpp"
        "d3d12/QuerySetD3D12.h"
        "d3d12/QueueD3D12.cpp"
        "d3d12/QueueD3D12.h"
        "d3d12/RenderPassBuilderD3D12.cpp"
        "d3d12/RenderPassBuilderD3D12.h"
        "d3d12/RenderPipelineD3D12.cpp"
        "d3d12/RenderPipelineD3D12.h"
        "d3d12/ResidencyManagerD3D12.cpp"
        "d3d12/ResidencyManagerD3D12.h"
        "d3d12/ResourceAllocatorManagerD3D12.cpp"
        "d3d12/ResourceAllocatorManagerD3D12.h"
        "d3d12/ResourceHeapAllocationD3D12.cpp"
        "d3d12/ResourceHeapAllocationD3D12.h"
        "d3d12/SamplerD3D12.cpp"
        "d3d12/SamplerD3D12.h"
        "d3d12/SamplerHeapCacheD3D12.cpp"
        "d3d12/SamplerHeapCacheD3D12.h"
        "d3d12/ShaderModuleD3D12.cpp"
        "d3d12/ShaderModuleD3D12.h"
        "d3d12/ShaderVisibleDescriptorAllocatorD3D12.cpp"
        "d3d12/ShaderVisibleDescriptorAllocatorD3D12.h"
        "d3d12/StagingBufferD3D12.cpp"
        "d3d12/StagingBufferD3D12.h"
        "d3d12/StagingDescriptorAllocatorD3D12.cpp"
        "d3d12/StagingDescriptorAllocatorD3D12.h"
        "d3d12/SwapChainD3D12.cpp"
        "d3d12/SwapChainD3D12.h"
        "d3d12/TextureCopySplitter.cpp"
        "d3d12/TextureCopySplitter.h"
        "d3d12/TextureD3D12.cpp"
        "d3d12/TextureD3D12.h"
        "d3d12/UtilsD3D12.cpp"
        "d3d12/UtilsD3D12.h"
        "d3d12/d3d12_platform.h"
    )
    target_link_libraries(dawn_native PRIVATE dxguid.lib)
endif()

if (DAWN_ENABLE_METAL)
    target_sources(dawn_native PRIVATE
        "${DAWN_INCLUDE_DIR}/dawn_native/MetalBackend.h"
        "Surface_metal.mm"
        "metal/BackendMTL.h"
        "metal/BackendMTL.mm"
        "metal/BindGroupLayoutMTL.h"
        "metal/BindGroupLayoutMTL.mm"
        "metal/BindGroupMTL.h"
        "metal/BindGroupMTL.mm"
        "metal/BufferMTL.h"
        "metal/BufferMTL.mm"
        "metal/CommandBufferMTL.h"
        "metal/CommandBufferMTL.mm"
        "metal/CommandRecordingContext.h"
        "metal/CommandRecordingContext.mm"
        "metal/ComputePipelineMTL.h"
        "metal/ComputePipelineMTL.mm"
        "metal/DeviceMTL.h"
        "metal/DeviceMTL.mm"
        "metal/Forward.h"
        "metal/PipelineLayoutMTL.h"
        "metal/PipelineLayoutMTL.mm"
        "metal/QueueMTL.h"
        "metal/QueueMTL.mm"
        "metal/RenderPipelineMTL.h"
        "metal/RenderPipelineMTL.mm"
        "metal/SamplerMTL.h"
        "metal/SamplerMTL.mm"
        "metal/ShaderModuleMTL.h"
        "metal/ShaderModuleMTL.mm"
        "metal/StagingBufferMTL.h"
        "metal/StagingBufferMTL.mm"
        "metal/SwapChainMTL.h"
        "metal/SwapChainMTL.mm"
        "metal/TextureMTL.h"
        "metal/TextureMTL.mm"
        "metal/UtilsMetal.h"
        "metal/UtilsMetal.mm"
    )
    target_link_libraries(dawn_native PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework IOSurface"
        "-framework QuartzCore"
    )
endif()

if (DAWN_ENABLE_NULL)
    target_sources(dawn_native PRIVATE
        "${DAWN_INCLUDE_DIR}/dawn_native/NullBackend.h"
        "null/DeviceNull.cpp"
        "null/DeviceNull.h"
    )
endif()

if (DAWN_ENABLE_OPENGL)
    DawnGenerator(
        SCRIPT "${Dawn_SOURCE_DIR}/generator/opengl_loader_generator.py"
        PRINT_NAME "OpenGL function loader"
        ARGS "--gl-xml"
             "${Dawn_SOURCE_DIR}/third_party/khronos/gl.xml"
             "--supported-extensions"
             "${Dawn_SOURCE_DIR}/src/dawn_native/opengl/supported_extensions.json"
        RESULT_VARIABLE "DAWN_NATIVE_OPENGL_AUTOGEN_SOURCES"
    )

    target_sources(dawn_native PRIVATE
        "${DAWN_INCLUDE_DIR}/dawn_native/OpenGLBackend.h"
        ${DAWN_NATIVE_OPENGL_AUTOGEN_SOURCES}
        "opengl/BackendGL.cpp"
        "opengl/BackendGL.h"
        "opengl/BindGroupGL.cpp"
        "opengl/BindGroupGL.h"
        "opengl/BindGroupLayoutGL.cpp"
        "opengl/BindGroupLayoutGL.h"
        "opengl/BufferGL.cpp"
        "opengl/BufferGL.h"
        "opengl/CommandBufferGL.cpp"
        "opengl/CommandBufferGL.h"
        "opengl/ComputePipelineGL.cpp"
        "opengl/ComputePipelineGL.h"
        "opengl/DeviceGL.cpp"
        "opengl/DeviceGL.h"
        "opengl/Forward.h"
        "opengl/GLFormat.cpp"
        "opengl/GLFormat.h"
        "opengl/NativeSwapChainImplGL.cpp"
        "opengl/NativeSwapChainImplGL.h"
        "opengl/OpenGLFunctions.cpp"
        "opengl/OpenGLFunctions.h"
        "opengl/PersistentPipelineStateGL.cpp"
        "opengl/PersistentPipelineStateGL.h"
        "opengl/PipelineGL.cpp"
        "opengl/PipelineGL.h"
        "opengl/PipelineLayoutGL.cpp"
        "opengl/PipelineLayoutGL.h"
        "opengl/QuerySetGL.cpp"
        "opengl/QuerySetGL.h"
        "opengl/QueueGL.cpp"
        "opengl/QueueGL.h"
        "opengl/RenderPipelineGL.cpp"
        "opengl/RenderPipelineGL.h"
        "opengl/SamplerGL.cpp"
        "opengl/SamplerGL.h"
        "opengl/ShaderModuleGL.cpp"
        "opengl/ShaderModuleGL.h"
        "opengl/SwapChainGL.cpp"
        "opengl/SwapChainGL.h"
        "opengl/TextureGL.cpp"
        "opengl/TextureGL.h"
        "opengl/UtilsGL.cpp"
        "opengl/UtilsGL.h"
        "opengl/opengl_platform.h"
    )

    target_link_libraries(dawn_native PRIVATE dawn_khronos_platform)
endif()

if (DAWN_ENABLE_VULKAN)
    target_sources(dawn_native PRIVATE
        "${DAWN_INCLUDE_DIR}/dawn_native/VulkanBackend.h"
        "vulkan/AdapterVk.cpp"
        "vulkan/AdapterVk.h"
        "vulkan/BackendVk.cpp"
        "vulkan/BackendVk.h"
        "vulkan/BindGroupLayoutVk.cpp"
        "vulkan/BindGroupLayoutVk.h"
        "vulkan/BindGroupVk.cpp"
        "vulkan/BindGroupVk.h"
        "vulkan/BufferVk.cpp"
        "vulkan/BufferVk.h"
        "vulkan/CommandBufferVk.cpp"
        "vulkan/CommandBufferVk.h"
        "vulkan/CommandRecordingContext.h"
        "vulkan/ComputePipelineVk.cpp"
        "vulkan/ComputePipelineVk.h"
        "vulkan/DescriptorSetAllocation.h"
        "vulkan/DescriptorSetAllocator.cpp"
        "vulkan/DescriptorSetAllocator.h"
        "vulkan/DeviceVk.cpp"
        "vulkan/DeviceVk.h"
        "vulkan/ExternalHandle.h"
        "vulkan/FencedDeleter.cpp"
        "vulkan/FencedDeleter.h"
        "vulkan/Forward.h"
        "vulkan/NativeSwapChainImplVk.cpp"
        "vulkan/NativeSwapChainImplVk.h"
        "vulkan/PipelineLayoutVk.cpp"
        "vulkan/PipelineLayoutVk.h"
        "vulkan/QueueVk.cpp"
        "vulkan/QueueVk.h"
        "vulkan/RenderPassCache.cpp"
        "vulkan/RenderPassCache.h"
        "vulkan/RenderPipelineVk.cpp"
        "vulkan/RenderPipelineVk.h"
        "vulkan/ResourceHeapVk.cpp"
        "vulkan/ResourceHeapVk.h"
        "vulkan/ResourceMemoryAllocatorVk.cpp"
        "vulkan/ResourceMemoryAllocatorVk.h"
        "vulkan/SamplerVk.cpp"
        "vulkan/SamplerVk.h"
        "vulkan/ShaderModuleVk.cpp"
        "vulkan/ShaderModuleVk.h"
        "vulkan/StagingBufferVk.cpp"
        "vulkan/StagingBufferVk.h"
        "vulkan/SwapChainVk.cpp"
        "vulkan/SwapChainVk.h"
        "vulkan/TextureVk.cpp"
        "vulkan/TextureVk.h"
        "vulkan/UtilsVulkan.cpp"
        "vulkan/UtilsVulkan.h"
        "vulkan/VulkanError.cpp"
        "vulkan/VulkanError.h"
        "vulkan/VulkanExtensions.cpp"
        "vulkan/VulkanExtensions.h"
        "vulkan/VulkanFunctions.cpp"
        "vulkan/VulkanFunctions.h"
        "vulkan/VulkanInfo.cpp"
        "vulkan/VulkanInfo.h"
        "vulkan/external_memory/MemoryService.h"
        "vulkan/external_semaphore/SemaphoreService.h"
    )

    target_link_libraries(dawn_native PUBLIC dawn_vulkan_headers)

    if (UNIX AND NOT APPLE)
        target_sources(dawn_native PRIVATE
            "vulkan/external_memory/MemoryServiceOpaqueFD.cpp"
            "vulkan/external_semaphore/SemaphoreServiceOpaqueFD.cpp"
        )
    else()
        target_sources(dawn_native PRIVATE
            "vulkan/external_memory/MemoryServiceNull.cpp"
            "vulkan/external_semaphore/SemaphoreServiceNull.cpp"
        )
    endif()
endif()

# TODO how to do the component build in CMake?
target_sources(dawn_native PRIVATE "DawnNative.cpp")
if (DAWN_ENABLE_D3D12)
    target_sources(dawn_native PRIVATE "d3d12/D3D12Backend.cpp")
endif()
if (DAWN_ENABLE_METAL)
    target_sources(dawn_native PRIVATE "metal/MetalBackend.mm")
endif()
if (DAWN_ENABLE_NULL)
    target_sources(dawn_native PRIVATE "null/NullBackend.cpp")
endif()
if (DAWN_ENABLE_OPENGL)
    target_sources(dawn_native PRIVATE "opengl/OpenGLBackend.cpp")
endif()
if (DAWN_ENABLE_VULKAN)
    target_sources(dawn_native PRIVATE "vulkan/VulkanBackend.cpp")
endif()
