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
 * \brief SSBO array length tests.
 *//*--------------------------------------------------------------------*/

#include "es31fSSBOArrayLengthTests.hpp"
#include "gluShaderProgram.hpp"
#include "gluRenderContext.hpp"
#include "tcuTestLog.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"

#include <sstream>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

class SSBOArrayLengthCase : public TestCase
{
public:
	enum ArrayAccess
	{
		ACCESS_DEFAULT = 0,
		ACCESS_WRITEONLY,
		ACCESS_READONLY,

		ACCESS_LAST
	};

						SSBOArrayLengthCase		(Context& context, const char* name, const char* desc, ArrayAccess access, bool sized);
						~SSBOArrayLengthCase	(void);

	void				init					(void);
	void				deinit					(void);
	IterateResult		iterate					(void);

private:
	std::string			genComputeSource		(void) const;

	const ArrayAccess	m_access;
	const bool			m_sized;

	glu::ShaderProgram*	m_shader;
	deUint32			m_targetBufferID;
	deUint32			m_outputBufferID;

	static const int	s_fixedBufferSize = 16;
};

SSBOArrayLengthCase::SSBOArrayLengthCase (Context& context, const char* name, const char* desc, ArrayAccess access, bool sized)
	: TestCase			(context, name, desc)
	, m_access			(access)
	, m_sized			(sized)
	, m_shader			(DE_NULL)
	, m_targetBufferID	(0)
	, m_outputBufferID	(0)
{
}

SSBOArrayLengthCase::~SSBOArrayLengthCase (void)
{
	deinit();
}

void SSBOArrayLengthCase::init (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const deUint32			invalidValue	= 0xFFFFFFFFUL;

	// program
	m_shader = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genComputeSource()));
	m_testCtx.getLog() << *m_shader;

	if (!m_shader->isOk())
		throw tcu::TestError("Failed to build shader");

	// gen and attach buffers
	gl.genBuffers(1, &m_outputBufferID);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_outputBufferID);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, 2 * (int)sizeof(deUint32), &invalidValue, GL_DYNAMIC_COPY);

	gl.genBuffers(1, &m_targetBufferID);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_targetBufferID);

	GLU_EXPECT_NO_ERROR(gl.getError(), "create buffers");

	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_outputBufferID);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_targetBufferID);

	GLU_EXPECT_NO_ERROR(gl.getError(), "bind buffers");

	// check the ssbo has expected layout
	{
		const deUint32		index	= gl.getProgramResourceIndex(m_shader->getProgram(), GL_BUFFER_VARIABLE, "Out.outLength");
		const glw::GLenum	prop	= GL_OFFSET;
		glw::GLint			result	= 0;

		if (index == GL_INVALID_INDEX)
			throw tcu::TestError("Failed to find outLength variable");

		gl.getProgramResourceiv(m_shader->getProgram(), GL_BUFFER_VARIABLE, index, 1, &prop, 1, DE_NULL, &result);

		if (result != 0)
			throw tcu::TestError("Unexpected outLength location");
	}
	{
		const deUint32		index	= gl.getProgramResourceIndex(m_shader->getProgram(), GL_BUFFER_VARIABLE, "Out.unused");
		const glw::GLenum	prop	= GL_OFFSET;
		glw::GLint			result	= 0;

		if (index == GL_INVALID_INDEX)
			throw tcu::TestError("Failed to find unused variable");

		gl.getProgramResourceiv(m_shader->getProgram(), GL_BUFFER_VARIABLE, index, 1, &prop, 1, DE_NULL, &result);

		if (result != 4)
			throw tcu::TestError("Unexpected unused location");
	}
	{
		const deUint32		index	= gl.getProgramResourceIndex(m_shader->getProgram(), GL_BUFFER_VARIABLE, "Target.array");
		const glw::GLenum	prop	= GL_ARRAY_STRIDE;
		glw::GLint			result	= 0;

		if (index == GL_INVALID_INDEX)
			throw tcu::TestError("Failed to find array variable");

		gl.getProgramResourceiv(m_shader->getProgram(), GL_BUFFER_VARIABLE, index, 1, &prop, 1, DE_NULL, &result);

		if (result != 4)
			throw tcu::TestError("Unexpected array stride");
	}
}

void SSBOArrayLengthCase::deinit (void)
{
	if (m_shader)
	{
		delete m_shader;
		m_shader = DE_NULL;
	}

	if (m_targetBufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_targetBufferID);
		m_targetBufferID = 0;
	}

	if (m_outputBufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_outputBufferID);
		m_outputBufferID = 0;
	}
}

SSBOArrayLengthCase::IterateResult SSBOArrayLengthCase::iterate (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	bool					error	= false;

	// Update buffer size

	m_testCtx.getLog() << tcu::TestLog::Message << "Allocating float memory buffer with " << static_cast<int>(s_fixedBufferSize) << " elements." << tcu::TestLog::EndMessage;

	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_targetBufferID);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, s_fixedBufferSize * (int)sizeof(float), DE_NULL, GL_DYNAMIC_COPY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "update buffer");

	// Run compute

	m_testCtx.getLog() << tcu::TestLog::Message << "Running compute shader." << tcu::TestLog::EndMessage;

	gl.useProgram(m_shader->getProgram());
	gl.dispatchCompute(1, 1, 1);

	GLU_EXPECT_NO_ERROR(gl.getError(), "dispatch");

	// Verify
	{
		const void* ptr;

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_outputBufferID);
		ptr = gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (int)sizeof(deUint32), GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "map");

		if (!ptr)
			throw tcu::TestError("mapBufferRange returned NULL");

		if (*(const deUint32*)ptr != (deUint32)s_fixedBufferSize)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Length returned was " << *(const deUint32*)ptr << ", expected " << static_cast<int>(s_fixedBufferSize) << tcu::TestLog::EndMessage;
			error = true;
		}
		else
			m_testCtx.getLog() << tcu::TestLog::Message << "Length returned was correct." << tcu::TestLog::EndMessage;

		if (gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER) == GL_FALSE)
			throw tcu::TestError("unmapBuffer returned false");

		GLU_EXPECT_NO_ERROR(gl.getError(), "unmap");
	}

	if (!error)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	return STOP;
}

std::string SSBOArrayLengthCase::genComputeSource (void) const
{
	const std::string qualifierStr	= (m_access == ACCESS_READONLY) ? ("readonly ") : (m_access == ACCESS_WRITEONLY) ? ("writeonly ") : ("");
	const std::string sizeStr		= (m_sized) ? (de::toString(static_cast<int>(s_fixedBufferSize))) : ("");

	std::ostringstream buf;
	buf << "#version 310 es\n"
		<< "layout(local_size_x = 1, local_size_y = 1) in;\n"
		<< "layout(std430) buffer;\n"
		<< "\n"
		<< "layout(binding = 0) buffer Out\n"
		<< "{\n"
		<< "    int outLength;\n"
		<< "    uint unused;\n"
		<< "} sb_out;\n"
		<< "layout(binding = 1) " << qualifierStr << "buffer Target\n"
		<< "{\n"
		<< "    float array[" << sizeStr << "];\n"
		<< "} sb_target;\n\n"
		<< "void main (void)\n"
		<< "{\n";

	// read
	if (m_access == ACCESS_READONLY || m_access == ACCESS_DEFAULT)
		buf << "    sb_out.unused = uint(sb_target.array[1]);\n";

	// write
	if (m_access == ACCESS_WRITEONLY || m_access == ACCESS_DEFAULT)
		buf << "    sb_target.array[2] = float(sb_out.unused);\n";

	// actual test
	buf << "\n"
		<< "    sb_out.outLength = sb_target.array.length();\n"
		<< "}\n";

	return buf.str();
}

} // anonymous

SSBOArrayLengthTests::SSBOArrayLengthTests (Context& context)
	: TestCaseGroup(context, "array_length", "Test array.length()")
{
}

SSBOArrayLengthTests::~SSBOArrayLengthTests (void)
{
}

void SSBOArrayLengthTests::init (void)
{
	static const struct Qualifier
	{
		SSBOArrayLengthCase::ArrayAccess	access;
		const char*							name;
		const char*							desc;
	}  qualifiers[] =
	{
		{ SSBOArrayLengthCase::ACCESS_DEFAULT,		"",				""			},
		{ SSBOArrayLengthCase::ACCESS_WRITEONLY,	"writeonly_",	"writeonly"	},
		{ SSBOArrayLengthCase::ACCESS_READONLY,		"readonly_",	"readonly"	},
	};

	static const bool arraysSized[]	= { true, false };

	for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(arraysSized); ++sizeNdx)
	for (int qualifierNdx = 0; qualifierNdx < DE_LENGTH_OF_ARRAY(qualifiers); ++qualifierNdx)
	{
		const std::string name = std::string() + ((arraysSized[sizeNdx]) ? ("sized_") : ("unsized_")) + qualifiers[qualifierNdx].name + "array";
		const std::string desc = std::string("Test length() of ") + ((arraysSized[sizeNdx]) ? ("sized ") : ("unsized ")) + qualifiers[qualifierNdx].name + " array";

		this->addChild(new SSBOArrayLengthCase(m_context, name.c_str(), desc.c_str(), qualifiers[qualifierNdx].access, arraysSized[sizeNdx]));
	}
}

} // Functional
} // gles31
} // deqp
