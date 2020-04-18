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
 * \brief Robust Vertex Buffer Access Tests
 *//*--------------------------------------------------------------------*/

#include "vktRobustnessVertexAccessTests.hpp"
#include "vktRobustnessUtil.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "tcuTestLog.hpp"
#include "deMath.h"
#include "deUniquePtr.hpp"
#include <vector>

namespace vkt
{
namespace robustness
{

using namespace vk;

typedef std::vector<VkVertexInputBindingDescription>	BindingList;
typedef std::vector<VkVertexInputAttributeDescription>	AttributeList;

class VertexAccessTest : public vkt::TestCase
{
public:
						VertexAccessTest	(tcu::TestContext&		testContext,
											 const std::string&		name,
											 const std::string&		description,
											 VkFormat				inputFormat,
											 deUint32				numVertexValues,
											 deUint32				numInstanceValues,
											 deUint32				numVertices,
											 deUint32				numInstances);

	virtual				~VertexAccessTest	(void) {}

	void				initPrograms		(SourceCollections& programCollection) const;
	TestInstance*		createInstance		(Context& context) const = 0;

protected:
	const VkFormat		m_inputFormat;
	const deUint32		m_numVertexValues;
	const deUint32		m_numInstanceValues;
	const deUint32		m_numVertices;
	const deUint32		m_numInstances;

};

class DrawAccessTest : public VertexAccessTest
{
public:
						DrawAccessTest		(tcu::TestContext&		testContext,
											 const std::string&		name,
											 const std::string&		description,
											 VkFormat				inputFormat,
											 deUint32				numVertexValues,
											 deUint32				numInstanceValues,
											 deUint32				numVertices,
											 deUint32				numInstances);

	virtual				~DrawAccessTest		(void) {}
	TestInstance*		createInstance		(Context& context) const;

protected:
};

class DrawIndexedAccessTest : public VertexAccessTest
{
public:
	enum IndexConfig
	{
		INDEX_CONFIG_LAST_INDEX_OUT_OF_BOUNDS,
		INDEX_CONFIG_INDICES_OUT_OF_BOUNDS,
		INDEX_CONFIG_TRIANGLE_OUT_OF_BOUNDS,

		INDEX_CONFIG_COUNT
	};

	const static std::vector<deUint32> s_indexConfigs[INDEX_CONFIG_COUNT];

						DrawIndexedAccessTest		(tcu::TestContext&		testContext,
													 const std::string&		name,
													 const std::string&		description,
													 VkFormat				inputFormat,
													 IndexConfig			indexConfig);

	virtual				~DrawIndexedAccessTest		(void) {}
	TestInstance*		createInstance				(Context& context) const;

protected:
	const IndexConfig	m_indexConfig;
};

class VertexAccessInstance : public vkt::TestInstance
{
public:
										VertexAccessInstance					(Context&						context,
																				 Move<VkDevice>					device,
																				 VkFormat						inputFormat,
																				 deUint32						numVertexValues,
																				 deUint32						numInstanceValues,
																				 deUint32						numVertices,
																				 deUint32						numInstances,
																				 const std::vector<deUint32>&	indices);

	virtual								~VertexAccessInstance					(void) {}
	virtual tcu::TestStatus				iterate									(void);
	virtual bool						verifyResult							(void);

private:
	bool								isValueWithinVertexBufferOrZero			(void* vertexBuffer, VkDeviceSize vertexBufferSize, const void* value, deUint32 valueIndexa);

protected:
	static bool							isExpectedValueFromVertexBuffer			(const void* vertexBuffer, deUint32 vertexIndex, VkFormat vertexFormat, const void* value);
	static VkDeviceSize					getBufferSizeInBytes					(deUint32 numScalars, VkFormat format);

	virtual void						initVertexIds							(deUint32 *indicesPtr, size_t indexCount) = 0;
	virtual deUint32					getIndex								(deUint32 vertexNum) const = 0;

	Move<VkDevice>						m_device;

	const VkFormat						m_inputFormat;
	const deUint32						m_numVertexValues;
	const deUint32						m_numInstanceValues;
	const deUint32						m_numVertices;
	const deUint32						m_numInstances;
	AttributeList						m_vertexInputAttributes;
	BindingList							m_vertexInputBindings;

	Move<VkBuffer>						m_vertexRateBuffer;
	VkDeviceSize						m_vertexRateBufferSize;
	de::MovePtr<Allocation>				m_vertexRateBufferAlloc;
	VkDeviceSize						m_vertexRateBufferAllocSize;

	Move<VkBuffer>						m_instanceRateBuffer;
	VkDeviceSize						m_instanceRateBufferSize;
	de::MovePtr<Allocation>				m_instanceRateBufferAlloc;
	VkDeviceSize						m_instanceRateBufferAllocSize;

	Move<VkBuffer>						m_vertexNumBuffer;
	VkDeviceSize						m_vertexNumBufferSize;
	de::MovePtr<Allocation>				m_vertexNumBufferAlloc;

	Move<VkBuffer>						m_indexBuffer;
	VkDeviceSize						m_indexBufferSize;
	de::MovePtr<Allocation>				m_indexBufferAlloc;

	Move<VkBuffer>						m_outBuffer; // SSBO
	VkDeviceSize						m_outBufferSize;
	de::MovePtr<Allocation>				m_outBufferAlloc;

	Move<VkDescriptorPool>				m_descriptorPool;
	Move<VkDescriptorSetLayout>			m_descriptorSetLayout;
	Move<VkDescriptorSet>				m_descriptorSet;

	Move<VkFence>						m_fence;
	VkQueue								m_queue;

	de::MovePtr<GraphicsEnvironment>	m_graphicsTestEnvironment;
};

class DrawAccessInstance : public VertexAccessInstance
{
public:
						DrawAccessInstance	(Context&				context,
											 Move<VkDevice>			device,
											 VkFormat				inputFormat,
											 deUint32				numVertexValues,
											 deUint32				numInstanceValues,
											 deUint32				numVertices,
											 deUint32				numInstances);

	virtual				~DrawAccessInstance	(void) {}

protected:
	virtual void		initVertexIds		(deUint32 *indicesPtr, size_t indexCount);
	virtual deUint32	getIndex			(deUint32 vertexNum) const;
};

class DrawIndexedAccessInstance : public VertexAccessInstance
{
public:
										DrawIndexedAccessInstance	(Context&						context,
																	 Move<VkDevice>					device,
																	 VkFormat						inputFormat,
																	 deUint32						numVertexValues,
																	 deUint32						numInstanceValues,
																	 deUint32						numVertices,
																	 deUint32						numInstances,
																	 const std::vector<deUint32>&	indices);

	virtual								~DrawIndexedAccessInstance	(void) {}

protected:
	virtual void						initVertexIds				(deUint32 *indicesPtr, size_t indexCount);
	virtual deUint32					getIndex					(deUint32 vertexNum) const;

	const std::vector<deUint32>			m_indices;
};

// VertexAccessTest

VertexAccessTest::VertexAccessTest (tcu::TestContext&		testContext,
									const std::string&		name,
									const std::string&		description,
									VkFormat				inputFormat,
									deUint32				numVertexValues,
									deUint32				numInstanceValues,
									deUint32				numVertices,
									deUint32				numInstances)

	: vkt::TestCase				(testContext, name, description)
	, m_inputFormat				(inputFormat)
	, m_numVertexValues			(numVertexValues)
	, m_numInstanceValues		(numInstanceValues)
	, m_numVertices				(numVertices)
	, m_numInstances			(numInstances)
{
}

void VertexAccessTest::initPrograms (SourceCollections& programCollection) const
{
	std::ostringstream		attributeDeclaration;
	std::ostringstream		attributeUse;

	std::ostringstream		vertexShaderSource;
	std::ostringstream		fragmentShaderSource;

	std::ostringstream		attributeTypeStr;
	const int				numChannels				= getNumUsedChannels(mapVkFormat(m_inputFormat).order);
	const deUint32			numScalarsPerVertex		= numChannels * 3; // Use 3 identical attributes
	deUint32				numValues				= 0;

	if (numChannels == 1)
	{
		if (isUintFormat(m_inputFormat))
			attributeTypeStr << "uint";
		else if (isIntFormat(m_inputFormat))
			attributeTypeStr << "int";
		else
			attributeTypeStr << "float";
	}
	else
	{
		if (isUintFormat(m_inputFormat))
			attributeTypeStr << "uvec";
		else if (isIntFormat(m_inputFormat))
			attributeTypeStr << "ivec";
		else
			attributeTypeStr << "vec";

		attributeTypeStr << numChannels;
	}

	for (int attrNdx = 0; attrNdx < 3; attrNdx++)
	{
		attributeDeclaration << "layout(location = " << attrNdx << ") in " << attributeTypeStr.str() << " attr" << attrNdx << ";\n";

		for (int chanNdx = 0; chanNdx < numChannels; chanNdx++)
		{
			attributeUse << "\toutData[(gl_InstanceIndex * " << numScalarsPerVertex * m_numVertices
						 << ") + (vertexNum * " << numScalarsPerVertex << " + " << numValues++ << ")] = attr" << attrNdx;

			if (numChannels == 1)
				attributeUse << ";\n";
			else
				attributeUse << "[" << chanNdx << "];\n";
		}
	}

	attributeDeclaration << "layout(location = 3) in int vertexNum;\n";

	attributeUse << "\n";

	const char *outType = "";
	if (isUintFormat(m_inputFormat))
		outType = "uint";
	else if (isIntFormat(m_inputFormat))
		outType = "int";
	else
		outType = "float";

	vertexShaderSource <<
		"#version 310 es\n"
		"precision highp float;\n"
		<< attributeDeclaration.str() <<
		"layout(set = 0, binding = 0, std430) buffer outBuffer\n"
		"{\n"
		"\t" << outType << " outData[" << (m_numVertices * numValues) * m_numInstances << "];\n"
		"};\n\n"
		"void main (void)\n"
		"{\n"
		<< attributeUse.str() <<
		"\tgl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
		"}\n";

	programCollection.glslSources.add("vertex") << glu::VertexSource(vertexShaderSource.str());

	fragmentShaderSource <<
		"#version 310 es\n"
		"precision highp float;\n"
		"layout(location = 0) out vec4 fragColor;\n"
		"void main (void)\n"
		"{\n"
		"\tfragColor = vec4(1.0);\n"
		"}\n";

	programCollection.glslSources.add("fragment") << glu::FragmentSource(fragmentShaderSource.str());
}

// DrawAccessTest

DrawAccessTest::DrawAccessTest (tcu::TestContext&		testContext,
								const std::string&		name,
								const std::string&		description,
								VkFormat				inputFormat,
								deUint32				numVertexValues,
								deUint32				numInstanceValues,
								deUint32				numVertices,
								deUint32				numInstances)

	: VertexAccessTest		(testContext, name, description, inputFormat, numVertexValues, numInstanceValues, numVertices, numInstances)
{
}

TestInstance* DrawAccessTest::createInstance (Context& context) const
{
	Move<VkDevice> device = createRobustBufferAccessDevice(context);

	return new DrawAccessInstance(context,
								  device,
								  m_inputFormat,
								  m_numVertexValues,
								  m_numInstanceValues,
								  m_numVertices,
								  m_numInstances);
}

// DrawIndexedAccessTest

const deUint32 lastIndexOutOfBounds[] =
{
	0, 1, 2, 3, 4, 100,		// Indices of 100 and above are out of bounds
};
const deUint32 indicesOutOfBounds[] =
{
	0, 100, 2, 101, 3, 102,	// Indices of 100 and above are out of bounds
};
const deUint32 triangleOutOfBounds[] =
{
	100, 101, 102, 3, 4, 5,	// Indices of 100 and above are out of bounds
};

const std::vector<deUint32> DrawIndexedAccessTest::s_indexConfigs[INDEX_CONFIG_COUNT] =
{
	std::vector<deUint32>(lastIndexOutOfBounds, lastIndexOutOfBounds + DE_LENGTH_OF_ARRAY(lastIndexOutOfBounds)),
	std::vector<deUint32>(indicesOutOfBounds, indicesOutOfBounds + DE_LENGTH_OF_ARRAY(indicesOutOfBounds)),
	std::vector<deUint32>(triangleOutOfBounds, triangleOutOfBounds + DE_LENGTH_OF_ARRAY(triangleOutOfBounds)),
};

DrawIndexedAccessTest::DrawIndexedAccessTest (tcu::TestContext&		testContext,
											  const std::string&	name,
											  const std::string&	description,
											  VkFormat				inputFormat,
											  IndexConfig			indexConfig)

	: VertexAccessTest	(testContext,
						 name,
						 description,
						 inputFormat,
						 getNumUsedChannels(mapVkFormat(inputFormat).order) * (deUint32)s_indexConfigs[indexConfig].size() * 2,	// numVertexValues
						 getNumUsedChannels(mapVkFormat(inputFormat).order),													// numInstanceValues
						 (deUint32)s_indexConfigs[indexConfig].size(),															// numVertices
						 1)																										// numInstances
	, m_indexConfig		(indexConfig)
{
}

TestInstance* DrawIndexedAccessTest::createInstance (Context& context) const
{
	Move<VkDevice> device = createRobustBufferAccessDevice(context);

	return new DrawIndexedAccessInstance(context,
										 device,
										 m_inputFormat,
										 m_numVertexValues,
										 m_numInstanceValues,
										 m_numVertices,
										 m_numInstances,
										 s_indexConfigs[m_indexConfig]);
}

// VertexAccessInstance

VertexAccessInstance::VertexAccessInstance (Context&						context,
											Move<VkDevice>					device,
											VkFormat						inputFormat,
											deUint32						numVertexValues,
											deUint32						numInstanceValues,
											deUint32						numVertices,
											deUint32						numInstances,
											const std::vector<deUint32>&	indices)

	: vkt::TestInstance			(context)
	, m_device					(device)
	, m_inputFormat				(inputFormat)
	, m_numVertexValues			(numVertexValues)
	, m_numInstanceValues		(numInstanceValues)
	, m_numVertices				(numVertices)
	, m_numInstances			(numInstances)
{
	const DeviceInterface&		vk						= context.getDeviceInterface();
	const deUint32				queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	SimpleAllocator				memAlloc				(vk, *m_device, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));
	const deUint32				formatSizeInBytes		= tcu::getPixelSize(mapVkFormat(m_inputFormat));

	// Check storage support
	if (!context.getDeviceFeatures().vertexPipelineStoresAndAtomics)
	{
		TCU_THROW(NotSupportedError, "Stores not supported in vertex stage");
	}

	const VkVertexInputAttributeDescription attributes[] =
	{
		// input rate: vertex
		{
			0u,								// deUint32 location;
			0u,								// deUint32 binding;
			m_inputFormat,					// VkFormat format;
			0u,								// deUint32 offset;
		},
		{
			1u,								// deUint32 location;
			0u,								// deUint32 binding;
			m_inputFormat,					// VkFormat format;
			formatSizeInBytes,				// deUint32 offset;
		},

		// input rate: instance
		{
			2u,								// deUint32 location;
			1u,								// deUint32 binding;
			m_inputFormat,					// VkFormat format;
			0u,								// deUint32 offset;
		},

		// Attribute for vertex number
		{
			3u,								// deUint32 location;
			2u,								// deUint32 binding;
			VK_FORMAT_R32_SINT,				// VkFormat format;
			0,								// deUint32 offset;
		},
	};

	const VkVertexInputBindingDescription bindings[] =
	{
		{
			0u,								// deUint32				binding;
			formatSizeInBytes * 2,			// deUint32				stride;
			VK_VERTEX_INPUT_RATE_VERTEX		// VkVertexInputRate	inputRate;
		},
		{
			1u,								// deUint32				binding;
			formatSizeInBytes,				// deUint32				stride;
			VK_VERTEX_INPUT_RATE_INSTANCE	// VkVertexInputRate	inputRate;
		},
		{
			2u,								// deUint32				binding;
			sizeof(deInt32),				// deUint32				stride;
			VK_VERTEX_INPUT_RATE_VERTEX		// VkVertexInputRate	inputRate;
		},
	};

	m_vertexInputBindings	= std::vector<VkVertexInputBindingDescription>(bindings, bindings + DE_LENGTH_OF_ARRAY(bindings));
	m_vertexInputAttributes	= std::vector<VkVertexInputAttributeDescription>(attributes, attributes + DE_LENGTH_OF_ARRAY(attributes));

	// Create vertex buffer for vertex input rate
	{
		VkMemoryRequirements bufferMemoryReqs;

		m_vertexRateBufferSize = getBufferSizeInBytes(m_numVertexValues, m_inputFormat); // All formats used in this test suite are 32-bit based.

		const VkBufferCreateInfo	vertexRateBufferParams	=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_vertexRateBufferSize,						// VkDeviceSize			size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_vertexRateBuffer			= createBuffer(vk, *m_device, &vertexRateBufferParams);
		bufferMemoryReqs			= getBufferMemoryRequirements(vk, *m_device, *m_vertexRateBuffer);
		m_vertexRateBufferAllocSize	= bufferMemoryReqs.size;
		m_vertexRateBufferAlloc		= memAlloc.allocate(bufferMemoryReqs, MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(*m_device, *m_vertexRateBuffer, m_vertexRateBufferAlloc->getMemory(), m_vertexRateBufferAlloc->getOffset()));
		populateBufferWithTestValues(m_vertexRateBufferAlloc->getHostPtr(), (deUint32)m_vertexRateBufferAllocSize, m_inputFormat);
		flushMappedMemoryRange(vk, *m_device, m_vertexRateBufferAlloc->getMemory(), m_vertexRateBufferAlloc->getOffset(), VK_WHOLE_SIZE);
	}

	// Create vertex buffer for instance input rate
	{
		VkMemoryRequirements bufferMemoryReqs;

		m_instanceRateBufferSize = getBufferSizeInBytes(m_numInstanceValues, m_inputFormat); // All formats used in this test suite are 32-bit based.

		const VkBufferCreateInfo	instanceRateBufferParams	=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_instanceRateBufferSize,					// VkDeviceSize			size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_instanceRateBuffer			= createBuffer(vk, *m_device, &instanceRateBufferParams);
		bufferMemoryReqs				= getBufferMemoryRequirements(vk, *m_device, *m_instanceRateBuffer);
		m_instanceRateBufferAllocSize	= bufferMemoryReqs.size;
		m_instanceRateBufferAlloc		= memAlloc.allocate(bufferMemoryReqs, MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(*m_device, *m_instanceRateBuffer, m_instanceRateBufferAlloc->getMemory(), m_instanceRateBufferAlloc->getOffset()));
		populateBufferWithTestValues(m_instanceRateBufferAlloc->getHostPtr(), (deUint32)m_instanceRateBufferAllocSize, m_inputFormat);
		flushMappedMemoryRange(vk, *m_device, m_instanceRateBufferAlloc->getMemory(), m_instanceRateBufferAlloc->getOffset(), VK_WHOLE_SIZE);
	}

	// Create vertex buffer that stores the vertex number (from 0 to m_numVertices - 1)
	{
		m_vertexNumBufferSize = 128 * sizeof(deInt32); // Allocate enough device memory for all indices (0 to 127).

		const VkBufferCreateInfo	vertexNumBufferParams	=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_vertexNumBufferSize,						// VkDeviceSize			size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_vertexNumBuffer		= createBuffer(vk, *m_device, &vertexNumBufferParams);
		m_vertexNumBufferAlloc	= memAlloc.allocate(getBufferMemoryRequirements(vk, *m_device, *m_vertexNumBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(*m_device, *m_vertexNumBuffer, m_vertexNumBufferAlloc->getMemory(), m_vertexNumBufferAlloc->getOffset()));
	}

	// Create index buffer if required
	if (!indices.empty())
	{
		m_indexBufferSize = sizeof(deUint32) * indices.size();

		const VkBufferCreateInfo	indexBufferParams	=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_indexBufferSize,							// VkDeviceSize			size;
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_indexBuffer		= createBuffer(vk, *m_device, &indexBufferParams);
		m_indexBufferAlloc	= memAlloc.allocate(getBufferMemoryRequirements(vk, *m_device, *m_indexBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(*m_device, *m_indexBuffer, m_indexBufferAlloc->getMemory(), m_indexBufferAlloc->getOffset()));
		deMemcpy(m_indexBufferAlloc->getHostPtr(), indices.data(), (size_t)m_indexBufferSize);
		flushMappedMemoryRange(vk, *m_device, m_indexBufferAlloc->getMemory(), m_indexBufferAlloc->getOffset(), VK_WHOLE_SIZE);
	}

	// Create result ssbo
	{
		const int	numChannels	= getNumUsedChannels(mapVkFormat(m_inputFormat).order);

		m_outBufferSize = getBufferSizeInBytes(m_numVertices * m_numInstances * numChannels * 3, VK_FORMAT_R32_UINT);

		const VkBufferCreateInfo	outBufferParams		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			m_outBufferSize,							// VkDeviceSize			size;
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		m_outBuffer			= createBuffer(vk, *m_device, &outBufferParams);
		m_outBufferAlloc	= memAlloc.allocate(getBufferMemoryRequirements(vk, *m_device, *m_outBuffer), MemoryRequirement::HostVisible);

		VK_CHECK(vk.bindBufferMemory(*m_device, *m_outBuffer, m_outBufferAlloc->getMemory(), m_outBufferAlloc->getOffset()));
		deMemset(m_outBufferAlloc->getHostPtr(), 0xFF, (size_t)m_outBufferSize);
		flushMappedMemoryRange(vk, *m_device, m_outBufferAlloc->getMemory(), m_outBufferAlloc->getOffset(), VK_WHOLE_SIZE);
	}

	// Create descriptor set data
	{
		DescriptorPoolBuilder descriptorPoolBuilder;
		descriptorPoolBuilder.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u);
		m_descriptorPool = descriptorPoolBuilder.build(vk, *m_device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

		DescriptorSetLayoutBuilder setLayoutBuilder;
		setLayoutBuilder.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
		m_descriptorSetLayout = setLayoutBuilder.build(vk, *m_device);

		const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,		// VkStructureType				sType;
			DE_NULL,											// const void*					pNext;
			*m_descriptorPool,									// VkDescriptorPool				desciptorPool;
			1u,													// deUint32						setLayoutCount;
			&m_descriptorSetLayout.get()						// const VkDescriptorSetLayout*	pSetLayouts;
		};

		m_descriptorSet = allocateDescriptorSet(vk, *m_device, &descriptorSetAllocateInfo);

		const VkDescriptorBufferInfo outBufferDescriptorInfo	= makeDescriptorBufferInfo(*m_outBuffer, 0ull, VK_WHOLE_SIZE);

		DescriptorSetUpdateBuilder setUpdateBuilder;
		setUpdateBuilder.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &outBufferDescriptorInfo);
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

	// Setup graphics test environment
	{
		GraphicsEnvironment::DrawConfig drawConfig;

		drawConfig.vertexBuffers.push_back(*m_vertexRateBuffer);
		drawConfig.vertexBuffers.push_back(*m_instanceRateBuffer);
		drawConfig.vertexBuffers.push_back(*m_vertexNumBuffer);

		drawConfig.vertexCount		= m_numVertices;
		drawConfig.instanceCount	= m_numInstances;
		drawConfig.indexBuffer		= *m_indexBuffer;
		drawConfig.indexCount		= (deUint32)(m_indexBufferSize / sizeof(deUint32));

		m_graphicsTestEnvironment	= de::MovePtr<GraphicsEnvironment>(new GraphicsEnvironment(m_context,
																							   *m_device,
																							   *m_descriptorSetLayout,
																							   *m_descriptorSet,
																							   GraphicsEnvironment::VertexBindings(bindings, bindings + DE_LENGTH_OF_ARRAY(bindings)),
																							   GraphicsEnvironment::VertexAttributes(attributes, attributes + DE_LENGTH_OF_ARRAY(attributes)),
																							   drawConfig));
	}
}

tcu::TestStatus VertexAccessInstance::iterate (void)
{
	const DeviceInterface&		vk			= m_context.getDeviceInterface();
	const vk::VkCommandBuffer	cmdBuffer	= m_graphicsTestEnvironment->getCommandBuffer();

	// Initialize vertex ids
	{
		deUint32 *bufferPtr = reinterpret_cast<deUint32*>(m_vertexNumBufferAlloc->getHostPtr());
		deMemset(bufferPtr, 0, (size_t)m_vertexNumBufferSize);

		initVertexIds(bufferPtr, (size_t)(m_vertexNumBufferSize / sizeof(deUint32)));

		flushMappedMemoryRange(vk, *m_device, m_vertexNumBufferAlloc->getMemory(), m_vertexNumBufferAlloc->getOffset(), VK_WHOLE_SIZE);
	}

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
			m_outBufferSize,						//  VkDeviceSize	size;
		};

		VK_CHECK(vk.invalidateMappedMemoryRanges(*m_device, 1u, &outBufferRange));
	}

	if (verifyResult())
		return tcu::TestStatus::pass("All values OK");
	else
		return tcu::TestStatus::fail("Invalid value(s) found");
}

bool VertexAccessInstance::verifyResult (void)
{
	std::ostringstream			logMsg;
	const DeviceInterface&		vk						= m_context.getDeviceInterface();
	tcu::TestLog&				log						= m_context.getTestContext().getLog();
	const deUint32				numChannels				= getNumUsedChannels(mapVkFormat(m_inputFormat).order);
	const deUint32				numScalarsPerVertex		= numChannels * 3; // Use 3 identical attributes
	void*						outDataPtr				= m_outBufferAlloc->getHostPtr();
	const deUint32				outValueSize			= sizeof(deUint32);
	bool						allOk					= true;

	const VkMappedMemoryRange	outBufferRange			=
	{
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,	// VkStructureType	sType;
		DE_NULL,								// const void*		pNext;
		m_outBufferAlloc->getMemory(),			// VkDeviceMemory	mem;
		m_outBufferAlloc->getOffset(),			// VkDeviceSize		offset;
		m_outBufferSize,						// VkDeviceSize		size;
	};

	VK_CHECK(vk.invalidateMappedMemoryRanges(*m_device, 1u, &outBufferRange));

	for (deUint32 valueNdx = 0; valueNdx < m_outBufferSize / outValueSize; valueNdx++)
	{
		deUint32			numInBufferValues;
		void*				inBufferPtr;
		VkDeviceSize		inBufferAllocSize;
		deUint32			inBufferValueIndex;
		bool				isOutOfBoundsAccess		= false;
		const deUint32		attributeIndex			= (valueNdx / numChannels) % 3;
		const deUint32*		outValuePtr				= (deUint32*)outDataPtr + valueNdx;

		if (attributeIndex == 2)
		{
			// Instance rate
			const deUint32	elementIndex	= valueNdx / (numScalarsPerVertex * m_numVertices); // instance id

			numInBufferValues	= m_numInstanceValues;
			inBufferPtr			= m_instanceRateBufferAlloc->getHostPtr();
			inBufferAllocSize	= m_instanceRateBufferAllocSize;
			inBufferValueIndex	= (elementIndex * numChannels) + (valueNdx % numScalarsPerVertex) - (2 * numChannels);
		}
		else
		{
			// Vertex rate
			const deUint32	vertexNdx		= valueNdx / numScalarsPerVertex;
			const deUint32	instanceNdx		= vertexNdx / m_numVertices;
			const deUint32	elementIndex	= valueNdx / numScalarsPerVertex; // vertex id

			numInBufferValues	= m_numVertexValues;
			inBufferPtr			= m_vertexRateBufferAlloc->getHostPtr();
			inBufferAllocSize	= m_vertexRateBufferAllocSize;
			inBufferValueIndex	= (getIndex(elementIndex) * (numChannels * 2)) + (valueNdx % numScalarsPerVertex) - instanceNdx * (m_numVertices * numChannels * 2);

			// Binding 0 contains two attributes, so bounds checking for attribute 0 must also consider attribute 1 to determine if the binding is out of bounds.
			if ((attributeIndex == 0) && (numInBufferValues >= numChannels))
				numInBufferValues -= numChannels;
		}

		isOutOfBoundsAccess	= (inBufferValueIndex >= numInBufferValues);

		const deInt32		distanceToOutOfBounds	= (deInt32)outValueSize * ((deInt32)numInBufferValues - (deInt32)inBufferValueIndex);

		if (!isOutOfBoundsAccess && (distanceToOutOfBounds < 16))
			isOutOfBoundsAccess = (((inBufferValueIndex / numChannels) + 1) * numChannels > numInBufferValues);

		// Log value information
		{
			// Vertex separator
			if (valueNdx && valueNdx % numScalarsPerVertex == 0)
				logMsg << "\n";

			logMsg << "\n" << valueNdx << ": Value ";

			// Result index and value
			if (m_inputFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
				logValue(logMsg, outValuePtr, VK_FORMAT_R32_SFLOAT, 4);
			else
				logValue(logMsg, outValuePtr, m_inputFormat, 4);

			// Attribute name
			logMsg << "\tfrom attr" << attributeIndex;
			if (numChannels > 1)
				logMsg << "[" << valueNdx % numChannels << "]";

			// Input rate
			if (attributeIndex == 2)
				logMsg << "\tinstance rate";
			else
				logMsg << "\tvertex rate";
		}

		if (isOutOfBoundsAccess)
		{
			const bool isValidValue = isValueWithinVertexBufferOrZero(inBufferPtr, inBufferAllocSize, outValuePtr, inBufferValueIndex);

			logMsg << "\t(out of bounds)";

			if (!isValidValue)
			{
				// Check if we are satisfying the [0, 0, 0, x] pattern, where x may be either 0 or 1,
				// or the maximum representable positive integer value (if the format is integer-based).

				const bool	canMatchVec4Pattern	= ((valueNdx % numChannels == 3) || m_inputFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32);
				bool		matchesVec4Pattern	= false;

				if (canMatchVec4Pattern)
				{
					if (m_inputFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
						matchesVec4Pattern	=  verifyOutOfBoundsVec4(outValuePtr - 3, VK_FORMAT_R32G32B32_SFLOAT);
					else
						matchesVec4Pattern	=  verifyOutOfBoundsVec4(outValuePtr - 3, m_inputFormat);
				}

				if (!canMatchVec4Pattern || !matchesVec4Pattern)
				{
					logMsg << ", Failed: expected a value within the buffer range or 0";

					if (canMatchVec4Pattern)
						logMsg << ", or the [0, 0, 0, x] pattern";

					allOk = false;
				}
			}
		}
		else if (!isExpectedValueFromVertexBuffer(inBufferPtr, inBufferValueIndex, m_inputFormat, outValuePtr))
		{
			logMsg << ", Failed: unexpected value";
			allOk = false;
		}
	}
	log << tcu::TestLog::Message << logMsg.str() << tcu::TestLog::EndMessage;

	return allOk;
}

bool VertexAccessInstance::isValueWithinVertexBufferOrZero(void* vertexBuffer, VkDeviceSize vertexBufferSize, const void* value, deUint32 valueIndex)
{
	if (m_inputFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
	{
		const float		normValue		= *reinterpret_cast<const float*>(value);
		const deUint32	scalarIndex		= valueIndex % 4;
		const bool		isAlpha			= (scalarIndex == 3);
		deUint32		encodedValue;

		if (isAlpha)
			encodedValue = deMin32(deUint32(normValue * 0x3u), 0x3u);
		else
			encodedValue = deMin32(deUint32(normValue * 0x3FFu), 0x3FFu);

		if (encodedValue == 0)
			return true;

		for (deUint32 i = 0; i < vertexBufferSize / 4; i++)
		{
			const deUint32	packedValue		= reinterpret_cast<deUint32*>(vertexBuffer)[i];
			deUint32		unpackedValue;

			if (scalarIndex < 3)
				unpackedValue = (packedValue >> (10 * scalarIndex)) & 0x3FFu;
			else
				unpackedValue = (packedValue >> 30) & 0x3u;

			if (unpackedValue == encodedValue)
				return true;
		}

		return false;
	}
	else
	{
		return isValueWithinBufferOrZero(vertexBuffer, vertexBufferSize, value, sizeof(deUint32));
	}
}

bool VertexAccessInstance::isExpectedValueFromVertexBuffer (const void* vertexBuffer, deUint32 vertexIndex, VkFormat vertexFormat, const void* value)
{
	if (isUintFormat(vertexFormat))
	{
		const deUint32* bufferPtr = reinterpret_cast<const deUint32*>(vertexBuffer);

		return bufferPtr[vertexIndex] == *reinterpret_cast<const deUint32 *>(value);
	}
	else if (isIntFormat(vertexFormat))
	{
		const deInt32* bufferPtr = reinterpret_cast<const deInt32*>(vertexBuffer);

		return bufferPtr[vertexIndex] == *reinterpret_cast<const deInt32 *>(value);
	}
	else if (isFloatFormat(vertexFormat))
	{
		const float* bufferPtr = reinterpret_cast<const float*>(vertexBuffer);

		return areEqual(bufferPtr[vertexIndex], *reinterpret_cast<const float *>(value));
	}
	else if (vertexFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
	{
		const deUint32*	bufferPtr		= reinterpret_cast<const deUint32*>(vertexBuffer);
		const deUint32	packedValue		= bufferPtr[vertexIndex / 4];
		const deUint32	scalarIndex		= vertexIndex % 4;
		float			normValue;

		if (scalarIndex < 3)
			normValue = float((packedValue >> (10 * scalarIndex)) & 0x3FFu) / 0x3FFu;
		else
			normValue = float(packedValue >> 30) / 0x3u;

		return areEqual(normValue, *reinterpret_cast<const float *>(value));
	}

	DE_ASSERT(false);
	return false;
}

VkDeviceSize VertexAccessInstance::getBufferSizeInBytes (deUint32 numScalars, VkFormat format)
{
	if (isUintFormat(format) || isIntFormat(format) || isFloatFormat(format))
	{
		return numScalars * 4;
	}
	else if (format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
	{
		DE_ASSERT(numScalars % 4 == 0);
		return numScalars;
	}

	DE_ASSERT(false);
	return 0;
}

// DrawAccessInstance

DrawAccessInstance::DrawAccessInstance (Context&				context,
										Move<VkDevice>			device,
										VkFormat				inputFormat,
										deUint32				numVertexValues,
										deUint32				numInstanceValues,
										deUint32				numVertices,
										deUint32				numInstances)
	: VertexAccessInstance (context,
							device,
							inputFormat,
							numVertexValues,
							numInstanceValues,
							numVertices,
							numInstances,
							std::vector<deUint32>()) // No index buffer
{
}

void DrawAccessInstance::initVertexIds (deUint32 *indicesPtr, size_t indexCount)
{
	for (deUint32 i = 0; i < indexCount; i++)
		indicesPtr[i] = i;
}

deUint32 DrawAccessInstance::getIndex (deUint32 vertexNum) const
{
	return vertexNum;
}

// DrawIndexedAccessInstance

DrawIndexedAccessInstance::DrawIndexedAccessInstance (Context&						context,
													  Move<VkDevice>				device,
													  VkFormat						inputFormat,
													  deUint32						numVertexValues,
													  deUint32						numInstanceValues,
													  deUint32						numVertices,
													  deUint32						numInstances,
													  const std::vector<deUint32>&	indices)
	: VertexAccessInstance	(context,
							 device,
							 inputFormat,
							 numVertexValues,
							 numInstanceValues,
							 numVertices,
							 numInstances,
							 indices)
	, m_indices				(indices)
{
}

void DrawIndexedAccessInstance::initVertexIds (deUint32 *indicesPtr, size_t indexCount)
{
	DE_UNREF(indexCount);

	for (deUint32 i = 0; i < m_indices.size(); i++)
	{
		DE_ASSERT(m_indices[i] < indexCount);

		indicesPtr[m_indices[i]] = i;
	}
}

deUint32 DrawIndexedAccessInstance::getIndex (deUint32 vertexNum) const
{
	DE_ASSERT(vertexNum < (deUint32)m_indices.size());

	return m_indices[vertexNum];
}

// Test node creation functions

static tcu::TestCaseGroup* createDrawTests (tcu::TestContext& testCtx, VkFormat format)
{
	struct TestConfig
	{
		std::string		name;
		std::string		description;
		VkFormat		inputFormat;
		deUint32		numVertexValues;
		deUint32		numInstanceValues;
		deUint32		numVertices;
		deUint32		numInstances;
	};

	const deUint32 numChannels = getNumUsedChannels(mapVkFormat(format).order);

	const TestConfig testConfigs[] =
	{
		// name						description											format	numVertexValues			numInstanceValues	numVertices		numInstances
		{ "vertex_out_of_bounds",	"Create data for 6 vertices, draw 9 vertices",		format,	numChannels * 2 * 6,	numChannels,		9,				1	 },
		{ "vertex_incomplete",		"Create data for half a vertex, draw 3 vertices",	format,	numChannels,			numChannels,		3,				1	 },
		{ "instance_out_of_bounds", "Create data for 1 instance, draw 3 instances",		format,	numChannels * 2 * 9,	numChannels,		3,				3	 },
	};

	de::MovePtr<tcu::TestCaseGroup>	drawTests (new tcu::TestCaseGroup(testCtx, "draw", ""));

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(testConfigs); i++)
	{
		const TestConfig &config = testConfigs[i];

		drawTests->addChild(new DrawAccessTest(testCtx, config.name, config.description, config.inputFormat,
											   config.numVertexValues, config.numInstanceValues,
											   config.numVertices, config.numInstances));
	}

	return drawTests.release();
}

static tcu::TestCaseGroup* createDrawIndexedTests (tcu::TestContext& testCtx, VkFormat format)
{
	struct TestConfig
	{
		std::string							name;
		std::string							description;
		VkFormat							inputFormat;
		DrawIndexedAccessTest::IndexConfig	indexConfig;
	};

	const TestConfig testConfigs[] =
	{
		// name							description								format		indexConfig
		{ "last_index_out_of_bounds",	"Only last index is out of bounds",		format,		DrawIndexedAccessTest::INDEX_CONFIG_LAST_INDEX_OUT_OF_BOUNDS },
		{ "indices_out_of_bounds",		"Random indices out of bounds",			format,		DrawIndexedAccessTest::INDEX_CONFIG_INDICES_OUT_OF_BOUNDS },
		{ "triangle_out_of_bounds",		"First triangle is out of bounds",		format,		DrawIndexedAccessTest::INDEX_CONFIG_TRIANGLE_OUT_OF_BOUNDS },
	};

	de::MovePtr<tcu::TestCaseGroup>	drawTests (new tcu::TestCaseGroup(testCtx, "draw_indexed", ""));

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(testConfigs); i++)
	{
		const TestConfig &config = testConfigs[i];

		drawTests->addChild(new DrawIndexedAccessTest(testCtx, config.name, config.description, config.inputFormat, config.indexConfig));
	}

	return drawTests.release();
}

static void addVertexFormatTests (tcu::TestContext& testCtx, tcu::TestCaseGroup* parentGroup)
{
	const VkFormat vertexFormats[] =
	{
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,

		VK_FORMAT_A2B10G10R10_UNORM_PACK32
	};

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(vertexFormats); i++)
	{
		const std::string				formatName	= getFormatName(vertexFormats[i]);
		de::MovePtr<tcu::TestCaseGroup>	formatGroup	(new tcu::TestCaseGroup(testCtx, de::toLower(formatName.substr(10)).c_str(), ""));

		formatGroup->addChild(createDrawTests(testCtx, vertexFormats[i]));
		formatGroup->addChild(createDrawIndexedTests(testCtx, vertexFormats[i]));

		parentGroup->addChild(formatGroup.release());
	}
}

tcu::TestCaseGroup* createVertexAccessTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> vertexAccessTests	(new tcu::TestCaseGroup(testCtx, "vertex_access", ""));

	addVertexFormatTests(testCtx, vertexAccessTests.get());

	return vertexAccessTests.release();
}

} // robustness
} // vkt
