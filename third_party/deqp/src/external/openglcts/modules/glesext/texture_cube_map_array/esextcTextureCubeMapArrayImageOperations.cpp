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
 * \file  esextcTextureCubeMapArrayImageOperations.cpp
 * \brief texture_cube_map_array extension - Image Operations (Test 8)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayImageOperations.hpp"
#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cmath>
#include <cstring>
#include <vector>

namespace glcts
{

/* Set constant values for tests */
const glw::GLfloat TextureCubeMapArrayImageOpCompute::m_f_base  = 1.5f;
const glw::GLint   TextureCubeMapArrayImageOpCompute::m_i_base  = -1;
const glw::GLuint  TextureCubeMapArrayImageOpCompute::m_ui_base = 1;

const glw::GLuint TextureCubeMapArrayImageOpCompute::m_n_components	= 4;
const glw::GLuint TextureCubeMapArrayImageOpCompute::m_n_dimensions	= 3;
const glw::GLuint TextureCubeMapArrayImageOpCompute::m_n_image_formats = 3;
const glw::GLuint TextureCubeMapArrayImageOpCompute::m_n_resolutions   = 4;
const glw::GLuint TextureCubeMapArrayImageOpCompute::m_n_storage_type  = 2;

const char* TextureCubeMapArrayImageOpCompute::m_mutable_storage   = "MUTABLE";
const char* TextureCubeMapArrayImageOpCompute::m_immutable_storage = "IMMUTABLE";

/* Helper arrays for tests configuration */

/* Different texture resolutions */
const int m_resolutions[TextureCubeMapArrayImageOpCompute::m_n_resolutions]
					   [TextureCubeMapArrayImageOpCompute::m_n_dimensions] = {
						   /* Width , Height, Depth */
						   { 16, 16, 12 },
						   { 32, 32, 6 },
						   { 4, 4, 18 },
						   { 8, 8, 6 }
					   };

/** Check if buffers contains the same values
 * @param a      buffer with data to compare
 * @param b      buffer with data to compare
 * @param length buffers length
 * @return       true if both buffers are equal, otherwise false
 */
template <typename T>
glw::GLboolean areBuffersEqual(const T* a, const T* b, glw::GLuint length)
{
	return (memcmp(a, b, length * sizeof(T))) ? false : true;
}

/** Check if buffers contains the same values (float type)
 * @param a      buffer with data to compare
 * @param b      buffer with data to compare
 * @param length buffers length
 * @return       true if both buffers are equal, otherwise false
 */
template <>
glw::GLboolean areBuffersEqual(const glw::GLfloat* a, const glw::GLfloat* b, glw::GLuint length)
{
	for (glw::GLuint i = 0; i < length; ++i)
	{
		if (de::abs(a[i] - b[i]) > TestCaseBase::m_epsilon_float)
		{
			return false;
		}
	}
	return true;
}

/** Fill buffer with test data
 * @param data       buffer where values will be stored
 * @param width      buffer/texture width
 * @param height     buffer/texture height
 * @param depth      buffer/texture depth
 * @param components buffer/texture components number
 * @param base       base value used to fill array
 **/
template <typename T>
void fillData(T* data, glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLuint components, T base)
{
	for (glw::GLuint i = 0; i < depth; ++i)
	{
		for (glw::GLuint j = 0; j < width; ++j)
		{
			for (glw::GLuint k = 0; k < height; ++k)
			{
				for (glw::GLuint l = 0; l < components; ++l)
				{
					data[i * width * height * components + j * height * components + k * components + l] = base + (T)i;
				}
			}
		}
	}
}

/** Check if results are es expected and log error if not
 * @param context      application context
 * @param id           id of texture
 * @param width        texture width
 * @param height       texture height
 * @param depth        texture depth
 * @param components   number of components per texel
 * @param format       texture data format
 * @param type         texture data type
 * @param storType     storageType
 * @param expectedData buffer with expected data
 * @return             return true if data read from the texture is the same as expected
 */
template <typename T>
bool checkResults(Context& context, glw::GLuint copy_po_id, glw::GLuint id, glw::GLuint width, glw::GLuint height,
				  glw::GLuint depth, glw::GLuint components, glw::GLenum format, glw::GLenum type,
				  STORAGE_TYPE storType, T* expectedData)
{
	/* Get GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* prepare buffers for result data */
	std::vector<T> resultData(width * height * components);

	glw::GLint  old_program = 0;
	glw::GLuint uint_tex_id = 0;

	/* Floating point textures are not renderable, so we will need to copy their bits to a temporary unsigned integer texture */
	if (type == GL_FLOAT)
	{
		/* Generate a new texture name */
		gl.genTextures(1, &uint_tex_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating temporary texture object!");

		/* Allocate unsigned integer storage */
		gl.bindTexture(GL_TEXTURE_2D, uint_tex_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding temporary texture object!");
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, width, height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating temporary texture object!");

		/* Set the filter mode */
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");

		/* Attach it to the framebuffer */
		gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, uint_tex_id, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to frame buffer");

		/* And bind it to an image unit for writing */
		gl.bindImageTexture(1, uint_tex_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding integer texture for copy destination");

		/* Finally, bind the copy compute shader */
		gl.getIntegerv(GL_CURRENT_PROGRAM, &old_program);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error querying old program!");
		gl.useProgram(copy_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");
	}

	bool result = true;

	for (glw::GLuint i = 0; i < depth; ++i)
	{
		/* Floating point textures are not renderable */
		if (type == GL_FLOAT)
		{
			/* Use a compute shader to store the float bits as unsigned integers */
			gl.bindImageTexture(0, id, 0, GL_FALSE, i, GL_READ_ONLY, GL_RGBA32F);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding floating point texture for copy source");
			gl.dispatchCompute(width, height, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error dispatching float-to-integer compute shader");

			/* Read data as unsigned ints */
			gl.readPixels(0, 0, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &resultData[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixels from frame buffer!");
		}
		else
		{
			/* Attach proper 2D texture to frame buffer and read pixels */
			gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, id, 0, i);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to frame buffer");

			/* Read data */
			gl.readPixels(0, 0, width, height, format, type, &resultData[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixels from frame buffer!");
		}

		/* Prepare correct pointer for expected data layer */
		T* pointer = &expectedData[0] + (i * width * height * components);

		/* If compared data are not equal log error and return false */
		if (!areBuffersEqual<T>(&resultData[0], pointer, width * height * components))
		{
			context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Wrong value in result texture for "
				<< ((type == GL_FLOAT) ? "imageCubeArray" : ((type == GL_INT) ? "iimageCubeArray" : "uimageCubeArray"))
				<< " for resolution[w,h,d] = [" << width << "," << height << "," << depth << "] for layer[" << i
				<< "] and " << ((storType == ST_MUTABLE) ? TextureCubeMapArrayImageOpCompute::m_mutable_storage :
														   TextureCubeMapArrayImageOpCompute::m_immutable_storage)
				<< " storage!" << tcu::TestLog::EndMessage;
			result = false;
			break;
		}
	}

	/* Clean up the floating point stuff */
	if (type == GL_FLOAT)
	{
		/* Restore the program */
		gl.useProgram(old_program);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

		/* Delete the temporary texture */
		gl.deleteTextures(1, &uint_tex_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error deleting temporary texture!");
	}

	return result;
}

/** Configure texture object
 * @param context        application context
 * @param id             pointer where texture id will be stored
 * @param width          texture width
 * @param height         texture height
 * @param depth          texture depth
 * @param storType       storageType
 * @param internalFormat texture internal format
 * @param format         texture data format
 * @param type           texture data type
 * @param data           initialization data for texture
 */
template <typename T>
void configureTexture(glcts::Context& context, glw::GLuint* id, glw::GLuint width, glw::GLuint height,
					  glw::GLuint depth, STORAGE_TYPE storType, glw::GLenum internalFormat, glw::GLenum format,
					  glw::GLenum type, const T* data)
{
	/* Get GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Generate texture object */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active texture unit!");

	gl.genTextures(1, id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, *id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture object's filter mode!");

	/* used glTexImage3D() method if texture should be MUTABLE */
	if (storType == ST_MUTABLE)
	{
		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture parameter.");
		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture parameter.");

		gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, internalFormat, width, height, depth, 0, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating texture object's data store");
	}
	/* used glTexStorage3D() method if texture should be IMMUTABLE */
	else
	{
		gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, internalFormat, width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating texture object's data store");

		gl.texSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0, width, height, depth, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture object's data store with data");
	}
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayImageOpCompute::TextureCubeMapArrayImageOpCompute(Context& context, const ExtParameters& extParams,
																	 const char* name, const char* description,
																	 SHADER_TO_CHECK shaderToCheck)
	: TestCaseBase(context, extParams, name, description)
	, m_shader_to_check(shaderToCheck)
	, m_cs_id(0)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_copy_po_id(0)
	, m_copy_cs_id(0)
	, m_iimage_read_to_id(0)
	, m_iimage_write_to_id(0)
	, m_image_read_to_id(0)
	, m_image_write_to_id(0)
	, m_uimage_read_to_id(0)
	, m_uimage_write_to_id(0)
{
	/* Nothing to be done here */
}

/** Initialize test case */
void TextureCubeMapArrayImageOpCompute::initTest(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check if texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}
	if (!m_is_geometry_shader_extension_supported && m_shader_to_check == STC_GEOMETRY_SHADER)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}
	if (!m_is_tessellation_shader_supported && (m_shader_to_check == STC_TESSELLATION_CONTROL_SHADER ||
												m_shader_to_check == STC_TESSELLATION_EVALUATION_SHADER))
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Generate and bind framebuffer */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating frame buffer object!");

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding frame buffer object!");

	/* Create the floating point copy program */
	m_copy_po_id			   = gl.createProgram();
	m_copy_cs_id			   = gl.createShader(GL_COMPUTE_SHADER);
	const char* copy_cs_source = getFloatingPointCopyShaderSource();
	buildProgram(m_copy_po_id, m_copy_cs_id, 1, &copy_cs_source);

	/* Create program */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	configureProgram();
}

/** Deinitialize test case */
void TextureCubeMapArrayImageOpCompute::deinit(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES configuration */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.useProgram(0);
	gl.bindVertexArray(0);

	/* Delete GLES objects */
	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}
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
	if (m_copy_cs_id != 0)
	{
		gl.deleteShader(m_copy_cs_id);
		m_copy_cs_id = 0;
	}
	if (m_copy_po_id != 0)
	{
		gl.deleteProgram(m_copy_po_id);
		m_copy_po_id = 0;
	}
	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}
	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
	}

	removeTextures();

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Delete texture objects */
void TextureCubeMapArrayImageOpCompute::removeTextures()
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

	/* Delete texture objects */
	if (m_iimage_read_to_id != 0)
	{
		gl.deleteTextures(1, &m_iimage_read_to_id);
		m_iimage_read_to_id = 0;
	}

	if (m_iimage_write_to_id != 0)
	{
		gl.deleteTextures(1, &m_iimage_write_to_id);
		m_iimage_write_to_id = 0;
	}

	if (m_image_read_to_id != 0)
	{
		gl.deleteTextures(1, &m_image_read_to_id);
		m_image_read_to_id = 0;
	}

	if (m_image_write_to_id != 0)
	{
		gl.deleteTextures(1, &m_image_write_to_id);
		m_image_write_to_id = 0;
	}

	if (m_uimage_read_to_id != 0)
	{
		gl.deleteTextures(1, &m_uimage_read_to_id);
		m_uimage_read_to_id = 0;
	}

	if (m_uimage_write_to_id != 0)
	{
		gl.deleteTextures(1, &m_uimage_write_to_id);
		m_uimage_write_to_id = 0;
	}
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestCase::IterateResult TextureCubeMapArrayImageOpCompute::iterate()
{
	initTest();

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

	bool test_passed = true;

	std::vector<glw::GLfloat> floatData;
	std::vector<glw::GLfloat> floatClean;
	std::vector<glw::GLint>   intData;
	std::vector<glw::GLint>   intClean;
	std::vector<glw::GLuint>  uIntData;
	std::vector<glw::GLuint>  uIntClean;

	/* Execute test throught all resolutions, storage types, and image types */
	for (glw::GLuint res_index = 0; res_index < m_n_resolutions; ++res_index)
	{
		glw::GLuint width  = m_resolutions[res_index][DL_WIDTH];
		glw::GLuint height = m_resolutions[res_index][DL_HEIGHT];
		glw::GLuint depth  = m_resolutions[res_index][DL_DEPTH];

		/* Allocate memory buffers for data */
		floatData.resize(width * height * depth * m_n_components);
		floatClean.resize(width * height * depth * m_n_components);
		intData.resize(width * height * depth * m_n_components);
		intClean.resize(width * height * depth * m_n_components);
		uIntData.resize(width * height * depth * m_n_components);
		uIntClean.resize(width * height * depth * m_n_components);

		memset(&floatClean[0], 0, width * height * depth * m_n_components * sizeof(glw::GLfloat));
		memset(&intClean[0], 0, width * height * depth * m_n_components * sizeof(glw::GLint));
		memset(&uIntClean[0], 0, width * height * depth * m_n_components * sizeof(glw::GLuint));

		/* Fill buffers with expected data*/
		fillData<glw::GLfloat>(&floatData[0], width, height, depth, m_n_components, m_f_base);
		fillData<glw::GLint>(&intData[0], width, height, depth, m_n_components, m_i_base);
		fillData<glw::GLuint>(&uIntData[0], width, height, depth, m_n_components, m_ui_base);

		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{

			/**
			 * Mutable textures cannot be bound as image textures on ES, but can be on
			 * desktop GL.
			 * */

			/* Work on mutable texture storage */

			/* Generate texture objects */
			configureTexture<glw::GLfloat>(m_context, &m_image_read_to_id, width, height, depth, ST_MUTABLE, GL_RGBA32F,
										   GL_RGBA, GL_FLOAT, &floatData[0]);
			configureTexture<glw::GLfloat>(m_context, &m_image_write_to_id, width, height, depth, ST_MUTABLE,
										   GL_RGBA32F, GL_RGBA, GL_FLOAT, &floatClean[0]);

			configureTexture<glw::GLint>(m_context, &m_iimage_read_to_id, width, height, depth, ST_MUTABLE, GL_RGBA32I,
										 GL_RGBA_INTEGER, GL_INT, &intData[0]);
			configureTexture<glw::GLint>(m_context, &m_iimage_write_to_id, width, height, depth, ST_MUTABLE, GL_RGBA32I,
										 GL_RGBA_INTEGER, GL_INT, &intClean[0]);

			configureTexture<glw::GLuint>(m_context, &m_uimage_read_to_id, width, height, depth, ST_MUTABLE,
										  GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &uIntData[0]);
			configureTexture<glw::GLuint>(m_context, &m_uimage_write_to_id, width, height, depth, ST_MUTABLE,
										  GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &uIntClean[0]);

			/* Bind texture objects to image units */
			gl.bindImageTexture(IF_IMAGE, m_image_read_to_id, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
			gl.bindImageTexture(IF_IIMAGE, m_iimage_read_to_id, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32I);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
			gl.bindImageTexture(IF_UIMAGE, m_uimage_read_to_id, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32UI);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
			gl.bindImageTexture(IF_IMAGE + m_n_image_formats, m_image_write_to_id, 0, GL_TRUE, 0, GL_WRITE_ONLY,
								GL_RGBA32F);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
			gl.bindImageTexture(IF_IIMAGE + m_n_image_formats, m_iimage_write_to_id, 0, GL_TRUE, 0, GL_WRITE_ONLY,
								GL_RGBA32I);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
			gl.bindImageTexture(IF_UIMAGE + m_n_image_formats, m_uimage_write_to_id, 0, GL_TRUE, 0, GL_WRITE_ONLY,
								GL_RGBA32UI);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");

			/* Call shaders */
			runShaders(width, height, depth);

			/* Check results */
			if (!checkResults<glw::GLfloat>(m_context, m_copy_po_id, m_image_write_to_id, width, height, depth,
											m_n_components, GL_RGBA, GL_FLOAT, ST_MUTABLE, &floatData[0]))
			{
				test_passed = false;
			}

			if (!checkResults<glw::GLint>(m_context, m_copy_po_id, m_iimage_write_to_id, width, height, depth,
										  m_n_components, GL_RGBA_INTEGER, GL_INT, ST_MUTABLE, &intData[0]))
			{
				test_passed = false;
			}

			if (!checkResults<glw::GLuint>(m_context, m_copy_po_id, m_uimage_write_to_id, width, height, depth,
										   m_n_components, GL_RGBA_INTEGER, GL_UNSIGNED_INT, ST_MUTABLE, &uIntData[0]))
			{
				test_passed = false;
			}

			/* Delete textures */
			removeTextures();
		}

		/* Work on immutable texture storage */

		/* Generate texture objects */
		configureTexture<glw::GLfloat>(m_context, &m_image_read_to_id, width, height, depth, ST_IMMUTABLE, GL_RGBA32F,
									   GL_RGBA, GL_FLOAT, &floatData[0]);
		configureTexture<glw::GLfloat>(m_context, &m_image_write_to_id, width, height, depth, ST_IMMUTABLE, GL_RGBA32F,
									   GL_RGBA, GL_FLOAT, &floatClean[0]);

		configureTexture<glw::GLint>(m_context, &m_iimage_read_to_id, width, height, depth, ST_IMMUTABLE, GL_RGBA32I,
									 GL_RGBA_INTEGER, GL_INT, &intData[0]);
		configureTexture<glw::GLint>(m_context, &m_iimage_write_to_id, width, height, depth, ST_IMMUTABLE, GL_RGBA32I,
									 GL_RGBA_INTEGER, GL_INT, &intClean[0]);

		configureTexture<glw::GLuint>(m_context, &m_uimage_read_to_id, width, height, depth, ST_IMMUTABLE, GL_RGBA32UI,
									  GL_RGBA_INTEGER, GL_UNSIGNED_INT, &uIntData[0]);
		configureTexture<glw::GLuint>(m_context, &m_uimage_write_to_id, width, height, depth, ST_IMMUTABLE, GL_RGBA32UI,
									  GL_RGBA_INTEGER, GL_UNSIGNED_INT, &uIntClean[0]);

		/* Bind texture objects to image units */
		gl.bindImageTexture(IF_IMAGE, m_image_read_to_id, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
		gl.bindImageTexture(IF_IIMAGE, m_iimage_read_to_id, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32I);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
		gl.bindImageTexture(IF_UIMAGE, m_uimage_read_to_id, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
		gl.bindImageTexture(IF_IMAGE + m_n_image_formats, m_image_write_to_id, 0, GL_TRUE, 0, GL_WRITE_ONLY,
							GL_RGBA32F);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
		gl.bindImageTexture(IF_IIMAGE + m_n_image_formats, m_iimage_write_to_id, 0, GL_TRUE, 0, GL_WRITE_ONLY,
							GL_RGBA32I);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");
		gl.bindImageTexture(IF_UIMAGE + m_n_image_formats, m_uimage_write_to_id, 0, GL_TRUE, 0, GL_WRITE_ONLY,
							GL_RGBA32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");

		/* Call shaders */
		runShaders(width, height, depth);

		/* Check results */
		if (!checkResults<glw::GLfloat>(m_context, m_copy_po_id, m_image_write_to_id, width, height, depth,
										m_n_components, GL_RGBA, GL_FLOAT, ST_IMMUTABLE, &floatData[0]))
		{
			test_passed = false;
		}

		if (!checkResults<glw::GLint>(m_context, m_copy_po_id, m_iimage_write_to_id, width, height, depth,
									  m_n_components, GL_RGBA_INTEGER, GL_INT, ST_IMMUTABLE, &intData[0]))
		{
			test_passed = false;
		}

		if (!checkResults<glw::GLuint>(m_context, m_copy_po_id, m_uimage_write_to_id, width, height, depth,
									   m_n_components, GL_RGBA_INTEGER, GL_UNSIGNED_INT, ST_IMMUTABLE, &uIntData[0]))
		{
			test_passed = false;
		}

		/* Delete textures */
		removeTextures();
	}

	if (test_passed)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

/** Run shaders - call glDispatchCompute for compuate shaders and glDrawArrays for other types of shaders */
void TextureCubeMapArrayImageOpCompute::runShaders(glw::GLuint width, glw::GLuint height, glw::GLuint depth)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	switch (m_shader_to_check)
	{
	/* Call compute shader */
	case STC_COMPUTE_SHADER:
	{
		gl.dispatchCompute(width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error running compute shader!");
		gl.memoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

		break;
	}
	/* Run programs for VERTEX/FRAGMENT/GEOMETRY shader */
	case STC_VERTEX_SHADER:
	case STC_FRAGMENT_SHADER:
	case STC_GEOMETRY_SHADER:
	{
		glw::GLint dimensions_location = gl.getUniformLocation(m_po_id, "dimensions");
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting uniform location!");

		if (dimensions_location == -1)
		{
			TCU_FAIL("Invalid location returned for active uniform!");
		}

		gl.uniform3i(dimensions_location, width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for uniform variable!");

		gl.drawArrays(GL_POINTS, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");
		gl.memoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

		break;
	}
	case STC_TESSELLATION_CONTROL_SHADER:
	case STC_TESSELLATION_EVALUATION_SHADER:
	{
		glw::GLint dimensions_location = gl.getUniformLocation(m_po_id, "dimensions");
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting uniform location!");

		if (dimensions_location == -1)
		{
			TCU_FAIL("Invalid location returned for active uniform!");
		}

		gl.uniform3i(dimensions_location, width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for uniform variable!");

		gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting patch parameter!");

		gl.drawArrays(m_glExtTokens.PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");
		gl.memoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

		gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting patch parameter!");

		break;
	}
	}
}

/** Configure program object with proper shaders depending on m_shader_to_check value */
void TextureCubeMapArrayImageOpCompute::configureProgram(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	switch (m_shader_to_check)
	{
	case STC_COMPUTE_SHADER:
	{
		m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");

		const char* csCode = getComputeShaderCode();

		/* Images are required for compute shader */
		if (!buildProgram(m_po_id, m_cs_id, 1 /* part */, &csCode))
		{
			TCU_FAIL("Could not create a program from valid compute shader code!");
		}
		break;
	}
	case STC_VERTEX_SHADER:
	{
		m_vs_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");

		const char* vsCode = getVertexShaderCode();
		const char* fsCode = getFragmentShaderCodeBoilerPlate();

		/* Execute test only if images are supported by vertex shader */
		if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_vs_id, 1 /* part */, &vsCode))
		{
			throw tcu::NotSupportedError(
				"imageCubeArray/iimageCubeArray/uimageCubeArray are not supported by Vertex Shader", "", __FILE__,
				__LINE__);
		}
		break;
	}
	case STC_FRAGMENT_SHADER:
	{
		m_vs_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");

		const char* vsCode = getVertexShaderCodeBoilerPlate();
		const char* fsCode = getFragmentShaderCode();

		/* Execute test only if images are supported by fragment shader */
		if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_vs_id, 1 /* part */, &vsCode))
		{
			throw tcu::NotSupportedError(
				"imageCubeArray/iimageCubeArray/uimageCubeArray are not supported by Fragment Shader", "", __FILE__,
				__LINE__);
		}
		break;
	}
	case STC_GEOMETRY_SHADER:
	{
		m_vs_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");

		const char* vsCode = getVertexShaderCodeBoilerPlate();
		const char* gsCode = getGeometryShaderCode();
		const char* fsCode = getFragmentShaderCodeBoilerPlate();

		/* Execute test only if images are supported by geometry shader */
		if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_gs_id, 1 /* part */, &gsCode, m_vs_id,
						  1 /* part */, &vsCode))
		{
			throw tcu::NotSupportedError(
				"imageCubeArray/iimageCubeArray/uimageCubeArray are not supported by Geometry Shader", "", __FILE__,
				__LINE__);
		}
		break;
	}
	case STC_TESSELLATION_CONTROL_SHADER:
	{
		m_vs_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");

		const char* vsCode  = getVertexShaderCodeBoilerPlate();
		const char* tcsCode = getTessControlShaderCode();
		const char* tesCode = getTessEvaluationShaderCodeBoilerPlate();
		const char* fsCode  = getFragmentShaderCodeBoilerPlate();

		/* Execute test only if images are supported by tessellation control shader */
		if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_tc_id, 1 /* part */, &tcsCode, m_te_id,
						  1 /* part */, &tesCode, m_vs_id, 1 /* part */, &vsCode))
		{
			throw tcu::NotSupportedError(
				"imageCubeArray/iimageCubeArray/uimageCubeArray are not supported by Tessellation Control Shader", "",
				__FILE__, __LINE__);
		}
		break;
	}
	case STC_TESSELLATION_EVALUATION_SHADER:
	{
		m_vs_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");
		m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");

		const char* vsCode  = getVertexShaderCodeBoilerPlate();
		const char* tcsCode = getTessControlShaderCodeBoilerPlate();
		const char* tesCode = getTessEvaluationShaderCode();
		const char* fsCode  = getFragmentShaderCodeBoilerPlate();

		/* Execute test only if images are supported by tessellation evaluation shader */
		if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_tc_id, 1 /* part */, &tcsCode, m_te_id,
						  1 /* part */, &tesCode, m_vs_id, 1 /* part */, &vsCode))
		{
			throw tcu::NotSupportedError(
				"imageCubeArray/iimageCubeArray/uimageCubeArray are not supported by Tessellation Evaluation Shader",
				"", __FILE__, __LINE__);
		}
		break;
	}
	default:
		break;
	}
}

/** Returns code for Compute Shader
 *  @return pointer to literal with Compute Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getComputeShaderCode()
{
	static const char* computeShaderCode =
		"${VERSION}\n"
		"\n"
		"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"layout (rgba32f,  binding = 0) highp uniform readonly imageCubeArray  imageRead;\n"
		"layout (rgba32i,  binding = 1) highp uniform readonly iimageCubeArray iimageRead;\n"
		"layout (rgba32ui, binding = 2) highp uniform readonly uimageCubeArray uimageRead;\n"
		"layout (rgba32f,  binding = 3) highp uniform writeonly imageCubeArray  imageWrite;\n"
		"layout (rgba32i,  binding = 4) highp uniform writeonly iimageCubeArray iimageWrite;\n"
		"layout (rgba32ui, binding = 5) highp uniform writeonly uimageCubeArray uimageWrite;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"    ivec3 position = ivec3(gl_GlobalInvocationID.xyz);\n"
		"    imageStore(imageWrite,  position, imageLoad(imageRead,  position));\n"
		"    imageStore(iimageWrite, position, imageLoad(iimageRead, position));\n"
		"    imageStore(uimageWrite, position, imageLoad(uimageRead, position));\n"
		"}\n";

	return computeShaderCode;
}

/** Returns code for Vertex Shader
 * @return pointer to literal with Vertex Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getVertexShaderCode(void)
{

	static const char* vertexShaderCode =
		"${VERSION}\n"
		"\n"
		"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout (rgba32f,  binding = 0) highp uniform readonly imageCubeArray  imageRead;\n"
		"layout (rgba32i,  binding = 1) highp uniform readonly iimageCubeArray iimageRead;\n"
		"layout (rgba32ui, binding = 2) highp uniform readonly uimageCubeArray uimageRead;\n"
		"layout (rgba32f,  binding = 3) highp uniform writeonly imageCubeArray  imageWrite;\n"
		"layout (rgba32i,  binding = 4) highp uniform writeonly iimageCubeArray iimageWrite;\n"
		"layout (rgba32ui, binding = 5) highp uniform writeonly uimageCubeArray uimageWrite;\n"
		"\n"
		"uniform ivec3 dimensions;\n"
		"\n"
		"void main()\n"
		"{\n"
		"\n"
		"    gl_PointSize = 1.0f;\n"
		"    for(int w = 0; w < dimensions[0]; ++w)\n" /* width */
		"    {\n"
		"        for(int h = 0; h < dimensions[1]; ++h)\n" /* height */
		"        {\n"
		"            for(int d = 0; d < dimensions[2]; ++d)\n" /* depth */
		"            {\n"
		"                ivec3  position  = ivec3(w,h,d);\n"
		"                imageStore(imageWrite,  position, imageLoad(imageRead,  position));\n"
		"                imageStore(iimageWrite, position, imageLoad(iimageRead, position));\n"
		"                imageStore(uimageWrite, position, imageLoad(uimageRead, position));\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"\n"
		"}\n";

	return vertexShaderCode;
}

/** Returns code for Boiler Plate Vertex Shader
 * @return pointer to literal with Boiler Plate Vertex Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getVertexShaderCodeBoilerPlate(void)
{
	static const char* vertexShaderBoilerPlateCode = "${VERSION}\n"
													 "\n"
													 "precision highp float;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    gl_Position = vec4(0, 0, 0, 1.0f);\n"
													 "    gl_PointSize = 1.0f;\n"
													 "}\n";

	return vertexShaderBoilerPlateCode;
}

/** Returns code for Fragment Shader
 *  @return pointer to literal with Fragment Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getFragmentShaderCode(void)
{
	static const char* fragmentShaderCode =
		"${VERSION}\n"
		"\n"
		"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout (rgba32f,  binding = 0) highp uniform readonly imageCubeArray  imageRead;\n"
		"layout (rgba32i,  binding = 1) highp uniform readonly iimageCubeArray iimageRead;\n"
		"layout (rgba32ui, binding = 2) highp uniform readonly uimageCubeArray uimageRead;\n"
		"layout (rgba32f,  binding = 3) highp uniform writeonly imageCubeArray  imageWrite;\n"
		"layout (rgba32i,  binding = 4) highp uniform writeonly iimageCubeArray iimageWrite;\n"
		"layout (rgba32ui, binding = 5) highp uniform writeonly uimageCubeArray uimageWrite;\n"
		"\n"
		"uniform ivec3 dimensions;\n"
		"\n"
		"void main()\n"
		"{\n"
		"\n"
		"    for(int w = 0; w < dimensions[0]; ++w)\n" /* width */
		"    {\n"
		"        for(int h = 0; h < dimensions[1]; ++h)\n" /* height */
		"        {\n"
		"            for(int d = 0; d < dimensions[2]; ++d)\n" /* depth */
		"            {\n"
		"                ivec3  position  = ivec3(w,h,d);\n"
		"                imageStore(imageWrite,  position, imageLoad(imageRead,  position));\n"
		"                imageStore(iimageWrite, position, imageLoad(iimageRead, position));\n"
		"                imageStore(uimageWrite, position, imageLoad(uimageRead, position));\n"
		"            }"
		"        }"
		"    }"
		"\n"
		"}\n";

	return fragmentShaderCode;
}

/** Returns code for Boiler Plate Fragment Shader
 *  @return pointer to literal with Boiler Plate Fragment Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getFragmentShaderCodeBoilerPlate(void)
{
	static const char* fragmentShaderBoilerPlateCode = "${VERSION}\n"
													   "\n"
													   "precision highp float;\n"
													   "\n"
													   "void main()\n"
													   "{\n"
													   "}\n";

	return fragmentShaderBoilerPlateCode;
}

/** Returns code for Geometry Shader
 * @return pointer to literal with Geometry Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getGeometryShaderCode(void)
{
	static const char* geometryShaderCode =
		"${VERSION}\n"
		"\n"
		"${GEOMETRY_SHADER_ENABLE}\n"
		"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout (rgba32f,  binding = 0) highp uniform readonly imageCubeArray  imageRead;\n"
		"layout (rgba32i,  binding = 1) highp uniform readonly iimageCubeArray iimageRead;\n"
		"layout (rgba32ui, binding = 2) highp uniform readonly uimageCubeArray uimageRead;\n"
		"layout (rgba32f,  binding = 3) highp uniform writeonly imageCubeArray  imageWrite;\n"
		"layout (rgba32i,  binding = 4) highp uniform writeonly iimageCubeArray iimageWrite;\n"
		"layout (rgba32ui, binding = 5) highp uniform writeonly uimageCubeArray uimageWrite;\n"
		"\n"
		"uniform ivec3 dimensions;\n"
		"\n"
		"layout(points) in;\n"
		"layout(points, max_vertices=1) out;\n"
		"\n"
		"void main()\n"
		"{\n"
		"\n"
		"    for(int w = 0; w < dimensions[0]; ++w)\n" /* width */
		"    {\n"
		"        for(int h = 0; h < dimensions[1]; ++h)\n" /* height */
		"        {\n"
		"            for(int d = 0; d < dimensions[2]; ++d)\n" /* depth */
		"            {\n"
		"                ivec3  position  = ivec3(w,h,d);\n"
		"                imageStore(imageWrite,  position, imageLoad(imageRead,  position));\n"
		"                imageStore(iimageWrite, position, imageLoad(iimageRead, position));\n"
		"                imageStore(uimageWrite, position, imageLoad(uimageRead, position));\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"\n"
		"}\n";

	return geometryShaderCode;
}

/** Returns code for Tessellation Control Shader
 *  @return pointer to literal with Tessellation Control Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getTessControlShaderCode(void)
{
	static const char* tessellationControlShaderCode =
		"${VERSION}\n"
		"\n"
		"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
		"${TESSELLATION_SHADER_ENABLE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout (rgba32f,  binding = 0) highp uniform readonly imageCubeArray  imageRead;\n"
		"layout (rgba32i,  binding = 1) highp uniform readonly iimageCubeArray iimageRead;\n"
		"layout (rgba32ui, binding = 2) highp uniform readonly uimageCubeArray uimageRead;\n"
		"layout (rgba32f,  binding = 3) highp uniform writeonly imageCubeArray  imageWrite;\n"
		"layout (rgba32i,  binding = 4) highp uniform writeonly iimageCubeArray iimageWrite;\n"
		"layout (rgba32ui, binding = 5) highp uniform writeonly uimageCubeArray uimageWrite;\n"
		"\n"
		"uniform ivec3 dimensions;\n"
		"\n"
		"layout (vertices = 1) out;\n"
		"\n"
		"void main()\n"
		"{\n"
		"\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"\n"
		"    for(int w = 0; w < dimensions[0]; ++w)\n" /* width */
		"    {\n"
		"        for(int h = 0; h < dimensions[1]; ++h)\n" /* height */
		"        {\n"
		"            for(int d = 0; d < dimensions[2]; ++d)\n" /* depth */
		"            {\n"
		"                ivec3  position  = ivec3(w,h,d);\n"
		"                imageStore(imageWrite,  position, imageLoad(imageRead,  position.xyz));\n"
		"                imageStore(iimageWrite, position, imageLoad(iimageRead, position.xyz));\n"
		"                imageStore(uimageWrite, position, imageLoad(uimageRead, position.xyz));\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"\n"
		"}\n";

	return tessellationControlShaderCode;
}

/** Returns code for Boiler Plate Tessellation Control Shader
 *  @return pointer to literal with Boiler Plate Tessellation Control Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getTessControlShaderCodeBoilerPlate(void)
{
	static const char* tessControlShaderBoilerPlateCode = "${VERSION}\n"
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
														  "}\n";

	return tessControlShaderBoilerPlateCode;
}

/** Returns code for Tessellation Evaluation Shader
 *  @return pointer to literal with Tessellation Evaluation Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getTessEvaluationShaderCode(void)
{
	static const char* tessellationEvaluationShaderCode =
		"${VERSION}\n"
		"\n"
		"${TESSELLATION_SHADER_ENABLE}\n"
		"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout (rgba32f,  binding = 0) highp uniform readonly imageCubeArray  imageRead;\n"
		"layout (rgba32i,  binding = 1) highp uniform readonly iimageCubeArray iimageRead;\n"
		"layout (rgba32ui, binding = 2) highp uniform readonly uimageCubeArray uimageRead;\n"
		"layout (rgba32f,  binding = 3) highp uniform writeonly imageCubeArray  imageWrite;\n"
		"layout (rgba32i,  binding = 4) highp uniform writeonly iimageCubeArray iimageWrite;\n"
		"layout (rgba32ui, binding = 5) highp uniform writeonly uimageCubeArray uimageWrite;\n"
		"\n"
		"uniform ivec3 dimensions;\n"
		"\n"
		"layout(isolines, point_mode) in;"
		"\n"
		"void main()\n"
		"{\n"
		"\n"
		"    for(int w = 0; w < dimensions[0]; ++w)\n" /* width */
		"    {\n"
		"        for(int h = 0; h < dimensions[1]; ++h)\n" /* height */
		"        {\n"
		"            for(int d = 0; d < dimensions[2]; ++d)\n" /* depth */
		"            {\n"
		"                ivec3  position  = ivec3(w,h,d);\n"
		"                imageStore(imageWrite,  position, imageLoad(imageRead,  position));\n"
		"                imageStore(iimageWrite, position, imageLoad(iimageRead, position));\n"
		"                imageStore(uimageWrite, position, imageLoad(uimageRead, position));\n"
		"            }\n"
		"        }\n"
		"    }\n"
		"\n"
		"}\n";

	return tessellationEvaluationShaderCode;
}

/** Returns code for Boiler Plate Tessellation Evaluation Shader
 *  @return pointer to literal with Boiler Plate Tessellation Evaluation Shader code
 **/
const char* TextureCubeMapArrayImageOpCompute::getTessEvaluationShaderCodeBoilerPlate(void)
{
	static const char* tessellationEvaluationShaderBoilerPlateCode = "${VERSION}\n"
																	 "\n"
																	 "${TESSELLATION_SHADER_ENABLE}\n"
																	 "\n"
																	 "precision highp float;\n"
																	 "\n"
																	 "layout(isolines, point_mode) in;"
																	 "\n"
																	 "void main()\n"
																	 "{\n"
																	 "}\n";

	return tessellationEvaluationShaderBoilerPlateCode;
}

const char* TextureCubeMapArrayImageOpCompute::getFloatingPointCopyShaderSource(void)
{
	static const char* floatingPointCopyShaderCode =
		"${VERSION}\n"
		"\n"
		"layout (local_size_x=1) in;\n"
		"\n"
		"layout(binding=0, rgba32f) uniform highp readonly image2D src;\n"
		"layout(binding=1, rgba32ui) uniform highp writeonly uimage2D dst;\n"
		"\n"
		"void main()\n"
		"{\n"
		"ivec2 coord = ivec2(gl_WorkGroupID.xy);\n"
		"imageStore(dst, coord, floatBitsToUint(imageLoad(src, coord)));\n"
		"}\n";

	return floatingPointCopyShaderCode;
}

} /* glcts */
