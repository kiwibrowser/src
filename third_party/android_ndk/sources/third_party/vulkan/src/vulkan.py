" ""VK API description"""

# Copyright (c) 2015-2016 The Khronos Group Inc.
# Copyright (c) 2015-2016 Valve Corporation
# Copyright (c) 2015-2016 LunarG, Inc.
# Copyright (c) 2015-2016 Google Inc.
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
#
# Author: Chia-I Wu <olv@lunarg.com>
# Author: Jon Ashburn <jon@lunarg.com>
# Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
# Author: Tobin Ehlis <tobin@lunarg.com>
# Author: Tony Barbour <tony@LunarG.com>
# Author: Gwan-gyeong Mun <kk.moon@samsung.com>

class Param(object):
    """A function parameter."""

    def __init__(self, ty, name):
        self.ty = ty
        self.name = name

    def c(self):
        """Return the parameter in C."""
        idx = self.ty.find("[")

        # arrays have a different syntax
        if idx >= 0:
            return "%s %s%s" % (self.ty[:idx], self.name, self.ty[idx:])
        else:
            return "%s %s" % (self.ty, self.name)

    def indirection_level(self):
        """Return the level of indirection."""
        return self.ty.count("*") + self.ty.count("[")

    def dereferenced_type(self, level=0):
        """Return the type after dereferencing."""
        if not level:
            level = self.indirection_level()

        deref = self.ty if level else ""
        while level > 0:
            idx = deref.rfind("[")
            if idx < 0:
                idx = deref.rfind("*")
            if idx < 0:
                deref = ""
                break
            deref = deref[:idx]
            level -= 1;

        return deref.rstrip()

class Proto(object):
    """A function prototype."""

    def __init__(self, ret, name, params=[]):
        # the proto has only a param
        if not isinstance(params, list):
            params = [params]

        self.ret = ret
        self.name = name
        self.params = params

    def c_params(self, need_type=True, need_name=True):
        """Return the parameter list in C."""
        if self.params and (need_type or need_name):
            if need_type and need_name:
                return ", ".join([param.c() for param in self.params])
            elif need_type:
                return ", ".join([param.ty for param in self.params])
            else:
                return ", ".join([param.name for param in self.params])
        else:
            return "void" if need_type else ""

    def c_decl(self, name, attr="", typed=False, need_param_names=True):
        """Return a named declaration in C."""
        if typed:
            return "%s (%s*%s)(%s)" % (
                self.ret,
                attr + "_PTR " if attr else "",
                name,
                self.c_params(need_name=need_param_names))
        else:
            return "%s%s %s%s(%s)" % (
                attr + "_ATTR " if attr else "",
                self.ret,
                attr + "_CALL " if attr else "",
                name,
                self.c_params(need_name=need_param_names))

    def c_pretty_decl(self, name, attr=""):
        """Return a named declaration in C, with vulkan.h formatting."""
        plist = []
        for param in self.params:
            idx = param.ty.find("[")
            if idx < 0:
                idx = len(param.ty)

            pad = 44 - idx
            if pad <= 0:
                pad = 1

            plist.append("    %s%s%s%s" % (param.ty[:idx],
                " " * pad, param.name, param.ty[idx:]))

        return "%s%s %s%s(\n%s)" % (
                attr + "_ATTR " if attr else "",
                self.ret,
                attr + "_CALL " if attr else "",
                name,
                ",\n".join(plist))

    def c_func(self, prefix="", attr=""):
        """Return the prototype in C."""
        return self.c_decl(prefix + self.name, attr=attr, typed=False)

    def c_call(self):
        """Return a call to the prototype in C."""
        return "%s(%s)" % (self.name, self.c_params(need_type=False))

    def object_in_params(self):
        """Return the params that are simple VK objects and are inputs."""
        return [param for param in self.params if param.ty in objects]

    def object_out_params(self):
        """Return the params that are simple VK objects and are outputs."""
        return [param for param in self.params
                if param.dereferenced_type() in objects]

    def __repr__(self):
        param_strs = []
        for param in self.params:
            param_strs.append(str(param))
        param_str = "    [%s]" % (",\n     ".join(param_strs))

        return "Proto(\"%s\", \"%s\",\n%s)" % \
                (self.ret, self.name, param_str)

class Extension(object):
    def __init__(self, name, headers, objects, protos, ifdef = None):
        self.name = name
        self.headers = headers
        self.objects = objects
        self.protos = protos
        self.ifdef = ifdef

# VK core API
core = Extension(
    name="VK_CORE",
    headers=["vulkan/vulkan.h"],
    objects=[
        "VkInstance",
        "VkPhysicalDevice",
        "VkDevice",
        "VkQueue",
        "VkSemaphore",
        "VkCommandBuffer",
        "VkFence",
        "VkDeviceMemory",
        "VkBuffer",
        "VkImage",
        "VkEvent",
        "VkQueryPool",
        "VkBufferView",
        "VkImageView",
        "VkShaderModule",
        "VkPipelineCache",
        "VkPipelineLayout",
        "VkRenderPass",
        "VkPipeline",
        "VkDescriptorSetLayout",
        "VkSampler",
        "VkDescriptorPool",
        "VkDescriptorSet",
        "VkFramebuffer",
        "VkCommandPool",
    ],
    protos=[
        Proto("VkResult", "CreateInstance",
            [Param("const VkInstanceCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkInstance*", "pInstance")]),

        Proto("void", "DestroyInstance",
            [Param("VkInstance", "instance"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "EnumeratePhysicalDevices",
            [Param("VkInstance", "instance"),
             Param("uint32_t*", "pPhysicalDeviceCount"),
             Param("VkPhysicalDevice*", "pPhysicalDevices")]),

        Proto("void", "GetPhysicalDeviceFeatures",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkPhysicalDeviceFeatures*", "pFeatures")]),

        Proto("void", "GetPhysicalDeviceFormatProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkFormat", "format"),
             Param("VkFormatProperties*", "pFormatProperties")]),

        Proto("VkResult", "GetPhysicalDeviceImageFormatProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkFormat", "format"),
             Param("VkImageType", "type"),
             Param("VkImageTiling", "tiling"),
             Param("VkImageUsageFlags", "usage"),
             Param("VkImageCreateFlags", "flags"),
             Param("VkImageFormatProperties*", "pImageFormatProperties")]),

        Proto("void", "GetPhysicalDeviceProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkPhysicalDeviceProperties*", "pProperties")]),

        Proto("void", "GetPhysicalDeviceQueueFamilyProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t*", "pQueueFamilyPropertyCount"),
             Param("VkQueueFamilyProperties*", "pQueueFamilyProperties")]),

        Proto("void", "GetPhysicalDeviceMemoryProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkPhysicalDeviceMemoryProperties*", "pMemoryProperties")]),

        Proto("PFN_vkVoidFunction", "GetInstanceProcAddr",
            [Param("VkInstance", "instance"),
             Param("const char*", "pName")]),

        Proto("PFN_vkVoidFunction", "GetDeviceProcAddr",
            [Param("VkDevice", "device"),
             Param("const char*", "pName")]),

        Proto("VkResult", "CreateDevice",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("const VkDeviceCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkDevice*", "pDevice")]),

        Proto("void", "DestroyDevice",
            [Param("VkDevice", "device"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "EnumerateInstanceExtensionProperties",
            [Param("const char*", "pLayerName"),
             Param("uint32_t*", "pPropertyCount"),
             Param("VkExtensionProperties*", "pProperties")]),

        Proto("VkResult", "EnumerateDeviceExtensionProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("const char*", "pLayerName"),
             Param("uint32_t*", "pPropertyCount"),
             Param("VkExtensionProperties*", "pProperties")]),

        Proto("VkResult", "EnumerateInstanceLayerProperties",
            [Param("uint32_t*", "pPropertyCount"),
             Param("VkLayerProperties*", "pProperties")]),

        Proto("VkResult", "EnumerateDeviceLayerProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t*", "pPropertyCount"),
             Param("VkLayerProperties*", "pProperties")]),

        Proto("void", "GetDeviceQueue",
            [Param("VkDevice", "device"),
             Param("uint32_t", "queueFamilyIndex"),
             Param("uint32_t", "queueIndex"),
             Param("VkQueue*", "pQueue")]),

        Proto("VkResult", "QueueSubmit",
            [Param("VkQueue", "queue"),
             Param("uint32_t", "submitCount"),
             Param("const VkSubmitInfo*", "pSubmits"),
             Param("VkFence", "fence")]),

        Proto("VkResult", "QueueWaitIdle",
            [Param("VkQueue", "queue")]),

        Proto("VkResult", "DeviceWaitIdle",
            [Param("VkDevice", "device")]),

        Proto("VkResult", "AllocateMemory",
            [Param("VkDevice", "device"),
             Param("const VkMemoryAllocateInfo*", "pAllocateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkDeviceMemory*", "pMemory")]),

        Proto("void", "FreeMemory",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "memory"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "MapMemory",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "memory"),
             Param("VkDeviceSize", "offset"),
             Param("VkDeviceSize", "size"),
             Param("VkMemoryMapFlags", "flags"),
             Param("void**", "ppData")]),

        Proto("void", "UnmapMemory",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "memory")]),

        Proto("VkResult", "FlushMappedMemoryRanges",
            [Param("VkDevice", "device"),
             Param("uint32_t", "memoryRangeCount"),
             Param("const VkMappedMemoryRange*", "pMemoryRanges")]),

        Proto("VkResult", "InvalidateMappedMemoryRanges",
            [Param("VkDevice", "device"),
             Param("uint32_t", "memoryRangeCount"),
             Param("const VkMappedMemoryRange*", "pMemoryRanges")]),

        Proto("void", "GetDeviceMemoryCommitment",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "memory"),
             Param("VkDeviceSize*", "pCommittedMemoryInBytes")]),

        Proto("VkResult", "BindBufferMemory",
            [Param("VkDevice", "device"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceMemory", "memory"),
             Param("VkDeviceSize", "memoryOffset")]),

        Proto("VkResult", "BindImageMemory",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("VkDeviceMemory", "memory"),
             Param("VkDeviceSize", "memoryOffset")]),

        Proto("void", "GetBufferMemoryRequirements",
            [Param("VkDevice", "device"),
             Param("VkBuffer", "buffer"),
             Param("VkMemoryRequirements*", "pMemoryRequirements")]),

        Proto("void", "GetImageMemoryRequirements",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("VkMemoryRequirements*", "pMemoryRequirements")]),

        Proto("void", "GetImageSparseMemoryRequirements",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("uint32_t*", "pSparseMemoryRequirementCount"),
             Param("VkSparseImageMemoryRequirements*", "pSparseMemoryRequirements")]),

        Proto("void", "GetPhysicalDeviceSparseImageFormatProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkFormat", "format"),
             Param("VkImageType", "type"),
             Param("VkSampleCountFlagBits", "samples"),
             Param("VkImageUsageFlags", "usage"),
             Param("VkImageTiling", "tiling"),
             Param("uint32_t*", "pPropertyCount"),
             Param("VkSparseImageFormatProperties*", "pProperties")]),

        Proto("VkResult", "QueueBindSparse",
            [Param("VkQueue", "queue"),
             Param("uint32_t", "bindInfoCount"),
             Param("const VkBindSparseInfo*", "pBindInfo"),
             Param("VkFence", "fence")]),

        Proto("VkResult", "CreateFence",
            [Param("VkDevice", "device"),
             Param("const VkFenceCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkFence*", "pFence")]),

        Proto("void", "DestroyFence",
            [Param("VkDevice", "device"),
             Param("VkFence", "fence"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "ResetFences",
            [Param("VkDevice", "device"),
             Param("uint32_t", "fenceCount"),
             Param("const VkFence*", "pFences")]),

        Proto("VkResult", "GetFenceStatus",
            [Param("VkDevice", "device"),
             Param("VkFence", "fence")]),

        Proto("VkResult", "WaitForFences",
            [Param("VkDevice", "device"),
             Param("uint32_t", "fenceCount"),
             Param("const VkFence*", "pFences"),
             Param("VkBool32", "waitAll"),
             Param("uint64_t", "timeout")]),

        Proto("VkResult", "CreateSemaphore",
            [Param("VkDevice", "device"),
             Param("const VkSemaphoreCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSemaphore*", "pSemaphore")]),

        Proto("void", "DestroySemaphore",
            [Param("VkDevice", "device"),
             Param("VkSemaphore", "semaphore"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreateEvent",
            [Param("VkDevice", "device"),
             Param("const VkEventCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkEvent*", "pEvent")]),

        Proto("void", "DestroyEvent",
            [Param("VkDevice", "device"),
             Param("VkEvent", "event"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "GetEventStatus",
            [Param("VkDevice", "device"),
             Param("VkEvent", "event")]),

        Proto("VkResult", "SetEvent",
            [Param("VkDevice", "device"),
             Param("VkEvent", "event")]),

        Proto("VkResult", "ResetEvent",
            [Param("VkDevice", "device"),
             Param("VkEvent", "event")]),

        Proto("VkResult", "CreateQueryPool",
            [Param("VkDevice", "device"),
             Param("const VkQueryPoolCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkQueryPool*", "pQueryPool")]),

        Proto("void", "DestroyQueryPool",
            [Param("VkDevice", "device"),
             Param("VkQueryPool", "queryPool"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "GetQueryPoolResults",
            [Param("VkDevice", "device"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "firstQuery"),
             Param("uint32_t", "queryCount"),
             Param("size_t", "dataSize"),
             Param("void*", "pData"),
             Param("VkDeviceSize", "stride"),
             Param("VkQueryResultFlags", "flags")]),

        Proto("VkResult", "CreateBuffer",
            [Param("VkDevice", "device"),
             Param("const VkBufferCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkBuffer*", "pBuffer")]),

        Proto("void", "DestroyBuffer",
            [Param("VkDevice", "device"),
             Param("VkBuffer", "buffer"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreateBufferView",
            [Param("VkDevice", "device"),
             Param("const VkBufferViewCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkBufferView*", "pView")]),

        Proto("void", "DestroyBufferView",
            [Param("VkDevice", "device"),
             Param("VkBufferView", "bufferView"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreateImage",
            [Param("VkDevice", "device"),
             Param("const VkImageCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkImage*", "pImage")]),

        Proto("void", "DestroyImage",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("void", "GetImageSubresourceLayout",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("const VkImageSubresource*", "pSubresource"),
             Param("VkSubresourceLayout*", "pLayout")]),

        Proto("VkResult", "CreateImageView",
            [Param("VkDevice", "device"),
             Param("const VkImageViewCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkImageView*", "pView")]),

        Proto("void", "DestroyImageView",
            [Param("VkDevice", "device"),
             Param("VkImageView", "imageView"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreateShaderModule",
            [Param("VkDevice", "device"),
             Param("const VkShaderModuleCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkShaderModule*", "pShaderModule")]),

        Proto("void", "DestroyShaderModule",
            [Param("VkDevice", "device"),
             Param("VkShaderModule", "shaderModule"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreatePipelineCache",
            [Param("VkDevice", "device"),
             Param("const VkPipelineCacheCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkPipelineCache*", "pPipelineCache")]),

        Proto("void", "DestroyPipelineCache",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "GetPipelineCacheData",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "MergePipelineCaches",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "dstCache"),
             Param("uint32_t", "srcCacheCount"),
             Param("const VkPipelineCache*", "pSrcCaches")]),

        Proto("VkResult", "CreateGraphicsPipelines",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache"),
             Param("uint32_t", "createInfoCount"),
             Param("const VkGraphicsPipelineCreateInfo*", "pCreateInfos"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkPipeline*", "pPipelines")]),

        Proto("VkResult", "CreateComputePipelines",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache"),
             Param("uint32_t", "createInfoCount"),
             Param("const VkComputePipelineCreateInfo*", "pCreateInfos"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkPipeline*", "pPipelines")]),

        Proto("void", "DestroyPipeline",
            [Param("VkDevice", "device"),
             Param("VkPipeline", "pipeline"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreatePipelineLayout",
            [Param("VkDevice", "device"),
             Param("const VkPipelineLayoutCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkPipelineLayout*", "pPipelineLayout")]),

        Proto("void", "DestroyPipelineLayout",
            [Param("VkDevice", "device"),
             Param("VkPipelineLayout", "pipelineLayout"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreateSampler",
            [Param("VkDevice", "device"),
             Param("const VkSamplerCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSampler*", "pSampler")]),

        Proto("void", "DestroySampler",
            [Param("VkDevice", "device"),
             Param("VkSampler", "sampler"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreateDescriptorSetLayout",
            [Param("VkDevice", "device"),
             Param("const VkDescriptorSetLayoutCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkDescriptorSetLayout*", "pSetLayout")]),

        Proto("void", "DestroyDescriptorSetLayout",
            [Param("VkDevice", "device"),
             Param("VkDescriptorSetLayout", "descriptorSetLayout"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreateDescriptorPool",
            [Param("VkDevice", "device"),
             Param("const VkDescriptorPoolCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkDescriptorPool*", "pDescriptorPool")]),

        Proto("void", "DestroyDescriptorPool",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "ResetDescriptorPool",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool"),
             Param("VkDescriptorPoolResetFlags", "flags")]),

        Proto("VkResult", "AllocateDescriptorSets",
            [Param("VkDevice", "device"),
             Param("const VkDescriptorSetAllocateInfo*", "pAllocateInfo"),
             Param("VkDescriptorSet*", "pDescriptorSets")]),

        Proto("VkResult", "FreeDescriptorSets",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool"),
             Param("uint32_t", "descriptorSetCount"),
             Param("const VkDescriptorSet*", "pDescriptorSets")]),

        Proto("void", "UpdateDescriptorSets",
            [Param("VkDevice", "device"),
             Param("uint32_t", "descriptorWriteCount"),
             Param("const VkWriteDescriptorSet*", "pDescriptorWrites"),
             Param("uint32_t", "descriptorCopyCount"),
             Param("const VkCopyDescriptorSet*", "pDescriptorCopies")]),

        Proto("VkResult", "CreateFramebuffer",
            [Param("VkDevice", "device"),
             Param("const VkFramebufferCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkFramebuffer*", "pFramebuffer")]),

        Proto("void", "DestroyFramebuffer",
            [Param("VkDevice", "device"),
             Param("VkFramebuffer", "framebuffer"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "CreateRenderPass",
            [Param("VkDevice", "device"),
             Param("const VkRenderPassCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkRenderPass*", "pRenderPass")]),

        Proto("void", "DestroyRenderPass",
            [Param("VkDevice", "device"),
             Param("VkRenderPass", "renderPass"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("void", "GetRenderAreaGranularity",
            [Param("VkDevice", "device"),
             Param("VkRenderPass", "renderPass"),
             Param("VkExtent2D*", "pGranularity")]),

        Proto("VkResult", "CreateCommandPool",
            [Param("VkDevice", "device"),
             Param("const VkCommandPoolCreateInfo*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkCommandPool*", "pCommandPool")]),

        Proto("void", "DestroyCommandPool",
            [Param("VkDevice", "device"),
             Param("VkCommandPool", "commandPool"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "ResetCommandPool",
            [Param("VkDevice", "device"),
             Param("VkCommandPool", "commandPool"),
             Param("VkCommandPoolResetFlags", "flags")]),

        Proto("VkResult", "AllocateCommandBuffers",
            [Param("VkDevice", "device"),
             Param("const VkCommandBufferAllocateInfo*", "pAllocateInfo"),
             Param("VkCommandBuffer*", "pCommandBuffers")]),

        Proto("void", "FreeCommandBuffers",
            [Param("VkDevice", "device"),
             Param("VkCommandPool", "commandPool"),
             Param("uint32_t", "commandBufferCount"),
             Param("const VkCommandBuffer*", "pCommandBuffers")]),

        Proto("VkResult", "BeginCommandBuffer",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("const VkCommandBufferBeginInfo*", "pBeginInfo")]),

        Proto("VkResult", "EndCommandBuffer",
            [Param("VkCommandBuffer", "commandBuffer")]),

        Proto("VkResult", "ResetCommandBuffer",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkCommandBufferResetFlags", "flags")]),

        Proto("void", "CmdBindPipeline",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("VkPipeline", "pipeline")]),

        Proto("void", "CmdSetViewport",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "firstViewport"),
             Param("uint32_t", "viewportCount"),
             Param("const VkViewport*", "pViewports")]),

        Proto("void", "CmdSetScissor",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "firstScissor"),
             Param("uint32_t", "scissorCount"),
             Param("const VkRect2D*", "pScissors")]),

        Proto("void", "CmdSetLineWidth",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("float", "lineWidth")]),

        Proto("void", "CmdSetDepthBias",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("float", "depthBiasConstantFactor"),
             Param("float", "depthBiasClamp"),
             Param("float", "depthBiasSlopeFactor")]),

        Proto("void", "CmdSetBlendConstants",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("const float[4]", "blendConstants")]),

        Proto("void", "CmdSetDepthBounds",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("float", "minDepthBounds"),
             Param("float", "maxDepthBounds")]),

        Proto("void", "CmdSetStencilCompareMask",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkStencilFaceFlags", "faceMask"),
             Param("uint32_t", "compareMask")]),

        Proto("void", "CmdSetStencilWriteMask",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkStencilFaceFlags", "faceMask"),
             Param("uint32_t", "writeMask")]),

        Proto("void", "CmdSetStencilReference",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkStencilFaceFlags", "faceMask"),
             Param("uint32_t", "reference")]),

        Proto("void", "CmdBindDescriptorSets",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("VkPipelineLayout", "layout"),
             Param("uint32_t", "firstSet"),
             Param("uint32_t", "descriptorSetCount"),
             Param("const VkDescriptorSet*", "pDescriptorSets"),
             Param("uint32_t", "dynamicOffsetCount"),
             Param("const uint32_t*", "pDynamicOffsets")]),

        Proto("void", "CmdBindIndexBuffer",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("VkIndexType", "indexType")]),

        Proto("void", "CmdBindVertexBuffers",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "firstBinding"),
             Param("uint32_t", "bindingCount"),
             Param("const VkBuffer*", "pBuffers"),
             Param("const VkDeviceSize*", "pOffsets")]),

        Proto("void", "CmdDraw",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "vertexCount"),
             Param("uint32_t", "instanceCount"),
             Param("uint32_t", "firstVertex"),
             Param("uint32_t", "firstInstance")]),

        Proto("void", "CmdDrawIndexed",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "indexCount"),
             Param("uint32_t", "instanceCount"),
             Param("uint32_t", "firstIndex"),
             Param("int32_t", "vertexOffset"),
             Param("uint32_t", "firstInstance")]),

        Proto("void", "CmdDrawIndirect",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("uint32_t", "drawCount"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDrawIndexedIndirect",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("uint32_t", "drawCount"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDispatch",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "x"),
             Param("uint32_t", "y"),
             Param("uint32_t", "z")]),

        Proto("void", "CmdDispatchIndirect",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset")]),

        Proto("void", "CmdCopyBuffer",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "srcBuffer"),
             Param("VkBuffer", "dstBuffer"),
             Param("uint32_t", "regionCount"),
             Param("const VkBufferCopy*", "pRegions")]),

        Proto("void", "CmdCopyImage",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkImage", "srcImage"),
             Param("VkImageLayout", "srcImageLayout"),
             Param("VkImage", "dstImage"),
             Param("VkImageLayout", "dstImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VkImageCopy*", "pRegions")]),

        Proto("void", "CmdBlitImage",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkImage", "srcImage"),
             Param("VkImageLayout", "srcImageLayout"),
             Param("VkImage", "dstImage"),
             Param("VkImageLayout", "dstImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VkImageBlit*", "pRegions"),
             Param("VkFilter", "filter")]),

        Proto("void", "CmdCopyBufferToImage",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "srcBuffer"),
             Param("VkImage", "dstImage"),
             Param("VkImageLayout", "dstImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VkBufferImageCopy*", "pRegions")]),

        Proto("void", "CmdCopyImageToBuffer",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkImage", "srcImage"),
             Param("VkImageLayout", "srcImageLayout"),
             Param("VkBuffer", "dstBuffer"),
             Param("uint32_t", "regionCount"),
             Param("const VkBufferImageCopy*", "pRegions")]),

        Proto("void", "CmdUpdateBuffer",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "dstBuffer"),
             Param("VkDeviceSize", "dstOffset"),
             Param("VkDeviceSize", "dataSize"),
             Param("const void*", "pData")]),

        Proto("void", "CmdFillBuffer",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "dstBuffer"),
             Param("VkDeviceSize", "dstOffset"),
             Param("VkDeviceSize", "size"),
             Param("uint32_t", "data")]),

        Proto("void", "CmdClearColorImage",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkImage", "image"),
             Param("VkImageLayout", "imageLayout"),
             Param("const VkClearColorValue*", "pColor"),
             Param("uint32_t", "rangeCount"),
             Param("const VkImageSubresourceRange*", "pRanges")]),

        Proto("void", "CmdClearDepthStencilImage",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkImage", "image"),
             Param("VkImageLayout", "imageLayout"),
             Param("const VkClearDepthStencilValue*", "pDepthStencil"),
             Param("uint32_t", "rangeCount"),
             Param("const VkImageSubresourceRange*", "pRanges")]),

        Proto("void", "CmdClearAttachments",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "attachmentCount"),
             Param("const VkClearAttachment*", "pAttachments"),
             Param("uint32_t", "rectCount"),
             Param("const VkClearRect*", "pRects")]),

        Proto("void", "CmdResolveImage",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkImage", "srcImage"),
             Param("VkImageLayout", "srcImageLayout"),
             Param("VkImage", "dstImage"),
             Param("VkImageLayout", "dstImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VkImageResolve*", "pRegions")]),

        Proto("void", "CmdSetEvent",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkEvent", "event"),
             Param("VkPipelineStageFlags", "stageMask")]),

        Proto("void", "CmdResetEvent",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkEvent", "event"),
             Param("VkPipelineStageFlags", "stageMask")]),

        Proto("void", "CmdWaitEvents",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "eventCount"),
             Param("const VkEvent*", "pEvents"),
             Param("VkPipelineStageFlags", "srcStageMask"),
             Param("VkPipelineStageFlags", "dstStageMask"),
             Param("uint32_t", "memoryBarrierCount"),
             Param("const VkMemoryBarrier*", "pMemoryBarriers"),
             Param("uint32_t", "bufferMemoryBarrierCount"),
             Param("const VkBufferMemoryBarrier*", "pBufferMemoryBarriers"),
             Param("uint32_t", "imageMemoryBarrierCount"),
             Param("const VkImageMemoryBarrier*", "pImageMemoryBarriers")]),

        Proto("void", "CmdPipelineBarrier",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkPipelineStageFlags", "srcStageMask"),
             Param("VkPipelineStageFlags", "dstStageMask"),
             Param("VkDependencyFlags", "dependencyFlags"),
             Param("uint32_t", "memoryBarrierCount"),
             Param("const VkMemoryBarrier*", "pMemoryBarriers"),
             Param("uint32_t", "bufferMemoryBarrierCount"),
             Param("const VkBufferMemoryBarrier*", "pBufferMemoryBarriers"),
             Param("uint32_t", "imageMemoryBarrierCount"),
             Param("const VkImageMemoryBarrier*", "pImageMemoryBarriers")]),

        Proto("void", "CmdBeginQuery",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "query"),
             Param("VkQueryControlFlags", "flags")]),

        Proto("void", "CmdEndQuery",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "query")]),

        Proto("void", "CmdResetQueryPool",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "firstQuery"),
             Param("uint32_t", "queryCount")]),

        Proto("void", "CmdWriteTimestamp",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkPipelineStageFlagBits", "pipelineStage"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "query")]),

        Proto("void", "CmdCopyQueryPoolResults",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "firstQuery"),
             Param("uint32_t", "queryCount"),
             Param("VkBuffer", "dstBuffer"),
             Param("VkDeviceSize", "dstOffset"),
             Param("VkDeviceSize", "stride"),
             Param("VkQueryResultFlags", "flags")]),

        Proto("void", "CmdPushConstants",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkPipelineLayout", "layout"),
             Param("VkShaderStageFlags", "stageFlags"),
             Param("uint32_t", "offset"),
             Param("uint32_t", "size"),
             Param("const void*", "pValues")]),

        Proto("void", "CmdBeginRenderPass",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("const VkRenderPassBeginInfo*", "pRenderPassBegin"),
             Param("VkSubpassContents", "contents")]),

        Proto("void", "CmdNextSubpass",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkSubpassContents", "contents")]),

        Proto("void", "CmdEndRenderPass",
            [Param("VkCommandBuffer", "commandBuffer")]),

        Proto("void", "CmdExecuteCommands",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("uint32_t", "commandBufferCount"),
             Param("const VkCommandBuffer*", "pCommandBuffers")]),
    ],
)

ext_amd_draw_indirect_count = Extension(
    name="VK_AMD_draw_indirect_count",
    headers=["vulkan/vulkan.h"],
    objects=[],
    protos=[
        Proto("void", "CmdDrawIndirectCountAMD",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("VkBuffer", "countBuffer"),
             Param("VkDeviceSize", "countBufferOffset"),
             Param("uint32_t", "maxDrawCount"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDrawIndexedIndirectCountAMD",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("VkBuffer", "countBuffer"),
             Param("VkDeviceSize", "countBufferOffset"),
             Param("uint32_t", "maxDrawCount"),
             Param("uint32_t", "stride")]),
    ],
)

ext_nv_external_memory_capabilities = Extension(
    name="VK_NV_external_memory_capabilities",
    headers=["vulkan/vulkan.h"],
    objects=[],
    protos=[
        Proto("VkResult", "GetPhysicalDeviceExternalImageFormatPropertiesNV",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkFormat", "format"),
             Param("VkImageType", "type"),
             Param("VkImageTiling", "tiling"),
             Param("VkImageUsageFlags", "usage"),
             Param("VkImageCreateFlags", "flags"),
             Param("VkExternalMemoryHandleTypeFlagsNV", "externalHandleType"),
             Param("VkExternalImageFormatPropertiesNV*", "pExternalImageFormatProperties")]),
    ],
)

ext_nv_external_memory_win32 = Extension(
    name="VK_NV_external_memory_win32",
    headers=["vulkan/vulkan.h"],
    objects=[],
    ifdef="VK_USE_PLATFORM_WIN32_KHR",
    protos=[
        Proto("VkResult", "GetMemoryWin32HandleNV",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "memory"),
             Param("VkExternalMemoryHandleTypeFlagsNV", "handleType"),
             Param("HANDLE*", "pHandle")]),
    ],
)

ext_khr_surface = Extension(
    name="VK_KHR_surface",
    headers=["vulkan/vulkan.h"],
    objects=["vkSurfaceKHR"],
    protos=[
        Proto("void", "DestroySurfaceKHR",
            [Param("VkInstance", "instance"),
             Param("VkSurfaceKHR", "surface"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "GetPhysicalDeviceSurfaceSupportKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t", "queueFamilyIndex"),
             Param("VkSurfaceKHR", "surface"),
             Param("VkBool32*", "pSupported")]),

        Proto("VkResult", "GetPhysicalDeviceSurfaceCapabilitiesKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkSurfaceKHR", "surface"),
             Param("VkSurfaceCapabilitiesKHR*", "pSurfaceCapabilities")]),

        Proto("VkResult", "GetPhysicalDeviceSurfaceFormatsKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkSurfaceKHR", "surface"),
             Param("uint32_t*", "pSurfaceFormatCount"),
             Param("VkSurfaceFormatKHR*", "pSurfaceFormats")]),

        Proto("VkResult", "GetPhysicalDeviceSurfacePresentModesKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkSurfaceKHR", "surface"),
             Param("uint32_t*", "pPresentModeCount"),
             Param("VkPresentModeKHR*", "pPresentModes")]),
    ],
)

ext_khr_display = Extension(
    name="VK_KHR_display",
    headers=["vulkan/vulkan.h"],
    objects=['VkSurfaceKHR', 'VkDisplayModeKHR'],
    protos=[
        Proto("VkResult", "GetPhysicalDeviceDisplayPropertiesKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t*", "pPropertyCount"),
             Param("VkDisplayPropertiesKHR*", "pProperties")]),

        Proto("VkResult", "GetPhysicalDeviceDisplayPlanePropertiesKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t*", "pPropertyCount"),
             Param("VkDisplayPlanePropertiesKHR*", "pProperties")]),

        Proto("VkResult", "GetDisplayPlaneSupportedDisplaysKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t", "planeIndex"),
             Param("uint32_t*", "pDisplayCount"),
             Param("VkDisplayKHR*", "pDisplays")]),

        Proto("VkResult", "GetDisplayModePropertiesKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkDisplayKHR", "display"),
             Param("uint32_t*", "pPropertyCount"),
             Param("VkDisplayModePropertiesKHR*", "pProperties")]),

        Proto("VkResult", "CreateDisplayModeKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkDisplayKHR", "display"),
             Param("const VkDisplayModeCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkDisplayModeKHR*", "pMode")]),

        Proto("VkResult", "GetDisplayPlaneCapabilitiesKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkDisplayModeKHR", "mode"),
             Param("uint32_t", "planeIndex"),
             Param("VkDisplayPlaneCapabilitiesKHR*", "pCapabilities")]),

        Proto("VkResult", "CreateDisplayPlaneSurfaceKHR",
            [Param("VkInstance", "instance"),
             Param("const VkDisplaySurfaceCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSurfaceKHR*", "pSurface")]),
    ],
)

ext_khr_device_swapchain = Extension(
    name="VK_KHR_swapchain",
    headers=["vulkan/vulkan.h"],
    objects=["VkSwapchainKHR"],
    protos=[
        Proto("VkResult", "CreateSwapchainKHR",
            [Param("VkDevice", "device"),
             Param("const VkSwapchainCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSwapchainKHR*", "pSwapchain")]),

        Proto("void", "DestroySwapchainKHR",
            [Param("VkDevice", "device"),
             Param("VkSwapchainKHR", "swapchain"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("VkResult", "GetSwapchainImagesKHR",
            [Param("VkDevice", "device"),
         Param("VkSwapchainKHR", "swapchain"),
         Param("uint32_t*", "pSwapchainImageCount"),
             Param("VkImage*", "pSwapchainImages")]),

        Proto("VkResult", "AcquireNextImageKHR",
            [Param("VkDevice", "device"),
             Param("VkSwapchainKHR", "swapchain"),
             Param("uint64_t", "timeout"),
             Param("VkSemaphore", "semaphore"),
             Param("VkFence", "fence"),
             Param("uint32_t*", "pImageIndex")]),

        Proto("VkResult", "QueuePresentKHR",
            [Param("VkQueue", "queue"),
             Param("const VkPresentInfoKHR*", "pPresentInfo")]),
    ],
)

ext_khr_display_swapchain = Extension(
    name="VK_KHR_display_swapchain",
    headers=["vulkan/vulkan.h"],
    objects=["VkDisplayPresentInfoKHR"],
    protos=[
        Proto("VkResult", "CreateSharedSwapchainsKHR",
            [Param("VkDevice", "device"),
             Param("uint32_t", "swapchainCount"),
             Param("const VkSwapchainCreateInfoKHR*", "pCreateInfos"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSwapchainKHR*", "pSwapchains")]),
    ],
)

ext_khr_xcb_surface = Extension(
    name="VK_KHR_xcb_surface",
    headers=["vulkan/vulkan.h"],
    objects=[],
    protos=[
        Proto("VkResult", "CreateXcbSurfaceKHR",
            [Param("VkInstance", "instance"),
             Param("const VkXcbSurfaceCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSurfaceKHR*", "pSurface")]),

        Proto("VkBool32", "GetPhysicalDeviceXcbPresentationSupportKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t", "queueFamilyIndex"),
             Param("xcb_connection_t*", "connection"),
             Param("xcb_visualid_t", "visual_id")]),
    ],
)
ext_khr_xlib_surface = Extension(
    name="VK_KHR_xlib_surface",
    headers=["vulkan/vulkan.h"],
    objects=[],
    ifdef="VK_USE_PLATFORM_XLIB_KHR",
    protos=[
        Proto("VkResult", "CreateXlibSurfaceKHR",
            [Param("VkInstance", "instance"),
             Param("const VkXlibSurfaceCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSurfaceKHR*", "pSurface")]),

        Proto("VkBool32", "GetPhysicalDeviceXlibPresentationSupportKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t", "queueFamilyIndex"),
             Param("Display*", "dpy"),
             Param("VisualID", "visualID")]),
    ],
)
ext_khr_wayland_surface = Extension(
    name="VK_KHR_wayland_surface",
    headers=["vulkan/vulkan.h"],
    objects=[],
    protos=[
        Proto("VkResult", "CreateWaylandSurfaceKHR",
            [Param("VkInstance", "instance"),
             Param("const VkWaylandSurfaceCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSurfaceKHR*", "pSurface")]),

        Proto("VkBool32", "GetPhysicalDeviceWaylandPresentationSupportKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t", "queueFamilyIndex"),
             Param("struct wl_display*", "display")]),
    ],
)
ext_khr_mir_surface = Extension(
    name="VK_KHR_mir_surface",
    headers=["vulkan/vulkan.h"],
    objects=[],
    protos=[
        Proto("VkResult", "CreateMirSurfaceKHR",
            [Param("VkInstance", "instance"),
             Param("const VkMirSurfaceCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSurfaceKHR*", "pSurface")]),

        Proto("VkBool32", "GetPhysicalDeviceMirPresentationSupportKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t", "queueFamilyIndex"),
             Param("MirConnection*", "connection")]),
    ],
)
ext_khr_android_surface = Extension(
    name="VK_KHR_android_surface",
    headers=["vulkan/vulkan.h"],
    objects=[],
    protos=[
        Proto("VkResult", "CreateAndroidSurfaceKHR",
            [Param("VkInstance", "instance"),
             Param("const VkAndroidSurfaceCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSurfaceKHR*", "pSurface")]),
    ],
)
ext_khr_win32_surface = Extension(
    name="VK_KHR_win32_surface",
    headers=["vulkan/vulkan.h"],
    objects=[],
    protos=[
        Proto("VkResult", "CreateWin32SurfaceKHR",
            [Param("VkInstance", "instance"),
             Param("const VkWin32SurfaceCreateInfoKHR*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkSurfaceKHR*", "pSurface")]),

        Proto("VkBool32", "GetPhysicalDeviceWin32PresentationSupportKHR",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t", "queueFamilyIndex")]),
    ],
)
ext_debug_report = Extension(
    name="VK_EXT_debug_report",
    headers=["vulkan/vulkan.h"],
    objects=[
        "VkDebugReportCallbackEXT",
    ],
    protos=[
        Proto("VkResult", "CreateDebugReportCallbackEXT",
            [Param("VkInstance", "instance"),
             Param("const VkDebugReportCallbackCreateInfoEXT*", "pCreateInfo"),
             Param("const VkAllocationCallbacks*", "pAllocator"),
             Param("VkDebugReportCallbackEXT*", "pCallback")]),

        Proto("void", "DestroyDebugReportCallbackEXT",
            [Param("VkInstance", "instance"),
             Param("VkDebugReportCallbackEXT", "callback"),
             Param("const VkAllocationCallbacks*", "pAllocator")]),

        Proto("void", "DebugReportMessageEXT",
            [Param("VkInstance", "instance"),
             Param("VkDebugReportFlagsEXT", "flags"),
             Param("VkDebugReportObjectTypeEXT", "objType"),
             Param("uint64_t", "object"),
             Param("size_t", "location"),
             Param("int32_t", "msgCode"),
             Param("const char *", "pLayerPrefix"),
             Param("const char *", "pMsg")]),
    ],
)
ext_debug_marker = Extension(
    name="VK_EXT_debug_marker",
    headers=["vulkan/vulkan.h"],
    objects=[
        "VkDebugMarkerObjectNameInfoEXT",
        "VkDebugMarkerObjectTagInfoEXT",
        "VkDebugMarkerMarkerInfoEXT"
    ],
    protos=[
        Proto("VkResult", "DebugMarkerSetObjectTagEXT",
            [Param("VkDevice", "device"),
             Param("VkDebugMarkerObjectTagInfoEXT*", "pTagInfo")]),

        Proto("VkResult", "DebugMarkerSetObjectNameEXT",
            [Param("VkDevice", "device"),
             Param("VkDebugMarkerObjectNameInfoEXT*", "pNameInfo")]),

        Proto("void", "CmdDebugMarkerBeginEXT",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkDebugMarkerMarkerInfoEXT*", "pMarkerInfo")]),

        Proto("void", "CmdDebugMarkerEndEXT",
            [Param("VkCommandBuffer", "commandBuffer")]),

        Proto("void", "CmdDebugMarkerInsertEXT",
            [Param("VkCommandBuffer", "commandBuffer"),
             Param("VkDebugMarkerMarkerInfoEXT*", "pMarkerInfo")]),
    ],
)

import sys

if sys.argv[1] == 'AllPlatforms':
    extensions = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_win32_surface, ext_khr_xcb_surface,
                         ext_khr_xlib_surface, ext_khr_wayland_surface, ext_khr_mir_surface, ext_khr_display,
                         ext_khr_android_surface, ext_khr_display_swapchain]
    extensions_all = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_win32_surface,
                             ext_khr_xcb_surface, ext_khr_xlib_surface, ext_khr_wayland_surface, ext_khr_mir_surface,
                             ext_khr_display, ext_khr_android_surface, ext_amd_draw_indirect_count,
                             ext_nv_external_memory_capabilities, ext_nv_external_memory_win32,
                             ext_khr_display_swapchain, ext_debug_report, ext_debug_marker]
else :
    if len(sys.argv) > 3:
        if (sys.platform.startswith('win32') or sys.platform.startswith('msys')) and sys.argv[1] != 'Android':
            extensions = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_win32_surface,
                                 ext_khr_display, ext_khr_display_swapchain]
            extensions_all = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_win32_surface,
                                      ext_khr_display, ext_amd_draw_indirect_count,
                                      ext_nv_external_memory_capabilities, ext_nv_external_memory_win32,
                                      ext_khr_display_swapchain, ext_debug_report, ext_debug_marker]
        elif sys.platform.startswith('linux') and sys.argv[1] != 'Android':
            extensions = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_xcb_surface,
                                 ext_khr_xlib_surface, ext_khr_wayland_surface, ext_khr_mir_surface, ext_khr_display,
                                 ext_khr_display_swapchain]
            extensions_all = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_xcb_surface,
                                      ext_khr_xlib_surface, ext_khr_wayland_surface, ext_khr_mir_surface,
                                      ext_khr_display, ext_amd_draw_indirect_count,
                                      ext_nv_external_memory_capabilities, ext_khr_display_swapchain,
                                      ext_debug_report, ext_debug_marker]
        else: # android
            extensions = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_android_surface,
                                 ext_khr_display_swapchain]
            extensions_all = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_android_surface,
                                      ext_amd_draw_indirect_count, ext_nv_external_memory_capabilities,
                                      ext_khr_display_swapchain, ext_debug_report, ext_debug_marker]
    else :
        if sys.argv[1] == 'Win32' or sys.argv[1] == 'msys':
            extensions = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_win32_surface,
                                 ext_khr_display, ext_khr_display_swapchain]
            extensions_all = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_win32_surface,
                                      ext_khr_display, ext_amd_draw_indirect_count,
                                      ext_nv_external_memory_capabilities, ext_nv_external_memory_win32,
                                      ext_khr_display_swapchain, ext_debug_report, ext_debug_marker]
        elif sys.argv[1] == 'Android':
            extensions = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_android_surface,
                                 ext_khr_display_swapchain]
            extensions_all = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_android_surface,
                                      ext_amd_draw_indirect_count, ext_nv_external_memory_capabilities,
                                      ext_khr_display_swapchain, ext_debug_report, ext_debug_marker]
        elif sys.argv[1] == 'Xcb' or sys.argv[1] == 'Xlib' or sys.argv[1] == 'Wayland' or sys.argv[1] == 'Mir' or sys.argv[1] == 'Display':
            extensions = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_xcb_surface,
                                 ext_khr_xlib_surface, ext_khr_wayland_surface, ext_khr_mir_surface,
                                 ext_khr_display, ext_khr_display_swapchain]
            extensions_all = [core, ext_khr_surface, ext_khr_device_swapchain, ext_khr_xcb_surface,
                                      ext_khr_xlib_surface, ext_khr_wayland_surface, ext_khr_mir_surface,
                                      ext_khr_display, ext_amd_draw_indirect_count,
                                      ext_nv_external_memory_capabilities, ext_khr_display_swapchain,
                                      ext_debug_report, ext_debug_marker]
        else:
            print('Error: Undefined DisplayServer')
            extensions = []
            extensions_all = []

object_dispatch_list = [
    "VkInstance",
    "VkPhysicalDevice",
    "VkDevice",
    "VkQueue",
    "VkCommandBuffer",
]

object_non_dispatch_list = [
    "VkCommandPool",
    "VkFence",
    "VkDeviceMemory",
    "VkBuffer",
    "VkImage",
    "VkSemaphore",
    "VkEvent",
    "VkQueryPool",
    "VkBufferView",
    "VkImageView",
    "VkShaderModule",
    "VkPipelineCache",
    "VkPipelineLayout",
    "VkPipeline",
    "VkDescriptorSetLayout",
    "VkSampler",
    "VkDescriptorPool",
    "VkDescriptorSet",
    "VkRenderPass",
    "VkFramebuffer",
    "VkSwapchainKHR",
    "VkSurfaceKHR",
    "VkDebugReportCallbackEXT",
    "VkDisplayKHR",
    "VkDisplayModeKHR",
]

object_type_list = object_dispatch_list + object_non_dispatch_list

headers = []
objects = []
protos = []
for ext in extensions:
    headers.extend(ext.headers)
    objects.extend(ext.objects)
    protos.extend(ext.protos)

proto_names = [proto.name for proto in protos]

headers_all = []
objects_all = []
protos_all = []
for ext in extensions_all:
    headers_all.extend(ext.headers)
    objects_all.extend(ext.objects)
    protos_all.extend(ext.protos)

proto_all_names = [proto.name for proto in protos_all]
