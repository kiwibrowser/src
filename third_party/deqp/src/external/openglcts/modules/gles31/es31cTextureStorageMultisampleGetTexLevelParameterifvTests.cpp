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
 */ /*!
 * \file  es31cTextureStorageMultisampleGetTexLevelParameterifvTests.cpp
 * \brief Verifies glGetTexLevelParameter(if)v() entry points work correctly.
 *        (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleGetTexLevelParameterifvTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace glcts
{
/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureGetTexLevelParametervFunctionalTest::MultisampleTextureGetTexLevelParametervFunctionalTest(
	Context& context)
	: TestCase(context, "functional_test", "Verifies glGetTexLevelParameter{if}v() entry-points "
										   "work correctly for all ES3.1 texture targets.")
	, gl_oes_texture_storage_multisample_2d_array_supported(GL_FALSE)
	, to_2d(0)
	, to_2d_array(0)
	, to_2d_multisample(0)
	, to_2d_multisample_array(0)
	, to_3d(0)
	, to_cubemap(0)
{
	/* Left blank on purpose */
}

/** Deinitializes GL ES objects used by the test */
void MultisampleTextureGetTexLevelParametervFunctionalTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_2d != 0)
	{
		gl.deleteTextures(1, &to_2d);

		to_2d = 0;
	}

	if (to_2d_array != 0)
	{
		gl.deleteTextures(1, &to_2d_array);

		to_2d_array = 0;
	}

	if (to_2d_multisample != 0)
	{
		gl.deleteTextures(1, &to_2d_multisample);

		to_2d_multisample = 0;
	}

	if (to_2d_multisample_array != 0)
	{
		gl.deleteTextures(1, &to_2d_multisample_array);

		to_2d_multisample_array = 0;
	}

	if (to_3d != 0)
	{
		gl.deleteTextures(1, &to_3d);

		to_3d = 0;
	}

	if (to_cubemap != 0)
	{
		gl.deleteTextures(1, &to_cubemap);

		to_cubemap = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureGetTexLevelParametervFunctionalTest::iterate()
{
	gl_oes_texture_storage_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Retrieve maximum number of sample supported by the implementation for GL_RGB565 internalformat
	 * used for GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target.
	 */
	glw::GLint max_rgb565_internalformat_samples = 0;

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_RGB565, GL_SAMPLES, 1,
							   &max_rgb565_internalformat_samples);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not retrieve maximum supported amount of samples for RGB565 internalformat");
	}

	/* Set up texture property descriptors. We'll use these later to set up texture objects
	 * in an automated manner.
	 */
	_texture_properties texture_2d_array_properties;
	_texture_properties texture_2d_multisample_array_properties;
	_texture_properties texture_2d_multisample_properties;
	_texture_properties texture_2d_properties;
	_texture_properties texture_3d_properties;
	_texture_properties texture_cm_face_properties;

	_texture_properties* texture_descriptors_with_extension[] = { &texture_2d_array_properties,
																  &texture_2d_multisample_array_properties,
																  &texture_2d_multisample_properties,
																  &texture_2d_properties,
																  &texture_3d_properties,
																  &texture_cm_face_properties };

	_texture_properties* texture_descriptors_without_extension[] = { &texture_2d_array_properties,
																	 &texture_2d_multisample_properties,
																	 &texture_2d_properties, &texture_3d_properties,
																	 &texture_cm_face_properties };

	unsigned int		  n_texture_descriptors = 0;
	_texture_properties** texture_descriptors   = NULL;

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		n_texture_descriptors =
			sizeof(texture_descriptors_with_extension) / sizeof(texture_descriptors_with_extension[0]);
		texture_descriptors = texture_descriptors_with_extension;
	}
	else
	{
		n_texture_descriptors =
			sizeof(texture_descriptors_without_extension) / sizeof(texture_descriptors_without_extension[0]);
		texture_descriptors = texture_descriptors_without_extension;
	}

	/* GL_TEXTURE_2D */
	texture_2d_properties.format		 = GL_DEPTH_STENCIL;
	texture_2d_properties.height		 = 16;
	texture_2d_properties.internalformat = GL_DEPTH24_STENCIL8;
	texture_2d_properties.is_2d_texture  = true;
	texture_2d_properties.target		 = GL_TEXTURE_2D;
	texture_2d_properties.to_id_ptr		 = &to_2d;
	texture_2d_properties.type			 = GL_UNSIGNED_INT_24_8;
	texture_2d_properties.width			 = 16;

	texture_2d_properties.expected_compressed		  = GL_FALSE;
	texture_2d_properties.expected_texture_alpha_size = 0;
	texture_2d_properties.expected_texture_alpha_types.push_back(GL_NONE);
	texture_2d_properties.expected_texture_blue_size = 0;
	texture_2d_properties.expected_texture_blue_types.push_back(GL_NONE);
	texture_2d_properties.expected_texture_depth	  = 1;
	texture_2d_properties.expected_texture_depth_size = 24;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_properties.expected_texture_depth_types.push_back(GL_UNSIGNED_INT);
	texture_2d_properties.expected_texture_depth_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_properties.expected_texture_fixed_sample_locations = GL_TRUE;
	texture_2d_properties.expected_texture_green_size			  = 0;
	texture_2d_properties.expected_texture_green_types.push_back(GL_NONE);
	texture_2d_properties.expected_texture_height		   = 16;
	texture_2d_properties.expected_texture_internal_format = GL_DEPTH24_STENCIL8;
	texture_2d_properties.expected_texture_red_size		   = 0;
	texture_2d_properties.expected_texture_red_types.push_back(GL_NONE);
	texture_2d_properties.expected_texture_samples		= 0;
	texture_2d_properties.expected_texture_shared_size  = 0;
	texture_2d_properties.expected_texture_stencil_size = 8;
	texture_2d_properties.expected_texture_width		= 16;

	/* GL_TEXTURE_2D_ARRAY */
	texture_2d_array_properties.depth		   = 32;
	texture_2d_array_properties.format		   = GL_RGBA;
	texture_2d_array_properties.height		   = 32;
	texture_2d_array_properties.internalformat = GL_RGBA8;
	texture_2d_array_properties.is_2d_texture  = false;
	texture_2d_array_properties.target		   = GL_TEXTURE_2D_ARRAY;
	texture_2d_array_properties.to_id_ptr	  = &to_2d_array;
	texture_2d_array_properties.type		   = GL_UNSIGNED_BYTE;
	texture_2d_array_properties.width		   = 32;

	texture_2d_array_properties.expected_compressed			= GL_FALSE;
	texture_2d_array_properties.expected_texture_alpha_size = 8;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_array_properties.expected_texture_alpha_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_array_properties.expected_texture_alpha_types.push_back(GL_UNSIGNED_INT);
	texture_2d_array_properties.expected_texture_blue_size = 8;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_array_properties.expected_texture_blue_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_array_properties.expected_texture_blue_types.push_back(GL_UNSIGNED_INT);
	texture_2d_array_properties.expected_texture_depth		= 32;
	texture_2d_array_properties.expected_texture_depth_size = 0;
	texture_2d_array_properties.expected_texture_depth_types.push_back(GL_NONE);
	texture_2d_array_properties.expected_texture_fixed_sample_locations = GL_TRUE;
	texture_2d_array_properties.expected_texture_green_size				= 8;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_array_properties.expected_texture_green_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_array_properties.expected_texture_green_types.push_back(GL_UNSIGNED_INT);
	texture_2d_array_properties.expected_texture_height			 = 32;
	texture_2d_array_properties.expected_texture_internal_format = GL_RGBA8;
	texture_2d_array_properties.expected_texture_red_size		 = 8;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_array_properties.expected_texture_red_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_array_properties.expected_texture_red_types.push_back(GL_UNSIGNED_INT);
	texture_2d_array_properties.expected_texture_samples	  = 0;
	texture_2d_array_properties.expected_texture_shared_size  = 0;
	texture_2d_array_properties.expected_texture_stencil_size = 0;
	texture_2d_array_properties.expected_texture_width		  = 32;

	/* GL_TEXTURE_2D_MULTISAMPLE */
	texture_2d_multisample_properties.fixedsamplelocations   = GL_FALSE;
	texture_2d_multisample_properties.format				 = GL_RGBA_INTEGER;
	texture_2d_multisample_properties.height				 = 8;
	texture_2d_multisample_properties.internalformat		 = GL_RGBA8UI;
	texture_2d_multisample_properties.is_2d_texture			 = true;
	texture_2d_multisample_properties.is_multisample_texture = true;
	texture_2d_multisample_properties.samples				 = 1;
	texture_2d_multisample_properties.target				 = GL_TEXTURE_2D_MULTISAMPLE;
	texture_2d_multisample_properties.to_id_ptr				 = &to_2d_multisample;
	texture_2d_multisample_properties.type					 = GL_UNSIGNED_INT;
	texture_2d_multisample_properties.width					 = 8;

	texture_2d_multisample_properties.expected_compressed		  = GL_FALSE;
	texture_2d_multisample_properties.expected_texture_alpha_size = 8;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_multisample_properties.expected_texture_alpha_types.push_back(GL_UNSIGNED_INT);
	texture_2d_multisample_properties.expected_texture_alpha_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_multisample_properties.expected_texture_blue_size = 8;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_multisample_properties.expected_texture_blue_types.push_back(GL_UNSIGNED_INT);
	texture_2d_multisample_properties.expected_texture_blue_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_multisample_properties.expected_texture_depth	  = 1;
	texture_2d_multisample_properties.expected_texture_depth_size = 0;
	texture_2d_multisample_properties.expected_texture_depth_types.push_back(GL_NONE);
	texture_2d_multisample_properties.expected_texture_fixed_sample_locations = GL_FALSE;
	texture_2d_multisample_properties.expected_texture_green_size			  = 8;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_multisample_properties.expected_texture_green_types.push_back(GL_UNSIGNED_INT);
	texture_2d_multisample_properties.expected_texture_green_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_multisample_properties.expected_texture_height		   = 8;
	texture_2d_multisample_properties.expected_texture_internal_format = GL_RGBA8UI;
	texture_2d_multisample_properties.expected_texture_red_size		   = 8;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_multisample_properties.expected_texture_red_types.push_back(GL_UNSIGNED_INT);
	texture_2d_multisample_properties.expected_texture_red_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_multisample_properties.expected_texture_samples		= 1;
	texture_2d_multisample_properties.expected_texture_shared_size  = 0;
	texture_2d_multisample_properties.expected_texture_stencil_size = 0;
	texture_2d_multisample_properties.expected_texture_width		= 8;

	/* GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES */
	texture_2d_multisample_array_properties.depth				   = 4; /* note: test spec missed this bit */
	texture_2d_multisample_array_properties.fixedsamplelocations   = GL_TRUE;
	texture_2d_multisample_array_properties.format				   = GL_RGB;
	texture_2d_multisample_array_properties.height				   = 4;
	texture_2d_multisample_array_properties.internalformat		   = GL_RGB565;
	texture_2d_multisample_array_properties.is_2d_texture		   = false;
	texture_2d_multisample_array_properties.is_multisample_texture = true;
	texture_2d_multisample_array_properties.samples				   = max_rgb565_internalformat_samples;
	texture_2d_multisample_array_properties.target				   = GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES;
	texture_2d_multisample_array_properties.to_id_ptr			   = &to_2d_multisample_array;
	texture_2d_multisample_array_properties.type				   = GL_UNSIGNED_BYTE;
	texture_2d_multisample_array_properties.width				   = 4;

	texture_2d_multisample_array_properties.expected_compressed			= GL_FALSE;
	texture_2d_multisample_array_properties.expected_texture_alpha_size = 0;
	texture_2d_multisample_array_properties.expected_texture_alpha_types.push_back(GL_NONE);
	texture_2d_multisample_array_properties.expected_texture_blue_size = 5;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_multisample_array_properties.expected_texture_blue_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_multisample_array_properties.expected_texture_blue_types.push_back(GL_UNSIGNED_INT);
	texture_2d_multisample_array_properties.expected_texture_depth		= 4;
	texture_2d_multisample_array_properties.expected_texture_depth_size = 0;
	texture_2d_multisample_array_properties.expected_texture_depth_types.push_back(GL_NONE);
	texture_2d_multisample_array_properties.expected_texture_fixed_sample_locations = GL_TRUE;
	texture_2d_multisample_array_properties.expected_texture_green_size				= 6;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_multisample_array_properties.expected_texture_green_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_multisample_array_properties.expected_texture_green_types.push_back(GL_UNSIGNED_INT);
	texture_2d_multisample_array_properties.expected_texture_height			 = 4;
	texture_2d_multisample_array_properties.expected_texture_internal_format = GL_RGB565;
	texture_2d_multisample_array_properties.expected_texture_red_size		 = 5;
	/* Return value is implementation-dependent. For current input values GL_UNSIGNED_INT and GL_UNSIGNED_NORMALIZED are valid */
	texture_2d_multisample_array_properties.expected_texture_red_types.push_back(GL_UNSIGNED_NORMALIZED);
	texture_2d_multisample_array_properties.expected_texture_red_types.push_back(GL_UNSIGNED_INT);
	texture_2d_multisample_array_properties.expected_texture_samples	  = max_rgb565_internalformat_samples;
	texture_2d_multisample_array_properties.expected_texture_shared_size  = 0;
	texture_2d_multisample_array_properties.expected_texture_stencil_size = 0;
	texture_2d_multisample_array_properties.expected_texture_width		  = 4;

	/* GL_TEXTURE_3D */
	texture_3d_properties.depth			 = 2;
	texture_3d_properties.format		 = GL_RGB;
	texture_3d_properties.height		 = 2;
	texture_3d_properties.internalformat = GL_RGB9_E5;
	texture_3d_properties.is_2d_texture  = false;
	texture_3d_properties.target		 = GL_TEXTURE_3D;
	texture_3d_properties.to_id_ptr		 = &to_3d;
	texture_3d_properties.type			 = GL_FLOAT;
	texture_3d_properties.width			 = 2;

	texture_3d_properties.expected_compressed		  = GL_FALSE;
	texture_3d_properties.expected_texture_alpha_size = 0;
	texture_3d_properties.expected_texture_alpha_types.push_back(GL_NONE);
	texture_3d_properties.expected_texture_blue_size = 9;
	texture_3d_properties.expected_texture_blue_types.push_back(GL_FLOAT);
	texture_3d_properties.expected_texture_depth	  = 2;
	texture_3d_properties.expected_texture_depth_size = 0;
	texture_3d_properties.expected_texture_depth_types.push_back(GL_NONE);
	texture_3d_properties.expected_texture_fixed_sample_locations = GL_TRUE;
	texture_3d_properties.expected_texture_green_size			  = 9;
	texture_3d_properties.expected_texture_green_types.push_back(GL_FLOAT);
	texture_3d_properties.expected_texture_height		   = 2;
	texture_3d_properties.expected_texture_internal_format = GL_RGB9_E5;
	texture_3d_properties.expected_texture_red_size		   = 9;
	texture_3d_properties.expected_texture_red_types.push_back(GL_FLOAT);
	texture_3d_properties.expected_texture_samples		= 0;
	texture_3d_properties.expected_texture_shared_size  = 5;
	texture_3d_properties.expected_texture_stencil_size = 0;
	texture_3d_properties.expected_texture_width		= 2;

	/* GL_TEXTURE_CUBE_MAP_* */
	texture_cm_face_properties.format		  = GL_RGB_INTEGER;
	texture_cm_face_properties.height		  = 1;
	texture_cm_face_properties.internalformat = GL_RGB16I;
	texture_cm_face_properties.is_cm_texture  = true;
	texture_cm_face_properties.target		  = GL_TEXTURE_CUBE_MAP;
	texture_cm_face_properties.to_id_ptr	  = &to_cubemap;
	texture_cm_face_properties.type			  = GL_SHORT;
	texture_cm_face_properties.width		  = 1;

	texture_cm_face_properties.expected_compressed		   = GL_FALSE;
	texture_cm_face_properties.expected_texture_alpha_size = 0;
	texture_cm_face_properties.expected_texture_alpha_types.push_back(GL_NONE);
	texture_cm_face_properties.expected_texture_blue_size = 16;
	texture_cm_face_properties.expected_texture_blue_types.push_back(GL_INT);
	texture_cm_face_properties.expected_texture_depth	  = 1;
	texture_cm_face_properties.expected_texture_depth_size = 0;
	texture_cm_face_properties.expected_texture_depth_types.push_back(GL_NONE);
	texture_cm_face_properties.expected_texture_fixed_sample_locations = GL_TRUE;
	texture_cm_face_properties.expected_texture_green_size			   = 16;
	texture_cm_face_properties.expected_texture_green_types.push_back(GL_INT);
	texture_cm_face_properties.expected_texture_height			= 1;
	texture_cm_face_properties.expected_texture_internal_format = GL_RGB16I;
	texture_cm_face_properties.expected_texture_red_size		= 16;
	texture_cm_face_properties.expected_texture_red_types.push_back(GL_INT);
	texture_cm_face_properties.expected_texture_samples		 = 0;
	texture_cm_face_properties.expected_texture_shared_size  = 0;
	texture_cm_face_properties.expected_texture_stencil_size = 0;
	texture_cm_face_properties.expected_texture_width		 = 1;

	/* The test needs to be run in two iterations:
	 *
	 * a) In first run,  we need to test immutable textures;
	 * b) In second one, mutable textures should be used.
	 */
	for (unsigned int n_iteration = 0; n_iteration < 2 /* immutable/mutable textures */; ++n_iteration)
	{
		bool is_immutable_run = (n_iteration == 0);

		/* Generate texture object IDs */
		gl.genTextures(1, &to_2d);
		gl.genTextures(1, &to_2d_array);
		gl.genTextures(1, &to_2d_multisample);
		gl.genTextures(1, &to_2d_multisample_array);
		gl.genTextures(1, &to_3d);
		gl.genTextures(1, &to_cubemap);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTexture() call(s) failed.");

		/* Configure texture storage for each target. */
		for (unsigned int n_descriptor = 0; n_descriptor < n_texture_descriptors; ++n_descriptor)
		{
			const _texture_properties* texture_ptr = texture_descriptors[n_descriptor];

			/* Multisample texture targets are not supported by glTexImage*D() API in ES3.1.
			 * Skip two iterations so that we follow the requirement.
			 */
			if (!is_immutable_run && (texture_ptr->target == GL_TEXTURE_2D_MULTISAMPLE ||
									  texture_ptr->target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES))
			{
				continue;
			}

			/* Bind the ID to processed texture target */
			gl.bindTexture(texture_ptr->target, *texture_ptr->to_id_ptr);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

			/* Set up texture storage */
			if (is_immutable_run)
			{
				if (texture_ptr->is_2d_texture)
				{
					if (texture_ptr->is_multisample_texture)
					{
						gl.texStorage2DMultisample(texture_ptr->target, texture_ptr->samples,
												   texture_ptr->internalformat, texture_ptr->width, texture_ptr->height,
												   texture_ptr->fixedsamplelocations);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed");
					}
					else
					{
						gl.texStorage2D(texture_ptr->target, 1, /* levels */
										texture_ptr->internalformat, texture_ptr->width, texture_ptr->height);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");
					}
				}
				else if (texture_ptr->is_cm_texture)
				{
					gl.texStorage2D(GL_TEXTURE_CUBE_MAP, 1, /* levels */
									texture_ptr->internalformat, texture_ptr->width, texture_ptr->height);

					GLU_EXPECT_NO_ERROR(gl.getError(),
										"glTexStorage2D() call failed for GL_TEXTURE_CUBE_MAP texture target");
				}
				else
				{
					if (texture_ptr->target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES)
					{
						if (gl_oes_texture_storage_multisample_2d_array_supported)
						{
							gl.texStorage3DMultisample(texture_ptr->target, texture_ptr->samples,
													   texture_ptr->internalformat, texture_ptr->width,
													   texture_ptr->height, texture_ptr->depth,
													   texture_ptr->fixedsamplelocations);

							GLU_EXPECT_NO_ERROR(gl.getError(), "gltexStorage3DMultisample() call failed");
						}
						else
						{
							TCU_FAIL("Invalid texture target is being used.");
						}
					}
					else
					{
						/* Must be a single-sampled 2D array or 3D texture */
						gl.texStorage3D(texture_ptr->target, 1, /* levels */
										texture_ptr->internalformat, texture_ptr->width, texture_ptr->height,
										texture_ptr->depth);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed");
					}
				}
			} /* if (!is_immutable_run) */
			else
			{
				/* Mutable run */
				if (texture_ptr->is_2d_texture)
				{
					gl.texImage2D(texture_ptr->target, 0,												   /* level */
								  texture_ptr->internalformat, texture_ptr->width, texture_ptr->height, 0, /* border */
								  texture_ptr->format, texture_ptr->type, NULL);						   /* pixels */

					GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D() call failed");
				}
				else if (texture_ptr->is_cm_texture)
				{
					const glw::GLenum cm_texture_targets[] = {
						GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
						GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
					};
					const unsigned int n_cm_texture_targets =
						sizeof(cm_texture_targets) / sizeof(cm_texture_targets[0]);

					for (unsigned int n_cm_texture_target = 0; n_cm_texture_target < n_cm_texture_targets;
						 ++n_cm_texture_target)
					{
						glw::GLenum texture_target = cm_texture_targets[n_cm_texture_target];

						gl.texImage2D(texture_target, 0, /* level */
									  texture_ptr->internalformat, texture_ptr->width, texture_ptr->height,
									  0,											 /* border */
									  texture_ptr->format, texture_ptr->type, NULL); /* pixels */

						GLU_EXPECT_NO_ERROR(gl.getError(),
											"glTexImage2D() call failed for a cube-map face texture target");
					} /* for (all cube-map texture targets) */
				}
				else
				{
					/* Must be a 2D array texture or 3D texture */
					gl.texImage3D(texture_ptr->target, 0, /* level */
								  texture_ptr->internalformat, texture_ptr->width, texture_ptr->height,
								  texture_ptr->depth, 0,						 /* border */
								  texture_ptr->format, texture_ptr->type, NULL); /* pixels */

					GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D() call failed");
				}
			}
		} /* for (all texture descriptors) */

		/* Check if correct values are reported for all texture properties described in
		 * the spec.
		 */
		typedef std::map<glw::GLenum, const _texture_properties*> target_to_texture_properties_map;
		typedef target_to_texture_properties_map::const_iterator target_to_texture_properties_map_const_iterator;

		const glw::GLenum pnames[] = { GL_TEXTURE_RED_TYPE,
									   GL_TEXTURE_GREEN_TYPE,
									   GL_TEXTURE_BLUE_TYPE,
									   GL_TEXTURE_ALPHA_TYPE,
									   GL_TEXTURE_DEPTH_TYPE,
									   GL_TEXTURE_RED_SIZE,
									   GL_TEXTURE_GREEN_SIZE,
									   GL_TEXTURE_BLUE_SIZE,
									   GL_TEXTURE_ALPHA_SIZE,
									   GL_TEXTURE_DEPTH_SIZE,
									   GL_TEXTURE_STENCIL_SIZE,
									   GL_TEXTURE_SHARED_SIZE,
									   GL_TEXTURE_COMPRESSED,
									   GL_TEXTURE_INTERNAL_FORMAT,
									   GL_TEXTURE_WIDTH,
									   GL_TEXTURE_HEIGHT,
									   GL_TEXTURE_DEPTH,
									   GL_TEXTURE_SAMPLES,
									   GL_TEXTURE_FIXED_SAMPLE_LOCATIONS };
		const unsigned int				 n_pnames = sizeof(pnames) / sizeof(pnames[0]);
		target_to_texture_properties_map targets;

		targets[GL_TEXTURE_2D]					= &texture_2d_properties;
		targets[GL_TEXTURE_2D_ARRAY]			= &texture_2d_array_properties;
		targets[GL_TEXTURE_2D_MULTISAMPLE]		= &texture_2d_multisample_properties;
		targets[GL_TEXTURE_3D]					= &texture_3d_properties;
		targets[GL_TEXTURE_CUBE_MAP_NEGATIVE_X] = &texture_cm_face_properties;
		targets[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y] = &texture_cm_face_properties;
		targets[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z] = &texture_cm_face_properties;
		targets[GL_TEXTURE_CUBE_MAP_POSITIVE_X] = &texture_cm_face_properties;
		targets[GL_TEXTURE_CUBE_MAP_POSITIVE_Y] = &texture_cm_face_properties;
		targets[GL_TEXTURE_CUBE_MAP_POSITIVE_Z] = &texture_cm_face_properties;

		if (gl_oes_texture_storage_multisample_2d_array_supported)
		{
			targets[GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES] = &texture_2d_multisample_array_properties;
		}

		for (target_to_texture_properties_map_const_iterator target_iterator = targets.begin();
			 target_iterator != targets.end(); target_iterator++)
		{
			glw::GLenum				   target	  = target_iterator->first;
			const _texture_properties* texture_ptr = target_iterator->second;

			/* Multisample texture targets are not supported by glTexImage*D() API in ES3.1.
			 * Skip two iterations so that we follow the requirement.
			 */
			if (m_context.getRenderContext().getType().getAPI() == glu::ApiType::es(3, 1) && !is_immutable_run &&
				(texture_ptr->target == GL_TEXTURE_2D_MULTISAMPLE ||
				 texture_ptr->target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES))
			{
				continue;
			}

			for (unsigned int n_pname = 0; n_pname < n_pnames; ++n_pname)
			{
				glw::GLenum pname = pnames[n_pname];

				/* Run the check in two stages:
				 *
				 * a) Use glGetTexLevelParameteriv() in first run;
				 * b) Use glGetTexLevelParameterfv() in the other run;
				 */
				for (unsigned int n_stage = 0; n_stage < 2 /* stages */; ++n_stage)
				{
					glw::GLfloat float_value = 0.0f;
					glw::GLint   int_value   = 0;

					/* Check if the retrieved value is correct */
					glw::GLenum				expected_error_code = GL_NO_ERROR;
					std::vector<glw::GLint> expected_int_values;

					switch (pname)
					{
					case GL_TEXTURE_RED_TYPE:
					{
						/* Return value is implementation-dependent and not enforced by spec.
						 * For this pname, more that one value could be valid. */
						expected_int_values = texture_ptr->expected_texture_red_types;

						break;
					}

					case GL_TEXTURE_GREEN_TYPE:
					{
						/* Return value is implementation-dependent and not enforced by spec.
						 * For this pname, more that one value could be valid. */
						expected_int_values = texture_ptr->expected_texture_green_types;

						break;
					}

					case GL_TEXTURE_BLUE_TYPE:
					{
						/* Return value is implementation-dependent and not enforced by spec.
						 * For this pname, more that one value could be valid. */
						expected_int_values = texture_ptr->expected_texture_blue_types;

						break;
					}

					case GL_TEXTURE_ALPHA_TYPE:
					{
						/* Return value is implementation-dependent and not enforced by spec.
						 * For this pname, more that one value could be valid. */
						expected_int_values = texture_ptr->expected_texture_alpha_types;

						break;
					}

					case GL_TEXTURE_DEPTH_TYPE:
					{
						/* Return value is implementation-dependent and not enforced by spec.
						 * For this pname, more that one value could be valid. */
						expected_int_values = texture_ptr->expected_texture_depth_types;

						break;
					}

					case GL_TEXTURE_RED_SIZE:
					{
						/* Return value is implementation-dependent and not enforced by spec,
						 * but it can't be less that expected_int_value */
						expected_int_values.push_back(texture_ptr->expected_texture_red_size);

						break;
					}

					case GL_TEXTURE_GREEN_SIZE:
					{
						/* Return value is implementation-dependent and not enforced by spec,
						 * but it can't be less that expected_int_value */
						expected_int_values.push_back(texture_ptr->expected_texture_green_size);

						break;
					}

					case GL_TEXTURE_BLUE_SIZE:
					{
						/* Return value is implementation-dependent and not enforced by spec,
						 * but it can't be less that expected_int_value */
						expected_int_values.push_back(texture_ptr->expected_texture_blue_size);

						break;
					}

					case GL_TEXTURE_ALPHA_SIZE:
					{
						/* Return value is implementation-dependent and not enforced by spec,
						 * but it can't be less that expected_int_value */
						expected_int_values.push_back(texture_ptr->expected_texture_alpha_size);

						break;
					}

					case GL_TEXTURE_DEPTH_SIZE:
					{
						/* Return value is implementation-dependent and not enforced by spec,
						 * but it can't be less that expected_int_value */
						expected_int_values.push_back(texture_ptr->expected_texture_depth_size);

						break;
					}

					case GL_TEXTURE_STENCIL_SIZE:
					{
						/* Return value is implementation-dependent and not enforced by spec,
						 * but it can't be less that expected_int_value */
						expected_int_values.push_back(texture_ptr->expected_texture_stencil_size);

						break;
					}

					case GL_TEXTURE_SHARED_SIZE:
					{
						/* Return value is implementation-dependent and not enforced by spec,
						 * but it can't be less that expected_int_value */
						expected_int_values.push_back(texture_ptr->expected_texture_shared_size);

						break;
					}

					case GL_TEXTURE_COMPRESSED:
					{
						expected_int_values.push_back(texture_ptr->expected_compressed);

						break;
					}

					case GL_TEXTURE_INTERNAL_FORMAT:
					{
						expected_int_values.push_back(texture_ptr->expected_texture_internal_format);

						break;
					}

					case GL_TEXTURE_WIDTH:
					{
						expected_int_values.push_back(texture_ptr->expected_texture_width);

						break;
					}

					case GL_TEXTURE_HEIGHT:
					{
						expected_int_values.push_back(texture_ptr->expected_texture_height);

						break;
					}

					case GL_TEXTURE_DEPTH:
					{
						expected_int_values.push_back(texture_ptr->expected_texture_depth);

						break;
					}

					case GL_TEXTURE_SAMPLES:
					{
						expected_int_values.push_back(texture_ptr->expected_texture_samples);

						break;
					}

					case GL_TEXTURE_FIXED_SAMPLE_LOCATIONS:
					{
						expected_int_values.push_back(texture_ptr->expected_texture_fixed_sample_locations);

						break;
					}

					default:
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Unrecognized pname [" << pname << "]"
										   << tcu::TestLog::EndMessage;

						TCU_FAIL("Unrecognized pname");
					}
					} /* switch (pname) */

					/* Do the actual call. If we're expecting an error, make sure it was generated.
					 * Otherwise, confirm no error was generated by the call */
					glw::GLenum error_code = GL_NO_ERROR;

					if (n_stage == 0)
					{
						/* call glGetTexLevelParameteriv() */
						gl.getTexLevelParameteriv(target, 0 /* level */, pname, &int_value);

						error_code = gl.getError();
						if (error_code != expected_error_code)
						{
							m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexLevelParameteriv() for pname ["
											   << pname << "]"
											   << " and target [" << target << "]"
											   << " generated an invalid error code [" << error_code << "]"
											   << tcu::TestLog::EndMessage;

							TCU_FAIL("Error calling glGetTexLevelParameteriv()");
						} /* if (error_code != GL_NO_ERROR) */
					}	 /* if (n_stage == 0) */
					else
					{
						/* call glGetTexLevelParameterfv() */
						gl.getTexLevelParameterfv(target, 0 /* level */, pname, &float_value);

						error_code = gl.getError();
						if (error_code != expected_error_code)
						{
							m_testCtx.getLog() << tcu::TestLog::Message << "glGetTexLevelParameteriv() for pname ["
											   << pname << "]"
											   << " and target [" << target << "]"
											   << " generated an invalid error code [" << error_code << "]"
											   << tcu::TestLog::EndMessage;

							TCU_FAIL("Error calling glGetTexLevelParameterfv()");
						} /* if (error_code != GL_NO_ERROR) */

						/* Cast the result to an integer - this is fine since none of the properties we are
						 * querying is FP.
						 **/
						DE_ASSERT(float_value == (float)(int)float_value);
						int_value = (int)float_value;
					}

					/* Check the result value only if no error was expected */
					if (expected_error_code == GL_NO_ERROR)
					{
						switch (pname)
						{
						case GL_TEXTURE_RED_SIZE:
						case GL_TEXTURE_GREEN_SIZE:
						case GL_TEXTURE_BLUE_SIZE:
						case GL_TEXTURE_ALPHA_SIZE:
						case GL_TEXTURE_DEPTH_SIZE:
						case GL_TEXTURE_STENCIL_SIZE:
						case GL_TEXTURE_SHARED_SIZE:
						case GL_TEXTURE_SAMPLES:
						{
							/* For some pnames with size, value range is valid.
							 * For example for GL_RGB16I and GL_TEXTURE_RED_SIZE implementation may return 16 or 32,
							 * which will still comply with the spec.
							 * In such case we check if returned value is no less than 16.
							 */
							if (expected_int_values.at(0) > int_value)
							{
								m_testCtx.getLog() << tcu::TestLog::Message << "Too small value reported for pname ["
												   << pname << "]"
												   << " and target [" << target << "]"
												   << " expected not less than:[" << expected_int_values.at(0) << "]"
												   << " retrieved:[" << int_value << "]" << tcu::TestLog::EndMessage;

								TCU_FAIL("Invalid value reported.");
							}
							break;
						}

						case GL_TEXTURE_COMPRESSED:
						case GL_TEXTURE_INTERNAL_FORMAT:
						case GL_TEXTURE_WIDTH:
						case GL_TEXTURE_HEIGHT:
						case GL_TEXTURE_DEPTH:
						case GL_TEXTURE_FIXED_SAMPLE_LOCATIONS:
						{
							if (expected_int_values.at(0) != int_value)
							{
								m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value reported for pname ["
												   << pname << "]"
												   << " and target [" << target << "]"
												   << " expected:[" << expected_int_values.at(0) << "]"
												   << " retrieved:[" << int_value << "]" << tcu::TestLog::EndMessage;

								TCU_FAIL("Invalid value reported.");
							}
							break;
						}

						case GL_TEXTURE_RED_TYPE:
						case GL_TEXTURE_GREEN_TYPE:
						case GL_TEXTURE_BLUE_TYPE:
						case GL_TEXTURE_ALPHA_TYPE:
						case GL_TEXTURE_DEPTH_TYPE:
						{
							/* For some pnames with types, more than one value could be valid.
							 * For example for GL_DEPTH24_STENCIL8 and GL_DEPTH_TYPE query, the returned value is implementation
							 * dependent, some implementations return GL_UNSIGNED_NORMALIZED, other may return GL_UNSIGNED_INT
							 * depending on hardware specific representation of depth.
							 */

							std::vector<glw::GLint>::iterator expected_value_it =
								find(expected_int_values.begin(), expected_int_values.end(), int_value);

							if (expected_value_it == expected_int_values.end())
							{
								std::ostringstream expected_values_string_stream;

								for (std::vector<glw::GLint>::iterator it = expected_int_values.begin();
									 it != expected_int_values.end(); ++it)
								{
									if (it != expected_int_values.begin())
									{
										expected_values_string_stream << ", ";
									}

									expected_values_string_stream << *it;
								}

								m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value reported for pname ["
												   << pname << "]"
												   << " and target [" << target << "]"
												   << " expected:[" << expected_values_string_stream.str() << "]"
												   << " retrieved:[" << int_value << "]" << tcu::TestLog::EndMessage;

								TCU_FAIL("Invalid value reported.");
							}
							break;
						}

						default:
						{
							m_testCtx.getLog() << tcu::TestLog::Message << "Unrecognized pname [" << pname << "]"
											   << tcu::TestLog::EndMessage;

							TCU_FAIL("Unrecognized pname");
						} /* default: */
						} /* switch (pname) */
					}	 /* if (expected_error_code == GL_NO_ERROR) */
				}		  /* for (all stages) */
			}			  /* for (all properties) */
		}				  /* for (all texture targets) */

		/* Iteration finished - clean up. */
		for (unsigned int n_descriptor = 0; n_descriptor < n_texture_descriptors; ++n_descriptor)
		{
			const _texture_properties* texture_ptr = texture_descriptors[n_descriptor];

			/* Release the texture object */
			gl.bindTexture(texture_ptr->target, 0);
			gl.deleteTextures(1, texture_ptr->to_id_ptr);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed");

			/* Assign a new object to the ID */
			gl.genTextures(1, texture_ptr->to_id_ptr);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");
		} /* for (all texture descriptors) */
	}	 /* for (immutable & mutable textures) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureGetTexLevelParametervWorksForMaximumLodTest::
	MultisampleTextureGetTexLevelParametervWorksForMaximumLodTest(Context& context)
	: TestCase(context, "functional_max_lod_test", "Verifies glGetTexLevelParameter{if}v() entry-points work "
												   "correctly when info about maximum LOD is requested.")
	, gl_oes_texture_storage_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes GL ES objects used by the test */
void MultisampleTextureGetTexLevelParametervWorksForMaximumLodTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id != 0)
	{
		gl.deleteTextures(1, &to_id);

		to_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureGetTexLevelParametervWorksForMaximumLodTest::iterate()
{
	gl_oes_texture_storage_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	const glw::Functions& gl				   = m_context.getRenderContext().getFunctions();
	int					  number_of_iterations = 0;

	/* Generate a texture object and bind id to a 2D multisample texture target */
	gl.genTextures(1, &to_id);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a texture object");

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		/* Run in two iterations:
		 *
		 * a) Texture storage initialized with glTexStorage2DMultisample();
		 * b) Texture storage initialized with gltexStorage3DMultisample().
		 */
		number_of_iterations = 2;
	}
	else
	{
		/* Run in one iteration:
		 *
		 * a) Texture storage initialized with glTexStorage2DMultisample();
		 */
		number_of_iterations = 1;
	}

	/* Run in two iterations:
	 *
	 * a) Texture storage initialized with glTexStorage2DMultisample();
	 * b) Texture storage initialized with gltexStorage3DMultisample().
	 */
	for (int n_iteration = 0; n_iteration < number_of_iterations; ++n_iteration)
	{
		bool	  is_2d_multisample_iteration = (n_iteration == 0);
		const int max_lod =
			0; /* For multisample textures only lod=0 is valid, queries for lod > 0 will always return GL_NONE, as those lods are not defined for such texture */
		glw::GLenum texture_target =
			(n_iteration == 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES;

		/* Set up texture storage */
		if (is_2d_multisample_iteration)
		{
			gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, /* samples */
									   GL_RGBA8, 16,				 /* width */
									   16,							 /* height */
									   GL_FALSE);					 /* fixedsamplelocations */

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed");
		} /* if (is_2d_multisample_iteration) */
		else
		{
			gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 1, /* samples */
									   GL_RGBA8, 16,						   /* width */
									   16,									   /* height */
									   16,									   /* depth */
									   GL_FALSE);							   /* fixedsamplelocations */

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed");
		}

		/* Check all glGetTexLevelParameter*() entry-points */
		for (int n_api_call = 0; n_api_call < 2 /* iv(), fv() */; ++n_api_call)
		{
			float	  float_value			   = 0;
			glw::GLint red_size				   = 0;
			glw::GLint red_type				   = 0;
			glw::GLint texture_internal_format = GL_NONE;
			glw::GLint texture_samples		   = 0;

			switch (n_api_call)
			{
			case 0:
			{
				gl.getTexLevelParameteriv(texture_target, max_lod, GL_TEXTURE_RED_TYPE, &red_type);
				gl.getTexLevelParameteriv(texture_target, max_lod, GL_TEXTURE_RED_SIZE, &red_size);
				gl.getTexLevelParameteriv(texture_target, max_lod, GL_TEXTURE_INTERNAL_FORMAT,
										  &texture_internal_format);
				gl.getTexLevelParameteriv(texture_target, max_lod, GL_TEXTURE_SAMPLES, &texture_samples);

				GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glGetTexLevelParameteriv() call failed.");

				break;
			}

			case 1:
			{
				gl.getTexLevelParameterfv(texture_target, max_lod, GL_TEXTURE_RED_TYPE, &float_value);
				red_type = (glw::GLint)float_value;

				gl.getTexLevelParameterfv(texture_target, max_lod, GL_TEXTURE_RED_SIZE, &float_value);
				red_size = (glw::GLint)float_value;

				gl.getTexLevelParameterfv(texture_target, max_lod, GL_TEXTURE_INTERNAL_FORMAT, &float_value);
				texture_internal_format = (glw::GLint)float_value;

				gl.getTexLevelParameterfv(texture_target, max_lod, GL_TEXTURE_SAMPLES, &float_value);
				texture_samples = (glw::GLint)float_value;

				GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glGetTexLevelParameterfv() call failed.");

				break;
			}

			default:
				TCU_FAIL("Unrecognized API call index");
			}

			/* Make sure the retrieved values are valid
			 *
			 * NOTE: The original test case did not make much sense in this regard. */
			if (red_type != GL_UNSIGNED_NORMALIZED)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "Invalid value returned for a GL_TEXTURE_RED_TYPE query: "
								   << "expected:GL_UNSIGNED_NORMALIZED, retrieved:" << red_type
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid value returned for a GL_TEXTURE_RED_TYPE query");
			}

			if (red_size != 8)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "Invalid value returned for a GL_TEXTURE_RED_SIZE query: "
								   << "expected:8, retrieved:" << red_size << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid value returned for a GL_TEXTURE_RED_SIZE query");
			}

			if (texture_internal_format != GL_RGBA8)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "Invalid value returned for a GL_TEXTURE_INTERNAL_FORMAT query: "
								   << "expected:GL_RGBA8, retrieved:" << texture_internal_format
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid value returned for a GL_TEXTURE_INTERNAL_FORMAT query");
			}

			/* Implementation is allowed to return more samples than requested */
			if (texture_samples < 1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value returned for a GL_TEXTURE_SAMPLES query: "
								   << "expected:1, retrieved:" << texture_samples << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid value returned for a GL_TEXTURE_SAMPLES query");
			}
		} /* for (both API call types) */

		/* Re-create the texture object and bind it to 2D multisample array texture target */
		gl.deleteTextures(1, &to_id);
		to_id = 0;

		/* Prepare for the next iteration (if needed). */
		if (n_iteration == 0 && number_of_iterations == 2)
		{
			gl.genTextures(1, &to_id);
			gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not re-create the texture object.");
	} /* for (all iterations) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureGetTexLevelParametervInvalidTextureTargetRejectedTest::
	MultisampleTextureGetTexLevelParametervInvalidTextureTargetRejectedTest(Context& context)
	: TestCase(context, "invalid_texture_target_rejected", "Verifies glGetTexLevelParameter{if}v() rejects invalid "
														   "texture target by generating GL_INVALID_ENUM error.")
	, float_data(0.0f)
	, int_data(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureGetTexLevelParametervInvalidTextureTargetRejectedTest::deinit()
{
	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureGetTexLevelParametervInvalidTextureTargetRejectedTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Call glGetTexLevelParameteriv() with invalid texture target. */
	gl.getTexLevelParameteriv(GL_TEXTURE_WIDTH /* invalid value */, 0, /* level */
							  GL_TEXTURE_RED_TYPE, &int_data);

	/* Expect GL_INVALID_ENUM error code. */
	TCU_CHECK_MSG(gl.getError() == GL_INVALID_ENUM,
				  "glGetTexLevelParameteriv() did not generate GL_INVALID_ENUM error.");

	/* Call glGetTexLevelParameterfv() with invalid texture target. */
	gl.getTexLevelParameterfv(GL_TEXTURE_WIDTH /* invalid value */, 0, /* level */
							  GL_TEXTURE_RED_TYPE, &float_data);

	/* Expect GL_INVALID_ENUM error code. */
	TCU_CHECK_MSG(gl.getError() == GL_INVALID_ENUM,
				  "glGetTexLevelParameterfv() did not generate GL_INVALID_ENUM error.");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureGetTexLevelParametervInvalidValueArgumentRejectedTest::
	MultisampleTextureGetTexLevelParametervInvalidValueArgumentRejectedTest(Context& context)
	: TestCase(context, "invalid_value_argument_rejected", "Verifies glGetTexLevelParameter{if}v() rejects invalid "
														   "value argument by generating GL_INVALID_VALUE error.")
	, float_data(0.0f)
	, int_data(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureGetTexLevelParametervInvalidValueArgumentRejectedTest::deinit()
{
	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureGetTexLevelParametervInvalidValueArgumentRejectedTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Call glGetTexLevelParameteriv() with invalid value argument. */
	gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, /* level */
							  GL_SIGNED_NORMALIZED /* invalid value */, &int_data);

	/* From spec:
	 * An INVALID_ENUM error is generated if pname is not one of the symbolic values in tables 6.12.
	 */

	/* Expect INVALID_ENUM error code. */
	glw::GLenum error_code = gl.getError();

	TCU_CHECK_MSG(error_code == GL_INVALID_ENUM, "glGetTexLevelParameteriv() did not generate GL_INVALID_ENUM error.");

	/* Call glGetTexLevelParameterfv() with invalid value argument. */
	gl.getTexLevelParameterfv(GL_TEXTURE_2D, 0, /* level */
							  GL_SIGNED_NORMALIZED /* invalid value */, &float_data);

	/* Expect INVALID_ENUM error code. */
	error_code = gl.getError();

	TCU_CHECK_MSG(error_code == GL_INVALID_ENUM, "glGetTexLevelParameterfv() did not generate GL_INVALID_ENUM error.");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureGetTexLevelParametervNegativeLodIsRejectedTest::
	MultisampleTextureGetTexLevelParametervNegativeLodIsRejectedTest(Context& context)
	: TestCase(context, "negative_lod_is_rejected_test", "Verifies glGetTexLevelParameter{if}v() rejects negative "
														 "<lod> by generating GL_INVALID_VALUE error.")
	, float_data(0.0f)
	, int_data(0)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureGetTexLevelParametervNegativeLodIsRejectedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	/* Delete texture object. */
	gl.deleteTextures(1, &to_id);

	to_id = 0;

	/* Check if no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object deletion failed.");

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureGetTexLevelParametervNegativeLodIsRejectedTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id. */
	gl.genTextures(1, &to_id);

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	/* Check if no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureGetTexLevelParametervNegativeLodIsRejectedTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Set up texture storage. */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, /* samples */
							   GL_RGBA8, 16,				 /* width */
							   16,							 /* height */
							   GL_FALSE);

	/* Check if no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");

	/* Call glGetTexLevelParameteriv() with negative lod. */
	gl.getTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE, -1 /* negative lod */, GL_TEXTURE_RED_TYPE, &int_data);

	/* Expect GL_INVALID_VALUE error code. */
	TCU_CHECK_MSG(gl.getError() == GL_INVALID_VALUE,
				  "glGetTexLevelParameteriv() did not generate GL_INVALID_VALUE error.");

	/* Call glGetTexLevelParameterfv() with negative lod. */
	gl.getTexLevelParameterfv(GL_TEXTURE_2D_MULTISAMPLE, -1 /* negative lod */, GL_TEXTURE_RED_TYPE, &float_data);

	/* Expect GL_INVALID_VALUE error code. */
	TCU_CHECK_MSG(gl.getError() == GL_INVALID_VALUE,
				  "glGetTexLevelParameterfv() did not generate GL_INVALID_VALUE error.");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}
} /* glcts namespace */
