/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
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
 * \brief Atomic operations (OpAtomic*) tests.
 *//*--------------------------------------------------------------------*/

#include "vktAtomicOperationTests.hpp"
#include "vktShaderExecutor.hpp"

#include "vkRefUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuResultCollector.hpp"

#include "deStringUtil.hpp"
#include "deSharedPtr.hpp"
#include "deRandom.hpp"
#include "deArrayUtil.hpp"

#include <string>

namespace vkt
{
namespace shaderexecutor
{

namespace
{

using de::UniquePtr;
using de::MovePtr;
using std::vector;

using namespace vk;

// Buffer helper
class Buffer
{
public:
						Buffer				(Context& context, VkBufferUsageFlags usage, size_t size);

	VkBuffer			getBuffer			(void) const { return *m_buffer;					}
	void*				getHostPtr			(void) const { return m_allocation->getHostPtr();	}
	void				flush				(void);
	void				invalidate			(void);

private:
	const DeviceInterface&		m_vkd;
	const VkDevice				m_device;
	const Unique<VkBuffer>		m_buffer;
	const UniquePtr<Allocation>	m_allocation;
};

typedef de::SharedPtr<Buffer> BufferSp;

Move<VkBuffer> createBuffer (const DeviceInterface& vkd, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usageFlags)
{
	const VkBufferCreateInfo createInfo	=
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
	MovePtr<Allocation>	alloc(allocator.allocate(getBufferMemoryRequirements(vkd, device, buffer), MemoryRequirement::HostVisible));

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

// Tests

enum AtomicOperation
{
	ATOMIC_OP_EXCHANGE = 0,
	ATOMIC_OP_COMP_SWAP,
	ATOMIC_OP_ADD,
	ATOMIC_OP_MIN,
	ATOMIC_OP_MAX,
	ATOMIC_OP_AND,
	ATOMIC_OP_OR,
	ATOMIC_OP_XOR,

	ATOMIC_OP_LAST
};

std::string atomicOp2Str (AtomicOperation op)
{
	static const char* const s_names[] =
	{
		"atomicExchange",
		"atomicCompSwap",
		"atomicAdd",
		"atomicMin",
		"atomicMax",
		"atomicAnd",
		"atomicOr",
		"atomicXor"
	};
	return de::getSizedArrayElement<ATOMIC_OP_LAST>(s_names, op);
}

enum
{
	NUM_ELEMENTS = 32
};

class AtomicOperationCaseInstance : public TestInstance
{
public:
									AtomicOperationCaseInstance		(Context&			context,
																	 const ShaderSpec&	shaderSpec,
																	 glu::ShaderType	shaderType,
																	 bool				sign,
																	 AtomicOperation	atomicOp);
	virtual							~AtomicOperationCaseInstance	(void);

	virtual tcu::TestStatus			iterate							(void);

private:
	const ShaderSpec&				m_shaderSpec;
	glu::ShaderType					m_shaderType;
	bool							m_sign;
	AtomicOperation					m_atomicOp;

	struct BufferInterface
	{
		// Use half the number of elements for inout to cause overlap between atomic operations.
		// Each inout element at index i will have two atomic operations using input from
		// indices i and i + NUM_ELEMENTS / 2.
		deInt32		index;
		deUint32	inout[NUM_ELEMENTS / 2];
		deUint32	input[NUM_ELEMENTS];
		deUint32	compare[NUM_ELEMENTS];
		deUint32	output[NUM_ELEMENTS];
	};

	template<typename T>
	struct Expected
	{
		T m_inout;
		T m_output[2];

		Expected (T inout, T output0, T output1)
		: m_inout(inout)
		{
			m_output[0] = output0;
			m_output[1] = output1;
		}

		bool compare (deUint32 inout, deUint32 output0, deUint32 output1)
		{
			return (deMemCmp((const void*)&m_inout, (const void*)&inout, sizeof(inout)) == 0
					&& deMemCmp((const void*)&m_output[0], (const void*)&output0, sizeof(output0)) == 0
					&& deMemCmp((const void*)&m_output[1], (const void*)&output1, sizeof(output1)) == 0);
		}
	};

	template<typename T> void checkOperation	(const BufferInterface&	original,
												 const BufferInterface&	result,
												 tcu::ResultCollector&	resultCollector);

};

AtomicOperationCaseInstance::AtomicOperationCaseInstance (Context&			context,
														  const ShaderSpec&	shaderSpec,
														  glu::ShaderType	shaderType,
														  bool				sign,
														  AtomicOperation	atomicOp)
	: TestInstance	(context)
	, m_shaderSpec	(shaderSpec)
	, m_shaderType	(shaderType)
	, m_sign		(sign)
	, m_atomicOp	(atomicOp)
{
}

AtomicOperationCaseInstance::~AtomicOperationCaseInstance (void)
{
}

// Use template to handle both signed and unsigned cases. SPIR-V should
// have separate operations for both.
template<typename T>
void AtomicOperationCaseInstance::checkOperation (const BufferInterface&	original,
												  const BufferInterface&	result,
												  tcu::ResultCollector&		resultCollector)
{
	// originalInout = original inout
	// input0 = input at index i
	// iinput1 = input at index i + NUM_ELEMENTS / 2
	//
	// atomic operation will return the memory contents before
	// the operation and this is stored as output. Two operations
	// are executed for each InOut value (using input0 and input1).
	//
	// Since there is an overlap of two operations per each
	// InOut element, the outcome of the resulting InOut and
	// the outputs of the operations have two result candidates
	// depending on the execution order. Verification passes
	// if the results match one of these options.

	for (int elementNdx = 0; elementNdx < NUM_ELEMENTS / 2; elementNdx++)
	{
		// Needed when reinterpeting the data as signed values.
		const T originalInout	= *reinterpret_cast<const T*>(&original.inout[elementNdx]);
		const T input0			= *reinterpret_cast<const T*>(&original.input[elementNdx]);
		const T input1			= *reinterpret_cast<const T*>(&original.input[elementNdx + NUM_ELEMENTS / 2]);

		// Expected results are collected to this vector.
		vector<Expected<T> > exp;

		switch (m_atomicOp)
		{
			case ATOMIC_OP_ADD:
			{
				exp.push_back(Expected<T>(originalInout + input0 + input1, originalInout, originalInout + input0));
				exp.push_back(Expected<T>(originalInout + input0 + input1, originalInout + input1, originalInout));
			}
			break;

			case ATOMIC_OP_AND:
			{
				exp.push_back(Expected<T>(originalInout & input0 & input1, originalInout, originalInout & input0));
				exp.push_back(Expected<T>(originalInout & input0 & input1, originalInout & input1, originalInout));
			}
			break;

			case ATOMIC_OP_OR:
			{
				exp.push_back(Expected<T>(originalInout | input0 | input1, originalInout, originalInout | input0));
				exp.push_back(Expected<T>(originalInout | input0 | input1, originalInout | input1, originalInout));
			}
			break;

			case ATOMIC_OP_XOR:
			{
				exp.push_back(Expected<T>(originalInout ^ input0 ^ input1, originalInout, originalInout ^ input0));
				exp.push_back(Expected<T>(originalInout ^ input0 ^ input1, originalInout ^ input1, originalInout));
			}
			break;

			case ATOMIC_OP_MIN:
			{
				exp.push_back(Expected<T>(de::min(de::min(originalInout, input0), input1), originalInout, de::min(originalInout, input0)));
				exp.push_back(Expected<T>(de::min(de::min(originalInout, input0), input1), de::min(originalInout, input1), originalInout));
			}
			break;

			case ATOMIC_OP_MAX:
			{
				exp.push_back(Expected<T>(de::max(de::max(originalInout, input0), input1), originalInout, de::max(originalInout, input0)));
				exp.push_back(Expected<T>(de::max(de::max(originalInout, input0), input1), de::max(originalInout, input1), originalInout));
			}
			break;

			case ATOMIC_OP_EXCHANGE:
			{
				exp.push_back(Expected<T>(input1, originalInout, input0));
				exp.push_back(Expected<T>(input0, input1, originalInout));
			}
			break;

			case ATOMIC_OP_COMP_SWAP:
			{
				if (elementNdx % 2 == 0)
				{
					exp.push_back(Expected<T>(input0, originalInout, input0));
					exp.push_back(Expected<T>(input0, originalInout, originalInout));
				}
				else
				{
					exp.push_back(Expected<T>(input1, input1, originalInout));
					exp.push_back(Expected<T>(input1, originalInout, originalInout));
				}
			}
			break;


			default:
				DE_FATAL("Unexpected atomic operation.");
				break;
		};

		const deUint32 resIo		= result.inout[elementNdx];
		const deUint32 resOutput0	= result.output[elementNdx];
		const deUint32 resOutput1	= result.output[elementNdx + NUM_ELEMENTS / 2];

		if (!exp[0].compare(resIo, resOutput0, resOutput1) && !exp[1].compare(resIo, resOutput0, resOutput1))
		{
			std::ostringstream errorMessage;
			errorMessage	<< "ERROR: Result value check failed at index " << elementNdx
							<< ". Expected one of the two outcomes: InOut = " << tcu::toHex(exp[0].m_inout)
							<< ", Output0 = " << tcu::toHex(exp[0].m_output[0]) << ", Output1 = "
							<< tcu::toHex(exp[0].m_output[1]) << ", or InOut = " << tcu::toHex(exp[1].m_inout)
							<< ", Output0 = " << tcu::toHex(exp[1].m_output[0]) << ", Output1 = "
							<< tcu::toHex(exp[1].m_output[1]) << ". Got: InOut = " << tcu::toHex(resIo)
							<< ", Output0 = " << tcu::toHex(resOutput0) << ", Output1 = "
							<< tcu::toHex(resOutput1) << ". Using Input0 = " << tcu::toHex(original.input[elementNdx])
							<< " and Input1 = " << tcu::toHex(original.input[elementNdx + NUM_ELEMENTS / 2]) << ".";

			resultCollector.fail(errorMessage.str());
		}
	}
}

tcu::TestStatus AtomicOperationCaseInstance::iterate (void)
{
	//Check stores and atomic operation support.
	switch (m_shaderType)
	{
		case glu::SHADERTYPE_VERTEX:
		case glu::SHADERTYPE_TESSELLATION_CONTROL:
		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
		case glu::SHADERTYPE_GEOMETRY:
			if(!m_context.getDeviceFeatures().vertexPipelineStoresAndAtomics)
				TCU_THROW(NotSupportedError, "Stores and atomic operations are not supported in Vertex, Tessellation, and Geometry shader.");
			break;
		case glu::SHADERTYPE_FRAGMENT:
			if(!m_context.getDeviceFeatures().fragmentStoresAndAtomics)
				TCU_THROW(NotSupportedError, "Stores and atomic operations are not supported in fragment shader.");
			break;
		case glu::SHADERTYPE_COMPUTE:
			break;
		default:
			DE_FATAL("Unsupported shader type");
	}

	tcu::TestLog&				log			= m_context.getTestContext().getLog();
	const DeviceInterface&		vkd			= m_context.getDeviceInterface();
	const VkDevice				device		= m_context.getDevice();
	de::Random					rnd			(0x62a15e34);
	Buffer						buffer		(m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(BufferInterface));
	BufferInterface*			ptr			= (BufferInterface*)buffer.getHostPtr();

	for (int i = 0; i < NUM_ELEMENTS / 2; i++)
	{
		ptr->inout[i] = rnd.getUint32();
		// The first half of compare elements match with every even index.
		// The second half matches with odd indices. This causes the
		// overlapping operations to only select one.
		ptr->compare[i] = ptr->inout[i] + (i % 2);
		ptr->compare[i + NUM_ELEMENTS / 2] = ptr->inout[i] + 1 - (i % 2);
	}
	for (int i = 0; i < NUM_ELEMENTS; i++)
	{
		ptr->input[i] = rnd.getUint32();
		ptr->output[i] = 0xcdcdcdcd;
	}
	ptr->index = 0;

	// Take a copy to be used when calculating expected values.
	BufferInterface original = *ptr;

	buffer.flush();

	Move<VkDescriptorSetLayout>	extraResourcesLayout;
	Move<VkDescriptorPool>		extraResourcesSetPool;
	Move<VkDescriptorSet>		extraResourcesSet;

	const VkDescriptorSetLayoutBinding bindings[] =
	{
		{ 0u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, DE_NULL }
	};

	const VkDescriptorSetLayoutCreateInfo	layoutInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		DE_NULL,
		(VkDescriptorSetLayoutCreateFlags)0u,
		DE_LENGTH_OF_ARRAY(bindings),
		bindings
	};

	extraResourcesLayout = createDescriptorSetLayout(vkd, device, &layoutInfo);

	const VkDescriptorPoolSize poolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u }
	};
	const VkDescriptorPoolCreateInfo poolInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		DE_NULL,
		(VkDescriptorPoolCreateFlags)VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		1u,		// maxSets
		DE_LENGTH_OF_ARRAY(poolSizes),
		poolSizes
	};

	extraResourcesSetPool = createDescriptorPool(vkd, device, &poolInfo);

	const VkDescriptorSetAllocateInfo allocInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		DE_NULL,
		*extraResourcesSetPool,
		1u,
		&extraResourcesLayout.get()
	};

	extraResourcesSet = allocateDescriptorSet(vkd, device, &allocInfo);

	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer	= buffer.getBuffer();
	bufferInfo.offset	= 0u;
	bufferInfo.range	= VK_WHOLE_SIZE;

	const VkWriteDescriptorSet descriptorWrite =
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
		(const VkBufferView*)DE_NULL
	};


	vkd.updateDescriptorSets(device, 1u, &descriptorWrite, 0u, DE_NULL);

	// Storage for output varying data.
	std::vector<deUint32>	outputs		(NUM_ELEMENTS);
	std::vector<void*>		outputPtr	(NUM_ELEMENTS);

	for (size_t i = 0; i < NUM_ELEMENTS; i++)
	{
		outputs[i] = 0xcdcdcdcd;
		outputPtr[i] = &outputs[i];
	}

	UniquePtr<ShaderExecutor> executor(createExecutor(m_context, m_shaderType, m_shaderSpec, *extraResourcesLayout));
	executor->execute(NUM_ELEMENTS, DE_NULL, &outputPtr[0], *extraResourcesSet);
	buffer.invalidate();

	tcu::ResultCollector resultCollector(log);

	// Check the results of the atomic operation
	if (m_sign)
		checkOperation<deInt32>(original, *ptr, resultCollector);
	else
		checkOperation<deUint32>(original, *ptr, resultCollector);

	return tcu::TestStatus(resultCollector.getResult(), resultCollector.getMessage());
}

class AtomicOperationCase : public TestCase
{
public:
							AtomicOperationCase		(tcu::TestContext&		testCtx,
													 const char*			name,
													 const char*			description,
													 glu::ShaderType		type,
													 bool					sign,
													 AtomicOperation		atomicOp);
	virtual					~AtomicOperationCase	(void);

	virtual TestInstance*	createInstance			(Context& ctx) const;
	virtual void			initPrograms			(vk::SourceCollections& programCollection) const
	{
		generateSources(m_shaderType, m_shaderSpec, programCollection);
	}

private:

	void					createShaderSpec();
	ShaderSpec				m_shaderSpec;
	const glu::ShaderType	m_shaderType;
	const bool				m_sign;
	const AtomicOperation	m_atomicOp;
};

AtomicOperationCase::AtomicOperationCase (tcu::TestContext&	testCtx,
										  const char*		name,
										  const char*		description,
										  glu::ShaderType	shaderType,
										  bool				sign,
										  AtomicOperation	atomicOp)
	: TestCase			(testCtx, name, description)
	, m_shaderType		(shaderType)
	, m_sign			(sign)
	, m_atomicOp		(atomicOp)
{
	createShaderSpec();
	init();
}

AtomicOperationCase::~AtomicOperationCase (void)
{
}

TestInstance* AtomicOperationCase::createInstance (Context& ctx) const
{
	return new AtomicOperationCaseInstance(ctx, m_shaderSpec, m_shaderType, m_sign, m_atomicOp);
}

void AtomicOperationCase::createShaderSpec (void)
{
	const tcu::StringTemplate shaderTemplateGlobal(
		"layout (set = ${SETIDX}, binding = 0, std430) buffer AtomicBuffer\n"
		"{\n"
		"    highp int index;\n"
		"    highp ${DATATYPE} inoutValues[${N}/2];\n"
		"    highp ${DATATYPE} inputValues[${N}];\n"
		"    highp ${DATATYPE} compareValues[${N}];\n"
		"    highp ${DATATYPE} outputValues[${N}];\n"
		"} buf;\n");

	std::map<std::string, std::string> specializations;
	specializations["DATATYPE"] = m_sign ? "int" : "uint";
	specializations["ATOMICOP"] = atomicOp2Str(m_atomicOp);
	specializations["SETIDX"] = de::toString((int)EXTRA_RESOURCES_DESCRIPTOR_SET_INDEX);
	specializations["N"] = de::toString((int)NUM_ELEMENTS);
	specializations["COMPARE_ARG"] = m_atomicOp == ATOMIC_OP_COMP_SWAP ? "buf.compareValues[idx], " : "";

	const tcu::StringTemplate shaderTemplateSrc(
		"int idx = atomicAdd(buf.index, 1);\n"
		"buf.outputValues[idx] = ${ATOMICOP}(buf.inoutValues[idx % (${N}/2)], ${COMPARE_ARG}buf.inputValues[idx]);\n");

	m_shaderSpec.outputs.push_back(Symbol("outData", glu::VarType(glu::TYPE_UINT, glu::PRECISION_HIGHP)));
	m_shaderSpec.globalDeclarations = shaderTemplateGlobal.specialize(specializations);
	m_shaderSpec.source = shaderTemplateSrc.specialize(specializations);
}

void addAtomicOperationTests (tcu::TestCaseGroup* atomicOperationTestsGroup)
{
	tcu::TestContext& testCtx = atomicOperationTestsGroup->getTestContext();

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

	static const struct
	{
		bool			value;
		const char*		name;
		const char*		description;
	} dataSign[] =
	{
		{ true,		"signed",	"Tests using signed data (int)"		},
		{ false,	"unsigned",	"Tests using unsigned data (uint)"	}
	};

	static const struct
	{
		AtomicOperation		value;
		const char*			name;
	} atomicOp[] =
	{
		{ ATOMIC_OP_EXCHANGE,	"exchange"	},
		{ ATOMIC_OP_COMP_SWAP,	"comp_swap"	},
		{ ATOMIC_OP_ADD,		"add"		},
		{ ATOMIC_OP_MIN,		"min"		},
		{ ATOMIC_OP_MAX,		"max"		},
		{ ATOMIC_OP_AND,		"and"		},
		{ ATOMIC_OP_OR,			"or"		},
		{ ATOMIC_OP_XOR,		"xor"		}
	};

	for (int opNdx = 0; opNdx < DE_LENGTH_OF_ARRAY(atomicOp); opNdx++)
	{
		for (int signNdx = 0; signNdx < DE_LENGTH_OF_ARRAY(dataSign); signNdx++)
		{
			for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(shaderTypes); shaderTypeNdx++)
			{
				const std::string description = std::string("Tests atomic operation ") + atomicOp2Str(atomicOp[opNdx].value) + std::string(".");
				std::string name = std::string(atomicOp[opNdx].name) + "_" + std::string(dataSign[signNdx].name) + "_" + std::string(shaderTypes[shaderTypeNdx].name);
				atomicOperationTestsGroup->addChild(new AtomicOperationCase(testCtx, name.c_str(), description.c_str(), shaderTypes[shaderTypeNdx].type, dataSign[signNdx].value, atomicOp[opNdx].value));
			}
		}
	}
}

} // anonymous

tcu::TestCaseGroup* createAtomicOperationTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "atomic_operations", "Atomic Operation Tests", addAtomicOperationTests);
}

} // shaderexecutor
} // vkt
