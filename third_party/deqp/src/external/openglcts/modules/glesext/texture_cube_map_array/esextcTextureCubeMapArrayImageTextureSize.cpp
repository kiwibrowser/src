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
 * \file  esextcTextureCubeMapArrayImageTextureSize.cpp
 * \brief texture_cube_map_array extension - Image Texture Size (Test 10)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayImageTextureSize.hpp"

#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cmath>
#include <iostream>

namespace glcts
{
/* Static const variables used for configuring tests */
const glw::GLuint TextureCubeMapArrayTextureSizeBase::m_n_dimensions		 = 3;
const glw::GLuint TextureCubeMapArrayTextureSizeBase::m_n_resolutions		 = 4;
const glw::GLuint TextureCubeMapArrayTextureSizeBase::m_n_layers_per_cube	= 6;
const glw::GLuint TextureCubeMapArrayTextureSizeBase::m_n_storage_types		 = 2;
const glw::GLuint TextureCubeMapArrayTextureSizeBase::m_n_texture_components = 4;

/* Array with resolutions */
glw::GLuint resolutionArray[TextureCubeMapArrayTextureSizeBase::m_n_resolutions]
						   [TextureCubeMapArrayTextureSizeBase::m_n_dimensions] = { { 32, 32, 18 },
																					{ 64, 64, 6 },
																					{ 128, 128, 12 },
																					{ 256, 256, 12 } };

/* Names of storage types */
static const char* mutableStorage   = "MUTABLE";
static const char* imMutableStorage = "IMMUTABLE";

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeBase::TextureCubeMapArrayTextureSizeBase(Context& context, const ExtParameters& extParams,
																	   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_po_id(0), m_to_std_id(0), m_to_shw_id(0), m_vao_id(0)
{
	/* Nothing to be done here */
}

/** Initialize test case */
void TextureCubeMapArrayTextureSizeBase::initTest(void)
{
	/* Check if texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Create program object */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create program object.");

	/* Create program object */
	configureProgram();

	/* Create GLES objects specific for the test */
	configureTestSpecificObjects();
}

/** Deinitialize test case */
void TextureCubeMapArrayTextureSizeBase::deinit(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset Opengl ES configuration */
	gl.useProgram(0);
	gl.bindVertexArray(0);

	/* Delete GLES objects specific for the test */
	deleteTestSpecificObjects();

	/* Delete shader objects */
	deleteProgram();

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

	/* Delete texture objects */
	deleteTextures();

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestCase::IterateResult TextureCubeMapArrayTextureSizeBase::iterate(void)
{
	/* Initialize test case */
	initTest();

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLboolean test_passed = true;

	/* Use program object */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set active program object.");

	glw::GLuint width  = 0;
	glw::GLuint height = 0;
	glw::GLuint depth  = 0;

	/* Go through IMMUTABLE AND MUTABLE storages */
	for (glw::GLuint i = 0; i < m_n_storage_types; ++i)
	{
		if (!isMutableTextureTestable() && (STORAGE_TYPE)i == ST_MUTABLE)
		{
			continue;
		}

		/* Go through all resolutions */
		for (glw::GLuint j = 0; j < m_n_resolutions; ++j)
		{
			width  = resolutionArray[j][0];
			height = resolutionArray[j][1];
			depth  = resolutionArray[j][2];

			/* Configure texture objects */
			configureTextures(width, height, depth, (STORAGE_TYPE)i);

			/* Configure uniforms */
			configureUniforms();

			/* Run shaders to get texture size */
			runShaders();

			/* Check if results are as expected */
			if (!checkResults(width, height, depth, (STORAGE_TYPE)i))
			{
				test_passed = false;
			}

			/* Delete texture objects used for this iteration */
			deleteTextures();
		}
	}

	/* Set proper test result */
	if (test_passed)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

/** Method to check if the test supports mutable textures.
 *
 *  @return return true if mutable textures work with the test
 **/
glw::GLboolean TextureCubeMapArrayTextureSizeBase::isMutableTextureTestable(void)
{
	return true;
}

/** Method to create texture cube map array with proper configuration
 @param texId    pointer to variable where texture id will be stored
 @param width    texture width
 @param height   texture height
 @param depth    texture depth
 @param storType inform if texture should be mutable or immutable
 @param shadow   inform if texture should be shadow texture or not
 */
void TextureCubeMapArrayTextureSizeBase::createCubeMapArrayTexture(glw::GLuint& texId, glw::GLuint width,
																   glw::GLuint height, glw::GLuint depth,
																   STORAGE_TYPE storType, glw::GLboolean shadow)
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Save the current binding */
	glw::GLuint savedTexId = 0;
	gl.getIntegerv(GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, (glw::GLint*)&savedTexId);

	/* Generate Texture object */
	gl.genTextures(1, &texId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object.");

	/* Bind texture object */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object.");

	/* Create immutable texture storage */
	if (storType == ST_IMMUTABLE)
	{
		/* Create shadow texture object */
		if (shadow)
		{
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture parameter.");

			gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_DEPTH_COMPONENT32F, width, height, depth);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating immutable texture storage.");
		}
		/* Create texture object */
		else
		{
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture parameter.");

			gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA32F, width, height, depth);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating immutable texture storage.");
		}
	}
	/* Create mutable texture storage */
	else
	{
		std::vector<glw::GLfloat> data(width * height * depth * m_n_texture_components, 0);

		/* Create shadow texture object */
		if (shadow)
		{
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture parameter.");

			gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT32F, width, height, depth, 0,
						  GL_DEPTH_COMPONENT, GL_FLOAT, &data[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating mutable texture storage.");
		}
		/* Create texture object */
		else
		{
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture parameter.");

			gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT,
						  &data[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating mutable texture storage.");
		}
	}

	/* Restore the original texture binding */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, savedTexId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object.");
}

/** Configure textures used in the test for textureSize() and imageSize() calls
 @param width    texture width
 @param height   texture height
 @param depth    texture depth
 @param storType inform if texture should be mutable or immutable
 */
void TextureCubeMapArrayTextureSizeBase::configureTextures(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
														   STORAGE_TYPE storType)
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create texture objects which will be tested */
	createCubeMapArrayTexture(m_to_std_id, width, height, depth, storType, false);
	createCubeMapArrayTexture(m_to_shw_id, width, height, depth, storType, true);

	/* Binding texture object to texture unit */
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_to_std_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	/* Binding texture object to texture unit */
	gl.activeTexture(GL_TEXTURE1);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_to_shw_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");
}

/** Delete textures used in the test for textureSize() and imageSize() calls */
void TextureCubeMapArrayTextureSizeBase::deleteTextures(void)
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
	gl.activeTexture(GL_TEXTURE1);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
	gl.activeTexture(GL_TEXTURE0);

	/* Delete GLES objects */
	if (m_to_std_id != 0)
	{
		gl.deleteTextures(1, &m_to_std_id);
		m_to_std_id = 0;
	}

	if (m_to_shw_id != 0)
	{
		gl.deleteTextures(1, &m_to_shw_id);
		m_to_shw_id = 0;
	}
}

/** Configure uniform variables */
void TextureCubeMapArrayTextureSizeBase::configureUniforms(void)
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind uniform samplers to texture units */
	glw::GLint texture_std_location = gl.getUniformLocation(m_po_id, "texture_std_sampler");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting sampler location!");

	gl.uniform1i(texture_std_location, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding sampler to texture unit!");

	glw::GLint texture_shw_location = gl.getUniformLocation(m_po_id, "texture_shw_sampler");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting sampler location!");

	gl.uniform1i(texture_shw_location, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding sampler to texture unit!");
}

/* Static const variables used for configuring tests */
const glw::GLsizei TextureCubeMapArrayTextureSizeTFBase::m_n_varyings	  = 2;
const glw::GLuint  TextureCubeMapArrayTextureSizeTFBase::m_n_tf_components = 3;

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeTFBase::TextureCubeMapArrayTextureSizeTFBase(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TextureCubeMapArrayTextureSizeBase(context, extParams, name, description), m_tf_bo_id(0)
{
	/* Nothing to be done here */
}

/** Configure GLES objects specific for the test configuration */
void TextureCubeMapArrayTextureSizeTFBase::configureTestSpecificObjects(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genBuffers(1, &m_tf_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_tf_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object.");

	std::vector<glw::GLint> buffer_data(m_n_varyings * m_n_tf_components, 0);

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, m_n_varyings * m_n_tf_components * sizeof(glw::GLint), &buffer_data[0],
				  GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_tf_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to transform feedback binding point.");
}

/** Delete GLES objects specific for the test configuration */
void TextureCubeMapArrayTextureSizeTFBase::deleteTestSpecificObjects(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

	/* Delete transform feedback buffer */
	if (m_tf_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_tf_bo_id);
		m_tf_bo_id = 0;
	}
}

/** Configure textures used in the test for textureSize() and imageSize() calls
 @param width    texture width
 @param height   texture height
 @param depth    texture depth
 @param storType inform if texture should be mutable or immutable
 */
void TextureCubeMapArrayTextureSizeTFBase::configureTextures(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
															 STORAGE_TYPE storType)
{
	TextureCubeMapArrayTextureSizeBase::configureTextures(width, height, depth, storType);

	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::vector<glw::GLint> buffer_data(m_n_varyings * m_n_tf_components, 0);

	gl.bufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_n_varyings * m_n_tf_components * sizeof(glw::GLint),
					 &buffer_data[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling buffer object's data store with data.");
}

/** Check textureSize() and imageSize() methods returned proper values
 * @param  width    texture width
 * @param  height   texture height
 * @param  depth    texture depth
 * @param  storType inform if texture is mutable or immutable
 * @return          return true if result data is as expected
 */
glw::GLboolean TextureCubeMapArrayTextureSizeTFBase::checkResults(glw::GLuint width, glw::GLuint height,
																  glw::GLuint depth, STORAGE_TYPE storType)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Read results from transform feedback */
	glw::GLuint* temp_buff = (glw::GLuint*)gl.mapBufferRange(
		GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_n_varyings * m_n_tf_components * sizeof(glw::GLuint), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object's data store to client's address space.");

	/* Copy results to helper buffer */
	glw::GLuint read_size[m_n_varyings * m_n_tf_components];
	memcpy(read_size, temp_buff, m_n_varyings * m_n_tf_components * sizeof(glw::GLint));

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error ummapping transform feedback buffer.");

	glw::GLboolean test_passed = true;

	/* Elements under index 0-2 contain result of textureSize called for samplerCubeArray sampler */
	if (read_size[0] != width || read_size[1] != height || read_size[2] != (depth / m_n_layers_per_cube))
	{
		getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Storage Type: " << ((storType == ST_MUTABLE) ? mutableStorage : imMutableStorage) << "\n"
			<< "textureSize() for samplerCubeArray returned wrong values. [width][height][layers]. They are equal "
			<< "[" << read_size[0] << "][" << read_size[1] << "][" << read_size[2] << "] but should be "
			<< "[" << width << "][" << height << "][" << depth / m_n_layers_per_cube << "]."
			<< tcu::TestLog::EndMessage;
		test_passed = false;
	}

	/* Elements under index 3-5 contain result of textureSize called for samplerCubeArrayShadow sampler */
	if (read_size[3] != width || read_size[4] != height || read_size[5] != (depth / m_n_layers_per_cube))
	{
		getTestContext().getLog() << tcu::TestLog::Message
								  << "Storage Type: " << ((storType == ST_MUTABLE) ? mutableStorage : imMutableStorage)
								  << "\n"
								  << "textureSize() for samplerCubeArrayShadow returned wrong values. "
									 "[width][height][layers]. They are equal "
								  << "[" << read_size[3] << "][" << read_size[4] << "][" << read_size[5]
								  << "] but should be "
								  << "[" << width << "][" << height << "][" << depth / m_n_layers_per_cube << "]."
								  << tcu::TestLog::EndMessage;
		test_passed = false;
	}

	return test_passed;
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeTFVertexShader::TextureCubeMapArrayTextureSizeTFVertexShader(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureCubeMapArrayTextureSizeTFBase(context, extParams, name, description), m_vs_id(0), m_fs_id(0)
{
	/* Nothing to be done here */
}

/* Configure program object */
void TextureCubeMapArrayTextureSizeTFVertexShader::configureProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set transform feedback varyings */
	const char* varyings[] = { "texture_std_size", "texture_shw_size" };
	gl.transformFeedbackVaryings(m_po_id, m_n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting transform feedback varyings.");

	const char* vsCode = getVertexShaderCode();
	const char* fsCode = getFragmentShaderCode();

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_vs_id, 1 /* part */, &vsCode))
	{
		TCU_FAIL("Could not compile/link program object from valid shader code.");
	}
}

/** Delete program object */
void TextureCubeMapArrayTextureSizeTFVertexShader::deleteProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete shader objects */
	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}
}

/** Render or dispatch compute */
void TextureCubeMapArrayTextureSizeTFVertexShader::runShaders(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error beginning transform feedback.");

	gl.drawArrays(GL_POINTS, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error ending transform feedback.");
}

/** Returns code for Vertex Shader
 *  @return pointer to literal with Vertex Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFVertexShader::getVertexShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"uniform highp samplerCubeArray       texture_std_sampler;\n"
								"uniform highp samplerCubeArrayShadow texture_shw_sampler;\n"
								"\n"
								"layout (location = 0) out flat uvec3 texture_std_size;\n"
								"layout (location = 1) out flat uvec3 texture_shw_size;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_PointSize = 1.0f;\n"
								"\n"
								"    texture_std_size = uvec3( textureSize(texture_std_sampler, 0) );\n"
								"    texture_shw_size = uvec3( textureSize(texture_shw_sampler, 0) );\n"
								"    gl_Position      = vec4(0.0f,0.0f,0.0f,0.0f);\n"
								"}\n";
	return result;
}

/** Return code for Fragment Shader
 *  @return pointer to literal with Fragment Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFVertexShader::getFragmentShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"void main()\n"
								"{\n"
								"}\n";
	return result;
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeTFGeometryShader::TextureCubeMapArrayTextureSizeTFGeometryShader(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureCubeMapArrayTextureSizeTFBase(context, extParams, name, description), m_vs_id(0), m_gs_id(0), m_fs_id(0)
{
	/* Nothing to be done here */
}

/* Configure program object */
void TextureCubeMapArrayTextureSizeTFGeometryShader::configureProgram(void)
{
	/* Check if geometry_shader extension is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set transform feedback varyings */
	const char* varyings[] = { "texture_std_size", "texture_shw_size" };
	gl.transformFeedbackVaryings(m_po_id, m_n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting transform feedback varyings.");

	const char* vsCode = getVertexShaderCode();
	const char* gsCode = getGeometryShaderCode();
	const char* fsCode = getFragmentShaderCode();

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_gs_id, 1 /* part */, &gsCode, m_vs_id, 1 /* part */,
					  &vsCode))
	{
		TCU_FAIL("Could not compile/link program object from valid shader code.");
	}
}

/** Delete program object */
void TextureCubeMapArrayTextureSizeTFGeometryShader::deleteProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete shader objects */
	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
		m_gs_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}
}

void TextureCubeMapArrayTextureSizeTFGeometryShader::runShaders(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error beginning transform feedback.");

	gl.drawArrays(GL_POINTS, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error ending transform feedback.");
}

/** Returns code for Vertex Shader
 *  @return pointer to literal with Vertex Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFGeometryShader::getVertexShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_Position      = vec4(0.0f,0.0f,0.0f,0.0f);\n"
								"}\n";
	return result;
}

/** Return code for Geometry Shader
 *  @return pointer to literal with Geometry Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFGeometryShader::getGeometryShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${GEOMETRY_SHADER_ENABLE}\n"
								"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"uniform highp samplerCubeArray       texture_std_sampler;\n"
								"uniform highp samplerCubeArrayShadow texture_shw_sampler;\n"
								"\n"
								"layout (location = 0) out flat uvec3 texture_std_size;\n"
								"layout (location = 1) out flat uvec3 texture_shw_size;\n"
								"\n"
								"layout(points)                 in;\n"
								"layout(points, max_vertices=1) out;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    texture_std_size = uvec3( textureSize(texture_std_sampler, 0) );\n"
								"    texture_shw_size = uvec3( textureSize(texture_shw_sampler, 0) );\n"
								"    gl_Position      = vec4(0.0f,0.0f,0.0f,0.0f);\n"
								"    EmitVertex();\n"
								"    EndPrimitive();\n"
								"}\n";
	return result;
}

/** Return code for Fragment Shader
 *  @return pointer to literal with Fragment Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFGeometryShader::getFragmentShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"void main()\n"
								"{\n"
								"}\n";
	return result;
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeTFTessControlShader::TextureCubeMapArrayTextureSizeTFTessControlShader(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureCubeMapArrayTextureSizeTFBase(context, extParams, name, description)
	, m_vs_id(0)
	, m_tcs_id(0)
	, m_tes_id(0)
	, m_fs_id(0)
{
	/* Nothing to be done here */
}

/* Configure program object */
void TextureCubeMapArrayTextureSizeTFTessControlShader::configureProgram(void)
{
	/* Check if tessellation_shader extension is supported */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set transform feedback varyings */
	const char* varyings[] = { "texture_std_size", "texture_shw_size" };
	gl.transformFeedbackVaryings(m_po_id, m_n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting transform feedback varyings.");

	const char* vsCode  = getVertexShaderCode();
	const char* tcsCode = getTessellationControlShaderCode();
	const char* tesCode = getTessellationEvaluationShaderCode();
	const char* fsCode  = getFragmentShaderCode();

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");
	m_tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");
	m_tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_tcs_id, 1 /* part */, &tcsCode, m_tes_id, 1 /* part */,
					  &tesCode, m_vs_id, 1 /* part */, &vsCode))
	{
		TCU_FAIL("Could not compile/link program object from valid shader code.");
	}
}

/** Delete program object */
void TextureCubeMapArrayTextureSizeTFTessControlShader::deleteProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete shader objects */
	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (m_tcs_id != 0)
	{
		gl.deleteShader(m_tcs_id);
		m_tcs_id = 0;
	}

	if (m_tes_id != 0)
	{
		gl.deleteShader(m_tes_id);
		m_tes_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}
}

/** Render or dispatch compute */
void TextureCubeMapArrayTextureSizeTFTessControlShader::runShaders(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error beginning transform feedback.");

	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Setting number of vertices per patch failed");

	gl.drawArrays(m_glExtTokens.PATCHES, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Setting number of vertices per patch failed");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error ending transform feedback.");
}

/** Returns code for Vertex Shader
 *  @return pointer to literal with Vertex Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFTessControlShader::getVertexShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_Position      = vec4(0.0f,0.0f,0.0f,0.0f);\n"
								"}\n";
	return result;
}

/** Return code for Tessellation Control Shader
 *  @return pointer to literal with Tessellation Control Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFTessControlShader::getTessellationControlShaderCode(void)
{
	static const char* result =
		"${VERSION}\n"
		"\n"
		"${TESSELLATION_SHADER_ENABLE}\n"
		"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"uniform highp samplerCubeArray       texture_std_sampler;\n"
		"uniform highp samplerCubeArrayShadow texture_shw_sampler;\n"
		"\n"
		"layout (location = 0) out flat uvec3 texture_std_size_array[];\n"
		"layout (location = 1) out flat uvec3 texture_shw_size_array[];\n"
		"\n"
		"layout (vertices = 1) out;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    texture_std_size_array[gl_InvocationID]    = uvec3( textureSize(texture_std_sampler, 0) );\n"
		"    texture_shw_size_array[gl_InvocationID]    = uvec3( textureSize(texture_shw_sampler, 0) );\n"
		"    gl_out[gl_InvocationID].gl_Position        = vec4(0.0f,0.0f,0.0f,0.0f);\n"
		"}\n";
	return result;
}

/** Returns code for Tessellation Evaluation Shader
 * @return pointer to literal with Tessellation Evaluation code
 **/
const char* TextureCubeMapArrayTextureSizeTFTessControlShader::getTessellationEvaluationShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${TESSELLATION_SHADER_REQUIRE}\n"
								"\n"
								"layout(isolines, point_mode) in;\n"
								"\n"
								"in layout(location = 0) flat uvec3 texture_std_size_array[];\n"
								"in layout(location = 1) flat uvec3 texture_shw_size_array[];\n"
								"\n"
								"out flat uvec3 texture_std_size;\n"
								"out flat uvec3 texture_shw_size;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    texture_std_size = texture_std_size_array[0];\n"
								"    texture_shw_size = texture_shw_size_array[0];\n"
								"    gl_Position      = vec4(0.0f,0.0f,0.0f,0.0f);\n"
								"}\n";
	return result;
}

/** Return code for Fragment Shader
 *  @return pointer to literal with Fragment Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFTessControlShader::getFragmentShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"void main()\n"
								"{\n"
								"}\n";
	return result;
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeTFTessEvaluationShader::TextureCubeMapArrayTextureSizeTFTessEvaluationShader(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureCubeMapArrayTextureSizeTFTessControlShader(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Return code for Tessellation Control Shader
 *  @return pointer to literal with Tessellation Control Shader code
 **/
const char* TextureCubeMapArrayTextureSizeTFTessEvaluationShader::getTessellationControlShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${TESSELLATION_SHADER_ENABLE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"layout (vertices = 1) out;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_TessLevelInner[0] = 1.0;\n"
								"    gl_TessLevelInner[1] = 1.0;\n"
								"    gl_TessLevelOuter[0] = 1.0;\n"
								"    gl_TessLevelOuter[1] = 1.0;\n"
								"    gl_TessLevelOuter[2] = 1.0;\n"
								"    gl_TessLevelOuter[3] = 1.0;\n"
								"    gl_out[gl_InvocationID].gl_Position = vec4(0.0f,0.0f,0.0f,0.0f);\n"
								"}\n";
	return result;
}

/** Returns code for Tessellation Evaluation Shader
 * @return pointer to literal with Tessellation Evaluation code
 **/
const char* TextureCubeMapArrayTextureSizeTFTessEvaluationShader::getTessellationEvaluationShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${TESSELLATION_SHADER_REQUIRE}\n"
								"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
								"\n"
								"layout(isolines, point_mode) in;\n"
								"\n"
								"uniform highp samplerCubeArray       texture_std_sampler;\n"
								"uniform highp samplerCubeArrayShadow texture_shw_sampler;\n"
								"\n"
								"layout (location = 0) out flat uvec3 texture_std_size;\n"
								"layout (location = 1) out flat uvec3 texture_shw_size;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    texture_std_size   = uvec3( textureSize(texture_std_sampler, 0) );\n"
								"    texture_shw_size   = uvec3( textureSize(texture_shw_sampler, 0) );\n"
								"    gl_Position        = vec4(0.0f,0.0f,0.0f,0.0f);\n"
								"}\n";
	return result;
}

const glw::GLuint TextureCubeMapArrayTextureSizeRTBase::m_n_rt_components = 4;

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeRTBase::TextureCubeMapArrayTextureSizeRTBase(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TextureCubeMapArrayTextureSizeBase(context, extParams, name, description)
	, m_read_fbo_id(0)
	, m_rt_std_id(0)
	, m_rt_shw_id(0)
{
	/* Nothing to be done here */
}

/** Configure GLES objects specific for the test configuration */
void TextureCubeMapArrayTextureSizeRTBase::configureTestSpecificObjects(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint rt_data[m_n_rt_components];
	memset(rt_data, 0, m_n_rt_components * sizeof(glw::GLuint));

	/* Create texture which will store result of textureSize() for samplerCubeArray sampler */
	gl.genTextures(1, &m_rt_std_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, m_rt_std_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating storage for texture object!");

	gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, rt_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture object's data store with data!");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");

	/* Create texture which will store result of textureSize() for samplerCubeArrayShadow sampler */
	gl.genTextures(1, &m_rt_shw_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	gl.activeTexture(GL_TEXTURE1);
	gl.bindTexture(GL_TEXTURE_2D, m_rt_shw_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating storage for texture object!");

	gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, rt_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture object's data store with data!");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");

	/* Generate frame buffer object */
	gl.genFramebuffers(1, &m_read_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating frame buffer object!");
}

/** Delete GLES objects specific for the test configuration */
void TextureCubeMapArrayTextureSizeRTBase::deleteTestSpecificObjects(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GL state */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.activeTexture(GL_TEXTURE1);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.activeTexture(GL_TEXTURE0);

	/* Delete GL objects */
	if (m_read_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_read_fbo_id);
		m_read_fbo_id = 0;
	}

	if (m_rt_std_id != 0)
	{
		gl.deleteTextures(1, &m_rt_std_id);
		m_rt_std_id = 0;
	}

	if (m_rt_shw_id != 0)
	{
		gl.deleteTextures(1, &m_rt_shw_id);
		m_rt_shw_id = 0;
	}
}

/** Check textureSize() and imageSize() methods returned proper values
 * @param  width    texture width
 * @param  height   texture height
 * @param  depth    texture depth
 * @param  storType inform if texture is mutable or immutable
 * @return          return true if result data is as expected
 */
glw::GLboolean TextureCubeMapArrayTextureSizeRTBase::checkResults(glw::GLuint width, glw::GLuint height,
																  glw::GLuint depth, STORAGE_TYPE storType)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLboolean test_passed = true;

	glw::GLuint read_size[m_n_rt_components];
	memset(read_size, 0, m_n_rt_components * sizeof(glw::GLuint));

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_read_fbo_id);

	/* Compare returned results of textureSize() called for samplerCubeArray sampler*/
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_rt_std_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to frame buffer");

	/* Check framebuffer status */
	checkFramebufferStatus(GL_READ_FRAMEBUFFER);

	gl.readPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, read_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixles from frame buffer!");

	if (read_size[0] != width || read_size[1] != height || read_size[2] != (depth / m_n_layers_per_cube))
	{
		getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Storage Type: " << ((storType == ST_MUTABLE) ? mutableStorage : imMutableStorage) << "\n"
			<< "textureSize() for samplerCubeArray returned wrong values of [width][height][layers]. They are equal to"
			<< "[" << read_size[0] << "][" << read_size[1] << "][" << read_size[2] << "] but should be "
			<< "[" << width << "][" << height << "][" << depth / m_n_layers_per_cube << "]."
			<< tcu::TestLog::EndMessage;
		test_passed = false;
	}

	/* Compare returned results of textureSize() for samplerCubeArrayShadow sampler*/
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_rt_shw_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to frame buffer");

	/* Check framebuffer status */
	checkFramebufferStatus(GL_READ_FRAMEBUFFER);

	gl.readPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, read_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixles from frame buffer!");

	if (read_size[0] != width || read_size[1] != height || read_size[2] != (depth / m_n_layers_per_cube))
	{
		getTestContext().getLog() << tcu::TestLog::Message
								  << "Storage Type: " << ((storType == ST_MUTABLE) ? mutableStorage : imMutableStorage)
								  << "\n"
								  << "textureSize() for samplerCubeArrayShadow returned wrong values of "
									 "[width][height][layers]. They are equal to"
								  << "[" << read_size[0] << "][" << read_size[1] << "][" << read_size[2]
								  << "] but should be "
								  << "[" << width << "][" << height << "][" << depth / m_n_layers_per_cube << "]."
								  << tcu::TestLog::EndMessage;
		test_passed = false;
	}

	return test_passed;
}

/** Check Framebuffer Status. Throws a TestError exception, should the framebuffer status
 *  be found incomplete.
 *
 *  @param framebuffer - GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER or GL_FRAMEBUFFER
 *
 */
void TextureCubeMapArrayTextureSizeRTBase::checkFramebufferStatus(glw::GLenum framebuffer)
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
		}; /* switch (framebuffer_status) */
	}	  /* if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status) */
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeRTFragmentShader::TextureCubeMapArrayTextureSizeRTFragmentShader(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureCubeMapArrayTextureSizeRTBase(context, extParams, name, description)
	, m_draw_fbo_id(0)
	, m_vs_id(0)
	, m_fs_id(0)
{
	/* Nothing to be done here */
}

/** Configure GLES objects specific for the test configuration */
void TextureCubeMapArrayTextureSizeRTFragmentShader::configureTestSpecificObjects(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	TextureCubeMapArrayTextureSizeRTBase::configureTestSpecificObjects();

	/* Generate frame buffer object */
	gl.genFramebuffers(1, &m_draw_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating frame buffer object!");
}

/** Delete GLES objects specific for the test configuration */
void TextureCubeMapArrayTextureSizeRTFragmentShader::deleteTestSpecificObjects(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	/* Delete GLEs objects */
	if (m_draw_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_draw_fbo_id);
		m_draw_fbo_id = 0;
	}

	TextureCubeMapArrayTextureSizeRTBase::deleteTestSpecificObjects();
}

/* Configure program object */
void TextureCubeMapArrayTextureSizeRTFragmentShader::configureProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const char* vsCode = getVertexShaderCode();
	const char* fsCode = getFragmentShaderCode();

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_vs_id, 1 /* part */, &vsCode))
	{
		TCU_FAIL("Could not compile/link program object from valid shader code.");
	}
}

/** Delete program object */
void TextureCubeMapArrayTextureSizeRTFragmentShader::deleteProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete shader objects */
	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}
}

/** Return code for bolierPlate Vertex Shader
 * @return pointer to literal with Vertex Shader code
 **/
const char* TextureCubeMapArrayTextureSizeRTFragmentShader::getVertexShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_PointSize = 1.0f;\n"
								"    gl_Position = vec4(0, 0, 0, 1.0f);\n"
								"}\n";

	return result;
}

/** Return code for Fragment Shader
 * @return pointer to literal with Fragment Shader code
 **/
const char* TextureCubeMapArrayTextureSizeRTFragmentShader::getFragmentShaderCode(void)
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"uniform highp samplerCubeArray       texture_std_sampler;\n"
								"uniform highp samplerCubeArrayShadow texture_shw_sampler;\n"
								"\n"
								"layout (location = 0) out uvec4 texture_std_size;\n"
								"layout (location = 1) out uvec4 texture_shw_size;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    texture_std_size = uvec4( textureSize(texture_std_sampler, 0), 0 );\n"
								"    texture_shw_size = uvec4( textureSize(texture_shw_sampler, 0), 0 );\n"
								"}\n";

	return result;
}

void TextureCubeMapArrayTextureSizeRTFragmentShader::runShaders(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Configure draw framebuffer */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_draw_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding framebuffer object");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_rt_std_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to GL_COLOR_ATTACHMENT0");
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_rt_shw_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to GL_COLOR_ATTACHMENT0");

	/* Check framebuffer status */
	checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	/* Configure draw buffers for fragment shader */
	const glw::GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	gl.drawBuffers(2, drawBuffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting draw buffers");

	glw::GLint viewport_size[4];
	gl.getIntegerv(GL_VIEWPORT, viewport_size);

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Setting viewport");

	gl.drawArrays(GL_POINTS, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	gl.viewport(viewport_size[0], viewport_size[1], viewport_size[2], viewport_size[3]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Setting viewport");
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayTextureSizeRTComputeShader::TextureCubeMapArrayTextureSizeRTComputeShader(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureCubeMapArrayTextureSizeRTBase(context, extParams, name, description)
	, m_cs_id(0)
	, m_to_img_id(0)
	, m_rt_img_id(0)
{
	/* Nothing to be done here */
}

/** Configure program object */
void TextureCubeMapArrayTextureSizeRTComputeShader::configureProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const char* csCode = getComputeShaderCode();

	m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object.");

	/* Build program */
	if (!buildProgram(m_po_id, m_cs_id, 1 /* part */, &csCode))
	{
		TCU_FAIL("Could not compile/link program object from valid shader code.");
	}
}

/** Delete program object */
void TextureCubeMapArrayTextureSizeRTComputeShader::deleteProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete shader objects */
	if (m_cs_id != 0)
	{
		gl.deleteShader(m_cs_id);
		m_cs_id = 0;
	}
}

/** Returns code for Compute Shader
 * @return pointer to literal with Compute Shader code
 **/
const char* TextureCubeMapArrayTextureSizeRTComputeShader::getComputeShaderCode(void)
{
	static const char* result =
		"${VERSION}\n"
		"\n"
		"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"                               uniform highp samplerCubeArray       texture_std_sampler;\n"
		"                               uniform highp samplerCubeArrayShadow texture_shw_sampler;\n"
		"layout(rgba32f,  binding = 0)  writeonly uniform highp imageCubeArray         texture_img_sampler;\n"
		"\n"
		"layout (rgba32ui, binding = 1) uniform highp writeonly uimage2D image_std_size;\n"
		"layout (rgba32ui, binding = 2) uniform highp writeonly uimage2D image_shw_size;\n"
		"layout (rgba32ui, binding = 3) uniform highp writeonly uimage2D image_img_size;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"    imageStore(image_std_size, ivec2(0,0),  uvec4(uvec3( textureSize(texture_std_sampler, 0)), 0) );\n"
		"    imageStore(image_shw_size, ivec2(0,0),  uvec4(uvec3( textureSize(texture_shw_sampler, 0)), 0) );\n"
		"    imageStore(image_img_size, ivec2(0,0),  uvec4(uvec3( imageSize  (texture_img_sampler)), 0) );\n"
		"}\n";

	return result;
}

/** Configure GLES objects specific for the test configuration */
void TextureCubeMapArrayTextureSizeRTComputeShader::configureTestSpecificObjects(void)
{
	TextureCubeMapArrayTextureSizeRTBase::configureTestSpecificObjects();

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint rt_data[m_n_rt_components];
	memset(rt_data, 0, m_n_rt_components * sizeof(glw::GLuint));

	/* Create texture which will store result of imageSize() for imageCubeArray image */
	gl.genTextures(1, &m_rt_img_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	gl.activeTexture(GL_TEXTURE2);
	gl.bindTexture(GL_TEXTURE_2D, m_rt_img_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating storage for texture object!");

	gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, rt_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture object's data store with data!");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");

	/* Image unit binding start from index 1 for compute shader results */
	gl.bindImageTexture(1, m_rt_std_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit");
	gl.bindImageTexture(2, m_rt_shw_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit");
	gl.bindImageTexture(3, m_rt_img_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit");
}

/** Delete GLES objects specific for the test configuration */
void TextureCubeMapArrayTextureSizeRTComputeShader::deleteTestSpecificObjects(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GL state */
	gl.activeTexture(GL_TEXTURE2);
	gl.bindTexture(GL_TEXTURE_2D, 0);

	/* Delete GL objects */
	if (m_rt_img_id != 0)
	{
		gl.deleteTextures(1, &m_rt_img_id);
		m_rt_img_id = 0;
	}

	TextureCubeMapArrayTextureSizeRTBase::deleteTestSpecificObjects();
}

/** Configure textures used in the test for textureSize() and imageSize() calls
 @param width    texture width
 @param height   texture height
 @param depth    texture depth
 @param storType inform if texture should be mutable or immutable
 */
void TextureCubeMapArrayTextureSizeRTComputeShader::configureTextures(glw::GLuint width, glw::GLuint height,
																	  glw::GLuint depth, STORAGE_TYPE storType)
{
	TextureCubeMapArrayTextureSizeRTBase::configureTextures(width, height, depth, storType);

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	createCubeMapArrayTexture(m_to_img_id, width, height, depth, storType, false);

	/* Binding texture object to texture unit */
	gl.activeTexture(GL_TEXTURE2);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_to_img_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.bindImageTexture(0, m_to_img_id, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit");
}

/** Delete textures used in the test for textureSize() and imageSize() calls */
void TextureCubeMapArrayTextureSizeRTComputeShader::deleteTextures(void)
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.activeTexture(GL_TEXTURE2);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

	/* Delete GLES objects */
	if (m_to_img_id != 0)
	{
		gl.deleteTextures(1, &m_to_img_id);
		m_to_img_id = 0;
	}

	TextureCubeMapArrayTextureSizeRTBase::deleteTextures();
}

/** Render or dispatch compute */
void TextureCubeMapArrayTextureSizeRTComputeShader::runShaders(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error executing compute shader");
	gl.memoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");
}

/** Check textureSize() and imageSize() methods returned proper values
 * @param  width    texture width
 * @param  height   texture height
 * @param  depth    texture depth
 * @param  storType inform if texture is mutable or immutable
 * @return          return true if result data is as expected
 */
glw::GLboolean TextureCubeMapArrayTextureSizeRTComputeShader::checkResults(glw::GLuint width, glw::GLuint height,
																		   glw::GLuint depth, STORAGE_TYPE storType)
{
	glw::GLboolean test_passed = TextureCubeMapArrayTextureSizeRTBase::checkResults(width, height, depth, storType);

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint read_size[m_n_rt_components];
	memset(read_size, 0, m_n_rt_components * sizeof(glw::GLuint));

	/* Compare returned results of imageSize() for imageCubeArray image */
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_rt_img_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to frame buffer");

	/* Check framebuffer status */
	checkFramebufferStatus(GL_READ_FRAMEBUFFER);

	gl.readPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, read_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixles from frame buffer!");

	if (read_size[0] != width || read_size[1] != height || read_size[2] != (depth / m_n_layers_per_cube))
	{
		getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Storage Type: " << ((storType == ST_MUTABLE) ? mutableStorage : imMutableStorage) << "\n"
			<< "imageSize() for imageCubeArray returned wrong values of [width][height][layers]. They are equal to"
			<< "[" << read_size[0] << "][" << read_size[1] << "][" << read_size[2] << "] but should be "
			<< "[" << width << "][" << height << "][" << depth / m_n_layers_per_cube << "]."
			<< tcu::TestLog::EndMessage;
		test_passed = false;
	}

	return test_passed;
}

/** Method to check if the test supports mutable textures.
 *
 *  @return return true if mutable textures work with the test
 */
glw::GLboolean TextureCubeMapArrayTextureSizeRTComputeShader::isMutableTextureTestable(void)
{
	/**
	 * Mutable textures cannot be bound as image textures on ES, but can be on
	 * desktop GL. This check enables/disables testing of mutable image textures.
	 */
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		return true;
	}
	else
	{
		return false;
	}
}

} /* glcts */
