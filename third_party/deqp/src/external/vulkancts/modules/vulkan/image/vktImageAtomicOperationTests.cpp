/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \file  vktImageAtomicOperationTests.cpp
 * \brief Image atomic operation tests
 *//*--------------------------------------------------------------------*/

#include "vktImageAtomicOperationTests.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"

#include "vktTestCaseUtil.hpp"
#include "vkPrograms.hpp"
#include "vkImageUtil.hpp"
#include "vktImageTestsUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"

#include "tcuTextureUtil.hpp"
#include "tcuTexture.hpp"
#include "tcuVectorType.hpp"

namespace vkt
{
namespace image
{
namespace
{

using namespace vk;
using namespace std;
using de::toString;

using tcu::TextureFormat;
using tcu::IVec2;
using tcu::IVec3;
using tcu::UVec3;
using tcu::Vec4;
using tcu::IVec4;
using tcu::UVec4;
using tcu::CubeFace;
using tcu::Texture1D;
using tcu::Texture2D;
using tcu::Texture3D;
using tcu::Texture2DArray;
using tcu::TextureCube;
using tcu::PixelBufferAccess;
using tcu::ConstPixelBufferAccess;
using tcu::Vector;
using tcu::TestContext;

enum
{
	NUM_INVOCATIONS_PER_PIXEL = 5u
};

enum AtomicOperation
{
	ATOMIC_OPERATION_ADD = 0,
	ATOMIC_OPERATION_MIN,
	ATOMIC_OPERATION_MAX,
	ATOMIC_OPERATION_AND,
	ATOMIC_OPERATION_OR,
	ATOMIC_OPERATION_XOR,
	ATOMIC_OPERATION_EXCHANGE,
	ATOMIC_OPERATION_COMPARE_EXCHANGE,

	ATOMIC_OPERATION_LAST
};

static string getCoordStr (const ImageType		imageType,
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
			return string("ivec2(" + x + "," + y + ")");
		case IMAGE_TYPE_2D_ARRAY:
		case IMAGE_TYPE_3D:
		case IMAGE_TYPE_CUBE:
		case IMAGE_TYPE_CUBE_ARRAY:
			return string("ivec3(" + x + "," + y + "," + z + ")");
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static string getAtomicFuncArgumentShaderStr (const AtomicOperation	op,
											  const string&			x,
											  const string&			y,
											  const string&			z,
											  const IVec3&			gridSize)
{
	switch (op)
	{
		case ATOMIC_OPERATION_ADD:
		case ATOMIC_OPERATION_MIN:
		case ATOMIC_OPERATION_MAX:
		case ATOMIC_OPERATION_AND:
		case ATOMIC_OPERATION_OR:
		case ATOMIC_OPERATION_XOR:
			return string("(" + x + "*" + x + " + " + y + "*" + y + " + " + z + "*" + z + ")");
		case ATOMIC_OPERATION_EXCHANGE:
		case ATOMIC_OPERATION_COMPARE_EXCHANGE:
			return string("((" + z + "*" + toString(gridSize.x()) + " + " + x + ")*" + toString(gridSize.y()) + " + " + y + ")");
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static string getAtomicOperationCaseName (const AtomicOperation op)
{
	switch (op)
	{
		case ATOMIC_OPERATION_ADD:				return string("add");
		case ATOMIC_OPERATION_MIN:				return string("min");
		case ATOMIC_OPERATION_MAX:				return string("max");
		case ATOMIC_OPERATION_AND:				return string("and");
		case ATOMIC_OPERATION_OR:				return string("or");
		case ATOMIC_OPERATION_XOR:				return string("xor");
		case ATOMIC_OPERATION_EXCHANGE:			return string("exchange");
		case ATOMIC_OPERATION_COMPARE_EXCHANGE:	return string("compare_exchange");
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static string getAtomicOperationShaderFuncName (const AtomicOperation op)
{
	switch (op)
	{
		case ATOMIC_OPERATION_ADD:				return string("imageAtomicAdd");
		case ATOMIC_OPERATION_MIN:				return string("imageAtomicMin");
		case ATOMIC_OPERATION_MAX:				return string("imageAtomicMax");
		case ATOMIC_OPERATION_AND:				return string("imageAtomicAnd");
		case ATOMIC_OPERATION_OR:				return string("imageAtomicOr");
		case ATOMIC_OPERATION_XOR:				return string("imageAtomicXor");
		case ATOMIC_OPERATION_EXCHANGE:			return string("imageAtomicExchange");
		case ATOMIC_OPERATION_COMPARE_EXCHANGE:	return string("imageAtomicCompSwap");
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static deInt32 getOperationInitialValue (const AtomicOperation op)
{
	switch (op)
	{
		// \note 18 is just an arbitrary small nonzero value.
		case ATOMIC_OPERATION_ADD:				return 18;
		case ATOMIC_OPERATION_MIN:				return (1 << 15) - 1;
		case ATOMIC_OPERATION_MAX:				return 18;
		case ATOMIC_OPERATION_AND:				return (1 << 15) - 1;
		case ATOMIC_OPERATION_OR:				return 18;
		case ATOMIC_OPERATION_XOR:				return 18;
		case ATOMIC_OPERATION_EXCHANGE:			return 18;
		case ATOMIC_OPERATION_COMPARE_EXCHANGE:	return 18;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

static deInt32 getAtomicFuncArgument (const AtomicOperation op, const IVec3& invocationID, const IVec3& gridSize)
{
	const int x = invocationID.x();
	const int y = invocationID.y();
	const int z = invocationID.z();

	switch (op)
	{
		// \note Fall-throughs.
		case ATOMIC_OPERATION_ADD:
		case ATOMIC_OPERATION_MIN:
		case ATOMIC_OPERATION_MAX:
		case ATOMIC_OPERATION_AND:
		case ATOMIC_OPERATION_OR:
		case ATOMIC_OPERATION_XOR:
			return x*x + y*y + z*z;
		case ATOMIC_OPERATION_EXCHANGE:
		case ATOMIC_OPERATION_COMPARE_EXCHANGE:
			return (z*gridSize.x() + x)*gridSize.y() + y;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

//! An order-independent operation is one for which the end result doesn't depend on the order in which the operations are carried (i.e. is both commutative and associative).
static bool isOrderIndependentAtomicOperation (const AtomicOperation op)
{
	return	op == ATOMIC_OPERATION_ADD ||
			op == ATOMIC_OPERATION_MIN ||
			op == ATOMIC_OPERATION_MAX ||
			op == ATOMIC_OPERATION_AND ||
			op == ATOMIC_OPERATION_OR ||
			op == ATOMIC_OPERATION_XOR;
}

//! Computes the result of an atomic operation where "a" is the data operated on and "b" is the parameter to the atomic function.
static deInt32 computeBinaryAtomicOperationResult (const AtomicOperation op, const deInt32 a, const deInt32 b)
{
	switch (op)
	{
		case ATOMIC_OPERATION_ADD:				return a + b;
		case ATOMIC_OPERATION_MIN:				return de::min(a, b);
		case ATOMIC_OPERATION_MAX:				return de::max(a, b);
		case ATOMIC_OPERATION_AND:				return a & b;
		case ATOMIC_OPERATION_OR:				return a | b;
		case ATOMIC_OPERATION_XOR:				return a ^ b;
		case ATOMIC_OPERATION_EXCHANGE:			return b;
		case ATOMIC_OPERATION_COMPARE_EXCHANGE:	return (a == 18) ? b : a;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

class BinaryAtomicEndResultCase : public vkt::TestCase
{
public:
								BinaryAtomicEndResultCase  (tcu::TestContext&			testCtx,
															const string&				name,
															const string&				description,
															const ImageType				imageType,
															const tcu::UVec3&			imageSize,
															const tcu::TextureFormat&	format,
															const AtomicOperation		operation,
															const glu::GLSLVersion		glslVersion);

	void						initPrograms			   (SourceCollections&			sourceCollections) const;
	TestInstance*				createInstance			   (Context&					context) const;
private:

	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
	const AtomicOperation		m_operation;
	const glu::GLSLVersion		m_glslVersion;
};

BinaryAtomicEndResultCase::BinaryAtomicEndResultCase (tcu::TestContext&			testCtx,
													  const string&				name,
													  const string&				description,
													  const ImageType			imageType,
													  const tcu::UVec3&			imageSize,
													  const tcu::TextureFormat&	format,
													  const AtomicOperation		operation,
													  const glu::GLSLVersion	glslVersion)
	: TestCase		(testCtx, name, description)
	, m_imageType	(imageType)
	, m_imageSize	(imageSize)
	, m_format		(format)
	, m_operation	(operation)
	, m_glslVersion	(glslVersion)
{
}

void BinaryAtomicEndResultCase::initPrograms (SourceCollections& sourceCollections) const
{
	const string	versionDecl				= glu::getGLSLVersionDeclaration(m_glslVersion);

	const bool		uintFormat				= isUintFormat(mapTextureFormat(m_format));
	const bool		intFormat				= isIntFormat(mapTextureFormat(m_format));
	const UVec3		gridSize				= getShaderGridSize(m_imageType, m_imageSize);
	const string	atomicCoord				= getCoordStr(m_imageType, "gx % " + toString(gridSize.x()), "gy", "gz");

	const string	atomicArgExpr			= (uintFormat ? "uint" : intFormat ? "int" : "float")
											+ getAtomicFuncArgumentShaderStr(m_operation, "gx", "gy", "gz", IVec3(NUM_INVOCATIONS_PER_PIXEL*gridSize.x(), gridSize.y(), gridSize.z()));

	const string	compareExchangeStr		= (m_operation == ATOMIC_OPERATION_COMPARE_EXCHANGE) ? ", 18" + string(uintFormat ? "u" : "") : "";
	const string	atomicInvocation		= getAtomicOperationShaderFuncName(m_operation) + "(u_resultImage, " + atomicCoord + compareExchangeStr + ", " + atomicArgExpr + ")";
	const string	shaderImageFormatStr	= getShaderImageFormatQualifier(m_format);
	const string	shaderImageTypeStr		= getShaderImageType(m_format, m_imageType);

	string source = versionDecl + "\n"
					"precision highp " + shaderImageTypeStr + ";\n"
					"\n"
					"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
					"layout (" + shaderImageFormatStr + ", binding=0) coherent uniform " + shaderImageTypeStr + " u_resultImage;\n"
					"\n"
					"void main (void)\n"
					"{\n"
					"	int gx = int(gl_GlobalInvocationID.x);\n"
					"	int gy = int(gl_GlobalInvocationID.y);\n"
					"	int gz = int(gl_GlobalInvocationID.z);\n"
					"	" + atomicInvocation + ";\n"
					"}\n";

	sourceCollections.glslSources.add(m_name) << glu::ComputeSource(source.c_str());
}

class BinaryAtomicIntermValuesCase : public vkt::TestCase
{
public:
								BinaryAtomicIntermValuesCase   (tcu::TestContext&			testCtx,
																const string&				name,
																const string&				description,
																const ImageType				imageType,
																const tcu::UVec3&			imageSize,
																const tcu::TextureFormat&	format,
																const AtomicOperation		operation,
																const glu::GLSLVersion		glslVersion);

	void						initPrograms				   (SourceCollections&			sourceCollections) const;
	TestInstance*				createInstance				   (Context&					context) const;
private:

	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const tcu::TextureFormat	m_format;
	const AtomicOperation		m_operation;
	const glu::GLSLVersion		m_glslVersion;
};

BinaryAtomicIntermValuesCase::BinaryAtomicIntermValuesCase (TestContext&			testCtx,
															const string&			name,
															const string&			description,
															const ImageType			imageType,
															const tcu::UVec3&		imageSize,
															const TextureFormat&	format,
															const AtomicOperation	operation,
															const glu::GLSLVersion	glslVersion)
	: TestCase		(testCtx, name, description)
	, m_imageType	(imageType)
	, m_imageSize	(imageSize)
	, m_format		(format)
	, m_operation	(operation)
	, m_glslVersion	(glslVersion)
{
}

void BinaryAtomicIntermValuesCase::initPrograms (SourceCollections& sourceCollections) const
{
	const string	versionDecl				= glu::getGLSLVersionDeclaration(m_glslVersion);

	const bool		uintFormat				= isUintFormat(mapTextureFormat(m_format));
	const bool		intFormat				= isIntFormat(mapTextureFormat(m_format));
	const string	colorVecTypeName		= string(uintFormat ? "u" : intFormat ? "i" : "") + "vec4";
	const UVec3		gridSize				= getShaderGridSize(m_imageType, m_imageSize);
	const string	atomicCoord				= getCoordStr(m_imageType, "gx % " + toString(gridSize.x()), "gy", "gz");
	const string	invocationCoord			= getCoordStr(m_imageType, "gx", "gy", "gz");
	const string	atomicArgExpr			= (uintFormat ? "uint" : intFormat ? "int" : "float")
											+ getAtomicFuncArgumentShaderStr(m_operation, "gx", "gy", "gz", IVec3(NUM_INVOCATIONS_PER_PIXEL*gridSize.x(), gridSize.y(), gridSize.z()));

	const string	compareExchangeStr		= (m_operation == ATOMIC_OPERATION_COMPARE_EXCHANGE) ? ", 18" + string(uintFormat ? "u" : "")  : "";
	const string	atomicInvocation		= getAtomicOperationShaderFuncName(m_operation) + "(u_resultImage, " + atomicCoord + compareExchangeStr + ", " + atomicArgExpr + ")";
	const string	shaderImageFormatStr	= getShaderImageFormatQualifier(m_format);
	const string	shaderImageTypeStr		= getShaderImageType(m_format, m_imageType);

	string source = versionDecl + "\n"
					"precision highp " + shaderImageTypeStr + ";\n"
					"\n"
					"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
					"layout (" + shaderImageFormatStr + ", binding=0) coherent uniform " + shaderImageTypeStr + " u_resultImage;\n"
					"layout (" + shaderImageFormatStr + ", binding=1) writeonly uniform " + shaderImageTypeStr + " u_intermValuesImage;\n"
					"\n"
					"void main (void)\n"
					"{\n"
					"	int gx = int(gl_GlobalInvocationID.x);\n"
					"	int gy = int(gl_GlobalInvocationID.y);\n"
					"	int gz = int(gl_GlobalInvocationID.z);\n"
					"	imageStore(u_intermValuesImage, " + invocationCoord + ", " + colorVecTypeName + "(" + atomicInvocation + "));\n"
					"}\n";

	sourceCollections.glslSources.add(m_name) << glu::ComputeSource(source.c_str());
}

class BinaryAtomicInstanceBase : public vkt::TestInstance
{
public:

								BinaryAtomicInstanceBase (Context&						context,
														  const string&					name,
														  const ImageType				imageType,
														  const tcu::UVec3&				imageSize,
														  const TextureFormat&			format,
														  const AtomicOperation			operation);

	tcu::TestStatus				iterate					 (void);

	virtual deUint32			getOutputBufferSize		 (void) const = 0;

	virtual void				prepareResources		 (void) = 0;
	virtual void				prepareDescriptors		 (void) = 0;

	virtual void				commandsBeforeCompute	 (const VkCommandBuffer			cmdBuffer) const = 0;
	virtual void				commandsAfterCompute	 (const VkCommandBuffer			cmdBuffer) const = 0;

	virtual bool				verifyResult			 (Allocation&					outputBufferAllocation) const = 0;

protected:
	const string				m_name;
	const ImageType				m_imageType;
	const tcu::UVec3			m_imageSize;
	const TextureFormat			m_format;
	const AtomicOperation		m_operation;

	de::MovePtr<Buffer>			m_outputBuffer;
	Move<VkDescriptorPool>		m_descriptorPool;
	Move<VkDescriptorSetLayout>	m_descriptorSetLayout;
	Move<VkDescriptorSet>		m_descriptorSet;
	de::MovePtr<Image>			m_resultImage;
	Move<VkImageView>			m_resultImageView;
};

BinaryAtomicInstanceBase::BinaryAtomicInstanceBase (Context&				context,
													const string&			name,
													const ImageType			imageType,
													const tcu::UVec3&		imageSize,
													const TextureFormat&	format,
													const AtomicOperation	operation)
	: vkt::TestInstance	(context)
	, m_name			(name)
	, m_imageType		(imageType)
	, m_imageSize		(imageSize)
	, m_format			(format)
	, m_operation		(operation)
{
}

tcu::TestStatus	BinaryAtomicInstanceBase::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	deviceInterface		= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();
	const VkDeviceSize		imageSizeInBytes	= tcu::getPixelSize(m_format) * getNumPixels(m_imageType, m_imageSize);
	const VkDeviceSize		outBuffSizeInBytes	= getOutputBufferSize();

	const VkImageCreateInfo imageParams	=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,					// VkStructureType			sType;
		DE_NULL,												// const void*				pNext;
		(m_imageType == IMAGE_TYPE_CUBE ||
		 m_imageType == IMAGE_TYPE_CUBE_ARRAY ?
		 (VkImageCreateFlags)VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT :
		 (VkImageCreateFlags)0u),								// VkImageCreateFlags		flags;
		mapImageType(m_imageType),								// VkImageType				imageType;
		mapTextureFormat(m_format),								// VkFormat					format;
		makeExtent3D(getLayerSize(m_imageType, m_imageSize)),	// VkExtent3D				extent;
		1u,														// deUint32					mipLevels;
		getNumLayers(m_imageType, m_imageSize),					// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,									// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,								// VkImageTiling			tiling;
		VK_IMAGE_USAGE_STORAGE_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT,						// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,								// VkSharingMode			sharingMode;
		0u,														// deUint32					queueFamilyIndexCount;
		DE_NULL,												// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,								// VkImageLayout			initialLayout;
	};

	//Create the image that is going to store results of atomic operations
	m_resultImage = de::MovePtr<Image>(new Image(deviceInterface, device, allocator, imageParams, MemoryRequirement::Any));

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, getNumLayers(m_imageType, m_imageSize));

	m_resultImageView = makeImageView(deviceInterface, device, m_resultImage->get(), mapImageViewType(m_imageType), mapTextureFormat(m_format), subresourceRange);

	//Prepare the buffer with the initial data for the image
	const Buffer inputBuffer(deviceInterface, device, allocator, makeBufferCreateInfo(imageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT), MemoryRequirement::HostVisible);

	Allocation& inputBufferAllocation = inputBuffer.getAllocation();

	//Prepare the initial data for the image
	const tcu::IVec4 initialValue(getOperationInitialValue(m_operation));

	tcu::UVec3 gridSize = getShaderGridSize(m_imageType, m_imageSize);
	tcu::PixelBufferAccess inputPixelBuffer(m_format, gridSize.x(), gridSize.y(), gridSize.z(), inputBufferAllocation.getHostPtr());

	for (deUint32 z = 0; z < gridSize.z(); z++)
	for (deUint32 y = 0; y < gridSize.y(); y++)
	for (deUint32 x = 0; x < gridSize.x(); x++)
	{
		inputPixelBuffer.setPixel(initialValue, x, y, z);
	}

	flushMappedMemoryRange(deviceInterface, device, inputBufferAllocation.getMemory(), inputBufferAllocation.getOffset(), imageSizeInBytes);

	// Create a buffer to store shader output copied from result image
	m_outputBuffer = de::MovePtr<Buffer>(new Buffer(deviceInterface, device, allocator, makeBufferCreateInfo(outBuffSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible));

	prepareResources();

	prepareDescriptors();

	// Create pipeline
	const Unique<VkShaderModule>	shaderModule(createShaderModule(deviceInterface, device, m_context.getBinaryCollection().get(m_name), 0));
	const Unique<VkPipelineLayout>	pipelineLayout(makePipelineLayout(deviceInterface, device, *m_descriptorSetLayout));
	const Unique<VkPipeline>		pipeline(makeComputePipeline(deviceInterface, device, *pipelineLayout, *shaderModule));

	// Create command buffer
	const Unique<VkCommandPool>		cmdPool(createCommandPool(deviceInterface, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer(allocateCommandBuffer(deviceInterface, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	beginCommandBuffer(deviceInterface, *cmdBuffer);

	deviceInterface.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	deviceInterface.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);

	const VkBufferMemoryBarrier inputBufferPostHostWriteBarrier	=
		makeBufferMemoryBarrier(VK_ACCESS_HOST_WRITE_BIT,
								VK_ACCESS_TRANSFER_READ_BIT,
								*inputBuffer,
								0ull,
								imageSizeInBytes);

	const VkImageMemoryBarrier	resultImagePreCopyBarrier =
		makeImageMemoryBarrier(	0u,
								VK_ACCESS_TRANSFER_WRITE_BIT,
								VK_IMAGE_LAYOUT_UNDEFINED,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								m_resultImage->get(),
								subresourceRange);

	deviceInterface.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, DE_FALSE, 0u, DE_NULL, 1u, &inputBufferPostHostWriteBarrier, 1u, &resultImagePreCopyBarrier);

	const VkBufferImageCopy		bufferImageCopyParams = makeBufferImageCopy(makeExtent3D(getLayerSize(m_imageType, m_imageSize)), getNumLayers(m_imageType, m_imageSize));

	deviceInterface.cmdCopyBufferToImage(*cmdBuffer, *inputBuffer, m_resultImage->get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &bufferImageCopyParams);

	const VkImageMemoryBarrier	resultImagePostCopyBarrier	=
		makeImageMemoryBarrier(	VK_ACCESS_TRANSFER_WRITE_BIT,
								VK_ACCESS_SHADER_READ_BIT,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								VK_IMAGE_LAYOUT_GENERAL,
								m_resultImage->get(),
								subresourceRange);

	deviceInterface.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, 0u, DE_NULL, 0u, DE_NULL, 1u, &resultImagePostCopyBarrier);

	commandsBeforeCompute(*cmdBuffer);

	deviceInterface.cmdDispatch(*cmdBuffer, NUM_INVOCATIONS_PER_PIXEL*gridSize.x(), gridSize.y(), gridSize.z());

	commandsAfterCompute(*cmdBuffer);

	const VkBufferMemoryBarrier	outputBufferPreHostReadBarrier
		= makeBufferMemoryBarrier(	VK_ACCESS_TRANSFER_WRITE_BIT,
									VK_ACCESS_HOST_READ_BIT,
									m_outputBuffer->get(),
									0ull,
									outBuffSizeInBytes);

	deviceInterface.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, 0u, DE_NULL, 1u, &outputBufferPreHostReadBarrier, 0u, DE_NULL);

	endCommandBuffer(deviceInterface, *cmdBuffer);

	submitCommandsAndWait(deviceInterface, device, queue, *cmdBuffer);

	Allocation& outputBufferAllocation = m_outputBuffer->getAllocation();

	invalidateMappedMemoryRange(deviceInterface, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outBuffSizeInBytes);

	if (verifyResult(outputBufferAllocation))
		return tcu::TestStatus::pass("Comparison succeeded");
	else
		return tcu::TestStatus::fail("Comparison failed");
}

class BinaryAtomicEndResultInstance : public BinaryAtomicInstanceBase
{
public:

						BinaryAtomicEndResultInstance  (Context&				context,
														const string&			name,
														const ImageType			imageType,
														const tcu::UVec3&		imageSize,
														const TextureFormat&	format,
														const AtomicOperation	operation)
							: BinaryAtomicInstanceBase(context, name, imageType, imageSize, format, operation) {}

	virtual deUint32	getOutputBufferSize			   (void) const;

	virtual void		prepareResources			   (void) {}
	virtual void		prepareDescriptors			   (void);

	virtual void		commandsBeforeCompute		   (const VkCommandBuffer) const {}
	virtual void		commandsAfterCompute		   (const VkCommandBuffer	cmdBuffer) const;

	virtual bool		verifyResult				   (Allocation&				outputBufferAllocation) const;
};

deUint32 BinaryAtomicEndResultInstance::getOutputBufferSize (void) const
{
	return tcu::getPixelSize(m_format) * getNumPixels(m_imageType, m_imageSize);
}

void BinaryAtomicEndResultInstance::prepareDescriptors (void)
{
	const VkDevice			device			= m_context.getDevice();
	const DeviceInterface&	deviceInterface = m_context.getDeviceInterface();

	m_descriptorSetLayout =
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(deviceInterface, device);

	m_descriptorPool =
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(deviceInterface, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	m_descriptorSet = makeDescriptorSet(deviceInterface, device, *m_descriptorPool, *m_descriptorSetLayout);

	const VkDescriptorImageInfo	descResultImageInfo = makeDescriptorImageInfo(DE_NULL, *m_resultImageView, VK_IMAGE_LAYOUT_GENERAL);

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descResultImageInfo)
		.update(deviceInterface, device);
}

void BinaryAtomicEndResultInstance::commandsAfterCompute (const VkCommandBuffer	cmdBuffer) const
{
	const DeviceInterface&			deviceInterface		= m_context.getDeviceInterface();
	const VkImageSubresourceRange	subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, getNumLayers(m_imageType, m_imageSize));

	const VkImageMemoryBarrier	resultImagePostDispatchBarrier =
		makeImageMemoryBarrier(	VK_ACCESS_SHADER_WRITE_BIT,
								VK_ACCESS_TRANSFER_READ_BIT,
								VK_IMAGE_LAYOUT_GENERAL,
								VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								m_resultImage->get(),
								subresourceRange);

	deviceInterface.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, DE_FALSE, 0u, DE_NULL, 0u, DE_NULL, 1u, &resultImagePostDispatchBarrier);

	const VkBufferImageCopy		bufferImageCopyParams = makeBufferImageCopy(makeExtent3D(getLayerSize(m_imageType, m_imageSize)), getNumLayers(m_imageType, m_imageSize));

	deviceInterface.cmdCopyImageToBuffer(cmdBuffer, m_resultImage->get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_outputBuffer->get(), 1u, &bufferImageCopyParams);
}

bool BinaryAtomicEndResultInstance::verifyResult (Allocation& outputBufferAllocation) const
{
	const UVec3	gridSize			= getShaderGridSize(m_imageType, m_imageSize);
	const IVec3 extendedGridSize	= IVec3(NUM_INVOCATIONS_PER_PIXEL*gridSize.x(), gridSize.y(), gridSize.z());

	tcu::ConstPixelBufferAccess resultBuffer(m_format, gridSize.x(), gridSize.y(), gridSize.z(), outputBufferAllocation.getHostPtr());

	for (deInt32 z = 0; z < resultBuffer.getDepth();  z++)
	for (deInt32 y = 0; y < resultBuffer.getHeight(); y++)
	for (deInt32 x = 0; x < resultBuffer.getWidth();  x++)
	{
		deInt32 resultValue = resultBuffer.getPixelInt(x, y, z).x();

		if (isOrderIndependentAtomicOperation(m_operation))
		{
			deInt32 reference = getOperationInitialValue(m_operation);

			for (deInt32 i = 0; i < static_cast<deInt32>(NUM_INVOCATIONS_PER_PIXEL); i++)
			{
				const IVec3 gid(x + i*gridSize.x(), y, z);
				reference = computeBinaryAtomicOperationResult(m_operation, reference, getAtomicFuncArgument(m_operation, gid, extendedGridSize));
			}

			if (resultValue != reference)
				return false;
		}
		else if (m_operation == ATOMIC_OPERATION_EXCHANGE)
		{
			// Check if the end result equals one of the atomic args.
			bool matchFound = false;

			for (deInt32 i = 0; i < static_cast<deInt32>(NUM_INVOCATIONS_PER_PIXEL) && !matchFound; i++)
			{
				const IVec3 gid(x + i*gridSize.x(), y, z);
				matchFound = (resultValue == getAtomicFuncArgument(m_operation, gid, extendedGridSize));
			}

			if (!matchFound)
				return false;
		}
		else if (m_operation == ATOMIC_OPERATION_COMPARE_EXCHANGE)
		{
			// Check if the end result equals one of the atomic args.
			bool matchFound = false;

			for (deInt32 i = 0; i < static_cast<deInt32>(NUM_INVOCATIONS_PER_PIXEL) && !matchFound; i++)
			{
				const IVec3 gid(x + i*gridSize.x(), y, z);
				matchFound = (resultValue == getAtomicFuncArgument(m_operation, gid, extendedGridSize));
			}

			if (!matchFound)
				return false;
		}
		else
			DE_ASSERT(false);
	}
	return true;
}

TestInstance* BinaryAtomicEndResultCase::createInstance (Context& context) const
{
	return new BinaryAtomicEndResultInstance(context, m_name, m_imageType, m_imageSize, m_format, m_operation);
}

class BinaryAtomicIntermValuesInstance : public BinaryAtomicInstanceBase
{
public:

						BinaryAtomicIntermValuesInstance   (Context&				context,
															const string&			name,
															const ImageType			imageType,
															const tcu::UVec3&		imageSize,
															const TextureFormat&	format,
															const AtomicOperation	operation)
							: BinaryAtomicInstanceBase(context, name, imageType, imageSize, format, operation) {}

	virtual deUint32	getOutputBufferSize				   (void) const;

	virtual void		prepareResources				   (void);
	virtual void		prepareDescriptors				   (void);

	virtual void		commandsBeforeCompute			   (const VkCommandBuffer	cmdBuffer) const;
	virtual void		commandsAfterCompute			   (const VkCommandBuffer	cmdBuffer) const;

	virtual bool		verifyResult					   (Allocation&				outputBufferAllocation) const;

protected:

	bool				verifyRecursive					   (const deInt32			index,
															const deInt32			valueSoFar,
															bool					argsUsed[NUM_INVOCATIONS_PER_PIXEL],
															const deInt32			atomicArgs[NUM_INVOCATIONS_PER_PIXEL],
															const deInt32			resultValues[NUM_INVOCATIONS_PER_PIXEL]) const;
	de::MovePtr<Image>	m_intermResultsImage;
	Move<VkImageView>	m_intermResultsImageView;
};

deUint32 BinaryAtomicIntermValuesInstance::getOutputBufferSize (void) const
{
	return NUM_INVOCATIONS_PER_PIXEL * tcu::getPixelSize(m_format) * getNumPixels(m_imageType, m_imageSize);
}

void BinaryAtomicIntermValuesInstance::prepareResources (void)
{
	const VkDevice			device			= m_context.getDevice();
	const DeviceInterface&	deviceInterface = m_context.getDeviceInterface();
	Allocator&				allocator		= m_context.getDefaultAllocator();

	const UVec3 layerSize			= getLayerSize(m_imageType, m_imageSize);
	const bool  isCubeBasedImage	= (m_imageType == IMAGE_TYPE_CUBE || m_imageType == IMAGE_TYPE_CUBE_ARRAY);
	const UVec3 extendedLayerSize	= isCubeBasedImage	? UVec3(NUM_INVOCATIONS_PER_PIXEL * layerSize.x(), NUM_INVOCATIONS_PER_PIXEL * layerSize.y(), layerSize.z())
														: UVec3(NUM_INVOCATIONS_PER_PIXEL * layerSize.x(), layerSize.y(), layerSize.z());

	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		(m_imageType == IMAGE_TYPE_CUBE ||
		 m_imageType == IMAGE_TYPE_CUBE_ARRAY ?
		 (VkImageCreateFlags)VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT :
		 (VkImageCreateFlags)0u),					// VkImageCreateFlags		flags;
		mapImageType(m_imageType),					// VkImageType				imageType;
		mapTextureFormat(m_format),					// VkFormat					format;
		makeExtent3D(extendedLayerSize),			// VkExtent3D				extent;
		1u,											// deUint32					mipLevels;
		getNumLayers(m_imageType, m_imageSize),		// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,						// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,					// VkImageTiling			tiling;
		VK_IMAGE_USAGE_STORAGE_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode			sharingMode;
		0u,											// deUint32					queueFamilyIndexCount;
		DE_NULL,									// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,					// VkImageLayout			initialLayout;
	};

	m_intermResultsImage = de::MovePtr<Image>(new Image(deviceInterface, device, allocator, imageParams, MemoryRequirement::Any));

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, getNumLayers(m_imageType, m_imageSize));

	m_intermResultsImageView = makeImageView(deviceInterface, device, m_intermResultsImage->get(), mapImageViewType(m_imageType), mapTextureFormat(m_format), subresourceRange);
}

void BinaryAtomicIntermValuesInstance::prepareDescriptors (void)
{
	const VkDevice			device			= m_context.getDevice();
	const DeviceInterface&	deviceInterface = m_context.getDeviceInterface();

	m_descriptorSetLayout =
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(deviceInterface, device);

	m_descriptorPool =
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2u)
		.build(deviceInterface, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	m_descriptorSet = makeDescriptorSet(deviceInterface, device, *m_descriptorPool, *m_descriptorSetLayout);

	const VkDescriptorImageInfo	descResultImageInfo			= makeDescriptorImageInfo(DE_NULL, *m_resultImageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorImageInfo	descIntermResultsImageInfo	= makeDescriptorImageInfo(DE_NULL, *m_intermResultsImageView, VK_IMAGE_LAYOUT_GENERAL);

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descResultImageInfo)
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descIntermResultsImageInfo)
		.update(deviceInterface, device);
}

void BinaryAtomicIntermValuesInstance::commandsBeforeCompute (const VkCommandBuffer cmdBuffer) const
{
	const DeviceInterface&			deviceInterface		= m_context.getDeviceInterface();
	const VkImageSubresourceRange	subresourceRange	= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, getNumLayers(m_imageType, m_imageSize));

	const VkImageMemoryBarrier	imagePreDispatchBarrier =
		makeImageMemoryBarrier(	0u,
								VK_ACCESS_SHADER_WRITE_BIT,
								VK_IMAGE_LAYOUT_UNDEFINED,
								VK_IMAGE_LAYOUT_GENERAL,
								m_intermResultsImage->get(),
								subresourceRange);

	deviceInterface.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, 0u, DE_NULL, 0u, DE_NULL, 1u, &imagePreDispatchBarrier);
}

void BinaryAtomicIntermValuesInstance::commandsAfterCompute (const VkCommandBuffer cmdBuffer) const
{
	const DeviceInterface&			deviceInterface		= m_context.getDeviceInterface();
	const VkImageSubresourceRange	subresourceRange	= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, getNumLayers(m_imageType, m_imageSize));

	const VkImageMemoryBarrier	imagePostDispatchBarrier =
		makeImageMemoryBarrier(	VK_ACCESS_SHADER_WRITE_BIT,
								VK_ACCESS_TRANSFER_READ_BIT,
								VK_IMAGE_LAYOUT_GENERAL,
								VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								m_intermResultsImage->get(),
								subresourceRange);

	deviceInterface.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, DE_FALSE, 0u, DE_NULL, 0u, DE_NULL, 1u, &imagePostDispatchBarrier);

	const UVec3					layerSize				= getLayerSize(m_imageType, m_imageSize);
	const UVec3					extendedLayerSize		= UVec3(NUM_INVOCATIONS_PER_PIXEL * layerSize.x(), layerSize.y(), layerSize.z());
	const VkBufferImageCopy		bufferImageCopyParams	= makeBufferImageCopy(makeExtent3D(extendedLayerSize), getNumLayers(m_imageType, m_imageSize));

	deviceInterface.cmdCopyImageToBuffer(cmdBuffer, m_intermResultsImage->get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_outputBuffer->get(), 1u, &bufferImageCopyParams);
}

bool BinaryAtomicIntermValuesInstance::verifyResult (Allocation&	outputBufferAllocation) const
{
	const UVec3	gridSize		 = getShaderGridSize(m_imageType, m_imageSize);
	const IVec3 extendedGridSize = IVec3(NUM_INVOCATIONS_PER_PIXEL*gridSize.x(), gridSize.y(), gridSize.z());

	tcu::ConstPixelBufferAccess resultBuffer(m_format, extendedGridSize.x(), extendedGridSize.y(), extendedGridSize.z(), outputBufferAllocation.getHostPtr());

	for (deInt32 z = 0; z < resultBuffer.getDepth(); z++)
	for (deInt32 y = 0; y < resultBuffer.getHeight(); y++)
	for (deUint32 x = 0; x < gridSize.x(); x++)
	{
		deInt32 resultValues[NUM_INVOCATIONS_PER_PIXEL];
		deInt32 atomicArgs[NUM_INVOCATIONS_PER_PIXEL];
		bool	argsUsed[NUM_INVOCATIONS_PER_PIXEL];

		for (deInt32 i = 0; i < static_cast<deInt32>(NUM_INVOCATIONS_PER_PIXEL); i++)
		{
			IVec3 gid(x + i*gridSize.x(), y, z);

			resultValues[i] = resultBuffer.getPixelInt(gid.x(), gid.y(), gid.z()).x();
			atomicArgs[i]	= getAtomicFuncArgument(m_operation, gid, extendedGridSize);
			argsUsed[i]		= false;
		}

		// Verify that the return values form a valid sequence.
		if (!verifyRecursive(0, getOperationInitialValue(m_operation), argsUsed, atomicArgs, resultValues))
		{
			return false;
		}
	}

	return true;
}

bool BinaryAtomicIntermValuesInstance::verifyRecursive (const deInt32	index,
														const deInt32	valueSoFar,
														bool			argsUsed[NUM_INVOCATIONS_PER_PIXEL],
														const deInt32	atomicArgs[NUM_INVOCATIONS_PER_PIXEL],
														const deInt32	resultValues[NUM_INVOCATIONS_PER_PIXEL]) const
{
	if (index >= static_cast<deInt32>(NUM_INVOCATIONS_PER_PIXEL))
		return true;

	for (deInt32 i = 0; i < static_cast<deInt32>(NUM_INVOCATIONS_PER_PIXEL); i++)
	{
		if (!argsUsed[i] && resultValues[i] == valueSoFar)
		{
			argsUsed[i] = true;

			if (verifyRecursive(index + 1, computeBinaryAtomicOperationResult(m_operation, valueSoFar, atomicArgs[i]), argsUsed, atomicArgs, resultValues))
			{
				return true;
			}

			argsUsed[i] = false;
		}
	}

	return false;
}

TestInstance* BinaryAtomicIntermValuesCase::createInstance (Context& context) const
{
	return new BinaryAtomicIntermValuesInstance(context, m_name, m_imageType, m_imageSize, m_format, m_operation);
}

} // anonymous ns

tcu::TestCaseGroup* createImageAtomicOperationTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> imageAtomicOperationsTests(new tcu::TestCaseGroup(testCtx, "atomic_operations", "Atomic image operations cases"));

	struct ImageParams
	{
		ImageParams(const ImageType imageType, const tcu::UVec3& imageSize)
			: m_imageType	(imageType)
			, m_imageSize	(imageSize)
		{
		}
		const ImageType		m_imageType;
		const tcu::UVec3	m_imageSize;
	};

	static const ImageParams imageParamsArray[] =
	{
		ImageParams(IMAGE_TYPE_1D,			tcu::UVec3(64u, 1u, 1u)),
		ImageParams(IMAGE_TYPE_1D_ARRAY,	tcu::UVec3(64u, 1u, 8u)),
		ImageParams(IMAGE_TYPE_2D,			tcu::UVec3(64u, 64u, 1u)),
		ImageParams(IMAGE_TYPE_2D_ARRAY,	tcu::UVec3(64u, 64u, 8u)),
		ImageParams(IMAGE_TYPE_3D,			tcu::UVec3(64u, 64u, 8u)),
		ImageParams(IMAGE_TYPE_CUBE,		tcu::UVec3(64u, 64u, 1u)),
		ImageParams(IMAGE_TYPE_CUBE_ARRAY,	tcu::UVec3(64u, 64u, 2u))
	};

	static const tcu::TextureFormat formats[] =
	{
		tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::UNSIGNED_INT32),
		tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::SIGNED_INT32)
	};

	for (deUint32 operationI = 0; operationI < ATOMIC_OPERATION_LAST; operationI++)
	{
		const AtomicOperation operation = (AtomicOperation)operationI;

		de::MovePtr<tcu::TestCaseGroup> operationGroup(new tcu::TestCaseGroup(testCtx, getAtomicOperationCaseName(operation).c_str(), ""));

		for (deUint32 imageTypeNdx = 0; imageTypeNdx < DE_LENGTH_OF_ARRAY(imageParamsArray); imageTypeNdx++)
		{
			const ImageType	 imageType = imageParamsArray[imageTypeNdx].m_imageType;
			const tcu::UVec3 imageSize = imageParamsArray[imageTypeNdx].m_imageSize;

			de::MovePtr<tcu::TestCaseGroup> imageTypeGroup(new tcu::TestCaseGroup(testCtx, getImageTypeName(imageType).c_str(), ""));

			for (deUint32 formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
			{
				const TextureFormat&	format		= formats[formatNdx];
				const std::string		formatName	= getShaderImageFormatQualifier(format);

				//!< Atomic case checks the end result of the operations, and not the intermediate return values
				const string caseEndResult = formatName + "_end_result";
				imageTypeGroup->addChild(new BinaryAtomicEndResultCase(testCtx, caseEndResult, "", imageType, imageSize, format, operation, glu::GLSL_VERSION_440));

				//!< Atomic case checks the return values of the atomic function and not the end result.
				const string caseIntermValues = formatName + "_intermediate_values";
				imageTypeGroup->addChild(new BinaryAtomicIntermValuesCase(testCtx, caseIntermValues, "", imageType, imageSize, format, operation, glu::GLSL_VERSION_440));
			}

			operationGroup->addChild(imageTypeGroup.release());
		}

		imageAtomicOperationsTests->addChild(operationGroup.release());
	}

	return imageAtomicOperationsTests.release();
}

} // image
} // vkt
