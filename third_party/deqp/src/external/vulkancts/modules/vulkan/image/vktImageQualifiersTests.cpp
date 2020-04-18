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
 * \brief Memory qualifiers tests
 *//*--------------------------------------------------------------------*/

#include "vktImageQualifiersTests.hpp"
#include "vktImageLoadStoreTests.hpp"
#include "vktImageTestsUtil.hpp"

#include "vkDefs.hpp"
#include "vkImageUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkMemUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deDefs.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include "tcuImageCompare.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorType.hpp"

using namespace vk;

namespace vkt
{
namespace image
{
namespace
{

static const tcu::UVec3		g_localWorkGroupSizeBase	= tcu::UVec3(8, 8, 2);
static const deInt32		g_ShaderReadOffsetsX[4]		= { 1, 4, 7, 10 };
static const deInt32		g_ShaderReadOffsetsY[4]		= { 2, 5, 8, 11 };
static const deInt32		g_ShaderReadOffsetsZ[4]		= { 3, 6, 9, 12 };
static const char* const	g_ShaderReadOffsetsXStr		= "int[]( 1, 4, 7, 10 )";
static const char* const	g_ShaderReadOffsetsYStr		= "int[]( 2, 5, 8, 11 )";
static const char* const	g_ShaderReadOffsetsZStr		= "int[]( 3, 6, 9, 12 )";

const tcu::UVec3 getLocalWorkGroupSize (const ImageType imageType, const tcu::UVec3& imageSize)
{
	const tcu::UVec3 computeGridSize	= getShaderGridSize(imageType, imageSize);

	const tcu::UVec3 localWorkGroupSize = tcu::UVec3(de::min(g_localWorkGroupSizeBase.x(), computeGridSize.x()),
													 de::min(g_localWorkGroupSizeBase.y(), computeGridSize.y()),
													 de::min(g_localWorkGroupSizeBase.z(), computeGridSize.z()));
	return localWorkGroupSize;
}

const tcu::UVec3 getNumWorkGroups (const ImageType imageType, const tcu::UVec3& imageSize)
{
	const tcu::UVec3 computeGridSize	= getShaderGridSize(imageType, imageSize);
	const tcu::UVec3 localWorkGroupSize = getLocalWorkGroupSize(imageType, imageSize);

	return computeGridSize / localWorkGroupSize;
}

tcu::ConstPixelBufferAccess getLayerOrSlice (const ImageType					imageType,
											 const tcu::ConstPixelBufferAccess&	access,
											 const deUint32						layer)
{
	switch (imageType)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_2D:
		case IMAGE_TYPE_BUFFER:
			DE_ASSERT(layer == 0);
			return access;

		case IMAGE_TYPE_1D_ARRAY:
			return tcu::getSubregion(access, 0, layer, access.getWidth(), 1);

		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_3D:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			return tcu::getSubregion(access, 0, 0, layer, access.getWidth(), access.getHeight(), 1);

		default:
			DE_FATAL("Unknown image type");
			return tcu::ConstPixelBufferAccess();
	}
}

bool comparePixelBuffers (tcu::TestContext&						testCtx,
						  const ImageType						imageType,
						  const tcu::UVec3&						imageSize,
						  const tcu::TextureFormat&				format,
						  const tcu::ConstPixelBufferAccess&	reference,
						  const tcu::ConstPixelBufferAccess&	result)
{
	DE_ASSERT(reference.getFormat() == result.getFormat());
	DE_ASSERT(reference.getSize() == result.getSize());

	const bool		 intFormat			= isIntFormat(mapTextureFormat(format)) || isUintFormat(mapTextureFormat(format));
	deUint32		 passedLayers		= 0;

	for (deUint32 layerNdx = 0; layerNdx < getNumLayers(imageType, imageSize); ++layerNdx)
	{
		const std::string comparisonName = "Comparison" + de::toString(layerNdx);

		std::string comparisonDesc = "Image Comparison, ";
		switch (imageType)
		{
			case IMAGE_TYPE_3D:
				comparisonDesc = comparisonDesc + "slice " + de::toString(layerNdx);
				break;

			case IMAGE_TYPE_CUBE:
			case IMAGE_TYPE_CUBE_ARRAY:
				comparisonDesc = comparisonDesc + "face " + de::toString(layerNdx % 6) + ", cube " + de::toString(layerNdx / 6);
				break;

			default:
				comparisonDesc = comparisonDesc + "layer " + de::toString(layerNdx);
				break;
		}

		const tcu::ConstPixelBufferAccess refLayer		= getLayerOrSlice(imageType, reference, layerNdx);
		const tcu::ConstPixelBufferAccess resultLayer	= getLayerOrSlice(imageType, result, layerNdx);

		bool ok = false;
		if (intFormat)
			ok = tcu::intThresholdCompare(testCtx.getLog(), comparisonName.c_str(), comparisonDesc.c_str(), refLayer, resultLayer, tcu::UVec4(0), tcu::COMPARE_LOG_RESULT);
		else
			ok = tcu::floatThresholdCompare(testCtx.getLog(), comparisonName.c_str(), comparisonDesc.c_str(), refLayer, resultLayer, tcu::Vec4(0.01f), tcu::COMPARE_LOG_RESULT);

		if (ok)
			++passedLayers;
	}

	return passedLayers == getNumLayers(imageType, imageSize);
}

const std::string getCoordStr (const ImageType		imageType,
							   const std::string&	x,
							   const std::string&	y,
							   const std::string&	z)
{
	switch (imageType)
	{
		case IMAGE_TYPE_1D:
		case IMAGE_TYPE_BUFFER:
			return x;

		case IMAGE_TYPE_1D_ARRAY:
		case IMAGE_TYPE_2D:
			return "ivec2(" + x + "," + y + ")";

		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_3D:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			return "ivec3(" + x + "," + y + "," + z + ")";

		default:
			DE_ASSERT(false);
			return "";
	}
}

class MemoryQualifierTestCase : public vkt::TestCase
{
public:

	enum Qualifier
	{
		QUALIFIER_COHERENT = 0,
		QUALIFIER_VOLATILE,
		QUALIFIER_RESTRICT,
		QUALIFIER_LAST
	};

								MemoryQualifierTestCase		(tcu::TestContext&			testCtx,
															 const std::string&			name,
															 const std::string&			description,
															 const Qualifier			qualifier,
															 const ImageType			imageType,
															 const tcu::UVec3&			imageSize,
															 const tcu::TextureFormat&	format,
															 const glu::GLSLVersion		glslVersion);

	virtual						~MemoryQualifierTestCase	(void) {}

	virtual void				initPrograms				(SourceCollections&			programCollection) const;
	virtual TestInstance*		createInstance				(Context&					context) const;

protected:

	const Qualifier				m_qualifier;
	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
	const glu::GLSLVersion		m_glslVersion;
};

MemoryQualifierTestCase::MemoryQualifierTestCase (tcu::TestContext&			testCtx,
												  const std::string&		name,
												  const std::string&		description,
												  const Qualifier			qualifier,
												  const ImageType			imageType,
												  const tcu::UVec3&			imageSize,
												  const tcu::TextureFormat&	format,
												  const glu::GLSLVersion	glslVersion)
	: vkt::TestCase(testCtx, name, description)
	, m_qualifier(qualifier)
	, m_imageType(imageType)
	, m_imageSize(imageSize)
	, m_format(format)
	, m_glslVersion(glslVersion)
{
}

void MemoryQualifierTestCase::initPrograms (SourceCollections& programCollection) const
{
	const char* const	versionDecl			= glu::getGLSLVersionDeclaration(m_glslVersion);

	const char* const	qualifierName		= m_qualifier == QUALIFIER_COHERENT ? "coherent"
											: m_qualifier == QUALIFIER_VOLATILE ? "volatile"
											: DE_NULL;

	const bool			uintFormat			= isUintFormat(mapTextureFormat(m_format));
	const bool			intFormat			= isIntFormat(mapTextureFormat(m_format));
	const std::string	colorVecTypeName	= std::string(uintFormat ? "u"	: intFormat ? "i" : "") + "vec4";
	const std::string	colorScalarTypeName = std::string(uintFormat ? "uint" : intFormat ? "int" : "float");
	const std::string	invocationCoord		= getCoordStr(m_imageType, "gx", "gy", "gz");
	const std::string	shaderImageFormat	= getShaderImageFormatQualifier(m_format);
	const std::string	shaderImageType		= getShaderImageType(m_format, m_imageType);

	const tcu::UVec3	localWorkGroupSize	= getLocalWorkGroupSize(m_imageType, m_imageSize);
	const std::string	localSizeX			= de::toString(localWorkGroupSize.x());
	const std::string	localSizeY			= de::toString(localWorkGroupSize.y());
	const std::string	localSizeZ			= de::toString(localWorkGroupSize.z());

	std::ostringstream	programBuffer;

	programBuffer
		<< versionDecl << "\n"
		<< "\n"
		<< "precision highp " << shaderImageType << ";\n"
		<< "\n"
		<< "layout (local_size_x = " << localSizeX << ", local_size_y = " << localSizeY << ", local_size_z = " + localSizeZ << ") in;\n"
		<< "layout (" << shaderImageFormat << ", binding=0) " << qualifierName << " uniform " << shaderImageType << " u_image;\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	int gx = int(gl_GlobalInvocationID.x);\n"
		<< "	int gy = int(gl_GlobalInvocationID.y);\n"
		<< "	int gz = int(gl_GlobalInvocationID.z);\n"
		<< "	imageStore(u_image, " << invocationCoord << ", " << colorVecTypeName << "(gx^gy^gz));\n"
		<< "\n"
		<< "	memoryBarrier();\n"
		<< "	barrier();\n"
		<< "\n"
		<< "	" << colorScalarTypeName << " sum = " << colorScalarTypeName << "(0);\n"
		<< "	int groupBaseX = gx/" << localSizeX << "*" << localSizeX << ";\n"
		<< "	int groupBaseY = gy/" << localSizeY << "*" << localSizeY << ";\n"
		<< "	int groupBaseZ = gz/" << localSizeZ << "*" << localSizeZ << ";\n"
		<< "	int xOffsets[] = " << g_ShaderReadOffsetsXStr << ";\n"
		<< "	int yOffsets[] = " << g_ShaderReadOffsetsYStr << ";\n"
		<< "	int zOffsets[] = " << g_ShaderReadOffsetsZStr << ";\n"
		<< "	for (int i = 0; i < " << de::toString(DE_LENGTH_OF_ARRAY(g_ShaderReadOffsetsX)) << "; i++)\n"
		<< "	{\n"
		<< "		int readX = groupBaseX + (gx + xOffsets[i]) % " + localSizeX + ";\n"
		<< "		int readY = groupBaseY + (gy + yOffsets[i]) % " + localSizeY + ";\n"
		<< "		int readZ = groupBaseZ + (gz + zOffsets[i]) % " + localSizeZ + ";\n"
		<< "		sum += imageLoad(u_image, " << getCoordStr(m_imageType, "readX", "readY", "readZ") << ").x;\n"
		<< "	}\n"
		<< "\n"
		<< "	memoryBarrier();\n"
		<< "	barrier();\n"
		<< "\n"
		<< "	imageStore(u_image, " + invocationCoord + ", " + colorVecTypeName + "(sum));\n"
		<< "}\n";

	programCollection.glslSources.add(m_name) << glu::ComputeSource(programBuffer.str());
}

class MemoryQualifierInstanceBase : public vkt::TestInstance
{
public:
									MemoryQualifierInstanceBase		(Context&					context,
																	 const std::string&			name,
																	 const ImageType			imageType,
																	 const tcu::UVec3&			imageSize,
																	 const tcu::TextureFormat&	format);

	virtual							~MemoryQualifierInstanceBase	(void) {};

	virtual tcu::TestStatus			iterate							(void);

	virtual void					prepareResources				(const VkDeviceSize			bufferSizeInBytes) = 0;

	virtual void					prepareDescriptors				(void) = 0;

	virtual void					commandsBeforeCompute			(const VkCommandBuffer		cmdBuffer,
																	 const VkDeviceSize			bufferSizeInBytes) const = 0;

	virtual void					commandsAfterCompute			(const VkCommandBuffer		cmdBuffer,
																	 const VkDeviceSize			bufferSizeInBytes) const = 0;
protected:

	tcu::TextureLevel				generateReferenceImage			(void) const;

	const std::string				m_name;
	const ImageType					m_imageType;
	const tcu::UVec3				m_imageSize;
	const tcu::TextureFormat		m_format;

	de::MovePtr<Buffer>				m_buffer;
	Move<VkDescriptorPool>			m_descriptorPool;
	Move<VkDescriptorSetLayout>		m_descriptorSetLayout;
	Move<VkDescriptorSet>			m_descriptorSet;
};

MemoryQualifierInstanceBase::MemoryQualifierInstanceBase (Context&					context,
														  const std::string&		name,
														  const ImageType			imageType,
														  const tcu::UVec3&			imageSize,
														  const tcu::TextureFormat&	format)
	: vkt::TestInstance(context)
	, m_name(name)
	, m_imageType(imageType)
	, m_imageSize(imageSize)
	, m_format(format)
{
}

tcu::TestStatus	MemoryQualifierInstanceBase::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	deviceInterface		= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();

	const VkDeviceSize	bufferSizeInBytes = getNumPixels(m_imageType, m_imageSize) * tcu::getPixelSize(m_format);

	// Prepare resources for the test
	prepareResources(bufferSizeInBytes);

	// Prepare descriptor sets
	prepareDescriptors();

	// Create compute shader
	const vk::Unique<VkShaderModule> shaderModule(createShaderModule(deviceInterface, device, m_context.getBinaryCollection().get(m_name), 0u));

	// Create compute pipeline
	const vk::Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(deviceInterface, device, *m_descriptorSetLayout));
	const vk::Unique<VkPipeline> pipeline(makeComputePipeline(deviceInterface, device, *pipelineLayout, *shaderModule));

	// Create command buffer
	const Unique<VkCommandPool> cmdPool(createCommandPool(deviceInterface, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(deviceInterface, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands
	beginCommandBuffer(deviceInterface, *cmdBuffer);

	deviceInterface.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	deviceInterface.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);

	commandsBeforeCompute(*cmdBuffer, bufferSizeInBytes);

	const tcu::UVec3 numGroups = getNumWorkGroups(m_imageType, m_imageSize);
	deviceInterface.cmdDispatch(*cmdBuffer, numGroups.x(), numGroups.y(), numGroups.z());

	commandsAfterCompute(*cmdBuffer, bufferSizeInBytes);

	endCommandBuffer(deviceInterface, *cmdBuffer);

	// Submit and wait for completion
	submitCommandsAndWait(deviceInterface, device, queue, *cmdBuffer);

	// Retrieve data from buffer to host memory
	const Allocation& allocation = m_buffer->getAllocation();
	invalidateMappedMemoryRange(deviceInterface, device, allocation.getMemory(), allocation.getOffset(), bufferSizeInBytes);

	const tcu::UVec3 computeGridSize = getShaderGridSize(m_imageType, m_imageSize);
	tcu::ConstPixelBufferAccess resultPixelBuffer(m_format, computeGridSize.x(), computeGridSize.y(), computeGridSize.z(), allocation.getHostPtr());

	// Create a reference image
	tcu::TextureLevel referenceImage = generateReferenceImage();
	tcu::ConstPixelBufferAccess referencePixelBuffer = referenceImage.getAccess();

	// Validate the result
	if (comparePixelBuffers(m_context.getTestContext(), m_imageType, m_imageSize, m_format, referencePixelBuffer, resultPixelBuffer))
		return tcu::TestStatus::pass("Passed");
	else
		return tcu::TestStatus::fail("Image comparison failed");
}

tcu::TextureLevel MemoryQualifierInstanceBase::generateReferenceImage (void) const
{
	// Generate a reference image data using the storage format
	const tcu::UVec3 computeGridSize = getShaderGridSize(m_imageType, m_imageSize);

	tcu::TextureLevel base(m_format, computeGridSize.x(), computeGridSize.y(), computeGridSize.z());
	tcu::PixelBufferAccess baseAccess = base.getAccess();

	tcu::TextureLevel reference(m_format, computeGridSize.x(), computeGridSize.y(), computeGridSize.z());
	tcu::PixelBufferAccess referenceAccess = reference.getAccess();

	for (deInt32 z = 0; z < baseAccess.getDepth(); ++z)
		for (deInt32 y = 0; y < baseAccess.getHeight(); ++y)
			for (deInt32 x = 0; x < baseAccess.getWidth(); ++x)
			{
				baseAccess.setPixel(tcu::IVec4(x^y^z), x, y, z);
			}

	const tcu::UVec3 localWorkGroupSize = getLocalWorkGroupSize(m_imageType, m_imageSize);

	for (deInt32 z = 0; z < referenceAccess.getDepth(); ++z)
		for (deInt32 y = 0; y < referenceAccess.getHeight(); ++y)
			for (deInt32 x = 0; x < referenceAccess.getWidth(); ++x)
			{
				const deInt32	groupBaseX	= x / localWorkGroupSize.x() * localWorkGroupSize.x();
				const deInt32	groupBaseY	= y / localWorkGroupSize.y() * localWorkGroupSize.y();
				const deInt32	groupBaseZ	= z / localWorkGroupSize.z() * localWorkGroupSize.z();
				deInt32			sum			= 0;

				for (deInt32 i = 0; i < DE_LENGTH_OF_ARRAY(g_ShaderReadOffsetsX); i++)
				{
					sum += baseAccess.getPixelInt(
						groupBaseX + (x + g_ShaderReadOffsetsX[i]) % localWorkGroupSize.x(),
						groupBaseY + (y + g_ShaderReadOffsetsY[i]) % localWorkGroupSize.y(),
						groupBaseZ + (z + g_ShaderReadOffsetsZ[i]) % localWorkGroupSize.z()).x();
				}

				referenceAccess.setPixel(tcu::IVec4(sum), x, y, z);
			}

	return reference;
}

class MemoryQualifierInstanceImage : public MemoryQualifierInstanceBase
{
public:
						MemoryQualifierInstanceImage	(Context&					context,
														 const std::string&			name,
														 const ImageType			imageType,
														 const tcu::UVec3&			imageSize,
														 const tcu::TextureFormat&	format)
							: MemoryQualifierInstanceBase(context, name, imageType, imageSize, format) {}

	virtual				~MemoryQualifierInstanceImage	(void) {};

	virtual void		prepareResources				(const VkDeviceSize			bufferSizeInBytes);

	virtual void		prepareDescriptors				(void);

	virtual void		commandsBeforeCompute			(const VkCommandBuffer		cmdBuffer,
														 const VkDeviceSize			bufferSizeInBytes) const;

	virtual void		commandsAfterCompute			(const VkCommandBuffer		cmdBuffer,
														 const VkDeviceSize			bufferSizeInBytes) const;
protected:

	de::MovePtr<Image>	m_image;
	Move<VkImageView>	m_imageView;
};

void MemoryQualifierInstanceImage::prepareResources (const VkDeviceSize bufferSizeInBytes)
{
	const VkDevice			device			= m_context.getDevice();
	const DeviceInterface&	deviceInterface = m_context.getDeviceInterface();
	Allocator&				allocator		= m_context.getDefaultAllocator();

	// Create image
	const VkImageCreateInfo imageCreateInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,							// VkStructureType			sType;
		DE_NULL,														// const void*				pNext;
		m_imageType == IMAGE_TYPE_CUBE ||
		m_imageType	== IMAGE_TYPE_CUBE_ARRAY
		? (VkImageCreateFlags)VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u,	// VkImageCreateFlags		flags;
		mapImageType(m_imageType),										// VkImageType				imageType;
		mapTextureFormat(m_format),										// VkFormat					format;
		makeExtent3D(getLayerSize(m_imageType, m_imageSize)),			// VkExtent3D				extent;
		1u,																// deUint32					mipLevels;
		getNumLayers(m_imageType, m_imageSize),							// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,											// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,										// VkImageTiling			tiling;
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,	// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,										// VkSharingMode			sharingMode;
		0u,																// deUint32					queueFamilyIndexCount;
		DE_NULL,														// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,										// VkImageLayout			initialLayout;
	};

	m_image = de::MovePtr<Image>(new Image(deviceInterface, device, allocator, imageCreateInfo, MemoryRequirement::Any));

	// Create imageView
	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, getNumLayers(m_imageType, m_imageSize));
	m_imageView = makeImageView(deviceInterface, device, m_image->get(), mapImageViewType(m_imageType), mapTextureFormat(m_format), subresourceRange);

	// Create a buffer to store shader output (copied from image data)
	const VkBufferCreateInfo	bufferCreateInfo = makeBufferCreateInfo(bufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	m_buffer = de::MovePtr<Buffer>(new Buffer(deviceInterface, device, allocator, bufferCreateInfo, MemoryRequirement::HostVisible));
}

void MemoryQualifierInstanceImage::prepareDescriptors (void)
{
	const VkDevice			device			= m_context.getDevice();
	const DeviceInterface&	deviceInterface = m_context.getDeviceInterface();

	// Create descriptor pool
	m_descriptorPool =
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(deviceInterface, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	// Create descriptor set layout
	m_descriptorSetLayout =
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(deviceInterface, device);

	// Allocate descriptor set
	m_descriptorSet = makeDescriptorSet(deviceInterface, device, *m_descriptorPool, *m_descriptorSetLayout);

	// Set the bindings
	const VkDescriptorImageInfo descriptorImageInfo = makeDescriptorImageInfo(DE_NULL, *m_imageView, VK_IMAGE_LAYOUT_GENERAL);

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorImageInfo)
		.update(deviceInterface, device);
}

void MemoryQualifierInstanceImage::commandsBeforeCompute (const VkCommandBuffer cmdBuffer, const VkDeviceSize bufferSizeInBytes) const
{
	DE_UNREF(bufferSizeInBytes);

	const DeviceInterface&			deviceInterface	 = m_context.getDeviceInterface();
	const VkImageSubresourceRange	subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, getNumLayers(m_imageType, m_imageSize));

	const VkImageMemoryBarrier imageLayoutBarrier
		= makeImageMemoryBarrier(0u,
								 VK_ACCESS_SHADER_READ_BIT,
								 VK_IMAGE_LAYOUT_UNDEFINED,
								 VK_IMAGE_LAYOUT_GENERAL,
								 m_image->get(),
								 subresourceRange);

	deviceInterface.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imageLayoutBarrier);
}

void MemoryQualifierInstanceImage::commandsAfterCompute (const VkCommandBuffer cmdBuffer, const VkDeviceSize bufferSizeInBytes) const
{
	const DeviceInterface&			deviceInterface	 = m_context.getDeviceInterface();
	const VkImageSubresourceRange	subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, getNumLayers(m_imageType, m_imageSize));

	const VkImageMemoryBarrier imagePreCopyBarrier
		= makeImageMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT,
								 VK_ACCESS_TRANSFER_READ_BIT,
								 VK_IMAGE_LAYOUT_GENERAL,
								 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								 m_image->get(),
								 subresourceRange);

	deviceInterface.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &imagePreCopyBarrier);

	const VkBufferImageCopy copyParams = makeBufferImageCopy(makeExtent3D(getLayerSize(m_imageType, m_imageSize)), getNumLayers(m_imageType, m_imageSize));
	deviceInterface.cmdCopyImageToBuffer(cmdBuffer, m_image->get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_buffer->get(), 1u, &copyParams);

	const VkBufferMemoryBarrier bufferPostCopyBarrier
		= makeBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT,
								  VK_ACCESS_HOST_READ_BIT,
								  m_buffer->get(),
								  0ull,
								  bufferSizeInBytes);

	deviceInterface.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, 1u, &bufferPostCopyBarrier, 0u, DE_NULL);
}

class MemoryQualifierInstanceBuffer : public MemoryQualifierInstanceBase
{
public:
						MemoryQualifierInstanceBuffer	(Context&					context,
														 const std::string&			name,
														 const ImageType			imageType,
														 const tcu::UVec3&			imageSize,
														 const tcu::TextureFormat&	format)
							: MemoryQualifierInstanceBase(context, name, imageType, imageSize, format) {}

	virtual				~MemoryQualifierInstanceBuffer	(void) {};

	virtual void		prepareResources				(const VkDeviceSize			bufferSizeInBytes);

	virtual void		prepareDescriptors				(void);

	virtual void		commandsBeforeCompute			(const VkCommandBuffer,
														 const VkDeviceSize) const {}

	virtual void		commandsAfterCompute			(const VkCommandBuffer		cmdBuffer,
														 const VkDeviceSize			bufferSizeInBytes) const;
protected:

	Move<VkBufferView>	m_bufferView;
};

void MemoryQualifierInstanceBuffer::prepareResources (const VkDeviceSize bufferSizeInBytes)
{
	const VkDevice			device			= m_context.getDevice();
	const DeviceInterface&	deviceInterface = m_context.getDeviceInterface();
	Allocator&				allocator		= m_context.getDefaultAllocator();

	// Create a buffer to store shader output
	const VkBufferCreateInfo bufferCreateInfo = makeBufferCreateInfo(bufferSizeInBytes, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
	m_buffer = de::MovePtr<Buffer>(new Buffer(deviceInterface, device, allocator, bufferCreateInfo, MemoryRequirement::HostVisible));

	m_bufferView = makeBufferView(deviceInterface, device, m_buffer->get(), mapTextureFormat(m_format), 0ull, bufferSizeInBytes);
}

void MemoryQualifierInstanceBuffer::prepareDescriptors (void)
{
	const VkDevice			device			= m_context.getDevice();
	const DeviceInterface&	deviceInterface = m_context.getDeviceInterface();

	// Create descriptor pool
	m_descriptorPool =
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
		.build(deviceInterface, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	// Create descriptor set layout
	m_descriptorSetLayout =
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(deviceInterface, device);

	// Allocate descriptor set
	m_descriptorSet = makeDescriptorSet(deviceInterface, device, *m_descriptorPool, *m_descriptorSetLayout);

	// Set the bindings
	DescriptorSetUpdateBuilder()
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, &m_bufferView.get())
		.update(deviceInterface, device);
}

void MemoryQualifierInstanceBuffer::commandsAfterCompute (const VkCommandBuffer cmdBuffer, const VkDeviceSize bufferSizeInBytes) const
{
	const DeviceInterface&	deviceInterface = m_context.getDeviceInterface();

	const VkBufferMemoryBarrier shaderWriteBarrier
		= makeBufferMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT,
								  VK_ACCESS_HOST_READ_BIT,
								  m_buffer->get(),
								  0ull,
								  bufferSizeInBytes);

	deviceInterface.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, 1u, &shaderWriteBarrier, 0u, DE_NULL);
}

TestInstance* MemoryQualifierTestCase::createInstance (Context& context) const
{
	if ( m_imageType == IMAGE_TYPE_BUFFER )
		return new MemoryQualifierInstanceBuffer(context, m_name, m_imageType, m_imageSize, m_format);
	else
		return new MemoryQualifierInstanceImage(context, m_name, m_imageType, m_imageSize, m_format);
}

} // anonymous ns

tcu::TestCaseGroup* createImageQualifiersTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> imageQualifiersTests(new tcu::TestCaseGroup(testCtx, "qualifiers", "Coherent, volatile and restrict"));

	struct ImageParams
	{
		ImageParams(const ImageType imageType, const tcu::UVec3& imageSize)
			: m_imageType	(imageType)
			, m_imageSize	(imageSize)
		{
		}
		ImageType	m_imageType;
		tcu::UVec3	m_imageSize;
	};

	static const ImageParams imageParamsArray[] =
	{
		ImageParams(IMAGE_TYPE_1D,			tcu::UVec3(64u, 1u,  1u)),
		ImageParams(IMAGE_TYPE_1D_ARRAY,	tcu::UVec3(64u, 1u,  8u)),
		ImageParams(IMAGE_TYPE_2D,			tcu::UVec3(64u, 64u, 1u)),
		ImageParams(IMAGE_TYPE_2D_ARRAY,	tcu::UVec3(64u, 64u, 8u)),
		ImageParams(IMAGE_TYPE_3D,			tcu::UVec3(64u, 64u, 8u)),
		ImageParams(IMAGE_TYPE_CUBE,		tcu::UVec3(64u, 64u, 1u)),
		ImageParams(IMAGE_TYPE_CUBE_ARRAY,	tcu::UVec3(64u, 64u, 2u)),
		ImageParams(IMAGE_TYPE_BUFFER,		tcu::UVec3(64u, 1u,  1u))
	};

	static const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::FLOAT),
		tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::SIGNED_INT32),
	};

	for (deUint32 qualifierI = 0; qualifierI < MemoryQualifierTestCase::QUALIFIER_LAST; ++qualifierI)
	{
		const MemoryQualifierTestCase::Qualifier	memoryQualifier		= (MemoryQualifierTestCase::Qualifier)qualifierI;
		const char* const							memoryQualifierName =
			memoryQualifier == MemoryQualifierTestCase::QUALIFIER_COHERENT ? "coherent" :
			memoryQualifier == MemoryQualifierTestCase::QUALIFIER_VOLATILE ? "volatile" :
			memoryQualifier == MemoryQualifierTestCase::QUALIFIER_RESTRICT ? "restrict" :
			DE_NULL;

		de::MovePtr<tcu::TestCaseGroup> qualifierGroup(new tcu::TestCaseGroup(testCtx, memoryQualifierName, ""));

		for (deInt32 imageTypeNdx = 0; imageTypeNdx < DE_LENGTH_OF_ARRAY(imageParamsArray); imageTypeNdx++)
		{
			const ImageType		imageType = imageParamsArray[imageTypeNdx].m_imageType;
			const tcu::UVec3	imageSize = imageParamsArray[imageTypeNdx].m_imageSize;

			if (memoryQualifier == MemoryQualifierTestCase::QUALIFIER_RESTRICT)
			{
				de::MovePtr<TestCase> restrictCase = createImageQualifierRestrictCase(testCtx, imageType, getImageTypeName(imageType));
				qualifierGroup->addChild(restrictCase.release());
			}
			else
			{
				de::MovePtr<tcu::TestCaseGroup> imageTypeGroup(new tcu::TestCaseGroup(testCtx, getImageTypeName(imageType).c_str(), ""));

				for (deInt32 formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
				{
					const tcu::TextureFormat&	format		= formats[formatNdx];
					const std::string			formatName	= getShaderImageFormatQualifier(formats[formatNdx]);

					imageTypeGroup->addChild(
						new MemoryQualifierTestCase(testCtx, formatName, "", memoryQualifier, imageType, imageSize, format, glu::GLSL_VERSION_440));
				}

				qualifierGroup->addChild(imageTypeGroup.release());
			}
		}

		imageQualifiersTests->addChild(qualifierGroup.release());
	}

	return imageQualifiersTests.release();
}

} // image
} // vkt
