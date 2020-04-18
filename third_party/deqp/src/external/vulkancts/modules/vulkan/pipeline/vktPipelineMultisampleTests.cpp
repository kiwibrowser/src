/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
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
 * \brief Multisample Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineMultisampleTests.hpp"
#include "vktPipelineMultisampleImageTests.hpp"
#include "vktPipelineClearUtil.hpp"
#include "vktPipelineImageUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktPipelineReferenceRenderer.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"
#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"
#include "deStringUtil.hpp"
#include "deMemory.h"

#include <sstream>
#include <vector>
#include <map>

namespace vkt
{
namespace pipeline
{

using namespace vk;

namespace
{
enum GeometryType
{
	GEOMETRY_TYPE_OPAQUE_TRIANGLE,
	GEOMETRY_TYPE_OPAQUE_LINE,
	GEOMETRY_TYPE_OPAQUE_POINT,
	GEOMETRY_TYPE_OPAQUE_QUAD,
	GEOMETRY_TYPE_OPAQUE_QUAD_NONZERO_DEPTH,	//!< placed at z = 0.5
	GEOMETRY_TYPE_TRANSLUCENT_QUAD,
	GEOMETRY_TYPE_INVISIBLE_TRIANGLE,
	GEOMETRY_TYPE_INVISIBLE_QUAD,
	GEOMETRY_TYPE_GRADIENT_QUAD
};

enum TestModeBits
{
	TEST_MODE_DEPTH_BIT		= 1u,
	TEST_MODE_STENCIL_BIT	= 2u,
};
typedef deUint32 TestModeFlags;

enum RenderType
{
	// resolve multisample rendering to single sampled image
	RENDER_TYPE_RESOLVE		= 0u,

	// copy samples to an array of single sampled images
	RENDER_TYPE_COPY_SAMPLES
};

void									initMultisamplePrograms				(SourceCollections& sources, GeometryType geometryType);
bool									isSupportedSampleCount				(const InstanceInterface& instanceInterface, VkPhysicalDevice physicalDevice, VkSampleCountFlagBits rasterizationSamples);
bool									isSupportedDepthStencilFormat		(const InstanceInterface& vki, const VkPhysicalDevice physDevice, const VkFormat format);
VkPipelineColorBlendAttachmentState		getDefaultColorBlendAttachmentState	(void);
deUint32								getUniqueColorsCount				(const tcu::ConstPixelBufferAccess& image);
VkImageAspectFlags						getImageAspectFlags					(const VkFormat format);
VkPrimitiveTopology						getPrimitiveTopology				(const GeometryType geometryType);
std::vector<Vertex4RGBA>				generateVertices					(const GeometryType geometryType);
VkFormat								findSupportedDepthStencilFormat		(Context& context, const bool useDepth, const bool useStencil);

class MultisampleTest : public vkt::TestCase
{
public:

												MultisampleTest						(tcu::TestContext&								testContext,
																					 const std::string&								name,
																					 const std::string&								description,
																					 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																					 const VkPipelineColorBlendAttachmentState&		blendState,
																					 GeometryType									geometryType);
	virtual										~MultisampleTest					(void) {}

	virtual void								initPrograms						(SourceCollections& programCollection) const;
	virtual TestInstance*						createInstance						(Context& context) const;

protected:
	virtual TestInstance*						createMultisampleTestInstance		(Context&										context,
																					 VkPrimitiveTopology							topology,
																					 const std::vector<Vertex4RGBA>&				vertices,
																					 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																					 const VkPipelineColorBlendAttachmentState&		colorBlendState) const = 0;
	VkPipelineMultisampleStateCreateInfo		m_multisampleStateParams;
	const VkPipelineColorBlendAttachmentState	m_colorBlendState;
	const GeometryType							m_geometryType;
	std::vector<VkSampleMask>					m_sampleMask;
};

class RasterizationSamplesTest : public MultisampleTest
{
public:
												RasterizationSamplesTest			(tcu::TestContext&		testContext,
																					 const std::string&		name,
																					 const std::string&		description,
																					 VkSampleCountFlagBits	rasterizationSamples,
																					 GeometryType			geometryType,
																					 TestModeFlags			modeFlags				= 0u);
	virtual										~RasterizationSamplesTest			(void) {}

protected:
	virtual TestInstance*						createMultisampleTestInstance		(Context&										context,
																					 VkPrimitiveTopology							topology,
																					 const std::vector<Vertex4RGBA>&				vertices,
																					 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																					 const VkPipelineColorBlendAttachmentState&		colorBlendState) const;

	static VkPipelineMultisampleStateCreateInfo	getRasterizationSamplesStateParams	(VkSampleCountFlagBits rasterizationSamples);

	const TestModeFlags							m_modeFlags;
};

class MinSampleShadingTest : public MultisampleTest
{
public:
												MinSampleShadingTest				(tcu::TestContext&		testContext,
																					 const std::string&		name,
																					 const std::string&		description,
																					 VkSampleCountFlagBits	rasterizationSamples,
																					 float					minSampleShading,
																					 GeometryType			geometryType);
	virtual										~MinSampleShadingTest				(void) {}

protected:
	virtual void								initPrograms						(SourceCollections& programCollection) const;
	virtual TestInstance*						createMultisampleTestInstance		(Context&										context,
																					 VkPrimitiveTopology							topology,
																					 const std::vector<Vertex4RGBA>&				vertices,
																					 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																					 const VkPipelineColorBlendAttachmentState&		colorBlendState) const;

	static VkPipelineMultisampleStateCreateInfo	getMinSampleShadingStateParams		(VkSampleCountFlagBits rasterizationSamples, float minSampleShading);
};

class SampleMaskTest : public MultisampleTest
{
public:
												SampleMaskTest						(tcu::TestContext&					testContext,
																					 const std::string&					name,
																					 const std::string&					description,
																					 VkSampleCountFlagBits				rasterizationSamples,
																					 const std::vector<VkSampleMask>&	sampleMask,
																					 GeometryType						geometryType);

	virtual										~SampleMaskTest						(void) {}

protected:
	virtual TestInstance*						createMultisampleTestInstance		(Context&										context,
																					 VkPrimitiveTopology							topology,
																					 const std::vector<Vertex4RGBA>&				vertices,
																					 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																					 const VkPipelineColorBlendAttachmentState&		colorBlendState) const;

	static VkPipelineMultisampleStateCreateInfo	getSampleMaskStateParams			(VkSampleCountFlagBits rasterizationSamples, const std::vector<VkSampleMask>& sampleMask);
};

class AlphaToOneTest : public MultisampleTest
{
public:
												AlphaToOneTest					(tcu::TestContext&					testContext,
																				 const std::string&					name,
																				 const std::string&					description,
																				 VkSampleCountFlagBits				rasterizationSamples);

	virtual										~AlphaToOneTest					(void) {}

protected:
	virtual TestInstance*						createMultisampleTestInstance	(Context&										context,
																				 VkPrimitiveTopology							topology,
																				 const std::vector<Vertex4RGBA>&				vertices,
																				 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																				 const VkPipelineColorBlendAttachmentState&		colorBlendState) const;

	static VkPipelineMultisampleStateCreateInfo	getAlphaToOneStateParams		(VkSampleCountFlagBits rasterizationSamples);
	static VkPipelineColorBlendAttachmentState	getAlphaToOneBlendState			(void);
};

class AlphaToCoverageTest : public MultisampleTest
{
public:
												AlphaToCoverageTest				(tcu::TestContext&		testContext,
																				 const std::string&		name,
																				 const std::string&		description,
																				 VkSampleCountFlagBits	rasterizationSamples,
																				 GeometryType			geometryType);

	virtual										~AlphaToCoverageTest			(void) {}

protected:
	virtual TestInstance*						createMultisampleTestInstance	(Context&										context,
																				 VkPrimitiveTopology							topology,
																				 const std::vector<Vertex4RGBA>&				vertices,
																				 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																				 const VkPipelineColorBlendAttachmentState&		colorBlendState) const;

	static VkPipelineMultisampleStateCreateInfo	getAlphaToCoverageStateParams	(VkSampleCountFlagBits rasterizationSamples);

	GeometryType								m_geometryType;
};

typedef de::SharedPtr<Unique<VkPipeline> > VkPipelineSp;

class MultisampleRenderer
{
public:
												MultisampleRenderer			(Context&										context,
																			 const VkFormat									colorFormat,
																			 const tcu::IVec2&								renderSize,
																			 const VkPrimitiveTopology						topology,
																			 const std::vector<Vertex4RGBA>&				vertices,
																			 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																			 const VkPipelineColorBlendAttachmentState&		blendState,
																			 const RenderType								renderType);

												MultisampleRenderer			(Context&										context,
																			 const VkFormat									colorFormat,
																			 const VkFormat									depthStencilFormat,
																			 const tcu::IVec2&								renderSize,
																			 const bool										useDepth,
																			 const bool										useStencil,
																			 const deUint32									numTopologies,
																			 const VkPrimitiveTopology*						pTopology,
																			 const std::vector<Vertex4RGBA>*				pVertices,
																			 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																			 const VkPipelineColorBlendAttachmentState&		blendState,
																		     const RenderType								renderType);

	virtual										~MultisampleRenderer		(void);

	de::MovePtr<tcu::TextureLevel>				render						(void);
	de::MovePtr<tcu::TextureLevel>				getSingleSampledImage		(deUint32 sampleId);

protected:
	void										initialize					(Context&										context,
																			 const deUint32									numTopologies,
																			 const VkPrimitiveTopology*						pTopology,
																			 const std::vector<Vertex4RGBA>*				pVertices);

	Context&									m_context;

	const VkFormat								m_colorFormat;
	const VkFormat								m_depthStencilFormat;
	tcu::IVec2									m_renderSize;
	const bool									m_useDepth;
	const bool									m_useStencil;

	const VkPipelineMultisampleStateCreateInfo	m_multisampleStateParams;
	const VkPipelineColorBlendAttachmentState	m_colorBlendState;

	const RenderType							m_renderType;

	Move<VkImage>								m_colorImage;
	de::MovePtr<Allocation>						m_colorImageAlloc;
	Move<VkImageView>							m_colorAttachmentView;

	Move<VkImage>								m_resolveImage;
	de::MovePtr<Allocation>						m_resolveImageAlloc;
	Move<VkImageView>							m_resolveAttachmentView;

	struct PerSampleImage
	{
		Move<VkImage>								m_image;
		de::MovePtr<Allocation>						m_imageAlloc;
		Move<VkImageView>							m_attachmentView;
	};
	std::vector<de::SharedPtr<PerSampleImage> >	m_perSampleImages;

	Move<VkImage>								m_depthStencilImage;
	de::MovePtr<Allocation>						m_depthStencilImageAlloc;
	Move<VkImageView>							m_depthStencilAttachmentView;

	Move<VkRenderPass>							m_renderPass;
	Move<VkFramebuffer>							m_framebuffer;

	Move<VkShaderModule>						m_vertexShaderModule;
	Move<VkShaderModule>						m_fragmentShaderModule;

	Move<VkShaderModule>						m_copySampleVertexShaderModule;
	Move<VkShaderModule>						m_copySampleFragmentShaderModule;

	Move<VkBuffer>								m_vertexBuffer;
	de::MovePtr<Allocation>						m_vertexBufferAlloc;

	Move<VkPipelineLayout>						m_pipelineLayout;
	std::vector<VkPipelineSp>					m_graphicsPipelines;

	Move<VkDescriptorSetLayout>					m_copySampleDesciptorLayout;
	Move<VkDescriptorPool>						m_copySampleDesciptorPool;
	Move<VkDescriptorSet>						m_copySampleDesciptorSet;

	Move<VkPipelineLayout>						m_copySamplePipelineLayout;
	std::vector<VkPipelineSp>					m_copySamplePipelines;

	Move<VkCommandPool>							m_cmdPool;
	Move<VkCommandBuffer>						m_cmdBuffer;

	Move<VkFence>								m_fence;
};

class RasterizationSamplesInstance : public vkt::TestInstance
{
public:
										RasterizationSamplesInstance	(Context&										context,
																		 VkPrimitiveTopology							topology,
																		 const std::vector<Vertex4RGBA>&				vertices,
																		 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																		 const VkPipelineColorBlendAttachmentState&		blendState,
																		 const TestModeFlags							modeFlags);
	virtual								~RasterizationSamplesInstance	(void) {}

	virtual tcu::TestStatus				iterate							(void);

protected:
	virtual tcu::TestStatus				verifyImage						(const tcu::ConstPixelBufferAccess& result);

	const VkFormat						m_colorFormat;
	const tcu::IVec2					m_renderSize;
	const VkPrimitiveTopology			m_primitiveTopology;
	const std::vector<Vertex4RGBA>		m_vertices;
	const std::vector<Vertex4RGBA>		m_fullQuadVertices;			//!< used by depth/stencil case
	const TestModeFlags					m_modeFlags;
	de::MovePtr<MultisampleRenderer>	m_multisampleRenderer;
};

class MinSampleShadingInstance : public vkt::TestInstance
{
public:
												MinSampleShadingInstance	(Context&										context,
																			 VkPrimitiveTopology							topology,
																			 const std::vector<Vertex4RGBA>&				vertices,
																			 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																			 const VkPipelineColorBlendAttachmentState&		blendState);
	virtual										~MinSampleShadingInstance	(void) {}

	virtual tcu::TestStatus						iterate						(void);

protected:
	virtual tcu::TestStatus						verifySampleShadedImage		(const std::vector<tcu::TextureLevel>& testShadingImages,
																			 const tcu::ConstPixelBufferAccess& noSampleshadingImage);

	const VkFormat								m_colorFormat;
	const tcu::IVec2							m_renderSize;
	const VkPrimitiveTopology					m_primitiveTopology;
	const std::vector<Vertex4RGBA>				m_vertices;
	const VkPipelineMultisampleStateCreateInfo	m_multisampleStateParams;
	const VkPipelineColorBlendAttachmentState	m_colorBlendState;
};

class SampleMaskInstance : public vkt::TestInstance
{
public:
												SampleMaskInstance			(Context&										context,
																			 VkPrimitiveTopology							topology,
																			 const std::vector<Vertex4RGBA>&				vertices,
																			 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																			 const VkPipelineColorBlendAttachmentState&		blendState);
	virtual										~SampleMaskInstance			(void) {}

	virtual tcu::TestStatus						iterate						(void);

protected:
	virtual tcu::TestStatus						verifyImage					(const tcu::ConstPixelBufferAccess& testShadingImage,
																			 const tcu::ConstPixelBufferAccess& minShadingImage,
																			 const tcu::ConstPixelBufferAccess& maxShadingImage);
	const VkFormat								m_colorFormat;
	const tcu::IVec2							m_renderSize;
	const VkPrimitiveTopology					m_primitiveTopology;
	const std::vector<Vertex4RGBA>				m_vertices;
	const VkPipelineMultisampleStateCreateInfo	m_multisampleStateParams;
	const VkPipelineColorBlendAttachmentState	m_colorBlendState;
};

class AlphaToOneInstance : public vkt::TestInstance
{
public:
												AlphaToOneInstance			(Context&										context,
																			 VkPrimitiveTopology							topology,
																			 const std::vector<Vertex4RGBA>&				vertices,
																			 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																			 const VkPipelineColorBlendAttachmentState&		blendState);
	virtual										~AlphaToOneInstance			(void) {}

	virtual tcu::TestStatus						iterate						(void);

protected:
	virtual tcu::TestStatus						verifyImage					(const tcu::ConstPixelBufferAccess& alphaOneImage,
																			 const tcu::ConstPixelBufferAccess& noAlphaOneImage);
	const VkFormat								m_colorFormat;
	const tcu::IVec2							m_renderSize;
	const VkPrimitiveTopology					m_primitiveTopology;
	const std::vector<Vertex4RGBA>				m_vertices;
	const VkPipelineMultisampleStateCreateInfo	m_multisampleStateParams;
	const VkPipelineColorBlendAttachmentState	m_colorBlendState;
};

class AlphaToCoverageInstance : public vkt::TestInstance
{
public:
												AlphaToCoverageInstance		(Context&										context,
																			 VkPrimitiveTopology							topology,
																			 const std::vector<Vertex4RGBA>&				vertices,
																			 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																			 const VkPipelineColorBlendAttachmentState&		blendState,
																			 GeometryType									geometryType);
	virtual										~AlphaToCoverageInstance	(void) {}

	virtual tcu::TestStatus						iterate						(void);

protected:
	virtual tcu::TestStatus						verifyImage					(const tcu::ConstPixelBufferAccess& result);
	const VkFormat								m_colorFormat;
	const tcu::IVec2							m_renderSize;
	const VkPrimitiveTopology					m_primitiveTopology;
	const std::vector<Vertex4RGBA>				m_vertices;
	const VkPipelineMultisampleStateCreateInfo	m_multisampleStateParams;
	const VkPipelineColorBlendAttachmentState	m_colorBlendState;
	const GeometryType							m_geometryType;
};


// Helper functions

void initMultisamplePrograms (SourceCollections& sources, GeometryType geometryType)
{
	std::ostringstream vertexSource;

	vertexSource <<
		"#version 310 es\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec4 color;\n"
		"layout(location = 0) out highp vec4 vtxColor;\n"
		"void main (void)\n"
		"{\n"
		"	gl_Position = position;\n"
		"	vtxColor = color;\n"
		<< (geometryType == GEOMETRY_TYPE_OPAQUE_POINT ? "	gl_PointSize = 3.0f;\n"
			: "")
		<< "}\n";

	static const char* fragmentSource =
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 vtxColor;\n"
		"layout(location = 0) out highp vec4 fragColor;\n"
		"void main (void)\n"
		"{\n"
		"	fragColor = vtxColor;\n"
		"}\n";

	sources.glslSources.add("color_vert") << glu::VertexSource(vertexSource.str());
	sources.glslSources.add("color_frag") << glu::FragmentSource(fragmentSource);
}

void initSampleShadingPrograms (SourceCollections& sources, GeometryType geometryType)
{
	{
		std::ostringstream vertexSource;

		vertexSource <<
			"#version 440\n"
			"layout(location = 0) in vec4 position;\n"
			"layout(location = 1) in vec4 color;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = position;\n"
			<< (geometryType == GEOMETRY_TYPE_OPAQUE_POINT ? "	gl_PointSize = 3.0f;\n"
				: "")
			<< "}\n";

		static const char* fragmentSource =
			"#version 440\n"
			"layout(location = 0) out highp vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	fragColor = vec4(fract(gl_FragCoord.xy), 0.0, 1.0);\n"
			"}\n";

		sources.glslSources.add("color_vert") << glu::VertexSource(vertexSource.str());
		sources.glslSources.add("color_frag") << glu::FragmentSource(fragmentSource);
	}

	{
		static const char*  vertexSource =
			"#version 440\n"
			"void main (void)\n"
			"{\n"
			"	const vec4 positions[4] = vec4[4](\n"
			"		vec4(-1.0, -1.0, 0.0, 1.0),\n"
			"		vec4(-1.0,  1.0, 0.0, 1.0),\n"
			"		vec4( 1.0, -1.0, 0.0, 1.0),\n"
			"		vec4( 1.0,  1.0, 0.0, 1.0)\n"
			"	);\n"
			"	gl_Position = positions[gl_VertexIndex];\n"
			"}\n";

		static const char* fragmentSource =
			"#version 440\n"
			"precision highp float;\n"
			"layout(location = 0) out highp vec4 fragColor;\n"
			"layout(set = 0, binding = 0, input_attachment_index = 0) uniform subpassInputMS imageMS;\n"
			"layout(push_constant) uniform PushConstantsBlock\n"
			"{\n"
			"	int sampleId;\n"
			"} pushConstants;\n"
			"void main (void)\n"
			"{\n"
			"	fragColor = subpassLoad(imageMS, pushConstants.sampleId);\n"
			"}\n";

		sources.glslSources.add("quad_vert") << glu::VertexSource(vertexSource);
		sources.glslSources.add("copy_sample_frag") << glu::FragmentSource(fragmentSource);
	}
}

bool isSupportedSampleCount (const InstanceInterface& instanceInterface, VkPhysicalDevice physicalDevice, VkSampleCountFlagBits rasterizationSamples)
{
	VkPhysicalDeviceProperties deviceProperties;

	instanceInterface.getPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	return !!(deviceProperties.limits.framebufferColorSampleCounts & rasterizationSamples);
}

VkPipelineColorBlendAttachmentState getDefaultColorBlendAttachmentState (void)
{
	const VkPipelineColorBlendAttachmentState colorBlendState =
	{
		false,														// VkBool32					blendEnable;
		VK_BLEND_FACTOR_ONE,										// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,										// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,											// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_ONE,										// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,										// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,											// VkBlendOp				alphaBlendOp;
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |		// VkColorComponentFlags	colorWriteMask;
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	return colorBlendState;
}

deUint32 getUniqueColorsCount (const tcu::ConstPixelBufferAccess& image)
{
	DE_ASSERT(image.getFormat().getPixelSize() == 4);

	std::map<deUint32, deUint32>	histogram; // map<pixel value, number of occurrences>
	const deUint32					pixelCount	= image.getWidth() * image.getHeight() * image.getDepth();

	for (deUint32 pixelNdx = 0; pixelNdx < pixelCount; pixelNdx++)
	{
		const deUint32 pixelValue = *((const deUint32*)image.getDataPtr() + pixelNdx);

		if (histogram.find(pixelValue) != histogram.end())
			histogram[pixelValue]++;
		else
			histogram[pixelValue] = 1;
	}

	return (deUint32)histogram.size();
}

VkImageAspectFlags getImageAspectFlags (const VkFormat format)
{
	const tcu::TextureFormat tcuFormat = mapVkFormat(format);

	if      (tcuFormat.order == tcu::TextureFormat::DS)		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	else if (tcuFormat.order == tcu::TextureFormat::D)		return VK_IMAGE_ASPECT_DEPTH_BIT;
	else if (tcuFormat.order == tcu::TextureFormat::S)		return VK_IMAGE_ASPECT_STENCIL_BIT;

	DE_ASSERT(false);
	return 0u;
}

std::vector<Vertex4RGBA> generateVertices (const GeometryType geometryType)
{
	std::vector<Vertex4RGBA> vertices;

	switch (geometryType)
	{
		case GEOMETRY_TYPE_OPAQUE_TRIANGLE:
		case GEOMETRY_TYPE_INVISIBLE_TRIANGLE:
		{
			Vertex4RGBA vertexData[3] =
			{
				{
					tcu::Vec4(-0.75f, 0.0f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				},
				{
					tcu::Vec4(0.75f, 0.125f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				},
				{
					tcu::Vec4(0.75f, -0.125f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				}
			};

			if (geometryType == GEOMETRY_TYPE_INVISIBLE_TRIANGLE)
			{
				for (int i = 0; i < 3; i++)
					vertexData[i].color = tcu::Vec4();
			}

			vertices = std::vector<Vertex4RGBA>(vertexData, vertexData + 3);
			break;
		}

		case GEOMETRY_TYPE_OPAQUE_LINE:
		{
			const Vertex4RGBA vertexData[2] =
			{
				{
					tcu::Vec4(-0.75f, 0.25f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				},
				{
					tcu::Vec4(0.75f, -0.25f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				}
			};

			vertices = std::vector<Vertex4RGBA>(vertexData, vertexData + 2);
			break;
		}

		case GEOMETRY_TYPE_OPAQUE_POINT:
		{
			const Vertex4RGBA vertex =
			{
				tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f),
				tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
			};

			vertices = std::vector<Vertex4RGBA>(1, vertex);
			break;
		}

		case GEOMETRY_TYPE_OPAQUE_QUAD:
		case GEOMETRY_TYPE_OPAQUE_QUAD_NONZERO_DEPTH:
		case GEOMETRY_TYPE_TRANSLUCENT_QUAD:
		case GEOMETRY_TYPE_INVISIBLE_QUAD:
		case GEOMETRY_TYPE_GRADIENT_QUAD:
		{
			Vertex4RGBA vertexData[4] =
			{
				{
					tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				},
				{
					tcu::Vec4(1.0f, -1.0f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				},
				{
					tcu::Vec4(-1.0f, 1.0f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				},
				{
					tcu::Vec4(1.0f, 1.0f, 0.0f, 1.0f),
					tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
				}
			};

			if (geometryType == GEOMETRY_TYPE_TRANSLUCENT_QUAD)
			{
				for (int i = 0; i < 4; i++)
					vertexData[i].color.w() = 0.25f;
			}
			else if (geometryType == GEOMETRY_TYPE_INVISIBLE_QUAD)
			{
				for (int i = 0; i < 4; i++)
					vertexData[i].color.w() = 0.0f;
			}
			else if (geometryType == GEOMETRY_TYPE_GRADIENT_QUAD)
			{
				vertexData[0].color.w() = 0.0f;
				vertexData[2].color.w() = 0.0f;
			}
			else if (geometryType == GEOMETRY_TYPE_OPAQUE_QUAD_NONZERO_DEPTH)
			{
				for (int i = 0; i < 4; i++)
					vertexData[i].position.z() = 0.5f;
			}

			vertices = std::vector<Vertex4RGBA>(vertexData, vertexData + 4);
			break;
		}

		default:
			DE_ASSERT(false);
	}
	return vertices;
}

VkPrimitiveTopology getPrimitiveTopology (const GeometryType geometryType)
{
	switch (geometryType)
	{
		case GEOMETRY_TYPE_OPAQUE_TRIANGLE:
		case GEOMETRY_TYPE_INVISIBLE_TRIANGLE:			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		case GEOMETRY_TYPE_OPAQUE_LINE:					return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case GEOMETRY_TYPE_OPAQUE_POINT:				return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

		case GEOMETRY_TYPE_OPAQUE_QUAD:
		case GEOMETRY_TYPE_OPAQUE_QUAD_NONZERO_DEPTH:
		case GEOMETRY_TYPE_TRANSLUCENT_QUAD:
		case GEOMETRY_TYPE_INVISIBLE_QUAD:
		case GEOMETRY_TYPE_GRADIENT_QUAD:				return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

		default:
			DE_ASSERT(false);
			return VK_PRIMITIVE_TOPOLOGY_LAST;
	}
}

bool isSupportedDepthStencilFormat (const InstanceInterface& vki, const VkPhysicalDevice physDevice, const VkFormat format)
{
	VkFormatProperties formatProps;
	vki.getPhysicalDeviceFormatProperties(physDevice, format, &formatProps);
	return (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
}

VkFormat findSupportedDepthStencilFormat (Context& context, const bool useDepth, const bool useStencil)
{
	if (useDepth && !useStencil)
		return VK_FORMAT_D16_UNORM;		// must be supported

	const InstanceInterface&	vki			= context.getInstanceInterface();
	const VkPhysicalDevice		physDevice	= context.getPhysicalDevice();

	// One of these formats must be supported.

	if (isSupportedDepthStencilFormat(vki, physDevice, VK_FORMAT_D24_UNORM_S8_UINT))
		return VK_FORMAT_D24_UNORM_S8_UINT;

	if (isSupportedDepthStencilFormat(vki, physDevice, VK_FORMAT_D32_SFLOAT_S8_UINT))
		return VK_FORMAT_D32_SFLOAT_S8_UINT;

	return VK_FORMAT_UNDEFINED;
}


// MultisampleTest

MultisampleTest::MultisampleTest (tcu::TestContext&								testContext,
								  const std::string&							name,
								  const std::string&							description,
								  const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
								  const VkPipelineColorBlendAttachmentState&	blendState,
								  GeometryType									geometryType)
	: vkt::TestCase				(testContext, name, description)
	, m_multisampleStateParams	(multisampleStateParams)
	, m_colorBlendState			(blendState)
	, m_geometryType			(geometryType)
{
	if (m_multisampleStateParams.pSampleMask)
	{
		// Copy pSampleMask to avoid dependencies with other classes

		const deUint32 maskCount = deCeilFloatToInt32(float(m_multisampleStateParams.rasterizationSamples) / 32);

		for (deUint32 maskNdx = 0; maskNdx < maskCount; maskNdx++)
			m_sampleMask.push_back(m_multisampleStateParams.pSampleMask[maskNdx]);

		m_multisampleStateParams.pSampleMask = m_sampleMask.data();
	}
}

void MultisampleTest::initPrograms (SourceCollections& programCollection) const
{
	initMultisamplePrograms(programCollection, m_geometryType);
}

TestInstance* MultisampleTest::createInstance (Context& context) const
{
	return createMultisampleTestInstance(context, getPrimitiveTopology(m_geometryType), generateVertices(m_geometryType), m_multisampleStateParams, m_colorBlendState);
}


// RasterizationSamplesTest

RasterizationSamplesTest::RasterizationSamplesTest (tcu::TestContext&		testContext,
													const std::string&		name,
													const std::string&		description,
													VkSampleCountFlagBits	rasterizationSamples,
													GeometryType			geometryType,
													TestModeFlags			modeFlags)
	: MultisampleTest	(testContext, name, description, getRasterizationSamplesStateParams(rasterizationSamples), getDefaultColorBlendAttachmentState(), geometryType)
	, m_modeFlags		(modeFlags)
{
}

VkPipelineMultisampleStateCreateInfo RasterizationSamplesTest::getRasterizationSamplesStateParams (VkSampleCountFlagBits rasterizationSamples)
{
	const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineMultisampleStateCreateFlags	flags;
		rasterizationSamples,										// VkSampleCountFlagBits					rasterizationSamples;
		false,														// VkBool32									sampleShadingEnable;
		0.0f,														// float									minSampleShading;
		DE_NULL,													// const VkSampleMask*						pSampleMask;
		false,														// VkBool32									alphaToCoverageEnable;
		false														// VkBool32									alphaToOneEnable;
	};

	return multisampleStateParams;
}

TestInstance* RasterizationSamplesTest::createMultisampleTestInstance (Context&										context,
																	   VkPrimitiveTopology							topology,
																	   const std::vector<Vertex4RGBA>&				vertices,
																	   const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																	   const VkPipelineColorBlendAttachmentState&	colorBlendState) const
{
	return new RasterizationSamplesInstance(context, topology, vertices, multisampleStateParams, colorBlendState, m_modeFlags);
}


// MinSampleShadingTest

MinSampleShadingTest::MinSampleShadingTest (tcu::TestContext&		testContext,
											const std::string&		name,
											const std::string&		description,
											VkSampleCountFlagBits	rasterizationSamples,
											float					minSampleShading,
											GeometryType			geometryType)
	: MultisampleTest	(testContext, name, description, getMinSampleShadingStateParams(rasterizationSamples, minSampleShading), getDefaultColorBlendAttachmentState(), geometryType)
{
}

void MinSampleShadingTest::initPrograms (SourceCollections& programCollection) const
{
	initSampleShadingPrograms(programCollection, m_geometryType);
}

TestInstance* MinSampleShadingTest::createMultisampleTestInstance (Context&										context,
																   VkPrimitiveTopology							topology,
																   const std::vector<Vertex4RGBA>&				vertices,
																   const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																   const VkPipelineColorBlendAttachmentState&	colorBlendState) const
{
	return new MinSampleShadingInstance(context, topology, vertices, multisampleStateParams, colorBlendState);
}

VkPipelineMultisampleStateCreateInfo MinSampleShadingTest::getMinSampleShadingStateParams (VkSampleCountFlagBits rasterizationSamples, float minSampleShading)
{
	const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineMultisampleStateCreateFlags	flags;
		rasterizationSamples,										// VkSampleCountFlagBits					rasterizationSamples;
		true,														// VkBool32									sampleShadingEnable;
		minSampleShading,											// float									minSampleShading;
		DE_NULL,													// const VkSampleMask*						pSampleMask;
		false,														//  VkBool32								alphaToCoverageEnable;
		false														//  VkBool32								alphaToOneEnable;
	};

	return multisampleStateParams;
}


// SampleMaskTest

SampleMaskTest::SampleMaskTest (tcu::TestContext&					testContext,
								const std::string&					name,
								const std::string&					description,
								VkSampleCountFlagBits				rasterizationSamples,
								const std::vector<VkSampleMask>&	sampleMask,
								GeometryType						geometryType)
	: MultisampleTest	(testContext, name, description, getSampleMaskStateParams(rasterizationSamples, sampleMask), getDefaultColorBlendAttachmentState(), geometryType)
{
}

TestInstance* SampleMaskTest::createMultisampleTestInstance (Context&										context,
															 VkPrimitiveTopology							topology,
															 const std::vector<Vertex4RGBA>&				vertices,
															 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
															 const VkPipelineColorBlendAttachmentState&		colorBlendState) const
{
	return new SampleMaskInstance(context, topology,vertices, multisampleStateParams, colorBlendState);
}

VkPipelineMultisampleStateCreateInfo SampleMaskTest::getSampleMaskStateParams (VkSampleCountFlagBits rasterizationSamples, const std::vector<VkSampleMask>& sampleMask)
{
	const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineMultisampleStateCreateFlags	flags;
		rasterizationSamples,										// VkSampleCountFlagBits					rasterizationSamples;
		false,														// VkBool32									sampleShadingEnable;
		0.0f,														// float									minSampleShading;
		sampleMask.data(),											// const VkSampleMask*						pSampleMask;
		false,														// VkBool32									alphaToCoverageEnable;
		false														// VkBool32									alphaToOneEnable;
	};

	return multisampleStateParams;
}


// AlphaToOneTest

AlphaToOneTest::AlphaToOneTest (tcu::TestContext&		testContext,
								const std::string&		name,
								const std::string&		description,
								VkSampleCountFlagBits	rasterizationSamples)
	: MultisampleTest	(testContext, name, description, getAlphaToOneStateParams(rasterizationSamples), getAlphaToOneBlendState(), GEOMETRY_TYPE_GRADIENT_QUAD)
{
}

TestInstance* AlphaToOneTest::createMultisampleTestInstance (Context&										context,
															 VkPrimitiveTopology							topology,
															 const std::vector<Vertex4RGBA>&				vertices,
															 const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
															 const VkPipelineColorBlendAttachmentState&		colorBlendState) const
{
	return new AlphaToOneInstance(context, topology, vertices, multisampleStateParams, colorBlendState);
}

VkPipelineMultisampleStateCreateInfo AlphaToOneTest::getAlphaToOneStateParams (VkSampleCountFlagBits rasterizationSamples)
{
	const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineMultisampleStateCreateFlags	flags;
		rasterizationSamples,										// VkSampleCountFlagBits					rasterizationSamples;
		false,														// VkBool32									sampleShadingEnable;
		0.0f,														// float									minSampleShading;
		DE_NULL,													// const VkSampleMask*						pSampleMask;
		false,														// VkBool32									alphaToCoverageEnable;
		true														// VkBool32									alphaToOneEnable;
	};

	return multisampleStateParams;
}

VkPipelineColorBlendAttachmentState AlphaToOneTest::getAlphaToOneBlendState (void)
{
	const VkPipelineColorBlendAttachmentState colorBlendState =
	{
		true,														// VkBool32					blendEnable;
		VK_BLEND_FACTOR_SRC_ALPHA,									// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,						// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,											// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_SRC_ALPHA,									// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,						// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,											// VkBlendOp				alphaBlendOp;
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |		// VkColorComponentFlags	colorWriteMask;
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	return colorBlendState;
}


// AlphaToCoverageTest

AlphaToCoverageTest::AlphaToCoverageTest (tcu::TestContext&			testContext,
										  const std::string&		name,
										  const std::string&		description,
										  VkSampleCountFlagBits		rasterizationSamples,
										  GeometryType				geometryType)
	: MultisampleTest	(testContext, name, description, getAlphaToCoverageStateParams(rasterizationSamples), getDefaultColorBlendAttachmentState(), geometryType)
	, m_geometryType	(geometryType)
{
}

TestInstance* AlphaToCoverageTest::createMultisampleTestInstance (Context&										context,
																  VkPrimitiveTopology							topology,
																  const std::vector<Vertex4RGBA>&				vertices,
																  const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
																  const VkPipelineColorBlendAttachmentState&	colorBlendState) const
{
	return new AlphaToCoverageInstance(context, topology, vertices, multisampleStateParams, colorBlendState, m_geometryType);
}

VkPipelineMultisampleStateCreateInfo AlphaToCoverageTest::getAlphaToCoverageStateParams (VkSampleCountFlagBits rasterizationSamples)
{
	const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		0u,															// VkPipelineMultisampleStateCreateFlags	flags;
		rasterizationSamples,										// VkSampleCountFlagBits					rasterizationSamples;
		false,														// VkBool32									sampleShadingEnable;
		0.0f,														// float									minSampleShading;
		DE_NULL,													// const VkSampleMask*						pSampleMask;
		true,														// VkBool32									alphaToCoverageEnable;
		false														// VkBool32									alphaToOneEnable;
	};

	return multisampleStateParams;
}

// RasterizationSamplesInstance

RasterizationSamplesInstance::RasterizationSamplesInstance (Context&										context,
															VkPrimitiveTopology								topology,
															const std::vector<Vertex4RGBA>&					vertices,
															const VkPipelineMultisampleStateCreateInfo&		multisampleStateParams,
															const VkPipelineColorBlendAttachmentState&		blendState,
															const TestModeFlags								modeFlags)
	: vkt::TestInstance		(context)
	, m_colorFormat			(VK_FORMAT_R8G8B8A8_UNORM)
	, m_renderSize			(32, 32)
	, m_primitiveTopology	(topology)
	, m_vertices			(vertices)
	, m_fullQuadVertices	(generateVertices(GEOMETRY_TYPE_OPAQUE_QUAD_NONZERO_DEPTH))
	, m_modeFlags			(modeFlags)
{
	if (m_modeFlags != 0)
	{
		const bool		useDepth			= (m_modeFlags & TEST_MODE_DEPTH_BIT) != 0;
		const bool		useStencil			= (m_modeFlags & TEST_MODE_STENCIL_BIT) != 0;
		const VkFormat	depthStencilFormat	= findSupportedDepthStencilFormat(context, useDepth, useStencil);

		if (depthStencilFormat == VK_FORMAT_UNDEFINED)
			TCU_THROW(NotSupportedError, "Required depth/stencil format is not supported");

		const VkPrimitiveTopology		pTopology[2] = { m_primitiveTopology, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP };
		const std::vector<Vertex4RGBA>	pVertices[2] = { m_vertices, m_fullQuadVertices };

		m_multisampleRenderer = de::MovePtr<MultisampleRenderer>(
			new MultisampleRenderer(
				context, m_colorFormat, depthStencilFormat, m_renderSize, useDepth, useStencil, 2u, pTopology, pVertices, multisampleStateParams, blendState, RENDER_TYPE_RESOLVE));
	}
	else
	{
		m_multisampleRenderer = de::MovePtr<MultisampleRenderer>(
			new MultisampleRenderer(context, m_colorFormat, m_renderSize, topology, vertices, multisampleStateParams, blendState, RENDER_TYPE_RESOLVE));
	}
}

tcu::TestStatus RasterizationSamplesInstance::iterate (void)
{
	de::MovePtr<tcu::TextureLevel> level(m_multisampleRenderer->render());
	return verifyImage(level->getAccess());
}

tcu::TestStatus RasterizationSamplesInstance::verifyImage (const tcu::ConstPixelBufferAccess& result)
{
	// Verify range of unique pixels
	{
		const deUint32	numUniqueColors = getUniqueColorsCount(result);
		const deUint32	minUniqueColors	= 3;

		tcu::TestLog& log = m_context.getTestContext().getLog();

		log << tcu::TestLog::Message
			<< "\nMin. unique colors expected: " << minUniqueColors << "\n"
			<< "Unique colors found: " << numUniqueColors << "\n"
			<< tcu::TestLog::EndMessage;

		if (numUniqueColors < minUniqueColors)
			return tcu::TestStatus::fail("Unique colors out of expected bounds");
	}

	// Verify shape of the rendered primitive (fuzzy-compare)
	{
		const tcu::TextureFormat	tcuColorFormat	= mapVkFormat(m_colorFormat);
		const tcu::TextureFormat	tcuDepthFormat	= tcu::TextureFormat();
		const ColorVertexShader		vertexShader;
		const ColorFragmentShader	fragmentShader	(tcuColorFormat, tcuDepthFormat);
		const rr::Program			program			(&vertexShader, &fragmentShader);
		ReferenceRenderer			refRenderer		(m_renderSize.x(), m_renderSize.y(), 1, tcuColorFormat, tcuDepthFormat, &program);
		rr::RenderState				renderState		(refRenderer.getViewportState());

		if (m_primitiveTopology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
		{
			VkPhysicalDeviceProperties deviceProperties;

			m_context.getInstanceInterface().getPhysicalDeviceProperties(m_context.getPhysicalDevice(), &deviceProperties);

			// gl_PointSize is clamped to pointSizeRange
			renderState.point.pointSize = deFloatMin(3.0f, deviceProperties.limits.pointSizeRange[1]);
		}

		if (m_modeFlags == 0)
		{
			refRenderer.colorClear(tcu::Vec4(0.0f));
			refRenderer.draw(renderState, mapVkPrimitiveTopology(m_primitiveTopology), m_vertices);
		}
		else
		{
			// For depth/stencil case the primitive is invisible and the surroundings are filled red.
			refRenderer.colorClear(tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
			refRenderer.draw(renderState, mapVkPrimitiveTopology(m_primitiveTopology), m_vertices);
		}

		if (!tcu::fuzzyCompare(m_context.getTestContext().getLog(), "FuzzyImageCompare", "Image comparison", refRenderer.getAccess(), result, 0.05f, tcu::COMPARE_LOG_RESULT))
			return tcu::TestStatus::fail("Primitive has unexpected shape");
	}

	return tcu::TestStatus::pass("Primitive rendered, unique colors within expected bounds");
}


// MinSampleShadingInstance

MinSampleShadingInstance::MinSampleShadingInstance (Context&									context,
													VkPrimitiveTopology							topology,
													const std::vector<Vertex4RGBA>&				vertices,
													const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
													const VkPipelineColorBlendAttachmentState&	colorBlendState)
	: vkt::TestInstance			(context)
	, m_colorFormat				(VK_FORMAT_R8G8B8A8_UNORM)
	, m_renderSize				(32, 32)
	, m_primitiveTopology		(topology)
	, m_vertices				(vertices)
	, m_multisampleStateParams	(multisampleStateParams)
	, m_colorBlendState			(colorBlendState)
{
	VkPhysicalDeviceFeatures deviceFeatures;

	m_context.getInstanceInterface().getPhysicalDeviceFeatures(m_context.getPhysicalDevice(), &deviceFeatures);

	if (!deviceFeatures.sampleRateShading)
		throw tcu::NotSupportedError("Sample shading is not supported");
}

tcu::TestStatus MinSampleShadingInstance::iterate (void)
{
	de::MovePtr<tcu::TextureLevel>	noSampleshadingImage;
	std::vector<tcu::TextureLevel>	sampleShadedImages;

	// Render and resolve without sample shading
	{
		VkPipelineMultisampleStateCreateInfo multisampleStateParms = m_multisampleStateParams;
		multisampleStateParms.sampleShadingEnable	= VK_FALSE;
		multisampleStateParms.minSampleShading		= 0.0;

		MultisampleRenderer renderer (m_context, m_colorFormat, m_renderSize, m_primitiveTopology, m_vertices, multisampleStateParms, m_colorBlendState, RENDER_TYPE_RESOLVE);
		noSampleshadingImage  = renderer.render();
	}

	// Render with test minSampleShading and collect per-sample images
	{
		MultisampleRenderer renderer (m_context, m_colorFormat, m_renderSize, m_primitiveTopology, m_vertices, m_multisampleStateParams, m_colorBlendState, RENDER_TYPE_COPY_SAMPLES);
		renderer.render();

		sampleShadedImages.resize(m_multisampleStateParams.rasterizationSamples);
		for (deUint32 sampleId = 0; sampleId < sampleShadedImages.size(); sampleId++)
		{
			sampleShadedImages[sampleId] = *renderer.getSingleSampledImage(sampleId);
		}
	}

	// Log images
	{
		tcu::TestLog& testLog	= m_context.getTestContext().getLog();

		testLog << tcu::TestLog::ImageSet("Images", "Images")
				<< tcu::TestLog::Image("noSampleshadingImage", "Image rendered without sample shading", noSampleshadingImage->getAccess());

		for (deUint32 sampleId = 0; sampleId < sampleShadedImages.size(); sampleId++)
		{
			testLog << tcu::TestLog::Image("sampleShadedImage", "One sample of sample shaded image", sampleShadedImages[sampleId].getAccess());
		}
		testLog << tcu::TestLog::EndImageSet;
	}

	return verifySampleShadedImage(sampleShadedImages, noSampleshadingImage->getAccess());
}

tcu::TestStatus MinSampleShadingInstance::verifySampleShadedImage (const std::vector<tcu::TextureLevel>& sampleShadedImages, const tcu::ConstPixelBufferAccess& noSampleshadingImage)
{
	const deUint32	pixelCount	= noSampleshadingImage.getWidth() * noSampleshadingImage.getHeight() * noSampleshadingImage.getDepth();

	bool anyPixelCovered		= false;

	for (deUint32 pixelNdx = 0; pixelNdx < pixelCount; pixelNdx++)
	{
		const deUint32 noSampleShadingValue = *((const deUint32*)noSampleshadingImage.getDataPtr() + pixelNdx);

		if (noSampleShadingValue == 0)
		{
			// non-covered pixel, continue
			continue;
		}
		else
		{
			anyPixelCovered = true;
		}

		int numNotCoveredSamples = 0;

		std::map<deUint32, deUint32>	histogram; // map<pixel value, number of occurrences>

		// Collect histogram of occurrences or each pixel across all samples
		for (size_t i = 0; i < sampleShadedImages.size(); ++i)
		{
			const deUint32 sampleShadedValue = *((const deUint32*)sampleShadedImages[i].getAccess().getDataPtr() + pixelNdx);

			if (sampleShadedValue == 0)
			{
				numNotCoveredSamples++;
				continue;
			}

			if (histogram.find(sampleShadedValue) != histogram.end())
				histogram[sampleShadedValue]++;
			else
				histogram[sampleShadedValue] = 1;
		}

		if (numNotCoveredSamples == static_cast<int>(sampleShadedImages.size()))
		{
			return tcu::TestStatus::fail("Got uncovered pixel, where covered samples were expected");
		}

		const int uniqueColorsCount				= (int)histogram.size();
		const int expectedUniqueSamplesCount	= static_cast<int>(m_multisampleStateParams.minSampleShading * static_cast<float>(sampleShadedImages.size()) + 0.5f);

		if (uniqueColorsCount + numNotCoveredSamples < expectedUniqueSamplesCount)
		{
			return tcu::TestStatus::fail("Got less unique colors than requested through minSampleShading");
		}
	}

	if (!anyPixelCovered)
	{
		return tcu::TestStatus::fail("Did not get any covered pixel, cannot test minSampleShading");
	}

	return tcu::TestStatus::pass("Got proper count of unique colors");
}

SampleMaskInstance::SampleMaskInstance (Context&										context,
										VkPrimitiveTopology								topology,
										const std::vector<Vertex4RGBA>&					vertices,
										const VkPipelineMultisampleStateCreateInfo&		multisampleStateParams,
										const VkPipelineColorBlendAttachmentState&		blendState)
	: vkt::TestInstance			(context)
	, m_colorFormat				(VK_FORMAT_R8G8B8A8_UNORM)
	, m_renderSize				(32, 32)
	, m_primitiveTopology		(topology)
	, m_vertices				(vertices)
	, m_multisampleStateParams	(multisampleStateParams)
	, m_colorBlendState			(blendState)
{
}

tcu::TestStatus SampleMaskInstance::iterate (void)
{
	de::MovePtr<tcu::TextureLevel>				testSampleMaskImage;
	de::MovePtr<tcu::TextureLevel>				minSampleMaskImage;
	de::MovePtr<tcu::TextureLevel>				maxSampleMaskImage;

	// Render with test flags
	{
		MultisampleRenderer renderer (m_context, m_colorFormat, m_renderSize, m_primitiveTopology, m_vertices, m_multisampleStateParams, m_colorBlendState, RENDER_TYPE_RESOLVE);
		testSampleMaskImage = renderer.render();
	}

	// Render with all flags off
	{
		VkPipelineMultisampleStateCreateInfo	multisampleParams	= m_multisampleStateParams;
		const std::vector<VkSampleMask>			sampleMask			(multisampleParams.rasterizationSamples / 32, (VkSampleMask)0);

		multisampleParams.pSampleMask = sampleMask.data();

		MultisampleRenderer renderer (m_context, m_colorFormat, m_renderSize, m_primitiveTopology, m_vertices, multisampleParams, m_colorBlendState, RENDER_TYPE_RESOLVE);
		minSampleMaskImage = renderer.render();
	}

	// Render with all flags on
	{
		VkPipelineMultisampleStateCreateInfo	multisampleParams	= m_multisampleStateParams;
		const std::vector<VkSampleMask>			sampleMask			(multisampleParams.rasterizationSamples / 32, ~((VkSampleMask)0));

		multisampleParams.pSampleMask = sampleMask.data();

		MultisampleRenderer renderer (m_context, m_colorFormat, m_renderSize, m_primitiveTopology, m_vertices, multisampleParams, m_colorBlendState, RENDER_TYPE_RESOLVE);
		maxSampleMaskImage = renderer.render();
	}

	return verifyImage(testSampleMaskImage->getAccess(), minSampleMaskImage->getAccess(), maxSampleMaskImage->getAccess());
}

tcu::TestStatus SampleMaskInstance::verifyImage (const tcu::ConstPixelBufferAccess& testSampleMaskImage,
												 const tcu::ConstPixelBufferAccess& minSampleMaskImage,
												 const tcu::ConstPixelBufferAccess& maxSampleMaskImage)
{
	const deUint32	testColorCount	= getUniqueColorsCount(testSampleMaskImage);
	const deUint32	minColorCount	= getUniqueColorsCount(minSampleMaskImage);
	const deUint32	maxColorCount	= getUniqueColorsCount(maxSampleMaskImage);

	tcu::TestLog& log = m_context.getTestContext().getLog();

	log << tcu::TestLog::Message
		<< "\nColors found: " << testColorCount << "\n"
		<< "Min. colors expected: " << minColorCount << "\n"
		<< "Max. colors expected: " << maxColorCount << "\n"
		<< tcu::TestLog::EndMessage;

	if (minColorCount > testColorCount || testColorCount > maxColorCount)
		return tcu::TestStatus::fail("Unique colors out of expected bounds");
	else
		return tcu::TestStatus::pass("Unique colors within expected bounds");
}

tcu::TestStatus testRasterSamplesConsistency (Context& context, GeometryType geometryType)
{
	// Use triangle only.
	DE_UNREF(geometryType);

	const VkSampleCountFlagBits samples[] =
	{
		VK_SAMPLE_COUNT_1_BIT,
		VK_SAMPLE_COUNT_2_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_64_BIT
	};

	const Vertex4RGBA vertexData[3] =
	{
		{
			tcu::Vec4(-0.75f, 0.0f, 0.0f, 1.0f),
			tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
		},
		{
			tcu::Vec4(0.75f, 0.125f, 0.0f, 1.0f),
			tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
		},
		{
			tcu::Vec4(0.75f, -0.125f, 0.0f, 1.0f),
			tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f)
		}
	};

	const std::vector<Vertex4RGBA>	vertices			(vertexData, vertexData + 3);
	deUint32						prevUniqueColors	= 2;
	int								renderCount			= 0;

	// Do not render with 1 sample (start with samplesNdx = 1).
	for (int samplesNdx = 1; samplesNdx < DE_LENGTH_OF_ARRAY(samples); samplesNdx++)
	{
		if (!isSupportedSampleCount(context.getInstanceInterface(), context.getPhysicalDevice(), samples[samplesNdx]))
			continue;

		const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineMultisampleStateCreateFlags	flags;
			samples[samplesNdx],										// VkSampleCountFlagBits					rasterizationSamples;
			false,														// VkBool32									sampleShadingEnable;
			0.0f,														// float									minSampleShading;
			DE_NULL,													// const VkSampleMask*						pSampleMask;
			false,														// VkBool32									alphaToCoverageEnable;
			false														// VkBool32									alphaToOneEnable;
		};

		MultisampleRenderer				renderer		(context, VK_FORMAT_R8G8B8A8_UNORM, tcu::IVec2(32, 32), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices, multisampleStateParams, getDefaultColorBlendAttachmentState(), RENDER_TYPE_RESOLVE);
		de::MovePtr<tcu::TextureLevel>	result			= renderer.render();
		const deUint32					uniqueColors	= getUniqueColorsCount(result->getAccess());

		renderCount++;

		if (prevUniqueColors > uniqueColors)
		{
			std::ostringstream message;

			message << "More unique colors generated with " << samples[samplesNdx - 1] << " than with " << samples[samplesNdx];
			return tcu::TestStatus::fail(message.str());
		}

		prevUniqueColors = uniqueColors;
	}

	if (renderCount == 0)
		throw tcu::NotSupportedError("Multisampling is unsupported");

	return tcu::TestStatus::pass("Number of unique colors increases as the sample count increases");
}


// AlphaToOneInstance

AlphaToOneInstance::AlphaToOneInstance (Context&									context,
										VkPrimitiveTopology							topology,
										const std::vector<Vertex4RGBA>&				vertices,
										const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
										const VkPipelineColorBlendAttachmentState&	blendState)
	: vkt::TestInstance			(context)
	, m_colorFormat				(VK_FORMAT_R8G8B8A8_UNORM)
	, m_renderSize				(32, 32)
	, m_primitiveTopology		(topology)
	, m_vertices				(vertices)
	, m_multisampleStateParams	(multisampleStateParams)
	, m_colorBlendState			(blendState)
{
	VkPhysicalDeviceFeatures deviceFeatures;

	context.getInstanceInterface().getPhysicalDeviceFeatures(context.getPhysicalDevice(), &deviceFeatures);

	if (!deviceFeatures.alphaToOne)
		throw tcu::NotSupportedError("Alpha-to-one is not supported");
}

tcu::TestStatus AlphaToOneInstance::iterate	(void)
{
	DE_ASSERT(m_multisampleStateParams.alphaToOneEnable);
	DE_ASSERT(m_colorBlendState.blendEnable);

	de::MovePtr<tcu::TextureLevel>	alphaOneImage;
	de::MovePtr<tcu::TextureLevel>	noAlphaOneImage;

	// Render with blend enabled and alpha to one on
	{
		MultisampleRenderer renderer (m_context, m_colorFormat, m_renderSize, m_primitiveTopology, m_vertices, m_multisampleStateParams, m_colorBlendState, RENDER_TYPE_RESOLVE);
		alphaOneImage = renderer.render();
	}

	// Render with blend enabled and alpha to one off
	{
		VkPipelineMultisampleStateCreateInfo	multisampleParams	= m_multisampleStateParams;
		multisampleParams.alphaToOneEnable = false;

		MultisampleRenderer renderer (m_context, m_colorFormat, m_renderSize, m_primitiveTopology, m_vertices, multisampleParams, m_colorBlendState, RENDER_TYPE_RESOLVE);
		noAlphaOneImage = renderer.render();
	}

	return verifyImage(alphaOneImage->getAccess(), noAlphaOneImage->getAccess());
}

tcu::TestStatus AlphaToOneInstance::verifyImage (const tcu::ConstPixelBufferAccess&	alphaOneImage,
												 const tcu::ConstPixelBufferAccess&	noAlphaOneImage)
{
	for (int y = 0; y < m_renderSize.y(); y++)
	{
		for (int x = 0; x < m_renderSize.x(); x++)
		{
			if (!tcu::boolAll(tcu::greaterThanEqual(alphaOneImage.getPixel(x, y), noAlphaOneImage.getPixel(x, y))))
			{
				std::ostringstream message;
				message << "Unsatisfied condition: " << alphaOneImage.getPixel(x, y) << " >= " << noAlphaOneImage.getPixel(x, y);
				return tcu::TestStatus::fail(message.str());
			}
		}
	}

	return tcu::TestStatus::pass("Image rendered with alpha-to-one contains pixels of image rendered with no alpha-to-one");
}


// AlphaToCoverageInstance

AlphaToCoverageInstance::AlphaToCoverageInstance (Context&										context,
												  VkPrimitiveTopology							topology,
												  const std::vector<Vertex4RGBA>&				vertices,
												  const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
												  const VkPipelineColorBlendAttachmentState&	blendState,
												  GeometryType									geometryType)
	: vkt::TestInstance			(context)
	, m_colorFormat				(VK_FORMAT_R8G8B8A8_UNORM)
	, m_renderSize				(32, 32)
	, m_primitiveTopology		(topology)
	, m_vertices				(vertices)
	, m_multisampleStateParams	(multisampleStateParams)
	, m_colorBlendState			(blendState)
	, m_geometryType			(geometryType)
{
}

tcu::TestStatus AlphaToCoverageInstance::iterate (void)
{
	DE_ASSERT(m_multisampleStateParams.alphaToCoverageEnable);

	de::MovePtr<tcu::TextureLevel>	result;
	MultisampleRenderer				renderer	(m_context, m_colorFormat, m_renderSize, m_primitiveTopology, m_vertices, m_multisampleStateParams, m_colorBlendState, RENDER_TYPE_RESOLVE);

	result = renderer.render();

	return verifyImage(result->getAccess());
}

tcu::TestStatus AlphaToCoverageInstance::verifyImage (const tcu::ConstPixelBufferAccess&	result)
{
	float maxColorValue;

	switch (m_geometryType)
	{
		case GEOMETRY_TYPE_OPAQUE_QUAD:
			maxColorValue = 1.01f;
			break;

		case GEOMETRY_TYPE_TRANSLUCENT_QUAD:
			maxColorValue = 0.52f;
			break;

		case GEOMETRY_TYPE_INVISIBLE_QUAD:
			maxColorValue = 0.01f;
			break;

		default:
			maxColorValue = 0.0f;
			DE_ASSERT(false);
	}

	for (int y = 0; y < m_renderSize.y(); y++)
	{
		for (int x = 0; x < m_renderSize.x(); x++)
		{
			if (result.getPixel(x, y).x() > maxColorValue)
			{
				std::ostringstream message;
				message << "Pixel is not below the threshold value (" << result.getPixel(x, y).x() << " > " << maxColorValue << ")";
				return tcu::TestStatus::fail(message.str());
			}
		}
	}

	return tcu::TestStatus::pass("Image matches reference value");
}


// MultisampleRenderer

MultisampleRenderer::MultisampleRenderer (Context&										context,
										  const VkFormat								colorFormat,
										  const tcu::IVec2&								renderSize,
										  const VkPrimitiveTopology						topology,
										  const std::vector<Vertex4RGBA>&				vertices,
										  const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
										  const VkPipelineColorBlendAttachmentState&	blendState,
										  const RenderType								renderType)
	: m_context					(context)
	, m_colorFormat				(colorFormat)
	, m_depthStencilFormat		(VK_FORMAT_UNDEFINED)
	, m_renderSize				(renderSize)
	, m_useDepth				(false)
	, m_useStencil				(false)
	, m_multisampleStateParams	(multisampleStateParams)
	, m_colorBlendState			(blendState)
	, m_renderType				(renderType)
{
	initialize(context, 1u, &topology, &vertices);
}

MultisampleRenderer::MultisampleRenderer (Context&										context,
										  const VkFormat								colorFormat,
										  const VkFormat								depthStencilFormat,
										  const tcu::IVec2&								renderSize,
										  const bool									useDepth,
										  const bool									useStencil,
										  const deUint32								numTopologies,
										  const VkPrimitiveTopology*					pTopology,
										  const std::vector<Vertex4RGBA>*				pVertices,
										  const VkPipelineMultisampleStateCreateInfo&	multisampleStateParams,
										  const VkPipelineColorBlendAttachmentState&	blendState,
										  const RenderType								renderType)
	: m_context					(context)
	, m_colorFormat				(colorFormat)
	, m_depthStencilFormat		(depthStencilFormat)
	, m_renderSize				(renderSize)
	, m_useDepth				(useDepth)
	, m_useStencil				(useStencil)
	, m_multisampleStateParams	(multisampleStateParams)
	, m_colorBlendState			(blendState)
	, m_renderType				(renderType)
{
	initialize(context, numTopologies, pTopology, pVertices);
}

void MultisampleRenderer::initialize (Context&									context,
									  const deUint32							numTopologies,
									  const VkPrimitiveTopology*				pTopology,
									  const std::vector<Vertex4RGBA>*			pVertices)
{
	if (!isSupportedSampleCount(context.getInstanceInterface(), context.getPhysicalDevice(), m_multisampleStateParams.rasterizationSamples))
		throw tcu::NotSupportedError("Unsupported number of rasterization samples");

	const DeviceInterface&		vk						= context.getDeviceInterface();
	const VkDevice				vkDevice				= context.getDevice();
	const deUint32				queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	SimpleAllocator				memAlloc				(vk, vkDevice, getPhysicalDeviceMemoryProperties(context.getInstanceInterface(), context.getPhysicalDevice()));
	const VkComponentMapping	componentMappingRGBA	= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	// Create color image
	{

		const VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			(m_renderType == RENDER_TYPE_COPY_SAMPLES ? VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT : (VkImageUsageFlagBits)0u);

		const VkImageCreateInfo colorImageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,										// VkStructureType			sType;
			DE_NULL,																	// const void*				pNext;
			0u,																			// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,															// VkImageType				imageType;
			m_colorFormat,																// VkFormat					format;
			{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y(), 1u },				// VkExtent3D				extent;
			1u,																			// deUint32					mipLevels;
			1u,																			// deUint32					arrayLayers;
			m_multisampleStateParams.rasterizationSamples,								// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,													// VkImageTiling			tiling;
			imageUsageFlags,															// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,													// VkSharingMode			sharingMode;
			1u,																			// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,															// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED,													// VkImageLayout			initialLayout;
		};

		m_colorImage			= createImage(vk, vkDevice, &colorImageParams);

		// Allocate and bind color image memory
		m_colorImageAlloc		= memAlloc.allocate(getImageMemoryRequirements(vk, vkDevice, *m_colorImage), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(vkDevice, *m_colorImage, m_colorImageAlloc->getMemory(), m_colorImageAlloc->getOffset()));
	}

	// Create resolve image
	if (m_renderType == RENDER_TYPE_RESOLVE)
	{
		const VkImageCreateInfo resolveImageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,											// VkStructureType			sType;
			DE_NULL,																		// const void*				pNext;
			0u,																				// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,																// VkImageType				imageType;
			m_colorFormat,																	// VkFormat					format;
			{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y(), 1u },					// VkExtent3D				extent;
			1u,																				// deUint32					mipLevels;
			1u,																				// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,															// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,														// VkImageTiling			tiling;
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |			// VkImageUsageFlags		usage;
				VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_SHARING_MODE_EXCLUSIVE,														// VkSharingMode			sharingMode;
			1u,																				// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,																// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED														// VkImageLayout			initialLayout;
		};

		m_resolveImage = createImage(vk, vkDevice, &resolveImageParams);

		// Allocate and bind resolve image memory
		m_resolveImageAlloc = memAlloc.allocate(getImageMemoryRequirements(vk, vkDevice, *m_resolveImage), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(vkDevice, *m_resolveImage, m_resolveImageAlloc->getMemory(), m_resolveImageAlloc->getOffset()));

		// Create resolve attachment view
		{
			const VkImageViewCreateInfo resolveAttachmentViewParams =
			{
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType			sType;
				DE_NULL,										// const void*				pNext;
				0u,												// VkImageViewCreateFlags	flags;
				*m_resolveImage,								// VkImage					image;
				VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType			viewType;
				m_colorFormat,									// VkFormat					format;
				componentMappingRGBA,							// VkComponentMapping		components;
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }	// VkImageSubresourceRange	subresourceRange;
			};

			m_resolveAttachmentView = createImageView(vk, vkDevice, &resolveAttachmentViewParams);
		}
	}

	// Create per-sample output images
	if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
	{
		const VkImageCreateInfo perSampleImageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,											// VkStructureType			sType;
			DE_NULL,																		// const void*				pNext;
			0u,																				// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,																// VkImageType				imageType;
			m_colorFormat,																	// VkFormat					format;
			{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y(), 1u },					// VkExtent3D				extent;
			1u,																				// deUint32					mipLevels;
			1u,																				// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,															// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,														// VkImageTiling			tiling;
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |			// VkImageUsageFlags		usage;
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_SHARING_MODE_EXCLUSIVE,														// VkSharingMode			sharingMode;
			1u,																				// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,																// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED														// VkImageLayout			initialLayout;
		};

		m_perSampleImages.resize(static_cast<size_t>(m_multisampleStateParams.rasterizationSamples));

		for (size_t i = 0; i < m_perSampleImages.size(); ++i)
		{
			m_perSampleImages[i]	= de::SharedPtr<PerSampleImage>(new PerSampleImage);
			PerSampleImage& image	= *m_perSampleImages[i];

			image.m_image			= createImage(vk, vkDevice, &perSampleImageParams);

			// Allocate and bind image memory
			image.m_imageAlloc		= memAlloc.allocate(getImageMemoryRequirements(vk, vkDevice, *image.m_image), MemoryRequirement::Any);
			VK_CHECK(vk.bindImageMemory(vkDevice, *image.m_image, image.m_imageAlloc->getMemory(), image.m_imageAlloc->getOffset()));

			// Create per-sample attachment view
			{
				const VkImageViewCreateInfo perSampleAttachmentViewParams =
				{
					VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType			sType;
					DE_NULL,										// const void*				pNext;
					0u,												// VkImageViewCreateFlags	flags;
					*image.m_image,									// VkImage					image;
					VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType			viewType;
					m_colorFormat,									// VkFormat					format;
					componentMappingRGBA,							// VkComponentMapping		components;
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }	// VkImageSubresourceRange	subresourceRange;
				};

				image.m_attachmentView = createImageView(vk, vkDevice, &perSampleAttachmentViewParams);
			}
		}
	}

	// Create a depth/stencil image
	if (m_useDepth || m_useStencil)
	{
		const VkImageCreateInfo depthStencilImageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,											// VkStructureType			sType;
			DE_NULL,																		// const void*				pNext;
			0u,																				// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,																// VkImageType				imageType;
			m_depthStencilFormat,															// VkFormat					format;
			{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y(), 1u },					// VkExtent3D				extent;
			1u,																				// deUint32					mipLevels;
			1u,																				// deUint32					arrayLayers;
			m_multisampleStateParams.rasterizationSamples,									// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,														// VkImageTiling			tiling;
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,									// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,														// VkSharingMode			sharingMode;
			1u,																				// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,																// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED														// VkImageLayout			initialLayout;
		};

		m_depthStencilImage = createImage(vk, vkDevice, &depthStencilImageParams);

		// Allocate and bind depth/stencil image memory
		m_depthStencilImageAlloc = memAlloc.allocate(getImageMemoryRequirements(vk, vkDevice, *m_depthStencilImage), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(vkDevice, *m_depthStencilImage, m_depthStencilImageAlloc->getMemory(), m_depthStencilImageAlloc->getOffset()));
	}

	// Create color attachment view
	{
		const VkImageViewCreateInfo colorAttachmentViewParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			0u,												// VkImageViewCreateFlags	flags;
			*m_colorImage,									// VkImage					image;
			VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType			viewType;
			m_colorFormat,									// VkFormat					format;
			componentMappingRGBA,							// VkComponentMapping		components;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }	// VkImageSubresourceRange	subresourceRange;
		};

		m_colorAttachmentView = createImageView(vk, vkDevice, &colorAttachmentViewParams);
	}

	VkImageAspectFlags	depthStencilAttachmentAspect	= (VkImageAspectFlagBits)0;

	// Create depth/stencil attachment view
	if (m_useDepth || m_useStencil)
	{
		depthStencilAttachmentAspect = getImageAspectFlags(m_depthStencilFormat);

		const VkImageViewCreateInfo depthStencilAttachmentViewParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkImageViewCreateFlags	flags;
			*m_depthStencilImage,								// VkImage					image;
			VK_IMAGE_VIEW_TYPE_2D,								// VkImageViewType			viewType;
			m_depthStencilFormat,								// VkFormat					format;
			componentMappingRGBA,								// VkComponentMapping		components;
			{ depthStencilAttachmentAspect, 0u, 1u, 0u, 1u }	// VkImageSubresourceRange	subresourceRange;
		};

		m_depthStencilAttachmentView = createImageView(vk, vkDevice, &depthStencilAttachmentViewParams);
	}

	// Create render pass
	{
		std::vector<VkAttachmentDescription> attachmentDescriptions;
		{
			const VkAttachmentDescription colorAttachmentDescription =
			{
				0u,													// VkAttachmentDescriptionFlags		flags;
				m_colorFormat,										// VkFormat							format;
				m_multisampleStateParams.rasterizationSamples,		// VkSampleCountFlagBits			samples;
				VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					initialLayout;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout					finalLayout;
			};
			attachmentDescriptions.push_back(colorAttachmentDescription);
		}

		deUint32 resolveAttachmentIndex = VK_ATTACHMENT_UNUSED;

		if (m_renderType == RENDER_TYPE_RESOLVE)
		{
			resolveAttachmentIndex = static_cast<deUint32>(attachmentDescriptions.size());

			const VkAttachmentDescription resolveAttachmentDescription =
			{
				0u,													// VkAttachmentDescriptionFlags		flags;
				m_colorFormat,										// VkFormat							format;
				VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
				VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					initialLayout;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout					finalLayout;
			};
			attachmentDescriptions.push_back(resolveAttachmentDescription);
		}

		deUint32 perSampleAttachmentIndex = VK_ATTACHMENT_UNUSED;

		if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
		{
			perSampleAttachmentIndex = static_cast<deUint32>(attachmentDescriptions.size());

			const VkAttachmentDescription perSampleAttachmentDescription =
			{
				0u,													// VkAttachmentDescriptionFlags		flags;
				m_colorFormat,										// VkFormat							format;
				VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
				VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
				VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
				VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					initialLayout;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout					finalLayout;
			};

			for (size_t i = 0; i < m_perSampleImages.size(); ++i)
			{
				attachmentDescriptions.push_back(perSampleAttachmentDescription);
			}
		}

		deUint32 depthStencilAttachmentIndex = VK_ATTACHMENT_UNUSED;

		if (m_useDepth || m_useStencil)
		{
			depthStencilAttachmentIndex = static_cast<deUint32>(attachmentDescriptions.size());

			const VkAttachmentDescription depthStencilAttachmentDescription =
			{
				0u,																					// VkAttachmentDescriptionFlags		flags;
				m_depthStencilFormat,																// VkFormat							format;
				m_multisampleStateParams.rasterizationSamples,										// VkSampleCountFlagBits			samples;
				(m_useDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE),		// VkAttachmentLoadOp				loadOp;
				(m_useDepth ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE),		// VkAttachmentStoreOp				storeOp;
				(m_useStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE),		// VkAttachmentStoreOp				stencilLoadOp;
				(m_useStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE),	// VkAttachmentStoreOp				stencilStoreOp;
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,									// VkImageLayout					initialLayout;
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL									// VkImageLayout					finalLayout;
			};
			attachmentDescriptions.push_back(depthStencilAttachmentDescription);
		};

		const VkAttachmentReference colorAttachmentReference =
		{
			0u,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
		};

		const VkAttachmentReference inputAttachmentReference =
		{
			0u,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL			// VkImageLayout	layout;
		};

		const VkAttachmentReference resolveAttachmentReference =
		{
			resolveAttachmentIndex,								// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
		};

		std::vector<VkAttachmentReference> perSampleAttachmentReferences(m_perSampleImages.size());
		if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
		{
			for (size_t i = 0; i < m_perSampleImages.size(); ++i)
			{
				const VkAttachmentReference perSampleAttachmentReference =
				{
					perSampleAttachmentIndex + static_cast<deUint32>(i),	// deUint32			attachment;
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL				// VkImageLayout	layout;
				};
				perSampleAttachmentReferences[i] = perSampleAttachmentReference;
			}
		}

		const VkAttachmentReference depthStencilAttachmentReference =
		{
			depthStencilAttachmentIndex,						// deUint32			attachment;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
		};

		std::vector<VkSubpassDescription>	subpassDescriptions;
		std::vector<VkSubpassDependency>	subpassDependencies;

		{
			const VkSubpassDescription renderSubpassDescription =
			{
				0u,																				// VkSubpassDescriptionFlags	flags;
				VK_PIPELINE_BIND_POINT_GRAPHICS,												// VkPipelineBindPoint			pipelineBindPoint;
				0u,																				// deUint32						inputAttachmentCount;
				DE_NULL,																		// const VkAttachmentReference*	pInputAttachments;
				1u,																				// deUint32						colorAttachmentCount;
				&colorAttachmentReference,														// const VkAttachmentReference*	pColorAttachments;
				(m_renderType == RENDER_TYPE_RESOLVE) ? &resolveAttachmentReference : DE_NULL,	// const VkAttachmentReference*	pResolveAttachments;
				(m_useDepth || m_useStencil ? &depthStencilAttachmentReference : DE_NULL),		// const VkAttachmentReference*	pDepthStencilAttachment;
				0u,																				// deUint32						preserveAttachmentCount;
				DE_NULL																			// const VkAttachmentReference*	pPreserveAttachments;
			};
			subpassDescriptions.push_back(renderSubpassDescription);
		}

		if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
		{

			for (size_t i = 0; i < m_perSampleImages.size(); ++i)
			{
				const VkSubpassDescription copySampleSubpassDescription =
				{
					0u,													// VkSubpassDescriptionFlags		flags;
					VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
					1u,													// deUint32							inputAttachmentCount;
					&inputAttachmentReference,							// const VkAttachmentReference*		pInputAttachments;
					1u,													// deUint32							colorAttachmentCount;
					&perSampleAttachmentReferences[i],					// const VkAttachmentReference*		pColorAttachments;
					DE_NULL,											// const VkAttachmentReference*		pResolveAttachments;
					DE_NULL,											// const VkAttachmentReference*		pDepthStencilAttachment;
					0u,													// deUint32							preserveAttachmentCount;
					DE_NULL												// const VkAttachmentReference*		pPreserveAttachments;
				};
				subpassDescriptions.push_back(copySampleSubpassDescription);

				const VkSubpassDependency copySampleSubpassDependency =
				{
					0u,													// deUint32							srcSubpass
					1u + static_cast<deUint32>(i),						// deUint32							dstSubpass
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,					// VkPipelineStageFlags				srcStageMask
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,				// VkPipelineStageFlags				dstStageMask
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// VkAccessFlags					srcAccessMask
					VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,				// VkAccessFlags					dstAccessMask
					0u,													// VkDependencyFlags				dependencyFlags
				};
				subpassDependencies.push_back(copySampleSubpassDependency);
			}
		}

		const VkRenderPassCreateInfo renderPassParams =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,					// VkStructureType					sType;
			DE_NULL,													// const void*						pNext;
			0u,															// VkRenderPassCreateFlags			flags;
			(deUint32)attachmentDescriptions.size(),					// deUint32							attachmentCount;
			&attachmentDescriptions[0],									// const VkAttachmentDescription*	pAttachments;
			(deUint32)subpassDescriptions.size(),						// deUint32							subpassCount;
			&subpassDescriptions[0],									// const VkSubpassDescription*		pSubpasses;
			(deUint32)subpassDependencies.size(),						// deUint32							dependencyCount;
			subpassDependencies.size() != 0 ? &subpassDependencies[0] : DE_NULL
		};

		m_renderPass = createRenderPass(vk, vkDevice, &renderPassParams);
	}

	// Create framebuffer
	{
		std::vector<VkImageView> attachments;
		attachments.push_back(*m_colorAttachmentView);
		if (m_renderType == RENDER_TYPE_RESOLVE)
		{
			attachments.push_back(*m_resolveAttachmentView);
		}
		if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
		{
			for (size_t i = 0; i < m_perSampleImages.size(); ++i)
			{
				attachments.push_back(*m_perSampleImages[i]->m_attachmentView);
			}
		}

		if (m_useDepth || m_useStencil)
		{
			attachments.push_back(*m_depthStencilAttachmentView);
		}

		const VkFramebufferCreateInfo framebufferParams =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkFramebufferCreateFlags			flags;
			*m_renderPass,										// VkRenderPass						renderPass;
			(deUint32)attachments.size(),						// deUint32							attachmentCount;
			&attachments[0],									// const VkImageView*				pAttachments;
			(deUint32)m_renderSize.x(),							// deUint32							width;
			(deUint32)m_renderSize.y(),							// deUint32							height;
			1u													// deUint32							layers;
		};

		m_framebuffer = createFramebuffer(vk, vkDevice, &framebufferParams);
	}

	// Create pipeline layout
	{
		const VkPipelineLayoutCreateInfo pipelineLayoutParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkPipelineLayoutCreateFlags		flags;
			0u,													// deUint32							setLayoutCount;
			DE_NULL,											// const VkDescriptorSetLayout*		pSetLayouts;
			0u,													// deUint32							pushConstantRangeCount;
			DE_NULL												// const VkPushConstantRange*		pPushConstantRanges;
		};

		m_pipelineLayout = createPipelineLayout(vk, vkDevice, &pipelineLayoutParams);

		if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
		{

			// Create descriptor set layout
			const VkDescriptorSetLayoutBinding		layoutBinding					=
			{
				0u,															// deUint32								binding;
				VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,						// VkDescriptorType						descriptorType;
				1u,															// deUint32								descriptorCount;
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlags					stageFlags;
				DE_NULL,													// const VkSampler*						pImmutableSamplers;
			};

			const VkDescriptorSetLayoutCreateInfo	descriptorSetLayoutParams		=
			{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,		// VkStructureType						sType
				DE_NULL,													// const void*							pNext
				0u,															// VkDescriptorSetLayoutCreateFlags		flags
				1u,															// deUint32								bindingCount
				&layoutBinding												// const VkDescriptorSetLayoutBinding*	pBindings
			};
			m_copySampleDesciptorLayout	= createDescriptorSetLayout(vk, vkDevice, &descriptorSetLayoutParams);

			// Create pipeline layout

			const VkPushConstantRange				pushConstantRange				=
			{
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlags					stageFlags;
				0u,															// deUint32								offset;
				sizeof(deInt32)												// deUint32								size;
			};
			const VkPipelineLayoutCreateInfo		copySamplePipelineLayoutParams	=
			{
				VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,				// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineLayoutCreateFlags			flags;
				1u,															// deUint32								setLayoutCount;
				&m_copySampleDesciptorLayout.get(),							// const VkDescriptorSetLayout*			pSetLayouts;
				1u,															// deUint32								pushConstantRangeCount;
				&pushConstantRange											// const VkPushConstantRange*			pPushConstantRanges;
			};
			m_copySamplePipelineLayout		= createPipelineLayout(vk, vkDevice, &copySamplePipelineLayoutParams);
		}
	}

	m_vertexShaderModule	= createShaderModule(vk, vkDevice, m_context.getBinaryCollection().get("color_vert"), 0);
	m_fragmentShaderModule	= createShaderModule(vk, vkDevice, m_context.getBinaryCollection().get("color_frag"), 0);

	if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
	{
		m_copySampleVertexShaderModule		= createShaderModule(vk, vkDevice, m_context.getBinaryCollection().get("quad_vert"), 0);
		m_copySampleFragmentShaderModule	= createShaderModule(vk, vkDevice, m_context.getBinaryCollection().get("copy_sample_frag"), 0);
	}

	// Create pipeline
	{
		const VkPipelineShaderStageCreateInfo	shaderStageParams[2] =
		{
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits				stage;
				*m_vertexShaderModule,										// VkShaderModule						module;
				"main",														// const char*							pName;
				DE_NULL														// const VkSpecializationInfo*			pSpecializationInfo;
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits				stage;
				*m_fragmentShaderModule,									// VkShaderModule						module;
				"main",														// const char*							pName;
				DE_NULL														// const VkSpecializationInfo*			pSpecializationInfo;
			}
		};

		const VkVertexInputBindingDescription	vertexInputBindingDescription =
		{
			0u,									// deUint32				binding;
			sizeof(Vertex4RGBA),				// deUint32				stride;
			VK_VERTEX_INPUT_RATE_VERTEX			// VkVertexInputRate	inputRate;
		};

		const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
		{
			{
				0u,									// deUint32	location;
				0u,									// deUint32	binding;
				VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat	format;
				0u									// deUint32	offset;
			},
			{
				1u,									// deUint32	location;
				0u,									// deUint32	binding;
				VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat	format;
				DE_OFFSET_OF(Vertex4RGBA, color),	// deUint32	offset;
			}
		};

		const VkPipelineVertexInputStateCreateInfo vertexInputStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineVertexInputStateCreateFlags	flags;
			1u,																// deUint32									vertexBindingDescriptionCount;
			&vertexInputBindingDescription,									// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
			2u,																// deUint32									vertexAttributeDescriptionCount;
			vertexInputAttributeDescriptions								// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
		};

		// Topology is set before the pipeline creation.
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineInputAssemblyStateCreateFlags	flags;
			VK_PRIMITIVE_TOPOLOGY_LAST,										// VkPrimitiveTopology						topology;
			false															// VkBool32									primitiveRestartEnable;
		};

		const VkViewport viewport =
		{
			0.0f,						// float	x;
			0.0f,						// float	y;
			(float)m_renderSize.x(),	// float	width;
			(float)m_renderSize.y(),	// float	height;
			0.0f,						// float	minDepth;
			1.0f						// float	maxDepth;
		};

		const VkRect2D scissor =
		{
			{ 0, 0 },													// VkOffset2D  offset;
			{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y() }	// VkExtent2D  extent;
		};

		const VkPipelineViewportStateCreateInfo viewportStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,														// const void*							pNext;
			0u,																// VkPipelineViewportStateCreateFlags	flags;
			1u,																// deUint32								viewportCount;
			&viewport,														// const VkViewport*					pViewports;
			1u,																// deUint32								scissorCount;
			&scissor														// const VkRect2D*						pScissors;
		};

		const VkPipelineRasterizationStateCreateInfo rasterStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineRasterizationStateCreateFlags	flags;
			false,															// VkBool32									depthClampEnable;
			false,															// VkBool32									rasterizerDiscardEnable;
			VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
			VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
			VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
			VK_FALSE,														// VkBool32									depthBiasEnable;
			0.0f,															// float									depthBiasConstantFactor;
			0.0f,															// float									depthBiasClamp;
			0.0f,															// float									depthBiasSlopeFactor;
			1.0f															// float									lineWidth;
		};

		const VkPipelineColorBlendStateCreateInfo colorBlendStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
			DE_NULL,													// const void*									pNext;
			0u,															// VkPipelineColorBlendStateCreateFlags			flags;
			false,														// VkBool32										logicOpEnable;
			VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
			1u,															// deUint32										attachmentCount;
			&m_colorBlendState,											// const VkPipelineColorBlendAttachmentState*	pAttachments;
			{ 0.0f, 0.0f, 0.0f, 0.0f }									// float										blendConstants[4];
		};

		const VkStencilOpState stencilOpState =
		{
			VK_STENCIL_OP_KEEP,						// VkStencilOp	failOp;
			VK_STENCIL_OP_REPLACE,					// VkStencilOp	passOp;
			VK_STENCIL_OP_KEEP,						// VkStencilOp	depthFailOp;
			VK_COMPARE_OP_GREATER,					// VkCompareOp	compareOp;
			1u,										// deUint32		compareMask;
			1u,										// deUint32		writeMask;
			1u,										// deUint32		reference;
		};

		const VkPipelineDepthStencilStateCreateInfo depthStencilStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineDepthStencilStateCreateFlags	flags;
			m_useDepth,													// VkBool32									depthTestEnable;
			m_useDepth,													// VkBool32									depthWriteEnable;
			VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
			false,														// VkBool32									depthBoundsTestEnable;
			m_useStencil,												// VkBool32									stencilTestEnable;
			stencilOpState,												// VkStencilOpState	front;
			stencilOpState,												// VkStencilOpState	back;
			0.0f,														// float			minDepthBounds;
			1.0f,														// float			maxDepthBounds;
		};

		const VkGraphicsPipelineCreateInfo graphicsPipelineParams =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
			DE_NULL,											// const void*										pNext;
			0u,													// VkPipelineCreateFlags							flags;
			2u,													// deUint32											stageCount;
			shaderStageParams,									// const VkPipelineShaderStageCreateInfo*			pStages;
			&vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
			&inputAssemblyStateParams,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
			DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
			&viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
			&rasterStateParams,									// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
			&m_multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
			&depthStencilStateParams,							// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
			&colorBlendStateParams,								// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
			(const VkPipelineDynamicStateCreateInfo*)DE_NULL,	// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
			*m_pipelineLayout,									// VkPipelineLayout									layout;
			*m_renderPass,										// VkRenderPass										renderPass;
			0u,													// deUint32											subpass;
			0u,													// VkPipeline										basePipelineHandle;
			0u													// deInt32											basePipelineIndex;
		};

		for (deUint32 i = 0u; i < numTopologies; ++i)
		{
			inputAssemblyStateParams.topology = pTopology[i];
			m_graphicsPipelines.push_back(VkPipelineSp(new Unique<VkPipeline>(createGraphicsPipeline(vk, vkDevice, DE_NULL, &graphicsPipelineParams))));
		}
	}

	if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
	{
		// Create pipelines for copying samples to single sampled images
		{
			const VkPipelineShaderStageCreateInfo shaderStageParams[2] =
			{
				{
					VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType							sType;
					DE_NULL,													// const void*								pNext;
					0u,															// VkPipelineShaderStageCreateFlags			flags;
					VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits					stage;
					*m_copySampleVertexShaderModule,							// VkShaderModule							module;
					"main",														// const char*								pName;
					DE_NULL														// const VkSpecializationInfo*				pSpecializationInfo;
				},
				{
					VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType							sType;
					DE_NULL,													// const void*								pNext;
					0u,															// VkPipelineShaderStageCreateFlags			flags;
					VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits					stage;
					*m_copySampleFragmentShaderModule,							// VkShaderModule							module;
					"main",														// const char*								pName;
					DE_NULL														// const VkSpecializationInfo*				pSpecializationInfo;
				}
			};

			const VkPipelineVertexInputStateCreateInfo vertexInputStateParams =
			{
				VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType							sType;
				DE_NULL,														// const void*								pNext;
				0u,																// VkPipelineVertexInputStateCreateFlags	flags;
				0u,																// deUint32									vertexBindingDescriptionCount;
				DE_NULL,														// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
				0u,																// deUint32									vertexAttributeDescriptionCount;
				DE_NULL															// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
			};

			// Topology is set before the pipeline creation.
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateParams =
			{
				VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
				DE_NULL,														// const void*								pNext;
				0u,																// VkPipelineInputAssemblyStateCreateFlags	flags;
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,							// VkPrimitiveTopology						topology;
				false															// VkBool32									primitiveRestartEnable;
			};

			const VkViewport viewport =
			{
				0.0f,						// float	x;
				0.0f,						// float	y;
				(float)m_renderSize.x(),	// float	width;
				(float)m_renderSize.y(),	// float	height;
				0.0f,						// float	minDepth;
				1.0f						// float	maxDepth;
			};

			const VkRect2D scissor =
			{
				{ 0, 0 },													// VkOffset2D  offset;
				{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y() }	// VkExtent2D  extent;
			};

			const VkPipelineViewportStateCreateInfo viewportStateParams =
			{
				VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType							sType;
				DE_NULL,														// const void*								pNext;
				0u,																// VkPipelineViewportStateCreateFlags		flags;
				1u,																// deUint32									viewportCount;
				&viewport,														// const VkViewport*						pViewports;
				1u,																// deUint32									scissorCount;
				&scissor														// const VkRect2D*							pScissors;
			};

			const VkPipelineRasterizationStateCreateInfo rasterStateParams =
			{
				VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
				DE_NULL,														// const void*								pNext;
				0u,																// VkPipelineRasterizationStateCreateFlags	flags;
				false,															// VkBool32									depthClampEnable;
				false,															// VkBool32									rasterizerDiscardEnable;
				VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
				VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
				VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
				VK_FALSE,														// VkBool32									depthBiasEnable;
				0.0f,															// float									depthBiasConstantFactor;
				0.0f,															// float									depthBiasClamp;
				0.0f,															// float									depthBiasSlopeFactor;
				1.0f															// float									lineWidth;
			};

			const VkPipelineColorBlendStateCreateInfo colorBlendStateParams =
			{
				VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
				DE_NULL,													// const void*									pNext;
				0u,															// VkPipelineColorBlendStateCreateFlags			flags;
				false,														// VkBool32										logicOpEnable;
				VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
				1u,															// deUint32										attachmentCount;
				&m_colorBlendState,											// const VkPipelineColorBlendAttachmentState*	pAttachments;
				{ 0.0f, 0.0f, 0.0f, 0.0f }									// float										blendConstants[4];
			};

			const  VkPipelineMultisampleStateCreateInfo multisampleStateParams =
			{
				VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType
				DE_NULL,													// const void*								pNext
				0u,															// VkPipelineMultisampleStateCreateFlags	flags
				VK_SAMPLE_COUNT_1_BIT,										// VkSampleCountFlagBits					rasterizationSamples
				VK_FALSE,													// VkBool32									sampleShadingEnable
				0.0f,														// float									minSampleShading
				DE_NULL,													// const VkSampleMask*						pSampleMask
				VK_FALSE,													// VkBool32									alphaToCoverageEnable
				VK_FALSE,													// VkBool32									alphaToOneEnable
			};

			const VkGraphicsPipelineCreateInfo graphicsPipelineTemplate =
			{
				VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
				DE_NULL,											// const void*										pNext;
				0u,													// VkPipelineCreateFlags							flags;
				2u,													// deUint32											stageCount;
				shaderStageParams,									// const VkPipelineShaderStageCreateInfo*			pStages;
				&vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
				&inputAssemblyStateParams,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
				DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
				&viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
				&rasterStateParams,									// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
				&multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
				DE_NULL,											// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
				&colorBlendStateParams,								// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
				(const VkPipelineDynamicStateCreateInfo*)DE_NULL,	// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
				*m_copySamplePipelineLayout,						// VkPipelineLayout									layout;
				*m_renderPass,										// VkRenderPass										renderPass;
				0u,													// deUint32											subpass;
				0u,													// VkPipeline										basePipelineHandle;
				0u													// deInt32											basePipelineIndex;
			};

			for (size_t i = 0; i < m_perSampleImages.size(); ++i)
			{
				VkGraphicsPipelineCreateInfo graphicsPipelineParams = graphicsPipelineTemplate;

				// Pipeline is to be used in subpasses subsequent to sample-shading subpass
				graphicsPipelineParams.subpass = 1u + (deUint32)i;

				m_copySamplePipelines.push_back(VkPipelineSp(new Unique<VkPipeline>(createGraphicsPipeline(vk, vkDevice, DE_NULL, &graphicsPipelineParams))));
			}
		}


		const VkDescriptorPoolSize			descriptorPoolSize			=
		{
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,					// VkDescriptorType					type;
			1u														// deUint32							descriptorCount;
		};

		const VkDescriptorPoolCreateInfo	descriptorPoolCreateInfo	=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,			// VkStructureType					sType
			DE_NULL,												// const void*						pNext
			VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,		// VkDescriptorPoolCreateFlags		flags
			1u,													// deUint32							maxSets
			1u,														// deUint32							poolSizeCount
			&descriptorPoolSize										// const VkDescriptorPoolSize*		pPoolSizes
		};

		m_copySampleDesciptorPool = createDescriptorPool(vk, vkDevice, &descriptorPoolCreateInfo);

		const VkDescriptorSetAllocateInfo	descriptorSetAllocateInfo	=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,			// VkStructureType					sType
			DE_NULL,												// const void*						pNext
			*m_copySampleDesciptorPool,								// VkDescriptorPool					descriptorPool
			1u,														// deUint32							descriptorSetCount
			&m_copySampleDesciptorLayout.get(),						// const VkDescriptorSetLayout*		pSetLayouts
		};

		m_copySampleDesciptorSet = allocateDescriptorSet(vk, vkDevice, &descriptorSetAllocateInfo);

		const VkDescriptorImageInfo			imageInfo					=
		{
			DE_NULL,
			*m_colorAttachmentView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		const VkWriteDescriptorSet			descriptorWrite				=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,			// VkStructureType					sType;
			DE_NULL,										// const void*						pNext;
			*m_copySampleDesciptorSet,						// VkDescriptorSet					dstSet;
			0u,												// deUint32							dstBinding;
			0u,												// deUint32							dstArrayElement;
			1u,												// deUint32							descriptorCount;
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,			// VkDescriptorType					descriptorType;
			&imageInfo,										// const VkDescriptorImageInfo*		pImageInfo;
			DE_NULL,										// const VkDescriptorBufferInfo*	pBufferInfo;
			DE_NULL,										// const VkBufferView*				pTexelBufferView;
		};
		vk.updateDescriptorSets(vkDevice, 1u, &descriptorWrite, 0u, DE_NULL);
	}

	// Create vertex buffer
	{
		const VkBufferCreateInfo vertexBufferParams =
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			1024u,										// VkDeviceSize			size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_vertexBuffer		= createBuffer(vk, vkDevice, &vertexBufferParams);
		m_vertexBufferAlloc	= memAlloc.allocate(getBufferMemoryRequirements(vk, vkDevice, *m_vertexBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(vkDevice, *m_vertexBuffer, m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset()));

		// Load vertices into vertex buffer
		{
			Vertex4RGBA* pDst = static_cast<Vertex4RGBA*>(m_vertexBufferAlloc->getHostPtr());
			for (deUint32 i = 0u; i < numTopologies; ++i)
			{
				deMemcpy(pDst, &pVertices[i][0], pVertices[i].size() * sizeof(Vertex4RGBA));
				pDst += pVertices[i].size();
			}
		}
		flushMappedMemoryRange(vk, vkDevice, m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset(), vertexBufferParams.size);
	}

	// Create command pool
	m_cmdPool = createCommandPool(vk, vkDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndex);

	// Create command buffer
	{
		const VkCommandBufferBeginInfo cmdBufferBeginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType					sType;
			DE_NULL,										// const void*						pNext;
			0u,												// VkCommandBufferUsageFlags		flags;
			(const VkCommandBufferInheritanceInfo*)DE_NULL,
		};

		VkClearValue colorClearValue;
		colorClearValue.color.float32[0] = 0.0f;
		colorClearValue.color.float32[1] = 0.0f;
		colorClearValue.color.float32[2] = 0.0f;
		colorClearValue.color.float32[3] = 0.0f;

		VkClearValue depthStencilClearValue;
		depthStencilClearValue.depthStencil.depth = 1.0f;
		depthStencilClearValue.depthStencil.stencil = 0u;

		std::vector<VkClearValue> clearValues;
		clearValues.push_back(colorClearValue);
		if (m_renderType == RENDER_TYPE_RESOLVE)
		{
			clearValues.push_back(colorClearValue);
		}
		if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
		{
			for (size_t i = 0; i < m_perSampleImages.size(); ++i)
			{
				clearValues.push_back(colorClearValue);
			}
		}
		if (m_useDepth || m_useStencil)
		{
			clearValues.push_back(depthStencilClearValue);
		}

		const VkRenderPassBeginInfo renderPassBeginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,				// VkStructureType		sType;
			DE_NULL,												// const void*			pNext;
			*m_renderPass,											// VkRenderPass			renderPass;
			*m_framebuffer,											// VkFramebuffer		framebuffer;
			{
				{ 0, 0 },
				{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y() }
			},														// VkRect2D				renderArea;
			(deUint32)clearValues.size(),							// deUint32				clearValueCount;
			&clearValues[0]											// const VkClearValue*	pClearValues;
		};

		std::vector<VkImageMemoryBarrier> imageLayoutBarriers;

		{
			const VkImageMemoryBarrier colorImageBarrier =
				// color attachment image
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
				DE_NULL,										// const void*				pNext;
				0u,												// VkAccessFlags			srcAccessMask;
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags			dstAccessMask;
				VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout			newLayout;
				VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,						// deUint32					dstQueueFamilyIndex;
				*m_colorImage,									// VkImage					image;
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },	// VkImageSubresourceRange	subresourceRange;
			};
			imageLayoutBarriers.push_back(colorImageBarrier);
		}
		if (m_renderType == RENDER_TYPE_RESOLVE)
		{
			const VkImageMemoryBarrier resolveImageBarrier =
			// resolve attachment image
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
				DE_NULL,										// const void*				pNext;
				0u,												// VkAccessFlags			srcAccessMask;
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags			dstAccessMask;
				VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout			newLayout;
				VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,						// deUint32					dstQueueFamilyIndex;
				*m_resolveImage,								// VkImage					image;
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },	// VkImageSubresourceRange	subresourceRange;
			};
			imageLayoutBarriers.push_back(resolveImageBarrier);
		}
		if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
		{
			for (size_t i = 0; i < m_perSampleImages.size(); ++i)
			{
				const VkImageMemoryBarrier perSampleImageBarrier =
				// resolve attachment image
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
					DE_NULL,										// const void*				pNext;
					0u,												// VkAccessFlags			srcAccessMask;
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags			dstAccessMask;
					VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,						// deUint32					dstQueueFamilyIndex;
					*m_perSampleImages[i]->m_image,					// VkImage					image;
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },	// VkImageSubresourceRange	subresourceRange;
				};
				imageLayoutBarriers.push_back(perSampleImageBarrier);
			}
		}
		if (m_useDepth || m_useStencil)
		{
			const VkImageMemoryBarrier depthStencilImageBarrier =
			// depth/stencil attachment image
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
				DE_NULL,											// const void*				pNext;
				0u,													// VkAccessFlags			srcAccessMask;
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,		// VkAccessFlags			dstAccessMask;
				VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			oldLayout;
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	// VkImageLayout			newLayout;
				VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,							// deUint32					dstQueueFamilyIndex;
				*m_depthStencilImage,								// VkImage					image;
				{ depthStencilAttachmentAspect, 0u, 1u, 0u, 1u },	// VkImageSubresourceRange	subresourceRange;
			};
			imageLayoutBarriers.push_back(depthStencilImageBarrier);
		};

		m_cmdBuffer = allocateCommandBuffer(vk, vkDevice, *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

		vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0,
			0u, DE_NULL, 0u, DE_NULL, (deUint32)imageLayoutBarriers.size(), &imageLayoutBarriers[0]);

		vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkDeviceSize vertexBufferOffset = 0u;

		for (deUint32 i = 0u; i < numTopologies; ++i)
		{
			vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **m_graphicsPipelines[i]);
			vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &m_vertexBuffer.get(), &vertexBufferOffset);
			vk.cmdDraw(*m_cmdBuffer, (deUint32)pVertices[i].size(), 1, 0, 0);

			vertexBufferOffset += static_cast<VkDeviceSize>(pVertices[i].size() * sizeof(Vertex4RGBA));
		}

		if (m_renderType == RENDER_TYPE_COPY_SAMPLES)
		{
			// Copy each sample id to single sampled image
			for (deInt32 sampleId = 0; sampleId < (deInt32)m_perSampleImages.size(); ++sampleId)
			{
				vk.cmdNextSubpass(*m_cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
				vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **m_copySamplePipelines[sampleId]);
				vk.cmdBindDescriptorSets(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_copySamplePipelineLayout, 0u, 1u, &m_copySampleDesciptorSet.get(), 0u, DE_NULL);
				vk.cmdPushConstants(*m_cmdBuffer, *m_copySamplePipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(deInt32), &sampleId);
				vk.cmdDraw(*m_cmdBuffer, 4, 1, 0, 0);
			}
		}

		vk.cmdEndRenderPass(*m_cmdBuffer);

		VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
	}

	// Create fence
	m_fence = createFence(vk, vkDevice);
}

MultisampleRenderer::~MultisampleRenderer (void)
{
}

de::MovePtr<tcu::TextureLevel> MultisampleRenderer::render (void)
{
	const DeviceInterface&		vk					= m_context.getDeviceInterface();
	const VkDevice				vkDevice			= m_context.getDevice();
	const VkQueue				queue				= m_context.getUniversalQueue();
	const deUint32				queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	SimpleAllocator				allocator			(vk, vkDevice, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));
	const VkSubmitInfo			submitInfo	=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
		DE_NULL,						// const void*				pNext;
		0u,								// deUint32					waitSemaphoreCount;
		DE_NULL,						// const VkSemaphore*		pWaitSemaphores;
		(const VkPipelineStageFlags*)DE_NULL,
		1u,								// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),				// const VkCommandBuffer*	pCommandBuffers;
		0u,								// deUint32					signalSemaphoreCount;
		DE_NULL							// const VkSemaphore*		pSignalSemaphores;
	};

	VK_CHECK(vk.resetFences(vkDevice, 1, &m_fence.get()));
	VK_CHECK(vk.queueSubmit(queue, 1, &submitInfo, *m_fence));
	VK_CHECK(vk.waitForFences(vkDevice, 1, &m_fence.get(), true, ~(0ull) /* infinity*/));

	if (m_renderType == RENDER_TYPE_RESOLVE)
	{
		return readColorAttachment(vk, vkDevice, queue, queueFamilyIndex, allocator, *m_resolveImage, m_colorFormat, m_renderSize.cast<deUint32>());
	}
	else
	{
		return de::MovePtr<tcu::TextureLevel>();
	}
}

de::MovePtr<tcu::TextureLevel> MultisampleRenderer::getSingleSampledImage (deUint32 sampleId)
{
	const DeviceInterface&		vk					= m_context.getDeviceInterface();
	const VkDevice				vkDevice			= m_context.getDevice();
	const VkQueue				queue				= m_context.getUniversalQueue();
	const deUint32				queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	SimpleAllocator				allocator			(vk, vkDevice, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));

	return readColorAttachment(vk, vkDevice, queue, queueFamilyIndex, allocator, *m_perSampleImages[sampleId]->m_image, m_colorFormat, m_renderSize.cast<deUint32>());
}

} // anonymous

tcu::TestCaseGroup* createMultisampleTests (tcu::TestContext& testCtx)
{
	const VkSampleCountFlagBits samples[] =
	{
		VK_SAMPLE_COUNT_2_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_64_BIT
	};

	de::MovePtr<tcu::TestCaseGroup> multisampleTests (new tcu::TestCaseGroup(testCtx, "multisample", ""));

	// Rasterization samples tests
	{
		de::MovePtr<tcu::TestCaseGroup> rasterizationSamplesTests(new tcu::TestCaseGroup(testCtx, "raster_samples", ""));

		for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(samples); samplesNdx++)
		{
			std::ostringstream caseName;
			caseName << "samples_" << samples[samplesNdx];

			de::MovePtr<tcu::TestCaseGroup> samplesTests	(new tcu::TestCaseGroup(testCtx, caseName.str().c_str(), ""));

			samplesTests->addChild(new RasterizationSamplesTest(testCtx, "primitive_triangle", "",	samples[samplesNdx], GEOMETRY_TYPE_OPAQUE_TRIANGLE));
			samplesTests->addChild(new RasterizationSamplesTest(testCtx, "primitive_line", "",		samples[samplesNdx], GEOMETRY_TYPE_OPAQUE_LINE));
			samplesTests->addChild(new RasterizationSamplesTest(testCtx, "primitive_point", "",		samples[samplesNdx], GEOMETRY_TYPE_OPAQUE_POINT));

			samplesTests->addChild(new RasterizationSamplesTest(testCtx, "depth", "",			samples[samplesNdx], GEOMETRY_TYPE_INVISIBLE_TRIANGLE, TEST_MODE_DEPTH_BIT));
			samplesTests->addChild(new RasterizationSamplesTest(testCtx, "stencil", "",			samples[samplesNdx], GEOMETRY_TYPE_INVISIBLE_TRIANGLE, TEST_MODE_STENCIL_BIT));
			samplesTests->addChild(new RasterizationSamplesTest(testCtx, "depth_stencil", "",	samples[samplesNdx], GEOMETRY_TYPE_INVISIBLE_TRIANGLE, TEST_MODE_DEPTH_BIT | TEST_MODE_STENCIL_BIT));

			rasterizationSamplesTests->addChild(samplesTests.release());
		}

		multisampleTests->addChild(rasterizationSamplesTests.release());
	}

	// Raster samples consistency check
	{
		de::MovePtr<tcu::TestCaseGroup> rasterSamplesConsistencyTests(new tcu::TestCaseGroup(testCtx, "raster_samples_consistency", ""));

		addFunctionCaseWithPrograms(rasterSamplesConsistencyTests.get(),
									"unique_colors_check",
									"",
									initMultisamplePrograms,
									testRasterSamplesConsistency,
									GEOMETRY_TYPE_OPAQUE_TRIANGLE);

		multisampleTests->addChild(rasterSamplesConsistencyTests.release());
	}

	// minSampleShading tests
	{
		struct TestConfig
		{
			const char*	name;
			float		minSampleShading;
		};

		const TestConfig testConfigs[] =
		{
			{ "min_0_0",	0.0f },
			{ "min_0_25",	0.25f },
			{ "min_0_5",	0.5f },
			{ "min_0_75",	0.75f },
			{ "min_1_0",	1.0f }
		};

		de::MovePtr<tcu::TestCaseGroup> minSampleShadingTests(new tcu::TestCaseGroup(testCtx, "min_sample_shading", ""));

		for (int configNdx = 0; configNdx < DE_LENGTH_OF_ARRAY(testConfigs); configNdx++)
		{
			const TestConfig&				testConfig				= testConfigs[configNdx];
			de::MovePtr<tcu::TestCaseGroup>	minShadingValueTests	(new tcu::TestCaseGroup(testCtx, testConfigs[configNdx].name, ""));

			for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(samples); samplesNdx++)
			{
				std::ostringstream caseName;
				caseName << "samples_" << samples[samplesNdx];

				de::MovePtr<tcu::TestCaseGroup> samplesTests	(new tcu::TestCaseGroup(testCtx, caseName.str().c_str(), ""));

				samplesTests->addChild(new MinSampleShadingTest(testCtx, "primitive_triangle", "", samples[samplesNdx], testConfig.minSampleShading, GEOMETRY_TYPE_OPAQUE_TRIANGLE));
				samplesTests->addChild(new MinSampleShadingTest(testCtx, "primitive_line", "", samples[samplesNdx], testConfig.minSampleShading, GEOMETRY_TYPE_OPAQUE_LINE));
				samplesTests->addChild(new MinSampleShadingTest(testCtx, "primitive_point", "", samples[samplesNdx], testConfig.minSampleShading, GEOMETRY_TYPE_OPAQUE_POINT));

				minShadingValueTests->addChild(samplesTests.release());
			}

			minSampleShadingTests->addChild(minShadingValueTests.release());
		}

		multisampleTests->addChild(minSampleShadingTests.release());
	}

	// pSampleMask tests
	{
		struct TestConfig
		{
			const char*		name;
			const char*		description;
			VkSampleMask	sampleMask;
		};

		const TestConfig testConfigs[] =
		{
			{ "mask_all_on",	"All mask bits are off",			0x0 },
			{ "mask_all_off",	"All mask bits are on",				0xFFFFFFFF },
			{ "mask_one",		"All mask elements are 0x1",		0x1},
			{ "mask_random",	"All mask elements are 0xAAAAAAAA",	0xAAAAAAAA },
		};

		de::MovePtr<tcu::TestCaseGroup> sampleMaskTests(new tcu::TestCaseGroup(testCtx, "sample_mask", ""));

		for (int configNdx = 0; configNdx < DE_LENGTH_OF_ARRAY(testConfigs); configNdx++)
		{
			const TestConfig&				testConfig				= testConfigs[configNdx];
			de::MovePtr<tcu::TestCaseGroup>	sampleMaskValueTests	(new tcu::TestCaseGroup(testCtx, testConfig.name, testConfig.description));

			for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(samples); samplesNdx++)
			{
				std::ostringstream caseName;
				caseName << "samples_" << samples[samplesNdx];

				const deUint32					sampleMaskCount	= samples[samplesNdx] / 32;
				de::MovePtr<tcu::TestCaseGroup> samplesTests	(new tcu::TestCaseGroup(testCtx, caseName.str().c_str(), ""));

				std::vector<VkSampleMask> mask;
				for (deUint32 maskNdx = 0; maskNdx < sampleMaskCount; maskNdx++)
					mask.push_back(testConfig.sampleMask);

				samplesTests->addChild(new SampleMaskTest(testCtx, "primitive_triangle", "", samples[samplesNdx], mask, GEOMETRY_TYPE_OPAQUE_TRIANGLE));
				samplesTests->addChild(new SampleMaskTest(testCtx, "primitive_line", "", samples[samplesNdx], mask, GEOMETRY_TYPE_OPAQUE_LINE));
				samplesTests->addChild(new SampleMaskTest(testCtx, "primitive_point", "", samples[samplesNdx], mask, GEOMETRY_TYPE_OPAQUE_POINT));

				sampleMaskValueTests->addChild(samplesTests.release());
			}

			sampleMaskTests->addChild(sampleMaskValueTests.release());
		}

		multisampleTests->addChild(sampleMaskTests.release());

	}

	// AlphaToOne tests
	{
		de::MovePtr<tcu::TestCaseGroup> alphaToOneTests(new tcu::TestCaseGroup(testCtx, "alpha_to_one", ""));

		for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(samples); samplesNdx++)
		{
			std::ostringstream caseName;
			caseName << "samples_" << samples[samplesNdx];

			alphaToOneTests->addChild(new AlphaToOneTest(testCtx, caseName.str(), "", samples[samplesNdx]));
		}

		multisampleTests->addChild(alphaToOneTests.release());
	}

	// AlphaToCoverageEnable tests
	{
		de::MovePtr<tcu::TestCaseGroup> alphaToCoverageTests (new tcu::TestCaseGroup(testCtx, "alpha_to_coverage", ""));

		for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(samples); samplesNdx++)
		{
			std::ostringstream caseName;
			caseName << "samples_" << samples[samplesNdx];

			de::MovePtr<tcu::TestCaseGroup> samplesTests	(new tcu::TestCaseGroup(testCtx, caseName.str().c_str(), ""));

			samplesTests->addChild(new AlphaToCoverageTest(testCtx, "alpha_opaque", "", samples[samplesNdx], GEOMETRY_TYPE_OPAQUE_QUAD));
			samplesTests->addChild(new AlphaToCoverageTest(testCtx, "alpha_translucent", "", samples[samplesNdx], GEOMETRY_TYPE_TRANSLUCENT_QUAD));
			samplesTests->addChild(new AlphaToCoverageTest(testCtx, "alpha_invisible", "", samples[samplesNdx], GEOMETRY_TYPE_INVISIBLE_QUAD));

			alphaToCoverageTests->addChild(samplesTests.release());
		}
		multisampleTests->addChild(alphaToCoverageTests.release());
	}

	// Sampling from a multisampled image texture (texelFetch)
	{
		multisampleTests->addChild(createMultisampleSampledImageTests(testCtx));
	}

	// Load/store on a multisampled rendered image (different kinds of access: color attachment write, storage image, etc.)
	{
		multisampleTests->addChild(createMultisampleStorageImageTests(testCtx));
	}

	return multisampleTests.release();
}

} // pipeline
} // vkt
