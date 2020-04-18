/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file  gl3cTransformFeedback.cpp
 * \brief Transform Feedback Test Suite Implementation
 */ /*-------------------------------------------------------------------*/

/* Includes. */
#include "gl3cTransformFeedbackTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <sstream>

/* Stringify macro. */
#define _STR(s) STR(s)
#define STR(s) #s

/* Unused attribute / variable MACRO.
 Some methods of clesses' heirs do not need all function parameters.
 This triggers warnings on GCC platform. This macro will silence them.
 */
#ifdef __GNUC__
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif

gl3cts::TransformFeedback::Tests::Tests(deqp::Context& context)
	: TestCaseGroup(context, "transform_feedback", "Transform Feedback Test Suite")
{
	addChild(new TransformFeedback::APIErrors(m_context));
	addChild(new TransformFeedback::LinkingErrors(m_context));
	addChild(new TransformFeedback::Limits(m_context));
	addChild(new TransformFeedback::CaptureVertexInterleaved(m_context, "capture_vertex_interleaved_test",
															 "Transform Feedback Capture Vertex Interleaved Test"));
	addChild(new TransformFeedback::CaptureGeometryInterleaved(m_context, "capture_geometry_interleaved_test",
															   "Transform Feedback Capture Geometry Interleaved Test"));
	addChild(new TransformFeedback::CaptureVertexSeparate(m_context, "capture_vertex_separate_test",
														  "Transform Feedback Capture Vertex Separate Test"));
	addChild(new TransformFeedback::CaptureGeometrySeparate(m_context, "capture_geometry_separate_test",
															"Transform Feedback Capture Geometry Separate Test"));
	addChild(new TransformFeedback::CheckGetXFBVarying(m_context, "get_xfb_varying",
													   "Transform Feedback Varying Getters Test"));
	addChild(new TransformFeedback::QueryVertexInterleaved(m_context, "query_vertex_interleaved_test",
														   "Transform Feedback Query Vertex Interleaved Test"));
	addChild(new TransformFeedback::QueryGeometryInterleaved(m_context, "query_geometry_interleaved_test",
															 "Transform Feedback Query Geometry Interleaved Test"));
	addChild(new TransformFeedback::QueryVertexSeparate(m_context, "query_vertex_separate_test",
														"Transform Feedback Query Vertex Separate Test"));
	addChild(new TransformFeedback::QueryGeometrySeparate(m_context, "query_geometry_separate_test",
														  "Transform Feedback Query Geometry Separate Test"));
	addChild(new TransformFeedback::DiscardVertex(m_context, "discard_vertex_test",
												  "Transform Feedback Discard Vertex Test"));
	addChild(new TransformFeedback::DiscardGeometry(m_context, "discard_geometry_test",
													"Transform Feedback Discard Geometry Test"));
	addChild(new TransformFeedback::DrawXFB(m_context, "draw_xfb_test", "Transform Feedback Draw Test"));
	addChild(new TransformFeedback::DrawXFBFeedback(m_context, "draw_xfb_feedbackk_test",
													"Transform Feedback Draw Feedback Test"));
	addChild(
		new TransformFeedback::DrawXFBStream(m_context, "draw_xfb_stream_test", "Transform Feedback Draw Stream Test"));
	addChild(new TransformFeedback::CaptureSpecialInterleaved(m_context, "capture_special_interleaved_test",
															  "Transform Feedback Capture Special Test"));
	addChild(new TransformFeedback::DrawXFBInstanced(m_context, "draw_xfb_instanced_test",
													 "Transform Feedback Draw Instanced Test"));
	addChild(new TransformFeedback::DrawXFBStreamInstanced(m_context, "draw_xfb_stream_instanced_test",
														   "Transform Feedback Draw Stream Instanced Test"));
}

gl3cts::TransformFeedback::Tests::~Tests(void)
{
}

void gl3cts::TransformFeedback::Tests::init(void)
{
}

gl3cts::TransformFeedback::APIErrors::APIErrors(deqp::Context& context)
	: deqp::TestCase(context, "api_errors_test", "Transform Feedback API Errors Test")
	, m_context(context)
	, m_buffer_0(0)
	, m_buffer_1(0)
	, m_vertex_array_object(0)
	, m_transform_feedback_object_0(0)
	, m_transform_feedback_object_1(0)
	, m_query_object(0)
	, m_program_id_with_input_output(0)
	, m_program_id_with_output(0)
	, m_program_id_without_output(0)
	, m_program_id_with_geometry_shader(0)
	, m_program_id_with_tessellation_shaders(0)
	, m_glBindBufferOffsetEXT(DE_NULL)
	, m_glGetIntegerIndexedvEXT(DE_NULL)
	, m_glGetBooleanIndexedvEXT(DE_NULL)
{
}

gl3cts::TransformFeedback::APIErrors::~APIErrors(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::APIErrors::iterate(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initializations. */
	bool is_at_least_gl_30 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)));
	bool is_at_least_gl_40 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)));
	bool is_at_least_gl_42 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 2)));

	bool is_ext_tf_1		 = m_context.getContextInfo().isExtensionSupported("GL_EXT_transform_feedback");
	bool is_arb_tf_2		 = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback2");
	bool is_arb_tf_3		 = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback3");
	bool is_arb_tf_instanced = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback_instanced");

	if (is_ext_tf_1)
	{
		/* Extension query. */
		m_glBindBufferOffsetEXT =
			(BindBufferOffsetEXT_ProcAddress)m_context.getRenderContext().getProcAddress("glBindBufferOffsetEXT");
		m_glGetIntegerIndexedvEXT =
			(GetIntegerIndexedvEXT_ProcAddress)m_context.getRenderContext().getProcAddress("glGetIntegerIndexedvEXT");
		m_glGetBooleanIndexedvEXT =
			(GetBooleanIndexedvEXT_ProcAddress)m_context.getRenderContext().getProcAddress("glGetBooleanIndexedvEXT");
	}

	if (is_at_least_gl_40 || is_arb_tf_2)
	{
		/* Create transform feedback objects. */
		gl.genTransformFeedbacks(1, &m_transform_feedback_object_0);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks call failed.");
	}

	if (is_at_least_gl_40 || is_arb_tf_3)
	{
		/* Create query object. */
		gl.genQueries(1, &m_query_object);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries call failed.");
	}

	if (is_at_least_gl_42 || is_arb_tf_instanced)
	{
		/* Create transform feedback objects. */
		gl.genTransformFeedbacks(1, &m_transform_feedback_object_1);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks call failed.");
	}

	/* Default result. */
	bool is_ok		= true;
	bool test_error = false;

	/* Entities setup. */
	try
	{
		/* VAO setup. */
		gl.genVertexArrays(1, &m_vertex_array_object);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

		gl.bindVertexArray(m_vertex_array_object);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

		/* Buffer setup. */
		gl.genBuffers(1, &m_buffer_0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer_0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 16, NULL, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

		gl.genBuffers(1, &m_buffer_1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

		gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer_1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		gl.bufferData(GL_ARRAY_BUFFER, m_buffer_1_size, m_buffer_1_data, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

		/* Programs setup. */

		m_program_id_with_input_output = gl3cts::TransformFeedback::Utilities::buildProgram(
			gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader_with_input_output,
			s_fragment_shader, &m_varying_name, 1, GL_INTERLEAVED_ATTRIBS);

		m_program_id_with_output = gl3cts::TransformFeedback::Utilities::buildProgram(
			gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader_with_output, s_fragment_shader,
			&m_varying_name, 1, GL_INTERLEAVED_ATTRIBS, true);

		m_program_id_without_output = gl3cts::TransformFeedback::Utilities::buildProgram(
			gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader_without_output,
			s_fragment_shader, NULL, 0, GL_SEPARATE_ATTRIBS);

		m_program_id_with_geometry_shader = gl3cts::TransformFeedback::Utilities::buildProgram(
			gl, m_context.getTestContext().getLog(), m_geometry_shader, NULL, NULL, s_vertex_shader_without_output,
			s_fragment_shader, &m_varying_name, 1, GL_INTERLEAVED_ATTRIBS);

		m_program_id_with_tessellation_shaders = gl3cts::TransformFeedback::Utilities::buildProgram(
			gl, m_context.getTestContext().getLog(), NULL, m_tessellation_control_shader,
			m_tessellation_evaluation_shader, s_vertex_shader_without_output, s_fragment_shader, &m_varying_name, 1,
			GL_INTERLEAVED_ATTRIBS);

		is_ok = is_ok && m_program_id_with_input_output && m_program_id_with_output && m_program_id_without_output &&
				m_program_id_with_geometry_shader && m_program_id_with_tessellation_shaders;
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Iterating tests. */
	try
	{
		if (is_at_least_gl_30 || is_ext_tf_1)
		{
			is_ok = is_ok && testExtension1();
		}

		if (is_at_least_gl_40 || is_arb_tf_2)
		{
			is_ok = is_ok && testExtension2();
		}

		if (is_at_least_gl_40 || is_arb_tf_3)
		{
			is_ok = is_ok && testExtension3();
		}

		if (is_at_least_gl_42 || is_arb_tf_instanced)
		{
			is_ok = is_ok && testInstanced();
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Deinitialization. */
	if (m_vertex_array_object)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object);

		m_vertex_array_object = 0;
	}

	if (m_buffer_0)
	{
		gl.deleteBuffers(1, &m_buffer_0); // silently unbinds

		m_buffer_0 = 0;
	}

	if (m_buffer_1)
	{
		gl.deleteBuffers(1, &m_buffer_1); // silently unbinds

		m_buffer_1 = 0;
	}

	if (m_transform_feedback_object_0)
	{
		gl.deleteTransformFeedbacks(1, &m_transform_feedback_object_0);

		m_transform_feedback_object_0 = 0;
	}

	if (m_transform_feedback_object_1)
	{
		gl.deleteTransformFeedbacks(1, &m_transform_feedback_object_1);

		m_transform_feedback_object_1 = 0;
	}

	if (m_query_object)
	{
		gl.deleteQueries(1, &m_query_object);

		m_query_object = 0;
	}

	if (m_program_id_with_input_output)
	{
		gl.deleteProgram(m_program_id_with_input_output);

		m_program_id_with_input_output = 0;
	}

	if (m_program_id_with_output)
	{
		gl.deleteProgram(m_program_id_with_output);

		m_program_id_with_output = 0;
	}

	if (m_program_id_without_output)
	{
		gl.deleteProgram(m_program_id_without_output);

		m_program_id_without_output = 0;
	}

	if (m_program_id_with_geometry_shader)
	{
		gl.deleteProgram(m_program_id_with_geometry_shader);

		m_program_id_with_geometry_shader = 0;
	}

	if (m_program_id_with_tessellation_shaders)
	{
		gl.deleteProgram(m_program_id_with_tessellation_shaders);

		m_program_id_with_tessellation_shaders = 0;
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

bool gl3cts::TransformFeedback::APIErrors::testExtension1(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*  INVALID_VALUE is generated by BindBufferRange, BindBufferOffset and
	 BindBufferBase when <index> is greater or equal to
	 MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS; */

	glw::GLint index_count = 0;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &index_count);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (index_count == 0)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "glGetIntegerv did not returned any value."
											<< tcu::TestLog::EndMessage;
		throw 0;
	}

	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, index_count, m_buffer_0, 0, 16);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_VALUE was not generated by BindBufferRange "
											   "when <index> was greater or equal to "
											   "MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS."
											<< tcu::TestLog::EndMessage;
		return false;
	}

	if (DE_NULL != m_glBindBufferOffsetEXT)
	{
		m_glBindBufferOffsetEXT(GL_TRANSFORM_FEEDBACK_BUFFER, index_count, m_buffer_0, 0);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "INVALID_VALUE was not generated by BindBufferOffset "
												   "when <index> was greater or equal to "
												   "MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS."
												<< tcu::TestLog::EndMessage;
			return false;
		}
	}

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, index_count, m_buffer_0);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_VALUE was not generated by "
											   "BindBufferBase when <index> was greater or equal to "
											   "MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS."
											<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_VALUE is generated by BindBufferRange when <size> is less or equal to zero; */

	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, index_count, m_buffer_0, 0, 0);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_VALUE was not generated by BindBufferRange when <size> was less or equal to zero."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_VALUE is generated by BindBufferRange and BindBufferOffset
	 when <offset> is not word-aligned; */

	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, index_count, m_buffer_0, 3, 4);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_VALUE was not generated by BindBufferRange when <offset> was not word-aligned."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	if (DE_NULL != m_glBindBufferOffsetEXT)
	{
		m_glBindBufferOffsetEXT(GL_TRANSFORM_FEEDBACK_BUFFER, index_count, m_buffer_0, 3);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_VALUE was not generated by BindBufferOffset when <offset> was not word-aligned."
				<< tcu::TestLog::EndMessage;
			return false;
		}
	}

	/*  INVALID_VALUE is generated by BindBufferRange when <size> is not word-aligned; */

	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, index_count, m_buffer_0, 0, 3);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_VALUE was not generated by BindBufferRange when <size> was not word-aligned."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_OPERATION is generated by BindBufferRange, BindBufferOffset and
	 BindBufferBase when <target> is TRANSFORM_FEEDBACK_BUFFER and transform
	 feedback is active; */

	gl.useProgram(m_program_id_with_output);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0, 0, 16);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION was not generated by BindBufferRange "
											   "when <target> was TRANSFORM_FEEDBACK_BUFFER and transform "
											   "feedback was active."
											<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	if (DE_NULL != m_glBindBufferOffsetEXT)
	{
		m_glBindBufferOffsetEXT(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0, 0);

		if (GL_INVALID_OPERATION != gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "INVALID_OPERATION was not generated by BindBufferOffset "
												   "when <target> was TRANSFORM_FEEDBACK_BUFFER and transform "
												   "feedback was active."
												<< tcu::TestLog::EndMessage;

			gl.endTransformFeedback();

			return false;
		}
	}

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by "
										"BindBufferBase when <target> was TRANSFORM_FEEDBACK_BUFFER and transform "
										"feedback was active."
			<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	gl.endTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/*  INVALID_OPERATION is generated by UseProgram when transform feedback is
	 active; */

	gl.beginTransformFeedback(GL_POINTS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.useProgram(0);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_OPERATION was not generated by UseProgram when transform feedback was active."
			<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	gl.endTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/*  INVALID_OPERATION is generated by LinkProgram when <program> is currently
	 active and transform feedback is active; */

	gl.useProgram(m_program_id_with_output);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.linkProgram(m_program_id_with_output);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION was not generated by LinkProgram when <program> was "
											   "currently active and transform feedback was active."
											<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	gl.endTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/*  INVALID_OPERATION is generated by BeginTransformFeedback when transform
	 feedback is active; */

	gl.useProgram(m_program_id_with_output);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_OPERATION was not generated by BeginTransformFeedback when transform feedback was active."
			<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	gl.endTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/*  INVALID_OPERATION is generated by EndTransformFeedback when transform
	 feedback is inactive; */

	gl.endTransformFeedback();

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_OPERATION was not generated by EndTransformFeedback when transform feedback was inactive."
			<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  INVALID_OPERATION is generated by draw command when generated primitives
	 type does not match <primitiveMode>; */

	gl.useProgram(m_program_id_with_output);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_LINES, 0, 2);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION was not generated by draw command when generated "
											   "primitives type does not match <primitiveMode>."
											<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	gl.endTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/*  INVALID_OPERATION is generated by BeginTransformFeedback when any binding
	 point used by XFB does not have buffer bound; */

	gl.useProgram(m_program_id_with_output);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION was not generated by BeginTransformFeedback when any "
											   "binding point used by XFB does not have buffer bound."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  INVALID_OPERATION is generated by BeginTransformFeedback when no program
	 is active; */

	gl.useProgram(0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_OPERATION was not generated by BeginTransformFeedback when no program was active."
			<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	/*  INVALID_OPERATION is generated by BeginTransformFeedback when no variable
	 are specified to be captured in the active program; */

	gl.useProgram(m_program_id_without_output);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_OPERATION was not generated by BeginTransformFeedback when no variable "
			   "are specified to be captured in the active program."
			<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	/*  INVALID_VALUE is generated by TransformFeedbackVaryings when <program> is
	 not id of the program object; */

	unsigned short int invalid_name = 1;

	while (gl.isProgram(invalid_name) || gl.isShader(invalid_name))
	{
		++invalid_name;

		/* Make sure that this loop ends someday, bad day. */
		if (invalid_name == USHRT_MAX)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Cannot find unused trnasform feedback object." << tcu::TestLog::EndMessage;
			throw 0;
		}
	}

	gl.transformFeedbackVaryings((glw::GLuint)invalid_name, 1, &m_varying_name, GL_INTERLEAVED_ATTRIBS);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_VALUE was not generated by TransformFeedbackVaryings when "
											   "<program> was not id of the program object."
											<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_VALUE is generated by TransformFeedbackVaryings when <bufferMode>
	 is SEPARATE_ATTRIBS and <count> is exceeds limits; */

	glw::GLint max_separate_attribs = 0;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &max_separate_attribs);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_separate_attribs == 0)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_VALUE was not generated by TransformFeedbackVaryings when <bufferMode>"
										" was SEPARATE_ATTRIBS and <count> was exceeds limits."
			<< tcu::TestLog::EndMessage;
		throw 0;
	}

	glw::GLint more_than_max_separate_attribs = max_separate_attribs + 1;

	glw::GLchar** attrib = new glw::GLchar*[more_than_max_separate_attribs];

	for (glw::GLint i = 0; i < more_than_max_separate_attribs; ++i)
	{
		std::string new_attrib = "a" + gl3cts::TransformFeedback::Utilities::itoa(i);

		size_t new_attrib_size = new_attrib.size();

		attrib[i] = new glw::GLchar[new_attrib_size + 1];

		memset(attrib[i], 0, new_attrib_size + 1);

		memcpy(attrib[i], new_attrib.c_str(), new_attrib_size);
	}

	gl.transformFeedbackVaryings(m_program_id_with_output, more_than_max_separate_attribs, attrib, GL_SEPARATE_ATTRIBS);

	for (glw::GLint i = 0; i < more_than_max_separate_attribs; ++i)
	{
		delete[] attrib[i];
	}

	delete[] attrib;

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_VALUE was not generated by TransformFeedbackVaryings when "
											   "<bufferMode> was SEPARATE_ATTRIBS and <count> exceeded limits."
											<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_VALUE is generated by GetTransformFeedbackVarying when <index> is
	 greater than or equal to TRANSFORM_FEEDBACK_VARYINGS; */

	glw::GLint transform_feedback_varyings = 0;

	gl.getProgramiv(m_program_id_with_output, GL_TRANSFORM_FEEDBACK_VARYINGS, &transform_feedback_varyings);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

	if (transform_feedback_varyings == 0)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetProgramiv failed to return GL_TRANSFORM_FEEDBACK_VARYINGS."
											<< tcu::TestLog::EndMessage;
		throw 0;
	}

	glw::GLchar tmp_buffer[256];

	glw::GLsizei tmp_size = 0;

	glw::GLenum tmp_type = GL_NONE;

	gl.getTransformFeedbackVarying(m_program_id_with_output, transform_feedback_varyings, sizeof(tmp_buffer), NULL,
								   &tmp_size, &tmp_type, tmp_buffer);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_VALUE was not generated by GetTransformFeedbackVarying when "
											   "<index> was greater than or equal to TRANSFORM_FEEDBACK_VARYINGS."
											<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_VALUE is generated by GetIntegerIndexdv when <index> exceeds the
	 limits of MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS and <param> is one of the
	 following:
	 * TRANSFORM_FEEDBACK_BUFFER_BINDING,
	 * TRANSFORM_FEEDBACK_BUFFER_START,
	 * TRANSFORM_FEEDBACK_BUFFER_SIZE; */

	if (DE_NULL != m_glGetIntegerIndexedvEXT)
	{
		glw::GLint tmp_int_value;

		m_glGetIntegerIndexedvEXT(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, more_than_max_separate_attribs, &tmp_int_value);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_VALUE was not generated by GetIntegerIndexdv when <index> exceeds the "
				   "limits of MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS and <param> was "
				   "TRANSFORM_FEEDBACK_BUFFER_BINDING."
				<< tcu::TestLog::EndMessage;
			return false;
		}

		m_glGetIntegerIndexedvEXT(GL_TRANSFORM_FEEDBACK_BUFFER_START, more_than_max_separate_attribs, &tmp_int_value);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_VALUE was not generated by GetIntegerIndexdv when <index> exceeds the "
				   "limits of MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS and <param> was "
				   "GL_TRANSFORM_FEEDBACK_BUFFER_START."
				<< tcu::TestLog::EndMessage;
			return false;
		}

		m_glGetIntegerIndexedvEXT(GL_TRANSFORM_FEEDBACK_BUFFER_SIZE, more_than_max_separate_attribs, &tmp_int_value);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_VALUE was not generated by GetIntegerIndexdv when <index> exceeds the "
				   "limits of MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS and <param> was "
				   "GL_TRANSFORM_FEEDBACK_BUFFER_SIZE."
				<< tcu::TestLog::EndMessage;
			return false;
		}
	}

	/*  INVALID_VALUE is generated by GetBooleanIndexedv when <index> exceeds the
	 limits of MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS and <param> is
	 TRANSFORM_FEEDBACK_BUFFER_BINDING. */

	if (DE_NULL != m_glGetBooleanIndexedvEXT)
	{
		glw::GLboolean tmp_bool_value;

		m_glGetBooleanIndexedvEXT(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, more_than_max_separate_attribs,
								  &tmp_bool_value);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_VALUE was not generated by GetBooleanIndexedv when <index> exceeds the "
				   "limits of MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS and <param> was "
				   "TRANSFORM_FEEDBACK_BUFFER_BINDING."
				<< tcu::TestLog::EndMessage;
			return false;
		}
	}

	return true;
}

bool gl3cts::TransformFeedback::APIErrors::testExtension2(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*  Bind Transform Feedback Object */
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transform_feedback_object_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback call failed.");

	/*  INVALID_OPERATION is generated by PauseTransformFeedback if current
	 transform feedback is not active or paused; */

	gl.pauseTransformFeedback();

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION is not generated by PauseTransformFeedback if "
											   "current transform feedback is not active or paused."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  INVALID_OPERATION is generated by ResumeTransformFeedback if current
	 transform feedback is not active; */

	gl.resumeTransformFeedback();

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION is not generated by ResumeTransformFeedback if "
											   "current transform feedback is not active."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  Prepare program and buffer. */
	gl.useProgram(m_program_id_with_output);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");

	/*  INVALID_OPERATION is generated by DrawTransformFeedback when
	 EndTransformFeedback was never called for the object named <id>. */
	gl.drawTransformFeedback(GL_POINTS, m_transform_feedback_object_0);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION is not generated by DrawTransformFeedback when "
											   "EndTransformFeedback was never called for the object named <id>."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  Make Transform Feedback Active */
	gl.beginTransformFeedback(GL_POINTS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	/*  INVALID_OPERATION is generated by BindTransformFeedback if current
	 transform feedback is active and not paused; */

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transform_feedback_object_0);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		gl.endTransformFeedback();

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION is not generated by BindTransformFeedback if current "
											   "transform feedback is active and not paused."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  INVALID_OPERATION is generated by DeleteTransformFeedbacks if any of <ids>
	 is active; */

	gl.deleteTransformFeedbacks(1, &m_transform_feedback_object_0);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		gl.endTransformFeedback();

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "INVALID_OPERATION is not generated by DeleteTransformFeedbacks if any of <ids> is active."
			<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  INVALID_OPERATION is generated by ResumeTransformFeedback if current
	 transform feedback is not not paused; */

	gl.resumeTransformFeedback();

	if (GL_INVALID_OPERATION != gl.getError())
	{
		gl.endTransformFeedback();

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION is not generated by ResumeTransformFeedback if "
											   "current transform feedback is not not paused."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  pause transform feedback */

	gl.pauseTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glPauseTransformFeedback call failed.");

	/*  No error is generated by draw command when transform feedback is paused
	 and primitive modes do not match; */

	gl.drawArrays(GL_LINES, 0, 2);

	if (GL_NO_ERROR != gl.getError())
	{
		gl.endTransformFeedback();

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "No error is not generated by draw command when transform feedback is "
											   "paused and primitive modes do not match."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	/*  INVALID_OPERATION is generated by LinkProgram when <program> is used by
	 some transform feedback object that is currently not active; */

	gl.linkProgram(m_program_id_with_output);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_OPERATION was not generated by LinkProgram when <program> was "
											   "used by some transform feedback object that is currently not active."
											<< tcu::TestLog::EndMessage;

		gl.endTransformFeedback();

		return false;
	}

	/*  No error is generated by UseProgram when transform feedback is paused; */

	gl.useProgram(0);

	if (GL_NO_ERROR != gl.getError())
	{
		gl.endTransformFeedback();

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glUseProgram unexpectedly failed when transform feedback is paused."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	gl.useProgram(m_program_id_with_output);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	/*  End Transform Feedback and make draw. */

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/*  INVALID_VALUE is generated by DrawTransformFeedback if <id> is not name of
	 transform feedback object; */

	unsigned short int invalid_name = 1;

	while (gl.isTransformFeedback(invalid_name))
	{
		++invalid_name;

		/* Make sure that this loop ends someday, bad day. */
		if (invalid_name == USHRT_MAX)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Cannot find unused trnasform feedback object." << tcu::TestLog::EndMessage;
			throw 0;
		}
	}

	gl.drawTransformFeedback(GL_POINTS, (glw::GLuint)invalid_name);

	if (GL_INVALID_VALUE != gl.getError())
	{
		gl.endTransformFeedback();

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glUseProgram unexpectedly failed when transform feedback is paused."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

bool gl3cts::TransformFeedback::APIErrors::testExtension3(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*  INVALID_VALUE is generated by BeginQueryIndexed, EndQueryIndexed and
	 GetQueryIndexediv when <target> is TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN and
	 <index> exceeds limits of MAX_VERTEX_STREAMS

	 INVALID_VALUE is generated by BeginQueryIndexed, EndQueryIndexed and
	 GetQueryIndexediv when <target> is PRIMITIVES_GENERATED and <index> exceeds
	 limits of MAX_VERTEX_STREAMS */

	glw::GLint max_vertex_streams = 0;

	gl.getIntegerv(GL_MAX_VERTEX_STREAMS, &max_vertex_streams);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_vertex_streams == 0)
	{
		/* Nothing returned. */
		throw 0;
	}

	++max_vertex_streams;

	static const glw::GLenum  target[]	 = { GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, GL_PRIMITIVES_GENERATED };
	static const glw::GLchar* target_str[] = { STR(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN),
											   STR(GL_PRIMITIVES_GENERATED) };
	static const glw::GLuint target_count = sizeof(target) / sizeof(target[0]);

	for (glw::GLuint i = 0; i < target_count; ++i)
	{
		gl.beginQueryIndexed(target[i], max_vertex_streams, m_query_object);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE was not generated by BeginQueryIndexed, EndQueryIndexed and"
											"GetQueryIndexediv when <target> was "
				<< target_str[i] << " and "
									"<index> exceeded limits of MAX_VERTEX_STREAMS."
				<< tcu::TestLog::EndMessage;
			return false;
		}

		gl.endQueryIndexed(target[i], max_vertex_streams);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "INVALID_VALUE was not generated by EndQueryIndexed "
												   "when <target> was "
												<< target_str[i] << " and "
																	"<index> exceeded limits of MAX_VERTEX_STREAMS."
												<< tcu::TestLog::EndMessage;
			return false;
		}

		glw::GLint param = 0;

		gl.getQueryIndexediv(target[i], max_vertex_streams, GL_QUERY_COUNTER_BITS, &param);

		if (GL_INVALID_VALUE != gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "INVALID_VALUE was not generated by "
																			"GetQueryIndexediv when <target> was "
												<< target_str[i] << " and "
																	"<index> exceeded limits of MAX_VERTEX_STREAMS."
												<< tcu::TestLog::EndMessage;
			return false;
		}
	}

	/*  INVALID_OPERATION is generated by EndQueryIndexed when name of active
	 query at <index> of <target> is zero */

	gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 0);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by EndQueryIndexed when name of active "
										"query at <index> of <target> is zero"
			<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_VALUE is generated by DrawTransformFeedbackStream when <stream>
	 exceeds limits of MAX_VERTEX_STREAMS */

	gl.drawTransformFeedbackStream(GL_POINTS, m_transform_feedback_object_0, max_vertex_streams);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_VALUE was not generated by DrawTransformFeedbackStream when <stream> "
										"exceeded limits of MAX_VERTEX_STREAMS"
			<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_OPERATION is generated by TransformFeedbackVaryings when
	 <varyings> contains any of the special names while <bufferMode> is not
	 INTERLEAVED_ATTRIBS */

	static const glw::GLchar* tf_varying_names[] = { "gl_NextBuffer", "gl_SkipComponents1", "gl_SkipComponents2",
													 "gl_SkipComponents3", "gl_SkipComponents4" };
	static const glw::GLuint tf_varying_names_count = sizeof(tf_varying_names) / sizeof(tf_varying_names[0]);

	for (glw::GLuint i = 0; i < tf_varying_names_count; ++i)
	{
		gl.transformFeedbackVaryings(m_program_id_with_output, 1, &tf_varying_names[i], GL_SEPARATE_ATTRIBS);

		if (GL_INVALID_OPERATION != gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by TransformFeedbackVaryings when "
											"<varyings> contained any of the special names while <bufferMode> was "
											"GL_SEPARATE_ATTRIBS."
				<< tcu::TestLog::EndMessage;
			return false;
		}
	}

	/*  INVALID_OPERATION is generated by TransformFeedbackVaryings when
	 <varyings> contains more "gl_NextBuffer" entries than allowed limit of
	 MAX_TRANSFORM_FEEDBACK_BUFFERS */

	glw::GLint max_transform_feedback_buffers = 0;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_transform_feedback_buffers);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_transform_feedback_buffers == 0)
	{
		/* Nothing returned. */
		throw 0;
	}

	glw::GLint more_than_max_transform_feedback_buffers = max_transform_feedback_buffers + 1;

	const glw::GLchar** tf_next_buffer_varying_names = new const glw::GLchar*[more_than_max_transform_feedback_buffers];

	if (DE_NULL == tf_next_buffer_varying_names)
	{
		/* Allocation error. */
		throw 0;
	}

	for (glw::GLint i = 0; i < more_than_max_transform_feedback_buffers; ++i)
	{
		tf_next_buffer_varying_names[i] = tf_varying_names[0];
	}

	gl.transformFeedbackVaryings(m_program_id_with_output, more_than_max_transform_feedback_buffers,
								 tf_next_buffer_varying_names, GL_INTERLEAVED_ATTRIBS);

	delete[] tf_next_buffer_varying_names;

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by TransformFeedbackVaryings when "
										"<varyings> contained more \"gl_NextBuffer\" entries than allowed limit of "
										"MAX_TRANSFORM_FEEDBACK_BUFFER."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

bool gl3cts::TransformFeedback::APIErrors::testInstanced(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*  INVALID_ENUM is generated by DrawTransformFeedbackInstanced and
	 DrawTransformFeedbackStreamInstanced if <mode> is invalid */

	glw::GLenum _supported_mode[] = { GL_POINTS,
									  GL_LINE_STRIP,
									  GL_LINE_LOOP,
									  GL_LINES,
									  GL_LINE_STRIP_ADJACENCY,
									  GL_LINES_ADJACENCY,
									  GL_TRIANGLE_STRIP,
									  GL_TRIANGLE_FAN,
									  GL_TRIANGLES,
									  GL_TRIANGLE_STRIP_ADJACENCY,
									  GL_TRIANGLES_ADJACENCY,
									  GL_PATCHES };

	std::set<glw::GLenum> supported_mode(_supported_mode,
										 _supported_mode + sizeof(_supported_mode) / sizeof(_supported_mode[0]));

	int mode = 0;

	while (supported_mode.find(mode) != supported_mode.end())
	{
		mode++;
	}

	gl.drawTransformFeedbackInstanced(mode, m_transform_feedback_object_0, 1);

	if (GL_INVALID_ENUM != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_ENUM was not generated by DrawTransformFeedbackInstanced and "
											   "DrawTransformFeedbackStreamInstanced when <mode> was invalid."
											<< tcu::TestLog::EndMessage;
		return false;
	}

	gl.drawTransformFeedbackStreamInstanced(mode, m_transform_feedback_object_0, 0, 1);

	if (GL_INVALID_ENUM != gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "INVALID_ENUM was not generated by DrawTransformFeedbackInstanced and "
											   "DrawTransformFeedbackStreamInstanced when <mode> was invalid."
											<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_OPERATION is generated by DrawTransformFeedbackInstanced and
	 DrawTransformFeedbackStreamInstanced if <mode> does not match geometry
	 shader */

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transform_feedback_object_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback call failed.");

	gl.useProgram(m_program_id_with_geometry_shader);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindVertexArray(m_vertex_array_object);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.beginTransformFeedback(GL_POINTS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_POINTS, 0, 1);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	gl.drawTransformFeedbackInstanced(GL_LINES, m_transform_feedback_object_0, 1);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by DrawTransformFeedbackInstanced and "
										"DrawTransformFeedbackStreamInstanced if <mode> did not match geometry shader."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	gl.drawTransformFeedbackStreamInstanced(GL_LINES, m_transform_feedback_object_0, 0, 1);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by DrawTransformFeedbackInstanced and "
										"DrawTransformFeedbackStreamInstanced if <mode> did not match geometry shader."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_OPERATION is generated by DrawTransformFeedbackInstanced and
	 DrawTransformFeedbackStreamInstanced if <mode> does not match tessellation */

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transform_feedback_object_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback call failed.");

	gl.useProgram(m_program_id_with_tessellation_shaders);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindVertexArray(m_vertex_array_object);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.beginTransformFeedback(GL_LINES);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_PATCHES, 0, 2);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	gl.drawTransformFeedbackInstanced(GL_POINTS, m_transform_feedback_object_0, 1);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by DrawTransformFeedbackInstanced and "
										"DrawTransformFeedbackStreamInstanced if <mode> did not match geometry shader."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	gl.drawTransformFeedbackStreamInstanced(GL_POINTS, m_transform_feedback_object_0, 0, 1);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by DrawTransformFeedbackInstanced and "
										"DrawTransformFeedbackStreamInstanced if <mode> did not match geometry shader."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_VALUE is generated by DrawTransformFeedbackStreamInstanced if
	 <stream> is greater than or equal to MAX_VERTEX_STREAMS */

	glw::GLint max_vertex_streams = 0;

	gl.getIntegerv(GL_MAX_VERTEX_STREAMS, &max_vertex_streams);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_vertex_streams == 0)
	{
		/* Failed to query GL_MAX_VERTEX_STREAMS. */
		throw 0;
	}

	glw::GLint more_than_max_vertex_streams = max_vertex_streams + 1;

	gl.drawTransformFeedbackStreamInstanced(GL_PATCHES, m_transform_feedback_object_0, more_than_max_vertex_streams, 1);

	if (GL_INVALID_VALUE != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by DrawTransformFeedbackInstanced and "
										"DrawTransformFeedbackStreamInstanced if <mode> did not match geometry shader."
			<< tcu::TestLog::EndMessage;
		return false;
	}

	/*  INVALID_VALUE is generated by DrawTransformFeedbackInstanced and
	 DrawTransformFeedbackStreamInstanced if <id> is not name of transform
	 feedback object */

	unsigned short int invalid_name = 1;

	while (gl.isTransformFeedback(invalid_name))
	{
		++invalid_name;

		/* Make sure that this loop ends someday, bad day. */
		if (invalid_name == USHRT_MAX)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Cannot find unused trnasform feedback object." << tcu::TestLog::EndMessage;
			throw 0;
		}
	}

	gl.drawTransformFeedbackInstanced(GL_PATCHES, (glw::GLuint)invalid_name, 1);

	if (GL_INVALID_VALUE != gl.getError())
	{
		gl.endTransformFeedback();

		return false;
	}

	gl.drawTransformFeedbackStreamInstanced(GL_PATCHES, (glw::GLuint)invalid_name, 0, 1);

	if (GL_INVALID_VALUE != gl.getError())
	{
		gl.endTransformFeedback();

		return false;
	}

	/*  INVALID_OPERATION is generated if by DrawTransformFeedbackStreamInstanced
	 if EndTransformFeedback was never called for the object named <id>.
	 return true */

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transform_feedback_object_1);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback call failed.");

	gl.drawTransformFeedbackStreamInstanced(GL_PATCHES, m_transform_feedback_object_1, 0, 1);

	if (GL_INVALID_OPERATION != gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "INVALID_OPERATION was not generated by DrawTransformFeedbackInstanced and "
										"DrawTransformFeedbackStreamInstanced if <mode> did not match geometry shader."
			<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

const glw::GLchar* gl3cts::TransformFeedback::APIErrors::m_tessellation_control_shader =
	"#version 400\n"
	"\n"
	"layout (vertices = 2 ) out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_TessLevelOuter[1]                = 3.0;\n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::APIErrors::m_tessellation_evaluation_shader =
	"#version 400\n"
	"\n"
	"layout(isolines, equal_spacing, ccw) in;\n"
	"\n"
	"out float result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    result      = 0.5;\n"
	"    gl_Position = gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position;\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::APIErrors::m_geometry_shader =
	"#version 150\n"
	"\n"
	"layout(points) in;\n"
	"layout(points, max_vertices = 2) out;\n"
	"\n"
	"out float result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.0, 0.0, 0.0);\n"
	"    result = 0.0;\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = gl_in[0].gl_Position + vec4(1.0, 0.0, 0.0, 0.0);\n"
	"    result = 1.0;\n"
	"    EmitVertex();\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::APIErrors::s_vertex_shader_with_output =
	"#version 130\n"
	"\n"
	"out float result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    result      = float(gl_VertexID);\n"
	"    gl_Position = vec4(1.0);\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::APIErrors::s_vertex_shader_with_input_output =
	"#version 130\n"
	"\n"
	"in float v_input;\n"
	"\n"
	"out float result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    result      = float(gl_VertexID);\n"
	"    gl_Position = vec4(v_input);\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::APIErrors::s_vertex_shader_without_output =
	"#version 130\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(1.0);\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::APIErrors::s_fragment_shader = "#version 130\n"
																			 "\n"
																			 "out vec4 color;\n"
																			 "\n"
																			 "void main()\n"
																			 "{\n"
																			 "    color = vec4(1.0);\n"
																			 "}\n";

const glw::GLchar* gl3cts::TransformFeedback::APIErrors::m_varying_name = "result";

const glw::GLfloat gl3cts::TransformFeedback::APIErrors::m_buffer_1_data[] = { 3.14159265359f, 2.7182818f };

const glw::GLsizei gl3cts::TransformFeedback::APIErrors::m_buffer_1_size =
	sizeof(gl3cts::TransformFeedback::APIErrors::m_buffer_1_data);

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::LinkingErrors::LinkingErrors(deqp::Context& context)
	: deqp::TestCase(context, "linking_errors_test", "Transform Feedback Linking Errors Test"), m_context(context)
{
	/* Left intentionally blank. */
}

gl3cts::TransformFeedback::LinkingErrors::~LinkingErrors(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::LinkingErrors::iterate(void)
{
	bool is_ok		= true;
	bool test_error = false;

	try
	{
		is_ok = is_ok && testNoVertexNoGeometry();
		is_ok = is_ok && testInvalidVarying();
		is_ok = is_ok && testRepeatedVarying();
		is_ok = is_ok && testTooManyVaryings();
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

bool gl3cts::TransformFeedback::LinkingErrors::testNoVertexNoGeometry(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*  Check if link process fails under the following conditions:
	 <count> specified by TransformFeedbackVaryings is non-zero and program has
	 neither vertex nor geometry shader; */

	glw::GLint linking_status = 1;

	glw::GLuint program = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, NULL, s_fragment_shader,
		&s_valid_transform_feedback_varying, 1, GL_INTERLEAVED_ATTRIBS, false, &linking_status);

	if ((GL_FALSE != linking_status) || program)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Linking unexpectedly succeded when Transform Feedback varying was "
											   "specified but program had neither vertex nor geometry shader stages."
											<< tcu::TestLog::EndMessage;

		if (program)
		{
			gl.deleteProgram(program);
		}

		return false;
	}

	/* Log success. */
	m_context.getTestContext().getLog() << tcu::TestLog::Message
										<< "Linking failed as expected when Transform Feedback varying was specified "
										   "but program had neither vertex nor geometry shader stages."
										<< tcu::TestLog::EndMessage;

	return true;
}

bool gl3cts::TransformFeedback::LinkingErrors::testInvalidVarying(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*  Check if link process fails under the following conditions:
	 <varyings> specified by TransformFeedbackVaryings contains name of
	 variable that is not available for capture; */

	std::string vertex_shader(s_vertex_shader_template);

	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(
		vertex_shader, "TEMPLATE_INPUT_OUTPUT_DECLARATIONS", "in float data;\n");
	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(vertex_shader, "TEMPLATE_OUTPUT_SETTERS", "");

	glw::GLint linking_status = 1;

	glw::GLuint program = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, vertex_shader.c_str(), s_fragment_shader,
		&s_invalid_transform_feedback_varying, 1, GL_INTERLEAVED_ATTRIBS, false, &linking_status);

	if ((GL_FALSE != linking_status) || program)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Linking unexpectedly succeded when Transform Feedback varying was specified with name of variable ("
			<< s_invalid_transform_feedback_varying << ") that is not available for capture."
			<< tcu::TestLog::EndMessage;

		if (program)
		{
			gl.deleteProgram(program);
		}

		return false;
	}

	/* Log success. */
	m_context.getTestContext().getLog()
		<< tcu::TestLog::Message
		<< "Linking failed as expected when Transform Feedback varying was specified with name of variable ("
		<< s_invalid_transform_feedback_varying << ") that is not available for capture." << tcu::TestLog::EndMessage;

	return true;
}

bool gl3cts::TransformFeedback::LinkingErrors::testRepeatedVarying(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*  Check if link process fails under the following conditions:
	 <varyings> specified by TransformFeedbackVaryings contains name of
	 variable more than once; */

	std::string vertex_shader(s_vertex_shader_template);

	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(
		vertex_shader, "TEMPLATE_INPUT_OUTPUT_DECLARATIONS", "out float result;\n");
	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(vertex_shader, "TEMPLATE_OUTPUT_SETTERS",
																		 "    result = 0.577215664901532;\n");

	glw::GLint linking_status = 1;

	glw::GLuint program = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, vertex_shader.c_str(), s_fragment_shader,
		s_repeated_transform_feedback_varying, s_repeated_transform_feedback_varying_count, GL_INTERLEAVED_ATTRIBS,
		false, &linking_status);

	if ((GL_FALSE != linking_status) || program)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Linking unexpectedly succeded when Transform Feedback varying was specified twice."
			<< tcu::TestLog::EndMessage;

		if (program)
		{
			gl.deleteProgram(program);
		}

		return false;
	}

	/* Log success. */
	m_context.getTestContext().getLog()
		<< tcu::TestLog::Message << "Linking failed as expected when Transform Feedback varying was specified twice."
		<< tcu::TestLog::EndMessage;

	return true;
}

bool gl3cts::TransformFeedback::LinkingErrors::testTooManyVaryings(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/*  Check if link process fails under the following conditions:
	 number of components specified to capture exceeds limits
	 MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS or
	 MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS. */

	/* Fetching limits. */
	glw::GLint max_transform_feedback_separate_components	= 0;
	glw::GLint max_transform_feedback_interleaved_components = 0;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, &max_transform_feedback_separate_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_transform_feedback_separate_components == 0)
	{
		throw 0;
	}

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &max_transform_feedback_interleaved_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_transform_feedback_interleaved_components == 0)
	{
		throw 0;
	}

	glw::GLint more_than_max_transform_feedback_components =
		deMax32(max_transform_feedback_separate_components, max_transform_feedback_interleaved_components) + 1;

	/* Preparing source code. */
	std::string						vertex_shader(s_vertex_shader_template);
	std::string						transform_feedback_variable_declarations("");
	std::string						transform_feedback_variable_setters("");
	std::vector<std::string>		transform_feedback_varyings(more_than_max_transform_feedback_components);
	std::vector<const glw::GLchar*> transform_feedback_varyings_c(more_than_max_transform_feedback_components);

	for (glw::GLint i = 0; i < more_than_max_transform_feedback_components; ++i)
	{
		std::string varying = "result_";
		varying.append(gl3cts::TransformFeedback::Utilities::itoa(i));

		transform_feedback_varyings[i] = varying;

		transform_feedback_varyings_c[i] = transform_feedback_varyings[i].c_str();

		transform_feedback_variable_declarations.append("out float ");
		transform_feedback_variable_declarations.append(varying);
		transform_feedback_variable_declarations.append(";\n");

		transform_feedback_variable_setters.append("    ");
		transform_feedback_variable_setters.append(varying);
		transform_feedback_variable_setters.append(" = ");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * i));
		transform_feedback_variable_setters.append(".0;\n");
	}

	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(
		vertex_shader, "TEMPLATE_INPUT_OUTPUT_DECLARATIONS", transform_feedback_variable_declarations);
	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(vertex_shader, "TEMPLATE_OUTPUT_SETTERS",
																		 transform_feedback_variable_setters);

	glw::GLuint program = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, vertex_shader.c_str(), s_fragment_shader,
		&transform_feedback_varyings_c[0], more_than_max_transform_feedback_components, GL_INTERLEAVED_ATTRIBS);

	/* Note: we check for program as not only linking shall fail, but also glTransformFeedbackVaryings shall return an error. */

	if (program)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Linking unexpectedly succeded when too many Transform Feedback varying "
											   "were specified in INTERLEAVED mode."
											<< tcu::TestLog::EndMessage;

		if (program)
		{
			gl.deleteProgram(program);
		}

		return false;
	}

	program = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, vertex_shader.c_str(), s_fragment_shader,
		&transform_feedback_varyings_c[0], more_than_max_transform_feedback_components, GL_SEPARATE_ATTRIBS);

	if (program)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Linking unexpectedly succeded when too many Transform Feedback "
											   "varyings were specified in SEPARATE mode."
											<< tcu::TestLog::EndMessage;

		if (program)
		{
			gl.deleteProgram(program);
		}

		return false;
	}

	/* Log success. */
	m_context.getTestContext().getLog()
		<< tcu::TestLog::Message
		<< "Linking failed as expected when too many Transform Feedback varyings were specified."
		<< tcu::TestLog::EndMessage;

	return true;
}

const glw::GLchar* gl3cts::TransformFeedback::LinkingErrors::s_fragment_shader = "#version 130\n"
																				 "\n"
																				 "out vec4 color;\n"
																				 "\n"
																				 "void main()\n"
																				 "{\n"
																				 "    color = vec4(1.0);\n"
																				 "}\n";

const glw::GLchar* gl3cts::TransformFeedback::LinkingErrors::s_vertex_shader_template =
	"#version 130\n"
	"\n"
	"TEMPLATE_INPUT_OUTPUT_DECLARATIONS"
	"\n"
	"void main()\n"
	"{\n"
	"TEMPLATE_OUTPUT_SETTERS"
	"\n"
	"    gl_Position = vec4(1.618033988749);\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::LinkingErrors::s_valid_transform_feedback_varying = "result";

const glw::GLchar* gl3cts::TransformFeedback::LinkingErrors::s_invalid_transform_feedback_varying = "data";

const glw::GLchar* gl3cts::TransformFeedback::LinkingErrors::s_repeated_transform_feedback_varying[] = { "result",
																										 "result" };

const glw::GLsizei gl3cts::TransformFeedback::LinkingErrors::s_repeated_transform_feedback_varying_count =
	sizeof(s_repeated_transform_feedback_varying) / sizeof(s_repeated_transform_feedback_varying[0]);

/*-----------------------------------------------------------------------------------------------*/

const glw::GLint gl3cts::TransformFeedback::Limits::s_min_value_of_max_transform_feedback_interleaved_components = 64;
const glw::GLint gl3cts::TransformFeedback::Limits::s_min_value_of_max_transform_feedback_separate_attribs		 = 4;
const glw::GLint gl3cts::TransformFeedback::Limits::s_min_value_of_max_transform_feedback_separate_components	= 4;
const glw::GLint gl3cts::TransformFeedback::Limits::s_min_value_of_max_transform_feedback_buffers				 = 4;
const glw::GLint gl3cts::TransformFeedback::Limits::s_min_value_of_max_vertex_streams							 = 1;

gl3cts::TransformFeedback::Limits::Limits(deqp::Context& context)
	: deqp::TestCase(context, "limits_test", "Transform Feedback Limits Test"), m_context(context)
{
}

gl3cts::TransformFeedback::Limits::~Limits(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::Limits::iterate(void)
{
	/* Initializations. */
	bool is_at_least_gl_30 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)));
	bool is_at_least_gl_40 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)));

	bool is_ext_tf_1 = m_context.getContextInfo().isExtensionSupported("GL_EXT_transform_feedback");
	bool is_arb_tf_3 = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback3");

	bool is_ok		= true;
	bool test_error = false;

	/* Tests. */
	try
	{
		if (is_at_least_gl_30 || is_ext_tf_1)
		{
			is_ok = is_ok && test_max_transform_feedback_interleaved_components();
			is_ok = is_ok && test_max_transform_feedback_separate_attribs();
			is_ok = is_ok && test_max_transform_feedback_separate_components();
		}

		if (is_at_least_gl_40 || is_arb_tf_3)
		{
			is_ok = is_ok && test_max_transform_feedback_buffers();
			is_ok = is_ok && test_max_vertex_streams();
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Limits are in range of specification."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

bool gl3cts::TransformFeedback::Limits::test_max_transform_feedback_interleaved_components(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check that MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS is at least 64. */
	glw::GLint max_transform_feedback_interleaved_components = 0;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &max_transform_feedback_interleaved_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_transform_feedback_interleaved_components < s_min_value_of_max_transform_feedback_interleaved_components)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS is equal to "
			<< max_transform_feedback_interleaved_components << " which is less than expected "
			<< s_min_value_of_max_transform_feedback_interleaved_components << "." << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

bool gl3cts::TransformFeedback::Limits::test_max_transform_feedback_separate_attribs(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check that MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS is at least 4. */
	glw::GLint max_transform_feedback_separate_attribs = 0;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &max_transform_feedback_separate_attribs);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_transform_feedback_separate_attribs < s_min_value_of_max_transform_feedback_separate_attribs)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS is equal to "
			<< max_transform_feedback_separate_attribs << " which is less than expected "
			<< s_min_value_of_max_transform_feedback_separate_attribs << "." << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

bool gl3cts::TransformFeedback::Limits::test_max_transform_feedback_separate_components(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check that MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS is at least 4. */
	glw::GLint max_transform_feedback_separate_components = 0;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, &max_transform_feedback_separate_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_transform_feedback_separate_components < s_min_value_of_max_transform_feedback_separate_components)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS is equal to "
			<< max_transform_feedback_separate_components << " which is less than expected "
			<< s_min_value_of_max_transform_feedback_separate_components << "." << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

bool gl3cts::TransformFeedback::Limits::test_max_transform_feedback_buffers(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check that MAX_TRANSFORM_FEEDBACK_BUFFERS is at least 4. */
	glw::GLint max_transform_feedback_buffers = 0;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_transform_feedback_buffers);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_transform_feedback_buffers < s_min_value_of_max_transform_feedback_buffers)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "MAX_TRANSFORM_FEEDBACK_BUFFERS is equal to "
											<< max_transform_feedback_buffers << " which is less than expected "
											<< s_min_value_of_max_transform_feedback_buffers << "."
											<< tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

bool gl3cts::TransformFeedback::Limits::test_max_vertex_streams(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check that MAX_VERTEX_STREAMS is at least 1. */
	glw::GLint max_vertex_streams = 0;

	gl.getIntegerv(GL_MAX_VERTEX_STREAMS, &max_vertex_streams);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_vertex_streams < s_min_value_of_max_vertex_streams)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "MAX_VERTEX_STREAMS is equal to "
											<< max_vertex_streams << " which is less than expected "
											<< s_min_value_of_max_vertex_streams << "." << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::CaptureVertexInterleaved::CaptureVertexInterleaved(deqp::Context& context,
																			  const char*	test_name,
																			  const char*	test_description)
	: deqp::TestCase(context, test_name, test_description)
	, m_context(context)
	, m_program(0)
	, m_framebuffer(0)
	, m_renderbuffer(0)
	, m_buffer(0)
	, m_buffer_size(0)
	, m_vertex_array_object(0)
	, m_max_transform_feedback_components(0)
	, m_attrib_type(GL_INTERLEAVED_ATTRIBS)
	, m_max_vertices_drawn(8)
	, m_glBindBufferOffsetEXT(DE_NULL)
{
}

gl3cts::TransformFeedback::CaptureVertexInterleaved::~CaptureVertexInterleaved(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::CaptureVertexInterleaved::iterate(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initializations. */
	bool is_at_least_gl_30 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)));
	bool is_ext_tf_1	   = m_context.getContextInfo().isExtensionSupported("GL_EXT_transform_feedback");

	bool is_ok		= true;
	bool test_error = false;

	try
	{
		if (is_ext_tf_1)
		{
			/* Extension query. */
			m_glBindBufferOffsetEXT =
				(BindBufferOffsetEXT_ProcAddress)m_context.getRenderContext().getProcAddress("glBindBufferOffsetEXT");

			if (DE_NULL == m_glBindBufferOffsetEXT)
			{
				throw 0;
			}
		}

		if (is_at_least_gl_30 || is_ext_tf_1)
		{
			fetchLimits();
			buildProgram();
			createFramebuffer();
			createTransformFeedbackBuffer();
			createVertexArrayObject();

			gl.useProgram(m_program);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

			for (glw::GLint i_bind_case = 0; (i_bind_case < BIND_BUFFER_CASES_COUNT) && is_ok; ++i_bind_case)
			{
				if ((i_bind_case == BIND_BUFFER_OFFSET_CASE) && (DE_NULL == m_glBindBufferOffsetEXT))
				{
					continue;
				}

				bindBuffer((BindBufferCase)i_bind_case);

				for (glw::GLuint i_primitive_case = 0; (i_primitive_case < s_primitive_cases_count) && is_ok;
					 ++i_primitive_case)
				{
					draw(i_primitive_case);

					is_ok = is_ok && checkFramebuffer(s_primitive_cases[i_primitive_case]);
					is_ok = is_ok && checkTransformFeedbackBuffer((BindBufferCase)i_bind_case,
																  s_primitive_cases[i_primitive_case]);
				}
			}
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Clean objects. */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Capture Vertex have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::fetchLimits(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching limits. */
	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &m_max_transform_feedback_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (m_max_transform_feedback_components == 0)
	{
		throw 0;
	}

	glw::GLint max_varyings_components = 0;

	gl.getIntegerv(GL_MAX_VARYING_COMPONENTS, &max_varyings_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_varyings_components == 0)
	{
		throw 0;
	}

	if (m_max_transform_feedback_components > max_varyings_components)
	{
		m_max_transform_feedback_components = max_varyings_components;
	}
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::buildProgram(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing source code. */
	std::string vertex_shader(s_vertex_shader_source_code_template); /* Storage for vertex shader source code. */
	std::string transform_feedback_variable_declarations(
		""); /* String to contain all custom outputs from vertex shader. */
	std::string transform_feedback_variable_setters(
		""); /* String containing all initializations of custom outputs from vertex shader. */
	std::vector<std::string> transform_feedback_varyings(m_max_transform_feedback_components); /* Varyings array. */
	std::vector<const glw::GLchar*> transform_feedback_varyings_c(
		m_max_transform_feedback_components); /* Varyings array in C form to pass to the GL. */

	glw::GLint user_defined_transform_feedback_interleaved_varyings_count =
		m_max_transform_feedback_components /* total max to be written by the shader */
			/ 4								/* components per vec4 */
		- 1 /* gl_Position */;

	glw::GLint all_transform_feedback_interleaved_varyings_count =
		user_defined_transform_feedback_interleaved_varyings_count + 1 /* gl_Position */;

	/* Most of varyings is declarated output variables. */
	for (glw::GLint i = 0; i < user_defined_transform_feedback_interleaved_varyings_count; ++i)
	{
		std::string varying = "result_";
		varying.append(gl3cts::TransformFeedback::Utilities::itoa(i));

		transform_feedback_varyings[i] = varying;

		transform_feedback_varyings_c[i] = transform_feedback_varyings[i].c_str();

		transform_feedback_variable_declarations.append("out vec4 ");
		transform_feedback_variable_declarations.append(varying);
		transform_feedback_variable_declarations.append(";\n");

		transform_feedback_variable_setters.append("    ");
		transform_feedback_variable_setters.append(varying);
		transform_feedback_variable_setters.append(" = vec4(");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * 4));
		transform_feedback_variable_setters.append(".0, ");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * 4 + 1));
		transform_feedback_variable_setters.append(".0, ");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * 4 + 2));
		transform_feedback_variable_setters.append(".0, ");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * 4 + 3));
		transform_feedback_variable_setters.append(".0);\n");
	}

	/* Last four varying components are gl_Position components. */
	transform_feedback_varyings[user_defined_transform_feedback_interleaved_varyings_count] = "gl_Position";

	transform_feedback_varyings_c[user_defined_transform_feedback_interleaved_varyings_count] =
		transform_feedback_varyings[user_defined_transform_feedback_interleaved_varyings_count].c_str();

	/* Preprocess vertex shader source code template. */
	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(
		vertex_shader, "TEMPLATE_INPUT_OUTPUT_DECLARATIONS", transform_feedback_variable_declarations);
	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(vertex_shader, "TEMPLATE_OUTPUT_SETTERS",
																		 transform_feedback_variable_setters);
	vertex_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(
		vertex_shader, "TEMPLATE_RASTERIZATION_EPSILON",
		gl3cts::TransformFeedback::Utilities::ftoa(s_rasterization_epsilon));

	/* Compile, link and check. */
	m_program = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, vertex_shader.c_str(), s_fragment_shader_source_code,
		&transform_feedback_varyings_c[0], all_transform_feedback_interleaved_varyings_count, m_attrib_type);

	if (0 == m_program)
	{
		throw 0;
	}
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::createFramebuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setting clear color */
	gl.clearColor(0.f, 0.f, 0.f, 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	/* Creating framebuffer */
	gl.genFramebuffers(1, &m_framebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_renderbuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R32F, s_framebuffer_size, s_framebuffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderbuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_framebuffer_size, s_framebuffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::createTransformFeedbackBuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating xfb buffer */
	gl.genBuffers(1, &m_buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	m_buffer_size =
		static_cast<glw::GLuint>(m_max_transform_feedback_components * m_max_vertices_drawn * sizeof(glw::GLfloat));

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer_size, NULL, GL_DYNAMIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::createVertexArrayObject(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* VAO Creations */
	gl.genVertexArrays(1, &m_vertex_array_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vertex_array_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::draw(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Draw */
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	gl.beginTransformFeedback(s_primitive_cases_xfb[primitive_case]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedbac call failed.");

	gl.drawElements(s_primitive_cases[primitive_case], s_element_indices_counts[primitive_case], GL_UNSIGNED_INT,
					s_element_indices[primitive_case]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElements call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedbac call failed.");
}

bool gl3cts::TransformFeedback::CaptureVertexInterleaved::checkFramebuffer(glw::GLenum primitive_type UNUSED)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch framebuffer. */
	std::vector<glw::GLfloat> pixels(s_framebuffer_size * s_framebuffer_size);

	if ((s_framebuffer_size > 0) && (s_framebuffer_size > 0))
	{
		gl.readPixels(0, 0, s_framebuffer_size, s_framebuffer_size, GL_RED, GL_FLOAT, pixels.data());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");
	}

	/* Check results.
	 Note: assuming that s_buffer_size == 2 -> then all points shall be drawn. */
	for (std::vector<glw::GLfloat>::iterator i = pixels.begin(); i != pixels.end(); ++i)
	{
		if (fabs(*i - 0.5f) > 0.0625f /* precision */)
		{
			return false;
		}
	}

	return true;
}

bool gl3cts::TransformFeedback::CaptureVertexInterleaved::checkTransformFeedbackBuffer(BindBufferCase bind_case UNUSED,
																					   glw::GLenum primitive_type)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check */
	glw::GLuint number_of_vertices = 0;

	switch (primitive_type)
	{
	case GL_POINTS:
		number_of_vertices = 4;
		break;
	case GL_LINES:
		number_of_vertices = 4;
		break;
	case GL_LINE_LOOP:
		number_of_vertices = 8;
		break;
	case GL_LINE_STRIP:
		number_of_vertices = 6;
		break;
	case GL_TRIANGLES:
		number_of_vertices = 6;
		break;
	case GL_TRIANGLE_STRIP:
		number_of_vertices = 6;
		break;
	case GL_TRIANGLE_FAN:
		number_of_vertices = 6;
		break;
	default:
		throw 0;
	}

	glw::GLfloat* results = (glw::GLfloat*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	bool is_ok = true;

	for (glw::GLuint j = 0; (j < number_of_vertices) && is_ok; ++j)
	{
		for (glw::GLint i = 0; i < m_max_transform_feedback_components - 4; ++i)
		{
			glw::GLfloat result	= results[i + j * m_max_transform_feedback_components];
			glw::GLfloat reference = (glw::GLfloat)(i);

			if (fabs(result - reference) > 0.125 /* precision */)
			{
				is_ok = false;

				break;
			}
		}

		/* gl_Position */
		glw::GLfloat result[4] = { results[(j + 1) * m_max_transform_feedback_components - 4],
								   results[(j + 1) * m_max_transform_feedback_components - 3],
								   results[(j + 1) * m_max_transform_feedback_components - 2],
								   results[(j + 1) * m_max_transform_feedback_components - 1] };

		if ((fabs(fabs(result[0]) - 1.0 + s_rasterization_epsilon) > 0.125 /* precision */) ||
			(fabs(fabs(result[1]) - 1.0 + s_rasterization_epsilon) > 0.125 /* precision */) ||
			(fabs(result[2]) > 0.125 /* precision */) || (fabs(result[3] - 1.0) > 0.125 /* precision */))
		{
			is_ok = false;

			break;
		}
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	return is_ok;
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::bindBuffer(BindBufferCase bind_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	switch (bind_case)
	{
	case BIND_BUFFER_BASE_CASE:
		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer);
		break;
	case BIND_BUFFER_RANGE_CASE:
		gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer, 0, m_buffer_size);
		break;
	case BIND_BUFFER_OFFSET_CASE:
		if (DE_NULL == m_glBindBufferOffsetEXT)
		{
			throw 0;
		}
		m_glBindBufferOffsetEXT(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer, 0);
		break;
	default:
		throw 0;
	}
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::clean(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_program)
	{
		gl.deleteProgram(m_program);

		m_program = 0;
	}

	if (m_framebuffer)
	{
		gl.deleteFramebuffers(1, &m_framebuffer);

		m_framebuffer = 0;
	}

	if (m_renderbuffer)
	{
		gl.deleteRenderbuffers(1, &m_renderbuffer);

		m_renderbuffer = 0;
	}

	cleanBuffer();

	if (m_vertex_array_object)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object);

		m_vertex_array_object = 0;
	}
}

void gl3cts::TransformFeedback::CaptureVertexInterleaved::cleanBuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_buffer)
	{
		gl.deleteBuffers(1, &m_buffer);

		m_buffer = 0;
	}
}

const glw::GLchar* gl3cts::TransformFeedback::CaptureVertexInterleaved::s_vertex_shader_source_code_template =
	"#version 130\n"
	"\n"
	"TEMPLATE_INPUT_OUTPUT_DECLARATIONS"
	"\n"
	"void main()\n"
	"{\n"
	"TEMPLATE_OUTPUT_SETTERS"
	"\n"
	"    vec4 position = vec4(0.0);\n"
	"\n"
	"    /* Note: The points are moved 0.0625 from the borders to\n"
	"             reduce non-XFB related rasterization problems. */\n"
	"    switch(gl_VertexID)\n"
	"    {\n"
	"        case 0:\n"
	"            position = vec4(-1.0 + TEMPLATE_RASTERIZATION_EPSILON,  1.0 - TEMPLATE_RASTERIZATION_EPSILON,  0.0,  "
	"1.0);\n"
	"            break;\n"
	"        case 1:\n"
	"            position = vec4( 1.0 - TEMPLATE_RASTERIZATION_EPSILON,  1.0 - TEMPLATE_RASTERIZATION_EPSILON,  0.0,  "
	"1.0);\n"
	"            break;\n"
	"        case 2:\n"
	"            position = vec4(-1.0 + TEMPLATE_RASTERIZATION_EPSILON, -1.0 + TEMPLATE_RASTERIZATION_EPSILON,  0.0,  "
	"1.0);\n"
	"            break;\n"
	"        case 3:\n"
	"            position = vec4( 1.0 - TEMPLATE_RASTERIZATION_EPSILON, -1.0 + TEMPLATE_RASTERIZATION_EPSILON,  0.0,  "
	"1.0);\n"
	"            break;\n"
	"    }\n"
	"\n"
	"    gl_Position = position;\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::CaptureVertexInterleaved::s_fragment_shader_source_code =
	"#version 130\n"
	"\n"
	"out vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    color = vec4(0.5);\n"
	"}\n";

const glw::GLuint
	gl3cts::TransformFeedback::CaptureVertexInterleaved::s_element_indices[][s_max_element_indices_count] = {
		{ 0, 1, 2, 3 },		  { 0, 1, 2, 3 }, { 0, 1, 3, 2 }, { 0, 1, 3, 2 },
		{ 2, 0, 1, 2, 1, 3 }, { 0, 1, 2, 3 }, { 2, 0, 1, 3 }
	};

const glw::GLuint gl3cts::TransformFeedback::CaptureVertexInterleaved::s_primitive_cases_count =
	sizeof(s_element_indices) / sizeof(s_element_indices[0]);

const glw::GLenum gl3cts::TransformFeedback::CaptureVertexInterleaved::s_primitive_cases[] = {
	GL_POINTS, GL_LINES, GL_LINE_LOOP, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN
};

const glw::GLenum gl3cts::TransformFeedback::CaptureVertexInterleaved::s_primitive_cases_xfb[] = {
	GL_POINTS, GL_LINES, GL_LINES, GL_LINES, GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES
};

const glw::GLuint gl3cts::TransformFeedback::CaptureVertexInterleaved::s_element_indices_counts[] = { 4, 4, 4, 4,
																									  6, 4, 4 };

const glw::GLuint gl3cts::TransformFeedback::CaptureVertexInterleaved::s_framebuffer_size =
	2; /* If you change this, update checkFramebuffer function according. */

const glw::GLfloat gl3cts::TransformFeedback::CaptureVertexInterleaved::s_rasterization_epsilon = 0.0625;

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::CaptureGeometryInterleaved::CaptureGeometryInterleaved(deqp::Context& context,
																				  const char*	test_name,
																				  const char*	test_description)
	: CaptureVertexInterleaved(context, test_name, test_description)
{
}

gl3cts::TransformFeedback::CaptureGeometryInterleaved::~CaptureGeometryInterleaved(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::CaptureGeometryInterleaved::iterate(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initializations. */
	bool is_at_least_gl_30 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)));
	bool is_at_least_gl_32 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 2)));
	bool is_ext_tf_1	   = m_context.getContextInfo().isExtensionSupported("GL_EXT_transform_feedback");
	bool is_arb_gs_4	   = m_context.getContextInfo().isExtensionSupported("GL_ARB_geometry_shader4");

	bool is_ok		= true;
	bool test_error = false;

	/* Tests. */
	try
	{
		if ((is_at_least_gl_30 || is_ext_tf_1) && (is_at_least_gl_32 || is_arb_gs_4))
		{
			fetchLimits();
			createFramebuffer();
			createTransformFeedbackBuffer();
			createVertexArrayObject();

			for (glw::GLuint i_primitive_case = 0;
				 (i_primitive_case < s_geometry_interleaved_primitive_cases_count) && is_ok; ++i_primitive_case)
			{
				buildProgram(i_primitive_case);

				gl.useProgram(m_program);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

				for (glw::GLint i_bind_case = 0; (i_bind_case < BIND_BUFFER_CASES_COUNT) && is_ok; ++i_bind_case)
				{
					if ((i_bind_case == BIND_BUFFER_OFFSET_CASE) && (DE_NULL == m_glBindBufferOffsetEXT))
					{
						continue;
					}

					bindBuffer((BindBufferCase)i_bind_case);

					draw(i_primitive_case);

					is_ok = is_ok && checkFramebuffer(s_primitive_cases[i_primitive_case]);
					is_ok = is_ok &&
							checkTransformFeedbackBuffer((BindBufferCase)i_bind_case,
														 s_geometry_interleaved_primitive_cases_xfb[i_primitive_case]);
				}

				gl.deleteProgram(m_program);

				m_program = 0;

				GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram call failed.");
			}
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Clean objects. */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Capture Geometry have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

void gl3cts::TransformFeedback::CaptureGeometryInterleaved::fetchLimits(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching limits. */
	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &m_max_transform_feedback_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (m_max_transform_feedback_components == 0)
	{
		throw 0;
	}

	glw::GLint max_geometry_total_components = 0;

	gl.getIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &max_geometry_total_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_geometry_total_components == 0)
	{
		throw 0;
	}

	if (m_max_transform_feedback_components * 4 > max_geometry_total_components)
	{
		m_max_transform_feedback_components = max_geometry_total_components / 4;
	}
}

void gl3cts::TransformFeedback::CaptureGeometryInterleaved::buildProgram(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing source code. */
	std::string geometry_shader(s_geometry_shader_source_code_template); /* Storage for vertex shader source code. */
	std::string transform_feedback_variable_declarations(
		""); /* String to contain all custom outputs from vertex shader. */
	std::string transform_feedback_variable_setters(
		""); /* String containing all initializations of custom outputs from vertex shader. */
	std::vector<std::string> transform_feedback_varyings(m_max_transform_feedback_components); /* Varyings array. */
	std::vector<const glw::GLchar*> transform_feedback_varyings_c(
		m_max_transform_feedback_components); /* Varyings array in C form to pass to the GL. */

	glw::GLint user_defined_transform_feedback_interleaved_varyings_count =
		m_max_transform_feedback_components /* total max to be written by the shader */
			/ 4								/* components per vec4 */
		//                                                                          / 4 /* number of vertices */
		- 1 /* gl_Position */;

	glw::GLint all_transform_feedback_interleaved_varyings_count =
		user_defined_transform_feedback_interleaved_varyings_count + 1 /* gl_Position */;

	/* Most of varyings is declarated output variables. */
	for (glw::GLint i = 0; i < user_defined_transform_feedback_interleaved_varyings_count; ++i)
	{
		/* Preparing variable name. */
		std::string varying = "result_";
		varying.append(gl3cts::TransformFeedback::Utilities::itoa(i));

		transform_feedback_varyings[i] = varying;

		transform_feedback_varyings_c[i] = transform_feedback_varyings[i].c_str();

		/* Preparing variable declaration. */
		transform_feedback_variable_declarations.append("out vec4 ");
		transform_feedback_variable_declarations.append(varying);
		transform_feedback_variable_declarations.append(";\n");

		/* Preparing variable setters. */
		transform_feedback_variable_setters.append("    ");
		transform_feedback_variable_setters.append(varying);
		transform_feedback_variable_setters.append(" = vec4(");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * 4));
		transform_feedback_variable_setters.append(".0, ");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * 4 + 1));
		transform_feedback_variable_setters.append(".0, ");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * 4 + 2));
		transform_feedback_variable_setters.append(".0, ");
		transform_feedback_variable_setters.append(gl3cts::TransformFeedback::Utilities::itoa(i * 4 + 3));
		transform_feedback_variable_setters.append(".0);\n");
	}

	/* Last four varying components are gl_Position components. */
	transform_feedback_varyings[user_defined_transform_feedback_interleaved_varyings_count /* gl_Position */] =
		"gl_Position";

	transform_feedback_varyings_c[user_defined_transform_feedback_interleaved_varyings_count /* gl_Position */] =
		transform_feedback_varyings[user_defined_transform_feedback_interleaved_varyings_count /* gl_Position */]
			.c_str();

	/* Preprocess vertex shader source code template. */
	geometry_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(
		geometry_shader, "TEMPLATE_PRIMITIVE_TYPE", s_geometry_interleaved_primitive_cases[primitive_case]);
	geometry_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(
		geometry_shader, "TEMPLATE_INPUT_OUTPUT_DECLARATIONS", transform_feedback_variable_declarations);
	geometry_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(geometry_shader, "TEMPLATE_OUTPUT_SETTERS",
																		   transform_feedback_variable_setters);
	geometry_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(
		geometry_shader, "TEMPLATE_RASTERIZATION_EPSILON",
		gl3cts::TransformFeedback::Utilities::ftoa(s_rasterization_epsilon));

	/* Compile, link and check. */
	m_program = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), geometry_shader.c_str(), NULL, NULL, s_blank_vertex_shader_source_code,
		s_fragment_shader_source_code, &transform_feedback_varyings_c[0],
		all_transform_feedback_interleaved_varyings_count, m_attrib_type);

	if (0 == m_program)
	{
		throw 0;
	}
}

void gl3cts::TransformFeedback::CaptureGeometryInterleaved::draw(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	gl.beginTransformFeedback(s_geometry_interleaved_primitive_cases_xfb[primitive_case]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_POINTS, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedbac call failed.");
}

const glw::GLchar* gl3cts::TransformFeedback::CaptureGeometryInterleaved::s_geometry_shader_source_code_template =
	"#version 150\n"
	"\n"
	"layout(points) in;\n"
	"layout(TEMPLATE_PRIMITIVE_TYPE, max_vertices = 4) out;\n"
	"\n"
	"TEMPLATE_INPUT_OUTPUT_DECLARATIONS"
	"\n"
	"void main()\n"
	"{\n"
	"    /* Note: The points are moved 0.0625 from the borders to\n"
	"             reduce non-XFB related rasterization problems. */\n"
	"\n"
	"    gl_Position = vec4(-1.0 + TEMPLATE_RASTERIZATION_EPSILON,  1.0 - TEMPLATE_RASTERIZATION_EPSILON,  0.0,  "
	"1.0);\n"
	"TEMPLATE_OUTPUT_SETTERS"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = vec4( 1.0 - TEMPLATE_RASTERIZATION_EPSILON,  1.0 - TEMPLATE_RASTERIZATION_EPSILON,  0.0,  "
	"1.0);\n"
	"TEMPLATE_OUTPUT_SETTERS"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = vec4(-1.0 + TEMPLATE_RASTERIZATION_EPSILON, -1.0 + TEMPLATE_RASTERIZATION_EPSILON,  0.0,  "
	"1.0);\n"
	"TEMPLATE_OUTPUT_SETTERS"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = vec4( 1.0 - TEMPLATE_RASTERIZATION_EPSILON, -1.0 + TEMPLATE_RASTERIZATION_EPSILON,  0.0,  "
	"1.0);\n"
	"TEMPLATE_OUTPUT_SETTERS"
	"    EmitVertex();\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::CaptureGeometryInterleaved::s_blank_vertex_shader_source_code =
	"#version 130\n"
	"\n"
	"void main()\n"
	"{\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::CaptureGeometryInterleaved::s_geometry_interleaved_primitive_cases[] = {
	"points", "line_strip", "triangle_strip"
};

const glw::GLenum gl3cts::TransformFeedback::CaptureGeometryInterleaved::s_geometry_interleaved_primitive_cases_xfb[] =
	{ GL_POINTS, GL_LINES, GL_TRIANGLES };

const glw::GLuint gl3cts::TransformFeedback::CaptureGeometryInterleaved::s_geometry_interleaved_primitive_cases_count =
	sizeof(s_geometry_interleaved_primitive_cases) / sizeof(s_geometry_interleaved_primitive_cases[0]);

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::CaptureVertexSeparate::CaptureVertexSeparate(deqp::Context& context, const char* test_name,
																		const char* test_description)
	: CaptureVertexInterleaved(context, test_name, test_description)
	, m_buffers(DE_NULL)
	, m_max_transform_feedback_separate_attribs(0)
{
	m_attrib_type = GL_SEPARATE_ATTRIBS;
}

void gl3cts::TransformFeedback::CaptureVertexSeparate::fetchLimits(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching limits. */
	glw::GLint max_transform_feedback_separate_components;

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, &max_transform_feedback_separate_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_transform_feedback_separate_components < 4)
	{
		throw 0;
	}

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &m_max_transform_feedback_separate_attribs);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (m_max_transform_feedback_separate_attribs == 0)
	{
		throw 0;
	}

	m_max_transform_feedback_components = m_max_transform_feedback_separate_attribs * 4 /* vec4 is used */;

	glw::GLint max_varyings_components = 0;

	gl.getIntegerv(GL_MAX_VARYING_COMPONENTS, &max_varyings_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if (max_varyings_components == 0)
	{
		throw 0;
	}

	if (m_max_transform_feedback_components > max_varyings_components)
	{
		m_max_transform_feedback_components = max_varyings_components;
	}
}

void gl3cts::TransformFeedback::CaptureVertexSeparate::createTransformFeedbackBuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_buffers = new glw::GLuint[m_max_transform_feedback_components];

	if (DE_NULL == m_buffers)
	{
		throw 0;
	}

	gl.genBuffers(m_max_transform_feedback_separate_attribs, m_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	m_buffer_size = static_cast<glw::GLuint>(m_max_vertices_drawn * 4 /* vec4 */ * sizeof(glw::GLfloat));

	for (glw::GLint i = 0; i < m_max_transform_feedback_separate_attribs; ++i)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffers[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer_size, NULL, GL_DYNAMIC_READ);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");
	}
}

void gl3cts::TransformFeedback::CaptureVertexSeparate::bindBuffer(BindBufferCase bind_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	switch (bind_case)
	{
	case BIND_BUFFER_BASE_CASE:
		for (glw::GLint i = 0; i < m_max_transform_feedback_separate_attribs; ++i)
		{
			gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, m_buffers[i]);
		}
		break;
	case BIND_BUFFER_RANGE_CASE:
		for (glw::GLint i = 0; i < m_max_transform_feedback_separate_attribs; ++i)
		{
			gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, i, m_buffers[i], 0, m_buffer_size);
		}
		break;
	case BIND_BUFFER_OFFSET_CASE:
		for (glw::GLint i = 0; i < m_max_transform_feedback_separate_attribs; ++i)
		{
			m_glBindBufferOffsetEXT(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffers[i], 0);
		}
		break;
	default:
		throw 0;
	}
}

void gl3cts::TransformFeedback::CaptureVertexSeparate::cleanBuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (DE_NULL != m_buffers)
	{
		gl.deleteBuffers(m_max_transform_feedback_separate_attribs, m_buffers);

		delete[] m_buffers;

		m_buffers = DE_NULL;
	}
}

bool gl3cts::TransformFeedback::CaptureVertexSeparate::checkTransformFeedbackBuffer(BindBufferCase bind_case UNUSED,
																					glw::GLenum primitive_type)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint number_of_vertices = 0;

	switch (primitive_type)
	{
	case GL_POINTS:
		number_of_vertices = 4;
		break;
	case GL_LINES:
		number_of_vertices = 4;
		break;
	case GL_LINE_LOOP:
		number_of_vertices = 8;
		break;
	case GL_LINE_STRIP:
		number_of_vertices = 6;
		break;
	case GL_TRIANGLES:
		number_of_vertices = 6;
		break;
	case GL_TRIANGLE_STRIP:
		number_of_vertices = 6;
		break;
	case GL_TRIANGLE_FAN:
		number_of_vertices = 6;
		break;
	default:
		throw 0;
	}

	bool is_ok = true;

	for (glw::GLint i = 0; i < m_max_transform_feedback_separate_attribs - 1; ++i)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffers[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		glw::GLfloat* results = (glw::GLfloat*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

		for (glw::GLuint j = 0; (j < number_of_vertices) && is_ok; ++j)
		{
			for (glw::GLuint k = 0; k < 4 /* vec4 */; ++k)
			{
				glw::GLfloat result	= results[j * 4 + k];
				glw::GLfloat reference = (glw::GLfloat)(i * 4 + k);

				if (fabs(result - reference) > 0.125 /* precision */)
				{
					is_ok = false;

					break;
				}
			}
		}

		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");
	}

	/* gl_Position */
	if (is_ok)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffers[m_max_transform_feedback_separate_attribs - 1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		glw::GLfloat* results = (glw::GLfloat*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

		for (glw::GLuint j = 0; (j < number_of_vertices) && is_ok; ++j)
		{
			glw::GLfloat result[4] = { results[j * 4], results[j * 4 + 1], results[j * 4 + 2], results[j * 4 + 3] };

			if ((fabs(fabs(result[0]) - 1.0 + s_rasterization_epsilon) > 0.125 /* precision */) ||
				(fabs(fabs(result[1]) - 1.0 + s_rasterization_epsilon) > 0.125 /* precision */) ||
				(fabs(result[2]) > 0.125 /* precision */) || (fabs(result[3] - 1.0) > 0.125 /* precision */))
			{
				is_ok = false;

				break;
			}
		}

		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");
	}

	return is_ok;
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::CaptureGeometrySeparate::CaptureGeometrySeparate(deqp::Context& context,
																			const char*	test_name,
																			const char*	test_description)
	: CaptureVertexInterleaved(context, test_name, test_description)
	, CaptureVertexSeparate(context, test_name, test_description)
	, CaptureGeometryInterleaved(context, test_name, test_description)
	, m_buffers(DE_NULL)
	, m_max_transform_feedback_separate_attribs(0)
{
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::CheckGetXFBVarying::CheckGetXFBVarying(deqp::Context& context, const char* test_name,
																  const char* test_description)
	: deqp::TestCase(context, test_name, test_description)
	, m_context(context)
	, m_max_xfb_interleaved_components(0)
	, m_max_xfb_separate_attributes(0)
	, m_max_xfb_separate_components(0)
	, m_max_varying_components(0)
	, m_max_varying_vectors(0)
	, m_max_geometry_total_output_components(0)
{
}

gl3cts::TransformFeedback::CheckGetXFBVarying::~CheckGetXFBVarying(void)
{
}

void gl3cts::TransformFeedback::CheckGetXFBVarying::fetchLimits(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching limits. */
	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &m_max_xfb_interleaved_components);
	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &m_max_xfb_separate_attributes);
	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, &m_max_xfb_separate_components);
	gl.getIntegerv(GL_MAX_VARYING_COMPONENTS, &m_max_varying_components);
	gl.getIntegerv(GL_MAX_VARYING_VECTORS, &m_max_varying_vectors);
	gl.getIntegerv(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, &m_max_geometry_total_output_components);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");
}

glw::GLuint gl3cts::TransformFeedback::CheckGetXFBVarying::numberOfAttributes(glw::GLuint capture_way,
																			  glw::GLuint shader_case,
																			  glw::GLuint varying_type)
{
	/* Setup limits of the case. */
	const glw::GLuint max_total_components =
		((s_shader_cases[shader_case].geometry_shader == DE_NULL) ? m_max_varying_components :
																	m_max_geometry_total_output_components) -
		4 /* gl_Position is not captured */;

	const glw::GLuint attribute_components = s_varying_types[varying_type].components_count;
	const glw::GLuint max_xfb_components   = (s_capture_ways[capture_way] == GL_INTERLEAVED_ATTRIBS) ?
											   m_max_xfb_interleaved_components :
											   (attribute_components * m_max_xfb_separate_components);

	if (s_capture_ways[capture_way] == GL_SEPARATE_ATTRIBS)
	{
		if (attribute_components > glw::GLuint(m_max_xfb_separate_components))
		{
			return 0;
		}
	}

	/* Setup number of attributes. */
	glw::GLuint number_of_attributes = max_xfb_components / attribute_components;

	if (s_capture_ways[capture_way] == GL_SEPARATE_ATTRIBS &&
		number_of_attributes > glw::GLuint(m_max_xfb_separate_attributes))
	{
		number_of_attributes = m_max_xfb_separate_attributes;
	}

	/* Clamp to limits. */
	if (number_of_attributes * attribute_components > max_total_components)
	{
		number_of_attributes = max_total_components / attribute_components;
	}

	/* Vectors limit. */
	if (attribute_components <= 4)
	{
		if (number_of_attributes > glw::GLuint(m_max_varying_vectors))
		{
			number_of_attributes = m_max_varying_vectors;
		}
	}
	else
	{
		if (number_of_attributes > glw::GLuint(m_max_varying_vectors) / 4)
		{
			number_of_attributes = glw::GLuint(m_max_varying_vectors) / 4;
		}
	}

	/* Return. */
	return number_of_attributes;
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::CheckGetXFBVarying::iterate(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initializations. */
	bool is_at_least_gl_30 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)));
	bool is_ext_tf_1	   = m_context.getContextInfo().isExtensionSupported("GL_EXT_transform_feedback");

	bool is_ok		= true;
	bool test_error = false;

	glw::GLuint program = 0;

	/* Tests. */
	try
	{
		if (is_at_least_gl_30 || is_ext_tf_1)
		{
			fetchLimits();

			for (glw::GLuint i = 0; (i < s_capture_ways_count) && is_ok; ++i)
			{
				for (glw::GLuint j = 0; (j < s_shader_cases_count) && is_ok; ++j)
				{
					for (glw::GLuint k = 0; (k < s_varying_types_count) && is_ok; ++k)
					{
						glw::GLuint n = numberOfAttributes(i, j, k);

						if (n)
						{
							program = buildProgram(i, j, k, n);

							is_ok = is_ok && (program != 0);

							is_ok = is_ok && check(program, i, j, k, n);

							gl.deleteProgram(program);

							GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram call failed.");

							program = 0;
						}
					}
				}
			}
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;

		if (program)
		{
			gl.deleteProgram(program);

			program = 0;
		}
	}

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Test checking Get Transform Feedback Varying have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

glw::GLuint gl3cts::TransformFeedback::CheckGetXFBVarying::buildProgram(glw::GLuint capture_way,
																		glw::GLuint shader_case,
																		glw::GLuint varying_type,
																		glw::GLuint number_of_attributes)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Preparing source code. */
	std::string						xfb_variable_declarations("");
	std::string						xfb_variable_setters("");
	std::vector<std::string>		xfb_varyings(number_of_attributes);
	std::vector<const glw::GLchar*> xfb_varyings_c(number_of_attributes);

	/* Most of varyings is declarated output variables. */
	for (glw::GLuint i = 0; i < number_of_attributes; ++i)
	{
		/* Varying name: result_# */
		std::string varying = "result_";
		varying.append(gl3cts::TransformFeedback::Utilities::itoa(i));

		xfb_varyings[i]   = varying;
		xfb_varyings_c[i] = xfb_varyings[i].c_str();

		/* Varying declaration: out TYPE result_#;*/
		xfb_variable_declarations.append("out ");
		xfb_variable_declarations.append(s_varying_types[varying_type].name);
		xfb_variable_declarations.append(" ");
		xfb_variable_declarations.append(varying);
		xfb_variable_declarations.append(";\n");

		/* Varying setter: result_# = TYPE(#); */
		xfb_variable_setters.append("    ");
		xfb_variable_setters.append(varying);
		xfb_variable_setters.append(" = ");
		xfb_variable_setters.append(s_varying_types[varying_type].name);
		xfb_variable_setters.append("(");
		xfb_variable_setters.append("2"); //gl3cts::TransformFeedback::Utilities::itoa(i));
		if (s_varying_types[varying_type].float_component)
		{
			/* if varying is float varying setter is: result_# = TYPE(#.0); */
			xfb_variable_setters.append(".0");
		}
		xfb_variable_setters.append(");\n");
	}

	/* Preprocess vertex shader source code template. */
	const glw::GLchar* vertex_shader   = s_shader_cases[shader_case].vertex_shader;
	const glw::GLchar* geometry_shader = s_shader_cases[shader_case].geometry_shader;

	std::string xfb_shader;

	if (DE_NULL == s_shader_cases[shader_case].geometry_shader)
	{
		/* XFB tested in vertex shader. */
		xfb_shader = vertex_shader;
	}
	else
	{
		/* XFB tested in geometry shader. */
		xfb_shader = geometry_shader;
	}

	/* Preprocess shader. */
	xfb_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(xfb_shader, "TEMPLATE_OUTPUT_DECLARATIONS",
																	  xfb_variable_declarations);
	xfb_shader = gl3cts::TransformFeedback::Utilities::preprocessCode(xfb_shader, "TEMPLATE_OUTPUT_SETTERS",
																	  xfb_variable_setters);

	if (DE_NULL == s_shader_cases[shader_case].geometry_shader)
	{
		/* XFB tested in vertex shader. */
		vertex_shader = xfb_shader.c_str();
	}
	else
	{
		/* XFB tested in geometry shader. */
		geometry_shader = xfb_shader.c_str();
	}

	/* Compile, link and check. */
	glw::GLuint program = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), geometry_shader, NULL, NULL, vertex_shader, s_generic_fragment_shader,
		&xfb_varyings_c[0], number_of_attributes, s_capture_ways[capture_way]);

	/* Check compilation status. */
	if (0 == program)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Building program has failed.\nVertex shader:\n"
											<< vertex_shader << "Geometry shader:\n"
											<< ((DE_NULL == geometry_shader) ? "" : geometry_shader)
											<< "Fragment shader:\n"
											<< s_generic_fragment_shader << tcu::TestLog::EndMessage;

		throw 0;
	}

	return program;
}

bool gl3cts::TransformFeedback::CheckGetXFBVarying::check(glw::GLuint program, glw::GLuint capture_way,
														  glw::GLuint shader_case UNUSED, glw::GLuint varying_type,
														  glw::GLuint number_of_attributes)
{
	/* Functions handler */
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();
	glw::GLuint			  max_length = 0;

	/* Inspect glGetTransformFeedbackVarying. */
	for (glw::GLuint i = 0; i < number_of_attributes; ++i)
	{
		const glw::GLsizei bufSize  = 18;
		glw::GLsizei	   length   = 0;
		glw::GLsizei	   size		= 0;
		glw::GLenum		   type		= GL_NONE;
		glw::GLchar		   name[18] = { 0 }; /* Size of bufSize. */

		gl.getTransformFeedbackVarying(program, i, bufSize, &length, &size, &type, name);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTransformFeedbackVarying call failed.");

		max_length = deMaxu32(max_length, glw::GLuint(length));

		/* Check name. */
		if (length)
		{
			std::string varying		= name;
			std::string varying_ref = "result_";
			varying_ref.append(gl3cts::TransformFeedback::Utilities::itoa(i));

			if (0 != varying.compare(varying_ref))
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		/* Check size. */
		const glw::GLuint size_ref = 1;

		if (size != size_ref)
		{
			return false;
		}

		/* Check type. */
		if (type != s_varying_types[varying_type].type)
		{
			return false;
		}
	}

	/* Inspect glGetProgramiv. */
	glw::GLint xfb_varyings			  = 0;
	glw::GLint xfb_mode				  = 0;
	glw::GLint xfb_varying_max_length = 0;

	gl.getProgramiv(program, GL_TRANSFORM_FEEDBACK_VARYINGS, &xfb_varyings);
	gl.getProgramiv(program, GL_TRANSFORM_FEEDBACK_BUFFER_MODE, &xfb_mode);
	gl.getProgramiv(program, GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH, &xfb_varying_max_length);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

	if (glw::GLuint(xfb_varyings) != number_of_attributes)
	{
		return false;
	}

	if (glw::GLenum(xfb_mode) != s_capture_ways[capture_way])
	{
		return false;
	}

	if (glw::GLuint(xfb_varying_max_length) < max_length)
	{
		return false;
	}

	return true;
}

const glw::GLchar* gl3cts::TransformFeedback::CheckGetXFBVarying::s_generic_fragment_shader = "#version 130\n"
																							  "\n"
																							  "out vec4 color;\n"
																							  "\n"
																							  "void main()\n"
																							  "{\n"
																							  "    color = vec4(1.0);\n"
																							  "}\n";

const struct gl3cts::TransformFeedback::CheckGetXFBVarying::ShaderCase
	gl3cts::TransformFeedback::CheckGetXFBVarying::s_shader_cases[] = { { /* Vertex Shader. */
																		  "#version 130\n"
																		  "\n"
																		  "TEMPLATE_OUTPUT_DECLARATIONS"
																		  "\n"
																		  "void main()\n"
																		  "{\n"
																		  "    gl_Position = vec4(1.0);\n"
																		  "TEMPLATE_OUTPUT_SETTERS"
																		  "}\n",

																		  /* Geometry Shader. */
																		  NULL },
																		{ /* Vertex Shader. */
																		  "#version 130\n"
																		  "\n"
																		  "void main()\n"
																		  "{\n"
																		  "}\n",

																		  /* Geometry Shader. */
																		  "#version 150\n"
																		  "\n"
																		  "layout(points) in;\n"
																		  "layout(points, max_vertices = 1) out;\n"
																		  "\n"
																		  "TEMPLATE_OUTPUT_DECLARATIONS"
																		  "\n"
																		  "void main()\n"
																		  "{\n"
																		  "    gl_Position = vec4(1.0);\n"
																		  "TEMPLATE_OUTPUT_SETTERS"
																		  "    EmitVertex();\n"
																		  "}\n" } };

const glw::GLuint gl3cts::TransformFeedback::CheckGetXFBVarying::s_shader_cases_count =
	sizeof(s_shader_cases) / sizeof(s_shader_cases[0]);

const struct gl3cts::TransformFeedback::CheckGetXFBVarying::VaryingType
	gl3cts::TransformFeedback::CheckGetXFBVarying::s_varying_types[] = {
		/* type,				name,		#components,	is component float */
		{ GL_FLOAT, "float", 1, true },
		{ GL_FLOAT_VEC2, "vec2", 2, true },
		{ GL_FLOAT_VEC3, "vec3", 3, true },
		{ GL_FLOAT_VEC4, "vec4", 4, true },
		{ GL_INT, "int", 1, false },
		{ GL_INT_VEC2, "ivec2", 2, false },
		{ GL_INT_VEC3, "ivec3", 3, false },
		{ GL_INT_VEC4, "ivec4", 4, false },
		{ GL_UNSIGNED_INT, "uint", 1, false },
		{ GL_UNSIGNED_INT_VEC2, "uvec2", 2, false },
		{ GL_UNSIGNED_INT_VEC3, "uvec3", 3, false },
		{ GL_UNSIGNED_INT_VEC4, "uvec4", 4, false },
		{ GL_FLOAT_MAT2, "mat2", 4, true },
		{ GL_FLOAT_MAT3, "mat3", 9, true },
		{ GL_FLOAT_MAT4, "mat4", 16, true }
	};

const glw::GLuint gl3cts::TransformFeedback::CheckGetXFBVarying::s_varying_types_count =
	sizeof(s_varying_types) / sizeof(s_varying_types[0]);

const glw::GLenum gl3cts::TransformFeedback::CheckGetXFBVarying::s_capture_ways[] = { GL_INTERLEAVED_ATTRIBS,
																					  GL_SEPARATE_ATTRIBS };

const glw::GLuint gl3cts::TransformFeedback::CheckGetXFBVarying::s_capture_ways_count =
	sizeof(s_capture_ways) / sizeof(s_capture_ways[0]);

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::QueryVertexInterleaved::QueryVertexInterleaved(deqp::Context& context, const char* test_name,
																		  const char* test_description)
	: CaptureVertexInterleaved(context, test_name, test_description), m_query_object(0)
{
	m_max_vertices_drawn = 3; /* Make buffer smaller up to 3 vertices. */
}

void gl3cts::TransformFeedback::QueryVertexInterleaved::createTransformFeedbackBuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create buffer object. */
	gl3cts::TransformFeedback::CaptureVertexInterleaved::createTransformFeedbackBuffer();

	/* Create query object. */
	gl.genQueries(1, &m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries call failed.");
}

void gl3cts::TransformFeedback::QueryVertexInterleaved::draw(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.beginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQuery call failed.");

	gl3cts::TransformFeedback::CaptureVertexInterleaved::draw(primitive_case);

	gl.endQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery call failed.");
}

bool gl3cts::TransformFeedback::QueryVertexInterleaved::checkTransformFeedbackBuffer(BindBufferCase bind_case UNUSED,
																					 glw::GLenum primitive_type)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint number_of_primitives;

	gl.getQueryObjectuiv(m_query_object, GL_QUERY_RESULT, &number_of_primitives);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryObjectuiv call failed.");

	/* expected result */
	glw::GLuint number_of_primitives_reference = (primitive_type == GL_POINTS) ? 3 : 1; /* m_max_vertices_drawn == 3 */

	if (number_of_primitives_reference != number_of_primitives)
	{
		return false;
	}

	return true;
}

void gl3cts::TransformFeedback::QueryVertexInterleaved::clean(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete query object. */
	gl.deleteQueries(1, &m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteQueries call failed.");

	/* Other */
	gl3cts::TransformFeedback::CaptureVertexInterleaved::clean();
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::QueryGeometryInterleaved::QueryGeometryInterleaved(deqp::Context& context,
																			  const char*	test_name,
																			  const char*	test_description)
	: CaptureVertexInterleaved(context, test_name, test_description)
	, CaptureGeometryInterleaved(context, test_name, test_description)
{
	m_query_object		 = 0;
	m_max_vertices_drawn = 3; /* Make buffer smaller up to 3 vertices. */
}

void gl3cts::TransformFeedback::QueryGeometryInterleaved::createTransformFeedbackBuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create buffer object. */
	gl3cts::TransformFeedback::CaptureGeometryInterleaved::createTransformFeedbackBuffer();

	/* Create query object. */
	gl.genQueries(1, &m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries call failed.");
}

void gl3cts::TransformFeedback::QueryGeometryInterleaved::draw(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.beginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQuery call failed.");

	gl3cts::TransformFeedback::CaptureGeometryInterleaved::draw(primitive_case);

	gl.endQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery call failed.");
}

bool gl3cts::TransformFeedback::QueryGeometryInterleaved::checkTransformFeedbackBuffer(BindBufferCase bind_case UNUSED,
																					   glw::GLenum primitive_type)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint number_of_primitives;

	gl.getQueryObjectuiv(m_query_object, GL_QUERY_RESULT, &number_of_primitives);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryObjectuiv call failed.");

	/* expected result */
	glw::GLuint number_of_primitives_reference = (primitive_type == GL_POINTS) ? 3 : 1; /* m_max_vertices_drawn == 3 */

	if (number_of_primitives_reference != number_of_primitives)
	{
		return false;
	}

	return true;
}

void gl3cts::TransformFeedback::QueryGeometryInterleaved::clean(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete query object. */
	gl.deleteQueries(1, &m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteQueries call failed.");

	/* Other. */
	gl3cts::TransformFeedback::CaptureGeometryInterleaved::clean();
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::QueryVertexSeparate::QueryVertexSeparate(deqp::Context& context, const char* test_name,
																	const char* test_description)
	: CaptureVertexInterleaved(context, test_name, test_description)
	, CaptureVertexSeparate(context, test_name, test_description)
{
	m_query_object		 = 0;
	m_max_vertices_drawn = 3; /* Make buffer smaller up to 3 vertices. */
}

void gl3cts::TransformFeedback::QueryVertexSeparate::createTransformFeedbackBuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create buffer object. */
	gl3cts::TransformFeedback::CaptureVertexSeparate::createTransformFeedbackBuffer();

	/* Create query object. */
	gl.genQueries(1, &m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries call failed.");
}

void gl3cts::TransformFeedback::QueryVertexSeparate::draw(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.beginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQuery call failed.");

	gl3cts::TransformFeedback::CaptureVertexSeparate::draw(primitive_case);

	gl.endQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery call failed.");
}

bool gl3cts::TransformFeedback::QueryVertexSeparate::checkTransformFeedbackBuffer(BindBufferCase bind_case UNUSED,
																				  glw::GLenum primitive_type)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint number_of_primitives;

	gl.getQueryObjectuiv(m_query_object, GL_QUERY_RESULT, &number_of_primitives);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryObjectuiv call failed.");

	/* expected result */
	glw::GLuint number_of_primitives_reference = (primitive_type == GL_POINTS) ? 3 : 1; /* m_max_vertices_drawn == 3 */

	if (number_of_primitives_reference != number_of_primitives)
	{
		return false;
	}

	return true;
}

void gl3cts::TransformFeedback::QueryVertexSeparate::clean(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete query object. */
	gl.deleteQueries(1, &m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteQueries call failed.");

	/* Other */
	gl3cts::TransformFeedback::CaptureVertexSeparate::clean();
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::QueryGeometrySeparate::QueryGeometrySeparate(deqp::Context& context, const char* test_name,
																		const char* test_description)
	: CaptureVertexInterleaved(context, test_name, test_description)
	, CaptureVertexSeparate(context, test_name, test_description)
	, CaptureGeometrySeparate(context, test_name, test_description)
{
	m_query_object		 = 0;
	m_max_vertices_drawn = 3; /* Make buffer smaller up to 3 vertices. */
}

void gl3cts::TransformFeedback::QueryGeometrySeparate::createTransformFeedbackBuffer(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create buffer object. */
	gl3cts::TransformFeedback::CaptureGeometrySeparate::createTransformFeedbackBuffer();

	/* Create query object. */
	gl.genQueries(1, &m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries call failed.");
}

void gl3cts::TransformFeedback::QueryGeometrySeparate::draw(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.beginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQuery call failed.");

	gl3cts::TransformFeedback::CaptureGeometrySeparate::draw(primitive_case);

	gl.endQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery call failed.");
}

bool gl3cts::TransformFeedback::QueryGeometrySeparate::checkTransformFeedbackBuffer(BindBufferCase bind_case UNUSED,
																					glw::GLenum primitive_type)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint number_of_primitives;

	gl.getQueryObjectuiv(m_query_object, GL_QUERY_RESULT, &number_of_primitives);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryObjectuiv call failed.");

	/* expected result */
	glw::GLuint number_of_primitives_reference = (primitive_type == GL_POINTS) ? 3 : 1; /* m_max_vertices_drawn == 3 */

	if (number_of_primitives_reference != number_of_primitives)
	{
		return false;
	}

	return true;
}

void gl3cts::TransformFeedback::QueryGeometrySeparate::clean(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete query object. */
	gl.deleteQueries(1, &m_query_object);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteQueries call failed.");

	/* Other */
	gl3cts::TransformFeedback::CaptureGeometrySeparate::clean();
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::DiscardVertex::DiscardVertex(deqp::Context& context, const char* test_name,
														const char* test_description)
	: CaptureVertexInterleaved(context, test_name, test_description)
{
}

void gl3cts::TransformFeedback::DiscardVertex::draw(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Must clear before rasterizer discard */
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl3cts::TransformFeedback::CaptureVertexInterleaved::draw(primitive_case);

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");
}

bool gl3cts::TransformFeedback::DiscardVertex::checkFramebuffer(glw::GLuint primitive_case UNUSED)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch framebuffer. */
	std::vector<glw::GLfloat> pixels(s_framebuffer_size * s_framebuffer_size);

	if ((s_framebuffer_size > 0) && (s_framebuffer_size > 0))
	{
		gl.readPixels(0, 0, s_framebuffer_size, s_framebuffer_size, GL_RED, GL_FLOAT, pixels.data());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");
	}

	/* Check results.
	 Note: assuming that s_buffer_size == 2 -> then all points shall be drawn. */
	for (std::vector<glw::GLfloat>::iterator i = pixels.begin(); i != pixels.end(); ++i)
	{
		if (fabs(*i) > 0.0625f /* precision */)
		{
			return false;
		}
	}

	return true;
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::DiscardGeometry::DiscardGeometry(deqp::Context& context, const char* test_name,
															const char* test_description)
	: CaptureVertexInterleaved(context, test_name, test_description)
	, CaptureGeometryInterleaved(context, test_name, test_description)
{
}

void gl3cts::TransformFeedback::DiscardGeometry::draw(glw::GLuint primitive_case)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Must clear before rasterizer discard */
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl3cts::TransformFeedback::CaptureGeometryInterleaved::draw(primitive_case);

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");
}

bool gl3cts::TransformFeedback::DiscardGeometry::checkFramebuffer(glw::GLuint primitive_case UNUSED)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch framebuffer. */
	std::vector<glw::GLfloat> pixels(s_framebuffer_size * s_framebuffer_size);

	if ((s_framebuffer_size > 0) && (s_framebuffer_size > 0))
	{
		gl.readPixels(0, 0, s_framebuffer_size, s_framebuffer_size, GL_RED, GL_FLOAT, pixels.data());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");
	}

	/* Check results.
	 Note: assuming that s_buffer_size == 2 -> then all points shall be drawn. */
	for (std::vector<glw::GLfloat>::iterator i = pixels.begin(); i != pixels.end(); ++i)
	{
		if (fabs(*i) > 0.0625f /* precision */)
		{
			return false;
		}
	}

	return true;
}

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::DrawXFB::DrawXFB(deqp::Context& context, const char* test_name, const char* test_description)
	: deqp::TestCase(context, test_name, test_description)
	, m_context(context)
	, m_program_id_xfb(0)
	, m_program_id_draw(0)
	, m_fbo_id(0)
	, m_rbo_id(0)
	, m_vao_id(0)
{
	memset(m_xfb_id, 0, sizeof(m_xfb_id));
	memset(m_bo_id, 0, sizeof(m_bo_id));
}

gl3cts::TransformFeedback::DrawXFB::~DrawXFB(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::DrawXFB::iterate(void)
{
	/* Initializations. */
	bool is_at_least_gl_40 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)));
	bool is_arb_tf_2	   = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback2");

	bool is_ok		= true;
	bool test_error = false;

	/* Tests. */
	try
	{
		if (is_at_least_gl_40 || is_arb_tf_2)
		{
			for (glw::GLuint i = 0; (i < s_capture_modes_count) && is_ok; ++i)
			{
				prepare(s_capture_modes[i]);

				bindVAO(m_vao_id);
				useProgram(m_program_id_xfb);

				for (glw::GLuint j = 0; (j < s_xfb_count) && is_ok; ++j)
				{
					bindXFB(m_xfb_id[j]);
					bindBOForXFB(s_capture_modes[i], m_bo_id[j]);
					useColour(m_program_id_xfb, s_colours[j][0], s_colours[j][1], s_colours[j][2], s_colours[j][3]);
					useGeometrySet(m_program_id_xfb, false);
					drawForCapture(true, true, false, false);

					is_ok = is_ok && inspectXFBState(true, true);
				}

				for (glw::GLuint j = 0; (j < s_xfb_count) && is_ok; ++j)
				{
					bindXFB(m_xfb_id[j]);
					useColour(m_program_id_xfb, s_colours[j][0], s_colours[j][1], s_colours[j][2], s_colours[j][3]);
					useGeometrySet(m_program_id_xfb, true);
					drawForCapture(false, false, true, true);

					is_ok = is_ok && inspectXFBState(false, false);
				}

				useProgram(m_program_id_draw);

				for (glw::GLuint j = 0; (j < s_xfb_count) && is_ok; ++j)
				{
					bindXFB(m_xfb_id[j]);
					bindBOForDraw(m_program_id_draw, s_capture_modes[i], m_bo_id[j]);
					drawToFramebuffer(m_xfb_id[j]);

					is_ok =
						is_ok && checkFramebuffer(s_colours[j][0], s_colours[j][1], s_colours[j][2], s_colours[j][3]);
				}

				clean();
			}
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
		clean();
	}

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			/* Log error. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB have approached error."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			/* Log fail. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB have failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

void gl3cts::TransformFeedback::DrawXFB::prepare(glw::GLenum capture_mode)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare programs. */
	m_program_id_xfb = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader_xfb, s_fragment_shader,
		s_xfb_varyings, s_xfb_varyings_count, capture_mode);

	if (0 == m_program_id_xfb)
	{
		throw 0;
	}

	m_program_id_draw = gl3cts::TransformFeedback::Utilities::buildProgram(gl, m_context.getTestContext().getLog(),
																		   NULL, NULL, NULL, s_vertex_shader_draw,
																		   s_fragment_shader, NULL, 0, capture_mode);

	if (0 == m_program_id_draw)
	{
		throw 0;
	}

	/* Prepare transform feedbacks. */
	gl.genTransformFeedbacks(s_xfb_count, m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks call failed.");

	/* Prepare buffer objects. */
	gl.genBuffers(s_xfb_count, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	for (glw::GLuint i = 0; i < s_xfb_count; ++i)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_capture_size, NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");
	}

	/* Prepare framebuffer. */
	gl.clearColor(0.f, 0.f, 0.f, 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Create empty Vertex Array Object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");
}

void gl3cts::TransformFeedback::DrawXFB::bindXFB(glw::GLuint xfb_id)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback call failed.");
}

void gl3cts::TransformFeedback::DrawXFB::bindVAO(glw::GLuint vao_id)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

void gl3cts::TransformFeedback::DrawXFB::bindBOForXFB(glw::GLenum capture_mode, glw::GLuint bo_id)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	switch (capture_mode)
	{
	case GL_INTERLEAVED_ATTRIBS:
		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");
		break;
	case GL_SEPARATE_ATTRIBS:
		for (glw::GLuint i = 0; i < s_xfb_varyings_count; ++i)
		{
			gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, i, bo_id, i * s_capture_size / s_xfb_varyings_count,
							   (i + 1) * s_capture_size / s_xfb_varyings_count);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");
		}
		break;
	default:
		throw 0;
	};
}

void gl3cts::TransformFeedback::DrawXFB::bindBOForDraw(glw::GLuint program_id, glw::GLenum capture_mode,
													   glw::GLuint bo_id)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(GL_ARRAY_BUFFER, bo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	glw::GLuint position_location = gl.getAttribLocation(program_id, "position");
	glw::GLuint color_location	= gl.getAttribLocation(program_id, "color");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation call failed.");

	glw::GLvoid* color_offset = (capture_mode == GL_INTERLEAVED_ATTRIBS) ?
									(glw::GLvoid*)(4 /* components */ * sizeof(glw::GLfloat)) :
									(glw::GLvoid*)(4 /* components */ * 6 /* vertices */ * sizeof(glw::GLfloat));

	glw::GLuint stride =
		static_cast<glw::GLuint>((capture_mode == GL_INTERLEAVED_ATTRIBS) ?
									 (4 /* components */ * 2 /* position and color */ * sizeof(glw::GLfloat)) :
									 (4 /* components */ * sizeof(glw::GLfloat)));

	gl.vertexAttribPointer(position_location, 4, GL_FLOAT, GL_FALSE, stride, NULL);
	gl.vertexAttribPointer(color_location, 4, GL_FLOAT, GL_FALSE, stride, color_offset);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(position_location);
	gl.enableVertexAttribArray(color_location);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");
}

void gl3cts::TransformFeedback::DrawXFB::useProgram(glw::GLuint program_id)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");
}

void gl3cts::TransformFeedback::DrawXFB::useColour(glw::GLuint program_id, glw::GLfloat r, glw::GLfloat g,
												   glw::GLfloat b, glw::GLfloat a)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint location = gl.getUniformLocation(program_id, "color");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation call failed.");

	gl.uniform4f(location, r, g, b, a);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4f call failed.");
}

void gl3cts::TransformFeedback::DrawXFB::useGeometrySet(glw::GLuint program_id, bool invert_sign)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint location = gl.getUniformLocation(program_id, "invert_sign");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation call failed.");

	gl.uniform1f(location, invert_sign ? -1.f : 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4f call failed.");
}

void gl3cts::TransformFeedback::DrawXFB::clean()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_program_id_xfb)
	{
		gl.deleteProgram(m_program_id_xfb);

		m_program_id_xfb = 0;
	}

	if (m_program_id_draw)
	{
		gl.deleteProgram(m_program_id_draw);

		m_program_id_draw = 1;
	}

	for (glw::GLuint i = 0; i < s_xfb_count; ++i)
	{
		if (m_xfb_id[i])
		{
			gl.deleteTransformFeedbacks(1, &m_xfb_id[i]);

			m_xfb_id[i] = 0;
		}
	}

	for (glw::GLuint i = 0; i < s_xfb_count; ++i)
	{
		if (m_bo_id[i])
		{
			gl.deleteBuffers(1, &m_bo_id[i]);

			m_bo_id[i] = 0;
		}
	}

	if (m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_fbo_id)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_rbo_id)
	{
		gl.deleteRenderbuffers(1, &m_rbo_id);

		m_rbo_id = 0;
	}
}

void gl3cts::TransformFeedback::DrawXFB::drawForCapture(bool begin_xfb, bool pause_xfb, bool resume_xfb, bool end_xfb)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	if (begin_xfb)
	{
		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");
	}

	if (resume_xfb)
	{
		gl.resumeTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glResumeTransformFeedback call failed.");
	}

	gl.drawArrays(GL_POINTS, 0, 3);

	if (pause_xfb)
	{
		gl.pauseTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glPauseTransformFeedback call failed.");
	}

	if (end_xfb)
	{
		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");
	}

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");
}

void gl3cts::TransformFeedback::DrawXFB::drawToFramebuffer(glw::GLuint xfb_id)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.clearColor(0.f, 0.f, 0.f, 0.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	gl.drawTransformFeedback(GL_TRIANGLES, xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawTransformFeedback call failed.");
}

bool gl3cts::TransformFeedback::DrawXFB::checkFramebuffer(glw::GLfloat r, glw::GLfloat g, glw::GLfloat b,
														  glw::GLfloat a)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Number of pixels. */
	const glw::GLuint number_of_pixels = s_view_size * s_view_size;

	/* Fetch framebuffer. */
	std::vector<glw::GLubyte> pixels(number_of_pixels * 4 /* components */);

	if ((s_view_size > 0) && (s_view_size > 0))
	{
		gl.readPixels(0, 0, s_view_size, s_view_size, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");
	}

	/* Convert color to integer. */
	glw::GLubyte ir = (glw::GLubyte)(255.f * r);
	glw::GLubyte ig = (glw::GLubyte)(255.f * g);
	glw::GLubyte ib = (glw::GLubyte)(255.f * b);
	glw::GLubyte ia = (glw::GLubyte)(255.f * a);

	/* Check results. */
	for (glw::GLuint i = 0; i < number_of_pixels; ++i)
	{
		if ((pixels[i * 4 /* components */] != ir) || (pixels[i * 4 /* components */ + 1] != ig) ||
			(pixels[i * 4 /* components */ + 2] != ib) || (pixels[i * 4 /* components */ + 3] != ia))
		{
			return false;
		}
	}

	return true;
}

bool gl3cts::TransformFeedback::DrawXFB::inspectXFBState(bool shall_be_paused, bool shall_be_active)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint is_paused = 0;
	glw::GLint is_active = 0;

	gl.getIntegerv(GL_TRANSFORM_FEEDBACK_PAUSED, &is_paused);
	gl.getIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE, &is_active);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

	if ((is_paused == -1) || (is_active == -1))
	{
		throw 0;
	}

	if (shall_be_paused ^ (is_paused == GL_TRUE))
	{
		return false;
	}

	if (shall_be_active ^ (is_active == GL_TRUE))
	{
		return false;
	}

	return true;
}

const glw::GLchar* gl3cts::TransformFeedback::DrawXFB::s_vertex_shader_xfb =
	"#version 130\n"
	"\n"
	"uniform vec4 color;\n"
	"uniform float invert_sign;\n"
	"out     vec4 colour;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    switch(gl_VertexID)\n"
	"    {\n"
	"        case 0:\n"
	"            gl_Position = vec4(-1.0,               -1.0,              0.0,  1.0);\n"
	"        break;\n"
	"        case 1:\n"
	"            gl_Position = vec4(-1.0 * invert_sign,  1.0,              0.0,  1.0);\n"
	"        break;\n"
	"        case 2:\n"
	"            gl_Position = vec4( 1.0,                1.0 * invert_sign, 0.0,  1.0);\n"
	"        break;\n"
	"    }\n"
	"\n"
	"    colour = color;\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFB::s_vertex_shader_draw = "#version 130\n"
																			  "\n"
																			  "in  vec4 color;\n"
																			  "in  vec4 position;\n"
																			  "out vec4 colour;\n"
																			  "\n"
																			  "void main()\n"
																			  "{\n"
																			  "    gl_Position = position;\n"
																			  "    colour      = color;\n"
																			  "}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFB::s_fragment_shader = "#version 130\n"
																		   "\n"
																		   "in  vec4 colour;\n"
																		   "out vec4 pixel;\n"
																		   "\n"
																		   "void main()\n"
																		   "{\n"
																		   "    pixel = colour;\n"
																		   "}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFB::s_xfb_varyings[s_xfb_varyings_count] = { "gl_Position",
																								"colour" };

const glw::GLenum gl3cts::TransformFeedback::DrawXFB::s_capture_modes[] = { GL_INTERLEAVED_ATTRIBS,
																			GL_SEPARATE_ATTRIBS };
const glw::GLuint gl3cts::TransformFeedback::DrawXFB::s_capture_modes_count =
	sizeof(s_capture_modes) / sizeof(s_capture_modes[0]);

const glw::GLfloat gl3cts::TransformFeedback::DrawXFB::s_colours[s_xfb_count][4] = { { 1.f, 0.f, 0.f, 1.f },
																					 { 0.f, 1.f, 0.f, 1.f },
																					 { 0.f, 0.f, 1.f, 1.f } };

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::DrawXFBFeedback::DrawXFBFeedback(deqp::Context& context, const char* test_name,
															const char* test_description)
	: deqp::TestCase(context, test_name, test_description)
	, m_context(context)
	, m_program_id(0)
	, m_xfb_id(0)
	, m_source_bo_index(0)
{
	memset(m_bo_id, 1, sizeof(m_bo_id));
	memset(m_bo_id, 1, sizeof(m_vao_id));
}

gl3cts::TransformFeedback::DrawXFBFeedback::~DrawXFBFeedback(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::DrawXFBFeedback::iterate(void)
{
	/* Initializations. */
	bool is_at_least_gl_40 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)));
	bool is_arb_tf_2	   = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback2");

	bool is_ok		= true;
	bool test_error = false;

	/* Tests. */
	try
	{
		if (is_at_least_gl_40 || is_arb_tf_2)
		{
			prepareAndBind();
			draw(true);
			swapBuffers();
			draw(false);
			swapBuffers();
			draw(false);

			is_ok = is_ok && check();
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Clean GL objects. */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Feedback have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			/* Log error. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Feedback have approached error."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			/* Log fail. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Feedback have failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

void gl3cts::TransformFeedback::DrawXFBFeedback::prepareAndBind()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare programs. */
	m_program_id = gl3cts::TransformFeedback::Utilities::buildProgram(gl, m_context.getTestContext().getLog(), NULL,
																	  NULL, NULL, s_vertex_shader, s_fragment_shader,
																	  &s_xfb_varying, 1, GL_INTERLEAVED_ATTRIBS);

	if (0 == m_program_id)
	{
		throw 0;
	}

	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	/* Prepare transform feedbacks. */
	gl.genTransformFeedbacks(1, &m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks call failed.");

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedbacks call failed.");

	/* Prepare buffer objects. */
	gl.genBuffers(s_bo_count, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, s_bo_size, s_initial_data, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_id[1]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_size, NULL, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	/* Setup vertex arrays. */
	gl.genVertexArrays(s_bo_count, m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	glw::GLuint position_location = gl.getAttribLocation(m_program_id, "position");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation call failed.");

	for (glw::GLuint i = 0; i < 2; ++i)
	{
		gl.bindVertexArray(m_vao_id[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

		gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		gl.vertexAttribPointer(position_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

		gl.enableVertexAttribArray(position_location);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");
	}

	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bindVertexArray(m_vao_id[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

void gl3cts::TransformFeedback::DrawXFBFeedback::swapBuffers()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_id[m_source_bo_index]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	m_source_bo_index = (m_source_bo_index + 1) % 2;

	gl.bindVertexArray(m_vao_id[(m_source_bo_index)]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

void gl3cts::TransformFeedback::DrawXFBFeedback::draw(bool is_first_draw)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	if (is_first_draw)
	{
		gl.drawArrays(GL_POINTS, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");
	}
	else
	{
		gl.drawTransformFeedback(GL_POINTS, m_xfb_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawTransformFeedback call failed.");
	}

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");
}

bool gl3cts::TransformFeedback::DrawXFBFeedback::check()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLfloat* results =
		(glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, s_bo_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange call failed.");

	bool is_ok = false;

	if (results)
	{
		if ((results[0] == 8.f) && (results[1] == 16.f) && (results[2] == 24.f) && (results[3] == 32.f))
		{
			is_ok = true;
		}

		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");
	}

	return is_ok;
}

void gl3cts::TransformFeedback::DrawXFBFeedback::clean()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_program_id)
	{
		gl.deleteProgram(m_program_id);

		m_program_id = 0;
	}

	if (m_xfb_id)
	{
		gl.deleteTransformFeedbacks(1, &m_xfb_id);

		m_xfb_id = 0;
	}

	for (glw::GLuint i = 0; i < s_bo_count; ++i)
	{
		if (m_bo_id[i])
		{
			gl.deleteBuffers(1, &m_bo_id[i]);

			m_bo_id[i] = 0;
		}
	}

	for (glw::GLuint i = 0; i < s_bo_count; ++i)
	{
		if (m_vao_id[i])
		{
			gl.deleteVertexArrays(1, &m_vao_id[i]);

			m_vao_id[i] = 0;
		}
	}
}

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBFeedback::s_vertex_shader = "#version 130\n"
																				 "\n"
																				 "in  vec4 position;\n"
																				 "\n"
																				 "void main()\n"
																				 "{\n"
																				 "    gl_Position = position * 2.0;\n"
																				 "}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBFeedback::s_fragment_shader = "#version 130\n"
																				   "\n"
																				   "out vec4 pixel;\n"
																				   "\n"
																				   "void main()\n"
																				   "{\n"
																				   "    pixel = vec4(1.0);\n"
																				   "}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBFeedback::s_xfb_varying = "gl_Position";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBFeedback::s_attrib = "position";

const glw::GLfloat gl3cts::TransformFeedback::DrawXFBFeedback::s_initial_data[] = { 1.f, 2.f, 3.f, 4.f };

const glw::GLuint gl3cts::TransformFeedback::DrawXFBFeedback::s_draw_vertex_count =
	sizeof(s_initial_data) / sizeof(s_initial_data[0]) / 4 /* components */;

const glw::GLuint gl3cts::TransformFeedback::DrawXFBFeedback::s_bo_size = sizeof(s_initial_data);

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::CaptureSpecialInterleaved::CaptureSpecialInterleaved(deqp::Context& context,
																				const char*	test_name,
																				const char*	test_description)
	: deqp::TestCase(context, test_name, test_description)
	, m_context(context)
	, m_program_id(0)
	, m_vao_id(0)
	, m_xfb_id(0)
{
	memset(m_bo_id, 0, sizeof(m_bo_id));
}

gl3cts::TransformFeedback::CaptureSpecialInterleaved::~CaptureSpecialInterleaved(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::CaptureSpecialInterleaved::iterate(void)
{
	/* Initializations. */
	bool is_at_least_gl_40 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)));
	bool is_arb_tf_3	   = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback3");

	bool is_ok		= true;
	bool test_error = false;

	/* Tests. */
	try
	{
		if (is_at_least_gl_40 || is_arb_tf_3)
		{
			prepareAndBind();
			draw();

			is_ok = is_ok && check();
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Clean GL objects. */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Capture Special Interleaved have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			/* Log error. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Capture Special Interleaved have approached error."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			/* Log fail. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Capture Special Interleaved have failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

void gl3cts::TransformFeedback::CaptureSpecialInterleaved::prepareAndBind()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare programs. */
	m_program_id = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader, s_fragment_shader, s_xfb_varyings,
		s_xfb_varyings_count, GL_INTERLEAVED_ATTRIBS);

	if (0 == m_program_id)
	{
		throw 0;
	}

	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	/* Prepare transform feedbacks. */
	gl.genTransformFeedbacks(1, &m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks call failed.");

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedbacks call failed.");

	/* Create empty Vertex Array Object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Prepare buffer objects. */
	gl.genBuffers(s_bo_ids_count, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	for (glw::GLuint i = 0; i < s_bo_ids_count; ++i)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_size, NULL, GL_DYNAMIC_COPY); /* allocation */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, m_bo_id[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange call failed.");
	}
}

void gl3cts::TransformFeedback::CaptureSpecialInterleaved::draw()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_POINTS, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");
}

bool gl3cts::TransformFeedback::CaptureSpecialInterleaved::check()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	glw::GLfloat* results =
		(glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, s_bo_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange call failed.");

	if ((results[0] != 1.0) || (results[1] != 2.0) || (results[2] != 3.0) || (results[3] != 4.0) ||
		/* gl_SkipComponents4 here */
		(results[8] != 5.0) || (results[9] != 6.0) || (results[10] != 7.0) || (results[11] != 8.0))
	{
		is_ok = false;
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id[1]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	results = (glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, s_bo_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange call failed.");

	if ((results[0] != 9.0) || (results[1] != 10.0) || (results[2] != 11.0) || (results[3] != 12.0) ||
		/* gl_SkipComponents4 here */
		(results[8] != 13.0) || (results[9] != 14.0) || (results[10] != 15.0) || (results[11] != 16.0))
	{
		is_ok = false;
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	return is_ok;
}

void gl3cts::TransformFeedback::CaptureSpecialInterleaved::clean()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_program_id)
	{
		gl.deleteProgram(m_program_id);

		m_program_id = 0;
	}

	if (m_xfb_id)
	{
		gl.deleteTransformFeedbacks(1, &m_xfb_id);

		m_xfb_id = 0;
	}

	for (glw::GLuint i = 0; i < s_bo_ids_count; ++i)
	{
		if (m_bo_id[i])
		{
			gl.deleteBuffers(1, &m_bo_id[i]);

			m_bo_id[i] = 0;
		}
	}

	if (m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

const glw::GLchar* gl3cts::TransformFeedback::CaptureSpecialInterleaved::s_vertex_shader =
	"#version 130\n"
	"\n"
	"out vec4 variable_1;\n"
	"out vec4 variable_2;\n"
	"out vec4 variable_3;\n"
	"out vec4 variable_4;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    variable_1 = vec4(1.0, 2.0, 3.0, 4.0);\n"
	"    variable_2 = vec4(5.0, 6.0, 7.0, 8.0);\n"
	"    variable_3 = vec4(9.0, 10.0, 11.0, 12.0);\n"
	"    variable_4 = vec4(13.0, 14.0, 15.0, 16.0);\n"
	"\n"
	"    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::CaptureSpecialInterleaved::s_fragment_shader = "#version 130\n"
																							 "\n"
																							 "out vec4 pixel;\n"
																							 "\n"
																							 "void main()\n"
																							 "{\n"
																							 "    pixel = vec4(1.0);\n"
																							 "}\n";

const glw::GLchar* gl3cts::TransformFeedback::CaptureSpecialInterleaved::s_xfb_varyings[] =
	{ "variable_1", "gl_SkipComponents4", "variable_2", "gl_NextBuffer",
	  "variable_3", "gl_SkipComponents4", "variable_4" };

const glw::GLuint gl3cts::TransformFeedback::CaptureSpecialInterleaved::s_xfb_varyings_count =
	sizeof(s_xfb_varyings) / sizeof(s_xfb_varyings[0]);

const glw::GLuint gl3cts::TransformFeedback::CaptureSpecialInterleaved::s_bo_size =
	3 /*number of variables / empty places */ * 4 /* vec4 */
	* sizeof(glw::GLfloat);

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::DrawXFBStream::DrawXFBStream(deqp::Context& context, const char* test_name,
														const char* test_description)
	: deqp::TestCase(context, test_name, test_description)
	, m_context(context)
	, m_program_id_generate(0)
	, m_program_id_draw(0)
	, m_vao_id(0)
	, m_xfb_id(0)
	, m_fbo_id(0)
	, m_rbo_id(0)
{
	memset(m_bo_id, 0, sizeof(m_bo_id));
	memset(m_qo_id, 0, sizeof(m_qo_id));
}

gl3cts::TransformFeedback::DrawXFBStream::~DrawXFBStream(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::DrawXFBStream::iterate(void)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initializations. */
	bool is_at_least_gl_40  = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)));
	bool is_arb_tf_3		= m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback3");
	bool is_arb_gpu_shader5 = m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader5");

	glw::GLint max_vertex_streams = 0;

	bool is_ok		= true;
	bool test_error = false;

	/* Tests. */
	try
	{
		if (is_at_least_gl_40 || (is_arb_tf_3 && is_arb_gpu_shader5))
		{
			gl.getIntegerv(GL_MAX_VERTEX_STREAMS, &max_vertex_streams);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

			if (max_vertex_streams >= 2)
			{
				prepareObjects();

				useProgram(m_program_id_generate);

				drawForXFB();

				is_ok = is_ok && inspectQueries();

				useProgram(m_program_id_draw);

				setupVertexArray(m_bo_id[0]);

				drawForFramebuffer(0);

				setupVertexArray(m_bo_id[1]);

				drawForFramebuffer(1);

				is_ok = is_ok && check();
			}
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Clean GL objects. */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Stream have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			/* Log error. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Stream have approached error."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			/* Log fail. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Stream have failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

void gl3cts::TransformFeedback::DrawXFBStream::prepareObjects()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare programs. */
	m_program_id_generate = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), s_geometry_shader, NULL, NULL, s_vertex_shader_blank,
		s_fragment_shader, s_xfb_varyings, s_xfb_varyings_count, GL_INTERLEAVED_ATTRIBS);

	if (0 == m_program_id_generate)
	{
		throw 0;
	}

	m_program_id_draw = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader_pass, s_fragment_shader, NULL, 0,
		GL_INTERLEAVED_ATTRIBS);

	if (0 == m_program_id_draw)
	{
		throw 0;
	}

	/* Prepare transform feedbacks. */
	gl.genTransformFeedbacks(1, &m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks call failed.");

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedbacks call failed.");

	/* Create empty Vertex Array Object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Prepare buffer objects. */
	gl.genBuffers(s_bo_ids_count, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	for (glw::GLuint i = 0; i < s_bo_ids_count; ++i)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_size, NULL, GL_DYNAMIC_COPY); /* allocation */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, m_bo_id[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange call failed.");
	}

	/* Generate queries */
	gl.genQueries(s_qo_ids_count, m_qo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries call failed.");

	/* Prepare framebuffer. */
	gl.clearColor(0.f, 0.f, 0.f, 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R8, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

void gl3cts::TransformFeedback::DrawXFBStream::setupVertexArray(glw::GLuint bo_id)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(GL_ARRAY_BUFFER, bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	glw::GLuint position_location = gl.getAttribLocation(m_program_id_draw, "position");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation call failed.");

	gl.vertexAttribPointer(position_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(position_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");
}

void gl3cts::TransformFeedback::DrawXFBStream::useProgram(glw::GLuint program_id)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");
}

void gl3cts::TransformFeedback::DrawXFBStream::drawForXFB()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.beginQueryIndexed(GL_PRIMITIVES_GENERATED, 0, m_qo_id[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQueryIndexed call failed.");

	gl.beginQueryIndexed(GL_PRIMITIVES_GENERATED, 1, m_qo_id[1]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQueryIndexed call failed.");

	gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 0, m_qo_id[2]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQueryIndexed call failed.");

	gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 1, m_qo_id[3]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQueryIndexed call failed.");

	gl.drawArrays(GL_POINTS, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endQueryIndexed(GL_PRIMITIVES_GENERATED, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQueryIndexed call failed.");

	gl.endQueryIndexed(GL_PRIMITIVES_GENERATED, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQueryIndexed call failed.");

	gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQueryIndexed call failed.");

	gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQueryIndexed call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");
}

void gl3cts::TransformFeedback::DrawXFBStream::drawForFramebuffer(glw::GLuint stream)
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawTransformFeedbackStream(GL_TRIANGLES, m_xfb_id, stream);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawTransformFeedbackStream call failed.");
}

bool gl3cts::TransformFeedback::DrawXFBStream::inspectQueries()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint generated_primitives_to_stream_0 = 0;
	glw::GLint generated_primitives_to_stream_1 = 0;

	gl.getQueryObjectiv(m_qo_id[0], GL_QUERY_RESULT, &generated_primitives_to_stream_0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryIndexediv call failed.");

	gl.getQueryObjectiv(m_qo_id[1], GL_QUERY_RESULT, &generated_primitives_to_stream_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryIndexediv call failed.");

	glw::GLint primitives_written_to_xfb_to_stream_0 = 0;
	glw::GLint primitives_written_to_xfb_to_stream_1 = 0;

	gl.getQueryObjectiv(m_qo_id[2], GL_QUERY_RESULT, &primitives_written_to_xfb_to_stream_0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryIndexediv call failed.");

	gl.getQueryObjectiv(m_qo_id[3], GL_QUERY_RESULT, &primitives_written_to_xfb_to_stream_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryIndexediv call failed.");

	if ((generated_primitives_to_stream_0 == 3) && (generated_primitives_to_stream_1 == 3) &&
		(primitives_written_to_xfb_to_stream_0 == 3) && (primitives_written_to_xfb_to_stream_1 == 3))
	{
		return true;
	}

	return false;
}

bool gl3cts::TransformFeedback::DrawXFBStream::check()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Number of pixels. */
	const glw::GLuint number_of_pixels = s_view_size * s_view_size;

	/* Fetch framebuffer. */
	std::vector<glw::GLfloat> pixels(number_of_pixels);

	gl.readPixels(0, 0, s_view_size, s_view_size, GL_RED, GL_FLOAT, &pixels[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

	/* Check results. */
	for (glw::GLuint i = 0; i < number_of_pixels; ++i)
	{
		if (fabs(pixels[i] - 1.f) > 0.0625 /* precision, expected result == 1.0 */)
		{
			return false;
		}
	}

	return true;
}

void gl3cts::TransformFeedback::DrawXFBStream::clean()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_program_id_generate)
	{
		gl.deleteProgram(m_program_id_generate);

		m_program_id_generate = 0;
	}

	if (m_program_id_draw)
	{
		glw::GLuint position_location = gl.getAttribLocation(m_program_id_draw, "position");

		gl.disableVertexAttribArray(position_location);

		gl.deleteProgram(m_program_id_draw);

		m_program_id_draw = 0;
	}

	if (m_xfb_id)
	{
		gl.deleteTransformFeedbacks(1, &m_xfb_id);

		m_xfb_id = 0;
	}

	for (glw::GLuint i = 0; i < s_bo_ids_count; ++i)
	{
		if (m_bo_id[i])
		{
			gl.deleteBuffers(1, &m_bo_id[i]);

			m_bo_id[i] = 0;
		}
	}

	if (m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_fbo_id)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_rbo_id)
	{
		gl.deleteRenderbuffers(1, &m_rbo_id);

		m_rbo_id = 0;
	}
}

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStream::s_vertex_shader_blank = "#version 130\n"
																					 "\n"
																					 "void main()\n"
																					 "{\n"
																					 "}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStream::s_vertex_shader_pass = "#version 130\n"
																					"\n"
																					"in vec4 position;\n"
																					"\n"
																					"void main()\n"
																					"{\n"
																					"    gl_Position = position;\n"
																					"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStream::s_geometry_shader =
	"#version 400\n"
	"\n"
	"layout(points) in;\n"
	"layout(points, max_vertices = 6) out;\n"
	"\n"
	"layout(stream = 1) out vec4 position;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(0);\n"
	"    gl_Position = vec4( 1.0, -1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(0);\n"
	"    gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(0);\n"
	"\n"
	"    position = vec4( 1.0, -1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(1);\n"
	"    position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(1);\n"
	"    position = vec4( 1.0,  1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(1);\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStream::s_fragment_shader = "#version 130\n"
																				 "\n"
																				 "out vec4 pixel;\n"
																				 "\n"
																				 "void main()\n"
																				 "{\n"
																				 "    pixel = vec4(1.0);\n"
																				 "}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStream::s_xfb_varyings[] = { "gl_Position", "gl_NextBuffer",
																				  "position" };

const glw::GLuint gl3cts::TransformFeedback::DrawXFBStream::s_xfb_varyings_count =
	sizeof(s_xfb_varyings) / sizeof(s_xfb_varyings[0]);

const glw::GLuint gl3cts::TransformFeedback::DrawXFBStream::s_bo_size =
	3 /* triangles */ * 4 /* vec4 */ * sizeof(glw::GLfloat);

const glw::GLuint gl3cts::TransformFeedback::DrawXFBStream::s_view_size = 2;

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::DrawXFBInstanced::DrawXFBInstanced(deqp::Context& context, const char* test_name,
															  const char* test_description)
	: deqp::TestCase(context, test_name, test_description)
	, m_context(context)
	, m_program_id_generate(0)
	, m_program_id_draw(0)
	, m_vao_id(0)
	, m_xfb_id(0)
	, m_bo_id_xfb(0)
	, m_bo_id_uniform(0)
	, m_fbo_id(0)
	, m_rbo_id(0)
	, m_glGetUniformBlockIndex(DE_NULL)
	, m_glUniformBlockBinding(DE_NULL)
{
}

gl3cts::TransformFeedback::DrawXFBInstanced::~DrawXFBInstanced(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::DrawXFBInstanced::iterate(void)
{
	/* Initializations. */
	bool is_at_least_gl_42   = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 2)));
	bool is_at_least_gl_31   = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 1)));
	bool is_arb_tf_instanced = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback_instanced");
	bool is_arb_ubo			 = m_context.getContextInfo().isExtensionSupported("GL_ARB_uniform_buffer_object");

	bool is_ok		= true;
	bool test_error = false;

	if (is_arb_ubo)
	{
		m_glGetUniformBlockIndex =
			(GetUniformBlockIndex_ProcAddress)m_context.getRenderContext().getProcAddress("glGetUniformBlockIndex");

		m_glUniformBlockBinding =
			(UniformBlockBinding_ProcAddress)m_context.getRenderContext().getProcAddress("glUniformBlockBinding");

		if (DE_NULL == m_glGetUniformBlockIndex || DE_NULL == m_glUniformBlockBinding)
		{
			throw 0;
		}
	}

	try
	{
		if (is_at_least_gl_42 || ((is_at_least_gl_31 || is_arb_ubo) && is_arb_tf_instanced))
		{
			prepareObjects();
			drawForXFB();
			drawInstanced();

			is_ok = is_ok && check();
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Clean GL objects */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Instanced have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			/* Log error. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Instanced have approached error."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			/* Log fail. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Instanced have failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

void gl3cts::TransformFeedback::DrawXFBInstanced::prepareObjects()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare programs. */
	m_program_id_generate = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader_generate, s_fragment_shader,
		&s_xfb_varying, 1, GL_INTERLEAVED_ATTRIBS);

	if (0 == m_program_id_generate)
	{
		throw 0;
	}

	m_program_id_draw = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader_draw, s_fragment_shader, NULL, 0,
		GL_INTERLEAVED_ATTRIBS);

	if (0 == m_program_id_draw)
	{
		throw 0;
	}

	/* Prepare transform feedbacks. */
	gl.genTransformFeedbacks(1, &m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks call failed.");

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedbacks call failed.");

	/* Create empty Vertex Array Object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Prepare xfb buffer object. */
	gl.genBuffers(1, &m_bo_id_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_xfb_size, NULL, GL_DYNAMIC_COPY); /* allocation */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_id_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange call failed.");

	/* Prepare uniform buffer object. */
	gl.genBuffers(1, &m_bo_id_uniform);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_UNIFORM_BUFFER, m_bo_id_uniform);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_UNIFORM_BUFFER, s_bo_uniform_size, s_bo_uniform_data, GL_DYNAMIC_COPY); /* allocation */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.bindBufferBase(GL_UNIFORM_BUFFER, 0, m_bo_id_uniform);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange call failed.");

	glw::GLuint uniform_index = m_glGetUniformBlockIndex(m_program_id_draw, s_uniform);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformBlockIndex call failed.");

	if (GL_INVALID_INDEX == uniform_index)
	{
		throw 0;
	}

	m_glUniformBlockBinding(m_program_id_draw, uniform_index, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformBlockBinding call failed.");

	/* Prepare framebuffer. */
	gl.clearColor(0.f, 0.f, 0.f, 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R8, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

void gl3cts::TransformFeedback::DrawXFBInstanced::drawForXFB()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_program_id_generate);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_POINTS, 0, 4 /* quad vertex count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");
}

void gl3cts::TransformFeedback::DrawXFBInstanced::drawInstanced()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	glw::GLuint position_location = gl.getAttribLocation(m_program_id_draw, "position");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation call failed.");

	gl.vertexAttribPointer(position_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(position_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	gl.useProgram(m_program_id_draw);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.drawTransformFeedbackInstanced(GL_TRIANGLE_STRIP, m_xfb_id, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");
}

bool gl3cts::TransformFeedback::DrawXFBInstanced::check()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Number of pixels. */
	const glw::GLuint number_of_pixels = s_view_size * s_view_size;

	/* Fetch framebuffer. */
	std::vector<glw::GLfloat> pixels(number_of_pixels);

	gl.readPixels(0, 0, s_view_size, s_view_size, GL_RED, GL_FLOAT, &pixels[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

	/* Check results. */
	for (glw::GLuint i = 0; i < number_of_pixels; ++i)
	{
		if (fabs(pixels[i] - 1.f) > 0.0625 /* precision, expected result == 1.0 */)
		{
			return false;
		}
	}

	return true;
}

void gl3cts::TransformFeedback::DrawXFBInstanced::clean()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_program_id_generate)
	{
		gl.deleteProgram(m_program_id_generate);

		m_program_id_generate = 0;
	}

	if (m_program_id_draw)
	{
		glw::GLuint position_location = gl.getAttribLocation(m_program_id_draw, "position");

		gl.disableVertexAttribArray(position_location);

		gl.deleteProgram(m_program_id_draw);

		m_program_id_draw = 0;
	}

	if (m_xfb_id)
	{
		gl.deleteTransformFeedbacks(1, &m_xfb_id);

		m_xfb_id = 0;
	}

	if (m_bo_id_xfb)
	{
		gl.deleteBuffers(1, &m_bo_id_xfb);

		m_bo_id_xfb = 0;
	}

	if (m_bo_id_uniform)
	{
		gl.deleteBuffers(1, &m_bo_id_uniform);

		m_bo_id_uniform = 0;
	}

	if (m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_fbo_id)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_rbo_id)
	{
		gl.deleteRenderbuffers(1, &m_rbo_id);

		m_rbo_id = 0;
	}
}

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBInstanced::s_vertex_shader_generate =
	"#version 140\n"
	"\n"
	"void main()\n"
	"{\n"
	"    switch(gl_VertexID % 4)\n"
	"    {\n"
	"    case 0:\n"
	"       gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
	"       break;\n"
	"    case 1:\n"
	"       gl_Position = vec4( 1.0, -1.0, 0.0, 1.0);\n"
	"       break;\n"
	"    case 2:\n"
	"       gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
	"       break;\n"
	"    case 3:\n"
	"       gl_Position = vec4( 1.0,  1.0, 0.0, 1.0);\n"
	"       break;\n"
	"    }\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBInstanced::s_vertex_shader_draw =
	"#version 140\n"
	"\n"
	"uniform MatrixBlock\n"
	"{\n"
	"    mat4 transformation_0;\n"
	"    mat4 transformation_1;\n"
	"    mat4 transformation_2;\n"
	"    mat4 transformation_3;\n"
	"};\n"
	"\n"
	"in vec4 position;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    switch(gl_InstanceID % 4)\n"
	"    {\n"
	"    case 0:\n"
	"       gl_Position = position * transformation_0;\n"
	"       break;\n"
	"    case 1:\n"
	"       gl_Position = position * transformation_1;\n"
	"       break;\n"
	"    case 2:\n"
	"       gl_Position = position * transformation_2;\n"
	"       break;\n"
	"    case 3:\n"
	"       gl_Position = position * transformation_3;\n"
	"       break;\n"
	"    }\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBInstanced::s_fragment_shader = "#version 130\n"
																					"\n"
																					"out vec4 pixel;\n"
																					"\n"
																					"void main()\n"
																					"{\n"
																					"    pixel = vec4(1.0);\n"
																					"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBInstanced::s_xfb_varying = "gl_Position";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBInstanced::s_uniform = "MatrixBlock";

const glw::GLuint gl3cts::TransformFeedback::DrawXFBInstanced::s_bo_xfb_size =
	4 /* vertex count */ * 4 /* vec4 components */
	* sizeof(glw::GLfloat) /* data type size */;

const glw::GLfloat gl3cts::TransformFeedback::DrawXFBInstanced::s_bo_uniform_data[] = {
	0.5f, 0.0f, 0.0f, 0.5f,  0.0f, 0.5f, 0.0f, 0.5f,  0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

	0.5f, 0.0f, 0.0f, -0.5f, 0.0f, 0.5f, 0.0f, 0.5f,  0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

	0.5f, 0.0f, 0.0f, -0.5f, 0.0f, 0.5f, 0.0f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

	0.5f, 0.0f, 0.0f, 0.5f,  0.0f, 0.5f, 0.0f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

const glw::GLuint gl3cts::TransformFeedback::DrawXFBInstanced::s_bo_uniform_size = sizeof(s_bo_uniform_data);

const glw::GLuint gl3cts::TransformFeedback::DrawXFBInstanced::s_view_size = 4;

/*-----------------------------------------------------------------------------------------------*/

gl3cts::TransformFeedback::DrawXFBStreamInstanced::DrawXFBStreamInstanced(deqp::Context& context, const char* test_name,
																		  const char* test_description)
	: deqp::TestCase(context, test_name, test_description)
	, m_context(context)
	, m_program_id_generate(0)
	, m_program_id_draw(0)
	, m_vao_id(0)
	, m_xfb_id(0)
	, m_bo_id_xfb_position(0)
	, m_bo_id_xfb_color(0)
	, m_bo_id_uniform(0)
	, m_fbo_id(0)
	, m_rbo_id(0)
	, m_glGetUniformBlockIndex(DE_NULL)
	, m_glUniformBlockBinding(DE_NULL)
{
}

gl3cts::TransformFeedback::DrawXFBStreamInstanced::~DrawXFBStreamInstanced(void)
{
}

tcu::TestNode::IterateResult gl3cts::TransformFeedback::DrawXFBStreamInstanced::iterate(void)
{
	/* Initializations. */
	bool is_at_least_gl_31   = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 1)));
	bool is_at_least_gl_40   = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)));
	bool is_at_least_gl_42   = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 2)));
	bool is_arb_tf_instanced = m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback_instanced");
	bool is_arb_ubo			 = m_context.getContextInfo().isExtensionSupported("GL_ARB_uniform_buffer_object");
	bool is_arb_gpu_shader5  = m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader5");

	bool is_ok		= true;
	bool test_error = false;

	if (is_arb_ubo)
	{
		m_glGetUniformBlockIndex =
			(GetUniformBlockIndex_ProcAddress)m_context.getRenderContext().getProcAddress("glGetUniformBlockIndex");

		m_glUniformBlockBinding =
			(UniformBlockBinding_ProcAddress)m_context.getRenderContext().getProcAddress("glUniformBlockBinding");

		if (DE_NULL == m_glGetUniformBlockIndex || DE_NULL == m_glUniformBlockBinding)
		{
			throw 0;
		}
	}

	/* Test. */
	try
	{
		if (is_at_least_gl_42 || ((is_at_least_gl_31 || is_arb_ubo) && is_arb_gpu_shader5 && is_arb_tf_instanced) ||
			(is_at_least_gl_40 && is_arb_tf_instanced))
		{
			prepareObjects();
			drawForXFB();
			drawStreamInstanced();

			is_ok = is_ok && check();
		}
	}
	catch (...)
	{
		is_ok	  = false;
		test_error = true;
	}

	/* Clean GL objects */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		/* Log success. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Stream Instanced have passed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (test_error)
		{
			/* Log error. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Draw XFB Stream Instanced have approached error."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			/* Log fail. */
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Draw XFB Stream Instanced have failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

void gl3cts::TransformFeedback::DrawXFBStreamInstanced::prepareObjects()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare programs. */
	m_program_id_generate = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), s_geometry_shader_generate, NULL, NULL, s_vertex_shader_blank,
		s_fragment_shader_blank, s_xfb_varyings, s_xfb_varyings_count, GL_INTERLEAVED_ATTRIBS);

	if (0 == m_program_id_generate)
	{
		throw 0;
	}

	m_program_id_draw = gl3cts::TransformFeedback::Utilities::buildProgram(
		gl, m_context.getTestContext().getLog(), NULL, NULL, NULL, s_vertex_shader_draw, s_fragment_shader_draw, NULL,
		0, GL_INTERLEAVED_ATTRIBS);

	if (0 == m_program_id_draw)
	{
		throw 0;
	}

	/* Prepare transform feedbacks. */
	gl.genTransformFeedbacks(1, &m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks call failed.");

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfb_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedbacks call failed.");

	/* Create empty Vertex Array Object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Prepare xfb buffer objects. */
	gl.genBuffers(1, &m_bo_id_xfb_position);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_xfb_position);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_xfb_size, NULL, GL_DYNAMIC_COPY); /* allocation */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_id_xfb_position);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange call failed.");

	gl.genBuffers(1, &m_bo_id_xfb_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_xfb_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_xfb_size, NULL, GL_DYNAMIC_COPY); /* allocation */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_bo_id_xfb_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange call failed.");

	/* Prepare uniform buffer object. */
	gl.genBuffers(1, &m_bo_id_uniform);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_UNIFORM_BUFFER, m_bo_id_uniform);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_UNIFORM_BUFFER, s_bo_uniform_size, s_bo_uniform_data, GL_DYNAMIC_COPY); /* allocation */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.bindBufferBase(GL_UNIFORM_BUFFER, 0, m_bo_id_uniform);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange call failed.");

	glw::GLuint uniform_index = m_glGetUniformBlockIndex(m_program_id_draw, s_uniform);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformBlockIndex call failed.");

	if (GL_INVALID_INDEX == uniform_index)
	{
		throw 0;
	}

	m_glUniformBlockBinding(m_program_id_draw, uniform_index, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformBlockBinding call failed.");

	/* Prepare framebuffer. */
	gl.clearColor(0.f, 0.f, 0.f, 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R8, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

void gl3cts::TransformFeedback::DrawXFBStreamInstanced::drawForXFB()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_program_id_generate);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_POINTS, 0, 4 /* quad vertex count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");
}

void gl3cts::TransformFeedback::DrawXFBStreamInstanced::drawStreamInstanced()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_xfb_position);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	glw::GLuint position_location = gl.getAttribLocation(m_program_id_draw, "position");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation call failed.");

	gl.vertexAttribPointer(position_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(position_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id_xfb_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	glw::GLuint color_location = gl.getAttribLocation(m_program_id_draw, "color");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation call failed.");

	gl.vertexAttribPointer(color_location, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(color_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.useProgram(m_program_id_draw);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.drawTransformFeedbackStreamInstanced(GL_TRIANGLE_STRIP, m_xfb_id, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");
}

bool gl3cts::TransformFeedback::DrawXFBStreamInstanced::check()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Number of pixels. */
	const glw::GLuint number_of_pixels = s_view_size * s_view_size;

	/* Fetch framebuffer. */
	std::vector<glw::GLfloat> pixels(number_of_pixels);

	gl.readPixels(0, 0, s_view_size, s_view_size, GL_RED, GL_FLOAT, &pixels[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

	/* Check results. */
	for (glw::GLuint i = 0; i < number_of_pixels; ++i)
	{
		if (fabs(pixels[i] - 1.f) > 0.0625 /* precision, expected result == 1.0 */)
		{
			return false;
		}
	}

	return true;
}

void gl3cts::TransformFeedback::DrawXFBStreamInstanced::clean()
{
	/* Functions handler */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_program_id_generate)
	{
		gl.deleteProgram(m_program_id_generate);

		m_program_id_generate = 0;
	}

	if (m_program_id_draw)
	{
		glw::GLuint position_location = gl.getAttribLocation(m_program_id_draw, "position");

		gl.disableVertexAttribArray(position_location);

		glw::GLuint color_location = gl.getAttribLocation(m_program_id_draw, "color");

		gl.disableVertexAttribArray(color_location);

		gl.deleteProgram(m_program_id_draw);

		m_program_id_draw = 0;
	}

	if (m_xfb_id)
	{
		gl.deleteTransformFeedbacks(1, &m_xfb_id);

		m_xfb_id = 0;
	}

	if (m_bo_id_xfb_position)
	{
		gl.deleteBuffers(1, &m_bo_id_xfb_position);

		m_bo_id_xfb_position = 0;
	}

	if (m_bo_id_xfb_color)
	{
		gl.deleteBuffers(1, &m_bo_id_xfb_color);

		m_bo_id_xfb_position = 0;
	}

	if (m_bo_id_uniform)
	{
		gl.deleteBuffers(1, &m_bo_id_uniform);

		m_bo_id_uniform = 0;
	}

	if (m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_fbo_id)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_rbo_id)
	{
		gl.deleteRenderbuffers(1, &m_rbo_id);

		m_rbo_id = 0;
	}
}

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_vertex_shader_blank = "#version 140\n"
																							  "\n"
																							  "void main()\n"
																							  "{\n"
																							  "}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_geometry_shader_generate =
	"#version 400\n"
	"\n"
	"layout(points) in;\n"
	"layout(points, max_vertices = 8) out;\n"
	"\n"
	"layout(stream = 1) out vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(0);\n"
	"    gl_Position = vec4( 1.0, -1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(0);\n"
	"    gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(0);\n"
	"    gl_Position = vec4( 1.0,  1.0, 0.0, 1.0);\n"
	"    EmitStreamVertex(0);\n"
	"\n"
	"    color = vec4(1.0, 1.0, 1.0, 1.0);\n"
	"    EmitStreamVertex(1);\n"
	"    color = vec4(1.0, 1.0, 1.0, 1.0);\n"
	"    EmitStreamVertex(1);\n"
	"    color = vec4(1.0, 1.0, 1.0, 1.0);\n"
	"    EmitStreamVertex(1);\n"
	"    color = vec4(1.0, 1.0, 1.0, 1.0);\n"
	"    EmitStreamVertex(1);\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_vertex_shader_draw =
	"#version 140\n"
	"\n"
	"uniform MatrixBlock\n"
	"{\n"
	"    mat4 transformation_0;\n"
	"    mat4 transformation_1;\n"
	"    mat4 transformation_2;\n"
	"    mat4 transformation_3;\n"
	"};\n"
	"\n"
	"in  vec4 position;\n"
	"in  vec4 color;\n"
	"out vec4 colour;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    switch(gl_InstanceID % 4)\n"
	"    {\n"
	"    case 0:\n"
	"       gl_Position = position * transformation_0;\n"
	"       break;\n"
	"    case 1:\n"
	"       gl_Position = position * transformation_1;\n"
	"       break;\n"
	"    case 2:\n"
	"       gl_Position = position * transformation_2;\n"
	"       break;\n"
	"    case 3:\n"
	"       gl_Position = position * transformation_3;\n"
	"       break;\n"
	"    }\n"
	"    colour = color;\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_fragment_shader_blank =
	"#version 130\n"
	"\n"
	"out vec4 pixel;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    pixel = vec4(1.0);\n"
	"}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_fragment_shader_draw = "#version 130\n"
																							   "\n"
																							   "in vec4 colour;\n"
																							   "out vec4 pixel;\n"
																							   "\n"
																							   "void main()\n"
																							   "{\n"
																							   "    pixel = colour;\n"
																							   "}\n";

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_xfb_varyings[] = { "gl_Position",
																						   "gl_NextBuffer", "color" };

const glw::GLuint gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_xfb_varyings_count =
	sizeof(s_xfb_varyings) / sizeof(s_xfb_varyings[0]);

const glw::GLchar* gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_uniform = "MatrixBlock";

const glw::GLuint gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_bo_xfb_size =
	4 /* vertex count */ * 4 /* vec4 components */
	* sizeof(glw::GLfloat) /* data type size */;

const glw::GLfloat gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_bo_uniform_data[] = {
	0.5f, 0.0f, 0.0f, 0.5f,  0.0f, 0.5f, 0.0f, 0.5f,  0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

	0.5f, 0.0f, 0.0f, -0.5f, 0.0f, 0.5f, 0.0f, 0.5f,  0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

	0.5f, 0.0f, 0.0f, -0.5f, 0.0f, 0.5f, 0.0f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

	0.5f, 0.0f, 0.0f, 0.5f,  0.0f, 0.5f, 0.0f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

const glw::GLuint gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_bo_uniform_size = sizeof(s_bo_uniform_data);

const glw::GLuint gl3cts::TransformFeedback::DrawXFBStreamInstanced::s_view_size = 4;

/*-----------------------------------------------------------------------------------------------*/

glw::GLuint gl3cts::TransformFeedback::Utilities::buildProgram(
	glw::Functions const& gl, tcu::TestLog& log, glw::GLchar const* const geometry_shader_source,
	glw::GLchar const* const tessellation_control_shader_source,
	glw::GLchar const* const tessellation_evaluation_shader_source, glw::GLchar const* const vertex_shader_source,
	glw::GLchar const* const fragment_shader_source, glw::GLchar const* const* const transform_feedback_varyings,
	glw::GLsizei const transform_feedback_varyings_count, glw::GLenum const transform_feedback_varyings_mode,
	bool const do_not_detach, glw::GLint* linking_status)
{
	glw::GLuint program = 0;

	struct Shader
	{
		glw::GLchar const* const source;
		glw::GLenum const		 type;
		glw::GLuint				 id;
	} shader[] = { { geometry_shader_source, GL_GEOMETRY_SHADER, 0 },
				   { tessellation_control_shader_source, GL_TESS_CONTROL_SHADER, 0 },
				   { tessellation_evaluation_shader_source, GL_TESS_EVALUATION_SHADER, 0 },
				   { vertex_shader_source, GL_VERTEX_SHADER, 0 },
				   { fragment_shader_source, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		program = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(program, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, 1, &(shader[i].source), NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					log << tcu::TestLog::Message << "Shader compilation has failed.\n"
						<< "Shader type: " << glu::getShaderTypeStr(shader[i].type) << "\n"
						<< "Shader compilation error log:\n"
						<< log_text << "\n"
						<< "Shader source code:\n"
						<< shader[i].source << "\n"
						<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

					throw 0;
				}
			}
		}

		/* Link. */
		if (transform_feedback_varyings_count)
		{
			gl.transformFeedbackVaryings(program, transform_feedback_varyings_count, transform_feedback_varyings,
										 transform_feedback_varyings_mode);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");
		}

		gl.linkProgram(program);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(program, GL_LINK_STATUS, &status);

		if (DE_NULL != linking_status)
		{
			*linking_status = status;
		}

		if (GL_TRUE == status)
		{
			if (!do_not_detach)
			{
				for (glw::GLuint i = 0; i < shader_count; ++i)
				{
					if (shader[i].id)
					{
						gl.detachShader(program, shader[i].id);

						GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
					}
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(program, log_size, NULL, &log_text[0]);

			log << tcu::TestLog::Message << "Program linkage has failed due to:\n"
				<< log_text << "\n"
				<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			throw 0;
		}
	}
	catch (...)
	{
		if (program)
		{
			gl.deleteProgram(program);

			program = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	return program;
}

/** @brief Substitute key with value within source code.
 *
 *  @param [in] source      Source code to be prerocessed.
 *  @param [in] key         Key to be substituted.
 *  @param [in] value       Value to be inserted.
 *
 *  @return Resulting string.
 */
std::string gl3cts::TransformFeedback::Utilities::preprocessCode(std::string source, std::string key, std::string value)
{
	std::string destination = source;

	while (true)
	{
		/* Find token in source code. */
		size_t position = destination.find(key, 0);

		/* No more occurences of this key. */
		if (position == std::string::npos)
		{
			break;
		}

		/* Replace token with sub_code. */
		destination.replace(position, key.size(), value);
	}

	return destination;
}

/** @brief Convert an integer to a string.
 *
 *  @param [in] i       Integer to be converted.
 *
 *  @return String representing integer.
 */
std::string gl3cts::TransformFeedback::Utilities::itoa(glw::GLint i)
{
	std::stringstream stream;

	stream << i;

	return stream.str();
}

/** @brief Convert an float to a string.
 *
 *  @param [in] f       Float to be converted.
 *
 *  @return String representing integer.
 */
std::string gl3cts::TransformFeedback::Utilities::ftoa(glw::GLfloat f)
{
	std::stringstream stream;

	stream << f;

	return stream.str();
}
