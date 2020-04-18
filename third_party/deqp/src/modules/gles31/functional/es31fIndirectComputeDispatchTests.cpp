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
 * \brief Indirect compute dispatch tests.
 *//*--------------------------------------------------------------------*/

#include "es31fIndirectComputeDispatchTests.hpp"
#include "gluObjectWrapper.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "tcuVector.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"
#include "deStringUtil.hpp"

#include <vector>
#include <string>
#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{

using tcu::UVec3;
using tcu::TestLog;
using std::vector;
using std::string;
using std::map;

// \todo [2014-02-17 pyry] Should be extended with following:

// Negative:
//  - no active shader program
//  - indirect negative or not aligned
//  - indirect + size outside buffer bounds
//  - no buffer bound to DRAW_INDIRECT_BUFFER
//  - (implict) buffer mapped

// Robustness:
//  - lot of small work group launches
//  - very large work group size
//  - no synchronization, touched by gpu
//  - compute program overwiting buffer

namespace
{

enum
{
	RESULT_BLOCK_BASE_SIZE				= (3+1)*(int)sizeof(deUint32),		// uvec3 + uint
	RESULT_BLOCK_EXPECTED_COUNT_OFFSET	= 0,
	RESULT_BLOCK_NUM_PASSED_OFFSET		= 3*(int)sizeof(deUint32),

	INDIRECT_COMMAND_SIZE				= 3*(int)sizeof(deUint32)
};

enum GenBuffer
{
	GEN_BUFFER_UPLOAD		= 0,
	GEN_BUFFER_COMPUTE,

	GEN_BUFFER_LAST
};

glu::ProgramSources genVerifySources (const UVec3& workGroupSize)
{
	static const char* s_verifyDispatchTmpl =
		"#version 310 es\n"
		"layout(local_size_x = ${LOCAL_SIZE_X}, local_size_y = ${LOCAL_SIZE_Y}, local_size_z = ${LOCAL_SIZE_Z}) in;\n"
		"layout(binding = 0, std430) buffer Result\n"
		"{\n"
		"    uvec3           expectedGroupCount;\n"
		"    coherent uint   numPassed;\n"
		"} result;\n"
		"void main (void)\n"
		"{\n"
		"    if (all(equal(result.expectedGroupCount, gl_NumWorkGroups)))\n"
		"        atomicAdd(result.numPassed, 1u);\n"
		"}\n";

	map<string, string> args;

	args["LOCAL_SIZE_X"] = de::toString(workGroupSize.x());
	args["LOCAL_SIZE_Y"] = de::toString(workGroupSize.y());
	args["LOCAL_SIZE_Z"] = de::toString(workGroupSize.z());

	return glu::ProgramSources() << glu::ComputeSource(tcu::StringTemplate(s_verifyDispatchTmpl).specialize(args));
}

class IndirectDispatchCase : public TestCase
{
public:
							IndirectDispatchCase	(Context& context, const char* name, const char* description, GenBuffer genBuffer);
							~IndirectDispatchCase	(void);

	IterateResult			iterate					(void);

protected:
	struct DispatchCommand
	{
		deIntptr	offset;
		UVec3		numWorkGroups;

		DispatchCommand (void) : offset(0) {}
		DispatchCommand (deIntptr offset_, const UVec3& numWorkGroups_) : offset(offset_), numWorkGroups(numWorkGroups_) {}
	};

	GenBuffer				m_genBuffer;
	deUintptr				m_bufferSize;
	UVec3					m_workGroupSize;
	vector<DispatchCommand>	m_commands;

	void					createCommandBuffer		(deUint32 buffer) const;
	void					createResultBuffer		(deUint32 buffer) const;

	bool					verifyResultBuffer		(deUint32 buffer);

	void					createCmdBufferUpload	(deUint32 buffer) const;
	void					createCmdBufferCompute	(deUint32 buffer) const;

private:
							IndirectDispatchCase	(const IndirectDispatchCase&);
	IndirectDispatchCase&	operator=				(const IndirectDispatchCase&);
};

IndirectDispatchCase::IndirectDispatchCase (Context& context, const char* name, const char* description, GenBuffer genBuffer)
	: TestCase		(context, name, description)
	, m_genBuffer	(genBuffer)
	, m_bufferSize	(0)
{
}

IndirectDispatchCase::~IndirectDispatchCase (void)
{
}

static int getResultBlockAlignedSize (const glw::Functions& gl)
{
	const int	baseSize	= RESULT_BLOCK_BASE_SIZE;
	int			alignment	= 0;
	gl.getIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);

	if (alignment == 0 || (baseSize % alignment == 0))
		return baseSize;
	else
		return (baseSize/alignment + 1)*alignment;
}

void IndirectDispatchCase::createCommandBuffer (deUint32 buffer) const
{
	switch (m_genBuffer)
	{
		case GEN_BUFFER_UPLOAD:		createCmdBufferUpload	(buffer);		break;
		case GEN_BUFFER_COMPUTE:	createCmdBufferCompute	(buffer);		break;
		default:
			DE_ASSERT(false);
	}
}

void IndirectDispatchCase::createCmdBufferUpload (deUint32 buffer) const
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	vector<deUint8>			data	(m_bufferSize);

	for (vector<DispatchCommand>::const_iterator cmdIter = m_commands.begin(); cmdIter != m_commands.end(); ++cmdIter)
	{
		DE_STATIC_ASSERT(INDIRECT_COMMAND_SIZE >= sizeof(deUint32)*3);
		DE_ASSERT(cmdIter->offset >= 0);
		DE_ASSERT(cmdIter->offset%sizeof(deUint32) == 0);
		DE_ASSERT(cmdIter->offset + INDIRECT_COMMAND_SIZE <= (deIntptr)m_bufferSize);

		deUint32* const dstPtr = (deUint32*)&data[cmdIter->offset];

		dstPtr[0] = cmdIter->numWorkGroups[0];
		dstPtr[1] = cmdIter->numWorkGroups[1];
		dstPtr[2] = cmdIter->numWorkGroups[2];
	}

	gl.bindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer);
	gl.bufferData(GL_DISPATCH_INDIRECT_BUFFER, (glw::GLsizeiptr)data.size(), &data[0], GL_STATIC_DRAW);
}

void IndirectDispatchCase::createCmdBufferCompute (deUint32 buffer) const
{
	std::ostringstream src;

	// Header
	src <<
		"#version 310 es\n"
		"layout(local_size_x = 1) in;\n"
		"layout(std430, binding = 1) buffer Out\n"
		"{\n"
		"	highp uint data[];\n"
		"};\n"
		"void writeCmd (uint offset, uvec3 numWorkGroups)\n"
		"{\n"
		"	data[offset+0u] = numWorkGroups.x;\n"
		"	data[offset+1u] = numWorkGroups.y;\n"
		"	data[offset+2u] = numWorkGroups.z;\n"
		"}\n"
		"void main (void)\n"
		"{\n";

	// Commands
	for (vector<DispatchCommand>::const_iterator cmdIter = m_commands.begin(); cmdIter != m_commands.end(); ++cmdIter)
	{
		const deUint32 offs = (deUint32)(cmdIter->offset/4);
		DE_ASSERT((deIntptr)offs*4 == cmdIter->offset);

		src << "\twriteCmd(" << offs << "u, uvec3("
			<< cmdIter->numWorkGroups.x() << "u, "
			<< cmdIter->numWorkGroups.y() << "u, "
			<< cmdIter->numWorkGroups.z() << "u));\n";
	}

	src << "}\n";

	{
		const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
		glu::ShaderProgram		program		(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(src.str()));

		m_testCtx.getLog() << program;
		if (!program.isOk())
			TCU_FAIL("Compile failed");

		gl.useProgram(program.getProgram());

		gl.bindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer);
		gl.bufferData(GL_DISPATCH_INDIRECT_BUFFER, (glw::GLsizeiptr)m_bufferSize, DE_NULL, GL_STATIC_DRAW);
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Buffer setup failed");

		gl.dispatchCompute(1,1,1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute() failed");

		gl.memoryBarrier(GL_COMMAND_BARRIER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier(GL_COMMAND_BARRIER_BIT) failed");
	}
}

void IndirectDispatchCase::createResultBuffer (deUint32 buffer) const
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const int				resultBlockSize		= getResultBlockAlignedSize(gl);
	const int				resultBufferSize	= resultBlockSize*(int)m_commands.size();
	vector<deUint8>			data				(resultBufferSize);

	for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
	{
		deUint8* const	dstPtr	= &data[resultBlockSize*cmdNdx];

		*(deUint32*)(dstPtr + RESULT_BLOCK_EXPECTED_COUNT_OFFSET + 0*4)	= m_commands[cmdNdx].numWorkGroups[0];
		*(deUint32*)(dstPtr + RESULT_BLOCK_EXPECTED_COUNT_OFFSET + 1*4)	= m_commands[cmdNdx].numWorkGroups[1];
		*(deUint32*)(dstPtr + RESULT_BLOCK_EXPECTED_COUNT_OFFSET + 2*4)	= m_commands[cmdNdx].numWorkGroups[2];
		*(deUint32*)(dstPtr + RESULT_BLOCK_NUM_PASSED_OFFSET)			= 0;
	}

	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, (glw::GLsizei)data.size(), &data[0], GL_STATIC_READ);
}

deUint32 computeInvocationCount (const UVec3& workGroupSize, const UVec3& numWorkGroups)
{
	const int	numInvocationsPerGroup	= workGroupSize[0]*workGroupSize[1]*workGroupSize[2];
	const int	numGroups				= numWorkGroups[0]*numWorkGroups[1]*numWorkGroups[2];

	return numInvocationsPerGroup*numGroups;
}

bool IndirectDispatchCase::verifyResultBuffer (deUint32 buffer)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();

	const int				resultBlockSize		= getResultBlockAlignedSize(gl);
	const int				resultBufferSize	= resultBlockSize*(int)m_commands.size();

	void*					mapPtr				= DE_NULL;
	bool					allOk				= true;

	try
	{
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
		mapPtr = gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, resultBufferSize, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() failed");
		TCU_CHECK(mapPtr);

		for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
		{
			const DispatchCommand&	cmd				= m_commands[cmdNdx];
			const deUint8* const	srcPtr			= (const deUint8*)mapPtr + cmdNdx*resultBlockSize;
			const deUint32			numPassed		= *(const deUint32*)(srcPtr + RESULT_BLOCK_NUM_PASSED_OFFSET);
			const deUint32			expectedCount	= computeInvocationCount(m_workGroupSize, cmd.numWorkGroups);

			// Verify numPassed.
			if (numPassed != expectedCount)
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: got invalid result for invocation " << cmdNdx
													   << ": got numPassed = " << numPassed << ", expected " << expectedCount
								   << TestLog::EndMessage;
				allOk = false;
			}
		}
	}
	catch (...)
	{
		if (mapPtr)
			gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed");

	return allOk;
}

IndirectDispatchCase::IterateResult IndirectDispatchCase::iterate (void)
{
	const glu::RenderContext&		renderCtx			= m_context.getRenderContext();
	const glw::Functions&			gl					= renderCtx.getFunctions();

	const glu::ShaderProgram		program				(renderCtx, genVerifySources(m_workGroupSize));

	glu::Buffer						cmdBuffer			(renderCtx);
	glu::Buffer						resultBuffer		(renderCtx);

	m_testCtx.getLog() << program;
	TCU_CHECK_MSG(program.isOk(), "Compile failed");

	m_testCtx.getLog() << TestLog::Message << "GL_DISPATCH_INDIRECT_BUFFER size = " << m_bufferSize << TestLog::EndMessage;
	{
		tcu::ScopedLogSection section(m_testCtx.getLog(), "Commands", "Indirect Dispatch Commands (" + de::toString(m_commands.size()) + " in total)");

		for (size_t cmdNdx = 0; cmdNdx < m_commands.size(); cmdNdx++)
			m_testCtx.getLog() << TestLog::Message << cmdNdx << ": " << "offset = " << m_commands[cmdNdx].offset
												   << ", numWorkGroups = " << m_commands[cmdNdx].numWorkGroups
							   << TestLog::EndMessage;
	}

	createResultBuffer(*resultBuffer);
	createCommandBuffer(*cmdBuffer);

	gl.useProgram(program.getProgram());
	gl.bindBuffer(GL_DISPATCH_INDIRECT_BUFFER, *cmdBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "State setup failed");

	{
		const int	resultBlockAlignedSize		= getResultBlockAlignedSize(gl);
		deIntptr	curOffset					= 0;

		for (vector<DispatchCommand>::const_iterator cmdIter = m_commands.begin(); cmdIter != m_commands.end(); ++cmdIter)
		{
			gl.bindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, *resultBuffer, (glw::GLintptr)curOffset, resultBlockAlignedSize);
			gl.dispatchComputeIndirect((glw::GLintptr)cmdIter->offset);

			curOffset += resultBlockAlignedSize;
		}
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchComputeIndirect() failed");

	if (verifyResultBuffer(*resultBuffer))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid values in result buffer");

	return STOP;
}

class SingleDispatchCase : public IndirectDispatchCase
{
public:
	SingleDispatchCase (Context& context, const char* name, const char* description, GenBuffer genBuffer, deUintptr bufferSize, deUintptr offset, const UVec3& workGroupSize, const UVec3& numWorkGroups)
		: IndirectDispatchCase(context, name, description, genBuffer)
	{
		m_bufferSize	= bufferSize;
		m_workGroupSize	= workGroupSize;
		m_commands.push_back(DispatchCommand(offset, numWorkGroups));
	}
};

class MultiDispatchCase : public IndirectDispatchCase
{
public:
	MultiDispatchCase (Context& context, GenBuffer genBuffer)
		: IndirectDispatchCase(context, "multi_dispatch", "Dispatch multiple compute commands from single buffer", genBuffer)
	{
		m_bufferSize	= 1<<10;
		m_workGroupSize	= UVec3(3,1,2);

		m_commands.push_back(DispatchCommand(0,						UVec3(1,1,1)));
		m_commands.push_back(DispatchCommand(INDIRECT_COMMAND_SIZE,	UVec3(2,1,1)));
		m_commands.push_back(DispatchCommand(104,					UVec3(1,3,1)));
		m_commands.push_back(DispatchCommand(40,					UVec3(1,1,7)));
		m_commands.push_back(DispatchCommand(52,					UVec3(1,1,4)));
	}
};

class MultiDispatchReuseCommandCase : public IndirectDispatchCase
{
public:
	MultiDispatchReuseCommandCase (Context& context, GenBuffer genBuffer)
		: IndirectDispatchCase(context, "multi_dispatch_reuse_command", "Dispatch multiple compute commands from single buffer", genBuffer)
	{
		m_bufferSize	= 1<<10;
		m_workGroupSize	= UVec3(3,1,2);

		m_commands.push_back(DispatchCommand(0,						UVec3(1,1,1)));
		m_commands.push_back(DispatchCommand(0,						UVec3(1,1,1)));
		m_commands.push_back(DispatchCommand(0,						UVec3(1,1,1)));
		m_commands.push_back(DispatchCommand(104,					UVec3(1,3,1)));
		m_commands.push_back(DispatchCommand(104,					UVec3(1,3,1)));
		m_commands.push_back(DispatchCommand(52,					UVec3(1,1,4)));
		m_commands.push_back(DispatchCommand(52,					UVec3(1,1,4)));
	}
};

} // anonymous

IndirectComputeDispatchTests::IndirectComputeDispatchTests (Context& context)
	: TestCaseGroup(context, "indirect_dispatch", "Indirect dispatch tests")
{
}

IndirectComputeDispatchTests::~IndirectComputeDispatchTests (void)
{
}

void IndirectComputeDispatchTests::init (void)
{
	static const struct
	{
		const char*		name;
		GenBuffer		gen;
	} s_genBuffer[] =
	{
		{ "upload_buffer",		GEN_BUFFER_UPLOAD	},
		{ "gen_in_compute",		GEN_BUFFER_COMPUTE	}
	};

	static const struct
	{
		const char*	name;
		const char*	description;
		deUintptr	bufferSize;
		deUintptr	offset;
		UVec3		workGroupSize;
		UVec3		numWorkGroups;
	} s_singleDispatchCases[] =
	{
	//	Name										Desc											BufferSize					Offs			WorkGroupSize	NumWorkGroups
		{ "single_invocation",						"Single invocation only from offset 0",			INDIRECT_COMMAND_SIZE,		0,				UVec3(1,1,1),	UVec3(1,1,1) },
		{ "multiple_groups",						"Multiple groups dispatched from offset 0",		INDIRECT_COMMAND_SIZE,		0,				UVec3(1,1,1),	UVec3(2,3,5) },
		{ "multiple_groups_multiple_invocations",	"Multiple groups of size 2x3x1 from offset 0",	INDIRECT_COMMAND_SIZE,		0,				UVec3(2,3,1),	UVec3(1,2,3) },
		{ "small_offset",							"Small offset",									16+INDIRECT_COMMAND_SIZE,	16,				UVec3(1,1,1),	UVec3(1,1,1) },
		{ "large_offset",							"Large offset",									(2<<20),					(1<<20) + 12,	UVec3(1,1,1),	UVec3(1,1,1) },
		{ "large_offset_multiple_invocations",		"Large offset, multiple invocations",			(2<<20),					(1<<20) + 12,	UVec3(2,3,1),	UVec3(1,2,3) },
		{ "empty_command",							"Empty command",								INDIRECT_COMMAND_SIZE,		0,				UVec3(1,1,1),	UVec3(0,0,0) },
	};

	for (int genNdx = 0; genNdx < DE_LENGTH_OF_ARRAY(s_genBuffer); genNdx++)
	{
		const GenBuffer				genBuf		= s_genBuffer[genNdx].gen;
		tcu::TestCaseGroup* const	genGroup	= new tcu::TestCaseGroup(m_testCtx, s_genBuffer[genNdx].name, "");
		addChild(genGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_singleDispatchCases); ndx++)
			genGroup->addChild(new SingleDispatchCase(m_context,
													  s_singleDispatchCases[ndx].name,
													  s_singleDispatchCases[ndx].description,
													  genBuf,
													  s_singleDispatchCases[ndx].bufferSize,
													  s_singleDispatchCases[ndx].offset,
													  s_singleDispatchCases[ndx].workGroupSize,
													  s_singleDispatchCases[ndx].numWorkGroups));

		genGroup->addChild(new MultiDispatchCase				(m_context, genBuf));
		genGroup->addChild(new MultiDispatchReuseCommandCase	(m_context, genBuf));
	}
}

} // Functional
} // gles31
} // deqp
