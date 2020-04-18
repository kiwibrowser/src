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
 * \file  gl4cShadingLanguage420PackTests.cpp
 * \brief Implements conformance tests for "Shading Language 420Pack" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cShadingLanguage420PackTests.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#define IS_DEBUG 0

using namespace glw;

namespace gl4cts
{

namespace GLSL420Pack
{
/** Check binding of uniform
 *
 * @param program          Program object
 * @param name             Array name
 * @param expected_binding Expected binding value
 *
 * @return true if binding is as expected, false otherwise
 **/
bool Utils::checkUniformBinding(Utils::program& program, const glw::GLchar* name, glw::GLint expected_binding)
{
	const GLint uniform_location = program.getUniformLocation(name);
	if (-1 == uniform_location)
	{
		TCU_FAIL("Uniform is inactive");
	}

	GLint binding = program.getUniform1i(uniform_location);

	return (expected_binding == binding);
}
/** Check binding of uniform array element at <index>
 *
 * @param program          Program object
 * @param name             Array name
 * @param index            Index
 * @param expected_binding Expected binding value
 *
 * @return true if binding is as expected, false otherwise
 **/
bool Utils::checkUniformArrayBinding(Utils::program& program, const glw::GLchar* name, glw::GLuint index,
									 glw::GLint expected_binding)
{
	GLchar buffer[64];
	sprintf(buffer, "%s[%d]", name, index);

	const GLint uniform_location = program.getUniformLocation(buffer);
	if (-1 == uniform_location)
	{
		TCU_FAIL("Uniform is inactive");
	}

	GLint binding = program.getUniform1i(uniform_location);

	return (expected_binding == binding);
}

/** Check if given qualifier is present in set
 *
 * @param qualifier  Specific qualifier
 * @param qualifiers Qualifiers' set
 *
 * @return true if qualifier is present, false otherwise
 **/
bool Utils::doesContainQualifier(Utils::QUALIFIERS qualifier, const Utils::qualifierSet& qualifiers)
{
	for (GLuint i = 0; i < qualifiers.size(); ++i)
	{
		if (qualifiers[i] == qualifier)
		{
			return true;
		}
	}

	return false;
}

/** Check if given stage supports specific qualifier
 *
 * @param stage     Shader stage
 * @param storage   Storage of variable
 * @param qualifier Qualifier
 *
 * @return true if qualifier can be used in given stage, false otherwise
 **/
bool Utils::doesStageSupportQualifier(Utils::SHADER_STAGES stage, Utils::VARIABLE_STORAGE storage,
									  Utils::QUALIFIERS qualifier)
{
	bool result = true;

	switch (stage)
	{
	case COMPUTE_SHADER:
		switch (qualifier)
		{
		case QUAL_NONE:
		case QUAL_UNIFORM:
		case QUAL_LOWP:
		case QUAL_MEDIUMP:
		case QUAL_HIGHP:
		case QUAL_INVARIANT:
			result = true;
			break;
		default:
			result = false;
			break;
		}
		break;
	case FRAGMENT_SHADER:
		if (QUAL_PATCH == qualifier)
		{
			result = false;
		}
		else if ((OUTPUT == storage) &&
				 ((QUAL_SMOOTH == qualifier) || (QUAL_NOPERSPECTIVE == qualifier) || (QUAL_FLAT == qualifier)))
		{
			result = false;
		}
		break;
	case VERTEX_SHADER:
		if (QUAL_PATCH == qualifier)
		{
			result = false;
		}
		else if ((INPUT == storage) &&
				 ((QUAL_SMOOTH == qualifier) || (QUAL_NOPERSPECTIVE == qualifier) || (QUAL_FLAT == qualifier) ||
				  (QUAL_INVARIANT == qualifier) || (QUAL_CENTROID == qualifier) || (QUAL_SAMPLE == qualifier)))
		{
			result = false;
		}
		break;
	case GEOMETRY_SHADER:
		if (QUAL_PATCH == qualifier)
		{
			result = false;
		}
		break;
	case TESS_CTRL_SHADER:
		if ((INPUT == storage) && (QUAL_PATCH == qualifier))
		{
			result = false;
		}
		break;
	case TESS_EVAL_SHADER:
		if ((OUTPUT == storage) && (QUAL_PATCH == qualifier))
		{
			result = false;
		}
		break;
	default:
		break;
	}

	return result;
}

/** Get string for qualifier
 *
 * @param qualifier Qualifier
 *
 * @return A string for given qualifier
 **/
const GLchar* Utils::getQualifierString(Utils::QUALIFIERS qualifier)
{
	const GLchar* result = 0;
	switch (qualifier)
	{
	case QUAL_NONE:
		result = "";
		break;
	case QUAL_CONST:
		result = "const";
		break;
	case QUAL_IN:
		result = "in";
		break;
	case QUAL_OUT:
		result = "out";
		break;
	case QUAL_INOUT:
		result = "inout";
		break;
	case QUAL_UNIFORM:
		result = "uniform";
		break;
	case QUAL_PATCH:
		result = "patch";
		break;
	case QUAL_CENTROID:
		result = "centroid";
		break;
	case QUAL_SAMPLE:
		result = "sample";
		break;
	case QUAL_FLAT:
		result = "flat";
		break;
	case QUAL_NOPERSPECTIVE:
		result = "noperspective";
		break;
	case QUAL_SMOOTH:
		result = "smooth";
		break;
	case QUAL_LOCATION:
		result = "layout (location = LOC_VALUE)";
		break;
	case QUAL_LOWP:
		result = "lowp";
		break;
	case QUAL_MEDIUMP:
		result = "mediump";
		break;
	case QUAL_HIGHP:
		result = "highp";
		break;
	case QUAL_PRECISE:
		result = "precise";
		break;
	case QUAL_INVARIANT:
		result = "invariant";
		break;
	default:
		TCU_FAIL("Invalid enum");
	};

	return result;
}

/** Returns a string with set of qualifiers.
 *
 * @param qualifiers Set of qualifiers
 *
 * @return String
 **/
std::string Utils::getQualifiersListString(const qualifierSet& qualifiers)
{
	static const GLchar* qualifier_list		   = "QUALIFIER QUALIFIER_LIST";
	const GLuint		 qualifier_list_length = static_cast<GLuint>(strlen(qualifier_list));

	/* Tokens */
	static const GLchar* token_qualifier = "QUALIFIER";
	static const GLchar* token_qual_list = "QUALIFIER_LIST";

	/* Variables */
	std::string list	 = token_qual_list;
	size_t		position = 0;

	/* Replace tokens */
	for (GLuint i = 0; i < qualifiers.size(); ++i)
	{
		Utils::replaceToken(token_qual_list, position, qualifier_list, list);
		position -= qualifier_list_length;

		const GLchar* qualifier_str = getQualifierString(qualifiers[i]);

		Utils::replaceToken(token_qualifier, position, qualifier_str, list);
	}

	Utils::replaceToken(token_qual_list, position, "", list);

	return list;
}

/** Prepare a set of qualifiers for given shader stage and variable storage.
 * Filters out not supported qualifiers from in_qualifiers
 *
 * @param in_qualifiers Origiranl set of qualifiers
 * @param stage         Shader stage
 * @param storage       Variable storage
 *
 * @return Set of qualifiers
 **/
Utils::qualifierSet Utils::prepareQualifiersSet(const qualifierSet& in_qualifiers, SHADER_STAGES stage,
												VARIABLE_STORAGE storage)
{
	qualifierSet result;

	for (GLuint i = 0; i < in_qualifiers.size(); ++i)
	{
		Utils::QUALIFIERS qualifier = in_qualifiers[i];

		if (false == doesStageSupportQualifier(stage, storage, qualifier))
		{
			continue;
		};

		/* Replace wrong storage qualifiers */
		if ((Utils::INPUT == storage) && ((Utils::QUAL_UNIFORM == qualifier) || (Utils::QUAL_OUT == qualifier)))
		{
			qualifier = QUAL_IN;
		}
		else if ((Utils::OUTPUT == storage) && ((Utils::QUAL_IN == qualifier) || (Utils::QUAL_UNIFORM == qualifier)))
		{
			qualifier = QUAL_OUT;
		}
		else if ((Utils::UNIFORM == storage) && ((Utils::QUAL_IN == qualifier) || (Utils::QUAL_OUT == qualifier)))
		{
			qualifier = QUAL_UNIFORM;
		}

		result.push_back(qualifier);
	}

	return result;
}

/** Get image type for given texture type
 *
 * @param type Texture type
 *
 * @return String representing sampler type
 **/
const GLchar* Utils::getImageType(Utils::TEXTURE_TYPES type)
{
	const GLchar* result = 0;

	switch (type)
	{
	case TEX_BUFFER:
		result = "imageBuffer";
		break;
	case TEX_2D:
		result = "image2D";
		break;
	case TEX_2D_RECT:
		result = "image2DRect";
		break;
	case TEX_2D_ARRAY:
		result = "image2DArray";
		break;
	case TEX_3D:
		result = "image3D";
		break;
	case TEX_CUBE:
		result = "imageCube";
		break;
	case TEX_1D:
		result = "image1D";
		break;
	case TEX_1D_ARRAY:
		result = "image1DArray";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return result;
}

/** Get number of coordinates required to address texture of given type
 *
 * @param type Type of texture
 *
 * @return Number of coordinates
 **/
GLuint Utils::getNumberOfCoordinates(Utils::TEXTURE_TYPES type)
{
	GLuint result = 0;

	switch (type)
	{
	case TEX_BUFFER:
		result = 1;
		break;
	case TEX_2D:
		result = 2;
		break;
	case TEX_2D_RECT:
		result = 2;
		break;
	case TEX_2D_ARRAY:
		result = 3;
		break;
	case TEX_3D:
		result = 3;
		break;
	case TEX_CUBE:
		result = 3;
		break;
	case TEX_1D:
		result = 1;
		break;
	case TEX_1D_ARRAY:
		result = 2;
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return result;
}

/** Get sampler type for given texture type
 *
 * @param type Texture type
 *
 * @return String representing sampler type
 **/
const GLchar* Utils::getSamplerType(Utils::TEXTURE_TYPES type)
{
	const GLchar* result = 0;

	switch (type)
	{
	case TEX_BUFFER:
		result = "samplerBuffer";
		break;
	case TEX_2D:
		result = "sampler2D";
		break;
	case TEX_2D_RECT:
		result = "sampler2DRect";
		break;
	case TEX_2D_ARRAY:
		result = "sampler2DArray";
		break;
	case TEX_3D:
		result = "sampler3D";
		break;
	case TEX_CUBE:
		result = "samplerCube";
		break;
	case TEX_1D:
		result = "sampler1D";
		break;
	case TEX_1D_ARRAY:
		result = "sampler1DArray";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return result;
}

/** Get target for given texture type
 *
 * @param type Type of texture
 *
 * @return Target
 **/
GLenum Utils::getTextureTartet(Utils::TEXTURE_TYPES type)
{
	GLenum result = 0;

	switch (type)
	{
	case TEX_BUFFER:
		result = GL_TEXTURE_BUFFER;
		break;
	case TEX_2D:
		result = GL_TEXTURE_2D;
		break;
	case TEX_2D_RECT:
		result = GL_TEXTURE_RECTANGLE;
		break;
	case TEX_2D_ARRAY:
		result = GL_TEXTURE_2D_ARRAY;
		break;
	case TEX_3D:
		result = GL_TEXTURE_3D;
		break;
	case TEX_CUBE:
		result = GL_TEXTURE_CUBE_MAP;
		break;
	case TEX_1D:
		result = GL_TEXTURE_1D;
		break;
	case TEX_1D_ARRAY:
		result = GL_TEXTURE_1D_ARRAY;
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return result;
}

/** Get name of given texture type
 *
 * @param type Texture type
 *
 * @return String representing name of texture type
 **/
const GLchar* Utils::getTextureTypeName(Utils::TEXTURE_TYPES type)
{
	const GLchar* result = 0;

	switch (type)
	{
	case TEX_BUFFER:
		result = "buffer";
		break;
	case TEX_2D:
		result = "2D";
		break;
	case TEX_2D_RECT:
		result = "2D rectangle";
		break;
	case TEX_2D_ARRAY:
		result = "2D array";
		break;
	case TEX_3D:
		result = "3D";
		break;
	case TEX_CUBE:
		result = "cube";
		break;
	case TEX_1D:
		result = "1D";
		break;
	case TEX_1D_ARRAY:
		result = "1D array";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return result;
}

/** Check if glsl support matrices for specific basic type
 *
 * @param type Basic type
 *
 * @return true if matrices of <type> are supported, false otherwise
 **/
bool Utils::doesTypeSupportMatrix(TYPES type)
{
	bool result = false;

	switch (type)
	{
	case FLOAT:
	case DOUBLE:
		result = true;
		break;
	case INT:
	case UINT:
		result = false;
		break;
	default:
		TCU_FAIL("Invliad enum");
	}

	return result;
}

/** Get string representing name of shader stage
 *
 * @param stage Shader stage
 *
 * @return String with name of shader stage
 **/
const glw::GLchar* Utils::getShaderStageName(Utils::SHADER_STAGES stage)
{
	const GLchar* result = 0;

	switch (stage)
	{
	case COMPUTE_SHADER:
		result = "compute";
		break;
	case VERTEX_SHADER:
		result = "vertex";
		break;
	case TESS_CTRL_SHADER:
		result = "tesselation control";
		break;
	case TESS_EVAL_SHADER:
		result = "tesselation evaluation";
		break;
	case GEOMETRY_SHADER:
		result = "geomtery";
		break;
	case FRAGMENT_SHADER:
		result = "fragment";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return result;
}

/** Get glsl name of specified type
 *
 * @param type      Basic type
 * @param n_columns Number of columns
 * @param n_rows    Number of rows
 *
 * @return Name of glsl type
 **/
const glw::GLchar* Utils::getTypeName(TYPES type, glw::GLuint n_columns, glw::GLuint n_rows)
{
	static const GLchar* float_lut[4][4] = {
		{ "float", "vec2", "vec3", "vec4" },
		{ 0, "mat2", "mat2x3", "mat2x4" },
		{ 0, "mat3x2", "mat3", "mat3x4" },
		{ 0, "mat4x2", "mat4x3", "mat4" },
	};

	static const GLchar* double_lut[4][4] = {
		{ "double", "dvec2", "dvec3", "dvec4" },
		{ 0, "dmat2", "dmat2x3", "dmat2x4" },
		{ 0, "dmat3x2", "dmat3", "dmat3x4" },
		{ 0, "dmat4x2", "dmat4x3", "dmat4" },
	};

	static const GLchar* int_lut[4] = { "int", "ivec2", "ivec3", "ivec4" };

	static const GLchar* uint_lut[4] = { "uint", "uvec2", "uvec3", "uvec4" };

	const GLchar* result = 0;

	if ((1 > n_columns) || (1 > n_rows) || (4 < n_columns) || (4 < n_rows))
	{
		return 0;
	}

	switch (type)
	{
	case FLOAT:
		result = float_lut[n_columns - 1][n_rows - 1];
		break;
	case DOUBLE:
		result = double_lut[n_columns - 1][n_rows - 1];
		break;
	case INT:
		result = int_lut[n_rows - 1];
		break;
	case UINT:
		result = uint_lut[n_rows - 1];
		break;
	default:
		TCU_FAIL("Invliad enum");
	}

	return result;
}

/** Get proper glUniformNdv routine for vectors with specified number of rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
Utils::uniformNdv Utils::getUniformNdv(const glw::Functions& gl, glw::GLuint n_rows)
{
	uniformNdv result = 0;

	switch (n_rows)
	{
	case 1:
		result = gl.uniform1dv;
		break;
	case 2:
		result = gl.uniform2dv;
		break;
	case 3:
		result = gl.uniform3dv;
		break;
	case 4:
		result = gl.uniform4dv;
		break;
	default:
		TCU_FAIL("Invalid number of rows");
	}

	return result;
}

/** Get proper glUniformNfv routine for vectors with specified number of rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
Utils::uniformNfv Utils::getUniformNfv(const glw::Functions& gl, glw::GLuint n_rows)
{
	uniformNfv result = 0;

	switch (n_rows)
	{
	case 1:
		result = gl.uniform1fv;
		break;
	case 2:
		result = gl.uniform2fv;
		break;
	case 3:
		result = gl.uniform3fv;
		break;
	case 4:
		result = gl.uniform4fv;
		break;
	default:
		TCU_FAIL("Invalid number of rows");
	}

	return result;
}

/** Get proper glUniformNiv routine for vectors with specified number of rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
Utils::uniformNiv Utils::getUniformNiv(const glw::Functions& gl, glw::GLuint n_rows)
{
	uniformNiv result = 0;

	switch (n_rows)
	{
	case 1:
		result = gl.uniform1iv;
		break;
	case 2:
		result = gl.uniform2iv;
		break;
	case 3:
		result = gl.uniform3iv;
		break;
	case 4:
		result = gl.uniform4iv;
		break;
	default:
		TCU_FAIL("Invalid number of rows");
	}

	return result;
}

/** Get proper glUniformNuiv routine for vectors with specified number of rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
Utils::uniformNuiv Utils::getUniformNuiv(const glw::Functions& gl, glw::GLuint n_rows)
{
	uniformNuiv result = 0;

	switch (n_rows)
	{
	case 1:
		result = gl.uniform1uiv;
		break;
	case 2:
		result = gl.uniform2uiv;
		break;
	case 3:
		result = gl.uniform3uiv;
		break;
	case 4:
		result = gl.uniform4uiv;
		break;
	default:
		TCU_FAIL("Invalid number of rows");
	}

	return result;
}

/** Get proper glUniformMatrixNdv routine for matrix with specified number of columns and rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
Utils::uniformMatrixNdv Utils::getUniformMatrixNdv(const glw::Functions& gl, glw::GLuint n_columns, glw::GLuint n_rows)
{
	uniformMatrixNdv result = 0;

	switch (n_columns)
	{
	case 2:
		switch (n_rows)
		{
		case 2:
			result = gl.uniformMatrix2dv;
			break;
		case 3:
			result = gl.uniformMatrix2x3dv;
			break;
		case 4:
			result = gl.uniformMatrix2x4dv;
			break;
		default:
			TCU_FAIL("Invalid number of rows");
		}
		break;
	case 3:
		switch (n_rows)
		{
		case 2:
			result = gl.uniformMatrix3x2dv;
			break;
		case 3:
			result = gl.uniformMatrix3dv;
			break;
		case 4:
			result = gl.uniformMatrix3x4dv;
			break;
		default:
			TCU_FAIL("Invalid number of rows");
		}
		break;
	case 4:
		switch (n_rows)
		{
		case 2:
			result = gl.uniformMatrix4x2dv;
			break;
		case 3:
			result = gl.uniformMatrix4x3dv;
			break;
		case 4:
			result = gl.uniformMatrix4dv;
			break;
		default:
			TCU_FAIL("Invalid number of rows");
		}
		break;
	default:
		TCU_FAIL("Invalid number of columns");
	}

	return result;
}

/** Get proper glUniformMatrixNfv routine for vectors with specified number of columns and rows
 *
 * @param gl     GL functions
 * @param n_rows Number of rows
 *
 * @return Function address
 **/
Utils::uniformMatrixNfv Utils::getUniformMatrixNfv(const glw::Functions& gl, glw::GLuint n_columns, glw::GLuint n_rows)
{
	uniformMatrixNfv result = 0;

	switch (n_columns)
	{
	case 2:
		switch (n_rows)
		{
		case 2:
			result = gl.uniformMatrix2fv;
			break;
		case 3:
			result = gl.uniformMatrix2x3fv;
			break;
		case 4:
			result = gl.uniformMatrix2x4fv;
			break;
		default:
			TCU_FAIL("Invalid number of rows");
		}
		break;
	case 3:
		switch (n_rows)
		{
		case 2:
			result = gl.uniformMatrix3x2fv;
			break;
		case 3:
			result = gl.uniformMatrix3fv;
			break;
		case 4:
			result = gl.uniformMatrix3x4fv;
			break;
		default:
			TCU_FAIL("Invalid number of rows");
		}
		break;
	case 4:
		switch (n_rows)
		{
		case 2:
			result = gl.uniformMatrix4x2fv;
			break;
		case 3:
			result = gl.uniformMatrix4x3fv;
			break;
		case 4:
			result = gl.uniformMatrix4fv;
			break;
		default:
			TCU_FAIL("Invalid number of rows");
		}
		break;
	default:
		TCU_FAIL("Invalid number of columns");
	}

	return result;
}

/** Prepare definition of input or output block's variable
 *
 * @param qualifiers    Set of qualifiers
 * @param type_name     Name of type
 * @param variable_name Meaningful part of variable name, eg. tex_coord
 *
 * @return Definition of variable
 **/
std::string Utils::getBlockVariableDefinition(const qualifierSet& qualifiers, const glw::GLchar* type_name,
											  const glw::GLchar* variable_name)
{
	/* Templates */
	static const GLchar* def_template = "QUALIFIER_LISTTYPE VARIABLE_NAME";

	/* Tokens */
	static const GLchar* token_type			 = "TYPE";
	static const GLchar* token_variable_name = "VARIABLE_NAME";
	static const GLchar* token_qual_list	 = "QUALIFIER_LIST";

	/* Variables */
	std::string variable_definition = def_template;
	size_t		position			= 0;

	/* Get qualifiers list */
	const std::string& list = getQualifiersListString(qualifiers);

	/* Replace tokens */
	Utils::replaceToken(token_qual_list, position, list.c_str(), variable_definition);
	Utils::replaceToken(token_type, position, type_name, variable_definition);
	Utils::replaceToken(token_variable_name, position, variable_name, variable_definition);

	/* Done */
	return variable_definition;
}

/** Prepare reference to input or output variable
 *
 * @param flavour       "Flavour" of variable
 * @param variable_name Meaningful part of variable name, eg. tex_coord
 * @param block_name    Name of block
 *
 * @return Reference to variable
 **/
std::string Utils::getBlockVariableReference(VARIABLE_FLAVOUR flavour, const glw::GLchar* variable_name,
											 const glw::GLchar* block_name)
{
	/* Templates */
	static const GLchar* ref_template		= "BLOCK_NAME.VARIABLE_NAME";
	static const GLchar* array_ref_template = "BLOCK_NAME[0].VARIABLE_NAME";
	static const GLchar* tcs_ref_template   = "BLOCK_NAME[gl_InvocationID].VARIABLE_NAME";

	/* Token */
	static const GLchar* token_block_name	= "BLOCK_NAME";
	static const GLchar* token_variable_name = "VARIABLE_NAME";

	/* Variables */
	std::string variable_definition;
	size_t		position = 0;

	/* Select variable reference template */
	switch (flavour)
	{
	case BASIC:
		variable_definition = ref_template;
		break;
	case ARRAY:
		variable_definition = array_ref_template;
		break;
	case INDEXED_BY_INVOCATION_ID:
		variable_definition = tcs_ref_template;
		break;
	default:
		variable_definition = ref_template;
		break;
	}

	/* Replace tokens */
	replaceAllTokens(token_block_name, block_name, variable_definition);
	replaceToken(token_variable_name, position, variable_name, variable_definition);

	/* Done */
	return variable_definition;
}

/** Prepare definition of input or output variable
 *
 * @param flavour       "Flavour" of variable
 * @param qualifiers    Set of qualifiers
 * @param type_name     Name of type
 * @param variable_name Meaningful part of variable name, eg. tex_coord
 *
 * @return Definition of variable
 **/
std::string Utils::getVariableDefinition(VARIABLE_FLAVOUR flavour, const qualifierSet& qualifiers,
										 const glw::GLchar* type_name, const glw::GLchar* variable_name)
{
	/* Templates */
	static const GLchar* def_template		= "QUALIFIER_LISTTYPE VARIABLE_NAME";
	static const GLchar* def_array_template = "QUALIFIER_LISTTYPE VARIABLE_NAME[]";

	/* Tokens */
	static const GLchar* token_type			 = "TYPE";
	static const GLchar* token_variable_name = "VARIABLE_NAME";
	static const GLchar* token_qual_list	 = "QUALIFIER_LIST";

	/* Variables */
	std::string variable_definition;
	size_t		position = 0;

	/* Select variable definition template */
	switch (flavour)
	{
	case BASIC:
		variable_definition = def_template;
		break;
	case ARRAY:
	case INDEXED_BY_INVOCATION_ID:
		variable_definition = def_array_template;
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}

	/* Get qualifiers list */
	const std::string& list = getQualifiersListString(qualifiers);

	/* Replace tokens */
	replaceToken(token_qual_list, position, list.c_str(), variable_definition);
	replaceToken(token_type, position, type_name, variable_definition);
	replaceToken(token_variable_name, position, variable_name, variable_definition);

	/* Done */
	return variable_definition;
}

/** Get "flavour" of variable
 *
 * @param stage      Shader stage
 * @param storage    Storage of variable
 * @param qualifiers Set of qualifiers for variable
 *
 * @return "Flavour" of variable
 **/
Utils::VARIABLE_FLAVOUR Utils::getVariableFlavour(SHADER_STAGES stage, VARIABLE_STORAGE storage,
												  const qualifierSet& qualifiers)
{
	VARIABLE_FLAVOUR result;

	if (UNIFORM == storage)
	{
		result = BASIC;
	}
	else
	{
		switch (stage)
		{
		case Utils::GEOMETRY_SHADER:
			if (Utils::INPUT == storage)
			{
				result = ARRAY;
			}
			else /* OUTPUT */
			{
				result = BASIC;
			}
			break;
		case Utils::TESS_EVAL_SHADER:
			if ((false == Utils::doesContainQualifier(Utils::QUAL_PATCH, qualifiers)) && (Utils::INPUT == storage))
			{
				result = ARRAY;
			}
			else /* OUTPUT */
			{
				result = BASIC;
			}
			break;
		case Utils::TESS_CTRL_SHADER:
			if ((true == Utils::doesContainQualifier(Utils::QUAL_PATCH, qualifiers)) && (Utils::OUTPUT == storage))
			{
				result = BASIC;
			}
			else
			{
				result = INDEXED_BY_INVOCATION_ID;
			}
			break;
		case Utils::VERTEX_SHADER:
		case Utils::FRAGMENT_SHADER:
			result = BASIC;
			break;
		default:
			TCU_FAIL("Invliad enum");
			break;
		}
	}

	return result;
}

/** Prepare name of input or output variable
 *
 * @param stage         Shader stage
 * @param storage       Storage of variable
 * @param variable_name Meaningful part of variable name, eg. tex_coord
 *
 * @return Name of variable
 **/
std::string Utils::getVariableName(SHADER_STAGES stage, VARIABLE_STORAGE storage, const glw::GLchar* variable_name)
{
	/* Variable name template */
	static const GLchar* variable_name_template = "PRECEEDING_PREFIX_VARIABLE_NAME";

	/* Tokens */
	static const GLchar* token_preceeding	= "PRECEEDING";
	static const GLchar* token_prefix		 = "PREFIX";
	static const GLchar* token_variable_name = "VARIABLE_NAME";

	static const GLchar* prefixes[Utils::STORAGE_MAX][Utils::SHADER_STAGES_MAX][2] = {
		/* COMPUTE,					VERTEX,				TCS,				TES,				GEOMETRY,			FRAGMENT					*/
		{ { "", "" },
		  { "in", "vs" },
		  { "vs", "tcs" },
		  { "tcs", "tes" },
		  { "tes", "gs" },
		  { "gs", "fs" } }, /* INPUT	*/
		{ { "", "" },
		  { "vs", "tcs" },
		  { "tcs", "tes" },
		  { "tes", "gs" },
		  { "gs", "fs" },
		  { "fs", "out" } }, /* OUTPUT	*/
		{ { "uni", "comp" },
		  { "uni", "vs" },
		  { "uni", "tcs" },
		  { "uni", "tes" },
		  { "uni", "gs" },
		  { "uni", "fs" } } /* UNIFORM	*/
	};

	/* Variables */
	const GLchar* preceeding = prefixes[storage][stage][0];
	const GLchar* prefix	 = prefixes[storage][stage][1];
	std::string   name		 = variable_name_template;
	size_t		  position   = 0;

	/* Replace tokens */
	Utils::replaceToken(token_preceeding, position, preceeding, name);
	Utils::replaceToken(token_prefix, position, prefix, name);
	Utils::replaceToken(token_variable_name, position, variable_name, name);

	/* Done */
	return name;
}

/** Prepare reference to input or output variable
 *
 * @param flavour       "Flavour" of variable
 * @param variable_name Meaningful part of variable name, eg. tex_coord
 *
 * @return Reference to variable
 **/
std::string Utils::getVariableReference(VARIABLE_FLAVOUR flavour, const glw::GLchar* variable_name)
{
	/* Templates */
	static const GLchar* ref_template		= "VARIABLE_NAME";
	static const GLchar* array_ref_template = "VARIABLE_NAME[0]";
	static const GLchar* tcs_ref_template   = "VARIABLE_NAME[gl_InvocationID]";

	/* Token */
	static const GLchar* token_variable_name = "VARIABLE_NAME";

	/* Variables */
	std::string variable_definition;
	size_t		position = 0;

	/* Select variable reference template */
	switch (flavour)
	{
	case BASIC:
		variable_definition = ref_template;
		break;
	case ARRAY:
		variable_definition = array_ref_template;
		break;
	case INDEXED_BY_INVOCATION_ID:
		variable_definition = tcs_ref_template;
		break;
	default:
		variable_definition = ref_template;
		break;
	}

	/* Replace token */
	Utils::replaceToken(token_variable_name, position, variable_name, variable_definition);

	/* Done */
	return variable_definition;
}

/** Prepare definition and reference string for block varaible
 *
 * @param in_stage         Shader stage
 * @param in_storage       Storage of variable
 * @param in_qualifiers    Set of qualifiers
 * @param in_type_name     Type name
 * @param in_variable_name Meaningful part of variable name, like "color"
 * @param in_block_name    Name of block, like "input"
 * @param out_definition   Definition string
 * @param out_reference    Reference string
 **/
void Utils::prepareBlockVariableStrings(Utils::SHADER_STAGES in_stage, Utils::VARIABLE_STORAGE in_storage,
										const Utils::qualifierSet& in_qualifiers, const glw::GLchar* in_type_name,
										const glw::GLchar* in_variable_name, const glw::GLchar* in_block_name,
										std::string& out_definition, std::string& out_reference)
{
	VARIABLE_FLAVOUR	flavour	= getVariableFlavour(in_stage, in_storage, in_qualifiers);
	const qualifierSet& qualifiers = prepareQualifiersSet(in_qualifiers, in_stage, in_storage);
	const std::string&  name	   = getVariableName(in_stage, in_storage, in_variable_name);

	out_definition = getBlockVariableDefinition(qualifiers, in_type_name, name.c_str());
	out_reference  = getBlockVariableReference(flavour, name.c_str(), in_block_name);
}

/** Prepare definition and reference string for block varaible
 *
 * @param in_stage         Shader stage
 * @param in_storage       Storage of variable
 * @param in_qualifiers    Set of qualifiers
 * @param in_type_name     Type name
 * @param in_variable_name Meaningful part of variable name, like "color"
 * @param out_definition   Definition string
 * @param out_reference    Reference string
 **/
void Utils::prepareVariableStrings(Utils::SHADER_STAGES in_stage, Utils::VARIABLE_STORAGE in_storage,
								   const Utils::qualifierSet& in_qualifiers, const glw::GLchar* in_type_name,
								   const glw::GLchar* in_variable_name, std::string& out_definition,
								   std::string& out_reference)
{
	VARIABLE_FLAVOUR	flavour	= getVariableFlavour(in_stage, in_storage, in_qualifiers);
	const qualifierSet& qualifiers = prepareQualifiersSet(in_qualifiers, in_stage, in_storage);
	const std::string&  name	   = getVariableName(in_stage, in_storage, in_variable_name);

	out_definition = getVariableDefinition(flavour, qualifiers, in_type_name, name.c_str());
	out_reference  = getVariableReference(flavour, name.c_str());
}

/** Returns string with UTF8 character for current test case
 *
 * @return String with UTF8 character
 **/
const GLchar* Utils::getUtf8Character(Utils::UTF8_CHARACTERS character)
{
	static const unsigned char two_bytes[]		 = { 0xd7, 0x84, 0x00 };
	static const unsigned char three_bytes[]	 = { 0xe3, 0x82, 0x81, 0x00 };
	static const unsigned char four_bytes[]		 = { 0xf0, 0x93, 0x83, 0x93, 0x00 };
	static const unsigned char five_bytes[]		 = { 0xfa, 0x82, 0x82, 0x82, 0x82, 0x00 };
	static const unsigned char six_bytes[]		 = { 0xfd, 0x82, 0x82, 0x82, 0x82, 0x82, 0x00 };
	static const unsigned char redundant_bytes[] = { 0xf2, 0x80, 0x80, 0x5e, 0x00 };

	const GLchar* result = 0;

	switch (character)
	{
	case TWO_BYTES:
		result = (const GLchar*)two_bytes;
		break;
	case THREE_BYTES:
		result = (const GLchar*)three_bytes;
		break;
	case FOUR_BYTES:
		result = (const GLchar*)four_bytes;
		break;
	case FIVE_BYTES:
		result = (const GLchar*)five_bytes;
		break;
	case SIX_BYTES:
		result = (const GLchar*)six_bytes;
		break;
	case REDUNDANT_ASCII:
		result = (const GLchar*)redundant_bytes;
		break;
	case EMPTY:
		result = "";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return result;
}
/** Check if extension is supported
 *
 * @param context        Test context
 * @param extension_name Name of extension
 *
 * @return true if extension is supported, false otherwise
 **/
bool Utils::isExtensionSupported(deqp::Context& context, const GLchar* extension_name)
{
	const std::vector<std::string>& extensions = context.getContextInfo().getExtensions();

	if (std::find(extensions.begin(), extensions.end(), extension_name) == extensions.end())
	{
		return false;
	}

	return true;
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

/** Replace all occurances of <token> with <text> in <string>
 *
 * @param token           Token string
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void Utils::replaceAllTokens(const glw::GLchar* token, const glw::GLchar* text, std::string& string)
{
	const size_t text_length  = strlen(text);
	const size_t token_length = strlen(token);

	size_t search_position = 0;

	while (1)
	{
		const size_t token_position = string.find(token, search_position);

		if (std::string::npos == token_position)
		{
			break;
		}

		search_position = token_position + text_length;

		string.replace(token_position, token_length, text, text_length);
	}
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
TestBase::TestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description)
	: TestCase(context, test_name, test_description)
	, m_is_compute_shader_supported(false)
	, m_is_explicit_uniform_location(false)
	, m_is_shader_language_420pack(false)
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult TestBase::iterate()
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check extension support and version */
	m_is_explicit_uniform_location = Utils::isExtensionSupported(m_context, "GL_ARB_explicit_uniform_location");
	m_is_shader_language_420pack   = Utils::isExtensionSupported(m_context, "GL_ARB_shading_language_420pack");
	m_is_compute_shader_supported  = Utils::isGLVersionAtLeast(gl, 4, 3);

	/* Execute test */
	bool test_result = test();

	/* Set result */
	if (true == test_result)
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

/** Basic implementation of getShaderSourceConfig method.
 *
 * @param out_n_parts     Number of source parts used by this test case
 * @param out_use_lengths If source lengths shall be provided to compiler
 **/
void TestBase::getShaderSourceConfig(glw::GLuint& out_n_parts, bool& out_use_lengths)
{
	out_n_parts		= 1;
	out_use_lengths = false;
}

/** Basic implementation of prepareNextTestCase method.
 *
 * @param test_case_index Index of test case
 *
 * @return true if index is -1 or 0, false otherwise
 **/
bool TestBase::prepareNextTestCase(GLuint test_case_index)
{
	if (((GLuint)-1 == test_case_index) || (0 == test_case_index))
	{
		return true;
	}
	else
	{
		return false;
	}
}

/** Basic implementation of prepareUniforms method
 *
 * @param ignored
 **/
void TestBase::prepareUniforms(Utils::program& /* program */)
{
	/* Nothing to be done */
}

/** Basic implementation of testInit method
 *
 * @return true if test can be executed, false otherwise
 **/
bool TestBase::testInit()
{
	return true;
}

/** Get layout specific for given stage
 *
 * @param stage Shader stage
 *
 * @return Stage specific part
 **/
const GLchar* TestBase::getStageSpecificLayout(Utils::SHADER_STAGES stage) const
{
	static const GLchar* stage_layout_geometry = "layout(points)                           in;\n"
												 "layout(triangle_strip, max_vertices = 4) out;\n";
	static const GLchar* stage_layout_tess_ctrl = "layout(vertices = 1)                     out;\n";
	static const GLchar* stage_layout_tess_eval = "layout(isolines, point_mode)             in;\n";

	const GLchar* result = "";

	switch (stage)
	{
	case Utils::GEOMETRY_SHADER:
		result = stage_layout_geometry;
		break;
	case Utils::TESS_CTRL_SHADER:
		result = stage_layout_tess_ctrl;
		break;
	case Utils::TESS_EVAL_SHADER:
		result = stage_layout_tess_eval;
		break;
	case Utils::VERTEX_SHADER:
	case Utils::FRAGMENT_SHADER:
	default:
		break;
	}

	return result;
}

/** Get "version" string
 *
 * @param stage           Shader stage, compute shader will use 430
 * @param use_version_400 Select if 400 or 420 should be used
 *
 * @return Version string
 **/
const GLchar* TestBase::getVersionString(Utils::SHADER_STAGES stage, bool use_version_400) const
{
	static const GLchar* version_400 = "#version 400\n"
									   "#extension GL_ARB_shading_language_420pack : require\n"
									   "#extension GL_ARB_separate_shader_objects : enable";
	static const GLchar* version_420 = "#version 420";
	static const GLchar* version_430 = "#version 430";

	const GLchar* result = "";

	if (Utils::COMPUTE_SHADER == stage)
	{
		result = version_430;
	}
	else if (true == use_version_400)
	{
		result = version_400;
	}
	else
	{
		result = version_420;
	}

	return result;
}

/** Initialize shaderSource instance, reserve storage and prepare shader source
 *
 * @param in_stage           Shader stage
 * @param in_use_version_400 If version 400 or 420 should be used
 * @param out_source         Shader source instance
 **/
void TestBase::initShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400, Utils::shaderSource& out_source)
{
	/* Shader source configuration */
	glw::GLuint n_parts		= 0;
	bool		use_lengths = false;

	getShaderSourceConfig(n_parts, use_lengths);

	out_source.m_parts.resize(n_parts);
	out_source.m_use_lengths = use_lengths;

	/* Request child class to prepare shader sources */
	prepareShaderSource(in_stage, in_use_version_400, out_source);

	/* Prepare source lengths */
	if (true == use_lengths)
	{
		for (GLuint i = 0; i < n_parts; ++i)
		{
			out_source.m_parts[i].m_length = static_cast<glw::GLint>(out_source.m_parts[i].m_code.length());

			out_source.m_parts[i].m_code.append("This should be ignored by compiler, as source length is provided");
		}
	}
	else
	{
		for (GLuint i = 0; i < n_parts; ++i)
		{
			out_source.m_parts[i].m_length = 0;
		}
	}
}

/** Execute test
 *
 * @return true if test pass, false otherwise
 **/
bool TestBase::test()
{
	bool   result		   = true;
	GLuint test_case_index = 0;

	/* Prepare test cases */
	testInit();

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Tesselation patch set up */
	gl.patchParameteri(GL_PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

	while (true == prepareNextTestCase(test_case_index))
	{
		bool case_result = true;

		/* Execute drawing case */
		if (false == testDrawArray(false))
		{
			case_result = false;
		}

		if (true == m_is_shader_language_420pack)
		{
			if (false == testDrawArray(true))
			{
				case_result = false;
			}
		}

		/* Execute compute shader case */
		if (true == m_is_compute_shader_supported)
		{
			if (false == testCompute())
			{
				case_result = false;
			}
		}

		/* Log failure */
		if (false == case_result)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Test case failed."
												<< tcu::TestLog::EndMessage;

			result = false;
		}

		/* Go to next test case */
		test_case_index += 1;
	}

	/* Done */
	return result;
}

int TestBase::maxImageUniforms(Utils::SHADER_STAGES stage) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	GLint max_image_uniforms;

	switch (stage)
	{
	case Utils::COMPUTE_SHADER:
		gl.getIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &max_image_uniforms);
		break;
	case Utils::FRAGMENT_SHADER:
		gl.getIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &max_image_uniforms);
		break;
	case Utils::GEOMETRY_SHADER:
		gl.getIntegerv(GL_MAX_GEOMETRY_IMAGE_UNIFORMS, &max_image_uniforms);
		break;
	case Utils::TESS_CTRL_SHADER:
		gl.getIntegerv(GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS, &max_image_uniforms);
		break;
	case Utils::TESS_EVAL_SHADER:
		gl.getIntegerv(GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, &max_image_uniforms);
		break;
	case Utils::VERTEX_SHADER:
		gl.getIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &max_image_uniforms);
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}
	return max_image_uniforms;
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Name of test
 * @param test_description Description of test
 **/
APITestBase::APITestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description)
	: TestBase(context, test_name, test_description)
{
	/* Nothing to be done here */
}

/** Execute test with compute shader
 *
 * @return true if test pass, false otherwise
 **/
bool APITestBase::testCompute()
{
	/* GL objects */
	Utils::program program(m_context);

	/* Shaders */
	Utils::shaderSource compute_shader;
	initShaderSource(Utils::COMPUTE_SHADER, false, compute_shader);

	/* Check if test support compute shaders */
	if (true == compute_shader.m_parts[0].m_code.empty())
	{
		return true;
	}

	/* Build program */
	try
	{
		program.build(compute_shader, 0 /* fragment shader */, 0 /* geometry shader */,
					  0 /* tesselation control shader */, 0 /* tesselation evaluation shader */, 0 /* vertex shader */,
					  0 /* varying names */, 0 /* n varying names */, false);
	}
	catch (Utils::shaderCompilationException& exc)
	{
		/* Something wrong with compilation, test case failed */
		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Shader compilation failed. Error message: " << exc.m_error_message;

		Utils::program::printShaderSource(exc.m_shader_source, message);

		message << tcu::TestLog::EndMessage;

		return false;
	}
	catch (Utils::programLinkageException& exc)
	{
		/* Something wrong with linking, test case failed */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Program linking failed. Error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		return false;
	}

	/* Set current program */
	program.use();

	/* Return result of verification */
	return checkResults(program);
}

/** Execute test with VS, TCS, TES, GS and FS
 *
 * @param use_version_400 Select if 400 or 420 should be used
 *
 * @return true if test pass, false otherwise
 **/
bool APITestBase::testDrawArray(bool use_version_400)
{
	/* GL objects */
	Utils::program program(m_context);

	/* Shaders */
	Utils::shaderSource fragment_data;
	Utils::shaderSource geometry_data;
	Utils::shaderSource tess_ctrl_data;
	Utils::shaderSource tess_eval_data;
	Utils::shaderSource vertex_data;

	initShaderSource(Utils::FRAGMENT_SHADER, use_version_400, fragment_data);
	initShaderSource(Utils::GEOMETRY_SHADER, use_version_400, geometry_data);
	initShaderSource(Utils::TESS_CTRL_SHADER, use_version_400, tess_ctrl_data);
	initShaderSource(Utils::TESS_EVAL_SHADER, use_version_400, tess_eval_data);
	initShaderSource(Utils::VERTEX_SHADER, use_version_400, vertex_data);

	/* Build program */
	try
	{
		program.build(0 /* compute shader */, fragment_data, geometry_data, tess_ctrl_data, tess_eval_data, vertex_data,
					  0 /* varying names */, 0 /* n varying names */, false);
	}
	catch (Utils::shaderCompilationException& exc)
	{
		/* Something wrong with compilation, test case failed */
		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Shader compilation failed. Error message: " << exc.m_error_message;

		Utils::program::printShaderSource(exc.m_shader_source, message);

		message << tcu::TestLog::EndMessage;

		return false;
	}
	catch (Utils::programLinkageException& exc)
	{
		/* Something wrong with linking, test case failed */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Program linking failed. Error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		return false;
	}

	/* Set current program */
	program.use();

	/* Return result of verification */
	return checkResults(program);
}

/* Constants used by GLSLTestBase */
const glw::GLenum GLSLTestBase::m_color_texture_internal_format = GL_RGBA8;
const glw::GLenum GLSLTestBase::m_color_texture_format			= GL_RGBA;
const glw::GLenum GLSLTestBase::m_color_texture_type			= GL_UNSIGNED_BYTE;
const glw::GLuint GLSLTestBase::m_color_texture_width			= 16;
const glw::GLuint GLSLTestBase::m_color_texture_height			= 16;

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
GLSLTestBase::GLSLTestBase(deqp::Context& context, const glw::GLchar* test_name, const glw::GLchar* test_description)
	: TestBase(context, test_name, test_description)
{
	/* Nothing to be done here */
}

/** Basic implementation of prepareSourceTexture method.
 *
 * @param ignored Texture instance
 *
 * @return 0
 **/
const GLchar* GLSLTestBase::prepareSourceTexture(Utils::texture&)
{
	return 0;
}

/** Basic implementation of prepareVertexBuffer method.
 *
 * @param ignored Program instance
 * @param ignored Buffer instance
 * @param vao     VertexArray instance
 *
 * @return 0
 **/
void GLSLTestBase::prepareVertexBuffer(const Utils::program&, Utils::buffer&, Utils::vertexArray& vao)
{
	vao.generate();
	vao.bind();
}

/** Basic implementation of verifyAdditionalResults
 *
 * @return true
 **/
bool GLSLTestBase::verifyAdditionalResults() const
{
	return true;
}

/** Basic implementation of releaseResource method
 *
 * @param ignored
 **/
void GLSLTestBase::releaseResource()
{
	/* Nothing to be done */
}

/** Bind texture to first image unit and set image uniform to that unit
 *
 * @param program      Program object
 * @param texture      Texture object
 * @param uniform_name Name of image uniform
 **/
void GLSLTestBase::bindTextureToimage(Utils::program& program, Utils::texture& texture,
									  const glw::GLchar* uniform_name) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindImageTexture(0 /* unit */, texture.m_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */, GL_WRITE_ONLY,
						GL_RGBA8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

	GLint location = program.getUniformLocation(uniform_name);
	gl.uniform1i(location, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
}

/** Bind texture to first texture unit and set sampler uniform to that unit
 *
 * @param program      Program object
 * @param texture      Texture object
 * @param uniform_name Name of sampler uniform
 **/
void GLSLTestBase::bindTextureToSampler(Utils::program& program, Utils::texture& texture,
										const glw::GLchar* uniform_name) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

	texture.bind();

	GLint location = program.getUniformLocation(uniform_name);
	gl.uniform1i(location, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
}

/** Check contents of texture. It is expected that it will be filled with green color
 *
 * @param color_texture Texture that will be verified
 *
 * @return true if texture is all green, false otherwise
 **/
bool GLSLTestBase::checkResults(Utils::texture& color_texture) const
{
	static const GLuint		 green_color	   = 0xff00ff00;
	const GLuint			 texture_data_size = m_color_texture_width * m_color_texture_height;
	std::vector<glw::GLuint> texture_data;

	texture_data.resize(texture_data_size);

	color_texture.get(m_color_texture_format, m_color_texture_type, &texture_data[0]);

	for (GLuint i = 0; i < texture_data_size; ++i)
	{
		if (green_color != texture_data[i])
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid texel: " << std::setbase(16)
												<< std::setfill('0') << std::setw(8) << texture_data[i]
												<< " at index: " << i << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return verifyAdditionalResults();
}

/** Prepare framebuffer with texture used as attachment
 *
 * @param framebuffer   Framebuffer
 * @param color_texture Textue used as color attachment 0
 **/
void GLSLTestBase::prepareFramebuffer(Utils::framebuffer& framebuffer, Utils::texture& color_texture) const
{
	framebuffer.generate();

	color_texture.create(m_color_texture_width, m_color_texture_height, m_color_texture_internal_format);

	framebuffer.attachTexture(GL_COLOR_ATTACHMENT0, color_texture.m_id, m_color_texture_width, m_color_texture_height);

	framebuffer.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	framebuffer.clear(GL_COLOR_BUFFER_BIT);
}

/** Prepare texture and bind it to image uniform
 *
 * @param framebuffer   Framebuffer
 * @param color_texture Textue used as color attachment 0
 **/
void GLSLTestBase::prepareImage(Utils::program& program, Utils::texture& color_texture) const
{
	color_texture.create(m_color_texture_width, m_color_texture_height, m_color_texture_internal_format);

	bindTextureToimage(program, color_texture, "uni_image");
}

/** Execute test with compute shader
 *
 * @return true if test pass, false otherwise
 **/
bool GLSLTestBase::testCompute()
{
	/* Test Result */
	bool result = true;

	/* GL objects */
	Utils::texture color_tex(m_context);
	Utils::program program(m_context);
	Utils::texture source_tex(m_context);

	/* Shaders */
	Utils::shaderSource compute_shader;
	initShaderSource(Utils::COMPUTE_SHADER, false, compute_shader);

	/* Check if test support compute shaders */
	if (true == compute_shader.m_parts[0].m_code.empty())
	{
		return true;
	}

	/* Build program */
	try
	{
		program.build(compute_shader, 0 /* fragment shader */, 0 /* geometry shader */,
					  0 /* tesselation control shader */, 0 /* tesselation evaluation shader */, 0 /* vertex shader */,
					  0 /* varying names */, 0 /* n varying names */, false);
	}
	catch (Utils::shaderCompilationException& exc)
	{
		/* Something wrong with compilation, test case failed */
		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Shader compilation failed. Error message: " << exc.m_error_message;

		Utils::program::printShaderSource(exc.m_shader_source, message);

		message << tcu::TestLog::EndMessage;

		return false;
	}
	catch (Utils::programLinkageException& exc)
	{
		/* Something wrong with linking, test case failed */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Program linking failed. Error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		return false;
	}

/* Log shaders, for debugging */
#if IS_DEBUG
	{
		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		Utils::program::printShaderSource(compute_shader, message);

		message << tcu::TestLog::EndMessage;
	}
#endif /* IS_DEBUG */

	/* Set current program */
	program.use();

	/* Prepare image unit */
	prepareImage(program, color_tex);

	/* Test specific preparation of source texture */
	const GLchar* sampler_name = prepareSourceTexture(source_tex);
	if (0 != sampler_name)
	{
		bindTextureToSampler(program, source_tex, sampler_name);
	}

	/* Set up uniforms */
	prepareUniforms(program);

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Draw */
	gl.dispatchCompute(m_color_texture_width, m_color_texture_height, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

	/* Return result of verification */
	result = checkResults(color_tex);

	/* Release extra resource for the test */
	releaseResource();

	return result;
}

/** Execute test with draw array operation
 *
 * @param use_version_400 Select if 400 or 420 should be used
 *
 * @return true if test pass, false otherwise
 **/
bool GLSLTestBase::testDrawArray(bool use_version_400)
{
	/* Test Result */
	bool result = true;

	/* GL objects */
	Utils::texture	 color_tex(m_context);
	Utils::framebuffer framebuffer(m_context);
	Utils::program	 program(m_context);
	Utils::texture	 source_tex(m_context);
	Utils::vertexArray vao(m_context);
	Utils::buffer	  vertex_buffer(m_context);

	/* Shaders */
	Utils::shaderSource fragment_data;
	Utils::shaderSource geometry_data;
	Utils::shaderSource tess_ctrl_data;
	Utils::shaderSource tess_eval_data;
	Utils::shaderSource vertex_data;

	initShaderSource(Utils::FRAGMENT_SHADER, use_version_400, fragment_data);
	initShaderSource(Utils::GEOMETRY_SHADER, use_version_400, geometry_data);
	initShaderSource(Utils::TESS_CTRL_SHADER, use_version_400, tess_ctrl_data);
	initShaderSource(Utils::TESS_EVAL_SHADER, use_version_400, tess_eval_data);
	initShaderSource(Utils::VERTEX_SHADER, use_version_400, vertex_data);

	/* Build program */
	try
	{
		program.build(0 /* compute shader */, fragment_data, geometry_data, tess_ctrl_data, tess_eval_data, vertex_data,
					  0 /* varying names */, 0 /* n varying names */, false);
	}
	catch (Utils::shaderCompilationException& exc)
	{
		/* Something wrong with compilation, test case failed */
		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Shader compilation failed. Error message: " << exc.m_error_message;

		Utils::program::printShaderSource(exc.m_shader_source, message);

		message << tcu::TestLog::EndMessage;

		return false;
	}
	catch (Utils::programLinkageException& exc)
	{
		/* Something wrong with linking, test case failed */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Program linking failed. Error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		return false;
	}

/* Log shaders, for debugging */
#if IS_DEBUG
	{
		const Utils::shaderSource* data[] = { &vertex_data, &tess_ctrl_data, &tess_eval_data, &geometry_data,
											  &fragment_data };

		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		for (GLuint i = 0; i < 5; ++i)
		{
			Utils::program::printShaderSource(*data[i], message);
		}

		message << tcu::TestLog::EndMessage;
	}
#endif /* IS_DEBUG */

	/* Test specific preparation of vertex buffer and vao*/
	prepareVertexBuffer(program, vertex_buffer, vao);

	/* Set current program */
	program.use();

	/* Prepare framebuffer */
	prepareFramebuffer(framebuffer, color_tex);

	/* Test specific preparation of source texture */
	const GLchar* sampler_name = prepareSourceTexture(source_tex);
	if (0 != sampler_name)
	{
		bindTextureToSampler(program, source_tex, sampler_name);
	}

	/* Set up uniforms */
	prepareUniforms(program);

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Draw */
	gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	/* Return result of verification */
	result = checkResults(color_tex);

	/* Release extra resource for the test */
	releaseResource();

	return result;
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
NegativeTestBase::NegativeTestBase(deqp::Context& context, const glw::GLchar* test_name,
								   const glw::GLchar* test_description)
	: TestBase(context, test_name, test_description)
{
	/* Nothing to be done here */
}

/** Execute test with compute shader
 *
 * @return true if test pass, false otherwise
 **/
bool NegativeTestBase::testCompute()
{
	/* GL objects */
	Utils::program program(m_context);

	/* Shaders */
	Utils::shaderSource conmpute_data;
	initShaderSource(Utils::COMPUTE_SHADER, false, conmpute_data);

	/* Build program */
	try
	{
		program.build(conmpute_data /* compute shader */, 0 /* fragment shader */, 0 /* geometry shader */,
					  0 /* tesselation control shader */, 0 /* tesselation evaluation shader */, 0 /* vertex shader */,
					  0 /* varying names */, 0 /* n varying names */, false);
	}
	catch (Utils::shaderCompilationException& exc)
	{
		/* Compilation failed, as expected. Verify that reason of failure is as expected */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Shader compilation error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		return true;
	}
	catch (Utils::programLinkageException& exc)
	{
		/* Something wrong with linking, test case failed */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Program linking failed. Error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		return true;
	}

	/* Build process succeded */
	return false;
}

/** Execute test with draw array operation
 *
 * @param use_version_400 Select if 400 or 420 should be used
 *
 * @return true if test pass, false otherwise
 **/
bool NegativeTestBase::testDrawArray(bool use_version_400)
{
	/* GL objects */
	Utils::program program(m_context);

	/* Shaders */
	Utils::shaderSource fragment_data;
	Utils::shaderSource geometry_data;
	Utils::shaderSource tess_ctrl_data;
	Utils::shaderSource tess_eval_data;
	Utils::shaderSource vertex_data;

	initShaderSource(Utils::FRAGMENT_SHADER, use_version_400, fragment_data);
	initShaderSource(Utils::GEOMETRY_SHADER, use_version_400, geometry_data);
	initShaderSource(Utils::TESS_CTRL_SHADER, use_version_400, tess_ctrl_data);
	initShaderSource(Utils::TESS_EVAL_SHADER, use_version_400, tess_eval_data);
	initShaderSource(Utils::VERTEX_SHADER, use_version_400, vertex_data);

	/* Build program */
	try
	{
		program.build(0 /* compute shader */, fragment_data, geometry_data, tess_ctrl_data, tess_eval_data, vertex_data,
					  0 /* varying names */, 0 /* n varying names */, false);
	}
	catch (Utils::shaderCompilationException& exc)
	{
		/* Compilation failed, as expected. Verify that reason of failure is as expected */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Shader compilation error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		return true;
	}
	catch (Utils::programLinkageException& exc)
	{
		/* Something wrong with linking, test case failed */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Program linking failed. Error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		return true;
	}

	/* Build process succeded */
	return false;
}

/* Constants used by BindingImageTest */
const GLuint BindingImageTest::m_width		 = 16;
const GLuint BindingImageTest::m_green_color = 0xff00ff00;
const GLuint BindingImageTest::m_height		 = 16;
const GLuint BindingImageTest::m_depth		 = 6;

/** Constructor
 *
 * @param context Test context
 **/
BindingImageTest::BindingImageTest(deqp::Context& context, const glw::GLchar* test_name,
								   const glw::GLchar* test_description)
	: GLSLTestBase(context, test_name, test_description)
{
	/* Nothing to be done */
}

/** Prepare buffer, filled with given color
 *
 * @param buffer Buffer object
 * @param color  Color
 **/
void BindingImageTest::prepareBuffer(Utils::buffer& buffer, GLuint color)
{
	std::vector<GLuint> texture_data;
	texture_data.resize(m_width);

	buffer.generate(GL_TEXTURE_BUFFER);

	for (GLuint i = 0; i < texture_data.size(); ++i)
	{
		texture_data[i] = color;
	}

	buffer.update(m_width * sizeof(GLuint), &texture_data[0], GL_STATIC_DRAW);
}

/** Prepare texture of given type filled with given color and bind to specified image unit
 *
 * @param texture      Texture
 * @param buffer       Buffer
 * @param texture_type Type of texture
 * @param color        Color
 **/
void BindingImageTest::prepareTexture(Utils::texture& texture, const Utils::buffer& buffer,
									  Utils::TEXTURE_TYPES texture_type, GLuint color, GLuint unit)
{
	std::vector<GLuint> texture_data;
	texture_data.resize(m_width * m_height * m_depth);

	GLboolean is_layered = GL_FALSE;

	for (GLuint i = 0; i < texture_data.size(); ++i)
	{
		texture_data[i] = color;
	}

	if (Utils::TEX_BUFFER != texture_type)
	{
		texture.create(m_width, m_height, m_depth, GL_RGBA8, texture_type);

		texture.update(m_width, m_height, m_depth, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);
	}
	else
	{
		buffer.bind();

		texture.createBuffer(GL_RGBA8, buffer.m_id);
	}

	switch (texture_type)
	{
	case Utils::TEX_1D_ARRAY:
	case Utils::TEX_2D_ARRAY:
	case Utils::TEX_3D:
	case Utils::TEX_CUBE:
		is_layered = GL_TRUE;
		break;
	default:
		break;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindImageTexture(unit, texture.m_id, 0 /* level */, is_layered /* layered */, 0 /* layer */, GL_READ_WRITE,
						GL_RGBA8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");
}

/** Verifies that texel at offset 0 is green
 *
 * @param buffer Buffer object
 *
 * @return true if texel at offset 0 is green, false otherwise
 **/
bool BindingImageTest::verifyBuffer(const Utils::buffer& buffer) const
{
	GLuint* data = (GLuint*)buffer.map(GL_READ_ONLY);

	GLuint color = data[0];

	buffer.unmap();

	return (m_green_color == color);
}

/** Verifies that texel at offset 0 is green
 *
 * @param buffer Buffer object
 *
 * @return true if texel at offset 0 is green, false otherwise
 **/
bool BindingImageTest::verifyTexture(const Utils::texture& texture) const
{
	static const GLuint texture_data_size = m_width * m_height * m_depth;

	std::vector<glw::GLuint> texture_data;
	texture_data.resize(texture_data_size);

	texture.get(GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);

	GLuint color = texture_data[0];

	return (m_green_color == color);
}

/* Constants used by LineContinuationTest */
const GLuint  LineContinuationTest::m_n_repetitions			   = 20;
const GLchar* LineContinuationTest::m_texture_coordinates_name = "texture_coordinates";

/** Constructor
 *
 * @param context Test context
 **/
LineContinuationTest::LineContinuationTest(deqp::Context& context) : GLSLTestBase(context, "line_continuation", "desc")
{
	/* Nothing to be done here */
}

/** Overwrite getShaderSourceConfig method
 *
 * @param out_n_parts     Number of source parts used by this test case
 * @param out_use_lengths If source lengths shall be provided to compiler
 **/
void LineContinuationTest::getShaderSourceConfig(GLuint& out_n_parts, bool& out_use_lengths)
{
	out_n_parts		= (true == isShaderMultipart()) ? 2 : 1;
	out_use_lengths = useSourceLengths();
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool LineContinuationTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	static const testCase test_cases[] = { { ASSIGNMENT_BEFORE_OPERATOR, ONCE, UNIX },
										   { ASSIGNMENT_BEFORE_OPERATOR, ONCE, DOS },
										   { ASSIGNMENT_BEFORE_OPERATOR, MULTIPLE_TIMES, UNIX },
										   { ASSIGNMENT_BEFORE_OPERATOR, MULTIPLE_TIMES, DOS },
										   { ASSIGNMENT_AFTER_OPERATOR, ONCE, UNIX },
										   { ASSIGNMENT_AFTER_OPERATOR, ONCE, DOS },
										   { ASSIGNMENT_AFTER_OPERATOR, MULTIPLE_TIMES, UNIX },
										   { ASSIGNMENT_AFTER_OPERATOR, MULTIPLE_TIMES, DOS },
										   { VECTOR_VARIABLE_INITIALIZER, ONCE, UNIX },
										   { VECTOR_VARIABLE_INITIALIZER, ONCE, DOS },
										   { VECTOR_VARIABLE_INITIALIZER, MULTIPLE_TIMES, UNIX },
										   { VECTOR_VARIABLE_INITIALIZER, MULTIPLE_TIMES, DOS },
										   { TOKEN_INSIDE_FUNCTION_NAME, ONCE, UNIX },
										   { TOKEN_INSIDE_FUNCTION_NAME, ONCE, DOS },
										   { TOKEN_INSIDE_FUNCTION_NAME, MULTIPLE_TIMES, UNIX },
										   { TOKEN_INSIDE_FUNCTION_NAME, MULTIPLE_TIMES, DOS },
										   { TOKEN_INSIDE_TYPE_NAME, ONCE, UNIX },
										   { TOKEN_INSIDE_TYPE_NAME, ONCE, DOS },
										   { TOKEN_INSIDE_TYPE_NAME, MULTIPLE_TIMES, UNIX },
										   { TOKEN_INSIDE_TYPE_NAME, MULTIPLE_TIMES, DOS },
										   { TOKEN_INSIDE_VARIABLE_NAME, ONCE, UNIX },
										   { TOKEN_INSIDE_VARIABLE_NAME, ONCE, DOS },
										   { TOKEN_INSIDE_VARIABLE_NAME, MULTIPLE_TIMES, UNIX },
										   { TOKEN_INSIDE_VARIABLE_NAME, MULTIPLE_TIMES, DOS },
										   { PREPROCESSOR_TOKEN_INSIDE, ONCE, UNIX },
										   { PREPROCESSOR_TOKEN_INSIDE, ONCE, DOS },
										   { PREPROCESSOR_TOKEN_INSIDE, MULTIPLE_TIMES, UNIX },
										   { PREPROCESSOR_TOKEN_INSIDE, MULTIPLE_TIMES, DOS },
										   { PREPROCESSOR_TOKEN_BETWEEN, ONCE, UNIX },
										   { PREPROCESSOR_TOKEN_BETWEEN, ONCE, DOS },
										   { PREPROCESSOR_TOKEN_BETWEEN, MULTIPLE_TIMES, UNIX },
										   { PREPROCESSOR_TOKEN_BETWEEN, MULTIPLE_TIMES, DOS },
										   { COMMENT, ONCE, UNIX },
										   { COMMENT, ONCE, DOS },
										   { COMMENT, MULTIPLE_TIMES, UNIX },
										   { COMMENT, MULTIPLE_TIMES, DOS },
										   { SOURCE_TERMINATION_NULL, ONCE, UNIX },
										   { SOURCE_TERMINATION_NULL, ONCE, DOS },
										   { SOURCE_TERMINATION_NULL, MULTIPLE_TIMES, UNIX },
										   { SOURCE_TERMINATION_NULL, MULTIPLE_TIMES, DOS },
										   { SOURCE_TERMINATION_NON_NULL, ONCE, UNIX },
										   { SOURCE_TERMINATION_NON_NULL, ONCE, DOS },
										   { SOURCE_TERMINATION_NON_NULL, MULTIPLE_TIMES, UNIX },
										   { SOURCE_TERMINATION_NON_NULL, MULTIPLE_TIMES, DOS },
										   { PART_TERMINATION_NULL, ONCE, UNIX },
										   { PART_TERMINATION_NULL, ONCE, DOS },
										   { PART_TERMINATION_NULL, MULTIPLE_TIMES, UNIX },
										   { PART_TERMINATION_NULL, MULTIPLE_TIMES, DOS },
										   { PART_NEXT_TO_TERMINATION_NULL, ONCE, UNIX },
										   { PART_NEXT_TO_TERMINATION_NULL, ONCE, DOS },
										   { PART_NEXT_TO_TERMINATION_NULL, MULTIPLE_TIMES, UNIX },
										   { PART_NEXT_TO_TERMINATION_NULL, MULTIPLE_TIMES, DOS },
										   { PART_TERMINATION_NON_NULL, ONCE, UNIX },
										   { PART_TERMINATION_NON_NULL, ONCE, DOS },
										   { PART_TERMINATION_NON_NULL, MULTIPLE_TIMES, UNIX },
										   { PART_TERMINATION_NON_NULL, MULTIPLE_TIMES, DOS },
										   { PART_NEXT_TO_TERMINATION_NON_NULL, ONCE, UNIX },
										   { PART_NEXT_TO_TERMINATION_NON_NULL, ONCE, DOS },
										   { PART_NEXT_TO_TERMINATION_NON_NULL, MULTIPLE_TIMES, UNIX },
										   { PART_NEXT_TO_TERMINATION_NON_NULL, MULTIPLE_TIMES, DOS } };

	static const GLuint max_test_cases = sizeof(test_cases) / sizeof(testCase);

	if ((GLuint)-1 == test_case_index)
	{
		m_test_case.m_case = DEBUG_CASE;
	}
	else if (max_test_cases <= test_case_index)
	{
		return false;
	}
	else
	{
		m_test_case = test_cases[test_case_index];
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message
										<< "Test case: " << repetitionsToStr((REPETITIONS)m_test_case.m_repetitions)
										<< " line continuation, with "
										<< lineEndingsToStr((LINE_ENDINGS)m_test_case.m_line_endings)
										<< " line endings, is placed " << casesToStr((CASES)m_test_case.m_case)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void LineContinuationTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											   Utils::shaderSource& out_source)
{
	if (Utils::COMPUTE_SHADER == in_stage)
	{
		prepareComputShaderSource(out_source);
	}
	else
	{
		prepareShaderSourceForDraw(in_stage, in_use_version_400, out_source);
	}
}

/** Prepare compute shader source
 *
 * @param source Result shader source
 **/
void LineContinuationTest::prepareComputShaderSource(Utils::shaderSource& source)
{
	static const GLchar* shader_template_part_0 =
		"#version 430\n"
		"\n"
		"// Lorem ipsum dolor sit amCOMMENT_CASEet, consectetur adipiscing elit posuere.\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"/* Lorem ipsum dolor sit amet, conCOMMENT_CASEsectetur adipiscing elit posuere. */\n"
		"\n"
		"writeonly uniform image2D   uni_image;\n"
		"          uniform sampler2D uni_sampler;\n"
		"\n"
		"void funFUNCTION_CASEction(in veTYPE_CASEc4 in_vVARIABLE_CASEalue)\n"
		"{\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), inVARIABLE_CASE_value);\n"
		"}\n"
		"\n"
		"#define SET_PREPROCESSOR_INSIDE_CASERESULT(XX) "
		"PREPROCESSOR_BETWEEN_CASEfuncFUNCTION_CASEtion(XPREPROCESSOR_INSIDE_CASEX)\n"
		"NEXT_TO_TERMINATION_CASE\nTERMINATION_CASE";

	static const GLchar* shader_template_part_1 =
		"void main()\n"
		"{\n"
		"    ivec2 coordinates   ASSIGNMENT_BEFORE_OPERATOR_CASE=ASSIGNMENT_AFTER_OPERATOR_CASE "
		"ivec2(gl_GlobalInvocationID.xy + ivec2(16, 16));\n"
		"    vec4 sampled_color = texelFetch(uni_sampler, coordinates, 0 /* lod */);\n"
		"    vec4 result        = vec4(0, 0VECTOR_VARIABLE_INITIALIZER_CASE, 0, 1);\n"
		"\n"
		"    if (vec4(0, 0, 1, 1) == sampled_color)\n"
		"    {\n"
		"        result = vecTYPE_CASE4(VECTOR_VARIABLE_INITIALIZER_CASE0, 1, 0, 1);\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        result = vec4(coordinates.xy, sampled_color.rg);\n"
		"    }\n"
		"\n"
		"    SET_RESULT(result);"
		"}\n";

	/* Init strings with templates and replace all CASE tokens */
	if (true == isShaderMultipart())
	{
		source.m_parts[0].m_code = shader_template_part_0;
		source.m_parts[1].m_code = shader_template_part_1;

		replaceAllCaseTokens(source.m_parts[0].m_code);
		replaceAllCaseTokens(source.m_parts[1].m_code);
	}
	else
	{
		source.m_parts[0].m_code = shader_template_part_0;
		source.m_parts[0].m_code.append(shader_template_part_1);

		replaceAllCaseTokens(source.m_parts[0].m_code);
	}
}

/** Prepare source for given shader stage
 *
 * @param stage           Shader stage, compute shader will use 430
 * @param use_version_400 Select if 400 or 420 should be used
 * @param source          Result shader sources
 **/
void LineContinuationTest::prepareShaderSourceForDraw(Utils::SHADER_STAGES stage, bool use_version_400,
													  Utils::shaderSource& source)
{
	/* Templates */
	static const GLchar* shader_template_part_0 =
		"VERSION\n"
		"\n"
		"// Lorem ipsum dolor sit amCOMMENT_CASEet, consectetur adipiscing elit posuere.\n"
		"\n"
		"STAGE_SPECIFIC\n"
		"\n"
		"/* Lorem ipsum dolor sit amet, conCOMMENT_CASEsectetur adipiscing elit posuere. */\n"
		"\n"
		"IN_COLOR_DEFINITION\n"
		"IN_TEXTURE_COORDINATES_DEFINITION\n"
		"OUT_COLOR_DEFINITION\n"
		"OUT_TEXTURE_COORDINATES_DEFINITION\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"void funFUNCTION_CASEction(in veTYPE_CASEc4 in_vVARIABLE_CASEalue)\n"
		"{\n"
		"    OUT_COLOR ASSIGNMENT_BEFORE_OPERATOR_CASE=ASSIGNMENT_AFTER_OPERATOR_CASE inVARIABLE_CASE_value;\n"
		"}\n"
		"\n"
		"#define SET_PREPROCESSOR_INSIDE_CASERESULT(XX) "
		"PREPROCESSOR_BETWEEN_CASEfuncFUNCTION_CASEtion(XPREPROCESSOR_INSIDE_CASEX)\n"
		"NEXT_TO_TERMINATION_CASE\nTERMINATION_CASE";

	static const GLchar* shader_template_part_1 =
		"void main()\n"
		"{\n"
		"    vec2 coordinates   = TEXTURE_COORDINATES;\n"
		"    vec4 sampled_color = texture(uni_sampler, coordinates);\n"
		"    vec4 result        = vec4(0, 0VECTOR_VARIABLE_INITIALIZER_CASE, 0, 1);\n"
		"\n"
		"    if (PASS_CONDITION)\n"
		"    {\n"
		"        result = vecTYPE_CASE4(VECTOR_VARIABLE_INITIALIZER_CASE0, 1, 0, 1);\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        result = vec4(coordinates.xy, sampled_color.rg);\n"
		"    }\n"
		"\n"
		"STORE_RESULTS"
		"}\n"
		"NEXT_TO_TERMINATION_CASE\nTERMINATION_CASE";

	static const GLchar* store_results_template = "    SET_RESULT(result);\n"
												  "    TEXTURE_COORDINATES = coordinates;\n";

	static const GLchar* store_results_tcs_template = "    SET_RESULT(result);\n"
													  "    TEXTURE_COORDINATES = coordinates;\n"
													  "    gl_TessLevelOuter[0] = 1.0;\n"
													  "    gl_TessLevelOuter[1] = 1.0;\n"
													  "    gl_TessLevelOuter[2] = 1.0;\n"
													  "    gl_TessLevelOuter[3] = 1.0;\n"
													  "    gl_TessLevelInner[0] = 1.0;\n"
													  "    gl_TessLevelInner[1] = 1.0;\n";

	static const GLchar* store_results_fs_template = "    SET_RESULT(result);\n";

	static const GLchar* store_results_gs_template = "    gl_Position = vec4(-1, -1, 0, 1);\n"
													 "    SET_RESULT(result);\n"
													 "    TEXTURE_COORDINATES = coordinates + vec2(-0.25, -0.25);\n"
													 "    EmitVertex();\n"
													 "    gl_Position = vec4(-1, 1, 0, 1);\n"
													 "    SET_RESULT(result);\n"
													 "    TEXTURE_COORDINATES = coordinates + vec2(-0.25, 0.25);\n"
													 "    EmitVertex();\n"
													 "    gl_Position = vec4(1, -1, 0, 1);\n"
													 "    SET_RESULT(result);\n"
													 "    TEXTURE_COORDINATES = coordinates + vec2(0.25, -0.25);\n"
													 "    EmitVertex();\n"
													 "    gl_Position = vec4(1, 1, 0, 1);\n"
													 "    SET_RESULT(result);\n"
													 "    TEXTURE_COORDINATES = coordinates + vec2(0.25, 0.25);\n"
													 "    EmitVertex();\n";

	static const GLchar* pass_condition_template = "(EXPECTED_VALUE == sampled_color) &&\n"
												   "        (vec4(0, 1, 0, 1) == IN_COLOR) ";

	static const GLchar* pass_condition_vs_template = "EXPECTED_VALUE == sampled_color";

	/* Tokens to be replaced with GLSL stuff */
	static const GLchar* token_version		  = "VERSION";
	static const GLchar* token_stage_specific = "STAGE_SPECIFIC";

	static const GLchar* token_in_color_definition		= "IN_COLOR_DEFINITION";
	static const GLchar* token_in_tex_coord_definition  = "IN_TEXTURE_COORDINATES_DEFINITION";
	static const GLchar* token_out_color_definition		= "OUT_COLOR_DEFINITION";
	static const GLchar* token_out_tex_coord_definition = "OUT_TEXTURE_COORDINATES_DEFINITION";

	static const GLchar* token_expected_value	  = "EXPECTED_VALUE";
	static const GLchar* token_texture_coordinates = "TEXTURE_COORDINATES";
	static const GLchar* token_in_color			   = "IN_COLOR";
	static const GLchar* token_out_color		   = "OUT_COLOR";

	static const GLchar* token_store_results  = "STORE_RESULTS";
	static const GLchar* token_pass_condition = "PASS_CONDITION";

	/* Name of variable and empty string*/
	static const GLchar* color_name = "color";
	static const GLchar* empty		= "";

	/* GLSL stuff */
	const GLchar* version				= getVersionString(stage, use_version_400);
	const GLchar* stage_specific_layout = getStageSpecificLayout(stage);
	const GLchar* expected_value		= getExpectedValueString();

	/* Qualifiers */
	Utils::qualifierSet in;
	Utils::qualifierSet out;
	in.push_back(Utils::QUAL_IN);
	out.push_back(Utils::QUAL_OUT);

	/* In/Out variables definitions and references */
	std::string in_tex_coord_reference;
	std::string out_tex_coord_reference;
	std::string in_color_reference;
	std::string out_color_reference;
	std::string in_tex_coord_definition;
	std::string out_tex_coord_definition;
	std::string in_color_definition;
	std::string out_color_definition;

	Utils::prepareVariableStrings(stage, Utils::INPUT, in, "vec2", m_texture_coordinates_name, in_tex_coord_definition,
								  in_tex_coord_reference);
	Utils::prepareVariableStrings(stage, Utils::OUTPUT, out, "vec2", m_texture_coordinates_name,
								  out_tex_coord_definition, out_tex_coord_reference);
	Utils::prepareVariableStrings(stage, Utils::INPUT, in, "vec4", color_name, in_color_definition, in_color_reference);
	Utils::prepareVariableStrings(stage, Utils::OUTPUT, out, "vec4", color_name, out_color_definition,
								  out_color_reference);

	in_tex_coord_definition.append(";");
	out_tex_coord_definition.append(";");
	in_color_definition.append(";");
	out_color_definition.append(";");

	/* Select pass condition and store results tempaltes */
	const GLchar* store_results  = store_results_template;
	const GLchar* pass_condition = pass_condition_template;

	switch (stage)
	{
	case Utils::FRAGMENT_SHADER:
		store_results = store_results_fs_template;
		break;
	case Utils::GEOMETRY_SHADER:
		store_results = store_results_gs_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		store_results = store_results_tcs_template;
		break;
	case Utils::VERTEX_SHADER:
		pass_condition = pass_condition_vs_template;
		break;
	default:
		break;
	};
	const GLuint store_results_length  = static_cast<GLuint>(strlen(store_results));
	const GLuint pass_condition_length = static_cast<GLuint>(strlen(pass_condition));

	/* Init strings with templates and replace all CASE tokens */
	if (true == isShaderMultipart())
	{
		source.m_parts[0].m_code = shader_template_part_0;
		source.m_parts[1].m_code = shader_template_part_1;

		replaceAllCaseTokens(source.m_parts[0].m_code);
		replaceAllCaseTokens(source.m_parts[1].m_code);
	}
	else
	{
		source.m_parts[0].m_code = shader_template_part_0;
		source.m_parts[0].m_code.append(shader_template_part_1);

		replaceAllCaseTokens(source.m_parts[0].m_code);
	}

	/* Get memory for shader source parts */
	const bool   is_multipart		  = isShaderMultipart();
	size_t		 position			  = 0;
	std::string& shader_source_part_0 = source.m_parts[0].m_code;
	std::string& shader_source_part_1 = (true == is_multipart) ? source.m_parts[1].m_code : source.m_parts[0].m_code;

	/* Replace tokens */
	/* Part 0 */
	Utils::replaceToken(token_version, position, version, shader_source_part_0);

	Utils::replaceToken(token_stage_specific, position, stage_specific_layout, shader_source_part_0);

	if (Utils::VERTEX_SHADER != stage)
	{
		Utils::replaceToken(token_in_color_definition, position, in_color_definition.c_str(), shader_source_part_0);
	}
	else
	{
		Utils::replaceToken(token_in_color_definition, position, empty, shader_source_part_0);
	}
	Utils::replaceToken(token_in_tex_coord_definition, position, in_tex_coord_definition.c_str(), shader_source_part_0);
	Utils::replaceToken(token_out_color_definition, position, out_color_definition.c_str(), shader_source_part_0);
	if (Utils::FRAGMENT_SHADER == stage)
	{
		Utils::replaceToken(token_out_tex_coord_definition, position, empty, shader_source_part_0);
	}
	else
	{
		Utils::replaceToken(token_out_tex_coord_definition, position, out_tex_coord_definition.c_str(),
							shader_source_part_0);
	}

	Utils::replaceToken(token_out_color, position, out_color_reference.c_str(), shader_source_part_0);

	/* Part 1 */
	if (true == is_multipart)
	{
		position = 0;
	}

	Utils::replaceToken(token_texture_coordinates, position, in_tex_coord_reference.c_str(), shader_source_part_1);

	Utils::replaceToken(token_pass_condition, position, pass_condition, shader_source_part_1);
	position -= pass_condition_length;

	Utils::replaceToken(token_expected_value, position, expected_value, shader_source_part_1);
	if (Utils::VERTEX_SHADER != stage)
	{
		Utils::replaceToken(token_in_color, position, in_color_reference.c_str(), shader_source_part_1);
	}

	Utils::replaceToken(token_store_results, position, store_results, shader_source_part_1);
	position -= store_results_length;

	if (Utils::GEOMETRY_SHADER == stage)
	{
		for (GLuint i = 0; i < 4; ++i)
		{
			Utils::replaceToken(token_texture_coordinates, position, out_tex_coord_reference.c_str(),
								shader_source_part_1);
		}
	}
	else if (Utils::FRAGMENT_SHADER == stage)
	{
		/* Nothing to be done */
	}
	else
	{
		Utils::replaceToken(token_texture_coordinates, position, out_tex_coord_reference.c_str(), shader_source_part_1);
	}
}

/** Prepare texture
 *
 * @param texture Texutre to be created and filled with content
 *
 * @return Name of sampler uniform that should be used for the texture
 **/
const GLchar* LineContinuationTest::prepareSourceTexture(Utils::texture& texture)
{
	std::vector<GLuint> data;
	static const GLuint width	  = 64;
	static const GLuint height	 = 64;
	static const GLuint data_size  = width * height;
	static const GLuint blue_color = 0xffff0000;
	static const GLuint grey_color = 0xaaaaaaaa;

	data.resize(data_size);

	for (GLuint i = 0; i < data_size; ++i)
	{
		data[i] = grey_color;
	}

	for (GLuint y = 16; y < 48; ++y)
	{
		const GLuint line_offset = y * 64;

		for (GLuint x = 16; x < 48; ++x)
		{
			const GLuint pixel_offset = x + line_offset;

			data[pixel_offset] = blue_color;
		}
	}

	texture.create(width, height, GL_RGBA8);

	texture.update(width, height, 0 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

	return "uni_sampler";
}

/** Prepare vertex buffer, vec2 tex_coord
 *
 * @param program Program object
 * @param buffer  Vertex buffer
 * @param vao     Vertex array object
 **/
void LineContinuationTest::prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer,
											   Utils::vertexArray& vao)
{
	std::string tex_coord_name = Utils::getVariableName(Utils::VERTEX_SHADER, Utils::INPUT, m_texture_coordinates_name);
	GLint		tex_coord_loc  = program.getAttribLocation(tex_coord_name.c_str());

	if (-1 == tex_coord_loc)
	{
		TCU_FAIL("Vertex attribute location is invalid");
	}

	vao.generate();
	vao.bind();

	buffer.generate(GL_ARRAY_BUFFER);

	GLfloat	data[]	= { 0.5f, 0.5f, 0.5f, 0.5f };
	GLsizeiptr data_size = sizeof(data);

	buffer.update(data_size, data, GL_STATIC_DRAW);

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up vao */
	gl.vertexAttribPointer(tex_coord_loc, 2 /* size */, GL_FLOAT /* type */, GL_FALSE /* normalized*/, 0 /* stride */,
						   0 /* offset */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribPointer");

	/* Enable attribute */
	gl.enableVertexAttribArray(tex_coord_loc);
	GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");
}

/** Get string describing test cases
 *
 * @param cases Test case
 *
 * @return String describing current test case
 **/
const GLchar* LineContinuationTest::casesToStr(CASES cases) const
{
	const GLchar* result = 0;
	switch (cases)
	{
	case ASSIGNMENT_BEFORE_OPERATOR:
		result = "just before assignment operator";
		break;
	case ASSIGNMENT_AFTER_OPERATOR:
		result = "just after assignment operator";
		break;
	case VECTOR_VARIABLE_INITIALIZER:
		result = "inside vector variable initializer";
		break;
	case TOKEN_INSIDE_FUNCTION_NAME:
		result = "inside function name";
		break;
	case TOKEN_INSIDE_TYPE_NAME:
		result = "inside type name";
		break;
	case TOKEN_INSIDE_VARIABLE_NAME:
		result = "inside variable name";
		break;
	case PREPROCESSOR_TOKEN_INSIDE:
		result = "inside preprocessor token";
		break;
	case PREPROCESSOR_TOKEN_BETWEEN:
		result = "between preprocessor token";
		break;
	case COMMENT:
		result = "inside comment";
		break;
	case SOURCE_TERMINATION_NULL:
		result = "just before null terminating source";
		break;
	case SOURCE_TERMINATION_NON_NULL:
		result = "as last character in source string, without null termination";
		break;
	case PART_TERMINATION_NULL:
		result = "just before null terminating part of source";
		break;
	case PART_NEXT_TO_TERMINATION_NULL:
		result = "just before last character in part of source";
		break;
	case PART_TERMINATION_NON_NULL:
		result = "as last character in part string, without null termination";
		break;
	case PART_NEXT_TO_TERMINATION_NON_NULL:
		result = "just before last character in part string, without null termination";
		break;
	case DEBUG_CASE: /* intended fall through */
	default:
		result = "nowhere at all. This is debug!";
		break;
	}

	return result;
}

/** Get expected value, blue color as vec4
 *
 * @return blue color
 **/
const GLchar* LineContinuationTest::getExpectedValueString() const
{
	return "vec4(0, 0, 1, 1)";
}

/** Get line continuation string, single or multiple \
         *
 * @return String
 **/
std::string LineContinuationTest::getLineContinuationString() const
{
	static const GLchar line_continuation_ending_dos[]  = { '\\', 0x0d, 0x0a, 0x00 };
	static const GLchar line_continuation_ending_unix[] = { '\\', 0x0a, 0x00 };

	std::string   result;
	const GLchar* selected_string;

	if (DOS == m_test_case.m_line_endings)
	{
		selected_string = line_continuation_ending_dos;
	}
	else
	{
		selected_string = line_continuation_ending_unix;
	}

	GLuint n_repetitions = (ONCE == m_test_case.m_repetitions) ? 1 : m_n_repetitions;

	for (GLuint i = 0; i < n_repetitions; ++i)
	{
		result.append(selected_string);
	}

	return result;
}

/** Decides if shader should consist of multiple parts for the current test case
 *
 * @return true if test case requires multiple parts, false otherwise
 **/
bool LineContinuationTest::isShaderMultipart() const
{
	bool result;

	switch (m_test_case.m_case)
	{
	case ASSIGNMENT_BEFORE_OPERATOR:
	case ASSIGNMENT_AFTER_OPERATOR:
	case VECTOR_VARIABLE_INITIALIZER:
	case TOKEN_INSIDE_FUNCTION_NAME:
	case TOKEN_INSIDE_TYPE_NAME:
	case TOKEN_INSIDE_VARIABLE_NAME:
	case PREPROCESSOR_TOKEN_INSIDE:
	case PREPROCESSOR_TOKEN_BETWEEN:
	case COMMENT:
	case SOURCE_TERMINATION_NULL:
	case SOURCE_TERMINATION_NON_NULL:
	default:
		result = false;
		break;
	case PART_TERMINATION_NULL:
	case PART_NEXT_TO_TERMINATION_NULL:
	case PART_TERMINATION_NON_NULL:
	case PART_NEXT_TO_TERMINATION_NON_NULL:
		result = true;
		break;
	};

	return result;
}

/** String describing line endings
 *
 * @param line_ending Line ending enum
 *
 * @return "unix" or "dos" strings
 **/
const GLchar* LineContinuationTest::lineEndingsToStr(LINE_ENDINGS line_ending) const
{
	const GLchar* result = 0;

	if (UNIX == line_ending)
	{
		result = "unix";
	}
	else
	{
		result = "dos";
	}

	return result;
}

/** String describing number of repetitions
 *
 * @param repetitions Repetitions enum
 *
 * @return "single" or "multiple" strings
 **/
const GLchar* LineContinuationTest::repetitionsToStr(REPETITIONS repetitions) const
{
	const GLchar* result = 0;

	if (ONCE == repetitions)
	{
		result = "single";
	}
	else
	{
		result = "multiple";
	}

	return result;
}

/** Replace all CASES tokens
 *
 * @param source String with shader template
 **/
void LineContinuationTest::replaceAllCaseTokens(std::string& source) const
{

	/* Tokens to be replaced with line continuation */
	static const GLchar* token_assignment_before_operator_case = "ASSIGNMENT_BEFORE_OPERATOR_CASE";
	static const GLchar* token_assignment_after_operator_case  = "ASSIGNMENT_AFTER_OPERATOR_CASE";
	static const GLchar* token_vector_initializer			   = "VECTOR_VARIABLE_INITIALIZER_CASE";
	static const GLchar* token_function_case				   = "FUNCTION_CASE";
	static const GLchar* token_type_case					   = "TYPE_CASE";
	static const GLchar* token_variable_case				   = "VARIABLE_CASE";
	static const GLchar* token_preprocessor_inside_case		   = "PREPROCESSOR_INSIDE_CASE";
	static const GLchar* token_preprocessor_between_case	   = "PREPROCESSOR_BETWEEN_CASE";
	static const GLchar* token_comment						   = "COMMENT_CASE";
	static const GLchar* token_termination					   = "TERMINATION_CASE";
	static const GLchar* token_next_to_termination			   = "NEXT_TO_TERMINATION_CASE";

	/* Line continuation and empty string*/
	static const GLchar* empty			   = "";
	const std::string&   line_continuation = getLineContinuationString();

	/* These strings will used to replace "CASE" tokens */
	const GLchar* assignment_before_operator_case  = empty;
	const GLchar* assignment_after_operator_case   = empty;
	const GLchar* vector_variable_initializer_case = empty;
	const GLchar* function_case					   = empty;
	const GLchar* type_case						   = empty;
	const GLchar* variable_case					   = empty;
	const GLchar* preprocessor_inside_case		   = empty;
	const GLchar* preprocessor_between_case		   = empty;
	const GLchar* comment_case					   = empty;
	const GLchar* source_termination_case		   = empty;
	const GLchar* part_termination_case			   = empty;
	const GLchar* next_to_part_termination_case	= empty;

	/* Configuration of test case */
	switch (m_test_case.m_case)
	{
	case ASSIGNMENT_BEFORE_OPERATOR:
		assignment_before_operator_case = line_continuation.c_str();
		break;
	case ASSIGNMENT_AFTER_OPERATOR:
		assignment_after_operator_case = line_continuation.c_str();
		break;
	case VECTOR_VARIABLE_INITIALIZER:
		vector_variable_initializer_case = line_continuation.c_str();
		break;
	case TOKEN_INSIDE_FUNCTION_NAME:
		function_case = line_continuation.c_str();
		break;
	case TOKEN_INSIDE_TYPE_NAME:
		type_case = line_continuation.c_str();
		break;
	case TOKEN_INSIDE_VARIABLE_NAME:
		variable_case = line_continuation.c_str();
		break;
	case PREPROCESSOR_TOKEN_INSIDE:
		preprocessor_inside_case = line_continuation.c_str();
		break;
	case PREPROCESSOR_TOKEN_BETWEEN:
		preprocessor_between_case = line_continuation.c_str();
		break;
	case COMMENT:
		comment_case = line_continuation.c_str();
		break;
	case SOURCE_TERMINATION_NULL: /* intended fall through */
	case SOURCE_TERMINATION_NON_NULL:
		source_termination_case = line_continuation.c_str();
		break;
	case PART_TERMINATION_NULL: /* intended fall through */
	case PART_TERMINATION_NON_NULL:
		part_termination_case   = line_continuation.c_str();
		source_termination_case = line_continuation.c_str();
		break;
	case PART_NEXT_TO_TERMINATION_NULL: /* intended fall through */
	case PART_NEXT_TO_TERMINATION_NON_NULL:
		next_to_part_termination_case = line_continuation.c_str();
		break;
	case DEBUG_CASE: /* intended fall through */
	default:
		break; /* no line continuations */
	};

	Utils::replaceAllTokens(token_assignment_after_operator_case, assignment_after_operator_case, source);
	Utils::replaceAllTokens(token_assignment_before_operator_case, assignment_before_operator_case, source);
	Utils::replaceAllTokens(token_comment, comment_case, source);
	Utils::replaceAllTokens(token_function_case, function_case, source);
	Utils::replaceAllTokens(token_next_to_termination, next_to_part_termination_case, source);
	Utils::replaceAllTokens(token_termination, part_termination_case, source);
	Utils::replaceAllTokens(token_preprocessor_between_case, preprocessor_between_case, source);
	Utils::replaceAllTokens(token_preprocessor_inside_case, preprocessor_inside_case, source);
	Utils::replaceAllTokens(token_termination, source_termination_case, source);
	Utils::replaceAllTokens(token_type_case, type_case, source);
	Utils::replaceAllTokens(token_variable_case, variable_case, source);
	Utils::replaceAllTokens(token_vector_initializer, vector_variable_initializer_case, source);
}

/** Decides if the current test case requires source lengths
 *
 * @return true if test requires lengths, false otherwise
 **/
bool LineContinuationTest::useSourceLengths() const
{
	bool result;

	switch (m_test_case.m_case)
	{
	case ASSIGNMENT_BEFORE_OPERATOR:
	case ASSIGNMENT_AFTER_OPERATOR:
	case VECTOR_VARIABLE_INITIALIZER:
	case TOKEN_INSIDE_FUNCTION_NAME:
	case TOKEN_INSIDE_TYPE_NAME:
	case TOKEN_INSIDE_VARIABLE_NAME:
	case PREPROCESSOR_TOKEN_INSIDE:
	case PREPROCESSOR_TOKEN_BETWEEN:
	case COMMENT:
	case SOURCE_TERMINATION_NULL:
	case PART_TERMINATION_NULL:
	case PART_NEXT_TO_TERMINATION_NULL:
	default:
		result = false;
		break;
	case SOURCE_TERMINATION_NON_NULL:
	case PART_TERMINATION_NON_NULL:
	case PART_NEXT_TO_TERMINATION_NON_NULL:
		result = true;
		break;
	};

	return result;
}

/** Constructor
 *
 * @param context Test context
 **/
LineNumberingTest::LineNumberingTest(deqp::Context& context)
	: GLSLTestBase(context, "line_numbering", "Verify if line numbering is correct after line continuation")
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void LineNumberingTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											Utils::shaderSource& out_source)
{
	static const GLchar* test_result_snippet_normal[6] = { /* Utils::COMPUTE_SHADER */
														   "ivec4(11, 1, 2, 3)",
														   /* Utils::VERTEX_SHADER */
														   "ivec4(9, 1, 2, 3)",
														   /* Utils::TESS_CTRL_SHADER */
														   "ivec4(12, 1, 2, 3)",
														   /* Utils::TESS_EVAL_SHADER */
														   "ivec4(12, 1, 2, 3)",
														   /* Utils::GEOMETRY_SHADER */
														   "ivec4(13, 1, 2, 3)",
														   /* Utils::FRAGMENT_SHADER */
														   "ivec4(10, 1, 2, 3)"
	};

	static const GLchar* test_result_snippet_400[6] = { /* Utils::COMPUTE_SHADER */
														"ivec4(13, 1, 2, 3)",
														/* Utils::VERTEX_SHADER */
														"ivec4(11, 1, 2, 3)",
														/* Utils::TESS_CTRL_SHADER */
														"ivec4(14, 1, 2, 3)",
														/* Utils::TESS_EVAL_SHADER */
														"ivec4(14, 1, 2, 3)",
														/* Utils::GEOMETRY_SHADER */
														"ivec4(15, 1, 2, 3)",
														/* Utils::FRAGMENT_SHADER */
														"ivec4(12, 1, 2, 3)"
	};

	static const GLchar* line_numbering_snippet = "ivec4 glsl\\\n"
												  "Test\\\n"
												  "Function(in ivec3 arg)\n"
												  "{\n"
												  "    return ivec4(__LINE__, arg.xyz);\n"
												  "}\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"GLSL_TEST_FUNCTION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if (GLSL_TEST_RESULT == glslTestFunction(ivec3(1, 2, 3)))\n"
		"    {\n"
		"        result = vec4(0, 1, 0, 1);\n"
		"    }\n"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template =
		"VERSION\n"
		"\n"
		"in  vec4 gs_fs_result;\n"
		"out vec4 fs_out_result;\n"
		"\n"
		"GLSL_TEST_FUNCTION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((GLSL_TEST_RESULT == glslTestFunction(ivec3(1, 2, 3))) &&\n"
		"        (vec4(0, 1, 0, 1) == gs_fs_result) )\n"
		"    {\n"
		"         result = vec4(0, 1, 0, 1);\n"
		"    }\n"
		"\n"
		"    fs_out_result = result;\n"
		"}\n"
		"\n";

	static const GLchar* geometry_shader_template =
		"VERSION\n"
		"\n"
		"layout(points)                           in;\n"
		"layout(triangle_strip, max_vertices = 4) out;\n"
		"\n"
		"in  vec4 tes_gs_result[];\n"
		"out vec4 gs_fs_result;\n"
		"\n"
		"GLSL_TEST_FUNCTION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((GLSL_TEST_RESULT == glslTestFunction(ivec3(1, 2, 3))) &&\n"
		"        (vec4(0, 1, 0, 1) == tes_gs_result[0]) )\n"
		"    {\n"
		"         result = vec4(0, 1, 0, 1);\n"
		"    }\n"
		"\n"
		"    gs_fs_result = result;\n"
		"    gl_Position  = vec4(-1, -1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_result = result;\n"
		"    gl_Position  = vec4(-1, 1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_result = result;\n"
		"    gl_Position  = vec4(1, -1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_result = result;\n"
		"    gl_Position  = vec4(1, 1, 0, 1);\n"
		"    EmitVertex();\n"
		"}\n"
		"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"GLSL_TEST_FUNCTION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((GLSL_TEST_RESULT == glslTestFunction(ivec3(1, 2, 3))) &&\n"
		"        (vec4(0, 1, 0, 1) == vs_tcs_result[gl_InvocationID]) )\n"
		"    {\n"
		"         result = vec4(0, 1, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template =
		"VERSION\n"
		"\n"
		"layout(isolines, point_mode) in;\n"
		"\n"
		"in  vec4 tcs_tes_result[];\n"
		"out vec4 tes_gs_result;\n"
		"\n"
		"GLSL_TEST_FUNCTION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((GLSL_TEST_RESULT == glslTestFunction(ivec3(1, 2, 3))) &&\n"
		"        (vec4(0, 1, 0, 1) == tcs_tes_result[0]) )\n"
		"    {\n"
		"         result = vec4(0, 1, 0, 1);\n"
		"    }\n"
		"\n"
		"    tes_gs_result = result;\n"
		"}\n"
		"\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "GLSL_TEST_FUNCTION"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(1, 0, 0, 1);\n"
												  "\n"
												  "    if (GLSL_TEST_RESULT == glslTestFunction(ivec3(1, 2, 3)))\n"
												  "    {\n"
												  "         result = vec4(0, 1, 0, 1);\n"
												  "    }\n"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("GLSL_TEST_FUNCTION", position, line_numbering_snippet, out_source.m_parts[0].m_code);

	Utils::replaceToken("GLSL_TEST_RESULT", position,
						in_use_version_400 ? test_result_snippet_400[in_stage] : test_result_snippet_normal[in_stage],
						out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
UTF8CharactersTest::UTF8CharactersTest(deqp::Context& context)
	: GLSLTestBase(context, "utf8_characters", "UTF8 character used in comment or preprocessor")
{
	/* Nothing to be done here */
}

/** Overwrite getShaderSourceConfig method
 *
 * @param out_n_parts     Number of source parts used by this test case
 * @param out_use_lengths If source lengths shall be provided to compiler
 **/
void UTF8CharactersTest::getShaderSourceConfig(GLuint& out_n_parts, bool& out_use_lengths)
{
	out_n_parts		= 1;
	out_use_lengths = (AS_LAST_CHARACTER_NON_NULL_TERMINATED == m_test_case.m_case) ? true : false;
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool UTF8CharactersTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	static const testCase test_cases[] = {
		{ IN_COMMENT, Utils::TWO_BYTES },
		{ IN_COMMENT, Utils::THREE_BYTES },
		{ IN_COMMENT, Utils::FOUR_BYTES },
		{ IN_COMMENT, Utils::FIVE_BYTES },
		{ IN_COMMENT, Utils::SIX_BYTES },
		{ IN_COMMENT, Utils::REDUNDANT_ASCII },
		{ IN_PREPROCESSOR, Utils::TWO_BYTES },
		{ IN_PREPROCESSOR, Utils::THREE_BYTES },
		{ IN_PREPROCESSOR, Utils::FOUR_BYTES },
		{ IN_PREPROCESSOR, Utils::FIVE_BYTES },
		{ IN_PREPROCESSOR, Utils::SIX_BYTES },
		{ IN_PREPROCESSOR, Utils::REDUNDANT_ASCII },
		{ AS_LAST_CHARACTER_NULL_TERMINATED, Utils::TWO_BYTES },
		{ AS_LAST_CHARACTER_NULL_TERMINATED, Utils::THREE_BYTES },
		{ AS_LAST_CHARACTER_NULL_TERMINATED, Utils::FOUR_BYTES },
		{ AS_LAST_CHARACTER_NULL_TERMINATED, Utils::FIVE_BYTES },
		{ AS_LAST_CHARACTER_NULL_TERMINATED, Utils::SIX_BYTES },
		{ AS_LAST_CHARACTER_NULL_TERMINATED, Utils::REDUNDANT_ASCII },
		{ AS_LAST_CHARACTER_NON_NULL_TERMINATED, Utils::TWO_BYTES },
		{ AS_LAST_CHARACTER_NON_NULL_TERMINATED, Utils::THREE_BYTES },
		{ AS_LAST_CHARACTER_NON_NULL_TERMINATED, Utils::FOUR_BYTES },
		{ AS_LAST_CHARACTER_NON_NULL_TERMINATED, Utils::FIVE_BYTES },
		{ AS_LAST_CHARACTER_NON_NULL_TERMINATED, Utils::SIX_BYTES },
		{ AS_LAST_CHARACTER_NON_NULL_TERMINATED, Utils::REDUNDANT_ASCII },
	};

	static const GLuint max_test_cases = sizeof(test_cases) / sizeof(testCase);

	if ((GLuint)-1 == test_case_index)
	{
		m_test_case.m_case		= DEBUG_CASE;
		m_test_case.m_character = Utils::EMPTY;
	}
	else if (max_test_cases <= test_case_index)
	{
		return false;
	}
	else
	{
		m_test_case = test_cases[test_case_index];
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Test case: utf8 character: "
										<< Utils::getUtf8Character(m_test_case.m_character) << " is placed "
										<< casesToStr() << tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void UTF8CharactersTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											 Utils::shaderSource& out_source)
{
	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"// Lorem ipsum dolor sit amCOMMENT_CASEet, consectetur adipiscing elit posuere.\n"
		"\n"
		"/* Lorem ipsum dolor sit amet, conCOMMENT_CASEsectetur adipiscing elit posuere. */\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D   uni_image;\n"
		"          uniform sampler2D uni_sampler;\n"
		"\n"
		"#if 0\n"
		"    #define SET_REPREPROCESSOR_CASESULT(XX) result = XX\n"
		"#else\n"
		"    #define SET_RESULT(XX) result = XX\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    ivec2 coordinates = ivec2(gl_GlobalInvocationID.xy + ivec2(16, 16));\n"
		"    vec4  result      = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if (vec4(0, 0, 1, 1) == texelFetch(uni_sampler, coordinates, 0 /* lod */))\n"
		"    {\n"
		"        SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"// Lorem ipsum LAST_CHARACTER_CASE";

	static const GLchar* fragment_shader_template =
		"VERSION\n"
		"\n"
		"// Lorem ipsum dolor sit amCOMMENT_CASEet, consectetur adipiscing elit posuere.\n"
		"\n"
		"/* Lorem ipsum dolor sit amet, conCOMMENT_CASEsectetur adipiscing elit posuere. */\n"
		"\n"
		"in      vec4      gs_fs_result;\n"
		"in      vec2      gs_fs_tex_coord;\n"
		"out     vec4      fs_out_result;\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#if 0\n"
		"    #define SET_REPREPROCESSOR_CASESULT(XX) result = XX\n"
		"#else\n"
		"    #define SET_RESULT(XX) result = XX\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((vec4(0, 0, 1, 1) == texture(uni_sampler, gs_fs_tex_coord)) &&\n"
		"        (vec4(0, 1, 0, 1) == gs_fs_result) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    fs_out_result = result;\n"
		"}\n"
		"// Lorem ipsum LAST_CHARACTER_CASE";

	static const GLchar* geometry_shader_template =
		"VERSION\n"
		"\n"
		"// Lorem ipsum dolor sit amCOMMENT_CASEet, consectetur adipiscing elit posuere.\n"
		"\n"
		"/* Lorem ipsum dolor sit amet, conCOMMENT_CASEsectetur adipiscing elit posuere. */\n"
		"\n"
		"layout(points)                           in;\n"
		"layout(triangle_strip, max_vertices = 4) out;\n"
		"\n"
		"in      vec4      tes_gs_result[];\n"
		"out     vec2      gs_fs_tex_coord;\n"
		"out     vec4      gs_fs_result;\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#if 0\n"
		"    #define SET_REPREPROCESSOR_CASESULT(XX) result = XX\n"
		"#else\n"
		"    #define SET_RESULT(XX) result = XX\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((vec4(0, 0, 1, 1) == texture(uni_sampler, vec2(0.5, 0.5))) &&\n"
		"        (vec4(0, 1, 0, 1) == tes_gs_result[0]) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    gs_fs_tex_coord = vec2(0.25, 0.25);\n"
		"    gs_fs_result    = result;\n"
		"    gl_Position     = vec4(-1, -1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_tex_coord = vec2(0.25, 0.75);\n"
		"    gs_fs_result    = result;\n"
		"    gl_Position     = vec4(-1, 1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_tex_coord = vec2(0.75, 0.25);\n"
		"    gs_fs_result    = result;\n"
		"    gl_Position     = vec4(1, -1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_tex_coord = vec2(0.75, 0.75);\n"
		"    gs_fs_result    = result;\n"
		"    gl_Position     = vec4(1, 1, 0, 1);\n"
		"    EmitVertex();\n"
		"}\n"
		"// Lorem ipsum LAST_CHARACTER_CASE";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"// Lorem ipsum dolor sit amCOMMENT_CASEet, consectetur adipiscing elit posuere.\n"
		"\n"
		"/* Lorem ipsum dolor sit amet, conCOMMENT_CASEsectetur adipiscing elit posuere. */\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in      vec4      vs_tcs_result[];\n"
		"out     vec4      tcs_tes_result[];\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#if 0\n"
		"    #define SET_REPREPROCESSOR_CASESULT(XX) result = XX\n"
		"#else\n"
		"    #define SET_RESULT(XX) result = XX\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((vec4(0, 0, 1, 1) == texture(uni_sampler, vec2(0.4, 0.4))) &&\n"
		"        (vec4(0, 1, 0, 1) == vs_tcs_result[gl_InvocationID]) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"// Lorem ipsum LAST_CHARACTER_CASE";

	static const GLchar* tess_eval_shader_template =
		"VERSION\n"
		"\n"
		"// Lorem ipsum dolor sit amCOMMENT_CASEet, consectetur adipiscing elit posuere.\n"
		"\n"
		"/* Lorem ipsum dolor sit amet, conCOMMENT_CASEsectetur adipiscing elit posuere. */\n"
		"\n"
		"layout(isolines, point_mode) in;\n"
		"\n"
		"in      vec4      tcs_tes_result[];\n"
		"out     vec4      tes_gs_result;\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#if 0\n"
		"    #define SET_REPREPROCESSOR_CASESULT(XX) result = XX\n"
		"#else\n"
		"    #define SET_RESULT(XX) result = XX\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((vec4(0, 0, 1, 1) == texture(uni_sampler, vec2(0.6, 0.6))) &&\n"
		"        (vec4(0, 1, 0, 1) == tcs_tes_result[0]) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    tes_gs_result = result;\n"
		"}\n"
		"// Lorem ipsum LAST_CHARACTER_CASE";

	static const GLchar* vertex_shader_template =
		"VERSION\n"
		"\n"
		"// Lorem ipsum dolor sit amCOMMENT_CASEet, consectetur adipiscing elit posuere.\n"
		"\n"
		"/* Lorem ipsum dolor sit amet, conCOMMENT_CASEsectetur adipiscing elit posuere. */\n"
		"\n"
		"out     vec4      vs_tcs_result;\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#if 0\n"
		"    #define SET_REPREPROCESSOR_CASESULT(XX) result = XX\n"
		"#else\n"
		"    #define SET_RESULT(XX) result = XX\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if (vec4(0, 0, 1, 1) == texture(uni_sampler, vec2(0.5, 0.5)) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    vs_tcs_result = result;\n"
		"}\n"
		"// Lorem ipsum LAST_CHARACTER_CASE";

	const GLchar* shader_template	 = 0;
	const GLchar* comment_case		  = "";
	const GLchar* preprocessor_case   = "";
	const GLchar* last_character_case = "";
	const GLchar* utf8_character	  = Utils::getUtf8Character(m_test_case.m_character);

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	switch (m_test_case.m_case)
	{
	case IN_COMMENT:
		comment_case = utf8_character;
		break;
	case IN_PREPROCESSOR:
		preprocessor_case = utf8_character;
		break;
	case AS_LAST_CHARACTER_NULL_TERMINATED:
		last_character_case = utf8_character;
		break;
	case AS_LAST_CHARACTER_NON_NULL_TERMINATED:
		last_character_case = utf8_character;
		break;
	case DEBUG_CASE:
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("COMMENT_CASE", comment_case, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("PREPROCESSOR_CASE", preprocessor_case, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("LAST_CHARACTER_CASE", last_character_case, out_source.m_parts[0].m_code);
}

/** Prepare texture
 *
 * @param texture Texutre to be created and filled with content
 *
 * @return Name of sampler uniform that should be used for the texture
 **/
const GLchar* UTF8CharactersTest::prepareSourceTexture(Utils::texture& texture)
{
	std::vector<GLuint> data;
	static const GLuint width	  = 64;
	static const GLuint height	 = 64;
	static const GLuint data_size  = width * height;
	static const GLuint blue_color = 0xffff0000;
	static const GLuint grey_color = 0xaaaaaaaa;

	data.resize(data_size);

	for (GLuint i = 0; i < data_size; ++i)
	{
		data[i] = grey_color;
	}

	for (GLuint y = 16; y < 48; ++y)
	{
		const GLuint line_offset = y * 64;

		for (GLuint x = 16; x < 48; ++x)
		{
			const GLuint pixel_offset = x + line_offset;

			data[pixel_offset] = blue_color;
		}
	}

	texture.create(width, height, GL_RGBA8);

	texture.update(width, height, 0 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

	return "uni_sampler";
}

/** Returns description of current test case
 *
 * @return String with description
 **/
const GLchar* UTF8CharactersTest::casesToStr() const
{
	const GLchar* result = 0;

	switch (m_test_case.m_case)
	{
	case IN_COMMENT:
		result = "in comment";
		break;
	case IN_PREPROCESSOR:
		result = "in preprocessor";
		break;
	case AS_LAST_CHARACTER_NULL_TERMINATED:
		result = "just before null";
		break;
	case AS_LAST_CHARACTER_NON_NULL_TERMINATED:
		result = "as last character";
		break;
	case DEBUG_CASE:
		result = "nowhere. This is debug!";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return result;
}

/** Constructor
 *
 * @param context Test context
 **/
UTF8InSourceTest::UTF8InSourceTest(deqp::Context& context)
	: NegativeTestBase(context, "utf8_in_source", "UTF8 characters used in shader source")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool UTF8InSourceTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	static const Utils::UTF8_CHARACTERS test_cases[] = {
		Utils::TWO_BYTES,  Utils::THREE_BYTES, Utils::FOUR_BYTES,
		Utils::FIVE_BYTES, Utils::SIX_BYTES,   Utils::REDUNDANT_ASCII
	};

	static const GLuint max_test_cases = sizeof(test_cases) / sizeof(Utils::UTF8_CHARACTERS);

	if ((GLuint)-1 == test_case_index)
	{
		m_character = Utils::EMPTY;
	}
	else if (max_test_cases <= test_case_index)
	{
		return false;
	}
	else
	{
		m_character = test_cases[test_case_index];
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message
										<< "Test case: utf8 character: " << Utils::getUtf8Character(m_character)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void UTF8InSourceTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
										   Utils::shaderSource& out_source)
{
	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D   uni_image;\n"
		"          uniform sampler2D uni_sampler;\n"
		"\n"
		"#define SET_RESULT(XX) resHEREult = XX\n"
		"\n"
		"void main()\n"
		"{\n"
		"    ivec2 coordinates = ivec2(gl_GlobalInvocationID.xy + ivec2(16, 16));\n"
		"    vec4  resHEREult      = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if (vec4(0, 0, 1, 1) == texelFetch(uni_sampler, coordinates, 0 /* lod */))\n"
		"    {\n"
		"        SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), resHEREult);\n"
		"}\n"
		"";

	static const GLchar* fragment_shader_template =
		"VERSION\n"
		"\n"
		"in      vec4      gs_fs_result;\n"
		"in      vec2      gs_fs_tex_coord;\n"
		"out     vec4      fs_out_result;\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#define SET_RESULT(XX) resHEREult = XX\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 resHEREult = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((vec4(0, 0, 1, 1) == texture(uni_sampler, gs_fs_tex_coord)) &&\n"
		"        (vec4(0, 1, 0, 1) == gs_fs_result) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    fs_out_result = resHEREult;\n"
		"}\n"
		"\n";

	static const GLchar* geometry_shader_template =
		"VERSION\n"
		"\n"
		"layout(points)                           in;\n"
		"layout(triangle_strip, max_vertices = 4) out;\n"
		"\n"
		"in      vec4      tes_gHEREs_result[];\n"
		"out     vec2      gs_fs_tex_coord;\n"
		"out     vec4      gs_fs_result;\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#define SET_RESULT(XX) result = XX\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((vec4(0, 0, 1, 1) == texture(uni_sampler, vec2(0.5, 0.5))) &&\n"
		"        (vec4(0, 1, 0, 1) == tes_gHEREs_result[0]) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    gs_fs_tex_coord = vec2(0.25, 0.25);\n"
		"    gs_fs_result    = result;\n"
		"    gl_Position     = vec4(-1, -1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_tex_coord = vec2(0.25, 0.75);\n"
		"    gs_fs_result    = result;\n"
		"    gl_Position     = vec4(-1, 1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_tex_coord = vec2(0.75, 0.25);\n"
		"    gs_fs_result    = result;\n"
		"    gl_Position     = vec4(1, -1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_tex_coord = vec2(0.75, 0.75);\n"
		"    gs_fs_result    = result;\n"
		"    gl_Position     = vec4(1, 1, 0, 1);\n"
		"    EmitVertex();\n"
		"}\n"
		"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in      vec4      vs_tcs_result[];\n"
		"out     vec4      tcHEREs_tes_result[];\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#define SET_RESULT(XX) resulHEREt = XX\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 resulHEREt = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((vec4(0, 0, 1, 1) == texture(uni_sampler, vec2(0.4, 0.4))) &&\n"
		"        (vec4(0, 1, 0, 1) == vs_tcs_result[gl_InvocationID]) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    tcHEREs_tes_result[gl_InvocationID] = resulHEREt;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template =
		"VERSION\n"
		"\n"
		"layout(isolines, point_mode) in;\n"
		"\n"
		"in      vec4      tcs_tes_result[];\n"
		"out     vec4      teHEREs_gs_result;\n"
		"uniform sampler2D uni_sampler;\n"
		"\n"
		"#define SET_RESULT(XX) reHEREsult = XX\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 reHEREsult = vec4(1, 0, 0, 1);\n"
		"\n"
		"    if ((vec4(0, 0, 1, 1) == texture(uni_sampler, vec2(0.6, 0.6))) &&\n"
		"        (vec4(0, 1, 0, 1) == tcs_tes_result[0]) )\n"
		"    {\n"
		"         SET_RESULT(vec4(0, 1, 0, 1));\n"
		"    }\n"
		"\n"
		"    teHEREs_gs_result = reHEREsult;\n"
		"}\n"
		"\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out     vec4      vs_tcs_HEREresult;\n"
												  "uniform sampler2D uni_sampler;\n"
												  "\n"
												  "#define SET_RHEREESULT(XX) resHEREult = XX\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 resHEREult = vec4(1, 0, 0, 1);\n"
												  "\n"
												  "    if (vec4(0, 0, 1, 1) == texture(uni_sampler, vec2(0.5, 0.5)) )\n"
												  "    {\n"
												  "         SET_RHEREESULT(vec4(0, 1, 0, 1));\n"
												  "    }\n"
												  "\n"
												  "    vs_tcs_HEREresult = resHEREult;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;
	const GLchar* utf8_character  = Utils::getUtf8Character(m_character);

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("HERE", utf8_character, out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
ImplicitConversionsValidTest::ImplicitConversionsValidTest(deqp::Context& context)
	: GLSLTestBase(context, "implicit_conversions", "Verifies that implicit conversions are allowed")
{
	/* Nothing to be done */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool ImplicitConversionsValidTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		return true;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	const testCase& test_case = m_test_cases[test_case_index];

	m_context.getTestContext().getLog() << tcu::TestLog::Message
										<< "T1:" << Utils::getTypeName(test_case.m_types.m_t1, test_case.m_n_cols,
																	   test_case.m_n_rows)
										<< " T2:" << Utils::getTypeName(test_case.m_types.m_t2, test_case.m_n_cols,
																		test_case.m_n_rows)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void ImplicitConversionsValidTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													   Utils::shaderSource& out_source)
{
	static const GLchar* function_definition = "T1 function(in T2 left, in T2 right)\n"
											   "{\n"
											   "    return left + right;\n"
											   "}\n";

	static const GLchar* verification_snippet = "    const T2 const_left  = T2(VALUE_LIST);\n"
												"    const T2 const_right = T2(VALUE_LIST);\n"
												"\n"
												"    T1 const_result = function(const_left, const_right);\n"
												"\n"
												"    T1 literal_result = function(T2(VALUE_LIST), T2(VALUE_LIST));\n"
												"\n"
												"    T2 var_left  = uni_left;\n"
												"    T2 var_right = uni_right;\n"
												"\n"
												"    T1 var_result = function(var_left, var_right);\n"
												"\n"
												"    if ((literal_result != const_result) ||\n"
												"        (const_result   != var_result) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform T2 uni_left;\n"
		"          uniform T2 uni_right;\n"
		"\n"
		"FUNCTION_DEFINITION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"uniform T2 uni_left;\n"
													"uniform T2 uni_right;\n"
													"\n"
													"FUNCTION_DEFINITION"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"uniform T2 uni_left;\n"
													"uniform T2 uni_right;\n"
													"\n"
													"FUNCTION_DEFINITION"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"uniform T2 uni_left;\n"
		"uniform T2 uni_right;\n"
		"\n"
		"FUNCTION_DEFINITION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "uniform T2 uni_left;\n"
													 "uniform T2 uni_right;\n"
													 "\n"
													 "FUNCTION_DEFINITION"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "uniform T2 uni_left;\n"
												  "uniform T2 uni_right;\n"
												  "\n"
												  "FUNCTION_DEFINITION"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const testCase&	test_case  = getCurrentTestCase();
	const GLchar*	  t1		  = Utils::getTypeName(test_case.m_types.m_t1, test_case.m_n_cols, test_case.m_n_rows);
	const GLchar*	  t2		  = Utils::getTypeName(test_case.m_types.m_t2, test_case.m_n_cols, test_case.m_n_rows);
	const std::string& value_list = getValueList(test_case.m_n_cols, test_case.m_n_rows);
	const GLchar*	  shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("FUNCTION_DEFINITION", position, function_definition, out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("VALUE_LIST", value_list.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("T1", t1, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("T2", t2, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void ImplicitConversionsValidTest::prepareUniforms(Utils::program& program)
{
	static const GLdouble double_data[16] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
											  1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
	static const GLfloat float_data[16] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
											1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLint  int_data[4]  = { 1, 1, 1, 1 };
	static const GLuint uint_data[4] = { 1u, 1u, 1u, 1u };

	const testCase& test_case = getCurrentTestCase();

	switch (test_case.m_types.m_t2)
	{
	case Utils::DOUBLE:
		program.uniform("uni_left", Utils::DOUBLE, test_case.m_n_cols, test_case.m_n_rows, double_data);
		program.uniform("uni_right", Utils::DOUBLE, test_case.m_n_cols, test_case.m_n_rows, double_data);
		break;
	case Utils::FLOAT:
		program.uniform("uni_left", Utils::FLOAT, test_case.m_n_cols, test_case.m_n_rows, float_data);
		program.uniform("uni_right", Utils::FLOAT, test_case.m_n_cols, test_case.m_n_rows, float_data);
		break;
	case Utils::INT:
		program.uniform("uni_left", Utils::INT, test_case.m_n_cols, test_case.m_n_rows, int_data);
		program.uniform("uni_right", Utils::INT, test_case.m_n_cols, test_case.m_n_rows, int_data);
		break;
	case Utils::UINT:
		program.uniform("uni_left", Utils::UINT, test_case.m_n_cols, test_case.m_n_rows, uint_data);
		program.uniform("uni_right", Utils::UINT, test_case.m_n_cols, test_case.m_n_rows, uint_data);
		break;
	default:
		TCU_FAIL("Invalid enum");
	}
}

/** Prepare test cases
 *
 * @return true
 **/
bool ImplicitConversionsValidTest::testInit()
{
	static const typesPair allowed_conversions[] = {
		{ Utils::UINT, Utils::INT },   { Utils::FLOAT, Utils::INT },   { Utils::DOUBLE, Utils::INT },
		{ Utils::FLOAT, Utils::UINT }, { Utils::DOUBLE, Utils::UINT }, { Utils::FLOAT, Utils::FLOAT },
	};

	static GLuint n_allowed_conversions = sizeof(allowed_conversions) / sizeof(typesPair);

	m_debug_test_case.m_types.m_t1 = Utils::FLOAT;
	m_debug_test_case.m_types.m_t2 = Utils::FLOAT;
	m_debug_test_case.m_n_cols	 = 4;
	m_debug_test_case.m_n_rows	 = 4;

	for (GLuint i = 0; i < n_allowed_conversions; ++i)
	{
		const typesPair& types = allowed_conversions[i];

		GLuint allowed_columns = 1;
		if ((true == Utils::doesTypeSupportMatrix(types.m_t1)) && (true == Utils::doesTypeSupportMatrix(types.m_t2)))
		{
			allowed_columns = 4;
		}

		{
			testCase test_case = { types, 1, 1 };

			m_test_cases.push_back(test_case);
		}

		for (GLuint row = 2; row <= 4; ++row)
		{
			for (GLuint col = 1; col <= allowed_columns; ++col)
			{
				testCase test_case = { types, col, row };

				m_test_cases.push_back(test_case);
			}
		}
	}

	return true;
}

/** Returns reference to current test case
 *
 * @return Reference to testCase
 **/
const ImplicitConversionsValidTest::testCase& ImplicitConversionsValidTest::getCurrentTestCase()
{
	if ((glw::GLuint)-1 == m_current_test_case_index)
	{
		return m_debug_test_case;
	}
	else
	{
		return m_test_cases[m_current_test_case_index];
	}
}

/** Get list of values to for glsl constants
 *
 * @param n_columns Number of columns
 * @param n_rows    Number of rows
 *
 * @return String with list of values separated with comma
 **/
std::string ImplicitConversionsValidTest::getValueList(glw::GLuint n_columns, glw::GLuint n_rows)
{
	std::string result;

	for (GLuint i = 0; i < n_columns * n_rows; ++i)
	{
		if (i != n_columns * n_rows - 1)
		{
			result.append("1, ");
		}
		else
		{
			result.append("1");
		}
	}

	return result;
}

/** Constructor
 *
 * @param context Test context
 **/
ImplicitConversionsInvalidTest::ImplicitConversionsInvalidTest(deqp::Context& context)
	: NegativeTestBase(context, "implicit_conversions_invalid",
					   "Verifies that implicit conversions from uint to int are forbidden")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool ImplicitConversionsInvalidTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		return false;
	}
	else if (4 <= test_case_index)
	{
		return false;
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message
										<< "T1:" << Utils::getTypeName(Utils::UINT, 1, test_case_index + 1)
										<< " T2:" << Utils::getTypeName(Utils::INT, 1, test_case_index + 1)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void ImplicitConversionsInvalidTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														 Utils::shaderSource& out_source)
{
	static const GLchar* function_definition = "T1 function(in T2 left, in T2 right)\n"
											   "{\n"
											   "    return left + right;\n"
											   "}\n";

	static const GLchar* verification_snippet = "    const T2 const_left  = T2(VALUE_LIST);\n"
												"    const T2 const_right = T2(VALUE_LIST);\n"
												"\n"
												"    T1 const_result = function(const_left, const_right);\n"
												"\n"
												"    T1 literal_result = function(T2(VALUE_LIST), T2(VALUE_LIST));\n"
												"\n"
												"    T2 var_left  = uni_left;\n"
												"    T2 var_right = uni_right;\n"
												"\n"
												"    T1 var_result = function(var_left, var_right);\n"
												"\n"
												"    if ((literal_result != const_result) ||\n"
												"        (const_result   != var_result) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform T2 uni_left;\n"
		"          uniform T2 uni_right;\n"
		"\n"
		"FUNCTION_DEFINITION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"uniform T2 uni_left;\n"
													"uniform T2 uni_right;\n"
													"\n"
													"FUNCTION_DEFINITION"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"uniform T2 uni_left;\n"
													"uniform T2 uni_right;\n"
													"\n"
													"FUNCTION_DEFINITION"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"uniform T2 uni_left;\n"
		"uniform T2 uni_right;\n"
		"\n"
		"FUNCTION_DEFINITION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "uniform T2 uni_left;\n"
													 "uniform T2 uni_right;\n"
													 "\n"
													 "FUNCTION_DEFINITION"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "uniform T2 uni_left;\n"
												  "uniform T2 uni_right;\n"
												  "\n"
												  "FUNCTION_DEFINITION"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	GLuint			   n_rows		   = m_current_test_case_index + 1;
	const GLchar*	  t1			   = Utils::getTypeName(Utils::INT, 1, n_rows);
	const GLchar*	  t2			   = Utils::getTypeName(Utils::UINT, 1, n_rows);
	const std::string& value_list	  = getValueList(n_rows);
	const GLchar*	  shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("FUNCTION_DEFINITION", position, function_definition, out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("VALUE_LIST", value_list.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("T1", t1, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("T2", t2, out_source.m_parts[0].m_code);
}

/** Get list of values to for glsl constants
 *
 * @return String with list of values separated with comma
 **/
std::string ImplicitConversionsInvalidTest::getValueList(glw::GLuint n_rows)
{
	std::string result;

	for (GLuint i = 0; i < n_rows; ++i)
	{
		if (i != n_rows - 1)
		{
			result.append("1, ");
		}
		else
		{
			result.append("1");
		}
	}

	return result;
}

/** Constructor
 *
 * @param context Test context
 **/
ConstDynamicValueTest::ConstDynamicValueTest(deqp::Context& context)
	: GLSLTestBase(context, "const_dynamic_value", "Test if constants can be initialized with dynamic values")
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void ConstDynamicValueTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												Utils::shaderSource& out_source)
{
	static const GLchar* struct_definition = "struct S {\n"
											 "    float scalar;\n"
											 "    vec4  vector;\n"
											 "    mat2  matrix;\n"
											 "};\n";

	static const GLchar* verification_snippet = "    const float c1     = uni_scalar;\n"
												"    const vec4  c2     = uni_vector;\n"
												"    const mat2  c3     = uni_matrix;\n"
												"    const S     c4     = { uni_scalar, uni_vector, uni_matrix };\n"
												"    const vec4  c5[15] = { uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector,\n"
												"                           uni_vector };\n"
												"    if ((SCALAR != c1)        ||\n"
												"        (VECTOR != c2)        ||\n"
												"        (MATRIX != c3)        ||\n"
												"        (SCALAR != c4.scalar) ||\n"
												"        (VECTOR != c4.vector) ||\n"
												"        (MATRIX != c4.matrix) ||\n"
												"        (VECTOR != c5[0])     ||\n"
												"        (VECTOR != c5[1])     ||\n"
												"        (VECTOR != c5[2])     ||\n"
												"        (VECTOR != c5[3])     ||\n"
												"        (VECTOR != c5[4])     ||\n"
												"        (VECTOR != c5[5])     ||\n"
												"        (VECTOR != c5[6])     ||\n"
												"        (VECTOR != c5[7])     ||\n"
												"        (VECTOR != c5[8])     ||\n"
												"        (VECTOR != c5[9])     ||\n"
												"        (VECTOR != c5[10])    ||\n"
												"        (VECTOR != c5[11])    ||\n"
												"        (VECTOR != c5[12])    ||\n"
												"        (VECTOR != c5[13])    ||\n"
												"        (VECTOR != c5[14])    )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform float uni_scalar;\n"
		"          uniform vec4  uni_vector;\n"
		"          uniform mat2  uni_matrix;\n"
		"\n"
		"STRUCTURE_DEFINITION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"uniform float uni_scalar;\n"
													"uniform vec4  uni_vector;\n"
													"uniform mat2  uni_matrix;\n"
													"\n"
													"STRUCTURE_DEFINITION"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"uniform float uni_scalar;\n"
													"uniform vec4  uni_vector;\n"
													"uniform mat2  uni_matrix;\n"
													"\n"
													"STRUCTURE_DEFINITION"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"uniform float uni_scalar;\n"
		"uniform vec4  uni_vector;\n"
		"uniform mat2  uni_matrix;\n"
		"\n"
		"STRUCTURE_DEFINITION"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "uniform float uni_scalar;\n"
													 "uniform vec4  uni_vector;\n"
													 "uniform mat2  uni_matrix;\n"
													 "\n"
													 "STRUCTURE_DEFINITION"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "uniform float uni_scalar;\n"
												  "uniform vec4  uni_vector;\n"
												  "uniform mat2  uni_matrix;\n"
												  "\n"
												  "STRUCTURE_DEFINITION"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	static const GLchar* scalar = "0.5";
	static const GLchar* vector = "vec4(0.5, 0.125, 0.375, 0)";
	static const GLchar* matrix = "mat2(0.5, 0.125, 0.375, 0)";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("STRUCTURE_DEFINITION", position, struct_definition, out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("SCALAR", scalar, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("VECTOR", vector, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("MATRIX", matrix, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void ConstDynamicValueTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat float_data[4] = { 0.5f, 0.125f, 0.375f, 0.0f };
	static const GLfloat scalar		   = 0.5f;

	program.uniform("uni_scalar", Utils::FLOAT, 1, 1, &scalar);
	program.uniform("uni_vector", Utils::FLOAT, 1, 4, float_data);
	program.uniform("uni_matrix", Utils::FLOAT, 2, 2, float_data);
}

/** Constructor
 *
 * @param context Test context
 **/
ConstAssignmentTest::ConstAssignmentTest(deqp::Context& context)
	: NegativeTestBase(context, "const_assignment", "Verifies that constants cannot be overwritten")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool ConstAssignmentTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		return true;
	}
	else if (2 <= test_case_index)
	{
		return false;
	}

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void ConstAssignmentTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											  Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet = "    const float c1 = INIT;\n"
												"\n"
												"    float temp = c1;\n"
												"\n"
												"    for (uint i = 0; i < 4; ++i)"
												"    {\n"
												"        temp += c1 + uni_value;\n"
												"        c1 -= 0.125;\n"
												"    }\n"
												"\n"
												"    if (0.0 == temp)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform float uni_value;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"uniform float uni_value;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"uniform float uni_value;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"uniform float uni_value;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "uniform float uni_value;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "uniform float uni_value;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	static const GLchar* dynamic_init = "uni_value";
	static const GLchar* const_init   = "0.75";

	const GLchar* shader_template = 0;
	const GLchar* l_init		  = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	if (0 == m_current_test_case_index)
	{
		l_init = dynamic_init;
	}
	else
	{
		l_init = const_init;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("INIT", l_init, out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
ConstDynamicValueAsConstExprTest::ConstDynamicValueAsConstExprTest(deqp::Context& context)
	: NegativeTestBase(context, "const_dynamic_value_as_const_expr",
					   "Verifies that dynamic constants cannot be used as constant foldable expressions")
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void ConstDynamicValueAsConstExprTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														   Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet = "    const uint c1 = INIT;\n"
												"\n"
												"    float temp[c1];\n"
												"\n"
												"    for (uint i = 0; i < c1; ++i)"
												"    {\n"
												"        temp[i] += uni_value;\n"
												"    }\n"
												"\n"
												"    if (0.0 == temp[c1 - 1])\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform uint    uni_value;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"uniform uint uni_value;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"uniform uint uni_value;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"uniform uint uni_value;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "uniform uint uni_value;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "uniform uint uni_value;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	static const GLchar* l_init = "uni_value";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("INIT", l_init, out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
QualifierOrderTest::QualifierOrderTest(deqp::Context& context)
	: GLSLTestBase(context, "qualifier_order",
				   "Test verifies that valid permutation of input and output qalifiers are accepted")
{
	/* Nothing to be done */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool QualifierOrderTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		/* Nothing to be done here */;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	const Utils::qualifierSet& set = getCurrentTestCase();

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	for (GLuint i = 0; i < set.size(); ++i)
	{
		message << Utils::getQualifierString(set[i]) << " ";
	}

	message << tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void QualifierOrderTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											 Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet =
		"    vec4 diff = INPUT_VARIABLE_NAME - vec4(0, 0, 1, 1);\n"
		"    if (false == all(lessThan(diff, vec4(0.001, 0.001, 0.001, 0.001))))\n"
		"    {\n"
		"        result = INPUT_VARIABLE_NAME;\n"
		"    }\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"layout (location = 0) out vec4 fs_out_result;\n"
													"\n"
													"VARIABLE_DECLARATION;\n"
													"VARIABLE_DECLARATION;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"VARIABLE_DECLARATION;\n"
													"VARIABLE_DECLARATION;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"VARIABLE_DECLARATION;\n"
		"VARIABLE_DECLARATION;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "VARIABLE_DECLARATION;\n"
													 "VARIABLE_DECLARATION;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "VARIABLE_DECLARATION;\n"
												  "VARIABLE_DECLARATION;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		return;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	const Utils::qualifierSet& test_case = getCurrentTestCase();

	std::string in_test_decl;
	std::string in_test_ref;
	std::string out_test_decl;
	std::string out_test_ref;

	Utils::prepareVariableStrings(in_stage, Utils::INPUT, test_case, "vec4", "test", in_test_decl, in_test_ref);
	Utils::prepareVariableStrings(in_stage, Utils::OUTPUT, test_case, "vec4", "test", out_test_decl, out_test_ref);

	// sample storage qualifier is not a valid qualifier for fragment output
	if (in_stage == Utils::FRAGMENT_SHADER)
	{
		if (out_test_decl.find("sample") != std::string::npos)
			out_test_decl.erase(out_test_decl.find("sample"), 7);
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VARIABLE_DECLARATION", position, in_test_decl.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VARIABLE_DECLARATION", position, out_test_decl.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	position -= strlen(verification_snippet);

	Utils::replaceAllTokens("OUTPUT_VARIABLE_NAME", out_test_ref.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("INPUT_VARIABLE_NAME", in_test_ref.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("LOC_VALUE", "1", out_source.m_parts[0].m_code);
}

/**Prepare vertex buffer and vertex array object.
 *
 * @param program Program instance
 * @param buffer  Buffer instance
 * @param vao     VertexArray instance
 *
 * @return 0
 **/
void QualifierOrderTest::prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer,
											 Utils::vertexArray& vao)
{
	std::string test_name = Utils::getVariableName(Utils::VERTEX_SHADER, Utils::INPUT, "test");
	GLint		test_loc  = program.getAttribLocation(test_name.c_str());

	if (-1 == test_loc)
	{
		TCU_FAIL("Vertex attribute location is invalid");
	}

	vao.generate();
	vao.bind();

	buffer.generate(GL_ARRAY_BUFFER);

	GLfloat	data[]	= { 0.0f, 0.0f, 1.0f, 1.0f };
	GLsizeiptr data_size = sizeof(data);

	buffer.update(data_size, data, GL_STATIC_DRAW);

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up vao */
	gl.vertexAttribPointer(test_loc, 4 /* size */, GL_FLOAT /* type */, GL_FALSE /* normalized*/, 0 /* stride */,
						   0 /* offset */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribPointer");

	/* Enable attribute */
	gl.enableVertexAttribArray(test_loc);
	GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");
}

/** Prepare test cases
 *
 * @return true
 **/
bool QualifierOrderTest::testInit()
{
	m_test_cases.resize(5);

	m_test_cases[0].push_back(Utils::QUAL_HIGHP);
	m_test_cases[0].push_back(Utils::QUAL_IN);
	m_test_cases[0].push_back(Utils::QUAL_LOCATION);
	m_test_cases[0].push_back(Utils::QUAL_SMOOTH);
	m_test_cases[0].push_back(Utils::QUAL_INVARIANT);

	m_test_cases[1].push_back(Utils::QUAL_LOWP);
	m_test_cases[1].push_back(Utils::QUAL_IN);
	m_test_cases[1].push_back(Utils::QUAL_SAMPLE);
	m_test_cases[1].push_back(Utils::QUAL_LOCATION);
	m_test_cases[1].push_back(Utils::QUAL_NOPERSPECTIVE);
	m_test_cases[1].push_back(Utils::QUAL_INVARIANT);

	m_test_cases[2].push_back(Utils::QUAL_HIGHP);
	m_test_cases[2].push_back(Utils::QUAL_IN);
	m_test_cases[2].push_back(Utils::QUAL_LOCATION);
	m_test_cases[2].push_back(Utils::QUAL_SMOOTH);
	m_test_cases[2].push_back(Utils::QUAL_INVARIANT);
	m_test_cases[2].push_back(Utils::QUAL_LOCATION);

	m_test_cases[3].push_back(Utils::QUAL_LOWP);
	m_test_cases[3].push_back(Utils::QUAL_IN);
	m_test_cases[3].push_back(Utils::QUAL_LOCATION);
	m_test_cases[3].push_back(Utils::QUAL_SAMPLE);
	m_test_cases[3].push_back(Utils::QUAL_LOCATION);
	m_test_cases[3].push_back(Utils::QUAL_NOPERSPECTIVE);
	m_test_cases[3].push_back(Utils::QUAL_INVARIANT);

	m_test_cases[4].push_back(Utils::QUAL_HIGHP);
	m_test_cases[4].push_back(Utils::QUAL_IN);
	m_test_cases[4].push_back(Utils::QUAL_PATCH);
	m_test_cases[4].push_back(Utils::QUAL_LOCATION);
	m_test_cases[4].push_back(Utils::QUAL_INVARIANT);

	return true;
}

/** Returns reference to current test case
 *
 * @return Reference to testCase
 **/
const Utils::qualifierSet& QualifierOrderTest::getCurrentTestCase()
{
	if ((glw::GLuint)-1 == m_current_test_case_index)
	{
		return m_test_cases[0];
	}
	else
	{
		return m_test_cases[m_current_test_case_index];
	}
}

/** Constructor
 *
 * @param context Test context
 **/
QualifierOrderBlockTest::QualifierOrderBlockTest(deqp::Context& context)
	: GLSLTestBase(context, "qualifier_order_block",
				   "Verifies that qualifiers of members of input block can be arranged in any order")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool QualifierOrderBlockTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		/* Nothing to be done here */;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	const Utils::qualifierSet& set = getCurrentTestCase();

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	for (GLuint i = 0; i < set.size(); ++i)
	{
		message << Utils::getQualifierString(set[i]) << " ";
	}

	message << tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void QualifierOrderBlockTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												  Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet =
		"    vec4 diff = INPUT_VARIABLE_NAME - vec4(0, 0, 1, 1);\n"
		"    if (false == all(lessThan(diff, vec4(0.001, 0.001, 0.001, 0.001))))\n"
		"    {\n"
		"        result = INPUT_VARIABLE_NAME;\n"
		"    }\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"layout (location = 0) out vec4 fs_out_result;\n"
													"\n"
													"in GSOutputBlock {\n"
													"    VARIABLE_DECLARATION;\n"
													"} input_block;\n"
													"out VARIABLE_DECLARATION;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"in TCSOutputBlock {\n"
													"    VARIABLE_DECLARATION;\n"
													"} input_block [];\n"
													"out GSOutputBlock {\n"
													"    VARIABLE_DECLARATION;\n"
													"} output_block;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"in VSOutputBlock {\n"
		"    VARIABLE_DECLARATION;\n"
		"} input_block [];\n"
		"out TCSOutputBlock {\n"
		"    VARIABLE_DECLARATION;\n"
		"} output_block [];\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "in TCSOutputBlock {\n"
													 "    VARIABLE_DECLARATION;\n"
													 "} input_block [];\n"
													 "out TCSOutputBlock {\n"
													 "    VARIABLE_DECLARATION;\n"
													 "} output_block;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "in VARIABLE_DECLARATION;\n"
												  "out VSOutputBlock {\n"
												  "    VARIABLE_DECLARATION;\n"
												  "} output_block;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		return;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	const Utils::qualifierSet& test_case = getCurrentTestCase();

	std::string in_test_decl;
	std::string in_test_ref;
	std::string out_test_decl;
	std::string out_test_ref;

	switch (in_stage)
	{
	case Utils::VERTEX_SHADER:
		Utils::prepareVariableStrings(in_stage, Utils::INPUT, test_case, "vec4", "test", in_test_decl, in_test_ref);
		break;
	default:
		Utils::prepareBlockVariableStrings(in_stage, Utils::INPUT, test_case, "vec4", "test", "input_block",
										   in_test_decl, in_test_ref);
		break;
	}

	switch (in_stage)
	{
	case Utils::FRAGMENT_SHADER:
		Utils::prepareVariableStrings(in_stage, Utils::OUTPUT, test_case, "vec4", "test", out_test_decl, out_test_ref);
		break;
	default:
		Utils::prepareBlockVariableStrings(in_stage, Utils::OUTPUT, test_case, "vec4", "test", "output_block",
										   out_test_decl, out_test_ref);
		break;
	}

	// sample storage qualifier is not a valid qualifier for fragment output
	if (in_stage == Utils::FRAGMENT_SHADER)
	{
		if (out_test_decl.find("sample") != std::string::npos)
			out_test_decl.erase(out_test_decl.find("sample"), 7);
	}
	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VARIABLE_DECLARATION", position, in_test_decl.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VARIABLE_DECLARATION", position, out_test_decl.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	position -= strlen(verification_snippet);

	Utils::replaceAllTokens("OUTPUT_VARIABLE_NAME", out_test_ref.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("INPUT_VARIABLE_NAME", in_test_ref.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("LOC_VALUE", "1", out_source.m_parts[0].m_code);
}

/**Prepare vertex buffer and vertex array object.
 *
 * @param program Program instance
 * @param buffer  Buffer instance
 * @param vao     VertexArray instance
 *
 * @return 0
 **/
void QualifierOrderBlockTest::prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer,
												  Utils::vertexArray& vao)
{
	std::string test_name = Utils::getVariableName(Utils::VERTEX_SHADER, Utils::INPUT, "test");
	GLint		test_loc  = program.getAttribLocation(test_name.c_str());

	if (-1 == test_loc)
	{
		TCU_FAIL("Vertex attribute location is invalid");
	}

	vao.generate();
	vao.bind();

	buffer.generate(GL_ARRAY_BUFFER);

	GLfloat	data[]	= { 0.0f, 0.0f, 1.0f, 1.0f };
	GLsizeiptr data_size = sizeof(data);

	buffer.update(data_size, data, GL_STATIC_DRAW);

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up vao */
	gl.vertexAttribPointer(test_loc, 4 /* size */, GL_FLOAT /* type */, GL_FALSE /* normalized*/, 0 /* stride */,
						   0 /* offset */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribPointer");

	/* Enable attribute */
	gl.enableVertexAttribArray(test_loc);
	GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");
}

/** Prepare test cases
 *
 * @return true
 **/
bool QualifierOrderBlockTest::testInit()
{
	m_test_cases.resize(4);

	m_test_cases[0].push_back(Utils::QUAL_HIGHP);
	m_test_cases[0].push_back(Utils::QUAL_FLAT);
	m_test_cases[0].push_back(Utils::QUAL_INVARIANT);

	m_test_cases[1].push_back(Utils::QUAL_LOWP);
	m_test_cases[1].push_back(Utils::QUAL_SAMPLE);
	m_test_cases[1].push_back(Utils::QUAL_NOPERSPECTIVE);
	m_test_cases[1].push_back(Utils::QUAL_INVARIANT);

	m_test_cases[2].push_back(Utils::QUAL_HIGHP);
	m_test_cases[2].push_back(Utils::QUAL_SMOOTH);
	m_test_cases[2].push_back(Utils::QUAL_INVARIANT);

	m_test_cases[3].push_back(Utils::QUAL_LOWP);
	m_test_cases[3].push_back(Utils::QUAL_SAMPLE);
	m_test_cases[3].push_back(Utils::QUAL_NOPERSPECTIVE);
	m_test_cases[3].push_back(Utils::QUAL_INVARIANT);

	return true;
}

/** Returns reference to current test case
 *
 * @return Reference to testCase
 **/
const Utils::qualifierSet& QualifierOrderBlockTest::getCurrentTestCase()
{
	if ((glw::GLuint)-1 == m_current_test_case_index)
	{
		return m_test_cases[0];
	}
	else
	{
		return m_test_cases[m_current_test_case_index];
	}
}

/** Constructor
 *
 * @param context Test context
 **/
QualifierOrderUniformTest::QualifierOrderUniformTest(deqp::Context& context)
	: GLSLTestBase(context, "qualifier_order_uniform",
				   "Test verifies that all valid permutation of input qalifiers are accepted")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool QualifierOrderUniformTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		/* Nothing to be done here */;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	const Utils::qualifierSet& set = getCurrentTestCase();

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	for (GLuint i = 0; i < set.size(); ++i)
	{
		message << Utils::getQualifierString(set[i]) << " ";
	}

	message << tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void QualifierOrderUniformTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet =
		"    vec4 diff = VARIABLE_NAME - vec4(0, 0, 1, 1);\n"
		"    if (false == all(lessThan(diff, vec4(0.001, 0.001, 0.001, 0.001))))\n"
		"    {\n"
		"        result = VARIABLE_NAME;\n"
		"    }\n";

	static const GLchar* variable_declaration = "    QUALIFIERS VARIABLE_NAME";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"#extension GL_ARB_explicit_uniform_location : enable\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"VARIABLE_DECLARATION;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"#extension GL_ARB_explicit_uniform_location : enable\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"VARIABLE_DECLARATION;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"#extension GL_ARB_explicit_uniform_location : enable\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"VARIABLE_DECLARATION;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "#extension GL_ARB_explicit_uniform_location : enable\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "VARIABLE_DECLARATION;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "#extension GL_ARB_explicit_uniform_location : enable\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "VARIABLE_DECLARATION;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;
	const GLchar* location_string = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		return;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		location_string = "0";
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		location_string = "1";
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		location_string = "4";
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		location_string = "3";
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		location_string = "2";
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	const Utils::qualifierSet& test_case = getCurrentTestCase();

	std::string uni_declaration;
	std::string uni_reference;
	Utils::prepareVariableStrings(in_stage, Utils::UNIFORM, test_case, "vec4", "test", uni_declaration, uni_reference);

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VARIABLE_DECLARATION", position, uni_declaration.c_str(), out_source.m_parts[0].m_code);

	position -= strlen(variable_declaration);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("VARIABLE_NAME", uni_reference.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("LOC_VALUE", location_string, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void QualifierOrderUniformTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat float_data[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

	program.uniform("uni_fs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_gs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_tcs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_tes_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_vs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
}

/** Prepare test cases
 *
 * @return false if ARB_explicit_uniform_location is not supported, true otherwise
 **/
bool QualifierOrderUniformTest::testInit()
{
	if (false == m_is_explicit_uniform_location)
	{
		return false;
	}

	m_test_cases.resize(4);

	m_test_cases[0].push_back(Utils::QUAL_HIGHP);
	m_test_cases[0].push_back(Utils::QUAL_UNIFORM);
	m_test_cases[0].push_back(Utils::QUAL_LOCATION);

	m_test_cases[1].push_back(Utils::QUAL_LOWP);
	m_test_cases[1].push_back(Utils::QUAL_UNIFORM);
	m_test_cases[1].push_back(Utils::QUAL_LOCATION);

	m_test_cases[2].push_back(Utils::QUAL_HIGHP);
	m_test_cases[2].push_back(Utils::QUAL_LOCATION);
	m_test_cases[2].push_back(Utils::QUAL_UNIFORM);
	m_test_cases[2].push_back(Utils::QUAL_LOCATION);

	m_test_cases[3].push_back(Utils::QUAL_LOCATION);
	m_test_cases[3].push_back(Utils::QUAL_LOWP);
	m_test_cases[3].push_back(Utils::QUAL_UNIFORM);
	m_test_cases[3].push_back(Utils::QUAL_LOCATION);

	return true;
}

/** Returns reference to current test case
 *
 * @return Reference to testCase
 **/
const Utils::qualifierSet& QualifierOrderUniformTest::getCurrentTestCase()
{
	if ((glw::GLuint)-1 == m_current_test_case_index)
	{
		return m_test_cases[0];
	}
	else
	{
		return m_test_cases[m_current_test_case_index];
	}
}

/** Constructor
 *
 * @param context Test context
 **/
QualifierOrderFunctionInoutTest::QualifierOrderFunctionInoutTest(deqp::Context& context)
	: GLSLTestBase(context, "qualifier_order_function_inout", "Verify order of qualifiers of inout function parameters")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool QualifierOrderFunctionInoutTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		/* Nothing to be done here */;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	const Utils::qualifierSet& set = getCurrentTestCase();

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	for (GLuint i = 0; i < set.size(); ++i)
	{
		message << Utils::getQualifierString(set[i]) << " ";
	}

	message << tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void QualifierOrderFunctionInoutTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														  Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet =
		"    vec4 temp = VARIABLE_NAME;\n"
		"\n"
		"    function(temp);\n"
		"\n"
		"    vec4 diff = temp - vec4(0, 0, 1, 1);\n"
		"    if (false == all(lessThan(diff, vec4(0.001, 0.001, 0.001, 0.001))))\n"
		"    {\n"
		"        result = VARIABLE_NAME;\n"
		"    }\n";

	static const GLchar* function_declaration = "void function(QUALIFIERS_LIST vec4 param)\n"
												"{\n"
												"    param = param.wzyx;\n"
												"}\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"uniform vec4 VARIABLE_NAME;\n"
													"\n"
													"FUNCTION_DECLARATION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"uniform vec4 VARIABLE_NAME;\n"
													"\n"
													"FUNCTION_DECLARATION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"uniform vec4 VARIABLE_NAME;\n"
		"\n"
		"FUNCTION_DECLARATION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "uniform vec4 VARIABLE_NAME;\n"
													 "\n"
													 "FUNCTION_DECLARATION\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "uniform vec4 VARIABLE_NAME;\n"
												  "\n"
												  "FUNCTION_DECLARATION\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		return;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	const std::string& uni_reference  = Utils::getVariableName(in_stage, Utils::UNIFORM, "test");
	const std::string& qualifier_list = Utils::getQualifiersListString(getCurrentTestCase());

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("FUNCTION_DECLARATION", position, function_declaration, out_source.m_parts[0].m_code);

	position -= strlen(function_declaration);

	Utils::replaceToken("QUALIFIERS_LIST", position, qualifier_list.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("VARIABLE_NAME", uni_reference.c_str(), out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void QualifierOrderFunctionInoutTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat float_data[4] = { 1.0f, 1.0f, 0.0f, 0.0f };

	program.uniform("uni_fs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_gs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_tcs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_tes_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_vs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
}

/** Prepare test cases
 *
 * @return false if ARB_explicit_uniform_location is not supported, true otherwise
 **/
bool QualifierOrderFunctionInoutTest::testInit()
{
	m_test_cases.resize(6);

	m_test_cases[0].push_back(Utils::QUAL_HIGHP);
	m_test_cases[0].push_back(Utils::QUAL_PRECISE);
	m_test_cases[0].push_back(Utils::QUAL_INOUT);

	m_test_cases[1].push_back(Utils::QUAL_INOUT);
	m_test_cases[1].push_back(Utils::QUAL_PRECISE);
	m_test_cases[1].push_back(Utils::QUAL_HIGHP);

	m_test_cases[2].push_back(Utils::QUAL_MEDIUMP);
	m_test_cases[2].push_back(Utils::QUAL_PRECISE);
	m_test_cases[2].push_back(Utils::QUAL_INOUT);

	m_test_cases[3].push_back(Utils::QUAL_INOUT);
	m_test_cases[3].push_back(Utils::QUAL_PRECISE);
	m_test_cases[3].push_back(Utils::QUAL_MEDIUMP);

	m_test_cases[4].push_back(Utils::QUAL_LOWP);
	m_test_cases[4].push_back(Utils::QUAL_PRECISE);
	m_test_cases[4].push_back(Utils::QUAL_INOUT);

	m_test_cases[5].push_back(Utils::QUAL_INOUT);
	m_test_cases[5].push_back(Utils::QUAL_PRECISE);
	m_test_cases[5].push_back(Utils::QUAL_LOWP);

	return true;
}

/** Returns reference to current test case
 *
 * @return Reference to testCase
 **/
const Utils::qualifierSet& QualifierOrderFunctionInoutTest::getCurrentTestCase()
{
	if ((glw::GLuint)-1 == m_current_test_case_index)
	{
		return m_test_cases[0];
	}
	else
	{
		return m_test_cases[m_current_test_case_index];
	}
}

/** Constructor
 *
 * @param context Test context
 **/
QualifierOrderFunctionInputTest::QualifierOrderFunctionInputTest(deqp::Context& context)
	: GLSLTestBase(context, "qualifier_order_function_input", "Verify order of qualifiers of function input parameters")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool QualifierOrderFunctionInputTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		/* Nothing to be done here */;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	const Utils::qualifierSet& set = getCurrentTestCase();

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	for (GLuint i = 0; i < set.size(); ++i)
	{
		message << Utils::getQualifierString(set[i]) << " ";
	}

	message << tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void QualifierOrderFunctionInputTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														  Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet =
		"    vec4 temp = function(VARIABLE_NAME);\n"
		"\n"
		"    vec4 diff = temp - vec4(0, 0, 1, 1);\n"
		"    if (false == all(lessThan(diff, vec4(0.001, 0.001, 0.001, 0.001))))\n"
		"    {\n"
		"        result = VARIABLE_NAME;\n"
		"    }\n";

	static const GLchar* function_declaration = "vec4 function(QUALIFIERS_LIST vec4 param)\n"
												"{\n"
												"    return param.wzyx;\n"
												"}\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"uniform vec4 VARIABLE_NAME;\n"
													"\n"
													"FUNCTION_DECLARATION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"uniform vec4 VARIABLE_NAME;\n"
													"\n"
													"FUNCTION_DECLARATION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"uniform vec4 VARIABLE_NAME;\n"
		"\n"
		"FUNCTION_DECLARATION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "uniform vec4 VARIABLE_NAME;\n"
													 "\n"
													 "FUNCTION_DECLARATION\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "uniform vec4 VARIABLE_NAME;\n"
												  "\n"
												  "FUNCTION_DECLARATION\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		return;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	const std::string& uni_reference  = Utils::getVariableName(in_stage, Utils::UNIFORM, "test");
	const std::string& qualifier_list = Utils::getQualifiersListString(getCurrentTestCase());

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("FUNCTION_DECLARATION", position, function_declaration, out_source.m_parts[0].m_code);

	position -= strlen(function_declaration);

	Utils::replaceToken("QUALIFIERS_LIST", position, qualifier_list.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("VARIABLE_NAME", uni_reference.c_str(), out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void QualifierOrderFunctionInputTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat float_data[4] = { 1.0f, 1.0f, 0.0f, 0.0f };

	program.uniform("uni_fs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_gs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_tcs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_tes_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_vs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
}

/** Prepare test cases
 *
 * @return false if ARB_explicit_uniform_location is not supported, true otherwise
 **/
bool QualifierOrderFunctionInputTest::testInit()
{
	m_test_cases.resize(6);

	m_test_cases[0].push_back(Utils::QUAL_CONST);
	m_test_cases[0].push_back(Utils::QUAL_HIGHP);
	m_test_cases[0].push_back(Utils::QUAL_PRECISE);
	m_test_cases[0].push_back(Utils::QUAL_IN);

	m_test_cases[1].push_back(Utils::QUAL_IN);
	m_test_cases[1].push_back(Utils::QUAL_CONST);
	m_test_cases[1].push_back(Utils::QUAL_PRECISE);
	m_test_cases[1].push_back(Utils::QUAL_HIGHP);

	m_test_cases[2].push_back(Utils::QUAL_PRECISE);
	m_test_cases[2].push_back(Utils::QUAL_MEDIUMP);
	m_test_cases[2].push_back(Utils::QUAL_CONST);
	m_test_cases[2].push_back(Utils::QUAL_IN);

	m_test_cases[3].push_back(Utils::QUAL_IN);
	m_test_cases[3].push_back(Utils::QUAL_PRECISE);
	m_test_cases[3].push_back(Utils::QUAL_MEDIUMP);
	m_test_cases[3].push_back(Utils::QUAL_CONST);

	m_test_cases[4].push_back(Utils::QUAL_LOWP);
	m_test_cases[4].push_back(Utils::QUAL_CONST);
	m_test_cases[4].push_back(Utils::QUAL_IN);
	m_test_cases[4].push_back(Utils::QUAL_PRECISE);

	m_test_cases[5].push_back(Utils::QUAL_IN);
	m_test_cases[5].push_back(Utils::QUAL_PRECISE);
	m_test_cases[5].push_back(Utils::QUAL_CONST);
	m_test_cases[5].push_back(Utils::QUAL_LOWP);

	return true;
}

/** Returns reference to current test case
 *
 * @return Reference to testCase
 **/
const Utils::qualifierSet& QualifierOrderFunctionInputTest::getCurrentTestCase()
{
	if ((glw::GLuint)-1 == m_current_test_case_index)
	{
		return m_test_cases[0];
	}
	else
	{
		return m_test_cases[m_current_test_case_index];
	}
}

/** Constructor
 *
 * @param context Test context
 **/
QualifierOrderFunctionOutputTest::QualifierOrderFunctionOutputTest(deqp::Context& context)
	: GLSLTestBase(context, "qualifier_order_function_output",
				   "Verify order of qualifiers of output function parameters")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool QualifierOrderFunctionOutputTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		/* Nothing to be done here */;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	const Utils::qualifierSet& set = getCurrentTestCase();

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	for (GLuint i = 0; i < set.size(); ++i)
	{
		message << Utils::getQualifierString(set[i]) << " ";
	}

	message << tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void QualifierOrderFunctionOutputTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														   Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet =
		"    vec4 temp;\n"
		"\n"
		"    function(temp);\n"
		"\n"
		"    vec4 diff = temp - vec4(0, 0, 1, 1);\n"
		"    if (false == all(lessThan(diff, vec4(0.001, 0.001, 0.001, 0.001))))\n"
		"    {\n"
		"        result = VARIABLE_NAME;\n"
		"    }\n";

	static const GLchar* function_declaration = "void function(QUALIFIERS_LIST vec4 param)\n"
												"{\n"
												"    param = VARIABLE_NAME.wzyx;\n"
												"}\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"uniform vec4 VARIABLE_NAME;\n"
													"\n"
													"FUNCTION_DECLARATION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"uniform vec4 VARIABLE_NAME;\n"
													"\n"
													"FUNCTION_DECLARATION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"uniform vec4 VARIABLE_NAME;\n"
		"\n"
		"FUNCTION_DECLARATION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "uniform vec4 VARIABLE_NAME;\n"
													 "\n"
													 "FUNCTION_DECLARATION\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "uniform vec4 VARIABLE_NAME;\n"
												  "\n"
												  "FUNCTION_DECLARATION\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		return;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	const std::string& uni_reference  = Utils::getVariableName(in_stage, Utils::UNIFORM, "test");
	const std::string& qualifier_list = Utils::getQualifiersListString(getCurrentTestCase());

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("FUNCTION_DECLARATION", position, function_declaration, out_source.m_parts[0].m_code);

	position -= strlen(function_declaration);

	Utils::replaceToken("QUALIFIERS_LIST", position, qualifier_list.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("VARIABLE_NAME", uni_reference.c_str(), out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void QualifierOrderFunctionOutputTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat float_data[4] = { 1.0f, 1.0f, 0.0f, 0.0f };

	program.uniform("uni_fs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_gs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_tcs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_tes_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
	program.uniform("uni_vs_test", Utils::FLOAT, 1 /* n_cols */, 4 /* n_rows */, float_data);
}

/** Prepare test cases
 *
 * @return false if ARB_explicit_uniform_location is not supported, true otherwise
 **/
bool QualifierOrderFunctionOutputTest::testInit()
{
	m_test_cases.resize(6);

	m_test_cases[0].push_back(Utils::QUAL_HIGHP);
	m_test_cases[0].push_back(Utils::QUAL_PRECISE);
	m_test_cases[0].push_back(Utils::QUAL_OUT);

	m_test_cases[1].push_back(Utils::QUAL_OUT);
	m_test_cases[1].push_back(Utils::QUAL_PRECISE);
	m_test_cases[1].push_back(Utils::QUAL_HIGHP);

	m_test_cases[2].push_back(Utils::QUAL_PRECISE);
	m_test_cases[2].push_back(Utils::QUAL_MEDIUMP);
	m_test_cases[2].push_back(Utils::QUAL_OUT);

	m_test_cases[3].push_back(Utils::QUAL_OUT);
	m_test_cases[3].push_back(Utils::QUAL_PRECISE);
	m_test_cases[3].push_back(Utils::QUAL_MEDIUMP);

	m_test_cases[4].push_back(Utils::QUAL_LOWP);
	m_test_cases[4].push_back(Utils::QUAL_OUT);
	m_test_cases[4].push_back(Utils::QUAL_PRECISE);

	m_test_cases[5].push_back(Utils::QUAL_OUT);
	m_test_cases[5].push_back(Utils::QUAL_PRECISE);
	m_test_cases[5].push_back(Utils::QUAL_LOWP);

	return true;
}

/** Returns reference to current test case
 *
 * @return Reference to testCase
 **/
const Utils::qualifierSet& QualifierOrderFunctionOutputTest::getCurrentTestCase()
{
	if ((glw::GLuint)-1 == m_current_test_case_index)
	{
		return m_test_cases[0];
	}
	else
	{
		return m_test_cases[m_current_test_case_index];
	}
}

/** Constructor
 *
 * @param context Test context
 **/
QualifierOverrideLayoutTest::QualifierOverrideLayoutTest(deqp::Context& context)
	: GLSLTestBase(context, "qualifier_override_layout", "Verifies overriding layout qualifiers")
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void QualifierOverrideLayoutTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													  Utils::shaderSource& out_source)
{
	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  layout(location = 3) layout(location = 2) vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"    if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template =
		"VERSION\n"
		"\n"
		"layout(points)                                                    in;\n"
		"layout(triangle_strip, max_vertices = 2) layout(max_vertices = 4) out;\n"
		"\n"
		"in  layout(location = 3) layout(location = 2) vec4 tes_gs_result[];\n"
		"out layout(location = 3) layout(location = 2) vec4 gs_fs_result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"    if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    gs_fs_result = result;\n"
		"    gl_Position  = vec4(-1, -1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_result = result;\n"
		"    gl_Position  = vec4(-1, 1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_result = result;\n"
		"    gl_Position  = vec4(1, -1, 0, 1);\n"
		"    EmitVertex();\n"
		"    gs_fs_result = result;\n"
		"    gl_Position  = vec4(1, 1, 0, 1);\n"
		"    EmitVertex();\n"
		"}\n"
		"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 4) layout(vertices = 1) out;\n"
		"\n"
		"in  layout(location = 3) layout(location = 2) vec4 vs_tcs_result[];\n"
		"out layout(location = 3) layout(location = 2) vec4 tcs_tes_result[];\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"    if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template =
		"VERSION\n"
		"\n"
		"layout(isolines, point_mode) in;\n"
		"\n"
		"in  layout(location = 3) layout(location = 2) vec4 tcs_tes_result[];\n"
		"out layout(location = 3) layout(location = 2) vec4 tes_gs_result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"    if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tes_gs_result = result;\n"
		"}\n"
		"\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "in  layout(location = 3) layout(location = 2) vec4 in_vs_test;\n"
												  "out layout(location = 3) layout(location = 2) vec4 vs_tcs_result;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = in_vs_test;\n"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		return;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);
}

/**Prepare vertex buffer and vertex array object.
 *
 * @param program Program instance
 * @param buffer  Buffer instance
 * @param vao     VertexArray instance
 *
 * @return 0
 **/
void QualifierOverrideLayoutTest::prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer,
													  Utils::vertexArray& vao)
{
	static const GLint expected_location = 2;

	std::string test_name = Utils::getVariableName(Utils::VERTEX_SHADER, Utils::INPUT, "test");
	GLint		test_loc  = program.getAttribLocation(test_name.c_str());

	if (expected_location != test_loc)
	{
		TCU_FAIL("Vertex attribute location is invalid");
	}

	vao.generate();
	vao.bind();

	buffer.generate(GL_ARRAY_BUFFER);

	GLfloat	data[]	= { 0.0f, 1.0f, 0.0f, 1.0f };
	GLsizeiptr data_size = sizeof(data);

	buffer.update(data_size, data, GL_STATIC_DRAW);

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up vao */
	gl.vertexAttribPointer(test_loc, 4 /* size */, GL_FLOAT /* type */, GL_FALSE /* normalized*/, 0 /* stride */,
						   0 /* offset */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribPointer");

	/* Enable attribute */
	gl.enableVertexAttribArray(test_loc);
	GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");
}

/** Constructor
 *
 * @param context Test context
 **/
BindingUniformBlocksTest::BindingUniformBlocksTest(deqp::Context& context)
	: GLSLTestBase(context, "binding_uniform_blocks", "Test verifies uniform block binding")
	, m_goku_buffer(context)
	, m_vegeta_buffer(context)
	, m_children_buffer(context)
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingUniformBlocksTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												   Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(std140, binding = 0) uniform GOKU {\n"
									"    vec4 chichi;\n"
									"} goku;\n";

	static const GLchar* uni_vegeta = "layout(std140, binding = 1) uniform VEGETA {\n"
									  "    vec3 bulma;\n"
									  "} vegeta;\n";

	static const GLchar* uni_children = "layout(std140, binding = 3) uniform CHILDREN {\n"
										"    vec4 gohan;\n"
										"    vec4 trunks;\n"
										"} children;\n\n";

	static const GLchar* verification_snippet = "    if ((vec4(1, 0, 0, 0) != goku.chichi)    ||\n"
												"        (vec3(0, 1, 0)    != vegeta.bulma)   ||\n"
												"        (vec4(0, 0, 1, 0) != children.gohan) ||\n"
												"        (vec4(0, 0, 0, 1) != children.trunks) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"UNI_VEGETA\n"
		"UNI_CHILDREN\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"UNI_VEGETA\n"
													"UNI_CHILDREN\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_CHILDREN\n"
													"UNI_GOKU\n"
													"UNI_VEGETA\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_VEGETA\n"
		"UNI_CHILDREN\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "UNI_CHILDREN\n"
													 "UNI_VEGETA\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_CHILDREN\n"
												  "UNI_VEGETA\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_VEGETA", uni_vegeta, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_CHILDREN", uni_children, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingUniformBlocksTest::prepareUniforms(Utils::program& program)
{
	(void)program;
	static const GLfloat goku_data[4]	 = { 1.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat vegeta_data[3]   = { 0.0f, 1.0f, 0.0f };
	static const GLfloat children_data[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	m_goku_buffer.generate(GL_UNIFORM_BUFFER);
	m_vegeta_buffer.generate(GL_UNIFORM_BUFFER);
	m_children_buffer.generate(GL_UNIFORM_BUFFER);

	m_goku_buffer.update(sizeof(goku_data), (GLvoid*)goku_data, GL_STATIC_DRAW);
	m_vegeta_buffer.update(sizeof(vegeta_data), (GLvoid*)vegeta_data, GL_STATIC_DRAW);
	m_children_buffer.update(sizeof(children_data), (GLvoid*)children_data, GL_STATIC_DRAW);

	m_goku_buffer.bindRange(0 /* index */, 0 /* offset */, sizeof(goku_data));
	m_vegeta_buffer.bindRange(1 /* index */, 0 /* offset */, sizeof(vegeta_data));
	m_children_buffer.bindRange(3 /* index */, 0 /* offset */, sizeof(children_data));
}

/** Overwrite of releaseResource method, release extra uniform buffer
 *
 * @param ignored
 **/
void BindingUniformBlocksTest::releaseResource()
{
	m_goku_buffer.release();
	m_vegeta_buffer.release();
	m_children_buffer.release();
}

/** Constructor
 *
 * @param context Test context
 **/
BindingUniformSingleBlockTest::BindingUniformSingleBlockTest(deqp::Context& context)
	: GLSLTestBase(context, "binding_uniform_single_block", "Test verifies uniform block binding")
	, m_goku_buffer(context)
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BindingUniformSingleBlockTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
	case 0:
		m_test_stage = Utils::VERTEX_SHADER;
		break;
	case 1:
		m_test_stage = Utils::TESS_CTRL_SHADER;
		break;
	case 2:
		m_test_stage = Utils::TESS_EVAL_SHADER;
		break;
	case 3:
		m_test_stage = Utils::GEOMETRY_SHADER;
		break;
	case 4:
		m_test_stage = Utils::FRAGMENT_SHADER;
		break;
	default:
		return false;
		break;
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Tested stage: "
										<< Utils::getShaderStageName((Utils::SHADER_STAGES)m_test_stage)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingUniformSingleBlockTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku_with_binding = "layout(std140, binding = 0) uniform GOKU {\n"
												 "    vec4 gohan;\n"
												 "    vec4 goten;\n"
												 "} goku;\n";

	static const GLchar* uni_goku_no_binding = "layout(std140) uniform GOKU {\n"
											   "    vec4 gohan;\n"
											   "    vec4 goten;\n"
											   "} goku;\n";

	static const GLchar* verification_snippet = "    if ((vec4(1, 0, 0, 0) != goku.gohan) ||\n"
												"        (vec4(0, 1, 0, 0) != goku.goten) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template	= 0;
	const GLchar* uniform_definition = uni_goku_no_binding;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template	= compute_shader_template;
		uniform_definition = uni_goku_with_binding;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	if (in_stage == m_test_stage)
	{
		uniform_definition = uni_goku_with_binding;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uniform_definition, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingUniformSingleBlockTest::prepareUniforms(Utils::program& program)
{
	(void)program;
	static const GLfloat goku_data[8] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };

	m_goku_buffer.generate(GL_UNIFORM_BUFFER);

	m_goku_buffer.update(sizeof(goku_data), (GLvoid*)goku_data, GL_STATIC_DRAW);

	m_goku_buffer.bindRange(0 /* index */, 0 /* offset */, sizeof(goku_data));
}

/** Overwrite of releaseResource method, release extra uniform buffer
 *
 * @param ignored
 **/
void BindingUniformSingleBlockTest::releaseResource()
{
	m_goku_buffer.release();
}

/** Constructor
 *
 * @param context Test context
 **/
BindingUniformBlockArrayTest::BindingUniformBlockArrayTest(deqp::Context& context)
	: GLSLTestBase(context, "binding_uniform_block_array", "Test verifies binding of uniform block arrays")
	, m_goku_00_buffer(context)
	, m_goku_01_buffer(context)
	, m_goku_02_buffer(context)
	, m_goku_03_buffer(context)
	, m_goku_04_buffer(context)
	, m_goku_05_buffer(context)
	, m_goku_06_buffer(context)
	, m_goku_07_buffer(context)
	, m_goku_08_buffer(context)
	, m_goku_09_buffer(context)
	, m_goku_10_buffer(context)
	, m_goku_11_buffer(context)
	, m_goku_12_buffer(context)
	, m_goku_13_buffer(context)
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingUniformBlockArrayTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													   Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(std140, binding = 2) uniform GOKU {\n"
									"    vec4 gohan;\n"
									"    vec4 goten;\n"
									"} goku[14];\n";

	static const GLchar* verification_snippet = "    if ((vec4(0, 0, 0, 0) != goku[0].gohan)  ||\n"
												"        (vec4(0, 0, 0, 1) != goku[0].goten)  ||\n"
												"        (vec4(0, 0, 1, 0) != goku[1].gohan)  ||\n"
												"        (vec4(0, 0, 1, 1) != goku[1].goten)  ||\n"
												"        (vec4(0, 1, 0, 0) != goku[2].gohan)  ||\n"
												"        (vec4(0, 1, 0, 1) != goku[2].goten)  ||\n"
												"        (vec4(0, 1, 1, 0) != goku[3].gohan)  ||\n"
												"        (vec4(0, 1, 1, 1) != goku[3].goten)  ||\n"
												"        (vec4(1, 0, 0, 0) != goku[4].gohan)  ||\n"
												"        (vec4(1, 0, 0, 1) != goku[4].goten)  ||\n"
												"        (vec4(1, 0, 1, 0) != goku[5].gohan)  ||\n"
												"        (vec4(1, 0, 1, 1) != goku[5].goten)  ||\n"
												"        (vec4(1, 1, 0, 0) != goku[6].gohan)  ||\n"
												"        (vec4(1, 1, 0, 1) != goku[6].goten)  ||\n"
												"        (vec4(1, 1, 1, 0) != goku[7].gohan)  ||\n"
												"        (vec4(1, 1, 1, 1) != goku[7].goten)  ||\n"
												"        (vec4(0, 0, 0, 0) != goku[8].gohan)  ||\n"
												"        (vec4(0, 0, 0, 1) != goku[8].goten)  ||\n"
												"        (vec4(0, 0, 1, 0) != goku[9].gohan)  ||\n"
												"        (vec4(0, 0, 1, 1) != goku[9].goten)  ||\n"
												"        (vec4(0, 1, 0, 0) != goku[10].gohan) ||\n"
												"        (vec4(0, 1, 0, 1) != goku[10].goten) ||\n"
												"        (vec4(0, 1, 1, 0) != goku[11].gohan) ||\n"
												"        (vec4(0, 1, 1, 1) != goku[11].goten) ||\n"
												"        (vec4(1, 0, 0, 0) != goku[12].gohan) ||\n"
												"        (vec4(1, 0, 0, 1) != goku[12].goten) ||\n"
												"        (vec4(1, 0, 1, 0) != goku[13].gohan) ||\n"
												"        (vec4(1, 0, 1, 1) != goku[13].goten) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingUniformBlockArrayTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat goku_data[][8] = {
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f }
	};

	Utils::buffer* buffers[14] = { &m_goku_00_buffer, &m_goku_01_buffer, &m_goku_02_buffer, &m_goku_03_buffer,
								   &m_goku_04_buffer, &m_goku_05_buffer, &m_goku_06_buffer, &m_goku_07_buffer,
								   &m_goku_08_buffer, &m_goku_09_buffer, &m_goku_10_buffer, &m_goku_11_buffer,
								   &m_goku_12_buffer, &m_goku_13_buffer };

	for (GLuint i = 0; i < 14; ++i)
	{
		checkBinding(program, i, i + 2);

		buffers[i]->generate(GL_UNIFORM_BUFFER);
		buffers[i]->update(sizeof(GLfloat) * 8, (GLvoid*)goku_data[i], GL_STATIC_DRAW);
		buffers[i]->bindRange(i + 2 /* index */, 0 /* offset */, sizeof(GLfloat) * 8);
	}
}

/** Overwrite of releaseResource method, release extra uniform buffer
 *
 * @param ignored
 **/
void BindingUniformBlockArrayTest::releaseResource()
{
	Utils::buffer* buffers[14] = { &m_goku_00_buffer, &m_goku_01_buffer, &m_goku_02_buffer, &m_goku_03_buffer,
								   &m_goku_04_buffer, &m_goku_05_buffer, &m_goku_06_buffer, &m_goku_07_buffer,
								   &m_goku_08_buffer, &m_goku_09_buffer, &m_goku_10_buffer, &m_goku_11_buffer,
								   &m_goku_12_buffer, &m_goku_13_buffer };

	for (GLuint i = 0; i < 14; ++i)
	{
		buffers[i]->release();
	}
}

/** Verifies that API reports correct uniform binding
 *
 * @param program          Program
 * @param index            Index of array element
 * @param expected_binding Expected binding
 **/
void BindingUniformBlockArrayTest::checkBinding(Utils::program& program, glw::GLuint index, glw::GLint expected_binding)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLchar buffer[64];
	sprintf(buffer, "GOKU[%d]", index);

	const GLuint uniform_index = gl.getUniformBlockIndex(program.m_program_object_id, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformBlockIndex");
	if (GL_INVALID_INDEX == uniform_index)
	{
		TCU_FAIL("Uniform block is inactive");
	}

	GLint binding = -1;

	gl.getActiveUniformBlockiv(program.m_program_object_id, uniform_index, GL_UNIFORM_BLOCK_BINDING, &binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformBlockiv");

	if (expected_binding != binding)
	{
		TCU_FAIL("Wrong binding reported by API");
	}
}

/** Constructor
 *
 * @param context Test context
 **/
BindingUniformDefaultTest::BindingUniformDefaultTest(deqp::Context& context)
	: APITestBase(context, "binding_uniform_default", "Test verifies default binding of uniform block")
{
	/* Nothing to be done here */
}

/** Execute API call and verifies results
 *
 * @return true when results are positive, false otherwise
 **/
bool BindingUniformDefaultTest::checkResults(Utils::program& program)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const GLuint index = gl.getUniformBlockIndex(program.m_program_object_id, "GOKU");
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformBlockIndex");
	if (GL_INVALID_INDEX == index)
	{
		TCU_FAIL("Uniform block is inactive");
		return false;
	}

	GLint binding = -1;

	gl.getActiveUniformBlockiv(program.m_program_object_id, index, GL_UNIFORM_BLOCK_BINDING, &binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformBlockiv");

	if (0 != binding)
	{
		return false;
	}

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingUniformDefaultTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(std140) uniform GOKU {\n"
									"    vec4 gohan;\n"
									"    vec4 goten;\n"
									"} goku;\n";

	static const GLchar* verification_snippet = "    if ((vec4(0, 0, 0, 0) != goku.gohan) ||\n"
												"        (vec4(0, 0, 0, 1) != goku.goten) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
BindingUniformAPIOverirdeTest::BindingUniformAPIOverirdeTest(deqp::Context& context)
	: GLSLTestBase(context, "binding_uniform_api_overirde", "Test verifies if binding can be overriden with API")
	, m_goku_buffer(context)
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingUniformAPIOverirdeTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(std140, binding = 2) uniform GOKU {\n"
									"    vec4 gohan;\n"
									"    vec4 goten;\n"
									"} goku;\n";

	static const GLchar* verification_snippet = "    if ((vec4(1, 0, 0, 0) != goku.gohan)  ||\n"
												"        (vec4(0, 1, 0, 0) != goku.goten)  )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingUniformAPIOverirdeTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat goku_data[8] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };

	static const GLuint new_binding = 11;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const GLuint index = gl.getUniformBlockIndex(program.m_program_object_id, "GOKU");
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformBlockIndex");
	if (GL_INVALID_INDEX == index)
	{
		TCU_FAIL("Uniform block is inactive");
		return;
	}

	gl.uniformBlockBinding(program.m_program_object_id, index, new_binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformBlockBinding");

	GLint binding = -1;

	gl.getActiveUniformBlockiv(program.m_program_object_id, index, GL_UNIFORM_BLOCK_BINDING, &binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveUniformBlockiv");

	if (new_binding != binding)
	{
		TCU_FAIL("GetActiveUniformBlockiv returned wrong binding");
		return;
	}

	m_goku_buffer.generate(GL_UNIFORM_BUFFER);
	m_goku_buffer.update(sizeof(GLfloat) * 8, (GLvoid*)goku_data, GL_STATIC_DRAW);
	m_goku_buffer.bindRange(new_binding /* index */, 0 /* offset */, sizeof(GLfloat) * 8);
}

/** Overwrite of releaseResource method, release extra uniform buffer
 *
 * @param ignored
 **/
void BindingUniformAPIOverirdeTest::releaseResource()
{
	m_goku_buffer.release();
}

/** Constructor
 *
 * @param context Test context
 **/
BindingUniformGlobalBlockTest::BindingUniformGlobalBlockTest(deqp::Context& context)
	: NegativeTestBase(context, "binding_uniform_global_block",
					   "Test verifies that global uniform cannot be qualified with binding")
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingUniformGlobalBlockTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet = "    if (vec4(0, 0, 1, 1) != uni_test)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* uniform_definition = "layout(binding = 0) uniform vec4 uni_test;\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNIFORM_DEFINITION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNIFORM_DEFINITION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNIFORM_DEFINITION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNIFORM_DEFINITION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNIFORM_DEFINITION\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNIFORM_DEFINITION\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("UNIFORM_DEFINITION", position, uniform_definition, out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
BindingUniformInvalidTest::BindingUniformInvalidTest(deqp::Context& context)
	: NegativeTestBase(context, "binding_uniform_invalid", "Test verifies invalid binding values")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BindingUniformInvalidTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
		m_case = TEST_CASES_MAX;
		break;
	case NEGATIVE_VALUE:
	case VARIABLE_NAME:
	case STD140:
	case MISSING:
		m_case = (TESTCASES)test_case_index;
		break;
	default:
		return false;
	}

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingUniformInvalidTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet = "    if (vec4(0, 0, 1, 1) != goku.gohan)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* uniform_definition = "layout(std140, binding BINDING) uniform GOKU {\n"
											  "    vec4 gohan;\n"
											  "    vec4 goten;\n"
											  "} goku;\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNIFORM_DEFINITION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNIFORM_DEFINITION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNIFORM_DEFINITION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNIFORM_DEFINITION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNIFORM_DEFINITION\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNIFORM_DEFINITION\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("UNIFORM_DEFINITION", position, uniform_definition, out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("BINDING", getCaseString(m_case), out_source.m_parts[0].m_code);
}

const GLchar* BindingUniformInvalidTest::getCaseString(TESTCASES test_case)
{
	(void)test_case;
	const GLchar* binding = 0;

	switch (m_case)
	{
	case NEGATIVE_VALUE:
		binding = "= -1";
		break;
	case VARIABLE_NAME:
		binding = "= goku";
		break;
	case STD140:
		binding = "= std140";
		break;
	case MISSING:
		binding = "";
		break;
	case TEST_CASES_MAX:
		binding = "= 0";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return binding;
}

/** Constructor
 *
 * @param context Test context
 **/
BindingSamplersTest::BindingSamplersTest(deqp::Context& context)
	: GLSLTestBase(context, "binding_samplers", "Test verifies smaplers binding")
	, m_goku_texture(context)
	, m_vegeta_texture(context)
	, m_trunks_texture(context)
	, m_buffer(context)
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BindingSamplersTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
	case 0:
		m_test_case = Utils::TEX_2D;
		break;
	case 1:
		m_test_case = Utils::TEX_BUFFER;
		break;
	case 2:
		m_test_case = Utils::TEX_2D_RECT;
		break;
	case 3:
		m_test_case = Utils::TEX_2D_ARRAY;
		break;
	case 4:
		m_test_case = Utils::TEX_3D;
		break;
	case 5:
		m_test_case = Utils::TEX_CUBE;
		break;
	case 6:
		m_test_case = Utils::TEX_1D;
		break;
	case 7:
		m_test_case = Utils::TEX_1D_ARRAY;
		break;
	default:
		return false;
		break;
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message
										<< "Tested texture type: " << Utils::getTextureTypeName(m_test_case)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingSamplersTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											  Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(binding = 0) uniform TYPE goku;\n";

	static const GLchar* uni_vegeta = "layout(binding = 1) uniform TYPE vegeta;\n";

	static const GLchar* uni_trunks = "layout(binding = 3) uniform TYPE trunks;\n\n";

	static const GLchar* verification_snippet = "    TEX_COORD_TYPE tex_coord = TEX_COORD_TYPE(COORDINATES);\n"
												"    vec4 goku_color   = SAMPLING_FUNCTION(goku,   tex_coord);\n"
												"    vec4 vegeta_color = SAMPLING_FUNCTION(vegeta, tex_coord);\n"
												"    vec4 trunks_color = SAMPLING_FUNCTION(trunks, tex_coord);\n"
												"\n"
												"    if ((vec4(1, 0, 0, 0) != goku_color)   ||\n"
												"        (vec4(0, 1, 0, 0) != vegeta_color) ||\n"
												"        (vec4(0, 0, 1, 0) != trunks_color)  )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"UNI_VEGETA\n"
		"UNI_TRUNKS\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"UNI_VEGETA\n"
													"UNI_TRUNKS\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_TRUNKS\n"
													"UNI_GOKU\n"
													"UNI_VEGETA\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_VEGETA\n"
		"UNI_TRUNKS\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "UNI_TRUNKS\n"
													 "UNI_VEGETA\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_TRUNKS\n"
												  "UNI_VEGETA\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const Utils::TYPES base_tex_coord_type = (Utils::TEX_BUFFER == m_test_case) ? Utils::INT : Utils::FLOAT;
	const GLchar*	  coordinates		   = 0;
	GLuint			   n_coordinates	   = Utils::getNumberOfCoordinates(m_test_case);
	const GLchar*	  shader_template	 = 0;
	const GLchar*	  sampler_type		   = Utils::getSamplerType(m_test_case);
	const GLchar*	  sampling_function   = (Utils::TEX_BUFFER == m_test_case) ? "texelFetch" : "texture";
	const GLchar*	  tex_coord_type	  = Utils::getTypeName(base_tex_coord_type, 1 /* n_columns */, n_coordinates);

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	switch (n_coordinates)
	{
	case 1:
		coordinates = "0";
		break;
	case 2:
		coordinates = "0, 0";
		break;
	case 3:
		coordinates = "0, 0, 0";
		break;
	case 4:
		coordinates = "0, 0, 0, 0";
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	position -= strlen(verification_snippet);

	Utils::replaceToken("COORDINATES", position, coordinates, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("SAMPLING_FUNCTION", sampling_function, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_VEGETA", uni_vegeta, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_TRUNKS", uni_trunks, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("TEX_COORD_TYPE", tex_coord_type, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("TYPE", sampler_type, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingSamplersTest::prepareUniforms(Utils::program& program)
{
	(void)program;
	static const GLuint goku_data   = 0x000000ff;
	static const GLuint vegeta_data = 0x0000ff00;
	static const GLuint trunks_data = 0x00ff0000;

	prepareTexture(m_goku_texture, m_test_case, goku_data);
	prepareTexture(m_vegeta_texture, m_test_case, vegeta_data);
	prepareTexture(m_trunks_texture, m_test_case, trunks_data);

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

	m_goku_texture.bind();

	gl.activeTexture(GL_TEXTURE1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

	m_vegeta_texture.bind();

	gl.activeTexture(GL_TEXTURE3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

	m_trunks_texture.bind();
}

/** Overwrite of releaseResource method, release extra buffer and texture
 *
 * @param ignored
 **/
void BindingSamplersTest::releaseResource()
{
	m_goku_texture.release();
	m_vegeta_texture.release();
	m_trunks_texture.release();
	m_buffer.release();
}

/** Prepare texture of given type filled with given color
 *
 * @param texture      Texture
 * @param texture_type Type of texture
 * @param color        Color
 **/
void BindingSamplersTest::prepareTexture(Utils::texture& texture, Utils::TEXTURE_TYPES texture_type, glw::GLuint color)
{
	(void)texture_type;
	static const GLuint width  = 16;
	static const GLuint height = 16;
	static const GLuint depth  = 1;

	std::vector<GLuint> texture_data;
	texture_data.resize(width * height);

	for (GLuint i = 0; i < texture_data.size(); ++i)
	{
		texture_data[i] = color;
	}

	if (Utils::TEX_BUFFER != m_test_case)
	{
		texture.create(width, height, depth, GL_RGBA8, m_test_case);

		texture.update(width, height, depth, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);
	}
	else
	{
		m_buffer.generate(GL_TEXTURE_BUFFER);
		m_buffer.update(texture_data.size(), &texture_data[0], GL_STATIC_DRAW);

		texture.createBuffer(GL_RGBA8, m_buffer.m_id);
	}
}

/** Constructor
 *
 * @param context Test context
 **/
BindingSamplerSingleTest::BindingSamplerSingleTest(deqp::Context& context)
	: GLSLTestBase(context, "binding_sampler_single", "Test verifies sampler binding"), m_goku_texture(context)
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BindingSamplerSingleTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
	case 0:
		m_test_stage = Utils::VERTEX_SHADER;
		break;
	case 1:
		m_test_stage = Utils::TESS_CTRL_SHADER;
		break;
	case 2:
		m_test_stage = Utils::TESS_EVAL_SHADER;
		break;
	case 3:
		m_test_stage = Utils::GEOMETRY_SHADER;
		break;
	case 4:
		m_test_stage = Utils::FRAGMENT_SHADER;
		break;
	default:
		return false;
		break;
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Tested stage: "
										<< Utils::getShaderStageName((Utils::SHADER_STAGES)m_test_stage)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingSamplerSingleTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												   Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku_with_binding = "layout(binding = 2) uniform sampler2D goku;\n";

	static const GLchar* uni_goku_no_binding = "uniform sampler2D goku;\n";

	static const GLchar* verification_snippet = "    vec4 goku_color = texture(goku, vec2(0,0));\n"
												"\n"
												"    if (vec4(1, 0, 0, 0) != goku_color)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template	= 0;
	const GLchar* uniform_definition = uni_goku_no_binding;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template	= compute_shader_template;
		uniform_definition = uni_goku_with_binding;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	if (in_stage == m_test_stage)
	{
		uniform_definition = uni_goku_with_binding;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uniform_definition, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingSamplerSingleTest::prepareUniforms(Utils::program& program)
{
	(void)program;
	static const GLuint goku_data = 0x000000ff;

	m_goku_texture.create(16, 16, GL_RGBA8);

	std::vector<GLuint> texture_data;
	texture_data.resize(16 * 16);

	for (GLuint i = 0; i < texture_data.size(); ++i)
	{
		texture_data[i] = goku_data;
	}

	m_goku_texture.update(16, 16, 0 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.activeTexture(GL_TEXTURE2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

	m_goku_texture.bind();
}

/** Overwrite of releaseResource method, release extra texture
 *
 * @param ignored
 **/
void BindingSamplerSingleTest::releaseResource()
{
	m_goku_texture.release();
}

/** Constructor
 *
 * @param context Test context
 **/
BindingSamplerArrayTest::BindingSamplerArrayTest(deqp::Context& context)
	: GLSLTestBase(context, "binding_sampler_array", "Test verifies binding of sampler arrays")
	, m_goku_00_texture(context)
	, m_goku_01_texture(context)
	, m_goku_02_texture(context)
	, m_goku_03_texture(context)
	, m_goku_04_texture(context)
	, m_goku_05_texture(context)
	, m_goku_06_texture(context)
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingSamplerArrayTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												  Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(binding = 1) uniform sampler2D goku[7];\n";

	static const GLchar* verification_snippet = "    vec4 color[7];\n"
												"\n"
												"    for (uint i = 0u; i < 7; ++i)\n"
												"    {\n"
												"        color[i] = texture(goku[i], vec2(0, 0));\n"
												"    }\n"
												"\n"
												"    if ((vec4(0, 0, 0, 0) != color[0]) ||\n"
												"        (vec4(0, 0, 0, 1) != color[1]) ||\n"
												"        (vec4(0, 0, 1, 0) != color[2]) ||\n"
												"        (vec4(0, 0, 1, 1) != color[3]) ||\n"
												"        (vec4(0, 1, 0, 0) != color[4]) ||\n"
												"        (vec4(0, 1, 0, 1) != color[5]) ||\n"
												"        (vec4(0, 1, 1, 0) != color[6]) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingSamplerArrayTest::prepareUniforms(Utils::program& program)
{
	static const GLuint goku_data[7] = {
		0x00000000, 0xff000000, 0x00ff0000, 0xffff0000, 0x0000ff00, 0xff00ff00, 0x00ffff00,
	};

	static const GLuint binding_offset = 1;

	Utils::texture* textures[7] = {
		&m_goku_00_texture, &m_goku_01_texture, &m_goku_02_texture, &m_goku_03_texture,
		&m_goku_04_texture, &m_goku_05_texture, &m_goku_06_texture,
	};

	std::vector<GLuint> texture_data;
	texture_data.resize(16 * 16);

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (GLuint i = 0; i < 7; ++i)
	{
		GLint expected_binding = i + binding_offset;

		checkBinding(program, i, expected_binding);

		gl.activeTexture(GL_TEXTURE0 + expected_binding);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

		textures[i]->create(16, 16, GL_RGBA8);

		for (GLuint j = 0; j < texture_data.size(); ++j)
		{
			texture_data[j] = goku_data[i];
		}

		textures[i]->update(16, 16, 0 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);
	}
}

/** Overwrite of releaseResource method, release extra textures
 *
 * @param ignored
 **/
void BindingSamplerArrayTest::releaseResource()
{
	Utils::texture* textures[7] = {
		&m_goku_00_texture, &m_goku_01_texture, &m_goku_02_texture, &m_goku_03_texture,
		&m_goku_04_texture, &m_goku_05_texture, &m_goku_06_texture,
	};

	for (GLuint i = 0; i < 7; ++i)
	{
		textures[i]->release();
	}
}

/** Verifies that API reports correct uniform binding
 *
 * @param program          Program
 * @param index            Index of array element
 * @param expected_binding Expected binding
 **/
void BindingSamplerArrayTest::checkBinding(Utils::program& program, GLuint index, GLint expected_binding)
{
	if (false == Utils::checkUniformArrayBinding(program, "goku", index, expected_binding))
	{
		TCU_FAIL("Wrong binding reported by API");
	}
}

/** Constructor
 *
 * @param context Test context
 **/
BindingSamplerDefaultTest::BindingSamplerDefaultTest(deqp::Context& context)
	: APITestBase(context, "binding_sampler_default", "Test verifies default sampler binding")
{
	/* Nothing to be done here */
}

/** Execute API call and verifies results
 *
 * @return true when results are positive, false otherwise
 **/
bool BindingSamplerDefaultTest::checkResults(Utils::program& program)
{
	return Utils::checkUniformBinding(program, "goku", 0 /* expected_binding */);
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingSamplerDefaultTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "uniform sampler2D goku;\n";

	static const GLchar* verification_snippet = "    vec4 color = texture(goku, vec2(0,0));\n"
												"    if (vec4(1, 0, 0, 0) != color)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
BindingSamplerAPIOverrideTest::BindingSamplerAPIOverrideTest(deqp::Context& context)
	: GLSLTestBase(context, "binding_sampler_api_override", "Verifies that API can override sampler binding")
	, m_goku_texture(context)
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingSamplerAPIOverrideTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(binding = 2) uniform sampler2D goku;\n";

	static const GLchar* verification_snippet = "    vec4 color = texture(goku, vec2(0,0));\n"
												"    if (vec4(1, 0, 0, 0) != color)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingSamplerAPIOverrideTest::prepareUniforms(Utils::program& program)
{
	static const GLuint goku_data   = 0x000000ff;
	static const GLint  new_binding = 11;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const GLint uniform_location = program.getUniformLocation("goku");
	if (-1 == uniform_location)
	{
		TCU_FAIL("Uniform is inactive");
	}

	gl.uniform1i(uniform_location, new_binding);

	GLint binding = -1;

	gl.getUniformiv(program.m_program_object_id, uniform_location, &binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getUniformiv");

	if (new_binding != binding)
	{
		TCU_FAIL("Wrong binding value");
		return;
	}

	m_goku_texture.create(16, 16, GL_RGBA8);

	std::vector<GLuint> texture_data;
	texture_data.resize(16 * 16);

	for (GLuint i = 0; i < texture_data.size(); ++i)
	{
		texture_data[i] = goku_data;
	}

	m_goku_texture.update(16, 16, 0 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);

	gl.activeTexture(GL_TEXTURE11);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

	m_goku_texture.bind();
}

/** Overwrite of releaseResource method, release extra texture
 *
 * @param ignored
 **/
void BindingSamplerAPIOverrideTest::releaseResource()
{
	m_goku_texture.release();
}

/** Constructor
 *
 * @param context Test context
 **/
BindingSamplerInvalidTest::BindingSamplerInvalidTest(deqp::Context& context)
	: NegativeTestBase(context, "binding_sampler_invalid", "Test verifies invalid binding values")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BindingSamplerInvalidTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
		m_case = TEST_CASES_MAX;
		break;
	case NEGATIVE_VALUE:
	case VARIABLE_NAME:
	case STD140:
	case MISSING:
		m_case = (TESTCASES)test_case_index;
		break;
	default:
		return false;
	}

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingSamplerInvalidTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(binding BINDING) uniform sampler2D goku;\n";

	static const GLchar* verification_snippet = "    vec4 color = texture(goku, vec2(0,0));\n"
												"    if (vec4(1, 0, 0, 0) != color)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("BINDING", getCaseString(m_case), out_source.m_parts[0].m_code);
}

const GLchar* BindingSamplerInvalidTest::getCaseString(TESTCASES test_case)
{
	(void)test_case;
	const GLchar* binding = 0;

	switch (m_case)
	{
	case NEGATIVE_VALUE:
		binding = "= -1";
		break;
	case VARIABLE_NAME:
		binding = "= goku";
		break;
	case STD140:
		binding = "= std140";
		break;
	case MISSING:
		binding = "";
		break;
	case TEST_CASES_MAX:
		binding = "= 0";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return binding;
}

/* Constants used by BindingImagesTest */
const GLuint BindingImagesTest::m_goku_data   = 0x000000ff;
const GLuint BindingImagesTest::m_vegeta_data = 0x0000ff00;
const GLuint BindingImagesTest::m_trunks_data = 0x00ff0000;

/** Constructor
 *
 * @param context Test context
 **/
BindingImagesTest::BindingImagesTest(deqp::Context& context)
	: BindingImageTest(context, "binding_images", "Test verifies binding of images")
	, m_goku_texture(context)
	, m_vegeta_texture(context)
	, m_trunks_texture(context)
	, m_goku_buffer(context)
	, m_vegeta_buffer(context)
	, m_trunks_buffer(context)
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BindingImagesTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
	case 0:
		m_test_case = Utils::TEX_2D;
		break;
	case 1:
		m_test_case = Utils::TEX_BUFFER;
		break;
	case 2:
		m_test_case = Utils::TEX_2D_RECT;
		break;
	case 3:
		m_test_case = Utils::TEX_2D_ARRAY;
		break;
	case 4:
		m_test_case = Utils::TEX_3D;
		break;
	case 5:
		m_test_case = Utils::TEX_CUBE;
		break;
	case 6:
		m_test_case = Utils::TEX_1D;
		break;
	case 7:
		m_test_case = Utils::TEX_1D_ARRAY;
		break;
	default:
		return false;
		break;
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message
										<< "Tested texture type: " << Utils::getTextureTypeName(m_test_case)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingImagesTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(binding = 1, rgba8) uniform TYPE goku;\n";

	static const GLchar* uni_vegeta = "layout(binding = 2, rgba8) uniform TYPE vegeta;\n";

	static const GLchar* uni_trunks = "layout(binding = 4, rgba8) uniform TYPE trunks;\n\n";

	static const GLchar* verification_snippet = "    TEX_COORD_TYPE tex_coord_read  = TEX_COORD_TYPE(COORDINATES);\n"
												"    TEX_COORD_TYPE tex_coord_write = TEX_COORD_TYPE(COORDINATES);\n"
												"    vec4 goku_color   = imageLoad(goku,   tex_coord_read);\n"
												"    vec4 vegeta_color = imageLoad(vegeta, tex_coord_read);\n"
												"    vec4 trunks_color = imageLoad(trunks, tex_coord_read);\n"
												"\n"
												"    imageStore(goku,   tex_coord_write, vec4(0, 1, 0, 1));\n"
												"    imageStore(vegeta, tex_coord_write, vec4(0, 1, 0, 1));\n"
												"    imageStore(trunks, tex_coord_write, vec4(0, 1, 0, 1));\n"
												"\n"
												"    if ((vec4(1, 0, 0, 0) != goku_color)   ||\n"
												"        (vec4(0, 1, 0, 0) != vegeta_color) ||\n"
												"        (vec4(0, 0, 1, 0) != trunks_color)  )\n"
												"    {\n"
												"        result = goku_color;\n"
												"        //result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"UNI_VEGETA\n"
		"UNI_TRUNKS\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"    if(gl_GlobalInvocationID.xy == vec2(0, 0)) {\n"
		"VERIFICATION"
		"    }\n"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"UNI_VEGETA\n"
													"UNI_TRUNKS\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"#if IMAGES\n"
													"UNI_TRUNKS\n"
													"UNI_GOKU\n"
													"UNI_VEGETA\n"
													"#endif\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"#if IMAGES\n"
													"VERIFICATION else\n"
													"#endif\n"
													"    if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"#if IMAGES\n"
		"UNI_VEGETA\n"
		"UNI_TRUNKS\n"
		"UNI_GOKU\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"#if IMAGES\n"
		"VERIFICATION else\n"
		"#endif\n"
		"    if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "#extension GL_ARB_shader_image_load_store : enable\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "#if IMAGES\n"
													 "UNI_GOKU\n"
													 "UNI_TRUNKS\n"
													 "UNI_VEGETA\n"
													 "#endif\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "#if IMAGES\n"
													 "VERIFICATION else\n"
													 "#endif\n"
													 "    if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "#extension GL_ARB_shader_image_load_store : enable\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "#if IMAGES\n"
												  "UNI_TRUNKS\n"
												  "UNI_VEGETA\n"
												  "UNI_GOKU\n"
												  "#endif\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "#if IMAGES\n"
												  "VERIFICATION"
												  "#endif\n"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* coordinates_read  = 0;
	const GLchar* coordinates_write = 0;
	const GLchar* image_type		= Utils::getImageType(m_test_case);
	GLuint		  n_coordinates		= Utils::getNumberOfCoordinates(m_test_case);
	const GLchar* shader_template   = 0;
	const GLchar* tex_coord_type	= Utils::getTypeName(Utils::INT, 1 /* n_columns */, n_coordinates);

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	switch (n_coordinates)
	{
	case 1:
		coordinates_read  = "1";
		coordinates_write = "0";
		break;
	case 2:
		coordinates_read  = "1, 0";
		coordinates_write = "0, 0";
		break;
	case 3:
		coordinates_read  = "1, 0, 0";
		coordinates_write = "0, 0, 0";
		break;
	case 4:
		coordinates_read  = "1, 0, 0, 0";
		coordinates_write = "0, 0, 0, 0";
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	position -= strlen(verification_snippet);

	Utils::replaceToken("COORDINATES", position, coordinates_read, out_source.m_parts[0].m_code);

	Utils::replaceToken("COORDINATES", position, coordinates_write, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("IMAGES", maxImageUniforms(in_stage) > 0 ? "1" : "0",
							out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_VEGETA", uni_vegeta, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_TRUNKS", uni_trunks, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("TEX_COORD_TYPE", tex_coord_type, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("TYPE", image_type, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingImagesTest::prepareUniforms(Utils::program& program)
{
	(void)program;
	prepareBuffer(m_goku_buffer, m_goku_data);
	prepareBuffer(m_vegeta_buffer, m_vegeta_data);
	prepareBuffer(m_trunks_buffer, m_trunks_data);

	prepareTexture(m_goku_texture, m_goku_buffer, m_test_case, m_goku_data, 1);
	prepareTexture(m_vegeta_texture, m_vegeta_buffer, m_test_case, m_vegeta_data, 2);
	prepareTexture(m_trunks_texture, m_trunks_buffer, m_test_case, m_trunks_data, 4);
}

/** Overwrite of releaseResource method, release extra buffers and textures
 *
 * @param ignored
 **/
void BindingImagesTest::releaseResource()
{
	m_goku_texture.release();
	m_vegeta_texture.release();
	m_trunks_texture.release();
	if (m_test_case != Utils::TEX_BUFFER)
	{
		m_goku_buffer.release();
		m_vegeta_buffer.release();
		m_trunks_buffer.release();
	}
}

/** Verify that all images have green texel at [0,0,0,0]
 *
 * @return true texel is green, false otherwise
 **/
bool BindingImagesTest::verifyAdditionalResults() const
{
	if (Utils::TEX_BUFFER != m_test_case)
	{
		return (verifyTexture(m_goku_texture) && verifyTexture(m_vegeta_texture) && verifyTexture(m_trunks_texture));
	}
	else
	{
		return (verifyBuffer(m_goku_buffer) && verifyBuffer(m_vegeta_buffer) && verifyBuffer(m_trunks_buffer));
	}
}

/** Constructor
 *
 * @param context Test context
 **/
BindingImageSingleTest::BindingImageSingleTest(deqp::Context& context)
	: BindingImageTest(context, "binding_image_single", "Test verifies single binding of image used in multiple stages")
	, m_goku_texture(context)
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BindingImageSingleTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
	case 0:
		m_test_stage = Utils::VERTEX_SHADER;
		break;
	case 1:
		m_test_stage = Utils::TESS_CTRL_SHADER;
		break;
	case 2:
		m_test_stage = Utils::TESS_EVAL_SHADER;
		break;
	case 3:
		m_test_stage = Utils::GEOMETRY_SHADER;
		break;
	case 4:
		m_test_stage = Utils::FRAGMENT_SHADER;
		break;
	default:
		return false;
		break;
	}

	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Tested stage: "
										<< Utils::getShaderStageName((Utils::SHADER_STAGES)m_test_stage)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingImageSingleTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												 Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku_with_binding = "layout(binding = 2, rgba8) uniform image2D goku;\n";

	static const GLchar* uni_goku_no_binding = "layout(rgba8) uniform image2D goku;\n";

	static const GLchar* verification_snippet = "    vec4 goku_color = imageLoad(goku, ivec2(0,1));\n"
												"\n"
												"    imageStore(goku, ivec2(0,0), vec4(0, 1, 0, 1));\n"
												"\n"
												"    if (vec4(1, 0, 0, 0) != goku_color)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"#if IMAGES\n"
													"UNI_GOKU\n"
													"#endif\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"#if IMAGES\n"
													"VERIFICATION else\n"
													"#endif\n"
													"    if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"#if IMAGES\n"
		"UNI_GOKU\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"#if IMAGES\n"
		"VERIFICATION else\n"
		"#endif\n"
		"    if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "#extension GL_ARB_shader_image_load_store : enable\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "#if IMAGES\n"
													 "UNI_GOKU\n"
													 "#endif\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "#if IMAGES\n"
													 "VERIFICATION else\n"
													 "#endif\n"
													 "    if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "#extension GL_ARB_shader_image_load_store : enable\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "#if IMAGES\n"
												  "UNI_GOKU\n"
												  "#endif\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "#if IMAGES\n"
												  "VERIFICATION"
												  "#endif\n"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template	= 0;
	const GLchar* uniform_definition = uni_goku_no_binding;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template	= compute_shader_template;
		uniform_definition = uni_goku_with_binding;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		/* We can't rely on the binding qualifier being present in m_test_stage
		 * if images are unsupported in that stage.
		 */
		if (maxImageUniforms(m_test_stage) == 0)
			uniform_definition = uni_goku_with_binding;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	if (in_stage == m_test_stage)
	{
		uniform_definition = uni_goku_with_binding;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("IMAGES", maxImageUniforms(in_stage) > 0 ? "1" : "0",
							out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uniform_definition, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingImageSingleTest::prepareUniforms(Utils::program& program)
{
	(void)program;
	static const GLuint goku_data = 0x000000ff;

	prepareTexture(m_goku_texture, Utils::buffer(m_context), Utils::TEX_2D, goku_data, 2 /* unit */);
}

/** Overwrite of releaseResource method, release extra texture
 *
 * @param ignored
 **/
void BindingImageSingleTest::releaseResource()
{
	m_goku_texture.release();
}

/** Verify that all images have green texel at [0,0,0,0]
 *
 * @return true texel is green, false otherwise
 **/
bool BindingImageSingleTest::verifyAdditionalResults() const
{
	return verifyTexture(m_goku_texture);
}

/** Constructor
 *
 * @param context Test context
 **/
BindingImageArrayTest::BindingImageArrayTest(deqp::Context& context)
	: BindingImageTest(context, "binding_image_array", "Test verifies binding of image array")
	, m_goku_00_texture(context)
	, m_goku_01_texture(context)
	, m_goku_02_texture(context)
	, m_goku_03_texture(context)
	, m_goku_04_texture(context)
	, m_goku_05_texture(context)
	, m_goku_06_texture(context)
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingImageArrayTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(binding = 1, rgba8) uniform image2D goku[7];\n";

	static const GLchar* verification_snippet = "    vec4 color[7];\n"
												"\n"
												"    for (uint i = 0u; i < 7; ++i)\n"
												"    {\n"
												"        color[i] = imageLoad(goku[i], ivec2(0,0));\n"
												"    }\n"
												"\n"
												"    if ((vec4(0, 0, 0, 0) != color[0]) ||\n"
												"        (vec4(0, 0, 0, 1) != color[1]) ||\n"
												"        (vec4(0, 0, 1, 0) != color[2]) ||\n"
												"        (vec4(0, 0, 1, 1) != color[3]) ||\n"
												"        (vec4(0, 1, 0, 0) != color[4]) ||\n"
												"        (vec4(0, 1, 0, 1) != color[5]) ||\n"
												"        (vec4(0, 1, 1, 0) != color[6]) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"#if IMAGES\n"
													"UNI_GOKU\n"
													"#endif\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"#if IMAGES\n"
													"VERIFICATION else\n"
													"#endif\n"
													"    if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"#if IMAGES\n"
		"UNI_GOKU\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"#if IMAGES\n"
		"VERIFICATION else\n"
		"#endif\n"
		"    if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "#extension GL_ARB_shader_image_load_store : enable\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "#if IMAGES\n"
													 "UNI_GOKU\n"
													 "#endif\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "#if IMAGES\n"
													 "VERIFICATION else\n"
													 "#endif\n"
													 "    if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "#extension GL_ARB_shader_image_load_store : enable\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "#if IMAGES\n"
												  "UNI_GOKU\n"
												  "#endif\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "#if IMAGES\n"
												  "VERIFICATION"
												  "#endif\n"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("IMAGES", maxImageUniforms(in_stage) > 0 ? "1" : "0",
							out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingImageArrayTest::prepareUniforms(Utils::program& program)
{
	static const GLuint goku_data[7] = {
		0x00000000, 0xff000000, 0x00ff0000, 0xffff0000, 0x0000ff00, 0xff00ff00, 0x00ffff00,
	};

	Utils::texture* textures[7] = {
		&m_goku_00_texture, &m_goku_01_texture, &m_goku_02_texture, &m_goku_03_texture,
		&m_goku_04_texture, &m_goku_05_texture, &m_goku_06_texture,
	};

	for (GLuint i = 0; i < 7; ++i)
	{
		GLint expected_binding = i + 1;

		checkBinding(program, i, expected_binding);

		prepareTexture(*textures[i], Utils::buffer(m_context), Utils::TEX_2D, goku_data[i], expected_binding);
	}
}

/** Overwrite of releaseResource method, release extra textures
 *
 * @param ignored
 **/
void BindingImageArrayTest::releaseResource()
{
	Utils::texture* textures[7] = {
		&m_goku_00_texture, &m_goku_01_texture, &m_goku_02_texture, &m_goku_03_texture,
		&m_goku_04_texture, &m_goku_05_texture, &m_goku_06_texture,
	};

	for (GLuint i = 0; i < 7; ++i)
	{
		textures[i]->release();
	}
}

/** Verifies that API reports correct uniform binding
 *
 * @param program          Program
 * @param index            Index of array element
 * @param expected_binding Expected binding
 **/
void BindingImageArrayTest::checkBinding(Utils::program& program, GLuint index, GLint expected_binding)
{
	if (false == Utils::checkUniformArrayBinding(program, "goku", index, expected_binding))
	{
		TCU_FAIL("Wrong binding reported by API");
	}
}

/** Constructor
 *
 * @param context Test context
 **/
BindingImageDefaultTest::BindingImageDefaultTest(deqp::Context& context)
	: APITestBase(context, "binding_image_default", "Test verifies default image binding")
{
	/* Nothing to be done here */
}

/** Execute API call and verifies results
 *
 * @return true when results are positive, false otherwise
 **/
bool BindingImageDefaultTest::checkResults(Utils::program& program)
{
	return Utils::checkUniformBinding(program, "goku", 0 /* expected_binding */);
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingImageDefaultTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												  Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(rgba8) uniform image2D goku;\n";

	static const GLchar* verification_snippet = "    vec4 goku_color = imageLoad(goku, ivec2(0,0));\n"
												"\n"
												"    if (vec4(1, 0, 0, 0) != goku_color)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"#if IMAGES\n"
													"UNI_GOKU\n"
													"#endif\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"#if IMAGES\n"
													"VERIFICATION else\n"
													"#endif\n"
													"    if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"#if IMAGES\n"
		"UNI_GOKU\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"#if IMAGES\n"
		"VERIFICATION else\n"
		"#endif\n"
		"    if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "#extension GL_ARB_shader_image_load_store : enable\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "#if IMAGES\n"
													 "UNI_GOKU\n"
													 "#endif\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "#if IMAGES\n"
													 "VERIFICATION else\n"
													 "#endif\n"
													 "    if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "#extension GL_ARB_shader_image_load_store : enable\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "#if IMAGES\n"
												  "UNI_GOKU\n"
												  "#endif\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "#if IMAGES\n"
												  "VERIFICATION"
												  "#endif\n"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("IMAGES", maxImageUniforms(in_stage) > 0 ? "1" : "0",
							out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
BindingImageAPIOverrideTest::BindingImageAPIOverrideTest(deqp::Context& context)
	: BindingImageTest(context, "binding_image_api_override", "Verifies that API can override image binding")
	, m_goku_texture(context)
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingImageAPIOverrideTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													  Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(binding = 3, rgba8) uniform image2D goku;\n";

	static const GLchar* verification_snippet = "    vec4 goku_color = imageLoad(goku, ivec2(0,0));\n"
												"\n"
												"    if (vec4(1, 0, 0, 0) != goku_color)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"#if IMAGES\n"
													"UNI_GOKU\n"
													"#endif\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"#if IMAGES\n"
													"VERIFICATION else\n"
													"#endif\n"
													"    if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"#if IMAGES\n"
		"UNI_GOKU\n"
		"#endif\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"#if IMAGES\n"
		"VERIFICATION else\n"
		"#endif\n"
		"    if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "#extension GL_ARB_shader_image_load_store : enable\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "#if IMAGES\n"
													 "UNI_GOKU\n"
													 "#endif\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "#if IMAGES\n"
													 "VERIFICATION else\n"
													 "#endif\n"
													 "    if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "#extension GL_ARB_shader_image_load_store : enable\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "#if IMAGES\n"
												  "UNI_GOKU\n"
												  "#endif\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "#if IMAGES\n"
												  "VERIFICATION"
												  "#endif\n"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("IMAGES", maxImageUniforms(in_stage) > 0 ? "1" : "0",
							out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method, set up values for unit_left and unit_right
 *
 * @param program Current program
 **/
void BindingImageAPIOverrideTest::prepareUniforms(Utils::program& program)
{
	static const GLuint goku_data   = 0x000000ff;
	static const GLint  new_binding = 7;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const GLint uniform_location = program.getUniformLocation("goku");
	if (-1 == uniform_location)
	{
		TCU_FAIL("Uniform is inactive");
	}

	gl.uniform1i(uniform_location, new_binding);

	GLint binding = -1;

	gl.getUniformiv(program.m_program_object_id, uniform_location, &binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getUniformiv");

	if (new_binding != binding)
	{
		TCU_FAIL("Wrong binding value");
		return;
	}

	prepareTexture(m_goku_texture, Utils::buffer(m_context), Utils::TEX_2D, goku_data, new_binding);
}

/** Overwrite of releaseResource method, release extra texture
 *
 * @param ignored
 **/
void BindingImageAPIOverrideTest::releaseResource()
{
	m_goku_texture.release();
}

/** Constructor
 *
 * @param context Test context
 **/
BindingImageInvalidTest::BindingImageInvalidTest(deqp::Context& context)
	: NegativeTestBase(context, "binding_image_invalid", "Test verifies invalid binding values")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BindingImageInvalidTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
		m_case = TEST_CASES_MAX;
		break;
	case NEGATIVE_VALUE:
	case VARIABLE_NAME:
	case STD140:
	case MISSING:
		m_case = (TESTCASES)test_case_index;
		break;
	default:
		return false;
	}

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BindingImageInvalidTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												  Utils::shaderSource& out_source)
{
	static const GLchar* uni_goku = "layout(binding BINDING, rgba8) uniform image2D goku;\n";

	static const GLchar* verification_snippet = "    vec4 goku_color = imageLoad(goku, ivec2(0,0));\n"
												"\n"
												"    if (vec4(1, 0, 0, 0) != goku_color)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"#extension GL_ARB_shader_image_load_store : enable\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNI_GOKU\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"#extension GL_ARB_shader_image_load_store : enable\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNI_GOKU\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "#extension GL_ARB_shader_image_load_store : enable\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNI_GOKU\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "#extension GL_ARB_shader_image_load_store : enable\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNI_GOKU\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;

	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("UNI_GOKU", uni_goku, out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("BINDING", getCaseString(m_case), out_source.m_parts[0].m_code);
}

const GLchar* BindingImageInvalidTest::getCaseString(TESTCASES test_case)
{
	(void)test_case;
	const GLchar* binding = 0;

	switch (m_case)
	{
	case NEGATIVE_VALUE:
		binding = "= -1";
		break;
	case VARIABLE_NAME:
		binding = "= goku";
		break;
	case STD140:
		binding = "= std140";
		break;
	case MISSING:
		binding = "";
		break;
	case TEST_CASES_MAX:
		binding = "= 0";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return binding;
}

/* Constants used by InitializerListTest */
const GLfloat InitializerListTest::m_value = 0.0625f;

/** Constructor
 *
 * @param context Test context
 **/
InitializerListTest::InitializerListTest(deqp::Context& context)
	: GLSLTestBase(context, "initializer_list", "Test verifies initializer lists")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool InitializerListTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		m_current_test_case_index = 0;
		return true;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	logTestCaseName();

	return true;
}

/** Overwritte of prepareUniforms method
 *
 * @param program Current program
 **/
void InitializerListTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat float_data[16] = { m_value, m_value, m_value, m_value, m_value, m_value, m_value, m_value,
											m_value, m_value, m_value, m_value, m_value, m_value, m_value, m_value };

	program.uniform("uni_matrix", Utils::FLOAT, 4, 4, float_data);
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void InitializerListTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											  Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet = "    TYPE_NAME variableARRAY_DEFINITION = INITIALIZATION;\n"
												"\n"
												"    float sum = SUM;\n"
												"\n"
												"    if (EXPECTED_VALUE != sum)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform mat4    uni_matrix;\n"
		"\n"
		"TYPE_DEFINITION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"uniform mat4 uni_matrix;\n"
													"\n"
													"TYPE_DEFINITION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"uniform mat4 uni_matrix;\n"
													"\n"
													"TYPE_DEFINITION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"uniform mat4 uni_matrix;\n"
		"\n"
		"TYPE_DEFINITION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "uniform mat4 uni_matrix;\n"
													 "\n"
													 "TYPE_DEFINITION\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "uniform mat4 uni_matrix;\n"
												  "\n"
												  "TYPE_DEFINITION\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const std::string& array_definition = getArrayDefinition();
	const std::string& expected_value   = getExpectedValue();
	const std::string& initialization   = getInitialization();
	const GLchar*	  shader_template  = 0;
	const std::string& sum				= getSum();
	const std::string& type_definition  = getTypeDefinition();
	const std::string& type_name		= getTypeName();

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("TYPE_DEFINITION", position, type_definition.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	position -= strlen(verification_snippet);

	Utils::replaceToken("TYPE_NAME", position, type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("ARRAY_DEFINITION", position, array_definition.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("INITIALIZATION", position, initialization.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("SUM", position, sum.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("EXPECTED_VALUE", position, expected_value.c_str(), out_source.m_parts[0].m_code);
}

/** Prepare test cases
 *
 * @return true
 **/
bool InitializerListTest::testInit()
{
	for (GLuint i = 0; i < TESTED_INITIALIZERS_MAX; ++i)
	{
		const TESTED_INITIALIZERS l_init = (TESTED_INITIALIZERS)i;

		testCase test_case = { l_init, 1, 1 };

		switch (l_init)
		{
		case VECTOR:
		case ARRAY_VECTOR_CTR:
		case ARRAY_VECTOR_LIST:
		case UNSIZED_ARRAY_VECTOR:
			for (GLuint row = 2; row <= 4; ++row)
			{
				test_case.m_n_rows = row;

				m_test_cases.push_back(test_case);
			}

			break;

		case MATRIX:
		case MATRIX_ROWS:
		case ARRAY_MATRIX_CTR:
		case ARRAY_MATRIX_LIST:
		case UNSIZED_ARRAY_MATRIX:
			for (GLuint col = 2; col <= 4; ++col)
			{
				for (GLuint row = 2; row <= 4; ++row)
				{
					test_case.m_n_cols = col;
					test_case.m_n_rows = row;

					m_test_cases.push_back(test_case);
				}
			}

			break;

		case ARRAY_SCALAR:
		case UNSIZED_ARRAY_SCALAR:
			m_test_cases.push_back(test_case);

			break;

		case STRUCT:
		case ARRAY_STRUCT:
		case NESTED_STRUCT_CTR:
		case NESTED_STRUCT_LIST:
		case NESTED_STURCT_ARRAYS_STRUCT_LIST:
		case NESTED_STURCT_ARRAYS_STRUCT_MIX:
		case NESTED_ARRAY_STRUCT_STRUCT_LIST:
		case NESTED_ARRAY_STRUCT_STRUCT_MIX:
		case NESTED_STRUCT_STRUCT_ARRAY_LIST:
		case NESTED_STRUCT_STRUCT_ARRAY_MIX:
		case UNSIZED_ARRAY_STRUCT:
			test_case.m_n_rows = 4;
			m_test_cases.push_back(test_case);

			break;
		default:
			DE_ASSERT(0);
			break;
		}
	}

	return true;
}

/** Get string representing "[SIZE]" for current test case
 *
 * @return String
 **/
std::string InitializerListTest::getArrayDefinition()
{
	const testCase& test_case = m_test_cases[m_current_test_case_index];

	std::string array_definition;

	switch (test_case.m_initializer)
	{
	case VECTOR:
	case MATRIX:
	case MATRIX_ROWS:
	case STRUCT:
	case NESTED_STRUCT_CTR:
	case NESTED_STRUCT_LIST:
	case NESTED_STURCT_ARRAYS_STRUCT_LIST:
	case NESTED_STURCT_ARRAYS_STRUCT_MIX:
	case NESTED_STRUCT_STRUCT_ARRAY_LIST:
	case NESTED_STRUCT_STRUCT_ARRAY_MIX:
		array_definition = "";
		break;
	case ARRAY_SCALAR:
	case ARRAY_VECTOR_CTR:
	case ARRAY_VECTOR_LIST:
	case ARRAY_MATRIX_CTR:
	case ARRAY_MATRIX_LIST:
	case ARRAY_STRUCT:
	case NESTED_ARRAY_STRUCT_STRUCT_LIST:
	case NESTED_ARRAY_STRUCT_STRUCT_MIX:
		array_definition = "[4]";
		break;
	case UNSIZED_ARRAY_SCALAR:
	case UNSIZED_ARRAY_VECTOR:
	case UNSIZED_ARRAY_MATRIX:
	case UNSIZED_ARRAY_STRUCT:
		array_definition = "[]";
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return array_definition;
}

/** Get string representing expected value of sum for current test case
 *
 * @return String
 **/
std::string InitializerListTest::getExpectedValue()
{
	const testCase& test_case = m_test_cases[m_current_test_case_index];

	GLfloat value = 0.0f;

	switch (test_case.m_initializer)
	{
	case VECTOR:
	case MATRIX:
	case MATRIX_ROWS:
		value = (GLfloat)(test_case.m_n_cols * test_case.m_n_rows);
		break;
	case ARRAY_VECTOR_CTR:
	case ARRAY_VECTOR_LIST:
	case ARRAY_MATRIX_CTR:
	case ARRAY_MATRIX_LIST:
	case UNSIZED_ARRAY_VECTOR:
	case UNSIZED_ARRAY_MATRIX:
		value = (GLfloat)(test_case.m_n_cols * test_case.m_n_rows) * 4.0f;
		break;
	case ARRAY_SCALAR:
	case UNSIZED_ARRAY_SCALAR:
		value = 4.0f;
		break;
	case STRUCT:
		value = 8.0f;
		break;
	case NESTED_STRUCT_CTR:
	case NESTED_STRUCT_LIST:
		value = 12.0f;
		break;
	case NESTED_STRUCT_STRUCT_ARRAY_LIST:
	case NESTED_STRUCT_STRUCT_ARRAY_MIX:
		value = 16.0f;
		break;
	case NESTED_STURCT_ARRAYS_STRUCT_LIST:
	case NESTED_STURCT_ARRAYS_STRUCT_MIX:
		value = 28.0f;
		break;
	case ARRAY_STRUCT:
	case UNSIZED_ARRAY_STRUCT:
		value = 32.0f;
		break;
	case NESTED_ARRAY_STRUCT_STRUCT_LIST:
	case NESTED_ARRAY_STRUCT_STRUCT_MIX:
		value = 48.0f;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	value *= m_value;

	std::string expected_value;
	expected_value.resize(64, 0);

	sprintf(&expected_value[0], "%f", value);

	return expected_value;
}

/** Get string representing initialization list for current test case
 *
 * @return String
 **/
std::string InitializerListTest::getInitialization()
{
	const testCase& test_case = m_test_cases[m_current_test_case_index];

	std::string initialization;

	switch (test_case.m_initializer)
	{
	case VECTOR:
		initialization.append(getVectorInitializer(0 /*column*/, test_case.m_n_rows));

		break;

	case MATRIX:
		initialization = "{ ";
		initialization.append(getVectorArrayList(test_case.m_n_cols, test_case.m_n_rows));
		initialization.append(" }");

		break;

	case MATRIX_ROWS:
	{
		initialization = "{ ";
		initialization.append(getVectorArrayCtr(test_case.m_n_cols, test_case.m_n_rows));
		initialization.append(" }");
	}
	break;

	case STRUCT:
		initialization = "{ ";
		initialization.append(getVectorInitializer(0 /* column */, test_case.m_n_rows));
		initialization.append(", ");
		initialization.append(getVectorInitializer(2 /* column */, test_case.m_n_rows));
		initialization.append(" }");

		break;

	case ARRAY_SCALAR:
	case UNSIZED_ARRAY_SCALAR:
		initialization = "{ ";
		initialization.append(getVectorValues(0 /* column */, 4 /* size */));
		initialization.append(" }");

		break;

	case ARRAY_VECTOR_LIST:
	case UNSIZED_ARRAY_VECTOR:
		initialization = "{ ";
		initialization.append(getVectorArrayList(4 /* columns */, test_case.m_n_rows));
		initialization.append(" }");

		break;

	case ARRAY_VECTOR_CTR:
		initialization = "{ ";
		initialization.append(getVectorArrayCtr(4 /* columns */, test_case.m_n_rows));
		initialization.append(" }");

		break;

	case ARRAY_MATRIX_LIST:
	case UNSIZED_ARRAY_MATRIX:
		initialization = "{ ";

		for (GLuint i = 0; i < 4; ++i)
		{
			initialization.append("{ ");
			initialization.append(getVectorArrayList(test_case.m_n_cols, test_case.m_n_rows));
			initialization.append(" }");

			if (i + 1 < 4)
			{
				initialization.append(", ");
			}
		}

		initialization.append(" }");

		break;

	case ARRAY_MATRIX_CTR:
	{
		const std::string& type_name = Utils::getTypeName(Utils::FLOAT, test_case.m_n_cols, test_case.m_n_rows);

		initialization = "{ ";

		for (GLuint i = 0; i < 4; ++i)
		{
			initialization.append(type_name);
			initialization.append("(");
			for (GLuint col = 0; col < test_case.m_n_cols; ++col)
			{
				initialization.append(getVectorValues(col, test_case.m_n_rows));

				if (col + 1 < test_case.m_n_cols)
				{
					initialization.append(", ");
				}
			}
			initialization.append(")");

			if (i + 1 < 4)
			{
				initialization.append(", ");
			}
		}

		initialization.append(" }");
	}
	break;

	case ARRAY_STRUCT:
	case UNSIZED_ARRAY_STRUCT:
		initialization = "{ ";

		for (GLuint i = 0; i < 4; ++i)
		{
			initialization.append("{ ");
			initialization.append(getVectorInitializer(0 /* column */, test_case.m_n_rows));
			initialization.append(", ");
			initialization.append(getVectorInitializer(2 /* column */, test_case.m_n_rows));
			initialization.append(" }");

			if (i + 1 < 4)
			{
				initialization.append(", ");
			}
		}

		initialization.append(" }");

		break;

	case NESTED_STRUCT_CTR:
		initialization = "StructureWithStructure(BasicStructure(";
		initialization.append(getVectorConstructor(0 /* column */, 4));
		initialization.append(", ");
		initialization.append(getVectorConstructor(2 /* column */, 4));
		initialization.append("), ");
		initialization.append(getVectorConstructor(3 /* column */, 4));
		initialization.append(")");

		break;

	case NESTED_STRUCT_LIST:
		initialization = "{ { ";
		initialization.append(getVectorInitializer(0 /* column */, 4));
		initialization.append(", ");
		initialization.append(getVectorInitializer(2 /* column */, 4));
		initialization.append(" }, ");
		initialization.append(getVectorInitializer(3 /* column */, 4));
		initialization.append(" }");

		break;

	case NESTED_STURCT_ARRAYS_STRUCT_LIST:
		initialization = "{ ";
		initialization.append(getVectorInitializer(0 /* column */, 4));
		initialization.append(", { ");

		for (GLuint i = 0; i < 3; ++i)
		{
			initialization.append("{ ");
			initialization.append(getVectorInitializer(2 /* column */, 4));
			initialization.append(", ");
			initialization.append(getVectorInitializer(3 /* column */, 4));
			initialization.append(" }");

			if (i + 1 < 3)
			{
				initialization.append(", ");
			}
		}

		initialization.append(" } }");

		break;

	case NESTED_STURCT_ARRAYS_STRUCT_MIX:
		initialization = "{ ";
		initialization.append(getVectorConstructor(0 /* column */, 4));
		initialization.append(", { ");

		for (GLuint i = 0; i < 3; ++i)
		{
			initialization.append("{ ");
			initialization.append(getVectorInitializer(2 /* column */, 4));
			initialization.append(", ");
			initialization.append(getVectorConstructor(3 /* column */, 4));
			initialization.append(" }");

			if (i + 1 < 3)
			{
				initialization.append(", ");
			}
		}

		initialization.append(" } }");

		break;

	case NESTED_ARRAY_STRUCT_STRUCT_LIST:
		initialization = "{ ";

		for (GLuint i = 0; i < 4; ++i)
		{
			initialization.append("{ { ");

			initialization.append(getVectorInitializer(0 /* column */, 4));
			initialization.append(", ");
			initialization.append(getVectorInitializer(1 /* column */, 4));

			initialization.append(" }, ");

			initialization.append(getVectorInitializer(2 /* column */, 4));

			initialization.append(" }");

			if (i + 1 < 4)
			{
				initialization.append(", ");
			}
		}

		initialization.append(" }");

		break;

	case NESTED_ARRAY_STRUCT_STRUCT_MIX:
		initialization = "{\n";

		for (GLuint i = 0; i < 2; ++i)
		{
			initialization.append("StructureWithStructure(\n");
			initialization.append("BasicStructure(");

			initialization.append(getVectorConstructor(0 /* column */, 4));
			initialization.append(" , ");
			initialization.append(getVectorConstructor(1 /* column */, 4));

			initialization.append("), ");

			initialization.append(getVectorConstructor(2 /* column */, 4));

			initialization.append(")");

			initialization.append(" , ");

			initialization.append("{ { ");

			initialization.append(getVectorInitializer(0 /* column */, 4));
			initialization.append(", ");
			initialization.append(getVectorInitializer(1 /* column */, 4));

			initialization.append(" }, ");

			initialization.append(getVectorInitializer(2 /* column */, 4));

			initialization.append(" }");

			if (i + 1 < 2)
			{
				initialization.append(" , ");
			}
		}

		initialization.append(" }");

		break;

	case NESTED_STRUCT_STRUCT_ARRAY_LIST:
		initialization = "{ ";
		initialization.append("{ ");
		initialization.append(getVectorInitializer(0 /* column */, 4));
		initialization.append(", ");
		initialization.append("{ ");
		initialization.append(getVectorInitializer(1 /* column */, 4));
		initialization.append(", ");
		initialization.append(getVectorInitializer(2 /* column */, 4));
		initialization.append(" }");
		initialization.append(" }");
		initialization.append(", ");
		initialization.append(getVectorInitializer(3 /* column */, 4));
		initialization.append(" }");

		break;

	case NESTED_STRUCT_STRUCT_ARRAY_MIX:
		initialization = "StructureWithStructureWithArray(";
		initialization.append("StructureWithArray(");
		initialization.append(getVectorConstructor(0 /* column */, 4));
		initialization.append(" , vec4[2]( ");
		initialization.append(getVectorConstructor(1 /* column */, 4));
		initialization.append(" , ");
		initialization.append(getVectorConstructor(2 /* column */, 4));
		initialization.append(" )");
		initialization.append(")");
		initialization.append(" , ");
		initialization.append(getVectorConstructor(3 /* column */, 4));
		initialization.append(")");

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return initialization;
}

/** Logs description of current test case
 *
 **/
void InitializerListTest::logTestCaseName()
{
	const testCase& test_case = m_test_cases[m_current_test_case_index];

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	switch (test_case.m_initializer)
	{
	case VECTOR:
		message << "List. Single vec" << test_case.m_n_rows;
		break;
	case MATRIX:
		message << "List. Single mat" << test_case.m_n_cols << "x" << test_case.m_n_rows;
		break;
	case MATRIX_ROWS:
		message << "Ctr. Single mat" << test_case.m_n_cols << "x" << test_case.m_n_rows;
		break;
	case STRUCT:
		message << "List. Structure";
		break;
	case NESTED_STRUCT_CTR:
		message << "Ctr. Nested structure";
		break;
	case NESTED_STRUCT_LIST:
		message << "List. Nested structure";
		break;
	case NESTED_STURCT_ARRAYS_STRUCT_LIST:
		message << "List. Structure with structure array";
		break;
	case NESTED_STURCT_ARRAYS_STRUCT_MIX:
		message << "Mix. Structure with structure array";
		break;
	case NESTED_STRUCT_STRUCT_ARRAY_LIST:
		message << "List. Structure with structure with array";
		break;
	case NESTED_STRUCT_STRUCT_ARRAY_MIX:
		message << "Mix. Structure with structure with array";
		break;
	case ARRAY_SCALAR:
		message << "List. Array of scalars";
		break;
	case ARRAY_VECTOR_CTR:
		message << "Ctr. Array of vec" << test_case.m_n_rows;
		break;
	case ARRAY_VECTOR_LIST:
		message << "List. Array of vec" << test_case.m_n_rows;
		break;
	case ARRAY_MATRIX_CTR:
		message << "Ctr. Array of mat" << test_case.m_n_cols << "x" << test_case.m_n_rows;
		break;
	case ARRAY_MATRIX_LIST:
		message << "List. Array of mat" << test_case.m_n_cols << "x" << test_case.m_n_rows;
		break;
	case ARRAY_STRUCT:
		message << "List. Array of structures";
		break;
	case NESTED_ARRAY_STRUCT_STRUCT_LIST:
		message << "List. Array of structures with structures";
		break;
	case NESTED_ARRAY_STRUCT_STRUCT_MIX:
		message << "Mix. Array of structures with structures";
		break;
	case UNSIZED_ARRAY_SCALAR:
		message << "List. Unsized array of scalars";
		break;
	case UNSIZED_ARRAY_VECTOR:
		message << "List. Unsized array of vec" << test_case.m_n_rows;
		break;
	case UNSIZED_ARRAY_MATRIX:
		message << "List. Unsized array of mat" << test_case.m_n_cols << "x" << test_case.m_n_rows;
		break;
	case UNSIZED_ARRAY_STRUCT:
		message << "List. Unsized array of structures";
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	message << tcu::TestLog::EndMessage;
}

/** Get string representing sum for current test case
 *
 * @return String
 **/
std::string InitializerListTest::getSum()
{
	static const GLchar* var = "variable";

	const testCase& test_case = m_test_cases[m_current_test_case_index];

	std::string sum;

	switch (test_case.m_initializer)
	{
	case VECTOR:
		sum = getVectorSum(var, test_case.m_n_rows);

		break;

	case MATRIX:
	case MATRIX_ROWS:
		sum = getVectorArraySum("variable[INDEX]", test_case.m_n_cols, test_case.m_n_rows);

		break;

	case STRUCT:
		sum = getVectorSum("variable.member_a", test_case.m_n_rows);
		sum.append(" + ");
		sum.append(getVectorSum("variable.member_b", test_case.m_n_rows));

		break;

	case ARRAY_SCALAR:
	case UNSIZED_ARRAY_SCALAR:
		sum = "variable[0] + variable[1] + variable[2] + variable[3]";

		break;

	case ARRAY_VECTOR_LIST:
	case ARRAY_VECTOR_CTR:
	case UNSIZED_ARRAY_VECTOR:
		sum = getVectorArraySum("variable[INDEX]", 4 /* columns */, test_case.m_n_rows);

		break;

	case ARRAY_MATRIX_LIST:
	case ARRAY_MATRIX_CTR:
	case UNSIZED_ARRAY_MATRIX:
		sum.append(getVectorArraySum("variable[0][INDEX]", test_case.m_n_cols, test_case.m_n_rows));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable[1][INDEX]", test_case.m_n_cols, test_case.m_n_rows));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable[2][INDEX]", test_case.m_n_cols, test_case.m_n_rows));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable[3][INDEX]", test_case.m_n_cols, test_case.m_n_rows));

		break;

	case ARRAY_STRUCT:
	case UNSIZED_ARRAY_STRUCT:
		sum.append(getVectorArraySum("variable[INDEX].member_a", 4, test_case.m_n_rows));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable[INDEX].member_b", 4, test_case.m_n_rows));

		break;

	case NESTED_STRUCT_CTR:
	case NESTED_STRUCT_LIST:
		sum.append(getVectorSum("variable.member_a.member_a", 4));
		sum.append(" + ");
		sum.append(getVectorSum("variable.member_a.member_b", 4));
		sum.append(" + ");
		sum.append(getVectorSum("variable.member_b", 4));

		break;

	case NESTED_STURCT_ARRAYS_STRUCT_LIST:
	case NESTED_STURCT_ARRAYS_STRUCT_MIX:
		sum.append(getVectorSum("variable.member_a", 4));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable.member_b[INDEX].member_a", 3, 4));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable.member_b[INDEX].member_b", 3, 4));

		break;

	case NESTED_ARRAY_STRUCT_STRUCT_LIST:
	case NESTED_ARRAY_STRUCT_STRUCT_MIX:
		sum.append(getVectorArraySum("variable[INDEX].member_a.member_a", 4, 4));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable[INDEX].member_a.member_b", 4, 4));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable[INDEX].member_b", 4, 4));

		break;

	case NESTED_STRUCT_STRUCT_ARRAY_LIST:
	case NESTED_STRUCT_STRUCT_ARRAY_MIX:
		sum.append(getVectorSum("variable.member_a.member_a", 4));
		sum.append(" + ");
		sum.append(getVectorArraySum("variable.member_a.member_b[INDEX]", 2, 4));
		sum.append(" + ");
		sum.append(getVectorSum("variable.member_b", 4));

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return sum;
}

/** Get string representing types definition for current test case
 *
 * @return String
 **/
std::string InitializerListTest::getTypeDefinition()
{
	const testCase& test_case = m_test_cases[m_current_test_case_index];

	static const GLchar* basic_struct = "struct BasicStructure {\n"
										"    vec4 member_a;\n"
										"    vec4 member_b;\n"
										"};\n";

	static const GLchar* struct_with_array = "struct StructureWithArray {\n"
											 "    vec4 member_a;\n"
											 "    vec4 member_b[2];\n"
											 "};\n";

	static const GLchar* struct_with_struct = "struct StructureWithStructure {\n"
											  "    BasicStructure member_a;\n"
											  "    vec4           member_b;\n"
											  "};\n";

	static const GLchar* struct_with_struct_array = "struct StructureWithStructArray {\n"
													"    vec4           member_a;\n"
													"    BasicStructure member_b[3];\n"
													"};\n";

	static const GLchar* struct_with_struct_with_array = "struct StructureWithStructureWithArray {\n"
														 "    StructureWithArray member_a;\n"
														 "    vec4               member_b;\n"
														 "};\n";

	std::string type_definition;

	switch (test_case.m_initializer)
	{
	case VECTOR:
	case MATRIX:
	case MATRIX_ROWS:
	case ARRAY_SCALAR:
	case ARRAY_VECTOR_CTR:
	case ARRAY_VECTOR_LIST:
	case ARRAY_MATRIX_CTR:
	case ARRAY_MATRIX_LIST:
	case UNSIZED_ARRAY_SCALAR:
	case UNSIZED_ARRAY_VECTOR:
	case UNSIZED_ARRAY_MATRIX:
		type_definition = "";
		break;
	case STRUCT:
	case ARRAY_STRUCT:
	case UNSIZED_ARRAY_STRUCT:
		type_definition = basic_struct;
		break;
	case NESTED_STRUCT_CTR:
	case NESTED_STRUCT_LIST:
	case NESTED_ARRAY_STRUCT_STRUCT_LIST:
	case NESTED_ARRAY_STRUCT_STRUCT_MIX:
		type_definition = basic_struct;
		type_definition.append(struct_with_struct);
		break;
	case NESTED_STURCT_ARRAYS_STRUCT_LIST:
	case NESTED_STURCT_ARRAYS_STRUCT_MIX:
		type_definition = basic_struct;
		type_definition.append(struct_with_struct_array);
		break;
	case NESTED_STRUCT_STRUCT_ARRAY_LIST:
	case NESTED_STRUCT_STRUCT_ARRAY_MIX:
		type_definition = struct_with_array;
		type_definition.append(struct_with_struct_with_array);
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return type_definition;
}

/** Get string representing name of variable's type for current test case
 *
 * @return String
 **/
std::string InitializerListTest::getTypeName()
{
	const testCase& test_case = m_test_cases[m_current_test_case_index];

	static const GLchar* basic_struct = "BasicStructure";

	static const GLchar* struct_with_struct = "StructureWithStructure";

	static const GLchar* struct_with_struct_array = "StructureWithStructArray";

	static const GLchar* struct_with_struct_with_array = "StructureWithStructureWithArray";

	std::string type_name;

	switch (test_case.m_initializer)
	{
	case VECTOR:
	case MATRIX:
	case MATRIX_ROWS:
	case ARRAY_VECTOR_CTR:
	case ARRAY_VECTOR_LIST:
	case ARRAY_MATRIX_CTR:
	case ARRAY_MATRIX_LIST:
	case UNSIZED_ARRAY_VECTOR:
	case UNSIZED_ARRAY_MATRIX:
		type_name = Utils::getTypeName(Utils::FLOAT, test_case.m_n_cols, test_case.m_n_rows);
		break;
	case STRUCT:
	case ARRAY_STRUCT:
	case UNSIZED_ARRAY_STRUCT:
		type_name = basic_struct;
		break;
	case NESTED_STRUCT_CTR:
	case NESTED_STRUCT_LIST:
	case NESTED_ARRAY_STRUCT_STRUCT_LIST:
	case NESTED_ARRAY_STRUCT_STRUCT_MIX:
		type_name = struct_with_struct;
		break;
	case NESTED_STURCT_ARRAYS_STRUCT_LIST:
	case NESTED_STURCT_ARRAYS_STRUCT_MIX:
		type_name = struct_with_struct_array;
		break;
	case NESTED_STRUCT_STRUCT_ARRAY_LIST:
	case NESTED_STRUCT_STRUCT_ARRAY_MIX:
		type_name = struct_with_struct_with_array;
		break;
	case ARRAY_SCALAR:
	case UNSIZED_ARRAY_SCALAR:
		type_name = "float";
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return type_name;
}

/** Get string representing array of vector constructors
 *
 * @param columns Number of columns
 * @param size    Size of vector
 *
 * @return String
 **/
std::string InitializerListTest::getVectorArrayCtr(GLuint columns, GLuint size)
{
	std::string result;

	for (GLuint col = 0; col < columns; ++col)
	{
		result.append(getVectorConstructor(col, size));

		if (col + 1 < columns)
		{
			result.append(", ");
		}
	}

	return result;
}

/** Get string representing array of vector initializers
 *
 * @param columns Number of columns
 * @param size    Size of vector
 *
 * @return String
 **/
std::string InitializerListTest::getVectorArrayList(GLuint columns, GLuint size)
{
	std::string result;

	for (GLuint col = 0; col < columns; ++col)
	{
		result.append(getVectorInitializer(col, size));

		if (col + 1 < columns)
		{
			result.append(", ");
		}
	}

	return result;
}

/** Get string representing vector constructor
 *
 * @param column Index of column of uni_matrix to use as data source
 * @param size   Size of vector
 *
 * @return String
 **/
std::string InitializerListTest::getVectorConstructor(GLuint column, GLuint size)
{
	const std::string& type_name = Utils::getTypeName(Utils::FLOAT, 1 /*n_cols*/, size);

	std::string result;

	result.append(type_name);
	result.append("(");
	result.append(getVectorValues(column, size));
	result.append(")");

	return result;
}

/** Get string representing vector initializer
 *
 * @param column Index of column of uni_matrix to use as data source
 * @param size   Size of vector
 *
 * @return String
 **/
std::string InitializerListTest::getVectorInitializer(GLuint column, GLuint size)
{
	std::string result;

	result.append("{");
	result.append(getVectorValues(column, size));
	result.append("}");

	return result;
}

/** Get string representing sum of vector array. Token INDEX in name will be replaced with element index.
 *
 * @param array_name Name of array variable
 * @param columns    Number of columns to sum
 * @param size       Size of vector
 *
 * @return String
 **/
std::string InitializerListTest::getVectorArraySum(const GLchar* array_name, GLuint columns, GLuint size)
{
	static const GLchar* lut[] = { "0", "1", "2", "3" };

	std::string sum;

	for (GLuint i = 0; i < columns; ++i)
	{
		size_t		position = 0;
		std::string name	 = array_name;

		Utils::replaceToken("INDEX", position, lut[i], name);

		sum.append(getVectorSum(name.c_str(), size));

		if (i + 1 < columns)
		{
			sum.append(" + ");
		}
	}

	return sum;
}

/** Get string representing sum of vectors' elements
 *
 * @param vector_name Name of vector variable
 * @param size        Size of vector
 *
 * @return String
 **/
std::string InitializerListTest::getVectorSum(const GLchar* vector_name, GLuint size)
{
	static const GLchar* lut[] = {
		".x", ".y", ".z", ".w",
	};

	std::string sum;

	for (GLuint i = 0; i < size; ++i)
	{
		sum.append(vector_name);
		sum.append(lut[i]);

		if (i + 1 < size)
		{
			sum.append(" + ");
		}
	}

	return sum;
}

/** Get string representing vector values
 *
 * @param column Index of column of uni_matrix to use as data source
 * @param size   Size of vector
 *
 * @return String
 **/
std::string InitializerListTest::getVectorValues(GLuint column, GLuint size)
{
	const GLchar* init_template = 0;
	const GLchar* column_index  = 0;

	switch (size)
	{
	case 2:
		init_template = "uni_matrix[COLUMN].x, uni_matrix[COLUMN].y";
		break;
	case 3:
		init_template = "uni_matrix[COLUMN].x, uni_matrix[COLUMN].y, uni_matrix[COLUMN].z";
		break;
	case 4:
		init_template = "uni_matrix[COLUMN].z, uni_matrix[COLUMN].y, uni_matrix[COLUMN].z, uni_matrix[COLUMN].w";
		break;
	}

	switch (column)
	{
	case 0:
		column_index = "0";
		break;
	case 1:
		column_index = "1";
		break;
	case 2:
		column_index = "2";
		break;
	case 3:
		column_index = "3";
		break;
	}

	std::string initializer = init_template;

	Utils::replaceAllTokens("COLUMN", column_index, initializer);

	return initializer;
}

/** Constructor
 *
 * @param context Test context
 **/
InitializerListNegativeTest::InitializerListNegativeTest(deqp::Context& context)
	: NegativeTestBase(context, "initializer_list_negative", "Verifies invalid initializers")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool InitializerListNegativeTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		m_current_test_case_index = 0;
		return true;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	logTestCaseName();

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void InitializerListNegativeTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													  Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet = "    TYPE_NAME variable = INITIALIZATION;\n"
												"\n"
												"    float sum = SUM;\n"
												"\n"
												"    if (0 != sum)\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"TYPE_DEFINITION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"TYPE_DEFINITION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"TYPE_DEFINITION\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result = result;\n"
													"    gl_Position  = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"TYPE_DEFINITION\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "TYPE_DEFINITION\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "TYPE_DEFINITION\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const std::string& initialization  = getInitialization();
	const GLchar*	  shader_template = 0;
	const std::string& sum			   = getSum();
	const std::string& type_definition = getTypeDefinition();
	const std::string& type_name	   = getTypeName();

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("TYPE_DEFINITION", position, type_definition.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	position -= strlen(verification_snippet);

	Utils::replaceToken("TYPE_NAME", position, type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("INITIALIZATION", position, initialization.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("SUM", position, sum.c_str(), out_source.m_parts[0].m_code);
}

/** Prepare test cases
 *
 * @return true
 **/
bool InitializerListNegativeTest::testInit()
{
	for (GLuint i = 0; i < TESTED_ERRORS_MAX; ++i)
	{
		const TESTED_ERRORS error = (TESTED_ERRORS)i;

		m_test_cases.push_back(error);
	}

	return true;
}

/** Get string representing initialization list for current test case
 *
 * @return String
 **/
std::string InitializerListNegativeTest::getInitialization()
{
	const TESTED_ERRORS& error = m_test_cases[m_current_test_case_index];

	std::string initialization;

	switch (error)
	{
	case TYPE_UIVEC_BOOL:
		initialization = "{ true, 0, 1, 2 }";

		break;

	case TYPE_IVEC_BOOL:
		initialization = "{ true, 0, -1, 2 }";

		break;

	case TYPE_VEC_BOOL:
		initialization = "{ true, 0.125, 0.25, 0.375 }";

		break;

	case TYPE_MAT_BOOL:
		initialization = "{ {false, 0, 1, 1}, {0, 1, 0, 1}, {1, 0, 1, 0}, {0, 1, 0, 1} }";

		break;

	case COMPONENTS_VEC_LESS:
	case COMPONENTS_VEC_MORE:
		initialization = "{ 0, 0.25, 0.375 }";

		break;

	case COMPONENTS_MAT_LESS_ROWS:
		initialization = "{ {0, 0, 1, 1}, {0, 0, 1}, {1, 0, 1, 0}, {0, 1, 0, 1} }";

		break;

	case COMPONENTS_MAT_MORE_ROWS:
		initialization = "{ {0, 0, 1}, {0, 0, 1}, {1, 0, 1, 0}, {1, 0, 1} }";

		break;

	case COMPONENTS_MAT_LESS_COLUMNS:
		initialization = "{ {0, 0, 1, 1}, {1, 0, 1, 0}, {0, 1, 0, 1} }";

		break;

	case COMPONENTS_MAT_MORE_COLUMNS:
		initialization = "{ {0, 0, 1}, {0, 0, 1}, {1, 0, 1}, {1, 0, 1} }";

		break;

	case LIST_IN_CONSTRUCTOR:
		initialization = "Struct( { vec4(0, 1, 0, 1), vec3(0, 1, 0) }, vec4(1, 0, 1, 0) )";

		break;

	case STRUCT_LAYOUT_MEMBER_TYPE:
		initialization = "{ { {0, 1, 0, 1}, vec4(0, 1, 0, 1) }, vec4(1, 0, 1, 0) }";

		break;

	case STRUCT_LAYOUT_MEMBER_COUNT_MORE:
		initialization = "{ { {0, 1, 0, 1}, vec3(0, 1, 0) } , vec4(1, 0, 1, 0), vec4(1, 0, 1, 0) }";

		break;

	case STRUCT_LAYOUT_MEMBER_COUNT_LESS:
		initialization = "{ { {0, 1, 0, 1}, vec3(0, 1, 0) } }";

		break;

	case STRUCT_LAYOUT_MEMBER_ORDER:
		initialization = "{ vec4(1, 0, 1, 0), { vec3(0, 1, 0) , {0, 1, 0, 1} } }";

		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return initialization;
}

/** Logs description of current test case
 *
 **/
void InitializerListNegativeTest::logTestCaseName()
{
	const TESTED_ERRORS& error = m_test_cases[m_current_test_case_index];

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	switch (error)
	{
	case TYPE_UIVEC_BOOL:
		message << "Wrong type in uvec initializer list";
		break;
	case TYPE_IVEC_BOOL:
		message << "Wrong type in ivec initializer list";
		break;
	case TYPE_VEC_BOOL:
		message << "Wrong type in vec initializer list";
		break;
	case TYPE_MAT_BOOL:
		message << "Wrong type in mat initializer list";
		break;
	case COMPONENTS_VEC_LESS:
		message << "Wrong number of componenets in vec initialize list - less";
		break;
	case COMPONENTS_VEC_MORE:
		message << "Wrong number of componenets in vec initialize list - more";
		break;
	case COMPONENTS_MAT_LESS_ROWS:
		message << "Wrong number of componenets in mat initialize list - less rows";
		break;
	case COMPONENTS_MAT_LESS_COLUMNS:
		message << "Wrong number of componenets in mat initialize list - less columns";
		break;
	case COMPONENTS_MAT_MORE_ROWS:
		message << "Wrong number of componenets in mat initialize list - more rows";
		break;
	case COMPONENTS_MAT_MORE_COLUMNS:
		message << "Wrong number of componenets in mat initialize list - more columns";
		break;
	case LIST_IN_CONSTRUCTOR:
		message << "Initializer list in constructor";
		break;
	case STRUCT_LAYOUT_MEMBER_TYPE:
		message << "Wrong type of structure member";
		break;
	case STRUCT_LAYOUT_MEMBER_COUNT_MORE:
		message << "Wrong number of structure members - more";
		break;
	case STRUCT_LAYOUT_MEMBER_COUNT_LESS:
		message << "Wrong number of structure members - less";
		break;
	case STRUCT_LAYOUT_MEMBER_ORDER:
		message << "Wrong order of structure members";
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	message << tcu::TestLog::EndMessage;
}

/** Get string representing sum for current test case
 *
 * @return String
 **/
std::string InitializerListNegativeTest::getSum()
{
	const TESTED_ERRORS& error = m_test_cases[m_current_test_case_index];

	std::string sum;

	switch (error)
	{
	case TYPE_UIVEC_BOOL:
	case TYPE_IVEC_BOOL:
	case TYPE_VEC_BOOL:
	case COMPONENTS_VEC_LESS:
		sum = "variable.x + variable.y + variable.z + variable.w";
		break;
	case TYPE_MAT_BOOL:
	case COMPONENTS_MAT_LESS_ROWS:
	case COMPONENTS_MAT_LESS_COLUMNS:
		sum = "variable[0].x + variable[0].y + variable[0].z + variable[0].w + "
			  "variable[1].x + variable[1].y + variable[1].z + variable[1].w + "
			  "variable[2].x + variable[2].y + variable[2].z + variable[2].w + "
			  "variable[3].x + variable[3].y + variable[3].z + variable[3].w";
		break;
	case COMPONENTS_VEC_MORE:
		sum = "variable.x + variable.y + variable.z";
		break;
	case COMPONENTS_MAT_MORE_ROWS:
	case COMPONENTS_MAT_MORE_COLUMNS:
		sum = "variable[0].x + variable[0].y + variable[0].z"
			  "variable[1].x + variable[1].y + variable[1].z"
			  "variable[2].x + variable[2].y + variable[2].z";
		break;
	case LIST_IN_CONSTRUCTOR:
	case STRUCT_LAYOUT_MEMBER_TYPE:
	case STRUCT_LAYOUT_MEMBER_COUNT_MORE:
	case STRUCT_LAYOUT_MEMBER_COUNT_LESS:
	case STRUCT_LAYOUT_MEMBER_ORDER:
		sum = "variable.member_a.member_a.x + variable.member_a.member_a.y + variable.member_a.member_a.z + "
			  "variable.member_a.member_a.w + "
			  "variable.member_a.member_b.x + variable.member_a.member_b.y + variable.member_a.member_b.z + "
			  "variable.member_b.x + variable.member_b.y + variable.member_b.z + variable.member_b.w";
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return sum;
}

/** Get string representing types definition for current test case
 *
 * @return String
 **/
std::string InitializerListNegativeTest::getTypeDefinition()
{
	const TESTED_ERRORS& error = m_test_cases[m_current_test_case_index];

	static const GLchar* struct_def = "struct BasicStructure {\n"
									  "    vec4 member_a;\n"
									  "    vec3 member_b;\n"
									  "};\n"
									  "\n"
									  "struct StructureWithStructure {\n"
									  "    BasicStructure member_a;\n"
									  "    vec4           member_b;\n"
									  "};\n";

	std::string type_definition;

	switch (error)
	{
	case TYPE_UIVEC_BOOL:
	case TYPE_IVEC_BOOL:
	case TYPE_VEC_BOOL:
	case TYPE_MAT_BOOL:
	case COMPONENTS_VEC_LESS:
	case COMPONENTS_VEC_MORE:
	case COMPONENTS_MAT_LESS_ROWS:
	case COMPONENTS_MAT_LESS_COLUMNS:
	case COMPONENTS_MAT_MORE_ROWS:
	case COMPONENTS_MAT_MORE_COLUMNS:
		type_definition = "";
		break;
	case LIST_IN_CONSTRUCTOR:
	case STRUCT_LAYOUT_MEMBER_TYPE:
	case STRUCT_LAYOUT_MEMBER_COUNT_MORE:
	case STRUCT_LAYOUT_MEMBER_COUNT_LESS:
	case STRUCT_LAYOUT_MEMBER_ORDER:
		type_definition = struct_def;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return type_definition;
}

/** Get string representing name of variable's type for current test case
 *
 * @return String
 **/
std::string InitializerListNegativeTest::getTypeName()
{
	const TESTED_ERRORS& error = m_test_cases[m_current_test_case_index];

	static const GLchar* struct_with_struct = "StructureWithStructure";

	std::string type_name;

	switch (error)
	{
	case TYPE_UIVEC_BOOL:
		type_name = "uvec4";
		break;
	case TYPE_IVEC_BOOL:
		type_name = "ivec4";
		break;
	case TYPE_VEC_BOOL:
	case COMPONENTS_VEC_LESS:
		type_name = "vec4";
		break;
	case COMPONENTS_VEC_MORE:
		type_name = "vec2";
		break;
	case TYPE_MAT_BOOL:
	case COMPONENTS_MAT_LESS_ROWS:
	case COMPONENTS_MAT_LESS_COLUMNS:
		type_name = "mat4";
		break;
	case COMPONENTS_MAT_MORE_ROWS:
	case COMPONENTS_MAT_MORE_COLUMNS:
		type_name = "mat3";
		break;
		break;
	case LIST_IN_CONSTRUCTOR:
	case STRUCT_LAYOUT_MEMBER_TYPE:
	case STRUCT_LAYOUT_MEMBER_COUNT_MORE:
	case STRUCT_LAYOUT_MEMBER_COUNT_LESS:
	case STRUCT_LAYOUT_MEMBER_ORDER:
		type_name = struct_with_struct;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return type_name;
}

/** Constructor
 *
 * @param context Test context
 **/
LengthOfVectorAndMatrixTest::LengthOfVectorAndMatrixTest(deqp::Context& context)
	: GLSLTestBase(context, "length_of_vector_and_matrix", "Test verifies .length() for vectors and matrices")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool LengthOfVectorAndMatrixTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	m_current_test_case_index = test_case_index;

	if ((glw::GLuint)-1 == test_case_index)
	{
		m_current_test_case_index = 0;
		return true;
	}
	else if (m_test_cases.size() <= test_case_index)
	{
		return false;
	}

	const testCase& test_case = m_test_cases[m_current_test_case_index];

	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Tested type: "
										<< Utils::getTypeName(test_case.m_type, test_case.m_n_cols, test_case.m_n_rows)
										<< tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void LengthOfVectorAndMatrixTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													  Utils::shaderSource& out_source)
{
	if (Utils::COMPUTE_SHADER == in_stage)
	{
		m_is_compute_program = true;
		prepareComputeShaderSource(out_source);
	}
	else
	{
		m_is_compute_program = false;
		prepareDrawShaderSource(in_stage, in_use_version_400, out_source);
	}
}

/** Overwritte of prepareUniforms method
 *
 * @param program Current program
 **/
void LengthOfVectorAndMatrixTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat float_value = 0.125;
	static const GLint   int_value   = -1;
	static const GLuint  uint_value  = 1;

	static const GLfloat float_data[16] = { float_value, float_value, float_value, float_value,
											float_value, float_value, float_value, float_value,
											float_value, float_value, float_value, float_value,
											float_value, float_value, float_value, float_value };

	static const GLint int_data[4] = { int_value, int_value, int_value, int_value };

	static const GLuint uint_data[4] = { uint_value, uint_value, uint_value, uint_value };

	if (false == m_is_compute_program)
	{
		return;
	}

	const testCase& test_case = m_test_cases[m_current_test_case_index];

	switch (test_case.m_type)
	{
	case Utils::FLOAT:
		program.uniform("uni_variable", Utils::FLOAT, test_case.m_n_cols, test_case.m_n_rows, float_data);
		break;
	case Utils::INT:
		program.uniform("uni_variable", Utils::INT, 1 /* columns */, test_case.m_n_rows, int_data);
		break;
	case Utils::UINT:
		program.uniform("uni_variable", Utils::UINT, 1 /* columns */, test_case.m_n_rows, uint_data);
		break;
	default:
		TCU_FAIL("Invalid enum");
	}
}

/** Prepare vertex buffer
 *
 * @param program Program object
 * @param buffer  Vertex buffer
 * @param vao     Vertex array object
 *
 * @return 0
 **/
void LengthOfVectorAndMatrixTest::prepareVertexBuffer(const Utils::program& program, Utils::buffer& buffer,
													  Utils::vertexArray& vao)
{
	static const GLfloat float_value = 0.125f;
	static const GLint   int_value   = -1;
	static const GLuint  uint_value  = 1;

	static const GLfloat float_data[16] = { float_value, float_value, float_value, float_value,
											float_value, float_value, float_value, float_value,
											float_value, float_value, float_value, float_value,
											float_value, float_value, float_value, float_value };

	static const GLint int_data[4] = { int_value, int_value, int_value, int_value };

	static const GLuint uint_data[4] = { uint_value, uint_value, uint_value, uint_value };

	const testCase& test_case = m_test_cases[m_current_test_case_index];

	std::string variable_name = Utils::getVariableName(Utils::VERTEX_SHADER, Utils::INPUT, "variable");
	GLint		variable_loc  = program.getAttribLocation(variable_name.c_str());

	if (-1 == variable_loc)
	{
		TCU_FAIL("Vertex attribute location is invalid");
	}

	vao.generate();
	vao.bind();

	buffer.generate(GL_ARRAY_BUFFER);

	GLvoid*	data_ptr  = 0;
	GLsizeiptr data_size = 0;

	switch (test_case.m_type)
	{
	case Utils::FLOAT:
		data_ptr  = (GLvoid*)float_data;
		data_size = sizeof(float_data);
		break;
	case Utils::INT:
		data_ptr  = (GLvoid*)int_data;
		data_size = sizeof(int_data);
		break;
	case Utils::UINT:
		data_ptr  = (GLvoid*)uint_data;
		data_size = sizeof(uint_data);
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	buffer.update(data_size, data_ptr, GL_STATIC_DRAW);

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up vao */
	switch (test_case.m_type)
	{
	case Utils::FLOAT:
		for (GLuint col = 0; col < test_case.m_n_cols; ++col)
		{
			const GLuint  index  = variable_loc + col;
			const GLint   size   = test_case.m_n_rows;
			const GLvoid* offset = (const GLvoid*)(test_case.m_n_rows * sizeof(GLfloat) * col);

			gl.vertexAttribPointer(index, size, GL_FLOAT /* type */, GL_FALSE /* normalized*/, 0, offset);
			GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribPointer");
		}
		break;
	case Utils::INT:
		gl.vertexAttribIPointer(variable_loc, test_case.m_n_rows /* size */, GL_INT /* type */, 0 /* stride */,
								0 /* offset */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribIPointer");
		break;
	case Utils::UINT:
		gl.vertexAttribIPointer(variable_loc, test_case.m_n_rows /* size */, GL_UNSIGNED_INT /* type */, 0 /* stride */,
								0 /* offset */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribIPointer");
		break;
	default:
		DE_ASSERT(0);
		break;
	}

	/* Enable attribute */
	for (GLuint col = 0; col < test_case.m_n_cols; ++col)
	{
		gl.enableVertexAttribArray(variable_loc + col);
		GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");
	}
}

/** Prepare test cases
 *
 * @return true
 **/
bool LengthOfVectorAndMatrixTest::testInit()
{
	/* Vectors */
	for (GLuint row = 2; row <= 4; ++row)
	{
		testCase test_case = { Utils::UINT, 1 /* n_cols */, row };

		m_test_cases.push_back(test_case);
	}

	for (GLuint row = 2; row <= 4; ++row)
	{
		testCase test_case = { Utils::INT, 1 /* n_cols */, row };

		m_test_cases.push_back(test_case);
	}

	for (GLuint row = 2; row <= 4; ++row)
	{
		testCase test_case = { Utils::FLOAT, 1 /* n_cols */, row };

		m_test_cases.push_back(test_case);
	}

	/* Matrices */
	for (GLuint col = 2; col <= 4; ++col)
	{
		for (GLuint row = 2; row <= 4; ++row)
		{
			testCase test_case = { Utils::FLOAT, col, row };

			m_test_cases.push_back(test_case);
		}
	}

	return true;
}

/** Get string representing value that should be placed at token EXPECTED_VALUE
 *
 * @param in_stage Shader stage
 *
 * @return String with value
 **/
std::string LengthOfVectorAndMatrixTest::getExpectedValue(Utils::SHADER_STAGES in_stage)
{
	const testCase& test_case = m_test_cases[m_current_test_case_index];

	GLuint count = 0;

	switch (in_stage)
	{
	case Utils::FRAGMENT_SHADER:
		count = 3;
		break;
	case Utils::COMPUTE_SHADER:
		count = 2;
		break;
	default:
		count = 4;
	}

	if (1 == test_case.m_n_cols)
	{
		count *= test_case.m_n_rows;
	}
	else
	{
		count *= test_case.m_n_cols;
	}

	std::string expected_value;
	expected_value.resize(64, 0);

	switch (test_case.m_type)
	{
	case Utils::FLOAT:
	{
		GLfloat value = 0.125f * (GLfloat)count;
		sprintf(&expected_value[0], "%f", value);
	}
	break;
	case Utils::INT:
	{
		GLint value = -1 * (GLint)count;
		sprintf(&expected_value[0], "%d", value);
	}
	break;
	case Utils::UINT:
	{
		GLuint value = 1 * (GLuint)count;
		sprintf(&expected_value[0], "%d", value);
	}
	break;
	default:
		DE_ASSERT(0);
		break;
	}

	return expected_value;
}

/** Get string reresenting initialization of local variables for current test case
 *
 * @return String with initialization
 **/
std::string LengthOfVectorAndMatrixTest::getInitialization()
{
	const testCase& test_case = m_test_cases[m_current_test_case_index];

	std::string initialization;

	if (1 == test_case.m_n_cols)
	{
		initialization = getVectorInitializer(test_case.m_type, test_case.m_n_rows);
	}
	else
	{
		initialization = getMatrixInitializer(test_case.m_n_cols, test_case.m_n_rows);
	}

	return initialization;
}

/** Get string reresenting initialization of local matrix variables
 *
 * @param n_cols Number of columns
 * @param n_rows Number of rows
 *
 * @return String with initialization
 **/
std::string LengthOfVectorAndMatrixTest::getMatrixInitializer(GLuint n_cols, GLuint n_rows)
{
	std::string result;

	result.append("{ ");

	for (GLuint col = 0; col < n_cols; ++col)
	{
		result.append(getVectorInitializer(Utils::FLOAT, n_rows));

		if (col + 1 < n_cols)
		{
			result.append(", ");
		}
	}

	result.append(" }");

	return result;
}

/** Get string reresenting initialization of local vector variables
 *
 * @param type   Basic type of vector
 * @param n_rows Number of rows
 *
 * @return String with initialization
 **/
std::string LengthOfVectorAndMatrixTest::getVectorInitializer(Utils::TYPES type, glw::GLuint n_rows)
{
	std::string   result;
	const GLchar* value = 0;

	switch (type)
	{
	case Utils::FLOAT:
		value = "0.125";
		break;
	case Utils::UINT:
		value = "1";
		break;
	case Utils::INT:
		value = "-1";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	result.append("{");

	for (GLuint row = 0; row < n_rows; ++row)
	{
		result.append(value);

		if (row + 1 < n_rows)
		{
			result.append(", ");
		}
	}

	result.append("}");

	return result;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void LengthOfVectorAndMatrixTest::prepareDrawShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
														  Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet =
		"    VARIABLE_TYPE variable  = INITIALIZATION;\n"
		"    Structure     structure = { { 0, 1, 0, 1 } , INITIALIZATION };\n"
		"\n"
		"    const uint variable_length           = variable.length();\n"
		"    const uint structure_member_b_length = structure.member_b.length();\n"
		"    const uint input_member_length       = INPUT_VARIABLE_NAME.length();\n"
		"#ifndef FRAGMENT\n"
		"    const uint output_member_length      = OUTPUT_VARIABLE_NAME.length();\n"
		"#endif // FRAGMENT\n"
		"\n"
		"    BASE_TYPE array_var[variable.length()];\n"
		"    BASE_TYPE array_str[structure.member_b.length()];\n"
		"    BASE_TYPE array_in [INPUT_VARIABLE_NAME.length()];\n"
		"#ifndef FRAGMENT\n"
		"    BASE_TYPE array_out[OUTPUT_VARIABLE_NAME.length()];\n"
		"#endif // FRAGMENT\n"
		"\n"
		"    BASE_TYPE sum = 0;\n"
		"\n"
		"    for (uint i = 0; i < variable_length; ++i)\n"
		"    {\n"
		"        array_var[i] = variableARRAY_INDEX.x;\n"
		"    }\n"
		"\n"
		"    for (uint i = 0; i < structure_member_b_length; ++i)\n"
		"    {\n"
		"        array_str[i] = structure.member_bARRAY_INDEX.y;\n"
		"    }\n"
		"\n"
		"    for (uint i = 0; i < input_member_length; ++i)\n"
		"    {\n"
		"        array_in[i]  = INPUT_VARIABLE_NAMEARRAY_INDEX.x;\n"
		"    }\n"
		"\n"
		"#ifndef FRAGMENT\n"
		"    for (uint i = 0; i < output_member_length; ++i)\n"
		"    {\n"
		"        array_out[i] = INPUT_VARIABLE_NAMEARRAY_INDEX.y;\n"
		"    }\n"
		"#endif // FRAGMENT\n"
		"\n"
		"    for (uint i = 0; i < array_var.length(); ++i)\n"
		"    {\n"
		"         sum += array_var[i];\n"
		"    }\n"
		"\n"
		"    for (uint i = 0; i < array_str.length(); ++i)\n"
		"    {\n"
		"         sum += array_str[i];\n"
		"    }\n"
		"\n"
		"    for (uint i = 0; i < array_in.length(); ++i)\n"
		"    {\n"
		"         sum += array_in[i];\n"
		"    }\n"
		"\n"
		"#ifndef FRAGMENT\n"
		"    for (uint i = 0; i < array_out.length(); ++i)\n"
		"    {\n"
		"         sum += array_out[i];\n"
		"    }\n"
		"#endif // FRAGMENT\n"
		"\n"
		"    if (EXPECTED_VALUE != sum)\n"
		"    {\n"
		"        result = vec4(1, 0, 0, 1);\n"
		"    }\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"#define FRAGMENT\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"in GSOutputBlock {\n"
													"    VARIABLE_DECLARATION;\n"
													"} input_block;\n"
													"\n"
													"struct Structure {\n"
													"    vec4 member_a;\n"
													"    TYPE_NAME member_b;\n"
													"};\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"in TCSOutputBlock {\n"
													"    VARIABLE_DECLARATION;\n"
													"} input_block[];\n"
													"out GSOutputBlock {\n"
													"    VARIABLE_DECLARATION;\n"
													"} output_block;\n"
													"\n"
													"struct Structure {\n"
													"    vec4 member_a;\n"
													"    TYPE_NAME member_b;\n"
													"};\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, -1, 0, 1);\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, 1, 0, 1);\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, -1, 0, 1);\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, 1, 0, 1);\n"
													"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"in VSOutputBlock {\n"
		"    VARIABLE_DECLARATION;\n"
		"} input_block[];\n"
		"out TCSOutputBlock {\n"
		"    VARIABLE_DECLARATION;\n"
		"} output_block[];\n"
		"\n"
		"struct Structure {\n"
		"    vec4 member_a;\n"
		"    TYPE_NAME member_b;\n"
		"};\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "in TCSOutputBlock {\n"
													 "    VARIABLE_DECLARATION;\n"
													 "} input_block[];\n"
													 "out TCSOutputBlock {\n"
													 "    VARIABLE_DECLARATION;\n"
													 "} output_block;\n"
													 "\n"
													 "struct Structure {\n"
													 "    vec4 member_a;\n"
													 "    TYPE_NAME member_b;\n"
													 "};\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "in VARIABLE_DECLARATION;\n"
												  "out VSOutputBlock {\n"
												  "    VARIABLE_DECLARATION;\n"
												  "} output_block;\n"
												  "\n"
												  "struct Structure {\n"
												  "    vec4 member_a;\n"
												  "    TYPE_NAME member_b;\n"
												  "};\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "    OUTPUT_VARIABLE_NAME = INPUT_VARIABLE_NAME;\n"
												  "}\n"
												  "\n";

	const GLchar*   array_index		  = "";
	const testCase& test_case		  = m_test_cases[m_current_test_case_index];
	const GLchar*   shader_template   = 0;
	const GLchar*   input_block_name  = "input_block";
	const GLchar*   output_block_name = "output_block";

	const std::string& base_type_name = Utils::getTypeName(test_case.m_type, 1 /* cols */, 1 /* rows */);
	const std::string& expected_value = getExpectedValue(in_stage);
	const std::string& initialization = getInitialization();
	const std::string& type_name	  = Utils::getTypeName(test_case.m_type, test_case.m_n_cols, test_case.m_n_rows);

	std::string input_decl;
	std::string input_ref;
	std::string output_decl;
	std::string output_ref;

	Utils::qualifierSet in_qualifiers;
	Utils::qualifierSet out_qualifiers;

	if ((Utils::UINT == test_case.m_type) || (Utils::INT == test_case.m_type))
	{
		if (Utils::VERTEX_SHADER != in_stage)
		{
			in_qualifiers.push_back(Utils::QUAL_FLAT);
		}

		out_qualifiers.push_back(Utils::QUAL_FLAT);
	}

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = 0;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	if (Utils::VERTEX_SHADER != in_stage)
	{
		Utils::prepareBlockVariableStrings(in_stage, Utils::INPUT, in_qualifiers, type_name.c_str(), "variable",
										   input_block_name, input_decl, input_ref);
	}
	else
	{
		Utils::prepareVariableStrings(in_stage, Utils::INPUT, in_qualifiers, type_name.c_str(), "variable", input_decl,
									  input_ref);
	}
	if (Utils::FRAGMENT_SHADER != in_stage)
	{
		Utils::prepareBlockVariableStrings(in_stage, Utils::OUTPUT, out_qualifiers, type_name.c_str(), "variable",
										   output_block_name, output_decl, output_ref);
	}
	else
	{
		Utils::prepareVariableStrings(in_stage, Utils::OUTPUT, out_qualifiers, type_name.c_str(), "variable",
									  output_decl, output_ref);
	}

	if (1 != test_case.m_n_cols)
	{
		array_index = "[i]";
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VARIABLE_DECLARATION", position, input_decl.c_str(), out_source.m_parts[0].m_code);

	if (Utils::FRAGMENT_SHADER != in_stage)
	{
		Utils::replaceToken("VARIABLE_DECLARATION", position, output_decl.c_str(), out_source.m_parts[0].m_code);
	}

	Utils::replaceToken("TYPE_NAME", position, type_name.c_str(), out_source.m_parts[0].m_code);

	size_t temp = position;

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	position = temp;

	Utils::replaceToken("VARIABLE_TYPE", position, type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("INITIALIZATION", position, initialization.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("INITIALIZATION", position, initialization.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("BASE_TYPE", position, base_type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("BASE_TYPE", position, base_type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("BASE_TYPE", position, base_type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("BASE_TYPE", position, base_type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("BASE_TYPE", position, base_type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("EXPECTED_VALUE", position, expected_value.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("INPUT_VARIABLE_NAME", input_ref.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("OUTPUT_VARIABLE_NAME", output_ref.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceAllTokens("ARRAY_INDEX", array_index, out_source.m_parts[0].m_code);
}

/** Prepare source for compute shader stage
 *
 * @param out_source Prepared shader source instance
 **/
void LengthOfVectorAndMatrixTest::prepareComputeShaderSource(Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet =
		"    VARIABLE_TYPE variable  = uni_variable;\n"
		"    Structure     structure = { { 0, 1, 0, 1 } , uni_variable };\n"
		"\n"
		"    const uint variable_length           = variable.length();\n"
		"    const uint structure_member_b_length = structure.member_b.length();\n"
		"\n"
		"    BASE_TYPE array_var[variable.length()];\n"
		"    BASE_TYPE array_str[structure.member_b.length()];\n"
		"\n"
		"    BASE_TYPE sum = 0;\n"
		"\n"
		"    for (uint i = 0; i < variable_length; ++i)\n"
		"    {\n"
		"        array_var[i] = variableARRAY_INDEX.x;\n"
		"    }\n"
		"\n"
		"    for (uint i = 0; i < structure_member_b_length; ++i)\n"
		"    {\n"
		"        array_str[i] = structure.member_bARRAY_INDEX.y;\n"
		"    }\n"
		"\n"
		"    for (uint i = 0; i < array_var.length(); ++i)\n"
		"    {\n"
		"         sum += array_var[i];\n"
		"    }\n"
		"\n"
		"    for (uint i = 0; i < array_str.length(); ++i)\n"
		"    {\n"
		"         sum += array_str[i];\n"
		"    }\n"
		"\n"
		"    if (EXPECTED_VALUE != sum)\n"
		"    {\n"
		"        result = vec4(1, 0, 0, 1);\n"
		"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform TYPE_NAME    uni_variable;\n"
		"\n"
		"struct Structure {\n"
		"    vec4 member_a;\n"
		"    TYPE_NAME member_b;\n"
		"};\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	const testCase& test_case   = m_test_cases[m_current_test_case_index];
	const GLchar*   array_index = "";

	const std::string& base_type_name = Utils::getTypeName(test_case.m_type, 1 /* cols */, 1 /* rows */);
	const std::string& expected_value = getExpectedValue(Utils::COMPUTE_SHADER);
	const std::string& type_name	  = Utils::getTypeName(test_case.m_type, test_case.m_n_cols, test_case.m_n_rows);

	if (1 != test_case.m_n_cols)
	{
		array_index = "[i]";
	}

	out_source.m_parts[0].m_code = compute_shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(Utils::COMPUTE_SHADER, false),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("TYPE_NAME", position, type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("TYPE_NAME", position, type_name.c_str(), out_source.m_parts[0].m_code);

	size_t temp = position;

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);

	position = temp;

	Utils::replaceToken("VARIABLE_TYPE", position, type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("BASE_TYPE", position, base_type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("BASE_TYPE", position, base_type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("BASE_TYPE", position, base_type_name.c_str(), out_source.m_parts[0].m_code);

	Utils::replaceToken("ARRAY_INDEX", position, array_index, out_source.m_parts[0].m_code);

	Utils::replaceToken("ARRAY_INDEX", position, array_index, out_source.m_parts[0].m_code);

	Utils::replaceToken("EXPECTED_VALUE", position, expected_value.c_str(), out_source.m_parts[0].m_code);
}

/** Constructor
 *
 * @param context Test context
 **/
LengthOfComputeResultTest::LengthOfComputeResultTest(deqp::Context& context)
	: GLSLTestBase(context, "length_of_compute_result", "Test verifies .length() for results of computation")
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void LengthOfComputeResultTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													Utils::shaderSource& out_source)
{
	static const GLchar* uniforms = "uniform mat2x4 goten;\n"
									"uniform uvec4  indices;\n"
									"uniform uvec4  expected_lengths;\n"
									"uniform mat4x3 gohan;\n"
									"uniform vec3   vegeta;\n"
									"uniform vec3   trunks;\n"
									"uniform uint   variable;\n"
									"uniform float  expected_sum;\n";

	static const GLchar* verification_snippet =
		"    uint lengths[4];\n"
		"    float x[(gohan * goten).length()];\n"
		"    float y[(gohan * goten)[variable - 1].length()];\n"
		"\n"
		"    lengths[indices.x] = gohan[variable].length();\n"
		"    lengths[indices.y] = (gohan * goten).length();\n"
		"    lengths[indices.z] = (gohan * goten)[variable].length();\n"
		"    lengths[indices.w] = (vegeta * trunks).length();\n"
		"\n"
		"    float  dot_result = dot(vegeta, trunks);\n"
		"    mat2x3 mul_result = gohan * goten;\n"
		"\n"
		"#ifdef TESS_CTRL\n"
		"    const uint position_length        = gl_out[gl_InvocationID].gl_Position.length();\n"
		"#endif\n"
		"#ifndef COMPUTE\n"
		"#ifndef FRAGMENT\n"
		"#ifndef TESS_CTRL\n"
		"    const uint position_length        = gl_Position.length();\n"
		"#endif  /*TESS_CTRL */\n"
		"#endif /* FRAGMENT */\n"
		"#endif /* COMPUTE */\n"
		"#ifdef FRAGMENT\n"
		"    const uint point_coord_length     = gl_PointCoord.length();\n"
		"    const uint sample_position_length = gl_SamplePosition.length();\n"
		"#endif /* FRAGMENT */\n"
		"    const uint outer_length           = outerProduct(vegeta, trunks).length();\n"
		"\n"
		"    for (uint i = 0; i < x.length(); ++i)\n"
		"    {\n"
		"        x[i] = mul_result[i].x;\n"
		"    }\n"
		"\n"
		"    for (uint i = 0; i < y.length(); ++i)\n"
		"    {\n"
		"        y[i] = mul_result[0][i];\n"
		"    }\n"
		"\n"
		"    if ( (expected_lengths.x != lengths[0])                   ||\n"
		"         (expected_lengths.y != lengths[1])                   ||\n"
		"         (expected_lengths.z != lengths[2])                   ||\n"
		"         (expected_lengths.w != lengths[3])                   ||\n"
		"#ifndef COMPUTE\n"
		"#ifndef FRAGMENT\n"
		"         (4 /* vec4 */       != position_length)              ||\n"
		"#endif /* FRAGMENT */\n"
		"#endif /* COMPUTE */\n"
		"#ifdef FRAGMENT\n"
		"         (2 /* vec2 */       != point_coord_length)           ||\n"
		"         (2 /* vec2 */       != sample_position_length)       ||\n"
		"#endif /* FRAGMENT */\n"
		"         (0.5                != dot_result)                   ||\n"
		"         (3 /* mat3 */       != outer_length)                 ||\n"
		"         (expected_sum       != x[variable] + y[variable])    )\n"
		"    {\n"
		"        result = vec4(1, 0, 0, 1);\n"
		"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"#define COMPUTE\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNIFORMS"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"#define FRAGMENT\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNIFORMS"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNIFORMS"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"#define TESS_CTRL\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNIFORMS"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNIFORMS"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNIFORMS"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("UNIFORMS", position, uniforms, out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method
 *
 * @param program Current program
 **/
void LengthOfComputeResultTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat gohan_data[12] = { 0.125f, 0.125f, 0.125f, 0.125f, 0.125f, 0.125f,
											0.125f, 0.125f, 0.125f, 0.125f, 0.125f, 0.125f };

	static const GLfloat goten_data[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

	static const GLfloat vegeta_data[3] = { 0.5f, 0.5f, 0.0f };

	static const GLfloat trunks_data[3] = { 0.5f, 0.5f, 0.0f };

	static const GLuint indices_data[4] = { 2, 1, 0, 3 };

	static const GLuint variable_data[1] = { 1 };

	static const GLuint expected_lengths_data[4] = { 3, 2, 3, 3 };

	static const GLfloat expected_sum_data[1] = { 1.0f };

	program.uniform("gohan", Utils::FLOAT, 4 /* n_cols */, 3 /* n_rows */, gohan_data);
	program.uniform("goten", Utils::FLOAT, 2 /* n_cols */, 4 /* n_rows */, goten_data);
	program.uniform("vegeta", Utils::FLOAT, 1 /* n_cols */, 3 /* n_rows */, vegeta_data);
	program.uniform("trunks", Utils::FLOAT, 1 /* n_cols */, 3 /* n_rows */, trunks_data);
	program.uniform("indices", Utils::UINT, 1 /* n_cols */, 4 /* n_rows */, indices_data);
	program.uniform("variable", Utils::UINT, 1 /* n_cols */, 1 /* n_rows */, variable_data);
	program.uniform("expected_lengths", Utils::UINT, 1 /* n_cols */, 4 /* n_rows */, expected_lengths_data);
	program.uniform("expected_sum", Utils::FLOAT, 1 /* n_cols */, 1 /* n_rows */, expected_sum_data);
}

/** Constructor
 *
 * @param context Test context
 **/
ScalarSwizzlersTest::ScalarSwizzlersTest(deqp::Context& context)
	: GLSLTestBase(context, "scalar_swizzlers", "Verifies that swizzlers can be used on scalars")
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void ScalarSwizzlersTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											  Utils::shaderSource& out_source)
{
	static const GLchar* uniforms = "uniform float variable;\n"
									"uniform vec3  expected_values;\n";

	static const GLchar* literal = "#define LITERAL 0.375\n";

	static const GLchar* structure = "struct Structure {\n"
									 "    vec2 m_xx;\n"
									 "    vec3 m_xxx;\n"
									 "    vec4 m_xxxx;\n"
									 "    vec2 m_nested_xx;\n"
									 "    vec3 m_nested_xxx;\n"
									 "    vec4 m_nested_xxxx;\n"
									 "};\n";

	static const GLchar* function = "bool check_values(in Structure structure, in float value)\n"
									"{\n"
									"    const vec2 xx   = vec2(value, value);\n"
									"    const vec3 xxx  = vec3(value, value, value);\n"
									"    const vec4 xxxx = vec4(value, value, value, value);\n"
									"\n"
									"    bool result = true;\n"
									"\n"
									"    if ((xx   != structure.m_xx)         ||\n"
									"        (xxx  != structure.m_xxx)        ||\n"
									"        (xxxx != structure.m_xxxx)       ||\n"
									"        (xx   != structure.m_nested_xx)  ||\n"
									"        (xxx  != structure.m_nested_xxx) ||\n"
									"        (xxxx != structure.m_nested_xxxx) )\n"
									"    {\n"
									"        result = false;\n"
									"    }\n"
									"\n"
									"    return result;\n"
									"}\n";

	static const GLchar* verification_snippet =
		"    Structure literal_result;\n"
		"    Structure constant_result;\n"
		"    Structure variable_result;\n"
		"\n"
		"    literal_result.m_xx          = LITERAL.xx  ;\n"
		"    literal_result.m_xxx         = LITERAL.xxx ;\n"
		"    literal_result.m_xxxx        = LITERAL.xxxx;\n"
		"    literal_result.m_nested_xx   = LITERAL.x.rr.sss.rr  ;\n"
		"    literal_result.m_nested_xxx  = LITERAL.s.xx.rrr.xxx ;\n"
		"    literal_result.m_nested_xxxx = LITERAL.r.ss.xxx.ssss;\n"
		"\n"
		"    const float constant = 0.125;\n"
		"\n"
		"    constant_result.m_xx          = constant.xx  ;\n"
		"    constant_result.m_xxx         = constant.xxx ;\n"
		"    constant_result.m_xxxx        = constant.xxxx;\n"
		"    constant_result.m_nested_xx   = constant.x.rr.sss.rr  ;\n"
		"    constant_result.m_nested_xxx  = constant.s.xx.rrr.xxx ;\n"
		"    constant_result.m_nested_xxxx = constant.r.ss.xxx.ssss;\n"
		"\n"
		"    variable_result.m_xx          = variable.xx  ;\n"
		"    variable_result.m_xxx         = variable.xxx ;\n"
		"    variable_result.m_xxxx        = variable.xxxx;\n"
		"    variable_result.m_nested_xx   = variable.x.rr.sss.rr  ;\n"
		"    variable_result.m_nested_xxx  = variable.s.xx.rrr.xxx ;\n"
		"    variable_result.m_nested_xxxx = variable.r.ss.xxx.ssss;\n"
		"\n"
		"    if ((false == check_values(literal_result,  expected_values.x)) ||\n"
		"        (false == check_values(constant_result, expected_values.y)) ||\n"
		"        (false == check_values(variable_result, expected_values.z)) )\n"
		"    {\n"
		"        result = vec4(1, 0, 0, 1);\n"
		"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"STRUCTURE"
		"\n"
		"UNIFORMS"
		"\n"
		"FUNCTION"
		"\n"
		"LITERAL"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"#define FRAGMENT\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"STRUCTURE"
													"\n"
													"UNIFORMS"
													"\n"
													"FUNCTION"
													"\n"
													"LITERAL"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"STRUCTURE"
													"\n"
													"UNIFORMS"
													"\n"
													"FUNCTION"
													"\n"
													"LITERAL"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"STRUCTURE"
		"\n"
		"UNIFORMS"
		"\n"
		"FUNCTION"
		"\n"
		"LITERAL"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "STRUCTURE"
													 "\n"
													 "UNIFORMS"
													 "\n"
													 "FUNCTION"
													 "\n"
													 "LITERAL"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "STRUCTURE"
												  "\n"
												  "UNIFORMS"
												  "\n"
												  "FUNCTION"
												  "\n"
												  "LITERAL"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("STRUCTURE", position, structure, out_source.m_parts[0].m_code);

	Utils::replaceToken("UNIFORMS", position, uniforms, out_source.m_parts[0].m_code);

	Utils::replaceToken("FUNCTION", position, function, out_source.m_parts[0].m_code);

	Utils::replaceToken("LITERAL", position, literal, out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method
 *
 * @param program Current program
 **/
void ScalarSwizzlersTest::prepareUniforms(Utils::program& program)
{
	static const GLfloat variable_data[4]		 = { 0.75f };
	static const GLfloat expected_values_data[3] = { 0.375f, 0.125f, 0.75f };

	program.uniform("variable", Utils::FLOAT, 1 /* n_cols */, 1 /* n_rows */, variable_data);
	program.uniform("expected_values", Utils::FLOAT, 1 /* n_cols */, 3 /* n_rows */, expected_values_data);
}

/** Constructor
 *
 * @param context Test context
 **/
ScalarSwizzlersInvalidTest::ScalarSwizzlersInvalidTest(deqp::Context& context)
	: NegativeTestBase(context, "scalar_swizzlers_invalid",
					   "Verifies if invalid use of swizzlers on scalars is reported as error")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool ScalarSwizzlersInvalidTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	switch (test_case_index)
	{
	case (glw::GLuint)-1:
	case INVALID_Y:
	case INVALID_B:
	case INVALID_Q:
	case INVALID_XY:
	case INVALID_XRS:
	case WRONG:
	case MISSING_PARENTHESIS:
		m_case = (TESTED_CASES)test_case_index;
		break;
	default:
		return false;
	}

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void ScalarSwizzlersInvalidTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
													 Utils::shaderSource& out_source)
{
	static const GLchar* uniforms = "uniform float variable;\n";

	static const GLchar* verification_invalid_y = "\n"
												  "    if (0.125 != variable.y) )\n"
												  "    {\n"
												  "        result = vec4(1, 0, 0, 1);\n"
												  "    }\n";

	static const GLchar* verification_invalid_b = "\n"
												  "    if (0.125 != variable.b) )\n"
												  "    {\n"
												  "        result = vec4(1, 0, 0, 1);\n"
												  "    }\n";

	static const GLchar* verification_invalid_q = "\n"
												  "    if (0.125 != variable.q) )\n"
												  "    {\n"
												  "        result = vec4(1, 0, 0, 1);\n"
												  "    }\n";

	static const GLchar* verification_invalid_xy = "\n"
												   "    if (vec2(0.125, 0.25) != variable.xy) )\n"
												   "    {\n"
												   "        result = vec4(1, 0, 0, 1);\n"
												   "    }\n";

	static const GLchar* verification_invalid_xrs = "\n"
													"    if (vec3(0.125, 0.125, 0.25) != variable.xrs) )\n"
													"    {\n"
													"        result = vec4(1, 0, 0, 1);\n"
													"    }\n";

	static const GLchar* verification_wrong_u = "\n"
												"    if (0.125 != variable.u) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* verification_missing_parenthesis = "\n"
															"    if (variable != 1.x) )\n"
															"    {\n"
															"        result = vec4(1, 0, 0, 1);\n"
															"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"\n"
		"UNIFORMS"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"#define FRAGMENT\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"UNIFORMS"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"UNIFORMS"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"UNIFORMS"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "UNIFORMS"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "UNIFORMS"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template	  = 0;
	const GLchar* verification_snippet = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	switch (m_case)
	{
	case INVALID_Y:
		verification_snippet = verification_invalid_y;
		break;
	case INVALID_B:
		verification_snippet = verification_invalid_b;
		break;
	case INVALID_Q:
		verification_snippet = verification_invalid_q;
		break;
	case INVALID_XY:
		verification_snippet = verification_invalid_xy;
		break;
	case INVALID_XRS:
		verification_snippet = verification_invalid_xrs;
		break;
	case WRONG:
		verification_snippet = verification_wrong_u;
		break;
	case MISSING_PARENTHESIS:
		verification_snippet = verification_missing_parenthesis;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	};

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("UNIFORMS", position, uniforms, out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);
}

/* Constants used by BuiltInValuesTest */
const GLint BuiltInValuesTest::m_min_program_texel_offset_limit = -8;
const GLint BuiltInValuesTest::m_max_program_texel_offset_limit = 7;

/** Constructor
 *
 * @param context Test context
 **/
BuiltInValuesTest::BuiltInValuesTest(deqp::Context& context)
	: GLSLTestBase(context, "built_in_values", "Test verifies values of gl_Min/Max_ProgramTexelOffset")
{
	/* Nothing to be done here */
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BuiltInValuesTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
											Utils::shaderSource& out_source)
{
	static const GLchar* verification_snippet = "    if ((expected_values.x != gl_MinProgramTexelOffset) ||\n"
												"        (expected_values.y != gl_MaxProgramTexelOffset) )\n"
												"    {\n"
												"        result = vec4(1, 0, 0, 1);\n"
												"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform ivec2   expected_values;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"uniform ivec2 expected_values;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"uniform ivec2 expected_values;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"uniform ivec2 expected_values;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "uniform ivec2 expected_values;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "uniform ivec2 expected_values;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);
}

/** Overwritte of prepareUniforms method
 *
 * @param program Current program
 **/
void BuiltInValuesTest::prepareUniforms(Utils::program& program)
{
	const GLint expected_values_data[2] = { m_min_program_texel_offset, m_max_program_texel_offset };

	program.uniform("expected_values", Utils::INT, 1 /* n_cols */, 2 /* n_rows */, expected_values_data);
}

/** Prepare test cases
 *
 * @return true
 **/
bool BuiltInValuesTest::testInit()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.getIntegerv(GL_MIN_PROGRAM_TEXEL_OFFSET, &m_min_program_texel_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	gl.getIntegerv(GL_MAX_PROGRAM_TEXEL_OFFSET, &m_max_program_texel_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	if ((m_min_program_texel_offset_limit > m_min_program_texel_offset) ||
		(m_max_program_texel_offset_limit > m_max_program_texel_offset))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Invalid GL_PROGRAM_TEXEL_OFFSET values."
			<< " Min: " << m_min_program_texel_offset << " expected at top: " << m_min_program_texel_offset_limit
			<< " Max: " << m_min_program_texel_offset << " expected at least: " << m_max_program_texel_offset_limit
			<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
BuiltInAssignmentTest::BuiltInAssignmentTest(deqp::Context& context)
	: NegativeTestBase(context, "built_in_assignment",
					   "Test verifies that built in gl_Min/MaxProgramTexelOffset cannot be assigned")
{
	/* Nothing to be done here */
}

/** Set up next test case
 *
 * @param test_case_index Index of next test case
 *
 * @return false if there is no more test cases, true otherwise
 **/
bool BuiltInAssignmentTest::prepareNextTestCase(glw::GLuint test_case_index)
{
	const GLchar* description = 0;

	switch (test_case_index)
	{
	case (glw::GLuint)-1:
	case 0:
		description = "Testing gl_MinProgramTexelOffset";
		break;
	case 1:
		description = "Testing gl_MaxProgramTexelOffset";
		break;
	default:
		return false;
	}

	m_case = test_case_index;

	m_context.getTestContext().getLog() << tcu::TestLog::Message << description << tcu::TestLog::EndMessage;

	return true;
}

/** Prepare source for given shader stage
 *
 * @param in_stage           Shader stage, compute shader will use 430
 * @param in_use_version_400 Select if 400 or 420 should be used
 * @param out_source         Prepared shader source instance
 **/
void BuiltInAssignmentTest::prepareShaderSource(Utils::SHADER_STAGES in_stage, bool in_use_version_400,
												Utils::shaderSource& out_source)
{
	static const GLchar* min_verification_snippet = "    gl_MinProgramTexelOffset += gl_MaxProgramTexelOffset\n"
													"\n"
													"    if (expected_value != gl_MinProgramTexelOffset)\n"
													"    {\n"
													"        result = vec4(1, 0, 0, 1);\n"
													"    }\n";

	static const GLchar* max_verification_snippet = "    gl_MaxProgramTexelOffset += gl_MinProgramTexelOffset\n"
													"\n"
													"    if (expected_value != gl_MaxProgramTexelOffset)\n"
													"    {\n"
													"        result = vec4(1, 0, 0, 1);\n"
													"    }\n";

	static const GLchar* compute_shader_template =
		"VERSION\n"
		"\n"
		"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"writeonly uniform image2D uni_image;\n"
		"          uniform ivec2   expected_values;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"\n"
		"    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), result);\n"
		"}\n"
		"\n";

	static const GLchar* fragment_shader_template = "VERSION\n"
													"\n"
													"in  vec4 gs_fs_result;\n"
													"out vec4 fs_out_result;\n"
													"\n"
													"uniform ivec2 expected_values;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != gs_fs_result)\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    fs_out_result = result;\n"
													"}\n"
													"\n";

	static const GLchar* geometry_shader_template = "VERSION\n"
													"\n"
													"layout(points)                           in;\n"
													"layout(triangle_strip, max_vertices = 4) out;\n"
													"\n"
													"in  vec4 tes_gs_result[];\n"
													"out vec4 gs_fs_result;\n"
													"\n"
													"uniform ivec2 expected_values;\n"
													"\n"
													"void main()\n"
													"{\n"
													"    vec4 result = vec4(0, 1, 0, 1);\n"
													"\n"
													"VERIFICATION"
													"    else if (vec4(0, 1, 0, 1) != tes_gs_result[0])\n"
													"    {\n"
													"         result = vec4(1, 0, 0, 1);\n"
													"    }\n"
													"\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(-1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, -1, 0, 1);\n"
													"    EmitVertex();\n"
													"    gs_fs_result  = result;\n"
													"    gl_Position   = vec4(1, 1, 0, 1);\n"
													"    EmitVertex();\n"
													"}\n"
													"\n";

	static const GLchar* tess_ctrl_shader_template =
		"VERSION\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"in  vec4 vs_tcs_result[];\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"uniform ivec2 expected_values;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    vec4 result = vec4(0, 1, 0, 1);\n"
		"\n"
		"VERIFICATION"
		"    else if (vec4(0, 1, 0, 1) != vs_tcs_result[gl_InvocationID])\n"
		"    {\n"
		"         result = vec4(1, 0, 0, 1);\n"
		"    }\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = result;\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"}\n"
		"\n";

	static const GLchar* tess_eval_shader_template = "VERSION\n"
													 "\n"
													 "layout(isolines, point_mode) in;\n"
													 "\n"
													 "in  vec4 tcs_tes_result[];\n"
													 "out vec4 tes_gs_result;\n"
													 "\n"
													 "uniform ivec2 expected_values;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec4 result = vec4(0, 1, 0, 1);\n"
													 "\n"
													 "VERIFICATION"
													 "    else if (vec4(0, 1, 0, 1) != tcs_tes_result[0])\n"
													 "    {\n"
													 "         result = vec4(1, 0, 0, 1);\n"
													 "    }\n"
													 "\n"
													 "    tes_gs_result = result;\n"
													 "}\n"
													 "\n";

	static const GLchar* vertex_shader_template = "VERSION\n"
												  "\n"
												  "out vec4 vs_tcs_result;\n"
												  "\n"
												  "uniform ivec2 expected_values;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    vec4 result = vec4(0, 1, 0, 1);\n"
												  "\n"
												  "VERIFICATION"
												  "\n"
												  "    vs_tcs_result = result;\n"
												  "}\n"
												  "\n";

	const GLchar* shader_template	  = 0;
	const GLchar* verification_snippet = 0;

	switch (in_stage)
	{
	case Utils::COMPUTE_SHADER:
		shader_template = compute_shader_template;
		break;
	case Utils::FRAGMENT_SHADER:
		shader_template = fragment_shader_template;
		break;
	case Utils::GEOMETRY_SHADER:
		shader_template = geometry_shader_template;
		break;
	case Utils::TESS_CTRL_SHADER:
		shader_template = tess_ctrl_shader_template;
		break;
	case Utils::TESS_EVAL_SHADER:
		shader_template = tess_eval_shader_template;
		break;
	case Utils::VERTEX_SHADER:
		shader_template = vertex_shader_template;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	switch (m_case)
	{
	case (glw::GLuint)-1:
	case 0:
		verification_snippet = min_verification_snippet;
		break;
	case 1:
		verification_snippet = max_verification_snippet;
		break;
	}

	out_source.m_parts[0].m_code = shader_template;

	size_t position = 0;
	Utils::replaceToken("VERSION", position, getVersionString(in_stage, in_use_version_400),
						out_source.m_parts[0].m_code);

	Utils::replaceToken("VERIFICATION", position, verification_snippet, out_source.m_parts[0].m_code);
}

/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::buffer::buffer(deqp::Context& context) : m_id(0), m_context(context), m_target(0)
{
}

/** Destructor
 *
 **/
Utils::buffer::~buffer()
{
	release();
}

/** Execute BindBuffer
 *
 **/
void Utils::buffer::bind() const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(m_target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");
}

/** Execute BindBufferRange
 *
 * @param index  <index> parameter
 * @param offset <offset> parameter
 * @param size   <size> parameter
 **/
void Utils::buffer::bindRange(glw::GLuint index, glw::GLintptr offset, glw::GLsizeiptr size)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBufferRange(m_target, index, m_id, offset, size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");
}

/** Execute GenBuffer
 *
 * @param target Target that will be used by this buffer
 **/
void Utils::buffer::generate(glw::GLenum target)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_target = target;

	gl.genBuffers(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");
}

/** Maps buffer content
 *
 * @param access Access rights for mapped region
 *
 * @return Mapped memory
 **/
void* Utils::buffer::map(GLenum access) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(m_target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer");

	void* result = gl.mapBuffer(m_target, access);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	return result;
}

/** Unmaps buffer
 *
 **/
void Utils::buffer::unmap() const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(m_target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer");

	gl.unmapBuffer(m_target);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");
}

/** Execute BufferData
 *
 * @param size   <size> parameter
 * @param data   <data> parameter
 * @param usage  <usage> parameter
 **/
void Utils::buffer::update(glw::GLsizeiptr size, glw::GLvoid* data, glw::GLenum usage)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(m_target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer");

	gl.bufferData(m_target, size, data, usage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bufferData");
}

/** Release buffer
 *
 **/
void Utils::buffer::release()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteBuffers(1, &m_id);
		m_id = 0;
	}
}

/** Constructor
 *
 * @param context CTS context
 **/
Utils::framebuffer::framebuffer(deqp::Context& context) : m_id(0), m_context(context)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Utils::framebuffer::~framebuffer()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteFramebuffers(1, &m_id);
		m_id = 0;
	}
}

/** Attach texture to specified attachment
 *
 * @param attachment Attachment
 * @param texture_id Texture id
 * @param width      Texture width
 * @param height     Texture height
 **/
void Utils::framebuffer::attachTexture(glw::GLenum attachment, glw::GLuint texture_id, glw::GLuint width,
									   glw::GLuint height)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bind();

	gl.bindTexture(GL_TEXTURE_2D, texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

	gl.viewport(0 /* x */, 0 /* y */, width, height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");
}

/** Binds framebuffer to DRAW_FRAMEBUFFER
 *
 **/
void Utils::framebuffer::bind()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");
}

/** Clear framebuffer
 *
 * @param mask <mask> parameter of glClear. Decides which shall be cleared
 **/
void Utils::framebuffer::clear(glw::GLenum mask)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.clear(mask);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");
}

/** Specifies clear color
 *
 * @param red   Red channel
 * @param green Green channel
 * @param blue  Blue channel
 * @param alpha Alpha channel
 **/
void Utils::framebuffer::clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.clearColor(red, green, blue, alpha);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ClearColor");
}

/** Generate framebuffer
 *
 **/
void Utils::framebuffer::generate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");
}

Utils::shaderSource::shaderSource()
{
}

Utils::shaderSource::shaderSource(const shaderSource& source) : m_parts(source.m_parts)
{
}

Utils::shaderSource::shaderSource(const glw::GLchar* source_code)
{
	if (0 != source_code)
	{
		m_parts.resize(1);

		m_parts[0].m_code = source_code;
	}
}

Utils::shaderCompilationException::shaderCompilationException(const shaderSource& source, const glw::GLchar* message)
	: m_shader_source(source), m_error_message(message)
{
	/* Nothing to be done */
}

const char* Utils::shaderCompilationException::what() const throw()
{
	return "Shader compilation failed";
}

Utils::programLinkageException::programLinkageException(const glw::GLchar* message) : m_error_message(message)
{
	/* Nothing to be done */
}

const char* Utils::programLinkageException::what() const throw()
{
	return "Program linking failed";
}

const glw::GLenum Utils::program::ARB_COMPUTE_SHADER = 0x91B9;

/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::program::program(deqp::Context& context)
	: m_compute_shader_id(0)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_object_id(0)
	, m_tesselation_control_shader_id(0)
	, m_tesselation_evaluation_shader_id(0)
	, m_vertex_shader_id(0)
	, m_context(context)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Utils::program::~program()
{
	remove();
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
 * @param is_separable                       Selects if monolithis or separable program should be built. Defaults to false
 **/
void Utils::program::build(const glw::GLchar* compute_shader_code, const glw::GLchar* fragment_shader_code,
						   const glw::GLchar* geometry_shader_code, const glw::GLchar* tesselation_control_shader_code,
						   const glw::GLchar* tesselation_evaluation_shader_code, const glw::GLchar* vertex_shader_code,
						   const glw::GLchar* const* varying_names, glw::GLuint n_varying_names, bool is_separable)
{
	const shaderSource compute_shader(compute_shader_code);
	const shaderSource fragment_shader(fragment_shader_code);
	const shaderSource geometry_shader(geometry_shader_code);
	const shaderSource tesselation_control_shader(tesselation_control_shader_code);
	const shaderSource tesselation_evaluation_shader(tesselation_evaluation_shader_code);
	const shaderSource vertex_shader(vertex_shader_code);

	build(compute_shader, fragment_shader, geometry_shader, tesselation_control_shader, tesselation_evaluation_shader,
		  vertex_shader, varying_names, n_varying_names, is_separable);
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
 * @param is_separable                       Selects if monolithis or separable program should be built. Defaults to false
 **/
void Utils::program::build(const shaderSource& compute_shader, const shaderSource& fragment_shader,
						   const shaderSource& geometry_shader, const shaderSource& tesselation_control_shader,
						   const shaderSource& tesselation_evaluation_shader, const shaderSource& vertex_shader,
						   const glw::GLchar* const* varying_names, glw::GLuint n_varying_names, bool is_separable)
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create shader objects and compile */
	if (false == compute_shader.m_parts.empty())
	{
		m_compute_shader_id = gl.createShader(ARB_COMPUTE_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_compute_shader_id, compute_shader);
	}

	if (false == fragment_shader.m_parts.empty())
	{
		m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_fragment_shader_id, fragment_shader);
	}

	if (false == geometry_shader.m_parts.empty())
	{
		m_geometry_shader_id = gl.createShader(GL_GEOMETRY_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_geometry_shader_id, geometry_shader);
	}

	if (false == tesselation_control_shader.m_parts.empty())
	{
		m_tesselation_control_shader_id = gl.createShader(GL_TESS_CONTROL_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_tesselation_control_shader_id, tesselation_control_shader);
	}

	if (false == tesselation_evaluation_shader.m_parts.empty())
	{
		m_tesselation_evaluation_shader_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_tesselation_evaluation_shader_id, tesselation_evaluation_shader);
	}

	if (false == vertex_shader.m_parts.empty())
	{
		m_vertex_shader_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_vertex_shader_id, vertex_shader);
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

	/* Set separable parameter */
	if (true == is_separable)
	{
		gl.programParameteri(m_program_object_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ProgramParameteri");
	}

	/* Link program */
	link();
}

void Utils::program::compile(glw::GLuint shader_id, const Utils::shaderSource& source) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Compilation status */
	glw::GLint status = GL_FALSE;

	/* Source parts and lengths vectors */
	std::vector<const GLchar*> parts;
	std::vector<GLint>		   lengths_vector;
	GLint*					   lengths = 0;

	/* Prepare storage */
	parts.resize(source.m_parts.size());

	/* Prepare arrays */
	for (GLuint i = 0; i < source.m_parts.size(); ++i)
	{
		parts[i] = source.m_parts[i].m_code.c_str();
	}

	if (true == source.m_use_lengths)
	{
		lengths_vector.resize(source.m_parts.size());

		for (GLuint i = 0; i < source.m_parts.size(); ++i)
		{
			lengths_vector[i] = source.m_parts[i].m_length;
		}

		lengths = &lengths_vector[0];
	}

	/* Set source code */
	gl.shaderSource(shader_id, static_cast<GLsizei>(source.m_parts.size()), &parts[0], lengths);
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

		throw shaderCompilationException(source, &message[0]);
	}
}

/** Create program from provided binary
 *
 * @param binary        Buffer with binary form of program
 * @param binary_format Format of <binary> data
 **/
void Utils::program::createFromBinary(const std::vector<GLubyte>& binary, GLenum binary_format)
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program object */
	m_program_object_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

	gl.programBinary(m_program_object_id, binary_format, &binary[0], static_cast<GLsizei>(binary.size()));
	GLU_EXPECT_NO_ERROR(gl.getError(), "ProgramBinary");
}

glw::GLint Utils::program::getAttribLocation(const glw::GLchar* name) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint location = gl.getAttribLocation(m_program_object_id, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetAttribLocation");

	return location;
}

/** Get binary form of program
 *
 * @param binary        Buffer for binary data
 * @param binary_format Format of binary data
 **/
void Utils::program::getBinary(std::vector<GLubyte>& binary, GLenum& binary_format) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get binary size */
	GLint length = 0;
	gl.getProgramiv(m_program_object_id, GL_PROGRAM_BINARY_LENGTH, &length);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

	/* Allocate storage */
	binary.resize(length);

	/* Get binary */
	gl.getProgramBinary(m_program_object_id, static_cast<GLsizei>(binary.size()), &length, &binary_format, &binary[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramBinary");
}

/** Get subroutine index
 *
 * @param subroutine_name Subroutine name
 *
 * @return Index of subroutine
 **/
GLuint Utils::program::getSubroutineIndex(const glw::GLchar* subroutine_name, glw::GLenum shader_stage) const
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	GLuint				  index = -1;

	index = gl.getSubroutineIndex(m_program_object_id, shader_stage, subroutine_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetSubroutineIndex");

	if (GL_INVALID_INDEX == index)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Subroutine: " << subroutine_name
											<< " is not available" << tcu::TestLog::EndMessage;

		TCU_FAIL("Subroutine is not available");
	}

	return index;
}

/** Get subroutine uniform location
 *
 * @param uniform_name Subroutine uniform name
 *
 * @return Location of subroutine uniform
 **/
GLint Utils::program::getSubroutineUniformLocation(const glw::GLchar* uniform_name, glw::GLenum shader_stage) const
{
	const glw::Functions& gl	   = m_context.getRenderContext().getFunctions();
	GLint				  location = -1;

	location = gl.getSubroutineUniformLocation(m_program_object_id, shader_stage, uniform_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetSubroutineUniformLocation");

	if (-1 == location)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Subroutine uniform: " << uniform_name
											<< " is not available" << tcu::TestLog::EndMessage;

		TCU_FAIL("Subroutine uniform is not available");
	}

	return location;
}

/** Get integer uniform at given location
 *
 * @param location Uniform location
 *
 * @return Value
 **/
GLint Utils::program::getUniform1i(GLuint location) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint result;

	gl.getUniformiv(m_program_object_id, location, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformiv");

	return result;
}

/** Get uniform location
 *
 * @param uniform_name Subroutine uniform name
 *
 * @return Location of uniform
 **/
GLint Utils::program::getUniformLocation(const glw::GLchar* uniform_name) const
{
	const glw::Functions& gl	   = m_context.getRenderContext().getFunctions();
	GLint				  location = -1;

	location = gl.getUniformLocation(m_program_object_id, uniform_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

	if (-1 == location)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Uniform: " << uniform_name
											<< " is not available" << tcu::TestLog::EndMessage;

		TCU_FAIL("Uniform is not available");
	}

	return location;
}

/** Attach shaders and link program
 *
 **/
void Utils::program::link() const
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

		throw programLinkageException(&message[0]);
	}
}

/** Delete program object and all attached shaders
 *
 **/
void Utils::program::remove()
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

void Utils::program::uniform(const glw::GLchar* uniform_name, TYPES type, glw::GLuint n_columns, glw::GLuint n_rows,
							 const void* data) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLuint location = getUniformLocation(uniform_name);

	if ((glw::GLuint)-1 == location)
	{
		TCU_FAIL("Uniform is inactive");
	}

	switch (type)
	{
	case DOUBLE:
		if (1 == n_columns)
		{
			getUniformNdv(gl, n_rows)(location, 1 /* count */, (const GLdouble*)data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UniformNdv");
		}
		else
		{
			getUniformMatrixNdv(gl, n_columns, n_rows)(location, 1 /* count */, false, (const GLdouble*)data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UniformMatrixNdv");
		}
		break;
	case FLOAT:
		if (1 == n_columns)
		{
			getUniformNfv(gl, n_rows)(location, 1 /* count */, (const GLfloat*)data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UniformNfv");
		}
		else
		{
			getUniformMatrixNfv(gl, n_columns, n_rows)(location, 1 /* count */, false, (const GLfloat*)data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UniformMatrixNfv");
		}
		break;
	case INT:
		getUniformNiv(gl, n_rows)(location, 1 /* count */, (const GLint*)data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UniformNiv");
		break;
	case UINT:
		getUniformNuiv(gl, n_rows)(location, 1 /* count */, (const GLuint*)data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UniformNuiv");
		break;
	default:
		TCU_FAIL("Invalid enum");
	}
}

/** Execute UseProgram
 *
 **/
void Utils::program::use() const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");
}

void Utils::program::printShaderSource(const shaderSource& source, tcu::MessageBuilder& log)
{
	GLuint line_number = 0;

	log << "Shader source.";

	for (GLuint i = 0; i < source.m_parts.size(); ++i)
	{
		log << "\nLine||Part: " << (i + 1) << "/" << source.m_parts.size();

		if (true == source.m_use_lengths)
		{
			log << " Length: " << source.m_parts[i].m_length;
		}

		log << "\n";

		const GLchar* part = source.m_parts[i].m_code.c_str();

		while (0 != part)
		{
			std::string   line;
			const GLchar* next_line = strchr(part, '\n');

			if (0 != next_line)
			{
				next_line += 1;
				line.assign(part, next_line - part);
			}
			else
			{
				line = part;
			}

			if (0 != *part)
			{
				log << std::setw(4) << line_number << "||" << line;
			}

			part = next_line;
			line_number += 1;
		}
	}
}

/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::texture::texture(deqp::Context& context) : m_id(0), m_context(context), m_texture_type(TEX_2D)
{
	/* Nothing to done here */
}

/** Destructor
 *
 **/
Utils::texture::~texture()
{
	release();
}

/** Bind texture to GL_TEXTURE_2D
 *
 **/
void Utils::texture::bind() const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLenum target = getTextureTartet(m_texture_type);

	gl.bindTexture(target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Create 2d texture
 *
 * @param width           Width of texture
 * @param height          Height of texture
 * @param internal_format Internal format of texture
 **/
void Utils::texture::create(glw::GLuint width, glw::GLuint height, glw::GLenum internal_format)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	release();

	m_texture_type = TEX_2D;

	gl.genTextures(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	bind();

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, internal_format, width, height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
}

/** Create texture of given type
 *
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param internal_format Internal format of texture
 * @param texture_type    Type of texture
 **/
void Utils::texture::create(GLuint width, GLuint height, GLuint depth, GLenum internal_format,
							TEXTURE_TYPES texture_type)
{
	static const GLuint levels = 1;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	release();

	m_texture_type = texture_type;

	GLenum target = getTextureTartet(m_texture_type);

	gl.genTextures(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	bind();

	switch (m_texture_type)
	{
	case TEX_1D:
		gl.texStorage1D(target, levels, internal_format, width);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage1D");
		break;
	case TEX_2D:
	case TEX_1D_ARRAY:
	case TEX_2D_RECT:
	case TEX_CUBE:
		gl.texStorage2D(target, levels, internal_format, width, height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
		break;
	case TEX_3D:
	case TEX_2D_ARRAY:
		gl.texStorage3D(target, levels, internal_format, width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage3D");
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/** Create buffer texture
 *
 * @param internal_format Internal format of texture
 * @param buffer_id       Id of buffer that will be used as data source
 **/
void Utils::texture::createBuffer(GLenum internal_format, GLuint buffer_id)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	release();

	m_texture_type = TEX_BUFFER;
	m_buffer_id	= buffer_id;

	gl.genTextures(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	bind();

	gl.texBuffer(GL_TEXTURE_BUFFER, internal_format, buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexBuffer");
}

/** Get contents of texture
 *
 * @param format   Format of image
 * @param type     Type of image
 * @param out_data Buffer for image
 **/
void Utils::texture::get(glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLenum target = getTextureTartet(m_texture_type);

	bind();

	gl.memoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

	if (TEX_CUBE != m_texture_type)
	{
		gl.getTexImage(target, 0 /* level */, format, type, out_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");
	}
	else
	{
		GLint width;
		GLint height;

		if ((GL_RGBA != format) && (GL_UNSIGNED_BYTE != type))
		{
			TCU_FAIL("Not implemented");
		}

		GLuint texel_size = 4;

		gl.getTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 /* level */, GL_TEXTURE_WIDTH, &width);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

		gl.getTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 /* level */, GL_TEXTURE_HEIGHT, &height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");

		const GLuint image_size = width * height * texel_size;

		gl.getTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 /* level */, format, type,
					   (GLvoid*)((GLchar*)out_data + (image_size * 0)));
		gl.getTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0 /* level */, format, type,
					   (GLvoid*)((GLchar*)out_data + (image_size * 1)));
		gl.getTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0 /* level */, format, type,
					   (GLvoid*)((GLchar*)out_data + (image_size * 2)));
		gl.getTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0 /* level */, format, type,
					   (GLvoid*)((GLchar*)out_data + (image_size * 3)));
		gl.getTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0 /* level */, format, type,
					   (GLvoid*)((GLchar*)out_data + (image_size * 4)));
		gl.getTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0 /* level */, format, type,
					   (GLvoid*)((GLchar*)out_data + (image_size * 5)));
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");
	}
}

/** Delete texture
 *
 **/
void Utils::texture::release()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteTextures(1, &m_id);
		m_id = 0;

		if ((m_texture_type == TEX_BUFFER) && (0 != m_buffer_id))
		{
			gl.deleteBuffers(1, &m_buffer_id);
			m_buffer_id = 0;
		}
	}
}

/** Update contents of texture
 *
 * @param width  Width of texture
 * @param height Height of texture
 * @param format Format of data
 * @param type   Type of data
 * @param data   Buffer with image
 **/
void Utils::texture::update(glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum format,
							glw::GLenum type, glw::GLvoid* data)
{
	static const GLuint level = 0;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLenum target = getTextureTartet(m_texture_type);

	bind();

	switch (m_texture_type)
	{
	case TEX_1D:
		gl.texSubImage1D(target, level, 0 /* x */, width, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage1D");
		break;
	case TEX_2D:
	case TEX_1D_ARRAY:
	case TEX_2D_RECT:
		gl.texSubImage2D(target, level, 0 /* x */, 0 /* y */, width, height, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
		break;
	case TEX_CUBE:
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, 0 /* x */, 0 /* y */, width, height, format, type,
						 data);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, 0 /* x */, 0 /* y */, width, height, format, type,
						 data);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, 0 /* x */, 0 /* y */, width, height, format, type,
						 data);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, 0 /* x */, 0 /* y */, width, height, format, type,
						 data);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, 0 /* x */, 0 /* y */, width, height, format, type,
						 data);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, 0 /* x */, 0 /* y */, width, height, format, type,
						 data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
		break;
	case TEX_3D:
	case TEX_2D_ARRAY:
		gl.texSubImage3D(target, level, 0 /* x */, 0 /* y */, 0 /* z */, width, height, depth, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage3D");
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::vertexArray::vertexArray(deqp::Context& context) : m_id(0), m_context(context)
{
}

/** Destructor
 *
 **/
Utils::vertexArray::~vertexArray()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteVertexArrays(1, &m_id);

		m_id = 0;
	}
}

/** Execute BindVertexArray
 *
 **/
void Utils::vertexArray::bind()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindVertexArray(m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");
}

/** Execute GenVertexArrays
 *
 **/
void Utils::vertexArray::generate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");
}
} /* GLSL420Pack namespace */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
ShadingLanguage420PackTests::ShadingLanguage420PackTests(deqp::Context& context)
	: TestCaseGroup(context, "shading_language_420pack", "Verifies \"shading_language_420pack\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void ShadingLanguage420PackTests::init(void)
{
	addChild(new GLSL420Pack::BindingSamplerSingleTest(m_context));
	addChild(new GLSL420Pack::BindingImageSingleTest(m_context));
	addChild(new GLSL420Pack::UTF8CharactersTest(m_context));
	addChild(new GLSL420Pack::UTF8InSourceTest(m_context));
	addChild(new GLSL420Pack::QualifierOrderTest(m_context));
	addChild(new GLSL420Pack::QualifierOrderBlockTest(m_context));
	addChild(new GLSL420Pack::LineContinuationTest(m_context));
	addChild(new GLSL420Pack::LineNumberingTest(m_context));
	addChild(new GLSL420Pack::ImplicitConversionsValidTest(m_context));
	addChild(new GLSL420Pack::ImplicitConversionsInvalidTest(m_context));
	addChild(new GLSL420Pack::ConstDynamicValueTest(m_context));
	addChild(new GLSL420Pack::ConstAssignmentTest(m_context));
	addChild(new GLSL420Pack::ConstDynamicValueAsConstExprTest(m_context));
	addChild(new GLSL420Pack::QualifierOrderUniformTest(m_context));
	addChild(new GLSL420Pack::QualifierOrderFunctionInoutTest(m_context));
	addChild(new GLSL420Pack::QualifierOrderFunctionInputTest(m_context));
	addChild(new GLSL420Pack::QualifierOrderFunctionOutputTest(m_context));
	addChild(new GLSL420Pack::QualifierOverrideLayoutTest(m_context));
	addChild(new GLSL420Pack::BindingUniformBlocksTest(m_context));
	addChild(new GLSL420Pack::BindingUniformSingleBlockTest(m_context));
	addChild(new GLSL420Pack::BindingUniformBlockArrayTest(m_context));
	addChild(new GLSL420Pack::BindingUniformDefaultTest(m_context));
	addChild(new GLSL420Pack::BindingUniformAPIOverirdeTest(m_context));
	addChild(new GLSL420Pack::BindingUniformGlobalBlockTest(m_context));
	addChild(new GLSL420Pack::BindingUniformInvalidTest(m_context));
	addChild(new GLSL420Pack::BindingSamplersTest(m_context));
	addChild(new GLSL420Pack::BindingSamplerArrayTest(m_context));
	addChild(new GLSL420Pack::BindingSamplerDefaultTest(m_context));
	addChild(new GLSL420Pack::BindingSamplerAPIOverrideTest(m_context));
	addChild(new GLSL420Pack::BindingSamplerInvalidTest(m_context));
	addChild(new GLSL420Pack::BindingImagesTest(m_context));
	addChild(new GLSL420Pack::BindingImageArrayTest(m_context));
	addChild(new GLSL420Pack::BindingImageDefaultTest(m_context));
	addChild(new GLSL420Pack::BindingImageAPIOverrideTest(m_context));
	addChild(new GLSL420Pack::BindingImageInvalidTest(m_context));
	addChild(new GLSL420Pack::InitializerListTest(m_context));
	addChild(new GLSL420Pack::InitializerListNegativeTest(m_context));
	addChild(new GLSL420Pack::LengthOfVectorAndMatrixTest(m_context));
	addChild(new GLSL420Pack::LengthOfComputeResultTest(m_context));
	addChild(new GLSL420Pack::ScalarSwizzlersTest(m_context));
	addChild(new GLSL420Pack::ScalarSwizzlersInvalidTest(m_context));
	addChild(new GLSL420Pack::BuiltInValuesTest(m_context));
	addChild(new GLSL420Pack::BuiltInAssignmentTest(m_context));
}

} /* gl4cts namespace */
