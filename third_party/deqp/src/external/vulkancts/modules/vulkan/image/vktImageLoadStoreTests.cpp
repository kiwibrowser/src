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
 * \brief Image load/store Tests
 *//*--------------------------------------------------------------------*/

#include "vktImageLoadStoreTests.hpp"
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
#include "deSharedPtr.hpp"
#include "deStringUtil.hpp"

#include "tcuImageCompare.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuFloat.hpp"

#include <string>
#include <vector>

using namespace vk;

namespace vkt
{
namespace image
{
namespace
{

inline VkBufferImageCopy makeBufferImageCopy (const Texture& texture)
{
	return image::makeBufferImageCopy(makeExtent3D(texture.layerSize()), texture.numLayers());
}

tcu::ConstPixelBufferAccess getLayerOrSlice (const Texture& texture, const tcu::ConstPixelBufferAccess access, const int layer)
{
	switch (texture.type())
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_2D:
		case IMAGE_TYPE_BUFFER:
			// Not layered
			DE_ASSERT(layer == 0);
			return access;

		case IMAGE_TYPE_1D_ARRAY:
			return tcu::getSubregion(access, 0, layer, access.getWidth(), 1);

		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
		case IMAGE_TYPE_3D:			// 3d texture is treated as if depth was the layers
			return tcu::getSubregion(access, 0, 0, layer, access.getWidth(), access.getHeight(), 1);

		default:
			DE_FATAL("Internal test error");
			return tcu::ConstPixelBufferAccess();
	}
}

//! \return true if all layers match in both pixel buffers
bool comparePixelBuffers (tcu::TestLog&						log,
						  const Texture&					texture,
						  const VkFormat					format,
						  const tcu::ConstPixelBufferAccess	reference,
						  const tcu::ConstPixelBufferAccess	result)
{
	DE_ASSERT(reference.getFormat() == result.getFormat());
	DE_ASSERT(reference.getSize() == result.getSize());

	const bool intFormat = isIntegerFormat(format);
	const bool is3d = (texture.type() == IMAGE_TYPE_3D);
	const int numLayersOrSlices = (is3d ? texture.size().z() : texture.numLayers());
	const int numCubeFaces = 6;

	int passedLayers = 0;
	for (int layerNdx = 0; layerNdx < numLayersOrSlices; ++layerNdx)
	{
		const std::string comparisonName = "Comparison" + de::toString(layerNdx);
		const std::string comparisonDesc = "Image Comparison, " +
			(isCube(texture) ? "face " + de::toString(layerNdx % numCubeFaces) + ", cube " + de::toString(layerNdx / numCubeFaces) :
			is3d			 ? "slice " + de::toString(layerNdx) : "layer " + de::toString(layerNdx));

		const tcu::ConstPixelBufferAccess refLayer = getLayerOrSlice(texture, reference, layerNdx);
		const tcu::ConstPixelBufferAccess resultLayer = getLayerOrSlice(texture, result, layerNdx);

		bool ok = false;
		if (intFormat)
			ok = tcu::intThresholdCompare(log, comparisonName.c_str(), comparisonDesc.c_str(), refLayer, resultLayer, tcu::UVec4(0), tcu::COMPARE_LOG_RESULT);
		else
			ok = tcu::floatThresholdCompare(log, comparisonName.c_str(), comparisonDesc.c_str(), refLayer, resultLayer, tcu::Vec4(0.01f), tcu::COMPARE_LOG_RESULT);

		if (ok)
			++passedLayers;
	}
	return passedLayers == numLayersOrSlices;
}

//!< Zero out invalid pixels in the image (denormalized, infinite, NaN values)
void replaceBadFloatReinterpretValues (const tcu::PixelBufferAccess access)
{
	DE_ASSERT(tcu::getTextureChannelClass(access.getFormat().type) == tcu::TEXTURECHANNELCLASS_FLOATING_POINT);

	for (int z = 0; z < access.getDepth(); ++z)
	for (int y = 0; y < access.getHeight(); ++y)
	for (int x = 0; x < access.getWidth(); ++x)
	{
		const tcu::Vec4 color(access.getPixel(x, y, z));
		tcu::Vec4 newColor = color;

		for (int i = 0; i < 4; ++i)
		{
			if (access.getFormat().type == tcu::TextureFormat::HALF_FLOAT)
			{
				const tcu::Float16 f(color[i]);
				if (f.isDenorm() || f.isInf() || f.isNaN())
					newColor[i] = 0.0f;
			}
			else
			{
				const tcu::Float32 f(color[i]);
				if (f.isDenorm() || f.isInf() || f.isNaN())
					newColor[i] = 0.0f;
			}
		}

		if (newColor != color)
			access.setPixel(newColor, x, y, z);
	}
}

//!< replace invalid pixels in the image (-128)
void replaceSnormReinterpretValues (const tcu::PixelBufferAccess access)
{
	DE_ASSERT(tcu::getTextureChannelClass(access.getFormat().type) == tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT);

	for (int z = 0; z < access.getDepth(); ++z)
	for (int y = 0; y < access.getHeight(); ++y)
	for (int x = 0; x < access.getWidth(); ++x)
	{
		const tcu::IVec4 color(access.getPixelInt(x, y, z));
		tcu::IVec4 newColor = color;

		for (int i = 0; i < 4; ++i)
		{
			const deInt32 oldColor(color[i]);
			if (oldColor == -128) newColor[i] = -127;
		}

		if (newColor != color)
		access.setPixel(newColor, x, y, z);
	}
}

tcu::TextureLevel generateReferenceImage (const tcu::IVec3& imageSize, const VkFormat imageFormat, const VkFormat readFormat)
{
	// Generate a reference image data using the storage format

	tcu::TextureLevel reference(mapVkFormat(imageFormat), imageSize.x(), imageSize.y(), imageSize.z());
	const tcu::PixelBufferAccess access = reference.getAccess();

	const float storeColorScale = computeStoreColorScale(imageFormat, imageSize);
	const float storeColorBias = computeStoreColorBias(imageFormat);

	const bool intFormat = isIntegerFormat(imageFormat);
	const int xMax = imageSize.x() - 1;
	const int yMax = imageSize.y() - 1;

	for (int z = 0; z < imageSize.z(); ++z)
	for (int y = 0; y < imageSize.y(); ++y)
	for (int x = 0; x < imageSize.x(); ++x)
	{
		const tcu::IVec4 color(x^y^z, (xMax - x)^y^z, x^(yMax - y)^z, (xMax - x)^(yMax - y)^z);

		if (intFormat)
			access.setPixel(color, x, y, z);
		else
			access.setPixel(color.asFloat()*storeColorScale + storeColorBias, x, y, z);
	}

	// If the image is to be accessed as a float texture, get rid of invalid values

	if (isFloatFormat(readFormat) && imageFormat != readFormat)
		replaceBadFloatReinterpretValues(tcu::PixelBufferAccess(mapVkFormat(readFormat), imageSize, access.getDataPtr()));
	if (isSnormFormat(readFormat) && imageFormat != readFormat)
		replaceSnormReinterpretValues(tcu::PixelBufferAccess(mapVkFormat(readFormat), imageSize, access.getDataPtr()));

	return reference;
}

inline tcu::TextureLevel generateReferenceImage (const tcu::IVec3& imageSize, const VkFormat imageFormat)
{
	return generateReferenceImage(imageSize, imageFormat, imageFormat);
}

void flipHorizontally (const tcu::PixelBufferAccess access)
{
	const int xMax = access.getWidth() - 1;
	const int halfWidth = access.getWidth() / 2;

	if (isIntegerFormat(mapTextureFormat(access.getFormat())))
		for (int z = 0; z < access.getDepth(); z++)
		for (int y = 0; y < access.getHeight(); y++)
		for (int x = 0; x < halfWidth; x++)
		{
			const tcu::UVec4 temp = access.getPixelUint(xMax - x, y, z);
			access.setPixel(access.getPixelUint(x, y, z), xMax - x, y, z);
			access.setPixel(temp, x, y, z);
		}
	else
		for (int z = 0; z < access.getDepth(); z++)
		for (int y = 0; y < access.getHeight(); y++)
		for (int x = 0; x < halfWidth; x++)
		{
			const tcu::Vec4 temp = access.getPixel(xMax - x, y, z);
			access.setPixel(access.getPixel(x, y, z), xMax - x, y, z);
			access.setPixel(temp, x, y, z);
		}
}

inline bool formatsAreCompatible (const VkFormat format0, const VkFormat format1)
{
	return format0 == format1 || mapVkFormat(format0).getPixelSize() == mapVkFormat(format1).getPixelSize();
}

void commandImageWriteBarrierBetweenShaderInvocations (Context& context, const VkCommandBuffer cmdBuffer, const VkImage image, const Texture& texture)
{
	const DeviceInterface& vk = context.getDeviceInterface();

	const VkImageSubresourceRange fullImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, texture.numLayers());
	const VkImageMemoryBarrier shaderWriteBarrier = makeImageMemoryBarrier(
		VK_ACCESS_SHADER_WRITE_BIT, 0u,
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
		image, fullImageSubresourceRange);

	vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &shaderWriteBarrier);
}

void commandBufferWriteBarrierBeforeHostRead (Context& context, const VkCommandBuffer cmdBuffer, const VkBuffer buffer, const VkDeviceSize bufferSizeBytes)
{
	const DeviceInterface& vk = context.getDeviceInterface();

	const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(
		VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
		buffer, 0ull, bufferSizeBytes);

	vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &shaderWriteBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
}

//! Copy all layers of an image to a buffer.
void commandCopyImageToBuffer (Context&					context,
							   const VkCommandBuffer	cmdBuffer,
							   const VkImage			image,
							   const VkBuffer			buffer,
							   const VkDeviceSize		bufferSizeBytes,
							   const Texture&			texture)
{
	const DeviceInterface& vk = context.getDeviceInterface();

	const VkImageSubresourceRange fullImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, texture.numLayers());
	const VkImageMemoryBarrier prepareForTransferBarrier = makeImageMemoryBarrier(
		VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		image, fullImageSubresourceRange);

	const VkBufferImageCopy copyRegion = makeBufferImageCopy(texture);

	const VkBufferMemoryBarrier copyBarrier = makeBufferMemoryBarrier(
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
		buffer, 0ull, bufferSizeBytes);

	vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &prepareForTransferBarrier);
	vk.cmdCopyImageToBuffer(cmdBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1u, &copyRegion);
	vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &copyBarrier, 0, (const VkImageMemoryBarrier*)DE_NULL);
}

class StoreTest : public TestCase
{
public:
	enum TestFlags
	{
		FLAG_SINGLE_LAYER_BIND				= 0x1,	//!< Run the shader multiple times, each time binding a different layer.
		FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER	= 0x2,	//!< Declare the format of the images in the shader code
	};

							StoreTest			(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const Texture&		texture,
												 const VkFormat		format,
												 const deUint32		flags = FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER);

	void					initPrograms		(SourceCollections& programCollection) const;

	TestInstance*			createInstance		(Context&			context) const;

private:
	const Texture			m_texture;
	const VkFormat			m_format;
	const bool				m_declareImageFormatInShader;
	const bool				m_singleLayerBind;
};

StoreTest::StoreTest (tcu::TestContext&		testCtx,
					  const std::string&	name,
					  const std::string&	description,
					  const Texture&		texture,
					  const VkFormat		format,
					  const deUint32		flags)
	: TestCase						(testCtx, name, description)
	, m_texture						(texture)
	, m_format						(format)
	, m_declareImageFormatInShader	((flags & FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER) != 0)
	, m_singleLayerBind				((flags & FLAG_SINGLE_LAYER_BIND) != 0)
{
	if (m_singleLayerBind)
		DE_ASSERT(m_texture.numLayers() > 1);
}

void StoreTest::initPrograms (SourceCollections& programCollection) const
{
	const float storeColorScale = computeStoreColorScale(m_format, m_texture.size());
	const float storeColorBias = computeStoreColorBias(m_format);
	DE_ASSERT(colorScaleAndBiasAreValid(m_format, storeColorScale, storeColorBias));

	const std::string xMax = de::toString(m_texture.size().x() - 1);
	const std::string yMax = de::toString(m_texture.size().y() - 1);
	const std::string signednessPrefix = isUintFormat(m_format) ? "u" : isIntFormat(m_format) ? "i" : "";
	const std::string colorBaseExpr = signednessPrefix + "vec4("
		+ "gx^gy^gz, "
		+ "(" + xMax + "-gx)^gy^gz, "
		+ "gx^(" + yMax + "-gy)^gz, "
		+ "(" + xMax + "-gx)^(" + yMax + "-gy)^gz)";

	const std::string colorExpr = colorBaseExpr + (storeColorScale == 1.0f ? "" : "*" + de::toString(storeColorScale))
								  + (storeColorBias == 0.0f ? "" : " + float(" + de::toString(storeColorBias) + ")");

	const int dimension = (m_singleLayerBind ? m_texture.layerDimension() : m_texture.dimension());
	const std::string texelCoordStr = (dimension == 1 ? "gx" : dimension == 2 ? "ivec2(gx, gy)" : dimension == 3 ? "ivec3(gx, gy, gz)" : "");

	const ImageType usedImageType = (m_singleLayerBind ? getImageTypeForSingleLayer(m_texture.type()) : m_texture.type());
	const std::string formatQualifierStr = getShaderImageFormatQualifier(mapVkFormat(m_format));
	const std::string imageTypeStr = getShaderImageType(mapVkFormat(m_format), usedImageType);

	for (deUint32 variant = 0; variant <= 1; variant++)
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_440) << "\n"
			<< "\n"
			<< "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n";
		if (variant == 0)
			src << "layout (binding = 0, " << formatQualifierStr << ") writeonly uniform " << imageTypeStr << " u_image;\n";
		else
			src << "layout (binding = 0) writeonly uniform " << imageTypeStr << " u_image;\n";

		if (m_singleLayerBind)
			src << "layout (binding = 1) readonly uniform Constants {\n"
				<< "    int u_layerNdx;\n"
				<< "};\n";

		src << "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    int gx = int(gl_GlobalInvocationID.x);\n"
			<< "    int gy = int(gl_GlobalInvocationID.y);\n"
			<< "    int gz = " << (m_singleLayerBind ? "u_layerNdx" : "int(gl_GlobalInvocationID.z)") << ";\n"
			<< "    imageStore(u_image, " << texelCoordStr << ", " << colorExpr << ");\n"
			<< "}\n";

		programCollection.glslSources.add(variant == 0 ? "comp" : "comp_fmt_unknown") << glu::ComputeSource(src.str());
	}
}

//! Generic test iteration algorithm for image tests
class BaseTestInstance : public TestInstance
{
public:
									BaseTestInstance						(Context&		context,
																			 const Texture&	texture,
																			 const VkFormat	format,
																			 const bool		declareImageFormatInShader,
																			 const bool		singleLayerBind);

	tcu::TestStatus                 iterate									(void);

	virtual							~BaseTestInstance						(void) {}

protected:
	virtual VkDescriptorSetLayout	prepareDescriptors						(void) = 0;
	virtual tcu::TestStatus			verifyResult							(void) = 0;

	virtual void					commandBeforeCompute					(const VkCommandBuffer	cmdBuffer) = 0;
	virtual void					commandBetweenShaderInvocations			(const VkCommandBuffer	cmdBuffer) = 0;
	virtual void					commandAfterCompute						(const VkCommandBuffer	cmdBuffer) = 0;

	virtual void					commandBindDescriptorsForLayer			(const VkCommandBuffer	cmdBuffer,
																			 const VkPipelineLayout pipelineLayout,
																			 const int				layerNdx) = 0;
	virtual void					checkRequirements						(void) {};

	const Texture					m_texture;
	const VkFormat					m_format;
	const bool						m_declareImageFormatInShader;
	const bool						m_singleLayerBind;
};

BaseTestInstance::BaseTestInstance (Context& context, const Texture& texture, const VkFormat format, const bool declareImageFormatInShader, const bool singleLayerBind)
	: TestInstance					(context)
	, m_texture						(texture)
	, m_format						(format)
	, m_declareImageFormatInShader	(declareImageFormatInShader)
	, m_singleLayerBind				(singleLayerBind)
{
}

tcu::TestStatus BaseTestInstance::iterate (void)
{
	checkRequirements();

	const DeviceInterface&			vk					= m_context.getDeviceInterface();
	const VkDevice					device				= m_context.getDevice();
	const VkQueue					queue				= m_context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get(m_declareImageFormatInShader ? "comp" : "comp_fmt_unknown"), 0));

	const VkDescriptorSetLayout descriptorSetLayout = prepareDescriptors();
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, descriptorSetLayout));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shaderModule));

	const Unique<VkCommandPool> cmdPool(createCommandPool(vk, device, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	commandBeforeCompute(*cmdBuffer);

	const tcu::IVec3 workSize = (m_singleLayerBind ? m_texture.layerSize() : m_texture.size());
	const int loopNumLayers = (m_singleLayerBind ? m_texture.numLayers() : 1);
	for (int layerNdx = 0; layerNdx < loopNumLayers; ++layerNdx)
	{
		commandBindDescriptorsForLayer(*cmdBuffer, *pipelineLayout, layerNdx);

		if (layerNdx > 0)
			commandBetweenShaderInvocations(*cmdBuffer);

		vk.cmdDispatch(*cmdBuffer, workSize.x(), workSize.y(), workSize.z());
	}

	commandAfterCompute(*cmdBuffer);

	endCommandBuffer(vk, *cmdBuffer);

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	return verifyResult();
}

//! Base store test implementation
class StoreTestInstance : public BaseTestInstance
{
public:
									StoreTestInstance						(Context&		context,
																			 const Texture&	texture,
																			 const VkFormat	format,
																			 const bool		declareImageFormatInShader,
																			 const bool		singleLayerBind);

protected:
	tcu::TestStatus					verifyResult							(void);

	// Add empty implementations for functions that might be not needed
	void							commandBeforeCompute					(const VkCommandBuffer) {}
	void							commandBetweenShaderInvocations			(const VkCommandBuffer) {}
	void							commandAfterCompute						(const VkCommandBuffer) {}
	void							checkRequirements						(void);

	de::MovePtr<Buffer>				m_imageBuffer;
	const VkDeviceSize				m_imageSizeBytes;
};

StoreTestInstance::StoreTestInstance (Context& context, const Texture& texture, const VkFormat format, const bool declareImageFormatInShader, const bool singleLayerBind)
	: BaseTestInstance		(context, texture, format, declareImageFormatInShader, singleLayerBind)
	, m_imageSizeBytes		(getImageSizeBytes(texture.size(), format))
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	const VkDevice			device		= m_context.getDevice();
	Allocator&				allocator	= m_context.getDefaultAllocator();

	// A helper buffer with enough space to hold the whole image. Usage flags accommodate all derived test instances.

	m_imageBuffer = de::MovePtr<Buffer>(new Buffer(
		vk, device, allocator,
		makeBufferCreateInfo(m_imageSizeBytes, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
		MemoryRequirement::HostVisible));
}

tcu::TestStatus StoreTestInstance::verifyResult	(void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	const tcu::IVec3 imageSize = m_texture.size();
	const tcu::TextureLevel reference = generateReferenceImage(imageSize, m_format);

	const Allocation& alloc = m_imageBuffer->getAllocation();
	invalidateMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), m_imageSizeBytes);
	const tcu::ConstPixelBufferAccess result(mapVkFormat(m_format), imageSize, alloc.getHostPtr());

	if (comparePixelBuffers(m_context.getTestContext().getLog(), m_texture, m_format, reference.getAccess(), result))
		return tcu::TestStatus::pass("Passed");
	else
		return tcu::TestStatus::fail("Image comparison failed");
}

void StoreTestInstance::checkRequirements (void)
{
	const VkPhysicalDeviceFeatures	features	= m_context.getDeviceFeatures();

	if (!m_declareImageFormatInShader && !features.shaderStorageImageWriteWithoutFormat)
		throw tcu::NotSupportedError("shaderStorageImageWriteWithoutFormat feature not supported");
}

//! Store test for images
class ImageStoreTestInstance : public StoreTestInstance
{
public:
										ImageStoreTestInstance					(Context&				context,
																				 const Texture&			texture,
																				 const VkFormat			format,
																				 const bool				declareImageFormatInShader,
																				 const bool				singleLayerBind);

protected:
	VkDescriptorSetLayout				prepareDescriptors						(void);
	void								commandBeforeCompute					(const VkCommandBuffer	cmdBuffer);
	void								commandBetweenShaderInvocations			(const VkCommandBuffer	cmdBuffer);
	void								commandAfterCompute						(const VkCommandBuffer	cmdBuffer);

	void								commandBindDescriptorsForLayer			(const VkCommandBuffer	cmdBuffer,
																				 const VkPipelineLayout pipelineLayout,
																				 const int				layerNdx);

	de::MovePtr<Image>					m_image;
	de::MovePtr<Buffer>					m_constantsBuffer;
	const VkDeviceSize					m_constantsBufferChunkSizeBytes;
	Move<VkDescriptorSetLayout>			m_descriptorSetLayout;
	Move<VkDescriptorPool>				m_descriptorPool;
	std::vector<SharedVkDescriptorSet>	m_allDescriptorSets;
	std::vector<SharedVkImageView>		m_allImageViews;
};

ImageStoreTestInstance::ImageStoreTestInstance (Context&		context,
												const Texture&	texture,
												const VkFormat	format,
												const bool		declareImageFormatInShader,
												const bool		singleLayerBind)
	: StoreTestInstance					(context, texture, format, declareImageFormatInShader, singleLayerBind)
	, m_constantsBufferChunkSizeBytes	(getOptimalUniformBufferChunkSize(context.getInstanceInterface(), context.getPhysicalDevice(), sizeof(deUint32)))
	, m_allDescriptorSets				(texture.numLayers())
	, m_allImageViews					(texture.numLayers())
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	m_image = de::MovePtr<Image>(new Image(
		vk, device, allocator,
		makeImageCreateInfo(m_texture, m_format, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0u),
		MemoryRequirement::Any));

	// This buffer will be used to pass constants to the shader

	const int numLayers = m_texture.numLayers();
	const VkDeviceSize constantsBufferSizeBytes = numLayers * m_constantsBufferChunkSizeBytes;
	m_constantsBuffer = de::MovePtr<Buffer>(new Buffer(
		vk, device, allocator,
		makeBufferCreateInfo(constantsBufferSizeBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
		MemoryRequirement::HostVisible));

	{
		const Allocation& alloc = m_constantsBuffer->getAllocation();
		deUint8* const basePtr = static_cast<deUint8*>(alloc.getHostPtr());

		deMemset(alloc.getHostPtr(), 0, static_cast<size_t>(constantsBufferSizeBytes));

		for (int layerNdx = 0; layerNdx < numLayers; ++layerNdx)
		{
			deUint32* valuePtr = reinterpret_cast<deUint32*>(basePtr + layerNdx * m_constantsBufferChunkSizeBytes);
			*valuePtr = static_cast<deUint32>(layerNdx);
		}

		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), constantsBufferSizeBytes);
	}
}

VkDescriptorSetLayout ImageStoreTestInstance::prepareDescriptors (void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	const int numLayers = m_texture.numLayers();
	m_descriptorSetLayout = DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device);

	m_descriptorPool = DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numLayers)
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numLayers)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, numLayers);

	if (m_singleLayerBind)
	{
		for (int layerNdx = 0; layerNdx < numLayers; ++layerNdx)
		{
			m_allDescriptorSets[layerNdx] = makeVkSharedPtr(makeDescriptorSet(vk, device, *m_descriptorPool, *m_descriptorSetLayout));
			m_allImageViews[layerNdx]     = makeVkSharedPtr(makeImageView(
												vk, device, m_image->get(), mapImageViewType(getImageTypeForSingleLayer(m_texture.type())), m_format,
												makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, layerNdx, 1u)));
		}
	}
	else // bind all layers at once
	{
		m_allDescriptorSets[0] = makeVkSharedPtr(makeDescriptorSet(vk, device, *m_descriptorPool, *m_descriptorSetLayout));
		m_allImageViews[0] = makeVkSharedPtr(makeImageView(
								vk, device, m_image->get(), mapImageViewType(m_texture.type()), m_format,
								makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, numLayers)));
	}

	return *m_descriptorSetLayout;  // not passing the ownership
}

void ImageStoreTestInstance::commandBindDescriptorsForLayer (const VkCommandBuffer cmdBuffer, const VkPipelineLayout pipelineLayout, const int layerNdx)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	const VkDescriptorSet descriptorSet = **m_allDescriptorSets[layerNdx];
	const VkImageView imageView = **m_allImageViews[layerNdx];

	const VkDescriptorImageInfo descriptorImageInfo = makeDescriptorImageInfo(DE_NULL, imageView, VK_IMAGE_LAYOUT_GENERAL);

	// Set the next chunk of the constants buffer. Each chunk begins with layer index that we've set before.
	const VkDescriptorBufferInfo descriptorConstantsBufferInfo = makeDescriptorBufferInfo(
		m_constantsBuffer->get(), layerNdx*m_constantsBufferChunkSizeBytes, m_constantsBufferChunkSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorImageInfo)
		.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorConstantsBufferInfo)
		.update(vk, device);
	vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);
}

void ImageStoreTestInstance::commandBeforeCompute (const VkCommandBuffer cmdBuffer)
{
	const DeviceInterface& vk = m_context.getDeviceInterface();

	const VkImageSubresourceRange fullImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, m_texture.numLayers());
	const VkImageMemoryBarrier setImageLayoutBarrier = makeImageMemoryBarrier(
		0u, 0u,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		m_image->get(), fullImageSubresourceRange);

	const VkDeviceSize constantsBufferSize = m_texture.numLayers() * m_constantsBufferChunkSizeBytes;
	const VkBufferMemoryBarrier writeConstantsBarrier = makeBufferMemoryBarrier(
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		m_constantsBuffer->get(), 0ull, constantsBufferSize);

	vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &writeConstantsBarrier, 1, &setImageLayoutBarrier);
}

void ImageStoreTestInstance::commandBetweenShaderInvocations (const VkCommandBuffer cmdBuffer)
{
	commandImageWriteBarrierBetweenShaderInvocations(m_context, cmdBuffer, m_image->get(), m_texture);
}

void ImageStoreTestInstance::commandAfterCompute (const VkCommandBuffer cmdBuffer)
{
	commandCopyImageToBuffer(m_context, cmdBuffer, m_image->get(), m_imageBuffer->get(), m_imageSizeBytes, m_texture);
}

//! Store test for buffers
class BufferStoreTestInstance : public StoreTestInstance
{
public:
									BufferStoreTestInstance					(Context&				context,
																			 const Texture&			texture,
																			 const VkFormat			format,
																			 const bool				declareImageFormatInShader);

protected:
	VkDescriptorSetLayout			prepareDescriptors						(void);
	void							commandAfterCompute						(const VkCommandBuffer	cmdBuffer);

	void							commandBindDescriptorsForLayer			(const VkCommandBuffer	cmdBuffer,
																			 const VkPipelineLayout pipelineLayout,
																			 const int				layerNdx);

	Move<VkDescriptorSetLayout>		m_descriptorSetLayout;
	Move<VkDescriptorPool>			m_descriptorPool;
	Move<VkDescriptorSet>			m_descriptorSet;
	Move<VkBufferView>				m_bufferView;
};

BufferStoreTestInstance::BufferStoreTestInstance (Context&			context,
												  const Texture&	texture,
												  const VkFormat	format,
												  const bool		declareImageFormatInShader)
	: StoreTestInstance(context, texture, format, declareImageFormatInShader, false)
{
}

VkDescriptorSetLayout BufferStoreTestInstance::prepareDescriptors (void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	m_descriptorSetLayout = DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device);

	m_descriptorPool = DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	m_descriptorSet = makeDescriptorSet(vk, device, *m_descriptorPool, *m_descriptorSetLayout);
	m_bufferView = makeBufferView(vk, device, m_imageBuffer->get(), m_format, 0ull, m_imageSizeBytes);

	return *m_descriptorSetLayout;  // not passing the ownership
}

void BufferStoreTestInstance::commandBindDescriptorsForLayer (const VkCommandBuffer cmdBuffer, const VkPipelineLayout pipelineLayout, const int layerNdx)
{
	DE_ASSERT(layerNdx == 0);
	DE_UNREF(layerNdx);

	const VkDevice			device	= m_context.getDevice();
	const DeviceInterface&	vk		= m_context.getDeviceInterface();

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, &m_bufferView.get())
		.update(vk, device);
	vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);
}

void BufferStoreTestInstance::commandAfterCompute (const VkCommandBuffer cmdBuffer)
{
	commandBufferWriteBarrierBeforeHostRead(m_context, cmdBuffer, m_imageBuffer->get(), m_imageSizeBytes);
}

class LoadStoreTest : public TestCase
{
public:
	enum TestFlags
	{
		FLAG_SINGLE_LAYER_BIND				= 1 << 0,	//!< Run the shader multiple times, each time binding a different layer.
		FLAG_RESTRICT_IMAGES				= 1 << 1,	//!< If given, images in the shader will be qualified with "restrict".
		FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER	= 1 << 2,	//!< Declare the format of the images in the shader code
	};

							LoadStoreTest			(tcu::TestContext&		testCtx,
													 const std::string&		name,
													 const std::string&		description,
													 const Texture&			texture,
													 const VkFormat			format,
													 const VkFormat			imageFormat,
													 const deUint32			flags = FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER);

	void					initPrograms			(SourceCollections&		programCollection) const;
	TestInstance*			createInstance			(Context&				context) const;

private:
	const Texture			m_texture;
	const VkFormat			m_format;						//!< Format as accessed in the shader
	const VkFormat			m_imageFormat;					//!< Storage format
	const bool				m_declareImageFormatInShader;	//!< Whether the shader will specify the format layout qualifier of the images
	const bool				m_singleLayerBind;
	const bool				m_restrictImages;
};

LoadStoreTest::LoadStoreTest (tcu::TestContext&		testCtx,
							  const std::string&	name,
							  const std::string&	description,
							  const Texture&		texture,
							  const VkFormat		format,
							  const VkFormat		imageFormat,
							  const deUint32		flags)
	: TestCase						(testCtx, name, description)
	, m_texture						(texture)
	, m_format						(format)
	, m_imageFormat					(imageFormat)
	, m_declareImageFormatInShader	((flags & FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER) != 0)
	, m_singleLayerBind				((flags & FLAG_SINGLE_LAYER_BIND) != 0)
	, m_restrictImages				((flags & FLAG_RESTRICT_IMAGES) != 0)
{
	if (m_singleLayerBind)
		DE_ASSERT(m_texture.numLayers() > 1);

	DE_ASSERT(formatsAreCompatible(m_format, m_imageFormat));
}

void LoadStoreTest::initPrograms (SourceCollections& programCollection) const
{
	const int			dimension			= (m_singleLayerBind ? m_texture.layerDimension() : m_texture.dimension());
	const ImageType		usedImageType		= (m_singleLayerBind ? getImageTypeForSingleLayer(m_texture.type()) : m_texture.type());
	const std::string	formatQualifierStr	= getShaderImageFormatQualifier(mapVkFormat(m_format));
	const std::string	imageTypeStr		= getShaderImageType(mapVkFormat(m_format), usedImageType);
	const std::string	maybeRestrictStr	= (m_restrictImages ? "restrict " : "");
	const std::string	xMax				= de::toString(m_texture.size().x() - 1);

	for (deUint32 variant = 0; variant <= 1; variant++)
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_440) << "\n"
			<< "\n";
		if (variant != 0)
		{
			src << "#extension GL_EXT_shader_image_load_formatted : require\n";
		}
		src << "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n";
		if (variant == 0)
			src << "layout (binding = 0, " << formatQualifierStr << ") " << maybeRestrictStr << "readonly uniform " << imageTypeStr << " u_image0;\n";
		else
			src << "layout (binding = 0) " << maybeRestrictStr << "readonly uniform " << imageTypeStr << " u_image0;\n";
		src << "layout (binding = 1, " << formatQualifierStr << ") " << maybeRestrictStr << "writeonly uniform " << imageTypeStr << " u_image1;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< (dimension == 1 ?
				"    int pos = int(gl_GlobalInvocationID.x);\n"
				"    imageStore(u_image1, pos, imageLoad(u_image0, " + xMax + "-pos));\n"
				: dimension == 2 ?
				"    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
				"    imageStore(u_image1, pos, imageLoad(u_image0, ivec2(" + xMax + "-pos.x, pos.y)));\n"
				: dimension == 3 ?
				"    ivec3 pos = ivec3(gl_GlobalInvocationID);\n"
				"    imageStore(u_image1, pos, imageLoad(u_image0, ivec3(" + xMax + "-pos.x, pos.y, pos.z)));\n"
				: "")
			<< "}\n";

		programCollection.glslSources.add(variant == 0 ? "comp" : "comp_fmt_unknown") << glu::ComputeSource(src.str());
	}
}

//! Load/store test base implementation
class LoadStoreTestInstance : public BaseTestInstance
{
public:
									LoadStoreTestInstance				(Context&			context,
																		 const Texture&		texture,
																		 const VkFormat		format,
																		 const VkFormat		imageFormat,
																		 const bool			declareImageFormatInShader,
																		 const bool			singleLayerBind);

protected:
	virtual Buffer*					getResultBuffer						(void) const = 0;	//!< Get the buffer that contains the result image

	tcu::TestStatus					verifyResult						(void);

	// Add empty implementations for functions that might be not needed
	void							commandBeforeCompute				(const VkCommandBuffer) {}
	void							commandBetweenShaderInvocations		(const VkCommandBuffer) {}
	void							commandAfterCompute					(const VkCommandBuffer) {}
	void							checkRequirements					(void);

	de::MovePtr<Buffer>				m_imageBuffer;		//!< Source data and helper buffer
	const VkDeviceSize				m_imageSizeBytes;
	const VkFormat					m_imageFormat;		//!< Image format (for storage, may be different than texture format)
	tcu::TextureLevel				m_referenceImage;	//!< Used as input data and later to verify result image
};

LoadStoreTestInstance::LoadStoreTestInstance (Context&			context,
											  const Texture&	texture,
											  const VkFormat	format,
											  const VkFormat	imageFormat,
											  const bool		declareImageFormatInShader,
											  const bool		singleLayerBind)
	: BaseTestInstance		(context, texture, format, declareImageFormatInShader, singleLayerBind)
	, m_imageSizeBytes		(getImageSizeBytes(texture.size(), format))
	, m_imageFormat			(imageFormat)
	, m_referenceImage		(generateReferenceImage(texture.size(), imageFormat, format))
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	const VkDevice			device		= m_context.getDevice();
	Allocator&				allocator	= m_context.getDefaultAllocator();

	// A helper buffer with enough space to hold the whole image.

	m_imageBuffer = de::MovePtr<Buffer>(new Buffer(
		vk, device, allocator,
		makeBufferCreateInfo(m_imageSizeBytes, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
		MemoryRequirement::HostVisible));

	// Copy reference data to buffer for subsequent upload to image.

	const Allocation& alloc = m_imageBuffer->getAllocation();
	deMemcpy(alloc.getHostPtr(), m_referenceImage.getAccess().getDataPtr(), static_cast<size_t>(m_imageSizeBytes));
	flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), m_imageSizeBytes);
}

tcu::TestStatus LoadStoreTestInstance::verifyResult	(void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	// Apply the same transformation as done in the shader
	const tcu::PixelBufferAccess reference = m_referenceImage.getAccess();
	flipHorizontally(reference);

	const Allocation& alloc = getResultBuffer()->getAllocation();
	invalidateMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), m_imageSizeBytes);
	const tcu::ConstPixelBufferAccess result(mapVkFormat(m_imageFormat), m_texture.size(), alloc.getHostPtr());

	if (comparePixelBuffers(m_context.getTestContext().getLog(), m_texture, m_imageFormat, reference, result))
		return tcu::TestStatus::pass("Passed");
	else
		return tcu::TestStatus::fail("Image comparison failed");
}

void LoadStoreTestInstance::checkRequirements (void)
{
	const VkPhysicalDeviceFeatures	features	= m_context.getDeviceFeatures();

	if (!m_declareImageFormatInShader && !features.shaderStorageImageReadWithoutFormat)
		throw tcu::NotSupportedError("shaderStorageImageReadWithoutFormat feature not supported");
}


//! Load/store test for images
class ImageLoadStoreTestInstance : public LoadStoreTestInstance
{
public:
										ImageLoadStoreTestInstance			(Context&				context,
																			 const Texture&			texture,
																			 const VkFormat			format,
																			 const VkFormat			imageFormat,
																			 const bool				declareImageFormatInShader,
																			 const bool				singleLayerBind);

protected:
	VkDescriptorSetLayout				prepareDescriptors					(void);
	void								commandBeforeCompute				(const VkCommandBuffer	cmdBuffer);
	void								commandBetweenShaderInvocations		(const VkCommandBuffer	cmdBuffer);
	void								commandAfterCompute					(const VkCommandBuffer	cmdBuffer);

	void								commandBindDescriptorsForLayer		(const VkCommandBuffer	cmdBuffer,
																			 const VkPipelineLayout pipelineLayout,
																			 const int				layerNdx);

	Buffer*								getResultBuffer						(void) const { return m_imageBuffer.get(); }

	de::MovePtr<Image>					m_imageSrc;
	de::MovePtr<Image>					m_imageDst;
	Move<VkDescriptorSetLayout>			m_descriptorSetLayout;
	Move<VkDescriptorPool>				m_descriptorPool;
	std::vector<SharedVkDescriptorSet>	m_allDescriptorSets;
	std::vector<SharedVkImageView>		m_allSrcImageViews;
	std::vector<SharedVkImageView>		m_allDstImageViews;
};

ImageLoadStoreTestInstance::ImageLoadStoreTestInstance (Context&		context,
														const Texture&	texture,
														const VkFormat	format,
														const VkFormat	imageFormat,
														const bool		declareImageFormatInShader,
														const bool		singleLayerBind)
	: LoadStoreTestInstance	(context, texture, format, imageFormat, declareImageFormatInShader, singleLayerBind)
	, m_allDescriptorSets	(texture.numLayers())
	, m_allSrcImageViews	(texture.numLayers())
	, m_allDstImageViews	(texture.numLayers())
{
	const DeviceInterface&		vk					= m_context.getDeviceInterface();
	const VkDevice				device				= m_context.getDevice();
	Allocator&					allocator			= m_context.getDefaultAllocator();
	const VkImageCreateFlags	imageFlags			= (m_format == m_imageFormat ? 0u : (VkImageCreateFlags)VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT);

	m_imageSrc = de::MovePtr<Image>(new Image(
		vk, device, allocator,
		makeImageCreateInfo(m_texture, m_imageFormat, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageFlags),
		MemoryRequirement::Any));

	m_imageDst = de::MovePtr<Image>(new Image(
		vk, device, allocator,
		makeImageCreateInfo(m_texture, m_imageFormat, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, imageFlags),
		MemoryRequirement::Any));
}

VkDescriptorSetLayout ImageLoadStoreTestInstance::prepareDescriptors (void)
{
	const VkDevice			device	= m_context.getDevice();
	const DeviceInterface&	vk		= m_context.getDeviceInterface();

	const int numLayers = m_texture.numLayers();
	m_descriptorSetLayout = DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device);

	m_descriptorPool = DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numLayers)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numLayers)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, numLayers);

	if (m_singleLayerBind)
	{
		for (int layerNdx = 0; layerNdx < numLayers; ++layerNdx)
		{
			const VkImageViewType viewType = mapImageViewType(getImageTypeForSingleLayer(m_texture.type()));
			const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, layerNdx, 1u);

			m_allDescriptorSets[layerNdx] = makeVkSharedPtr(makeDescriptorSet(vk, device, *m_descriptorPool, *m_descriptorSetLayout));
			m_allSrcImageViews[layerNdx]  = makeVkSharedPtr(makeImageView(vk, device, m_imageSrc->get(), viewType, m_format, subresourceRange));
			m_allDstImageViews[layerNdx]  = makeVkSharedPtr(makeImageView(vk, device, m_imageDst->get(), viewType, m_format, subresourceRange));
		}
	}
	else // bind all layers at once
	{
		const VkImageViewType viewType = mapImageViewType(m_texture.type());
		const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, numLayers);

		m_allDescriptorSets[0] = makeVkSharedPtr(makeDescriptorSet(vk, device, *m_descriptorPool, *m_descriptorSetLayout));
		m_allSrcImageViews[0]  = makeVkSharedPtr(makeImageView(vk, device, m_imageSrc->get(), viewType, m_format, subresourceRange));
		m_allDstImageViews[0]  = makeVkSharedPtr(makeImageView(vk, device, m_imageDst->get(), viewType, m_format, subresourceRange));
	}

	return *m_descriptorSetLayout;  // not passing the ownership
}

void ImageLoadStoreTestInstance::commandBindDescriptorsForLayer (const VkCommandBuffer cmdBuffer, const VkPipelineLayout pipelineLayout, const int layerNdx)
{
	const VkDevice			device	= m_context.getDevice();
	const DeviceInterface&	vk		= m_context.getDeviceInterface();

	const VkDescriptorSet descriptorSet = **m_allDescriptorSets[layerNdx];
	const VkImageView	  srcImageView	= **m_allSrcImageViews[layerNdx];
	const VkImageView	  dstImageView	= **m_allDstImageViews[layerNdx];

	const VkDescriptorImageInfo descriptorSrcImageInfo = makeDescriptorImageInfo(DE_NULL, srcImageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorImageInfo descriptorDstImageInfo = makeDescriptorImageInfo(DE_NULL, dstImageView, VK_IMAGE_LAYOUT_GENERAL);

	DescriptorSetUpdateBuilder()
		.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorSrcImageInfo)
		.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorDstImageInfo)
		.update(vk, device);
	vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);
}

void ImageLoadStoreTestInstance::commandBeforeCompute (const VkCommandBuffer cmdBuffer)
{
	const DeviceInterface& vk = m_context.getDeviceInterface();

	const VkImageSubresourceRange fullImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, m_texture.numLayers());
	{
		const VkImageMemoryBarrier preCopyImageBarriers[] =
		{
			makeImageMemoryBarrier(
				0u, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				m_imageSrc->get(), fullImageSubresourceRange),
			makeImageMemoryBarrier(
				0u, VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				m_imageDst->get(), fullImageSubresourceRange)
		};

		const VkBufferMemoryBarrier barrierFlushHostWriteBeforeCopy = makeBufferMemoryBarrier(
			VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			m_imageBuffer->get(), 0ull, m_imageSizeBytes);

		vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
			(VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, &barrierFlushHostWriteBeforeCopy, DE_LENGTH_OF_ARRAY(preCopyImageBarriers), preCopyImageBarriers);
	}
	{
		const VkImageMemoryBarrier barrierAfterCopy = makeImageMemoryBarrier(
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			m_imageSrc->get(), fullImageSubresourceRange);

		const VkBufferImageCopy copyRegion = makeBufferImageCopy(m_texture);

		vk.cmdCopyBufferToImage(cmdBuffer, m_imageBuffer->get(), m_imageSrc->get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyRegion);
		vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &barrierAfterCopy);
	}
}

void ImageLoadStoreTestInstance::commandBetweenShaderInvocations (const VkCommandBuffer cmdBuffer)
{
	commandImageWriteBarrierBetweenShaderInvocations(m_context, cmdBuffer, m_imageDst->get(), m_texture);
}

void ImageLoadStoreTestInstance::commandAfterCompute (const VkCommandBuffer cmdBuffer)
{
	commandCopyImageToBuffer(m_context, cmdBuffer, m_imageDst->get(), m_imageBuffer->get(), m_imageSizeBytes, m_texture);
}

//! Load/store test for buffers
class BufferLoadStoreTestInstance : public LoadStoreTestInstance
{
public:
									BufferLoadStoreTestInstance		(Context&				context,
																	 const Texture&			texture,
																	 const VkFormat			format,
																	 const VkFormat			imageFormat,
																	 const bool				declareImageFormatInShader);

protected:
	VkDescriptorSetLayout			prepareDescriptors				(void);
	void							commandAfterCompute				(const VkCommandBuffer	cmdBuffer);

	void							commandBindDescriptorsForLayer	(const VkCommandBuffer	cmdBuffer,
																	 const VkPipelineLayout pipelineLayout,
																	 const int				layerNdx);

	Buffer*							getResultBuffer					(void) const { return m_imageBufferDst.get(); }

	de::MovePtr<Buffer>				m_imageBufferDst;
	Move<VkDescriptorSetLayout>		m_descriptorSetLayout;
	Move<VkDescriptorPool>			m_descriptorPool;
	Move<VkDescriptorSet>			m_descriptorSet;
	Move<VkBufferView>				m_bufferViewSrc;
	Move<VkBufferView>				m_bufferViewDst;
};

BufferLoadStoreTestInstance::BufferLoadStoreTestInstance (Context&			context,
														  const Texture&	texture,
														  const VkFormat	format,
														  const VkFormat	imageFormat,
														  const bool		declareImageFormatInShader)
	: LoadStoreTestInstance(context, texture, format, imageFormat, declareImageFormatInShader, false)
{
	const DeviceInterface&	vk			= m_context.getDeviceInterface();
	const VkDevice			device		= m_context.getDevice();
	Allocator&				allocator	= m_context.getDefaultAllocator();

	// Create a destination buffer.

	m_imageBufferDst = de::MovePtr<Buffer>(new Buffer(
		vk, device, allocator,
		makeBufferCreateInfo(m_imageSizeBytes, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT),
		MemoryRequirement::HostVisible));
}

VkDescriptorSetLayout BufferLoadStoreTestInstance::prepareDescriptors (void)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	m_descriptorSetLayout = DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device);

	m_descriptorPool = DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	m_descriptorSet = makeDescriptorSet(vk, device, *m_descriptorPool, *m_descriptorSetLayout);
	m_bufferViewSrc = makeBufferView(vk, device, m_imageBuffer->get(), m_format, 0ull, m_imageSizeBytes);
	m_bufferViewDst = makeBufferView(vk, device, m_imageBufferDst->get(), m_format, 0ull, m_imageSizeBytes);

	return *m_descriptorSetLayout;  // not passing the ownership
}

void BufferLoadStoreTestInstance::commandBindDescriptorsForLayer (const VkCommandBuffer cmdBuffer, const VkPipelineLayout pipelineLayout, const int layerNdx)
{
	DE_ASSERT(layerNdx == 0);
	DE_UNREF(layerNdx);

	const VkDevice			device	= m_context.getDevice();
	const DeviceInterface&	vk		= m_context.getDeviceInterface();

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, &m_bufferViewSrc.get())
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, &m_bufferViewDst.get())
		.update(vk, device);
	vk.cmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);
}

void BufferLoadStoreTestInstance::commandAfterCompute (const VkCommandBuffer cmdBuffer)
{
	commandBufferWriteBarrierBeforeHostRead(m_context, cmdBuffer, m_imageBufferDst->get(), m_imageSizeBytes);
}

TestInstance* StoreTest::createInstance (Context& context) const
{
	if (m_texture.type() == IMAGE_TYPE_BUFFER)
		return new BufferStoreTestInstance(context, m_texture, m_format, m_declareImageFormatInShader);
	else
		return new ImageStoreTestInstance(context, m_texture, m_format, m_declareImageFormatInShader, m_singleLayerBind);
}

TestInstance* LoadStoreTest::createInstance (Context& context) const
{
	if (m_texture.type() == IMAGE_TYPE_BUFFER)
		return new BufferLoadStoreTestInstance(context, m_texture, m_format, m_imageFormat, m_declareImageFormatInShader);
	else
		return new ImageLoadStoreTestInstance(context, m_texture, m_format, m_imageFormat, m_declareImageFormatInShader, m_singleLayerBind);
}

static const Texture s_textures[] =
{
	Texture(IMAGE_TYPE_1D,			tcu::IVec3(64,	1,	1),	1),
	Texture(IMAGE_TYPE_1D_ARRAY,	tcu::IVec3(64,	1,	1),	8),
	Texture(IMAGE_TYPE_2D,			tcu::IVec3(64,	64,	1),	1),
	Texture(IMAGE_TYPE_2D_ARRAY,	tcu::IVec3(64,	64,	1),	8),
	Texture(IMAGE_TYPE_3D,			tcu::IVec3(64,	64,	8),	1),
	Texture(IMAGE_TYPE_CUBE,		tcu::IVec3(64,	64,	1),	6),
	Texture(IMAGE_TYPE_CUBE_ARRAY,	tcu::IVec3(64,	64,	1),	2*6),
	Texture(IMAGE_TYPE_BUFFER,		tcu::IVec3(64,	1,	1),	1),
};

const Texture& getTestTexture (const ImageType imageType)
{
	for (int textureNdx = 0; textureNdx < DE_LENGTH_OF_ARRAY(s_textures); ++textureNdx)
		if (s_textures[textureNdx].type() == imageType)
			return s_textures[textureNdx];

	DE_FATAL("Internal error");
	return s_textures[0];
}

static const VkFormat s_formats[] =
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

} // anonymous ns

tcu::TestCaseGroup* createImageStoreTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "store", "Plain imageStore() cases"));
	de::MovePtr<tcu::TestCaseGroup> testGroupWithFormat(new tcu::TestCaseGroup(testCtx, "with_format", "Declare a format layout qualifier for write images"));
	de::MovePtr<tcu::TestCaseGroup> testGroupWithoutFormat(new tcu::TestCaseGroup(testCtx, "without_format", "Do not declare a format layout qualifier for write images"));

	for (int textureNdx = 0; textureNdx < DE_LENGTH_OF_ARRAY(s_textures); ++textureNdx)
	{
		const Texture& texture = s_textures[textureNdx];
		de::MovePtr<tcu::TestCaseGroup> groupWithFormatByImageViewType (new tcu::TestCaseGroup(testCtx, getImageTypeName(texture.type()).c_str(), ""));
		de::MovePtr<tcu::TestCaseGroup> groupWithoutFormatByImageViewType (new tcu::TestCaseGroup(testCtx, getImageTypeName(texture.type()).c_str(), ""));
		const bool isLayered = (texture.numLayers() > 1);

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(s_formats); ++formatNdx)
		{
			groupWithFormatByImageViewType->addChild(new StoreTest(testCtx, getFormatShortString(s_formats[formatNdx]), "", texture, s_formats[formatNdx]));
			groupWithoutFormatByImageViewType->addChild(new StoreTest(testCtx, getFormatShortString(s_formats[formatNdx]), "", texture, s_formats[formatNdx], 0));

			if (isLayered)
				groupWithFormatByImageViewType->addChild(new StoreTest(testCtx, getFormatShortString(s_formats[formatNdx]) + "_single_layer", "",
														texture, s_formats[formatNdx],
														StoreTest::FLAG_SINGLE_LAYER_BIND | StoreTest::FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER));
		}

		testGroupWithFormat->addChild(groupWithFormatByImageViewType.release());
		testGroupWithoutFormat->addChild(groupWithoutFormatByImageViewType.release());
	}

	testGroup->addChild(testGroupWithFormat.release());
	testGroup->addChild(testGroupWithoutFormat.release());

	return testGroup.release();
}

tcu::TestCaseGroup* createImageLoadStoreTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "load_store", "Cases with imageLoad() followed by imageStore()"));
	de::MovePtr<tcu::TestCaseGroup> testGroupWithFormat(new tcu::TestCaseGroup(testCtx, "with_format", "Declare a format layout qualifier for read images"));
	de::MovePtr<tcu::TestCaseGroup> testGroupWithoutFormat(new tcu::TestCaseGroup(testCtx, "without_format", "Do not declare a format layout qualifier for read images"));

	for (int textureNdx = 0; textureNdx < DE_LENGTH_OF_ARRAY(s_textures); ++textureNdx)
	{
		const Texture& texture = s_textures[textureNdx];
		de::MovePtr<tcu::TestCaseGroup> groupWithFormatByImageViewType (new tcu::TestCaseGroup(testCtx, getImageTypeName(texture.type()).c_str(), ""));
		de::MovePtr<tcu::TestCaseGroup> groupWithoutFormatByImageViewType (new tcu::TestCaseGroup(testCtx, getImageTypeName(texture.type()).c_str(), ""));
		const bool isLayered = (texture.numLayers() > 1);

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(s_formats); ++formatNdx)
		{
			groupWithFormatByImageViewType->addChild(new LoadStoreTest(testCtx, getFormatShortString(s_formats[formatNdx]), "", texture, s_formats[formatNdx], s_formats[formatNdx]));
			groupWithoutFormatByImageViewType->addChild(new LoadStoreTest(testCtx, getFormatShortString(s_formats[formatNdx]), "", texture, s_formats[formatNdx], s_formats[formatNdx], 0));

			if (isLayered)
				groupWithFormatByImageViewType->addChild(new LoadStoreTest(testCtx, getFormatShortString(s_formats[formatNdx]) + "_single_layer", "",
														texture, s_formats[formatNdx], s_formats[formatNdx],
														LoadStoreTest::FLAG_SINGLE_LAYER_BIND | LoadStoreTest::FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER));
		}

		testGroupWithFormat->addChild(groupWithFormatByImageViewType.release());
		testGroupWithoutFormat->addChild(groupWithoutFormatByImageViewType.release());
	}

	testGroup->addChild(testGroupWithFormat.release());
	testGroup->addChild(testGroupWithoutFormat.release());

	return testGroup.release();
}

tcu::TestCaseGroup* createImageFormatReinterpretTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "format_reinterpret",	"Cases with differing texture and image formats"));

	for (int textureNdx = 0; textureNdx < DE_LENGTH_OF_ARRAY(s_textures); ++textureNdx)
	{
		const Texture& texture = s_textures[textureNdx];
		de::MovePtr<tcu::TestCaseGroup> groupByImageViewType (new tcu::TestCaseGroup(testCtx, getImageTypeName(texture.type()).c_str(), ""));

		for (int imageFormatNdx = 0; imageFormatNdx < DE_LENGTH_OF_ARRAY(s_formats); ++imageFormatNdx)
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(s_formats); ++formatNdx)
		{
			const std::string caseName = getFormatShortString(s_formats[imageFormatNdx]) + "_" + getFormatShortString(s_formats[formatNdx]);
			if (imageFormatNdx != formatNdx && formatsAreCompatible(s_formats[imageFormatNdx], s_formats[formatNdx]))
				groupByImageViewType->addChild(new LoadStoreTest(testCtx, caseName, "", texture, s_formats[formatNdx], s_formats[imageFormatNdx]));
		}
		testGroup->addChild(groupByImageViewType.release());
	}

	return testGroup.release();
}

de::MovePtr<TestCase> createImageQualifierRestrictCase (tcu::TestContext& testCtx, const ImageType imageType, const std::string& name)
{
	const VkFormat format = VK_FORMAT_R32G32B32A32_UINT;
	const Texture& texture = getTestTexture(imageType);
	return de::MovePtr<TestCase>(new LoadStoreTest(testCtx, name, "", texture, format, format, LoadStoreTest::FLAG_RESTRICT_IMAGES | LoadStoreTest::FLAG_DECLARE_IMAGE_FORMAT_IN_SHADER));
}

} // image
} // vkt
