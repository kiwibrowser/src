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

#include "esextcGeometryShaderLayeredFramebuffer.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{
/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderLayeredFramebufferBlending::GeometryShaderLayeredFramebufferBlending(Context&				context,
																				   const ExtParameters& extParams,
																				   const char*			name,
																				   const char*			description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_read_fbo_id(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderLayeredFramebufferBlending::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean up */
	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
	}

	if (m_read_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_read_fbo_id);
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderLayeredFramebufferBlending::iterate(void)
{
/* Test-wide constants */
#define N_TEXTURE_COMPONENTS (4)
#define TEXTURE_DEPTH (4)
#define TEXTURE_HEIGHT (4)
#define TEXTURE_WIDTH (4)

	/* Fragment shader code */
	const char* fs_code = "${VERSION}\n"
						  "\n"
						  "precision highp float;\n"
						  "\n"
						  "out vec4 result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    result = vec4(0.2);\n"
						  "}\n";

	/* Geometry shader code */
	const char* gs_code = "${VERSION}\n"
						  "${GEOMETRY_SHADER_REQUIRE}\n"
						  "\n"
						  "layout(points)                          in;\n"
						  "layout(triangle_strip, max_vertices=64) out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    for (int n = 0; n < 4; ++n)\n"
						  "    {\n"
						  "        gl_Layer    = n;\n"
						  "        gl_Position = vec4(1, 1, 0, 1);\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Layer    = n;\n"
						  "        gl_Position = vec4(1, -1, 0, 1);\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Layer    = n;\n"
						  "        gl_Position = vec4(-1, 1, 0, 1);\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Layer    = n;\n"
						  "        gl_Position = vec4(-1, -1, 0, 1);\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        EndPrimitive();\n"
						  "    }\n"
						  "}\n";

	/* General variables */
	const glw::Functions& gl		  = m_context.getRenderContext().getFunctions();
	unsigned int		  n			  = 0;
	unsigned int		  n_component = 0;
	unsigned int		  n_layer	 = 0;
	unsigned int		  n_slice	 = 0;
	unsigned int		  x			  = 0;
	unsigned int		  y			  = 0;

	unsigned char buffer[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char buffer_slice1[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char buffer_slice2[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char buffer_slice3[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char buffer_slice4[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char ref_buffer_slice1[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char ref_buffer_slice2[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char ref_buffer_slice3[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char ref_buffer_slice4[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];

	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Set up shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate shader objects");

	/* Set up program objects */
	m_po_id = gl.createProgram();

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fs_code, m_gs_id, 1 /* part */, &gs_code, m_vs_id, 1 /* part */,
					  &m_boilerplate_vs_code))
	{
		TCU_FAIL("Could not build program object");
	}

	/* Prepare texture data we will use for each slice */
	for (n = 0; n < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++n)
	{
		unsigned char* slice_pixels_ptr[] = { buffer_slice1 + n * N_TEXTURE_COMPONENTS,
											  buffer_slice2 + n * N_TEXTURE_COMPONENTS,
											  buffer_slice3 + n * N_TEXTURE_COMPONENTS,
											  buffer_slice4 + n * N_TEXTURE_COMPONENTS };

		for (n_slice = 0; n_slice < sizeof(slice_pixels_ptr) / sizeof(slice_pixels_ptr[0]); ++n_slice)
		{
			slice_pixels_ptr[n_slice][0] = 0;
			slice_pixels_ptr[n_slice][1] = (unsigned char)(n_slice * 255 / 4);
			slice_pixels_ptr[n_slice][2] = (unsigned char)(n_slice * 255 / 8);
			slice_pixels_ptr[n_slice][3] = (unsigned char)(n_slice * 255 / 12);
		} /* for (all slices) */
	}	 /* for (all pixels) */

	/* Calculate reference texture data we will later use when verifying the rendered data */
	for (n = 0; n < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++n)
	{
		unsigned char* ref_slice_pixels_ptr[] = { ref_buffer_slice1 + n * N_TEXTURE_COMPONENTS,
												  ref_buffer_slice2 + n * N_TEXTURE_COMPONENTS,
												  ref_buffer_slice3 + n * N_TEXTURE_COMPONENTS,
												  ref_buffer_slice4 + n * N_TEXTURE_COMPONENTS };

		unsigned char* slice_pixels_ptr[] = { buffer_slice1 + n * N_TEXTURE_COMPONENTS,
											  buffer_slice2 + n * N_TEXTURE_COMPONENTS,
											  buffer_slice3 + n * N_TEXTURE_COMPONENTS,
											  buffer_slice4 + n * N_TEXTURE_COMPONENTS };

		for (n_slice = 0; n_slice < sizeof(slice_pixels_ptr) / sizeof(slice_pixels_ptr[0]); ++n_slice)
		{
			unsigned char* ref_slice_ptr = ref_slice_pixels_ptr[n_slice];
			unsigned char* slice_ptr	 = slice_pixels_ptr[n_slice];
			float		   slice_rgba[]  = {
				float(slice_ptr[0]) / 255.0f, /* convert to FP representation */
				float(slice_ptr[1]) / 255.0f, /* convert to FP representation */
				float(slice_ptr[2]) / 255.0f, /* convert to FP representation */
				float(slice_ptr[3]) / 255.0f  /* convert to FP representation */
			};

			for (n_component = 0; n_component < N_TEXTURE_COMPONENTS; ++n_component)
			{
				float temp_component = slice_rgba[n_component] /* dst_color */ * slice_rgba[n_component] /* dst_color */
									   + 0.8f /* 1-src_color */ * 0.2f /* src_color */;

				/* Clamp if necessary */
				if (temp_component < 0)
				{
					temp_component = 0.0f;
				}
				else if (temp_component > 1)
				{
					temp_component = 1.0f;
				}

				/* Convert back to GL_RGBA8 */
				ref_slice_ptr[n_component] = (unsigned char)(temp_component * 255.0f);
			} /* for (all components) */
		}	 /* for (all slices) */
	}		  /* for (all pixels) */

	/* Set up texture object used for the test */
	gl.genTextures(1, &m_to_id);
	gl.bindTexture(GL_TEXTURE_3D, m_to_id);
	gl.texStorage3D(GL_TEXTURE_3D, 1 /* levels */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* zoffset */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, buffer_slice1);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 1 /* zoffset */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, buffer_slice2);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 2 /* zoffset */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, buffer_slice3);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 3 /* zoffset */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, buffer_slice4);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture object");

	/* Set up framebuffer object used for the test */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);

	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_id, 0 /* level */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up draw framebuffer");

	/* Generate and bind a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up vertex array object");

	/* Set up blending */
	gl.blendFunc(GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR);
	gl.enable(GL_BLEND);

	/* Render */
	gl.useProgram(m_po_id);
	gl.viewport(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT);

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Draw call failed");

	/* Verify rendered data in the layers */
	gl.genFramebuffers(1, &m_read_fbo_id);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_read_fbo_id);

	for (n_layer = 0; n_layer < TEXTURE_DEPTH; ++n_layer)
	{
		bool has_layer_failed = false;

		const unsigned char* ref_buffer =
			(n_layer == 0) ?
				ref_buffer_slice1 :
				(n_layer == 1) ? ref_buffer_slice2 : (n_layer == 2) ? ref_buffer_slice3 : ref_buffer_slice4;

		gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_id, 0 /* level */, n_layer);
		gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read back pixel data!");

		for (y = 0; y < TEXTURE_HEIGHT; ++y)
		{
			const unsigned int   pixel_size = N_TEXTURE_COMPONENTS;
			const unsigned char* ref_row	= ref_buffer + y * pixel_size;
			const unsigned char* row		= buffer + y * pixel_size;

			for (x = 0; x < TEXTURE_WIDTH; ++x)
			{
#define EPSILON (1)

				const unsigned char* data	 = row + x * pixel_size;
				const unsigned char* ref_data = ref_row + x * pixel_size;

				if (de::abs((int)data[0] - (int)ref_data[0]) > EPSILON ||
					de::abs((int)data[1] - (int)ref_data[1]) > EPSILON ||
					de::abs((int)data[2] - (int)ref_data[2]) > EPSILON ||
					de::abs((int)data[3] - (int)ref_data[3]) > EPSILON)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "(layer=" << n_layer << " x=" << x << " y=" << y
									   << ") "
									   << "Reference value is different than the rendered data (epsilon > " << EPSILON
									   << "): "
									   << "(" << (unsigned int)ref_data[0] << ", " << (unsigned int)ref_data[1] << ", "
									   << (unsigned int)ref_data[2] << ", " << (unsigned int)ref_data[3] << ") vs "
									   << "(" << (unsigned int)data[0] << ", " << (unsigned int)data[1] << ", "
									   << (unsigned int)data[2] << ", " << (unsigned int)data[3] << ")."
									   << tcu::TestLog::EndMessage;

					has_layer_failed = true;
				} /* if (regions are different) */

#undef EPSILON
			} /* for (all pixels in a row) */
		}	 /* for (all rows) */

		if (has_layer_failed)
		{
			TCU_FAIL("Pixel data comparison failed");
		}
	} /* for (all layers) */

	/* Done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;

#undef N_TEXTURE_COMPONENTS
#undef TEXTURE_DEPTH
#undef TEXTURE_HEIGHT
#undef TEXTURE_WIDTH
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderLayeredFramebufferClear::GeometryShaderLayeredFramebufferClear(Context&			  context,
																			 const ExtParameters& extParams,
																			 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_char_id(0)
	, m_fbo_int_id(0)
	, m_fbo_uint_id(0)
	, m_read_fbo_id(0)
	, m_to_rgba32i_id(0)
	, m_to_rgba32ui_id(0)
	, m_to_rgba8_id(0)
{
	/* Left blank on purpose */
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderLayeredFramebufferClear::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

/* Test-wide definitions */
#define N_TEXTURE_COMPONENTS (4)
#define TEXTURE_DEPTH (4)
#define TEXTURE_HEIGHT (4)
#define TEXTURE_WIDTH (4)

	/* Type definitions */
	typedef enum {
		/* Always first */
		CLEAR_FIRST = 0,

		/* glClear() */
		CLEAR_PLAIN = CLEAR_FIRST,
		/* glClearBufferfv() */
		CLEAR_BUFFERFV,
		/* glClearBufferiv() */
		CLEAR_BUFFERIV,
		/* glClearBufferuiv() */
		CLEAR_BUFFERUIV,

		/* Always last */
		CLEAR_COUNT
	} _clear_type;

	/* General variables */
	const glw::GLenum fbo_draw_buffer = GL_COLOR_ATTACHMENT0;

	int n		= 0;
	int n_layer = 0;
	int x		= 0;
	int y		= 0;

	unsigned char buffer_char[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	int			  buffer_int[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned int  buffer_uint[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char slice_1_data_char[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	int			  slice_1_data_int[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned int  slice_1_data_uint[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char slice_2_data_char[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	int			  slice_2_data_int[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned int  slice_2_data_uint[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char slice_3_data_char[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	int			  slice_3_data_int[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned int  slice_3_data_uint[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned char slice_4_data_char[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	int			  slice_4_data_int[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
	unsigned int  slice_4_data_uint[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];

	/* Only carry on if geometry shaders are supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Set up slice data */
	memset(slice_1_data_char, 0, sizeof(slice_1_data_char));
	memset(slice_1_data_int, 0, sizeof(slice_1_data_int));
	memset(slice_1_data_uint, 0, sizeof(slice_1_data_uint));
	memset(slice_2_data_char, 0, sizeof(slice_2_data_char));
	memset(slice_2_data_int, 0, sizeof(slice_2_data_int));
	memset(slice_2_data_uint, 0, sizeof(slice_2_data_uint));
	memset(slice_3_data_char, 0, sizeof(slice_3_data_char));
	memset(slice_3_data_int, 0, sizeof(slice_3_data_int));
	memset(slice_3_data_uint, 0, sizeof(slice_3_data_uint));
	memset(slice_4_data_char, 0, sizeof(slice_4_data_char));
	memset(slice_4_data_int, 0, sizeof(slice_4_data_int));
	memset(slice_4_data_uint, 0, sizeof(slice_4_data_uint));

	for (n = 0; n < 4 /* width */ * 4 /* height */; ++n)
	{
		slice_1_data_char[4 * n + 0] = 255;
		slice_1_data_int[4 * n + 0]  = 255;
		slice_1_data_uint[4 * n + 0] = 255;

		slice_2_data_char[4 * n + 1] = 255;
		slice_2_data_int[4 * n + 1]  = 255;
		slice_2_data_uint[4 * n + 1] = 255;

		slice_3_data_char[4 * n + 2] = 255;
		slice_3_data_int[4 * n + 2]  = 255;
		slice_3_data_uint[4 * n + 2] = 255;

		slice_4_data_char[4 * n + 0] = 255;
		slice_4_data_char[4 * n + 1] = 255;
		slice_4_data_int[4 * n + 0]  = 255;
		slice_4_data_int[4 * n + 1]  = 255;
		slice_4_data_uint[4 * n + 0] = 255;
		slice_4_data_uint[4 * n + 1] = 255;
	} /* for (all pixels) */

	/* Set up texture objects */
	gl.genTextures(1, &m_to_rgba8_id);
	gl.genTextures(1, &m_to_rgba32i_id);
	gl.genTextures(1, &m_to_rgba32ui_id);

	for (n = 0; n < 3 /* textures */; ++n)
	{
		void* to_data_1 =
			(n == 0) ? (void*)slice_1_data_char : (n == 1) ? (void*)slice_1_data_int : (void*)slice_1_data_uint;

		void* to_data_2 =
			(n == 0) ? (void*)slice_2_data_char : (n == 1) ? (void*)slice_2_data_int : (void*)slice_2_data_uint;

		void* to_data_3 =
			(n == 0) ? (void*)slice_3_data_char : (n == 1) ? (void*)slice_3_data_int : (void*)slice_3_data_uint;

		void* to_data_4 =
			(n == 0) ? (void*)slice_4_data_char : (n == 1) ? (void*)slice_4_data_int : (void*)slice_4_data_uint;

		glw::GLenum to_format = (n == 0) ? GL_RGBA : (n == 1) ? GL_RGBA_INTEGER : GL_RGBA_INTEGER;

		glw::GLenum to_internalformat = (n == 0) ? GL_RGBA8 : (n == 1) ? GL_RGBA32I : GL_RGBA32UI;

		glw::GLuint to_id = (n == 0) ? m_to_rgba8_id : (n == 1) ? m_to_rgba32i_id : m_to_rgba32ui_id;

		glw::GLenum to_type = (n == 0) ? GL_UNSIGNED_BYTE : (n == 1) ? GL_INT : GL_UNSIGNED_INT;

		gl.bindTexture(GL_TEXTURE_3D, to_id);

		gl.texStorage3D(GL_TEXTURE_3D, 1 /* levels */, to_internalformat, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH);

		gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* zoffset */, TEXTURE_WIDTH,
						 TEXTURE_HEIGHT, 1 /* depth */, to_format, to_type, to_data_1);
		gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 1 /* zoffset */, TEXTURE_WIDTH,
						 TEXTURE_HEIGHT, 1 /* depth */, to_format, to_type, to_data_2);
		gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 2 /* zoffset */, TEXTURE_WIDTH,
						 TEXTURE_HEIGHT, 1 /* depth */, to_format, to_type, to_data_3);
		gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 3 /* zoffset */, TEXTURE_WIDTH,
						 TEXTURE_HEIGHT, 1 /* depth */, to_format, to_type, to_data_4);

		gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
		gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
		gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not initialize texture object");
	} /* for (all texture objects) */

	/* Set up framebuffer object */
	gl.genFramebuffers(1, &m_fbo_char_id);
	gl.genFramebuffers(1, &m_fbo_int_id);
	gl.genFramebuffers(1, &m_fbo_uint_id);
	gl.genFramebuffers(1, &m_read_fbo_id);

	for (n = 0; n < 3 /* framebuffers */; ++n)
	{
		glw::GLuint fbo_id = (n == 0) ? m_fbo_char_id : (n == 1) ? m_fbo_int_id : m_fbo_uint_id;

		glw::GLuint to_id = (n == 0) ? m_to_rgba8_id : (n == 1) ? m_to_rgba32i_id : m_to_rgba32ui_id;

		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);
		gl.drawBuffers(1, &fbo_draw_buffer);
		gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_id, 0 /* level */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not initialize framebuffer object");
	} /* for (all framebuffers) */

	/* Try reading from the layered framebuffer. */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_char_id);
	gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer_char);

	/* Is the returned data an exact copy of what we've uploaded for layer zero? */
	if (memcmp(buffer_char, slice_1_data_char, sizeof(slice_1_data_char)) != 0)
	{
		TCU_FAIL("Retrieved data is different from data uploaded for layer 0 of a layered framebuffer.");
	}

	/* Iterate through all clear calls supported */

	for (int current_test = static_cast<int>(CLEAR_FIRST); current_test < static_cast<int>(CLEAR_COUNT); current_test++)
	{
		void*				buffer				= NULL;
		const unsigned char clear_color_byte[]  = { 64, 128, 255, 32 };
		const float			clear_color_float[] = { 0.25f, 0.5f, 1.0f, 32.0f / 255.0f };
		const int			clear_color_int[]   = { 64, 128, 255, 32 };
		glw::GLuint			fbo_id				= 0;
		int					pixel_size			= 0;
		void*				slice_1_data		= NULL;
		void*				slice_2_data		= NULL;
		void*				slice_3_data		= NULL;
		void*				slice_4_data		= NULL;
		glw::GLenum			to_format			= GL_NONE;
		glw::GLuint			to_id				= 0;
		glw::GLenum			to_type				= GL_NONE;

		switch (static_cast<_clear_type>(current_test))
		{
		case CLEAR_BUFFERFV:
		case CLEAR_PLAIN:
		{
			buffer		 = buffer_char;
			fbo_id		 = m_fbo_char_id;
			pixel_size   = N_TEXTURE_COMPONENTS;
			slice_1_data = slice_1_data_char;
			slice_2_data = slice_2_data_char;
			slice_3_data = slice_3_data_char;
			slice_4_data = slice_4_data_char;
			to_format	= GL_RGBA;
			to_id		 = m_to_rgba8_id;
			to_type		 = GL_UNSIGNED_BYTE;

			break;
		}

		case CLEAR_BUFFERIV:
		{
			buffer		 = (void*)buffer_int;
			fbo_id		 = m_fbo_int_id;
			pixel_size   = N_TEXTURE_COMPONENTS * sizeof(int);
			slice_1_data = slice_1_data_int;
			slice_2_data = slice_2_data_int;
			slice_3_data = slice_3_data_int;
			slice_4_data = slice_4_data_int;
			to_format	= GL_RGBA_INTEGER;
			to_id		 = m_to_rgba32i_id;
			to_type		 = GL_INT;

			break;
		}

		case CLEAR_BUFFERUIV:
		{
			buffer		 = (void*)buffer_uint;
			fbo_id		 = m_fbo_uint_id;
			pixel_size   = N_TEXTURE_COMPONENTS * sizeof(unsigned int);
			slice_1_data = slice_1_data_uint;
			slice_2_data = slice_2_data_uint;
			slice_3_data = slice_3_data_uint;
			slice_4_data = slice_4_data_uint;
			to_format	= GL_RGBA_INTEGER;
			to_id		 = m_to_rgba32ui_id;
			to_type		 = GL_UNSIGNED_INT;

			break;
		}

		default:
		{
			/* This location should never be reached */
			TCU_FAIL("Execution flow failure");
		}
		} /* switch (current_test)*/

		/* Restore layer data just in case */
		gl.bindTexture(GL_TEXTURE_3D, to_id);
		gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* zoffset */, TEXTURE_WIDTH,
						 TEXTURE_HEIGHT, 1 /* depth */, to_format, to_type, slice_1_data);
		gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 1 /* zoffset */, TEXTURE_WIDTH,
						 TEXTURE_HEIGHT, 1 /* depth */, to_format, to_type, slice_2_data);
		gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 2 /* zoffset */, TEXTURE_WIDTH,
						 TEXTURE_HEIGHT, 1 /* depth */, to_format, to_type, slice_3_data);
		gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 3 /* zoffset */, TEXTURE_WIDTH,
						 TEXTURE_HEIGHT, 1 /* depth */, to_format, to_type, slice_4_data);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage3D() call failed.");

		/* Issue requested clear call */
		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo_id);

		switch (current_test)
		{
		case CLEAR_PLAIN:
		{
			gl.clearColor(clear_color_float[0], clear_color_float[1], clear_color_float[2], clear_color_float[3]);
			gl.clear(GL_COLOR_BUFFER_BIT);

			break;
		}

		case CLEAR_BUFFERIV:
		{
			gl.clearBufferiv(GL_COLOR, 0 /* draw buffer index */, clear_color_int);

			break;
		}

		case CLEAR_BUFFERUIV:
		{
			gl.clearBufferuiv(GL_COLOR, 0 /* draw buffer index */, (const glw::GLuint*)clear_color_int);

			break;
		}

		case CLEAR_BUFFERFV:
		{
			gl.clearBufferfv(GL_COLOR, 0 /* draw buffer index */, clear_color_float);

			break;
		}

		default:
		{
			/* This location should never be reached */
			TCU_FAIL("Execution flow failure");
		}
		} /* switch (current_test) */

		/* Make sure no error was generated */
		GLU_EXPECT_NO_ERROR(gl.getError(), "Clear call failed.");

		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_read_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create read framebuffer object!");

		/* Check the layer data after a clear call */
		for (n_layer = 0; n_layer < TEXTURE_DEPTH; ++n_layer)
		{
			gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_id, 0 /* level */, n_layer);
			gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, to_format, to_type, buffer);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

			/* If we requested an integer clear, the pixels we obtained should be reset to specific values.
			 * If we asked for a FP-based clear, consider an epsilon. */
			for (y = 0; y < TEXTURE_HEIGHT; ++y)
			{
				const int	  row_size = TEXTURE_WIDTH * pixel_size;
				unsigned char* row		= (unsigned char*)buffer + y * row_size;

				for (x = 0; x < TEXTURE_WIDTH; ++x)
				{
					if (current_test == CLEAR_BUFFERIV || current_test == CLEAR_BUFFERUIV)
					{
						unsigned int* pixel = (unsigned int*)(row + x * pixel_size);

						if (memcmp(pixel, clear_color_int, sizeof(clear_color_int)) != 0)
						{
							/* Test fails at this point */
							m_testCtx.getLog()
								<< tcu::TestLog::Message << "(x=" << x << " y=" << y << ") Reference pixel ["
								<< clear_color_byte[0] << ", " << clear_color_byte[1] << ", " << clear_color_byte[2]
								<< ", " << clear_color_byte[3] << "] is different from the one retrieved ["
								<< (int)pixel[0] << ", " << (int)pixel[1] << ", " << (int)pixel[2] << ", "
								<< (int)pixel[3] << "]" << tcu::TestLog::EndMessage;

							TCU_FAIL("Data comparison failure");
						} /* if (memcmp(pixel, clear_color_byte, sizeof(clear_color_byte) ) != 0) */
					}	 /* if (current_test == CLEAR_BUFFERIV || current_test == CLEAR_BUFFERUIV) */
					else
					{
#define EPSILON (1)

						unsigned char* pixel = (unsigned char*)(row + x * pixel_size);

						if (de::abs((int)pixel[0] - clear_color_int[0]) > EPSILON ||
							de::abs((int)pixel[1] - clear_color_int[1]) > EPSILON ||
							de::abs((int)pixel[2] - clear_color_int[2]) > EPSILON ||
							de::abs((int)pixel[3] - clear_color_int[3]) > EPSILON)
						{
							/* Test fails at this point */
							m_testCtx.getLog()
								<< tcu::TestLog::Message << "(x=" << x << " y=" << y << ") Reference pixel ["
								<< clear_color_byte[0] << ", " << clear_color_byte[1] << ", " << clear_color_byte[2]
								<< ", " << clear_color_byte[3] << "] is different from the one retrieved ["
								<< (int)pixel[0] << ", " << (int)pixel[1] << ", " << (int)pixel[2] << ", "
								<< (int)pixel[3] << "]" << tcu::TestLog::EndMessage;

							TCU_FAIL("Data comparison failure");
						} /* if (pixel component has exceeded an allowed epsilon) */

#undef EPSILON
					}
				} /* for (all pixels) */
			}	 /* for (all rows) */
		}		  /* for (all layers) */
	}			  /* for (all clear call types) */

	/* Done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;

#undef TEXTURE_DEPTH
#undef TEXTURE_HEIGHT
#undef TEXTURE_WIDTH
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderLayeredFramebufferClear::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_char_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_char_id);
	}

	if (m_fbo_int_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_int_id);
	}

	if (m_fbo_uint_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_uint_id);
	}

	if (m_to_rgba32i_id != 0)
	{
		gl.deleteTextures(1, &m_to_rgba32i_id);
	}

	if (m_to_rgba32ui_id != 0)
	{
		gl.deleteTextures(1, &m_to_rgba32ui_id);
	}

	if (m_to_rgba8_id != 0)
	{
		gl.deleteTextures(1, &m_to_rgba8_id);
	}

	if (m_read_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_read_fbo_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderLayeredFramebufferDepth::GeometryShaderLayeredFramebufferDepth(Context&			  context,
																			 const ExtParameters& extParams,
																			 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_read_fbo_id(0)
	, m_to_a_id(0)
	, m_to_b_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderLayeredFramebufferDepth::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean up */
	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
	}

	if (m_read_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_read_fbo_id);
	}

	if (m_to_a_id != 0)
	{
		gl.deleteTextures(1, &m_to_a_id);
	}

	if (m_to_b_id != 0)
	{
		gl.deleteTextures(1, &m_to_b_id);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderLayeredFramebufferDepth::iterate(void)
{
	/* General variables */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	unsigned int		  n  = 0;
	unsigned int		  x  = 0;
	unsigned int		  y  = 0;

/* Test-wide definitions */
#define N_TEXTURE_COMPONENTS (4)
#define TEXTURE_DEPTH (4)
#define TEXTURE_HEIGHT (4)
#define TEXTURE_WIDTH (4)

	/* Fragment shader code */
	const char* fs_code = "${VERSION}\n"
						  "\n"
						  "precision highp float;\n"
						  "\n"
						  "out vec4 result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    result = vec4(1.0);\n"
						  "}\n";

	const char* gs_code = "${VERSION}\n"
						  "${GEOMETRY_SHADER_REQUIRE}\n"
						  "\n"
						  "layout(points)                          in;\n"
						  "layout(triangle_strip, max_vertices=64) out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    for (int n = 0; n < 4; ++n)\n"
						  "    {\n"
						  "        float depth = -1.0 + float(n) * 0.5;\n"
						  "\n"
						  "        gl_Layer    = n;\n"
						  "        gl_Position = vec4(1, 1, depth, 1);\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Layer    = n;\n"
						  "        gl_Position = vec4(1, -1, depth, 1);\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Layer    = n;\n"
						  "        gl_Position = vec4(-1, 1, depth, 1);\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Layer    = n;\n"
						  "        gl_Position = vec4(-1, -1, depth, 1);\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        EndPrimitive();\n"
						  "    }\n"
						  "}\n";

	/* This test should only run if EXT_geometry_shader is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Set up texture objects we will be using for the test */
	gl.genTextures(1, &m_to_a_id);
	gl.genTextures(1, &m_to_b_id);

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_a_id);
	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1 /* levels */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_b_id);
	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1 /* levels */, GL_DEPTH_COMPONENT32F, TEXTURE_WIDTH, TEXTURE_HEIGHT,
					TEXTURE_DEPTH);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not initialize texture objects");

	/* Set up framebuffer object we will use for the test */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.genFramebuffers(1, &m_read_fbo_id);

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);

	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_a_id, 0 /* level */);
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_to_b_id, 0 /* level */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not initialize framebuffer object");

	if (gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Draw framebuffer is incomplete: " << gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER)
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("Draw framebuffer is incomplete.");
	}

	/* Set up shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not initialize shader objects");

	/* Set up program object */
	m_po_id = gl.createProgram();

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fs_code, m_gs_id, 1 /* part */, &gs_code, m_vs_id, 1 /* part */,
					  &m_boilerplate_vs_code))
	{
		TCU_FAIL("Could not build program object");
	}

	/* Set up vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind vertex array object");

	/* Clear the depth attachment before we continue */
	gl.clearColor(0.0f /* red */, 0.0f /* green */, 0.0f /* blue */, 0.0f /* alpha */);
	gl.clearDepthf(0.5f);
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Depth buffer clear operation failed");

	/* Render */
	gl.useProgram(m_po_id);
	gl.viewport(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT);

	gl.enable(GL_DEPTH_TEST);
	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed");

	/* Verify the output */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_read_fbo_id);

	for (n = 0; n < TEXTURE_DEPTH; ++n)
	{
		unsigned char buffer[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS];
		unsigned char ref_color[4];

		/* Determine reference color */
		if (n < 2)
		{
			memset(ref_color, 0xFF, sizeof(ref_color));
		}
		else
		{
			memset(ref_color, 0, sizeof(ref_color));
		}

		/* Read the rendered layer data */
		gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_a_id, 0 /* level */, n);
		gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

		/* Compare all pixels against the reference value */
		for (y = 0; y < TEXTURE_HEIGHT; ++y)
		{
			const unsigned int   pixel_size = N_TEXTURE_COMPONENTS;
			const unsigned int   row_width  = TEXTURE_WIDTH * pixel_size;
			const unsigned char* row_data   = buffer + y * row_width;

			for (x = 0; x < TEXTURE_WIDTH; ++x)
			{
				const unsigned char* result = row_data + x * pixel_size;

				if (memcmp(result, ref_color, sizeof(ref_color)) != 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "(x=" << x << " y=" << y << ") "
									   << "Reference value is different than the rendered data: "
									   << "(" << (unsigned int)ref_color[0] << ", " << (unsigned int)ref_color[1]
									   << ", " << (unsigned int)ref_color[2] << ", " << (unsigned int)ref_color[3]
									   << ") vs "
									   << "(" << (unsigned int)result[0] << ", " << (unsigned int)result[1] << ", "
									   << (unsigned int)result[2] << ", " << (unsigned int)result[3] << ")."
									   << tcu::TestLog::EndMessage;

					TCU_FAIL("Rendered data mismatch");
				} /* if (memcmp(row_data + x * pixel_size, ref_color, sizeof(ref_color) ) != 0)*/
			}	 /* for (all pixels in a row) */
		}		  /* for (all rows) */
	}			  /* for (all layers) */

	/* Done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;

#undef N_TEXTURE_COMPONENTS
#undef TEXTURE_DEPTH
#undef TEXTURE_HEIGHT
#undef TEXTURE_WIDTH
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderLayeredFramebufferStencil::GeometryShaderLayeredFramebufferStencil(Context&			  context,
																				 const ExtParameters& extParams,
																				 const char*		  name,
																				 const char*		  description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_to_a_id(0)
	, m_to_b_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderLayeredFramebufferStencil::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean up */
	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
	}

	if (m_to_a_id != 0)
	{
		gl.deleteTextures(1, &m_to_a_id);
	}

	if (m_to_b_id != 0)
	{
		gl.deleteTextures(1, &m_to_b_id);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderLayeredFramebufferStencil::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

/* Test-wide definitions */
#define N_TEXTURE_COMPONENTS (4)
#define TEXTURE_DEPTH (4)
#define TEXTURE_HEIGHT (4)
#define TEXTURE_WIDTH (8)

	/* Fragment shader code */
	const char* fs_code = "${VERSION}\n"
						  "\n"
						  "precision highp float;\n"
						  "\n"
						  "out vec4 result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    result = vec4(1, 1, 1, 1);\n"
						  "}\n";

	/* Geometry shader code */
	const char* gs_code = "${VERSION}\n"
						  "${GEOMETRY_SHADER_REQUIRE}\n"
						  "\n"
						  "precision highp float;\n"
						  "\n"
						  "layout (points)                            in;\n"
						  "layout (triangle_strip, max_vertices = 16) out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    for (int n = 0; n < 4; ++n)\n"
						  "    {\n"
						  "        gl_Position = vec4(1, -1, 0, 1);\n"
						  "        gl_Layer    = n;\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Position = vec4(1,  1, 0, 1);\n"
						  "        gl_Layer    = n;\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Position = vec4(-1, -1, 0, 1);\n"
						  "        gl_Layer    = n;\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        gl_Position = vec4(-1,  1, 0, 1);\n"
						  "        gl_Layer    = n;\n"
						  "        EmitVertex();\n"
						  "\n"
						  "        EndPrimitive();\n"
						  "    }\n"
						  "\n"
						  "}\n";

	/* General variables */
	int n = 0;

	unsigned char slice_null_data[TEXTURE_DEPTH * TEXTURE_WIDTH * TEXTURE_HEIGHT * 1 /* byte  per texel */] = { 0 };
	unsigned char slice_1_data[TEXTURE_DEPTH * TEXTURE_WIDTH * TEXTURE_HEIGHT * 8 /* bytes per texel */]	= { 0 };
	unsigned char slice_2_data[TEXTURE_DEPTH * TEXTURE_WIDTH * TEXTURE_HEIGHT * 8 /* bytes per texel */]	= { 0 };
	unsigned char slice_3_data[TEXTURE_DEPTH * TEXTURE_WIDTH * TEXTURE_HEIGHT * 8 /* bytes per texel */]	= { 0 };
	unsigned char slice_4_data[TEXTURE_DEPTH * TEXTURE_WIDTH * TEXTURE_HEIGHT * 8 /* bytes per texel */]	= { 0 };

	/* This test should only run if EXT_geometry_shader is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create shader & program objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_po_id = gl.createProgram();
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader or program objects");

	/* Build test program object */
	if (!buildProgram(m_po_id, m_fs_id, 1 /* parts */, &fs_code, m_gs_id, 1 /* parts */, &gs_code, m_vs_id,
					  1 /* parts */, &m_boilerplate_vs_code))
	{
		TCU_FAIL("Could not build test program object");
	}

	/* Generate and bind a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create & bind a vertex array object.");

	/* Configure slice data */
	for (n = 0; n < TEXTURE_DEPTH * TEXTURE_WIDTH * TEXTURE_HEIGHT * 8 /* bytes per texel */; ++n)
	{
		/* We are okay with depth data being filled with glitch, but need the stencil data to be
		 * as per test spec. To keep code simple, we do not differentiate between floating-point and
		 * stencil part.
		 */
		slice_1_data[n] = 2;
		slice_2_data[n] = 1;

		/* Slices 3 and 4 should be filled with 0s */
		slice_3_data[n] = 0;
		slice_4_data[n] = 0;
	} /* for (all pixels making up slices) */

	/* Set up texture objects */
	gl.genTextures(1, &m_to_a_id);
	gl.genTextures(1, &m_to_b_id);

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_a_id);
	gl.texImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH,
				  0 /* border */, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* zoffset */,
					 TEXTURE_WIDTH, TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, slice_null_data);
	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 1 /* zoffset */,
					 TEXTURE_WIDTH, TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, slice_null_data);
	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 2 /* zoffset */,
					 TEXTURE_WIDTH, TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, slice_null_data);
	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 3 /* zoffset */,
					 TEXTURE_WIDTH, TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, slice_null_data);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture object A.");

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_b_id);
	gl.texImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, GL_DEPTH32F_STENCIL8, TEXTURE_WIDTH, TEXTURE_HEIGHT,
				  TEXTURE_DEPTH, 0 /* border */, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL);

	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* zoffset */,
					 TEXTURE_WIDTH, TEXTURE_HEIGHT, 1 /* depth */, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
					 slice_1_data);
	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 1 /* zoffset */,
					 TEXTURE_WIDTH, TEXTURE_HEIGHT, 1 /* depth */, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
					 slice_2_data);
	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 2 /* zoffset */,
					 TEXTURE_WIDTH, TEXTURE_HEIGHT, 1 /* depth */, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
					 slice_3_data);
	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 3 /* zoffset */,
					 TEXTURE_WIDTH, TEXTURE_HEIGHT, 1 /* depth */, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
					 slice_4_data);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture object B.");

	/* Set up depth tests */
	gl.disable(GL_DEPTH_TEST);

	/* Set up stencil tests */
	gl.enable(GL_STENCIL_TEST);
	gl.stencilFunc(GL_LESS, 0, /* reference value */ 0xFFFFFFFF /* mask */);

	/* Set up framebuffer objects */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);

	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_a_id, 0 /* level */);
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_to_b_id, 0 /* level */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up draw framebuffer.");

	/* Issue the draw call. */
	gl.useProgram(m_po_id);
	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Draw call failed.");

	/* Check the results */
	for (n = 0; n < 4 /* layers */; ++n)
	{
		glw::GLenum   fbo_completeness													 = GL_NONE;
		int			  m																	 = 0;
		unsigned char ref_data[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS]	= { 0 };
		unsigned char result_data[TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS] = { 0 };
		int			  x																	 = 0;
		int			  y																	 = 0;

		/* Configure the read framebuffer */
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);

		gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_a_id, 0 /* level */, n);
		gl.framebufferTexture(GL_READ_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 0 /* texture */, 0 /* level */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up read framebuffer.");

		/* Verify read framebuffer is considered complete */
		fbo_completeness = gl.checkFramebufferStatus(GL_READ_FRAMEBUFFER);

		if (fbo_completeness != GL_FRAMEBUFFER_COMPLETE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Read FBO is incomplete: "
							   << "[" << fbo_completeness << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL("Read FBO is incomplete.");
		}

		/* Build reference data */
		for (m = 0; m < TEXTURE_WIDTH * TEXTURE_HEIGHT * N_TEXTURE_COMPONENTS; ++m)
		{
			if (n < 2)
			{
				ref_data[m] = 255;
			}
			else
			{
				ref_data[m] = 0;
			}
		} /* for (all reference pixels) */

		/* Read the rendered data */
		gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, result_data);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed");

		for (y = 0; y < TEXTURE_HEIGHT; ++y)
		{
			int			   pixel_size = N_TEXTURE_COMPONENTS;
			int			   row_size   = pixel_size * TEXTURE_WIDTH;
			unsigned char* ref_row	= ref_data + y * row_size;
			unsigned char* result_row = result_data + y * row_size;

			for (x = 0; x < TEXTURE_WIDTH; ++x)
			{
				if (memcmp(ref_row, result_row, pixel_size) != 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "(x=" << x << " y=" << y << "): Rendered data "
									   << "(" << result_row[0] << ", " << result_row[1] << ", " << result_row[2] << ", "
									   << result_row[3] << ")"
									   << " is different from reference data "
									   << "(" << ref_row[0] << ", " << ref_row[1] << ", " << ref_row[2] << ", "
									   << ref_row[3] << ")" << tcu::TestLog::EndMessage;

					TCU_FAIL("Data comparison failed");
				} /* if (memcmp(ref_row, result_row, pixel_size) != 0) */

				ref_row += pixel_size;
				result_row += pixel_size;
			} /* for (all pixels in a row) */
		}	 /* for (all pixels) */
	}		  /* for (all layers) */

	/* Done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;

#undef N_TEXTURE_COMPONENTS
#undef TEXTURE_DEPTH
#undef TEXTURE_HEIGHT
#undef TEXTURE_WIDTH
}

} // namespace glcts
