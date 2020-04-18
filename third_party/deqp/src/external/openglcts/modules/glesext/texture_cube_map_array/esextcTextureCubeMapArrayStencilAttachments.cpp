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
 * \file  esextcTextureCubeMapArrayStencilAttachments.cpp
 * \brief Texture Cube Map Array Stencil Attachments (Test 3)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayStencilAttachments.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <string.h>

namespace glcts
{

/** Number of byte for one vec4 position */
const glw::GLuint TextureCubeMapArrayStencilAttachments::m_n_components = 4;
/** Number of configuration different cube map array textures*/
const glw::GLuint TextureCubeMapArrayStencilAttachments::m_n_cube_map_array_configurations = 4;
/** Number of vertices per triangle use in gemoetry shader*/
const glw::GLuint TextureCubeMapArrayStencilAttachments::m_n_vertices_gs = 3;

/** Constructor **/
CubeMapArrayDataStorage::CubeMapArrayDataStorage() : m_data_array(DE_NULL), m_depth(0), m_height(0), m_width(0)
{
}

/** Destructor **/
CubeMapArrayDataStorage::~CubeMapArrayDataStorage()
{
	deinit();
}

/** Initializes data array
 *
 *   @param width         width of the texture;
 *   @param height        height of the texture;
 *   @param depth         number of layers in the texture (for cube map array must be multiple of 6);
 *   @param initial_value value which the allocated storage should be cleared with;
 **/
void CubeMapArrayDataStorage::init(const glw::GLuint width, const glw::GLuint height, const glw::GLuint depth,
								   glw::GLubyte initial_value)
{
	deinit();

	m_width  = width;
	m_height = height;
	m_depth  = depth;

	m_data_array = new glw::GLubyte[getArraySize()];

	memset(m_data_array, initial_value, getArraySize() * sizeof(glw::GLubyte));
}

/** Deinitializes data array **/
void CubeMapArrayDataStorage::deinit(void)
{
	if (m_data_array != DE_NULL)
	{
		delete[] m_data_array;

		m_data_array = DE_NULL;
	}

	m_width  = 0;
	m_height = 0;
	m_depth  = 0;
}

/** Returns pointer to array.
 *
 *  @return As per description.
 **/
glw::GLubyte* CubeMapArrayDataStorage::getDataArray() const
{
	return m_data_array;
}

/** Returns size of the array in bytes **/
glw::GLuint CubeMapArrayDataStorage::getArraySize() const
{
	return m_width * m_height * m_depth * TextureCubeMapArrayStencilAttachments::m_n_components;
}

/* Fragment shader code */
const char* TextureCubeMapArrayStencilAttachments::m_fragment_shader_code = "${VERSION}\n"
																			"\n"
																			"${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
																			"\n"
																			"precision highp float;\n"
																			"\n"
																			"layout(location = 0) out vec4 color;\n"
																			"\n"
																			"void main()\n"
																			"{\n"
																			"    color = vec4(0, 1, 1, 0);\n"
																			"}\n";

/* Vertex shader code */
const char* TextureCubeMapArrayStencilAttachments::m_vertex_shader_code = "${VERSION}\n"
																		  "\n"
																		  "${TEXTURE_CUBE_MAP_ARRAY_REQUIRE}\n"
																		  "\n"
																		  "precision highp float;\n"
																		  "\n"
																		  "in vec4 vertex_position;\n"
																		  "\n"
																		  "void main()\n"
																		  "{\n"
																		  "    gl_Position = vertex_position;\n"
																		  "}\n";

/** Constructor
 *
 *  @param context            Test context
 *  @param name               Test case's name
 *  @param description        Test case's description
 *  @param immutable_storage  if set to true, immutable textures will be used;
 *  @param fbo_layered        if set to true, a layered draw framebuffer will be used;
 **/
TextureCubeMapArrayStencilAttachments::TextureCubeMapArrayStencilAttachments(Context&			  context,
																			 const ExtParameters& extParams,
																			 const char* name, const char* description,
																			 glw::GLboolean immutable_storage,
																			 glw::GLboolean fbo_layered)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_layered(fbo_layered)
	, m_immutable_storage(immutable_storage)
	, m_fbo_draw_id(0)
	, m_fbo_read_id(0)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_id(0)
	, m_texture_cube_array_stencil_id(0)
	, m_texture_cube_array_color_id(0)
	, m_vao_id(0)
	, m_vbo_id(0)
	, m_vertex_shader_id(0)
	, m_cube_map_array_data(DE_NULL)
	, m_result_data(DE_NULL)
{
	/* Nothing to be done here */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TextureCubeMapArrayStencilAttachments::iterate(void)
{
	initTest();

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up stencil function */
	gl.stencilFunc(GL_LESS, 0 /* ref */, 0xFF /* mask */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not setup stencil function");

	/* Set up stencil operation */
	gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not setup stencil operation");

	/* Set up clear color */
	gl.clearColor(1 /* red */, 0 /* green */, 0 /* blue */, 1 /* alpha */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set stencil color value!");

	bool has_test_failed = false;

	/* Iterate through all defined configurations */
	for (glw::GLuint test_index = 0; test_index < m_n_cube_map_array_configurations; test_index++)
	{
		/* Build and activate a test-specific program object */
		buildAndUseProgram(test_index);

		/* Create textures, framebuffer... before for every test iteration */
		initTestIteration(test_index);

		/* Clear the color buffer with (1, 0, 0, 1) color */
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not clear color buffer");

		/* Draw a full-screen quad */
		gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

		/* Read the rendered data */
		glw::GLuint array_size =
			m_cube_map_array_data[test_index].getArraySize() / m_cube_map_array_data[test_index].getDepth();

		m_result_data = new glw::GLubyte[array_size];

		memset(m_result_data, 0, sizeof(glw::GLubyte) * array_size);

		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind read framebuffer");

		if (m_fbo_layered)
		{
			for (glw::GLuint n_layer = 0; n_layer < m_cube_map_array_data[test_index].getDepth(); ++n_layer)
			{
				gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture_cube_array_color_id,
										   0, /* level */
										   n_layer);
				GLU_EXPECT_NO_ERROR(gl.getError(),
									"Could not attach texture layer to color attachment of read framebuffer");

				/* Is the data correct? */
				has_test_failed = readPixelsAndCompareWithExpectedResult(test_index);

				if (has_test_failed)
				{
					break;
				}
			} /* for (all layers) */
		}	 /* if (m_fbo_layered) */
		else
		{
			gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture_cube_array_color_id,
									   0 /* level */, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(),
								"Could not attach texture layer to color attachment of read framebuffer");

			/* Is the data correct? */
			has_test_failed = readPixelsAndCompareWithExpectedResult(test_index);
		}

		/* Clean up */
		cleanAfterTest();

		if (has_test_failed)
		{
			break;
		}
	} /* for (all configurations) */

	if (has_test_failed)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureCubeMapArrayStencilAttachments::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Disable the stencil test */
	gl.disable(GL_STENCIL_TEST);

	/* If any of the iterations have broken, we should clean up here. */
	cleanAfterTest();

	gl.bindVertexArray(0);

	/* Release test-wide objects */
	if (m_vbo_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_id);

		m_vbo_id = 0;
	}

	if (m_fbo_draw_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_draw_id);

		m_fbo_draw_id = 0;
	}

	if (m_fbo_read_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_read_id);

		m_fbo_read_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_cube_map_array_data != DE_NULL)
	{
		delete[] m_cube_map_array_data;

		m_cube_map_array_data = DE_NULL;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Build and use a program object **/
void TextureCubeMapArrayStencilAttachments::buildAndUseProgram(glw::GLuint test_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create a program object */
	m_program_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a program object");

	/* Create shader objects that will make up the program object */
	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a fragment shader object");

	m_vertex_shader_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a vertex shader object");

	if (m_fbo_layered)
	{
		std::string		  geometry_shader_code;
		const char*		  geometry_shader_code_ptr = DE_NULL;
		std::stringstream max_vertices_sstream;
		std::stringstream n_iterations_sstream;

		max_vertices_sstream << m_cube_map_array_data[test_index].getDepth() * m_n_vertices_gs;
		n_iterations_sstream << m_cube_map_array_data[test_index].getDepth();

		geometry_shader_code	 = getGeometryShaderCode(max_vertices_sstream.str(), n_iterations_sstream.str());
		geometry_shader_code_ptr = geometry_shader_code.c_str();

		m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a geometry shader object");

		/* Build a program object */
		if (!buildProgram(m_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code, m_geometry_shader_id, 1,
						  &geometry_shader_code_ptr, m_vertex_shader_id, 1, &m_vertex_shader_code))
		{
			TCU_FAIL("Program could not have been created sucessfully from valid vertex/geometry/fragment shaders");
		}
	} /* if (m_fbo_layered) */
	else
	{
		/* Build a program object */
		if (!buildProgram(m_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code, m_vertex_shader_id, 1,
						  &m_vertex_shader_code))
		{
			TCU_FAIL("Program could not have been created sucessfully from valid vertex/fragment shaders");
		}
	}

	/* Use program */
	glw::GLint posAttrib = -1;

	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	posAttrib = gl.getAttribLocation(m_program_id, "vertex_position");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get attribute location!");

	if (posAttrib == -1)
	{
		TCU_FAIL("vertex_position attribute is considered inactive");
	}

	gl.vertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure vertex_position vertex attribute array");

	gl.enableVertexAttribArray(posAttrib);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable vertex attribute array!");
}

/** Check Framebuffer Status. Throws a TestError exception, should the framebuffer status
 *  be found incomplete.
 *
 *  @param framebuffer_status FBO status, as returned by glCheckFramebufferStatus(), to check.
 *
 */
void TextureCubeMapArrayStencilAttachments::checkFramebufferStatus(glw::GLenum framebuffer_status)
{
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

/** Clean after test **/
void TextureCubeMapArrayStencilAttachments::cleanAfterTest(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
	gl.useProgram(0);

	if (m_program_id != 0)
	{
		gl.deleteProgram(m_program_id);

		m_program_id = 0;
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);

		m_fragment_shader_id = 0;
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);

		m_geometry_shader_id = 0;
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}

	if (m_texture_cube_array_color_id != 0)
	{
		gl.deleteTextures(1, &m_texture_cube_array_color_id);

		m_texture_cube_array_color_id = 0;
	}

	if (m_texture_cube_array_stencil_id != 0)
	{
		gl.deleteTextures(1, &m_texture_cube_array_stencil_id);

		m_texture_cube_array_stencil_id = 0;
	}

	if (m_result_data != DE_NULL)
	{
		delete[] m_result_data;

		m_result_data = DE_NULL;
	}
}

/** Create an immutable texture storage for a color texture.
 *
 *  @param test_index number of the test configuration to use texture properties from.
 **/
void TextureCubeMapArrayStencilAttachments::createImmutableCubeArrayColor(glw::GLuint test_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY,					   /* target */
					1,											   /* levels */
					GL_RGBA8,									   /* internalformat */
					m_cube_map_array_data[test_index].getWidth(),  /* width */
					m_cube_map_array_data[test_index].getHeight(), /* height */
					m_cube_map_array_data[test_index].getDepth()); /* depth */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create an immutable color texture storage!");
}

/** Create an immutable texture storage for a stencil texture.
 *
 *  @param test_index number of the test configuration to use texture properties from.
 **/
void TextureCubeMapArrayStencilAttachments::createImmutableCubeArrayStencil(glw::GLuint test_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	fillStencilData(test_index);

	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY,					   /* target */
					1,											   /* levels */
					GL_DEPTH24_STENCIL8,						   /* internalformat */
					m_cube_map_array_data[test_index].getWidth(),  /* width */
					m_cube_map_array_data[test_index].getHeight(), /* height */
					m_cube_map_array_data[test_index].getDepth()); /* depth */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create an immutable stencil texture storage!");

	gl.texSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,							/* target */
					 0,													/* level */
					 0,													/* xoffset */
					 0,													/* yoffset */
					 0,													/* zoffset */
					 m_cube_map_array_data[test_index].getWidth(),		/* width */
					 m_cube_map_array_data[test_index].getHeight(),		/* height */
					 m_cube_map_array_data[test_index].getDepth(),		/* depth */
					 GL_DEPTH_STENCIL,									/* format */
					 GL_UNSIGNED_INT_24_8,								/* type */
					 m_cube_map_array_data[test_index].getDataArray()); /* data */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not fill allocated texture storage with texture data!");
}

/** Create a mutable texture storage for a color texture.
 *
 *  @param test_index number of the test configuration to use texture properties from.
 *
 **/
void TextureCubeMapArrayStencilAttachments::createMutableCubeArrayColor(glw::GLuint test_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,						 /* target */
				  0,												 /* level */
				  GL_RGBA8,											 /* internal format */
				  m_cube_map_array_data[test_index].getWidth(),		 /* width */
				  m_cube_map_array_data[test_index].getHeight(),	 /* height */
				  m_cube_map_array_data[test_index].getDepth(),		 /* depth */
				  0,												 /* border */
				  GL_RGBA,											 /* format */
				  GL_UNSIGNED_BYTE,									 /* type */
				  m_cube_map_array_data[test_index].getDataArray()); /* pixel data */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a mutable color texture storage!");
}

/** Create a mutable texture storage for a stencil texture.
 *
 *  @param test_index number of the test configuration to use texture properties from.
 **/
void TextureCubeMapArrayStencilAttachments::createMutableCubeArrayStencil(glw::GLuint test_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	fillStencilData(test_index);

	gl.texImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,						 /* target */
				  0,												 /* level */
				  GL_DEPTH24_STENCIL8,								 /* internal format */
				  m_cube_map_array_data[test_index].getWidth(),		 /* width */
				  m_cube_map_array_data[test_index].getHeight(),	 /* height */
				  m_cube_map_array_data[test_index].getDepth(),		 /* depth */
				  0,												 /* border */
				  GL_DEPTH_STENCIL,									 /* format */
				  GL_UNSIGNED_INT_24_8,								 /* type */
				  m_cube_map_array_data[test_index].getDataArray()); /* pixel data */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a mutable stencil texture storage!");
}

/** Fill the texture used as stencil attachment with reference values
 *
 *  @param test_index index of the test configuration this call is being made for.
 *
 **/
void TextureCubeMapArrayStencilAttachments::fillStencilData(glw::GLuint test_index)
{
	glw::GLubyte* const data = m_cube_map_array_data[test_index].getDataArray();

	memset(data, 0, m_cube_map_array_data[test_index].getArraySize() * sizeof(glw::GLubyte));

	for (glw::GLuint depth_index = 0; depth_index < m_cube_map_array_data[test_index].getDepth(); depth_index++)
	{
		for (glw::GLuint x = 0; x < m_cube_map_array_data[test_index].getWidth(); x++)
		{
			for (glw::GLuint y = m_cube_map_array_data[test_index].getHeight() / 2;
				 y < m_cube_map_array_data[test_index].getHeight(); y++)
			{
				glw::GLuint depth_position_in_array = m_cube_map_array_data[test_index].getWidth() *
													  m_cube_map_array_data[test_index].getHeight() * m_n_components *
													  depth_index;

				glw::GLuint y_position_in_array = m_cube_map_array_data[test_index].getHeight() * m_n_components * y;

				glw::GLuint x_position_in_array = m_n_components * x;

				glw::GLuint index_array = depth_position_in_array + y_position_in_array + x_position_in_array;

				memset((data + index_array), 1, m_n_components);

			} /* for (all rows) */
		}	 /* for (all columns) */
	}		  /* for (all layers) */
}

/** Returns a geometry shader code, adapted to user-specific values.
 *
 *  @param max_vertices  String storing maximum amount of vertices that geometry shader can output;
 *  @param n_layers      String storing number of layer-faces in cube map array texture;
 */
std::string TextureCubeMapArrayStencilAttachments::getGeometryShaderCode(const std::string& max_vertices,
																		 const std::string& n_layers)
{
	/* Geometry shader template code */
	std::string m_geometry_shader_template = "${VERSION}\n"
											 "\n"
											 "${GEOMETRY_SHADER_REQUIRE}\n"
											 "\n"
											 "layout(triangles)                                       in;\n"
											 "layout(triangle_strip, max_vertices = <-MAX-VERTICES->) out;\n"
											 "\n"
											 "void main(void)\n"
											 "{\n"
											 "   int layer;\n"
											 "\n"
											 "   for (layer = 0; layer < <-N_LAYERS->; layer++)\n"
											 "   {\n"
											 "       gl_Layer    = layer;\n"
											 "       gl_Position = gl_in[0].gl_Position;\n"
											 "       EmitVertex();\n"
											 "\n"
											 "       gl_Layer    = layer;\n"
											 "       gl_Position = gl_in[1].gl_Position;\n"
											 "       EmitVertex();\n"
											 "\n"
											 "       gl_Layer    = layer;\n"
											 "       gl_Position = gl_in[2].gl_Position;\n"
											 "       EmitVertex();\n"
											 "\n"
											 "       EndPrimitive();\n"
											 "   }\n"
											 "}\n";

	/* Replace a "maximum number of emitted vertices" token with user-provided value */
	std::string template_name	 = "<-MAX-VERTICES->";
	std::size_t template_position = m_geometry_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_geometry_shader_template =
			m_geometry_shader_template.replace(template_position, template_name.length(), max_vertices);

		template_position = m_geometry_shader_template.find(template_name);
	}

	/* Do the same for the number of layers we want the geometry shader to modify. */
	template_name	 = "<-N_LAYERS->";
	template_position = m_geometry_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_geometry_shader_template =
			m_geometry_shader_template.replace(template_position, template_name.length(), n_layers);

		template_position = m_geometry_shader_template.find(template_name);
	}
	if (!m_is_geometry_shader_extension_supported && m_fbo_layered)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	return m_geometry_shader_template;
}

/** Initializes GLES objects created during the test. **/
void TextureCubeMapArrayStencilAttachments::initTest(void)
{
	/* Check if EXT_texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}
	if (!m_is_geometry_shader_extension_supported && m_fbo_layered)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create 4 different configurations of cube map array texture */
	m_cube_map_array_data = new CubeMapArrayDataStorage[m_n_cube_map_array_configurations];

	m_cube_map_array_data[0].init(64, 64, 18);
	m_cube_map_array_data[1].init(117, 117, 6);
	m_cube_map_array_data[2].init(256, 256, 6);
	m_cube_map_array_data[3].init(173, 173, 12);

	/* full screen square */
	glw::GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
								1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f };

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");

	gl.genBuffers(1, &m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate buffer object!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set data for buffer object!");

	/* Create and configure framebuffer object */
	gl.genFramebuffers(1, &m_fbo_read_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate framebuffer object!");

	gl.genFramebuffers(1, &m_fbo_draw_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate framebuffer object!");

	gl.enable(GL_STENCIL_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable stencil test!");
}

/** Create and set OpenGL object need for one test iteration
 *
 *  @param test_index number of the test configuration to use texture properties from.
 **/
void TextureCubeMapArrayStencilAttachments::initTestIteration(glw::GLuint test_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up texture storage for color data */
	gl.genTextures(1, &m_texture_cube_array_color_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate texture object!");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture_cube_array_color_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a texture object to GL_TEXTURE_CUBE_MAP_ARRAY_EXT target");

	if (m_immutable_storage)
	{
		createImmutableCubeArrayColor(test_index);
	}
	else
	{
		createMutableCubeArrayColor(test_index);
	}

	/* Set up texture storage for stencil data */
	gl.genTextures(1, &m_texture_cube_array_stencil_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate texture object!");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture_cube_array_stencil_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind texture object to GL_TEXTURE_CUBE_MAP_ARRAY_EXT target");

	if (m_immutable_storage)
	{
		createImmutableCubeArrayStencil(test_index);
	}
	else
	{
		createMutableCubeArrayStencil(test_index);
	}

	/* Set up the draw framebuffer */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_draw_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind draw framebuffer!");

	gl.viewport(0, 0, m_cube_map_array_data[test_index].getWidth(), m_cube_map_array_data[test_index].getHeight());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set viewport");

	if (m_fbo_layered)
	{
		setupLayeredFramebuffer();
	}
	else
	{
		setupNonLayeredFramebuffer();
	}

	/* Is the draw FBO complete, now that we're done with configuring it? */
	checkFramebufferStatus(gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER));
}

/** Read pixels from color attachment of read framebuffer and compare them with expected result.
 *
 *  @param test_index index of cube map array configuration
 *
 *  @return true if failed, false otherwise.
 */
bool TextureCubeMapArrayStencilAttachments::readPixelsAndCompareWithExpectedResult(glw::GLuint test_index)
{
	const glw::Functions& gl			  = m_context.getRenderContext().getFunctions();
	bool				  has_test_failed = false;

	gl.readPixels(0 /* x */, 0 /* y */, m_cube_map_array_data[test_index].getWidth(),
				  m_cube_map_array_data[test_index].getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, m_result_data);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read pixel data from color buffer");

	glw::GLubyte expectedData[] = { 0, 0, 0, 0 };

	/* Loop over all pixels and compare the rendered data with reference values */
	for (glw::GLuint y = 0; y < m_cube_map_array_data[test_index].getHeight(); ++y)
	{
		/* Top half should be filled with different data than the bottom half */
		if (y >= m_cube_map_array_data[test_index].getHeight() / 2)
		{
			expectedData[0] = 0;
			expectedData[1] = 255;
			expectedData[2] = 255;
			expectedData[3] = 0;
		}
		else
		{
			expectedData[0] = 255;
			expectedData[1] = 0;
			expectedData[2] = 0;
			expectedData[3] = 255;
		}

		const glw::GLubyte* data_row =
			m_result_data + y * m_cube_map_array_data[test_index].getHeight() * m_n_cube_map_array_configurations;

		for (glw::GLuint x = 0; x < m_cube_map_array_data[test_index].getWidth(); ++x)
		{
			const glw::GLubyte* data = data_row + x * m_n_components;

			if (data[0] != expectedData[0] || data[1] != expectedData[1] || data[2] != expectedData[2] ||
				data[3] != expectedData[3])
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Expected Data ( " << (unsigned int)expectedData[0]
								   << "," << (unsigned int)expectedData[1] << "," << (unsigned int)expectedData[2]
								   << "," << (unsigned int)expectedData[3] << ") "
								   << "Result Data ( " << (unsigned int)data[0] << "," << (unsigned int)data[1] << ","
								   << (unsigned int)data[2] << "," << (unsigned int)data[3] << ")"
								   << tcu::TestLog::EndMessage;

				has_test_failed = true;
			}
		} /* for (all pixels in a row) */
	}	 /* for (all rows) */

	return has_test_failed;
}

/** Attach color and stencil attachments to a layer framebuffer **/
void TextureCubeMapArrayStencilAttachments::setupLayeredFramebuffer()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture_cube_array_color_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to GL_COLOR_ATTACHMENT0!");

	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_texture_cube_array_stencil_id,
						  0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to GL_DEPTH_STENCIL_ATTACHMENT! ");
}

/** Attach color and stencil attachments to a non-layered framebuffer. **/
void TextureCubeMapArrayStencilAttachments::setupNonLayeredFramebuffer()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture_cube_array_color_id, 0 /* level */,
							   0 /* layer */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture layer 0 to GL_COLOR_ATTACHMENT0!");

	gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_texture_cube_array_stencil_id,
							   0 /* level */, 0 /* layer */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture layer 0 to GL_DEPTH_STENCIL_ATTACHMENT! ");
}

} // namespace glcts
