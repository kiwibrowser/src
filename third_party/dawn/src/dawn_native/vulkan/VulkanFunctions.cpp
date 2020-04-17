// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn_native/vulkan/VulkanFunctions.h"

#include "common/DynamicLib.h"
#include "dawn_native/vulkan/VulkanInfo.h"

namespace dawn_native { namespace vulkan {

#define GET_GLOBAL_PROC(name)                                                          \
    name = reinterpret_cast<decltype(name)>(GetInstanceProcAddr(nullptr, "vk" #name)); \
    if (name == nullptr) {                                                             \
        return DAWN_CONTEXT_LOST_ERROR(std::string("Couldn't get proc vk") + #name);   \
    }

    MaybeError VulkanFunctions::LoadGlobalProcs(const DynamicLib& vulkanLib) {
        if (!vulkanLib.GetProc(&GetInstanceProcAddr, "vkGetInstanceProcAddr")) {
            return DAWN_CONTEXT_LOST_ERROR("Couldn't get vkGetInstanceProcAddr");
        }

        GET_GLOBAL_PROC(CreateInstance);
        GET_GLOBAL_PROC(EnumerateInstanceExtensionProperties);
        GET_GLOBAL_PROC(EnumerateInstanceLayerProperties);

        return {};
    }

#define GET_INSTANCE_PROC(name)                                                         \
    name = reinterpret_cast<decltype(name)>(GetInstanceProcAddr(instance, "vk" #name)); \
    if (name == nullptr) {                                                              \
        return DAWN_CONTEXT_LOST_ERROR(std::string("Couldn't get proc vk") + #name);    \
    }

    MaybeError VulkanFunctions::LoadInstanceProcs(VkInstance instance,
                                                  const VulkanGlobalKnobs& usedKnobs) {
        // Load this proc first so that we can destroy the instance even if some other
        // GET_INSTANCE_PROC fails
        GET_INSTANCE_PROC(DestroyInstance);

        GET_INSTANCE_PROC(CreateDevice);
        GET_INSTANCE_PROC(DestroyDevice);
        GET_INSTANCE_PROC(EnumerateDeviceExtensionProperties);
        GET_INSTANCE_PROC(EnumerateDeviceLayerProperties);
        GET_INSTANCE_PROC(EnumeratePhysicalDevices);
        GET_INSTANCE_PROC(GetDeviceProcAddr);
        GET_INSTANCE_PROC(GetPhysicalDeviceFeatures);
        GET_INSTANCE_PROC(GetPhysicalDeviceFormatProperties);
        GET_INSTANCE_PROC(GetPhysicalDeviceImageFormatProperties);
        GET_INSTANCE_PROC(GetPhysicalDeviceMemoryProperties);
        GET_INSTANCE_PROC(GetPhysicalDeviceProperties);
        GET_INSTANCE_PROC(GetPhysicalDeviceQueueFamilyProperties);
        GET_INSTANCE_PROC(GetPhysicalDeviceSparseImageFormatProperties);

        if (usedKnobs.debugReport) {
            GET_INSTANCE_PROC(CreateDebugReportCallbackEXT);
            GET_INSTANCE_PROC(DebugReportMessageEXT);
            GET_INSTANCE_PROC(DestroyDebugReportCallbackEXT);
        }

        if (usedKnobs.surface) {
            GET_INSTANCE_PROC(DestroySurfaceKHR);
            GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceSupportKHR);
            GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceCapabilitiesKHR);
            GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceFormatsKHR);
            GET_INSTANCE_PROC(GetPhysicalDeviceSurfacePresentModesKHR);
        }

        return {};
    }

#define GET_DEVICE_PROC(name)                                                        \
    name = reinterpret_cast<decltype(name)>(GetDeviceProcAddr(device, "vk" #name));  \
    if (name == nullptr) {                                                           \
        return DAWN_CONTEXT_LOST_ERROR(std::string("Couldn't get proc vk") + #name); \
    }

    MaybeError VulkanFunctions::LoadDeviceProcs(VkDevice device,
                                                const VulkanDeviceKnobs& usedKnobs) {
        GET_DEVICE_PROC(AllocateCommandBuffers);
        GET_DEVICE_PROC(AllocateDescriptorSets);
        GET_DEVICE_PROC(AllocateMemory);
        GET_DEVICE_PROC(BeginCommandBuffer);
        GET_DEVICE_PROC(BindBufferMemory);
        GET_DEVICE_PROC(BindImageMemory);
        GET_DEVICE_PROC(CmdBeginQuery);
        GET_DEVICE_PROC(CmdBeginRenderPass);
        GET_DEVICE_PROC(CmdBindDescriptorSets);
        GET_DEVICE_PROC(CmdBindIndexBuffer);
        GET_DEVICE_PROC(CmdBindPipeline);
        GET_DEVICE_PROC(CmdBindVertexBuffers);
        GET_DEVICE_PROC(CmdBlitImage);
        GET_DEVICE_PROC(CmdClearAttachments);
        GET_DEVICE_PROC(CmdClearColorImage);
        GET_DEVICE_PROC(CmdClearDepthStencilImage);
        GET_DEVICE_PROC(CmdCopyBuffer);
        GET_DEVICE_PROC(CmdCopyBufferToImage);
        GET_DEVICE_PROC(CmdCopyImage);
        GET_DEVICE_PROC(CmdCopyImageToBuffer);
        GET_DEVICE_PROC(CmdCopyQueryPoolResults);
        GET_DEVICE_PROC(CmdDispatch);
        GET_DEVICE_PROC(CmdDispatchIndirect);
        GET_DEVICE_PROC(CmdDraw);
        GET_DEVICE_PROC(CmdDrawIndexed);
        GET_DEVICE_PROC(CmdDrawIndexedIndirect);
        GET_DEVICE_PROC(CmdDrawIndirect);
        GET_DEVICE_PROC(CmdEndQuery);
        GET_DEVICE_PROC(CmdEndRenderPass);
        GET_DEVICE_PROC(CmdExecuteCommands);
        GET_DEVICE_PROC(CmdFillBuffer);
        GET_DEVICE_PROC(CmdNextSubpass);
        GET_DEVICE_PROC(CmdPipelineBarrier);
        GET_DEVICE_PROC(CmdPushConstants);
        GET_DEVICE_PROC(CmdResetEvent);
        GET_DEVICE_PROC(CmdResetQueryPool);
        GET_DEVICE_PROC(CmdResolveImage);
        GET_DEVICE_PROC(CmdSetBlendConstants);
        GET_DEVICE_PROC(CmdSetDepthBias);
        GET_DEVICE_PROC(CmdSetDepthBounds);
        GET_DEVICE_PROC(CmdSetEvent);
        GET_DEVICE_PROC(CmdSetLineWidth);
        GET_DEVICE_PROC(CmdSetScissor);
        GET_DEVICE_PROC(CmdSetStencilCompareMask);
        GET_DEVICE_PROC(CmdSetStencilReference);
        GET_DEVICE_PROC(CmdSetStencilWriteMask);
        GET_DEVICE_PROC(CmdSetViewport);
        GET_DEVICE_PROC(CmdUpdateBuffer);
        GET_DEVICE_PROC(CmdWaitEvents);
        GET_DEVICE_PROC(CmdWriteTimestamp);
        GET_DEVICE_PROC(CreateBuffer);
        GET_DEVICE_PROC(CreateBufferView);
        GET_DEVICE_PROC(CreateCommandPool);
        GET_DEVICE_PROC(CreateComputePipelines);
        GET_DEVICE_PROC(CreateDescriptorPool);
        GET_DEVICE_PROC(CreateDescriptorSetLayout);
        GET_DEVICE_PROC(CreateEvent);
        GET_DEVICE_PROC(CreateFence);
        GET_DEVICE_PROC(CreateFramebuffer);
        GET_DEVICE_PROC(CreateGraphicsPipelines);
        GET_DEVICE_PROC(CreateImage);
        GET_DEVICE_PROC(CreateImageView);
        GET_DEVICE_PROC(CreatePipelineCache);
        GET_DEVICE_PROC(CreatePipelineLayout);
        GET_DEVICE_PROC(CreateQueryPool);
        GET_DEVICE_PROC(CreateRenderPass);
        GET_DEVICE_PROC(CreateSampler);
        GET_DEVICE_PROC(CreateSemaphore);
        GET_DEVICE_PROC(CreateShaderModule);
        GET_DEVICE_PROC(DestroyBuffer);
        GET_DEVICE_PROC(DestroyBufferView);
        GET_DEVICE_PROC(DestroyCommandPool);
        GET_DEVICE_PROC(DestroyDescriptorPool);
        GET_DEVICE_PROC(DestroyDescriptorSetLayout);
        GET_DEVICE_PROC(DestroyEvent);
        GET_DEVICE_PROC(DestroyFence);
        GET_DEVICE_PROC(DestroyFramebuffer);
        GET_DEVICE_PROC(DestroyImage);
        GET_DEVICE_PROC(DestroyImageView);
        GET_DEVICE_PROC(DestroyPipeline);
        GET_DEVICE_PROC(DestroyPipelineCache);
        GET_DEVICE_PROC(DestroyPipelineLayout);
        GET_DEVICE_PROC(DestroyQueryPool);
        GET_DEVICE_PROC(DestroyRenderPass);
        GET_DEVICE_PROC(DestroySampler);
        GET_DEVICE_PROC(DestroySemaphore);
        GET_DEVICE_PROC(DestroyShaderModule);
        GET_DEVICE_PROC(DeviceWaitIdle);
        GET_DEVICE_PROC(EndCommandBuffer);
        GET_DEVICE_PROC(FlushMappedMemoryRanges);
        GET_DEVICE_PROC(FreeCommandBuffers);
        GET_DEVICE_PROC(FreeDescriptorSets);
        GET_DEVICE_PROC(FreeMemory);
        GET_DEVICE_PROC(GetBufferMemoryRequirements);
        GET_DEVICE_PROC(GetDeviceMemoryCommitment);
        GET_DEVICE_PROC(GetDeviceQueue);
        GET_DEVICE_PROC(GetEventStatus);
        GET_DEVICE_PROC(GetFenceStatus);
        GET_DEVICE_PROC(GetImageMemoryRequirements);
        GET_DEVICE_PROC(GetImageSparseMemoryRequirements);
        GET_DEVICE_PROC(GetImageSubresourceLayout);
        GET_DEVICE_PROC(GetPipelineCacheData);
        GET_DEVICE_PROC(GetQueryPoolResults);
        GET_DEVICE_PROC(GetRenderAreaGranularity);
        GET_DEVICE_PROC(InvalidateMappedMemoryRanges);
        GET_DEVICE_PROC(MapMemory);
        GET_DEVICE_PROC(MergePipelineCaches);
        GET_DEVICE_PROC(QueueBindSparse);
        GET_DEVICE_PROC(QueueSubmit);
        GET_DEVICE_PROC(QueueWaitIdle);
        GET_DEVICE_PROC(ResetCommandBuffer);
        GET_DEVICE_PROC(ResetCommandPool);
        GET_DEVICE_PROC(ResetDescriptorPool);
        GET_DEVICE_PROC(ResetEvent);
        GET_DEVICE_PROC(ResetFences);
        GET_DEVICE_PROC(SetEvent);
        GET_DEVICE_PROC(UnmapMemory);
        GET_DEVICE_PROC(UpdateDescriptorSets);
        GET_DEVICE_PROC(WaitForFences);

        if (usedKnobs.debugMarker) {
            GET_DEVICE_PROC(CmdDebugMarkerBeginEXT);
            GET_DEVICE_PROC(CmdDebugMarkerEndEXT);
            GET_DEVICE_PROC(CmdDebugMarkerInsertEXT);
        }

        if (usedKnobs.swapchain) {
            GET_DEVICE_PROC(CreateSwapchainKHR);
            GET_DEVICE_PROC(DestroySwapchainKHR);
            GET_DEVICE_PROC(GetSwapchainImagesKHR);
            GET_DEVICE_PROC(AcquireNextImageKHR);
            GET_DEVICE_PROC(QueuePresentKHR);
        }

        return {};
    }

}}  // namespace dawn_native::vulkan
