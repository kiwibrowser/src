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

/*!
 * \file  esextcTextureBufferActiveUniformValidation.cpp
 * \brief Texture Buffer - Active Uniform Value Validation (Test 8)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferActiveUniformValidation.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>
#include <map>

namespace glcts
{

/* Buffer size for uniform variable name */
const glw::GLuint TextureBufferActiveUniformValidation::m_param_value_size = 100;

/** Constructor
 *
 **/
TextureParameters::TextureParameters() : m_texture_buffer_size(0), m_texture_format(0), m_texture_uniform_type(0)
{
}

/** Constructor
 *
 *  @param textureBufferSize  size of buffer object
 *  @param textureFormat      texture format
 *  @param textureUniforType  texture uniform type
 *  @param uniformName        pointer to literal containing uniform name
 **/
TextureParameters::TextureParameters(glw::GLuint textureBufferSize, glw::GLenum textureFormat,
									 glw::GLenum textureUniforType, const char* uniformName)
{
	m_texture_buffer_size  = textureBufferSize;
	m_texture_format	   = textureFormat;
	m_texture_uniform_type = textureUniforType;
	m_uniform_name		   = uniformName;
}

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferActiveUniformValidation::TextureBufferActiveUniformValidation(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_po_id(0), m_tbo_ids(0), m_tbo_tex_ids(0)
{
	/* Nothing to be done here */
}

/** Add parameters to the vector of texture parameters
 *
 * @param uniformType enum with type of uniform
 * @param format      enum with texture format
 * @param size        texture size
 * @param name        uniform name
 * @param params      pointer to vector where parameters will be added
 */
void TextureBufferActiveUniformValidation::addTextureParam(glw::GLenum uniformType, glw::GLenum format,
														   glw::GLuint size, const char* name,
														   std::vector<TextureParameters>* params)
{
	TextureParameters texParam(size, format, uniformType, name);
	params->push_back(texParam);
}

/** Initializes GLES objects used during the test.
 *
 */
void TextureBufferActiveUniformValidation::initTest(void)
{
	/* Check if required extensions are supported */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Call specific implementation to configure texture params */
	configureParams(&m_texture_params);

	m_tbo_tex_ids = new glw::GLuint[m_texture_params.size()];
	m_tbo_ids	 = new glw::GLuint[m_texture_params.size()];

	memset(m_tbo_tex_ids, 0, m_texture_params.size() * sizeof(glw::GLuint));
	memset(m_tbo_ids, 0, m_texture_params.size() * sizeof(glw::GLuint));

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create buffers and textures */
	for (glw::GLuint i = 0; i < m_texture_params.size(); ++i)
	{
		/* Create buffer object*/
		gl.genBuffers(1, &m_tbo_ids[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
		gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_tbo_ids[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object !");
		gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_texture_params[i].get_texture_buffer_size(), 0, GL_DYNAMIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

		/* Create texture buffer */
		gl.genTextures(1, &m_tbo_tex_ids[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");
		gl.activeTexture(GL_TEXTURE0 + i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error activating texture unit!");
		gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_tbo_tex_ids[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");
		gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, m_texture_params[i].get_texture_format(), m_tbo_ids[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting buffer object as data store for texture buffer!");
	}

	/* Create program */
	createProgram();
}

/** Returns uniform type name
 *
 * @param  uniformType enum value of uniform type
 * @return             pointer to literal with uniform type name
 */
const char* TextureBufferActiveUniformValidation::getUniformTypeName(glw::GLenum uniformType)
{
	static const char* str_GL_SAMPLER_BUFFER_EXT			  = "GL_SAMPLER_BUFFER_EXT";
	static const char* str_GL_INT_SAMPLER_BUFFER_EXT		  = "GL_INT_SAMPLER_BUFFER_EXT";
	static const char* str_GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT = "GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT";
	static const char* str_GL_IMAGE_BUFFER_EXT				  = "GL_IMAGE_BUFFER_EXT";
	static const char* str_GL_INT_IMAGE_BUFFER_EXT			  = "GL_INT_IMAGE_BUFFER_EXT";
	static const char* str_GL_UNSIGNED_INT_IMAGE_BUFFER_EXT   = "GL_UNSIGNED_INT_IMAGE_BUFFER_EXT";
	static const char* str_UNKNOWN							  = "UNKNOWN";

	if (uniformType == m_glExtTokens.SAMPLER_BUFFER)
	{
		return str_GL_SAMPLER_BUFFER_EXT;
	}
	else if (uniformType == m_glExtTokens.INT_SAMPLER_BUFFER)
	{
		return str_GL_INT_SAMPLER_BUFFER_EXT;
	}
	else if (uniformType == m_glExtTokens.UNSIGNED_INT_SAMPLER_BUFFER)
	{
		return str_GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT;
	}
	else if (uniformType == m_glExtTokens.IMAGE_BUFFER)
	{
		return str_GL_IMAGE_BUFFER_EXT;
	}
	else if (uniformType == m_glExtTokens.INT_IMAGE_BUFFER)
	{
		return str_GL_INT_IMAGE_BUFFER_EXT;
	}
	else if (uniformType == m_glExtTokens.UNSIGNED_INT_IMAGE_BUFFER)
	{
		return str_GL_UNSIGNED_INT_IMAGE_BUFFER_EXT;
	}
	else
	{
		return str_UNKNOWN;
	}
}

/** Returns pointer to texture parameters for specific uniform type
 *
 * @param uniformType  enum specifying unform type
 *
 * @return             if TextureParameters for specific uniformType was found returns pointer to the element, otherwise return NULL
 */
const TextureParameters* TextureBufferActiveUniformValidation::getParamsForType(glw::GLenum uniformType) const
{
	for (glw::GLuint i = 0; i < m_texture_params.size(); ++i)
	{
		if (m_texture_params[i].get_texture_uniform_type() == uniformType)
		{
			return &m_texture_params[i];
		}
	}
	return DE_NULL;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBufferActiveUniformValidation::iterate(void)
{
	/* Initialize */
	initTest();

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool testResult = true;

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

	/* Configure Program */
	configureProgram(&m_texture_params, m_tbo_tex_ids);

	/* Get number of active uniforms for current program */
	glw::GLint n_active_uniforms;

	gl.getProgramiv(m_po_id, GL_ACTIVE_UNIFORMS, &n_active_uniforms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting value of GL_ACTIVE_UNIFORMS!");

	if ((glw::GLuint)n_active_uniforms != m_texture_params.size())
	{
		/* Log error if number of active uniforms different than expected */
		m_testCtx.getLog() << tcu::TestLog::Message << "Result is different than expected!\n"
						   << "Expected number of active uniforms: " << m_texture_params.size() << "\n"
						   << "Result   number of active uniforms: " << n_active_uniforms << "\n"
						   << tcu::TestLog::EndMessage;

		testResult = false;
	}

	/* Retrieve parameters for specific indices */
	std::vector<glw::GLchar> nameValue(m_param_value_size);
	glw::GLsizei			 paramLength = 0;
	glw::GLsizei			 uniformSize = 0;
	glw::GLenum				 uniformType;

	/* store map of indices and uniform types */
	std::map<glw::GLuint, glw::GLenum> resultTypes;

	for (glw::GLuint i = 0; i < (glw::GLuint)n_active_uniforms; ++i)
	{
		gl.getActiveUniform(m_po_id, i /* index */, (glw::GLsizei)(m_param_value_size - 1), &paramLength, &uniformSize,
							&uniformType, &nameValue[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting information about active uniform variable!");

		/*Check if returned uniform type is one of types defined in current program*/
		const TextureParameters* param = getParamsForType(uniformType);

		if (0 == param)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Following uniform type was not expected to be defined in current program : \n"
							   << getUniformTypeName(uniformType) << "\n"
							   << tcu::TestLog::EndMessage;
			testResult = false;
		}
		else if (strncmp(&nameValue[0], param->get_uniform_name().c_str(), m_param_value_size))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "For :" << getUniformTypeName(uniformType) << " type name \n"
							   << "expected  uniform name is: " << param->get_uniform_name().c_str() << "\n"
							   << "result    uniform name is: " << &nameValue[0] << "\n"
							   << tcu::TestLog::EndMessage;
			testResult = false;
		}

		resultTypes[i] = uniformType;
	}

	/* Check if all uniform types defined in program were returned */
	for (glw::GLuint i = 0; i < (glw::GLuint)n_active_uniforms; ++i)
	{
		/* Log error if expected uniform type is missing */
		std::map<glw::GLuint, glw::GLenum>::iterator it = resultTypes.begin();
		for (; it != resultTypes.end(); ++it)
		{
			if (it->second == m_texture_params[i].get_texture_uniform_type())
			{
				break;
			}
		}

		/* Log if there is some missing uniform type */
		if (it == resultTypes.end())
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Following uniform type is missing from active uniforms list: "
							   << getUniformTypeName(m_texture_params[i].get_texture_uniform_type()) << "\n"
							   << tcu::TestLog::EndMessage;

			testResult = false;
		}
	}

	/* Get all active uniform types using glGetActiveUniformsiv and compare with results from glGetActiveUniform function */
	std::vector<glw::GLuint> indicies(n_active_uniforms);
	std::vector<glw::GLint>  types(n_active_uniforms);

	for (glw::GLuint i = 0; i < (glw::GLuint)n_active_uniforms; ++i)
	{
		indicies[i] = i;
	}

	gl.getActiveUniformsiv(m_po_id, n_active_uniforms, &indicies[0], GL_UNIFORM_TYPE, &types[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting information about active uniform variables!");

	for (glw::GLuint i = 0; i < (glw::GLuint)n_active_uniforms; ++i)
	{
		/* Log error if expected result is different from expected*/
		if (resultTypes[i] != (glw::GLuint)types[i])
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Wrong uniform type for index(" << i << ")\n"
							   << "expected uniform type: " << getUniformTypeName(resultTypes[i]) << "\n"
							   << "result   uniform type: " << getUniformTypeName(types[i]) << "\n"
							   << tcu::TestLog::EndMessage;

			testResult = false;
		}
	}

	glw::GLenum paramVal = GL_TYPE;
	glw::GLint  type	 = -1;

	for (glw::GLuint i = 0; i < (glw::GLuint)n_active_uniforms; ++i)
	{
		gl.getProgramResourceiv(m_po_id, GL_UNIFORM, i /*index */, 1 /* parameters count */,
								&paramVal /* parameter enum */, 1 /* buffer size */, 0, &type);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting information about one of program resources!");

		if (resultTypes[i] != (glw::GLuint)type)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Wrong uniform type for index(" << i << ")\n"
							   << "expected uniform type: " << getUniformTypeName(resultTypes[i]) << "\n"
							   << "result   uniform type: " << getUniformTypeName(type) << "\n"
							   << tcu::TestLog::EndMessage;
			testResult = false;
		}
	}

	if (testResult)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBufferActiveUniformValidation::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);

	for (glw::GLuint i = 0; i < m_texture_params.size(); ++i)
	{
		gl.activeTexture(GL_TEXTURE0 + i);
		gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, 0);
	}
	gl.activeTexture(GL_TEXTURE0);

	/* Delete GLES objects */
	if (0 != m_po_id)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (0 != m_tbo_tex_ids)
	{
		gl.deleteTextures((glw::GLsizei)m_texture_params.size(), m_tbo_tex_ids);
		delete[] m_tbo_tex_ids;
		m_tbo_tex_ids = 0;
	}

	if (0 != m_tbo_ids)
	{
		gl.deleteBuffers((glw::GLsizei)m_texture_params.size(), m_tbo_ids);
		delete[] m_tbo_ids;
		m_tbo_ids = 0;
	}

	m_texture_params.clear();

	/* Call base class' deinit() */
	TestCaseBase::deinit();
}

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferActiveUniformValidationVSFS::TextureBufferActiveUniformValidationVSFS(Context&				context,
																				   const ExtParameters& extParams,
																				   const char*			name,
																				   const char*			description)
	: TextureBufferActiveUniformValidation(context, extParams, name, description), m_fs_id(0), m_vs_id(0)
{

	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 **/
void TextureBufferActiveUniformValidationVSFS::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (0 != m_po_id)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (0 != m_fs_id)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}

	if (0 != m_vs_id)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	/* Call base class' deinit() */
	TextureBufferActiveUniformValidation::deinit();
}

/** Returns Fragment shader Code
 *
 * @return pointer to literal with Fragment Shader Code
 **/
const char* TextureBufferActiveUniformValidationVSFS::getFragmentShaderCode() const
{
	const char* result = "${VERSION}\n"
						 "\n"
						 "${TEXTURE_BUFFER_REQUIRE}\n"
						 "\n"
						 "precision highp float;\n"
						 "\n"
						 "uniform highp samplerBuffer  sampler_buffer;\n"
						 "uniform highp isamplerBuffer isampler_buffer;\n"
						 "uniform highp usamplerBuffer usampler_buffer;\n"
						 "\n"
						 "layout(location = 0) out vec4 outColor;\n"
						 "void main(void)\n"
						 "{\n"
						 "    outColor =  texelFetch(sampler_buffer, 0);\n"
						 "    outColor += vec4(texelFetch(isampler_buffer, 0));\n"
						 "    outColor += vec4(texelFetch(usampler_buffer, 0));\n"
						 "}\n";

	return result;
}

/** Returns Vertex shader Code
 *
 * @return pointer to literal with Vertex Shader Code
 **/
const char* TextureBufferActiveUniformValidationVSFS::getVertexShaderCode() const
{
	const char* result = "${VERSION}\n"
						 "\n"
						 "precision highp float;\n"
						 "\n"
						 "void main(void)\n"
						 "{\n"
						 "    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);\n"
						 "}\n";

	return result;
}

/** Configure Texture parameters for test
 *
 * @param params  pointer to the buffer where parameters will be added
 *
 **/
void TextureBufferActiveUniformValidationVSFS::configureParams(std::vector<TextureParameters>* params)
{
	addTextureParam(m_glExtTokens.SAMPLER_BUFFER, GL_R32F, sizeof(glw::GLfloat), "sampler_buffer", params);
	addTextureParam(m_glExtTokens.INT_SAMPLER_BUFFER, GL_R32I, sizeof(glw::GLint), "isampler_buffer", params);
	addTextureParam(m_glExtTokens.UNSIGNED_INT_SAMPLER_BUFFER, GL_R32UI, sizeof(glw::GLuint), "usampler_buffer",
					params);
}

/** Create program used for test
 *
 **/
void TextureBufferActiveUniformValidationVSFS::createProgram(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

	const char* fsCode = getFragmentShaderCode();
	const char* vsCode = getVertexShaderCode();

	if (!buildProgram(m_po_id, m_fs_id, 1, &fsCode, m_vs_id, 1, &vsCode))
	{
		TCU_FAIL("Error building a program!");
	}
}

/** Configure Program elements
 *
 * @param params pointer to buffer with texture parameters
 * @param params pointer to textures' ids
 *
 */
void TextureBufferActiveUniformValidationVSFS::configureProgram(std::vector<TextureParameters>* params,
																glw::GLuint*					texIds)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (glw::GLuint i = 0; i < params->size(); ++i)
	{
		glw::GLint location = gl.getUniformLocation(m_po_id, (*params)[i].get_uniform_name().c_str());
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting uniform location!");
		if (location == -1)
		{
			TCU_FAIL("Could not get uniform location for active uniform variable");
		}

		gl.activeTexture(GL_TEXTURE0 + i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error activating texture unit!");
		gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, texIds[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");
		gl.uniform1i(location, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for uniform location!");
	}
}

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferActiveUniformValidationCS::TextureBufferActiveUniformValidationCS(Context&				context,
																			   const ExtParameters& extParams,
																			   const char*			name,
																			   const char*			description)
	: TextureBufferActiveUniformValidation(context, extParams, name, description), m_cs_id(0)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBufferActiveUniformValidationCS::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (0 != m_po_id)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (0 != m_cs_id)
	{
		gl.deleteShader(m_cs_id);
		m_cs_id = 0;
	}

	/* Call base class' deinit() */
	TextureBufferActiveUniformValidation::deinit();
}

/** Returns Compute shader Code
 *
 * @return pointer to literal with Compute Shader Code
 */
const char* TextureBufferActiveUniformValidationCS::getComputeShaderCode() const
{
	const char* result = "${VERSION}\n"
						 "\n"
						 "${TEXTURE_BUFFER_REQUIRE}\n"
						 "\n"
						 "precision highp float;\n"
						 "\n"
						 "layout(r32f)  uniform highp imageBuffer    image_buffer;\n"
						 "layout(r32i)  uniform highp iimageBuffer   iimage_buffer;\n"
						 "layout(r32ui) uniform highp uimageBuffer   uimage_buffer;\n"
						 "\n"
						 "layout (local_size_x = 1) in;\n"
						 "\n"
						 "void main(void)\n"
						 "{\n"
						 "    imageStore(image_buffer,  0, vec4 (1.0, 1.0, 1.0, 1.0));\n"
						 "    imageStore(iimage_buffer, 0, ivec4(1,   1,   1,   1)  );\n"
						 "    imageStore(uimage_buffer, 0, uvec4(1,   1,   1,   1)  );\n"
						 "}\n";

	return result;
}

/** Configure Texture parameters for test
 *
 * @param params  pointer to the buffer where parameters will be added
 *
 **/
void TextureBufferActiveUniformValidationCS::configureParams(std::vector<TextureParameters>* params)
{
	addTextureParam(m_glExtTokens.IMAGE_BUFFER, GL_R32F, sizeof(glw::GLfloat), "image_buffer", params);
	addTextureParam(m_glExtTokens.INT_IMAGE_BUFFER, GL_R32I, sizeof(glw::GLint), "iimage_buffer", params);
	addTextureParam(m_glExtTokens.UNSIGNED_INT_IMAGE_BUFFER, GL_R32UI, sizeof(glw::GLuint), "uimage_buffer", params);
}

/** Create program used for test
 *
 **/
void TextureBufferActiveUniformValidationCS::createProgram(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

	const char* csCode = getComputeShaderCode();

	if (!buildProgram(m_po_id, m_cs_id, 1, &csCode))
	{
		TCU_FAIL("Error building a program!");
	}
}

/** Configure Program elements
 *
 * @param params pointer to buffer with texture parameters
 * @param params pointer to textures' ids
 *
 */
void TextureBufferActiveUniformValidationCS::configureProgram(std::vector<TextureParameters>* params,
															  glw::GLuint*					  texIds)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (glw::GLuint i = 0; i < params->size(); ++i)
	{
		gl.bindImageTexture(i, texIds[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, (*params)[i].get_texture_format());
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture to image unit!");
	}
}

} // namespace glcts
