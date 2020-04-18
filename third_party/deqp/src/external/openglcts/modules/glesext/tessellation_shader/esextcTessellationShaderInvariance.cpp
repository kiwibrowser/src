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

#include "esextcTessellationShaderInvariance.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/* Defines a single vertex in tessellation space */
typedef struct _vertex
{
	float u;
	float v;
	float w;

	_vertex()
	{
		u = 0.0f;
		v = 0.0f;
		w = 0.0f;
	}
} _vertex;

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderInvarianceTests::TessellationShaderInvarianceTests(glcts::Context&	  context,
																	 const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "tessellation_invariance",
						"Verifies the implementation conforms to invariance rules.")
{
	/* No implementation needed */
}

/**
 * Initializes test groups for geometry shader tests
 **/
void TessellationShaderInvarianceTests::init(void)
{
	addChild(new glcts::TessellationShaderInvarianceRule1Test(m_context, m_extParams));
	addChild(new glcts::TessellationShaderInvarianceRule2Test(m_context, m_extParams));
	addChild(new glcts::TessellationShaderInvarianceRule3Test(m_context, m_extParams));
	addChild(new glcts::TessellationShaderInvarianceRule4Test(m_context, m_extParams));
	addChild(new glcts::TessellationShaderInvarianceRule5Test(m_context, m_extParams));
	addChild(new glcts::TessellationShaderInvarianceRule6Test(m_context, m_extParams));
	addChild(new glcts::TessellationShaderInvarianceRule7Test(m_context, m_extParams));
}

/** Constructor
 *
 * @param context     Test context
 * @param name        Test name
 * @param description Test description
 **/
TessellationShaderInvarianceBaseTest::TessellationShaderInvarianceBaseTest(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_utils_ptr(DE_NULL)
	, m_bo_id(0)
	, m_qo_tfpw_id(0)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test. */
void TessellationShaderInvarianceBaseTest::deinit()
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Revert buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Reset GL_PATCH_VERTICES_EXT to default value */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Deinitialize all ES objects that were created for test purposes */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	for (_programs_iterator it = m_programs.begin(); it != m_programs.end(); ++it)
	{
		_test_program& program = *it;

		if (program.po_id != 0)
		{
			gl.deleteProgram(program.po_id);
		}
	}
	m_programs.clear();

	if (m_qo_tfpw_id != 0)
	{
		gl.deleteQueries(1, &m_qo_tfpw_id);

		m_qo_tfpw_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Deinitialize TS utils instance */
	if (m_utils_ptr != NULL)
	{
		delete m_utils_ptr;

		m_utils_ptr = NULL;
	}
}

/** Executes a single-counted GL_PATCHES_EXT draw call.
 *
 *  Throws TestError exception if an error occurs.
 *
 *  @param n_iteration Not used.
 *
 **/
void TessellationShaderInvarianceBaseTest::executeDrawCall(unsigned int n_iteration)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	DE_UNREF(n_iteration);

	gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, getDrawCallCountArgument());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");
}

/** Returns a value that should be used for the draw call's "count" argument.
 *
 *  @param Always 1
 **/
unsigned int TessellationShaderInvarianceBaseTest::getDrawCallCountArgument()
{
	return 1;
}

/** Returns source code for fragment shader stage which does
 *  not do anything.
 *
 *  @param n_iteration Not used.
 *
 *  @return Requested string.
 **/
std::string TessellationShaderInvarianceBaseTest::getFSCode(unsigned int n_iteration)
{
	DE_UNREF(n_iteration);

	std::string result = "${VERSION}\n"
						 "\n"
						 "void main()\n"
						 "{\n"
						 "}\n";

	return result;
}

/** Retrieves name of a vec2 uniform that stores inner tesselaton level information,
 *  later assigned to gl_TessLevelInner in tessellation evaluation shader.
 *
 *  @return Requested name.
 **/
const char* TessellationShaderInvarianceBaseTest::getInnerTessLevelUniformName()
{
	static const char* result = "inner_tess_level";

	return result;
}

/** Retrieves name of a vec4 uniform that stores outer tesselation level information,
 *  later assigned to gl_TessLevelOuter in tessellation evaluation shader.
 *
 *  @return Requested name.
 **/
const char* TessellationShaderInvarianceBaseTest::getOuterTessLevelUniformName()
{
	static const char* result = "outer_tess_level";

	return result;
}

/** Returns generic tessellation control shader code, which sends 4 output patch
 *  to tessellation evaluation shader stage and uses the very first input patch
 *  vertex only.
 *
 *  @return Tessellation control source code.
 */
std::string TessellationShaderInvarianceBaseTest::getTCCode(unsigned int n_iteration)
{
	DE_UNREF(n_iteration);

	/* In order to support all three primitive types, our generic tessellation
	 * control shader will pass 4 vertices to TE stage */
	return TessellationShaderUtils::getGenericTCCode(4, /* n_patch_vertices */
													 false);
}

/** Retrieves XFB properties for the test pass.
 *
 *  @param n_iteration Not used.
 *  @param out_n_names Deref will be used to store amount of strings @param *out_n_names
 *                     offers.
 *  @param out_names   Deref will be used to store pointer to an array of strings holding
 *                     names of varyings that should be captured via transform feedback.
 *                     Must not be NULL.
 *
 **/
void TessellationShaderInvarianceBaseTest::getXFBProperties(unsigned int n_iteration, unsigned int* out_n_names,
															const char*** out_names)
{
	static const char* names[] = { "result_uvw" };

	DE_UNREF(n_iteration);

	*out_n_names = 1;
	*out_names   = names;
}

/** Returns vertex shader source code. The shader sets gl_Position to
 *  vec4(1, 2, 3, 0).
 *
 *  @return Vertex shader source code.
 **/
std::string TessellationShaderInvarianceBaseTest::getVSCode(unsigned int n_iteration)
{
	DE_UNREF(n_iteration);

	std::string result = "${VERSION}\n"
						 "\n"
						 "void main()\n"
						 "{\n"
						 "    gl_Position = vec4(1.0, 2.0, 3.0, 0.0);\n"
						 "}\n";

	return result;
}

/** Initializes ES objects required to execute the test.
 *
 *  Throws TestError exception if an error occurs.
 *
 **/
void TessellationShaderInvarianceBaseTest::initTest()
{
	const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();
	glw::GLuint			  shared_fs_id = 0;
	glw::GLuint			  shared_tc_id = 0;
	glw::GLuint			  shared_te_id = 0;
	glw::GLuint			  shared_vs_id = 0;

	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Initialize GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query object */
	gl.genQueries(1, &m_qo_tfpw_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries() call failed");

	/* Initialize tessellation shader utils */
	m_utils_ptr = new TessellationShaderUtils(gl, this);

	/* Initialize a buffer object we will use to store XFB data.
	 * Note: we intentionally skip a glBufferData() call here,
	 *       the actual buffer storage size is iteration-specific.
	 **/
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

	/* Iterate through all iterations */
	const unsigned int n_iterations = getAmountOfIterations();
	m_programs.reserve(n_iterations);

	const glw::GLenum SHADER_TYPE_FRAGMENT				  = GL_FRAGMENT_SHADER;
	const glw::GLenum SHADER_TYPE_TESSELLATION_CONTROL	= m_glExtTokens.TESS_CONTROL_SHADER;
	const glw::GLenum SHADER_TYPE_TESSELLATION_EVALUATION = m_glExtTokens.TESS_EVALUATION_SHADER;
	const glw::GLenum SHADER_TYPE_VERTEX				  = GL_VERTEX_SHADER;

	for (unsigned int n_iteration = 0; n_iteration < n_iterations; ++n_iteration)
	{
		_test_program program;

		/* Create an iteration-specific program object */
		program.po_id = gl.createProgram();

		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

		/* Query the implementation on which shader types should be compiled on
		 * a per-iteration basis, and which can be initialized only once */
		static const glw::GLenum shader_types[] = { SHADER_TYPE_FRAGMENT, SHADER_TYPE_TESSELLATION_CONTROL,
													SHADER_TYPE_TESSELLATION_EVALUATION, SHADER_TYPE_VERTEX };
		static const unsigned int n_shader_types = sizeof(shader_types) / sizeof(shader_types[0]);

		for (unsigned int n_shader_type = 0; n_shader_type < n_shader_types; ++n_shader_type)
		{
			std::string shader_body;
			const char* shader_body_ptr = DE_NULL;
			glw::GLuint shader_id		= 0;
			glw::GLenum shader_type		= shader_types[n_shader_type];
			glw::GLenum shader_type_es  = (glw::GLenum)shader_type;

			// Check whether the test should use a separate program objects for each iteration.
			bool is_shader_iteration_specific = false;
			if (shader_type == SHADER_TYPE_TESSELLATION_EVALUATION)
			{
				is_shader_iteration_specific = true;
			}
			else if ((shader_type != SHADER_TYPE_FRAGMENT) && (shader_type != SHADER_TYPE_TESSELLATION_CONTROL) &&
					 (shader_type != SHADER_TYPE_VERTEX))
			{
				TCU_FAIL("Unrecognized shader type");
			}

			/* We need to initialize the shader object if:
			 *
			 * - its body differs between iterations;
			 * - its body is shared by all iterations AND this is the first iteration
			 */
			bool has_shader_been_generated = false;

			if ((!is_shader_iteration_specific && n_iteration == 0) || is_shader_iteration_specific)
			{
				/* Create the shader object */
				has_shader_been_generated = true;
				shader_id				  = gl.createShader(shader_type_es);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed");

				/* Assign shader body to the object */
				if (shader_type == SHADER_TYPE_FRAGMENT)
				{
					shader_body = getFSCode(n_iteration);
				}
				else if (shader_type == SHADER_TYPE_TESSELLATION_CONTROL)
				{
					shader_body = getTCCode(n_iteration);
				}
				else if (shader_type == SHADER_TYPE_TESSELLATION_EVALUATION)
				{
					shader_body = getTECode(n_iteration);
				}
				else if (shader_type == SHADER_TYPE_VERTEX)
				{
					shader_body = getVSCode(n_iteration);
				}
				else
				{
					TCU_FAIL("Unrecognized shader type");
				}

				shader_body_ptr = shader_body.c_str();

				shaderSourceSpecialized(shader_id, 1, &shader_body_ptr);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

				/* Compile the shader object */
				m_utils_ptr->compileShaders(1,				   /* n_shaders */
											&shader_id, true); /* should_succeed */

				/* If this is a shader object that will be shared by all iterations, cache it
				 * in a dedicated variable */
				if (!is_shader_iteration_specific)
				{
					if (shader_type == SHADER_TYPE_FRAGMENT)
					{
						shared_fs_id = shader_id;
					}
					else if (shader_type == SHADER_TYPE_TESSELLATION_CONTROL)
					{
						shared_tc_id = shader_id;
					}
					else if (shader_type == SHADER_TYPE_TESSELLATION_EVALUATION)
					{
						shared_te_id = shader_id;
					}
					else if (shader_type == SHADER_TYPE_VERTEX)
					{
						shared_vs_id = shader_id;
					}
					else
					{
						TCU_FAIL("Unrecognized shader type");
					}
				} /* if (!is_shader_iteration_specific) */
			}	 /* if (shader object needs to be initialized) */
			else
			{
				shader_id = (shader_type == SHADER_TYPE_FRAGMENT) ?
								shared_fs_id :
								(shader_type == SHADER_TYPE_TESSELLATION_CONTROL) ?
								shared_tc_id :
								(shader_type == SHADER_TYPE_TESSELLATION_EVALUATION) ? shared_te_id : shared_vs_id;
			}

			/* Attach the shader object to iteration-specific program object */
			gl.attachShader(program.po_id, shader_id);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

			/* Now that the object has been attached, we can flag it for deletion */
			if (has_shader_been_generated)
			{
				gl.deleteShader(shader_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader() call failed");
			}
		} /* for (all shader types) */

		/* Set up transform feed-back */
		unsigned int n_xfb_names = 0;
		const char** xfb_names   = NULL;

		getXFBProperties(n_iteration, &n_xfb_names, &xfb_names);

		gl.transformFeedbackVaryings(program.po_id, n_xfb_names, xfb_names, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

		/* Try to link the program object */
		glw::GLint link_status = GL_FALSE;

		gl.linkProgram(program.po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed");

		gl.getProgramiv(program.po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed");

		if (link_status != GL_TRUE)
		{
			TCU_FAIL("Program linking failed");
		}

		/* Retrieve inner/outer tess level uniform locations */
		program.inner_tess_level_uniform_location =
			gl.getUniformLocation(program.po_id, getInnerTessLevelUniformName());
		program.outer_tess_level_uniform_location =
			gl.getUniformLocation(program.po_id, getOuterTessLevelUniformName());

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call(s) failed");

		/* Store the program object */
		m_programs.push_back(program);
	} /* for (all iterations) */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderInvarianceBaseTest::iterate(void)
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize all objects needed to run the test */
	initTest();

	/* Do a general set-up */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed.");

	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed for GL_PATCH_VERTICES_EXT pname");

	/* There are two types of verification supported by this base test implementation:
	 *
	 * - iteration-specific (verifyResultDataForIteration() )
	 * - global             (verifyResultData() )
	 *
	 * It is up to test implementation to decide which of the two (or perhaps both)
	 * entry-points it should overload and use for validating the result data.
	 */
	const unsigned int n_iterations   = getAmountOfIterations();
	char**			   iteration_data = new char*[n_iterations];

	/* Execute the test */
	for (unsigned int n_iteration = 0; n_iteration < n_iterations; ++n_iteration)
	{
		_test_program& program = m_programs[n_iteration];

		/* Retrieve iteration properties for current iteration */
		unsigned int						 bo_size			  = 0;
		float								 inner_tess_levels[2] = { 0 };
		bool								 is_point_mode		  = false;
		float								 outer_tess_levels[4] = { 0 };
		_tessellation_primitive_mode		 primitive_mode		  = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
		_tessellation_shader_vertex_ordering vertex_ordering	  = TESSELLATION_SHADER_VERTEX_ORDERING_UNKNOWN;

		getIterationProperties(n_iteration, inner_tess_levels, outer_tess_levels, &is_point_mode, &primitive_mode,
							   &vertex_ordering, &bo_size);

		DE_ASSERT(bo_size != 0);

		/* Activate iteration-specific program */
		gl.useProgram(program.po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

		/* Set up buffer object storage */
		{
			char* zero_bo_data = new char[bo_size];

			memset(zero_bo_data, 0, bo_size);

			gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, zero_bo_data, GL_STATIC_DRAW);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

			delete[] zero_bo_data;
			zero_bo_data = NULL;
		}

		/* Allocate space for iteration-specific data */
		iteration_data[n_iteration] = new char[bo_size];

		/* Configure inner/outer tessellation levels as requested for the iteration */
		gl.uniform2fv(program.inner_tess_level_uniform_location, 1, /* count */
					  inner_tess_levels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform2fv() call failed");

		gl.uniform4fv(program.outer_tess_level_uniform_location, 1, /* count */
					  outer_tess_levels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() call failed");

		/* Launch the TFPW query */
		gl.beginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_qo_tfpw_id);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"glBeginQuery() for GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN target failed.");

		/* Prepare for TF */
		glw::GLenum tf_mode = TessellationShaderUtils::getTFModeForPrimitiveMode(primitive_mode, is_point_mode);

		gl.beginTransformFeedback(tf_mode);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");
		{
			/* Execute the draw call */
			executeDrawCall(n_iteration);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Draw call failed");
		}
		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

		gl.endQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN) call failed");

		/* Make sure that we had sufficient amount of space in a buffer object we used to
		 * capture XFB data.
		 **/
		glw::GLuint n_tf_primitives_written = 0;
		glw::GLuint used_tf_bo_size			= 0;

		gl.getQueryObjectuiv(m_qo_tfpw_id, GL_QUERY_RESULT, &n_tf_primitives_written);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not retrieve GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query object result");

		if (is_point_mode)
		{
			used_tf_bo_size = static_cast<glw::GLuint>(n_tf_primitives_written * sizeof(float) * 3 /* components */);
		}
		else if (primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES)
		{
			used_tf_bo_size = static_cast<glw::GLuint>(n_tf_primitives_written * sizeof(float) * 2 /* vertices */ *
													   3 /* components */);
		}
		else
		{
			used_tf_bo_size = static_cast<glw::GLuint>(n_tf_primitives_written * sizeof(float) * 3 /* vertices */ *
													   3 /* components */);
		}

		if (used_tf_bo_size != bo_size)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Expected " << bo_size
							   << " to be filled with tessellation data, "
								  "only "
							   << used_tf_bo_size << "was used." << tcu::TestLog::EndMessage;

			TCU_FAIL("Amount of primitives generated during TF does not match amount of primitives that were expected"
					 " to be generated by the tessellator");
		}

		/* Map the buffer object we earlier bound to GL_TRANSFORM_FEEDBACK_BUFFER
		 * target into process space. */
		const void* xfb_data = gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
												 bo_size, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed");

		memcpy(iteration_data[n_iteration], xfb_data, bo_size);

		/* Unmap the buffer object */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");

		/* Ask the test implementation to verify the results */
		verifyResultDataForIteration(n_iteration, iteration_data[n_iteration]);
	} /* for (all iterations) */

	/* Now that we've executed all iterations, we can call a global verification
	 * entry-point */
	verifyResultData((const void**)iteration_data);

	/* At this point we're safe to release space allocated for data coming from
	 * all the iterations */
	for (unsigned int n_iteration = 0; n_iteration < n_iterations; ++n_iteration)
	{
		char* iter_data = (char*)iteration_data[n_iteration];
		delete[] iter_data;

		iteration_data[n_iteration] = DE_NULL;
	} /* for (all iterations) */

	delete[] iteration_data;
	iteration_data = DE_NULL;

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/* Does nothing (stub implementation)
 *
 * @param n_iteration Not used.
 * @param data        Not used.
 *
 **/
void TessellationShaderInvarianceBaseTest::verifyResultDataForIteration(unsigned int n_iteration, const void* data)
{
	DE_UNREF(n_iteration && data);

	/* Do nothing - this is just a stub. */
}

/* Does nothing (stub implementation)
 *
 * @param all_iterations_data Not used.
 *
 **/
void TessellationShaderInvarianceBaseTest::verifyResultData(const void** all_iterations_data)
{
	DE_UNREF(all_iterations_data);

	/* Do nothing - this is just a stub. */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TessellationShaderInvarianceRule1Test::TessellationShaderInvarianceRule1Test(Context&			  context,
																			 const ExtParameters& extParams)
	: TessellationShaderInvarianceBaseTest(context, extParams, "invariance_rule1",
										   "Verifies conformance with first invariance rule")
{
	/* Left blank intentionally */
}

/** Destructor. */
TessellationShaderInvarianceRule1Test::~TessellationShaderInvarianceRule1Test()
{
	/* Left blank intentionally */
}

/** Retrieves amount of iterations the base test implementation should run before
 *  calling global verification routine.
 *
 *  @return Always 6.
 **/
unsigned int TessellationShaderInvarianceRule1Test::getAmountOfIterations()
{
	return 6;
}

/** Returns a value that should be used for the draw call's "count" argument.
 *
 *  @param Always 3
 **/
unsigned int TessellationShaderInvarianceRule1Test::getDrawCallCountArgument()
{
	return 3;
}

/** Retrieves iteration-specific tessellation properties.
 *
 *  @param n_iteration            Iteration index to retrieve the properties for.
 *  @param out_inner_tess_levels  Deref will be used to store iteration-specific inner
 *                                tessellation level values. Must not be NULL.
 *  @param out_outer_tess_levels  Deref will be used to store iteration-specific outer
 *                                tessellation level values. Must not be NULL.
 *  @param out_point_mode         Deref will be used to store iteration-specific flag
 *                                telling whether point mode should be enabled for given pass.
 *                                Must not be NULL.
 *  @param out_primitive_mode     Deref will be used to store iteration-specific primitive
 *                                mode. Must not be NULL.
 *  @param out_vertex_ordering    Deref will be used to store iteration-specific vertex ordering.
 *                                Must not be NULL.
 *  @param out_result_buffer_size Deref will be used to store amount of bytes XFB buffer object
 *                                storage should offer for the draw call to succeed. Must not
 *                                be NULL.
 **/
void TessellationShaderInvarianceRule1Test::getIterationProperties(
	unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels, bool* out_point_mode,
	_tessellation_primitive_mode* out_primitive_mode, _tessellation_shader_vertex_ordering* out_vertex_ordering,
	unsigned int* out_result_buffer_size)
{
	*out_vertex_ordering = TESSELLATION_SHADER_VERTEX_ORDERING_CCW;

	switch (n_iteration)
	{
	case 0:
	case 5:
	{
		/* Triangles (point mode) */
		out_inner_tess_levels[0] = 1.0f;
		out_outer_tess_levels[0] = 1.0f;
		out_outer_tess_levels[1] = 1.0f;
		out_outer_tess_levels[2] = 1.0f;

		*out_point_mode		= true;
		*out_primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES;

		break;
	}

	case 1:
	case 3:
	{
		/* Lines */
		out_outer_tess_levels[0] = 1.0f;
		out_outer_tess_levels[1] = 1.0f;

		*out_point_mode		= false;
		*out_primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES;

		break;
	}

	case 2:
	case 4:
	{
		/* Triangles */
		out_inner_tess_levels[0] = 1.0f;
		out_outer_tess_levels[0] = 1.0f;
		out_outer_tess_levels[1] = 1.0f;
		out_outer_tess_levels[2] = 1.0f;

		*out_point_mode		= false;
		*out_primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognzied iteration index");
	}
	}

	*out_result_buffer_size = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
		*out_primitive_mode, out_inner_tess_levels, out_outer_tess_levels, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
		*out_point_mode);

	*out_result_buffer_size = static_cast<unsigned int>(*out_result_buffer_size * getDrawCallCountArgument() *
														3 /* components */ * sizeof(float));

	DE_ASSERT(*out_result_buffer_size != 0);
}

/** Retrieves iteration-specific tessellation evaluation shader code.
 *
 *  @param n_iteration Iteration index, for which the source code is being obtained.
 *
 *  @return Requested source code.
 **/
std::string TessellationShaderInvarianceRule1Test::getTECode(unsigned int n_iteration)
{
	unsigned int						 bo_size			  = 0;
	float								 inner_tess_levels[2] = { 0 };
	float								 outer_tess_levels[4] = { 0 };
	bool								 point_mode			  = false;
	_tessellation_primitive_mode		 primitive_mode		  = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
	_tessellation_shader_vertex_ordering vertex_ordering	  = TESSELLATION_SHADER_VERTEX_ORDERING_UNKNOWN;

	getIterationProperties(n_iteration, inner_tess_levels, outer_tess_levels, &point_mode, &primitive_mode,
						   &vertex_ordering, &bo_size);

	return TessellationShaderUtils::getGenericTECode(TESSELLATION_SHADER_VERTEX_SPACING_EQUAL, primitive_mode,
													 vertex_ordering, point_mode);
}

/** Verifies result data. Accesses data generated by all iterations.
 *
 *  Throws TestError exception if an error occurs.
 *
 *  @param all_iterations_data An array of pointers to buffers, holding gl_TessCoord
 *                             data generated by subsequent iterations.
 **/
void TessellationShaderInvarianceRule1Test::verifyResultData(const void** all_iterations_data)
{
	const float* lines_vertex_data_1 = (const float*)all_iterations_data[1];
	const float* lines_vertex_data_2 = (const float*)all_iterations_data[3];
	const float* point_vertex_data_1 = (const float*)all_iterations_data[0];
	const float* point_vertex_data_2 = (const float*)all_iterations_data[5];
	const float* tris_vertex_data_1  = (const float*)all_iterations_data[2];
	const float* tris_vertex_data_2  = (const float*)all_iterations_data[4];

	const unsigned int n_line_vertices  = 2 /* vertices per line segment */ * getDrawCallCountArgument(); /* lines */
	const unsigned int n_point_vertices = 1 /* vertices per point */ * getDrawCallCountArgument();		  /* points */
	const unsigned int n_tri_vertices   = 3 /* vertices per triangle */ * getDrawCallCountArgument(); /* triangles */
	const unsigned int vertex_size		= sizeof(float) * 3;										  /* components */

	/* Make sure the data sets match, given different draw call ordering */
	for (int n_type = 0; n_type < 3 /* lines, points, tris */; ++n_type)
	{
		const float* data1_ptr = DE_NULL;
		const float* data2_ptr = DE_NULL;
		std::string  data_type_string;
		unsigned int n_vertices = 0;

		switch (n_type)
		{
		case 0:
		{
			data1_ptr		 = lines_vertex_data_1;
			data2_ptr		 = lines_vertex_data_2;
			data_type_string = "Line";
			n_vertices		 = n_line_vertices;

			break;
		}

		case 1:
		{
			data1_ptr		 = point_vertex_data_1;
			data2_ptr		 = point_vertex_data_2;
			data_type_string = "Point";
			n_vertices		 = n_point_vertices;

			break;
		}

		case 2:
		{
			data1_ptr		 = tris_vertex_data_1;
			data2_ptr		 = tris_vertex_data_2;
			data_type_string = "Triangle";
			n_vertices		 = n_tri_vertices;

			break;
		}

		default:
		{
			TCU_FAIL("Internal error: type index was not recognized");
		}
		} /* switch (n_type) */

		/* Make sure the buffer storage in both cases has been modified */
		{
			unsigned int zero_bo_size = vertex_size * n_vertices;
			char*		 zero_bo_data = new char[vertex_size * n_vertices];

			memset(zero_bo_data, 0, zero_bo_size);

			if (memcmp(data1_ptr, zero_bo_data, zero_bo_size) == 0 ||
				memcmp(data2_ptr, zero_bo_data, zero_bo_size) == 0)
			{
				TCU_FAIL("One of the draw calls has not outputted any data to XFB buffer object storage");
			}

			delete[] zero_bo_data;
			zero_bo_data = NULL;
		}

		/* Compare the data */
		if (memcmp(data1_ptr, data2_ptr, vertex_size * n_vertices) != 0)
		{
			std::stringstream logMessage;

			logMessage << data_type_string << " data rendered in pass 1: (";

			for (unsigned int n_vertex = 0; n_vertex < n_vertices; ++n_vertex)
			{
				logMessage << data1_ptr[n_vertex];

				if (n_vertex != (n_vertices - 1))
				{
					logMessage << ", ";
				}
				else
				{
					logMessage << ") ";
				}
			} /* for (all vertices) */

			logMessage << "and in pass 2: (";

			for (unsigned int n_vertex = 0; n_vertex < n_vertices; ++n_vertex)
			{
				logMessage << data2_ptr[n_vertex];

				if (n_vertex != (n_vertices - 1))
				{
					logMessage << ", ";
				}
				else
				{
					logMessage << ") ";
				}
			} /* for (all vertices) */

			logMessage << "do not match";

			m_testCtx.getLog() << tcu::TestLog::Message << logMessage.str().c_str() << tcu::TestLog::EndMessage;

			TCU_FAIL("Data mismatch");
		} /* if (data mismatch) */
	}	 /* for (all primitive types) */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TessellationShaderInvarianceRule2Test::TessellationShaderInvarianceRule2Test(Context&			  context,
																			 const ExtParameters& extParams)
	: TessellationShaderInvarianceBaseTest(context, extParams, "invariance_rule2",
										   "Verifies conformance with second invariance rule")
{
	memset(m_n_tessellated_vertices, 0, sizeof(m_n_tessellated_vertices));
}

/** Destructor. */
TessellationShaderInvarianceRule2Test::~TessellationShaderInvarianceRule2Test()
{
	/* Left blank intentionally */
}

/** Retrieves amount of iterations the base test implementation should run before
 *  calling global verification routine.
 *
 *  @return Always 4.
 **/
unsigned int TessellationShaderInvarianceRule2Test::getAmountOfIterations()
{
	return 4;
}

/** Retrieves iteration-specific tessellation properties.
 *
 *  @param n_iteration            Iteration index to retrieve the properties for.
 *  @param out_inner_tess_levels  Deref will be used to store iteration-specific inner
 *                                tessellation level values. Must not be NULL.
 *  @param out_outer_tess_levels  Deref will be used to store iteration-specific outer
 *                                tessellation level values. Must not be NULL.
 *  @param out_point_mode         Deref will be used to store iteration-specific flag
 *                                telling whether point mode should be enabled for given pass.
 *                                Must not be NULL.
 *  @param out_primitive_mode     Deref will be used to store iteration-specific primitive
 *                                mode. Must not be NULL.
 *  @param out_vertex_ordering    Deref will be used to store iteration-specific vertex
 *                                ordering. Must not be NULL.
 *  @param out_result_buffer_size Deref will be used to store amount of bytes XFB buffer object
 *                                storage should offer for the draw call to succeed. Can
 *                                be NULL.
 **/
void TessellationShaderInvarianceRule2Test::getIterationProperties(
	unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels, bool* out_point_mode,
	_tessellation_primitive_mode* out_primitive_mode, _tessellation_shader_vertex_ordering* out_vertex_ordering,
	unsigned int* out_result_buffer_size)
{
	*out_vertex_ordering = TESSELLATION_SHADER_VERTEX_ORDERING_CCW;

	switch (n_iteration)
	{
	case 0:
	case 1:
	{
		/* Triangles */
		out_outer_tess_levels[0] = 2.0f;
		out_outer_tess_levels[1] = 3.0f;
		out_outer_tess_levels[2] = 4.0f;

		if (n_iteration == 0)
		{
			out_inner_tess_levels[0] = 4.0f;
			out_inner_tess_levels[1] = 5.0f;
		}
		else
		{
			out_inner_tess_levels[0] = 3.0f;
			out_inner_tess_levels[1] = 4.0f;
		}

		*out_point_mode		= false;
		*out_primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES;

		break;
	}

	case 2:
	case 3:
	{
		/* Quads */
		out_outer_tess_levels[0] = 2.0f;
		out_outer_tess_levels[1] = 3.0f;
		out_outer_tess_levels[2] = 4.0f;
		out_outer_tess_levels[3] = 5.0f;

		if (n_iteration == 2)
		{
			out_inner_tess_levels[0] = 2.0f;
			out_inner_tess_levels[1] = 3.0f;
		}
		else
		{
			out_inner_tess_levels[0] = 4.0f;
			out_inner_tess_levels[1] = 5.0f;
		}

		*out_point_mode		= false;
		*out_primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized iteration index");
	}
	}

	if (out_result_buffer_size != DE_NULL)
	{
		*out_result_buffer_size = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
			*out_primitive_mode, out_inner_tess_levels, out_outer_tess_levels, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
			*out_point_mode);

		m_n_tessellated_vertices[n_iteration] = *out_result_buffer_size;
		*out_result_buffer_size =
			static_cast<unsigned int>(*out_result_buffer_size * 3 /* components */ * sizeof(float));

		DE_ASSERT(*out_result_buffer_size != 0);
	}
}

/** Retrieves iteration-specific tessellation evaluation shader code.
 *
 *  @param n_iteration Iteration index, for which the source code is being obtained.
 *
 *  @return Requested source code.
 **/
std::string TessellationShaderInvarianceRule2Test::getTECode(unsigned int n_iteration)
{
	unsigned int						 bo_size			  = 0;
	float								 inner_tess_levels[2] = { 0 };
	float								 outer_tess_levels[4] = { 0 };
	bool								 point_mode			  = false;
	_tessellation_primitive_mode		 primitive_mode		  = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
	_tessellation_shader_vertex_ordering vertex_ordering	  = TESSELLATION_SHADER_VERTEX_ORDERING_UNKNOWN;

	getIterationProperties(n_iteration, inner_tess_levels, outer_tess_levels, &point_mode, &primitive_mode,
						   &vertex_ordering, &bo_size);

	return TessellationShaderUtils::getGenericTECode(TESSELLATION_SHADER_VERTEX_SPACING_EQUAL, primitive_mode,
													 vertex_ordering, point_mode);
}

/** Verifies result data. Accesses data generated by all iterations.
 *
 *  Throws TestError exception if an error occurs.
 *
 *  @param all_iterations_data An array of pointers to buffers, holding gl_TessCoord
 *                             data generated by subsequent iterations.
 **/
void TessellationShaderInvarianceRule2Test::verifyResultData(const void** all_iterations_data)
{
	/* Iterate through one tessellated set of vertices for a given primitive type
	 * and identify outer vertices. Make sure exactly the same vertices can be
	 * found in the other set of vertices, generated with different input tessellation
	 * level.
	 */
	for (int n_primitive_type = 0; n_primitive_type < 2; /* triangles / quads */
		 ++n_primitive_type)
	{
		const float*				 data1_ptr		= (const float*)all_iterations_data[n_primitive_type * 2 + 0];
		const float*				 data2_ptr		= (const float*)all_iterations_data[n_primitive_type * 2 + 1];
		_tessellation_primitive_mode primitive_mode = (n_primitive_type == 0) ?
														  TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES :
														  TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS;
		std::vector<_vertex> outer_vertices;

		/* Iterate through all data1 vertices.. */
		for (unsigned int n_vertex = 0; n_vertex < m_n_tessellated_vertices[n_primitive_type * 2]; ++n_vertex)
		{
			/* Check if any of the components is equal to 0 or 1. If so, this could
			 * be an edge vertex.
			 */
			const float* vertex_ptr = data1_ptr + n_vertex * 3; /* components */

			if (TessellationShaderUtils::isOuterEdgeVertex(primitive_mode, vertex_ptr))
			{
				/* Only add the vertex if it has not been added already to the vector */
				bool has_already_been_added = false;

				for (std::vector<_vertex>::const_iterator vertex_iterator = outer_vertices.begin();
					 vertex_iterator != outer_vertices.end(); vertex_iterator++)
				{
					if (vertex_iterator->u == vertex_ptr[0] && vertex_iterator->v == vertex_ptr[1] &&
						vertex_iterator->w == vertex_ptr[2])
					{
						has_already_been_added = true;

						break;
					}
				} /* for (all outer vertices stored so far) */

				if (!has_already_been_added)
				{
					_vertex vertex;

					vertex.u = vertex_ptr[0];
					vertex.v = vertex_ptr[1];
					vertex.w = vertex_ptr[2];

					outer_vertices.push_back(vertex);
				}
			} /* if (input vertex is located on outer edge) */
		}	 /* for (all input vertices) */

		DE_ASSERT(outer_vertices.size() != 0);

		/* Now that we know where outer vertices are, make sure they are present in the other data set */
		for (std::vector<_vertex>::const_iterator ref_vertex_iterator = outer_vertices.begin();
			 ref_vertex_iterator != outer_vertices.end(); ref_vertex_iterator++)
		{
			bool		   has_been_found = false;
			const _vertex& ref_vertex	 = *ref_vertex_iterator;

			for (unsigned int n_vertex = 0; n_vertex < m_n_tessellated_vertices[n_primitive_type * 2 + 1]; ++n_vertex)
			{
				const float* vertex_ptr = data2_ptr + n_vertex * 3; /* components */

				if (vertex_ptr[0] == ref_vertex.u && vertex_ptr[1] == ref_vertex.v && vertex_ptr[2] == ref_vertex.w)
				{
					has_been_found = true;

					break;
				}
			} /* for (all vertices in the other data set) */

			if (!has_been_found)
			{
				float								 cmp_inner_tess_levels[2];
				float								 cmp_outer_tess_levels[4];
				bool								 cmp_point_mode;
				_tessellation_primitive_mode		 cmp_primitive_mode;
				_tessellation_shader_vertex_ordering cmp_vertex_ordering;
				std::string							 primitive_type = (n_primitive_type == 0) ? "triangles" : "quads";
				float								 ref_inner_tess_levels[2];
				float								 ref_outer_tess_levels[4];
				bool								 ref_point_mode;
				_tessellation_primitive_mode		 ref_primitive_mode;
				_tessellation_shader_vertex_ordering ref_vertex_ordering;

				getIterationProperties(n_primitive_type * 2, ref_inner_tess_levels, ref_outer_tess_levels,
									   &ref_point_mode, &ref_primitive_mode, &ref_vertex_ordering, NULL);
				getIterationProperties(n_primitive_type * 2 + 1, cmp_inner_tess_levels, cmp_outer_tess_levels,
									   &cmp_point_mode, &cmp_primitive_mode, &cmp_vertex_ordering, NULL);

				m_testCtx.getLog() << tcu::TestLog::Message << "Outer vertex"
								   << " (" << ref_vertex.u << ", " << ref_vertex.v << ", " << ref_vertex.w << ")"
								   << " was not found in tessellated data coordinate set generated"
								   << " for primitive type: " << primitive_type.c_str()
								   << ". Reference inner tessellation level:"
								   << " (" << ref_inner_tess_levels[0] << ", " << ref_inner_tess_levels[1]
								   << "), reference outer tessellation level:"
								   << " (" << ref_outer_tess_levels[0] << ", " << ref_outer_tess_levels[1] << ", "
								   << ref_outer_tess_levels[2] << ", " << ref_outer_tess_levels[3]
								   << "), test inner tessellation level:"
								   << " (" << cmp_inner_tess_levels[0] << ", " << cmp_inner_tess_levels[1]
								   << "), reference outer tessellation level:"
								   << " (" << cmp_outer_tess_levels[0] << ", " << cmp_outer_tess_levels[1] << ", "
								   << cmp_outer_tess_levels[2] << ", " << cmp_outer_tess_levels[3] << ")."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Outer edge vertex was not found in tessellated coordinate set generated for "
						 "the same outer tessellation levels and vertex spacing, but different inner "
						 "tessellation levels");
			} /* if (outer edge vertex was not found in the other set) */
		}	 /* for (all outer edge vertices) */
	}		  /* for (both primitive types) */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TessellationShaderInvarianceRule3Test::TessellationShaderInvarianceRule3Test(Context&			  context,
																			 const ExtParameters& extParams)
	: TessellationShaderInvarianceBaseTest(context, extParams, "invariance_rule3",
										   "Verifies conformance with third invariance rule")
{
}

/** Destructor. */
TessellationShaderInvarianceRule3Test::~TessellationShaderInvarianceRule3Test()
{
	/* Left blank intentionally */
}

/** Retrieves amount of iterations the base test implementation should run before
 *  calling global verification routine.
 *
 *  @return A value that depends on initTestIterations() behavior.
 **/
unsigned int TessellationShaderInvarianceRule3Test::getAmountOfIterations()
{
	if (m_test_iterations.size() == 0)
	{
		initTestIterations();
	}

	return (unsigned int)m_test_iterations.size();
}

/** Retrieves iteration-specific tessellation properties.
 *
 *  @param n_iteration            Iteration index to retrieve the properties for.
 *  @param out_inner_tess_levels  Deref will be used to store iteration-specific inner
 *                                tessellation level values. Must not be NULL.
 *  @param out_outer_tess_levels  Deref will be used to store iteration-specific outer
 *                                tessellation level values. Must not be NULL.
 *  @param out_point_mode         Deref will be used to store iteration-specific flag
 *                                telling whether point mode should be enabled for given pass.
 *                                Must not be NULL.
 *  @param out_primitive_mode     Deref will be used to store iteration-specific primitive
 *                                mode. Must not be NULL.
 *  @param out_vertex_ordering    Deref will be used to store iteration-specific vertex
 *                                ordering. Must not be NULL.
 *  @param out_result_buffer_size Deref will be used to store amount of bytes XFB buffer object
 *                                storage should offer for the draw call to succeed. Can
 *                                be NULL.
 **/
void TessellationShaderInvarianceRule3Test::getIterationProperties(
	unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels, bool* out_point_mode,
	_tessellation_primitive_mode* out_primitive_mode, _tessellation_shader_vertex_ordering* out_vertex_ordering,
	unsigned int* out_result_buffer_size)
{
	DE_ASSERT(m_test_iterations.size() > n_iteration);

	_test_iteration& test_iteration = m_test_iterations[n_iteration];

	memcpy(out_inner_tess_levels, test_iteration.inner_tess_levels, sizeof(test_iteration.inner_tess_levels));
	memcpy(out_outer_tess_levels, test_iteration.outer_tess_levels, sizeof(test_iteration.outer_tess_levels));

	*out_point_mode		 = false;
	*out_primitive_mode  = test_iteration.primitive_mode;
	*out_vertex_ordering = TESSELLATION_SHADER_VERTEX_ORDERING_CCW;

	if (out_result_buffer_size != DE_NULL)
	{
		*out_result_buffer_size = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
			*out_primitive_mode, out_inner_tess_levels, out_outer_tess_levels, test_iteration.vertex_spacing,
			*out_point_mode);
		test_iteration.n_vertices = *out_result_buffer_size;
		*out_result_buffer_size =
			static_cast<unsigned int>(*out_result_buffer_size * 3 /* components */ * sizeof(float));

		DE_ASSERT(*out_result_buffer_size != 0);
	}
}

/** Retrieves iteration-specific tessellation evaluation shader code.
 *
 *  @param n_iteration Iteration index, for which the source code is being obtained.
 *
 *  @return Requested source code.
 **/
std::string TessellationShaderInvarianceRule3Test::getTECode(unsigned int n_iteration)
{
	DE_ASSERT(m_test_iterations.size() > n_iteration);

	const _test_iteration& test_iteration = m_test_iterations[n_iteration];

	return TessellationShaderUtils::getGenericTECode(test_iteration.vertex_spacing, test_iteration.primitive_mode,
													 TESSELLATION_SHADER_VERTEX_ORDERING_CCW, false); /* point mode */
}

/** Initializes test iterations for the test. The following modes and inner/outer tess level
 *  configurations are used to form the test set:
 *
 *  - Inner/outer tessellation level combinations as returned by
 *    TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode()
 *  - All primitive modes;
 *  - All vertex spacing modes;
 *
 *  All permutations are used to generate the test set.
 **/
void TessellationShaderInvarianceRule3Test::initTestIterations()
{
	DE_ASSERT(m_test_iterations.size() == 0);

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	const glw::Functions& gl						  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Iterate through all primitive and vertex spacing modes */
	_tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES,
													   TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
													   TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	_tessellation_shader_vertex_spacing vs_modes[] = { TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
													   TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN,
													   TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD };

	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);
	const unsigned int n_vs_modes		 = sizeof(vs_modes) / sizeof(vs_modes[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];

		for (unsigned int n_vs_mode = 0; n_vs_mode < n_vs_modes; ++n_vs_mode)
		{
			_tessellation_shader_vertex_spacing vs_mode = vs_modes[n_vs_mode];

			/* Retrieve inner/outer tessellation level combinations we want the tests to be run for */
			_tessellation_levels_set levels_set;

			levels_set = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
				primitive_mode, gl_max_tess_gen_level_value,
				TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

			/* Iterate through all configurations */
			for (_tessellation_levels_set_const_iterator levels_iterator = levels_set.begin();
				 levels_iterator != levels_set.end(); levels_iterator++)
			{
				const _tessellation_levels& levels = *levels_iterator;

				/* Create a test descriptor for all the parameters we now have */
				_test_iteration test;

				memcpy(test.inner_tess_levels, levels.inner, sizeof(test.inner_tess_levels));
				memcpy(test.outer_tess_levels, levels.outer, sizeof(test.outer_tess_levels));

				test.primitive_mode = primitive_mode;
				test.vertex_spacing = vs_mode;

				m_test_iterations.push_back(test);
			} /* for (all inner/outer tessellation levels) */
		}	 /* for (all vertex spacing modes) */
	}		  /* for (all primitive modes) */
}

/** Verifies result data on per-iteration basis.
 *
 *  Throws TestError exception if an error occurs.
 *
 *  @param n_iteration Index of iteration the check should be performed for.
 *  @param data        Points to array of vec3s storing the vertices as
 *                     generated by tessellation
 **/
void TessellationShaderInvarianceRule3Test::verifyResultDataForIteration(unsigned int n_iteration, const void* data)
{
	DE_ASSERT(m_test_iterations.size() > n_iteration);

	const glw::GLfloat*	data_float	 = (const glw::GLfloat*)data;
	const _test_iteration& test_iteration = m_test_iterations[n_iteration];

	/* Iterate through all generated vertices.. */
	for (unsigned int n_vertex = 0; n_vertex < test_iteration.n_vertices; ++n_vertex)
	{
		_vertex				expected_vertex;
		const glw::GLfloat* vertex_data = data_float + 3 /* components */ * n_vertex;

		expected_vertex.u = -1.0f;
		expected_vertex.v = -1.0f;
		expected_vertex.w = -1.0f;

		/* Make sure that for each vertex, the following language from the extension
		 * spec is followed:
		 */
		switch (test_iteration.primitive_mode)
		{
		case TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES:
		{
			/* For isoline tessellation, if it generates vertices at (0,x) and (1,x)
			 * where <x> is not zero, it will also generate vertices at exactly (0,1-x)
			 * and (1,1-x), respectively.
			 */
			if (vertex_data[0] == 0.0f && vertex_data[1] != 0.0f)
			{
				expected_vertex.u = vertex_data[0];
				expected_vertex.v = 1.0f - vertex_data[1];
				expected_vertex.w = -1.0f;
			}
			else if (vertex_data[0] == 1.0f && vertex_data[1] != 0.0f)
			{
				expected_vertex.u = vertex_data[0];
				expected_vertex.v = 1.0f - vertex_data[1];
				expected_vertex.w = -1.0f;
			}

			break;
		} /* case TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES: */

		case TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS:
		{
			/* For quad tessellation, if the subdivision generates a vertex with
			 * coordinates of (x,0) or (0,x), it will also generate a vertex with
			 * coordinates of exactly (1-x,0) or (0,1-x), respectively.
			 */
			if (vertex_data[0] != 0.0f && vertex_data[1] == 0.0f)
			{
				expected_vertex.u = 1.0f - vertex_data[0];
				expected_vertex.v = vertex_data[1];
				expected_vertex.w = -1.0f;
			}
			else if (vertex_data[0] == 0.0f && vertex_data[1] != 0.0f)
			{
				expected_vertex.u = vertex_data[0];
				expected_vertex.v = 1.0f - vertex_data[1];
				expected_vertex.w = -1.0f;
			}

			break;
		} /* case TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS: */

		case TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES:
		{
			/* For triangle tessellation, if the subdivision generates a vertex with
			 * tessellation coordinates of the form (0,x,1-x), (x,0,1-x), or (x,1-x,0),
			 * it will also generate a vertex with coordinates of exactly (0,1-x,x),
			 * (1-x,0,x), or (1-x,x,0), respectively.
			 */
			if (vertex_data[0] == 0.0f && vertex_data[1] != 0.0f && vertex_data[2] == (1.0f - vertex_data[1]))
			{
				expected_vertex.u = vertex_data[0];
				expected_vertex.v = vertex_data[2];
				expected_vertex.w = vertex_data[1];
			}
			else if (vertex_data[0] != 0.0f && vertex_data[1] == 0.0f && vertex_data[2] == (1.0f - vertex_data[0]))
			{
				expected_vertex.u = vertex_data[2];
				expected_vertex.v = vertex_data[1];
				expected_vertex.w = vertex_data[0];
			}
			else if (vertex_data[0] != 0.0f && vertex_data[1] == (1.0f - vertex_data[0]) && vertex_data[2] == 0.0f)
			{
				expected_vertex.u = vertex_data[1];
				expected_vertex.v = vertex_data[0];
				expected_vertex.w = vertex_data[2];
			}

			break;
		} /* case TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES: */

		default:
		{
			TCU_FAIL("Primitive mode unrecognized");
		}
		} /* switch (test_iteration.primitive_mode) */

		/* If any of the "expected vertex"'s components is no longer negative,
		 * make sure the vertex can be found in the result data */
		if (expected_vertex.u >= 0.0f || expected_vertex.v >= 0.0f || expected_vertex.w >= 0.0f)
		{
			bool has_been_found = false;

			for (unsigned int n_find_vertex = 0; n_find_vertex < test_iteration.n_vertices; ++n_find_vertex)
			{
				const glw::GLfloat* current_vertex_data = data_float + 3 /* components */ * n_find_vertex;

				const glw::GLfloat epsilon	 = 1e-4f;
				glw::GLfloat	   absDelta[3] = { de::abs(current_vertex_data[0] - expected_vertex.u),
											 de::abs(current_vertex_data[1] - expected_vertex.v),
											 de::abs(current_vertex_data[2] - expected_vertex.w) };

				if (absDelta[0] < epsilon && absDelta[1] < epsilon &&
					((expected_vertex.w < 0.0f) || (expected_vertex.w >= 0.0f && absDelta[2] < epsilon)))
				{
					has_been_found = true;

					break;
				} /* if (the vertex data matches the expected vertex data) */
			}	 /* for (all generated vertices)*/

			if (!has_been_found)
			{
				TCU_FAIL("Expected symmetrical vertex data was not generated by the tessellator.");
			}
		} /* if (any of the components of expected_vertex is no longer negative) */
	}	 /* for (all generated vertices) */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TessellationShaderInvarianceRule4Test::TessellationShaderInvarianceRule4Test(Context&			  context,
																			 const ExtParameters& extParams)
	: TessellationShaderInvarianceBaseTest(context, extParams, "invariance_rule4",
										   "Verifies conformance with fourth invariance rule")
{
}

/** Destructor. */
TessellationShaderInvarianceRule4Test::~TessellationShaderInvarianceRule4Test()
{
	/* Left blank intentionally */
}

/** Retrieves amount of iterations the base test implementation should run before
 *  calling global verification routine.
 *
 *  @return A value that depends on initTestIterations() behavior.
 **/
unsigned int TessellationShaderInvarianceRule4Test::getAmountOfIterations()
{
	if (m_test_iterations.size() == 0)
	{
		initTestIterations();
	}

	return (unsigned int)m_test_iterations.size();
}

/** Retrieves iteration-specific tessellation properties.
 *
 *  @param n_iteration            Iteration index to retrieve the properties for.
 *  @param out_inner_tess_levels  Deref will be used to store iteration-specific inner
 *                                tessellation level values. Must not be NULL.
 *  @param out_outer_tess_levels  Deref will be used to store iteration-specific outer
 *                                tessellation level values. Must not be NULL.
 *  @param out_point_mode         Deref will be used to store iteration-specific flag
 *                                telling whether point mode should be enabled for given pass.
 *                                Must not be NULL.
 *  @param out_primitive_mode     Deref will be used to store iteration-specific primitive
 *                                mode. Must not be NULL.
 *  @param out_vertex_ordering    Deref will be used to store iteration-specific vertex
 *                                ordering. Must not be NULL.
 *  @param out_result_buffer_size Deref will be used to store amount of bytes XFB buffer object
 *                                storage should offer for the draw call to succeed. Can
 *                                be NULL.
 **/
void TessellationShaderInvarianceRule4Test::getIterationProperties(
	unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels, bool* out_point_mode,
	_tessellation_primitive_mode* out_primitive_mode, _tessellation_shader_vertex_ordering* out_vertex_ordering,
	unsigned int* out_result_buffer_size)
{
	DE_ASSERT(m_test_iterations.size() > n_iteration);

	_test_iteration& test_iteration = m_test_iterations[n_iteration];

	memcpy(out_inner_tess_levels, test_iteration.inner_tess_levels, sizeof(test_iteration.inner_tess_levels));
	memcpy(out_outer_tess_levels, test_iteration.outer_tess_levels, sizeof(test_iteration.outer_tess_levels));

	*out_point_mode		 = false;
	*out_primitive_mode  = test_iteration.primitive_mode;
	*out_vertex_ordering = TESSELLATION_SHADER_VERTEX_ORDERING_CCW;

	if (out_result_buffer_size != DE_NULL)
	{
		*out_result_buffer_size = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
			*out_primitive_mode, out_inner_tess_levels, out_outer_tess_levels, test_iteration.vertex_spacing,
			*out_point_mode);
		test_iteration.n_vertices = *out_result_buffer_size;
		*out_result_buffer_size =
			static_cast<unsigned int>(*out_result_buffer_size * 3 /* components */ * sizeof(float));

		DE_ASSERT(*out_result_buffer_size != 0);
	}
}

/** Retrieves iteration-specific tessellation evaluation shader code.
 *
 *  @param n_iteration Iteration index, for which the source code is being obtained.
 *
 *  @return Requested source code.
 **/
std::string TessellationShaderInvarianceRule4Test::getTECode(unsigned int n_iteration)
{
	DE_ASSERT(m_test_iterations.size() > n_iteration);

	const _test_iteration& test_iteration = m_test_iterations[n_iteration];

	return TessellationShaderUtils::getGenericTECode(test_iteration.vertex_spacing, test_iteration.primitive_mode,
													 TESSELLATION_SHADER_VERTEX_ORDERING_CCW, false); /* point mode */
}

/** Initializes test iterations for the test. The following modes and inner/outer tess level
 *  configurations are used to form the test set:
 *
 *  - Inner/outer tessellation level combinations as returned by
 *    TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode()
 *  - 'Quads' and 'Triangles' primitive modes;
 *  - All vertex spacing modes;
 *
 *  All permutations are used to generate the test set.
 **/
void TessellationShaderInvarianceRule4Test::initTestIterations()
{
	DE_ASSERT(m_test_iterations.size() == 0);

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	const glw::Functions& gl						  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Iterate through all primitive and vertex spacing modes relevant to the test */
	_tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
													   TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	_tessellation_shader_vertex_spacing vs_modes[] = { TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
													   TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN,
													   TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD };

	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);
	const unsigned int n_vs_modes		 = sizeof(vs_modes) / sizeof(vs_modes[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];

		for (unsigned int n_vs_mode = 0; n_vs_mode < n_vs_modes; ++n_vs_mode)
		{
			_tessellation_shader_vertex_spacing vs_mode = vs_modes[n_vs_mode];

			/* Retrieve inner/outer tessellation level combinations we want the tests to be run for */
			_tessellation_levels_set levels;

			levels = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
				primitive_mode, gl_max_tess_gen_level_value,
				TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

			/* Iterate through all configurations */
			for (_tessellation_levels_set_const_iterator levels_iterator = levels.begin();
				 levels_iterator != levels.end(); levels_iterator++)
			{
				const _tessellation_levels& current_levels = *levels_iterator;

				/* Create a test descriptor for all the parameters we now have.
				 *
				 * The set we're operating on uses different outer tessellation level values, so we
				 * need to make sure the levels are set to the same FP values in order for the test
				 * to succeed. */
				_test_iteration test;

				memcpy(test.inner_tess_levels, current_levels.inner, sizeof(test.inner_tess_levels));

				for (int n = 0; n < 4 /* outer tess levels */; ++n)
				{
					test.outer_tess_levels[n] = current_levels.outer[0];
				}

				test.primitive_mode = primitive_mode;
				test.vertex_spacing = vs_mode;

				m_test_iterations.push_back(test);
			} /* for (all inner/outer tessellation levels) */
		}	 /* for (all vertex spacing modes) */
	}		  /* for (all primitive modes) */
}

/** Verifies user-provided vertex data can be found in the provided vertex data array.
 *
 *  @param vertex_data                     Vertex data array the requested vertex data are to be found in.
 *  @param n_vertices                      Amount of vertices declared in @param vertex_data;
 *  @param vertex_data_seeked              Vertex data to be found in @param vertex_data;
 *  @param n_vertex_data_seeked_components Amount of components to take into account.
 *
 *  @return true if the vertex data was found, false otherwise.
 **/
bool TessellationShaderInvarianceRule4Test::isVertexDefined(const float* vertex_data, unsigned int n_vertices,
															const float* vertex_data_seeked,
															unsigned int n_vertex_data_seeked_components)
{
	bool result = false;

	DE_ASSERT(n_vertex_data_seeked_components >= 2);

	for (unsigned int n_vertex = 0; n_vertex < n_vertices; ++n_vertex)
	{
		const float* current_vertex_data = vertex_data + 3 /* components */ * n_vertex;

		if ((vertex_data_seeked[0] == current_vertex_data[0]) && (vertex_data_seeked[1] == current_vertex_data[1]) &&
			((n_vertex_data_seeked_components < 3) ||
			 (n_vertex_data_seeked_components >= 3 && (vertex_data_seeked[2] == current_vertex_data[2]))))
		{
			result = true;

			break;
		} /* if (components match) */
	}	 /* for (all vertices) */

	return result;
}

/** Verifies result data on per-iteration basis.
 *
 *  Throws TestError exception if an error occurs.
 *
 *  @param n_iteration Index of iteration the check should be performed for.
 *  @param data        Points to array of vec3s storing the vertices as
 *                     generated by tessellation
 **/
void TessellationShaderInvarianceRule4Test::verifyResultDataForIteration(unsigned int n_iteration, const void* data)
{
	DE_ASSERT(m_test_iterations.size() > n_iteration);

	const glw::GLfloat*	data_float	 = (const glw::GLfloat*)data;
	const _test_iteration& test_iteration = m_test_iterations[n_iteration];

	/* Iterate through all generated vertices.. */
	for (unsigned int n_vertex = 0; n_vertex < test_iteration.n_vertices; ++n_vertex)
	{
		std::vector<_vertex> expected_vertices;
		const glw::GLfloat*  vertex_data = data_float + 3 /* components */ * n_vertex;

		/* Make sure that for each vertex, the following language from the extension
		 * spec is followed:
		 */
		switch (test_iteration.primitive_mode)
		{
		case TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS:
		{
			/* For quad tessellation, if vertices at (x,0) and (1-x,0) are generated
			 * when subdividing the v==0 edge, vertices must be generated at (0,x) and
			 * (0,1-x) when subdividing an otherwise identical u==0 edge.
			 */
			if (vertex_data[0] != 0.0f && vertex_data[1] == 0.0f)
			{
				const float paired_vertex_data[] = { 1.0f - vertex_data[0], vertex_data[1] };

				if (isVertexDefined(data_float, test_iteration.n_vertices, paired_vertex_data, 2)) /* components */
				{
					_vertex expected_vertex;

					/* First expected vertex */
					expected_vertex.u = vertex_data[1];
					expected_vertex.v = vertex_data[0];
					expected_vertex.w = -1.0f;

					expected_vertices.push_back(expected_vertex);

					/* The other expected vertex */
					expected_vertex.u = vertex_data[1];
					expected_vertex.v = 1.0f - vertex_data[0];

					expected_vertices.push_back(expected_vertex);
				} /* if (the other required vertex is defined) */
			}	 /* if (the first required vertex is defined) */

			break;
		} /* case TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS: */

		case TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES:
		{
			/* For triangular tessellation, if vertices at (x,1-x,0) and (1-x,x,0) are
			 * generated when subdividing the w==0 edge, vertices must be generated at
			 * (x,0,1-x) and (1-x,0,x) when subdividing an otherwise identical v==0
			 * edge.
			 */
			if (vertex_data[0] != 0.0f && vertex_data[1] == (1.0f - vertex_data[0]) && vertex_data[2] == 0)
			{
				const float paired_vertex_data[] = { vertex_data[1], vertex_data[0], vertex_data[2] };

				if (isVertexDefined(data_float, test_iteration.n_vertices, paired_vertex_data, 3)) /* components */
				{
					_vertex expected_vertex;

					/* First expected vertex */
					expected_vertex.u = vertex_data[0];
					expected_vertex.v = vertex_data[2];
					expected_vertex.w = vertex_data[1];

					expected_vertices.push_back(expected_vertex);

					/* The other expected vertex */
					expected_vertex.u = vertex_data[1];
					expected_vertex.v = vertex_data[2];
					expected_vertex.w = vertex_data[0];

					expected_vertices.push_back(expected_vertex);
				} /* if (the other required vertex is defined) */
			}	 /* if (the firsst required vertex is defined) */

			break;
		} /* case TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES: */

		default:
		{
			TCU_FAIL("Primitive mode unrecognized");
		}
		} /* switch (test_iteration.primitive_mode) */

		/* Iterate through all expected vertices */
		for (std::vector<_vertex>::const_iterator expected_vertex_iterator = expected_vertices.begin();
			 expected_vertex_iterator != expected_vertices.end(); expected_vertex_iterator++)
		{
			const _vertex& expected_vertex		 = *expected_vertex_iterator;
			const float	expected_vertex_raw[] = { expected_vertex.u, expected_vertex.v, expected_vertex.w };
			bool		   has_been_found		 = false;

			has_been_found = isVertexDefined(data_float, test_iteration.n_vertices, expected_vertex_raw,
											 (expected_vertex.w < 0) ? 2 : 3);

			if (!has_been_found)
			{
				std::stringstream expected_vertex_sstream;

				expected_vertex_sstream << expected_vertex.u << ", " << expected_vertex.v;

				if (expected_vertex.w >= 0.0f)
				{
					expected_vertex_sstream << ", " << expected_vertex.w;
				}

				m_testCtx.getLog() << tcu::TestLog::Message << "For primitive mode:"
								   << "["
								   << TessellationShaderUtils::getESTokenForPrimitiveMode(test_iteration.primitive_mode)
								   << "]"
								   << "and vertex spacing mode:"
								   << "[" << TessellationShaderUtils::getESTokenForVertexSpacingMode(
												 test_iteration.vertex_spacing)
								   << "]"
								   << " and inner tessellation levels:[" << test_iteration.inner_tess_levels[0] << ", "
								   << test_iteration.inner_tess_levels[1] << ") "
								   << " and outer tessellation levels:[" << test_iteration.outer_tess_levels[0] << ", "
								   << test_iteration.outer_tess_levels[1] << ", " << test_iteration.outer_tess_levels[2]
								   << ", " << test_iteration.outer_tess_levels[3] << ") "
								   << " the following vertex was expected:[" << expected_vertex_sstream.str().c_str()
								   << "] but was not found in the tessellated cooordinate data set"
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Expected symmetrical vertex data was not generated by the tessellator.");
			} /* if (the expected vertex data was not found) */
		}	 /* for (all expected vertices) */
	}		  /* for (all generated vertices) */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TessellationShaderInvarianceRule5Test::TessellationShaderInvarianceRule5Test(Context&			  context,
																			 const ExtParameters& extParams)
	: TessellationShaderInvarianceBaseTest(context, extParams, "invariance_rule5",
										   "Verifies conformance with fifth invariance rule")
{
}

/** Destructor. */
TessellationShaderInvarianceRule5Test::~TessellationShaderInvarianceRule5Test()
{
	/* Left blank intentionally */
}

/** Retrieves amount of iterations the base test implementation should run before
 *  calling global verification routine.
 *
 *  @return A value that depends on initTestIterations() behavior.
 **/
unsigned int TessellationShaderInvarianceRule5Test::getAmountOfIterations()
{
	if (m_test_quads_iterations.size() == 0 || m_test_triangles_iterations.size() == 0)
	{
		initTestIterations();
	}

	return (unsigned int)(m_test_quads_iterations.size() + m_test_triangles_iterations.size());
}

/** Retrieves _test_iteration instance specific for user-specified iteration index.
 *
 *  @param n_iteration Iteration index to retrieve _test_iteration instance for.
 *
 * @return Iteration-specific _test_iteration instance.
 *
 **/
TessellationShaderInvarianceRule5Test::_test_iteration& TessellationShaderInvarianceRule5Test::getTestForIteration(
	unsigned int n_iteration)
{
	unsigned int	 n_triangles_tests = (unsigned int)m_test_triangles_iterations.size();
	_test_iteration& test_iteration	= (n_iteration < n_triangles_tests) ?
										  m_test_triangles_iterations[n_iteration] :
										  m_test_quads_iterations[n_iteration - n_triangles_tests];

	return test_iteration;
}

/** Retrieves iteration-specific tessellation properties.
 *
 *  @param n_iteration            Iteration index to retrieve the properties for.
 *  @param out_inner_tess_levels  Deref will be used to store iteration-specific inner
 *                                tessellation level values. Must not be NULL.
 *  @param out_outer_tess_levels  Deref will be used to store iteration-specific outer
 *                                tessellation level values. Must not be NULL.
 *  @param out_point_mode         Deref will be used to store iteration-specific flag
 *                                telling whether point mode should be enabled for given pass.
 *                                Must not be NULL.
 *  @param out_primitive_mode     Deref will be used to store iteration-specific primitive
 *                                mode. Must not be NULL.
 *  @param out_vertex_ordering    Deref will be used to store iteration-specific vertex
 *                                ordering. Must not be NULL.
 *  @param out_result_buffer_size Deref will be used to store amount of bytes XFB buffer object
 *                                storage should offer for the draw call to succeed. Can
 *                                be NULL.
 **/
void TessellationShaderInvarianceRule5Test::getIterationProperties(
	unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels, bool* out_point_mode,
	_tessellation_primitive_mode* out_primitive_mode, _tessellation_shader_vertex_ordering* out_vertex_ordering,
	unsigned int* out_result_buffer_size)
{
	DE_ASSERT(m_test_triangles_iterations.size() + m_test_quads_iterations.size() > n_iteration);

	_test_iteration& test_iteration = getTestForIteration(n_iteration);

	memcpy(out_inner_tess_levels, test_iteration.inner_tess_levels, sizeof(test_iteration.inner_tess_levels));
	memcpy(out_outer_tess_levels, test_iteration.outer_tess_levels, sizeof(test_iteration.outer_tess_levels));

	*out_point_mode		 = false;
	*out_primitive_mode  = test_iteration.primitive_mode;
	*out_vertex_ordering = test_iteration.vertex_ordering;

	if (out_result_buffer_size != DE_NULL)
	{
		*out_result_buffer_size = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
			*out_primitive_mode, out_inner_tess_levels, out_outer_tess_levels, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
			*out_point_mode);
		test_iteration.n_vertices = *out_result_buffer_size;
		*out_result_buffer_size =
			static_cast<unsigned int>(*out_result_buffer_size * 3 /* components */ * sizeof(float));

		DE_ASSERT(*out_result_buffer_size != 0);
	}
}

/** Retrieves iteration-specific tessellation evaluation shader code.
 *
 *  @param n_iteration Iteration index, for which the source code is being obtained.
 *
 *  @return Requested source code.
 **/
std::string TessellationShaderInvarianceRule5Test::getTECode(unsigned int n_iteration)
{
	DE_ASSERT(m_test_triangles_iterations.size() + m_test_quads_iterations.size() > n_iteration);

	const _test_iteration& test_iteration = getTestForIteration(n_iteration);

	return TessellationShaderUtils::getGenericTECode(TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
													 test_iteration.primitive_mode, test_iteration.vertex_ordering,
													 false); /* point mode */
}

/** Initializes test iterations for the test. The following modes and inner/outer tess level
 *  configurations are used to form the test set:
 *
 *  - Last inner/outer tessellation level combination as returned by
 *    TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode()
 *  - All primitive modes;
 *  - All vertex spacing modes;
 *
 *  All permutations are used to generate the test set.
 **/
void TessellationShaderInvarianceRule5Test::initTestIterations()
{
	DE_ASSERT(m_test_quads_iterations.size() == 0 && m_test_triangles_iterations.size() == 0);

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	const glw::Functions& gl						  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Iterate through all primitive and vertex spacing modes relevant to the test */
	_tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
													   TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	_tessellation_shader_vertex_ordering vo_modes[] = {
		TESSELLATION_SHADER_VERTEX_ORDERING_CCW, TESSELLATION_SHADER_VERTEX_ORDERING_CW,
	};

	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);
	const unsigned int n_vo_modes		 = sizeof(vo_modes) / sizeof(vo_modes[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];

		for (unsigned int n_vo_mode = 0; n_vo_mode < n_vo_modes; ++n_vo_mode)
		{
			_tessellation_shader_vertex_ordering vertex_ordering = vo_modes[n_vo_mode];

			/* Retrieve inner/outer tessellation level combinations we want the tests to be run for */
			_tessellation_levels_set levels_set;

			levels_set = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
				primitive_mode, gl_max_tess_gen_level_value,
				TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

			/* Only use the last inner/outer level configuration, as reported by the utils function. */
			const _tessellation_levels& levels = levels_set[levels_set.size() - 1];

			/* Create a test descriptor for all the parameters we now have */
			_test_iteration test;

			memcpy(test.inner_tess_levels, levels.inner, sizeof(test.inner_tess_levels));
			memcpy(test.outer_tess_levels, levels.outer, sizeof(test.outer_tess_levels));

			test.primitive_mode  = primitive_mode;
			test.vertex_ordering = vertex_ordering;

			if (primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES)
			{
				m_test_triangles_iterations.push_back(test);
			}
			else
			{
				m_test_quads_iterations.push_back(test);
			}
		} /* for (all vertex spacing modes) */
	}	 /* for (all primitive modes) */
}

/** Verifies user-provided vertex data can be found in the provided vertex data array.
 *
 *  @param vertex_data                     Vertex data array the requested vertex data are to be found in.
 *  @param n_vertices                      Amount of vertices declared in @param vertex_data;
 *  @param vertex_data_seeked              Vertex data to be found in @param vertex_data;
 *  @param n_vertex_data_seeked_components Amount of components to take into account.
 *
 *  @return true if the vertex data was found, false otherwise.
 **/
bool TessellationShaderInvarianceRule5Test::isVertexDefined(const float* vertex_data, unsigned int n_vertices,
															const float* vertex_data_seeked,
															unsigned int n_vertex_data_seeked_components)
{
	const float epsilon = 1e-5f;
	bool		result  = false;

	DE_ASSERT(n_vertex_data_seeked_components >= 2);

	for (unsigned int n_vertex = 0; n_vertex < n_vertices; ++n_vertex)
	{
		const float* current_vertex_data = vertex_data + 3 /* components */ * n_vertex;

		if (de::abs(vertex_data_seeked[0] - current_vertex_data[0]) < epsilon &&
			de::abs(vertex_data_seeked[1] - current_vertex_data[1]) < epsilon &&
			((n_vertex_data_seeked_components < 3) ||
			 (n_vertex_data_seeked_components >= 3 &&
			  de::abs(vertex_data_seeked[2] - current_vertex_data[2]) < epsilon)))
		{
			result = true;

			break;
		} /* if (components match) */
	}	 /* for (all vertices) */

	return result;
}

/** Verifies result data. Accesses data generated by all iterations.
 *
 *  Throws TestError exception if an error occurs.
 *
 *  @param all_iterations_data An array of pointers to buffers, holding gl_TessCoord
 *                             data generated by subsequent iterations.
 **/
void TessellationShaderInvarianceRule5Test::verifyResultData(const void** all_iterations_data)
{
	/* Run two separate iterations:
	 *
	 * a) triangles
	 * b) quads
	 */
	for (unsigned int n_iteration = 0; n_iteration < 2 /* quads, triangles */; ++n_iteration)
	{
		const unsigned int n_base_iteration =
			(n_iteration == 0) ? 0 : (const unsigned int)m_test_triangles_iterations.size();
		const unsigned int set_size = (n_iteration == 0) ? (const unsigned int)m_test_triangles_iterations.size() :
														   (const unsigned int)m_test_quads_iterations.size();
		const _test_iterations& test_iterations =
			(n_iteration == 0) ? m_test_triangles_iterations : m_test_quads_iterations;

		DE_ASSERT(test_iterations.size() != 0);

		/* For each iteration, verify that all vertices generated for all three vertex spacing modes.
		 * are exactly the same (but in different order) */
		const float* base_vertex_data = (const float*)all_iterations_data[n_base_iteration + 0];

		for (unsigned int n_set = 1; n_set < set_size; ++n_set)
		{
			const float* set_vertex_data = (const float*)all_iterations_data[n_base_iteration + n_set];

			/* Amount of vertices should not differ between sets */
			DE_ASSERT(test_iterations[0].n_vertices == test_iterations[n_set].n_vertices);

			/* Run through all vertices in base set and make sure they can be found in currently
			 * processed set */
			for (unsigned int n_base_vertex = 0; n_base_vertex < test_iterations[0].n_vertices; ++n_base_vertex)
			{
				const float* base_vertex = base_vertex_data + 3 /* components */ * n_base_vertex;

				if (!isVertexDefined(set_vertex_data, test_iterations[n_set].n_vertices, base_vertex,
									 3)) /* components */
				{
					const char* primitive_mode = (n_iteration == 0) ? "triangles" : "quads";

					m_testCtx.getLog() << tcu::TestLog::Message << "For primitive mode [" << primitive_mode << "] "
									   << "a vertex with tessellation coordinates:[" << base_vertex[0] << ", "
									   << base_vertex[1] << ", " << base_vertex[2] << ") "
									   << "could not have been found for both vertex orderings."
									   << tcu::TestLog::EndMessage;

					TCU_FAIL("Implementation does not follow Rule 5.");
				}
			} /* for (all base set's vertices) */
		}	 /* for (all sets) */
	}		  /* for (both primitive types) */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TessellationShaderInvarianceRule6Test::TessellationShaderInvarianceRule6Test(Context&			  context,
																			 const ExtParameters& extParams)
	: TessellationShaderInvarianceBaseTest(context, extParams, "invariance_rule6",
										   "Verifies conformance with sixth invariance rule")
{
}

/** Destructor. */
TessellationShaderInvarianceRule6Test::~TessellationShaderInvarianceRule6Test()
{
	/* Left blank intentionally */
}

/** Retrieves amount of iterations the base test implementation should run before
 *  calling global verification routine.
 *
 *  @return A value that depends on initTestIterations() behavior.
 **/
unsigned int TessellationShaderInvarianceRule6Test::getAmountOfIterations()
{
	if (m_test_quads_iterations.size() == 0 || m_test_triangles_iterations.size() == 0)
	{
		initTestIterations();
	}

	return (unsigned int)(m_test_quads_iterations.size() + m_test_triangles_iterations.size());
}

/** Retrieves _test_iteration instance specific for user-specified iteration index.
 *
 *  @param n_iteration Iteration index to retrieve _test_iteration instance for.
 *
 * @return Iteration-specific _test_iteration instance.
 *
 **/
TessellationShaderInvarianceRule6Test::_test_iteration& TessellationShaderInvarianceRule6Test::getTestForIteration(
	unsigned int n_iteration)
{
	unsigned int	 n_triangles_tests = (unsigned int)m_test_triangles_iterations.size();
	_test_iteration& test_iteration	= (n_iteration < n_triangles_tests) ?
										  m_test_triangles_iterations[n_iteration] :
										  m_test_quads_iterations[n_iteration - n_triangles_tests];

	return test_iteration;
}

/** Retrieves iteration-specific tessellation properties.
 *
 *  @param n_iteration            Iteration index to retrieve the properties for.
 *  @param out_inner_tess_levels  Deref will be used to store iteration-specific inner
 *                                tessellation level values. Must not be NULL.
 *  @param out_outer_tess_levels  Deref will be used to store iteration-specific outer
 *                                tessellation level values. Must not be NULL.
 *  @param out_point_mode         Deref will be used to store iteration-specific flag
 *                                telling whether point mode should be enabled for given pass.
 *                                Must not be NULL.
 *  @param out_primitive_mode     Deref will be used to store iteration-specific primitive
 *                                mode. Must not be NULL.
 *  @param out_vertex_ordering    Deref will be used to store iteration-specific vertex
 *                                ordering. Must not be NULL.
 *  @param out_result_buffer_size Deref will be used to store amount of bytes XFB buffer object
 *                                storage should offer for the draw call to succeed. Can
 *                                be NULL.
 **/
void TessellationShaderInvarianceRule6Test::getIterationProperties(
	unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels, bool* out_point_mode,
	_tessellation_primitive_mode* out_primitive_mode, _tessellation_shader_vertex_ordering* out_vertex_ordering,
	unsigned int* out_result_buffer_size)
{
	DE_ASSERT(m_test_triangles_iterations.size() + m_test_quads_iterations.size() > n_iteration);

	_test_iteration& test_iteration = getTestForIteration(n_iteration);

	memcpy(out_inner_tess_levels, test_iteration.inner_tess_levels, sizeof(test_iteration.inner_tess_levels));
	memcpy(out_outer_tess_levels, test_iteration.outer_tess_levels, sizeof(test_iteration.outer_tess_levels));

	*out_point_mode		 = false;
	*out_primitive_mode  = test_iteration.primitive_mode;
	*out_vertex_ordering = test_iteration.vertex_ordering;

	if (out_result_buffer_size != DE_NULL)
	{
		*out_result_buffer_size = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
			*out_primitive_mode, out_inner_tess_levels, out_outer_tess_levels, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
			*out_point_mode);
		test_iteration.n_vertices = *out_result_buffer_size;
		*out_result_buffer_size =
			static_cast<unsigned int>(*out_result_buffer_size * 3 /* components */ * sizeof(float));

		DE_ASSERT(*out_result_buffer_size != 0);
	}
}

/** Retrieves iteration-specific tessellation evaluation shader code.
 *
 *  @param n_iteration Iteration index, for which the source code is being obtained.
 *
 *  @return Requested source code.
 **/
std::string TessellationShaderInvarianceRule6Test::getTECode(unsigned int n_iteration)
{
	DE_ASSERT(m_test_triangles_iterations.size() + m_test_quads_iterations.size() > n_iteration);

	const _test_iteration& test_iteration = getTestForIteration(n_iteration);

	return TessellationShaderUtils::getGenericTECode(TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
													 test_iteration.primitive_mode, test_iteration.vertex_ordering,
													 false); /* point mode */
}

/** Initializes test iterations for the test. The following modes and inner/outer tess level
 *  configurations are used to form the test set:
 *
 *  - Tessellation level combinations as returned by
 *    TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode() (however, the inner
 *    tessellation level values are all set to values corresponding to last item returned for
 *    the set)
 *  - All primitive modes;
 *  - All vertex ordering modes;
 *
 *  All permutations are used to generate the test set.
 **/
void TessellationShaderInvarianceRule6Test::initTestIterations()
{
	DE_ASSERT(m_test_quads_iterations.size() == 0 && m_test_triangles_iterations.size() == 0);

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	const glw::Functions& gl						  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Iterate through all primitive and vertex spacing modes relevant to the test */
	_tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES,
													   TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS };
	_tessellation_shader_vertex_ordering vertex_ordering_modes[] = { TESSELLATION_SHADER_VERTEX_ORDERING_CCW,
																	 TESSELLATION_SHADER_VERTEX_ORDERING_CW };

	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);
	const unsigned int n_vo_modes		 = sizeof(vertex_ordering_modes) / sizeof(vertex_ordering_modes[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];

		for (unsigned int n_vo_mode = 0; n_vo_mode < n_vo_modes; ++n_vo_mode)
		{
			_tessellation_shader_vertex_ordering vertex_ordering = vertex_ordering_modes[n_vo_mode];

			/* Retrieve inner/outer tessellation level combinations we want the tests to be run for.
			 * Since each level set we will be provided by getTessellationLevelSetForPrimitiveMode()
			 * is unique and does not repeat, we'll just make sure the inner level values are set to
			 * the same set of values, so that the conditions the test must meet are actually met.
			 */
			float*					 inner_levels_to_use = DE_NULL;
			_tessellation_levels_set levels_set;
			unsigned int			 n_levels_sets = 0;

			levels_set = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
				primitive_mode, gl_max_tess_gen_level_value,
				TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

			n_levels_sets		= (unsigned int)levels_set.size();
			inner_levels_to_use = levels_set[n_levels_sets - 1].inner;

			for (unsigned int n_levels_set = 0; n_levels_set < n_levels_sets - 1; n_levels_set++)
			{
				/* Make sure the Utils function was not changed and that inner level values
				 * are actually unique across the whole set */
				DE_ASSERT(levels_set[n_levels_set].inner[0] != levels_set[n_levels_sets - 1].inner[0] &&
						  levels_set[n_levels_set].inner[1] != levels_set[n_levels_sets - 1].inner[1]);

				/* Force the last set's inner values to all level combinations we'll be using */
				memcpy(levels_set[n_levels_set].inner, inner_levels_to_use, sizeof(levels_set[n_levels_set].inner));
			} /* for (all sets retrieved from Utils function */

			for (_tessellation_levels_set_const_iterator set_iterator = levels_set.begin();
				 set_iterator != levels_set.end(); set_iterator++)
			{
				const _tessellation_levels& levels = *set_iterator;

				/* Create a test descriptor for all the parameters we now have */
				_test_iteration test;

				memcpy(test.inner_tess_levels, levels.inner, sizeof(test.inner_tess_levels));
				memcpy(test.outer_tess_levels, levels.outer, sizeof(test.outer_tess_levels));

				test.primitive_mode  = primitive_mode;
				test.vertex_ordering = vertex_ordering;

				if (primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES)
				{
					m_test_triangles_iterations.push_back(test);
				}
				else
				{
					m_test_quads_iterations.push_back(test);
				}
			} /* for (all level sets) */
		}	 /* for (all vertex ordering modes) */
	}		  /* for (all primitive modes) */
}

/** Verifies result data. Accesses data generated by all iterations.
 *
 *  Throws TestError exception if an error occurs.
 *
 *  @param all_iterations_data An array of pointers to buffers, holding gl_TessCoord
 *                             data generated by subsequent iterations.
 **/
void TessellationShaderInvarianceRule6Test::verifyResultData(const void** all_iterations_data)
{
	/* Run two separate iterations:
	 *
	 * a) triangles
	 * b) quads
	 */

	for (unsigned int n_iteration = 0; n_iteration < 2 /* quads, triangles */; ++n_iteration)
	{
		const unsigned int n_base_iteration =
			(n_iteration == 0) ? 0 : (const unsigned int)m_test_triangles_iterations.size();

		const unsigned int n_sets = (n_iteration == 0) ? (const unsigned int)m_test_triangles_iterations.size() :
														 (const unsigned int)m_test_quads_iterations.size();

		_tessellation_primitive_mode primitive_mode = (n_iteration == 0) ?
														  TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES :
														  TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS;

		const _test_iterations& test_iterations =
			(n_iteration == 0) ? m_test_triangles_iterations : m_test_quads_iterations;

		const unsigned int n_triangles_in_base_set = test_iterations[0].n_vertices / 3 /* vertices per triangle */;

		DE_ASSERT(test_iterations.size() != 0);

		/* For each iteration, verify that all vertices generated for all three vertex spacing modes.
		 * are exactly the same (but in different order) */
		const _test_iteration& base_test		= test_iterations[0];
		const float*		   base_vertex_data = (const float*)all_iterations_data[n_base_iteration + 0];

		for (unsigned int n_set = 1; n_set < n_sets; ++n_set)
		{
			const _test_iteration& set_test		   = test_iterations[n_set];
			const float*		   set_vertex_data = (const float*)all_iterations_data[n_base_iteration + n_set];

			/* We're operating on triangles so make sure the amount of vertices we're dealing with is
			 * divisible by 3 */
			DE_ASSERT((test_iterations[n_set].n_vertices % 3) == 0);

			const unsigned int n_triangles_in_curr_set = test_iterations[n_set].n_vertices / 3;

			/* Take base triangles and make sure they can be found in iteration-specific set.
			 * Now, thing to keep in mind here is that we must not assume any specific vertex
			 * and triangle order which is why the slow search. */
			for (unsigned int n_base_triangle = 0; n_base_triangle < n_triangles_in_base_set; ++n_base_triangle)
			{
				/* Extract base triangle data first */
				const float* base_triangle_vertex1 = base_vertex_data +
													 n_base_triangle * 3 *		/* vertices per triangle */
														 3;						/* components */
				const float* base_triangle_vertex2 = base_triangle_vertex1 + 3; /* components */
				const float* base_triangle_vertex3 = base_triangle_vertex2 + 3; /* components */

				/* Only interior triangles should be left intact. Is this an interior triangle? */
				if (!TessellationShaderUtils::isOuterEdgeVertex(primitive_mode, base_triangle_vertex1) &&
					!TessellationShaderUtils::isOuterEdgeVertex(primitive_mode, base_triangle_vertex2) &&
					!TessellationShaderUtils::isOuterEdgeVertex(primitive_mode, base_triangle_vertex3))
				{
					/* Iterate through all triangles in considered set */
					bool has_base_set_triangle_been_found = false;

					for (unsigned int n_curr_set_triangle = 0; n_curr_set_triangle < n_triangles_in_curr_set;
						 ++n_curr_set_triangle)
					{
						const float* curr_triangle = set_vertex_data +
													 n_curr_set_triangle * 3 * /* vertices per triangle */
														 3;					   /* components */

						if (TessellationShaderUtils::isTriangleDefined(base_triangle_vertex1, curr_triangle))
						{
							has_base_set_triangle_been_found = true;

							break;
						}
					} /* for (all triangles in currently processed set) */

					if (!has_base_set_triangle_been_found)
					{
						std::string primitive_mode_str =
							TessellationShaderUtils::getESTokenForPrimitiveMode(base_test.primitive_mode);

						m_testCtx.getLog()
							<< tcu::TestLog::Message << "For primitive mode [" << primitive_mode_str << "]"
							<< ", base inner tessellation levels:"
							<< "[" << base_test.inner_tess_levels[0] << ", " << base_test.inner_tess_levels[1] << "]"
							<< ", base outer tessellation levels:"
							<< "[" << base_test.outer_tess_levels[0] << ", " << base_test.outer_tess_levels[1] << ", "
							<< base_test.outer_tess_levels[2] << ", " << base_test.outer_tess_levels[3] << "]"
							<< ", reference inner tessellation levels:"
							<< "[" << set_test.inner_tess_levels[0] << ", " << set_test.inner_tess_levels[1] << "]"
							<< ", reference outer tessellation levels:"
							<< "[" << set_test.outer_tess_levels[0] << ", " << set_test.outer_tess_levels[1] << ", "
							<< set_test.outer_tess_levels[2] << ", " << set_test.outer_tess_levels[3] << "]"
							<< ", the following triangle formed during base tessellation run was not found in "
							   "reference run:"
							<< "[" << base_triangle_vertex1[0] << ", " << base_triangle_vertex1[1] << ", "
							<< base_triangle_vertex1[2] << "]x"
							<< "[" << base_triangle_vertex2[0] << ", " << base_triangle_vertex2[1] << ", "
							<< base_triangle_vertex2[2] << "]x"
							<< "[" << base_triangle_vertex3[0] << ", " << base_triangle_vertex3[1] << ", "
							<< base_triangle_vertex3[2]

							<< tcu::TestLog::EndMessage;

						TCU_FAIL("Implementation does not appear to be rule 6-conformant");
					} /* if (triangle created during base run was not found in reference run) */
				}	 /* if (base triangle is interior) */
			}		  /* for (all base set's vertices) */
		}			  /* for (all sets) */
	}				  /* for (both primitive types) */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
TessellationShaderInvarianceRule7Test::TessellationShaderInvarianceRule7Test(Context&			  context,
																			 const ExtParameters& extParams)
	: TessellationShaderInvarianceBaseTest(context, extParams, "invariance_rule7",
										   "Verifies conformance with seventh invariance rule")
{
}

/** Destructor. */
TessellationShaderInvarianceRule7Test::~TessellationShaderInvarianceRule7Test()
{
	/* Left blank intentionally */
}

/** Retrieves amount of iterations the base test implementation should run before
 *  calling global verification routine.
 *
 *  @return A value that depends on initTestIterations() behavior.
 **/
unsigned int TessellationShaderInvarianceRule7Test::getAmountOfIterations()
{
	if (m_test_quads_iterations.size() == 0 || m_test_triangles_iterations.size() == 0)
	{
		initTestIterations();
	}

	return (unsigned int)(m_test_quads_iterations.size() + m_test_triangles_iterations.size());
}

/** Retrieves index of a test iteration that was initialized with user-provided
 *  properties.
 *
 *  @param is_triangles_iteration      true if the seeked test iteration should have
 *                                     been run for 'triangles' primitive mode', false
 *                                     if 'quads' primitive mode run is seeked.
 *  @param inner_tess_levels           Two FP values describing inner tessellation level
 *                                     values the seeked run should have used.
 *  @param outer_tess_levels           Four FP values describing outer tessellation level
 *                                     values the seeked run should have used.
 *  @param vertex_ordering             Vertex ordering mode the seeked run should have used.
 *  @param n_modified_outer_tess_level Tells which outer tessellation level should be
 *                                     excluded from checking.
 *
 *  @return 0xFFFFFFFF if no test iteration was run for user-provided properties,
 *          actual index otherwise.
 *
 **/
unsigned int TessellationShaderInvarianceRule7Test::getTestIterationIndex(
	bool is_triangles_iteration, const float* inner_tess_levels, const float* outer_tess_levels,
	_tessellation_shader_vertex_ordering vertex_ordering, unsigned int n_modified_outer_tess_level)
{
	const float				epsilon = 1e-5f;
	unsigned int			result  = 0xFFFFFFFF;
	const _test_iterations& test_iterations =
		(is_triangles_iteration) ? m_test_triangles_iterations : m_test_quads_iterations;
	const unsigned int n_test_iterations = (const unsigned int)test_iterations.size();

	for (unsigned int n_test_iteration = 0; n_test_iteration < n_test_iterations; ++n_test_iteration)
	{
		_test_iteration test_iteration = test_iterations[n_test_iteration];

		if (de::abs(test_iteration.inner_tess_levels[0] - inner_tess_levels[0]) < epsilon &&
			de::abs(test_iteration.inner_tess_levels[1] - inner_tess_levels[1]) < epsilon &&
			test_iteration.vertex_ordering == vertex_ordering &&
			test_iteration.n_modified_outer_tess_level == n_modified_outer_tess_level)
		{
			/* Only compare outer tessellation levels that have not been modified */
			if (((n_modified_outer_tess_level == 0) ||
				 (n_modified_outer_tess_level != 0 &&
				  de::abs(test_iteration.outer_tess_levels[0] - outer_tess_levels[0]) < epsilon)) &&
				((n_modified_outer_tess_level == 1) ||
				 (n_modified_outer_tess_level != 1 &&
				  de::abs(test_iteration.outer_tess_levels[1] - outer_tess_levels[1]) < epsilon)) &&
				((n_modified_outer_tess_level == 2) ||
				 (n_modified_outer_tess_level != 2 &&
				  de::abs(test_iteration.outer_tess_levels[2] - outer_tess_levels[2]) < epsilon)) &&
				((n_modified_outer_tess_level == 3) ||
				 (n_modified_outer_tess_level != 3 &&
				  de::abs(test_iteration.outer_tess_levels[3] - outer_tess_levels[3]) < epsilon)))
			{
				result = n_test_iteration;

				break;
			}
		}
	} /* for (all test iterations) */

	return result;
}

/** Retrieves _test_iteration instance specific for user-specified iteration index.
 *
 *  @param n_iteration Iteration index to retrieve _test_iteration instance for.
 *
 * @return Iteration-specific _test_iteration instance.
 *
 **/
TessellationShaderInvarianceRule7Test::_test_iteration& TessellationShaderInvarianceRule7Test::getTestForIteration(
	unsigned int n_iteration)
{
	unsigned int	 n_triangles_tests = (unsigned int)m_test_triangles_iterations.size();
	_test_iteration& test_iteration	= (n_iteration < n_triangles_tests) ?
										  m_test_triangles_iterations[n_iteration] :
										  m_test_quads_iterations[n_iteration - n_triangles_tests];

	return test_iteration;
}

/** Retrieves iteration-specific tessellation properties.
 *
 *  @param n_iteration            Iteration index to retrieve the properties for.
 *  @param out_inner_tess_levels  Deref will be used to store iteration-specific inner
 *                                tessellation level values. Must not be NULL.
 *  @param out_outer_tess_levels  Deref will be used to store iteration-specific outer
 *                                tessellation level values. Must not be NULL.
 *  @param out_point_mode         Deref will be used to store iteration-specific flag
 *                                telling whether point mode should be enabled for given pass.
 *                                Must not be NULL.
 *  @param out_primitive_mode     Deref will be used to store iteration-specific primitive
 *                                mode. Must not be NULL.
 *  @param out_vertex_ordering    Deref will be used to store iteration-specific vertex
 *                                ordering. Must not be NULL.
 *  @param out_result_buffer_size Deref will be used to store amount of bytes XFB buffer object
 *                                storage should offer for the draw call to succeed. Can
 *                                be NULL.
 **/
void TessellationShaderInvarianceRule7Test::getIterationProperties(
	unsigned int n_iteration, float* out_inner_tess_levels, float* out_outer_tess_levels, bool* out_point_mode,
	_tessellation_primitive_mode* out_primitive_mode, _tessellation_shader_vertex_ordering* out_vertex_ordering,
	unsigned int* out_result_buffer_size)
{
	DE_ASSERT(m_test_triangles_iterations.size() + m_test_quads_iterations.size() > n_iteration);

	_test_iteration& test_iteration = getTestForIteration(n_iteration);

	memcpy(out_inner_tess_levels, test_iteration.inner_tess_levels, sizeof(test_iteration.inner_tess_levels));
	memcpy(out_outer_tess_levels, test_iteration.outer_tess_levels, sizeof(test_iteration.outer_tess_levels));

	*out_point_mode		 = false;
	*out_primitive_mode  = test_iteration.primitive_mode;
	*out_vertex_ordering = test_iteration.vertex_ordering;

	if (out_result_buffer_size != DE_NULL)
	{
		*out_result_buffer_size = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
			*out_primitive_mode, out_inner_tess_levels, out_outer_tess_levels, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
			*out_point_mode);
		test_iteration.n_vertices = *out_result_buffer_size;
		*out_result_buffer_size =
			static_cast<unsigned int>(*out_result_buffer_size * 3 /* components */ * sizeof(float));

		DE_ASSERT(*out_result_buffer_size != 0);
	}
}

/** Retrieves iteration-specific tessellation evaluation shader code.
 *
 *  @param n_iteration Iteration index, for which the source code is being obtained.
 *
 *  @return Requested source code.
 **/
std::string TessellationShaderInvarianceRule7Test::getTECode(unsigned int n_iteration)
{
	DE_ASSERT(m_test_triangles_iterations.size() + m_test_quads_iterations.size() > n_iteration);

	const _test_iteration& test_iteration = getTestForIteration(n_iteration);

	return TessellationShaderUtils::getGenericTECode(TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
													 test_iteration.primitive_mode, test_iteration.vertex_ordering,
													 false); /* point mode */
}

/** Initializes test iterations for the test. The following modes and inner/outer tess level
 *  configurations are used to form the test set:
 *
 *  - All inner/outer tessellation level combinations as returned by
 *    TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode()
 *    times 3 (for triangles) or 4 (for quads). For each combination,
 *    the test will capture tessellation coordinates multiple times, each
 *    time changing a different outer tessellation level value and leaving
 *    the rest intact.
 *  - All primitive modes;
 *  - All vertex spacing modes;
 *
 *  All permutations are used to generate the test set.
 **/
void TessellationShaderInvarianceRule7Test::initTestIterations()
{
	DE_ASSERT(m_test_quads_iterations.size() == 0 && m_test_triangles_iterations.size() == 0);

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	const glw::Functions& gl						  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Iterate through all primitive and vertex spacing modes relevant to the test */
	_tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
													   TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	_tessellation_shader_vertex_ordering vo_modes[] = {
		TESSELLATION_SHADER_VERTEX_ORDERING_CCW, TESSELLATION_SHADER_VERTEX_ORDERING_CW,
	};

	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);
	const unsigned int n_vo_modes		 = sizeof(vo_modes) / sizeof(vo_modes[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];
		const unsigned int			 n_relevant_outer_tess_levels =
			(primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS) ? 4 : 3;

		for (unsigned int n_vo_mode = 0; n_vo_mode < n_vo_modes; ++n_vo_mode)
		{
			_tessellation_shader_vertex_ordering vertex_ordering = vo_modes[n_vo_mode];

			/* Retrieve inner/outer tessellation level combinations we want the tests to be run for */
			_tessellation_levels_set levels_set;

			levels_set = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
				primitive_mode, gl_max_tess_gen_level_value,
				TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

			/* Create test descriptor for all inner/outer level configurations we received from the utils function. */
			for (_tessellation_levels_set_const_iterator levels_set_iterator = levels_set.begin();
				 levels_set_iterator != levels_set.end(); levels_set_iterator++)
			{
				const _tessellation_levels& levels = *levels_set_iterator;

				for (unsigned int n_outer_level_to_change = 0;
					 n_outer_level_to_change < n_relevant_outer_tess_levels + 1 /* base iteration */;
					 ++n_outer_level_to_change)
				{
					/* Create a test descriptor for all the parameters we now have */
					_test_iteration test;

					memcpy(test.inner_tess_levels, levels.inner, sizeof(test.inner_tess_levels));
					memcpy(test.outer_tess_levels, levels.outer, sizeof(test.outer_tess_levels));

					test.primitive_mode  = primitive_mode;
					test.vertex_ordering = vertex_ordering;

					/* Change iteration-specific outer tessellation level to a different value, but only
					 * if we're not preparing a base iteration*/
					if (n_outer_level_to_change != n_relevant_outer_tess_levels)
					{
						test.n_modified_outer_tess_level				= n_outer_level_to_change;
						test.outer_tess_levels[n_outer_level_to_change] = (float)(gl_max_tess_gen_level_value) / 3.0f;
					}
					else
					{
						test.is_base_iteration = true;
					}

					if (primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES)
					{
						m_test_triangles_iterations.push_back(test);
					}
					else
					{
						m_test_quads_iterations.push_back(test);
					}
				}
			} /* for (all levels set entries) */
		}	 /* for (all vertex spacing modes) */
	}		  /* for (all primitive modes) */
}

/** Tells whether a triangle is included in user-provided set of triangles.
 *  The triangle is expected to use an undefined vertex ordering.
 *
 *  @param base_triangle_data     9 FP values defining 3 vertices of a triangle.
 *  @param vertex_data            Vertex stream. It is expected these vertices
 *                                form triangles. It is also assumed each vertex
 *                                is expressed with 3 components.
 *  @param vertex_data_n_vertices Amount of vertices that can be found in @param
 *                                vertex_data
 *
 *  @return true if the triangle was found in user-provided triangle set,
 *          false otherwise.
 *
 **/
bool TessellationShaderInvarianceRule7Test::isTriangleDefinedInVertexDataSet(const float* base_triangle_data,
																			 const float* vertex_data,
																			 unsigned int vertex_data_n_vertices)
{
	bool result = false;

	for (unsigned int n_triangle = 0; n_triangle < vertex_data_n_vertices / 3 /* vertices per triangle */; n_triangle++)
	{
		const float* current_triangle_data = vertex_data +
											 n_triangle * 3 * /* vertices per triangle */
												 3;			  /* components */

		if (TessellationShaderUtils::isTriangleDefined(current_triangle_data, base_triangle_data))
		{
			result = true;

			break;
		}
	} /* for (all vertices) */

	return result;
}

/** Verifies result data. Accesses data generated by all iterations.
 *
 *  Throws TestError exception if an error occurs.
 *
 *  @param all_iterations_data An array of pointers to buffers, holding gl_TessCoord
 *                             data generated by subsequent iterations.
 **/
void TessellationShaderInvarianceRule7Test::verifyResultData(const void** all_iterations_data)
{
	const float epsilon = 1e-5f;

	/* Run two separate iterations:
	 *
	 * a) triangles
	 * b) quads
	 */
	for (unsigned int n_iteration = 0; n_iteration < 2 /* triangles, quads */; ++n_iteration)
	{
		bool			   is_triangles_iteration = (n_iteration == 0);
		const unsigned int n_base_iteration =
			(n_iteration == 0) ? 0 : (const unsigned int)m_test_triangles_iterations.size();
		const unsigned int n_relevant_outer_tess_levels = (is_triangles_iteration) ? 3 : 4;

		_tessellation_primitive_mode primitive_mode = (n_iteration == 0) ?
														  TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES :
														  TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS;

		const _test_iterations& test_iterations =
			(n_iteration == 0) ? m_test_triangles_iterations : m_test_quads_iterations;

		DE_ASSERT(test_iterations.size() != 0);

		/* Find a base iteration first */
		for (unsigned int n_base_test_iteration = 0; n_base_test_iteration < test_iterations.size();
			 n_base_test_iteration++)
		{
			const _test_iteration& base_iteration = test_iterations[n_base_test_iteration];
			std::vector<int>	   ref_iteration_indices;

			if (!base_iteration.is_base_iteration)
			{
				continue;
			}

			/* Retrieve reference test iterations */
			for (unsigned int n_reference_iteration = 0; n_reference_iteration < n_relevant_outer_tess_levels;
				 ++n_reference_iteration)
			{
				const unsigned int n_modified_outer_tess_level =
					(base_iteration.n_modified_outer_tess_level + n_reference_iteration + 1) %
					n_relevant_outer_tess_levels;
				const unsigned int ref_iteration_index = getTestIterationIndex(
					is_triangles_iteration, base_iteration.inner_tess_levels, base_iteration.outer_tess_levels,
					base_iteration.vertex_ordering, n_modified_outer_tess_level);

				DE_ASSERT(ref_iteration_index != 0xFFFFFFFF);
				DE_ASSERT(ref_iteration_index != n_base_test_iteration);

				ref_iteration_indices.push_back(ref_iteration_index);
			}

			/* We can now start comparing base data with the information generated for
			 * reference iterations. */
			for (std::vector<int>::const_iterator ref_iteration_iterator = ref_iteration_indices.begin();
				 ref_iteration_iterator != ref_iteration_indices.end(); ref_iteration_iterator++)
			{
				const int&			   n_ref_test_iteration = *ref_iteration_iterator;
				const _test_iteration& ref_iteration		= test_iterations[n_ref_test_iteration];

				/* Now move through all triangles generated for base test iteration. Focus on the ones
				 * that connect the outer edge with one of the inner ones */
				const float* base_iteration_vertex_data =
					(const float*)all_iterations_data[n_base_iteration + n_base_test_iteration];
				const float* ref_iteration_vertex_data =
					(const float*)all_iterations_data[n_base_iteration + n_ref_test_iteration];

				for (unsigned int n_base_triangle = 0;
					 n_base_triangle < base_iteration.n_vertices / 3; /* vertices per triangle */
					 ++n_base_triangle)
				{
					const float* base_triangle_data =
						base_iteration_vertex_data + n_base_triangle * 3 /* vertices */ * 3; /* components */

					/* Is that the triangle type we're after? */
					const float* base_triangle_vertex1 = base_triangle_data;
					const float* base_triangle_vertex2 = base_triangle_vertex1 + 3; /* components */
					const float* base_triangle_vertex3 = base_triangle_vertex2 + 3; /* components */
					bool		 is_base_triangle_vertex1_outer =
						TessellationShaderUtils::isOuterEdgeVertex(primitive_mode, base_triangle_vertex1);
					bool is_base_triangle_vertex2_outer =
						TessellationShaderUtils::isOuterEdgeVertex(primitive_mode, base_triangle_vertex2);
					bool is_base_triangle_vertex3_outer =
						TessellationShaderUtils::isOuterEdgeVertex(primitive_mode, base_triangle_vertex3);
					unsigned int n_outer_edge_vertices_found = 0;

					n_outer_edge_vertices_found += (is_base_triangle_vertex1_outer == true);
					n_outer_edge_vertices_found += (is_base_triangle_vertex2_outer == true);
					n_outer_edge_vertices_found += (is_base_triangle_vertex3_outer == true);

					if (n_outer_edge_vertices_found == 0)
					{
						/* This is an inner triangle, not really of our interest */
						continue;
					}

					/* Which outer tessellation level describes the base data edge? */
					unsigned int n_base_tess_level = 0;

					if (primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES)
					{
						if ((!is_base_triangle_vertex1_outer ||
							 (is_base_triangle_vertex1_outer && base_triangle_vertex1[0] == 0.0f)) &&
							(!is_base_triangle_vertex2_outer ||
							 (is_base_triangle_vertex2_outer && base_triangle_vertex2[0] == 0.0f)) &&
							(!is_base_triangle_vertex3_outer ||
							 (is_base_triangle_vertex3_outer && base_triangle_vertex3[0] == 0.0f)))
						{
							n_base_tess_level = 0;
						}
						else if ((!is_base_triangle_vertex1_outer ||
								  (is_base_triangle_vertex1_outer && base_triangle_vertex1[1] == 0.0f)) &&
								 (!is_base_triangle_vertex2_outer ||
								  (is_base_triangle_vertex2_outer && base_triangle_vertex2[1] == 0.0f)) &&
								 (!is_base_triangle_vertex3_outer ||
								  (is_base_triangle_vertex3_outer && base_triangle_vertex3[1] == 0.0f)))
						{
							n_base_tess_level = 1;
						}
						else
						{
							DE_ASSERT((!is_base_triangle_vertex1_outer ||
									   (is_base_triangle_vertex1_outer && base_triangle_vertex1[2] == 0.0f)) &&
									  (!is_base_triangle_vertex2_outer ||
									   (is_base_triangle_vertex2_outer && base_triangle_vertex2[2] == 0.0f)) &&
									  (!is_base_triangle_vertex3_outer ||
									   (is_base_triangle_vertex3_outer && base_triangle_vertex3[2] == 0.0f)));

							n_base_tess_level = 2;
						}
					} /* if (primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES) */
					else
					{
						DE_ASSERT(primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS);

						std::size_t				  n_outer_edge_vertices = 0;
						std::vector<const float*> outer_edge_vertices;

						if (is_base_triangle_vertex1_outer)
						{
							outer_edge_vertices.push_back(base_triangle_vertex1);
						}

						if (is_base_triangle_vertex2_outer)
						{
							outer_edge_vertices.push_back(base_triangle_vertex2);
						}

						if (is_base_triangle_vertex3_outer)
						{
							outer_edge_vertices.push_back(base_triangle_vertex3);
						}

						n_outer_edge_vertices = outer_edge_vertices.size();

						DE_ASSERT(n_outer_edge_vertices >= 1 && n_outer_edge_vertices <= 2);

						bool is_top_outer_edge	= true;
						bool is_right_outer_edge  = true;
						bool is_bottom_outer_edge = true;
						bool is_left_outer_edge   = true;

						/* Find which outer edges the vertices don't belong to. If one is in a corner,
						 * the other will clarify to which edge both vertices belong. */
						for (unsigned int n_vertex = 0; n_vertex < n_outer_edge_vertices; ++n_vertex)
						{
							/* Y < 1, not top outer edge */
							if (de::abs(outer_edge_vertices[n_vertex][1] - 1.0f) > epsilon)
							{
								is_top_outer_edge = false;
							}

							/* X < 1, not right outer edge */
							if (de::abs(outer_edge_vertices[n_vertex][0] - 1.0f) > epsilon)
							{
								is_right_outer_edge = false;
							}

							/* Y > 0, not bottom outer edge */
							if (de::abs(outer_edge_vertices[n_vertex][1]) > epsilon)
							{
								is_bottom_outer_edge = false;
							}

							/* X > 0, not left outer edge */
							if (de::abs(outer_edge_vertices[n_vertex][0]) > epsilon)
							{
								is_left_outer_edge = false;
							}
						}

						if (n_outer_edge_vertices == 1)
						{
							/* A single vertex with corner-coordinates belongs to two edges. Choose one */
							bool x_is_0 = de::abs(outer_edge_vertices[0][0]) < epsilon;
							bool x_is_1 = de::abs(outer_edge_vertices[0][0] - 1.0f) < epsilon;
							bool y_is_0 = de::abs(outer_edge_vertices[0][1]) < epsilon;
							bool y_is_1 = de::abs(outer_edge_vertices[0][1] - 1.0f) < epsilon;

							if (x_is_0 && y_is_0)
							{
								/* bottom edge */
								DE_ASSERT(is_left_outer_edge && is_bottom_outer_edge);
								is_left_outer_edge = false;
							}
							else if (x_is_0 && y_is_1)
							{
								/* left edge */
								DE_ASSERT(is_left_outer_edge && is_top_outer_edge);
								is_top_outer_edge = false;
							}
							else if (x_is_1 && y_is_0)
							{
								/* right edge */
								DE_ASSERT(is_right_outer_edge && is_bottom_outer_edge);
								is_bottom_outer_edge = false;
							}
							else if (x_is_1 && y_is_1)
							{
								/* top edge */
								DE_ASSERT(is_right_outer_edge && is_top_outer_edge);
								is_right_outer_edge = false;
							}
							else
							{
								/* Not a corner vertex, only one of the edge-flags is set */
							}
						}

						/* Sanity checks */
						DE_UNREF(is_top_outer_edge);
						DE_ASSERT((is_left_outer_edge && !is_top_outer_edge && !is_bottom_outer_edge &&
								   !is_right_outer_edge) ||
								  (!is_left_outer_edge && is_top_outer_edge && !is_bottom_outer_edge &&
								   !is_right_outer_edge) ||
								  (!is_left_outer_edge && !is_top_outer_edge && is_bottom_outer_edge &&
								   !is_right_outer_edge) ||
								  (!is_left_outer_edge && !is_top_outer_edge && !is_bottom_outer_edge &&
								   is_right_outer_edge));

						/* We have all the data needed to determine which tessellation level describes
						 * subdivision of the edge that the triangle touches */
						if (is_left_outer_edge)
						{
							n_base_tess_level = 0;
						}
						else if (is_bottom_outer_edge)
						{
							n_base_tess_level = 1;
						}
						else if (is_right_outer_edge)
						{
							n_base_tess_level = 2;
						}
						else
						{
							n_base_tess_level = 3;
						}
					}

					/* We shouldn't perform the check if the edge we're processing was described
					 * by a different outer tessellation level in the reference data set */
					if (n_base_tess_level == ref_iteration.n_modified_outer_tess_level)
					{
						continue;
					}

					/* This triangle should be present in both vertex data sets */
					if (!isTriangleDefinedInVertexDataSet(base_triangle_data, ref_iteration_vertex_data,
														  ref_iteration.n_vertices))
					{
						const char* primitive_mode_str = (is_triangles_iteration) ? "triangles" : "quads";

						m_testCtx.getLog()
							<< tcu::TestLog::Message << "For primitive mode [" << primitive_mode_str << "] "
							<< ", inner tessellation levels:"
							<< "[" << base_iteration.inner_tess_levels[0] << ", " << base_iteration.inner_tess_levels[1]
							<< "], outer tessellation levels:"
							<< "[" << base_iteration.outer_tess_levels[0] << ", " << base_iteration.outer_tess_levels[1]
							<< ", " << base_iteration.outer_tess_levels[2] << ", "
							<< base_iteration.outer_tess_levels[3] << "], a triangle connecting inner & outer edges:"
							<< "[" << base_triangle_vertex1[0] << ", " << base_triangle_vertex1[1] << ", "
							<< base_triangle_vertex1[2] << "]x"
							<< "[" << base_triangle_vertex2[0] << ", " << base_triangle_vertex2[1] << ", "
							<< base_triangle_vertex2[2] << "]x"
							<< "[" << base_triangle_vertex3[0] << ", " << base_triangle_vertex3[1] << ", "
							<< base_triangle_vertex3[2] << "] was not found for runs using CW and CCW vertex ordering, "
														   "which is against the extension specification's rule 7."
							<< tcu::TestLog::EndMessage;

						TCU_FAIL("Implementation is not conformant with Tessellation Rule 7");
					}
				} /* for (all triangles generated for base test iteration) */
			}	 /* for (all reference iterations) */
		}		  /* for (all base test iterations) */
	}			  /* for (both primitive types) */
}

} /* namespace glcts */
