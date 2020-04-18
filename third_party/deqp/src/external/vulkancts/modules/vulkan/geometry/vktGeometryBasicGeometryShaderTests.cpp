/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Basic Geometry Shader Tests
 *//*--------------------------------------------------------------------*/

#include "vktGeometryBasicGeometryShaderTests.hpp"
#include "vktGeometryBasicClass.hpp"
#include "vktGeometryTestsUtil.hpp"

#include "gluTextureUtil.hpp"
#include "glwEnums.hpp"
#include "vkDefs.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkPrograms.hpp"
#include "vkBuilderUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"
#include "tcuTextureUtil.hpp"

#include <string>

using namespace vk;

namespace vkt
{
namespace geometry
{
namespace
{
using tcu::TestStatus;
using tcu::TestContext;
using tcu::TestCaseGroup;
using de::MovePtr;
using std::string;
using std::vector;

enum VaryingSource
{
	READ_ATTRIBUTE = 0,
	READ_UNIFORM,
	READ_TEXTURE,

	READ_LAST
};
enum ShaderInstancingMode
{
	MODE_WITHOUT_INSTANCING = 0,
	MODE_WITH_INSTANCING,

	MODE_LAST
};
enum
{
	EMIT_COUNT_VERTEX_0 = 6,
	EMIT_COUNT_VERTEX_1 = 0,
	EMIT_COUNT_VERTEX_2 = -1,
	EMIT_COUNT_VERTEX_3 = 10,
};
enum VariableTest
{
	TEST_POINT_SIZE = 0,
	TEST_PRIMITIVE_ID_IN,
	TEST_PRIMITIVE_ID,
	TEST_LAST
};

void uploadImage (Context&								context,
				  const tcu::ConstPixelBufferAccess&	access,
				  VkImage								destImage)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	const VkQueue					queue				= context.getUniversalQueue();
	Allocator&						memAlloc			= context.getDefaultAllocator();
	const VkImageAspectFlags		aspectMask			= VK_IMAGE_ASPECT_COLOR_BIT;
	const deUint32					bufferSize			= access.getWidth() * access.getHeight() * access.getDepth() * access.getFormat().getPixelSize();
	Move<VkBuffer>					buffer;
	de::MovePtr<Allocation>			bufferAlloc;
	Move<VkCommandPool>				cmdPool;
	Move<VkCommandBuffer>			cmdBuffer;
	Move<VkFence>					fence;

	// Create source buffer
	{
		const VkBufferCreateInfo bufferParams =
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			bufferSize,									// VkDeviceSize			size;
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			0u,											// deUint32				queueFamilyIndexCount;
			DE_NULL,									// const deUint32*		pQueueFamilyIndices;
		};
		buffer		= createBuffer(vk, device, &bufferParams);
		bufferAlloc	= memAlloc.allocate(getBufferMemoryRequirements(vk, device, *buffer), MemoryRequirement::HostVisible);
		VK_CHECK(vk.bindBufferMemory(device, *buffer, bufferAlloc->getMemory(), bufferAlloc->getOffset()));
	}

	// Create command pool and buffer
	{
		cmdPool = createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndex);

		const VkCommandBufferAllocateInfo cmdBufferAllocateInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			*cmdPool,										// VkCommandPool			commandPool;
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// VkCommandBufferLevel		level;
			1u,												// deUint32					bufferCount;
		};
		cmdBuffer = allocateCommandBuffer(vk, device, &cmdBufferAllocateInfo);
	}

	// Create fence
	fence = createFence(vk, device);

	// Barriers for copying buffer to image
	const VkBufferMemoryBarrier preBufferBarrier =
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType	sType;
		DE_NULL,									// const void*		pNext;
		VK_ACCESS_HOST_WRITE_BIT,					// VkAccessFlags	srcAccessMask;
		VK_ACCESS_TRANSFER_READ_BIT,				// VkAccessFlags	dstAccessMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32			dstQueueFamilyIndex;
		*buffer,									// VkBuffer			buffer;
		0u,											// VkDeviceSize		offset;
		bufferSize									// VkDeviceSize		size;
	};

	const VkImageMemoryBarrier preImageBarrier =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		0u,											// VkAccessFlags			srcAccessMask;
		VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags			dstAccessMask;
		VK_IMAGE_LAYOUT_UNDEFINED,					// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					dstQueueFamilyIndex;
		destImage,									// VkImage					image;
		{											// VkImageSubresourceRange	subresourceRange;
			aspectMask,								// VkImageAspect	aspect;
			0u,										// deUint32			baseMipLevel;
			1u,										// deUint32			mipLevels;
			0u,										// deUint32			baseArraySlice;
			1u										// deUint32			arraySize;
		}
	};

	const VkImageMemoryBarrier postImageBarrier =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags			srcAccessMask;
		VK_ACCESS_SHADER_READ_BIT,					// VkAccessFlags			dstAccessMask;
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,	// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32					dstQueueFamilyIndex;
		destImage,									// VkImage					image;
		{											// VkImageSubresourceRange	subresourceRange;
			aspectMask,								// VkImageAspect	aspect;
			0u,										// deUint32			baseMipLevel;
			1u,										// deUint32			mipLevels;
			0u,										// deUint32			baseArraySlice;
			1u										// deUint32			arraySize;
		}
	};

	const VkCommandBufferBeginInfo cmdBufferBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType					sType;
		DE_NULL,										// const void*						pNext;
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	// VkCommandBufferUsageFlags		flags;
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// Get copy regions and write buffer data
	const VkBufferImageCopy			copyRegions	=
	{
		0u,								// VkDeviceSize				bufferOffset;
		(deUint32)access.getWidth(),	// deUint32					bufferRowLength;
		(deUint32)access.getHeight(),	// deUint32					bufferImageHeight;
		{								// VkImageSubresourceLayers	imageSubresource;
				aspectMask,				// VkImageAspectFlags		aspectMask;
				(deUint32)0u,			// uint32_t					mipLevel;
				(deUint32)0u,			// uint32_t					baseArrayLayer;
				1u						// uint32_t					layerCount;
		},
		{ 0u, 0u, 0u },					// VkOffset3D			imageOffset;
		{								// VkExtent3D			imageExtent;
			(deUint32)access.getWidth(),
			(deUint32)access.getHeight(),
			(deUint32)access.getDepth()
		}
	};

	{
		const tcu::PixelBufferAccess	destAccess	(access.getFormat(), access.getSize(), bufferAlloc->getHostPtr());
		tcu::copy(destAccess, access);
		flushMappedMemoryRange(vk, device, bufferAlloc->getMemory(), bufferAlloc->getOffset(), bufferSize);
	}

	// Copy buffer to image
	VK_CHECK(vk.beginCommandBuffer(*cmdBuffer, &cmdBufferBeginInfo));
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &preBufferBarrier, 1, &preImageBarrier);
	vk.cmdCopyBufferToImage(*cmdBuffer, *buffer, destImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyRegions);
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &postImageBarrier);
	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));

	const VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
		DE_NULL,						// const void*					pNext;
		0u,								// deUint32						waitSemaphoreCount;
		DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
		DE_NULL,						// const VkPipelineStageFlags*	pWaitDstStageMask;
		1u,								// deUint32						commandBufferCount;
		&cmdBuffer.get(),				// const VkCommandBuffer*		pCommandBuffers;
		0u,								// deUint32						signalSemaphoreCount;
		DE_NULL							// const VkSemaphore*			pSignalSemaphores;
	};

	VK_CHECK(vk.queueSubmit(queue, 1, &submitInfo, *fence));
	VK_CHECK(vk.waitForFences(device, 1, &fence.get(), true, ~(0ull) /* infinity */));
}

class GeometryOutputCountTestInstance : public GeometryExpanderRenderTestInstance
{
public:
								GeometryOutputCountTestInstance	(Context&					context,
																 const VkPrimitiveTopology	primitiveType,
																 const int					primitiveCount,
																 const char*				name);
	void						genVertexAttribData				(void);
private:
	const int					m_primitiveCount;
};

GeometryOutputCountTestInstance::GeometryOutputCountTestInstance (Context&					context,
																  const VkPrimitiveTopology	primitiveType,
																  const int					primitiveCount,
																  const char*				name)
	: GeometryExpanderRenderTestInstance	(context, primitiveType, name)
	, m_primitiveCount						(primitiveCount)

{
	genVertexAttribData();
}

void GeometryOutputCountTestInstance::genVertexAttribData (void)
{
	m_vertexPosData.resize(m_primitiveCount);
	m_vertexAttrData.resize(m_primitiveCount);

	for (int ndx = 0; ndx < m_primitiveCount; ++ndx)
	{
		m_vertexPosData[ndx] = tcu::Vec4(-1.0f, ((float)ndx) / (float)m_primitiveCount * 2.0f - 1.0f, 0.0f, 1.0f);
		m_vertexAttrData[ndx] = (ndx % 2 == 0) ? tcu::Vec4(1, 1, 1, 1) : tcu::Vec4(1, 0, 0, 1);
	}
	m_numDrawVertices = m_primitiveCount;
}

class VaryingOutputCountTestInstance : public GeometryExpanderRenderTestInstance
{
public:
								VaryingOutputCountTestInstance	(Context&					context,
																 const char*				name,
																 const VkPrimitiveTopology	primitiveType,
																 const VaryingSource		test,
																 const ShaderInstancingMode	mode);
	void						genVertexAttribData				(void);
protected:
	Move<VkPipelineLayout>		createPipelineLayout			(const DeviceInterface& vk, const VkDevice device);
	void						bindDescriptorSets				(const DeviceInterface&		vk,
																 const VkDevice				device,
																 Allocator&					memAlloc,
																 const VkCommandBuffer&		cmdBuffer,
																 const VkPipelineLayout&	pipelineLayout);
private:
	void						genVertexDataWithoutInstancing	(void);
	void						genVertexDataWithInstancing		(void);

	const VaryingSource			m_test;
	const ShaderInstancingMode	m_mode;
	const deInt32				m_maxEmitCount;
	Move<VkDescriptorPool>		m_descriptorPool;
	Move<VkDescriptorSetLayout>	m_descriptorSetLayout;
	Move<VkDescriptorSet>		m_descriptorSet;
	Move<VkBuffer>				m_buffer;
	Move<VkImage>				m_texture;
	Move<VkImageView>			m_imageView;
	Move<VkSampler>				m_sampler;
	de::MovePtr<Allocation>		m_allocation;
};

VaryingOutputCountTestInstance::VaryingOutputCountTestInstance (Context&					context,
																const char*					name,
																const VkPrimitiveTopology	primitiveType,
																const VaryingSource			test,
																const ShaderInstancingMode	mode)
	: GeometryExpanderRenderTestInstance	(context, primitiveType, name)
	, m_test								(test)
	, m_mode								(mode)
	, m_maxEmitCount						(128)
{
	genVertexAttribData ();
}

void VaryingOutputCountTestInstance::genVertexAttribData (void)
{
	if (m_mode == MODE_WITHOUT_INSTANCING)
		genVertexDataWithoutInstancing();
	else if (m_mode == MODE_WITH_INSTANCING)
		genVertexDataWithInstancing();
	else
		DE_ASSERT(false);
}

Move<VkPipelineLayout> VaryingOutputCountTestInstance::createPipelineLayout (const DeviceInterface& vk, const VkDevice device)
{
	if (m_test == READ_UNIFORM)
	{
		m_descriptorSetLayout	=	DescriptorSetLayoutBuilder()
									.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT)
									.build(vk, device);
		m_descriptorPool		=	DescriptorPoolBuilder()
									.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
									.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);
		m_descriptorSet			=	makeDescriptorSet(vk, device, *m_descriptorPool, *m_descriptorSetLayout);

		return makePipelineLayout(vk, device, *m_descriptorSetLayout);
	}
	else if (m_test == READ_TEXTURE)
	{
		const tcu::Vec4				data[4]				=
														{
															tcu::Vec4(255, 0, 0, 0),
															tcu::Vec4(0, 255, 0, 0),
															tcu::Vec4(0, 0, 255, 0),
															tcu::Vec4(0, 0, 0, 255)
														};
		const tcu::UVec2			viewportSize		(4, 1);
		const tcu::TextureFormat	texFormat			= glu::mapGLInternalFormat(GL_RGBA8);
		const VkFormat				format				= mapTextureFormat(texFormat);
		const VkImageUsageFlags		imageUsageFlags		= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		Allocator&					memAlloc			= m_context.getDefaultAllocator();
		tcu::TextureLevel			texture				(texFormat, static_cast<int>(viewportSize.x()), static_cast<int>(viewportSize.y()));

		// Fill with data
		{
			tcu::PixelBufferAccess access = texture.getAccess();
			for (int x = 0; x < texture.getWidth(); ++x)
				access.setPixel(data[x], x, 0);
		}
		// Create image
		const VkImageCreateInfo			imageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,	// VkStructureType			sType;
			DE_NULL,								// const void*				pNext;
			0,										// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,						// VkImageType				imageType;
			format,									// VkFormat					format;
			{										// VkExtent3D				extent;
					viewportSize.x(),
					viewportSize.y(),
					1u,
			},
			1u,							// deUint32					mipLevels;
			1u,							// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,		// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,	// VkImageTiling			tiling;
			imageUsageFlags,			// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,	// VkSharingMode			sharingMode;
			0u,							// deUint32					queueFamilyIndexCount;
			DE_NULL,					// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED	// VkImageLayout			initialLayout;
		};

		m_texture		= createImage(vk, device, &imageParams);
		m_allocation	= memAlloc.allocate(getImageMemoryRequirements(vk, device, *m_texture), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(device, *m_texture, m_allocation->getMemory(), m_allocation->getOffset()));
		uploadImage(m_context, texture.getAccess(), *m_texture);

		m_descriptorSetLayout	=	DescriptorSetLayoutBuilder()
									.addSingleBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_GEOMETRY_BIT)
									.build(vk, device);
		m_descriptorPool		=	DescriptorPoolBuilder()
									.addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
									.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);
		m_descriptorSet			=	makeDescriptorSet(vk, device, *m_descriptorPool, *m_descriptorSetLayout);

		return makePipelineLayout(vk, device, *m_descriptorSetLayout);
	}
	else
		return makePipelineLayout(vk, device);
}

void VaryingOutputCountTestInstance::bindDescriptorSets (const DeviceInterface& vk, const VkDevice device, Allocator& memAlloc,
														 const VkCommandBuffer& cmdBuffer, const VkPipelineLayout& pipelineLayout)
{
	if (m_test == READ_UNIFORM)
	{
		const deInt32				emitCount[4]		= { 6, 0, m_maxEmitCount, 10 };
		const VkBufferCreateInfo	bufferCreateInfo	= makeBufferCreateInfo(sizeof(emitCount), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		m_buffer										= createBuffer(vk, device, &bufferCreateInfo);
		m_allocation									= memAlloc.allocate(getBufferMemoryRequirements(vk, device, *m_buffer), MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(device, *m_buffer, m_allocation->getMemory(), m_allocation->getOffset()));
		{
			deMemcpy(m_allocation->getHostPtr(), &emitCount[0], sizeof(emitCount));
			flushMappedMemoryRange(vk, device, m_allocation->getMemory(), m_allocation->getOffset(), sizeof(emitCount));

			const VkDescriptorBufferInfo bufferDescriptorInfo = makeDescriptorBufferInfo(*m_buffer, 0ull, sizeof(emitCount));

			DescriptorSetUpdateBuilder()
				.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferDescriptorInfo)
				.update(vk, device);
			vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0u, 1u, &*m_descriptorSet, 0u, DE_NULL);
		}
	}
	else if (m_test == READ_TEXTURE)
	{
		const tcu::TextureFormat	texFormat			= glu::mapGLInternalFormat(GL_RGBA8);
		const VkFormat				format				= mapTextureFormat(texFormat);
		const VkSamplerCreateInfo	samplerParams		=
														{
															VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,		// VkStructureType			sType;
															DE_NULL,									// const void*				pNext;
															0u,											// VkSamplerCreateFlags		flags;
															VK_FILTER_NEAREST,							// VkFilter					magFilter;
															VK_FILTER_NEAREST,							// VkFilter					minFilter;
															VK_SAMPLER_MIPMAP_MODE_NEAREST,				// VkSamplerMipmapMode		mipmapMode;
															VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeU;
															VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeV;
															VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeW;
															0.0f,										// float					mipLodBias;
															VK_FALSE,									// VkBool32					anisotropyEnable;
															1.0f,										// float					maxAnisotropy;
															false,										// VkBool32					compareEnable;
															VK_COMPARE_OP_NEVER,						// VkCompareOp				compareOp;
															0.0f,										// float					minLod;
															0.0f,										// float					maxLod;
															VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,	// VkBorderColor			borderColor;
															false										// VkBool32					unnormalizedCoordinates;
														};
		m_sampler										= createSampler(vk, device, &samplerParams);
		const VkImageViewCreateInfo	viewParams			=
														{
															VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,	// VkStructureType			sType;
															NULL,										// const voide*				pNext;
															0u,											// VkImageViewCreateFlags	flags;
															*m_texture,									// VkImage					image;
															VK_IMAGE_VIEW_TYPE_2D,						// VkImageViewType			viewType;
															format,										// VkFormat					format;
															makeComponentMappingRGBA(),					// VkChannelMapping			channels;
															{
																VK_IMAGE_ASPECT_COLOR_BIT,				// VkImageAspectFlags	aspectMask;
																0u,										// deUint32				baseMipLevel;
																1u,										// deUint32				mipLevels;
																0,										// deUint32				baseArraySlice;
																1u										// deUint32				arraySize;
															},											// VkImageSubresourceRange	subresourceRange;
														};
		m_imageView										= createImageView(vk, device, &viewParams);
		const VkDescriptorImageInfo descriptorImageInfo = makeDescriptorImageInfo (*m_sampler, *m_imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		DescriptorSetUpdateBuilder()
			.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptorImageInfo)
			.update(vk, device);
		vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0u, 1u, &*m_descriptorSet, 0u, DE_NULL);
	}
}

void VaryingOutputCountTestInstance::genVertexDataWithoutInstancing (void)
{
	m_numDrawVertices = 4;
	m_vertexPosData.resize(m_numDrawVertices);
	m_vertexAttrData.resize(m_numDrawVertices);

	m_vertexPosData[0] = tcu::Vec4( 0.5f,  0.0f, 0.0f, 1.0f);
	m_vertexPosData[1] = tcu::Vec4( 0.0f,  0.5f, 0.0f, 1.0f);
	m_vertexPosData[2] = tcu::Vec4(-0.7f, -0.1f, 0.0f, 1.0f);
	m_vertexPosData[3] = tcu::Vec4(-0.1f, -0.7f, 0.0f, 1.0f);

	if (m_test == READ_ATTRIBUTE)
	{
		m_vertexAttrData[0] = tcu::Vec4(((EMIT_COUNT_VERTEX_0 == -1) ? ((float)m_maxEmitCount) : ((float)EMIT_COUNT_VERTEX_0)), 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[1] = tcu::Vec4(((EMIT_COUNT_VERTEX_1 == -1) ? ((float)m_maxEmitCount) : ((float)EMIT_COUNT_VERTEX_1)), 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[2] = tcu::Vec4(((EMIT_COUNT_VERTEX_2 == -1) ? ((float)m_maxEmitCount) : ((float)EMIT_COUNT_VERTEX_2)), 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[3] = tcu::Vec4(((EMIT_COUNT_VERTEX_3 == -1) ? ((float)m_maxEmitCount) : ((float)EMIT_COUNT_VERTEX_3)), 0.0f, 0.0f, 0.0f);
	}
	else
	{
		m_vertexAttrData[0] = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[1] = tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[2] = tcu::Vec4(2.0f, 0.0f, 0.0f, 0.0f);
		m_vertexAttrData[3] = tcu::Vec4(3.0f, 0.0f, 0.0f, 0.0f);
	}
}

void VaryingOutputCountTestInstance::genVertexDataWithInstancing (void)
{
	m_numDrawVertices = 1;
	m_vertexPosData.resize(m_numDrawVertices);
	m_vertexAttrData.resize(m_numDrawVertices);

	m_vertexPosData[0] = tcu::Vec4(0.0f,  0.0f, 0.0f, 1.0f);

	if (m_test == READ_ATTRIBUTE)
	{
		const int emitCounts[] =
		{
			(EMIT_COUNT_VERTEX_0 == -1) ? (m_maxEmitCount) : (EMIT_COUNT_VERTEX_0),
			(EMIT_COUNT_VERTEX_1 == -1) ? (m_maxEmitCount) : (EMIT_COUNT_VERTEX_1),
			(EMIT_COUNT_VERTEX_2 == -1) ? (m_maxEmitCount) : (EMIT_COUNT_VERTEX_2),
			(EMIT_COUNT_VERTEX_3 == -1) ? (m_maxEmitCount) : (EMIT_COUNT_VERTEX_3),
		};

		m_vertexAttrData[0] = tcu::Vec4((float)emitCounts[0], (float)emitCounts[1], (float)emitCounts[2], (float)emitCounts[3]);
	}
	else
	{
		// not used
		m_vertexAttrData[0] = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

class BuiltinVariableRenderTestInstance: public GeometryExpanderRenderTestInstance
{
public:
			BuiltinVariableRenderTestInstance	(Context&				context,
												 const char*			name,
												 const VariableTest		test,
												 const bool				indicesTest);
	void	genVertexAttribData					(void);
	void	createIndicesBuffer					(void);

protected:
	void	drawCommand							(const VkCommandBuffer&	cmdBuffer);

private:
	const bool				m_indicesTest;
	std::vector<deUint16>	m_indices;
	Move<vk::VkBuffer>		m_indicesBuffer;
	MovePtr<Allocation>		m_allocation;
};

BuiltinVariableRenderTestInstance::BuiltinVariableRenderTestInstance (Context& context, const char* name, const VariableTest test, const bool indicesTest)
	: GeometryExpanderRenderTestInstance	(context, (test == TEST_PRIMITIVE_ID_IN) ? VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : VK_PRIMITIVE_TOPOLOGY_POINT_LIST, name)
	, m_indicesTest							(indicesTest)
{
	genVertexAttribData();
}

void BuiltinVariableRenderTestInstance::genVertexAttribData (void)
{
	m_numDrawVertices = 5;

	m_vertexPosData.resize(m_numDrawVertices);
	m_vertexPosData[0] = tcu::Vec4( 0.5f,  0.0f, 0.0f, 1.0f);
	m_vertexPosData[1] = tcu::Vec4( 0.0f,  0.5f, 0.0f, 1.0f);
	m_vertexPosData[2] = tcu::Vec4(-0.7f, -0.1f, 0.0f, 1.0f);
	m_vertexPosData[3] = tcu::Vec4(-0.1f, -0.7f, 0.0f, 1.0f);
	m_vertexPosData[4] = tcu::Vec4( 0.5f,  0.0f, 0.0f, 1.0f);

	m_vertexAttrData.resize(m_numDrawVertices);
	m_vertexAttrData[0] = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
	m_vertexAttrData[1] = tcu::Vec4(1.0f, 0.0f, 0.0f, 0.0f);
	m_vertexAttrData[2] = tcu::Vec4(2.0f, 0.0f, 0.0f, 0.0f);
	m_vertexAttrData[3] = tcu::Vec4(3.0f, 0.0f, 0.0f, 0.0f);
	m_vertexAttrData[4] = tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f);

	if (m_indicesTest)
	{
		// Only used by primitive ID restart test
		m_indices.resize(m_numDrawVertices);
		m_indices[0] = 1;
		m_indices[1] = 4;
		m_indices[2] = 0xFFFF; // restart
		m_indices[3] = 2;
		m_indices[4] = 1;
		createIndicesBuffer();
	}
}

void BuiltinVariableRenderTestInstance::createIndicesBuffer (void)
{
	// Create vertex indices buffer
	const DeviceInterface&			vk					= m_context.getDeviceInterface();
	const VkDevice					device				= m_context.getDevice();
	Allocator&						memAlloc			= m_context.getDefaultAllocator();
	const VkDeviceSize				indexBufferSize		= m_indices.size() * sizeof(deUint16);
	const VkBufferCreateInfo		indexBufferParams	=
														{
															VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
															DE_NULL,								// const void*			pNext;
															0u,										// VkBufferCreateFlags	flags;
															indexBufferSize,						// VkDeviceSize			size;
															VK_BUFFER_USAGE_INDEX_BUFFER_BIT,		// VkBufferUsageFlags	usage;
															VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
															0u,										// deUint32				queueFamilyCount;
															DE_NULL									// const deUint32*		pQueueFamilyIndices;
														};

	m_indicesBuffer = createBuffer(vk, device, &indexBufferParams);
	m_allocation = memAlloc.allocate(getBufferMemoryRequirements(vk, device, *m_indicesBuffer), MemoryRequirement::HostVisible);
	VK_CHECK(vk.bindBufferMemory(device, *m_indicesBuffer, m_allocation->getMemory(), m_allocation->getOffset()));
	// Load indices into buffer
	deMemcpy(m_allocation->getHostPtr(), &m_indices[0], (size_t)indexBufferSize);
	flushMappedMemoryRange(vk, device, m_allocation->getMemory(), m_allocation->getOffset(), indexBufferSize);
}

void BuiltinVariableRenderTestInstance::drawCommand (const VkCommandBuffer&	cmdBuffer)
{
	const DeviceInterface&	vk = m_context.getDeviceInterface();
	if (m_indicesTest)
	{
		vk.cmdBindIndexBuffer(cmdBuffer, *m_indicesBuffer, 0, VK_INDEX_TYPE_UINT16);
		vk.cmdDrawIndexed(cmdBuffer, static_cast<deUint32>(m_indices.size()), 1, 0, 0, 0);
	}
	else
		vk.cmdDraw(cmdBuffer, static_cast<deUint32>(m_numDrawVertices), 1u, 0u, 0u);
}

class GeometryOutputCountTest : public TestCase
{
public:
							GeometryOutputCountTest	(TestContext&		testCtx,
													 const char*		name,
													 const char*		description,
													 const vector<int>	pattern);

	void					initPrograms			(SourceCollections&			sourceCollections) const;
	virtual TestInstance*	createInstance			(Context&					context) const;

protected:
	const vector<int> m_pattern;
};

GeometryOutputCountTest::GeometryOutputCountTest (TestContext& testCtx, const char* name, const char* description, const vector<int> pattern)
	: TestCase	(testCtx, name, description)
	, m_pattern	(pattern)
{

}

void GeometryOutputCountTest::initPrograms (SourceCollections& sourceCollections) const
{
	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<<"layout(location = 0) in highp vec4 a_position;\n"
			<<"layout(location = 1) in highp vec4 a_color;\n"
			<<"layout(location = 0) out highp vec4 v_geom_FragColor;\n"
			<<"void main (void)\n"
			<<"{\n"
			<<"	gl_Position = a_position;\n"
			<<"	v_geom_FragColor = a_color;\n"
			<<"}\n";
		sourceCollections.glslSources.add("vertex") << glu::VertexSource(src.str());
	}

	{
		const int max_vertices = m_pattern.size() == 2 ? std::max(m_pattern[0], m_pattern[1]) : m_pattern[0];

		std::ostringstream src;
		src	<< "#version 310 es\n"
			<< "#extension GL_EXT_geometry_shader : require\n"
			<< "#extension GL_OES_texture_storage_multisample_2d_array : require\n"
			<< "layout(points) in;\n"
			<< "layout(triangle_strip, max_vertices = " << max_vertices << ") out;\n"
			<< "layout(location = 0) in highp vec4 v_geom_FragColor[];\n"
			<< "layout(location = 0) out highp vec4 v_frag_FragColor;\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "	const highp float rowHeight = 2.0 / float(" << m_pattern.size() << ");\n"
			<< "	const highp float colWidth = 2.0 / float(" << max_vertices << ");\n";

		if (m_pattern.size() == 2)
			src	<< "	highp int emitCount = (gl_PrimitiveIDIn == 0) ? (" << m_pattern[0] << ") : (" << m_pattern[1] << ");\n";
		else
			src	<< "	highp int emitCount = " << m_pattern[0] << ";\n";
		src	<< "	for (highp int ndx = 0; ndx < emitCount / 2; ndx++)\n"
			<< "	{\n"
			<< "		gl_Position = gl_in[0].gl_Position + vec4(float(ndx) * 2.0 * colWidth, 0.0, 0.0, 0.0);\n"
			<< "		v_frag_FragColor = v_geom_FragColor[0];\n"
			<< "		EmitVertex();\n"

			<< "		gl_Position = gl_in[0].gl_Position + vec4(float(ndx) * 2.0 * colWidth, rowHeight, 0.0, 0.0);\n"
			<< "		v_frag_FragColor = v_geom_FragColor[0];\n"
			<< "		EmitVertex();\n"

			<< "	}\n"
			<< "}\n";
		sourceCollections.glslSources.add("geometry") << glu::GeometrySource(src.str());
	}

	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<<"layout(location = 0) out mediump vec4 fragColor;\n"
			<<"layout(location = 0) in highp vec4 v_frag_FragColor;\n"
			<<"void main (void)\n"
			<<"{\n"
			<<"	fragColor = v_frag_FragColor;\n"
			<<"}\n";
		sourceCollections.glslSources.add("fragment") << glu::FragmentSource(src.str());
	}
}

TestInstance* GeometryOutputCountTest::createInstance (Context& context) const
{
	return new GeometryOutputCountTestInstance (context, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, static_cast<int>(m_pattern.size()), getName());
}

class VaryingOutputCountCase : public TestCase
{
public:
							VaryingOutputCountCase	(TestContext&				testCtx,
													 const char*				name,
													 const char*				description,
													 const VaryingSource		test,
													 const ShaderInstancingMode	mode);
	void					initPrograms			(SourceCollections&			sourceCollections) const;
	virtual TestInstance*	createInstance			(Context&					context) const;
protected:
	const VaryingSource			m_test;
	const ShaderInstancingMode	m_mode;
};

VaryingOutputCountCase::VaryingOutputCountCase (TestContext& testCtx, const char* name, const char* description, const VaryingSource test, const ShaderInstancingMode mode)
	: TestCase	(testCtx, name, description)
	, m_test	(test)
	, m_mode	(mode)
{
}

void VaryingOutputCountCase::initPrograms (SourceCollections& sourceCollections) const
{
	{
		std::ostringstream src;
		switch(m_test)
		{
			case READ_ATTRIBUTE:
			case READ_TEXTURE:
				src	<< "#version 310 es\n"
					<< "layout(location = 0) in highp vec4 a_position;\n"
					<< "layout(location = 1) in highp vec4 a_emitCount;\n"
					<< "layout(location = 0) out highp vec4 v_geom_emitCount;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = a_position;\n"
					<< "	v_geom_emitCount = a_emitCount;\n"
					<< "}\n";
				break;
			case READ_UNIFORM:
				src	<< "#version 310 es\n"
					<< "layout(location = 0) in highp vec4 a_position;\n"
					<< "layout(location = 1) in highp vec4 a_vertexNdx;\n"
					<< "layout(location = 0) out highp vec4 v_geom_vertexNdx;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = a_position;\n"
					<< "	v_geom_vertexNdx = a_vertexNdx;\n"
					<< "}\n";
				break;
			default:
				DE_ASSERT(0);
				break;
		}
		sourceCollections.glslSources.add("vertex") << glu::VertexSource(src.str());
	}

	{
		const bool instanced = MODE_WITH_INSTANCING == m_mode;
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<< "#extension GL_EXT_geometry_shader : require\n"
			<< "#extension GL_OES_texture_storage_multisample_2d_array : require\n";
		if (instanced)
			src	<< "layout(points, invocations=4) in;\n";
		else
			src	<< "layout(points) in;\n";

		switch(m_test)
		{
			case READ_ATTRIBUTE:
				src	<< "layout(triangle_strip, max_vertices = 128) out;\n"
					<< "layout(location = 0) in highp vec4 v_geom_emitCount[];\n"
					<< "layout(location = 0) out highp vec4 v_frag_FragColor;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	highp vec4 attrEmitCounts = v_geom_emitCount[0];\n"
					<< "	mediump int emitCount = int(attrEmitCounts[" << ((instanced) ? ("gl_InvocationID") : ("0")) << "]);\n"
					<< "	highp vec4 color = vec4((emitCount < 10) ? (0.0) : (1.0), (emitCount > 10) ? (0.0) : (1.0), 1.0, 1.0);\n"
					<< "	highp vec4 basePos = " << ((instanced) ? ("gl_in[0].gl_Position + 0.5 * vec4(cos(float(gl_InvocationID)), sin(float(gl_InvocationID)), 0.0, 0.0)") : ("gl_in[0].gl_Position")) << ";\n"
					<< "	for (mediump int i = 0; i < emitCount / 2; i++)\n"
					<< "	{\n"
					<< "		highp float angle = (float(i) + 0.5) / float(emitCount / 2) * 3.142;\n"
					<< "		gl_Position = basePos + vec4(cos(angle),  sin(angle), 0.0, 0.0) * 0.15;\n"
					<< "		v_frag_FragColor = color;\n"
					<< "		EmitVertex();\n"
					<< "		gl_Position = basePos + vec4(cos(angle), -sin(angle), 0.0, 0.0) * 0.15;\n"
					<< "		v_frag_FragColor = color;\n"
					<< "		EmitVertex();\n"
					<< "	}\n"
					<<"}\n";
				break;
			case READ_UNIFORM:
				src	<< "layout(triangle_strip, max_vertices = 128) out;\n"
					<< "layout(location = 0) in highp vec4 v_geom_vertexNdx[];\n"
					<< "layout(binding = 0) readonly uniform Input {\n"
					<< "	ivec4 u_emitCount;\n"
					<< "} emit;\n"
					<< "layout(location = 0) out highp vec4 v_frag_FragColor;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	mediump int primitiveNdx = " << ((instanced) ? ("gl_InvocationID") : ("int(v_geom_vertexNdx[0].x)")) << ";\n"
					<< "	mediump int emitCount = emit.u_emitCount[primitiveNdx];\n"
					<< "\n"
					<< "	const highp vec4 red = vec4(1.0, 0.0, 0.0, 1.0);\n"
					<< "	const highp vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
					<< "	const highp vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);\n"
					<< "	const highp vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
					<< "	const highp vec4 colors[4] = vec4[4](red, green, blue, yellow);\n"
					<< "	highp vec4 color = colors[int(primitiveNdx)];\n"
					<< "\n"
					<< "	highp vec4 basePos = " << ((instanced) ? ("gl_in[0].gl_Position + 0.5 * vec4(cos(float(gl_InvocationID)), sin(float(gl_InvocationID)), 0.0, 0.0)") : ("gl_in[0].gl_Position")) << ";\n"
					<< "	for (mediump int i = 0; i < emitCount / 2; i++)\n"
					<< "	{\n"
					<< "		highp float angle = (float(i) + 0.5) / float(emitCount / 2) * 3.142;\n"
					<< "		gl_Position = basePos + vec4(cos(angle),  sin(angle), 0.0, 0.0) * 0.15;\n"
					<< "		v_frag_FragColor = color;\n"
					<< "		EmitVertex();\n"
					<< "		gl_Position = basePos + vec4(cos(angle), -sin(angle), 0.0, 0.0) * 0.15;\n"
					<< "		v_frag_FragColor = color;\n"
					<< "		EmitVertex();\n"
					<< "	}\n"
					<<"}\n";
				break;
			case READ_TEXTURE:
				src	<< "layout(triangle_strip, max_vertices = 128) out;\n"
					<< "layout(location = 0) in highp vec4 v_geom_vertexNdx[];\n"
					<< "layout(binding = 0) uniform highp sampler2D u_sampler;\n"
					<< "layout(location = 0) out highp vec4 v_frag_FragColor;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	highp float primitiveNdx = " << ((instanced) ? ("float(gl_InvocationID)") : ("v_geom_vertexNdx[0].x")) << ";\n"
					<< "	highp vec2 texCoord = vec2(1.0 / 8.0 + primitiveNdx / 4.0, 0.5);\n"
					<< "	highp vec4 texColor = texture(u_sampler, texCoord);\n"
					<< "	mediump int emitCount = 0;\n"
					<< "	if (texColor.x > 0.0)\n"
					<< "		emitCount += 6;\n"
					<< "	if (texColor.y > 0.0)\n"
					<< "		emitCount += 0;\n"
					<< "	if (texColor.z > 0.0)\n"
					<< "		emitCount += 128;\n"
					<< "	if (texColor.w > 0.0)\n"
					<< "		emitCount += 10;\n"
					<< "	const highp vec4 red = vec4(1.0, 0.0, 0.0, 1.0);\n"
					<< "	const highp vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
					<< "	const highp vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);\n"
					<< "	const highp vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
					<< "	const highp vec4 colors[4] = vec4[4](red, green, blue, yellow);\n"
					<< "	highp vec4 color = colors[int(primitiveNdx)];\n"
					<< "	highp vec4 basePos = "<< ((instanced) ? ("gl_in[0].gl_Position + 0.5 * vec4(cos(float(gl_InvocationID)), sin(float(gl_InvocationID)), 0.0, 0.0)") : ("gl_in[0].gl_Position")) << ";\n"
					<< "	for (mediump int i = 0; i < emitCount / 2; i++)\n"
					<< "	{\n"
					<< "		highp float angle = (float(i) + 0.5) / float(emitCount / 2) * 3.142;\n"
					<< "		gl_Position = basePos + vec4(cos(angle),  sin(angle), 0.0, 0.0) * 0.15;\n"
					<< "		v_frag_FragColor = color;\n"
					<< "		EmitVertex();\n"
					<< "		gl_Position = basePos + vec4(cos(angle), -sin(angle), 0.0, 0.0) * 0.15;\n"
					<< "		v_frag_FragColor = color;\n"
					<< "		EmitVertex();\n"
					<< "	}\n"
					<<"}\n";
				break;
			default:
				DE_ASSERT(0);
				break;
		}
		sourceCollections.glslSources.add("geometry") << glu::GeometrySource(src.str());
	}

	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<< "layout(location = 0) out mediump vec4 fragColor;\n"
			<< "layout(location = 0) in highp vec4 v_frag_FragColor;\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "	fragColor = v_frag_FragColor;\n"
			<< "}\n";
		sourceCollections.glslSources.add("fragment") << glu::FragmentSource(src.str());
	}
}

TestInstance* VaryingOutputCountCase::createInstance (Context& context) const
{
	return new VaryingOutputCountTestInstance (context, getName(), VK_PRIMITIVE_TOPOLOGY_POINT_LIST, m_test, m_mode);
}

class BuiltinVariableRenderTest : public TestCase
{
public:
							BuiltinVariableRenderTest	(TestContext&		testCtx,
														const char*			name,
														const char*			desc,
														const VariableTest	test,
														const bool			flag = false);
	void					initPrograms				(SourceCollections&	sourceCollections) const;
	virtual TestInstance*	createInstance				(Context&			context) const;
protected:
	const VariableTest	m_test;
	const bool			m_flag;
};

BuiltinVariableRenderTest::BuiltinVariableRenderTest (TestContext& testCtx, const char* name, const char* description, const VariableTest test, const bool flag)
	: TestCase	(testCtx, name, description)
	, m_test	(test)
	, m_flag	(flag)
{
}

void BuiltinVariableRenderTest::initPrograms (SourceCollections& sourceCollections) const
{
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "out gl_PerVertex\n"
			<<" {\n"
			<< "	vec4 gl_Position;\n"
			<< "	float gl_PointSize;\n"
			<< "};\n"
			<< "layout(location = 0) in vec4 a_position;\n";
		switch(m_test)
		{
			case TEST_POINT_SIZE:
				src	<< "layout(location = 1) in vec4 a_pointSize;\n"
					<< "layout(location = 0) out vec4 v_geom_pointSize;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = a_position;\n"
					<< "	gl_PointSize = 1.0;\n"
					<< "	v_geom_pointSize = a_pointSize;\n"
					<< "}\n";
				break;
			case TEST_PRIMITIVE_ID_IN:
				src	<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = a_position;\n"
					<< "}\n";
				break;
			case TEST_PRIMITIVE_ID:
				src	<< "layout(location = 1) in vec4 a_primitiveID;\n"
					<< "layout(location = 0) out vec4 v_geom_primitiveID;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = a_position;\n"
					<< "	v_geom_primitiveID = a_primitiveID;\n"
					<< "}\n";
				break;
			default:
				DE_ASSERT(0);
				break;
		}
		sourceCollections.glslSources.add("vertex") << glu::VertexSource(src.str());
	}

	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "in gl_PerVertex\n"
			<<"{\n"
			<< "	vec4 gl_Position;\n"
			<< "	float gl_PointSize;\n"
			<< "} gl_in[];\n"
			<< "out gl_PerVertex\n"
			<<"{\n"
			<< "	vec4 gl_Position;\n"
			<< "	float gl_PointSize;\n"
			<< "};\n";
		switch(m_test)
		{
			case TEST_POINT_SIZE:
				src	<< "#extension GL_EXT_geometry_point_size : require\n"
					<< "layout(points) in;\n"
					<< "layout(points, max_vertices = 1) out;\n"
					<< "layout(location = 0) in vec4 v_geom_pointSize[];\n"
					<< "layout(location = 0) out vec4 v_frag_FragColor;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = gl_in[0].gl_Position;\n"
					<< "	gl_PointSize = v_geom_pointSize[0].x + 1.0;\n"
					<< "	v_frag_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
					<< "	EmitVertex();\n"
					<< "}\n";
				break;
			case TEST_PRIMITIVE_ID_IN:
				src	<< "layout(lines) in;\n"
					<< "layout(triangle_strip, max_vertices = 10) out;\n"
					<< "layout(location = 0) out vec4 v_frag_FragColor;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	const vec4 red = vec4(1.0, 0.0, 0.0, 1.0);\n"
					<< "	const vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
					<< "	const vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);\n"
					<< "	const vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
					<< "	const vec4 colors[4] = vec4[4](red, green, blue, yellow);\n"
					<< "	for (int counter = 0; counter < 3; ++counter)\n"
					<< "	{\n"
					<< "		float percent = 0.1 * counter;\n"
					<< "		gl_Position = gl_in[0].gl_Position * vec4(1.0 + percent, 1.0 + percent, 1.0, 1.0);\n"
					<< "		v_frag_FragColor = colors[gl_PrimitiveIDIn % 4];\n"
					<< "		EmitVertex();\n"
					<< "		gl_Position = gl_in[1].gl_Position * vec4(1.0 + percent, 1.0 + percent, 1.0, 1.0);\n"
					<< "		v_frag_FragColor = colors[gl_PrimitiveIDIn % 4];\n"
					<< "		EmitVertex();\n"
					<< "	}\n"
					<< "}\n";
				break;
			case TEST_PRIMITIVE_ID:
				src	<< "layout(points, invocations=1) in;\n"
					<< "layout(triangle_strip, max_vertices = 3) out;\n"
					<< "layout(location = 0) in vec4 v_geom_primitiveID[];\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = gl_in[0].gl_Position + vec4(0.05, 0.0, 0.0, 0.0);\n"
					<< "	gl_PrimitiveID = int(floor(v_geom_primitiveID[0].x)) + 3;\n"
					<< "	EmitVertex();\n"
					<< "	gl_Position = gl_in[0].gl_Position - vec4(0.05, 0.0, 0.0, 0.0);\n"
					<< "	gl_PrimitiveID = int(floor(v_geom_primitiveID[0].x)) + 3;\n"
					<< "	EmitVertex();\n"
					<< "	gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.05, 0.0, 0.0);\n"
					<< "	gl_PrimitiveID = int(floor(v_geom_primitiveID[0].x)) + 3;\n"
					<< "	EmitVertex();\n"
					<< "}\n";
				break;
			default:
				DE_ASSERT(0);
				break;
		}
		sourceCollections.glslSources.add("geometry") << glu::GeometrySource(src.str());
	}

	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n";
		switch(m_test)
		{
			case TEST_POINT_SIZE:
				src	<< "layout(location = 0) out vec4 fragColor;\n"
					<< "layout(location = 0) in vec4 v_frag_FragColor;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	fragColor = v_frag_FragColor;\n"
					<< "}\n";
				break;
			case TEST_PRIMITIVE_ID_IN:
				src	<< "layout(location = 0) out vec4 fragColor;\n"
					<< "layout(location = 0) in vec4 v_frag_FragColor;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	fragColor = v_frag_FragColor;\n"
					<< "}\n";
				break;
			case TEST_PRIMITIVE_ID:
				src	<< "layout(location = 0) out vec4 fragColor;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	const vec4 red			= vec4(1.0, 0.0, 0.0, 1.0);\n"
					<< "	const vec4 green		= vec4(0.0, 1.0, 0.0, 1.0);\n"
					<< "	const vec4 blue			= vec4(0.0, 0.0, 1.0, 1.0);\n"
					<< "	const vec4 yellow		= vec4(1.0, 1.0, 0.0, 1.0);\n"
					<< "	const vec4 colors[4]	= vec4[4](yellow, red, green, blue);\n"
					<< "	fragColor = colors[gl_PrimitiveID % 4];\n"
					<< "}\n";
				break;
			default:
				DE_ASSERT(0);
				break;
		}
		sourceCollections.glslSources.add("fragment") << glu::FragmentSource(src.str());
	}
}

TestInstance* BuiltinVariableRenderTest::createInstance (Context& context) const
{
	if (m_test == TEST_POINT_SIZE && !checkPointSize(context.getInstanceInterface(), context.getPhysicalDevice()))
			TCU_THROW(NotSupportedError, "Missing feature: pointSize");
	return new BuiltinVariableRenderTestInstance(context, getName(), m_test, m_flag);
}

inline vector<int> createPattern (int count)
{
	vector<int>	pattern;
	pattern.push_back(count);
	return pattern;
}

inline vector<int> createPattern (int count0, int count1)
{
	vector<int>	pattern;
	pattern.push_back(count0);
	pattern.push_back(count1);
	return pattern;
}

} // anonymous

TestCaseGroup* createBasicGeometryShaderTests (TestContext& testCtx)
{
	MovePtr<TestCaseGroup> basicGroup	(new tcu::TestCaseGroup(testCtx, "basic", "Basic tests."));

	basicGroup->addChild(new GeometryOutputCountTest	(testCtx,	"output_10",				"Output 10 vertices",								createPattern(10)));
	basicGroup->addChild(new GeometryOutputCountTest	(testCtx,	"output_128",				"Output 128 vertices",								createPattern(128)));
	basicGroup->addChild(new GeometryOutputCountTest	(testCtx,	"output_10_and_100",		"Output 10 and 100 vertices in two invocations",	createPattern(10, 100)));
	basicGroup->addChild(new GeometryOutputCountTest	(testCtx,	"output_100_and_10",		"Output 100 and 10 vertices in two invocations",	createPattern(100, 10)));
	basicGroup->addChild(new GeometryOutputCountTest	(testCtx,	"output_0_and_128",			"Output 0 and 128 vertices in two invocations",		createPattern(0, 128)));
	basicGroup->addChild(new GeometryOutputCountTest	(testCtx,	"output_128_and_0",			"Output 128 and 0 vertices in two invocations",		createPattern(128, 0)));

	basicGroup->addChild(new VaryingOutputCountCase		(testCtx,	"output_vary_by_attribute",				"Output varying number of vertices",	READ_ATTRIBUTE,	MODE_WITHOUT_INSTANCING));
	basicGroup->addChild(new VaryingOutputCountCase		(testCtx,	"output_vary_by_uniform",				"Output varying number of vertices",	READ_UNIFORM,	MODE_WITHOUT_INSTANCING));
	basicGroup->addChild(new VaryingOutputCountCase		(testCtx,	"output_vary_by_texture",				"Output varying number of vertices",	READ_TEXTURE,	MODE_WITHOUT_INSTANCING));
	basicGroup->addChild(new VaryingOutputCountCase		(testCtx,	"output_vary_by_attribute_instancing",	"Output varying number of vertices",	READ_ATTRIBUTE,	MODE_WITH_INSTANCING));
	basicGroup->addChild(new VaryingOutputCountCase		(testCtx,	"output_vary_by_uniform_instancing",	"Output varying number of vertices",	READ_UNIFORM,	MODE_WITH_INSTANCING));
	basicGroup->addChild(new VaryingOutputCountCase		(testCtx,	"output_vary_by_texture_instancing",	"Output varying number of vertices",	READ_TEXTURE,	MODE_WITH_INSTANCING));

	basicGroup->addChild(new BuiltinVariableRenderTest	(testCtx,	"point_size",					"test gl_PointSize",								TEST_POINT_SIZE));
	basicGroup->addChild(new BuiltinVariableRenderTest	(testCtx,	"primitive_id_in",				"test gl_PrimitiveIDIn",							TEST_PRIMITIVE_ID_IN));
	basicGroup->addChild(new BuiltinVariableRenderTest	(testCtx,	"primitive_id_in_restarted",	"test gl_PrimitiveIDIn with primitive restart",		TEST_PRIMITIVE_ID_IN, true));
	basicGroup->addChild(new BuiltinVariableRenderTest	(testCtx,	"primitive_id",					"test gl_PrimitiveID",								TEST_PRIMITIVE_ID));

	return basicGroup.release();
}

} // geometry
} // vkt
