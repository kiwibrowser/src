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

#include "esextcTestCaseBase.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"
#include <algorithm>
#include <cstdarg>
#include <iostream>

namespace glcts
{

/* Predefined shader source code */
const char* TestCaseBase::m_boilerplate_vs_code = "${VERSION}\n"
												  "\n"
												  "precision highp float;\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
												  "}\n";

const float TestCaseBase::m_epsilon_float = 0.0001f;

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
TestCaseBase::TestCaseBase(Context& context, const ExtParameters& extParam, const char* name, const char* description)
	: tcu::TestCase(context.getTestContext(), name, description)
	, m_context(context)
	, m_glslVersion(extParam.glslVersion)
	, m_extType(extParam.extType)
	, m_is_framebuffer_no_attachments_supported(false)
	, m_is_geometry_shader_extension_supported(false)
	, m_is_geometry_shader_point_size_supported(false)
	, m_is_gpu_shader5_supported(false)
	, m_is_program_interface_query_supported(false)
	, m_is_shader_image_load_store_supported(false)
	, m_is_shader_image_atomic_supported(false)
	, m_is_texture_storage_multisample_supported(false)
	, m_is_texture_storage_multisample_2d_array_supported(false)
	, m_is_tessellation_shader_supported(false)
	, m_is_tessellation_shader_point_size_supported(false)
	, m_is_texture_cube_map_array_supported(false)
	, m_is_texture_border_clamp_supported(false)
	, m_is_texture_buffer_supported(false)
	, m_is_viewport_array_supported(false)
	, seed_value(1)
{
	m_glExtTokens.init(context.getRenderContext().getType());
}

/** Initializes base class that all geometry shader test implementations derive from.
 *
 **/
void TestCaseBase::init(void)
{
	initExtensions();
	initGLSLSpecializationMap();
}

/** Initializes function pointers for ES3.1 extensions, as well as determines
 *  availability of these extensions.
 **/
void TestCaseBase::initExtensions()
{
	const glu::ContextType& context_type = m_context.getRenderContext().getType();

	/* OpenGL 4.0 or higher is minimum expectation for any of these tests */
	if (glu::contextSupports(context_type, glu::ApiType::core(4, 0)))
	{
		m_is_geometry_shader_extension_supported	  = true;
		m_is_geometry_shader_point_size_supported	 = true;
		m_is_gpu_shader5_supported					  = true;
		m_is_tessellation_shader_supported			  = true;
		m_is_tessellation_shader_point_size_supported = true;
		m_is_texture_cube_map_array_supported		  = true;
		m_is_texture_border_clamp_supported			  = true;
		m_is_texture_buffer_supported				  = true;
		m_is_shader_image_atomic_supported			  = glu::contextSupports(context_type, glu::ApiType::core(4, 2));
		m_is_texture_storage_multisample_2d_array_supported =
			glu::contextSupports(context_type, glu::ApiType::core(4, 3));
		m_is_framebuffer_no_attachments_supported  = glu::contextSupports(context_type, glu::ApiType::core(4, 3));
		m_is_program_interface_query_supported	 = glu::contextSupports(context_type, glu::ApiType::core(4, 3));
		m_is_texture_storage_multisample_supported = glu::contextSupports(context_type, glu::ApiType::core(4, 3));
		m_is_shader_image_load_store_supported	 = glu::contextSupports(context_type, glu::ApiType::core(4, 2));
		m_is_viewport_array_supported			   = glu::contextSupports(context_type, glu::ApiType::core(4, 1));
	}
	else if (glu::contextSupports(context_type, glu::ApiType::es(3, 2)))
	{
		m_is_geometry_shader_extension_supported			= true;
		m_is_gpu_shader5_supported							= true;
		m_is_tessellation_shader_supported					= true;
		m_is_texture_cube_map_array_supported				= true;
		m_is_texture_border_clamp_supported					= true;
		m_is_texture_buffer_supported						= true;
		m_is_shader_image_atomic_supported					= true;
		m_is_texture_storage_multisample_2d_array_supported = true;
		m_is_framebuffer_no_attachments_supported			= true;
		m_is_program_interface_query_supported				= true;
		m_is_texture_storage_multisample_supported			= true;
		m_is_shader_image_load_store_supported				= true;
		m_is_geometry_shader_point_size_supported =
			isExtensionSupported("GL_OES_geometry_point_size") || isExtensionSupported("GL_EXT_geometry_point_size");
		m_is_tessellation_shader_point_size_supported = isExtensionSupported("GL_OES_tessellation_point_size") ||
														isExtensionSupported("GL_EXT_tessellation_point_size");
		m_is_viewport_array_supported = isExtensionSupported("GL_OES_viewport_array");
	}
	else
	{
		/* ES3.1 core functionality is assumed*/
		DE_ASSERT(isContextTypeES(context_type));
		DE_ASSERT(glu::contextSupports(context_type, glu::ApiType::es(3, 1)));

		/* these are part of ES 3.1 */
		m_is_framebuffer_no_attachments_supported  = true;
		m_is_program_interface_query_supported	 = true;
		m_is_texture_storage_multisample_supported = true;
		m_is_shader_image_load_store_supported	 = true;

		/* AEP extensions - either test OES variants or EXT variants */
		if (m_extType == EXTENSIONTYPE_OES)
		{
			/* These are all ES 3.1 extensions */
			m_is_geometry_shader_extension_supported	  = isExtensionSupported("GL_OES_geometry_shader");
			m_is_geometry_shader_point_size_supported	 = isExtensionSupported("GL_OES_geometry_point_size");
			m_is_gpu_shader5_supported					  = isExtensionSupported("GL_OES_gpu_shader5");
			m_is_tessellation_shader_supported			  = isExtensionSupported("GL_OES_tessellation_shader");
			m_is_tessellation_shader_point_size_supported = isExtensionSupported("GL_OES_tessellation_point_size");
			m_is_texture_cube_map_array_supported		  = isExtensionSupported("GL_OES_texture_cube_map_array");
			m_is_texture_border_clamp_supported			  = isExtensionSupported("GL_OES_texture_border_clamp");
			m_is_texture_buffer_supported				  = isExtensionSupported("GL_OES_texture_buffer");
		}
		else
		{
			DE_ASSERT(m_extType == EXTENSIONTYPE_EXT);

			/* These are all ES 3.1 extensions */
			m_is_geometry_shader_extension_supported	  = isExtensionSupported("GL_EXT_geometry_shader");
			m_is_geometry_shader_point_size_supported	 = isExtensionSupported("GL_EXT_geometry_point_size");
			m_is_gpu_shader5_supported					  = isExtensionSupported("GL_EXT_gpu_shader5");
			m_is_tessellation_shader_supported			  = isExtensionSupported("GL_EXT_tessellation_shader");
			m_is_tessellation_shader_point_size_supported = isExtensionSupported("GL_EXT_tessellation_point_size");
			m_is_texture_cube_map_array_supported		  = isExtensionSupported("GL_EXT_texture_cube_map_array");
			m_is_texture_border_clamp_supported			  = isExtensionSupported("GL_EXT_texture_border_clamp");
			m_is_texture_buffer_supported				  = isExtensionSupported("GL_EXT_texture_buffer");
		}

		/* other ES 3.1 extensions */
		m_is_shader_image_atomic_supported = isExtensionSupported("GL_OES_shader_image_atomic");
		m_is_texture_storage_multisample_2d_array_supported =
			isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");
		m_is_viewport_array_supported = isExtensionSupported("GL_OES_viewport_array");
	}
}

/** Initializes function pointers for ES3.1 extensions, as well as determines
 *  availability of these extensions.
 **/
void TestCaseBase::initGLSLSpecializationMap()
{
	m_specializationMap["VERSION"] = glu::getGLSLVersionDeclaration(m_glslVersion);
	m_specializationMap["SHADER_IO_BLOCKS_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_SHADER_IO_BLOCKS, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["SHADER_IO_BLOCKS_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_SHADER_IO_BLOCKS, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["GEOMETRY_SHADER_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_GEOMETRY_SHADER, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["GEOMETRY_SHADER_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_GEOMETRY_SHADER, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["GEOMETRY_POINT_SIZE_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_GEOMETRY_POINT_SIZE, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["GEOMETRY_POINT_SIZE_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_GEOMETRY_POINT_SIZE, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["TESSELLATION_SHADER_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_TESSELLATION_SHADER, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["TESSELLATION_SHADER_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_TESSELLATION_SHADER, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["TESSELLATION_POINT_SIZE_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_TESSELLATION_POINT_SIZE, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["TESSELLATION_POINT_SIZE_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_TESSELLATION_POINT_SIZE, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["GPU_SHADER5_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_GPU_SHADER5, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["GPU_SHADER5_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_GPU_SHADER5, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["TEXTURE_BUFFER_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_TEXTURE_BUFFER, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["TEXTURE_BUFFER_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_TEXTURE_BUFFER, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["TEXTURE_CUBE_MAP_ARRAY_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_TEXTURE_CUBE_MAP_ARRAY, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["TEXTURE_CUBE_MAP_ARRAY_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_TEXTURE_CUBE_MAP_ARRAY, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["SHADER_IMAGE_ATOMIC_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_SHADER_IMAGE_ATOMIC, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["SHADER_IMAGE_ATOMIC_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_SHADER_IMAGE_ATOMIC, EXTENSIONBEHAVIOR_REQUIRE);
	m_specializationMap["VIEWPORT_ARRAY_ENABLE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_VIEWPORT_ARRAY, EXTENSIONBEHAVIOR_ENABLE);
	m_specializationMap["VIEWPORT_ARRAY_REQUIRE"] =
		getGLSLExtDirective(m_extType, EXTENSIONNAME_VIEWPORT_ARRAY, EXTENSIONBEHAVIOR_REQUIRE);

	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		m_specializationMap["IN_PER_VERTEX_DECL_ARRAY"]				= "\n";
		m_specializationMap["IN_PER_VERTEX_DECL_ARRAY_POINT_SIZE"]  = "\n";
		m_specializationMap["OUT_PER_VERTEX_DECL"]					= "\n";
		m_specializationMap["OUT_PER_VERTEX_DECL_POINT_SIZE"]		= "\n";
		m_specializationMap["OUT_PER_VERTEX_DECL_ARRAY"]			= "\n";
		m_specializationMap["OUT_PER_VERTEX_DECL_ARRAY_POINT_SIZE"] = "\n";
		m_specializationMap["IN_DATA_DECL"]							= "\n";
		m_specializationMap["POSITION_WITH_IN_DATA"]				= "gl_Position = gl_in[0].gl_Position;\n";
	}
	else
	{
		m_specializationMap["IN_PER_VERTEX_DECL_ARRAY"] = "in gl_PerVertex {\n"
														  "    vec4 gl_Position;\n"
														  "} gl_in[];\n";
		m_specializationMap["IN_PER_VERTEX_DECL_ARRAY_POINT_SIZE"] = "in gl_PerVertex {\n"
																	 "    vec4 gl_Position;\n"
																	 "    float gl_PointSize;\n"
																	 "} gl_in[];\n";
		m_specializationMap["OUT_PER_VERTEX_DECL"] = "out gl_PerVertex {\n"
													 "    vec4 gl_Position;\n"
													 "};\n";
		m_specializationMap["OUT_PER_VERTEX_DECL_POINT_SIZE"] = "out gl_PerVertex {\n"
																"    vec4 gl_Position;\n"
																"    float gl_PointSize;\n"
																"};\n";
		m_specializationMap["OUT_PER_VERTEX_DECL_ARRAY"] = "out gl_PerVertex {\n"
														   "    vec4 gl_Position;\n"
														   "} gl_out[];\n";
		m_specializationMap["OUT_PER_VERTEX_DECL_ARRAY_POINT_SIZE"] = "out gl_PerVertex {\n"
																	  "    vec4 gl_Position;\n"
																	  "    float gl_PointSize;\n"
																	  "} gl_out[];\n";
		m_specializationMap["IN_DATA_DECL"] = "in Data {\n"
											  "    vec4 pos;\n"
											  "} input_data[1];\n";
		m_specializationMap["POSITION_WITH_IN_DATA"] = "gl_Position = input_data[0].pos;\n";
	}
}

/** Sets the seed for the random generator
 *  @param seed - seed for the random generator
 */
void TestCaseBase::randomSeed(const glw::GLuint seed)
{
	seed_value = seed;
}

/** Returns random unsigned integer from the range [0,max)
 *  @param  max - the value that is the upper boundary for the returned random numbers
 *  @return random unsigned integer from the range [0,max)
 */
glw::GLuint TestCaseBase::randomFormula(const glw::GLuint max)
{
	static const glw::GLuint a = 11;
	static const glw::GLuint b = 17;

	seed_value = (a * seed_value + b) % max;

	return seed_value;
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult TestCaseBase::iterate(void)
{
	qpTestResult result = QP_TEST_RESULT_FAIL;

	m_testCtx.setTestResult(result, "This location should never be called.");

	return STOP;
}

/** Deinitializes base class that all test implementations inherit from.
 *
 **/
void TestCaseBase::deinit(void)
{
	/* Left empty on purpose */
}

/** Tells whether particular extension is supported.
 *
 *  @param extName: The name of the extension
 *
 *  @return true   if given extension name is reported as supported, false otherwise.
 **/
bool TestCaseBase::isExtensionSupported(const std::string& extName) const
{
	const std::vector<std::string>& extensions = m_context.getContextInfo().getExtensions();

	if (std::find(extensions.begin(), extensions.end(), extName) != extensions.end())
	{
		return true;
	}

	return false;
}

/** Helper method for specializing a shader */
std::string TestCaseBase::specializeShader(const unsigned int parts, const char* const* code) const
{
	std::stringstream code_merged;
	for (unsigned int i = 0; i < parts; i++)
	{
		code_merged << code[i];
	}
	return tcu::StringTemplate(code_merged.str().c_str()).specialize(m_specializationMap);
}

void TestCaseBase::shaderSourceSpecialized(glw::GLuint shader_id, glw::GLsizei shader_count,
										   const glw::GLchar* const* shader_string)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::string specialized		 = specializeShader(shader_count, shader_string);
	const char* specialized_cstr = specialized.c_str();
	gl.shaderSource(shader_id, 1, &specialized_cstr, NULL);
}

std::string getShaderTypeName(glw::GLenum shader_type)
{
	switch (shader_type)
	{
	case GL_VERTEX_SHADER:
		return "Vertex shader";
	case GL_TESS_CONTROL_SHADER:
		return "Tessellation control shader";
	case GL_TESS_EVALUATION_SHADER:
		return "Tessellation evaluation shader";
	case GL_GEOMETRY_SHADER:
		return "Geometry shader";
	case GL_FRAGMENT_SHADER:
		return "Fragment shader";
	case GL_COMPUTE_SHADER:
		return "Compute shader";
	default:
		DE_ASSERT(0);
		return "??? shader";
	}
}

/** Compiles and links program with variable amount of shaders
 *
 * @param po_id                      Program handle
 * @param out_has_compilation_failed Deref will be set to true, if shader compilation
 *                                   failed for any of the submitted shaders.
 *                                   Will be set to false otherwise. Can be NULL.
 * @param sh_stages                  Shader stages
 * @for all shader stages
 * {
 *   @param sh_id          Shader handle. 0 means "skip"
 *   @param sh_parts       Number of shader source code parts.
 *                         0 means that it's already compiled.
 *   @param sh_code        Shader source code.
 * }
 **/
bool TestCaseBase::buildProgramVA(glw::GLuint po_id, bool* out_has_compilation_failed, unsigned int sh_stages, ...)
{
	const glw::Functions&	gl = m_context.getRenderContext().getFunctions();
	std::vector<glw::GLuint> vec_sh_id;

	va_list values;
	va_start(values, sh_stages);

	/* Shaders compilation */
	glw::GLint compilation_status = GL_FALSE;

	for (unsigned int stage = 0; stage < sh_stages; ++stage)
	{
		glw::GLuint		   sh_id	= va_arg(values, glw::GLuint);
		unsigned int	   sh_parts = va_arg(values, unsigned int);
		const char* const* sh_code  = va_arg(values, const char* const*);

		if (sh_id == 0)
		{
			continue;
		}

		if (sh_parts != 0)
		{
			std::string sh_merged_string = specializeShader(sh_parts, sh_code);
			const char* sh_merged_ptr	= sh_merged_string.c_str();

			gl.shaderSource(sh_id, 1, &sh_merged_ptr, NULL);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed!");

			gl.compileShader(sh_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed!");

			gl.getShaderiv(sh_id, GL_COMPILE_STATUS, &compilation_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed!");

			if (compilation_status != GL_TRUE)
			{
				glw::GLint  shader_type = 0;
				std::string info_log	= getCompilationInfoLog(sh_id);

				gl.getShaderiv(sh_id, GL_SHADER_TYPE, &shader_type);
				std::string shader_type_str = getShaderTypeName(shader_type);

				m_testCtx.getLog() << tcu::TestLog::Message << shader_type_str << " compilation failure:\n\n"
								   << info_log << "\n\n"
								   << shader_type_str << " source:\n\n"
								   << sh_merged_string << "\n\n"
								   << tcu::TestLog::EndMessage;

				break;
			}
		}

		gl.attachShader(po_id, sh_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader(VERTEX_SHADER) call failed");

		vec_sh_id.push_back(sh_id);
	}

	va_end(values);

	if (out_has_compilation_failed != NULL)
	{
		*out_has_compilation_failed = (compilation_status == GL_FALSE);
	}

	if (compilation_status != GL_TRUE)
	{
		return false;
	}

	/* Linking the program */

	glw::GLint link_status = GL_FALSE;
	gl.linkProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed!");

	gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed!");

	if (link_status != GL_TRUE)
	{
		/* Dump link log */
		std::string link_log = getLinkingInfoLog(po_id);
		m_testCtx.getLog() << tcu::TestLog::Message << "Link failure:\n\n"
						   << link_log << "\n\n"
						   << tcu::TestLog::EndMessage;

		/* Dump shader source */
		for (std::vector<glw::GLuint>::iterator it = vec_sh_id.begin(); it != vec_sh_id.end(); ++it)
		{
			glw::GLint shader_type = 0;
			gl.getShaderiv(*it, GL_SHADER_TYPE, &shader_type);
			std::string shader_type_str = getShaderTypeName(shader_type);
			std::string shader_source   = getShaderSource(*it);
			m_testCtx.getLog() << tcu::TestLog::Message << shader_type_str << " source:\n\n"
							   << shader_source << "\n\n"
							   << tcu::TestLog::EndMessage;
		}

		return false;
	}

	return true;
}

/** Builds an OpenGL ES program by configuring contents of 1 shader object,
 *  compiling it, attaching to specified program object, and finally
 *  by linking the program object.
 *
 *  Implementation assumes all aforementioned objects have already been
 *  generated.
 *
 *  @param po_id            ID of program object
 *  @param sh1_shader_id    ID of first shader to configure.
 *  @param n_sh1_body_parts Number of elements of @param sh1_body_parts array.
 *  @param sh1_body_parts   Pointer to array of strings to make up first shader's body.
 *                          Can be NULL.
 *
 *  @return GTFtrue if successful, false otherwise.
 */
bool TestCaseBase::buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
								const char* const* sh1_body_parts, bool* out_has_compilation_failed)
{
	return buildProgramVA(po_id, out_has_compilation_failed, 1, sh1_shader_id, n_sh1_body_parts, sh1_body_parts);
}

/** Builds an OpenGL ES program by configuring contents of 2 shader objects,
 *  compiling them, attaching to specified program object, and finally
 *  by linking the program object.
 *
 *  Implementation assumes all aforementioned objects have already been
 *  generated.
 *
 *  @param po_id            ID of program object
 *  @param sh1_shader_id    ID of first shader to configure.
 *  @param n_sh1_body_parts Number of elements of @param sh1_body_parts array.
 *  @param sh1_body_parts   Pointer to array of strings to make up first shader's body.
 *                          Can be NULL.
 *  @param sh2_shader_id    ID of second shader to configure.
 *  @param n_sh2_body_parts Number of elements of @param sh2_body_parts array.
 *  @param sh2_body_parts   Pointer to array of strings to make up second shader's body.
 *                          Can be NULL.
 *
 *  @return GTFtrue if successful, false otherwise.
 */
bool TestCaseBase::buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
								const char* const* sh1_body_parts, glw::GLuint sh2_shader_id,
								unsigned int n_sh2_body_parts, const char* const* sh2_body_parts,
								bool* out_has_compilation_failed)
{
	return buildProgramVA(po_id, out_has_compilation_failed, 2, sh1_shader_id, n_sh1_body_parts, sh1_body_parts,
						  sh2_shader_id, n_sh2_body_parts, sh2_body_parts);
}

/** Builds an OpenGL ES program by configuring contents of 3 shader objects,
 *  compiling them, attaching to specified program object, and finally
 *  by linking the program object.
 *
 *  Implementation assumes all aforementioned objects have already been
 *  generated.
 *
 *  @param po_id                  ID of program object
 *  @param sh1_shader_id          ID of first shader to configure.
 *  @param n_sh1_body_parts       Number of elements of @param sh1_body_parts array.
 *  @param sh1_body_parts         Pointer to array of strings to make up first shader's body.
 *                                Can be NULL.
 *  @param sh2_shader_id          ID of second shader to configure.
 *  @param n_sh2_body_parts       Number of elements of @param sh2_body_parts array.
 *  @param sh2_body_parts         Pointer to array of strings to make up second shader's body.
 *                                Can be NULL.
 *  @param sh3_shader_id          ID of third shader to configure.
 *  @param n_sh3_body_parts       Number of elements of @param sh3_body_parts array.
 *  @param sh3_body_parts         Pointer to array of strings to make up third shader's body.
 *                                Can be NULL.
 *  @param has_compilation_failed Deref will be set to true if shader compilation failed,
 *                                false if shader compilation was successful. Can be NULL.
 *
 *  @return GTFtrue if successful, false otherwise.
 */
bool TestCaseBase::buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
								const char* const* sh1_body_parts, glw::GLuint sh2_shader_id,
								unsigned int n_sh2_body_parts, const char* const* sh2_body_parts,
								glw::GLuint sh3_shader_id, unsigned int n_sh3_body_parts,
								const char* const* sh3_body_parts, bool* out_has_compilation_failed)
{
	return buildProgramVA(po_id, out_has_compilation_failed, 3, sh1_shader_id, n_sh1_body_parts, sh1_body_parts,
						  sh2_shader_id, n_sh2_body_parts, sh2_body_parts, sh3_shader_id, n_sh3_body_parts,
						  sh3_body_parts);
}

/** Builds an OpenGL ES program by configuring contents of 4 shader objects,
 *  compiling them, attaching to specified program object, and finally
 *  by linking the program object.
 *
 *  Implementation assumes all aforementioned objects have already been
 *  generated.
 *
 *  @param po_id            ID of program object
 *  @param sh1_shader_id    ID of first shader to configure.
 *  @param n_sh1_body_parts Number of elements of @param sh1_body_parts array.
 *  @param sh1_body_parts   Pointer to array of strings to make up first shader's body.
 *                          Can be NULL.
 *  @param sh2_shader_id    ID of second shader to configure.
 *  @param n_sh2_body_parts Number of elements of @param sh2_body_parts array.
 *  @param sh2_body_parts   Pointer to array of strings to make up second shader's body.
 *                          Can be NULL.
 *  @param sh3_shader_id    ID of third shader to configure.
 *  @param n_sh3_body_parts Number of elements of @param sh3_body_parts array.
 *  @param sh3_body_parts   Pointer to array of strings to make up third shader's body.
 *                          Can be NULL.
 *  @param sh4_shader_id    ID of fourth shader to configure.
 *  @param n_sh4_body_parts Number of elements of @param sh4_body_parts array.
 *  @param sh4_body_parts   Pointer to array of strings to make up fourth shader's body.
 *                          Can be NULL.
 *
 *  @return GTFtrue if successful, false otherwise.
 */
bool TestCaseBase::buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
								const char* const* sh1_body_parts, glw::GLuint sh2_shader_id,
								unsigned int n_sh2_body_parts, const char* const* sh2_body_parts,
								glw::GLuint sh3_shader_id, unsigned int n_sh3_body_parts,
								const char* const* sh3_body_parts, glw::GLuint sh4_shader_id,
								unsigned int n_sh4_body_parts, const char* const* sh4_body_parts,
								bool* out_has_compilation_failed)
{
	return buildProgramVA(po_id, out_has_compilation_failed, 4, sh1_shader_id, n_sh1_body_parts, sh1_body_parts,
						  sh2_shader_id, n_sh2_body_parts, sh2_body_parts, sh3_shader_id, n_sh3_body_parts,
						  sh3_body_parts, sh4_shader_id, n_sh4_body_parts, sh4_body_parts);
}

/** Builds an OpenGL ES program by configuring contents of 5 shader objects,
 *  compiling them, attaching to specified program object, and finally
 *  by linking the program object.
 *
 *  Implementation assumes all aforementioned objects have already been
 *  generated.
 *
 *  @param po_id            ID of program object
 *  @param sh1_shader_id    ID of first shader to configure.
 *  @param n_sh1_body_parts Number of elements of @param sh1_body_parts array.
 *  @param sh1_body_parts   Pointer to array of strings to make up first shader's body.
 *                          Can be NULL.
 *  @param sh2_shader_id    ID of second shader to configure.
 *  @param n_sh2_body_parts Number of elements of @param sh2_body_parts array.
 *  @param sh2_body_parts   Pointer to array of strings to make up second shader's body.
 *                          Can be NULL.
 *  @param sh3_shader_id    ID of third shader to configure.
 *  @param n_sh3_body_parts Number of elements of @param sh3_body_parts array.
 *  @param sh3_body_parts   Pointer to array of strings to make up third shader's body.
 *                          Can be NULL.
 *  @param sh4_shader_id    ID of fourth shader to configure.
 *  @param n_sh4_body_parts Number of elements of @param sh4_body_parts array.
 *  @param sh4_body_parts   Pointer to array of strings to make up fourth shader's body.
 *                          Can be NULL.
 *  @param sh5_shader_id    ID of fifth shader to configure.
 *  @param n_sh5_body_parts Number of elements of @param sh5_body_parts array.
 *  @param sh5_body_parts   Pointer to array of strings to make up fifth shader's body.
 *                          Can be NULL.
 *
 *  @return GTFtrue if successful, false otherwise.
 */
bool TestCaseBase::buildProgram(glw::GLuint po_id, glw::GLuint sh1_shader_id, unsigned int n_sh1_body_parts,
								const char* const* sh1_body_parts, glw::GLuint sh2_shader_id,
								unsigned int n_sh2_body_parts, const char* const* sh2_body_parts,
								glw::GLuint sh3_shader_id, unsigned int n_sh3_body_parts,
								const char* const* sh3_body_parts, glw::GLuint sh4_shader_id,
								unsigned int n_sh4_body_parts, const char* const* sh4_body_parts,
								glw::GLuint sh5_shader_id, unsigned int n_sh5_body_parts,
								const char* const* sh5_body_parts, bool* out_has_compilation_failed)
{
	return buildProgramVA(po_id, out_has_compilation_failed, 5, sh1_shader_id, n_sh1_body_parts, sh1_body_parts,
						  sh2_shader_id, n_sh2_body_parts, sh2_body_parts, sh3_shader_id, n_sh3_body_parts,
						  sh3_body_parts, sh4_shader_id, n_sh4_body_parts, sh4_body_parts, sh5_shader_id,
						  n_sh5_body_parts, sh5_body_parts);
}

/** Compare pixel's color with specified value.
 *  Assumptions:
 *  - size of each channel is 1 byte
 *  - channel order is R G B A
 *  - lines are stored one after another, without any additional data
 *
 * @param buffer            Image data
 * @param x                 X coordinate of pixel
 * @param y                 Y coordinate of pixel
 * @param width             Image width
 * @param height            Image height
 * @param pixel_size        Size of single pixel in bytes, eg. for RGBA8 it should be set to 4
 * @param expected_red      Expected value of red channel, default is 0
 * @param expected_green    Expected value of green channel, default is 0
 * @param expected_blue     Expected value of blue channel, default is 0
 * @param expected_alpha    Expected value of alpha channel, default is 0
 *
 * @retrun true    When pixel color matches expected values
 *         false   When:
 *                  - buffer is null_ptr
 *                  - offset of pixel exceeds size of image
 *                  - pixel_size is not in range <1 ; 4>
 *                  - pixel color does not match expected values
 **/
bool TestCaseBase::comparePixel(const unsigned char* buffer, unsigned int x, unsigned int y, unsigned int width,
								unsigned int height, unsigned int pixel_size, unsigned char expected_red,
								unsigned char expected_green, unsigned char expected_blue,
								unsigned char expected_alpha) const
{
	const unsigned int line_size	= width * pixel_size;
	const unsigned int image_size   = height * line_size;
	const unsigned int texel_offset = y * line_size + x * pixel_size;

	bool result = true;

	/* Sanity checks */
	if (0 == buffer)
	{
		return false;
	}

	if (image_size < texel_offset)
	{
		return false;
	}

	switch (pixel_size)
	{
	/* Fall through by design */
	case 4:
	{
		result &= (expected_alpha == buffer[texel_offset + 3]);
	}

	case 3:
	{
		result &= (expected_blue == buffer[texel_offset + 2]);
	}

	case 2:
	{
		result &= (expected_green == buffer[texel_offset + 1]);
	}

	case 1:
	{
		result &= (expected_red == buffer[texel_offset + 0]);

		break;
	}

	default:
	{
		return false;
	}
	} /* switch (pixel_size) */

	return result;
}

/** Checks whether a combination of fragment/geometry/vertex shader objects compiles and links into a program
 *
 *  @param n_fs_body_parts Number of elements of @param fs_body_parts array.
 *  @param fs_body_parts   Pointer to array of strings to make up fragment shader's body.
 *                         Must not be NULL.
 *
 *  @param n_gs_body_parts Number of elements of @param gs_body_parts array.
 *  @param gs_body_parts   Pointer to array of strings to make up geometry shader's body.
 *                         Can be NULL.
 *
 *  @param n_vs_body_parts Number of elements of @param vs_body_parts array.
 *  @param vs_body_parts   Pointer to array of strings to make up vertex shader's body.
 *                         Must not be NULL.
 *
 *  @return true if program creation was successful, false otherwise.
 **/
bool TestCaseBase::doesProgramBuild(unsigned int n_fs_body_parts, const char* const* fs_body_parts,
									unsigned int n_gs_body_parts, const char* const* gs_body_parts,
									unsigned int n_vs_body_parts, const char* const* vs_body_parts)
{
	/* General variables */
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = false;

	/* Shaders */
	glw::GLuint vertex_shader_id   = 0;
	glw::GLuint geometry_shader_id = 0;
	glw::GLuint fragment_shader_id = 0;

	/* Program */
	glw::GLuint program_object_id = 0;

	/* Create shaders */
	vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);
	geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);

	/* Create program */
	program_object_id = gl.createProgram();

	/* Check createProgram call for errors */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Compile and link the program */
	result = buildProgram(program_object_id, fragment_shader_id, n_fs_body_parts, fs_body_parts, geometry_shader_id,
						  n_gs_body_parts, gs_body_parts, vertex_shader_id, n_vs_body_parts, vs_body_parts);

	if (program_object_id != 0)
		gl.deleteProgram(program_object_id);
	if (fragment_shader_id != 0)
		gl.deleteShader(fragment_shader_id);
	if (geometry_shader_id != 0)
		gl.deleteShader(geometry_shader_id);
	if (vertex_shader_id != 0)
		gl.deleteShader(vertex_shader_id);

	return result;
}

/** Retrieves source for a shader object with GLES id @param shader_id.
 *
 *  @param shader_id GLES id of a shader object to retrieve source for.
 *
 *  @return String instance containing the shader source.
 **/
std::string TestCaseBase::getShaderSource(glw::GLuint shader_id)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint length = 0;

	gl.getShaderiv(shader_id, GL_SHADER_SOURCE_LENGTH, &length);

	std::vector<char> result_vec(length + 1);

	gl.getShaderSource(shader_id, length + 1, NULL, &result_vec[0]);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve shader source!");

	return std::string(&result_vec[0]);
}

/** Retrieves compilation info log for a shader object with GLES id
 *  @param shader_id.
 *
 *  @param shader_id GLES id of a shader object to retrieve compilation
 *          info log for.
 *
 *  @return String instance containing the log.
 **/
std::string TestCaseBase::getCompilationInfoLog(glw::GLuint shader_id)
{
	return getInfoLog(LT_SHADER_OBJECT, shader_id);
}

/** Retrieves linking info log for a program object with GLES id
 *  @param po_id.
 *
 *  @param po_id GLES id of a program object to retrieve linking
 *               info log for.
 *
 *  @return String instance containing the log.
 **/
std::string TestCaseBase::getLinkingInfoLog(glw::GLuint po_id)
{
	return getInfoLog(LT_PROGRAM_OBJECT, po_id);
}

/** Retrieves linking info log for a pipeline object with GLES id
 *  @param ppo_id.
 *
 *  @param ppo_id GLES id of a pipeline object to retrieve validation
 *               info log for.
 *
 *  @return String instance containing the log.
 **/
std::string TestCaseBase::getPipelineInfoLog(glw::GLuint ppo_id)
{
	return getInfoLog(LT_PIPELINE_OBJECT, ppo_id);
}

/** Retrieves compilation OR linking info log for a shader/program object with GLES id
 *  @param id.
 *
 *  @param is_compilation_info_log true if @param id is a GLES id of a shader object;
 *                                 false if it represents a program object.
 *  @param id                      GLES id of a shader OR a program object to
 *                                 retrieve info log for.
 *
 *  @return String instance containing the log..
 **/
std::string TestCaseBase::getInfoLog(LOG_TYPE log_type, glw::GLuint id)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint n_characters = 0;
	/* Retrieve amount of characters needed to store the info log (terminator-inclusive) */
	switch (log_type)
	{
	case LT_SHADER_OBJECT:
		gl.getShaderiv(id, GL_INFO_LOG_LENGTH, &n_characters);
		break;
	case LT_PROGRAM_OBJECT:
		gl.getProgramiv(id, GL_INFO_LOG_LENGTH, &n_characters);
		break;
	case LT_PIPELINE_OBJECT:
		gl.getProgramPipelineiv(id, GL_INFO_LOG_LENGTH, &n_characters);
		break;
	default:
		TCU_FAIL("Invalid parameter");
	}

	/* Check if everything is fine so far */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query info log length!");

	/* Allocate buffer */
	std::vector<char> result_vec(n_characters + 1);

	/* Retrieve the info log */
	switch (log_type)
	{
	case LT_SHADER_OBJECT:
		gl.getShaderInfoLog(id, n_characters + 1, 0, &result_vec[0]);
		break;
	case LT_PROGRAM_OBJECT:
		gl.getProgramInfoLog(id, n_characters + 1, 0, &result_vec[0]);
		break;
	case LT_PIPELINE_OBJECT:
		gl.getProgramPipelineInfoLog(id, n_characters + 1, 0, &result_vec[0]);
		break;
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve info log!");

	return std::string(&result_vec[0]);
}

/** Setup frame buffer:
 * 1 allocate texture storage for specified format and dimensions,
 * 2 bind framebuffer and attach texture to GL_COLOR_ATTACHMENT0
 * 3 setup viewport to specified dimensions
 *
 * @param framebuffer_object_id FBO handle
 * @param color_texture_id      Texture handle
 * @param texture_format        Requested texture format, eg. GL_RGBA8
 * @param texture_width         Requested texture width
 * @param texture_height        Requested texture height
 *
 * @return true  All operations succeded
 *         false In case of any error
 **/
bool TestCaseBase::setupFramebufferWithTextureAsAttachment(glw::GLuint framebuffer_object_id,
														   glw::GLuint color_texture_id, glw::GLenum texture_format,
														   glw::GLuint texture_width, glw::GLuint texture_height) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Allocate texture storage */
	gl.bindTexture(GL_TEXTURE_2D, color_texture_id);
	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, texture_format, texture_width, texture_height);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not allocate texture storage!");

	/* Setup framebuffer */
	gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_object_id);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture_id, 0 /* level */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not setup framebuffer!");

	/* Setup viewport */
	gl.viewport(0, 0, texture_width, texture_height);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not setup viewport!");

	/* Success */
	return true;
}

/** Check Framebuffer Status.
 *   Throws a TestError exception, should the framebuffer be found incomplete.
 *
 *  @param framebuffer - GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER or GL_FRAMEBUFFER
 *
 */
void TestCaseBase::checkFramebufferStatus(glw::GLenum framebuffer) const
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum framebuffer_status = gl.checkFramebufferStatus(framebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting framebuffer status!");

	if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status)
	{
		switch (framebuffer_status)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
		}

		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
		}

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
		}

		case GL_FRAMEBUFFER_UNSUPPORTED:
		{
			TCU_FAIL("Framebuffer incomplete, status: Error: GL_FRAMEBUFFER_UNSUPPORTED");
		}

		default:
		{
			TCU_FAIL("Framebuffer incomplete, status not recognized");
		}
		} /* switch (framebuffer_status) */
	}	 /* if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status) */
}

std::string TestCaseBase::getGLSLExtDirective(ExtensionType type, ExtensionName name, ExtensionBehavior behavior)
{
	if (type == EXTENSIONTYPE_NONE && name != EXTENSIONNAME_GEOMETRY_POINT_SIZE &&
		name != EXTENSIONNAME_TESSELLATION_POINT_SIZE)
	{
		return "";
	}

	const char* type_str	 = NULL;
	const char* name_str	 = NULL;
	const char* behavior_str = NULL;

	if (name == EXTENSIONNAME_SHADER_IMAGE_ATOMIC)
	{
		// There is no EXT version of shader_image_atomic; use OES
		type = EXTENSIONTYPE_OES;
	}

	if (name == EXTENSIONNAME_TESSELLATION_POINT_SIZE)
	{
		// there is no core version of tessellation_point_size, use OES or EXT
		if (isExtensionSupported("GL_OES_tessellation_point_size"))
		{
			type = EXTENSIONTYPE_OES;
		}
		else if (isExtensionSupported("GL_EXT_tessellation_point_size"))
		{
			type = EXTENSIONTYPE_EXT;
		}
		else
		{
			return "";
		}
	}

	if (name == EXTENSIONNAME_GEOMETRY_POINT_SIZE)
	{
		// there is no core version of geometry_point_size, use OES or EXT
		if (isExtensionSupported("GL_OES_geometry_point_size"))
		{
			type = EXTENSIONTYPE_OES;
		}
		else if (isExtensionSupported("GL_EXT_geometry_point_size"))
		{
			type = EXTENSIONTYPE_EXT;
		}
		else
		{
			return "";
		}
	}

	switch (type)
	{
	case EXTENSIONTYPE_EXT:
		type_str = "EXT_";
		break;
	case EXTENSIONTYPE_OES:
		type_str = "OES_";
		break;
	default:
		DE_ASSERT(0);
		return "#error unknown extension type\n";
	}

	switch (name)
	{
	case EXTENSIONNAME_SHADER_IMAGE_ATOMIC:
		name_str = "shader_image_atomic";
		break;
	case EXTENSIONNAME_SHADER_IO_BLOCKS:
		name_str = "shader_io_blocks";
		break;
	case EXTENSIONNAME_GEOMETRY_SHADER:
		name_str = "geometry_shader";
		break;
	case EXTENSIONNAME_GEOMETRY_POINT_SIZE:
		name_str = "geometry_point_size";
		break;
	case EXTENSIONNAME_TESSELLATION_SHADER:
		name_str = "tessellation_shader";
		break;
	case EXTENSIONNAME_TESSELLATION_POINT_SIZE:
		name_str = "tessellation_point_size";
		break;
	case EXTENSIONNAME_TEXTURE_BUFFER:
		name_str = "texture_buffer";
		break;
	case EXTENSIONNAME_TEXTURE_CUBE_MAP_ARRAY:
		name_str = "texture_cube_map_array";
		break;
	case EXTENSIONNAME_GPU_SHADER5:
		name_str = "gpu_shader5";
		break;
	case EXTENSIONNAME_VIEWPORT_ARRAY:
		name_str = "viewport_array";
		break;
	default:
		DE_ASSERT(0);
		return "#error unknown extension name\n";
	}

	switch (behavior)
	{
	case EXTENSIONBEHAVIOR_DISABLE:
		behavior_str = "disable";
		break;
	case EXTENSIONBEHAVIOR_WARN:
		behavior_str = "warn";
		break;
	case EXTENSIONBEHAVIOR_ENABLE:
		behavior_str = "enable";
		break;
	case EXTENSIONBEHAVIOR_REQUIRE:
		behavior_str = "require";
		break;
	default:
		DE_ASSERT(0);
		return "#error unknown extension behavior";
	}

	std::stringstream str;
	str << "#extension GL_" << type_str << name_str << " : " << behavior_str;
	return str.str();
}

} // namespace glcts
