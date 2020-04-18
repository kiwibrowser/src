/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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

#include "es31fComputeShaderBuiltinVarTests.hpp"
#include "gluShaderProgram.hpp"
#include "gluShaderUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluObjectWrapper.hpp"
#include "gluProgramInterfaceQuery.hpp"
#include "tcuVector.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "deSharedPtr.hpp"
#include "deStringUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::string;
using std::vector;
using std::map;
using tcu::TestLog;
using tcu::UVec3;
using tcu::IVec3;

using namespace glu;

template<typename T, int Size>
struct LexicalCompareVec
{
	inline bool operator() (const tcu::Vector<T, Size>& a, const tcu::Vector<T, Size>& b) const
	{
		for (int ndx = 0; ndx < Size; ndx++)
		{
			if (a[ndx] < b[ndx])
				return true;
			else if (a[ndx] > b[ndx])
				return false;
		}
		return false;
	}
};

typedef de::SharedPtr<glu::ShaderProgram>										ShaderProgramSp;
typedef std::map<tcu::UVec3, ShaderProgramSp, LexicalCompareVec<deUint32, 3> >	LocalSizeProgramMap;

class ComputeBuiltinVarCase : public TestCase
{
public:
							ComputeBuiltinVarCase	(Context& context, const char* name, const char* varName, DataType varType);
							~ComputeBuiltinVarCase	(void);

	void					init					(void);
	void					deinit					(void);
	IterateResult			iterate					(void);

	virtual UVec3			computeReference		(const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const = 0;

protected:
	struct SubCase
	{
		UVec3		localSize;
		UVec3		numWorkGroups;

		SubCase (void) {}
		SubCase (const UVec3& localSize_, const UVec3& numWorkGroups_) : localSize(localSize_), numWorkGroups(numWorkGroups_) {}
	};

	vector<SubCase>			m_subCases;

private:
							ComputeBuiltinVarCase	(const ComputeBuiltinVarCase& other);
	ComputeBuiltinVarCase&	operator=				(const ComputeBuiltinVarCase& other);

	deUint32				getProgram				(const UVec3& localSize);

	const string			m_varName;
	const DataType			m_varType;

	LocalSizeProgramMap		m_progMap;
	int						m_subCaseNdx;
};

ComputeBuiltinVarCase::ComputeBuiltinVarCase (Context& context, const char* name, const char* varName, DataType varType)
	: TestCase		(context, name, varName)
	, m_varName		(varName)
	, m_varType		(varType)
	, m_subCaseNdx	(0)
{
}

ComputeBuiltinVarCase::~ComputeBuiltinVarCase (void)
{
	ComputeBuiltinVarCase::deinit();
}

void ComputeBuiltinVarCase::init (void)
{
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	m_subCaseNdx = 0;
}

void ComputeBuiltinVarCase::deinit (void)
{
	m_progMap.clear();
}

static string genBuiltinVarSource (const string& varName, DataType varType, const UVec3& localSize)
{
	std::ostringstream src;

	src << "#version 310 es\n"
		<< "layout (local_size_x = " << localSize.x() << ", local_size_y = " << localSize.y() << ", local_size_z = " << localSize.z() << ") in;\n"
		<< "uniform highp uvec2 u_stride;\n"
		<< "layout(binding = 0) buffer Output\n"
		<< "{\n"
		<< "	" << glu::getDataTypeName(varType) << " result[];\n"
		<< "} sb_out;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	highp uint offset = u_stride.x*gl_GlobalInvocationID.z + u_stride.y*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
		<< "	sb_out.result[offset] = " << varName << ";\n"
		<< "}\n";

	return src.str();
}

deUint32 ComputeBuiltinVarCase::getProgram (const UVec3& localSize)
{
	LocalSizeProgramMap::const_iterator cachePos = m_progMap.find(localSize);
	if (cachePos != m_progMap.end())
		return cachePos->second->getProgram();
	else
	{
		ShaderProgramSp program(new ShaderProgram(m_context.getRenderContext(),
												  ProgramSources() << ComputeSource(genBuiltinVarSource(m_varName, m_varType, localSize))));

		// Log all compiled programs.
		m_testCtx.getLog() << *program;
		if (!program->isOk())
			throw tcu::TestError("Compile failed");

		m_progMap[localSize] = program;
		return program->getProgram();
	}
}

static inline UVec3 readResultVec (const deUint32* ptr, int numComps)
{
	UVec3 res;
	for (int ndx = 0; ndx < numComps; ndx++)
		res[ndx] = ptr[ndx];
	return res;
}

static inline bool compareComps (const UVec3& a, const UVec3& b, int numComps)
{
	DE_ASSERT(numComps == 1 || numComps == 3);
	return numComps == 3 ? tcu::allEqual(a, b) : a.x() == b.x();
}

struct LogComps
{
	const UVec3&	v;
	int				numComps;

	LogComps (const UVec3& v_, int numComps_) : v(v_), numComps(numComps_) {}
};

static inline std::ostream& operator<< (std::ostream& str, const LogComps& c)
{
	DE_ASSERT(c.numComps == 1 || c.numComps == 3);
	return c.numComps == 3 ? str << c.v : str << c.v.x();
}

ComputeBuiltinVarCase::IterateResult ComputeBuiltinVarCase::iterate (void)
{
	const tcu::ScopedLogSection		section			(m_testCtx.getLog(), string("Iteration") + de::toString(m_subCaseNdx), string("Iteration ") + de::toString(m_subCaseNdx));
	const glw::Functions&			gl				= m_context.getRenderContext().getFunctions();
	const SubCase&					subCase			= m_subCases[m_subCaseNdx];
	const deUint32					program			= getProgram(subCase.localSize);

	const tcu::UVec3				globalSize		= subCase.localSize*subCase.numWorkGroups;
	const tcu::UVec2				stride			(globalSize[0]*globalSize[1], globalSize[0]);
	const deUint32					numInvocations	= subCase.localSize[0]*subCase.localSize[1]*subCase.localSize[2]*subCase.numWorkGroups[0]*subCase.numWorkGroups[1]*subCase.numWorkGroups[2];

	const deUint32					outVarIndex		= gl.getProgramResourceIndex(program, GL_BUFFER_VARIABLE, "Output.result");
	const InterfaceVariableInfo		outVarInfo		= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, outVarIndex);
	const deUint32					bufferSize		= numInvocations*outVarInfo.arrayStride;
	Buffer							outputBuffer	(m_context.getRenderContext());

	TCU_CHECK(outVarInfo.arraySize == 0); // Unsized variable.

	m_testCtx.getLog() << TestLog::Message << "Number of work groups = " << subCase.numWorkGroups << TestLog::EndMessage
					   << TestLog::Message << "Work group size = " << subCase.localSize << TestLog::EndMessage;

	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, (glw::GLsizeiptr)bufferSize, DE_NULL, GL_STREAM_READ);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Buffer setup failed");

	gl.useProgram(program);
	gl.uniform2uiv(gl.getUniformLocation(program, "u_stride"), 1, stride.getPtr());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program setup failed");

	gl.dispatchCompute(subCase.numWorkGroups[0], subCase.numWorkGroups[1], subCase.numWorkGroups[2]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute() failed");

	{
		const void*	ptr				= gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, GL_MAP_READ_BIT);
		int			numFailed		= 0;
		const int	numScalars		= getDataTypeScalarSize(m_varType);
		const int	maxLogPrints	= 10;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() failed");
		TCU_CHECK(ptr);

		for (deUint32 groupZ = 0; groupZ < subCase.numWorkGroups.z(); groupZ++)
		for (deUint32 groupY = 0; groupY < subCase.numWorkGroups.y(); groupY++)
		for (deUint32 groupX = 0; groupX < subCase.numWorkGroups.x(); groupX++)
		for (deUint32 localZ = 0; localZ < subCase.localSize.z(); localZ++)
		for (deUint32 localY = 0; localY < subCase.localSize.y(); localY++)
		for (deUint32 localX = 0; localX < subCase.localSize.x(); localX++)
		{
			const UVec3			refGroupID		(groupX, groupY, groupZ);
			const UVec3			refLocalID		(localX, localY, localZ);
			const UVec3			refGlobalID		= refGroupID * subCase.localSize + refLocalID;
			const deUint32		refOffset		= stride.x()*refGlobalID.z() + stride.y()*refGlobalID.y() + refGlobalID.x();
			const UVec3			refValue		= computeReference(subCase.numWorkGroups, subCase.localSize, refGroupID, refLocalID);

			const deUint32*		resPtr			= (const deUint32*)((const deUint8*)ptr + refOffset*outVarInfo.arrayStride);
			const UVec3			resValue		= readResultVec(resPtr, numScalars);

			if (!compareComps(refValue, resValue, numScalars))
			{
				if (numFailed < maxLogPrints)
					m_testCtx.getLog() << TestLog::Message << "ERROR: comparison failed at offset " << refOffset
														   << ": expected " << LogComps(refValue, numScalars)
														   << ", got " << LogComps(resValue, numScalars)
									   << TestLog::EndMessage;
				else if (numFailed == maxLogPrints)
					m_testCtx.getLog() << TestLog::Message << "..." << TestLog::EndMessage;

				numFailed += 1;
			}
		}

		m_testCtx.getLog() << TestLog::Message << (numInvocations-numFailed) << " / " << numInvocations << " values passed" << TestLog::EndMessage;

		if (numFailed > 0)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Comparison failed");

		gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	m_subCaseNdx += 1;
	return (m_subCaseNdx < (int)m_subCases.size() && m_testCtx.getTestResult() == QP_TEST_RESULT_PASS) ? CONTINUE : STOP;
}

// Test cases

class NumWorkGroupsCase : public ComputeBuiltinVarCase
{
public:
	NumWorkGroupsCase (Context& context)
		: ComputeBuiltinVarCase(context, "num_work_groups", "gl_NumWorkGroups", TYPE_UINT_VEC3)
	{
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(52,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,39,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,78)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(4,7,11)));
		m_subCases.push_back(SubCase(UVec3(2,3,4), UVec3(4,7,11)));
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
	WorkGroupSizeCase (Context& context)
		: ComputeBuiltinVarCase(context, "work_group_size", "gl_WorkGroupSize", TYPE_UINT_VEC3)
	{
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(2,7,3)));
		m_subCases.push_back(SubCase(UVec3(2,1,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(2,1,1), UVec3(1,3,5)));
		m_subCases.push_back(SubCase(UVec3(1,3,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,7), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,7), UVec3(3,3,1)));
		m_subCases.push_back(SubCase(UVec3(10,3,4), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(10,3,4), UVec3(3,1,2)));
	}

	UVec3 computeReference (const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const
	{
		DE_UNREF(numWorkGroups);
		DE_UNREF(workGroupID);
		DE_UNREF(localInvocationID);
		return workGroupSize;
	}
};

class WorkGroupIDCase : public ComputeBuiltinVarCase
{
public:
	WorkGroupIDCase (Context& context)
		: ComputeBuiltinVarCase(context, "work_group_id", "gl_WorkGroupID", TYPE_UINT_VEC3)
	{
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(52,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,39,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,78)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(4,7,11)));
		m_subCases.push_back(SubCase(UVec3(2,3,4), UVec3(4,7,11)));
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
	LocalInvocationIDCase (Context& context)
		: ComputeBuiltinVarCase(context, "local_invocation_id", "gl_LocalInvocationID", TYPE_UINT_VEC3)
	{
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(2,7,3)));
		m_subCases.push_back(SubCase(UVec3(2,1,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(2,1,1), UVec3(1,3,5)));
		m_subCases.push_back(SubCase(UVec3(1,3,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,7), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,7), UVec3(3,3,1)));
		m_subCases.push_back(SubCase(UVec3(10,3,4), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(10,3,4), UVec3(3,1,2)));
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
	GlobalInvocationIDCase (Context& context)
		: ComputeBuiltinVarCase(context, "global_invocation_id", "gl_GlobalInvocationID", TYPE_UINT_VEC3)
	{
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(52,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,39,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,78)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(4,7,11)));
		m_subCases.push_back(SubCase(UVec3(2,3,4), UVec3(4,7,11)));
		m_subCases.push_back(SubCase(UVec3(10,3,4), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(10,3,4), UVec3(3,1,2)));
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
	LocalInvocationIndexCase (Context& context)
		: ComputeBuiltinVarCase(context, "local_invocation_index", "gl_LocalInvocationIndex", TYPE_UINT)
	{
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(1,39,1)));
		m_subCases.push_back(SubCase(UVec3(1,1,1), UVec3(4,7,11)));
		m_subCases.push_back(SubCase(UVec3(2,3,4), UVec3(4,7,11)));
		m_subCases.push_back(SubCase(UVec3(10,3,4), UVec3(1,1,1)));
		m_subCases.push_back(SubCase(UVec3(10,3,4), UVec3(3,1,2)));
	}

	UVec3 computeReference (const UVec3& numWorkGroups, const UVec3& workGroupSize, const UVec3& workGroupID, const UVec3& localInvocationID) const
	{
		DE_UNREF(workGroupID);
		DE_UNREF(numWorkGroups);
		return UVec3(localInvocationID.z()*workGroupSize.x()*workGroupSize.y() + localInvocationID.y()*workGroupSize.x() + localInvocationID.x(), 0, 0);
	}
};

ComputeShaderBuiltinVarTests::ComputeShaderBuiltinVarTests (Context& context)
	: TestCaseGroup(context, "compute", "Compute Shader Builtin Variables")
{
}

ComputeShaderBuiltinVarTests::~ComputeShaderBuiltinVarTests (void)
{
}

void ComputeShaderBuiltinVarTests::init (void)
{
	addChild(new NumWorkGroupsCase			(m_context));
	addChild(new WorkGroupSizeCase			(m_context));
	addChild(new WorkGroupIDCase			(m_context));
	addChild(new LocalInvocationIDCase		(m_context));
	addChild(new GlobalInvocationIDCase		(m_context));
	addChild(new LocalInvocationIndexCase	(m_context));
}

} // Functional
} // gles31
} // deqp
