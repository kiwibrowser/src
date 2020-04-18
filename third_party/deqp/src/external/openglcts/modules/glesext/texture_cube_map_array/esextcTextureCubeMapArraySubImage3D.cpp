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
 * \file  esextcTextureCubeMapArraySubImage3D.cpp
 * \brief Texture Cube Map Array SubImage3D (Test 5)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArraySubImage3D.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <string.h>

namespace glcts
{
const glw::GLuint TextureCubeMapArraySubImage3D::m_n_components   = 4;
const glw::GLuint TextureCubeMapArraySubImage3D::m_n_dimensions   = 3;
const glw::GLuint TextureCubeMapArraySubImage3D::m_n_resolutions  = 4;
const glw::GLuint TextureCubeMapArraySubImage3D::m_n_storage_type = 2;

/* Helper arrays for tests configuration */

/* Different texture resolutions */
const glw::GLuint resolutions[TextureCubeMapArraySubImage3D::m_n_resolutions]
							 [TextureCubeMapArraySubImage3D::m_n_dimensions] = {
								 /* Width , Height, Depth */
								 { 32, 32, 12 },
								 { 64, 64, 12 },
								 { 16, 16, 18 },
								 { 16, 16, 24 }
							 };

/* Location of dimension in array with texture resolutions */
enum Dimensions_Location
{
	DL_WIDTH  = 0,
	DL_HEIGHT = 1,
	DL_DEPTH  = 2
};

/** Constructor
 *
 *  @param context              Test context
 *  @param name                 Test case's name
 *  @param description          Test case's description
 */
TextureCubeMapArraySubImage3D::TextureCubeMapArraySubImage3D(Context& context, const ExtParameters& extParams,
															 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_read_fbo_id(0)
	, m_pixel_buffer_id(0)
	, m_tex_cube_map_array_id(0)
	, m_tex_2d_id(0)
{
	/* Nothing to be done here */
}

/** Initialize test case */
void TextureCubeMapArraySubImage3D::initTest(void)
{
	/* Check if texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_read_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating frame buffer object!");

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_read_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding frame buffer object!");
}

/** Deinitialize test case */
void TextureCubeMapArraySubImage3D::deinit(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES configuration */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	/* Delete GLES objects */
	if (m_read_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_read_fbo_id);
		m_read_fbo_id = 0;
	}

	/* Delete pixel unpack buffer */
	deletePixelUnpackBuffer();

	/* Delete cube map array texture */
	deleteCubeMapArrayTexture();

	/* Delete 2D texture */
	delete2DTexture();

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
 */
tcu::TestCase::IterateResult TextureCubeMapArraySubImage3D::iterate()
{
	initTest();

	glw::GLboolean test_passed = true;

	/* Execute test throught all storage types */
	for (glw::GLuint storage_index = 0; storage_index < m_n_storage_type; ++storage_index)
	{
		/* Execute test throught all texture resolutions */
		for (glw::GLuint resolution_index = 0; resolution_index < m_n_resolutions; ++resolution_index)
		{
			glw::GLuint width  = resolutions[resolution_index][DL_WIDTH];
			glw::GLuint height = resolutions[resolution_index][DL_HEIGHT];
			glw::GLuint depth  = resolutions[resolution_index][DL_DEPTH];

			configureCubeMapArrayTexture(width, height, depth, static_cast<STORAGE_TYPE>(storage_index), 0);

			/* A single whole layer-face at index 0 should be replaced (both functions) */
			SubImage3DCopyParams copy_params;
			copy_params.init(0, 0, 0, width, height, 1);

			configureDataBuffer(width, height, depth, copy_params, 0);
			configurePixelUnpackBuffer(copy_params);
			configure2DTexture(copy_params);
			testTexSubImage3D(width, height, depth, copy_params, test_passed);
			testCopyTexSubImage3D(width, height, depth, copy_params, test_passed);
			deletePixelUnpackBuffer();
			delete2DTexture();

			/* A region of a layer-face at index 0 should be replaced (both functions) */
			copy_params.init(width / 2, height / 2, 0, width / 2, height / 2, 1);

			configureDataBuffer(width, height, depth, copy_params, 0);
			configurePixelUnpackBuffer(copy_params);
			configure2DTexture(copy_params);
			testTexSubImage3D(width, height, depth, copy_params, test_passed);
			testCopyTexSubImage3D(width, height, depth, copy_params, test_passed);
			deletePixelUnpackBuffer();
			delete2DTexture();

			/* 6 layer-faces, making up a single layer, should be replaced (glTexSubImage3D() only) */
			copy_params.init(0, 0, 0, width, height, 6);

			configureDataBuffer(width, height, depth, copy_params, 0);
			configurePixelUnpackBuffer(copy_params);
			testTexSubImage3D(width, height, depth, copy_params, test_passed);
			deletePixelUnpackBuffer();

			/* 6 layer-faces, making up two different layers (for instance: three last layer-faces of
			 layer 1 and three first layer-faces of layer 2) should be replaced (glTexSubImage3D() only) */
			copy_params.init(0, 0, 3, width, height, 6);

			configureDataBuffer(width, height, depth, copy_params, 0);
			configurePixelUnpackBuffer(copy_params);
			testTexSubImage3D(width, height, depth, copy_params, test_passed);
			deletePixelUnpackBuffer();

			/* 6 layer-faces, making up a single layer, should be replaced (glTexSubImage3D() only),
			 but limited to a quad */
			copy_params.init(width / 2, height / 2, 0, width / 2, height / 2, 6);

			configureDataBuffer(width, height, depth, copy_params, 0);
			configurePixelUnpackBuffer(copy_params);
			testTexSubImage3D(width, height, depth, copy_params, test_passed);
			deletePixelUnpackBuffer();

			/* 6 layer-faces, making up two different layers (for instance: three last layer-faces of
			 layer 1 and three first layer-faces of layer 2) should be replaced (glTexSubImage3D() only),
			 but limited to a quad */
			copy_params.init(width / 2, height / 2, 3, width / 2, height / 2, 6);

			configureDataBuffer(width, height, depth, copy_params, 0);
			configurePixelUnpackBuffer(copy_params);
			testTexSubImage3D(width, height, depth, copy_params, test_passed);
			deletePixelUnpackBuffer();

			deleteCubeMapArrayTexture();
		}
	}

	if (test_passed)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

/** Resizes data buffer and fills it with values
 * @param width       - width of the texture
 * @param height      - height of the texture
 * @param depth       - depth of the texture
 * @param copy_params - data structure specifying which region of the data store to replace
 * @param clear_value - value with which to fill the data buffer outside of region specified by copy_params
 */
void TextureCubeMapArraySubImage3D::configureDataBuffer(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
														const SubImage3DCopyParams& copy_params,
														glw::GLuint					clear_value)
{
	glw::GLuint index = 0;

	m_copy_data_buffer.assign(copy_params.m_width * copy_params.m_height * copy_params.m_depth * m_n_components,
							  clear_value);
	for (glw::GLuint zoffset = copy_params.m_zoffset; zoffset < copy_params.m_zoffset + copy_params.m_depth; ++zoffset)
	{
		for (glw::GLuint yoffset = copy_params.m_yoffset; yoffset < copy_params.m_yoffset + copy_params.m_height;
			 ++yoffset)
		{
			for (glw::GLuint xoffset = copy_params.m_xoffset; xoffset < copy_params.m_xoffset + copy_params.m_width;
				 ++xoffset)
			{
				for (glw::GLuint component = 0; component < m_n_components; ++component)
				{
					m_copy_data_buffer[index++] =
						(zoffset * width * height + yoffset * width + xoffset) * m_n_components + component;
				}
			}
		}
	}

	m_expected_data_buffer.assign(width * height * depth * m_n_components, clear_value);
	for (glw::GLuint zoffset = copy_params.m_zoffset; zoffset < copy_params.m_zoffset + copy_params.m_depth; ++zoffset)
	{
		for (glw::GLuint yoffset = copy_params.m_yoffset; yoffset < copy_params.m_yoffset + copy_params.m_height;
			 ++yoffset)
		{
			for (glw::GLuint xoffset = copy_params.m_xoffset; xoffset < copy_params.m_xoffset + copy_params.m_width;
				 ++xoffset)
			{
				glw::GLuint* data_pointer =
					&m_expected_data_buffer[(zoffset * width * height + yoffset * width + xoffset) * m_n_components];
				for (glw::GLuint component = 0; component < m_n_components; ++component)
				{
					data_pointer[component] =
						(zoffset * width * height + yoffset * width + xoffset) * m_n_components + component;
				}
			}
		}
	}
}

/** Creates a pixel unpack buffer that will be used as data source for filling a region of cube map array texture with data
 * @param copy_params - data structure specifying which region of the data store to replace
 */
void TextureCubeMapArraySubImage3D::configurePixelUnpackBuffer(const SubImage3DCopyParams& copy_params)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* generate buffer for pixel unpack buffer */
	gl.genBuffers(1, &m_pixel_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate buffer object!");

	/* bind buffer to PIXEL_UNPACK_BUFFER binding point */
	gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pixel_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");

	/* fill buffer with data */
	gl.bufferData(GL_PIXEL_UNPACK_BUFFER,
				  copy_params.m_width * copy_params.m_height * copy_params.m_depth * m_n_components *
					  sizeof(glw::GLuint),
				  &m_copy_data_buffer[0], GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not fill buffer object's data store with data!");

	gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");
}

/** Creates cube map array texture and fills it with data
 * @param width       - width of the texture
 * @param height      - height of the texture
 * @param depth       - depth of the texture
 * @param storType    - mutable or immutable storage type
 * @param clear_value - value with which to initialize the texture's data store
 */
void TextureCubeMapArraySubImage3D::configureCubeMapArrayTexture(glw::GLuint width, glw::GLuint height,
																 glw::GLuint depth, STORAGE_TYPE storType,
																 glw::GLuint clear_value)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_tex_cube_map_array_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_tex_cube_map_array_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	/* used glTexImage3D() method if texture should be MUTABLE */
	if (storType == ST_MUTABLE)
	{
		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture parameter.");
		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting texture parameter.");

		DataBufferVec data_buffer(width * height * depth * m_n_components, clear_value);

		gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA32UI, width, height, depth, 0, GL_RGBA_INTEGER,
					  GL_UNSIGNED_INT, &data_buffer[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating texture object's data store");
	}
	/* used glTexStorage3D() method if texture should be IMMUTABLE */
	else
	{
		gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_RGBA32UI, width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating texture object's data store");

		clearCubeMapArrayTexture(width, height, depth, clear_value);
	}
}

/** Fills cube map array texture's data store with data
 * @param width       - width of the texture
 * @param height      - height of the texture
 * @param depth       - depth of the texture
 * @param clear_value - value with which to fill the texture's data store
 */
void TextureCubeMapArraySubImage3D::clearCubeMapArrayTexture(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
															 glw::GLuint clear_value)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	DataBufferVec data_buffer(width * height * depth * m_n_components, clear_value);

	gl.texSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0, width, height, depth, GL_RGBA_INTEGER, GL_UNSIGNED_INT,
					 &data_buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture object's data store with data");
}

/** Creates 2D texture that will be used as data source by the glCopyTexSubImage3D call
 * @param copy_params - data structure specifying which region of the data store to replace
 */
void TextureCubeMapArraySubImage3D::configure2DTexture(const SubImage3DCopyParams& copy_params)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_tex_2d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate texture object!");

	gl.bindTexture(GL_TEXTURE_2D, m_tex_2d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind texture object!");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, copy_params.m_width, copy_params.m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating texture object's data store");

	gl.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, copy_params.m_width, copy_params.m_height, GL_RGBA_INTEGER,
					 GL_UNSIGNED_INT, &m_copy_data_buffer[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture object's data store with data");
}

/** Replaces region of cube map array texture's data store using texSubImage3D function
 * @param copy_params    - data structure specifying which region of the data store to replace
 * @param data_pointer   - pointer to the data that should end up in the specified region of the data store
 */
void TextureCubeMapArraySubImage3D::texSubImage3D(const SubImage3DCopyParams& copy_params,
												  const glw::GLuint*		  data_pointer)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.texSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, copy_params.m_xoffset, copy_params.m_yoffset, copy_params.m_zoffset,
					 copy_params.m_width, copy_params.m_height, copy_params.m_depth, GL_RGBA_INTEGER, GL_UNSIGNED_INT,
					 data_pointer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture object's data store with data");
}

/** Replaces region of cube map array texture's data store using copyTexSubImage3D function
 * @param copy_params    - data structure specifying which region of the data store to replace
 */
void TextureCubeMapArraySubImage3D::copyTexSubImage3D(const SubImage3DCopyParams& copy_params)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex_2d_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not attach texture object to framebuffer's attachment");

	checkFramebufferStatus(GL_READ_FRAMEBUFFER);

	gl.copyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, copy_params.m_xoffset, copy_params.m_yoffset,
						 copy_params.m_zoffset, 0, 0, copy_params.m_width, copy_params.m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error filling texture object's data store with data");
}

/** Compares the region of data specified by copy_params taken from the cube map array texture's data store with
 *  the reference data stored in m_data_buffer.
 * @param width       - width of the texture
 * @param height      - height of the texture
 * @param depth       - depth of the texture
 * @return            - true if the result of comparison is that the regions contain the same data, false otherwise
 */
bool TextureCubeMapArraySubImage3D::checkResults(glw::GLuint width, glw::GLuint height, glw::GLuint depth)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint   n_elements = width * height * depth * m_n_components;
	DataBufferVec result_data_buffer(n_elements, 0);

	for (glw::GLuint layer_nr = 0; layer_nr < depth; ++layer_nr)
	{
		/* attach one layer to framebuffer's attachment */
		gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_tex_cube_map_array_id, 0, layer_nr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not attach texture object to framebuffer's attachment");

		/* check framebuffer status */
		checkFramebufferStatus(GL_READ_FRAMEBUFFER);

		/* read data from the texture */
		gl.readPixels(0, 0, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT,
					  &result_data_buffer[layer_nr * width * height * m_n_components]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read pixels from framebuffer's attachment!");
	}

	return memcmp(&result_data_buffer[0], &m_expected_data_buffer[0], n_elements * sizeof(glw::GLuint)) == 0;
}

/** Perform a full test of testTexSubImage3D function on cube map array texture, both with client pointer and pixel unpack buffer
 * @param width          - width of the texture
 * @param height         - height of the texture
 * @param depth          - depth of the texture
 * @param copy_params    - data structure specifying which region of the cube map array to test
 * @param test_passed    - a boolean variable set to false if at any stage of the test we experience wrong result
 */
void TextureCubeMapArraySubImage3D::testTexSubImage3D(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
													  const SubImage3DCopyParams& copy_params,
													  glw::GLboolean&			  test_passed)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	clearCubeMapArrayTexture(width, height, depth, 0);

	texSubImage3D(copy_params, &m_copy_data_buffer[0]);

	if (!checkResults(width, height, depth))
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "glTexSubImage3D failed to copy data to texture cube map array's data store from client's memory\n"
			<< "Texture Cube Map Array Dimensions (width, height, depth) "
			<< "(" << width << "," << height << "," << depth << ")\n"
			<< "Texture Cube Map Array Offsets (xoffset, yoffset, zoffset) "
			<< "(" << copy_params.m_xoffset << "," << copy_params.m_yoffset << "," << copy_params.m_zoffset << ")\n"
			<< "Texture Cube Map Array Copy Size (width, height, depth) "
			<< "(" << copy_params.m_width << "," << copy_params.m_height << "," << copy_params.m_depth << ")\n"
			<< tcu::TestLog::EndMessage;

		test_passed = false;
	}

	clearCubeMapArrayTexture(width, height, depth, 0);

	gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pixel_buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");

	texSubImage3D(copy_params, 0);

	if (!checkResults(width, height, depth))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glTexSubImage3D failed to copy data to texture cube map "
													   "array's data store from GL_PIXEL_UNPACK_BUFFER\n"
						   << "Texture Cube Map Array Dimensions (width, height, depth) "
						   << "(" << width << "," << height << "," << depth << ")\n"
						   << "Texture Cube Map Array Offsets (xoffset, yoffset, zoffset) "
						   << "(" << copy_params.m_xoffset << "," << copy_params.m_yoffset << ","
						   << copy_params.m_zoffset << ")\n"
						   << "Texture Cube Map Array Copy Size (width, height, depth) "
						   << "(" << copy_params.m_width << "," << copy_params.m_height << "," << copy_params.m_depth
						   << ")\n"
						   << tcu::TestLog::EndMessage;

		test_passed = false;
	}

	gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");
}

/** Perform a full test of copyTexSubImage3D function on cube map array texture
 * @param width          - width of the texture
 * @param height         - height of the texture
 * @param depth          - depth of the texture
 * @param copy_params    - data structure specifying which region of the cube map array to test
 * @param test_passed    - a boolean variable set to false if at any stage of the test we experience wrong result
 */
void TextureCubeMapArraySubImage3D::testCopyTexSubImage3D(glw::GLuint width, glw::GLuint height, glw::GLuint depth,
														  const SubImage3DCopyParams& copy_params,
														  glw::GLboolean&			  test_passed)
{
	clearCubeMapArrayTexture(width, height, depth, 0);

	copyTexSubImage3D(copy_params);

	if (!checkResults(width, height, depth))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glCopyTexSubImage3D failed to copy data to texture cube map array's data store\n"
						   << "Texture Cube Map Array Dimensions (width, height, depth) "
						   << "(" << width << "," << height << "," << depth << ")\n"
						   << "Texture Cube Map Array Offsets (xoffset, yoffset, zoffset) "
						   << "(" << copy_params.m_xoffset << "," << copy_params.m_yoffset << ","
						   << copy_params.m_zoffset << ")\n"
						   << "Texture Cube Map Array Copy Size (width, height, depth) "
						   << "(" << copy_params.m_width << "," << copy_params.m_height << "," << copy_params.m_depth
						   << ")\n"
						   << tcu::TestLog::EndMessage;

		test_passed = false;
	}
}

/** Delete pixel unpack buffer */
void TextureCubeMapArraySubImage3D::deletePixelUnpackBuffer()
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES configuration */
	gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	/* Delete buffer object */
	if (m_pixel_buffer_id != 0)
	{
		gl.deleteBuffers(1, &m_pixel_buffer_id);
		m_pixel_buffer_id = 0;
	}
}

/** Delete cube map array texture */
void TextureCubeMapArraySubImage3D::deleteCubeMapArrayTexture()
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES configuration */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

	/* Delete texture object */
	if (m_tex_cube_map_array_id != 0)
	{
		gl.deleteTextures(1, &m_tex_cube_map_array_id);
		m_tex_cube_map_array_id = 0;
	}
}

/* Delete 2D texture that had been used as data source by the glCopyTexSubImage3D call */
void TextureCubeMapArraySubImage3D::delete2DTexture()
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES configuration */
	gl.bindTexture(GL_TEXTURE_2D, 0);

	/* Delete texture object */
	if (m_tex_2d_id != 0)
	{
		gl.deleteTextures(1, &m_tex_2d_id);
		m_tex_2d_id = 0;
	}
}

} // namespace glcts
