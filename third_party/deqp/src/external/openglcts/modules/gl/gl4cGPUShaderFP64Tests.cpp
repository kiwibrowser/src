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
 * \file  gl4GPUShaderFP64Tests.cpp
 * \brief Implements conformance tests for "GPU Shader FP64" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cGPUShaderFP64Tests.hpp"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "tcuMatrix.hpp"
#include "tcuTestLog.hpp"

#include <iomanip>

#include "deMath.h"
#include "deUniquePtr.hpp"
#include "tcuMatrixUtil.hpp"
#include "tcuVectorUtil.hpp"

#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>

namespace gl4cts
{

const glw::GLenum Utils::programInfo::ARB_COMPUTE_SHADER = 0x91B9;

/** Constructor
 *
 * @param context Test context
 **/
Utils::programInfo::programInfo(deqp::Context& context)
	: m_context(context)
	, m_compute_shader_id(0)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_object_id(0)
	, m_tesselation_control_shader_id(0)
	, m_tesselation_evaluation_shader_id(0)
	, m_vertex_shader_id(0)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Utils::programInfo::~programInfo()
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure program object is no longer used by GL */
	gl.useProgram(0);

	/* Clean program object */
	if (0 != m_program_object_id)
	{
		gl.deleteProgram(m_program_object_id);
		m_program_object_id = 0;
	}

	/* Clean shaders */
	if (0 != m_compute_shader_id)
	{
		gl.deleteShader(m_compute_shader_id);
		m_compute_shader_id = 0;
	}

	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);
		m_fragment_shader_id = 0;
	}

	if (0 != m_geometry_shader_id)
	{
		gl.deleteShader(m_geometry_shader_id);
		m_geometry_shader_id = 0;
	}

	if (0 != m_tesselation_control_shader_id)
	{
		gl.deleteShader(m_tesselation_control_shader_id);
		m_tesselation_control_shader_id = 0;
	}

	if (0 != m_tesselation_evaluation_shader_id)
	{
		gl.deleteShader(m_tesselation_evaluation_shader_id);
		m_tesselation_evaluation_shader_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);
		m_vertex_shader_id = 0;
	}
}

/** Build program
 *
 * @param compute_shader_code                Compute shader source code
 * @param fragment_shader_code               Fragment shader source code
 * @param geometry_shader_code               Geometry shader source code
 * @param tesselation_control_shader_code    Tesselation control shader source code
 * @param tesselation_evaluation_shader_code Tesselation evaluation shader source code
 * @param vertex_shader_code                 Vertex shader source code
 * @param varying_names                      Array of strings containing names of varyings to be captured with transfrom feedback
 * @param n_varying_names                    Number of varyings to be captured with transfrom feedback
 **/
void Utils::programInfo::build(const glw::GLchar* compute_shader_code, const glw::GLchar* fragment_shader_code,
							   const glw::GLchar* geometry_shader_code,
							   const glw::GLchar* tesselation_control_shader_code,
							   const glw::GLchar* tesselation_evaluation_shader_code,
							   const glw::GLchar* vertex_shader_code, const glw::GLchar* const* varying_names,
							   glw::GLuint n_varying_names)
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create shader objects and compile */
	if (0 != compute_shader_code)
	{
		m_compute_shader_id = gl.createShader(ARB_COMPUTE_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_compute_shader_id, compute_shader_code);
	}

	if (0 != fragment_shader_code)
	{
		m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_fragment_shader_id, fragment_shader_code);
	}

	if (0 != geometry_shader_code)
	{
		m_geometry_shader_id = gl.createShader(GL_GEOMETRY_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_geometry_shader_id, geometry_shader_code);
	}

	if (0 != tesselation_control_shader_code)
	{
		m_tesselation_control_shader_id = gl.createShader(GL_TESS_CONTROL_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_tesselation_control_shader_id, tesselation_control_shader_code);
	}

	if (0 != tesselation_evaluation_shader_code)
	{
		m_tesselation_evaluation_shader_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_tesselation_evaluation_shader_id, tesselation_evaluation_shader_code);
	}

	if (0 != vertex_shader_code)
	{
		m_vertex_shader_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_vertex_shader_id, vertex_shader_code);
	}

	/* Create program object */
	m_program_object_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

	/* Set up captyured varyings' names */
	if (0 != n_varying_names)
	{
		gl.transformFeedbackVaryings(m_program_object_id, n_varying_names, varying_names, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TransformFeedbackVaryings");
	}

	/* Link program */
	link();
}

/** Compile shader
 *
 * @param shader_id   Shader object id
 * @param shader_code Shader source code
 **/
void Utils::programInfo::compile(glw::GLuint shader_id, const glw::GLchar* shader_code) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Compilation status */
	glw::GLint status = GL_FALSE;

	/* Set source code */
	gl.shaderSource(shader_id, 1 /* count */, &shader_code, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");

	/* Compile */
	gl.compileShader(shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CompileShader");

	/* Get compilation status */
	gl.getShaderiv(shader_id, GL_COMPILE_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

	/* Log compilation error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Error log length */
		gl.getShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

		/* Prepare storage */
		message.resize(length);

		/* Get error log */
		gl.getShaderInfoLog(shader_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

		/* Log */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to compile shader:\n"
											<< &message[0] << "\nShader source\n"
											<< shader_code << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to compile shader");
	}
}

/** Attach shaders and link program
 *
 **/
void Utils::programInfo::link() const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Link status */
	glw::GLint status = GL_FALSE;

	/* Attach shaders */
	if (0 != m_compute_shader_id)
	{
		gl.attachShader(m_program_object_id, m_compute_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_fragment_shader_id)
	{
		gl.attachShader(m_program_object_id, m_fragment_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_geometry_shader_id)
	{
		gl.attachShader(m_program_object_id, m_geometry_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_tesselation_control_shader_id)
	{
		gl.attachShader(m_program_object_id, m_tesselation_control_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_tesselation_evaluation_shader_id)
	{
		gl.attachShader(m_program_object_id, m_tesselation_evaluation_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_vertex_shader_id)
	{
		gl.attachShader(m_program_object_id, m_vertex_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	/* Link */
	gl.linkProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

	/* Get link status */
	gl.getProgramiv(m_program_object_id, GL_LINK_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

	/* Log link error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Get error log length */
		gl.getProgramiv(m_program_object_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

		message.resize(length);

		/* Get error log */
		gl.getProgramInfoLog(m_program_object_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

		/* Log */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to link program:\n"
											<< &message[0] << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to link program");
	}
}

/** Retrieves base type of user-provided variable type. (eg. VARIABLE_TYPE_DOUBLE for double-precision
 *  matrix types.
 *
 *  @param type Variable type to return base type for.
 *
 *  @return Requested value or VARAIBLE_TYPE_UNKNOWN if @param type was not recognized.
 **/
Utils::_variable_type Utils::getBaseVariableType(_variable_type type)
{
	_variable_type result = VARIABLE_TYPE_UNKNOWN;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
	{
		result = VARIABLE_TYPE_BOOL;

		break;
	}

	case VARIABLE_TYPE_DOUBLE:
	case VARIABLE_TYPE_DMAT2:
	case VARIABLE_TYPE_DMAT2X3:
	case VARIABLE_TYPE_DMAT2X4:
	case VARIABLE_TYPE_DMAT3:
	case VARIABLE_TYPE_DMAT3X2:
	case VARIABLE_TYPE_DMAT3X4:
	case VARIABLE_TYPE_DMAT4:
	case VARIABLE_TYPE_DMAT4X2:
	case VARIABLE_TYPE_DMAT4X3:
	case VARIABLE_TYPE_DVEC2:
	case VARIABLE_TYPE_DVEC3:
	case VARIABLE_TYPE_DVEC4:
	{
		result = VARIABLE_TYPE_DOUBLE;

		break;
	}

	case VARIABLE_TYPE_INT:
	case VARIABLE_TYPE_IVEC2:
	case VARIABLE_TYPE_IVEC3:
	case VARIABLE_TYPE_IVEC4:
	{
		result = VARIABLE_TYPE_INT;

		break;
	}

	case VARIABLE_TYPE_UINT:
	case VARIABLE_TYPE_UVEC2:
	case VARIABLE_TYPE_UVEC3:
	case VARIABLE_TYPE_UVEC4:
	{
		result = VARIABLE_TYPE_UINT;

		break;
	}

	case VARIABLE_TYPE_FLOAT:
	case VARIABLE_TYPE_MAT2:
	case VARIABLE_TYPE_MAT2X3:
	case VARIABLE_TYPE_MAT2X4:
	case VARIABLE_TYPE_MAT3:
	case VARIABLE_TYPE_MAT3X2:
	case VARIABLE_TYPE_MAT3X4:
	case VARIABLE_TYPE_MAT4:
	case VARIABLE_TYPE_MAT4X2:
	case VARIABLE_TYPE_MAT4X3:
	case VARIABLE_TYPE_VEC2:
	case VARIABLE_TYPE_VEC3:
	case VARIABLE_TYPE_VEC4:
	{
		result = VARIABLE_TYPE_FLOAT;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized variable type");
	}
	} /* switch (type) */

	return result;
}

/** Returns size (in bytes) of a single component of a base variable type.
 *
 *  @param type Base variable type to use for the query.
 *
 *  @return Requested value or 0 if @param type was not recognized.
 **/
unsigned int Utils::getBaseVariableTypeComponentSize(_variable_type type)
{
	unsigned int result = 0;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
		result = sizeof(bool);
		break;
	case VARIABLE_TYPE_DOUBLE:
		result = sizeof(double);
		break;
	case VARIABLE_TYPE_FLOAT:
		result = sizeof(float);
		break;
	case VARIABLE_TYPE_INT:
		result = sizeof(int);
		break;
	case VARIABLE_TYPE_UINT:
		result = sizeof(unsigned int);
		break;

	default:
	{
		TCU_FAIL("Unrecognized variable type");
	}
	} /* switch (type) */

	return result;
}

/** Returns component, corresponding to user-specified index
 *  (eg. index:0 corresponds to 'x', index:1 corresponds to 'y',
 *  and so on.
 *
 *  @param index Component index.
 *
 *  @return As per description.
 **/
unsigned char Utils::getComponentAtIndex(unsigned int index)
{
	unsigned char result = '?';

	switch (index)
	{
	case 0:
		result = 'x';
		break;
	case 1:
		result = 'y';
		break;
	case 2:
		result = 'z';
		break;
	case 3:
		result = 'w';
		break;

	default:
	{
		TCU_FAIL("Unrecognized component index");
	}
	}

	return result;
}

/** Get _variable_type representing double-precision type with given dimmensions
 *
 * @param n_columns Number of columns
 * @param n_row     Number of rows
 *
 * @return Corresponding _variable_type
 **/
Utils::_variable_type Utils::getDoubleVariableType(glw::GLuint n_columns, glw::GLuint n_rows)
{
	Utils::_variable_type type = VARIABLE_TYPE_UNKNOWN;

	static const _variable_type types[4][4] = {
		{ VARIABLE_TYPE_DOUBLE, VARIABLE_TYPE_DVEC2, VARIABLE_TYPE_DVEC3, VARIABLE_TYPE_DVEC4 },
		{ VARIABLE_TYPE_UNKNOWN, VARIABLE_TYPE_DMAT2, VARIABLE_TYPE_DMAT2X3, VARIABLE_TYPE_DMAT2X4 },
		{ VARIABLE_TYPE_UNKNOWN, VARIABLE_TYPE_DMAT3X2, VARIABLE_TYPE_DMAT3, VARIABLE_TYPE_DMAT3X4 },
		{ VARIABLE_TYPE_UNKNOWN, VARIABLE_TYPE_DMAT4X2, VARIABLE_TYPE_DMAT4X3, VARIABLE_TYPE_DMAT4 }
	};

	type = types[n_columns - 1][n_rows - 1];

	return type;
}

/** Returns a single-precision equivalent of a double-precision floating-point variable
 *  type.
 *
 *  @param type Double-precision variable type. Only VARIABLE_TYPE_D* variable types
 *              are accepted.
 *
 *  @return Requested GLSL type.
 **/
std::string Utils::getFPVariableTypeStringForVariableType(_variable_type type)
{
	std::string result = "[?]";

	switch (type)
	{
	case VARIABLE_TYPE_DOUBLE:
		result = "float";
		break;
	case VARIABLE_TYPE_DMAT2:
		result = "mat2";
		break;
	case VARIABLE_TYPE_DMAT2X3:
		result = "mat2x3";
		break;
	case VARIABLE_TYPE_DMAT2X4:
		result = "mat2x4";
		break;
	case VARIABLE_TYPE_DMAT3:
		result = "mat3";
		break;
	case VARIABLE_TYPE_DMAT3X2:
		result = "mat3x2";
		break;
	case VARIABLE_TYPE_DMAT3X4:
		result = "mat3x4";
		break;
	case VARIABLE_TYPE_DMAT4:
		result = "mat4";
		break;
	case VARIABLE_TYPE_DMAT4X2:
		result = "mat4x2";
		break;
	case VARIABLE_TYPE_DMAT4X3:
		result = "mat4x3";
		break;
	case VARIABLE_TYPE_DVEC2:
		result = "vec2";
		break;
	case VARIABLE_TYPE_DVEC3:
		result = "vec3";
		break;
	case VARIABLE_TYPE_DVEC4:
		result = "vec4";
		break;

	default:
	{
		TCU_FAIL("Unrecognized variable type");
	}
	}; /* switch (type) */

	return result;
}

/** Returns GL data type enum corresponding to user-provided base variable type.
 *
 *  @param type Base variable type to return corresponding GLenum value for.
 *
 *  @return Corresponding GLenum value or GL_NONE if the input value was not
 *          recognized.
 **/
glw::GLenum Utils::getGLDataTypeOfBaseVariableType(_variable_type type)
{
	glw::GLenum result = GL_NONE;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
		result = GL_BOOL;
		break;
	case VARIABLE_TYPE_DOUBLE:
		result = GL_DOUBLE;
		break;
	case VARIABLE_TYPE_FLOAT:
		result = GL_FLOAT;
		break;
	case VARIABLE_TYPE_INT:
		result = GL_INT;
		break;
	case VARIABLE_TYPE_UINT:
		result = GL_UNSIGNED_INT;
		break;

	default:
	{
		TCU_FAIL("Unrecognized variable type");
	}
	}

	return result;
}

/** Return GLenum representing given <type>
 *
 * @param type Type of variable
 *
 * @return GL enumeration
 **/
glw::GLenum Utils::getGLDataTypeOfVariableType(_variable_type type)
{
	glw::GLenum result = GL_NONE;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
		result = GL_BOOL;
		break;
	case VARIABLE_TYPE_DOUBLE:
		result = GL_DOUBLE;
		break;
	case VARIABLE_TYPE_DMAT2:
		result = GL_DOUBLE_MAT2;
		break;
	case VARIABLE_TYPE_DMAT2X3:
		result = GL_DOUBLE_MAT2x3;
		break;
	case VARIABLE_TYPE_DMAT2X4:
		result = GL_DOUBLE_MAT2x4;
		break;
	case VARIABLE_TYPE_DMAT3:
		result = GL_DOUBLE_MAT3;
		break;
	case VARIABLE_TYPE_DMAT3X2:
		result = GL_DOUBLE_MAT3x2;
		break;
	case VARIABLE_TYPE_DMAT3X4:
		result = GL_DOUBLE_MAT3x4;
		break;
	case VARIABLE_TYPE_DMAT4:
		result = GL_DOUBLE_MAT4;
		break;
	case VARIABLE_TYPE_DMAT4X2:
		result = GL_DOUBLE_MAT4x2;
		break;
	case VARIABLE_TYPE_DMAT4X3:
		result = GL_DOUBLE_MAT4x3;
		break;
	case VARIABLE_TYPE_DVEC2:
		result = GL_DOUBLE_VEC2;
		break;
	case VARIABLE_TYPE_DVEC3:
		result = GL_DOUBLE_VEC3;
		break;
	case VARIABLE_TYPE_DVEC4:
		result = GL_DOUBLE_VEC4;
		break;
	case VARIABLE_TYPE_FLOAT:
		result = GL_FLOAT;
		break;
	case VARIABLE_TYPE_INT:
		result = GL_INT;
		break;
	case VARIABLE_TYPE_IVEC2:
		result = GL_INT_VEC2;
		break;
	case VARIABLE_TYPE_IVEC3:
		result = GL_INT_VEC3;
		break;
	case VARIABLE_TYPE_IVEC4:
		result = GL_INT_VEC4;
		break;
	case VARIABLE_TYPE_MAT2:
		result = GL_FLOAT_MAT2;
		break;
	case VARIABLE_TYPE_MAT2X3:
		result = GL_FLOAT_MAT2x3;
		break;
	case VARIABLE_TYPE_MAT2X4:
		result = GL_FLOAT_MAT2x4;
		break;
	case VARIABLE_TYPE_MAT3:
		result = GL_FLOAT_MAT3;
		break;
	case VARIABLE_TYPE_MAT3X2:
		result = GL_FLOAT_MAT3x2;
		break;
	case VARIABLE_TYPE_MAT3X4:
		result = GL_FLOAT_MAT3x4;
		break;
	case VARIABLE_TYPE_MAT4:
		result = GL_FLOAT_MAT4;
		break;
	case VARIABLE_TYPE_MAT4X2:
		result = GL_FLOAT_MAT4x2;
		break;
	case VARIABLE_TYPE_MAT4X3:
		result = GL_FLOAT_MAT4x3;
		break;
	case VARIABLE_TYPE_UINT:
		result = GL_UNSIGNED_INT;
		break;
	case VARIABLE_TYPE_UVEC2:
		result = GL_UNSIGNED_INT_VEC2;
		break;
	case VARIABLE_TYPE_UVEC3:
		result = GL_UNSIGNED_INT_VEC3;
		break;
	case VARIABLE_TYPE_UVEC4:
		result = GL_UNSIGNED_INT_VEC4;
		break;
	case VARIABLE_TYPE_VEC2:
		result = GL_FLOAT_VEC2;
		break;
	case VARIABLE_TYPE_VEC3:
		result = GL_FLOAT_VEC3;
		break;
	case VARIABLE_TYPE_VEC4:
		result = GL_FLOAT_VEC4;
		break;

	default:
	{
		TCU_FAIL("Unrecognized variable type");
	}
	}

	return result;
}

/** Get _variable_type representing integer type with given dimmensions
 *
 * @param n_columns Number of columns
 * @param n_row     Number of rows
 *
 * @return Corresponding _variable_type
 **/
Utils::_variable_type Utils::getIntVariableType(glw::GLuint n_columns, glw::GLuint n_rows)
{
	Utils::_variable_type type = VARIABLE_TYPE_UNKNOWN;

	static const _variable_type types[4] = { VARIABLE_TYPE_INT, VARIABLE_TYPE_IVEC2, VARIABLE_TYPE_IVEC3,
											 VARIABLE_TYPE_IVEC4 };

	if (1 != n_columns)
	{
		TCU_FAIL("Not implemented");
	}
	else
	{
		type = types[n_rows - 1];
	}

	return type;
}

/** Returns te number of components that variables defined with user-specified type
 *  support. For matrix types, total amount of values accessible for the type will be
 *  returned.
 *
 *  @param type Variable type to return the described vale for.
 *
 *  @return As per description or 0 if @param type was not recognized.
 */
unsigned int Utils::getNumberOfComponentsForVariableType(_variable_type type)
{
	unsigned int result = 0;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
	case VARIABLE_TYPE_DOUBLE:
	case VARIABLE_TYPE_FLOAT:
	case VARIABLE_TYPE_INT:
	case VARIABLE_TYPE_UINT:
	{
		result = 1;

		break;
	}

	case VARIABLE_TYPE_DVEC2:
	case VARIABLE_TYPE_IVEC2:
	case VARIABLE_TYPE_UVEC2:
	case VARIABLE_TYPE_VEC2:
	{
		result = 2;

		break;
	}

	case VARIABLE_TYPE_DVEC3:
	case VARIABLE_TYPE_IVEC3:
	case VARIABLE_TYPE_UVEC3:
	case VARIABLE_TYPE_VEC3:
	{
		result = 3;

		break;
	}

	case VARIABLE_TYPE_DVEC4:
	case VARIABLE_TYPE_IVEC4:
	case VARIABLE_TYPE_UVEC4:
	case VARIABLE_TYPE_VEC4:
	{
		result = 4;

		break;
	}

	case VARIABLE_TYPE_DMAT2:
	case VARIABLE_TYPE_MAT2:
	{
		result = 2 * 2;

		break;
	}

	case VARIABLE_TYPE_DMAT2X3:
	case VARIABLE_TYPE_DMAT3X2:
	case VARIABLE_TYPE_MAT2X3:
	case VARIABLE_TYPE_MAT3X2:
	{
		result = 2 * 3;

		break;
	}

	case VARIABLE_TYPE_DMAT2X4:
	case VARIABLE_TYPE_DMAT4X2:
	case VARIABLE_TYPE_MAT2X4:
	case VARIABLE_TYPE_MAT4X2:
	{
		result = 2 * 4;

		break;
	}

	case VARIABLE_TYPE_DMAT3:
	case VARIABLE_TYPE_MAT3:
	{
		result = 3 * 3;

		break;
	}

	case VARIABLE_TYPE_DMAT3X4:
	case VARIABLE_TYPE_DMAT4X3:
	case VARIABLE_TYPE_MAT3X4:
	case VARIABLE_TYPE_MAT4X3:
	{
		result = 3 * 4;

		break;
	}

	case VARIABLE_TYPE_DMAT4:
	case VARIABLE_TYPE_MAT4:
	{
		result = 4 * 4;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized type");
	}
	} /* switch (type) */

	return result;
}

/** Returns number of columns user-specified matrix variable type describes.
 *
 *  @param type Variable type to use for the query. Only VARIABLE_TYPE_DMAT*
 *              values are valid.
 *
 *  @return As per description.
 **/
unsigned int Utils::getNumberOfColumnsForVariableType(_variable_type type)
{
	unsigned int result = 0;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
	case VARIABLE_TYPE_DOUBLE:
	case VARIABLE_TYPE_FLOAT:
	case VARIABLE_TYPE_INT:
	case VARIABLE_TYPE_UINT:
	case VARIABLE_TYPE_DVEC2:
	case VARIABLE_TYPE_IVEC2:
	case VARIABLE_TYPE_UVEC2:
	case VARIABLE_TYPE_VEC2:
	case VARIABLE_TYPE_DVEC3:
	case VARIABLE_TYPE_IVEC3:
	case VARIABLE_TYPE_UVEC3:
	case VARIABLE_TYPE_VEC3:
	case VARIABLE_TYPE_DVEC4:
	case VARIABLE_TYPE_IVEC4:
	case VARIABLE_TYPE_UVEC4:
	case VARIABLE_TYPE_VEC4:
	{
		result = 1;

		break;
	}

	case VARIABLE_TYPE_DMAT2:
	case VARIABLE_TYPE_DMAT2X3:
	case VARIABLE_TYPE_DMAT2X4:
	case VARIABLE_TYPE_MAT2:
	case VARIABLE_TYPE_MAT2X3:
	case VARIABLE_TYPE_MAT2X4:
	{
		result = 2;

		break;
	}

	case VARIABLE_TYPE_DMAT3:
	case VARIABLE_TYPE_DMAT3X2:
	case VARIABLE_TYPE_DMAT3X4:
	case VARIABLE_TYPE_MAT3:
	case VARIABLE_TYPE_MAT3X2:
	case VARIABLE_TYPE_MAT3X4:
	{
		result = 3;

		break;
	}

	case VARIABLE_TYPE_DMAT4:
	case VARIABLE_TYPE_DMAT4X2:
	case VARIABLE_TYPE_DMAT4X3:
	case VARIABLE_TYPE_MAT4:
	case VARIABLE_TYPE_MAT4X2:
	case VARIABLE_TYPE_MAT4X3:
	{
		result = 4;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized type");
	}
	} /* switch (type) */

	return result;
}

/** Returns maximum number of uniform locations taken by user-specified double-precision
 *  variable type.
 *
 *  @param type Variable type to use for the query. Only VARIABLE_TYPE_D* values are valid.
 *
 *  @return As per description.
 **/
unsigned int Utils::getNumberOfLocationsUsedByDoublePrecisionVariableType(_variable_type type)
{
	unsigned int result = 0;

	switch (type)
	{
	case VARIABLE_TYPE_DOUBLE:
		result = 1;
		break;
	case VARIABLE_TYPE_DVEC2:
		result = 1;
		break;
	case VARIABLE_TYPE_DVEC3:
		result = 2;
		break;
	case VARIABLE_TYPE_DVEC4:
		result = 2;
		break;
	case VARIABLE_TYPE_DMAT2:
		result = 2;
		break;
	case VARIABLE_TYPE_DMAT2X3:
		result = 6;
		break;
	case VARIABLE_TYPE_DMAT2X4:
		result = 8;
		break;
	case VARIABLE_TYPE_DMAT3:
		result = 6;
		break;
	case VARIABLE_TYPE_DMAT3X2:
		result = 4;
		break;
	case VARIABLE_TYPE_DMAT3X4:
		result = 8;
		break;
	case VARIABLE_TYPE_DMAT4:
		result = 8;
		break;
	case VARIABLE_TYPE_DMAT4X2:
		result = 4;
		break;
	case VARIABLE_TYPE_DMAT4X3:
		result = 6;
		break;

	default:
	{
		TCU_FAIL("Unrecognized type");
	}
	} /* switch (type) */

	return result;
}

/** Get number of rows for given variable type
 *
 * @param type Type of variable
 *
 * @return Number of rows
 **/
unsigned int Utils::getNumberOfRowsForVariableType(_variable_type type)
{
	unsigned int result = 0;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
	case VARIABLE_TYPE_DOUBLE:
	case VARIABLE_TYPE_FLOAT:
	case VARIABLE_TYPE_INT:
	case VARIABLE_TYPE_UINT:
	{
		result = 1;

		break;
	}

	case VARIABLE_TYPE_DVEC2:
	case VARIABLE_TYPE_IVEC2:
	case VARIABLE_TYPE_UVEC2:
	case VARIABLE_TYPE_VEC2:
	case VARIABLE_TYPE_DMAT2:
	case VARIABLE_TYPE_DMAT3X2:
	case VARIABLE_TYPE_DMAT4X2:
	case VARIABLE_TYPE_MAT2:
	case VARIABLE_TYPE_MAT3X2:
	case VARIABLE_TYPE_MAT4X2:
	{
		result = 2;

		break;
	}

	case VARIABLE_TYPE_DVEC3:
	case VARIABLE_TYPE_IVEC3:
	case VARIABLE_TYPE_UVEC3:
	case VARIABLE_TYPE_VEC3:
	case VARIABLE_TYPE_DMAT2X3:
	case VARIABLE_TYPE_DMAT3:
	case VARIABLE_TYPE_DMAT4X3:
	case VARIABLE_TYPE_MAT2X3:
	case VARIABLE_TYPE_MAT3:
	case VARIABLE_TYPE_MAT4X3:
	{
		result = 3;

		break;
	}

	case VARIABLE_TYPE_DVEC4:
	case VARIABLE_TYPE_IVEC4:
	case VARIABLE_TYPE_UVEC4:
	case VARIABLE_TYPE_VEC4:
	case VARIABLE_TYPE_DMAT2X4:
	case VARIABLE_TYPE_DMAT3X4:
	case VARIABLE_TYPE_DMAT4:
	case VARIABLE_TYPE_MAT2X4:
	case VARIABLE_TYPE_MAT3X4:
	case VARIABLE_TYPE_MAT4:
	{
		result = 4;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized type");
	}
	} /* switch (type) */

	return result;
}

/** Returns variable type of a matrix constructed by multiplying two matrices.
 *
 *  @param type_matrix_a L-side matrix type.
 *  @param type_matrix_b R-side matrix type.
 *
 *  @return As per description.
 **/
Utils::_variable_type Utils::getPostMatrixMultiplicationVariableType(_variable_type type_matrix_a,
																	 _variable_type type_matrix_b)
{
	const unsigned int	n_a_columns	  = Utils::getNumberOfColumnsForVariableType(type_matrix_a);
	const unsigned int	n_a_components   = Utils::getNumberOfComponentsForVariableType(type_matrix_a);
	const unsigned int	n_a_rows		   = n_a_components / n_a_columns;
	const unsigned int	n_b_columns	  = Utils::getNumberOfColumnsForVariableType(type_matrix_b);
	const unsigned int	n_result_columns = n_b_columns;
	const unsigned int	n_result_rows	= n_a_rows;
	Utils::_variable_type result;

	switch (n_result_columns)
	{
	case 2:
	{
		switch (n_result_rows)
		{
		case 2:
			result = VARIABLE_TYPE_DMAT2;
			break;
		case 3:
			result = VARIABLE_TYPE_DMAT2X3;
			break;
		case 4:
			result = VARIABLE_TYPE_DMAT2X4;
			break;

		default:
		{
			TCU_FAIL("Unrecognized amount of rows in result variable");
		}
		} /* switch (n_result_rows) */

		break;
	} /* case 2: */

	case 3:
	{
		switch (n_result_rows)
		{
		case 2:
			result = VARIABLE_TYPE_DMAT3X2;
			break;
		case 3:
			result = VARIABLE_TYPE_DMAT3;
			break;
		case 4:
			result = VARIABLE_TYPE_DMAT3X4;
			break;

		default:
		{
			TCU_FAIL("Unrecognized amount of rows in result variable");
		}
		} /* switch (n_result_rows) */

		break;
	} /* case 3: */

	case 4:
	{
		switch (n_result_rows)
		{
		case 2:
			result = VARIABLE_TYPE_DMAT4X2;
			break;
		case 3:
			result = VARIABLE_TYPE_DMAT4X3;
			break;
		case 4:
			result = VARIABLE_TYPE_DMAT4;
			break;

		default:
		{
			TCU_FAIL("Unrecognized amount of rows in result variable");
		}
		} /* switch (n_result_rows) */

		break;
	} /* case 4: */

	default:
	{
		TCU_FAIL("Unrecognized amount of columns in result variable");
	}
	} /* switch (n_result_columns) */

	/* Done */
	return result;
}

/** Returns a string holding the value represented by user-provided variable, for which
 *  the data are represented in @param type variable type.
 *
 *  @return As per description.
 **/
std::string Utils::getStringForVariableTypeValue(_variable_type type, const unsigned char* data_ptr)
{
	std::stringstream result_sstream;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
		result_sstream << *((bool*)data_ptr);
		break;
	case VARIABLE_TYPE_DOUBLE:
		result_sstream << *((double*)data_ptr);
		break;
	case VARIABLE_TYPE_FLOAT:
		result_sstream << *((float*)data_ptr);
		break;
	case VARIABLE_TYPE_INT:
		result_sstream << *((int*)data_ptr);
		break;
	case VARIABLE_TYPE_UINT:
		result_sstream << *((unsigned int*)data_ptr);
		break;

	default:
	{
		TCU_FAIL("Unrecognized variable type requested");
	}
	} /* switch (type) */

	return result_sstream.str();
}

/** Returns variable type of a transposed matrix of user-specified variable type.
 *
 *  @param type Variable type of the matrix to be transposed.
 *
 *  @return Transposed matrix variable type.
 **/
Utils::_variable_type Utils::getTransposedMatrixVariableType(Utils::_variable_type type)
{
	Utils::_variable_type result;

	switch (type)
	{
	case VARIABLE_TYPE_DMAT2:
		result = VARIABLE_TYPE_DMAT2;
		break;
	case VARIABLE_TYPE_DMAT2X3:
		result = VARIABLE_TYPE_DMAT3X2;
		break;
	case VARIABLE_TYPE_DMAT2X4:
		result = VARIABLE_TYPE_DMAT4X2;
		break;
	case VARIABLE_TYPE_DMAT3:
		result = VARIABLE_TYPE_DMAT3;
		break;
	case VARIABLE_TYPE_DMAT3X2:
		result = VARIABLE_TYPE_DMAT2X3;
		break;
	case VARIABLE_TYPE_DMAT3X4:
		result = VARIABLE_TYPE_DMAT4X3;
		break;
	case VARIABLE_TYPE_DMAT4:
		result = VARIABLE_TYPE_DMAT4;
		break;
	case VARIABLE_TYPE_DMAT4X2:
		result = VARIABLE_TYPE_DMAT2X4;
		break;
	case VARIABLE_TYPE_DMAT4X3:
		result = VARIABLE_TYPE_DMAT3X4;
		break;

	default:
	{
		TCU_FAIL("Unrecognized double-precision matrix variable type.");
	}
	} /* switch (type) */

	return result;
}

/** Get _variable_type representing unsigned integer type with given dimmensions
 *
 * @param n_columns Number of columns
 * @param n_row     Number of rows
 *
 * @return Corresponding _variable_type
 **/
Utils::_variable_type Utils::getUintVariableType(glw::GLuint n_columns, glw::GLuint n_rows)
{
	Utils::_variable_type type = VARIABLE_TYPE_UNKNOWN;

	static const _variable_type types[4] = { VARIABLE_TYPE_UINT, VARIABLE_TYPE_UVEC2, VARIABLE_TYPE_UVEC3,
											 VARIABLE_TYPE_UVEC4 };

	if (1 != n_columns)
	{
		TCU_FAIL("Not implemented");
	}
	else
	{
		type = types[n_rows - 1];
	}

	return type;
}

/** Returns a literal corresponding to a GLSL keyword describing user-specified
 *  variable type.
 *
 *  @param type Variable type to use for the query.
 *
 *  @return Requested GLSL keyword or [?] if @param type was not recognized.
 **/
std::string Utils::getVariableTypeString(_variable_type type)
{
	std::string result = "[?]";

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
		result = "bool";
		break;
	case VARIABLE_TYPE_BVEC2:
		result = "bvec2";
		break;
	case VARIABLE_TYPE_BVEC3:
		result = "bvec3";
		break;
	case VARIABLE_TYPE_BVEC4:
		result = "bvec4";
		break;
	case VARIABLE_TYPE_DOUBLE:
		result = "double";
		break;
	case VARIABLE_TYPE_DMAT2:
		result = "dmat2";
		break;
	case VARIABLE_TYPE_DMAT2X3:
		result = "dmat2x3";
		break;
	case VARIABLE_TYPE_DMAT2X4:
		result = "dmat2x4";
		break;
	case VARIABLE_TYPE_DMAT3:
		result = "dmat3";
		break;
	case VARIABLE_TYPE_DMAT3X2:
		result = "dmat3x2";
		break;
	case VARIABLE_TYPE_DMAT3X4:
		result = "dmat3x4";
		break;
	case VARIABLE_TYPE_DMAT4:
		result = "dmat4";
		break;
	case VARIABLE_TYPE_DMAT4X2:
		result = "dmat4x2";
		break;
	case VARIABLE_TYPE_DMAT4X3:
		result = "dmat4x3";
		break;
	case VARIABLE_TYPE_DVEC2:
		result = "dvec2";
		break;
	case VARIABLE_TYPE_DVEC3:
		result = "dvec3";
		break;
	case VARIABLE_TYPE_DVEC4:
		result = "dvec4";
		break;
	case VARIABLE_TYPE_FLOAT:
		result = "float";
		break;
	case VARIABLE_TYPE_INT:
		result = "int";
		break;
	case VARIABLE_TYPE_IVEC2:
		result = "ivec2";
		break;
	case VARIABLE_TYPE_IVEC3:
		result = "ivec3";
		break;
	case VARIABLE_TYPE_IVEC4:
		result = "ivec4";
		break;
	case VARIABLE_TYPE_MAT2:
		result = "mat2";
		break;
	case VARIABLE_TYPE_MAT2X3:
		result = "mat2x3";
		break;
	case VARIABLE_TYPE_MAT2X4:
		result = "mat2x4";
		break;
	case VARIABLE_TYPE_MAT3:
		result = "mat3";
		break;
	case VARIABLE_TYPE_MAT3X2:
		result = "mat3x2";
		break;
	case VARIABLE_TYPE_MAT3X4:
		result = "mat3x4";
		break;
	case VARIABLE_TYPE_MAT4:
		result = "mat4";
		break;
	case VARIABLE_TYPE_MAT4X2:
		result = "mat4x2";
		break;
	case VARIABLE_TYPE_MAT4X3:
		result = "mat4x3";
		break;
	case VARIABLE_TYPE_UINT:
		result = "uint";
		break;
	case VARIABLE_TYPE_UVEC2:
		result = "uvec2";
		break;
	case VARIABLE_TYPE_UVEC3:
		result = "uvec3";
		break;
	case VARIABLE_TYPE_UVEC4:
		result = "uvec4";
		break;
	case VARIABLE_TYPE_VEC2:
		result = "vec2";
		break;
	case VARIABLE_TYPE_VEC3:
		result = "vec3";
		break;
	case VARIABLE_TYPE_VEC4:
		result = "vec4";
		break;

	default:
	{
		TCU_FAIL("Unrecognized variable type");
	}
	}; /* switch (type) */

	return result;
}

/** Check if GL context meets version requirements
 *
 * @param gl             Functions
 * @param required_major Minimum required MAJOR_VERSION
 * @param required_minor Minimum required MINOR_VERSION
 *
 * @return true if GL context version is at least as requested, false otherwise
 **/
bool Utils::isGLVersionAtLeast(const glw::Functions& gl, glw::GLint required_major, glw::GLint required_minor)
{
	glw::GLint major = 0;
	glw::GLint minor = 0;

	gl.getIntegerv(GL_MAJOR_VERSION, &major);
	gl.getIntegerv(GL_MINOR_VERSION, &minor);

	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	if (major > required_major)
	{
		/* Major is higher than required one */
		return true;
	}
	else if (major == required_major)
	{
		if (minor >= required_minor)
		{
			/* Major is equal to required one */
			/* Minor is higher than or equal to required one */
			return true;
		}
		else
		{
			/* Major is equal to required one */
			/* Minor is lower than required one */
			return false;
		}
	}
	else
	{
		/* Major is lower than required one */
		return false;
	}
}

/** Tells whether user-specified variable type corresponds to a matrix type.
 *
 *  @param type Variable type to use for the query.
 *
 *  @return true if the variable type describes a matrix, false otherwise.
 **/
bool Utils::isMatrixVariableType(_variable_type type)
{
	return (type == VARIABLE_TYPE_MAT2 || type == VARIABLE_TYPE_MAT3 || type == VARIABLE_TYPE_MAT4 ||
			type == VARIABLE_TYPE_MAT2X3 || type == VARIABLE_TYPE_MAT2X4 || type == VARIABLE_TYPE_MAT3X2 ||
			type == VARIABLE_TYPE_MAT3X4 || type == VARIABLE_TYPE_MAT4X2 || type == VARIABLE_TYPE_MAT4X3 ||
			type == VARIABLE_TYPE_DMAT2 || type == VARIABLE_TYPE_DMAT3 || type == VARIABLE_TYPE_DMAT4 ||
			type == VARIABLE_TYPE_DMAT2X3 || type == VARIABLE_TYPE_DMAT2X4 || type == VARIABLE_TYPE_DMAT3X2 ||
			type == VARIABLE_TYPE_DMAT3X4 || type == VARIABLE_TYPE_DMAT4X2 || type == VARIABLE_TYPE_DMAT4X3);
}

/** Tells whether user-specified variable type is scalar.
 *
 *  @return true if @param type is a scalar variable type, false otherwise.
 **/
bool Utils::isScalarVariableType(_variable_type type)
{
	bool result = false;

	switch (type)
	{
	case VARIABLE_TYPE_BOOL:
		result = true;
		break;
	case VARIABLE_TYPE_DOUBLE:
		result = true;
		break;
	case VARIABLE_TYPE_FLOAT:
		result = true;
		break;
	case VARIABLE_TYPE_INT:
		result = true;
		break;
	case VARIABLE_TYPE_UINT:
		result = true;
		break;
	default:
		break;
	}; /* switch (type) */

	return result;
}

/** Replace first occurance of <token> with <text> in <string> starting at <search_posistion>
 *
 * @param token           Token string
 * @param search_position Position at which find will start, it is updated to position at which replaced text ends
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void Utils::replaceToken(const glw::GLchar* token, size_t& search_position, const glw::GLchar* text,
						 std::string& string)
{
	const size_t text_length	= strlen(text);
	const size_t token_length   = strlen(token);
	const size_t token_position = string.find(token, search_position);

	string.replace(token_position, token_length, text, text_length);

	search_position = token_position + text_length;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
GPUShaderFP64Test1::GPUShaderFP64Test1(deqp::Context& context)
	: TestCase(context, "errors", "Verifies that various erroneous conditions related to double-precision "
								  "float uniform support & glUniform*() / glUniformMatrix*() API "
								  " are reported correctly.")
	, m_has_test_passed(true)
	, m_po_bool_arr_uniform_location(0)
	, m_po_bool_uniform_location(0)
	, m_po_bvec2_arr_uniform_location(0)
	, m_po_bvec2_uniform_location(0)
	, m_po_bvec3_arr_uniform_location(0)
	, m_po_bvec3_uniform_location(0)
	, m_po_bvec4_arr_uniform_location(0)
	, m_po_bvec4_uniform_location(0)
	, m_po_dmat2_arr_uniform_location(0)
	, m_po_dmat2_uniform_location(0)
	, m_po_dmat2x3_arr_uniform_location(0)
	, m_po_dmat2x3_uniform_location(0)
	, m_po_dmat2x4_arr_uniform_location(0)
	, m_po_dmat2x4_uniform_location(0)
	, m_po_dmat3_arr_uniform_location(0)
	, m_po_dmat3_uniform_location(0)
	, m_po_dmat3x2_arr_uniform_location(0)
	, m_po_dmat3x2_uniform_location(0)
	, m_po_dmat3x4_arr_uniform_location(0)
	, m_po_dmat3x4_uniform_location(0)
	, m_po_dmat4_arr_uniform_location(0)
	, m_po_dmat4_uniform_location(0)
	, m_po_dmat4x2_arr_uniform_location(0)
	, m_po_dmat4x2_uniform_location(0)
	, m_po_dmat4x3_arr_uniform_location(0)
	, m_po_dmat4x3_uniform_location(0)
	, m_po_double_arr_uniform_location(0)
	, m_po_double_uniform_location(0)
	, m_po_dvec2_arr_uniform_location(0)
	, m_po_dvec2_uniform_location(0)
	, m_po_dvec3_arr_uniform_location(0)
	, m_po_dvec3_uniform_location(0)
	, m_po_dvec4_arr_uniform_location(0)
	, m_po_dvec4_uniform_location(0)
	, m_po_float_arr_uniform_location(0)
	, m_po_float_uniform_location(0)
	, m_po_int_arr_uniform_location(0)
	, m_po_int_uniform_location(0)
	, m_po_ivec2_arr_uniform_location(0)
	, m_po_ivec2_uniform_location(0)
	, m_po_ivec3_arr_uniform_location(0)
	, m_po_ivec3_uniform_location(0)
	, m_po_ivec4_arr_uniform_location(0)
	, m_po_ivec4_uniform_location(0)
	, m_po_sampler_uniform_location(0)
	, m_po_uint_arr_uniform_location(0)
	, m_po_uint_uniform_location(0)
	, m_po_uvec2_arr_uniform_location(0)
	, m_po_uvec2_uniform_location(0)
	, m_po_uvec3_arr_uniform_location(0)
	, m_po_uvec3_uniform_location(0)
	, m_po_uvec4_arr_uniform_location(0)
	, m_po_uvec4_uniform_location(0)
	, m_po_vec2_arr_uniform_location(0)
	, m_po_vec2_uniform_location(0)
	, m_po_vec3_arr_uniform_location(0)
	, m_po_vec3_uniform_location(0)
	, m_po_vec4_arr_uniform_location(0)
	, m_po_vec4_uniform_location(0)
	, m_po_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during
 *  test execution.
 **/
void GPUShaderFP64Test1::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Returns a string describing GL API function represented by user-provided enum.
 *
 *  @param func Uniform function to return the string for.
 *
 *  @return As per description. [?] will be returned if the function was not recognized.
 **/
const char* GPUShaderFP64Test1::getUniformFunctionString(_uniform_function func)
{
	const char* result = "[?]";

	switch (func)
	{
	case UNIFORM_FUNCTION_1D:
		result = "glUniform1d";
		break;
	case UNIFORM_FUNCTION_1DV:
		result = "glUniform1dv";
		break;
	case UNIFORM_FUNCTION_2D:
		result = "glUniform2d";
		break;
	case UNIFORM_FUNCTION_2DV:
		result = "glUniform2dv";
		break;
	case UNIFORM_FUNCTION_3D:
		result = "glUniform3d";
		break;
	case UNIFORM_FUNCTION_3DV:
		result = "glUniform3dv";
		break;
	case UNIFORM_FUNCTION_4D:
		result = "glUniform4d";
		break;
	case UNIFORM_FUNCTION_4DV:
		result = "glUniform4dv";
		break;
	case UNIFORM_FUNCTION_MATRIX2DV:
		result = "glUniformMatrix2dv";
		break;
	case UNIFORM_FUNCTION_MATRIX2X3DV:
		result = "glUniformMatrix2x3dv";
		break;
	case UNIFORM_FUNCTION_MATRIX2X4DV:
		result = "glUniformMatrix2x4dv";
		break;
	case UNIFORM_FUNCTION_MATRIX3DV:
		result = "glUniformMatrix3dv";
		break;
	case UNIFORM_FUNCTION_MATRIX3X2DV:
		result = "glUniformMatrix3x2dv";
		break;
	case UNIFORM_FUNCTION_MATRIX3X4DV:
		result = "glUniformMatrix3x4dv";
		break;
	case UNIFORM_FUNCTION_MATRIX4DV:
		result = "glUniformMatrix4dv";
		break;
	case UNIFORM_FUNCTION_MATRIX4X2DV:
		result = "glUniformMatrix4x2dv";
		break;
	case UNIFORM_FUNCTION_MATRIX4X3DV:
		result = "glUniformMatrix4x3dv";
		break;
	default:
		break;
	}

	return result;
}

/** Returns name of an uniform bound to user-provided location.
 *
 *  @param location Location of the uniform to return the name for.
 *
 *  @return As per description. [?] will be returned if the location was not
 *          recognized.
 **/
const char* GPUShaderFP64Test1::getUniformNameForLocation(glw::GLint location)
{
	const char* result = "[?]";

	if (location == m_po_bool_arr_uniform_location)
		result = "uniform_bool_arr";
	else if (location == m_po_bool_uniform_location)
		result = "uniform_bool";
	else if (location == m_po_bvec2_arr_uniform_location)
		result = "uniform_bvec2_arr";
	else if (location == m_po_bvec2_uniform_location)
		result = "uniform_bvec2";
	else if (location == m_po_bvec3_arr_uniform_location)
		result = "uniform_bvec3_arr";
	else if (location == m_po_bvec3_uniform_location)
		result = "uniform_bvec3";
	else if (location == m_po_bvec4_arr_uniform_location)
		result = "uniform_bvec4_arr";
	else if (location == m_po_bvec4_uniform_location)
		result = "uniform_bvec4";
	else if (location == m_po_dmat2_arr_uniform_location)
		result = "uniform_dmat2_arr";
	else if (location == m_po_dmat2_uniform_location)
		result = "uniform_dmat2";
	else if (location == m_po_dmat2x3_arr_uniform_location)
		result = "uniform_dmat2x3_arr";
	else if (location == m_po_dmat2x3_uniform_location)
		result = "uniform_dmat2x3";
	else if (location == m_po_dmat2x4_arr_uniform_location)
		result = "uniform_dmat2x4_arr";
	else if (location == m_po_dmat2x4_uniform_location)
		result = "uniform_dmat2x4";
	else if (location == m_po_dmat3_arr_uniform_location)
		result = "uniform_dmat3_arr";
	else if (location == m_po_dmat3_uniform_location)
		result = "uniform_dmat3";
	else if (location == m_po_dmat3x2_arr_uniform_location)
		result = "uniform_dmat3x2_arr";
	else if (location == m_po_dmat3x2_uniform_location)
		result = "uniform_dmat3x2";
	else if (location == m_po_dmat3x4_arr_uniform_location)
		result = "uniform_dmat3x4_arr";
	else if (location == m_po_dmat3x4_uniform_location)
		result = "uniform_dmat3x4";
	else if (location == m_po_dmat4_arr_uniform_location)
		result = "uniform_dmat4_arr";
	else if (location == m_po_dmat4_uniform_location)
		result = "uniform_dmat4";
	else if (location == m_po_dmat4x2_arr_uniform_location)
		result = "uniform_dmat4x2_arr";
	else if (location == m_po_dmat4x2_uniform_location)
		result = "uniform_dmat4x2";
	else if (location == m_po_dmat4x3_arr_uniform_location)
		result = "uniform_dmat4x3_arr";
	else if (location == m_po_dmat4x3_uniform_location)
		result = "uniform_dmat4x3";
	else if (location == m_po_double_arr_uniform_location)
		result = "uniform_double_arr";
	else if (location == m_po_double_uniform_location)
		result = "uniform_double";
	else if (location == m_po_dvec2_arr_uniform_location)
		result = "uniform_dvec2_arr";
	else if (location == m_po_dvec2_uniform_location)
		result = "uniform_dvec2";
	else if (location == m_po_dvec3_arr_uniform_location)
		result = "uniform_dvec3_arr";
	else if (location == m_po_dvec3_uniform_location)
		result = "uniform_dvec3";
	else if (location == m_po_dvec4_arr_uniform_location)
		result = "uniform_dvec4_arr";
	else if (location == m_po_dvec4_uniform_location)
		result = "uniform_dvec4";
	else if (location == m_po_float_arr_uniform_location)
		result = "uniform_float_arr";
	else if (location == m_po_float_uniform_location)
		result = "uniform_float";
	else if (location == m_po_int_arr_uniform_location)
		result = "uniform_int_arr";
	else if (location == m_po_int_uniform_location)
		result = "uniform_int";
	else if (location == m_po_ivec2_arr_uniform_location)
		result = "uniform_ivec2_arr";
	else if (location == m_po_ivec2_uniform_location)
		result = "uniform_ivec2";
	else if (location == m_po_ivec3_arr_uniform_location)
		result = "uniform_ivec3_arr";
	else if (location == m_po_ivec3_uniform_location)
		result = "uniform_ivec3";
	else if (location == m_po_ivec4_arr_uniform_location)
		result = "uniform_ivec4_arr";
	else if (location == m_po_ivec4_uniform_location)
		result = "uniform_ivec4";
	else if (location == m_po_uint_arr_uniform_location)
		result = "uniform_uint_arr";
	else if (location == m_po_uint_uniform_location)
		result = "uniform_uint";
	else if (location == m_po_uvec2_arr_uniform_location)
		result = "uniform_uvec2_arr";
	else if (location == m_po_uvec2_uniform_location)
		result = "uniform_uvec2";
	else if (location == m_po_uvec3_arr_uniform_location)
		result = "uniform_uvec3_arr";
	else if (location == m_po_uvec3_uniform_location)
		result = "uniform_uvec3";
	else if (location == m_po_uvec4_arr_uniform_location)
		result = "uniform_uvec4_arr";
	else if (location == m_po_uvec4_uniform_location)
		result = "uniform_uvec4";
	else if (location == m_po_vec2_arr_uniform_location)
		result = "uniform_vec2_arr";
	else if (location == m_po_vec2_uniform_location)
		result = "uniform_vec2";
	else if (location == m_po_vec3_arr_uniform_location)
		result = "uniform_vec3_arr";
	else if (location == m_po_vec3_uniform_location)
		result = "uniform_vec3";
	else if (location == m_po_vec4_arr_uniform_location)
		result = "uniform_vec4_arr";
	else if (location == m_po_vec4_uniform_location)
		result = "uniform_vec4";

	return result;
}

/** Initializes all GL objects required to run the test. Also extracts locations of all
 *  uniforms used by the test.
 *
 *  This function can throw a TestError exception if the implementation misbehaves.
 */
void GPUShaderFP64Test1::initTest()
{
	glw::GLint			  compile_status = GL_FALSE;
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();
	glw::GLint			  link_status	= GL_FALSE;

	/* Set up a program object using all new double-precision types */
	const char* vs_body =
		"#version 400\n"
		"\n"
		"uniform bool      uniform_bool;\n"
		"uniform bvec2     uniform_bvec2;\n"
		"uniform bvec3     uniform_bvec3;\n"
		"uniform bvec4     uniform_bvec4;\n"
		"uniform dmat2     uniform_dmat2;\n"
		"uniform dmat2x3   uniform_dmat2x3;\n"
		"uniform dmat2x4   uniform_dmat2x4;\n"
		"uniform dmat3     uniform_dmat3;\n"
		"uniform dmat3x2   uniform_dmat3x2;\n"
		"uniform dmat3x4   uniform_dmat3x4;\n"
		"uniform dmat4     uniform_dmat4;\n"
		"uniform dmat4x2   uniform_dmat4x2;\n"
		"uniform dmat4x3   uniform_dmat4x3;\n"
		"uniform double    uniform_double;\n"
		"uniform dvec2     uniform_dvec2;\n"
		"uniform dvec3     uniform_dvec3;\n"
		"uniform dvec4     uniform_dvec4;\n"
		"uniform float     uniform_float;\n"
		"uniform int       uniform_int;\n"
		"uniform ivec2     uniform_ivec2;\n"
		"uniform ivec3     uniform_ivec3;\n"
		"uniform ivec4     uniform_ivec4;\n"
		"uniform sampler2D uniform_sampler;\n"
		"uniform uint      uniform_uint;\n"
		"uniform uvec2     uniform_uvec2;\n"
		"uniform uvec3     uniform_uvec3;\n"
		"uniform uvec4     uniform_uvec4;\n"
		"uniform vec2      uniform_vec2;\n"
		"uniform vec3      uniform_vec3;\n"
		"uniform vec4      uniform_vec4;\n"
		"uniform bool      uniform_bool_arr   [2];\n"
		"uniform bvec2     uniform_bvec2_arr  [2];\n"
		"uniform bvec3     uniform_bvec3_arr  [2];\n"
		"uniform bvec4     uniform_bvec4_arr  [2];\n"
		"uniform dmat2     uniform_dmat2_arr  [2];\n"
		"uniform dmat2x3   uniform_dmat2x3_arr[2];\n"
		"uniform dmat2x4   uniform_dmat2x4_arr[2];\n"
		"uniform dmat3     uniform_dmat3_arr  [2];\n"
		"uniform dmat3x2   uniform_dmat3x2_arr[2];\n"
		"uniform dmat3x4   uniform_dmat3x4_arr[2];\n"
		"uniform dmat4     uniform_dmat4_arr  [2];\n"
		"uniform dmat4x2   uniform_dmat4x2_arr[2];\n"
		"uniform dmat4x3   uniform_dmat4x3_arr[2];\n"
		"uniform double    uniform_double_arr [2];\n"
		"uniform dvec2     uniform_dvec2_arr  [2];\n"
		"uniform dvec3     uniform_dvec3_arr  [2];\n"
		"uniform dvec4     uniform_dvec4_arr  [2];\n"
		"uniform float     uniform_float_arr  [2];\n"
		"uniform int       uniform_int_arr    [2];\n"
		"uniform ivec2     uniform_ivec2_arr  [2];\n"
		"uniform ivec3     uniform_ivec3_arr  [2];\n"
		"uniform ivec4     uniform_ivec4_arr  [2];\n"
		"uniform uint      uniform_uint_arr   [2];\n"
		"uniform uvec2     uniform_uvec2_arr  [2];\n"
		"uniform uvec3     uniform_uvec3_arr  [2];\n"
		"uniform uvec4     uniform_uvec4_arr  [2];\n"
		"uniform vec2      uniform_vec2_arr   [2];\n"
		"uniform vec3      uniform_vec3_arr   [2];\n"
		"uniform vec4      uniform_vec4_arr   [2];\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = vec4(0) + texture(uniform_sampler, vec2(0) );\n"
		"\n"
		"    if (uniform_bool        && uniform_bvec2.y        && uniform_bvec3.z        && uniform_bvec4.w        &&\n"
		"        uniform_bool_arr[1] && uniform_bvec2_arr[1].y && uniform_bvec3_arr[1].z && uniform_bvec4_arr[1].w)\n"
		"    {\n"
		"        double sum = uniform_dmat2       [0].x + uniform_dmat2x3       [0].x + uniform_dmat2x4       [0].x +\n"
		"                     uniform_dmat3       [0].x + uniform_dmat3x2       [0].x + uniform_dmat3x4       [0].x +\n"
		"                     uniform_dmat4       [0].x + uniform_dmat4x2       [0].x + uniform_dmat4x3       [0].x +\n"
		"                     uniform_dmat2_arr[0][0].x + uniform_dmat2x3_arr[0][0].x + uniform_dmat2x4_arr[0][0].x +\n"
		"                     uniform_dmat3_arr[0][0].x + uniform_dmat3x2_arr[0][0].x + uniform_dmat3x4_arr[0][0].x +\n"
		"                     uniform_dmat4_arr[0][0].x + uniform_dmat4x2_arr[0][0].x + uniform_dmat4x3_arr[0][0].x +\n"
		"                     uniform_double            + uniform_double_arr [0]      +\n"
		"                     uniform_dvec2.x           + uniform_dvec3.x             + uniform_dvec4.x        +\n"
		"                     uniform_dvec2_arr[0].x    + uniform_dvec3_arr[0].x      + uniform_dvec4_arr[0].x;\n"
		"        int   sum2 = uniform_int               + uniform_ivec2.x             + uniform_ivec3.x        +\n"
		"                     uniform_ivec4.x           + uniform_ivec2_arr[0].x      + uniform_ivec3_arr[0].x +\n"
		"                     uniform_ivec4_arr[0].x    + uniform_int_arr[0];\n"
		"        uint  sum3 = uniform_uint              + uniform_uvec2.x             + uniform_uvec3.x        +\n"
		"                     uniform_uvec4.x           + uniform_uint_arr[0]         + uniform_uvec2_arr[0].x +\n"
		"                     uniform_uvec3_arr[0].x    + uniform_uvec4_arr[0].x;\n"
		"        float sum4 = uniform_float             + uniform_float_arr[0]  + \n"
		"                     uniform_vec2.x            + uniform_vec2_arr[0].x + \n"
		"                     uniform_vec3.x            + uniform_vec3_arr[0].x + \n"
		"                     uniform_vec4.x            + uniform_vec4_arr[0].x;\n"
		"\n"
		"        if (sum * sum4 > intBitsToFloat(sum2) * uintBitsToFloat(sum3) )\n"
		"        {\n"
		"            gl_Position = vec4(1);\n"
		"        }\n"
		"    }\n"
		"}\n";

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status != GL_TRUE)
	{
		TCU_FAIL("Shader compilation failed.");
	}

	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed.");
	}

	m_po_bool_arr_uniform_location	= gl.getUniformLocation(m_po_id, "uniform_bool_arr[0]");
	m_po_bool_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_bool");
	m_po_bvec2_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_bvec2_arr[0]");
	m_po_bvec2_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_bvec2");
	m_po_bvec3_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_bvec3_arr[0]");
	m_po_bvec3_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_bvec3");
	m_po_bvec4_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_bvec4_arr[0]");
	m_po_bvec4_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_bvec4");
	m_po_dmat2_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_dmat2_arr[0]");
	m_po_dmat2_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_dmat2");
	m_po_dmat2x3_arr_uniform_location = gl.getUniformLocation(m_po_id, "uniform_dmat2x3_arr[0]");
	m_po_dmat2x3_uniform_location	 = gl.getUniformLocation(m_po_id, "uniform_dmat2x3");
	m_po_dmat2x4_arr_uniform_location = gl.getUniformLocation(m_po_id, "uniform_dmat2x4_arr[0]");
	m_po_dmat2x4_uniform_location	 = gl.getUniformLocation(m_po_id, "uniform_dmat2x4");
	m_po_dmat3_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_dmat3_arr[0]");
	m_po_dmat3_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_dmat3");
	m_po_dmat3x2_arr_uniform_location = gl.getUniformLocation(m_po_id, "uniform_dmat3x2_arr[0]");
	m_po_dmat3x2_uniform_location	 = gl.getUniformLocation(m_po_id, "uniform_dmat3x2");
	m_po_dmat3x4_arr_uniform_location = gl.getUniformLocation(m_po_id, "uniform_dmat3x4_arr[0]");
	m_po_dmat3x4_uniform_location	 = gl.getUniformLocation(m_po_id, "uniform_dmat3x4");
	m_po_dmat4_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_dmat4_arr[0]");
	m_po_dmat4_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_dmat4");
	m_po_dmat4x2_arr_uniform_location = gl.getUniformLocation(m_po_id, "uniform_dmat4x2_arr[0]");
	m_po_dmat4x2_uniform_location	 = gl.getUniformLocation(m_po_id, "uniform_dmat4x2");
	m_po_dmat4x3_arr_uniform_location = gl.getUniformLocation(m_po_id, "uniform_dmat4x3_arr[0]");
	m_po_dmat4x3_uniform_location	 = gl.getUniformLocation(m_po_id, "uniform_dmat4x3");
	m_po_double_arr_uniform_location  = gl.getUniformLocation(m_po_id, "uniform_double_arr[0]");
	m_po_double_uniform_location	  = gl.getUniformLocation(m_po_id, "uniform_double");
	m_po_dvec2_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_dvec2_arr[0]");
	m_po_dvec2_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_dvec2");
	m_po_dvec3_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_dvec3_arr[0]");
	m_po_dvec3_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_dvec3");
	m_po_dvec4_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_dvec4_arr[0]");
	m_po_dvec4_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_dvec4");
	m_po_float_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_float_arr[0]");
	m_po_float_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_float");
	m_po_int_arr_uniform_location	 = gl.getUniformLocation(m_po_id, "uniform_int_arr[0]");
	m_po_int_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_int");
	m_po_ivec2_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_ivec2_arr[0]");
	m_po_ivec2_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_ivec2");
	m_po_ivec3_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_ivec3_arr[0]");
	m_po_ivec3_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_ivec3");
	m_po_ivec4_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_ivec4_arr[0]");
	m_po_ivec4_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_ivec4");
	m_po_sampler_uniform_location	 = gl.getUniformLocation(m_po_id, "uniform_sampler");
	m_po_uint_arr_uniform_location	= gl.getUniformLocation(m_po_id, "uniform_uint_arr[0]");
	m_po_uint_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_uint");
	m_po_uvec2_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_uvec2_arr[0]");
	m_po_uvec2_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_uvec2");
	m_po_uvec3_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_uvec3_arr[0]");
	m_po_uvec3_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_uvec3");
	m_po_uvec4_arr_uniform_location   = gl.getUniformLocation(m_po_id, "uniform_uvec4_arr[0]");
	m_po_uvec4_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_uvec4");
	m_po_vec2_arr_uniform_location	= gl.getUniformLocation(m_po_id, "uniform_vec2_arr[0]");
	m_po_vec2_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_vec2");
	m_po_vec3_arr_uniform_location	= gl.getUniformLocation(m_po_id, "uniform_vec3_arr[0]");
	m_po_vec3_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_vec3");
	m_po_vec4_arr_uniform_location	= gl.getUniformLocation(m_po_id, "uniform_vec4_arr[0]");
	m_po_vec4_uniform_location		  = gl.getUniformLocation(m_po_id, "uniform_vec4");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call(s) failed.");

	if (m_po_bool_arr_uniform_location == -1 || m_po_bool_uniform_location == -1 ||
		m_po_bvec2_arr_uniform_location == -1 || m_po_bvec2_uniform_location == -1 ||
		m_po_bvec3_arr_uniform_location == -1 || m_po_bvec3_uniform_location == -1 ||
		m_po_bvec4_arr_uniform_location == -1 || m_po_bvec4_uniform_location == -1 ||
		m_po_dmat2_arr_uniform_location == -1 || m_po_dmat2_uniform_location == -1 ||
		m_po_dmat2x3_arr_uniform_location == -1 || m_po_dmat2x3_uniform_location == -1 ||
		m_po_dmat2x4_arr_uniform_location == -1 || m_po_dmat2x4_uniform_location == -1 ||
		m_po_dmat3_arr_uniform_location == -1 || m_po_dmat3_uniform_location == -1 ||
		m_po_dmat3x2_arr_uniform_location == -1 || m_po_dmat3x2_uniform_location == -1 ||
		m_po_dmat3x4_arr_uniform_location == -1 || m_po_dmat3x4_uniform_location == -1 ||
		m_po_dmat4_arr_uniform_location == -1 || m_po_dmat4_uniform_location == -1 ||
		m_po_dmat4x2_arr_uniform_location == -1 || m_po_dmat4x2_uniform_location == -1 ||
		m_po_dmat4x3_arr_uniform_location == -1 || m_po_dmat4x3_uniform_location == -1 ||
		m_po_double_arr_uniform_location == -1 || m_po_double_uniform_location == -1 ||
		m_po_dvec2_arr_uniform_location == -1 || m_po_dvec2_uniform_location == -1 ||
		m_po_dvec3_arr_uniform_location == -1 || m_po_dvec3_uniform_location == -1 ||
		m_po_dvec4_arr_uniform_location == -1 || m_po_dvec4_uniform_location == -1 ||
		m_po_float_arr_uniform_location == -1 || m_po_float_uniform_location == -1 ||
		m_po_int_arr_uniform_location == -1 || m_po_int_uniform_location == -1 ||
		m_po_ivec2_arr_uniform_location == -1 || m_po_ivec2_uniform_location == -1 ||
		m_po_ivec3_arr_uniform_location == -1 || m_po_ivec3_uniform_location == -1 ||
		m_po_ivec4_arr_uniform_location == -1 || m_po_ivec4_uniform_location == -1 ||
		m_po_sampler_uniform_location == -1 || m_po_uint_arr_uniform_location == -1 ||
		m_po_uint_uniform_location == -1 || m_po_uvec2_arr_uniform_location == -1 ||
		m_po_uvec2_uniform_location == -1 || m_po_uvec3_arr_uniform_location == -1 ||
		m_po_uvec3_uniform_location == -1 || m_po_uvec4_arr_uniform_location == -1 ||
		m_po_uvec4_uniform_location == -1 || m_po_vec2_arr_uniform_location == -1 || m_po_vec2_uniform_location == -1 ||
		m_po_vec3_arr_uniform_location == -1 || m_po_vec3_uniform_location == -1 ||
		m_po_vec4_arr_uniform_location == -1 || m_po_vec4_uniform_location == -1)
	{
		TCU_FAIL("At last one of the required uniforms is considered inactive.");
	}
}

/** Tells wheter uniform at user-specified location represents a double-precision
 *  matrix uniform.
 *
 *  @param uniform_location Location of the uniform to use for the query.
 *
 *  @return Requested information.
 **/
bool GPUShaderFP64Test1::isMatrixUniform(glw::GLint uniform_location)
{
	return (uniform_location == m_po_dmat2_uniform_location || uniform_location == m_po_dmat2x3_uniform_location ||
			uniform_location == m_po_dmat2x4_uniform_location || uniform_location == m_po_dmat3_uniform_location ||
			uniform_location == m_po_dmat3x2_uniform_location || uniform_location == m_po_dmat3x4_uniform_location ||
			uniform_location == m_po_dmat4_uniform_location || uniform_location == m_po_dmat4x2_uniform_location ||
			uniform_location == m_po_dmat4x3_uniform_location);
}

/** Tells whether user-specified uniform function corresponds to one of the
 *  functions in glUniformMatrix*() class.
 *
 *  @param func Uniform function enum to use for the query.
 *
 *  @return true if the specified enum represents one of the glUniformMatrix*() functions,
 *          false otherwise.
 **/
bool GPUShaderFP64Test1::isMatrixUniformFunction(_uniform_function func)
{
	return (func == UNIFORM_FUNCTION_MATRIX2DV || func == UNIFORM_FUNCTION_MATRIX2X3DV ||
			func == UNIFORM_FUNCTION_MATRIX2X4DV || func == UNIFORM_FUNCTION_MATRIX3DV ||
			func == UNIFORM_FUNCTION_MATRIX3X2DV || func == UNIFORM_FUNCTION_MATRIX3X4DV ||
			func == UNIFORM_FUNCTION_MATRIX4DV || func == UNIFORM_FUNCTION_MATRIX4X2DV ||
			func == UNIFORM_FUNCTION_MATRIX4X3DV);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GPUShaderFP64Test1::iterate()
{
	/* Do not execute the test if GL_ARB_texture_view is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported.");
	}

	/* Initialize all ES objects required to run all the checks */
	initTest();

	/* Make sure GL_INVALID_OPERATION is generated by glUniform*() and
	 * glUniformMatrix*() functions if there is no current program object.
	 */
	m_has_test_passed &= verifyErrorGenerationWhenUniformFunctionsCalledWithoutActivePO();

	/* Make sure GL_INVALID_OPERATION is generated by glUniform*() if
	 * the size of the uniform variable declared in the shader does not
	 * match the size indicated by the command.
	 */
	m_has_test_passed &= verifyErrorGenerationWhenCallingSizeMismatchedUniformFunctions();

	/* Make sure GL_INVALID_OPERATION is generated if glUniform*() and
	 * glUniformMatrix*() are used to load a uniform variable of type
	 * bool, bvec2, bvec3, bvec4, float, int, ivec2, ivec3, ivec4,
	 * unsigned int, uvec2, uvec3, uvec4, vec2, vec3, vec4 or an array
	 * of these.
	 */
	m_has_test_passed &= verifyErrorGenerationWhenCallingTypeMismatchedUniformFunctions();

	/* Make sure GL_INVALID_OPERATION is generated if glUniform*() and
	 * glUniformMatrix*() are used to load incompatible double-typed
	 * uniforms, as presented below:
	 *
	 * I.    double-typed uniform configured by glUniform2d();
	 * II.   double-typed uniform configured by glUniform3d();
	 * III.  double-typed uniform configured by glUniform4d();
	 * IV.   double-typed uniform configured by glUniformMatrix*();
	 * V.    dvec2-typed  uniform configured by glUniform1d();
	 * VI.   dvec2-typed  uniform configured by glUniform3d();
	 * VII.  dvec2-typed  uniform configured by glUniform4d();
	 * VIII. dvec2-typed  uniform configured by glUniformMatrix*();
	 *
	 *                          (etc.)
	 *
	 */
	m_has_test_passed &= verifyErrorGenerationWhenCallingMismatchedDoubleUniformFunctions();

	/* Make sure GL_INVALID_OPERATION is generated if <location> of
	 * glUniform*() and glUniformMatrix*() is an invalid uniform
	 * location for the current program object and location is not
	 * equal to -1.
	 */
	m_has_test_passed &= verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithInvalidLocation();

	/* Make sure GL_INVALID_VALUE is generated if <count> of
	 * glUniform*() (*dv() functions only) and glUniformMatrix*() is
	 * negative.
	 */
	m_has_test_passed &= verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithNegativeCount();

	/* Make sure GL_INVALID_OPERATION is generated if <count> of
	 * glUniform*() (*dv() functions only) and glUniformMatrix*() is
	 * greater than 1 and the indicated uniform variable is not an
	 * array variable.
	 */
	m_has_test_passed &= verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithInvalidCount();

	/* Make sure GL_INVALID_OPERATION is generated if a sampler is
	 * loaded by glUniform*() and glUniformMatrix*().
	 */
	m_has_test_passed &= verifyErrorGenerationWhenCallingDoubleUniformFunctionsForSamplers();

	/* Make sure GL_INVALID_OPERATION is generated if glUniform*() and
	 * glUniformMatrix*() is used to load values for uniforms of
	 * boolean types.
	 */
	m_has_test_passed &= verifyErrorGenerationWhenCallingDoubleUniformFunctionsForBooleans();

	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies GL_INVALID_OPERATION is generated if any of the glUniform*d(), glUniform*dv() or
 *  glUniformMatrix*dv() functions is used to load a boolean uniform.
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenCallingDoubleUniformFunctionsForBooleans()
{
	const double double_data[] = {
		1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0
	};
	const glw::Functions& gl				  = m_context.getRenderContext().getFunctions();
	bool				  result			  = true;
	glw::GLint			  uniform_locations[] = { m_po_bool_arr_uniform_location,  m_po_bool_uniform_location,
									   m_po_bvec2_arr_uniform_location, m_po_bvec2_uniform_location,
									   m_po_bvec3_arr_uniform_location, m_po_bvec3_uniform_location,
									   m_po_bvec4_arr_uniform_location, m_po_bvec4_uniform_location };
	const unsigned int n_uniform_locations = sizeof(uniform_locations) / sizeof(uniform_locations[0]);

	for (unsigned int n_uniform_function = UNIFORM_FUNCTION_FIRST; n_uniform_function < UNIFORM_FUNCTION_COUNT;
		 ++n_uniform_function)
	{
		const _uniform_function uniform_function = (_uniform_function)n_uniform_function;

		for (unsigned int n_uniform_location = 0; n_uniform_location < n_uniform_locations; ++n_uniform_location)
		{
			const glw::GLint uniform_location = uniform_locations[n_uniform_location];

			switch (uniform_function)
			{
			case UNIFORM_FUNCTION_1D:
				gl.uniform1d(uniform_location, 0.0);
				break;
			case UNIFORM_FUNCTION_2D:
				gl.uniform2d(uniform_location, 0.0, 1.0);
				break;
			case UNIFORM_FUNCTION_3D:
				gl.uniform3d(uniform_location, 0.0, 1.0, 2.0);
				break;
			case UNIFORM_FUNCTION_4D:
				gl.uniform4d(uniform_location, 0.0, 1.0, 2.0, 3.0);
				break;

			case UNIFORM_FUNCTION_1DV:
				gl.uniform1dv(uniform_location, 1 /* count */, double_data);
				break;
			case UNIFORM_FUNCTION_2DV:
				gl.uniform2dv(uniform_location, 1 /* count */, double_data);
				break;
			case UNIFORM_FUNCTION_3DV:
				gl.uniform3dv(uniform_location, 1 /* count */, double_data);
				break;
			case UNIFORM_FUNCTION_4DV:
				gl.uniform4dv(uniform_location, 1 /* count */, double_data);
				break;

			case UNIFORM_FUNCTION_MATRIX2DV:
				gl.uniformMatrix2dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX2X3DV:
				gl.uniformMatrix2x3dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX2X4DV:
				gl.uniformMatrix2x4dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3DV:
				gl.uniformMatrix3dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3X2DV:
				gl.uniformMatrix3x2dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3X4DV:
				gl.uniformMatrix3x4dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4DV:
				gl.uniformMatrix4dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4X2DV:
				gl.uniformMatrix4x2dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4X3DV:
				gl.uniformMatrix4x3dv(uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
				break;

			default:
			{
				TCU_FAIL("Unrecognized uniform function");
			}
			}

			/* Make sure GL_INVALID_OPERATION was generated by the call */
			const glw::GLenum error_code = gl.getError();

			if (error_code != GL_INVALID_OPERATION)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Function " << getUniformFunctionString(uniform_function)
								   << "() did not generate an error"
									  " when applied against a boolean uniform."
								   << tcu::TestLog::EndMessage;

				result = false;
			}
		} /* for (all bool uniforms) */
	}	 /* for (all uniform functions) */

	return result;
}

/** Verifies GL_INVALID_OPERATION is generated if any of the glUniform*d(), glUniform*dv() or
 *  glUniformMatrix*dv() functions is used to load a sampler2D uniform.
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenCallingDoubleUniformFunctionsForSamplers()
{
	const double double_data[] = {
		1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0
	};
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	for (unsigned int n_uniform_function = UNIFORM_FUNCTION_FIRST; n_uniform_function < UNIFORM_FUNCTION_COUNT;
		 ++n_uniform_function)
	{
		_uniform_function uniform_function = (_uniform_function)n_uniform_function;

		switch (uniform_function)
		{
		case UNIFORM_FUNCTION_1D:
			gl.uniform1d(m_po_sampler_uniform_location, 0.0);
			break;
		case UNIFORM_FUNCTION_2D:
			gl.uniform2d(m_po_sampler_uniform_location, 0.0, 1.0);
			break;
		case UNIFORM_FUNCTION_3D:
			gl.uniform3d(m_po_sampler_uniform_location, 0.0, 1.0, 2.0);
			break;
		case UNIFORM_FUNCTION_4D:
			gl.uniform4d(m_po_sampler_uniform_location, 0.0, 1.0, 2.0, 3.0);
			break;

		case UNIFORM_FUNCTION_1DV:
			gl.uniform1dv(m_po_sampler_uniform_location, 1 /* count */, double_data);
			break;
		case UNIFORM_FUNCTION_2DV:
			gl.uniform2dv(m_po_sampler_uniform_location, 1 /* count */, double_data);
			break;
		case UNIFORM_FUNCTION_3DV:
			gl.uniform3dv(m_po_sampler_uniform_location, 1 /* count */, double_data);
			break;
		case UNIFORM_FUNCTION_4DV:
			gl.uniform4dv(m_po_sampler_uniform_location, 1 /* count */, double_data);
			break;

		case UNIFORM_FUNCTION_MATRIX2DV:
			gl.uniformMatrix2dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX2X3DV:
			gl.uniformMatrix2x3dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX2X4DV:
			gl.uniformMatrix2x4dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX3DV:
			gl.uniformMatrix3dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX3X2DV:
			gl.uniformMatrix3x2dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX3X4DV:
			gl.uniformMatrix3x4dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX4DV:
			gl.uniformMatrix4dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX4X2DV:
			gl.uniformMatrix4x2dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX4X3DV:
			gl.uniformMatrix4x3dv(m_po_sampler_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;

		default:
		{
			TCU_FAIL("Unrecognized uniform function");
		}
		}

		/* Make sure GL_INVALID_OPERATION was generated by the call */
		const glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Function " << getUniformFunctionString(uniform_function)
							   << "() did not generate an error"
								  " when applied against a sampler uniform."
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all uniform functions) */

	return result;
}

/** Verifies GL_INVALID_OPERATION is generated if any of the glUniform*dv() or
 *  glUniformMatrix*dv() functions is used to load a compatible uniform using an
 *  invalid <count> argument.
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithInvalidCount()
{
	const glw::GLdouble double_values[16] = { 1.0, 2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,
											  9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0 };
	const glw::Functions&   gl					= m_context.getRenderContext().getFunctions();
	bool					result				= true;
	const _uniform_function uniform_functions[] = { UNIFORM_FUNCTION_1DV,		  UNIFORM_FUNCTION_2DV,
													UNIFORM_FUNCTION_3DV,		  UNIFORM_FUNCTION_4DV,
													UNIFORM_FUNCTION_MATRIX2DV,   UNIFORM_FUNCTION_MATRIX2X3DV,
													UNIFORM_FUNCTION_MATRIX2X4DV, UNIFORM_FUNCTION_MATRIX3DV,
													UNIFORM_FUNCTION_MATRIX3X2DV, UNIFORM_FUNCTION_MATRIX3X4DV,
													UNIFORM_FUNCTION_MATRIX4DV,   UNIFORM_FUNCTION_MATRIX4X2DV,
													UNIFORM_FUNCTION_MATRIX4X3DV };
	const glw::GLint uniforms[] = {
		m_po_bool_uniform_location,	m_po_bvec2_uniform_location,  m_po_bvec3_uniform_location,
		m_po_bvec4_uniform_location,   m_po_dmat2_uniform_location,  m_po_dmat2x3_uniform_location,
		m_po_dmat2x4_uniform_location, m_po_dmat3_uniform_location,  m_po_dmat3x2_uniform_location,
		m_po_dmat3x4_uniform_location, m_po_dmat4_uniform_location,  m_po_dmat4x2_uniform_location,
		m_po_dmat4x3_uniform_location, m_po_double_uniform_location, m_po_dvec2_uniform_location,
		m_po_dvec3_uniform_location,   m_po_dvec4_uniform_location,  m_po_float_uniform_location,
		m_po_int_uniform_location,	 m_po_ivec2_uniform_location,  m_po_ivec3_uniform_location,
		m_po_ivec4_uniform_location,   m_po_uint_uniform_location,   m_po_uvec2_uniform_location,
		m_po_uvec3_uniform_location,   m_po_uvec4_uniform_location,  m_po_vec2_uniform_location,
		m_po_vec3_uniform_location,	m_po_vec4_uniform_location
	};

	const unsigned int n_uniform_functions = sizeof(uniform_functions) / sizeof(uniform_functions[0]);
	const unsigned int n_uniforms		   = sizeof(uniforms) / sizeof(uniforms[0]);

	for (unsigned int n_uniform_function = 0; n_uniform_function < n_uniform_functions; ++n_uniform_function)
	{
		_uniform_function uniform_function = uniform_functions[n_uniform_function];

		for (unsigned int n_uniform = 0; n_uniform < n_uniforms; ++n_uniform)
		{
			glw::GLint uniform_location = uniforms[n_uniform];

			/* Make sure we only use glUniformMatrix*() functions with matrix uniforms,
			 * and glUniform*() functions with vector uniforms.
			 */
			bool is_matrix_uniform = isMatrixUniform(uniform_location);

			if (((false == is_matrix_uniform) && (true == isMatrixUniformFunction(uniform_function))) ||
				((true == is_matrix_uniform) && (false == isMatrixUniformFunction(uniform_function))))
			{
				continue;
			}

			/* Issue the call with an invalid <count> argument */
			switch (uniform_function)
			{
			case UNIFORM_FUNCTION_1DV:
				gl.uniform1dv(uniform_location, 2, double_values);
				break;
			case UNIFORM_FUNCTION_2DV:
				gl.uniform2dv(uniform_location, 2, double_values);
				break;
			case UNIFORM_FUNCTION_3DV:
				gl.uniform3dv(uniform_location, 2, double_values);
				break;
			case UNIFORM_FUNCTION_4DV:
				gl.uniform4dv(uniform_location, 2, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX2DV:
				gl.uniformMatrix2dv(uniform_location, 2, GL_FALSE, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX2X3DV:
				gl.uniformMatrix2x3dv(uniform_location, 2, GL_FALSE, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX2X4DV:
				gl.uniformMatrix2x4dv(uniform_location, 2, GL_FALSE, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX3DV:
				gl.uniformMatrix3dv(uniform_location, 2, GL_FALSE, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX3X2DV:
				gl.uniformMatrix3x2dv(uniform_location, 2, GL_FALSE, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX3X4DV:
				gl.uniformMatrix3x4dv(uniform_location, 2, GL_FALSE, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX4DV:
				gl.uniformMatrix4dv(uniform_location, 2, GL_FALSE, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX4X2DV:
				gl.uniformMatrix4x2dv(uniform_location, 2, GL_FALSE, double_values);
				break;
			case UNIFORM_FUNCTION_MATRIX4X3DV:
				gl.uniformMatrix4x3dv(uniform_location, 2, GL_FALSE, double_values);
				break;

			default:
			{
				TCU_FAIL("Unrecognized uniform function");
			}
			} /* switch (uniform_function) */

			/* Make sure GL_INVALID_VALUE was generated */
			glw::GLenum error_code = gl.getError();

			if (error_code != GL_INVALID_OPERATION)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Function " << getUniformFunctionString(uniform_function)
								   << "() "
									  "was called with an invalid count argument but did not generate a "
									  "GL_INVALID_OPERATION error"
								   << tcu::TestLog::EndMessage;

				result = false;
			}
		} /* for (all non-arrayed uniforms) */
	}	 /* for (all uniform functions) */

	return result;
}

/** Verifies GL_INVALID_OPERATION is generated if any of the glUniform*d(), glUniform*dv() or
 *  glUniformMatrix*dv() functions is used to load an uniform at an invalid location.
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithInvalidLocation()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Find the largest valid uniform location */
	const glw::GLint uniform_locations[] = {
		m_po_bool_arr_uniform_location,	m_po_bool_uniform_location,		  m_po_bvec2_arr_uniform_location,
		m_po_bvec2_uniform_location,	   m_po_bvec3_arr_uniform_location,   m_po_bvec3_uniform_location,
		m_po_bvec4_arr_uniform_location,   m_po_bvec4_uniform_location,		  m_po_dmat2_arr_uniform_location,
		m_po_dmat2_uniform_location,	   m_po_dmat2x3_arr_uniform_location, m_po_dmat2x3_uniform_location,
		m_po_dmat2x4_arr_uniform_location, m_po_dmat2x4_uniform_location,	 m_po_dmat3_arr_uniform_location,
		m_po_dmat3_uniform_location,	   m_po_dmat3x2_arr_uniform_location, m_po_dmat3x2_uniform_location,
		m_po_dmat3x4_arr_uniform_location, m_po_dmat3x4_uniform_location,	 m_po_dmat4_arr_uniform_location,
		m_po_dmat4_uniform_location,	   m_po_dmat4x2_arr_uniform_location, m_po_dmat4x2_uniform_location,
		m_po_dmat4x3_arr_uniform_location, m_po_dmat4x3_uniform_location,	 m_po_double_arr_uniform_location,
		m_po_double_uniform_location,	  m_po_dvec2_arr_uniform_location,   m_po_dvec2_uniform_location,
		m_po_dvec3_arr_uniform_location,   m_po_dvec3_uniform_location,		  m_po_dvec4_arr_uniform_location,
		m_po_dvec4_uniform_location,	   m_po_float_arr_uniform_location,   m_po_float_uniform_location,
		m_po_int_arr_uniform_location,	 m_po_int_uniform_location,		  m_po_ivec2_arr_uniform_location,
		m_po_ivec2_uniform_location,	   m_po_ivec3_arr_uniform_location,   m_po_ivec3_uniform_location,
		m_po_ivec4_arr_uniform_location,   m_po_ivec4_uniform_location,		  m_po_uint_arr_uniform_location,
		m_po_uint_uniform_location,		   m_po_uvec2_arr_uniform_location,   m_po_uvec2_uniform_location,
		m_po_uvec3_arr_uniform_location,   m_po_uvec3_uniform_location,		  m_po_uvec4_arr_uniform_location,
		m_po_uvec4_uniform_location,	   m_po_vec2_arr_uniform_location,	m_po_vec2_uniform_location,
		m_po_vec3_arr_uniform_location,	m_po_vec3_uniform_location,		  m_po_vec4_arr_uniform_location,
		m_po_vec4_uniform_location
	};
	const unsigned int n_uniform_locations	= sizeof(uniform_locations) / sizeof(uniform_locations[0]);
	glw::GLint		   valid_uniform_location = -1;

	for (unsigned int n_uniform_location = 0; n_uniform_location < n_uniform_locations; ++n_uniform_location)
	{
		glw::GLint uniform_location = uniform_locations[n_uniform_location];

		if (uniform_location > valid_uniform_location)
		{
			valid_uniform_location = uniform_location;
		}
	} /* for (all  uniform locations) */

	/* Iterate through all uniform functions and make sure GL_INVALID_OPERATION error is always generated
	 * for invalid uniform location that is != -1
	 */
	const double double_data[] = {
		1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0
	};
	const glw::GLint invalid_uniform_location = valid_uniform_location + 1;

	for (unsigned int n_uniform_function = UNIFORM_FUNCTION_FIRST; n_uniform_function < UNIFORM_FUNCTION_COUNT;
		 ++n_uniform_function)
	{
		_uniform_function uniform_function = (_uniform_function)n_uniform_function;

		switch (uniform_function)
		{
		case UNIFORM_FUNCTION_1D:
			gl.uniform1d(invalid_uniform_location, 0.0);
			break;
		case UNIFORM_FUNCTION_2D:
			gl.uniform2d(invalid_uniform_location, 0.0, 1.0);
			break;
		case UNIFORM_FUNCTION_3D:
			gl.uniform3d(invalid_uniform_location, 0.0, 1.0, 2.0);
			break;
		case UNIFORM_FUNCTION_4D:
			gl.uniform4d(invalid_uniform_location, 0.0, 1.0, 2.0, 3.0);
			break;

		case UNIFORM_FUNCTION_1DV:
			gl.uniform1dv(invalid_uniform_location, 1 /* count */, double_data);
			break;
		case UNIFORM_FUNCTION_2DV:
			gl.uniform2dv(invalid_uniform_location, 1 /* count */, double_data);
			break;
		case UNIFORM_FUNCTION_3DV:
			gl.uniform3dv(invalid_uniform_location, 1 /* count */, double_data);
			break;
		case UNIFORM_FUNCTION_4DV:
			gl.uniform4dv(invalid_uniform_location, 1 /* count */, double_data);
			break;

		case UNIFORM_FUNCTION_MATRIX2DV:
			gl.uniformMatrix2dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX2X3DV:
			gl.uniformMatrix2x3dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX2X4DV:
			gl.uniformMatrix2x4dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX3DV:
			gl.uniformMatrix3dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX3X2DV:
			gl.uniformMatrix3x2dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX3X4DV:
			gl.uniformMatrix3x4dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX4DV:
			gl.uniformMatrix4dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX4X2DV:
			gl.uniformMatrix4x2dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;
		case UNIFORM_FUNCTION_MATRIX4X3DV:
			gl.uniformMatrix4x3dv(invalid_uniform_location, 1 /* count */, GL_FALSE /* transpose */, double_data);
			break;

		default:
		{
			TCU_FAIL("Unrecognized uniform function");
		}
		}

		const glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Function " << getUniformFunctionString(uniform_function)
							   << "() did not generate an error"
								  " when passed an invalid uniform location different from -1."
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all uniform functions) */

	return result;
}

/** Verifies GL_INVALID_VALUE is generated if any of the glUniform*dv() or
 *  glUniformMatrix*dv() functions is used to load a compatible uniform using an
 *  invalid <count> argument of -1.
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithNegativeCount()
{
	const glw::GLdouble double_values[16] = { 1.0, 2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,
											  9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0 };
	const glw::Functions&   gl					= m_context.getRenderContext().getFunctions();
	bool					result				= true;
	const _uniform_function uniform_functions[] = { UNIFORM_FUNCTION_1DV,		  UNIFORM_FUNCTION_2DV,
													UNIFORM_FUNCTION_3DV,		  UNIFORM_FUNCTION_4DV,
													UNIFORM_FUNCTION_MATRIX2DV,   UNIFORM_FUNCTION_MATRIX2X3DV,
													UNIFORM_FUNCTION_MATRIX2X4DV, UNIFORM_FUNCTION_MATRIX3DV,
													UNIFORM_FUNCTION_MATRIX3X2DV, UNIFORM_FUNCTION_MATRIX3X4DV,
													UNIFORM_FUNCTION_MATRIX4DV,   UNIFORM_FUNCTION_MATRIX4X2DV,
													UNIFORM_FUNCTION_MATRIX4X3DV };
	const unsigned int n_uniform_functions = sizeof(uniform_functions) / sizeof(uniform_functions[0]);

	for (unsigned int n_uniform_function = 0; n_uniform_function < n_uniform_functions; ++n_uniform_function)
	{
		_uniform_function uniform_function = uniform_functions[n_uniform_function];

		switch (uniform_function)
		{
		case UNIFORM_FUNCTION_1DV:
			gl.uniform1dv(m_po_double_arr_uniform_location, -1, double_values);
			break;
		case UNIFORM_FUNCTION_2DV:
			gl.uniform2dv(m_po_dvec2_arr_uniform_location, -1, double_values);
			break;
		case UNIFORM_FUNCTION_3DV:
			gl.uniform3dv(m_po_dvec3_arr_uniform_location, -1, double_values);
			break;
		case UNIFORM_FUNCTION_4DV:
			gl.uniform4dv(m_po_dvec4_arr_uniform_location, -1, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX2DV:
			gl.uniformMatrix2dv(m_po_dmat2_arr_uniform_location, -1, GL_FALSE, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX2X3DV:
			gl.uniformMatrix2x3dv(m_po_dmat2x3_arr_uniform_location, -1, GL_FALSE, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX2X4DV:
			gl.uniformMatrix2x4dv(m_po_dmat2x4_arr_uniform_location, -1, GL_FALSE, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX3DV:
			gl.uniformMatrix3dv(m_po_dmat3_arr_uniform_location, -1, GL_FALSE, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX3X2DV:
			gl.uniformMatrix3x2dv(m_po_dmat3x2_arr_uniform_location, -1, GL_FALSE, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX3X4DV:
			gl.uniformMatrix3x4dv(m_po_dmat3x4_arr_uniform_location, -1, GL_FALSE, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX4DV:
			gl.uniformMatrix4dv(m_po_dmat4_arr_uniform_location, -1, GL_FALSE, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX4X2DV:
			gl.uniformMatrix4x2dv(m_po_dmat4x2_arr_uniform_location, -1, GL_FALSE, double_values);
			break;
		case UNIFORM_FUNCTION_MATRIX4X3DV:
			gl.uniformMatrix4x3dv(m_po_dmat4x3_arr_uniform_location, -1, GL_FALSE, double_values);
			break;

		default:
		{
			TCU_FAIL("Unrecognized uniform function");
		}
		} /* switch (uniform_function) */

		/* Make sure GL_INVALID_VALUE was generated */
		glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_VALUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Function " << getUniformFunctionString(uniform_function)
							   << "() "
								  "was called with a negative count argument but did not generate a "
								  "GL_INVALID_VALUE error"
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all uniform functions) */

	return result;
}

/** Verifies GL_INVALID_OPERATION is generated if any of the glUniform*d(), glUniform*dv() or
 *  glUniformMatrix*dv() functions is used to load an uniform that's incompatible with the
 *  function (as per spec).
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenCallingMismatchedDoubleUniformFunctions()
{
	const double		  double_data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	glw::GLenum			  error_code	= GL_NO_ERROR;
	const glw::Functions& gl			= m_context.getRenderContext().getFunctions();
	bool				  result		= true;

	const glw::GLint double_uniform_locations[] = { m_po_dmat2_uniform_location,   m_po_dmat2x3_uniform_location,
													m_po_dmat2x4_uniform_location, m_po_dmat3_uniform_location,
													m_po_dmat3x2_uniform_location, m_po_dmat3x4_uniform_location,
													m_po_dmat4_uniform_location,   m_po_dmat4x2_uniform_location,
													m_po_dmat4x3_uniform_location, m_po_double_uniform_location,
													m_po_dvec2_uniform_location,   m_po_dvec3_uniform_location,
													m_po_dvec4_uniform_location };
	const unsigned int n_double_uniform_locations =
		sizeof(double_uniform_locations) / sizeof(double_uniform_locations[0]);

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	for (unsigned int n_uniform_location = 0; n_uniform_location < n_double_uniform_locations; ++n_uniform_location)
	{
		glw::GLint uniform_location = double_uniform_locations[n_uniform_location];

		for (int function = static_cast<int>(UNIFORM_FUNCTION_FIRST);
			 function < static_cast<int>(UNIFORM_FUNCTION_COUNT); function++)
		{
			_uniform_function e_function = static_cast<_uniform_function>(function);
			/* Exclude valid combinations */
			if (((uniform_location == m_po_dmat2_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX2DV)) ||
				((uniform_location == m_po_dmat2x3_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX2X3DV)) ||
				((uniform_location == m_po_dmat2x4_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX2X4DV)) ||
				((uniform_location == m_po_dmat3_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX3DV)) ||
				((uniform_location == m_po_dmat3x2_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX3X2DV)) ||
				((uniform_location == m_po_dmat3x4_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX3X4DV)) ||
				((uniform_location == m_po_dmat4_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX4DV)) ||
				((uniform_location == m_po_dmat4x2_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX4X2DV)) ||
				((uniform_location == m_po_dmat4x3_uniform_location) && (e_function == UNIFORM_FUNCTION_MATRIX4X3DV)) ||
				((uniform_location == m_po_double_uniform_location) &&
				 ((e_function == UNIFORM_FUNCTION_1D) || (e_function == UNIFORM_FUNCTION_1DV))) ||
				((uniform_location == m_po_dvec2_uniform_location) &&
				 ((e_function == UNIFORM_FUNCTION_2D) || (e_function == UNIFORM_FUNCTION_2DV))) ||
				((uniform_location == m_po_dvec3_uniform_location) &&
				 ((e_function == UNIFORM_FUNCTION_3D) || (e_function == UNIFORM_FUNCTION_3DV))) ||
				((uniform_location == m_po_dvec4_uniform_location) &&
				 ((e_function == UNIFORM_FUNCTION_4D) || (e_function == UNIFORM_FUNCTION_4DV))))
			{
				continue;
			}

			switch (e_function)
			{
			case UNIFORM_FUNCTION_1D:
			{
				gl.uniform1d(uniform_location, double_data[0]);

				break;
			}

			case UNIFORM_FUNCTION_2D:
			{
				gl.uniform2d(uniform_location, double_data[0], double_data[1]);

				break;
			}

			case UNIFORM_FUNCTION_3D:
			{
				gl.uniform3d(uniform_location, double_data[0], double_data[1], double_data[2]);

				break;
			}

			case UNIFORM_FUNCTION_4D:
			{
				gl.uniform4d(uniform_location, double_data[0], double_data[1], double_data[2], double_data[3]);

				break;
			}

			case UNIFORM_FUNCTION_1DV:
				gl.uniform1dv(uniform_location, 1 /* count */, double_data);
				break;
			case UNIFORM_FUNCTION_2DV:
				gl.uniform2dv(uniform_location, 1 /* count */, double_data);
				break;
			case UNIFORM_FUNCTION_3DV:
				gl.uniform3dv(uniform_location, 1 /* count */, double_data);
				break;
			case UNIFORM_FUNCTION_4DV:
				gl.uniform4dv(uniform_location, 1 /* count */, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX2DV:
				gl.uniformMatrix2dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX2X3DV:
				gl.uniformMatrix2x3dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX2X4DV:
				gl.uniformMatrix2x4dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3DV:
				gl.uniformMatrix3dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3X2DV:
				gl.uniformMatrix3x2dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3X4DV:
				gl.uniformMatrix3x4dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4DV:
				gl.uniformMatrix4dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4X2DV:
				gl.uniformMatrix4x2dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4X3DV:
				gl.uniformMatrix4x3dv(uniform_location, 1 /* count */, GL_FALSE, double_data);
				break;

			default:
			{
				TCU_FAIL("Unrecognized function");
			}
			} /* switch (function) */

			/* Make sure GL_INVALID_OPERATION error was generated */
			error_code = gl.getError();

			if (error_code != GL_INVALID_OPERATION)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid error [" << error_code
								   << "] was generated when a mismatched "
									  "double-precision uniform function "
								   << getUniformFunctionString(e_function) << "() was used to configure uniform "
								   << getUniformNameForLocation(uniform_location) << "." << tcu::TestLog::EndMessage;

				result = false;
			}
		} /* for (all uniform functions) */
	}	 /* for (all uniform locations) */

	return result;
}

/** Verifies GL_INVALID_OPERATION is generated if any of the glUniform*d() or
 *  glUniform*dv() functions is used to load an uniform, size of which is incompatible
 *  with the function.
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenCallingSizeMismatchedUniformFunctions()
{
	const double		  double_data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	glw::GLenum			  error_code	= GL_NO_ERROR;
	const glw::Functions& gl			= m_context.getRenderContext().getFunctions();
	bool				  result		= true;

	const int data[] = {
		/* API function */ /* Uniform location */ /* Count (dv functions only) */
		(int)UNIFORM_FUNCTION_2D, m_po_double_uniform_location, 0, (int)UNIFORM_FUNCTION_2DV,
		m_po_double_uniform_location, 2, (int)UNIFORM_FUNCTION_3D, m_po_double_uniform_location, 0,
		(int)UNIFORM_FUNCTION_3DV, m_po_double_uniform_location, 2, (int)UNIFORM_FUNCTION_4D,
		m_po_double_uniform_location, 0, (int)UNIFORM_FUNCTION_4DV, m_po_double_uniform_location, 2,
		(int)UNIFORM_FUNCTION_1D, m_po_dvec2_uniform_location, 0, (int)UNIFORM_FUNCTION_1DV,
		m_po_dvec2_uniform_location, 2, (int)UNIFORM_FUNCTION_3D, m_po_dvec2_uniform_location, 0,
		(int)UNIFORM_FUNCTION_3DV, m_po_dvec2_uniform_location, 2, (int)UNIFORM_FUNCTION_4D,
		m_po_dvec2_uniform_location, 0, (int)UNIFORM_FUNCTION_4DV, m_po_dvec2_uniform_location, 2,
		(int)UNIFORM_FUNCTION_1D, m_po_dvec3_uniform_location, 0, (int)UNIFORM_FUNCTION_1DV,
		m_po_dvec3_uniform_location, 2, (int)UNIFORM_FUNCTION_2D, m_po_dvec3_uniform_location, 0,
		(int)UNIFORM_FUNCTION_2DV, m_po_dvec3_uniform_location, 2, (int)UNIFORM_FUNCTION_4D,
		m_po_dvec3_uniform_location, 0, (int)UNIFORM_FUNCTION_4DV, m_po_dvec3_uniform_location, 2,
		(int)UNIFORM_FUNCTION_1D, m_po_dvec4_uniform_location, 0, (int)UNIFORM_FUNCTION_1DV,
		m_po_dvec4_uniform_location, 2, (int)UNIFORM_FUNCTION_2D, m_po_dvec4_uniform_location, 0,
		(int)UNIFORM_FUNCTION_2DV, m_po_dvec4_uniform_location, 2, (int)UNIFORM_FUNCTION_3D,
		m_po_dvec4_uniform_location, 0, (int)UNIFORM_FUNCTION_3DV, m_po_dvec4_uniform_location, 2,
	};
	const unsigned int n_checks = sizeof(data) / sizeof(data[0]) / 3 /* entries per row */;

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	for (unsigned int n_check = 0; n_check < n_checks; ++n_check)
	{
		_uniform_function function		   = (_uniform_function)data[n_check * 3 + 0];
		int				  uniform_location = data[n_check * 3 + 1];
		int				  uniform_count	= data[n_check * 3 + 2];

		switch (function)
		{
		case UNIFORM_FUNCTION_1D:
			gl.uniform1d(uniform_location, 0.0);
			break;
		case UNIFORM_FUNCTION_1DV:
			gl.uniform1dv(uniform_location, uniform_count, double_data);
			break;
		case UNIFORM_FUNCTION_2D:
			gl.uniform2d(uniform_location, 0.0, 1.0);
			break;
		case UNIFORM_FUNCTION_2DV:
			gl.uniform2dv(uniform_location, uniform_count, double_data);
			break;
		case UNIFORM_FUNCTION_3D:
			gl.uniform3d(uniform_location, 0.0, 1.0, 2.0);
			break;
		case UNIFORM_FUNCTION_3DV:
			gl.uniform3dv(uniform_location, uniform_count, double_data);
			break;
		case UNIFORM_FUNCTION_4D:
			gl.uniform4d(uniform_location, 0.0, 1.0, 2.0, 3.0);
			break;
		case UNIFORM_FUNCTION_4DV:
			gl.uniform4dv(uniform_location, uniform_count, double_data);
			break;

		default:
		{
			DE_ASSERT(false);
		}
		} /* switch (function) */

		error_code = gl.getError();
		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << getUniformFunctionString(function)
							   << "() function did not generate GL_INVALID_OPERATION error when called for"
								  " a uniform of incompatible size. (check index: "
							   << n_check << ")" << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all checks) */

	return result;
}

/** Verifies GL_INVALID_OPERATION is generated if any of the glUniform*d() or
 *  glUniform*dv() functions is used to load an uniform, type of which is incompatible
 *  with the function.
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenCallingTypeMismatchedUniformFunctions()
{
	const double		  double_data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	glw::GLenum			  error_code	= GL_NO_ERROR;
	const glw::Functions& gl			= m_context.getRenderContext().getFunctions();
	bool				  result		= true;

	const glw::GLint nondouble_uniform_locations[] = { m_po_bool_uniform_location,  m_po_bvec2_uniform_location,
													   m_po_bvec3_uniform_location, m_po_bvec4_uniform_location,
													   m_po_float_uniform_location, m_po_int_uniform_location,
													   m_po_ivec2_uniform_location, m_po_ivec3_uniform_location,
													   m_po_ivec4_uniform_location, m_po_uint_uniform_location,
													   m_po_uvec2_uniform_location, m_po_uvec3_uniform_location,
													   m_po_uvec4_uniform_location, m_po_vec2_uniform_location,
													   m_po_vec3_uniform_location,  m_po_vec4_uniform_location };
	const unsigned int n_nondouble_uniform_locations =
		sizeof(nondouble_uniform_locations) / sizeof(nondouble_uniform_locations[0]);

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	for (unsigned int n_uniform_location = 0; n_uniform_location < n_nondouble_uniform_locations; ++n_uniform_location)
	{
		glw::GLint uniform_location = nondouble_uniform_locations[n_uniform_location];

		for (int function = static_cast<int>(UNIFORM_FUNCTION_FIRST);
			 function < static_cast<int>(UNIFORM_FUNCTION_COUNT); ++function)
		{
			switch (static_cast<_uniform_function>(function))
			{
			case UNIFORM_FUNCTION_1D:
				gl.uniform1d(uniform_location, 0.0);
				break;
			case UNIFORM_FUNCTION_1DV:
				gl.uniform1dv(uniform_location, 1, double_data);
				break;
			case UNIFORM_FUNCTION_2D:
				gl.uniform2d(uniform_location, 0.0, 1.0);
				break;
			case UNIFORM_FUNCTION_2DV:
				gl.uniform2dv(uniform_location, 1, double_data);
				break;
			case UNIFORM_FUNCTION_3D:
				gl.uniform3d(uniform_location, 0.0, 1.0, 2.0);
				break;
			case UNIFORM_FUNCTION_3DV:
				gl.uniform3dv(uniform_location, 1, double_data);
				break;
			case UNIFORM_FUNCTION_4D:
				gl.uniform4d(uniform_location, 0.0, 1.0, 2.0, 3.0);
				break;
			case UNIFORM_FUNCTION_4DV:
				gl.uniform4dv(uniform_location, 1, double_data);
				break;

			case UNIFORM_FUNCTION_MATRIX2DV:
				gl.uniformMatrix2dv(uniform_location, 1, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX2X3DV:
				gl.uniformMatrix2x3dv(uniform_location, 1, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX2X4DV:
				gl.uniformMatrix2x4dv(uniform_location, 1, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3DV:
				gl.uniformMatrix3dv(uniform_location, 1, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3X2DV:
				gl.uniformMatrix3x2dv(uniform_location, 1, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX3X4DV:
				gl.uniformMatrix3x4dv(uniform_location, 1, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4DV:
				gl.uniformMatrix4dv(uniform_location, 1, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4X2DV:
				gl.uniformMatrix4x2dv(uniform_location, 1, GL_FALSE, double_data);
				break;
			case UNIFORM_FUNCTION_MATRIX4X3DV:
				gl.uniformMatrix4x3dv(uniform_location, 1, GL_FALSE, double_data);
				break;

			default:
			{
				DE_ASSERT(false);
			}
			} /* switch (function) */

			error_code = gl.getError();
			if (error_code != GL_INVALID_OPERATION)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << getUniformFunctionString(static_cast<_uniform_function>(function))
								   << "() function did not generate GL_INVALID_OPERATION error when called for"
									  " a uniform of incompatible type."
								   << tcu::TestLog::EndMessage;

				result = false;
			}
		}
	} /* for (all checks) */

	return result;
}

/** Verifies GL_INVALID_OPERATION is generated if any of the glUniform*d() or
 *  glUniform*dv() functions are called without a bound program object.
 *
 *  @return true if the implementation was found to behave as expected, false otherwise.
 **/
bool GPUShaderFP64Test1::verifyErrorGenerationWhenUniformFunctionsCalledWithoutActivePO()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	for (int function = static_cast<int>(UNIFORM_FUNCTION_FIRST); function < static_cast<int>(UNIFORM_FUNCTION_COUNT);
		 function++)
	{
		const double data[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0 };

		switch (static_cast<_uniform_function>(function))
		{
		case UNIFORM_FUNCTION_1D:
			gl.uniform1d(m_po_double_uniform_location, 0.0);
			break;
		case UNIFORM_FUNCTION_1DV:
			gl.uniform1dv(m_po_double_uniform_location, 1, data);
			break;
		case UNIFORM_FUNCTION_2D:
			gl.uniform2d(m_po_dvec2_uniform_location, 0.0, 1.0);
			break;
		case UNIFORM_FUNCTION_2DV:
			gl.uniform2dv(m_po_dvec2_uniform_location, 1, data);
			break;
		case UNIFORM_FUNCTION_3D:
			gl.uniform3d(m_po_dvec3_uniform_location, 0.0, 1.0, 2.0);
			break;
		case UNIFORM_FUNCTION_3DV:
			gl.uniform3dv(m_po_dvec3_uniform_location, 1, data);
			break;
		case UNIFORM_FUNCTION_4D:
			gl.uniform4d(m_po_dvec4_uniform_location, 0.0, 1.0, 2.0, 3.0);
			break;
		case UNIFORM_FUNCTION_4DV:
			gl.uniform4dv(m_po_dvec4_uniform_location, 1, data);
			break;

		case UNIFORM_FUNCTION_MATRIX2DV:
			gl.uniformMatrix2dv(m_po_dmat2_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;
		case UNIFORM_FUNCTION_MATRIX2X3DV:
			gl.uniformMatrix2x3dv(m_po_dmat2x3_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;
		case UNIFORM_FUNCTION_MATRIX2X4DV:
			gl.uniformMatrix2x4dv(m_po_dmat2x4_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;
		case UNIFORM_FUNCTION_MATRIX3DV:
			gl.uniformMatrix3dv(m_po_dmat3_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;
		case UNIFORM_FUNCTION_MATRIX3X2DV:
			gl.uniformMatrix3x2dv(m_po_dmat3x2_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;
		case UNIFORM_FUNCTION_MATRIX3X4DV:
			gl.uniformMatrix3x4dv(m_po_dmat3x4_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;
		case UNIFORM_FUNCTION_MATRIX4DV:
			gl.uniformMatrix4dv(m_po_dmat4_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;
		case UNIFORM_FUNCTION_MATRIX4X2DV:
			gl.uniformMatrix4x2dv(m_po_dmat4x2_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;
		case UNIFORM_FUNCTION_MATRIX4X3DV:
			gl.uniformMatrix4x3dv(m_po_dmat4x3_uniform_location, 1, GL_FALSE /* transpose */, data);
			break;

		default:
		{
			TCU_FAIL("Unrecognized uniform function");
		}
		} /* switch (func) */

		/* Query the error code */
		glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Implementation did not return GL_INVALID_OPERATION when "
							   << getUniformFunctionString(static_cast<_uniform_function>(function))
							   << "() was called without an active program object" << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all uniform functions) */

	return result;
}

/* Defeinitions of static const symbols declared in GPUShaderFP64Test2 */
const glw::GLuint GPUShaderFP64Test2::m_n_captured_results = 1024;
const glw::GLint  GPUShaderFP64Test2::m_result_failure	 = 2;
const glw::GLint  GPUShaderFP64Test2::m_result_success	 = 1;
const glw::GLuint GPUShaderFP64Test2::m_texture_width	  = 32;
const glw::GLuint GPUShaderFP64Test2::m_texture_height	 = m_n_captured_results / m_texture_width;
const glw::GLuint GPUShaderFP64Test2::m_transform_feedback_buffer_size =
	m_n_captured_results * sizeof(captured_varying_type);
const glw::GLchar* GPUShaderFP64Test2::m_uniform_block_name				  = "UniformBlock";
const glw::GLenum  GPUShaderFP64Test2::ARB_MAX_COMPUTE_UNIFORM_COMPONENTS = 0x8263;

/** Constructor
 *
 * @param context Test context
 **/
GPUShaderFP64Test2::GPUShaderFP64Test2(deqp::Context& context)
	: TestCase(context, "max_uniform_components",
			   "Verifies that maximum allowed uniform components can be used as double-precision float types")
	, m_pDispatchCompute(0)
	, m_framebuffer_id(0)
	, m_texture_id(0)
	, m_transform_feedback_buffer_id(0)
	, m_uniform_buffer_id(0)
	, m_vertex_array_object_id(0)
{
	/* Nothing to be done */
}

/** Deinitialize test
 *
 **/
void GPUShaderFP64Test2::deinit()
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean frambuffer */
	if (0 != m_framebuffer_id)
	{
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl.deleteFramebuffers(1, &m_framebuffer_id);
		m_framebuffer_id = 0;
	}

	/* Clean texture */
	if (0 != m_texture_id)
	{
		gl.bindTexture(GL_TEXTURE_2D, 0);
		gl.bindImageTexture(0 /* unit */, 0 /* texture */, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
							GL_READ_ONLY, GL_RGBA8);
		gl.deleteTextures(1, &m_texture_id);
		m_texture_id = 0;
	}

	/* Clean buffers */
	if (0 != m_transform_feedback_buffer_id)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
		gl.deleteBuffers(1, &m_transform_feedback_buffer_id);
		m_transform_feedback_buffer_id = 0;
	}

	if (0 != m_uniform_buffer_id)
	{
		gl.bindBuffer(GL_UNIFORM_BUFFER, 0);
		gl.deleteBuffers(1, &m_uniform_buffer_id);
		m_uniform_buffer_id = 0;
	}

	/* Clean VAO */
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
tcu::TestNode::IterateResult GPUShaderFP64Test2::iterate()
{
	bool result = true;

	/* Check if extension is supported */
	if (false == m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported");
	}

	/* Initialize test */
	testInit();

	prepareShaderStages();
	prepareUniformTypes();

	/* For all shaders and uniform type combinations */
	for (std::vector<shaderStage>::const_iterator shader_stage = m_shader_stages.begin();
		 m_shader_stages.end() != shader_stage; ++shader_stage)
	{
		for (std::vector<uniformTypeDetails>::const_iterator uniform_type = m_uniform_types.begin();
			 m_uniform_types.end() != uniform_type; ++uniform_type)
		{
			/* Execute test */
			if (false == test(*shader_stage, *uniform_type))
			{
				result = false;
			}
		}
	}

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param n_columns Number of columns
 * @param n_rows    Number of rows
 **/
GPUShaderFP64Test2::uniformTypeDetails::uniformTypeDetails(glw::GLuint n_columns, glw::GLuint n_rows)
	: m_n_columns(n_columns), m_n_rows(n_rows)
{
	Utils::_variable_type type = Utils::getDoubleVariableType(n_columns, n_rows);

	m_type_name = Utils::getVariableTypeString(type);
	m_type		= Utils::getGLDataTypeOfVariableType(type);
}

/** Get primitive type captured with transform feedback
 *
 * @param shader_stage Tested shader stage id
 *
 * @return Primitive type
 **/
glw::GLenum GPUShaderFP64Test2::getCapturedPrimitiveType(shaderStage shader_stage) const
{
	switch (shader_stage)
	{
	case GEOMETRY_SHADER:
	case TESS_CTRL_SHADER:
	case TESS_EVAL_SHADER:
	case VERTEX_SHADER:
		return GL_POINTS;
		break;

	default:
		return GL_NONE;
		break;
	}
}

/** Get primitive type drawn with DrawArrays
 *
 * @param shader_stage Tested shader stage id
 *
 * @return Primitive type
 **/
glw::GLenum GPUShaderFP64Test2::getDrawPrimitiveType(shaderStage shader_stage) const
{
	switch (shader_stage)
	{
	case FRAGMENT_SHADER:
		return GL_TRIANGLE_FAN;
		break;

	case GEOMETRY_SHADER:
	case VERTEX_SHADER:
		return GL_POINTS;
		break;

	case TESS_CTRL_SHADER:
	case TESS_EVAL_SHADER:
		return GL_PATCHES;
		break;

	default:
		return GL_NONE;
		break;
	}
}

/** Get maximum allowed number of uniform components
 *
 * @param shader_stage Tested shader stage id
 *
 * @return Maxmimum uniform components
 **/
glw::GLuint GPUShaderFP64Test2::getMaxUniformComponents(shaderStage shader_stage) const
{
	const glw::Functions& gl					 = m_context.getRenderContext().getFunctions();
	glw::GLint			  max_uniform_components = 0;
	glw::GLenum			  pname					 = 0;

	switch (shader_stage)
	{
	case COMPUTE_SHADER:
		pname = ARB_MAX_COMPUTE_UNIFORM_COMPONENTS;
		break;
	case FRAGMENT_SHADER:
		pname = GL_MAX_FRAGMENT_UNIFORM_COMPONENTS;
		break;
	case GEOMETRY_SHADER:
		pname = GL_MAX_GEOMETRY_UNIFORM_COMPONENTS;
		break;
	case TESS_CTRL_SHADER:
		pname = GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS;
		break;
	case TESS_EVAL_SHADER:
		pname = GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS;
		break;
	case VERTEX_SHADER:
		pname = GL_MAX_VERTEX_UNIFORM_COMPONENTS;
		break;
	}

	gl.getIntegerv(pname, &max_uniform_components);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	return max_uniform_components;
}

/** Get maximum size allowed for an uniform block
 *
 * @return Maxmimum uniform block size
 **/
glw::GLuint GPUShaderFP64Test2::getMaxUniformBlockSize() const
{
	const glw::Functions& gl					 = m_context.getRenderContext().getFunctions();
	glw::GLint			  max_uniform_block_size = 0;

	gl.getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_uniform_block_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	return max_uniform_block_size;
}

/** Get number of components required to store single uniform of given type
 *
 * @param uniform_type Tested uniform type
 *
 * @return Number of components
 **/
glw::GLuint GPUShaderFP64Test2::getRequiredComponentsNumber(const uniformTypeDetails& uniform_type) const
{
	static const glw::GLuint type_size	 = 2; /* double takes 2 N */
	const glw::GLuint		 column_length = uniform_type.m_n_rows;

	if (1 == uniform_type.m_n_columns)
	{
		return type_size * column_length;
	}
	else
	{
		const glw::GLuint alignment = type_size * ((3 == column_length) ? 4 : column_length);

		return alignment * uniform_type.m_n_columns;
	}
}

/** Get size used for each member of a uniform array of a given type in a std140 column-major layout
 *
 * @param uniform_type Tested uniform type
 *
 * @return Size of a single member
 **/
glw::GLuint GPUShaderFP64Test2::getUniformTypeMemberSize(const uniformTypeDetails& uniform_type) const
{
	static const glw::GLuint vec4_size	 = 4 * Utils::getBaseVariableTypeComponentSize(Utils::VARIABLE_TYPE_FLOAT);
	const glw::GLuint		 column_length = uniform_type.m_n_rows;

	/** Size for a layout(std140, column_major) uniform_type uniform[] **/
	return vec4_size * ((column_length + 1) / 2) * uniform_type.m_n_columns;
}

/** Get the maximum amount of uniforms to be used in a shader stage for a given type
 *
 * @param shader_stage Tested shader stage id
 * @param uniform_type Tested uniform type
 *
 * @return Number of components
 **/
glw::GLuint GPUShaderFP64Test2::getAmountUniforms(shaderStage				shader_stage,
												  const uniformTypeDetails& uniform_type) const
{
	const glw::GLuint max_uniform_components   = getMaxUniformComponents(shader_stage);
	const glw::GLuint required_components	  = getRequiredComponentsNumber(uniform_type);
	const glw::GLuint n_uniforms			   = max_uniform_components / required_components;
	const glw::GLuint max_uniform_block_size   = getMaxUniformBlockSize();
	const glw::GLuint uniform_type_member_size = getUniformTypeMemberSize(uniform_type);
	const glw::GLuint max_uniforms			   = max_uniform_block_size / uniform_type_member_size;

	return max_uniforms < n_uniforms ? max_uniforms : n_uniforms;
}

/** Get name of shader stage
 *
 * @param shader_stage Tested shader stage id
 *
 * @return Name
 **/
const glw::GLchar* GPUShaderFP64Test2::getShaderStageName(shaderStage shader_stage) const
{
	switch (shader_stage)
	{
	case COMPUTE_SHADER:
		return "compute shader";
		break;
	case FRAGMENT_SHADER:
		return "fragment shader";
		break;
	case GEOMETRY_SHADER:
		return "geometry shader";
		break;
	case TESS_CTRL_SHADER:
		return "tesselation control shader";
		break;
	case TESS_EVAL_SHADER:
		return "tesselation evaluation shader";
		break;
	case VERTEX_SHADER:
		return "vertex shader";
		break;
	}

	return 0;
}

/** Inspect program to get: buffer_size, offset, strides and block index
 *
 * @param program_id          Program id
 * @param out_buffer_size     Size of uniform buffer
 * @param out_uniform_details Uniform offset and strides
 * @param uniform_block_index Uniform block index
 **/
void GPUShaderFP64Test2::inspectProgram(glw::GLuint program_id, glw::GLint n_uniforms,
										const uniformTypeDetails& uniform_type, glw::GLint& out_buffer_size,
										uniformDetails& out_uniform_details, glw::GLuint uniform_block_index) const
{
	glw::GLint				 array_stride = 0;
	std::vector<glw::GLchar> extracted_uniform_name;
	const glw::Functions&	gl			   = m_context.getRenderContext().getFunctions();
	glw::GLuint				 index		   = 0;
	glw::GLint				 matrix_stride = 0;
	glw::GLint				 offset		   = 0;
	glw::GLsizei			 size		   = 0;
	glw::GLenum				 type		   = 0;
	const glw::GLchar*		 uniform_name  = 0;
	std::string				 uniform_name_str;
	std::stringstream		 uniform_name_stream;

	/* Get index of uniform block */
	uniform_block_index = gl.getUniformBlockIndex(program_id, m_uniform_block_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformBlockIndex");

	if (GL_INVALID_INDEX == uniform_block_index)
	{
		TCU_FAIL("Unifom block is inactive");
	}

	/* Get size of uniform block */
	gl.getActiveUniformBlockiv(program_id, uniform_block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &out_buffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformBlockiv");

	if (0 == out_buffer_size)
	{
		TCU_FAIL("Unifom block size is 0");
	}

	/* Prepare uniform name */
	uniform_name_stream << "uniform_array";

	uniform_name_str = uniform_name_stream.str();
	uniform_name	 = uniform_name_str.c_str();

	/* Get index of uniform */
	gl.getUniformIndices(program_id, 1 /* count */, &uniform_name, &index);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformIndices");

	if (GL_INVALID_INDEX == index)
	{
		TCU_FAIL("Unifom is inactive");
	}

	/* Verify getActiveUniform results */
	extracted_uniform_name.resize(uniform_name_str.length() * 2);

	gl.getActiveUniform(program_id, index, (glw::GLsizei)(uniform_name_str.length() * 2) /* bufSize */, 0, &size, &type,
						&extracted_uniform_name[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniform");

	if ((n_uniforms != size) || (uniform_type.m_type != type))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid GetActiveUniform results."
											<< " Size: " << size << " expected: " << n_uniforms << ". Type: " << type
											<< " expected: " << uniform_type.m_type
											<< ". Name: " << &extracted_uniform_name[0] << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid GetActiveUniform results");
	}

	/* Get offset of uniform */
	gl.getActiveUniformsiv(program_id, 1 /* count */, &index, GL_UNIFORM_OFFSET, &offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformsiv");

	if (-1 == offset)
	{
		TCU_FAIL("Unifom has invalid offset");
	}

	out_uniform_details.m_offset = offset;

	/* Get matrix stride of uniform */
	gl.getActiveUniformsiv(program_id, 1 /* count */, &index, GL_UNIFORM_MATRIX_STRIDE, &matrix_stride);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformsiv");

	if (-1 == matrix_stride)
	{
		TCU_FAIL("Unifom has invalid matrix stride");
	}

	out_uniform_details.m_matrix_stride = matrix_stride;

	/* Get array stride of uniform */
	gl.getActiveUniformsiv(program_id, 1 /* count */, &index, GL_UNIFORM_ARRAY_STRIDE, &array_stride);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformsiv");

	if (-1 == matrix_stride)
	{
		TCU_FAIL("Unifom has invalid matrix stride");
	}

	out_uniform_details.m_array_stride = array_stride;
}

/** Prepare source code for "boilerplate" shaders
 *
 * @param stage_specific_layout    String that will replace STAGE_SPECIFIC_LAYOUT token
 * @param stage_specific_main_body String that will replace STAGE_SPECIFIC_MAIN_BODY token
 * @param out_source_code          Source code
 **/
void GPUShaderFP64Test2::prepareBoilerplateShader(const glw::GLchar* stage_specific_layout,
												  const glw::GLchar* stage_specific_main_body,
												  std::string&		 out_source_code) const
{
	/* Shader template */
	static const glw::GLchar* boilerplate_shader_template_code = "#version 400 core\n"
																 "\n"
																 "precision highp float;\n"
																 "\n"
																 "STAGE_SPECIFIC_LAYOUT"
																 "void main()\n"
																 "{\n"
																 "STAGE_SPECIFIC_MAIN_BODY"
																 "}\n"
																 "\n";

	std::string string = boilerplate_shader_template_code;

	/* Tokens */
	static const glw::GLchar* body_token   = "STAGE_SPECIFIC_MAIN_BODY";
	static const glw::GLchar* layout_token = "STAGE_SPECIFIC_LAYOUT";

	size_t search_position = 0;

	/* Replace tokens */
	Utils::replaceToken(layout_token, search_position, stage_specific_layout, string);
	Utils::replaceToken(body_token, search_position, stage_specific_main_body, string);

	/* Store resuls */
	out_source_code = string;
}

/** Prepare program for given combination of shader stage and uniform type
 *
 * @param shader_stage     Shader stage
 * @param uniform_type     Uniform type
 * @param out_program_info Instance of programInfo
 **/
void GPUShaderFP64Test2::prepareProgram(shaderStage shader_stage, const uniformTypeDetails& uniform_type,
										Utils::programInfo& out_program_info) const
{
	/* Stage specific layouts */
	static const glw::GLchar* geometry_shader_layout_code = "layout(points)                   in;\n"
															"layout(points, max_vertices = 1) out;\n"
															"\n";

	static const glw::GLchar* tess_ctrl_shader_layout_code = "layout(vertices = 1) out;\n"
															 "\n";

	static const glw::GLchar* tess_eval_shader_layout_code = "layout(isolines, point_mode) in;\n"
															 "\n";

	/* Stage specific main body */
	static const glw::GLchar* boilerplate_fragment_shader_body_code = "    discard;\n";

	static const glw::GLchar* boilerplate_tess_ctrl_shader_body_code = "    gl_TessLevelOuter[0] = 1.0;\n"
																	   "    gl_TessLevelOuter[1] = 1.0;\n"
																	   "    gl_TessLevelOuter[2] = 1.0;\n"
																	   "    gl_TessLevelOuter[3] = 1.0;\n"
																	   "    gl_TessLevelInner[0] = 1.0;\n"
																	   "    gl_TessLevelInner[1] = 1.0;\n";

	static const glw::GLchar* boilerplate_vertex_shader_body_code = "    gl_Position = vec4(1, 0, 0, 1);\n";

	static const glw::GLchar* corner_vertex_shader_body_code = "    if (0 == gl_VertexID)\n"
															   "    {\n"
															   "        gl_Position = vec4(-1, -1, 0, 1);\n"
															   "    }\n"
															   "    else if (1 == gl_VertexID)\n"
															   "    {\n"
															   "        gl_Position = vec4(-1, 1, 0, 1);\n"
															   "    }\n"
															   "    else if (2 == gl_VertexID)\n"
															   "    {\n"
															   "        gl_Position = vec4(1, 1, 0, 1);\n"
															   "    }\n"
															   "    else if (3 == gl_VertexID)\n"
															   "    {\n"
															   "        gl_Position = vec4(1, -1, 0, 1);\n"
															   "    }\n";

	static const glw::GLchar* passthrough_tess_eval_shader_body_code = "    result = tcs_tes_result[0];\n";

	static const glw::GLchar* test_shader_body_code = "\n    result = verification_result;\n";

	static const glw::GLchar* test_geometry_shader_body_code = "\n    result = verification_result;\n"
															   "\n"
															   "    EmitVertex();\n"
															   "    EndPrimitive();\n";

	static const glw::GLchar* test_tess_ctrl_shader_body_code =
		"\n    tcs_tes_result[gl_InvocationID] = verification_result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n";

	/* In variables */
	static const glw::GLchar* test_tess_ctrl_shader_in_variable = "in  int tcs_tes_result[];\n";

	/* Out variables */
	static const glw::GLchar* test_fragment_shader_out_variable = "layout(location = 0) out int result;\n";

	static const glw::GLchar* test_tess_ctrl_shader_out_variable = "out int tcs_tes_result[];\n";

	static const glw::GLchar* test_shader_out_variable = "out int result;\n";

	/* Varying name */
	static const glw::GLchar* varying_name = "result";
	glw::GLuint				  n_varyings   = 1;

	/* Storage for ready shaders */
	std::string compute_shader_code;
	std::string fragment_shader_code;
	std::string geometry_shader_code;
	std::string tess_ctrl_shader_code;
	std::string tess_eval_shader_code;
	std::string vertex_shader_code;

	/* Storage for uniform definition and verification code */
	std::string uniform_definitions;
	std::string uniform_verification;

	/* Get uniform definition and verification code */
	prepareUniformDefinitions(shader_stage, uniform_type, uniform_definitions);
	prepareUniformVerification(shader_stage, uniform_type, uniform_verification);

	/* Prepare vertex shader */
	switch (shader_stage)
	{
	case FRAGMENT_SHADER:

		prepareBoilerplateShader("", corner_vertex_shader_body_code, vertex_shader_code);

		break;

	case GEOMETRY_SHADER:
	case TESS_CTRL_SHADER:
	case TESS_EVAL_SHADER:

		prepareBoilerplateShader("", boilerplate_vertex_shader_body_code, vertex_shader_code);

		break;

	case VERTEX_SHADER:

		prepareTestShader("" /* layout */, uniform_definitions.c_str() /* uniforms */, "" /* in var */,
						  test_shader_out_variable /* out var */, uniform_verification.c_str() /* verification */,
						  test_shader_body_code /* body */, vertex_shader_code);

		break;

	default:
		break;
	}

	/* Prepare fragment shader */
	switch (shader_stage)
	{
	case FRAGMENT_SHADER:

		prepareTestShader("" /* layout */, uniform_definitions.c_str() /* uniforms */, "" /* in var */,
						  test_fragment_shader_out_variable /* out var */,
						  uniform_verification.c_str() /* verification */, test_shader_body_code /* body */,
						  fragment_shader_code);

		break;

	case GEOMETRY_SHADER:
	case TESS_CTRL_SHADER:
	case TESS_EVAL_SHADER:
	case VERTEX_SHADER:

		prepareBoilerplateShader("" /* layout */, boilerplate_fragment_shader_body_code /* body */,
								 fragment_shader_code);

		break;

	default:
		break;
	}

	/* Prepare compute, tess_ctrl, tess_eval, geometry shaders */
	switch (shader_stage)
	{
	case COMPUTE_SHADER:

		prepareTestComputeShader(uniform_definitions.c_str(), uniform_verification.c_str(), compute_shader_code);

		break;

	case GEOMETRY_SHADER:

		prepareTestShader(geometry_shader_layout_code /* layout */, uniform_definitions.c_str() /* uniforms */,
						  "" /* in var */, test_shader_out_variable /* out var */,
						  uniform_verification.c_str() /* verification */, test_geometry_shader_body_code /* body */,
						  geometry_shader_code);

		break;

	case TESS_CTRL_SHADER:

		prepareTestShader(tess_ctrl_shader_layout_code /* layout */, uniform_definitions.c_str() /* uniforms */,
						  "" /* in var */, test_tess_ctrl_shader_out_variable /* out var */,
						  uniform_verification.c_str() /* verification */, test_tess_ctrl_shader_body_code /* body */,
						  tess_ctrl_shader_code);

		prepareTestShader(tess_eval_shader_layout_code /* layout */, "" /* uniforms */,
						  test_tess_ctrl_shader_in_variable /* in var */, test_shader_out_variable /* out var */,
						  "" /* verification */, passthrough_tess_eval_shader_body_code /* body */,
						  tess_eval_shader_code);

		break;

	case TESS_EVAL_SHADER:

		prepareBoilerplateShader(tess_ctrl_shader_layout_code /* layout */,
								 boilerplate_tess_ctrl_shader_body_code /* body */, tess_ctrl_shader_code);

		prepareTestShader(tess_eval_shader_layout_code /* layout */, uniform_definitions.c_str() /* uniforms */,
						  "" /* in var */, test_shader_out_variable /* out var */,
						  uniform_verification.c_str() /* verification */, test_shader_body_code /* body */,
						  tess_eval_shader_code);

		break;

	default:
		break;
	};

	/* Select shaders that will be used by program */
	const glw::GLchar* cs_c_str  = 0;
	const glw::GLchar* fs_c_str  = 0;
	const glw::GLchar* gs_c_str  = 0;
	const glw::GLchar* tcs_c_str = 0;
	const glw::GLchar* tes_c_str = 0;
	const glw::GLchar* vs_c_str  = 0;

	if (false == compute_shader_code.empty())
	{
		cs_c_str = compute_shader_code.c_str();
	}

	if (false == fragment_shader_code.empty())
	{
		fs_c_str = fragment_shader_code.c_str();
	}

	if (false == geometry_shader_code.empty())
	{
		gs_c_str = geometry_shader_code.c_str();
	}

	if (false == tess_ctrl_shader_code.empty())
	{
		tcs_c_str = tess_ctrl_shader_code.c_str();
	}

	if (false == tess_eval_shader_code.empty())
	{
		tes_c_str = tess_eval_shader_code.c_str();
	}

	if (false == vertex_shader_code.empty())
	{
		vs_c_str = vertex_shader_code.c_str();
	}

	/* Compute and fragment shader results are stored in texture, do not set varyings for transfrom feedback */
	if ((COMPUTE_SHADER == shader_stage) || (FRAGMENT_SHADER == shader_stage))
	{
		n_varyings = 0;
	}

	/* Build */
	out_program_info.build(cs_c_str, fs_c_str, gs_c_str, tcs_c_str, tes_c_str, vs_c_str, &varying_name, n_varyings);
}

/** Prepare collection of tested shader stages
 *
 */
void GPUShaderFP64Test2::prepareShaderStages()
{
	/* m_pDispatchCompute is initialized only if compute_shader are supproted and context is at least 4.2 */
	if (0 != m_pDispatchCompute)
	{
		m_shader_stages.push_back(COMPUTE_SHADER);
	}

	m_shader_stages.push_back(FRAGMENT_SHADER);
	m_shader_stages.push_back(GEOMETRY_SHADER);
	m_shader_stages.push_back(TESS_CTRL_SHADER);
	m_shader_stages.push_back(TESS_EVAL_SHADER);
	m_shader_stages.push_back(VERTEX_SHADER);
}

/** Prepare source code for "tested" shader stage
 *
 * @param stage_specific_layout    String that will replace STAGE_SPECIFIC_LAYOUT token
 * @param uniform_definitions      String that will replace UNIFORM_DEFINITIONS token
 * @param in_variable_definitions  String that will replace IN_VARIABLE_DEFINITION token
 * @param out_variable_definitions String that will replace OUT_VARIABLE_DEFINITION token
 * @param uniform_verification     String that will replace UNIFORM_VERIFICATION token
 * @param stage_specific_main_body String that will replace STAGE_SPECIFIC_MAIN_BODY token
 * @param out_source_code          Shader source code
 **/
void GPUShaderFP64Test2::prepareTestShader(const glw::GLchar* stage_specific_layout,
										   const glw::GLchar* uniform_definitions,
										   const glw::GLchar* in_variable_definitions,
										   const glw::GLchar* out_variable_definitions,
										   const glw::GLchar* uniform_verification,
										   const glw::GLchar* stage_specific_main_body,
										   std::string&		  out_source_code) const
{
	/* Shader template */
	static const glw::GLchar* test_shader_template_code = "#version 400 core\n"
														  "\n"
														  "precision highp float;\n"
														  "\n"
														  "STAGE_SPECIFIC_LAYOUT"
														  "UNIFORM_DEFINITIONS"
														  "IN_VARIABLE_DEFINITION"
														  "OUT_VARIABLE_DEFINITION"
														  "\n"
														  "void main()\n"
														  "{\n"
														  "UNIFORM_VERIFICATION"
														  "STAGE_SPECIFIC_MAIN_BODY"
														  "}\n"
														  "\n";

	std::string string = test_shader_template_code;

	/* Tokens */
	static const glw::GLchar* body_token	= "STAGE_SPECIFIC_MAIN_BODY";
	static const glw::GLchar* in_var_token  = "IN_VARIABLE_DEFINITION";
	static const glw::GLchar* layout_token  = "STAGE_SPECIFIC_LAYOUT";
	static const glw::GLchar* out_var_token = "OUT_VARIABLE_DEFINITION";
	static const glw::GLchar* uni_def_token = "UNIFORM_DEFINITIONS";
	static const glw::GLchar* uni_ver_token = "UNIFORM_VERIFICATION";

	size_t search_position = 0;

	/* Replace tokens */
	Utils::replaceToken(layout_token, search_position, stage_specific_layout, string);
	Utils::replaceToken(uni_def_token, search_position, uniform_definitions, string);
	Utils::replaceToken(in_var_token, search_position, in_variable_definitions, string);
	Utils::replaceToken(out_var_token, search_position, out_variable_definitions, string);
	Utils::replaceToken(uni_ver_token, search_position, uniform_verification, string);
	Utils::replaceToken(body_token, search_position, stage_specific_main_body, string);

	/* Store resuls */
	out_source_code = string;
}

/** Prepare source code for "tested" compute shaders
 *
 * @param uniform_definitions  String that will replace UNIFORM_DEFINITIONS token
 * @param uniform_verification String that will replace UNIFORM_VERIFICATION token
 * @param out_source_code      Source code
 **/
void GPUShaderFP64Test2::prepareTestComputeShader(const glw::GLchar* uniform_definitions,
												  const glw::GLchar* uniform_verification,
												  std::string&		 out_source_code) const
{
	/* Shader template */
	static const glw::GLchar* test_shader_template_code =
		"#version 420 core\n"
		"#extension GL_ARB_compute_shader          : require\n"
		"#extension GL_ARB_shader_image_load_store : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"UNIFORM_DEFINITIONS"
		"layout(r32i) writeonly uniform iimage2D result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"UNIFORM_VERIFICATION"
		"\n"
		"    imageStore(result, ivec2(gl_WorkGroupID.xy), ivec4(verification_result, 0, 0, 0));\n"
		"}\n"
		"\n";

	std::string string = test_shader_template_code;

	/* Tokens */
	static const glw::GLchar* uni_def_token = "UNIFORM_DEFINITIONS";
	static const glw::GLchar* uni_ver_token = "UNIFORM_VERIFICATION";

	size_t search_position = 0;

	/* Replace tokens */
	Utils::replaceToken(uni_def_token, search_position, uniform_definitions, string);
	Utils::replaceToken(uni_ver_token, search_position, uniform_verification, string);

	/* Store resuls */
	out_source_code = string;
}

/** Prepare source code which defines uniforms for tested shader stage
 *
 * @param shader_stage    Shader stage id
 * @param uniform_type    Details of uniform type
 * @param out_source_code Source code
 **/
void GPUShaderFP64Test2::prepareUniformDefinitions(shaderStage shader_stage, const uniformTypeDetails& uniform_type,
												   std::string& out_source_code) const
{
	const glw::GLuint n_uniforms = getAmountUniforms(shader_stage, uniform_type);
	std::stringstream stream;

	/*
	 * layout(std140) uniform M_UNIFORM_BLOCK_NAME
	 * {
	 *     TYPE_NAME uniform_array[N_UNIFORMS];
	 * };
	 */
	stream << "layout(std140) uniform " << m_uniform_block_name << "\n"
																   "{\n";

	stream << "    " << uniform_type.m_type_name << " uniform_array[" << n_uniforms << "];\n";

	stream << "};\n\n";

	out_source_code = stream.str();
}

/** Prepare uniform buffer for test
 *
 * @param shader_stage Shader stage id
 * @param uniform_type Details of uniform type
 * @param program_info Program object info
 **/
void GPUShaderFP64Test2::prepareUniforms(shaderStage shader_stage, const uniformTypeDetails& uniform_type,
										 const Utils::programInfo& program_info) const
{
	glw::GLint				  buffer_size	 = 0;
	glw::GLuint				  element_ordinal = 1;
	const glw::Functions&	 gl			  = m_context.getRenderContext().getFunctions();
	const glw::GLuint		  n_columns		  = uniform_type.m_n_columns;
	const glw::GLuint		  n_rows		  = uniform_type.m_n_rows;
	const glw::GLuint		  n_elements	  = n_columns * n_rows;
	uniformDetails			  uniform_details;
	const glw::GLuint		  program_id = program_info.m_program_object_id;
	const glw::GLint		  n_uniforms = getAmountUniforms(shader_stage, uniform_type);
	std::vector<glw::GLubyte> uniform_buffer_data;
	glw::GLuint				  uniform_block_index = 0;

	/* Get uniform details */
	inspectProgram(program_id, n_uniforms, uniform_type, buffer_size, uniform_details, uniform_block_index);

	/* Uniform offset and strides */
	const glw::GLuint array_stride   = uniform_details.m_array_stride;
	const glw::GLuint matrix_stride  = uniform_details.m_matrix_stride;
	const glw::GLuint uniform_offset = uniform_details.m_offset;

	/* Prepare storage for buffer data */
	uniform_buffer_data.resize(buffer_size);

	/* Prepare uniform data */
	for (glw::GLint i = 0; i < n_uniforms; ++i)
	{
		const glw::GLuint array_entry_offset = uniform_offset + i * array_stride;

		for (glw::GLuint element = 0; element < n_elements; ++element, ++element_ordinal)
		{
			const glw::GLuint   column		 = element / n_rows;
			const glw::GLuint   column_elem  = element % n_rows;
			const glw::GLdouble value		 = element_ordinal;
			const glw::GLuint   value_offset = static_cast<glw::GLuint>(array_entry_offset + column * matrix_stride +
																	  column_elem * sizeof(glw::GLdouble));
			glw::GLdouble* value_dst = (glw::GLdouble*)&uniform_buffer_data[value_offset];

			*value_dst = value;
		}
	}

	/* Update uniform buffer with new set of data */
	gl.bindBuffer(GL_UNIFORM_BUFFER, m_uniform_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	gl.bufferData(GL_UNIFORM_BUFFER, buffer_size, &uniform_buffer_data[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");

	/* Bind uniform block to uniform buffer */
	gl.bindBufferRange(GL_UNIFORM_BUFFER, 0 /* index */, m_uniform_buffer_id, 0 /* offset */, buffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");

	gl.uniformBlockBinding(program_id, uniform_block_index, 0 /* binding */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformBlockBinding");
}

/** Prepare collection of tested uniform types
 *
 **/
void GPUShaderFP64Test2::prepareUniformTypes()
{
	m_uniform_types.push_back(uniformTypeDetails(1 /* n_columns */, 1 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(1 /* n_columns */, 2 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(1 /* n_columns */, 3 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(1 /* n_columns */, 4 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(2 /* n_columns */, 2 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(2 /* n_columns */, 3 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(2 /* n_columns */, 4 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(3 /* n_columns */, 2 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(3 /* n_columns */, 3 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(3 /* n_columns */, 4 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(4 /* n_columns */, 2 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(4 /* n_columns */, 3 /* n_rows */));
	m_uniform_types.push_back(uniformTypeDetails(4 /* n_columns */, 4 /* n_rows */));
}

/** Prepare source code that verifes uniform values
 *
 * @param shader_stage    Shader stage id
 * @param uniform_type    Details of uniform type
 * @param out_source_code Source code
 **/
void GPUShaderFP64Test2::prepareUniformVerification(shaderStage shader_stage, const uniformTypeDetails& uniform_type,
													std::string& out_source_code) const
{
	glw::GLuint		  element_ordinal = 1;
	const glw::GLuint n_columns		  = uniform_type.m_n_columns;
	const glw::GLuint n_rows		  = uniform_type.m_n_rows;
	const glw::GLuint n_elements	  = n_columns * n_rows;
	const glw::GLuint n_uniforms	  = getAmountUniforms(shader_stage, uniform_type);
	std::stringstream stream;

	/*
	 * int verification_result = M_RESULT_SUCCESS;
	 *
	 * for (int i = 0; i < N_UNIFORMS; ++i)
	 * {
	 *     if (TYPE_NAME(i * (N_ELEMENTS) + 1) != uniform_array[i])
	 *     {
	 *         verification_result = M_RESULT_FAILURE
	 *     }
	 * }
	 */
	stream << "    int verification_result = " << m_result_success << ";\n"
																	  "\n"
																	  "    for (int i = 0; i < "
		   << n_uniforms << "; ++i)\n"
							"    {\n"
							"        if ("
		   << uniform_type.m_type_name << "(";

	for (glw::GLuint element = 0; element < n_elements; ++element, ++element_ordinal)
	{
		stream << "i * (" << n_elements << ") + " << element + 1;

		if (n_elements != element + 1)
		{
			stream << ", ";
		}
	}

	stream << ") != uniform_array[i])\n"
			  "        {\n"
			  "           verification_result = "
		   << m_result_failure << ";\n"
								  "        }\n"
								  "    }\n";

	out_source_code = stream.str();
}

/** Execute test for given combination of "tested" shader stage and uniform type
 *
 * @param shader_stage Tested shader stage id
 * @param uniform_type Tested uniform type
 *
 * @return true if test passed, false otherwise
 **/
bool GPUShaderFP64Test2::test(shaderStage shader_stage, const uniformTypeDetails& uniform_type) const
{
	const glw::GLenum		draw_primitive = getDrawPrimitiveType(shader_stage);
	static const glw::GLint first_vertex   = 0;
	const glw::Functions&   gl			   = m_context.getRenderContext().getFunctions();
	const glw::GLsizei		n_vertices	 = (FRAGMENT_SHADER == shader_stage) ? 4 : m_n_captured_results;
	Utils::programInfo		program_info(m_context);
	bool					result = true;

	/* Prepare program */
	prepareProgram(shader_stage, uniform_type, program_info);

	gl.useProgram(program_info.m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	/* Prepare uniform buffer and bind it with uniform block */
	prepareUniforms(shader_stage, uniform_type, program_info);

	/* Prepare storage for test results */
	testBegin(program_info.m_program_object_id, shader_stage);

	/* Execute */
	if (COMPUTE_SHADER == shader_stage)
	{
		m_pDispatchCompute(m_texture_width, m_texture_height, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");
	}
	else
	{
		gl.drawArrays(draw_primitive, first_vertex, n_vertices);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");
	}

	/* Clean after test */
	testEnd(shader_stage);

	/* Check results */
	if (false == verifyResults(shader_stage))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Shader stage: " << getShaderStageName(shader_stage)
			<< ". Uniform type: " << uniform_type.m_type_name << tcu::TestLog::EndMessage;

		result = false;
	}

	return result;
}

/** Prepare transform feedback buffer, framebuffer or image unit for test results
 *
 * @param program_id   Program object id
 * @param shader_stage Tested shader stage id
 **/
void GPUShaderFP64Test2::testBegin(glw::GLuint program_id, shaderStage shader_stage) const
{
	std::vector<glw::GLint> buffer_data;
	const glw::GLenum		captured_primitive = getCapturedPrimitiveType(shader_stage);
	const glw::Functions&   gl				   = m_context.getRenderContext().getFunctions();

	/* Prepare buffer filled with m_result_failure */
	buffer_data.resize(m_n_captured_results);
	for (glw::GLuint i = 0; i < m_n_captured_results; ++i)
	{
		buffer_data[i] = m_result_failure;
	}

	/* Prepare buffer for test results */
	switch (shader_stage)
	{
	case GEOMETRY_SHADER:
	case TESS_CTRL_SHADER:
	case TESS_EVAL_SHADER:
	case VERTEX_SHADER:

		/* Verify getTransformFeedbackVarying results */
		{
			glw::GLsizei size = 0;
			glw::GLenum  type = 0;
			glw::GLchar  name[16];

			gl.getTransformFeedbackVarying(program_id, 0 /* index */, 16 /* bufSize */, 0 /* length */, &size, &type,
										   name);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetTransformFeedbackVarying");

			if ((1 != size) || (GL_INT != type) || (0 != strcmp("result", name)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Error. Invalid GetTransformFeedbackVarying results."
					<< " Size: " << size << " expected: " << 1 << ". Type: " << type << " expected: " << GL_INT
					<< ". Name: " << name << " expected: result" << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid GetTransformFeedbackVarying results");
			}
		}

		/* Create/clean transform feedback buffer */
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transform_feedback_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, m_transform_feedback_buffer_size, &buffer_data[0], GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");

		/* Set up transform feedback buffer */
		gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_transform_feedback_buffer_id, 0 /* offset */,
						   m_transform_feedback_buffer_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");

		gl.beginTransformFeedback(captured_primitive);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

		break;

	case FRAGMENT_SHADER:

		/* Clean texture */
		gl.bindTexture(GL_TEXTURE_2D, m_texture_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texSubImage2D(GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, m_texture_width, m_texture_height,
						 GL_RED_INTEGER, GL_INT, &buffer_data[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");

		/* Set up texture as color attachment 0 */
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0 /* level */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

		gl.viewport(0 /* x */, 0 /* y */, m_texture_width, m_texture_height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");

		break;

	case COMPUTE_SHADER:

		/* Clean texture */
		gl.bindTexture(GL_TEXTURE_2D, m_texture_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texSubImage2D(GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, m_texture_width, m_texture_height,
						 GL_RED_INTEGER, GL_INT, &buffer_data[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");

		glw::GLint location = gl.getUniformLocation(program_id, "result");
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");

		if (-1 == location)
		{
			TCU_FAIL("Inactive uniform \"result\"");
		}

		gl.uniform1i(location, 0 /* first image unit */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");

		/* Bind texture to first image unit */
		gl.bindImageTexture(0 /* first image unit */, m_texture_id, 0 /* level */, GL_FALSE /* layered */,
							0 /* layer */, GL_WRITE_ONLY, GL_R32I);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

		break;
	}
}

/** Unbind transform feedback buffer, framebuffer or image unit
 *
 * @param shader_stage Tested shader stage id
 **/
void GPUShaderFP64Test2::testEnd(shaderStage shader_stage) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	switch (shader_stage)
	{
	case GEOMETRY_SHADER:
	case TESS_CTRL_SHADER:
	case TESS_EVAL_SHADER:
	case VERTEX_SHADER:

		gl.endTransformFeedback();

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		break;

	case FRAGMENT_SHADER:

		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0 /* texture_id */,
								0 /* level */);

		gl.bindTexture(GL_TEXTURE_2D, 0);

		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		break;

	case COMPUTE_SHADER:

		gl.bindImageTexture(0 /* first image unit */, 0 /* texture_id */, 0 /* level */, GL_FALSE /* layered */,
							0 /* layer */, GL_WRITE_ONLY, GL_R32I);

		break;
	}
}

/** Initialize OpenGL objects for test
 *
 **/
void GPUShaderFP64Test2::testInit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* The test is in 4.0 group. However:
	 * - compute_shader is core since 4.3
	 * - compute_shader require at least version 4.2 of GL */
	if ((true == m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader")) &&
		(true == Utils::isGLVersionAtLeast(gl, 4 /* major */, 2 /* minor */)))
	{
		m_pDispatchCompute = (arbDispatchComputeFunc)gl.dispatchCompute;
	}

	/* Tesselation patch set up */
	gl.patchParameteri(GL_PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

	/* Generate FBO */
	gl.genFramebuffers(1, &m_framebuffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

	/* Prepare texture */
	gl.genTextures(1, &m_texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	gl.bindTexture(GL_TEXTURE_2D, m_texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R32I, m_texture_width, m_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");

	/* Prepare transform feedback buffer */
	gl.genBuffers(1, &m_transform_feedback_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

	/* Generate uniform buffer */
	gl.genBuffers(1, &m_uniform_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

	/* Prepare VAO */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

	gl.bindVertexArray(m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");
}

/** Result verification, expected result is that whole buffer is filled with m_result_success
 *
 * @param shader_stage Tested shader stage id
 **/
bool GPUShaderFP64Test2::verifyResults(shaderStage shader_stage) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if ((FRAGMENT_SHADER == shader_stage) || (COMPUTE_SHADER == shader_stage))
	{
		/* Verify contents of texture */

		/* Prepare storage for testure data */
		std::vector<glw::GLint> image_data;
		image_data.resize(m_texture_width * m_texture_height);

		/* Get texture contents */
		gl.bindTexture(GL_TEXTURE_2D, m_texture_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.getTexImage(GL_TEXTURE_2D, 0 /* level */, GL_RED_INTEGER, GL_INT, &image_data[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");

		for (glw::GLuint y = 0; y < m_texture_width; ++y)
		{
			for (glw::GLuint x = 0; x < m_texture_height; ++x)
			{
				const glw::GLuint offset = y * m_texture_width + x;
				const glw::GLint  value  = image_data[offset];

				if (m_result_success != value)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Error. Texture contents are wrong at (" << x << ", " << y << ")"
						<< tcu::TestLog::EndMessage;

					return false;
				}
			}
		}

		return true;
	}
	else
	{
		/* Verify contents of transform feedback buffer */

		bool result = true;

		/* Get transform feedback data */
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transform_feedback_buffer_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

		glw::GLint* feedback_data = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

		for (glw::GLuint i = 0; i < m_n_captured_results; ++i)
		{
			const glw::GLint value = feedback_data[i];

			if (m_result_success != value)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "Error. Transform feedback buffer contents are wrong at " << i
													<< tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}

		/* Unmap transform feedback buffer */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

		return result;
	}
}

/* Definitions of static const fields declared in GPUShaderFP64Test3 */
const glw::GLuint GPUShaderFP64Test3::m_result_failure = 0;
const glw::GLuint GPUShaderFP64Test3::m_result_success = 1;

const glw::GLchar* GPUShaderFP64Test3::m_uniform_block_name			 = "UniformBlock";
const glw::GLchar* GPUShaderFP64Test3::m_uniform_block_instance_name = "uniform_block";

const glw::GLchar* GPUShaderFP64Test3::m_varying_name_fs_out_fs_result   = "fs_out_fs_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_gs_fs_gs_result	= "gs_fs_gs_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_gs_fs_tcs_result   = "gs_fs_tcs_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_gs_fs_tes_result   = "gs_fs_tes_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_gs_fs_vs_result	= "gs_fs_vs_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_tcs_tes_tcs_result = "tcs_tes_tcs_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_tcs_tes_vs_result  = "tcs_tes_vs_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_tes_gs_tcs_result  = "tes_gs_tcs_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_tes_gs_tes_result  = "tes_gs_tes_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_tes_gs_vs_result   = "tes_gs_vs_result";
const glw::GLchar* GPUShaderFP64Test3::m_varying_name_vs_tcs_vs_result   = "vs_tcs_vs_result";

/* Definitions of static const fields declared in GPUShaderFP64Test3::programInfo */
const glw::GLint GPUShaderFP64Test3::programInfo::m_invalid_uniform_offset			 = -1;
const glw::GLint GPUShaderFP64Test3::programInfo::m_invalid_uniform_matrix_stride	= -1;
const glw::GLint GPUShaderFP64Test3::programInfo::m_non_matrix_uniform_matrix_stride = 0;

/** Constructor
 *
 * @param context Test context
 **/
GPUShaderFP64Test3::GPUShaderFP64Test3(deqp::Context& context)
	: TestCase(context, "named_uniform_blocks",
			   "Verifies usage of \"double precision\" floats in \"named uniform block\"")
{
	/* Nothing to be done */
}

/** Deinitialize test
 *
 **/
void GPUShaderFP64Test3::deinit()
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean programs */
	m_packed_program.deinit(m_context);
	m_shared_program.deinit(m_context);
	m_std140_program.deinit(m_context);

	/* Clean frambuffer */
	if (0 != m_framebuffer_id)
	{
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl.deleteFramebuffers(1, &m_framebuffer_id);

		m_framebuffer_id = 0;
	}

	/* Clean texture */
	if (0 != m_color_texture_id)
	{
		gl.bindTexture(GL_TEXTURE_2D, 0);
		gl.deleteTextures(1, &m_color_texture_id);

		m_color_texture_id = 0;
	}

	/* Clean buffers */
	if (0 != m_transform_feedback_buffer_id)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
		gl.deleteBuffers(1, &m_transform_feedback_buffer_id);

		m_transform_feedback_buffer_id = 0;
	}

	if (0 != m_uniform_buffer_id)
	{
		gl.bindBuffer(GL_UNIFORM_BUFFER, 0);
		gl.deleteBuffers(1, &m_uniform_buffer_id);

		m_uniform_buffer_id = 0;
	}

	/* Clean VAO */
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
tcu::TestNode::IterateResult GPUShaderFP64Test3::iterate()
{
	bool result = true;

	/* Check if extension is supported */
	if (false == m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported");
	}

	/* Initialize test */
	testInit();

	/* Test "packed" uniform buffer layout */
	if (false == test(PACKED))
	{
		result = false;
	}

	/* Test "shared" uniform buffer layout */
	if (false == test(SHARED))
	{
		result = false;
	}

	/* Test "std140" uniform buffer layout */
	if (false == test(STD140))
	{
		result = false;
	}

	/* Set result */
	if (true == result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 **/
GPUShaderFP64Test3::programInfo::programInfo()
	: m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_object_id(0)
	, m_tesselation_control_shader_id(0)
	, m_tesselation_evaluation_shader_id(0)
	, m_vertex_shader_id(0)
	, m_buffer_size(0)
	, m_uniform_block_index(0)
{
	/* Nothing to be done here */
}

/** Compile shader
 *
 * @param context     Test context
 * @param shader_id   Shader object id
 * @param shader_code Shader source code
 **/
void GPUShaderFP64Test3::programInfo::compile(deqp::Context& context, glw::GLuint shader_id,
											  const glw::GLchar* shader_code) const
{
	/* GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Compilation status */
	glw::GLint status = GL_FALSE;

	/* Set source code */
	gl.shaderSource(shader_id, 1 /* count */, &shader_code, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");

	/* Compile */
	gl.compileShader(shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CompileShader");

	/* Get compilation status */
	gl.getShaderiv(shader_id, GL_COMPILE_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

	/* Log compilation error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Error log length */
		gl.getShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

		/* Prepare storage */
		message.resize(length);

		/* Get error log */
		gl.getShaderInfoLog(shader_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

		/* Log */
		context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to compile shader:\n"
										  << &message[0] << "\nShader source\n"
										  << shader_code << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to compile shader");
	}
}

/** Cleans program and attached shaders
 *
 * @param context Test context
 **/
void GPUShaderFP64Test3::programInfo::deinit(deqp::Context& context)
{
	/* GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Restore default program */
	gl.useProgram(0);

	/* Clean program object */
	if (0 != m_program_object_id)
	{
		gl.deleteProgram(m_program_object_id);
		m_program_object_id = 0;
	}

	/* Clean shaders */
	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);
		m_fragment_shader_id = 0;
	}

	if (0 != m_geometry_shader_id)
	{
		gl.deleteShader(m_geometry_shader_id);
		m_geometry_shader_id = 0;
	}

	if (0 != m_tesselation_control_shader_id)
	{
		gl.deleteShader(m_tesselation_control_shader_id);
		m_tesselation_control_shader_id = 0;
	}

	if (0 != m_tesselation_evaluation_shader_id)
	{
		gl.deleteShader(m_tesselation_evaluation_shader_id);
		m_tesselation_evaluation_shader_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);
		m_vertex_shader_id = 0;
	}
}

/** Build program and query for uniform layout
 *
 * @param context                            Test context
 * @param uniform_details                  Collection of uniform details
 * @param fragment_shader_code               Fragment shader source code
 * @param geometry_shader_code               Geometry shader source code
 * @param tesselation_control_shader_code    Tesselation control shader source code
 * @param tesselation_evaluation_shader_code Tesselation evaluation shader source code
 * @param vertex_shader_code                 Vertex shader source code
 **/
void GPUShaderFP64Test3::programInfo::init(deqp::Context& context, const std::vector<uniformDetails> uniform_details,
										   const glw::GLchar* fragment_shader_code,
										   const glw::GLchar* geometry_shader_code,
										   const glw::GLchar* tesselation_control_shader_code,
										   const glw::GLchar* tesselation_evaluation_shader_code,
										   const glw::GLchar* vertex_shader_code)
{
	/* GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Names of varyings to be captured with transform feedback */
	static const glw::GLchar* varying_names[] = { m_varying_name_gs_fs_gs_result, m_varying_name_gs_fs_tcs_result,
												  m_varying_name_gs_fs_tes_result, m_varying_name_gs_fs_vs_result };
	static const glw::GLuint n_varying_names = sizeof(varying_names) / sizeof(varying_names[0]);

	/* Create shader objects */
	m_fragment_shader_id			   = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id			   = gl.createShader(GL_GEOMETRY_SHADER);
	m_tesselation_control_shader_id	= gl.createShader(GL_TESS_CONTROL_SHADER);
	m_tesselation_evaluation_shader_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	m_vertex_shader_id				   = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

	/* Create program object */
	m_program_object_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

	/* Set up names of varyings to be captured with transform feedback */
	gl.transformFeedbackVaryings(m_program_object_id, n_varying_names, varying_names, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TransformFeedbackVaryings");

	/* Compile shaders */
	compile(context, m_fragment_shader_id, fragment_shader_code);
	compile(context, m_geometry_shader_id, geometry_shader_code);
	compile(context, m_tesselation_control_shader_id, tesselation_control_shader_code);
	compile(context, m_tesselation_evaluation_shader_id, tesselation_evaluation_shader_code);
	compile(context, m_vertex_shader_id, vertex_shader_code);

	/* Link program */
	link(context);

	/* Inspect program object */
	/* Get index of named uniform block */
	m_uniform_block_index = gl.getUniformBlockIndex(m_program_object_id, m_uniform_block_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformBlockIndex");

	if (GL_INVALID_INDEX == m_uniform_block_index)
	{
		TCU_FAIL("Unifom block is inactive");
	}

	/* Get size of named uniform block */
	gl.getActiveUniformBlockiv(m_program_object_id, m_uniform_block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &m_buffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformBlockiv");

	if (0 == m_buffer_size)
	{
		TCU_FAIL("Unifom block size is 0");
	}

	/* Get information about "double precision" uniforms */
	for (std::vector<uniformDetails>::const_iterator it = uniform_details.begin(), end = uniform_details.end();
		 end != it; ++it)
	{
		const glw::GLchar* uniform_name = 0;
		std::string		   uniform_name_str;
		std::stringstream  uniform_name_stream;
		glw::GLuint		   index		 = 0;
		glw::GLint		   offset		 = 0;
		glw::GLint		   matrix_stride = 0;

		/* Uniform name = UNIFORM_BLOCK_NAME.UNIFORM_NAME */
		uniform_name_stream << m_uniform_block_name << "." << it->m_name;

		uniform_name_str = uniform_name_stream.str();
		uniform_name	 = uniform_name_str.c_str();

		/* Get index of uniform */
		gl.getUniformIndices(m_program_object_id, 1 /* count */, &uniform_name, &index);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformIndices");

		if (GL_INVALID_INDEX == index)
		{
			TCU_FAIL("Unifom is inactive");
		}

		/* Get offset of uniform */
		gl.getActiveUniformsiv(m_program_object_id, 1 /* count */, &index, GL_UNIFORM_OFFSET, &offset);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformsiv");

		if (m_invalid_uniform_offset == offset)
		{
			TCU_FAIL("Unifom has invalid offset");
		}

		m_uniform_offsets.push_back(offset);

		/* Get matrix stride of uniform */
		gl.getActiveUniformsiv(m_program_object_id, 1 /* count */, &index, GL_UNIFORM_MATRIX_STRIDE, &matrix_stride);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformsiv");

		if (m_invalid_uniform_matrix_stride == offset)
		{
			TCU_FAIL("Unifom has invalid matrix stride");
		}

		m_uniform_matrix_strides.push_back(matrix_stride);
	}
}

/** Attach shaders and link program
 *
 * @param context Test context
 **/
void GPUShaderFP64Test3::programInfo::link(deqp::Context& context) const
{
	/* GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Link status */
	glw::GLint status = GL_FALSE;

	/* Attach shaders */
	gl.attachShader(m_program_object_id, m_fragment_shader_id);
	gl.attachShader(m_program_object_id, m_geometry_shader_id);
	gl.attachShader(m_program_object_id, m_tesselation_control_shader_id);
	gl.attachShader(m_program_object_id, m_tesselation_evaluation_shader_id);
	gl.attachShader(m_program_object_id, m_vertex_shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");

	/* Link */
	gl.linkProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

	/* Get link status */
	gl.getProgramiv(m_program_object_id, GL_LINK_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

	/* Log link error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Get error log length */
		gl.getProgramiv(m_program_object_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

		message.resize(length);

		/* Get error log */
		gl.getProgramInfoLog(m_program_object_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

		/* Log */
		context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to link program:\n"
										  << &message[0] << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to link program");
	}
}

/** Returns "predefined" values that will be used to fill uniform data
 *
 * @param type_ordinal Ordinal number of "double precision" uniform type
 * @param element      Index of element in uniform
 *
 * @return "Predefined" value
 **/
glw::GLdouble GPUShaderFP64Test3::getExpectedValue(glw::GLuint type_ordinal, glw::GLuint element) const
{
	return m_base_type_ordinal + (13.0 * (glw::GLdouble)type_ordinal) +
		   ((m_base_element - (glw::GLdouble)element) / 4.0);
}

/** Returns a reference of programInfo instance specific for given buffer layout
 *
 * @param uniform_data_layout Buffer layout
 *
 * @return Reference to an instance of programInfo
 **/
const GPUShaderFP64Test3::programInfo& GPUShaderFP64Test3::getProgramInfo(uniformDataLayout uniform_data_layout) const
{
	const programInfo* program_info = 0;

	switch (uniform_data_layout)
	{
	case PACKED:

		program_info = &m_packed_program;

		break;

	case SHARED:

		program_info = &m_shared_program;

		break;

	case STD140:

		program_info = &m_std140_program;

		break;
	}

	return *program_info;
}

/** Get "name" of buffer layout
 *
 * @param uniform_data_layout Buffer layout
 *
 * @return "Name" of layout
 **/
const glw::GLchar* GPUShaderFP64Test3::getUniformLayoutName(uniformDataLayout uniform_data_layout) const
{
	const glw::GLchar* layout = "";

	switch (uniform_data_layout)
	{
	case PACKED:
		layout = "packed";
		break;
	case SHARED:
		layout = "shared";
		break;
	case STD140:
		layout = "std140";
		break;
	}

	return layout;
}

/** Prepare programInfo instance for specific buffer layout
 *
 * @param program_info        Instance of programInfo
 * @param uniform_data_layout Buffer layout
 **/
void GPUShaderFP64Test3::prepareProgram(programInfo& program_info, uniformDataLayout uniform_data_layout) const
{
	/* Storage for shader source code */
	std::stringstream fragment_shader_code;
	std::stringstream geometry_shader_code;
	std::stringstream tess_control_shader_code;
	std::stringstream tess_eval_shader_code;
	std::stringstream vertex_shader_code;

	/* Write preambles */
	writePreamble(fragment_shader_code, FRAGMENT_SHADER);
	writePreamble(geometry_shader_code, GEOMETRY_SHADER);
	writePreamble(tess_control_shader_code, TESS_CONTROL_SHADER);
	writePreamble(tess_eval_shader_code, TESS_EVAL_SHADER);
	writePreamble(vertex_shader_code, VERTEX_SHADER);

	/* Write definition of named uniform block */
	writeUniformBlock(fragment_shader_code, uniform_data_layout);
	writeUniformBlock(geometry_shader_code, uniform_data_layout);
	writeUniformBlock(tess_control_shader_code, uniform_data_layout);
	writeUniformBlock(tess_eval_shader_code, uniform_data_layout);
	writeUniformBlock(vertex_shader_code, uniform_data_layout);

	/* Write definitions of varyings */
	writeVaryingDeclarations(fragment_shader_code, FRAGMENT_SHADER);
	writeVaryingDeclarations(geometry_shader_code, GEOMETRY_SHADER);
	writeVaryingDeclarations(tess_control_shader_code, TESS_CONTROL_SHADER);
	writeVaryingDeclarations(tess_eval_shader_code, TESS_EVAL_SHADER);
	writeVaryingDeclarations(vertex_shader_code, VERTEX_SHADER);

	/* Write main routine */
	writeMainBody(fragment_shader_code, FRAGMENT_SHADER);
	writeMainBody(geometry_shader_code, GEOMETRY_SHADER);
	writeMainBody(tess_control_shader_code, TESS_CONTROL_SHADER);
	writeMainBody(tess_eval_shader_code, TESS_EVAL_SHADER);
	writeMainBody(vertex_shader_code, VERTEX_SHADER);

	/* Init programInfo instance */
	program_info.init(m_context, m_uniform_details, fragment_shader_code.str().c_str(),
					  geometry_shader_code.str().c_str(), tess_control_shader_code.str().c_str(),
					  tess_eval_shader_code.str().c_str(), vertex_shader_code.str().c_str());
}

/** Prepare uniform buffer
 *
 * @param program_info   Instance of programInfo
 * @param verify_offsets If uniform offsets should be verified against expected values
 *
 * @return false if uniform offsets verification result is failure, true otherwise
 **/
bool GPUShaderFP64Test3::prepareUniformBuffer(const programInfo& program_info, bool verify_offsets) const
{
	const glw::GLuint							buffer_size = program_info.m_buffer_size;
	const glw::Functions&						gl			= m_context.getRenderContext().getFunctions();
	bool										offset_verification_result = true;
	glw::GLuint									type_ordinal			   = 1;
	std::vector<uniformDetails>::const_iterator it_uniform_details		   = m_uniform_details.begin();
	std::vector<glw::GLint>::const_iterator		it_uniform_offsets		   = program_info.m_uniform_offsets.begin();
	std::vector<glw::GLint>::const_iterator it_uniform_matrix_strides = program_info.m_uniform_matrix_strides.begin();

	/* Prepare storage for uniform buffer data */
	std::vector<glw::GLubyte> buffer_data;
	buffer_data.resize(buffer_size);

	/* For each "double precision" uniform */
	for (/* start conditions already set up */; m_uniform_details.end() != it_uniform_details;
		 ++it_uniform_details, ++it_uniform_offsets, ++it_uniform_matrix_strides, ++type_ordinal)
	{
		const glw::GLint  matrix_stride  = *it_uniform_matrix_strides;
		const glw::GLuint n_columns		 = it_uniform_details->m_n_columns;
		const glw::GLuint n_elements	 = it_uniform_details->m_n_elements;
		const glw::GLuint column_length  = n_elements / n_columns;
		const glw::GLint  uniform_offset = *it_uniform_offsets;

		/* For each element of uniform */
		for (glw::GLuint element = 0; element < n_elements; ++element)
		{
			const glw::GLuint   column		= element / column_length;
			const glw::GLuint   column_elem = element % column_length;
			const glw::GLdouble value		= getExpectedValue(type_ordinal, element);
			const glw::GLuint   value_offset =
				static_cast<glw::GLuint>(uniform_offset + column * matrix_stride + column_elem * sizeof(glw::GLdouble));

			glw::GLdouble* value_dst = (glw::GLdouble*)&buffer_data[value_offset];

			/* Store value */
			*value_dst = value;
		}

		/* Uniform offset verification */
		if (true == verify_offsets)
		{
			const glw::GLint expected_offset = it_uniform_details->m_expected_std140_offset;

			if (expected_offset != uniform_offset)
			{
				if (true == offset_verification_result)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error" << tcu::TestLog::EndMessage;
				}

				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Uniform: " << it_uniform_details->m_name
					<< " has offset: " << uniform_offset << ". Expected offset: " << expected_offset
					<< tcu::TestLog::EndMessage;

				offset_verification_result = false;
			}
		}
	}

	/* Update uniform buffer with prepared data */
	gl.bindBuffer(GL_UNIFORM_BUFFER, m_uniform_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	gl.bufferData(GL_UNIFORM_BUFFER, buffer_size, &buffer_data[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");

	/* Bind uniform buffer as data source for named uniform block */
	gl.bindBufferRange(GL_UNIFORM_BUFFER, 0 /* index */, m_uniform_buffer_id, 0, buffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");

	gl.uniformBlockBinding(program_info.m_program_object_id, program_info.m_uniform_block_index, 0 /* binding */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformBlockBinding");

	/* Done */
	return offset_verification_result;
}

/** Prepare data, execute draw call and verify results
 *
 * @param uniform_data_layout
 *
 * @return true if test pass, false otherwise
 **/
bool GPUShaderFP64Test3::test(uniformDataLayout uniform_data_layout) const
{
	bool				  are_offsets_verified		 = (STD140 == uniform_data_layout);
	const glw::Functions& gl						 = m_context.getRenderContext().getFunctions();
	bool				  offset_verification_result = true;
	const programInfo&	program_info				 = getProgramInfo(uniform_data_layout);
	bool				  result					 = true;

	/* Set up program */
	gl.useProgram(program_info.m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	/* Prepare uniform buffer */
	offset_verification_result = prepareUniformBuffer(program_info, are_offsets_verified);

	if (true == are_offsets_verified && false == offset_verification_result)
	{
		/* Offsets verification failure was already reported, add info about buffer layout */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Uniform buffer layout: " << getUniformLayoutName(uniform_data_layout)
											<< tcu::TestLog::EndMessage;

		result = false;
	}

	/* Set up transform feedback buffer */
	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_transform_feedback_buffer_id, 0, 4 * sizeof(glw::GLint));
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	/* Begin transform feedback */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	/* Execute draw call for singe vertex */
	gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	/* Stop transform feedback */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	/* Verify results */
	if (false == verifyResults())
	{
		/* Result verificatioon failure was already reported, add info about layout */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Uniform buffer layout: " << getUniformLayoutName(uniform_data_layout)
											<< tcu::TestLog::EndMessage;

		result = false;
	}

	/* Done */
	return result;
}

void GPUShaderFP64Test3::testInit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Uniform block declaration with std140 offsets calculated
	 *                       | align | loc_req | begins | ends | offset in bytes | imp |
	 * ivec3   dummy1[3]     |     4 |      12 |      0 |   12 |               0 |     |
	 * double  double_value  |     2 |       2 |     12 |   14 |              48 | XXX |
	 * bool    dummy2        |     1 |       1 |     14 |   15 |              56 |     |
	 * dvec2   dvec2_value   |     4 |       4 |     16 |   20 |              64 | XXX |
	 * bvec3   dummy3        |     4 |       4 |     20 |   24 |              80 |     |
	 * dvec3   dvec3_value   |     8 |       8 |     24 |   32 |              96 | XXX |
	 * int     dummy4[3]     |     4 |      12 |     32 |   44 |             128 |     |
	 * dvec4   dvec4_value   |     8 |       8 |     48 |   56 |             192 | XXX |
	 * bool    dummy5        |     1 |       1 |     56 |   57 |             224 |     |
	 * bool    dummy6[2]     |     4 |       8 |     60 |   68 |             240 |     |
	 * dmat2   dmat2_value   |     4 |       8 |     68 |   76 |             272 | XXX |
	 * dmat3   dmat3_value   |     8 |      24 |     80 |  104 |             320 | XXX |
	 * bool    dummy7        |     1 |       1 |    104 |  105 |             416 |     |
	 * dmat4   dmat4_value   |     8 |      32 |    112 |  144 |             448 | XXX |
	 * dmat2x3 dmat2x3_value |     8 |      16 |    144 |  160 |             576 | XXX |
	 * uvec3   dummy8        |     4 |       4 |    160 |  164 |             640 |     |
	 * dmat2x4 dmat2x4_value |     8 |      16 |    168 |  184 |             672 | XXX |
	 * dmat3x2 dmat3x2_value |     4 |      12 |    184 |  196 |             736 | XXX |
	 * bool    dummy9        |     1 |       1 |    196 |  197 |             784 |     |
	 * dmat3x4 dmat3x4_value |     8 |      24 |    200 |  224 |             800 | XXX |
	 * int     dummy10       |     1 |       1 |    224 |  225 |             896 |     |
	 * dmat4x2 dmat4x2_value |     4 |      16 |    228 |  244 |             912 | XXX |
	 * dmat4x3 dmat4x3_value |     8 |      32 |    248 |  280 |             992 | XXX |
	 */

	/* Prepare "double precision" unfiorms' details */
	m_uniform_details.push_back(uniformDetails(48 /* off */, "double_value" /* name */, 1 /* n_columns */,
											   1 /* n_elements */, "double" /* type_name */));
	m_uniform_details.push_back(uniformDetails(64 /* off */, "dvec2_value" /* name */, 1 /* n_columns */,
											   2 /* n_elements */, "dvec2" /* type_name */));
	m_uniform_details.push_back(uniformDetails(96 /* off */, "dvec3_value" /* name */, 1 /* n_columns */,
											   3 /* n_elements */, "dvec3" /* type_name */));
	m_uniform_details.push_back(uniformDetails(192 /* off */, "dvec4_value" /* name */, 1 /* n_columns */,
											   4 /* n_elements */, "dvec4" /* type_name */));
	m_uniform_details.push_back(uniformDetails(272 /* off */, "dmat2_value" /* name */, 2 /* n_columns */,
											   4 /* n_elements */, "dmat2" /* type_name */));
	m_uniform_details.push_back(uniformDetails(320 /* off */, "dmat3_value" /* name */, 3 /* n_columns */,
											   9 /* n_elements */, "dmat3" /* type_name */));
	m_uniform_details.push_back(uniformDetails(448 /* off */, "dmat4_value" /* name */, 4 /* n_columns */,
											   16 /* n_elements */, "dmat4" /* type_name */));
	m_uniform_details.push_back(uniformDetails(576 /* off */, "dmat2x3_value" /* name */, 2 /* n_columns */,
											   6 /* n_elements */, "dmat2x3" /* type_name */));
	m_uniform_details.push_back(uniformDetails(672 /* off */, "dmat2x4_value" /* name */, 2 /* n_columns */,
											   8 /* n_elements */, "dmat2x4" /* type_name */));
	m_uniform_details.push_back(uniformDetails(736 /* off */, "dmat3x2_value" /* name */, 3 /* n_columns */,
											   6 /* n_elements */, "dmat3x2" /* type_name */));
	m_uniform_details.push_back(uniformDetails(800 /* off */, "dmat3x4_value" /* name */, 3 /* n_columns */,
											   12 /* n_elements */, "dmat3x4" /* type_name */));
	m_uniform_details.push_back(uniformDetails(912 /* off */, "dmat4x2_value" /* name */, 4 /* n_columns */,
											   8 /* n_elements */, "dmat4x2" /* type_name */));
	m_uniform_details.push_back(uniformDetails(992 /* off */, "dmat4x3_value" /* name */, 4 /* n_columns */,
											   12 /* n_elements */, "dmat4x3" /* type_name */));

	/* Get random values for getExpectedValue */
	m_base_element		= (glw::GLdouble)(rand() % 13);
	m_base_type_ordinal = (glw::GLdouble)(rand() % 213);

	/* Prepare programInfos for all buffer layouts */
	prepareProgram(m_packed_program, PACKED);
	prepareProgram(m_shared_program, SHARED);
	prepareProgram(m_std140_program, STD140);

	/* Generate buffers */
	gl.genBuffers(1, &m_transform_feedback_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

	gl.genBuffers(1, &m_uniform_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

	/* Prepare transform feedback buffer */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transform_feedback_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 4 * sizeof(glw::GLint), 0 /* data */, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");

	/* Prepare texture for color attachment 0 */
	gl.genTextures(1, &m_color_texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	gl.bindTexture(GL_TEXTURE_2D, m_color_texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_R32I, 1 /* width */, 1 /* height */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");

	/* Prepare FBO with color attachment 0 */
	gl.genFramebuffers(1, &m_framebuffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color_texture_id,
							0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

	gl.viewport(0 /* x */, 0 /* y */, 1 /* width */, 1 /* height */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");

	/* Prepare VAO */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

	gl.bindVertexArray(m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");

	/* Tesselation patch set up */
	gl.patchParameteri(GL_PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");
}

/** Verify contents of transform feedback buffer and framebuffer's color attachment 0
 *
 * @return true if all values are as expected, false otherwise
 **/
bool GPUShaderFP64Test3::verifyResults() const
{
	glw::GLint*			  feedback_data			  = 0;
	bool				  fragment_shader_result  = false;
	bool				  geometry_shader_result  = false;
	const glw::Functions& gl					  = m_context.getRenderContext().getFunctions();
	bool				  tess_ctrl_shader_result = false;
	bool				  tess_eval_shader_result = false;
	bool				  vertex_shader_result	= false;

	/* Prepare storage for testure data */
	std::vector<glw::GLint> image_data;
	image_data.resize(1);

	/* Get texture contents */
	gl.bindTexture(GL_TEXTURE_2D, m_color_texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.getTexImage(GL_TEXTURE_2D, 0 /* level */, GL_RED_INTEGER, GL_INT, &image_data[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");

	/* Get transform feedback data */
	feedback_data = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	/* Verify results */
	fragment_shader_result  = (m_result_success == image_data[0]);
	geometry_shader_result  = (m_result_success == feedback_data[0]);
	tess_ctrl_shader_result = (m_result_success == feedback_data[1]);
	tess_eval_shader_result = (m_result_success == feedback_data[2]);
	vertex_shader_result	= (m_result_success == feedback_data[3]);

	/* Unmap transform feedback buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	/* Set result */
	if (true != (fragment_shader_result && geometry_shader_result && tess_ctrl_shader_result &&
				 tess_eval_shader_result && vertex_shader_result))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error" << tcu::TestLog::EndMessage;

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Vertex shader stage result: " << vertex_shader_result
											<< tcu::TestLog::EndMessage;

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Tesselation control shader stage result: " << tess_ctrl_shader_result
											<< tcu::TestLog::EndMessage;

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Tesselation evaluation shader stage result: " << tess_eval_shader_result
											<< tcu::TestLog::EndMessage;

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Geometry shader stage result: " << geometry_shader_result
											<< tcu::TestLog::EndMessage;

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Fragment shader stage result: " << fragment_shader_result
											<< tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		return true;
	}
}

/** Write main routine of <shader_stage> shader to stream
 *
 * @param stream       Output stream with source code of shader
 * @param shader_stage Shader stage
 **/
void GPUShaderFP64Test3::writeMainBody(std::ostream& stream, shaderStage shader_stage) const
{
	glw::GLuint		   type_ordinal = 1;
	const glw::GLchar* varying_name = "";

	/* Select name for varying that will hold result of "that" shader_stage */
	switch (shader_stage)
	{
	case FRAGMENT_SHADER:
		varying_name = m_varying_name_fs_out_fs_result;
		break;
	case GEOMETRY_SHADER:
		varying_name = m_varying_name_gs_fs_gs_result;
		break;
	case TESS_CONTROL_SHADER:
		varying_name = m_varying_name_tcs_tes_tcs_result;
		break;
	case TESS_EVAL_SHADER:
		varying_name = m_varying_name_tes_gs_tes_result;
		break;
	case VERTEX_SHADER:
		varying_name = m_varying_name_vs_tcs_vs_result;
		break;
	}

	/* void main() */
	stream << "void main()\n"
			  "{\n";

	/* Tesselation levels output */
	if (TESS_CONTROL_SHADER == shader_stage)
	{
		stream << "gl_TessLevelOuter[0] = 1.0;\n"
				  "gl_TessLevelOuter[1] = 1.0;\n"
				  "gl_TessLevelOuter[2] = 1.0;\n"
				  "gl_TessLevelOuter[3] = 1.0;\n"
				  "gl_TessLevelInner[0] = 1.0;\n"
				  "gl_TessLevelInner[1] = 1.0;\n"
				  "\n";
	}

	/* For each "double precision" uniform
	 *
	 * [else] if (TYPE_NAME(PREDIFINED_VALUES) != NAMED_UNIFORM_BLOCK_NAME.UNIFORM_NAME)
	 * {
	 *     VARYING_NAME = m_result_failure;
	 * }
	 */
	for (std::vector<uniformDetails>::const_iterator it = m_uniform_details.begin(), end = m_uniform_details.end();
		 end != it; ++it, ++type_ordinal)
	{
		stream << "    ";

		/* First comparison is done with if, next with else if */
		if (1 != type_ordinal)
		{
			stream << "else ";
		}

		/* if (TYPE_NAME( */
		stream << "if (" << it->m_type_name << "(";

		/* PREDIFINED_VALUES */
		for (glw::GLuint element = 0; element < it->m_n_elements; ++element)
		{
			stream << getExpectedValue(type_ordinal, element);

			/* Separate with comma */
			if (it->m_n_elements != element + 1)
			{
				stream << ", ";
			}
		}

		/*
		 * ) != NAMED_UNIFORM_BLOCK_NAME.UNIFORM_NAME)
		 * {
		 *     VARYING_NAME
		 */
		stream << ") != " << m_uniform_block_instance_name << "." << it->m_name << ")\n"
																				   "    {\n"
																				   "        "
			   << varying_name;

		/* Tesselation control outputs have to be arrays indexed with gl_InvocationID */
		if (TESS_CONTROL_SHADER == shader_stage)
		{
			stream << "[gl_InvocationID]";
		}

		/*
		 * = m_result_failure;
		 * }
		 */
		stream << " = " << m_result_failure << ";\n"
			   << "    }\n";
	}

	/* If all comparisons are ok
	 *
	 *     else
	 *     {
	 *         VARYING_NAME = m_result_success;
	 *     }
	 */

	/*
	 * else
	 * {
	 *     VARYING_NAME
	 */
	stream << "    else\n"
			  "    {\n"
			  "        "
		   << varying_name;

	/* Tesselation control outputs have to be arrays indexed with gl_InvocationID */
	if (TESS_CONTROL_SHADER == shader_stage)
	{
		stream << "[gl_InvocationID]";
	}

	/*
	 * = m_result_success;
	 * }
	 *
	 */
	stream << " = " << m_result_success << ";\n"
		   << "    }\n"
		   << "\n";

	/* For each pair of "input/output" varyings
	 *
	 * OUTPUT_VARYING_NAME = INPUT_VARYING_NAME;
	 **/
	writeVaryingPassthrough(stream, shader_stage);

	/* Geometry shader have to emit vertex */
	if (GEOMETRY_SHADER == shader_stage)
	{
		stream << "\n"
				  "gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);"
				  "EmitVertex();\n"
				  "EndPrimitive();\n";
	}

	/* Close scope of main */
	stream << "}\n\n";
}

/** Write shader preamble to stream
 *
 * @param stream       Output stream with source code of shader
 * @param shader_stage Shader stage
 **/
void GPUShaderFP64Test3::writePreamble(std::ostream& stream, shaderStage shader_stage) const
{
	stream << "#version 400 core\n"
			  "\n"
			  "precision highp float;\n"
			  "\n";

	switch (shader_stage)
	{
	case FRAGMENT_SHADER:
		break;
	case GEOMETRY_SHADER:
		stream << "layout(points)                   in;\n"
				  "layout(points, max_vertices = 1) out;\n"
				  "\n";
		break;
	case TESS_CONTROL_SHADER:
		stream << "layout(vertices = 1) out;\n"
				  "\n";
		break;
	case TESS_EVAL_SHADER:
		stream << "layout(isolines, point_mode) in;\n"
				  "\n";
		break;
	case VERTEX_SHADER:
		break;
	}
}

/** Write name uniform blcok definition with specific layout to stream
 *
 * @param stream              Output stream with source code of shader
 * @param uniform_data_layout Buffer layout
 **/
void GPUShaderFP64Test3::writeUniformBlock(std::ostream& stream, uniformDataLayout uniform_data_layout) const
{
	const glw::GLchar* layout = getUniformLayoutName(uniform_data_layout);

	stream << "layout(" << layout << ") uniform " << m_uniform_block_name << "\n"
																			 "{\n"
																			 "    ivec3   dummy1[3];\n"
																			 "    double  double_value;\n"
																			 "    bool    dummy2;\n"
																			 "    dvec2   dvec2_value;\n"
																			 "    bvec3   dummy3;\n"
																			 "    dvec3   dvec3_value;\n"
																			 "    int     dummy4[3];\n"
																			 "    dvec4   dvec4_value;\n"
																			 "    bool    dummy5;\n"
																			 "    bool    dummy6[2];\n"
																			 "    dmat2   dmat2_value;\n"
																			 "    dmat3   dmat3_value;\n"
																			 "    bool    dummy7;\n"
																			 "    dmat4   dmat4_value;\n"
																			 "    dmat2x3 dmat2x3_value;\n"
																			 "    uvec3   dummy8;\n"
																			 "    dmat2x4 dmat2x4_value;\n"
																			 "    dmat3x2 dmat3x2_value;\n"
																			 "    bool    dummy9;\n"
																			 "    dmat3x4 dmat3x4_value;\n"
																			 "    int     dummy10;\n"
																			 "    dmat4x2 dmat4x2_value;\n"
																			 "    dmat4x3 dmat4x3_value;\n"
																			 "} "
		   << m_uniform_block_instance_name << ";\n";

	stream << "\n";
}

/** Write definitions of varyings specific for given <shader_stage> to stream
 *
 * @param stream       Output stream with source code of shader
 * @param shader_stage Shader stage
 **/
void GPUShaderFP64Test3::writeVaryingDeclarations(std::ostream& stream, shaderStage shader_stage) const
{
	static const glw::GLchar* const varying_type = "int";

	switch (shader_stage)
	{
	case FRAGMENT_SHADER:

		/* In */
		stream << "flat in " << varying_type << " " << m_varying_name_gs_fs_gs_result << ";\n";
		stream << "flat in " << varying_type << " " << m_varying_name_gs_fs_tcs_result << ";\n";
		stream << "flat in " << varying_type << " " << m_varying_name_gs_fs_tes_result << ";\n";
		stream << "flat in " << varying_type << " " << m_varying_name_gs_fs_vs_result << ";\n";

		stream << "\n";

		/* Out */
		stream << "layout (location = 0) out " << varying_type << " " << m_varying_name_fs_out_fs_result << ";\n";

		break;

	case GEOMETRY_SHADER:

		/* In */
		stream << "in  " << varying_type << " " << m_varying_name_tes_gs_tcs_result << "[];\n";
		stream << "in  " << varying_type << " " << m_varying_name_tes_gs_tes_result << "[];\n";
		stream << "in  " << varying_type << " " << m_varying_name_tes_gs_vs_result << "[];\n";

		stream << "\n";

		/* Out */
		stream << "flat out " << varying_type << " " << m_varying_name_gs_fs_gs_result << ";\n";
		stream << "flat out " << varying_type << " " << m_varying_name_gs_fs_tcs_result << ";\n";
		stream << "flat out " << varying_type << " " << m_varying_name_gs_fs_tes_result << ";\n";
		stream << "flat out " << varying_type << " " << m_varying_name_gs_fs_vs_result << ";\n";

		break;

	case TESS_CONTROL_SHADER:

		/* In */
		stream << "in  " << varying_type << " " << m_varying_name_vs_tcs_vs_result << "[];\n";

		stream << "\n";

		/* Out */
		stream << "out " << varying_type << " " << m_varying_name_tcs_tes_tcs_result << "[];\n";
		stream << "out " << varying_type << " " << m_varying_name_tcs_tes_vs_result << "[];\n";

		break;

	case TESS_EVAL_SHADER:

		/* In */
		stream << "in  " << varying_type << " " << m_varying_name_tcs_tes_tcs_result << "[];\n";
		stream << "in  " << varying_type << " " << m_varying_name_tcs_tes_vs_result << "[];\n";

		stream << "\n";

		/* Out */
		stream << "out " << varying_type << " " << m_varying_name_tes_gs_tcs_result << ";\n";
		stream << "out " << varying_type << " " << m_varying_name_tes_gs_tes_result << ";\n";
		stream << "out " << varying_type << " " << m_varying_name_tes_gs_vs_result << ";\n";

		break;

	case VERTEX_SHADER:

		/* Out */
		stream << "out " << varying_type << " " << m_varying_name_vs_tcs_vs_result << ";\n";

		break;
	}

	stream << "\n";
}

/** Write passthrough code of "input/output" varying pairs to stream
 *
 * @param stream       Output stream with source code of shader
 * @param shader_stage Shader stage
 **/
void GPUShaderFP64Test3::writeVaryingPassthrough(std::ostream& stream, shaderStage shader_stage) const
{
	switch (shader_stage)
	{
	case FRAGMENT_SHADER:
		break;

	case GEOMETRY_SHADER:

		stream << "    " << m_varying_name_gs_fs_tcs_result << " = " << m_varying_name_tes_gs_tcs_result << "[0];\n";
		stream << "    " << m_varying_name_gs_fs_tes_result << " = " << m_varying_name_tes_gs_tes_result << "[0];\n";
		stream << "    " << m_varying_name_gs_fs_vs_result << " = " << m_varying_name_tes_gs_vs_result << "[0];\n";

		break;

	case TESS_CONTROL_SHADER:

		stream << "    " << m_varying_name_tcs_tes_vs_result
			   << "[gl_InvocationID] = " << m_varying_name_vs_tcs_vs_result << "[0];\n";

		break;

	case TESS_EVAL_SHADER:

		stream << "    " << m_varying_name_tes_gs_tcs_result << " = " << m_varying_name_tcs_tes_tcs_result << "[0];\n";
		stream << "    " << m_varying_name_tes_gs_vs_result << " = " << m_varying_name_tcs_tes_vs_result << "[0];\n";

		break;

	case VERTEX_SHADER:

		break;
	}
}

/** Constructor. Sets all uniform locations to -1 and sets all
 *  values to 0.
 */
GPUShaderFP64Test4::_data::_data()
{
	memset(&uniform_double, 0, sizeof(uniform_double));
	memset(uniform_dvec2, 0, sizeof(uniform_dvec2));
	memset(uniform_dvec2_arr, 0, sizeof(uniform_dvec2_arr));
	memset(uniform_dvec3, 0, sizeof(uniform_dvec3));
	memset(uniform_dvec3_arr, 0, sizeof(uniform_dvec3_arr));
	memset(uniform_dvec4, 0, sizeof(uniform_dvec4));
	memset(uniform_dvec4_arr, 0, sizeof(uniform_dvec4_arr));

	uniform_location_double		   = -1;
	uniform_location_double_arr[0] = -1;
	uniform_location_double_arr[1] = -1;
	uniform_location_dvec2		   = -1;
	uniform_location_dvec2_arr[0]  = -1;
	uniform_location_dvec2_arr[1]  = -1;
	uniform_location_dvec3		   = -1;
	uniform_location_dvec3_arr[0]  = -1;
	uniform_location_dvec3_arr[1]  = -1;
	uniform_location_dvec4		   = -1;
	uniform_location_dvec4_arr[0]  = -1;
	uniform_location_dvec4_arr[1]  = -1;
}

/** Constructor
 *
 *  @param context Rendering context.
 */
GPUShaderFP64Test4::GPUShaderFP64Test4(deqp::Context& context)
	: TestCase(context, "state_query", "Verifies glGet*() calls, as well as \"program interface query\"-specific tools"
									   " report correct properties of & values assigned to double-precision uniforms.")
	, m_has_test_passed(true)
	, m_uniform_name_buffer(0)
	, m_cs_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_cs_id(0)
	, m_po_noncs_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects, as well as releases all bufers, that may
 *  have beenallocated or  created during test execution.
 **/
void GPUShaderFP64Test4::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_cs_id != 0)
	{
		gl.deleteShader(m_cs_id);

		m_cs_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_cs_id != 0)
	{
		gl.deleteProgram(m_po_cs_id);

		m_po_cs_id = 0;
	}

	if (m_po_noncs_id != 0)
	{
		gl.deleteProgram(m_po_noncs_id);

		m_po_noncs_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_uniform_name_buffer != DE_NULL)
	{
		delete[] m_uniform_name_buffer;

		m_uniform_name_buffer = DE_NULL;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Generates double-precision values for all uniforms defined for all program objects
 *  used by the test.
 *
 *  This function DOES NOT use any GL API. It only calculates & stores the values
 *  in internal storage for further usage.
 */
void GPUShaderFP64Test4::generateUniformValues()
{
	_stage_data*	   stages[] = { &m_data_cs, &m_data_fs, &m_data_gs, &m_data_tc, &m_data_te, &m_data_vs };
	const unsigned int n_stages = sizeof(stages) / sizeof(stages[0]);

	for (unsigned int n_stage = 0; n_stage < n_stages; ++n_stage)
	{
		_stage_data* stage_ptr = stages[n_stage];

		/* Iterate through all uniform components and assign them double values */
		double* double_ptrs[] = {
			&stage_ptr->uniform_structure_arrays[0].uniform_double,
			stage_ptr->uniform_structure_arrays[0].uniform_double_arr + 0,
			stage_ptr->uniform_structure_arrays[0].uniform_double_arr + 1,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec2 + 0,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec2 + 1,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec2 + 2,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec2 + 3,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec3 + 0,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec3 + 1,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec3 + 2,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec3 + 3,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec3 + 4,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec3 + 5,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec4 + 0,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec4 + 1,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec4 + 2,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec4 + 3,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec4 + 4,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec4 + 5,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec4 + 6,
			stage_ptr->uniform_structure_arrays[0].uniform_dvec4 + 7,
			&stage_ptr->uniform_structure_arrays[1].uniform_double,
			stage_ptr->uniform_structure_arrays[1].uniform_double_arr + 0,
			stage_ptr->uniform_structure_arrays[1].uniform_double_arr + 1,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec2 + 0,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec2 + 1,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec2 + 2,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec2 + 3,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec3 + 0,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec3 + 1,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec3 + 2,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec3 + 3,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec3 + 4,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec3 + 5,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec4 + 0,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec4 + 1,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec4 + 2,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec4 + 3,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec4 + 4,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec4 + 5,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec4 + 6,
			stage_ptr->uniform_structure_arrays[1].uniform_dvec4 + 7,
			&stage_ptr->uniforms.uniform_double,
			stage_ptr->uniforms.uniform_double_arr + 0,
			stage_ptr->uniforms.uniform_double_arr + 1,
			stage_ptr->uniforms.uniform_dvec2 + 0,
			stage_ptr->uniforms.uniform_dvec2 + 1,
			stage_ptr->uniforms.uniform_dvec2_arr + 0,
			stage_ptr->uniforms.uniform_dvec2_arr + 1,
			stage_ptr->uniforms.uniform_dvec2_arr + 2,
			stage_ptr->uniforms.uniform_dvec2_arr + 3,
			stage_ptr->uniforms.uniform_dvec3 + 0,
			stage_ptr->uniforms.uniform_dvec3 + 1,
			stage_ptr->uniforms.uniform_dvec3 + 2,
			stage_ptr->uniforms.uniform_dvec3_arr + 0,
			stage_ptr->uniforms.uniform_dvec3_arr + 1,
			stage_ptr->uniforms.uniform_dvec3_arr + 2,
			stage_ptr->uniforms.uniform_dvec3_arr + 3,
			stage_ptr->uniforms.uniform_dvec3_arr + 4,
			stage_ptr->uniforms.uniform_dvec3_arr + 5,
			stage_ptr->uniforms.uniform_dvec4 + 0,
			stage_ptr->uniforms.uniform_dvec4 + 1,
			stage_ptr->uniforms.uniform_dvec4 + 2,
			stage_ptr->uniforms.uniform_dvec4 + 3,
			stage_ptr->uniforms.uniform_dvec4_arr + 0,
			stage_ptr->uniforms.uniform_dvec4_arr + 1,
			stage_ptr->uniforms.uniform_dvec4_arr + 2,
			stage_ptr->uniforms.uniform_dvec4_arr + 3,
			stage_ptr->uniforms.uniform_dvec4_arr + 4,
			stage_ptr->uniforms.uniform_dvec4_arr + 5,
			stage_ptr->uniforms.uniform_dvec4_arr + 6,
			stage_ptr->uniforms.uniform_dvec4_arr + 7,
		};
		const unsigned int n_double_ptrs = sizeof(double_ptrs) / sizeof(double_ptrs[0]);

		for (unsigned int n_double_ptr = 0; n_double_ptr < n_double_ptrs; ++n_double_ptr)
		{
			double* double_ptr = double_ptrs[n_double_ptr];

			/* Generate the value. Use magic numbers to generate a set of double-precision
			 * floating-point numbers.
			 */
			static int seed = 16762362;

			*double_ptr = ((double)(seed % 1732)) / 125.7 * (((seed % 2) == 0) ? 1 : -1);

			seed += 751;
		} /* for (all pointers to double variables) */
	}	 /* for (all stages) */
}

/** Initializes all program & shader objects required to run the test. The function also
 *  retrieves locations of all uniforms defined by both program objects.
 **/
void GPUShaderFP64Test4::initProgramObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program & shader objects */

	/* Compute shader support and GL 4.2 required */
	if ((true == m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader")) &&
		(true == Utils::isGLVersionAtLeast(gl, 4 /* major */, 2 /* minor */)))
	{
		m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	}

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
	m_te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* m_cs_id is initialized only if compute shaders are supported */
	if (0 != m_cs_id)
	{
		m_po_cs_id = gl.createProgram();
	}

	m_po_noncs_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call(s) failed.");

	/* Configure compute shader body */
	const char* cs_body = "#version 420\n"
						  "#extension GL_ARB_compute_shader          : require\n"
						  "\n"
						  "layout(local_size_x = 2, local_size_y = 2, local_size_z = 2) in;\n"
						  "\n"
						  "layout(rgba32f) uniform image2D testImage;\n"
						  "\n"
						  "uniform double cs_double;\n"
						  "uniform dvec2  cs_dvec2;\n"
						  "uniform dvec3  cs_dvec3;\n"
						  "uniform dvec4  cs_dvec4;\n"
						  "uniform double cs_double_arr[2];\n"
						  "uniform dvec2  cs_dvec2_arr [2];\n"
						  "uniform dvec3  cs_dvec3_arr [2];\n"
						  "uniform dvec4  cs_dvec4_arr [2];\n"
						  "\n"
						  "uniform struct cs_struct\n"
						  "{\n"
						  "    double struct_double;\n"
						  "    dvec2  struct_dvec2;\n"
						  "    dvec3  struct_dvec3;\n"
						  "    dvec4  struct_dvec4;\n"
						  "} cs_array[2];\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    double tmp = cs_double                  * cs_dvec2.x                 * cs_dvec3.y       "
						  "          * cs_dvec4.z                 *\n"
						  "                 cs_double_arr[0]           * cs_dvec2_arr[0].x          * "
						  "cs_dvec3_arr[0].z          * cs_dvec4_arr[0].w          *\n"
						  "                 cs_double_arr[1]           * cs_dvec2_arr[1].x          * "
						  "cs_dvec3_arr[1].z          * cs_dvec4_arr[1].w          *\n"
						  "                 cs_array[0].struct_double  * cs_array[0].struct_dvec2.y * "
						  "cs_array[0].struct_dvec3.z * cs_array[0].struct_dvec4.w *\n"
						  "                 cs_array[1].struct_double  * cs_array[1].struct_dvec2.y * "
						  "cs_array[1].struct_dvec3.z * cs_array[1].struct_dvec4.w;\n"
						  "\n"
						  "    imageStore(testImage, ivec2(0, 0), (tmp > 1.0) ? vec4(1.0) : vec4(0.0) );\n"
						  "}\n";

	/* m_cs_id is initialized only if compute shaders are supported */
	if (0 != m_cs_id)
	{
		gl.shaderSource(m_cs_id, 1 /* count */, &cs_body, DE_NULL /* length */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");
	}

	/* Configure vertex shader body */
	const char* vs_body = "#version 400\n"
						  "\n"
						  "uniform double vs_double;\n"
						  "uniform dvec2  vs_dvec2;\n"
						  "uniform dvec3  vs_dvec3;\n"
						  "uniform dvec4  vs_dvec4;\n"
						  "uniform double vs_double_arr[2];\n"
						  "uniform dvec2  vs_dvec2_arr [2];\n"
						  "uniform dvec3  vs_dvec3_arr [2];\n"
						  "uniform dvec4  vs_dvec4_arr [2];\n"
						  "\n"
						  "uniform struct vs_struct\n"
						  "{\n"
						  "    double struct_double;\n"
						  "    dvec2  struct_dvec2;\n"
						  "    dvec3  struct_dvec3;\n"
						  "    dvec4  struct_dvec4;\n"
						  "} vs_array[2];\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    if (vs_double                 * vs_dvec2.x                 * vs_dvec3.x                 "
						  "* vs_dvec4.x                 *\n"
						  "        vs_double_arr[0]          * vs_dvec2_arr[0].x          * vs_dvec3_arr[0].x          "
						  "* vs_dvec4_arr[0].x          *\n"
						  "        vs_double_arr[1]          * vs_dvec2_arr[1].x          * vs_dvec3_arr[1].x          "
						  "* vs_dvec4_arr[1].x          *\n"
						  "        vs_array[0].struct_double * vs_array[0].struct_dvec2.x * vs_array[0].struct_dvec3.x "
						  "* vs_array[0].struct_dvec4.x *\n"
						  "        vs_array[1].struct_double * vs_array[1].struct_dvec2.x * vs_array[1].struct_dvec3.x "
						  "* vs_array[1].struct_dvec4.x > 1.0)\n"
						  "    {\n"
						  "        gl_Position = vec4(0);\n"
						  "    }\n"
						  "    else\n"
						  "    {\n"
						  "        gl_Position = vec4(1);\n"
						  "    }\n"
						  "}\n";

	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Configure tessellation control shader body */
	const char* tc_body = "#version 400\n"
						  "\n"
						  "uniform double tc_double;\n"
						  "uniform dvec2  tc_dvec2;\n"
						  "uniform dvec3  tc_dvec3;\n"
						  "uniform dvec4  tc_dvec4;\n"
						  "uniform double tc_double_arr[2];\n"
						  "uniform dvec2  tc_dvec2_arr [2];\n"
						  "uniform dvec3  tc_dvec3_arr [2];\n"
						  "uniform dvec4  tc_dvec4_arr [2];\n"
						  "\n"
						  "uniform struct tc_struct\n"
						  "{\n"
						  "    double struct_double;\n"
						  "    dvec2  struct_dvec2;\n"
						  "    dvec3  struct_dvec3;\n"
						  "    dvec4  struct_dvec4;\n"
						  "} tc_array[2];\n"
						  "\n"
						  "layout(vertices = 4) out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_TessLevelOuter[0] = (tc_double        > 1.0) ? 2.0 : 3.0;\n"
						  "    gl_TessLevelOuter[1] = (tc_dvec2.x       > 1.0) ? 3.0 : 4.0;\n"
						  "    gl_TessLevelOuter[2] = (tc_dvec3.x       > 1.0) ? 4.0 : 5.0;\n"
						  "    gl_TessLevelOuter[3] = (tc_dvec4.x       > 1.0) ? 5.0 : 6.0;\n"
						  "    gl_TessLevelInner[0] = (tc_double_arr[0] > 1.0) ? 6.0 : 7.0;\n"
						  "    gl_TessLevelInner[1] = (tc_double_arr[1] > 1.0) ? 7.0 : 8.0;\n"
						  "\n"
						  "    if (tc_dvec2_arr[0].y          * tc_dvec2_arr[1].y          *\n"
						  "        tc_dvec3_arr[0].z          * tc_dvec3_arr[1].z          *\n"
						  "        tc_dvec4_arr[0].z          * tc_dvec4_arr[1].z          *\n"
						  "        tc_array[0].struct_double  * tc_array[0].struct_dvec2.x * \n"
						  "        tc_array[0].struct_dvec3.y * tc_array[0].struct_dvec4.z * \n"
						  "        tc_array[1].struct_double  * tc_array[1].struct_dvec2.x * \n"
						  "        tc_array[1].struct_dvec3.y * tc_array[1].struct_dvec4.z > 0.0)\n"
						  "    {\n"
						  "        gl_TessLevelInner[1] = 3.0;\n"
						  "    }\n"
						  "}\n";

	gl.shaderSource(m_tc_id, 1 /* count */, &tc_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Configure tessellation evaluation shader body */
	const char* te_body = "#version 400\n"
						  "\n"
						  "uniform double te_double;\n"
						  "uniform dvec2  te_dvec2;\n"
						  "uniform dvec3  te_dvec3;\n"
						  "uniform dvec4  te_dvec4;\n"
						  "uniform double te_double_arr[2];\n"
						  "uniform dvec2  te_dvec2_arr [2];\n"
						  "uniform dvec3  te_dvec3_arr [2];\n"
						  "uniform dvec4  te_dvec4_arr [2];\n"
						  "\n"
						  "uniform struct te_struct\n"
						  "{\n"
						  "    double struct_double;\n"
						  "    dvec2  struct_dvec2;\n"
						  "    dvec3  struct_dvec3;\n"
						  "    dvec4  struct_dvec4;\n"
						  "} te_array[2];\n"
						  "\n"
						  "layout(triangles) in;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    if (te_double                 * te_dvec2.x                 * te_dvec3.x                 "
						  "* te_dvec4.x                 *\n"
						  "        te_double_arr[0]          * te_dvec2_arr[0].x          * te_dvec3_arr[0].x          "
						  "* te_dvec4_arr[0].x          *\n"
						  "        te_double_arr[1]          * te_dvec2_arr[1].x          * te_dvec3_arr[1].x          "
						  "* te_dvec4_arr[1].x          *\n"
						  "        te_array[0].struct_double * te_array[0].struct_dvec2.x * te_array[0].struct_dvec3.x "
						  "* te_array[0].struct_dvec4.x *\n"
						  "        te_array[1].struct_double * te_array[1].struct_dvec2.x * te_array[1].struct_dvec3.x "
						  "* te_array[1].struct_dvec4.x > 1.0)\n"
						  "    {\n"
						  "        gl_Position = gl_in[0].gl_Position;\n"
						  "    }\n"
						  "    else\n"
						  "    {\n"
						  "        gl_Position = gl_in[0].gl_Position + vec4(1);\n"
						  "    }\n"
						  "}\n";

	gl.shaderSource(m_te_id, 1 /* count */, &te_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Configure geometry shader body */
	const char* gs_body = "#version 400\n"
						  "\n"
						  "uniform double gs_double;\n"
						  "uniform dvec2  gs_dvec2;\n"
						  "uniform dvec3  gs_dvec3;\n"
						  "uniform dvec4  gs_dvec4;\n"
						  "uniform double gs_double_arr[2];\n"
						  "uniform dvec2  gs_dvec2_arr [2];\n"
						  "uniform dvec3  gs_dvec3_arr [2];\n"
						  "uniform dvec4  gs_dvec4_arr [2];\n"
						  "\n"
						  "uniform struct gs_struct\n"
						  "{\n"
						  "    double struct_double;\n"
						  "    dvec2  struct_dvec2;\n"
						  "    dvec3  struct_dvec3;\n"
						  "    dvec4  struct_dvec4;\n"
						  "} gs_array[2];\n"
						  "\n"
						  "layout (points)                   in;\n"
						  "layout (points, max_vertices = 1) out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    if (gs_double                 * gs_dvec2.x                 * gs_dvec3.x                 "
						  "* gs_dvec4.x        *\n"
						  "        gs_double_arr[0]          * gs_dvec2_arr[0].x          * gs_dvec3_arr[0].x          "
						  "* gs_dvec4_arr[0].x *\n"
						  "        gs_double_arr[1]          * gs_dvec2_arr[1].x          * gs_dvec3_arr[1].x          "
						  "* gs_dvec4_arr[1].x *\n"
						  "        gs_array[0].struct_double * gs_array[0].struct_dvec2.x * gs_array[0].struct_dvec3.x "
						  "* gs_array[0].struct_dvec4.x *\n"
						  "        gs_array[1].struct_double * gs_array[1].struct_dvec2.x * gs_array[1].struct_dvec3.x "
						  "* gs_array[1].struct_dvec4.x > 1.0)\n"
						  "    {\n"
						  "        gl_Position = gl_in[0].gl_Position;\n"
						  "    }\n"
						  "    else\n"
						  "    {\n"
						  "        gl_Position = gl_in[0].gl_Position + vec4(1);\n"
						  "    }\n"
						  "\n"
						  "    EmitVertex();\n"
						  "}\n";

	gl.shaderSource(m_gs_id, 1 /* count */, &gs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Configure fragment shader body */
	const char* fs_body = "#version 400\n"
						  "\n"
						  "uniform double fs_double;\n"
						  "uniform dvec2  fs_dvec2;\n"
						  "uniform dvec3  fs_dvec3;\n"
						  "uniform dvec4  fs_dvec4;\n"
						  "uniform double fs_double_arr[2];\n"
						  "uniform dvec2  fs_dvec2_arr [2];\n"
						  "uniform dvec3  fs_dvec3_arr [2];\n"
						  "uniform dvec4  fs_dvec4_arr [2];\n"
						  "\n"
						  "uniform struct fs_struct\n"
						  "{\n"
						  "    double struct_double;\n"
						  "    dvec2  struct_dvec2;\n"
						  "    dvec3  struct_dvec3;\n"
						  "    dvec4  struct_dvec4;\n"
						  "} fs_array[2];\n"
						  "\n"
						  "out vec4 result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    if (fs_double                 * fs_dvec2.x                 * fs_dvec3.x                 "
						  "* fs_dvec4.x        *\n"
						  "        fs_double_arr[0]          * fs_dvec2_arr[0].x          * fs_dvec3_arr[0].x          "
						  "* fs_dvec4_arr[0].x *\n"
						  "        fs_double_arr[1]          * fs_dvec2_arr[1].x          * fs_dvec3_arr[1].x          "
						  "* fs_dvec4_arr[1].x *\n"
						  "        fs_array[0].struct_double * fs_array[0].struct_dvec2.x * fs_array[0].struct_dvec3.x "
						  "* fs_array[0].struct_dvec4.x *\n"
						  "        fs_array[1].struct_double * fs_array[1].struct_dvec2.x * fs_array[1].struct_dvec3.x "
						  "* fs_array[1].struct_dvec4.x > 1.0)\n"
						  "    {\n"
						  "        result = vec4(0.0);\n"
						  "    }\n"
						  "    else\n"
						  "    {\n"
						  "        result = vec4(1.0);\n"
						  "    }\n"
						  "}\n";

	gl.shaderSource(m_fs_id, 1 /* count */, &fs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

	/* Compile the shaders */
	const glw::GLuint  shaders[] = { m_cs_id, m_fs_id, m_gs_id, m_tc_id, m_te_id, m_vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLint  compile_status = GL_FALSE;
		glw::GLuint so_id		   = shaders[n_shader];

		/* Skip compute shader if not supported */
		if (0 == so_id)
		{
			continue;
		}

		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

		if (compile_status != GL_TRUE)
		{
			TCU_FAIL("Shader compilation failed");
		}

		if (so_id == m_cs_id)
		{
			gl.attachShader(m_po_cs_id, so_id);
		}
		else
		{
			gl.attachShader(m_po_noncs_id, so_id);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");
	} /* for (all shaders) */

	/* Link the program */
	const glw::GLuint  programs[]  = { m_po_cs_id, m_po_noncs_id };
	const unsigned int n_programs  = sizeof(programs) / sizeof(programs[0]);
	glw::GLint		   link_status = GL_FALSE;

	for (unsigned int n_program = 0; n_program < n_programs; ++n_program)
	{
		glw::GLuint po_id = programs[n_program];

		/* Skip compute shader program if not supported */
		if (0 == po_id)
		{
			continue;
		}

		gl.linkProgram(po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

		gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed");

		if (link_status != GL_TRUE)
		{
			TCU_FAIL("Program linking failed");
		}
	} /* for (both program objects) */

	/* Retrieve uniform locations */
	_stage_data*			  cs_stage_data[]		= { &m_data_cs };
	static const char*		  cs_uniform_prefixes[] = { "cs_" };
	static const unsigned int n_cs_uniform_prefixes = sizeof(cs_uniform_prefixes) / sizeof(cs_uniform_prefixes[0]);

	_stage_data*			  noncs_stage_data[]	   = { &m_data_fs, &m_data_gs, &m_data_tc, &m_data_te, &m_data_vs };
	static const char*		  noncs_uniform_prefixes[] = { "fs_", "gs_", "tc_", "te_", "vs_" };
	static const unsigned int n_noncs_uniform_prefixes =
		sizeof(noncs_uniform_prefixes) / sizeof(noncs_uniform_prefixes[0]);

	for (unsigned int n_program = 0; n_program < n_programs; ++n_program)
	{
		unsigned int  n_uniform_prefixes = DE_NULL;
		glw::GLuint   po_id				 = programs[n_program];
		_stage_data** stages_data		 = DE_NULL;
		const char**  uniform_prefixes   = DE_NULL;

		if (n_program == 0)
		{
			stages_data		   = cs_stage_data;
			uniform_prefixes   = cs_uniform_prefixes;
			n_uniform_prefixes = n_cs_uniform_prefixes;
		}
		else
		{
			stages_data		   = noncs_stage_data;
			uniform_prefixes   = noncs_uniform_prefixes;
			n_uniform_prefixes = n_noncs_uniform_prefixes;
		}

		/* Skip compute shader program if not supported */
		if (0 == po_id)
		{
			continue;
		}

		/* Uniform names used by the test program consist of a prefix (different for each
		 * shader stage) and a common part.
		 */
		for (unsigned int n_uniform_prefix = 0; n_uniform_prefix < n_uniform_prefixes; ++n_uniform_prefix)
		{
			_stage_data* stage_data				  = stages_data[n_uniform_prefix];
			std::string  uniform_prefix			  = std::string(uniform_prefixes[n_uniform_prefix]);
			std::string  uniform_double_name	  = uniform_prefix + "double";
			std::string  uniform_double_arr0_name = uniform_prefix + "double_arr[0]";
			std::string  uniform_double_arr1_name = uniform_prefix + "double_arr[1]";
			std::string  uniform_dvec2_name		  = uniform_prefix + "dvec2";
			std::string  uniform_dvec2_arr0_name  = uniform_prefix + "dvec2_arr[0]";
			std::string  uniform_dvec2_arr1_name  = uniform_prefix + "dvec2_arr[1]";
			std::string  uniform_dvec3_name		  = uniform_prefix + "dvec3";
			std::string  uniform_dvec3_arr0_name  = uniform_prefix + "dvec3_arr[0]";
			std::string  uniform_dvec3_arr1_name  = uniform_prefix + "dvec3_arr[1]";
			std::string  uniform_dvec4_name		  = uniform_prefix + "dvec4";
			std::string  uniform_dvec4_arr0_name  = uniform_prefix + "dvec4_arr[0]";
			std::string  uniform_dvec4_arr1_name  = uniform_prefix + "dvec4_arr[1]";
			std::string  uniform_arr0_double_name = uniform_prefix + "array[0].struct_double";
			std::string  uniform_arr0_dvec2_name  = uniform_prefix + "array[0].struct_dvec2";
			std::string  uniform_arr0_dvec3_name  = uniform_prefix + "array[0].struct_dvec3";
			std::string  uniform_arr0_dvec4_name  = uniform_prefix + "array[0].struct_dvec4";
			std::string  uniform_arr1_double_name = uniform_prefix + "array[1].struct_double";
			std::string  uniform_arr1_dvec2_name  = uniform_prefix + "array[1].struct_dvec2";
			std::string  uniform_arr1_dvec3_name  = uniform_prefix + "array[1].struct_dvec3";
			std::string  uniform_arr1_dvec4_name  = uniform_prefix + "array[1].struct_dvec4";

			/* Retrieve uniform locations */
			stage_data->uniforms.uniform_location_double = gl.getUniformLocation(po_id, uniform_double_name.c_str());
			stage_data->uniforms.uniform_location_double_arr[0] =
				gl.getUniformLocation(po_id, uniform_double_arr0_name.c_str());
			stage_data->uniforms.uniform_location_double_arr[1] =
				gl.getUniformLocation(po_id, uniform_double_arr1_name.c_str());
			stage_data->uniforms.uniform_location_dvec2 = gl.getUniformLocation(po_id, uniform_dvec2_name.c_str());
			stage_data->uniforms.uniform_location_dvec2_arr[0] =
				gl.getUniformLocation(po_id, uniform_dvec2_arr0_name.c_str());
			stage_data->uniforms.uniform_location_dvec2_arr[1] =
				gl.getUniformLocation(po_id, uniform_dvec2_arr1_name.c_str());
			stage_data->uniforms.uniform_location_dvec3 = gl.getUniformLocation(po_id, uniform_dvec3_name.c_str());
			stage_data->uniforms.uniform_location_dvec3_arr[0] =
				gl.getUniformLocation(po_id, uniform_dvec3_arr0_name.c_str());
			stage_data->uniforms.uniform_location_dvec3_arr[1] =
				gl.getUniformLocation(po_id, uniform_dvec3_arr1_name.c_str());
			stage_data->uniforms.uniform_location_dvec4 = gl.getUniformLocation(po_id, uniform_dvec4_name.c_str());
			stage_data->uniforms.uniform_location_dvec4_arr[0] =
				gl.getUniformLocation(po_id, uniform_dvec4_arr0_name.c_str());
			stage_data->uniforms.uniform_location_dvec4_arr[1] =
				gl.getUniformLocation(po_id, uniform_dvec4_arr1_name.c_str());
			stage_data->uniform_structure_arrays[0].uniform_location_double =
				gl.getUniformLocation(po_id, uniform_arr0_double_name.c_str());
			stage_data->uniform_structure_arrays[0].uniform_location_dvec2 =
				gl.getUniformLocation(po_id, uniform_arr0_dvec2_name.c_str());
			stage_data->uniform_structure_arrays[0].uniform_location_dvec3 =
				gl.getUniformLocation(po_id, uniform_arr0_dvec3_name.c_str());
			stage_data->uniform_structure_arrays[0].uniform_location_dvec4 =
				gl.getUniformLocation(po_id, uniform_arr0_dvec4_name.c_str());
			stage_data->uniform_structure_arrays[1].uniform_location_double =
				gl.getUniformLocation(po_id, uniform_arr1_double_name.c_str());
			stage_data->uniform_structure_arrays[1].uniform_location_dvec2 =
				gl.getUniformLocation(po_id, uniform_arr1_dvec2_name.c_str());
			stage_data->uniform_structure_arrays[1].uniform_location_dvec3 =
				gl.getUniformLocation(po_id, uniform_arr1_dvec3_name.c_str());
			stage_data->uniform_structure_arrays[1].uniform_location_dvec4 =
				gl.getUniformLocation(po_id, uniform_arr1_dvec4_name.c_str());
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call(s) failed.");

			if (stage_data->uniforms.uniform_location_double == -1 ||
				stage_data->uniforms.uniform_location_double_arr[0] == -1 ||
				stage_data->uniforms.uniform_location_double_arr[1] == -1 ||
				stage_data->uniforms.uniform_location_dvec2 == -1 ||
				stage_data->uniforms.uniform_location_dvec2_arr[0] == -1 ||
				stage_data->uniforms.uniform_location_dvec2_arr[1] == -1 ||
				stage_data->uniforms.uniform_location_dvec3 == -1 ||
				stage_data->uniforms.uniform_location_dvec3_arr[0] == -1 ||
				stage_data->uniforms.uniform_location_dvec3_arr[1] == -1 ||
				stage_data->uniforms.uniform_location_dvec4 == -1 ||
				stage_data->uniforms.uniform_location_dvec4_arr[0] == -1 ||
				stage_data->uniforms.uniform_location_dvec4_arr[1] == -1 ||
				stage_data->uniform_structure_arrays[0].uniform_location_double == -1 ||
				stage_data->uniform_structure_arrays[0].uniform_location_dvec2 == -1 ||
				stage_data->uniform_structure_arrays[0].uniform_location_dvec3 == -1 ||
				stage_data->uniform_structure_arrays[0].uniform_location_dvec4 == -1 ||
				stage_data->uniform_structure_arrays[1].uniform_location_double == -1 ||
				stage_data->uniform_structure_arrays[1].uniform_location_dvec2 == -1 ||
				stage_data->uniform_structure_arrays[1].uniform_location_dvec3 == -1 ||
				stage_data->uniform_structure_arrays[1].uniform_location_dvec4 == -1)
			{
				TCU_FAIL("At least one uniform is considered inactive which is invalid.");
			}

			/* Make sure locations of subsequent items in array uniforms are correct */
			if (stage_data->uniforms.uniform_location_double_arr[1] !=
					(stage_data->uniforms.uniform_location_double_arr[0] + 1) ||
				stage_data->uniforms.uniform_location_dvec2_arr[1] !=
					(stage_data->uniforms.uniform_location_dvec2_arr[0] + 1) ||
				stage_data->uniforms.uniform_location_dvec3_arr[1] !=
					(stage_data->uniforms.uniform_location_dvec3_arr[0] + 1) ||
				stage_data->uniforms.uniform_location_dvec4_arr[1] !=
					(stage_data->uniforms.uniform_location_dvec4_arr[0] + 1))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Uniform locations:"
															   " double_arr[0]:"
								   << stage_data->uniforms.uniform_location_double_arr[0]
								   << " double_arr[1]:" << stage_data->uniforms.uniform_location_double_arr[1]
								   << " dvec2_arr[0]:" << stage_data->uniforms.uniform_location_dvec2_arr[0]
								   << " dvec2_arr[1]:" << stage_data->uniforms.uniform_location_dvec2_arr[1]
								   << " dvec3_arr[0]:" << stage_data->uniforms.uniform_location_dvec3_arr[0]
								   << " dvec3_arr[1]:" << stage_data->uniforms.uniform_location_dvec3_arr[1]
								   << " dvec4_arr[0]:" << stage_data->uniforms.uniform_location_dvec4_arr[0]
								   << " dvec4_arr[1]:" << stage_data->uniforms.uniform_location_dvec4_arr[1]
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Double-precision uniform array item locations are invalid.");
			}
		} /* for (all uniform prefixes) */
	}	 /* for (both program objects) */
}

/** Initializes all objects required to run the test. */
void GPUShaderFP64Test4::initTest()
{
	initProgramObjects();

	generateUniformValues();
	initUniformValues();
}

/** Assigns values generated by generateUniformValues() to uniforms defined by
 *  both program objects.
 **/
void GPUShaderFP64Test4::initUniformValues()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Iterate through all programs */
	_stage_data*	   cs_stages[]	= { &m_data_cs };
	_stage_data*	   noncs_stages[] = { &m_data_fs, &m_data_gs, &m_data_tc, &m_data_te, &m_data_vs };
	const unsigned int n_cs_stages	= sizeof(cs_stages) / sizeof(cs_stages[0]);
	const unsigned int n_noncs_stages = sizeof(noncs_stages) / sizeof(noncs_stages[0]);

	const glw::GLuint  programs[] = { m_po_cs_id, m_po_noncs_id };
	const unsigned int n_programs = sizeof(programs) / sizeof(programs[0]);

	for (unsigned int n_program = 0; n_program < n_programs; ++n_program)
	{
		glw::GLuint   po_id		 = programs[n_program];
		unsigned int  n_stages   = 0;
		_stage_data** stage_data = DE_NULL;

		if (po_id == m_po_cs_id)
		{
			n_stages   = n_cs_stages;
			stage_data = cs_stages;
		}
		else
		{
			n_stages   = n_noncs_stages;
			stage_data = noncs_stages;
		}

		/* Skip compute shader program if not supported */
		if (0 == po_id)
		{
			continue;
		}

		gl.useProgram(po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

		for (unsigned int n_stage = 0; n_stage < n_stages; ++n_stage)
		{
			/* Iterate through all uniforms */
			_stage_data* stage_ptr = stage_data[n_stage];

			gl.uniform1d(stage_ptr->uniforms.uniform_location_double, stage_ptr->uniforms.uniform_double);
			gl.uniform1d(stage_ptr->uniforms.uniform_location_double_arr[0], stage_ptr->uniforms.uniform_double_arr[0]);
			gl.uniform1d(stage_ptr->uniforms.uniform_location_double_arr[1], stage_ptr->uniforms.uniform_double_arr[1]);
			gl.uniform1d(stage_ptr->uniform_structure_arrays[0].uniform_location_double,
						 stage_ptr->uniform_structure_arrays[0].uniform_double);
			gl.uniform1d(stage_ptr->uniform_structure_arrays[1].uniform_location_double,
						 stage_ptr->uniform_structure_arrays[1].uniform_double);
			GLU_EXPECT_NO_ERROR(gl.getError(), "gluniform1d() call(s) failed.");

			gl.uniform2dv(stage_ptr->uniforms.uniform_location_dvec2, 1 /* count */, stage_ptr->uniforms.uniform_dvec2);
			gl.uniform2dv(stage_ptr->uniforms.uniform_location_dvec2_arr[0], 1 /* count */,
						  stage_ptr->uniforms.uniform_dvec2_arr + 0);
			gl.uniform2dv(stage_ptr->uniforms.uniform_location_dvec2_arr[1], 1 /* count */,
						  stage_ptr->uniforms.uniform_dvec2_arr + 2);
			gl.uniform2dv(stage_ptr->uniform_structure_arrays[0].uniform_location_dvec2, 1 /* count */,
						  stage_ptr->uniform_structure_arrays[0].uniform_dvec2);
			gl.uniform2dv(stage_ptr->uniform_structure_arrays[1].uniform_location_dvec2, 1 /* count */,
						  stage_ptr->uniform_structure_arrays[1].uniform_dvec2);
			GLU_EXPECT_NO_ERROR(gl.getError(), "gluniform2dv() call(s) failed.");

			gl.uniform3dv(stage_ptr->uniforms.uniform_location_dvec3, 1 /* count */, stage_ptr->uniforms.uniform_dvec3);
			gl.uniform3dv(stage_ptr->uniforms.uniform_location_dvec3_arr[0], 1 /* count */,
						  stage_ptr->uniforms.uniform_dvec3_arr + 0);
			gl.uniform3dv(stage_ptr->uniforms.uniform_location_dvec3_arr[1], 1 /* count */,
						  stage_ptr->uniforms.uniform_dvec3_arr + 3);
			gl.uniform3dv(stage_ptr->uniform_structure_arrays[0].uniform_location_dvec3, 1 /* count */,
						  stage_ptr->uniform_structure_arrays[0].uniform_dvec3);
			gl.uniform3dv(stage_ptr->uniform_structure_arrays[1].uniform_location_dvec3, 1 /* count */,
						  stage_ptr->uniform_structure_arrays[1].uniform_dvec3);
			GLU_EXPECT_NO_ERROR(gl.getError(), "gluniform3dv() call(s) failed.");

			gl.uniform4dv(stage_ptr->uniforms.uniform_location_dvec4, 1 /* count */, stage_ptr->uniforms.uniform_dvec4);
			gl.uniform4dv(stage_ptr->uniforms.uniform_location_dvec4_arr[0], 1 /* count */,
						  stage_ptr->uniforms.uniform_dvec4_arr + 0);
			gl.uniform4dv(stage_ptr->uniforms.uniform_location_dvec4_arr[1], 1 /* count */,
						  stage_ptr->uniforms.uniform_dvec4_arr + 4);
			gl.uniform4dv(stage_ptr->uniform_structure_arrays[0].uniform_location_dvec4, 1 /* count */,
						  stage_ptr->uniform_structure_arrays[0].uniform_dvec4);
			gl.uniform4dv(stage_ptr->uniform_structure_arrays[1].uniform_location_dvec4, 1 /* count */,
						  stage_ptr->uniform_structure_arrays[1].uniform_dvec4);
			GLU_EXPECT_NO_ERROR(gl.getError(), "gluniform4dv() call(s) failed.");
		} /* for (all shader stages) */
	}	 /* for (both program objects) */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GPUShaderFP64Test4::iterate()
{
	/* This test does not verify GL_ARB_gpu_shader_fp64 support on purpose */

	/* Initialize all objects required to run the test */
	initTest();

	/* Verify the implementation reports correct values for all stages we've configured */
	m_has_test_passed &= verifyUniformValues();

	/* Is this also the case when "program interface query" mechanism is used? */
	if (m_context.getContextInfo().isExtensionSupported("GL_ARB_program_interface_query"))
	{
		m_has_test_passed &= verifyProgramInterfaceQuerySupport();
	}

	/* We're done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies that:
 *
 *  a) glGetProgramResourceIndex()
 *  b) glGetProgramResourceiv()
 *  c) glGetProgramResourceName()
 *
 *  functions return correct values for double-precision uniforms.
 *
 *  @return true if the verification was passed, false otherwise.
 */
bool GPUShaderFP64Test4::verifyProgramInterfaceQuerySupport()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Iterate through all programs */
	const char*		   cs_prefixes[]	= { "cs_" };
	_stage_data*	   cs_stages[]		= { &m_data_cs };
	const char*		   noncs_prefixes[] = { "fs_", "gs_", "tc_", "te_", "vs_" };
	_stage_data*	   noncs_stages[]   = { &m_data_fs, &m_data_gs, &m_data_tc, &m_data_te, &m_data_vs };
	const unsigned int n_cs_stages		= sizeof(cs_stages) / sizeof(cs_stages[0]);
	const unsigned int n_noncs_stages   = sizeof(noncs_stages) / sizeof(noncs_stages[0]);

	const glw::GLuint  programs[] = { m_po_cs_id, m_po_noncs_id };
	const unsigned int n_programs = sizeof(programs) / sizeof(programs[0]);

	for (unsigned int n_program = 0; n_program < n_programs; ++n_program)
	{
		glw::GLuint   po_id			 = programs[n_program];
		unsigned int  n_stages		 = 0;
		const char**  stage_prefixes = DE_NULL;
		_stage_data** stage_data	 = DE_NULL;

		if (po_id == m_po_cs_id)
		{
			n_stages	   = n_cs_stages;
			stage_data	 = cs_stages;
			stage_prefixes = cs_prefixes;
		}
		else
		{
			n_stages	   = n_noncs_stages;
			stage_data	 = noncs_stages;
			stage_prefixes = noncs_prefixes;
		}

		/* Skip compute shader program if not supported */
		if (0 == po_id)
		{
			continue;
		}

		/* Determine maximum uniform name length */
		glw::GLint max_uniform_name_length = 0;

		gl.getProgramInterfaceiv(po_id, GL_UNIFORM, GL_MAX_NAME_LENGTH, &max_uniform_name_length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInterfaceiv() call failed.");

		/* Allocate a buffer we will use to hold uniform names */
		m_uniform_name_buffer = new char[max_uniform_name_length];

		for (unsigned int n_stage = 0; n_stage < n_stages; ++n_stage)
		{
			/* Iterate through all uniforms */
			_stage_data* stage_ptr	= stage_data[n_stage];
			const char*  stage_prefix = stage_prefixes[n_stage];

			/* Construct an array that will be used to run the test in an automated manner */
			_program_interface_query_test_item uniforms[] = {
				/* array size */ /* name */ /* type */ /* location */
				{ 1, "double", GL_DOUBLE, stage_ptr->uniforms.uniform_location_double },
				{ 2, "double_arr[0]", GL_DOUBLE, stage_ptr->uniforms.uniform_location_double_arr[0] },
				{ 1, "dvec2", GL_DOUBLE_VEC2, stage_ptr->uniforms.uniform_location_dvec2 },
				{ 2, "dvec2_arr[0]", GL_DOUBLE_VEC2, stage_ptr->uniforms.uniform_location_dvec2_arr[0] },
				{ 1, "dvec3", GL_DOUBLE_VEC3, stage_ptr->uniforms.uniform_location_dvec3 },
				{ 2, "dvec3_arr[0]", GL_DOUBLE_VEC3, stage_ptr->uniforms.uniform_location_dvec3_arr[0] },
				{ 1, "dvec4", GL_DOUBLE_VEC4, stage_ptr->uniforms.uniform_location_dvec4 },
				{ 2, "dvec4_arr[0]", GL_DOUBLE_VEC4, stage_ptr->uniforms.uniform_location_dvec4_arr[0] },
				{ 1, "array[0].struct_double", GL_DOUBLE,
				  stage_ptr->uniform_structure_arrays->uniform_location_double },
				{ 1, "array[0].struct_dvec2", GL_DOUBLE_VEC2,
				  stage_ptr->uniform_structure_arrays->uniform_location_dvec2 },
				{ 1, "array[0].struct_dvec3", GL_DOUBLE_VEC3,
				  stage_ptr->uniform_structure_arrays->uniform_location_dvec3 },
				{ 1, "array[0].struct_dvec4", GL_DOUBLE_VEC4,
				  stage_ptr->uniform_structure_arrays->uniform_location_dvec4 },
				{ 1, "array[1].struct_double", GL_DOUBLE,
				  stage_ptr->uniform_structure_arrays->uniform_location_double },
				{ 1, "array[1].struct_dvec2", GL_DOUBLE_VEC2,
				  stage_ptr->uniform_structure_arrays->uniform_location_dvec2 },
				{ 1, "array[1].struct_dvec3", GL_DOUBLE_VEC3,
				  stage_ptr->uniform_structure_arrays->uniform_location_dvec3 },
				{ 1, "array[1].struct_dvec4", GL_DOUBLE_VEC4,
				  stage_ptr->uniform_structure_arrays->uniform_location_dvec4 },
			};
			const unsigned int n_uniforms = sizeof(uniforms) / sizeof(uniforms[0]);

			/* Prefix the names with stage-specific string */
			for (unsigned int n_uniform = 0; n_uniform < n_uniforms; ++n_uniform)
			{
				_program_interface_query_test_item& current_item = uniforms[n_uniform];

				current_item.name = std::string(stage_prefix) + current_item.name;
			} /* for (all uniform descriptors) */

			const glw::GLenum  properties[] = { GL_ARRAY_SIZE, GL_TYPE };
			const unsigned int n_properties = sizeof(properties) / sizeof(properties[0]);

			for (unsigned int n_uniform = 0; n_uniform < n_uniforms; ++n_uniform)
			{
				_program_interface_query_test_item& current_item		  = uniforms[n_uniform];
				glw::GLint							n_written_items		  = 0;
				glw::GLint							retrieved_array_size  = 0;
				glw::GLint							retrieved_name_length = 0;
				glw::GLenum							retrieved_type		  = GL_NONE;
				glw::GLint							temp_buffer[2]		  = { 0, GL_NONE };

				/* Retrieve index of the iteration-specific uniform */
				glw::GLuint resource_index = gl.getProgramResourceIndex(po_id, GL_UNIFORM, current_item.name.c_str());
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceIndex() call failed.");

				/* Make sure glGetProgramResourceName() returns correct values */
				memset(m_uniform_name_buffer, 0, max_uniform_name_length);

				gl.getProgramResourceName(po_id, GL_UNIFORM, /* interface */
										  resource_index, max_uniform_name_length, &retrieved_name_length,
										  m_uniform_name_buffer);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceName() call failed.");

				if (current_item.name.length() != (glw::GLuint)retrieved_name_length ||
					memcmp(m_uniform_name_buffer, current_item.name.c_str(), retrieved_name_length) != 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid uniform name was reported at index ["
									   << resource_index << "]"
															": expected:["
									   << current_item.name << "]"
															   ", reported:["
									   << m_uniform_name_buffer << "]" << tcu::TestLog::EndMessage;

					result = false;
					continue;
				}

				/* Make sure glGetProgramResourceiv() returns correct values for GL_TYPE and GL_ARRAY_SIZE queries */
				gl.getProgramResourceiv(po_id, GL_UNIFORM, /* interface */
										resource_index, n_properties, properties,
										sizeof(temp_buffer) / sizeof(temp_buffer[0]), &n_written_items, temp_buffer);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceiv() call failed.");

				if (n_written_items != n_properties)
				{
					TCU_FAIL("Invalid amount of items were reported by glGetProgramResourceiv() call.");
				}

				/* For clarity, copy the retrieved values to separate variables */
				retrieved_array_size = temp_buffer[0];
				retrieved_type		 = temp_buffer[1];

				/* Verify the values */
				if (retrieved_array_size != current_item.expected_array_size)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid array size reported for uniform ["
									   << current_item.name << "]"
									   << ": expected:[" << current_item.expected_array_size << "]"
																								", reported:["
									   << retrieved_array_size << "]" << tcu::TestLog::EndMessage;

					result = false;
				}

				if (retrieved_type != current_item.expected_type)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid type reported for uniform ["
									   << current_item.name << "]"
									   << ": expected:[" << current_item.expected_type << "]"
																						  ", reported:["
									   << retrieved_type << "]" << tcu::TestLog::EndMessage;

					result = false;
				}
			} /* for (all uniforms) */
		}	 /* for (all shader stages) */

		/* We're now OK to release the buffer we used to hold uniform names for
		 * the program */
		if (m_uniform_name_buffer != DE_NULL)
		{
			delete[] m_uniform_name_buffer;

			m_uniform_name_buffer = DE_NULL;
		}
	} /* for (both program objects) */

	return result;
}

/** Verifies glGetUniform*() calls return correct values assigned to
 *  double-precision uniforms.
 *
 *  @return true if all values reported by OpenGL were found to be correct,
 *          false otherwise.
 **/
bool GPUShaderFP64Test4::verifyUniformValues()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Iterate through all programs */
	_stage_data*	   cs_stages[]	= { &m_data_cs };
	_stage_data*	   noncs_stages[] = { &m_data_fs, &m_data_gs, &m_data_tc, &m_data_te, &m_data_vs };
	const unsigned int n_cs_stages	= sizeof(cs_stages) / sizeof(cs_stages[0]);
	const unsigned int n_noncs_stages = sizeof(noncs_stages) / sizeof(noncs_stages[0]);

	const glw::GLuint programs[] = {
		m_po_noncs_id, m_po_cs_id,
	};
	const unsigned int n_programs = sizeof(programs) / sizeof(programs[0]);

	/* Set up rounding for the tests */
	deSetRoundingMode(DE_ROUNDINGMODE_TO_NEAREST_EVEN);

	for (unsigned int n_program = 0; n_program < n_programs; ++n_program)
	{
		glw::GLuint   po_id		 = programs[n_program];
		unsigned int  n_stages   = 0;
		_stage_data** stage_data = DE_NULL;

		if (po_id == m_po_cs_id)
		{
			n_stages   = n_cs_stages;
			stage_data = cs_stages;
		}
		else
		{
			n_stages   = n_noncs_stages;
			stage_data = noncs_stages;
		}

		/* Skip compute shader program if not supported */
		if (0 == po_id)
		{
			continue;
		}

		for (unsigned int n_stage = 0; n_stage < n_stages; ++n_stage)
		{
			/* Iterate through all uniforms */
			_stage_data* stage_ptr = stage_data[n_stage];

			/* Set up arrays that we will guide the automated testing */
			const uniform_value_pair double_uniforms[] = {
				uniform_value_pair(stage_ptr->uniforms.uniform_location_double, &stage_ptr->uniforms.uniform_double),
				uniform_value_pair(stage_ptr->uniforms.uniform_location_double_arr[0],
								   stage_ptr->uniforms.uniform_double_arr + 0),
				uniform_value_pair(stage_ptr->uniforms.uniform_location_double_arr[1],
								   stage_ptr->uniforms.uniform_double_arr + 1),
				uniform_value_pair(stage_ptr->uniform_structure_arrays[0].uniform_location_double,
								   &stage_ptr->uniform_structure_arrays[0].uniform_double),
				uniform_value_pair(stage_ptr->uniform_structure_arrays[1].uniform_location_double,
								   &stage_ptr->uniform_structure_arrays[1].uniform_double)
			};
			const uniform_value_pair dvec2_uniforms[] = {
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec2, stage_ptr->uniforms.uniform_dvec2),
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec2_arr[0],
								   stage_ptr->uniforms.uniform_dvec2_arr + 0),
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec2_arr[1],
								   stage_ptr->uniforms.uniform_dvec2_arr + 2),
				uniform_value_pair(stage_ptr->uniform_structure_arrays[0].uniform_location_dvec2,
								   stage_ptr->uniform_structure_arrays[0].uniform_dvec2),
				uniform_value_pair(stage_ptr->uniform_structure_arrays[1].uniform_location_dvec2,
								   stage_ptr->uniform_structure_arrays[1].uniform_dvec2)
			};
			const uniform_value_pair dvec3_uniforms[] = {
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec3, stage_ptr->uniforms.uniform_dvec3),
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec3_arr[0],
								   stage_ptr->uniforms.uniform_dvec3_arr + 0),
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec3_arr[1],
								   stage_ptr->uniforms.uniform_dvec3_arr + 3),
				uniform_value_pair(stage_ptr->uniform_structure_arrays[0].uniform_location_dvec3,
								   stage_ptr->uniform_structure_arrays[0].uniform_dvec3),
				uniform_value_pair(stage_ptr->uniform_structure_arrays[1].uniform_location_dvec3,
								   stage_ptr->uniform_structure_arrays[1].uniform_dvec3)
			};
			const uniform_value_pair dvec4_uniforms[] = {
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec4, stage_ptr->uniforms.uniform_dvec4),
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec4_arr[0],
								   stage_ptr->uniforms.uniform_dvec4_arr + 0),
				uniform_value_pair(stage_ptr->uniforms.uniform_location_dvec4_arr[1],
								   stage_ptr->uniforms.uniform_dvec4_arr + 4),
				uniform_value_pair(stage_ptr->uniform_structure_arrays[0].uniform_location_dvec4,
								   stage_ptr->uniform_structure_arrays[0].uniform_dvec4),
				uniform_value_pair(stage_ptr->uniform_structure_arrays[1].uniform_location_dvec4,
								   stage_ptr->uniform_structure_arrays[1].uniform_dvec4)
			};

			/* Iterate over all uniforms and verify the values reported by the API */
			double		 returned_double_data[4];
			float		 returned_float_data[4];
			int			 returned_int_data[4];
			unsigned int returned_uint_data[4];

			for (unsigned int n_type = 0; n_type < 4 /* double/dvec2/dvec3/dvec4 */; ++n_type)
			{
				const uniform_value_pair* current_uv_pairs  = NULL;
				const unsigned int		  n_components_used = n_type + 1; /* n_type=0: double, n_type=1: dvec2, etc.. */
				unsigned int			  n_pairs			= 0;

				switch (n_type)
				{
				case 0: /* double */
				{
					current_uv_pairs = double_uniforms;
					n_pairs			 = sizeof(double_uniforms) / sizeof(double_uniforms[0]);

					break;
				}

				case 1: /* dvec2 */
				{
					current_uv_pairs = dvec2_uniforms;
					n_pairs			 = sizeof(dvec2_uniforms) / sizeof(dvec2_uniforms[0]);

					break;
				}

				case 2: /* dvec3 */
				{
					current_uv_pairs = dvec3_uniforms;
					n_pairs			 = sizeof(dvec3_uniforms) / sizeof(dvec3_uniforms[0]);

					break;
				}

				case 3: /* dvec4 */
				{
					current_uv_pairs = dvec4_uniforms;
					n_pairs			 = sizeof(dvec4_uniforms) / sizeof(dvec4_uniforms[0]);

					break;
				}

				default:
				{
					TCU_FAIL("Invalid type index requested");
				}
				} /* switch (n_type) */

				for (unsigned int n_pair = 0; n_pair < n_pairs; ++n_pair)
				{
					const uniform_value_pair& current_uv_pair  = current_uv_pairs[n_pair];
					glw::GLint				  uniform_location = current_uv_pair.first;
					const double*			  uniform_value	= current_uv_pair.second;

					/* Retrieve the values from the GL implementation*/
					gl.getUniformdv(po_id, uniform_location, returned_double_data);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformdv() call failed.");

					gl.getUniformfv(po_id, uniform_location, returned_float_data);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformfv() call failed.");

					gl.getUniformiv(po_id, uniform_location, returned_int_data);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformiv() call failed.");

					gl.getUniformuiv(po_id, uniform_location, returned_uint_data);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformuiv() call failed.");

					/* Make sure the values reported match the reference values */
					bool		can_continue = true;
					const float epsilon		 = 1e-5f;

					for (unsigned int n_component = 0; n_component < n_components_used && can_continue; ++n_component)
					{
						if (de::abs(returned_double_data[n_component] - uniform_value[n_component]) > epsilon)
						{
							m_testCtx.getLog()
								<< tcu::TestLog::Message
								<< "Invalid uniform value reported by glGetUniformdv() for uniform location ["
								<< uniform_location << "]"
													   " and component ["
								<< n_component << "]"
												  ": retrieved:["
								<< returned_double_data[n_component] << "]"
																		", expected:["
								<< uniform_value[n_component] << "]" << tcu::TestLog::EndMessage;

							result = false;
						}

						if (de::abs(returned_float_data[n_component] - uniform_value[n_component]) > epsilon)
						{
							m_testCtx.getLog()
								<< tcu::TestLog::Message
								<< "Invalid uniform value reported by glGetUniformfv() for uniform location ["
								<< uniform_location << "]"
													   " and component ["
								<< n_component << "]"
												  ": retrieved:["
								<< returned_float_data[n_component] << "]"
																	   ", expected:["
								<< uniform_value[n_component] << "]" << tcu::TestLog::EndMessage;

							result = false;
						}

						/* ints */
						int rounded_uniform_value_sint = (int)(deFloatRound((float)uniform_value[n_component]));
						unsigned int rounded_uniform_value_uint =
							(unsigned int)(uniform_value[n_component] > 0.0) ?
								((unsigned int)deFloatRound((float)uniform_value[n_component])) :
								0;

						if (returned_int_data[n_component] != rounded_uniform_value_sint)
						{
							m_testCtx.getLog()
								<< tcu::TestLog::Message
								<< "Invalid uniform value reported by glGetUniformiv() for uniform location ["
								<< uniform_location << "]"
													   " and component ["
								<< n_component << "]"
												  ": retrieved:["
								<< returned_int_data[n_component] << "]"
																	 ", expected:["
								<< rounded_uniform_value_sint << "]" << tcu::TestLog::EndMessage;

							result = false;
						}

						if (returned_uint_data[n_component] != rounded_uniform_value_uint)
						{
							m_testCtx.getLog()
								<< tcu::TestLog::Message
								<< "Invalid uniform value reported by glGetUniformuiv() for uniform location ["
								<< uniform_location << "]"
													   " and component ["
								<< n_component << "]"
												  ": retrieved:["
								<< returned_uint_data[n_component] << "]"
																	  ", expected:["
								<< rounded_uniform_value_uint << "]" << tcu::TestLog::EndMessage;

							result = false;
						}
					} /* for (all components) */
				}	 /* for (all uniform+value pairs) */
			}		  /* for (all 4 uniform types) */
		}			  /* for (all shader stages) */
	}				  /* for (both program objects) */

	/* All done! */
	return result;
}

/** Constructor
 *
 *  @param context Rendering context.
 */
GPUShaderFP64Test5::GPUShaderFP64Test5(deqp::Context& context)
	: TestCase(context, "conversions", "Verifies explicit and implicit casts involving double-precision"
									   " floating-point variables work correctly")
	, m_base_value_bo_data(DE_NULL)
	, m_base_value_bo_id(0)
	, m_has_test_passed(true)
	, m_po_base_value_attribute_location(-1)
	, m_po_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_xfb_bo_id(0)
	, m_xfb_bo_size(0)
{
	/* Set up base value array (as per test spec) */
	m_base_values[0] = -25.12065f;
	m_base_values[1] = 0.0f;
	m_base_values[2] = 0.001f;
	m_base_values[3] = 1.0f;
	m_base_values[4] = 256.78901f;

	/* Set up swizzle matrix */
	m_swizzle_matrix[0][0] = SWIZZLE_TYPE_NONE;
	m_swizzle_matrix[0][1] = SWIZZLE_TYPE_Y;
	m_swizzle_matrix[0][2] = SWIZZLE_TYPE_Z;
	m_swizzle_matrix[0][3] = SWIZZLE_TYPE_W;
	m_swizzle_matrix[1][0] = SWIZZLE_TYPE_NONE;
	m_swizzle_matrix[1][1] = SWIZZLE_TYPE_YX;
	m_swizzle_matrix[1][2] = SWIZZLE_TYPE_ZY;
	m_swizzle_matrix[1][3] = SWIZZLE_TYPE_WX;
	m_swizzle_matrix[2][0] = SWIZZLE_TYPE_NONE;
	m_swizzle_matrix[2][1] = SWIZZLE_TYPE_YXX;
	m_swizzle_matrix[2][2] = SWIZZLE_TYPE_XZY;
	m_swizzle_matrix[2][3] = SWIZZLE_TYPE_XWZY;
	m_swizzle_matrix[3][0] = SWIZZLE_TYPE_NONE;
	m_swizzle_matrix[3][1] = SWIZZLE_TYPE_YXXY;
	m_swizzle_matrix[3][2] = SWIZZLE_TYPE_XZXY;
	m_swizzle_matrix[3][3] = SWIZZLE_TYPE_XZYW;
}

void GPUShaderFP64Test5::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_base_value_bo_data != DE_NULL)
	{
		delete[] m_base_value_bo_data;

		m_base_value_bo_data = DE_NULL;
	}

	if (m_base_value_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_base_value_bo_id);

		m_base_value_bo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_xfb_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_xfb_bo_id);

		m_xfb_bo_id = 0;
	}

	/* TCU_FAIL will skip the per sub test iteration de-initialization, we need to
	 * take care of it here
	 */
	deinitInteration();
}

/** Deinitializes all buffers and GL objects that may have been generated
 *  during test execution.
 **/
void GPUShaderFP64Test5::deinitInteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test case iteration using user-provided test case descriptor.
 *
 *  This function may throw a TestError exception if GL implementation misbehaves.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return true if the values returned by GL implementation were found to be valid,
 *          false otherwise.
 **/
bool GPUShaderFP64Test5::executeIteration(const _test_case& test_case)
{
	bool result = true;

	/* Convert the base values array to the type of input attribute we'll be using
	 * for the iteration.
	 */
	Utils::_variable_type base_value_type = Utils::getBaseVariableType(test_case.src_type);

	if (base_value_type == Utils::VARIABLE_TYPE_BOOL)
	{
		/* bools are actually represented by ints, since bool varyings are not allowed */
		base_value_type = Utils::VARIABLE_TYPE_INT;
	}

	const unsigned int base_value_component_size = Utils::getBaseVariableTypeComponentSize(base_value_type);
	const unsigned int n_base_values			 = sizeof(m_base_values) / sizeof(m_base_values[0]);

	m_base_value_bo_data = new unsigned char[base_value_component_size * n_base_values];

	unsigned char* base_value_traveller_ptr = m_base_value_bo_data;

	for (unsigned int n_base_value = 0; n_base_value < n_base_values; ++n_base_value)
	{
		switch (base_value_type)
		{
		case Utils::VARIABLE_TYPE_DOUBLE:
			*((double*)base_value_traveller_ptr) = (double)m_base_values[n_base_value];
			break;
		case Utils::VARIABLE_TYPE_FLOAT:
			*((float*)base_value_traveller_ptr) = (float)m_base_values[n_base_value];
			break;
		case Utils::VARIABLE_TYPE_INT:
			*((int*)base_value_traveller_ptr) = (int)m_base_values[n_base_value];
			break;
		case Utils::VARIABLE_TYPE_UINT:
			*((unsigned int*)base_value_traveller_ptr) = (unsigned int)m_base_values[n_base_value];
			break;

		default:
		{
			TCU_FAIL("Unrecognized base value type");
		}
		}

		base_value_traveller_ptr += base_value_component_size;
	} /* for (all base values) */

	/* Update buffer object storage with the data we've just finished preparing. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(GL_ARRAY_BUFFER, m_base_value_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferSubData(GL_ARRAY_BUFFER, 0 /* offset */, base_value_component_size * n_base_values, m_base_value_bo_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");

	/* Configure vertex attribute array corresponding to 'base_value' attribute, so that the
	 * new data is interpreted correctly.
	 */
	if (base_value_type == Utils::VARIABLE_TYPE_FLOAT)
	{
		gl.vertexAttribPointer(m_po_base_value_attribute_location, 1,							  /* size */
							   Utils::getGLDataTypeOfBaseVariableType(base_value_type), GL_FALSE, /* normalized */
							   0,																  /* stride */
							   DE_NULL);														  /* pointer */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed.");
	}
	else if (base_value_type == Utils::VARIABLE_TYPE_INT || base_value_type == Utils::VARIABLE_TYPE_UINT)
	{
		gl.vertexAttribIPointer(m_po_base_value_attribute_location, 1,						/* size */
								Utils::getGLDataTypeOfBaseVariableType(base_value_type), 0, /* stride */
								DE_NULL);													/* pointer */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIPointer() call failed.");
	}
	else
	{
		DE_ASSERT(base_value_type == Utils::VARIABLE_TYPE_DOUBLE);

		gl.vertexAttribLPointer(m_po_base_value_attribute_location, 1, /* size */
								GL_DOUBLE, 0,						   /* stride */
								DE_NULL);							   /* pointer */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertxAttribLPointer() call failed.");
	}

	gl.enableVertexAttribArray(m_po_base_value_attribute_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

	/* Execute the draw call */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
	{
		gl.drawArrays(GL_POINTS, 0 /* first */, n_base_values);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	/* Map the XFB buffer object into process space */
	void* xfb_data_ptr = gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");
	DE_ASSERT(xfb_data_ptr != NULL);

	/* Verify the data */
	result &= verifyXFBData((const unsigned char*)xfb_data_ptr, test_case);

	/* Unmap the XFB BO */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	/** Good to release the data buffer at this point */
	if (m_base_value_bo_data != DE_NULL)
	{
		delete[] m_base_value_bo_data;

		m_base_value_bo_data = DE_NULL;
	}

	/* All done */
	return result;
}

/** Returns properties of a swizzle operator described by @param type swizzle type.
 *
 *  @param out_swizzle_string  Deref will be used to store a GLSL literal
 *                             corresponding to the specific swizzle operator.
 *                             Must not be NULL.
 *  @param out_n_components    Deref will be used to store the amount of components
 *                             used by the operator. Must not be NULL.
 *  @param out_component_order Deref will be used to store up to 4 integer values,
 *                             corresponding to component indices described by the
 *                             operator for a particular position.  Must not be NULL.
 **/
void GPUShaderFP64Test5::getSwizzleTypeProperties(_swizzle_type type, std::string* out_swizzle_string,
												  unsigned int* out_n_components, unsigned int* out_component_order)
{
	unsigned int result_component_order[4] = { 0 };
	unsigned int result_n_components	   = 0;
	std::string  result_swizzle_string;

	switch (type)
	{
	case SWIZZLE_TYPE_NONE:
	{
		result_swizzle_string = "";
		result_n_components   = 0;

		break;
	}

	case SWIZZLE_TYPE_XWZY:
	{
		result_swizzle_string	 = "xwzy";
		result_n_components		  = 4;
		result_component_order[0] = 0;
		result_component_order[1] = 3;
		result_component_order[2] = 2;
		result_component_order[3] = 1;

		break;
	}

	case SWIZZLE_TYPE_XZXY:
	{
		result_swizzle_string	 = "xzxy";
		result_n_components		  = 4;
		result_component_order[0] = 0;
		result_component_order[1] = 2;
		result_component_order[2] = 0;
		result_component_order[3] = 1;

		break;
	}

	case SWIZZLE_TYPE_XZY:
	{
		result_swizzle_string	 = "xzy";
		result_n_components		  = 3;
		result_component_order[0] = 0;
		result_component_order[1] = 2;
		result_component_order[2] = 1;

		break;
	}

	case SWIZZLE_TYPE_XZYW:
	{
		result_swizzle_string	 = "xzyw";
		result_n_components		  = 4;
		result_component_order[0] = 0;
		result_component_order[1] = 2;
		result_component_order[2] = 1;
		result_component_order[3] = 3;

		break;
	}

	case SWIZZLE_TYPE_Y:
	{
		result_swizzle_string	 = "y";
		result_n_components		  = 1;
		result_component_order[0] = 1;

		break;
	}

	case SWIZZLE_TYPE_YX:
	{
		result_swizzle_string	 = "yx";
		result_n_components		  = 2;
		result_component_order[0] = 1;
		result_component_order[1] = 0;

		break;
	}

	case SWIZZLE_TYPE_YXX:
	{
		result_swizzle_string	 = "yxx";
		result_n_components		  = 3;
		result_component_order[0] = 1;
		result_component_order[1] = 0;
		result_component_order[2] = 0;

		break;
	}

	case SWIZZLE_TYPE_YXXY:
	{
		result_swizzle_string	 = "yxxy";
		result_n_components		  = 4;
		result_component_order[0] = 1;
		result_component_order[1] = 0;
		result_component_order[2] = 0;
		result_component_order[3] = 1;

		break;
	}

	case SWIZZLE_TYPE_Z:
	{
		result_swizzle_string	 = "z";
		result_n_components		  = 1;
		result_component_order[0] = 2;

		break;
	}

	case SWIZZLE_TYPE_ZY:
	{
		result_swizzle_string	 = "zy";
		result_n_components		  = 2;
		result_component_order[0] = 2;
		result_component_order[1] = 1;

		break;
	}

	case SWIZZLE_TYPE_W:
	{
		result_swizzle_string	 = "w";
		result_n_components		  = 1;
		result_component_order[0] = 3;

		break;
	}

	case SWIZZLE_TYPE_WX:
	{
		result_swizzle_string	 = "wx";
		result_n_components		  = 2;
		result_component_order[0] = 3;
		result_component_order[1] = 0;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized swizzle type");
	}
	} /* switch (type) */

	if (out_swizzle_string != DE_NULL)
	{
		*out_swizzle_string = result_swizzle_string;
	}

	if (out_n_components != DE_NULL)
	{
		*out_n_components = result_n_components;
	}

	if (out_component_order != DE_NULL)
	{
		memcpy(out_component_order, result_component_order, sizeof(unsigned int) * result_n_components);
	}
}

/** Returns body of a vertex shader that should be used for particular test case,
 *  given user-specified test case descriptor.
 *
 *  @param test_case Descriptor to use for the query.
 *
 *  @return Requested data.
 **/
std::string GPUShaderFP64Test5::getVertexShaderBody(const _test_case& test_case)
{
	std::stringstream  result;
	const std::string  base_type_string = Utils::getVariableTypeString(Utils::getBaseVariableType(test_case.src_type));
	const std::string  dst_type_string  = Utils::getVariableTypeString(test_case.dst_type);
	const unsigned int n_dst_components = Utils::getNumberOfComponentsForVariableType(test_case.dst_type);
	const unsigned int n_src_components = Utils::getNumberOfComponentsForVariableType(test_case.src_type);
	const std::string  src_type_string  = Utils::getVariableTypeString(test_case.src_type);

	/* Add version preamble */
	result << "#version 420\n"
			  "\n";

	/* Declare output variables. Note that boolean output variables are not supported, so we need
	 * to handle that special case correctly */
	if (test_case.dst_type == Utils::VARIABLE_TYPE_BOOL)
	{
		result << "out int result;\n";
	}
	else
	{
		result << "out " << dst_type_string << " result;\n";
	}

	/* Declare input variables. Handle the bool case exclusively. */
	if (test_case.src_type == Utils::VARIABLE_TYPE_BOOL)
	{
		/* Use ints for bools. We will cast them to bool in the code later. */
		result << "in int base_value;\n";
	}
	else
	{
		result << "in " << base_type_string << " base_value;\n";
	}

	/* Declare main() and construct the value we will be casting from.
	 *
	 * Note: Addition operations on bool values cause an implicit conversion to int
	 *       which is not allowed. Hence, we skip these operations for this special
	 *       case.
	 */
	result << "void main()\n"
			  "{\n"
		   << src_type_string << " lside_value = ";

	if (test_case.src_type == Utils::VARIABLE_TYPE_BOOL)
	{
		result << src_type_string << "(0 != ";
	}
	else
	{
		result << src_type_string << "(";
	}

	if (test_case.src_type != Utils::VARIABLE_TYPE_BOOL)
	{
		for (unsigned int n_component = 0; n_component < n_src_components; ++n_component)
		{
			result << "base_value + " << n_component;

			if (n_component != (n_src_components - 1))
			{
				result << ", ";
			}
		} /* for (all components) */
	}
	else
	{
		DE_ASSERT(n_src_components == 1);

		result << "base_value";
	}

	result << ");\n";

	/* Perform the casting operation. Add swizzle operator if possible. */
	if (test_case.dst_type == Utils::VARIABLE_TYPE_BOOL)
	{
		/* Handle the bool case exclusively */
		if (test_case.type == TEST_CASE_TYPE_EXPLICIT)
		{
			result << "result = (bool(lside_value) == false) ? 0 : 1";
		}
		else
		{
			result << "result = (lside_value == false) ? 0 : 1";
		}
	}
	else
	{
		if (test_case.type == TEST_CASE_TYPE_EXPLICIT)
		{
			result << "result = " << dst_type_string << "(lside_value)";
		}
		else
		{
			result << "result = lside_value";
		}
	}

	if (n_src_components > 1 && !Utils::isMatrixVariableType(test_case.src_type))
	{
		/* Add a swizzle operator  */
		DE_ASSERT(n_dst_components > 0 && n_dst_components <= 4);
		DE_ASSERT(n_src_components > 0 && n_src_components <= 4);

		unsigned int  swizzle_component_order[4] = { 0 };
		unsigned int  swizzle_n_components		 = 0;
		_swizzle_type swizzle_operator			 = m_swizzle_matrix[n_dst_components - 1][n_src_components - 1];
		std::string   swizzle_string;

		getSwizzleTypeProperties(swizzle_operator, &swizzle_string, &swizzle_n_components, swizzle_component_order);

		if (swizzle_n_components > 0)
		{
			result << "." << swizzle_string;
		}
	}

	/* Close the shader implementation. */
	result << ";\n"
			  "}\n";

	return result.str();
}

/** Initializes program & shader objects needed to run the iteration, given
 *  user-specified test case descriptor.
 *
 *  This function can throw a TestError exception if a GL error is detected
 *  during execution.
 *
 *  @param test_case Descriptor to use for the iteration.
 **/
void GPUShaderFP64Test5::initIteration(_test_case& test_case)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program & shader objects */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Configure shader body */
	std::string body		 = getVertexShaderBody(test_case);
	const char* body_raw_ptr = body.c_str();

	gl.shaderSource(m_vs_id, 1 /* count */, &body_raw_ptr, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Store it in the test case descriptor for logging purposes */
	test_case.shader_body = body;

	/* Compile the shader */
	glw::GLint compile_status = GL_FALSE;

	gl.compileShader(m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status != GL_TRUE)
	{
		TCU_FAIL("Shader compilation failed");
	}

	/* Attach the shader to the program obejct */
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

	/* Configure XFB for the program object */
	const char* xfb_varying_name = "result";

	gl.transformFeedbackVaryings(m_po_id, 1 /* count */, &xfb_varying_name, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* Retrieve attribute locations */
	m_po_base_value_attribute_location = gl.getAttribLocation(m_po_id, "base_value");
	GLU_EXPECT_NO_ERROR(gl.getError(), "getAttribLocation() call failed.");

	if (m_po_base_value_attribute_location == -1)
	{
		TCU_FAIL("'base_value' is considered an inactive attribute which is invalid.");
	}
}

/** Initializes GL objects used by all test cases.
 *
 *  This function may throw a TestError exception if GL implementation reports
 *  an error at any point.
 **/
void GPUShaderFP64Test5::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate buffer object IDs */
	gl.genBuffers(1, &m_base_value_bo_id);
	gl.genBuffers(1, &m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call(s) failed.");

	/* Allocate buffer object storage for 'base_value' input attribute data. All iterations
	 * will never eat up more than 1 double (as per test spec) and we will be drawing
	 * as many points in a single draw call as there are defined in m_base_values array.
	 */
	const unsigned int n_base_values = sizeof(m_base_values) / sizeof(m_base_values[0]);

	gl.bindBuffer(GL_ARRAY_BUFFER, m_base_value_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(double) * n_base_values, DE_NULL /* data */, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Allocate buffer object storage for XFB data. For each iteratiom we will be using
	 * five base values. Each XFBed value can take up to 16 components (eg. mat4) and be
	 * of double type (eg. dmat4), so make sure a sufficient amount of space is requested.
	 */
	const unsigned int xfb_bo_size = sizeof(double) * 16 /* components */ * n_base_values;

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_bo_size, DE_NULL /* data */, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Allocate a client-side buffer to hold the data we will be mapping from XFB BO */
	m_xfb_bo_size = xfb_bo_size;

	/* Generate a vertex array object we will need to use for the draw calls */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GPUShaderFP64Test5::iterate()
{
	/* Do not execute the test if GL_ARB_texture_view is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported.");
	}

	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_vertex_attrib_64bit"))
	{
		throw tcu::NotSupportedError("GL_ARB_vertex_attrib_64bit is not supported.");
	}

	/* Initialize GL objects needed to run the tests */
	initTest();

	/* Build iteration array to run the tests in an automated manner */
	_test_case test_cases[] = {
		/* test case type */ /* source type */ /* destination type */
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_INT, Utils::VARIABLE_TYPE_DOUBLE, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_IVEC2, Utils::VARIABLE_TYPE_DVEC2, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_IVEC3, Utils::VARIABLE_TYPE_DVEC3, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_IVEC4, Utils::VARIABLE_TYPE_DVEC4, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_UINT, Utils::VARIABLE_TYPE_DOUBLE, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_UVEC2, Utils::VARIABLE_TYPE_DVEC2, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_UVEC3, Utils::VARIABLE_TYPE_DVEC3, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_UVEC4, Utils::VARIABLE_TYPE_DVEC4, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_FLOAT, Utils::VARIABLE_TYPE_DOUBLE, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_VEC2, Utils::VARIABLE_TYPE_DVEC2, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_VEC3, Utils::VARIABLE_TYPE_DVEC3, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_VEC4, Utils::VARIABLE_TYPE_DVEC4, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT2, Utils::VARIABLE_TYPE_DMAT2, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT3, Utils::VARIABLE_TYPE_DMAT3, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT4, Utils::VARIABLE_TYPE_DMAT4, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT2X3, Utils::VARIABLE_TYPE_DMAT2X3, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT2X4, Utils::VARIABLE_TYPE_DMAT2X4, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT3X2, Utils::VARIABLE_TYPE_DMAT3X2, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT3X4, Utils::VARIABLE_TYPE_DMAT3X4, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT4X2, Utils::VARIABLE_TYPE_DMAT4X2, "" },
		{ TEST_CASE_TYPE_IMPLICIT, Utils::VARIABLE_TYPE_MAT4X3, Utils::VARIABLE_TYPE_DMAT4X3, "" },

		{ TEST_CASE_TYPE_EXPLICIT, Utils::VARIABLE_TYPE_INT, Utils::VARIABLE_TYPE_DOUBLE, "" },
		{ TEST_CASE_TYPE_EXPLICIT, Utils::VARIABLE_TYPE_UINT, Utils::VARIABLE_TYPE_DOUBLE, "" },
		{ TEST_CASE_TYPE_EXPLICIT, Utils::VARIABLE_TYPE_FLOAT, Utils::VARIABLE_TYPE_DOUBLE, "" },
		{ TEST_CASE_TYPE_EXPLICIT, Utils::VARIABLE_TYPE_DOUBLE, Utils::VARIABLE_TYPE_INT, "" },
		{ TEST_CASE_TYPE_EXPLICIT, Utils::VARIABLE_TYPE_DOUBLE, Utils::VARIABLE_TYPE_UINT, "" },
		{ TEST_CASE_TYPE_EXPLICIT, Utils::VARIABLE_TYPE_DOUBLE, Utils::VARIABLE_TYPE_FLOAT, "" },
		{ TEST_CASE_TYPE_EXPLICIT, Utils::VARIABLE_TYPE_DOUBLE, Utils::VARIABLE_TYPE_BOOL, "" },
		{ TEST_CASE_TYPE_EXPLICIT, Utils::VARIABLE_TYPE_BOOL, Utils::VARIABLE_TYPE_DOUBLE, "" }
	};
	const unsigned int n_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

	/* Execute all iterations */
	for (unsigned int n_test_case = 0; n_test_case < n_test_cases; ++n_test_case)
	{
		_test_case& test_case = test_cases[n_test_case];

		/* Initialize a program object we will use to perform the casting */
		initIteration(test_case);

		/* Use the program object to XFB the results */
		m_has_test_passed &= executeIteration(test_case);

		/* Release the GL Resource for this sub test */
		deinitInteration();

	} /* for (all test cases) */
	/* We're done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies if data XFBed out by the vertex shader are valid, given test case descriptor,
 *  for which the data have been generated.
 *
 *  @param data_ptr  Buffer holding the data XFBed out by the shader.
 *  @param test_case Descriptor of the test case, for which the vertex shader was
 *                   generated.
 *
 *  @return true if the data were found to be valid, false otherwise.
 **/
bool GPUShaderFP64Test5::verifyXFBData(const unsigned char* data_ptr, const _test_case& test_case)
{
	const Utils::_variable_type base_dst_type		= Utils::getBaseVariableType(test_case.dst_type);
	const Utils::_variable_type base_src_type		= Utils::getBaseVariableType(test_case.src_type);
	const float					epsilon				= 1e-5f;
	const unsigned int			n_base_values		= sizeof(m_base_values) / sizeof(m_base_values[0]);
	const unsigned int			n_result_components = Utils::getNumberOfComponentsForVariableType(test_case.dst_type);
	const unsigned int			n_src_components	= Utils::getNumberOfComponentsForVariableType(test_case.src_type);
	bool						result				= true;
	_swizzle_type				swizzle_operator	= SWIZZLE_TYPE_NONE;
	unsigned int				swizzle_order[4]	= { 0 };
	const unsigned char*		traveller_ptr		= data_ptr;

	if (!Utils::isMatrixVariableType(test_case.src_type))
	{
		DE_ASSERT(n_result_components >= 1 && n_result_components <= 4);
		DE_ASSERT(n_src_components >= 1 && n_src_components <= 4);

		swizzle_operator = m_swizzle_matrix[n_result_components - 1][n_src_components - 1];

		getSwizzleTypeProperties(swizzle_operator, DE_NULL, /* out_swizzle_string */
								 DE_NULL,					/* out_n_components */
								 swizzle_order);
	}

	for (unsigned int n_base_value = 0; n_base_value < n_base_values; ++n_base_value)
	{
		for (unsigned int n_result_component = 0; n_result_component < n_result_components; ++n_result_component)
		{
			unsigned int n_swizzled_component = n_result_component;

			if (swizzle_operator != SWIZZLE_TYPE_NONE)
			{
				n_swizzled_component =
					(n_result_component / n_result_components) * n_result_component + swizzle_order[n_result_component];
			}

			switch (base_dst_type)
			{
			case Utils::VARIABLE_TYPE_BOOL:
			case Utils::VARIABLE_TYPE_INT:
			{
				double ref_expected_value = (m_base_values[n_base_value]) + static_cast<float>(n_swizzled_component);
				double expected_value	 = ref_expected_value;
				int	result_value		  = *((int*)traveller_ptr);

				if (base_dst_type == Utils::VARIABLE_TYPE_BOOL)
				{
					if (expected_value != 0.0)
					{
						expected_value = 1.0;
					}
				}

				if (result_value != (int)expected_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Invalid boolean/integer value obtained when doing an "
									   << ((test_case.type == TEST_CASE_TYPE_EXPLICIT) ? "explicit" : "implicit")
									   << " cast from GLSL type [" << Utils::getVariableTypeString(test_case.src_type)
									   << "]"
										  ", component index: ["
									   << n_swizzled_component << "]"
																  ", value: ["
									   << ref_expected_value << "]"
																" to GLSL type ["
									   << Utils::getVariableTypeString(test_case.dst_type) << "]"
																							  ", retrieved value: ["
									   << result_value << "]"
														  ", expected value: ["
									   << (int)expected_value << "]"
																 ", shader used:\n"
									   << test_case.shader_body.c_str() << tcu::TestLog::EndMessage;

					result = false;
				}

				traveller_ptr += sizeof(int);
				break;
			} /* VARIABLE_TYPE_BOOL or VARIABLE_TYPE_INT cases */

			case Utils::VARIABLE_TYPE_DOUBLE:
			{
				double ref_expected_value = m_base_values[n_base_value] + (double)n_swizzled_component;
				double expected_value	 = ref_expected_value;
				double result_value		  = *((double*)traveller_ptr);

				if (base_src_type == Utils::VARIABLE_TYPE_BOOL)
				{
					expected_value = ((int)expected_value != 0.0) ? 1.0 : 0.0;
				}
				else if (base_src_type == Utils::VARIABLE_TYPE_INT)
				{
					expected_value = (int)expected_value;
				}
				else if (base_src_type == Utils::VARIABLE_TYPE_UINT)
				{
					// Negative values in base values array when converted to unsigned int will be ZERO
					// Addition operations done inside the shader in such cases will operate on ZERO rather
					// than the negative value being passed.
					// Replicate the sequence of conversion and addition operations done on the
					// shader input, to calculate the expected values in XFB data in the
					// problematic cases.
					if (expected_value < 0)
					{
						expected_value = (unsigned int)m_base_values[n_base_value] + n_swizzled_component;
					}
					expected_value = (unsigned int)expected_value;
				}

				traveller_ptr += sizeof(double);
				if (de::abs(result_value - expected_value) > epsilon)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Invalid double-precision floating-point value obtained when doing an "
									   << ((test_case.type == TEST_CASE_TYPE_EXPLICIT) ? "explicit" : "implicit")
									   << " cast from GLSL type [" << Utils::getVariableTypeString(test_case.src_type)
									   << "]"
										  ", component index: ["
									   << n_swizzled_component << "]"
																  ", value: ["
									   << ref_expected_value << "]"
																" to GLSL type ["
									   << Utils::getVariableTypeString(test_case.dst_type) << "]"
																							  ", retrieved value: ["
									   << std::setprecision(16) << result_value << "]"
																				   ", expected value: ["
									   << std::setprecision(16) << expected_value << "]"
																					 ", shader used:\n"
									   << test_case.shader_body.c_str() << tcu::TestLog::EndMessage;

					result = false;
				}

				break;
			} /* VARIABLE_TYPE_DOUBLE case */

			case Utils::VARIABLE_TYPE_FLOAT:
			{
				float ref_expected_value = (float)m_base_values[n_base_value] + (float)n_swizzled_component;
				float expected_value	 = ref_expected_value;
				float result_value		 = *((float*)traveller_ptr);

				if (base_src_type == Utils::VARIABLE_TYPE_BOOL)
				{
					expected_value = (expected_value != 0.0f) ? 1.0f : 0.0f;
				}
				else if (base_src_type == Utils::VARIABLE_TYPE_INT)
				{
					expected_value = (float)((int)expected_value);
				}
				else if (base_src_type == Utils::VARIABLE_TYPE_UINT)
				{
					expected_value = (float)((unsigned int)expected_value);
				}

				traveller_ptr += sizeof(float);
				if (de::abs(result_value - expected_value) > epsilon)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Invalid single-precision floating-point value obtained when doing an "
									   << ((test_case.type == TEST_CASE_TYPE_EXPLICIT) ? "explicit" : "implicit")
									   << " cast from GLSL type [" << Utils::getVariableTypeString(test_case.src_type)
									   << "]"
										  ", component index: ["
									   << n_swizzled_component << "]"
																  ", value: ["
									   << ref_expected_value << "]"
																" to GLSL type ["
									   << Utils::getVariableTypeString(test_case.dst_type) << "]"
																							  ", retrieved value: ["
									   << std::setprecision(16) << result_value << "]"
																				   ", expected value: ["
									   << std::setprecision(16) << expected_value << "]"
																					 ", shader used:\n"
									   << test_case.shader_body.c_str() << tcu::TestLog::EndMessage;

					result = false;
				}

				break;
			} /* VARIABLE_TYPE_FLOAT case */

			case Utils::VARIABLE_TYPE_UINT:
			{
				double ref_expected_value = (m_base_values[n_base_value]) + static_cast<float>(n_swizzled_component);
				double expected_value	 = ref_expected_value;
				unsigned int result_value = *((unsigned int*)traveller_ptr);

				traveller_ptr += sizeof(unsigned int);
				if (result_value != (unsigned int)expected_value)
				{
					if (expected_value < 0.0)
					{
						// It is undefined to convert a negative floating-point value to an uint.
						break;
					}

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Invalid unsigned integer value obtained when doing an "
									   << ((test_case.type == TEST_CASE_TYPE_EXPLICIT) ? "explicit" : "implicit")
									   << " cast from GLSL type [" << Utils::getVariableTypeString(test_case.src_type)
									   << "]"
										  ", component index: ["
									   << n_swizzled_component << "]"
																  ", value: ["
									   << ref_expected_value << "]"
																" to GLSL type ["
									   << Utils::getVariableTypeString(test_case.dst_type) << "]"
																							  ", retrieved value: ["
									   << result_value << "]"
														  ", expected value: ["
									   << (unsigned int)expected_value << "]"
																		  ", shader used:\n"
									   << test_case.shader_body.c_str() << tcu::TestLog::EndMessage;

					result = false;
				}

				break;
			} /* VARIABLE_TYPE_UINT case */

			default:
			{
				TCU_FAIL("Unrecognized variable type");
			}
			} /* switch (test_case.dst_type) */
		}	 /* for (all result components) */
	}		  /* for (all base values) */

	return result;
}

/** Constructor
 *
 *  @param context Rendering context.
 */
GPUShaderFP64Test6::GPUShaderFP64Test6(deqp::Context& context)
	: TestCase(context, "illegal_conversions", "Verifies that invalid casts to double-precision variables are detected "
											   "during compilation time.")
	, m_cs_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
	, m_has_test_passed(true)
{
}

/** Deinitializes all buffers and GL objects that may have been generated
 *  during test execution.
 **/
void GPUShaderFP64Test6::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_cs_id != 0)
	{
		gl.deleteShader(m_cs_id);

		m_cs_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test case.
 *
 *  This function can throw TestError exceptions if GL implementation reports
 *  an error.
 *
 *  @param test_case Test case descriptor.
 *
 *  @return true if test case passed, false otherwise.
 **/
bool GPUShaderFP64Test6::executeIteration(const _test_case& test_case)
{
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();
	const glw::GLuint	 so_ids[]   = { m_cs_id, m_fs_id, m_gs_id, m_tc_id, m_te_id, m_vs_id };
	const unsigned int	n_so_ids   = sizeof(so_ids) / sizeof(so_ids[0]);
	bool				  result	 = true;
	const char*			  stage_body = NULL;
	const char*			  stage_name = NULL;

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		const glw::GLuint so_id = so_ids[n_so_id];

		/* Skip compute shader if it is not supported */
		if (0 == so_id)
		{
			continue;
		}

		/* Compile the shader */
		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		/* Has the compilation failed as expected? */
		glw::GLint compile_status = GL_TRUE;

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		if (compile_status == GL_TRUE)
		{
			/* What is the current stage's name? */
			if (so_id == m_cs_id)
			{
				stage_body = test_case.cs_shader_body.c_str();
				stage_name = "Compute shader";
			}
			else if (so_id == m_fs_id)
			{
				stage_body = test_case.fs_shader_body.c_str();
				stage_name = "Fragment shader";
			}
			else if (so_id == m_gs_id)
			{
				stage_body = test_case.gs_shader_body.c_str();
				stage_name = "Geometry shader";
			}
			else if (so_id == m_tc_id)
			{
				stage_body = test_case.tc_shader_body.c_str();
				stage_name = "Tessellation control shader";
			}
			else if (so_id == m_te_id)
			{
				stage_body = test_case.te_shader_body.c_str();
				stage_name = "Tessellation evaluation shader";
			}
			else if (so_id == m_vs_id)
			{
				stage_body = test_case.vs_shader_body.c_str();
				stage_name = "Vertex shader";
			}
			else
			{
				/* Doesn't make much sense to throw exceptions here so.. */
				stage_body = "";
				stage_name = "[?]";
			}

			/* This shader should have never compiled successfully! */
			m_testCtx.getLog() << tcu::TestLog::Message << stage_name
							   << " has been compiled successfully, even though the shader was malformed."
								  " Following is shader body:\n"
							   << stage_body << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all shader objects) */

	return result;
}

/** Retrieves body of a compute shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test6::getComputeShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Add pre-amble */
	result_sstream << "#version 420\n"
					  "#extension GL_ARB_compute_shader          : require\n"
					  "\n"
					  "layout(local_size_x = 6) in;\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	/* Add local variable declarations */
	result_sstream << Utils::getVariableTypeString(test_case.src_type) << " src";

	if (test_case.src_array_size > 1)
	{
		result_sstream << "[" << test_case.src_array_size << "]";
	}

	result_sstream << ";\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "struct\n"
						  "{\n"
					   << Utils::getVariableTypeString(test_case.dst_type) << " member";
	}
	else
	{
		result_sstream << Utils::getVariableTypeString(test_case.dst_type) << " dst";
	}

	result_sstream << ";\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "\n} dst;\n";
	}

	/* Add actual body */
	result_sstream << "dst = src;\n"
					  "}\n";

	/* Return the body */
	return result_sstream.str();
}

/** Retrieves body of a fragment shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test6::getFragmentShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Add pre-amble */
	result_sstream << "#version 420\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	/* Add local variable declarations */
	result_sstream << Utils::getVariableTypeString(test_case.src_type) << " src";

	if (test_case.src_array_size > 1)
	{
		result_sstream << "[" << test_case.src_array_size << "]";
	}

	result_sstream << ";\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "struct\n"
						  "{\n"
					   << Utils::getVariableTypeString(test_case.dst_type) << " member";
	}
	else
	{
		result_sstream << Utils::getVariableTypeString(test_case.dst_type) << " dst";
	}

	result_sstream << ";\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "\n} dst;\n";
	}

	/* Add actual body */
	result_sstream << "dst = src;\n"
					  "}\n";

	/* Return the body */
	return result_sstream.str();
}

/** Retrieves body of a geometry shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test6::getGeometryShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Add preamble */
	result_sstream << "#version 420\n"
					  "\n"
					  "layout(points)                 in;\n"
					  "layout(max_vertices=1, points) out;\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	/* Add local variable declarations */
	result_sstream << Utils::getVariableTypeString(test_case.src_type) << " src";

	if (test_case.src_array_size > 1)
	{
		result_sstream << "[" << test_case.src_array_size << "]";
	}

	result_sstream << ";\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "struct\n"
						  "{\n"
					   << Utils::getVariableTypeString(test_case.dst_type) << " member";
	}
	else
	{
		result_sstream << Utils::getVariableTypeString(test_case.dst_type) << " dst";
	}

	result_sstream << ";\n"
					  "\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "} dst;\n";
	}

	/* Add actual body */
	result_sstream << "dst = src;\n"
					  "}\n";

	/* We're done! */
	return result_sstream.str();
}

/** Retrieves body of a tesellation control shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test6::getTessellationControlShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Add preamble */
	result_sstream << "#version 420\n"
					  "\n"
					  "layout(vertices=4) out;\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	/* Add local variable declarations. */
	result_sstream << Utils::getVariableTypeString(test_case.src_type) << " src";

	if (test_case.src_array_size > 1)
	{
		result_sstream << "[" << test_case.src_array_size << "]";
	}

	result_sstream << ";\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "struct\n"
						  "{\n"
					   << Utils::getVariableTypeString(test_case.dst_type) << " member";
	}
	else
	{
		result_sstream << Utils::getVariableTypeString(test_case.dst_type) << " dst";
	}

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << ";\n"
						  "} dst;\n";
	}
	else
	{
		result_sstream << ";\n";
	}

	/* Continue with the actual body. */
	result_sstream << "gl_TessLevelOuter[0] = 1.0;\n"
					  "gl_TessLevelOuter[1] = 1.0;\n"
					  "dst                  = src;\n"
					  "}\n";

	/* Return the body */
	return result_sstream.str();
}

/** Retrieves body of a tessellation evaluation shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test6::getTessellationEvaluationShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Add preamble */
	result_sstream << "#version 420\n"
					  "\n"
					  "layout(isolines) in;\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	/* Add local variable declarations */
	result_sstream << Utils::getVariableTypeString(test_case.src_type) << " src";

	if (test_case.src_array_size > 1)
	{
		result_sstream << "[" << test_case.src_array_size << "]";
	}

	result_sstream << ";\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "struct\n"
						  "{\n"
					   << Utils::getVariableTypeString(test_case.dst_type) << " member";
	}
	else
	{
		result_sstream << Utils::getVariableTypeString(test_case.dst_type) << " dst";
	}

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << ";\n"
						  "} dst;\n";
	}
	else
	{
		result_sstream << ";\n";
	}

	/* Continue with the actual body. */
	result_sstream << "dst = src;\n";

	/* Complete the body */
	result_sstream << "}\n";

	/* Return the body */
	return result_sstream.str();
}

/** Retrieves body of a vertex shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test6::getVertexShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Add preamble */
	result_sstream << "#version 420\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	/* Add local variables */
	result_sstream << Utils::getVariableTypeString(test_case.src_type) << " src";

	if (test_case.src_array_size > 1)
	{
		result_sstream << "[" << test_case.src_array_size << "]";
	}

	result_sstream << ";\n";

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << "struct\n"
						  "{\n"
					   << Utils::getVariableTypeString(test_case.dst_type) << " member";
	}
	else
	{
		result_sstream << Utils::getVariableTypeString(test_case.dst_type) << " dst";
	}

	if (test_case.wrap_dst_type_in_structure)
	{
		result_sstream << ";\n"
						  "} dst;\n";
	}
	else
	{
		result_sstream << ";\n";
	}

	/* Start actual body */
	result_sstream << "dst         = src;\n"
					  "gl_Position = vec4(1.0);\n"
					  "}";

	return result_sstream.str();
}

/** Initializes shader objects required to run the test. */
void GPUShaderFP64Test6::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate shader objects */

	/* Compute shader support and GL 4.2 required */
	if ((true == m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader")) &&
		(true == Utils::isGLVersionAtLeast(gl, 4 /* major */, 2 /* minor */)))
	{
		m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	}

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
	m_te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");
}

/** Assigns shader bodies to all shader objects that will be used for a single iteration.
 *
 *  @param test_case Test case descriptor to generate the shader bodies for.
 **/
void GPUShaderFP64Test6::initIteration(_test_case& test_case)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	test_case.cs_shader_body = getComputeShaderBody(test_case);
	test_case.fs_shader_body = getFragmentShaderBody(test_case);
	test_case.gs_shader_body = getGeometryShaderBody(test_case);
	test_case.tc_shader_body = getTessellationControlShaderBody(test_case);
	test_case.te_shader_body = getTessellationEvaluationShaderBody(test_case);
	test_case.vs_shader_body = getVertexShaderBody(test_case);

	/* Assign the bodies to relevant shaders */
	const char* cs_body_raw_ptr = test_case.cs_shader_body.c_str();
	const char* fs_body_raw_ptr = test_case.fs_shader_body.c_str();
	const char* gs_body_raw_ptr = test_case.gs_shader_body.c_str();
	const char* tc_body_raw_ptr = test_case.tc_shader_body.c_str();
	const char* te_body_raw_ptr = test_case.te_shader_body.c_str();
	const char* vs_body_raw_ptr = test_case.vs_shader_body.c_str();

	/* m_cs_id is initialized only if compute_shader is supported */
	if (0 != m_cs_id)
	{
		gl.shaderSource(m_cs_id, 1 /* count */, &cs_body_raw_ptr, DE_NULL /* length */);
	}

	gl.shaderSource(m_fs_id, 1 /* count */, &fs_body_raw_ptr, DE_NULL /* length */);
	gl.shaderSource(m_gs_id, 1 /* count */, &gs_body_raw_ptr, DE_NULL /* length */);
	gl.shaderSource(m_tc_id, 1 /* count */, &tc_body_raw_ptr, DE_NULL /* length */);
	gl.shaderSource(m_te_id, 1 /* count */, &te_body_raw_ptr, DE_NULL /* length */);
	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body_raw_ptr, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GPUShaderFP64Test6::iterate()
{
	/* Do not execute the test if GL_ARB_texture_view is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported.");
	}

	/* Initialize GL objects needed to run the tests */
	initTest();

	/* Build iteration array to run the tests in an automated manner */
	_test_case test_cases[] = {
		/* Src array size */ /* Src type */ /* Dst type */ /* wrap_dst_type_in_structure */
		{ 2, Utils::VARIABLE_TYPE_INT, Utils::VARIABLE_TYPE_DOUBLE, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_INT, Utils::VARIABLE_TYPE_DOUBLE, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_IVEC2, Utils::VARIABLE_TYPE_DVEC2, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_IVEC2, Utils::VARIABLE_TYPE_DVEC2, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_IVEC3, Utils::VARIABLE_TYPE_DVEC3, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_IVEC3, Utils::VARIABLE_TYPE_DVEC3, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_IVEC4, Utils::VARIABLE_TYPE_DVEC4, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_IVEC4, Utils::VARIABLE_TYPE_DVEC4, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_UINT, Utils::VARIABLE_TYPE_DOUBLE, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_UINT, Utils::VARIABLE_TYPE_DOUBLE, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_UVEC2, Utils::VARIABLE_TYPE_DVEC2, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_UVEC2, Utils::VARIABLE_TYPE_DVEC2, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_UVEC3, Utils::VARIABLE_TYPE_DVEC3, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_UVEC3, Utils::VARIABLE_TYPE_DVEC3, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_UVEC4, Utils::VARIABLE_TYPE_DVEC4, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_UVEC4, Utils::VARIABLE_TYPE_DVEC4, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_FLOAT, Utils::VARIABLE_TYPE_DOUBLE, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_FLOAT, Utils::VARIABLE_TYPE_DOUBLE, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_VEC2, Utils::VARIABLE_TYPE_DVEC2, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_VEC2, Utils::VARIABLE_TYPE_DVEC2, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_VEC3, Utils::VARIABLE_TYPE_DVEC3, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_VEC3, Utils::VARIABLE_TYPE_DVEC3, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_VEC4, Utils::VARIABLE_TYPE_DVEC4, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_VEC4, Utils::VARIABLE_TYPE_DVEC4, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT2, Utils::VARIABLE_TYPE_DMAT2, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT2, Utils::VARIABLE_TYPE_DMAT2, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT3, Utils::VARIABLE_TYPE_DMAT3, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT3, Utils::VARIABLE_TYPE_DMAT3, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT4, Utils::VARIABLE_TYPE_DMAT4, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT4, Utils::VARIABLE_TYPE_DMAT4, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT2X3, Utils::VARIABLE_TYPE_DMAT2X3, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT2X3, Utils::VARIABLE_TYPE_DMAT2X3, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT2X4, Utils::VARIABLE_TYPE_DMAT2X4, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT2X4, Utils::VARIABLE_TYPE_DMAT2X4, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT3X2, Utils::VARIABLE_TYPE_DMAT3X2, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT3X2, Utils::VARIABLE_TYPE_DMAT3X2, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT3X4, Utils::VARIABLE_TYPE_DMAT3X4, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT3X4, Utils::VARIABLE_TYPE_DMAT3X4, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT4X2, Utils::VARIABLE_TYPE_DMAT4X2, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT4X2, Utils::VARIABLE_TYPE_DMAT4X2, true, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT4X3, Utils::VARIABLE_TYPE_DMAT4X3, false, "", "", "", "", "", "" },
		{ 2, Utils::VARIABLE_TYPE_MAT4X3, Utils::VARIABLE_TYPE_DMAT4X3, true, "", "", "", "", "", "" }
	};
	const unsigned int n_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

	/* Execute all iterations */
	for (unsigned int n_test_case = 0; n_test_case < n_test_cases; ++n_test_case)
	{
		_test_case& test_case = test_cases[n_test_case];

		/* Initialize a program object we will use to perform the casting */
		initIteration(test_case);

		/* Use the program object to XFB the results */
		m_has_test_passed &= executeIteration(test_case);

	} /* for (all test cases) */

	/* We're done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 *  @param context Rendering context.
 */
GPUShaderFP64Test7::GPUShaderFP64Test7(deqp::Context& context)
	: TestCase(context, "varyings", "Verifies double-precision floating-point varyings work correctly "
									"in all shader stages.")
	, m_are_double_inputs_supported(false)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_has_test_passed(true)
	, m_n_max_components_per_stage(0)
	, m_n_xfb_varyings(0)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_to_id(0)
	, m_to_data(NULL)
	, m_to_height(4)
	, m_to_width(4)
	, m_xfb_bo_id(0)
	, m_xfb_varyings(NULL)
	, m_vao_id(0)
	, m_vs_id(0)
{
}

/** Compiles all shaders attached to test program object and links it.
 *
 *  @param variables
 *
 *  @return true if the process was executed successfully, false otherwise.
 */
bool GPUShaderFP64Test7::buildTestProgram(_variables& variables)
{
	std::string			  fs_body = getFragmentShaderBody(variables);
	const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();
	std::string			  gs_body = getGeometryShaderBody(variables);
	std::string			  tc_body = getTessellationControlShaderBody(variables);
	std::string			  te_body = getTessellationEvaluationShaderBody(variables);
	std::string			  vs_body = getVertexShaderBody(variables);
	bool				  result  = false;

	/* Try to link the program object */
	glw::GLint link_status = GL_FALSE;

	/* Compile the shaders */
	if (!compileShader(m_fs_id, fs_body))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Fragment shader failed to compile." << tcu::TestLog::EndMessage;

		goto end;
	}

	if (!compileShader(m_gs_id, gs_body))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Geometry shader failed to compile." << tcu::TestLog::EndMessage;

		goto end;
	}

	if (!compileShader(m_tc_id, tc_body))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Tessellation control shader failed to compile."
						   << tcu::TestLog::EndMessage;

		goto end;
	}

	if (!compileShader(m_te_id, te_body))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Tessellation evaluation shader failed to compile."
						   << tcu::TestLog::EndMessage;

		goto end;
	}

	if (!compileShader(m_vs_id, vs_body))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Vertex shader failed to compile." << tcu::TestLog::EndMessage;

		goto end;
	}

	/* Configure XFB */
	releaseXFBVaryingNames();
	generateXFBVaryingNames(variables);

	gl.transformFeedbackVaryings(m_po_id, m_n_xfb_varyings, m_xfb_varyings, GL_INTERLEAVED_ATTRIBS);

	gl.linkProgram(m_po_id);

	/* Have we succeeded? */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Either glTransformFeedbackVaryings() or glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "A valid program object failed to link."
						   << tcu::TestLog::EndMessage;

		goto end;
	}

	/* Retrieve attribute locations *if* GL_ARB_vertex_attrib_64bit is supported */
	if (m_are_double_inputs_supported)
	{
		const size_t n_variables = variables.size();

		for (size_t n_variable = 0; n_variable < n_variables; ++n_variable)
		{
			_variable&		  current_variable = variables[n_variable];
			std::stringstream attribute_name_sstream;

			attribute_name_sstream << "in_vs_variable" << n_variable;

			if (current_variable.array_size > 1)
			{
				attribute_name_sstream << "[0]";
			}

			current_variable.attribute_location = gl.getAttribLocation(m_po_id, attribute_name_sstream.str().c_str());

			if (current_variable.attribute_location == -1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Input double-precision attribute named ["
								   << attribute_name_sstream.str().c_str()
								   << "] is considered inactive which is invalid." << tcu::TestLog::EndMessage;

				m_has_test_passed = false;
				goto end;
			}
		} /* for (all test variables) */
	}	 /* if (m_are_double_inputs_supported) */

	m_current_fs_body = fs_body;
	m_current_gs_body = gs_body;
	m_current_tc_body = tc_body;
	m_current_te_body = te_body;
	m_current_vs_body = vs_body;

	result = true;

end:
	return result;
}

/** Updates shader object's body and then compiles the shader.
 *
 *  @param body Body to use for the shader.
 *
 *  @return true if the shader compiled successfully, false otherwise.
 **/
bool GPUShaderFP64Test7::compileShader(glw::GLint shader_id, const std::string& body)
{
	const char*			  body_raw_ptr   = body.c_str();
	glw::GLint			  compile_status = GL_FALSE;
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();

	gl.shaderSource(shader_id, 1 /* count */, &body_raw_ptr, NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	return (compile_status == GL_TRUE);
}

/** Configure storage of a buffer object used for capturing XFB data.
 *
 *  @param variables Holds descriptor for all variables used for the iteration the
 *                   BO is being configured for. Storage size will be directly related
 *                   to the number of the variables and their type.
 */
void GPUShaderFP64Test7::configureXFBBuffer(const _variables& variables)
{
	DE_ASSERT(m_n_xfb_varyings != 0);

	/* Geometry shaders outputs 4 vertices making up a triangle strip per draw call.
	 * The test only draws a single patch, and triangles are caught by transform feed-back.
	 * Let's initialize the storage, according to the list of variables that will be used
	 * for the test run.
	 */
	unsigned int bo_size = 0;

	for (_variables_const_iterator variables_iterator = variables.begin(); variables_iterator != variables.end();
		 variables_iterator++)
	{
		const _variable& variable		= *variables_iterator;
		unsigned int	 n_bytes_needed = static_cast<unsigned int>(
			Utils::getNumberOfComponentsForVariableType(variable.type) * variable.array_size * sizeof(double));

		bo_size += n_bytes_needed;
	} /* for (all variables) */

	bo_size *= 3 /* vertices per triangle */ * 2; /* triangles emitted by geometry shader */

	/* Set up the BO storage */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL /* data */, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");
}

/** Deinitializes all buffers and GL objects that may have been generated
 *  during test execution.
 **/
void GPUShaderFP64Test7::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_to_data != NULL)
	{
		delete[] m_to_data;

		m_to_data = NULL;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_xfb_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_xfb_bo_id);

		m_xfb_bo_id = 0;
	}

	if (m_xfb_varyings != DE_NULL)
	{
		releaseXFBVaryingNames();
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

/** Executes the functional part of the test (case a) from the test spec)
 *
 *  @param variables Vector of variable descriptors defining properties of
 *                   variables that should be used for the iteration.
 *
 *  @return true if the test passed, false otherwise.
 **/
bool GPUShaderFP64Test7::executeFunctionalTest(_variables& variables)
{
	bool result = true;

	/* Build the test program */
	if (!buildTestProgram(variables))
	{
		return false;
	}

	/* Set up input attributes if GL_ARB_vertex_attrib_64bit extension is supported */
	if (m_are_double_inputs_supported)
	{
		setInputAttributeValues(variables);
	}

	/* Set up buffer object to hold XFB data. The data will be used for logging purposes
	 * only, if a data mismatch is detected.
	 */
	configureXFBBuffer(variables);

	/* Issue a draw call using the test program */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.clearColor(1.0f, 1.0f, 1.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor() call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.viewport(0, /* x */
				0, /* y */
				m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");

	gl.beginTransformFeedback(GL_TRIANGLES);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
	{
		gl.drawArrays(GL_PATCHES, 0 /* first */, 4 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	/* Verify color attachment contents */
	const float epsilon = 1.0f / 255.0f;

	gl.readPixels(0 /* x */, 0 /* y */, m_to_width, m_to_height, GL_RGBA, GL_UNSIGNED_BYTE, m_to_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

	for (unsigned int y = 0; y < m_to_height; ++y)
	{
		const unsigned char* row_ptr = m_to_data + 4 /* rgba */ * m_to_width * y;

		for (unsigned int x = 0; x < m_to_width; ++x)
		{
			const unsigned char* pixel_ptr = row_ptr + 4 /* rgba */ * x;

			if (de::abs(pixel_ptr[0]) > epsilon || de::abs(pixel_ptr[1] - 255) > epsilon ||
				de::abs(pixel_ptr[2]) > epsilon || de::abs(pixel_ptr[3]) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid pixel found at (" << x << ", " << y
								   << ")"
									  "; expected:(0, 255, 0, 0), found: ("
								   << (int)pixel_ptr[0] << ", " << (int)pixel_ptr[1] << ", " << (int)pixel_ptr[2]
								   << ", " << (int)pixel_ptr[3]
								   << "), with the following variable types used as varyings:"
								   << tcu::TestLog::EndMessage;

				/* List the variable types that failed the test */
				const size_t n_variables = variables.size();

				for (size_t n_variable = 0; n_variable < n_variables; ++n_variable)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "gs_variable" << n_variable << ": "
									   << Utils::getVariableTypeString(variables[n_variable].type)
									   << " (array size:" << variables[n_variable].array_size << ")"
									   << tcu::TestLog::EndMessage;
				} /* for (all variable types) */

				/* Log the variable contents */
				logVariableContents(variables);

				/* Log shaders used for the iteration */
				m_testCtx.getLog() << tcu::TestLog::Message << "Shaders used:\n"
															   "\n"
															   "(VS):\n"
								   << m_current_vs_body.c_str() << "\n"
								   << "(TC):\n"
									  "\n"
								   << m_current_tc_body.c_str() << "\n"
																   "(TE):\n"
																   "\n"
								   << m_current_te_body.c_str() << "\n"
																   "(GS):\n"
								   << m_current_gs_body.c_str() << "\n"
																   "(FS):\n"
																   "\n"
								   << m_current_fs_body.c_str() << tcu::TestLog::EndMessage;

				result = false;

				goto end;
			}
		} /* for (all columns) */
	}	 /* for (all rows) */

/* All done! */
end:
	return result;
}

/** Takes user-input vector of test variables and allocates & fills an array of strings
 *  holding names of geometry shader stage varyings that should be captured during
 *  transform feedback operation. The array will be stored in m_xfb_varyings.
 *
 *  @param variables Holds all test variable descriptors to be used for the iteration.
 */
void GPUShaderFP64Test7::generateXFBVaryingNames(const _variables& variables)
{
	unsigned int n_variable = 0;
	unsigned int n_varying  = 0;
	unsigned int n_varyings = 0;

	if (m_xfb_varyings != NULL)
	{
		releaseXFBVaryingNames();
	}

	for (_variables_const_iterator variables_iterator = variables.begin(); variables_iterator != variables.end();
		 ++variables_iterator)
	{
		const _variable& variable = *variables_iterator;

		n_varyings += variable.array_size;
	}

	m_xfb_varyings = new glw::GLchar*[n_varyings];

	for (_variables_const_iterator variables_iterator = variables.begin(); variables_iterator != variables.end();
		 ++variables_iterator, ++n_variable)
	{
		const _variable& variable = *variables_iterator;

		for (unsigned int array_index = 0; array_index < variable.array_size; ++array_index, ++n_varying)
		{
			std::stringstream varying_sstream;
			size_t			  varying_length;

			varying_sstream << "gs_variable" << n_variable;

			if (variable.array_size > 1)
			{
				varying_sstream << "[" << array_index << "]";
			}

			/* Store the varying name */
			varying_length			  = varying_sstream.str().length();
			m_xfb_varyings[n_varying] = new glw::GLchar[varying_length + 1 /* terminator */];

			memcpy(m_xfb_varyings[n_varying], varying_sstream.str().c_str(), varying_length);
			m_xfb_varyings[n_varying][varying_length] = 0;
		} /* for (all array indices) */
	}	 /* for (all varyings) */

	m_n_xfb_varyings = n_varyings;
}

/** Retrieves body of a shader that defines input variable of user-specified type & array size
 *  without using the "flat" keyword. (case c) )
 *
 *  @param input_variable_type Variable type to use for input variable declaration.
 *  @param array_size          1 if the variable should not be arrayed; otherwise defines size
 *                             of the arrayed variable.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test7::getCodeOfFragmentShaderWithNonFlatDoublePrecisionInput(
	Utils::_variable_type input_variable_type, unsigned int array_size)
{
	std::stringstream result_sstream;
	std::stringstream array_index_stringstream;
	std::stringstream array_size_stringstream;

	if (array_size > 1)
	{
		array_index_stringstream << "[0]";
		array_size_stringstream << "[" << array_size << "]";
	}

	if (Utils::isMatrixVariableType(input_variable_type))
	{
		array_index_stringstream << "[0].x";
	}
	else if (Utils::getNumberOfComponentsForVariableType(input_variable_type) > 1)
	{
		array_index_stringstream << "[0]";
	}

	result_sstream << "#version 400\n"
					  "\n"
					  "in "
				   << Utils::getVariableTypeString(input_variable_type) << " test_input"
				   << array_size_stringstream.str() << ";\n"
													   "\n"
													   "out float test_output;\n"
													   "\n"
													   "void main()\n"
													   "{\n"
													   "    if (test_input"
				   << array_index_stringstream.str() << " > 2.0)\n"
														"    {\n"
														"        test_output = 1.0;\n"
														"    }\n"
														"    else\n"
														"    {\n"
														"        test_output = 3.0;\n"
														"    }\n"
														"}\n";

	return result_sstream.str();
}

/** Retrieves body of a shader that defines double-precision floating-point output variable. (case b) ).
 *
 *  @param input_variable_type Variable type to use for input variable declaration.
 *  @param array_size          1 if the variable should not be arrayed; otherwise defines size
 *                             of the arrayed variable.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test7::getCodeOfFragmentShaderWithDoublePrecisionOutput(
	Utils::_variable_type output_variable_type, unsigned int array_size)
{
	std::stringstream array_index_sstream;
	std::stringstream array_size_sstream;
	std::stringstream result_sstream;
	std::string		  output_variable_type_string = Utils::getVariableTypeString(output_variable_type);

	if (array_size > 1)
	{
		array_index_sstream << "[0]";
		array_size_sstream << "[" << array_size << "]";
	}

	result_sstream << "#version 400\n"
					  "\n"
					  "out "
				   << output_variable_type_string << " test_output" << array_size_sstream.str() << ";\n"
																								   "\n"
																								   "void main()\n"
																								   "{\n"
																								   "    test_output"
				   << array_index_sstream.str() << " = " << output_variable_type_string << "(2.0);\n"
																						   "}\n";

	return result_sstream.str();
}

/** Retrieves body of a fragment shader that uses user-specified set of variables
 *  to declare contents of input & output block.
 *
 *  @param variables As per description.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test7::getFragmentShaderBody(const _variables& variables)
{
	std::stringstream result_sstream;

	/* Form the pre-amble */
	result_sstream << "#version 400\n"
					  "\n"

				   /* Add input block */
				   << "in GS_DATA\n"
					  "{\n"
				   << getVariableDeclarations("gs", variables, "flat") << "};\n"
																		  "\n"

				   /* Add output variable */
				   << "out vec4 result;\n"
					  "\n"

					  /* Add main() definition */
					  "void main()\n"
					  "{\n"
					  "const double epsilon = 1e-5;\n"
					  "\n"
					  "result = vec4(1, 0, 0, 0);\n"
					  "\n";

	/* Determine expected values first */
	unsigned int base_counter = 1;
	const size_t n_variables  = variables.size();

	for (unsigned int n_variable = 0; n_variable < n_variables; ++n_variable)
	{
		unsigned int		  variable_array_size		 = variables[n_variable].array_size;
		Utils::_variable_type variable_type				 = variables[n_variable].type;
		unsigned int		  n_variable_type_components = Utils::getNumberOfComponentsForVariableType(variable_type);
		std::string			  variable_type_string		 = Utils::getVariableTypeString(variable_type);

		std::stringstream array_size_sstream;

		if (variable_array_size > 1)
		{
			array_size_sstream << "[" << variable_array_size << "]";
		}

		/* Local variable declaration */
		result_sstream << variable_type_string << " expected_variable" << n_variable << array_size_sstream.str()
					   << ";\n"
						  "\n";

		/* Set expected values */
		for (unsigned int index = 0; index < variable_array_size; ++index)
		{
			std::stringstream array_index_sstream;

			if (variable_array_size > 1)
			{
				array_index_sstream << "[" << index << "]";
			}

			result_sstream << "expected_variable" << n_variable << array_index_sstream.str() << " = "
						   << variable_type_string << "(";

			for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
			{
				unsigned int expected_value =
					(base_counter + 0) + (base_counter + 1) + (base_counter + 2) + (base_counter + 3);

				if (m_are_double_inputs_supported)
				{
					/* VS input attributes */
					//expected_value += (base_counter + 6);
					expected_value -= 1;
				}

				result_sstream << expected_value;

				if (n_component != (n_variable_type_components - 1))
				{
					result_sstream << ", ";
				}

				++base_counter;
			} /* for (all components) */

			result_sstream << ");\n";
		} /* for (all array indices) */

		result_sstream << "\n";
	} /* for (all variable types) */

	/* Now that we know the expected values, do a huge conditional check to verify if all
	 * input variables carry correct information.
	 */
	result_sstream << "if (";

	for (unsigned int n_variable = 0; n_variable < n_variables; ++n_variable)
	{
		unsigned int		  variable_array_size		 = variables[n_variable].array_size;
		Utils::_variable_type variable_type				 = variables[n_variable].type;
		bool				  is_variable_type_matrix	= Utils::isMatrixVariableType(variable_type);
		unsigned int		  n_variable_type_components = Utils::getNumberOfComponentsForVariableType(variable_type);
		std::string			  variable_type_string		 = Utils::getVariableTypeString(variable_type);

		for (unsigned int index = 0; index < variable_array_size; ++index)
		{
			std::stringstream array_index_sstream;

			if (variable_array_size > 1)
			{
				array_index_sstream << "[" << index << "]";
			}

			for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
			{
				std::stringstream component_index_sstream;

				if (n_variable_type_components > 1)
				{
					component_index_sstream << "[" << n_component << "]";
				}

				result_sstream << "abs(expected_variable" << n_variable << array_index_sstream.str();

				if (is_variable_type_matrix)
				{
					const unsigned int n_columns = Utils::getNumberOfColumnsForVariableType(variable_type);
					const unsigned int column	= n_component % n_columns;
					const unsigned int row		 = n_component / n_columns;

					result_sstream << "[" << column << "]"
													   "."
								   << Utils::getComponentAtIndex(row);
				}
				else
				{
					result_sstream << component_index_sstream.str();
				}

				result_sstream << " - gs_variable" << n_variable << array_index_sstream.str();

				if (is_variable_type_matrix)
				{
					const unsigned int n_columns = Utils::getNumberOfColumnsForVariableType(variable_type);
					const unsigned int column	= n_component % n_columns;
					const unsigned int row		 = n_component / n_columns;

					result_sstream << "[" << column << "]"
													   "."
								   << Utils::getComponentAtIndex(row);
				}
				else
				{
					result_sstream << component_index_sstream.str();
				}

				result_sstream << ") <= epsilon &&";
			} /* for (all components) */
		}	 /* for (all array indices) */
	}		  /* for (all variable types) */

	result_sstream << "true)\n"
					  "{\n"
					  "    result = vec4(0, 1, 0, 0);\n"
					  "}\n"
					  "}\n";

	/* All done */
	return result_sstream.str();
}

/** Retrieves body of a geometry shader that uses user-specified set of variables
 *  to declare contents of input & output block.
 *
 *  @param variables As per description.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test7::getGeometryShaderBody(const _variables& variables)
{
	std::stringstream result_sstream;

	/* Form the pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "layout(triangles)                      in;\n"
					  "layout(triangle_strip, max_vertices=4) out;\n"
					  "\n"

					  /* Add the input block */
					  "in TE_DATA\n"
					  "{\n"
				   << getVariableDeclarations("te", variables) << "} in_data[];\n"
																  "\n"

																  /* Add the output block */
																  "out GS_DATA\n"
																  "{\n"
				   << getVariableDeclarations("gs", variables, "flat") << "};\n"
																		  "\n"

																		  /* Declare main() function */
																		  "void main()\n"
																		  "{\n";

	/* Take input variables, add a predefined value and forward them to output variables */
	const float quad_vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
									1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f };
	const unsigned int n_quad_vertices =
		sizeof(quad_vertices) / sizeof(quad_vertices[0]) / 4 /* components per vertex */;
	const size_t n_variables = variables.size();

	for (unsigned int n_quad_vertex = 0; n_quad_vertex < n_quad_vertices; ++n_quad_vertex)
	{
		unsigned int counter			 = 4;
		const float* current_quad_vertex = quad_vertices + n_quad_vertex * 4 /* components per vertex */;

		for (unsigned int n_variable = 0; n_variable < n_variables; ++n_variable)
		{
			unsigned int		  variable_array_size = variables[n_variable].array_size;
			Utils::_variable_type variable_type		  = variables[n_variable].type;
			unsigned int n_variable_type_components   = Utils::getNumberOfComponentsForVariableType(variable_type);
			std::string  variable_type_string		  = Utils::getVariableTypeString(variable_type);

			for (unsigned int index = 0; index < variable_array_size; ++index)
			{
				std::stringstream array_index_sstream;

				if (variable_array_size > 1)
				{
					array_index_sstream << "[" << index << "]";
				}

				result_sstream << "gs_variable" << n_variable << array_index_sstream.str()
							   << " = in_data[0].te_variable" << n_variable << array_index_sstream.str() << " + "
							   << variable_type_string << "(";

				for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
				{
					result_sstream << (counter++);

					if (n_component != (n_variable_type_components - 1))
					{
						result_sstream << ", ";
					}
				} /* for (all components) */

				result_sstream << ");\n";
			} /* for (all array indices) */
		}	 /* for (all variable types) */

		result_sstream << "gl_Position = vec4(" << current_quad_vertex[0] << ", " << current_quad_vertex[1] << ", "
					   << current_quad_vertex[2] << ", " << current_quad_vertex[3] << ");\n"
																					  "EmitVertex();\n";
	} /* for (all emitted quad vertices) */

	result_sstream << "EndPrimitive();\n"
					  "}\n";

	/* All done */
	return result_sstream.str();
}

/** Retrieves body of a tessellation control shader that uses user-specified set of variables
 *  to declare contents of input & output block.
 *
 *  @param variables As per description.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test7::getTessellationControlShaderBody(const _variables& variables)
{
	std::stringstream result_sstream;

	/* Form the pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "layout (vertices=4) out;\n"

					  /* Declare input block */
					  "in VS_DATA\n"
					  "{\n"
				   << getVariableDeclarations("vs", variables) << "} in_data[];\n"

																  /* Declare output block */
																  "out TC_DATA\n"
																  "{\n"
				   << getVariableDeclarations("tc", variables) << "} out_data[];\n"
																  "\n"

																  /* Define main() */
																  "void main()\n"
																  "{\n"
																  "    gl_TessLevelInner[0] = 1;\n"
																  "    gl_TessLevelInner[1] = 1;\n"
																  "    gl_TessLevelOuter[0] = 1;\n"
																  "    gl_TessLevelOuter[1] = 1;\n"
																  "    gl_TessLevelOuter[2] = 1;\n"
																  "    gl_TessLevelOuter[3] = 1;\n"
																  "\n";

	/* Take input variables, add a predefined value and forward them to output variables */
	const size_t n_variables = variables.size();
	unsigned int counter	 = 2;

	for (unsigned int n_variable = 0; n_variable < n_variables; ++n_variable)
	{
		unsigned int		  variable_array_size		 = variables[n_variable].array_size;
		Utils::_variable_type variable_type				 = variables[n_variable].type;
		unsigned int		  n_variable_type_components = Utils::getNumberOfComponentsForVariableType(variable_type);
		std::string			  variable_type_string		 = Utils::getVariableTypeString(variable_type);

		for (unsigned int index = 0; index < variable_array_size; ++index)
		{
			std::stringstream array_index_sstream;

			if (variable_array_size > 1)
			{
				array_index_sstream << "[" << index << "]";
			}

			result_sstream << "out_data[gl_InvocationID].tc_variable" << n_variable << array_index_sstream.str()
						   << " = in_data[0].vs_variable" << n_variable << array_index_sstream.str() << " + "
						   << variable_type_string << "(";

			for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
			{
				result_sstream << (counter++);

				if (n_component != (n_variable_type_components - 1))
				{
					result_sstream << ", ";
				}
			}

			result_sstream << ");\n";
		} /* for (all array indices) */
	}	 /* for (all variable types) */

	result_sstream << "}\n";

	/* We're done */
	return result_sstream.str();
}

/** Retrieves body of a tessellation evaluation shader that uses user-specified set of variables
 *  to declare contents of input & output block.
 *
 *  @param variables As per description.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test7::getTessellationEvaluationShaderBody(const _variables& variables)
{
	std::stringstream result_sstream;

	/* Form the pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "layout(quads) in;\n"
					  "\n"

					  /* Define input block */
					  "in TC_DATA\n"
					  "{\n"
				   << getVariableDeclarations("tc", variables) << "} in_data[];\n"
																  "\n"

																  /* Define output block */
																  "out TE_DATA\n"
																  "{\n"
				   << getVariableDeclarations("te", variables) << "};\n"
																  "\n"

																  /* Define main() */
																  "void main()\n"
																  "{\n";

	/* Take input variables, add a predefined value and forward them to output variables */
	const size_t n_variables = variables.size();
	unsigned int counter	 = 3;

	for (unsigned int n_variable = 0; n_variable < n_variables; ++n_variable)
	{
		unsigned int		  variable_array_size		 = variables[n_variable].array_size;
		Utils::_variable_type variable_type				 = variables[n_variable].type;
		unsigned int		  n_variable_type_components = Utils::getNumberOfComponentsForVariableType(variable_type);
		std::string			  variable_type_string		 = Utils::getVariableTypeString(variable_type);

		for (unsigned int index = 0; index < variable_array_size; ++index)
		{
			std::stringstream array_index_sstream;

			if (variable_array_size > 1)
			{
				array_index_sstream << "[" << index << "]";
			}

			result_sstream << "te_variable" << n_variable << array_index_sstream.str() << " = in_data[0].tc_variable"
						   << n_variable << array_index_sstream.str() << " + " << variable_type_string << "(";

			for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
			{
				result_sstream << (counter++);

				if (n_component != (n_variable_type_components - 1))
				{
					result_sstream << ", ";
				}
			} /* for (all components) */

			result_sstream << ");\n";
		} /* for (all array indices) */
	}	 /* for (all variable types) */

	result_sstream << "}\n";

	/* All done */
	return result_sstream.str();
}

/** Returns a string containing declarations of user-specified set of variables.
 *  Each declaration can optionally use a layot qualifier requested by the caller.
 *
 *  @param prefix             Prefix to use for variable names.
 *  @param variables          List of variables to declare in the result string.
 *  @param explicit_locations true if each declaration should explicitly define location
 *                            of the variable ( eg. (layout location=X) )
 *  @param layout_qualifier   Optional qualifier to use for the declaration. Must not
 *                            be NULL.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test7::getVariableDeclarations(const char* prefix, const _variables& variables,
														const char* layout_qualifier)
{
	std::stringstream result_sstream;

	/* Define output variables */
	const size_t n_variables = variables.size();

	for (unsigned int n_variable = 0; n_variable < n_variables; ++n_variable)
	{
		unsigned int		  variable_array_size  = variables[n_variable].array_size;
		Utils::_variable_type variable_type		   = variables[n_variable].type;
		std::string			  variable_type_string = Utils::getVariableTypeString(variable_type);

		result_sstream << layout_qualifier << " " << variable_type_string << " " << prefix << "_variable" << n_variable;

		if (variable_array_size > 1)
		{
			result_sstream << "[" << variable_array_size << "]";
		}

		result_sstream << ";\n";
	} /* for (all user-specified variable types) */

	return result_sstream.str();
}

/** Retrieves body of a vertex shader that uses user-specified set of variables
 *  to declare contents of input & output block.
 *
 *  @param variables As per description.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test7::getVertexShaderBody(const _variables& variables)
{
	std::stringstream result_sstream;

	/* Form pre-amble */
	result_sstream << "#version 400\n"
					  "\n";

	/* Define input variables if GL_ARB_vertex_attrib_64bit is supported */
	if (m_are_double_inputs_supported)
	{
		result_sstream << "#extension GL_ARB_vertex_attrib_64bit : require\n"
					   << getVariableDeclarations("in_vs", variables, "in");
	}

	/* Define output variables */
	result_sstream << "out VS_DATA\n"
					  "{\n"
				   << getVariableDeclarations("vs", variables);

	/* Define main() */
	result_sstream << "};\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	/* Set output variable values */
	unsigned int counter	 = 1;
	const size_t n_variables = variables.size();

	for (unsigned int n_variable = 0; n_variable < n_variables; ++n_variable)
	{
		unsigned int		  variable_array_size		 = variables[n_variable].array_size;
		Utils::_variable_type variable_type				 = variables[n_variable].type;
		const unsigned int	n_variable_type_components = Utils::getNumberOfComponentsForVariableType(variable_type);
		std::string			  variable_type_string		 = Utils::getVariableTypeString(variable_type);

		for (unsigned int index = 0; index < variable_array_size; ++index)
		{
			if (variable_array_size == 1)
			{
				result_sstream << "vs_variable" << n_variable << " = " << variable_type_string << "(";
			}
			else
			{
				result_sstream << "vs_variable" << n_variable << "[" << index << "]"
							   << " = " << variable_type_string << "(";
			}

			for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
			{
				result_sstream << (double)(counter++);

				/* Use input attributes, if available */
				if (m_are_double_inputs_supported)
				{
					result_sstream << " + in_vs_variable" << n_variable;

					if (variable_array_size > 1)
					{
						result_sstream << "[" << index << "]";
					}

					if (Utils::isMatrixVariableType(variables[n_variable].type))
					{
						const unsigned int n_columns = Utils::getNumberOfColumnsForVariableType(variable_type);
						const unsigned int column	= n_component % n_columns;
						const unsigned int row		 = n_component / n_columns;

						result_sstream << "[" << (column) << "]"
															 "."
									   << Utils::getComponentAtIndex(row);
					}
					else if (n_variable_type_components > 1)
					{
						result_sstream << "[" << n_component << "]";
					}
				}

				if (n_component != (n_variable_type_components - 1))
				{
					result_sstream << ", ";
				}
			} /* for (all components) */

			result_sstream << ");\n";
		}
	} /* for (all variable types) */

	/* We will be using geometry shader to lay out the actual vertices so
	 * the only thing we need to make sure is that the vertex never gets
	 * culled.
	 */
	result_sstream << "gl_Position = vec4(0, 0, 0, 1);\n"
					  "}\n";

	/* That's it */
	return result_sstream.str();
}

/** Initializes shader objects required to run the test. */
void GPUShaderFP64Test7::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Are double-precision input variables supported? */
	m_are_double_inputs_supported = m_context.getContextInfo().isExtensionSupported("GL_ARB_vertex_attrib_64bit");

	/* Create a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Create a texture object we will use as FBO's color attachment */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Allocate temporary buffer to hold the texture data we will be reading
	 * from color attachment. */
	m_to_data = new unsigned char[m_to_width * m_to_height * 4 /* RGBA */];

	/* Create and set up a framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindframebuffer() call failed.");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	/* Create all shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
	m_te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Create test program object */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Attach the shaders to the program object */
	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_gs_id);
	gl.attachShader(m_po_id, m_tc_id);
	gl.attachShader(m_po_id, m_te_id);
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	/* The test passes double-precision values through the whole rendering pipeline.
	 * This translates to a notable amount of components that we would need to transfer
	 * all values in one fell swoop. The number is large enough to exceed minimum
	 * capabilities as described for OpenGL 4.0 implementations.
	 * For that reason, the test executes in turns. Each turn is allocated as many
	 * double-precision scalar/matrix values as supported by the tested GL implementation.
	 */
	glw::GLint gl_max_fragment_input_components_value				  = 0;
	glw::GLint gl_max_geometry_input_components_value				  = 0;
	glw::GLint gl_max_geometry_output_components_value				  = 0;
	glw::GLint gl_max_tess_control_input_components_value			  = 0;
	glw::GLint gl_max_tess_control_output_components_value			  = 0;
	glw::GLint gl_max_tess_evaluation_input_components_value		  = 0;
	glw::GLint gl_max_tess_evaluation_output_components_value		  = 0;
	glw::GLint gl_max_transform_feedback_interleaved_components_value = 0;
	glw::GLint gl_max_vertex_output_components_value				  = 0;

	gl.getIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &gl_max_fragment_input_components_value);
	gl.getIntegerv(GL_MAX_GEOMETRY_INPUT_COMPONENTS, &gl_max_geometry_input_components_value);
	gl.getIntegerv(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, &gl_max_geometry_output_components_value);
	gl.getIntegerv(GL_MAX_TESS_CONTROL_INPUT_COMPONENTS, &gl_max_tess_control_input_components_value);
	gl.getIntegerv(GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS, &gl_max_tess_control_output_components_value);
	gl.getIntegerv(GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS, &gl_max_tess_evaluation_input_components_value);
	gl.getIntegerv(GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, &gl_max_tess_evaluation_output_components_value);
	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,
				   &gl_max_transform_feedback_interleaved_components_value);
	gl.getIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &gl_max_vertex_output_components_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetintegerv() call(s) failed.");

	m_n_max_components_per_stage =
		de::min(gl_max_vertex_output_components_value, gl_max_tess_control_input_components_value);
	m_n_max_components_per_stage = de::min(m_n_max_components_per_stage, gl_max_fragment_input_components_value);
	m_n_max_components_per_stage = de::min(m_n_max_components_per_stage, gl_max_geometry_input_components_value);
	m_n_max_components_per_stage = de::min(m_n_max_components_per_stage, gl_max_geometry_output_components_value);
	m_n_max_components_per_stage = de::min(m_n_max_components_per_stage, gl_max_tess_control_output_components_value);
	m_n_max_components_per_stage = de::min(m_n_max_components_per_stage, gl_max_tess_evaluation_input_components_value);
	m_n_max_components_per_stage =
		de::min(m_n_max_components_per_stage, gl_max_tess_evaluation_output_components_value);
	m_n_max_components_per_stage =
		de::min(m_n_max_components_per_stage, gl_max_transform_feedback_interleaved_components_value);

	/* Update GL_PATCH_VERTICES setting so that we only use a single vertex to build
	 * the input patch */
	gl.patchParameteri(GL_PATCH_VERTICES, 1);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteri() call failed.");

	/* Initialize a BO we will use to hold XFB data */
	gl.genBuffers(1, &m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GPUShaderFP64Test7::iterate()
{
	/* Do not execute the test if GL_ARB_texture_view is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported.");
	}

	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_vertex_attrib_64bit"))
	{
		throw tcu::NotSupportedError("GL_ARB_vertex_attrib_64bit is not supported.");
	}

	/* Initialize GL objects required to run the test */
	initTest();

	/* Check the negative cases first */
	const Utils::_variable_type double_variable_types[] = {
		Utils::VARIABLE_TYPE_DOUBLE,  Utils::VARIABLE_TYPE_DVEC2, Utils::VARIABLE_TYPE_DVEC3,
		Utils::VARIABLE_TYPE_DVEC4,   Utils::VARIABLE_TYPE_DMAT2, Utils::VARIABLE_TYPE_DMAT2X3,
		Utils::VARIABLE_TYPE_DMAT2X4, Utils::VARIABLE_TYPE_DMAT3, Utils::VARIABLE_TYPE_DMAT3X2,
		Utils::VARIABLE_TYPE_DMAT3X4, Utils::VARIABLE_TYPE_DMAT4, Utils::VARIABLE_TYPE_DMAT4X2,
		Utils::VARIABLE_TYPE_DMAT4X3,
	};
	const unsigned int n_double_variable_types = sizeof(double_variable_types) / sizeof(double_variable_types[0]);

	for (unsigned int n_double_variable_type = 0; n_double_variable_type < n_double_variable_types;
		 ++n_double_variable_type)
	{
		for (unsigned int array_size = 1; array_size < 3; ++array_size)
		{
			Utils::_variable_type variable_type = double_variable_types[n_double_variable_type];

			if (compileShader(m_fs_id, getCodeOfFragmentShaderWithDoublePrecisionOutput(variable_type, array_size)))
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "A fragment shader with double-precision output variable compiled successfully."
								   << tcu::TestLog::EndMessage;

				m_has_test_passed = false;
			}

			if (compileShader(m_fs_id,
							  getCodeOfFragmentShaderWithNonFlatDoublePrecisionInput(variable_type, array_size)))
			{
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "A fragment shader with double-precision input variables lacking flat layout qualifier"
					   " compiled successfully."
					<< tcu::TestLog::EndMessage;

				m_has_test_passed = false;
			}
		}
	} /* for (all variable types) */

	/* Execute functional test. Split the run into as many iterations as necessary
	 * so that we do not exceed GL implementation's capabilities. */
	unsigned int n_tested_variables = 0;
	_variables   variables_to_test;

	while (n_tested_variables != n_double_variable_types * 2 /* arrayed & non-arrayed */)
	{
		glw::GLint total_n_used_components = 0;

		/* Use as many variables as possible for the iterations. Do not exceed maximum amount
		 * of varying components that can be used for all shadr stages.
		 */
		while (total_n_used_components < m_n_max_components_per_stage &&
			   n_tested_variables != n_double_variable_types * 2 /* arrayed & non-arrayed */)
		{
			_variable	new_variable;
			unsigned int n_type_components = 0;
			glw::GLint   n_used_components = 0;

			new_variable.array_size =
				((n_tested_variables % 2) == 0) ? 1 /* non-arrayed variable */ : 2; /* arrayed variable */
			new_variable.type = double_variable_types[n_tested_variables / 2];

			/* Double-precision varyings can use twice as many components as single-precision FPs */
			n_type_components = 4 /* components per location */ *
								Utils::getNumberOfLocationsUsedByDoublePrecisionVariableType(new_variable.type);
			n_used_components = n_type_components * new_variable.array_size * 2;

			/* Do we have enough space? */
			if (total_n_used_components + n_used_components > m_n_max_components_per_stage)
			{
				if (n_used_components > m_n_max_components_per_stage)
				{ //if the number of components for this variable is larger than the max_components_per_stage, then skip it.
					n_tested_variables++;
				}
				break;
			}

			/* We can safely test the type in current iteration */
			total_n_used_components += n_used_components;
			n_tested_variables++;

			variables_to_test.push_back(new_variable);
		}

		if (variables_to_test.size() > 0)
		{
			m_has_test_passed &= executeFunctionalTest(variables_to_test);

			variables_to_test.clear();
		}
	}

	/* We're done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Logs contents of test variables, as XFBed out by already executed test iteration. */
void GPUShaderFP64Test7::logVariableContents(const _variables& variables)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	std::stringstream	 log_sstream;

	log_sstream << "Test variable values as retrieved from geometry shader:\n";

	/* Map the XFB BO contents into process space */
	const void* xfb_bo_data = gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

	/* Read the variable contents. We only care about the set of varyings emitted
	 * for first vertex in the geometry shader */
	unsigned int		 n_varying	 = 0;
	const unsigned char* traveller_ptr = (const unsigned char*)xfb_bo_data;

	for (_variables_const_iterator variables_iterator = variables.begin(); variables_iterator != variables.end();
		 ++variables_iterator, ++n_varying)
	{
		const _variable&			 variable			= *variables_iterator;
		const Utils::_variable_type& base_variable_type = Utils::getBaseVariableType(variable.type);
		const unsigned int			 n_components		= Utils::getNumberOfComponentsForVariableType(variable.type);

		for (unsigned int array_index = 0; array_index < variable.array_size; ++array_index)
		{
			log_sstream << "gs_variable" << n_varying;

			if (variable.array_size > 1)
			{
				log_sstream << "[" << array_index << "]";
			}

			log_sstream << ": (";

			for (unsigned int n_component = 0; n_component < n_components; ++n_component)
			{
				log_sstream << Utils::getStringForVariableTypeValue(base_variable_type, traveller_ptr);

				if (n_component != (n_components - 1))
				{
					log_sstream << ", ";
				}

				traveller_ptr += sizeof(double);
			}

			log_sstream << ")\n";
		} /* for (all array indices) */
	}	 /* for (all variables) */

	/* Unmap the BO */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	/* Pass the logged stream into the framework */
	m_testCtx.getLog() << tcu::TestLog::Message << log_sstream.str().c_str() << tcu::TestLog::EndMessage;
}

/** De-allocates an arary holding strings representing names of varyings that
 *  should be used for transform feed-back.
 **/
void GPUShaderFP64Test7::releaseXFBVaryingNames()
{
	for (unsigned int n_varying = 0; n_varying < m_n_xfb_varyings; ++n_varying)
	{
		delete[] m_xfb_varyings[n_varying];
	}

	delete m_xfb_varyings;
	m_xfb_varyings = DE_NULL;

	m_n_xfb_varyings = 0;
}

/** This function should only be called if GL_ARB_vertex_attrib_64bit extension is supported.
 *  Takes a list of test variables used for current iteration and assigns increasing values
 *  to subsequent input attributes of the test program.
 *
 *  @param variables Test variables of the current iteration.
 */
void GPUShaderFP64Test7::setInputAttributeValues(const _variables& variables)
{
	const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();
	unsigned int		  counter = 6;

	for (_variables_const_iterator variable_iterator = variables.begin(); variable_iterator != variables.end();
		 variable_iterator++)
	{
		const _variable&   variable			  = *variable_iterator;
		const bool		   is_matrix_type	 = Utils::isMatrixVariableType(variable.type);
		const unsigned int n_total_components = Utils::getNumberOfComponentsForVariableType(variable.type);
		unsigned int	   n_components		  = 0;
		unsigned int	   n_columns		  = 1;

		if (is_matrix_type)
		{
			n_columns	= Utils::getNumberOfColumnsForVariableType(variable.type);
			n_components = n_total_components / n_columns;

			DE_ASSERT(n_total_components % n_columns == 0);
		}
		else
		{
			n_components = n_total_components;
		}

		DE_ASSERT(n_components >= 1 && n_components <= 4);

		for (unsigned int index = 0; index < n_columns * variable.array_size; ++index)
		{
			const double data[] = { -1, -1, -1, -1 };

			switch (n_components)
			{
			case 1:
			{
				gl.vertexAttribL1dv(variable.attribute_location + index, data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttrib1dv() call failed.");

				break;
			}

			case 2:
			{
				gl.vertexAttribL2dv(variable.attribute_location + index, data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttrib2dv() call failed.");

				break;
			}

			case 3:
			{
				gl.vertexAttribL3dv(variable.attribute_location + index, data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttrib3dv() call failed.");

				break;
			}

			case 4:
			{
				gl.vertexAttribL4dv(variable.attribute_location + index, data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttrib4dv() call failed.");

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized number of components");
			}
			} /* switch (n_components) */

			/* Make sure VAAs are disabled */
			gl.disableVertexAttribArray(variable.attribute_location + index);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray() call failed.");

			counter += n_components;
		} /* for (all array indices) */
	}	 /* for (all variables) */
}

/** Constructor
 *
 *  @param context Rendering context.
 */
GPUShaderFP64Test8::GPUShaderFP64Test8(deqp::Context& context)
	: TestCase(context, "valid_constructors", "Verifies that valid double-precision floating-point constructors "
											  "are accepted during compilation stage")
	, m_cs_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
	, m_has_test_passed(true)
{
}

/** Deinitializes all buffers and GL objects that may have been generated
 *  during test execution.
 **/
void GPUShaderFP64Test8::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_cs_id != 0)
	{
		gl.deleteShader(m_cs_id);

		m_cs_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test case.
 *
 *  This function can throw TestError exceptions if GL implementation reports
 *  an error.
 *
 *  @param test_case Test case descriptor.
 *
 *  @return true if test case passed, false otherwise.
 **/
bool GPUShaderFP64Test8::executeIteration(const _test_case& test_case)
{
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();
	const glw::GLuint	 so_ids[]   = { m_cs_id, m_fs_id, m_gs_id, m_tc_id, m_te_id, m_vs_id };
	const unsigned int	n_so_ids   = sizeof(so_ids) / sizeof(so_ids[0]);
	bool				  result	 = true;
	const char*			  stage_body = NULL;
	const char*			  stage_name = NULL;

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		const glw::GLuint so_id = so_ids[n_so_id];

		/* Skip compute shader if it is not supported */
		if (0 == so_id)
		{
			continue;
		}

		/* Compile the shader */
		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		/* Has the compilation succeeded as expected? */
		glw::GLint compile_status = GL_FALSE;

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		if (compile_status == GL_FALSE)
		{
			/* What is the current stage's name? */
			if (so_id == m_cs_id)
			{
				stage_body = test_case.cs_shader_body.c_str();
				stage_name = "Compute shader";
			}
			else if (so_id == m_fs_id)
			{
				stage_body = test_case.fs_shader_body.c_str();
				stage_name = "Fragment shader";
			}
			else if (so_id == m_gs_id)
			{
				stage_body = test_case.gs_shader_body.c_str();
				stage_name = "Geometry shader";
			}
			else if (so_id == m_tc_id)
			{
				stage_body = test_case.tc_shader_body.c_str();
				stage_name = "Tessellation control shader";
			}
			else if (so_id == m_te_id)
			{
				stage_body = test_case.te_shader_body.c_str();
				stage_name = "Tessellation evaluation shader";
			}
			else if (so_id == m_vs_id)
			{
				stage_body = test_case.vs_shader_body.c_str();
				stage_name = "Vertex shader";
			}
			else
			{
				/* Doesn't make much sense to throw exceptions here so.. */
				stage_body = "";
				stage_name = "[?]";
			}

			/* This shader should have never failed to compile! */
			m_testCtx.getLog() << tcu::TestLog::Message << stage_name
							   << " has not compiled successfully, even though the shader is valid."
								  " Following is shader's body:\n"
							   << stage_body << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all shader objects) */

	return result;
}

/** Retrieves all argument lists that can be used to initialize a variable of user-specified
 *  type.
 *
 *  @param variable_type Variable type to return valid argument lists for.
 **/
GPUShaderFP64Test8::_argument_lists GPUShaderFP64Test8::getArgumentListsForVariableType(
	const Utils::_variable_type& variable_type)
{
	const Utils::_variable_type matrix_types[] = {
		Utils::VARIABLE_TYPE_DMAT2, Utils::VARIABLE_TYPE_DMAT2X3, Utils::VARIABLE_TYPE_DMAT2X4,
		Utils::VARIABLE_TYPE_DMAT3, Utils::VARIABLE_TYPE_DMAT3X2, Utils::VARIABLE_TYPE_DMAT3X4,
		Utils::VARIABLE_TYPE_DMAT4, Utils::VARIABLE_TYPE_DMAT4X2, Utils::VARIABLE_TYPE_DMAT4X3,
	};
	const Utils::_variable_type scalar_types[] = { Utils::VARIABLE_TYPE_DOUBLE, Utils::VARIABLE_TYPE_DVEC2,
												   Utils::VARIABLE_TYPE_DVEC3, Utils::VARIABLE_TYPE_DVEC4 };
	const unsigned int n_matrix_types	 = sizeof(matrix_types) / sizeof(matrix_types[0]);
	const unsigned int n_scalar_types	 = sizeof(scalar_types) / sizeof(scalar_types[0]);
	const int		   n_total_components = (int)Utils::getNumberOfComponentsForVariableType(variable_type);

	/* Construct the argument list tree root. Each node carries a counter that tells how many components
	 * have already been assigned. Nodes that eat up all components are considered leaves and do not have
	 * any children. Otherwise, each node is assigned as many children, as there are types that could be
	 * used to define a subsequent argument, and its counter is increased by the amount of components
	 * described by the type.
	 */
	_argument_list_tree_node root;

	root.n_components_used = 0;
	root.parent			   = NULL;
	root.type			   = variable_type;

	/* Fill till all leaves use up all available components */
	_argument_list_tree_node_queue nodes_queue;

	nodes_queue.push(&root);

	do
	{
		/* Pop the first item in the queue */
		_argument_list_tree_node* current_node_ptr = nodes_queue.front();
		nodes_queue.pop();

		/* Matrix variable types can be defined by a combination of non-matrix variable types OR
		 * a single matrix variable type.
		 *
		 * Let's handle the latter case first.
		 */
		const int n_components_remaining = n_total_components - current_node_ptr->n_components_used;

		if (Utils::isMatrixVariableType(current_node_ptr->type))
		{
			/* Iterate through all known matrix types. All the types can be used
			 * as a constructor, assuming only one value is used to define new matrix's
			 * contents. */
			for (unsigned int n_matrix_type = 0; n_matrix_type < n_matrix_types; ++n_matrix_type)
			{
				Utils::_variable_type new_argument_type = matrix_types[n_matrix_type];

				/* Construct a new child node. Since GLSL spec clearly states we must not use more
				 * than one constructor argument if the only argument is a matrix type, mark the node
				 * as if it defined all available components.
				 */
				_argument_list_tree_node* new_subnode = new _argument_list_tree_node;

				new_subnode->n_components_used = n_total_components;
				new_subnode->parent			   = current_node_ptr;
				new_subnode->type			   = new_argument_type;

				/* Add the descriptor to node list but do not add it to the queue. This would be
				 * a redundant operation, since no new children nodes would have been assigned to
				 * this node anyway.
				 */
				current_node_ptr->children.push_back(new_subnode);
			} /* for (all matrix types) */
		}	 /* if (current node's type is a matrix) */

		/* Now for a combination of non-matrix variable types.. */
		if (!Utils::isMatrixVariableType(current_node_ptr->type))
		{
			/* Iterate through all known scalar types */
			for (unsigned int n_scalar_type = 0; n_scalar_type < n_scalar_types; ++n_scalar_type)
			{
				Utils::_variable_type new_argument_type = scalar_types[n_scalar_type];
				const int n_new_argument_components = Utils::getNumberOfComponentsForVariableType(new_argument_type);

				/* Only use the scalar type if we don't exceed the amount of components we can define
				 * for requested type.
				 */
				if (n_new_argument_components <= n_components_remaining)
				{
					/* Form new node descriptor */
					_argument_list_tree_node* new_subnode = new _argument_list_tree_node;

					new_subnode->n_components_used = n_new_argument_components + current_node_ptr->n_components_used;
					new_subnode->parent			   = current_node_ptr;
					new_subnode->type			   = new_argument_type;

					current_node_ptr->children.push_back(new_subnode);
					nodes_queue.push(new_subnode);
				}
			} /* for (all scalar types) */
		}	 /* if (!Utils::isMatrixVariableType(current_node_ptr->type) ) */
	} while (nodes_queue.size() > 0);

	/* To construct the argument lists, traverse the tree. Each path from root to child
	 * gives us a single argument list.
	 *
	 * First, identify leaf nodes.
	 */
	_argument_list_tree_nodes leaf_nodes;

	nodes_queue.push(&root);

	do
	{
		_argument_list_tree_node* current_node_ptr = nodes_queue.front();
		nodes_queue.pop();

		if (current_node_ptr->children.size() == 0)
		{
			/* This is a leaf node !*/
			leaf_nodes.push_back(current_node_ptr);
		}
		else
		{
			/* Throw all children nodes to the queue */
			const unsigned int n_children_nodes = (const unsigned int)current_node_ptr->children.size();

			for (unsigned int n_children_node = 0; n_children_node < n_children_nodes; ++n_children_node)
			{
				nodes_queue.push(current_node_ptr->children[n_children_node]);
			} /* for (all children nodes) */
		}
	} while (nodes_queue.size() > 0);

	/* For all leaf nodes, move up the tree and construct the argument lists. */
	const unsigned int n_leaf_nodes = (const unsigned int)leaf_nodes.size();
	_argument_lists	result;

	for (unsigned int n_leaf_node = 0; n_leaf_node < n_leaf_nodes; ++n_leaf_node)
	{
		_argument_list			  argument_list;
		_argument_list_tree_node* current_node_ptr = leaf_nodes[n_leaf_node];

		do
		{
			if (current_node_ptr != &root)
			{
				if (argument_list.size() == 0)
				{
					argument_list.push_back(current_node_ptr->type);
				}
				else
				{
					argument_list.insert(argument_list.begin(), current_node_ptr->type);
				}
			}

			current_node_ptr = current_node_ptr->parent;
		} while (current_node_ptr != NULL);

		result.push_back(argument_list);
	} /* for (all leaf nodes) */

	return result;
}

/** Retrieves body of a compute shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test8::getComputeShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form the body */
	result_sstream << "#version 420\n"
					  "#extension GL_ARB_compute_shader          : require\n"
					  "\n"
					  "layout(local_size_x = 1) in;\n"
					  "\n"
					  "void main()\n"
					  "{\n"
				   << getGeneralBody(test_case) << "}\n";

	/* Return the body */
	return result_sstream.str();
}

/** Retrieves body of a fragment shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test8::getFragmentShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form the body */
	result_sstream << "#version 420\n"
					  "\n"
					  "void main()\n"
					  "{\n"
				   << getGeneralBody(test_case) << "}\n"
												   "\n";

	/* Return the body */
	return result_sstream.str();
}

/** Returns a GLSL line that defines and initializes a variable as described by
 *  user-specified test case descriptor.
 *
 *  @param test_case Test case descriptor to use for the query.
 *
 *  @return As per description
 **/
std::string GPUShaderFP64Test8::getGeneralBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form the body */
	std::string variable_type_string = Utils::getVariableTypeString(test_case.type);

	result_sstream << variable_type_string << " src = " << variable_type_string << "(";

	for (_argument_list_const_iterator argument_list_iterator = test_case.argument_list.begin();
		 argument_list_iterator != test_case.argument_list.end(); argument_list_iterator++)
	{
		const Utils::_variable_type argument_variable_type = *argument_list_iterator;
		std::string		   argument_variable_type_string   = Utils::getVariableTypeString(argument_variable_type);
		const unsigned int argument_n_components = Utils::getNumberOfComponentsForVariableType(argument_variable_type);

		if (argument_list_iterator != test_case.argument_list.begin())
		{
			result_sstream << ", ";
		}

		result_sstream << argument_variable_type_string << "(";

		for (unsigned int n_component = 0; n_component < argument_n_components; ++n_component)
		{
			result_sstream << (double)(n_component + 1);

			if (n_component != (argument_n_components - 1))
			{
				result_sstream << ", ";
			}
		} /* for (all argument components) */

		result_sstream << ")";
	} /* for (all arguments) */

	result_sstream << ");\n";

	return result_sstream.str();
}

/** Retrieves body of a geometry shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test8::getGeometryShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form the body */
	result_sstream << "#version 420\n"
					  "\n"
					  "layout(points)                 in;\n"
					  "layout(max_vertices=1, points) out;\n"
					  "\n"
					  "void main()\n"
					  "{\n"
				   << getGeneralBody(test_case) << "}\n"
												   "\n";

	/* We're done! */
	return result_sstream.str();
}

/** Retrieves body of a tesellation control shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test8::getTessellationControlShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form the body */
	result_sstream << "#version 420\n"
					  "\n"
					  "layout(vertices=4) out;\n"
					  "\n"
					  "void main()\n"
					  "{\n"
				   << getGeneralBody(test_case) << "}\n"
												   "\n";

	/* Return the body */
	return result_sstream.str();
}

/** Retrieves body of a tessellation evaluation shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test8::getTessellationEvaluationShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form the body */
	result_sstream << "#version 420\n"
					  "\n"
					  "layout(isolines) in;\n"
					  "\n"
					  "void main()\n"
					  "{\n"
				   << getGeneralBody(test_case) << "}\n"
												   "\n";

	/* Return the body */
	return result_sstream.str();
}

/** Retrieves body of a vertex shader that should be used for the purpose of
 *  user-specified test case.
 *
 *  @param test_case Test case descriptor to use.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test8::getVertexShaderBody(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form the body */
	result_sstream << "#version 420\n"
					  "\n"
					  "void main()\n"
					  "{\n"
				   << getGeneralBody(test_case) << "}\n"
												   "\n";

	return result_sstream.str();
}

/** Initializes shader objects required to run the test. */
void GPUShaderFP64Test8::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate shader objects */

	/* Compute shader support and GL 4.2 required */
	if ((true == m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader")) &&
		(true == Utils::isGLVersionAtLeast(gl, 4 /* major */, 2 /* minor */)))
	{
		m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	}

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
	m_te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");
}

/** Assigns shader bodies to all shader objects that will be used for a single iteration.
 *
 *  @param test_case Test case descriptor to generate the shader bodies for.
 **/
void GPUShaderFP64Test8::initIteration(_test_case& test_case)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	test_case.cs_shader_body = getComputeShaderBody(test_case);
	test_case.fs_shader_body = getFragmentShaderBody(test_case);
	test_case.gs_shader_body = getGeometryShaderBody(test_case);
	test_case.tc_shader_body = getTessellationControlShaderBody(test_case);
	test_case.te_shader_body = getTessellationEvaluationShaderBody(test_case);
	test_case.vs_shader_body = getVertexShaderBody(test_case);

	/* Assign the bodies to relevant shaders */
	const char* cs_body_raw_ptr = test_case.cs_shader_body.c_str();
	const char* fs_body_raw_ptr = test_case.fs_shader_body.c_str();
	const char* gs_body_raw_ptr = test_case.gs_shader_body.c_str();
	const char* tc_body_raw_ptr = test_case.tc_shader_body.c_str();
	const char* te_body_raw_ptr = test_case.te_shader_body.c_str();
	const char* vs_body_raw_ptr = test_case.vs_shader_body.c_str();

	/* m_cs_id is initialized only if compute_shader is supported */
	if (0 != m_cs_id)
	{
		gl.shaderSource(m_cs_id, 1 /* count */, &cs_body_raw_ptr, DE_NULL /* length */);
	}

	gl.shaderSource(m_fs_id, 1 /* count */, &fs_body_raw_ptr, DE_NULL /* length */);
	gl.shaderSource(m_gs_id, 1 /* count */, &gs_body_raw_ptr, DE_NULL /* length */);
	gl.shaderSource(m_tc_id, 1 /* count */, &tc_body_raw_ptr, DE_NULL /* length */);
	gl.shaderSource(m_te_id, 1 /* count */, &te_body_raw_ptr, DE_NULL /* length */);
	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body_raw_ptr, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GPUShaderFP64Test8::iterate()
{
	/* Do not execute the test if GL_ARB_texture_view is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported.");
	}

	/* Initialize GL objects needed to run the tests */
	initTest();

	/* Build iteration array to run the tests in an automated manner */
	const Utils::_variable_type variable_types[] = { Utils::VARIABLE_TYPE_DMAT2,   Utils::VARIABLE_TYPE_DMAT2X3,
													 Utils::VARIABLE_TYPE_DMAT2X4, Utils::VARIABLE_TYPE_DMAT3,
													 Utils::VARIABLE_TYPE_DMAT3X2, Utils::VARIABLE_TYPE_DMAT3X4,
													 Utils::VARIABLE_TYPE_DMAT4,   Utils::VARIABLE_TYPE_DMAT4X2,
													 Utils::VARIABLE_TYPE_DMAT4X3, Utils::VARIABLE_TYPE_DOUBLE,
													 Utils::VARIABLE_TYPE_DVEC2,   Utils::VARIABLE_TYPE_DVEC3,
													 Utils::VARIABLE_TYPE_DVEC4 };
	const unsigned int n_variable_types = sizeof(variable_types) / sizeof(variable_types[0]);

	for (unsigned int n_variable_type = 0; n_variable_type < n_variable_types; ++n_variable_type)
	{
		const Utils::_variable_type variable_type = variable_types[n_variable_type];

		/* Construct a set of argument lists valid for the variable type considered */
		_argument_lists argument_lists = getArgumentListsForVariableType(variable_type);

		for (_argument_lists_const_iterator argument_list_iterator = argument_lists.begin();
			 argument_list_iterator != argument_lists.end(); argument_list_iterator++)
		{
			/* Constructor thwe test case descriptor */
			_test_case test_case;

			test_case.argument_list = *argument_list_iterator;
			test_case.type			= variable_type;

			/* Initialize a program object we will use to perform the casting */
			initIteration(test_case);

			/* See if the shader compiles. */
			m_has_test_passed &= executeIteration(test_case);
		} /* for (all argument lists) */
	}	 /* for (all variable types) */

	/* We're done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
GPUShaderFP64Test9::GPUShaderFP64Test9(deqp::Context& context)
	: TestCase(context, "operators", "Verifies that general and relational operators work "
									 "correctly when used against double-precision floating-"
									 "point types.")
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_xfb_bo_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all ES objects that may have been created during
 *  test execution.
 **/
void GPUShaderFP64Test9::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_xfb_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_xfb_bo_id);

		m_xfb_bo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays() call failed.");
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test iteration using user-specified test case properties.
 *
 *  @param test_case Test case descriptor.
 *
 *  @return true if the pass was successful, false if the test should fail.
 **/
bool GPUShaderFP64Test9::executeTestIteration(const _test_case& test_case)
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Activate the test program object */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Draw a single point with XFB enabled */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
	{
		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	/* Map the XFB BO into process space */
	const void* xfb_data_ptr = gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

	result = verifyXFBData(test_case, (const unsigned char*)xfb_data_ptr);

	/* Unmap the BO */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	return result;
}

/** Performs a matrix multiplication, given types of l-side and r-side matrices and stores the result
 *  under user-specified location.
 *
 *  @param matrix_a_type  Type of the l-side matrix.
 *  @param matrix_a_data  Row-ordered data of l-side matrix.
 *  @param matrix_b_type  Type of the r-side matrix.
 *  @param matrix_b_data  Row-ordered data of r-side matrix.
 *  @param out_result_ptr Deref to be used to store the multiplication result.
 **/
void GPUShaderFP64Test9::getMatrixMultiplicationResult(const Utils::_variable_type& matrix_a_type,
													   const std::vector<double>&   matrix_a_data,
													   const Utils::_variable_type& matrix_b_type,
													   const std::vector<double>& matrix_b_data, double* out_result_ptr)
{
	(void)matrix_b_type;
	using namespace tcu;
	/* To keep the code maintainable, we only consider cases relevant for this test */
	switch (matrix_a_type)
	{
	case Utils::VARIABLE_TYPE_DMAT2:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT2);

		tcu::Matrix2d matrix_a(&matrix_a_data[0]);
		tcu::Matrix2d matrix_b(&matrix_b_data[0]);
		tcu::Matrix2d result;

		matrix_a = transpose(matrix_a);
		matrix_b = transpose(matrix_b);
		result   = matrix_a * matrix_b;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 2 * 2);
		break;
	}

	case Utils::VARIABLE_TYPE_DMAT2X3:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT3X2);

		tcu::Matrix<double, 2, 3> matrix_a(&matrix_a_data[0]);
		tcu::Matrix<double, 3, 2> matrix_a_transposed;
		tcu::Matrix<double, 3, 2> matrix_b(&matrix_b_data[0]);
		tcu::Matrix<double, 2, 3> matrix_b_transposed;
		tcu::Matrix<double, 3, 3> result;

		matrix_a_transposed = transpose(matrix_a);
		matrix_b_transposed = transpose(matrix_b);
		result				= matrix_a_transposed * matrix_b_transposed;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 3 * 3);
		break;
	}

	case Utils::VARIABLE_TYPE_DMAT2X4:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT4X2);

		tcu::Matrix<double, 2, 4> matrix_a(&matrix_a_data[0]);
		tcu::Matrix<double, 4, 2> matrix_a_transposed;
		tcu::Matrix<double, 4, 2> matrix_b(&matrix_b_data[0]);
		tcu::Matrix<double, 2, 4> matrix_b_transposed;
		tcu::Matrix<double, 4, 4> result;

		matrix_a_transposed = transpose(matrix_a);
		matrix_b_transposed = transpose(matrix_b);
		result				= matrix_a_transposed * matrix_b_transposed;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 4 * 4);
		break;
	}

	case Utils::VARIABLE_TYPE_DMAT3:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT3);

		tcu::Matrix<double, 3, 3> matrix_a(&matrix_a_data[0]);
		tcu::Matrix<double, 3, 3> matrix_b(&matrix_b_data[0]);
		tcu::Matrix<double, 3, 3> result;

		matrix_a = transpose(matrix_a);
		matrix_b = transpose(matrix_b);
		result   = matrix_a * matrix_b;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 3 * 3);
		break;
	}

	case Utils::VARIABLE_TYPE_DMAT3X2:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT2X3);

		tcu::Matrix<double, 3, 2> matrix_a(&matrix_a_data[0]);
		tcu::Matrix<double, 2, 3> matrix_a_transposed;
		tcu::Matrix<double, 2, 3> matrix_b(&matrix_b_data[0]);
		tcu::Matrix<double, 3, 2> matrix_b_transposed;
		tcu::Matrix<double, 2, 2> result;

		matrix_a_transposed = transpose(matrix_a);
		matrix_b_transposed = transpose(matrix_b);
		result				= matrix_a_transposed * matrix_b_transposed;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 2 * 2);
		break;
	}

	case Utils::VARIABLE_TYPE_DMAT3X4:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT4X3);

		tcu::Matrix<double, 3, 4> matrix_a(&matrix_a_data[0]);
		tcu::Matrix<double, 4, 3> matrix_a_transposed;
		tcu::Matrix<double, 4, 3> matrix_b(&matrix_b_data[0]);
		tcu::Matrix<double, 3, 4> matrix_b_transposed;
		tcu::Matrix<double, 4, 4> result;

		matrix_a_transposed = transpose(matrix_a);
		matrix_b_transposed = transpose(matrix_b);
		result				= matrix_a_transposed * matrix_b_transposed;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 4 * 4);
		break;
	}

	case Utils::VARIABLE_TYPE_DMAT4:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT4);

		tcu::Matrix<double, 4, 4> matrix_a(&matrix_a_data[0]);
		tcu::Matrix<double, 4, 4> matrix_b(&matrix_b_data[0]);
		tcu::Matrix<double, 4, 4> result;

		matrix_a = transpose(matrix_a);
		matrix_b = transpose(matrix_b);
		result   = matrix_a * matrix_b;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 4 * 4);
		break;
	}

	case Utils::VARIABLE_TYPE_DMAT4X2:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT2X4);

		tcu::Matrix<double, 4, 2> matrix_a(&matrix_a_data[0]);
		tcu::Matrix<double, 2, 4> matrix_a_transposed;
		tcu::Matrix<double, 2, 4> matrix_b(&matrix_b_data[0]);
		tcu::Matrix<double, 4, 2> matrix_b_transposed;
		tcu::Matrix<double, 2, 2> result;

		matrix_a_transposed = transpose(matrix_a);
		matrix_b_transposed = transpose(matrix_b);
		result				= matrix_a_transposed * matrix_b_transposed;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 2 * 2);
		break;
	}

	case Utils::VARIABLE_TYPE_DMAT4X3:
	{
		DE_ASSERT(matrix_b_type == Utils::VARIABLE_TYPE_DMAT3X4);

		tcu::Matrix<double, 4, 3> matrix_a(&matrix_a_data[0]);
		tcu::Matrix<double, 3, 4> matrix_a_transposed;
		tcu::Matrix<double, 3, 4> matrix_b(&matrix_b_data[0]);
		tcu::Matrix<double, 4, 3> matrix_b_transposed;
		tcu::Matrix<double, 3, 3> result;

		matrix_a_transposed = transpose(matrix_a);
		matrix_b_transposed = transpose(matrix_b);
		result				= matrix_a_transposed * matrix_b_transposed;

		memcpy(out_result_ptr, result.getColumnMajorData().getPtr(), sizeof(double) * 3 * 3);
		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized matrix A type");
	}
	} /* switch (matrix_a_type) */
}

/** Returns GLSL operator representation of the user-specified operation.
 *
 *  @param operation_type Internal operation type to retrieve the operator for.
 *
 *  @return As per description.
 **/
const char* GPUShaderFP64Test9::getOperatorForOperationType(const _operation_type& operation_type)
{
	const char* result = NULL;

	switch (operation_type)
	{
	case OPERATION_TYPE_ADDITION:
		result = "+";
		break;
	case OPERATION_TYPE_DIVISION:
		result = "/";
		break;
	case OPERATION_TYPE_MULTIPLICATION:
		result = "*";
		break;
	case OPERATION_TYPE_SUBTRACTION:
		result = "-";
		break;

	case OPERATION_TYPE_PRE_DECREMENTATION:
	case OPERATION_TYPE_POST_DECREMENTATION:
	{
		result = "--";

		break;
	}

	case OPERATION_TYPE_PRE_INCREMENTATION:
	case OPERATION_TYPE_POST_INCREMENTATION:
	{
		result = "++";

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized operation type");
	}
	} /* switch(operation_type) */

	return result;
}

/** Returns a string representing user-specified operation type.
 *
 *  @param operation_type Operation type to return the literal for.
 *
 *  @return Requested string.
 **/
std::string GPUShaderFP64Test9::getOperationTypeString(const _operation_type& operation_type)
{
	std::string result = "[?]";

	switch (operation_type)
	{
	case OPERATION_TYPE_ADDITION:
		result = "addition";
		break;
	case OPERATION_TYPE_DIVISION:
		result = "division";
		break;
	case OPERATION_TYPE_MULTIPLICATION:
		result = "multiplication";
		break;
	case OPERATION_TYPE_SUBTRACTION:
		result = "subtraction";
		break;
	case OPERATION_TYPE_PRE_DECREMENTATION:
		result = "pre-decrementation";
		break;
	case OPERATION_TYPE_PRE_INCREMENTATION:
		result = "pre-incrementation";
		break;
	case OPERATION_TYPE_POST_DECREMENTATION:
		result = "post-decrementation";
		break;
	case OPERATION_TYPE_POST_INCREMENTATION:
		result = "post-incrementation";
		break;

	default:
	{
		TCU_FAIL("Unrecognized operation type");
	}
	}

	return result;
}

/** Returns body of a vertex shader that should be used for user-specified test case
 *  descriptor.
 *
 *  @param test_case Test case descriptor.
 *
 *  @return Requested GLSL shader body.
 **/
std::string GPUShaderFP64Test9::getVertexShaderBody(_test_case& test_case)
{
	std::stringstream  result_sstream;
	std::string		   result_variable_type_string = Utils::getVariableTypeString(test_case.variable_type);
	std::string		   variable_type_fp_string = Utils::getFPVariableTypeStringForVariableType(test_case.variable_type);
	std::string		   variable_type_string	= Utils::getVariableTypeString(test_case.variable_type);
	const unsigned int n_variable_components   = Utils::getNumberOfComponentsForVariableType(test_case.variable_type);

	/* If we are to multiply matrices, we will need to use a different type
	 * for the result variable if either of the matrices is not square.
	 */
	if (test_case.operation_type == OPERATION_TYPE_MULTIPLICATION &&
		Utils::isMatrixVariableType(test_case.variable_type))
	{
		Utils::_variable_type result_variable_type;
		Utils::_variable_type transposed_matrix_variable_type =
			Utils::getTransposedMatrixVariableType(test_case.variable_type);

		result_variable_type =
			Utils::getPostMatrixMultiplicationVariableType(test_case.variable_type, transposed_matrix_variable_type);
		result_variable_type_string = Utils::getVariableTypeString(result_variable_type);

		test_case.result_variable_type = result_variable_type;
	}

	/* Form the pre-amble */
	result_sstream << "#version 400\n"
					  "\n"

					  /* Add output variables */
					  "out "
				   << result_variable_type_string << " result;\n"
													 "out ivec2 result_lt;\n"
													 "out ivec2 result_lte;\n"
													 "out ivec2 result_gt;\n"
													 "out ivec2 result_gte;\n"
													 "void main()\n"
													 "{\n";

	/* Form reference values */
	result_sstream << variable_type_string << " reference1 = " << variable_type_string << "(";

	for (unsigned int n_variable_component = 0; n_variable_component < n_variable_components; ++n_variable_component)
	{
		result_sstream << (n_variable_component + 1);

		if (n_variable_component != (n_variable_components - 1))
		{
			result_sstream << ", ";
		}
	} /* for (all variable components) */

	result_sstream << ");\n";

	for (unsigned int n_ref2_case = 0; n_ref2_case < 2; /* single- and double-precision cases */
		 ++n_ref2_case)
	{
		Utils::_variable_type compatible_variable_type = test_case.variable_type;

		if (Utils::isMatrixVariableType(compatible_variable_type) &&
			test_case.operation_type == OPERATION_TYPE_MULTIPLICATION)
		{
			compatible_variable_type = Utils::getTransposedMatrixVariableType(compatible_variable_type);
		}

		std::string ref2_variable_type_fp_string =
			Utils::getFPVariableTypeStringForVariableType(compatible_variable_type);
		std::string ref2_variable_type_string = Utils::getVariableTypeString(compatible_variable_type);
		std::string ref2_variable_type = (n_ref2_case == 0) ? ref2_variable_type_fp_string : ref2_variable_type_string;
		std::string ref2_variable_name = (n_ref2_case == 0) ? "reference2f" : "reference2";

		result_sstream << ref2_variable_type << " " << ref2_variable_name << " = " << ref2_variable_type << "(";

		for (unsigned int n_variable_component = 0; n_variable_component < n_variable_components;
			 ++n_variable_component)
		{
			result_sstream << (n_variable_components - (n_variable_component + 1));

			if (n_variable_component != (n_variable_components - 1))
			{
				result_sstream << ", ";
			}
		} /* for (all variable components) */

		result_sstream << ");\n";
	} /* for (both reference2 declarations) */

	/* Add actual body */
	result_sstream << "\n"
					  "result = ";

	if (test_case.operation_type == OPERATION_TYPE_PRE_DECREMENTATION ||
		test_case.operation_type == OPERATION_TYPE_PRE_INCREMENTATION)
	{
		result_sstream << getOperatorForOperationType(test_case.operation_type);
	}

	result_sstream << "reference1 ";

	if (test_case.operation_type == OPERATION_TYPE_PRE_DECREMENTATION ||
		test_case.operation_type == OPERATION_TYPE_PRE_INCREMENTATION ||
		test_case.operation_type == OPERATION_TYPE_POST_DECREMENTATION ||
		test_case.operation_type == OPERATION_TYPE_POST_INCREMENTATION)
	{
		if (test_case.operation_type == OPERATION_TYPE_POST_DECREMENTATION ||
			test_case.operation_type == OPERATION_TYPE_POST_INCREMENTATION)
		{
			result_sstream << getOperatorForOperationType(test_case.operation_type);
		}
	}
	else
	{
		result_sstream << getOperatorForOperationType(test_case.operation_type) << " reference2";
	}

	result_sstream << ";\n";

	if (Utils::isScalarVariableType(test_case.variable_type))
	{
		result_sstream << "result_lt [0] = (reference1 <  reference2)  ? 1 : 0;\n"
						  "result_lt [1] = (reference1 <  reference2f) ? 1 : 0;\n"
						  "result_lte[0] = (reference1 <= reference2)  ? 1 : 0;\n"
						  "result_lte[1] = (reference1 <= reference2f) ? 1 : 0;\n"
						  "result_gt [0] = (reference1 >  reference2)  ? 1 : 0;\n"
						  "result_gt [1] = (reference1 >  reference2f) ? 1 : 0;\n"
						  "result_gte[0] = (reference1 >= reference2)  ? 1 : 0;\n"
						  "result_gte[1] = (reference1 >= reference2f) ? 1 : 0;\n";
	}
	else
	{
		result_sstream << "result_lt [0] = 1;\n"
						  "result_lt [1] = 1;\n"
						  "result_lte[0] = 1;\n"
						  "result_lte[1] = 1;\n"
						  "result_gt [0] = 1;\n"
						  "result_gt [1] = 1;\n"
						  "result_gte[0] = 1;\n"
						  "result_gte[1] = 1;\n";
	}

	result_sstream << "}\n";

	/* All done */
	return result_sstream.str();
}

/** Initializes all GL objects required to run the test.
 *
 *  This function can throw a TestError exception if the implementation misbehaves.
 */
void GPUShaderFP64Test9::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program & vertex shader objects */
	m_po_id = gl.createProgram();
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() or glCreateShader() call failed.");

	/* Attach the shader to the program */
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

	/* Set up a buffer object */
	gl.genBuffers(1, &m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Set up a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** Initializes all GL objects required to run an iteration described by
 *  user-specified test case descriptor.
 *
 *  @param test_case Test case descriptor to use for the initialization.
 **/
void GPUShaderFP64Test9::initTestIteration(_test_case& test_case)
{
	const glw::Functions& gl			  = m_context.getRenderContext().getFunctions();
	std::string			  vs_body		  = getVertexShaderBody(test_case);
	const char*			  vs_body_raw_ptr = vs_body.c_str();

	/* Store the shader's body */
	test_case.vs_body = vs_body;

	/* Try to compile the shader */
	glw::GLint compile_status = GL_FALSE;

	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body_raw_ptr, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status != GL_TRUE)
	{
		TCU_FAIL("Test shader compilation failed.");
	}

	/* Configure XFB */
	const char*		   xfb_names[] = { "result", "result_lt", "result_lte", "result_gt", "result_gte" };
	const unsigned int n_xfb_names = sizeof(xfb_names) / sizeof(xfb_names[0]);

	gl.transformFeedbackVaryings(m_po_id, n_xfb_names, xfb_names, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	/* Try to link the program */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Test program linking failure");
	}

	/* Set up XFB BO data storage */
	const unsigned int result_variable_size = static_cast<unsigned int>(
		Utils::getNumberOfComponentsForVariableType(test_case.result_variable_type) * sizeof(double));
	const unsigned int xfb_bo_size = static_cast<unsigned int>(
		result_variable_size + sizeof(int) * 2 /* ivec2s */ * 4); /* result_ output variables */

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_bo_size, DE_NULL /* data */, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GPUShaderFP64Test9::iterate()
{
	/* Do not execute the test if GL_ARB_texture_view is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported.");
	}

	/* Initialize all ES objects required to run all the checks */
	initTest();

	/* Iterate through all variable types we want to test */
	const Utils::_variable_type variable_types[] = { Utils::VARIABLE_TYPE_DMAT2,   Utils::VARIABLE_TYPE_DMAT2X3,
													 Utils::VARIABLE_TYPE_DMAT2X4, Utils::VARIABLE_TYPE_DMAT3,
													 Utils::VARIABLE_TYPE_DMAT3X2, Utils::VARIABLE_TYPE_DMAT3X4,
													 Utils::VARIABLE_TYPE_DMAT4,   Utils::VARIABLE_TYPE_DMAT4X2,
													 Utils::VARIABLE_TYPE_DMAT4X3, Utils::VARIABLE_TYPE_DOUBLE,
													 Utils::VARIABLE_TYPE_DVEC2,   Utils::VARIABLE_TYPE_DVEC3,
													 Utils::VARIABLE_TYPE_DVEC4 };
	const unsigned int n_variable_types = sizeof(variable_types) / sizeof(variable_types[0]);

	for (unsigned int n_variable_type = 0; n_variable_type < n_variable_types; ++n_variable_type)
	{
		/* Iterate through all operation types we want to check */
		for (unsigned int n_operation_type = 0; n_operation_type < OPERATION_TYPE_COUNT; ++n_operation_type)
		{
			_operation_type				 operation_type = (_operation_type)n_operation_type;
			const Utils::_variable_type& variable_type  = variable_types[n_variable_type];

			/* Construct test case descriptor */
			_test_case test_case;

			test_case.operation_type	   = operation_type;
			test_case.result_variable_type = variable_type;
			test_case.variable_type		   = variable_type;

			/* Run the iteration */
			initTestIteration(test_case);

			m_has_test_passed &= executeTestIteration(test_case);
		} /* for (all operation types) */
	}	 /* for (all variable types) */

	/* All done. */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies data XFBed out by the draw call for user-specified test case
 *  descriptor.
 *
 *  @param test_case Test case descriptor
 *  @param xfb_data  Buffer holding the data XFBed out during the draw call.
 *                   Must not be NULL.
 *
 *  @return true if the data was found to be correct, false otherwise.
 **/
bool GPUShaderFP64Test9::verifyXFBData(const _test_case& test_case, const unsigned char* xfb_data)
{
	const double	   epsilon = 1e-5;
	const unsigned int n_result_components =
		Utils::getNumberOfComponentsForVariableType(test_case.result_variable_type);
	bool		  result			  = true;
	const double* xfb_data_result	 = (const double*)xfb_data;
	const int*	xfb_data_result_lt  = (const int*)(xfb_data_result + n_result_components);
	const int*	xfb_data_result_lte = (const int*)xfb_data_result_lt + 2;  /* cast/non-cast cases */
	const int*	xfb_data_result_gt  = (const int*)xfb_data_result_lte + 2; /* cast/non-cast cases */
	const int*	xfb_data_result_gte = (const int*)xfb_data_result_gt + 2;  /* cast/non-cast cases */

	/* Prepare reference values */
	int					modifier;
	std::vector<double> reference1;
	std::vector<double> reference2;

	if (test_case.operation_type == OPERATION_TYPE_PRE_INCREMENTATION ||
		test_case.operation_type == OPERATION_TYPE_POST_INCREMENTATION)
	{
		modifier = 1;
	}
	else if (test_case.operation_type == OPERATION_TYPE_PRE_DECREMENTATION ||
			 test_case.operation_type == OPERATION_TYPE_POST_DECREMENTATION)
	{
		modifier = -1;
	}
	else
	{
		modifier = 0;
	}

	if (Utils::isMatrixVariableType(test_case.variable_type))
	{
		/* Matrices may be of different sizes so we need to compute the
		 * reference values separately for each matrix
		 */
		const Utils::_variable_type matrix_a_type = test_case.variable_type;
		const Utils::_variable_type matrix_b_type = Utils::getTransposedMatrixVariableType(test_case.variable_type);
		const unsigned int			n_matrix_a_components = Utils::getNumberOfComponentsForVariableType(matrix_a_type);
		const unsigned int			n_matrix_b_components = Utils::getNumberOfComponentsForVariableType(matrix_b_type);

		for (unsigned int n_component = 0; n_component < n_matrix_a_components; ++n_component)
		{
			reference1.push_back(modifier + n_component + 1);
		}

		for (unsigned int n_component = 0; n_component < n_matrix_b_components; ++n_component)
		{
			reference2.push_back(n_matrix_b_components - (n_component + 1));
		}
	} /* if (Utils::isMatrixVariableType(test_case.variable_type) */
	else
	{
		/* Generate as many components as will be expected for the result variable */
		for (unsigned int n_result_component = 0; n_result_component < n_result_components; ++n_result_component)
		{
			reference1.push_back(modifier + n_result_component + 1);
			reference2.push_back(n_result_components - (n_result_component + 1));
		}
	}

	/* Verify the result value(s) */
	if (test_case.operation_type == OPERATION_TYPE_MULTIPLICATION &&
		Utils::isMatrixVariableType(test_case.variable_type))
	{
		/* Matrix multiplication */
		double				  expected_result_data[4 * 4];
		Utils::_variable_type matrix_a_type = test_case.variable_type;
		Utils::_variable_type matrix_b_type = Utils::getTransposedMatrixVariableType(test_case.variable_type);

		getMatrixMultiplicationResult(matrix_a_type, reference1, matrix_b_type, reference2, expected_result_data);

		for (unsigned int n_component = 0; n_component < n_result_components; ++n_component)
		{
			if (de::abs(xfb_data_result[n_component] - expected_result_data[n_component]) > epsilon)
			{
				std::stringstream log_sstream;

				log_sstream << "Data returned for " << Utils::getVariableTypeString(matrix_a_type) << " * "
							<< Utils::getVariableTypeString(matrix_b_type)
							<< " matrix multiplication was incorrect; expected:(";

				for (unsigned int n_logged_component = 0; n_logged_component < n_result_components;
					 ++n_logged_component)
				{
					log_sstream << expected_result_data[n_logged_component];

					if (n_logged_component != (n_result_components - 1))
					{
						log_sstream << ", ";
					}
				} /* for (all components to be logged) */

				log_sstream << "), retrieved:(";

				for (unsigned int n_logged_component = 0; n_logged_component < n_result_components;
					 ++n_logged_component)
				{
					log_sstream << xfb_data_result[n_logged_component];

					if (n_logged_component != (n_result_components - 1))
					{
						log_sstream << ", ";
					}
				} /* for (all components to be logged) */

				log_sstream << ")";

				m_testCtx.getLog() << tcu::TestLog::Message << log_sstream.str().c_str() << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		} /* for (all result components) */
	}	 /* if (dealing with matrix multiplication) */
	else
	{
		for (unsigned int n_component = 0; n_component < n_result_components; ++n_component)
		{
			double expected_value = reference1[n_component];

			switch (test_case.operation_type)
			{
			case OPERATION_TYPE_ADDITION:
				expected_value += reference2[n_component];
				break;
			case OPERATION_TYPE_DIVISION:
				expected_value /= reference2[n_component];
				break;
			case OPERATION_TYPE_MULTIPLICATION:
				expected_value *= reference2[n_component];
				break;
			case OPERATION_TYPE_SUBTRACTION:
				expected_value -= reference2[n_component];
				break;

			case OPERATION_TYPE_PRE_DECREMENTATION:
			case OPERATION_TYPE_PRE_INCREMENTATION:
			{
				/* Modifier already applied */
				break;
			}

			case OPERATION_TYPE_POST_DECREMENTATION:
			case OPERATION_TYPE_POST_INCREMENTATION:
			{
				/* Need to reverse the modification for the purpose of the following check */
				expected_value -= modifier;

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized operation type");
			}
			} /* switch (test_case.operation_type) */

			if (de::abs(xfb_data_result[n_component] - expected_value) > epsilon)
			{
				std::string operation_type_string = getOperationTypeString(test_case.operation_type);
				std::string variable_type_string  = Utils::getVariableTypeString(test_case.variable_type);

				m_testCtx.getLog() << tcu::TestLog::Message << "Value(s) generated for variable type ["
								   << variable_type_string << "]"
															  " and operation type ["
								   << operation_type_string << "]"
															   " were found invalid."
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			} /* if (test case failed) */
		}	 /* for (all components) */
	}

	/* Verify the comparison operation results */
	if (Utils::isScalarVariableType(test_case.variable_type))
	{
		DE_ASSERT(n_result_components == 1);

		const bool expected_result_lt[2]  = { reference1[0] < reference2[0], reference1[0] < (float)reference2[0] };
		const bool expected_result_lte[2] = { reference1[0] <= reference2[0], reference1[0] <= (float)reference2[0] };
		const bool expected_result_gt[2]  = { reference1[0] > reference2[0], reference1[0] > (float)reference2[0] };
		const bool expected_result_gte[2] = { reference1[0] >= reference2[0], reference1[0] >= (float)reference2[0] };

		if ((xfb_data_result_lt[0] ? 1 : 0) != expected_result_lt[0] ||
			(xfb_data_result_lt[1] ? 1 : 0) != expected_result_lt[1])
		{
			std::string operation_type_string = getOperationTypeString(test_case.operation_type);
			std::string variable_type_string  = Utils::getVariableTypeString(test_case.variable_type);

			m_testCtx.getLog() << tcu::TestLog::Message << "Values reported for lower-than operator used for "
														   "variable type ["
							   << variable_type_string << "]"
														  "and operation type ["
							   << operation_type_string << "]"
														   "was found invalid; expected:("
							   << expected_result_lt[0] << ", " << expected_result_lt[1] << "); found:("
							   << xfb_data_result_lt[0] << ", " << xfb_data_result_lt[1] << ")."
							   << tcu::TestLog::EndMessage;

			result = false;
		}

		if ((xfb_data_result_lte[0] ? 1 : 0) != expected_result_lte[0] ||
			(xfb_data_result_lte[1] ? 1 : 0) != expected_result_lte[1])
		{
			std::string operation_type_string = getOperationTypeString(test_case.operation_type);
			std::string variable_type_string  = Utils::getVariableTypeString(test_case.variable_type);

			m_testCtx.getLog() << tcu::TestLog::Message << "Values reported for lower-than-or-equal operator used for "
														   "variable type ["
							   << variable_type_string << "]"
														  "and operation type ["
							   << operation_type_string << "]"
														   "was found invalid; expected:("
							   << expected_result_lt[0] << ", " << expected_result_lt[1] << "); found:("
							   << xfb_data_result_lt[0] << ", " << xfb_data_result_lt[1] << ")."
							   << tcu::TestLog::EndMessage;

			result = false;
		}

		if ((xfb_data_result_gt[0] ? 1 : 0) != expected_result_gt[0] ||
			(xfb_data_result_gt[1] ? 1 : 0) != expected_result_gt[1])
		{
			std::string operation_type_string = getOperationTypeString(test_case.operation_type);
			std::string variable_type_string  = Utils::getVariableTypeString(test_case.variable_type);

			m_testCtx.getLog() << tcu::TestLog::Message << "Values reported for greater-than operator used for "
														   "variable type ["
							   << variable_type_string << "]"
														  "and operation type ["
							   << operation_type_string << "]"
														   "was found invalid; expected:("
							   << expected_result_lt[0] << ", " << expected_result_lt[1] << "); found:("
							   << xfb_data_result_lt[0] << ", " << xfb_data_result_lt[1] << ")."
							   << tcu::TestLog::EndMessage;

			result = false;
		}

		if ((xfb_data_result_gte[0] ? 1 : 0) != expected_result_gte[0] ||
			(xfb_data_result_gte[1] ? 1 : 0) != expected_result_gte[1])
		{
			std::string operation_type_string = getOperationTypeString(test_case.operation_type);
			std::string variable_type_string  = Utils::getVariableTypeString(test_case.variable_type);

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Values reported for greater-than-or-equal operator used for "
								  "variable type ["
							   << variable_type_string << "]"
														  "and operation type ["
							   << operation_type_string << "]"
														   "was found invalid; expected:("
							   << expected_result_lt[0] << ", " << expected_result_lt[1] << "); found:("
							   << xfb_data_result_lt[0] << ", " << xfb_data_result_lt[1] << ")."
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* if (Utils::isScalarVariableType(test_case.variable_type) ) */
	else
	{
		if (xfb_data_result_lt[0] != 1 || xfb_data_result_lt[1] != 1 || xfb_data_result_lte[0] != 1 ||
			xfb_data_result_lte[1] != 1 || xfb_data_result_gt[0] != 1 || xfb_data_result_gt[1] != 1 ||
			xfb_data_result_gte[0] != 1 || xfb_data_result_gte[1] != 1)
		{
			std::string operation_type_string = getOperationTypeString(test_case.operation_type);
			std::string variable_type_string  = Utils::getVariableTypeString(test_case.variable_type);

			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid value was reported for matrix variable type, for which "
								  " operator checks are not executed; variable type ["
							   << variable_type_string << "]"
														  "and operation type ["
							   << operation_type_string << "]" << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	return result;
}

namespace TypeHelpers
{
/** Get base type for reference types
 *
 * @tparam T type
 **/
template <typename T>
class referenceToType
{
public:
	typedef T result;
};

template <typename T>
class referenceToType<const T&>
{
public:
	typedef T result;
};

/** Maps variable type with enumeration Utils::_variable_type
 *
 * @tparam T type
 **/
template <typename T>
class typeInfo;

template <>
class typeInfo<glw::GLboolean>
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_BOOL;
};

template <>
class typeInfo<glw::GLdouble>
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DOUBLE;
};

template <>
class typeInfo<tcu::UVec2>
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_UVEC2;
};

template <>
class typeInfo<tcu::UVec3>
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_UVEC3;
};

template <>
class typeInfo<tcu::UVec4>
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_UVEC4;
};

template <>
class typeInfo<tcu::DVec2>
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DVEC2;
};

template <>
class typeInfo<tcu::DVec3>
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DVEC3;
};

template <>
class typeInfo<tcu::DVec4>
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DVEC4;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 2, 2> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT2;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 3, 2> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT2X3;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 4, 2> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT2X4;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 3, 3> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT3;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 2, 3> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT3X2;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 4, 3> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT3X4;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 4, 4> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT4;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 2, 4> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT4X2;
};

template <>
class typeInfo<tcu::Matrix<glw::GLdouble, 3, 4> >
{
public:
	static const Utils::_variable_type variable_type = Utils::VARIABLE_TYPE_DMAT4X3;
};
} /* TypeHelpers */

/** Implementations of "math" functions required by "GPUShaderFP64Test10"
 *
 **/
namespace Math
{
template <typename T>
static T clamp(T x, T minVal, T maxVal);

template <int Size>
static tcu::Matrix<glw::GLdouble, Size, Size> cofactors(const tcu::Matrix<glw::GLdouble, Size, Size>& matrix);

template <int Size>
static tcu::Vector<glw::GLuint, Size> convertBvecToUvec(const tcu::Vector<bool, Size>& src);

template <typename T>
static T determinant(T val);

template <typename T>
static T determinant(const tcu::Matrix<T, 2, 2>& mat);

template <typename T>
static T determinant(const tcu::Matrix<T, 3, 3>& mat);

template <typename T>
static T determinant(const tcu::Matrix<T, 4, 4>& mat);

template <int Size>
static tcu::Matrix<glw::GLdouble, Size - 1, Size - 1> eliminate(const tcu::Matrix<glw::GLdouble, Size, Size>& matrix,
																glw::GLuint column, glw::GLuint row);

template <int Size>
static tcu::Vector<glw::GLuint, Size> equal(const tcu::Vector<glw::GLdouble, Size>& left,
											const tcu::Vector<glw::GLdouble, Size>& right);

static glw::GLdouble fma(glw::GLdouble a, glw::GLdouble b, glw::GLdouble c);

static glw::GLdouble fract(glw::GLdouble val);

template <typename T>
static T frexp(T val, glw::GLint& exp);

template <int Size>
static tcu::Vector<glw::GLuint, Size> greaterThan(const tcu::Vector<glw::GLdouble, Size>& left,
												  const tcu::Vector<glw::GLdouble, Size>& right);

template <int Size>
static tcu::Vector<glw::GLuint, Size> greaterThanEqual(const tcu::Vector<glw::GLdouble, Size>& left,
													   const tcu::Vector<glw::GLdouble, Size>& right);

template <int Size>
static tcu::Matrix<glw::GLdouble, Size, Size> inverse(const tcu::Matrix<glw::GLdouble, Size, Size>& matrix);

static glw::GLdouble inverseSqrt(glw::GLdouble val);

static glw::GLuint isinf_impl(glw::GLdouble val);

static glw::GLuint isnan_impl(glw::GLdouble val);

template <typename T>
static T ldexp(T val, glw::GLint exp);

template <int Size>
static tcu::Vector<glw::GLuint, Size> lessThan(const tcu::Vector<glw::GLdouble, Size>& left,
											   const tcu::Vector<glw::GLdouble, Size>& right);

template <int Size>
static tcu::Vector<glw::GLuint, Size> lessThanEqual(const tcu::Vector<glw::GLdouble, Size>& left,
													const tcu::Vector<glw::GLdouble, Size>& right);

template <typename T>
static T max(T left, T right);

template <typename T>
static T min(T left, T right);

template <int		 Size>
static glw::GLdouble minor_impl(const tcu::Matrix<glw::GLdouble, Size, Size>& matrix, glw::GLuint column,
								glw::GLuint row);

template <typename T>
static T mix(T left, T right, T weight);

template <typename T>
static T mod(T left, T right);

template <typename T>
static T modf(T val, T& integer);

template <typename T>
static T multiply(T left, T right);

template <int Size>
static tcu::Vector<glw::GLuint, Size> notEqual(const tcu::Vector<glw::GLdouble, Size>& left,
											   const tcu::Vector<glw::GLdouble, Size>& right);

template <int Cols, int Rows>
static tcu::Matrix<glw::GLdouble, Rows, Cols> outerProduct(const tcu::Vector<glw::GLdouble, Rows>& left,
														   const tcu::Vector<glw::GLdouble, Cols>& right);

static glw::GLdouble packDouble2x32(const tcu::UVec2& in);

template <typename T>
static T round(T t);

template <typename T>
static T roundEven(T t);

template <typename T>
static T sign(T t);

template <typename T>
static T smoothStep(T e0, T e1, T val);

template <typename T>
static T step(T edge, T val);

template <typename T, int Rows, int Cols>
static tcu::Matrix<T, Cols, Rows> transpose(const tcu::Matrix<T, Rows, Cols>& matrix);

template <typename T>
static T trunc(T t);

static tcu::UVec2 unpackDouble2x32(const glw::GLdouble& val);

template <typename T>
static T clamp(T x, T minVal, T maxVal)
{
	return min(max(x, minVal), maxVal);
}

template <int Size>
static tcu::Matrix<glw::GLdouble, Size, Size> cofactors(const tcu::Matrix<glw::GLdouble, Size, Size>& matrix)
{
	tcu::Matrix<glw::GLdouble, Size, Size> result;

	for (glw::GLuint c = 0; c < Size; ++c)
	{
		for (glw::GLuint r = 0; r < Size; ++r)
		{
			const glw::GLdouble minor_value = minor_impl(matrix, c, r);

			result(r, c) = (1 == (c + r) % 2) ? -minor_value : minor_value;
		}
	}

	return result;
}

template <int Size>
static tcu::Vector<glw::GLuint, Size> convertBvecToUvec(const tcu::Vector<bool, Size>& src)
{
	tcu::Vector<glw::GLuint, Size> result;

	for (glw::GLint i = 0; i < Size; ++i)
	{
		if (GL_FALSE != src[i])
		{
			result[i] = 1;
		}
		else
		{
			result[i] = 0;
		}
	}

	return result;
}

template <typename T>
static T det2(T _00, T _10, T _01, T _11)
{
	return _00 * _11 - _01 * _10;
}

template <typename T>
static T det3(T _00, T _10, T _20, T _01, T _11, T _21, T _02, T _12, T _22)
{
	return _00 * det2(_11, _21, _12, _22) - _10 * det2(_01, _21, _02, _22) + _20 * det2(_01, _11, _02, _12);
}

template <typename T>
static T det4(T _00, T _10, T _20, T _30, T _01, T _11, T _21, T _31, T _02, T _12, T _22, T _32, T _03, T _13, T _23,
			  T _33)
{
	return _00 * det3(_11, _21, _31, _12, _22, _32, _13, _23, _33) -
		   _10 * det3(_01, _21, _31, _02, _22, _32, _03, _23, _33) +
		   _20 * det3(_01, _11, _31, _02, _12, _32, _03, _13, _33) -
		   _30 * det3(_01, _11, _21, _02, _12, _22, _03, _13, _23);
}

template <typename T>
static T determinant(T val)
{
	return val;
}

template <typename T>
static T determinant(const tcu::Matrix<T, 2, 2>& mat)
{
	return det2(mat(0, 0), mat(0, 1), mat(1, 0), mat(1, 1));
}

template <typename T>
static T determinant(const tcu::Matrix<T, 3, 3>& mat)
{
	return det3(mat(0, 0), mat(0, 1), mat(0, 2), mat(1, 0), mat(1, 1), mat(1, 2), mat(2, 0), mat(2, 1), mat(2, 2));
}

template <typename T>
static T determinant(const tcu::Matrix<T, 4, 4>& mat)
{
	return det4(mat(0, 0), mat(0, 1), mat(0, 2), mat(0, 3), mat(1, 0), mat(1, 1), mat(1, 2), mat(1, 3), mat(2, 0),
				mat(2, 1), mat(2, 2), mat(2, 3), mat(3, 0), mat(3, 1), mat(3, 2), mat(3, 3));
}

template <int Size>
static tcu::Matrix<glw::GLdouble, Size - 1, Size - 1> eliminate(const tcu::Matrix<glw::GLdouble, Size, Size>& matrix,
																glw::GLuint column, glw::GLuint row)
{
	tcu::Matrix<glw::GLdouble, Size - 1, Size - 1> result;

	for (glw::GLuint c = 0; c < Size; ++c)
	{
		/* Skip eliminated column */
		if (column == c)
		{
			continue;
		}

		for (glw::GLuint r = 0; r < Size; ++r)
		{
			/* Skip eliminated row */
			if (row == r)
			{
				continue;
			}

			const glw::GLint r_offset = (r > row) ? -1 : 0;
			const glw::GLint c_offset = (c > column) ? -1 : 0;

			result(r + r_offset, c + c_offset) = matrix(r, c);
		}
	}

	return result;
}

template <int Size>
static tcu::Vector<glw::GLuint, Size> equal(const tcu::Vector<glw::GLdouble, Size>& left,
											const tcu::Vector<glw::GLdouble, Size>& right)
{
	return convertBvecToUvec(tcu::equal(left, right));
}

static glw::GLdouble fma(glw::GLdouble a, glw::GLdouble b, glw::GLdouble c)
{
	return a * b + c;
}

static glw::GLdouble fract(glw::GLdouble val)
{
	return val - floor(val);
}

template <typename T>
static T frexp(T val, glw::GLint& exp)
{
	return ::frexp(val, &exp);
}

template <int Size>
static tcu::Vector<glw::GLuint, Size> greaterThan(const tcu::Vector<glw::GLdouble, Size>& left,
												  const tcu::Vector<glw::GLdouble, Size>& right)
{
	return convertBvecToUvec(tcu::greaterThan(left, right));
}

template <int Size>
static tcu::Vector<glw::GLuint, Size> greaterThanEqual(const tcu::Vector<glw::GLdouble, Size>& left,
													   const tcu::Vector<glw::GLdouble, Size>& right)
{
	return convertBvecToUvec(tcu::greaterThanEqual(left, right));
}

template <int Size>
static tcu::Matrix<glw::GLdouble, Size, Size> inverse(const tcu::Matrix<glw::GLdouble, Size, Size>& matrix)
{
	const tcu::Matrix<glw::GLdouble, Size, Size> cof	  = cofactors(matrix);
	const tcu::Matrix<glw::GLdouble, Size, Size> adjugate = tcu::transpose(cof);
	const glw::GLdouble det		= determinant(matrix);
	const glw::GLdouble inv_det = 1.0 / det;

	tcu::Matrix<glw::GLdouble, Size, Size> result = adjugate * inv_det;

	return result;
}

static glw::GLdouble inverseSqrt(glw::GLdouble val)
{
	const glw::GLdouble root = sqrt(val);

	return (1.0 / root);
}

static glw::GLuint isinf_impl(glw::GLdouble val)
{
	const glw::GLdouble infinity = std::numeric_limits<glw::GLdouble>::infinity();

	return ((infinity == val) || (-infinity == val));
}

static glw::GLuint isnan_impl(glw::GLdouble val)
{
	return val != val;
}

template <typename T>
static T ldexp(T val, glw::GLint exp)
{
	return ::ldexp(val, exp);
}

template <int Size>
static tcu::Vector<glw::GLuint, Size> lessThan(const tcu::Vector<glw::GLdouble, Size>& left,
											   const tcu::Vector<glw::GLdouble, Size>& right)
{
	return convertBvecToUvec(tcu::lessThan(left, right));
}

template <int Size>
static tcu::Vector<glw::GLuint, Size> lessThanEqual(const tcu::Vector<glw::GLdouble, Size>& left,
													const tcu::Vector<glw::GLdouble, Size>& right)
{
	return convertBvecToUvec(tcu::lessThanEqual(left, right));
}

template <typename T>
static T max(T left, T right)
{
	return (left >= right) ? left : right;
}

template <typename T>
static T min(T left, T right)
{
	return (left <= right) ? left : right;
}

template <int		 Size>
static glw::GLdouble minor_impl(const tcu::Matrix<glw::GLdouble, Size, Size>& matrix, glw::GLuint column,
								glw::GLuint row)
{
	tcu::Matrix<glw::GLdouble, Size - 1, Size - 1> eliminated = eliminate(matrix, column, row);

	return determinant(eliminated);
}

template <>
glw::GLdouble minor_impl<2>(const tcu::Matrix<glw::GLdouble, 2, 2>& matrix, glw::GLuint column, glw::GLuint row)
{
	const glw::GLuint r = (0 == row) ? 1 : 0;
	const glw::GLuint c = (0 == column) ? 1 : 0;

	return matrix(r, c);
}

template <typename T>
static T mix(T left, T right, T weight)
{
	return left * (1 - weight) + right * (weight);
}

template <typename T>
static T mod(T left, T right)
{
	const T div_res = left / right;
	const T floored = floor(div_res);

	return left - right * floored;
}

template <typename T>
static T modf(T val, T& integer)
{
	return ::modf(val, &integer);
}

template <typename T>
static T multiply(T left, T right)
{
	T result = left * right;

	return result;
}

template <int Size>
static tcu::Vector<glw::GLuint, Size> notEqual(const tcu::Vector<glw::GLdouble, Size>& left,
											   const tcu::Vector<glw::GLdouble, Size>& right)
{
	return convertBvecToUvec(tcu::notEqual(left, right));
}

template <int Cols, int Rows>
static tcu::Matrix<glw::GLdouble, Rows, Cols> outerProduct(const tcu::Vector<glw::GLdouble, Rows>& left,
														   const tcu::Vector<glw::GLdouble, Cols>& right)
{
	tcu::Matrix<glw::GLdouble, Rows, 1>	left_mat;
	tcu::Matrix<glw::GLdouble, 1, Cols>	right_mat;
	tcu::Matrix<glw::GLdouble, Rows, Cols> result;

	for (glw::GLuint i = 0; i < Rows; ++i)
	{
		left_mat(i, 0) = left[i];
	}

	for (glw::GLuint i = 0; i < Cols; ++i)
	{
		right_mat(0, i) = right[i];
	}

	result = left_mat * right_mat;

	return result;
}

static glw::GLdouble packDouble2x32(const tcu::UVec2& in)
{
	const glw::GLuint buffer[2] = { in[0], in[1] };
	glw::GLdouble	 result;
	memcpy(&result, buffer, sizeof(result));
	return result;
}

template <typename T>
static T round(T t)
{
	T frac = fract(t);
	T res  = t - frac;

	if (((T)0.5) < frac)
	{
		res += ((T)1.0);
	}

	return res;
}

template <typename T>
static T roundEven(T t)
{
	T frac = fract(t);
	T res  = t - frac;

	if (((T)0.5) < frac)
	{
		res += ((T)1.0);
	}
	else if ((((T)0.5) == frac) && (0 != ((int)res) % 2))
	{
		res += ((T)1.0);
	}

	return res;
}

template <typename T>
static T sign(T t)
{
	if (0 > t)
	{
		return -1;
	}
	else if (0 == t)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

template <typename T>
static T smoothStep(T e0, T e1, T val)
{
	if (e0 >= val)
	{
		return 0;
	}

	if (e1 <= val)
	{
		return 1;
	}

	T temp = (val - e0) / (e1 - e0);

	T result = temp * temp * (3 - 2 * temp);

	return result;
}

template <typename T>
static T step(T edge, T val)
{
	if (edge > val)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

template <typename T, int Rows, int Cols>
static tcu::Matrix<T, Cols, Rows> transpose(const tcu::Matrix<T, Rows, Cols>& matrix)
{
	tcu::Matrix<T, Cols, Rows> result = tcu::transpose(matrix);

	return result;
}

template <typename T>
static T trunc(T t)
{
	const T abs_value	= de::abs(t);
	const T result_value = floor(abs_value);

	const T result = sign(t) * result_value;

	return result;
}

static tcu::UVec2 unpackDouble2x32(const glw::GLdouble& val)
{
	glw::GLuint* ptr = (glw::GLuint*)&val;
	tcu::UVec2   result(ptr[0], ptr[1]);

	return result;
}
} /* Math */

/** Enumeration of tested functions
 * *_AGAINT_SCALAR enums are used to call glsl function with mixed scalar and genDType arguments.
 * For example "max" can be called for (dvec3, double).
 **/
enum FunctionEnum
{
	FUNCTION_ABS = 0,
	FUNCTION_CEIL,
	FUNCTION_CLAMP,
	FUNCTION_CLAMP_AGAINST_SCALAR,
	FUNCTION_CROSS,
	FUNCTION_DETERMINANT,
	FUNCTION_DISTANCE,
	FUNCTION_DOT,
	FUNCTION_EQUAL,
	FUNCTION_FACEFORWARD,
	FUNCTION_FLOOR,
	FUNCTION_FMA,
	FUNCTION_FRACT,
	FUNCTION_FREXP,
	FUNCTION_GREATERTHAN,
	FUNCTION_GREATERTHANEQUAL,
	FUNCTION_INVERSE,
	FUNCTION_INVERSESQRT,
	FUNCTION_LDEXP,
	FUNCTION_LESSTHAN,
	FUNCTION_LESSTHANEQUAL,
	FUNCTION_LENGTH,
	FUNCTION_MATRIXCOMPMULT,
	FUNCTION_MAX,
	FUNCTION_MAX_AGAINST_SCALAR,
	FUNCTION_MIN,
	FUNCTION_MIN_AGAINST_SCALAR,
	FUNCTION_MIX,
	FUNCTION_MOD,
	FUNCTION_MOD_AGAINST_SCALAR,
	FUNCTION_MODF,
	FUNCTION_NORMALIZE,
	FUNCTION_NOTEQUAL,
	FUNCTION_OUTERPRODUCT,
	FUNCTION_PACKDOUBLE2X32,
	FUNCTION_REFLECT,
	FUNCTION_REFRACT,
	FUNCTION_ROUND,
	FUNCTION_ROUNDEVEN,
	FUNCTION_SIGN,
	FUNCTION_SMOOTHSTEP,
	FUNCTION_SMOOTHSTEP_AGAINST_SCALAR,
	FUNCTION_SQRT,
	FUNCTION_STEP,
	FUNCTION_STEP_AGAINST_SCALAR,
	FUNCTION_TRANSPOSE,
	FUNCTION_TRUNC,
	FUNCTION_UNPACKDOUBLE2X32,
	FUNCTION_ISNAN,
	FUNCTION_ISINF,
};

struct TypeDefinition
{
	std::string name;
	glw::GLuint n_columns;
	glw::GLuint n_rows;
};

/** Implementation of BuiltinFunctionTest test, description follows:
 *
 *  Verify double-precision support in common functions works correctly.
 *  All double-precision types that apply for particular cases should
 *  be tested for the following functions:
 *
 *  - abs();
 *  - ceil();
 *  - clamp();
 *  - cross();
 *  - determinant();
 *  - distance();
 *  - dot();
 *  - equal();
 *  - faceforward();
 *  - floor();
 *  - fma();
 *  - fract();
 *  - frexp();
 *  - greaterThan();
 *  - greaterThanEqual();
 *  - inverse();
 *  - inversesqrt();
 *  - ldexp();
 *  - lessThan();
 *  - lessThanEqual();
 *  - length();
 *  - matrixCompMult();
 *  - max();
 *  - min();
 *  - mix();
 *  - mod();
 *  - modf();
 *  - normalize();
 *  - notEqual();
 *  - outerProduct();
 *  - packDouble2x32();
 *  - reflect();
 *  - refract();
 *  - round();
 *  - roundEven();
 *  - sign();
 *  - smoothstep();
 *  - sqrt();
 *  - step();
 *  - transpose();
 *  - trunc();
 *  - unpackDouble2x32();
 *  - isnan();
 *  - isinf();
 *
 *  The test should work by creating a program object (for each case
 *  considered), to which a vertex shader should be attached. The
 *  shader should define input variables that should be used as
 *  arguments for the function in question. The result of the
 *  operation should then be XFBed back to the test, where the
 *  value should be verified.
 *
 *  Reference function implementation from pre-DEQP CTS framework
 *  should be ported to C for verification purposes where available.
 *
 *  The test should use 1024 different scalar/vector/matrix argument
 *  combinations. It should pass if all functions are determined
 *  to work correctly for all argument combinations used.
 **/
class BuiltinFunctionTest : public deqp::TestCase
{
public:
	/* Public methods */
	BuiltinFunctionTest(deqp::Context& context, std::string caseName, FunctionEnum function,
						TypeDefinition typeDefinition);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

	/** Base class of functionObject. Main goal of it is to keep function details toghether and hide calling code.
	 *
	 **/
	class functionObject
	{
	public:
		functionObject(FunctionEnum function_enum, const glw::GLchar* function_name, glw::GLvoid* function_pointer,
					   Utils::_variable_type result_type);

		virtual ~functionObject()
		{
		}

		virtual void call(glw::GLvoid* result_dst, const glw::GLvoid* argument_src) const = 0;

		virtual glw::GLuint			  getArgumentCount() const					  = 0;
		virtual Utils::_variable_type getArgumentType(glw::GLuint argument) const = 0;
		glw::GLuint getArgumentComponents(glw::GLuint argument) const;
		glw::GLuint getArgumentComponentSize(glw::GLuint argument) const;
		glw::GLuint getArgumentOffset(glw::GLuint argument) const;
		glw::GLuint getArgumentStride() const;
		glw::GLuint getArgumentStride(glw::GLuint argument) const;
		FunctionEnum	   getFunctionEnum() const;
		const glw::GLchar* getName() const;
		glw::GLuint getResultComponents(glw::GLuint result) const;
		virtual glw::GLuint getResultCount() const;
		glw::GLuint getResultOffset(glw::GLuint result) const;
		virtual Utils::_variable_type getResultType(glw::GLuint result) const;
		glw::GLuint getResultStride(glw::GLuint result) const;
		glw::GLuint getBaseTypeSize(glw::GLuint result) const;
		glw::GLuint getResultStride() const;

	protected:
		const FunctionEnum			m_function_enum;
		const glw::GLchar*			m_function_name;
		const glw::GLvoid*			m_p_function;
		const Utils::_variable_type m_res_type;
	};

private:
	/* Private types */
	/** General type enumeration
	 *
	 **/
	enum generalType
	{
		SCALAR = 0,
		VECTOR,
		MATRIX,
	};

	/** Details of variable type
	 *
	 **/
	struct typeDetails
	{
		typeDetails(glw::GLuint n_columns, glw::GLuint n_rows);

		generalType m_general_type;
		glw::GLuint m_n_columns;
		glw::GLuint m_n_rows;
		glw::GLenum m_type;
		std::string m_type_name;
	};

	/* Typedefs for gl.uniform* function pointers */
	typedef GLW_APICALL void(GLW_APIENTRY* uniformDMatFunctionPointer)(glw::GLint, glw::GLsizei, glw::GLboolean,
																	   const glw::GLdouble*);
	typedef GLW_APICALL void(GLW_APIENTRY* uniformDVecFunctionPointer)(glw::GLint, glw::GLsizei, const glw::GLdouble*);
	typedef GLW_APICALL void(GLW_APIENTRY* uniformIVecFunctionPointer)(glw::GLint, glw::GLsizei, const glw::GLint*);
	typedef GLW_APICALL void(GLW_APIENTRY* uniformUVecFunctionPointer)(glw::GLint, glw::GLsizei, const glw::GLuint*);

	/* Private methods */
	bool compare(Utils::_variable_type type, const glw::GLvoid* left, const glw::GLvoid* right);

	functionObject* getFunctionObject(FunctionEnum function, const typeDetails& type);

	uniformDMatFunctionPointer getUniformFunctionForDMat(glw::GLuint		   argument,
														 const functionObject& function_object) const;

	uniformDVecFunctionPointer getUniformFunctionForDVec(glw::GLuint		   argument,
														 const functionObject& function_object) const;

	uniformIVecFunctionPointer getUniformFunctionForIVec(glw::GLuint		   argument,
														 const functionObject& function_object) const;

	uniformUVecFunctionPointer getUniformFunctionForUVec(glw::GLuint		   argument,
														 const functionObject& function_object) const;

	const glw::GLchar* getUniformName(glw::GLuint argument) const;
	const glw::GLchar* getVaryingName(glw::GLuint argument) const;

	bool isFunctionImplemented(FunctionEnum function, const typeDetails& type) const;

	void logVariableType(const glw::GLvoid* buffer, const glw::GLchar* name, Utils::_variable_type type) const;

	void prepareArgument(const functionObject& function_object, glw::GLuint vertex, glw::GLubyte* buffer);

	void prepareComponents(const functionObject& function_object, glw::GLuint vertex, glw::GLuint argument,
						   glw::GLubyte* buffer);

	void prepareProgram(const functionObject& function_object, Utils::programInfo& program_info);

	void prepareTestData(const functionObject& function_object);
	void prepareVertexShaderCode(const functionObject& function_object);

	bool test(FunctionEnum function, const typeDetails& type);

	void testBegin(const functionObject& function_object, glw::GLuint program_id, glw::GLuint vertex);

	void testInit();

	bool verifyResults(const functionObject& function_object, glw::GLuint vertex);

	/* Private constants */
	static const glw::GLdouble m_epsilon;
	static const glw::GLuint   m_n_veritces;

	/* Private fields */
	glw::GLuint m_transform_feedback_buffer_id;
	glw::GLuint m_vertex_array_object_id;

	std::vector<glw::GLubyte> m_expected_results_data;
	FunctionEnum			  m_function;
	TypeDefinition			  m_typeDefinition;
	std::vector<glw::GLubyte> m_argument_data;
	std::string				  m_vertex_shader_code;
};

/* Constants used by BuiltinFunctionTest */
/** Khronos Bug #14010
 *  Using an epsilon value for comparing floating points is error prone.
 *  Rather than writing a new floating point comparison function, I am
 *  increasing the epsilon value to allow greater orders of magnitude
 *  of floating point values.
 **/
const glw::GLdouble BuiltinFunctionTest::m_epsilon	= 0.00002;
const glw::GLuint   BuiltinFunctionTest::m_n_veritces = 1024;

/** Implementations of function objects required by "BuiltinFunctionTest"
 *
 **/
namespace FunctionObject
{
/** Maps variable type with enumeration Utils::_variable_type
 *
 * @tparam T type
 **/
template <typename T>
class typeInfo
{
public:
	static const Utils::_variable_type variable_type =
		TypeHelpers::typeInfo<typename TypeHelpers::referenceToType<T>::result>::variable_type;
};

/** Place data from <in> into <buffer>
 *
 * @param buffer Buffer
 * @param in     Input data
 **/
template <typename T>
class pack
{
public:
	static void set(glw::GLvoid* buffer, const T& in)
	{
		*(T*)buffer = in;
	}
};

/** Place tcu::Matrix data from <in> into <buffer>
 *
 * @param buffer Buffer
 * @param in     Input data
 **/
template <int Cols, int Rows>
class pack<tcu::Matrix<glw::GLdouble, Rows, Cols> >
{
public:
	static void set(glw::GLvoid* buffer, const tcu::Matrix<glw::GLdouble, Rows, Cols>& in)
	{
		glw::GLdouble* data = (glw::GLdouble*)buffer;

		for (glw::GLint column = 0; column < Cols; ++column)
		{
			for (glw::GLint row = 0; row < Rows; ++row)
			{
				glw::GLint index = column * Rows + row;

				data[index] = in(row, column);
			}
		}
	}
};

/** Get data of <out> from <buffer>
 *
 * @param buffer Buffer
 * @param out    Output data
 **/
template <typename T>
class unpack
{
public:
	static void get(const glw::GLvoid* buffer, T& out)
	{
		out = *(T*)buffer;
	}
};

/** Get tcu::Matrix data from <buffer>
 *
 * @param buffer Buffer
 * @param out    Output data
 **/
template <int Cols, int Rows>
class unpack<tcu::Matrix<glw::GLdouble, Rows, Cols> >
{
public:
	static void get(const glw::GLvoid* buffer, tcu::Matrix<glw::GLdouble, Rows, Cols>& out)
	{
		const glw::GLdouble* data = (glw::GLdouble*)buffer;

		for (glw::GLint column = 0; column < Cols; ++column)
		{
			for (glw::GLint row = 0; row < Rows; ++row)
			{
				glw::GLint index = column * Rows + row;

				out(row, column) = data[index];
			}
		}
	}
};

/** Base of unary function classes
 *
 **/
class unaryBase : public BuiltinFunctionTest::functionObject
{
public:
	unaryBase(FunctionEnum function_enum, const glw::GLchar* function_name, glw::GLvoid* function_pointer,
			  const Utils::_variable_type res_type, const Utils::_variable_type arg_type)
		: functionObject(function_enum, function_name, function_pointer, res_type), m_arg_type(arg_type)
	{
	}

	virtual glw::GLuint getArgumentCount() const
	{
		return 1;
	}

	virtual Utils::_variable_type getArgumentType(glw::GLuint /* argument */) const
	{
		return m_arg_type;
	}

protected:
	const Utils::_variable_type m_arg_type;
};

/** Unary function class. It treats input argument as one variable.
 *
 * @tparam ResT Type of result
 * @tparam ArgT Type of argument
 **/
template <typename ResT, typename ArgT>
class unary : public unaryBase
{
public:
	typedef ResT (*functionPointer)(const ArgT&);

	unary(FunctionEnum function_enum, const glw::GLchar* function_name, functionPointer function_pointer)
		: unaryBase(function_enum, function_name, (glw::GLvoid*)function_pointer, typeInfo<ResT>::variable_type,
					typeInfo<ArgT>::variable_type)
	{
	}

	virtual void call(glw::GLvoid* result_dst, const glw::GLvoid* argument_src) const
	{
		ResT result;
		ArgT arg;

		unpack<ArgT>::get(argument_src, arg);

		functionPointer p_function = (functionPointer)m_p_function;

		result = p_function(arg);

		pack<ResT>::set(result_dst, result);
	}
};

/** Unary function class. It treats input argument as separate components.
 *
 * @tparam ResT Type of result
 **/
template <typename ResT>
class unaryByComponent : public unaryBase
{
public:
	typedef ResT (*functionPointer)(glw::GLdouble);

	unaryByComponent(FunctionEnum function_enum, const glw::GLchar* function_name, functionPointer function_pointer,
					 const Utils::_variable_type res_type, const Utils::_variable_type arg_type)
		: unaryBase(function_enum, function_name, (glw::GLvoid*)function_pointer, res_type, arg_type)
	{
	}

	virtual void call(glw::GLvoid* result_dst, const glw::GLvoid* argument_src) const
	{
		glw::GLuint	n_components = Utils::getNumberOfComponentsForVariableType(m_arg_type);
		ResT*		   p_result		= (ResT*)result_dst;
		glw::GLdouble* p_arg		= (glw::GLdouble*)argument_src;

		functionPointer p_function = (functionPointer)m_p_function;

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			p_result[component] = p_function(p_arg[component]);
		}
	}
};

/** Class of functions with one input and one output parameter. It treats arguments as separate components.
 *
 * @tparam ResT Type of result
 * @tparam ArgT Type of argument
 * @tparam OutT Type of output parameter
 **/
template <typename ResT, typename ArgT, typename OutT>
class unaryWithOutputByComponent : public unaryBase
{
public:
	typedef ResT (*functionPointer)(ArgT, OutT&);

	unaryWithOutputByComponent(FunctionEnum function_enum, const glw::GLchar* function_name,
							   functionPointer function_pointer, const Utils::_variable_type res_type,
							   const Utils::_variable_type arg_type, const Utils::_variable_type out_type)
		: unaryBase(function_enum, function_name, (glw::GLvoid*)function_pointer, res_type, arg_type)
		, m_out_type(out_type)
	{
	}

	virtual void call(glw::GLvoid* result_dst, const glw::GLvoid* argument_src) const
	{
		ResT* p_result = (ResT*)result_dst;
		OutT* p_out	= (OutT*)((glw::GLubyte*)result_dst + getResultOffset(1));
		ArgT* p_arg	= (ArgT*)argument_src;

		const glw::GLuint n_components_0 = getArgumentComponents(0);
		const glw::GLuint n_components_1 = getResultComponents(1);
		const glw::GLuint n_components   = de::max(n_components_0, n_components_1);

		const glw::GLuint component_step_0 = (1 == n_components_0) ? 0 : 1;
		const glw::GLuint component_step_1 = (1 == n_components_1) ? 0 : 1;

		functionPointer p_function = (functionPointer)m_p_function;

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			const ArgT first_arg  = p_arg[component * component_step_0];
			OutT&	  second_arg = p_out[component * component_step_1];

			p_result[component] = p_function(first_arg, second_arg);
		}
	}

	glw::GLuint getResultCount() const
	{
		return 2;
	}

	Utils::_variable_type getResultType(glw::GLuint result) const
	{
		Utils::_variable_type type = Utils::VARIABLE_TYPE_UNKNOWN;

		switch (result)
		{
		case 0:
			type = m_res_type;
			break;
		case 1:
			type = m_out_type;
			break;
		default:
			TCU_FAIL("Not implemented");
			break;
		}

		return type;
	}

protected:
	const Utils::_variable_type m_out_type;
};

/** Base of binary function classes.
 *
 **/
class binaryBase : public BuiltinFunctionTest::functionObject
{
public:
	binaryBase(FunctionEnum function_enum, const glw::GLchar* function_name, glw::GLvoid* function_pointer,
			   const Utils::_variable_type res_type, const Utils::_variable_type arg_1_type,
			   const Utils::_variable_type arg_2_type)
		: functionObject(function_enum, function_name, function_pointer, res_type)
		, m_arg_1_type(arg_1_type)
		, m_arg_2_type(arg_2_type)
	{
	}

	virtual glw::GLuint getArgumentCount() const
	{
		return 2;
	}

	virtual Utils::_variable_type getArgumentType(glw::GLuint argument) const
	{
		switch (argument)
		{
		case 0:
			return m_arg_1_type;
			break;
		case 1:
			return m_arg_2_type;
			break;
		default:
			return Utils::VARIABLE_TYPE_UNKNOWN;
			break;
		}
	}

protected:
	const Utils::_variable_type m_arg_1_type;
	const Utils::_variable_type m_arg_2_type;
};

/** Binary function class. It treats input arguments as two variables.
 *
 * @param ResT  Type of result
 * @param Arg1T Type of first argument
 * @param Arg2T Type of second argument
 **/
template <typename ResT, typename Arg1T, typename Arg2T>
class binary : public binaryBase
{
public:
	typedef ResT (*functionPointer)(const Arg1T&, const Arg2T&);

	binary(FunctionEnum function_enum, const glw::GLchar* function_name, functionPointer function_pointer)
		: binaryBase(function_enum, function_name, (glw::GLvoid*)function_pointer, typeInfo<ResT>::variable_type,
					 typeInfo<Arg1T>::variable_type, typeInfo<Arg2T>::variable_type)
	{
	}

	virtual void call(glw::GLvoid* result_dst, const glw::GLvoid* argument_src) const
	{
		const glw::GLuint argument_1_stride = getArgumentStride(0);

		functionPointer p_function = (functionPointer)m_p_function;

		Arg1T arg_1;
		Arg2T arg_2;
		ResT  result;

		unpack<Arg1T>::get(argument_src, arg_1);
		unpack<Arg2T>::get((glw::GLubyte*)argument_src + argument_1_stride, arg_2);

		result = p_function(arg_1, arg_2);

		pack<ResT>::set(result_dst, result);
	}
};

/** Binary function class. It treats input arguments as separate components.
 *
 * @param ResT  Type of result
 * @param Arg1T Type of first argument
 * @param Arg2T Type of second argument
 **/
template <typename ResT, typename Arg1T, typename Arg2T>
class binaryByComponent : public binaryBase
{
public:
	typedef ResT (*functionPointer)(Arg1T, Arg2T);

	binaryByComponent(FunctionEnum function_enum, const glw::GLchar* function_name, functionPointer function_pointer,
					  const Utils::_variable_type res_type, const Utils::_variable_type arg_1_type,
					  const Utils::_variable_type arg_2_type)
		: binaryBase(function_enum, function_name, (glw::GLvoid*)function_pointer, res_type, arg_1_type, arg_2_type)
	{
	}

	virtual void call(glw::GLvoid* result_dst, const glw::GLvoid* argument_src) const
	{
		ResT*  p_result = (ResT*)result_dst;
		Arg1T* p_arg_1  = (Arg1T*)argument_src;
		Arg2T* p_arg_2  = (Arg2T*)((glw::GLubyte*)argument_src + getArgumentOffset(1));

		const glw::GLuint n_components_0 = getArgumentComponents(0);
		const glw::GLuint n_components_1 = getArgumentComponents(1);
		const glw::GLuint n_components   = de::max(n_components_0, n_components_1);

		const glw::GLuint component_step_0 = (1 == n_components_0) ? 0 : 1;
		const glw::GLuint component_step_1 = (1 == n_components_1) ? 0 : 1;

		functionPointer p_function = (functionPointer)m_p_function;

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			const Arg1T first_arg  = p_arg_1[component * component_step_0];
			const Arg2T second_arg = p_arg_2[component * component_step_1];

			p_result[component] = p_function(first_arg, second_arg);
		}
	}
};

/** Base of tenary function classes.
 *
 **/
class tenaryBase : public BuiltinFunctionTest::functionObject
{
public:
	tenaryBase(FunctionEnum function_enum, const glw::GLchar* function_name, glw::GLvoid* function_pointer,
			   const Utils::_variable_type res_type, const Utils::_variable_type arg_1_type,
			   const Utils::_variable_type arg_2_type, const Utils::_variable_type arg_3_type)
		: functionObject(function_enum, function_name, function_pointer, res_type)
		, m_arg_1_type(arg_1_type)
		, m_arg_2_type(arg_2_type)
		, m_arg_3_type(arg_3_type)
	{
	}

	virtual glw::GLuint getArgumentCount() const
	{
		return 3;
	}

	virtual Utils::_variable_type getArgumentType(glw::GLuint argument) const
	{
		switch (argument)
		{
		case 0:
			return m_arg_1_type;
			break;
		case 1:
			return m_arg_2_type;
			break;
		case 2:
			return m_arg_3_type;
			break;
		default:
			return Utils::VARIABLE_TYPE_UNKNOWN;
			break;
		}
	}

protected:
	const Utils::_variable_type m_arg_1_type;
	const Utils::_variable_type m_arg_2_type;
	const Utils::_variable_type m_arg_3_type;
};

/** Tenary function class. It treats input arguments as three variables.
 *
 * @param ResT  Type of result
 * @param Arg1T Type of first argument
 * @param Arg2T Type of second argument
 * @param Arg3T Type of third argument
 **/
template <typename ResT, typename Arg1T, typename Arg2T, typename Arg3T>
class tenary : public tenaryBase
{
public:
	typedef ResT (*functionPointer)(Arg1T, Arg2T, Arg3T);
	typedef typename TypeHelpers::referenceToType<Arg1T>::result arg1T;
	typedef typename TypeHelpers::referenceToType<Arg2T>::result arg2T;
	typedef typename TypeHelpers::referenceToType<Arg3T>::result arg3T;

	tenary(FunctionEnum function_enum, const glw::GLchar* function_name, functionPointer function_pointer)
		: tenaryBase(function_enum, function_name, (glw::GLvoid*)function_pointer, typeInfo<ResT>::variable_type,
					 typeInfo<Arg1T>::variable_type, typeInfo<Arg2T>::variable_type, typeInfo<Arg3T>::variable_type)
	{
	}

	virtual void call(glw::GLvoid* result_dst, const glw::GLvoid* argument_src) const
	{
		const glw::GLuint argument_2_offset = getArgumentOffset(1);
		const glw::GLuint argument_3_offset = getArgumentOffset(2);

		functionPointer p_function = (functionPointer)m_p_function;

		arg1T arg_1;
		arg2T arg_2;
		arg3T arg_3;
		ResT  result;

		unpack<arg1T>::get(argument_src, arg_1);
		unpack<arg2T>::get((glw::GLubyte*)argument_src + argument_2_offset, arg_2);
		unpack<arg3T>::get((glw::GLubyte*)argument_src + argument_3_offset, arg_3);

		result = p_function(arg_1, arg_2, arg_3);

		pack<ResT>::set(result_dst, result);
	}
};

/** Tenary function class. It treats input arguments as separate components.
 *

 **/
class tenaryByComponent : public tenaryBase
{
public:
	typedef glw::GLdouble (*functionPointer)(glw::GLdouble, glw::GLdouble, glw::GLdouble);

	tenaryByComponent(FunctionEnum function_enum, const glw::GLchar* function_name, functionPointer function_pointer,
					  const Utils::_variable_type res_type, const Utils::_variable_type arg_1_type,
					  const Utils::_variable_type arg_2_type, const Utils::_variable_type arg_3_type)
		: tenaryBase(function_enum, function_name, (glw::GLvoid*)function_pointer, res_type, arg_1_type, arg_2_type,
					 arg_3_type)
	{
	}

	virtual void call(glw::GLvoid* result_dst, const glw::GLvoid* argument_src) const
	{
		glw::GLdouble*		 p_result = (glw::GLdouble*)result_dst;
		const glw::GLdouble* p_arg	= (const glw::GLdouble*)argument_src;

		const glw::GLuint n_components_0 = getArgumentComponents(0);
		const glw::GLuint n_components_1 = getArgumentComponents(1);
		const glw::GLuint n_components_2 = getArgumentComponents(2);
		const glw::GLuint n_components   = de::max(de::max(n_components_0, n_components_1), n_components_2);

		const glw::GLuint component_step_0 = (1 == n_components_0) ? 0 : 1;
		const glw::GLuint component_step_1 = (1 == n_components_1) ? 0 : 1;
		const glw::GLuint component_step_2 = (1 == n_components_2) ? 0 : 1;

		functionPointer p_function = (functionPointer)m_p_function;

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			const glw::GLdouble first_arg  = p_arg[component * component_step_0];
			const glw::GLdouble second_arg = p_arg[component * component_step_1 + n_components_0];
			const glw::GLdouble third_arg  = p_arg[component * component_step_2 + n_components_0 + n_components_1];

			p_result[component] = p_function(first_arg, second_arg, third_arg);
		}
	}
};
} /* FunctionObject */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
BuiltinFunctionTest::BuiltinFunctionTest(deqp::Context& context, std::string caseName, FunctionEnum function,
										 TypeDefinition typeDefinition)
	: TestCase(context, caseName.c_str(), "Verify that built-in functions support double-precision types")
	, m_transform_feedback_buffer_id(0)
	, m_vertex_array_object_id(0)
	, m_function(function)
	, m_typeDefinition(typeDefinition)
{
	/* Nothing to be done here */
}

/** Deinitializes all GL objects that may have been created during test execution.
 *
 **/
void BuiltinFunctionTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean buffers */
	if (0 != m_transform_feedback_buffer_id)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
		gl.deleteBuffers(1, &m_transform_feedback_buffer_id);
		m_transform_feedback_buffer_id = 0;
	}

	/* Clean VAO */
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
tcu::TestNode::IterateResult BuiltinFunctionTest::iterate()
{
	/* Check if extension is supported */
	if (false == m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64"))
	{
		throw tcu::NotSupportedError("GL_ARB_gpu_shader_fp64 is not supported");
	}

	testInit();

	/* Verify result */
	typeDetails type(m_typeDefinition.n_columns, m_typeDefinition.n_rows);
	if (test(m_function, type))
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param function_enum    Function enumeration
 * @param function_name    Function name
 * @param function_pointer Pointer to routine that wiil be executed
 * @param result_type      Type of result
 **/
BuiltinFunctionTest::functionObject::functionObject(FunctionEnum function_enum, const glw::GLchar* function_name,
													glw::GLvoid* function_pointer, Utils::_variable_type result_type)
	: m_function_enum(function_enum)
	, m_function_name(function_name)
	, m_p_function(function_pointer)
	, m_res_type(result_type)
{
	/* Nothing to be done here */
}

/** Get number of components for <argument>
 *
 * @param argument Argument ordinal, starts with 0
 *
 * @return Number of components
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getArgumentComponents(glw::GLuint argument) const
{
	const Utils::_variable_type type		  = getArgumentType(argument);
	const glw::GLuint			n_components  = Utils::getNumberOfComponentsForVariableType(type);

	return n_components;
}

/** Get size in bytes of single component of <argument>
 *
 * @param argument Argument ordinal, starts with 0
 *
 * @return Size of component
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getArgumentComponentSize(glw::GLuint argument) const
{
	const Utils::_variable_type type		   = getArgumentType(argument);
	const Utils::_variable_type base_type	  = Utils::getBaseVariableType(type);
	const glw::GLuint			base_type_size = Utils::getBaseVariableTypeComponentSize(base_type);

	return base_type_size;
}

/** Get offset in bytes of <argument>. 0 is offset of first argument. Assumes tight packing.
 *
 * @param argument Argument ordinal, starts with 0
 *
 * @return Offset of arguemnt's data
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getArgumentOffset(glw::GLuint argument) const
{
	glw::GLuint result = 0;

	for (glw::GLuint i = 0; i < argument; ++i)
	{
		result += getArgumentStride(i);
	}

	return result;
}

/** Get stride in bytes of all arguments
 *
 * @return Stride of all arguments
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getArgumentStride() const
{
	const glw::GLuint n_args = getArgumentCount();
	glw::GLuint		  result = 0;

	for (glw::GLuint i = 0; i < n_args; ++i)
	{
		result += getArgumentStride(i);
	}

	return result;
}

/** Get stride in bytes of <argument>
 *
 * @param argument Argument ordinal, starts with 0
 *
 * @return Stride of argument
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getArgumentStride(glw::GLuint argument) const
{
	const glw::GLuint component_size = getArgumentComponentSize(argument);
	const glw::GLuint n_components   = getArgumentComponents(argument);

	return n_components * component_size;
}

/** Get function enumeration
 *
 * @return Function enumeration
 **/
FunctionEnum BuiltinFunctionTest::functionObject::getFunctionEnum() const
{
	return m_function_enum;
}

/** Get function name
 *
 * @return Function name
 **/
const glw::GLchar* BuiltinFunctionTest::functionObject::getName() const
{
	return m_function_name;
}

/** Get number of components for <result>
 *
 * @param result Result ordinal, starts with 0
 *
 * @return Number of components
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getResultComponents(glw::GLuint result) const
{
	const Utils::_variable_type type		  = getResultType(result);
	const glw::GLuint			n_components  = Utils::getNumberOfComponentsForVariableType(type);

	return n_components;
}

/** Get number of results
 *
 * @return Number of results
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getResultCount() const
{
	return 1;
}

/** Get offset in bytes of <result>. First result offset is 0. Assume tight packing.
 *
 * @param result Result ordinal, starts with 0
 *
 * @return Offset
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getResultOffset(glw::GLuint result) const
{
	glw::GLuint offset = 0;

	for (glw::GLuint i = 0; i < result; ++i)
	{
		offset += getResultStride(i);
		offset = deAlign32(offset, getBaseTypeSize(i));
	}

	return offset;
}

/** Get stride in bytes of <result>.
 *
 * @param result Result ordinal, starts with 0
 *
 * @return Stride
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getResultStride(glw::GLuint result) const
{
	const Utils::_variable_type type		   = getResultType(result);
	const glw::GLuint			n_components   = Utils::getNumberOfComponentsForVariableType(type);

	return n_components * getBaseTypeSize(result);
}

/** Get size in bytes of <result> base component.
 *
 * @param result Result ordinal, starts with 0
 *
 * @return Alignment
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getBaseTypeSize(glw::GLuint result) const
{
	const Utils::_variable_type type		   = getResultType(result);
	const Utils::_variable_type base_type	  = Utils::getBaseVariableType(type);
	const glw::GLuint			base_type_size = Utils::getBaseVariableTypeComponentSize(base_type);

	return base_type_size;
}

/** Get stride in bytes of all results.
 *
 * @return Stride
 **/
glw::GLuint BuiltinFunctionTest::functionObject::getResultStride() const
{
	const glw::GLuint n_results	= getResultCount();
	glw::GLuint		  stride	   = 0;
	glw::GLuint		  maxAlignment = 0;

	for (glw::GLuint i = 0; i < n_results; ++i)
	{
		const glw::GLuint alignment = getBaseTypeSize(i);
		stride += getResultStride(i);
		stride		 = deAlign32(stride, alignment);
		maxAlignment = deMaxu32(maxAlignment, alignment);
	}

	// The stride of all results must also be aligned,
	// so results for next vertex are aligned.
	return deAlign32(stride, maxAlignment);
}

/** Get type of <result>.
 *
 * @param result Result ordinal, starts with 0
 *
 * @return Type
 **/
Utils::_variable_type BuiltinFunctionTest::functionObject::getResultType(glw::GLuint /* result */) const
{
	return m_res_type;
}

/** Constructor
 *
 * @param n_columns Number of columns
 * @param n_rows    Number of rows
 **/
BuiltinFunctionTest::typeDetails::typeDetails(glw::GLuint n_columns, glw::GLuint n_rows)
	: m_n_columns(n_columns), m_n_rows(n_rows)
{
	Utils::_variable_type type = Utils::getDoubleVariableType(n_columns, n_rows);
	m_type					   = Utils::getGLDataTypeOfVariableType(type);
	m_type_name				   = Utils::getVariableTypeString(type);

	if (1 == m_n_columns)
	{
		if (1 == m_n_rows)
		{
			m_general_type = SCALAR;
		}
		else
		{
			m_general_type = VECTOR;
		}
	}
	else
	{
		m_general_type = MATRIX;
	}
}

/** Compare two values
 *
 * @param type  Type of values
 * @param left  Pointer to left value
 * @param right Pointer to right value
 *
 * @return true if values are equal, false otherwise
 **/
bool BuiltinFunctionTest::compare(Utils::_variable_type type, const glw::GLvoid* left, const glw::GLvoid* right)
{
	bool result = true;

	const glw::GLuint			n_components = Utils::getNumberOfComponentsForVariableType(type);
	const Utils::_variable_type base_type	= Utils::getBaseVariableType(type);

	switch (base_type)
	{
	case Utils::VARIABLE_TYPE_DOUBLE:

	{
		const glw::GLdouble* left_values  = (glw::GLdouble*)left;
		const glw::GLdouble* right_values = (glw::GLdouble*)right;

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			const glw::GLdouble left_value  = left_values[component];
			const glw::GLdouble right_value = right_values[component];

			if ((left_value != right_value) && (m_epsilon < de::abs(left_value - right_value)) &&
				(0 == Math::isnan_impl(left_value)) && (0 == Math::isnan_impl(right_value)))
			{
				result = false;
				break;
			}
		}
	}

	break;

	case Utils::VARIABLE_TYPE_INT:

	{
		const glw::GLint* left_values  = (glw::GLint*)left;
		const glw::GLint* right_values = (glw::GLint*)right;

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			const glw::GLint left_value  = left_values[component];
			const glw::GLint right_value = right_values[component];

			if (left_value != right_value)
			{
				result = false;
				break;
			}
		}
	}

	break;

	case Utils::VARIABLE_TYPE_UINT:

	{
		const glw::GLuint* left_values  = (glw::GLuint*)left;
		const glw::GLuint* right_values = (glw::GLuint*)right;

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			const glw::GLuint left_value  = left_values[component];
			const glw::GLuint right_value = right_values[component];

			if (left_value != right_value)
			{
				result = false;
				break;
			}
		}
	}

	break;

	default:

		TCU_FAIL("Not implemented");

		break;
	}

	return result;
}

/** Create instance of function object for given function enumeration and type
 *
 * @param function Function enumeration
 * @param type     Type details
 *
 * @return Create object
 **/
BuiltinFunctionTest::functionObject* BuiltinFunctionTest::getFunctionObject(FunctionEnum	   function,
																			const typeDetails& type)
{
	typedef tcu::Matrix<glw::GLdouble, 2, 2> DMat2;
	typedef tcu::Matrix<glw::GLdouble, 3, 2> DMat2x3;
	typedef tcu::Matrix<glw::GLdouble, 4, 2> DMat2x4;
	typedef tcu::Matrix<glw::GLdouble, 2, 3> DMat3x2;
	typedef tcu::Matrix<glw::GLdouble, 3, 3> DMat3;
	typedef tcu::Matrix<glw::GLdouble, 4, 3> DMat3x4;
	typedef tcu::Matrix<glw::GLdouble, 2, 4> DMat4x2;
	typedef tcu::Matrix<glw::GLdouble, 3, 4> DMat4x3;
	typedef tcu::Matrix<glw::GLdouble, 4, 4> DMat4;

	const glw::GLuint			n_columns	 = type.m_n_columns;
	const glw::GLuint			n_rows		  = type.m_n_rows;
	const Utils::_variable_type scalar_type   = Utils::getDoubleVariableType(1, 1);
	const Utils::_variable_type variable_type = Utils::getDoubleVariableType(n_columns, n_rows);
	const Utils::_variable_type uint_type	 = Utils::getUintVariableType(1, n_rows);
	const Utils::_variable_type int_type	  = Utils::getIntVariableType(1, n_rows);

	switch (function)
	{
	case FUNCTION_ABS:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "abs", de::abs, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_CEIL:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "ceil", ceil, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_CLAMP:

		return new FunctionObject::tenaryByComponent(function, "clamp", Math::clamp, variable_type /* res_type  */,
													 variable_type /* arg1_type */, variable_type /* arg2_type */,
													 variable_type /* arg3_type */);
		break;

	case FUNCTION_CLAMP_AGAINST_SCALAR:

		return new FunctionObject::tenaryByComponent(function, "clamp", Math::clamp, variable_type /* res_type  */,
													 variable_type /* arg1_type */, scalar_type /* arg2_type */,
													 scalar_type /* arg3_type */);
		break;

	case FUNCTION_CROSS:

		return new FunctionObject::binary<tcu::DVec3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
			function, "cross", tcu::cross);

		break;

	case FUNCTION_DETERMINANT:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DMAT2:
			return new FunctionObject::unary<glw::GLdouble /* ResT */, DMat2 /* ArgT */>(function, "determinant",
																						 Math::determinant);
		case Utils::VARIABLE_TYPE_DMAT3:
			return new FunctionObject::unary<glw::GLdouble /* ResT */, DMat3 /* ArgT */>(function, "determinant",
																						 Math::determinant);
		case Utils::VARIABLE_TYPE_DMAT4:
			return new FunctionObject::unary<glw::GLdouble /* ResT */, DMat4 /* ArgT */>(function, "determinant",
																						 Math::determinant);
		default:
			TCU_FAIL("Not implemented");
			break;
		}

		break;

	case FUNCTION_DISTANCE:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<glw::GLdouble /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "distance", tcu::distance);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<glw::GLdouble /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "distance", tcu::distance);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<glw::GLdouble /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "distance", tcu::distance);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_DOT:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<glw::GLdouble /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "dot", tcu::dot);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<glw::GLdouble /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "dot", tcu::dot);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<glw::GLdouble /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "dot", tcu::dot);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_EQUAL:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<tcu::UVec2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "equal", Math::equal);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<tcu::UVec3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "equal", Math::equal);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<tcu::UVec4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "equal", Math::equal);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_FACEFORWARD:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::tenary<tcu::DVec2 /* ResT */, const tcu::DVec2& /* Arg1T */,
											  const tcu::DVec2& /* Arg2T */, const tcu::DVec2& /* Arg3T */>(
				function, "faceforward", tcu::faceForward);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::tenary<tcu::DVec3 /* ResT */, const tcu::DVec3& /* Arg1T */,
											  const tcu::DVec3& /* Arg2T */, const tcu::DVec3& /* Arg3T */>(
				function, "faceforward", tcu::faceForward);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::tenary<tcu::DVec4 /* ResT */, const tcu::DVec4& /* Arg1T */,
											  const tcu::DVec4& /* Arg2T */, const tcu::DVec4& /* Arg3T */>(
				function, "faceforward", tcu::faceForward);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_FLOOR:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "floor", floor, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_FMA:

		return new FunctionObject::tenaryByComponent(function, "fma", Math::fma, variable_type /* res_type  */,
													 variable_type /* arg1_type */, variable_type /* arg2_type */,
													 variable_type /* arg3_type */);

		break;

	case FUNCTION_FRACT:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "fract", Math::fract, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_FREXP:

		return new FunctionObject::unaryWithOutputByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* ArgT */,
															  glw::GLint /* OutT */>(
			function, "frexp", Math::frexp, variable_type /* res_type */, variable_type /* arg_type */,
			int_type /* out_type */);

		break;

	case FUNCTION_GREATERTHAN:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<tcu::UVec2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "greaterThan", Math::greaterThan);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<tcu::UVec3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "greaterThan", Math::greaterThan);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<tcu::UVec4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "greaterThan", Math::greaterThan);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_GREATERTHANEQUAL:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<tcu::UVec2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "greaterThanEqual", Math::greaterThanEqual);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<tcu::UVec3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "greaterThanEqual", Math::greaterThanEqual);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<tcu::UVec4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "greaterThanEqual", Math::greaterThanEqual);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_INVERSE:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DMAT2:
			return new FunctionObject::unary<DMat2 /* ResT */, DMat2 /* ArgT */>(function, "inverse", Math::inverse);
			break;
		case Utils::VARIABLE_TYPE_DMAT3:
			return new FunctionObject::unary<DMat3 /* ResT */, DMat3 /* ArgT */>(function, "inverse", Math::inverse);
			break;
		case Utils::VARIABLE_TYPE_DMAT4:
			return new FunctionObject::unary<DMat4 /* ResT */, DMat4 /* ArgT */>(function, "inverse", Math::inverse);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_INVERSESQRT:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "inversesqrt", Math::inverseSqrt, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_LDEXP:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLint /* Arg2T */>(
			function, "ldexp", ::ldexp, variable_type /* res_type  */, variable_type /* arg1_type */,
			int_type /* arg2_type */);

		break;

	case FUNCTION_LESSTHAN:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<tcu::UVec2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "lessThan", Math::lessThan);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<tcu::UVec3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "lessThan", Math::lessThan);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<tcu::UVec4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "lessThan", Math::lessThan);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_LESSTHANEQUAL:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<tcu::UVec2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "lessThanEqual", Math::lessThanEqual);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<tcu::UVec3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "lessThanEqual", Math::lessThanEqual);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<tcu::UVec4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "lessThanEqual", Math::lessThanEqual);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_LENGTH:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::unary<glw::GLdouble /* ResT */, tcu::DVec2 /* ArgT */>(function, "length",
																							  tcu::length);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::unary<glw::GLdouble /* ResT */, tcu::DVec3 /* ArgT */>(function, "length",
																							  tcu::length);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::unary<glw::GLdouble /* ResT */, tcu::DVec4 /* ArgT */>(function, "length",
																							  tcu::length);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_MATRIXCOMPMULT:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "matrixCompMult", Math::multiply, variable_type /* res_type  */, variable_type /* arg1_type */,
			variable_type /* arg2_type */);

		break;

	case FUNCTION_MAX:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "max", Math::max, variable_type /* res_type  */, variable_type /* arg1_type */,
			variable_type /* arg2_type */);

		break;

	case FUNCTION_MAX_AGAINST_SCALAR:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "max", Math::max, variable_type /* res_type  */, variable_type /* arg1_type */,
			scalar_type /* arg2_type */);

		break;

	case FUNCTION_MIN:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "min", Math::min, variable_type /* res_type  */, variable_type /* arg1_type */,
			variable_type /* arg2_type */);

		break;

	case FUNCTION_MIN_AGAINST_SCALAR:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "min", Math::min, variable_type /* res_type  */, variable_type /* arg1_type */,
			scalar_type /* arg2_type */);

		break;

	case FUNCTION_MIX:

		return new FunctionObject::tenaryByComponent(function, "mix", Math::mix, variable_type /* res_type  */,
													 variable_type /* arg1_type */, variable_type /* arg2_type */,
													 variable_type /* arg3_type */);

		break;

	case FUNCTION_MOD:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "mod", Math::mod, variable_type /* res_type  */, variable_type /* arg1_type */,
			variable_type /* arg2_type */);

		break;

	case FUNCTION_MOD_AGAINST_SCALAR:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "mod", Math::mod, variable_type /* res_type  */, variable_type /* arg1_type */,
			scalar_type /* arg2_type */);

		break;

	case FUNCTION_MODF:

		return new FunctionObject::unaryWithOutputByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* ArgT */,
															  glw::GLdouble /* OutT */>(
			function, "modf", Math::modf, variable_type /* res_type */, variable_type /* arg_type */,
			variable_type /* out_type */);

		break;

	case FUNCTION_NORMALIZE:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::unary<tcu::DVec2 /* ResT */, tcu::DVec2 /* ArgT */>(function, "normalize",
																						   tcu::normalize);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::unary<tcu::DVec3 /* ResT */, tcu::DVec3 /* ArgT */>(function, "normalize",
																						   tcu::normalize);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::unary<tcu::DVec4 /* ResT */, tcu::DVec4 /* ArgT */>(function, "normalize",
																						   tcu::normalize);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_NOTEQUAL:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<tcu::UVec2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "notEqual", Math::notEqual);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<tcu::UVec3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "notEqual", Math::notEqual);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<tcu::UVec4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "notEqual", Math::notEqual);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_OUTERPRODUCT:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DMAT2:
			return new FunctionObject::binary<DMat2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		case Utils::VARIABLE_TYPE_DMAT2X3:
			return new FunctionObject::binary<DMat2x3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		case Utils::VARIABLE_TYPE_DMAT2X4:
			return new FunctionObject::binary<DMat2x4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		case Utils::VARIABLE_TYPE_DMAT3:
			return new FunctionObject::binary<DMat3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		case Utils::VARIABLE_TYPE_DMAT3X2:
			return new FunctionObject::binary<DMat3x2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		case Utils::VARIABLE_TYPE_DMAT3X4:
			return new FunctionObject::binary<DMat3x4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		case Utils::VARIABLE_TYPE_DMAT4:
			return new FunctionObject::binary<DMat4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		case Utils::VARIABLE_TYPE_DMAT4X2:
			return new FunctionObject::binary<DMat4x2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		case Utils::VARIABLE_TYPE_DMAT4X3:
			return new FunctionObject::binary<DMat4x3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "outerProduct", Math::outerProduct);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_PACKDOUBLE2X32:

		return new FunctionObject::unary<glw::GLdouble /* ResT */, tcu::UVec2 /* ArgT */>(function, "packDouble2x32",
																						  Math::packDouble2x32);

		break;

	case FUNCTION_REFLECT:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::binary<tcu::DVec2 /* ResT */, tcu::DVec2 /* Arg1T */, tcu::DVec2 /* Arg2T */>(
				function, "reflect", tcu::reflect);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::binary<tcu::DVec3 /* ResT */, tcu::DVec3 /* Arg1T */, tcu::DVec3 /* Arg2T */>(
				function, "reflect", tcu::reflect);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::binary<tcu::DVec4 /* ResT */, tcu::DVec4 /* Arg1T */, tcu::DVec4 /* Arg2T */>(
				function, "reflect", tcu::reflect);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_REFRACT:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DVEC2:
			return new FunctionObject::tenary<tcu::DVec2 /* ResT */, const tcu::DVec2& /* Arg1T */,
											  const tcu::DVec2& /* Arg2T */, glw::GLdouble /* Arg3T */>(
				function, "refract", tcu::refract);
			break;
		case Utils::VARIABLE_TYPE_DVEC3:
			return new FunctionObject::tenary<tcu::DVec3 /* ResT */, const tcu::DVec3& /* Arg1T */,
											  const tcu::DVec3& /* Arg2T */, glw::GLdouble /* Arg3T */>(
				function, "refract", tcu::refract);
			break;
		case Utils::VARIABLE_TYPE_DVEC4:
			return new FunctionObject::tenary<tcu::DVec4 /* ResT */, const tcu::DVec4& /* Arg1T */,
											  const tcu::DVec4& /* Arg2T */, glw::GLdouble /* Arg3T */>(
				function, "refract", tcu::refract);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_ROUND:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "round", Math::round, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_ROUNDEVEN:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "roundEven", Math::roundEven, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_SIGN:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "sign", Math::sign, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_SMOOTHSTEP:

		return new FunctionObject::tenaryByComponent(function, "smoothstep", Math::smoothStep,
													 variable_type /* res_type  */, variable_type /* arg1_type */,
													 variable_type /* arg2_type */, variable_type /* arg3_type */);

		break;

	case FUNCTION_SMOOTHSTEP_AGAINST_SCALAR:

		return new FunctionObject::tenaryByComponent(function, "smoothstep", Math::smoothStep,
													 variable_type /* res_type  */, scalar_type /* arg1_type */,
													 scalar_type /* arg2_type */, variable_type /* arg3_type */);

		break;

	case FUNCTION_SQRT:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "sqrt", sqrt, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_STEP:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "step", Math::step, variable_type /* res_type  */, variable_type /* arg1_type */,
			variable_type /* arg2_type */);

		break;

	case FUNCTION_STEP_AGAINST_SCALAR:

		return new FunctionObject::binaryByComponent<glw::GLdouble /* ResT */, glw::GLdouble /* Arg1T */,
													 glw::GLdouble /* Arg2T */>(
			function, "step", Math::step, variable_type /* res_type  */, scalar_type /* arg1_type */,
			variable_type /* arg2_type */);

		break;

	case FUNCTION_TRANSPOSE:

		switch (variable_type)
		{
		case Utils::VARIABLE_TYPE_DMAT2:
			return new FunctionObject::unary<DMat2 /* ResT */, DMat2 /* ArgT */>(function, "transpose",
																				 Math::transpose);
			break;
		case Utils::VARIABLE_TYPE_DMAT2X3:
			return new FunctionObject::unary<DMat2x3 /* ResT */, DMat3x2 /* ArgT */>(function, "transpose",
																					 Math::transpose);
			break;
		case Utils::VARIABLE_TYPE_DMAT2X4:
			return new FunctionObject::unary<DMat2x4 /* ResT */, DMat4x2 /* ArgT */>(function, "transpose",
																					 Math::transpose);
			break;
		case Utils::VARIABLE_TYPE_DMAT3:
			return new FunctionObject::unary<DMat3 /* ResT */, DMat3 /* ArgT */>(function, "transpose",
																				 Math::transpose);
			break;
		case Utils::VARIABLE_TYPE_DMAT3X2:
			return new FunctionObject::unary<DMat3x2 /* ResT */, DMat2x3 /* ArgT */>(function, "transpose",
																					 Math::transpose);
			break;
		case Utils::VARIABLE_TYPE_DMAT3X4:
			return new FunctionObject::unary<DMat3x4 /* ResT */, DMat4x3 /* ArgT */>(function, "transpose",
																					 Math::transpose);
			break;
		case Utils::VARIABLE_TYPE_DMAT4:
			return new FunctionObject::unary<DMat4 /* ResT */, DMat4 /* ArgT */>(function, "transpose",
																				 Math::transpose);
			break;
		case Utils::VARIABLE_TYPE_DMAT4X2:
			return new FunctionObject::unary<DMat4x2 /* ResT */, DMat2x4 /* ArgT */>(function, "transpose",
																					 Math::transpose);
			break;
		case Utils::VARIABLE_TYPE_DMAT4X3:
			return new FunctionObject::unary<DMat4x3 /* ResT */, DMat3x4 /* ArgT */>(function, "transpose",
																					 Math::transpose);
			break;
		default:
			break;
		}

		break;

	case FUNCTION_TRUNC:

		return new FunctionObject::unaryByComponent<glw::GLdouble /* ResT */>(
			function, "trunc", Math::trunc, variable_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_UNPACKDOUBLE2X32:

		return new FunctionObject::unary<tcu::UVec2 /* ResT */, glw::GLdouble /* ArgT */>(function, "unpackDouble2x32",
																						  Math::unpackDouble2x32);

		break;

	case FUNCTION_ISNAN:

		return new FunctionObject::unaryByComponent<glw::GLuint /* ResT */>(
			function, "isnan", Math::isnan_impl, uint_type /* res_type */, variable_type /* arg_type */);

		break;

	case FUNCTION_ISINF:

		return new FunctionObject::unaryByComponent<glw::GLuint /* ResT */>(
			function, "isinf", Math::isinf_impl, uint_type /* res_type */, variable_type /* arg_type */);

		break;

	default:
		TCU_FAIL("Not implemented");
		return 0;
		break;
	}

	TCU_FAIL("Not implemented");
	return 0;
}

/** Get gl.uniform* that match type of argument. Assumes that type is matrix of double.
 *
 * @param argument        Argument index
 * @param function_object Function object
 *
 * @return Function pointer
 **/
BuiltinFunctionTest::uniformDMatFunctionPointer BuiltinFunctionTest::getUniformFunctionForDMat(
	glw::GLuint argument, const functionObject& function_object) const
{
	const Utils::_variable_type argument_type = function_object.getArgumentType(argument);
	const glw::Functions&		gl			  = m_context.getRenderContext().getFunctions();

	switch (argument_type)
	{
	case Utils::VARIABLE_TYPE_DMAT2:
		return gl.uniformMatrix2dv;
		break;
	case Utils::VARIABLE_TYPE_DMAT2X3:
		return gl.uniformMatrix2x3dv;
		break;
	case Utils::VARIABLE_TYPE_DMAT2X4:
		return gl.uniformMatrix2x4dv;
		break;
	case Utils::VARIABLE_TYPE_DMAT3:
		return gl.uniformMatrix3dv;
		break;
	case Utils::VARIABLE_TYPE_DMAT3X2:
		return gl.uniformMatrix3x2dv;
		break;
	case Utils::VARIABLE_TYPE_DMAT3X4:
		return gl.uniformMatrix3x4dv;
		break;
	case Utils::VARIABLE_TYPE_DMAT4:
		return gl.uniformMatrix4dv;
		break;
	case Utils::VARIABLE_TYPE_DMAT4X2:
		return gl.uniformMatrix4x2dv;
		break;
	case Utils::VARIABLE_TYPE_DMAT4X3:
		return gl.uniformMatrix4x3dv;
		break;
	default:
		break;
	}

	TCU_FAIL("Not implemented");
	return 0;
}

/** Get gl.uniform* that match type of argument. Assumes that type is scalar or vector of double.
 *
 * @param argument        Argument index
 * @param function_object Function object
 *
 * @return Function pointer
 **/
BuiltinFunctionTest::uniformDVecFunctionPointer BuiltinFunctionTest::getUniformFunctionForDVec(
	glw::GLuint argument, const functionObject& function_object) const
{
	const Utils::_variable_type argument_type = function_object.getArgumentType(argument);
	const glw::Functions&		gl			  = m_context.getRenderContext().getFunctions();

	switch (argument_type)
	{
	case Utils::VARIABLE_TYPE_DOUBLE:
		return gl.uniform1dv;
		break;
	case Utils::VARIABLE_TYPE_DVEC2:
		return gl.uniform2dv;
		break;
	case Utils::VARIABLE_TYPE_DVEC3:
		return gl.uniform3dv;
		break;
	case Utils::VARIABLE_TYPE_DVEC4:
		return gl.uniform4dv;
		break;
	default:
		TCU_FAIL("Not implemented");
		break;
	}

	return 0;
}

/** Get gl.uniform* that match type of argument. Assumes that type is scalar or vector of signed integer.
 *
 * @param argument        Argument index
 * @param function_object Function object
 *
 * @return Function pointer
 **/
BuiltinFunctionTest::uniformIVecFunctionPointer BuiltinFunctionTest::getUniformFunctionForIVec(
	glw::GLuint argument, const functionObject& function_object) const
{
	const Utils::_variable_type argument_type = function_object.getArgumentType(argument);
	const glw::Functions&		gl			  = m_context.getRenderContext().getFunctions();

	switch (argument_type)
	{
	case Utils::VARIABLE_TYPE_INT:
		return gl.uniform1iv;
		break;
	case Utils::VARIABLE_TYPE_IVEC2:
		return gl.uniform2iv;
		break;
	case Utils::VARIABLE_TYPE_IVEC3:
		return gl.uniform3iv;
		break;
	case Utils::VARIABLE_TYPE_IVEC4:
		return gl.uniform4iv;
		break;
	default:
		TCU_FAIL("Not implemented");
		break;
	}

	return 0;
}

/** Get gl.uniform* that match type of argument. Assumes that type is scalar or vector of unsigned integer.
 *
 * @param argument        Argument index
 * @param function_object Function object
 *
 * @return Function pointer
 **/
BuiltinFunctionTest::uniformUVecFunctionPointer BuiltinFunctionTest::getUniformFunctionForUVec(
	glw::GLuint argument, const functionObject& function_object) const
{
	const Utils::_variable_type argument_type = function_object.getArgumentType(argument);
	const glw::Functions&		gl			  = m_context.getRenderContext().getFunctions();

	switch (argument_type)
	{
	case Utils::VARIABLE_TYPE_UVEC2:
		return gl.uniform2uiv;
		break;
	default:
		TCU_FAIL("Not implemented");
		break;
	}

	return 0;
}

/** Get name of uniform that will be used as <argument>.
 *
 * @param argument Argument index
 *
 * @return Name of uniform
 **/
const glw::GLchar* BuiltinFunctionTest::getUniformName(glw::GLuint argument) const
{
	switch (argument)
	{
	case 0:
		return "uniform_0";
		break;
	case 1:
		return "uniform_1";
		break;
	case 2:
		return "uniform_2";
		break;
	default:
		TCU_FAIL("Not implemented");
		return 0;
		break;
	}
}

/** Get name of varying that will be used as <result>.
 *
 * @param result Result index
 *
 * @return Name of varying
 **/
const glw::GLchar* BuiltinFunctionTest::getVaryingName(glw::GLuint result) const
{
	switch (result)
	{
	case 0:
		return "result_0";
		break;
	case 1:
		return "result_1";
		break;
	case 2:
		return "result_2";
		break;
	default:
		TCU_FAIL("Not implemented");
		return 0;
		break;
	}
}

/** Check if given combination of function and type is implemented
 *
 * @param function Function enumeration
 * @param type     Type details
 *
 * @return true if function is available for given type, false otherwise
 **/
bool BuiltinFunctionTest::isFunctionImplemented(FunctionEnum function, const typeDetails& type) const
{
	static const bool look_up_table[][3] = {
		/* SCALAR, VECTOR, MATRIX */
		/* FUNCTION_ABS:                       */ { true, true, false },
		/* FUNCTION_CEIL:                      */ { true, true, false },
		/* FUNCTION_CLAMP:                     */ { true, true, false },
		/* FUNCTION_CLAMP_AGAINST_SCALAR:      */ { false, true, false },
		/* FUNCTION_CROSS:                     */ { false, true, false },
		/* FUNCTION_DETERMINANT:               */ { false, false, true },
		/* FUNCTION_DISTANCE:                  */ { false, true, false },
		/* FUNCTION_DOT:                       */ { false, true, false },
		/* FUNCTION_EQUAL:                     */ { false, true, false },
		/* FUNCTION_FACEFORWARD:               */ { false, true, false },
		/* FUNCTION_FLOOR:                     */ { true, true, false },
		/* FUNCTION_FMA:                       */ { true, true, false },
		/* FUNCTION_FRACT:                     */ { true, true, false },
		/* FUNCTION_FREXP:                     */ { true, true, false },
		/* FUNCTION_GREATERTHAN:               */ { false, true, false },
		/* FUNCTION_GREATERTHANEQUAL:          */ { false, true, false },
		/* FUNCTION_INVERSE:                   */ { false, false, true },
		/* FUNCTION_INVERSESQRT:               */ { true, true, false },
		/* FUNCTION_LDEXP:                     */ { true, true, false },
		/* FUNCTION_LESSTHAN:                  */ { false, true, false },
		/* FUNCTION_LESSTHANEQUAL:             */ { false, true, false },
		/* FUNCTION_LENGTH:                    */ { false, true, false },
		/* FUNCTION_MATRIXCOMPMULT:            */ { false, false, true },
		/* FUNCTION_MAX:                       */ { true, true, false },
		/* FUNCTION_MAX_AGAINST_SCALAR:        */ { false, true, false },
		/* FUNCTION_MIN:                       */ { true, true, false },
		/* FUNCTION_MIN_AGAINST_SCALAR:        */ { false, true, false },
		/* FUNCTION_MIX:                       */ { true, true, false },
		/* FUNCTION_MOD:                       */ { true, true, false },
		/* FUNCTION_MOD_AGAINST_SCALAR:        */ { false, true, false },
		/* FUNCTION_MODF:                      */ { true, true, false },
		/* FUNCTION_NORMALIZE:                 */ { false, true, false },
		/* FUNCTION_NOTEQUAL:                  */ { false, true, false },
		/* FUNCTION_OUTERPRODUCT:              */ { false, false, true },
		/* FUNCTION_PACKDOUBLE2X32:            */ { true, false, false },
		/* FUNCTION_REFLECT:                   */ { false, true, false },
		/* FUNCTION_REFRACT:                   */ { false, true, false },
		/* FUNCTION_ROUND:                     */ { true, true, false },
		/* FUNCTION_ROUNDEVEN:                 */ { true, true, false },
		/* FUNCTION_SIGN:                      */ { true, false, false },
		/* FUNCTION_SMOOTHSTEP:                */ { true, true, false },
		/* FUNCTION_SMOOTHSTEP_AGAINST_SCALAR: */ { false, true, false },
		/* FUNCTION_SQRT:                      */ { true, true, false },
		/* FUNCTION_STEP:                      */ { true, true, false },
		/* FUNCTION_STEP_AGAINST_SCALAR:       */ { false, true, false },
		/* FUNCTION_TRANSPOSE:                 */ { false, false, false },
		/* FUNCTION_TRUNC:                     */ { true, true, false },
		/* FUNCTION_UNPACKDOUBLE2X32:          */ { true, false, false },
		/* FUNCTION_ISNAN:                     */ { true, true, false },
		/* FUNCTION_ISINF:                     */ { true, true, false },
	};

	bool result = look_up_table[function][type.m_general_type];

	if (true == result)
	{
		switch (function)
		{
		case FUNCTION_CROSS: /* Only 3 element vectors */
			result = (3 == type.m_n_rows);
			break;
		case FUNCTION_DETERMINANT: /* Only square matrices */
		case FUNCTION_INVERSE:
			result = (type.m_n_columns == type.m_n_rows);
			break;
		default:
			break;
		}
	}

	return result;
}

/** Logs variable of given type: name (type) [values]
 *
 * @param buffer Source of data
 * @param name   Name of variable
 * @param type   Type of variable
 **/
void BuiltinFunctionTest::logVariableType(const glw::GLvoid* buffer, const glw::GLchar* name,
										  Utils::_variable_type type) const
{
	const Utils::_variable_type base_type	= Utils::getBaseVariableType(type);
	const glw::GLuint			n_components = Utils::getNumberOfComponentsForVariableType(type);
	tcu::MessageBuilder			message		 = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	message << name << " (" << Utils::getVariableTypeString(type) << ") [";

	for (glw::GLuint component = 0; component < n_components; ++component)
	{
		if (0 != component)
		{
			message << ", ";
		}

		switch (base_type)
		{
		case Utils::VARIABLE_TYPE_DOUBLE:
			message << ((glw::GLdouble*)buffer)[component];
			break;
		case Utils::VARIABLE_TYPE_INT:
			message << ((glw::GLint*)buffer)[component];
			break;
		case Utils::VARIABLE_TYPE_UINT:
			message << ((glw::GLuint*)buffer)[component];
			break;
		default:
			TCU_FAIL("Not implemented");
		}
	}

	message << "]" << tcu::TestLog::EndMessage;
}

/** Prepare input arguments, data are stored in <buffer>
 *
 * @param function_object Function object
 * @param vertex          Vertex index
 * @param buffer          Buffer pointer
 **/
void BuiltinFunctionTest::prepareArgument(const functionObject& function_object, glw::GLuint vertex,
										  glw::GLubyte* buffer)
{
	const glw::GLuint n_arguments = function_object.getArgumentCount();

	for (glw::GLuint argument = 0; argument < n_arguments; ++argument)
	{
		const glw::GLuint offset = function_object.getArgumentOffset(argument);

		prepareComponents(function_object, vertex, argument, buffer + offset);
	}
}

/** Prepare components for given <function_object>, <vertex> and <argument>
 *
 * @param function_object Function object
 * @param vertex          Vertex index
 * @param argument        Argument index
 * @param buffer          Buffer pointer
 **/
void BuiltinFunctionTest::prepareComponents(const functionObject& function_object, glw::GLuint vertex,
											glw::GLuint argument, glw::GLubyte* buffer)
{
	glw::GLuint					argument_index[3]		 = { 0 };
	glw::GLuint					argument_reset[3]		 = { 0 };
	glw::GLuint					argument_step[3]		 = { 0 };
	glw::GLdouble				double_argument_start[3] = { 0.0 };
	const Utils::_variable_type base_arg_type = Utils::getBaseVariableType(function_object.getArgumentType(argument));
	glw::GLuint					int_argument_start  = -4;
	const glw::GLuint			n_arguments			= function_object.getArgumentCount();
	const glw::GLuint			n_components		= function_object.getArgumentComponents(argument);
	glw::GLuint					uint_argument_start = 0;

	switch (n_arguments)
	{
	case 1:
		argument_step[0]		 = 1;
		argument_reset[0]		 = 1024;
		double_argument_start[0] = -511.5;
		break;
	case 2:
		argument_step[0]		 = 32;
		argument_step[1]		 = 1;
		argument_reset[0]		 = 32;
		argument_reset[1]		 = 32;
		double_argument_start[0] = -15.5;
		double_argument_start[1] = -15.5;
		break;
	case 3:
		argument_step[0]		 = 64;
		argument_step[1]		 = 8;
		argument_step[2]		 = 1;
		argument_reset[0]		 = 16;
		argument_reset[1]		 = 8;
		argument_reset[2]		 = 8;
		double_argument_start[0] = -7.5;
		double_argument_start[1] = -3.5;
		double_argument_start[2] = -3.5;
		break;
	default:
		TCU_FAIL("Not implemented");
		return;
		break;
	};

	switch (function_object.getFunctionEnum())
	{
	case FUNCTION_CLAMP: /* arg_2 must be less than arg_3 */
	case FUNCTION_CLAMP_AGAINST_SCALAR:
		double_argument_start[2] = 4.5;
		break;
	case FUNCTION_INVERSESQRT: /* inversesqrt is undefined for argument <= 0 */
		double_argument_start[0] = 16.5;
		break;
	case FUNCTION_SMOOTHSTEP: /* arg_1 must be less than arg_2 */
	case FUNCTION_SMOOTHSTEP_AGAINST_SCALAR:
		argument_step[0]		 = 1;
		argument_step[1]		 = 8;
		argument_step[2]		 = 64;
		argument_reset[0]		 = 8;
		argument_reset[1]		 = 8;
		argument_reset[2]		 = 16;
		double_argument_start[0] = -3.5;
		double_argument_start[1] = 4.5;
		double_argument_start[2] = -7.5;
		break;
	default:
		break;
	}

	for (glw::GLuint i = 0; i < n_arguments; ++i)
	{
		argument_index[i] = (vertex / argument_step[i]) % argument_reset[i];
	}

	switch (base_arg_type)
	{
	case Utils::VARIABLE_TYPE_DOUBLE:
	{
		glw::GLdouble* argument_dst = (glw::GLdouble*)buffer;

		double_argument_start[argument] += argument_index[argument];

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			glw::GLdouble value = double_argument_start[argument] + ((glw::GLdouble)component) / 8.0;

			switch (function_object.getFunctionEnum())
			{
			case FUNCTION_ROUND: /* Result for 0.5 depends on implementation */
				if (0.5 == Math::fract(value))
				{
					value += 0.01;
				}
				break;
			default:
				break;
			}

			argument_dst[component] = value;
		}
	}
	break;
	case Utils::VARIABLE_TYPE_INT:
	{
		glw::GLint* argument_dst = (glw::GLint*)buffer;

		uint_argument_start += argument_index[argument];

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			const glw::GLint value = int_argument_start + component;

			argument_dst[component] = value;
		}
	}
	break;
	case Utils::VARIABLE_TYPE_UINT:
	{
		glw::GLuint* argument_dst = (glw::GLuint*)buffer;

		uint_argument_start += argument_index[argument];

		for (glw::GLuint component = 0; component < n_components; ++component)
		{
			const glw::GLuint value = uint_argument_start + component;

			argument_dst[component] = value;
		}
	}
	break;
	default:
		TCU_FAIL("Not implemented");
		return;
		break;
	}
}

/** Prepare programInfo for given functionObject
 *
 * @param function_object  Function object
 * @param out_program_info Program info
 **/
void BuiltinFunctionTest::prepareProgram(const functionObject& function_object, Utils::programInfo& out_program_info)
{
	const glw::GLuint		  n_varying_names  = function_object.getResultCount();
	static const glw::GLchar* varying_names[3] = { getVaryingName(0), getVaryingName(1), getVaryingName(2) };

	prepareVertexShaderCode(function_object);

	out_program_info.build(0 /* cs */, 0 /* fs */, 0 /* gs */, 0 /* tcs */, 0 /* tes */, m_vertex_shader_code.c_str(),
						   varying_names, n_varying_names);
}

/** Prepare input data and expected results for given function object
 *
 * @param function_object Function object
 **/
void BuiltinFunctionTest::prepareTestData(const functionObject& function_object)
{
	const glw::GLuint result_stride		   = function_object.getResultStride();
	const glw::GLuint result_buffer_size   = result_stride * m_n_veritces;
	const glw::GLuint argument_stride	  = function_object.getArgumentStride();
	const glw::GLuint argument_buffer_size = m_n_veritces * argument_stride;

	m_argument_data.clear();
	m_expected_results_data.clear();

	m_argument_data.resize(argument_buffer_size);
	m_expected_results_data.resize(result_buffer_size);

	for (glw::GLuint vertex = 0; vertex < m_n_veritces; ++vertex)
	{
		const glw::GLuint result_offset   = vertex * result_stride;
		glw::GLdouble*	result_dst	  = (glw::GLdouble*)&m_expected_results_data[result_offset];
		const glw::GLuint argument_offset = vertex * argument_stride;
		glw::GLubyte*	 argument_dst	= &m_argument_data[argument_offset];

		prepareArgument(function_object, vertex, argument_dst);
		function_object.call(result_dst, argument_dst);
	}
}

/** Prepare source code of vertex shader for given function object. Result is stored in m_vertex_shader_code.
 *
 * @param function_object Function object
 **/
void BuiltinFunctionTest::prepareVertexShaderCode(const functionObject& function_object)
{
	static const glw::GLchar* shader_template_code = "#version 400 core\n"
													 "\n"
													 "precision highp float;\n"
													 "\n"
													 "ARGUMENT_DEFINITION"
													 "\n"
													 "RESULT_DEFINITION"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    RESULT_NAME = RESULT_TYPE(FUNCTION_NAME(ARGUMENT));\n"
													 "}\n"
													 "\n";

	static const glw::GLchar* argument_definition_token = "ARGUMENT_DEFINITION";
	static const glw::GLchar* argument_token			= "ARGUMENT";
	static const glw::GLchar* function_name_token		= "FUNCTION_NAME";
	static const glw::GLchar* result_definition_token   = "RESULT_DEFINITION";
	static const glw::GLchar* result_name_token			= "RESULT_NAME";
	static const glw::GLchar* result_type_token			= "RESULT_TYPE";
	static const glw::GLchar* uniform_name_token		= "UNIFORM_NAME";
	static const glw::GLchar* uniform_type_token		= "UNIFORM_TYPE";

	static const glw::GLchar* argument_definition = "uniform UNIFORM_TYPE UNIFORM_NAME;\nARGUMENT_DEFINITION";
	static const glw::GLchar* argument_str		  = ", UNIFORM_NAMEARGUMENT";
	static const glw::GLchar* first_argument	  = "UNIFORM_NAMEARGUMENT";
	static const glw::GLchar* result_definition   = "flat out RESULT_TYPE RESULT_NAME;\nRESULT_DEFINITION";

	const glw::GLuint argument_definition_length = (glw::GLuint)strlen(argument_definition);
	const glw::GLuint first_argument_length		 = (glw::GLuint)strlen(first_argument);
	const glw::GLuint n_arguments				 = function_object.getArgumentCount();
	const glw::GLuint n_results					 = function_object.getResultCount();
	const glw::GLuint result_definition_length   = (glw::GLuint)strlen(result_definition);
	std::string		  result_type				 = Utils::getVariableTypeString(function_object.getResultType(0));

	size_t		search_position = 0;
	std::string string			= shader_template_code;

	/* Replace ARGUMENT_DEFINITION with definitions */
	for (glw::GLuint argument = 0; argument < n_arguments; ++argument)
	{
		Utils::_variable_type argument_type = function_object.getArgumentType(argument);
		const glw::GLchar*	uniform_name  = getUniformName(argument);
		std::string			  uniform_type  = Utils::getVariableTypeString(argument_type);

		Utils::replaceToken(argument_definition_token, search_position, argument_definition, string);

		search_position -= argument_definition_length;

		Utils::replaceToken(uniform_type_token, search_position, uniform_type.c_str(), string);
		Utils::replaceToken(uniform_name_token, search_position, uniform_name, string);
	}

	/* Remove ARGUMENT_DEFINITION */
	Utils::replaceToken(argument_definition_token, search_position, "", string);

	/* Replace RESULT_DEFINITION with definitions */
	for (glw::GLuint result = 0; result < n_results; ++result)
	{
		Utils::_variable_type variable_type = function_object.getResultType(result);
		const glw::GLchar*	varying_name  = getVaryingName(result);
		std::string			  varying_type  = Utils::getVariableTypeString(variable_type);

		Utils::replaceToken(result_definition_token, search_position, result_definition, string);

		search_position -= result_definition_length;

		Utils::replaceToken(result_type_token, search_position, varying_type.c_str(), string);
		Utils::replaceToken(result_name_token, search_position, varying_name, string);
	}

	/* Remove RESULT_DEFINITION */
	Utils::replaceToken(result_definition_token, search_position, "", string);

	/* Replace RESULT_NAME */
	Utils::replaceToken(result_name_token, search_position, getVaryingName(0), string);

	/* Replace RESULT_TYPE */
	Utils::replaceToken(result_type_token, search_position, result_type.c_str(), string);

	/* Replace FUNCTION_NAME */
	Utils::replaceToken(function_name_token, search_position, function_object.getName(), string);

	/* Replace ARGUMENT with list of arguments */
	for (glw::GLuint argument = 0; argument < n_arguments; ++argument)
	{
		const glw::GLchar* uniform_name = getUniformName(argument);

		if (0 == argument)
		{
			Utils::replaceToken(argument_token, search_position, first_argument, string);
		}
		else
		{
			Utils::replaceToken(argument_token, search_position, argument_str, string);
		}

		search_position -= first_argument_length;

		Utils::replaceToken(uniform_name_token, search_position, uniform_name, string);
	}

	for (glw::GLuint result = 1; result < n_results; ++result)
	{
		const glw::GLchar* varying_name = getVaryingName(result);

		Utils::replaceToken(argument_token, search_position, argument_str, string);

		search_position -= first_argument_length;

		Utils::replaceToken(uniform_name_token, search_position, varying_name, string);
	}

	/* Remove ARGUMENT */
	Utils::replaceToken(argument_token, search_position, "", string);

	m_vertex_shader_code = string;
}

/** Test single function with one type
 *
 * param function Function enumeration
 * param type     Type details
 *
 * @return true if test pass (or function is not available for <type>), false otherwise
 **/
bool BuiltinFunctionTest::test(FunctionEnum function, const typeDetails& type)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Skip if function is not implemented for type */
	if (false == isFunctionImplemented(function, type))
	{
		return true;
	}

	Utils::programInfo			  program(m_context);
	de::UniquePtr<functionObject> function_object(getFunctionObject(function, type));

	prepareProgram(*function_object, program);
	prepareTestData(*function_object);

	/* Set up program */
	gl.useProgram(program.m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	for (glw::GLuint vertex = 0; vertex < m_n_veritces; ++vertex)
	{
		testBegin(*function_object, program.m_program_object_id, vertex);

		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

		if (false == verifyResults(*function_object, vertex))
		{
			return false;
		}
	}

	return true;
}

/** Update transform feedback buffer and uniforms
 *
 * @param function_object Function object
 * @param program_id      Program object id
 * @param vertex          Vertex index
 **/
void BuiltinFunctionTest::testBegin(const functionObject& function_object, glw::GLuint program_id, glw::GLuint vertex)
{
	const glw::GLuint	 arguments_stride   = function_object.getArgumentStride();
	const glw::Functions& gl				 = m_context.getRenderContext().getFunctions();
	const glw::GLuint	 n_arguments		 = function_object.getArgumentCount();
	const glw::GLuint	 result_buffer_size = function_object.getResultStride();
	const glw::GLuint	 vertex_offset		 = arguments_stride * vertex;

	/* Update transform feedback buffer */
	std::vector<glw::GLubyte> transform_feedback_buffer_data;
	transform_feedback_buffer_data.resize(result_buffer_size);

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transform_feedback_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, result_buffer_size, &transform_feedback_buffer_data[0],
				  GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");

	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_transform_feedback_buffer_id, 0 /* offset */,
					   result_buffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");

	/* Update VAO */
	gl.bindVertexArray(m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");

	for (glw::GLuint argument = 0; argument < n_arguments; ++argument)
	{
		const glw::GLuint			argument_offset  = function_object.getArgumentOffset(argument);
		const Utils::_variable_type argument_type	= function_object.getArgumentType(argument);
		const glw::GLuint			n_columns		 = Utils::getNumberOfColumnsForVariableType(argument_type);
		const glw::GLchar*			uniform_name	 = getUniformName(argument);
		const glw::GLint			uniform_location = gl.getUniformLocation(program_id, uniform_name);
		const glw::GLdouble*		uniform_src = (glw::GLdouble*)&m_argument_data[vertex_offset + argument_offset];

		GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

		if (-1 == uniform_location)
		{
			TCU_FAIL("Inactive uniform");
		}

		if (1 == n_columns)
		{
			switch (Utils::getBaseVariableType(argument_type))
			{
			case Utils::VARIABLE_TYPE_DOUBLE:
			{
				uniformDVecFunctionPointer p_uniform_function = getUniformFunctionForDVec(argument, function_object);

				p_uniform_function(uniform_location, 1 /* count */, uniform_src);
			}
			break;
			case Utils::VARIABLE_TYPE_UINT:
			{
				uniformUVecFunctionPointer p_uniform_function = getUniformFunctionForUVec(argument, function_object);

				p_uniform_function(uniform_location, 1 /* count */, (glw::GLuint*)uniform_src);
			}
			break;
			case Utils::VARIABLE_TYPE_INT:
			{
				uniformIVecFunctionPointer p_uniform_function = getUniformFunctionForIVec(argument, function_object);

				p_uniform_function(uniform_location, 1 /* count */, (glw::GLint*)uniform_src);
			}
			break;
			default:
				TCU_FAIL("Not implemented");
				break;
			}
		}
		else
		{
			uniformDMatFunctionPointer p_uniform_function = getUniformFunctionForDMat(argument, function_object);

			p_uniform_function(uniform_location, 1 /* count */, GL_FALSE /* transpose */, uniform_src);
		}
	}
}

/** Init GL obejcts
 *
 **/
void BuiltinFunctionTest::testInit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genBuffers(1, &m_transform_feedback_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

	gl.genVertexArrays(1, &m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

	gl.enable(GL_RASTERIZER_DISCARD);
}

/** Compare contents of transform feedback buffer with expected results
 *
 * @param function_object Function object
 * @param vertex          Vertex index
 *
 * @return true if all results are as expected, false otherwise
 **/
bool BuiltinFunctionTest::verifyResults(const functionObject& function_object, glw::GLuint vertex)
{
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	bool				  test_result	  = true;
	const glw::GLuint	 n_results		   = function_object.getResultCount();
	const glw::GLuint	 results_stride   = function_object.getResultStride();
	const glw::GLuint	 results_offset   = vertex * results_stride;
	const glw::GLubyte*   expected_results = &m_expected_results_data[results_offset];

	/* Get transform feedback data */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transform_feedback_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

	glw::GLubyte* feedback_data = (glw::GLubyte*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	for (glw::GLuint result = 0; result < n_results; ++result)
	{
		const Utils::_variable_type result_type   = function_object.getResultType(result);
		const glw::GLuint			result_offset = function_object.getResultOffset(result);

		const glw::GLvoid* expected_result_src = expected_results + result_offset;
		const glw::GLvoid* result_src		   = feedback_data + result_offset;

		if (false == compare(result_type, expected_result_src, result_src))
		{
			test_result = false;
			break;
		}
	}

	/* Unmap transform feedback buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	if (false == test_result)
	{
		const glw::GLuint argument_stride  = function_object.getArgumentStride();
		const glw::GLuint arguments_offset = vertex * argument_stride;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid result."
											<< tcu::TestLog::EndMessage;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Function: " << function_object.getName()
											<< tcu::TestLog::EndMessage;

		for (glw::GLuint result = 0; result < n_results; ++result)
		{
			const Utils::_variable_type result_type   = function_object.getResultType(result);
			const glw::GLuint			result_offset = function_object.getResultOffset(result);

			const glw::GLvoid* expected_result_src = expected_results + result_offset;
			const glw::GLvoid* result_src		   = feedback_data + result_offset;

			logVariableType(result_src, "Result", result_type);
			logVariableType(expected_result_src, "Expected result", result_type);
		}

		for (glw::GLuint argument = 0; argument < function_object.getArgumentCount(); ++argument)
		{
			const glw::GLuint   argument_offset = function_object.getArgumentOffset(argument);
			const glw::GLubyte* argument_src	= &m_argument_data[arguments_offset + argument_offset];

			logVariableType(argument_src, "Argument", function_object.getArgumentType(argument));
		}

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader:\n"
											<< m_vertex_shader_code << tcu::TestLog::EndMessage;
	}

	return test_result;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
GPUShaderFP64Tests::GPUShaderFP64Tests(deqp::Context& context)
	: TestCaseGroup(context, "gpu_shader_fp64", "Verifies \"gpu_shader_fp64\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void GPUShaderFP64Tests::init(void)
{
	TestCaseGroup* fp64 = new TestCaseGroup(m_context, "fp64", "");
	fp64->addChild(new GPUShaderFP64Test1(m_context));
	fp64->addChild(new GPUShaderFP64Test2(m_context));
	fp64->addChild(new GPUShaderFP64Test3(m_context));
	fp64->addChild(new GPUShaderFP64Test4(m_context));
	fp64->addChild(new GPUShaderFP64Test5(m_context));
	fp64->addChild(new GPUShaderFP64Test6(m_context));
	fp64->addChild(new GPUShaderFP64Test7(m_context));
	fp64->addChild(new GPUShaderFP64Test8(m_context));
	fp64->addChild(new GPUShaderFP64Test9(m_context));
	addChild(fp64);

	TypeDefinition typeDefinition[] =
	{
		{ "double",  1, 1 },
		{ "dvec2",   1, 2 },
		{ "dvec3",   1, 3 },
		{ "dvec4",   1, 4 },
		{ "dmat2",   2, 2 },
		{ "dmat2x3", 2, 3 },
		{ "dmat2x4", 2, 4 },
		{ "dmat3x2", 3, 2 },
		{ "dmat3",   3, 3 },
		{ "dmat3x4", 3, 4 },
		{ "dmat4x2", 4, 2 },
		{ "dmat4x3", 4, 3 },
		{ "dmat4",   4, 4 }
	};

	struct BuiltinFunctions
	{
		std::string  name;
		FunctionEnum function;
	} builtinFunctions[] = {
		{ "abs",						FUNCTION_ABS },
		{ "ceil",						FUNCTION_CEIL },
		{ "clamp",						FUNCTION_CLAMP },
		{ "clamp_against_scalar",		FUNCTION_CLAMP_AGAINST_SCALAR },
		{ "cross",						FUNCTION_CROSS },
		{ "determinant",				FUNCTION_DETERMINANT },
		{ "distance",					FUNCTION_DISTANCE },
		{ "dot",						FUNCTION_DOT },
		{ "equal",						FUNCTION_EQUAL },
		{ "faceforward",				FUNCTION_FACEFORWARD },
		{ "floor",						FUNCTION_FLOOR },
		{ "fma",						FUNCTION_FMA },
		{ "fract",						FUNCTION_FRACT },
		{ "frexp",						FUNCTION_FREXP },
		{ "greaterthan",				FUNCTION_GREATERTHAN },
		{ "greaterthanequal",			FUNCTION_GREATERTHANEQUAL },
		{ "inverse",					FUNCTION_INVERSE },
		{ "inversesqrt",				FUNCTION_INVERSESQRT },
		{ "ldexp",						FUNCTION_LDEXP },
		{ "lessthan",					FUNCTION_LESSTHAN },
		{ "lessthanequal",				FUNCTION_LESSTHANEQUAL },
		{ "length",						FUNCTION_LENGTH },
		{ "matrixcompmult",				FUNCTION_MATRIXCOMPMULT },
		{ "max",						FUNCTION_MAX },
		{ "max_against_scalar",			FUNCTION_MAX_AGAINST_SCALAR },
		{ "min",						FUNCTION_MIN },
		{ "min_against_scalar",			FUNCTION_MIN_AGAINST_SCALAR },
		{ "mix",						FUNCTION_MIX },
		{ "mod",						FUNCTION_MOD },
		{ "mod_against_scalar",			FUNCTION_MOD_AGAINST_SCALAR },
		{ "modf",						FUNCTION_MODF },
		{ "normalize",					FUNCTION_NORMALIZE },
		{ "notequal",					FUNCTION_NOTEQUAL },
		{ "outerproduct",				FUNCTION_OUTERPRODUCT },
		{ "packdouble2x32",				FUNCTION_PACKDOUBLE2X32 },
		{ "reflect",					FUNCTION_REFLECT },
		{ "refract",					FUNCTION_REFRACT },
		{ "round",						FUNCTION_ROUND },
		{ "roundeven",					FUNCTION_ROUNDEVEN },
		{ "sign",						FUNCTION_SIGN },
		{ "smoothstep",					FUNCTION_SMOOTHSTEP },
		{ "smoothstep_against_scalar",	FUNCTION_SMOOTHSTEP_AGAINST_SCALAR },
		{ "sqrt",						FUNCTION_SQRT },
		{ "step",						FUNCTION_STEP },
		{ "step_against_scalar",		FUNCTION_STEP_AGAINST_SCALAR },
		{ "transpose",					FUNCTION_TRANSPOSE },
		{ "trunc",						FUNCTION_TRUNC },
		{ "unpackdouble2x32",			FUNCTION_UNPACKDOUBLE2X32 },
		{ "isnan",						FUNCTION_ISNAN },
		{ "isinf",						FUNCTION_ISINF }
	};

	TestCaseGroup* builin = new TestCaseGroup(m_context, "builtin", "");
	for (int i = 0; i < DE_LENGTH_OF_ARRAY(builtinFunctions); ++i)
	{
		const BuiltinFunctions& bf = builtinFunctions[i];
		for (int j = 0; j < DE_LENGTH_OF_ARRAY(typeDefinition); ++j)
		{
			std::string caseName = bf.name + "_" + typeDefinition[j].name;
			builin->addChild(new BuiltinFunctionTest(m_context, caseName, bf.function, typeDefinition[j]));
		}
	}
	addChild(builin);
}

} /* glcts namespace */
