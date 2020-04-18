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
 * \brief Multisampled image load/store Tests
 *//*--------------------------------------------------------------------*/

#include "vktImageMultisampleLoadStoreTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktImageTestsUtil.hpp"
#include "vktImageLoadStoreUtil.hpp"
#include "vktImageTexture.hpp"

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkMemUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkImageUtil.hpp"

#include "deUniquePtr.hpp"

#include "tcuTextureUtil.hpp"
#include "tcuTestLog.hpp"

#include <string>
#include <vector>

namespace vkt
{
namespace image
{
namespace
{
using namespace vk;
using de::MovePtr;
using de::UniquePtr;
using tcu::IVec3;

static const VkFormat CHECKSUM_IMAGE_FORMAT = VK_FORMAT_R32_SINT;

struct CaseDef
{
	Texture					texture;
	VkFormat				format;
	VkSampleCountFlagBits	numSamples;
	bool					singleLayerBind;
};

//  Multisampled storage image test.
//
//  Pass 1: Write a slightly different color pattern per-sample to the whole image.
//  Pass 2: Read samples of the same image and check if color values are in the expected range.
//          Write back results as a checksum image and verify them on the host.
//  Each checksum image pixel should contain an integer equal to the number of samples.

void initPrograms (SourceCollections& programCollection, const  CaseDef caseDef)
{
	const int			dimension			= (caseDef.singleLayerBind ? caseDef.texture.layerDimension() : caseDef.texture.dimension());
	const std::string	texelCoordStr		= (dimension == 1 ? "gx" : dimension == 2 ? "ivec2(gx, gy)" : dimension == 3 ? "ivec3(gx, gy, gz)" : "");

	const ImageType		usedImageType		= (caseDef.singleLayerBind ? getImageTypeForSingleLayer(caseDef.texture.type()) : caseDef.texture.type());
	const std::string	formatQualifierStr	= getShaderImageFormatQualifier(mapVkFormat(caseDef.format));
	const std::string	msImageTypeStr		= getShaderImageType(mapVkFormat(caseDef.format), usedImageType, (caseDef.texture.numSamples() > 1));

	const std::string	xMax				= de::toString(caseDef.texture.size().x() - 1);
	const std::string	yMax				= de::toString(caseDef.texture.size().y() - 1);
	const std::string	signednessPrefix	= isUintFormat(caseDef.format) ? "u" : isIntFormat(caseDef.format) ? "i" : "";
	const std::string	gvec4Expr			= signednessPrefix + "vec4";
	const int			numColorComponents	= tcu::getNumUsedChannels(mapVkFormat(caseDef.format).order);

	const float			storeColorScale		= computeStoreColorScale(caseDef.format, caseDef.texture.size());
	const float			storeColorBias		= computeStoreColorBias(caseDef.format);
	DE_ASSERT(colorScaleAndBiasAreValid(caseDef.format, storeColorScale, storeColorBias));

	const std::string	colorScaleExpr		= (storeColorScale == 1.0f ? "" : "*" + de::toString(storeColorScale))
											+ (storeColorBias == 0.0f ? "" : " + float(" + de::toString(storeColorBias) + ")");
	const std::string	colorExpr			=
		gvec4Expr + "("
		+                           "gx^gy^gz^(sampleNdx >> 5)^(sampleNdx & 31), "		// we "split" sampleNdx to keep this value in [0, 31] range for numSamples = 64 case
		+ (numColorComponents > 1 ? "(" + xMax + "-gx)^gy^gz, "              : "0, ")
		+ (numColorComponents > 2 ? "gx^(" + yMax + "-gy)^gz, "              : "0, ")
		+ (numColorComponents > 3 ? "(" + xMax + "-gx)^(" + yMax + "-gy)^gz" : "1")
		+ ")" + colorScaleExpr;

	// Store shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(local_size_x = 1) in;\n"
			<< "layout(set = 0, binding = 1, " << formatQualifierStr << ") writeonly uniform " << msImageTypeStr << " u_msImage;\n";

		if (caseDef.singleLayerBind)
			src << "layout(set = 0, binding = 0) readonly uniform Constants {\n"
				<< "    int u_layerNdx;\n"
				<< "};\n";

		src << "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    int gx = int(gl_GlobalInvocationID.x);\n"
			<< "    int gy = int(gl_GlobalInvocationID.y);\n"
			<< "    int gz = " << (caseDef.singleLayerBind ? "u_layerNdx" : "int(gl_GlobalInvocationID.z)") << ";\n"
			<< "\n"
			<< "    for (int sampleNdx = 0; sampleNdx < " << caseDef.texture.numSamples() <<"; ++sampleNdx) {\n"
			<< "        imageStore(u_msImage, " << texelCoordStr << ", sampleNdx, " << colorExpr << ");\n"
			<< "    }\n"
			<< "}\n";

		programCollection.glslSources.add("comp_store") << glu::ComputeSource(src.str());
	}

	// Load shader
	{
		const tcu::TextureFormat	checksumFormat			= mapVkFormat(CHECKSUM_IMAGE_FORMAT);
		const std::string			checksumImageTypeStr	= getShaderImageType(checksumFormat, usedImageType);
		const bool					useExactCompare			= isIntegerFormat(caseDef.format);

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(local_size_x = 1) in;\n"
			<< "layout(set = 0, binding = 1, " << formatQualifierStr << ") readonly  uniform " << msImageTypeStr << " u_msImage;\n"
			<< "layout(set = 0, binding = 2, " << getShaderImageFormatQualifier(checksumFormat) << ") writeonly uniform " << checksumImageTypeStr << " u_checksumImage;\n";

		if (caseDef.singleLayerBind)
			src << "layout(set = 0, binding = 0) readonly uniform Constants {\n"
				<< "    int u_layerNdx;\n"
				<< "};\n";

		src << "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    int gx = int(gl_GlobalInvocationID.x);\n"
			<< "    int gy = int(gl_GlobalInvocationID.y);\n"
			<< "    int gz = " << (caseDef.singleLayerBind ? "u_layerNdx" : "int(gl_GlobalInvocationID.z)") << ";\n"
			<< "\n"
			<< "    int checksum = 0;\n"
			<< "    for (int sampleNdx = 0; sampleNdx < " << caseDef.texture.numSamples() <<"; ++sampleNdx) {\n"
			<< "        " << gvec4Expr << " color = imageLoad(u_msImage, " << texelCoordStr << ", sampleNdx);\n";

		if (useExactCompare)
			src << "        if (color == " << colorExpr << ")\n"
				<< "            ++checksum;\n";
		else
			src << "        " << gvec4Expr << " diff  = abs(abs(color) - abs(" << colorExpr << "));\n"
				<< "        if (all(lessThan(diff, " << gvec4Expr << "(0.02))))\n"
				<< "            ++checksum;\n";

		src << "    }\n"
			<< "\n"
			<< "    imageStore(u_checksumImage, " << texelCoordStr << ", ivec4(checksum));\n"
			<< "}\n";

		programCollection.glslSources.add("comp_load") << glu::ComputeSource(src.str());
	}
}

void checkRequirements (const InstanceInterface& vki, const VkPhysicalDevice physDevice, const CaseDef& caseDef)
{
	VkPhysicalDeviceFeatures	features;
	vki.getPhysicalDeviceFeatures(physDevice, &features);

	if (!features.shaderStorageImageMultisample)
		TCU_THROW(NotSupportedError, "Multisampled storage images are not supported");

	VkImageFormatProperties		imageFormatProperties;
	const VkResult				imageFormatResult		= vki.getPhysicalDeviceImageFormatProperties(
		physDevice, caseDef.format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT, (VkImageCreateFlags)0, &imageFormatProperties);

	if (imageFormatResult == VK_ERROR_FORMAT_NOT_SUPPORTED)
		TCU_THROW(NotSupportedError, "Format is not supported");

	if ((imageFormatProperties.sampleCounts & caseDef.numSamples) != caseDef.numSamples)
		TCU_THROW(NotSupportedError, "Requested sample count is not supported");
}

//! Helper function to deal with per-layer resources.
void insertImageViews (const DeviceInterface& vk, const VkDevice device, const CaseDef& caseDef, const VkFormat format, const VkImage image, std::vector<SharedVkImageView>* const pOutImageViews)
{
	if (caseDef.singleLayerBind)
	{
		pOutImageViews->clear();
		pOutImageViews->resize(caseDef.texture.numLayers());
		for (int layerNdx = 0; layerNdx < caseDef.texture.numLayers(); ++layerNdx)
		{
			(*pOutImageViews)[layerNdx] = makeVkSharedPtr(makeImageView(
				vk, device, image, mapImageViewType(getImageTypeForSingleLayer(caseDef.texture.type())), format,
				makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, layerNdx, 1u)));
		}
	}
	else // bind all layers at once
	{
		pOutImageViews->clear();
		pOutImageViews->resize(1);
		(*pOutImageViews)[0] = makeVkSharedPtr(makeImageView(
			vk, device, image, mapImageViewType(caseDef.texture.type()), format,
			makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, caseDef.texture.numLayers())));
	}
}

//! Helper function to deal with per-layer resources.
void insertDescriptorSets (const DeviceInterface& vk, const VkDevice device, const CaseDef& caseDef, const VkDescriptorPool descriptorPool, const VkDescriptorSetLayout descriptorSetLayout, std::vector<SharedVkDescriptorSet>* const pOutDescriptorSets)
{
	if (caseDef.singleLayerBind)
	{
		pOutDescriptorSets->clear();
		pOutDescriptorSets->resize(caseDef.texture.numLayers());
		for (int layerNdx = 0; layerNdx < caseDef.texture.numLayers(); ++layerNdx)
			(*pOutDescriptorSets)[layerNdx] = makeVkSharedPtr(makeDescriptorSet(vk, device, descriptorPool, descriptorSetLayout));
	}
	else // bind all layers at once
	{
		pOutDescriptorSets->clear();
		pOutDescriptorSets->resize(1);
		(*pOutDescriptorSets)[0] = makeVkSharedPtr(makeDescriptorSet(vk, device, descriptorPool, descriptorSetLayout));
	}
}

tcu::TestStatus test (Context& context, const CaseDef caseDef)
{
	const InstanceInterface&	vki					= context.getInstanceInterface();
	const VkPhysicalDevice		physDevice			= context.getPhysicalDevice();
	const DeviceInterface&		vk					= context.getDeviceInterface();
	const VkDevice				device				= context.getDevice();
	const VkQueue				queue				= context.getUniversalQueue();
	const deUint32				queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&					allocator			= context.getDefaultAllocator();

	checkRequirements(vki, physDevice, caseDef);

	// Images

	const UniquePtr<Image> msImage(new Image(
		vk, device, allocator, makeImageCreateInfo(caseDef.texture, caseDef.format, VK_IMAGE_USAGE_STORAGE_BIT, 0u), MemoryRequirement::Any));

	const UniquePtr<Image> checksumImage(new Image(
		vk, device, allocator,
		makeImageCreateInfo(Texture(caseDef.texture, 1), CHECKSUM_IMAGE_FORMAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0u),
		MemoryRequirement::Any));

	// Buffer used to pass constants to the shader.

	const int			numLayers					= caseDef.texture.numLayers();
	const VkDeviceSize	bufferChunkSize				= getOptimalUniformBufferChunkSize(vki, physDevice, sizeof(deInt32));
	const VkDeviceSize	constantsBufferSizeBytes	= numLayers * bufferChunkSize;
	UniquePtr<Buffer>	constantsBuffer				(new Buffer(vk, device, allocator, makeBufferCreateInfo(constantsBufferSizeBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
													 MemoryRequirement::HostVisible));

	{
		const Allocation&	alloc	= constantsBuffer->getAllocation();
		deUint8* const		basePtr = static_cast<deUint8*>(alloc.getHostPtr());

		deMemset(alloc.getHostPtr(), 0, static_cast<size_t>(constantsBufferSizeBytes));

		for (int layerNdx = 0; layerNdx < numLayers; ++layerNdx)
		{
			deInt32* const valuePtr = reinterpret_cast<deInt32*>(basePtr + layerNdx * bufferChunkSize);
			*valuePtr = layerNdx;
		}

		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), constantsBufferSizeBytes);
	}

	const VkDeviceSize	resultBufferSizeBytes	= getImageSizeBytes(caseDef.texture.size(), CHECKSUM_IMAGE_FORMAT);
	UniquePtr<Buffer>	resultBuffer			(new Buffer(vk, device, allocator, makeBufferCreateInfo(resultBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT),
												 MemoryRequirement::HostVisible));

	{
		const Allocation& alloc = resultBuffer->getAllocation();
		deMemset(alloc.getHostPtr(), 0, static_cast<size_t>(resultBufferSizeBytes));
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), resultBufferSizeBytes);
	}

	// Descriptors

	Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numLayers)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numLayers)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numLayers)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, numLayers));

	std::vector<SharedVkDescriptorSet>	allDescriptorSets;
	std::vector<SharedVkImageView>		allMultisampledImageViews;
	std::vector<SharedVkImageView>		allChecksumImageViews;

	insertDescriptorSets(vk, device, caseDef, *descriptorPool, *descriptorSetLayout, &allDescriptorSets);
	insertImageViews	(vk, device, caseDef, caseDef.format, **msImage, &allMultisampledImageViews);
	insertImageViews	(vk, device, caseDef, CHECKSUM_IMAGE_FORMAT, **checksumImage, &allChecksumImageViews);

	// Prepare commands

	const Unique<VkPipelineLayout>	pipelineLayout	(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>		cmdPool			(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const tcu::IVec3				workSize				= (caseDef.singleLayerBind ? caseDef.texture.layerSize() : caseDef.texture.size());
	const int						loopNumLayers			= (caseDef.singleLayerBind ? numLayers : 1);
	const VkImageSubresourceRange	subresourceAllLayers	= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, caseDef.texture.numLayers());

	// Pass 1: Write MS image
	{
		const Unique<VkShaderModule>	shaderModule	(createShaderModule	(vk, device, context.getBinaryCollection().get("comp_store"), 0));
		const Unique<VkPipeline>		pipeline		(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

		beginCommandBuffer(vk, *cmdBuffer);
		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);

		{
			const VkImageMemoryBarrier barriers[] =
			{
				makeImageMemoryBarrier((VkAccessFlags)0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, **msImage, subresourceAllLayers),
				makeImageMemoryBarrier((VkAccessFlags)0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, **checksumImage, subresourceAllLayers),
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
		}

		for (int layerNdx = 0; layerNdx < loopNumLayers; ++layerNdx)
		{
			const VkDescriptorSet			descriptorSet					= **allDescriptorSets[layerNdx];
			const VkDescriptorImageInfo		descriptorMultiImageInfo		= makeDescriptorImageInfo(DE_NULL, **allMultisampledImageViews[layerNdx], VK_IMAGE_LAYOUT_GENERAL);
			const VkDescriptorBufferInfo	descriptorConstantsBufferInfo	= makeDescriptorBufferInfo(constantsBuffer->get(), layerNdx*bufferChunkSize, bufferChunkSize);

			DescriptorSetUpdateBuilder()
				.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorConstantsBufferInfo)
				.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorMultiImageInfo)
				.update(vk, device);

			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);
			vk.cmdDispatch(*cmdBuffer, workSize.x(), workSize.y(), workSize.z());
		}

		endCommandBuffer(vk, *cmdBuffer);
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Pass 2: "Resolve" MS image in compute shader
	{
		const Unique<VkShaderModule>	shaderModule	(createShaderModule	(vk, device, context.getBinaryCollection().get("comp_load"), 0));
		const Unique<VkPipeline>		pipeline		(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

		beginCommandBuffer(vk, *cmdBuffer);
		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);

		{
			const VkImageMemoryBarrier barriers[] =
			{
				makeImageMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, **msImage, subresourceAllLayers),
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
		}

		for (int layerNdx = 0; layerNdx < loopNumLayers; ++layerNdx)
		{
			const VkDescriptorSet			descriptorSet					= **allDescriptorSets[layerNdx];
			const VkDescriptorImageInfo		descriptorMultiImageInfo		= makeDescriptorImageInfo(DE_NULL, **allMultisampledImageViews[layerNdx], VK_IMAGE_LAYOUT_GENERAL);
			const VkDescriptorImageInfo		descriptorChecksumImageInfo		= makeDescriptorImageInfo(DE_NULL, **allChecksumImageViews[layerNdx], VK_IMAGE_LAYOUT_GENERAL);
			const VkDescriptorBufferInfo	descriptorConstantsBufferInfo	= makeDescriptorBufferInfo(constantsBuffer->get(), layerNdx*bufferChunkSize, bufferChunkSize);

			DescriptorSetUpdateBuilder()
				.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorConstantsBufferInfo)
				.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorMultiImageInfo)
				.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(2u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorChecksumImageInfo)
				.update(vk, device);

			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);
			vk.cmdDispatch(*cmdBuffer, workSize.x(), workSize.y(), workSize.z());
		}

		endCommandBuffer(vk, *cmdBuffer);
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Retrieve result
	{
		beginCommandBuffer(vk, *cmdBuffer);

		{
			const VkImageMemoryBarrier barriers[] =
			{
				makeImageMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, **checksumImage, subresourceAllLayers),
			};
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
		}
		{
			const VkBufferImageCopy copyRegion = makeBufferImageCopy(makeExtent3D(caseDef.texture.layerSize()), caseDef.texture.numLayers());
			vk.cmdCopyImageToBuffer(*cmdBuffer, **checksumImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, **resultBuffer, 1u, &copyRegion);
		}
		{
			const VkBufferMemoryBarrier barriers[] =
			{
				makeBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, **resultBuffer, 0ull, resultBufferSizeBytes),
			};
			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0,
				0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers, 0u, DE_NULL);
		}

		endCommandBuffer(vk, *cmdBuffer);
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Verify
	{
		const Allocation& alloc = resultBuffer->getAllocation();
		invalidateMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), resultBufferSizeBytes);

		const IVec3		imageSize			= caseDef.texture.size();
		const deInt32*	pDataPtr			= static_cast<deInt32*>(alloc.getHostPtr());
		const deInt32	expectedChecksum	= caseDef.texture.numSamples();

		for (int layer = 0; layer < imageSize.z(); ++layer)
		for (int y = 0; y < imageSize.y(); ++y)
		for (int x = 0; x < imageSize.x(); ++x)
		{
			if (*pDataPtr != expectedChecksum)
			{
				context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Some sample colors were incorrect at (x, y, layer) = (" << x << ", " << y << ", " << layer << ")"	<< tcu::TestLog::EndMessage
					<< tcu::TestLog::Message << "Checksum value is " << *pDataPtr << " but expected " << expectedChecksum << tcu::TestLog::EndMessage;

				return tcu::TestStatus::fail("Some sample colors were incorrect");
			}
			++pDataPtr;
		}

		return tcu::TestStatus::pass("OK");
	}
}

} // anonymous ns

tcu::TestCaseGroup* createImageMultisampleLoadStoreTests (tcu::TestContext& testCtx)
{
	const Texture textures[] =
	{
		// \note Shader code is tweaked to work with image size of 32, take a look if this needs to be modified.
		Texture(IMAGE_TYPE_2D,			tcu::IVec3(32,	32,	1),		1),
		Texture(IMAGE_TYPE_2D_ARRAY,	tcu::IVec3(32,	32,	1),		4),
	};

	static const VkFormat formats[] =
	{
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_SFLOAT,

		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R32_UINT,

		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R32_SINT,

		VK_FORMAT_R8G8B8A8_UNORM,

		VK_FORMAT_R8G8B8A8_SNORM,
	};

	static const VkSampleCountFlagBits samples[] =
	{
		VK_SAMPLE_COUNT_2_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_64_BIT,
	};

	MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "load_store_multisample", "Multisampled image store and load"));

	for (int baseTextureNdx = 0; baseTextureNdx < DE_LENGTH_OF_ARRAY(textures); ++baseTextureNdx)
	{
		const Texture&				baseTexture			= textures[baseTextureNdx];
		MovePtr<tcu::TestCaseGroup>	imageViewGroup		(new tcu::TestCaseGroup(testCtx, getImageTypeName(baseTexture.type()).c_str(), ""));
		const int					numLayerBindModes	= (baseTexture.numLayers() == 1 ? 1 : 2);

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
		for (int layerBindMode = 0; layerBindMode < numLayerBindModes; ++layerBindMode)
		{
			const bool					singleLayerBind	= (layerBindMode != 0);
			const std::string			formatGroupName	= getFormatShortString(formats[formatNdx]) + (singleLayerBind ? "_single_layer" : "");
			MovePtr<tcu::TestCaseGroup>	formatGroup		(new tcu::TestCaseGroup(testCtx, formatGroupName.c_str(), ""));

			for (int samplesNdx = 0; samplesNdx < DE_LENGTH_OF_ARRAY(samples); ++samplesNdx)
			{
				const std::string	samplesCaseName = "samples_" + de::toString(samples[samplesNdx]);

				const CaseDef		caseDef =
				{
					Texture(baseTexture, samples[samplesNdx]),
					formats[formatNdx],
					samples[samplesNdx],
					singleLayerBind,
				};

				addFunctionCaseWithPrograms(formatGroup.get(), samplesCaseName, "", initPrograms, test, caseDef);
			}
			imageViewGroup->addChild(formatGroup.release());
		}
		testGroup->addChild(imageViewGroup.release());
	}

	return testGroup.release();
}

} // image
} // vkt
