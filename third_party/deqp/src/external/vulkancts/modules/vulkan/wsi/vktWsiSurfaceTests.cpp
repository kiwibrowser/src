/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief VkSurface Tests
 *//*--------------------------------------------------------------------*/

#include "vktWsiSurfaceTests.hpp"

#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include "vkDefs.hpp"
#include "vkPlatform.hpp"
#include "vkStrUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkDeviceUtil.hpp"
#include "vkPrograms.hpp"
#include "vkTypeUtil.hpp"
#include "vkWsiPlatform.hpp"
#include "vkWsiUtil.hpp"
#include "vkAllocationCallbackUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuPlatform.hpp"
#include "tcuResultCollector.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "deMemory.h"

namespace vk
{

inline bool operator!= (const VkSurfaceFormatKHR& a, const VkSurfaceFormatKHR& b)
{
	return (a.format != b.format) || (a.colorSpace != b.colorSpace);
}

inline bool operator== (const VkSurfaceFormatKHR& a, const VkSurfaceFormatKHR& b)
{
	return !(a != b);
}

inline bool operator!= (const VkExtent2D& a, const VkExtent2D& b)
{
	return (a.width != b.width) || (a.height != b.height);
}

inline bool operator!= (const VkSurfaceCapabilitiesKHR& a, const VkSurfaceCapabilitiesKHR& b)
{
	return (a.minImageCount				!= b.minImageCount)				||
		   (a.maxImageCount				!= b.maxImageCount)				||
		   (a.currentExtent				!= b.currentExtent)				||
		   (a.minImageExtent			!= b.minImageExtent)			||
		   (a.maxImageExtent			!= b.maxImageExtent)			||
		   (a.maxImageArrayLayers		!= b.maxImageArrayLayers)		||
		   (a.supportedTransforms		!= b.supportedTransforms)		||
		   (a.currentTransform			!= b.currentTransform)			||
		   (a.supportedCompositeAlpha	!= b.supportedCompositeAlpha)	||
		   (a.supportedUsageFlags		!= b.supportedUsageFlags);
}

} // vk

namespace vkt
{
namespace wsi
{

namespace
{

using namespace vk;
using namespace vk::wsi;

using tcu::TestLog;
using tcu::Maybe;
using tcu::UVec2;

using de::MovePtr;
using de::UniquePtr;

using std::string;
using std::vector;

enum
{
	SURFACE_EXTENT_DETERMINED_BY_SWAPCHAIN_MAGIC	= 0xffffffff
};

template<typename T>
class CheckIncompleteResult
{
public:
	virtual			~CheckIncompleteResult	(void) {}
	virtual void	getResult				(const InstanceInterface& vki, const VkPhysicalDevice physDevice, const VkSurfaceKHR surface, T* data) = 0;

	void operator() (tcu::ResultCollector&		results,
					 const InstanceInterface&	vki,
					 const VkPhysicalDevice		physDevice,
					 const VkSurfaceKHR			surface,
					 const std::size_t			expectedCompleteSize)
	{
		if (expectedCompleteSize == 0)
			return;

		vector<T>		outputData	(expectedCompleteSize);
		const deUint32	usedSize	= static_cast<deUint32>(expectedCompleteSize / 3);

		ValidateQueryBits::fillBits(outputData.begin(), outputData.end());	// unused entries should have this pattern intact
		m_count		= usedSize;
		m_result	= VK_SUCCESS;

		getResult(vki, physDevice, surface, &outputData[0]);				// update m_count and m_result

		if (m_count != usedSize || m_result != VK_INCOMPLETE || !ValidateQueryBits::checkBits(outputData.begin() + m_count, outputData.end()))
			results.fail("Query didn't return VK_INCOMPLETE");
	}

protected:
	deUint32	m_count;
	VkResult	m_result;
};

struct CheckPhysicalDeviceSurfaceFormatsIncompleteResult : public CheckIncompleteResult<VkSurfaceFormatKHR>
{
	void getResult (const InstanceInterface& vki, const VkPhysicalDevice physDevice, const VkSurfaceKHR surface, VkSurfaceFormatKHR* data)
	{
		m_result = vki.getPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &m_count, data);
	}
};

struct CheckPhysicalDeviceSurfacePresentModesIncompleteResult : public CheckIncompleteResult<VkPresentModeKHR>
{
	void getResult (const InstanceInterface& vki, const VkPhysicalDevice physDevice, const VkSurfaceKHR surface, VkPresentModeKHR* data)
	{
		m_result = vki.getPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &m_count, data);
	}
};

typedef vector<VkExtensionProperties> Extensions;

void checkAllSupported (const Extensions& supportedExtensions, const vector<string>& requiredExtensions)
{
	for (vector<string>::const_iterator requiredExtName = requiredExtensions.begin();
		 requiredExtName != requiredExtensions.end();
		 ++requiredExtName)
	{
		if (!isExtensionSupported(supportedExtensions, RequiredExtension(*requiredExtName)))
			TCU_THROW(NotSupportedError, (*requiredExtName + " is not supported").c_str());
	}
}

Move<VkInstance> createInstanceWithWsi (const PlatformInterface&		vkp,
										const Extensions&				supportedExtensions,
										Type							wsiType,
										const vector<string>			extraExtensions,
										const VkAllocationCallbacks*	pAllocator	= DE_NULL)
{
	vector<string>	extensions	= extraExtensions;

	extensions.push_back("VK_KHR_surface");
	extensions.push_back(getExtensionName(wsiType));

	checkAllSupported(supportedExtensions, extensions);

	return createDefaultInstance(vkp, vector<string>(), extensions, pAllocator);
}

struct InstanceHelper
{
	const vector<VkExtensionProperties>	supportedExtensions;
	Unique<VkInstance>					instance;
	const InstanceDriver				vki;

	InstanceHelper (Context& context, Type wsiType, const VkAllocationCallbacks* pAllocator = DE_NULL)
		: supportedExtensions	(enumerateInstanceExtensionProperties(context.getPlatformInterface(),
																	  DE_NULL))
		, instance				(createInstanceWithWsi(context.getPlatformInterface(),
													   supportedExtensions,
													   wsiType,
													   vector<string>(),
													   pAllocator))
		, vki					(context.getPlatformInterface(), *instance)
	{}

	InstanceHelper (Context& context, Type wsiType, const vector<string>& extensions, const VkAllocationCallbacks* pAllocator = DE_NULL)
		: supportedExtensions	(enumerateInstanceExtensionProperties(context.getPlatformInterface(),
																	  DE_NULL))
		, instance				(createInstanceWithWsi(context.getPlatformInterface(),
													   supportedExtensions,
													   wsiType,
													   extensions,
													   pAllocator))
		, vki					(context.getPlatformInterface(), *instance)
	{}
};

MovePtr<Display> createDisplay (const vk::Platform&	platform,
								const Extensions&	supportedExtensions,
								Type				wsiType)
{
	try
	{
		return MovePtr<Display>(platform.createWsiDisplay(wsiType));
	}
	catch (const tcu::NotSupportedError& e)
	{
		if (isExtensionSupported(supportedExtensions, RequiredExtension(getExtensionName(wsiType))))
		{
			// If VK_KHR_{platform}_surface was supported, vk::Platform implementation
			// must support creating native display & window for that WSI type.
			throw tcu::TestError(e.getMessage());
		}
		else
			throw;
	}
}

MovePtr<Window> createWindow (const Display& display, const Maybe<UVec2>& initialSize)
{
	try
	{
		return MovePtr<Window>(display.createWindow(initialSize));
	}
	catch (const tcu::NotSupportedError& e)
	{
		// See createDisplay - assuming that wsi::Display was supported platform port
		// should also support creating a window.
		throw tcu::TestError(e.getMessage());
	}
}

struct NativeObjects
{
	const UniquePtr<Display>	display;
	const UniquePtr<Window>		window;

	NativeObjects (Context&				context,
				   const Extensions&	supportedExtensions,
				   Type					wsiType,
				   const Maybe<UVec2>&	initialWindowSize = tcu::nothing<UVec2>())
		: display	(createDisplay(context.getTestContext().getPlatform().getVulkanPlatform(), supportedExtensions, wsiType))
		, window	(createWindow(*display, initialWindowSize))
	{}
};

tcu::TestStatus createSurfaceTest (Context& context, Type wsiType)
{
	const InstanceHelper		instHelper	(context, wsiType);
	const NativeObjects			native		(context, instHelper.supportedExtensions, wsiType);
	const Unique<VkSurfaceKHR>	surface		(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));

	return tcu::TestStatus::pass("Creating surface succeeded");
}

tcu::TestStatus createSurfaceCustomAllocatorTest (Context& context, Type wsiType)
{
	AllocationCallbackRecorder	allocationRecorder	(getSystemAllocator());
	tcu::TestLog&				log					= context.getTestContext().getLog();

	{
		const InstanceHelper		instHelper	(context, wsiType, allocationRecorder.getCallbacks());
		const NativeObjects			native		(context, instHelper.supportedExtensions, wsiType);
		const Unique<VkSurfaceKHR>	surface		(createSurface(instHelper.vki,
															   *instHelper.instance,
															   wsiType,
															   *native.display,
															   *native.window,
															   allocationRecorder.getCallbacks()));

		if (!validateAndLog(log,
							allocationRecorder,
							(1u<<VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)		|
							(1u<<VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE)))
			return tcu::TestStatus::fail("Detected invalid system allocation callback");
	}

	if (!validateAndLog(log, allocationRecorder, 0u))
		return tcu::TestStatus::fail("Detected invalid system allocation callback");

	if (allocationRecorder.getRecordsBegin() == allocationRecorder.getRecordsEnd())
		return tcu::TestStatus(QP_TEST_RESULT_QUALITY_WARNING, "Allocation callbacks were not used");
	else
		return tcu::TestStatus::pass("Creating surface succeeded using custom allocator");
}

tcu::TestStatus createSurfaceSimulateOOMTest (Context& context, Type wsiType)
{
	tcu::TestLog&	log	= context.getTestContext().getLog();

	for (deUint32 numPassingAllocs = 0; numPassingAllocs <= 1024u; ++numPassingAllocs)
	{
		AllocationCallbackRecorder	allocationRecorder	(getSystemAllocator());
		DeterministicFailAllocator	failingAllocator	(allocationRecorder.getCallbacks(),
														 DeterministicFailAllocator::MODE_DO_NOT_COUNT,
														 0);
		bool						gotOOM				= false;

		log << TestLog::Message << "Testing with " << numPassingAllocs << " first allocations succeeding" << TestLog::EndMessage;

		try
		{
			const InstanceHelper		instHelper	(context, wsiType, failingAllocator.getCallbacks());

			// OOM is not simulated for VkInstance as we don't want to spend time
			// testing OOM paths inside instance creation.
			failingAllocator.reset(DeterministicFailAllocator::MODE_COUNT_AND_FAIL, numPassingAllocs);

			const NativeObjects			native		(context, instHelper.supportedExtensions, wsiType);
			const Unique<VkSurfaceKHR>	surface		(createSurface(instHelper.vki,
																   *instHelper.instance,
																   wsiType,
																   *native.display,
																   *native.window,
																   failingAllocator.getCallbacks()));

			if (!validateAndLog(log,
								allocationRecorder,
								(1u<<VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)		|
								(1u<<VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE)))
				return tcu::TestStatus::fail("Detected invalid system allocation callback");
		}
		catch (const OutOfMemoryError& e)
		{
			log << TestLog::Message << "Got " << e.getError() << TestLog::EndMessage;
			gotOOM = true;
		}

		if (!validateAndLog(log, allocationRecorder, 0u))
			return tcu::TestStatus::fail("Detected invalid system allocation callback");

		if (!gotOOM)
		{
			log << TestLog::Message << "Creating surface succeeded!" << TestLog::EndMessage;

			if (numPassingAllocs == 0)
				return tcu::TestStatus(QP_TEST_RESULT_QUALITY_WARNING, "Allocation callbacks were not used");
			else
				return tcu::TestStatus::pass("OOM simulation completed");
		}
	}

	return tcu::TestStatus(QP_TEST_RESULT_QUALITY_WARNING, "Creating surface did not succeed, callback limit exceeded");
}

deUint32 getNumQueueFamilies (const InstanceInterface& vki, VkPhysicalDevice physicalDevice)
{
	deUint32	numFamilies		= 0;

	vki.getPhysicalDeviceQueueFamilyProperties(physicalDevice, &numFamilies, DE_NULL);

	return numFamilies;
}

tcu::TestStatus querySurfaceSupportTest (Context& context, Type wsiType)
{
	tcu::TestLog&					log						= context.getTestContext().getLog();
	tcu::ResultCollector			results					(log);

	const InstanceHelper			instHelper				(context, wsiType);
	const NativeObjects				native					(context, instHelper.supportedExtensions, wsiType);
	const Unique<VkSurfaceKHR>		surface					(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));
	const vector<VkPhysicalDevice>	physicalDevices			= enumeratePhysicalDevices(instHelper.vki, *instHelper.instance);

	// On Android surface must be supported by all devices and queue families
	const bool						expectSupportedOnAll	= wsiType == TYPE_ANDROID;

	for (size_t deviceNdx = 0; deviceNdx < physicalDevices.size(); ++deviceNdx)
	{
		const VkPhysicalDevice		physicalDevice		= physicalDevices[deviceNdx];
		const deUint32				numQueueFamilies	= getNumQueueFamilies(instHelper.vki, physicalDevice);

		for (deUint32 queueFamilyNdx = 0; queueFamilyNdx < numQueueFamilies; ++queueFamilyNdx)
		{
			const VkBool32	isSupported		= getPhysicalDeviceSurfaceSupport(instHelper.vki, physicalDevice, queueFamilyNdx, *surface);

			log << TestLog::Message << "Device " << deviceNdx << ", queue family " << queueFamilyNdx << ": "
									<< (isSupported == VK_FALSE ? "NOT " : "") << "supported"
				<< TestLog::EndMessage;

			if (expectSupportedOnAll && !isSupported)
				results.fail("Surface must be supported by all devices and queue families");
		}
	}

	return tcu::TestStatus(results.getResult(), results.getMessage());
}

bool isSupportedByAnyQueue (const InstanceInterface& vki, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	const deUint32	numQueueFamilies	= getNumQueueFamilies(vki, physicalDevice);

	for (deUint32 queueFamilyNdx = 0; queueFamilyNdx < numQueueFamilies; ++queueFamilyNdx)
	{
		if (getPhysicalDeviceSurfaceSupport(vki, physicalDevice, queueFamilyNdx, surface) != VK_FALSE)
			return true;
	}

	return false;
}

void validateSurfaceCapabilities (tcu::ResultCollector& results, const VkSurfaceCapabilitiesKHR& capabilities)
{
	results.check(capabilities.minImageCount > 0,
				  "minImageCount must be larger than 0");

	results.check(capabilities.minImageExtent.width > 0 &&
				  capabilities.minImageExtent.height > 0,
				  "minImageExtent dimensions must be larger than 0");

	results.check(capabilities.maxImageExtent.width > 0 &&
				  capabilities.maxImageExtent.height > 0,
				  "maxImageExtent dimensions must be larger than 0");

	results.check(capabilities.minImageExtent.width <= capabilities.maxImageExtent.width &&
				  capabilities.minImageExtent.height <= capabilities.maxImageExtent.height,
				  "maxImageExtent must be larger or equal to minImageExtent");

	if (capabilities.currentExtent.width != SURFACE_EXTENT_DETERMINED_BY_SWAPCHAIN_MAGIC ||
		capabilities.currentExtent.height != SURFACE_EXTENT_DETERMINED_BY_SWAPCHAIN_MAGIC)
	{
		results.check(capabilities.currentExtent.width > 0 &&
					  capabilities.currentExtent.height > 0,
					  "currentExtent dimensions must be larger than 0");

		results.check(de::inRange(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width) &&
					  de::inRange(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
					  "currentExtent is not in supported extent limits");
	}

	results.check(capabilities.maxImageArrayLayers > 0,
				  "maxImageArrayLayers must be larger than 0");

	results.check((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0,
				  "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT must be set in supportedUsageFlags");

	results.check(capabilities.supportedTransforms != 0,
				  "At least one transform must be supported");

	results.check(dePop32(capabilities.currentTransform) != 0,
				  "Invalid currentTransform");

	results.check((capabilities.supportedTransforms & capabilities.currentTransform) != 0,
				  "currentTransform is not supported by surface");

	results.check(capabilities.supportedCompositeAlpha != 0,
				  "At least one alpha mode must be supported");
}

tcu::TestStatus querySurfaceCapabilitiesTest (Context& context, Type wsiType)
{
	tcu::TestLog&					log						= context.getTestContext().getLog();
	tcu::ResultCollector			results					(log);

	const InstanceHelper			instHelper				(context, wsiType);
	const NativeObjects				native					(context, instHelper.supportedExtensions, wsiType);
	const Unique<VkSurfaceKHR>		surface					(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));
	const vector<VkPhysicalDevice>	physicalDevices			= enumeratePhysicalDevices(instHelper.vki, *instHelper.instance);

	for (size_t deviceNdx = 0; deviceNdx < physicalDevices.size(); ++deviceNdx)
	{
		if (isSupportedByAnyQueue(instHelper.vki, physicalDevices[deviceNdx], *surface))
		{
			const VkSurfaceCapabilitiesKHR	capabilities	= getPhysicalDeviceSurfaceCapabilities(instHelper.vki,
																								   physicalDevices[deviceNdx],
																								   *surface);

			log << TestLog::Message << "Device " << deviceNdx << ": " << capabilities << TestLog::EndMessage;

			validateSurfaceCapabilities(results, capabilities);
		}
		// else skip query as surface is not supported by the device
	}

	return tcu::TestStatus(results.getResult(), results.getMessage());
}

tcu::TestStatus querySurfaceCapabilities2Test (Context& context, Type wsiType)
{
	tcu::TestLog&					log						= context.getTestContext().getLog();
	tcu::ResultCollector			results					(log);

	const InstanceHelper			instHelper				(context, wsiType, vector<string>(1, string("VK_KHR_get_surface_capabilities2")));
	const NativeObjects				native					(context, instHelper.supportedExtensions, wsiType);
	const Unique<VkSurfaceKHR>		surface					(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));
	const vector<VkPhysicalDevice>	physicalDevices			= enumeratePhysicalDevices(instHelper.vki, *instHelper.instance);

	for (size_t deviceNdx = 0; deviceNdx < physicalDevices.size(); ++deviceNdx)
	{
		if (isSupportedByAnyQueue(instHelper.vki, physicalDevices[deviceNdx], *surface))
		{
			const VkSurfaceCapabilitiesKHR	refCapabilities	= getPhysicalDeviceSurfaceCapabilities(instHelper.vki,
																								   physicalDevices[deviceNdx],
																								   *surface);
			VkSurfaceCapabilities2KHR		extCapabilities;

			deMemset(&extCapabilities, 0xcd, sizeof(VkSurfaceCapabilities2KHR));
			extCapabilities.sType	= VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
			extCapabilities.pNext	= DE_NULL;

			{
				const VkPhysicalDeviceSurfaceInfo2KHR	surfaceInfo	=
				{
					VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
					DE_NULL,
					*surface
				};
				VkPhysicalDeviceSurfaceInfo2KHR			infoCopy;

				deMemcpy(&infoCopy, &surfaceInfo, sizeof(VkPhysicalDeviceSurfaceInfo2KHR));

				VK_CHECK(instHelper.vki.getPhysicalDeviceSurfaceCapabilities2KHR(physicalDevices[deviceNdx], &surfaceInfo, &extCapabilities));

				results.check(deMemoryEqual(&surfaceInfo, &infoCopy, sizeof(VkPhysicalDeviceSurfaceInfo2KHR)) == DE_TRUE, "Driver wrote into input struct");
			}

			results.check(extCapabilities.sType == VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR &&
						  extCapabilities.pNext == DE_NULL,
						  "sType/pNext modified");

			if (refCapabilities != extCapabilities.surfaceCapabilities)
			{
				log << TestLog::Message
					<< "Device " << deviceNdx
					<< ": expected " << refCapabilities
					<< ", got " << extCapabilities.surfaceCapabilities
					<< TestLog::EndMessage;
				results.fail("Mismatch between VK_KHR_surface and VK_KHR_surface2 query results");
			}
		}
	}

	return tcu::TestStatus(results.getResult(), results.getMessage());
}

void validateSurfaceFormats (tcu::ResultCollector& results, Type wsiType, const vector<VkSurfaceFormatKHR>& formats)
{
	const VkSurfaceFormatKHR*	requiredFormats		= DE_NULL;
	size_t						numRequiredFormats	= 0;

	if (wsiType == TYPE_ANDROID)
	{
		static const VkSurfaceFormatKHR s_androidFormats[] =
		{
			{ VK_FORMAT_R8G8B8A8_UNORM,			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR	},
			{ VK_FORMAT_R8G8B8A8_SRGB,			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR	},
			{ VK_FORMAT_R5G6B5_UNORM_PACK16,	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR	}
		};

		requiredFormats		= &s_androidFormats[0];
		numRequiredFormats	= DE_LENGTH_OF_ARRAY(s_androidFormats);
	}

	for (size_t ndx = 0; ndx < numRequiredFormats; ++ndx)
	{
		const VkSurfaceFormatKHR&	requiredFormat	= requiredFormats[ndx];

		if (!de::contains(formats.begin(), formats.end(), requiredFormat))
			results.fail(de::toString(requiredFormat) + " not supported");
	}

	// Check that there are no duplicates
	for (size_t ndx = 1; ndx < formats.size(); ++ndx)
	{
		if (de::contains(formats.begin(), formats.begin() + ndx, formats[ndx]))
			results.fail("Found duplicate entry " + de::toString(formats[ndx]));
	}
}

tcu::TestStatus querySurfaceFormatsTest (Context& context, Type wsiType)
{
	tcu::TestLog&					log				= context.getTestContext().getLog();
	tcu::ResultCollector			results			(log);

	const InstanceHelper			instHelper		(context, wsiType);
	const NativeObjects				native			(context, instHelper.supportedExtensions, wsiType);
	const Unique<VkSurfaceKHR>		surface			(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));
	const vector<VkPhysicalDevice>	physicalDevices	= enumeratePhysicalDevices(instHelper.vki, *instHelper.instance);

	for (size_t deviceNdx = 0; deviceNdx < physicalDevices.size(); ++deviceNdx)
	{
		if (isSupportedByAnyQueue(instHelper.vki, physicalDevices[deviceNdx], *surface))
		{
			const vector<VkSurfaceFormatKHR>	formats	= getPhysicalDeviceSurfaceFormats(instHelper.vki,
																						  physicalDevices[deviceNdx],
																						  *surface);

			log << TestLog::Message << "Device " << deviceNdx << ": " << tcu::formatArray(formats.begin(), formats.end()) << TestLog::EndMessage;

			validateSurfaceFormats(results, wsiType, formats);
			CheckPhysicalDeviceSurfaceFormatsIncompleteResult()(results, instHelper.vki, physicalDevices[deviceNdx], *surface, formats.size());
		}
		// else skip query as surface is not supported by the device
	}

	return tcu::TestStatus(results.getResult(), results.getMessage());
}

tcu::TestStatus querySurfaceFormats2Test (Context& context, Type wsiType)
{
	tcu::TestLog&					log				= context.getTestContext().getLog();
	tcu::ResultCollector			results			(log);

	const InstanceHelper			instHelper		(context, wsiType, vector<string>(1, string("VK_KHR_get_surface_capabilities2")));
	const NativeObjects				native			(context, instHelper.supportedExtensions, wsiType);
	const Unique<VkSurfaceKHR>		surface			(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));
	const vector<VkPhysicalDevice>	physicalDevices	= enumeratePhysicalDevices(instHelper.vki, *instHelper.instance);

	for (size_t deviceNdx = 0; deviceNdx < physicalDevices.size(); ++deviceNdx)
	{
		if (isSupportedByAnyQueue(instHelper.vki, physicalDevices[deviceNdx], *surface))
		{
			const vector<VkSurfaceFormatKHR>		refFormats	= getPhysicalDeviceSurfaceFormats(instHelper.vki,
																								  physicalDevices[deviceNdx],
																								  *surface);
			const VkPhysicalDeviceSurfaceInfo2KHR	surfaceInfo	=
			{
				VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
				DE_NULL,
				*surface
			};
			deUint32								numFormats	= 0;

			VK_CHECK(instHelper.vki.getPhysicalDeviceSurfaceFormats2KHR(physicalDevices[deviceNdx], &surfaceInfo, &numFormats, DE_NULL));

			if ((size_t)numFormats != refFormats.size())
				results.fail("vkGetPhysicalDeviceSurfaceFormats2KHR() returned different number of formats");

			if (numFormats > 0)
			{
				vector<VkSurfaceFormat2KHR>	formats	(numFormats);

				for (size_t ndx = 0; ndx < formats.size(); ++ndx)
				{
					formats[ndx].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
					formats[ndx].pNext = DE_NULL;
				}

				VK_CHECK(instHelper.vki.getPhysicalDeviceSurfaceFormats2KHR(physicalDevices[deviceNdx], &surfaceInfo, &numFormats, &formats[0]));

				if ((size_t)numFormats != formats.size())
					results.fail("Format count changed between calls");

				{
					vector<VkSurfaceFormatKHR>	extFormats	(formats.size());

					for (size_t ndx = 0; ndx < formats.size(); ++ndx)
					{
						results.check(formats[ndx].sType == VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR &&
									  formats[ndx].pNext == DE_NULL,
									  "sType/pNext modified");
						extFormats[ndx] = formats[ndx].surfaceFormat;
					}

					for (size_t ndx = 0; ndx < refFormats.size(); ++ndx)
					{
						if (!de::contains(extFormats.begin(), extFormats.end(), refFormats[ndx]))
							results.fail(de::toString(refFormats[ndx]) + " missing from extended query");
					}
				}

				// Check VK_INCOMPLETE
				{
					vector<VkSurfaceFormat2KHR>	formatsClone	(formats);
					deUint32					numToSupply		= numFormats/2;
					VkResult					queryResult;

					ValidateQueryBits::fillBits(formatsClone.begin() + numToSupply, formatsClone.end());

					queryResult = instHelper.vki.getPhysicalDeviceSurfaceFormats2KHR(physicalDevices[deviceNdx], &surfaceInfo, &numToSupply, &formatsClone[0]);

					results.check(queryResult == VK_INCOMPLETE, "Expected VK_INCOMPLETE");
					results.check(ValidateQueryBits::checkBits(formatsClone.begin() + numToSupply, formatsClone.end()),
								  "Driver wrote past last element");

					for (size_t ndx = 0; ndx < (size_t)numToSupply; ++ndx)
					{
						results.check(formatsClone[ndx].sType == VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR &&
									  formatsClone[ndx].pNext == DE_NULL &&
									  formatsClone[ndx].surfaceFormat == formats[ndx].surfaceFormat,
									  "Returned element " + de::toString(ndx) + " is different");
					}
				}
			}
		}
		// else skip query as surface is not supported by the device
	}

	return tcu::TestStatus(results.getResult(), results.getMessage());
}

void validateSurfacePresentModes (tcu::ResultCollector& results, Type wsiType, const vector<VkPresentModeKHR>& modes)
{
	results.check(de::contains(modes.begin(), modes.end(), VK_PRESENT_MODE_FIFO_KHR),
				  "VK_PRESENT_MODE_FIFO_KHR is not supported");

	if (wsiType == TYPE_ANDROID)
		results.check(de::contains(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR),
					  "VK_PRESENT_MODE_MAILBOX_KHR is not supported");
}

tcu::TestStatus querySurfacePresentModesTest (Context& context, Type wsiType)
{
	tcu::TestLog&					log				= context.getTestContext().getLog();
	tcu::ResultCollector			results			(log);

	const InstanceHelper			instHelper		(context, wsiType);
	const NativeObjects				native			(context, instHelper.supportedExtensions, wsiType);
	const Unique<VkSurfaceKHR>		surface			(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));
	const vector<VkPhysicalDevice>	physicalDevices	= enumeratePhysicalDevices(instHelper.vki, *instHelper.instance);

	for (size_t deviceNdx = 0; deviceNdx < physicalDevices.size(); ++deviceNdx)
	{
		if (isSupportedByAnyQueue(instHelper.vki, physicalDevices[deviceNdx], *surface))
		{
			const vector<VkPresentModeKHR>	modes	= getPhysicalDeviceSurfacePresentModes(instHelper.vki, physicalDevices[deviceNdx], *surface);

			log << TestLog::Message << "Device " << deviceNdx << ": " << tcu::formatArray(modes.begin(), modes.end()) << TestLog::EndMessage;

			validateSurfacePresentModes(results, wsiType, modes);
			CheckPhysicalDeviceSurfacePresentModesIncompleteResult()(results, instHelper.vki, physicalDevices[deviceNdx], *surface, modes.size());
		}
		// else skip query as surface is not supported by the device
	}

	return tcu::TestStatus(results.getResult(), results.getMessage());
}

tcu::TestStatus createSurfaceInitialSizeTest (Context& context, Type wsiType)
{
	tcu::TestLog&					log				= context.getTestContext().getLog();
	tcu::ResultCollector			results			(log);

	const InstanceHelper			instHelper		(context, wsiType);

	const UniquePtr<Display>		nativeDisplay	(createDisplay(context.getTestContext().getPlatform().getVulkanPlatform(),
																   instHelper.supportedExtensions,
																   wsiType));

	const vector<VkPhysicalDevice>	physicalDevices	= enumeratePhysicalDevices(instHelper.vki, *instHelper.instance);
	const UVec2						sizes[]			=
	{
		UVec2(64, 64),
		UVec2(124, 119),
		UVec2(256, 512)
	};

	DE_ASSERT(getPlatformProperties(wsiType).features & PlatformProperties::FEATURE_INITIAL_WINDOW_SIZE);

	for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes); ++sizeNdx)
	{
		const UVec2&				testSize		= sizes[sizeNdx];
		const UniquePtr<Window>		nativeWindow	(createWindow(*nativeDisplay, tcu::just(testSize)));
		const Unique<VkSurfaceKHR>	surface			(createSurface(instHelper.vki, *instHelper.instance, wsiType, *nativeDisplay, *nativeWindow));

		for (size_t deviceNdx = 0; deviceNdx < physicalDevices.size(); ++deviceNdx)
		{
			if (isSupportedByAnyQueue(instHelper.vki, physicalDevices[deviceNdx], *surface))
			{
				const VkSurfaceCapabilitiesKHR	capabilities	= getPhysicalDeviceSurfaceCapabilities(instHelper.vki, physicalDevices[deviceNdx], *surface);

				// \note Assumes that surface size is NOT set by swapchain if initial window size is honored by platform
				results.check(capabilities.currentExtent.width == testSize.x() &&
								capabilities.currentExtent.height == testSize.y(),
								"currentExtent " + de::toString(capabilities.currentExtent) + " doesn't match requested size " + de::toString(testSize));
			}
		}
	}

	return tcu::TestStatus(results.getResult(), results.getMessage());
}

tcu::TestStatus resizeSurfaceTest (Context& context, Type wsiType)
{
	tcu::TestLog&					log				= context.getTestContext().getLog();
	tcu::ResultCollector			results			(log);

	const InstanceHelper			instHelper		(context, wsiType);

	const UniquePtr<Display>		nativeDisplay	(createDisplay(context.getTestContext().getPlatform().getVulkanPlatform(),
																   instHelper.supportedExtensions,
																   wsiType));
	UniquePtr<Window>				nativeWindow	(createWindow(*nativeDisplay, tcu::nothing<UVec2>()));

	const vector<VkPhysicalDevice>	physicalDevices	= enumeratePhysicalDevices(instHelper.vki, *instHelper.instance);
	const Unique<VkSurfaceKHR>		surface			(createSurface(instHelper.vki, *instHelper.instance, wsiType, *nativeDisplay, *nativeWindow));

	const UVec2						sizes[]			=
	{
		UVec2(64, 64),
		UVec2(124, 119),
		UVec2(256, 512)
	};

	DE_ASSERT(getPlatformProperties(wsiType).features & PlatformProperties::FEATURE_RESIZE_WINDOW);

	for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes); ++sizeNdx)
	{
		const UVec2		testSize	= sizes[sizeNdx];

		try
		{
			nativeWindow->resize(testSize);
		}
		catch (const tcu::Exception& e)
		{
			// Make sure all exception types result in a test failure
			results.fail(e.getMessage());
		}

		for (size_t deviceNdx = 0; deviceNdx < physicalDevices.size(); ++deviceNdx)
		{
			if (isSupportedByAnyQueue(instHelper.vki, physicalDevices[deviceNdx], *surface))
			{
				const VkSurfaceCapabilitiesKHR	capabilities	= getPhysicalDeviceSurfaceCapabilities(instHelper.vki, physicalDevices[deviceNdx], *surface);

				// \note Assumes that surface size is NOT set by swapchain if initial window size is honored by platform
				results.check(capabilities.currentExtent.width == testSize.x() &&
								capabilities.currentExtent.height == testSize.y(),
								"currentExtent " + de::toString(capabilities.currentExtent) + " doesn't match requested size " + de::toString(testSize));
			}
		}
	}

	return tcu::TestStatus(results.getResult(), results.getMessage());
}

tcu::TestStatus destroyNullHandleSurfaceTest (Context& context, Type wsiType)
{
	const InstanceHelper	instHelper	(context, wsiType);
	const VkSurfaceKHR		nullHandle	= DE_NULL;

	// Default allocator
	instHelper.vki.destroySurfaceKHR(*instHelper.instance, nullHandle, DE_NULL);

	// Custom allocator
	{
		AllocationCallbackRecorder	recordingAllocator	(getSystemAllocator(), 1u);

		instHelper.vki.destroySurfaceKHR(*instHelper.instance, nullHandle, recordingAllocator.getCallbacks());

		if (recordingAllocator.getNumRecords() != 0u)
			return tcu::TestStatus::fail("Implementation allocated/freed the memory");
	}

	return tcu::TestStatus::pass("Destroying a VK_NULL_HANDLE surface has no effect");
}

} // anonymous

void createSurfaceTests (tcu::TestCaseGroup* testGroup, vk::wsi::Type wsiType)
{
	const PlatformProperties&	platformProperties	= getPlatformProperties(wsiType);

	addFunctionCase(testGroup, "create",					"Create surface",						createSurfaceTest,					wsiType);
	addFunctionCase(testGroup, "create_custom_allocator",	"Create surface with custom allocator",	createSurfaceCustomAllocatorTest,	wsiType);
	addFunctionCase(testGroup, "create_simulate_oom",		"Create surface with simulating OOM",	createSurfaceSimulateOOMTest,		wsiType);
	addFunctionCase(testGroup, "query_support",				"Query surface support",				querySurfaceSupportTest,			wsiType);
	addFunctionCase(testGroup, "query_capabilities",		"Query surface capabilities",			querySurfaceCapabilitiesTest,		wsiType);
	addFunctionCase(testGroup, "query_capabilities2",		"Query extended surface capabilities",	querySurfaceCapabilities2Test,		wsiType);
	addFunctionCase(testGroup, "query_formats",				"Query surface formats",				querySurfaceFormatsTest,			wsiType);
	addFunctionCase(testGroup, "query_formats2",			"Query extended surface formats",		querySurfaceFormats2Test,			wsiType);
	addFunctionCase(testGroup, "query_present_modes",		"Query surface present modes",			querySurfacePresentModesTest,		wsiType);
	addFunctionCase(testGroup, "destroy_null_handle",		"Destroy VK_NULL_HANDLE surface",		destroyNullHandleSurfaceTest,		wsiType);

	if ((platformProperties.features & PlatformProperties::FEATURE_INITIAL_WINDOW_SIZE) != 0)
		addFunctionCase(testGroup, "initial_size",	"Create surface with initial window size set",	createSurfaceInitialSizeTest,	wsiType);

	if ((platformProperties.features & PlatformProperties::FEATURE_RESIZE_WINDOW) != 0)
		addFunctionCase(testGroup, "resize",		"Resize window and surface",					resizeSurfaceTest,				wsiType);
}

} // wsi
} // vkt
