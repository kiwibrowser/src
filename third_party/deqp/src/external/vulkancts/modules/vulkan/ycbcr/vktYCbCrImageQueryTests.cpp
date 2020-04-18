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
 * \brief OpImageQuery & YCbCr Tests
 *//*--------------------------------------------------------------------*/

#include "vktYCbCrImageQueryTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktShaderExecutor.hpp"
#include "vktYCbCrUtil.hpp"
#include "vktDrawUtil.hpp"

#include "vkStrUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkImageUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTexLookupVerifier.hpp"

#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "deUniquePtr.hpp"
#include "deRandom.hpp"
#include "deSTLUtil.hpp"

namespace vkt
{
namespace ycbcr
{
namespace
{

using namespace vk;
using namespace shaderexecutor;

using tcu::UVec2;
using tcu::Vec2;
using tcu::Vec4;
using tcu::TestLog;
using de::MovePtr;
using de::UniquePtr;
using std::vector;
using std::string;

typedef de::SharedPtr<Allocation>				AllocationSp;
typedef de::SharedPtr<vk::Unique<VkBuffer> >	VkBufferSp;

enum QueryType
{
	QUERY_TYPE_IMAGE_SIZE_LOD,			// OpImageQuerySizeLod
	QUERY_TYPE_IMAGE_LOD,				// OpImageQueryLod
	QUERY_TYPE_IMAGE_LEVELS,			// OpImageQueryLevels

	QUERY_TYPE_LAST
};

struct TestParameters
{
	QueryType			query;
	VkFormat			format;
	VkImageCreateFlags	flags;
	glu::ShaderType		shaderType;

	TestParameters (QueryType query_, VkFormat format_, VkImageCreateFlags flags_, glu::ShaderType shaderType_)
		: query		(query_)
		, format	(format_)
		, flags		(flags_)
		, shaderType(shaderType_)
	{
	}

	TestParameters (void)
		: query		(QUERY_TYPE_LAST)
		, format	(VK_FORMAT_UNDEFINED)
		, flags		(0u)
		, shaderType(glu::SHADERTYPE_LAST)
	{
	}
};

ShaderSpec getShaderSpec (const TestParameters& params)
{
	ShaderSpec		spec;
	const char*		expr		= DE_NULL;
	glu::DataType	resultType	= glu::TYPE_LAST;

	switch (params.query)
	{
		case QUERY_TYPE_IMAGE_SIZE_LOD:
			expr		= "textureSize(u_image, lod)";
			resultType	= glu::TYPE_INT_VEC2;
			break;

		case QUERY_TYPE_IMAGE_LEVELS:
			expr		= "textureQueryLevels(u_image)";
			resultType	= glu::TYPE_INT;
			break;

		default:
			DE_FATAL("Unknown query");
	}

	spec.glslVersion = glu::GLSL_VERSION_450;

	spec.inputs.push_back(Symbol("lod", glu::VarType(glu::TYPE_INT, glu::PRECISION_HIGHP)));
	spec.outputs.push_back(Symbol("result", glu::VarType(resultType, glu::PRECISION_HIGHP)));

	spec.globalDeclarations =
		"layout(binding = 0, set = 1) uniform highp sampler2D u_image;\n";

	spec.source =
		string("result = ") + expr + ";\n";

	return spec;
}

Move<VkImage> createTestImage (const DeviceInterface&	vkd,
							   VkDevice					device,
							   VkFormat					format,
							   const UVec2&				size,
							   VkImageCreateFlags		createFlags)
{
	const VkImageCreateInfo		createInfo	=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		DE_NULL,
		createFlags,
		VK_IMAGE_TYPE_2D,
		format,
		makeExtent3D(size.x(), size.y(), 1u),
		1u,		// mipLevels
		1u,		// arrayLayers
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0u,
		(const deUint32*)DE_NULL,
		VK_IMAGE_LAYOUT_UNDEFINED,
	};

	return createImage(vkd, device, &createInfo);
}

Move<VkImageView> createImageView (const DeviceInterface&		vkd,
								   VkDevice						device,
								   VkImage						image,
								   VkFormat						format,
								   VkSamplerYcbcrConversionKHR	conversion)
{
	const VkSamplerYcbcrConversionInfoKHR				samplerConversionInfo	=
	{
		VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO_KHR,
		DE_NULL,
		conversion
	};

	const VkImageViewCreateInfo	viewInfo	=
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		(conversion != DE_NULL) ? &samplerConversionInfo : DE_NULL,
		(VkImageViewCreateFlags)0,
		image,
		VK_IMAGE_VIEW_TYPE_2D,
		format,
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },
	};

	return createImageView(vkd, device, &viewInfo);
}

class TestImage
{
public:
								TestImage		(const DeviceInterface&			vkd,
												 VkDevice						device,
												 Allocator&						allocator,
												 VkFormat						format,
												 const UVec2&					size,
												 const VkImageCreateFlags		createFlags,
												 VkSamplerYcbcrConversionKHR	conversion);

	const UVec2&				getSize			(void) const { return m_size;		}
	VkImageView					getImageView	(void) const { return *m_imageView; }

private:
	const UVec2					m_size;
	const Unique<VkImage>		m_image;
	const vector<AllocationSp>	m_allocations;
	const Unique<VkImageView>	m_imageView;
};

TestImage::TestImage (const DeviceInterface&		vkd,
					  VkDevice						device,
					  Allocator&					allocator,
					  VkFormat						format,
					  const UVec2&					size,
					  const VkImageCreateFlags		createFlags,
					  VkSamplerYcbcrConversionKHR	conversion)
	: m_size		(size)
	, m_image		(createTestImage(vkd, device, format, size, createFlags))
	, m_allocations	(allocateAndBindImageMemory(vkd, device, allocator, *m_image, format, createFlags))
	, m_imageView	(createImageView(vkd, device, *m_image, format, conversion))
{
}

typedef de::SharedPtr<TestImage> TestImageSp;

Move<VkDescriptorSetLayout> createDescriptorSetLayout (const DeviceInterface& vkd, VkDevice device, VkSampler sampler)
{
	const VkDescriptorSetLayoutBinding		binding		=
	{
		0u,												// binding
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1u,												// descriptorCount
		VK_SHADER_STAGE_ALL,
		&sampler
	};
	const VkDescriptorSetLayoutCreateInfo	layoutInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		DE_NULL,
		(VkDescriptorSetLayoutCreateFlags)0u,
		1u,
		&binding,
	};

	return createDescriptorSetLayout(vkd, device, &layoutInfo);
}

Move<VkDescriptorPool> createDescriptorPool (const DeviceInterface& vkd, VkDevice device)
{
	const VkDescriptorPoolSize			poolSizes[]	=
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	1u	},
	};
	const VkDescriptorPoolCreateInfo	poolInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		DE_NULL,
		(VkDescriptorPoolCreateFlags)VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		1u,		// maxSets
		DE_LENGTH_OF_ARRAY(poolSizes),
		poolSizes,
	};

	return createDescriptorPool(vkd, device, & poolInfo);
}

Move<VkDescriptorSet> createDescriptorSet (const DeviceInterface&	vkd,
										   VkDevice					device,
										   VkDescriptorPool			descPool,
										   VkDescriptorSetLayout	descLayout)
{
	const VkDescriptorSetAllocateInfo	allocInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		DE_NULL,
		descPool,
		1u,
		&descLayout,
	};

	return allocateDescriptorSet(vkd, device, &allocInfo);
}

void bindImage (const DeviceInterface& vkd,
				VkDevice device,
				VkDescriptorSet descriptorSet,
				VkImageView imageView,
				VkSampler sampler)
{
	const VkDescriptorImageInfo		imageInfo			=
	{
		sampler,
		imageView,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	const VkWriteDescriptorSet		descriptorWrite		=
	{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		DE_NULL,
		descriptorSet,
		0u,		// dstBinding
		0u,		// dstArrayElement
		1u,		// descriptorCount
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		&imageInfo,
		(const VkDescriptorBufferInfo*)DE_NULL,
		(const VkBufferView*)DE_NULL,
	};

	vkd.updateDescriptorSets(device, 1u, &descriptorWrite, 0u, DE_NULL);
}

UVec2 getMaxPlaneDivisor (const PlanarFormatDescription& formatDesc)
{
	UVec2	maxDivisor	(1u, 1u);

	for (deUint32 ndx = 0; ndx < formatDesc.numPlanes; ++ndx)
	{
		maxDivisor.x() = de::max<deUint32>(maxDivisor.x(), formatDesc.planes[ndx].widthDivisor);
		maxDivisor.y() = de::max<deUint32>(maxDivisor.y(), formatDesc.planes[ndx].heightDivisor);
	}

	return maxDivisor;
}

tcu::TestStatus testImageQuery (Context& context, TestParameters params)
{
	const bool							isYCbCrImage	= isYCbCrFormat(params.format);

	if (isYCbCrImage)
		checkImageSupport(context, params.format, params.flags);

	const DeviceInterface&				vkd				= context.getDeviceInterface();
	const VkDevice						device			= context.getDevice();

	const VkSamplerYcbcrConversionCreateInfoKHR			conversionInfo			=
	{
		VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO_KHR,
		DE_NULL,
		params.format,
		VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR,
		VK_SAMPLER_YCBCR_RANGE_ITU_FULL_KHR,
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		VK_CHROMA_LOCATION_MIDPOINT_KHR,
		VK_CHROMA_LOCATION_MIDPOINT_KHR,
		VK_FILTER_NEAREST,
		VK_FALSE,									// forceExplicitReconstruction
	};
	const Unique<VkSamplerYcbcrConversionKHR>			conversion				(isYCbCrImage
																				 ? createSamplerYcbcrConversionKHR(vkd, device, &conversionInfo)
																				 : Move<VkSamplerYcbcrConversionKHR>());

	const VkSamplerYcbcrConversionInfoKHR				samplerConversionInfo	=
	{
		VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO_KHR,
		DE_NULL,
		*conversion,
	};

	const VkSamplerCreateInfo							samplerInfo				=
	{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		isYCbCrImage ? &samplerConversionInfo : DE_NULL,
		0u,
		VK_FILTER_NEAREST,							// magFilter
		VK_FILTER_NEAREST,							// minFilter
		VK_SAMPLER_MIPMAP_MODE_NEAREST,				// mipmapMode
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// addressModeU
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// addressModeV
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// addressModeW
		0.0f,										// mipLodBias
		VK_FALSE,									// anisotropyEnable
		1.0f,										// maxAnisotropy
		VK_FALSE,									// compareEnable
		VK_COMPARE_OP_ALWAYS,						// compareOp
		0.0f,										// minLod
		0.0f,										// maxLod
		VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,	// borderColor
		VK_FALSE,									// unnormalizedCoords
	};

	const Unique<VkSampler>				sampler		(createSampler(vkd, device, &samplerInfo));
	const Unique<VkDescriptorSetLayout>	descLayout	(createDescriptorSetLayout(vkd, device, *sampler));
	const Unique<VkDescriptorPool>		descPool	(createDescriptorPool(vkd, device));
	const Unique<VkDescriptorSet>		descSet		(createDescriptorSet(vkd, device, *descPool, *descLayout));

	vector<TestImageSp>					testImages;

	if (params.query == QUERY_TYPE_IMAGE_SIZE_LOD)
	{
		const PlanarFormatDescription&		formatDesc	= getPlanarFormatDescription(params.format);
		const UVec2							maxDivisor	= getMaxPlaneDivisor(formatDesc);
		vector<UVec2>						testSizes;

		testSizes.push_back(maxDivisor);
		testSizes.push_back(maxDivisor * UVec2(2u, 1u));
		testSizes.push_back(maxDivisor * UVec2(1u, 2u));
		testSizes.push_back(maxDivisor * UVec2(63u, 79u));
		testSizes.push_back(maxDivisor * UVec2(99u, 1u));
		testSizes.push_back(maxDivisor * UVec2(421u, 1117u));

		testImages.resize(testSizes.size());

		for (size_t ndx = 0; ndx < testSizes.size(); ++ndx)
			testImages[ndx] = TestImageSp(new TestImage(vkd, device, context.getDefaultAllocator(), params.format, testSizes[ndx], params.flags, *conversion));
	}
	else
		testImages.push_back(TestImageSp(new TestImage(vkd, device, context.getDefaultAllocator(), params.format, UVec2(16, 18), params.flags, *conversion)));

	{
		UniquePtr<ShaderExecutor>	executor	(createExecutor(context, params.shaderType, getShaderSpec(params), *descLayout));
		bool						allOk		= true;

		for (size_t imageNdx = 0; imageNdx < testImages.size(); ++imageNdx)
		{
			const deUint32	lod			= 0u;
			UVec2			result		(~0u, ~0u);
			const void*		inputs[]	= { &lod };
			void*			outputs[]	= { result.getPtr() };

			bindImage(vkd, device, *descSet, testImages[imageNdx]->getImageView(), *sampler);

			executor->execute(1, inputs, outputs, *descSet);

			switch (params.query)
			{
				case QUERY_TYPE_IMAGE_SIZE_LOD:
				{
					const UVec2	reference	= testImages[imageNdx]->getSize();

					if (result != reference)
					{
						context.getTestContext().getLog()
							<< TestLog::Message << "ERROR: Image " << imageNdx
												<< ": got " << result
												<< ", expected " << reference
							<< TestLog::EndMessage;
						allOk = false;
					}
					break;
				}

				case QUERY_TYPE_IMAGE_LEVELS:
				{
					if (result.x() != 1u)
					{
						context.getTestContext().getLog()
							<< TestLog::Message << "ERROR: Image " << imageNdx
												<< ": got " << result.x()
												<< ", expected " << 1
							<< TestLog::EndMessage;
						allOk = false;
					}
					break;
				}

				default:
					DE_FATAL("Invalid query type");
			}
		}

		if (allOk)
			return tcu::TestStatus::pass("Queries passed");
		else
			return tcu::TestStatus::fail("Got invalid results");
	}
}

tcu::TestStatus testImageQueryLod (Context& context, TestParameters params)
{
	const bool							isYCbCrImage	= isYCbCrFormat(params.format);

	if (isYCbCrImage)
		checkImageSupport(context, params.format, params.flags);

	const DeviceInterface&				vkd				= context.getDeviceInterface();
	const VkDevice						device			= context.getDevice();

	const VkSamplerYcbcrConversionCreateInfoKHR			conversionInfo			=
	{
		VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO_KHR,
		DE_NULL,
		params.format,
		VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY_KHR,
		VK_SAMPLER_YCBCR_RANGE_ITU_FULL_KHR,
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		VK_CHROMA_LOCATION_MIDPOINT_KHR,
		VK_CHROMA_LOCATION_MIDPOINT_KHR,
		VK_FILTER_NEAREST,
		VK_FALSE,									// forceExplicitReconstruction
	};
	const Unique<VkSamplerYcbcrConversionKHR>			conversion				(isYCbCrImage
																				 ? createSamplerYcbcrConversionKHR(vkd, device, &conversionInfo)
																				 : Move<VkSamplerYcbcrConversionKHR>());

	const VkSamplerYcbcrConversionInfoKHR				samplerConversionInfo	=
	{
		VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO_KHR,
		DE_NULL,
		*conversion,
	};

	const VkSamplerCreateInfo							samplerInfo				=
	{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		isYCbCrImage ? &samplerConversionInfo : DE_NULL,
		0u,
		VK_FILTER_NEAREST,							// magFilter
		VK_FILTER_NEAREST,							// minFilter
		VK_SAMPLER_MIPMAP_MODE_NEAREST,				// mipmapMode
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// addressModeU
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// addressModeV
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// addressModeW
		0.0f,										// mipLodBias
		VK_FALSE,									// anisotropyEnable
		1.0f,										// maxAnisotropy
		VK_FALSE,									// compareEnable
		VK_COMPARE_OP_ALWAYS,						// compareOp
		0.0f,										// minLod
		0.0f,										// maxLod
		VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,	// borderColor
		VK_FALSE,									// unnormalizedCoords
	};

	const Unique<VkSampler>				sampler		(createSampler(vkd, device, &samplerInfo));
	const Unique<VkDescriptorSetLayout>	descLayout	(createDescriptorSetLayout(vkd, device, *sampler));
	const Unique<VkDescriptorPool>		descPool	(createDescriptorPool(vkd, device));
	const Unique<VkDescriptorSet>		descSet		(createDescriptorSet(vkd, device, *descPool, *descLayout));

	vector<TestImageSp>					testImages;

	DE_ASSERT(params.query == QUERY_TYPE_IMAGE_LOD);
	DE_ASSERT(params.shaderType == glu::SHADERTYPE_FRAGMENT);

	{
		const PlanarFormatDescription&		formatDesc	= getPlanarFormatDescription(params.format);
		const UVec2							maxDivisor	= getMaxPlaneDivisor(formatDesc);
		vector<UVec2>						testSizes;

		testSizes.push_back(maxDivisor);
		testSizes.push_back(maxDivisor * UVec2(2u, 1u));
		testSizes.push_back(maxDivisor * UVec2(1u, 2u));
		testSizes.push_back(maxDivisor * UVec2(4u, 123u));
		testSizes.push_back(maxDivisor * UVec2(312u, 13u));
		testSizes.push_back(maxDivisor * UVec2(841u, 917u));

		testImages.resize(testSizes.size());

		for (size_t ndx = 0; ndx < testSizes.size(); ++ndx)
			testImages[ndx] = TestImageSp(new TestImage(vkd, device, context.getDefaultAllocator(), params.format, testSizes[ndx], params.flags, *conversion));
	}

	{
		using namespace drawutil;

		struct LocalUtil
		{
			static DrawState getDrawState (UVec2 renderSize)
			{
				DrawState state(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, renderSize.x(), renderSize.y());
				state.colorFormat = VK_FORMAT_R32G32_SFLOAT;
				return state;
			}

			static vector<Vec4> getVertices (void)
			{
				vector<Vec4> vertices;

				vertices.push_back(Vec4(-1.0f, -1.0f, 0.0f, 1.0f));
				vertices.push_back(Vec4(+1.0f, -1.0f, 0.0f, 1.0f));
				vertices.push_back(Vec4(-1.0f, +1.0f, 0.0f, 1.0f));

				vertices.push_back(Vec4(+1.0f, -1.0f, 0.0f, 1.0f));
				vertices.push_back(Vec4(-1.0f, +1.0f, 0.0f, 1.0f));
				vertices.push_back(Vec4(+1.0f, +1.0f, 0.0f, 1.0f));

				return vertices;
			}

			static VulkanProgram getProgram (Context& ctx, VkDescriptorSetLayout descriptorLayout, VkDescriptorSet descriptorSet)
			{
				VulkanProgram	prog;

				prog.shaders.push_back(VulkanShader(VK_SHADER_STAGE_VERTEX_BIT,		ctx.getBinaryCollection().get("vert")));
				prog.shaders.push_back(VulkanShader(VK_SHADER_STAGE_FRAGMENT_BIT,	ctx.getBinaryCollection().get("frag")));

				prog.descriptorSet			= descriptorSet;
				prog.descriptorSetLayout	= descriptorLayout;

				return prog;
			}
		};

		const UVec2					renderSize	(128, 256);
		const vector<Vec4>			vertices	(LocalUtil::getVertices());
		const DrawState				drawState	(LocalUtil::getDrawState(renderSize));
		const DrawCallData			drawCallData(vertices);
		const VulkanProgram			program		(LocalUtil::getProgram(context, *descLayout, *descSet));

		bool						allOk		= true;

		context.getTestContext().getLog()
			<< TestLog::Message << "Rendering " << renderSize << " quad" << TestLog::EndMessage;

		for (size_t imageNdx = 0; imageNdx < testImages.size(); ++imageNdx)
		{
			context.getTestContext().getLog()
				<< TestLog::Message << "Testing image size " << testImages[imageNdx]->getSize() << TestLog::EndMessage;

			bindImage(vkd, device, *descSet, testImages[imageNdx]->getImageView(), *sampler);

			VulkanDrawContext	renderer	(context, drawState, drawCallData, program);
			renderer.draw();

			{
				// Only du/dx and dv/dy are non-zero
				const Vec2					dtdp		= testImages[imageNdx]->getSize().cast<float>() / renderSize.cast<float>();
				const tcu::LodPrecision		lodPrec		(16, 4); // Pretty lax since we are not verifying LOD precision
				const Vec2					lodBounds	(tcu::computeLodBoundsFromDerivates(dtdp.x(), 0.0f, 0.0f, dtdp.y(), lodPrec));
				tcu::ConstPixelBufferAccess	resultImg	(renderer.getColorPixels());
				const int					maxErrors	= 5;
				int							numErrors	= 0;

				for (int y = 0; y < resultImg.getHeight(); ++y)
				for (int x = 0; x < resultImg.getWidth(); ++x)
				{
					const Vec2	result		= resultImg.getPixel(x, y).swizzle(0,1);
					const bool	levelOk		= result.x() == 0.0f;
					const bool	lodOk		= de::inRange(result.y(), lodBounds.x(), lodBounds.y());

					if (!levelOk || !lodOk)
					{
						if (numErrors < maxErrors)
						{
							context.getTestContext().getLog()
								<< TestLog::Message << "ERROR: At (" << x << ", " << y << ")"
													<< ": got " << result
													<< ", expected (0, [" << lodBounds.x() << ", " << lodBounds.y() << "])"
								<< TestLog::EndMessage;
						}
						else if (numErrors == maxErrors)
							context.getTestContext().getLog() << TestLog::Message << "..." << TestLog::EndMessage;

						numErrors += 1;
					}
				}

				allOk = allOk && (numErrors  == 0);
			}
		}

		if (allOk)
			return tcu::TestStatus::pass("Queries passed");
		else
			return tcu::TestStatus::fail("Got invalid results");
	}
}

void initImageQueryPrograms (SourceCollections& dst, TestParameters params)
{
	const ShaderSpec	spec	= getShaderSpec(params);

	generateSources(params.shaderType, spec, dst);
}

void initImageQueryLodPrograms (SourceCollections& dst, TestParameters)
{
	dst.glslSources.add("vert")
		<< glu::VertexSource("#version 450\n"
							 "layout(location = 0) in highp vec4 a_position;\n"
							 "layout(location = 0) out highp vec2 v_texCoord;\n"
							 "\n"
							 "void main (void)\n"
							 "{\n"
							 "	gl_Position = a_position;\n"
							 "	v_texCoord = a_position.xy * 0.5 - 0.5;\n"
							 "}\n");
	dst.glslSources.add("frag")
		<< glu::FragmentSource("#version 450\n"
							   "layout(binding = 0, set = 0) uniform highp sampler2D u_image;\n"
							   "layout(location = 0) in highp vec2 v_texCoord;\n"
							   "layout(location = 0) out highp vec2 o_lod;\n"
							   "\n"
							   "void main (void)\n"
							   "{\n"
							   "	o_lod = textureQueryLod(u_image, v_texCoord);\n"
							   "}\n");
}

void addImageQueryCase (tcu::TestCaseGroup* group, const TestParameters& params)
{
	std::string	name	= de::toLower(de::toString(params.format).substr(10));
	const bool	isLod	= params.query == QUERY_TYPE_IMAGE_LOD;

	if ((params.flags & VK_IMAGE_CREATE_DISJOINT_BIT_KHR) != 0)
		name += "_disjoint";

	addFunctionCaseWithPrograms(group,
								name,
								"",
								isLod ? initImageQueryLodPrograms : initImageQueryPrograms,
								isLod ? testImageQueryLod : testImageQuery,
								params);
}

struct QueryGroupParams
{
	QueryType		query;
	glu::ShaderType	shaderType;

	QueryGroupParams (QueryType query_, glu::ShaderType shaderType_)
		: query		(query_)
		, shaderType(shaderType_)
	{}

	QueryGroupParams (void)
		: query		(QUERY_TYPE_LAST)
		, shaderType(glu::SHADERTYPE_LAST)
	{}
};

void populateQueryInShaderGroup (tcu::TestCaseGroup* group, QueryGroupParams params)
{
	// "Reference" formats for testing
	addImageQueryCase(group, TestParameters(params.query, VK_FORMAT_R8G8B8A8_UNORM, 0u, params.shaderType));

	for (int formatNdx = VK_YCBCR_FORMAT_FIRST; formatNdx < VK_YCBCR_FORMAT_LAST; formatNdx++)
	{
		const VkFormat	format	= (VkFormat)formatNdx;

		addImageQueryCase(group, TestParameters(params.query, format, 0u, params.shaderType));

		if (getPlaneCount(format) > 1)
			addImageQueryCase(group, TestParameters(params.query, format, (VkImageCreateFlags)VK_IMAGE_CREATE_DISJOINT_BIT_KHR, params.shaderType));
	}
}

void populateQueryGroup (tcu::TestCaseGroup* group, QueryType query)
{
	for (int shaderTypeNdx = 0; shaderTypeNdx < glu::SHADERTYPE_LAST; ++shaderTypeNdx)
	{
		const glu::ShaderType	shaderType	= (glu::ShaderType)shaderTypeNdx;

		if (query == QUERY_TYPE_IMAGE_LOD && shaderType != glu::SHADERTYPE_FRAGMENT)
			continue;

		addTestGroup(group, glu::getShaderTypeName(shaderType), "", populateQueryInShaderGroup, QueryGroupParams(query, shaderType));
	}
}

void populateImageQueryGroup (tcu::TestCaseGroup* group)
{
	addTestGroup(group, "size_lod",	"OpImageQuerySizeLod",	populateQueryGroup, QUERY_TYPE_IMAGE_SIZE_LOD);
	addTestGroup(group, "lod",		"OpImageQueryLod",		populateQueryGroup, QUERY_TYPE_IMAGE_LOD);
	addTestGroup(group, "levels",	"OpImageQueryLevels",	populateQueryGroup, QUERY_TYPE_IMAGE_LEVELS);
}

} // namespace

tcu::TestCaseGroup* createImageQueryTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "query", "Image Query Tests", populateImageQueryGroup);
}

} // ycbcr
} // vkt
