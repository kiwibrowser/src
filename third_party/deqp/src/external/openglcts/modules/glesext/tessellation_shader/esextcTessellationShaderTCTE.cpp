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

#include "esextcTessellationShaderTCTE.hpp"
#include "esextcTessellationShaderUtils.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <algorithm>

namespace glcts
{
/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTCTETests::TessellationShaderTCTETests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "tessellation_control_to_tessellation_evaluation",
						"Verifies various aspects of communication between tessellation "
						"control and tessellation evaluation stages")
{
	/* No implementation needed */
}

/**
 * Initializes test groups for geometry shader tests
 **/
void TessellationShaderTCTETests::init(void)
{
	addChild(new glcts::TessellationShaderTCTEDataPassThrough(m_context, m_extParams));
	addChild(new glcts::TessellationShaderTCTEgl_in(m_context, m_extParams));
	addChild(new glcts::TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize(m_context, m_extParams));
	addChild(new glcts::TessellationShaderTCTEgl_PatchVerticesIn(m_context, m_extParams));
	addChild(new glcts::TessellationShaderTCTEgl_TessLevel(m_context, m_extParams));
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTCTEDataPassThrough::TessellationShaderTCTEDataPassThrough(Context&			  context,
																			 const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "data_pass_through",
				   "Verifies data is correctly passed down the VS->TC->TS->(GS) pipeline.")
	, m_bo_id(0)
	, m_n_input_vertices_per_run(4)
	, m_utils_ptr(DE_NULL)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderTCTEDataPassThrough::deinit()
{
	/** Call base class' deinit() function */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Revert GL_PATCH_VERTICES_EXT value to the default setting */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Revert TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Release all objects we might've created */
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

	for (_runs::iterator it = m_runs.begin(); it != m_runs.end(); ++it)
	{
		deinitTestRun(*it);
	}
	m_runs.clear();

	/* Release Utils instance */
	if (m_utils_ptr != DE_NULL)
	{
		delete m_utils_ptr;

		m_utils_ptr = DE_NULL;
	}
}

/** Deinitializes all ES object created for a specific test run. **/
void TessellationShaderTCTEDataPassThrough::deinitTestRun(_run& run)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (run.fs_id != 0)
	{
		gl.deleteShader(run.fs_id);

		run.fs_id = 0;
	}

	if (run.gs_id != 0)
	{
		gl.deleteShader(run.gs_id);

		run.gs_id = 0;
	}

	if (run.po_id != 0)
	{
		gl.deleteProgram(run.po_id);

		run.po_id = 0;
	}

	if (run.tcs_id != 0)
	{
		gl.deleteShader(run.tcs_id);

		run.tcs_id = 0;
	}

	if (run.tes_id != 0)
	{
		gl.deleteShader(run.tes_id);

		run.tes_id = 0;
	}

	if (run.vs_id != 0)
	{
		gl.deleteShader(run.vs_id);

		run.vs_id = 0;
	}
}

/** Initializes all ES objects that will be used for the test. */
void TessellationShaderTCTEDataPassThrough::initTest()
{
	/* The test requires EXT_tessellation_shader */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Create an Utils instance */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_utils_ptr = new TessellationShaderUtils(gl, this);

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Our program objects take a single vertex per patch */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() call failed");

	/* Disable rasterization */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed");

	/* Create a buffer object we will use for XFB */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	/* Set up XFB buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed");

	/* Prepare all the runs */
	const _tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);

	/* Iterate over all supported primitive modes */
	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];

		/* If geometry shaders are supported, include a separate iteration to include them
		 * in the pipeline
		 */
		for (int n_gs_stage_usage = 0; n_gs_stage_usage < ((m_is_geometry_shader_extension_supported) ? 2 : 1);
			 ++n_gs_stage_usage)
		{
			bool use_gs_stage = (n_gs_stage_usage == 1);

			/* If geometry shaders support gl_PointSize, include a separate iteration to pass
			 * point size data as well */
			for (int n_gs_pointsize_usage = 0;
				 n_gs_pointsize_usage < ((m_is_geometry_shader_point_size_supported) ? 2 : 1); ++n_gs_pointsize_usage)
			{
				bool use_gs_pointsize_data = (n_gs_pointsize_usage == 1);

				/* If tessellation shaders support gl_PointSize, include a separate iteration to pass
				 * point size data as well */
				for (int n_ts_pointsize_usage = 0;
					 n_ts_pointsize_usage < ((m_is_tessellation_shader_point_size_supported) ? 2 : 1);
					 ++n_ts_pointsize_usage)
				{
					bool use_ts_pointsize_data = (n_ts_pointsize_usage == 1);

					/* Note: it does not make sense to try to pass gl_PointSize data
					 *       in geometry stage if tessellation stage did not provide it.
					 */
					if (!use_ts_pointsize_data && use_gs_pointsize_data)
					{
						continue;
					}

					/* Initialize test run data */
					_run run;

					executeTestRun(run, primitive_mode, use_gs_stage, use_gs_pointsize_data, use_ts_pointsize_data);

					/* Store the run for later usage */
					m_runs.push_back(run);
				} /* for (tessellation point size data usage off and on cases) */
			}	 /* for (geometry point size data usage off and on cases) */
		}		  /* for (GS stage usage) */
	}			  /* for (all primitive modes) */
}

/** Initializes a test run, executes it and gathers all the rendered data for further
 *  processing. Extracted data is stored in the run descriptor.
 *
 *  @param run                               Test run descriptor to fill with ES object data,
 *                                           as well as generated data.
 *  @param primitive_mode                    Primitive mode to use for the test run.
 *  @param should_use_geometry_shader        true  if the test run should use Geometry Shader stage,
 *                                           false otherwise.
 *  @param should_pass_point_size_data_in_gs true if the test run should define two output variables
 *                                           in Geometry Shader, later set to gl_PointSize values from
 *                                           TC and TE stages. False to skip them.
 *                                           Only set to true if GL_EXT_geometry_point_size extension
 *                                           is supported.
 *  @param should_pass_point_size_data_in_ts true if the test run should define two output variables
 *                                           in both Tessellation Shader types, set to gl_PointSize values
 *                                           as accessible during execution. False to skip the definitions.
 *                                           Only set to true if GL_EXT_tessellation_point_size extension
 *                                           is supported.
 */
void TessellationShaderTCTEDataPassThrough::executeTestRun(_run& run, _tessellation_primitive_mode primitive_mode,
														   bool should_use_geometry_shader,
														   bool should_pass_point_size_data_in_gs,
														   bool should_pass_point_size_data_in_ts)
{
	run.primitive_mode = primitive_mode;

	/* Retrieve ES entry-points before we start */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create a program object first */
	run.po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Create all shader objects we wil be later attaching to the program object */
	run.fs_id  = gl.createShader(GL_FRAGMENT_SHADER);
	run.tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	run.tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	run.vs_id  = gl.createShader(GL_VERTEX_SHADER);

	if (should_use_geometry_shader)
	{
		run.gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed");

	/* Attach the shader objects to the program object */
	gl.attachShader(run.po_id, run.fs_id);
	gl.attachShader(run.po_id, run.tcs_id);
	gl.attachShader(run.po_id, run.tes_id);
	gl.attachShader(run.po_id, run.vs_id);

	if (should_use_geometry_shader)
	{
		gl.attachShader(run.po_id, run.gs_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed");

	/* Set vertex shader's body */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "${SHADER_IO_BLOCKS_REQUIRE}\n"
						  "\n"
						  "out OUT_VS\n"
						  "{\n"
						  "    vec4  value1;\n"
						  "    ivec4 value2;\n"
						  "} out_data;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position     = vec4( float(gl_VertexID) );\n"
						  "    gl_PointSize    = 1.0 / float(gl_VertexID + 1);\n"
						  "    out_data.value1 =  vec4(float(gl_VertexID),        float(gl_VertexID) * 0.5,\n"
						  "                            float(gl_VertexID) * 0.25, float(gl_VertexID) * 0.125);\n"
						  "    out_data.value2 = ivec4(gl_VertexID,               gl_VertexID + 1,\n"
						  "                            gl_VertexID + 2,           gl_VertexID + 3);\n"
						  "}\n";

	shaderSourceSpecialized(run.vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for vertex shader");

	/* Set dummy fragment shader's body */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(run.fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for fragment shader");

	/* Set tessellation control shader's body */
	{
		std::stringstream body_sstream;
		std::string		  body_string;
		const char*		  body_raw_ptr = DE_NULL;

		body_sstream << "${VERSION}\n"
						"\n"
						"${TESSELLATION_SHADER_REQUIRE}\n";

		if (should_pass_point_size_data_in_ts)
		{
			body_sstream << "${TESSELLATION_POINT_SIZE_REQUIRE}\n";
		}

		body_sstream << "\n"
						"layout(vertices = 2) out;\n"
						"\n"
						"in OUT_VS\n"
						"{\n"
						"    vec4  value1;\n"
						"    ivec4 value2;\n"
						"} in_vs_data[];\n"
						"\n"
						"out OUT_TC\n"
						"{\n";

		if (should_pass_point_size_data_in_ts)
		{
			body_sstream << "    float tc_pointSize;\n";
		}

		body_sstream << "     vec4  tc_position;\n"
						"     vec4  tc_value1;\n"
						"    ivec4  tc_value2;\n"
						"} out_data[];\n"
						"\n"
						"patch out vec4 tc_patch_data;\n"
						"\n"
						"void main()\n"
						"{\n"
						"    int multiplier = 1;\n"
						"\n"
						"    if (gl_InvocationID == 0)\n"
						"    {\n"
						"        multiplier = 2;\n"
						"    }\n";

		if (should_pass_point_size_data_in_ts)
		{
			body_sstream << "    out_data         [gl_InvocationID].tc_pointSize = gl_in[0].gl_PointSize;\n"
							"    gl_out           [gl_InvocationID].gl_PointSize = gl_in[0].gl_PointSize * 2.0;\n";
		}

		body_sstream << "    out_data         [gl_InvocationID].tc_position  = gl_in     [0].gl_Position;\n"
						"    out_data         [gl_InvocationID].tc_value1    = in_vs_data[0].value1      *  "
						"vec4(float(multiplier) );\n"
						"    out_data         [gl_InvocationID].tc_value2    = in_vs_data[0].value2      * ivec4(      "
						"multiplier);\n"
						"    gl_out           [gl_InvocationID].gl_Position  = gl_in     [0].gl_Position + vec4(3.0);\n"
						"    gl_TessLevelInner[0]                            = 4.0;\n"
						"    gl_TessLevelInner[1]                            = 4.0;\n"
						"    gl_TessLevelOuter[0]                            = 4.0;\n"
						"    gl_TessLevelOuter[1]                            = 4.0;\n"
						"    gl_TessLevelOuter[2]                            = 4.0;\n"
						"    gl_TessLevelOuter[3]                            = 4.0;\n"
						"\n"
						"    if (gl_InvocationID == 0)\n"
						"    {\n"
						"        tc_patch_data = in_vs_data[0].value1 *  vec4(float(multiplier) );\n"
						"    }\n"
						"}\n";

		body_string  = body_sstream.str();
		body_raw_ptr = body_string.c_str();

		shaderSourceSpecialized(run.tcs_id, 1 /* count */, &body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for tessellation control shader");
	}

	/* Set tessellation evaluation shader's body */
	{
		std::stringstream body_sstream;
		std::string		  body_string;
		const char*		  body_raw_ptr = DE_NULL;

		/* Preamble */
		body_sstream << "${VERSION}\n"
						"\n"
						"${TESSELLATION_SHADER_REQUIRE}\n";

		if (should_pass_point_size_data_in_ts)
		{
			body_sstream << "${TESSELLATION_POINT_SIZE_REQUIRE}\n";
		}

		/* Layout qualifiers */
		body_sstream << "\n"
						"layout(PRIMITIVE_MODE, point_mode) in;\n"
						"\n"

						/* Input block definition starts here: */
						"in OUT_TC\n"
						"{\n";

		if (should_pass_point_size_data_in_ts)
		{
			body_sstream << "    float tc_pointSize;\n";
		}

		body_sstream << "    vec4  tc_position;\n"
						"    vec4  tc_value1;\n"
						"   ivec4  tc_value2;\n"
						"} in_data[];\n"
						"\n"
						"patch in vec4 tc_patch_data;\n"
						"\n";
		/* Input block definition ends here. */

		/* Output block definition (only defined if GS stage is present) starts here: */
		if (should_use_geometry_shader)
		{
			body_sstream << "out OUT_TE\n"
							"{\n";

			/* Output block contents */
			if (should_pass_point_size_data_in_ts)
			{
				body_sstream << "    float tc_pointSize;\n"
								"    float te_pointSize;\n";
			}

			body_sstream << "    vec4  tc_position;\n"
							"    vec4  tc_value1;\n"
							"   ivec4  tc_value2;\n"
							"    vec4  te_position;\n"
							"} out_data;\n";
		}
		/* Output block definition ends here. */
		else
		{
			if (should_pass_point_size_data_in_ts)
			{
				body_sstream << "out float tc_pointSize;\n"
								"out float te_pointSize;\n";
			}

			body_sstream << "out  vec4  tc_position;\n"
							"out  vec4  tc_value1;\n"
							"flat out ivec4  tc_value2;\n"
							"out  vec4  te_position;\n"
							"out  vec4  te_patch_data;\n";
		}

		body_sstream << "\n"
						"void main()\n"
						"{\n";

		if (should_use_geometry_shader)
		{
			body_sstream << "#define OUTPUT_VARIABLE(x) out_data.x\n";
		}
		else
		{
			body_sstream << "#define OUTPUT_VARIABLE(x) x\n";
		}

		if (should_pass_point_size_data_in_ts)
		{
			body_sstream << "    OUTPUT_VARIABLE(tc_pointSize) = in_data[1].tc_pointSize;\n"
							"    OUTPUT_VARIABLE(te_pointSize) = gl_in[1].gl_PointSize;\n";
		}

		body_sstream << "    OUTPUT_VARIABLE(tc_position)   = in_data[1].tc_position;\n"
						"    OUTPUT_VARIABLE(tc_value1)     = in_data[0].tc_value1;\n"
						"    OUTPUT_VARIABLE(tc_value2)     = in_data[1].tc_value2;\n"
						"    OUTPUT_VARIABLE(te_position)   = gl_in[0].gl_Position;\n";

		if (!should_use_geometry_shader)
		{
			body_sstream << "    OUTPUT_VARIABLE(te_patch_data) = tc_patch_data;\n";
		}
		body_sstream << "}\n";

		body_string = body_sstream.str();

		/* Replace PRIMITIVE_MODE token with user-requested primitive mode */
		std::string primitive_mode_replacement	= TessellationShaderUtils::getESTokenForPrimitiveMode(primitive_mode);
		std::string primitive_mode_token		  = "PRIMITIVE_MODE";
		std::size_t primitive_mode_token_position = std::string::npos;

		primitive_mode_token_position = body_string.find(primitive_mode_token);

		while (primitive_mode_token_position != std::string::npos)
		{
			body_string = body_string.replace(primitive_mode_token_position, primitive_mode_token.length(),
											  primitive_mode_replacement);

			primitive_mode_token_position = body_string.find(primitive_mode_token);
		}

		body_raw_ptr = body_string.c_str();

		shaderSourceSpecialized(run.tes_id, 1 /* count */, &body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for tessellation evaluation shader");
	}

	/* Set geometry shader's body (if requested) */
	if (should_use_geometry_shader)
	{
		std::stringstream body_sstream;
		std::string		  body_string;
		const char*		  body_raw_ptr = DE_NULL;

		body_sstream << "${VERSION}\n"
						"\n"
						"${GEOMETRY_SHADER_REQUIRE}\n";

		if (should_pass_point_size_data_in_gs)
		{
			body_sstream << "${GEOMETRY_POINT_SIZE_REQUIRE}\n";
		}

		body_sstream << "${SHADER_IO_BLOCKS_REQUIRE}\n"
						"\n"
						"layout(points)                   in;\n"
						"layout(max_vertices = 2, points) out;\n"
						"\n"
						"in OUT_TE\n"
						"{\n";

		if (should_pass_point_size_data_in_ts)
		{
			body_sstream << "    float tc_pointSize;\n"
							"    float te_pointSize;\n";
		}

		body_sstream << "    vec4  tc_position;\n"
						"    vec4  tc_value1;\n"
						"   ivec4  tc_value2;\n"
						"    vec4  te_position;\n"
						"} in_data[1];\n"
						"\n"
						"out float gs_tc_pointSize;\n"
						"out float gs_te_pointSize;\n"
						"out  vec4 gs_tc_position;\n"
						"out  vec4 gs_tc_value1;\n"
						"flat out ivec4 gs_tc_value2;\n"
						"out  vec4 gs_te_position;\n"
						"\n"
						"void main()\n"
						"{\n";

		if (should_pass_point_size_data_in_gs)
		{
			body_sstream << "    gs_tc_pointSize = in_data[0].tc_pointSize;\n"
							"    gs_te_pointSize = in_data[0].te_pointSize;\n";
		}

		body_sstream << "    gs_tc_position = in_data[0].tc_position;\n"
						"    gs_tc_value1   = in_data[0].tc_value1;\n"
						"    gs_tc_value2   = in_data[0].tc_value2;\n"
						"    gs_te_position = in_data[0].te_position;\n"
						"    EmitVertex();\n";

		if (should_pass_point_size_data_in_gs)
		{
			body_sstream << "    gs_tc_pointSize = in_data[0].tc_pointSize + 1.0;\n"
							"    gs_te_pointSize = in_data[0].te_pointSize + 1.0;\n";
		}

		body_sstream << "    gs_tc_position = in_data[0].tc_position +  vec4(1.0);\n"
						"    gs_tc_value1   = in_data[0].tc_value1   +  vec4(1.0);\n"
						"    gs_tc_value2   = in_data[0].tc_value2   + ivec4(1);\n"
						"    gs_te_position = in_data[0].te_position +  vec4(1.0);\n"
						"\n"
						"    EmitVertex();\n"
						"\n"
						"}\n";

		body_string  = body_sstream.str();
		body_raw_ptr = body_string.c_str();

		shaderSourceSpecialized(run.gs_id, 1 /* count */, &body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for geometry shader");
	}

	/* Configure varyings */
	unsigned int	   n_varyings					= 0;
	int				   varying_tc_pointSize_offset  = -1;
	int				   varying_tc_position_offset   = -1;
	int				   varying_tc_value1_offset		= -1;
	int				   varying_tc_value2_offset		= -1;
	int				   varying_te_patch_data_offset = -1;
	int				   varying_te_pointSize_offset  = -1;
	int				   varying_te_position_offset   = -1;
	const unsigned int varying_patch_data_size		= sizeof(float) * 4; /*  vec4 */
	const unsigned int varying_pointSize_size		= sizeof(float);
	const unsigned int varying_position_size		= sizeof(float) * 4; /*  vec4 */
	const unsigned int varying_value1_size			= sizeof(float) * 4; /*  vec4 */
	const unsigned int varying_value2_size			= sizeof(int) * 4;   /* ivec4 */
	const char**	   varyings						= DE_NULL;
	unsigned int	   varyings_size				= 0;

	const char* gs_non_point_size_varyings[] = { "gs_tc_position", "gs_tc_value1", "gs_tc_value2", "gs_te_position" };
	const char* gs_point_size_varyings[]	 = { "gs_tc_position", "gs_tc_value1",	"gs_tc_value2",
											 "gs_te_position", "gs_tc_pointSize", "gs_te_pointSize" };
	const char* non_gs_non_point_size_varyings[] = { "tc_position", "tc_value1", "tc_value2", "te_position",
													 "te_patch_data" };
	const char* non_gs_point_size_varyings[] = { "tc_position",  "tc_value1",	"tc_value2",	"te_position",
												 "tc_pointSize", "te_pointSize", "te_patch_data" };

	if (should_use_geometry_shader)
	{
		if (should_pass_point_size_data_in_gs)
		{
			n_varyings	= sizeof(gs_point_size_varyings) / sizeof(gs_point_size_varyings[0]);
			varyings	  = gs_point_size_varyings;
			varyings_size = varying_position_size +  /* gs_tc_position  */
							varying_value1_size +	/* gs_tc_value1    */
							varying_value2_size +	/* gs_tc_value2    */
							varying_position_size +  /* gs_te_position  */
							varying_pointSize_size + /* gs_tc_pointSize */
							varying_pointSize_size;  /* gs_te_pointSize */

			varying_tc_position_offset  = 0;
			varying_tc_value1_offset	= varying_tc_position_offset + varying_position_size;
			varying_tc_value2_offset	= varying_tc_value1_offset + varying_value1_size;
			varying_te_position_offset  = varying_tc_value2_offset + varying_value2_size;
			varying_tc_pointSize_offset = varying_te_position_offset + varying_position_size;
			varying_te_pointSize_offset = varying_tc_pointSize_offset + varying_pointSize_size;
		}
		else
		{
			n_varyings	= sizeof(gs_non_point_size_varyings) / sizeof(gs_non_point_size_varyings[0]);
			varyings	  = gs_non_point_size_varyings;
			varyings_size = varying_position_size + /* gs_tc_position */
							varying_value1_size +   /* gs_tc_value1   */
							varying_value2_size +   /* gs_tc_value2   */
							varying_position_size;  /* gs_te_position */

			varying_tc_position_offset = 0;
			varying_tc_value1_offset   = varying_tc_position_offset + varying_position_size;
			varying_tc_value2_offset   = varying_tc_value1_offset + varying_value1_size;
			varying_te_position_offset = varying_tc_value2_offset + varying_value2_size;
		}
	} /* if (should_use_geometry_shader) */
	else
	{
		if (should_pass_point_size_data_in_ts)
		{
			n_varyings	= sizeof(non_gs_point_size_varyings) / sizeof(non_gs_point_size_varyings[0]);
			varyings	  = non_gs_point_size_varyings;
			varyings_size = varying_position_size +  /* tc_position   */
							varying_value1_size +	/* tc_value1     */
							varying_value2_size +	/* tc_value2     */
							varying_position_size +  /* te_position   */
							varying_pointSize_size + /* tc_pointSize  */
							varying_pointSize_size + /* te_pointSize  */
							varying_patch_data_size; /* tc_patch_data */

			varying_tc_position_offset   = 0;
			varying_tc_value1_offset	 = varying_tc_position_offset + varying_position_size;
			varying_tc_value2_offset	 = varying_tc_value1_offset + varying_value1_size;
			varying_te_position_offset   = varying_tc_value2_offset + varying_value2_size;
			varying_tc_pointSize_offset  = varying_te_position_offset + varying_position_size;
			varying_te_pointSize_offset  = varying_tc_pointSize_offset + varying_pointSize_size;
			varying_te_patch_data_offset = varying_te_pointSize_offset + varying_pointSize_size;
		}
		else
		{
			n_varyings	= sizeof(non_gs_non_point_size_varyings) / sizeof(non_gs_non_point_size_varyings[0]);
			varyings	  = non_gs_non_point_size_varyings;
			varyings_size = varying_position_size +  /* tc_position   */
							varying_value1_size +	/* tc_value1     */
							varying_value2_size +	/* tc_value2     */
							varying_position_size +  /* te_position   */
							varying_patch_data_size; /* tc_patch_data */

			varying_tc_position_offset   = 0;
			varying_tc_value1_offset	 = varying_tc_position_offset + varying_position_size;
			varying_tc_value2_offset	 = varying_tc_value1_offset + varying_value1_size;
			varying_te_position_offset   = varying_tc_value2_offset + varying_value2_size;
			varying_te_patch_data_offset = varying_te_position_offset + varying_position_size;
		}
	}

	gl.transformFeedbackVaryings(run.po_id, n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");

	/* Compile all the shader objects */
	const glw::GLuint  shaders[] = { run.fs_id, run.gs_id, run.tcs_id, run.tes_id, run.vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLuint shader = shaders[n_shader];

		if (shader != 0)
		{
			m_utils_ptr->compileShaders(1 /* n_shaders */, &shader, true);
		}
	}

	/* Link the program object */
	gl.linkProgram(run.po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed");

	/* Make sure the linking has succeeded */
	glw::GLint link_status = GL_FALSE;

	gl.getProgramiv(run.po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* Now that we have a linked program object, it's time to determine how much space
	 * we will need to hold XFB data.
	 */
	unsigned int bo_size			  = 0;
	unsigned int n_result_tess_coords = 0;
	const float  tess_levels[]		  = /* as per shaders constructed by the test */
		{ 4.0f, 4.0f, 4.0f, 4.0f };

	n_result_tess_coords = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
		run.primitive_mode, tess_levels, tess_levels, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
		true); /* is_point_mode_enabled */

	if (should_use_geometry_shader)
	{
		/* Geometry shader will output twice as many vertices */
		n_result_tess_coords *= 2;
	}

	run.n_result_vertices_per_patch = n_result_tess_coords;
	n_result_tess_coords *= m_n_input_vertices_per_run;
	bo_size = n_result_tess_coords * varyings_size;

	/* Proceed with buffer object storage allocation */
	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed");

	/* Great, time to actually render the data! */
	glw::GLenum tf_mode =
		TessellationShaderUtils::getTFModeForPrimitiveMode(run.primitive_mode, true); /* is_point_mode_enabled */

	gl.useProgram(run.po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed");

	gl.beginTransformFeedback(tf_mode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() failed");
	{
		gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, m_n_input_vertices_per_run);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

	/* The data should have landed in the buffer object storage by now. Map the BO into
	 * process space. */
	const void* bo_ptr = gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
										   bo_size, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() failed");

	/* Extract varyings' data */
	for (unsigned int n_tess_coord = 0; n_tess_coord < n_result_tess_coords; ++n_tess_coord)
	{
		const char* data = (const char*)bo_ptr + n_tess_coord * varyings_size;

		if (varying_tc_position_offset != -1)
		{
			const float* position_data((const float*)(data + varying_tc_position_offset));
			_vec4		 new_entry(position_data[0], position_data[1], position_data[2], position_data[3]);

			run.result_tc_position_data.push_back(new_entry);
		}

		if (varying_tc_value1_offset != -1)
		{
			const float* value1_data((const float*)(data + varying_tc_value1_offset));
			_vec4		 new_entry(value1_data[0], value1_data[1], value1_data[2], value1_data[3]);

			run.result_tc_value1_data.push_back(new_entry);
		}

		if (varying_tc_value2_offset != -1)
		{
			const int* value2_data((const int*)(data + varying_tc_value2_offset));
			_ivec4	 new_entry(value2_data[0], value2_data[1], value2_data[2], value2_data[3]);

			run.result_tc_value2_data.push_back(new_entry);
		}

		if (varying_te_position_offset != -1)
		{
			const float* position_data((const float*)(data + varying_te_position_offset));
			_vec4		 new_entry(position_data[0], position_data[1], position_data[2], position_data[3]);

			run.result_te_position_data.push_back(new_entry);
		}

		if (varying_tc_pointSize_offset != -1)
		{
			const float* pointSize_ptr((const float*)(data + varying_tc_pointSize_offset));

			run.result_tc_pointSize_data.push_back(*pointSize_ptr);
		}

		if (varying_te_pointSize_offset != -1)
		{
			const float* pointSize_ptr((const float*)(data + varying_te_pointSize_offset));

			run.result_te_pointSize_data.push_back(*pointSize_ptr);
		}

		if (varying_te_patch_data_offset != -1)
		{
			const float* patch_data_ptr((const float*)(data + varying_te_patch_data_offset));
			_vec4		 new_entry(patch_data_ptr[0], patch_data_ptr[1], patch_data_ptr[2], patch_data_ptr[3]);

			run.result_te_patch_data.push_back(new_entry);
		}
	} /* for (all XFB data associated with tessellated coordinates) */

	/* Now that we're done extracting the data we need, we're fine to unmap the buffer object */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed");
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderTCTEDataPassThrough::iterate(void)
{
	const float epsilon = 1e-5f;

	/* Initialize ES test objects */
	initTest();

	/* Iterate over all runs */
	for (_runs_const_iterator run_iterator = m_runs.begin(); run_iterator != m_runs.end(); ++run_iterator)
	{
		const _run& run = *run_iterator;

		/* Check result tc_pointSize data if available */
		unsigned int n_vertex = 0;

		for (std::vector<glw::GLfloat>::const_iterator data_iterator = run.result_tc_pointSize_data.begin();
			 data_iterator != run.result_tc_pointSize_data.end(); data_iterator++, n_vertex++)
		{
			const glw::GLfloat data			  = *data_iterator;
			unsigned int	   vertex_id	  = n_vertex / run.n_result_vertices_per_patch;
			float			   expected_value = 1.0f / (float(vertex_id) + 1.0f);

			if (run.gs_id != 0 && (n_vertex % 2) != 0)
			{
				/* Odd vertices emitted by geometry shader add 1 to all components */
				expected_value += 1.0f;
			}

			if (de::abs(data - expected_value) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid tc_pointSize value found at index [" << n_vertex
								   << "];"
									  " expected:["
								   << expected_value << "], "
														" found:["
								   << data << "]." << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid tc_pointSize value found");
			}
		}

		/* Check result tc_position data if available */
		n_vertex -= n_vertex;

		for (std::vector<_vec4>::const_iterator data_iterator = run.result_tc_position_data.begin();
			 data_iterator != run.result_tc_position_data.end(); data_iterator++, n_vertex++)
		{
			const _vec4& data			= *data_iterator;
			float		 expected_value = (float)(n_vertex / run.n_result_vertices_per_patch);

			if (run.gs_id != 0 && (n_vertex % 2) != 0)
			{
				/* Odd vertices emitted by geometry shader add 1 to all components */
				expected_value += 1.0f;
			}

			if (de::abs(data.x - expected_value) > epsilon || de::abs(data.y - expected_value) > epsilon ||
				de::abs(data.z - expected_value) > epsilon || de::abs(data.w - expected_value) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid tc_position value found at index [" << n_vertex
								   << "];"
									  " expected:"
									  " ["
								   << expected_value << ", " << expected_value << ", " << expected_value << ", "
								   << expected_value << "], found:"
														" ["
								   << data.x << ", " << data.y << ", " << data.z << ", " << data.w << "]."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid tc_position value found");
			}
		}

		/* Check result tc_value1 data if available */
		n_vertex -= n_vertex;

		for (std::vector<_vec4>::const_iterator data_iterator = run.result_tc_value1_data.begin();
			 data_iterator != run.result_tc_value1_data.end(); data_iterator++, n_vertex++)
		{
			const _vec4& data			= *data_iterator;
			unsigned int vertex_id		= n_vertex / run.n_result_vertices_per_patch;
			_vec4		 expected_value = _vec4((float)vertex_id, ((float)vertex_id) * 0.5f, ((float)vertex_id) * 0.25f,
										 ((float)vertex_id) * 0.125f);

			/* TE uses an even vertex outputted by TC, so we need
			 * to multiply the expected value by 2.
			 */
			expected_value.x *= 2.0f;
			expected_value.y *= 2.0f;
			expected_value.z *= 2.0f;
			expected_value.w *= 2.0f;

			if (run.gs_id != 0 && (n_vertex % 2) != 0)
			{
				/* Odd vertices emitted by geometry shader add 1 to all components */
				expected_value.x += 1.0f;
				expected_value.y += 1.0f;
				expected_value.z += 1.0f;
				expected_value.w += 1.0f;
			}

			if (de::abs(data.x - expected_value.x) > epsilon || de::abs(data.y - expected_value.y) > epsilon ||
				de::abs(data.z - expected_value.z) > epsilon || de::abs(data.w - expected_value.w) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid tc_value1 value found at index [" << n_vertex
								   << "];"
									  " expected:"
									  " ["
								   << expected_value.x << ", " << expected_value.y << ", " << expected_value.z << ", "
								   << expected_value.w << "], found:"
														  " ["
								   << data.x << ", " << data.y << ", " << data.z << ", " << data.w << "]."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid tc_value1 value found");
			}
		}

		/* Check result tc_value2 data if available */
		n_vertex -= n_vertex;

		for (std::vector<_ivec4>::const_iterator data_iterator = run.result_tc_value2_data.begin();
			 data_iterator != run.result_tc_value2_data.end(); data_iterator++, n_vertex++)
		{
			const _ivec4& data			 = *data_iterator;
			unsigned int  vertex_id		 = n_vertex / run.n_result_vertices_per_patch;
			_ivec4		  expected_value = _ivec4(vertex_id, vertex_id + 1, vertex_id + 2, vertex_id + 3);

			if (run.gs_id != 0 && (n_vertex % 2) != 0)
			{
				/* Odd vertices emitted by geometry shader add 1 to all components */
				expected_value.x++;
				expected_value.y++;
				expected_value.z++;
				expected_value.w++;
			}

			if (data.x != expected_value.x || data.y != expected_value.y || data.z != expected_value.z ||
				data.w != expected_value.w)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid tc_value2 value found at index [" << n_vertex
								   << "];"
									  " expected:"
									  " ["
								   << expected_value.x << ", " << expected_value.y << ", " << expected_value.z << ", "
								   << expected_value.w << "], found:"
														  " ["
								   << data.x << ", " << data.y << ", " << data.z << ", " << data.w << "]."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid tc_value2 value found");
			}
		}

		/* Check result te_pointSize data if available */
		n_vertex -= n_vertex;

		for (std::vector<glw::GLfloat>::const_iterator data_iterator = run.result_te_pointSize_data.begin();
			 data_iterator != run.result_te_pointSize_data.end(); data_iterator++, n_vertex++)
		{
			const glw::GLfloat data			  = *data_iterator;
			unsigned int	   vertex_id	  = n_vertex / run.n_result_vertices_per_patch;
			float			   expected_value = 2.0f / (float(vertex_id) + 1.0f);

			if (run.gs_id != 0 && (n_vertex % 2) != 0)
			{
				/* Odd vertices emitted by geometry shader add 1 to all components */
				expected_value += 1.0f;
			}

			if (de::abs(data - expected_value) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid te_pointSize value found at index [" << n_vertex
								   << "];"
									  " expected:["
								   << expected_value << "], "
														" found:["
								   << data << "]." << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid te_pointSize value found");
			}
		}

		/* Check result te_position data if available */
		n_vertex -= n_vertex;

		for (std::vector<_vec4>::const_iterator data_iterator = run.result_te_position_data.begin();
			 data_iterator != run.result_te_position_data.end(); data_iterator++, n_vertex++)
		{
			const _vec4& data			= *data_iterator;
			float		 expected_value = (float)(n_vertex / run.n_result_vertices_per_patch);

			/* te_position should be equal to tc_position, with 3 added to all components */
			expected_value += 3.0f;

			if (run.gs_id != 0 && (n_vertex % 2) != 0)
			{
				/* Odd vertices emitted by geometry shader add 1 to all components */
				expected_value += 1.0f;
			}

			if (de::abs(data.x - expected_value) > epsilon || de::abs(data.y - expected_value) > epsilon ||
				de::abs(data.z - expected_value) > epsilon || de::abs(data.w - expected_value) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid te_position value found at index [" << n_vertex
								   << "];"
									  " expected:"
									  " ["
								   << expected_value << ", " << expected_value << ", " << expected_value << ", "
								   << expected_value << "], found:"
														" ["
								   << data.x << ", " << data.y << ", " << data.z << ", " << data.w << "]."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid te_position value found");
			}
		}

		/* Check result tc_patch_data data if available */
		n_vertex -= n_vertex;

		for (std::vector<_vec4>::const_iterator data_iterator = run.result_te_patch_data.begin();
			 data_iterator != run.result_te_patch_data.end(); data_iterator++, n_vertex++)
		{
			const _vec4& data			= *data_iterator;
			unsigned int vertex_id		= n_vertex / run.n_result_vertices_per_patch;
			_vec4		 expected_value = _vec4((float)vertex_id, ((float)vertex_id) * 0.5f, ((float)vertex_id) * 0.25f,
										 ((float)vertex_id) * 0.125f);

			/* TE uses an even vertex outputted by TC, so we need
			 * to multiply the expected value by 2.
			 */
			expected_value.x *= 2.0f;
			expected_value.y *= 2.0f;
			expected_value.z *= 2.0f;
			expected_value.w *= 2.0f;

			if (de::abs(data.x - expected_value.x) > epsilon || de::abs(data.y - expected_value.y) > epsilon ||
				de::abs(data.z - expected_value.z) > epsilon || de::abs(data.w - expected_value.w) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid tc_patch_data value found at index ["
								   << n_vertex << "];"
												  " expected:"
												  " ["
								   << expected_value.x << ", " << expected_value.y << ", " << expected_value.z << ", "
								   << expected_value.w << "], found:"
														  " ["
								   << data.x << ", " << data.y << ", " << data.z << ", " << data.w << "]."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid tc_patch_data value found");
			}
		}
	} /* for (all runs) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTCTEgl_in::TessellationShaderTCTEgl_in(Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "gl_in", "Verifies values of gl_in[] in a tessellation evaluation shader "
												"are taken from output variables of a tessellation control shader"
												"if one is present.")
	, m_bo_id(0)
	, m_fs_id(0)
	, m_po_id(0)
	, m_tcs_id(0)
	, m_tes_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderTCTEgl_in::deinit()
{
	/** Call base class' deinit() function */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Revert TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Reset GL_PATCH_VERTICES_EXT value to the default setting */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Disable GL_RASTERIZER_DISCARD mdoe */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Release all objects we might've created */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tcs_id != 0)
	{
		gl.deleteShader(m_tcs_id);

		m_tcs_id = 0;
	}

	if (m_tes_id != 0)
	{
		gl.deleteShader(m_tes_id);

		m_tes_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Initializes all ES objects that will be used for the test. */
void TessellationShaderTCTEgl_in::initTest()
{
	/* The test requires EXT_tessellation_shader */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Generate a program object we will later configure */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Create program object */
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Generate shader objects the test will use */
	m_fs_id  = gl.createShader(GL_FRAGMENT_SHADER);
	m_tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	m_tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	m_vs_id  = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Configure fragment shader */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(m_fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader object");

	/* Configure tessellation control shader */
	const char* tc_body = "${VERSION}\n"
						  "\n"
						  /* Required EXT_tessellation_shader functionality */
						  "${TESSELLATION_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (vertices = 1) out;\n"
						  "\n"
						  "out float out_float[];\n"
						  "out int   out_int[];\n"
						  "out ivec3 out_ivec3[];\n"
						  "out mat2  out_mat2[];\n"
						  "out uint  out_uint[];\n"
						  "out uvec2 out_uvec2[];\n"
						  "out vec4  out_vec4[];\n"
						  "\n"
						  "out struct\n"
						  "{\n"
						  "    int   test1;\n"
						  "    float test2;\n"
						  "} out_struct[];\n"
						  /* Body */
						  "void main()\n"
						  "{\n"
						  "    gl_out           [gl_InvocationID].gl_Position = vec4(5.0, 6.0, 7.0, 8.0);\n"
						  "    gl_TessLevelOuter[0]                           = 1.0;\n"
						  "    gl_TessLevelOuter[1]                           = 1.0;\n"
						  "\n"
						  "    out_float[gl_InvocationID] = 22.0;\n"
						  "    out_int  [gl_InvocationID] = 23;\n"
						  "    out_ivec3[gl_InvocationID] = ivec3(24, 25, 26);\n"
						  "    out_mat2 [gl_InvocationID] = mat2(vec2(27.0, 28.0), vec2(29.0, 30.0) );\n"
						  "    out_uint [gl_InvocationID] = 31u;\n"
						  "    out_uvec2[gl_InvocationID] = uvec2(32, 33);\n"
						  "    out_vec4 [gl_InvocationID] = vec4(34.0, 35.0, 36.0, 37.0);\n"
						  "\n"
						  "    out_struct[gl_InvocationID].test1 = 38;\n"
						  "    out_struct[gl_InvocationID].test2 = 39.0;\n"
						  "}\n";

	shaderSourceSpecialized(m_tcs_id, 1 /* count */, &tc_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation control shader object");

	/* Configure tessellation evaluation shader */
	const char* te_body = "${VERSION}\n"
						  "\n"
						  "${TESSELLATION_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (isolines, point_mode) in;\n"
						  "\n"
						  "in float out_float[];\n"
						  "in int   out_int[];\n"
						  "in ivec3 out_ivec3[];\n"
						  "in mat2  out_mat2[];\n"
						  "in uint  out_uint[];\n"
						  "in uvec2 out_uvec2[];\n"
						  "in vec4  out_vec4[];\n"
						  "in struct\n"
						  "{\n"
						  "    int   test1;\n"
						  "    float test2;\n"
						  "} out_struct[];\n"
						  "\n"
						  "out float result_float;\n"
						  "flat out int   result_int;\n"
						  "flat out ivec3 result_ivec3;\n"
						  "out mat2  result_mat2;\n"
						  "flat out int   result_struct_test1;\n"
						  "out float result_struct_test2;\n"
						  "flat out uint  result_uint;\n"
						  "flat out uvec2 result_uvec2;\n"
						  "out vec4  result_vec4;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = gl_in[0].gl_Position;\n"
						  "\n"
						  "    result_float        = out_float [0];\n"
						  "    result_int          = out_int   [0];\n"
						  "    result_ivec3        = out_ivec3 [0];\n"
						  "    result_mat2         = out_mat2  [0];\n"
						  "    result_struct_test1 = out_struct[0].test1;\n"
						  "    result_struct_test2 = out_struct[0].test2;\n"
						  "    result_uint         = out_uint  [0];\n"
						  "    result_uvec2        = out_uvec2 [0];\n"
						  "    result_vec4         = out_vec4  [0];\n"
						  "}\n";

	shaderSourceSpecialized(m_tes_id, 1 /* count */, &te_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation evaluation shader object");

	/* Configure vertex shader */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "${SHADER_IO_BLOCKS_ENABLE}\n"
						  "\n"
						  "out float out_float;\n"
						  "flat out int   out_int;\n"
						  "flat out ivec3 out_ivec3;\n"
						  "out mat2  out_mat2;\n"
						  "flat out uint  out_uint;\n"
						  "flat out uvec2 out_uvec2;\n"
						  "out vec4  out_vec4;\n"
						  "\n"
						  "flat out struct\n"
						  "{\n"
						  "    int   test1;\n"
						  "    float test2;\n"
						  "} out_struct;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(1.0, 2.0, 3.0, 4.0);\n"
						  "\n"
						  "    out_float        = 1.0;\n"
						  "    out_int          = 2;\n"
						  "    out_ivec3        = ivec3(3, 4, 5);\n"
						  "    out_mat2         = mat2(vec2(6.0, 7.0), vec2(8.0, 9.0) );\n"
						  "    out_uint         = 10u;\n"
						  "    out_uvec2        = uvec2(11u, 12u);\n"
						  "    out_vec4         = vec4(12.0, 13.0, 14.0, 15.0);\n"
						  "    out_struct.test1 = 20;\n"
						  "    out_struct.test2 = 21.0;\n"
						  "}\n";

	shaderSourceSpecialized(m_vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader object");

	/* Compile all shaders of our interest */
	const glw::GLuint  shaders[] = { m_fs_id, m_tcs_id, m_tes_id, m_vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLint  compile_status = GL_FALSE;
		glw::GLuint shader		   = shaders[n_shader];

		gl.compileShader(shader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

		gl.getShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

		if (compile_status != GL_TRUE)
		{
			const char* src[] = { fs_body, tc_body, te_body, vs_body };
			m_testCtx.getLog() << tcu::TestLog::Message << "Compilation of shader object at index " << n_shader
							   << " failed.\n"
							   << "Info log:\n"
							   << getCompilationInfoLog(shader) << "Shader:\n"
							   << src[n_shader] << tcu::TestLog::EndMessage;

			TCU_FAIL("Shader compilation failed");
		}
	} /* for (all shaders) */

	/* Attach the shaders to the test program object, set up XFB and then link the program */
	glw::GLint			link_status	= GL_FALSE;
	glw::GLint			n_xfb_varyings = 0;
	const glw::GLchar** xfb_varyings   = NULL;
	glw::GLint			xfb_size	   = 0;

	getXFBProperties(&xfb_varyings, &n_xfb_varyings, &xfb_size);

	gl.transformFeedbackVaryings(m_po_id, n_xfb_varyings, xfb_varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");

	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_tcs_id);
	gl.attachShader(m_po_id, m_tes_id);
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* Generate and set up a buffer object we will use to hold XFBed data. */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_size, NULL /* data */, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed");

	/* We're good to execute the test! */
}

/** Retrieves XFB-specific properties that are used in various locations of
 *  this test implementation.
 *
 *  @param out_names    Deref will be used to store location of an array keeping
 *                      names of varyings that should be used for TF. Can be NULL,
 *                      in which case nothing will be stored under *out_names.
 *  @param out_n_names  Deref will be used to store number of strings the @param
 *                      out_names array holds. Can be NULL, in which case nothing
 *                      will be stored under *out_n_names.
 *  @param out_xfb_size Deref will be used to store amount of bytes needed to hold
 *                      all data generated by a draw call used by this test. Can be
 *                      NULL, in which case nothing will be stored under *out_xfb_size.
 **/
void TessellationShaderTCTEgl_in::getXFBProperties(const glw::GLchar*** out_names, glw::GLint* out_n_names,
												   glw::GLint* out_xfb_size)
{
	static const glw::GLchar* xfb_varyings[] = { "result_float", "result_int",			"result_ivec3",
												 "result_mat2",  "result_struct_test1", "result_struct_test2",
												 "result_uint",  "result_uvec2",		"result_vec4",
												 "gl_Position" };
	static const unsigned int xfb_size = (sizeof(float) +	  /* result_float */
										  sizeof(int) +		   /* result_int */
										  sizeof(int) * 3 +	/* result_ivec3 */
										  sizeof(float) * 4 +  /* result_mat2 */
										  sizeof(int) +		   /* result_struct_test1 */
										  sizeof(float) +	  /* result_struct_test2 */
										  sizeof(int) +		   /* result_uint */
										  sizeof(int) * 2 +	/* result_uvec2 */
										  sizeof(float) * 4 +  /* result_vec4 */
										  sizeof(float) * 4) * /* gl_Position */
										 2;					   /* two points will be generated by tessellation */

	static const unsigned int n_xfb_varyings = sizeof(xfb_varyings) / sizeof(xfb_varyings[0]);

	if (out_names != NULL)
	{
		*out_names = xfb_varyings;
	}

	if (out_n_names != NULL)
	{
		*out_n_names = n_xfb_varyings;
	}

	if (out_xfb_size != NULL)
	{
		/* NOTE: Tessellator is expected to generate two points for the purpose of
		 *       this test, which is why we need to multiply the amount of bytes store
		 *       in xfb_size by two.
		 */
		*out_xfb_size = xfb_size * 2;
	}
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderTCTEgl_in::iterate(void)
{
	/* Initialize ES test objects */
	initTest();

	/* Our program object takes a single vertex per patch */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() call failed");

	/* Render the geometry. We're only interested in XFB data, not the visual outcome,
	 * so disable rasterization before we fire a draw call.
	 */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback(GL_POINTS) call failed");
	{
		gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, 1 /* count */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

	/* Download the data we stored with TF */
	glw::GLint			n_xfb_names   = 0;
	void*				rendered_data = NULL;
	const glw::GLchar** xfb_names	 = NULL;
	glw::GLint			xfb_size	  = 0;

	getXFBProperties(&xfb_names, &n_xfb_names, &xfb_size);

	rendered_data = gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, xfb_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	/* Move through the result buffer and make sure the values we retrieved are valid.
	 * Note that two points will be generated by the tessellator, so run the checks
	 * twice.
	 */
	typedef enum {
		XFB_VARYING_TYPE_FLOAT,
		XFB_VARYING_TYPE_INT,

		XFB_VARYING_TYPE_UNKNOWN
	} _xfb_varying_type;

	unsigned char* traveller_ptr = (unsigned char*)rendered_data;

	for (glw::GLint n_point = 0; n_point < 2 /* points */; ++n_point)
	{
		for (glw::GLint n_xfb_name = 0; n_xfb_name < n_xfb_names; ++n_xfb_name)
		{
			glw::GLfloat	  expected_value_float[4] = { 0.0f };
			glw::GLint		  expected_value_int[4]   = { 0 };
			std::string		  name					  = xfb_names[n_xfb_name];
			unsigned int	  n_varying_components	= 0;
			_xfb_varying_type varying_type			  = XFB_VARYING_TYPE_UNKNOWN;

			if (name.compare("result_float") == 0)
			{
				expected_value_float[0] = 22.0f;
				n_varying_components	= 1;
				varying_type			= XFB_VARYING_TYPE_FLOAT;
			}
			else if (name.compare("result_int") == 0)
			{
				expected_value_int[0] = 23;
				n_varying_components  = 1;
				varying_type		  = XFB_VARYING_TYPE_INT;
			}
			else if (name.compare("result_ivec3") == 0)
			{
				expected_value_int[0] = 24;
				expected_value_int[1] = 25;
				expected_value_int[2] = 26;
				n_varying_components  = 3;
				varying_type		  = XFB_VARYING_TYPE_INT;
			}
			else if (name.compare("result_mat2") == 0)
			{
				expected_value_float[0] = 27.0f;
				expected_value_float[1] = 28.0f;
				expected_value_float[2] = 29.0f;
				expected_value_float[3] = 30.0f;
				n_varying_components	= 4;
				varying_type			= XFB_VARYING_TYPE_FLOAT;
			}
			else if (name.compare("result_struct_test1") == 0)
			{
				expected_value_int[0] = 38;
				n_varying_components  = 1;
				varying_type		  = XFB_VARYING_TYPE_INT;
			}
			else if (name.compare("result_struct_test2") == 0)
			{
				expected_value_float[0] = 39.0f;
				n_varying_components	= 1;
				varying_type			= XFB_VARYING_TYPE_FLOAT;
			}
			else if (name.compare("result_uint") == 0)
			{
				expected_value_int[0] = 31;
				n_varying_components  = 1;
				varying_type		  = XFB_VARYING_TYPE_INT;
			}
			else if (name.compare("result_uvec2") == 0)
			{
				expected_value_int[0] = 32;
				expected_value_int[1] = 33;
				n_varying_components  = 2;
				varying_type		  = XFB_VARYING_TYPE_INT;
			}
			else if (name.compare("result_vec4") == 0)
			{
				expected_value_float[0] = 34.0f;
				expected_value_float[1] = 35.0f;
				expected_value_float[2] = 36.0f;
				expected_value_float[3] = 37.0f;
				n_varying_components	= 4;
				varying_type			= XFB_VARYING_TYPE_FLOAT;
			}
			else if (name.compare("gl_Position") == 0)
			{
				expected_value_float[0] = 5.0f;
				expected_value_float[1] = 6.0f;
				expected_value_float[2] = 7.0f;
				expected_value_float[3] = 8.0f;
				n_varying_components	= 4;
				varying_type			= XFB_VARYING_TYPE_FLOAT;
			}
			else
			{
				TCU_FAIL("Unrecognized XFB name");
			}

			/* Move through the requested amount of components and perform type-specific
			 * comparison.
			 */
			const float epsilon = (float)1e-5;

			for (unsigned int n_component = 0; n_component < n_varying_components; ++n_component)
			{
				switch (varying_type)
				{
				case XFB_VARYING_TYPE_FLOAT:
				{
					glw::GLfloat* rendered_value = (glw::GLfloat*)traveller_ptr;

					if (de::abs(*rendered_value - expected_value_float[n_component]) > epsilon)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Invalid component at index [" << n_component << "] "
							<< "(found:" << *rendered_value << " expected:" << expected_value_float[n_component]
							<< ") for varying [" << name.c_str() << "]" << tcu::TestLog::EndMessage;
					}

					traveller_ptr += sizeof(glw::GLfloat);

					break;
				}

				case XFB_VARYING_TYPE_INT:
				{
					glw::GLint* rendered_value = (glw::GLint*)traveller_ptr;

					if (*rendered_value != expected_value_int[n_component])
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Invalid component at index [" << n_component << "] "
							<< "(found:" << *rendered_value << " expected:" << expected_value_int[n_component]
							<< ") for varying [" << name.c_str() << "]" << tcu::TestLog::EndMessage;

						TCU_FAIL("Invalid rendered value");
					}

					traveller_ptr += sizeof(glw::GLint);

					break;
				}

				default:
				{
					TCU_FAIL("Unrecognized varying type");
				}
				} /* switch(varying_type) */

			} /* for (all components) */
		}	 /* for (all XFBed variables) */
	}		  /* for (both points) */

	/* Unmap the BO */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::
	TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize(Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "gl_MaxPatchVertices_Position_PointSize",
				   "Verifies gl_Position and gl_PointSize (if supported) "
				   "are set to correct values in TE stage. Checks if up to "
				   "gl_MaxPatchVertices input block values can be accessed "
				   "from TE stage. Also verifies if TC/TE stage properties "
				   "can be correctly queried for both regular and separate "
				   "program objects.")
	, m_bo_id(0)
	, m_gl_max_patch_vertices_value(0)
	, m_gl_max_tess_gen_level_value(0)
	, m_utils_ptr(DE_NULL)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::deinit()
{
	/** Call base class' deinit() function */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Revert TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Reset GL_PATCH_VERTICES_EXT value to the default setting */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Release all objects we might've created */
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

	if (m_utils_ptr != DE_NULL)
	{
		delete m_utils_ptr;

		m_utils_ptr = DE_NULL;
	}

	/* Release all test runs */
	for (_runs::iterator it = m_runs.begin(); it != m_runs.end(); ++it)
	{
		deinitTestRun(*it);
	}
	m_runs.clear();
}

/** Deinitializes all ES objects generated for a test run */
void TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::deinitTestRun(_run& run)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (run.fs_id != 0)
	{
		gl.deleteShader(run.fs_id);

		run.fs_id = 0;
	}

	if (run.fs_program_id != 0)
	{
		gl.deleteProgram(run.fs_program_id);

		run.fs_program_id = 0;
	}

	if (run.pipeline_object_id != 0)
	{
		gl.deleteProgramPipelines(1, &run.pipeline_object_id);

		run.pipeline_object_id = 0;
	}

	if (run.po_id != 0)
	{
		gl.deleteProgram(run.po_id);

		run.po_id = 0;
	}

	if (run.tc_id != 0)
	{
		gl.deleteShader(run.tc_id);

		run.tc_id = 0;
	}

	if (run.tc_program_id != 0)
	{
		gl.deleteProgram(run.tc_program_id);

		run.tc_program_id = 0;
	}

	if (run.te_id != 0)
	{
		gl.deleteShader(run.te_id);

		run.te_id = 0;
	}

	if (run.te_program_id != 0)
	{
		gl.deleteProgram(run.te_program_id);

		run.te_program_id = 0;
	}

	if (run.vs_id != 0)
	{
		gl.deleteShader(run.vs_id);

		run.vs_id = 0;
	}

	if (run.vs_program_id != 0)
	{
		gl.deleteProgram(run.vs_program_id);

		run.vs_program_id = 0;
	}
}

/** Retrieves a dummy fragment shader code to be used for forming program objects
 *  used by the test.
 *
 *  @return As per description.
 **/
std::string TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::getFragmentShaderCode(
	bool should_accept_pointsize_data)
{
	// Requires input to match previous stage's output
	std::stringstream result_code;

	result_code << "${VERSION}\n"
				   "\n"
				   "${SHADER_IO_BLOCKS_REQUIRE}\n"
				   "\n"
				   "precision highp float;\n"
				   "precision highp int;\n"
				   "out layout (location = 0) vec4 col;\n";

	if (should_accept_pointsize_data)
	{
		result_code << "in      float te_pointsize;\n";
	}

	result_code << "in       vec4 te_position;\n"
				   "in       vec2 te_value1;\n"
				   "in flat ivec4 te_value2;\n"
				   "\n"
				   "void main()\n"
				   "{\n"
				   "  col = vec4(1.0, 1.0, 1.0, 1.0);\n"
				   "}\n";

	return result_code.str();
}

/** Retrieves tessellation control shader source code, given user-provided arguments.
 *
 *  @param should_pass_pointsize_data true if Tessellation Control shader should configure
 *                                    gl_PointSize value. This should be only set to true
 *                                    if the tested ES implementation reports support of
 *                                    GL_EXT_tessellation_point_size extension.
 *  @param inner_tess_levels          Two FP values defining inner tessellation level values.
 *                                    Must not be NULL.
 *  @param outer_tess_levels          Four FP values defining outer tessellation level values.
 *                                    Must not be NULL.
 *
 *  @return As per description.
 **/
std::string TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::getTessellationControlShaderCode(
	bool should_pass_pointsize_data, const glw::GLfloat* inner_tess_levels, const glw::GLfloat* outer_tess_levels)
{
	std::stringstream result_code;

	result_code << "${VERSION}\n"
				   "\n"
				   "${TESSELLATION_SHADER_REQUIRE}\n";

	if (should_pass_pointsize_data)
	{
		result_code << "${TESSELLATION_POINT_SIZE_REQUIRE}\n";
	}

	result_code << "\n"
				   "layout(vertices = "
				<< m_gl_max_patch_vertices_value << ") out;\n"
													"\n";
	if (should_pass_pointsize_data)
	{
		result_code << "${IN_PER_VERTEX_DECL_ARRAY_POINT_SIZE}";
		result_code << "${OUT_PER_VERTEX_DECL_ARRAY_POINT_SIZE}";
	}
	else
	{
		result_code << "${IN_PER_VERTEX_DECL_ARRAY}";
		result_code << "${OUT_PER_VERTEX_DECL_ARRAY}";
	}
	result_code << "out OUT_TC\n"
				   "{\n"
				   "     vec2 value1;\n"
				   "    ivec4 value2;\n"
				   "} result[];\n"
				   "\n"
				   "void main()\n"
				   "{\n";

	if (should_pass_pointsize_data)
	{
		result_code << "    gl_out[gl_InvocationID].gl_PointSize = 1.0 / float(gl_InvocationID + 1);\n";
	}

	result_code << "    gl_out[gl_InvocationID].gl_Position =  vec4(      float(gl_InvocationID * 4 + 0),   "
				   "float(gl_InvocationID * 4 + 1),\n"
				   "                                                      float(gl_InvocationID * 4 + 2),   "
				   "float(gl_InvocationID * 4 + 3));\n"
				   "    result[gl_InvocationID].value1      =  vec2(1.0 / float(gl_InvocationID + 1), 1.0 / "
				   "float(gl_InvocationID + 2) );\n"
				   "    result[gl_InvocationID].value2      = ivec4(            gl_InvocationID + 1,              "
				   "gl_InvocationID + 2,\n"
				   "                                                            gl_InvocationID + 3,              "
				   "gl_InvocationID + 4);\n"
				   "\n"
				   "    gl_TessLevelInner[0] = float("
				<< inner_tess_levels[0] << ");\n"
										   "    gl_TessLevelInner[1] = float("
				<< inner_tess_levels[1] << ");\n"
										   "    gl_TessLevelOuter[0] = float("
				<< outer_tess_levels[0] << ");\n"
										   "    gl_TessLevelOuter[1] = float("
				<< outer_tess_levels[1] << ");\n"
										   "    gl_TessLevelOuter[2] = float("
				<< outer_tess_levels[2] << ");\n"
										   "    gl_TessLevelOuter[3] = float("
				<< outer_tess_levels[3] << ");\n"
										   "}\n";

	return result_code.str();
}

/** Retrieves tessellation evaluation shader source code, given user-provided arguments.
 *
 *  @param should_pass_pointsize_data true if Tessellation Evaluation shader should set
 *                                    gl_PointSize to the value set by Tessellation Control
 *                                    stage. This should be only set to true if the tested
 *                                    ES implementation reports support of GL_EXT_tessellation_point_size
 *                                    extension, and TC stage assigns a value to gl_PointSize.
 *  @param primitive_mode             Primitive mode to use for the stage.
 *  @param vertex_ordering            Vertex ordering to use for the stage.
 *  @param vertex_spacing             Vertex spacing to use for the stage.
 *  @param is_point_mode_enabled      true to make the TE stage work in point mode, false otherwise.
 *
 *  @return As per description.
 **/
std::string TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::getTessellationEvaluationShaderCode(
	bool should_pass_pointsize_data, _tessellation_primitive_mode primitive_mode,
	_tessellation_shader_vertex_ordering vertex_ordering, _tessellation_shader_vertex_spacing vertex_spacing,
	bool is_point_mode_enabled)
{
	std::stringstream result_sstream;
	std::string		  result;

	result_sstream << "${VERSION}\n"
					  "\n"
					  "${TESSELLATION_SHADER_REQUIRE}\n";

	if (should_pass_pointsize_data)
	{
		result_sstream << "${TESSELLATION_POINT_SIZE_REQUIRE}\n";
	}

	result_sstream << "\n"
					  "layout (TESSELLATOR_PRIMITIVE_MODE VERTEX_SPACING_MODE VERTEX_ORDERING POINT_MODE) in;\n"
					  "\n";
	if (should_pass_pointsize_data)
	{
		result_sstream << "${IN_PER_VERTEX_DECL_ARRAY_POINT_SIZE}";
		result_sstream << "${OUT_PER_VERTEX_DECL_POINT_SIZE}";
	}
	else
	{
		result_sstream << "${IN_PER_VERTEX_DECL_ARRAY}";
		result_sstream << "${OUT_PER_VERTEX_DECL}";
	}
	result_sstream << "in OUT_TC\n"
					  "{\n"
					  "     vec2 value1;\n"
					  "    ivec4 value2;\n"
					  "} tc_data[];\n"
					  "\n";

	if (should_pass_pointsize_data)
	{
		result_sstream << "out      float te_pointsize;\n";
	}

	result_sstream << "out       vec4 te_position;\n"
					  "out       vec2 te_value1;\n"
					  "out flat ivec4 te_value2;\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	if (should_pass_pointsize_data)
	{
		result_sstream << "    te_pointsize = 0.0;\n";
	}

	result_sstream << "    te_position  = vec4 (0.0);\n"
					  "    te_value1    = vec2 (0.0);\n"
					  "    te_value2    = ivec4(0);\n"
					  "\n"
					  "    for (int n = 0; n < "
				   << m_gl_max_patch_vertices_value << "; ++n)\n"
													   "    {\n";

	if (should_pass_pointsize_data)
	{
		result_sstream << "        te_pointsize += gl_in[n].gl_PointSize;\n";
	}

	result_sstream << "        te_position += gl_in  [n].gl_Position;\n"
					  "        te_value1   += tc_data[n].value1;\n"
					  "        te_value2   += tc_data[n].value2;\n"
					  "    }\n"
					  "}\n";

	result = result_sstream.str();

	/* Replace the tokens */
	const char* point_mode_token		   = "POINT_MODE";
	std::size_t point_mode_token_index	 = std::string::npos;
	std::string primitive_mode_string	  = TessellationShaderUtils::getESTokenForPrimitiveMode(primitive_mode);
	const char* primitive_mode_token	   = "TESSELLATOR_PRIMITIVE_MODE";
	std::size_t primitive_mode_token_index = std::string::npos;
	std::string vertex_ordering_string;
	const char* vertex_ordering_token		= "VERTEX_ORDERING";
	std::size_t vertex_ordering_token_index = std::string::npos;
	std::string vertex_spacing_mode_string;
	const char* vertex_spacing_token	   = "VERTEX_SPACING_MODE";
	std::size_t vertex_spacing_token_index = std::string::npos;

	/* Prepare the vertex ordering token. We need to do this manually, because the default vertex spacing
	 * mode translates to empty string and the shader would fail to compile if we hadn't taken care of the
	 * comma
	 */
	if (vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT)
	{
		vertex_ordering_string = TessellationShaderUtils::getESTokenForVertexOrderingMode(vertex_ordering);
	}
	else
	{
		std::stringstream helper_sstream;

		helper_sstream << ", " << TessellationShaderUtils::getESTokenForVertexOrderingMode(vertex_ordering);

		vertex_ordering_string = helper_sstream.str();
	}

	/* Do the same for vertex spacing token */
	if (vertex_spacing == TESSELLATION_SHADER_VERTEX_SPACING_DEFAULT)
	{
		vertex_spacing_mode_string = TessellationShaderUtils::getESTokenForVertexSpacingMode(vertex_spacing);
	}
	else
	{
		std::stringstream helper_sstream;

		helper_sstream << ", " << TessellationShaderUtils::getESTokenForVertexSpacingMode(vertex_spacing);

		vertex_spacing_mode_string = helper_sstream.str();
	}

	/* Primitive mode */
	while ((primitive_mode_token_index = result.find(primitive_mode_token)) != std::string::npos)
	{
		result = result.replace(primitive_mode_token_index, strlen(primitive_mode_token), primitive_mode_string);

		primitive_mode_token_index = result.find(primitive_mode_token);
	}

	/* Vertex ordering */
	while ((vertex_ordering_token_index = result.find(vertex_ordering_token)) != std::string::npos)
	{
		result = result.replace(vertex_ordering_token_index, strlen(vertex_ordering_token), vertex_ordering_string);

		vertex_ordering_token_index = result.find(vertex_ordering_token);
	}

	/* Vertex spacing */
	while ((vertex_spacing_token_index = result.find(vertex_spacing_token)) != std::string::npos)
	{
		result = result.replace(vertex_spacing_token_index, strlen(vertex_spacing_token), vertex_spacing_mode_string);

		vertex_spacing_token_index = result.find(vertex_spacing_token);
	}

	/* Point mode */
	while ((point_mode_token_index = result.find(point_mode_token)) != std::string::npos)
	{
		result = result.replace(point_mode_token_index, strlen(point_mode_token),
								(is_point_mode_enabled) ? ", point_mode" : "");

		point_mode_token_index = result.find(point_mode_token);
	}

	return result;
}

/** Retrieves a dummy vertex shader code to be used for forming program objects
 *  used by the test.
 *
 *  @return As per description.
 **/
std::string TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::getVertexShaderCode(
	bool should_pass_pointsize_data)
{
	std::stringstream result_sstream;
	result_sstream << "${VERSION}\n\n";
	if (should_pass_pointsize_data)
	{
		result_sstream << "${OUT_PER_VERTEX_DECL_POINT_SIZE}";
	}
	else
	{
		result_sstream << "${OUT_PER_VERTEX_DECL}";
	}
	result_sstream << "\n"
					  "void main()\n"
					  "{\n"
					  "}\n";

	return result_sstream.str();
}

/** Initializes all ES objects that will be used for the test. */
void TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::initTest()
{
	/* The test requires EXT_tessellation_shader */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Generate a buffer object we will use to hold XFB data */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	/* Configure XFB buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() / glBindBufferBase() call(s) failed");

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &m_gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Retrieve GL_MAX_PATCH_VERTICES_EXT value */
	gl.getIntegerv(m_glExtTokens.MAX_PATCH_VERTICES, &m_gl_max_patch_vertices_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_PATCH_VERTICES_EXT pname");

	/* We only need 1 vertex per input patch */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed for GL_PATCH_VERTICES_EXT pname");

	/* Disable rasterization */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed");

	/* Spawn utilities class instance */
	m_utils_ptr = new TessellationShaderUtils(gl, this);

	/* Initialize all test iterations */
	bool							   point_mode_enabled_flags[] = { false, true };
	const _tessellation_primitive_mode primitive_modes[]		  = { TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	const _tessellation_shader_vertex_ordering vertex_ordering_modes[] = {
		TESSELLATION_SHADER_VERTEX_ORDERING_CCW, TESSELLATION_SHADER_VERTEX_ORDERING_CW,
		TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT
	};
	const _tessellation_shader_vertex_spacing vertex_spacing_modes[] = {
		TESSELLATION_SHADER_VERTEX_SPACING_EQUAL, TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN,
		TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD, TESSELLATION_SHADER_VERTEX_SPACING_DEFAULT
	};
	const unsigned int n_point_mode_enabled_flags =
		sizeof(point_mode_enabled_flags) / sizeof(point_mode_enabled_flags[0]);
	const unsigned int n_primitive_modes	   = sizeof(primitive_modes) / sizeof(primitive_modes[0]);
	const unsigned int n_vertex_ordering_modes = sizeof(vertex_ordering_modes) / sizeof(vertex_ordering_modes[0]);
	const unsigned int n_vertex_spacing_modes  = sizeof(vertex_spacing_modes) / sizeof(vertex_spacing_modes[0]);

	bool deleteResources = false;

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; n_primitive_mode++)
	{
		_tessellation_primitive_mode primitive_mode  = primitive_modes[n_primitive_mode];
		_tessellation_levels_set tessellation_levels = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
			primitive_mode, m_gl_max_tess_gen_level_value, TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

		for (_tessellation_levels_set_const_iterator tessellation_levels_iterator = tessellation_levels.begin();
			 tessellation_levels_iterator != tessellation_levels.end(); tessellation_levels_iterator++)
		{
			const _tessellation_levels& tess_levels = *tessellation_levels_iterator;

			for (unsigned int n_vertex_ordering_mode = 0; n_vertex_ordering_mode < n_vertex_ordering_modes;
				 ++n_vertex_ordering_mode)
			{
				_tessellation_shader_vertex_ordering vertex_ordering = vertex_ordering_modes[n_vertex_ordering_mode];

				for (unsigned int n_vertex_spacing_mode = 0; n_vertex_spacing_mode < n_vertex_spacing_modes;
					 ++n_vertex_spacing_mode)
				{
					_tessellation_shader_vertex_spacing vertex_spacing = vertex_spacing_modes[n_vertex_spacing_mode];

					for (unsigned int n_point_mode_enabled_flag = 0;
						 n_point_mode_enabled_flag < n_point_mode_enabled_flags; ++n_point_mode_enabled_flag)
					{
						bool is_point_mode_enabled = point_mode_enabled_flags[n_point_mode_enabled_flag];

						/* Only create gl_PointSize-enabled runs if the implementation supports
						 * GL_EXT_tessellation_point_size extension
						 */
						if (!m_is_tessellation_shader_point_size_supported && is_point_mode_enabled)
						{
							continue;
						}

						/* Execute the test run */
						_run run;

						memcpy(run.inner, tess_levels.inner, sizeof(run.inner));
						memcpy(run.outer, tess_levels.outer, sizeof(run.outer));

						run.point_mode		= is_point_mode_enabled;
						run.primitive_mode  = primitive_mode;
						run.vertex_ordering = vertex_ordering;
						run.vertex_spacing  = vertex_spacing;

						initTestRun(run);

						if (deleteResources)
						{
							deinitTestRun(run);
						}

						deleteResources = true;

						/* Store it for further processing */
						m_runs.push_back(run);
					} /* for (all 'point mode' enabled flags) */
				}	 /* for (all vertex spacing modes) */
			}		  /* for (all vertex ordering modes) */
		}			  /* for (all tessellation levels for active primitive mode) */
	}				  /* for (all primitive modes) */
}

/** Initializes all ES objects used by the test, captures the tessellation coordinates
 *  and stores them in the descriptor.
 *  Also performs a handful of other minor checks, as described by test specification.
 *
 *  @param run Run descriptor to operate on.
 **/
void TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::initTestRun(_run& run)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Build shader objects */
	run.fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	run.tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	run.te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	run.vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed");

	/* Generate fragment shader (or stand-alone program) */
	std::string fs_code_string  = getFragmentShaderCode(run.point_mode);
	const char* fs_code_raw_ptr = fs_code_string.c_str();

	shaderSourceSpecialized(run.fs_id, 1 /* count */, &fs_code_raw_ptr);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for fragment shader");

	/* Generate tessellation control shader (or stand-alone program) */
	std::string tc_code_string  = getTessellationControlShaderCode(run.point_mode, run.inner, run.outer);
	const char* tc_code_raw_ptr = tc_code_string.c_str();

	shaderSourceSpecialized(run.tc_id, 1 /* count */, &tc_code_raw_ptr);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for tessellation control shader");

	/* Generate tessellation evaluation shader (or stand-alone program) */
	std::string te_code_string = getTessellationEvaluationShaderCode(
		run.point_mode, run.primitive_mode, run.vertex_ordering, run.vertex_spacing, run.point_mode);
	const char* te_code_raw_ptr = te_code_string.c_str();

	shaderSourceSpecialized(run.te_id, 1 /* count */, &te_code_raw_ptr);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for tessellation evaluation shader");

	/* Generate vertex shader (or stand-alone program) */
	std::string vs_code_string  = getVertexShaderCode(run.point_mode);
	const char* vs_code_raw_ptr = vs_code_string.c_str();

	shaderSourceSpecialized(run.vs_id, 1 /* count */, &vs_code_raw_ptr);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed for vertex shader");

	/* Compile all shaders first. Also make sure the shader objects we have
	 * attached are correctly reported.
	 */
	const glw::GLuint  shaders[] = { run.fs_id, run.tc_id, run.te_id, run.vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLint compile_status = GL_FALSE;
		glw::GLint shader		  = shaders[n_shader];

		gl.compileShader(shader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed");

		gl.getShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed");

		if (compile_status != GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Info log:\n"
							   << getCompilationInfoLog(shader) << "\nShader:\n"
							   << getShaderSource(shader) << tcu::TestLog::EndMessage;
			TCU_FAIL("Shader compilation failed");
		}
	}

	/* Run two iterations:
	 *
	 * 1) First, using a program object;
	 * 2) The other one using pipeline objects;
	 */
	for (unsigned int n_iteration = 0; n_iteration < 2 /* program / pipeline objects */; ++n_iteration)
	{
		bool should_use_program_object = (n_iteration == 0);

		/* Generate container object(s) first */
		if (!should_use_program_object)
		{
			gl.genProgramPipelines(1, &run.pipeline_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() failed");

			/* As per test spec, make sure no tessellation stages are defined for
			 * a pipeline object by default */
			glw::GLint program_tc_id = 1;
			glw::GLint program_te_id = 1;

			gl.getProgramPipelineiv(run.pipeline_object_id, m_glExtTokens.TESS_CONTROL_SHADER, &program_tc_id);
			gl.getProgramPipelineiv(run.pipeline_object_id, m_glExtTokens.TESS_EVALUATION_SHADER, &program_te_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv() failed");

			if (program_tc_id != 0 || program_te_id != 0)
			{
				GLU_EXPECT_NO_ERROR(gl.getError(), "A pipeline object returned a non-zero ID of "
												   "a separate program object when asked for TC/TE"
												   " program ID.");
			}
		}
		else
		{
			run.po_id = gl.createProgram();
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");
		}

		if (!should_use_program_object)
		{
			run.fs_program_id = gl.createProgram();
			run.tc_program_id = gl.createProgram();
			run.te_program_id = gl.createProgram();
			run.vs_program_id = gl.createProgram();

			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call(s) failed");
		}

		/* Link program object(s) (and configure the pipeline object, if necessary) */
		const glw::GLuint programs_for_pipeline_iteration[] = { run.fs_program_id, run.tc_program_id, run.te_program_id,
																run.vs_program_id };
		const glw::GLuint  programs_for_program_iteration[] = { run.po_id };
		const unsigned int n_programs_for_pipeline_iteration =
			sizeof(programs_for_pipeline_iteration) / sizeof(programs_for_pipeline_iteration[0]);
		const unsigned int n_programs_for_program_iteration =
			sizeof(programs_for_program_iteration) / sizeof(programs_for_program_iteration[0]);

		unsigned int	   n_programs				 = 0;
		const glw::GLuint* programs					 = DE_NULL;
		int				   xfb_pointsize_data_offset = -1;
		int				   xfb_position_data_offset  = -1;
		int				   xfb_value1_data_offset	= -1;
		int				   xfb_value2_data_offset	= -1;
		int				   xfb_varyings_size		 = 0;

		if (should_use_program_object)
		{
			n_programs = n_programs_for_program_iteration;
			programs   = programs_for_program_iteration;
		}
		else
		{
			n_programs = n_programs_for_pipeline_iteration;
			programs   = programs_for_pipeline_iteration;
		}

		/* Attach and verify shader objects */
		for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
		{
			glw::GLuint parent_po_id = 0;
			glw::GLuint shader		 = shaders[n_shader];

			if (should_use_program_object)
			{
				gl.attachShader(run.po_id, shader);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");

				parent_po_id = run.po_id;
			}
			else
			{
				if (shader == run.fs_id)
				{
					gl.attachShader(run.fs_program_id, run.fs_id);

					parent_po_id = run.fs_program_id;
				}
				else if (shader == run.tc_id)
				{
					gl.attachShader(run.tc_program_id, run.tc_id);

					parent_po_id = run.tc_program_id;
				}
				else if (shader == run.te_id)
				{
					gl.attachShader(run.te_program_id, run.te_id);

					parent_po_id = run.te_program_id;
				}
				else
				{
					DE_ASSERT(shader == run.vs_id);

					gl.attachShader(run.vs_program_id, run.vs_id);

					parent_po_id = run.vs_program_id;
				}

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");
			}

			/* Make sure the shader object we've attached is reported as a part
			 * of the program object.
			 */
			unsigned int attached_shaders[n_shaders] = { 0 };
			bool		 has_found_attached_shader   = false;
			glw::GLsizei n_attached_shaders			 = 0;

			memset(attached_shaders, 0, sizeof(attached_shaders));

			gl.getAttachedShaders(parent_po_id, n_shaders, &n_attached_shaders, attached_shaders);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttachedShaders() failed");

			for (glw::GLsizei n_attached_shader = 0; n_attached_shader < n_attached_shaders; n_attached_shader++)
			{
				if (attached_shaders[n_attached_shader] == shader)
				{
					has_found_attached_shader = true;

					break;
				}
			} /* for (all attached shader object IDs) */

			if (!has_found_attached_shader)
			{
				TCU_FAIL("A shader object that was successfully attached to a program "
						 "object was not reported as one by subsequent glGetAttachedShaders() "
						 "call");
			}
		}

		/* Set up XFB */
		const char* xfb_varyings_w_pointsize[]  = { "te_position", "te_value1", "te_value2", "te_pointsize" };
		const char* xfb_varyings_wo_pointsize[] = {
			"te_position", "te_value1", "te_value2",
		};
		const char** xfb_varyings   = DE_NULL;
		unsigned int n_xfb_varyings = 0;

		if (run.point_mode)
		{
			xfb_varyings   = xfb_varyings_w_pointsize;
			n_xfb_varyings = sizeof(xfb_varyings_w_pointsize) / sizeof(xfb_varyings_w_pointsize[0]);

			xfb_position_data_offset = 0;
			xfb_value1_data_offset =
				static_cast<unsigned int>(xfb_position_data_offset + sizeof(float) * 4); /* size of te_position */
			xfb_value2_data_offset =
				static_cast<unsigned int>(xfb_value1_data_offset + sizeof(float) * 2); /* size of te_value1 */
			xfb_pointsize_data_offset =
				static_cast<unsigned int>(xfb_value2_data_offset + sizeof(int) * 4); /* size of te_value2 */

			xfb_varyings_size = sizeof(float) * 4 + /* size of te_position */
								sizeof(float) * 2 + /* size of te_value1 */
								sizeof(int) * 4 +   /* size of te_value2 */
								sizeof(int);		/* size of te_pointsize */
		}
		else
		{
			xfb_varyings   = xfb_varyings_wo_pointsize;
			n_xfb_varyings = sizeof(xfb_varyings_wo_pointsize) / sizeof(xfb_varyings_wo_pointsize[0]);

			xfb_position_data_offset = 0;
			xfb_value1_data_offset =
				static_cast<unsigned int>(xfb_position_data_offset + sizeof(float) * 4); /* size of te_position */
			xfb_value2_data_offset =
				static_cast<unsigned int>(xfb_value1_data_offset + sizeof(float) * 2); /* size of te_value1 */

			xfb_varyings_size = sizeof(float) * 4 + /* size of te_position */
								sizeof(float) * 2 + /* size of te_value1 */
								sizeof(int) * 4;
		}

		if (!should_use_program_object)
		{
			gl.transformFeedbackVaryings(run.te_program_id, n_xfb_varyings, xfb_varyings, GL_INTERLEAVED_ATTRIBS);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed");
		}
		else
		{
			gl.transformFeedbackVaryings(run.po_id, n_xfb_varyings, xfb_varyings, GL_INTERLEAVED_ATTRIBS);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed");
		}

		/* Mark all program objects as separable for pipeline run */
		if (!should_use_program_object)
		{
			for (unsigned int n_program = 0; n_program < n_programs; ++n_program)
			{
				glw::GLuint program = programs[n_program];

				gl.programParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramParameteri() failed.");
			}
		}

		/* Link the program object(s) */
		for (unsigned int n_program = 0; n_program < n_programs; ++n_program)
		{
			glw::GLint  link_status = GL_FALSE;
			glw::GLuint program		= programs[n_program];

			gl.linkProgram(program);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Program linking failed");

			gl.getProgramiv(program, GL_LINK_STATUS, &link_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

			if (link_status != GL_TRUE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Info log:\n"
								   << getLinkingInfoLog(program) << tcu::TestLog::EndMessage;
				TCU_FAIL("Program linking failed");
			}

			/* Make sure glGetProgramiv() reports correct tessellation properties for
			 * the program object we've just linked successfully */
			if (program == run.po_id || program == run.tc_program_id || program == run.te_program_id)
			{
				glw::GLenum expected_tess_gen_mode_value		 = GL_NONE;
				glw::GLenum expected_tess_gen_spacing_value		 = GL_NONE;
				glw::GLenum expected_tess_gen_vertex_order_value = GL_NONE;
				glw::GLint  tess_control_output_vertices_value   = GL_NONE;
				glw::GLint  tess_gen_mode_value					 = GL_NONE;
				glw::GLint  tess_gen_point_mode_value			 = GL_NONE;
				glw::GLint  tess_gen_spacing_value				 = GL_NONE;
				glw::GLint  tess_gen_vertex_order_value			 = GL_NONE;

				switch (run.primitive_mode)
				{
				case TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES:
					expected_tess_gen_mode_value = m_glExtTokens.ISOLINES;
					break;
				case TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS:
					expected_tess_gen_mode_value = m_glExtTokens.QUADS;
					break;
				case TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES:
					expected_tess_gen_mode_value = GL_TRIANGLES;
					break;

				default:
				{
					/* Unrecognized primitive mode? */
					DE_ASSERT(false);
				}
				} /* switch (run.primitive_mode) */

				switch (run.vertex_spacing)
				{
				case TESSELLATION_SHADER_VERTEX_SPACING_DEFAULT:
				case TESSELLATION_SHADER_VERTEX_SPACING_EQUAL:
					expected_tess_gen_spacing_value = GL_EQUAL;
					break;
				case TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN:
					expected_tess_gen_spacing_value = m_glExtTokens.FRACTIONAL_EVEN;
					break;
				case TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD:
					expected_tess_gen_spacing_value = m_glExtTokens.FRACTIONAL_ODD;
					break;

				default:
				{
					/* Unrecognized vertex spacing mode? */
					DE_ASSERT(false);
				}
				} /* switch (run.vertex_spacing) */

				switch (run.vertex_ordering)
				{
				case TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT:
				case TESSELLATION_SHADER_VERTEX_ORDERING_CCW:
					expected_tess_gen_vertex_order_value = GL_CCW;
					break;
				case TESSELLATION_SHADER_VERTEX_ORDERING_CW:
					expected_tess_gen_vertex_order_value = GL_CW;
					break;

				default:
				{
					/* Unrecognized vertex ordering mode? */
					DE_ASSERT(false);
				}
				} /* switch (run.vertex_ordering) */

				if (program == run.po_id || program == run.tc_program_id)
				{
					gl.getProgramiv(program, m_glExtTokens.TESS_CONTROL_OUTPUT_VERTICES,
									&tess_control_output_vertices_value);
					GLU_EXPECT_NO_ERROR(gl.getError(),
										"glGetProgramiv() failed for GL_TESS_CONTROL_OUTPUT_VERTICES_EXT pname");

					if (tess_control_output_vertices_value != m_gl_max_patch_vertices_value)
					{
						TCU_FAIL(
							"Invalid value returned by glGetProgramiv() for GL_TESS_CONTROL_OUTPUT_VERTICES_EXT query");
					}
				}

				if (program == run.po_id || program == run.te_program_id)
				{
					gl.getProgramiv(program, m_glExtTokens.TESS_GEN_MODE, &tess_gen_mode_value);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed for GL_TESS_GEN_MODE_EXT pname");

					if ((glw::GLuint)tess_gen_mode_value != expected_tess_gen_mode_value)
					{
						TCU_FAIL("Invalid value returned by glGetProgramiv() for GL_TESS_GEN_MODE_EXT query");
					}
				}

				if (program == run.po_id || program == run.te_program_id)
				{
					gl.getProgramiv(program, m_glExtTokens.TESS_GEN_SPACING, &tess_gen_spacing_value);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed for GL_TESS_GEN_SPACING_EXT pname");

					if ((glw::GLuint)tess_gen_spacing_value != expected_tess_gen_spacing_value)
					{
						TCU_FAIL("Invalid value returned by glGetProgramiv() for GL_TESS_GEN_SPACING_EXT query");
					}
				}

				if (program == run.po_id || program == run.te_program_id)
				{
					gl.getProgramiv(program, m_glExtTokens.TESS_GEN_VERTEX_ORDER, &tess_gen_vertex_order_value);
					GLU_EXPECT_NO_ERROR(gl.getError(),
										"glGetProgramiv() failed for GL_TESS_GEN_VERTEX_ORDER_EXT pname");

					if ((glw::GLuint)tess_gen_vertex_order_value != expected_tess_gen_vertex_order_value)
					{
						TCU_FAIL("Invalid value returned by glGetProgramiv() for GL_TESS_GEN_VERTEX_ORDER_EXT query");
					}
				}

				if (program == run.po_id || program == run.te_program_id)
				{
					gl.getProgramiv(program, m_glExtTokens.TESS_GEN_POINT_MODE, &tess_gen_point_mode_value);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed for GL_TESS_GEN_POINT_MODE_EXT pname");

					if (tess_gen_point_mode_value != ((run.point_mode) ? GL_TRUE : GL_FALSE))
					{
						TCU_FAIL("Invalid value returned by glGetProgramiv() for GL_TESS_GEN_POINT_MODE_EXT query");
					}
				}
			} /* if (program == run.po_id || program == run.tc_program_id || program == run.te_program_id) */
		}	 /* for (all considered program objects) */

		if (!should_use_program_object)
		{
			/* Attach all stages to the pipeline object */
			gl.useProgramStages(run.pipeline_object_id, GL_FRAGMENT_SHADER_BIT, run.fs_program_id);
			gl.useProgramStages(run.pipeline_object_id, m_glExtTokens.TESS_CONTROL_SHADER_BIT, run.tc_program_id);
			gl.useProgramStages(run.pipeline_object_id, m_glExtTokens.TESS_EVALUATION_SHADER_BIT, run.te_program_id);
			gl.useProgramStages(run.pipeline_object_id, GL_VERTEX_SHADER_BIT, run.vs_program_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call(s) failed");

			/* Make sure the pipeline object validates correctly */
			glw::GLint validate_status = GL_FALSE;

			gl.validateProgramPipeline(run.pipeline_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glValidateProgramPipeline() call failed");

			gl.getProgramPipelineiv(run.pipeline_object_id, GL_VALIDATE_STATUS, &validate_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv() call failed");

			if (validate_status != GL_TRUE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Info log:\n"
								   << getPipelineInfoLog(run.pipeline_object_id) << "\n\nVertex Shader:\n"
								   << vs_code_raw_ptr << "\n\nTessellation Control Shader:\n"
								   << tc_code_raw_ptr << "\n\nTessellation Evaluation Shader:\n"
								   << te_code_raw_ptr << "\n\nFragment Shader:\n"
								   << fs_code_raw_ptr << tcu::TestLog::EndMessage;
				TCU_FAIL("Pipeline object was found to be invalid");
			}
		}

		/* Determine how many vertices are going to be generated by the tessellator
		 * for particular tessellation configuration.
		 */
		unsigned int n_vertices_generated = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
			run.primitive_mode, run.inner, run.outer, run.vertex_spacing, run.point_mode);

		/* Allocate enough space to hold the result XFB data */
		const unsigned int bo_size = xfb_varyings_size * n_vertices_generated;

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL /* data */, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed");

		/* Use the pipeline or program object and render the data */
		glw::GLenum tf_mode = TessellationShaderUtils::getTFModeForPrimitiveMode(run.primitive_mode, run.point_mode);

		if (should_use_program_object)
		{
			gl.bindProgramPipeline(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed");

			gl.useProgram(run.po_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");
		}
		else
		{
			gl.bindProgramPipeline(run.pipeline_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed");

			gl.useProgram(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");
		}

		gl.beginTransformFeedback(tf_mode);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");
		{
			gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, 1 /* count */);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");
		}
		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

		/* Map the buffer object contents into process space */
		const char* xfb_data = (const char*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
															  bo_size, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed");

		/* Iterate through all vertices and extract all captured data. To reduce amount
		 * of time necessary to verify the generated data, only store *unique* values.
		 */
		for (unsigned int n_vertex = 0; n_vertex < n_vertices_generated; ++n_vertex)
		{
			if (xfb_pointsize_data_offset != -1)
			{
				const float* data_ptr =
					(const float*)(xfb_data + xfb_varyings_size * n_vertex + xfb_pointsize_data_offset);

				if (std::find(run.result_pointsize_data.begin(), run.result_pointsize_data.end(), *data_ptr) ==
					run.result_pointsize_data.end())
				{
					run.result_pointsize_data.push_back(*data_ptr);
				}
			}

			if (xfb_position_data_offset != -1)
			{
				const float* data_ptr =
					(const float*)(xfb_data + xfb_varyings_size * n_vertex + xfb_position_data_offset);
				_vec4 new_item = _vec4(data_ptr[0], data_ptr[1], data_ptr[2], data_ptr[3]);

				if (std::find(run.result_position_data.begin(), run.result_position_data.end(), new_item) ==
					run.result_position_data.end())
				{
					run.result_position_data.push_back(new_item);
				}
			}

			if (xfb_value1_data_offset != -1)
			{
				const float* data_ptr =
					(const float*)(xfb_data + xfb_varyings_size * n_vertex + xfb_value1_data_offset);
				_vec2 new_item = _vec2(data_ptr[0], data_ptr[1]);

				if (std::find(run.result_value1_data.begin(), run.result_value1_data.end(), new_item) ==
					run.result_value1_data.end())
				{
					run.result_value1_data.push_back(new_item);
				}
			}

			if (xfb_value2_data_offset != -1)
			{
				const int* data_ptr = (const int*)(xfb_data + xfb_varyings_size * n_vertex + xfb_value2_data_offset);
				_ivec4	 new_item = _ivec4(data_ptr[0], data_ptr[1], data_ptr[2], data_ptr[3]);

				if (std::find(run.result_value2_data.begin(), run.result_value2_data.end(), new_item) ==
					run.result_value2_data.end())
				{
					run.result_value2_data.push_back(new_item);
				}
			}
		} /* for (all result tessellation coordinates) */

		/* Good to unmap the buffer object at this point */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");
	} /* for (two iterations) */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize::iterate(void)
{
	/* Initialize ES test objects */
	initTest();

	/* Calculate reference values that should be generated for all runs */
	float		reference_result_pointsize(0);
	_vec4		reference_result_position(0, 0, 0, 0);
	_vec2		reference_result_value1(0, 0);
	_ivec4		reference_result_value2(0, 0, 0, 0);
	const float epsilon = (float)1e-5;

	for (glw::GLint n_invocation = 0; n_invocation < m_gl_max_patch_vertices_value; ++n_invocation)
	{
		/* As per TC and TE shaders */
		reference_result_pointsize += 1.0f / static_cast<float>(n_invocation + 1);

		reference_result_position.x += static_cast<float>(n_invocation * 4 + 0);
		reference_result_position.y += static_cast<float>(n_invocation * 4 + 1);
		reference_result_position.z += static_cast<float>(n_invocation * 4 + 2);
		reference_result_position.w += static_cast<float>(n_invocation * 4 + 3);

		reference_result_value1.x += 1.0f / static_cast<float>(n_invocation + 1);
		reference_result_value1.y += 1.0f / static_cast<float>(n_invocation + 2);

		reference_result_value2.x += (n_invocation + 1);
		reference_result_value2.y += (n_invocation + 2);
		reference_result_value2.z += (n_invocation + 3);
		reference_result_value2.w += (n_invocation + 4);
	}

	/* Iterate through test runs and analyse the result data */
	for (_runs_const_iterator run_iterator = m_runs.begin(); run_iterator != m_runs.end(); run_iterator++)
	{
		const _run& run = *run_iterator;

		/* For the very first run, make sure that the type of tessellation shader objects
		 * is reported correctly for both program and pipeline object cases.
		 */
		if (run_iterator == m_runs.begin())
		{
			const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();
			glw::GLint			  shader_type_tc = GL_NONE;
			glw::GLint			  shader_type_te = GL_NONE;

			/* Program objects first */
			gl.getShaderiv(run.tc_id, GL_SHADER_TYPE, &shader_type_tc);
			gl.getShaderiv(run.te_id, GL_SHADER_TYPE, &shader_type_te);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call(s) failed");

			if ((glw::GLenum)shader_type_tc != m_glExtTokens.TESS_CONTROL_SHADER)
			{
				TCU_FAIL("Invalid shader type reported by glGetShaderiv() for a tessellation control shader");
			}

			if ((glw::GLenum)shader_type_te != m_glExtTokens.TESS_EVALUATION_SHADER)
			{
				TCU_FAIL("Invalid shader type reported by glGetShaderiv() for a tessellation evaluation shader");
			}

			/* Let's query the pipeline object now */
			glw::GLint shader_id_tc = 0;
			glw::GLint shader_id_te = 0;

			gl.getProgramPipelineiv(run.pipeline_object_id, m_glExtTokens.TESS_CONTROL_SHADER, &shader_id_tc);
			gl.getProgramPipelineiv(run.pipeline_object_id, m_glExtTokens.TESS_EVALUATION_SHADER, &shader_id_te);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv() failed for GL_TESS_CONTROL_SHADER_EXT / "
											   "GL_TESS_EVALUATION_SHADER_EXT enum(s)");

			if ((glw::GLuint)shader_id_tc != run.tc_program_id)
			{
				TCU_FAIL("Invalid separate program object ID reported for Tessellation Control stage");
			}

			if ((glw::GLuint)shader_id_te != run.te_program_id)
			{
				TCU_FAIL("Invalid separate program object ID reported for Tessellation Evaluation stage");
			}
		}

		if ((run.point_mode && run.result_pointsize_data.size() != 1) ||
			(run.point_mode && de::abs(run.result_pointsize_data[0] - reference_result_pointsize) > epsilon))
		{
			/* It is a test bug if result_pointsize_data.size() == 0 */
			DE_ASSERT(run.result_pointsize_data.size() > 0);

			m_testCtx.getLog() << tcu::TestLog::Message << "Tessellation Evaluation stage set gl_PointSize value to "
							   << run.result_pointsize_data[0] << " instead of expected value "
							   << reference_result_pointsize << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid gl_PointSize data exposed in TE stage");
		}

		if (run.result_position_data.size() != 1 || run.result_position_data[0] != reference_result_position)
		{
			/* It is a test bug if result_position_data.size() == 0 */
			DE_ASSERT(run.result_position_data.size() > 0);

			m_testCtx.getLog() << tcu::TestLog::Message << "Tessellation Evaluation stage set gl_Position to "
							   << " (" << run.result_position_data[0].x << ", " << run.result_position_data[0].y << ", "
							   << run.result_position_data[0].z << ", " << run.result_position_data[0].w
							   << " ) instead of expected value"
								  " ("
							   << reference_result_position.x << ", " << reference_result_position.y << ", "
							   << reference_result_position.z << ", " << reference_result_position.w << ")"
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid gl_Position data exposed in TE stage");
		}

		if (run.result_value1_data.size() != 1 ||
			de::abs(run.result_value1_data[0].x - reference_result_value1.x) > epsilon ||
			de::abs(run.result_value1_data[0].y - reference_result_value1.y) > epsilon)
		{
			/* It is a test bug if result_value1_data.size() == 0 */
			DE_ASSERT(run.result_value1_data.size() > 0);

			m_testCtx.getLog() << tcu::TestLog::Message << "Tessellation Evaluation stage set te_value1 to "
							   << " (" << run.result_value1_data[0].x << ", " << run.result_value1_data[0].y
							   << " ) instead of expected value"
								  " ("
							   << reference_result_value1.x << ", " << reference_result_value1.y << ")"
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid gl_Position data exposed in TE stage");
		}

		if (run.result_value2_data.size() != 1 || run.result_value2_data[0] != reference_result_value2)
		{
			/* It is a test bug if result_value2_data.size() == 0 */
			DE_ASSERT(run.result_value2_data.size() > 0);

			m_testCtx.getLog() << tcu::TestLog::Message << "Tessellation Evaluation stage set te_value2 to "
							   << " (" << run.result_value2_data[0].x << ", " << run.result_value2_data[0].y << ", "
							   << run.result_value2_data[0].z << ", " << run.result_value2_data[0].w
							   << " ) instead of expected value"
								  " ("
							   << reference_result_value2.x << ", " << reference_result_value2.y << ", "
							   << reference_result_value2.z << ", " << reference_result_value2.w << ")"
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value2 data saved in TE stage");
		}
	} /* for (all runs) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTCTEgl_TessLevel::TessellationShaderTCTEgl_TessLevel(Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "gl_tessLevel",
				   "Verifies gl_TessLevelOuter and gl_TessLevelInner patch variable "
				   "values in a tessellation evaluation shader are valid and correspond"
				   "to values configured in a tessellation control shader (should one be "
				   "present) or to the default values, as set with glPatchParameterfv() calls")
	, m_gl_max_tess_gen_level_value(0)
	, m_bo_id(0)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderTCTEgl_TessLevel::deinit()
{
	/** Call base class' deinit() function */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Reset GL_PATCH_VERTICES_EXT value to the default setting */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		/* Revert GL_PATCH_DEFAULT_INNER_LEVEL and GL_PATCH_DEFAULT_OUTER_LEVEL pname
		 * values to the default settings */
		const float default_levels[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		gl.patchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, default_levels);
		gl.patchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, default_levels);
	}

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Release all objects we might've created */
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

	for (_tests::iterator it = m_tests.begin(); it != m_tests.end(); ++it)
	{
		deinitTestDescriptor(&*it);
	}
	m_tests.clear();
}

/** Deinitializes ES objects created for particular test pass.
 *
 *  @param test_ptr Test run descriptor. Must not be NULL.
 *
 **/
void TessellationShaderTCTEgl_TessLevel::deinitTestDescriptor(_test_descriptor* test_ptr)
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	/* Release all objects */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (test_ptr->fs_id != 0)
	{
		gl.deleteShader(test_ptr->fs_id);

		test_ptr->fs_id = 0;
	}

	if (test_ptr->po_id != 0)
	{
		gl.deleteProgram(test_ptr->po_id);

		test_ptr->po_id = 0;
	}

	if (test_ptr->tcs_id != 0)
	{
		gl.deleteShader(test_ptr->tcs_id);

		test_ptr->tcs_id = 0;
	}

	if (test_ptr->tes_id != 0)
	{
		gl.deleteShader(test_ptr->tes_id);

		test_ptr->tes_id = 0;
	}

	if (test_ptr->vs_id != 0)
	{
		gl.deleteShader(test_ptr->vs_id);

		test_ptr->vs_id = 0;
	}
}

/** Initializes all ES objects that will be used for the test. */
void TessellationShaderTCTEgl_TessLevel::initTest()
{
	/* The test requires EXT_tessellation_shader */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value before we carry on */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Retrieve gen level */
	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &m_gl_max_tess_gen_level_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_TESS_GEN_LEVEL_EXT pname failed");

	/* Initialize test descriptors */
	_test_descriptor test_tcs_tes_equal;
	_test_descriptor test_tcs_tes_fe;
	_test_descriptor test_tcs_tes_fo;
	_test_descriptor test_tes_equal;
	_test_descriptor test_tes_fe;
	_test_descriptor test_tes_fo;

	initTestDescriptor(TESSELLATION_TEST_TYPE_TCS_TES, &test_tcs_tes_equal, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL);
	initTestDescriptor(TESSELLATION_TEST_TYPE_TCS_TES, &test_tcs_tes_fe,
					   TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN);
	initTestDescriptor(TESSELLATION_TEST_TYPE_TCS_TES, &test_tcs_tes_fo,
					   TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD);
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		initTestDescriptor(TESSELLATION_TEST_TYPE_TES, &test_tes_equal, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL);
		initTestDescriptor(TESSELLATION_TEST_TYPE_TES, &test_tes_fe,
						   TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN);
		initTestDescriptor(TESSELLATION_TEST_TYPE_TES, &test_tes_fo, TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD);
	}

	m_tests.push_back(test_tcs_tes_equal);
	m_tests.push_back(test_tcs_tes_fe);
	m_tests.push_back(test_tcs_tes_fo);
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		m_tests.push_back(test_tes_equal);
		m_tests.push_back(test_tes_fe);
		m_tests.push_back(test_tes_fo);
	}

	/* Generate and set up a buffer object we will use to hold XFBed data.
	 *
	 * NOTE: We do not set the buffer object's storage here because its size
	 *       is iteration-specific.
	 **/
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

	/* We're good to execute the test! */
}

/** Initializes ES objects for a particular tess pass.
 *
 *  @param test_type           Determines test type to be used for initialization.
 *                             TEST_TYPE_TCS_TES will use both TC and TE stages,
 *                             TEST_TYPE_TES will assume only TE stage should be used.
 *  @param out_test_ptr        Deref will be used to store object data. Must not be NULL.
 *  @param vertex_spacing_mode Vertex spacing mode to use for the TE stage.
 *
 **/
void TessellationShaderTCTEgl_TessLevel::initTestDescriptor(_tessellation_test_type				test_type,
															_test_descriptor*					out_test_ptr,
															_tessellation_shader_vertex_spacing vertex_spacing_mode)
{
	out_test_ptr->type			 = test_type;
	out_test_ptr->vertex_spacing = vertex_spacing_mode;

	/* Generate a program object we will later configure */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	out_test_ptr->po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Generate shader objects the test will use */
	out_test_ptr->fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	out_test_ptr->vs_id = gl.createShader(GL_VERTEX_SHADER);

	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES || test_type == TESSELLATION_TEST_TYPE_TES)
	{
		out_test_ptr->tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	}

	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES)
	{
		out_test_ptr->tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Configure fragment shader */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(out_test_ptr->fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader object");

	/* Configure tessellation control shader */
	const char* tc_body = "${VERSION}\n"
						  "\n"
						  /* Required EXT_tessellation_shader functionality */
						  "${TESSELLATION_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (vertices = 4) out;\n"
						  "\n"
						  "uniform vec2 inner_tess_levels;\n"
						  "uniform vec4 outer_tess_levels;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
						  "    if (gl_InvocationID == 0) {\n"
						  "        gl_TessLevelInner[0]                           = inner_tess_levels[0];\n"
						  "        gl_TessLevelInner[1]                           = inner_tess_levels[1];\n"
						  "        gl_TessLevelOuter[0]                           = outer_tess_levels[0];\n"
						  "        gl_TessLevelOuter[1]                           = outer_tess_levels[1];\n"
						  "        gl_TessLevelOuter[2]                           = outer_tess_levels[2];\n"
						  "        gl_TessLevelOuter[3]                           = outer_tess_levels[3];\n"
						  "   }\n"
						  "}\n";

	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES)
	{
		shaderSourceSpecialized(out_test_ptr->tcs_id, 1 /* count */, &tc_body);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation control shader object");
	}

	/* Configure tessellation evaluation shader */
	const char* te_body = "${VERSION}\n"
						  "\n"
						  "${TESSELLATION_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (quads, point_mode, VERTEX_SPACING_MODE) in;\n"
						  "\n"
						  "out vec2 result_tess_level_inner;\n"
						  "out vec4 result_tess_level_outer;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);\n"
						  "    vec4 p2 = mix(gl_in[2].gl_Position,gl_in[3].gl_Position,gl_TessCoord.x);\n"
						  "    gl_Position = mix(p1, p2, gl_TessCoord.y);\n"
						  "\n"
						  "    result_tess_level_inner = vec2(gl_TessLevelInner[0],\n"
						  "                                   gl_TessLevelInner[1]);\n"
						  "    result_tess_level_outer = vec4(gl_TessLevelOuter[0],\n"
						  "                                   gl_TessLevelOuter[1],\n"
						  "                                   gl_TessLevelOuter[2],\n"
						  "                                   gl_TessLevelOuter[3]);\n"
						  "}\n";

	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES || test_type == TESSELLATION_TEST_TYPE_TES)
	{
		/* Replace VERTEX_SPACING_MODE with the mode provided by the caller */
		std::stringstream te_body_stringstream;
		std::string		  te_body_string;
		const std::string token = "VERTEX_SPACING_MODE";
		std::size_t		  token_index;
		std::string		  vertex_spacing_string =
			TessellationShaderUtils::getESTokenForVertexSpacingMode(vertex_spacing_mode);

		te_body_stringstream << te_body;
		te_body_string = te_body_stringstream.str();

		token_index = te_body_string.find(token);

		while (token_index != std::string::npos)
		{
			te_body_string = te_body_string.replace(token_index, token.length(), vertex_spacing_string.c_str());

			token_index = te_body_string.find(token);
		}

		/* Set the shader source */
		const char* te_body_string_raw = te_body_string.c_str();

		shaderSourceSpecialized(out_test_ptr->tes_id, 1 /* count */, &te_body_string_raw);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation evaluation shader object");
	}

	/* Configure vertex shader */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(1.0, 2.0, 3.0, 4.0);\n"
						  "}\n";

	shaderSourceSpecialized(out_test_ptr->vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader object");

	/* Compile all shaders of our interest */
	const glw::GLuint shaders[] = { out_test_ptr->fs_id, out_test_ptr->tcs_id, out_test_ptr->tes_id,
									out_test_ptr->vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLint  compile_status = GL_FALSE;
		glw::GLuint shader		   = shaders[n_shader];

		if (shader != 0)
		{
			gl.compileShader(shader);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

			gl.getShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

			if (compile_status != GL_TRUE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Compilation of shader object at index " << n_shader
								   << " failed." << tcu::TestLog::EndMessage;

				TCU_FAIL("Shader compilation failed");
			}
		} /* if (shader != 0) */
	}	 /* for (all shaders) */

	/* Attach the shaders to the test program object, set up XFB and then link the program */
	glw::GLint		   link_status = GL_FALSE;
	const char*		   varyings[]  = { "result_tess_level_inner", "result_tess_level_outer" };
	const unsigned int n_varyings  = sizeof(varyings) / sizeof(varyings[0]);

	gl.transformFeedbackVaryings(out_test_ptr->po_id, n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");

	gl.attachShader(out_test_ptr->po_id, out_test_ptr->fs_id);
	gl.attachShader(out_test_ptr->po_id, out_test_ptr->vs_id);

	if (out_test_ptr->tcs_id != 0)
	{
		gl.attachShader(out_test_ptr->po_id, out_test_ptr->tcs_id);
	}

	if (out_test_ptr->tes_id != 0)
	{
		gl.attachShader(out_test_ptr->po_id, out_test_ptr->tes_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	gl.linkProgram(out_test_ptr->po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(out_test_ptr->po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* Retrieve uniform locations */
	out_test_ptr->inner_tess_levels_uniform_location = gl.getUniformLocation(out_test_ptr->po_id, "inner_tess_levels");
	out_test_ptr->outer_tess_levels_uniform_location = gl.getUniformLocation(out_test_ptr->po_id, "outer_tess_levels");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call(s) failed");

	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES)
	{
		DE_ASSERT(out_test_ptr->inner_tess_levels_uniform_location != -1);
		DE_ASSERT(out_test_ptr->outer_tess_levels_uniform_location != -1);
	}
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderTCTEgl_TessLevel::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize ES test objects */
	initTest();

	/* Our program object takes a single quad per patch */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() call failed");

	/* Prepare for rendering */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed.");

	/* We will iterate through all added tests. */
	for (_tests_const_iterator test_iterator = m_tests.begin(); test_iterator != m_tests.end(); ++test_iterator)
	{
		/* Iterate through a few different inner/outer tessellation level combinations */
		glw::GLfloat tessellation_level_combinations[] = {
			/* inner[0] */ /* inner[1] */ /* outer[0] */ /* outer[1] */ /* outer[2] */ /* outer[3] */
			1.1f, 1.4f, 2.7f, 3.1f, 4.4f, 5.7f, 64.2f, 32.5f, 16.8f, 8.2f, 4.5f, 2.8f, 3.3f, 6.6f, 9.9f, 12.3f, 15.6f,
			18.9f
		};
		const unsigned int n_tessellation_level_combinations = sizeof(tessellation_level_combinations) /
															   sizeof(tessellation_level_combinations[0]) /
															   6; /* 2 inner + 4 outer levels */

		for (unsigned int n_combination = 0; n_combination < n_tessellation_level_combinations; ++n_combination)
		{
			glw::GLfloat inner_tess_level[] = { tessellation_level_combinations[n_combination * 6 + 0],
												tessellation_level_combinations[n_combination * 6 + 1] };

			glw::GLfloat outer_tess_level[] = { tessellation_level_combinations[n_combination * 6 + 2],
												tessellation_level_combinations[n_combination * 6 + 3],
												tessellation_level_combinations[n_combination * 6 + 4],
												tessellation_level_combinations[n_combination * 6 + 5] };

			TessellationShaderUtils tessUtils(gl, this);
			const unsigned int		n_rendered_vertices = tessUtils.getAmountOfVerticesGeneratedByTessellator(
				TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS, inner_tess_level, outer_tess_level,
				test_iterator->vertex_spacing, true); /* is_point_mode_enabled */

			/* Test type determines how the tessellation levels should be set. */
			gl.useProgram(test_iterator->po_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

			switch (test_iterator->type)
			{
			case TESSELLATION_TEST_TYPE_TCS_TES:
			{
				gl.uniform2fv(test_iterator->inner_tess_levels_uniform_location, 1, /* count */
							  inner_tess_level);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform2fv() call failed");

				gl.uniform4fv(test_iterator->outer_tess_levels_uniform_location, 1, /* count */
							  outer_tess_level);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() call failed");

				break;
			}

			case TESSELLATION_TEST_TYPE_TES:
			{
				if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
				{
					gl.patchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, inner_tess_level);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameterfv() call failed for"
													   " GL_PATCH_DEFAULT_INNER_LEVEL pname");

					gl.patchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outer_tess_level);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameterfv() call failed for"
													   " GL_PATCH_DEFAULT_OUTER_LEVEL pname");
				}
				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized test type");
			}
			} /* switch (test_iterator->type) */

			/* Set up storage properties for the buffer object, to which XFBed data will be
			 * written.
			 */
			const unsigned int n_bytes_needed =
				static_cast<unsigned int>(n_rendered_vertices * (2 /* vec2 */ + 4 /* vec4 */) * sizeof(float));

			gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, n_bytes_needed, NULL /* data */, GL_STATIC_DRAW);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed");

			/* Render the test geometry */
			gl.beginTransformFeedback(GL_POINTS);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");
			{
				/* A single vertex will do, since we configured GL_PATCH_VERTICES_EXT to be 1 */
				gl.drawArrays(GL_PATCHES_EXT, 0 /* first */, 4 /* count */);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");
			}
			gl.endTransformFeedback();
			GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

			/* Now that the BO is filled with data, map it so we can check the storage's contents */
			const float* mapped_data_ptr = (const float*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
																		   n_bytes_needed, GL_MAP_READ_BIT);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

			/* Verify the contents. For each result vertex, inner/outer tessellation levels should
			 * be unchanged. */
			const float		   epsilon = (float)1e-5;
			const unsigned int n_result_points =
				static_cast<unsigned int>(n_bytes_needed / sizeof(float) / (2 /* vec2 */ + 4 /* vec4 */));

			for (unsigned int n_point = 0; n_point < n_result_points; ++n_point)
			{
				const float* point_data_ptr = mapped_data_ptr + (2 /* vec2 */ + 4 /* vec4 */) * n_point;

				if (de::abs(point_data_ptr[2] - outer_tess_level[0]) > epsilon ||
					de::abs(point_data_ptr[3] - outer_tess_level[1]) > epsilon)
				{
					std::string vertex_spacing_mode_string =
						TessellationShaderUtils::getESTokenForVertexSpacingMode(test_iterator->vertex_spacing);

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Invalid inner/outer tessellation level used in TE stage;"
									   << " expected outer:(" << outer_tess_level[0] << ", " << outer_tess_level[1]
									   << ") "
									   << " rendered outer:(" << point_data_ptr[2] << ", " << point_data_ptr[3] << ")"
									   << " vertex spacing mode: " << vertex_spacing_mode_string.c_str()
									   << tcu::TestLog::EndMessage;

					TCU_FAIL("Invalid inner/outer tessellation level used in TE stage");
				}
			} /* for (all points) */

			/* All done - unmap the storage */
			gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");
		} /* for (all tess level combinations) */
	}	 /* for (all tests) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTCTEgl_PatchVerticesIn::TessellationShaderTCTEgl_PatchVerticesIn(Context&				context,
																				   const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "gl_PatchVerticesIn",
				   "Verifies gl_PatchVerticesIn size is valid in a tessellation"
				   " evaluation shader and corresponds to the value configured in"
				   " a tessellation control shader (should one be present) or to"
				   " the default value, as set with glPatchParameteriEXT() call")
	, m_gl_max_patch_vertices_value(0)
	, m_bo_id(0)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderTCTEgl_PatchVerticesIn::deinit()
{
	/** Call base class' deinit() function */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Reset GL_PATCH_VERTICES_EXT to the default setting */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Release all objects we might've created */
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

	for (_tests::iterator it = m_tests.begin(); it != m_tests.end(); ++it)
	{
		deinitTestDescriptor(&*it);
	}
	m_tests.clear();
}

/** Deinitializes ES objects created for particular test pass.
 *
 *  @param test_ptr Test run descriptor. Must not be NULL.
 *
 **/
void TessellationShaderTCTEgl_PatchVerticesIn::deinitTestDescriptor(_test_descriptor* test_ptr)
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	/* Release all objects */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (test_ptr->fs_id != 0)
	{
		gl.deleteShader(test_ptr->fs_id);

		test_ptr->fs_id = 0;
	}

	if (test_ptr->po_id != 0)
	{
		gl.deleteProgram(test_ptr->po_id);

		test_ptr->po_id = 0;
	}

	if (test_ptr->tcs_id != 0)
	{
		gl.deleteShader(test_ptr->tcs_id);

		test_ptr->tcs_id = 0;
	}

	if (test_ptr->tes_id != 0)
	{
		gl.deleteShader(test_ptr->tes_id);

		test_ptr->tes_id = 0;
	}

	if (test_ptr->vs_id != 0)
	{
		gl.deleteShader(test_ptr->vs_id);

		test_ptr->vs_id = 0;
	}
}

/** Initializes all ES objects that will be used for the test. */
void TessellationShaderTCTEgl_PatchVerticesIn::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* The test requires EXT_tessellation_shader */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Retrieve GL_MAX_PATCH_VERTICES_EXT value before we carry on */
	gl.getIntegerv(m_glExtTokens.MAX_PATCH_VERTICES, &m_gl_max_patch_vertices_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_MAX_PATCH_VERTICES_EXT pname failed");

	/* Initialize test descriptors.
	 *
	 * Make sure the values we use are multiples of 4 - this is because we're using isolines in the
	 * tessellation stage, and in order to have the requested amount of line segments generated, we need
	 * to use a multiply of 4 vertices per patch */
	glw::GLint n_half_max_patch_vertices_mul_4 = m_gl_max_patch_vertices_value / 2;
	glw::GLint n_max_patch_vertices_mul_4	  = m_gl_max_patch_vertices_value;

	if ((n_half_max_patch_vertices_mul_4 % 4) != 0)
	{
		/* Round to nearest mul-of-4 integer */
		n_half_max_patch_vertices_mul_4 += (4 - (m_gl_max_patch_vertices_value / 2) % 4);
	}

	if ((n_max_patch_vertices_mul_4 % 4) != 0)
	{
		/* Round to previous nearest mul-of-4 integer */
		n_max_patch_vertices_mul_4 -= (m_gl_max_patch_vertices_value % 4);
	}

	_test_descriptor test_tcs_tes_4;
	_test_descriptor test_tcs_tes_half_max_patch_vertices_mul_4;
	_test_descriptor test_tcs_tes_max_patch_vertices_mul_4;
	_test_descriptor test_tes_4;
	_test_descriptor test_tes_half_max_patch_vertices_mul_4;
	_test_descriptor test_tes_max_patch_vertices_mul_4;

	initTestDescriptor(TESSELLATION_TEST_TYPE_TCS_TES, &test_tcs_tes_4, 4);
	initTestDescriptor(TESSELLATION_TEST_TYPE_TCS_TES, &test_tcs_tes_half_max_patch_vertices_mul_4,
					   n_half_max_patch_vertices_mul_4);
	initTestDescriptor(TESSELLATION_TEST_TYPE_TCS_TES, &test_tcs_tes_max_patch_vertices_mul_4,
					   n_max_patch_vertices_mul_4);
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		initTestDescriptor(TESSELLATION_TEST_TYPE_TES, &test_tes_4, 4);
		initTestDescriptor(TESSELLATION_TEST_TYPE_TES, &test_tes_half_max_patch_vertices_mul_4,
						   n_half_max_patch_vertices_mul_4);
		initTestDescriptor(TESSELLATION_TEST_TYPE_TES, &test_tes_max_patch_vertices_mul_4, n_max_patch_vertices_mul_4);
	}

	m_tests.push_back(test_tcs_tes_4);
	m_tests.push_back(test_tcs_tes_half_max_patch_vertices_mul_4);
	m_tests.push_back(test_tcs_tes_max_patch_vertices_mul_4);
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		m_tests.push_back(test_tes_4);
		m_tests.push_back(test_tes_half_max_patch_vertices_mul_4);
		m_tests.push_back(test_tes_max_patch_vertices_mul_4);
	}

	/* Generate and set up a buffer object we will use to hold XFBed data.
	 *
	 * NOTE: We do not set the buffer object's storage here because its size
	 *       is iteration-specific.
	 **/
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

	/* We're good to execute the test! */
}

/** Initializes ES objects for a particular tess pass.
 *
 *  @param test_type        Determines test type to be used for initialization.
 *                          TEST_TYPE_TCS_TES will use both TC and TE stages,
 *                          TEST_TYPE_TES will assume only TE stage should be used.
 *  @param out_test_ptr     Deref will be used to store object data. Must not be NULL.
 *  @param input_patch_size Tells how many vertices should be used per patch for hte
 *                          result program object.
 **/
void TessellationShaderTCTEgl_PatchVerticesIn::initTestDescriptor(_tessellation_test_type test_type,
																  _test_descriptor*		  out_test_ptr,
																  unsigned int			  input_patch_size)
{
	out_test_ptr->input_patch_size = input_patch_size;
	out_test_ptr->type			   = test_type;

	/* Generate a program object we will later configure */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	out_test_ptr->po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Generate shader objects the test will use */
	out_test_ptr->fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	out_test_ptr->vs_id = gl.createShader(GL_VERTEX_SHADER);

	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES || test_type == TESSELLATION_TEST_TYPE_TES)
	{
		out_test_ptr->tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	}

	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES)
	{
		out_test_ptr->tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Configure fragment shader */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(out_test_ptr->fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader object");

	/* Configure tessellation control shader */
	const char* tc_body = "${VERSION}\n"
						  "\n"
						  /* Required EXT_tessellation_shader functionality */
						  "${TESSELLATION_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (vertices = VERTICES_TOKEN) out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_out           [gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
						  "    gl_TessLevelOuter[0]                           = 1.0;\n"
						  "    gl_TessLevelOuter[1]                           = 1.0;\n"
						  "}\n";

	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES)
	{
		const char*		  result_body	= NULL;
		std::string		  tc_body_string = tc_body;
		std::size_t		  token_index	= -1;
		const char*		  token_string   = "VERTICES_TOKEN";
		std::stringstream vertices_stringstream;
		std::string		  vertices_string;

		vertices_stringstream << input_patch_size;
		vertices_string = vertices_stringstream.str();

		while ((token_index = tc_body_string.find(token_string)) != std::string::npos)
		{
			tc_body_string = tc_body_string.replace(token_index, strlen(token_string), vertices_string);
		}

		result_body = tc_body_string.c_str();

		shaderSourceSpecialized(out_test_ptr->tcs_id, 1 /* count */, &result_body);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation control shader object");
	}

	/* Configure tessellation evaluation shader */
	const char* te_body = "${VERSION}\n"
						  "\n"
						  "${TESSELLATION_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (isolines, point_mode) in;\n"
						  "\n"
						  "flat out int result_PatchVerticesIn;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = gl_in[0].gl_Position;\n"
						  "\n"
						  "    result_PatchVerticesIn = gl_PatchVerticesIn;\n"
						  "}\n";

	shaderSourceSpecialized(out_test_ptr->tes_id, 1 /* count */, &te_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation evaluation shader object");

	/* Configure vertex shader */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(float(gl_VertexID), 2.0, 3.0, 4.0);\n"
						  "}\n";

	shaderSourceSpecialized(out_test_ptr->vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader object");

	/* Compile all shaders of our interest */
	const glw::GLuint shaders[] = { out_test_ptr->fs_id, out_test_ptr->tcs_id, out_test_ptr->tes_id,
									out_test_ptr->vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLint  compile_status = GL_FALSE;
		glw::GLuint shader		   = shaders[n_shader];

		if (shader != 0)
		{
			gl.compileShader(shader);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

			gl.getShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

			if (compile_status != GL_TRUE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Compilation of shader object at index " << n_shader
								   << " failed." << tcu::TestLog::EndMessage;

				TCU_FAIL("Shader compilation failed");
			}
		} /* if (shader != 0) */
	}	 /* for (all shaders) */

	/* Attach the shaders to the test program object, set up XFB and then link the program */
	glw::GLint  link_status = GL_FALSE;
	const char* varyings[]  = {
		"result_PatchVerticesIn",
	};
	const unsigned int n_varyings = sizeof(varyings) / sizeof(varyings[0]);

	gl.transformFeedbackVaryings(out_test_ptr->po_id, n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");

	gl.attachShader(out_test_ptr->po_id, out_test_ptr->fs_id);
	gl.attachShader(out_test_ptr->po_id, out_test_ptr->vs_id);

	if (out_test_ptr->tcs_id != 0)
	{
		gl.attachShader(out_test_ptr->po_id, out_test_ptr->tcs_id);
	}

	if (out_test_ptr->tes_id != 0)
	{
		gl.attachShader(out_test_ptr->po_id, out_test_ptr->tes_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	gl.linkProgram(out_test_ptr->po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(out_test_ptr->po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderTCTEgl_PatchVerticesIn::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize ES test objects */
	initTest();

	/* Prepare for rendering */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed.");

	/* We will iterate through all added tests. */
	for (_tests_const_iterator test_iterator = m_tests.begin(); test_iterator != m_tests.end(); ++test_iterator)
	{
		/* Activate test-specific program object first. */
		gl.useProgram(test_iterator->po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

		/* Test type tells determines how the tessellation levels should be set.
		 * We don't need to do anything specific if TCS+TES are in, but if no
		 * TCS is present, we need to configure default amount of input patch-vertices
		 * to the test-specific value.
		 */
		glw::GLint n_patch_vertices = 0;

		switch (test_iterator->type)
		{
		case TESSELLATION_TEST_TYPE_TCS_TES:
		{
			/* We're using isolines mode which requires at least 4 input vertices per patch */
			gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 4);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() for GL_PATCH_VERTICES_EXT failed.");

			n_patch_vertices = 4;

			break;
		}

		case TESSELLATION_TEST_TYPE_TES:
		{
			gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, test_iterator->input_patch_size);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() call failed");

			n_patch_vertices = test_iterator->input_patch_size;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized test type");
		}
		} /* switch (test_iterator->type) */

		/* Set up storage properties for the buffer object, to which XFBed data will be
		 * written.
		 **/
		const unsigned int n_bytes_needed = sizeof(int) * 2; /* the tessellator will output two vertices */

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, n_bytes_needed, NULL /* data */, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed");

		/* Render the test geometry */
		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");
		{
			/* Pass a single patch only */
			gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, n_patch_vertices);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");
		}
		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

		/* Now that the BO is filled with data, map it so we can check the storage's contents */
		const int* mapped_data_ptr = (const int*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
																   n_bytes_needed, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

		/* Verify the contents. Make sure the value we retrieved is equal to the test-specific
		 * amount of vertices per patch.
		 */
		for (unsigned int n_vertex = 0; n_vertex < 2 /* output vertices */; ++n_vertex)
		{
			unsigned int te_PatchVerticesInSize = mapped_data_ptr[n_vertex];

			if (te_PatchVerticesInSize != test_iterator->input_patch_size)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid gl_PatchVerticesIn defined for TE stage "
								   << " and result vertex index:" << n_vertex
								   << " expected:" << test_iterator->input_patch_size
								   << " rendered:" << te_PatchVerticesInSize << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid gl_PatchVerticesIn size used in TE stage");
			} /* if (comparison failed)  */
		}

		/* All done - unmap the storage */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");
	} /* for (all tests) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} /* namespace glcts */
