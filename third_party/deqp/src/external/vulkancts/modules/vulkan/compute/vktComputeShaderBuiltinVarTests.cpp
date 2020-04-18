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
 * \brief Compute Shader Built-in variable tests.
 *//*--------------------------------------------------------------------*/

#include "vktComputeShaderBuiltinVarTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktComputeTestsUtil.hpp"

#include "vkDefs.hpp"
#include "vkPlatform.hpp"
#include "vkRef.hpp"
#include "vkPrograms.hpp"
#include "vkStrUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkDeviceUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkBuilderUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuVectorUtil.hpp"

#include "gluShaderUtil.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"

#include <map>
#include <string>
#include <vector>

namespace vkt
{
namespace compute
{
namespace
{

using namespace vk;
using std::string;
using std::vector;
using std::map;
using tcu::TestLog;
using tcu::UVec3;
using tcu::IVec3;

class ComputeBuiltinVarInstance;
class ComputeBuiltinVarCase;

static const string s_prefixProgramName ="compute_";

static inline bool compareNumComponents (const UVec3& a, const UVec3& b,const int numComps)
{
	DE_ASSERT(numComps == 1 || numComps == 3);
	return numComps == 3 ? tcu::allEqual(a, b) : a.x() == b.x();
}

static inline UVec3 readResultVec (const deUint32* ptr, const int numComps)
{
	UVec3 res;
	for (int ndx = 0; ndx < numComps; ndx++)
		res[ndx] = ptr[ndx];
	return res;
}

struct LogComps
{
	const UVec3&	v;
	int				numComps;

					LogComps	(const UVec3 &v_, int numComps_) : v(v_), numComps(numComps_) {}
};

static inline std::ostream& operator<< (std::ostream& str, const LogComps& c)
{
	DE_ASSERT(c.numComps == 1 || c.numComps == 3);
	return c.numComps == 3 ? str << c.v : str << c.v.x();
}

class SubCase
{
public:
	// Use getters instead of public const members, because SubCase must be assignable
	// in order to be stored in a vector.

	const UVec3&	localSize		(void) const { return m_localSize; }
	const UVec3&	numWorkGroups	(void) const { return m_numWorkGroups; }

					SubCase			(void) {}
					SubCase			(const UVec3& localSize_, const UVec3& numWorkGroups_)
						: m_localSize		(localSize_)
						, m_numWorkGroups	(numWorkGroups_) {}

private:
	UVec3	m_localSize;
	UVec3	m_numWorkGroups;
};


class ComputeBuiltinVarInstance : public vkt::TestInstance
{
public:
									ComputeBuiltinVarInstance	(Context&						context,
																 const vector<SubCase>&			subCases,
																 const glu::DataType			varType,
																 const ComputeBuiltinVarCase*	builtinVarCase);

	virtual tcu::TestStatus			iterate						(void);

private:
	const VkDevice					m_device;
	const DeviceInterface&			m_vki;
	const VkQueue					m_queue;
	const deUint32					m_queueFamilyIndex;
	vector<SubCase>					m_subCases;
	const ComputeBuiltinVarCase*	m_builtin_var_case;
	int								m_subCaseNdx;
	const glu::DataType				m_varType;
};

class ComputeBuiltinVarCase : public vkt::TestCase
{
public:
							ComputeBuiltinVarCase	(tcu::TestContext& context, const string& name, const char* varName, glu::DataType varType, bool readByComponent);
							~ComputeBuiltinVarCase	(void);

	TestInstance*			createInstance			(Context& context) const
	{
		return new ComputeBuiltinVarInstance(context, m_subCases, m_varType, this);
	};

	virtual void			initPrograms			(SourceCollections& programCollection) const;
	virtual UVec3			computeReference		(const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const = 0;

protected:
	string					genBuiltinVarSource		(const string& varName, glu::DataType varType, const UVec3& localSize, bool readByComponent) const;
	vector<SubCase>			m_subCases;

private:
	deUint32				getProgram				(const tcu::UVec3& localSize);

	const string			m_varName;
	const glu::DataType		m_varType;
	int						m_subCaseNdx;
	bool					m_readByComponent;

	ComputeBuiltinVarCase (const ComputeBuiltinVarCase& other);
	ComputeBuiltinVarCase& operator= (const ComputeBuiltinVarCase& other);
};

ComputeBuiltinVarCase::ComputeBuiltinVarCase (tcu::TestContext& context, const string& name, const char* varName, glu::DataType varType, bool readByComponent)
	: TestCase			(context, name + (readByComponent ? "_component" : ""), varName)
	, m_varName			(varName)
	, m_varType			(varType)
	, m_subCaseNdx		(0)
	, m_readByComponent	(readByComponent)
{
}

ComputeBuiltinVarCase::~ComputeBuiltinVarCase (void)
{
	ComputeBuiltinVarCase::deinit();
}

void ComputeBuiltinVarCase::initPrograms (SourceCollections& programCollection) const
{
	for (std::size_t i = 0; i < m_subCases.size(); i++)
	{
		const SubCase&	subCase = m_subCases[i];
		std::ostringstream name;
		name << s_prefixProgramName << i;
		programCollection.glslSources.add(name.str()) << glu::ComputeSource(genBuiltinVarSource(m_varName, m_varType, subCase.localSize(), m_readByComponent).c_str());
	}
}

string ComputeBuiltinVarCase::genBuiltinVarSource (const string& varName, glu::DataType varType, const UVec3& localSize, bool readByComponent) const
{
	std::ostringstream src;

	src << "#version 310 es\n"
		<< "layout (local_size_x = " << localSize.x() << ", local_size_y = " << localSize.y() << ", local_size_z = " << localSize.z() << ") in;\n";

	// For the gl_WorkGroupSize case, force it to be specialized so that
	// Glslang can't just bypass the read of the builtin variable.
	// We will not override these spec constants.
	src << "layout (local_size_x_id = 0, local_size_y_id = 1, local_size_z_id = 2) in;\n";

	src << "layout(set = 0, binding = 0) uniform Stride\n"
		<< "{\n"
		<< "	uvec2 u_stride;\n"
		<< "}stride;\n"
		<< "layout(set = 0, binding = 1, std430) buffer Output\n"
		<< "{\n"
		<< "	" << glu::getDataTypeName(varType) << " result[];\n"
		<< "} sb_out;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	highp uint offset = stride.u_stride.x*gl_GlobalInvocationID.z + stride.u_stride.y*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n";

	if (readByComponent && varType != glu::TYPE_UINT) {
		switch(varType)
		{
			case glu::TYPE_UINT_VEC4:
				src << "	sb_out.result[offset].w = " << varName << ".w;\n";
				// Fall through
			case glu::TYPE_UINT_VEC3:
				src << "	sb_out.result[offset].z = " << varName << ".z;\n";
				// Fall through
			case glu::TYPE_UINT_VEC2:
				src << "	sb_out.result[offset].y = " << varName << ".y;\n"
					<< "	sb_out.result[offset].x = " << varName << ".x;\n";
				break;
			default:
				DE_ASSERT("Illegal data type");
				break;
		}
	} else {
		src << "	sb_out.result[offset] = " << varName << ";\n";
	}
	src << "}\n";

	return src.str();
}

class NumWorkGroupsCase : public ComputeBuiltinVarCase
{
public:
	NumWorkGroupsCase (tcu::TestContext& context, bool readByCompnent)
		: ComputeBuiltinVarCase(context, "num_work_groups", "gl_NumWorkGroups", glu::TYPE_UINT_VEC3, readByCompnent)
	{
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(52, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 39, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 78)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(4, 7, 11)));
		m_subCases.push_back(SubCase(UVec3(2, 3, 4), UVec3(4, 7, 11)));
	}

	UVec3 computeReference (const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const
	{
		DE_UNREF(numWorkGroups);
		DE_UNREF(workGroupSize);
		DE_UNREF(workGroupID);
		DE_UNREF(localInvocationID);
		return numWorkGroups;
	}
};

class WorkGroupSizeCase : public ComputeBuiltinVarCase
{
public:
	WorkGroupSizeCase (tcu::TestContext& context, bool readByComponent)
		: ComputeBuiltinVarCase(context, "work_group_size", "gl_WorkGroupSize", glu::TYPE_UINT_VEC3, readByComponent)
	{
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(2, 7, 3)));
		m_subCases.push_back(SubCase(UVec3(2, 1, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(2, 1, 1), UVec3(1, 3, 5)));
		m_subCases.push_back(SubCase(UVec3(1, 3, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 7), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 7), UVec3(3, 3, 1)));
		m_subCases.push_back(SubCase(UVec3(10, 3, 4), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(10, 3, 4), UVec3(3, 1, 2)));
	}

	UVec3 computeReference (const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const
	{
		DE_UNREF(numWorkGroups);
		DE_UNREF(workGroupID);
		DE_UNREF(localInvocationID);
		return workGroupSize;
	}
};

//-----------------------------------------------------------------------
class WorkGroupIDCase : public ComputeBuiltinVarCase
{
public:
	WorkGroupIDCase (tcu::TestContext& context, bool readbyComponent)
		: ComputeBuiltinVarCase(context, "work_group_id", "gl_WorkGroupID", glu::TYPE_UINT_VEC3, readbyComponent)
	{
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(52, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 39, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 78)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(4, 7, 11)));
		m_subCases.push_back(SubCase(UVec3(2, 3, 4), UVec3(4, 7, 11)));
	}

	UVec3 computeReference (const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const
	{
		DE_UNREF(numWorkGroups);
		DE_UNREF(workGroupSize);
		DE_UNREF(localInvocationID);
		return workGroupID;
	}
};

class LocalInvocationIDCase : public ComputeBuiltinVarCase
{
public:
	LocalInvocationIDCase (tcu::TestContext& context, bool readByComponent)
		: ComputeBuiltinVarCase(context, "local_invocation_id", "gl_LocalInvocationID", glu::TYPE_UINT_VEC3, readByComponent)
	{
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(2, 7, 3)));
		m_subCases.push_back(SubCase(UVec3(2, 1, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(2, 1, 1), UVec3(1, 3, 5)));
		m_subCases.push_back(SubCase(UVec3(1, 3, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 7), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 7), UVec3(3, 3, 1)));
		m_subCases.push_back(SubCase(UVec3(10, 3, 4), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(10, 3, 4), UVec3(3, 1, 2)));
	}

	UVec3 computeReference (const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const
	{
		DE_UNREF(numWorkGroups);
		DE_UNREF(workGroupSize);
		DE_UNREF(workGroupID);
		return localInvocationID;
	}
};

class GlobalInvocationIDCase : public ComputeBuiltinVarCase
{
public:
	GlobalInvocationIDCase (tcu::TestContext& context, bool readByComponent)
		: ComputeBuiltinVarCase(context, "global_invocation_id", "gl_GlobalInvocationID", glu::TYPE_UINT_VEC3, readByComponent)
	{
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(52, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 39, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 78)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(4, 7, 11)));
		m_subCases.push_back(SubCase(UVec3(2, 3, 4), UVec3(4, 7, 11)));
		m_subCases.push_back(SubCase(UVec3(10, 3, 4), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(10, 3, 4), UVec3(3, 1, 2)));
	}

	UVec3 computeReference (const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const
	{
		DE_UNREF(numWorkGroups);
		return workGroupID * workGroupSize + localInvocationID;
	}
};

class LocalInvocationIndexCase : public ComputeBuiltinVarCase
{
public:
	LocalInvocationIndexCase (tcu::TestContext& context, bool readByComponent)
		: ComputeBuiltinVarCase(context, "local_invocation_index", "gl_LocalInvocationIndex", glu::TYPE_UINT, readByComponent)
	{
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(1, 39, 1)));
		m_subCases.push_back(SubCase(UVec3(1, 1, 1), UVec3(4, 7, 11)));
		m_subCases.push_back(SubCase(UVec3(2, 3, 4), UVec3(4, 7, 11)));
		m_subCases.push_back(SubCase(UVec3(10, 3, 4), UVec3(1, 1, 1)));
		m_subCases.push_back(SubCase(UVec3(10, 3, 4), UVec3(3, 1, 2)));
	}

	UVec3 computeReference (const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const
	{
		DE_UNREF(workGroupID);
		DE_UNREF(numWorkGroups);
		return UVec3(localInvocationID.z()*workGroupSize.x()*workGroupSize.y() + localInvocationID.y()*workGroupSize.x() + localInvocationID.x(), 0, 0);
	}
};

ComputeBuiltinVarInstance::ComputeBuiltinVarInstance (Context&						context,
													  const vector<SubCase>&		subCases,
													  const glu::DataType			varType,
													  const ComputeBuiltinVarCase*	builtinVarCase)
	: vkt::TestInstance		(context)
	, m_device				(m_context.getDevice())
	, m_vki					(m_context.getDeviceInterface())
	, m_queue				(context.getUniversalQueue())
	, m_queueFamilyIndex	(context.getUniversalQueueFamilyIndex())
	, m_subCases			(subCases)
	, m_builtin_var_case	(builtinVarCase)
	, m_subCaseNdx			(0)
	, m_varType				(varType)
{
}

tcu::TestStatus	ComputeBuiltinVarInstance::iterate (void)
{
	std::ostringstream program_name;
	program_name << s_prefixProgramName << m_subCaseNdx;

	const SubCase&				subCase				= m_subCases[m_subCaseNdx];
	const tcu::UVec3			globalSize			= subCase.localSize()*subCase.numWorkGroups();
	const tcu::UVec2			stride				(globalSize[0] * globalSize[1], globalSize[0]);
	const deUint32				sizeOfUniformBuffer	= sizeof(stride);
	const int					numScalars			= glu::getDataTypeScalarSize(m_varType);
	const deUint32				numInvocations		= subCase.localSize()[0] * subCase.localSize()[1] * subCase.localSize()[2] * subCase.numWorkGroups()[0] * subCase.numWorkGroups()[1] * subCase.numWorkGroups()[2];

	deUint32					resultBufferStride = 0;
	switch (m_varType)
	{
		case glu::TYPE_UINT:
			resultBufferStride = sizeof(deUint32);
			break;
		case glu::TYPE_UINT_VEC2:
			resultBufferStride = sizeof(tcu::UVec2);
			break;
		case glu::TYPE_UINT_VEC3:
		case glu::TYPE_UINT_VEC4:
			resultBufferStride = sizeof(tcu::UVec4);
			break;
		default:
			DE_ASSERT("Illegal data type");
	}

	const deUint32				resultBufferSize	= numInvocations * resultBufferStride;

	// Create result buffer
	Buffer uniformBuffer(m_vki, m_device, m_context.getDefaultAllocator(), makeBufferCreateInfo(sizeOfUniformBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT), MemoryRequirement::HostVisible);
	Buffer resultBuffer(m_vki, m_device, m_context.getDefaultAllocator(), makeBufferCreateInfo(resultBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	{
		const Allocation& alloc = uniformBuffer.getAllocation();
		memcpy(alloc.getHostPtr(), &stride, sizeOfUniformBuffer);
		flushMappedMemoryRange(m_vki, m_device, alloc.getMemory(), alloc.getOffset(), sizeOfUniformBuffer);
	}

	// Create descriptorSetLayout
	const Unique<VkDescriptorSetLayout>	descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(m_vki, m_device));

	const Unique<VkShaderModule> shaderModule(createShaderModule(m_vki, m_device, m_context.getBinaryCollection().get(program_name.str()), 0u));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(m_vki, m_device, *descriptorSetLayout));
	const Unique<VkPipeline> pipeline(makeComputePipeline(m_vki, m_device, *pipelineLayout, *shaderModule));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(m_vki, m_device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const VkBufferMemoryBarrier bufferBarrier = makeBufferMemoryBarrier(
		VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *resultBuffer, 0ull, resultBufferSize);

	const Unique<VkCommandPool> cmdPool(makeCommandPool(m_vki, m_device, m_queueFamilyIndex));
	const Unique<VkCommandBuffer> cmdBuffer(allocateCommandBuffer(m_vki, m_device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Start recording commands
	beginCommandBuffer(m_vki, *cmdBuffer);

	m_vki.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);

	// Create descriptor set
	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(m_vki, m_device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo resultDescriptorInfo = makeDescriptorBufferInfo(*resultBuffer, 0ull, resultBufferSize);
	const VkDescriptorBufferInfo uniformDescriptorInfo = makeDescriptorBufferInfo(*uniformBuffer, 0ull, sizeOfUniformBuffer);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uniformDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultDescriptorInfo)
		.update(m_vki, m_device);

	m_vki.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	// Dispatch indirect compute command
	m_vki.cmdDispatch(*cmdBuffer, subCase.numWorkGroups()[0], subCase.numWorkGroups()[1], subCase.numWorkGroups()[2]);

	m_vki.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0,
							 0, (const VkMemoryBarrier*)DE_NULL,
							 1, &bufferBarrier,
							 0, (const VkImageMemoryBarrier*)DE_NULL);

	// End recording commands
	endCommandBuffer(m_vki, *cmdBuffer);

	// Wait for command buffer execution finish
	submitCommandsAndWait(m_vki, m_device, m_queue, *cmdBuffer);

	const Allocation& resultAlloc = resultBuffer.getAllocation();
	invalidateMappedMemoryRange(m_vki, m_device, resultAlloc.getMemory(), resultAlloc.getOffset(), resultBufferSize);

	const deUint8*	 ptr = reinterpret_cast<deUint8*>(resultAlloc.getHostPtr());

	int			numFailed		= 0;
	const int	maxLogPrints	= 10;

	tcu::TestContext& testCtx	= m_context.getTestContext();

	for (deUint32 groupZ = 0; groupZ < subCase.numWorkGroups().z(); groupZ++)
	for (deUint32 groupY = 0; groupY < subCase.numWorkGroups().y(); groupY++)
	for (deUint32 groupX = 0; groupX < subCase.numWorkGroups().x(); groupX++)
	for (deUint32 localZ = 0; localZ < subCase.localSize().z(); localZ++)
	for (deUint32 localY = 0; localY < subCase.localSize().y(); localY++)
	for (deUint32 localX = 0; localX < subCase.localSize().x(); localX++)
	{
		const UVec3			refGroupID(groupX, groupY, groupZ);
		const UVec3			refLocalID(localX, localY, localZ);
		const UVec3			refGlobalID = refGroupID * subCase.localSize() + refLocalID;

		const deUint32		refOffset = stride.x()*refGlobalID.z() + stride.y()*refGlobalID.y() + refGlobalID.x();

		const UVec3			refValue = m_builtin_var_case->computeReference(subCase.numWorkGroups(), subCase.localSize(), refGroupID, refLocalID);

		const deUint32*		resPtr = (const deUint32*)(ptr + refOffset * resultBufferStride);
		const UVec3			resValue = readResultVec(resPtr, numScalars);

		if (!compareNumComponents(refValue, resValue, numScalars))
		{
			if (numFailed < maxLogPrints)
				testCtx.getLog()
				<< TestLog::Message
				<< "ERROR: comparison failed at offset " << refOffset
				<< ": expected " << LogComps(refValue, numScalars)
				<< ", got " << LogComps(resValue, numScalars)
				<< TestLog::EndMessage;
			else if (numFailed == maxLogPrints)
				testCtx.getLog() << TestLog::Message << "..." << TestLog::EndMessage;

			numFailed += 1;
		}
	}

	testCtx.getLog() << TestLog::Message << (numInvocations - numFailed) << " / " << numInvocations << " values passed" << TestLog::EndMessage;

	if (numFailed > 0)
		return tcu::TestStatus::fail("Comparison failed");

	m_subCaseNdx += 1;
	return (m_subCaseNdx < (int)m_subCases.size()) ? tcu::TestStatus::incomplete() :tcu::TestStatus::pass("Comparison succeeded");
}

class ComputeShaderBuiltinVarTests : public tcu::TestCaseGroup
{
public:
			ComputeShaderBuiltinVarTests	(tcu::TestContext& context);

	void	init							(void);

private:
	ComputeShaderBuiltinVarTests (const ComputeShaderBuiltinVarTests& other);
	ComputeShaderBuiltinVarTests& operator= (const ComputeShaderBuiltinVarTests& other);
};

ComputeShaderBuiltinVarTests::ComputeShaderBuiltinVarTests (tcu::TestContext& context)
	: TestCaseGroup(context, "builtin_var", "Shader builtin var tests")
{
}

void ComputeShaderBuiltinVarTests::init (void)
{
	// Builtin variables with vector values should be read whole and by component.
	for (int i = 0; i < 2; i++)
	{
		const bool readByComponent = (i != 0);
		addChild(new NumWorkGroupsCase(this->getTestContext(), readByComponent));
		addChild(new WorkGroupSizeCase(this->getTestContext(), readByComponent));
		addChild(new WorkGroupIDCase(this->getTestContext(), readByComponent));
		addChild(new LocalInvocationIDCase(this->getTestContext(), readByComponent));
		addChild(new GlobalInvocationIDCase(this->getTestContext(), readByComponent));
	}
	// Local invocation index is already just a scalar.
	addChild(new LocalInvocationIndexCase(this->getTestContext(), false));
}

} // anonymous

tcu::TestCaseGroup* createComputeShaderBuiltinVarTests (tcu::TestContext& testCtx)
{
	return new ComputeShaderBuiltinVarTests(testCtx);
}

} // compute
} // vkt
