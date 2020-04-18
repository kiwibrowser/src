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

#include "esextcTextureCubeMapArrayColorDepthAttachments.hpp"

#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{
/* Shader parts */
const glw::GLchar* const TextureCubeMapArrayColorDepthAttachmentsTest::m_fragment_shader_code =
	"${VERSION}\n"
	"/* FS */\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"in flat int fs_in_color;\n"
	"\n"
	"layout(location = 0) out int fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = fs_in_color;\n"
	"}\n"
	"\n";

const glw::GLchar* const TextureCubeMapArrayColorDepthAttachmentsTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"/* Layered GS */\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                         in;\n"
	"layout(triangle_strip, max_vertices=4) out;\n"
	"\n";

const glw::GLchar* const TextureCubeMapArrayColorDepthAttachmentsTest::m_geometry_shader_code_layered =
	"in  flat int vs_out_layer[];\n"
	"\n"
	"out flat int fs_in_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    int   layer = vs_out_layer[0];\n";

const glw::GLchar* const TextureCubeMapArrayColorDepthAttachmentsTest::m_geometry_shader_code_non_layered =
	"uniform  int uni_layer;\n"
	"\n"
	"out flat int fs_in_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    int   layer = uni_layer;\n";

const glw::GLchar* const TextureCubeMapArrayColorDepthAttachmentsTest::m_geometry_shader_code_body =
	";\n"
	"    \n"
	"    // Left-Bottom\n"
	"    gl_Position = vec4(-1.0, -1.0, depth, 1);\n"
	"    gl_Layer    = layer;\n"
	"    fs_in_color = layer;\n"
	"    EmitVertex();\n"
	"    \n"
	"    // Left-Top\n"
	"    gl_Position = vec4(-1.0,  1.0, depth, 1);\n"
	"    gl_Layer    = layer;\n"
	"    fs_in_color = layer;\n"
	"    EmitVertex();\n"
	"    \n"
	"    // Right-Bottom\n"
	"    gl_Position = vec4( 1.0, -1.0, depth, 1);\n"
	"    gl_Layer    = layer;\n"
	"    fs_in_color = layer;\n"
	"    EmitVertex();\n"
	"    \n"
	"    // Right-Top\n"
	"    gl_Position = vec4( 1.0,  1.0, depth, 1);\n"
	"    gl_Layer    = layer;\n"
	"    fs_in_color = layer;\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"}\n"
	"\n";

const glw::GLchar* const TextureCubeMapArrayColorDepthAttachmentsTest::m_vertex_shader_code =
	"${VERSION}\n"
	"/* VS */\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"flat out int vs_out_layer;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_PointSize = 1.0f;\n"
	"    vs_out_layer = gl_VertexID;\n"
	"}\n"
	"\n";

/* Static constants */
const glw::GLenum TextureCubeMapArrayColorDepthAttachmentsTest::m_color_internal_format = GL_R32I;
const glw::GLenum TextureCubeMapArrayColorDepthAttachmentsTest::m_color_format			= GL_RED_INTEGER;
const glw::GLenum TextureCubeMapArrayColorDepthAttachmentsTest::m_color_type			= GL_INT;
const glw::GLenum TextureCubeMapArrayColorDepthAttachmentsTest::m_depth_format			= GL_DEPTH_COMPONENT;

/** Verifies all texels in user-provided data buffer are equal to user-specified vector value.
 *
 * @tparam T            Type of image components
 * @tparam N_Components Number of image components
 *
 * @param  image_width  Width of image
 * @param  image_height Height of image
 * @param  components   Amount of components per texel
 * @param  image        Image data
 *
 * @return true if all texels are found valid, false otherwise.
 **/
template <typename T, unsigned int N_Components>
bool verifyImage(glw::GLuint image_width, glw::GLuint image_height, const T* components, const T* image)
{
	const glw::GLuint line_size = image_width * N_Components;

	for (glw::GLuint y = 0; y < image_height; ++y)
	{
		const glw::GLuint line_offset = y * line_size;

		for (glw::GLuint x = 0; x < image_width; ++x)
		{
			const glw::GLuint pixel_offset = line_offset + x * N_Components;

			for (glw::GLuint component = 0; component < N_Components; ++component)
			{
				if (image[pixel_offset + component] != components[component])
				{
					return false;
				}
			} /* for (all components) */
		}	 /* for (all columns) */
	}		  /* for (all rows) */

	return true;
}

/** Constructor
 *
 * @param size       Size of texture
 * @param n_cubemaps Number of cube-maps in array
 **/
TextureCubeMapArrayColorDepthAttachmentsTest::_texture_size::_texture_size(glw::GLuint size, glw::GLuint n_cubemaps)
	: m_size(size), m_n_cubemaps(n_cubemaps)
{
	/* Nothing to be done here */
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
TextureCubeMapArrayColorDepthAttachmentsTest::TextureCubeMapArrayColorDepthAttachmentsTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_vao_id(0)
	, m_color_texture_id(0)
	, m_depth_texture_id(0)
	, m_fragment_shader_id(0)
	, m_framebuffer_object_id(0)
	, m_layered_geometry_shader_id(0)
	, m_layered_program_id(0)
	, m_non_layered_geometry_shader_id(0)
	, m_non_layered_program_id(0)
	, m_non_layered_program_id_uni_layer_uniform_location(0)
	, m_vertex_shader_id(0)
	, m_depth_internal_format(0)
	, m_depth_type(0)
	, m_n_invalid_color_checks(0)
	, m_n_invalid_depth_checks(0)
{
	/* Define tested resolutions */
	m_resolutions.push_back(_texture_size(8, 8));
	m_resolutions.push_back(_texture_size(64, 3));
	m_resolutions.push_back(_texture_size(117, 1));
	m_resolutions.push_back(_texture_size(256, 1));
	m_resolutions.push_back(_texture_size(173, 3));
}

/** Attaches an user-specified texture object to zeroth color attachment OR depth attachment of
 *  test-maintained FBO in a layered manner.
 *
 * @param texture_id                     Texture object's ID.
 * @param should_use_as_color_attachment true to attach the texture object to GL_COLOR_ATTACHMENT0 of
 *                                       the test-maintained FBO; false to use GL_DEPTH_ATTACHMENT
 *                                       binding point.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::configureLayeredFramebufferAttachment(
	glw::GLuint texture_id, bool should_use_as_color_attachment)
{
	glw::GLenum			  attachment = GL_NONE;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	/* Determine which attachment should be used */
	if (true == should_use_as_color_attachment)
	{
		attachment = GL_COLOR_ATTACHMENT0;
	}
	else
	{
		attachment = GL_DEPTH_ATTACHMENT;
	}

	/* Re-bind the draw framebuffer, just in case. */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Update the FBO's attachment  */
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, attachment, texture_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureEXT() call failed.");
}

/** Attaches an user-specified texture object to zeroth color attachment OR depth attachment of
 *  test-maintained FBO in a non-layered manner.
 *
 * @param texture_id                     Texture object's ID.
 * @param n_layer                        Layer of the texture to attach.
 * @param should_use_as_color_attachment true to attach the texture object to GL_COLOR_ATTACHMENT0 of
 *                                       the test-maintained FBO; false to use GL_DEPTH_ATTACHMENT
 *                                       binding point.
 * @param should_update_draw_framebuffer true to bind the test-maintained FBO to GL_DRAW_FRAMEBUFFER
 *                                       binding point first, false to use GL_READ_FRAMEBUFFER binding
 *                                       point.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::configureNonLayeredFramebufferAttachment(
	glw::GLuint texture_id, glw::GLuint n_layer, bool should_use_as_color_attachment,
	bool should_update_draw_framebuffer)
{
	glw::GLenum			  attachment_type	= GL_NONE;
	glw::GLenum			  framebuffer_target = GL_NONE;
	const glw::Functions& gl				 = m_context.getRenderContext().getFunctions();

	/* Determine which attachment should be used */
	if (true == should_use_as_color_attachment)
	{
		attachment_type = GL_COLOR_ATTACHMENT0;
	}
	else
	{
		attachment_type = GL_DEPTH_ATTACHMENT;
	}

	/* Determine which framebuffer target should be used */
	if (true == should_update_draw_framebuffer)
	{
		framebuffer_target = GL_DRAW_FRAMEBUFFER;
	}
	else
	{
		framebuffer_target = GL_READ_FRAMEBUFFER;
	}

	/* Re-bind the framebuffer, just in case. */
	gl.bindFramebuffer(framebuffer_target, m_framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Use the specified texture layer as attachment */
	gl.framebufferTextureLayer(framebuffer_target, attachment_type, texture_id, 0 /* level */, n_layer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureLayer() call failed.");
}

/* Deinitializes GLES objects created during the test. */
void TextureCubeMapArrayColorDepthAttachmentsTest::deinit()
{
	/* Deinitialize base class */
	TestCaseBase::deinit();

	if (true != m_is_texture_cube_map_array_supported)
	{
		return;
	}
	if (true != m_is_geometry_shader_extension_supported)
	{
		return;
	}

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release texture objects */
	releaseAndDetachTextureObject(m_color_texture_id, true /* is_color_attachment */);
	releaseAndDetachTextureObject(m_depth_texture_id, false /* is_color_attachment */);

	/* Restore default state */
	gl.useProgram(0);
	gl.bindVertexArray(0);

	/* Delete all remaining ES objects the test may have created. */
	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);

		m_fragment_shader_id = 0;
	}

	if (0 != m_framebuffer_object_id)
	{
		gl.deleteFramebuffers(1, &m_framebuffer_object_id);

		m_framebuffer_object_id = 0;
	}

	if (0 != m_layered_geometry_shader_id)
	{
		gl.deleteShader(m_layered_geometry_shader_id);

		m_layered_geometry_shader_id = 0;
	}

	if (0 != m_layered_program_id)
	{
		gl.deleteProgram(m_layered_program_id);

		m_layered_program_id = 0;
	}

	if (0 != m_non_layered_geometry_shader_id)
	{
		gl.deleteShader(m_non_layered_geometry_shader_id);

		m_non_layered_geometry_shader_id = 0;
	}

	if (0 != m_non_layered_program_id)
	{
		gl.deleteProgram(m_non_layered_program_id);

		m_non_layered_program_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/* Determines depth internalformat that can be used for a draw framebuffer.
 * The result is stored in m_depth_internal_format and m_depth_type.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::determineSupportedDepthFormat()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Start with 16-bit depth internalformat */
	m_depth_internal_format = GL_DEPTH_COMPONENT16;
	m_depth_type			= GL_UNSIGNED_SHORT;

	while (true)
	{
		/* Create color and depth texture objectss */
		generateAndConfigureTextureObjects(8,	  /* texture_width */
										   1,	  /* n_cubemaps */
										   false); /* should_generate_mutable_textures */

		/* Set framebuffer attachments up */
		configureNonLayeredFramebufferAttachment(m_color_texture_id, 0 /* layer */, true /* is_color_attachment */,
												 true /* should_update_draw_framebuffer */);
		configureNonLayeredFramebufferAttachment(m_depth_texture_id, 0 /* layer */, false /* is_color_attachment */,
												 true /* should_update_draw_framebuffer */);

		/* Check framebuffer status */
		const glw::GLenum framebuffer_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

		if (GL_FRAMEBUFFER_COMPLETE == framebuffer_status)
		{
			return;
		}

		/* Current format does not work too well, try another one */
		switch (m_depth_internal_format)
		{
		case GL_DEPTH_COMPONENT16:
		{
			m_depth_internal_format = GL_DEPTH_COMPONENT24;
			m_depth_type			= GL_UNSIGNED_INT;

			break;
		}

		case GL_DEPTH_COMPONENT24:
		{
			m_depth_internal_format = GL_DEPTH_COMPONENT32F;
			m_depth_type			= GL_FLOAT;

			break;
		}

		case GL_DEPTH_COMPONENT32F:
		{
			throw tcu::NotSupportedError("Implementation does not support any known depth format");
		}

		default:
		{
			TCU_FAIL("Unrecognized depth internalformat");
		}
		} /* switch (m_depth_internal_format) */
	}	 /* while (true) */
}

/** Execute a draw call that renders (texture_size.m_n_cubemaps * 6) points.
 *  First, the viewport is configured to match the texture resolution and
 *  both color & depth buffers are cleared.
 *
 *  @param texture_size Render-target resolution.
 *
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::draw(const _texture_size& texture_size)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up the viewport */
	gl.viewport(0, /* x */
				0, /* y */
				texture_size.m_size, texture_size.m_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");

	/* Clear color & depth buffers */
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

	gl.drawArrays(GL_POINTS, 0 /* first */, texture_size.m_n_cubemaps * 6 /* layer-faces per cube-map */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "drawArrays");
}

/** Releases existing color & depth cube-map texture array objects, generates new
 *  ones and configures them as per user-specified properties.
 *
 *  @param texture_width                    Size to use for each layer-face's width and height.
 *  @param n_cubemaps                       Number of cube-maps to initialize for the cube-map texture arrays.
 *  @param should_generate_mutable_textures true if the texture should be initialized as mutable, false otherwise.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::generateAndConfigureTextureObjects(
	glw::GLuint texture_width, glw::GLuint n_cubemaps, bool should_generate_mutable_textures)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release any texture objects that may have already been initialized */
	releaseAndDetachTextureObject(m_color_texture_id, true /* is_color_attachment */);
	releaseAndDetachTextureObject(m_depth_texture_id, false /* is_color_attachment */);

	/* Generate texture objects */
	gl.genTextures(1, &m_color_texture_id);
	gl.genTextures(1, &m_depth_texture_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

	/* Configure new textures' storage */
	if (true == should_generate_mutable_textures)
	{
		prepareMutableTextureObject(m_color_texture_id, texture_width, n_cubemaps,
									true /* should_take_color_texture_properties */);
		prepareMutableTextureObject(m_depth_texture_id, texture_width, n_cubemaps,
									false /* should_take_color_texture_properties */);
	}
	else
	{
		prepareImmutableTextureObject(m_color_texture_id, texture_width, n_cubemaps,
									  true /* should_take_color_texture_properties */);
		prepareImmutableTextureObject(m_depth_texture_id, texture_width, n_cubemaps,
									  false /* should_take_color_texture_properties */);
	}
}

/* Initializes all ES objects needed to run the test */
void TextureCubeMapArrayColorDepthAttachmentsTest::initTest()
{
	const glw::GLchar*	depth_calculation_code = DE_NULL;
	const glw::Functions& gl					 = m_context.getRenderContext().getFunctions();

	/* Check if EXT_texture_cube_map_array extension is supported */
	if (true != m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED);
	}

	/* This test should only run if EXT_geometry_shader is supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Create a framebuffer object */
	gl.genFramebuffers(1, &m_framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramebuffers");

	/* Determine which depth format can be used as a depth attachment without
	 * making the FBO incomplete */
	determineSupportedDepthFormat();

	/* Decide which code snippet to use for depth value calculation */
	switch (m_depth_internal_format)
	{
	case GL_DEPTH_COMPONENT16:
	{
		depth_calculation_code = "-1.0 + float(2 * layer) / float(0xffff)";

		break;
	}

	case GL_DEPTH_COMPONENT24:
	{
		depth_calculation_code = "-1.0 + float(2 * layer) / float(0xffffff)";

		break;
	}

	case GL_DEPTH_COMPONENT32F:
	{
		depth_calculation_code = "-1.0 + float(2 * layer) / 256.0";

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized depth internal format");
	}
	} /* switch (m_depth_internal_format) */

	/* Create shader objects */
	m_fragment_shader_id			 = gl.createShader(GL_FRAGMENT_SHADER);
	m_layered_geometry_shader_id	 = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_non_layered_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id				 = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Create program objects */
	m_layered_program_id	 = gl.createProgram();
	m_non_layered_program_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call(s) failed");

	/* Build up an array of snippets making up bodies of two geometry shaders
	 * we'll be using for the test.
	 */
	const glw::GLchar* const layered_geometry_shader_parts[] = { m_geometry_shader_code_preamble,
																 m_geometry_shader_code_layered,
																 "    float depth = ", depth_calculation_code,
																 m_geometry_shader_code_body };

	const glw::GLchar* const non_layered_geometry_shader_parts[] = { m_geometry_shader_code_preamble,
																	 m_geometry_shader_code_non_layered,
																	 "    float depth = ", depth_calculation_code,
																	 m_geometry_shader_code_body };

	const glw::GLuint n_layered_geometry_shader_parts =
		sizeof(layered_geometry_shader_parts) / sizeof(layered_geometry_shader_parts[0]);
	const glw::GLuint n_non_layered_geometry_shader_parts =
		sizeof(non_layered_geometry_shader_parts) / sizeof(non_layered_geometry_shader_parts[0]);

	/* Build both programs */
	if (!buildProgram(m_layered_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code,
					  m_layered_geometry_shader_id, n_layered_geometry_shader_parts, layered_geometry_shader_parts,
					  m_vertex_shader_id, 1, &m_vertex_shader_code))
	{
		TCU_FAIL("Could not build layered-case program object");
	}

	if (!buildProgram(m_non_layered_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code,
					  m_non_layered_geometry_shader_id, n_non_layered_geometry_shader_parts,
					  non_layered_geometry_shader_parts, m_vertex_shader_id, 1, &m_vertex_shader_code))
	{
		TCU_FAIL("Could not build non-layered-case program object");
	}

	/* Get location of "uni_layer" uniform */
	m_non_layered_program_id_uni_layer_uniform_location = gl.getUniformLocation(m_non_layered_program_id, "uni_layer");

	if ((-1 == m_non_layered_program_id_uni_layer_uniform_location) || (GL_NO_ERROR != gl.getError()))
	{
		TCU_FAIL("Could not retrieve location of uni_layer uniform for non-layered program");
	}
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestCase::IterateResult TextureCubeMapArrayColorDepthAttachmentsTest::iterate()
{
	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize all ES objects needed to run the test */
	initTest();

	/* Setup clear values */
	gl.clearColor(0.0f /* red */, 0.0f /* green */, 0.0f /* blue */, 0.0f /* alpha */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor() call failed.");

	gl.clearDepthf(1.0f /* d */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearDepthf() call failed.");

	/* Enable depth test */
	gl.enable(GL_DEPTH_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable() call failed");

	/* Execute tests for each resolution */
	for (_texture_size_vector::iterator texture_size_iterator = m_resolutions.begin(),
										end_iterator		  = m_resolutions.end();
		 end_iterator != texture_size_iterator; ++texture_size_iterator)
	{
		testNonLayeredRendering(*texture_size_iterator, false);
		testNonLayeredRendering(*texture_size_iterator, true);
		testLayeredRendering(*texture_size_iterator, false);
		testLayeredRendering(*texture_size_iterator, true);
	}

	/* Test passes if there were no errors */
	if ((0 != m_n_invalid_color_checks) || (0 != m_n_invalid_depth_checks))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	/* Done */
	return STOP;
}

/** Takes a texture ID, binds it to GL_TEXTURE_CUBE_MAP_ARRAY texture target and
 *  initializes an immutable texture storage of @param texture_size x @param texture_size
 *  x (@param n_elements * 6) resolution.
 *
 * @param texture_id                           ID to use for the initialization.
 * @param texture_size                         Width & height to use for each layer-face.
 * @param n_cubemaps                           Amount of cube-maps to initialize.
 * @param should_take_color_texture_properties true if m_color_internal_format, m_color_format,
 *                                             m_color_type should be used for texture storage
 *                                             initialization, false to use relevant m_depth_*
 *                                             fields.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::prepareImmutableTextureObject(
	glw::GLuint texture_id, glw::GLuint texture_size, glw::GLuint n_cubemaps, bool should_take_color_texture_properties)
{
	const glw::Functions& gl			  = m_context.getRenderContext().getFunctions();
	glw::GLenum			  internal_format = GL_NONE;

	/* Set internal_format accordingly to requested texture type */
	if (true == should_take_color_texture_properties)
	{
		internal_format = m_color_internal_format;
	}
	else
	{
		internal_format = m_depth_internal_format;
	}

	/* Bind the texture object to GL_TEXTURE_CUBE_MAP_ARRAY texture target. */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	/* Initialize immutable texture storage as per description */
	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, /* n_mipmap_levels */
					internal_format, texture_size, texture_size, n_cubemaps * 6 /* layer-faces per cube-map */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");
}

/** Takes a texture ID, binds it to GL_TEXTURE_CUBE_MAP_ARRAY texture target and
 *  initializes a mutable texture storage of @param texture_size x @param texture_size
 *  x (@param n_elements * 6) resolution. Finally,the function sets GL_TEXTURE_MAX_LEVEL
 *  of the texture object to 0.
 *
 * @param texture_id                           ID to use for the initialization.
 * @param texture_size                         Width & height to use for each layer-face.
 * @param n_cubemaps                           Amount of cube-maps to initialize.
 * @param should_take_color_texture_properties true if m_color_internal_format, m_color_format,
 *                                             m_color_type should be used for texture storage
 *                                             initialization, false to use relevant m_depth_*
 *                                             fields.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::prepareMutableTextureObject(
	glw::GLuint texture_id, glw::GLuint texture_size, glw::GLuint n_cubemaps, bool should_take_color_texture_properties)
{
	const glw::Functions& gl			  = m_context.getRenderContext().getFunctions();
	glw::GLenum			  format		  = GL_NONE;
	glw::GLenum			  internal_format = GL_NONE;
	glw::GLenum			  type			  = GL_NONE;

	/* Set internal_format, format and type accordingly to requested texture type */
	if (true == should_take_color_texture_properties)
	{
		internal_format = m_color_internal_format;
		format			= m_color_format;
		type			= m_color_type;
	}
	else
	{
		internal_format = m_depth_internal_format;
		format			= m_depth_format;
		type			= m_depth_type;
	}

	/* Bind the texture object to GL_TEXTURE_CUBE_MAP_ARRAY texture target. */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	/* Initialize mutable texture storage as per description */
	gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0 /* mipmap_level */, internal_format, texture_size, texture_size,
				  n_cubemaps * 6 /* layer-faces per cube-map */, 0 /* border */, format, type,
				  DE_NULL); /* initial data */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D() call failed");

	/* Update GL_TEXTURE_MAX_LEVEL so that the texture is considered complete */
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, 0 /* param */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() call failed");
}

/** Releases a texture object and detaches it from test-maintained draw framebuffer.
 *
 * @param texture_id          Id of the texture object;
 * @param is_color_attachment true if the texture object described by id @param texture_id
 *                            is current draw framebuffer's color attachment, false if it's
 *                            a depth attachment.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::releaseAndDetachTextureObject(glw::GLuint texture_id,
																				 bool		 is_color_attachment)
{
	glw::GLenum			  attachment = GL_NONE;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	if (true == is_color_attachment)
	{
		attachment = GL_COLOR_ATTACHMENT0;
	}
	else
	{
		attachment = GL_DEPTH_ATTACHMENT;
	}

	/* Update draw framewbuffer binding just in case. */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Clean framebuffer's attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, 0, /* texture */
							0);												   /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	/* Unbind the texture object from GL_TEXTURE_CUBE_MAP_ARRAY binding point */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	/* Finally delete the texture object */
	gl.deleteTextures(1, &texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");
}

/** Verifies layered rendering works correctly.
 *
 * @param texture_size                Resolution of texture;
 * @param should_use_mutable_textures true if mutable textures should be used for the test,
 *                                    false to use immutable textures.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::testLayeredRendering(const _texture_size& texture_size,
																		bool should_use_mutable_textures)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture objects for the test */
	generateAndConfigureTextureObjects(texture_size.m_size, texture_size.m_n_cubemaps, should_use_mutable_textures);

	/* Setup layered framebuffer */
	configureLayeredFramebufferAttachment(m_color_texture_id, true /* should_use_as_color_attachment */);
	configureLayeredFramebufferAttachment(m_depth_texture_id, false /* should_use_as_color_attachment */);

	/* Activate the program object that performs layered rendering */
	gl.useProgram(m_layered_program_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Issue the draw call. */
	draw(texture_size);

	/* Restore default framebuffer attachments */
	configureLayeredFramebufferAttachment(0 /* texture_id */, true /* should_use_as_color_attachment */);
	configureLayeredFramebufferAttachment(0 /* texture_id */, false /* should_use_as_color_attachment */);

	/* Restore draw framebuffer binding */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Time to verify the results - update read framebuffer binding first. */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Iterate through all layer-faces */
	for (glw::GLuint n_layer_face = 0; n_layer_face < texture_size.m_n_cubemaps * 6 /* layer-faces per cube-map */;
		 ++n_layer_face)
	{
		/* Configure read framebuffer attachments to point to the layer of our current interest */
		configureNonLayeredFramebufferAttachment(m_color_texture_id, n_layer_face,
												 true,   /* should_use_as_color_attachment */
												 false); /* should_update_draw_framebuffer */
		configureNonLayeredFramebufferAttachment(m_depth_texture_id, n_layer_face,
												 false,  /* should_use_as_color_attachment */
												 false); /* should_update_draw_framebuffer */

		/* Verify contents of color and depth attachments */
		bool is_color_data_ok = verifyColorData(texture_size, n_layer_face);
		bool is_depth_data_ok = false;

		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			switch (m_depth_internal_format)
			{
			case GL_DEPTH_COMPONENT16:
			{
				is_depth_data_ok = verifyDepth16Data(texture_size, n_layer_face);

				break;
			}

			case GL_DEPTH_COMPONENT24:
			{
				is_depth_data_ok = verifyDepth24Data(texture_size, n_layer_face);

				break;
			}

			case GL_DEPTH_COMPONENT32F:
			{
				is_depth_data_ok = verifyDepth32FData(texture_size, n_layer_face);

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized depth internalformat");
			}
			} /* switch (m_depth_internal_format) */
		}
		else
		{
			is_depth_data_ok = true;
		}

		/* Any errors? Increment relevant counters */
		if (false == is_color_data_ok)
		{
			m_n_invalid_color_checks++;
		}

		if (false == is_depth_data_ok)
		{
			m_n_invalid_depth_checks++;
		}
	} /* for (all layer-faces) */
}

/** Verifies layered rendering works correctly.
 *
 * @param texture_size               Resolution of texture
 * @param should_use_mutable_texture true if an immutable texture should be used for
 *                                   the invocation; false if mutable.
 **/
void TextureCubeMapArrayColorDepthAttachmentsTest::testNonLayeredRendering(const _texture_size& texture_size,
																		   bool should_use_mutable_texture)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Activate a program object that renders in a non-layered fashion */
	gl.useProgram(m_non_layered_program_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Create relevant textures */
	generateAndConfigureTextureObjects(texture_size.m_size, texture_size.m_n_cubemaps, should_use_mutable_texture);

	/* Iterate over all layer-faces */
	for (glw::GLuint n_layer_face = 0; n_layer_face < texture_size.m_n_cubemaps * 6 /* layer-faces per cube-map */;
		 ++n_layer_face)
	{
		/* Set up non-layered framebuffer attachments */
		configureNonLayeredFramebufferAttachment(m_color_texture_id, n_layer_face, true /* is_color_attachment */);
		configureNonLayeredFramebufferAttachment(m_depth_texture_id, n_layer_face, false /* is_color_attachment */);

		/* Update value assigned to "uni_layer" uniform */
		gl.uniform1i(m_non_layered_program_id_uni_layer_uniform_location, n_layer_face);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");

		/* Execute a draw call */
		draw(texture_size);

		/* Restore default framebuffer attachments */
		configureNonLayeredFramebufferAttachment(0 /* texture_id */, 0 /* n_layer */,
												 true /* should_use_as_color_attachment */);
		configureNonLayeredFramebufferAttachment(0 /* texture_id */, 0 /* n_layer */,
												 false /* should_use_as_color_attachment */);

		/* Remove draw framebuffer binding */
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

		/* Verify the results. First, make sure the read framebuffer binding is configured
		 * accordingly.
		 */
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

		/* Configure read framebuffer attachments to point to the layer of our current interest */
		configureNonLayeredFramebufferAttachment(m_color_texture_id, n_layer_face,
												 true,   /* should_use_as_color_attachment */
												 false); /* should_update_draw_framebuffer */
		configureNonLayeredFramebufferAttachment(m_depth_texture_id, n_layer_face,
												 false,  /* should_use_as_color_attachment */
												 false); /* should_update_draw_framebuffer */

		/* Verify contents of color and depth attachments */
		bool is_color_data_ok = verifyColorData(texture_size, n_layer_face);
		bool is_depth_data_ok = false;

		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			switch (m_depth_internal_format)
			{
			case GL_DEPTH_COMPONENT16:
			{
				is_depth_data_ok = verifyDepth16Data(texture_size, n_layer_face);

				break;
			}

			case GL_DEPTH_COMPONENT24:
			{
				is_depth_data_ok = verifyDepth24Data(texture_size, n_layer_face);

				break;
			}

			case GL_DEPTH_COMPONENT32F:
			{
				is_depth_data_ok = verifyDepth32FData(texture_size, n_layer_face);

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized depth internalformat");
			}
			} /* switch (m_depth_internal_format) */
		}
		else
		{
			is_depth_data_ok = true;
		}

		/* Any errors? Increment relevant counters */
		if (false == is_color_data_ok)
		{
			m_n_invalid_color_checks++;
		}

		if (false == is_depth_data_ok)
		{
			m_n_invalid_depth_checks++;
		}
	} /* for (all layer-faces) */
}

/** Reads read buffer's color data and verifies its correctness.
 *
 *  @param texture_size Texture size
 *  @param n_layer      Index of the layer to verify.
 *
 *  @return true if the retrieved data was found correct, false otherwise.
 **/
bool TextureCubeMapArrayColorDepthAttachmentsTest::verifyColorData(const _texture_size& texture_size,
																   glw::GLuint			n_layer)
{
	/* Allocate buffer for the data we will retrieve from the implementation */
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	bool				  result		   = false;
	const glw::GLuint	 result_data_size = texture_size.m_size * texture_size.m_size * 4;
	glw::GLuint*		  result_data	  = new glw::GLuint[result_data_size];

	DE_ASSERT(result_data != NULL);

	/* Read the data */
	gl.readPixels(0, /* x */
				  0, /* y */
				  texture_size.m_size, texture_size.m_size, GL_RGBA_INTEGER, m_color_type, result_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

	glw::GLuint expected[4] = { n_layer, 0, 0, 1 };

	/* Verify image, expected value is layer index */
	result = verifyImage<glw::GLuint, 4>(texture_size.m_size, texture_size.m_size, expected, result_data);

	/* Release the buffer */
	if (result_data != NULL)
	{
		delete[] result_data;

		result_data = NULL;
	}

	return result;
}

/** Reads read buffer's depth data (assuming it's of 16-bit resolution)
 *  and verifies its correctness.
 *
 *  @param texture_size Texture size
 *  @param n_layer      Index of the layer to verify.
 *
 *  @return true if the retrieved data was found correct, false otherwise.
 **/
bool TextureCubeMapArrayColorDepthAttachmentsTest::verifyDepth16Data(const _texture_size& texture_size,
																	 glw::GLuint		  n_layer)
{
	/* Allocate buffer for the data we will retrieve from the implementation */
	glw::GLushort		  expected_value   = (glw::GLushort)n_layer;
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	bool				  result		   = false;
	const glw::GLuint	 result_data_size = texture_size.m_size * texture_size.m_size;
	glw::GLushort*		  result_data	  = new glw::GLushort[result_data_size];

	DE_ASSERT(result_data != NULL);

	gl.pixelStorei(GL_PACK_ALIGNMENT, 2);

	/* Read the data */
	gl.readPixels(0, /* x */
				  0, /* y */
				  texture_size.m_size, texture_size.m_size, m_depth_format, m_depth_type, result_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

	/* Verify image, expected value is layer index */
	result = verifyImage<glw::GLushort, 1>(texture_size.m_size, texture_size.m_size, &expected_value, result_data);

	/* Release the buffer */
	if (result_data != NULL)
	{
		delete[] result_data;

		result_data = NULL;
	}

	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);

	return result;
}

/** Reads read buffer's depth data (assuming it's of 24-bit resolution)
 *  and verifies its correctness.
 *
 *  @param texture_size Texture size
 *  @param n_layer      Index of the layer to verify.
 *
 *  @return true if the retrieved data was found correct, false otherwise.
 **/
bool TextureCubeMapArrayColorDepthAttachmentsTest::verifyDepth24Data(const _texture_size& texture_size,
																	 glw::GLuint		  n_layer)
{
	/* Allocate buffer for the data we will retrieve from the implementation */
	glw::GLuint			  expected_value   = n_layer << 8;
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	bool				  result		   = false;
	const glw::GLuint	 result_data_size = texture_size.m_size * texture_size.m_size;
	glw::GLuint*		  result_data	  = new glw::GLuint[result_data_size];

	DE_ASSERT(result_data != NULL);

	/* Read the data */
	gl.readPixels(0, /* x */
				  0, /* y */
				  texture_size.m_size, texture_size.m_size, m_depth_format, m_depth_type, result_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

	/* Verify image, expected value is layer index */
	result = verifyImage<glw::GLuint, 1>(texture_size.m_size, texture_size.m_size, &expected_value, result_data);

	/* Release the buffer */
	if (result_data != NULL)
	{
		delete[] result_data;

		result_data = NULL;
	}

	return result;
}

/** Reads read buffer's depth data (assuming it's of 32-bit FP resolution)
 *  and verifies its correctness.
 *
 *  @param texture_size Texture size
 *  @param n_layer      Index of the layer to verify.
 *
 *  @return true if the retrieved data was found correct, false otherwise.
 **/
bool TextureCubeMapArrayColorDepthAttachmentsTest::verifyDepth32FData(const _texture_size& texture_size,
																	  glw::GLuint		   n_layer)
{
	/* Allocate buffer for the data we will retrieve from the implementation */
	glw::GLfloat		  expected_value   = (glw::GLfloat)n_layer / 256.0f;
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	bool				  result		   = false;
	const glw::GLuint	 result_data_size = texture_size.m_size * texture_size.m_size;
	glw::GLfloat*		  result_data	  = new glw::GLfloat[result_data_size];

	DE_ASSERT(result_data != NULL);

	/* Read the data */
	gl.readPixels(0, /* x */
				  0, /* y */
				  texture_size.m_size, texture_size.m_size, m_depth_format, m_depth_type, result_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

	/* Verify image, expected value is layer index */
	result = verifyImage<glw::GLfloat, 1>(texture_size.m_size, texture_size.m_size, &expected_value, result_data);

	/* Release the buffer */
	if (result_data != NULL)
	{
		delete[] result_data;

		result_data = NULL;
	}

	return result;
}

} /* glcts */
