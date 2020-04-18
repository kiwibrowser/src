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
 * \file  gl4cPipelineStatisticsQueryTests.cpp
 * \brief Implements conformance tests for GL_ARB_pipeline_statistics_query functionality
 */ /*-------------------------------------------------------------------*/

#include "gl4cPipelineStatisticsQueryTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <string>
#include <vector>

#ifndef GL_VERTICES_SUBMITTED_ARB
#define GL_VERTICES_SUBMITTED_ARB (0x82EE)
#endif
#ifndef GL_PRIMITIVES_SUBMITTED_ARB
#define GL_PRIMITIVES_SUBMITTED_ARB (0x82EF)
#endif
#ifndef GL_VERTEX_SHADER_INVOCATIONS_ARB
#define GL_VERTEX_SHADER_INVOCATIONS_ARB (0x82F0)
#endif
#ifndef GL_TESS_CONTROL_SHADER_PATCHES_ARB
#define GL_TESS_CONTROL_SHADER_PATCHES_ARB (0x82F1)
#endif
#ifndef GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB
#define GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB (0x82F2)
#endif
#ifndef GL_GEOMETRY_SHADER_INVOCATIONS
#define GL_GEOMETRY_SHADER_INVOCATIONS (0x887F)
#endif
#ifndef GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB
#define GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB (0x82F3)
#endif
#ifndef GL_FRAGMENT_SHADER_INVOCATIONS_ARB
#define GL_FRAGMENT_SHADER_INVOCATIONS_ARB (0x82F4)
#endif
#ifndef GL_COMPUTE_SHADER_INVOCATIONS_ARB
#define GL_COMPUTE_SHADER_INVOCATIONS_ARB (0x82F5)
#endif
#ifndef GL_CLIPPING_INPUT_PRIMITIVES_ARB
#define GL_CLIPPING_INPUT_PRIMITIVES_ARB (0x82F6)
#endif
#ifndef GL_CLIPPING_OUTPUT_PRIMITIVES_ARB
#define GL_CLIPPING_OUTPUT_PRIMITIVES_ARB (0x82F7)
#endif

namespace glcts
{
const char* PipelineStatisticsQueryUtilities::dummy_cs_code =
	"#version 430\n"
	"\n"
	"layout(local_size_x=1, local_size_y = 1, local_size_z = 1) in;\n"
	"\n"
	"layout(binding = 0) uniform atomic_uint test_counter;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    atomicCounterIncrement(test_counter);\n"
	"}\n";
const char* PipelineStatisticsQueryUtilities::dummy_fs_code = "#version 130\n"
															  "\n"
															  "out vec4 result;\n"
															  "\n"
															  "void main()\n"
															  "{\n"
															  "    result = gl_FragCoord;\n"
															  "}\n";
const char* PipelineStatisticsQueryUtilities::dummy_tc_code =
	"#version 400\n"
	"\n"
	"layout(vertices = 3) out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
	"    gl_TessLevelInner[0]                = 1.0;\n"
	"    gl_TessLevelInner[1]                = 2.0;\n"
	"    gl_TessLevelOuter[0]                = 3.0;\n"
	"    gl_TessLevelOuter[1]                = 4.0;\n"
	"    gl_TessLevelOuter[2]                = 5.0;\n"
	"    gl_TessLevelOuter[3]                = 6.0;\n"
	"}\n";
const char* PipelineStatisticsQueryUtilities::dummy_te_code =
	"#version 400\n"
	"\n"
	"layout(triangles) in;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = gl_TessCoord.xyxy * gl_in[gl_PrimitiveID].gl_Position;\n"
	"}\n";
const char* PipelineStatisticsQueryUtilities::dummy_vs_code = "#version 130\n"
															  "\n"
															  "in vec4 position;\n"
															  "\n"
															  "void main()\n"
															  "{\n"
															  "    gl_Position = position;\n"
															  "}\n";

/** An array holding all query targets that are introduced by GL_ARB_pipeline_statistics_query */
const glw::GLenum PipelineStatisticsQueryUtilities::query_targets[] = {
	GL_VERTICES_SUBMITTED_ARB,
	GL_PRIMITIVES_SUBMITTED_ARB,
	GL_VERTEX_SHADER_INVOCATIONS_ARB,
	GL_TESS_CONTROL_SHADER_PATCHES_ARB,
	GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB,
	GL_GEOMETRY_SHADER_INVOCATIONS,
	GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB,
	GL_FRAGMENT_SHADER_INVOCATIONS_ARB,
	GL_COMPUTE_SHADER_INVOCATIONS_ARB,
	GL_CLIPPING_INPUT_PRIMITIVES_ARB,
	GL_CLIPPING_OUTPUT_PRIMITIVES_ARB,
};
const unsigned int PipelineStatisticsQueryUtilities::n_query_targets = sizeof(query_targets) / sizeof(query_targets[0]);

/* Offsets that point to locations in a buffer object storage that will hold
 * query object result values for a specific value type. */
const unsigned int PipelineStatisticsQueryUtilities::qo_bo_int_offset   = (0);
const unsigned int PipelineStatisticsQueryUtilities::qo_bo_int64_offset = (0 + 4 /* int */ + 4 /* alignment */);
const unsigned int PipelineStatisticsQueryUtilities::qo_bo_uint_offset  = (0 + 8 /* int + alignment */ + 8 /* int64 */);
const unsigned int PipelineStatisticsQueryUtilities::qo_bo_uint64_offset =
	(0 + 8 /* int + alignment */ + 8 /* int64 */ + 8 /* uint + alignment */);
const unsigned int PipelineStatisticsQueryUtilities::qo_bo_size = 32;

/** Buffer object size required to run the second functional test. */
const unsigned int PipelineStatisticsQueryTestFunctional2::bo_size = 32;

/** Builds body of a geometry shader, given user-specified properties.
 *
 *  This function works in two different ways:
 *
 *  1) If the caller only needs the geometry shader to use a single stream, the body
 *     will be constructed in a way that ignores stream existence completely.
 *  2) Otherwise, the shader will only be compilable by platforms that support vertex
 *     streams.
 *
 *  The shader will emit @param n_primitives_to_emit_in_stream0 primitives on the zeroth
 *  stream, (@param n_primitives_to_emit_in_stream0 + 1) primitives on the first stream,
 *  and so on.
 *
 *  @param gs_input                         Input primitive type that should be used by the geometry shader body.
 *  @param gs_output                        Output primitive type that should be used by the geometry shader body.
 *  @param n_primitives_to_emit_in_stream0  Number of primitives to be emitted on the zeroth vertex stream.
 *  @param n_streams                        Number of streams the geometry shader should emit primitives on.
 *
 *  @return Geometry shader body.
 **/
std::string PipelineStatisticsQueryUtilities::buildGeometryShaderBody(_geometry_shader_input  gs_input,
																	  _geometry_shader_output gs_output,
																	  unsigned int n_primitives_to_emit_in_stream0,
																	  unsigned int n_streams)
{
	DE_ASSERT(n_primitives_to_emit_in_stream0 >= 1);
	DE_ASSERT(n_streams >= 1);

	/* Each stream will output (n+1) primitives, where n corresponds to the number of primitives emitted
	 * by the previous stream. Stream 0 emits user-defined number of primitievs.
	 */
	std::stringstream gs_body_sstream;
	const std::string gs_input_string  = getGLSLStringForGSInput(gs_input);
	const std::string gs_output_string = getGLSLStringForGSOutput(gs_output);
	unsigned int	  n_max_vertices   = 0;
	unsigned int	  n_vertices_required_for_gs_output =
		PipelineStatisticsQueryUtilities::getNumberOfVerticesForGSOutput(gs_output);

	for (unsigned int n_stream = 0; n_stream < n_streams; ++n_stream)
	{
		n_max_vertices += n_vertices_required_for_gs_output * (n_primitives_to_emit_in_stream0 + n_stream);
	} /* for (all streams) */

	/* Form the preamble. Note that we need to use the right ES SL version,
	 * since vertex streams are not a core GL3.2 feature.
	 **/
	gs_body_sstream << ((n_streams > 1) ? "#version 400" : "#version 150\n") << "\n"
																				"layout("
					<< gs_input_string << ")                 in;\n"
										  "layout("
					<< gs_output_string << ", max_vertices=" << n_max_vertices << ") out;\n";

	/* If we need to define multiple streams, do it now */
	if (n_streams > 1)
	{
		gs_body_sstream << "\n";

		for (unsigned int n_stream = 0; n_stream < n_streams; ++n_stream)
		{
			gs_body_sstream << "layout(stream = " << n_stream << ") out vec4 out_stream" << n_stream << ";\n";
		} /* for (all streams) */
	}	 /* if (n_streams > 1) */

	/* Contine forming actual body */
	gs_body_sstream << "\n"
					   "void main()\n"
					   "{\n";

	/* Emit primitives */
	const unsigned int n_output_primitive_vertices =
		PipelineStatisticsQueryUtilities::getNumberOfVerticesForGSOutput(gs_output);

	for (unsigned int n_stream = 0; n_stream < n_streams; ++n_stream)
	{
		const unsigned int n_primitives_to_emit = n_primitives_to_emit_in_stream0 + n_stream;

		for (unsigned int n_primitive = 0; n_primitive < n_primitives_to_emit; ++n_primitive)
		{
			for (unsigned int n_vertex = 0; n_vertex < n_output_primitive_vertices; ++n_vertex)
			{
				gs_body_sstream << "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n";

				if (n_streams == 1)
				{
					gs_body_sstream << "    EmitVertex();\n";
				}
				else
				{
					gs_body_sstream << "    EmitStreamVertex(" << n_stream << ");\n";
				}
			}

			if (n_streams == 1)
			{
				gs_body_sstream << "    EndPrimitive();\n";
			}
			else
			{
				gs_body_sstream << "    EndStreamPrimitive(" << n_stream << ");\n";
			}
		} /* for (all primitives the caller wants the shader to emit) */
	}	 /* for (all streams) */

	gs_body_sstream << "}\n";

	return gs_body_sstream.str();
}

/** Executes the query and collects the result data from both query object buffer object
 *  (if these are supported by the running OpenGL implementation) and the query counters.
 *  The result data is then exposed via @param out_result.
 *
 *  @param query_type     Type of the query to be executed for the iteration.
 *  @param qo_id          ID of the query object to be used for the execution.
 *                        The query object must not have been assigned a type
 *                        prior to the call, or the type must be a match with
 *                        @param query_type .
 *  @param qo_bo_id       ID of the query buffer object to use for the call.
 *                        Pass 0, if the running OpenGL implementation does not
 *                        support QBOs.
 *  @param pfn_draw       Function pointer to caller-specific routine that is
 *                        going to execute the draw call. Must not be NULL.
 *  @param draw_user_arg  Caller-specific user argument to be passed with the
 *                        @param pfn_draw callback.
 *  @param render_context glu::RenderContext& to be used by the method.
 *  @param test_context   tcu::TestContext& to be used by the method.
 *  @param context_info   glu::ContextInfo& to be used by the method.
 *  @param out_result     Deref will be used to store the test execution result.
 *                        Must not be NULL. Will only be modified if the method
 *                        returns true.
 *
 *  @return true if the test executed successfully, and @param out_result 's fields
 *          were modified.
 *
 */
bool PipelineStatisticsQueryUtilities::executeQuery(glw::GLenum query_type, glw::GLuint qo_id, glw::GLuint qo_bo_id,
													PFNQUERYDRAWHANDLERPROC pfn_draw, void* draw_user_arg,
													const glu::RenderContext& render_context,
													tcu::TestContext&		  test_context,
													const glu::ContextInfo&   context_info,
													_test_execution_result*   out_result)
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = render_context.getFunctions();
	bool				  result	 = true;

	/* Check if the implementation provides non-zero number of bits for the query.
	 * Queries, for which GL implementations provide zero bits of space return
	 * indeterminate values, so we should skip them */
	glw::GLint n_query_bits = 0;

	gl.getQueryiv(query_type, GL_QUERY_COUNTER_BITS, &n_query_bits);

	error_code = gl.getError();
	if (error_code != GL_NO_ERROR)
	{
		test_context.getLog() << tcu::TestLog::Message
							  << "glGetQueryiv() call failed for GL_QUERY_COUNTER_BITS pname and "
							  << PipelineStatisticsQueryUtilities::getStringForEnum(query_type) << "query target."
							  << tcu::TestLog::EndMessage;

		return false;
	}

	if (n_query_bits == 0)
	{
		test_context.getLog() << tcu::TestLog::Message << "Skipping "
														  "["
							  << PipelineStatisticsQueryUtilities::getStringForEnum(query_type)
							  << "]"
								 ": zero bits available for counter storage"
							  << tcu::TestLog::EndMessage;

		return result;
	}

	/* Start the query */
	gl.beginQuery(query_type, qo_id);

	error_code = gl.getError();
	if (error_code != GL_NO_ERROR)
	{
		test_context.getLog() << tcu::TestLog::Message
							  << "A valid glBeginQuery() call generated the following error code:"
								 "["
							  << error_code << "]" << tcu::TestLog::EndMessage;

		return false;
	}

	/* If GL_ARB_query_buffer_object is supported and the caller supplied a BO id, use
	 * it before we fire any draw calls */
	if (context_info.isExtensionSupported("GL_ARB_query_buffer_object") && qo_bo_id != 0)
	{
		gl.bindBuffer(GL_QUERY_BUFFER, qo_bo_id);

		error_code = gl.getError();
		if (error_code != GL_NO_ERROR)
		{
			test_context.getLog() << tcu::TestLog::Message
								  << "Could not bind a buffer object to GL_QUERY_BUFFER buffer object "
									 "binding point. Error reported:"
									 "["
								  << error_code << "]" << tcu::TestLog::EndMessage;

			/* Stop the query before we leave */
			gl.endQuery(query_type);

			return false;
		} /* if (buffer binding operation failed) */
	}	 /* if (GL_ARB_query_buffer_object extension is supported and the supplied QO BO id
	 *     is not 0) */
	else
	{
		/* Reset the QO BO id, so that we can skip the checks later */
		qo_bo_id = 0;
	}

	/* Perform the draw calls, if any supplied call-back function pointer was supplied
	 * by the caller. */
	if (pfn_draw != DE_NULL)
	{
		pfn_draw(draw_user_arg);
	}

	/* End the query */
	gl.endQuery(query_type);

	error_code = gl.getError();
	if (error_code != GL_NO_ERROR)
	{
		test_context.getLog() << tcu::TestLog::Message << "glEndQuery() call failed with error code"
														  "["
							  << error_code << "]" << tcu::TestLog::EndMessage;

		return false;
	} /* if (glEndQuery() call failed) */

	/* We now need to retrieve the result data using all query getter functions
	 * GL has to offer. This will be handled in two iterations:
	 *
	 * 1. The data will be retrieved using the getters without a QO BO being bound.
	 * 2. If QO was provided, we will need to issue all getter calls executed against
	 *    the QO BO. We will then need to retrieve that data directly from the BO
	 *    storage.
	 */
	const unsigned int iteration_index_wo_qo_bo   = 0;
	const unsigned int iteration_index_with_qo_bo = 1;

	for (unsigned int n_iteration = 0; n_iteration < 2; /* as per description */
		 ++n_iteration)
	{
		glw::GLint*	offset_int			 = DE_NULL;
		glw::GLint64*  offset_int64			 = DE_NULL;
		glw::GLuint*   offset_uint			 = DE_NULL;
		glw::GLuint64* offset_uint64		 = DE_NULL;
		glw::GLint	 result_int			 = INT_MAX;
		glw::GLint64   result_int64			 = LLONG_MAX;
		bool		   result_int64_written  = false;
		glw::GLuint	result_uint			 = UINT_MAX;
		glw::GLuint64  result_uint64		 = ULLONG_MAX;
		bool		   result_uint64_written = false;

		/* Skip the QO BO iteration if QO BO id has not been provided */
		if (n_iteration == iteration_index_with_qo_bo && qo_bo_id == 0)
		{
			continue;
		}

		/* Determine the offsets we should use for the getter calls */
		if (n_iteration == iteration_index_wo_qo_bo)
		{
			offset_int	= &result_int;
			offset_int64  = &result_int64;
			offset_uint   = &result_uint;
			offset_uint64 = &result_uint64;
		}
		else
		{
			offset_int	= (glw::GLint*)(deUintptr)PipelineStatisticsQueryUtilities::qo_bo_int_offset;
			offset_int64  = (glw::GLint64*)(deUintptr)PipelineStatisticsQueryUtilities::qo_bo_int64_offset;
			offset_uint   = (glw::GLuint*)(deUintptr)PipelineStatisticsQueryUtilities::qo_bo_uint_offset;
			offset_uint64 = (glw::GLuint64*)(deUintptr)PipelineStatisticsQueryUtilities::qo_bo_uint64_offset;
		}

		/* Bind the QO BO if we need to use it for the getter calls */
		if (n_iteration == iteration_index_with_qo_bo)
		{
			gl.bindBuffer(GL_QUERY_BUFFER, qo_bo_id);
		}
		else
		{
			gl.bindBuffer(GL_QUERY_BUFFER, 0 /* buffer */);
		}

		error_code = gl.getError();
		if (error_code != GL_NO_ERROR)
		{
			test_context.getLog() << tcu::TestLog::Message
								  << "glBindBuffer() call failed for GL_QUERY_BUFFER target with error "
									 "["
								  << error_code << "]" << tcu::TestLog::EndMessage;

			return false;
		}

		/* Issue the getter calls.
		 *
		 * NOTE: 64-bit getter calls are supported only if >= GL 3.3*/
		if (glu::contextSupports(render_context.getType(), glu::ApiType::core(3, 3)))
		{
			gl.getQueryObjecti64v(qo_id, GL_QUERY_RESULT, offset_int64);

			error_code = gl.getError();
			if (error_code != GL_NO_ERROR)
			{
				test_context.getLog() << tcu::TestLog::Message << "glGetQueryObjecti64v() call failed with error "
																  "["
									  << error_code << "]" << tcu::TestLog::EndMessage;

				return false;
			}

			result_int64_written = true;
		}
		else
		{
			result_int64_written = false;
		}

		gl.getQueryObjectiv(qo_id, GL_QUERY_RESULT, offset_int);

		error_code = gl.getError();
		if (error_code != GL_NO_ERROR)
		{
			test_context.getLog() << tcu::TestLog::Message << "glGetQueryObjectiv() call failed with error "
															  "["
								  << error_code << "]" << tcu::TestLog::EndMessage;

			return false;
		}

		if (glu::contextSupports(render_context.getType(), glu::ApiType::core(3, 3)))
		{
			gl.getQueryObjectui64v(qo_id, GL_QUERY_RESULT, offset_uint64);

			error_code = gl.getError();
			if (error_code != GL_NO_ERROR)
			{
				test_context.getLog() << tcu::TestLog::Message << "glGetQueryObjectui64v() call failed with error "
																  "["
									  << error_code << "]" << tcu::TestLog::EndMessage;

				return false;
			}

			result_uint64_written = true;
		}
		else
		{
			result_uint64_written = false;
		}

		gl.getQueryObjectuiv(qo_id, GL_QUERY_RESULT, offset_uint);

		error_code = gl.getError();
		if (error_code != GL_NO_ERROR)
		{
			test_context.getLog() << tcu::TestLog::Message << "glGetQueryObjectuiv() call failed with error "
															  "["
								  << error_code << "]" << tcu::TestLog::EndMessage;

			return false;
		}

		/* If the getters wrote the result values to the BO, we need to retrieve the data
		 * from the BO storage */
		if (n_iteration == iteration_index_with_qo_bo)
		{
			/* Map the BO to process space */
			const unsigned char* bo_data_ptr = (const unsigned char*)gl.mapBuffer(GL_QUERY_BUFFER, GL_READ_ONLY);

			error_code = gl.getError();

			if (error_code != GL_NO_ERROR || bo_data_ptr == NULL)
			{
				test_context.getLog() << tcu::TestLog::Message << "QO BO mapping failed with error "
																  "["
									  << error_code << "] and data ptr returned:"
													   "["
									  << bo_data_ptr << "]" << tcu::TestLog::EndMessage;

				return false;
			}

			/* Retrieve the query result data */
			result_int	= *(glw::GLint*)(bo_data_ptr + (int)(deIntptr)offset_int);
			result_int64  = *(glw::GLint64*)(bo_data_ptr + (int)(deIntptr)offset_int64);
			result_uint   = *(glw::GLuint*)(bo_data_ptr + (int)(deIntptr)offset_uint);
			result_uint64 = *(glw::GLuint64*)(bo_data_ptr + (int)(deIntptr)offset_uint64);

			/* Unmap the BO */
			gl.unmapBuffer(GL_QUERY_BUFFER);

			error_code = gl.getError();
			if (error_code != GL_NO_ERROR)
			{
				test_context.getLog() << tcu::TestLog::Message << "QO BO unmapping failed with error "
																  "["
									  << error_code << "]" << tcu::TestLog::EndMessage;

				return false;
			}
		} /* if (QO BO iteration) */

		/* Store the retrieved data in user-provided location */
		if (n_iteration == iteration_index_with_qo_bo)
		{
			out_result->result_qo_int	= result_int;
			out_result->result_qo_int64  = result_int64;
			out_result->result_qo_uint   = result_uint;
			out_result->result_qo_uint64 = result_uint64;
		}
		else
		{
			out_result->result_int	= result_int;
			out_result->result_int64  = result_int64;
			out_result->result_uint   = result_uint;
			out_result->result_uint64 = result_uint64;
		}

		out_result->int64_written  = result_int64_written;
		out_result->uint64_written = result_uint64_written;
	} /* for (both iterations) */
	return result;
}

/** Retrieves a GLenum value corresponding to internal _primitive_type
 *  enum value.
 *
 *  @param primitive_type Internal primitive type to use for the getter call.
 *
 *  @return Corresponding GL value that can be used for the draw calls, or
 *          GL_NONE if the conversion failed.
 *
 **/
glw::GLenum PipelineStatisticsQueryUtilities::getEnumForPrimitiveType(_primitive_type primitive_type)
{
	glw::GLenum result = GL_NONE;

	switch (primitive_type)
	{
	case PRIMITIVE_TYPE_POINTS:
		result = GL_POINTS;
		break;
	case PRIMITIVE_TYPE_LINE_LOOP:
		result = GL_LINE_LOOP;
		break;
	case PRIMITIVE_TYPE_LINE_STRIP:
		result = GL_LINE_STRIP;
		break;
	case PRIMITIVE_TYPE_LINES:
		result = GL_LINES;
		break;
	case PRIMITIVE_TYPE_LINES_ADJACENCY:
		result = GL_LINES_ADJACENCY;
		break;
	case PRIMITIVE_TYPE_PATCHES:
		result = GL_PATCHES;
		break;
	case PRIMITIVE_TYPE_TRIANGLE_FAN:
		result = GL_TRIANGLE_FAN;
		break;
	case PRIMITIVE_TYPE_TRIANGLE_STRIP:
		result = GL_TRIANGLE_STRIP;
		break;
	case PRIMITIVE_TYPE_TRIANGLES:
		result = GL_TRIANGLES;
		break;
	case PRIMITIVE_TYPE_TRIANGLES_ADJACENCY:
		result = GL_TRIANGLES_ADJACENCY;
		break;

	default:
	{
		TCU_FAIL("Unrecognized primitive type");
	}
	} /* switch (primitive_type) */

	return result;
}

/** Retrieves a human-readable name for a _geometry_shader_input value.
 *
 *  @param gs_input Internal _geometry_shader_input value to use for
 *                  the conversion.
 *
 *  @return Human-readable string or empty string, if the conversion failed.
 *
 **/
std::string PipelineStatisticsQueryUtilities::getGLSLStringForGSInput(_geometry_shader_input gs_input)
{
	std::string result;

	switch (gs_input)
	{
	case GEOMETRY_SHADER_INPUT_POINTS:
		result = "points";
		break;
	case GEOMETRY_SHADER_INPUT_LINES:
		result = "lines";
		break;
	case GEOMETRY_SHADER_INPUT_LINES_ADJACENCY:
		result = "lines_adjacency";
		break;
	case GEOMETRY_SHADER_INPUT_TRIANGLES:
		result = "triangles";
		break;
	case GEOMETRY_SHADER_INPUT_TRIANGLES_ADJACENCY:
		result = "triangles_adjacency";
		break;

	default:
	{
		TCU_FAIL("Unrecognized geometry shader input enum");
	}
	} /* switch (gs_input) */

	return result;
}

/** Retrieves a human-readable string for a _geometry_shader_output value.
 *
 *  @param  gs_output _geometry_shader_output value to use for the conversion.
 *
 *  @return Requested value or empty string, if the value was not recognized.
 *
 **/
std::string PipelineStatisticsQueryUtilities::getGLSLStringForGSOutput(_geometry_shader_output gs_output)
{
	std::string result;

	switch (gs_output)
	{
	case GEOMETRY_SHADER_OUTPUT_POINTS:
		result = "points";
		break;
	case GEOMETRY_SHADER_OUTPUT_LINE_STRIP:
		result = "line_strip";
		break;
	case GEOMETRY_SHADER_OUTPUT_TRIANGLE_STRIP:
		result = "triangle_strip";
		break;

	default:
	{
		TCU_FAIL("Unrecognized geometry shader output enum");
	}
	} /* switch (gs_output) */

	return result;
}

/** Number of vertices the geometry shader can access on the input, if the shader
 *  uses @param gs_input input primitive type.
 *
 *  @param gs_input Geometry shader input to use for the query.
 *
 *  @return Requested value or 0 if @param gs_input was not recognized.
 **/
unsigned int PipelineStatisticsQueryUtilities::getNumberOfVerticesForGSInput(_geometry_shader_input gs_input)
{
	unsigned int result = 0;

	switch (gs_input)
	{
	case GEOMETRY_SHADER_INPUT_POINTS:
		result = 1;
		break;
	case GEOMETRY_SHADER_INPUT_LINES:
		result = 2;
		break;
	case GEOMETRY_SHADER_INPUT_LINES_ADJACENCY:
		result = 4;
		break;
	case GEOMETRY_SHADER_INPUT_TRIANGLES:
		result = 3;
		break;
	case GEOMETRY_SHADER_INPUT_TRIANGLES_ADJACENCY:
		result = 6;
		break;

	default:
	{
		TCU_FAIL("Unrecognized geometry shader input type");
	}
	} /* switch (gs_input) */

	return result;
}

/** Retrieves a number of vertices that need to be emitted before the shader
 *  can end the primitive, with the primitive being complete, assuming the
 *  geometry shader outputs a primitive of type described by @param gs_output.
 *
 *  @param gs_output Primitive type to be outputted by the geometry shader.
 *
 *  @return As per description, or 0 if @param gs_output was not recognized.
 **/
unsigned int PipelineStatisticsQueryUtilities::getNumberOfVerticesForGSOutput(_geometry_shader_output gs_output)
{
	unsigned int n_result_vertices = 0;

	switch (gs_output)
	{
	case GEOMETRY_SHADER_OUTPUT_LINE_STRIP:
		n_result_vertices = 2;
		break;
	case GEOMETRY_SHADER_OUTPUT_POINTS:
		n_result_vertices = 1;
		break;
	case GEOMETRY_SHADER_OUTPUT_TRIANGLE_STRIP:
		n_result_vertices = 3;
		break;

	default:
		TCU_FAIL("Unrecognized geometry shader output type");
	}

	/* All done */
	return n_result_vertices;
}

/** Returns the number of vertices a single primitive of type described by @param primitive_type
 *  consists of.
 *
 *  @param primitive_type Primitive type to use for the query.
 *
 *  @return Result value, or 0 if @param primive_type was not recognized.
 **/
unsigned int PipelineStatisticsQueryUtilities::getNumberOfVerticesForPrimitiveType(_primitive_type primitive_type)
{
	unsigned int result = 0;

	switch (primitive_type)
	{
	case PRIMITIVE_TYPE_POINTS:
		result = 1;
		break;
	case PRIMITIVE_TYPE_LINE_LOOP:  /* fall-through */
	case PRIMITIVE_TYPE_LINE_STRIP: /* fall-through */
	case PRIMITIVE_TYPE_LINES:
		result = 2;
		break;
	case PRIMITIVE_TYPE_TRIANGLE_FAN:   /* fall-through */
	case PRIMITIVE_TYPE_TRIANGLE_STRIP: /* fall-through */
	case PRIMITIVE_TYPE_TRIANGLES:
		result = 3;
		break;
	case PRIMITIVE_TYPE_LINES_ADJACENCY:
		result = 4;
		break;
	case PRIMITIVE_TYPE_TRIANGLES_ADJACENCY:
		result = 6;
		break;

	default:
		TCU_FAIL("Unrecognized primitive type");
	} /* switch (primitive_type) */

	return result;
}

/** Converts user-specified _geometry_shader_input value to a _primitive_type value.
 *
 *  @param gs_input Input value for the conversion.
 *
 *  @return Requested value, or PRIMITIVE_TYPE_COUNT if the user-specified value
 *          was unrecognized.
 **/
PipelineStatisticsQueryUtilities::_primitive_type PipelineStatisticsQueryUtilities::getPrimitiveTypeFromGSInput(
	_geometry_shader_input gs_input)
{
	_primitive_type result = PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_COUNT;

	switch (gs_input)
	{
	case GEOMETRY_SHADER_INPUT_POINTS:
		result = PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_POINTS;
		break;
	case GEOMETRY_SHADER_INPUT_LINES:
		result = PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINES;
		break;
	case GEOMETRY_SHADER_INPUT_LINES_ADJACENCY:
		result = PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINES_ADJACENCY;
		break;
	case GEOMETRY_SHADER_INPUT_TRIANGLES:
		result = PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLES;
		break;
	case GEOMETRY_SHADER_INPUT_TRIANGLES_ADJACENCY:
		result = PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLES_ADJACENCY;
		break;

	default:
	{
		TCU_FAIL("Unrecognized geometry shader input enum");
	}
	} /* switch (gs_input) */

	return result;
}

/** Converts user-specified _draw_call_type value to a human-readable string.
 *
 *  @param draw_call_type Input value to use for the conversion.
 *
 *  @return Human-readable string, or "[?]" (without the quotation marks) if
 *          the input value was not recognized.
 **/
std::string PipelineStatisticsQueryUtilities::getStringForDrawCallType(_draw_call_type draw_call_type)
{
	std::string result = "[?]";

	switch (draw_call_type)
	{
	case DRAW_CALL_TYPE_GLDRAWARRAYS:
		result = "glDrawArrays()";
		break;
	case DRAW_CALL_TYPE_GLDRAWARRAYSINDIRECT:
		result = "glDrawArraysIndirect()";
		break;
	case DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCED:
		result = "glDrawArraysInstanced()";
		break;
	case DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCEDBASEINSTANCE:
		result = "glDrawArraysInstancedBaseInstance()";
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTS:
		result = "glDrawElements()";
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSBASEVERTEX:
		result = "glDrawElementsBaseVertex()";
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSINDIRECT:
		result = "glDrawElementsIndirect()";
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCED:
		result = "glDrawElementsInstanced()";
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEINSTANCE:
		result = "glDrawElementsInstancedBaseInstance()";
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE:
		result = "glDrawElementsInstancedBaseVertexBaseInstance()";
		break;
	case DRAW_CALL_TYPE_GLDRAWRANGEELEMENTS:
		result = "glDrawRangeElements()";
		break;
	case DRAW_CALL_TYPE_GLDRAWRANGEELEMENTSBASEVERTEX:
		result = "glDrawRangeElementsBaseVertex()";
		break;
	default:
		DE_ASSERT(0);
		break;
	}

	return result;
}

/** Converts a GL enum value expressing a pipeline statistics query type
 *  into a human-readable string.
 *
 *  @param value Input value to use for the conversion.
 *
 *  @return Human-readable string or "[?]" (without the quotation marks)
 *          if the input value was not recognized.
 **/
std::string PipelineStatisticsQueryUtilities::getStringForEnum(glw::GLenum value)
{
	std::string result = "[?]";

	switch (value)
	{
	case GL_CLIPPING_INPUT_PRIMITIVES_ARB:
		result = "GL_CLIPPING_INPUT_PRIMITIVES_ARB";
		break;
	case GL_CLIPPING_OUTPUT_PRIMITIVES_ARB:
		result = "GL_CLIPPING_OUTPUT_PRIMITIVES_ARB";
		break;
	case GL_COMPUTE_SHADER_INVOCATIONS_ARB:
		result = "GL_COMPUTE_SHADER_INVOCATIONS_ARB";
		break;
	case GL_FRAGMENT_SHADER_INVOCATIONS_ARB:
		result = "GL_FRAGMENT_SHADER_INVOCATIONS_ARB";
		break;
	case GL_GEOMETRY_SHADER_INVOCATIONS:
		result = "GL_GEOMETRY_SHADER_INVOCATIONS";
		break;
	case GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB:
		result = "GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB";
		break;
	case GL_PRIMITIVES_SUBMITTED_ARB:
		result = "GL_PRIMITIVES_SUBMITTED_ARB";
		break;
	case GL_TESS_CONTROL_SHADER_PATCHES_ARB:
		result = "GL_TESS_CONTROL_SHADER_PATCHES_ARB";
		break;
	case GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB:
		result = "GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB";
		break;
	case GL_VERTEX_SHADER_INVOCATIONS_ARB:
		result = "GL_VERTEX_SHADER_INVOCATIONS_ARB";
		break;
	case GL_VERTICES_SUBMITTED_ARB:
		result = "GL_VERTICES_SUBMITTED_ARB";
		break;
	} /* switch (value) */

	return result;
}

/** Converts a _primitive_type value into a human-readable string.
 *
 *  @param primitive_type Input value to use for the conversion.
 *
 *  @return Requested string or "[?]" (without the quotation marks)
 *          if the input value was not recognized.
 **/
std::string PipelineStatisticsQueryUtilities::getStringForPrimitiveType(_primitive_type primitive_type)
{
	std::string result = "[?]";

	switch (primitive_type)
	{
	case PRIMITIVE_TYPE_POINTS:
		result = "GL_POINTS";
		break;
	case PRIMITIVE_TYPE_LINE_LOOP:
		result = "GL_LINE_LOOP";
		break;
	case PRIMITIVE_TYPE_LINE_STRIP:
		result = "GL_LINE_STRIP";
		break;
	case PRIMITIVE_TYPE_LINES:
		result = "GL_LINES";
		break;
	case PRIMITIVE_TYPE_LINES_ADJACENCY:
		result = "GL_LINES_ADJACENCY";
		break;
	case PRIMITIVE_TYPE_PATCHES:
		result = "GL_PATCHES";
		break;
	case PRIMITIVE_TYPE_TRIANGLE_FAN:
		result = "GL_TRIANGLE_FAN";
		break;
	case PRIMITIVE_TYPE_TRIANGLE_STRIP:
		result = "GL_TRIANGLE_STRIP";
		break;
	case PRIMITIVE_TYPE_TRIANGLES:
		result = "GL_TRIANGLES";
		break;
	case PRIMITIVE_TYPE_TRIANGLES_ADJACENCY:
		result = "GL_TRIANGLES_ADJACENCY";
		break;
	default:
		DE_ASSERT(0);
		break;
	}

	return result;
}

/** Tells if it is safe to use a specific draw call type.
 *
 *  @param draw_call Draw call type to use for the query.
 *
 *  @return True if corresponding GL entry-point is available.
 */
bool PipelineStatisticsQueryUtilities::isDrawCallSupported(_draw_call_type draw_call, const glw::Functions& gl)
{

	bool result = false;

	switch (draw_call)
	{
	case DRAW_CALL_TYPE_GLDRAWARRAYS:
		result = (gl.drawArrays != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWARRAYSINDIRECT:
		result = (gl.drawArraysIndirect != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCED:
		result = (gl.drawArraysInstanced != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCEDBASEINSTANCE:
		result = (gl.drawArraysInstancedBaseInstance != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTS:
		result = (gl.drawElements != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSBASEVERTEX:
		result = (gl.drawElementsBaseVertex != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSINDIRECT:
		result = (gl.drawElementsIndirect != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCED:
		result = (gl.drawElementsInstanced != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEINSTANCE:
		result = (gl.drawElementsInstancedBaseInstance != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE:
		result = (gl.drawElementsInstancedBaseVertexBaseInstance != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWRANGEELEMENTS:
		result = (gl.drawRangeElements != DE_NULL);
		break;
	case DRAW_CALL_TYPE_GLDRAWRANGEELEMENTSBASEVERTEX:
		result = (gl.drawRangeElementsBaseVertex != DE_NULL);
		break;

	default:
	{
		TCU_FAIL("Unrecognized draw call type");
	}
	} /* switch (draw_call) */

	return result;
}

/** Tells if user-specified draw call type is an instanced draw call.
 *
 *  @param draw_call Input value to use for the conversion.
 *
 *  @return true if @param draw_call corresponds to an instanced draw call,
 *          false otherwise.
 **/
bool PipelineStatisticsQueryUtilities::isInstancedDrawCall(_draw_call_type draw_call)
{
	bool result =
		(draw_call == PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWARRAYSINDIRECT ||
		 draw_call == PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCED ||
		 draw_call == PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCEDBASEINSTANCE ||
		 draw_call == PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSINDIRECT ||
		 draw_call == PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCED ||
		 draw_call == PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEINSTANCE ||
		 draw_call == PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE);

	return result;
}

/** Tells if the running GL implementation supports user-specified pipeline
 *  statistics query.
 *
 *  @param value          GL enum definining the pipeline statistics query type
 *                        that should be used for the query.
 *  @param context_info   glu::ContextInfo instance that can be used by the method.
 *  @param render_context glu::RenderContext instance that can be used by the method.
 *
 *  @return true if the query is supported, false otherwise. This method will return
 *          true for unrecognized enums.
 **/
bool PipelineStatisticsQueryUtilities::isQuerySupported(glw::GLenum value, const glu::ContextInfo& context_info,
														const glu::RenderContext& render_context)
{
	bool result = true;

	switch (value)
	{
	case GL_GEOMETRY_SHADER_INVOCATIONS:
	case GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB:
	{
		if (!glu::contextSupports(render_context.getType(), glu::ApiType::core(3, 2)) &&
			!context_info.isExtensionSupported("GL_ARB_geometry_shader4"))
		{
			result = false;
		}

		break;
	}

	case GL_TESS_CONTROL_SHADER_PATCHES_ARB:
	case GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB:
	{
		if (!glu::contextSupports(render_context.getType(), glu::ApiType::compatibility(4, 0)) &&
			!context_info.isExtensionSupported("GL_ARB_tessellation_shader"))
		{
			result = false;
		}

		break;
	}

	case GL_COMPUTE_SHADER_INVOCATIONS_ARB:
	{
		if (!glu::contextSupports(render_context.getType(), glu::ApiType::core(4, 3)) &&
			!context_info.isExtensionSupported("GL_ARB_compute_shader"))
		{
			result = false;
		}

		break;
	}
	} /* switch (value) */

	return result;
}

/** Takes a filled _test_execution_result structure and performs the validation
 *  of the embedded data.
 *
 *  @param run_result                   A filled _test_execution_result structure that
 *                                      should be used as input by the method.
 *  @param n_expected_values            Number of possible expected values.
 *  @param expected_values              Array of possible expected values.
 *  @param should_check_qo_bo_values    true if the method should also verify the values
 *                                      retrieved from a query buffer object, false
 *                                      if it is OK to ignore them.
 *  @param query_type                   Pipeline statistics query type that was used to
 *                                      capture the results stored in @param run_result .
 *  @param draw_call_type_ptr           Type of the draw call that was used to capture the
 *                                      results stored in @param run_result .
 *  @param primitive_type_ptr           Primitive type that was used for the draw call that
 *                                      was used to capture the results stored in @param
 *                                      run_result .
 *  @param is_primitive_restart_enabled true if "Primitive Restart" rendering mode had been enabled
 *                                      when the draw call used to capture the results was made.
 *  @param test_context                 tcu::TestContext instance that the method can use.
 *  @param verification_type            Tells how the captured values should be compared against the
 *                                      reference value.
 *
 *  @return true if the result values were found valid, false otherwise.
 **/
bool PipelineStatisticsQueryUtilities::verifyResultValues(
	const _test_execution_result& run_result, unsigned int n_expected_values, const glw::GLuint64* expected_values,
	bool should_check_qo_bo_values, const glw::GLenum query_type, const _draw_call_type* draw_call_type_ptr,
	const _primitive_type* primitive_type_ptr, bool is_primitive_restart_enabled, tcu::TestContext& test_context,
	_verification_type verification_type)
{
	bool result = true;

	/* Make sure all values are set to one of the expected values */
	std::string draw_call_name;
	std::string primitive_name;

	bool is_result_int_valid	   = false;
	bool is_result_int64_valid	 = false;
	bool is_result_uint_valid	  = false;
	bool is_result_uint64_valid	= false;
	bool is_result_qo_int_valid	= false;
	bool is_result_qo_int64_valid  = false;
	bool is_result_qo_uint_valid   = false;
	bool is_result_qo_uint64_valid = false;

	if (draw_call_type_ptr != DE_NULL)
	{
		draw_call_name = getStringForDrawCallType(*draw_call_type_ptr);
	}
	else
	{
		draw_call_name = "(does not apply)";
	}

	if (primitive_type_ptr != DE_NULL)
	{
		primitive_name = getStringForPrimitiveType(*primitive_type_ptr);
	}
	else
	{
		primitive_name = "(does not apply)";
	}

	for (unsigned int n_expected_value = 0; n_expected_value < n_expected_values; ++n_expected_value)
	{
		glw::GLuint64 expected_value = 0;

		expected_value = expected_values[n_expected_value];

		if ((verification_type == VERIFICATION_TYPE_EXACT_MATCH &&
			 (glw::GLuint64)run_result.result_int == expected_value) ||
			(verification_type == VERIFICATION_TYPE_EQUAL_OR_GREATER &&
			 (glw::GLuint64)run_result.result_int >= expected_value))
		{
			is_result_int_valid = true;
		}

		if (run_result.int64_written && ((verification_type == VERIFICATION_TYPE_EXACT_MATCH &&
										  run_result.result_int64 == (glw::GLint64)expected_value) ||
										 (verification_type == VERIFICATION_TYPE_EQUAL_OR_GREATER &&
										  run_result.result_int64 >= (glw::GLint64)expected_value)))
		{
			is_result_int64_valid = true;
		}

		if ((verification_type == VERIFICATION_TYPE_EXACT_MATCH &&
			 (glw::GLuint64)run_result.result_uint == expected_value) ||
			(verification_type == VERIFICATION_TYPE_EQUAL_OR_GREATER &&
			 (glw::GLuint64)run_result.result_uint >= expected_value))
		{
			is_result_uint_valid = true;
		}

		if (run_result.uint64_written &&
			((verification_type == VERIFICATION_TYPE_EXACT_MATCH && run_result.result_uint64 == expected_value) ||
			 (verification_type == VERIFICATION_TYPE_EQUAL_OR_GREATER && run_result.result_uint64 >= expected_value)))
		{
			is_result_uint64_valid = true;
		}

		if (should_check_qo_bo_values)
		{
			if ((verification_type == VERIFICATION_TYPE_EXACT_MATCH &&
				 (glw::GLuint64)run_result.result_qo_int == expected_value) ||
				(verification_type == VERIFICATION_TYPE_EQUAL_OR_GREATER &&
				 (glw::GLuint64)run_result.result_qo_int >= expected_value))
			{
				is_result_qo_int_valid = true;
			}

			if (run_result.int64_written && ((verification_type == VERIFICATION_TYPE_EXACT_MATCH &&
											  run_result.result_qo_int64 == (glw::GLint64)expected_value) ||
											 (verification_type == VERIFICATION_TYPE_EQUAL_OR_GREATER &&
											  run_result.result_qo_int64 >= (glw::GLint64)expected_value)))
			{
				is_result_qo_int64_valid = true;
			}

			if ((verification_type == VERIFICATION_TYPE_EXACT_MATCH &&
				 (glw::GLuint64)run_result.result_qo_uint == expected_value) ||
				(verification_type == VERIFICATION_TYPE_EQUAL_OR_GREATER &&
				 (glw::GLuint64)run_result.result_qo_uint >= expected_value))
			{
				is_result_qo_uint_valid = true;
			}

			if (run_result.uint64_written && ((verification_type == VERIFICATION_TYPE_EXACT_MATCH &&
											   run_result.result_qo_uint64 == expected_value) ||
											  (verification_type == VERIFICATION_TYPE_EQUAL_OR_GREATER &&
											   run_result.result_qo_uint64 >= expected_value)))
			{
				is_result_qo_uint64_valid = true;
			}
		} /* if (should_check_qo_bo_values) */
	}	 /* for (both expected values) */

	if (!is_result_int_valid)
	{
		std::string log = PipelineStatisticsQueryUtilities::getVerifyResultValuesErrorString<glw::GLint>(
			run_result.result_int, "non-QO BO int32", n_expected_values, expected_values, query_type, draw_call_name,
			primitive_name, is_primitive_restart_enabled);

		test_context.getLog() << tcu::TestLog::Message << log.c_str() << tcu::TestLog::EndMessage;

		result = false;
	}

	if (run_result.int64_written && !is_result_int64_valid)
	{
		std::string log = PipelineStatisticsQueryUtilities::getVerifyResultValuesErrorString<glw::GLint64>(
			run_result.result_int64, "non-QO BO int64", n_expected_values, expected_values, query_type, draw_call_name,
			primitive_name, is_primitive_restart_enabled);

		test_context.getLog() << tcu::TestLog::Message << log.c_str() << tcu::TestLog::EndMessage;

		result = false;
	}

	if (!is_result_uint_valid)
	{
		std::string log = PipelineStatisticsQueryUtilities::getVerifyResultValuesErrorString<glw::GLuint>(
			run_result.result_uint, "non-QO BO uint32", n_expected_values, expected_values, query_type, draw_call_name,
			primitive_name, is_primitive_restart_enabled);

		test_context.getLog() << tcu::TestLog::Message << log.c_str() << tcu::TestLog::EndMessage;

		result = false;
	}

	if (run_result.uint64_written && !is_result_uint64_valid)
	{
		std::string log = PipelineStatisticsQueryUtilities::getVerifyResultValuesErrorString<glw::GLuint64>(
			run_result.result_uint, "non-QO BO uint64", n_expected_values, expected_values, query_type, draw_call_name,
			primitive_name, is_primitive_restart_enabled);

		test_context.getLog() << tcu::TestLog::Message << log.c_str() << tcu::TestLog::EndMessage;

		result = false;
	}

	if (should_check_qo_bo_values)
	{
		if (!is_result_qo_int_valid)
		{
			std::string log = PipelineStatisticsQueryUtilities::getVerifyResultValuesErrorString<glw::GLint>(
				run_result.result_qo_int, "QO BO int32", n_expected_values, expected_values, query_type, draw_call_name,
				primitive_name, is_primitive_restart_enabled);

			test_context.getLog() << tcu::TestLog::Message << log.c_str() << tcu::TestLog::EndMessage;

			result = false;
		}

		if (run_result.int64_written && !is_result_qo_int64_valid)
		{
			std::string log = PipelineStatisticsQueryUtilities::getVerifyResultValuesErrorString<glw::GLint64>(
				run_result.result_qo_int64, "QO BO int64", n_expected_values, expected_values, query_type,
				draw_call_name, primitive_name, is_primitive_restart_enabled);

			test_context.getLog() << tcu::TestLog::Message << log.c_str() << tcu::TestLog::EndMessage;

			result = false;
		}

		if (!is_result_qo_uint_valid)
		{
			std::string log = PipelineStatisticsQueryUtilities::getVerifyResultValuesErrorString<glw::GLuint>(
				run_result.result_qo_uint, "QO BO uint32", n_expected_values, expected_values, query_type,
				draw_call_name, primitive_name, is_primitive_restart_enabled);

			test_context.getLog() << tcu::TestLog::Message << log.c_str() << tcu::TestLog::EndMessage;

			result = false;
		}

		if (run_result.uint64_written && !is_result_qo_uint64_valid)
		{
			std::string log = PipelineStatisticsQueryUtilities::getVerifyResultValuesErrorString<glw::GLuint64>(
				run_result.result_qo_uint64, "QO BO uint64", n_expected_values, expected_values, query_type,
				draw_call_name, primitive_name, is_primitive_restart_enabled);

			test_context.getLog() << tcu::TestLog::Message << log.c_str() << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	return result;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
PipelineStatisticsQueryTestAPICoverage1::PipelineStatisticsQueryTestAPICoverage1(deqp::Context& context)
	: TestCase(context, "api_coverage_invalid_glbeginquery_calls",
			   "Verifies that an attempt to assign a different query object type "
			   "to an object thas has already been assigned a type, results in "
			   "an error.")
	, m_qo_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestAPICoverage1::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_qo_id != 0)
	{
		gl.deleteQueries(1, &m_qo_id);

		m_qo_id = 0;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult PipelineStatisticsQueryTestAPICoverage1::iterate()
{
	const glu::ContextInfo& context_info   = m_context.getContextInfo();
	bool					has_passed	 = true;
	glu::RenderContext&		render_context = m_context.getRenderContext();
	glu::ContextType		contextType	= m_context.getRenderContext().getType();

	/* Only continue if GL_ARB_pipeline_statistics_query extension is supported */
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!context_info.isExtensionSupported("GL_ARB_pipeline_statistics_query"))
	{
		throw tcu::NotSupportedError("GL_ARB_pipeline_statistics_query extension is not supported");
	}

	/* Verify that a query object which has been assigned a pipeline statistics query target A,
	 * cannot be assigned another type B (assuming A != B) */
	const glw::Functions& gl = render_context.getFunctions();

	for (unsigned int n_current_item = 0; n_current_item < PipelineStatisticsQueryUtilities::n_query_targets;
		 ++n_current_item)
	{
		glw::GLenum current_pq = PipelineStatisticsQueryUtilities::query_targets[n_current_item];

		/* Make sure the query is supported */
		if (!PipelineStatisticsQueryUtilities::isQuerySupported(current_pq, context_info, render_context))
		{
			continue;
		}

		/* Generate a new query object */
		gl.genQueries(1, &m_qo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries() call failed.");

		/* Assign a type to the query object */
		gl.beginQuery(current_pq, m_qo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQuery() call failed.");

		gl.endQuery(current_pq);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery() call failed.");

		for (unsigned int n_different_item = 0; n_different_item < PipelineStatisticsQueryUtilities::n_query_targets;
			 ++n_different_item)
		{
			glw::GLenum different_pq = PipelineStatisticsQueryUtilities::query_targets[n_different_item];

			if (current_pq == different_pq)
			{
				/* Skip valid iterations */
				continue;
			}

			/* Make sure the other query type is supported */
			if (!PipelineStatisticsQueryUtilities::isQuerySupported(different_pq, context_info, render_context))
			{
				continue;
			}

			/* Try using a different type for the same object */
			glw::GLenum error_code = GL_NO_ERROR;

			gl.beginQuery(different_pq, m_qo_id);

			/* Has GL_INVALID_OPERATION error been generated? */
			error_code = gl.getError();

			if (error_code != GL_INVALID_OPERATION)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Unexpected error code "
															   "["
								   << error_code << "]"
													" generated when using glBeginQuery() for a query object of type "
													"["
								   << PipelineStatisticsQueryUtilities::getStringForEnum(current_pq)
								   << "]"
									  ", when used for a query type "
									  "["
								   << PipelineStatisticsQueryUtilities::getStringForEnum(different_pq) << "]"
								   << tcu::TestLog::EndMessage;

				has_passed = false;
			}

			if (error_code == GL_NO_ERROR)
			{
				/* Clean up before we continue */
				gl.endQuery(different_pq);
				GLU_EXPECT_NO_ERROR(gl.getError(),
									"glEndQuery() should not have failed for a successful glBeginQuery() call");
			}
		} /* for (all query object types) */

		/* We need to switch to a new pipeline statistics query, so
		 * delete the query object */
		gl.deleteQueries(1, &m_qo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteQueries() call failed.");
	} /* for (all pipeline statistics query object types) */

	if (has_passed)
	{
		/* Test case passed */
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
PipelineStatisticsQueryTestAPICoverage2::PipelineStatisticsQueryTestAPICoverage2(deqp::Context& context)
	: TestCase(context, "api_coverage_unsupported_calls",
			   "Verifies that an attempt of using unsupported pipeline statistics queries"
			   " results in a GL_INVALID_ENUM error.")
	, m_qo_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestAPICoverage2::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_qo_id != 0)
	{
		gl.deleteQueries(1, &m_qo_id);

		m_qo_id = 0;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult PipelineStatisticsQueryTestAPICoverage2::iterate()
{
	const glu::ContextInfo& context_info   = m_context.getContextInfo();
	glw::GLenum				error_code	 = GL_NO_ERROR;
	bool					has_passed	 = true;
	glu::RenderContext&		render_context = m_context.getRenderContext();
	const glw::Functions&   gl			   = render_context.getFunctions();
	glu::ContextType		contextType	= m_context.getRenderContext().getType();

	/* Only continue if GL_ARB_pipeline_statistics_query extension is supported */
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_pipeline_statistics_query"))
	{
		throw tcu::NotSupportedError("GL_ARB_pipeline_statistics_query extension is not supported");
	}

	/* Generate a query object we will use for the tests */
	gl.genQueries(1, &m_qo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries() call failed.");

	const glw::GLenum query_types[] = { GL_GEOMETRY_SHADER_INVOCATIONS, GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB,
										GL_TESS_CONTROL_SHADER_PATCHES_ARB, GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB,
										GL_COMPUTE_SHADER_INVOCATIONS_ARB };
	const unsigned int n_query_types = sizeof(query_types) / sizeof(query_types[0]);

	for (unsigned int n_query_type = 0; n_query_type < n_query_types; ++n_query_type)
	{
		glw::GLenum query_type = query_types[n_query_type];

		if (!PipelineStatisticsQueryUtilities::isQuerySupported(query_type, context_info, render_context))
		{
			gl.beginQuery(query_type, m_qo_id);

			error_code = gl.getError();
			if (error_code != GL_INVALID_ENUM)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "glBeginQuery() call did not generate a GL_INVALID_ENUM error "
									  "for an unsupported query type "
									  "["
								   << PipelineStatisticsQueryUtilities::getStringForEnum(query_type) << "]"
								   << tcu::TestLog::EndMessage;

				has_passed = false;
			}

			/* If the query succeeded, stop it before we continue */
			if (error_code == GL_NO_ERROR)
			{
				gl.endQuery(query_type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery() call failed.");
			}
		} /* if (query is not supported) */
	}	 /* for (all query types) */

	if (has_passed)
	{
		/* Test case passed */
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
PipelineStatisticsQueryTestFunctionalBase::PipelineStatisticsQueryTestFunctionalBase(deqp::Context& context,
																					 const char*	name,
																					 const char*	description)
	: TestCase(context, name, description)
	, m_bo_qo_id(0)
	, m_fbo_id(0)
	, m_po_id(0)
	, m_qo_id(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_vbo_id(0)
	, m_to_height(64)
	, m_to_width(64)
	, m_current_draw_call_type(PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_COUNT)
	, m_current_primitive_type(PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_COUNT)
	, m_indirect_draw_call_baseinstance_argument(0)
	, m_indirect_draw_call_basevertex_argument(0)
	, m_indirect_draw_call_count_argument(0)
	, m_indirect_draw_call_first_argument(0)
	, m_indirect_draw_call_firstindex_argument(0)
	, m_indirect_draw_call_primcount_argument(0)
{
	/* Left blank intentionally */
}

/** Creates a program object that can be used for dispatch/draw calls, using
 *  user-specified shader bodies. The method can either create a compute program,
 *  or a regular rendering program.
 *
 *  ID of the initialized program object is stored in m_po_id.
 *
 *  @param cs_body Compute shader body. If not NULL, all other arguments must
 *                 be NULL.
 *  @param fs_body Fragment shader body. If not NULL, @param cs_body must be NULL.
 *  @param gs_body Geometry shader body. If not NULL, @param cs_body must be NULL.
 *  @param tc_body Tess control shader body. If not NULL, @param cs_body must be NULL.
 *  @param te_body Tess evaluation shader body. If not NULL, @param cs_body must be NULL.
 *  @param vs_body Vertex shader body. If not NULL, @param cs_body must be NULL.
 *
 * */
void PipelineStatisticsQueryTestFunctionalBase::buildProgram(const char* cs_body, const char* fs_body,
															 const char* gs_body, const char* tc_body,
															 const char* te_body, const char* vs_body)
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	glw::GLuint			  cs_id = 0;
	glw::GLuint			  fs_id = 0;
	glw::GLuint			  gs_id = 0;
	glw::GLuint			  tc_id = 0;
	glw::GLuint			  te_id = 0;
	glw::GLuint			  vs_id = 0;

	/* Sanity checks */
	DE_ASSERT((cs_body != DE_NULL && (fs_body == DE_NULL && gs_body == DE_NULL && tc_body == DE_NULL &&
									  te_body == DE_NULL && vs_body == DE_NULL)) ||
			  (cs_body == DE_NULL && (fs_body != DE_NULL || gs_body != DE_NULL || tc_body != DE_NULL ||
									  te_body != DE_NULL || vs_body != DE_NULL)));

	/* Any existing program object already initialzied? Purge it before we continue */
	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram() call failed.");

		m_po_id = 0;
	}

	/* Generate all shader objects we'll need to use for the program */
	if (cs_body != DE_NULL)
	{
		cs_id = gl.createShader(GL_COMPUTE_SHADER);
	}

	if (fs_body != DE_NULL)
	{
		fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	}

	if (gs_body != DE_NULL)
	{
		gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	}

	if (tc_body != DE_NULL)
	{
		tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
	}

	if (te_body != DE_NULL)
	{
		te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	}

	if (vs_body != DE_NULL)
	{
		vs_id = gl.createShader(GL_VERTEX_SHADER);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Create a program object */
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Set source code of the shaders we've created */
	if (cs_id != 0)
	{
		gl.shaderSource(cs_id, 1,			/* count */
						&cs_body, DE_NULL); /* length */
	}

	if (fs_id != 0)
	{
		gl.shaderSource(fs_id, 1,			/* count */
						&fs_body, DE_NULL); /* length */
	}

	if (gs_id != 0)
	{
		gl.shaderSource(gs_id, 1,			/* count */
						&gs_body, DE_NULL); /* length */
	}

	if (tc_id != 0)
	{
		gl.shaderSource(tc_id, 1,			/* count */
						&tc_body, DE_NULL); /* length */
	}

	if (te_id != 0)
	{
		gl.shaderSource(te_id, 1,			/* count */
						&te_body, DE_NULL); /* length */
	}

	if (vs_id != 0)
	{
		gl.shaderSource(vs_id, 1,			/* count */
						&vs_body, DE_NULL); /* length */
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");

	/* Compile the shaders */
	const glw::GLuint  so_ids[] = { cs_id, fs_id, gs_id, tc_id, te_id, vs_id };
	const unsigned int n_so_ids = sizeof(so_ids) / sizeof(so_ids[0]);

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		glw::GLint  compile_status = GL_FALSE;
		glw::GLuint so_id		   = so_ids[n_so_id];

		if (so_id != 0)
		{
			gl.compileShader(so_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

			gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

			if (compile_status == GL_FALSE)
			{
				TCU_FAIL("Shader compilation failed.");
			}

			gl.attachShader(m_po_id, so_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");
		} /* if (so_id != 0) */
	}	 /* for (all shader objects) */

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status == GL_FALSE)
	{
		TCU_FAIL("Program linking failed.");
	}

	/* Release the shader objects - we no longer need them */
	if (cs_id != 0)
	{
		gl.deleteShader(cs_id);
	}

	if (fs_id != 0)
	{
		gl.deleteShader(fs_id);
	}

	if (gs_id != 0)
	{
		gl.deleteShader(gs_id);
	}

	if (tc_id != 0)
	{
		gl.deleteShader(tc_id);
	}

	if (te_id != 0)
	{
		gl.deleteShader(te_id);
	}

	if (vs_id != 0)
	{
		gl.deleteShader(vs_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader() call(s) failed.");
}

/** Deinitializes all GL objects that were created during test execution.
 *  Also calls the inheriting object's deinitObjects() method.
 **/
void PipelineStatisticsQueryTestFunctionalBase::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_qo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_qo_id);

		m_bo_qo_id = 0;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_qo_id != 0)
	{
		gl.deleteQueries(1, &m_qo_id);

		m_qo_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vbo_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_id);

		m_vbo_id = 0;
	}

	deinitObjects();
}

/** Dummy method that should be overloaded by inheriting methods.
 *
 *  The method can be thought as of a placeholder for code that deinitializes
 *  test-specific GL objects.
 **/
void PipelineStatisticsQueryTestFunctionalBase::deinitObjects()
{
	/* Left blank intentionally - this method should be overloaded by deriving
	 * classes.
	 */
}

/** Initializes a framebuffer object that can be used by inheriting tests
 *  for holding rendered data.
 **/
void PipelineStatisticsQueryTestFunctionalBase::initFBO()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up a framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Set up a texture object we will later use as a color attachment for the FBO */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_RGBA8, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Set up the TO as a color attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	gl.viewport(0, 0, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");
}

/** A dummy method, which can be thought of as a placeholder to initialize
 *  test-specific GL objects.
 **/
void PipelineStatisticsQueryTestFunctionalBase::initObjects()
{
	/* Left blank intentionally - this method should be overloaded by deriving
	 * classes.
	 */
}

/** Initializes a vertex array object which is going to be used for the draw calls.
 *  The initialized VAO's ID is going to be stored under m_vao_id. Zeroth vertex
 *  array attribute will be configured to use @param n_components_per_vertex components
 *  and will use vertex buffer object defined by ID stored in m_vbo_id, whose data
 *  are expected to start at an offset defined by m_vbo_vertex_data_offset.
 *
 *  @param n_components_per_vertex As per description.
 */
void PipelineStatisticsQueryTestFunctionalBase::initVAO(unsigned int n_components_per_vertex)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release an VAO that's already been created */
	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Generate a new one */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Set it up */
	gl.vertexAttribPointer(0,											/* index */
						   n_components_per_vertex, GL_FLOAT, GL_FALSE, /* normalized */
						   0,											/* stride */
						   (glw::GLvoid*)(deUintptr)m_vbo_vertex_data_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed.");

	gl.enableVertexAttribArray(0); /* index */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");
}

/** Initializes a vertex buffer object and stores its ID under m_vbo_id.
 *  It is assumed index data is expressed in GL_UNSIGNED_INT.
 *
 *  The following fields will be modified by the method:
 *
 *  m_vbo_n_indices:                          Will hold the number of indices stored in index
 *                                            data buffer.
 *  m_vbo_vertex_data_offset:                 Will hold the offset, from which the vertex
 *                                            data will be stored in VBO.
 *  m_vbo_index_data_offset:                  Will hold the offset, from which the index
 *                                            data will be stored in VBO.
 *  m_vbo_indirect_arrays_argument_offset:    Will hold the offset, from which
 *                                            glDrawArraysIndirect() arguments will be
 *                                            stored in VBO.
 *  m_vbo_indirect_elements_argument_offset:  Will hold the offset, from which
 *                                            glDrawElementsIndirect() arguments will be
 *                                            stored in VBO.
 *  m_indirect_draw_call_firstindex_argument: Will be updated to point to the location, from
 *                                            which index data starts.
 *
 *  @param raw_vertex_data                        Pointer to a buffer that holds vertex data
 *                                                which should be used when constructing the VBO.
 *                                                Must not be NULL.
 *  @param raw_vertex_data_size                   Number of bytes available for reading under
 *                                                @param raw_vertex_data.
 *  @param raw_index_data                         Pointer to a buffer that holds index data
 *                                                which should be used when constructing the VBO.
 *                                                Must not be NULL.
 *  @param raw_index_data_size                    Number of bytes available for reading under
 *                                                @param raw_index_data .
 *  @param indirect_draw_bo_count_argument        Argument to be used for indirect draw calls'
 *                                                "count" argument.
 *  @param indirect_draw_bo_primcount_argument    Argument to be used for indirect draw calls'
 *                                                "primcount" argument.
 *  @param indirect_draw_bo_baseinstance_argument Argument to be used for indirect draw calls'
 *                                                "baseinstance" argument.
 *  @param indirect_draw_bo_first_argument        Argument to be used for indirect draw calls'
 *                                                "first" argument.
 *  @param indirect_draw_bo_basevertex_argument   Argument to be used for indirect draw calls'
 *                                                "basevertex" argument.
 *
 **/
void PipelineStatisticsQueryTestFunctionalBase::initVBO(
	const float* raw_vertex_data, unsigned int raw_vertex_data_size, const unsigned int* raw_index_data,
	unsigned int raw_index_data_size, unsigned int indirect_draw_bo_count_argument,
	unsigned int indirect_draw_bo_primcount_argument, unsigned int indirect_draw_bo_baseinstance_argument,
	unsigned int indirect_draw_bo_first_argument, unsigned int indirect_draw_bo_basevertex_argument)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* If we already have initialized a VBO, delete it before we continue */
	if (m_vbo_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_id);

		m_vbo_id = 0;
	}

	/* Our BO storage is formed as below:
	 *
	 * [raw vertex data]
	 * [raw index data]
	 * [indirect glDrawArrays() call arguments]
	 * [indirect glDrawElements() call arguments]
	 *
	 * We store the relevant offsets in member fields, so that they can be used by actual test
	 * implementation.
	 */
	const unsigned int indirect_arrays_draw_call_arguments_size   = sizeof(unsigned int) * 4; /* as per spec */
	const unsigned int indirect_elements_draw_call_arguments_size = sizeof(unsigned int) * 5; /* as per spec */

	m_vbo_n_indices						  = sizeof(raw_index_data_size) / sizeof(unsigned int);
	m_vbo_vertex_data_offset			  = 0;
	m_vbo_index_data_offset				  = raw_vertex_data_size;
	m_vbo_indirect_arrays_argument_offset = m_vbo_index_data_offset + raw_index_data_size;
	m_vbo_indirect_elements_argument_offset =
		m_vbo_indirect_arrays_argument_offset + indirect_arrays_draw_call_arguments_size;

	/* Set up 'firstindex' argument so that it points at correct index data location */
	DE_ASSERT((m_vbo_index_data_offset % sizeof(unsigned int)) == 0);

	m_indirect_draw_call_firstindex_argument =
		static_cast<unsigned int>(m_vbo_index_data_offset / sizeof(unsigned int));

	/* Form indirect draw call argument buffers */
	unsigned int arrays_draw_call_arguments[] = { indirect_draw_bo_count_argument, indirect_draw_bo_primcount_argument,
												  indirect_draw_bo_first_argument,
												  indirect_draw_bo_baseinstance_argument };
	unsigned int elements_draw_call_arguments[] = {
		indirect_draw_bo_count_argument, indirect_draw_bo_primcount_argument, m_indirect_draw_call_firstindex_argument,
		indirect_draw_bo_basevertex_argument, indirect_draw_bo_baseinstance_argument
	};

	/* Set up BO storage */
	const unsigned int bo_data_size =
		m_vbo_indirect_elements_argument_offset + indirect_elements_draw_call_arguments_size;

	gl.genBuffers(1, &m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_vbo_id);
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call(s) failed.");

	gl.bufferData(GL_ARRAY_BUFFER, bo_data_size, DE_NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	gl.bufferSubData(GL_ARRAY_BUFFER, m_vbo_vertex_data_offset, raw_vertex_data_size, raw_vertex_data);
	gl.bufferSubData(GL_ARRAY_BUFFER, m_vbo_index_data_offset, raw_index_data_size, raw_index_data);
	gl.bufferSubData(GL_ARRAY_BUFFER, m_vbo_indirect_arrays_argument_offset, sizeof(arrays_draw_call_arguments),
					 arrays_draw_call_arguments);
	gl.bufferSubData(GL_ARRAY_BUFFER, m_vbo_indirect_elements_argument_offset, sizeof(elements_draw_call_arguments),
					 elements_draw_call_arguments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");
}

/** Performs the actual test.
 *
 *  @return Always STOP.
 **/
tcu::TestNode::IterateResult PipelineStatisticsQueryTestFunctionalBase::iterate()
{
	bool has_passed = true;
	glu::ContextType contextType = m_context.getRenderContext().getType();

	/* Carry on only if GL_ARB_pipeline_statistics_query extension is supported */
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_pipeline_statistics_query"))
	{
		throw tcu::NotSupportedError("GL_ARB_pipeline_statistics_query extension is not supported");
	}

	/* Initialize QO BO storage if GL_ARB_query_buffer_object is supported */
	if (m_context.getContextInfo().isExtensionSupported("GL_ARB_query_buffer_object"))
	{
		initQOBO();

		DE_ASSERT(m_bo_qo_id != 0);
	}

	/* Initialize other test-specific objects */
	initObjects();

	/* Iterate through all pipeline statistics query object types */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (unsigned int n_query_target = 0; n_query_target < PipelineStatisticsQueryUtilities::n_query_targets;
		 ++n_query_target)
	{
		glw::GLenum current_query_target = PipelineStatisticsQueryUtilities::query_targets[n_query_target];

		if (shouldExecuteForQueryTarget(current_query_target))
		{
			/* Initialize the query object */
			gl.genQueries(1, &m_qo_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries() call failed.");

			/* Execute the test for the particular query target. */
			has_passed &= executeTest(current_query_target);

			/* Delete the query object */
			gl.deleteQueries(1, &m_qo_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteQueries() call failed.");

			m_qo_id = 0;
		}
	} /* for (all query targets) */

	if (has_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Initializes a query buffer object. */
void PipelineStatisticsQueryTestFunctionalBase::initQOBO()
{
	const glw::Functions gl = m_context.getRenderContext().getFunctions();

	/* Set up the buffer object we will use for storage of query object results */
	unsigned char bo_data[PipelineStatisticsQueryUtilities::qo_bo_size];

	memset(bo_data, 0xFF, sizeof(bo_data));

	gl.genBuffers(1, &m_bo_qo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_qo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, PipelineStatisticsQueryUtilities::qo_bo_size, bo_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");
}

/** Executes a draw call, whose type is specified under pThis->m_current_draw_call_type.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctionalBase instance, which
 *               should be used to extract the draw call type.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctionalBase::queryCallbackDrawCallHandler(void* pThis)
{
	PipelineStatisticsQueryTestFunctionalBase* pInstance = (PipelineStatisticsQueryTestFunctionalBase*)pThis;
	const glw::Functions&					   gl		 = pInstance->m_context.getRenderContext().getFunctions();

	/* Issue the draw call */
	glw::GLenum primitive_type =
		PipelineStatisticsQueryUtilities::getEnumForPrimitiveType(pInstance->m_current_primitive_type);

	switch (pInstance->m_current_draw_call_type)
	{
	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWARRAYS:
	{
		gl.drawArrays(primitive_type, pInstance->m_indirect_draw_call_first_argument,
					  pInstance->m_indirect_draw_call_count_argument);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWARRAYSINDIRECT:
	{
		gl.drawArraysIndirect(primitive_type,
							  (const glw::GLvoid*)(deUintptr)pInstance->m_vbo_indirect_arrays_argument_offset);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArraysIndirect() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCED:
	{
		gl.drawArraysInstanced(primitive_type, pInstance->m_indirect_draw_call_first_argument,
							   pInstance->m_indirect_draw_call_count_argument,
							   pInstance->m_indirect_draw_call_primcount_argument);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArraysInstanced() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWARRAYSINSTANCEDBASEINSTANCE:
	{
		gl.drawArraysInstancedBaseInstance(primitive_type, pInstance->m_indirect_draw_call_first_argument,
										   pInstance->m_indirect_draw_call_count_argument,
										   pInstance->m_indirect_draw_call_primcount_argument,
										   pInstance->m_indirect_draw_call_baseinstance_argument);

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTS:
	{
		gl.drawElements(primitive_type, pInstance->m_indirect_draw_call_count_argument, GL_UNSIGNED_INT,
						(glw::GLvoid*)(deUintptr)pInstance->m_vbo_index_data_offset);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElements() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSBASEVERTEX:
	{
		gl.drawElementsBaseVertex(primitive_type, pInstance->m_indirect_draw_call_count_argument, GL_UNSIGNED_INT,
								  (glw::GLvoid*)(deUintptr)pInstance->m_vbo_index_data_offset,
								  pInstance->m_indirect_draw_call_basevertex_argument);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsBaseVertex() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSINDIRECT:
	{
		gl.drawElementsIndirect(primitive_type, GL_UNSIGNED_INT,
								(glw::GLvoid*)(deUintptr)pInstance->m_vbo_indirect_elements_argument_offset);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsIndirect() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCED:
	{
		gl.drawElementsInstanced(primitive_type, pInstance->m_indirect_draw_call_count_argument, GL_UNSIGNED_INT,
								 (glw::GLvoid*)(deUintptr)pInstance->m_vbo_index_data_offset,
								 pInstance->m_indirect_draw_call_primcount_argument);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsInstanced() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEINSTANCE:
	{
		gl.drawElementsInstancedBaseInstance(
			primitive_type, pInstance->m_indirect_draw_call_count_argument, GL_UNSIGNED_INT,
			(glw::GLvoid*)(deUintptr)pInstance->m_vbo_index_data_offset,
			pInstance->m_indirect_draw_call_primcount_argument, pInstance->m_indirect_draw_call_baseinstance_argument);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsInstancedBaseInstance() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCE:
	{
		gl.drawElementsInstancedBaseVertexBaseInstance(
			primitive_type, pInstance->m_indirect_draw_call_count_argument, GL_UNSIGNED_INT,
			(glw::GLvoid*)(deUintptr)pInstance->m_vbo_index_data_offset,
			pInstance->m_indirect_draw_call_primcount_argument, pInstance->m_indirect_draw_call_basevertex_argument,
			pInstance->m_indirect_draw_call_baseinstance_argument);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsInstancedBaseVertexBaseInstance() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWRANGEELEMENTS:
	{
		gl.drawRangeElements(primitive_type, 0, /* start */
							 pInstance->m_vbo_n_indices, pInstance->m_indirect_draw_call_count_argument,
							 GL_UNSIGNED_INT, (glw::GLvoid*)(deUintptr)pInstance->m_vbo_index_data_offset);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawRangeElements() call failed.");

		break;
	}

	case PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWRANGEELEMENTSBASEVERTEX:
	{
		gl.drawRangeElementsBaseVertex(primitive_type, 0,								   /* start */
									   pInstance->m_indirect_draw_call_count_argument - 1, /* end */
									   pInstance->m_indirect_draw_call_count_argument, GL_UNSIGNED_INT,
									   (glw::GLvoid*)(deUintptr)pInstance->m_vbo_index_data_offset,
									   pInstance->m_indirect_draw_call_basevertex_argument);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawRangeElementsBaseVertex() call failed.");

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized draw call type");
	}
	} /* switch (m_current_draw_call_type) */

	return true;
}

/** Tells whether the test instance should be executed for user-specified query target.
 *  Base class implementation returns true for all values of @param query_target.
 *
 *  @param query_target Query target to be used for the call.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctionalBase::shouldExecuteForQueryTarget(glw::GLenum query_target)
{
	(void)query_target;
	return true;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
PipelineStatisticsQueryTestFunctional1::PipelineStatisticsQueryTestFunctional1(deqp::Context& context)
	: PipelineStatisticsQueryTestFunctionalBase(context, "functional_default_qo_values",
												"Verifies that all pipeline statistics query objects "
												"use a default value of 0.")
{
	/* Left blank intentionally */
}

/** Executes a test iteration for user-specified query target.
 *
 *  @param current_query_target Pipeline statistics query target to execute the iteration
 *                              for.
 *
 *  @return true if the test passed for the iteration, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional1::executeTest(glw::GLenum current_query_target)
{
	bool													 result = true;
	PipelineStatisticsQueryUtilities::_test_execution_result run_result;

	if (!PipelineStatisticsQueryUtilities::executeQuery(
			current_query_target, m_qo_id, m_bo_qo_id, DE_NULL, /* pfn_draw */
			DE_NULL,											/* draw_user_arg */
			m_context.getRenderContext(), m_testCtx, m_context.getContextInfo(), &run_result))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Could not retrieve test run results for query target "
													   "["
						   << PipelineStatisticsQueryUtilities::getStringForEnum(current_query_target) << "]"
						   << tcu::TestLog::EndMessage;

		result = false;
	}
	else
	{
		const glw::GLuint64 expected_value = 0;

		result &= PipelineStatisticsQueryUtilities::verifyResultValues(
			run_result, 1, &expected_value, m_qo_id != 0, /* should_check_qo_bo_values */
			current_query_target, DE_NULL, DE_NULL,
			false, /* is_primitive_restart_enabled */
			m_testCtx, PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EXACT_MATCH);
	} /* if (run results were obtained successfully) */

	return result;
}

/** Constructor.
 *
 *  @param context Rendering context
 */
PipelineStatisticsQueryTestFunctional2::PipelineStatisticsQueryTestFunctional2(deqp::Context& context)
	: PipelineStatisticsQueryTestFunctionalBase(context, "functional_non_rendering_commands_do_not_affect_queries",
												"Verifies that non-rendering commands do not affect query"
												" values.")
	, m_bo_id(0)
	, m_fbo_draw_id(0)
	, m_fbo_read_id(0)
	, m_to_draw_fbo_id(0)
	, m_to_read_fbo_id(0)
	, m_to_height(16)
	, m_to_width(16)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestFunctional2::deinitObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
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

	if (m_to_draw_fbo_id != 0)
	{
		gl.deleteTextures(1, &m_to_draw_fbo_id);

		m_to_draw_fbo_id = 0;
	}

	if (m_to_read_fbo_id != 0)
	{
		gl.deleteTextures(1, &m_to_read_fbo_id);

		m_to_read_fbo_id = 0;
	}
}

/** Callback handler which calls glBlitFramebuffer() API function and makes sure it
 *  was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeBlitFramebufferTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl		 = data_ptr->m_context.getRenderContext().getFunctions();

	/* Framebuffer objects are bound to their FB targets at this point */
	gl.blitFramebuffer(0,						   /* srcX0 */
					   0,						   /* srcY0 */
					   data_ptr->m_to_width,	   /* srcX1 */
					   data_ptr->m_to_height,	  /* srcY1 */
					   0,						   /* dstX0 */
					   0,						   /* dstY0 */
					   data_ptr->m_to_width << 1,  /* dstX1 */
					   data_ptr->m_to_height << 1, /* dstY1 */
					   GL_COLOR_BUFFER_BIT,		   /* mask */
					   GL_LINEAR);				   /* filter */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBlitFramebuffer() call failed.");

	return true;
}

/** Callback handler which calls glBufferSubData() API function and makes sure it
 *  was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeBufferSubDataTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr	   = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl			   = data_ptr->m_context.getRenderContext().getFunctions();
	const unsigned int						test_data_size = (PipelineStatisticsQueryTestFunctional2::bo_size / 2);
	unsigned char							test_bo_data[test_data_size];

	memset(test_bo_data, 0xFF, test_data_size);

	gl.bindBuffer(GL_ARRAY_BUFFER, data_ptr->m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferSubData(GL_ARRAY_BUFFER, 0, /* offset */
					 test_data_size, test_bo_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");

	return true;
}

/** Callback handler which calls glClearBufferfv() API function and makes sure it
 *  was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeClearBufferfvColorBufferTest(void* pThis)
{
	const glw::GLfloat						clear_color[4] = { 0, 0.1f, 0.2f, 0.3f };
	PipelineStatisticsQueryTestFunctional2* data_ptr	   = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl			   = data_ptr->m_context.getRenderContext().getFunctions();

	gl.clearBufferfv(GL_COLOR, 0, /* drawbuffer */
					 clear_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferfv() call failed.");

	return true;
}

/** Callback handler which calls glClearBufferfv() API function and makes sure it
 *  was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeClearBufferfvDepthBufferTest(void* pThis)
{
	const glw::GLfloat						clear_depth = 0.1f;
	PipelineStatisticsQueryTestFunctional2* data_ptr	= (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl			= data_ptr->m_context.getRenderContext().getFunctions();

	gl.clearBufferfv(GL_DEPTH, 0, /* drawbuffer */
					 &clear_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferfv() call failed.");

	return true;
}

/** Callback handler which calls glClearBufferiv() API function and makes sure it
 *  was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeClearBufferivStencilBufferTest(void* pThis)
{
	const glw::GLint						clear_stencil = 123;
	PipelineStatisticsQueryTestFunctional2* data_ptr	  = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl			  = data_ptr->m_context.getRenderContext().getFunctions();

	gl.clearBufferiv(GL_STENCIL, 0, /* drawbuffer */
					 &clear_stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferfv() call failed.");

	return true;
}

/** Callback handler which calls glClearBufferSubData() API function and makes sure it
 *  was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return true if glClearBufferSubData() is available, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeClearBufferSubDataTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl		 = data_ptr->m_context.getRenderContext().getFunctions();
	bool									result   = true;

	if (!glu::contextSupports(data_ptr->m_context.getRenderContext().getType(), glu::ApiType::core(4, 3)) &&
		gl.clearBufferSubData == NULL)
	{
		/* API is unavailable */
		return false;
	}

	/* Execute the API call */
	const unsigned char value = 0xFF;

	gl.clearBufferSubData(GL_ARRAY_BUFFER, GL_R8, 0, /* offset */
						  data_ptr->bo_size, GL_RED, GL_UNSIGNED_BYTE, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferSubData() call failed.");

	/* All done */
	return result;
}

/** Callback handler which calls glClear() API function configured to clear color
 *  buffer and makes sure it was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeClearColorBufferTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl		 = data_ptr->m_context.getRenderContext().getFunctions();

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

	return true;
}

/** Callback handler which calls glClear() API function configured to clear depth
 *  buffer and makes sure it was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeClearDepthBufferTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl		 = data_ptr->m_context.getRenderContext().getFunctions();

	gl.clear(GL_DEPTH_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

	return true;
}

/** Callback handler which calls glClear() API function configured to clear stencil
 *  buffer and makes sure it was executed successfully.
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeClearStencilBufferTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl		 = data_ptr->m_context.getRenderContext().getFunctions();

	gl.clear(GL_STENCIL_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

	return true;
}

/** Callback handler which calls glClearTexSubImage() API function (if available).
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return true if the function is supported by the running GL implementation, false
 *               otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeClearTexSubImageTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl		 = data_ptr->m_context.getRenderContext().getFunctions();
	bool									result   = true;

	if (!glu::contextSupports(data_ptr->m_context.getRenderContext().getType(), glu::ApiType::core(4, 4)) &&
		gl.clearTexSubImage == NULL)
	{
		/* API is unavailable */
		return false;
	}

	/* Execute the API call */
	const unsigned char test_value = 0xFF;

	gl.clearTexSubImage(data_ptr->m_to_draw_fbo_id, 0,							/* level */
						0,														/* xoffset */
						0,														/* yoffset */
						0,														/* zoffset */
						data_ptr->m_to_width / 2, data_ptr->m_to_height / 2, 1, /* depth */
						GL_RED, GL_UNSIGNED_BYTE, &test_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearTexSubImage() call failed.");

	/* All done */
	return result;
}

/** Callback handler which calls glCopyImageSubData() API function (if available).
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return true if the function is supported by the running GL implementation, false
 *               otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeCopyImageSubDataTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl		 = data_ptr->m_context.getRenderContext().getFunctions();
	bool									result   = true;

	if (!glu::contextSupports(data_ptr->m_context.getRenderContext().getType(), glu::ApiType::core(4, 3)) &&
		gl.copyImageSubData == NULL)
	{
		/* API is unavailable */
		return false;
	}

	/* Execute the API call */
	gl.copyImageSubData(data_ptr->m_to_draw_fbo_id, GL_TEXTURE_2D, 0,			 /* srcLevel */
						0,														 /* srcX */
						0,														 /* srcY */
						0,														 /* srcZ */
						data_ptr->m_to_read_fbo_id, GL_TEXTURE_2D, 0,			 /* dstLevel */
						0,														 /* dstX */
						0,														 /* dstY */
						0,														 /* dstZ */
						data_ptr->m_to_width / 2, data_ptr->m_to_height / 2, 1); /* src_depth */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCopyImageSubData() call failed.");

	/* All done */
	return result;
}

/** Callback handler which calls glTexSubImage2D().
 *
 *  @param pThis Pointer to a PipelineStatisticsQueryTestFunctional2 instance. Must not
 *               be NULL.
 *
 *  @return true Always true.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeTexSubImageTest(void* pThis)
{
	PipelineStatisticsQueryTestFunctional2* data_ptr	   = (PipelineStatisticsQueryTestFunctional2*)pThis;
	const glw::Functions&					gl			   = data_ptr->m_context.getRenderContext().getFunctions();
	const unsigned int						test_data_size = PipelineStatisticsQueryTestFunctional2::bo_size / 2;
	unsigned char							test_data[test_data_size];

	memset(test_data, 0xFF, test_data_size);

	gl.texSubImage2D(GL_TEXTURE_2D, 0, /* level */
					 0,				   /* xoffset */
					 0,				   /* yoffset */
					 data_ptr->m_to_width / 2, data_ptr->m_to_height / 2, GL_RED, GL_UNSIGNED_BYTE, test_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed.");

	return true;
}

/** Executes a test iteration for user-specified query target.
 *
 *  @param current_query_target Pipeline statistics query target to execute the iteration
 *                              for.
 *
 *  @return true if the test passed for the iteration, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional2::executeTest(glw::GLenum current_query_target)
{
	bool															result = true;
	PipelineStatisticsQueryUtilities::_test_execution_result		run_result;
	const PipelineStatisticsQueryUtilities::PFNQUERYDRAWHANDLERPROC query_draw_handlers[] = {
		executeBlitFramebufferTest,
		executeBufferSubDataTest,
		executeClearBufferfvColorBufferTest,
		executeClearBufferfvDepthBufferTest,
		executeClearBufferivStencilBufferTest,
		executeClearBufferSubDataTest,
		executeClearColorBufferTest,
		executeClearDepthBufferTest,
		executeClearStencilBufferTest,
		executeClearTexSubImageTest,
		executeCopyImageSubDataTest,
		executeTexSubImageTest,
	};
	const unsigned int n_query_draw_handlers = sizeof(query_draw_handlers) / sizeof(query_draw_handlers[0]);

	for (unsigned int n = 0; n < n_query_draw_handlers; ++n)
	{
		const PipelineStatisticsQueryUtilities::PFNQUERYDRAWHANDLERPROC& draw_handler_pfn = query_draw_handlers[n];

		/* Query executors can return false if a given test cannot be executed, given
		 * work environment constraint (eg. insufficient GL version). In case of an error,
		 * they will throw an exception.
		 */
		if (draw_handler_pfn(this))
		{
			if (!PipelineStatisticsQueryUtilities::executeQuery(
					current_query_target, m_qo_id, m_bo_qo_id, DE_NULL, /* pfn_draw */
					DE_NULL,											/* draw_user_arg */
					m_context.getRenderContext(), m_testCtx, m_context.getContextInfo(), &run_result))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Query execution failed for query target "
															   "["
								   << PipelineStatisticsQueryUtilities::getStringForEnum(current_query_target) << "]"
								   << tcu::TestLog::EndMessage;

				result = false;
			}
			else
			{
				const glw::GLuint64 expected_value = 0;
				bool				has_passed	 = true;

				has_passed = PipelineStatisticsQueryUtilities::verifyResultValues(
					run_result, 1, &expected_value, m_qo_id != 0,  /* should_check_qo_bo_values */
					current_query_target, DE_NULL, DE_NULL, false, /* is_primitive_restart_enabled */
					m_testCtx, PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EXACT_MATCH);

				if (!has_passed)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Test failed for iteration index [" << n << "]."
									   << tcu::TestLog::EndMessage;

					result = false;
				}
			} /* if (run results were obtained successfully) */
		}	 /* if (draw handler executed successfully) */
	}

	return result;
}

/* Initializes all GL objects used by the test */
void PipelineStatisticsQueryTestFunctional2::initObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up a buffer object we will use for one of the tests */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call(s) failed.");

	gl.bufferData(GL_ARRAY_BUFFER, bo_size, DE_NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Set up texture objects we will  use as color attachments for test FBOs */
	gl.genTextures(1, &m_to_draw_fbo_id);
	gl.genTextures(1, &m_to_read_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to_draw_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_RGBA8, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_read_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_RGBA8, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Set up framebuffer objects */
	gl.genFramebuffers(1, &m_fbo_draw_id);
	gl.genFramebuffers(1, &m_fbo_read_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call(s) failed.");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_draw_id);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call(s) failed.");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_draw_fbo_id, 0); /* level */
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_read_fbo_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call(s) failed.");
}

/** Constructor.
 *
 *  @param context Rendering context
 */
PipelineStatisticsQueryTestFunctional3::PipelineStatisticsQueryTestFunctional3(deqp::Context& context)
	: PipelineStatisticsQueryTestFunctionalBase(
		  context, "functional_primitives_vertices_submitted_and_clipping_input_output_primitives",
		  "Verifies that GL_PRIMITIVES_SUBMITTED_ARB, GL_VERTICES_SUBMITTED_ARB, "
		  "GL_CLIPPING_INPUT_PRIMITIVES_ARB, and GL_CLIPPING_OUTPUT_PRIMITIVES_ARB "
		  "queries work correctly.")
	, m_is_primitive_restart_enabled(false)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestFunctional3::deinitObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	/* Disable "primitive restart" functionality */
	gl.disable(GL_PRIMITIVE_RESTART);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable() call failed.");
}

/** Executes a test iteration for user-specified query target.
 *
 *  @param current_query_target Pipeline statistics query target to execute the iteration
 *                              for.
 *
 *  @return true if the test passed for the iteration, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional3::executeTest(glw::GLenum current_query_target)
{
	const glw::Functions&									 gl		= m_context.getRenderContext().getFunctions();
	bool													 result = true;
	PipelineStatisticsQueryUtilities::_test_execution_result run_result;

	/* Sanity check: This method should only be called for GL_VERTICES_SUBMITTED_ARB,
	 * GL_PRIMITIVES_SUBMITTED_ARB, GL_CLIPPING_INPUT_PRIMITIVES_ARB and
	 * GL_CLIPPING_OUTPUT_PRIMITIVES_ARB queries */
	DE_ASSERT(current_query_target == GL_VERTICES_SUBMITTED_ARB ||
			  current_query_target == GL_PRIMITIVES_SUBMITTED_ARB ||
			  current_query_target == GL_CLIPPING_INPUT_PRIMITIVES_ARB ||
			  current_query_target == GL_CLIPPING_OUTPUT_PRIMITIVES_ARB);

	/* Set up VBO. We don't really care much about the visual outcome,
	 * so any data will do.
	 */
	const unsigned int n_vertex_components = 2;
	const float		   vertex_data[]	   = { -0.1f, 0.2f, 0.3f,  0.1f,  0.2f,  -0.7f, 0.5f,  -0.5f,
								  0.0f,  0.0f, -0.6f, -0.9f, -0.3f, 0.3f,  -0.5f, -0.5f };
	const unsigned int index_data[] = {
		0, 6, 2, 1, 3, 5, 4,
	};
	const unsigned int n_indices = sizeof(index_data) / sizeof(index_data[0]);

	m_indirect_draw_call_baseinstance_argument = 1;
	m_indirect_draw_call_basevertex_argument   = 0;
	m_indirect_draw_call_count_argument		   = n_indices;
	m_indirect_draw_call_first_argument		   = 0;
	m_indirect_draw_call_primcount_argument	= 3;

	initVBO(vertex_data, sizeof(vertex_data), index_data, sizeof(index_data), m_indirect_draw_call_count_argument,
			m_indirect_draw_call_primcount_argument, m_indirect_draw_call_baseinstance_argument,
			m_indirect_draw_call_first_argument, m_indirect_draw_call_basevertex_argument);

	initVAO(n_vertex_components);

	/* Verify that the query works correctly both when primitive restart functionality
	 * is disabled and enabled */
	const bool		   pr_statuses[] = { false, true };
	const unsigned int n_pr_statuses = sizeof(pr_statuses) / sizeof(pr_statuses[0]);

	for (unsigned int n_pr_status = 0; n_pr_status < n_pr_statuses; ++n_pr_status)
	{
		m_is_primitive_restart_enabled = pr_statuses[n_pr_status];

		/* Primitive restart should never be enabled for GL_CLIPPING_INPUT_PRIMITIVES_ARB query. */
		if ((current_query_target == GL_CLIPPING_INPUT_PRIMITIVES_ARB ||
			 current_query_target == GL_CLIPPING_OUTPUT_PRIMITIVES_ARB) &&
			m_is_primitive_restart_enabled)
		{
			continue;
		}

		/* Configure 'primitive restart' functionality */
		if (!m_is_primitive_restart_enabled)
		{
			gl.disable(GL_PRIMITIVE_RESTART);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable() call failed.");
		}
		else
		{
			gl.primitiveRestartIndex(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glPrimitiveRestartIndex() call failed.");

			gl.enable(GL_PRIMITIVE_RESTART);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable() call failed.");
		}

		/* Iterate through all primitive types */
		for (unsigned int n_primitive_type = 0;
			 n_primitive_type < PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_COUNT; ++n_primitive_type)
		{
			m_current_primitive_type = (PipelineStatisticsQueryUtilities::_primitive_type)n_primitive_type;

			/* Exclude patches from the test */
			if (m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_PATCHES)
			{
				continue;
			}

			/* Iterate through all draw call types while the query is enabled (skip DrawArrays calls if primitive restart is enabled) */
			for (unsigned int n_draw_call_type =
					 (m_is_primitive_restart_enabled ? PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_GLDRAWELEMENTS :
													   0);
				 n_draw_call_type < PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_COUNT; ++n_draw_call_type)
			{
				m_current_draw_call_type = (PipelineStatisticsQueryUtilities::_draw_call_type)n_draw_call_type;

				/* Only continue if the draw call is supported by the context */
				if (!PipelineStatisticsQueryUtilities::isDrawCallSupported(m_current_draw_call_type, gl))
				{
					continue;
				}

				if (!PipelineStatisticsQueryUtilities::executeQuery(
						current_query_target, m_qo_id, m_bo_qo_id, queryCallbackDrawCallHandler,
						(PipelineStatisticsQueryTestFunctionalBase*)this, m_context.getRenderContext(), m_testCtx,
						m_context.getContextInfo(), &run_result))
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Could not retrieve test run results for query target "
										  "["
									   << PipelineStatisticsQueryUtilities::getStringForEnum(current_query_target)
									   << "]" << tcu::TestLog::EndMessage;

					result = false;
				}
				else
				{
					glw::GLuint64										 expected_values[4] = { 0 };
					unsigned int										 n_expected_values  = 0;
					PipelineStatisticsQueryUtilities::_verification_type verification_type =
						PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EXACT_MATCH;

					if (current_query_target == GL_CLIPPING_OUTPUT_PRIMITIVES_ARB)
					{
						verification_type = PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EQUAL_OR_GREATER;
					}

					if (current_query_target == GL_VERTICES_SUBMITTED_ARB)
					{
						getExpectedVerticesSubmittedQueryResult(m_current_primitive_type, &n_expected_values,
																expected_values);
					}
					else
					{
						getExpectedPrimitivesSubmittedQueryResult(m_current_primitive_type, &n_expected_values,
																  expected_values);
					}

					result &= PipelineStatisticsQueryUtilities::verifyResultValues(
						run_result, n_expected_values, expected_values, m_qo_id != 0, /* should_check_qo_bo_values */
						current_query_target, &m_current_draw_call_type, &m_current_primitive_type,
						m_is_primitive_restart_enabled, m_testCtx, verification_type);

				} /* if (run results were obtained successfully) */
			}	 /* for (all draw call types) */
		}		  /* for (all primitive types) */
	}			  /* for (both when primitive restart is disabled and enabled) */

	return result;
}

/** Returns the expected result value(s) for a GL_PRIMITIVES_SUBMITTED_ARB query. There
 *  can be either one or two result values, depending on how the implementation handles
 *  incomplete primitives.
 *
 *  @param current_primitive_type Primitive type used for the draw call, for which
 *                                the query would be executed
 *  @param out_result1_written    Deref will be set to true, if the first result value
 *                                was written to @param out_result1. Otherwise, it will
 *                                be set to false.
 *  @param out_result1            Deref will be set to the first of the acceptable
 *                                result values, if @param out_result1_written was set
 *                                to true.
 *  @param out_result2_written    Deref will be set to true, if the second result value
 *                                was written to @param out_result2. Otherwise, it will
 *                                be set to false.
 *  @param out_result2            Deref will be set to the second of the acceptable
 *                                result values, if @param out_result2_written was set
 *                                to true.
 *
 **/
void PipelineStatisticsQueryTestFunctional3::getExpectedPrimitivesSubmittedQueryResult(
	PipelineStatisticsQueryUtilities::_primitive_type current_primitive_type, unsigned int* out_results_written,
	glw::GLuint64 out_results[4])
{
	unsigned int n_input_vertices = m_indirect_draw_call_count_argument;

	*out_results_written = 0;

	/* Sanity checks */
	DE_ASSERT(current_primitive_type != PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_PATCHES);

	/* Carry on */
	if (m_is_primitive_restart_enabled)
	{
		/* Primitive restart functionality in our test removes a single index.
		 *
		 * Note: This also applies to arrayed draw calls, since we're testing
		 *       GL_PRIMITIVE_RESTART rendering mode, and we're using a primitive
		 *       restart index of 0.
		 **/
		n_input_vertices--;
	}

	switch (current_primitive_type)
	{
	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_POINTS:
	{
		out_results[(*out_results_written)++] = n_input_vertices;

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINE_LOOP:
	{
		if (n_input_vertices > 2)
		{
			out_results[(*out_results_written)++] = n_input_vertices;
		}
		else if (n_input_vertices > 1)
		{
			out_results[(*out_results_written)++] = 1;
		}
		else
		{
			out_results[(*out_results_written)++] = 0;
		}

		break;
	} /* PRIMITIVE_TYPE_LINE_LOOP */

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLE_FAN:
	{
		if (n_input_vertices > 2)
		{
			out_results[(*out_results_written)++] = n_input_vertices - 2;
		}
		else
		{
			out_results[(*out_results_written)++] = 0;

			if (n_input_vertices >= 1)
			{
				/* If the submitted triangle fan is incomplete, also include the case
				 * where the incomplete triangle fan's vertices are counted as a primitive.
				 */
				out_results[(*out_results_written)++] = 1;
			}
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINE_STRIP:
	{
		if (n_input_vertices > 1)
		{
			out_results[(*out_results_written)++] = n_input_vertices - 1;
		}
		else
		{
			out_results[(*out_results_written)++] = 0;

			if (n_input_vertices > 0)
			{
				/* If the submitted line strip is incomplete, also include the case
				 * where the incomplete line's vertices are counted as a primitive.
				 */
				out_results[(*out_results_written)++] = 1;
			}
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLE_STRIP:
	{
		if (n_input_vertices > 2)
		{
			out_results[(*out_results_written)++] = n_input_vertices - 2;
		}
		else
		{
			out_results[(*out_results_written)++] = 0;

			if (n_input_vertices >= 1)
			{
				/* If the submitted triangle strip is incomplete, also include the case
				 * where the incomplete triangle's vertices are counted as a primitive.
				 */
				out_results[(*out_results_written)++] = 1;
			}
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINES:
	{
		out_results[(*out_results_written)++] = n_input_vertices / 2;

		/* If the submitted line is incomplete, also include the case where
		 * the incomplete line's vertices are counted as a primitive.
		 */
		if (n_input_vertices > 0 && (n_input_vertices % 2) != 0)
		{
			out_results[(*out_results_written)++] = n_input_vertices / 2 + 1;
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINES_ADJACENCY:
	{
		out_results[(*out_results_written)++] = n_input_vertices / 4;

		/* If the submitted line is incomplete, also include the case where
		 * the incomplete line's vertices are counted as a primitive.
		 */
		if (n_input_vertices > 0 && (n_input_vertices % 4) != 0)
		{
			out_results[(*out_results_written)++] = n_input_vertices / 4 + 1;
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLES:
	{
		out_results[(*out_results_written)++] = n_input_vertices / 3;

		/* If the submitted triangle is incomplete, also include the case
		 * when the incomplete triangle's vertices are counted as a primitive.
		 */
		if (n_input_vertices > 0 && (n_input_vertices % 3) != 0)
		{
			out_results[(*out_results_written)++] = n_input_vertices / 3 + 1;
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLES_ADJACENCY:
	{
		out_results[(*out_results_written)++] = n_input_vertices / 6;

		/* If the submitted triangle is incomplete, also include the case
		 * when the incomplete triangle's vertices are counted as a primitive.
		 */
		if (n_input_vertices > 0 && (n_input_vertices % 6) != 0)
		{
			out_results[(*out_results_written)++] = n_input_vertices / 6 + 1;
		}

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized primitive type");
	}
	} /* switch (current_primitive_type) */

	if (PipelineStatisticsQueryUtilities::isInstancedDrawCall(m_current_draw_call_type))
	{
		for (unsigned int i = 0; i < *out_results_written; ++i)
		{
			out_results[i] *= m_indirect_draw_call_primcount_argument;
		}
	} /* if (instanced draw call type) */
}

/** Returns the expected result value(s) for a GL_VERTICES_SUBMITTED_ARB query. There
 *  can be either one or two result values, depending on how the implementation handles
 *  incomplete primitives.
 *
 *  @param current_primitive_type Primitive type used for the draw call, for which
 *                                the query would be executed
 *  @param out_result1_written    Deref will be set to true, if the first result value
 *                                was written to @param out_result1. Otherwise, it will
 *                                be set to false.
 *  @param out_result1            Deref will be set to the first of the acceptable
 *                                result values, if @param out_result1_written was set
 *                                to true.
 *  @param out_result2_written    Deref will be set to true, if the second result value
 *                                was written to @param out_result2. Otherwise, it will
 *                                be set to false.
 *  @param out_result2            Deref will be set to the second of the acceptable
 *                                result values, if @param out_result2_written was set
 *                                to true.
 *
 **/
void PipelineStatisticsQueryTestFunctional3::getExpectedVerticesSubmittedQueryResult(
	PipelineStatisticsQueryUtilities::_primitive_type current_primitive_type, unsigned int* out_results_written,
	glw::GLuint64 out_results[4])
{
	unsigned int n_input_vertices = m_indirect_draw_call_count_argument;

	*out_results_written = 0;

	/* Sanity checks */
	DE_ASSERT(current_primitive_type != PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_PATCHES);

	/* Carry on */
	if (m_is_primitive_restart_enabled)
	{
		/* Primitive restart functionality in our test removes a single index.
		 *
		 * Note: This also applies to arrayed draw calls, since we're testing
		 *       GL_PRIMITIVE_RESTART rendering mode, and we're using a primitive
		 *       restart index of 0.
		 **/
		n_input_vertices--;
	}

	switch (current_primitive_type)
	{
	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_POINTS:
	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINE_STRIP:
	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLE_FAN:
	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLE_STRIP:
	{
		out_results[(*out_results_written)++] = n_input_vertices;

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINE_LOOP:
	{
		out_results[(*out_results_written)++] = n_input_vertices;

		/* Allow line loops to count the first vertex twice as that vertex
		 * is part of both the first and the last primitives.
		 */
		out_results[(*out_results_written)++] = n_input_vertices + 1;
		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINES:
	{
		out_results[(*out_results_written)++] = n_input_vertices;

		/* If the submitted line is incomplete, also include the case where
		 * the incomplete line's vertices are not counted.
		 */
		if (n_input_vertices > 0 && (n_input_vertices % 2) != 0)
		{
			out_results[(*out_results_written)++] = n_input_vertices - 1;
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINES_ADJACENCY:
	{
		/* Allow implementations to both include or exclude the adjacency
		 * vertices.
		 */
		out_results[(*out_results_written)++] = n_input_vertices;
		out_results[(*out_results_written)++] = n_input_vertices / 2;

		/* If the submitted line is incomplete, also include the case where
		 * the incomplete line's vertices are not counted.
		 */
		if (n_input_vertices > 0 && (n_input_vertices % 4) != 0)
		{
			out_results[(*out_results_written)++] = n_input_vertices - (n_input_vertices % 4);
			out_results[(*out_results_written)++] = (n_input_vertices - (n_input_vertices % 4)) / 2;
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLES:
	{
		out_results[(*out_results_written)++] = n_input_vertices;

		/* If the submitted triangle is incomplete, also include the case
		 * when the incomplete triangle's vertices are not counted.
		 */
		if (n_input_vertices > 0 && (n_input_vertices % 3) != 0)
		{
			out_results[(*out_results_written)++] = n_input_vertices - (n_input_vertices % 3);
		}

		break;
	}

	case PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLES_ADJACENCY:
	{
		/* Allow implementations to both include or exclude the adjacency
		 * vertices.
		 */
		out_results[(*out_results_written)++] = n_input_vertices;
		out_results[(*out_results_written)++] = n_input_vertices / 2;

		/* If the submitted triangle is incomplete, also include the case
		 * when the incomplete triangle's vertices are not counted.
		 */
		if (n_input_vertices > 0 && (n_input_vertices % 6) != 0)
		{
			out_results[(*out_results_written)++] = n_input_vertices - (n_input_vertices % 6);
			out_results[(*out_results_written)++] = (n_input_vertices - (n_input_vertices % 6)) / 2;
		}

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized primitive type");
	}
	} /* switch (current_primitive_type) */

	if (PipelineStatisticsQueryUtilities::isInstancedDrawCall(m_current_draw_call_type))
	{
		for (unsigned int i = 0; i < *out_results_written; ++i)
		{
			out_results[i] *= m_indirect_draw_call_primcount_argument;
		}
	} /* if (instanced draw call type) */
}

/** Initializes GL objects used by the test */
void PipelineStatisticsQueryTestFunctional3::initObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	buildProgram(DE_NULL,												   /* cs_body */
				 PipelineStatisticsQueryUtilities::dummy_fs_code, DE_NULL, /* gs_body */
				 DE_NULL,												   /* tc_body */
				 DE_NULL,												   /* te_body */
				 PipelineStatisticsQueryUtilities::dummy_vs_code);

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
}

/** Tells whether the test instance should be executed for user-specified query target.
 *
 *  @param query_target Query target to be used for the call.
 *
 *  @return true  if @param query_target is either GL_VERTICES_SUBMITTED_ARB,
 *                GL_PRIMITIVES_SUBMITTED_ARB, GL_CLIPPING_INPUT_PRIMITIVES_ARB, or
 *                GL_CLIPPING_OUTPUT_PRIMITIVES_ARB.
 *          false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional3::shouldExecuteForQueryTarget(glw::GLenum query_target)
{
	return (query_target == GL_VERTICES_SUBMITTED_ARB || query_target == GL_PRIMITIVES_SUBMITTED_ARB ||
			query_target == GL_CLIPPING_INPUT_PRIMITIVES_ARB || query_target == GL_CLIPPING_OUTPUT_PRIMITIVES_ARB);
}

/** Constructor.
 *
 *  @param context Rendering context
 */
PipelineStatisticsQueryTestFunctional4::PipelineStatisticsQueryTestFunctional4(deqp::Context& context)
	: PipelineStatisticsQueryTestFunctionalBase(context, "functional_vertex_shader_invocations",
												"Verifies GL_VERTEX_SHADER_INVOCATIONS_ARB query works correctly")
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestFunctional4::deinitObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** Executes a test iteration for user-specified query target.
 *
 *  @param current_query_target Pipeline statistics query target to execute the iteration
 *                              for.
 *
 *  @return true if the test passed for the iteration, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional4::executeTest(glw::GLenum current_query_target)
{
	const glw::Functions&									 gl		= m_context.getRenderContext().getFunctions();
	bool													 result = true;
	PipelineStatisticsQueryUtilities::_test_execution_result run_result;

	/* Sanity check: This method should only be called for GL_VERTEX_SHADER_INVOCATIONS_ARB
	 * query */
	DE_ASSERT(current_query_target == GL_VERTEX_SHADER_INVOCATIONS_ARB);

	/* Set up VBO. */
	const unsigned int n_vertex_components = 2;
	const float		   vertex_data[]  = { -0.1f, 0.2f, 0.3f, 0.1f, 0.2f, -0.7f, 0.5f, -0.5f, 0.0f, 0.0f, 0.1f, 0.2f };
	const unsigned int index_data[]   = { 4, 3, 2, 1, 0 };
	const unsigned int n_data_indices = sizeof(index_data) / sizeof(index_data[0]);

	/* Issue the test for 1 to 5 indices */
	for (unsigned int n_indices = 1; n_indices < n_data_indices; ++n_indices)
	{
		m_indirect_draw_call_baseinstance_argument = 1;
		m_indirect_draw_call_basevertex_argument   = 1;
		m_indirect_draw_call_count_argument		   = n_indices;
		m_indirect_draw_call_first_argument		   = 0;
		m_indirect_draw_call_primcount_argument	= 4;

		initVBO(vertex_data, sizeof(vertex_data), index_data, sizeof(index_data), m_indirect_draw_call_count_argument,
				m_indirect_draw_call_primcount_argument, m_indirect_draw_call_baseinstance_argument,
				m_indirect_draw_call_first_argument, m_indirect_draw_call_basevertex_argument);

		initVAO(n_vertex_components);

		/* Iterate through all primitive types */
		for (unsigned int n_primitive_type = 0;
			 n_primitive_type < PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_COUNT; ++n_primitive_type)
		{
			m_current_primitive_type = (PipelineStatisticsQueryUtilities::_primitive_type)n_primitive_type;

			/* Exclude patches from the test */
			if (m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_PATCHES)
			{
				continue;
			}

			/* Exclude the primitive types, for which the number of indices is insufficient to form
			 * a primitive.
			 */
			if ((m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINE_LOOP &&
				 n_indices < 2) ||
				(m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINE_STRIP &&
				 n_indices < 2) ||
				(m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINES && n_indices < 2) ||
				(m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLE_FAN &&
				 n_indices < 3) ||
				(m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLE_STRIP &&
				 n_indices < 3) ||
				(m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLES &&
				 n_indices < 3))
			{
				/* Skip the iteration */
				continue;
			}

			/* Exclude adjacency primitive types from the test, since we're not using geometry shader stage. */
			if (m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_LINES_ADJACENCY ||
				m_current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_TRIANGLES_ADJACENCY)
			{
				continue;
			}

			/* Iterate through all draw call types */
			for (unsigned int n_draw_call_type = 0;
				 n_draw_call_type < PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_COUNT; ++n_draw_call_type)
			{
				m_current_draw_call_type = (PipelineStatisticsQueryUtilities::_draw_call_type)n_draw_call_type;

				/* Only continue if the draw call is supported by the context */
				if (!PipelineStatisticsQueryUtilities::isDrawCallSupported(m_current_draw_call_type, gl))
				{
					continue;
				}

				/* Execute the query */
				if (!PipelineStatisticsQueryUtilities::executeQuery(
						current_query_target, m_qo_id, m_bo_qo_id, queryCallbackDrawCallHandler,
						(PipelineStatisticsQueryTestFunctionalBase*)this, m_context.getRenderContext(), m_testCtx,
						m_context.getContextInfo(), &run_result))
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Could not retrieve test run results for query target "
										  "["
									   << PipelineStatisticsQueryUtilities::getStringForEnum(current_query_target)
									   << "]" << tcu::TestLog::EndMessage;

					result = false;
				}
				else
				{
					static const glw::GLuint64 expected_value = 1;

					/* Compare it against query result values */
					result &= PipelineStatisticsQueryUtilities::verifyResultValues(
						run_result, 1, &expected_value, m_qo_id != 0, /* should_check_qo_bo_values */
						current_query_target, &m_current_draw_call_type, &m_current_primitive_type,
						false, /* is_primitive_restart_enabled */
						m_testCtx, PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EQUAL_OR_GREATER);

				} /* if (run results were obtained successfully) */
			}	 /* for (all draw call types) */
		}		  /* for (all primitive types) */
	}			  /* for (1 to 5 indices) */

	return result;
}

/** Initializes all GL objects used by the test */
void PipelineStatisticsQueryTestFunctional4::initObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	buildProgram(DE_NULL, /* cs_body */
				 DE_NULL, /* fs_body */
				 DE_NULL, /* gs_body */
				 DE_NULL, /* tc_body */
				 DE_NULL, /* te_body */
				 PipelineStatisticsQueryUtilities::dummy_vs_code);

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
}

/** Tells whether the test instance should be executed for user-specified query target.
 *
 *  @param query_target Query target to be used for the call.
 *
 *  @return true  if @param query_target is GL_VERTEX_SHADER_INVOCATIONS_ARB.
 *          false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional4::shouldExecuteForQueryTarget(glw::GLenum query_target)
{
	return (query_target == GL_VERTEX_SHADER_INVOCATIONS_ARB);
}

/** Constructor.
 *
 *  @param context Rendering context
 */
PipelineStatisticsQueryTestFunctional5::PipelineStatisticsQueryTestFunctional5(deqp::Context& context)
	: PipelineStatisticsQueryTestFunctionalBase(context, "functional_tess_queries",
												"Verifies that GL_TESS_CONTROL_SHADER_PATCHES_ARB and "
												"GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB queries "
												"work correctly.")
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestFunctional5::deinitObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** Executes a test iteration for user-specified query target.
 *
 *  @param current_query_target Pipeline statistics query target to execute the iteration
 *                              for.
 *
 *  @return true if the test passed for the iteration, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional5::executeTest(glw::GLenum current_query_target)
{
	const glw::Functions&									 gl		= m_context.getRenderContext().getFunctions();
	bool													 result = true;
	PipelineStatisticsQueryUtilities::_test_execution_result run_result;

	/* Sanity check: This method should only be called for GL_TESS_CONTROL_SHADER_PATCHES_ARB and
	 * GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB queries. */
	DE_ASSERT(current_query_target == GL_TESS_CONTROL_SHADER_PATCHES_ARB ||
			  current_query_target == GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB);

	/* Set up VBO. */
	const unsigned int n_vertex_components = 2;
	const float		   vertex_data[]	   = {
		-0.1f, 0.2f, 0.2f, -0.7f, 0.5f, -0.5f,
	};
	const unsigned int index_data[] = { 2, 1, 0 };

	m_indirect_draw_call_baseinstance_argument = 1;
	m_indirect_draw_call_basevertex_argument   = 1;
	m_indirect_draw_call_count_argument		   = 3; /* default GL_PATCH_VERTICES value */
	m_indirect_draw_call_first_argument		   = 0;
	m_indirect_draw_call_primcount_argument	= 4;

	initVBO(vertex_data, sizeof(vertex_data), index_data, sizeof(index_data), m_indirect_draw_call_count_argument,
			m_indirect_draw_call_primcount_argument, m_indirect_draw_call_baseinstance_argument,
			m_indirect_draw_call_first_argument, m_indirect_draw_call_basevertex_argument);

	initVAO(n_vertex_components);

	/* Set up the primitive type */
	m_current_primitive_type = PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_PATCHES;

	/* Iterate through all draw call types */
	for (unsigned int n_draw_call_type = 0; n_draw_call_type < PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_COUNT;
		 ++n_draw_call_type)
	{
		m_current_draw_call_type = (PipelineStatisticsQueryUtilities::_draw_call_type)n_draw_call_type;

		/* Only continue if the draw call is supported by the context */
		if (!PipelineStatisticsQueryUtilities::isDrawCallSupported(m_current_draw_call_type, gl))
		{
			continue;
		}

		/* Execute the query */
		if (!PipelineStatisticsQueryUtilities::executeQuery(
				current_query_target, m_qo_id, m_bo_qo_id, queryCallbackDrawCallHandler,
				(PipelineStatisticsQueryTestFunctionalBase*)this, m_context.getRenderContext(), m_testCtx,
				m_context.getContextInfo(), &run_result))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Could not retrieve test run results for query target "
														   "["
							   << PipelineStatisticsQueryUtilities::getStringForEnum(current_query_target) << "]"
							   << tcu::TestLog::EndMessage;

			result = false;
		}
		else
		{
			static const glw::GLuint64 expected_value = 1; /* as per test spec */

			/* Compare it against query result values */
			result &= PipelineStatisticsQueryUtilities::verifyResultValues(
				run_result, 1, &expected_value, m_qo_id != 0, /* should_check_qo_bo_values */
				current_query_target, &m_current_draw_call_type, &m_current_primitive_type,
				false, /* is_primitive_restart_enabled */
				m_testCtx, PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EQUAL_OR_GREATER);

		} /* if (run results were obtained successfully) */
	}	 /* for (all draw call types) */

	return result;
}

/** Initializes all GL objects used by the test */
void PipelineStatisticsQueryTestFunctional5::initObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* This test should not execute if we're not running at least a GL4.0 context */
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)))
	{
		throw tcu::NotSupportedError("OpenGL 4.0+ is required to run this test.");
	}

	buildProgram(DE_NULL,												   /* cs_body */
				 PipelineStatisticsQueryUtilities::dummy_fs_code, DE_NULL, /* gs_body */
				 PipelineStatisticsQueryUtilities::dummy_tc_code, PipelineStatisticsQueryUtilities::dummy_te_code,
				 PipelineStatisticsQueryUtilities::dummy_vs_code);

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
}

/** Tells whether the test instance should be executed for user-specified query target.
 *
 *  @param query_target Query target to be used for the call.
 *
 *  @return true  if @param query_target is either GL_TESS_CONTROL_SHADER_PATCHES_ARB,
 *                or GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB.
 *          false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional5::shouldExecuteForQueryTarget(glw::GLenum query_target)
{
	return (query_target == GL_TESS_CONTROL_SHADER_PATCHES_ARB ||
			query_target == GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB);
}

/** Constructor.
 *
 *  @param context Rendering context
 */
PipelineStatisticsQueryTestFunctional6::PipelineStatisticsQueryTestFunctional6(deqp::Context& context)
	: PipelineStatisticsQueryTestFunctionalBase(context, "functional_geometry_shader_queries",
												"Verifies that GL_GEOMETRY_SHADER_INVOCATIONS and "
												"GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB queries "
												"work correctly.")
	, m_n_primitives_emitted_by_gs(3)
	, m_n_streams_emitted_by_gs(3)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestFunctional6::deinitObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** Executes a test iteration for user-specified query target.
 *
 *  @param current_query_target Pipeline statistics query target to execute the iteration
 *                              for.
 *
 *  @return true if the test passed for the iteration, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional6::executeTest(glw::GLenum current_query_target)
{
	const glw::Functions&									 gl		= m_context.getRenderContext().getFunctions();
	bool													 result = true;
	PipelineStatisticsQueryUtilities::_test_execution_result run_result;

	/* Sanity check: This method should only be called for GL_GEOMETRY_SHADER_INVOCATIONS and
	 * GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB queries. */
	DE_ASSERT(current_query_target == GL_GEOMETRY_SHADER_INVOCATIONS ||
			  current_query_target == GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB);

	/* Set up VBO. */
	const unsigned int n_vertex_components = 2;
	const float		   vertex_data[]	   = {
		-0.1f, 0.2f, 0.2f, -0.7f, 0.5f, -0.5f, 0.1f, -0.2f, -0.2f, 0.7f, -0.5f, 0.5f,
	};
	const unsigned int index_data[]			   = { 2, 1, 0 };
	m_indirect_draw_call_baseinstance_argument = 1;
	m_indirect_draw_call_basevertex_argument   = 1;
	m_indirect_draw_call_count_argument =
		3; /* note: we will update the argument per iteration, so just use anything for now */
	m_indirect_draw_call_first_argument		= 0;
	m_indirect_draw_call_primcount_argument = 4;

	initVBO(vertex_data, sizeof(vertex_data), index_data, sizeof(index_data), m_indirect_draw_call_count_argument,
			m_indirect_draw_call_primcount_argument, m_indirect_draw_call_baseinstance_argument,
			m_indirect_draw_call_first_argument, m_indirect_draw_call_basevertex_argument);

	initVAO(n_vertex_components);

	/* Iterate over all input primitives supported by geometry shaders */
	for (int gs_input_it = static_cast<int>(PipelineStatisticsQueryUtilities::GEOMETRY_SHADER_INPUT_FIRST);
		 gs_input_it != static_cast<int>(PipelineStatisticsQueryUtilities::GEOMETRY_SHADER_INPUT_COUNT); ++gs_input_it)
	{
		PipelineStatisticsQueryUtilities::_geometry_shader_input gs_input =
			static_cast<PipelineStatisticsQueryUtilities::_geometry_shader_input>(gs_input_it);
		/* Set up the 'count' argument and update the VBO contents */
		m_indirect_draw_call_count_argument = PipelineStatisticsQueryUtilities::getNumberOfVerticesForGSInput(gs_input);

		/* Update the VBO contents */
		gl.bufferSubData(
			GL_ARRAY_BUFFER,
			m_vbo_indirect_arrays_argument_offset, /* the very first argument is 'count' which we need to update */
			sizeof(m_indirect_draw_call_count_argument), &m_indirect_draw_call_count_argument);
		gl.bufferSubData(
			GL_ARRAY_BUFFER,
			m_vbo_indirect_elements_argument_offset, /* the very first argument is 'count' which we need to update */
			sizeof(m_indirect_draw_call_count_argument), &m_indirect_draw_call_count_argument);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call(s) failed.");

		for (int gs_output_it = static_cast<int>(PipelineStatisticsQueryUtilities::GEOMETRY_SHADER_OUTPUT_FIRST);
			 gs_output_it != static_cast<int>(PipelineStatisticsQueryUtilities::GEOMETRY_SHADER_OUTPUT_COUNT);
			 ++gs_output_it)
		{
			PipelineStatisticsQueryUtilities::_geometry_shader_output gs_output =
				static_cast<PipelineStatisticsQueryUtilities::_geometry_shader_output>(gs_output_it);
			/* For GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB query, we need to test both single-stream and
			 * multi-stream geometry shaders.
			 *
			 * For GL_GEOMETRY_SHADER_INVOCATIONS, we only need a single iteration.
			 **/
			const unsigned int n_internal_iterations =
				(current_query_target == GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB) ? 2 : 1;

			for (unsigned int n_internal_iteration = 0; n_internal_iteration < n_internal_iterations;
				 ++n_internal_iteration)
			{
				/* Build the test program. */
				std::string gs_body;

				if (n_internal_iteration == 1)
				{
					/* This path will only be entered for GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB query.
					 *
					 * OpenGL does not support multiple vertex streams for output primitive types other than
					 * points.
					 */
					if (gs_output != PipelineStatisticsQueryUtilities::GEOMETRY_SHADER_OUTPUT_POINTS)
					{
						continue;
					}

					/* Build a multi-streamed geometry shader */
					gs_body = PipelineStatisticsQueryUtilities::buildGeometryShaderBody(
						gs_input, gs_output, m_n_primitives_emitted_by_gs, m_n_streams_emitted_by_gs);
				}
				else
				{
					gs_body = PipelineStatisticsQueryUtilities::buildGeometryShaderBody(
						gs_input, gs_output, m_n_primitives_emitted_by_gs, 1); /* n_streams */
				}

				buildProgram(DE_NULL,																	/* cs_body */
							 PipelineStatisticsQueryUtilities::dummy_fs_code, gs_body.c_str(), DE_NULL, /* tc_body */
							 DE_NULL,																	/* te_body */
							 PipelineStatisticsQueryUtilities::dummy_vs_code);

				gl.useProgram(m_po_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

				/* Set up the primitive type */
				m_current_primitive_type = PipelineStatisticsQueryUtilities::getPrimitiveTypeFromGSInput(gs_input);

				/* Iterate through all draw call types */
				for (unsigned int n_draw_call_type = 0;
					 n_draw_call_type < PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_COUNT; ++n_draw_call_type)
				{
					m_current_draw_call_type = (PipelineStatisticsQueryUtilities::_draw_call_type)n_draw_call_type;

					/* Only continue if the draw call is supported by the context */
					if (!PipelineStatisticsQueryUtilities::isDrawCallSupported(m_current_draw_call_type, gl))
					{
						continue;
					}

					/* Execute the query */
					if (!PipelineStatisticsQueryUtilities::executeQuery(
							current_query_target, m_qo_id, m_bo_qo_id, queryCallbackDrawCallHandler,
							(PipelineStatisticsQueryTestFunctionalBase*)this, m_context.getRenderContext(), m_testCtx,
							m_context.getContextInfo(), &run_result))
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Could not retrieve test run results for query target "
														"["
							<< PipelineStatisticsQueryUtilities::getStringForEnum(current_query_target) << "]"
							<< tcu::TestLog::EndMessage;

						result = false;
					}
					else
					{
						unsigned int										 n_expected_values  = 0;
						glw::GLuint64										 expected_values[2] = { 0 };
						PipelineStatisticsQueryUtilities::_verification_type verification_type =
							PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_UNDEFINED;

						if (current_query_target == GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB)
						{
							n_expected_values  = 2;
							expected_values[0] = m_n_primitives_emitted_by_gs;
							expected_values[1] = m_n_primitives_emitted_by_gs;
							verification_type  = PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EXACT_MATCH;

							if (n_internal_iteration == 1)
							{
								/* Multi-stream geometry shader case. Count in non-default vertex streams */
								for (unsigned int n_stream = 1; n_stream < m_n_streams_emitted_by_gs; ++n_stream)
								{
									expected_values[1] += (m_n_primitives_emitted_by_gs + n_stream);
								} /* for (non-default streams) */
							}

							if (PipelineStatisticsQueryUtilities::isInstancedDrawCall(m_current_draw_call_type))
							{
								expected_values[0] *= m_indirect_draw_call_primcount_argument;
								expected_values[1] *= m_indirect_draw_call_primcount_argument;
							}
						}
						else
						{
							n_expected_values  = 1;
							expected_values[0] = 1; /* as per test spec */
							verification_type  = PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EQUAL_OR_GREATER;
						}

						/* Compare it against query result values */
						result &= PipelineStatisticsQueryUtilities::verifyResultValues(
							run_result, n_expected_values, expected_values,
							m_qo_id != 0, /* should_check_qo_bo_values */
							current_query_target, &m_current_draw_call_type, &m_current_primitive_type,
							false, /* is_primitive_restart_enabled */
							m_testCtx, verification_type);

					} /* if (run results were obtained successfully) */
				}	 /* for (all draw call types) */
			}		  /* for (all internal iterations) */
		}			  /* for (all geometry shader output primitive types) */
	}				  /* for (all geometry shader input primitive types) */
	return result;
}

/** Initializes all GL objects used by the test */
void PipelineStatisticsQueryTestFunctional6::initObjects()
{
	/* This test should not execute if we're not running at least a GL3.2 context */
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 2)))
	{
		throw tcu::NotSupportedError("OpenGL 3.2+ is required to run this test.");
	}
}

/** Tells whether the test instance should be executed for user-specified query target.
 *
 *  @param query_target Query target to be used for the call.
 *
 *  @return true  if @param query_target is either GL_GEOMETRY_SHADER_INVOCATIONS, or
 *                GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB.
 *          false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional6::shouldExecuteForQueryTarget(glw::GLenum query_target)
{
	return (query_target == GL_GEOMETRY_SHADER_INVOCATIONS ||
			query_target == GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB);
}

/** Constructor.
 *
 *  @param context Rendering context
 */
PipelineStatisticsQueryTestFunctional7::PipelineStatisticsQueryTestFunctional7(deqp::Context& context)
	: PipelineStatisticsQueryTestFunctionalBase(context, "functional_fragment_shader_invocations",
												"Verifies GL_FRAGMENT_SHADER_INVOCATIONS_ARB queries "
												"work correctly.")
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestFunctional7::deinitObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** Executes a test iteration for user-specified query target.
 *
 *  @param current_query_target Pipeline statistics query target to execute the iteration
 *                              for.
 *
 *  @return true if the test passed for the iteration, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional7::executeTest(glw::GLenum current_query_target)
{
	const glw::Functions&									 gl		= m_context.getRenderContext().getFunctions();
	bool													 result = true;
	PipelineStatisticsQueryUtilities::_test_execution_result run_result;

	/* Sanity check: This method should only be called for GL_FRAGMENT_SHADER_INVOCATIONS_ARB query */
	DE_ASSERT(current_query_target == GL_FRAGMENT_SHADER_INVOCATIONS_ARB);

	/* Set up VBO. */
	const unsigned int n_vertex_components = 2;
	const float		   vertex_data[]	   = { 0.0f,  0.75f, -0.75f, -0.75f, 0.75f, -0.75f, 0.3f, 0.7f,
								  -0.4f, 0.2f,  0.6f,   -0.3f,  -0.3f, -0.7f,  0.0f, 0.0f };
	const unsigned int index_data[] = { 0, 2, 1, 3, 4, 5, 6, 7 };

	m_indirect_draw_call_baseinstance_argument = 1;
	m_indirect_draw_call_basevertex_argument   = 1;
	m_indirect_draw_call_count_argument =
		3; /* this value will be updated in the actual loop, so use anything for now */
	m_indirect_draw_call_first_argument		= 0;
	m_indirect_draw_call_primcount_argument = 4;

	initFBO();
	initVBO(vertex_data, sizeof(vertex_data), index_data, sizeof(index_data), m_indirect_draw_call_count_argument,
			m_indirect_draw_call_primcount_argument, m_indirect_draw_call_baseinstance_argument,
			m_indirect_draw_call_first_argument, m_indirect_draw_call_basevertex_argument);

	initVAO(n_vertex_components);

	/* Iterate over all primitive types */
	for (int current_primitive_type_it = static_cast<int>(PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_FIRST);
		 current_primitive_type_it < static_cast<int>(PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_COUNT);
		 ++current_primitive_type_it)
	{
		PipelineStatisticsQueryUtilities::_primitive_type current_primitive_type =
			static_cast<PipelineStatisticsQueryUtilities::_primitive_type>(current_primitive_type_it);
		/* Exclude 'patches' primitive type */
		if (current_primitive_type == PipelineStatisticsQueryUtilities::PRIMITIVE_TYPE_PATCHES)
		{
			continue;
		}

		m_current_primitive_type = current_primitive_type;

		/* Update 'count' argument so that we only use as many vertices as needed for current
		 * primitive type.
		 */
		unsigned int count_argument_value =
			PipelineStatisticsQueryUtilities::getNumberOfVerticesForPrimitiveType(m_current_primitive_type);

		m_indirect_draw_call_count_argument = count_argument_value;

		gl.bufferSubData(GL_ARRAY_BUFFER, m_vbo_indirect_arrays_argument_offset, sizeof(unsigned int),
						 &m_indirect_draw_call_count_argument);
		gl.bufferSubData(GL_ARRAY_BUFFER, m_vbo_indirect_elements_argument_offset, sizeof(unsigned int),
						 &m_indirect_draw_call_count_argument);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call(s) failed.");

		/* Iterate through all draw call types */
		for (unsigned int n_draw_call_type = 0;
			 n_draw_call_type < PipelineStatisticsQueryUtilities::DRAW_CALL_TYPE_COUNT; ++n_draw_call_type)
		{
			m_current_draw_call_type = (PipelineStatisticsQueryUtilities::_draw_call_type)n_draw_call_type;

			/* Only continue if the draw call is supported by the context */
			if (!PipelineStatisticsQueryUtilities::isDrawCallSupported(m_current_draw_call_type, gl))
			{
				continue;
			}

			/* Clear the buffers before we proceed */
			gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

			/* Execute the query */
			if (!PipelineStatisticsQueryUtilities::executeQuery(
					current_query_target, m_qo_id, m_bo_qo_id, queryCallbackDrawCallHandler,
					(PipelineStatisticsQueryTestFunctionalBase*)this, m_context.getRenderContext(), m_testCtx,
					m_context.getContextInfo(), &run_result))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Could not retrieve test run results for query target "
															   "["
								   << PipelineStatisticsQueryUtilities::getStringForEnum(current_query_target) << "]"
								   << tcu::TestLog::EndMessage;

				result = false;
			}
			else
			{
				static const glw::GLuint64 expected_value = 1; /* as per test spec */

				/* Compare it against query result values */
				result &= PipelineStatisticsQueryUtilities::verifyResultValues(
					run_result, 1, &expected_value, m_qo_id != 0, /* should_check_qo_bo_values */
					current_query_target, &m_current_draw_call_type, &m_current_primitive_type,
					false, /* is_primitive_restart_enabled */
					m_testCtx, PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EQUAL_OR_GREATER);

			} /* if (run results were obtained successfully) */
		}	 /* for (all draw call types) */
	}		  /* for (all primitive types) */

	return result;
}

/** Initializes all GL objects used by the test */
void PipelineStatisticsQueryTestFunctional7::initObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	buildProgram(DE_NULL,												   /* cs_body */
				 PipelineStatisticsQueryUtilities::dummy_fs_code, DE_NULL, /* gs_body */
				 DE_NULL,												   /* tc_body */
				 DE_NULL,												   /* te_body */
				 PipelineStatisticsQueryUtilities::dummy_vs_code);

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
}

/** Tells whether the test instance should be executed for user-specified query target.
 *
 *  @param query_target Query target to be used for the call.
 *
 *  @return true  if @param query_target is GL_FRAGMENT_SHADER_INVOCATIONS_ARB.
 *          false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional7::shouldExecuteForQueryTarget(glw::GLenum query_target)
{
	return (query_target == GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
}

/** Constructor.
 *
 *  @param context Rendering context
 */
PipelineStatisticsQueryTestFunctional8::PipelineStatisticsQueryTestFunctional8(deqp::Context& context)
	: PipelineStatisticsQueryTestFunctionalBase(context, "functional_compute_shader_invocations",
												"Verifies that GL_COMPUTE_SHADER_INVOCATIONS_ARB queries "
												"work correctly.")
	, m_bo_dispatch_compute_indirect_args_offset(0)
	, m_bo_id(0)
	, m_current_iteration(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that were created during test execution. */
void PipelineStatisticsQueryTestFunctional8::deinitObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}
}

/** Executes a test iteration for user-specified query target.
 *
 *  @param current_query_target Pipeline statistics query target to execute the iteration
 *                              for.
 *
 *  @return true if the test passed for the iteration, false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional8::executeTest(glw::GLenum current_query_target)
{
	bool													 result = true;
	PipelineStatisticsQueryUtilities::_test_execution_result run_result;

	/* Sanity check: This method should only be called for
	 * GL_COMPUTE_SHADER_INVOCATIONS_ARB queries. */
	DE_ASSERT(current_query_target == GL_COMPUTE_SHADER_INVOCATIONS_ARB);

	/* This test needs to be run in two iterations:
	 *
	 * 1. glDispatchCompute() should be called.
	 * 2. glDispatchComputeIndirect() should be called.
	 *
	 */
	for (m_current_iteration = 0; m_current_iteration < 2; /* as per description */
		 ++m_current_iteration)
	{
		/* Execute the query */
		if (!PipelineStatisticsQueryUtilities::executeQuery(
				current_query_target, m_qo_id, m_bo_qo_id, queryCallbackDispatchCallHandler, this,
				m_context.getRenderContext(), m_testCtx, m_context.getContextInfo(), &run_result))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Could not retrieve test run results for query target "
														   "["
							   << PipelineStatisticsQueryUtilities::getStringForEnum(current_query_target) << "]"
							   << tcu::TestLog::EndMessage;

			result = false;
		}
		else
		{
			static const glw::GLuint64 expected_value = 1; /* as per test spec */

			/* Compare it against query result values */
			result &= PipelineStatisticsQueryUtilities::verifyResultValues(
				run_result, 1, &expected_value, m_qo_id != 0,  /* should_check_qo_bo_values */
				current_query_target, DE_NULL, DE_NULL, false, /* is_primitive_restart_enabled */
				m_testCtx, PipelineStatisticsQueryUtilities::VERIFICATION_TYPE_EQUAL_OR_GREATER);
		} /* if (run results were obtained successfully) */
	}	 /* for (both iterations) */

	return result;
}

/** Initializes all GL objects used by the test */
void PipelineStatisticsQueryTestFunctional8::initObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	buildProgram(PipelineStatisticsQueryUtilities::dummy_cs_code, DE_NULL, /* fs_body */
				 DE_NULL,												   /* gs_body */
				 DE_NULL,												   /* tc_body */
				 DE_NULL,												   /* te_body */
				 DE_NULL);												   /* vs_body */

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Init BO to hold atomic counter data, as well as the indirect dispatch compute
	 * draw call arguments */
	unsigned int	   atomic_counter_value = 0;
	const unsigned int bo_size				= sizeof(unsigned int) * (1 /* counter value */ + 3 /* draw call args */);

	const unsigned int drawcall_args[] = {
		1, /* num_groups_x */
		1, /* num_groups_y */
		1  /* num_groups_z */
	};

	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	gl.bindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call(s) failed.");

	gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, /* index */
					  m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, bo_size, DE_NULL, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	gl.bufferSubData(GL_ARRAY_BUFFER, 0, /* offset */
					 sizeof(unsigned int), &atomic_counter_value);
	gl.bufferSubData(GL_ARRAY_BUFFER, sizeof(unsigned int), /* offset */
					 sizeof(drawcall_args), drawcall_args);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");

	/* Store rthe offset, at which the draw call args start */
	m_bo_dispatch_compute_indirect_args_offset = sizeof(unsigned int);
}

/** Either issues a regular or indirect compute shader dispatch call, and then verifies
 *  the call has executed without any error being generated. The regular dispatch call
 *  will be executed if pInstance->m_current_iteration is equal to 0, otherwise the
 *  method will use the indirect version.
 *
 *  @param pInstance Pointer to a PipelineStatisticsQueryTestFunctional8 instance.
 */
bool PipelineStatisticsQueryTestFunctional8::queryCallbackDispatchCallHandler(void* pInstance)
{
	glw::GLenum								error_code = GL_NO_ERROR;
	PipelineStatisticsQueryTestFunctional8* pThis	  = (PipelineStatisticsQueryTestFunctional8*)pInstance;
	bool									result	 = true;
	const glw::Functions&					gl		   = pThis->m_context.getRenderContext().getFunctions();

	if (pThis->m_current_iteration == 0)
	{
		gl.dispatchCompute(1,  /* num_groups_x */
						   1,  /* num_groups_y */
						   1); /* num_groups_z */
	}
	else
	{
		gl.dispatchComputeIndirect(pThis->m_bo_dispatch_compute_indirect_args_offset);
	}

	error_code = gl.getError();
	if (error_code != GL_NO_ERROR)
	{
		pThis->m_testCtx.getLog() << tcu::TestLog::Message
								  << ((pThis->m_current_iteration == 0) ? "glDispatchCompute()" :
																		  "glDispatchComputeIndirect()")
								  << " call failed with error code "
									 "["
								  << error_code << "]." << tcu::TestLog::EndMessage;

		result = false;
	}

	return result;
}

/** Tells whether the test instance should be executed for user-specified query target.
 *
 *  @param query_target Query target to be used for the call.
 *
 *  @return true  if @param query_target is GL_COMPUT_SHADER_INVOCATIONS_ARB.
 *          false otherwise.
 **/
bool PipelineStatisticsQueryTestFunctional8::shouldExecuteForQueryTarget(glw::GLenum query_target)
{
	return (query_target == GL_COMPUTE_SHADER_INVOCATIONS_ARB);
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
PipelineStatisticsQueryTests::PipelineStatisticsQueryTests(deqp::Context& context)
	: TestCaseGroup(context, "pipeline_statistics_query_tests_ARB",
					"Contains conformance tests that verify GL implementation's support "
					"for GL_ARB_pipeline_statistics_query extension.")
{
	/* Left blank intentionally */
}

/** Initializes the test group contents. */
void PipelineStatisticsQueryTests::init()
{
	addChild(new PipelineStatisticsQueryTestAPICoverage1(m_context));
	addChild(new PipelineStatisticsQueryTestAPICoverage2(m_context));
	addChild(new PipelineStatisticsQueryTestFunctional1(m_context));
	addChild(new PipelineStatisticsQueryTestFunctional2(m_context));
	addChild(new PipelineStatisticsQueryTestFunctional3(m_context));
	addChild(new PipelineStatisticsQueryTestFunctional4(m_context));
	addChild(new PipelineStatisticsQueryTestFunctional5(m_context));
	addChild(new PipelineStatisticsQueryTestFunctional6(m_context));
	addChild(new PipelineStatisticsQueryTestFunctional7(m_context));
	addChild(new PipelineStatisticsQueryTestFunctional8(m_context));
}
} /* glcts namespace */
