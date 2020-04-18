/*-------------------------------------------------------------------------
 *  Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
 * \brief Vulkan Buffer View Creation Tests
 *//*--------------------------------------------------------------------*/

#include "vktApiBufferViewCreateTests.hpp"
#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "gluVarType.hpp"
#include "tcuTestLog.hpp"
#include "vkPrograms.hpp"
#include "vkRefUtil.hpp"
#include "vktTestCase.hpp"

namespace vkt
{

using namespace vk;

namespace api
{

namespace
{

enum AllocationKind
{
	ALLOCATION_KIND_SUBALLOCATED = 0,
	ALLOCATION_KIND_DEDICATED,

	ALLOCATION_KIND_LAST,
};

class IBufferAllocator;

struct BufferViewCaseParameters
{
	VkFormat							format;
	VkDeviceSize						offset;
	VkDeviceSize						range;
	VkBufferUsageFlags					usage;
	VkFormatFeatureFlags				features;
	AllocationKind						bufferAllocationKind;
};

class BufferViewTestInstance : public TestInstance
{
public:
										BufferViewTestInstance			(Context&					ctx,
																		 BufferViewCaseParameters	createInfo)
										: TestInstance					(ctx)
										, m_testCase					(createInfo)
	{}
	virtual tcu::TestStatus				iterate							(void);

protected:
	BufferViewCaseParameters			m_testCase;
};

class IBufferAllocator
{
public:
	virtual tcu::TestStatus				createTestBuffer				(VkDeviceSize				size,
																		 VkBufferUsageFlags			usage,
																		 Context&					context,
																		 Move<VkBuffer>&			testBuffer,
																		 Move<VkDeviceMemory>&		memory) const = 0;
};

class BufferSuballocation : public IBufferAllocator
{
public:
	virtual tcu::TestStatus				createTestBuffer				(VkDeviceSize				size,
																		 VkBufferUsageFlags			usage,
																		 Context&					context,
																		 Move<VkBuffer>&			testBuffer,
																		 Move<VkDeviceMemory>&		memory) const;
};

class BufferDedicatedAllocation	: public IBufferAllocator
{
public:
	virtual tcu::TestStatus				createTestBuffer				(VkDeviceSize				size,
																		 VkBufferUsageFlags			usage,
																		 Context&					context,
																		 Move<VkBuffer>&			testBuffer,
																		 Move<VkDeviceMemory>&		memory) const;
};

class BufferViewTestCase : public TestCase
{
public:
										BufferViewTestCase				(tcu::TestContext&			testCtx,
																		 const std::string&			name,
																		 const std::string&			description,
																		 BufferViewCaseParameters	createInfo)
										: TestCase						(testCtx, name, description)
										, m_testCase					(createInfo)
	{}
	virtual								~BufferViewTestCase				(void)
	{}
	virtual TestInstance*				createInstance					(Context&					ctx) const
	{
		return new BufferViewTestInstance(ctx, m_testCase);
	}
private:
	BufferViewCaseParameters			m_testCase;
};

tcu::TestStatus BufferSuballocation::createTestBuffer					(VkDeviceSize				size,
																		 VkBufferUsageFlags			usage,
																		 Context&					context,
																		 Move<VkBuffer>&			testBuffer,
																		 Move<VkDeviceMemory>&		memory) const
{
	const VkDevice						vkDevice						= context.getDevice();
	const DeviceInterface&				vk								= context.getDeviceInterface();
	const deUint32						queueFamilyIndex				= context.getUniversalQueueFamilyIndex();
	VkMemoryRequirements				memReqs;
	const VkBufferCreateInfo			bufferParams					=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,							//	VkStructureType			sType;
		DE_NULL,														//	const void*				pNext;
		0u,																//	VkBufferCreateFlags		flags;
		size,															//	VkDeviceSize			size;
		usage,															//	VkBufferUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,										//	VkSharingMode			sharingMode;
		1u,																//	deUint32				queueFamilyCount;
		&queueFamilyIndex,												//	const deUint32*			pQueueFamilyIndices;
	};

	try
	{
		testBuffer = vk::createBuffer(vk, vkDevice, &bufferParams, (const VkAllocationCallbacks*)DE_NULL);
	}
	catch (const vk::Error& error)
	{
		return tcu::TestStatus::fail("Buffer creation failed! (Error code: " + de::toString(error.getMessage()) + ")");
	}

	vk.getBufferMemoryRequirements(vkDevice, *testBuffer, &memReqs);

	if (size > memReqs.size)
	{
		std::ostringstream errorMsg;
		errorMsg << "Requied memory size (" << memReqs.size << " bytes) smaller than the buffer's size (" << size << " bytes)!";
		return tcu::TestStatus::fail(errorMsg.str());
	}

	const VkMemoryAllocateInfo			memAlloc						=
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,							//	VkStructureType			sType
		NULL,															//	const void*				pNext
		memReqs.size,													//	VkDeviceSize			allocationSize
		(deUint32)deCtz32(memReqs.memoryTypeBits)						//	deUint32				memoryTypeIndex
	};

	try
	{
		memory = allocateMemory(vk, vkDevice, &memAlloc, (const VkAllocationCallbacks*)DE_NULL);
	}
	catch (const vk::Error& error)
	{
		return tcu::TestStatus::fail("Alloc memory failed! (Error code: " + de::toString(error.getMessage()) + ")");
	}

	if (vk.bindBufferMemory(vkDevice, *testBuffer, *memory, 0) != VK_SUCCESS)
		return tcu::TestStatus::fail("Bind buffer memory failed!");

	return tcu::TestStatus::incomplete();
}

tcu::TestStatus BufferDedicatedAllocation::createTestBuffer				(VkDeviceSize				size,
																		 VkBufferUsageFlags			usage,
																		 Context&					context,
																		 Move<VkBuffer>&			testBuffer,
																		 Move<VkDeviceMemory>&		memory) const
{
	const std::vector<std::string>&		extensions						= context.getDeviceExtensions();
	const deBool						isSupported						= std::find(extensions.begin(), extensions.end(), "VK_KHR_dedicated_allocation") != extensions.end();
	if (!isSupported)
		TCU_THROW(NotSupportedError, "Not supported");

	const InstanceInterface&			vkInstance						= context.getInstanceInterface();
	const VkDevice						vkDevice						= context.getDevice();
	const VkPhysicalDevice				vkPhysicalDevice				= context.getPhysicalDevice();
	const DeviceInterface&				vk								= context.getDeviceInterface();
	const deUint32						queueFamilyIndex				= context.getUniversalQueueFamilyIndex();
	VkPhysicalDeviceMemoryProperties	memoryProperties;
	VkMemoryDedicatedRequirementsKHR	dedicatedRequirements			=
	{
		VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR,			// VkStructureType		sType;
		DE_NULL,														// const void*			pNext;
		false,															// VkBool32				prefersDedicatedAllocation
		false															// VkBool32				requiresDedicatedAllocation
	};
	VkMemoryRequirements2KHR			memReqs							=
	{
		VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR,					// VkStructureType		sType
		&dedicatedRequirements,											// void*				pNext
		{0, 0, 0}														// VkMemoryRequirements	memoryRequirements
	};

	const VkBufferCreateInfo			bufferParams					=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,							//	VkStructureType			sType;
		DE_NULL,														//	const void*				pNext;
		0u,																//	VkBufferCreateFlags		flags;
		size,															//	VkDeviceSize			size;
		usage,															//	VkBufferUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,										//	VkSharingMode			sharingMode;
		1u,																//	deUint32				queueFamilyCount;
		&queueFamilyIndex,												//	const deUint32*			pQueueFamilyIndices;
	};

	try
	{
		testBuffer = vk::createBuffer(vk, vkDevice, &bufferParams, (const VkAllocationCallbacks*)DE_NULL);
	}
	catch (const vk::Error& error)
	{
		return tcu::TestStatus::fail("Buffer creation failed! (Error code: " + de::toString(error.getMessage()) + ")");
	}

	VkBufferMemoryRequirementsInfo2KHR	info							=
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR,		// VkStructureType		sType
		DE_NULL,														// const void*			pNext
		*testBuffer														// VkBuffer				buffer
	};

	vk.getBufferMemoryRequirements2KHR(vkDevice, &info, &memReqs);

	if (dedicatedRequirements.requiresDedicatedAllocation == VK_TRUE)
	{
		std::ostringstream				errorMsg;
		errorMsg << "Nonexternal objects cannot require dedicated allocation.";
		return tcu::TestStatus::fail(errorMsg.str());
	}

	if (size > memReqs.memoryRequirements.size)
	{
		std::ostringstream				errorMsg;
		errorMsg << "Requied memory size (" << memReqs.memoryRequirements.size << " bytes) smaller than the buffer's size (" << size << " bytes)!";
		return tcu::TestStatus::fail(errorMsg.str());
	}

	deMemset(&memoryProperties, 0, sizeof(memoryProperties));
	vkInstance.getPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memoryProperties);

	const deUint32						heapTypeIndex					= static_cast<deUint32>(deCtz32(memReqs.memoryRequirements.memoryTypeBits));
	//const VkMemoryType					memoryType						= memoryProperties.memoryTypes[heapTypeIndex];
	//const VkMemoryHeap					memoryHeap						= memoryProperties.memoryHeaps[memoryType.heapIndex];

	vk.getBufferMemoryRequirements2KHR(vkDevice, &info, &memReqs); // get the proper size requirement

	if (size > memReqs.memoryRequirements.size)
	{
		std::ostringstream				errorMsg;
		errorMsg << "Requied memory size (" << memReqs.memoryRequirements.size << " bytes) smaller than the buffer's size (" << size << " bytes)!";
		return tcu::TestStatus::fail(errorMsg.str());
	}

	{
		VkResult						result							= VK_ERROR_OUT_OF_HOST_MEMORY;
		VkDeviceMemory					rawMemory						= DE_NULL;

		vk::VkMemoryDedicatedAllocateInfoKHR
										dedicatedInfo					=
		{
			VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,		// VkStructureType			sType
			DE_NULL,													// const void*				pNext
			DE_NULL,													// VkImage					image
			*testBuffer													// VkBuffer					buffer
		};

		VkMemoryAllocateInfo			memoryAllocateInfo				=
		{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,						// VkStructureType			sType
			&dedicatedInfo,												// const void*				pNext
			memReqs.memoryRequirements.size,							// VkDeviceSize				allocationSize
			heapTypeIndex,												// deUint32					memoryTypeIndex
		};

		result = vk.allocateMemory(vkDevice, &memoryAllocateInfo, (VkAllocationCallbacks*)DE_NULL, &rawMemory);

		if (result != VK_SUCCESS)
			return tcu::TestStatus::fail("Unable to allocate " + de::toString(memReqs.memoryRequirements.size) + " bytes of memory");

		memory = Move<VkDeviceMemory>(check<VkDeviceMemory>(rawMemory), Deleter<VkDeviceMemory>(vk, vkDevice, DE_NULL));
	}


	if (vk.bindBufferMemory(vkDevice, *testBuffer, *memory, 0) != VK_SUCCESS)
		return tcu::TestStatus::fail("Bind buffer memory failed! (requested memory size: " + de::toString(size) + ")");

	return tcu::TestStatus::incomplete();
}

tcu::TestStatus BufferViewTestInstance::iterate							(void)
{
	const VkDevice						vkDevice						= m_context.getDevice();
	const DeviceInterface&				vk								= m_context.getDeviceInterface();
	const VkDeviceSize					size							= 3 * 5 * 7 * 64;
	Move<VkBuffer>						testBuffer;
	Move<VkDeviceMemory>				testBufferMemory;
	VkFormatProperties					properties;

	m_context.getInstanceInterface().getPhysicalDeviceFormatProperties(m_context.getPhysicalDevice(), m_testCase.format, &properties);
	if (!(properties.bufferFeatures & m_testCase.features))
		TCU_THROW(NotSupportedError, "Format not supported");

	// Create buffer
	if (m_testCase.bufferAllocationKind == ALLOCATION_KIND_DEDICATED)
	{
		BufferDedicatedAllocation().createTestBuffer(size, m_testCase.usage, m_context, testBuffer, testBufferMemory);
	}
	else
	{
		BufferSuballocation().createTestBuffer(size, m_testCase.usage, m_context, testBuffer, testBufferMemory);
	}

	{
		// Create buffer view.
		Move<VkBufferView>				bufferView;
		const VkBufferViewCreateInfo	bufferViewCreateInfo			=
		{
			VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,					//	VkStructureType			sType;
			NULL,														//	const void*				pNext;
			(VkBufferViewCreateFlags)0,
			*testBuffer,												//	VkBuffer				buffer;
			m_testCase.format,											//	VkFormat				format;
			m_testCase.offset,											//	VkDeviceSize			offset;
			m_testCase.range,											//	VkDeviceSize			range;
		};

		try
		{
			bufferView = createBufferView(vk, vkDevice, &bufferViewCreateInfo, (const VkAllocationCallbacks*)DE_NULL);
		}
		catch (const vk::Error& error)
		{
			return tcu::TestStatus::fail("Buffer View creation failed! (Error code: " + de::toString(error.getMessage()) + ")");
		}
	}

	// Testing complete view size.
	{
		Move<VkBufferView>				completeBufferView;
		VkBufferViewCreateInfo			completeBufferViewCreateInfo	=
		{
			VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,					//	VkStructureType			sType;
			NULL,														//	const void*				pNext;
			(VkBufferViewCreateFlags)0,
			*testBuffer,												//	VkBuffer				buffer;
			m_testCase.format,											//	VkFormat				format;
			m_testCase.offset,											//	VkDeviceSize			offset;
			size,														//	VkDeviceSize			range;
		};

		try
		{
			completeBufferView = createBufferView(vk, vkDevice, &completeBufferViewCreateInfo, (const VkAllocationCallbacks*)DE_NULL);
		}
		catch (const vk::Error& error)
		{
			return tcu::TestStatus::fail("Buffer View creation failed! (Error code: " + de::toString(error.getMessage()) + ")");
		}
	}

	return tcu::TestStatus::pass("BufferView test");
}

} // anonymous

 tcu::TestCaseGroup* createBufferViewCreateTests						(tcu::TestContext& testCtx)
{
	const VkDeviceSize					range							= VK_WHOLE_SIZE;
	const vk::VkBufferUsageFlags		usage[]							= { vk::VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, vk::VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT };
	const vk::VkFormatFeatureFlags		feature[]						= { vk::VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT, vk::VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT };
	const char* const					usageName[]						= { "uniform", "storage"};

	de::MovePtr<tcu::TestCaseGroup>		bufferViewTests					(new tcu::TestCaseGroup(testCtx, "create", "BufferView Construction Tests"));

	if (!bufferViewTests)
		TCU_THROW(InternalError, "Could not create test group \"create\".");

	de::MovePtr<tcu::TestCaseGroup>		bufferViewAllocationGroupTests[ALLOCATION_KIND_LAST]
																		=
	{
		de::MovePtr<tcu::TestCaseGroup>(new tcu::TestCaseGroup(testCtx, "suballocation", "BufferView Construction Tests for Suballocated Buffer")),
		de::MovePtr<tcu::TestCaseGroup>(new tcu::TestCaseGroup(testCtx, "dedicated_alloc", "BufferView Construction Tests for Dedicatedly Allocated Buffer"))
	};

	for (deUint32 allocationKind = 0; allocationKind < ALLOCATION_KIND_LAST; ++allocationKind)
	for (deUint32 usageNdx = 0; usageNdx < DE_LENGTH_OF_ARRAY(usage); ++usageNdx)
	{
		de::MovePtr<tcu::TestCaseGroup>	usageGroup		(new tcu::TestCaseGroup(testCtx, usageName[usageNdx], ""));

		for (deUint32 format = vk::VK_FORMAT_UNDEFINED + 1; format < VK_CORE_FORMAT_LAST; format++)
		{
			const std::string				formatName		= de::toLower(getFormatName((VkFormat)format)).substr(10);
			de::MovePtr<tcu::TestCaseGroup>	formatGroup		(new tcu::TestCaseGroup(testCtx, "suballocation", "BufferView Construction Tests for Suballocated Buffer"));

			const std::string				testName		= de::toLower(getFormatName((VkFormat)format)).substr(10);
			const std::string				testDescription	= "vkBufferView test " + testName;

			{
				const BufferViewCaseParameters	testParams					=
				{
					static_cast<vk::VkFormat>(format),						// VkFormat					format;
					0,														// VkDeviceSize				offset;
					range,													// VkDeviceSize				range;
					usage[usageNdx],										// VkBufferUsageFlags		usage;
					feature[usageNdx],										// VkFormatFeatureFlags		flags;
					static_cast<AllocationKind>(allocationKind)				// AllocationKind			bufferAllocationKind;
				};

				usageGroup->addChild(new BufferViewTestCase(testCtx, testName.c_str(), testDescription.c_str(), testParams));
			}
		}
		bufferViewAllocationGroupTests[allocationKind]->addChild(usageGroup.release());
	}

	for (deUint32 subgroupNdx = 0u; subgroupNdx < DE_LENGTH_OF_ARRAY(bufferViewAllocationGroupTests); ++subgroupNdx)
	{
		bufferViewTests->addChild(bufferViewAllocationGroupTests[subgroupNdx].release());
	}

	return bufferViewTests.release();
}

} // api
} // vk
