/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 Google Inc.
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
 * \brief VkSwapchain Tests
 *//*--------------------------------------------------------------------*/

#include "vktWsiSwapchainTests.hpp"

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
#include "deArrayUtil.hpp"
#include "deSharedPtr.hpp"

#include <limits>

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
										const VkAllocationCallbacks*	pAllocator	= DE_NULL)
{
	vector<string>	extensions;

	extensions.push_back("VK_KHR_surface");
	extensions.push_back(getExtensionName(wsiType));

	// VK_EXT_swapchain_colorspace adds new surface formats. Driver can enumerate
	// the formats regardless of whether VK_EXT_swapchain_colorspace was enabled,
	// but using them without enabling the extension is not allowed. Thus we have
	// two options:
	//
	// 1) Filter out non-core formats to stay within valid usage.
	//
	// 2) Enable VK_EXT_swapchain colorspace if advertised by the driver.
	//
	// We opt for (2) as it provides basic coverage for the extension as a bonus.
	if (isExtensionSupported(supportedExtensions, RequiredExtension("VK_EXT_swapchain_colorspace")))
		extensions.push_back("VK_EXT_swapchain_colorspace");

	checkAllSupported(supportedExtensions, extensions);

	return createDefaultInstance(vkp, vector<string>(), extensions, pAllocator);
}

VkPhysicalDeviceFeatures getDeviceFeaturesForWsi (void)
{
	VkPhysicalDeviceFeatures features;
	deMemset(&features, 0, sizeof(features));
	return features;
}

Move<VkDevice> createDeviceWithWsi (const InstanceInterface&		vki,
									VkPhysicalDevice				physicalDevice,
									const Extensions&				supportedExtensions,
									const deUint32					queueFamilyIndex,
									const VkAllocationCallbacks*	pAllocator = DE_NULL)
{
	const float						queuePriorities[]	= { 1.0f };
	const VkDeviceQueueCreateInfo	queueInfos[]		=
	{
		{
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			DE_NULL,
			(VkDeviceQueueCreateFlags)0,
			queueFamilyIndex,
			DE_LENGTH_OF_ARRAY(queuePriorities),
			&queuePriorities[0]
		}
	};
	const VkPhysicalDeviceFeatures	features		= getDeviceFeaturesForWsi();
	const char* const				extensions[]	= { "VK_KHR_swapchain" };
	const VkDeviceCreateInfo		deviceParams	=
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		DE_NULL,
		(VkDeviceCreateFlags)0,
		DE_LENGTH_OF_ARRAY(queueInfos),
		&queueInfos[0],
		0u,									// enabledLayerCount
		DE_NULL,							// ppEnabledLayerNames
		DE_LENGTH_OF_ARRAY(extensions),		// enabledExtensionCount
		DE_ARRAY_BEGIN(extensions),			// ppEnabledExtensionNames
		&features
	};

	if (!isExtensionSupported(supportedExtensions, RequiredExtension(extensions[0])))
		TCU_THROW(NotSupportedError, (string(extensions[0]) + " is not supported").c_str());

	return createDevice(vki, physicalDevice, &deviceParams, pAllocator);
}

deUint32 getNumQueueFamilyIndices (const InstanceInterface& vki, VkPhysicalDevice physicalDevice)
{
	deUint32	numFamilies		= 0;

	vki.getPhysicalDeviceQueueFamilyProperties(physicalDevice, &numFamilies, DE_NULL);

	return numFamilies;
}

vector<deUint32> getSupportedQueueFamilyIndices (const InstanceInterface& vki, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	const deUint32		numTotalFamilyIndices	= getNumQueueFamilyIndices(vki, physicalDevice);
	vector<deUint32>	supportedFamilyIndices;

	for (deUint32 queueFamilyNdx = 0; queueFamilyNdx < numTotalFamilyIndices; ++queueFamilyNdx)
	{
		if (getPhysicalDeviceSurfaceSupport(vki, physicalDevice, queueFamilyNdx, surface) != VK_FALSE)
			supportedFamilyIndices.push_back(queueFamilyNdx);
	}

	return supportedFamilyIndices;
}

deUint32 chooseQueueFamilyIndex (const InstanceInterface& vki, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	const vector<deUint32>	supportedFamilyIndices	= getSupportedQueueFamilyIndices(vki, physicalDevice, surface);

	if (supportedFamilyIndices.empty())
		TCU_THROW(NotSupportedError, "Device doesn't support presentation");

	return supportedFamilyIndices[0];
}

struct InstanceHelper
{
	const vector<VkExtensionProperties>	supportedExtensions;
	const Unique<VkInstance>			instance;
	const InstanceDriver				vki;

	InstanceHelper (Context& context, Type wsiType, const VkAllocationCallbacks* pAllocator = DE_NULL)
		: supportedExtensions	(enumerateInstanceExtensionProperties(context.getPlatformInterface(),
																	  DE_NULL))
		, instance				(createInstanceWithWsi(context.getPlatformInterface(),
													   supportedExtensions,
													   wsiType,
													   pAllocator))
		, vki					(context.getPlatformInterface(), *instance)
	{}
};

struct DeviceHelper
{
	const VkPhysicalDevice	physicalDevice;
	const deUint32			queueFamilyIndex;
	const Unique<VkDevice>	device;
	const DeviceDriver		vkd;
	const VkQueue			queue;

	DeviceHelper (Context&						context,
				  const InstanceInterface&		vki,
				  VkInstance					instance,
				  VkSurfaceKHR					surface,
				  const VkAllocationCallbacks*	pAllocator = DE_NULL)
		: physicalDevice	(chooseDevice(vki, instance, context.getTestContext().getCommandLine()))
		, queueFamilyIndex	(chooseQueueFamilyIndex(vki, physicalDevice, surface))
		, device			(createDeviceWithWsi(vki,
												 physicalDevice,
												 enumerateDeviceExtensionProperties(vki, physicalDevice, DE_NULL),
												 queueFamilyIndex,
												 pAllocator))
		, vkd				(vki, *device)
		, queue				(getDeviceQueue(vkd, *device, queueFamilyIndex, 0))
	{
	}
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

enum TestDimension
{
	TEST_DIMENSION_MIN_IMAGE_COUNT = 0,	//!< Test all supported image counts
	TEST_DIMENSION_IMAGE_FORMAT,		//!< Test all supported formats
	TEST_DIMENSION_IMAGE_EXTENT,		//!< Test various (supported) extents
	TEST_DIMENSION_IMAGE_ARRAY_LAYERS,
	TEST_DIMENSION_IMAGE_USAGE,
	TEST_DIMENSION_IMAGE_SHARING_MODE,
	TEST_DIMENSION_PRE_TRANSFORM,
	TEST_DIMENSION_COMPOSITE_ALPHA,
	TEST_DIMENSION_PRESENT_MODE,
	TEST_DIMENSION_CLIPPED,

	TEST_DIMENSION_LAST
};

struct TestParameters
{
	Type			wsiType;
	TestDimension	dimension;

	TestParameters (Type wsiType_, TestDimension dimension_)
		: wsiType	(wsiType_)
		, dimension	(dimension_)
	{}

	TestParameters (void)
		: wsiType	(TYPE_LAST)
		, dimension	(TEST_DIMENSION_LAST)
	{}
};

struct GroupParameters
{
	typedef FunctionInstance1<TestParameters>::Function	Function;

	Type		wsiType;
	Function	function;

	GroupParameters (Type wsiType_, Function function_)
		: wsiType	(wsiType_)
		, function	(function_)
	{}

	GroupParameters (void)
		: wsiType	(TYPE_LAST)
		, function	((Function)DE_NULL)
	{}
};

VkSwapchainCreateInfoKHR getBasicSwapchainParameters (Type						wsiType,
													  const InstanceInterface&	vki,
													  VkPhysicalDevice			physicalDevice,
													  VkSurfaceKHR				surface,
													  VkSurfaceFormatKHR		surfaceFormat,
													  const tcu::UVec2&			desiredSize,
													  deUint32					desiredImageCount)
{
	const VkSurfaceCapabilitiesKHR		capabilities		= getPhysicalDeviceSurfaceCapabilities(vki,
																								   physicalDevice,
																								   surface);
	const PlatformProperties&			platformProperties	= getPlatformProperties(wsiType);
	const VkSurfaceTransformFlagBitsKHR transform			= (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : capabilities.currentTransform;
	const VkSwapchainCreateInfoKHR		parameters			=
	{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		DE_NULL,
		(VkSwapchainCreateFlagsKHR)0,
		surface,
		de::clamp(desiredImageCount, capabilities.minImageCount, capabilities.maxImageCount > 0 ? capabilities.maxImageCount : capabilities.minImageCount + desiredImageCount),
		surfaceFormat.format,
		surfaceFormat.colorSpace,
		(platformProperties.swapchainExtent == PlatformProperties::SWAPCHAIN_EXTENT_MUST_MATCH_WINDOW_SIZE
			? capabilities.currentExtent : vk::makeExtent2D(desiredSize.x(), desiredSize.y())),
		1u,									// imageArrayLayers
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0u,
		(const deUint32*)DE_NULL,
		transform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
		VK_FALSE,							// clipped
		(VkSwapchainKHR)0					// oldSwapchain
	};

	return parameters;
}

typedef de::SharedPtr<Unique<VkImageView> >		ImageViewSp;
typedef de::SharedPtr<Unique<VkFramebuffer> >	FramebufferSp;

class TriangleRenderer
{
public:
									TriangleRenderer	(const DeviceInterface&		vkd,
														 const VkDevice				device,
														 Allocator&					allocator,
														 const BinaryCollection&	binaryRegistry,
														 const vector<VkImage>		swapchainImages,
														 const VkFormat				framebufferFormat,
														 const UVec2&				renderSize);
									~TriangleRenderer	(void);

	void							recordFrame			(VkCommandBuffer			cmdBuffer,
														 deUint32					imageNdx,
														 deUint32					frameNdx) const;

	static void						getPrograms			(SourceCollections& dst);

private:
	static Move<VkRenderPass>		createRenderPass	(const DeviceInterface&		vkd,
														 const VkDevice				device,
														 const VkFormat				colorAttachmentFormat);
	static Move<VkPipelineLayout>	createPipelineLayout(const DeviceInterface&		vkd,
														 VkDevice					device);
	static Move<VkPipeline>			createPipeline		(const DeviceInterface&		vkd,
														 const VkDevice				device,
														 const VkRenderPass			renderPass,
														 const VkPipelineLayout		pipelineLayout,
														 const BinaryCollection&	binaryCollection,
														 const UVec2&				renderSize);

	static Move<VkImageView>		createAttachmentView(const DeviceInterface&		vkd,
														 const VkDevice				device,
														 const VkImage				image,
														 const VkFormat				format);
	static Move<VkFramebuffer>		createFramebuffer	(const DeviceInterface&		vkd,
														 const VkDevice				device,
														 const VkRenderPass			renderPass,
														 const VkImageView			colorAttachment,
														 const UVec2&				renderSize);

	static Move<VkBuffer>			createBuffer		(const DeviceInterface&		vkd,
														 VkDevice					device,
														 VkDeviceSize				size,
														 VkBufferUsageFlags			usage);

	const DeviceInterface&			m_vkd;

	const vector<VkImage>			m_swapchainImages;
	const tcu::UVec2				m_renderSize;

	const Unique<VkRenderPass>		m_renderPass;
	const Unique<VkPipelineLayout>	m_pipelineLayout;
	const Unique<VkPipeline>		m_pipeline;

	const Unique<VkBuffer>			m_vertexBuffer;
	const UniquePtr<Allocation>		m_vertexBufferMemory;

	vector<ImageViewSp>				m_attachmentViews;
	vector<FramebufferSp>			m_framebuffers;
};

Move<VkRenderPass> TriangleRenderer::createRenderPass (const DeviceInterface&	vkd,
													   const VkDevice			device,
													   const VkFormat			colorAttachmentFormat)
{
	const VkAttachmentDescription	colorAttDesc		=
	{
		(VkAttachmentDescriptionFlags)0,
		colorAttachmentFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};
	const VkAttachmentReference		colorAttRef			=
	{
		0u,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	const VkSubpassDescription		subpassDesc			=
	{
		(VkSubpassDescriptionFlags)0u,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0u,							// inputAttachmentCount
		DE_NULL,					// pInputAttachments
		1u,							// colorAttachmentCount
		&colorAttRef,				// pColorAttachments
		DE_NULL,					// pResolveAttachments
		DE_NULL,					// depthStencilAttachment
		0u,							// preserveAttachmentCount
		DE_NULL,					// pPreserveAttachments
	};
	const VkSubpassDependency		dependencies[]		=
	{
		{
			VK_SUBPASS_EXTERNAL,	// srcSubpass
			0u,						// dstSubpass
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|
			 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
			VK_DEPENDENCY_BY_REGION_BIT
		},
		{
			0u,						// srcSubpass
			VK_SUBPASS_EXTERNAL,	// dstSubpass
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|
			 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
			VK_ACCESS_MEMORY_READ_BIT,
			VK_DEPENDENCY_BY_REGION_BIT
		},
	};
	const VkRenderPassCreateInfo	renderPassParams	=
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		DE_NULL,
		(VkRenderPassCreateFlags)0,
		1u,
		&colorAttDesc,
		1u,
		&subpassDesc,
		DE_LENGTH_OF_ARRAY(dependencies),
		dependencies,
	};

	return vk::createRenderPass(vkd, device, &renderPassParams);
}

Move<VkPipelineLayout> TriangleRenderer::createPipelineLayout (const DeviceInterface&	vkd,
															   const VkDevice			device)
{
	const VkPushConstantRange						pushConstantRange		=
	{
		VK_SHADER_STAGE_VERTEX_BIT,
		0u,											// offset
		(deUint32)sizeof(deUint32),					// size
	};
	const VkPipelineLayoutCreateInfo				pipelineLayoutParams	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		DE_NULL,
		(vk::VkPipelineLayoutCreateFlags)0,
		0u,											// setLayoutCount
		DE_NULL,									// pSetLayouts
		1u,
		&pushConstantRange,
	};

	return vk::createPipelineLayout(vkd, device, &pipelineLayoutParams);
}

Move<VkPipeline> TriangleRenderer::createPipeline (const DeviceInterface&	vkd,
												   const VkDevice			device,
												   const VkRenderPass		renderPass,
												   const VkPipelineLayout	pipelineLayout,
												   const BinaryCollection&	binaryCollection,
												   const UVec2&				renderSize)
{
	// \note VkShaderModules are fully consumed by vkCreateGraphicsPipelines()
	//		 and can be deleted immediately following that call.
	const Unique<VkShaderModule>					vertShaderModule		(createShaderModule(vkd, device, binaryCollection.get("tri-vert"), 0));
	const Unique<VkShaderModule>					fragShaderModule		(createShaderModule(vkd, device, binaryCollection.get("tri-frag"), 0));

	const VkSpecializationInfo						emptyShaderSpecParams	=
	{
		0u,											// mapEntryCount
		DE_NULL,									// pMap
		0,											// dataSize
		DE_NULL,									// pData
	};
	const VkPipelineShaderStageCreateInfo			shaderStageParams[]		=
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			DE_NULL,
			(VkPipelineShaderStageCreateFlags)0,
			VK_SHADER_STAGE_VERTEX_BIT,
			*vertShaderModule,
			"main",
			&emptyShaderSpecParams,
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			DE_NULL,
			(VkPipelineShaderStageCreateFlags)0,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			*fragShaderModule,
			"main",
			&emptyShaderSpecParams,
		}
	};
	const VkPipelineDepthStencilStateCreateInfo		depthStencilParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineDepthStencilStateCreateFlags)0,
		DE_FALSE,									// depthTestEnable
		DE_FALSE,									// depthWriteEnable
		VK_COMPARE_OP_ALWAYS,						// depthCompareOp
		DE_FALSE,									// depthBoundsTestEnable
		DE_FALSE,									// stencilTestEnable
		{
			VK_STENCIL_OP_KEEP,							// failOp
			VK_STENCIL_OP_KEEP,							// passOp
			VK_STENCIL_OP_KEEP,							// depthFailOp
			VK_COMPARE_OP_ALWAYS,						// compareOp
			0u,											// compareMask
			0u,											// writeMask
			0u,											// reference
		},											// front
		{
			VK_STENCIL_OP_KEEP,							// failOp
			VK_STENCIL_OP_KEEP,							// passOp
			VK_STENCIL_OP_KEEP,							// depthFailOp
			VK_COMPARE_OP_ALWAYS,						// compareOp
			0u,											// compareMask
			0u,											// writeMask
			0u,											// reference
		},											// back
		-1.0f,										// minDepthBounds
		+1.0f,										// maxDepthBounds
	};
	const VkViewport								viewport0				=
	{
		0.0f,										// x
		0.0f,										// y
		(float)renderSize.x(),						// width
		(float)renderSize.y(),						// height
		0.0f,										// minDepth
		1.0f,										// maxDepth
	};
	const VkRect2D									scissor0				=
	{
		{ 0u, 0u, },								// offset
		{ renderSize.x(), renderSize.y() },			// extent
	};
	const VkPipelineViewportStateCreateInfo			viewportParams			=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineViewportStateCreateFlags)0,
		1u,
		&viewport0,
		1u,
		&scissor0
	};
	const VkPipelineMultisampleStateCreateInfo		multisampleParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineMultisampleStateCreateFlags)0,
		VK_SAMPLE_COUNT_1_BIT,						// rasterizationSamples
		VK_FALSE,									// sampleShadingEnable
		0.0f,										// minSampleShading
		(const VkSampleMask*)DE_NULL,				// sampleMask
		VK_FALSE,									// alphaToCoverageEnable
		VK_FALSE,									// alphaToOneEnable
	};
	const VkPipelineRasterizationStateCreateInfo	rasterParams			=
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineRasterizationStateCreateFlags)0,
		VK_FALSE,									// depthClampEnable
		VK_FALSE,									// rasterizerDiscardEnable
		VK_POLYGON_MODE_FILL,						// polygonMode
		VK_CULL_MODE_NONE,							// cullMode
		VK_FRONT_FACE_COUNTER_CLOCKWISE,			// frontFace
		VK_FALSE,									// depthBiasEnable
		0.0f,										// depthBiasConstantFactor
		0.0f,										// depthBiasClamp
		0.0f,										// depthBiasSlopeFactor
		1.0f,										// lineWidth
	};
	const VkPipelineInputAssemblyStateCreateInfo	inputAssemblyParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineInputAssemblyStateCreateFlags)0,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		DE_FALSE,									// primitiveRestartEnable
	};
	const VkVertexInputBindingDescription			vertexBinding0			=
	{
		0u,											// binding
		(deUint32)sizeof(tcu::Vec4),				// stride
		VK_VERTEX_INPUT_RATE_VERTEX,				// inputRate
	};
	const VkVertexInputAttributeDescription			vertexAttrib0			=
	{
		0u,											// location
		0u,											// binding
		VK_FORMAT_R32G32B32A32_SFLOAT,				// format
		0u,											// offset
	};
	const VkPipelineVertexInputStateCreateInfo		vertexInputStateParams	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineVertexInputStateCreateFlags)0,
		1u,
		&vertexBinding0,
		1u,
		&vertexAttrib0,
	};
	const VkPipelineColorBlendAttachmentState		attBlendParams0			=
	{
		VK_FALSE,									// blendEnable
		VK_BLEND_FACTOR_ONE,						// srcColorBlendFactor
		VK_BLEND_FACTOR_ZERO,						// dstColorBlendFactor
		VK_BLEND_OP_ADD,							// colorBlendOp
		VK_BLEND_FACTOR_ONE,						// srcAlphaBlendFactor
		VK_BLEND_FACTOR_ZERO,						// dstAlphaBlendFactor
		VK_BLEND_OP_ADD,							// alphaBlendOp
		(VK_COLOR_COMPONENT_R_BIT|
		 VK_COLOR_COMPONENT_G_BIT|
		 VK_COLOR_COMPONENT_B_BIT|
		 VK_COLOR_COMPONENT_A_BIT),					// colorWriteMask
	};
	const VkPipelineColorBlendStateCreateInfo		blendParams				=
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineColorBlendStateCreateFlags)0,
		VK_FALSE,									// logicOpEnable
		VK_LOGIC_OP_COPY,
		1u,
		&attBlendParams0,
		{ 0.0f, 0.0f, 0.0f, 0.0f },					// blendConstants[4]
	};
	const VkGraphicsPipelineCreateInfo				pipelineParams			=
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		DE_NULL,
		(VkPipelineCreateFlags)0,
		(deUint32)DE_LENGTH_OF_ARRAY(shaderStageParams),
		shaderStageParams,
		&vertexInputStateParams,
		&inputAssemblyParams,
		(const VkPipelineTessellationStateCreateInfo*)DE_NULL,
		&viewportParams,
		&rasterParams,
		&multisampleParams,
		&depthStencilParams,
		&blendParams,
		(const VkPipelineDynamicStateCreateInfo*)DE_NULL,
		pipelineLayout,
		renderPass,
		0u,											// subpass
		DE_NULL,									// basePipelineHandle
		0u,											// basePipelineIndex
	};

	return vk::createGraphicsPipeline(vkd, device, (VkPipelineCache)0, &pipelineParams);
}

Move<VkImageView> TriangleRenderer::createAttachmentView (const DeviceInterface&	vkd,
														  const VkDevice			device,
														  const VkImage				image,
														  const VkFormat			format)
{
	const VkImageViewCreateInfo		viewParams	=
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		DE_NULL,
		(VkImageViewCreateFlags)0,
		image,
		VK_IMAGE_VIEW_TYPE_2D,
		format,
		vk::makeComponentMappingRGBA(),
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0u,						// baseMipLevel
			1u,						// levelCount
			0u,						// baseArrayLayer
			1u,						// layerCount
		},
	};

	return vk::createImageView(vkd, device, &viewParams);
}

Move<VkFramebuffer> TriangleRenderer::createFramebuffer	(const DeviceInterface&		vkd,
														 const VkDevice				device,
														 const VkRenderPass			renderPass,
														 const VkImageView			colorAttachment,
														 const UVec2&				renderSize)
{
	const VkFramebufferCreateInfo	framebufferParams	=
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		DE_NULL,
		(VkFramebufferCreateFlags)0,
		renderPass,
		1u,
		&colorAttachment,
		renderSize.x(),
		renderSize.y(),
		1u,							// layers
	};

	return vk::createFramebuffer(vkd, device, &framebufferParams);
}

Move<VkBuffer> TriangleRenderer::createBuffer (const DeviceInterface&	vkd,
											   VkDevice					device,
											   VkDeviceSize				size,
											   VkBufferUsageFlags		usage)
{
	const VkBufferCreateInfo	bufferParams	=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		DE_NULL,
		(VkBufferCreateFlags)0,
		size,
		usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		DE_NULL
	};

	return vk::createBuffer(vkd, device, &bufferParams);
}

TriangleRenderer::TriangleRenderer (const DeviceInterface&	vkd,
									const VkDevice			device,
									Allocator&				allocator,
									const BinaryCollection&	binaryRegistry,
									const vector<VkImage>	swapchainImages,
									const VkFormat			framebufferFormat,
									const UVec2&			renderSize)
	: m_vkd					(vkd)
	, m_swapchainImages		(swapchainImages)
	, m_renderSize			(renderSize)
	, m_renderPass			(createRenderPass(vkd, device, framebufferFormat))
	, m_pipelineLayout		(createPipelineLayout(vkd, device))
	, m_pipeline			(createPipeline(vkd, device, *m_renderPass, *m_pipelineLayout, binaryRegistry, renderSize))
	, m_vertexBuffer		(createBuffer(vkd, device, (VkDeviceSize)(sizeof(float)*4*3), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
	, m_vertexBufferMemory	(allocator.allocate(getBufferMemoryRequirements(vkd, device, *m_vertexBuffer),
							 MemoryRequirement::HostVisible))
{
	m_attachmentViews.resize(swapchainImages.size());
	m_framebuffers.resize(swapchainImages.size());

	for (size_t imageNdx = 0; imageNdx < swapchainImages.size(); ++imageNdx)
	{
		m_attachmentViews[imageNdx]	= ImageViewSp(new Unique<VkImageView>(createAttachmentView(vkd, device, swapchainImages[imageNdx], framebufferFormat)));
		m_framebuffers[imageNdx]	= FramebufferSp(new Unique<VkFramebuffer>(createFramebuffer(vkd, device, *m_renderPass, **m_attachmentViews[imageNdx], renderSize)));
	}

	VK_CHECK(vkd.bindBufferMemory(device, *m_vertexBuffer, m_vertexBufferMemory->getMemory(), m_vertexBufferMemory->getOffset()));

	{
		const VkMappedMemoryRange	memRange	=
		{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			DE_NULL,
			m_vertexBufferMemory->getMemory(),
			m_vertexBufferMemory->getOffset(),
			VK_WHOLE_SIZE
		};
		const tcu::Vec4				vertices[]	=
		{
			tcu::Vec4(-0.5f, -0.5f, 0.0f, 1.0f),
			tcu::Vec4(+0.5f, -0.5f, 0.0f, 1.0f),
			tcu::Vec4( 0.0f, +0.5f, 0.0f, 1.0f)
		};
		DE_STATIC_ASSERT(sizeof(vertices) == sizeof(float)*4*3);

		deMemcpy(m_vertexBufferMemory->getHostPtr(), &vertices[0], sizeof(vertices));
		VK_CHECK(vkd.flushMappedMemoryRanges(device, 1u, &memRange));
	}
}

TriangleRenderer::~TriangleRenderer (void)
{
}

void TriangleRenderer::recordFrame (VkCommandBuffer	cmdBuffer,
									deUint32		imageNdx,
									deUint32		frameNdx) const
{
	const VkFramebuffer	curFramebuffer	= **m_framebuffers[imageNdx];

	{
		const VkCommandBufferBeginInfo	cmdBufBeginParams	=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			DE_NULL,
			(VkCommandBufferUsageFlags)0,
			(const VkCommandBufferInheritanceInfo*)DE_NULL,
		};
		VK_CHECK(m_vkd.beginCommandBuffer(cmdBuffer, &cmdBufBeginParams));
	}

	{
		const VkClearValue			clearValue		= makeClearValueColorF32(0.125f, 0.25f, 0.75f, 1.0f);
		const VkRenderPassBeginInfo	passBeginParams	=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			DE_NULL,
			*m_renderPass,
			curFramebuffer,
			{
				{ 0, 0 },
				{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y() }
			},													// renderArea
			1u,													// clearValueCount
			&clearValue,										// pClearValues
		};
		m_vkd.cmdBeginRenderPass(cmdBuffer, &passBeginParams, VK_SUBPASS_CONTENTS_INLINE);
	}

	m_vkd.cmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

	{
		const VkDeviceSize bindingOffset = 0;
		m_vkd.cmdBindVertexBuffers(cmdBuffer, 0u, 1u, &m_vertexBuffer.get(), &bindingOffset);
	}

	m_vkd.cmdPushConstants(cmdBuffer, *m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u, (deUint32)sizeof(deUint32), &frameNdx);
	m_vkd.cmdDraw(cmdBuffer, 3u, 1u, 0u, 0u);
	m_vkd.cmdEndRenderPass(cmdBuffer);

	VK_CHECK(m_vkd.endCommandBuffer(cmdBuffer));
}

void TriangleRenderer::getPrograms (SourceCollections& dst)
{
	dst.glslSources.add("tri-vert") << glu::VertexSource(
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 a_position;\n"
		"layout(push_constant) uniform FrameData\n"
		"{\n"
		"    highp uint frameNdx;\n"
		"} frameData;\n"
		"void main (void)\n"
		"{\n"
		"    highp float angle = float(frameData.frameNdx) / 100.0;\n"
		"    highp float c     = cos(angle);\n"
		"    highp float s     = sin(angle);\n"
		"    highp mat4  t     = mat4( c, -s,  0,  0,\n"
		"                              s,  c,  0,  0,\n"
		"                              0,  0,  1,  0,\n"
		"                              0,  0,  0,  1);\n"
		"    gl_Position = t * a_position;\n"
		"}\n");
	dst.glslSources.add("tri-frag") << glu::FragmentSource(
		"#version 310 es\n"
		"layout(location = 0) out lowp vec4 o_color;\n"
		"void main (void) { o_color = vec4(1.0, 0.0, 1.0, 1.0); }\n");
}

typedef de::SharedPtr<Unique<VkCommandBuffer> >	CommandBufferSp;
typedef de::SharedPtr<Unique<VkFence> >			FenceSp;
typedef de::SharedPtr<Unique<VkSemaphore> >		SemaphoreSp;

vector<FenceSp> createFences (const DeviceInterface&	vkd,
							  const VkDevice			device,
							  size_t					numFences)
{
	vector<FenceSp> fences(numFences);

	for (size_t ndx = 0; ndx < numFences; ++ndx)
		fences[ndx] = FenceSp(new Unique<VkFence>(createFence(vkd, device)));

	return fences;
}

vector<SemaphoreSp> createSemaphores (const DeviceInterface&	vkd,
									  const VkDevice			device,
									  size_t					numSemaphores)
{
	vector<SemaphoreSp> semaphores(numSemaphores);

	for (size_t ndx = 0; ndx < numSemaphores; ++ndx)
		semaphores[ndx] = SemaphoreSp(new Unique<VkSemaphore>(createSemaphore(vkd, device)));

	return semaphores;
}

vector<CommandBufferSp> allocateCommandBuffers (const DeviceInterface&		vkd,
												const VkDevice				device,
												const VkCommandPool			commandPool,
												const VkCommandBufferLevel	level,
												const size_t				numCommandBuffers)
{
	vector<CommandBufferSp>				buffers		(numCommandBuffers);

	for (size_t ndx = 0; ndx < numCommandBuffers; ++ndx)
		buffers[ndx] = CommandBufferSp(new Unique<VkCommandBuffer>(allocateCommandBuffer(vkd, device, commandPool, level)));

	return buffers;
}

tcu::TestStatus basicExtensionTest (Context& context, Type wsiType)
{
	const tcu::UVec2				desiredSize		(256, 256);
	const InstanceHelper			instHelper		(context, wsiType);
	const NativeObjects				native			(context, instHelper.supportedExtensions, wsiType, tcu::just(desiredSize));
	const Unique<VkSurfaceKHR>		surface			(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));
	const DeviceHelper				devHelper		(context, instHelper.vki, *instHelper.instance, *surface);

	if (!de::contains(context.getInstanceExtensions().begin(), context.getInstanceExtensions().end(), "VK_EXT_swapchain_colorspace"))
		TCU_THROW(NotSupportedError, "Extension VK_EXT_swapchain_colorspace not supported");

	const vector<VkSurfaceFormatKHR>	formats			= getPhysicalDeviceSurfaceFormats(instHelper.vki,
																						  devHelper.physicalDevice,
																						  *surface);

	bool found = false;
	for (vector<VkSurfaceFormatKHR>::const_iterator curFmt = formats.begin(); curFmt != formats.end(); ++curFmt)
	{
		if (curFmt->colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		TCU_THROW(NotSupportedError, "VK_EXT_swapchain_colorspace supported, but no non-SRGB_NONLINEAR_KHR surface formats found.");
	}
	return tcu::TestStatus::pass("Extension tests succeeded");
}

tcu::TestStatus surfaceFormatRenderTest (Context& context, Type wsiType, VkSurfaceKHR surface, VkSurfaceFormatKHR curFmt)
{
	const tcu::UVec2					desiredSize		(256, 256);
	const InstanceHelper				instHelper		(context, wsiType);
	const DeviceHelper					devHelper		(context, instHelper.vki, *instHelper.instance, surface);
	const DeviceInterface&				vkd				= devHelper.vkd;
	const VkDevice						device			= *devHelper.device;
	SimpleAllocator						allocator		(vkd, device, getPhysicalDeviceMemoryProperties(instHelper.vki, devHelper.physicalDevice));

	tcu::print("Test Surface format: %s, %s\n", de::toString(curFmt.format).c_str(), de::toString(curFmt.colorSpace).c_str());
	const VkSwapchainCreateInfoKHR	swapchainInfo				= getBasicSwapchainParameters(wsiType, instHelper.vki, devHelper.physicalDevice, surface, curFmt, desiredSize, 2);
	const Unique<VkSwapchainKHR>	swapchain					(createSwapchainKHR(vkd, device, &swapchainInfo));
	const vector<VkImage>			swapchainImages				= getSwapchainImages(vkd, device, *swapchain);

	const TriangleRenderer			renderer					(vkd,
																 device,
																 allocator,
																 context.getBinaryCollection(),
																 swapchainImages,
																 swapchainInfo.imageFormat,
																 tcu::UVec2(swapchainInfo.imageExtent.width, swapchainInfo.imageExtent.height));

	const Unique<VkCommandPool>		commandPool					(createCommandPool(vkd, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, devHelper.queueFamilyIndex));

	const size_t					maxQueuedFrames				= swapchainImages.size()*2;

	// We need to keep hold of fences from vkAcquireNextImageKHR to actually
	// limit number of frames we allow to be queued.
	const vector<FenceSp>			imageReadyFences			(createFences(vkd, device, maxQueuedFrames));

	// We need maxQueuedFrames+1 for imageReadySemaphores pool as we need to pass
	// the semaphore in same time as the fence we use to meter rendering.
	const vector<SemaphoreSp>		imageReadySemaphores		(createSemaphores(vkd, device, maxQueuedFrames+1));

	// For rest we simply need maxQueuedFrames as we will wait for image
	// from frameNdx-maxQueuedFrames to become available to us, guaranteeing that
	// previous uses must have completed.
	const vector<SemaphoreSp>		renderingCompleteSemaphores	(createSemaphores(vkd, device, maxQueuedFrames));
	const vector<CommandBufferSp>	commandBuffers				(allocateCommandBuffers(vkd, device, *commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, maxQueuedFrames));

	try
	{
		const deUint32	numFramesToRender	= 60;

		for (deUint32 frameNdx = 0; frameNdx < numFramesToRender; ++frameNdx)
		{
			const VkFence		imageReadyFence		= **imageReadyFences[frameNdx%imageReadyFences.size()];
			const VkSemaphore	imageReadySemaphore	= **imageReadySemaphores[frameNdx%imageReadySemaphores.size()];
			deUint32			imageNdx			= ~0u;

			if (frameNdx >= maxQueuedFrames)
				VK_CHECK(vkd.waitForFences(device, 1u, &imageReadyFence, VK_TRUE, std::numeric_limits<deUint64>::max()));

			VK_CHECK(vkd.resetFences(device, 1, &imageReadyFence));

			{
				const VkResult	acquireResult	= vkd.acquireNextImageKHR(device,
																		  *swapchain,
																		  std::numeric_limits<deUint64>::max(),
																		  imageReadySemaphore,
																		  (vk::VkFence)0,
																		  &imageNdx);

				if (acquireResult == VK_SUBOPTIMAL_KHR)
					context.getTestContext().getLog() << TestLog::Message << "Got " << acquireResult << " at frame " << frameNdx << TestLog::EndMessage;
				else
					VK_CHECK(acquireResult);
			}

			TCU_CHECK((size_t)imageNdx < swapchainImages.size());

			{
				const VkSemaphore			renderingCompleteSemaphore	= **renderingCompleteSemaphores[frameNdx%renderingCompleteSemaphores.size()];
				const VkCommandBuffer		commandBuffer				= **commandBuffers[frameNdx%commandBuffers.size()];
				const VkPipelineStageFlags	waitDstStage				= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				const VkSubmitInfo			submitInfo					=
				{
					VK_STRUCTURE_TYPE_SUBMIT_INFO,
					DE_NULL,
					1u,
					&imageReadySemaphore,
					&waitDstStage,
					1u,
					&commandBuffer,
					1u,
					&renderingCompleteSemaphore
				};
				const VkPresentInfoKHR		presentInfo					=
				{
					VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
					DE_NULL,
					1u,
					&renderingCompleteSemaphore,
					1u,
					&*swapchain,
					&imageNdx,
					(VkResult*)DE_NULL
				};

				renderer.recordFrame(commandBuffer, imageNdx, frameNdx);
				VK_CHECK(vkd.queueSubmit(devHelper.queue, 1u, &submitInfo, imageReadyFence));
				VK_CHECK(vkd.queuePresentKHR(devHelper.queue, &presentInfo));
			}
		}

		VK_CHECK(vkd.deviceWaitIdle(device));
	}
	catch (...)
	{
		// Make sure device is idle before destroying resources
		vkd.deviceWaitIdle(device);
		throw;
	}
	tcu::print("Done\n");

	return tcu::TestStatus::pass("Rendering test succeeded");
}

tcu::TestStatus surfaceFormatRenderTests (Context& context, Type wsiType)
{
	const tcu::UVec2					desiredSize		(256, 256);
	const InstanceHelper				instHelper		(context, wsiType);
	const NativeObjects					native			(context, instHelper.supportedExtensions, wsiType, tcu::just(desiredSize));
	const Unique<VkSurfaceKHR>			surface			(createSurface(instHelper.vki, *instHelper.instance, wsiType, *native.display, *native.window));
	const DeviceHelper					devHelper		(context, instHelper.vki, *instHelper.instance, *surface);

	if (!de::contains(context.getInstanceExtensions().begin(), context.getInstanceExtensions().end(), "VK_EXT_swapchain_colorspace"))
		TCU_THROW(NotSupportedError, "Extension VK_EXT_swapchain_colorspace not supported");

	const vector<VkSurfaceFormatKHR>	formats			= getPhysicalDeviceSurfaceFormats(instHelper.vki,
																						  devHelper.physicalDevice,
																						  *surface);
	for (vector<VkSurfaceFormatKHR>::const_iterator curFmt = formats.begin(); curFmt != formats.end(); ++curFmt)
	{
		surfaceFormatRenderTest(context, wsiType, *surface, *curFmt);
	}
	return tcu::TestStatus::pass("Rendering tests succeeded");
}

void getBasicRenderPrograms (SourceCollections& dst, Type)
{
	TriangleRenderer::getPrograms(dst);
}

} // anonymous

void createColorSpaceTests (tcu::TestCaseGroup* testGroup, vk::wsi::Type wsiType)
{
	addFunctionCase(testGroup, "extensions", "Verify Colorspace Extensions", basicExtensionTest, wsiType);
	addFunctionCaseWithPrograms(testGroup, "basic", "Basic Rendering Tests", getBasicRenderPrograms, surfaceFormatRenderTests, wsiType);
}

} // wsi
} // vkt
