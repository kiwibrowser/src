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
 * \file  esextcDrawElementsBaseVertexTests.cpp
 * \brief Implements conformance tests for "draw elements base vertex" functionality
 *        for both ES and GL.
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawElementsBaseVertexTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <string>
#include <vector>

namespace glcts
{
/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
DrawElementsBaseVertexTestBase::DrawElementsBaseVertexTestBase(glcts::Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_is_draw_elements_base_vertex_supported(false)
	, m_is_ext_multi_draw_arrays_supported(false)
	, m_is_geometry_shader_supported(false)
	, m_is_tessellation_shader_supported(false)
	, m_is_vertex_attrib_binding_supported(false)
	, m_bo_id(0)
	, m_bo_id_2(0)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_po_color_attribute_location(-1)
	, m_po_uses_gs_stage(false)
	, m_po_uses_tc_te_stages(false)
	, m_po_uses_vertex_attrib_binding(false)
	, m_po_vertex_attribute_location(-1)
	, m_tc_id(0)
	, m_te_id(0)
	, m_to_base_id(0)
	, m_to_ref_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_bo_negative_data_index_size(-1)
	, m_bo_negative_data_vertex_size(-1)
	, m_draw_call_color_offset(DE_NULL)
	, m_draw_call_index_offset(DE_NULL)
	, m_draw_call_index2_offset(DE_NULL)
	, m_draw_call_index3_offset(DE_NULL)
	, m_draw_call_index4_offset(DE_NULL)
	, m_draw_call_index5_offset(DE_NULL)
	, m_draw_call_vertex_offset(DE_NULL)
	, m_draw_call_vertex2_offset(DE_NULL)
	, m_to_height(128)
	, m_to_width(128)
	, m_to_base_data(DE_NULL)
	, m_to_ref_data(DE_NULL)
{
	static const glw::GLuint functional_index_data[] = /* used for a number of Functional Tests */
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22 };
	static const glw::GLuint functional2_index_data[] = /* used for Functional Test IV */
		{ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		  30, 31, 32, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16 };
	static const glw::GLubyte functional3_index_data[] = /* used for Functional Test IX */
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	static const glw::GLushort functional4_index_data[] = /* used for Functional Test IX */
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	static const glw::GLuint functional5_index_data[] = /* used for Functional Test IX */
		{ 2147483647 + 3u,								/* 2^31 + 2 */
		  2147483647 + 4u,
		  2147483647 + 5u,
		  2147483647 + 6u,
		  2147483647 + 7u,
		  2147483647 + 8u,
		  2147483647 + 9u,
		  2147483647 + 10u,
		  2147483647 + 11u,
		  2147483647 + 12u,
		  2147483647 + 13u,
		  2147483647 + 14u,
		  257, // regular draw call indices for ubyte ref image
		  258,
		  259,
		  260,
		  261,
		  262,
		  263,
		  264,
		  265,
		  266,
		  267,
		  268,
		  65537, // regular draw call indices for ushort ref image
		  65538,
		  65539,
		  65540,
		  65541,
		  65542,
		  65543,
		  65544,
		  65545,
		  65546,
		  65547,
		  65548,
		  0, // regular draw call indices for uint ref image
		  1,
		  2,
		  3,
		  4,
		  5,
		  6,
		  7,
		  8,
		  9,
		  10,
		  11 };
	static const glw::GLfloat functional_color_data[] = /* used for "vertex attrib binding" Functional Test */
		{
		  0.1f, 0.2f, 0.2f, 0.3f, 0.3f, 0.4f, 0.4f, 0.5f, 0.5f, 0.6f, 0.6f, 0.7f, 0.7f, 0.8f, 0.8f, 0.9f, 0.9f,
		  1.0f, 1.0f, 0.9f, 0.9f, 0.8f, 0.8f, 0.7f, 0.7f, 0.6f, 0.6f, 0.5f, 0.5f, 0.4f, 0.4f, 0.3f, 0.3f, 0.2f,
		  0.2f, 0.1f, 0.1f, 0.0f, 0.0f, 0.1f, 0.1f, 0.2f, 0.2f, 0.4f, 0.3f, 0.9f, 0.4f, 0.8f, 0.5f, 1.0f, 0.6f,
		  0.8f, 0.7f, 0.1f, 0.8f, 0.3f, 0.9f, 0.5f, 1.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.3f, 0.1f, 1.0f,
		};
	static const glw::GLfloat functional_vertex_data[] = /* used by a number of Functional Tests */
		{
		  0.0f,  0.0f,  -0.1f, -0.1f, 0.2f,  0.8f,  -0.3f, -0.3f, 0.4f,  -0.4f, 0.5f,  0.5f, -0.6f, 0.6f,
		  0.7f,  -0.7f, -0.8f, 0.8f,  0.9f,  -0.9f, -1.0f, -1.0f, 1.0f,  -1.0f, 0.0f,  1.0f, -0.9f, 0.1f,
		  0.8f,  -0.2f, -0.7f, 0.3f,  -0.6f, -0.4f, 0.5f,  -0.5f, -0.4f, -0.6f, 0.3f,  0.7f, -0.2f, -0.8f,
		  -0.1f, -0.9f, 0.0f,  0.0f,  0.5f,  0.5f,  -0.6f, 0.6f,  0.7f,  -0.7f, -0.8f, 0.8f, 0.9f,  -0.9f,
		  -1.0f, -1.0f, 1.0f,  -1.0f, 0.0f,  1.0f,  -0.9f, 0.1f,  0.8f,  -0.2f,
		};
	static const glw::GLfloat functional2_vertex_data[] = /* used by a number of Functional Tests */
		{
		  -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.3f, 0.3f, 0.4f, 0.4f, 0.5f, 0.5f, 0.6f, 0.6f, 0.7f, 0.7f,
		  0.8f,  0.8f,  0.9f, 0.9f,  0.1f, 0.2f, 0.2f, 0.1f, 0.1f, 0.2f, 0.9f, 0.1f, 0.8f, 0.2f, 0.7f, 0.3f,
		  0.6f,  0.4f,  0.5f, 0.5f,  0.4f, 0.6f, 0.3f, 0.7f, 0.2f, 0.8f, 0.1f, 0.9f, 0.0f, 0.0f,

		};
	static const glw::GLuint negative_index_data[]  = { 0, 1, 2 };
	static const float		 negative_vertex_data[] = { -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 1.0f };

	m_bo_functional_data_color		  = functional_color_data;
	m_bo_functional_data_color_size   = sizeof(functional_color_data);
	m_bo_functional_data_index		  = functional_index_data;
	m_bo_functional_data_index_size   = sizeof(functional_index_data);
	m_bo_functional_data_vertex		  = functional_vertex_data;
	m_bo_functional_data_vertex_size  = sizeof(functional_vertex_data);
	m_bo_functional2_data_index		  = functional2_index_data;
	m_bo_functional2_data_index_size  = sizeof(functional2_index_data);
	m_bo_functional3_data_index		  = functional3_index_data;
	m_bo_functional3_data_index_size  = sizeof(functional3_index_data);
	m_bo_functional4_data_index		  = functional4_index_data;
	m_bo_functional4_data_index_size  = sizeof(functional4_index_data);
	m_bo_functional5_data_index		  = functional5_index_data;
	m_bo_functional5_data_index_size  = sizeof(functional5_index_data);
	m_bo_functional2_data_vertex	  = functional2_vertex_data;
	m_bo_functional2_data_vertex_size = sizeof(functional2_vertex_data);
	m_bo_negative_data_index		  = negative_index_data;
	m_bo_negative_data_index_size	 = sizeof(negative_index_data);
	m_bo_negative_data_vertex		  = negative_vertex_data;
	m_bo_negative_data_vertex_size	= sizeof(negative_vertex_data);
}

/** Creates & initializes a number of shader objects, assigns user-provided
 *  code to relevant shader objects and compiles them. If all shaders are
 *  compiled successfully, they are later attached to a program object, id
 *  of which is stored in m_po_id. Finally, the program object is linked.
 *
 *  If the compilation process or the linking process fails for any reason,
 *  the method throws a TestError exception.
 *
 *  Fragment shader object ID is stored under m_fs_id.
 *  Geometry shader object ID is stored under m_gs_id.
 *  Tessellation control shader object ID is stored under m_tc_id.
 *  Tessellation evaluation shader object ID is stored under m_te_id.
 *  Vertex shader object ID is stored under m_vs_id.
 *
 *  @param fs_code Code to use for the fragment shader. Must not be NULL.
 *  @param gs_code Code to use for the geometry shader. Can be NULL.
 *  @param tc_code Code to use for the tessellation control shader. Can be NULL.
 *  @param te_code Code to use for the tessellation evaluation shader. Can be NULL.
 *  @param vs_code Code to use for the vertex shader. Must not be NULL.
 *
 */
void DrawElementsBaseVertexTestBase::buildProgram(const char* fs_code, const char* vs_code, const char* tc_code,
												  const char* te_code, const char* gs_code)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program & shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	if (tc_code != DE_NULL)
	{
		m_tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
	}

	if (te_code != DE_NULL)
	{
		m_te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	}

	if (gs_code != DE_NULL)
	{
		m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Assign source code to the shader objects */
	gl.shaderSource(m_fs_id, 1,			/* count */
					&fs_code, DE_NULL); /* length */
	gl.shaderSource(m_vs_id, 1,			/* count */
					&vs_code, DE_NULL); /* length */

	if (m_tc_id != 0)
	{
		gl.shaderSource(m_tc_id, 1,			/* count */
						&tc_code, DE_NULL); /* length */
	}

	if (m_te_id != 0)
	{
		gl.shaderSource(m_te_id, 1,			/* count */
						&te_code, DE_NULL); /* length */
	}

	if (m_gs_id != 0)
	{
		gl.shaderSource(m_gs_id, 1,			/* count */
						&gs_code, DE_NULL); /* length */
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");

	/* Try to compile the shaders */
	const glw::GLuint  so_ids[] = { m_fs_id, m_vs_id, m_tc_id, m_te_id, m_gs_id };
	const unsigned int n_so_ids = sizeof(so_ids) / sizeof(so_ids[0]);

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		glw::GLint  compile_status = GL_FALSE;
		glw::GLuint so_id		   = so_ids[n_so_id];

		if (so_id != 0)
		{
			gl.compileShader(so_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

			gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

			if (compile_status == GL_FALSE)
			{
				TCU_FAIL("Shader compilation failed");
			} /* if (compile_status == GL_FALSE) */
		}	 /* if (so_id != 0) */
	}		  /* for (all shader objects) */

	/* Attach the shaders to the program object */
	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_vs_id);

	if (m_tc_id != 0)
	{
		gl.attachShader(m_po_id, m_tc_id);
	}

	if (m_te_id != 0)
	{
		gl.attachShader(m_po_id, m_te_id);
	}

	if (m_gs_id != 0)
	{
		gl.attachShader(m_po_id, m_gs_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

	/* Set up TFO */
	const glw::GLchar* tf_varyings[] = { "gl_Position" };

	gl.transformFeedbackVaryings(m_po_id, 1, /* count */
								 tf_varyings, GL_SEPARATE_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	/* Try to link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status == GL_FALSE)
	{
		TCU_FAIL("Program linking failed.");
	}

	/* Retrieve attribute locations */
	m_po_color_attribute_location =
		gl.getAttribLocation(m_po_id, "in_color"); /* != -1 only for "vertex attrib binding" tests */
	m_po_vertex_attribute_location = gl.getAttribLocation(m_po_id, "vertex");

	DE_ASSERT(m_po_vertex_attribute_location != -1);
}

/** Verifies contents of the base & reference textures. This method can work
 *  in two modes:
 *
 *  a) If @param should_be_equal is true, the method will throw a TestError exception
 *     if the two textures are not a match.
 *  b) If @param should_be_equal is false, the method will throw a TestError exception
 *     if the two extures are a match.
 *
 *  Furthermore, in order to verify that the basevertex & regular draw calls actually
 *  generated at least one sample, the method verifies that at least one texel in both
 *  of the textures has been modified. If all texels in any of the textures are found
 *  to be (0, 0, 0) (alpha channel is ignored), TestError exception will be generated.
 *
 *  @param should_be_equal Please see description for more details.
 **/
void DrawElementsBaseVertexTestBase::compareBaseAndReferenceTextures(bool should_be_equal)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Read contents of both base and reference textures */
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_base_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	gl.readPixels(0, /* x */
				  0, /* y */
				  m_to_width, m_to_height, GL_RGBA, GL_UNSIGNED_BYTE, m_to_base_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_ref_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");
	gl.readPixels(0, /* x */
				  0, /* y */
				  m_to_width, m_to_height, GL_RGBA, GL_UNSIGNED_BYTE, m_to_ref_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

	/* Both tests should be a match */
	const unsigned char* base_data_ptr				   = m_to_base_data;
	bool				 all_base_rgb_texels_zero	  = true;
	bool				 all_reference_rgb_texels_zero = true;
	const unsigned int   n_texels					   = m_to_width * m_to_height;
	const unsigned char* ref_data_ptr				   = m_to_ref_data;
	bool				 textures_identical			   = true;

	for (unsigned int n_texel = 0; n_texel < n_texels; ++n_texel)
	{
		/* Verify something was rendered to one of the render-targets. Note we
		 * omit alpha channel, since the clear color is set to 0xFF by default */
		if (base_data_ptr[0] != 0 || base_data_ptr[1] != 0 || base_data_ptr[2] != 0)
		{
			all_base_rgb_texels_zero = false;
		}

		if (ref_data_ptr[0] != 0 || ref_data_ptr[1] != 0 || ref_data_ptr[2] != 0)
		{
			all_reference_rgb_texels_zero = false;
		}

		if (base_data_ptr[0] != ref_data_ptr[0] || base_data_ptr[1] != ref_data_ptr[1] ||
			base_data_ptr[2] != ref_data_ptr[2] || base_data_ptr[3] != ref_data_ptr[3])
		{
			if (should_be_equal)
			{
				const unsigned int y = n_texel / m_to_width;
				const unsigned int x = n_texel % m_to_width;

				m_testCtx.getLog() << tcu::TestLog::Message << "Pixels at (" << x << ", " << y
								   << ") do not match. Found:"
								   << "(" << (unsigned int)base_data_ptr[0] << ", " << (unsigned int)base_data_ptr[1]
								   << ", " << (unsigned int)base_data_ptr[2] << ", " << (unsigned int)base_data_ptr[3]
								   << "), expected:"
								   << "(" << (unsigned int)ref_data_ptr[0] << ", " << (unsigned int)ref_data_ptr[1]
								   << ", " << (unsigned int)ref_data_ptr[2] << ", " << (unsigned int)ref_data_ptr[3]
								   << ")." << tcu::TestLog::EndMessage;

				TCU_FAIL("Pixel mismatch");
			}
			else
			{
				/* Base and reference textures are *not* identical. */
				textures_identical = false;
			}
		}

		base_data_ptr += 4; /* components */
		ref_data_ptr += 4;  /* components */
	}						/* for (all texels) */

	if (all_base_rgb_texels_zero)
	{
		TCU_FAIL("Draw call used to render contents of the base texture did not change the texture");
	}

	if (all_reference_rgb_texels_zero)
	{
		TCU_FAIL("Draw call used to render contents of the reference texture did not change the texture");
	}

	if (!should_be_equal && textures_identical)
	{
		TCU_FAIL("Textures are a match, even though they should not be identical.");
	}
}

/** Updates m_draw_call_color_offset, m_draw_call_index*_offset and m_draw_call_vertex*_offset
 *  members with valid values, depending on the input arguments.
 *
 *  @param use_clientside_index_data  true if client-side index data buffer is going to be
 *                                    used for the test-issued draw calls, false otherwise.
 *  @param use_clientside_vertex_data true if client-side color & vertex data buffer is going
 *                                    to be used for the test-issued draw calls, false
 *                                    otherwise.
 */
void DrawElementsBaseVertexTestBase::computeVBODataOffsets(bool use_clientside_index_data,
														   bool use_clientside_vertex_data)
{
	if (use_clientside_index_data)
	{
		m_draw_call_index_offset  = m_bo_functional_data_index;
		m_draw_call_index2_offset = m_bo_functional2_data_index;
		m_draw_call_index3_offset = m_bo_functional3_data_index;
		m_draw_call_index4_offset = m_bo_functional4_data_index;
		m_draw_call_index5_offset = m_bo_functional5_data_index;
	}
	else
	{
		/* Note that these assignments correspond to how the buffer object storage is constructed.
		 * If you need to update these offsets, don't forget to update the glBufferSubData() calls
		 */
		m_draw_call_index_offset =
			(const glw::GLuint*)(deUintptr)(m_bo_functional_data_vertex_size + m_bo_functional2_data_vertex_size);
		m_draw_call_index2_offset = (const glw::GLuint*)(deUintptr)(
			m_bo_functional_data_vertex_size + m_bo_functional2_data_vertex_size + m_bo_functional_data_index_size);
		m_draw_call_index3_offset =
			(const glw::GLubyte*)(deUintptr)(m_bo_functional_data_vertex_size + m_bo_functional2_data_vertex_size +
											 m_bo_functional_data_index_size + m_bo_functional2_data_index_size);
		m_draw_call_index4_offset = (const glw::GLushort*)(deUintptr)(
			m_bo_functional_data_vertex_size + m_bo_functional2_data_vertex_size + m_bo_functional_data_index_size +
			m_bo_functional2_data_index_size + m_bo_functional3_data_index_size);
		m_draw_call_index5_offset = (const glw::GLuint*)(deUintptr)(
			m_bo_functional_data_vertex_size + m_bo_functional2_data_vertex_size + m_bo_functional_data_index_size +
			m_bo_functional2_data_index_size + m_bo_functional3_data_index_size + m_bo_functional4_data_index_size);
	}

	if (use_clientside_vertex_data)
	{
		m_draw_call_color_offset   = m_bo_functional_data_color;
		m_draw_call_vertex_offset  = m_bo_functional_data_vertex;
		m_draw_call_vertex2_offset = m_bo_functional2_data_vertex;
	}
	else
	{
		/* Note: same observation as above holds. */
		m_draw_call_color_offset = (const glw::GLfloat*)(deUintptr)(
			m_bo_functional_data_vertex_size + m_bo_functional2_data_vertex_size + m_bo_functional_data_index_size +
			m_bo_functional2_data_index_size + m_bo_functional3_data_index_size + m_bo_functional4_data_index_size +
			m_bo_functional5_data_index_size);
		m_draw_call_vertex_offset  = DE_NULL;
		m_draw_call_vertex2_offset = (const glw::GLfloat*)(deUintptr)m_bo_functional_data_vertex_size;
	}
}

void DrawElementsBaseVertexTestBase::deinit()
{
	/* TCU_FAIL will skip the per test object de-initialization, we need to
	 * take care of it here
	 */
	deinitPerTestObjects();
}

/** Deinitializes all ES objects created by the test. */
void DrawElementsBaseVertexTestBase::deinitPerTestObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	deinitProgramAndShaderObjects();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_bo_id_2 != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_2);

		m_bo_id_2 = 0;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_to_base_data != DE_NULL)
	{
		delete[] m_to_base_data;

		m_to_base_data = DE_NULL;
	}

	if (m_to_base_id != 0)
	{
		gl.deleteTextures(1, &m_to_base_id);

		m_to_base_id = 0;
	}

	if (m_to_ref_data != DE_NULL)
	{
		delete[] m_to_ref_data;

		m_to_ref_data = DE_NULL;
	}

	if (m_to_ref_id != 0)
	{
		gl.deleteTextures(1, &m_to_ref_id);

		m_to_ref_id = 0;
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

	/* Restore the default GL_PACK_ALIGNMENT setting */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
}

/** Deinitializes all program & shader objects that may have been initialized
 *  by the test.
 */
void DrawElementsBaseVertexTestBase::deinitProgramAndShaderObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

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
}

/** Executes all test cases stored in m_test_cases.
 *
 *  This method throws a TestError exception upon test failure.
 **/
void DrawElementsBaseVertexTestBase::executeTestCases()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Iterate over all test cases */
	for (_test_cases_const_iterator test_cases_iterator = m_test_cases.begin();
		 test_cases_iterator != m_test_cases.end(); ++test_cases_iterator)
	{
		const _test_case& test_case = *test_cases_iterator;

		/* What is the size of a single index value? */
		unsigned int index_size = 0;

		switch (test_case.index_type)
		{
		case GL_UNSIGNED_BYTE:
			index_size = 1;
			break;
		case GL_UNSIGNED_SHORT:
			index_size = 2;
			break;
		case GL_UNSIGNED_INT:
			index_size = 4;
			break;

		default:
		{
			TCU_FAIL("Unrecognized index type");
		}
		} /* switch (test_case.index_type) */

		/* Set up the work environment */
		setUpFunctionalTestObjects(test_case.use_clientside_vertex_data, test_case.use_clientside_index_data,
								   test_case.use_tessellation_shader_stage, test_case.use_geometry_shader_stage,
								   test_case.use_vertex_attrib_binding, test_case.use_overflow_test_vertices);

		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_base_id, 0); /* level */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed");

		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

		switch (test_case.function_type)
		{
		case FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX:
		{
			gl.drawElementsBaseVertex(test_case.primitive_mode, 3, /* count */
									  test_case.index_type, test_case.index_offset, test_case.basevertex);

			break;
		}

		case FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX:
		{
			gl.drawElementsInstancedBaseVertex(test_case.primitive_mode, 3,						/* count */
											   test_case.index_type, test_case.index_offset, 3, /* instancecount */
											   test_case.basevertex);

			break;
		}

		case FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX:
		{
			gl.drawRangeElementsBaseVertex(test_case.primitive_mode, test_case.range_start, test_case.range_end,
										   3, /* count */
										   test_case.index_type, test_case.index_offset, test_case.basevertex);

			break;
		}

		case FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX:
		{
			const glw::GLint basevertex_array[] = { test_case.basevertex, test_case.basevertex, test_case.basevertex };

			gl.multiDrawElementsBaseVertex(
				test_case.primitive_mode, test_case.multi_draw_call_count_array, test_case.index_type,
				(const glw::GLvoid**)test_case.multi_draw_call_indices_array, 3, /* primcount */
				basevertex_array);

			break;
		}

		default:
		{
			TCU_FAIL("Unsupported function index");
		}
		} /* switch (n_function) */

		if (gl.getError() != GL_NO_ERROR)
		{
			std::stringstream error_sstream;

			error_sstream << getFunctionName(test_case.function_type) << " call failed";
			TCU_FAIL(error_sstream.str().c_str());
		}

		/* Now, render the same triangle using glDrawElements() to the reference texture */
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_ref_id, 0); /* level */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed");

		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

		glw::GLenum regular_draw_call_index_type = test_case.index_type;
		if (test_case.regular_draw_call_index_type != 0)
		{
			/* Need to use a different index type for regular draw call */
			regular_draw_call_index_type = test_case.regular_draw_call_index_type;

			switch (test_case.regular_draw_call_index_type)
			{
			case GL_UNSIGNED_BYTE:
				index_size = 1;
				break;
			case GL_UNSIGNED_SHORT:
				index_size = 2;
				break;
			case GL_UNSIGNED_INT:
				index_size = 4;
				break;

			default:
			{
				TCU_FAIL("Unrecognized index type");
			}
			}
		}

		glw::GLubyte* regular_draw_call_offset =
			(glw::GLubyte*)test_case.index_offset + index_size * test_case.regular_draw_call_offset;
		if (test_case.use_overflow_test_vertices)
		{
			/* Special case for overflow test */
			regular_draw_call_offset = (glw::GLubyte*)test_case.regular_draw_call_offset2;
		}

		switch (test_case.function_type)
		{
		case FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX: /* pass-through */
		case FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX:
		{
			gl.drawElements(test_case.primitive_mode, 3,							 /* count */
							regular_draw_call_index_type, regular_draw_call_offset); /* as per test spec */
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElements() call failed");

			break;
		}

		case FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX:
		{
			gl.drawElementsInstanced(test_case.primitive_mode, 3,								 /* count */
									 regular_draw_call_index_type, regular_draw_call_offset, 3); /* instancecount */
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsInstanced() call failed");

			break;
		}

		case FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX:
		{
			/* Normally we'd just use glMultiDrawElements() here but it's not a part
			 * of ES3.1 core spec and we're trying to avoid any dependencies in the test.
			 * No damage done under GL, either.
			 */
			for (unsigned int n_draw_call = 0; n_draw_call < 3; /* drawcount */
				 ++n_draw_call)
			{
				gl.drawElements(test_case.primitive_mode, test_case.multi_draw_call_count_array[n_draw_call],
								regular_draw_call_index_type,
								(const glw::GLvoid*)test_case.regular_multi_draw_call_offseted_array[n_draw_call]);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElements() call failed.");
			}

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized function index");
		}
		} /* switch (n_function) */

		/* Compare the two textures and make sure they are either a match or not,
		 * depending on whether basevertex values match the offsets used for regular
		 * draw calls.
		 */
		compareBaseAndReferenceTextures(test_case.should_base_texture_match_reference_texture);

		/* ES Resources are allocated per test objects, we need to release them here */
		deinitPerTestObjects();
	} /* for (all test cases) */
}

/* Returns name of the function represented by _function_type.
 *
 * @param function_type Function type to use for the query.
 *
 * @return As per description, or "[?]" (without the quotation marks)
 *         if @param function_type was not recognized.
 */
std::string DrawElementsBaseVertexTestBase::getFunctionName(_function_type function_type)
{
	std::string result = "[?]";

	switch (function_type)
	{
	case FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX:
	{
		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			result = "glDrawElementsBaseVertexEXT()";

			break;
		}
		else
		{
			result = "glDrawElementsBaseVertex()";

			break;
		}

		break;
	}

	case FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX:
	{
		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			result = "glDrawElementsInstancedBaseVertexEXT()";

			break;
		}
		else
		{
			result = "glDrawElementsInstancedBaseVertex()";

			break;
		}

		break;
	}

	case FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX:
	{
		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			result = "glDrawRangeElementsBaseVertexEXT()";

			break;
		}
		else
		{
			result = "glDrawRangeElementsBaseVertex()";

			break;
		}

		break;
	}

	case FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX:
	{
		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			result = "glMultiDrawElementsBaseVertexEXT()";

			break;
		}
		else
		{
			result = "glMultiDrawElementsBaseVertex()";

			break;
		}

		break;
	}

	default:
	{
		TCU_FAIL("Unknown function type");

		break;
	}
	} /* switch (function_type) */

	return result;
}

/** Initializes extension-specific function pointers and caches information about
 *  extension availability.
 *
 *  Function pointers for the following extensions are retrieved & stored:
 *
 *  - GL_EXT_draw_elements_base_vertex (ES) or GL_ARB_draw_elements_base_vertex (GL)
 *  - GL_EXT_multi_draw_arrays         (ES & GL)
 *
 *  Availability of the following extensions is checked & cached:
 *
 *  - GL_EXT_draw_elements_base_vertex (ES) or GL_ARB_draw_elements_base_vertex (GL)
 *  - GL_EXT_geometry_shader           (ES) or GL_ARB_geometry_shader4          (GL)
 *  - GL_EXT_multi_draw_arrays         (ES & GL)
 *  - GL_EXT_tessellation_shader       (ES) or GL_ARB_tessellation_shader (GL)
 */
void DrawElementsBaseVertexTestBase::init()
{
	TestCaseBase::init();

	const glu::ContextInfo& context_info = m_context.getContextInfo();

	if ((glu::isContextTypeES(m_context.getRenderContext().getType()) &&
		 (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) ||
		  context_info.isExtensionSupported("GL_EXT_draw_elements_base_vertex"))) ||
		context_info.isExtensionSupported("GL_ARB_draw_elements_base_vertex"))
	{
#if defined(DE_DEBUG) && !defined(DE_COVERAGE_BUILD)
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
#endif

		m_is_draw_elements_base_vertex_supported = true;

		DE_ASSERT(gl.drawElementsBaseVertex != NULL);
		DE_ASSERT(gl.drawElementsInstancedBaseVertex != NULL);
		DE_ASSERT(gl.drawRangeElementsBaseVertex != NULL);

		/* NOTE: glMultiDrawElementsBaseVertex() is a part of the draw_elements_base_vertex extension in GL,
		 *       but requires a separate extension under ES.
		 */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()) ||
			context_info.isExtensionSupported("GL_EXT_multi_draw_arrays"))
		{
			m_is_ext_multi_draw_arrays_supported = true;

			DE_ASSERT(gl.multiDrawElementsBaseVertex != NULL);
		} /* if (GL_EXT_multi_draw_arrays is supported or GL context is being tested) */

		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			/* This conformance test needs to be adjusted in order to run under < ES3.1 contexts */
			DE_ASSERT(glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 1)));
		}
		else
		{
			m_is_vertex_attrib_binding_supported = context_info.isExtensionSupported("GL_ARB_vertex_attrib_binding");
		}
	} /* if (GL_{ARB, EXT}_draw_elements_base_vertex is supported) */

	if ((glu::isContextTypeES(m_context.getRenderContext().getType()) &&
		 (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) ||
		  context_info.isExtensionSupported("GL_EXT_geometry_shader"))) ||
		context_info.isExtensionSupported("GL_ARB_geometry_shader4"))
	{
		m_is_geometry_shader_supported = true;
	}

	if ((glu::isContextTypeES(m_context.getRenderContext().getType()) &&
		 (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) ||
		  context_info.isExtensionSupported("GL_EXT_tessellation_shader"))) ||
		context_info.isExtensionSupported("GL_ARB_tessellation_shader"))
	{
		m_is_tessellation_shader_supported = true;
	}

	if (!m_is_draw_elements_base_vertex_supported)
	{
		throw tcu::NotSupportedError("draw_elements_base_vertex functionality is not supported");
	}
}

/** Sets up all ES objects required to run a single functional test case iteration.
 *
 *  @param use_clientside_vertex_data    true if the test case requires client-side buffers to
 *                                       back the color/vertex vertex attribute arrays; false
 *                                       to use buffer object storage.
 *  @param use_clientside_index_data     true if the test case requires client-side buffers to
 *                                       be used as index data source; false to use buffer object
 *                                       storage.
 *  @param use_tessellation_shader_stage true if the program object used for the test should include
 *                                       tessellation control & evaluation shader stages; false to
 *                                       not to include these shader stages in the pipeline.
 *  @param use_geometry_shader_stage     true if the program object used for the test should include
 *                                       geometry shader stage; false to not to include the shader
 *                                       stage in the pipeline.
 *  @param use_vertex_attrib_binding     true if vertex attribute bindings should be used for
 *                                       vertex array object configuration. This also modifies the
 *                                       vertex shader, so that instead of calculating vertex color
 *                                       on a per-vertex basis, contents of the "color" input
 *                                       will be used as a data source for the color data.
 *                                       false to use a vertex attribute array configured with
 *                                       a casual glVertexAttribPointer() call.
 * @param use_overflow_test_vertices     true if using an especially large vertex array to test
 *                                       overflow behavior.
 *
 *  This method can throw an exception if any of the ES calls fail.
 **/
void DrawElementsBaseVertexTestBase::setUpFunctionalTestObjects(
	bool use_clientside_vertex_data, bool use_clientside_index_data, bool use_tessellation_shader_stage,
	bool use_geometry_shader_stage, bool use_vertex_attrib_binding, bool use_overflow_test_vertices)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up a texture object that we will use as a color attachment */
	gl.genTextures(1, &m_to_base_id);
	gl.genTextures(1, &m_to_ref_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed.");

	const glw::GLuint  to_ids[] = { m_to_base_id, m_to_ref_id };
	const unsigned int n_to_ids = sizeof(to_ids) / sizeof(to_ids[0]);

	for (unsigned int n_to_id = 0; n_to_id < n_to_ids; ++n_to_id)
	{
		gl.bindTexture(GL_TEXTURE_2D, to_ids[n_to_id]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
						GL_RGBA8, m_to_width, m_to_height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");
	}

	/* Also set up some buffers we will use for data comparison */
	m_to_base_data = new unsigned char[m_to_width * m_to_height * 4 /* components */];
	m_to_ref_data  = new unsigned char[m_to_width * m_to_height * 4 /* components */];

	/* Finally, for glReadPixels() operations, we need to make sure that pack alignment is set to 1 */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call failed.");

	/* Proceed with framebuffer object initialization. Since we will be rendering to different
	 * render-targets, there's not much point in attaching any of the textures at this point.
	 */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebufer() call failed");

	gl.viewport(0, /* x */
				0, /* y */
				m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed");

	/* Set up buffer object we will use for the draw calls. Use the data
	 * from the test specification.
	 *
	 * NOTE: If you need to change the data layout here, make sure you also update
	 *       m_draw_call_color_offset, m_draw_call_index*_offset, and
	 *       m_draw_call_vertex*_offset setter calls elsewhere.
	 **/
	if (m_bo_id == 0)
	{
		unsigned int current_offset = 0;

		gl.genBuffers(1, &m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

		gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

		gl.bufferData(GL_ARRAY_BUFFER,
					  m_bo_functional_data_index_size + m_bo_functional_data_vertex_size +
						  m_bo_functional2_data_vertex_size + m_bo_functional2_data_index_size +
						  m_bo_functional3_data_index_size + m_bo_functional4_data_index_size +
						  m_bo_functional5_data_index_size + m_bo_functional_data_color_size,
					  DE_NULL, /* data */
					  GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

		gl.bufferSubData(GL_ARRAY_BUFFER, current_offset, m_bo_functional_data_vertex_size,
						 m_bo_functional_data_vertex);
		current_offset += m_bo_functional_data_vertex_size;

		gl.bufferSubData(GL_ARRAY_BUFFER, current_offset, m_bo_functional2_data_vertex_size,
						 m_bo_functional2_data_vertex);
		current_offset += m_bo_functional2_data_vertex_size;

		gl.bufferSubData(GL_ARRAY_BUFFER, current_offset, m_bo_functional_data_index_size, m_bo_functional_data_index);
		current_offset += m_bo_functional_data_index_size;

		gl.bufferSubData(GL_ARRAY_BUFFER, current_offset, m_bo_functional2_data_index_size,
						 m_bo_functional2_data_index);
		current_offset += m_bo_functional2_data_index_size;

		gl.bufferSubData(GL_ARRAY_BUFFER, current_offset, m_bo_functional3_data_index_size,
						 m_bo_functional3_data_index);
		current_offset += m_bo_functional3_data_index_size;

		gl.bufferSubData(GL_ARRAY_BUFFER, current_offset, m_bo_functional4_data_index_size,
						 m_bo_functional4_data_index);
		current_offset += m_bo_functional4_data_index_size;

		gl.bufferSubData(GL_ARRAY_BUFFER, current_offset, m_bo_functional5_data_index_size,
						 m_bo_functional5_data_index);
		current_offset += m_bo_functional5_data_index_size;

		gl.bufferSubData(GL_ARRAY_BUFFER, current_offset, m_bo_functional_data_color_size, m_bo_functional_data_color);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call(s) failed.");
	}

	if (use_overflow_test_vertices && m_bo_id_2 == 0)
	{
		/* Create a special buffer that only has vertex data in a few slots */
		gl.genBuffers(1, &m_bo_id_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

		gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_2);
		gl.bufferData(GL_ARRAY_BUFFER, 2 * 65550 * sizeof(glw::GLfloat), NULL, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed");

		/*
		 * Upload to offset 0:     regular draw call indices for making sure ubyte
		 *                         and ushort indices don't wrap around too early
		 * Upload to offset 256:   base draw call indices of type ubyte
		 * Upload to offset 65536: base draw call indices of type ushort
		 */
		glw::GLfloat badVtx[] = { 0.9f,  0.9f,  -0.9f, 0.9f,  -0.9f, -0.9f, 0.9f,  -0.9f, 0.9f,  0.9f,  -0.9f, 0.9f,
								  -0.9f, -0.9f, 0.9f,  -0.9f, 0.9f,  0.9f,  -0.9f, 0.9f,  -0.9f, -0.9f, 0.9f,  -0.9f };
		gl.bufferSubData(GL_ARRAY_BUFFER, 0, sizeof(badVtx), badVtx);
		gl.bufferSubData(GL_ARRAY_BUFFER, 2 * 256 * sizeof(glw::GLfloat), 2 * 12 * sizeof(glw::GLfloat),
						 m_bo_functional_data_vertex);
		gl.bufferSubData(GL_ARRAY_BUFFER, 2 * 65536 * sizeof(glw::GLfloat), 2 * 12 * sizeof(glw::GLfloat),
						 m_bo_functional_data_vertex);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed");
	}

	/* Set up the test program object */
	if (m_po_id == 0 || m_po_uses_gs_stage != use_geometry_shader_stage ||
		m_po_uses_tc_te_stages != use_tessellation_shader_stage ||
		m_po_uses_vertex_attrib_binding != use_vertex_attrib_binding)

	{
		static const char* functional_fs_code = "${VERSION}\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"in  vec4 FS_COLOR_INPUT_NAME;\n"
												"out vec4 result;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    result = FS_COLOR_INPUT_NAME;\n"
												"}\n";
		static const char* functional_gs_code = "${VERSION}\n"
												"${GEOMETRY_SHADER_REQUIRE}\n"
												"\n"
												"layout(triangles)                        in;\n"
												"layout(triangle_strip, max_vertices = 3) out;\n"
												"\n"
												"in  vec4 GS_COLOR_INPUT_NAME[];\n"
												"out vec4 FS_COLOR_INPUT_NAME;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    for (int n = 0; n < 3; ++n)\n"
												"    {\n"
												"        gl_Position         = vec4(gl_in[n].gl_Position.x, "
												"-gl_in[n].gl_Position.y, gl_in[n].gl_Position.zw);\n"
												"        FS_COLOR_INPUT_NAME = GS_COLOR_INPUT_NAME[n];\n"
												"\n"
												"        EmitVertex();\n"
												"    }\n"
												"\n"
												"    EndPrimitive();\n"
												"}\n";
		static const char* functional_tc_code =
			"${VERSION}\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"\n"
			"in  vec4 TC_COLOR_INPUT_NAME[];\n"
			"out vec4 TE_COLOR_INPUT_NAME[];\n"
			"\n"
			"layout(vertices = 3) out;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    gl_out             [gl_InvocationID].gl_Position = gl_in              [gl_InvocationID].gl_Position;\n"
			"    TE_COLOR_INPUT_NAME[gl_InvocationID]             = TC_COLOR_INPUT_NAME[gl_InvocationID];\n"
			"\n"
			"    gl_TessLevelInner[0] = 3.0;\n"
			"    gl_TessLevelOuter[0] = 3.0;\n"
			"    gl_TessLevelOuter[1] = 3.0;\n"
			"    gl_TessLevelOuter[2] = 3.0;\n"
			"}\n";
		static const char* functional_te_code =
			"${VERSION}\n"
			"\n"
			"${TESSELLATION_SHADER_REQUIRE}\n"
			"\n"
			"layout(triangles, equal_spacing, cw) in;\n"
			"\n"
			"in  vec4 TE_COLOR_INPUT_NAME[];\n"
			"out vec4 TE_COLOR_OUTPUT_NAME;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    vec2 p1 = gl_in[0].gl_Position.xy;\n"
			"    vec2 p2 = gl_in[1].gl_Position.xy;\n"
			"    vec2 p3 = gl_in[2].gl_Position.xy;\n"
			"\n"
			"    TE_COLOR_OUTPUT_NAME = TE_COLOR_INPUT_NAME[0] + TE_COLOR_INPUT_NAME[1] + TE_COLOR_INPUT_NAME[2];\n"
			"    gl_Position          = vec4(gl_TessCoord.x * p1.x + gl_TessCoord.y * p2.x + gl_TessCoord.z * p3.x,\n"
			"                                gl_TessCoord.x * p1.y + gl_TessCoord.y * p2.y + gl_TessCoord.z * p3.y,\n"
			"                                0.0,\n"
			"                                1.0);\n"
			"}\n";
		static const char* functional_vs_code = "${VERSION}\n"
												"\n"
												"out vec4 VS_COLOR_OUTPUT_NAME;\n"
												"in  vec4 vertex;\n"
												"\n"
												"OPTIONAL_USE_VERTEX_ATTRIB_BINDING_DEFINITIONS\n"
												"\n"
												"void main()\n"
												"{\n"
												"    float vertex_id_float = float(gl_VertexID);\n"
												"\n"
												"#ifdef USE_VERTEX_ATTRIB_BINDING\n"
												"    vec4  color           = in_color;\n"
												"#else\n"
												"    vec4  color           = vec4(vertex_id_float / 7.0,\n"
												"                                 vertex_id_float / 3.0,\n"
												"                                 vertex_id_float / 17.0,\n"
												"                                 1.0);\n"
												"#endif\n"
												"    float scale           = (gl_InstanceID == 0) ? 1.0 :\n"
												"                            (gl_InstanceID == 1) ? 0.8 :\n"
												"                                                   0.5;\n"
												"\n"
												"    VS_COLOR_OUTPUT_NAME = color;\n"
												"    gl_Position          = vec4(vertex.xy * scale, vertex.zw);\n"
												"}\n";

		/* Release a program object if one has already been set up for the running test */
		deinitProgramAndShaderObjects();

		/* Replace the tokens with actual variable names */
		std::string fs_body					  = specializeShader(1, &functional_fs_code);
		std::string fs_color_input_name_token = "FS_COLOR_INPUT_NAME";
		std::string fs_color_input_name_token_value;
		std::string gs_body					  = specializeShader(1, &functional_gs_code);
		std::string gs_color_input_name_token = "GS_COLOR_INPUT_NAME";
		std::string gs_color_input_name_token_value;
		std::string tc_body					  = specializeShader(1, &functional_tc_code);
		std::string tc_color_input_name_token = "TC_COLOR_INPUT_NAME";
		std::string tc_color_input_name_token_value;
		std::string te_body					  = specializeShader(1, &functional_te_code);
		std::string te_color_input_name_token = "TE_COLOR_INPUT_NAME";
		std::string te_color_input_name_token_value;
		std::string te_color_output_name_token = "TE_COLOR_OUTPUT_NAME";
		std::string te_color_output_name_token_value;
		std::string vs_body					   = specializeShader(1, &functional_vs_code);
		std::string vs_color_output_name_token = "VS_COLOR_OUTPUT_NAME";
		std::string vs_color_output_name_token_value;
		std::string vs_optional_use_vertex_attrib_binding_definitions_token =
			"OPTIONAL_USE_VERTEX_ATTRIB_BINDING_DEFINITIONS";
		std::string vs_optional_use_vertex_attrib_binding_definitions_token_value;

		std::string*	   bodies[] = { &fs_body, &gs_body, &tc_body, &te_body, &vs_body };
		const unsigned int n_bodies = sizeof(bodies) / sizeof(bodies[0]);

		std::string* token_value_pairs[] = { &fs_color_input_name_token,
											 &fs_color_input_name_token_value,
											 &gs_color_input_name_token,
											 &gs_color_input_name_token_value,
											 &tc_color_input_name_token,
											 &tc_color_input_name_token_value,
											 &te_color_input_name_token,
											 &te_color_input_name_token_value,
											 &te_color_output_name_token,
											 &te_color_output_name_token_value,
											 &vs_color_output_name_token,
											 &vs_color_output_name_token_value,
											 &vs_optional_use_vertex_attrib_binding_definitions_token,
											 &vs_optional_use_vertex_attrib_binding_definitions_token_value };
		const unsigned int n_token_value_pairs =
			sizeof(token_value_pairs) / sizeof(token_value_pairs[0]) / 2 /* token+value */;

		if (!use_tessellation_shader_stage)
		{
			if (!use_geometry_shader_stage)
			{
				/* Geometry & tessellation shader stages are not required.
				 *
				 * NOTE: This code-path is also used by Functional Test VII which verifies that
				 *       vertex attribute bindings work correctly with basevertex draw calls.
				 *       The test only uses FS & VS in its rendering pipeline and consumes a color
				 *       taken from an enabled vertex attribute array instead of a vector value
				 *       calculated in vertex shader stage,
				 */
				vs_color_output_name_token_value = "out_vs_color";
				fs_color_input_name_token_value  = vs_color_output_name_token_value;

				if (use_vertex_attrib_binding)
				{
					vs_optional_use_vertex_attrib_binding_definitions_token_value =
						"#define USE_VERTEX_ATTRIB_BINDING\n"
						"in vec4 in_color;\n";
				}
			} /* if (!use_geometry_shader_stage) */
			else
			{
				/* Geometry shader stage is needed, but tessellation shader stage
				 * can be skipped */
				fs_color_input_name_token_value  = "out_fs_color";
				gs_color_input_name_token_value  = "out_vs_color";
				vs_color_output_name_token_value = "out_vs_color";

				DE_ASSERT(!use_vertex_attrib_binding);
			}
		} /* if (!use_tessellation_shader_stage) */
		else
		{
			DE_ASSERT(!use_vertex_attrib_binding);

			if (!use_geometry_shader_stage)
			{
				/* Tessellation shader stage is needed, but geometry shader stage
				 * can be skipped */
				fs_color_input_name_token_value  = "out_fs_color";
				tc_color_input_name_token_value  = "out_vs_color";
				te_color_input_name_token_value  = "out_tc_color";
				te_color_output_name_token_value = "out_fs_color";
				vs_color_output_name_token_value = "out_vs_color";
			} /* if (!use_geometry_shader_stage) */
			else
			{
				/* Both tessellation and geometry shader stages are needed */
				fs_color_input_name_token_value  = "out_fs_color";
				gs_color_input_name_token_value  = "out_te_color";
				tc_color_input_name_token_value  = "out_vs_color";
				te_color_input_name_token_value  = "out_tc_color";
				te_color_output_name_token_value = "out_te_color";
				vs_color_output_name_token_value = "out_vs_color";
			}
		}

		for (unsigned int n_body = 0; n_body < n_bodies; ++n_body)
		{
			std::string* body_ptr = bodies[n_body];

			for (unsigned int n_token_value_pair = 0; n_token_value_pair < n_token_value_pairs; ++n_token_value_pair)
			{
				std::string token		   = *token_value_pairs[2 * n_token_value_pair + 0];
				std::size_t token_location = std::string::npos;
				std::string value		   = *token_value_pairs[2 * n_token_value_pair + 1];

				while ((token_location = body_ptr->find(token)) != std::string::npos)
				{
					body_ptr->replace(token_location, token.length(), value);
				} /* while (tokens are found) */
			}	 /* for (all token+value pairs) */
		}		  /* for (all bodies) */

		/* Build the actual program */
		buildProgram(fs_body.c_str(), vs_body.c_str(), use_tessellation_shader_stage ? tc_body.c_str() : DE_NULL,
					 use_tessellation_shader_stage ? te_body.c_str() : DE_NULL,
					 use_geometry_shader_stage ? gs_body.c_str() : DE_NULL);

		m_po_uses_gs_stage				= use_geometry_shader_stage;
		m_po_uses_tc_te_stages			= use_tessellation_shader_stage;
		m_po_uses_vertex_attrib_binding = use_vertex_attrib_binding;
	}

	/* Set up the vertex array object */
	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays() call failed.");

		m_vao_id = 0;
	}

	if (!use_clientside_vertex_data)
	{
		gl.genVertexArrays(1, &m_vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");
	}
	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	if (m_po_color_attribute_location != -1)
	{
		DE_ASSERT(use_vertex_attrib_binding);

		gl.enableVertexAttribArray(m_po_color_attribute_location);
	}

	gl.enableVertexAttribArray(m_po_vertex_attribute_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call(s) failed.");

	/* Configure the VAO */
	if (use_clientside_index_data)
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
	}
	else
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
	}

	if (use_clientside_vertex_data)
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
	}
	else
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, use_overflow_test_vertices ? m_bo_id_2 : m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
	}

	if (!use_vertex_attrib_binding)
	{
		DE_ASSERT(m_po_color_attribute_location == -1);

		gl.vertexAttribPointer(m_po_vertex_attribute_location, 2, /* size */
							   GL_FLOAT, GL_FALSE,				  /* normalized */
							   0,								  /* stride */
							   m_draw_call_vertex_offset);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed.");
	} /* if (!use_vertex_attrib_binding) */
	else
	{
		DE_ASSERT(m_po_color_attribute_location != -1);
		DE_ASSERT(m_draw_call_vertex_offset < m_draw_call_color_offset);

		gl.vertexAttribFormat(m_po_color_attribute_location, 2, /* size */
							  GL_FLOAT,							/* type */
							  GL_FALSE,							/* normalized */
							  (glw::GLuint)((const glw::GLubyte*)m_draw_call_color_offset -
											(const glw::GLubyte*)m_draw_call_vertex_offset));
		gl.vertexAttribFormat(m_po_vertex_attribute_location, 2, /* size */
							  GL_FLOAT,							 /* type */
							  GL_FALSE,							 /* normalized */
							  0);								 /* relativeoffset */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribFormat() call(s) failed.");

		gl.vertexAttribBinding(m_po_color_attribute_location, 0);  /* bindingindex */
		gl.vertexAttribBinding(m_po_vertex_attribute_location, 0); /* bindingindex */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribBinding() call(s) failed.");

		gl.bindVertexBuffer(0, /* bindingindex */
							(use_clientside_vertex_data) ? 0 : (use_overflow_test_vertices ? m_bo_id_2 : m_bo_id),
							(glw::GLintptr)m_draw_call_vertex_offset, sizeof(float) * 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexBuffer() call failed.");
	}

	/* Bind the program object to the rendering context */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
}

/** Sets up all ES objects required to run a single negative test case iteration.
 *
 *  @param use_clientside_vertex_data    true if the test case requires client-side buffers to
 *                                       back the color/vertex vertex attribute arrays; false
 *                                       to use buffer object storage.
 *  @param use_clientside_index_data     true if the test case requires client-side buffers to
 *                                       be used as index data source; false to use buffer object
 *                                       storage.
 */
void DrawElementsBaseVertexTestBase::setUpNegativeTestObjects(bool use_clientside_vertex_data,
															  bool use_clientside_index_data)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up buffer object we will use for the draw calls. Use the data
	 * from the test specification */
	if (m_bo_id == 0)
	{
		gl.genBuffers(1, &m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

		gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

		gl.bufferData(GL_ARRAY_BUFFER, m_bo_negative_data_index_size + m_bo_negative_data_vertex_size,
					  DE_NULL, /* data */
					  GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

		gl.bufferSubData(GL_ARRAY_BUFFER, 0, /* offset */
						 m_bo_negative_data_vertex_size, m_bo_negative_data_vertex);
		gl.bufferSubData(GL_ARRAY_BUFFER, m_bo_negative_data_vertex_size, /* offset */
						 m_bo_negative_data_index_size, m_bo_negative_data_index);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call(s) failed.");
	}

	if (use_clientside_index_data)
	{
		m_draw_call_index_offset = m_bo_negative_data_index;
	}
	else
	{
		m_draw_call_index_offset = (const glw::GLuint*)(deUintptr)m_bo_negative_data_vertex_size;
	}

	if (use_clientside_vertex_data)
	{
		m_draw_call_vertex_offset = m_bo_negative_data_vertex;
	}
	else
	{
		m_draw_call_vertex_offset = DE_NULL;
	}

	/* Set up the test program object */
	if (m_po_id == 0)
	{
		static const char* negative_fs_code = "${VERSION}\n"
											  "precision highp float;\n"
											  "\n"
											  "out vec4 result;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    result = vec4(1.0);\n"
											  "}\n";
		static const char* negative_vs_code = "${VERSION}\n"
											  "\n"
											  "in vec4 vertex;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    gl_Position = vertex;\n"
											  "}\n";

		std::string fs_specialized_code		= specializeShader(1, &negative_fs_code);
		const char* fs_specialized_code_raw = fs_specialized_code.c_str();
		std::string vs_specialized_code		= specializeShader(1, &negative_vs_code);
		const char* vs_specialized_code_raw = vs_specialized_code.c_str();

		buildProgram(fs_specialized_code_raw, vs_specialized_code_raw, DE_NULL, /* tc_code */
					 DE_NULL,													/* te_code */
					 DE_NULL);													/* gs_code */
	}

	/* Set up a vertex array object */
	if (m_vao_id == 0)
	{
		gl.genVertexArrays(1, &m_vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

		gl.bindVertexArray(m_vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

		gl.enableVertexAttribArray(m_po_vertex_attribute_location);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");
	}

	/* Configure the VAO */
	if (use_clientside_index_data)
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
	}
	else
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
	}

	if (use_clientside_vertex_data)
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

		gl.bindVertexArray(0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
	}
	else
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
	}

	gl.vertexAttribPointer(m_po_vertex_attribute_location, 2, /* size */
						   GL_FLOAT, GL_FALSE,				  /* normalized */
						   0,								  /* stride */
						   m_draw_call_vertex_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed.");

	/* Bind the program object to the rendering context */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior::DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior(
	Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "basevertex_behavior1",
									 "Verifies basevertex draw calls work correctly for a number of "
									 "different rendering pipeline and VAO configurations")
{
	/* Left blank on purpose */
}

/** Sets up test case descriptors for the test instance. These will later be used
 *  as input for DrawElementsBaseVertexTestBase::executeTestCases(), which performs
 *  the actual testing.
 **/
void DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior::setUpTestCases()
{
	/* Set up test case descriptors */
	const glw::GLint   basevertex_values[] = { 10, 0 };
	const unsigned int n_basevertex_values = sizeof(basevertex_values) / sizeof(basevertex_values[0]);

	/* The test needs to be run in two iterations, using client-side memory and buffer object
	 * for index data respectively
	 */
	for (int vao_iteration = 0; vao_iteration < 2; ++vao_iteration)
	{
		/* Skip client-side vertex array as gl_VertexID is undefined when no buffer bound to ARRAY_BUFFER
		 *  See section 11.1.3.9 in OpenGL ES 3.1 spec
		 */
		bool use_clientside_vertex_data = 0;
		bool use_clientside_index_data  = ((vao_iteration & (1 << 0)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Compute the offsets */
		computeVBODataOffsets(use_clientside_index_data, use_clientside_vertex_data);

		/* There are two index data sets we need to iterate over */
		const glw::GLuint* index_offsets[] = { m_draw_call_index_offset, m_draw_call_index2_offset };
		const unsigned int n_index_offsets = sizeof(index_offsets) / sizeof(index_offsets[0]);

		for (unsigned int n_index_offset = 0; n_index_offset < n_index_offsets; ++n_index_offset)
		{
			const glw::GLuint* current_index_offset = index_offsets[n_index_offset];

			/* We need to test four different functions:
			 *
			 * a)    glDrawElementsBaseVertex()             (GL)
			 *    or glDrawElementsBaseVertexEXT()          (ES)
			 * b)    glDrawRangeElementsBaseVertex()        (GL)
			 *    or glDrawRangeElementsBaseVertexEXT()     (ES)
			 * c)    glDrawElementsInstancedBaseVertex()    (GL)
			 *    or glDrawElementsInstancedBaseVertexEXT() (ES)
			 * d)    glMultiDrawElementsBaseVertex()        (GL)
			 *    or glMultiDrawElementsBaseVertexEXT()     (ES) (if supported)
			 **/
			for (int n_function = 0; n_function < FUNCTION_COUNT; ++n_function)
			{
				/* Do not try to use the multi draw call if relevant extension is
				 * not supported. */
				if (!m_is_ext_multi_draw_arrays_supported && n_function == FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX)
				{
					continue;
				}

				/* We need to run the test for a number of different basevertex values. */
				for (unsigned int n_basevertex_value = 0; n_basevertex_value < n_basevertex_values;
					 ++n_basevertex_value)
				{
					/* Finally, we want to verify that basevertex draw calls work correctly both when vertex attribute
					 * bindings are enabled and disabled */
					bool		 vertex_attrib_binding_statuses[] = { false, true };
					unsigned int n_vertex_attrib_binding_statuses =
						sizeof(vertex_attrib_binding_statuses) / sizeof(vertex_attrib_binding_statuses[0]);

					for (unsigned int n_vertex_attrib_binding_status = 0;
						 n_vertex_attrib_binding_status < n_vertex_attrib_binding_statuses;
						 ++n_vertex_attrib_binding_status)
					{
						bool use_vertex_attrib_binding = vertex_attrib_binding_statuses[n_vertex_attrib_binding_status];

						/* Under GL, "vertex attrib binding" functionality is only available if GL_ARB_vertex_attrib_binding
						 * extension is supported.
						 */
						if (!m_is_vertex_attrib_binding_supported && use_vertex_attrib_binding)
						{
							continue;
						}

						/* Prepare a few arrays so that we can handle the multi draw call and its emulated version.. */
						const glw::GLsizei multi_draw_call_count_array[3]   = { 3, 6, 3 };
						const glw::GLuint* multi_draw_call_indices_array[3] = {
							(glw::GLuint*)(current_index_offset), (glw::GLuint*)(current_index_offset + 3),
							(glw::GLuint*)(current_index_offset + 9)
						};

						/* Reference texture should always reflect basevertex=10 behavior. */
						const glw::GLuint  regular_draw_call_offset					 = basevertex_values[0];
						const glw::GLuint* regular_multi_draw_call_offseted_array[3] = {
							multi_draw_call_indices_array[0] + regular_draw_call_offset,
							multi_draw_call_indices_array[1] + regular_draw_call_offset,
							multi_draw_call_indices_array[2] + regular_draw_call_offset,
						};

						/* Construct the test case descriptor */
						_test_case new_test_case;

						new_test_case.basevertex			   = basevertex_values[n_basevertex_value];
						new_test_case.function_type			   = (_function_type)n_function;
						new_test_case.index_offset			   = current_index_offset;
						new_test_case.range_start			   = n_index_offset == 0 ? 0 : 10;
						new_test_case.range_end				   = n_index_offset == 0 ? 22 : 32;
						new_test_case.index_type			   = GL_UNSIGNED_INT;
						new_test_case.primitive_mode		   = GL_TRIANGLES;
						new_test_case.regular_draw_call_offset = basevertex_values[0];
						new_test_case.should_base_texture_match_reference_texture =
							((glw::GLuint)new_test_case.basevertex == new_test_case.regular_draw_call_offset);
						new_test_case.use_clientside_index_data		= use_clientside_index_data;
						new_test_case.use_clientside_vertex_data	= use_clientside_vertex_data;
						new_test_case.use_geometry_shader_stage		= false;
						new_test_case.use_tessellation_shader_stage = false;
						new_test_case.use_vertex_attrib_binding		= use_vertex_attrib_binding;

						memcpy(new_test_case.multi_draw_call_count_array, multi_draw_call_count_array,
							   sizeof(multi_draw_call_count_array));
						memcpy(new_test_case.multi_draw_call_indices_array, multi_draw_call_indices_array,
							   sizeof(multi_draw_call_indices_array));
						memcpy(new_test_case.regular_multi_draw_call_offseted_array,
							   regular_multi_draw_call_offseted_array, sizeof(regular_multi_draw_call_offseted_array));

						m_test_cases.push_back(new_test_case);
					} /* for (all vertex_attrib_binding statuses) */
				}	 /* for (all basevertex values) */
			}		  /* for (all four functions) */
		}			  /* for (all index data sets) */
	}				  /* for (all VAO iterations) */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior::iterate()
{
	setUpTestCases();
	executeTestCases();

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior2::DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior2(
	Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "basevertex_behavior2",
									 "Verifies basevertex draw calls work correctly for a number of "
									 "different rendering pipeline and VAO configurations. Uses slightly "
									 "different data set than basevertex_behavior.")
{
	/* Left blank on purpose */
}

/** Sets up test case descriptors for the test instance. These will later be used
 *  as input for DrawElementsBaseVertexTestBase::executeTestCases(), which performs
 *  the actual testing.
 **/
void DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior2::setUpTestCases()
{
	/* Set up test case descriptors */
	const glw::GLint basevertex = 5;

	/* The test needs to be run in two iterations, using client-side memory and buffer object
	 * for index data respectively
	 */
	for (int vao_iteration = 0; vao_iteration < 2; ++vao_iteration)
	{
		/* Skip client-side vertex array as gl_VertexID is undefined when no buffer bound to ARRAY_BUFFER
		 *  See section 11.1.3.9 in OpenGL ES 3.1 spec
		 */
		bool use_clientside_vertex_data = 0;
		bool use_clientside_index_data  = ((vao_iteration & (1 << 0)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Compute the offsets */
		computeVBODataOffsets(use_clientside_index_data, use_clientside_vertex_data);

		/* We need to test four different functions:
		 *
		 * a)    glDrawElementsBaseVertex()             (GL)
		 *    or glDrawElementsBaseVertexEXT()          (ES)
		 * b)    glDrawRangeElementsBaseVertex()        (GL)
		 *    or glDrawRangeElementsBaseVertexEXT()     (ES)
		 * c)    glDrawElementsInstancedBaseVertex()    (GL)
		 *    or glDrawElementsInstancedBaseVertexEXT() (ES)
		 * d)    glMultiDrawElementsBaseVertex()        (GL)
		 *    or glMultiDrawElementsBaseVertexEXT()     (ES) (if supported)
		 **/
		for (int n_function = 0; n_function < FUNCTION_COUNT; ++n_function)
		{
			/* Do not try to use the multi draw call if relevant extension is
			 * not supported. */
			if (!m_is_ext_multi_draw_arrays_supported && n_function == FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX)
			{
				continue;
			}

			/* Prepare a few arrays so that we can handle the multi draw call and its emulated version.. */
			const glw::GLsizei multi_draw_call_count_array[3]   = { 3, 6, 3 };
			const glw::GLuint* multi_draw_call_indices_array[3] = { (glw::GLuint*)(m_draw_call_index_offset),
																	(glw::GLuint*)(m_draw_call_index_offset + 3),
																	(glw::GLuint*)(m_draw_call_index_offset + 9) };

			/* Reference texture should always reflect basevertex=10 behavior. */
			const glw::GLuint  regular_draw_call_offset					 = 0;
			const glw::GLuint* regular_multi_draw_call_offseted_array[3] = {
				multi_draw_call_indices_array[0] + regular_draw_call_offset,
				multi_draw_call_indices_array[1] + regular_draw_call_offset,
				multi_draw_call_indices_array[2] + regular_draw_call_offset,
			};

			/* Construct the test case descriptor */
			_test_case new_test_case;

			new_test_case.basevertex			   = basevertex;
			new_test_case.function_type			   = (_function_type)n_function;
			new_test_case.index_offset			   = m_draw_call_index_offset;
			new_test_case.range_start			   = 0;
			new_test_case.range_end				   = 22;
			new_test_case.index_type			   = GL_UNSIGNED_INT;
			new_test_case.primitive_mode		   = GL_TRIANGLES;
			new_test_case.regular_draw_call_offset = regular_draw_call_offset;
			new_test_case.should_base_texture_match_reference_texture =
				((glw::GLuint)new_test_case.basevertex == new_test_case.regular_draw_call_offset);
			new_test_case.use_clientside_index_data		= use_clientside_index_data;
			new_test_case.use_clientside_vertex_data	= use_clientside_vertex_data;
			new_test_case.use_geometry_shader_stage		= false;
			new_test_case.use_tessellation_shader_stage = false;
			new_test_case.use_vertex_attrib_binding		= false;

			memcpy(new_test_case.multi_draw_call_count_array, multi_draw_call_count_array,
				   sizeof(multi_draw_call_count_array));
			memcpy(new_test_case.multi_draw_call_indices_array, multi_draw_call_indices_array,
				   sizeof(multi_draw_call_indices_array));
			memcpy(new_test_case.regular_multi_draw_call_offseted_array, regular_multi_draw_call_offseted_array,
				   sizeof(regular_multi_draw_call_offseted_array));

			m_test_cases.push_back(new_test_case);
		} /* for (all four functions) */
	}	 /* for (all VAO iterations) */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior2::iterate()
{
	setUpTestCases();
	executeTestCases();

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorUnderflow::
	DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorUnderflow(Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "underflow",
									 "Verifies basevertex draw calls work correctly for negative "
									 "basevertex values")
{
	/* Left blank on purpose */
}

/** Sets up test case descriptors for the test instance. These will later be used
 *  as input for DrawElementsBaseVertexTestBase::executeTestCases(), which performs
 *  the actual testing.
 **/
void DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorUnderflow::setUpTestCases()
{
	/* Set up test case descriptors */
	const glw::GLint basevertex = -10;

	/* The test needs to be run in two iterations, using client-side memory and buffer object
	 * for index data respectively
	 */
	for (int vao_iteration = 0; vao_iteration < 2; ++vao_iteration)
	{
		/* Skip client-side vertex array as gl_VertexID is undefined when no buffer bound to ARRAY_BUFFER
		 *  See section 11.1.3.9 in OpenGL ES 3.1 spec
		 */
		bool use_clientside_vertex_data = 0;
		bool use_clientside_index_data  = ((vao_iteration & (1 << 0)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Compute the offsets */
		computeVBODataOffsets(use_clientside_index_data, use_clientside_vertex_data);

		/* We need to test four different functions:
		 *
		 * a)    glDrawElementsBaseVertex()             (GL)
		 *    or glDrawElementsBaseVertexEXT()          (ES)
		 * b)    glDrawRangeElementsBaseVertex()        (GL)
		 *    or glDrawRangeElementsBaseVertexEXT()     (ES)
		 * c)    glDrawElementsInstancedBaseVertex()    (GL)
		 *    or glDrawElementsInstancedBaseVertexEXT() (ES)
		 * d)    glMultiDrawElementsBaseVertex()        (GL)
		 *    or glMultiDrawElementsBaseVertexEXT()     (ES) (if supported)
		 **/
		for (int n_function = 0; n_function < FUNCTION_COUNT; ++n_function)
		{
			/* Do not try to use the multi draw call if relevant extension is
			 * not supported. */
			if (!m_is_ext_multi_draw_arrays_supported && n_function == FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX)
			{
				continue;
			}

			/* Prepare a few arrays so that we can handle the multi draw call and its emulated version.. */
			unsigned int offset_from_10_to_0_index = 23; /* this offset moves us from the start index of 10 to a zero
			 * index, given the second index data set we will be using.
			 * Please see declaration of functional2_index_data if you
			 * need to verify this by yourself. */

			const glw::GLsizei multi_draw_call_count_array[3]   = { 3, 6, 3 };
			const glw::GLuint* multi_draw_call_indices_array[3] = { (glw::GLuint*)(m_draw_call_index2_offset),
																	(glw::GLuint*)(m_draw_call_index2_offset + 3),
																	(glw::GLuint*)(m_draw_call_index2_offset + 9) };
			const glw::GLuint* regular_multi_draw_call_offseted_array[3] = {
				multi_draw_call_indices_array[0] + offset_from_10_to_0_index,
				multi_draw_call_indices_array[1] + offset_from_10_to_0_index,
				multi_draw_call_indices_array[2] + offset_from_10_to_0_index,
			};

			/* Construct the test case descriptor */
			_test_case new_test_case;

			new_test_case.basevertex								  = basevertex;
			new_test_case.function_type								  = (_function_type)n_function;
			new_test_case.index_offset								  = m_draw_call_index2_offset;
			new_test_case.range_start								  = 10;
			new_test_case.range_end									  = 32;
			new_test_case.index_type								  = GL_UNSIGNED_INT;
			new_test_case.primitive_mode							  = GL_TRIANGLES;
			new_test_case.regular_draw_call_offset					  = offset_from_10_to_0_index;
			new_test_case.should_base_texture_match_reference_texture = true;
			new_test_case.use_clientside_index_data					  = use_clientside_index_data;
			new_test_case.use_clientside_vertex_data				  = use_clientside_vertex_data;
			new_test_case.use_geometry_shader_stage					  = false;
			new_test_case.use_tessellation_shader_stage				  = false;
			new_test_case.use_vertex_attrib_binding					  = false;

			memcpy(new_test_case.multi_draw_call_count_array, multi_draw_call_count_array,
				   sizeof(multi_draw_call_count_array));
			memcpy(new_test_case.multi_draw_call_indices_array, multi_draw_call_indices_array,
				   sizeof(multi_draw_call_indices_array));
			memcpy(new_test_case.regular_multi_draw_call_offseted_array, regular_multi_draw_call_offseted_array,
				   sizeof(regular_multi_draw_call_offseted_array));

			m_test_cases.push_back(new_test_case);
		} /* for (all four functions) */
	}	 /* for (all VAO iterations) */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorUnderflow::iterate()
{
	setUpTestCases();
	executeTestCases();

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorOverflow::
	DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorOverflow(Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "overflow",
									 "Verifies basevertex draw calls work correctly for overflowing "
									 "basevertex values")
{
	/* Left blank on purpose */
}

/** Sets up test case descriptors for the test instance. These will later be used
 *  as input for DrawElementsBaseVertexTestBase::executeTestCases(), which performs
 *  the actual testing.
 **/
void DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorOverflow::setUpTestCases()
{
	/* The test needs to be run in two iterations, using client-side memory and buffer object
	 * for index data respectively
	 */
	for (int vao_iteration = 0; vao_iteration < 2; ++vao_iteration)
	{
		/* Skip client-side vertex array as gl_VertexID is undefined when no buffer bound to ARRAY_BUFFER
		 *  See section 11.1.3.9 in OpenGL ES 3.1 spec
		 */
		bool use_clientside_vertex_data = 0;
		bool use_clientside_index_data  = ((vao_iteration & (1 << 0)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Compute the offsets */
		computeVBODataOffsets(use_clientside_index_data, use_clientside_vertex_data);

		/* We need to test four different functions:
		 *
		 * a)    glDrawElementsBaseVertex()             (GL)
		 *    or glDrawElementsBaseVertexEXT()          (ES)
		 * b)    glDrawRangeElementsBaseVertex()        (GL)
		 *    or glDrawRangeElementsBaseVertexEXT()     (ES)
		 * c)    glDrawElementsInstancedBaseVertex()    (GL)
		 *    or glDrawElementsInstancedBaseVertexEXT() (ES)
		 * d)    glMultiDrawElementsBaseVertex()        (GL)
		 *    or glMultiDrawElementsBaseVertexEXT()     (ES) (if supported)
		 **/
		for (int n_function = 0; n_function < FUNCTION_COUNT; ++n_function)
		{
			/* Do not try to use the multi draw call if relevant extension is
			 * not supported. */
			if (!m_is_ext_multi_draw_arrays_supported && n_function == FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX)
			{
				continue;
			}

			const glw::GLenum index_types[] = { GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT };

			const unsigned int n_index_types = sizeof(index_types) / sizeof(index_types[0]);

			for (unsigned int n_index_type = 0; n_index_type < n_index_types; ++n_index_type)
			{

				glw::GLint			basevertex					 = -1;
				const glw::GLubyte* index_offset				 = NULL;
				int					index_size					 = 0;
				glw::GLenum			index_type					 = index_types[n_index_type];
				glw::GLenum			regular_draw_call_index_type = 0;
				int					regular_draw_call_index_size = 0;
				glw::GLboolean		use_overflow_test_vertices   = false;

				/* To meet requirements of tests IX.1-IX.2 and IX.3, we need to use two basevertex deltas. */
				const glw::GLint basevertex_deltas[] = {
					0, /* IX.1-IX.2 */
					1  /* IX.3 */
				};
				const unsigned int n_basevertex_deltas = sizeof(basevertex_deltas) / sizeof(basevertex_deltas[0]);

				for (unsigned int n_basevertex_delta = 0; n_basevertex_delta < n_basevertex_deltas;
					 ++n_basevertex_delta)
				{
					const glw::GLint basevertex_delta		   = basevertex_deltas[n_basevertex_delta];
					glw::GLuint		 regular_draw_call_offset  = 0;
					glw::GLuint*	 regular_draw_call_offset2 = NULL;
					glw::GLuint		 range_start			   = 0;
					glw::GLuint		 range_end				   = 0;
					bool			 shouldMatch;

					switch (index_type)
					{
					/*
					 * UBYTE base draw indices: 0, 1, 2
					 *              baseVertex: 256+0
					 *    regular draw indices: 0, 1, 2
					 *         expected result: ubyte indices should not wrap around at 8-bit width,
					 *                          base draw result should not match regular draw result
					 *
					 * UBYTE base draw indices: 0, 1, 2
					 *              baseVertex: 256+1
					 *    regular draw indices: 257, 258, 259 (uint)
					 *         expected result: ubyte indices should be upconverted to 32-bit uint,
					 *                          base draw result should match regular draw result
					 */
					case GL_UNSIGNED_BYTE:
					{
						basevertex					 = 256 + basevertex_delta;
						index_offset				 = (const glw::GLubyte*)m_draw_call_index3_offset;
						index_size					 = 1;
						range_start					 = 0;
						range_end					 = 2;
						regular_draw_call_index_type = (basevertex_delta == 0) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT;
						regular_draw_call_index_size = (regular_draw_call_index_type == GL_UNSIGNED_BYTE) ? 1 : 4;
						regular_draw_call_offset2 =
							(basevertex_delta == 0) ?
								(glw::GLuint*)(m_draw_call_index3_offset) :
								(glw::GLuint*)(m_draw_call_index5_offset + 12); // 12th in functional5_index_data
						shouldMatch				   = (basevertex_delta == 1) ? true : false;
						use_overflow_test_vertices = true;
						break;
					}

					/*
						 * USHORT base draw indices: 0, 1, 2
						 *               baseVertex: 65536+0
						 *     regular draw indices: 0, 1, 2
						 *          expected result: ubyte indices should not wrap around at 16-bit width,
						 *                           base draw result should not match regular draw result
						 *
						 * USHORT base draw indices: 0, 1, 2
						 *               baseVertex: 65536+1
						 *     regular draw indices: 65537, 65538, 65539 (uint)
						 *          expected result: ushort indices should be upconverted to 32-bit uint,
						 *                           base draw result should match regular draw result
						 */
					case GL_UNSIGNED_SHORT:
					{
						basevertex					 = 65536 + basevertex_delta;
						index_offset				 = (const glw::GLubyte*)m_draw_call_index4_offset;
						index_size					 = 2;
						range_start					 = 0;
						range_end					 = 2;
						regular_draw_call_index_type = (basevertex_delta == 0) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
						regular_draw_call_index_size = (regular_draw_call_index_type == GL_UNSIGNED_SHORT) ? 2 : 4;
						regular_draw_call_offset2 =
							(basevertex_delta == 0) ?
								(glw::GLuint*)(m_draw_call_index4_offset) :
								(glw::GLuint*)(m_draw_call_index5_offset + 24); // 24th in functional5_index_data
						shouldMatch				   = (basevertex_delta == 1) ? true : false;
						use_overflow_test_vertices = true;
						break;
					}

					/*
						 * UINT base draw indices: 2^31+2, 2^31+3, 2^31+4
						 *             baseVertex: 2^31-2
						 *   regular draw indices: 0, 1, 2
						 *        expected result: uint indices should wrap to {0, 1, 2},
						 *                         base draw result should match regular draw result
						 *
						 * UINT base draw indices: 2^31+2, 2^31+3, 2^31+4
						 *             baseVertex: 2^31-1
						 *   regular draw indices: 0, 1, 2
						 *        expected result: uint indices should wrap to {1, 2, 3},
						 *                         base draw result should not match regular draw result
						 */
					case GL_UNSIGNED_INT:
					{
						basevertex					 = 2147483647 - 1 + basevertex_delta;
						index_offset				 = (const glw::GLubyte*)m_draw_call_index5_offset;
						index_size					 = 4;
						range_start					 = 2147483647 + 3u; // 2^31+2
						range_end					 = 2147483647 + 5u; // 2^31+4
						regular_draw_call_index_type = GL_UNSIGNED_INT;
						regular_draw_call_index_size = 4;
						regular_draw_call_offset	 = 36; // 36th in functional5_index_data
						shouldMatch					 = (basevertex_delta == 0) ? true : false;
						use_overflow_test_vertices   = false;
						break;
					}

					default:
					{
						TCU_FAIL("Unrecognized index type");
					}
					} /* switch (index_type) */

					/* Prepare a few arrays so that we can handle the multi draw call and its emulated version.. */
					const glw::GLsizei  multi_draw_call_count_array[3]   = { 3, 6, 3 };
					const glw::GLubyte* multi_draw_call_indices_array[3] = { index_offset,
																			 index_offset + 3 * index_size,
																			 index_offset + 9 * index_size };
					const glw::GLubyte* regular_multi_draw_call_offseted_array[3];
					if (use_overflow_test_vertices)
					{
						regular_multi_draw_call_offseted_array[0] = (glw::GLubyte*)regular_draw_call_offset2;
						regular_multi_draw_call_offseted_array[1] =
							(glw::GLubyte*)regular_draw_call_offset2 + 3 * regular_draw_call_index_size;
						regular_multi_draw_call_offseted_array[2] =
							(glw::GLubyte*)regular_draw_call_offset2 + 9 * regular_draw_call_index_size;
					}
					else
					{
						regular_multi_draw_call_offseted_array[0] =
							multi_draw_call_indices_array[0] + index_size * regular_draw_call_offset;
						regular_multi_draw_call_offseted_array[1] =
							multi_draw_call_indices_array[1] + index_size * regular_draw_call_offset;
						regular_multi_draw_call_offseted_array[2] =
							multi_draw_call_indices_array[2] + index_size * regular_draw_call_offset;
					}

					/* Construct the test case descriptor */
					_test_case new_test_case;

					new_test_case.basevertex								  = basevertex;
					new_test_case.function_type								  = (_function_type)n_function;
					new_test_case.index_offset								  = (const glw::GLuint*)index_offset;
					new_test_case.range_start								  = range_start;
					new_test_case.range_end									  = range_end;
					new_test_case.index_type								  = index_type;
					new_test_case.primitive_mode							  = GL_TRIANGLES;
					new_test_case.regular_draw_call_offset					  = regular_draw_call_offset;
					new_test_case.regular_draw_call_offset2					  = regular_draw_call_offset2;
					new_test_case.regular_draw_call_index_type				  = regular_draw_call_index_type;
					new_test_case.should_base_texture_match_reference_texture = shouldMatch;
					new_test_case.use_clientside_index_data					  = use_clientside_index_data;
					new_test_case.use_clientside_vertex_data				  = use_clientside_vertex_data;
					new_test_case.use_geometry_shader_stage					  = false;
					new_test_case.use_tessellation_shader_stage				  = false;
					new_test_case.use_vertex_attrib_binding					  = false;
					new_test_case.use_overflow_test_vertices				  = use_overflow_test_vertices != GL_FALSE;

					memcpy(new_test_case.multi_draw_call_count_array, multi_draw_call_count_array,
						   sizeof(multi_draw_call_count_array));
					memcpy(new_test_case.multi_draw_call_indices_array, multi_draw_call_indices_array,
						   sizeof(multi_draw_call_indices_array));
					memcpy(new_test_case.regular_multi_draw_call_offseted_array, regular_multi_draw_call_offseted_array,
						   sizeof(regular_multi_draw_call_offseted_array));

					m_test_cases.push_back(new_test_case);
				} /* for (all basevertex deltas) */
			}	 /* for (all index types) */
		}		  /* for (all four functions) */
	}			  /* for (all VAO iterations) */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorOverflow::iterate()
{
	setUpTestCases();
	executeTestCases();

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorAEPShaderStages::
	DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorAEPShaderStages(Context&			  context,
																			 const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "AEP_shader_stages",
									 "Verifies basevertex draw calls work correctly when geometry & "
									 "tessellation shader stages are used in the rendering pipeline.")
{
	/* Left blank intentionally */
}

/** Sets up test case descriptors for the test instance. These will later be used
 *  as input for DrawElementsBaseVertexTestBase::executeTestCases(), which performs
 *  the actual testing.
 **/
void DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorAEPShaderStages::setUpTestCases()
{
	/* Set up test case descriptors */
	const glw::GLuint basevertex_values[] = {
		10, /* VI.1-VI.4 */
		0   /* VI.5 */
	};
	const unsigned int n_basevertex_values = sizeof(basevertex_values) / sizeof(basevertex_values[0]);

	/* The test needs to be run in two iterations, using client-side memory and buffer object
	 * for index data respectively
	 */
	for (int vao_iteration = 0; vao_iteration < 2; ++vao_iteration)
	{
		/* Skip client-side vertex array as gl_VertexID is undefined when zero buffer is bound to ARRAY_BUFFER */
		bool use_clientside_vertex_data = 0;
		bool use_clientside_index_data  = ((vao_iteration & (1 << 0)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Compute the offsets */
		computeVBODataOffsets(use_clientside_index_data, use_clientside_vertex_data);

		/* There are two index data sets we need to iterate over */
		const glw::GLuint* index_offsets[] = { m_draw_call_index_offset, m_draw_call_index2_offset };
		const unsigned int n_index_offsets = sizeof(index_offsets) / sizeof(index_offsets[0]);

		for (unsigned int n_index_offset = 0; n_index_offset < n_index_offsets; ++n_index_offset)
		{
			const glw::GLuint* current_index_offset = index_offsets[n_index_offset];

			/* We need to test four different functions:
			 *
			 * a)    glDrawElementsBaseVertex()             (GL)
			 *    or glDrawElementsBaseVertexEXT()          (ES)
			 * b)    glDrawRangeElementsBaseVertex()        (GL)
			 *    or glDrawRangeElementsBaseVertexEXT()     (ES)
			 * c)    glDrawElementsInstancedBaseVertex()    (GL)
			 *    or glDrawElementsInstancedBaseVertexEXT() (ES)
			 * d)    glMultiDrawElementsBaseVertex()        (GL)
			 *    or glMultiDrawElementsBaseVertexEXT()     (ES) (if supported)
			 **/
			for (int n_function = 0; n_function < FUNCTION_COUNT; ++n_function)
			{
				/* Iterate over all basevertex values */
				for (unsigned int n_basevertex_value = 0; n_basevertex_value < n_basevertex_values;
					 ++n_basevertex_value)
				{

					/* Iterate over all GS+(TC & TE) stage combinations */
					for (unsigned int n_stage_combination = 0; n_stage_combination < 4; ++n_stage_combination)
					{
						bool should_include_gs	= (n_stage_combination & 1) != 0;
						bool should_include_tc_te = (n_stage_combination & 2) != 0;

						/* Skip iterations, for which we'd need to use extensions not supported
						 * by the running implementation */
						if (should_include_gs && !m_is_geometry_shader_supported)
						{
							continue;
						}

						if (should_include_tc_te && !m_is_tessellation_shader_supported)
						{
							continue;
						}

						/* Do not try to use the multi draw call if relevant extension is
						 * not supported. */
						if (!m_is_ext_multi_draw_arrays_supported &&
							n_function == FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX)
						{
							continue;
						}

						/* Prepare a few arrays so that we can handle the multi draw call and its emulated version.. */
						const glw::GLsizei multi_draw_call_count_array[3]   = { 3, 6, 3 };
						const glw::GLuint* multi_draw_call_indices_array[3] = {
							(glw::GLuint*)(current_index_offset), (glw::GLuint*)(current_index_offset + 3),
							(glw::GLuint*)(current_index_offset + 9)
						};

						/* Reference texture should always reflect basevertex=10 behavior. */
						const glw::GLuint  regular_draw_call_offset					 = basevertex_values[0];
						const glw::GLuint* regular_multi_draw_call_offseted_array[3] = {
							multi_draw_call_indices_array[0] + regular_draw_call_offset,
							multi_draw_call_indices_array[1] + regular_draw_call_offset,
							multi_draw_call_indices_array[2] + regular_draw_call_offset,
						};

						/* Construct the test case descriptor */
						_test_case new_test_case;

						new_test_case.basevertex			   = basevertex_values[n_basevertex_value];
						new_test_case.function_type			   = (_function_type)n_function;
						new_test_case.index_offset			   = current_index_offset;
						new_test_case.range_start			   = n_index_offset == 0 ? 0 : 10;
						new_test_case.range_end				   = n_index_offset == 0 ? 22 : 32;
						new_test_case.index_type			   = GL_UNSIGNED_INT;
						new_test_case.primitive_mode		   = (should_include_tc_te) ? GL_PATCHES : GL_TRIANGLES;
						new_test_case.regular_draw_call_offset = regular_draw_call_offset;
						new_test_case.should_base_texture_match_reference_texture =
							((glw::GLuint)new_test_case.basevertex == new_test_case.regular_draw_call_offset);
						new_test_case.use_clientside_index_data		= use_clientside_index_data;
						new_test_case.use_clientside_vertex_data	= use_clientside_vertex_data;
						new_test_case.use_geometry_shader_stage		= should_include_gs;
						new_test_case.use_tessellation_shader_stage = should_include_tc_te;
						new_test_case.use_vertex_attrib_binding		= false;

						memcpy(new_test_case.multi_draw_call_count_array, multi_draw_call_count_array,
							   sizeof(multi_draw_call_count_array));
						memcpy(new_test_case.multi_draw_call_indices_array, multi_draw_call_indices_array,
							   sizeof(multi_draw_call_indices_array));
						memcpy(new_test_case.regular_multi_draw_call_offseted_array,
							   regular_multi_draw_call_offseted_array, sizeof(regular_multi_draw_call_offseted_array));

						m_test_cases.push_back(new_test_case);
					} /* for (all shader stage combinations) */
				}	 /* for (all basevertex values) */
			}		  /* for (all four functions) */
		}			  /* for (all index data sets) */
	}				  /* for (all VAO iterations) */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorAEPShaderStages::iterate()
{
	/* This test should not be run on implementations that don't support both tessellation and geometry
	 * shader stages
	 */
	if (!m_is_geometry_shader_supported && !m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError("Geometry shader and tessellation shader functionality is not supported");
	}

	setUpTestCases();
	executeTestCases();

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexNegativeActiveTransformFeedbackTest::DrawElementsBaseVertexNegativeActiveTransformFeedbackTest(
	Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "valid_active_tf",
									 "Tries to do \"base vertex\" draw calls while Transform Feedback is active")
	, m_bo_tf_result_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects per test case if test fails and exit through exception path */
void DrawElementsBaseVertexNegativeActiveTransformFeedbackTest::deinit()
{
	deinitPerTestObjects();
}

/** Deinitializes all ES objects that may have been created by the test */
void DrawElementsBaseVertexNegativeActiveTransformFeedbackTest::deinitPerTestObjects()
{
	/* Call the base class' deinitPerTestObjects() first. */
	DrawElementsBaseVertexTestBase::deinitPerTestObjects();

	/* Proceed with internal deinitialization */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_tf_result_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_tf_result_id);

		m_bo_tf_result_id = 0;
	}
}

/** Initializes all ES objects used by the test. */
void DrawElementsBaseVertexNegativeActiveTransformFeedbackTest::init()
{
	/* Call the base class' init() first. */
	DrawElementsBaseVertexTestBase::init();

	/* Proceed with internal initialization */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genBuffers(1, &m_bo_tf_result_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_tf_result_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");

	gl.bufferData(GL_ARRAY_BUFFER, 3 /* count */ * sizeof(float) * 4 /* components used by gl_Position */,
				  DE_NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_bo_tf_result_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexNegativeActiveTransformFeedbackTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* this test doesn't apply to OpenGL contexts */
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	/* Set up the work environment */
	setUpNegativeTestObjects(false,  /* use_clientside_vertex_data */
							 false); /* use_clientside_index_data */

	/* Kick off transform feedback */
	gl.beginTransformFeedback(GL_TRIANGLES);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");

	/* Try to perform indiced draw calls which are invalid when TF is active.
	 */
	glw::GLenum error_code = GL_NONE;

	gl.drawElementsBaseVertex(GL_TRIANGLES, 3,								 /* count */
							  GL_UNSIGNED_INT, m_draw_call_index_offset, 0); /* basevertex */

	error_code = gl.getError();

	/* The error for using DrawElements* commands while transform feedback is
	 active is lifted in OES/EXT_geometry_shader. See issue 13 in the spec.
	 */
	glw::GLenum expected_error_code = m_is_geometry_shader_supported ? GL_NO_ERROR : GL_INVALID_OPERATION;

	if (error_code != expected_error_code)
	{
		std::stringstream error_sstream;

		error_sstream << "Invalid error code generated by " << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX);

		m_testCtx.getLog() << tcu::TestLog::Message << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX)
						   << " returned error code [" << error_code << "] instead of [" << expected_error_code << "]."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, error_sstream.str().c_str());

		goto end;
	}

	gl.drawElementsInstancedBaseVertex(GL_TRIANGLES, 3,								 /* count */
									   GL_UNSIGNED_INT, m_draw_call_index_offset, 1, /* instancecount */
									   0);											 /* basevertex */

	error_code = gl.getError();
	if (error_code != expected_error_code)
	{
		std::stringstream error_sstream;

		error_sstream << "Invalid error code generated by "
					  << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX);

		m_testCtx.getLog() << tcu::TestLog::Message << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX)
						   << " returned error code [" << error_code << "] instead of [" << expected_error_code << "]."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, error_sstream.str().c_str());

		goto end;
	}

	gl.drawRangeElementsBaseVertex(GL_TRIANGLES, 0, 2,							  /* end */
								   3,											  /* count */
								   GL_UNSIGNED_INT, m_draw_call_index_offset, 0); /* basevertex */

	error_code = gl.getError();
	if (error_code != expected_error_code)
	{
		std::stringstream error_sstream;

		error_sstream << "Invalid error code generated by "
					  << getFunctionName(FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX);

		m_testCtx.getLog() << tcu::TestLog::Message << getFunctionName(FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX)
						   << " returned error code [" << error_code << "] instead of [" << expected_error_code << "]."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, error_sstream.str().c_str());

		goto end;
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

end:
	gl.endTransformFeedback();
	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexNegativeInvalidCountArgumentTest::DrawElementsBaseVertexNegativeInvalidCountArgumentTest(
	Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "invalid_count_argument",
									 "Tries to use invalid 'count' argument values for the \"base vertex\" draw calls")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexNegativeInvalidCountArgumentTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* The test needs to be run in four iterations, where for each iteration we configure the VAO
	 * in a slightly different manner.
	 */
	for (int iteration = 0; iteration < 4; ++iteration)
	{
		bool use_clientside_index_data  = ((iteration & (1 << 0)) != 0);
		bool use_clientside_vertex_data = ((iteration & (1 << 1)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Set up the work environment */
		setUpNegativeTestObjects(use_clientside_vertex_data, use_clientside_index_data);

		/* Try to execute the invalid draw calls */
		glw::GLenum error_code = GL_NONE;

		gl.drawElementsBaseVertex(GL_TRIANGLES, -1,													 /* count */
								  GL_UNSIGNED_INT, (const glw::GLvoid*)m_draw_call_index_offset, 0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_VALUE)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_VALUE, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		gl.drawRangeElementsBaseVertex(GL_TRIANGLES, /* mode */
									   -1,			 /* start */
									   2,			 /* end */
									   1,			 /* count */
									   GL_UNSIGNED_INT, (const glw::GLvoid*)m_draw_call_index_offset,
									   0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_VALUE)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_VALUE, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		gl.drawElementsInstancedBaseVertex(GL_TRIANGLES, /* mode */
										   -1,			 /* count */
										   GL_UNSIGNED_INT, (const glw::GLvoid*)m_draw_call_index_offset,
										   1,  /* instancecount */
										   0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_VALUE)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_VALUE, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		if (m_is_ext_multi_draw_arrays_supported)
		{
			const glw::GLsizei count	 = -1;
			const glw::GLvoid* offsets[] = { (const glw::GLvoid*)m_draw_call_index_offset };

			gl.multiDrawElementsBaseVertex(GL_TRIANGLES,						/* mode */
										   &count, GL_UNSIGNED_INT, offsets, 1, /* primcount */
										   0);									/* basevertex */

			error_code = gl.getError();
			if (error_code != GL_INVALID_VALUE)
			{
				std::stringstream error_sstream;

				error_sstream << "Invalid error code reported for an invalid "
							  << getFunctionName(FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX) << " call";

				m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
								   << ": expected GL_INVALID_VALUE, got:"
									  "["
								   << error_code << "]" << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid error code reported for an invalid glMultiDrawElementsBaseVertexEXT() call.");
			}
		} /* if (m_is_ext_multi_draw_arrays_supported) */
	}	 /* for (all iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexNegativeInvalidInstanceCountArgumentTest::
	DrawElementsBaseVertexNegativeInvalidInstanceCountArgumentTest(Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "invalid_instancecount_argument",
									 "Tries to use invalid 'instancecount' argument values for "
									 "glDrawElementsInstancedBaseVertexEXT (ES) or "
									 "glDrawElementsInstancedBaseVertex (GL) draw call")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexNegativeInvalidInstanceCountArgumentTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* The test needs to be run in four iterations, where for each iteration we configure the VAO
	 * in a slightly different manner.
	 */
	for (int iteration = 0; iteration < 4; ++iteration)
	{
		bool use_clientside_index_data  = ((iteration & (1 << 0)) != 0);
		bool use_clientside_vertex_data = ((iteration & (1 << 1)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Set up the work environment */
		setUpNegativeTestObjects(use_clientside_vertex_data, use_clientside_index_data);

		/* Try to execute the invalid draw call */
		glw::GLenum error_code = GL_NONE;

		gl.drawElementsInstancedBaseVertex(GL_TRIANGLES, /* mode */
										   3,			 /* count */
										   GL_UNSIGNED_INT, (const glw::GLvoid*)m_draw_call_index_offset,
										   -1, /* instancecount */
										   0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_VALUE)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_VALUE, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}
	} /* for (all test iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexNegativeInvalidModeArgumentTest::DrawElementsBaseVertexNegativeInvalidModeArgumentTest(
	Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "invalid_mode_argument",
									 "Tries to use invalid 'mode' argument values for the \"base vertex\" draw calls")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexNegativeInvalidModeArgumentTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* The test needs to be run in four iterations, where for each iteration we configure the VAO
	 * in a slightly different manner.
	 */
	for (int iteration = 0; iteration < 4; ++iteration)
	{
		bool use_clientside_index_data  = ((iteration & (1 << 0)) != 0);
		bool use_clientside_vertex_data = ((iteration & (1 << 1)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Set up the work environment */
		setUpNegativeTestObjects(use_clientside_vertex_data, use_clientside_index_data);

		/* Try to execute the invalid draw calls */
		glw::GLenum error_code = GL_NONE;

		gl.drawElementsBaseVertex(GL_GREATER,														 /* mode */
								  3,																 /* count */
								  GL_UNSIGNED_INT, (const glw::GLvoid*)m_draw_call_index_offset, 0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_ENUM)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_ENUM, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		gl.drawRangeElementsBaseVertex(GL_GREATER, /* mode */
									   0,		   /* start */
									   2,		   /* end */
									   3,		   /* count */
									   GL_UNSIGNED_INT, (const glw::GLvoid*)m_draw_call_index_offset,
									   0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_ENUM)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_ENUM, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		gl.drawElementsInstancedBaseVertex(GL_GREATER, /* mode */
										   3,		   /* count */
										   GL_UNSIGNED_INT, (const glw::GLvoid*)m_draw_call_index_offset,
										   1,  /* instancecount */
										   0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_ENUM)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_ENUM, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		if (m_is_ext_multi_draw_arrays_supported)
		{
			const glw::GLsizei count	 = 3;
			const glw::GLvoid* offsets[] = { (const glw::GLvoid*)m_draw_call_index_offset };

			gl.multiDrawElementsBaseVertex(GL_GREATER,							/* mode */
										   &count, GL_UNSIGNED_INT, offsets, 1, /* primcount */
										   0);									/* basevertex */

			error_code = gl.getError();
			if (error_code != GL_INVALID_ENUM)
			{
				std::stringstream error_sstream;

				error_sstream << "Invalid error code reported for an invalid "
							  << getFunctionName(FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX) << " call";

				m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
								   << ": expected GL_INVALID_ENUM, got:"
									  "["
								   << error_code << "]" << tcu::TestLog::EndMessage;

				TCU_FAIL(error_sstream.str().c_str());
			}
		} /* if (m_is_ext_multi_draw_arrays_supported) */
	}	 /* for (all test iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexNegativeInvalidPrimcountArgumentTest::DrawElementsBaseVertexNegativeInvalidPrimcountArgumentTest(
	Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(
		  context, extParams, "invalid_primcount_argument",
		  "Tries to use invalid 'primcount' argument values for the \"base vertex\" draw calls")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexNegativeInvalidPrimcountArgumentTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* This test requires presence of GL_EXT_multi_draw_arrays under ES contexts */
	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		if (!m_is_ext_multi_draw_arrays_supported)
		{
			throw tcu::NotSupportedError("GL_EXT_multi_draw_arrays is not supported");
		}
	}

	/* The test needs to be run in four iterations, where for each iteration we configure the VAO
	 * in a slightly different manner.
	 */
	for (int iteration = 0; iteration < 4; ++iteration)
	{
		bool use_clientside_index_data  = ((iteration & (1 << 0)) != 0);
		bool use_clientside_vertex_data = ((iteration & (1 << 1)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Set up the work environment */
		setUpNegativeTestObjects(use_clientside_vertex_data, use_clientside_index_data);

		/* Perform the test */
		const glw::GLsizei count	  = 3;
		glw::GLenum		   error_code = GL_NO_ERROR;
		const glw::GLvoid* offsets[]  = { (const glw::GLvoid*)m_draw_call_index_offset };

		gl.multiDrawElementsBaseVertex(GL_TRIANGLES,						 /* mode */
									   &count, GL_UNSIGNED_INT, offsets, -1, /* primcount */
									   0);									 /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_VALUE)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_VALUE, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}
	} /* for (all test iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexNegativeInvalidStartEndArgumentsTest::DrawElementsBaseVertexNegativeInvalidStartEndArgumentsTest(
	Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "invalid_start_end_arguments",
									 "Tries to use invalid 'start' and 'end' argument values for the "
									 "glDrawRangeElementsBaseVertexEXT() (under ES) or "
									 "glDrawRangeElementsBaseVertex() (under GL) draw call")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexNegativeInvalidStartEndArgumentsTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* The test needs to be run in four iterations, where for each iteration we configure the VAO
	 * in a slightly different manner.
	 */
	for (int iteration = 0; iteration < 4; ++iteration)
	{
		bool use_clientside_index_data  = ((iteration & (1 << 0)) != 0);
		bool use_clientside_vertex_data = ((iteration & (1 << 1)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Set up the work environment */
		setUpNegativeTestObjects(use_clientside_vertex_data, use_clientside_index_data);

		/* Try to execute the invalid draw call */
		glw::GLenum error_code = GL_NONE;

		gl.drawRangeElementsBaseVertex(GL_TRIANGLES, /* mode */
									   3,			 /* start */
									   0,			 /* end */
									   3,			 /* count */
									   GL_UNSIGNED_INT, (const glw::GLvoid*)m_draw_call_index_offset,
									   0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_VALUE)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_VALUE, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}
	} /* for (all test iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
DrawElementsBaseVertexNegativeInvalidTypeArgumentTest::DrawElementsBaseVertexNegativeInvalidTypeArgumentTest(
	Context& context, const ExtParameters& extParams)
	: DrawElementsBaseVertexTestBase(context, extParams, "invalid_type_argument",
									 "Tries to use invalid 'type' argument values for the \"base vertex\" draw calls")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult DrawElementsBaseVertexNegativeInvalidTypeArgumentTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* The test needs to be run in four iterations, where for each iteration we configure the VAO
	 * in a slightly different manner.
	 */
	for (int iteration = 0; iteration < 4; ++iteration)
	{
		bool use_clientside_index_data  = ((iteration & (1 << 0)) != 0);
		bool use_clientside_vertex_data = ((iteration & (1 << 1)) != 0);

		/* OpenGL does not support client-side data. */
		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			if (use_clientside_index_data || use_clientside_vertex_data)
			{
				continue;
			}
		}

		/* Set up the work environment */
		setUpNegativeTestObjects(use_clientside_vertex_data, use_clientside_index_data);

		/* Try to execute the invalid draw calls */
		glw::GLenum error_code = GL_NONE;

		gl.drawElementsBaseVertex(GL_TRIANGLES,												 /* mode */
								  3,														 /* count */
								  GL_NONE, (const glw::GLvoid*)m_draw_call_index_offset, 0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_ENUM)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_ENUM, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		gl.drawRangeElementsBaseVertex(GL_TRIANGLES,											  /* mode */
									   0,														  /* start */
									   2,														  /* end */
									   3,														  /* count */
									   GL_NONE, (const glw::GLvoid*)m_draw_call_index_offset, 0); /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_ENUM)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_RANGE_ELEMENTS_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_ENUM, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		gl.drawElementsInstancedBaseVertex(GL_TRIANGLES,											 /* mode */
										   3,														 /* count */
										   GL_NONE, (const glw::GLvoid*)m_draw_call_index_offset, 1, /* instancecount */
										   0);														 /* basevertex */

		error_code = gl.getError();
		if (error_code != GL_INVALID_ENUM)
		{
			std::stringstream error_sstream;

			error_sstream << "Invalid error code reported for an invalid "
						  << getFunctionName(FUNCTION_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX) << " call";

			m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
							   << ": expected GL_INVALID_ENUM, got:"
								  "["
							   << error_code << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL(error_sstream.str().c_str());
		}

		if (m_is_ext_multi_draw_arrays_supported)
		{
			const glw::GLsizei count	 = 3;
			const glw::GLvoid* offsets[] = { (const glw::GLvoid*)m_draw_call_index_offset };

			gl.multiDrawElementsBaseVertex(GL_TRIANGLES,				/* mode */
										   &count, GL_NONE, offsets, 1, /* primcount */
										   0);							/* basevertex */

			error_code = gl.getError();
			if (error_code != GL_INVALID_ENUM)
			{
				std::stringstream error_sstream;

				error_sstream << "Invalid error code reported for an invalid "
							  << getFunctionName(FUNCTION_GL_MULTI_DRAW_ELEMENTS_BASE_VERTEX) << " call";

				m_testCtx.getLog() << tcu::TestLog::Message << error_sstream.str().c_str()
								   << ": expected GL_INVALID_ENUM, got:"
									  "["
								   << error_code << "]" << tcu::TestLog::EndMessage;

				TCU_FAIL(error_sstream.str().c_str());
			}
		} /* if (m_is_ext_multi_draw_arrays_supported) */
	}	 /* for (all test iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
DrawElementsBaseVertexTests::DrawElementsBaseVertexTests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "draw_elements_base_vertex_tests",
						"Contains conformance tests that verify ES and GL implementation's support "
						"for GL_EXT_draw_elements_base_vertex (ES) and "
						"GL_ARB_draw_elements_base_vertex (GL) extensions.")
{
}

/** Initializes the test group contents. */
void DrawElementsBaseVertexTests::init()
{
	addChild(new DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexFunctionalCorrectBaseVertexBehavior2(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorAEPShaderStages(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorUnderflow(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexFunctionalCorrectBaseVertexBehaviorOverflow(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexNegativeActiveTransformFeedbackTest(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexNegativeInvalidCountArgumentTest(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexNegativeInvalidInstanceCountArgumentTest(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexNegativeInvalidModeArgumentTest(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexNegativeInvalidPrimcountArgumentTest(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexNegativeInvalidStartEndArgumentsTest(m_context, m_extParams));
	addChild(new DrawElementsBaseVertexNegativeInvalidTypeArgumentTest(m_context, m_extParams));
}

} /* glcts namespace */
