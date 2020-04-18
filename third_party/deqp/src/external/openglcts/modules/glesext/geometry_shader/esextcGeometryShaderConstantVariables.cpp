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

#include "esextcGeometryShaderConstantVariables.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/* Fragment shader code */
const char* GeometryShaderConstantVariables::m_fragment_shader_code = "${VERSION}\n"
																	  "\n"
																	  "precision highp float;\n"
																	  "\n"
																	  "out vec4 color;\n"
																	  "\n"
																	  "void main()\n"
																	  "{\n"
																	  "   color = vec4(1, 1, 1, 1);\n"
																	  "}\n";

/* Geometry shader code */
const char* GeometryShaderConstantVariables::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                 in;\n"
	"layout(points, max_vertices=1) out;\n"
	"\n"
	"flat out int test_MaxGeometryInputComponents;\n"
	"flat out int test_MaxGeometryOutputComponents;\n"
	"flat out int test_MaxGeometryTextureImageUnits;\n"
	"flat out int test_MaxGeometryOutputVertices;\n"
	"flat out int test_MaxGeometryTotalOutputComponents;\n"
	"flat out int test_MaxGeometryUniformComponents;\n"
	"flat out int test_MaxGeometryAtomicCounters;\n"
	"flat out int test_MaxGeometryAtomicCounterBuffers;\n"
	"flat out int test_MaxGeometryImageUniforms;\n"
	"\n"
	"void main()\n"
	"{\n"
	"   test_MaxGeometryInputComponents       = gl_MaxGeometryInputComponents;\n"
	"   test_MaxGeometryOutputComponents      = gl_MaxGeometryOutputComponents;\n"
	"   test_MaxGeometryTextureImageUnits     = gl_MaxGeometryTextureImageUnits;\n"
	"   test_MaxGeometryOutputVertices        = gl_MaxGeometryOutputVertices;\n"
	"   test_MaxGeometryTotalOutputComponents = gl_MaxGeometryTotalOutputComponents;\n"
	"   test_MaxGeometryUniformComponents     = gl_MaxGeometryUniformComponents;\n"
	"   test_MaxGeometryAtomicCounters        = gl_MaxGeometryAtomicCounters;\n"
	"   test_MaxGeometryAtomicCounterBuffers  = gl_MaxGeometryAtomicCounterBuffers;\n"
	"   test_MaxGeometryImageUniforms         = gl_MaxGeometryImageUniforms;\n"
	"\n"
	"   EmitVertex();\n"
	"   EndPrimitive();\n"
	"}\n";

/* Vertex shader code */
const char* GeometryShaderConstantVariables::m_vertex_shader_code = "${VERSION}\n"
																	"\n"
																	"precision highp float;\n"
																	"\n"
																	"void main()\n"
																	"{\n"
																	"}\n";

/* Specify transform feedback varyings */
const char* GeometryShaderConstantVariables::m_feedbackVaryings[] = {
	"test_MaxGeometryInputComponents", "test_MaxGeometryOutputComponents",		"test_MaxGeometryTextureImageUnits",
	"test_MaxGeometryOutputVertices",  "test_MaxGeometryTotalOutputComponents", "test_MaxGeometryUniformComponents",
	"test_MaxGeometryAtomicCounters",  "test_MaxGeometryAtomicCounterBuffers",  "test_MaxGeometryImageUniforms"
};

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's desricption
 **/
GeometryShaderConstantVariables::GeometryShaderConstantVariables(Context& context, const ExtParameters& extParams,
																 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_vertex_shader_id(0)
	, m_program_id(0)
	, m_bo_id(0)
	, m_vao_id(0)
	, m_min_MaxGeometryImagesUniforms(0)
	, m_min_MaxGeometryTextureImagesUnits(16)
	, m_min_MaxGeometryShaderStorageBlocks(0)
	, m_min_MaxGeometryAtomicCounterBuffers(0)
	, m_min_MaxGeometryAtomicCounters(0)
	, m_min_MaxFramebufferLayers(256)
	, m_min_MaxGeometryInputComponents(64)
	, m_min_MaxGeometryOutputComponents(64)
	, m_min_MaxGeometryOutputVertices(256)
	, m_min_MaxGeometryShaderInvocations(32)
	, m_min_MaxGeometryTotalOutputComponents(1024)
	, m_min_MaxGeometryUniformBlocks(12)
	, m_min_MaxGeometryUniformComponents(1024)
{
	/* Left blank intentionally */
}

/** Initializes GLES objects used during the test.
 *
 **/
void GeometryShaderConstantVariables::initTest(void)
{
	/* This test should only run if EXT_geometry_shader is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create VAO */
	gl.genVertexArrays(1, &m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create VAO!");

	/* Create a program object and set it up for TF */
	const unsigned int n_varyings = sizeof(m_feedbackVaryings) / sizeof(m_feedbackVaryings[0]);

	m_program_id = gl.createProgram();

	gl.transformFeedbackVaryings(m_program_id, n_varyings, m_feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set program object for transform feedback");

	/* Create shaders */
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);

	/* Build the test program */
	if (!buildProgram(m_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code, m_geometry_shader_id, 1,
					  &m_geometry_shader_code, m_vertex_shader_id, 1, &m_vertex_shader_code))
	{
		TCU_FAIL("Program could not have been created from a valid vertex/geometry/fragment shader!");
	}

	/* Create and set up a buffer object we will use for the test */
	gl.genBuffers(1, &m_bo_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLint) * n_varyings, DE_NULL, GL_STATIC_COPY);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set a buffer object up");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderConstantVariables::iterate(void)
{
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up relevant bindings */
	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a vertex array object");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a buffer object!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to transform feedback binding point.");

	/* Prepare for rendering. */
	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program");

	gl.enable(GL_RASTERIZER_DISCARD);

	gl.beginTransformFeedback(GL_POINTS);
	{
		/* Render */
		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed.");

	gl.disable(GL_RASTERIZER_DISCARD);

	/* First, retrieve the ES constant values using the API. */
	const unsigned int n_varyings				   = sizeof(m_feedbackVaryings) / sizeof(m_feedbackVaryings[0]);
	glw::GLint		   constant_values[n_varyings] = { 0 };
	unsigned int	   index					   = 0;

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_INPUT_COMPONENTS, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT failed.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_OUTPUT_COMPONENTS, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT failed.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT failed.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_OUTPUT_VERTICES, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT failed.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT failed.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_UNIFORM_COMPONENTS, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT failed.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTERS, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT failed.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT failed.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_IMAGE_UNIFORMS, &constant_values[index]);
	index++;
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT failed.");

	const glw::GLint* stored_data_ptr = (const glw::GLint*)gl.mapBufferRange(
		GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(glw::GLint) * n_varyings, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map buffer object storage");

	/* Compare the values that were stored by the draw call with values
	 * returned by the getter call.
	 */
	bool has_failed = false;
	for (unsigned int id = 0; id < n_varyings; ++id)
	{
		if (constant_values[id] != stored_data_ptr[id])
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Values reported for ES constant " << m_feedbackVaryings[id]
							   << " in a shader: " << stored_data_ptr[id]
							   << " and via a glGetIntegerv() call: " << constant_values[id] << " do not match."
							   << tcu::TestLog::EndMessage;
			has_failed = true;
		}
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

	/* Check whether the reported values are at least of the minimum value described in relevant
	 * extension specifications */

	glw::GLint int_value = 0;

	/* Check values of ES constants specific to shader atomic counters */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT");

	if (int_value < m_min_MaxGeometryAtomicCounterBuffers)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Reported GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT constant value " << int_value
						   << " is smaller than required minimum value of " << m_min_MaxGeometryAtomicCounterBuffers
						   << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTERS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT");

	if (int_value < m_min_MaxGeometryAtomicCounters)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT constant value "
						   << int_value << " is smaller than required minimum value of "
						   << m_min_MaxGeometryAtomicCounters << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	/* Check values of ES constants specific to image load store */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_IMAGE_UNIFORMS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT");

	if (int_value < m_min_MaxGeometryImagesUniforms)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT constant value "
						   << int_value << " is smaller than required minimum value of "
						   << m_min_MaxGeometryImagesUniforms << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT");

	if (int_value < m_min_MaxGeometryTextureImagesUnits)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Reported GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT constant value " << int_value
						   << " is smaller than required minimum value of " << m_min_MaxGeometryTextureImagesUnits
						   << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	/* Check values of ES constants specific to shader storage buffer objects */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT");

	if (int_value < m_min_MaxGeometryShaderStorageBlocks)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Reported GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT constant value " << int_value
						   << " is smaller than required minimum value of " << m_min_MaxGeometryShaderStorageBlocks
						   << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_UNIFORM_COMPONENTS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT");

	if (int_value < m_min_MaxGeometryUniformComponents)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT constant value "
						   << int_value << " is smaller than required minimum value of "
						   << m_min_MaxGeometryUniformComponents << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	/* Check EXT_geometry_shader specific constant values */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_UNIFORM_BLOCKS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT");

	if (int_value < m_min_MaxGeometryUniformBlocks)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT constant value "
						   << int_value << " is smaller than required minimum value of "
						   << m_min_MaxGeometryUniformBlocks << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_INPUT_COMPONENTS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT");

	if (int_value < m_min_MaxGeometryInputComponents)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT constant value "
						   << int_value << " is smaller than required minimum value of "
						   << m_min_MaxGeometryInputComponents << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_OUTPUT_COMPONENTS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT");

	if (int_value < m_min_MaxGeometryOutputComponents)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT constant value "
						   << int_value << " is smaller than required minimum value of "
						   << m_min_MaxGeometryOutputComponents << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_OUTPUT_VERTICES, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT");

	if (int_value < m_min_MaxGeometryOutputVertices)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT constant value "
						   << int_value << " is smaller than required minimum value of "
						   << m_min_MaxGeometryOutputVertices << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT");

	if (int_value < m_min_MaxGeometryTotalOutputComponents)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Reported GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT constant value " << int_value
						   << " is smaller than required minimum value of " << m_min_MaxGeometryTotalOutputComponents
						   << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_SHADER_INVOCATIONS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT");

	if (int_value < m_min_MaxGeometryShaderInvocations)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT constant value "
						   << int_value << " is smaller than required minimum value of "
						   << m_min_MaxGeometryShaderInvocations << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	gl.getIntegerv(m_glExtTokens.MAX_FRAMEBUFFER_LAYERS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_FRAMEBUFFER_LAYERS_EXT");

	if (int_value < m_min_MaxFramebufferLayers)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_MAX_FRAMEBUFFER_LAYERS_EXT constant value "
						   << int_value << " is smaller than required minimum value of " << m_min_MaxFramebufferLayers
						   << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	/* Compute minimum value that is acceptable for gl_MaxCombinedGeometryUniformComponents */
	glw::GLint n_max_geometry_uniform_blocks	 = 0;
	glw::GLint n_max_geometry_uniform_block_size = 0;
	glw::GLint n_max_geometry_uniform_components = 0;

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_UNIFORM_BLOCKS, &n_max_geometry_uniform_blocks);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT.");

	gl.getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &n_max_geometry_uniform_block_size);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_UNIFORM_BLOCK_SIZE.");

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_UNIFORM_COMPONENTS, &n_max_geometry_uniform_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT.");

	glw::GLint n_max_combined_geometry_uniform_components =
		n_max_geometry_uniform_blocks * n_max_geometry_uniform_block_size / 4 + n_max_geometry_uniform_components;

	/* Compare against actual constant value */
	gl.getIntegerv(m_glExtTokens.MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS_EXT.");

	if (int_value < n_max_combined_geometry_uniform_components)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Reported GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS_EXT constant value " << int_value
						   << " is smaller than required minimum value of "
						   << n_max_combined_geometry_uniform_components << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	/* Make sure value reported for GL_LAYER_PROVOKING_VERTEX_EXT is valid */
	gl.getIntegerv(m_glExtTokens.LAYER_PROVOKING_VERTEX, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_LAYER_PROVOKING_VERTEX_EXT.");

	if (
		/* This value is allowed in Desktop OpenGL, but not in the ES 3.1 extension. */
		(!glu::isContextTypeES(m_context.getRenderContext().getType()) &&
		 (glw::GLenum)int_value != GL_PROVOKING_VERTEX) &&
		(glw::GLenum)int_value != m_glExtTokens.FIRST_VERTEX_CONVENTION &&
		(glw::GLenum)int_value != m_glExtTokens.LAST_VERTEX_CONVENTION &&
		(glw::GLenum)int_value != m_glExtTokens.UNDEFINED_VERTEX)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Reported GL_LAYER_PROVOKING_VERTEX_EXT constant value "
						   << int_value << " is not among permissible values" << tcu::TestLog::EndMessage;

		has_failed = true;
	}

	if (has_failed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderConstantVariables::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);

	/* Delete program object and shaders */
	if (m_program_id != 0)
	{
		gl.deleteProgram(m_program_id);

		m_program_id = 0;
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);

		m_geometry_shader_id = 0;
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);

		m_fragment_shader_id = 0;
	}

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

} // namespace glcts
