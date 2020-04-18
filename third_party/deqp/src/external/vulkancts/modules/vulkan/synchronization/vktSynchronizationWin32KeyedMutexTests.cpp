/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \brief Synchronization tests for resources shared with DX11 keyed mutex
 *//*--------------------------------------------------------------------*/

#include "vktSynchronizationWin32KeyedMutexTests.hpp"

#include "vkDeviceUtil.hpp"
#include "vkPlatform.hpp"

#include "vktTestCaseUtil.hpp"

#include "vktSynchronizationUtil.hpp"
#include "vktSynchronizationOperation.hpp"
#include "vktSynchronizationOperationTestData.hpp"
#include "vktExternalMemoryUtil.hpp"

#include "tcuResultCollector.hpp"
#include "tcuTestLog.hpp"

#if (DE_OS == DE_OS_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	include <windows.h>
#	include <aclapi.h>
#	include "VersionHelpers.h"
#	include "d3d11_2.h"
#	include "d3dcompiler.h"

typedef HRESULT				(WINAPI * LPD3DX11COMPILEFROMMEMORY)(LPCSTR,
																 SIZE_T,
																 LPCSTR,
																 CONST D3D10_SHADER_MACRO*,
																 LPD3D10INCLUDE,
																 LPCSTR,
																 LPCSTR,
																 UINT,
																 UINT,
																 void*, /* ID3DX11ThreadPump */
																 ID3D10Blob** ,
																 ID3D10Blob** ,
																 HRESULT*);
#endif

using tcu::TestLog;
using namespace vkt::ExternalMemoryUtil;

namespace vkt
{
namespace synchronization
{
namespace
{

static const ResourceDescription s_resourcesWin32KeyedMutex[] =
{
	{ RESOURCE_TYPE_BUFFER,	tcu::IVec4( 0x4000, 0, 0, 0),	vk::VK_IMAGE_TYPE_LAST,	vk::VK_FORMAT_UNDEFINED,			(vk::VkImageAspectFlags)0	  },	// 16 KiB (min max UBO range)
	{ RESOURCE_TYPE_BUFFER,	tcu::IVec4(0x40000, 0, 0, 0),	vk::VK_IMAGE_TYPE_LAST,	vk::VK_FORMAT_UNDEFINED,			(vk::VkImageAspectFlags)0	  },	// 256 KiB

	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R8_UNORM,				vk::VK_IMAGE_ASPECT_COLOR_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R16_UINT,				vk::VK_IMAGE_ASPECT_COLOR_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R8G8B8A8_UNORM,		vk::VK_IMAGE_ASPECT_COLOR_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R16G16B16A16_UINT,	vk::VK_IMAGE_ASPECT_COLOR_BIT },
	{ RESOURCE_TYPE_IMAGE,	tcu::IVec4(128, 128, 0, 0),		vk::VK_IMAGE_TYPE_2D,	vk::VK_FORMAT_R32G32B32A32_SFLOAT,	vk::VK_IMAGE_ASPECT_COLOR_BIT },
};

struct TestConfig
{
								TestConfig		(const ResourceDescription&						resource_,
												 OperationName									writeOp_,
												 OperationName									readOp_,
												 vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleTypeBuffer_,
												 vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleTypeImage_)
		: resource					(resource_)
		, writeOp					(writeOp_)
		, readOp					(readOp_)
		, memoryHandleTypeBuffer	(memoryHandleTypeBuffer_)
		, memoryHandleTypeImage		(memoryHandleTypeImage_)
	{
	}

	const ResourceDescription							resource;
	const OperationName									writeOp;
	const OperationName									readOp;
	const vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleTypeBuffer;
	const vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleTypeImage;
};

bool checkQueueFlags (vk::VkQueueFlags availableFlags, const vk::VkQueueFlags neededFlags)
{
	if ((availableFlags & (vk::VK_QUEUE_GRAPHICS_BIT | vk::VK_QUEUE_COMPUTE_BIT)) != 0)
		availableFlags |= vk::VK_QUEUE_TRANSFER_BIT;

	return (availableFlags & neededFlags) != 0;
}

class SimpleAllocation : public vk::Allocation
{
public:
								SimpleAllocation	(const vk::DeviceInterface&	vkd,
													 vk::VkDevice				device,
													 const vk::VkDeviceMemory	memory);
								~SimpleAllocation	(void);

private:
	const vk::DeviceInterface&	m_vkd;
	const vk::VkDevice			m_device;
};

SimpleAllocation::SimpleAllocation (const vk::DeviceInterface&	vkd,
									vk::VkDevice				device,
									const vk::VkDeviceMemory	memory)
	: Allocation	(memory, 0, DE_NULL)
	, m_vkd			(vkd)
	, m_device		(device)
{
}

SimpleAllocation::~SimpleAllocation (void)
{
	m_vkd.freeMemory(m_device, getMemory(), DE_NULL);
}

vk::Move<vk::VkInstance> createInstance (const vk::PlatformInterface& vkp)
{
	try
	{
		std::vector<std::string> extensions;

		extensions.push_back("VK_KHR_get_physical_device_properties2");

		extensions.push_back("VK_KHR_external_memory_capabilities");

		return vk::createDefaultInstance(vkp, std::vector<std::string>(), extensions);
	}
	catch (const vk::Error& error)
	{
		if (error.getError() == vk::VK_ERROR_EXTENSION_NOT_PRESENT)
			TCU_THROW(NotSupportedError, "Required external memory extensions not supported by the instance");
		else
			throw;
	}
}

vk::VkPhysicalDevice getPhysicalDevice (const vk::InstanceInterface&	vki,
										vk::VkInstance					instance,
										const tcu::CommandLine&			cmdLine)
{
	return vk::chooseDevice(vki, instance, cmdLine);
}

vk::Move<vk::VkDevice> createDevice (const vk::InstanceInterface&					vki,
									 vk::VkPhysicalDevice							physicalDevice)
{
	const float										priority				= 0.0f;
	const std::vector<vk::VkQueueFamilyProperties>	queueFamilyProperties	= vk::getPhysicalDeviceQueueFamilyProperties(vki, physicalDevice);
	std::vector<deUint32>							queueFamilyIndices		(queueFamilyProperties.size(), 0xFFFFFFFFu);
	std::vector<const char*>						extensions;

	extensions.push_back("VK_KHR_external_memory");
	extensions.push_back("VK_KHR_external_memory_win32");
	extensions.push_back("VK_KHR_win32_keyed_mutex");
	extensions.push_back("VK_KHR_dedicated_allocation");
	extensions.push_back("VK_KHR_get_memory_requirements2");

	try
	{
		std::vector<vk::VkDeviceQueueCreateInfo>	queues;

		for (size_t ndx = 0; ndx < queueFamilyProperties.size(); ndx++)
		{
			const vk::VkDeviceQueueCreateInfo	createInfo	=
			{
				vk::VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				DE_NULL,
				0u,

				(deUint32)ndx,
				1u,
				&priority
			};

			queues.push_back(createInfo);
		}

		const vk::VkDeviceCreateInfo		createInfo			=
		{
			vk::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			DE_NULL,
			0u,

			(deUint32)queues.size(),
			&queues[0],

			0u,
			DE_NULL,

			(deUint32)extensions.size(),
			extensions.empty() ? DE_NULL : &extensions[0],
			0u
		};

		return vk::createDevice(vki, physicalDevice, &createInfo);
	}
	catch (const vk::Error& error)
	{
		if (error.getError() == vk::VK_ERROR_EXTENSION_NOT_PRESENT)
			TCU_THROW(NotSupportedError, "Required extensions not supported");
		else
			throw;
	}
}

deUint32 chooseMemoryType (deUint32 bits)
{
	DE_ASSERT(bits != 0);

	for (deUint32 memoryTypeIndex = 0; (1u << memoryTypeIndex) <= bits; memoryTypeIndex++)
	{
		if ((bits & (1u << memoryTypeIndex)) != 0)
			return memoryTypeIndex;
	}

	DE_FATAL("No supported memory types");
	return -1;
}

vk::Move<vk::VkDeviceMemory> importMemory (const vk::DeviceInterface&					vkd,
										   vk::VkDevice									device,
										   const vk::VkMemoryRequirements&				requirements,
										   vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType,
										   NativeHandle&								handle,
										   bool											requiresDedicated,
										   vk::VkBuffer									buffer,
										   vk::VkImage									image)
{
	const vk::VkMemoryDedicatedAllocateInfoKHR	dedicatedInfo	=
	{
		vk::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
		DE_NULL,
		image,
		buffer,
	};
	const vk::VkImportMemoryWin32HandleInfoKHR	importInfo		=
	{
		vk::VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
		(requiresDedicated) ? &dedicatedInfo : DE_NULL,
		externalType,
		handle.getWin32Handle(),
        NULL
	};

	const vk::VkMemoryAllocateInfo				info			=
	{
		vk::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		&importInfo,
		requirements.size,
		chooseMemoryType(requirements.memoryTypeBits)
	};

	vk::Move<vk::VkDeviceMemory> memory (vk::allocateMemory(vkd, device, &info));

	handle.disown();

	return memory;
}

de::MovePtr<vk::Allocation> importAndBindMemory (const vk::DeviceInterface&					vkd,
												 vk::VkDevice								device,
												 vk::VkBuffer								buffer,
												 NativeHandle&								nativeHandle,
												 vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType)
{
	const vk::VkBufferMemoryRequirementsInfo2KHR	requirementsInfo		=
	{
		vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR,
		DE_NULL,
		buffer,
	};
	vk::VkMemoryDedicatedRequirementsKHR			dedicatedRequirements	=
	{
		vk::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,
		DE_NULL,
		VK_FALSE,
		VK_FALSE,
	};
	vk::VkMemoryRequirements2KHR					requirements			=
	{
		vk::VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR,
		&dedicatedRequirements,
		{ 0u, 0u, 0u, },
	};
	vkd.getBufferMemoryRequirements2KHR(device, &requirementsInfo, &requirements);

	vk::Move<vk::VkDeviceMemory> memory = importMemory(vkd, device, requirements.memoryRequirements, externalType, nativeHandle, !!dedicatedRequirements.requiresDedicatedAllocation, buffer, DE_NULL);
	VK_CHECK(vkd.bindBufferMemory(device, buffer, *memory, 0u));

	return de::MovePtr<vk::Allocation>(new SimpleAllocation(vkd, device, memory.disown()));
}

de::MovePtr<vk::Allocation> importAndBindMemory (const vk::DeviceInterface&						vkd,
												   vk::VkDevice									device,
												   vk::VkImage									image,
												   NativeHandle&								nativeHandle,
												   vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType)
{
	const vk::VkImageMemoryRequirementsInfo2KHR		requirementsInfo		=
	{
		vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR,
		DE_NULL,
		image,
	};
	vk::VkMemoryDedicatedRequirementsKHR			dedicatedRequirements	=
	{
		vk::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,
		DE_NULL,
		VK_FALSE,
		VK_FALSE,
	};
	vk::VkMemoryRequirements2KHR					requirements			=
	{
		vk::VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR,
		&dedicatedRequirements,
		{ 0u, 0u, 0u, },
	};
	vkd.getImageMemoryRequirements2KHR(device, &requirementsInfo, &requirements);

	vk::Move<vk::VkDeviceMemory> memory = importMemory(vkd, device, requirements.memoryRequirements, externalType, nativeHandle, !!dedicatedRequirements.requiresDedicatedAllocation, DE_NULL, image);
	VK_CHECK(vkd.bindImageMemory(device, image, *memory, 0u));

	return de::MovePtr<vk::Allocation>(new SimpleAllocation(vkd, device, memory.disown()));
}

de::MovePtr<Resource> importResource (const vk::DeviceInterface&				vkd,
									  vk::VkDevice								device,
									  const ResourceDescription&				resourceDesc,
									  const std::vector<deUint32>&				queueFamilyIndices,
									  const OperationSupport&					readOp,
									  const OperationSupport&					writeOp,
									  NativeHandle&								nativeHandle,
									  vk::VkExternalMemoryHandleTypeFlagBitsKHR	externalType)
{
	if (resourceDesc.type == RESOURCE_TYPE_IMAGE)
	{
		const vk::VkExtent3D								extent					=
		{
			(deUint32)resourceDesc.size.x(),
			de::max(1u, (deUint32)resourceDesc.size.y()),
			de::max(1u, (deUint32)resourceDesc.size.z())
		};
		const vk::VkImageSubresourceRange					subresourceRange		=
		{
			resourceDesc.imageAspect,
			0u,
			1u,
			0u,
			1u
		};
		const vk::VkImageSubresourceLayers					subresourceLayers		=
		{
			resourceDesc.imageAspect,
			0u,
			0u,
			1u
		};
		const vk::VkExternalMemoryImageCreateInfoKHR		externalInfo			=
		{
			vk::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
			DE_NULL,
			(vk::VkExternalMemoryHandleTypeFlagsKHR)externalType
		};
		const vk::VkImageCreateInfo			createInfo				=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			&externalInfo,
			0u,

			resourceDesc.imageType,
			resourceDesc.imageFormat,
			extent,
			1u,
			1u,
			vk::VK_SAMPLE_COUNT_1_BIT,
			vk::VK_IMAGE_TILING_OPTIMAL,
			readOp.getResourceUsageFlags() | writeOp.getResourceUsageFlags(),
			vk::VK_SHARING_MODE_EXCLUSIVE,

			(deUint32)queueFamilyIndices.size(),
			&queueFamilyIndices[0],
			vk::VK_IMAGE_LAYOUT_UNDEFINED
		};

		vk::Move<vk::VkImage>			image		= vk::createImage(vkd, device, &createInfo);
		de::MovePtr<vk::Allocation>		allocation	= importAndBindMemory(vkd, device, *image, nativeHandle, externalType);

		return de::MovePtr<Resource>(new Resource(image, allocation, extent, resourceDesc.imageType, resourceDesc.imageFormat, subresourceRange, subresourceLayers));
	}
	else
	{
		const vk::VkDeviceSize							offset			= 0u;
		const vk::VkDeviceSize							size			= static_cast<vk::VkDeviceSize>(resourceDesc.size.x());
		const vk::VkBufferUsageFlags					usage			= readOp.getResourceUsageFlags() | writeOp.getResourceUsageFlags();
		const vk::VkExternalMemoryBufferCreateInfoKHR	externalInfo	=
		{
			vk::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHR,
			DE_NULL,
			(vk::VkExternalMemoryHandleTypeFlagsKHR)externalType
		};
		const vk::VkBufferCreateInfo					createInfo		=
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			&externalInfo,
			0u,

			size,
			usage,
			vk::VK_SHARING_MODE_EXCLUSIVE,
			(deUint32)queueFamilyIndices.size(),
			&queueFamilyIndices[0]
		};
		vk::Move<vk::VkBuffer>		buffer		= vk::createBuffer(vkd, device, &createInfo);
		de::MovePtr<vk::Allocation>	allocation	= importAndBindMemory(vkd, device, *buffer, nativeHandle, externalType);

		return de::MovePtr<Resource>(new Resource(resourceDesc.type, buffer, allocation, offset, size));
	}
}

void recordWriteBarrier (const vk::DeviceInterface&	vkd,
						 vk::VkCommandBuffer		commandBuffer,
						 const Resource&			resource,
						 const SyncInfo&			writeSync,
						 deUint32					writeQueueFamilyIndex,
						 const SyncInfo&			readSync)
{
	const vk::VkPipelineStageFlags	srcStageMask		= writeSync.stageMask;
	const vk::VkAccessFlags			srcAccessMask		= writeSync.accessMask;

	const vk::VkPipelineStageFlags	dstStageMask		= readSync.stageMask;
	const vk::VkAccessFlags			dstAccessMask		= readSync.accessMask;

	const vk::VkDependencyFlags		dependencyFlags		= 0;

	if (resource.getType() == RESOURCE_TYPE_IMAGE)
	{
		const vk::VkImageMemoryBarrier	barrier				=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			srcAccessMask,
			dstAccessMask,

			writeSync.imageLayout,
			readSync.imageLayout,

			writeQueueFamilyIndex,
			VK_QUEUE_FAMILY_EXTERNAL_KHR,

			resource.getImage().handle,
			resource.getImage().subresourceRange
		};

		vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, 0u, (const vk::VkMemoryBarrier*)DE_NULL, 0u, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1u, (const vk::VkImageMemoryBarrier*)&barrier);
	}
	else
	{
		const vk::VkBufferMemoryBarrier	barrier				=
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,

			srcAccessMask,
			dstAccessMask,

			writeQueueFamilyIndex,
			VK_QUEUE_FAMILY_EXTERNAL_KHR,

			resource.getBuffer().handle,
			0u,
			VK_WHOLE_SIZE
		};

		vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, 0u, (const vk::VkMemoryBarrier*)DE_NULL, 1u, (const vk::VkBufferMemoryBarrier*)&barrier, 0u, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}
}

void recordReadBarrier (const vk::DeviceInterface&	vkd,
						vk::VkCommandBuffer			commandBuffer,
						const Resource&				resource,
						const SyncInfo&				writeSync,
						const SyncInfo&				readSync,
						deUint32					readQueueFamilyIndex)
{
	const vk::VkPipelineStageFlags	srcStageMask		= readSync.stageMask;
	const vk::VkAccessFlags			srcAccessMask		= readSync.accessMask;

	const vk::VkPipelineStageFlags	dstStageMask		= readSync.stageMask;
	const vk::VkAccessFlags			dstAccessMask		= readSync.accessMask;

	const vk::VkDependencyFlags		dependencyFlags		= 0;

	if (resource.getType() == RESOURCE_TYPE_IMAGE)
	{
		const vk::VkImageMemoryBarrier	barrier				=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			srcAccessMask,
			dstAccessMask,

			writeSync.imageLayout,
			readSync.imageLayout,

			VK_QUEUE_FAMILY_EXTERNAL_KHR,
			readQueueFamilyIndex,

			resource.getImage().handle,
			resource.getImage().subresourceRange
		};

		vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, 0u, (const vk::VkMemoryBarrier*)DE_NULL, 0u, (const vk::VkBufferMemoryBarrier*)DE_NULL, 1u, (const vk::VkImageMemoryBarrier*)&barrier);
	}
	else
	{
		const vk::VkBufferMemoryBarrier	barrier				=
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,

			srcAccessMask,
			dstAccessMask,

			VK_QUEUE_FAMILY_EXTERNAL_KHR,
			readQueueFamilyIndex,

			resource.getBuffer().handle,
			0u,
			VK_WHOLE_SIZE
		};

		vkd.cmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, 0u, (const vk::VkMemoryBarrier*)DE_NULL, 1u, (const vk::VkBufferMemoryBarrier*)&barrier, 0u, (const vk::VkImageMemoryBarrier*)DE_NULL);
	}
}

std::vector<deUint32> getFamilyIndices (const std::vector<vk::VkQueueFamilyProperties>& properties)
{
	std::vector<deUint32> indices (properties.size(), 0);

	for (deUint32 ndx = 0; ndx < properties.size(); ndx++)
		indices[ndx] = ndx;

	return indices;
}

class DX11Operation
{
public:
	enum Buffer
	{
		BUFFER_VK_WRITE,
		BUFFER_VK_READ,
		BUFFER_COUNT,
	};

	enum KeyedMutex
	{
		KEYED_MUTEX_INIT		= 0,
		KEYED_MUTEX_VK_WRITE	= 1,
		KEYED_MUTEX_DX_COPY		= 2,
		KEYED_MUTEX_VK_VERIFY	= 3,
		KEYED_MUTEX_DONE		= 4,
	};

#if (DE_OS == DE_OS_WIN32)
	DX11Operation (const ResourceDescription&					resourceDesc,
				   vk::VkExternalMemoryHandleTypeFlagBitsKHR	memoryHandleType,
				   ID3D11Device*								pDevice,
				   ID3D11DeviceContext*							pContext,
				   LPD3DX11COMPILEFROMMEMORY					fnD3DX11CompileFromMemory,
				   pD3DCompile									fnD3DCompile)
		: m_resourceDesc				(resourceDesc)

		, m_pDevice						(pDevice)
		, m_pContext					(pContext)
		, m_fnD3DX11CompileFromMemory	(fnD3DX11CompileFromMemory)
		, m_fnD3DCompile				(fnD3DCompile)

		, m_pRenderTargetView			(0)
		, m_pVertexShader				(0)
		, m_pPixelShader				(0)
		, m_pVertexBuffer				(0)
		, m_pTextureRV					(0)
		, m_pSamplerLinear				(0)
		, m_numFrames					(0)
	{
		HRESULT	hr;

		if (memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR ||
			memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT_KHR)

			m_isMemNtHandle = true;
		else
			m_isMemNtHandle = false;

		m_securityAttributes.lpSecurityDescriptor = 0;

		for (UINT i = 0; i < BUFFER_COUNT; i++)
		{
			m_pTexture[i] = NULL;
			m_pBuffer[i] = NULL;
			m_keyedMutex[i] = NULL;
		}

		if (m_resourceDesc.type == RESOURCE_TYPE_BUFFER)
		{
			// SHARED_NTHANDLE is not supported with CreateBuffer().
			TCU_CHECK_INTERNAL(!m_isMemNtHandle);

			D3D11_BUFFER_DESC descBuf = { };
			descBuf.ByteWidth = (UINT)m_resourceDesc.size.x();
			descBuf.Usage = D3D11_USAGE_DEFAULT;
			descBuf.BindFlags = 0;
			descBuf.CPUAccessFlags = 0;
			descBuf.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
			descBuf.StructureByteStride = 0;

			for (UINT i = 0; i < BUFFER_COUNT; ++i)
			{
				hr = m_pDevice->CreateBuffer(&descBuf, NULL, &m_pBuffer[i]);
				if (FAILED(hr))
					TCU_FAIL("Failed to create a buffer");

				m_sharedMemHandle[i] = 0;

				IDXGIResource* tempResource = NULL;
				hr = m_pBuffer[i]->QueryInterface(__uuidof(IDXGIResource), (void**)&tempResource);
				if (FAILED(hr))
					TCU_FAIL("Query interface of IDXGIResource failed");
				hr = tempResource->GetSharedHandle(&m_sharedMemHandle[i]);
				tempResource->Release();
				if (FAILED(hr))
					TCU_FAIL("Failed to get DX shared handle");

				hr = m_pBuffer[i]->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&m_keyedMutex[i]);
				if (FAILED(hr))
					TCU_FAIL("Query interface of IDXGIKeyedMutex failed");

				// Take ownership of the lock.
				m_keyedMutex[i]->AcquireSync(KEYED_MUTEX_INIT, INFINITE);
			}

			// Release the buffer write lock for Vulkan to write into.
			m_keyedMutex[BUFFER_VK_WRITE]->ReleaseSync(KEYED_MUTEX_VK_WRITE);

			m_sharedMemSize = descBuf.ByteWidth;
			m_sharedMemOffset = 0;
		}
		else
		{
			DE_ASSERT(m_resourceDesc.type == RESOURCE_TYPE_IMAGE);

			for (UINT i = 0; i < BUFFER_COUNT; ++i)
			{
				D3D11_TEXTURE2D_DESC descColor = { };
				descColor.Width = m_resourceDesc.size.x();
				descColor.Height = m_resourceDesc.size.y();
				descColor.MipLevels = 1;
				descColor.ArraySize = 1;
				descColor.Format = getDxgiFormat(m_resourceDesc.imageFormat);
				descColor.SampleDesc.Count = 1;
				descColor.SampleDesc.Quality = 0;
				descColor.Usage = D3D11_USAGE_DEFAULT;
				descColor.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				descColor.CPUAccessFlags = 0;

				if (m_isMemNtHandle)
					descColor.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
				else
					descColor.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

				hr = m_pDevice->CreateTexture2D(&descColor, NULL, &m_pTexture[i]);
				if (FAILED(hr))
					TCU_FAIL("Unable to create DX11 texture");

				m_sharedMemHandle[i] = 0;

				if (m_isMemNtHandle)
				{
					IDXGIResource1* tempResource1 = NULL;
					hr = m_pTexture[i]->QueryInterface(__uuidof(IDXGIResource1), (void**)&tempResource1);
					if (FAILED(hr))
						TCU_FAIL("Unable to query IDXGIResource1 interface");

					hr = tempResource1->CreateSharedHandle(getSecurityAttributes(), DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, /*lpName*/NULL, &m_sharedMemHandle[i]);
					tempResource1->Release();
					if (FAILED(hr))
						TCU_FAIL("Enable to get DX shared handle");
				}
				else
				{
					IDXGIResource* tempResource = NULL;
					hr = m_pTexture[i]->QueryInterface(__uuidof(IDXGIResource), (void**)&tempResource);
					if (FAILED(hr))
						TCU_FAIL("Query interface of IDXGIResource failed");
					hr = tempResource->GetSharedHandle(&m_sharedMemHandle[i]);
					tempResource->Release();
					if (FAILED(hr))
						TCU_FAIL("Failed to get DX shared handle");
				}

				hr = m_pTexture[i]->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&m_keyedMutex[i]);
				if (FAILED(hr))
					TCU_FAIL("Unable to query DX11 keyed mutex interface");

				// Take ownership of the lock.
				m_keyedMutex[i]->AcquireSync(KEYED_MUTEX_INIT, INFINITE);
			}

			m_sharedMemSize = 0;
			m_sharedMemOffset = 0;

			hr = m_pDevice->CreateRenderTargetView(m_pTexture[BUFFER_VK_READ], NULL, &m_pRenderTargetView);
			if (FAILED(hr))
				TCU_FAIL("Unable to create DX11 render target view");

			m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);

			// Setup the viewport
			D3D11_VIEWPORT vp;
			vp.Width = (FLOAT)m_resourceDesc.size.x();
			vp.Height = (FLOAT)m_resourceDesc.size.y();
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			m_pContext->RSSetViewports(1, &vp);

			// Compile the vertex shader
			LPCSTR shader =
				"Texture2D txDiffuse : register(t0);\n"
				"SamplerState samLinear : register(s0);\n"
				"struct VS_INPUT\n"
				"{\n"
				"    float4 Pos : POSITION;\n"
				"    float2 Tex : TEXCOORD0;\n"
				"};\n"
				"struct PS_INPUT\n"
				"{\n"
				"    float4 Pos : SV_POSITION;\n"
				"    float2 Tex : TEXCOORD0;\n"
				"};\n"
				"PS_INPUT VS(VS_INPUT input)\n"
				"{\n"
				"    PS_INPUT output = (PS_INPUT)0;\n"
				"    output.Pos = input.Pos;\n"
				"    output.Tex = input.Tex;\n"
				"\n"
				"    return output;\n"
				"}\n"
				"float4 PS(PS_INPUT input) : SV_Target\n"
				"{\n"
				"    return txDiffuse.Sample(samLinear, input.Tex);\n"
				"}\n";

			// Define the input layout
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			createShaders(shader, "VS", "vs_4_0", ARRAYSIZE(layout), layout, &m_pVertexShader, "PS", "ps_4_0", &m_pPixelShader);

			struct SimpleVertex
			{
				float Pos[3];
				float Tex[2];
			};

			SimpleVertex vertices[] =
			{
				{ { -1.f, -1.f, 0.0f }, { 0.0f, 1.0f } },
				{ { -1.f,  1.f, 0.0f }, { 0.0f, 0.0f } },
				{ {  1.f, -1.f, 0.0f }, { 1.0f, 1.0f } },
				{ {  1.f,  1.f, 0.0f }, { 1.0f, 0.0f } },
			};

			D3D11_BUFFER_DESC bd = { };
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof (vertices);
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			D3D11_SUBRESOURCE_DATA InitData = { };
			InitData.pSysMem = vertices;
			hr = m_pDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
			if (FAILED(hr))
				TCU_FAIL("Failed to create DX11 vertex buffer");

			// Set vertex buffer
			UINT stride = sizeof (SimpleVertex);
			UINT offset = 0;
			m_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

			// Set primitive topology
			m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			m_pTextureRV = NULL;

			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = { };
			SRVDesc.Format = getDxgiFormat(m_resourceDesc.imageFormat);
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = 1;

			hr = m_pDevice->CreateShaderResourceView(m_pTexture[BUFFER_VK_WRITE], &SRVDesc, &m_pTextureRV);
			if (FAILED(hr))
				TCU_FAIL("Failed to create DX11 resource view");

			// Create the sample state
			D3D11_SAMPLER_DESC sampDesc = { };
			sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			sampDesc.MinLOD = 0;
			sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = m_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);
			if (FAILED(hr))
				TCU_FAIL("Failed to create DX11 sampler state");

			// Release the lock for VK to write into the texture.
			m_keyedMutex[BUFFER_VK_WRITE]->ReleaseSync(KEYED_MUTEX_VK_WRITE);
		}
	}

	~DX11Operation ()
	{
		cleanup();
	}
#endif // #if (DE_OS == DE_OS_WIN32)

	NativeHandle getNativeHandle (Buffer buffer)
	{
#if (DE_OS == DE_OS_WIN32)
		return NativeHandle((m_isMemNtHandle) ? NativeHandle::WIN32HANDLETYPE_NT : NativeHandle::WIN32HANDLETYPE_KMT, vk::pt::Win32Handle(m_sharedMemHandle[buffer]));
#else
		DE_UNREF(buffer);
		return NativeHandle();
#endif
	}

	void copyMemory ()
	{
#if (DE_OS == DE_OS_WIN32)
		m_keyedMutex[BUFFER_VK_WRITE]->AcquireSync(KEYED_MUTEX_DX_COPY, INFINITE);

		if (m_resourceDesc.type == RESOURCE_TYPE_BUFFER) {
			m_pContext->CopySubresourceRegion(m_pBuffer[BUFFER_VK_READ], 0, 0, 0, 0, m_pBuffer[BUFFER_VK_WRITE], 0, NULL);
		} else {
			m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);

			const FLOAT gray[] = { 0.f, 0.f, 1.f, 1.f };
			m_pContext->ClearRenderTargetView(m_pRenderTargetView, gray);

			m_pContext->VSSetShader(m_pVertexShader, NULL, 0);
			m_pContext->PSSetShader(m_pPixelShader, NULL, 0);
			m_pContext->PSSetShaderResources(0, 1, &m_pTextureRV);
			m_pContext->PSSetSamplers(0, 1, &m_pSamplerLinear);
			m_pContext->Draw(4, 0);
		}

		m_keyedMutex[BUFFER_VK_WRITE]->ReleaseSync(KEYED_MUTEX_DONE);
		m_keyedMutex[BUFFER_VK_READ]->ReleaseSync(KEYED_MUTEX_VK_VERIFY);
#endif // #if (DE_OS == DE_OS_WIN32)
	}

#if (DE_OS == DE_OS_WIN32)
	void d3dx11CompileShader (const char* shaderCode, const char * entryPoint, const char* shaderModel, ID3D10Blob** ppBlobOut)
	{
		HRESULT hr;

		ID3D10Blob* pErrorBlob;
		hr = m_fnD3DX11CompileFromMemory (shaderCode,
										  strlen(shaderCode),
										  "Memory",
										  NULL,
										  NULL,
										  entryPoint,
										  shaderModel,
										  0,
										  0,
										  NULL,
										  ppBlobOut,
										  &pErrorBlob,
										  NULL);
		if (pErrorBlob)
			pErrorBlob->Release();

		if (FAILED(hr))
			TCU_FAIL("D3DX11CompileFromMemory failed to compile shader");
	}

	void d3dCompileShader (const char* shaderCode, const char * entryPoint, const char* shaderModel, ID3DBlob** ppBlobOut)
	{
		HRESULT hr;

		ID3DBlob* pErrorBlob;
		hr = m_fnD3DCompile (shaderCode,
							 strlen(shaderCode),
							 NULL,
							 NULL,
							 NULL,
							 entryPoint,
							 shaderModel,
							 0,
							 0,
							 ppBlobOut,
							 &pErrorBlob);
		if (pErrorBlob)
			pErrorBlob->Release();

		if (FAILED(hr))
			TCU_FAIL("D3DCompile failed to compile shader");
	}

	void createShaders (const char* shaderSrc,
						const char* vsEntryPoint,
						const char* vsShaderModel,
						UINT numLayoutDesc,
						D3D11_INPUT_ELEMENT_DESC* pLayoutDesc,
						ID3D11VertexShader** pVertexShader,
						const char* psEntryPoint,
						const char* psShaderModel,
						ID3D11PixelShader** pPixelShader)
{
		HRESULT	hr;

		if (m_fnD3DX11CompileFromMemory) {
			// VS
			ID3D10Blob* pVSBlob;
			d3dx11CompileShader(shaderSrc, vsEntryPoint, vsShaderModel, &pVSBlob);

			hr = m_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, pVertexShader);
			if (FAILED(hr))
				TCU_FAIL("Failed to create DX11 vertex shader");

			ID3D11InputLayout *pVertexLayout;
			hr = m_pDevice->CreateInputLayout(pLayoutDesc, numLayoutDesc, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pVertexLayout);
			if (FAILED(hr))
				TCU_FAIL("Failed to create vertex input layout");

			m_pContext->IASetInputLayout(pVertexLayout);
			pVertexLayout->Release();
			pVSBlob->Release();

			// PS
			ID3D10Blob* pPSBlob;
			d3dx11CompileShader(shaderSrc, psEntryPoint, psShaderModel, &pPSBlob);

			hr = m_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, pPixelShader);
			if (FAILED(hr))
				TCU_FAIL("Failed to create DX11 pixel shader");
		} else {
			// VS
			ID3DBlob* pVSBlob;
			d3dCompileShader(shaderSrc, vsEntryPoint, vsShaderModel, &pVSBlob);

			hr = m_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, pVertexShader);
			if (FAILED(hr))
				TCU_FAIL("Failed to create DX11 vertex shader");

			ID3D11InputLayout *pVertexLayout;
			hr = m_pDevice->CreateInputLayout(pLayoutDesc, numLayoutDesc, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pVertexLayout);
			if (FAILED(hr))
				TCU_FAIL("Failed to create vertex input layout");

			m_pContext->IASetInputLayout(pVertexLayout);
			pVertexLayout->Release();
			pVSBlob->Release();

			// PS
			ID3DBlob* pPSBlob;
			d3dCompileShader(shaderSrc, psEntryPoint, psShaderModel, &pPSBlob);

			hr = m_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, pPixelShader);
			if (FAILED(hr))
				TCU_FAIL("Failed to create DX11 pixel shader");
		}
	}
#endif // #if (DE_OS == DE_OS_WIN32)

private:
#if (DE_OS == DE_OS_WIN32)
	void cleanup ()
	{
		if (m_securityAttributes.lpSecurityDescriptor)
		{
			freeSecurityDescriptor(m_securityAttributes.lpSecurityDescriptor);
			m_securityAttributes.lpSecurityDescriptor = NULL;
		}

		if (m_pContext)
			m_pContext->ClearState();

		if (m_pRenderTargetView)
		{
			m_pRenderTargetView->Release();
			m_pRenderTargetView = NULL;
		}

		if (m_pSamplerLinear)
		{
			m_pSamplerLinear->Release();
			m_pSamplerLinear = NULL;
		}

		if (m_pTextureRV)
		{
			m_pTextureRV->Release();
			m_pTextureRV = NULL;
		}

		if (m_pVertexBuffer)
		{
			m_pVertexBuffer->Release();
			m_pVertexBuffer = NULL;
		}

		if (m_pVertexShader)
		{
			m_pVertexShader->Release();
			m_pVertexShader = NULL;
		}

		if (m_pPixelShader)
		{
			m_pPixelShader->Release();
			m_pPixelShader = NULL;
		}

		for (int i = 0; i < BUFFER_COUNT; i++)
		{
			if (m_keyedMutex[i])
			{
				m_keyedMutex[i]->AcquireSync(KEYED_MUTEX_DONE, INFINITE);
				m_keyedMutex[i]->Release();
				m_keyedMutex[i] = NULL;
			}

			if (m_isMemNtHandle && m_sharedMemHandle[i]) {
				CloseHandle(m_sharedMemHandle[i]);
				m_sharedMemHandle[i] = 0;
			}

			if (m_pBuffer[i]) {
				m_pBuffer[i]->Release();
				m_pBuffer[i] = NULL;
			}

			if (m_pTexture[i]) {
				m_pTexture[i]->Release();
				m_pTexture[i] = NULL;
			}
		}
	}

	static void* getSecurityDescriptor ()
	{
		PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)deCalloc(SECURITY_DESCRIPTOR_MIN_LENGTH + 2 * sizeof (void**));

		if (pSD)
		{
			PSID*	ppEveryoneSID	= (PSID*)((PBYTE)pSD + SECURITY_DESCRIPTOR_MIN_LENGTH);
			PACL*	ppACL			= (PACL*)((PBYTE)ppEveryoneSID + sizeof(PSID*));

			InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

			SID_IDENTIFIER_AUTHORITY	SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
			AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, ppEveryoneSID);

			EXPLICIT_ACCESS	ea = { };
			ea.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
			ea.grfAccessMode = SET_ACCESS;
			ea.grfInheritance = INHERIT_ONLY;
			ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
			ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			ea.Trustee.ptstrName = (LPTSTR)*ppEveryoneSID;

			SetEntriesInAcl(1, &ea, NULL, ppACL);

			SetSecurityDescriptorDacl(pSD, TRUE, *ppACL, FALSE);
		}

		return pSD;
	}

	static void freeSecurityDescriptor (void* pSD)
	{
		if (pSD)
		{
			PSID*	ppEveryoneSID	= (PSID*)((PBYTE)pSD + SECURITY_DESCRIPTOR_MIN_LENGTH);
			PACL*	ppACL			= (PACL*)((PBYTE)ppEveryoneSID + sizeof(PSID*));

			if (*ppEveryoneSID)
				FreeSid(*ppEveryoneSID);

			if (*ppACL)
				LocalFree(*ppACL);

			deFree(pSD);
		}
	}

	static DXGI_FORMAT getDxgiFormat (vk::VkFormat format)
	{
		switch (format)
		{
			case vk::VK_FORMAT_R8_UNORM:
				return DXGI_FORMAT_R8_UNORM;
			case vk::VK_FORMAT_R16_UINT:
				return DXGI_FORMAT_R16_UINT;
			case vk::VK_FORMAT_R8G8B8A8_UNORM:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			case vk::VK_FORMAT_R16G16B16A16_UINT:
				return DXGI_FORMAT_R16G16B16A16_UINT;
			case vk::VK_FORMAT_R32G32B32A32_SFLOAT:
				return DXGI_FORMAT_R32G32B32A32_FLOAT;
			case vk::VK_FORMAT_D16_UNORM:
				return DXGI_FORMAT_D16_UNORM;
			case vk::VK_FORMAT_D32_SFLOAT:
				return DXGI_FORMAT_D32_FLOAT;
			default:
				TCU_CHECK_INTERNAL(!"Unsupported DXGI format");
				return DXGI_FORMAT_UNKNOWN;
		}
	}

	ResourceDescription			m_resourceDesc;

	deUint64					m_sharedMemSize;
	deUint64					m_sharedMemOffset;
	HANDLE						m_sharedMemHandle[BUFFER_COUNT];
	bool						m_isMemNtHandle;

	ID3D11Device*				m_pDevice;
	ID3D11DeviceContext*		m_pContext;
	LPD3DX11COMPILEFROMMEMORY	m_fnD3DX11CompileFromMemory;
	pD3DCompile					m_fnD3DCompile;

	ID3D11RenderTargetView*		m_pRenderTargetView;
	ID3D11VertexShader*			m_pVertexShader;
	ID3D11PixelShader*			m_pPixelShader;
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11ShaderResourceView*	m_pTextureRV;
	ID3D11SamplerState*			m_pSamplerLinear;

	ID3D11Texture2D*			m_pTexture[BUFFER_COUNT];
	ID3D11Buffer*				m_pBuffer[BUFFER_COUNT];
	IDXGIKeyedMutex*			m_keyedMutex[BUFFER_COUNT];
	UINT						m_numFrames;
	SECURITY_ATTRIBUTES			m_securityAttributes;

	SECURITY_ATTRIBUTES* getSecurityAttributes ()
	{
		m_securityAttributes.nLength = sizeof (SECURITY_ATTRIBUTES);
		m_securityAttributes.bInheritHandle = TRUE;
		if (!m_securityAttributes.lpSecurityDescriptor)
			m_securityAttributes.lpSecurityDescriptor = getSecurityDescriptor();

		return &m_securityAttributes;
	}
#endif // #if (DE_OS == DE_OS_WIN32)
};

class DX11OperationSupport
{
public:
	DX11OperationSupport (const vk::InstanceInterface&	vki,
						  vk::VkPhysicalDevice			physicalDevice,
						  const ResourceDescription&	resourceDesc)
		: m_resourceDesc			(resourceDesc)
#if (DE_OS == DE_OS_WIN32)
		, m_hD3D11Lib					(0)
		, m_hD3DX11Lib					(0)
		, m_hD3DCompilerLib				(0)
		, m_hDxgiLib					(0)
		, m_fnD3D11CreateDevice			(0)
		, m_fnD3DX11CompileFromMemory	(0)
		, m_fnD3DCompile				(0)
#endif
	{
#if (DE_OS == DE_OS_WIN32)
		HRESULT										hr;

		vk::VkPhysicalDeviceIDPropertiesKHR			propertiesId = { vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR };
		vk::VkPhysicalDeviceProperties2KHR			properties = { vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR };

		properties.pNext = &propertiesId;

		vki.getPhysicalDeviceProperties2KHR(physicalDevice, &properties);
		if (!propertiesId.deviceLUIDValid)
			TCU_FAIL("Physical device deviceLUIDValid is not valid");


		m_hD3D11Lib = LoadLibrary("d3d11.dll");
		if (!m_hD3D11Lib)
			TCU_FAIL("Failed to load d3d11.dll");


		m_fnD3D11CreateDevice = (LPD3D11CREATEDEVICE) GetProcAddress(m_hD3D11Lib, "D3D11CreateDevice");
		if (!m_fnD3D11CreateDevice)
			TCU_FAIL("Unable to find D3D11CreateDevice() function");

		m_hD3DX11Lib = LoadLibrary("d3dx11_42.dll");
		if (m_hD3DX11Lib)
			m_fnD3DX11CompileFromMemory =  (LPD3DX11COMPILEFROMMEMORY) GetProcAddress(m_hD3DX11Lib, "D3DX11CompileFromMemory");
		else
		{
			m_hD3DCompilerLib = LoadLibrary("d3dcompiler_43.dll");
			if (!m_hD3DCompilerLib)
				m_hD3DCompilerLib = LoadLibrary("d3dcompiler_47.dll");
			if (!m_hD3DCompilerLib)
				TCU_FAIL("Unable to load DX11 d3dcompiler_43.dll or d3dcompiler_47.dll");

			m_fnD3DCompile = (pD3DCompile)GetProcAddress(m_hD3DCompilerLib, "D3DCompile");
			if (!m_fnD3DCompile)
				TCU_FAIL("Unable to load find D3DCompile");
		}

		m_hDxgiLib = LoadLibrary("dxgi.dll");
		if (!m_hDxgiLib)
			TCU_FAIL("Unable to load DX11 dxgi.dll");

		typedef HRESULT (WINAPI *LPCREATEDXGIFACTORY1)(REFIID riid, void** ppFactory);
		LPCREATEDXGIFACTORY1 CreateDXGIFactory1 = (LPCREATEDXGIFACTORY1)GetProcAddress(m_hDxgiLib, "CreateDXGIFactory1");
		if (!CreateDXGIFactory1)
			TCU_FAIL("Unable to load find CreateDXGIFactory1");

		IDXGIFactory1* pFactory = NULL;
		hr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)&pFactory);
		if (FAILED(hr))
			TCU_FAIL("Unable to create IDXGIFactory interface");

		IDXGIAdapter *pAdapter = NULL;
		for (UINT i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC desc;
			pAdapter->GetDesc(&desc);

			if (deMemCmp(&desc.AdapterLuid, propertiesId.deviceLUID, VK_LUID_SIZE_KHR) == 0)
				break;
		}
		pFactory->Release();

		D3D_FEATURE_LEVEL fLevel[] = {D3D_FEATURE_LEVEL_11_0};
		UINT devflags = D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS | // no separate D3D11 worker thread
#if 0
						D3D11_CREATE_DEVICE_DEBUG | // useful for diagnosing DX failures
#endif
						D3D11_CREATE_DEVICE_SINGLETHREADED;

		hr = m_fnD3D11CreateDevice (pAdapter,
									pAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
									NULL,
									devflags,
									fLevel,
									DE_LENGTH_OF_ARRAY(fLevel),
									D3D11_SDK_VERSION,
									&m_pDevice,
									NULL,
									&m_pContext);

		if (pAdapter) {
			pAdapter->Release();
		}

		if (!m_pDevice)
			TCU_FAIL("Failed to created DX11 device");
		if (!m_pContext)
			TCU_FAIL("Failed to created DX11 context");
#else
		DE_UNREF(vki);
		DE_UNREF(physicalDevice);
		TCU_THROW(NotSupportedError, "OS not supported");
#endif
	}

	~DX11OperationSupport ()
	{
#if (DE_OS == DE_OS_WIN32)
		cleanup ();
#endif
	}

#if (DE_OS == DE_OS_WIN32)
	void cleanup ()
	{
		if (m_pContext) {
			m_pContext->Release();
			m_pContext = 0;
		}

		if (m_pDevice) {
			m_pDevice->Release();
			m_pDevice = 0;
		}

		if (m_hDxgiLib)
		{
			FreeLibrary(m_hDxgiLib);
			m_hDxgiLib = 0;
		}

		if (m_hD3DCompilerLib)
		{
			FreeLibrary(m_hD3DCompilerLib);
			m_hD3DCompilerLib = 0;
		}

		if (m_hD3DX11Lib)
		{
			FreeLibrary(m_hD3DX11Lib);
			m_hD3DX11Lib = 0;
		}

		if (m_hD3D11Lib)
		{
			FreeLibrary(m_hD3D11Lib);
			m_hD3D11Lib = 0;
		}
	}

#endif

	virtual de::MovePtr<DX11Operation> build (const ResourceDescription& resourceDesc, vk::VkExternalMemoryHandleTypeFlagBitsKHR memoryHandleType) const
	{
#if (DE_OS == DE_OS_WIN32)
		return de::MovePtr<DX11Operation>(new DX11Operation(resourceDesc, memoryHandleType, m_pDevice, m_pContext, m_fnD3DX11CompileFromMemory, m_fnD3DCompile));
#else
		DE_UNREF(resourceDesc);
		DE_UNREF(memoryHandleType);
		TCU_THROW(NotSupportedError, "OS not supported");
#endif
	}

private:
	const ResourceDescription	m_resourceDesc;

#if (DE_OS == DE_OS_WIN32)
	typedef HRESULT				(WINAPI *LPD3D11CREATEDEVICE)(IDXGIAdapter*,
															  D3D_DRIVER_TYPE,
															  HMODULE,
															  UINT,
															  const D3D_FEATURE_LEVEL*,
															  UINT,
															  UINT,
															  ID3D11Device **,
															  D3D_FEATURE_LEVEL*,
															  ID3D11DeviceContext**);

	HMODULE						m_hD3D11Lib;
	HMODULE						m_hD3DX11Lib;
	HMODULE						m_hD3DCompilerLib;
	HMODULE						m_hDxgiLib;
	LPD3D11CREATEDEVICE			m_fnD3D11CreateDevice;
	LPD3DX11COMPILEFROMMEMORY	m_fnD3DX11CompileFromMemory;
	pD3DCompile					m_fnD3DCompile;
	ID3D11Device*				m_pDevice;
	ID3D11DeviceContext*		m_pContext;
#endif
};

class Win32KeyedMutexTestInstance : public TestInstance
{
public:
														Win32KeyedMutexTestInstance	(Context&	context,
																					 TestConfig	config);

	virtual tcu::TestStatus								iterate					(void);

private:
	const TestConfig									m_config;
	const de::UniquePtr<OperationSupport>				m_supportWriteOp;
	const de::UniquePtr<OperationSupport>				m_supportReadOp;

	const vk::Unique<vk::VkInstance>					m_instance;

	const vk::InstanceDriver							m_vki;
	const vk::VkPhysicalDevice							m_physicalDevice;
	const std::vector<vk::VkQueueFamilyProperties>		m_queueFamilies;
	const std::vector<deUint32>							m_queueFamilyIndices;
	const vk::Unique<vk::VkDevice>						m_device;
	const vk::DeviceDriver								m_vkd;

	const de::UniquePtr<DX11OperationSupport>			m_supportDX11;

	const vk::VkExternalMemoryHandleTypeFlagBitsKHR		m_memoryHandleType;

	// \todo Should this be moved to the group same way as in the other tests?
	PipelineCacheData									m_pipelineCacheData;
	tcu::ResultCollector								m_resultCollector;
	size_t												m_queueNdx;

	bool												m_useDedicatedAllocation;
};

Win32KeyedMutexTestInstance::Win32KeyedMutexTestInstance	(Context&		context,
															 TestConfig		config)
	: TestInstance				(context)
	, m_config					(config)
	, m_supportWriteOp			(makeOperationSupport(config.writeOp, config.resource))
	, m_supportReadOp			(makeOperationSupport(config.readOp, config.resource))

	, m_instance				(createInstance(context.getPlatformInterface()))

	, m_vki						(context.getPlatformInterface(), *m_instance)
	, m_physicalDevice			(getPhysicalDevice(m_vki, *m_instance, context.getTestContext().getCommandLine()))
	, m_queueFamilies			(vk::getPhysicalDeviceQueueFamilyProperties(m_vki, m_physicalDevice))
	, m_queueFamilyIndices		(getFamilyIndices(m_queueFamilies))
	, m_device					(createDevice(m_vki, m_physicalDevice))
	, m_vkd						(m_vki, *m_device)

	, m_supportDX11				(new DX11OperationSupport(m_vki, m_physicalDevice, config.resource))

	, m_memoryHandleType		((m_config.resource.type == RESOURCE_TYPE_IMAGE) ? m_config.memoryHandleTypeImage : m_config.memoryHandleTypeBuffer)

	, m_resultCollector			(context.getTestContext().getLog())
	, m_queueNdx				(0)

	, m_useDedicatedAllocation	(false)
{
#if (DE_OS == DE_OS_WIN32)
	TestLog& log = m_context.getTestContext().getLog();

	// Check resource support
	if (m_config.resource.type == RESOURCE_TYPE_IMAGE)
	{
		if (m_memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT_KHR && !IsWindows8OrGreater())
			TCU_THROW(NotSupportedError, "Memory handle type not supported by this OS");

		const vk::VkPhysicalDeviceExternalImageFormatInfoKHR	externalInfo		=
		{
			vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO_KHR,
			DE_NULL,
			m_memoryHandleType
		};
		const vk::VkPhysicalDeviceImageFormatInfo2KHR	imageFormatInfo		=
		{
			vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2_KHR,
			&externalInfo,
			m_config.resource.imageFormat,
			m_config.resource.imageType,
			vk::VK_IMAGE_TILING_OPTIMAL,
			m_supportReadOp->getResourceUsageFlags() | m_supportWriteOp->getResourceUsageFlags(),
			0u
		};
		vk::VkExternalImageFormatPropertiesKHR			externalProperties	=
		{
			vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO_KHR,
			DE_NULL,
			{ 0u, 0u, 0u }
		};
		vk::VkImageFormatProperties2KHR					formatProperties	=
		{
			vk::VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2_KHR,
			&externalProperties,
			{
				{ 0u, 0u, 0u },
				0u,
				0u,
				0u,
				0u,
			}
		};
		VK_CHECK(m_vki.getPhysicalDeviceImageFormatProperties2KHR(m_physicalDevice, &imageFormatInfo, &formatProperties));

		// \todo How to log this nicely?
		log << TestLog::Message << "External image format properties: " << imageFormatInfo << "\n"<< externalProperties << TestLog::EndMessage;

		if ((externalProperties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR) == 0)
			TCU_THROW(NotSupportedError, "Importing image resource not supported");

		if (externalProperties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_KHR)
			m_useDedicatedAllocation = true;
	}
	else
	{
		if (m_memoryHandleType == vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR && !IsWindows8OrGreater())
			TCU_THROW(NotSupportedError, "Memory handle type not supported by this OS");

		const vk::VkPhysicalDeviceExternalBufferInfoKHR	info	=
		{
			vk::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO_KHR,
			DE_NULL,

			0u,
			m_supportReadOp->getResourceUsageFlags() | m_supportWriteOp->getResourceUsageFlags(),
			m_memoryHandleType
		};
		vk::VkExternalBufferPropertiesKHR				properties			=
		{
			vk::VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES_KHR,
			DE_NULL,
			{ 0u, 0u, 0u}
		};
		m_vki.getPhysicalDeviceExternalBufferPropertiesKHR(m_physicalDevice, &info, &properties);

		log << TestLog::Message << "External buffer properties: " << info << "\n" << properties << TestLog::EndMessage;

		if ((properties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR) == 0)
			TCU_THROW(NotSupportedError, "Importing memory type not supported");

		if (properties.externalMemoryProperties.externalMemoryFeatures & vk::VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_KHR)
			m_useDedicatedAllocation = true;
	}
#else
	DE_UNREF(m_useDedicatedAllocation);
	TCU_THROW(NotSupportedError, "OS not supported");
#endif
}

tcu::TestStatus Win32KeyedMutexTestInstance::iterate (void)
{
	TestLog&									log					(m_context.getTestContext().getLog());

	try
	{
		const deUint32							queueFamily			= (deUint32)m_queueNdx;

		const tcu::ScopedLogSection				queuePairSection	(log, "Queue-" + de::toString(queueFamily), "Queue-" + de::toString(queueFamily));

		const vk::VkQueue						queue				(getDeviceQueue(m_vkd, *m_device, queueFamily, 0u));
		const vk::Unique<vk::VkCommandPool>		commandPool			(createCommandPool(m_vkd, *m_device, 0u, queueFamily));
		const vk::Unique<vk::VkCommandBuffer>	commandBufferWrite	(allocateCommandBuffer(m_vkd, *m_device, *commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const vk::Unique<vk::VkCommandBuffer>	commandBufferRead	(allocateCommandBuffer(m_vkd, *m_device, *commandPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		vk::SimpleAllocator						allocator			(m_vkd, *m_device, vk::getPhysicalDeviceMemoryProperties(m_vki, m_physicalDevice));
		const std::vector<std::string>			deviceExtensions;
		OperationContext						operationContext	(m_vki, m_vkd, m_physicalDevice, *m_device, allocator, deviceExtensions, m_context.getBinaryCollection(), m_pipelineCacheData);

		if (!checkQueueFlags(m_queueFamilies[m_queueNdx].queueFlags, vk::VK_QUEUE_GRAPHICS_BIT))
			TCU_THROW(NotSupportedError, "Operation not supported by the source queue");

		const de::UniquePtr<DX11Operation>		dx11Op				(m_supportDX11->build(m_config.resource, m_memoryHandleType));

		NativeHandle nativeHandleWrite = dx11Op->getNativeHandle(DX11Operation::BUFFER_VK_WRITE);
		const de::UniquePtr<Resource>			resourceWrite		(importResource(m_vkd, *m_device, m_config.resource, m_queueFamilyIndices, *m_supportReadOp, *m_supportWriteOp, nativeHandleWrite, m_memoryHandleType));

		NativeHandle nativeHandleRead = dx11Op->getNativeHandle(DX11Operation::BUFFER_VK_READ);
		const de::UniquePtr<Resource>			resourceRead		(importResource(m_vkd, *m_device, m_config.resource, m_queueFamilyIndices, *m_supportReadOp, *m_supportWriteOp, nativeHandleRead, m_memoryHandleType));

		const de::UniquePtr<Operation>			writeOp				(m_supportWriteOp->build(operationContext, *resourceWrite));
		const de::UniquePtr<Operation>			readOp				(m_supportReadOp->build(operationContext, *resourceRead));

		const SyncInfo							writeSync			= writeOp->getSyncInfo();
		const SyncInfo							readSync			= readOp->getSyncInfo();

		beginCommandBuffer(m_vkd, *commandBufferWrite);
		writeOp->recordCommands(*commandBufferWrite);
		recordWriteBarrier(m_vkd, *commandBufferWrite, *resourceWrite, writeSync, queueFamily, readSync);
		endCommandBuffer(m_vkd, *commandBufferWrite);

		beginCommandBuffer(m_vkd, *commandBufferRead);
		recordReadBarrier(m_vkd, *commandBufferRead, *resourceRead, writeSync, readSync, queueFamily);
		readOp->recordCommands(*commandBufferRead);
		endCommandBuffer(m_vkd, *commandBufferRead);

		{
			vk::VkDeviceMemory							memory			= resourceWrite->getMemory();
			deUint64									keyInit			= DX11Operation::KEYED_MUTEX_VK_WRITE;
			deUint32									timeout			= 0xFFFFFFFF; // INFINITE
			deUint64									keyExternal		= DX11Operation::KEYED_MUTEX_DX_COPY;
			vk::VkWin32KeyedMutexAcquireReleaseInfoKHR	keyedMutexInfo	=
			{
				vk::VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR,
				DE_NULL,

				1,
				&memory,
				&keyInit,
				&timeout,

				1,
				&memory,
				&keyExternal,
			};

			const vk::VkCommandBuffer	commandBuffer	= *commandBufferWrite;
			const vk::VkSubmitInfo		submitInfo			=
			{
				vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,
				&keyedMutexInfo,

				0u,
				DE_NULL,
				DE_NULL,

				1u,
				&commandBuffer,
				0u,
				DE_NULL
			};

			VK_CHECK(m_vkd.queueSubmit(queue, 1u, &submitInfo, DE_NULL));
		}

		dx11Op->copyMemory();

		{
			vk::VkDeviceMemory							memory			= resourceRead->getMemory();
			deUint64									keyInternal		= DX11Operation::KEYED_MUTEX_VK_VERIFY;
			deUint32									timeout			= 0xFFFFFFFF; // INFINITE
			deUint64									keyExternal		= DX11Operation::KEYED_MUTEX_DONE;
			vk::VkWin32KeyedMutexAcquireReleaseInfoKHR	keyedMutexInfo	=
			{
				vk::VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR,
				DE_NULL,

				1,
				&memory,
				&keyInternal,
				&timeout,

				1,
				&memory,
				&keyExternal,
			};

			const vk::VkCommandBuffer	commandBuffer	= *commandBufferRead;
			const vk::VkSubmitInfo		submitInfo			=
			{
				vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,
				&keyedMutexInfo,

				0u,
				DE_NULL,
				DE_NULL,

				1u,
				&commandBuffer,
				0u,
				DE_NULL
			};

			VK_CHECK(m_vkd.queueSubmit(queue, 1u, &submitInfo, DE_NULL));
		}

		VK_CHECK(m_vkd.queueWaitIdle(queue));

		{
			const Data	expected	= writeOp->getData();
			const Data	actual		= readOp->getData();

			DE_ASSERT(expected.size == actual.size);

			if (0 != deMemCmp(expected.data, actual.data, expected.size))
			{
				const size_t		maxBytesLogged	= 256;
				std::ostringstream	expectedData;
				std::ostringstream	actualData;
				size_t				byteNdx			= 0;

				// Find first byte difference
				for (; actual.data[byteNdx] == expected.data[byteNdx]; byteNdx++)
				{
					// Nothing
				}

				log << TestLog::Message << "First different byte at offset: " << byteNdx << TestLog::EndMessage;

				// Log 8 previous bytes before the first incorrect byte
				if (byteNdx > 8)
				{
					expectedData << "... ";
					actualData << "... ";

					byteNdx -= 8;
				}
				else
					byteNdx = 0;

				for (size_t i = 0; i < maxBytesLogged && byteNdx < expected.size; i++, byteNdx++)
				{
					expectedData << (i > 0 ? ", " : "") << (deUint32)expected.data[byteNdx];
					actualData << (i > 0 ? ", " : "") << (deUint32)actual.data[byteNdx];
				}

				if (expected.size > byteNdx)
				{
					expectedData << "...";
					actualData << "...";
				}

				log << TestLog::Message << "Expected data: (" << expectedData.str() << ")" << TestLog::EndMessage;
				log << TestLog::Message << "Actual data: (" << actualData.str() << ")" << TestLog::EndMessage;

				m_resultCollector.fail("Memory contents don't match");
			}
		}
	}
	catch (const tcu::NotSupportedError& error)
	{
		log << TestLog::Message << "Not supported: " << error.getMessage() << TestLog::EndMessage;
	}
	catch (const tcu::TestError& error)
	{
		m_resultCollector.fail(std::string("Exception: ") + error.getMessage());
	}

	// Move to next queue
	{
		m_queueNdx++;

		if (m_queueNdx >= m_queueFamilies.size())
		{
			return tcu::TestStatus(m_resultCollector.getResult(), m_resultCollector.getMessage());
		}
		else
		{
			return tcu::TestStatus::incomplete();
		}
	}
}

struct Progs
{
	void init (vk::SourceCollections& dst, TestConfig config) const
	{
		const de::UniquePtr<OperationSupport>	readOp	(makeOperationSupport(config.readOp, config.resource));
		const de::UniquePtr<OperationSupport>	writeOp	(makeOperationSupport(config.writeOp, config.resource));

		readOp->initPrograms(dst);
		writeOp->initPrograms(dst);
	}
};

} // anonymous

tcu::TestCaseGroup* createWin32KeyedMutexTest (tcu::TestContext& testCtx)
{
	const struct
	{
		vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleTypeBuffer;
		vk::VkExternalMemoryHandleTypeFlagBitsKHR		memoryHandleTypeImage;
		const char*										nameSuffix;
	} cases[] =
	{
		{
			(vk::VkExternalMemoryHandleTypeFlagBitsKHR)0u,				// DX11 doesn't support buffers with an NT handle
			vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT_KHR,
			"_nt"
		},
		{
			vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR,
			vk::VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT_KHR,
			"_kmt"
		},
	};
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "win32_keyed_mutex", ""));

	for (size_t writeOpNdx = 0; writeOpNdx < DE_LENGTH_OF_ARRAY(s_writeOps); ++writeOpNdx)
	for (size_t readOpNdx = 0; readOpNdx < DE_LENGTH_OF_ARRAY(s_readOps); ++readOpNdx)
	{
		const OperationName	writeOp		= s_writeOps[writeOpNdx];
		const OperationName	readOp		= s_readOps[readOpNdx];
		const std::string	opGroupName	= getOperationName(writeOp) + "_" + getOperationName(readOp);
		bool				empty		= true;

		de::MovePtr<tcu::TestCaseGroup> opGroup	(new tcu::TestCaseGroup(testCtx, opGroupName.c_str(), ""));

		for (size_t resourceNdx = 0; resourceNdx < DE_LENGTH_OF_ARRAY(s_resourcesWin32KeyedMutex); ++resourceNdx)
		{
			const ResourceDescription&	resource	= s_resourcesWin32KeyedMutex[resourceNdx];

			for (size_t caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); caseNdx++)
			{
				if (resource.type == RESOURCE_TYPE_BUFFER && !cases[caseNdx].memoryHandleTypeBuffer)
					continue;

				if (resource.type == RESOURCE_TYPE_IMAGE && !cases[caseNdx].memoryHandleTypeImage)
					continue;

				std::string	name	= getResourceName(resource) + cases[caseNdx].nameSuffix;

				if (isResourceSupported(writeOp, resource) && isResourceSupported(readOp, resource))
				{
					const TestConfig config (resource, writeOp, readOp, cases[caseNdx].memoryHandleTypeBuffer, cases[caseNdx].memoryHandleTypeImage);

					opGroup->addChild(new InstanceFactory1<Win32KeyedMutexTestInstance, TestConfig, Progs>(testCtx, tcu::NODETYPE_SELF_VALIDATE,  name, "", Progs(), config));
					empty = false;
				}
			}
		}

		if (!empty)
			group->addChild(opGroup.release());
	}

	return group.release();
}

} // synchronization
} // vkt
