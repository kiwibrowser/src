/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2015 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Null (dummy) Vulkan implementation.
 *//*--------------------------------------------------------------------*/

#include "vkNullDriver.hpp"
#include "vkPlatform.hpp"
#include "vkImageUtil.hpp"
#include "vkQueryUtil.hpp"
#include "tcuFunctionLibrary.hpp"
#include "deMemory.h"

#include <stdexcept>
#include <algorithm>

namespace vk
{

namespace
{

using std::vector;

// Memory management

template<typename T>
void* allocateSystemMem (const VkAllocationCallbacks* pAllocator, VkSystemAllocationScope scope)
{
	void* ptr = pAllocator->pfnAllocation(pAllocator->pUserData, sizeof(T), sizeof(void*), scope);
	if (!ptr)
		throw std::bad_alloc();
	return ptr;
}

void freeSystemMem (const VkAllocationCallbacks* pAllocator, void* mem)
{
	pAllocator->pfnFree(pAllocator->pUserData, mem);
}

template<typename Object, typename Handle, typename Parent, typename CreateInfo>
Handle allocateHandle (Parent parent, const CreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	Object* obj = DE_NULL;

	if (pAllocator)
	{
		void* mem = allocateSystemMem<Object>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
		try
		{
			obj = new (mem) Object(parent, pCreateInfo);
			DE_ASSERT(obj == mem);
		}
		catch (...)
		{
			pAllocator->pfnFree(pAllocator->pUserData, mem);
			throw;
		}
	}
	else
		obj = new Object(parent, pCreateInfo);

	return reinterpret_cast<Handle>(obj);
}

template<typename Object, typename Handle, typename CreateInfo>
Handle allocateHandle (const CreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	Object* obj = DE_NULL;

	if (pAllocator)
	{
		void* mem = allocateSystemMem<Object>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
		try
		{
			obj = new (mem) Object(pCreateInfo);
			DE_ASSERT(obj == mem);
		}
		catch (...)
		{
			pAllocator->pfnFree(pAllocator->pUserData, mem);
			throw;
		}
	}
	else
		obj = new Object(pCreateInfo);

	return reinterpret_cast<Handle>(obj);
}

template<typename Object, typename Handle>
void freeHandle (Handle handle, const VkAllocationCallbacks* pAllocator)
{
	Object* obj = reinterpret_cast<Object*>(handle);

	if (pAllocator)
	{
		obj->~Object();
		freeSystemMem(pAllocator, reinterpret_cast<void*>(obj));
	}
	else
		delete obj;
}

template<typename Object, typename Handle, typename Parent, typename CreateInfo>
Handle allocateNonDispHandle (Parent parent, const CreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	Object* const	obj		= allocateHandle<Object, Object*>(parent, pCreateInfo, pAllocator);
	return Handle((deUint64)(deUintptr)obj);
}

template<typename Object, typename Handle>
void freeNonDispHandle (Handle handle, const VkAllocationCallbacks* pAllocator)
{
	freeHandle<Object>(reinterpret_cast<Object*>((deUintptr)handle.getInternal()), pAllocator);
}

// Object definitions

#define VK_NULL_RETURN(STMT)					\
	do {										\
		try {									\
			STMT;								\
			return VK_SUCCESS;					\
		} catch (const std::bad_alloc&) {		\
			return VK_ERROR_OUT_OF_HOST_MEMORY;	\
		} catch (VkResult res) {				\
			return res;							\
		}										\
	} while (deGetFalse())

// \todo [2015-07-14 pyry] Check FUNC type by checkedCastToPtr<T>() or similar
#define VK_NULL_FUNC_ENTRY(NAME, FUNC)	{ #NAME, (deFunctionPtr)FUNC }  // NOLINT(FUNC)

#define VK_NULL_DEFINE_DEVICE_OBJ(NAME)				\
struct NAME											\
{													\
	NAME (VkDevice, const Vk##NAME##CreateInfo*) {}	\
}

VK_NULL_DEFINE_DEVICE_OBJ(Fence);
VK_NULL_DEFINE_DEVICE_OBJ(Semaphore);
VK_NULL_DEFINE_DEVICE_OBJ(Event);
VK_NULL_DEFINE_DEVICE_OBJ(QueryPool);
VK_NULL_DEFINE_DEVICE_OBJ(BufferView);
VK_NULL_DEFINE_DEVICE_OBJ(ImageView);
VK_NULL_DEFINE_DEVICE_OBJ(ShaderModule);
VK_NULL_DEFINE_DEVICE_OBJ(PipelineCache);
VK_NULL_DEFINE_DEVICE_OBJ(PipelineLayout);
VK_NULL_DEFINE_DEVICE_OBJ(RenderPass);
VK_NULL_DEFINE_DEVICE_OBJ(DescriptorSetLayout);
VK_NULL_DEFINE_DEVICE_OBJ(Sampler);
VK_NULL_DEFINE_DEVICE_OBJ(Framebuffer);

class Instance
{
public:
										Instance		(const VkInstanceCreateInfo* instanceInfo);
										~Instance		(void) {}

	PFN_vkVoidFunction					getProcAddr		(const char* name) const { return (PFN_vkVoidFunction)m_functions.getFunction(name); }

private:
	const tcu::StaticFunctionLibrary	m_functions;
};

class SurfaceKHR
{
public:
										SurfaceKHR		(VkInstance, const VkXlibSurfaceCreateInfoKHR*)		{}
										SurfaceKHR		(VkInstance, const VkXcbSurfaceCreateInfoKHR*)		{}
										SurfaceKHR		(VkInstance, const VkWaylandSurfaceCreateInfoKHR*)	{}
										SurfaceKHR		(VkInstance, const VkMirSurfaceCreateInfoKHR*)		{}
										SurfaceKHR		(VkInstance, const VkAndroidSurfaceCreateInfoKHR*)	{}
										SurfaceKHR		(VkInstance, const VkWin32SurfaceCreateInfoKHR*)	{}
										SurfaceKHR		(VkInstance, const VkDisplaySurfaceCreateInfoKHR*)	{}
										~SurfaceKHR		(void) {}
};

class DisplayModeKHR
{
public:
										DisplayModeKHR	(VkDisplayKHR, const VkDisplayModeCreateInfoKHR*) {}
										~DisplayModeKHR	(void) {}
};

class DebugReportCallbackEXT
{
public:
										DebugReportCallbackEXT	(VkInstance, const VkDebugReportCallbackCreateInfoEXT*) {}
										~DebugReportCallbackEXT	(void) {}
};

class Device
{
public:
										Device			(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* deviceInfo);
										~Device			(void) {}

	PFN_vkVoidFunction					getProcAddr		(const char* name) const { return (PFN_vkVoidFunction)m_functions.getFunction(name); }

private:
	const tcu::StaticFunctionLibrary	m_functions;
};

class Pipeline
{
public:
	Pipeline (VkDevice, const VkGraphicsPipelineCreateInfo*) {}
	Pipeline (VkDevice, const VkComputePipelineCreateInfo*) {}
};

class SwapchainKHR
{
public:
										SwapchainKHR	(VkDevice, const VkSwapchainCreateInfoKHR*) {}
										~SwapchainKHR	(void) {}
};

class SamplerYcbcrConversionKHR
{
public:
	SamplerYcbcrConversionKHR (VkDevice, const VkSamplerYcbcrConversionCreateInfoKHR*) {}
};

void* allocateHeap (const VkMemoryAllocateInfo* pAllocInfo)
{
	// \todo [2015-12-03 pyry] Alignment requirements?
	// \todo [2015-12-03 pyry] Empty allocations okay?
	if (pAllocInfo->allocationSize > 0)
	{
		void* const heapPtr = deMalloc((size_t)pAllocInfo->allocationSize);
		if (!heapPtr)
			throw std::bad_alloc();
		return heapPtr;
	}
	else
		return DE_NULL;
}

void freeHeap (void* ptr)
{
	deFree(ptr);
}

class DeviceMemory
{
public:
						DeviceMemory	(VkDevice, const VkMemoryAllocateInfo* pAllocInfo)
							: m_memory(allocateHeap(pAllocInfo))
						{
							// \todo [2016-08-03 pyry] In some cases leaving data unintialized would help valgrind analysis,
							//						   but currently it mostly hinders it.
							if (m_memory)
								deMemset(m_memory, 0xcd, (size_t)pAllocInfo->allocationSize);
						}
						~DeviceMemory	(void)
						{
							freeHeap(m_memory);
						}

	void*				getPtr			(void) const { return m_memory; }

private:
	void* const			m_memory;
};

class Buffer
{
public:
						Buffer		(VkDevice, const VkBufferCreateInfo* pCreateInfo)
							: m_size(pCreateInfo->size)
						{}

	VkDeviceSize		getSize		(void) const { return m_size;	}

private:
	const VkDeviceSize	m_size;
};

class Image
{
public:
								Image			(VkDevice, const VkImageCreateInfo* pCreateInfo)
									: m_imageType	(pCreateInfo->imageType)
									, m_format		(pCreateInfo->format)
									, m_extent		(pCreateInfo->extent)
									, m_samples		(pCreateInfo->samples)
								{}

	VkImageType					getImageType	(void) const { return m_imageType;	}
	VkFormat					getFormat		(void) const { return m_format;		}
	VkExtent3D					getExtent		(void) const { return m_extent;		}
	VkSampleCountFlagBits		getSamples		(void) const { return m_samples;	}

private:
	const VkImageType			m_imageType;
	const VkFormat				m_format;
	const VkExtent3D			m_extent;
	const VkSampleCountFlagBits	m_samples;
};

class CommandBuffer
{
public:
						CommandBuffer(VkDevice, VkCommandPool, VkCommandBufferLevel)
						{}
};

class DescriptorUpdateTemplateKHR
{
public:
	DescriptorUpdateTemplateKHR (VkDevice, const VkDescriptorUpdateTemplateCreateInfoKHR*) {}
};


class CommandPool
{
public:
										CommandPool		(VkDevice device, const VkCommandPoolCreateInfo*)
											: m_device(device)
										{}
										~CommandPool	(void);

	VkCommandBuffer						allocate		(VkCommandBufferLevel level);
	void								free			(VkCommandBuffer buffer);

private:
	const VkDevice						m_device;

	vector<CommandBuffer*>				m_buffers;
};

CommandPool::~CommandPool (void)
{
	for (size_t ndx = 0; ndx < m_buffers.size(); ++ndx)
		delete m_buffers[ndx];
}

VkCommandBuffer CommandPool::allocate (VkCommandBufferLevel level)
{
	CommandBuffer* const	impl	= new CommandBuffer(m_device, VkCommandPool(reinterpret_cast<deUintptr>(this)), level);

	try
	{
		m_buffers.push_back(impl);
	}
	catch (...)
	{
		delete impl;
		throw;
	}

	return reinterpret_cast<VkCommandBuffer>(impl);
}

void CommandPool::free (VkCommandBuffer buffer)
{
	CommandBuffer* const	impl	= reinterpret_cast<CommandBuffer*>(buffer);

	for (size_t ndx = 0; ndx < m_buffers.size(); ++ndx)
	{
		if (m_buffers[ndx] == impl)
		{
			std::swap(m_buffers[ndx], m_buffers.back());
			m_buffers.pop_back();
			delete impl;
			return;
		}
	}

	DE_FATAL("VkCommandBuffer not owned by VkCommandPool");
}

class DescriptorSet
{
public:
	DescriptorSet (VkDevice, VkDescriptorPool, VkDescriptorSetLayout) {}
};

class DescriptorPool
{
public:
										DescriptorPool	(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo)
											: m_device	(device)
											, m_flags	(pCreateInfo->flags)
										{}
										~DescriptorPool	(void)
										{
											reset();
										}

	VkDescriptorSet						allocate		(VkDescriptorSetLayout setLayout);
	void								free			(VkDescriptorSet set);

	void								reset			(void);

private:
	const VkDevice						m_device;
	const VkDescriptorPoolCreateFlags	m_flags;

	vector<DescriptorSet*>				m_managedSets;
};

VkDescriptorSet DescriptorPool::allocate (VkDescriptorSetLayout setLayout)
{
	DescriptorSet* const	impl	= new DescriptorSet(m_device, VkDescriptorPool(reinterpret_cast<deUintptr>(this)), setLayout);

	try
	{
		m_managedSets.push_back(impl);
	}
	catch (...)
	{
		delete impl;
		throw;
	}

	return VkDescriptorSet(reinterpret_cast<deUintptr>(impl));
}

void DescriptorPool::free (VkDescriptorSet set)
{
	DescriptorSet* const	impl	= reinterpret_cast<DescriptorSet*>((deUintptr)set.getInternal());

	DE_ASSERT(m_flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
	DE_UNREF(m_flags);

	for (size_t ndx = 0; ndx < m_managedSets.size(); ++ndx)
	{
		if (m_managedSets[ndx] == impl)
		{
			std::swap(m_managedSets[ndx], m_managedSets.back());
			m_managedSets.pop_back();
			delete impl;
			return;
		}
	}

	DE_FATAL("VkDescriptorSet not owned by VkDescriptorPool");
}

void DescriptorPool::reset (void)
{
	for (size_t ndx = 0; ndx < m_managedSets.size(); ++ndx)
		delete m_managedSets[ndx];
	m_managedSets.clear();
}

// API implementation

extern "C"
{

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL getDeviceProcAddr (VkDevice device, const char* pName)
{
	return reinterpret_cast<Device*>(device)->getProcAddr(pName);
}

VKAPI_ATTR VkResult VKAPI_CALL createGraphicsPipelines (VkDevice device, VkPipelineCache, deUint32 count, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	deUint32 allocNdx;
	try
	{
		for (allocNdx = 0; allocNdx < count; allocNdx++)
			pPipelines[allocNdx] = allocateNonDispHandle<Pipeline, VkPipeline>(device, pCreateInfos+allocNdx, pAllocator);

		return VK_SUCCESS;
	}
	catch (const std::bad_alloc&)
	{
		for (deUint32 freeNdx = 0; freeNdx < allocNdx; freeNdx++)
			freeNonDispHandle<Pipeline, VkPipeline>(pPipelines[freeNdx], pAllocator);

		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}
	catch (VkResult err)
	{
		for (deUint32 freeNdx = 0; freeNdx < allocNdx; freeNdx++)
			freeNonDispHandle<Pipeline, VkPipeline>(pPipelines[freeNdx], pAllocator);

		return err;
	}
}

VKAPI_ATTR VkResult VKAPI_CALL createComputePipelines (VkDevice device, VkPipelineCache, deUint32 count, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	deUint32 allocNdx;
	try
	{
		for (allocNdx = 0; allocNdx < count; allocNdx++)
			pPipelines[allocNdx] = allocateNonDispHandle<Pipeline, VkPipeline>(device, pCreateInfos+allocNdx, pAllocator);

		return VK_SUCCESS;
	}
	catch (const std::bad_alloc&)
	{
		for (deUint32 freeNdx = 0; freeNdx < allocNdx; freeNdx++)
			freeNonDispHandle<Pipeline, VkPipeline>(pPipelines[freeNdx], pAllocator);

		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}
	catch (VkResult err)
	{
		for (deUint32 freeNdx = 0; freeNdx < allocNdx; freeNdx++)
			freeNonDispHandle<Pipeline, VkPipeline>(pPipelines[freeNdx], pAllocator);

		return err;
	}
}

VKAPI_ATTR VkResult VKAPI_CALL enumeratePhysicalDevices (VkInstance, deUint32* pPhysicalDeviceCount, VkPhysicalDevice* pDevices)
{
	if (pDevices && *pPhysicalDeviceCount >= 1u)
		*pDevices = reinterpret_cast<VkPhysicalDevice>((void*)(deUintptr)1u);

	*pPhysicalDeviceCount = 1;

	return VK_SUCCESS;
}

VkResult enumerateExtensions (deUint32 numExtensions, const VkExtensionProperties* extensions, deUint32* pPropertyCount, VkExtensionProperties* pProperties)
{
	const deUint32	dstSize		= pPropertyCount ? *pPropertyCount : 0;

	if (pPropertyCount)
		*pPropertyCount = numExtensions;

	if (pProperties)
	{
		for (deUint32 ndx = 0; ndx < de::min(numExtensions, dstSize); ++ndx)
			pProperties[ndx] = extensions[ndx];

		if (dstSize < numExtensions)
			return VK_INCOMPLETE;
	}

	return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL enumerateInstanceExtensionProperties (const char* pLayerName, deUint32* pPropertyCount, VkExtensionProperties* pProperties)
{
	static const VkExtensionProperties	s_extensions[]	=
	{
		{ "VK_KHR_get_physical_device_properties2", 1u }
	};

	if (!pLayerName)
		return enumerateExtensions((deUint32)DE_LENGTH_OF_ARRAY(s_extensions), s_extensions, pPropertyCount, pProperties);
	else
		return enumerateExtensions(0, DE_NULL, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL enumerateDeviceExtensionProperties (VkPhysicalDevice physicalDevice, const char* pLayerName, deUint32* pPropertyCount, VkExtensionProperties* pProperties)
{
	DE_UNREF(physicalDevice);

	static const VkExtensionProperties	s_extensions[]	=
	{
		{ "VK_KHR_get_memory_requirements2",	1u },
		{ "VK_KHR_bind_memory2",				1u },
		{ "VK_KHR_sampler_ycbcr_conversion",	1u },
	};

	if (!pLayerName)
		return enumerateExtensions((deUint32)DE_LENGTH_OF_ARRAY(s_extensions), s_extensions, pPropertyCount, pProperties);
	else
		return enumerateExtensions(0, DE_NULL, pPropertyCount, pProperties);
}

VKAPI_ATTR void VKAPI_CALL getPhysicalDeviceFeatures (VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures)
{
	DE_UNREF(physicalDevice);

	// Enable all features allow as many tests to run as possible
	pFeatures->robustBufferAccess							= VK_TRUE;
	pFeatures->fullDrawIndexUint32							= VK_TRUE;
	pFeatures->imageCubeArray								= VK_TRUE;
	pFeatures->independentBlend								= VK_TRUE;
	pFeatures->geometryShader								= VK_TRUE;
	pFeatures->tessellationShader							= VK_TRUE;
	pFeatures->sampleRateShading							= VK_TRUE;
	pFeatures->dualSrcBlend									= VK_TRUE;
	pFeatures->logicOp										= VK_TRUE;
	pFeatures->multiDrawIndirect							= VK_TRUE;
	pFeatures->drawIndirectFirstInstance					= VK_TRUE;
	pFeatures->depthClamp									= VK_TRUE;
	pFeatures->depthBiasClamp								= VK_TRUE;
	pFeatures->fillModeNonSolid								= VK_TRUE;
	pFeatures->depthBounds									= VK_TRUE;
	pFeatures->wideLines									= VK_TRUE;
	pFeatures->largePoints									= VK_TRUE;
	pFeatures->alphaToOne									= VK_TRUE;
	pFeatures->multiViewport								= VK_TRUE;
	pFeatures->samplerAnisotropy							= VK_TRUE;
	pFeatures->textureCompressionETC2						= VK_TRUE;
	pFeatures->textureCompressionASTC_LDR					= VK_TRUE;
	pFeatures->textureCompressionBC							= VK_TRUE;
	pFeatures->occlusionQueryPrecise						= VK_TRUE;
	pFeatures->pipelineStatisticsQuery						= VK_TRUE;
	pFeatures->vertexPipelineStoresAndAtomics				= VK_TRUE;
	pFeatures->fragmentStoresAndAtomics						= VK_TRUE;
	pFeatures->shaderTessellationAndGeometryPointSize		= VK_TRUE;
	pFeatures->shaderImageGatherExtended					= VK_TRUE;
	pFeatures->shaderStorageImageExtendedFormats			= VK_TRUE;
	pFeatures->shaderStorageImageMultisample				= VK_TRUE;
	pFeatures->shaderStorageImageReadWithoutFormat			= VK_TRUE;
	pFeatures->shaderStorageImageWriteWithoutFormat			= VK_TRUE;
	pFeatures->shaderUniformBufferArrayDynamicIndexing		= VK_TRUE;
	pFeatures->shaderSampledImageArrayDynamicIndexing		= VK_TRUE;
	pFeatures->shaderStorageBufferArrayDynamicIndexing		= VK_TRUE;
	pFeatures->shaderStorageImageArrayDynamicIndexing		= VK_TRUE;
	pFeatures->shaderClipDistance							= VK_TRUE;
	pFeatures->shaderCullDistance							= VK_TRUE;
	pFeatures->shaderFloat64								= VK_TRUE;
	pFeatures->shaderInt64									= VK_TRUE;
	pFeatures->shaderInt16									= VK_TRUE;
	pFeatures->shaderResourceResidency						= VK_TRUE;
	pFeatures->shaderResourceMinLod							= VK_TRUE;
	pFeatures->sparseBinding								= VK_TRUE;
	pFeatures->sparseResidencyBuffer						= VK_TRUE;
	pFeatures->sparseResidencyImage2D						= VK_TRUE;
	pFeatures->sparseResidencyImage3D						= VK_TRUE;
	pFeatures->sparseResidency2Samples						= VK_TRUE;
	pFeatures->sparseResidency4Samples						= VK_TRUE;
	pFeatures->sparseResidency8Samples						= VK_TRUE;
	pFeatures->sparseResidency16Samples						= VK_TRUE;
	pFeatures->sparseResidencyAliased						= VK_TRUE;
	pFeatures->variableMultisampleRate						= VK_TRUE;
	pFeatures->inheritedQueries								= VK_TRUE;
}

VKAPI_ATTR void VKAPI_CALL getPhysicalDeviceFeatures2KHR (VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2KHR* pFeatures)
{
	// Core features
	getPhysicalDeviceFeatures(physicalDevice, &pFeatures->features);

	// VK_KHR_sampler_ycbcr_conversion
	{
		VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR*	extFeatures	= findStructure<VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR>(pFeatures->pNext);

		if (extFeatures)
			extFeatures->samplerYcbcrConversion = VK_TRUE;
	}
}

VKAPI_ATTR void VKAPI_CALL getPhysicalDeviceProperties (VkPhysicalDevice, VkPhysicalDeviceProperties* props)
{
	deMemset(props, 0, sizeof(VkPhysicalDeviceProperties));

	props->apiVersion		= VK_API_VERSION;
	props->driverVersion	= 1u;
	props->deviceType		= VK_PHYSICAL_DEVICE_TYPE_OTHER;

	deMemcpy(props->deviceName, "null", 5);

	// Spec minmax
	props->limits.maxImageDimension1D									= 4096;
	props->limits.maxImageDimension2D									= 4096;
	props->limits.maxImageDimension3D									= 256;
	props->limits.maxImageDimensionCube									= 4096;
	props->limits.maxImageArrayLayers									= 256;
	props->limits.maxTexelBufferElements								= 65536;
	props->limits.maxUniformBufferRange									= 16384;
	props->limits.maxStorageBufferRange									= 1u<<27;
	props->limits.maxPushConstantsSize									= 128;
	props->limits.maxMemoryAllocationCount								= 4096;
	props->limits.maxSamplerAllocationCount								= 4000;
	props->limits.bufferImageGranularity								= 131072;
	props->limits.sparseAddressSpaceSize								= 1u<<31;
	props->limits.maxBoundDescriptorSets								= 4;
	props->limits.maxPerStageDescriptorSamplers							= 16;
	props->limits.maxPerStageDescriptorUniformBuffers					= 12;
	props->limits.maxPerStageDescriptorStorageBuffers					= 4;
	props->limits.maxPerStageDescriptorSampledImages					= 16;
	props->limits.maxPerStageDescriptorStorageImages					= 4;
	props->limits.maxPerStageDescriptorInputAttachments					= 4;
	props->limits.maxPerStageResources									= 128;
	props->limits.maxDescriptorSetSamplers								= 96;
	props->limits.maxDescriptorSetUniformBuffers						= 72;
	props->limits.maxDescriptorSetUniformBuffersDynamic					= 8;
	props->limits.maxDescriptorSetStorageBuffers						= 24;
	props->limits.maxDescriptorSetStorageBuffersDynamic					= 4;
	props->limits.maxDescriptorSetSampledImages							= 96;
	props->limits.maxDescriptorSetStorageImages							= 24;
	props->limits.maxDescriptorSetInputAttachments						= 4;
	props->limits.maxVertexInputAttributes								= 16;
	props->limits.maxVertexInputBindings								= 16;
	props->limits.maxVertexInputAttributeOffset							= 2047;
	props->limits.maxVertexInputBindingStride							= 2048;
	props->limits.maxVertexOutputComponents								= 64;
	props->limits.maxTessellationGenerationLevel						= 64;
	props->limits.maxTessellationPatchSize								= 32;
	props->limits.maxTessellationControlPerVertexInputComponents		= 64;
	props->limits.maxTessellationControlPerVertexOutputComponents		= 64;
	props->limits.maxTessellationControlPerPatchOutputComponents		= 120;
	props->limits.maxTessellationControlTotalOutputComponents			= 2048;
	props->limits.maxTessellationEvaluationInputComponents				= 64;
	props->limits.maxTessellationEvaluationOutputComponents				= 64;
	props->limits.maxGeometryShaderInvocations							= 32;
	props->limits.maxGeometryInputComponents							= 64;
	props->limits.maxGeometryOutputComponents							= 64;
	props->limits.maxGeometryOutputVertices								= 256;
	props->limits.maxGeometryTotalOutputComponents						= 1024;
	props->limits.maxFragmentInputComponents							= 64;
	props->limits.maxFragmentOutputAttachments							= 4;
	props->limits.maxFragmentDualSrcAttachments							= 1;
	props->limits.maxFragmentCombinedOutputResources					= 4;
	props->limits.maxComputeSharedMemorySize							= 16384;
	props->limits.maxComputeWorkGroupCount[0]							= 65535;
	props->limits.maxComputeWorkGroupCount[1]							= 65535;
	props->limits.maxComputeWorkGroupCount[2]							= 65535;
	props->limits.maxComputeWorkGroupInvocations						= 128;
	props->limits.maxComputeWorkGroupSize[0]							= 128;
	props->limits.maxComputeWorkGroupSize[1]							= 128;
	props->limits.maxComputeWorkGroupSize[2]							= 128;
	props->limits.subPixelPrecisionBits									= 4;
	props->limits.subTexelPrecisionBits									= 4;
	props->limits.mipmapPrecisionBits									= 4;
	props->limits.maxDrawIndexedIndexValue								= 0xffffffffu;
	props->limits.maxDrawIndirectCount									= (1u<<16) - 1u;
	props->limits.maxSamplerLodBias										= 2.0f;
	props->limits.maxSamplerAnisotropy									= 16.0f;
	props->limits.maxViewports											= 16;
	props->limits.maxViewportDimensions[0]								= 4096;
	props->limits.maxViewportDimensions[1]								= 4096;
	props->limits.viewportBoundsRange[0]								= -8192.f;
	props->limits.viewportBoundsRange[1]								= 8191.f;
	props->limits.viewportSubPixelBits									= 0;
	props->limits.minMemoryMapAlignment									= 64;
	props->limits.minTexelBufferOffsetAlignment							= 256;
	props->limits.minUniformBufferOffsetAlignment						= 256;
	props->limits.minStorageBufferOffsetAlignment						= 256;
	props->limits.minTexelOffset										= -8;
	props->limits.maxTexelOffset										= 7;
	props->limits.minTexelGatherOffset									= -8;
	props->limits.maxTexelGatherOffset									= 7;
	props->limits.minInterpolationOffset								= -0.5f;
	props->limits.maxInterpolationOffset								= 0.5f; // -1ulp
	props->limits.subPixelInterpolationOffsetBits						= 4;
	props->limits.maxFramebufferWidth									= 4096;
	props->limits.maxFramebufferHeight									= 4096;
	props->limits.maxFramebufferLayers									= 256;
	props->limits.framebufferColorSampleCounts							= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;
	props->limits.framebufferDepthSampleCounts							= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;
	props->limits.framebufferStencilSampleCounts						= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;
	props->limits.framebufferNoAttachmentsSampleCounts					= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;
	props->limits.maxColorAttachments									= 4;
	props->limits.sampledImageColorSampleCounts							= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;
	props->limits.sampledImageIntegerSampleCounts						= VK_SAMPLE_COUNT_1_BIT;
	props->limits.sampledImageDepthSampleCounts							= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;
	props->limits.sampledImageStencilSampleCounts						= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;
	props->limits.storageImageSampleCounts								= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;
	props->limits.maxSampleMaskWords									= 1;
	props->limits.timestampComputeAndGraphics							= VK_TRUE;
	props->limits.timestampPeriod										= 1.0f;
	props->limits.maxClipDistances										= 8;
	props->limits.maxCullDistances										= 8;
	props->limits.maxCombinedClipAndCullDistances						= 8;
	props->limits.discreteQueuePriorities								= 2;
	props->limits.pointSizeRange[0]										= 1.0f;
	props->limits.pointSizeRange[1]										= 64.0f; // -1ulp
	props->limits.lineWidthRange[0]										= 1.0f;
	props->limits.lineWidthRange[1]										= 8.0f; // -1ulp
	props->limits.pointSizeGranularity									= 1.0f;
	props->limits.lineWidthGranularity									= 1.0f;
	props->limits.strictLines											= 0;
	props->limits.standardSampleLocations								= VK_TRUE;
	props->limits.optimalBufferCopyOffsetAlignment						= 256;
	props->limits.optimalBufferCopyRowPitchAlignment					= 256;
	props->limits.nonCoherentAtomSize									= 128;
}


VKAPI_ATTR void VKAPI_CALL getPhysicalDeviceProperties2KHR (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2KHR* pProperties)
{
	getPhysicalDeviceProperties(physicalDevice, &pProperties->properties);
}

VKAPI_ATTR void VKAPI_CALL getPhysicalDeviceQueueFamilyProperties (VkPhysicalDevice, deUint32* count, VkQueueFamilyProperties* props)
{
	if (props && *count >= 1u)
	{
		deMemset(props, 0, sizeof(VkQueueFamilyProperties));

		props->queueCount			= 4u;
		props->queueFlags			= VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT;
		props->timestampValidBits	= 64;
	}

	*count = 1u;
}

VKAPI_ATTR void VKAPI_CALL getPhysicalDeviceMemoryProperties (VkPhysicalDevice, VkPhysicalDeviceMemoryProperties* props)
{
	deMemset(props, 0, sizeof(VkPhysicalDeviceMemoryProperties));

	props->memoryTypeCount				= 1u;
	props->memoryTypes[0].heapIndex		= 0u;
	props->memoryTypes[0].propertyFlags	= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
										| VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
										| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	props->memoryHeapCount				= 1u;
	props->memoryHeaps[0].size			= 1ull << 31;
	props->memoryHeaps[0].flags			= 0u;
}

VKAPI_ATTR void VKAPI_CALL getPhysicalDeviceFormatProperties (VkPhysicalDevice, VkFormat format, VkFormatProperties* pFormatProperties)
{
	const VkFormatFeatureFlags	allFeatures	= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
											| VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT
											| VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT
											| VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT
											| VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT
											| VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT
											| VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT
											| VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
											| VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT
											| VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
											| VK_FORMAT_FEATURE_BLIT_SRC_BIT
											| VK_FORMAT_FEATURE_BLIT_DST_BIT
											| VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT
											| VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT_KHR
											| VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT_KHR
											| VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT_KHR
											| VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT_KHR
											| VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT_KHR
											| VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT_KHR;

	pFormatProperties->linearTilingFeatures		= allFeatures;
	pFormatProperties->optimalTilingFeatures	= allFeatures;
	pFormatProperties->bufferFeatures			= allFeatures;

	if (isYCbCrFormat(format) && getPlaneCount(format) > 1)
		pFormatProperties->optimalTilingFeatures |= VK_FORMAT_FEATURE_DISJOINT_BIT_KHR;
}

VKAPI_ATTR VkResult VKAPI_CALL getPhysicalDeviceImageFormatProperties (VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
	DE_UNREF(physicalDevice);
	DE_UNREF(format);
	DE_UNREF(type);
	DE_UNREF(tiling);
	DE_UNREF(usage);
	DE_UNREF(flags);

	pImageFormatProperties->maxArrayLayers		= 8;
	pImageFormatProperties->maxExtent.width		= 4096;
	pImageFormatProperties->maxExtent.height	= 4096;
	pImageFormatProperties->maxExtent.depth		= 4096;
	pImageFormatProperties->maxMipLevels		= deLog2Ceil32(4096) + 1;
	pImageFormatProperties->maxResourceSize		= 64u * 1024u * 1024u;
	pImageFormatProperties->sampleCounts		= VK_SAMPLE_COUNT_1_BIT|VK_SAMPLE_COUNT_4_BIT;

	return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL getDeviceQueue (VkDevice device, deUint32 queueFamilyIndex, deUint32 queueIndex, VkQueue* pQueue)
{
	DE_UNREF(device);
	DE_UNREF(queueFamilyIndex);

	if (pQueue)
		*pQueue = reinterpret_cast<VkQueue>((deUint64)queueIndex + 1);
}

VKAPI_ATTR void VKAPI_CALL getBufferMemoryRequirements (VkDevice, VkBuffer bufferHandle, VkMemoryRequirements* requirements)
{
	const Buffer*	buffer	= reinterpret_cast<const Buffer*>(bufferHandle.getInternal());

	requirements->memoryTypeBits	= 1u;
	requirements->size				= buffer->getSize();
	requirements->alignment			= (VkDeviceSize)1u;
}

VKAPI_ATTR void VKAPI_CALL getBufferMemoryRequirements2KHR (VkDevice device, const VkBufferMemoryRequirementsInfo2KHR* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements)
{
	getBufferMemoryRequirements(device, pInfo->buffer, &pMemoryRequirements->memoryRequirements);
}

VkDeviceSize getPackedImageDataSize (VkFormat format, VkExtent3D extent, VkSampleCountFlagBits samples)
{
	return (VkDeviceSize)getPixelSize(mapVkFormat(format))
			* (VkDeviceSize)extent.width
			* (VkDeviceSize)extent.height
			* (VkDeviceSize)extent.depth
			* (VkDeviceSize)samples;
}

VkDeviceSize getCompressedImageDataSize (VkFormat format, VkExtent3D extent)
{
	try
	{
		const tcu::CompressedTexFormat	tcuFormat		= mapVkCompressedFormat(format);
		const size_t					blockSize		= tcu::getBlockSize(tcuFormat);
		const tcu::IVec3				blockPixelSize	= tcu::getBlockPixelSize(tcuFormat);
		const int						numBlocksX		= deDivRoundUp32((int)extent.width, blockPixelSize.x());
		const int						numBlocksY		= deDivRoundUp32((int)extent.height, blockPixelSize.y());
		const int						numBlocksZ		= deDivRoundUp32((int)extent.depth, blockPixelSize.z());

		return blockSize*numBlocksX*numBlocksY*numBlocksZ;
	}
	catch (...)
	{
		return 0; // Unsupported compressed format
	}
}

VkDeviceSize getYCbCrImageDataSize (VkFormat format, VkExtent3D extent)
{
	const PlanarFormatDescription	desc		= getPlanarFormatDescription(format);
	VkDeviceSize					totalSize	= 0;

	DE_ASSERT(extent.depth == 1);

	for (deUint32 planeNdx = 0; planeNdx < desc.numPlanes; ++planeNdx)
	{
		const deUint32		planeW		= extent.width / desc.planes[planeNdx].widthDivisor;
		const deUint32		planeH		= extent.height / desc.planes[planeNdx].heightDivisor;
		const deUint32		elementSize	= desc.planes[planeNdx].elementSizeBytes;

		totalSize = (VkDeviceSize)deAlign64((deInt64)totalSize, elementSize);
		totalSize += planeW * planeH * elementSize;
	}

	return totalSize;
}

VKAPI_ATTR void VKAPI_CALL getImageMemoryRequirements (VkDevice, VkImage imageHandle, VkMemoryRequirements* requirements)
{
	const Image*	image	= reinterpret_cast<const Image*>(imageHandle.getInternal());

	requirements->memoryTypeBits	= 1u;
	requirements->alignment			= 16u;

	if (isCompressedFormat(image->getFormat()))
		requirements->size = getCompressedImageDataSize(image->getFormat(), image->getExtent());
	else if (isYCbCrFormat(image->getFormat()))
		requirements->size = getYCbCrImageDataSize(image->getFormat(), image->getExtent());
	else
		requirements->size = getPackedImageDataSize(image->getFormat(), image->getExtent(), image->getSamples());
}

VKAPI_ATTR void VKAPI_CALL getImageMemoryRequirements2KHR (VkDevice device, const VkImageMemoryRequirementsInfo2KHR* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements)
{
	const VkImagePlaneMemoryRequirementsInfoKHR*	planeReqs	= findStructure<VkImagePlaneMemoryRequirementsInfoKHR>(pInfo->pNext);

	if (planeReqs)
	{
		const deUint32					planeNdx	= getAspectPlaneNdx(planeReqs->planeAspect);
		const Image*					image		= reinterpret_cast<const Image*>(pInfo->image.getInternal());
		const VkFormat					format		= image->getFormat();
		const PlanarFormatDescription	formatDesc	= getPlanarFormatDescription(format);

		DE_ASSERT(de::inBounds<deUint32>(planeNdx, 0u, formatDesc.numPlanes));

		const VkExtent3D				extent		= image->getExtent();
		const deUint32					planeW		= extent.width / formatDesc.planes[planeNdx].widthDivisor;
		const deUint32					planeH		= extent.height / formatDesc.planes[planeNdx].heightDivisor;
		const deUint32					elementSize	= formatDesc.planes[planeNdx].elementSizeBytes;

		pMemoryRequirements->memoryRequirements.memoryTypeBits	= 1u;
		pMemoryRequirements->memoryRequirements.alignment		= 16u;
		pMemoryRequirements->memoryRequirements.size			= planeW * planeH * elementSize;
	}
	else
		getImageMemoryRequirements(device, pInfo->image, &pMemoryRequirements->memoryRequirements);
}

VKAPI_ATTR VkResult VKAPI_CALL mapMemory (VkDevice, VkDeviceMemory memHandle, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
	const DeviceMemory*	memory	= reinterpret_cast<DeviceMemory*>(memHandle.getInternal());

	DE_UNREF(size);
	DE_UNREF(flags);

	*ppData = (deUint8*)memory->getPtr() + offset;

	return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL allocateDescriptorSets (VkDevice, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
	DescriptorPool* const	poolImpl	= reinterpret_cast<DescriptorPool*>((deUintptr)pAllocateInfo->descriptorPool.getInternal());

	for (deUint32 ndx = 0; ndx < pAllocateInfo->descriptorSetCount; ++ndx)
	{
		try
		{
			pDescriptorSets[ndx] = poolImpl->allocate(pAllocateInfo->pSetLayouts[ndx]);
		}
		catch (const std::bad_alloc&)
		{
			for (deUint32 freeNdx = 0; freeNdx < ndx; freeNdx++)
				delete reinterpret_cast<DescriptorSet*>((deUintptr)pDescriptorSets[freeNdx].getInternal());

			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}
		catch (VkResult res)
		{
			for (deUint32 freeNdx = 0; freeNdx < ndx; freeNdx++)
				delete reinterpret_cast<DescriptorSet*>((deUintptr)pDescriptorSets[freeNdx].getInternal());

			return res;
		}
	}

	return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL freeDescriptorSets (VkDevice, VkDescriptorPool descriptorPool, deUint32 count, const VkDescriptorSet* pDescriptorSets)
{
	DescriptorPool* const	poolImpl	= reinterpret_cast<DescriptorPool*>((deUintptr)descriptorPool.getInternal());

	for (deUint32 ndx = 0; ndx < count; ++ndx)
		poolImpl->free(pDescriptorSets[ndx]);
}

VKAPI_ATTR VkResult VKAPI_CALL resetDescriptorPool (VkDevice, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags)
{
	DescriptorPool* const	poolImpl	= reinterpret_cast<DescriptorPool*>((deUintptr)descriptorPool.getInternal());

	poolImpl->reset();

	return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL allocateCommandBuffers (VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
	DE_UNREF(device);

	if (pAllocateInfo && pCommandBuffers)
	{
		CommandPool* const	poolImpl	= reinterpret_cast<CommandPool*>((deUintptr)pAllocateInfo->commandPool.getInternal());

		for (deUint32 ndx = 0; ndx < pAllocateInfo->commandBufferCount; ++ndx)
			pCommandBuffers[ndx] = poolImpl->allocate(pAllocateInfo->level);
	}

	return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL freeCommandBuffers (VkDevice device, VkCommandPool commandPool, deUint32 commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
	CommandPool* const	poolImpl	= reinterpret_cast<CommandPool*>((deUintptr)commandPool.getInternal());

	DE_UNREF(device);

	for (deUint32 ndx = 0; ndx < commandBufferCount; ++ndx)
		poolImpl->free(pCommandBuffers[ndx]);
}


VKAPI_ATTR VkResult VKAPI_CALL createDisplayModeKHR (VkPhysicalDevice, VkDisplayKHR display, const VkDisplayModeCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDisplayModeKHR* pMode)
{
	DE_UNREF(pAllocator);
	VK_NULL_RETURN((*pMode = allocateNonDispHandle<DisplayModeKHR, VkDisplayModeKHR>(display, pCreateInfo, pAllocator)));
}

VKAPI_ATTR VkResult VKAPI_CALL createSharedSwapchainsKHR (VkDevice device, deUint32 swapchainCount, const VkSwapchainCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchains)
{
	for (deUint32 ndx = 0; ndx < swapchainCount; ++ndx)
	{
		pSwapchains[ndx] = allocateNonDispHandle<SwapchainKHR, VkSwapchainKHR>(device, pCreateInfos+ndx, pAllocator);
	}

	return VK_SUCCESS;
}

// \note getInstanceProcAddr is a little bit special:
// vkNullDriverImpl.inl needs it to define s_platformFunctions but
// getInstanceProcAddr() implementation needs other entry points from
// vkNullDriverImpl.inl.
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL getInstanceProcAddr (VkInstance instance, const char* pName);

#include "vkNullDriverImpl.inl"

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL getInstanceProcAddr (VkInstance instance, const char* pName)
{
	if (instance)
	{
		return reinterpret_cast<Instance*>(instance)->getProcAddr(pName);
	}
	else
	{
		const std::string	name	= pName;

		if (name == "vkCreateInstance")
			return (PFN_vkVoidFunction)createInstance;
		else if (name == "vkEnumerateInstanceExtensionProperties")
			return (PFN_vkVoidFunction)enumerateInstanceExtensionProperties;
		else if (name == "vkEnumerateInstanceLayerProperties")
			return (PFN_vkVoidFunction)enumerateInstanceLayerProperties;
		else
			return (PFN_vkVoidFunction)DE_NULL;
	}
}

} // extern "C"

Instance::Instance (const VkInstanceCreateInfo*)
	: m_functions(s_instanceFunctions, DE_LENGTH_OF_ARRAY(s_instanceFunctions))
{
}

Device::Device (VkPhysicalDevice, const VkDeviceCreateInfo*)
	: m_functions(s_deviceFunctions, DE_LENGTH_OF_ARRAY(s_deviceFunctions))
{
}

class NullDriverLibrary : public Library
{
public:
										NullDriverLibrary (void)
											: m_library	(s_platformFunctions, DE_LENGTH_OF_ARRAY(s_platformFunctions))
											, m_driver	(m_library)
										{}

	const PlatformInterface&			getPlatformInterface	(void) const { return m_driver;	}

private:
	const tcu::StaticFunctionLibrary	m_library;
	const PlatformDriver				m_driver;
};

} // anonymous

Library* createNullDriver (void)
{
	return new NullDriverLibrary();
}

} // vk
