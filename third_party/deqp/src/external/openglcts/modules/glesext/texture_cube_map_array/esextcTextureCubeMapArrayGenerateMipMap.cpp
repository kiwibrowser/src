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
 * \file  esextcTextureCubeMapArrayGenerateMipMap.cpp
 * \brief texture_cube_map_array extenstion - glGenerateMipmap() (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayGenerateMipMap.hpp"

#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cmath>
#include <cstring>
#include <vector>

namespace glcts
{

/* Defines two pattern colors for each layer-face */
const unsigned char TextureCubeMapArrayGenerateMipMapFilterable::m_layer_face_data
	[m_n_max_faces][m_n_colors_per_layer_face][m_n_components] = {
		/* Color 1           ---      Color 2      */
		{ { 0, 0, 0, 0 }, { 255, 255, 255, 255 } },   /* Layer-face 0 */
		{ { 255, 0, 0, 0 }, { 0, 255, 255, 255 } },   /* Layer-face 1 */
		{ { 0, 255, 0, 0 }, { 255, 0, 255, 255 } },   /* Layer-face 2 */
		{ { 0, 0, 255, 0 }, { 255, 255, 0, 255 } },   /* Layer-face 3 */
		{ { 0, 0, 0, 255 }, { 255, 255, 255, 0 } },   /* Layer-face 4 */
		{ { 255, 255, 0, 0 }, { 0, 0, 255, 255 } },   /* Layer-face 5 */
		{ { 255, 0, 255, 0 }, { 0, 255, 0, 255 } },   /* Layer-face 6 */
		{ { 255, 0, 0, 255 }, { 0, 255, 255, 0 } },   /* Layer-face 7 */
		{ { 0, 255, 255, 0 }, { 255, 0, 0, 255 } },   /* Layer-face 8 */
		{ { 0, 255, 0, 255 }, { 255, 0, 255, 0 } },   /* Layer-face 9 */
		{ { 255, 255, 255, 0 }, { 0, 0, 0, 255 } },   /* Layer-face 10 */
		{ { 255, 255, 0, 255 }, { 0, 0, 255, 0 } },   /* Layer-face 11 */
		{ { 255, 255, 255, 255 }, { 0, 0, 0, 0 } },   /* Layer-face 12 */
		{ { 0, 0, 0, 255 }, { 255, 0, 0, 0 } },		  /* Layer-face 13 */
		{ { 0, 0, 0, 255 }, { 255, 255, 0, 0 } },	 /* Layer-face 14 */
		{ { 0, 0, 255, 0 }, { 255, 255, 255, 0 } },   /* Layer-face 15 */
		{ { 0, 255, 0, 0 }, { 255, 255, 255, 255 } }, /* Layer-face 16 */
		{ { 255, 0, 0, 0 }, { 255, 0, 255, 255 } }	/* Layer-face 17 */
	};

/** Retrieves maximum amount of levels that should be defined for
 *  a texture of user-provided dimensions.
 *
 *  @param width  Width of the texture in question.
 *  @param height Height of the texture in question.
 *
 *  @return Requested value.
 **/
static int getAmountOfLevelsForTexture(int width, int height)
{
	return (int)floor(log((float)(de::max(width, height))) / log(2.0f)) + 1;
}

/** Constructor
 *
 *  @param context     Test context;
 *  @param name        Test case's name;
 *  @param description Test case's description;
 *  @param storageType Precises whether texture objects used by this
 *                     test instance should be created as immutable or
 *                     mutable objects.
 **/
TextureCubeMapArrayGenerateMipMapFilterable::TextureCubeMapArrayGenerateMipMapFilterable(Context&			  context,
																						 const ExtParameters& extParams,
																						 const char*		  name,
																						 const char*  description,
																						 STORAGE_TYPE storageType)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_storage_type(storageType)
	, m_reference_data_ptr(DE_NULL)
	, m_rendered_data_ptr(DE_NULL)
{
	/* Nothing to be done here */
}

/** Deinitialize test case **/
void TextureCubeMapArrayGenerateMipMapFilterable::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset texture and FBO bindings */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

	/* Release any ES objects that may have been created. */
	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	for (unsigned int i = 0; i < m_storage_configs.size(); ++i)
	{
		if (m_storage_configs[i].m_to_id != 0)
		{
			gl.deleteTextures(1, &m_storage_configs[i].m_to_id);

			m_storage_configs[i].m_to_id = 0;
		}
	}

	/* Release buffers the test may have allocated */
	if (m_reference_data_ptr != DE_NULL)
	{
		delete[] m_reference_data_ptr;

		m_reference_data_ptr = DE_NULL;
	}

	if (m_rendered_data_ptr != DE_NULL)
	{
		delete[] m_rendered_data_ptr;

		m_rendered_data_ptr = DE_NULL;
	}

	/* Restore pixel pack/unpack settings */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 4);

	/* Call base class' deinitialization routine. */
	TestCaseBase::deinit();
}

/** Fills user-provided buffer with expected pixel data for user-specified
 *  layer index.
 *
 * @param n_layer Layer index to return expected data for.
 * @param data    Pointer to a buffer that will be filled with
 *                result data. Must not be NULL.
 * @param width   Render-target width.
 * @param height  Render-target height.
 */
void TextureCubeMapArrayGenerateMipMapFilterable::generateTestData(int n_layer, unsigned char* data, int width,
																   int height)
{
	DE_ASSERT(data != DE_NULL);

	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			for (int n_component = 0; n_component < m_n_components; ++n_component)
			{
				const unsigned int pixel_size = m_n_components;
				unsigned char*	 result_ptr = data + ((y * width + x) * pixel_size + n_component);
				unsigned int	   n_color	= ((x % m_n_colors_per_layer_face) + y) % m_n_colors_per_layer_face;

				*result_ptr = m_layer_face_data[n_layer][n_color][n_component];
			}
		}
	}
}

/** Initialize test case **/
void TextureCubeMapArrayGenerateMipMapFilterable::init()
{
	/* Base class initialization */
	TestCaseBase::init();

	/* Check if texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED);
	}
}

/** Initializes all ES objects that will be used by the test */
void TextureCubeMapArrayGenerateMipMapFilterable::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure the storage config container is empty */
	m_storage_configs.clear();

	/* Update pixel pack/unpack settings */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call(s) failed");

	/* Define following texture configurations
	 *
	 *    [width x height x depth]
	 * 1)   64   x   64   x  18;
	 * 2)  117   x  117   x   6;
	 * 3)  256   x  256   x   6;
	 * 4)  173   x  173   x  12;
	 *
	 * We use GL_RGBA8 internal format in all cases.
	 */

	/* Resolution 64 x 64 x 18 */
	StorageConfig storage_config_1;

	storage_config_1.m_width  = 64;
	storage_config_1.m_height = 64;
	storage_config_1.m_depth  = 18;
	storage_config_1.m_to_id  = 0;
	storage_config_1.m_levels = getAmountOfLevelsForTexture(storage_config_1.m_width, storage_config_1.m_height);

	/* Resolution 117 x 117 x 6 */
	StorageConfig storage_config_2;

	storage_config_2.m_width  = 117;
	storage_config_2.m_height = 117;
	storage_config_2.m_depth  = 6;
	storage_config_2.m_to_id  = 0;
	storage_config_2.m_levels = getAmountOfLevelsForTexture(storage_config_2.m_width, storage_config_2.m_height);

	/* Resolution 256 x 256 x 6 */
	StorageConfig storage_config_3;

	storage_config_3.m_width  = 256;
	storage_config_3.m_height = 256;
	storage_config_3.m_depth  = 6;
	storage_config_3.m_to_id  = 0;
	storage_config_3.m_levels = getAmountOfLevelsForTexture(storage_config_3.m_width, storage_config_3.m_height);

	/* Resolution 173 x 173 x 12 */
	StorageConfig storage_config_4;

	storage_config_4.m_width  = 173;
	storage_config_4.m_height = 173;
	storage_config_4.m_depth  = 12;
	storage_config_4.m_to_id  = 0;
	storage_config_4.m_levels = getAmountOfLevelsForTexture(storage_config_4.m_width, storage_config_4.m_height);

	m_storage_configs.push_back(storage_config_1);
	m_storage_configs.push_back(storage_config_2);
	m_storage_configs.push_back(storage_config_3);
	m_storage_configs.push_back(storage_config_4);

	/* Generate and configure a texture object for each storage config. */
	for (unsigned int n_storage_config = 0; n_storage_config < m_storage_configs.size(); n_storage_config++)
	{
		StorageConfig& config = m_storage_configs[n_storage_config];

		gl.genTextures(1, &config.m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

		gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, config.m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call(s) failed");

		if (m_storage_type == ST_MUTABLE)
		{
			gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0,									/* level */
						  GL_RGBA8, config.m_width, config.m_height, config.m_depth, 0, /* border */
						  GL_RGBA, GL_UNSIGNED_BYTE, DE_NULL);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D() call failed");
		}
		else
		{
			DE_ASSERT(m_storage_type == ST_IMMUTABLE);

			gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, config.m_levels, GL_RGBA8, config.m_width, config.m_height,
							config.m_depth);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed");
		}
	} /* for (all storage configs) */

	/* Generate a frame-buffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed");
}

/** Executes the test.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return Always STOP.
 **/
tcu::TestCase::IterateResult TextureCubeMapArrayGenerateMipMapFilterable::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize ES objects we will need to run the test */
	initTest();

	/* Iterate through all configuration descriptors */
	unsigned int n_storage_config = 0;

	for (std::vector<StorageConfig>::const_iterator storage_config_iterator = m_storage_configs.begin();
		 storage_config_iterator != m_storage_configs.end(); storage_config_iterator++, n_storage_config++)
	{
		const StorageConfig& storage_config = *storage_config_iterator;

		gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, storage_config.m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

		/* Fill each base layer-face with pattern data */
		for (unsigned int n_layer_face = 0; n_layer_face < storage_config.m_depth; ++n_layer_face)
		{
			/* Allocate buffer we will use to store the layer data */
			const int data_size = static_cast<int>(storage_config.m_width * storage_config.m_height * m_n_components *
												   sizeof(unsigned char));
			unsigned char* data_ptr = new unsigned char[data_size];

			if (data_ptr == DE_NULL)
			{
				TCU_FAIL("Out of memory");
			}

			generateTestData(n_layer_face, data_ptr, storage_config.m_width, storage_config.m_height);

			/* Fill base texture-layer Texture */
			gl.texSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0,						 /* level */
							 0,													 /* xoffset */
							 0,													 /* yoffset */
							 n_layer_face,										 /* zoffset */
							 storage_config.m_width, storage_config.m_height, 1, /* depth */
							 GL_RGBA, GL_UNSIGNED_BYTE, data_ptr);

			/* Release the data buffer */
			delete[] data_ptr;

			data_ptr = DE_NULL;

			/* Make sure the call was successful */
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage3D() call failed");
		}

		/* Generate mip-maps */
		gl.generateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenerateMipmap() call failed");

		/* Attach a FBO to GL_READ_FRAMEBUFFER binding point. */
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed");

		/* Allocate buffers to hold reference & rendered data */
		unsigned char	  last_level_data_buffer[m_n_components] = { 0 };
		const unsigned int size =
			static_cast<int>(storage_config.m_width * storage_config.m_height * m_n_components * sizeof(unsigned char));

		m_reference_data_ptr = new unsigned char[size];
		m_rendered_data_ptr  = new unsigned char[size];

		if (m_reference_data_ptr == DE_NULL || m_rendered_data_ptr == DE_NULL)
		{
			TCU_FAIL("Out of memory");
		}

		/* Verify correctness of layer-face data */
		for (unsigned int n_layer_face = 0; n_layer_face < storage_config.m_depth; ++n_layer_face)
		{
			/* Generate reference data */
			generateTestData(n_layer_face, m_reference_data_ptr, storage_config.m_width, storage_config.m_height);

			/* Attach iteration-specific base layer-face to the read frame-buffer */
			gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, storage_config.m_to_id, 0, /* level */
									   n_layer_face);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureLayer() call failed.");

			/* Read the layer-face data */
			gl.readPixels(0, /* x */
						  0, /* y */
						  storage_config.m_width, storage_config.m_height, GL_RGBA, GL_UNSIGNED_BYTE,
						  m_rendered_data_ptr);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

			/* Make sure the base layer-face contents reported with the call is as was uploaded */
			const unsigned int base_layer_data_size = static_cast<int>(
				storage_config.m_width * storage_config.m_height * m_n_components * sizeof(unsigned char));

			if (memcmp(m_reference_data_ptr, m_rendered_data_ptr, base_layer_data_size))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Data stored in base layer mip-map for storage config ["
								   << n_storage_config << "]"
														  "and layer-face index ["
								   << n_layer_face << "]"
													  " is different than was uploaded"
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid data found for base layer mip-map");
			}

			/* Update the read framebuffer's color attachment to read from
			 * the last mip-map available for currently processed layer-face
			 */
			gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, storage_config.m_to_id, /* texture */
									   storage_config.m_levels - 1,										  /* level */
									   n_layer_face);													  /* layer */

			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureLayer() call failed.");

			/* Read the data */
			gl.readPixels(0, /* x */
						  0, /* y */
						  1, /* width */
						  1, /* height */
						  GL_RGBA, GL_UNSIGNED_BYTE, last_level_data_buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

			/* Make sure that the data read from the last layer is not equal to either
			 * of the pattern colors used for the layer-face
			 */
			for (int n_pattern_color = 0; n_pattern_color < m_n_colors_per_layer_face; ++n_pattern_color)
			{
				if (!memcmp(m_layer_face_data[n_layer_face][n_pattern_color], last_level_data_buffer,
							m_n_components * sizeof(unsigned char)))
				{
					m_testCtx.getLog()
						<< tcu::TestLog::Message << "Texel stored in layer-face's smallest mip-map for storage config ["
						<< n_storage_config << "]"
											   "and layer-face index ["
						<< n_layer_face
						<< "]"
						   "describes one of the colors used for the pattern used in base mip-map, which is invalid."
						<< tcu::TestLog::EndMessage;

					TCU_FAIL("Invalid color found in the layer-face's smallest mip-map");
				}
			} /* for (all pattern colors) */
		}	 /* for (all layer-faces) */

		/* Release the buffers we allocated specifically for currently processed
		 * storage config.
		 */
		if (m_reference_data_ptr != DE_NULL)
		{
			delete[] m_reference_data_ptr;

			m_reference_data_ptr = DE_NULL;
		}

		if (m_rendered_data_ptr != DE_NULL)
		{
			delete[] m_rendered_data_ptr;

			m_rendered_data_ptr = DE_NULL;
		}
	} /* for (all storage configs) */

	/* Test has passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 *  @param storageType Precises whether texture objects used by this
 *                     test instance should be created as immutable or
 *                     mutable objects.
 **/
TextureCubeMapArrayGenerateMipMapNonFilterable::TextureCubeMapArrayGenerateMipMapNonFilterable(
	Context& context, const ExtParameters& extParams, const char* name, const char* description,
	STORAGE_TYPE storageType)
	: TestCaseBase(context, extParams, name, description), m_storage_type(storageType)
{
	/* Nothing to be done here */
}

/** Deinitialize test case **/
void TextureCubeMapArrayGenerateMipMapNonFilterable::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Restore default bindings */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

	/* Restore default pixel pack/unpack settings */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 4);

	/* Delete all textures the test may have created. */
	for (unsigned int n_storage_config = 0; n_storage_config < m_non_filterable_texture_configs.size();
		 ++n_storage_config)
	{
		if (m_non_filterable_texture_configs[n_storage_config].m_to_id != 0)
		{
			gl.deleteTextures(1, &m_non_filterable_texture_configs[n_storage_config].m_to_id);

			m_non_filterable_texture_configs[n_storage_config].m_to_id = 0;
		}
	}

	/* Call base class' deinit() implementation */
	TestCaseBase::deinit();
}

/** Initialize test case **/
void TextureCubeMapArrayGenerateMipMapNonFilterable::init()
{
	/* Base class initialization */
	TestCaseBase::init();

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		throw tcu::NotSupportedError("The test can be run only in ES context");
	}

	/* Check if texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED);
	}
}

/** Initializes all ES objects used by the test **/
void TextureCubeMapArrayGenerateMipMapNonFilterable::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure no configs are already in place */
	m_non_filterable_texture_configs.clear();

	/* Update pixel pack/unpack settings */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call(s) failed");

	/* Define a number of storage configurations, all using GL_RGBA32I
	 * internalformat:
	 *
	 *    [width x height x depth]
	 * 1)   64   x   64   x  18;
	 * 2)  117   x  117   x   6;
	 * 3)  256   x  256   x   6;
	 * 4)  173   x  173   x  12;
	 */

	/* Size 64 x 64 x 18 */
	StorageConfig storage_config1;

	storage_config1.m_width  = 64;
	storage_config1.m_height = 64;
	storage_config1.m_depth  = 18;
	storage_config1.m_to_id  = 0;
	storage_config1.m_levels = getAmountOfLevelsForTexture(storage_config1.m_width, storage_config1.m_height);

	/* Size 117 x 117 x 6 */
	StorageConfig storage_config2;

	storage_config2.m_width  = 117;
	storage_config2.m_height = 117;
	storage_config2.m_depth  = 6;
	storage_config2.m_to_id  = 0;
	storage_config2.m_levels = getAmountOfLevelsForTexture(storage_config2.m_width, storage_config2.m_height);

	/* Size 256 x 256 x 6 */
	StorageConfig storage_config3;

	storage_config3.m_width  = 256;
	storage_config3.m_height = 256;
	storage_config3.m_depth  = 6;
	storage_config3.m_to_id  = 0;
	storage_config3.m_levels = getAmountOfLevelsForTexture(storage_config3.m_width, storage_config3.m_height);

	/* Size 173 x 173 x 12 */
	StorageConfig storage_config4;

	storage_config4.m_width  = 173;
	storage_config4.m_height = 173;
	storage_config4.m_depth  = 12;
	storage_config4.m_to_id  = 0;
	storage_config4.m_levels = getAmountOfLevelsForTexture(storage_config4.m_width, storage_config4.m_height);

	m_non_filterable_texture_configs.push_back(storage_config1);
	m_non_filterable_texture_configs.push_back(storage_config2);
	m_non_filterable_texture_configs.push_back(storage_config3);
	m_non_filterable_texture_configs.push_back(storage_config4);

	/* Generate and configure a texture object for each storage config. */
	for (std::vector<StorageConfig>::iterator storage_config_iterator = m_non_filterable_texture_configs.begin();
		 storage_config_iterator != m_non_filterable_texture_configs.end(); storage_config_iterator++)
	{
		StorageConfig& storage_config = *storage_config_iterator;

		gl.genTextures(1, &storage_config.m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

		gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, storage_config.m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call(s) failed.");

		/* Initialize texture storage. */
		if (m_storage_type == ST_MUTABLE)
		{
			gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, /* level */
						  GL_RGBA32I, storage_config.m_width, storage_config.m_height, storage_config.m_depth, 0,
						  GL_RGBA_INTEGER, GL_INT, 0); /* data */

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D() call failed.");
		}
		else
		{
			gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, storage_config.m_levels, GL_RGBA32I, storage_config.m_width,
							storage_config.m_height, storage_config.m_depth);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");
		}
	} /* for (all storage configs) */
}

/** Executes the test.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return Always STOP.
 **/
tcu::TestCase::IterateResult TextureCubeMapArrayGenerateMipMapNonFilterable::iterate()
{
	/* Initialize ES objects used by the test */
	initTest();

	/* Verify that glGenerateMipmap() always throws GL_INVALID_OPERATION, if the
	 * texture object the call would operate on uses non-filterable internalformat.
	 */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (unsigned int n_storage_config = 0; n_storage_config < m_non_filterable_texture_configs.size();
		 ++n_storage_config)
	{
		gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_non_filterable_texture_configs[n_storage_config].m_to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.generateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);

		/* What's the error code at this point? */
		int error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "glGenerateMipmap() operating on an non-filterable internalformat "
								  "did not report GL_INVALID_OPERATION as per spec but "
							   << error_code << " instead." << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid error code reported for an invalid glGenerateMipmap() call.");
		}
	} /* for (all storage configs) */

	/* The test has passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

} /* glcts */
