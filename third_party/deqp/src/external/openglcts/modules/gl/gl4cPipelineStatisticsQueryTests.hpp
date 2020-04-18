#ifndef _GL4CPIPELINESTATISTICSQUERYTESTS_HPP
#define _GL4CPIPELINESTATISTICSQUERYTESTS_HPP
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
 * \file  gl4c PipelineStatisticsQueryTests.hpp
 * \brief Declares test classes that verify conformance of the
 *        GL implementation with GL_ARB_pipeline_statistics_query
 *        extension specification.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"
#include <limits.h>
#include <sstream>
#include <string.h>

namespace glcts
{
class PipelineStatisticsQueryUtilities
{
public:
	/* Public type definitions */
	typedef bool (*PFNQUERYDRAWHANDLERPROC)(void* user_arg);

	/* Type of the draw call used for a test iteration */
	enum _draw_call_type
	{
		/* glDrawArrays() */
		DRAW_CALL_TYPE_GLDRAWARRAYS,
		/* glDrawArraysIndirect() */
		DRAW_CALL_TYPE_GLDRAWARRAYSINDIRECT,
		/* glDrawArraysInstanced() */
		DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCED,
		/* glDrawArraysInstancedBaseInstance() */
		DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCEDBASEINSTANCE,
		/* glDrawElements() */
		DRAW_CALL_TYPE_GLDRAWELEMENTS,
		/* glDrawElementsBaseVertex() */
		DRAW_CALL_TYPE_GLDRAWELEMENTSBASEVERTEX,
		/* glDrawElementsIndirect() */
		DRAW_CALL_TYPE_GLDRAWELEMENTSINDIRECT,
		/* glDrawElementsInstanced() */
		DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCED,
		/* glDrawElementsInstancedBaseInstance() */
		DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEINSTANCE,
		/* glDrawElementsInstancedBaseVertexBaseInstance() */
		DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE,
		/* glDrawRangeElements() */
		DRAW_CALL_TYPE_GLDRAWRANGEELEMENTS,
		/* glDrawRangeElementsBaseVertex() */
		DRAW_CALL_TYPE_GLDRAWRANGEELEMENTSBASEVERTEX,

		/* Always last */
		DRAW_CALL_TYPE_COUNT
	};

	/* Input primitive type defined in geometry shader body used by a test iteration */
	enum _geometry_shader_input
	{
		GEOMETRY_SHADER_INPUT_FIRST,

		GEOMETRY_SHADER_INPUT_POINTS = GEOMETRY_SHADER_INPUT_FIRST,
		GEOMETRY_SHADER_INPUT_LINES,
		GEOMETRY_SHADER_INPUT_LINES_ADJACENCY,
		GEOMETRY_SHADER_INPUT_TRIANGLES,
		GEOMETRY_SHADER_INPUT_TRIANGLES_ADJACENCY,

		/* Always last */
		GEOMETRY_SHADER_INPUT_COUNT
	};

	/* Output primitive type defined in geometry shader body used by a test iteration */
	enum _geometry_shader_output
	{
		GEOMETRY_SHADER_OUTPUT_FIRST,

		GEOMETRY_SHADER_OUTPUT_POINTS = GEOMETRY_SHADER_OUTPUT_FIRST,
		GEOMETRY_SHADER_OUTPUT_LINE_STRIP,
		GEOMETRY_SHADER_OUTPUT_TRIANGLE_STRIP,

		/* Always last */
		GEOMETRY_SHADER_OUTPUT_COUNT
	};

	/* Primitive type used for a draw call */
	enum _primitive_type
	{
		PRIMITIVE_TYPE_FIRST,

		/* GL_POINTS */
		PRIMITIVE_TYPE_POINTS = PRIMITIVE_TYPE_FIRST,
		/* GL_LINE_LOOP */
		PRIMITIVE_TYPE_LINE_LOOP,
		/* GL_LINE_STRIP */
		PRIMITIVE_TYPE_LINE_STRIP,
		/* GL_LINES */
		PRIMITIVE_TYPE_LINES,
		/* GL_LINES_ADJACENCY */
		PRIMITIVE_TYPE_LINES_ADJACENCY,
		/* GL_PATCHES */
		PRIMITIVE_TYPE_PATCHES,
		/* GL_TRIANGLE_FAN */
		PRIMITIVE_TYPE_TRIANGLE_FAN,
		/* GL_TRIANGLE_STRIP */
		PRIMITIVE_TYPE_TRIANGLE_STRIP,
		/* GL_TRIANGLES */
		PRIMITIVE_TYPE_TRIANGLES,
		/* GL_TRIANGLES_ADJACENCY */
		PRIMITIVE_TYPE_TRIANGLES_ADJACENCY,

		/* Always last */
		PRIMITIVE_TYPE_COUNT
	};

	/* Stores result of a single test iteration. */
	struct _test_execution_result
	{
		/* true if 64-bit signed integer counter value was retrieved for the iteration,
		 * false otherwise.
		 */
		bool int64_written;
		/* true if 64-bit unsigned integer counter value was retrieved for the iteration,
		 * false otherwise.
		 */
		bool uint64_written;

		/* 32-bit signed integer counter value, as stored in the query buffer object
		 * used for the test iteration. This variable will only be modified if query
		 * buffer objects are supported.
		 */
		glw::GLint result_qo_int;
		/* 64-bit signed integer counter value, as stored in the query buffer object
		 * used for the test iteration. This variable will only be modified if query
		 * buffer objects are supported, and int64_written is true.
		 */
		glw::GLint64 result_qo_int64;
		/* 32-bit unsigned integer counter value, as stored in the query buffer object
		 * used for the test iteration. This variable will only be modified if query
		 * buffer objects are supported.
		 */
		glw::GLuint result_qo_uint;
		/* 64-bit unsigned integer counter value, as stored in the query buffer object
		 * used for the test iteration. This variable will only be modified if query
		 * buffer objects are supported, and uint64_written is true.
		 */
		glw::GLuint64 result_qo_uint64;

		/* 32-bit signed integer counter value, as stored in the query object
		 * used for the test iteration.
		 */
		glw::GLint result_int;
		/* 64-bit signed integer counter value, as stored in the query object
		 * used for the test iteration. Only set if int64_written is true.
		 */
		glw::GLint64 result_int64;
		/* 32-bit unsigned integer counter value, as stored in the query object
		 * used for the test iteration.
		 */
		glw::GLuint result_uint;
		/* 64-bit unsigned integer counter value, as stored in the query object
		 * used for the test iteration. Only set if uint64_written is true.
		 */
		glw::GLuint64 result_uint64;

		/* Constructor */
		_test_execution_result()
		{
			result_qo_int	= INT_MAX;
			result_qo_int64  = LLONG_MAX;
			result_qo_uint   = UINT_MAX;
			result_qo_uint64 = ULLONG_MAX;

			result_int	= INT_MAX;
			result_int64  = LLONG_MAX;
			result_uint   = UINT_MAX;
			result_uint64 = ULLONG_MAX;

			int64_written  = false;
			uint64_written = false;
		}
	};

	/* Tells how the result values should be verified against
	 * the reference value.
	 */
	enum _verification_type
	{
		/* The result value should be equal to the reference value */
		VERIFICATION_TYPE_EXACT_MATCH,
		/* The result value should be equal or larger than the reference value */
		VERIFICATION_TYPE_EQUAL_OR_GREATER,

		VERIFICATION_TYPE_UNDEFINED
	};

	/* Code of a compute shader used by a functional test that verifies
	 * GL_COMPUTE_SHADER_INVOCATIONS_ARB query works correctly.
	 */
	static const char* dummy_cs_code;
	/* Code of a fragment shader used by a number of functional tests */
	static const char* dummy_fs_code;
	/* Code of a tessellation control shader used by a functional test that verifies
	 * GL_TESS_CONTROL_SHADER_PATCHES_ARB and GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB
	 * queries work correctly.
	 */
	static const char* dummy_tc_code;
	/* Code of a tessellation evaluation shader used by a functional test that verifies
	 * GL_TESS_CONTROL_SHADER_PATCHES_ARB and GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB
	 * queries work correctly.
	 */
	static const char* dummy_te_code;
	/* Code of a vertex shader used by a number of functional tests */
	static const char* dummy_vs_code;

	/* Tells how many query targets are stored in query_targets */
	static const unsigned int n_query_targets;
	/* Stores all query targets that should be used by the tests */
	static const glw::GLenum query_targets[];

	/* Tells the offset, relative to the beginning of the buffer object storage,
	 * from which the query's int32 result value starts. */
	static const unsigned int qo_bo_int_offset;
	/* Tells the offset, relative to the beginning of the buffer object storage,
	 * from which the query's int64 result value starts. */
	static const unsigned int qo_bo_int64_offset;
	/* Tells the offset, relative to the beginning of the buffer object storage,
	 * from which the query's uint32 result value starts. */
	static const unsigned int qo_bo_uint_offset;
	/* Tells the offset, relative to the beginning of the buffer object storage,
	 * from which the query's uint64 result value starts. */
	static const unsigned int qo_bo_uint64_offset;
	static const unsigned int qo_bo_size;

	/* Public methods */
	static std::string buildGeometryShaderBody(_geometry_shader_input gs_input, _geometry_shader_output gs_output,
											   unsigned int n_primitives_to_emit_in_stream0, unsigned int n_streams);

	static bool executeQuery(glw::GLenum query_type, glw::GLuint qo_id, glw::GLuint qo_bo_id,
							 PFNQUERYDRAWHANDLERPROC pfn_draw, void* draw_user_arg,
							 const glu::RenderContext& render_context, tcu::TestContext& test_context,
							 const glu::ContextInfo& context_info, _test_execution_result* out_result);

	static glw::GLenum getEnumForPrimitiveType(_primitive_type primitive_type);
	static std::string getGLSLStringForGSInput(_geometry_shader_input gs_input);
	static std::string getGLSLStringForGSOutput(_geometry_shader_output gs_output);
	static unsigned int getNumberOfVerticesForGSInput(_geometry_shader_input gs_input);
	static unsigned int getNumberOfVerticesForGSOutput(_geometry_shader_output gs_output);
	static unsigned int getNumberOfVerticesForPrimitiveType(_primitive_type primitive_type);
	static _primitive_type getPrimitiveTypeFromGSInput(_geometry_shader_input gs_input);
	static std::string getStringForDrawCallType(_draw_call_type draw_call_type);
	static std::string getStringForEnum(glw::GLenum value);
	static std::string getStringForPrimitiveType(_primitive_type primitive_type);

	static bool isDrawCallSupported(_draw_call_type draw_call, const glw::Functions& gl);

	static bool isInstancedDrawCall(_draw_call_type draw_call);

	static bool isQuerySupported(glw::GLenum value, const glu::ContextInfo& context_info,
								 const glu::RenderContext& render_context);

	static bool verifyResultValues(const _test_execution_result& run_result, unsigned int n_expected_values,
								   const glw::GLuint64* expected_values, bool should_check_qo_bo_values,
								   const glw::GLenum query_type, const _draw_call_type* draw_call_type_ptr,
								   const _primitive_type* primitive_type_ptr, bool is_primitive_restart_enabled,
								   tcu::TestContext& test_context, _verification_type verification_type);

	/** Constructs a string that describes details of a test iteration that has been
	 *  detected to have failed.
	 *
	 *  @param value                        Query counter value as retrieved by the test iteration.
	 *  @param value_type                   Null-terminated string holding the name of the type
	 *                                      of the result value.
	 *  @param n_expected_values            Number of possible expected values.
	 *  @param expected_values              Array of possible expected values.
	 *  @param query_type                   Type of the query used by the test iteration.
	 *  @param draw_call_type_name          Type of the draw call used by the test iteration.
	 *  @param primitive_type_name          Primitive type used for the test iteration's draw call.
	 *  @param is_primitive_restart_enabled true if the test iteration was run with "primitive restart"
	 *                                      functionality enabled, false otherwise.
	 *
	 *  @return                             String that includes a human-readable description of the
	 *                                      test iteration's properties.
	 *
	 */
	template <typename VALUE_TYPE>
	static std::string getVerifyResultValuesErrorString(VALUE_TYPE value, const char* value_type,
														unsigned int		 n_expected_values,
														const glw::GLuint64* expected_values, glw::GLenum query_type,
														std::string draw_call_type_name,
														std::string primitive_type_name,
														bool		is_primitive_restart_enabled)
	{
		std::stringstream log_sstream;

		DE_ASSERT(expected_values != DE_NULL);

		log_sstream << "Invalid default " << value_type << " query result value: found "
														   "["
					<< value << "], expected:"
								"["
					<< expected_values[0] << "]";

		for (unsigned int i = 1; i < n_expected_values; ++i)
		{
			log_sstream << " or [" << expected_values[i] << "]";
		}

		log_sstream << ", query type:"
					   "["
					<< getStringForEnum(query_type) << "], "
													   "GL_PRIMITIVE_RESTART mode:"
													   "["
					<< ((is_primitive_restart_enabled) ? "enabled" : "disabled") << "]"
																					", draw call type:"
																					"["
					<< draw_call_type_name.c_str() << "]"
													  ", primitive type:"
													  "["
					<< primitive_type_name.c_str() << "].";

		return log_sstream.str();
	}
};

/** Check that calling BeginQuery with a pipeline statistics query target
 *  generates an INVALID_OPERATION error if the specified query was
 *  previously used with a different pipeline statistics query target.
 **/
class PipelineStatisticsQueryTestAPICoverage1 : public deqp::TestCase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestAPICoverage1(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private fields */
	glw::GLuint m_qo_id;
};

/** Performs the following tests:
 *
 *  * If GL 3.2 and ARB_geometry_shader4 are not supported then check that
 *    calling BeginQuery with target GEOMETRY_SHADER_INVOCATIONS or
 *    GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB generates an INVALID_ENUM error.
 *
 *  * If GL 4.0 and ARB_tessellation_shader are not supported then check that
 *    calling BeginQuery with target TESS_CONTROL_SHADER_INVOCATIONS_ARB or
 *    TESS_EVALUATION_SHADER_INVOCATIONS_ARB generates an INVALID_ENUM error.
 *
 *  * If GL 4.3 and ARB_compute_shader are not supported then check that
 *    calling BeginQuery with target COMPUTE_SHADER_INVOCATIONS_ARB generates
 *    an INVALID_ENUM error.
 *
 **/
class PipelineStatisticsQueryTestAPICoverage2 : public deqp::TestCase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestAPICoverage2(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private fields */
	glw::GLuint m_qo_id;
};

/** Base class used by all functional test implementations. Provides various
 *  methods that are shared between functional tests.
 *
 *  Derivative classes must implement executeTest() method.
 */
class PipelineStatisticsQueryTestFunctionalBase : public deqp::TestCase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctionalBase(deqp::Context& context, const char* name, const char* description);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected methods */
	void buildProgram(const char* cs_body, const char* fs_body, const char* gs_body, const char* tc_body,
					  const char* te_body, const char* vs_body);

	virtual void deinitObjects();

	virtual bool executeTest(glw::GLenum current_query_target) = 0;
	void		 initFBO();
	virtual void initObjects();
	void		 initQOBO();
	void initVAO(unsigned int n_components_per_vertex);

	void initVBO(const float* raw_vertex_data, unsigned int raw_vertex_data_size, const unsigned int* raw_index_data,
				 unsigned int raw_index_data_size, unsigned int indirect_draw_bo_count_argument,
				 unsigned int indirect_draw_bo_primcount_argument, unsigned int indirect_draw_bo_baseinstance_argument,
				 unsigned int indirect_draw_bo_first_argument,		 /* glDrawArrays() only */
				 unsigned int indirect_draw_bo_basevertex_argument); /* glDrawElements() only */

	virtual bool shouldExecuteForQueryTarget(glw::GLenum query_target);

	/* Protected static methods */
	static bool queryCallbackDrawCallHandler(void* pThis);

	/* Protected fields */
	glw::GLuint m_bo_qo_id;
	glw::GLuint m_fbo_id;
	glw::GLuint m_po_id;
	glw::GLuint m_qo_id;
	glw::GLuint m_to_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vbo_id;

	const unsigned int m_to_height;
	const unsigned int m_to_width;

	unsigned int m_vbo_indirect_arrays_argument_offset;
	unsigned int m_vbo_indirect_elements_argument_offset;
	unsigned int m_vbo_index_data_offset;
	unsigned int m_vbo_n_indices;
	unsigned int m_vbo_vertex_data_offset;

	PipelineStatisticsQueryUtilities::_draw_call_type m_current_draw_call_type;
	PipelineStatisticsQueryUtilities::_primitive_type m_current_primitive_type;
	unsigned int									  m_indirect_draw_call_baseinstance_argument;
	unsigned int									  m_indirect_draw_call_basevertex_argument;
	unsigned int									  m_indirect_draw_call_count_argument;
	unsigned int									  m_indirect_draw_call_first_argument;
	unsigned int									  m_indirect_draw_call_firstindex_argument;
	unsigned int									  m_indirect_draw_call_primcount_argument;
};

/** Performs the following functional test:
 *
 * Check that all pipeline statistics query types return a result of zero
 * if no rendering commands are issued between BeginQuery and EndQuery.
 *
 **/
class PipelineStatisticsQueryTestFunctional1 : public PipelineStatisticsQueryTestFunctionalBase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctional1(deqp::Context& context);

protected:
	/* Protected methods */
	bool executeTest(glw::GLenum current_query_target);
};

/** Performs the following functional test:
 *
 * Check that all pipeline statistics query types return a result of zero
 * if Clear, ClearBuffer*, BlitFramebuffer, or any of the non-rendering
 * commands involving blits or copies (e.g. TexSubImage, CopyImageSubData,
 * ClearTexSubImage, BufferSubData, ClearBufferSubData) are issued between
 * BeginQuery and EndQuery.
 *
 **/
class PipelineStatisticsQueryTestFunctional2 : public PipelineStatisticsQueryTestFunctionalBase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctional2(deqp::Context& context);

protected:
	/* Protected methods */
	void deinitObjects();
	bool executeTest(glw::GLenum current_query_target);
	void initObjects();

private:
	static bool executeBlitFramebufferTest(void* pThis);
	static bool executeBufferSubDataTest(void* pThis);
	static bool executeClearBufferfvColorBufferTest(void* pThis);
	static bool executeClearBufferfvDepthBufferTest(void* pThis);
	static bool executeClearBufferivStencilBufferTest(void* pThis);
	static bool executeClearBufferSubDataTest(void* pThis);
	static bool executeClearColorBufferTest(void* pThis);
	static bool executeClearDepthBufferTest(void* pThis);
	static bool executeClearStencilBufferTest(void* pThis);
	static bool executeClearTexSubImageTest(void* pThis);
	static bool executeCopyImageSubDataTest(void* pThis);
	static bool executeTexSubImageTest(void* pThis);

	/* Private fields */
	glw::GLuint m_bo_id;
	glw::GLuint m_fbo_draw_id;
	glw::GLuint m_fbo_read_id;
	glw::GLuint m_to_draw_fbo_id;
	glw::GLuint m_to_read_fbo_id;

	const glw::GLuint m_to_height;
	const glw::GLuint m_to_width;

	static const glw::GLuint bo_size;
};

/** Performs the following functional tests:
 *
 * Using the basic outline check that VERTICES_SUBMITTED_ARB queries return
 * the number of vertices submitted using draw commands. Vertices
 * corresponding to partial/incomplete primitives may or may not be counted
 * (e.g. when submitting a single instance with 8 vertices with primitive
 * mode TRIANGLES, the result of the query could be either 6 or 8).
 *
 * Using the basic outline check that VERTICES_SUBMITTED_ARB queries don't
 * count primitive topology restarts when PRIMITIVE_RESTART is enabled and
 * DrawElements* is used to submit an element array which contains one or
 * more primitive restart index values.
 *
 * Using the basic outline check that PRIMITIVES_SUBMITTED_ARB queries
 * return the exact number of primitives submitted using draw commands.
 * Partial/incomplete primitives may or may not be counted (e.g. when
 * submitting a single instance with 8 vertices with primitive mode
 * TRIANGLES, the result of the query could be either 2 or 3). Test the
 * behavior with all supported primitive modes.
 *
 * Using the basic outline check that PRIMITIVES_SUBMITTED_ARB queries
 * don't count primitive topology restarts when PRIMITIVE_RESTART is
 * enabled and DrawElements* is used to submit an element array which
 * contains one or more primitive restart index values. Also, partial/
 * incomplete primitives resulting from the use of primitive restart index
 * values may or may not be counted.
 *
 * Using the basic outline check that CLIPPING_INPUT_PRIMITIVES_ARB queries
 * return the exact number of primitives reaching the primitive clipping
 * stage. If RASTERIZER_DISCARD is disabled, and the tessellation and
 * geometry shader stage is not used, this should match the number of
 * primitives submitted.
 *
 * Using the basic outline check that CLIPPING_OUTPUT_PRIMITIVES_ARB
 * queries return a value that is always greater than or equal to the
 * number of primitives submitted if RASTERIZER_DISCARD is disabled, there
 * is no tessellation and geometry shader stage used, and all primitives
 * lie entirely in the cull volume.
 *
 **/
class PipelineStatisticsQueryTestFunctional3 : public PipelineStatisticsQueryTestFunctionalBase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctional3(deqp::Context& context);

protected:
	/* Protected methods */
	void deinitObjects();
	bool executeTest(glw::GLenum current_query_target);
	void initObjects();
	bool shouldExecuteForQueryTarget(glw::GLenum query_target);

private:
	/* Private methods */
	void getExpectedPrimitivesSubmittedQueryResult(
		PipelineStatisticsQueryUtilities::_primitive_type current_primitive_type, unsigned int* out_results_written,
		glw::GLuint64 out_results[4]);

	void getExpectedVerticesSubmittedQueryResult(
		PipelineStatisticsQueryUtilities::_primitive_type current_primitive_type, unsigned int* out_results_written,
		glw::GLuint64 out_results[4]);

	/* Private fields */
	bool m_is_primitive_restart_enabled;
};

/** Performs the following functional test:
 *
 * Using the basic outline with a program having a vertex shader check that
 * VERTEX_SHADER_INVOCATIONS_ARB queries return a result greater than zero
 * if at least one vertex is submitted to the GL.
 *
 **/
class PipelineStatisticsQueryTestFunctional4 : public PipelineStatisticsQueryTestFunctionalBase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctional4(deqp::Context& context);

protected:
	/* Protected methods */
	void deinitObjects();
	bool executeTest(glw::GLenum current_query_target);
	void initObjects();
	bool shouldExecuteForQueryTarget(glw::GLenum query_target);
};

/** Performs the following functional test:
 *
 * If GL 4.0 is supported, using the basic outline with a program having a
 * tessellation control and tessellation evaluation shader check that
 * TESS_CONTROL_SHADER_INVOCATIONS_ARB and TESS_EVALUATION_SHADER_-
 * INVOCATIONS_ARB queries return a result greater than zero if at least
 * one patch is needed to be processed by the GL (e.g. PATCH_VERTICES is 3,
 * and at least 3 vertices are submitted to the GL).
 *
 **/
class PipelineStatisticsQueryTestFunctional5 : public PipelineStatisticsQueryTestFunctionalBase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctional5(deqp::Context& context);

protected:
	/* Protected methods */
	void deinitObjects();
	bool executeTest(glw::GLenum current_query_target);
	void initObjects();
	bool shouldExecuteForQueryTarget(glw::GLenum query_target);
};

/** Performs the following functional test:
 *
 * If GL 3.2 is supported, using the basic outline with a program having a
 * geometry shader check that GEOMETRY_SHADER_INVOCATIONS queries return
 * a result greater than zero if at least one complete primitive is
 * submitted to the GL.
 *
 * If GL 3.2 is supported, using the basic outline with a program having a
 * geometry shader check that GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB
 * queries return the exact number of primitives emitted by the geometry
 * shader. Also, if GL 4.0 is supported, use a geometry shader that emits
 * primitives to multiple vertex streams. In this case the result of
 * GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB has to match the sum of the
 * number of primitives emitted to each particular vertex stream.
 *
 **/
class PipelineStatisticsQueryTestFunctional6 : public PipelineStatisticsQueryTestFunctionalBase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctional6(deqp::Context& context);

protected:
	/* Protected methods */
	void deinitObjects();
	bool executeTest(glw::GLenum current_query_target);
	void initObjects();
	bool shouldExecuteForQueryTarget(glw::GLenum query_target);

private:
	/* Private fields */
	const unsigned int m_n_primitives_emitted_by_gs;
	const unsigned int m_n_streams_emitted_by_gs;
};

/** Performs the following functional test:
 *
 * Using the basic outline with a program having a fragment shader check
 * that FRAGMENT_SHADER_INVOCATIONS_ARB queries return a result greater
 * than zero if at least one fragment gets rasterized.
 *
 **/
class PipelineStatisticsQueryTestFunctional7 : public PipelineStatisticsQueryTestFunctionalBase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctional7(deqp::Context& context);

protected:
	/* Protected methods */
	void deinitObjects();
	bool executeTest(glw::GLenum current_query_target);
	void initObjects();
	bool shouldExecuteForQueryTarget(glw::GLenum query_target);
};

/** Performs the following functional test:
 *
 * Using the basic outline with a program having a compute shader check
 * that COMPUTE_SHADER_INVOCATIONS_ARB queries return a result greater
 * than zero if at least a single work group is submitted using one of the
 * Dispatch* commands.
 *
 **/
class PipelineStatisticsQueryTestFunctional8 : public PipelineStatisticsQueryTestFunctionalBase
{
public:
	/* Public methods */
	PipelineStatisticsQueryTestFunctional8(deqp::Context& context);

protected:
	/* Protected methods */
	void deinitObjects();
	bool executeTest(glw::GLenum current_query_target);
	void initObjects();
	bool shouldExecuteForQueryTarget(glw::GLenum query_target);

private:
	/* Private methods */
	static bool queryCallbackDispatchCallHandler(void* pInstance);

	/* Private fields */
	glw::GLuint  m_bo_dispatch_compute_indirect_args_offset;
	glw::GLuint  m_bo_id;
	unsigned int m_current_iteration;
};

/** Test group which encapsulates all conformance tests for
 *  GL_ARB_pipeline_statistics_query extension.
 */
class PipelineStatisticsQueryTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	PipelineStatisticsQueryTests(deqp::Context& context);

	void init(void);

private:
	PipelineStatisticsQueryTests(const PipelineStatisticsQueryTests& other);
	PipelineStatisticsQueryTests& operator=(const PipelineStatisticsQueryTests& other);
};

} /* glcts namespace */

#endif // _GL4CPIPELINESTATISTICSQUERYTESTS_HPP
