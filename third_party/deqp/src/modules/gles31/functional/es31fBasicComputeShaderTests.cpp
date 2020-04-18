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
 * \brief Basic Compute Shader Tests.
 *//*--------------------------------------------------------------------*/

#include "es31fBasicComputeShaderTests.hpp"
#include "gluShaderProgram.hpp"
#include "gluObjectWrapper.hpp"
#include "gluRenderContext.hpp"
#include "gluProgramInterfaceQuery.hpp"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "tcuTestLog.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deMemory.h"

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::string;
using std::vector;
using tcu::TestLog;
using namespace glu;

//! Utility for mapping buffers.
class BufferMemMap
{
public:
	BufferMemMap (const glw::Functions& gl, deUint32 target, int offset, int size, deUint32 access)
		: m_gl		(gl)
		, m_target	(target)
		, m_ptr		(DE_NULL)
	{
		m_ptr = gl.mapBufferRange(target, offset, size, access);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange()");
		TCU_CHECK(m_ptr);
	}

	~BufferMemMap (void)
	{
		m_gl.unmapBuffer(m_target);
	}

	void*	getPtr		(void) const { return m_ptr; }
	void*	operator*	(void) const { return m_ptr; }

private:
							BufferMemMap			(const BufferMemMap& other);
	BufferMemMap&			operator=				(const BufferMemMap& other);

	const glw::Functions&	m_gl;
	const deUint32			m_target;
	void*					m_ptr;
};

namespace
{

class EmptyComputeShaderCase : public TestCase
{
public:
	EmptyComputeShaderCase (Context& context)
		: TestCase(context, "empty", "Empty shader")
	{
	}

	IterateResult iterate (void)
	{
		const GLSLVersion	glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream	src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = 1) in;\n"
			   "void main (void) {}\n";

		const ShaderProgram program(m_context.getRenderContext(),
			ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		gl.useProgram(program.getProgram());
		gl.dispatchCompute(1, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}
};

class UBOToSSBOInvertCase : public TestCase
{
public:
	UBOToSSBOInvertCase (Context& context, const char* name, const char* description, int numValues, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_numValues	(numValues)
		, m_localSize	(localSize)
		, m_workSize	(workSize)
	{
		DE_ASSERT(m_numValues % (m_workSize[0]*m_workSize[1]*m_workSize[2]*m_localSize[0]*m_localSize[1]*m_localSize[2]) == 0);
	}

	IterateResult iterate (void)
	{
		const GLSLVersion	glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream	src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ", local_size_z = " << m_localSize[2] << ") in;\n"
			<< "uniform Input {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} ub_in;\n"
			<< "layout(binding = 1) buffer Output {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_out;\n"
			<< "void main (void) {\n"
			<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
			<< "    uint numValuesPerInv = uint(ub_in.values.length()) / (size.x*size.y*size.z);\n"
			<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
			<< "    uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "        sb_out.values[offset + ndx] = ~ub_in.values[offset + ndx];\n"
			<< "}\n";

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));
		const Buffer				inputBuffer		(m_context.getRenderContext());
		const Buffer				outputBuffer	(m_context.getRenderContext());
		std::vector<deUint32>		inputValues		(m_numValues);

		// Compute input values.
		{
			de::Random rnd(0x111223f);
			for (int ndx = 0; ndx < (int)inputValues.size(); ndx++)
				inputValues[ndx] = rnd.getUint32();
		}

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Input buffer setup
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_UNIFORM_BLOCK, "Input");
			const InterfaceBlockInfo	blockInfo	= getProgramInterfaceBlockInfo(gl, program.getProgram(), GL_UNIFORM_BLOCK, blockIndex);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_UNIFORM, "Input.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_UNIFORM, valueIndex);

			gl.bindBuffer(GL_UNIFORM_BUFFER, *inputBuffer);
			gl.bufferData(GL_UNIFORM_BUFFER, (glw::GLsizeiptr)blockInfo.dataSize, DE_NULL, GL_STATIC_DRAW);

			{
				const BufferMemMap bufMap(gl, GL_UNIFORM_BUFFER, 0, (int)blockInfo.dataSize, GL_MAP_WRITE_BIT);

				for (deUint32 ndx = 0; ndx < de::min(valueInfo.arraySize, (deUint32)inputValues.size()); ndx++)
					*(deUint32*)((deUint8*)bufMap.getPtr() + valueInfo.offset + ndx*valueInfo.arrayStride) = inputValues[ndx];
			}

			gl.uniformBlockBinding(program.getProgram(), blockIndex, 0);
			gl.bindBufferBase(GL_UNIFORM_BUFFER, 0, *inputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Input buffer setup failed");
		}

		// Output buffer setup
		{
			const deUint32		blockIndex		= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Output.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			TCU_CHECK(valueInfo.arraySize == (deUint32)inputValues.size());
			for (deUint32 ndx = 0; ndx < valueInfo.arraySize; ndx++)
			{
				const deUint32	res		= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*ndx));
				const deUint32	ref		= ~inputValues[ndx];

				if (res != ref)
					throw tcu::TestError(string("Comparison failed for Output.values[") + de::toString(ndx) + "]");
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const int			m_numValues;
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class CopyInvertSSBOCase : public TestCase
{
public:
	CopyInvertSSBOCase (Context& context, const char* name, const char* description, int numValues, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_numValues	(numValues)
		, m_localSize	(localSize)
		, m_workSize	(workSize)
	{
		DE_ASSERT(m_numValues % (m_workSize[0]*m_workSize[1]*m_workSize[2]*m_localSize[0]*m_localSize[1]*m_localSize[2]) == 0);
	}

	IterateResult iterate (void)
	{
		const GLSLVersion	glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream	src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ", local_size_z = " << m_localSize[2] << ") in;\n"
			<< "layout(binding = 0) buffer Input {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_in;\n"
			<< "layout (binding = 1) buffer Output {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_out;\n"
			<< "void main (void) {\n"
			<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
			<< "    uint numValuesPerInv = uint(sb_in.values.length()) / (size.x*size.y*size.z);\n"
			<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
			<< "    uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "        sb_out.values[offset + ndx] = ~sb_in.values[offset + ndx];\n"
			<< "}\n";

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));
		const Buffer				inputBuffer		(m_context.getRenderContext());
		const Buffer				outputBuffer	(m_context.getRenderContext());
		std::vector<deUint32>		inputValues		(m_numValues);

		// Compute input values.
		{
			de::Random rnd(0x124fef);
			for (int ndx = 0; ndx < (int)inputValues.size(); ndx++)
				inputValues[ndx] = rnd.getUint32();
		}

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Input buffer setup
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Input");
			const InterfaceBlockInfo	blockInfo	= getProgramInterfaceBlockInfo(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Input.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *inputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, (glw::GLsizeiptr)blockInfo.dataSize, DE_NULL, GL_STATIC_DRAW);

			TCU_CHECK(valueInfo.arraySize == (deUint32)inputValues.size());

			{
				const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, (int)blockInfo.dataSize, GL_MAP_WRITE_BIT);

				for (deUint32 ndx = 0; ndx < (deUint32)inputValues.size(); ndx++)
					*(deUint32*)((deUint8*)bufMap.getPtr() + valueInfo.offset + ndx*valueInfo.arrayStride) = inputValues[ndx];
			}

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, blockInfo.bufferBinding, *inputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Input buffer setup failed");
		}

		// Output buffer setup
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const InterfaceBlockInfo	blockInfo	= getProgramInterfaceBlockInfo(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockInfo.dataSize, DE_NULL, GL_STREAM_READ);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, blockInfo.bufferBinding, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Output.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			TCU_CHECK(valueInfo.arraySize == (deUint32)inputValues.size());
			for (deUint32 ndx = 0; ndx < valueInfo.arraySize; ndx++)
			{
				const deUint32	res		= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*ndx));
				const deUint32	ref		= ~inputValues[ndx];

				if (res != ref)
					throw tcu::TestError(string("Comparison failed for Output.values[") + de::toString(ndx) + "]");
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const int			m_numValues;
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class InvertSSBOInPlaceCase : public TestCase
{
public:
	InvertSSBOInPlaceCase (Context& context, const char* name, const char* description, int numValues, bool isSized, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_numValues	(numValues)
		, m_isSized		(isSized)
		, m_localSize	(localSize)
		, m_workSize	(workSize)
	{
		DE_ASSERT(m_numValues % (m_workSize[0]*m_workSize[1]*m_workSize[2]*m_localSize[0]*m_localSize[1]*m_localSize[2]) == 0);
	}

	IterateResult iterate (void)
	{
		const GLSLVersion	glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream	src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ", local_size_z = " << m_localSize[2] << ") in;\n"
			<< "layout(binding = 0) buffer InOut {\n"
			<< "    uint values[" << (m_isSized ? de::toString(m_numValues) : string("")) << "];\n"
			<< "} sb_inout;\n"
			<< "void main (void) {\n"
			<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
			<< "    uint numValuesPerInv = uint(sb_inout.values.length()) / (size.x*size.y*size.z);\n"
			<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
			<< "    uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "        sb_inout.values[offset + ndx] = ~sb_inout.values[offset + ndx];\n"
			<< "}\n";

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		const Buffer				outputBuffer	(m_context.getRenderContext());
		const deUint32				valueIndex		= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "InOut.values");
		const InterfaceVariableInfo	valueInfo		= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
		const deUint32				blockSize		= valueInfo.arrayStride*(deUint32)m_numValues;
		std::vector<deUint32>		inputValues		(m_numValues);

		// Compute input values.
		{
			de::Random rnd(0x82ce7f);
			for (int ndx = 0; ndx < (int)inputValues.size(); ndx++)
				inputValues[ndx] = rnd.getUint32();
		}

		TCU_CHECK(valueInfo.arraySize == (deUint32)(m_isSized ? m_numValues : 0));

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Output buffer setup
		{
			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_DRAW);

			{
				const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, (int)blockSize, GL_MAP_WRITE_BIT);

				for (deUint32 ndx = 0; ndx < (deUint32)inputValues.size(); ndx++)
					*(deUint32*)((deUint8*)bufMap.getPtr() + valueInfo.offset + ndx*valueInfo.arrayStride) = inputValues[ndx];
			}

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			for (deUint32 ndx = 0; ndx < (deUint32)inputValues.size(); ndx++)
			{
				const deUint32	res		= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*ndx));
				const deUint32	ref		= ~inputValues[ndx];

				if (res != ref)
					throw tcu::TestError(string("Comparison failed for InOut.values[") + de::toString(ndx) + "]");
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const int			m_numValues;
	const bool			m_isSized;
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class WriteToMultipleSSBOCase : public TestCase
{
public:
	WriteToMultipleSSBOCase (Context& context, const char* name, const char* description, int numValues, bool isSized, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_numValues	(numValues)
		, m_isSized		(isSized)
		, m_localSize	(localSize)
		, m_workSize	(workSize)
	{
		DE_ASSERT(m_numValues % (m_workSize[0]*m_workSize[1]*m_workSize[2]*m_localSize[0]*m_localSize[1]*m_localSize[2]) == 0);
	}

	IterateResult iterate (void)
	{
		const GLSLVersion	glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream	src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ", local_size_z = " << m_localSize[2] << ") in;\n"
			<< "layout(binding = 0) buffer Out0 {\n"
			<< "    uint values[" << (m_isSized ? de::toString(m_numValues) : string("")) << "];\n"
			<< "} sb_out0;\n"
			<< "layout(binding = 1) buffer Out1 {\n"
			<< "    uint values[" << (m_isSized ? de::toString(m_numValues) : string("")) << "];\n"
			<< "} sb_out1;\n"
			<< "void main (void) {\n"
			<< "    uvec3 size      = gl_NumWorkGroups * gl_WorkGroupSize;\n"
			<< "    uint groupNdx   = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
			<< "\n"
			<< "    {\n"
			<< "        uint numValuesPerInv = uint(sb_out0.values.length()) / (size.x*size.y*size.z);\n"
			<< "        uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "        for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "            sb_out0.values[offset + ndx] = offset + ndx;\n"
			<< "    }\n"
			<< "    {\n"
			<< "        uint numValuesPerInv = uint(sb_out1.values.length()) / (size.x*size.y*size.z);\n"
			<< "        uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "        for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "            sb_out1.values[offset + ndx] = uint(sb_out1.values.length()) - offset - ndx;\n"
			<< "    }\n"
			<< "}\n";

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		const Buffer				outputBuffer0	(m_context.getRenderContext());
		const deUint32				value0Index		= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Out0.values");
		const InterfaceVariableInfo	value0Info		= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, value0Index);
		const deUint32				block0Size		= value0Info.arrayStride*(deUint32)m_numValues;

		const Buffer				outputBuffer1	(m_context.getRenderContext());
		const deUint32				value1Index		= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Out1.values");
		const InterfaceVariableInfo	value1Info		= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, value1Index);
		const deUint32				block1Size		= value1Info.arrayStride*(deUint32)m_numValues;

		TCU_CHECK(value0Info.arraySize == (deUint32)(m_isSized ? m_numValues : 0));
		TCU_CHECK(value1Info.arraySize == (deUint32)(m_isSized ? m_numValues : 0));

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Output buffer setup
		{
			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer0);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, block0Size, DE_NULL, GL_STREAM_DRAW);

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Buffer setup failed");
		}
		{
			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer1);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, block1Size, DE_NULL, GL_STREAM_DRAW);

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *outputBuffer1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer0);
		{
			const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, block0Size, GL_MAP_READ_BIT);

			for (deUint32 ndx = 0; ndx < (deUint32)m_numValues; ndx++)
			{
				const deUint32	res		= *((const deUint32*)((const deUint8*)bufMap.getPtr() + value0Info.offset + value0Info.arrayStride*ndx));
				const deUint32	ref		= ndx;

				if (res != ref)
					throw tcu::TestError(string("Comparison failed for Out0.values[") + de::toString(ndx) + "] res=" + de::toString(res) + " ref=" + de::toString(ref));
			}
		}
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer1);
		{
			const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, block1Size, GL_MAP_READ_BIT);

			for (deUint32 ndx = 0; ndx < (deUint32)m_numValues; ndx++)
			{
				const deUint32	res		= *((const deUint32*)((const deUint8*)bufMap.getPtr() + value1Info.offset + value1Info.arrayStride*ndx));
				const deUint32	ref		= m_numValues - ndx;

				if (res != ref)
					throw tcu::TestError(string("Comparison failed for Out1.values[") + de::toString(ndx) + "] res=" + de::toString(res) + " ref=" + de::toString(ref));
			}
		}
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const int			m_numValues;
	const bool			m_isSized;
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class SSBOLocalBarrierCase : public TestCase
{
public:
	SSBOLocalBarrierCase (Context& context, const char* name, const char* description, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_localSize	(localSize)
		, m_workSize	(workSize)
	{
	}

	IterateResult iterate (void)
	{
		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Buffer				outputBuffer	(m_context.getRenderContext());
		const int					workGroupSize	= m_localSize[0]*m_localSize[1]*m_localSize[2];
		const int					workGroupCount	= m_workSize[0]*m_workSize[1]*m_workSize[2];
		const int					numValues		= workGroupSize*workGroupCount;

		const GLSLVersion			glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream			src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ", local_size_z = " << m_localSize[2] << ") in;\n"
			<< "layout(binding = 0) buffer Output {\n"
			<< "    coherent uint values[" << numValues << "];\n"
			<< "} sb_out;\n\n"
			<< "shared uint offsets[" << workGroupSize << "];\n\n"
			<< "void main (void) {\n"
			<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
			<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
			<< "    uint globalOffs = localSize*globalNdx;\n"
			<< "    uint localOffs  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_LocalInvocationID.z + gl_WorkGroupSize.x*gl_LocalInvocationID.y + gl_LocalInvocationID.x;\n"
			<< "\n"
			<< "    sb_out.values[globalOffs + localOffs] = globalOffs;\n"
			<< "    memoryBarrierBuffer();\n"
			<< "    barrier();\n"
			<< "    sb_out.values[globalOffs + ((localOffs+1u)%localSize)] += localOffs;\n"
			<< "    memoryBarrierBuffer();\n"
			<< "    barrier();\n"
			<< "    sb_out.values[globalOffs + ((localOffs+2u)%localSize)] += localOffs;\n"
			<< "}\n";

		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ComputeSource(src.str()));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Output buffer setup
		{
			const deUint32		blockIndex		= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Output.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			for (int groupNdx = 0; groupNdx < workGroupCount; groupNdx++)
			{
				for (int localOffs = 0; localOffs < workGroupSize; localOffs++)
				{
					const int		globalOffs	= groupNdx*workGroupSize;
					const deUint32	res			= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*(globalOffs + localOffs)));
					const int		offs0		= localOffs-1 < 0 ? ((localOffs+workGroupSize-1)%workGroupSize) : ((localOffs-1)%workGroupSize);
					const int		offs1		= localOffs-2 < 0 ? ((localOffs+workGroupSize-2)%workGroupSize) : ((localOffs-2)%workGroupSize);
					const deUint32	ref			= (deUint32)(globalOffs + offs0 + offs1);

					if (res != ref)
						throw tcu::TestError(string("Comparison failed for Output.values[") + de::toString(globalOffs + localOffs) + "]");
				}
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class SSBOBarrierCase : public TestCase
{
public:
	SSBOBarrierCase (Context& context, const char* name, const char* description, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_workSize	(workSize)
	{
	}

	IterateResult iterate (void)
	{
		const GLSLVersion	glslVersion				= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		const char* const	glslVersionDeclaration	= getGLSLVersionDeclaration(glslVersion);

		std::ostringstream src0;
		src0 << glslVersionDeclaration << "\n"
			 << "layout (local_size_x = 1) in;\n"
						  "uniform uint u_baseVal;\n"
						  "layout(binding = 1) buffer Output {\n"
						  "    uint values[];\n"
						  "};\n"
						  "void main (void) {\n"
						  "    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
						  "    values[offset] = u_baseVal+offset;\n"
				"}\n";

		std::ostringstream src1;
		src1 << glslVersionDeclaration << "\n"
			 << "layout (local_size_x = 1) in;\n"
						  "uniform uint u_baseVal;\n"
						  "layout(binding = 1) buffer Input {\n"
						  "    uint values[];\n"
						  "};\n"
						  "layout(binding = 0) buffer Output {\n"
						  "    coherent uint sum;\n"
						  "};\n"
						  "void main (void) {\n"
						  "    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
						  "    uint value  = values[offset];\n"
						  "    atomicAdd(sum, value);\n"
				"}\n";

		const ShaderProgram			program0		(m_context.getRenderContext(), ProgramSources() << ComputeSource(src0.str()));
		const ShaderProgram			program1		(m_context.getRenderContext(), ProgramSources() << ComputeSource(src1.str()));

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Buffer				tempBuffer		(m_context.getRenderContext());
		const Buffer				outputBuffer	(m_context.getRenderContext());
		const deUint32				baseValue		= 127;

		m_testCtx.getLog() << program0 << program1;
		if (!program0.isOk() || !program1.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		// Temp buffer setup
		{
			const deUint32				valueIndex		= gl.getProgramResourceIndex(program0.getProgram(), GL_BUFFER_VARIABLE, "values[0]");
			const InterfaceVariableInfo	valueInfo		= getProgramInterfaceVariableInfo(gl, program0.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const deUint32				bufferSize		= valueInfo.arrayStride*m_workSize[0]*m_workSize[1]*m_workSize[2];

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *tempBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, (glw::GLsizeiptr)bufferSize, DE_NULL, GL_STATIC_DRAW);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *tempBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Temp buffer setup failed");
		}

		// Output buffer setup
		{
			const deUint32		blockIndex		= gl.getProgramResourceIndex(program1.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= getProgramResourceInt(gl, program1.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);

			{
				const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_WRITE_BIT);
				deMemset(bufMap.getPtr(), 0, blockSize);
			}

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.useProgram(program0.getProgram());
		gl.uniform1ui(gl.getUniformLocation(program0.getProgram(), "u_baseVal"), baseValue);
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		gl.memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		gl.useProgram(program1.getProgram());
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to dispatch commands");

		// Read back and compare
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program1.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program1.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program1.getProgram(), GL_BUFFER_VARIABLE, "sum");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program1.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			const deUint32				res			= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset));
			deUint32					ref			= 0;

			for (int ndx = 0; ndx < m_workSize[0]*m_workSize[1]*m_workSize[2]; ndx++)
				ref += baseValue + (deUint32)ndx;

			if (res != ref)
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: comparison failed, expected " << ref << ", got " << res << TestLog::EndMessage;
				throw tcu::TestError("Comparison failed");
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const tcu::IVec3	m_workSize;
};

class BasicSharedVarCase : public TestCase
{
public:
	BasicSharedVarCase (Context& context, const char* name, const char* description, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_localSize	(localSize)
		, m_workSize	(workSize)
	{
	}

	IterateResult iterate (void)
	{
		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Buffer				outputBuffer	(m_context.getRenderContext());
		const int					workGroupSize	= m_localSize[0]*m_localSize[1]*m_localSize[2];
		const int					workGroupCount	= m_workSize[0]*m_workSize[1]*m_workSize[2];
		const int					numValues		= workGroupSize*workGroupCount;

		const GLSLVersion			glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream			src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ", local_size_z = " << m_localSize[2] << ") in;\n"
			<< "layout(binding = 0) buffer Output {\n"
			<< "    uint values[" << numValues << "];\n"
			<< "} sb_out;\n\n"
			<< "shared uint offsets[" << workGroupSize << "];\n\n"
			<< "void main (void) {\n"
			<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
			<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
			<< "    uint globalOffs = localSize*globalNdx;\n"
			<< "    uint localOffs  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_LocalInvocationID.z + gl_WorkGroupSize.x*gl_LocalInvocationID.y + gl_LocalInvocationID.x;\n"
			<< "\n"
			<< "    offsets[localSize-localOffs-1u] = globalOffs + localOffs*localOffs;\n"
			<< "    barrier();\n"
			<< "    sb_out.values[globalOffs + localOffs] = offsets[localOffs];\n"
			<< "}\n";

		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Output buffer setup
		{
			const deUint32		blockIndex		= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Output.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			for (int groupNdx = 0; groupNdx < workGroupCount; groupNdx++)
			{
				for (int localOffs = 0; localOffs < workGroupSize; localOffs++)
				{
					const int		globalOffs	= groupNdx*workGroupSize;
					const deUint32	res			= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*(globalOffs + localOffs)));
					const deUint32	ref			= (deUint32)(globalOffs + (workGroupSize-localOffs-1)*(workGroupSize-localOffs-1));

					if (res != ref)
						throw tcu::TestError(string("Comparison failed for Output.values[") + de::toString(globalOffs + localOffs) + "]");
				}
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class SharedVarAtomicOpCase : public TestCase
{
public:
	SharedVarAtomicOpCase (Context& context, const char* name, const char* description, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_localSize	(localSize)
		, m_workSize	(workSize)
	{
	}

	IterateResult iterate (void)
	{
		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Buffer				outputBuffer	(m_context.getRenderContext());
		const int					workGroupSize	= m_localSize[0]*m_localSize[1]*m_localSize[2];
		const int					workGroupCount	= m_workSize[0]*m_workSize[1]*m_workSize[2];
		const int					numValues		= workGroupSize*workGroupCount;

		const GLSLVersion			glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream			src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ", local_size_z = " << m_localSize[2] << ") in;\n"
			<< "layout(binding = 0) buffer Output {\n"
			<< "    uint values[" << numValues << "];\n"
			<< "} sb_out;\n\n"
			<< "shared uint count;\n\n"
			<< "void main (void) {\n"
			<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
			<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
			<< "    uint globalOffs = localSize*globalNdx;\n"
			<< "\n"
			<< "    count = 0u;\n"
			<< "    barrier();\n"
			<< "    uint oldVal = atomicAdd(count, 1u);\n"
			<< "    sb_out.values[globalOffs+oldVal] = oldVal+1u;\n"
			<< "}\n";

		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Output buffer setup
		{
			const deUint32		blockIndex		= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Output.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			for (int groupNdx = 0; groupNdx < workGroupCount; groupNdx++)
			{
				for (int localOffs = 0; localOffs < workGroupSize; localOffs++)
				{
					const int		globalOffs	= groupNdx*workGroupSize;
					const deUint32	res			= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*(globalOffs + localOffs)));
					const deUint32	ref			= (deUint32)(localOffs+1);

					if (res != ref)
						throw tcu::TestError(string("Comparison failed for Output.values[") + de::toString(globalOffs + localOffs) + "]");
				}
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

class CopyImageToSSBOCase : public TestCase
{
public:
	CopyImageToSSBOCase (Context& context, const char* name, const char* description, const tcu::IVec2& localSize, const tcu::IVec2& imageSize)
		: TestCase		(context, name, description)
		, m_localSize	(localSize)
		, m_imageSize	(imageSize)
	{
		DE_ASSERT(m_imageSize[0] % m_localSize[0] == 0);
		DE_ASSERT(m_imageSize[1] % m_localSize[1] == 0);
	}

	IterateResult iterate (void)
	{
		const GLSLVersion			glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream			src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ") in;\n"
			<< "layout(r32ui, binding = 1) readonly uniform highp uimage2D u_srcImg;\n"
			<< "layout(binding = 0) buffer Output {\n"
			<< "    uint values[" << (m_imageSize[0]*m_imageSize[1]) << "];\n"
			<< "} sb_out;\n\n"
			<< "void main (void) {\n"
			<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
			<< "    uint value  = imageLoad(u_srcImg, ivec2(gl_GlobalInvocationID.xy)).x;\n"
			<< "    sb_out.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x] = value;\n"
			<< "}\n";

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Buffer				outputBuffer	(m_context.getRenderContext());
		const Texture				inputTexture	(m_context.getRenderContext());
		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));
		const tcu::IVec2			workSize		= m_imageSize / m_localSize;
		de::Random					rnd				(0xab2c7);
		vector<deUint32>			inputValues		(m_imageSize[0]*m_imageSize[1]);

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Input values
		for (vector<deUint32>::iterator i = inputValues.begin(); i != inputValues.end(); ++i)
			*i = rnd.getUint32();

		// Input image setup
		gl.bindTexture(GL_TEXTURE_2D, *inputTexture);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, m_imageSize[0], m_imageSize[1]);
		gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_imageSize[0], m_imageSize[1], GL_RED_INTEGER, GL_UNSIGNED_INT, &inputValues[0]);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading image data failed");

		// Bind to unit 1
		gl.bindImageTexture(1, *inputTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Image setup failed");

		// Output buffer setup
		{
			const deUint32		blockIndex		= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(workSize[0], workSize[1], 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Output.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			TCU_CHECK(valueInfo.arraySize == (deUint32)inputValues.size());

			for (deUint32 ndx = 0; ndx < valueInfo.arraySize; ndx++)
			{
				const deUint32	res		= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*ndx));
				const deUint32	ref		= inputValues[ndx];

				if (res != ref)
					throw tcu::TestError(string("Comparison failed for Output.values[") + de::toString(ndx) + "]");
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const tcu::IVec2	m_localSize;
	const tcu::IVec2	m_imageSize;
};

class CopySSBOToImageCase : public TestCase
{
public:
	CopySSBOToImageCase (Context& context, const char* name, const char* description, const tcu::IVec2& localSize, const tcu::IVec2& imageSize)
		: TestCase		(context, name, description)
		, m_localSize	(localSize)
		, m_imageSize	(imageSize)
	{
		DE_ASSERT(m_imageSize[0] % m_localSize[0] == 0);
		DE_ASSERT(m_imageSize[1] % m_localSize[1] == 0);
	}

	IterateResult iterate (void)
	{
		const GLSLVersion			glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream			src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ") in;\n"
			<< "layout(r32ui, binding = 1) writeonly uniform highp uimage2D u_dstImg;\n"
			<< "buffer Input {\n"
			<< "    uint values[" << (m_imageSize[0]*m_imageSize[1]) << "];\n"
			<< "} sb_in;\n\n"
			<< "void main (void) {\n"
			<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
			<< "    uint value  = sb_in.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x];\n"
			<< "    imageStore(u_dstImg, ivec2(gl_GlobalInvocationID.xy), uvec4(value, 0, 0, 0));\n"
			<< "}\n";

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Buffer				inputBuffer		(m_context.getRenderContext());
		const Texture				outputTexture	(m_context.getRenderContext());
		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));
		const tcu::IVec2			workSize		= m_imageSize / m_localSize;
		de::Random					rnd				(0x77238ac2);
		vector<deUint32>			inputValues		(m_imageSize[0]*m_imageSize[1]);

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Input values
		for (vector<deUint32>::iterator i = inputValues.begin(); i != inputValues.end(); ++i)
			*i = rnd.getUint32();

		// Input buffer setup
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Input");
			const InterfaceBlockInfo	blockInfo	= getProgramInterfaceBlockInfo(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Input.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *inputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, (glw::GLsizeiptr)blockInfo.dataSize, DE_NULL, GL_STATIC_DRAW);

			TCU_CHECK(valueInfo.arraySize == (deUint32)inputValues.size());

			{
				const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, (int)blockInfo.dataSize, GL_MAP_WRITE_BIT);

				for (deUint32 ndx = 0; ndx < (deUint32)inputValues.size(); ndx++)
					*(deUint32*)((deUint8*)bufMap.getPtr() + valueInfo.offset + ndx*valueInfo.arrayStride) = inputValues[ndx];
			}

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, blockInfo.bufferBinding, *inputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Input buffer setup failed");
		}

		// Output image setup
		gl.bindTexture(GL_TEXTURE_2D, *outputTexture);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, m_imageSize[0], m_imageSize[1]);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading image data failed");

		// Bind to unit 1
		gl.bindImageTexture(1, *outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Image setup failed");

		// Dispatch compute workload
		gl.dispatchCompute(workSize[0], workSize[1], 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			Framebuffer			fbo			(m_context.getRenderContext());
			vector<deUint32>	pixels		(inputValues.size()*4);

			gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
			gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *outputTexture, 0);
			TCU_CHECK(gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

			// \note In ES3 we have to use GL_RGBA_INTEGER
			gl.readBuffer(GL_COLOR_ATTACHMENT0);
			gl.readPixels(0, 0, m_imageSize[0], m_imageSize[1], GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixels[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Reading pixels failed");

			for (deUint32 ndx = 0; ndx < (deUint32)inputValues.size(); ndx++)
			{
				const deUint32	res		= pixels[ndx*4];
				const deUint32	ref		= inputValues[ndx];

				if (res != ref)
					throw tcu::TestError(string("Comparison failed for pixel ") + de::toString(ndx));
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const tcu::IVec2	m_localSize;
	const tcu::IVec2	m_imageSize;
};

class ImageAtomicOpCase : public TestCase
{
public:
	ImageAtomicOpCase (Context& context, const char* name, const char* description, int localSize, const tcu::IVec2& imageSize)
		: TestCase		(context, name, description)
		, m_localSize	(localSize)
		, m_imageSize	(imageSize)
	{
	}

	void init (void)
	{
		if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
			if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_image_atomic"))
				throw tcu::NotSupportedError("Test requires OES_shader_image_atomic extension");
	}

	IterateResult iterate (void)
	{
		const GLSLVersion			glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		const bool					supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
		std::ostringstream			src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< (supportsES32 ? "\n" : "#extension GL_OES_shader_image_atomic : require\n")
			<< "layout (local_size_x = " << m_localSize << ") in;\n"
			<< "layout(r32ui, binding = 1) uniform highp uimage2D u_dstImg;\n"
			<< "buffer Input {\n"
			<< "    uint values[" << (m_imageSize[0]*m_imageSize[1]*m_localSize) << "];\n"
			<< "} sb_in;\n\n"
			<< "void main (void) {\n"
			<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
			<< "    uint value  = sb_in.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x];\n"
			<< "\n"
			<< "    if (gl_LocalInvocationIndex == 0u)\n"
			<< "        imageStore(u_dstImg, ivec2(gl_WorkGroupID.xy), uvec4(0));\n"
			<< "    barrier();\n"
			<< "    imageAtomicAdd(u_dstImg, ivec2(gl_WorkGroupID.xy), value);\n"
			<< "}\n";

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Buffer				inputBuffer		(m_context.getRenderContext());
		const Texture				outputTexture	(m_context.getRenderContext());
		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ShaderSource(SHADERTYPE_COMPUTE, src.str()));
		de::Random					rnd				(0x77238ac2);
		vector<deUint32>			inputValues		(m_imageSize[0]*m_imageSize[1]*m_localSize);

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_imageSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Input values
		for (vector<deUint32>::iterator i = inputValues.begin(); i != inputValues.end(); ++i)
			*i = rnd.getUint32();

		// Input buffer setup
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Input");
			const InterfaceBlockInfo	blockInfo	= getProgramInterfaceBlockInfo(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Input.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *inputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, (glw::GLsizeiptr)blockInfo.dataSize, DE_NULL, GL_STATIC_DRAW);

			TCU_CHECK(valueInfo.arraySize == (deUint32)inputValues.size());

			{
				const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, (int)blockInfo.dataSize, GL_MAP_WRITE_BIT);

				for (deUint32 ndx = 0; ndx < (deUint32)inputValues.size(); ndx++)
					*(deUint32*)((deUint8*)bufMap.getPtr() + valueInfo.offset + ndx*valueInfo.arrayStride) = inputValues[ndx];
			}

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, blockInfo.bufferBinding, *inputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Input buffer setup failed");
		}

		// Output image setup
		gl.bindTexture(GL_TEXTURE_2D, *outputTexture);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, m_imageSize[0], m_imageSize[1]);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading image data failed");

		// Bind to unit 1
		gl.bindImageTexture(1, *outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Image setup failed");

		// Dispatch compute workload
		gl.dispatchCompute(m_imageSize[0], m_imageSize[1], 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare
		{
			Framebuffer			fbo			(m_context.getRenderContext());
			vector<deUint32>	pixels		(m_imageSize[0]*m_imageSize[1]*4);

			gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
			gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *outputTexture, 0);
			TCU_CHECK(gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

			// \note In ES3 we have to use GL_RGBA_INTEGER
			gl.readBuffer(GL_COLOR_ATTACHMENT0);
			gl.readPixels(0, 0, m_imageSize[0], m_imageSize[1], GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixels[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Reading pixels failed");

			for (int pixelNdx = 0; pixelNdx < (int)inputValues.size()/m_localSize; pixelNdx++)
			{
				const deUint32	res		= pixels[pixelNdx*4];
				deUint32		ref		= 0;

				for (int offs = 0; offs < m_localSize; offs++)
					ref += inputValues[pixelNdx*m_localSize + offs];

				if (res != ref)
					throw tcu::TestError(string("Comparison failed for pixel ") + de::toString(pixelNdx));
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const int			m_localSize;
	const tcu::IVec2	m_imageSize;
};

class ImageBarrierCase : public TestCase
{
public:
	ImageBarrierCase (Context& context, const char* name, const char* description, const tcu::IVec2& workSize)
		: TestCase		(context, name, description)
		, m_workSize	(workSize)
	{
	}

	IterateResult iterate (void)
	{
		const GLSLVersion			glslVersion				= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		const char* const			glslVersionDeclaration	= getGLSLVersionDeclaration(glslVersion);

		std::ostringstream src0;
		src0 << glslVersionDeclaration << "\n"
			 << "layout (local_size_x = 1) in;\n"
						  "uniform uint u_baseVal;\n"
						  "layout(r32ui, binding = 2) writeonly uniform highp uimage2D u_img;\n"
						  "void main (void) {\n"
						  "    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
						  "    imageStore(u_img, ivec2(gl_WorkGroupID.xy), uvec4(offset+u_baseVal, 0, 0, 0));\n"
				"}\n";

		std::ostringstream src1;
		src1 << glslVersionDeclaration << "\n"
			 << "layout (local_size_x = 1) in;\n"
						  "layout(r32ui, binding = 2) readonly uniform highp uimage2D u_img;\n"
						  "layout(binding = 0) buffer Output {\n"
						  "    coherent uint sum;\n"
						  "};\n"
						  "void main (void) {\n"
						  "    uint value = imageLoad(u_img, ivec2(gl_WorkGroupID.xy)).x;\n"
						  "    atomicAdd(sum, value);\n"
				"}\n";

		const ShaderProgram			program0		(m_context.getRenderContext(), ProgramSources() << ComputeSource(src0.str()));
		const ShaderProgram			program1		(m_context.getRenderContext(), ProgramSources() << ComputeSource(src1.str()));

		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Texture				tempTexture		(m_context.getRenderContext());
		const Buffer				outputBuffer	(m_context.getRenderContext());
		const deUint32				baseValue		= 127;

		m_testCtx.getLog() << program0 << program1;
		if (!program0.isOk() || !program1.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		// Temp texture setup
		gl.bindTexture(GL_TEXTURE_2D, *tempTexture);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, m_workSize[0], m_workSize[1]);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading image data failed");

		// Bind to unit 2
		gl.bindImageTexture(2, *tempTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Image setup failed");

		// Output buffer setup
		{
			const deUint32		blockIndex		= gl.getProgramResourceIndex(program1.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= getProgramResourceInt(gl, program1.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);

			{
				const BufferMemMap bufMap(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_WRITE_BIT);
				deMemset(bufMap.getPtr(), 0, blockSize);
			}

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.useProgram(program0.getProgram());
		gl.uniform1ui(gl.getUniformLocation(program0.getProgram(), "u_baseVal"), baseValue);
		gl.dispatchCompute(m_workSize[0], m_workSize[1], 1);
		gl.memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		gl.useProgram(program1.getProgram());
		gl.dispatchCompute(m_workSize[0], m_workSize[1], 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to dispatch commands");

		// Read back and compare
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program1.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program1.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program1.getProgram(), GL_BUFFER_VARIABLE, "sum");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program1.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);

			const deUint32				res			= *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset));
			deUint32					ref			= 0;

			for (int ndx = 0; ndx < m_workSize[0]*m_workSize[1]; ndx++)
				ref += baseValue + (deUint32)ndx;

			if (res != ref)
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: comparison failed, expected " << ref << ", got " << res << TestLog::EndMessage;
				throw tcu::TestError("Comparison failed");
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const tcu::IVec2	m_workSize;
};

class AtomicCounterCase : public TestCase
{
public:
	AtomicCounterCase (Context& context, const char* name, const char* description, const tcu::IVec3& localSize, const tcu::IVec3& workSize)
		: TestCase		(context, name, description)
		, m_localSize	(localSize)
		, m_workSize	(workSize)
	{
	}

	IterateResult iterate (void)
	{
		const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
		const Buffer				outputBuffer	(m_context.getRenderContext());
		const Buffer				counterBuffer	(m_context.getRenderContext());
		const int					workGroupSize	= m_localSize[0]*m_localSize[1]*m_localSize[2];
		const int					workGroupCount	= m_workSize[0]*m_workSize[1]*m_workSize[2];
		const int					numValues		= workGroupSize*workGroupCount;

		const GLSLVersion			glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
		std::ostringstream			src;

		src << getGLSLVersionDeclaration(glslVersion) << "\n"
			<< "layout (local_size_x = " << m_localSize[0] << ", local_size_y = " << m_localSize[1] << ", local_size_z = " << m_localSize[2] << ") in;\n"
			<< "layout(binding = 0) buffer Output {\n"
			<< "    uint values[" << numValues << "];\n"
			<< "} sb_out;\n\n"
			<< "layout(binding = 0, offset = 0) uniform atomic_uint u_count;\n\n"
			<< "void main (void) {\n"
			<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
			<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
			<< "    uint globalOffs = localSize*globalNdx;\n"
			<< "    uint localOffs  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_LocalInvocationID.z + gl_WorkGroupSize.x*gl_LocalInvocationID.y + gl_LocalInvocationID.x;\n"
			<< "\n"
			<< "    uint oldVal = atomicCounterIncrement(u_count);\n"
			<< "    sb_out.values[globalOffs+localOffs] = oldVal;\n"
			<< "}\n";

		const ShaderProgram			program			(m_context.getRenderContext(), ProgramSources() << ComputeSource(src.str()));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		m_testCtx.getLog() << TestLog::Message << "Work groups: " << m_workSize << TestLog::EndMessage;

		gl.useProgram(program.getProgram());

		// Atomic counter buffer setup
		{
			const deUint32	uniformIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_UNIFORM, "u_count");
			const deUint32	bufferIndex		= getProgramResourceUint(gl, program.getProgram(), GL_UNIFORM, uniformIndex, GL_ATOMIC_COUNTER_BUFFER_INDEX);
			const deUint32	bufferSize		= getProgramResourceUint(gl, program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, bufferIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, *counterBuffer);
			gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, bufferSize, DE_NULL, GL_STREAM_READ);

			{
				const BufferMemMap memMap(gl, GL_ATOMIC_COUNTER_BUFFER, 0, bufferSize, GL_MAP_WRITE_BIT);
				deMemset(memMap.getPtr(), 0, (int)bufferSize);
			}

			gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, *counterBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Atomic counter buffer setup failed");
		}

		// Output buffer setup
		{
			const deUint32		blockIndex		= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int			blockSize		= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *outputBuffer);
			gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockSize, DE_NULL, GL_STREAM_READ);
			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *outputBuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
		}

		// Dispatch compute workload
		gl.dispatchCompute(m_workSize[0], m_workSize[1], m_workSize[2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

		// Read back and compare atomic counter
		{
			const deUint32		uniformIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_UNIFORM, "u_count");
			const deUint32		uniformOffset	= getProgramResourceUint(gl, program.getProgram(), GL_UNIFORM, uniformIndex, GL_OFFSET);
			const deUint32		bufferIndex		= getProgramResourceUint(gl, program.getProgram(), GL_UNIFORM, uniformIndex, GL_ATOMIC_COUNTER_BUFFER_INDEX);
			const deUint32		bufferSize		= getProgramResourceUint(gl, program.getProgram(), GL_ATOMIC_COUNTER_BUFFER, bufferIndex, GL_BUFFER_DATA_SIZE);
			const BufferMemMap	bufMap			(gl, GL_ATOMIC_COUNTER_BUFFER, 0, bufferSize, GL_MAP_READ_BIT);

			const deUint32		resVal			= *((const deUint32*)((const deUint8*)bufMap.getPtr() + uniformOffset));

			if (resVal != (deUint32)numValues)
				throw tcu::TestError("Invalid atomic counter value");
		}

		// Read back and compare SSBO
		{
			const deUint32				blockIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_SHADER_STORAGE_BLOCK, "Output");
			const int					blockSize	= getProgramResourceInt(gl, program.getProgram(), GL_SHADER_STORAGE_BLOCK, blockIndex, GL_BUFFER_DATA_SIZE);
			const deUint32				valueIndex	= gl.getProgramResourceIndex(program.getProgram(), GL_BUFFER_VARIABLE, "Output.values");
			const InterfaceVariableInfo	valueInfo	= getProgramInterfaceVariableInfo(gl, program.getProgram(), GL_BUFFER_VARIABLE, valueIndex);
			const BufferMemMap			bufMap		(gl, GL_SHADER_STORAGE_BUFFER, 0, blockSize, GL_MAP_READ_BIT);
			deUint32					valSum		= 0;
			deUint32					refSum		= 0;

			for (int valNdx = 0; valNdx < numValues; valNdx++)
			{
				const deUint32 res = *((const deUint32*)((const deUint8*)bufMap.getPtr() + valueInfo.offset + valueInfo.arrayStride*valNdx));

				valSum += res;
				refSum += (deUint32)valNdx;

				if (!de::inBounds<deUint32>(res, 0, (deUint32)numValues))
					throw tcu::TestError(string("Comparison failed for Output.values[") + de::toString(valNdx) + "]");
			}

			if (valSum != refSum)
				throw tcu::TestError("Total sum of values in Output.values doesn't match");
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	const tcu::IVec3	m_localSize;
	const tcu::IVec3	m_workSize;
};

} // anonymous

BasicComputeShaderTests::BasicComputeShaderTests (Context& context)
	: TestCaseGroup(context, "basic", "Basic Compute Shader Tests")
{
}

BasicComputeShaderTests::~BasicComputeShaderTests (void)
{
}

void BasicComputeShaderTests::init (void)
{
	addChild(new EmptyComputeShaderCase(m_context));

	addChild(new UBOToSSBOInvertCase	(m_context, "ubo_to_ssbo_single_invocation",			"Copy from UBO to SSBO, inverting bits",	256,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new UBOToSSBOInvertCase	(m_context, "ubo_to_ssbo_single_group",					"Copy from UBO to SSBO, inverting bits",	1024,	tcu::IVec3(2,1,4),	tcu::IVec3(1,1,1)));
	addChild(new UBOToSSBOInvertCase	(m_context, "ubo_to_ssbo_multiple_invocations",			"Copy from UBO to SSBO, inverting bits",	1024,	tcu::IVec3(1,1,1),	tcu::IVec3(2,4,1)));
	addChild(new UBOToSSBOInvertCase	(m_context, "ubo_to_ssbo_multiple_groups",				"Copy from UBO to SSBO, inverting bits",	1024,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	addChild(new CopyInvertSSBOCase		(m_context, "copy_ssbo_single_invocation",				"Copy between SSBOs, inverting bits",	256,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new CopyInvertSSBOCase		(m_context, "copy_ssbo_multiple_invocations",			"Copy between SSBOs, inverting bits",	1024,	tcu::IVec3(1,1,1),	tcu::IVec3(2,4,1)));
	addChild(new CopyInvertSSBOCase		(m_context, "copy_ssbo_multiple_groups",				"Copy between SSBOs, inverting bits",	1024,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	addChild(new InvertSSBOInPlaceCase	(m_context, "ssbo_rw_single_invocation",				"Read and write same SSBO",				256,	true,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new InvertSSBOInPlaceCase	(m_context, "ssbo_rw_multiple_groups",					"Read and write same SSBO",				1024,	true,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	addChild(new InvertSSBOInPlaceCase	(m_context, "ssbo_unsized_arr_single_invocation",		"Read and write same SSBO",				256,	false,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new InvertSSBOInPlaceCase	(m_context, "ssbo_unsized_arr_multiple_groups",			"Read and write same SSBO",				1024,	false,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	addChild(new WriteToMultipleSSBOCase(m_context, "write_multiple_arr_single_invocation",		"Write to multiple SSBOs",				256,	true,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new WriteToMultipleSSBOCase(m_context, "write_multiple_arr_multiple_groups",		"Write to multiple SSBOs",				1024,	true,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	addChild(new WriteToMultipleSSBOCase(m_context, "write_multiple_unsized_arr_single_invocation",	"Write to multiple SSBOs",			256,	false,	tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new WriteToMultipleSSBOCase(m_context, "write_multiple_unsized_arr_multiple_groups",	"Write to multiple SSBOs",			1024,	false,	tcu::IVec3(1,4,2),	tcu::IVec3(2,2,4)));

	addChild(new SSBOLocalBarrierCase	(m_context, "ssbo_local_barrier_single_invocation",		"SSBO local barrier usage",				tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new SSBOLocalBarrierCase	(m_context, "ssbo_local_barrier_single_group",			"SSBO local barrier usage",				tcu::IVec3(3,2,5),	tcu::IVec3(1,1,1)));
	addChild(new SSBOLocalBarrierCase	(m_context, "ssbo_local_barrier_multiple_groups",		"SSBO local barrier usage",				tcu::IVec3(3,4,1),	tcu::IVec3(2,7,3)));

	addChild(new SSBOBarrierCase		(m_context, "ssbo_cmd_barrier_single",					"SSBO memory barrier usage",			tcu::IVec3(1,1,1)));
	addChild(new SSBOBarrierCase		(m_context, "ssbo_cmd_barrier_multiple",				"SSBO memory barrier usage",			tcu::IVec3(11,5,7)));

	addChild(new BasicSharedVarCase		(m_context, "shared_var_single_invocation",				"Basic shared variable usage",			tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new BasicSharedVarCase		(m_context, "shared_var_single_group",					"Basic shared variable usage",			tcu::IVec3(3,2,5),	tcu::IVec3(1,1,1)));
	addChild(new BasicSharedVarCase		(m_context, "shared_var_multiple_invocations",			"Basic shared variable usage",			tcu::IVec3(1,1,1),	tcu::IVec3(2,5,4)));
	addChild(new BasicSharedVarCase		(m_context, "shared_var_multiple_groups",				"Basic shared variable usage",			tcu::IVec3(3,4,1),	tcu::IVec3(2,7,3)));

	addChild(new SharedVarAtomicOpCase	(m_context, "shared_atomic_op_single_invocation",		"Atomic operation with shared var",		tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new SharedVarAtomicOpCase	(m_context, "shared_atomic_op_single_group",			"Atomic operation with shared var",		tcu::IVec3(3,2,5),	tcu::IVec3(1,1,1)));
	addChild(new SharedVarAtomicOpCase	(m_context, "shared_atomic_op_multiple_invocations",	"Atomic operation with shared var",		tcu::IVec3(1,1,1),	tcu::IVec3(2,5,4)));
	addChild(new SharedVarAtomicOpCase	(m_context, "shared_atomic_op_multiple_groups",			"Atomic operation with shared var",		tcu::IVec3(3,4,1),	tcu::IVec3(2,7,3)));

	addChild(new CopyImageToSSBOCase	(m_context, "copy_image_to_ssbo_small",					"Image to SSBO copy",					tcu::IVec2(1,1),	tcu::IVec2(64,64)));
	addChild(new CopyImageToSSBOCase	(m_context, "copy_image_to_ssbo_large",					"Image to SSBO copy",					tcu::IVec2(2,4),	tcu::IVec2(512,512)));

	addChild(new CopySSBOToImageCase	(m_context, "copy_ssbo_to_image_small",					"SSBO to image copy",					tcu::IVec2(1,1),	tcu::IVec2(64,64)));
	addChild(new CopySSBOToImageCase	(m_context, "copy_ssbo_to_image_large",					"SSBO to image copy",					tcu::IVec2(2,4),	tcu::IVec2(512,512)));

	addChild(new ImageAtomicOpCase		(m_context, "image_atomic_op_local_size_1",				"Atomic operation with image",			1,	tcu::IVec2(64,64)));
	addChild(new ImageAtomicOpCase		(m_context, "image_atomic_op_local_size_8",				"Atomic operation with image",			8,	tcu::IVec2(64,64)));

	addChild(new ImageBarrierCase		(m_context, "image_barrier_single",						"Image barrier",						tcu::IVec2(1,1)));
	addChild(new ImageBarrierCase		(m_context, "image_barrier_multiple",					"Image barrier",						tcu::IVec2(64,64)));

	addChild(new AtomicCounterCase		(m_context, "atomic_counter_single_invocation",			"Basic atomic counter test",			tcu::IVec3(1,1,1),	tcu::IVec3(1,1,1)));
	addChild(new AtomicCounterCase		(m_context, "atomic_counter_single_group",				"Basic atomic counter test",			tcu::IVec3(3,2,5),	tcu::IVec3(1,1,1)));
	addChild(new AtomicCounterCase		(m_context, "atomic_counter_multiple_invocations",		"Basic atomic counter test",			tcu::IVec3(1,1,1),	tcu::IVec3(2,5,4)));
	addChild(new AtomicCounterCase		(m_context, "atomic_counter_multiple_groups",			"Basic atomic counter test",			tcu::IVec3(3,4,1),	tcu::IVec3(2,7,3)));
}

} // Functional
} // gles31
} // deqp
