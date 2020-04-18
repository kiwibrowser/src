/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 * @file  gl4cVertexAttrib64BitTests.hpp
 * @brief Implement conformance tests for GL_ARB_vertex_attrib_64bit functionality
 **/

#include "gl4cVertexAttrib64BitTest.hpp"

#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

using namespace glw;

namespace VertexAttrib64Bit
{

class Base : public deqp::TestCase
{
public:
	/* Public constructor and destructor */
	Base(deqp::Context& context, const char* name, const char* description);

	virtual ~Base()
	{
	}

	/* Public methods */
	void BuildProgram(const GLchar* fragment_shader_code, GLuint& program_id, const GLchar* vertex_shader_code,
					  GLuint& out_fragment_shader_id, GLuint& out_vertex_shader_id) const;

	void BuildProgramVSOnly(GLuint out_program_id, const GLchar* vertex_shader_code,
							GLuint& out_vertex_shader_id) const;

	void CompileShader(GLuint id, const GLchar* source_code) const;

	GLint GetMaxVertexAttribs() const;

	void IterateStart();

	tcu::TestNode::IterateResult IterateStop(bool result) const;

	void LinkProgram(GLuint id) const;

	static GLdouble RandomDouble(GLdouble min, GLdouble max);

	void RequireExtension(const GLchar* extension_name) const;

	/* Public fields */
	/* Test framework objects */
	glw::Functions gl; /* prefix "m_" ommitted for readability */
	tcu::TestLog&  m_log;
};

/** Constructor
 *
 **/
Base::Base(deqp::Context& context, const char* name, const char* description)
	: TestCase(context, name, description), m_log(m_context.getTestContext().getLog())
{
	/* Nothing to be done here */
}

void Base::BuildProgram(const GLchar* fragment_shader_code, GLuint& program_id, const GLchar* vertex_shader_code,
						GLuint& out_fragment_shader_id, GLuint& out_vertex_shader_id) const
{
	out_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

	out_vertex_shader_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

	CompileShader(out_fragment_shader_id, fragment_shader_code);
	CompileShader(out_vertex_shader_id, vertex_shader_code);

	gl.attachShader(program_id, out_fragment_shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");

	gl.attachShader(program_id, out_vertex_shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");

	LinkProgram(program_id);
}

/** Builds and links a program object consisting only of vertex shader stage.
 *  The function also creates a vertex shader, assigns it user-provided body
 *  and compiles it.
 *
 *  @param program_id           ID of a created program object to configure.
 *  @param vertex_shader_code   Source code to use for the vertex shader.
 *  @param out_vertex_shader_id Will hold
 **/
void Base::BuildProgramVSOnly(GLuint program_id, const GLchar* vertex_shader_code, GLuint& out_vertex_shader_id) const
{
	out_vertex_shader_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

	CompileShader(out_vertex_shader_id, vertex_shader_code);

	gl.attachShader(program_id, out_vertex_shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");

	LinkProgram(program_id);
}

void Base::CompileShader(GLuint id, const GLchar* source_code) const
{
	GLint status = 0;

	gl.shaderSource(id, 1, &source_code, 0 /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");

	gl.compileShader(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CompileShader");

	gl.getShaderiv(id, GL_COMPILE_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

	if (GL_FALSE == status)
	{
		GLint				message_length = 0;
		std::vector<GLchar> message;

		gl.getShaderiv(id, GL_INFO_LOG_LENGTH, &message_length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

		message.resize(message_length + 1);

		gl.getShaderInfoLog(id, message_length, &message_length, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

		m_log << tcu::TestLog::Section("Shader compilation error", "");

		m_log << tcu::TestLog::Message << "Compilation log:\n" << &message[0] << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "Shader source:\n" << source_code << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::EndSection;

		TCU_FAIL("Shader compilation failed");
	}
}

/** Get value of GL_MAX_VERTEX_ATTRIBS
 *
 * Throws exception in case of failure
 *
 * @return Value
 **/
GLint Base::GetMaxVertexAttribs() const
{
	GLint max_vertex_attribs;

	gl.getIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	return max_vertex_attribs;
}

void Base::IterateStart()
{
	gl = m_context.getRenderContext().getFunctions();
}

tcu::TestNode::IterateResult Base::IterateStop(bool result) const
{
	/* Set test result */
	if (false == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

void Base::LinkProgram(GLuint id) const
{
	GLint status = 0;
	gl.linkProgram(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

	gl.getProgramiv(id, GL_LINK_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

	if (GL_FALSE == status)
	{
		GLint				message_length = 0;
		std::vector<GLchar> message;

		gl.getProgramiv(id, GL_INFO_LOG_LENGTH, &message_length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

		message.resize(message_length + 1);

		gl.getProgramInfoLog(id, message_length, &message_length, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

		m_log << tcu::TestLog::Section("Program link error", "");

		m_log << tcu::TestLog::Message << "Link log:\n" << &message[0] << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::EndSection;

		TCU_FAIL("Program linking failed");
	}
}

/** Return "random" double value from range <min:max>
 *
 * @return Value
 **/
GLdouble Base::RandomDouble(GLdouble min, GLdouble max)
{
	static const glw::GLushort max_value = 0x2000;
	static glw::GLushort	   value	 = 0x1234;

	GLdouble	   fraction = ((GLdouble)value) / ((GLdouble)max_value);
	const GLdouble range	= max - min;

	value = static_cast<glw::GLushort>((max_value <= value) ? 0 : value + 1);

	return min + fraction * range;
}

/** Throws tcu::NotSupportedError if requested extensions is not available.
 *
 **/
void Base::RequireExtension(const GLchar* extension_name) const
{
	const std::vector<std::string>& extensions = m_context.getContextInfo().getExtensions();

	if (std::find(extensions.begin(), extensions.end(), extension_name) == extensions.end())
	{
		std::string message = "Required extension is not supported: ";
		message.append(extension_name);

		throw tcu::NotSupportedError(message);
	}
}

/** Implementation of conformance test "1", description follows.
 *
 *  Make sure the following errors are generated as specified:
 *
 *  a) GL_INVALID_VALUE should be generated by:
 *     I.    glVertexAttribL1d     ()
 *     II.   glVertexAttribL2d     ()
 *     III.  glVertexAttribL3d     ()
 *     IV.   glVertexAttribL4d     ()
 *     V.    glVertexAttribL1dv    ()
 *     VI.   glVertexAttribL2dv    ()
 *     VII.  glVertexAttribL3dv    ()
 *     VIII. glVertexAttribL4dv    ()
 *     IX.   glVertexAttribLPointer()
 *
 *     if <index> is greater than or equal to GL_MAX_VERTEX_ATTRIBS;
 *
 *  b) GL_INVALID_ENUM should be generated by glVertexAttribLPointer()
 *     if <type> is not GL_DOUBLE;
 *
 *  c) GL_INVALID_VALUE should be generated by glVertexAttribLPointer()
 *     if <size> is not 1, 2, 3 or 4.
 *
 *  d) GL_INVALID_VALUE should be generated by glVertexAttribLPointer()
 *     if <stride> is negative.
 *
 *  e) GL_INVALID_OPERATION should be generated by glVertexAttribLPointer()
 *     if zero is bound to the GL_ARRAY_BUFFER buffer object binding
 *     point and the <pointer> argument is not NULL.
 *
 *  f) GL_INVALID_OPERATION should be generated by glGetVertexAttribLdv()
 *     if <index> is zero.
 **/
class ApiErrorsTest : public Base
{
public:
	/* Public methods */
	ApiErrorsTest(deqp::Context& context);

	virtual ~ApiErrorsTest()
	{
	}

	/* Public methods inheritated from TestCase */
	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void invalidEnum(bool& result);
	void invalidOperation(bool& result);
	void invalidValue(bool& result);
	void verifyError(GLenum expected_error, const char* function_name, int line_number, bool& result);

	/* Private fields */
	GLuint m_vertex_array_object_id;
};

/** Constructor
 *
 * @param context CTS context instance
 **/
ApiErrorsTest::ApiErrorsTest(deqp::Context& context)
	: Base(context, "api_errors", "Verify that API routines provoke errors as specified"), m_vertex_array_object_id(0)
{
	/* Nothing to be done here */
}

void ApiErrorsTest::deinit()
{
	/* Delete VAO */
	if (0 != m_vertex_array_object_id)
	{
		gl.bindVertexArray(0);
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);
		m_vertex_array_object_id = 0;
	}
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ApiErrorsTest::iterate()
{
	IterateStart();

	bool result = true;

	RequireExtension("GL_ARB_vertex_attrib_64bit");

	gl.genVertexArrays(1, &m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

	gl.bindVertexArray(m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");

	invalidEnum(result);
	invalidOperation(result);
	invalidValue(result);

	/* Done */
	return IterateStop(result);
}

/** Test if GL_INVALID_ENUM error is provoked as expected
 *
 * @param result If test fails result is set to false, not modified otherwise.
 **/
void ApiErrorsTest::invalidEnum(bool& result)
{
	/*
	 *b) GL_INVALID_ENUM should be generated by glVertexAttribLPointer()
	 *   if <type> is not GL_DOUBLE;
	 */

	static const GLenum type_array[] = { GL_BYTE,
										 GL_UNSIGNED_BYTE,
										 GL_SHORT,
										 GL_UNSIGNED_SHORT,
										 GL_INT,
										 GL_UNSIGNED_INT,
										 GL_HALF_FLOAT,
										 GL_FLOAT,
										 GL_FIXED,
										 GL_INT_2_10_10_10_REV,
										 GL_UNSIGNED_INT_2_10_10_10_REV,
										 GL_UNSIGNED_INT_10F_11F_11F_REV };
	static const GLuint type_array_length = sizeof(type_array) / sizeof(type_array[0]);

	for (GLuint i = 0; i < type_array_length; ++i)
	{
		const GLenum type = type_array[i];

		std::stringstream message;
		message << "VertexAttribLPointer(..., " << glu::getTypeName(type) << " /* type */, ...)";

		gl.vertexAttribLPointer(1 /*index */, 4 /*size */, type, 0 /* stride */, 0 /* pointer */);
		verifyError(GL_INVALID_ENUM, message.str().c_str(), __LINE__, result);
	}

	gl.vertexAttribLPointer(1 /* index */, 4 /*size */, GL_DOUBLE, 0 /* stride */, 0 /* pointer */);
	verifyError(GL_NO_ERROR, "VertexAttribLPointer(..., GL_DOUBLE /* type */, ...)", __LINE__, result);
}

/** Test if GL_INVALID_OPERATON error is provoked as expected
 *
 * @param result If test fails result is set to false, not modified otherwise.
 **/
void ApiErrorsTest::invalidOperation(bool& result)
{

	/*
	 *e) GL_INVALID_OPERATION should be generated by glVertexAttribLPointer()
	 *   if zero is bound to the GL_ARRAY_BUFFER buffer object binding
	 *   point and the <pointer> argument is not NULL.
	 */
	static const GLvoid* pointer_array[]	  = { (GLvoid*)1, (GLvoid*)4, (GLvoid*)-16 };
	static const GLuint  pointer_array_length = sizeof(pointer_array) / sizeof(pointer_array[0]);

	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	for (GLuint i = 0; i < pointer_array_length; ++i)
	{
		const GLvoid* pointer = pointer_array[i];

		std::stringstream message;
		message << "VertexAttribLPointer(..., " << pointer << " /* pointer */)";

		gl.vertexAttribLPointer(1 /* index */, 4 /*size */, GL_DOUBLE, 0 /* stride */, pointer);
		verifyError(GL_INVALID_OPERATION, message.str().c_str(), __LINE__, result);
	}
}

/** Test if GL_INVALID_VALUE error is provoked as expected
 *
 * @param result If test fails result is set to false, not modified otherwise.
 **/
void ApiErrorsTest::invalidValue(bool& result)
{
	GLint		   max_vertex_attribs = GetMaxVertexAttribs();
	const GLdouble vector[4]		  = { 0.0, 0.0, 0.0, 0.0 };

	/*
	 * a) GL_INVALID_VALUE should be generated by:
	 *    I.    glVertexAttribL1d     ()
	 *    II.   glVertexAttribL2d     ()
	 *    III.  glVertexAttribL3d     ()
	 *    IV.   glVertexAttribL4d     ()
	 *    V.    glVertexAttribL1dv    ()
	 *    VI.   glVertexAttribL2dv    ()
	 *    VII.  glVertexAttribL3dv    ()
	 *    VIII. glVertexAttribL4dv    ()
	 *    IX.   glVertexAttribLPointer()
	 *
	 *    if <index> is greater than or equal to GL_MAX_VERTEX_ATTRIBS;
	 */
	gl.vertexAttribL1d(max_vertex_attribs, 0.0);
	verifyError(GL_INVALID_VALUE, "VertexAttribL1d(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribL1d(max_vertex_attribs + 1, 0.0);
	verifyError(GL_INVALID_VALUE, "VertexAttribL1d(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	gl.vertexAttribL2d(max_vertex_attribs, 0.0, 0.0);
	verifyError(GL_INVALID_VALUE, "VertexAttribL2d(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribL2d(max_vertex_attribs + 1, 0.0, 0.0);
	verifyError(GL_INVALID_VALUE, "VertexAttribL2d(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	gl.vertexAttribL3d(max_vertex_attribs, 0.0, 0.0, 0.0);
	verifyError(GL_INVALID_VALUE, "VertexAttribL3d(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribL3d(max_vertex_attribs + 1, 0.0, 0.0, 0.0);
	verifyError(GL_INVALID_VALUE, "VertexAttribL3d(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	gl.vertexAttribL4d(max_vertex_attribs, 0.0, 0.0, 0.0, 0.0);
	verifyError(GL_INVALID_VALUE, "VertexAttribL4d(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribL4d(max_vertex_attribs + 1, 0.0, 0.0, 0.0, 0.0);
	verifyError(GL_INVALID_VALUE, "VertexAttribL4d(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	gl.vertexAttribL1dv(max_vertex_attribs, vector);
	verifyError(GL_INVALID_VALUE, "VertexAttribL1dv(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribL1dv(max_vertex_attribs + 1, vector);
	verifyError(GL_INVALID_VALUE, "VertexAttribL1dv(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	gl.vertexAttribL2dv(max_vertex_attribs, vector);
	verifyError(GL_INVALID_VALUE, "VertexAttribL2dv(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribL2dv(max_vertex_attribs + 1, vector);
	verifyError(GL_INVALID_VALUE, "VertexAttribL2dv(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	gl.vertexAttribL3dv(max_vertex_attribs, vector);
	verifyError(GL_INVALID_VALUE, "VertexAttribL3dv(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribL3dv(max_vertex_attribs + 1, vector);
	verifyError(GL_INVALID_VALUE, "VertexAttribL3dv(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	gl.vertexAttribL4dv(max_vertex_attribs, vector);
	verifyError(GL_INVALID_VALUE, "VertexAttribL4dv(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribL4dv(max_vertex_attribs + 1, vector);
	verifyError(GL_INVALID_VALUE, "VertexAttribL4dv(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	gl.vertexAttribLPointer(max_vertex_attribs, 4 /*size */, GL_DOUBLE, 0 /* stride */, 0 /* pointer */);
	verifyError(GL_INVALID_VALUE, "VertexAttribLPointer(GL_MAX_VERTEX_ATTRIBS, ...)", __LINE__, result);

	gl.vertexAttribLPointer(max_vertex_attribs + 1, 4 /*size */, GL_DOUBLE, 0 /* stride */, 0 /* pointer */);
	verifyError(GL_INVALID_VALUE, "VertexAttribLPointer(GL_MAX_VERTEX_ATTRIBS + 1, ...)", __LINE__, result);

	/*
	 *c) GL_INVALID_VALUE should be generated by glVertexAttribLPointer()
	 *if <size> is not 1, 2, 3 or 4.
	 */
	gl.vertexAttribLPointer(1, 0 /*size */, GL_DOUBLE, 0 /* stride */, 0 /* pointer */);
	verifyError(GL_INVALID_VALUE, "VertexAttribLPointer(..., 0 /* size */, ...)", __LINE__, result);

	gl.vertexAttribLPointer(1, 5 /*size */, GL_DOUBLE, 0 /* stride */, 0 /* pointer */);
	verifyError(GL_INVALID_VALUE, "VertexAttribLPointer(..., 5 /* size */, ...)", __LINE__, result);

	/*
	 *d) GL_INVALID_VALUE should be generated by glVertexAttribLPointer()
	 *   if <stride> is negative.
	 */
	gl.vertexAttribLPointer(1, 4 /*size */, GL_DOUBLE, -1 /* stride */, 0 /* pointer */);
	verifyError(GL_INVALID_VALUE, "VertexAttribLPointer(..., -1 /* stride */, ...)", __LINE__, result);

	gl.vertexAttribLPointer(1, 4 /*size */, GL_DOUBLE, -4 /* stride */, 0 /* pointer */);
	verifyError(GL_INVALID_VALUE, "VertexAttribLPointer(..., -4 /* stride */, ...)", __LINE__, result);
}

/** Verify that GetError returns expected error code. In case of failure logs error message.
 *
 * @param expected_error Expected error code
 * @param function_name  Name of function to log in case of error
 * @param line_number    Line number, for reference
 * @param result         Result of verification, set to false in case of failure, not modified otherwise
 **/
void ApiErrorsTest::verifyError(GLenum expected_error, const char* function_name, int line_number, bool& result)
{
	GLenum error = gl.getError();

	if (expected_error != error)
	{
		m_log << tcu::TestLog::Section("Error", "");

		m_log << tcu::TestLog::Message << "GetError returned: " << glu::getErrorStr(error) << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "Expected: " << glu::getErrorStr(expected_error) << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "Operation: " << function_name << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "File: " << __FILE__ << "@" << line_number << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::EndSection;

		result = false;
	}
}

/** Implementation of conformance test "2", description follows.
 *
 *  Make sure that all available generic vertex attributes report
 *  correct values when queried with corresponding glGetVertexAttribL*()
 *  function, after they had been set with a glVertexAttribL*() call.
 *  All double-precision floating-point setters and getters should
 *  be checked, as enlisted below:
 *
 *  * glVertexAttribL1d ()
 *  * glVertexAttribL2d ()
 *  * glVertexAttribL3d ()
 *  * glVertexAttribL4d ()
 *  * glVertexAttribL1dv()
 *  * glVertexAttribL2dv()
 *  * glVertexAttribL3dv()
 *  * glVertexAttribL4dv()
 *
 *  The test should also verify glGetVertexAttribiv() and
 *  glGetVertexAttribLdv() report correct property values for all
 *  vertex attribute arrays configured with glVertexAttribLPointer()
 *  call. Two different configurations should be checked for each
 *  VAA index.
 **/
class GetVertexAttribTest : public Base
{
public:
	/* Public constructor and destructor */
	GetVertexAttribTest(deqp::Context& context);

	virtual ~GetVertexAttribTest()
	{
	}

	/* Public methods inheritated from TestCase */
	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private types */
	/** Template class to store vertex attribute data
	 *
	 * @tparam SIZE Number of elements
	 **/
	template <GLuint SIZE>
	class vertexAttribute
	{
	public:
		vertexAttribute(GLdouble min, GLdouble max)
		{
			for (GLuint i = 0; i < SIZE; ++i)
			{
				m_array[i] = RandomDouble(min, max);
			}
		}

		GLdouble m_array[SIZE];
	};

	/* Private methods */
	/* checkVertexAttrib methods */
	template <GLuint SIZE>
	void checkVertexAttribLd(GLuint index, bool& result) const;

	template <GLuint SIZE>
	void checkVertexAttribLdv(GLuint index, bool& result) const;

	void checkVertexAttribLPointer(GLuint index, bool& result) const;

	/* Wrappers for vertexAttribLd routines */
	template <GLuint SIZE>
	void vertexAttribLd(GLuint index, const vertexAttribute<SIZE>& attribute) const;

	template <GLuint SIZE>
	void vertexAttribLdv(GLuint index, const vertexAttribute<SIZE>& attribute) const;

	/* Utilities */
	bool compareDoubles(const GLdouble* a, const GLdouble* b, GLuint length) const;

	void initTest();

	bool verifyResults(GLuint index, GLenum pname, GLint expected_value) const;

	bool verifyResults(const GLdouble* set_values, GLuint length, GLuint index, const char* function_name,
					   int line_number) const;

	bool verifyPointerResults(const GLdouble* set_values, GLuint length, GLuint index, int line_number) const;

	void logError(const GLdouble* set_values, const GLdouble* get_values, GLuint length, const char* function_name,
				  GLuint index, int line_number) const;

	/* Private fields */
	const GLdouble		m_epsilon;
	static const GLuint m_n_iterations = 128;
	GLint				m_max_vertex_attribs;
	const GLdouble		m_min;
	const GLdouble		m_max;

	/* GL objects */
	GLuint m_buffer_object_id;
	GLuint m_vertex_array_object_id;
};

/** Constructor
 *
 * @param context CTS context
 **/
GetVertexAttribTest::GetVertexAttribTest(deqp::Context& context)
	: Base(context, "get_vertex_attrib", "Verify that GetVertexAttribL* routines")
	, m_epsilon(0.0)
	, m_max_vertex_attribs(0)
	, m_min(-16.384)
	, m_max(16.384)
	, m_buffer_object_id(0)
	, m_vertex_array_object_id(0)
{
	/* Nothing to be done */
}

/** Clean up after test
 *
 **/
void GetVertexAttribTest::deinit()
{
	if (0 != m_buffer_object_id)
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		gl.deleteBuffers(1, &m_buffer_object_id);
		m_buffer_object_id = 0;
	}

	if (0 != m_vertex_array_object_id)
	{
		gl.bindVertexArray(0);
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);
		m_vertex_array_object_id = 0;
	}
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult GetVertexAttribTest::iterate()
{
	IterateStart();

	bool result = true;

	RequireExtension("GL_ARB_vertex_attrib_64bit");

	initTest();

	for (GLint i = 1; i < m_max_vertex_attribs; ++i)
	{
		checkVertexAttribLd<1>(i, result);
		checkVertexAttribLd<2>(i, result);
		checkVertexAttribLd<3>(i, result);
		checkVertexAttribLd<4>(i, result);
		checkVertexAttribLdv<1>(i, result);
		checkVertexAttribLdv<2>(i, result);
		checkVertexAttribLdv<3>(i, result);
		checkVertexAttribLdv<4>(i, result);
		checkVertexAttribLPointer(i, result);
	}

	/* Done */
	return IterateStop(result);
}

/** Verifies glVertexAttribLd routines
 *
 * @tparam SIZE Size of vertex attribute
 *
 * @param index  Index of vertex attribute, starts from 1.
 * @param result Result of verification, set to false in case of failure, not modified otherwise.
 **/
template <GLuint SIZE>
void GetVertexAttribTest::checkVertexAttribLd(GLuint index, bool& result) const
{
	std::stringstream function_name;

	function_name << "VertexAttribL" << SIZE << "d";

	for (GLuint i = 0; i < m_n_iterations; ++i)
	{
		vertexAttribute<SIZE> vertex_attribute(m_min, m_max);

		vertexAttribLd<SIZE>(index, vertex_attribute);
		GLU_EXPECT_NO_ERROR(gl.getError(), function_name.str().c_str());

		if (false == verifyResults(vertex_attribute.m_array, SIZE, index, function_name.str().c_str(), __LINE__))
		{
			result = false;
			return;
		}
	}
}

/** Verifies glVertexAttribLdv routines
 *
 * @tparam SIZE Size of vertex attribute
 *
 * @param index  Index of vertex attribute, starts from 1.
 * @param result Result of verification, set to false in case of failure, not modified otherwise.
 **/
template <GLuint SIZE>
void GetVertexAttribTest::checkVertexAttribLdv(GLuint index, bool& result) const
{
	std::stringstream function_name;

	function_name << "VertexAttribL" << SIZE << "dv";

	for (GLuint i = 0; i < m_n_iterations; ++i)
	{
		vertexAttribute<SIZE> vertex_attribute(m_min, m_max);

		vertexAttribLdv<SIZE>(index, vertex_attribute);
		GLU_EXPECT_NO_ERROR(gl.getError(), function_name.str().c_str());

		if (false == verifyResults(vertex_attribute.m_array, SIZE, index, function_name.str().c_str(), __LINE__))
		{
			result = false;
			return;
		}
	}
}

/** Verifies glVertexAttribLPointer
 *
 * @param index  Index of vertex attribute, starts from 1.
 * @param result Result of verification, set to false in case of failure, not modified otherwise.
 **/
void GetVertexAttribTest::checkVertexAttribLPointer(GLuint index, bool& result) const
{
	static const GLuint max_size   = 4;
	static const GLuint max_stride = 16;

	gl.bindVertexArray(m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	for (GLuint size = 1; size <= max_size; ++size)
	{
		for (GLuint stride = 0; stride < max_stride; ++stride)
		{
			gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

			gl.vertexAttribLPointer(index, size, GL_DOUBLE, stride, (GLvoid*)0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribLPointer");

			gl.bindBuffer(GL_ARRAY_BUFFER, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

			if (false == verifyResults(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, m_buffer_object_id))
			{
				result = false;
			}

			if (false == verifyResults(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, GL_FALSE))
			{
				result = false;
			}

			if (false == verifyResults(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, size))
			{
				result = false;
			}

			if (false == verifyResults(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, stride))
			{
				result = false;
			}

			if (false == verifyResults(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, GL_DOUBLE))
			{
				result = false;
			}

			if (false == verifyResults(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, GL_FALSE))
			{
				result = false;
			}

			if (false == verifyResults(index, GL_VERTEX_ATTRIB_ARRAY_INTEGER, GL_FALSE))
			{
				result = false;
			}

			if (false == verifyResults(index, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, 0))
			{
				result = false;
			}
		}
	}
}

/** Wrapper of vertexAttribLd routines.
 *
 * @tparam SIZE Size of vertex attribute. Specialisation for 1.
 *
 * @param index     Index parameter
 * @param attribute Vertex attribute data are taken from provided instance of vertexAttribute
 **/
template <>
void GetVertexAttribTest::vertexAttribLd<1>(GLuint										   index,
											const GetVertexAttribTest::vertexAttribute<1>& attribute) const
{
	gl.vertexAttribL1d(index, attribute.m_array[0]);
}

/** Wrapper of vertexAttribLd routines.
 *
 * @tparam SIZE Size of vertex attribute. Specialisation for 2.
 *
 * @param index     Index parameter
 * @param attribute Vertex attribute data are taken from provided instance of vertexAttribute
 **/
template <>
void GetVertexAttribTest::vertexAttribLd<2>(GLuint										   index,
											const GetVertexAttribTest::vertexAttribute<2>& attribute) const
{
	gl.vertexAttribL2d(index, attribute.m_array[0], attribute.m_array[1]);
}

/** Wrapper of vertexAttribLd routines.
 *
 * @tparam SIZE Size of vertex attribute. Specialisation for 3.
 *
 * @param index     Index parameter
 * @param attribute Vertex attribute data are taken from provided instance of vertexAttribute
 **/
template <>
void GetVertexAttribTest::vertexAttribLd<3>(GLuint										   index,
											const GetVertexAttribTest::vertexAttribute<3>& attribute) const
{
	gl.vertexAttribL3d(index, attribute.m_array[0], attribute.m_array[1], attribute.m_array[2]);
}

/** Wrapper of vertexAttribLd routines.
 *
 * @tparam SIZE Size of vertex attribute. Specialisation for 4.
 *
 * @param index     Index parameter
 * @param attribute Vertex attribute data are taken from provided instance of vertexAttribute
 **/
template <>
void GetVertexAttribTest::vertexAttribLd<4>(GLuint										   index,
											const GetVertexAttribTest::vertexAttribute<4>& attribute) const
{
	gl.vertexAttribL4d(index, attribute.m_array[0], attribute.m_array[1], attribute.m_array[2], attribute.m_array[3]);
}

/** Wrapper of vertexAttribLdv routines.
 *
 * @tparam SIZE Size of vertex attribute. Specialisation for 1.
 *
 * @param index     Index parameter
 * @param attribute Vertex attribute data are taken from provided instance of vertexAttribute
 **/
template <>
void GetVertexAttribTest::vertexAttribLdv<1>(GLuint											index,
											 const GetVertexAttribTest::vertexAttribute<1>& attribute) const
{
	gl.vertexAttribL1dv(index, attribute.m_array);
}

/** Wrapper of vertexAttribLdv routines.
 *
 * @tparam SIZE Size of vertex attribute. Specialisation for 2.
 *
 * @param index     Index parameter
 * @param attribute Vertex attribute data are taken from provided instance of vertexAttribute
 **/
template <>
void GetVertexAttribTest::vertexAttribLdv<2>(GLuint											index,
											 const GetVertexAttribTest::vertexAttribute<2>& attribute) const
{
	gl.vertexAttribL2dv(index, attribute.m_array);
}

/** Wrapper of vertexAttribLdv routines.
 *
 * @tparam SIZE Size of vertex attribute. Specialisation for 3.
 *
 * @param index     Index parameter
 * @param attribute Vertex attribute data are taken from provided instance of vertexAttribute
 **/
template <>
void GetVertexAttribTest::vertexAttribLdv<3>(GLuint											index,
											 const GetVertexAttribTest::vertexAttribute<3>& attribute) const
{
	gl.vertexAttribL3dv(index, attribute.m_array);
}

/** Wrapper of vertexAttribLdv routines.
 *
 * @tparam SIZE Size of vertex attribute. Specialisation for 4.
 *
 * @param index     Index parameter
 * @param attribute Vertex attribute data are taken from provided instance of vertexAttribute
 **/
template <>
void GetVertexAttribTest::vertexAttribLdv<4>(GLuint											index,
											 const GetVertexAttribTest::vertexAttribute<4>& attribute) const
{
	gl.vertexAttribL4dv(index, attribute.m_array);
}

/** Compare two arrays of doubles
 *
 * @param a      First array of doubles
 * @param b      Second array of doubles
 * @param length Length of arrays
 *
 * @return true if arrays are considered equal, false otherwise
 **/
bool GetVertexAttribTest::compareDoubles(const GLdouble* a, const GLdouble* b, GLuint length) const
{
	for (GLuint i = 0; i < length; ++i)
	{
		if ((b[i] > a[i] + m_epsilon) || (b[i] < a[i] - m_epsilon))
		{
			return false;
		}
	}

	return true;
}

/** Prepare buffer and vertex array object, get max vertex attributes
 *
 **/
void GetVertexAttribTest::initTest()
{
	gl.genBuffers(1, &m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	gl.bufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLdouble), 0, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferStorage");

	gl.genVertexArrays(1, &m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

	m_max_vertex_attribs = GetMaxVertexAttribs();
}

/** Logs message informing that values got with GetVertexAttribLdv do not match set with "function_name"
 *
 * @param set_values    Values set with "function_name"
 * @param get_values    Values extracted with GetVertexAttribLdv
 * @param length        Length of "get/set_values" arrays
 * @param function_name Name of function used to set vertex attributes
 * @param index         Index of vertex attribute
 * @param line_number   Line number refereing to location of "function_name"
 **/
void GetVertexAttribTest::logError(const GLdouble* set_values, const GLdouble* get_values, GLuint length,
								   const char* function_name, GLuint index, int line_number) const
{
	m_log << tcu::TestLog::Section("Error", "");

	tcu::MessageBuilder message = m_log << tcu::TestLog::Message;
	message << "Values set with " << function_name << " [";

	for (GLuint i = 0; i < length; ++i)
	{
		message << std::setprecision(24) << set_values[i];

		if (length != i + 1)
		{
			message << ", ";
		}
	}

	message << "]" << tcu::TestLog::EndMessage;

	message = m_log << tcu::TestLog::Message;
	message << "Values got with GetVertexAttribLdv"
			<< " [";

	for (GLuint i = 0; i < length; ++i)
	{
		message << std::setprecision(24) << get_values[i];

		if (length != i + 1)
		{
			message << ", ";
		}
	}

	message << "]" << tcu::TestLog::EndMessage;

	m_log << tcu::TestLog::Message << "Index: " << index << tcu::TestLog::EndMessage;

	m_log << tcu::TestLog::Message << "File: " << __FILE__ << "@" << line_number << tcu::TestLog::EndMessage;

	m_log << tcu::TestLog::EndSection;
}

/** Verify results of vertexAttribLPointer
 *
 * @param index          Index of vertex attribute
 * @param pname          Parameter name to be querried with getVertexAttribiv and getVertexAttribLdv
 * @param expected_value Expected valued
 *
 * @return true if Results match expected_value, false otherwise
 **/
bool GetVertexAttribTest::verifyResults(GLuint index, GLenum pname, GLint expected_value) const
{
	GLint	params_getVertexAttribiv  = 0;
	GLdouble params_getVertexAttribLdv = 0.0;

	gl.getVertexAttribiv(index, pname, &params_getVertexAttribiv);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetVertexAttribiv");

	gl.getVertexAttribLdv(index, pname, &params_getVertexAttribLdv);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetVertexAttribLdv");

	if ((expected_value != params_getVertexAttribiv) || (expected_value != params_getVertexAttribLdv))
	{
		m_log << tcu::TestLog::Section("Error", "");

		m_log << tcu::TestLog::Message << "GetVertexAttribiv(" << index << "/* index */, "
			  << glu::getVertexAttribParameterNameName(pname) << "/* pname */)" << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "Result: " << params_getVertexAttribiv << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "GetVertexAttribLdv(" << index << "/* index */, "
			  << glu::getVertexAttribParameterNameName(pname) << "/* pname */)" << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "Result: " << params_getVertexAttribLdv << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "Expected: " << expected_value << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::Message << "File: " << __FILE__ << "@" << __LINE__ << tcu::TestLog::EndMessage;

		m_log << tcu::TestLog::EndSection;

		return false;
	}

	return true;
}

/** Verify results of vertexAttribLdv routines
 *
 * @param set_values    Values set with vertexAttribLdv
 * @param length        Length of "set_values" array
 * @param index         Index of vertex attribute
 * @param function_name Name of function used to set, it will be used for error logging
 * @param line_number   Line number refering to location of "function_name", used to log errors
 *
 * @return true if results match set values, false otherwise
 **/
bool GetVertexAttribTest::verifyResults(const GLdouble* set_values, GLuint length, GLuint index,
										const char* function_name, int line_number) const
{
	GLdouble results[4] = { 0.0, 0.0, 0.0, 0.0 };

	gl.getVertexAttribLdv(index, GL_CURRENT_VERTEX_ATTRIB, results);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetVertexAttribLdv");

	if (false == compareDoubles(set_values, results, length))
	{
		logError(set_values, results, length, function_name, index, line_number);

		return false;
	}

	return true;
}

/** Implementation of conformance test "3", description follows.
 *
 *  Verify that a total of GL_MAX_VERTEX_ATTRIBS double and dvec2,
 *  (GL_MAX_VERTEX_ATTRIBS / 2) dvec3, dvec4 and dmat2,
 *  (GL_MAX_VERTEX_ATTRIBS / 3) dmat3x2,
 *  (GL_MAX_VERTEX_ATTRIBS / 4) dmat4x2, dmat2x3 and dmat2x4,
 *  (GL_MAX_VERTEX_ATTRIBS / 6) dmat3 and dmat3x4,
 *  (GL_MAX_VERTEX_ATTRIBS / 8) dmat4x3 and dmat4,
 *  attributes can be used in each shader stage at the same time.
 *
 *  The test should run in 7 iterations:
 *
 *  a) In the first iteration, (GL_MAX_VERTEX_ATTRIBS / 2) double
 *      attributes and (GL_MAX_VERTEX_ATTRIBS / 2) dvec2 attributes
 *      should be defined in a vertex shader. The test should verify
 *      the values exposed by these attributes and write 1 to an
 *      output variable if all attribute values are found to be
 *      correct, or set it to 0 if at least one of the retrieved
 *      values is found invalid.
 *
 *      Double attributes should be assigned the value:
 *                  (n_attribute + gl_VertexID * 2)
 *
 *      Dvec2 attribute components should be assigned the following
 *      vector values:
 *                  (n_attribute + gl_VertexID * 3 + 1,
 *                  n_attribute + gl_VertexID * 3 + 2)
 *
 *  b) In the second iteration, (GL_MAX_VERTEX_ATTRIBS / 4) dvec3
 *      and (GL_MAX_VERTEX_ATTRIBS / 4) dvec4 attributes should be
 *      defined in a vertex shader. Verification of the data exposed
 *      by these input variables should be performed as in step a),
 *      with an exception of the values passed through the attributes.
 *
 *      Dvec3 attribute components should be assigned the following
 *      vector values:
 *                  (n_attribute + gl_VertexID * 3 + 0,
 *                  n_attribute + gl_VertexID * 3 + 1,
 *                  n_attribute + gl_VertexID * 3 + 2).
 *
 *      Dvec4 attribute components should be assigned the following
 *      vector values:
 *                  (n_attribute + gl_VertexID * 4 + 0,
 *                  n_attribute + gl_VertexID * 4 + 1,
 *                  n_attribute + gl_VertexID * 4 + 2,
 *                  n_attribute + gl_VertexID * 4 + 3).
 *
 *      n_attribute corresponds to the ordinal number of each attribute,
 *      as defined in the shader.
 *
 *  c) In the third iteration, (GL_MAX_VERTEX_ATTRIBS / 2) dmat2 attributes
 *      should be defined in a vertex shader. Verification of the data exposed
 *      by these input variables should be performed as in step a), with an
 *      exception of the values passed through the attributes.
 *
 *      Subsequent matrix elements should be assigned the following value:
 *              (n_type + n_attribute + gl_VertexID * 16 + n_value)
 *
 *      n_type corresponds to the ordinal number of type as per the
 *      order at the beginning of the paragraph.
 *      n_value corresponds to the ordinal number of the element.
 *
 *  d) In the fourth iteration, (GL_MAX_VERTEX_ATTRIBS / 8) dmat3x2 and
 *      (GL_MAX_VERTEX_ATTRIBS / 8) dmat4x2 attributes should be defined in a
 *      vertex shader. Verification of the data exposed by these input
 *      variables should be performed as in step a), with an exception of the
 *      values passed through the attributes.
 *
 *      Use the same element values as in step c)
 *
 *  e) In the fifth iteration, (GL_MAX_VERTEX_ATTRIBS / 8) dmat2x3 and
 *      (GL_MAX_VERTEX_ATTRIBS / 8) dmat2x4 attributes should be defined in a
 *      vertex shader. Verification of the data exposed by these input
 *      variables should be performed as in step a), with an exception of the
 *      values passed through the attributes.
 *
 *      Use the same element values as in step c)
 *
 *  f) In the sixth iteration, (GL_MAX_VERTEX_ATTRIBS / 12) dmat3 and
 *      (GL_MAX_VERTEX_ATTRIBS / 12) dmat3x4 attributes should be defined in a
 *      vertex shader. Verification of the data exposed by these input
 *      variables should be performed as in step a), with an exception of the
 *      values passed through the attributes.
 *
 *      Use the same element values as in step c)
 *
 *  g) In the seventh iteration, (GL_MAX_VERTEX_ATTRIBS / 16) dmat4x3 and
 *      (GL_MAX_VERTEX_ATTRIBS / 16) dmat4 attributes should be defined in a
 *      vertex shader. Verification of the data exposed by these input
 *      variables should be performed as in step a), with an exception of the
 *      values passed through the attributes.
 *
 *      Use the same element values as in step c)
 *
 *  h) Modify the language of cases a) - g), so that instead of separate
 *      attributes, all attributes of the same type are now a single arrayed
 *      attribute.
 *
 *  Vertex shaders from both iterations should be used to form two program
 *  objects. 1024 vertices should be used for a non-indiced GL_POINTS
 *  draw call, made using those two programs.
 *
 *  All glVertexAttribL*() and glVertexAttribLPointer() should be used for
 *  the purpose of the test. The following draw call API functions should be
 *  tested:
 *
 *  a) glDrawArrays()
 *  b) glDrawArraysInstanced(), primcount > 1, zero vertex attrib divisor
 *  c) glDrawArraysInstanced(), primcount > 1, non-zero vertex attrib divisor
 *  d) glDrawElements()
 *  e) glDrawElementsInstanced(), properties as in b)
 *  f) glDrawElementsInstanced(), properties as in c)
 *
 *  All shaders used by the test should come in two flavors:
 *
 *  - one where attribute locations are explicitly defined in the body;
 *  - the other one where attribute locations are to be assigned by
 *      the compiler.
 *
 *  For each shader, the test should make sure that all attributes have
 *  been assigned correct amount of locations. (eg: dvec4 attribute
 *  should be granted exactly one location).
 *
 *  Data stored in output variables should be XFBed to the test.
 *  The test passes if the retrieved values are found to be valid
 *  for all vertex shader invocations.
 **/

class LimitTest : public Base
{
public:
	/* Public constructor and destructor */
	LimitTest(deqp::Context& context);

	virtual ~LimitTest()
	{
	}

	/* Public methods inheritated from TestCase */
	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private types */
	class programInfo
	{
	public:
		programInfo(const glw::Functions& gl);
		~programInfo();

		GLuint m_fragment_shader_id;
		GLuint m_program_id;
		GLuint m_vertex_shader_id;

	private:
		const glw::Functions& gl;
	};

	struct attributeConfiguration
	{
		attributeConfiguration()
			: m_n_attributes_per_group(0)
			, m_n_elements(0)
			, m_n_rows(0)
			, m_n_types(0)
			, m_type_names(0)
			, m_vertex_length(0)
		{
			/* nothing to be done */
		}

		GLint				 m_n_attributes_per_group;
		const GLint*		 m_n_elements;
		const GLint*		 m_n_rows;
		GLint				 m_n_types;
		const GLchar* const* m_type_names;
		GLint				 m_vertex_length;
	};

	typedef GLint _varyingType;

	/* Private enums */
	enum _iteration
	{
		DOUBLE_DVEC2,	// 1 + 1         = 2
		DVEC3_DVEC4,	 // 2 + 2         = 4
		DMAT2,			 // 2 * 1         = 2
		DMAT3X2_DMAT4X2, // 3 * 1 + 4 * 1 = 8
		DMAT2X3_DMAT2X4, // 2 * 2 + 2 * 2 = 8
		DMAT3_DMAT3X4,   // 3 * 2 + 3 * 2 = 12
		DMAT4X3_DMAT4	// 4 * 2 + 4 * 2 = 16
	};

	enum _attributeType
	{
		REGULAR,
		PER_INSTANCE,
		CONSTANT,
	};

	/*Private methods */
	GLint calculateAttributeGroupOffset(const attributeConfiguration& configuration, GLint index) const;

	GLint calculateAttributeLocation(const attributeConfiguration& configuration, GLint attribute, GLint n_type) const;

	void calculateVertexLength(attributeConfiguration& configuration) const;

	void configureAttribute(_iteration iteration, const attributeConfiguration& configuration, GLint n_type,
							GLuint program_id, bool use_arrays, bool use_vertex_array) const;

	void getProgramDetails(_iteration iteration, bool use_arrays, bool use_locations, bool use_vertex_attrib_divisor,
						   std::string& out_varying_name, std::string& out_vertex_shader_code) const;

	void getVertexArrayConfiguration(_iteration iteration, attributeConfiguration& out_configuration) const;

	void logTestIterationAndConfig(_iteration iteration, _attributeType attribute_type, bool use_arrays,
								   bool use_locations) const;

	void prepareProgram(_iteration iteration, bool use_arrays, bool use_locations, bool use_vertex_attrib_divisor,
						programInfo& programInfo);

	void prepareVertexArray(_iteration iteration, _attributeType attribute_type, GLuint program_id,
							bool use_arrays) const;

	void prepareVertexArrayBuffer(_iteration iteration);

	void setAttributes(_iteration iteration, const attributeConfiguration& configuration, GLuint vertex,
					   std::vector<GLdouble>& out_buffer_data) const;

	void setAttributes_a(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
						 std::vector<GLdouble>& out_buffer_data) const;

	void setAttributes_a_scalar(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
								std::vector<GLdouble>& out_buffer_data) const;

	void setAttributes_a_vec(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
							 std::vector<GLdouble>& out_buffer_data) const;

	void setAttributes_b(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
						 std::vector<GLdouble>& out_buffer_dataa) const;

	void setAttributes_c(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
						 std::vector<GLdouble>& out_buffer_data) const;

	bool testDrawArrays() const;
	bool testDrawArraysInstanced() const;
	bool testDrawElements() const;
	bool testDrawElementsInstanced() const;
	void testInit();
	bool testIteration(_iteration iteration);

	bool testProgram(_iteration iteration, GLuint program_id, bool use_arrays) const;

	bool testProgramWithConstant(_iteration iteration, GLuint program_id, bool use_arrays) const;

	bool testProgramWithDivisor(_iteration iteration, GLuint program_id, bool use_arrays) const;

	bool verifyResult(bool use_instancing) const;

	/* Private fields */
	/* Constants */
	static const GLint  m_array_attribute = -1;
	static const GLuint m_n_instances	 = 16;
	static const GLuint m_n_varyings	  = 1;
	static const GLuint m_n_vertices	  = 1024;
	static const GLuint m_transform_feedback_buffer_size =
		sizeof(_varyingType) * m_n_instances * m_n_vertices * m_n_varyings;

	/* GL objects */
	GLuint m_element_array_buffer_id;
	GLuint m_transoform_feedback_buffer_id;
	GLuint m_vertex_array_buffer_id;
	GLuint m_vertex_array_object_id;
};

/** Constructor
 *
 **/
LimitTest::programInfo::programInfo(const glw::Functions& gl_functions)
	: m_fragment_shader_id(0), m_program_id(0), m_vertex_shader_id(0), gl(gl_functions)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
LimitTest::programInfo::~programInfo()
{
	if (0 != m_program_id)
	{
		gl.deleteProgram(m_program_id);
		m_program_id = 0;
	}

	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);
		m_fragment_shader_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);
		m_vertex_shader_id = 0;
	}
}

/** Constructor
 *
 * @param context CTS context
 **/
LimitTest::LimitTest(deqp::Context& context)
	: Base(context, "limits_test", "Verify that maximum allowed number of attribiutes can be used")
	, m_element_array_buffer_id(0)
	, m_transoform_feedback_buffer_id(0)
	, m_vertex_array_buffer_id(0)
	, m_vertex_array_object_id(0)
{
	/* Nothing to be done here */
}

/** Clean up after test
 *
 **/
void LimitTest::deinit()
{
	/* Restore default settings */
	if (0 != gl.disable)
	{
		gl.disable(GL_RASTERIZER_DISCARD);
	}

	/* Delete GL objects */
	if (0 != m_element_array_buffer_id)
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		gl.deleteBuffers(1, &m_element_array_buffer_id);
		m_element_array_buffer_id = 0;
	}

	if (0 != m_transoform_feedback_buffer_id)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
		gl.deleteBuffers(1, &m_transoform_feedback_buffer_id);
		m_transoform_feedback_buffer_id = 0;
	}

	if (0 != m_vertex_array_buffer_id)
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		gl.deleteBuffers(1, &m_vertex_array_buffer_id);
		m_vertex_array_buffer_id = 0;
	}

	if (0 != m_vertex_array_object_id)
	{
		gl.bindVertexArray(0);
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);
		m_vertex_array_object_id = 0;
	}
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult LimitTest::iterate()
{
	IterateStart();

	bool result = true;

	RequireExtension("GL_ARB_vertex_attrib_64bit");

	testInit();

	if (false == testIteration(DOUBLE_DVEC2))
	{
		result = false;
	}

	if (false == testIteration(DVEC3_DVEC4))
	{
		result = false;
	}

	if (false == testIteration(DMAT2))
	{
		result = false;
	}

	if (false == testIteration(DMAT3X2_DMAT4X2))
	{
		result = false;
	}

	if (false == testIteration(DMAT2X3_DMAT2X4))
	{
		result = false;
	}

	if (false == testIteration(DMAT3_DMAT3X4))
	{
		result = false;
	}

	if (false == testIteration(DMAT4X3_DMAT4))
	{
		result = false;
	}

	/* Done */
	return IterateStop(result);
}

/** Calculate offset of "n_type" attributes group in doubles, tightly packed, for vertex buffer offsets
 *
 * @param configuration Attribute configuration
 * @param n_type        Attribute type ordinal number
 *
 * @return Calculated offset
 **/
GLint LimitTest::calculateAttributeGroupOffset(const attributeConfiguration& configuration, GLint n_type) const
{
	GLint result = 0;

	for (GLint i = 0; i < n_type; ++i)
	{
		result += configuration.m_n_attributes_per_group * configuration.m_n_elements[i];
	}

	return result;
}

/** Calculates attribute location for manually setting "layout(location =)".
 *  Results are in reveresed order of vertex buffer
 *
 * @param configuration Attribute configuration
 * @param attribute     Intex of attribute in "n_type" group
 * @param n_type        Ordinal number of type
 *
 * @return Calculated location
 **/
GLint LimitTest::calculateAttributeLocation(const attributeConfiguration& configuration, GLint attribute,
											GLint n_type) const
{
	const GLint n_types = configuration.m_n_types;
	GLint		result  = 0;

	/* Amount of location required for types after given "n_type" */
	for (GLint i = n_types - 1; i > n_type; --i)
	{
		const GLint n_elements = configuration.m_n_elements[i];
		const GLint n_rows	 = configuration.m_n_rows[i];
		const GLint n_columns  = n_elements / n_rows;

		result += n_columns * configuration.m_n_attributes_per_group;
	}

	/* Amount of locations required for attributes after given attribute in given "n_type" */
	/* Arrayed attributes does not have any attributes after */
	if (m_array_attribute != attribute)
	{
		const GLint n_elements = configuration.m_n_elements[n_type];
		const GLint n_rows	 = configuration.m_n_rows[n_type];
		const GLint n_columns  = n_elements / n_rows;

		result += n_columns * (configuration.m_n_attributes_per_group - 1 - attribute);
	}

	/* Done */
	return result;
}

/** Calculate vertex length in "doubles", tightly packed, for offset in vertex buffer
 *
 * @param configuration Attribute configuration, result is store as field ::m_vertex_length
 **/
void LimitTest::calculateVertexLength(attributeConfiguration& configuration) const
{
	GLint result = 0;

	for (GLint i = 0; i < configuration.m_n_types; ++i)
	{
		result += configuration.m_n_elements[i] * configuration.m_n_attributes_per_group;
	}

	configuration.m_vertex_length = result;
}

/** Configure attributes in given "n_type" group
 *
 * @param iteration        Iteration id
 * @param configuration    Configuration of attributes
 * @param n_type           "n_type" of attibutes
 * @param program_id       Program object id
 * @param use_arrays       If attributes are groupd in arrays
 * @param use_vertex_array If attributes are configured with vertex array or as constants
 **/
void LimitTest::configureAttribute(_iteration iteration, const attributeConfiguration& configuration, GLint n_type,
								   GLuint program_id, bool use_arrays, bool use_vertex_array) const
{
	static const GLint invalid_attrib_location = -1;

	const GLint attributes_index = n_type * configuration.m_n_attributes_per_group;
	const GLint group_offset	 = calculateAttributeGroupOffset(configuration, n_type);
	const GLint n_elements		 = configuration.m_n_elements[n_type];
	const GLint n_rows			 = configuration.m_n_rows[n_type];
	const GLint n_columns		 = n_elements / n_rows;
	const GLint vertex_length	= configuration.m_vertex_length;

	/* For each attribute in "n_type" group */
	for (GLint i = 0; i < configuration.m_n_attributes_per_group; ++i)
	{
		const GLint		  attribute_ordinal = i + attributes_index;
		std::stringstream attribute_name;

		/* Separate attributes are called: attribute_ORDINAL, arrayed: attribute_N_TYPE[INDEX] */
		if (false == use_arrays)
		{
			attribute_name << "attribute_" << attribute_ordinal;
		}
		else
		{
			attribute_name << "attribute_" << n_type << "[" << i << "]";
		}

		/* get location */
		GLint attribute_location = gl.getAttribLocation(program_id, attribute_name.str().c_str());
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetAttribLocation");

		if (invalid_attrib_location == attribute_location)
		{
			m_log << tcu::TestLog::Message << "GetAttribLocation(" << program_id << ", " << attribute_name.str()
				  << ") returned: " << attribute_location << tcu::TestLog::EndMessage;

			TCU_FAIL("Inactive attribute");
		}

		/* Configure */
		if (true == use_vertex_array)
		{
			/* With vertex array */
			for (GLint column = 0; column < n_columns; ++column)
			{
				const GLint attribute_offset = group_offset + i * n_elements;
				const GLint column_offset	= column * n_rows;

				gl.enableVertexAttribArray(attribute_location + column);
				GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");

				gl.vertexAttribLPointer(attribute_location + column, n_rows /* size */, GL_DOUBLE,
										static_cast<glw::GLsizei>(vertex_length * sizeof(GLdouble)),
										(GLvoid*)((attribute_offset + column_offset) * sizeof(GLdouble)));
				GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribLPointer");
			}
		}
		else
		{
			/* As constant */
			for (GLint column = 0; column < n_columns; ++column)
			{
				switch (iteration)
				{
				case DOUBLE_DVEC2:

					/* Double attributes should be assigned the value:
					 (n_attribute + gl_VertexID * 2) */
					/* Dvec2 attribute components should be assigned the following
					 vector values:
					 (n_attribute + gl_VertexID * 3 + 1,
					 n_attribute + gl_VertexID * 3 + 2)*/

					if (1 == n_rows)
					{
						gl.vertexAttribL1d(attribute_location, attribute_ordinal);
					}
					else
					{
						gl.vertexAttribL2d(attribute_location, attribute_ordinal + 1, attribute_ordinal + 2);
					}

					break;

				case DVEC3_DVEC4:

					/* Dvec3 attribute components should be assigned the following
					 vector values:
					 (n_attribute + gl_VertexID * 3 + 0,
					 n_attribute + gl_VertexID * 3 + 1,
					 n_attribute + gl_VertexID * 3 + 2).

					 Dvec4 attribute components should be assigned the following
					 vector values:
					 (n_attribute + gl_VertexID * 4 + 0,
					 n_attribute + gl_VertexID * 4 + 1,
					 n_attribute + gl_VertexID * 4 + 2,
					 n_attribute + gl_VertexID * 4 + 3).*/

					if (3 == n_rows)
					{
						gl.vertexAttribL3d(attribute_location, attribute_ordinal + 0, attribute_ordinal + 1,
										   attribute_ordinal + 2);
					}
					else
					{
						gl.vertexAttribL4d(attribute_location, attribute_ordinal + 0, attribute_ordinal + 1,
										   attribute_ordinal + 2, attribute_ordinal + 3);
					}

					break;

				case DMAT2:
				case DMAT3X2_DMAT4X2:
				case DMAT2X3_DMAT2X4:
				case DMAT3_DMAT3X4:
				case DMAT4X3_DMAT4:

					/* Subsequent matrix elements should be assigned the following value:
					 (n_type + n_attribute + gl_VertexID * 16 + n_value)*/

					if (2 == n_rows)
					{
						gl.vertexAttribL2d(attribute_location + column,
										   n_type + attribute_ordinal + 0 + column * n_rows,
										   n_type + attribute_ordinal + 1 + column * n_rows);
					}
					else if (3 == n_rows)
					{
						gl.vertexAttribL3d(attribute_location + column,
										   n_type + attribute_ordinal + 0 + column * n_rows,
										   n_type + attribute_ordinal + 1 + column * n_rows,
										   n_type + attribute_ordinal + 2 + column * n_rows);
					}
					else
					{
						gl.vertexAttribL4d(attribute_location + column,
										   n_type + attribute_ordinal + 0 + column * n_rows,
										   n_type + attribute_ordinal + 1 + column * n_rows,
										   n_type + attribute_ordinal + 2 + column * n_rows,
										   n_type + attribute_ordinal + 3 + column * n_rows);
					}

					break;
				}
			}
		}
	}
}

/** Get varying name and vertex shader code for given configuration
 *
 * @param iteration                 Iteration id
 * @param use_arrays                If attributes should be grouped in arrays
 * @param use_locations             If attributes locations should be set manualy
 * @param use_vertex_attrib_divisor If vertex attribute divisor should be used
 * @param out_varying_name          Name of varying to be captured with transform feedback
 * @param out_vertex_shader_code    Source code of vertex shader
 **/
void LimitTest::getProgramDetails(_iteration iteration, bool use_arrays, bool use_locations,
								  bool use_vertex_attrib_divisor, std::string& out_varying_name,
								  std::string& out_vertex_shader_code) const
{
	static const GLchar* varying_name = "vs_output_value";

	attributeConfiguration configuration;
	GLint				   n_attributes = 0;
	GLint				   n_types		= 0;
	std::stringstream	  stream;

	const GLchar* advancement_str = (true == use_vertex_attrib_divisor) ? "gl_InstanceID" : "gl_VertexID";

	getVertexArrayConfiguration(iteration, configuration);

	n_attributes = configuration.m_n_attributes_per_group;
	n_types		 = configuration.m_n_types;

	/* Preamble */
	stream << "#version 400\n"
			  "#extension GL_ARB_vertex_attrib_64bit : require\n"
			  "\n"
			  "precision highp float;\n"
			  "\n";

	/* Attribute declarations */
	/* Spearated attributes are called: attribute_ORDINAL, arrayed: attribute_N_TYPE */
	for (GLint n_type = 0; n_type < n_types; ++n_type)
	{
		const GLint   attribute_offset = n_type * n_attributes;
		const GLchar* type_name		   = configuration.m_type_names[n_type];

		stream << "// " << type_name << "\n";

		if (false == use_arrays)
		{
			for (GLint attribute = 0; attribute < n_attributes; ++attribute)
			{
				if (true == use_locations)
				{
					const GLint location = calculateAttributeLocation(configuration, attribute, n_type);

					stream << "layout(location = " << location << ") ";
				}

				stream << "in " << type_name << " attribute_" << attribute + attribute_offset << ";\n";
			}
		}
		else
		{
			if (true == use_locations)
			{
				const GLint location = calculateAttributeLocation(configuration, m_array_attribute, n_type);

				stream << "layout(location = " << location << ") ";
			}

			stream << "in " << type_name << " attribute_" << n_type << "[" << n_attributes << "];\n";
		}

		stream << "\n";
	}

	/* Varying declaration */
	stream << "out int " << varying_name << ";\n\n";

	/* Main */
	stream << "void main()\n"
			  "{\n";

	for (GLint n_type = 0; n_type < n_types; ++n_type)
	{
		const GLint   n_elements = configuration.m_n_elements[n_type];
		const GLchar* type_name  = configuration.m_type_names[n_type];

		stream << "// " << type_name << "\n";

		/* if (attribute_name != type(values))
		 * {
		 *     varying = 0;
		 * }
		 */
		for (GLint attribute = 0; attribute < n_attributes; ++attribute)
		{
			const GLint attribute_ordinal = attribute + n_type * n_attributes;

			/* First attribute is verified with "if", rest with "else if" */
			if (0 == attribute_ordinal)
			{
				stream << "    if (attribute_";
			}
			else
			{
				stream << "    else if (attribute_";
			}

			/* Spearated attributes are called: attribute_ORDINAL, arrayed: attribute_N_TYPE[INDEX] */
			if (false == use_arrays)
			{
				stream << attribute_ordinal;
			}
			else
			{
				stream << n_type << "[" << attribute << "]";
			}

			/* != type() */
			stream << " != " << type_name << "(";

			/* Values for type constructor, depend on iteration */
			switch (iteration)
			{
			case DOUBLE_DVEC2:

				/* Double attributes should be assigned the value:
				 (n_attribute + gl_VertexID * 2) */
				/* Dvec2 attribute components should be assigned the following
				 vector values:
				 (n_attribute + gl_VertexID * 3 + 1,
				 n_attribute + gl_VertexID * 3 + 2)*/

				if (1 == n_elements)
				{
					stream << attribute_ordinal << " + " << advancement_str << " * 2";
				}
				else
				{
					stream << attribute_ordinal << " + " << advancement_str << " * 3 + 1"
						   << ", " << attribute_ordinal << " + " << advancement_str << " * 3 + 2";
				}

				break;

			case DVEC3_DVEC4:

				/* Dvec3 attribute components should be assigned the following
				 vector values:
				 (n_attribute + gl_VertexID * 3 + 0,
				 n_attribute + gl_VertexID * 3 + 1,
				 n_attribute + gl_VertexID * 3 + 2).

				 Dvec4 attribute components should be assigned the following
				 vector values:
				 (n_attribute + gl_VertexID * 4 + 0,
				 n_attribute + gl_VertexID * 4 + 1,
				 n_attribute + gl_VertexID * 4 + 2,
				 n_attribute + gl_VertexID * 4 + 3).*/

				for (GLint element = 0; element < n_elements; ++element)
				{
					stream << attribute_ordinal << " + " << advancement_str << " * " << n_elements << " + " << element;

					if (n_elements != element + 1)
					{
						stream << ", ";
					}
				}

				break;

			case DMAT2:
			case DMAT3X2_DMAT4X2:
			case DMAT2X3_DMAT2X4:
			case DMAT3_DMAT3X4:
			case DMAT4X3_DMAT4:

				/* Subsequent matrix elements should be assigned the following value:
				 (n_type + n_attribute + gl_VertexID * 16 + n_value)*/

				for (GLint element = 0; element < n_elements; ++element)
				{
					stream << n_type << " + " << attribute_ordinal << " + " << advancement_str << " * 16 + " << element;

					if (n_elements != element + 1)
					{
						stream << ", ";
					}
				}

				break;
			}

			/* type() { varying = 0 } */
			stream << "))\n"
				   << "    {\n"
				   << "        " << varying_name << " = 0;\n"
				   << "    }\n";
		}
	}

	/* All attributes verified: else { varyin = 1 }
	 Close main body */
	stream << "    else\n"
		   << "    {\n"
		   << "        " << varying_name << " = 1;\n"
		   << "    }\n"
		   << "}\n\n";

	/* Store results */
	out_varying_name	   = varying_name;
	out_vertex_shader_code = stream.str();
}

/** Get configuration of vertex array object
 *
 * @param iteration         Iteration id
 * @param out_configuration Configuration
 **/
void LimitTest::getVertexArrayConfiguration(_iteration iteration, attributeConfiguration& out_configuration) const
{
	static const GLuint n_elements_per_scalar = 1;
	static const GLuint n_elements_per_vec2   = 2;
	static const GLuint n_elements_per_vec3   = 3;
	static const GLuint n_elements_per_vec4   = 4;
	static const GLuint n_elements_per_mat2   = 4;
	static const GLuint n_elements_per_mat2x3 = 6;
	static const GLuint n_elements_per_mat2x4 = 8;
	static const GLuint n_elements_per_mat3   = 9;
	static const GLuint n_elements_per_mat3x2 = 6;
	static const GLuint n_elements_per_mat3x4 = 12;
	static const GLuint n_elements_per_mat4   = 16;
	static const GLuint n_elements_per_mat4x2 = 8;
	static const GLuint n_elements_per_mat4x3 = 12;

	const GLint max_vertex_attribs = GetMaxVertexAttribs();

	switch (iteration)
	{
	case DOUBLE_DVEC2:
	{
		static const GLint n_elements[] = { n_elements_per_scalar, n_elements_per_vec2 };
		static const GLint n_types		= sizeof(n_elements) / sizeof(n_elements[0]);

		static const GLint divisor = 2;

		static const GLchar* type_names[] = { "double", "dvec2" };

		static const GLint n_rows[] = {
			1, 2,
		};

		out_configuration.m_n_attributes_per_group = max_vertex_attribs / divisor;
		out_configuration.m_n_elements			   = n_elements;
		out_configuration.m_n_rows				   = n_rows;
		out_configuration.m_n_types				   = n_types;
		out_configuration.m_type_names			   = type_names;
	}
	break;
	case DVEC3_DVEC4:
	{
		static const GLint n_elements[] = { n_elements_per_vec3, n_elements_per_vec4 };
		static const GLint n_types		= sizeof(n_elements) / sizeof(n_elements[0]);

		static const GLint divisor = 4;

		static const GLchar* type_names[] = { "dvec3", "dvec4" };

		static const GLint n_rows[] = {
			3, 4,
		};

		out_configuration.m_n_attributes_per_group = max_vertex_attribs / divisor;
		out_configuration.m_n_elements			   = n_elements;
		out_configuration.m_n_rows				   = n_rows;
		out_configuration.m_n_types				   = n_types;
		out_configuration.m_type_names			   = type_names;
	}
	break;
	case DMAT2:
	{
		static const GLint n_elements[] = { n_elements_per_mat2 };
		static const GLint n_types		= sizeof(n_elements) / sizeof(n_elements[0]);

		static const GLint divisor = 2;

		static const GLchar* type_names[] = { "dmat2" };

		static const GLint n_rows[] = { 2 };

		out_configuration.m_n_attributes_per_group = max_vertex_attribs / divisor;
		out_configuration.m_n_elements			   = n_elements;
		out_configuration.m_n_rows				   = n_rows;
		out_configuration.m_n_types				   = n_types;
		out_configuration.m_type_names			   = type_names;
	}
	break;
	case DMAT3X2_DMAT4X2:
	{
		static const GLint n_elements[] = { n_elements_per_mat3x2, n_elements_per_mat4x2 };
		static const GLint n_types		= sizeof(n_elements) / sizeof(n_elements[0]);

		static const GLint divisor = 8;

		static const GLchar* type_names[] = { "dmat3x2", "dmat4x2" };

		static const GLint n_rows[] = { 2, 2 };

		out_configuration.m_n_attributes_per_group = max_vertex_attribs / divisor;
		out_configuration.m_n_elements			   = n_elements;
		out_configuration.m_n_rows				   = n_rows;
		out_configuration.m_n_types				   = n_types;
		out_configuration.m_type_names			   = type_names;
	}
	break;
	case DMAT2X3_DMAT2X4:
	{
		static const GLint n_elements[] = { n_elements_per_mat2x3, n_elements_per_mat2x4 };
		static const GLint n_types		= sizeof(n_elements) / sizeof(n_elements[0]);

		static const GLint divisor = 8;

		static const GLchar* type_names[] = { "dmat2x3", "dmat2x4" };

		static const GLint n_rows[] = { 3, 4 };

		out_configuration.m_n_attributes_per_group = max_vertex_attribs / divisor;
		out_configuration.m_n_elements			   = n_elements;
		out_configuration.m_n_rows				   = n_rows;
		out_configuration.m_n_types				   = n_types;
		out_configuration.m_type_names			   = type_names;
	}
	break;
	case DMAT3_DMAT3X4:
	{
		static const GLint n_elements[] = { n_elements_per_mat3, n_elements_per_mat3x4 };
		static const GLint n_types		= sizeof(n_elements) / sizeof(n_elements[0]);

		static const GLint divisor = 12;

		static const GLchar* type_names[] = { "dmat3", "dmat3x4" };

		static const GLint n_rows[] = { 3, 4 };

		out_configuration.m_n_attributes_per_group = max_vertex_attribs / divisor;
		out_configuration.m_n_elements			   = n_elements;
		out_configuration.m_n_rows				   = n_rows;
		out_configuration.m_n_types				   = n_types;
		out_configuration.m_type_names			   = type_names;
	}
	break;
	case DMAT4X3_DMAT4:
	{
		static const GLint n_elements[] = { n_elements_per_mat4x3, n_elements_per_mat4 };
		static const GLint n_types		= sizeof(n_elements) / sizeof(n_elements[0]);

		static const GLint divisor = 16;

		static const GLchar* type_names[] = { "dmat4x3", "dmat4" };

		static const GLint n_rows[] = { 3, 4 };

		out_configuration.m_n_attributes_per_group = max_vertex_attribs / divisor;
		out_configuration.m_n_elements			   = n_elements;
		out_configuration.m_n_rows				   = n_rows;
		out_configuration.m_n_types				   = n_types;
		out_configuration.m_type_names			   = type_names;
	}
	break;
	}

	calculateVertexLength(out_configuration);
}

/** Logs iteration and configuration of test
 *
 * @param iteration      Iteration id
 * @param use_arrays     If attributes are grouped in arrays
 * @param use_locations  If manual attribute locations are used
 * @param attribute_type Regular, constant or per instance
 **/
void LimitTest::logTestIterationAndConfig(_iteration iteration, _attributeType attribute_type, bool use_arrays,
										  bool use_locations) const
{
	tcu::MessageBuilder message = m_log << tcu::TestLog::Message;

	switch (iteration)
	{
	case DOUBLE_DVEC2:
		message << "Iteration: double + dvec2";

		break;
	case DVEC3_DVEC4:
		message << "Iteration: devc3 + dvec4";

		break;
	case DMAT2:
		message << "Iteration: dmat2";

		break;
	case DMAT3X2_DMAT4X2:
		message << "Iteration: dmat3x2 + dmat4x2";

		break;
	case DMAT2X3_DMAT2X4:
		message << "Iteration: dmat2x3 + dmat2x4";

		break;
	case DMAT3_DMAT3X4:
		message << "Iteration: dmat3 + dmat3x4";

		break;
	case DMAT4X3_DMAT4:
		message << "Iteration: dmat4x3 + dmat4";

		break;
	}

	message << "Configuration: ";

	if (true == use_arrays)
	{
		message << "arrayed attributes";
	}
	else
	{
		message << "separate attributes";
	}

	message << ", ";

	if (true == use_locations)
	{
		message << "reversed locations";
	}
	else
	{
		message << "default locations";
	}

	message << ", ";

	switch (attribute_type)
	{
	case REGULAR:
		message << "vertex attribute divisor: 0";

		break;
	case CONSTANT:
		message << "constant vertex attribute";

		break;
	case PER_INSTANCE:
		message << "vertex attribute divisor: 1";

		break;
	}

	message << tcu::TestLog::EndMessage;
}

/** Prepare program info for given configuration
 *
 * @param iteration                 Iteration id
 * @param use_arrays                If attributes should be grouped in arrays
 * @param use_locations             If manual attribute locations should be used
 * @param use_vertex_attrib_divisor If vertex attribute divisor should be used
 * @param program_info              Program info
 **/
void LimitTest::prepareProgram(_iteration iteration, bool use_arrays, bool use_locations,
							   bool use_vertex_attrib_divisor, programInfo& program_info)
{
	static const GLchar* fragment_shader_code = "#version 400\n"
												"#extension GL_ARB_vertex_attrib_64bit : require\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    discard;\n"
												"}\n\n";
	std::string varying_name;
	std::string vertex_shader_code;

	program_info.m_program_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

	getProgramDetails(iteration, use_arrays, use_locations, use_vertex_attrib_divisor, varying_name,
					  vertex_shader_code);

	{
		const GLchar* temp_varying_name = varying_name.c_str();

		gl.transformFeedbackVaryings(program_info.m_program_id, m_n_varyings, &temp_varying_name,
									 GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TransformFeedbackVaryings");
	}

	BuildProgram(fragment_shader_code, program_info.m_program_id, vertex_shader_code.c_str(),
				 program_info.m_fragment_shader_id, program_info.m_vertex_shader_id);
}

/** Configure vertex array object for all attributes
 *
 * @param iteration      Iteration id
 * @param attribute_type Regular, constant or per instance
 * @param program_id     Program object id
 * @param use_arrays     If attributes are grouped with arrays
 **/
void LimitTest::prepareVertexArray(_iteration iteration, _attributeType attribute_type, GLuint program_id,
								   bool use_arrays) const
{
	const GLint  max_vertex_attribs	= GetMaxVertexAttribs();
	const GLuint vertex_attrib_divisor = (PER_INSTANCE == attribute_type) ? 1 : 0;

	attributeConfiguration configuration;

	getVertexArrayConfiguration(iteration, configuration);

	/* Set vertex attributes divisor and disable */
	for (GLint i = 0; i < max_vertex_attribs; ++i)
	{
		gl.vertexAttribDivisor(i, vertex_attrib_divisor);
		GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribDivisor");

		gl.disableVertexAttribArray(i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DisableVertexAttribArray");
	}

	for (GLint n_type = 0; n_type < configuration.m_n_types; ++n_type)
	{
		configureAttribute(iteration, configuration, n_type, program_id, use_arrays, (CONSTANT != attribute_type));
	}
}

/** Prepare vertex buffer data for given iteration
 *
 * @param iteration Iteration id
 **/
void LimitTest::prepareVertexArrayBuffer(_iteration iteration)
{
	GLuint				   buffer_length = 0;
	attributeConfiguration configuration;

	getVertexArrayConfiguration(iteration, configuration);

	buffer_length = m_n_vertices * configuration.m_vertex_length;

	std::vector<GLdouble> buffer_data;
	buffer_data.resize(buffer_length);

	for (GLuint vertex = 0; vertex < m_n_vertices; ++vertex)
	{
		setAttributes(iteration, configuration, vertex, buffer_data);
	}

	gl.bufferData(GL_ARRAY_BUFFER, buffer_length * sizeof(GLdouble), &buffer_data[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");
}

/** Set all attributes for <vertex>
 *
 * @param iteration       Iteration id
 * @param configuration   Attribute configuration
 * @param vertex          Vertex orinal number
 * @param out_buffer_data Buffer data
 **/
void LimitTest::setAttributes(_iteration iteration, const attributeConfiguration& configuration, GLuint vertex,
							  std::vector<GLdouble>& out_buffer_data) const
{
	const GLint n_types = configuration.m_n_types;

	for (GLint n_type = 0; n_type < n_types; ++n_type)
	{
		switch (iteration)
		{
		case DOUBLE_DVEC2:

			setAttributes_a(configuration, n_type, vertex, out_buffer_data);

			break;

		case DVEC3_DVEC4:

			setAttributes_b(configuration, n_type, vertex, out_buffer_data);

			break;

		case DMAT2:
		case DMAT3X2_DMAT4X2:
		case DMAT2X3_DMAT2X4:
		case DMAT3_DMAT3X4:
		case DMAT4X3_DMAT4:

			setAttributes_c(configuration, n_type, vertex, out_buffer_data);

			break;
		}
	}
}

/** Set attributes of given <n_type> for <vertex>, as described in "iteration a".
 *
 * @param configuration   Attribute configuration
 * @param n_type          "n_type" ordinal number
 * @param vertex          Vertex orinal number
 * @param out_buffer_data Buffer data
 **/
void LimitTest::setAttributes_a(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
								std::vector<GLdouble>& out_buffer_data) const
{
	const GLint n_elements = configuration.m_n_elements[n_type];

	if (1 == n_elements)
	{
		setAttributes_a_scalar(configuration, n_type, vertex, out_buffer_data);
	}
	else
	{
		setAttributes_a_vec(configuration, n_type, vertex, out_buffer_data);
	}
}

/** Set scalar double attributes of given <n_type> for <vertex>, as described in "iteration a".
 *
 * @param configuration   Attribute configuration
 * @param n_type          "n_type" ordinal number
 * @param vertex          Vertex orinal number
 * @param out_buffer_data Buffer data
 **/
void LimitTest::setAttributes_a_scalar(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
									   std::vector<GLdouble>& out_buffer_data) const
{
	const GLint n_attributes	= configuration.m_n_attributes_per_group;
	const GLint attribute_index = n_attributes * n_type;
	GLuint		vertex_offset   = vertex * configuration.m_vertex_length;

	const GLint group_offset = calculateAttributeGroupOffset(configuration, n_type) + vertex_offset;

	/* Double attributes should be assigned the value:
	 (n_attribute + gl_VertexID * 2) */

	for (GLint attribute = 0; attribute < n_attributes; ++attribute)
	{
		const GLuint attribute_offset = attribute + group_offset;

		out_buffer_data[attribute_offset] = attribute + attribute_index + vertex * 2;
	}
}

/** Set dvec2 attributes of given <n_type> for <vertex>, as described in "iteration a".
 *
 * @param configuration   Attribute configuration
 * @param n_type          "n_type" ordinal number
 * @param vertex          Vertex orinal number
 * @param out_buffer_data Buffer data
 **/
void LimitTest::setAttributes_a_vec(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
									std::vector<GLdouble>& out_buffer_data) const
{
	const GLint n_attributes	= configuration.m_n_attributes_per_group;
	const GLint attribute_index = n_attributes * n_type;
	const GLint n_elements		= configuration.m_n_elements[n_type];
	GLuint		vertex_offset   = vertex * configuration.m_vertex_length;

	const GLint group_offset = calculateAttributeGroupOffset(configuration, n_type) + vertex_offset;

	/* Dvec2 attribute components should be assigned the following
	 vector values:
	 (n_attribute + gl_VertexID * 3 + 1,
	 n_attribute + gl_VertexID * 3 + 2)*/

	for (GLint attribute = 0; attribute < n_attributes; ++attribute)
	{
		const GLuint attribute_offset = n_elements * attribute + group_offset;

		for (GLint i = 0; i < n_elements; ++i)
		{
			out_buffer_data[attribute_offset + i] = attribute + attribute_index + vertex * 3 + i + 1;
		}
	}
}

/** Set attributes of given <n_type> for <vertex>, as described in "iteration b".
 *
 * @param configuration   Attribute configuration
 * @param n_type          "n_type" ordinal number
 * @param vertex          Vertex orinal number
 * @param out_buffer_data Buffer data
 **/
void LimitTest::setAttributes_b(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
								std::vector<GLdouble>& out_buffer_data) const
{
	const GLint n_attributes	= configuration.m_n_attributes_per_group;
	const GLint attribute_index = n_attributes * n_type;
	const GLint n_elements		= configuration.m_n_elements[n_type];
	GLuint		vertex_offset   = vertex * configuration.m_vertex_length;

	const GLint group_offset = calculateAttributeGroupOffset(configuration, n_type) + vertex_offset;

	/* Dvec3 attribute components should be assigned the following
	 vector values:
	 (n_attribute + gl_VertexID * 3 + 0,
	 n_attribute + gl_VertexID * 3 + 1,
	 n_attribute + gl_VertexID * 3 + 2).

	 Dvec4 attribute components should be assigned the following
	 vector values:
	 (n_attribute + gl_VertexID * 4 + 0,
	 n_attribute + gl_VertexID * 4 + 1,
	 n_attribute + gl_VertexID * 4 + 2,
	 n_attribute + gl_VertexID * 4 + 3).*/

	for (GLint attribute = 0; attribute < n_attributes; ++attribute)
	{
		const GLuint attribute_offset = n_elements * attribute + group_offset;

		for (GLint i = 0; i < n_elements; ++i)
		{
			out_buffer_data[attribute_offset + i] = attribute + attribute_index + vertex * n_elements + i;
		}
	}
}

/** Set attributes of given <n_type> for <vertex>, as described in "iteration c".
 *
 * @param configuration   Attribute configuration
 * @param n_type          "n_type" ordinal number
 * @param vertex          Vertex orinal number
 * @param out_buffer_data Buffer data
 **/
void LimitTest::setAttributes_c(const attributeConfiguration& configuration, GLint n_type, GLuint vertex,
								std::vector<GLdouble>& out_buffer_data) const
{
	const GLint n_attributes	= configuration.m_n_attributes_per_group;
	const GLint attribute_index = n_attributes * n_type;
	const GLint n_elements		= configuration.m_n_elements[n_type];
	GLuint		vertex_offset   = vertex * configuration.m_vertex_length;

	const GLint group_offset = calculateAttributeGroupOffset(configuration, n_type) + vertex_offset;

	/* Subsequent matrix elements should be assigned the following value:
	 (n_type + n_attribute + gl_VertexID * 16 + n_value)*/

	for (GLint attribute = 0; attribute < n_attributes; ++attribute)
	{
		const GLuint attribute_offset = n_elements * attribute + group_offset;

		for (GLint i = 0; i < n_elements; ++i)
		{
			out_buffer_data[attribute_offset + i] = n_type + attribute + attribute_index + vertex * 16 + i;
		}
	}
}

/** Run test with DrawArrays routine
 *
 * @return true if test pass, false otherwise
 **/
bool LimitTest::testDrawArrays() const
{
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawArrays(GL_POINTS, 0 /* first */, m_n_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	if (true == verifyResult(false))
	{
		return true;
	}
	else
	{
		m_log << tcu::TestLog::Message << "Draw function: DrawArrays" << tcu::TestLog::EndMessage;

		return false;
	}
}

/** Run test with DrawArraysInstanced routine
 *
 * @return true if test pass, false otherwise
 **/
bool LimitTest::testDrawArraysInstanced() const
{
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawArraysInstanced(GL_POINTS, 0 /* first */, m_n_vertices, m_n_instances);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArraysInstanced");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	if (true == verifyResult(false))
	{
		return true;
	}
	else
	{
		m_log << tcu::TestLog::Message << "Draw function: DrawArraysInstanced" << tcu::TestLog::EndMessage;

		return false;
	}
}

/** Run test with DrawElements routine
 *
 * @return true if test pass, false otherwise
 **/
bool LimitTest::testDrawElements() const
{
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawElements(GL_POINTS, m_n_vertices, GL_UNSIGNED_INT, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	if (true == verifyResult(false))
	{
		return true;
	}
	else
	{
		m_log << tcu::TestLog::Message << "Draw function: DrawElements" << tcu::TestLog::EndMessage;

		return false;
	}
}

/** Run test with DrawElementsInstanced routine
 *
 * @return true if test pass, false otherwise
 **/
bool LimitTest::testDrawElementsInstanced() const
{
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawElementsInstanced(GL_POINTS, m_n_vertices, GL_UNSIGNED_INT, 0, m_n_instances);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElementsInstanced");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	if (true == verifyResult(false))
	{
		return true;
	}
	else
	{
		m_log << tcu::TestLog::Message << "Draw function: DrawElementsInstanced" << tcu::TestLog::EndMessage;

		return false;
	}
}

/** Test initialisation
 *
 **/
void LimitTest::testInit()
{
	/* Prepare data for element array buffer */
	std::vector<GLuint> indices_data;
	indices_data.resize(m_n_vertices);
	for (GLuint i = 0; i < m_n_vertices; ++i)
	{
		indices_data[i] = i;
	}

	/* Prepare vertex array object */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

	gl.bindVertexArray(m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");

	/* Generate buffers */
	gl.genBuffers(1, &m_element_array_buffer_id);
	gl.genBuffers(1, &m_transoform_feedback_buffer_id);
	gl.genBuffers(1, &m_vertex_array_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

	/* Prepare element array buffer */
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_element_array_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, m_n_vertices * sizeof(GLuint), &indices_data[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");

	/* Prepare transform feedback buffer */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transoform_feedback_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, m_transform_feedback_buffer_size, 0 /* data */, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");

	/* Bind array buffer for future use */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_array_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	/* Disabe rasterization */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Enable");
}

/** Tests specified "iteration"
 *
 * @param iteration Iteration id
 *
 * @return true if tests pass, false otherwise
 **/
bool LimitTest::testIteration(_iteration iteration)
{
	bool result = true;

	/* Program infos */
	programInfo _no_array__no_location______regular(gl);
	programInfo use_array__no_location______regular(gl);
	programInfo _no_array_use_location______regular(gl);
	programInfo use_array_use_location______regular(gl);
	programInfo _no_array__no_location_per_instance(gl);
	programInfo use_array__no_location_per_instance(gl);
	programInfo _no_array_use_location_per_instance(gl);
	programInfo use_array_use_location_per_instance(gl);

	/* Prepare programs for all configuration */
	prepareProgram(iteration, false, false, false, _no_array__no_location______regular);
	prepareProgram(iteration, true, false, false, use_array__no_location______regular);
	prepareProgram(iteration, false, true, false, _no_array_use_location______regular);
	prepareProgram(iteration, true, true, false, use_array_use_location______regular);
	prepareProgram(iteration, false, false, true, _no_array__no_location_per_instance);
	prepareProgram(iteration, true, false, true, use_array__no_location_per_instance);
	prepareProgram(iteration, false, true, true, _no_array_use_location_per_instance);
	prepareProgram(iteration, true, true, true, use_array_use_location_per_instance);

	/* Bind buffers */
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_element_array_buffer_id);
	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_transoform_feedback_buffer_id, 0,
					   m_transform_feedback_buffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");

	/* Prepare vertex array buffer for iteration */
	prepareVertexArrayBuffer(iteration);

	/* Regular and instanced draw calls, vertex attribute divisor: 0 */
	if (false == testProgram(iteration, _no_array__no_location______regular.m_program_id, false))
	{
		logTestIterationAndConfig(iteration, REGULAR, false /* use_arrays */, false /* use_locations */);

		result = false;
	}

	if (false == testProgram(iteration, use_array__no_location______regular.m_program_id, true))
	{
		logTestIterationAndConfig(iteration, REGULAR, true /* use_arrays */, false /* use_locations */);

		result = false;
	}

	if (false == testProgram(iteration, _no_array_use_location______regular.m_program_id, false))
	{
		logTestIterationAndConfig(iteration, REGULAR, false /* use_arrays */, true /* use_locations */);

		result = false;
	}

	if (false == testProgram(iteration, use_array_use_location______regular.m_program_id, true))
	{
		logTestIterationAndConfig(iteration, REGULAR, true /* use_arrays */, true /* use_locations */);

		result = false;
	}

	/* Regular draw calls, constant vertex attribute */
	if (false == testProgramWithConstant(iteration, _no_array__no_location_per_instance.m_program_id, false))
	{
		logTestIterationAndConfig(iteration, CONSTANT, false /* use_arrays */, false /* use_locations */);

		result = false;
	}

	if (false == testProgramWithConstant(iteration, use_array__no_location_per_instance.m_program_id, true))
	{
		logTestIterationAndConfig(iteration, CONSTANT, true /* use_arrays */, false /* use_locations */);

		result = false;
	}

	if (false == testProgramWithConstant(iteration, _no_array_use_location_per_instance.m_program_id, false))
	{
		logTestIterationAndConfig(iteration, CONSTANT, false /* use_arrays */, true /* use_locations */);

		result = false;
	}

	if (false == testProgramWithConstant(iteration, use_array_use_location_per_instance.m_program_id, true))
	{
		logTestIterationAndConfig(iteration, CONSTANT, true /* use_arrays */, true /* use_locations */);

		result = false;
	}

	/* Instanced draw calls, vertex attribute divisor: 1 */
	if (false == testProgramWithDivisor(iteration, _no_array__no_location_per_instance.m_program_id, false))
	{
		logTestIterationAndConfig(iteration, PER_INSTANCE, false /* use_arrays */, false /* use_locations */);

		result = false;
	}

	if (false == testProgramWithDivisor(iteration, use_array__no_location_per_instance.m_program_id, true))
	{
		logTestIterationAndConfig(iteration, PER_INSTANCE, true /* use_arrays */, false /* use_locations */);

		result = false;
	}

	if (false == testProgramWithDivisor(iteration, _no_array_use_location_per_instance.m_program_id, false))
	{
		logTestIterationAndConfig(iteration, PER_INSTANCE, false /* use_arrays */, true /* use_locations */);

		result = false;
	}

	if (false == testProgramWithDivisor(iteration, use_array_use_location_per_instance.m_program_id, true))
	{
		logTestIterationAndConfig(iteration, PER_INSTANCE, true /* use_arrays */, true /* use_locations */);

		result = false;
	}

	/* Done */
	return result;
}

/** Tests regular and instanced draw calls with vertex attribute divisor set to 0
 *
 * @param iteration  Iteration id
 * @param program_id Program object id
 * @param use_arrays true if arrays of attributes are used
 *
 * @return true if tests pass, false otherwise
 **/
bool LimitTest::testProgram(_iteration iteration, GLuint program_id, bool use_arrays) const
{
	bool result = true;

	gl.useProgram(program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	prepareVertexArray(iteration, REGULAR, program_id, use_arrays);

	if (false == testDrawArrays())
	{
		result = false;
	}

	if (false == testDrawElements())
	{
		result = false;
	}

	if (false == testDrawArraysInstanced())
	{
		result = false;
	}

	if (false == testDrawElementsInstanced())
	{
		result = false;
	}

	return result;
}

/** Tests constant attributes value, set with VertexAttribLd* routines
 *
 * @param iteration  Iteration id
 * @param program_id Program object id
 * @param use_arrays true if arrays of attributes are used
 *
 * @return true if tests pass, false otherwise
 **/
bool LimitTest::testProgramWithConstant(_iteration iteration, GLuint program_id, bool use_arrays) const
{
	bool result = true;

	gl.useProgram(program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	prepareVertexArray(iteration, CONSTANT, program_id, use_arrays);

	if (false == testDrawArrays())
	{
		result = false;
	}

	if (false == testDrawElements())
	{
		result = false;
	}

	return result;
}

/** Tests instanced draw calls with vertex attribute divisor set to 1
 *
 * @param iteration  Iteration id
 * @param program_id Program object id
 * @param use_arrays true if arrays of attributes are used
 *
 * @return true if tests pass, false otherwise
 **/
bool LimitTest::testProgramWithDivisor(_iteration iteration, GLuint program_id, bool use_arrays) const
{
	bool result = true;

	gl.useProgram(program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	prepareVertexArray(iteration, PER_INSTANCE, program_id, use_arrays);

	if (false == testDrawArraysInstanced())
	{
		result = false;
	}

	if (false == testDrawElementsInstanced())
	{
		result = false;
	}

	return result;
}

/** Verifies results
 *
 * @param use_instancing true if instanced draw call was made, otherwise false
 *
 * @result true if all vertices outputed 1, false otherwise
 **/
bool LimitTest::verifyResult(bool use_instancing) const
{
	_varyingType* buffer_data = (_varyingType*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	const GLuint  n_instances = (true == use_instancing) ? m_n_instances : 1;
	bool		  result	  = true;

	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	if (0 == buffer_data)
	{
		TCU_FAIL("Failed to map GL_TRANSFORM_FEEDBACK_BUFFER buffer");
	}

	/* For each instance */
	for (GLuint instance = 0; instance < n_instances; ++instance)
	{
		const GLuint instance_offset = instance * m_n_vertices * m_n_varyings;

		/* For each vertex */
		for (GLuint vertex = 0; vertex < m_n_vertices; ++vertex)
		{
			const GLuint vertex_offset = vertex * m_n_varyings;

			if (1 != buffer_data[vertex_offset + instance_offset])
			{
				if (true == use_instancing)
				{
					m_log << tcu::TestLog::Message << "Failure. Instance: " << instance << " Vertex: " << vertex
						  << tcu::TestLog::EndMessage;
				}
				else
				{
					m_log << tcu::TestLog::Message << "Failure. Vertex: " << vertex << tcu::TestLog::EndMessage;
				}

				/* Save failure and break loop */
				result = false;

				/* Sorry about that, but this is nested loop */
				goto end;
			}
		}
	}

end:
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	return result;
}

/** Implementation of conformance test "4", description follows.
 *
 *  Make sure non-trivial VAO configurations are correctly supported
 *  for double-precision floating-point types.
 *
 *  Consider the following Vertex Buffer Object configurations:
 *
 *  BO1:
 *  0            72 73 75      91 96
 *  --------------+-+--+-------+---
 *  |      A      |B| C|    D  | E|  (times 1024)
 *  -------------------------------
 *
 *  where:
 *
 *  A: 3x3 double matrix  (72 bytes)
 *  B: 1 unsigned byte    (1  byte)
 *  C: 1 short            (2  bytes)
 *  D: 2 doubles          (16 bytes)
 *  E: padding            (5  bytes)
 *                     (+) --------
 *                         96 bytes
 *
 *  BO2:
 *  --+------------------
 *  |A|         B       |  (times 1024)
 *  --+------------------
 *
 *  where:
 *
 *  A: 1 signed byte     (1  byte)
 *  B: 4x2 double matrix (64 bytes)
 *                    (+) --------
 *                        65 bytes
 *
 *  A VAO used for the test should be configured as described
 *  below:
 *
 *  Att 0 (L): VAP-type:GL_DOUBLE,        GLSL-type: dmat3,   stride:96,
 *             offset:  0,                normalized:0,       source:BO1;
 *  Att 1 (F): VAP-type:GL_UNSIGNED_BYTE, GLSL-type: float,   stride:5,
 *             offset:  0,                normalized:1,       source:BO2;
 *  Att 2 (L): VAP-type:GL_DOUBLE,        GLSL-type: dvec2,   stride:96,
 *             offset:  75,               normalized:0,       source:BO1;
 *  Att 3 (L): VAP-type:GL_DOUBLE,        GLSL-type: double,  stride:48,
 *             offset:  0,                normalized:0,       source:BO1;
 *  Att 4 (L): VAP-type:GL_DOUBLE,        GLSL-type: dmat4x2, stride:65,
 *             offset:  1,                normalized:0,       source:BO2;
 *  Att 5 (F): VAP-type:GL_SHORT,         GLSL-type: float,   stride:96,
 *             offset:  73,               normalized:0,       source:BO1;
 *  Att 6 (I): VAP-type:GL_BYTE,          GLSL-type: int,     stride:96,
 *             offset:  72,               normalized:1,       source:BO1;
 *
 *  where:
 *
 *  GLSL-type: Input variable type, as to be used in corresponding
 *             vertex shader.
 *  (F):       glVertexAttribPointer() call should be used to configure
 *             given vertex attribute array;
 *  (I):       glVertexAttribIPointer() call should be used to configure
 *             given vertex attribute array;
 *  (L):       glVertexAttribLPointer() call should be used to configure
 *             given vertex attribute array;
 *  VAP-type:  <type> argument as passed to corresponding
 *             glVertexAttrib*Pointer() call.
 *
 *  The test should use a program object consisting only of VS.
 *  The shader should read all the attributes and store the
 *  values in corresponding output variables. These should then be
 *  XFBed out to the test implementation, which should then verify
 *  the values read in the shader are valid in light of the specification.
 *
 *  All the draw call types described in test 3) should be tested.
 *  A single draw call for each of the types, rendering a total of
 *  1024 points should be used for the purpose of the test
 *
 **/
class VAOTest : public Base
{
public:
	/* Public methods */
	VAOTest(deqp::Context& context);

	virtual ~VAOTest()
	{
	}

	/* Public methods inheritated from TestCase */
	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type declarations */
	enum _draw_call_type
	{
		DRAW_CALL_TYPE_ARRAYS,
		DRAW_CALL_TYPE_ELEMENTS,

		/* Always last */
		DRAW_CALL_TYPE_COUNT
	};

	/* Private methods */
	bool executeTest(_draw_call_type draw_call, bool instanced, bool zero_vertex_attrib_divisor);

	void initBufferObjects();
	void initBuffers();
	void initProgramObject();
	void initVAO();

	bool verifyXFBData(const void* data, _draw_call_type draw_call, bool instanced, bool zero_vertex_attrib_divisor);

	/* Private fields */
	unsigned char*  m_bo_1_data;
	unsigned int	m_bo_1_data_size;
	unsigned int	m_bo_1_offset_matrix;
	unsigned int	m_bo_1_offset_ubyte;
	unsigned int	m_bo_1_offset_short;
	unsigned int	m_bo_1_offset_double;
	unsigned char*  m_bo_2_data;
	unsigned int	m_bo_2_data_size;
	unsigned int	m_bo_2_offset_sbyte;
	unsigned int	m_bo_2_offset_matrix;
	unsigned short* m_bo_index_data;
	unsigned int	m_bo_index_data_size;
	glw::GLuint		m_bo_id_1;
	glw::GLuint		m_bo_id_2;
	glw::GLuint		m_bo_id_indices;
	glw::GLuint		m_bo_id_result;
	glw::GLint		m_po_bo1_dmat3_attr_location;
	glw::GLint		m_po_bo2_dmat4x2_attr_location;
	glw::GLint		m_po_bo1_double_attr_location;
	glw::GLint		m_po_bo1_dvec2_attr_location;
	glw::GLint		m_po_bo1_float2_attr_location;
	glw::GLint		m_po_bo1_int_attr_location;
	glw::GLint		m_po_bo2_float_attr_location;
	glw::GLuint		m_po_id;
	glw::GLuint		m_vao_id;
	glw::GLuint		m_vs_id;
	unsigned int	m_xfb_bo1_dmat3_offset;
	unsigned int	m_xfb_bo1_dmat3_size;
	unsigned int	m_xfb_bo1_double_offset;
	unsigned int	m_xfb_bo1_double_size;
	unsigned int	m_xfb_bo1_dvec2_offset;
	unsigned int	m_xfb_bo1_dvec2_size;
	unsigned int	m_xfb_bo1_float2_offset;
	unsigned int	m_xfb_bo1_float2_size;
	unsigned int	m_xfb_bo1_int_offset;
	unsigned int	m_xfb_bo1_int_size;
	unsigned int	m_xfb_bo2_dmat4x2_offset;
	unsigned int	m_xfb_bo2_dmat4x2_size;
	unsigned int	m_xfb_bo2_float_offset;
	unsigned int	m_xfb_bo2_float_size;
	unsigned int	m_xfb_total_size;

	const unsigned int m_bo_1_batch_size;
	const unsigned int m_bo_2_batch_size;
	const unsigned int m_n_batches;
	const unsigned int m_n_draw_call_instances;
	const unsigned int m_nonzero_vertex_attrib_divisor;
	const unsigned int m_po_bo1_dmat3_attr_offset;
	const unsigned int m_po_bo1_dmat3_attr_stride;
	const unsigned int m_po_bo1_double_attr_offset;
	const unsigned int m_po_bo1_double_attr_stride;
	const unsigned int m_po_bo1_dvec2_attr_offset;
	const unsigned int m_po_bo1_dvec2_attr_stride;
	const unsigned int m_po_bo1_float2_attr_offset;
	const unsigned int m_po_bo1_float2_attr_stride;
	const unsigned int m_po_bo1_int_attr_offset;
	const unsigned int m_po_bo1_int_attr_stride;
	const unsigned int m_po_bo2_dmat4x2_attr_offset;
	const unsigned int m_po_bo2_dmat4x2_attr_stride;
	const unsigned int m_po_bo2_float_attr_offset;
	const unsigned int m_po_bo2_float_attr_stride;
};

/** Constructor
 *
 * @param context CTS context instance
 **/
VAOTest::VAOTest(deqp::Context& context)
	: Base(context, "vao", "Verify that non-trivial VAO configurations are correctly supported "
						   "for double-precision floating-point types.")
	, m_bo_1_data(DE_NULL)
	, m_bo_1_data_size(0)
	, m_bo_1_offset_matrix(0)
	, m_bo_1_offset_ubyte(72)
	, m_bo_1_offset_short(73)
	, m_bo_1_offset_double(75)
	, m_bo_2_data(DE_NULL)
	, m_bo_2_data_size(0)
	, m_bo_2_offset_sbyte(0)
	, m_bo_2_offset_matrix(1)
	, m_bo_index_data(DE_NULL)
	, m_bo_index_data_size(0)
	, m_bo_id_1(0)
	, m_bo_id_2(0)
	, m_bo_id_indices(0)
	, m_bo_id_result(0)
	, m_po_bo1_dmat3_attr_location(-1)
	, m_po_bo2_dmat4x2_attr_location(-1)
	, m_po_bo1_double_attr_location(-1)
	, m_po_bo1_dvec2_attr_location(-1)
	, m_po_bo1_float2_attr_location(-1)
	, m_po_bo1_int_attr_location(-1)
	, m_po_bo2_float_attr_location(-1)
	, m_po_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_xfb_bo1_dmat3_offset(0)
	, m_xfb_bo1_dmat3_size(0)
	, m_xfb_bo1_double_offset(0)
	, m_xfb_bo1_double_size(0)
	, m_xfb_bo1_dvec2_offset(0)
	, m_xfb_bo1_dvec2_size(0)
	, m_xfb_bo1_float2_offset(0)
	, m_xfb_bo1_float2_size(0)
	, m_xfb_bo1_int_offset(0)
	, m_xfb_bo1_int_size(0)
	, m_xfb_bo2_dmat4x2_offset(0)
	, m_xfb_bo2_dmat4x2_size(0)
	, m_xfb_bo2_float_offset(0)
	, m_xfb_bo2_float_size(0)
	, m_xfb_total_size(0)
	, m_bo_1_batch_size(96)
	, m_bo_2_batch_size(65)
	, m_n_batches(1024)
	, m_n_draw_call_instances(4)
	, m_nonzero_vertex_attrib_divisor(2)
	, m_po_bo1_dmat3_attr_offset(0)
	, m_po_bo1_dmat3_attr_stride(96)
	, m_po_bo1_double_attr_offset(0)
	, m_po_bo1_double_attr_stride(48)
	, m_po_bo1_dvec2_attr_offset(75)
	, m_po_bo1_dvec2_attr_stride(96)
	, m_po_bo1_float2_attr_offset(73)
	, m_po_bo1_float2_attr_stride(96)
	, m_po_bo1_int_attr_offset(72)
	, m_po_bo1_int_attr_stride(96)
	, m_po_bo2_dmat4x2_attr_offset(1)
	, m_po_bo2_dmat4x2_attr_stride(65)
	, m_po_bo2_float_attr_offset(0)
	, m_po_bo2_float_attr_stride(5)
{
	/* Nothing to be done here */
}

/** Deinitializes GL objects and deallocates buffers that may have
 *  been created during test execution */
void VAOTest::deinit()
{
	if (m_bo_1_data != DE_NULL)
	{
		delete[] m_bo_1_data;

		m_bo_1_data = DE_NULL;
	}

	if (m_bo_2_data != DE_NULL)
	{
		delete[] m_bo_2_data;

		m_bo_2_data = DE_NULL;
	}

	if (m_bo_index_data != DE_NULL)
	{
		delete[] m_bo_index_data;

		m_bo_index_data = DE_NULL;
	}

	if (m_bo_id_1 != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_1);

		m_bo_id_1 = 0;
	}

	if (m_bo_id_2 != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_2);

		m_bo_id_2 = 0;
	}

	if (m_bo_id_indices != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_indices);

		m_bo_id_indices = 0;
	}

	if (m_bo_id_result != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_result);

		m_bo_id_result = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test iteration.
 *
 *  This function may throw error exceptions if GL implementation misbehaves.
 *
 *  @param draw_call                  Type of the draw call that should be issued.
 *  @param instanced                  True if the draw call should be instanced, false otherwise.
 *  @param zero_vertex_attrib_divisor True if a zero divisor should be used for all checked attributes,
 *                                    false to use a value of m_nonzero_vertex_attrib_divisor as the divisor.
 *
 *  @return true if the test iteration passed, false otherwise.
 **/
bool VAOTest::executeTest(_draw_call_type draw_call, bool instanced, bool zero_vertex_attrib_divisor)
{
	bool result = true;

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
	{
		const glw::GLint divisor	  = (zero_vertex_attrib_divisor) ? 0 : m_nonzero_vertex_attrib_divisor;
		const glw::GLint attributes[] = { m_po_bo1_dmat3_attr_location,		  m_po_bo1_dmat3_attr_location + 1,
										  m_po_bo1_dmat3_attr_location + 2,

										  m_po_bo2_dmat4x2_attr_location,	 m_po_bo2_dmat4x2_attr_location + 1,
										  m_po_bo2_dmat4x2_attr_location + 2, m_po_bo2_dmat4x2_attr_location + 3,

										  m_po_bo1_double_attr_location,	  m_po_bo1_dvec2_attr_location,
										  m_po_bo1_float2_attr_location,	  m_po_bo1_int_attr_location,
										  m_po_bo2_float_attr_location };
		const unsigned int n_attributes = sizeof(attributes) / sizeof(attributes[0]);

		for (unsigned int n_attribute = 0; n_attribute < n_attributes; ++n_attribute)
		{
			glw::GLint attribute = attributes[n_attribute];

			/* Configure vertex attribute divisor */
			gl.vertexAttribDivisor(attribute, divisor);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribDivisor() call failed.");
		} /* for (all attribute locations) */

		/* Issue the draw call */
		switch (draw_call)
		{
		case DRAW_CALL_TYPE_ARRAYS:
		{
			if (instanced)
			{
				gl.drawArraysInstanced(GL_POINTS, 0 /* first */, m_n_batches, m_n_draw_call_instances);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArraysInstanced() call failed");
			}
			else
			{
				gl.drawArrays(GL_POINTS, 0 /* first */, m_n_batches);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
			}

			break;
		} /* case DRAW_CALL_TYPE_ARRAYS: */

		case DRAW_CALL_TYPE_ELEMENTS:
		{
			if (instanced)
			{
				gl.drawElementsInstanced(GL_POINTS, m_n_batches, GL_UNSIGNED_SHORT, DE_NULL /* indices */,
										 m_n_draw_call_instances);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsInstanced() call failed.");
			}
			else
			{
				gl.drawElements(GL_POINTS, m_n_batches, GL_UNSIGNED_SHORT, DE_NULL); /* indices */
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElements() call failed.");
			}

			break;
		} /* case DRAW_CALL_TYPE_ELEMENTS: */

		default:
		{
			TCU_FAIL("Unrecognized draw call type");
		}
		} /* switch (draw_call) */
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	/* Retrieve the results */
	const void* pXFBData = gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

	/* Verify the data */
	result = verifyXFBData(pXFBData, draw_call, instanced, zero_vertex_attrib_divisor);

	/* Unmap the buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	return result;
}

/** Initializes buffer objects that will be used by the test.
 *
 *  This function may throw error exceptions if GL implementation misbehaves.
 **/
void VAOTest::initBufferObjects()
{
	DE_ASSERT(m_bo_1_data != DE_NULL);
	DE_ASSERT(m_bo_2_data != DE_NULL);

	/* Generate BOs */
	gl.genBuffers(1, &m_bo_id_1);
	gl.genBuffers(1, &m_bo_id_2);
	gl.genBuffers(1, &m_bo_id_indices);
	gl.genBuffers(1, &m_bo_id_result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call(s) failed.");

	/* Initiailize BO storage */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, m_bo_1_data_size, m_bo_1_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, m_bo_2_data_size, m_bo_2_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bo_id_indices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, m_bo_index_data_size, m_bo_index_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Finally, reserve sufficient amount of space for the data to be XFBed out from
	 * the test program. We need:
	 *
	 * a) dmat3:   (3 * 3 * 2) components: 18 float components
	 * b) float:   (1)         component :  1 float component
	 * c) dvec2:   (2 * 2)     components:  4 float components
	 * d) double:  (1 * 2)     components:  2 float components
	 * e) dmat4x2: (4 * 2 * 2) components: 16 float components
	 * f) int:     (1)         components:  1 int   component
	 * g) float:   (1)         component:   1 float components
	 * h) padding: 4 bytes because fp64 buffer needs 8 bytes alignment
	 *                                 (+)------
	 *                                    (42 float + 1 int + 4 bytes padding) components times 1024 batches: 43008 floats, 1024 ints
	 *
	 * Don't forget about instanced draw calls. We'll be XFBing data for either 1 or m_n_draw_call_instances
	 * instances.
	 */
	const unsigned int xfb_dat_pad = sizeof(int);
	const unsigned int xfb_data_size =
		static_cast<unsigned int>((42 * sizeof(float) + sizeof(int) + xfb_dat_pad) * 1024 * m_n_draw_call_instances);

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, xfb_data_size, DE_NULL /* data */, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Set up XFB bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id_result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");
}

/** Initializes buffers that will later be used to fill storage of buffer objects used by the test. */
void VAOTest::initBuffers()
{
	DE_ASSERT(m_bo_1_data == DE_NULL);
	DE_ASSERT(m_bo_2_data == DE_NULL);
	DE_ASSERT(m_bo_index_data == DE_NULL);

	/* Prepare buffers storing underlying data. The buffers will be used for:
	 *
	 * - storage purposes;
	 * - verification of the data XFBed from the vertex shader.
	 */
	m_bo_1_data_size	 = m_bo_1_batch_size * m_n_batches;
	m_bo_2_data_size	 = m_bo_2_batch_size * m_n_batches;
	m_bo_index_data_size = static_cast<unsigned int>(sizeof(unsigned short) * m_n_batches);

	m_bo_1_data		= new unsigned char[m_bo_1_data_size];
	m_bo_2_data		= new unsigned char[m_bo_2_data_size];
	m_bo_index_data = new unsigned short[m_bo_index_data_size / sizeof(unsigned short)];

	/* Workaround for alignment issue that may result in bus error on some platforms */
	union {
		double		  d;
		unsigned char c[sizeof(double)];
	} u;

	/* Fill index data */
	for (unsigned short n_index = 0; n_index < (unsigned short)m_n_batches; ++n_index)
	{
		m_bo_index_data[n_index] = (unsigned short)((unsigned short)(m_n_batches - 1) - n_index);
	}

	/* Fill 3x3 matrix data in BO1 */
	for (unsigned int n_matrix = 0; n_matrix < m_n_batches; ++n_matrix)
	{
		double* matrix_ptr = (double*)(m_bo_1_data + n_matrix * m_bo_1_batch_size + m_bo_1_offset_matrix);

		for (unsigned int n_element = 0; n_element < 9 /* 3x3 matrix */; ++n_element)
		{
			matrix_ptr[n_element] = (double)(n_matrix * 3 * 3 + n_element + 1);
		}
	} /* for (all matrices) */

	/* Fill unsigned byte data in BO1 */
	for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
	{
		unsigned char* data_ptr = m_bo_1_data + n_element * m_bo_1_batch_size + m_bo_1_offset_ubyte;

		*data_ptr = (unsigned char)n_element;
	}

	/* Fill short data in BO1 */
	for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
	{
		unsigned short* data_ptr = (unsigned short*)(m_bo_1_data + n_element * m_bo_1_batch_size + m_bo_1_offset_short);

		*data_ptr = (unsigned short)n_element;
	}

	/* Fill 2 doubles data in BO1 */
	for (unsigned int n_batch = 0; n_batch < m_n_batches; ++n_batch)
	{
		unsigned char* data1_ptr = m_bo_1_data + n_batch * m_bo_1_batch_size + m_bo_1_offset_double;
		unsigned char* data2_ptr = data1_ptr + sizeof(double);

		u.d = (double)(2 * n_batch);
		memcpy(data1_ptr, u.c, sizeof(double));
		u.d = (double)(2 * n_batch + 1);
		memcpy(data2_ptr, u.c, sizeof(double));
	}

	/* Fill signed byte data in BO2 */
	for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
	{
		signed char* data_ptr = (signed char*)(m_bo_2_data + n_element * m_bo_2_batch_size + m_bo_2_offset_sbyte);

		*data_ptr = (signed char)n_element;
	}

	/* Fill 4x2 matrix data in BO2 */
	for (unsigned int n_matrix = 0; n_matrix < m_n_batches; ++n_matrix)
	{
		unsigned char* matrix_ptr = m_bo_2_data + n_matrix * m_bo_2_batch_size + m_bo_2_offset_matrix;

		for (unsigned int n_element = 0; n_element < 8 /* 4x2 matrix */; ++n_element)
		{
			u.d = (double)(n_matrix * 4 * 2 + n_element);
			memcpy(matrix_ptr + (sizeof(double) * n_element), u.c, sizeof(double));
		}
	} /* for (all matrices) */
}

/** Initializes a program object used by the test.
 *
 *  This function may throw error exceptions if GL implementation misbehaves.
 *
 **/
void VAOTest::initProgramObject()
{
	DE_ASSERT(m_po_id == 0);
	DE_ASSERT(m_vs_id == 0);

	/* Generate a program object */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Configure XFB */
	const char* xfb_varyings[] = { "out_bo1_dmat3",  "out_bo1_double",  "out_bo1_int",   "out_bo1_dvec2",
								   "out_bo1_float2", "out_bo2_dmat4x2", "out_bo2_float", "gl_SkipComponents1" };
	const unsigned int n_xfb_varyings = sizeof(xfb_varyings) / sizeof(xfb_varyings[0]);

	gl.transformFeedbackVaryings(m_po_id, n_xfb_varyings, xfb_varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	/* Initialize XFB-specific offset information for the verification routine */
	m_xfb_bo1_dmat3_offset   = 0;
	m_xfb_bo1_dmat3_size	 = sizeof(double) * 3 * 3;
	m_xfb_bo1_double_offset  = m_xfb_bo1_dmat3_offset + m_xfb_bo1_dmat3_size;
	m_xfb_bo1_double_size	= sizeof(double);
	m_xfb_bo1_int_offset	 = m_xfb_bo1_double_offset + m_xfb_bo1_double_size;
	m_xfb_bo1_int_size		 = sizeof(int);
	m_xfb_bo1_dvec2_offset   = m_xfb_bo1_int_offset + m_xfb_bo1_int_size;
	m_xfb_bo1_dvec2_size	 = sizeof(double) * 2;
	m_xfb_bo1_float2_offset  = m_xfb_bo1_dvec2_offset + m_xfb_bo1_dvec2_size;
	m_xfb_bo1_float2_size	= sizeof(float);
	m_xfb_bo2_dmat4x2_offset = m_xfb_bo1_float2_offset + m_xfb_bo1_float2_size;
	m_xfb_bo2_dmat4x2_size   = sizeof(double) * 4 * 2;
	m_xfb_bo2_float_offset   = m_xfb_bo2_dmat4x2_offset + m_xfb_bo2_dmat4x2_size;
	m_xfb_bo2_float_size	 = sizeof(float);
	m_xfb_total_size = m_xfb_bo1_dmat3_size + m_xfb_bo1_double_size + m_xfb_bo1_int_size + m_xfb_bo1_dvec2_size +
					   m_xfb_bo1_float2_size + m_xfb_bo2_dmat4x2_size + m_xfb_bo2_float_size + sizeof(int);

	/* Build the test program object */
	const char* vs_code = "#version 400\n"
						  "\n"
						  "#extension GL_ARB_vertex_attrib_64bit : require\n"
						  "\n"
						  "in dmat3   in_bo1_dmat3;\n"
						  "in double  in_bo1_double;\n"
						  "in dvec2   in_bo1_dvec2;\n"
						  "in float   in_bo1_float2;\n"
						  "in int     in_bo1_int;\n"
						  "in dmat4x2 in_bo2_dmat4x2;\n"
						  "in float   in_bo2_float;\n"
						  "\n"
						  "out dmat3   out_bo1_dmat3;\n"
						  "out double  out_bo1_double;\n"
						  "out dvec2   out_bo1_dvec2;\n"
						  "out float   out_bo1_float2;\n"
						  "out int     out_bo1_int;\n"
						  "out dmat4x2 out_bo2_dmat4x2;\n"
						  "out float   out_bo2_float;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    out_bo1_dmat3   = in_bo1_dmat3;\n"
						  "    out_bo1_double  = in_bo1_double;\n"
						  "    out_bo1_dvec2   = in_bo1_dvec2;\n"
						  "    out_bo1_int     = in_bo1_int;\n"
						  "    out_bo1_float2  = in_bo1_float2;\n"
						  "    out_bo2_dmat4x2 = in_bo2_dmat4x2;\n"
						  "    out_bo2_float   = in_bo2_float;\n"
						  "}\n";

	BuildProgramVSOnly(m_po_id, vs_code, m_vs_id);

	m_po_bo1_dmat3_attr_location   = gl.getAttribLocation(m_po_id, "in_bo1_dmat3");
	m_po_bo1_double_attr_location  = gl.getAttribLocation(m_po_id, "in_bo1_double");
	m_po_bo1_dvec2_attr_location   = gl.getAttribLocation(m_po_id, "in_bo1_dvec2");
	m_po_bo1_float2_attr_location  = gl.getAttribLocation(m_po_id, "in_bo1_float2");
	m_po_bo1_int_attr_location	 = gl.getAttribLocation(m_po_id, "in_bo1_int");
	m_po_bo2_dmat4x2_attr_location = gl.getAttribLocation(m_po_id, "in_bo2_dmat4x2");
	m_po_bo2_float_attr_location   = gl.getAttribLocation(m_po_id, "in_bo2_float");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation() call(s) failed.");

	if (m_po_bo1_dmat3_attr_location == -1 || m_po_bo1_double_attr_location == -1 ||
		m_po_bo1_dvec2_attr_location == -1 || m_po_bo1_int_attr_location == -1 || m_po_bo1_float2_attr_location == -1 ||
		m_po_bo2_dmat4x2_attr_location == -1 || m_po_bo2_float_attr_location == -1)
	{
		TCU_FAIL("At least one attribute is considered inactive which is invalid.");
	}
}

/** Initializes a vertex array object used by the test.
 *
 *  This function may throw error exceptions if GL implementation misbehaves.
 **/
void VAOTest::initVAO()
{
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Set up BO1-sourced attributes */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.vertexAttribLPointer(m_po_bo1_dmat3_attr_location + 0, 3, /* size */
							GL_DOUBLE, m_po_bo1_dmat3_attr_stride,
							(const glw::GLvoid*)(deUintptr)m_po_bo1_dmat3_attr_offset);
	gl.vertexAttribLPointer(m_po_bo1_dmat3_attr_location + 1, 3, /* size */
							GL_DOUBLE, m_po_bo1_dmat3_attr_stride,
							(const glw::GLvoid*)(m_po_bo1_dmat3_attr_offset + 1 * sizeof(double) * 3));
	gl.vertexAttribLPointer(m_po_bo1_dmat3_attr_location + 2, 3, /* size */
							GL_DOUBLE, m_po_bo1_dmat3_attr_stride,
							(const glw::GLvoid*)(m_po_bo1_dmat3_attr_offset + 2 * sizeof(double) * 3));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribLPointer() call(s) failed.");

	gl.enableVertexAttribArray(m_po_bo1_dmat3_attr_location + 0);
	gl.enableVertexAttribArray(m_po_bo1_dmat3_attr_location + 1);
	gl.enableVertexAttribArray(m_po_bo1_dmat3_attr_location + 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call(s) failed.");

	gl.vertexAttribLPointer(m_po_bo1_dvec2_attr_location, 2, /* size */
							GL_DOUBLE, m_po_bo1_dvec2_attr_stride,
							(const glw::GLvoid*)(deUintptr)m_po_bo1_dvec2_attr_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribLPointer() call failed.");

	gl.enableVertexAttribArray(m_po_bo1_dvec2_attr_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

	gl.vertexAttribLPointer(m_po_bo1_double_attr_location, 1, /* size */
							GL_DOUBLE, m_po_bo1_double_attr_stride,
							(const glw::GLvoid*)(deUintptr)m_po_bo1_double_attr_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribLPointer() call failed.");

	gl.enableVertexAttribArray(m_po_bo1_double_attr_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

	gl.vertexAttribPointer(m_po_bo1_float2_attr_location, 1, /* size */
						   GL_SHORT, GL_FALSE,				 /* normalized */
						   m_po_bo1_float2_attr_stride, (const glw::GLvoid*)(deUintptr)m_po_bo1_float2_attr_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed.");

	gl.enableVertexAttribArray(m_po_bo1_float2_attr_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

	gl.vertexAttribIPointer(m_po_bo1_int_attr_location, 1, /* size */
							GL_BYTE, m_po_bo1_int_attr_stride, (const glw::GLvoid*)(deUintptr)m_po_bo1_int_attr_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIPointer() call failed.");

	gl.enableVertexAttribArray(m_po_bo1_int_attr_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

	/* Set up BO2-sourced attributes */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.vertexAttribPointer(m_po_bo2_float_attr_location, 1, /* size */
						   GL_UNSIGNED_BYTE, GL_TRUE, m_po_bo2_float_attr_stride,
						   (const glw::GLvoid*)(deUintptr)m_po_bo2_float_attr_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed.");

	gl.enableVertexAttribArray(m_po_bo2_float_attr_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

	gl.vertexAttribLPointer(m_po_bo2_dmat4x2_attr_location + 0, 2, /* size */
							GL_DOUBLE, m_po_bo2_dmat4x2_attr_stride,
							(const glw::GLvoid*)(deUintptr)m_po_bo2_dmat4x2_attr_offset);
	gl.vertexAttribLPointer(m_po_bo2_dmat4x2_attr_location + 1, 2, /* size */
							GL_DOUBLE, m_po_bo2_dmat4x2_attr_stride,
							(const glw::GLvoid*)(m_po_bo2_dmat4x2_attr_offset + 2 * sizeof(double)));
	gl.vertexAttribLPointer(m_po_bo2_dmat4x2_attr_location + 2, 2, /* size */
							GL_DOUBLE, m_po_bo2_dmat4x2_attr_stride,
							(const glw::GLvoid*)(m_po_bo2_dmat4x2_attr_offset + 4 * sizeof(double)));
	gl.vertexAttribLPointer(m_po_bo2_dmat4x2_attr_location + 3, 2, /* size */
							GL_DOUBLE, m_po_bo2_dmat4x2_attr_stride,
							(const glw::GLvoid*)(m_po_bo2_dmat4x2_attr_offset + 6 * sizeof(double)));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribLPointer() call failed.");

	gl.enableVertexAttribArray(m_po_bo2_dmat4x2_attr_location + 0);
	gl.enableVertexAttribArray(m_po_bo2_dmat4x2_attr_location + 1);
	gl.enableVertexAttribArray(m_po_bo2_dmat4x2_attr_location + 2);
	gl.enableVertexAttribArray(m_po_bo2_dmat4x2_attr_location + 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call(s) failed.");

	/* Set up element binding */
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bo_id_indices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
}

/** Executes the test
 *
 *  @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult VAOTest::iterate()
{
	IterateStart();

	bool result = true;

	RequireExtension("GL_ARB_vertex_attrib_64bit");

	/* Initialize GL objects required to run the test */
	initBuffers();
	initBufferObjects();
	initProgramObject();
	initVAO();

	/* Activate the program object before we continue */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Iterate through all draw call combinations */
	for (int n_draw_call_type = 0; n_draw_call_type < DRAW_CALL_TYPE_COUNT; ++n_draw_call_type)
	{
		_draw_call_type draw_call = (_draw_call_type)n_draw_call_type;

		for (int n_instanced_draw_call = 0; n_instanced_draw_call <= 1; /* false & true */
			 ++n_instanced_draw_call)
		{
			bool instanced_draw_call = (n_instanced_draw_call == 1);

			for (int n_vertex_attrib_divisor = 0; n_vertex_attrib_divisor <= 1; /* 0 & non-zero divisor */
				 ++n_vertex_attrib_divisor)
			{
				bool zero_vertex_attrib_divisor = (n_vertex_attrib_divisor == 0);

				/* Execute the test */
				result &= executeTest(draw_call, instanced_draw_call, zero_vertex_attrib_divisor);
			} /* for (two vertex attrib divisor configurations) */
		}	 /* for (non-instanced & instanced draw calls) */
	}		  /* for (array-based & indiced draw calls) */

	/* Done */
	return IterateStop(result);
}

/** Verifies data that has been XFBed out by the draw call.
 *
 *  @param data                       XFBed data. Must not be NULL.
 *  @param draw_call                  Type of the draw call that was issued.
 *  @param instanced                  True if the draw call was instanced, false otherwise.
 *  @param zero_vertex_attrib_divisor True if a zero divisor was used for all checked attributes,
 *                                    false if the divisors were set to a value of m_nonzero_vertex_attrib_divisor.
 */
bool VAOTest::verifyXFBData(const void* data, _draw_call_type draw_call, bool instanced,
							bool zero_vertex_attrib_divisor)
{
	const float			 epsilon	  = 1e-5f;
	bool				 is_indiced   = (draw_call == DRAW_CALL_TYPE_ELEMENTS);
	const unsigned int   n_instances  = (instanced) ? m_n_draw_call_instances : 1;
	bool				 result		  = true;
	const unsigned char* xfb_data_ptr = (const unsigned char*)data;

	for (unsigned int n_instance = 0; n_instance < n_instances; ++n_instance)
	{
		/* Verify dmat3 data from BO1 has been exposed correctly */
		for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
		{
			unsigned int in_index  = n_element;
			unsigned int xfb_index = is_indiced ? m_bo_index_data[n_element] : n_element;

			if (!zero_vertex_attrib_divisor)
			{
				in_index = n_instance / m_nonzero_vertex_attrib_divisor;
			}

			const double* in_matrix_data_ptr =
				(const double*)(m_bo_1_data + (in_index)*m_po_bo1_dmat3_attr_stride + m_po_bo1_dmat3_attr_offset);
			const double* xfb_matrix_data_ptr =
				(const double*)(xfb_data_ptr + (m_n_batches * n_instance + xfb_index) * m_xfb_total_size +
								m_xfb_bo1_dmat3_offset);

			if (memcmp(in_matrix_data_ptr, xfb_matrix_data_ptr, m_xfb_bo1_dmat3_size) != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "BO1 dmat3 attribute values mismatch for batch ["
								   << n_element << "]"
												   ", expected:["
								   << in_matrix_data_ptr[0] << ", " << in_matrix_data_ptr[1] << ", "
								   << in_matrix_data_ptr[2] << ", " << in_matrix_data_ptr[3] << ", "
								   << in_matrix_data_ptr[4] << ", " << in_matrix_data_ptr[5] << ", "
								   << in_matrix_data_ptr[6] << ", " << in_matrix_data_ptr[7] << ", "
								   << in_matrix_data_ptr[8] << ", "
								   << "], XFBed out:[" << xfb_matrix_data_ptr[0] << ", " << xfb_matrix_data_ptr[1]
								   << ", " << xfb_matrix_data_ptr[2] << ", " << xfb_matrix_data_ptr[3] << ", "
								   << xfb_matrix_data_ptr[4] << ", " << xfb_matrix_data_ptr[5] << ", "
								   << xfb_matrix_data_ptr[6] << ", " << xfb_matrix_data_ptr[7] << ", "
								   << xfb_matrix_data_ptr[8] << ", "
								   << "]" << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}

		/* Verify float data from BO2 has been exposed correctly */
		for (unsigned int n_batch = 0; n_batch < m_n_batches; ++n_batch)
		{
			unsigned int	   in_index  = n_batch;
			const unsigned int xfb_index = is_indiced ? m_bo_index_data[n_batch] : n_batch;

			if (!zero_vertex_attrib_divisor)
			{
				in_index = n_instance / m_nonzero_vertex_attrib_divisor;
			}

			const unsigned char* in_ubyte_data_ptr =
				(const unsigned char*)(m_bo_2_data + (in_index)*m_po_bo2_float_attr_stride +
									   m_po_bo2_float_attr_offset);
			const float* xfb_float_data_ptr =
				(const float*)(xfb_data_ptr + (m_n_batches * n_instance + xfb_index) * m_xfb_total_size +
							   m_xfb_bo2_float_offset);
			float expected_value = ((float)*in_ubyte_data_ptr / 255.0f);

			if (de::abs(expected_value - *xfb_float_data_ptr) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "BO2 float attribute value mismatch for batch ["
								   << n_batch << "]"
												 ", expected: ["
								   << expected_value << "]"
														", XFBed out:["
								   << *xfb_float_data_ptr << "]" << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}

		/* Verify dvec2 data from BO1 has been exposed correctly */
		for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
		{
			unsigned int	   in_index  = n_element;
			const unsigned int xfb_index = is_indiced ? m_bo_index_data[n_element] : n_element;

			if (!zero_vertex_attrib_divisor)
			{
				in_index = n_instance / m_nonzero_vertex_attrib_divisor;
			}

			const double* in_dvec2_data_ptr =
				(const double*)(m_bo_1_data + (in_index)*m_po_bo1_dvec2_attr_stride + m_po_bo1_dvec2_attr_offset);
			const double* xfb_dvec2_data_ptr =
				(const double*)(xfb_data_ptr + (m_n_batches * n_instance + xfb_index) * m_xfb_total_size +
								m_xfb_bo1_dvec2_offset);

			if (memcmp(in_dvec2_data_ptr, in_dvec2_data_ptr, m_xfb_bo1_dvec2_size) != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "BO1 dvec2 attribute values mismatch for batch ["
								   << n_element << "]"
												   ", expected:["
								   << in_dvec2_data_ptr[0] << ", " << in_dvec2_data_ptr[1] << ", "
								   << "], XFBed out:[" << xfb_dvec2_data_ptr[0] << ", " << xfb_dvec2_data_ptr[1] << ", "
								   << "]" << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}

		/* Verify double data from BO1 has been exposed correctly */
		for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
		{
			unsigned int	   in_index  = n_element;
			const unsigned int xfb_index = is_indiced ? m_bo_index_data[n_element] : n_element;

			if (!zero_vertex_attrib_divisor)
			{
				in_index = n_instance / m_nonzero_vertex_attrib_divisor;
			}

			const double* in_double_data_ptr =
				(const double*)(m_bo_1_data + (in_index)*m_po_bo1_double_attr_stride + m_po_bo1_double_attr_offset);
			const double* xfb_double_data_ptr =
				(const double*)(xfb_data_ptr + (m_n_batches * n_instance + xfb_index) * m_xfb_total_size +
								m_xfb_bo1_double_offset);

			if (memcmp(in_double_data_ptr, xfb_double_data_ptr, m_xfb_bo1_double_size) != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "BO1 double attribute value mismatch for batch ["
								   << n_element << "]"
												   ", expected: ["
								   << *in_double_data_ptr << "]"
															 ", XFBed out:["
								   << *xfb_double_data_ptr << "]" << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}

		/* Verify dmat4x2 data from BO2 has been exposed correctly */
		for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
		{
			unsigned int	   in_index  = n_element;
			const unsigned int xfb_index = is_indiced ? m_bo_index_data[n_element] : n_element;

			if (!zero_vertex_attrib_divisor)
			{
				in_index = n_instance / m_nonzero_vertex_attrib_divisor;
			}

			const unsigned char* in_matrix_data_ptr =
				m_bo_2_data + (in_index)*m_po_bo2_dmat4x2_attr_stride + m_po_bo2_dmat4x2_attr_offset;
			const unsigned char* xfb_matrix_data_ptr =
				xfb_data_ptr + (m_n_batches * n_instance + xfb_index) * m_xfb_total_size + m_xfb_bo2_dmat4x2_offset;

			if (memcmp(in_matrix_data_ptr, xfb_matrix_data_ptr, m_xfb_bo2_dmat4x2_size) != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "BO2 dmat4x2 attribute values mismatch for batch ["
								   << n_element << "]"
												   ", expected:["
								   << in_matrix_data_ptr[0] << ", " << in_matrix_data_ptr[1] << ", "
								   << in_matrix_data_ptr[2] << ", " << in_matrix_data_ptr[3] << ", "
								   << in_matrix_data_ptr[4] << ", " << in_matrix_data_ptr[5] << ", "
								   << in_matrix_data_ptr[6] << ", " << in_matrix_data_ptr[7] << ", "
								   << "], XFBed out:[" << xfb_matrix_data_ptr[0] << ", " << xfb_matrix_data_ptr[1]
								   << ", " << xfb_matrix_data_ptr[2] << ", " << xfb_matrix_data_ptr[3] << ", "
								   << xfb_matrix_data_ptr[4] << ", " << xfb_matrix_data_ptr[5] << ", "
								   << xfb_matrix_data_ptr[6] << ", " << xfb_matrix_data_ptr[7] << ", "
								   << "]" << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}

		/* Verify int data from BO1 has been exposed correctly */
		for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
		{
			unsigned int	   in_index  = n_element;
			const unsigned int xfb_index = is_indiced ? m_bo_index_data[n_element] : n_element;

			if (!zero_vertex_attrib_divisor)
			{
				in_index = n_instance / m_nonzero_vertex_attrib_divisor;
			}

			const signed char* in_char_data_ptr =
				(const signed char*)(m_bo_1_data + (in_index)*m_po_bo1_int_attr_stride + m_po_bo1_int_attr_offset);
			const signed int* xfb_int_data_ptr =
				(const signed int*)(xfb_data_ptr + (m_n_batches * n_instance + xfb_index) * m_xfb_total_size +
									m_xfb_bo1_int_offset);

			if (de::abs((signed int)*in_char_data_ptr - *xfb_int_data_ptr) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "BO1 int attribute value mismatch for batch ["
								   << n_element << "]"
												   ", expected: ["
								   << (signed int)*in_char_data_ptr << "]"
																	   ", XFBed out:["
								   << *xfb_int_data_ptr << "]" << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}

		/* Verify float data from BO1 has been exposed correctly */
		for (unsigned int n_element = 0; n_element < m_n_batches; ++n_element)
		{
			unsigned int	   in_index  = n_element;
			const unsigned int xfb_index = is_indiced ? m_bo_index_data[n_element] : n_element;

			if (!zero_vertex_attrib_divisor)
			{
				in_index = n_instance / m_nonzero_vertex_attrib_divisor;
			}

			const unsigned short* in_short_data_ptr =
				(const unsigned short*)(m_bo_1_data + (in_index)*m_po_bo1_float2_attr_stride +
										m_po_bo1_float2_attr_offset);
			const float* xfb_float_data_ptr =
				(const float*)(xfb_data_ptr + (m_n_batches * n_instance + xfb_index) * m_xfb_total_size +
							   m_xfb_bo1_float2_offset);

			if (de::abs(*in_short_data_ptr - *xfb_float_data_ptr) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "BO1 float attribute value mismatch for batch ["
								   << n_element << "]"
												   ", expected: ["
								   << (signed int)*in_short_data_ptr << "]"
																		", XFBed out:["
								   << *xfb_float_data_ptr << "]" << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	} /* for (all instances) */

	return result;
}

} /* namespace VertexAttrib64Bit */

namespace gl4cts
{

VertexAttrib64BitTests::VertexAttrib64BitTests(deqp::Context& context)
	: TestCaseGroup(context, "vertex_attrib_64bit", "Verifes GL_ARB_vertex_attrib_64bit functionality")
{
	/* Nothing to be done here */
}

void VertexAttrib64BitTests::init(void)
{
	addChild(new VertexAttrib64Bit::ApiErrorsTest(m_context));
	addChild(new VertexAttrib64Bit::GetVertexAttribTest(m_context));
	addChild(new VertexAttrib64Bit::LimitTest(m_context));
	addChild(new VertexAttrib64Bit::VAOTest(m_context));
}

} /* namespace gl4cts */
