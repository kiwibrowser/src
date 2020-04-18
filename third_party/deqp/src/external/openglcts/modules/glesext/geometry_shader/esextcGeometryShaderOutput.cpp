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

#include "esextcGeometryShaderOutput.hpp"

#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/* Shared shaders */
const char* const GeometryShaderOutput::m_fragment_shader_code_white_color = "${VERSION}\n"
																			 "\n"
																			 "precision highp float;\n"
																			 "\n"
																			 "out vec4 color;\n"
																			 "\n"
																			 "void main()\n"
																			 "{\n"
																			 "    color = vec4(1, 1, 1, 1);\n"
																			 "}\n";

const char* const GeometryShaderOutput::m_vertex_shader_code_two_vec4 = "${VERSION}\n"
																		"\n"
																		"precision highp float;\n"
																		"\n"
																		"out vec4 v1;\n"
																		"out vec4 v2;\n"
																		"\n"
																		"void main()\n"
																		"{\n"
																		"   v1 = vec4(-0.5, -0.5, 0, 1);\n"
																		"   v2 = vec4( 0.5,  0.5, 0, 1);\n"
																		"}\n";

const char* const GeometryShaderOutput::m_vertex_shader_code_vec4_0_0_0_1 = "${VERSION}\n"
																			"\n"
																			"precision highp float;\n"
																			"\n"
																			"void main()\n"
																			"{\n"
																			"   gl_Position = vec4(0, 0, 0, 1);\n"
																			"}\n";

/* Shaders for GeometryShaderDuplicateOutputLayoutQualifierTest */
const char* const GeometryShaderDuplicateOutputLayoutQualifierTest::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"layout(points)                            in;\n"
	"layout(triangle_strip, max_vertices = 60) out;\n"
	"layout(points)                            out;\n"
	"\n"
	"in vec4 v1[];\n"
	"in vec4 v2[];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = v1[0] + vec4(-0.1, -0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    gl_Position = v1[0] + vec4(0.1, -0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    gl_Position = v1[0] + vec4(0.1, 0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"\n"
	"    gl_Position = v2[0] + vec4(-0.1, -0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    gl_Position = v2[0] + vec4(-0.1, 0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    gl_Position = v2[0] + vec4(0.1, 0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"}\n";

/* Shaders for GeometryShaderDuplicateMaxVerticesLayoutQualifierTest */
const char* const GeometryShaderDuplicateMaxVerticesLayoutQualifierTest::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"layout(points)                            in;\n"
	"layout(triangle_strip, max_vertices = 60) out;\n"
	"layout(max_vertices = 20)                 out;\n"
	"\n"
	"in vec4 v1[];\n"
	"in vec4 v2[];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = v1[0] + vec4(-0.1, -0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    gl_Position = v1[0] + vec4(0.1, -0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    gl_Position = v1[0] + vec4(0.1, 0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"\n"
	"    gl_Position = v2[0] + vec4(-0.1, -0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    gl_Position = v2[0] + vec4(-0.1, 0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    gl_Position = v2[0] + vec4(0.1, 0.1, 0, 0);\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"}\n";

/* Shaders for GeometryShaderIfVertexEmitIsDoneAtEndTest */
const char* const GeometryShaderIfVertexEmitIsDoneAtEndTest::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                            in;\n"
	"layout(triangle_strip, max_vertices = 60) out;\n"
	"\n"
	"in vec4 v1[];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(-1, -1, 0, 1);\n"
	"    EmitVertex();\n"
	"    gl_Position = vec4(-1, 1, 0, 1);\n"
	"    EmitVertex();\n"
	"    gl_Position = vec4(1, 1, 0, 1);\n"
	"    EndPrimitive();\n"
	"}\n";

/* Shaders for GeometryShaderMissingEndPrimitiveCallTest */
const char* const GeometryShaderMissingEndPrimitiveCallTest::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"layout(points)                            in;\n"
	"layout(triangle_strip, max_vertices = 60) out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(-1, -1.004, 0, 1);\n"
	"    EmitVertex();\n"
	"    gl_Position = vec4(-1, 1, 0, 1);\n"
	"    EmitVertex();\n"
	"    gl_Position = vec4(1.004, 1, 0, 1);\n"
	"    EmitVertex();\n"
	"}\n";

/* Shaders for GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest */
const char* const GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GEOMETRY_POINT_SIZE_ENABLE}\n"
	"\n"
	"layout(points)                   in;\n"
	"layout(points, max_vertices = 1) out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position  = vec4(-1, -1, 0, 1);\n"
	"    gl_PointSize = 2.0f;\n"
	"    EmitVertex();\n"
	"}\n";

/* Definitions used by all test cases */
#define TEXTURE_HEIGHT (16)
#define TEXTURE_PIXEL_SIZE (4)
#define TEXTURE_WIDTH (16)

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
GeometryShaderOutput::GeometryShaderOutput(Context& context, const ExtParameters& extParams, const char* name,
										   const char* description)
	: TestCaseBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's desricption
 **/
GeometryShaderDuplicateOutputLayoutQualifierTest::GeometryShaderDuplicateOutputLayoutQualifierTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GeometryShaderOutput(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShaderDuplicateOutputLayoutQualifierTest::iterate()
{
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Verify the program does not build. */
	bool result = doesProgramBuild(1, &m_fragment_shader_code_white_color, 1, &m_geometry_shader_code, 1,
								   &m_vertex_shader_code_two_vec4);

	if (false == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid program was linked successfully."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's desricption
 **/
GeometryShaderDuplicateMaxVerticesLayoutQualifierTest::GeometryShaderDuplicateMaxVerticesLayoutQualifierTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GeometryShaderOutput(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShaderDuplicateMaxVerticesLayoutQualifierTest::iterate()
{
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Verify the program does not build. */
	bool result = doesProgramBuild(1, &m_fragment_shader_code_white_color, 1, &m_geometry_shader_code, 1,
								   &m_vertex_shader_code_two_vec4);

	if (false == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid program was linked successfully."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 *  @param context                  Test context
 *  @param name                     Test case's name
 *  @param description              Test case's desricption
 *  @param geometry_shader_code     Code of geometry shader
 **/
GeometryShaderOutputRenderingBase::GeometryShaderOutputRenderingBase(Context& context, const ExtParameters& extParams,
																	 const char* name, const char* description,
																	 const char* geometry_shader_code)
	: GeometryShaderOutput(context, extParams, name, description)
	, m_geometry_shader_code(geometry_shader_code)
	, m_program_object_id(0)
	, m_vertex_shader_id(0)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_vao_id(0)
	, m_fbo_id(0)
	, m_color_tex_id(0)
{
	/* Left blank on purpose */
}

/** Initialize test case
 *
 **/
void GeometryShaderOutputRenderingBase::initTest()
{
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create shader objects */
	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);

	/* Create program object */
	m_program_object_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create program object");

	/* Build the program object */
	if (false == buildProgram(m_program_object_id, m_fragment_shader_id, 1, &m_fragment_shader_code_white_color,
							  m_geometry_shader_id, 1, &m_geometry_shader_code, m_vertex_shader_id, 1,
							  &m_vertex_shader_code_vec4_0_0_0_1))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}

	/* Set up texture object and a FBO */
	gl.genTextures(1, &m_color_tex_id);
	gl.genFramebuffers(1, &m_fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create framebuffer");

	if (false ==
		setupFramebufferWithTextureAsAttachment(m_fbo_id, m_color_tex_id, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT))
	{
		TCU_FAIL("Failed to setup framebuffer");
	}

	/* Set up a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.enable(GL_PROGRAM_POINT_SIZE);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShaderOutputRenderingBase::iterate()
{
	initTest();

	/* Variables used for image verification purposes */
	unsigned char result_image[TEXTURE_HEIGHT * TEXTURE_WIDTH * TEXTURE_PIXEL_SIZE];

	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Render */
	gl.useProgram(m_program_object_id);

	gl.clearColor(0 /* red */, 0 /* green */, 0 /* blue */, 0 /* alpha */);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not clear the color buffer");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Call drawArrays() failed");

	/* Extract image from FBO */
	gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, result_image);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read back pixels from color buffer");

	/* Run verification */
	if (true == verifyResult(result_image, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_PIXEL_SIZE))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Deinitialize test case
 *
 **/
void GeometryShaderOutputRenderingBase::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0 /* texture */, 0 /* level */);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.disable(GL_PROGRAM_POINT_SIZE);
	}

	if (m_program_object_id != 0)
	{
		gl.deleteProgram(m_program_object_id);
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	if (m_color_tex_id != 0)
	{
		gl.deleteTextures(1, &m_color_tex_id);
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's desricption
 **/
GeometryShaderIfVertexEmitIsDoneAtEndTest::GeometryShaderIfVertexEmitIsDoneAtEndTest(Context&			  context,
																					 const ExtParameters& extParams,
																					 const char*		  name,
																					 const char*		  description)
	: GeometryShaderOutputRenderingBase(context, extParams, name, description, m_geometry_shader_code)
{
	/* Left blank on purpose */
}

/** Verifies result of draw call
 *
 *  @param result_image Image data
 *  @param width        Image width
 *  @param height       Image height
 *  @param pixel_size   Size of single pixel in bytes
 *
 *  @return true  if test succeded
 *          false if the test failed
 *          Note the function throws exception should an error occur!
 **/
bool GeometryShaderIfVertexEmitIsDoneAtEndTest::verifyResult(const unsigned char* result_image, unsigned int width,
															 unsigned int height, unsigned int pixel_size) const
{
	/* Check if the data was modified during the rendering process */
	for (unsigned int y = 0; y < height; ++y)
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			if (false == comparePixel(result_image, x, y, width, height, pixel_size))
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "Vertex emitted without a corresponding EmitVertex() call made"
								   << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	/* Done */
	return true;
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's desricption
 **/
GeometryShaderMissingEndPrimitiveCallTest::GeometryShaderMissingEndPrimitiveCallTest(Context&			  context,
																					 const ExtParameters& extParams,
																					 const char*		  name,
																					 const char*		  description)
	: GeometryShaderOutputRenderingBase(context, extParams, name, description, m_geometry_shader_code)
{
	/* Left blank on purpose */
}

/** Verifies result of draw call
 *
 *  @param result_image Image data
 *  @param width        Image width
 *  @param height       Image height
 *  @param pixel_size   Size of single pixel in bytes
 *
 *  @return true  if test succeded
 *          false if the test failed
 **/
bool GeometryShaderMissingEndPrimitiveCallTest::verifyResult(const unsigned char* result_image, unsigned int width,
															 unsigned int height, unsigned int pixel_size) const
{
	/* Image size */
	const unsigned int left   = 0;
	const unsigned int right  = width - 1;
	const unsigned int bottom = 0;
	const unsigned int top	= height - 1;

	/* Verification */
	if ((true == comparePixel(result_image, left, bottom, width, height, pixel_size, 255, 255, 255, 255)) &&
		(true == comparePixel(result_image, left, top, width, height, pixel_size, 255, 255, 255, 255)) &&
		(true == comparePixel(result_image, right, top, width, height, pixel_size, 255, 255, 255, 255)) &&
		(true == comparePixel(result_image, right, bottom, width, height, pixel_size, 0, 0, 0, 0)))
	{
		return true;
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "EndPrimitive() is not called at the end of geometry shader"
						   << tcu::TestLog::EndMessage;

		return false;
	}
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's desricption
 **/
GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest::
	GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest(Context& context, const ExtParameters& extParams,
																const char* name, const char* description)
	: GeometryShaderOutputRenderingBase(context, extParams, name, description, m_geometry_shader_code)
{
	/* Left blank on purpose */
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest::iterate()
{
	if (!m_is_geometry_shader_point_size_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_POINT_SIZE_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	return GeometryShaderOutputRenderingBase::iterate();
}

/** Verifies result of draw call
 *
 *  @param result_image Image data
 *  @param width        Image width
 *  @param height       Image height
 *  @param pixel_size   Size of single pixel in bytes
 *
 *  @return true  if test succeded
 *          false if the test failed
 *          Note the function throws exception should an error occur!
 **/
bool GeometryShaderMissingEndPrimitiveCallForSinglePrimitiveTest::verifyResult(const unsigned char* result_image,
																			   unsigned int width, unsigned int height,
																			   unsigned int pixel_size) const
{
	/* Image size */
	const unsigned int left   = 0;
	const unsigned int right  = width - 1;
	const unsigned int bottom = 0;
	const unsigned int top	= height - 1;

	/* Verification */
	if ((true == comparePixel(result_image, left, bottom, width, height, pixel_size, 255, 255, 255, 255)) &&
		(true == comparePixel(result_image, left, top, width, height, pixel_size, 0, 0, 0, 0)) &&
		(true == comparePixel(result_image, right, top, width, height, pixel_size, 0, 0, 0, 0)) &&
		(true == comparePixel(result_image, right, bottom, width, height, pixel_size, 0, 0, 0, 0)))
	{
		return true;
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "EndPrimitive() is not done for a single primitive"
						   << tcu::TestLog::EndMessage;

		return false;
	}
}

} // namespace glcts
