#ifndef _GL3CCLIPDISTANCE_HPP
#define _GL3CCLIPDISTANCE_HPP
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
 */ /*!
 * \file  gl3cClipDistance.hpp
 * \brief Conformance tests for Clip Distance feature functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "gluDefs.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

/* Includes. */
#include <cstring>
#include <map>
#include <typeinfo>
#include <vector>

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

namespace gl3cts
{
namespace ClipDistance
{
namespace Utility
{
/** @class Program
 *
 *  @brief GLSL program encapsulation class.
 */
class Program
{
public:
	/* Public type definitions */

	/** @struct CompilationStatus
	 *
	 *  @brief GLSL shader encapsulation class.
	 */
	struct CompilationStatus
	{
		glw::GLuint shader_id;
		glw::GLint  shader_compilation_status;
		std::string shader_log;
	};

	/** @struct CompilationStatus
	 *
	 *  @brief GLSL shader encapsulation class.
	 */
	struct LinkageStatus
	{
		glw::GLuint program_id;
		glw::GLint  program_linkage_status;
		std::string program_linkage_log;
	};

	/* Public member variables */
	Program(const glw::Functions& gl, const std::string& vertex_shader_code, const std::string& fragment_shader_code,
			std::vector<std::string> transform_feedback_varyings = std::vector<std::string>());

	~Program();

	const CompilationStatus& VertexShaderStatus() const;
	const CompilationStatus& FragmentShaderStatus() const;
	const LinkageStatus&	 ProgramStatus() const;

	void UseProgram() const;

private:
	/* Private member variables */
	CompilationStatus m_vertex_shader_status;
	CompilationStatus m_fragment_shader_status;
	LinkageStatus	 m_program_status;

	const glw::Functions& m_gl;

	/* Private member functions */
	CompilationStatus compileShader(const glw::GLenum shader_type, const glw::GLchar* const* shader_code);

	LinkageStatus linkShaders(const CompilationStatus& vertex_shader, const CompilationStatus& fragment_shader,
							  std::vector<std::string>& transform_feedback_varyings);
};
/* Program class */

/** @class Framebuffer
 *
 *  @brief OpenGL's Framebuffer encapsulation class.
 *
 *  @note Created framebuffer is red-color-only and float type.
 */
class Framebuffer
{
public:
	Framebuffer(const glw::Functions& gl, const glw::GLsizei size_x, const glw::GLsizei size_y);
	~Framebuffer();

	bool					  isValid();
	void					  bind();
	std::vector<glw::GLfloat> readPixels();
	void					  clear();

private:
	const glw::Functions& m_gl;
	const glw::GLsizei	m_size_x;
	const glw::GLsizei	m_size_y;
	glw::GLuint			  m_framebuffer_id;
	glw::GLuint			  m_renderbuffer_id;
};
/* Framebuffer class */

/** @class Vertex Array Object
 *
 *  @brief OpenGL's Vertex Array Object encapsulation class.
 */
class VertexArrayObject
{
public:
	VertexArrayObject(const glw::Functions& gl, const glw::GLenum primitive_type); // create empty vao
	~VertexArrayObject();

	void bind();
	void draw(glw::GLuint first, glw::GLuint count);
	void drawWithTransformFeedback(glw::GLuint first, glw::GLuint count, bool discard_rasterizer);

private:
	const glw::Functions& m_gl;
	glw::GLuint			  m_vertex_array_object_id;
	glw::GLenum			  m_primitive_type;
};
/* VertexArrayObject class */

/** @class Vertex Buffer Object
 *
 *  @brief OpenGL's Vertex Buffer Object encapsulation template class.
 *
 *  @note Input data type is a template parameter.
 */
template <class T>
class VertexBufferObject
{
public:
	VertexBufferObject(const glw::Functions& gl, const glw::GLenum target, std::vector<T> data);
	~VertexBufferObject();

	bool bind();
	bool useAsShaderInput(Program program, std::string input_attribute_name, glw::GLint number_of_components);
	std::vector<T> readBuffer();

private:
	const glw::Functions& m_gl;
	glw::GLuint			  m_vertex_buffer_object_id;
	glw::GLenum			  m_target;
	glw::GLsizei		  m_size;

	std::vector<glw::GLint> m_enabled_arrays;
};
/* VertexBufferObject template class */

std::string preprocessCode(std::string source, std::string key, std::string value);
std::string itoa(glw::GLint i);
} /* Utility namespace */

/** @class Tests
 *
 *  @brief Clip distance test group.
 */
class Tests : public deqp::TestCaseGroup
{
public:
	/* Public member functions */
	Tests(deqp::Context& context);

	void init();

private:
	/* Private member functions */
	Tests(const Tests& other);
	Tests& operator=(const Tests& other);
};

/** @class CoverageTest
 *
 *  @brief Clip distance API Coverage test cases.
 */
class CoverageTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CoverageTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CoverageTest(const CoverageTest& other);
	CoverageTest& operator=(const CoverageTest& other);

	bool MaxClipDistancesValueTest(const glw::Functions& gl);
	bool EnableDisableTest(const glw::Functions& gl);
	bool MaxClipDistancesValueInVertexShaderTest(const glw::Functions& gl);
	bool MaxClipDistancesValueInFragmentShaderTest(const glw::Functions& gl);
	bool ClipDistancesValuePassing(const glw::Functions& gl);

	/* Private member variables */
	glw::GLint m_gl_max_clip_distances_value;

	/* Private static constants */
	static const glw::GLchar* m_vertex_shader_code_case_0;
	static const glw::GLchar* m_fragment_shader_code_case_0;

	static const glw::GLchar* m_vertex_shader_code_case_1;
	static const glw::GLchar* m_fragment_shader_code_case_1;

	static const glw::GLchar* m_vertex_shader_code_case_2;
	static const glw::GLchar* m_fragment_shader_code_case_2;
};

/** @class FunctionalTest
 *
 *  @brief Clip distance Functional test cases.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions */
	FunctionalTest(deqp::Context& context);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	FunctionalTest(const FunctionalTest& other);
	FunctionalTest& operator=(const FunctionalTest& other);

	std::string prepareVertexShaderCode(bool explicit_redeclaration, bool dynamic_setter, glw::GLuint clip_count,
										glw::GLuint clip_function, glw::GLenum primitive_type);

	gl3cts::ClipDistance::Utility::VertexBufferObject<glw::GLfloat>* prepareGeometry(const glw::Functions& gl,
																					 const glw::GLenum primitive_type);

	bool checkResults(glw::GLenum primitive_type, glw::GLuint clip_function, std::vector<glw::GLfloat>& results);

	/* Private member variables */
	glw::GLint m_gl_max_clip_distances_value;

	/* Private static constants */
	static const glw::GLchar* m_vertex_shader_code;
	static const glw::GLchar* m_fragment_shader_code;
	static const glw::GLchar* m_dynamic_array_setter;
	static const glw::GLchar* m_static_array_setter;
	static const glw::GLchar* m_explicit_redeclaration;
	static const glw::GLchar* m_clip_function[];
	static const glw::GLuint  m_clip_function_count;

	static const glw::GLenum m_primitive_types[];
	static const glw::GLenum m_primitive_indices[];
	static const glw::GLuint m_primitive_types_count;

	static const glw::GLfloat m_expected_integral[];
};

/** @class NegativeTest
 *
 *  @brief Clip distance API Negative test cases.
 */
class NegativeTest : public deqp::TestCase
{
public:
	/* Public member functions */
	NegativeTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	NegativeTest(const NegativeTest& other);
	NegativeTest& operator=(const NegativeTest& other);

	bool testClipVertexBuildingErrors(const glw::Functions& gl);
	bool testMaxClipDistancesBuildingErrors(const glw::Functions& gl);
	bool testClipDistancesRedeclarationBuildingErrors(const glw::Functions& gl);

	/* Private static constants */
	static const glw::GLchar* m_vertex_shader_code_case_0;
	static const glw::GLchar* m_vertex_shader_code_case_1;
	static const glw::GLchar* m_vertex_shader_code_case_2;
	static const glw::GLchar* m_fragment_shader_code;
};
} /* ClipDistance namespace */
} /* gl3cts namespace */

/* Template classes' implementation */

/** @brief Vertex Buffer Object constructor.
 *
 *  @note It silently binds VAO to OpenGL.
 *
 *  @param [in] gl               OpenGL functions access.
 *  @param [in] target           Binding target of the VBO.
 *  @param [in] data             Data of the buffer (may be empty).
 */
template <class T>
gl3cts::ClipDistance::Utility::VertexBufferObject<T>::VertexBufferObject(const glw::Functions& gl,
																		 const glw::GLenum target, std::vector<T> data)
	: m_gl(gl), m_vertex_buffer_object_id(0), m_target(target), m_size(0)
{
	m_gl.genBuffers(1, &m_vertex_buffer_object_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers call failed.");

	if (m_vertex_buffer_object_id)
	{
		m_size = (glw::GLsizei)(sizeof(T) * data.size());

		bind();

		m_gl.bufferData(m_target, m_size, &data[0], GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferData call failed.");
	}
}

/** @brief Vertex Buffer Object destructor. */
template <class T>
gl3cts::ClipDistance::Utility::VertexBufferObject<T>::~VertexBufferObject()
{
	m_gl.deleteBuffers(1, &m_vertex_buffer_object_id); /* Delete silently unbinds the buffer. */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDeleteBuffers call failed.");

	for (std::vector<glw::GLint>::iterator i_enabled_array = m_enabled_arrays.begin();
		 i_enabled_array != m_enabled_arrays.end(); ++i_enabled_array)
	{
		m_gl.disableVertexAttribArray(*i_enabled_array);
	}
}

/** @brief Bind Vertex Buffer Object to its target.
 *
 *  @note It binds also to indexed binding point for GL_TRANSFORM_FEEDBACK_BUFFER target.
 */
template <class T>
bool gl3cts::ClipDistance::Utility::VertexBufferObject<T>::bind()
{
	if (m_vertex_buffer_object_id)
	{
		m_gl.bindBuffer(m_target, m_vertex_buffer_object_id);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer call failed.");

		if (m_target == GL_TRANSFORM_FEEDBACK_BUFFER)
		{
			m_gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_vertex_buffer_object_id);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferBase call failed.");
		}

		return true;
	}

	return false;
}

/** @brief Use VBO as attribute vertex array.
 *
 *  @note It silently binds VBO.
 *
 *  @param [in] program                 GLSL Program to which VBO shall be bound.
 *  @param [in] input_attribute_name    Name of GLSL asttribute.
 *  @param [in] number_of_components    Number of attribute's components.
 *
 *  @return True on success, false otherwise.
 */
template <class T>
bool gl3cts::ClipDistance::Utility::VertexBufferObject<T>::useAsShaderInput(Program		program,
																			std::string input_attribute_name,
																			glw::GLint  number_of_components)
{
	if (program.ProgramStatus().program_id)
	{
		glw::GLint location = m_gl.getAttribLocation(program.ProgramStatus().program_id, input_attribute_name.c_str());
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetAttribLocation call failed.");

		if (location >= 0)
		{
			const std::type_info& buffer_type = typeid(T);
			const std::type_info& float_type  = typeid(glw::GLfloat);
			const std::type_info& int_type	= typeid(glw::GLint);

			m_gl.enableVertexAttribArray(location);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glEnableVertexAttribArray call failed.");
			m_enabled_arrays.push_back(location);

			bind();

			if (buffer_type == float_type)
			{
				m_gl.vertexAttribPointer(location, number_of_components, GL_FLOAT, false, 0, NULL);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glVertexAttribPointer call failed.");
			}
			else if (buffer_type == int_type)
			{
				m_gl.vertexAttribIPointer(location, number_of_components, GL_FLOAT, 0, NULL);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glVertexAttribIPointer call failed.");
			}
			else
			{
				return false;
			}

			return true;
		}
	}

	return false;
}

/** @brief Read VBO content (potentially set by transform feedback).
 *
 *  @return Content of VBO.
 */
template <class T>
std::vector<T> gl3cts::ClipDistance::Utility::VertexBufferObject<T>::readBuffer()
{
	std::vector<T> buffer_data(m_size / sizeof(T));

	bind();

	glw::GLvoid* results = m_gl.mapBuffer(m_target, GL_READ_ONLY);

	if (results)
	{
		memcpy(&buffer_data[0], results, m_size);
	}

	m_gl.unmapBuffer(m_target);

	return buffer_data;
}
#endif // _GL3CCLIPDISTANCE_HPP
