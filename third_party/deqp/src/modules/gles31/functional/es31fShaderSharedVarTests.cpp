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
 * \brief GLSL Shared variable tests.
 *//*--------------------------------------------------------------------*/

#include "es31fShaderSharedVarTests.hpp"
#include "es31fShaderAtomicOpTests.hpp"
#include "gluShaderProgram.hpp"
#include "gluShaderUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluObjectWrapper.hpp"
#include "gluProgramInterfaceQuery.hpp"
#include "tcuVector.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuFormatUtil.hpp"
#include "deRandom.hpp"
#include "deArrayUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include <algorithm>
#include <set>

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::string;
using std::vector;
using tcu::TestLog;
using tcu::UVec3;
using std::set;
using namespace glu;

enum
{
	MAX_VALUE_ARRAY_LENGTH	= 15	// * 2 * sizeof(mat4) + sizeof(int) = 481 uniform components (limit 512)
};

template<typename T, int Size>
static inline T product (const tcu::Vector<T, Size>& v)
{
	T res = v[0];
	for (int ndx = 1; ndx < Size; ndx++)
		res *= v[ndx];
	return res;
}

class SharedBasicVarCase : public TestCase
{
public:
							SharedBasicVarCase		(Context& context, const char* name, DataType basicType, Precision precision, const tcu::UVec3& workGroupSize);
							~SharedBasicVarCase		(void);

	void					init					(void);
	void					deinit					(void);
	IterateResult			iterate					(void);

private:
							SharedBasicVarCase		(const SharedBasicVarCase& other);
	SharedBasicVarCase&		operator=				(const SharedBasicVarCase& other);

	const DataType			m_basicType;
	const Precision			m_precision;
	const tcu::UVec3		m_workGroupSize;

	ShaderProgram*			m_program;
};

static std::string getBasicCaseDescription (DataType basicType, Precision precision, const tcu::UVec3& workGroupSize)
{
	std::ostringstream str;
	if (precision != PRECISION_LAST)
		str << getPrecisionName(precision) << " ";
	str << getDataTypeName(basicType) << ", work group size = " << workGroupSize;
	return str.str();
}

SharedBasicVarCase::SharedBasicVarCase (Context& context, const char* name, DataType basicType, Precision precision, const tcu::UVec3& workGroupSize)
	: TestCase			(context, name, getBasicCaseDescription(basicType, precision, workGroupSize).c_str())
	, m_basicType		(basicType)
	, m_precision		(precision)
	, m_workGroupSize	(workGroupSize)
	, m_program			(DE_NULL)
{
}

SharedBasicVarCase::~SharedBasicVarCase (void)
{
	SharedBasicVarCase::deinit();
}

void SharedBasicVarCase::init (void)
{
	const int			valArrayLength	= de::min<int>(MAX_VALUE_ARRAY_LENGTH, product(m_workGroupSize));
	const char*			precName		= m_precision != glu::PRECISION_LAST ? getPrecisionName(m_precision) : "";
	const char*			typeName		= getDataTypeName(m_basicType);
	std::ostringstream	src;

	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_workGroupSize[0]
		<< ", local_size_y = " << m_workGroupSize[1]
		<< ", local_size_z = " << m_workGroupSize[2]
		<< ") in;\n"
		<< "const uint LOCAL_SIZE = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "shared " << precName << " " << typeName << " s_var;\n"
		<< "uniform " << precName << " " << typeName << " u_val[" << valArrayLength << "];\n"
		<< "uniform " << precName << " " << typeName << " u_ref[" << valArrayLength << "];\n"
		<< "uniform uint u_numIters;\n"
		<< "layout(binding = 0) buffer Result\n"
		<< "{\n"
		<< "	bool isOk[LOCAL_SIZE];\n"
		<< "};\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	bool allOk = true;\n"
		<< "	for (uint ndx = 0u; ndx < u_numIters; ndx++)\n"
		<< "	{\n"
		<< "		if (ndx == gl_LocalInvocationIndex)\n"
		<< "			s_var = u_val[ndx%uint(u_val.length())];\n"
		<< "\n"
		<< "		barrier();\n"
		<< "\n"
		<< "		if (s_var != u_ref[ndx%uint(u_ref.length())])\n"
		<< "			allOk = false;\n"
		<< "\n"
		<< "		barrier();\n"
		<< "	}\n"
		<< "\n"
		<< "	isOk[gl_LocalInvocationIndex] = allOk;\n"
		<< "}\n";

	DE_ASSERT(!m_program);
	m_program = new ShaderProgram(m_context.getRenderContext(), ProgramSources() << ComputeSource(src.str()));

	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
	{
		delete m_program;
		m_program = DE_NULL;
		throw tcu::TestError("Compile failed");
	}
}

void SharedBasicVarCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

SharedBasicVarCase::IterateResult SharedBasicVarCase::iterate (void)
{
	const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
	const deUint32				program			= m_program->getProgram();
	Buffer						outputBuffer	(m_context.getRenderContext());
	const deUint32				outBlockNdx		= gl.getProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "Result");
	const InterfaceBlockInfo	outBlockInfo	= getProgramInterfaceBlockInfo(gl, program, GL_SHADER_STORAGE_BLOCK, outBlockNdx);

	gl.useProgram(program);

	// Setup input values.
	{
		const int		numValues		= (int)product(m_workGroupSize);
		const int		valLoc			= gl.getUniformLocation(program, "u_val[0]");
		const int		refLoc			= gl.getUniformLocation(program, "u_ref[0]");
		const int		iterCountLoc	= gl.getUniformLocation(program, "u_numIters");
		const int		scalarSize		= getDataTypeScalarSize(m_basicType);

		if (isDataTypeFloatOrVec(m_basicType))
		{
			const int		maxInt			= m_precision == glu::PRECISION_LOWP ? 2 : 1024;
			const int		minInt			= -de::min(numValues/2, maxInt);
			vector<float>	values			(numValues*scalarSize);

			for (int ndx = 0; ndx < (int)values.size(); ndx++)
				values[ndx] = float(minInt + (ndx % (maxInt-minInt+1)));

			for (int uNdx = 0; uNdx < 2; uNdx++)
			{
				const int location = uNdx == 1 ? refLoc : valLoc;

				if (scalarSize == 1)		gl.uniform1fv(location, numValues, &values[0]);
				else if (scalarSize == 2)	gl.uniform2fv(location, numValues, &values[0]);
				else if (scalarSize == 3)	gl.uniform3fv(location, numValues, &values[0]);
				else if (scalarSize == 4)	gl.uniform4fv(location, numValues, &values[0]);
			}
		}
		else if (isDataTypeIntOrIVec(m_basicType))
		{
			const int		maxInt			= m_precision == glu::PRECISION_LOWP ? 64 : 1024;
			const int		minInt			= -de::min(numValues/2, maxInt);
			vector<int>		values			(numValues*scalarSize);

			for (int ndx = 0; ndx < (int)values.size(); ndx++)
				values[ndx] = minInt + (ndx % (maxInt-minInt+1));

			for (int uNdx = 0; uNdx < 2; uNdx++)
			{
				const int location = uNdx == 1 ? refLoc : valLoc;

				if (scalarSize == 1)		gl.uniform1iv(location, numValues, &values[0]);
				else if (scalarSize == 2)	gl.uniform2iv(location, numValues, &values[0]);
				else if (scalarSize == 3)	gl.uniform3iv(location, numValues, &values[0]);
				else if (scalarSize == 4)	gl.uniform4iv(location, numValues, &values[0]);
			}
		}
		else if (isDataTypeUintOrUVec(m_basicType))
		{
			const deUint32		maxInt		= m_precision == glu::PRECISION_LOWP ? 128 : 1024;
			vector<deUint32>	values		(numValues*scalarSize);

			for (int ndx = 0; ndx < (int)values.size(); ndx++)
				values[ndx] = ndx % (maxInt+1);

			for (int uNdx = 0; uNdx < 2; uNdx++)
			{
				const int location = uNdx == 1 ? refLoc : valLoc;

				if (scalarSize == 1)		gl.uniform1uiv(location, numValues, &values[0]);
				else if (scalarSize == 2)	gl.uniform2uiv(location, numValues, &values[0]);
				else if (scalarSize == 3)	gl.uniform3uiv(location, numValues, &values[0]);
				else if (scalarSize == 4)	gl.uniform4uiv(location, numValues, &values[0]);
			}
		}
		else if (isDataTypeBoolOrBVec(m_basicType))
		{
			de::Random		rnd				(0x324f);
			vector<int>		values			(numValues*scalarSize);

			for (int ndx = 0; ndx < (int)values.size(); ndx++)
				values[ndx] = rnd.getBool() ? 1 : 0;

			for (int uNdx = 0; uNdx < 2; uNdx++)
			{
				const int location = uNdx == 1 ? refLoc : valLoc;

				if (scalarSize == 1)		gl.uniform1iv(location, numValues, &values[0]);
				else if (scalarSize == 2)	gl.uniform2iv(location, numValues, &values[0]);
				else if (scalarSize == 3)	gl.uniform3iv(location, numValues, &values[0]);
				else if (scalarSize == 4)	gl.uniform4iv(location, numValues, &values[0]);
			}
		}
		else if (isDataTypeMatrix(m_basicType))
		{
			const int		maxInt			= m_precision == glu::PRECISION_LOWP ? 2 : 1024;
			const int		minInt			= -de::min(numValues/2, maxInt);
			vector<float>	values			(numValues*scalarSize);

			for (int ndx = 0; ndx < (int)values.size(); ndx++)
				values[ndx] = float(minInt + (ndx % (maxInt-minInt+1)));

			for (int uNdx = 0; uNdx < 2; uNdx++)
			{
				const int location = uNdx == 1 ? refLoc : valLoc;

				switch (m_basicType)
				{
					case TYPE_FLOAT_MAT2:	gl.uniformMatrix2fv  (location, numValues, DE_FALSE, &values[0]);	break;
					case TYPE_FLOAT_MAT2X3:	gl.uniformMatrix2x3fv(location, numValues, DE_FALSE, &values[0]);	break;
					case TYPE_FLOAT_MAT2X4:	gl.uniformMatrix2x4fv(location, numValues, DE_FALSE, &values[0]);	break;
					case TYPE_FLOAT_MAT3X2:	gl.uniformMatrix3x2fv(location, numValues, DE_FALSE, &values[0]);	break;
					case TYPE_FLOAT_MAT3:	gl.uniformMatrix3fv  (location, numValues, DE_FALSE, &values[0]);	break;
					case TYPE_FLOAT_MAT3X4:	gl.uniformMatrix3x4fv(location, numValues, DE_FALSE, &values[0]);	break;
					case TYPE_FLOAT_MAT4X2:	gl.uniformMatrix4x2fv(location, numValues, DE_FALSE, &values[0]);	break;
					case TYPE_FLOAT_MAT4X3:	gl.uniformMatrix4x3fv(location, numValues, DE_FALSE, &values[0]);	break;
					case TYPE_FLOAT_MAT4:	gl.uniformMatrix4fv  (location, numValues, DE_FALSE, &values[0]);	break;
					default:
						DE_ASSERT(false);
				}
			}
		}

		gl.uniform1ui(iterCountLoc, product(m_workGroupSize));
		GLU_EXPECT_NO_ERROR(gl.getError(), "Input value setup failed");
	}

	// Setup output buffer.
	{
		vector<deUint8> emptyData(outBlockInfo.dataSize);
		std::fill(emptyData.begin(), emptyData.end(), 0);

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, outBlockInfo.dataSize, &emptyData[0], GL_STATIC_READ);
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
	}

	gl.dispatchCompute(1, 1, 1);

	// Read back and compare
	{
		const deUint32				numValues	= product(m_workGroupSize);
		const InterfaceVariableInfo	outVarInfo	= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, outBlockInfo.activeVariables[0]);
		const void*					resPtr		= gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, outBlockInfo.dataSize, GL_MAP_READ_BIT);
		const int					maxErrMsg	= 10;
		int							numFailed	= 0;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange()");
		TCU_CHECK(resPtr);

		for (deUint32 ndx = 0; ndx < numValues; ndx++)
		{
			const int resVal = *((const int*)((const deUint8*)resPtr + outVarInfo.offset + outVarInfo.arrayStride*ndx));

			if (resVal == 0)
			{
				if (numFailed < maxErrMsg)
					m_testCtx.getLog() << TestLog::Message << "ERROR: isOk[" << ndx << "] = " << resVal << " != true" << TestLog::EndMessage;
				else if (numFailed == maxErrMsg)
					m_testCtx.getLog() << TestLog::Message << "..." << TestLog::EndMessage;

				numFailed += 1;
			}
		}

		gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer()");

		m_testCtx.getLog() << TestLog::Message << (numValues-numFailed) << " / " << numValues << " values passed" << TestLog::EndMessage;

		m_testCtx.setTestResult(numFailed == 0 ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
								numFailed == 0 ? "Pass"					: "Comparison failed");
	}

	return STOP;
}

ShaderSharedVarTests::ShaderSharedVarTests (Context& context)
	: TestCaseGroup(context, "shared_var", "Shared Variable Tests")
{
}

ShaderSharedVarTests::~ShaderSharedVarTests (void)
{
}

void ShaderSharedVarTests::init (void)
{
	// .basic_type
	{
		tcu::TestCaseGroup *const basicTypeGroup = new tcu::TestCaseGroup(m_testCtx, "basic_type", "Basic Types");
		addChild(basicTypeGroup);

		for (int basicType = TYPE_FLOAT; basicType <= TYPE_BOOL_VEC4; basicType++)
		{
			if (glu::getDataTypeScalarType(DataType(basicType)) == glu::TYPE_DOUBLE)
				continue;

			if (glu::isDataTypeBoolOrBVec(DataType(basicType)))
			{
				const tcu::UVec3	workGroupSize	(2,1,3);
				basicTypeGroup->addChild(new SharedBasicVarCase(m_context, getDataTypeName(DataType(basicType)), DataType(basicType), PRECISION_LAST, workGroupSize));
			}
			else
			{
				for (int precision = 0; precision < PRECISION_LAST; precision++)
				{
					const tcu::UVec3	workGroupSize	(2,1,3);
					const string		name			= string(getDataTypeName(DataType(basicType))) + "_" + getPrecisionName(Precision(precision));

					basicTypeGroup->addChild(new SharedBasicVarCase(m_context, name.c_str(), DataType(basicType), Precision(precision), workGroupSize));
				}
			}
		}
	}

	// .work_group_size
	{
		tcu::TestCaseGroup *const workGroupSizeGroup = new tcu::TestCaseGroup(m_testCtx, "work_group_size", "Shared Variables with Various Work Group Sizes");
		addChild(workGroupSizeGroup);

		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "float_1_1_1",		TYPE_FLOAT,			PRECISION_HIGHP,	tcu::UVec3(1,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "float_64_1_1",		TYPE_FLOAT,			PRECISION_HIGHP,	tcu::UVec3(64,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "float_1_64_1",		TYPE_FLOAT,			PRECISION_HIGHP,	tcu::UVec3(1,64,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "float_1_1_64",		TYPE_FLOAT,			PRECISION_HIGHP,	tcu::UVec3(1,1,64)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "float_128_1_1",		TYPE_FLOAT,			PRECISION_HIGHP,	tcu::UVec3(128,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "float_1_128_1",		TYPE_FLOAT,			PRECISION_HIGHP,	tcu::UVec3(1,128,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "float_13_2_4",		TYPE_FLOAT,			PRECISION_HIGHP,	tcu::UVec3(13,2,4)));

		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "vec4_1_1_1",		TYPE_FLOAT_VEC4,	PRECISION_HIGHP,	tcu::UVec3(1,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "vec4_64_1_1",		TYPE_FLOAT_VEC4,	PRECISION_HIGHP,	tcu::UVec3(64,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "vec4_1_64_1",		TYPE_FLOAT_VEC4,	PRECISION_HIGHP,	tcu::UVec3(1,64,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "vec4_1_1_64",		TYPE_FLOAT_VEC4,	PRECISION_HIGHP,	tcu::UVec3(1,1,64)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "vec4_128_1_1",		TYPE_FLOAT_VEC4,	PRECISION_HIGHP,	tcu::UVec3(128,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "vec4_1_128_1",		TYPE_FLOAT_VEC4,	PRECISION_HIGHP,	tcu::UVec3(1,128,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "vec4_13_2_4",		TYPE_FLOAT_VEC4,	PRECISION_HIGHP,	tcu::UVec3(13,2,4)));

		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "mat4_1_1_1",		TYPE_FLOAT_MAT4,	PRECISION_HIGHP,	tcu::UVec3(1,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "mat4_64_1_1",		TYPE_FLOAT_MAT4,	PRECISION_HIGHP,	tcu::UVec3(64,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "mat4_1_64_1",		TYPE_FLOAT_MAT4,	PRECISION_HIGHP,	tcu::UVec3(1,64,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "mat4_1_1_64",		TYPE_FLOAT_MAT4,	PRECISION_HIGHP,	tcu::UVec3(1,1,64)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "mat4_128_1_1",		TYPE_FLOAT_MAT4,	PRECISION_HIGHP,	tcu::UVec3(128,1,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "mat4_1_128_1",		TYPE_FLOAT_MAT4,	PRECISION_HIGHP,	tcu::UVec3(1,128,1)));
		workGroupSizeGroup->addChild(new SharedBasicVarCase(m_context, "mat4_13_2_4",		TYPE_FLOAT_MAT4,	PRECISION_HIGHP,	tcu::UVec3(13,2,4)));
	}

	// .atomic
	addChild(new ShaderAtomicOpTests(m_context, "atomic", ATOMIC_OPERAND_SHARED_VARIABLE));
}

} // Functional
} // gles31
} // deqp
