#ifndef _GL3CGPUSHADER5TESTS_HPP
#define _GL3CGPUSHADER5TESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file  gl3cGPUShader5Tests.hpp
 * \brief Declares test classes for "GPU Shader 5" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"
#include "tcuVector.hpp"
#include <queue>

namespace gl3cts
{
class Utils
{
public:
	/* Public type definitions */
	/* Defines GLSL variable type */
	enum _variable_type
	{
		VARIABLE_TYPE_FLOAT,
		VARIABLE_TYPE_INT,
		VARIABLE_TYPE_IVEC2,
		VARIABLE_TYPE_IVEC3,
		VARIABLE_TYPE_IVEC4,
		VARIABLE_TYPE_UINT,
		VARIABLE_TYPE_UVEC2,
		VARIABLE_TYPE_UVEC3,
		VARIABLE_TYPE_UVEC4,
		VARIABLE_TYPE_VEC2,
		VARIABLE_TYPE_VEC3,
		VARIABLE_TYPE_VEC4,

		/* Always last */
		VARIABLE_TYPE_UNKNOWN
	};

	/** Store information about program object
	 *
	 **/
	struct programInfo
	{
		programInfo(deqp::Context& context);
		~programInfo();

		void build(const glw::GLchar* fragment_shader_code, const glw::GLchar* vertex_shader_code);
		void compile(glw::GLuint shader_id, const glw::GLchar* shader_code) const;
		void link() const;

		void setUniform(Utils::_variable_type type, const glw::GLchar* name, const glw::GLvoid* data);

		deqp::Context& m_context;

		glw::GLuint m_fragment_shader_id;
		glw::GLuint m_program_object_id;
		glw::GLuint m_vertex_shader_id;
	};

	/* Public static methods */
	static void replaceToken(const glw::GLchar* token, size_t& search_position, const glw::GLchar* text,
							 std::string& string);
};

/** Implements ImplicitConversions test, description follows:
 *
 * Verifies that compiler accepts implicit conversions and the results of
 * implicit conversions are the same as explicit conversions.
 *
 * Steps:
 * - prepare a program consisting of vertex and fragment shader; Vertex shader
 * should implement the following snippet:
 *
 *   uniform SOURCE_TYPE u1;
 *   uniform SOURCE_TYPE u2;
 *
 *   out vec4 result;
 *
 *   void main()
 *   {
 *     DESTINATION_TYPE v = 0;
 *
 *     v = DESTINATION_TYPE(u2) - u1;
 *
 *     result = vec4(0.0, 0.0, 0.0, 0.0);
 *
 *     if (0 == v)
 *     {
 *       result = vec4(1.0, 1.0, 1.0, 1.0);
 *     }
 *   }
 *
 * Fragment shader should pass result from vertex shader to output color.
 * - it is expected that program will link without any errors;
 * - set u1 and u2 with different values;
 * - draw fullscreen quad;
 * - it is expected that drawn image is filled with black color;
 * - set u1 and u2 with the same value;
 * - draw fullscreen quad;
 * - it is expected that drawn image is filled with white color;
 *
 * Repeat steps for the following pairs:
 *
 *   int   - uint
 *   int   - float
 *   ivec2 - uvec2
 *   ivec3 - uvec3
 *   ivec4 - uvec4
 *   ivec2 - vec2
 *   ivec3 - vec3
 *   ivec4 - vec4
 *   uint  - float
 *   uvec2 - vec2
 *   uvec3 - vec3
 *   uvec4 - vec4
 **/
class GPUShader5ImplicitConversionsTest : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShader5ImplicitConversionsTest(deqp::Context& context);
	GPUShader5ImplicitConversionsTest(deqp::Context& context, const char* name, const char* description);

	void								 deinit();
	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected methods*/
	void testInit();
	void verifyImage(glw::GLuint color, bool is_expected) const;

private:
	/* Private type definitions */
	struct testCase
	{
		const glw::GLchar*	m_destination_type;
		bool				  m_is_white_expected;
		const glw::GLchar*	m_source_type;
		Utils::_variable_type m_source_variable_type;
		const void*			  m_u1_data;
		const void*			  m_u2_data;
	};

	/* Private methods */
	void executeTestCase(const testCase& test_case);
	std::string getFragmentShader();
	std::string getVertexShader(const glw::GLchar* destination_type, const glw::GLchar* source_type);

	/* Private fields */
	glw::GLuint m_fbo_id;
	glw::GLuint m_tex_id;
	glw::GLuint m_vao_id;

	/* Private constants */
	static const glw::GLsizei m_width;
	static const glw::GLsizei m_height;
};

/** Implements FunctionOverloading test, description follows:
 *
 * Verifies that compiler accepts overloaded functions and selects proper one.
 *
 * Steps:
 * - prepare a program consisting of vertex and fragment shader; Vertex shader
 * should implement the following snippet:
 *
 *   uniform ivec4 u1;
 *   uniform uvec4 u2;
 *
 *   out vec4 result;
 *
 *   vec4 f(in vec4 a, in vec4 b) // first
 *   {
 *     return a * b;
 *   }
 *
 *   vec4 f(in uvec4 a, in uvec4 b) // second
 *   {
 *     return a - b;
 *   }
 *
 *   void main()
 *   {
 *     result = f(u1, u2);
 *   }
 *
 * Fragment shader should pass result from vertex shader to output color.
 * - it is expected that program will link without any errors;
 * - set u1 and u2 with different positive values;
 * - draw fullscreen quad;
 * - it is expected that drawn image is filled with non-black color;
 * - set u1 and u2 with the same positive value;
 * - draw fullscreen quad;
 * - it is expected that drawn image is filled with black color;
 *
 * The second function should be considered a better match as u2 is exact
 * match.
 **/
class GPUShader5FunctionOverloadingTest : public GPUShader5ImplicitConversionsTest
{
public:
	/* Public methods */
	GPUShader5FunctionOverloadingTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void execute(const glw::GLint* u1_data, const glw::GLuint* u2_data, bool is_black_expected);
};

/** Implements FunctionOverloading test, description follows:
 *
 * Verifies functions: floatBitsTo* and *BitsToFloat work as expected.
 *
 * Steps:
 * - prepare a program consisting of vertex and fragment shader; Vertex shader
 * should implement the following snippet:
 *
 *   uniform T1 value;
 *   uniform T2 expected_result;
 *
 *   out vec4 result;
 *
 *   void main()
 *   {
 *     result = 1;
 *
 *     T2 ret_val = TESTED_FUNCTION(value);
 *
 *     if (expected_result != ret_val)
 *     {
 *       result = 0;
 *     }
 *   }
 *
 * Fragment shader should pass result from vertex shader to output color.
 * - it is expected that program will link without any errors;
 * - set uniforms with "matching" values;
 * - draw fullscreen quad;
 * - inspect drawn image.
 *
 * Repeat steps to test the following functions:
 * - floatBitsToInt
 * - floatBitsToUint
 * - intBitsToFloat
 * - uintBitsToFloat
 *
 * Select "value" and "expected_result" to provoke both "white" and "black"
 * results.
 **/
class GPUShader5FloatEncodingTest : public GPUShader5ImplicitConversionsTest
{
public:
	/* Public methods */
	GPUShader5FloatEncodingTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private types */
	struct valueInfo
	{
		const Utils::_variable_type m_type;
		const glw::GLchar*			m_type_name;
		const void*					m_data;
	};

	struct testCase
	{
		const valueInfo	m_expected_value;
		const valueInfo	m_value;
		const glw::GLchar* m_function_name;
		bool			   m_is_white_expected;
	};

	/* Private methods */
	void execute(const testCase& test_case);
	std::string getVertexShader(const testCase& test_case) const;
};

/** Group class for GPU Shader 5 conformance tests */
class GPUShader5Tests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	GPUShader5Tests(deqp::Context& context);
	virtual ~GPUShader5Tests()
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	GPUShader5Tests(const GPUShader5Tests&);
	GPUShader5Tests& operator=(const GPUShader5Tests&);
};
} /* gl3cts namespace */

#endif // _GL3CGPUSHADER5TESTS_HPP
