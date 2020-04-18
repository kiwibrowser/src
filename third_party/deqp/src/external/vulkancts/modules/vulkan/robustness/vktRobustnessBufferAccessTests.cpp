/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Imagination Technologies Ltd.
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
 * \brief Robust buffer access tests for uniform/storage buffers and
 *        uniform/storage texel buffers.
 *//*--------------------------------------------------------------------*/

#include "vktRobustnessBufferAccessTests.hpp"
#include "vktRobustnessUtil.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "tcuTestLog.hpp"

#include <limits>
#include <sstream>

namespace vkt
{
namespace robustness
{

using namespace vk;

enum ShaderType
{
	SHADER_TYPE_MATRIX_COPY,
	SHADER_TYPE_VECTOR_COPY,
	SHADER_TYPE_SCALAR_COPY,
	SHADER_TYPE_TEXEL_COPY,

	SHADER_TYPE_COUNT
};

enum BufferAccessType
{
	BUFFER_ACCESS_TYPE_READ,
	BUFFER_ACCESS_TYPE_READ_FROM_STORAGE,
	BUFFER_ACCESS_TYPE_WRITE,
};

static VkDeviceSize min (VkDeviceSize a, VkDeviceSize b)
{
	return (a < b) ? a : b;
}

class RobustBufferAccessTest : public vkt::TestCase
{
public:
	static const deUint32		s_testArraySize;
	static const deUint32		s_numberOfBytesAccessed;

								RobustBufferAccessTest		(tcu::TestContext&		testContext,
															 const std::string&		name,
															 const std::string&		description,
															 VkShaderStageFlags		shaderStage,
															 ShaderType				shaderType,
															 VkFormat				bufferFormat);

	virtual						~RobustBufferAccessTest		(void) {}

private:
	static void					genBufferShaderAccess		(ShaderType				shaderType,
															 VkFormat				bufferFormat,
															 bool					readFromStorage,
															 std::ostringstream&	bufferDefinition,
															 std::ostringstream&	bufferUse);

	static void					genTexelBufferShaderAccess	(VkFormat				bufferFormat,
															 std::ostringstream&	bufferDefinition,
															 std::ostringstream&	bufferUse,
															 bool					readFromStorage);

protected:
	static void					initBufferAccessPrograms	(SourceCollections&		programCollection,
															 VkShaderStageFlags		shaderStage,
															 ShaderType				shaderType,
															 VkFormat				bufferFormat,
															 bool					readFromStorage);

	const VkShaderStageFlags	m_shaderStage;
	const ShaderType			m_shaderType;
	const VkFormat				m_bufferFormat;
};

class RobustBufferReadTest : public RobustBufferAccessTest
{
public:
								RobustBufferReadTest		(tcu::TestContext&		testContext,
															 const std::string&		name,
															 const std::string&		description,
															 VkShaderStageFlags		shaderStage,
															 ShaderType				shaderType,
															 VkFormat				bufferFormat,
															 VkDeviceSize			readAccessRange,
															 bool					readFromStorage,
															 bool					accessOutOfBackingMemory);

	virtual						~RobustBufferReadTest		(void) {}

	virtual void				initPrograms				(SourceCollections& programCollection) const;
	virtual TestInstance*		createInstance				(Context& context) const;

private:
	const bool					m_readFromStorage;
	const VkDeviceSize			m_readAccessRange;
	const bool					m_accessOutOfBackingMemory;
};

class RobustBufferWriteTest : public RobustBufferAccessTest
{
public:
								RobustBufferWriteTest		(tcu::TestContext&		testContext,
															 const std::string&		name,
															 const std::string&		description,
															 VkShaderStageFlags		shaderStage,
															 ShaderType				shaderType,
															 VkFormat				bufferFormat,
															 VkDeviceSize			writeAccessRange,
															 bool					accessOutOfBackingMemory);

	virtual						~RobustBufferWriteTest		(void) {}

	virtual void				initPrograms				(SourceCollections& programCollection) const;
	virtual TestInstance*		createInstance				(Context& context) const;

private:
	const VkDeviceSize			m_writeAccessRange;
	const bool					m_accessOutOfBackingMemory;
};

class BufferAccessInstance : public vkt::TestInstance
{
public:
									BufferAccessInstance			(Context&			context,
																	 Move<VkDevice>		device,
																	 ShaderType			shaderType,
																	 VkShaderStageFlags	shaderStage,
																	 VkFormat			bufferFormat,
																	 BufferAccessType	bufferAccessType,
																	 VkDeviceSize		inBufferAccessRange,
																	 VkDeviceSize		outBufferAccessRange,
																	 bool				accessOutOfBackingMemory);

	virtual							~BufferAccessInstance			(void) {}

	virtual tcu::TestStatus			iterate							(void);

	virtual bool					verifyResult					(void);

private:
	bool							isExpectedValueFromInBuffer		(VkDeviceSize offsetInBytes, const void* valuePtr, VkDeviceSize valueSize);
	bool							isOutBufferValueUnchanged		(VkDeviceSize offsetInBytes, VkDeviceSize valueSize);

protected:
	Move<VkDevice>					m_device;
	de::MovePtr<TestEnvironment>	m_testEnvironment;

	const ShaderType				m_shaderType;
	const VkShaderStageFlags		m_shaderStage;

	const VkFormat					m_bufferFormat;
	const BufferAccessType			m_bufferAccessType;

	const VkDeviceSize				m_inBufferAccessRange;
	Move<VkBuffer>					m_inBuffer;
	de::MovePtr<Allocation>			m_inBufferAlloc;
	VkDeviceSize					m_inBufferAllocSize;
	VkDeviceSize					m_inBufferMaxAccessRange;

	const VkDeviceSize				m_outBufferAccessRange;
	Move<VkBuffer>					m_outBuffer;
	de::MovePtr<Allocation>			m_outBufferAlloc;
	VkDeviceSize					m_outBufferAllocSize;
	VkDeviceSize					m_outBufferMaxAccessRange;

	Move<VkBuffer>					m_indicesBuffer;
	de::MovePtr<Allocation>			m_indicesBufferAlloc;

	Move<VkDescriptorPool>			m_descriptorPool;
	Move<VkDescriptorSetLayout>		m_descriptorSetLayout;
	Move<VkDescriptorSet>			m_descriptorSet;

	Move<VkFence>					m_fence;
	VkQueue							m_queue;

	// Used when m_shaderStage == VK_SHADER_STAGE_VERTEX_BIT
	Move<VkBuffer>					m_vertexBuffer;
	de::MovePtr<Allocation>			m_vertexBufferAlloc;

	// Used when m_shaderType == SHADER_TYPE_TEXEL_COPY
	Move<VkBufferView>				m_inTexelBufferView;
	Move<VkBufferView>				m_outTexelBufferView;

	const bool						m_accessOutOfBackingMemory;
};

class BufferReadInstance: public BufferAccessInstance
{
public:
									BufferReadInstance			(Context&				context,
																 Move<VkDevice>			device,
																 ShaderType				shaderType,
																 VkShaderStageFlags		shaderStage,
																 VkFormat				bufferFormat,
																 bool					readFromStorage,
																 VkDeviceSize			inBufferAccessRange,
																 bool					accessOutOfBackingMemory);

	virtual							~BufferReadInstance			(void) {}

private:
};

class BufferWriteInstance: public BufferAccessInstance
{
public:
									BufferWriteInstance			(Context&				context,
																 Move<VkDevice>			device,
																 ShaderType				shaderType,
																 VkShaderStageFlags		shaderStage,
																 VkFormat				bufferFormat,
																 VkDeviceSize			writeBufferAccessRange,
																 bool					accessOutOfBackingMemory);

	virtual							~BufferWriteInstance		(void) {}
};

// RobustBufferAccessTest

const deUint32 RobustBufferAccessTest::s_testArraySize = 1024;
const deUint32 RobustBufferAccessTest::s_numberOfBytesAccessed	= (deUint32)(16 * sizeof(float)); // size of mat4

RobustBufferAccessTest::RobustBufferAccessTest (tcu::TestContext&		testContext,
												const std::string&		name,
												const std::string&		description,
												VkShaderStageFlags		shaderStage,
												ShaderType				shaderType,
												VkFormat				bufferFormat)
	: vkt::TestCase		(testContext, name, description)
	, m_shaderStage		(shaderStage)
	, m_shaderType		(shaderType)
	, m_bufferFormat	(bufferFormat)
{
	DE_ASSERT(m_shaderStage == VK_SHADER_STAGE_VERTEX_BIT || m_shaderStage == VK_SHADER_STAGE_FRAGMENT_BIT || m_shaderStage == VK_SHADER_STAGE_COMPUTE_BIT);
}

void RobustBufferAccessTest::genBufferShaderAccess (ShaderType			shaderType,
												    VkFormat			bufferFormat,
												    bool				readFromStorage,
												    std::ostringstream&	bufferDefinition,
												    std::ostringstream&	bufferUse)
{
	if (isFloatFormat(bufferFormat))
	{
		bufferDefinition <<
			"layout(binding = 0, " << (readFromStorage ? "std430" : "std140") << ") " << (readFromStorage ? "buffer" : "uniform") << " InBuffer\n"
			"{\n"
			"	mat4 inMatrix[" << s_testArraySize << "];\n"
			"};\n\n";

		bufferDefinition <<
			"layout(binding = 1, std430) buffer OutBuffer\n"
			"{\n"
			"	mat4 outMatrix[" << s_testArraySize << "];\n"
			"};\n\n";

		bufferDefinition <<
			"layout(binding = 2, std140) uniform Indices\n"
			"{\n"
			"	int inIndex;\n"
			"	int outIndex;\n"
			"};\n\n";

		switch (shaderType)
		{
			case SHADER_TYPE_MATRIX_COPY:
				bufferUse <<
					"	mat4 tmp = inMatrix[inIndex];\n"
					"	outMatrix[outIndex] = tmp;\n";
				break;

			case SHADER_TYPE_VECTOR_COPY:
				bufferUse <<
					"	outMatrix[outIndex][0] = inMatrix[inIndex][0];\n"
					"	outMatrix[outIndex][1] = inMatrix[inIndex][1];\n"
					"	outMatrix[outIndex][2] = inMatrix[inIndex][2];\n"
					"	outMatrix[outIndex][3] = inMatrix[inIndex][3];\n";
				break;

			case SHADER_TYPE_SCALAR_COPY:
				bufferUse <<
					"	outMatrix[outIndex][0][0] = inMatrix[inIndex][0][0];\n"
					"	outMatrix[outIndex][0][1] = inMatrix[inIndex][0][1];\n"
					"	outMatrix[outIndex][0][2] = inMatrix[inIndex][0][2];\n"
					"	outMatrix[outIndex][0][3] = inMatrix[inIndex][0][3];\n"

					"	outMatrix[outIndex][1][0] = inMatrix[inIndex][1][0];\n"
					"	outMatrix[outIndex][1][1] = inMatrix[inIndex][1][1];\n"
					"	outMatrix[outIndex][1][2] = inMatrix[inIndex][1][2];\n"
					"	outMatrix[outIndex][1][3] = inMatrix[inIndex][1][3];\n"

					"	outMatrix[outIndex][2][0] = inMatrix[inIndex][2][0];\n"
					"	outMatrix[outIndex][2][1] = inMatrix[inIndex][2][1];\n"
					"	outMatrix[outIndex][2][2] = inMatrix[inIndex][2][2];\n"
					"	outMatrix[outIndex][2][3] = inMatrix[inIndex][2][3];\n"

					"	outMatrix[outIndex][3][0] = inMatrix[inIndex][3][0];\n"
					"	outMatrix[outIndex][3][1] = inMatrix[inIndex][3][1];\n"
					"	outMatrix[outIndex][3][2] = inMatrix[inIndex][3][2];\n"
					"	outMatrix[outIndex][3][3] = inMatrix[inIndex][3][3];\n";
				break;

			default:
				DE_ASSERT(false);
		}
	}
	else
	{
		std::string typePrefixStr;

		if (isUintFormat(bufferFormat))
		{
			typePrefixStr = "u";
		}
		else if (isIntFormat(bufferFormat))
		{
			typePrefixStr = "i";
		}
		else
		{
			DE_ASSERT(false);
		}

		bufferDefinition <<
			"layout(binding = 0, " << (readFromStorage ? "std430" : "std140") << ") " << (readFromStorage ? "buffer readonly" : "uniform") << " InBuffer\n"
			"{\n"
			"	" << typePrefixStr << "vec4 inVecs[" << s_testArraySize << "][4];\n"
			"};\n\n";

		bufferDefinition <<
			"layout(binding = 1, std430) buffer OutBuffer\n"
			"{\n"
			"	" << typePrefixStr << "vec4 outVecs[" << s_testArraySize << "][4];\n"
			"};\n\n";

		bufferDefinition <<
			"layout(binding = 2, std140) uniform Indices\n"
			"{\n"
			"	int inIndex;\n"
			"	int outIndex;\n"
			"};\n\n";

		switch (shaderType)
		{
			case SHADER_TYPE_MATRIX_COPY:
				// Shader type not supported for integer types.
				DE_ASSERT(false);
				break;

			case SHADER_TYPE_VECTOR_COPY:
				bufferUse <<
					"	outVecs[outIndex][0] = inVecs[inIndex][0];\n"
					"	outVecs[outIndex][1] = inVecs[inIndex][1];\n"
					"	outVecs[outIndex][2] = inVecs[inIndex][2];\n"
					"	outVecs[outIndex][3] = inVecs[inIndex][3];\n";
				break;

			case SHADER_TYPE_SCALAR_COPY:
				bufferUse <<
					"	outVecs[outIndex][0][0] = inVecs[inIndex][0][0];\n"
					"	outVecs[outIndex][0][1] = inVecs[inIndex][0][1];\n"
					"	outVecs[outIndex][0][2] = inVecs[inIndex][0][2];\n"
					"	outVecs[outIndex][0][3] = inVecs[inIndex][0][3];\n"

					"	outVecs[outIndex][1][0] = inVecs[inIndex][1][0];\n"
					"	outVecs[outIndex][1][1] = inVecs[inIndex][1][1];\n"
					"	outVecs[outIndex][1][2] = inVecs[inIndex][1][2];\n"
					"	outVecs[outIndex][1][3] = inVecs[inIndex][1][3];\n"

					"	outVecs[outIndex][2][0] = inVecs[inIndex][2][0];\n"
					"	outVecs[outIndex][2][1] = inVecs[inIndex][2][1];\n"
					"	outVecs[outIndex][2][2] = inVecs[inIndex][2][2];\n"
					"	outVecs[outIndex][2][3] = inVecs[inIndex][2][3];\n"

					"	outVecs[outIndex][3][0] = inVecs[inIndex][3][0];\n"
					"	outVecs[outIndex][3][1] = inVecs[inIndex][3][1];\n"
					"	outVecs[outIndex][3][2] = inVecs[inIndex][3][2];\n"
					"	outVecs[outIndex][3][3] = inVecs[inIndex][3][3];\n";
				break;

			default:
				DE_ASSERT(false);
		}
	}
}

void RobustBufferAccessTest::genTexelBufferShaderAccess (VkFormat				bufferFormat,
														 std::ostringstream&	bufferDefinition,
														 std::ostringstream&	bufferUse,
														 bool					readFromStorage)
{
	const char*		layoutTypeStr;
	const char*		inTexelBufferTypeStr;
	const char*		outTexelBufferTypeStr;
	const deUint32	texelSize				= mapVkFormat(bufferFormat).getPixelSize();

	if (isFloatFormat(bufferFormat))
	{
		layoutTypeStr			= "rgba32f";
		inTexelBufferTypeStr	= readFromStorage ? "imageBuffer" : "samplerBuffer";
		outTexelBufferTypeStr	= "imageBuffer";
	}
	else if (isUintFormat(bufferFormat))
	{
		layoutTypeStr			= "rgba32ui";
		inTexelBufferTypeStr	= readFromStorage ? "uimageBuffer" : "usamplerBuffer";
		outTexelBufferTypeStr	= "uimageBuffer";
	}
	else if (isIntFormat(bufferFormat))
	{
		layoutTypeStr			= "rgba32i";
		inTexelBufferTypeStr	= readFromStorage ? "iimageBuffer" : "isamplerBuffer";
		outTexelBufferTypeStr	= "iimageBuffer";
	}
	else if (bufferFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
	{
		layoutTypeStr			= "rgb10_a2";
		inTexelBufferTypeStr	= readFromStorage ? "imageBuffer" : "samplerBuffer"; outTexelBufferTypeStr	= "imageBuffer";
	}
	else
	{
		TCU_THROW(NotSupportedError, (std::string("Unsupported format: ") + getFormatName(bufferFormat)).c_str());
	}

	bufferDefinition << "layout(set = 0, binding = 0" << ((readFromStorage) ? (std::string(", ") + layoutTypeStr) : "") << ") uniform highp "
					 << ((readFromStorage) ? "readonly " : "") << inTexelBufferTypeStr << " inImage;\n";

	bufferDefinition << "layout(set = 0, binding = 1, " << layoutTypeStr << ") uniform highp writeonly " << outTexelBufferTypeStr << " outImage;\n";

	bufferDefinition <<
		"layout(binding = 2, std140) uniform Offsets\n"
		"{\n"
		"	int inOffset;\n"
		"	int outOffset;\n"
		"};\n\n";

	bufferUse << "	for (int i = 0; i < " << (s_numberOfBytesAccessed / texelSize) << "; i++)\n"
			  << "	{\n"
			  << "		imageStore(outImage, outOffset + i, " << (readFromStorage ? "imageLoad" : "texelFetch") << "(inImage, inOffset + i));\n"
			  << "	}\n";
}

void RobustBufferAccessTest::initBufferAccessPrograms (SourceCollections&	programCollection,
													   VkShaderStageFlags	shaderStage,
													   ShaderType			shaderType,
													   VkFormat				bufferFormat,
													   bool					readFromStorage)
{
	std::ostringstream	bufferDefinition;
	std::ostringstream	bufferUse;

	if (shaderType != SHADER_TYPE_TEXEL_COPY)
	{
		genBufferShaderAccess(shaderType, bufferFormat, readFromStorage, bufferDefinition, bufferUse);
	}

	if (shaderStage == VK_SHADER_STAGE_COMPUTE_BIT)
	{
		std::ostringstream computeShaderSource;

		if (shaderType == SHADER_TYPE_TEXEL_COPY)
			genTexelBufferShaderAccess(bufferFormat, bufferDefinition, bufferUse, readFromStorage);

		computeShaderSource <<
			"#version 440\n"
			"#extension GL_EXT_texture_buffer : require\n"
			"precision highp float;\n"
			"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
			<< bufferDefinition.str() <<
			"void main (void)\n"
			"{\n"
			<< bufferUse.str() <<
			"}\n";

		programCollection.glslSources.add("compute") << glu::ComputeSource(computeShaderSource.str());
	}
	else
	{
		std::ostringstream vertexShaderSource;
		std::ostringstream fragmentShaderSource;

		if (shaderStage == VK_SHADER_STAGE_VERTEX_BIT)
		{
			if (shaderType == SHADER_TYPE_TEXEL_COPY)
				genTexelBufferShaderAccess(bufferFormat, bufferDefinition, bufferUse, readFromStorage);

			vertexShaderSource <<
				"#version 440\n"
				"#extension GL_EXT_texture_buffer : require\n"
				"precision highp float;\n"
				"layout(location = 0) in vec4 position;\n\n"
				<< bufferDefinition.str() << "\n"
				"out gl_PerVertex {\n"
				"	vec4 gl_Position;\n"
				"};\n\n"
				"void main (void)\n"
				"{\n"
				<< bufferUse.str() <<
				"	gl_Position = position;\n"
				"}\n";
		}
		else
		{
			vertexShaderSource <<
				"#version 440\n"
				"precision highp float;\n"
				"layout(location = 0) in vec4 position;\n\n"
				"out gl_PerVertex {\n"
				"	vec4 gl_Position;\n"
				"};\n\n"
				"void main (void)\n"
				"{\n"
				"	gl_Position = position;\n"
				"}\n";
		}

		programCollection.glslSources.add("vertex") << glu::VertexSource(vertexShaderSource.str());

		if (shaderStage == VK_SHADER_STAGE_FRAGMENT_BIT)
		{
			if (shaderType == SHADER_TYPE_TEXEL_COPY)
				genTexelBufferShaderAccess(bufferFormat, bufferDefinition, bufferUse, readFromStorage);

			fragmentShaderSource <<
				"#version 440\n"
				"#extension GL_EXT_texture_buffer : require\n"
				"precision highp float;\n"
				"layout(location = 0) out vec4 fragColor;\n"
				<< bufferDefinition.str() <<
				"void main (void)\n"
				"{\n"
				<< bufferUse.str() <<
				"	fragColor = vec4(1.0);\n"
				"}\n";
		}
		else
		{
			fragmentShaderSource <<
				"#version 440\n"
				"precision highp float;\n"
				"layout(location = 0) out vec4 fragColor;\n\n"
				"void main (void)\n"
				"{\n"
				"	fragColor = vec4(1.0);\n"
				"}\n";
		}

		programCollection.glslSources.add("fragment") << glu::FragmentSource(fragmentShaderSource.str());
	}
}

// RobustBufferReadTest

RobustBufferReadTest::RobustBufferReadTest (tcu::TestContext&	testContext,
											const std::string&	name,
											const std::string&	description,
											VkShaderStageFlags	shaderStage,
											ShaderType			shaderType,
											VkFormat			bufferFormat,
											VkDeviceSize		readAccessRange,
											bool				readFromStorage,
											bool				accessOutOfBackingMemory)
	: RobustBufferAccessTest		(testContext, name, description, shaderStage, shaderType, bufferFormat)
	, m_readFromStorage				(readFromStorage)
	, m_readAccessRange				(readAccessRange)
	, m_accessOutOfBackingMemory	(accessOutOfBackingMemory)
{
}

void RobustBufferReadTest::initPrograms (SourceCollections& programCollection) const
{
	initBufferAccessPrograms(programCollection, m_shaderStage, m_shaderType, m_bufferFormat, m_readFromStorage);
}

TestInstance* RobustBufferReadTest::createInstance (Context& context) const
{
	Move<VkDevice>	device			= createRobustBufferAccessDevice(context);

	return new BufferReadInstance(context, device, m_shaderType, m_shaderStage, m_bufferFormat, m_readFromStorage, m_readAccessRange, m_accessOutOfBackingMemory);
}

// RobustBufferWriteTest

RobustBufferWriteTest::RobustBufferWriteTest (tcu::TestContext&		testContext,
											  const std::string&	name,
											  const std::string&	description,
											  VkShaderStageFlags	shaderStage,
											  ShaderType			shaderType,
											  VkFormat				bufferFormat,
											  VkDeviceSize			writeAccessRange,
											  bool					accessOutOfBackingMemory)

	: RobustBufferAccessTest		(testContext, name, description, shaderStage, shaderType, bufferFormat)
	, m_writeAccessRange			(writeAccessRange)
	, m_accessOutOfBackingMemory	(accessOutOfBackingMemory)
{
}

void RobustBufferWriteTest::initPrograms (SourceCollections& programCollection) const
{
	initBufferAccessPrograms(programCollection, m_shaderStage, m_shaderType, m_bufferFormat, false /* readFromStorage */);
}

TestInstance* RobustBufferWriteTest::createInstance (Context& context) const
{
	Move<VkDevice>	device			= createRobustBufferAccessDevice(context);

	return new BufferWriteInstance(context, device, m_shaderType, m_shaderStage, m_bufferFormat, m_writeAccessRange, m_accessOutOfBackingMemory);
}

// BufferAccessInstance

BufferAccessInstance::BufferAccessInstance (Context&			context,
											Move<VkDevice>		device,
											ShaderType			shaderType,
											VkShaderStageFlags	shaderStage,
											VkFormat			bufferFormat,
											BufferAccessType	bufferAccessType,
											VkDeviceSize		inBufferAccessRange,
											VkDeviceSize		outBufferAccessRange,
											bool				accessOutOfBackingMemory)
	: vkt::TestInstance				(context)
	, m_device						(device)
	, m_shaderType					(shaderType)
	, m_shaderStage					(shaderStage)
	, m_bufferFormat				(bufferFormat)
	, m_bufferAccessType			(bufferAccessType)
	, m_inBufferAccessRange			(inBufferAccessRange)
	, m_outBufferAccessRange		(outBufferAccessRange)
	, m_accessOutOfBackingMemory	(accessOutOfBackingMemory)
{
	const DeviceInterface&		vk						= context.getDeviceInterface();
	const deUint32				queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	const bool					isTexelAccess			= !!(m_shaderType == SHADER_TYPE_TEXEL_COPY);
	const bool					readFromStorage			= !!(m_bufferAccessType == BUFFER_ACCESS_TYPE_READ_FROM_STORAGE);
	SimpleAllocator				memAlloc				(vk, *m_device, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));
	tcu::TestLog&				log						= m_context.getTestContext().getLog();

	DE_ASSERT(RobustBufferAccessTest::s_numberOfBytesAccessed % sizeof(deUint32) == 0);
	DE_ASSERT(inBufferAccessRange <= RobustBufferAccessTest::s_numberOfBytesAccessed);
	DE_ASSERT(outBufferAccessRange <= RobustBufferAccessTest::s_numberOfBytesAccessed);

	// Check storage support
	if (shaderStage == VK_SHADER_STAGE_VERTEX_BIT)
	{
		if (!context.getDeviceFeatures().vertexPipelineStoresAndAtomics)
		{
			TCU_THROW(NotSupportedError, "Stores not supported in vertex stage");
		}
	}
	else if (shaderStage == VK_SHADER_STAGE_FRAGMENT_BIT)
	{
		if (!context.getDeviceFeatures().fragmentStoresAndAtomics)
		{
			TCU_THROW(NotSupportedError, "Stores not supported in fragment stage");
		}
	}

	// Check format support
	{
		VkFormatFeatureFlags		requiredFormatFeatures	= 0;
		const VkFormatProperties	formatProperties		= getPhysicalDeviceFormatProperties(context.getInstanceInterface(), context.getPhysicalDevice(), m_bufferFormat);

		if (isTexelAccess)
		{
			requiredFormatFeatures	= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;
		}

		if ((formatProperties.bufferFeatures & requiredFormatFeatures) != requiredFormatFeatures)
		{
			TCU_THROW(NotSupportedError, (std::string("Format cannot be used in uniform and storage") + (isTexelAccess ? " texel" : "") + " buffers: "
										  + getFormatName(m_bufferFormat)).c_str());
		}
	}

	// Create buffer to read data from
	{
		VkBufferUsageFlags		inBufferUsageFlags;
		VkMemoryRequirements	inBufferMemoryReqs;

		if (isTexelAccess)
		{
			inBufferUsageFlags = readFromStorage ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		}
		else
		{
			inBufferUsageFlags = readFromStorage ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}

		const VkBufferCreateInfo	inBufferParams =
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_inBufferAccessRange,						// VkDeviceSize			size;
			inBufferUsageFlags,							// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32				queueFamilyIndexCount;
			DE_NULL										// const deUint32*		pQueueFamilyIndices;
		};

		m_inBuffer				= createBuffer(vk, *m_device, &inBufferParams);

		inBufferMemoryReqs		= getBufferMemoryRequirements(vk, *m_device, *m_inBuffer);
		m_inBufferAllocSize		= inBufferMemoryReqs.size;
		m_inBufferAlloc			= memAlloc.allocate(inBufferMemoryReqs, MemoryRequirement::HostVisible);

		// Size of the most restrictive bound
		m_inBufferMaxAccessRange = min(m_inBufferAllocSize, min(inBufferParams.size, m_inBufferAccessRange));

		VK_CHECK(vk.bindBufferMemory(*m_device, *m_inBuffer, m_inBufferAlloc->getMemory(), m_inBufferAlloc->getOffset()));
		populateBufferWithTestValues(m_inBufferAlloc->getHostPtr(), m_inBufferAllocSize, m_bufferFormat);
		flushMappedMemoryRange(vk, *m_device, m_inBufferAlloc->getMemory(), m_inBufferAlloc->getOffset(), VK_WHOLE_SIZE);

		log << tcu::TestLog::Message << "inBufferAllocSize = " << m_inBufferAllocSize << tcu::TestLog::EndMessage;
		log << tcu::TestLog::Message << "inBufferMaxAccessRange = " << m_inBufferMaxAccessRange << tcu::TestLog::EndMessage;
	}

	// Create buffer to write data into
	{
		VkMemoryRequirements		outBufferMemoryReqs;
		const VkBufferUsageFlags	outBufferUsageFlags	= (m_shaderType == SHADER_TYPE_TEXEL_COPY) ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT
																								   : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		const VkBufferCreateInfo	outBufferParams		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_outBufferAccessRange,						// VkDeviceSize			size;
			outBufferUsageFlags,						// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32				queueFamilyIndexCount;
			DE_NULL										// const deUint32*		pQueueFamilyIndices;
		};

		m_outBuffer					= createBuffer(vk, *m_device, &outBufferParams);

		outBufferMemoryReqs			= getBufferMemoryRequirements(vk, *m_device, *m_outBuffer);
		m_outBufferAllocSize		= outBufferMemoryReqs.size;
		m_outBufferAlloc			= memAlloc.allocate(outBufferMemoryReqs, MemoryRequirement::HostVisible);

		// If we are requesting access out of the memory that backs the buffer, make sure the test is able to do so.
		if (m_accessOutOfBackingMemory)
		{
			if (m_outBufferAllocSize >= ((RobustBufferAccessTest::s_testArraySize + 1) * RobustBufferAccessTest::s_numberOfBytesAccessed))
			{
				TCU_THROW(NotSupportedError, "Cannot access beyond the end of the memory that backs the buffer");
			}
		}

		// Size of the most restrictive bound
		m_outBufferMaxAccessRange = min(m_outBufferAllocSize, min(outBufferParams.size, m_outBufferAccessRange));

		VK_CHECK(vk.bindBufferMemory(*m_device, *m_outBuffer, m_outBufferAlloc->getMemory(), m_outBufferAlloc->getOffset()));
		deMemset(m_outBufferAlloc->getHostPtr(), 0xFF, (size_t)m_outBufferAllocSize);
		flushMappedMemoryRange(vk, *m_device, m_outBufferAlloc->getMemory(), m_outBufferAlloc->getOffset(), VK_WHOLE_SIZE);

		log << tcu::TestLog::Message << "outBufferAllocSize = " << m_outBufferAllocSize << tcu::TestLog::EndMessage;
		log << tcu::TestLog::Message << "outBufferMaxAccessRange = " << m_outBufferMaxAccessRange << tcu::TestLog::EndMessage;
	}

	// Create buffer for indices/offsets
	{
		struct IndicesBuffer
		{
			deInt32 inIndex;
			deInt32 outIndex;
		};

		IndicesBuffer indices = { 0, 0 };

		const VkBufferCreateInfo	indicesBufferParams			=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			sizeof(IndicesBuffer),						// VkDeviceSize			size;
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32				queueFamilyIndexCount;
			DE_NULL,									// const deUint32*		pQueueFamilyIndices;
		};

		m_indicesBuffer				= createBuffer(vk, *m_device, &indicesBufferParams);
		m_indicesBufferAlloc		= memAlloc.allocate(getBufferMemoryRequirements(vk, *m_device, *m_indicesBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(*m_device, *m_indicesBuffer, m_indicesBufferAlloc->getMemory(), m_indicesBufferAlloc->getOffset()));

		if (m_accessOutOfBackingMemory)
		{
			if (m_bufferAccessType == BUFFER_ACCESS_TYPE_WRITE)
			{
				indices.outIndex = RobustBufferAccessTest::s_testArraySize - 1;
			}
			else
			{
				indices.inIndex = RobustBufferAccessTest::s_testArraySize - 1;
			}
		}

		deMemcpy(m_indicesBufferAlloc->getHostPtr(), &indices, sizeof(IndicesBuffer));

		flushMappedMemoryRange(vk, *m_device, m_indicesBufferAlloc->getMemory(), m_indicesBufferAlloc->getOffset(), VK_WHOLE_SIZE);

		log << tcu::TestLog::Message << "inIndex = " << indices.inIndex << tcu::TestLog::EndMessage;
		log << tcu::TestLog::Message << "outIndex = " << indices.outIndex << tcu::TestLog::EndMessage;
	}

	// Create descriptor data
	{
		VkDescriptorType	inBufferDescriptorType;
		VkDescriptorType	outBufferDescriptorType;

		if (isTexelAccess)
		{
			inBufferDescriptorType	= readFromStorage ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			outBufferDescriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		}
		else
		{
			inBufferDescriptorType	= readFromStorage ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			outBufferDescriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		DescriptorPoolBuilder descriptorPoolBuilder;
		descriptorPoolBuilder.addType(inBufferDescriptorType, 1u);
		descriptorPoolBuilder.addType(outBufferDescriptorType, 1u);
		descriptorPoolBuilder.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u);
		m_descriptorPool = descriptorPoolBuilder.build(vk, *m_device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

		DescriptorSetLayoutBuilder setLayoutBuilder;
		setLayoutBuilder.addSingleBinding(inBufferDescriptorType, VK_SHADER_STAGE_ALL);
		setLayoutBuilder.addSingleBinding(outBufferDescriptorType, VK_SHADER_STAGE_ALL);
		setLayoutBuilder.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
		m_descriptorSetLayout = setLayoutBuilder.build(vk, *m_device);

		const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		// VkStructureType				sType;
			DE_NULL,											// const void*					pNext;
			*m_descriptorPool,									// VkDescriptorPool				descriptorPool;
			1u,													// deUint32						setLayoutCount;
			&m_descriptorSetLayout.get()						// const VkDescriptorSetLayout*	pSetLayouts;
		};

		m_descriptorSet = allocateDescriptorSet(vk, *m_device, &descriptorSetAllocateInfo);

		DescriptorSetUpdateBuilder setUpdateBuilder;

		if (isTexelAccess)
		{
			const VkBufferViewCreateInfo inBufferViewCreateInfo =
			{
				VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,		// VkStructureType			sType;
				DE_NULL,										// const void*				pNext;
				0u,												// VkBufferViewCreateFlags	flags;
				*m_inBuffer,									// VkBuffer					buffer;
				m_bufferFormat,									// VkFormat					format;
				0ull,											// VkDeviceSize				offset;
				m_inBufferAccessRange							// VkDeviceSize				range;
			};
			m_inTexelBufferView	= createBufferView(vk, *m_device, &inBufferViewCreateInfo, DE_NULL);

			const VkBufferViewCreateInfo outBufferViewCreateInfo =
			{
				VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,		// VkStructureType			sType;
				DE_NULL,										// const void*				pNext;
				0u,												// VkBufferViewCreateFlags	flags;
				*m_outBuffer,									// VkBuffer					buffer;
				m_bufferFormat,									// VkFormat					format;
				0ull,											// VkDeviceSize				offset;
				m_outBufferAccessRange,							// VkDeviceSize				range;
			};
			m_outTexelBufferView	= createBufferView(vk, *m_device, &outBufferViewCreateInfo, DE_NULL);

			setUpdateBuilder.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0), inBufferDescriptorType, &m_inTexelBufferView.get());
			setUpdateBuilder.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1), outBufferDescriptorType, &m_outTexelBufferView.get());
		}
		else
		{
			const VkDescriptorBufferInfo inBufferDescriptorInfo		= makeDescriptorBufferInfo(*m_inBuffer, 0ull, m_inBufferAccessRange);
			const VkDescriptorBufferInfo outBufferDescriptorInfo	= makeDescriptorBufferInfo(*m_outBuffer, 0ull, m_outBufferAccessRange);

			setUpdateBuilder.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0), inBufferDescriptorType, &inBufferDescriptorInfo);
			setUpdateBuilder.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1), outBufferDescriptorType, &outBufferDescriptorInfo);
		}

		const VkDescriptorBufferInfo indicesBufferDescriptorInfo	= makeDescriptorBufferInfo(*m_indicesBuffer, 0ull, 8ull);
		setUpdateBuilder.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(2), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &indicesBufferDescriptorInfo);

		setUpdateBuilder.update(vk, *m_device);
	}

	// Create fence
	{
		const VkFenceCreateInfo fenceParams =
		{
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,	// VkStructureType		sType;
			DE_NULL,								// const void*			pNext;
			0u										// VkFenceCreateFlags	flags;
		};

		m_fence = createFence(vk, *m_device, &fenceParams);
	}

	// Get queue
	vk.getDeviceQueue(*m_device, queueFamilyIndex, 0, &m_queue);

	if (m_shaderStage == VK_SHADER_STAGE_COMPUTE_BIT)
	{
		m_testEnvironment = de::MovePtr<TestEnvironment>(new ComputeEnvironment(m_context, *m_device, *m_descriptorSetLayout, *m_descriptorSet));
	}
	else
	{
		using tcu::Vec4;

		const VkVertexInputBindingDescription vertexInputBindingDescription =
		{
			0u,								// deUint32					binding;
			sizeof(tcu::Vec4),				// deUint32					strideInBytes;
			VK_VERTEX_INPUT_RATE_VERTEX		// VkVertexInputStepRate	inputRate;
		};

		const VkVertexInputAttributeDescription vertexInputAttributeDescription =
		{
			0u,								// deUint32	location;
			0u,								// deUint32	binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,	// VkFormat	format;
			0u								// deUint32	offset;
		};

		const Vec4 vertices[] =
		{
			Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			Vec4(-1.0f, 1.0f, 0.0f, 1.0f),
			Vec4(1.0f, -1.0f, 0.0f, 1.0f),
		};

		// Create vertex buffer
		{
			const VkDeviceSize			vertexBufferSize	= (VkDeviceSize)(4u * sizeof(tcu::Vec4));
			const VkBufferCreateInfo	vertexBufferParams	=
			{
				VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
				DE_NULL,									// const void*			pNext;
				0u,											// VkBufferCreateFlags	flags;
				vertexBufferSize,							// VkDeviceSize			size;
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
				VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
				VK_QUEUE_FAMILY_IGNORED,					// deUint32				queueFamilyIndexCount;
				DE_NULL										// const deUint32*		pQueueFamilyIndices;
			};

			DE_ASSERT(vertexBufferSize > 0);

			m_vertexBuffer		= createBuffer(vk, *m_device, &vertexBufferParams);
			m_vertexBufferAlloc	= memAlloc.allocate(getBufferMemoryRequirements(vk, *m_device, *m_vertexBuffer), MemoryRequirement::HostVisible);

			VK_CHECK(vk.bindBufferMemory(*m_device, *m_vertexBuffer, m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset()));

			// Load vertices into vertex buffer
			deMemcpy(m_vertexBufferAlloc->getHostPtr(), vertices, sizeof(tcu::Vec4) * DE_LENGTH_OF_ARRAY(vertices));
			flushMappedMemoryRange(vk, *m_device, m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset(), VK_WHOLE_SIZE);
		}

		const GraphicsEnvironment::DrawConfig drawWithOneVertexBuffer =
		{
			std::vector<VkBuffer>(1, *m_vertexBuffer),	// std::vector<VkBuffer>	vertexBuffers;
			DE_LENGTH_OF_ARRAY(vertices),				// deUint32					vertexCount;
			1,											// deUint32					instanceCount;
			DE_NULL,									// VkBuffer					indexBuffer;
			0u,											// deUint32					indexCount;
		};

		m_testEnvironment = de::MovePtr<TestEnvironment>(new GraphicsEnvironment(m_context,
																				 *m_device,
																				 *m_descriptorSetLayout,
																				 *m_descriptorSet,
																				 GraphicsEnvironment::VertexBindings(1, vertexInputBindingDescription),
																				 GraphicsEnvironment::VertexAttributes(1, vertexInputAttributeDescription),
																				 drawWithOneVertexBuffer));
	}
}

// Verifies if the buffer has the value initialized by BufferAccessInstance::populateReadBuffer at a given offset.
bool BufferAccessInstance::isExpectedValueFromInBuffer (VkDeviceSize offsetInBytes, const void* valuePtr, VkDeviceSize valueSize)
{
	DE_ASSERT(offsetInBytes % 4 == 0);
	DE_ASSERT(offsetInBytes < m_inBufferAllocSize);

	const deUint32 valueIndex = deUint32(offsetInBytes / 4) + 2;

	if (isUintFormat(m_bufferFormat))
	{
		return !deMemCmp(valuePtr, &valueIndex, (size_t)valueSize);
	}
	else if (isIntFormat(m_bufferFormat))
	{
		const deInt32 value = -deInt32(valueIndex);
		return !deMemCmp(valuePtr, &value, (size_t)valueSize);
	}
	else if (isFloatFormat(m_bufferFormat))
	{
		const float value = float(valueIndex);
		return !deMemCmp(valuePtr, &value, (size_t)valueSize);
	}
	else if (m_bufferFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
	{
		const deUint32	r		= ((valueIndex + 0) & ((2u << 10) - 1u));
		const deUint32	g		= ((valueIndex + 1) & ((2u << 10) - 1u));
		const deUint32	b		= ((valueIndex + 2) & ((2u << 10) - 1u));
		const deUint32	a		= ((valueIndex + 0) & ((2u << 2) - 1u));
		const deUint32	abgr	= (a << 30) | (b << 20) | (g << 10) | r;

		return !deMemCmp(valuePtr, &abgr, (size_t)valueSize);
	}
	else
	{
		DE_ASSERT(false);
		return false;
	}
}

bool BufferAccessInstance::isOutBufferValueUnchanged (VkDeviceSize offsetInBytes, VkDeviceSize valueSize)
{
	const deUint8 *const	outValuePtr		= (deUint8*)m_outBufferAlloc->getHostPtr() + offsetInBytes;
	const deUint32			defaultValue	= 0xFFFFFFFFu;

	return !deMemCmp(outValuePtr, &defaultValue, (size_t)valueSize);
}

tcu::TestStatus BufferAccessInstance::iterate (void)
{
	const DeviceInterface&		vk			= m_context.getDeviceInterface();
	const vk::VkCommandBuffer	cmdBuffer	= m_testEnvironment->getCommandBuffer();

	// Submit command buffer
	{
		const VkSubmitInfo	submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType				sType;
			DE_NULL,						// const void*					pNext;
			0u,								// deUint32						waitSemaphoreCount;
			DE_NULL,						// const VkSemaphore*			pWaitSemaphores;
			DE_NULL,						// const VkPIpelineStageFlags*	pWaitDstStageMask;
			1u,								// deUint32						commandBufferCount;
			&cmdBuffer,						// const VkCommandBuffer*		pCommandBuffers;
			0u,								// deUint32						signalSemaphoreCount;
			DE_NULL							// const VkSemaphore*			pSignalSemaphores;
		};

		VK_CHECK(vk.resetFences(*m_device, 1, &m_fence.get()));
		VK_CHECK(vk.queueSubmit(m_queue, 1, &submitInfo, *m_fence));
		VK_CHECK(vk.waitForFences(*m_device, 1, &m_fence.get(), true, ~(0ull) /* infinity */));
	}

	// Prepare result buffer for read
	{
		const VkMappedMemoryRange	outBufferRange	=
		{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,	//  VkStructureType	sType;
			DE_NULL,								//  const void*		pNext;
			m_outBufferAlloc->getMemory(),			//  VkDeviceMemory	mem;
			0ull,									//  VkDeviceSize	offset;
			m_outBufferAllocSize,					//  VkDeviceSize	size;
		};

		VK_CHECK(vk.invalidateMappedMemoryRanges(*m_device, 1u, &outBufferRange));
	}

	if (verifyResult())
		return tcu::TestStatus::pass("All values OK");
	else
		return tcu::TestStatus::fail("Invalid value(s) found");
}

bool BufferAccessInstance::verifyResult (void)
{
	std::ostringstream	logMsg;
	tcu::TestLog&		log					= m_context.getTestContext().getLog();
	const bool			isReadAccess		= !!(m_bufferAccessType == BUFFER_ACCESS_TYPE_READ || m_bufferAccessType == BUFFER_ACCESS_TYPE_READ_FROM_STORAGE);
	const void*			inDataPtr			= m_inBufferAlloc->getHostPtr();
	const void*			outDataPtr			= m_outBufferAlloc->getHostPtr();
	bool				allOk				= true;
	deUint32			valueNdx			= 0;
	const VkDeviceSize	maxAccessRange		= isReadAccess ? m_inBufferMaxAccessRange : m_outBufferMaxAccessRange;

	for (VkDeviceSize offsetInBytes = 0; offsetInBytes < m_outBufferAllocSize; offsetInBytes += 4)
	{
		deUint8*			outValuePtr		= (deUint8*)outDataPtr + offsetInBytes;
		const size_t		outValueSize	= (size_t)min(4, (m_outBufferAllocSize - offsetInBytes));

		if (offsetInBytes >= RobustBufferAccessTest::s_numberOfBytesAccessed)
		{
			// The shader will only write 16 values into the result buffer. The rest of the values
			// should remain unchanged or may be modified if we are writing out of bounds.
			if (!isOutBufferValueUnchanged(offsetInBytes, outValueSize)
				&& (isReadAccess || !isValueWithinBufferOrZero(inDataPtr, m_inBufferAllocSize, outValuePtr, 4)))
			{
				logMsg << "\nValue " << valueNdx++ << " has been modified with an unknown value: " << *((deUint32 *)outValuePtr);
				allOk = false;
			}
		}
		else
		{
			const deInt32	distanceToOutOfBounds	= (deInt32)maxAccessRange - (deInt32)offsetInBytes;
			bool			isOutOfBoundsAccess		= false;

			logMsg << "\n" << valueNdx++ << ": ";

			logValue(logMsg, outValuePtr, m_bufferFormat, outValueSize);

			if (m_accessOutOfBackingMemory)
				isOutOfBoundsAccess = true;

			// Check if the shader operation accessed an operand located less than 16 bytes away
			// from the out of bounds address.
			if (!isOutOfBoundsAccess && distanceToOutOfBounds < 16)
			{
				deUint32 operandSize = 0;

				switch (m_shaderType)
				{
					case SHADER_TYPE_SCALAR_COPY:
						operandSize		= 4; // Size of scalar
						break;

					case SHADER_TYPE_VECTOR_COPY:
						operandSize		= 4 * 4; // Size of vec4
						break;

					case SHADER_TYPE_MATRIX_COPY:
						operandSize		= 4 * 16; // Size of mat4
						break;

					case SHADER_TYPE_TEXEL_COPY:
						operandSize		= mapVkFormat(m_bufferFormat).getPixelSize();
						break;

					default:
						DE_ASSERT(false);
				}

				isOutOfBoundsAccess = (((offsetInBytes / operandSize) + 1) * operandSize > maxAccessRange);
			}

			if (isOutOfBoundsAccess)
			{
				logMsg << " (out of bounds " << (isReadAccess ? "read": "write") << ")";

				const bool	isValuePartiallyOutOfBounds = ((distanceToOutOfBounds > 0) && ((deUint32)distanceToOutOfBounds < 4));
				bool		isValidValue				= false;

				if (isValuePartiallyOutOfBounds && !m_accessOutOfBackingMemory)
				{
					// The value is partially out of bounds

					bool	isOutOfBoundsPartOk = true;
					bool	isWithinBoundsPartOk = true;

					if (isReadAccess)
					{
						isWithinBoundsPartOk	= isValueWithinBufferOrZero(inDataPtr, m_inBufferAllocSize, outValuePtr, distanceToOutOfBounds);
						isOutOfBoundsPartOk		= isValueWithinBufferOrZero(inDataPtr, m_inBufferAllocSize, (deUint8*)outValuePtr + distanceToOutOfBounds , outValueSize - distanceToOutOfBounds);
					}
					else
					{
						isWithinBoundsPartOk	= isValueWithinBufferOrZero(inDataPtr, m_inBufferAllocSize, outValuePtr, distanceToOutOfBounds)
												  || isOutBufferValueUnchanged(offsetInBytes, distanceToOutOfBounds);

						isOutOfBoundsPartOk		= isValueWithinBufferOrZero(inDataPtr, m_inBufferAllocSize, (deUint8*)outValuePtr + distanceToOutOfBounds, outValueSize - distanceToOutOfBounds)
												  || isOutBufferValueUnchanged(offsetInBytes + distanceToOutOfBounds, outValueSize - distanceToOutOfBounds);
					}

					logMsg << ", first " << distanceToOutOfBounds << " byte(s) " << (isWithinBoundsPartOk ? "OK": "wrong");
					logMsg << ", last " << outValueSize - distanceToOutOfBounds << " byte(s) " << (isOutOfBoundsPartOk ? "OK": "wrong");

					isValidValue	= isWithinBoundsPartOk && isOutOfBoundsPartOk;
				}
				else
				{
					if (isReadAccess)
					{
						isValidValue	= isValueWithinBufferOrZero(inDataPtr, m_inBufferAllocSize, outValuePtr, outValueSize);
					}
					else
					{
						isValidValue	= isOutBufferValueUnchanged(offsetInBytes, outValueSize);

						if (!isValidValue)
						{
							// Out of bounds writes may modify values withing the memory ranges bound to the buffer
							isValidValue	= isValueWithinBufferOrZero(inDataPtr, m_inBufferAllocSize, outValuePtr, outValueSize);

							if (isValidValue)
								logMsg << ", OK, written within the memory range bound to the buffer";
						}
					}
				}

				if (!isValidValue)
				{
					// Check if we are satisfying the [0, 0, 0, x] pattern, where x may be either 0 or 1,
					// or the maximum representable positive integer value (if the format is integer-based).

					const bool	canMatchVec4Pattern	= (isReadAccess
													&& !isValuePartiallyOutOfBounds
													&& (m_shaderType == SHADER_TYPE_VECTOR_COPY || m_shaderType == SHADER_TYPE_TEXEL_COPY)
													&& ((offsetInBytes / 4 + 1) % 4 == 0 || m_bufferFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32));
					bool		matchesVec4Pattern	= false;

					if (canMatchVec4Pattern)
					{
						if (m_bufferFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
							matchesVec4Pattern	= verifyOutOfBoundsVec4(outValuePtr, m_bufferFormat);
						else
							matchesVec4Pattern	= verifyOutOfBoundsVec4(reinterpret_cast<deUint32*>(outValuePtr) - 3, m_bufferFormat);
					}

					if (!canMatchVec4Pattern || !matchesVec4Pattern)
					{
						logMsg << ". Failed: ";

						if (isReadAccess)
						{
							logMsg << "expected value within the buffer range or 0";

							if (canMatchVec4Pattern)
								logMsg << ", or the [0, 0, 0, x] pattern";
						}
						else
						{
							logMsg << "written out of the range";
						}

						allOk = false;
					}
				}
			}
			else // We are within bounds
			{
				if (isReadAccess)
				{
					if (!isExpectedValueFromInBuffer(offsetInBytes, outValuePtr, 4))
					{
						logMsg << ", Failed: unexpected value";
						allOk = false;
					}
				}
				else
				{
					// Out of bounds writes may change values within the bounds.
					if (!isValueWithinBufferOrZero(inDataPtr, m_inBufferAccessRange, outValuePtr, 4))
					{
						logMsg << ", Failed: unexpected value";
						allOk = false;
					}
				}
			}
		}
	}

	log << tcu::TestLog::Message << logMsg.str() << tcu::TestLog::EndMessage;

	return allOk;
}

// BufferReadInstance

BufferReadInstance::BufferReadInstance (Context&			context,
										Move<VkDevice>		device,
										ShaderType			shaderType,
										VkShaderStageFlags	shaderStage,
										VkFormat			bufferFormat,
										bool				readFromStorage,
										VkDeviceSize		inBufferAccessRange,
										bool				accessOutOfBackingMemory)

	: BufferAccessInstance	(context, device, shaderType, shaderStage, bufferFormat,
							 readFromStorage ? BUFFER_ACCESS_TYPE_READ_FROM_STORAGE : BUFFER_ACCESS_TYPE_READ,
							 inBufferAccessRange,
							 RobustBufferAccessTest::s_numberOfBytesAccessed,	// outBufferAccessRange
							 accessOutOfBackingMemory)
{
}

// BufferWriteInstance

BufferWriteInstance::BufferWriteInstance (Context&				context,
										  Move<VkDevice>		device,
										  ShaderType			shaderType,
										  VkShaderStageFlags	shaderStage,
										  VkFormat				bufferFormat,
										  VkDeviceSize			writeBufferAccessRange,
										  bool					accessOutOfBackingMemory)

	: BufferAccessInstance	(context, device, shaderType, shaderStage, bufferFormat,
							 BUFFER_ACCESS_TYPE_WRITE,
							 RobustBufferAccessTest::s_numberOfBytesAccessed,	// inBufferAccessRange
							 writeBufferAccessRange,
							 accessOutOfBackingMemory)
{
}

// Test node creation functions

static const char* getShaderStageName (VkShaderStageFlagBits shaderStage)
{
	switch (shaderStage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:					return "vertex";
		case VK_SHADER_STAGE_FRAGMENT_BIT:					return "fragment";
		case VK_SHADER_STAGE_COMPUTE_BIT:					return "compute";
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:		return "tess_control";
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:	return "tess_eval";
		case VK_SHADER_STAGE_GEOMETRY_BIT:					return "geometry";

		default:
			DE_ASSERT(false);
	}

	return DE_NULL;
}

static void addBufferAccessTests (tcu::TestContext& testCtx, tcu::TestCaseGroup* parentNode)
{
	struct BufferRangeConfig
	{
		const char*		name;
		VkDeviceSize	range;
	};

	const VkShaderStageFlagBits bufferAccessStages[] =
	{
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_COMPUTE_BIT,
	};

	const VkFormat bufferFormats[] =
	{
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SFLOAT
	};

	const VkFormat texelBufferFormats[] =
	{
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,

		VK_FORMAT_A2B10G10R10_UNORM_PACK32
	};

	const BufferRangeConfig bufferRangeConfigs[] =
	{
		{ "range_1_byte",		1ull },
		{ "range_3_bytes",		3ull },
		{ "range_4_bytes",		4ull },		// size of float
		{ "range_32_bytes",		32ull },	// size of half mat4
	};

	const BufferRangeConfig texelBufferRangeConfigs[] =
	{
		{ "range_1_texel",		1u },
		{ "range_3_texels",		3u },
	};

	const char* shaderTypeNames[SHADER_TYPE_COUNT] =
	{
		"mat4_copy",
		"vec4_copy",
		"scalar_copy",
		"texel_copy",
	};

	for (int stageNdx = 0; stageNdx < DE_LENGTH_OF_ARRAY(bufferAccessStages); stageNdx++)
	{
		const VkShaderStageFlagBits			stage			= bufferAccessStages[stageNdx];
		de::MovePtr<tcu::TestCaseGroup>		stageTests		(new tcu::TestCaseGroup(testCtx, getShaderStageName(stage), ""));

		for (int shaderTypeNdx = 0; shaderTypeNdx < SHADER_TYPE_COUNT; shaderTypeNdx++)
		{
			const VkFormat*					formats;
			size_t							formatsLength;
			const BufferRangeConfig*		ranges;
			size_t							rangesLength;
			deUint32						rangeMultiplier;
			de::MovePtr<tcu::TestCaseGroup> shaderTypeTests	(new tcu::TestCaseGroup(testCtx, shaderTypeNames[shaderTypeNdx], ""));

			if ((ShaderType)shaderTypeNdx == SHADER_TYPE_TEXEL_COPY)
			{
				formats			= texelBufferFormats;
				formatsLength	= DE_LENGTH_OF_ARRAY(texelBufferFormats);

				ranges			= texelBufferRangeConfigs;
				rangesLength	= DE_LENGTH_OF_ARRAY(texelBufferRangeConfigs);
			}
			else
			{
				formats			= bufferFormats;
				formatsLength	= DE_LENGTH_OF_ARRAY(bufferFormats);

				ranges			= bufferRangeConfigs;
				rangesLength	= DE_LENGTH_OF_ARRAY(bufferRangeConfigs);
			}

			for (size_t formatNdx = 0; formatNdx < formatsLength; formatNdx++)
			{
				const VkFormat	bufferFormat	= formats[formatNdx];

				rangeMultiplier = ((ShaderType)shaderTypeNdx == SHADER_TYPE_TEXEL_COPY) ? mapVkFormat(bufferFormat).getPixelSize() : 1;

				if (!isFloatFormat(bufferFormat) && ((ShaderType)shaderTypeNdx) == SHADER_TYPE_MATRIX_COPY)
				{
					// Use SHADER_TYPE_MATRIX_COPY with floating-point formats only
					break;
				}

				const std::string				formatName		= getFormatName(bufferFormat);
				de::MovePtr<tcu::TestCaseGroup>	formatTests		(new tcu::TestCaseGroup(testCtx, de::toLower(formatName.substr(10)).c_str(), ""));

				de::MovePtr<tcu::TestCaseGroup> uboReadTests		(new tcu::TestCaseGroup(testCtx, "oob_uniform_read", ""));
				de::MovePtr<tcu::TestCaseGroup> ssboReadTests		(new tcu::TestCaseGroup(testCtx, "oob_storage_read", ""));
				de::MovePtr<tcu::TestCaseGroup> ssboWriteTests		(new tcu::TestCaseGroup(testCtx, "oob_storage_write", ""));

				for (size_t rangeNdx = 0; rangeNdx < rangesLength; rangeNdx++)
				{
					const BufferRangeConfig&	rangeConfig			= ranges[rangeNdx];
					const VkDeviceSize			rangeInBytes		= rangeConfig.range * rangeMultiplier;

					uboReadTests->addChild(new RobustBufferReadTest(testCtx, rangeConfig.name, "", stage, (ShaderType)shaderTypeNdx, bufferFormat, rangeInBytes, false, false));
					ssboReadTests->addChild(new RobustBufferReadTest(testCtx, rangeConfig.name, "", stage, (ShaderType)shaderTypeNdx, bufferFormat, rangeInBytes, true, false));
					ssboWriteTests->addChild(new RobustBufferWriteTest(testCtx, rangeConfig.name, "", stage, (ShaderType)shaderTypeNdx, bufferFormat, rangeInBytes, false));

				}

				formatTests->addChild(uboReadTests.release());
				formatTests->addChild(ssboReadTests.release());
				formatTests->addChild(ssboWriteTests.release());

				shaderTypeTests->addChild(formatTests.release());
			}

			// Read/write out of the memory that backs the buffer
			{
				de::MovePtr<tcu::TestCaseGroup>	outOfAllocTests		(new tcu::TestCaseGroup(testCtx, "out_of_alloc", ""));

				const VkFormat format = (((ShaderType)shaderTypeNdx == SHADER_TYPE_TEXEL_COPY ) ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R32_SFLOAT);

				outOfAllocTests->addChild(new RobustBufferReadTest(testCtx, "oob_uniform_read", "", stage, (ShaderType)shaderTypeNdx, format, 16, false, true));
				outOfAllocTests->addChild(new RobustBufferReadTest(testCtx, "oob_storage_read", "", stage, (ShaderType)shaderTypeNdx, format, 16, true, true));
				outOfAllocTests->addChild(new RobustBufferWriteTest(testCtx, "oob_storage_write", "", stage, (ShaderType)shaderTypeNdx, format, 16, true));

				shaderTypeTests->addChild(outOfAllocTests.release());
			}

			stageTests->addChild(shaderTypeTests.release());
		}
		parentNode->addChild(stageTests.release());
	}
}

tcu::TestCaseGroup* createBufferAccessTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> bufferAccessTests	(new tcu::TestCaseGroup(testCtx, "buffer_access", ""));

	addBufferAccessTests(testCtx, bufferAccessTests.get());

	return bufferAccessTests.release();
}

} // robustness
} // vkt
