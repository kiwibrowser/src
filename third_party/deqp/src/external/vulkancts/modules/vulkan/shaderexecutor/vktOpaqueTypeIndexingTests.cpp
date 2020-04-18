/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Opaque type (sampler, buffer, atomic counter, ...) indexing tests.
 *//*--------------------------------------------------------------------*/

#include "vktOpaqueTypeIndexingTests.hpp"

#include "vkRefUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkQueryUtil.hpp"

#include "tcuTexture.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTextureUtil.hpp"

#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "deRandom.hpp"
#include "deSTLUtil.hpp"

#include "vktShaderExecutor.hpp"

#include <sstream>

namespace vkt
{
namespace shaderexecutor
{

namespace
{

using de::UniquePtr;
using de::MovePtr;
using de::SharedPtr;
using std::vector;

using namespace vk;

typedef SharedPtr<Unique<VkSampler> > VkSamplerSp;

// Buffer helper

class Buffer
{
public:
								Buffer				(Context& context, VkBufferUsageFlags usage, size_t size);

	VkBuffer					getBuffer			(void) const { return *m_buffer;					}
	void*						getHostPtr			(void) const { return m_allocation->getHostPtr();	}
	void						flush				(void);
	void						invalidate			(void);

private:
	const DeviceInterface&		m_vkd;
	const VkDevice				m_device;
	const Unique<VkBuffer>		m_buffer;
	const UniquePtr<Allocation>	m_allocation;
};

typedef de::SharedPtr<Buffer> BufferSp;

Move<VkBuffer> createBuffer (const DeviceInterface& vkd, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usageFlags)
{
	const VkBufferCreateInfo	createInfo		=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		DE_NULL,
		(VkBufferCreateFlags)0,
		size,
		usageFlags,
		VK_SHARING_MODE_EXCLUSIVE,
		0u,
		DE_NULL
	};
	return createBuffer(vkd, device, &createInfo);
}

MovePtr<Allocation> allocateAndBindMemory (const DeviceInterface& vkd, VkDevice device, Allocator& allocator, VkBuffer buffer)
{
	MovePtr<Allocation>		alloc	(allocator.allocate(getBufferMemoryRequirements(vkd, device, buffer), MemoryRequirement::HostVisible));

	VK_CHECK(vkd.bindBufferMemory(device, buffer, alloc->getMemory(), alloc->getOffset()));

	return alloc;
}

Buffer::Buffer (Context& context, VkBufferUsageFlags usage, size_t size)
	: m_vkd			(context.getDeviceInterface())
	, m_device		(context.getDevice())
	, m_buffer		(createBuffer			(context.getDeviceInterface(),
											 context.getDevice(),
											 (VkDeviceSize)size,
											 usage))
	, m_allocation	(allocateAndBindMemory	(context.getDeviceInterface(),
											 context.getDevice(),
											 context.getDefaultAllocator(),
											 *m_buffer))
{
}

void Buffer::flush (void)
{
	flushMappedMemoryRange(m_vkd, m_device, m_allocation->getMemory(), m_allocation->getOffset(), VK_WHOLE_SIZE);
}

void Buffer::invalidate (void)
{
	invalidateMappedMemoryRange(m_vkd, m_device, m_allocation->getMemory(), m_allocation->getOffset(), VK_WHOLE_SIZE);
}

MovePtr<Buffer> createUniformIndexBuffer (Context& context, int numIndices, const int* indices)
{
	MovePtr<Buffer>		buffer	(new Buffer(context, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(int)*numIndices));
	int* const			bufPtr	= (int*)buffer->getHostPtr();

	for (int ndx = 0; ndx < numIndices; ++ndx)
		bufPtr[ndx] = indices[ndx];

	buffer->flush();

	return buffer;
}

// Tests

enum IndexExprType
{
	INDEX_EXPR_TYPE_CONST_LITERAL	= 0,
	INDEX_EXPR_TYPE_CONST_EXPRESSION,
	INDEX_EXPR_TYPE_UNIFORM,
	INDEX_EXPR_TYPE_DYNAMIC_UNIFORM,

	INDEX_EXPR_TYPE_LAST
};

enum TextureType
{
	TEXTURE_TYPE_1D = 0,
	TEXTURE_TYPE_2D,
	TEXTURE_TYPE_CUBE,
	TEXTURE_TYPE_2D_ARRAY,
	TEXTURE_TYPE_3D,

	TEXTURE_TYPE_LAST
};

class OpaqueTypeIndexingCase : public TestCase
{
public:
										OpaqueTypeIndexingCase		(tcu::TestContext&			testCtx,
																	 const char*				name,
																	 const char*				description,
																	 const glu::ShaderType		shaderType,
																	 const IndexExprType		indexExprType);
	virtual								~OpaqueTypeIndexingCase		(void);

	virtual void						initPrograms				(vk::SourceCollections& programCollection) const
										{
											generateSources(m_shaderType, m_shaderSpec, programCollection);
										}

protected:
	const char*							m_name;
	const glu::ShaderType				m_shaderType;
	const IndexExprType					m_indexExprType;
	ShaderSpec							m_shaderSpec;
};

OpaqueTypeIndexingCase::OpaqueTypeIndexingCase (tcu::TestContext&			testCtx,
												const char*					name,
												const char*					description,
												const glu::ShaderType		shaderType,
												const IndexExprType			indexExprType)
	: TestCase			(testCtx, name, description)
	, m_name			(name)
	, m_shaderType		(shaderType)
	, m_indexExprType	(indexExprType)
{
}

OpaqueTypeIndexingCase::~OpaqueTypeIndexingCase (void)
{
}

class OpaqueTypeIndexingTestInstance : public TestInstance
{
public:
										OpaqueTypeIndexingTestInstance		(Context&					context,
																			 const glu::ShaderType		shaderType,
																			 const ShaderSpec&			shaderSpec,
																			 const char*				name,
																			 const IndexExprType		indexExprType);
	virtual								~OpaqueTypeIndexingTestInstance		(void);

	virtual tcu::TestStatus				iterate								(void) = 0;

protected:
	void								checkSupported						(const VkDescriptorType descriptorType);

protected:
	tcu::TestContext&					m_testCtx;
	const glu::ShaderType				m_shaderType;
	const ShaderSpec&					m_shaderSpec;
	const char*							m_name;
	const IndexExprType					m_indexExprType;
};

OpaqueTypeIndexingTestInstance::OpaqueTypeIndexingTestInstance (Context&					context,
																const glu::ShaderType		shaderType,
																const ShaderSpec&			shaderSpec,
																const char*					name,
																const IndexExprType			indexExprType)
	: TestInstance		(context)
	, m_testCtx			(context.getTestContext())
	, m_shaderType		(shaderType)
	, m_shaderSpec		(shaderSpec)
	, m_name			(name)
	, m_indexExprType	(indexExprType)
{
}

OpaqueTypeIndexingTestInstance::~OpaqueTypeIndexingTestInstance (void)
{
}

void OpaqueTypeIndexingTestInstance::checkSupported (const VkDescriptorType descriptorType)
{
	const VkPhysicalDeviceFeatures& deviceFeatures = m_context.getDeviceFeatures();

	if (m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL && m_indexExprType != INDEX_EXPR_TYPE_CONST_EXPRESSION)
	{
		switch (descriptorType)
		{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				if (!deviceFeatures.shaderSampledImageArrayDynamicIndexing)
					TCU_THROW(NotSupportedError, "Dynamic indexing of sampler arrays is not supported");
				break;

			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				if (!deviceFeatures.shaderUniformBufferArrayDynamicIndexing)
					TCU_THROW(NotSupportedError, "Dynamic indexing of uniform buffer arrays is not supported");
				break;

			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				if (!deviceFeatures.shaderStorageBufferArrayDynamicIndexing)
					TCU_THROW(NotSupportedError, "Dynamic indexing of storage buffer arrays is not supported");
				break;

			default:
				break;
		}
	}
}

static void declareUniformIndexVars (std::ostream& str, deUint32 bindingLocation, const char* varPrefix, int numVars)
{
	str << "layout(set = " << EXTRA_RESOURCES_DESCRIPTOR_SET_INDEX << ", binding = " << bindingLocation << ", std140) uniform Indices\n{\n";

	for (int varNdx = 0; varNdx < numVars; varNdx++)
		str << "\thighp int " << varPrefix << varNdx << ";\n";

	str << "};\n";
}

static TextureType getTextureType (glu::DataType samplerType)
{
	switch (samplerType)
	{
		case glu::TYPE_SAMPLER_1D:
		case glu::TYPE_INT_SAMPLER_1D:
		case glu::TYPE_UINT_SAMPLER_1D:
		case glu::TYPE_SAMPLER_1D_SHADOW:
			return TEXTURE_TYPE_1D;

		case glu::TYPE_SAMPLER_2D:
		case glu::TYPE_INT_SAMPLER_2D:
		case glu::TYPE_UINT_SAMPLER_2D:
		case glu::TYPE_SAMPLER_2D_SHADOW:
			return TEXTURE_TYPE_2D;

		case glu::TYPE_SAMPLER_CUBE:
		case glu::TYPE_INT_SAMPLER_CUBE:
		case glu::TYPE_UINT_SAMPLER_CUBE:
		case glu::TYPE_SAMPLER_CUBE_SHADOW:
			return TEXTURE_TYPE_CUBE;

		case glu::TYPE_SAMPLER_2D_ARRAY:
		case glu::TYPE_INT_SAMPLER_2D_ARRAY:
		case glu::TYPE_UINT_SAMPLER_2D_ARRAY:
		case glu::TYPE_SAMPLER_2D_ARRAY_SHADOW:
			return TEXTURE_TYPE_2D_ARRAY;

		case glu::TYPE_SAMPLER_3D:
		case glu::TYPE_INT_SAMPLER_3D:
		case glu::TYPE_UINT_SAMPLER_3D:
			return TEXTURE_TYPE_3D;

		default:
			throw tcu::InternalError("Invalid sampler type");
	}
}

static bool isShadowSampler (glu::DataType samplerType)
{
	return samplerType == glu::TYPE_SAMPLER_1D_SHADOW		||
		   samplerType == glu::TYPE_SAMPLER_2D_SHADOW		||
		   samplerType == glu::TYPE_SAMPLER_2D_ARRAY_SHADOW	||
		   samplerType == glu::TYPE_SAMPLER_CUBE_SHADOW;
}

static glu::DataType getSamplerOutputType (glu::DataType samplerType)
{
	switch (samplerType)
	{
		case glu::TYPE_SAMPLER_1D:
		case glu::TYPE_SAMPLER_2D:
		case glu::TYPE_SAMPLER_CUBE:
		case glu::TYPE_SAMPLER_2D_ARRAY:
		case glu::TYPE_SAMPLER_3D:
			return glu::TYPE_FLOAT_VEC4;

		case glu::TYPE_SAMPLER_1D_SHADOW:
		case glu::TYPE_SAMPLER_2D_SHADOW:
		case glu::TYPE_SAMPLER_CUBE_SHADOW:
		case glu::TYPE_SAMPLER_2D_ARRAY_SHADOW:
			return glu::TYPE_FLOAT;

		case glu::TYPE_INT_SAMPLER_1D:
		case glu::TYPE_INT_SAMPLER_2D:
		case glu::TYPE_INT_SAMPLER_CUBE:
		case glu::TYPE_INT_SAMPLER_2D_ARRAY:
		case glu::TYPE_INT_SAMPLER_3D:
			return glu::TYPE_INT_VEC4;

		case glu::TYPE_UINT_SAMPLER_1D:
		case glu::TYPE_UINT_SAMPLER_2D:
		case glu::TYPE_UINT_SAMPLER_CUBE:
		case glu::TYPE_UINT_SAMPLER_2D_ARRAY:
		case glu::TYPE_UINT_SAMPLER_3D:
			return glu::TYPE_UINT_VEC4;

		default:
			throw tcu::InternalError("Invalid sampler type");
	}
}

static tcu::TextureFormat getSamplerTextureFormat (glu::DataType samplerType)
{
	const glu::DataType		outType			= getSamplerOutputType(samplerType);
	const glu::DataType		outScalarType	= glu::getDataTypeScalarType(outType);

	switch (outScalarType)
	{
		case glu::TYPE_FLOAT:
			if (isShadowSampler(samplerType))
				return tcu::TextureFormat(tcu::TextureFormat::D, tcu::TextureFormat::UNORM_INT16);
			else
				return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);

		case glu::TYPE_INT:		return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::SIGNED_INT8);
		case glu::TYPE_UINT:	return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT8);

		default:
			throw tcu::InternalError("Invalid sampler type");
	}
}

static glu::DataType getSamplerCoordType (glu::DataType samplerType)
{
	const TextureType	texType		= getTextureType(samplerType);
	int					numCoords	= 0;

	switch (texType)
	{
		case TEXTURE_TYPE_1D:		numCoords = 1;	break;
		case TEXTURE_TYPE_2D:		numCoords = 2;	break;
		case TEXTURE_TYPE_2D_ARRAY:	numCoords = 3;	break;
		case TEXTURE_TYPE_CUBE:		numCoords = 3;	break;
		case TEXTURE_TYPE_3D:		numCoords = 3;	break;
		default:
			DE_ASSERT(false);
	}

	if (isShadowSampler(samplerType))
		numCoords += 1;

	DE_ASSERT(de::inRange(numCoords, 1, 4));

	return numCoords == 1 ? glu::TYPE_FLOAT : glu::getDataTypeFloatVec(numCoords);
}

static void fillTextureData (const tcu::PixelBufferAccess& access, de::Random& rnd)
{
	DE_ASSERT(access.getHeight() == 1 && access.getDepth() == 1);

	if (access.getFormat().order == tcu::TextureFormat::D)
	{
		// \note Texture uses odd values, lookup even values to avoid precision issues.
		const float values[] = { 0.1f, 0.3f, 0.5f, 0.7f, 0.9f };

		for (int ndx = 0; ndx < access.getWidth(); ndx++)
			access.setPixDepth(rnd.choose<float>(DE_ARRAY_BEGIN(values), DE_ARRAY_END(values)), ndx, 0);
	}
	else
	{
		TCU_CHECK_INTERNAL(access.getFormat().order == tcu::TextureFormat::RGBA && access.getFormat().getPixelSize() == 4);

		for (int ndx = 0; ndx < access.getWidth(); ndx++)
			*((deUint32*)access.getDataPtr() + ndx) = rnd.getUint32();
	}
}

static vk::VkImageType getVkImageType (TextureType texType)
{
	switch (texType)
	{
		case TEXTURE_TYPE_1D:			return vk::VK_IMAGE_TYPE_1D;
		case TEXTURE_TYPE_2D:
		case TEXTURE_TYPE_2D_ARRAY:		return vk::VK_IMAGE_TYPE_2D;
		case TEXTURE_TYPE_CUBE:			return vk::VK_IMAGE_TYPE_2D;
		case TEXTURE_TYPE_3D:			return vk::VK_IMAGE_TYPE_3D;
		default:
			DE_FATAL("Impossible");
			return (vk::VkImageType)0;
	}
}

static vk::VkImageViewType getVkImageViewType (TextureType texType)
{
	switch (texType)
	{
		case TEXTURE_TYPE_1D:			return vk::VK_IMAGE_VIEW_TYPE_1D;
		case TEXTURE_TYPE_2D:			return vk::VK_IMAGE_VIEW_TYPE_2D;
		case TEXTURE_TYPE_2D_ARRAY:		return vk::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case TEXTURE_TYPE_CUBE:			return vk::VK_IMAGE_VIEW_TYPE_CUBE;
		case TEXTURE_TYPE_3D:			return vk::VK_IMAGE_VIEW_TYPE_3D;
		default:
			DE_FATAL("Impossible");
			return (vk::VkImageViewType)0;
	}
}

//! Test image with 1-pixel dimensions and no mipmaps
class TestImage
{
public:
								TestImage		(Context& context, TextureType texType, tcu::TextureFormat format, const void* colorValue);

	VkImageView					getImageView	(void) const { return *m_imageView; }

private:
	const Unique<VkImage>		m_image;
	const UniquePtr<Allocation>	m_allocation;
	const Unique<VkImageView>	m_imageView;
};

Move<VkImage> createTestImage (const DeviceInterface& vkd, VkDevice device, TextureType texType, tcu::TextureFormat format)
{
	const VkImageCreateInfo		createInfo		=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		DE_NULL,
		(texType == TEXTURE_TYPE_CUBE ? (VkImageCreateFlags)VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : (VkImageCreateFlags)0),
		getVkImageType(texType),
		mapTextureFormat(format),
		makeExtent3D(1, 1, 1),
		1u,
		(texType == TEXTURE_TYPE_CUBE) ? 6u : 1u,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0u,
		DE_NULL,
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	return createImage(vkd, device, &createInfo);
}

de::MovePtr<Allocation> allocateAndBindMemory (const DeviceInterface& vkd, VkDevice device, Allocator& allocator, VkImage image)
{
	de::MovePtr<Allocation>		alloc	= allocator.allocate(getImageMemoryRequirements(vkd, device, image), MemoryRequirement::Any);

	VK_CHECK(vkd.bindImageMemory(device, image, alloc->getMemory(), alloc->getOffset()));

	return alloc;
}

Move<VkImageView> createTestImageView (const DeviceInterface& vkd, VkDevice device, VkImage image, TextureType texType, tcu::TextureFormat format)
{
	const bool					isDepthImage	= format.order == tcu::TextureFormat::D;
	const VkImageViewCreateInfo	createInfo		=
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		DE_NULL,
		(VkImageViewCreateFlags)0,
		image,
		getVkImageViewType(texType),
		mapTextureFormat(format),
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		{
			(VkImageAspectFlags)(isDepthImage ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
			0u,
			1u,
			0u,
			(texType == TEXTURE_TYPE_CUBE ? 6u : 1u)
		}
	};

	return createImageView(vkd, device, &createInfo);
}

TestImage::TestImage (Context& context, TextureType texType, tcu::TextureFormat format, const void* colorValue)
	: m_image		(createTestImage		(context.getDeviceInterface(), context.getDevice(), texType, format))
	, m_allocation	(allocateAndBindMemory	(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), *m_image))
	, m_imageView	(createTestImageView	(context.getDeviceInterface(), context.getDevice(), *m_image, texType, format))
{
	const DeviceInterface&		vkd					= context.getDeviceInterface();
	const VkDevice				device				= context.getDevice();

	const size_t				pixelSize			= (size_t)format.getPixelSize();
	const deUint32				numLayers			= (texType == TEXTURE_TYPE_CUBE) ? 6u : 1u;
	const size_t				numReplicas			= (size_t)numLayers;
	const size_t				stagingBufferSize	= pixelSize*numReplicas;

	const VkBufferCreateInfo	stagingBufferInfo	=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		DE_NULL,
		(VkBufferCreateFlags)0u,
		(VkDeviceSize)stagingBufferSize,
		(VkBufferCreateFlags)VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0u,
		DE_NULL,
	};
	const Unique<VkBuffer>		stagingBuffer		(createBuffer(vkd, device, &stagingBufferInfo));
	const UniquePtr<Allocation>	alloc				(context.getDefaultAllocator().allocate(getBufferMemoryRequirements(vkd, device, *stagingBuffer), MemoryRequirement::HostVisible));

	VK_CHECK(vkd.bindBufferMemory(device, *stagingBuffer, alloc->getMemory(), alloc->getOffset()));

	for (size_t ndx = 0; ndx < numReplicas; ++ndx)
		deMemcpy((deUint8*)alloc->getHostPtr() + ndx*pixelSize, colorValue, pixelSize);

	flushMappedMemoryRange(vkd, device, alloc->getMemory(), alloc->getOffset(), VK_WHOLE_SIZE);

	{
		const Unique<VkCommandPool>		cmdPool			(createCommandPool(vkd, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, context.getUniversalQueueFamilyIndex()));
		const Unique<VkCommandBuffer>	cmdBuf			(allocateCommandBuffer(vkd, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		const VkCommandBufferBeginInfo	beginInfo		=
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			DE_NULL,
			(VkCommandBufferUsageFlags)VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			(const VkCommandBufferInheritanceInfo*)DE_NULL,
		};
		const VkImageAspectFlags		imageAspect		= (VkImageAspectFlags)(format.order == tcu::TextureFormat::D ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
		const VkBufferImageCopy			copyInfo		=
		{
			0u,
			1u,
			1u,
			{
				imageAspect,
				0u,
				0u,
				numLayers
			},
			{ 0u, 0u, 0u },
			{ 1u, 1u, 1u }
		};
		const VkImageMemoryBarrier		preCopyBarrier	=
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,
			(VkAccessFlags)0u,
			(VkAccessFlags)VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			*m_image,
			{
				imageAspect,
				0u,
				1u,
				0u,
				numLayers
			}
		};
		const VkImageMemoryBarrier		postCopyBarrier	=
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,
			(VkAccessFlags)VK_ACCESS_TRANSFER_WRITE_BIT,
			(VkAccessFlags)VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			*m_image,
			{
				imageAspect,
				0u,
				1u,
				0u,
				numLayers
			}
		};

		VK_CHECK(vkd.beginCommandBuffer(*cmdBuf, &beginInfo));
		vkd.cmdPipelineBarrier(*cmdBuf,
							   (VkPipelineStageFlags)VK_PIPELINE_STAGE_HOST_BIT,
							   (VkPipelineStageFlags)VK_PIPELINE_STAGE_TRANSFER_BIT,
							   (VkDependencyFlags)0u,
							   0u,
							   (const VkMemoryBarrier*)DE_NULL,
							   0u,
							   (const VkBufferMemoryBarrier*)DE_NULL,
							   1u,
							   &preCopyBarrier);
		vkd.cmdCopyBufferToImage(*cmdBuf, *stagingBuffer, *m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyInfo);
		vkd.cmdPipelineBarrier(*cmdBuf,
							   (VkPipelineStageFlags)VK_PIPELINE_STAGE_TRANSFER_BIT,
							   (VkPipelineStageFlags)VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
							   (VkDependencyFlags)0u,
							   0u,
							   (const VkMemoryBarrier*)DE_NULL,
							   0u,
							   (const VkBufferMemoryBarrier*)DE_NULL,
							   1u,
							   &postCopyBarrier);
		VK_CHECK(vkd.endCommandBuffer(*cmdBuf));

		{
			const Unique<VkFence>	fence		(createFence(vkd, device));
			const VkSubmitInfo		submitInfo	=
			{
				VK_STRUCTURE_TYPE_SUBMIT_INFO,
				DE_NULL,
				0u,
				(const VkSemaphore*)DE_NULL,
				(const VkPipelineStageFlags*)DE_NULL,
				1u,
				&cmdBuf.get(),
				0u,
				(const VkSemaphore*)DE_NULL,
			};

			VK_CHECK(vkd.queueSubmit(context.getUniversalQueue(), 1u, &submitInfo, *fence));
			VK_CHECK(vkd.waitForFences(device, 1u, &fence.get(), VK_TRUE, ~0ull));
		}
	}
}

typedef SharedPtr<TestImage> TestImageSp;

// SamplerIndexingCaseInstance

class SamplerIndexingCaseInstance : public OpaqueTypeIndexingTestInstance
{
public:
	enum
	{
		NUM_INVOCATIONS		= 64,
		NUM_SAMPLERS		= 8,
		NUM_LOOKUPS			= 4
	};

								SamplerIndexingCaseInstance		(Context&					context,
																 const glu::ShaderType		shaderType,
																 const ShaderSpec&			shaderSpec,
																 const char*				name,
																 glu::DataType				samplerType,
																 const IndexExprType		indexExprType,
																 const std::vector<int>&	lookupIndices);
	virtual						~SamplerIndexingCaseInstance	(void);

	virtual tcu::TestStatus		iterate							(void);

protected:
	const glu::DataType			m_samplerType;
	const std::vector<int>		m_lookupIndices;
};

SamplerIndexingCaseInstance::SamplerIndexingCaseInstance (Context&						context,
														  const glu::ShaderType			shaderType,
														  const ShaderSpec&				shaderSpec,
														  const char*					name,
														  glu::DataType					samplerType,
														  const IndexExprType			indexExprType,
														  const std::vector<int>&		lookupIndices)
	: OpaqueTypeIndexingTestInstance	(context, shaderType, shaderSpec, name, indexExprType)
	, m_samplerType						(samplerType)
	, m_lookupIndices					(lookupIndices)
{
}

SamplerIndexingCaseInstance::~SamplerIndexingCaseInstance (void)
{
}

bool isIntegerFormat (const tcu::TextureFormat& format)
{
	const tcu::TextureChannelClass	chnClass	= tcu::getTextureChannelClass(format.type);

	return chnClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER ||
		   chnClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER;
}

tcu::TestStatus SamplerIndexingCaseInstance::iterate (void)
{
	const int						numInvocations		= SamplerIndexingCaseInstance::NUM_INVOCATIONS;
	const int						numSamplers			= SamplerIndexingCaseInstance::NUM_SAMPLERS;
	const int						numLookups			= SamplerIndexingCaseInstance::NUM_LOOKUPS;
	const glu::DataType				coordType			= getSamplerCoordType(m_samplerType);
	const glu::DataType				outputType			= getSamplerOutputType(m_samplerType);
	const tcu::TextureFormat		texFormat			= getSamplerTextureFormat(m_samplerType);
	const int						outLookupStride		= numInvocations*getDataTypeScalarSize(outputType);
	vector<float>					coords;
	vector<deUint32>				outData;
	vector<deUint8>					texData				(numSamplers * texFormat.getPixelSize());
	const tcu::PixelBufferAccess	refTexAccess		(texFormat, numSamplers, 1, 1, &texData[0]);
	de::Random						rnd					(deInt32Hash(m_samplerType) ^ deInt32Hash(m_shaderType) ^ deInt32Hash(m_indexExprType));
	const TextureType				texType				= getTextureType(m_samplerType);
	const tcu::Sampler::FilterMode	filterMode			= (isShadowSampler(m_samplerType) || isIntegerFormat(texFormat)) ? tcu::Sampler::NEAREST : tcu::Sampler::LINEAR;

	// The shadow sampler with unnormalized coordinates is only used with the reference texture. Actual samplers in shaders use normalized coords.
	const tcu::Sampler				refSampler			= isShadowSampler(m_samplerType)
																? tcu::Sampler(tcu::Sampler::CLAMP_TO_EDGE, tcu::Sampler::CLAMP_TO_EDGE, tcu::Sampler::CLAMP_TO_EDGE,
																				filterMode, filterMode, 0.0f, false /* non-normalized */,
																				tcu::Sampler::COMPAREMODE_LESS)
																: tcu::Sampler(tcu::Sampler::CLAMP_TO_EDGE, tcu::Sampler::CLAMP_TO_EDGE, tcu::Sampler::CLAMP_TO_EDGE,
																				filterMode, filterMode);

	const DeviceInterface&			vkd					= m_context.getDeviceInterface();
	const VkDevice					device				= m_context.getDevice();
	vector<TestImageSp>				images;
	vector<VkSamplerSp>				samplers;
	MovePtr<Buffer>					indexBuffer;
	Move<VkDescriptorSetLayout>		extraResourcesLayout;
	Move<VkDescriptorPool>			extraResourcesSetPool;
	Move<VkDescriptorSet>			extraResourcesSet;

	checkSupported(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	coords.resize(numInvocations * getDataTypeScalarSize(coordType));

	if (texType == TEXTURE_TYPE_CUBE)
	{
		if (isShadowSampler(m_samplerType))
		{
			for (size_t i = 0; i < coords.size() / 4; i++)
			{
				coords[4 * i] = 1.0f;
				coords[4 * i + 1] = coords[4 * i + 2] = coords[4 * i + 3] = 0.0f;
			}
		}
		else
		{
			for (size_t i = 0; i < coords.size() / 3; i++)
			{
				coords[3 * i] = 1.0f;
				coords[3 * i + 1] = coords[3 * i + 2] = 0.0f;
			}
		}
	}

	if (isShadowSampler(m_samplerType))
	{
		// Use different comparison value per invocation.
		// \note Texture uses odd values, comparison even values.
		const int	numCoordComps	= getDataTypeScalarSize(coordType);
		const float	cmpValues[]		= { 0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };

		for (int invocationNdx = 0; invocationNdx < numInvocations; invocationNdx++)
			coords[invocationNdx*numCoordComps + (numCoordComps-1)] = rnd.choose<float>(DE_ARRAY_BEGIN(cmpValues), DE_ARRAY_END(cmpValues));
	}

	fillTextureData(refTexAccess, rnd);

	outData.resize(numLookups*outLookupStride);

	for (int ndx = 0; ndx < numSamplers; ++ndx)
	{
		images.push_back(TestImageSp(new TestImage(m_context, texType, texFormat, &texData[ndx * texFormat.getPixelSize()])));

		{
			tcu::Sampler	samplerCopy	(refSampler);
			samplerCopy.normalizedCoords = true;

			{
				const VkSamplerCreateInfo	samplerParams	= mapSampler(samplerCopy, texFormat);
				samplers.push_back(VkSamplerSp(new Unique<VkSampler>(createSampler(vkd, device, &samplerParams))));
			}
		}
	}

	if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
		indexBuffer = createUniformIndexBuffer(m_context, numLookups, &m_lookupIndices[0]);

	{
		const VkDescriptorSetLayoutBinding		bindings[]	=
		{
			{ 0u,						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	(deUint32)numSamplers,		VK_SHADER_STAGE_ALL,	DE_NULL		},
			{ (deUint32)numSamplers,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			1u,							VK_SHADER_STAGE_ALL,	DE_NULL		}
		};
		const VkDescriptorSetLayoutCreateInfo	layoutInfo	=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			DE_NULL,
			(VkDescriptorSetLayoutCreateFlags)0u,
			DE_LENGTH_OF_ARRAY(bindings),
			bindings,
		};

		extraResourcesLayout = createDescriptorSetLayout(vkd, device, &layoutInfo);
	}

	{
		const VkDescriptorPoolSize			poolSizes[]	=
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	(deUint32)numSamplers	},
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			1u,						}
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

		extraResourcesSetPool = createDescriptorPool(vkd, device, &poolInfo);
	}

	{
		const VkDescriptorSetAllocateInfo	allocInfo	=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,
			*extraResourcesSetPool,
			1u,
			&extraResourcesLayout.get(),
		};

		extraResourcesSet = allocateDescriptorSet(vkd, device, &allocInfo);
	}

	{
		vector<VkDescriptorImageInfo>	imageInfos			(numSamplers);
		const VkWriteDescriptorSet		descriptorWrite		=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			DE_NULL,
			*extraResourcesSet,
			0u,		// dstBinding
			0u,		// dstArrayElement
			(deUint32)numSamplers,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			&imageInfos[0],
			(const VkDescriptorBufferInfo*)DE_NULL,
			(const VkBufferView*)DE_NULL,
		};

		for (int ndx = 0; ndx < numSamplers; ++ndx)
		{
			imageInfos[ndx].sampler		= **samplers[ndx];
			imageInfos[ndx].imageView	= images[ndx]->getImageView();
			imageInfos[ndx].imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		vkd.updateDescriptorSets(device, 1u, &descriptorWrite, 0u, DE_NULL);
	}

	if (indexBuffer)
	{
		const VkDescriptorBufferInfo	bufferInfo	=
		{
			indexBuffer->getBuffer(),
			0u,
			VK_WHOLE_SIZE
		};
		const VkWriteDescriptorSet		descriptorWrite		=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			DE_NULL,
			*extraResourcesSet,
			(deUint32)numSamplers,	// dstBinding
			0u,						// dstArrayElement
			1u,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			(const VkDescriptorImageInfo*)DE_NULL,
			&bufferInfo,
			(const VkBufferView*)DE_NULL,
		};

		vkd.updateDescriptorSets(device, 1u, &descriptorWrite, 0u, DE_NULL);
	}

	{
		std::vector<void*>			inputs;
		std::vector<void*>			outputs;
		std::vector<int>			expandedIndices;
		UniquePtr<ShaderExecutor>	executor		(createExecutor(m_context, m_shaderType, m_shaderSpec, *extraResourcesLayout));

		inputs.push_back(&coords[0]);

		if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
		{
			expandedIndices.resize(numInvocations * m_lookupIndices.size());
			for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
			{
				for (int invNdx = 0; invNdx < numInvocations; invNdx++)
					expandedIndices[lookupNdx*numInvocations + invNdx] = m_lookupIndices[lookupNdx];
			}

			for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
				inputs.push_back(&expandedIndices[lookupNdx*numInvocations]);
		}

		for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
			outputs.push_back(&outData[outLookupStride*lookupNdx]);

		executor->execute(numInvocations, &inputs[0], &outputs[0], *extraResourcesSet);
	}

	{
		tcu::TestLog&		log				= m_context.getTestContext().getLog();
		tcu::TestStatus		testResult		= tcu::TestStatus::pass("Pass");

		if (isShadowSampler(m_samplerType))
		{
			const int			numCoordComps	= getDataTypeScalarSize(coordType);

			TCU_CHECK_INTERNAL(getDataTypeScalarSize(outputType) == 1);

			// Each invocation may have different results.
			for (int invocationNdx = 0; invocationNdx < numInvocations; invocationNdx++)
			{
				const float	coord	= coords[invocationNdx*numCoordComps + (numCoordComps-1)];

				for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
				{
					const int		texNdx		= m_lookupIndices[lookupNdx];
					const float		result		= *((const float*)(const deUint8*)&outData[lookupNdx*outLookupStride + invocationNdx]);
					const float		reference	= refTexAccess.sample2DCompare(refSampler, tcu::Sampler::NEAREST, coord, (float)texNdx, 0.0f, tcu::IVec3(0));

					if (de::abs(result-reference) > 0.005f)
					{
						log << tcu::TestLog::Message << "ERROR: at invocation " << invocationNdx << ", lookup " << lookupNdx << ": expected "
							<< reference << ", got " << result
							<< tcu::TestLog::EndMessage;

						if (testResult.getCode() == QP_TEST_RESULT_PASS)
							testResult = tcu::TestStatus::fail("Got invalid lookup result");
					}
				}
			}
		}
		else
		{
			TCU_CHECK_INTERNAL(getDataTypeScalarSize(outputType) == 4);

			// Validate results from first invocation
			for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
			{
				const int		texNdx	= m_lookupIndices[lookupNdx];
				const deUint8*	resPtr	= (const deUint8*)&outData[lookupNdx*outLookupStride];
				bool			isOk;

				if (outputType == glu::TYPE_FLOAT_VEC4)
				{
					const float			threshold		= 1.0f / 256.0f;
					const tcu::Vec4		reference		= refTexAccess.getPixel(texNdx, 0);
					const float*		floatPtr		= (const float*)resPtr;
					const tcu::Vec4		result			(floatPtr[0], floatPtr[1], floatPtr[2], floatPtr[3]);

					isOk = boolAll(lessThanEqual(abs(reference-result), tcu::Vec4(threshold)));

					if (!isOk)
					{
						log << tcu::TestLog::Message << "ERROR: at lookup " << lookupNdx << ": expected "
							<< reference << ", got " << result
							<< tcu::TestLog::EndMessage;
					}
				}
				else
				{
					const tcu::UVec4	reference		= refTexAccess.getPixelUint(texNdx, 0);
					const deUint32*		uintPtr			= (const deUint32*)resPtr;
					const tcu::UVec4	result			(uintPtr[0], uintPtr[1], uintPtr[2], uintPtr[3]);

					isOk = boolAll(equal(reference, result));

					if (!isOk)
					{
						log << tcu::TestLog::Message << "ERROR: at lookup " << lookupNdx << ": expected "
							<< reference << ", got " << result
							<< tcu::TestLog::EndMessage;
					}
				}

				if (!isOk && testResult.getCode() == QP_TEST_RESULT_PASS)
					testResult = tcu::TestStatus::fail("Got invalid lookup result");
			}

			// Check results of other invocations against first one
			for (int invocationNdx = 1; invocationNdx < numInvocations; invocationNdx++)
			{
				for (int lookupNdx = 0; lookupNdx < numLookups; lookupNdx++)
				{
					const deUint32*		refPtr		= &outData[lookupNdx*outLookupStride];
					const deUint32*		resPtr		= refPtr + invocationNdx*4;
					bool				isOk		= true;

					for (int ndx = 0; ndx < 4; ndx++)
						isOk = isOk && (refPtr[ndx] == resPtr[ndx]);

					if (!isOk)
					{
						log << tcu::TestLog::Message << "ERROR: invocation " << invocationNdx << " result "
							<< tcu::formatArray(tcu::Format::HexIterator<deUint32>(resPtr), tcu::Format::HexIterator<deUint32>(resPtr+4))
							<< " for lookup " << lookupNdx << " doesn't match result from first invocation "
							<< tcu::formatArray(tcu::Format::HexIterator<deUint32>(refPtr), tcu::Format::HexIterator<deUint32>(refPtr+4))
							<< tcu::TestLog::EndMessage;

						if (testResult.getCode() == QP_TEST_RESULT_PASS)
							testResult = tcu::TestStatus::fail("Inconsistent lookup results");
					}
				}
			}
		}

		return testResult;
	}
}

class SamplerIndexingCase : public OpaqueTypeIndexingCase
{
public:
								SamplerIndexingCase			(tcu::TestContext&			testCtx,
															 const char*				name,
															 const char*				description,
															 const glu::ShaderType		shaderType,
															 glu::DataType				samplerType,
															 IndexExprType				indexExprType);
	virtual						~SamplerIndexingCase		(void);

	virtual TestInstance*		createInstance				(Context& ctx) const;

private:
								SamplerIndexingCase			(const SamplerIndexingCase&);
	SamplerIndexingCase&		operator=					(const SamplerIndexingCase&);

	void						createShaderSpec			(void);

	const glu::DataType			m_samplerType;
	const int					m_numSamplers;
	const int					m_numLookups;
	std::vector<int>			m_lookupIndices;
};

SamplerIndexingCase::SamplerIndexingCase (tcu::TestContext&			testCtx,
										  const char*				name,
										  const char*				description,
										  const glu::ShaderType		shaderType,
										  glu::DataType				samplerType,
										  IndexExprType				indexExprType)
	: OpaqueTypeIndexingCase	(testCtx, name, description, shaderType, indexExprType)
	, m_samplerType				(samplerType)
	, m_numSamplers				(SamplerIndexingCaseInstance::NUM_SAMPLERS)
	, m_numLookups				(SamplerIndexingCaseInstance::NUM_LOOKUPS)
	, m_lookupIndices			(m_numLookups)
{
	createShaderSpec();
	init();
}

SamplerIndexingCase::~SamplerIndexingCase (void)
{
}

TestInstance* SamplerIndexingCase::createInstance (Context& ctx) const
{
	return new SamplerIndexingCaseInstance(ctx,
										   m_shaderType,
										   m_shaderSpec,
										   m_name,
										   m_samplerType,
										   m_indexExprType,
										   m_lookupIndices);
}

void SamplerIndexingCase::createShaderSpec (void)
{
	de::Random			rnd				(deInt32Hash(m_samplerType) ^ deInt32Hash(m_shaderType) ^ deInt32Hash(m_indexExprType));
	const char*			samplersName	= "texSampler";
	const char*			coordsName		= "coords";
	const char*			indicesPrefix	= "index";
	const char*			resultPrefix	= "result";
	const glu::DataType	coordType		= getSamplerCoordType(m_samplerType);
	const glu::DataType	outType			= getSamplerOutputType(m_samplerType);
	std::ostringstream	global, code;

	for (int ndx = 0; ndx < m_numLookups; ndx++)
		m_lookupIndices[ndx] = rnd.getInt(0, m_numSamplers-1);

	m_shaderSpec.inputs.push_back(Symbol(coordsName, glu::VarType(coordType, glu::PRECISION_HIGHP)));

	if (m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL)
		global << "#extension GL_EXT_gpu_shader5 : require\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
		global << "const highp int indexBase = 1;\n";

	global <<
		"layout(set = " << EXTRA_RESOURCES_DESCRIPTOR_SET_INDEX << ", binding = 0) uniform highp " << getDataTypeName(m_samplerType) << " " << samplersName << "[" << m_numSamplers << "];\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
	{
		for (int lookupNdx = 0; lookupNdx < m_numLookups; lookupNdx++)
		{
			const std::string varName = indicesPrefix + de::toString(lookupNdx);
			m_shaderSpec.inputs.push_back(Symbol(varName, glu::VarType(glu::TYPE_INT, glu::PRECISION_HIGHP)));
		}
	}
	else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
		declareUniformIndexVars(global, (deUint32)m_numSamplers, indicesPrefix, m_numLookups);

	for (int lookupNdx = 0; lookupNdx < m_numLookups; lookupNdx++)
	{
		const std::string varName = resultPrefix + de::toString(lookupNdx);
		m_shaderSpec.outputs.push_back(Symbol(varName, glu::VarType(outType, glu::PRECISION_HIGHP)));
	}

	for (int lookupNdx = 0; lookupNdx < m_numLookups; lookupNdx++)
	{
		code << resultPrefix << "" << lookupNdx << " = texture(" << samplersName << "[";

		if (m_indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL)
			code << m_lookupIndices[lookupNdx];
		else if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
			code << "indexBase + " << (m_lookupIndices[lookupNdx]-1);
		else
			code << indicesPrefix << lookupNdx;

		code << "], " << coordsName << ");\n";
	}

	m_shaderSpec.globalDeclarations	= global.str();
	m_shaderSpec.source				= code.str();
}

enum BlockType
{
	BLOCKTYPE_UNIFORM = 0,
	BLOCKTYPE_BUFFER,

	BLOCKTYPE_LAST
};

class BlockArrayIndexingCaseInstance : public OpaqueTypeIndexingTestInstance
{
public:
	enum
	{
		NUM_INVOCATIONS		= 32,
		NUM_INSTANCES		= 4,
		NUM_READS			= 4
	};

	enum Flags
	{
		FLAG_USE_STORAGE_BUFFER	= (1<<0)	// Use VK_KHR_storage_buffer_storage_class
	};

									BlockArrayIndexingCaseInstance	(Context&						context,
																	 const glu::ShaderType			shaderType,
																	 const ShaderSpec&				shaderSpec,
																	 const char*					name,
																	 BlockType						blockType,
																	 const deUint32					flags,
																	 const IndexExprType			indexExprType,
																	 const std::vector<int>&		readIndices,
																	 const std::vector<deUint32>&	inValues);
	virtual							~BlockArrayIndexingCaseInstance	(void);

	virtual tcu::TestStatus			iterate							(void);

private:
	const BlockType					m_blockType;
	const deUint32					m_flags;
	const std::vector<int>&			m_readIndices;
	const std::vector<deUint32>&	m_inValues;
};

BlockArrayIndexingCaseInstance::BlockArrayIndexingCaseInstance (Context&						context,
																const glu::ShaderType			shaderType,
																const ShaderSpec&				shaderSpec,
																const char*						name,
																BlockType						blockType,
																const deUint32					flags,
																const IndexExprType				indexExprType,
																const std::vector<int>&			readIndices,
																const std::vector<deUint32>&	inValues)
	: OpaqueTypeIndexingTestInstance	(context, shaderType, shaderSpec, name, indexExprType)
	, m_blockType						(blockType)
	, m_flags							(flags)
	, m_readIndices						(readIndices)
	, m_inValues						(inValues)
{
}

BlockArrayIndexingCaseInstance::~BlockArrayIndexingCaseInstance (void)
{
}

tcu::TestStatus BlockArrayIndexingCaseInstance::iterate (void)
{
	const int					numInvocations		= NUM_INVOCATIONS;
	const int					numReads			= NUM_READS;
	std::vector<deUint32>		outValues			(numInvocations*numReads);

	tcu::TestLog&				log					= m_context.getTestContext().getLog();
	tcu::TestStatus				testResult			= tcu::TestStatus::pass("Pass");

	std::vector<int>			expandedIndices;
	std::vector<void*>			inputs;
	std::vector<void*>			outputs;
	const VkBufferUsageFlags	bufferUsage			= m_blockType == BLOCKTYPE_UNIFORM ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	const VkDescriptorType		descriptorType		= m_blockType == BLOCKTYPE_UNIFORM ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	const DeviceInterface&		vkd					= m_context.getDeviceInterface();
	const VkDevice				device				= m_context.getDevice();

	// \note Using separate buffer per element - might want to test
	// offsets & single buffer in the future.
	vector<BufferSp>			buffers				(m_inValues.size());
	MovePtr<Buffer>				indexBuffer;

	Move<VkDescriptorSetLayout>	extraResourcesLayout;
	Move<VkDescriptorPool>		extraResourcesSetPool;
	Move<VkDescriptorSet>		extraResourcesSet;

	checkSupported(descriptorType);

	if ((m_flags & FLAG_USE_STORAGE_BUFFER) != 0)
	{
		if (!de::contains(m_context.getDeviceExtensions().begin(), m_context.getDeviceExtensions().end(), "VK_KHR_storage_buffer_storage_class"))
			TCU_THROW(NotSupportedError, "VK_KHR_storage_buffer_storage_class is not supported");
	}

	for (size_t bufferNdx = 0; bufferNdx < m_inValues.size(); ++bufferNdx)
	{
		buffers[bufferNdx] = BufferSp(new Buffer(m_context, bufferUsage, sizeof(deUint32)));
		*(deUint32*)buffers[bufferNdx]->getHostPtr() = m_inValues[bufferNdx];
		buffers[bufferNdx]->flush();
	}

	if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
		indexBuffer = createUniformIndexBuffer(m_context, numReads, &m_readIndices[0]);

	{
		const VkDescriptorSetLayoutBinding		bindings[]	=
		{
			{ 0u,							descriptorType,						(deUint32)m_inValues.size(),	VK_SHADER_STAGE_ALL,	DE_NULL		},
			{ (deUint32)m_inValues.size(),	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	1u,								VK_SHADER_STAGE_ALL,	DE_NULL		}
		};
		const VkDescriptorSetLayoutCreateInfo	layoutInfo	=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			DE_NULL,
			(VkDescriptorSetLayoutCreateFlags)0u,
			DE_LENGTH_OF_ARRAY(bindings),
			bindings,
		};

		extraResourcesLayout = createDescriptorSetLayout(vkd, device, &layoutInfo);
	}

	{
		const VkDescriptorPoolSize			poolSizes[]	=
		{
			{ descriptorType,						(deUint32)m_inValues.size()	},
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	1u,							}
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

		extraResourcesSetPool = createDescriptorPool(vkd, device, &poolInfo);
	}

	{
		const VkDescriptorSetAllocateInfo	allocInfo	=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,
			*extraResourcesSetPool,
			1u,
			&extraResourcesLayout.get(),
		};

		extraResourcesSet = allocateDescriptorSet(vkd, device, &allocInfo);
	}

	{
		vector<VkDescriptorBufferInfo>	bufferInfos			(m_inValues.size());
		const VkWriteDescriptorSet		descriptorWrite		=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			DE_NULL,
			*extraResourcesSet,
			0u,		// dstBinding
			0u,		// dstArrayElement
			(deUint32)m_inValues.size(),
			descriptorType,
			(const VkDescriptorImageInfo*)DE_NULL,
			&bufferInfos[0],
			(const VkBufferView*)DE_NULL,
		};

		for (size_t ndx = 0; ndx < m_inValues.size(); ++ndx)
		{
			bufferInfos[ndx].buffer		= buffers[ndx]->getBuffer();
			bufferInfos[ndx].offset		= 0u;
			bufferInfos[ndx].range		= VK_WHOLE_SIZE;
		}

		vkd.updateDescriptorSets(device, 1u, &descriptorWrite, 0u, DE_NULL);
	}

	if (indexBuffer)
	{
		const VkDescriptorBufferInfo	bufferInfo	=
		{
			indexBuffer->getBuffer(),
			0u,
			VK_WHOLE_SIZE
		};
		const VkWriteDescriptorSet		descriptorWrite		=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			DE_NULL,
			*extraResourcesSet,
			(deUint32)m_inValues.size(),	// dstBinding
			0u,								// dstArrayElement
			1u,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			(const VkDescriptorImageInfo*)DE_NULL,
			&bufferInfo,
			(const VkBufferView*)DE_NULL,
		};

		vkd.updateDescriptorSets(device, 1u, &descriptorWrite, 0u, DE_NULL);
	}

	if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
	{
		expandedIndices.resize(numInvocations * m_readIndices.size());

		for (int readNdx = 0; readNdx < numReads; readNdx++)
		{
			int* dst = &expandedIndices[numInvocations*readNdx];
			std::fill(dst, dst+numInvocations, m_readIndices[readNdx]);
		}

		for (int readNdx = 0; readNdx < numReads; readNdx++)
			inputs.push_back(&expandedIndices[readNdx*numInvocations]);
	}

	for (int readNdx = 0; readNdx < numReads; readNdx++)
		outputs.push_back(&outValues[readNdx*numInvocations]);

	{
		UniquePtr<ShaderExecutor>	executor	(createExecutor(m_context, m_shaderType, m_shaderSpec, *extraResourcesLayout));

		executor->execute(numInvocations, inputs.empty() ? DE_NULL : &inputs[0], &outputs[0], *extraResourcesSet);
	}

	for (int invocationNdx = 0; invocationNdx < numInvocations; invocationNdx++)
	{
		for (int readNdx = 0; readNdx < numReads; readNdx++)
		{
			const deUint32	refValue	= m_inValues[m_readIndices[readNdx]];
			const deUint32	resValue	= outValues[readNdx*numInvocations + invocationNdx];

			if (refValue != resValue)
			{
				log << tcu::TestLog::Message << "ERROR: at invocation " << invocationNdx
					<< ", read " << readNdx << ": expected "
					<< tcu::toHex(refValue) << ", got " << tcu::toHex(resValue)
					<< tcu::TestLog::EndMessage;

				if (testResult.getCode() == QP_TEST_RESULT_PASS)
					testResult = tcu::TestStatus::fail("Invalid result value");
			}
		}
	}

	return testResult;
}

class BlockArrayIndexingCase : public OpaqueTypeIndexingCase
{
public:
								BlockArrayIndexingCase		(tcu::TestContext&			testCtx,
															 const char*				name,
															 const char*				description,
															 BlockType					blockType,
															 IndexExprType				indexExprType,
															 const glu::ShaderType		shaderType,
															 deUint32					flags = 0u);
	virtual						~BlockArrayIndexingCase		(void);

	virtual TestInstance*		createInstance				(Context& ctx) const;

private:
								BlockArrayIndexingCase		(const BlockArrayIndexingCase&);
	BlockArrayIndexingCase&		operator=					(const BlockArrayIndexingCase&);

	void						createShaderSpec			(void);

	const BlockType				m_blockType;
	const deUint32				m_flags;
	std::vector<int>			m_readIndices;
	std::vector<deUint32>		m_inValues;
};

BlockArrayIndexingCase::BlockArrayIndexingCase (tcu::TestContext&			testCtx,
												const char*					name,
												const char*					description,
												BlockType					blockType,
												IndexExprType				indexExprType,
												const glu::ShaderType		shaderType,
												deUint32					flags)
	: OpaqueTypeIndexingCase	(testCtx, name, description, shaderType, indexExprType)
	, m_blockType				(blockType)
	, m_flags					(flags)
	, m_readIndices				(BlockArrayIndexingCaseInstance::NUM_READS)
	, m_inValues				(BlockArrayIndexingCaseInstance::NUM_INSTANCES)
{
	createShaderSpec();
	init();
}

BlockArrayIndexingCase::~BlockArrayIndexingCase (void)
{
}

TestInstance* BlockArrayIndexingCase::createInstance (Context& ctx) const
{
	return new BlockArrayIndexingCaseInstance(ctx,
											  m_shaderType,
											  m_shaderSpec,
											  m_name,
											  m_blockType,
											  m_flags,
											  m_indexExprType,
											  m_readIndices,
											  m_inValues);
}

void BlockArrayIndexingCase::createShaderSpec (void)
{
	const int			numInstances	= BlockArrayIndexingCaseInstance::NUM_INSTANCES;
	const int			numReads		= BlockArrayIndexingCaseInstance::NUM_READS;
	de::Random			rnd				(deInt32Hash(m_shaderType) ^ deInt32Hash(m_blockType) ^ deInt32Hash(m_indexExprType));
	const char*			blockName		= "Block";
	const char*			instanceName	= "block";
	const char*			indicesPrefix	= "index";
	const char*			resultPrefix	= "result";
	const char*			interfaceName	= m_blockType == BLOCKTYPE_UNIFORM ? "uniform" : "buffer";
	std::ostringstream	global, code;

	for (int readNdx = 0; readNdx < numReads; readNdx++)
		m_readIndices[readNdx] = rnd.getInt(0, numInstances-1);

	for (int instanceNdx = 0; instanceNdx < numInstances; instanceNdx++)
		m_inValues[instanceNdx] = rnd.getUint32();

	if (m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL)
		global << "#extension GL_EXT_gpu_shader5 : require\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
		global << "const highp int indexBase = 1;\n";

	global <<
		"layout(set = " << EXTRA_RESOURCES_DESCRIPTOR_SET_INDEX << ", binding = 0) " << interfaceName << " " << blockName << "\n"
		"{\n"
		"	highp uint value;\n"
		"} " << instanceName << "[" << numInstances << "];\n";

	if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
	{
		for (int readNdx = 0; readNdx < numReads; readNdx++)
		{
			const std::string varName = indicesPrefix + de::toString(readNdx);
			m_shaderSpec.inputs.push_back(Symbol(varName, glu::VarType(glu::TYPE_INT, glu::PRECISION_HIGHP)));
		}
	}
	else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
		declareUniformIndexVars(global, (deUint32)m_inValues.size(), indicesPrefix, numReads);

	for (int readNdx = 0; readNdx < numReads; readNdx++)
	{
		const std::string varName = resultPrefix + de::toString(readNdx);
		m_shaderSpec.outputs.push_back(Symbol(varName, glu::VarType(glu::TYPE_UINT, glu::PRECISION_HIGHP)));
	}

	for (int readNdx = 0; readNdx < numReads; readNdx++)
	{
		code << resultPrefix << readNdx << " = " << instanceName << "[";

		if (m_indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL)
			code << m_readIndices[readNdx];
		else if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
			code << "indexBase + " << (m_readIndices[readNdx]-1);
		else
			code << indicesPrefix << readNdx;

		code << "].value;\n";
	}

	m_shaderSpec.globalDeclarations	= global.str();
	m_shaderSpec.source				= code.str();

	if ((m_flags & BlockArrayIndexingCaseInstance::FLAG_USE_STORAGE_BUFFER) != 0)
		m_shaderSpec.buildOptions.flags |= vk::ShaderBuildOptions::FLAG_USE_STORAGE_BUFFER_STORAGE_CLASS;
}

class AtomicCounterIndexingCaseInstance : public OpaqueTypeIndexingTestInstance
{
public:
	enum
	{
		NUM_INVOCATIONS		= 32,
		NUM_COUNTERS		= 4,
		NUM_OPS				= 4
	};

								AtomicCounterIndexingCaseInstance	(Context&					context,
																	 const glu::ShaderType		shaderType,
																	 const ShaderSpec&			shaderSpec,
																	 const char*				name,
																	 const std::vector<int>&	opIndices,
																	 const IndexExprType		indexExprType);
	virtual						~AtomicCounterIndexingCaseInstance	(void);

	virtual	tcu::TestStatus		iterate								(void);

private:
	const std::vector<int>&		m_opIndices;
};

AtomicCounterIndexingCaseInstance::AtomicCounterIndexingCaseInstance (Context&					context,
																	  const glu::ShaderType		shaderType,
																	  const ShaderSpec&			shaderSpec,
																	  const char*				name,
																	  const std::vector<int>&	opIndices,
																	  const IndexExprType		indexExprType)
	: OpaqueTypeIndexingTestInstance	(context, shaderType, shaderSpec, name, indexExprType)
	, m_opIndices						(opIndices)
{
}

AtomicCounterIndexingCaseInstance::~AtomicCounterIndexingCaseInstance (void)
{
}

tcu::TestStatus AtomicCounterIndexingCaseInstance::iterate (void)
{
	const int					numInvocations		= NUM_INVOCATIONS;
	const int					numCounters			= NUM_COUNTERS;
	const int					numOps				= NUM_OPS;
	std::vector<int>			expandedIndices;
	std::vector<void*>			inputs;
	std::vector<void*>			outputs;
	std::vector<deUint32>		outValues			(numInvocations*numOps);

	const DeviceInterface&			vkd				= m_context.getDeviceInterface();
	const VkDevice					device			= m_context.getDevice();
	const VkPhysicalDeviceFeatures& deviceFeatures	= m_context.getDeviceFeatures();

	//Check stores and atomic operation support.
	switch (m_shaderType)
	{
		case glu::SHADERTYPE_VERTEX:
		case glu::SHADERTYPE_TESSELLATION_CONTROL:
		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
		case glu::SHADERTYPE_GEOMETRY:
			if(!deviceFeatures.vertexPipelineStoresAndAtomics)
				TCU_THROW(NotSupportedError, "Stores and atomic operations are not supported in Vertex, Tessellation, and Geometry shader.");
			break;
		case glu::SHADERTYPE_FRAGMENT:
			if(!deviceFeatures.fragmentStoresAndAtomics)
				TCU_THROW(NotSupportedError, "Stores and atomic operations are not supported in fragment shader.");
			break;
		case glu::SHADERTYPE_COMPUTE:
			break;
		default:
			throw tcu::InternalError("Unsupported shader type");
	}

	// \note Using separate buffer per element - might want to test
	// offsets & single buffer in the future.
	Buffer						atomicOpBuffer		(m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(deUint32)*numCounters);
	MovePtr<Buffer>				indexBuffer;

	Move<VkDescriptorSetLayout>	extraResourcesLayout;
	Move<VkDescriptorPool>		extraResourcesSetPool;
	Move<VkDescriptorSet>		extraResourcesSet;

	checkSupported(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

	deMemset(atomicOpBuffer.getHostPtr(), 0, sizeof(deUint32)*numCounters);
	atomicOpBuffer.flush();

	if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
		indexBuffer = createUniformIndexBuffer(m_context, numOps, &m_opIndices[0]);

	{
		const VkDescriptorSetLayoutBinding		bindings[]	=
		{
			{ 0u,	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	1u,	VK_SHADER_STAGE_ALL,	DE_NULL		},
			{ 1u,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	1u,	VK_SHADER_STAGE_ALL,	DE_NULL		}
		};
		const VkDescriptorSetLayoutCreateInfo	layoutInfo	=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			DE_NULL,
			(VkDescriptorSetLayoutCreateFlags)0u,
			DE_LENGTH_OF_ARRAY(bindings),
			bindings,
		};

		extraResourcesLayout = createDescriptorSetLayout(vkd, device, &layoutInfo);
	}

	{
		const VkDescriptorPoolSize			poolSizes[]	=
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	1u,	},
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	1u,	}
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

		extraResourcesSetPool = createDescriptorPool(vkd, device, &poolInfo);
	}

	{
		const VkDescriptorSetAllocateInfo	allocInfo	=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			DE_NULL,
			*extraResourcesSetPool,
			1u,
			&extraResourcesLayout.get(),
		};

		extraResourcesSet = allocateDescriptorSet(vkd, device, &allocInfo);
	}

	{
		const VkDescriptorBufferInfo	bufferInfo			=
		{
			atomicOpBuffer.getBuffer(),
			0u,
			VK_WHOLE_SIZE
		};
		const VkWriteDescriptorSet		descriptorWrite		=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			DE_NULL,
			*extraResourcesSet,
			0u,		// dstBinding
			0u,		// dstArrayElement
			1u,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			(const VkDescriptorImageInfo*)DE_NULL,
			&bufferInfo,
			(const VkBufferView*)DE_NULL,
		};

		vkd.updateDescriptorSets(device, 1u, &descriptorWrite, 0u, DE_NULL);
	}

	if (indexBuffer)
	{
		const VkDescriptorBufferInfo	bufferInfo	=
		{
			indexBuffer->getBuffer(),
			0u,
			VK_WHOLE_SIZE
		};
		const VkWriteDescriptorSet		descriptorWrite		=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			DE_NULL,
			*extraResourcesSet,
			1u,		// dstBinding
			0u,		// dstArrayElement
			1u,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			(const VkDescriptorImageInfo*)DE_NULL,
			&bufferInfo,
			(const VkBufferView*)DE_NULL,
		};

		vkd.updateDescriptorSets(device, 1u, &descriptorWrite, 0u, DE_NULL);
	}

	if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
	{
		expandedIndices.resize(numInvocations * m_opIndices.size());

		for (int opNdx = 0; opNdx < numOps; opNdx++)
		{
			int* dst = &expandedIndices[numInvocations*opNdx];
			std::fill(dst, dst+numInvocations, m_opIndices[opNdx]);
		}

		for (int opNdx = 0; opNdx < numOps; opNdx++)
			inputs.push_back(&expandedIndices[opNdx*numInvocations]);
	}

	for (int opNdx = 0; opNdx < numOps; opNdx++)
		outputs.push_back(&outValues[opNdx*numInvocations]);

	{
		UniquePtr<ShaderExecutor>	executor	(createExecutor(m_context, m_shaderType, m_shaderSpec, *extraResourcesLayout));

		executor->execute(numInvocations, inputs.empty() ? DE_NULL : &inputs[0], &outputs[0], *extraResourcesSet);
	}

	{
		tcu::TestLog&					log				= m_context.getTestContext().getLog();
		tcu::TestStatus					testResult		= tcu::TestStatus::pass("Pass");
		std::vector<int>				numHits			(numCounters, 0);	// Number of hits per counter.
		std::vector<deUint32>			counterValues	(numCounters);
		std::vector<std::vector<bool> >	counterMasks	(numCounters);

		for (int opNdx = 0; opNdx < numOps; opNdx++)
			numHits[m_opIndices[opNdx]] += 1;

		// Read counter values
		{
			const void* mapPtr = atomicOpBuffer.getHostPtr();
			DE_ASSERT(mapPtr != DE_NULL);
			atomicOpBuffer.invalidate();
			std::copy((const deUint32*)mapPtr, (const deUint32*)mapPtr + numCounters, &counterValues[0]);
		}

		// Verify counter values
		for (int counterNdx = 0; counterNdx < numCounters; counterNdx++)
		{
			const deUint32		refCount	= (deUint32)(numHits[counterNdx]*numInvocations);
			const deUint32		resCount	= counterValues[counterNdx];

			if (refCount != resCount)
			{
				log << tcu::TestLog::Message << "ERROR: atomic counter " << counterNdx << " has value " << resCount
					<< ", expected " << refCount
					<< tcu::TestLog::EndMessage;

				if (testResult.getCode() == QP_TEST_RESULT_PASS)
					testResult = tcu::TestStatus::fail("Invalid atomic counter value");
			}
		}

		// Allocate bitmasks - one bit per each valid result value
		for (int counterNdx = 0; counterNdx < numCounters; counterNdx++)
		{
			const int	counterValue	= numHits[counterNdx]*numInvocations;
			counterMasks[counterNdx].resize(counterValue, false);
		}

		// Verify result values from shaders
		for (int invocationNdx = 0; invocationNdx < numInvocations; invocationNdx++)
		{
			for (int opNdx = 0; opNdx < numOps; opNdx++)
			{
				const int		counterNdx	= m_opIndices[opNdx];
				const deUint32	resValue	= outValues[opNdx*numInvocations + invocationNdx];
				const bool		rangeOk		= de::inBounds(resValue, 0u, (deUint32)counterMasks[counterNdx].size());
				const bool		notSeen		= rangeOk && !counterMasks[counterNdx][resValue];
				const bool		isOk		= rangeOk && notSeen;

				if (!isOk)
				{
					log << tcu::TestLog::Message << "ERROR: at invocation " << invocationNdx
						<< ", op " << opNdx << ": got invalid result value "
						<< resValue
						<< tcu::TestLog::EndMessage;

					if (testResult.getCode() == QP_TEST_RESULT_PASS)
						testResult = tcu::TestStatus::fail("Invalid result value");
				}
				else
				{
					// Mark as used - no other invocation should see this value from same counter.
					counterMasks[counterNdx][resValue] = true;
				}
			}
		}

		if (testResult.getCode() == QP_TEST_RESULT_PASS)
		{
			// Consistency check - all masks should be 1 now
			for (int counterNdx = 0; counterNdx < numCounters; counterNdx++)
			{
				for (std::vector<bool>::const_iterator i = counterMasks[counterNdx].begin(); i != counterMasks[counterNdx].end(); i++)
					TCU_CHECK_INTERNAL(*i);
			}
		}

		return testResult;
	}
}

class AtomicCounterIndexingCase : public OpaqueTypeIndexingCase
{
public:
								AtomicCounterIndexingCase	(tcu::TestContext&			testCtx,
															 const char*				name,
															 const char*				description,
															 IndexExprType				indexExprType,
															 const glu::ShaderType		shaderType);
	virtual						~AtomicCounterIndexingCase	(void);

	virtual TestInstance*		createInstance				(Context& ctx) const;

private:
								AtomicCounterIndexingCase	(const BlockArrayIndexingCase&);
	AtomicCounterIndexingCase&	operator=					(const BlockArrayIndexingCase&);

	void						createShaderSpec			(void);

	std::vector<int>			m_opIndices;
};

AtomicCounterIndexingCase::AtomicCounterIndexingCase (tcu::TestContext&			testCtx,
													  const char*				name,
													  const char*				description,
													  IndexExprType				indexExprType,
													  const glu::ShaderType		shaderType)
	: OpaqueTypeIndexingCase	(testCtx, name, description, shaderType, indexExprType)
	, m_opIndices				(AtomicCounterIndexingCaseInstance::NUM_OPS)
{
	createShaderSpec();
	init();
}

AtomicCounterIndexingCase::~AtomicCounterIndexingCase (void)
{
}

TestInstance* AtomicCounterIndexingCase::createInstance (Context& ctx) const
{
	return new AtomicCounterIndexingCaseInstance(ctx,
												 m_shaderType,
												 m_shaderSpec,
												 m_name,
												 m_opIndices,
												 m_indexExprType);
}

void AtomicCounterIndexingCase::createShaderSpec (void)
{
	const int				numCounters		= AtomicCounterIndexingCaseInstance::NUM_COUNTERS;
	const int				numOps			= AtomicCounterIndexingCaseInstance::NUM_OPS;
	de::Random				rnd				(deInt32Hash(m_shaderType) ^ deInt32Hash(m_indexExprType));

	for (int opNdx = 0; opNdx < numOps; opNdx++)
		m_opIndices[opNdx] = rnd.getInt(0, numOps-1);

	{
		const char*			indicesPrefix	= "index";
		const char*			resultPrefix	= "result";
		std::ostringstream	global, code;

		if (m_indexExprType != INDEX_EXPR_TYPE_CONST_LITERAL)
			global << "#extension GL_EXT_gpu_shader5 : require\n";

		if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
			global << "const highp int indexBase = 1;\n";

		global <<
			"layout(set = " << EXTRA_RESOURCES_DESCRIPTOR_SET_INDEX << ", binding = 0, std430) buffer AtomicBuffer { highp uint counter[" << numCounters << "]; };\n";

		if (m_indexExprType == INDEX_EXPR_TYPE_DYNAMIC_UNIFORM)
		{
			for (int opNdx = 0; opNdx < numOps; opNdx++)
			{
				const std::string varName = indicesPrefix + de::toString(opNdx);
				m_shaderSpec.inputs.push_back(Symbol(varName, glu::VarType(glu::TYPE_INT, glu::PRECISION_HIGHP)));
			}
		}
		else if (m_indexExprType == INDEX_EXPR_TYPE_UNIFORM)
			declareUniformIndexVars(global, 1, indicesPrefix, numOps);

		for (int opNdx = 0; opNdx < numOps; opNdx++)
		{
			const std::string varName = resultPrefix + de::toString(opNdx);
			m_shaderSpec.outputs.push_back(Symbol(varName, glu::VarType(glu::TYPE_UINT, glu::PRECISION_HIGHP)));
		}

		for (int opNdx = 0; opNdx < numOps; opNdx++)
		{
			code << resultPrefix << opNdx << " = atomicAdd(counter[";

			if (m_indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL)
				code << m_opIndices[opNdx];
			else if (m_indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
				code << "indexBase + " << (m_opIndices[opNdx]-1);
			else
				code << indicesPrefix << opNdx;

			code << "], uint(1));\n";
		}

		m_shaderSpec.globalDeclarations	= global.str();
		m_shaderSpec.source				= code.str();
	}
}

class OpaqueTypeIndexingTests : public tcu::TestCaseGroup
{
public:
								OpaqueTypeIndexingTests		(tcu::TestContext& testCtx);
	virtual						~OpaqueTypeIndexingTests	(void);

	virtual void				init						(void);

private:
								OpaqueTypeIndexingTests		(const OpaqueTypeIndexingTests&);
	OpaqueTypeIndexingTests&	operator=					(const OpaqueTypeIndexingTests&);
};

OpaqueTypeIndexingTests::OpaqueTypeIndexingTests (tcu::TestContext& testCtx)
	: tcu::TestCaseGroup(testCtx, "opaque_type_indexing", "Opaque Type Indexing Tests")
{
}

OpaqueTypeIndexingTests::~OpaqueTypeIndexingTests (void)
{
}

void OpaqueTypeIndexingTests::init (void)
{
	static const struct
	{
		IndexExprType	type;
		const char*		name;
		const char*		description;
	} indexingTypes[] =
	{
		{ INDEX_EXPR_TYPE_CONST_LITERAL,	"const_literal",		"Indexing by constant literal"					},
		{ INDEX_EXPR_TYPE_CONST_EXPRESSION,	"const_expression",		"Indexing by constant expression"				},
		{ INDEX_EXPR_TYPE_UNIFORM,			"uniform",				"Indexing by uniform value"						},
		{ INDEX_EXPR_TYPE_DYNAMIC_UNIFORM,	"dynamically_uniform",	"Indexing by dynamically uniform expression"	}
	};

	static const struct
	{
		glu::ShaderType	type;
		const char*		name;
	} shaderTypes[] =
	{
		{ glu::SHADERTYPE_VERTEX,					"vertex"	},
		{ glu::SHADERTYPE_FRAGMENT,					"fragment"	},
		{ glu::SHADERTYPE_GEOMETRY,					"geometry"	},
		{ glu::SHADERTYPE_TESSELLATION_CONTROL,		"tess_ctrl"	},
		{ glu::SHADERTYPE_TESSELLATION_EVALUATION,	"tess_eval"	},
		{ glu::SHADERTYPE_COMPUTE,					"compute"	}
	};

	// .sampler
	{
		static const glu::DataType samplerTypes[] =
		{
			// \note 1D images will be added by a later extension.
//			glu::TYPE_SAMPLER_1D,
			glu::TYPE_SAMPLER_2D,
			glu::TYPE_SAMPLER_CUBE,
			glu::TYPE_SAMPLER_2D_ARRAY,
			glu::TYPE_SAMPLER_3D,
//			glu::TYPE_SAMPLER_1D_SHADOW,
			glu::TYPE_SAMPLER_2D_SHADOW,
			glu::TYPE_SAMPLER_CUBE_SHADOW,
			glu::TYPE_SAMPLER_2D_ARRAY_SHADOW,
//			glu::TYPE_INT_SAMPLER_1D,
			glu::TYPE_INT_SAMPLER_2D,
			glu::TYPE_INT_SAMPLER_CUBE,
			glu::TYPE_INT_SAMPLER_2D_ARRAY,
			glu::TYPE_INT_SAMPLER_3D,
//			glu::TYPE_UINT_SAMPLER_1D,
			glu::TYPE_UINT_SAMPLER_2D,
			glu::TYPE_UINT_SAMPLER_CUBE,
			glu::TYPE_UINT_SAMPLER_2D_ARRAY,
			glu::TYPE_UINT_SAMPLER_3D,
		};

		tcu::TestCaseGroup* const samplerGroup = new tcu::TestCaseGroup(m_testCtx, "sampler", "Sampler Array Indexing Tests");
		addChild(samplerGroup);

		for (int indexTypeNdx = 0; indexTypeNdx < DE_LENGTH_OF_ARRAY(indexingTypes); indexTypeNdx++)
		{
			const IndexExprType			indexExprType	= indexingTypes[indexTypeNdx].type;
			tcu::TestCaseGroup* const	indexGroup		= new tcu::TestCaseGroup(m_testCtx, indexingTypes[indexTypeNdx].name, indexingTypes[indexTypeNdx].description);
			samplerGroup->addChild(indexGroup);

			for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(shaderTypes); shaderTypeNdx++)
			{
				const glu::ShaderType		shaderType		= shaderTypes[shaderTypeNdx].type;
				tcu::TestCaseGroup* const	shaderGroup		= new tcu::TestCaseGroup(m_testCtx, shaderTypes[shaderTypeNdx].name, "");
				indexGroup->addChild(shaderGroup);

				// \note [pyry] In Vulkan CTS 1.0.2 sampler groups should not cover tess/geom stages
				if ((shaderType != glu::SHADERTYPE_VERTEX)		&&
					(shaderType != glu::SHADERTYPE_FRAGMENT)	&&
					(shaderType != glu::SHADERTYPE_COMPUTE))
					continue;

				for (int samplerTypeNdx = 0; samplerTypeNdx < DE_LENGTH_OF_ARRAY(samplerTypes); samplerTypeNdx++)
				{
					const glu::DataType	samplerType	= samplerTypes[samplerTypeNdx];
					const char*			samplerName	= getDataTypeName(samplerType);
					const std::string	caseName	= de::toLower(samplerName);

					shaderGroup->addChild(new SamplerIndexingCase(m_testCtx, caseName.c_str(), "", shaderType, samplerType, indexExprType));
				}
			}
		}
	}

	// .ubo / .ssbo / .atomic_counter
	{
		tcu::TestCaseGroup* const	uboGroup			= new tcu::TestCaseGroup(m_testCtx, "ubo",								"Uniform Block Instance Array Indexing Tests");
		tcu::TestCaseGroup* const	ssboGroup			= new tcu::TestCaseGroup(m_testCtx, "ssbo",								"Buffer Block Instance Array Indexing Tests");
		tcu::TestCaseGroup* const	ssboStorageBufGroup	= new tcu::TestCaseGroup(m_testCtx, "ssbo_storage_buffer_decoration",	"Buffer Block (new StorageBuffer decoration) Instance Array Indexing Tests");
		tcu::TestCaseGroup* const	acGroup				= new tcu::TestCaseGroup(m_testCtx, "atomic_counter",					"Atomic Counter Array Indexing Tests");
		addChild(uboGroup);
		addChild(ssboGroup);
		addChild(ssboStorageBufGroup);
		addChild(acGroup);

		for (int indexTypeNdx = 0; indexTypeNdx < DE_LENGTH_OF_ARRAY(indexingTypes); indexTypeNdx++)
		{
			const IndexExprType		indexExprType		= indexingTypes[indexTypeNdx].type;
			const char*				indexExprName		= indexingTypes[indexTypeNdx].name;
			const char*				indexExprDesc		= indexingTypes[indexTypeNdx].description;

			for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(shaderTypes); shaderTypeNdx++)
			{
				const glu::ShaderType	shaderType		= shaderTypes[shaderTypeNdx].type;
				const std::string		name			= std::string(indexExprName) + "_" + shaderTypes[shaderTypeNdx].name;

				// \note [pyry] In Vulkan CTS 1.0.2 ubo/ssbo/atomic_counter groups should not cover tess/geom stages
				if ((shaderType == glu::SHADERTYPE_VERTEX)		||
					(shaderType == glu::SHADERTYPE_FRAGMENT)	||
					(shaderType == glu::SHADERTYPE_COMPUTE))
				{
					uboGroup->addChild	(new BlockArrayIndexingCase		(m_testCtx, name.c_str(), indexExprDesc, BLOCKTYPE_UNIFORM,	indexExprType, shaderType));
					acGroup->addChild	(new AtomicCounterIndexingCase	(m_testCtx, name.c_str(), indexExprDesc, indexExprType, shaderType));

					if (indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL || indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
						ssboGroup->addChild	(new BlockArrayIndexingCase	(m_testCtx, name.c_str(), indexExprDesc, BLOCKTYPE_BUFFER, indexExprType, shaderType));
				}

				if (indexExprType == INDEX_EXPR_TYPE_CONST_LITERAL || indexExprType == INDEX_EXPR_TYPE_CONST_EXPRESSION)
					ssboStorageBufGroup->addChild	(new BlockArrayIndexingCase	(m_testCtx, name.c_str(), indexExprDesc, BLOCKTYPE_BUFFER, indexExprType, shaderType, (deUint32)BlockArrayIndexingCaseInstance::FLAG_USE_STORAGE_BUFFER));
			}
		}
	}
}

} // anonymous

tcu::TestCaseGroup* createOpaqueTypeIndexingTests (tcu::TestContext& testCtx)
{
	return new OpaqueTypeIndexingTests(testCtx);
}

} // shaderexecutor
} // vkt
