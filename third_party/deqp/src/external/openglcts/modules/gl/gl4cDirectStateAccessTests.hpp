#ifndef _GL4CDIRECTSTATEACCESSTESTS_HPP
#define _GL4CDIRECTSTATEACCESSTESTS_HPP
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
 * \file  gl4cDirectStateAccessTests.hpp
 * \brief Conformance tests for the Direct State Access feature functionality.
 */ /*-----------------------------------------------------------------------------*/

/* Includes. */

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

#include <string>
#include <typeinfo>

namespace gl4cts
{
namespace DirectStateAccess
{
/** @class Tests
 *
 *  @brief Direct State Access test group.
 */
class Tests : public deqp::TestCaseGroup
{
public:
	/* Public member functions */
	Tests(deqp::Context& context);

	void init();

private:
	/* Private member functions */
	Tests(const Tests& other);
	Tests& operator=(const Tests& other);
};
/* Tests class */

/* Direct State Access Feature Interfaces */

/* Direct State Access Transform Feedback Tests */
namespace TransformFeedback
{
/** @class CreationTest
 *
 *  @brief Direct State Access Transform Feedback Creation test cases.
 *
 *  Test follows the steps:
 *
 *      Create at least two transform feedback objects names with
 *      GenTransformFeedbacks function. Check them without binding, using
 *      IsTransformFeedback function. Expect GL_FALSE.
 *
 *      Create at least two transform feedback objects with
 *      CreateTransformFeedbacks function. Check them without binding, using
 *      IsTransformFeedback function. Expect GL_TRUE.
 *
 *      Check that transform feedback binding point is unchanged.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

/** @class DefaultsTest
 *
 *  @brief Direct State Access Transform Feedback Default State test cases.
 *
 *  Test follows the steps:
 *
 *       Create transform feedback object with CreateTransformFeedbacks function.
 *
 *      Query parameters TRANSFORM_FEEDBACK_BUFFER_BINDING using
 *      GetTransformFeedbacki_v for all available indexed binding points. For
 *      all queries, expect value equal to 0.
 *
 *      Query parameters:
 *       -  TRANSFORM_FEEDBACK_BUFFER_START and
 *       -  TRANSFORM_FEEDBACK_BUFFER_SIZE
 *      using GetTransformFeedbacki64_v for all available indexed binding
 *      points. For all queries, expect value equal to 0.
 *
 *      Query parameters:
 *       -  TRANSFORM_FEEDBACK_PAUSED and
 *       -  TRANSFORM_FEEDBACK_ACTIVE
 *      using GetTransformFeedbackiv. For all queries, expect value equal to
 *      FALSE.
 */
class DefaultsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DefaultsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DefaultsTest(const DefaultsTest& other);
	DefaultsTest& operator=(const DefaultsTest& other);

	/* Function pointers type definitions and declarations. */
	typedef void(GLW_APIENTRY* GetTransformFeedbackiv_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																   glw::GLint* param);
	typedef void(GLW_APIENTRY* GetTransformFeedbacki_v_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																	glw::GLuint index, glw::GLint* param);
	typedef void(GLW_APIENTRY* GetTransformFeedbacki64_v_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																	  glw::GLuint index, glw::GLint64* param);

	GetTransformFeedbackiv_ProcAddress	m_gl_getTransformFeedbackiv;
	GetTransformFeedbacki_v_ProcAddress   m_gl_getTransformFeedbacki_v;
	GetTransformFeedbacki64_v_ProcAddress m_gl_getTransformFeedbacki64_v;

	/* Private member variables */
	glw::GLuint m_xfb_dsa;
	glw::GLint  m_xfb_indexed_binding_points_count;

	/* Private member functions. */
	void prepare();
	bool testBuffersBindingPoints();
	bool testBuffersDimensions();
	bool testActive();
	bool testPaused();
	void clean();
};
/* DefaultsTest class */

/** @class BuffersTest
 *
 *  @brief Direct State Access Transform Feedback Buffer Objects binding test cases.
 *         The test follows the steps:
 *
 *             Create transform feedback object with CreateTransformFeedbacks function.
 *
 *             Create two buffer objects using GenBuffers and BindBuffer functions.
 *             Allocate storage for them using BufferData.
 *
 *             Bind the first buffer to transform feedback object indexed binding point
 *             0 using TransformFeedbackBufferBase function.
 *
 *             Bind a first half of the second buffer to transform feedback object
 *             indexed binding point 1 using TransformFeedbackBufferRange.
 *
 *             Bind a second half of the second buffer to transform feedback object
 *             indexed binding point 12 using TransformFeedbackBufferRange.
 *
 *             Query parameter TRANSFORM_FEEDBACK_BUFFER_BINDING using
 *             GetTransformFeedbacki_v for all 1st, 2nd and 3rd indexed binding point.
 *             For all queries, expect value equal to the corresponding buffers'
 *             identifiers.
 *
 *             Query parameters:
 *              -  TRANSFORM_FEEDBACK_BUFFER_START and
 *              -  TRANSFORM_FEEDBACK_BUFFER_SIZE
 *             using GetTransformFeedbacki64_v for indexed binding points 0, 1 and 2.
 *             Verify returned values.
 */
class BuffersTest : public deqp::TestCase
{
public:
	/* Public member functions */
	BuffersTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	BuffersTest(const BuffersTest& other);
	BuffersTest& operator=(const BuffersTest& other);

	/* Function pointers type definitions and declarations. */
	typedef void(GLW_APIENTRY* GetTransformFeedbacki_v_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																	glw::GLuint index, glw::GLint* param);
	typedef void(GLW_APIENTRY* GetTransformFeedbacki64_v_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																	  glw::GLuint index, glw::GLint64* param);
	typedef void(GLW_APIENTRY* TransformFeedbackBufferBase_ProcAddress)(glw::GLuint xfb, glw::GLuint index,
																		glw::GLuint buffer);
	typedef void(GLW_APIENTRY* TransformFeedbackBufferRange_ProcAddress)(glw::GLuint xfb, glw::GLuint index,
																		 glw::GLuint buffer, glw::GLintptr offset,
																		 glw::GLsizei size);

	GetTransformFeedbacki_v_ProcAddress		 m_gl_getTransformFeedbacki_v;
	GetTransformFeedbacki64_v_ProcAddress	m_gl_getTransformFeedbacki64_v;
	TransformFeedbackBufferBase_ProcAddress  m_gl_TransformFeedbackBufferBase;
	TransformFeedbackBufferRange_ProcAddress m_gl_TransformFeedbackBufferRange;

	/* Private member variables */
	glw::GLuint m_xfb_dsa;
	glw::GLuint m_bo_a;
	glw::GLuint m_bo_b;

	/* Private static variables */
	static const glw::GLuint s_bo_size;

	/* Private member functions. */
	void prepareObjects();
	bool prepareTestSetup();
	bool testBindingPoint(glw::GLuint const index, glw::GLint const expected_value,
						  glw::GLchar const* const tested_function_name);
	bool testStart(glw::GLuint const index, glw::GLint const expected_value,
				   glw::GLchar const* const tested_function_name);
	bool testSize(glw::GLuint const index, glw::GLint const expected_value,
				  glw::GLchar const* const tested_function_name);
	void clean();
};
/* BuffersTest class */

/** @class ErrorsTest
 *
 *  @brief Direct State Access Transform Feedback Negative test cases.
 *         The test follows steps:
 *
 *              Check that CreateTransformFeedbacks generates INVALID_VALUE error if
 *              number of transform feedback objects to create is negative.
 *
 *              Check that GetTransformFeedbackiv, GetTransformFeedbacki_v and
 *              GetTransformFeedbacki64_v generate INVALID_OPERATION error if xfb is not
 *              zero or the name of an existing transform feedback object.
 *
 *              Check that GetTransformFeedbackiv generates INVALID_ENUM error if pname
 *              is not TRANSFORM_FEEDBACK_PAUSED or TRANSFORM_FEEDBACK_ACTIVE.
 *
 *              Check that GetTransformFeedbacki_v generates INVALID_ENUM error if pname
 *              is not TRANSFORM_FEEDBACK_BUFFER_BINDING.
 *
 *              Check that GetTransformFeedbacki64_v generates INVALID_ENUM error if
 *              pname is not TRANSFORM_FEEDBACK_BUFFER_START or
 *              TRANSFORM_FEEDBACK_BUFFER_SIZE.
 *
 *              Check that GetTransformFeedbacki_v and GetTransformFeedbacki64_v
 *              generate INVALID_VALUE error by GetTransformFeedbacki_v and
 *              GetTransformFeedbacki64_v if index is greater than or equal to the
 *              number of binding points for transform feedback (the value of
 *              MAX_TRANSFORM_FEEDBACK_BUFFERS).
 */
class ErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	ErrorsTest(const ErrorsTest& other);
	ErrorsTest& operator=(const ErrorsTest& other);

	/* Function pointers type definitions and declarations. */
	typedef void(GLW_APIENTRY* GetTransformFeedbackiv_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																   glw::GLint* param);
	typedef void(GLW_APIENTRY* GetTransformFeedbacki_v_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																	glw::GLuint index, glw::GLint* param);
	typedef void(GLW_APIENTRY* GetTransformFeedbacki64_v_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																	  glw::GLuint index, glw::GLint64* param);

	GetTransformFeedbackiv_ProcAddress	m_gl_getTransformFeedbackiv;
	GetTransformFeedbacki_v_ProcAddress   m_gl_getTransformFeedbacki_v;
	GetTransformFeedbacki64_v_ProcAddress m_gl_getTransformFeedbacki64_v;

	/* Private member functions. */
	void prepareFunctionPointers();
	void cleanErrors();

	bool testCreateTransformFeedbacksForInvalidNumberOfObjects();
	bool testQueriesForInvalidNameOfObject();
	bool testGetTransformFeedbackivQueryForInvalidParameterName();
	bool testGetTransformFeedbacki_vQueryForInvalidParameterName();
	bool testGetTransformFeedbacki64_vQueryForInvalidParameterName();
	bool testIndexedQueriesForInvalidBindingPoint();
};
/* BuffersTest class */

/** @class FunctionalTest
 *
 *  @brief Direct State Access Transform Feedback Functional test cases.
 *
 *  @note  The test follows steps:
 *
 *             Create transform feedback object with CreateTransformFeedbacks function.
 *
 *             Create buffer object using GenBuffers and BindBuffer functions.
 *             Allocate storage for it using BufferData.
 *
 *             Bind the buffer to transform feedback object indexed binding point 0
 *             using TransformFeedbackBufferBase function.
 *
 *             Prepare program with vertex shader which outputs VertexID to transform
 *             feedback varying.
 *
 *             Create and bind empty vertex array object.
 *
 *             Begin transform feedback environment.
 *
 *             Using the program with discarded rasterizer, draw array of 4 indices
 *             using POINTS.
 *
 *             Pause transform feedback environment.
 *
 *             Query parameter TRANSFORM_FEEDBACK_PAUSED using GetTransformFeedbackiv.
 *             Expect value equal to TRUE.
 *
 *             Query parameter TRANSFORM_FEEDBACK_PAUSED using GetTransformFeedbackiv.
 *             Expect value equal to FALSE.
 *
 *             Resume transform feedback environment.
 *
 *             Query parameter TRANSFORM_FEEDBACK_PAUSED using GetTransformFeedbackiv.
 *             Expect value equal to FALSE.
 *
 *             Query parameter TRANSFORM_FEEDBACK_PAUSED using GetTransformFeedbackiv.
 *             Expect value equal to TRUE.
 *
 *             End Transform feedback environment.
 *
 *             Verify data in the buffer using MapBuffer function.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions */
	FunctionalTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	FunctionalTest(const FunctionalTest& other);
	FunctionalTest& operator=(const FunctionalTest& other);

	/* Function pointers type definitions and declarations. */
	typedef void(GLW_APIENTRY* GetTransformFeedbackiv_ProcAddress)(glw::GLuint xfb, glw::GLenum pname,
																   glw::GLint* param);
	typedef void(GLW_APIENTRY* TransformFeedbackBufferBase_ProcAddress)(glw::GLuint xfb, glw::GLuint index,
																		glw::GLuint buffer);

	GetTransformFeedbackiv_ProcAddress		m_gl_getTransformFeedbackiv;
	TransformFeedbackBufferBase_ProcAddress m_gl_TransformFeedbackBufferBase;

	/* Private member variables. */
	glw::GLuint m_xfb_dsa;
	glw::GLuint m_bo;
	glw::GLuint m_po;
	glw::GLuint m_vao;

	/* Private member functions. */
	void prepareFunctionPointers();
	void prepareTransformFeedback();
	void prepareBuffer();
	void prepareProgram();
	void prepareVertexArrayObject();

	bool draw();
	bool testTransformFeedbackStatus(glw::GLenum parameter_name, glw::GLint expected_value);
	bool verifyBufferContent();

	void clean();

	/* Private static variables. */
	static const glw::GLuint		s_bo_size;
	static const glw::GLchar		s_vertex_shader[];
	static const glw::GLchar		s_fragment_shader[];
	static const glw::GLchar* const s_xfb_varying;
};
/* FunctionalTest class */
} /* xfb namespace */

namespace Samplers
{
/** @class CreationTest
 *
 *  @brief Direct State Access Sampler Objects Creation test cases.
 *
 *  Test follows the steps:
 *
 *       Create at least two Sampler Objects names using GenSamplers function.
 *       Check them without binding, using IsSampler function. Expect GL_FALSE.
 *
 *       Create at least two Sampler Objects using CreateSamplers function. Check
 *       them without binding, using IsSampler function. Expect GL_TRUE.
 *
 *       Release objects.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

/** @class DefaultsTest
 *
 *  @brief Direct State Access Sampler Objects Default State test.
 *
 *         Test follows the steps:
 *
 *             Create Sampler Object with CreateSamplers function.
 *
 *             Verify that default value of TEXTURE_BORDER_COLOR queried with function
 *             GetSamplerParameterfv is 0.0, 0.0, 0.0, 0.0.
 *
 *             Verify that default value of TEXTURE_COMPARE_FUNC queried with function
 *             GetSamplerParameteriv is LEQUAL.
 *
 *             Verify that default value of TEXTURE_COMPARE_MODE queried with function
 *             GetSamplerParameteriv is NONE.
 *
 *             Verify that default value of TEXTURE_LOD_BIAS queried with function
 *             GetSamplerParameterfv is 0.0.
 *
 *             Verify that default value of TEXTURE_MAX_LOD queried with function
 *             GetSamplerParameterfv is 1000.
 *
 *             Verify that default value of TEXTURE_MAG_FILTER queried with function
 *             GetSamplerParameteriv is LINEAR.
 *
 *             Verify that default value of TEXTURE_MIN_FILTER queried with function
 *             GetSamplerParameteriv is NEAREST_MIPMAP_LINEAR.
 *
 *             Verify that default value of TEXTURE_MIN_LOD queried with function
 *             GetSamplerParameterfv is -1000.
 *
 *             Verify that default value of TEXTURE_WRAP_S queried with function
 *             GetSamplerParameteriv is REPEAT.
 *
 *             Verify that default value of TEXTURE_WRAP_T queried with function
 *             GetSamplerParameteriv is REPEAT.
 *
 *             Verify that default value of TEXTURE_WRAP_R queried with function
 *             GetSamplerParameteriv is REPEAT.
 *
 *             Release objects.
 */
class DefaultsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DefaultsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DefaultsTest(const DefaultsTest& other);
	DefaultsTest& operator=(const DefaultsTest& other);

	/* Private member variables */
	glw::GLuint m_sampler_dsa;

	/* Private member functions. */
	void prepare();
	bool testSamplerIntegerParameter(glw::GLenum pname, glw::GLint expected_value);
	bool testSamplerFloatParameter(glw::GLenum pname, glw::GLfloat expected_value);
	bool testSamplerFloatVectorParameter(glw::GLenum pname, glw::GLfloat expected_value[4]);
	void clean();
};
/* DefaultsTest class */

/** @class ErrorsTest
 *
 *  @brief Direct State Access Samplers Negative test.
 *
 *         The test follows steps:
 *
 *             Check that CreateSamplers generates INVALID_VALUE error if
 *             number of sampler objects to create is negative.
 */
class ErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	ErrorsTest(const ErrorsTest& other);
	ErrorsTest& operator=(const ErrorsTest& other);
};
/* ErrorsTest class */

/** @class FunctionalTest
 *
 *  @brief Direct State Access Samplers Functional test cases.
 *
 *  @note  The test follows steps:
 *
 *             Create framebuffer with renderbuffer with color attachment and 1x1 pixel
 *             size. Clean framebuffer content with black color.
 *
 *             Create and bind empty vertex array object.
 *
 *             Build and use simple GLSL program drawing full screen textured quad
 *             depending on VertexID. Fragment shader shall output texture point at
 *             (1/3, 1/3).
 *
 *             Create texture 2 x 2 texels in size. Bind it. Upload texture with
 *             following color data:
 *                 RED,    GREEN,
 *                 BLUE,   YELLOW.
 *
 *             Create Sampler object using CreateSamplers function and bind it to the
 *             texture unit. Setup following sampler parameters:
 *              *  TEXTURE_WRAP_S to the value of REPEAT,
 *              *  TEXTURE_WRAP_T to REPEAT,
 *              *  TEXTURE_MIN_FILTER to NEAREST,
 *              *  TEXTURE_MAG_FILTER to NEAREST.
 *
 *             Draw full screen quad.
 *
 *             Fetch framebuffer content with ReadPixels function. Check that,
 *             framebuffer is filled with red color.
 *
 *             Release objects.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions */
	FunctionalTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	FunctionalTest(const FunctionalTest& other);
	FunctionalTest& operator=(const FunctionalTest& other);

	void prepareFramebuffer();
	void prepareVertexArrayObject();
	void prepareProgram();
	void prepareTexture();
	void prepareSampler();
	void draw();
	bool checkFramebufferContent();
	void clean();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
	glw::GLuint m_vao;
	glw::GLuint m_to;
	glw::GLuint m_so;
	glw::GLuint m_po;

	/* Private static variables. */
	static const glw::GLchar  s_vertex_shader[];
	static const glw::GLchar  s_fragment_shader[];
	static const glw::GLchar  s_uniform_sampler[];
	static const glw::GLubyte s_texture_data[];
};
/* FunctionalTest class */
} /* Samplers namespace */

namespace ProgramPipelines
{
/** @class CreationTest
 *
 *  @brief Direct State Access Program Pipeline Objects Creation test cases.
 *
 *  @note Test follows the steps:
 *
 *            Create at least two Program Pipeline Objects names using
 *            GenProgramPipelines function. Check them without binding, using
 *            IsProgramPipeline function. Expect GL_FALSE.
 *
 *            Create at least two Program Pipeline Objects using
 *            CreateProgramPipelines function. Check them without binding, using
 *            IsProgramPipeline function. Expect GL_TRUE.
 *
 *            Release objects.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

/** @class DefaultsTest
 *
 *  @brief Direct State Access Program Pipeline Objects Default State test.
 *
 *  @note Test follows the steps:
 *
 *            Create Program Pipeline Object with CreateProgramPipelines function.
 *
 *            Verify that default value of ACTIVE_PROGRAM queried with function
 *            GetProgramPipelineiv is 0.
 *
 *            Verify that default value of VERTEX_SHADER queried with function
 *            GetProgramPipelineiv is 0.
 *
 *            Verify that default value of GEOMETRY_SHADER queried with function
 *            GetProgramPipelineiv is 0.
 *
 *            Verify that default value of FRAGMENT_SHADER queried with function
 *            GetProgramPipelineiv is 0.
 *
 *            Verify that default value of COMPUTE_SHADER queried with function
 *            GetProgramPipelineiv is 0.
 *
 *            Verify that default value of TESS_CONTROL_SHADER queried with function
 *            GetProgramPipelineiv is 0.
 *
 *            Verify that default value of TESS_EVALUATION_SHADER queried with
 *            function GetProgramPipelineiv is 0.
 *
 *            Verify that default value of VALIDATE_STATUS queried with function
 *            GetProgramPipelineiv is 0.
 *
 *            Verify that default value of info log queried with function
 *            GetProgramPiplineInfoLog is 0.
 *
 *            Verify that default value of INFO_LOG_LENGTH queried with function
 *            GetProgramPipelineiv is 0.
 *
 *            Release object.
 */
class DefaultsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DefaultsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DefaultsTest(const DefaultsTest& other);
	DefaultsTest& operator=(const DefaultsTest& other);

	/* Private member variables */
	glw::GLuint m_program_pipeline_dsa;

	/* Private member functions. */
	void prepare();
	bool testProgramPipelineParameter(glw::GLenum pname, glw::GLint expected_value);
	bool testProgramPipelineInfoLog(glw::GLchar* expected_value);
	void clean();
};
/* DefaultsTest class */

/** @class ErrorsTest
 *
 *  @brief Direct State Access Program Pipeline Negative test.
 *
 *         The test follows steps:
 *
 *             Check that CreateProgramPipelines generates INVALID_VALUE error if
 *             number of program pipeline objects to create is negative.
 */
class ErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	ErrorsTest(const ErrorsTest& other);
	ErrorsTest& operator=(const ErrorsTest& other);
};
/* ErrorsTest class */

/** @class FunctionalTest
 *
 *  @brief Direct State Access Program Pipeline Functional test cases.
 *
 *  @note  The test follows steps:
 *
 *             Create framebuffer with renderbuffer with color attachment and 1x1 pixel
 *             size. Clean framebuffer content with black color.
 *
 *             Create and bind empty vertex array object.
 *
 *             Make sure that no GLSL program is being used.
 *
 *             Create two shader programs (with CreateShaderProgramv) - one vertex
 *             shader and one fragment shader. The vertex shader shall output full
 *             screen quad depending on VertexID. The fragment shader shall output red
 *             color.
 *
 *             Create the Program Pipeline Object using CreateProgramPipelines
 *             function. Bind it using BindProgramPipeline. Setup Program Pipeline
 *             with the created shader programs using UseProgramStages.
 *
 *             Draw full screen quad.
 *
 *             Fetch framebuffer content with ReadPixels function. Check that,
 *             framebuffer is filled with red color.
 *
 *             Release objects.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions */
	FunctionalTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	FunctionalTest(const FunctionalTest& other);
	FunctionalTest& operator=(const FunctionalTest& other);

	void prepareFramebuffer();
	void prepareVertexArrayObject();
	void prepareShaderPrograms();
	void preparePipeline();
	void draw();
	bool checkFramebufferContent();
	void clean();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
	glw::GLuint m_vao;
	glw::GLuint m_spo_v;
	glw::GLuint m_spo_f;
	glw::GLuint m_ppo;

	/* Private static variables. */
	static const glw::GLchar* s_vertex_shader;
	static const glw::GLchar* s_fragment_shader;
};
/* FunctionalTest class */
} /* ProgramPipelines namespace */

namespace Queries
{
/** @class CreationTest
 *
 *  @brief Direct State Access Queries Creation test cases.
 *
 *  @note Test follows the steps:
 *
 *            Create at least two Query Objects names using GenQueries function.
 *            Check them without binding, using IsQuery function. Expect GL_FALSE.
 *
 *            Create at least two Query Objects using CreateQueries function. Check
 *            them without binding, using IsQuery function. Expect GL_TRUE.
 *
 *            Release objects.
 *
 *            Repeat test for all of following supported targets:
 *             -  SAMPLES_PASSED,
 *             -  ANY_SAMPLES_PASSED,
 *             -  ANY_SAMPLES_PASSED_CONSERVATIVE,
 *             -  TIME_ELAPSED,
 *             -  TIMESTAMP,
 *             -  PRIMITIVES_GENERATED and
 *             -  TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN.
 *
 *  See Ref. OpenGL 4.5 Core Profile, Section 4.2.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

/** @class DefaultsTest
 *
 *  @brief Direct State Access Queries Default State test.
 *
 *  @note Test follows the steps:
 *
 *            Create Query Object with CreateQueries function.
 *
 *            Verify that default value of QUERY_RESULT queried with function
 *            GetQueryObjectuiv is 0 or FALSE.
 *
 *            Verify that default value of QUERY_RESULT_AVAILABLE queried with
 *            function GetQueryObjectiv is TRUE.
 *
 *            Release object.
 *
 *            Repeat test for all of following supported targets:
 *             -  SAMPLES_PASSED,
 *             -  ANY_SAMPLES_PASSED,
 *             -  ANY_SAMPLES_PASSED_CONSERVATIVE,
 *             -  TIME_ELAPSED,
 *             -  TIMESTAMP,
 *             -  PRIMITIVES_GENERATED and
 *             -  TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN.
 *
 *  See Ref. OpenGL 4.5 Core Profile, Section 4.2.
 */
class DefaultsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DefaultsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DefaultsTest(const DefaultsTest& other);
	DefaultsTest& operator=(const DefaultsTest& other);

	/* Private member variables */
	glw::GLuint m_query_dsa;

	/* Private member functions. */
	void prepare(const glw::GLenum target);
	bool testQueryParameter(const glw::GLenum pname, const glw::GLuint expected_value, const glw::GLchar* target_name);
	void clean();
};
/* DefaultsTest class */

/** @class ErrorsTest
 *
 *  @brief Direct State Access Queries Negative test.
 *
 *  @note The test follows steps:
 *
 *            Check that CreateQueries generates INVALID_VALUE error if number of
 *            query objects to create is negative.
 *
 *            Check that CreateQueries generates INVALID_ENUM error if target is not
 *            one of accepted values:
 *             -  SAMPLES_PASSED,
 *             -  ANY_SAMPLES_PASSED,
 *             -  ANY_SAMPLES_PASSED_CONSERVATIVE,
 *             -  TIME_ELAPSED,
 *             -  TIMESTAMP,
 *             -  PRIMITIVES_GENERATED or
 *             -  TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN.
 *
 *            Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *            GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *            INVALID_OPERATION error if <id> is not the name of a query object, or
 *            if the query object named by <id> is currently active.
 *
 *            Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *            GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *            INVALID_OPERATION error if <buffer> is not the name of an existing
 *            buffer object.
 *
 *            Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *            GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *            INVALID_ENUM error if <pname> is not QUERY_RESULT,
 *            QUERY_RESULT_AVAILABLE, QUERY_RESULT_NO_WAIT or QUERY_TARGET.
 *
 *            Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *            GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *            INVALID_OPERATION error if the query writes to a buffer object, and the
 *            specified buffer offset would cause data to be written beyond the bounds
 *            of that buffer object.
 *
 *            Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *            GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *            INVALID_VALUE error if <offset> is negative.
 *
 *  See Ref. OpenGL 4.5 Core Profile, Section 4.2.
 */
class ErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	ErrorsTest(const ErrorsTest& other);
	ErrorsTest& operator=(const ErrorsTest& other);

	/* Private member functions. */
	bool testNegativeNumberOfObjects();
	bool testInvalidTarget();
	bool testInvalidQueryName();
	bool testInvalidBufferName();
	bool testInvalidParameterName();
	bool testBufferOverflow();
	bool testBufferNegativeOffset();

	bool isTarget(glw::GLenum maybe_target);
	bool isParameterName(glw::GLenum maybe_pname);

	/* Function pointers. */
	typedef void(GLW_APIENTRY* PFNGLGETQUERYBUFFEROBJECT)(glw::GLuint id, glw::GLuint buffer, glw::GLenum pname,
														  glw::GLintptr offset);

	PFNGLGETQUERYBUFFEROBJECT m_pGetQueryBufferObjectiv;
	PFNGLGETQUERYBUFFEROBJECT m_pGetQueryBufferObjectuiv;
	PFNGLGETQUERYBUFFEROBJECT m_pGetQueryBufferObjecti64v;
	PFNGLGETQUERYBUFFEROBJECT m_pGetQueryBufferObjectui64v;

	/* Private static variables. */
	static const glw::GLenum  s_targets[];
	static const glw::GLchar* s_target_names[];
	static const glw::GLuint  s_targets_count;
};
/* ErrorsTest class */

/** @class FunctionalTest
 *
 *  @brief Direct State Access Queries Functional test cases.
 *
 *  @note The test follows steps:
 *
 *            Create framebuffer with renderbuffer with color attachment and 1x1 pixel
 *            size. Clean framebuffer content with black color.
 *
 *            Create and bind empty vertex array object.
 *
 *            Create buffer object. Bind it to TRANFORM_FEEDBACK_BUFFER binding point.
 *            Bind buffer base to TRANFORM_FEEDBACK_BUFFER binding point with index 0.
 *            Setup data storage of the buffer with size equal to 6 * sizeof(int).
 *
 *            Build GLSL program consisting of vertex and fragment shader stages.
 *            Vertex shader shall output full screen quad depending on VertexID. The
 *            VertexID shall be saved to transform feedback varying. Fragment shader
 *            shall output red color.
 *
 *            Create query objects with CreateQueries function for following targets:
 *             -  SAMPLES_PASSED,
 *             -  PRIMITIVES_GENERATED,
 *             -  TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,
 *             -  TIME_ELAPSED.
 *
 *            Begin query for every target.
 *
 *            Begin transform feedback for TRIANGLES primitive type.
 *
 *            Draw full screen quad with TRIANGLE_STRIP primitive type.
 *
 *            End transform feedback.
 *
 *            End all queries.
 *
 *            Call Finish function.
 *
 *            Check that framebuffer is filled with red color.
 *
 *            Check that transform feedback buffer contains successive primitive
 *            vertex ids (0, 1, 2,  2, 1, 3).
 *
 *            For every query objects, using GetQueryBufferObjectiv,
 *            GetQueryBufferObjectuiv, GetQueryBufferObjecti64v,
 *            GetQueryBufferObjectui64v functions do following comparisons:
 *
 *                Check that value of parameter QUERY_TARGET is equal to target.
 *
 *                Check that value of parameter QUERY_RESULT_AVAILABLE is TRUE.
 *
 *                Check that value of parameter QUERY_RESULT and QUERY_RESULT_NO_WAIT:
 *                 -  is equal to 1 if target is SAMPLES_PASSED; or
 *                 -  is equal to 2 if target is PRIMITIVES_GENERATED or
 *                    TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN; or
 *                 -  is positive if target is TIME_ELAPSED.
 *
 *            Release objects.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions */
	FunctionalTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	FunctionalTest(const FunctionalTest& other);
	FunctionalTest& operator=(const FunctionalTest& other);

	/* Function pointers. */
	typedef void(GLW_APIENTRY* PFNGLGETQUERYBUFFEROBJECT)(glw::GLuint id, glw::GLuint buffer, glw::GLenum pname,
														  glw::GLintptr offset);

	PFNGLGETQUERYBUFFEROBJECT m_pGetQueryBufferObjectiv;
	PFNGLGETQUERYBUFFEROBJECT m_pGetQueryBufferObjectuiv;
	PFNGLGETQUERYBUFFEROBJECT m_pGetQueryBufferObjecti64v;
	PFNGLGETQUERYBUFFEROBJECT m_pGetQueryBufferObjectui64v;

	/* Private templated functions. */
	template <typename T>
	static bool equal(T, T);

	template <typename T>
	static bool less(T, T);

	template <typename T>
	void GetQueryBufferObject(glw::GLuint id, glw::GLuint buffer, glw::GLenum pname, glw::GLintptr offset);

	template <typename T>
	bool checkQueryBufferObject(glw::GLuint query, glw::GLenum pname, T expected_value, bool (*comparison)(T, T));

	/* Private member functions. */
	void prepareView();
	void prepareVertexArray();
	void prepareBuffers();
	void prepareQueries();
	void prepareProgram();
	void draw();
	bool checkView();
	bool checkXFB();
	void clean();

	/* Private member variables. */
	glw::GLuint  m_fbo;
	glw::GLuint  m_rbo;
	glw::GLuint  m_vao;
	glw::GLuint  m_bo_query;
	glw::GLuint  m_bo_xfb;
	glw::GLuint* m_qo;
	glw::GLuint  m_po;

	/* Private static variables. */
	static const glw::GLenum s_targets[];
	static const glw::GLuint s_targets_count;

	static const glw::GLint s_results[];

	static const glw::GLchar  s_vertex_shader[];
	static const glw::GLchar  s_fragment_shader[];
	static const glw::GLchar* s_xfb_varying_name;
};
/* FunctionalTest class */
} /* Queries namespace */

namespace Buffers
{
/** @class CreationTest
 *
 *  @brief Direct State Access Buffers Creation test cases.
 *
 *         Test follows the steps:
 *
 *             Create at least two buffer objects using GenBuffers function. Check
 *             them without binding, using IsBuffer function. Expect GL_FALSE.
 *
 *             Create at least two buffer objects using CreateBuffers function. Check
 *             them without binding, using IsBuffer function. Expect GL_TRUE.
 *
 *             Release objects.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

/** @class DataTest
 *
 *  @brief Direct State Access Buffers Data test cases.
 *
 *         Test follows the steps:
 *
 *             Prepare test case with following steps:
 *
 *             Create buffer object using CreateBuffers.
 *
 *             Create data storage using given function and reference data.
 *
 *             Bind buffer.
 *
 *             Check buffer content using MapBuffer function.
 *
 *             Release objects.
 *
 *             Repeat the test case with function for data creation:
 *              -  NamedBufferData,
 *              -  NamedBufferData and it with NamedBufferSubData,
 *              -  NamedBufferStorage,
 *              -  CopyNamedBufferSubData from auxiliary buffer.
 *
 *             If NamedBufferData function is used then repeat the test case for
 *             usage:
 *              -  STREAM_DRAW,
 *              -  STREAM_READ,
 *              -  STREAM_COPY,
 *              -  STATIC_DRAW,
 *              -  STATIC_READ,
 *              -  STATIC_COPY,
 *              -  DYNAMIC_DRAW,
 *              -  DYNAMIC_READ, and
 *              -  DYNAMIC_COPY.
 *
 *             If NamedBufferStorage function is used then repeat the test case using
 *             flag MAP_READ_BIT and one of following:
 *              -  DYNAMIC_STORAGE_BIT,
 *              -  MAP_WRITE_BIT,
 *              -  MAP_PERSISTENT_BIT,
 *              -  MAP_COHERENT_BIT and
 *              -  CLIENT_STORAGE_BIT.
 */
class DataTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DataTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DataTest(const DataTest& other);
	DataTest& operator=(const DataTest& other);

	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizei size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei size,
														const glw::GLvoid* data);
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERSTORAGE)(glw::GLuint buffer, glw::GLsizeiptr size,
														const glw::GLvoid* data, glw::GLbitfield flags);
	typedef void(GLW_APIENTRY* PFNGLCOPYNAMEDBUFFERSUBDATA)(glw::GLuint readBuffer, glw::GLuint writeBuffer,
															glw::GLintptr readOffset, glw::GLintptr writeOffset,
															glw::GLsizeiptr size);

	PFNGLNAMEDBUFFERDATA		m_pNamedBufferData;
	PFNGLNAMEDBUFFERSUBDATA		m_pNamedBufferSubData;
	PFNGLNAMEDBUFFERSTORAGE		m_pNamedBufferStorage;
	PFNGLCOPYNAMEDBUFFERSUBDATA m_pCopyNamedBufferSubData;

	bool TestCase(void (DataTest::*UploadDataFunction)(glw::GLuint, glw::GLenum), glw::GLenum parameter);

	void UploadUsingNamedBufferData(glw::GLuint id, glw::GLenum parameter);
	void UploadUsingNamedBufferSubData(glw::GLuint id, glw::GLenum parameter);
	void UploadUsingNamedBufferStorage(glw::GLuint id, glw::GLenum parameter);
	void UploadUsingCopyNamedBufferSubData(glw::GLuint id, glw::GLenum parameter);

	bool compare(const glw::GLuint* data, const glw::GLuint* reference, const glw::GLsizei count);
	void LogFail(void (DataTest::*UploadDataFunction)(glw::GLuint, glw::GLenum), glw::GLenum parameter,
				 const glw::GLuint* data, const glw::GLuint* reference, const glw::GLsizei count);
	void LogError(void (DataTest::*UploadDataFunction)(glw::GLuint, glw::GLenum), glw::GLenum parameter);

	static const glw::GLuint  s_reference[];
	static const glw::GLsizei s_reference_size;
	static const glw::GLsizei s_reference_count;
};
/* DataTest class */

/** @class ClearTest
 *
 *  @brief Direct State Access Buffers Clear test cases.
 *
 *         Test follows the steps:
 *
 *             Prepare test case with following steps:
 *
 *                 Create buffer object using CreateBuffers.
 *
 *                 Create data storage using NamedBufferData without data
 *                 specification.
 *
 *                 Clear buffer content using given function.
 *
 *                 Bind buffer.
 *
 *                 Check buffer content using MapBuffer function.
 *
 *                 Release objects.
 *
 *             Repeat test case for following clear functions:
 *              -  ClearNamedBufferData and
 *              -  ClearNamedBufferSubData.
 *
 *             Repeat test case for following internal formats:
 *              -  GL_R8,
 *              -  GL_R16,
 *              -  GL_R16F,
 *              -  GL_R32F,
 *              -  GL_R8I,
 *              -  GL_R16I,
 *              -  GL_R32I,
 *              -  GL_R8UI,
 *              -  GL_R16UI,
 *              -  GL_R32UI,
 *              -  GL_RG8,
 *              -  GL_RG16,
 *              -  GL_RG16F,
 *              -  GL_RG32F,
 *              -  GL_RG8I,
 *              -  GL_RG16I,
 *              -  GL_RG32I,
 *              -  GL_RG8UI,
 *              -  GL_RG16UI,
 *              -  GL_RG32UI,
 *              -  GL_RGB32F,
 *              -  GL_RGB32I,
 *              -  GL_RGB32UI,
 *              -  GL_RGBA8,
 *              -  GL_RGBA16,
 *              -  GL_RGBA16F,
 *              -  GL_RGBA32F,
 *              -  GL_RGBA8I,
 *              -  GL_RGBA16I,
 *              -  GL_RGBA32I,
 *              -  GL_RGBA8UI,
 *              -  GL_RGBA16UI and
 *              -  GL_RGBA32UI.
 */
class ClearTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ClearTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	ClearTest(const ClearTest& other);
	ClearTest& operator=(const ClearTest& other);

	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizei size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void(GLW_APIENTRY* PFNGLCLEARNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLenum internalformat,
														  glw::GLenum format, glw::GLenum type,
														  const glw::GLvoid* data);
	typedef void(GLW_APIENTRY* PFNGLCLEARNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLenum internalformat,
															 glw::GLintptr offset, glw::GLsizei size,
															 glw::GLenum format, glw::GLenum type,
															 const glw::GLvoid* data);

	PFNGLNAMEDBUFFERDATA		 m_pNamedBufferData;
	PFNGLCLEARNAMEDBUFFERDATA	m_pClearNamedBufferData;
	PFNGLCLEARNAMEDBUFFERSUBDATA m_pClearNamedBufferSubData;

	template <typename T, bool USE_SUB_DATA>
	bool TestClearNamedBufferData(glw::GLenum internalformat, glw::GLsizei count, glw::GLenum format, glw::GLenum type,
								  T* data);

	template <bool USE_SUB_DATA>
	void ClearNamedBuffer(glw::GLuint buffer, glw::GLenum internalformat, glw::GLsizei size, glw::GLenum format,
						  glw::GLenum type, glw::GLvoid* data);

	template <typename T>
	bool Compare(const T* data, const T* reference, const glw::GLsizei count);

	template <typename T>
	void LogFail(bool use_sub_data, glw::GLenum internalformat, const T* data, const T* reference,
				 const glw::GLsizei count);

	void LogError(bool use_sub_data, glw::GLenum internalformat);
};
/* ClearTest class */

/** @class MapReadOnlyTest
 *
 *  @brief Direct State Access Buffers Map Read Only test cases.
 *
 *         Test follows the steps:
 *
 *             Create buffer object using CreateBuffers.
 *
 *             Create data storage using NamedBufferData function and reference
 *             data.
 *
 *             Map buffer with MapNamedBuffer function and READ_ONLY access flag.
 *
 *             Compare mapped buffer content with reference data.
 *
 *             Unmap buffer using UnmapNamedBuffer. Test if UnmapNamedBuffer
 *             returned GL_TRUE.
 *
 *             Release buffer.
 */
class MapReadOnlyTest : public deqp::TestCase
{
public:
	/* Public member functions */
	MapReadOnlyTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	MapReadOnlyTest(const MapReadOnlyTest& other);
	MapReadOnlyTest& operator=(const MapReadOnlyTest& other);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizei size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFER)(glw::GLuint buffer, glw::GLenum access);
	typedef glw::GLboolean(GLW_APIENTRY* PFNGLUNMAPNAMEDBUFFER)(glw::GLuint buffer);

	PFNGLNAMEDBUFFERDATA  m_pNamedBufferData;
	PFNGLMAPNAMEDBUFFER   m_pMapNamedBuffer;
	PFNGLUNMAPNAMEDBUFFER m_pUnmapNamedBuffer;

	static const glw::GLuint  s_reference[];	 //<! Reference data.
	static const glw::GLsizei s_reference_size;  //<! Reference data size.
	static const glw::GLsizei s_reference_count; //<! Reference data count (number of GLuint elements).
};
/* MapReadOnlyTest class */

/** @class MapReadWriteTest
 *
 *  @brief Direct State Access Buffers Map Read Write test cases.
 *
 *         Test follows the steps:
 *
 *             Create buffer object using CreateBuffers.
 *
 *             Create data storage using NamedBufferData function and reference
 *             data.
 *
 *             Map buffer with MapNamedBuffer function and READ_WRITE access flag.
 *
 *             Compare mapped buffer content with reference.
 *
 *             Write to the mapped buffer inverted reference content.
 *
 *             Unmap buffer.
 *
 *             Map buffer with MapNamedBuffer function and READ_WRITE access flag.
 *
 *             Compare mapped buffer content with inverted reference.
 *
 *             Unmap buffer.
 *
 *             Release buffer.
 */
class MapReadWriteTest : public deqp::TestCase
{
public:
	/* Public member functions */
	MapReadWriteTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	MapReadWriteTest(const MapReadWriteTest& other);
	MapReadWriteTest& operator=(const MapReadWriteTest& other);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizei size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFER)(glw::GLuint buffer, glw::GLenum access);
	typedef glw::GLboolean(GLW_APIENTRY* PFNGLUNMAPNAMEDBUFFER)(glw::GLuint buffer);

	PFNGLNAMEDBUFFERDATA  m_pNamedBufferData;
	PFNGLMAPNAMEDBUFFER   m_pMapNamedBuffer;
	PFNGLUNMAPNAMEDBUFFER m_pUnmapNamedBuffer;

	static const glw::GLuint  s_reference[];	 //<! Reference data.
	static const glw::GLsizei s_reference_size;  //<! Reference data size.
	static const glw::GLsizei s_reference_count; //<! Reference data count (number of GLuint elements).
};
/* MapReadWriteTest class */

/** @class MapWriteOnlyTest
 *
 *  @brief Direct State Access Buffers Map Write Only test cases.
 *
 *         Test follows the steps:
 *
 *             Create buffer object using CreateBuffers.
 *
 *             Create data storage using NamedBufferData function.
 *
 *             Map buffer with MapNamedBuffer function and WRITE_ONLY access flag.
 *
 *             Write reference data.
 *
 *             Unmap buffer.
 *
 *             Bind buffer to the binding point.
 *
 *             Map buffer with MapBuffer function and READ_ONLY access flag.
 *
 *             Compare mapped buffer content with reference.
 *
 *             Unmap buffer.
 *
 *             Release buffer.
 */
class MapWriteOnlyTest : public deqp::TestCase
{
public:
	/* Public member functions */
	MapWriteOnlyTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	MapWriteOnlyTest(const MapWriteOnlyTest& other);
	MapWriteOnlyTest& operator=(const MapWriteOnlyTest& other);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizei size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFER)(glw::GLuint buffer, glw::GLenum access);
	typedef glw::GLboolean(GLW_APIENTRY* PFNGLUNMAPNAMEDBUFFER)(glw::GLuint buffer);

	PFNGLNAMEDBUFFERDATA  m_pNamedBufferData;
	PFNGLMAPNAMEDBUFFER   m_pMapNamedBuffer;
	PFNGLUNMAPNAMEDBUFFER m_pUnmapNamedBuffer;

	static const glw::GLuint  s_reference[];	 //<! Reference data.
	static const glw::GLsizei s_reference_size;  //<! Reference data size.
	static const glw::GLsizei s_reference_count; //<! Reference data count (number of GLuint elements).
};
/* MapReadOnlyTest class */

/** @class MapRangeReadBitTest
 *
 *  @brief Direct State Access Buffers Range Map Read Bit test cases.
 *
 *         Test follows the steps:
 *
 *             Create buffer object using CreateBuffers.
 *
 *             Create data storage using NamedBufferStorage function, reference
 *             data and MAP_READ_BIT access flag.
 *
 *             Map first half of buffer with MapNamedBufferRange and MAP_READ_BIT
 *             access flag.
 *
 *             Compare mapped buffer content with reference.
 *
 *             Unmap buffer.
 *
 *             Map second half of buffer with MapNamedBufferRange and MAP_READ_BIT
 *             access flag.
 *
 *             Compare mapped buffer content with reference.
 *
 *             Unmap buffer.
 *
 *             Release buffer.
 *
 *             Repeat the test with also MAP_PERSISTENT_BIT flag turned on.
 *
 *             Repeat the test with also MAP_PERSISTENT_BIT and MAP_COHERENT_BIT
 *             flags turned on.
 */
class MapRangeReadBitTest : public deqp::TestCase
{
public:
	/* Public member functions */
	MapRangeReadBitTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	MapRangeReadBitTest(const MapRangeReadBitTest& other);
	MapRangeReadBitTest& operator=(const MapRangeReadBitTest& other);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERSTORAGE)(glw::GLuint buffer, glw::GLsizeiptr size,
														const glw::GLvoid* data, glw::GLbitfield flags);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFERRANGE)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei length,
														  glw::GLbitfield access);
	typedef glw::GLboolean(GLW_APIENTRY* PFNGLUNMAPNAMEDBUFFER)(glw::GLuint buffer);

	PFNGLNAMEDBUFFERSTORAGE  m_pNamedBufferStorage;
	PFNGLMAPNAMEDBUFFERRANGE m_pMapNamedBufferRange;
	PFNGLUNMAPNAMEDBUFFER	m_pUnmapNamedBuffer;

	bool CompareWithReference(glw::GLuint* data, glw::GLintptr offset, glw::GLsizei length);

	static const glw::GLuint  s_reference[];	 //<! Reference data.
	static const glw::GLsizei s_reference_size;  //<! Reference data size.
	static const glw::GLsizei s_reference_count; //<! Reference data count (number of GLuint elements).
};
/* MapRangeReadBitTest class */

/** @class MapRangeWriteBitTest
 *
 *  @brief Direct State Access Buffers Range Map Read Bit test cases.
 *
 *         Test follows the steps:
 *
 *             Create buffer object using CreateBuffers.
 *
 *             Create data storage using NamedBufferStorage function, reference
 *             data and (MAP_READ_BIT | MAP_WRITE_BIT) access flag.
 *
 *             Map first half of buffer with MapNamedBufferRange and MAP_WRITE_BIT
 *             access flag.
 *
 *             Write reference data.
 *
 *             Unmap buffer.
 *
 *             Map second half of buffer with MapNamedBufferRange and MAP_WRITE_BIT
 *             access flag.
 *
 *             Write reference data.
 *
 *             Unmap buffer.
 *
 *             Bind buffer to the binding point.
 *
 *             Map buffer with MapBuffer function and READ_ONLY access flag.
 *
 *             Compare mapped buffer content with reference.
 *
 *             Unmap buffer.
 *
 *             Release buffer.
 *
 *             Repeat the test with also MAP_INVALIDATE_RANGE_BIT flag turned on.
 *
 *             Repeat the test with also MAP_INVALIDATE_BUFFER_BIT flag turned on with
 *             only the first mapping.
 *
 *             Repeat the test with also MAP_FLUSH_EXPLICIT_BIT flag turned on. Make
 *             sure that all writes are flushed using FlushNamedMappedBufferRange
 *             function.
 *
 *             Repeat the test with also MAP_UNSYNCHRONIZED_BIT flag turned on with
 *             only the second mapping.
 */
class MapRangeWriteBitTest : public deqp::TestCase
{
public:
	/* Public member functions */
	MapRangeWriteBitTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	MapRangeWriteBitTest(const MapRangeWriteBitTest& other);
	MapRangeWriteBitTest& operator=(const MapRangeWriteBitTest& other);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERSTORAGE)(glw::GLuint buffer, glw::GLsizeiptr size,
														const glw::GLvoid* data, glw::GLbitfield flags);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFERRANGE)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei length,
														  glw::GLbitfield access);
	typedef glw::GLboolean(GLW_APIENTRY* PFNGLUNMAPNAMEDBUFFER)(glw::GLuint buffer);
	typedef void(GLW_APIENTRY* PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE)(glw::GLuint buffer, glw::GLintptr offset,
																 glw::GLsizei length);

	PFNGLNAMEDBUFFERSTORAGE			 m_pNamedBufferStorage;
	PFNGLMAPNAMEDBUFFERRANGE		 m_pMapNamedBufferRange;
	PFNGLUNMAPNAMEDBUFFER			 m_pUnmapNamedBuffer;
	PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE m_pFlushMappedNamedBufferRange;

	bool CompareWithReference(glw::GLuint buffer, glw::GLbitfield access_flag);

	static const glw::GLuint  s_reference[];	 //<! Reference data.
	static const glw::GLsizei s_reference_size;  //<! Reference data size.
	static const glw::GLsizei s_reference_count; //<! Reference data count (number of GLuint elements).
};
/* MapRangeWriteBitTest class */

/** @class SubDataQueryTest
 *
 *  @brief Direct State Access GetNamedBufferSubData Query test cases.
 *
 *         Test follows the steps:
 *
 *             Create buffer object using CreateBuffers.
 *
 *             Create data storage using NamedBufferData function and reference data.
 *
 *             Fetch first half of the buffer using GetNamedBufferSubData function.
 *
 *             Fetch second half of the buffer using GetNamedBufferSubData function.
 *
 *             Compare fetched data with reference values.
 *
 *             Release object.
 */
class SubDataQueryTest : public deqp::TestCase
{
public:
	/* Public member functions */
	SubDataQueryTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	SubDataQueryTest(const SubDataQueryTest& other);
	SubDataQueryTest& operator=(const SubDataQueryTest& other);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizei size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void*(GLW_APIENTRY* PFNGLGETNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei size,
															glw::GLvoid* data);

	PFNGLNAMEDBUFFERDATA	   m_pNamedBufferData;
	PFNGLGETNAMEDBUFFERSUBDATA m_pGetNamedBufferSubData;

	static const glw::GLuint  s_reference[];	 //<! Reference data.
	static const glw::GLsizei s_reference_size;  //<! Reference data size.
	static const glw::GLsizei s_reference_count; //<! Reference data count (number of GLuint elements).
};
/* SubDataQueryTest class */

/** @class DefaultsTest
 *
 *  @brief Direct State Access Buffer Objects Default Values Test.
 *
 *         Test follows the steps:
 *
 *             Create buffer object using CreateBuffers.
 *
 *             Check that GetNamedBufferParameteriv and GetNamedBufferParameteri64v
 *             function called with parameter name
 *              -  BUFFER_SIZE returns value equal to 0;
 *              -  BUFFER_USAGE returns value equal to STATIC_DRAW;
 *              -  BUFFER_ACCESS returns value equal to READ_WRITE;
 *              -  BUFFER_ACCESS_FLAGS returns value equal to 0;
 *              -  BUFFER_IMMUTABLE_STORAGE returns value equal to FALSE;
 *              -  BUFFER_MAPPED returns value equal to FALSE;
 *              -  BUFFER_MAP_OFFSET returns value equal to 0;
 *              -  BUFFER_MAP_LENGTH returns value equal to 0;
 *              -  BUFFER_STORAGE_FLAGS returns value equal to 0.
 *
 *            Check that GetNamedBufferPointerv function called with parameter name
 *            BUFFER_MAP_POINTER returns value equal to NULL;
 */
class DefaultsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DefaultsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DefaultsTest(const DefaultsTest& other);
	DefaultsTest& operator=(const DefaultsTest& other);

	bool CheckParameterError(const glw::GLchar* pname_string, const glw::GLchar* function_string);

	template <typename T>
	bool CheckValue(const T value, const T reference_value, const glw::GLchar* pname_string,
					const glw::GLchar* function_string);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizei size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERI64V)(glw::GLuint buffer, glw::GLenum pname,
																 glw::GLint64* data);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERIV)(glw::GLuint buffer, glw::GLenum pname, glw::GLint* data);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPOINTERV)(glw::GLuint buffer, glw::GLenum pname,
															glw::GLvoid** params);

	PFNGLNAMEDBUFFERDATA			 m_pNamedBufferData;
	PFNGLGETNAMEDBUFFERPARAMETERI64V m_pGetNamedBufferParameteri64v;
	PFNGLGETNAMEDBUFFERPARAMETERIV   m_pGetNamedBufferParameteriv;
	PFNGLGETNAMEDBUFFERPOINTERV		 m_pGetNamedBufferPointerv;
};
/* DefaultsTest class */

/** @class ErrorsTest
 *
 *  @brief Direct State Access Buffer Objects Errors Test.
 *
 *         Test follows the steps:
 *
 *                     Check that INVALID_VALUE is generated by CreateBuffers if n is negative.
 *
 *
 *                     Check that INVALID_OPERATION is generated by NamedBufferStorage if
 *                     buffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_VALUE is generated by NamedBufferStorage if size is
 *                     less than or equal to zero.
 *
 *                     Check that INVALID_VALUE is generated by NamedBufferStorage if flags has
 *                     any bits set other than DYNAMIC_STORAGE_BIT, MAP_READ_BIT,
 *                     MAP_WRITE_BIT, MAP_PERSISTENT_BIT, MAP_COHERENT_BIT or
 *                     CLIENT_STORAGE_BIT.
 *
 *                     Check that INVALID_VALUE error is generated by NamedBufferStorage if
 *                     flags contains MAP_PERSISTENT_BIT but does not contain at least one of
 *                     MAP_READ_BIT or MAP_WRITE_BIT.
 *
 *                     Check that INVALID_VALUE is generated by NamedBufferStorage if flags
 *                     contains MAP_COHERENT_BIT, but does not also contain MAP_PERSISTENT_BIT.
 *
 *                     Check that OUT_OF_MEMORY is generated if the GL is unable to create a
 *                     data store with the specified size. Do not set result, if out of memory
 *                     situation was impossible to generate or unable to verify.
 *
 *
 *                     Check that INVALID_OPERATION is generated by NamedBufferData if buffer
 *                     is not the name of an existing buffer object.
 *
 *                     Check that INVALID_ENUM is generated by NamedBufferData if usage is not
 *                     STREAM_DRAW, STREAM_READ, STREAM_COPY, STATIC_DRAW, STATIC_READ,
 *                     STATIC_COPY, DYNAMIC_DRAW, DYNAMIC_READ or DYNAMIC_COPY.
 *
 *                     Check that INVALID_VALUE is generated by NamedBufferData if size is
 *                     negative.
 *
 *                     Check that INVALID_OPERATION is generated by NamedBufferData if the
 *                     BUFFER_IMMUTABLE_STORAGE flag of the buffer object is TRUE.
 *
 *                     Check that OUT_OF_MEMORY is generated if the GL is unable to create a
 *                     data store with the specified size. Do not set result, if out of memory
 *                     situation was impossible to generate or unable to verify.
 *
 *
 *                     Check that INVALID_OPERATION is generated by NamedBufferSubData if
 *                     buffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_VALUE is generated by NamedBufferSubData if offset or
 *                     size is negative, or if offset+size is greater than the value of
 *                     BUFFER_SIZE for the specified buffer object.
 *
 *                     Check that INVALID_OPERATION is generated by NamedBufferSubData if any
 *                     part of the specified range of the buffer object is mapped with
 *                     MapBufferRange or MapBuffer, unless it was mapped with the
 *                     MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.
 *
 *                     Check that INVALID_OPERATION is generated by NamedBufferSubData if the
 *                     value of the BUFFER_IMMUTABLE_STORAGE flag of the buffer object is TRUE
 *                     and the value of BUFFER_STORAGE_FLAGS for the buffer object does not
 *                     have the DYNAMIC_STORAGE_BIT bit set.
 *
 *
 *                     Check that INVALID_OPERATION is generated by ClearNamedBufferData if
 *                     buffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_ENUM is generated by ClearNamedBufferData if
 *                     internal format is not one of the valid sized internal formats listed in
 *                     the table above.
 *
 *                     Check that INVALID_OPERATION is generated by ClearNamedBufferData if
 *                     any part of the specified range of the buffer object is mapped with
 *                     MapBufferRange or MapBuffer, unless it was mapped with the
 *                     MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.
 *
 *                     Check that INVALID_VALUE is generated by ClearNamedBufferData if
 *                     format is not a valid format, or type is not a valid type.
 *
 *
 *                     Check that INVALID_OPERATION is generated by ClearNamedBufferSubData
 *                     if buffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_ENUM is generated by ClearNamedBufferSubData if
 *                     internal format is not one of the valid sized internal formats listed in
 *                     the table above.
 *
 *                     Check that INVALID_VALUE is generated by ClearNamedBufferSubData if
 *                     offset or range are not multiples of the number of basic machine units
 *                     per-element for the internal format specified by internal format. This
 *                     value may be computed by multiplying the number of components for
 *                     internal format from the table by the size of the base type from the
 *                     specification table.
 *
 *                     Check that INVALID_VALUE is generated by ClearNamedBufferSubData if
 *                     offset or size is negative, or if offset+size is greater than the value
 *                     of BUFFER_SIZE for the buffer object.
 *
 *                     Check that INVALID_OPERATION is generated by ClearNamedBufferSubData
 *                     if any part of the specified range of the buffer object is mapped with
 *                     MapBufferRange or MapBuffer, unless it was mapped with the
 *                     MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.
 *
 *                     Check that INVALID_VALUE is generated by ClearNamedBufferSubData if
 *                     format is not a valid format, or type is not a valid type.
 *
 *
 *                     Check that INVALID_OPERATION is generated by CopyNamedBufferSubData if
 *                     readBuffer or writeBuffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_VALUE is generated by CopyNamedBufferSubData if any of
 *                     readOffset, writeOffset or size is negative, if readOffset+size is
 *                     greater than the size of the source buffer object (its value of
 *                     BUFFER_SIZE), or if writeOffset+size is greater than the size of the
 *                     destination buffer object.
 *
 *                     Check that INVALID_VALUE is generated by CopyNamedBufferSubData if the
 *                     source and destination are the same buffer object, and the ranges
 *                     [readOffset,readOffset+size) and [writeOffset,writeOffset+size) overlap.
 *
 *                     Check that INVALID_OPERATION is generated by CopyNamedBufferSubData if
 *                     either the source or destination buffer object is mapped with
 *                     MapBufferRange or MapBuffer, unless they were mapped with the
 *                     MAP_PERSISTENT bit set in the MapBufferRange access flags.
 *
 *
 *                     Check that INVALID_OPERATION is generated by MapNamedBuffer if buffer is
 *                     not the name of an existing buffer object.
 *
 *                     Check that INVALID_ENUM is generated by MapNamedBuffer if access is not
 *                     READ_ONLY, WRITE_ONLY, or READ_WRITE.
 *
 *                     Check that INVALID_OPERATION is generated by MapNamedBuffer if the
 *                     buffer object is in a mapped state.
 *
 *
 *                     Check that INVALID_OPERATION is generated by MapNamedBufferRange if
 *                     buffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_VALUE is generated by MapNamedBufferRange if offset
 *                     or length is negative, if offset+length is greater than the value of
 *                     BUFFER_SIZE for the buffer object, or if access has any bits set other
 *                     than those defined above.
 *
 *                     Check that INVALID_OPERATION is generated by MapNamedBufferRange for any
 *                     of the following conditions:
 *                      -  length is zero.
 *                      -  The buffer object is already in a mapped state.
 *                      -  Neither MAP_READ_BIT nor MAP_WRITE_BIT is set.
 *                      -  MAP_READ_BIT is set and any of MAP_INVALIDATE_RANGE_BIT,
 *                         MAP_INVALIDATE_BUFFER_BIT or MAP_UNSYNCHRONIZED_BIT is set.
 *                      -  MAP_FLUSH_EXPLICIT_BIT is set and MAP_WRITE_BIT is not set.
 *                      -  Any of MAP_READ_BIT, MAP_WRITE_BIT, MAP_PERSISTENT_BIT, or
 *                         MAP_COHERENT_BIT are set, but the same bit is not included in the
 *                         buffer's storage flags.
 *
 *
 *                     Check that INVALID_OPERATION is generated by UnmapNamedBuffer if buffer
 *                     is not the name of an existing buffer object.
 *
 *                     Check that INVALID_OPERATION is generated by UnmapNamedBuffer if the
 *                     buffer object is not in a mapped state.
 *
 *
 *                     Check that INVALID_OPERATION is generated by FlushMappedNamedBufferRange
 *                     if buffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_VALUE is generated by FlushMappedNamedBufferRange if
 *                     offset or length is negative, or if offset + length exceeds the size of
 *                     the mapping.
 *
 *                     Check that INVALID_OPERATION is generated by FlushMappedNamedBufferRange
 *                     if the buffer object is not mapped, or is mapped without the
 *                     MAP_FLUSH_EXPLICIT_BIT flag.
 *
 *
 *                     Check that INVALID_OPERATION is generated by GetNamedBufferParameter* if
 *                     buffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_ENUM is generated by GetNamedBufferParameter* if
 *                     pname is not one of the buffer object parameter names: BUFFER_ACCESS,
 *                     BUFFER_ACCESS_FLAGS, BUFFER_IMMUTABLE_STORAGE, BUFFER_MAPPED,
 *                     BUFFER_MAP_LENGTH, BUFFER_MAP_OFFSET, BUFFER_SIZE, BUFFER_STORAGE_FLAGS,
 *                     BUFFER_USAGE.
 *
 *
 *                     Check that INVALID_OPERATION is generated by GetNamedBufferPointerv
 *                     if buffer is not the name of an existing buffer object.
 *
 *
 *                     Check that INVALID_OPERATION is generated by GetNamedBufferSubData if
 *                     buffer is not the name of an existing buffer object.
 *
 *                     Check that INVALID_VALUE is generated by GetNamedBufferSubData if offset
 *                     or size is negative, or if offset+size is greater than the value of
 *                     BUFFER_SIZE for the buffer object.
 *
 *                     Check that INVALID_OPERATION is generated by GetNamedBufferSubData if
 *                     the buffer object is mapped with MapBufferRange or MapBuffer, unless it
 *                     was mapped with the MAP_PERSISTENT_BIT bit set in the MapBufferRange
 *                     access flags.
 */
class ErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	ErrorsTest(const ErrorsTest& other);
	ErrorsTest& operator=(const ErrorsTest& other);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLCLEARNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLenum internalformat,
														  glw::GLenum format, glw::GLenum type,
														  const glw::GLvoid* data);
	typedef void(GLW_APIENTRY* PFNGLCLEARNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLenum internalformat,
															 glw::GLintptr offset, glw::GLsizei size,
															 glw::GLenum format, glw::GLenum type,
															 const glw::GLvoid* data);
	typedef void(GLW_APIENTRY* PFNGLCOPYNAMEDBUFFERSUBDATA)(glw::GLuint readBuffer, glw::GLuint writeBuffer,
															glw::GLintptr readOffset, glw::GLintptr writeOffset,
															glw::GLsizeiptr size);
	typedef void(GLW_APIENTRY* PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE)(glw::GLuint buffer, glw::GLintptr offset,
																 glw::GLsizei length);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERI64V)(glw::GLuint buffer, glw::GLenum pname,
																 glw::GLint64* data);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERIV)(glw::GLuint buffer, glw::GLenum pname, glw::GLint* data);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPOINTERV)(glw::GLuint buffer, glw::GLenum pname,
															glw::GLvoid** params);
	typedef void*(GLW_APIENTRY* PFNGLGETNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei size,
															glw::GLvoid* data);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFER)(glw::GLuint buffer, glw::GLenum access);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFERRANGE)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei length,
														  glw::GLbitfield access);
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizeiptr size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERSTORAGE)(glw::GLuint buffer, glw::GLsizeiptr size,
														const glw::GLvoid* data, glw::GLbitfield flags);
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei size,
														const glw::GLvoid* data);
	typedef glw::GLboolean(GLW_APIENTRY* PFNGLUNMAPNAMEDBUFFER)(glw::GLuint buffer);

	PFNGLCLEARNAMEDBUFFERDATA		 m_pClearNamedBufferData;
	PFNGLCLEARNAMEDBUFFERSUBDATA	 m_pClearNamedBufferSubData;
	PFNGLCOPYNAMEDBUFFERSUBDATA		 m_pCopyNamedBufferSubData;
	PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE m_pFlushMappedNamedBufferRange;
	PFNGLGETNAMEDBUFFERPARAMETERI64V m_pGetNamedBufferParameteri64v;
	PFNGLGETNAMEDBUFFERPARAMETERIV   m_pGetNamedBufferParameteriv;
	PFNGLGETNAMEDBUFFERPOINTERV		 m_pGetNamedBufferPointerv;
	PFNGLGETNAMEDBUFFERSUBDATA		 m_pGetNamedBufferSubData;
	PFNGLMAPNAMEDBUFFER				 m_pMapNamedBuffer;
	PFNGLMAPNAMEDBUFFERRANGE		 m_pMapNamedBufferRange;
	PFNGLNAMEDBUFFERDATA			 m_pNamedBufferData;
	PFNGLNAMEDBUFFERSTORAGE			 m_pNamedBufferStorage;
	PFNGLNAMEDBUFFERSUBDATA			 m_pNamedBufferSubData;
	PFNGLUNMAPNAMEDBUFFER			 m_pUnmapNamedBuffer;

	/* Private member functions */
	bool TestErrorsOfClearNamedBufferData();
	bool TestErrorsOfClearNamedBufferSubData();
	bool TestErrorsOfCopyNamedBufferSubData();
	bool TestErrorsOfCreateBuffers();
	bool TestErrorsOfFlushMappedNamedBufferRange();
	bool TestErrorsOfGetNamedBufferParameter();
	bool TestErrorsOfGetNamedBufferPointerv();
	bool TestErrorsOfGetNamedBufferSubData();
	bool TestErrorsOfMapNamedBuffer();
	bool TestErrorsOfMapNamedBufferRange();
	bool TestErrorsOfNamedBufferData();
	bool TestErrorsOfNamedBufferStorage();
	bool TestErrorsOfNamedBufferSubData();
	bool TestErrorsOfUnmapNamedBuffer();

	bool ErrorCheckAndLog(const glw::GLchar* function_name, const glw::GLenum expected_error,
						  const glw::GLchar* when_shall_be_generated);
};
/* ErrorsTest class */

/** @class FunctionalTest
 *
 *  @brief Direct State Access Buffer Objects Functional Test.
 *
 *         This test verifies basic usage in rendering pipeline of the tested
 *         functions:
 *         -  ClearNamedBufferData,
 *         -  ClearNamedBufferSubData,
 *         -  CopyNamedBufferSubData,
 *         -  FlushMappedNamedBufferRange,
 *         -  GetNamedBufferParameteri64v,
 *         -  GetNamedBufferParameteriv,
 *         -  GetNamedBufferPointerv,
 *         -  GetNamedBufferSubData,
 *         -  MapNamedBuffer,
 *         -  MapNamedBufferRange,
 *         -  NamedBufferData,
 *         -  NamedBufferStorage,
 *         -  NamedBufferSubData and
 *         -  UnmapNamedBuffer.
 *
 *         Test follows the steps:
 *
 *             Prepare program with vertex shader and fragment shader. Fragment shader
 *             shall be pass-trough. Vertex shader shall have one integer input
 *             variable. Vertex shader shall output (to transform feedback) square of
 *             input variable. gl_Position shall be set up to vec4(0.0, 0.0, 0.0, 1.0).
 *             Build and use the program.
 *
 *             Create and bind empty vertex array object.
 *
 *             Prepare one buffer using CreateBuffers and NamedBufferStorage with size
 *             of 6 integers and passing pointer to data {1, 2, 3, 4, 5, 5} and dynamic
 *             storage flag set on. Clear (with 0) the first element with
 *             ClearNamedBufferSubData. Set second data element to 1 using
 *             NamedBufferSubData. Set third data element to 2 using MapNamedBuffer
 *             and UnmapNamedBuffer. Bind it to ARRAY_BUFFER binding point. Copy forth
 *             element into fifth element using CopyNamedBufferSubData. Set fourth data
 *             element using MapNamedBufferRange and FlushMappedNamedBuffer to 3.
 *             During mapping check that GetNamedBufferPointerv called with
 *             BUFFER_MAP_POINTER returns proper pointer. Unmap it using
 *             UnmapNamedBuffer function. Setup buffer as an input attribute to GLSL
 *             program. Consequently, the resulting buffer shall be {0, 1, 2, 3, 4, 5}.
 *
 *             Prepare one buffer using GenBuffers. Bind it to transform feedback.
 *             Allocate it's storage using NamedBufferData with size of 7 integers and
 *             passing pointer to data {0, 0, 0, 0, 0, 0, 36}. Set up this buffer as
 *             transform feedback output.
 *
 *             Begin transform feedback.
 *
 *             Draw six indices using points.
 *
 *             End transform feedback.
 *
 *             Query array buffer's parameter BUFFER_IMMUTABLE_STORAGE with
 *             GetNamedBufferParameteriv and compare with previous setup.
 *
 *             Query transform feedback buffer size with GetNamedBufferParameteri64v
 *             and compare with previous setup.
 *
 *             Fetch transform feedback buffer content using GetNamedBufferSubData and
 *             queried size.
 *
 *             Check that fetched data is equal to {0, 1, 4, 9, 16, 25, 36}.
 *
 *             If any check fails, test shall fail.
 *
 *             If any of the tested functions generates error, test shall fail.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions */
	FunctionalTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	FunctionalTest(const FunctionalTest& other);
	FunctionalTest& operator=(const FunctionalTest& other);

	/* API entry points. */
	typedef void(GLW_APIENTRY* PFNGLCLEARNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLenum internalformat,
														  glw::GLenum format, glw::GLenum type,
														  const glw::GLvoid* data);
	typedef void(GLW_APIENTRY* PFNGLCLEARNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLenum internalformat,
															 glw::GLintptr offset, glw::GLsizei size,
															 glw::GLenum format, glw::GLenum type,
															 const glw::GLvoid* data);
	typedef void(GLW_APIENTRY* PFNGLCOPYNAMEDBUFFERSUBDATA)(glw::GLuint readBuffer, glw::GLuint writeBuffer,
															glw::GLintptr readOffset, glw::GLintptr writeOffset,
															glw::GLsizeiptr size);
	typedef void(GLW_APIENTRY* PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE)(glw::GLuint buffer, glw::GLintptr offset,
																 glw::GLsizei length);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERI64V)(glw::GLuint buffer, glw::GLenum pname,
																 glw::GLint64* data);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERIV)(glw::GLuint buffer, glw::GLenum pname, glw::GLint* data);
	typedef void(GLW_APIENTRY* PFNGLGETNAMEDBUFFERPOINTERV)(glw::GLuint buffer, glw::GLenum pname,
															glw::GLvoid** params);
	typedef void*(GLW_APIENTRY* PFNGLGETNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei size,
															glw::GLvoid* data);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFER)(glw::GLuint buffer, glw::GLenum access);
	typedef void*(GLW_APIENTRY* PFNGLMAPNAMEDBUFFERRANGE)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei length,
														  glw::GLbitfield access);
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERDATA)(glw::GLuint buffer, glw::GLsizeiptr size, const glw::GLvoid* data,
													 glw::GLenum usage);
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERSTORAGE)(glw::GLuint buffer, glw::GLsizeiptr size,
														const glw::GLvoid* data, glw::GLbitfield flags);
	typedef void(GLW_APIENTRY* PFNGLNAMEDBUFFERSUBDATA)(glw::GLuint buffer, glw::GLintptr offset, glw::GLsizei size,
														const glw::GLvoid* data);
	typedef glw::GLboolean(GLW_APIENTRY* PFNGLUNMAPNAMEDBUFFER)(glw::GLuint buffer);

	PFNGLCLEARNAMEDBUFFERDATA		 m_pClearNamedBufferData;
	PFNGLCLEARNAMEDBUFFERSUBDATA	 m_pClearNamedBufferSubData;
	PFNGLCOPYNAMEDBUFFERSUBDATA		 m_pCopyNamedBufferSubData;
	PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE m_pFlushMappedNamedBufferRange;
	PFNGLGETNAMEDBUFFERPARAMETERI64V m_pGetNamedBufferParameteri64v;
	PFNGLGETNAMEDBUFFERPARAMETERIV   m_pGetNamedBufferParameteriv;
	PFNGLGETNAMEDBUFFERPOINTERV		 m_pGetNamedBufferPointerv;
	PFNGLGETNAMEDBUFFERSUBDATA		 m_pGetNamedBufferSubData;
	PFNGLMAPNAMEDBUFFER				 m_pMapNamedBuffer;
	PFNGLMAPNAMEDBUFFERRANGE		 m_pMapNamedBufferRange;
	PFNGLNAMEDBUFFERDATA			 m_pNamedBufferData;
	PFNGLNAMEDBUFFERSTORAGE			 m_pNamedBufferStorage;
	PFNGLNAMEDBUFFERSUBDATA			 m_pNamedBufferSubData;
	PFNGLUNMAPNAMEDBUFFER			 m_pUnmapNamedBuffer;

	/* Private member variables. */
	glw::GLuint m_po;
	glw::GLuint m_vao;
	glw::GLuint m_bo_in;
	glw::GLuint m_bo_out;
	glw::GLint  m_attrib_location;

	/* Private static variables. */
	static const glw::GLchar  s_vertex_shader[];
	static const glw::GLchar  s_fragment_shader[];
	static const glw::GLchar  s_vertex_shader_input_name[];
	static const glw::GLchar* s_vertex_shader_output_name;

	static const glw::GLint s_initial_data_a[];
	static const glw::GLint s_initial_data_b[];
	static const glw::GLint s_expected_data[];

	/* Private member functions */
	void BuildProgram();
	void PrepareVertexArrayObject();
	bool PrepareInputBuffer();
	bool PrepareOutputBuffer();
	void Draw();
	bool CheckArrayBufferImmutableFlag();
	bool CheckTransformFeedbackBufferSize();
	bool CheckTransformFeedbackResult();
	void Cleanup();
};
/* FunctionalTest class */
} /* Buffers namespace */

namespace Framebuffers
{
/** Framebuffer Creation
 *
 *      Create at least two framebuffer objects using GenFramebuffers function.
 *      Check them without binding, using IsFramebuffer function. Expect FALSE.
 *
 *      Create at least two framebuffer objects using CreateFramebuffers
 *      function. Check them without binding, using IsFramebuffer function.
 *      Expect TRUE.
 *
 *      Release objects.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

/** Framebuffer Renderbuffer Attachment
 *
 *      Create renderbuffer using GenRenderbuffers function. Bind it. Prepare
 *      its storage using RenderbufferStorage. Unbind it.
 *
 *      Create framebuffer using CreateFramebuffers. Setup renderbuffer storage.
 *      Attach the renderbuffer to the framebuffer using
 *      NamedFramebufferRenderbuffer function.
 *
 *      Bind framebuffer and check its status using CheckFramebufferStatus
 *      function call.
 *
 *      Clear the framebuffer's content with the reference value. Fetch the
 *      framebuffer's content using ReadPixels and compare it with reference
 *      values.
 *
 *      Repeat the test for following attachment types:
 *       -  COLOR_ATTACHMENTi for i from 0 to value of MAX_COLOR_ATTACHMENTS
 *          minus one,
 *       -  DEPTH_ATTACHMENT,
 *       -  STENCIL_ATTACHMENT and
 *       -  DEPTH_STENCIL_ATTACHMENT.
 */
class RenderbufferAttachmentTest : public deqp::TestCase
{
public:
	/* Public member functions */
	RenderbufferAttachmentTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	RenderbufferAttachmentTest(const RenderbufferAttachmentTest& other);
	RenderbufferAttachmentTest& operator=(const RenderbufferAttachmentTest& other);

	bool Test(glw::GLenum attachment, glw::GLenum internalformat);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
};
/* RenderbufferAttachmentTest class */

/** Named Framebuffer Texture Attachment
 *
 *      Create texture using CreateTexture function. Prepare its storage using
 *      TexStorage*D*.
 *
 *      Create framebuffer using CreateFramebuffers. Attach the texture to
 *      the framebuffer using NamedFramebufferTexture function. Repeat
 *      attachment for all texture levels.
 *
 *      Bind framebuffer and check its status using CheckFramebufferStatus
 *      function call.
 *
 *      Clear the framebuffer's content with the reference value. Fetch the
 *      framebuffer's content using ReadPixels and compare it with reference
 *      values.
 *
 *      Repeat the test for following attachment types:
 *       -  COLOR_ATTACHMENTi for i from 0 to value of MAX_COLOR_ATTACHMENTS
 *          minus one,
 *       -  DEPTH_ATTACHMENT,
 *       -  STENCIL_ATTACHMENT and
 *       -  DEPTH_STENCIL_ATTACHMENT.
 *
 *      Repeat the test for following texture targets:
 *       -  TEXTURE_RECTANGLE,
 *       -  TEXTURE_2D,
 *       -  TEXTURE_2D_MULTISAMPLE,
 *       -  TEXTURE_CUBE_MAP.
 *
 *      Repeat the test with each possible texture level, that is:
 *       -  0 for TEXTURE_RECTANGLE and TEXTURE_2D_MULTISAMPLE targets;
 *       -  from zero to value one less than base 2 logarithm of the value of
 *          MAX_3D_TEXTURE_SIZE for TEXTURE_2D target
 *       -  from zero to value one less than base 2 logarithm of the value of
 *          MAX_CUBE_MAP_TEXTURE_SIZE for TEXTURE_CUBE_MAP target.
 */
class TextureAttachmentTest : public deqp::TestCase
{
public:
	/* Public member functions */
	TextureAttachmentTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	TextureAttachmentTest(const TextureAttachmentTest& other);
	TextureAttachmentTest& operator=(const TextureAttachmentTest& other);

	bool Test(glw::GLenum attachment, bool is_color_attachment, glw::GLenum texture_target, glw::GLenum internalformat,
			  glw::GLuint levels);
	bool SubTestAttachmentError(glw::GLenum attachment, glw::GLenum texture_target, glw::GLuint level,
								glw::GLuint levels);
	bool SubTestStatus(glw::GLenum attachment, glw::GLenum texture_target, glw::GLuint level, glw::GLuint levels);
	bool SubTestContent(glw::GLenum attachment, glw::GLenum texture_target, glw::GLenum internalformat,
						glw::GLuint level, glw::GLuint levels);
	glw::GLuint MaxTextureLevels(glw::GLenum texture_target);
	void Clear();
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_to;

	/* Static private variables. */
	static const glw::GLenum s_targets[];
	static const glw::GLuint s_targets_count;

	static const glw::GLfloat s_reference_color[4];
	static const glw::GLint   s_reference_color_integer[4];
	static const glw::GLfloat s_reference_depth;
	static const glw::GLint   s_reference_stencil;
};
/* TextureAttachmentTest class */

/** Named Framebuffer Texture Layer Attachment
 *
 *      Create texture using CreateTexture function. Prepare its storage using
 *      TexStorage*D*.
 *
 *      Create framebuffer using CreateFramebuffers. Attach the texture to the
 *      framebuffer using NamedFramebufferTextureLayer function.
 *
 *      Bind framebuffer and check its status using CheckFramebufferStatus
 *      function call.
 *
 *      For non multisample target, clean the framebuffer's content with the
 *      reference value. Fetch one pixel from framebuffer's content using
 *      ReadPixels and compare it with reference values.
 *
 *      Repeat the test for following attachment types:
 *       -  COLOR_ATTACHMENTi for i from 0 to value of MAX_COLOR_ATTACHMENTS
 *          minus one,
 *       -  DEPTH_ATTACHMENT,
 *       -  STENCIL_ATTACHMENT and
 *       -  DEPTH_STENCIL_ATTACHMENT.
 *
 *      Repeat the test for following texture targets:
 *       -  TEXTURE_2D_MULTISAMPLE_ARRAY,
 *       -  TEXTURE_2D_ARRAY,
 *       -  TEXTURE_CUBE_MAP_ARRAY,
 *       -  TEXTURE_3D.
 *
 *      Repeat the test for texture levels from zero to value one less than base
 *      2 logarithm of the value of MAX_3D_TEXTURE_SIZE.
 *
 *      Repeat with texture which has number of layers:
 *       -  1,
 *       -  256,
 *       -  value of MAX_CUBE_MAP_TEXTURE_SIZE for TEXTURE_CUBE_ARRAY or value
 *          of MAX_3D_TEXTURE_SIZE.
 *      Test only limited set of the layers of the above textures to reduce time
 *      complexity of the test.
 */
class TextureLayerAttachmentTest : public deqp::TestCase
{
public:
	/* Public member functions */
	TextureLayerAttachmentTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	TextureLayerAttachmentTest(const TextureLayerAttachmentTest& other);
	TextureLayerAttachmentTest& operator=(const TextureLayerAttachmentTest& other);

	bool Test(glw::GLenum attachment, bool is_color_attachment, glw::GLenum texture_target, glw::GLenum internalformat,
			  glw::GLuint levels, glw::GLint layers);
	bool SubTestAttachmentError(glw::GLenum attachment, glw::GLenum texture_target, glw::GLuint level, glw::GLint layer,
								glw::GLuint levels, glw::GLint layers);
	bool SubTestStatus(glw::GLenum attachment, glw::GLenum texture_target, glw::GLuint level, glw::GLint layer,
					   glw::GLuint levels, glw::GLint layers);
	bool SubTestContent(glw::GLenum attachment, glw::GLenum texture_target, glw::GLenum internalformat,
						glw::GLuint level, glw::GLint layer, glw::GLuint levels, glw::GLint layers);
	void		Clear();
	glw::GLuint MaxTextureLevels(glw::GLenum texture_target);
	glw::GLuint MaxTextureLayers(glw::GLenum texture_target);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_to;

	/* Static private variables. */
	static const glw::GLenum s_targets[];
	static const glw::GLuint s_targets_count;

	static const glw::GLfloat s_reference_color[4];
	static const glw::GLint   s_reference_color_integer[4];
	static const glw::GLfloat s_reference_depth;
	static const glw::GLint   s_reference_stencil;
};
/* TextureLayerAttachmentTest class */

/** Named Framebuffer Draw Read Buffer
 *
 *      Create named framebuffer with maximum number of color attachments (use
 *      named renderbuffer storage).
 *
 *      For each color attachment use NamedFramebufferDrawBuffer to set up it as
 *      a draw buffer. Clear it with unique color.
 *
 *      For each color attachment use NamedFramebufferReadBuffer to set up it as
 *      a read buffer. Fetch the pixel data and compare that it contains unique
 *      color with the attachment was cleared
 *
 *      Check that NamedFramebufferDrawBuffer and NamedFramebufferReadBuffer
 *      accept GL_NONE as mode without error.
 *
 *      Release all objects.
 */
class DrawReadBufferTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DrawReadBufferTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DrawReadBufferTest(const DrawReadBufferTest& other);
	DrawReadBufferTest& operator=(const DrawReadBufferTest& other);
};
/* DrawReadBufferTest class */

/** Named Framebuffer Draw Buffers
 *
 *      Create named framebuffer with maximum number of color attachments (use
 *      named renderbuffer storage).
 *
 *      Set up all attachments as a draw buffer using the function
 *      NamedFramebufferDrawBuffers. Then clear them at once with unique color.
 *
 *      For each color attachment fetch pixel data and compare that contain
 *      the same unique color.
 *
 *      Release all objects.
 */
class DrawBuffersTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DrawBuffersTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DrawBuffersTest(const DrawBuffersTest& other);
	DrawBuffersTest& operator=(const DrawBuffersTest& other);

	/* Private static constants. */
	static const glw::GLfloat s_rgba[4];
};
/* DrawReadBuffersTest class */

/** Invalidate Named Framebuffer Data
 *
 *      For default framebuffer try to invalidate each of COLOR, DEPTH, and
 *      STENCIL attachments. Expect no error.
 *
 *      For default framebuffer try to invalidate all (COLOR, DEPTH, and
 *      STENCIL) attachments. Expect no error.
 *
 *      Create named framebuffer with maximum number of color attachments (use
 *      named renderbuffer storage), depth attachment and stencil attachment.
 *
 *      Clear all attachments.
 *
 *      Try to invalidate content of all attachments using
 *      InvalidateNamedFramebufferData. Expect no error.
 *
 *      Try to invalidate content of each attachment using
 *      InvalidateNamedFramebufferData. Expect no error.
 *
 *      Release all objects.
 */
class InvalidateDataTest : public deqp::TestCase
{
public:
	/* Public member functions */
	InvalidateDataTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	InvalidateDataTest(const InvalidateDataTest& other);
	InvalidateDataTest& operator=(const InvalidateDataTest& other);

	bool CheckErrorAndLog(const glw::GLenum attachment);
	bool CheckErrorAndLog(const glw::GLenum attachments[], const glw::GLuint count);
};
/* InvalidateDataTest class */

/** Invalidate Named Framebuffer SubData
 *
 *      For default framebuffer try to invalidate part of each of COLOR, DEPTH,
 *      and STENCIL attachments. Expect no error.
 *
 *      For default framebuffer try to invalidate part of all (COLOR, DEPTH, and
 *      STENCIL) attachments. Expect no error.
 *
 *      Create named framebuffer with maximum number of color attachments (use
 *      named renderbuffer storage), depth attachment and stencil attachment.
 *
 *      Clear all attachments.
 *
 *      Try to invalidate content of part of all attachments using
 *      InvalidateNamedFramebufferData. Expect no error.
 *
 *      Try to invalidate content of part of each attachment using
 *      InvalidateNamedFramebufferData. Expect no error.
 *
 *      Release all objects.
 */
class InvalidateSubDataTest : public deqp::TestCase
{
public:
	/* Public member functions */
	InvalidateSubDataTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	InvalidateSubDataTest(const InvalidateSubDataTest& other);
	InvalidateSubDataTest& operator=(const InvalidateSubDataTest& other);

	bool CheckErrorAndLog(const glw::GLenum attachment);
	bool CheckErrorAndLog(const glw::GLenum attachments[], const glw::GLuint count);
};
/* InvalidateSubDataTest class */

/** Clear Named Framebuffer
 *
 *      Repeat following steps for fixed-point, floating-point, signed integer,
 *      and unsigned integer color attachments.
 *
 *          Create named framebuffer with maximum number of color attachments
 *          (use named renderbuffer storage).
 *
 *          Clear each of the color attachment with unique color using proper
 *          ClearNamedFramebuffer* function.
 *
 *          For each color attachment fetch pixel data and compare that contain
 *          unique color with which it was cleared.
 *
 *          Release all objects.
 *
 *      Next, do following steps:
 *
 *          Create named framebuffer with depth attachment and stencil
 *          attachment.
 *
 *          Clear each of the attachments with unique value using proper
 *          ClearNamedFramebufferfi function.
 *
 *          Fetch pixel data of each attachment and compare that contain unique
 *          value with which it was cleared.
 *
 *          Release all objects.
 */
class ClearTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ClearTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	ClearTest(const ClearTest& other);
	ClearTest& operator=(const ClearTest& other);

	void PrepareFramebuffer(glw::GLenum buffer, glw::GLenum internalformat);

	template <typename T>
	bool TestClearColor(glw::GLenum buffer, glw::GLenum attachment, T value);

	template <typename T>
	bool ClearColor(glw::GLenum buffer, glw::GLint drawbuffer, T value);

	bool TestClearDepthAndStencil(glw::GLfloat depth, glw::GLint stencil);

	template <typename T>
	glw::GLenum Format();

	template <typename T>
	glw::GLenum Type();

	template <typename T>
	bool Compare(const T first, const T second);

	void Clean();

	/* Private member variables. */
	glw::GLuint  m_fbo;
	glw::GLuint* m_renderbuffers;
	glw::GLuint  m_renderbuffers_count;
};
/* ClearTest class */

/** Blit Named Framebuffer
 *
 *      Create named framebuffer with color, depth and stencil attachments with
 *      size 2x2 pixels(use named renderbuffer storage).
 *
 *      Create named framebuffer with color, depth and stencil attachments with
 *      size 2x3 pixels(use named renderbuffer storage).
 *
 *      Clear the first framebuffer with red color, 0.5 depth and 1 as a stencil
 *      index.
 *
 *      Blit one pixel of the first framebuffer to the second framebuffer to the
 *      pixel at (0, 0) position with NEAREST filter.
 *
 *      Clear first the framebuffer with green color, 0.25 depth and 2 as a
 *      stencil index.
 *
 *      Blit one pixel of the first framebuffer to the second framebuffer to the
 *      pixel at (1, 0) position with LINEAR filter for color attachment, but
 *      NEAREST filter for depth and stencil attachments.
 *
 *      Clear the first framebuffer with blue color, 0.125 depth and 3 as a
 *      stencil index.
 *
 *      Blit the whole first framebuffer to the second framebuffer by shrinking
 *      it to the single pixel at (2, 0) position.
 *
 *      Clear the first framebuffer with yellow color, 0.0625 depth and 4 as a
 *      stencil index.
 *
 *      Blit one pixel of the framebuffer to the second framebuffer by expanding
 *      it to the three pixel constructing horizontal line at (0, 1) position.
 *
 *      Expect no error.
 *
 *      Check that color attachment of the second framebuffer has following
 *      values:
 *          red,    green,  blue,
 *          yellow, yellow, yellow.
 *
 *      Check that depth attachment of the second framebuffer has following
 *      values:
 *          0.5,    0.25,   0.125
 *          0.0625, 0.0625, 0.0625.
 *
 *      Check that stencil attachment of the second framebuffer has following
 *      values:
 *          1,  2,  3
 *          4,  4,  4.
 *
 *      Release all objects.
 */
class BlitTest : public deqp::TestCase
{
public:
	/* Public member functions */
	BlitTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	BlitTest(const BlitTest& other);
	BlitTest& operator=(const BlitTest& other);

	void PrepareFramebuffers();
	bool Test();
	void ClearFramebuffer(glw::GLfloat red, glw::GLfloat green, glw::GLfloat blue, glw::GLfloat depth,
						  glw::GLint stencil);
	bool CheckErrorAndLog();
	bool CheckColor();
	bool CheckDepth();
	bool CheckStencil();
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_src;
	glw::GLuint m_rbo_color_src;
	glw::GLuint m_rbo_depth_stencil_src;
	glw::GLuint m_fbo_dst;
	glw::GLuint m_rbo_color_dst;
	glw::GLuint m_rbo_depth_stencil_dst;
};
/* BlitTest class */

/** Check Named Framebuffer Status
 *
 *      Do following test cases:
 *
 *          Incomplete attachment case
 *
 *              Prepare framebuffer with one incomplete attachment.
 *
 *              Check the framebuffer status using CheckNamedFramebufferStatus.
 *              Expect FRAMEBUFFER_INCOMPLETE_ATTACHMENT return value.
 *
 *              Release all objects.
 *
 *              Repeat the test case for all possible color, depth and stencil
 *              attachments.
 *
 *          Missing attachment case
 *
 *              Prepare framebuffer without any attachment.
 *
 *              Check the framebuffer status using CheckNamedFramebufferStatus.
 *              Expect FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT return value.
 *
 *              Release all objects.
 *
 *          Incomplete multisample renderbuffer case
 *
 *              Prepare framebuffer with two multisampled renderbuffer color
 *              attachments which have different number of samples.
 *
 *              Check the framebuffer status using CheckNamedFramebufferStatus.
 *              Expect FRAMEBUFFER_INCOMPLETE_MULTISAMPLE return value. If the
 *              check fails return TEST_RESULT_COMPATIBILITY_WARNING.
 *
 *              Release all objects.
 *
 *          Incomplete multisample texture case
 *
 *              Prepare framebuffer with two multisampled texture color
 *              attachments and one multisampled renderbuffer which all have
 *              different number of sample locations. One of the textures shall
 *              have fixed sample locations set, one not.
 *
 *              Check the framebuffer status using CheckNamedFramebufferStatus.
 *              Expect FRAMEBUFFER_INCOMPLETE_MULTISAMPLE return value. If the
 *              check fails return TEST_RESULT_COMPATIBILITY_WARNING.
 *
 *              Release all objects.
 *
 *          Incomplete layer targets case
 *
 *              Prepare framebuffer with one 3D texture and one 2D texture.
 *
 *              Check the framebuffer status using CheckNamedFramebufferStatus.
 *              Expect FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS return value. If the
 *              check fails return TEST_RESULT_COMPATIBILITY_WARNING.
 *
 *              Release all objects.
 *
 *      Note
 *
 *      The specification is not clear about framebuffer completeness. The
 *      OpenGL 4.5 Core Profile Specification in chapter 9.4.2 says:
 *
 *          "The framebuffer object bound to target is said to be framebuffer
 *          complete if all the following conditions are true [...]"
 *
 *      It does not say that framebuffer is incomplete when any of the
 *      conditions are not met. Due to this wording, except for obvious cases
 *      (incomplete attachment and missing attachments) other tests are optional
 *      and may result in QP_TEST_RESULT_COMPATIBILITY_WARNING when fail.
 */
class CheckStatusTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CheckStatusTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CheckStatusTest(const CheckStatusTest& other);
	CheckStatusTest& operator=(const CheckStatusTest& other);

	bool IncompleteAttachmentTestCase();
	bool MissingAttachmentTestCase();
	bool IncompleteMultisampleRenderbufferTestCase();
	bool IncompleteMultisampleTextureTestCase();
	bool IncompleteLayerTargetsTestCase();
};
/* CheckStatusTest class */

/** Get Named Framebuffer Parameters
 *
 *      Prepare framebuffer with read and write buffers and renderbuffer color
 *      attachment.
 *
 *      Do following checks for the created framebuffer:
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          FRAMEBUFFER_DEFAULT_WIDTH returns the value of
 *          FRAMEBUFFER_DEFAULT_WIDTH for the framebuffer object.
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          FRAMEBUFFER_DEFAULT_HEIGHT returns the value of
 *          FRAMEBUFFER_DEFAULT_HEIGHT for the framebuffer object.
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          FRAMEBUFFER_DEFAULT_LAYERS returns the value of
 *          FRAMEBUFFER_DEFAULT_LAYERS for the framebuffer object.
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          FRAMEBUFFER_DEFAULT_SAMPLES returns the value of
 *          FRAMEBUFFER_DEFAULT_SAMPLES for the framebuffer object.
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS returns the boolean value
 *          of FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS.
 *
 *      Do following checks for the created and default (if available)
 *      framebuffer:
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          DOUBLEBUFFER returns a boolean value indicating whether double
 *          buffering is supported for the framebuffer object.
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          IMPLEMENTATION_COLOR_READ_FORMAT returns a GLenum value indicating
 *          the preferred pixel data format for the framebuffer object.
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          IMPLEMENTATION_COLOR_READ_TYPE returns a GLenum value indicating the
 *          implementation's preferred pixel data type for the framebuffer
 *          object.
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          SAMPLES returns an integer value indicating the coverage mask size
 *          for the framebuffer object.
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          SAMPLE_BUFFERS returns an integer value indicating the number of
 *          sample buffers associated with the framebuffer object.
 *
 *          Check that GetNamedFramebufferParameteriv called with parameter name
 *          STEREO returns a boolean value indicating whether stereo buffers
 *          (left and right) are supported for the framebuffer object.
 *
 *      Release all objects.
 */
class GetParametersTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetParametersTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	GetParametersTest(const GetParametersTest& other);
	GetParametersTest& operator=(const GetParametersTest& other);

	void PrepareFramebuffer();
	bool TestDefaultFramebuffer();
	bool TestCustomFramebuffer();
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
};
/* GetParametersTest class */

/** Get Named Framebuffer Attachment Parameters
 *
 *      For default framebuffer, for all attachments:
 *          FRONT_LEFT,
 *          FRONT_RIGHT,
 *          BACK_LEFT,
 *          BACK_RIGHT,
 *          DEPTH,
 *          STENCIL
 *      query FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE using
 *      GetNamedFramebufferAttachmentParameteriv function. Expect
 *      FRAMEBUFFER_DEFAULT return value (as queried with non-DSA way).
 *
 *      For any attachments not equal to GL_NONE do following queries using
 *      GetNamedFramebufferAttachmentParameteriv function:
 *          FRAMEBUFFER_ATTACHMENT_RED_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
 *          FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING.
 *      Expect value equal to framebuffers setup (as queried with non-DSA way).
 *      Expect no error.
 *
 *      Create 3 framebuffer objects with renderbuffer color attachment, and
 *      depth or stencil or depth-stencil attachments.
 *
 *      For each of framebuffers, for each of following attachments:
 *          DEPTH_ATTACHMENT,
 *          STENCIL_ATTACHMENT,
 *          DEPTH_STENCIL_ATTACHMENT,
 *          COLOR_ATTACHMENT0,
 *          COLOR_ATTACHMENT1
 *      query FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE using
 *      GetNamedFramebufferAttachmentParameteriv function. Expect the same
 *      return value as queried with non-DSA way.
 *
 *      For each of framebuffers, for any attachments not equal to GL_NONE do
 *      following queries using GetNamedFramebufferAttachmentParameteriv
 *      function:
 *          FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
 *          FRAMEBUFFER_ATTACHMENT_RED_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
 *          FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING
 *      Expect value equal to framebuffers setup (as queried with non-DSA way).
 *      Expect no error.
 *
 *      Release objects.
 *
 *      Create 3 framebuffer objects with texture color attachment, and
 *      depth or stencil or depth-stencil attachments.
 *
 *      For each of framebuffers, for each of following attachments:
 *          DEPTH_ATTACHMENT,
 *          STENCIL_ATTACHMENT,
 *          DEPTH_STENCIL_ATTACHMENT,
 *          COLOR_ATTACHMENT0,
 *          COLOR_ATTACHMENT1
 *      query FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE using
 *      GetNamedFramebufferAttachmentParameteriv function. Expect the same
 *      return value as queried with non-DSA way.
 *
 *      For each of framebuffers, for any attachments not equal to GL_NONE do
 *      following queries using GetNamedFramebufferAttachmentParameteriv
 *      function:
 *          FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
 *          FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,
 *          FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,
 *          FRAMEBUFFER_ATTACHMENT_LAYERED,
 *          FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER,
 *          FRAMEBUFFER_ATTACHMENT_RED_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
 *          FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
 *          FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING
 *      Expect value equal to framebuffers setup (as queried with non-DSA way).
 *      Expect no error.
 *
 *      Release objects.
 *
 *      Additional conditions:
 *
 *          Do not query DEPTH_STENCIL_ATTACHMENT attachment if the renderbuffer
 *          or texture is not depth-stencil.
 *
 *          Do not query FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE pname with
 *          DEPTH_STENCIL_ATTACHMENT attachment.
 */
class GetAttachmentParametersTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetAttachmentParametersTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	GetAttachmentParametersTest(const GetAttachmentParametersTest& other);
	GetAttachmentParametersTest& operator=(const GetAttachmentParametersTest& other);

	void CreateRenderbufferFramebuffer(bool depth, bool stencil);
	void CreateTextureFramebuffer(bool depth, bool stencil);
	bool TestDefaultFramebuffer();
	bool TestRenderbufferFramebuffer(bool depth_stencil);
	bool TestTextureFramebuffer(bool depth_stencil);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo[2];
	glw::GLuint m_to[2];
};
/* GetParametersTest class */

/** Create Named Framebuffers Errors
 *
 *      Check that INVALID_VALUE is generated by CreateFramebuffers if n is
 *      negative.
 */
class CreationErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationErrorsTest(const CreationErrorsTest& other);
	CreationErrorsTest& operator=(const CreationErrorsTest& other);
};
/* CreationErrorsTest class */

/** Named Framebuffer Renderbuffer Errors
 *
 *      Check that INVALID_OPERATION is generated by
 *      NamedFramebufferRenderbuffer if framebuffer is not the name of an
 *      existing framebuffer object.
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferRenderbuffer
 *      if attachment is COLOR_ATTACHMENTm where m is greater than or equal to
 *      the value of MAX_COLOR_ATTACHMENTS.
 *
 *      Check that INVALID_ENUM is generated by NamedFramebufferRenderbuffer if
 *      attachment is not one of the attachments in table 9.2, and attachment is
 *      not COLOR_ATTACHMENTm where m is greater than or equal to the value of
 *      MAX_COLOR_ATTACHMENTS.
 *
 *      Check that INVALID_ENUM is generated by NamedFramebufferRenderbuffer if
 *      renderbuffer target is not RENDERBUFFER.
 *
 *      Check that INVALID_OPERATION is generated by
 *      NamedFramebufferRenderbuffer if renderbuffer target is not zero or the
 *      name of an existing renderbuffer object of type RENDERBUFFER.
 */
class RenderbufferAttachmentErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	RenderbufferAttachmentErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	RenderbufferAttachmentErrorsTest(const RenderbufferAttachmentErrorsTest& other);
	RenderbufferAttachmentErrorsTest& operator=(const RenderbufferAttachmentErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, bool framebuffer, bool attachment, bool color_attachment,
					 bool renderbuffertarget, bool renderbuffer);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_valid;
	glw::GLuint m_rbo_valid;
	glw::GLuint m_fbo_invalid;
	glw::GLuint m_rbo_invalid;
	glw::GLenum m_color_attachment_invalid;
	glw::GLenum m_attachment_invalid;
	glw::GLenum m_renderbuffer_target_invalid;
};
/* RenderbufferAttachmentErrorsTest class */

/** Named Framebuffer Texture Errors
 *
 *      Check that GL_INVALID_OPERATION is generated by glNamedFramebufferTexture and glNamedFramebufferTextureLayer
 *      if framebuffer is not the name of an existing framebuffer object.
 *
 *      Check that INVALID_OPERATION is generated by glNamedFramebufferTexture and glNamedFramebufferTextureLayer
 *      if attachment is COLOR_ATTACHMENTm where m is greater than or equal to
 *      the value of MAX_COLOR_ATTACHMENTS.
 *
 *      Check that INVALID_ENUM is generated by glNamedFramebufferTexture and glNamedFramebufferTextureLayer if
 *      attachment is not one of the attachments in table 9.2, and attachment is
 *      not COLOR_ATTACHMENTm where m is greater than or equal to the value of
 *      MAX_COLOR_ATTACHMENTS.
 *
 *      Check that INVALID_VALUE is generated by glNamedFramebufferTexture if texture is not zero or the name
 *      of an existing texture object.
 *
 *      Check that INVALID_OPERATION is generated by glNamedFramebufferTextureLayer if texture is not zero or
 *      the name of an existing texture object.
 *
 *      Check that INVALID_VALUE is generated by glNamedFramebufferTexture and glNamedFramebufferTextureLayer if
 *      texture is not zero and level is not a supported texture level for
 *      texture.
 *
 *      Check that INVALID_OPERATION is generated by glNamedFramebufferTexture and glNamedFramebufferTextureLayer
 *      if texture is a buffer texture.
 *
 *      Check that INVALID_VALUE error is generated by NamedFramebufferTextureLayer if texture is a three-dimensional
 *      texture, and layer is larger than the value of MAX_3D_TEXTURE_SIZE minus one.
 *
 *      Check that INVALID_VALUE error is generated by NamedFramebufferTextureLayer if texture is an array texture,
 *      and layer is larger than the value of MAX_ARRAY_TEXTURE_LAYERS minus one.
 *
 *      Check that INVALID_VALUE error is generated by NamedFramebufferTextureLayer if texture is a cube map array texture,
 *      and (layer / 6) is larger than the value of MAX_CUBE_MAP_TEXTURE_SIZE minus one (see section 9.8).
 *      Check that INVALID_VALUE error is generated by NamedFramebufferTextureLayer if texture is non-zero
 *      and layer is negative.
 *
 *      Check that INVALID_OPERATION error is generated by NamedFramebufferTextureLayer if texture is non-zero
 *      and is not the name of a three-dimensional, two-dimensional multisample array, one- or two-dimensional array,
 *      or cube map array texture.
 */
class TextureAttachmentErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	TextureAttachmentErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	TextureAttachmentErrorsTest(const TextureAttachmentErrorsTest& other);
	TextureAttachmentErrorsTest& operator=(const TextureAttachmentErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function_name, bool framebuffer, bool attachment,
					 bool color_attachment, bool texture, bool level, const glw::GLchar* texture_type, bool layer);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_valid;
	glw::GLuint m_to_valid;
	glw::GLuint m_to_3d_valid;
	glw::GLuint m_to_array_valid;
	glw::GLuint m_to_cubearray_valid;
	glw::GLuint m_tbo_valid;
	glw::GLuint m_fbo_invalid;
	glw::GLuint m_to_invalid;
	glw::GLuint m_to_layer_invalid; /* it is valid rectangle texture, but invalid for NamedFramebufferTextureLayer */
	glw::GLenum m_color_attachment_invalid;
	glw::GLenum m_attachment_invalid;
	glw::GLint  m_level_invalid;
	glw::GLint  m_max_3d_texture_size;
	glw::GLint  m_max_3d_texture_depth;
	glw::GLint  m_max_array_texture_layers;
	glw::GLint  m_max_cube_map_texture_size;
};
/* TextureAttachmentErrorsTest class */

/** Named Framebuffer Draw Read Buffers Errors
 *
 *      Check that INVALID_OPERATION error is generated by
 *      NamedFramebufferDrawBuffer if framebuffer is not zero or the name of an
 *      existing framebuffer object.
 *
 *      Check that INVALID_ENUM is generated by NamedFramebufferDrawBuffer if
 *      buf is not an accepted value.
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffer
 *      if the GL is bound to a draw framebuffer object and the ith argument is
 *      a value other than COLOR_ATTACHMENTi or NONE.
 *
 *      Check that INVALID_OPERATION error is generated by
 *      NamedFramebufferDrawBuffers if framebuffer is not zero or the name of an
 *      existing framebuffer object.
 *
 *      Check that INVALID_VALUE is generated by NamedFramebufferDrawBuffers if
 *      n is less than 0.
 *
 *      Check that INVALID_VALUE is generated by NamedFramebufferDrawBuffers if
 *      n is greater than MAX_DRAW_BUFFERS.
 *
 *      Check that INVALID_ENUM is generated by NamedFramebufferDrawBuffers if
 *      one of the values in bufs is not an accepted value.
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffers
 *      if a symbolic constant other than GL_NONE appears more than once in
 *      bufs.
 *
 *      Check that INVALID_ENUM error is generated by
 *      NamedFramebufferDrawBuffers if any value in bufs is FRONT, LEFT, RIGHT,
 *      or FRONT_AND_BACK. This restriction applies to both the default
 *      framebuffer and framebuffer objects, and exists because these constants
 *      may themselves refer to multiple buffers, as shown in table 17.4.
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffers
 *      if any value in bufs is BACK, and n is not one.
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffers
 *      if the API call refers to a framebuffer object and one or more of the
 *      values in bufs is anything other than NONE or one of the
 *      COLOR_ATTACHMENTn tokens.
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffers
 *      if the API call refers to the default framebuffer and one or more of the
 *      values in bufs is one of the COLOR_ATTACHMENTn tokens.
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferReadBuffer
 *      if framebuffer is not zero or the name of an existing framebuffer
 *      object.
 *
 *      Check that INVALID_ENUM is generated by NamedFramebufferReadBuffer if
 *      src is not one of the accepted values (tables 17.4 and 17.5 of OpenGL
 *      4.5 Core Profile Specification).
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferReadBuffer
 *      if the default framebuffer is affected and src is a value (other than
 *      NONE) that does not indicate any of the color buffers allocated to the
 *      default framebuffer.
 *
 *      Check that INVALID_OPERATION is generated by NamedFramebufferReadBuffer
 *      if a framebuffer object is affected, and src is one of the  constants
 *      from table 17.4 (other than NONE, or COLOR_ATTACHMENTm where m is
 *      greater than or equal to the value of MAX_COLOR_ATTACHMENTS).
 */
class DrawReadBuffersErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DrawReadBuffersErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	DrawReadBuffersErrorsTest(const DrawReadBuffersErrorsTest& other);
	DrawReadBuffersErrorsTest operator=(const DrawReadBuffersErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint  m_fbo_valid;
	glw::GLuint  m_fbo_invalid;
	glw::GLenum  m_attachment_color;
	glw::GLenum  m_attachment_back_left;
	glw::GLenum  m_attachment_right;
	glw::GLenum  m_attachment_left;
	glw::GLenum  m_attachment_front;
	glw::GLenum  m_attachment_front_and_back;
	glw::GLenum  m_attachment_back;
	glw::GLenum  m_attachment_invalid;
	glw::GLenum  m_attachments_invalid[2];
	glw::GLenum  m_attachments_back_invalid[2];
	glw::GLint   m_attachments_too_many_count;
	glw::GLenum* m_attachments_too_many;
	glw::GLint   m_max_color_attachments;
};
/* DrawReadBuffersErrorsTest class */

/** Invalidate Framebuffer Data and SubData Errors

 Check that INVALID_OPERATION error is generated by
 InvalidateNamedFramebufferData if framebuffer is not zero or the name of
 an existing framebuffer object.

 Check that INVALID_ENUM error is generated by
 InvalidateNamedFramebufferData if a framebuffer object is affected, and
 any element of of attachments is not one of the values
 {COLOR_ATTACHMENTi, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT,
 DEPTH_STENCIL_ATTACHMENT}.

 Check that INVALID_OPERATION error is generated by
 InvalidateNamedFramebufferData if attachments contains COLOR_ATTACHMENTm
 where m is greater than or equal to the value of MAX_COLOR_ATTACHMENTS.

 Check that INVALID_ENUM error is generated by
 InvalidateNamedFramebufferData if the default framebuffer is affected,
 and any elements of attachments are not one of:
 -  FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, and BACK_RIGHT, identifying that
 specific buffer,
 -  COLOR, which is treated as BACK_LEFT for a double-buffered context
 and FRONT_LEFT for a single-buffered context,
 -  DEPTH, identifying the depth buffer,
 -  STENCIL, identifying the stencil buffer.

 Check that INVALID_OPERATION error is generated by
 InvalidateNamedSubFramebuffer if framebuffer is not zero or the name of
 an existing framebuffer object.

 Check that INVALID_VALUE error is generated by
 InvalidateNamedSubFramebuffer if numAttachments, width, or height is
 negative.

 Check that INVALID_ENUM error is generated by
 InvalidateNamedSubFramebuffer if a framebuffer object is affected, and
 any element of attachments is not one of the values {COLOR_ATTACHMENTi,
 DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT}.

 Check that INVALID_OPERATION error is generated by
 InvalidateNamedSubFramebuffer if attachments contains COLOR_ATTACHMENTm
 where m is greater than or equal to the value of MAX_COLOR_ATTACHMENTS.

 Check that INVALID_ENUM error is generated by
 InvalidateNamedSubFramebuffer if the default framebuffer is affected,
 and any elements of attachments are not one of:
 -  FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, and BACK_RIGHT, identifying that
 specific buffer,
 -  COLOR, which is treated as BACK_LEFT for a double-buffered context
 and FRONT_LEFT for a single-buffered context,
 -  DEPTH, identifying the depth buffer,
 -  STENCIL, identifying the stencil buffer.
 */
class InvalidateDataAndSubDataErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	InvalidateDataAndSubDataErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	InvalidateDataAndSubDataErrorsTest(const InvalidateDataAndSubDataErrorsTest& other);
	InvalidateDataAndSubDataErrorsTest operator=(const InvalidateDataAndSubDataErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_valid;
	glw::GLuint m_rbo;
	glw::GLuint m_fbo_invalid;
	glw::GLenum m_fbo_attachment_valid;
	glw::GLenum m_fbo_attachment_invalid;
	glw::GLenum m_color_attachment_invalid;
	glw::GLenum m_default_attachment_invalid;
};
/* InvalidateDataAndSubDataErrorsTest class */

/** Clear Named Framebuffer Errors
 *
 *      Check that INVALID_OPERATION is generated by ClearNamedFramebuffer* if
 *      framebuffer is not zero or the name of an existing framebuffer object.
 *
 *      Check that INVALID_ENUM is generated by ClearNamedFramebufferiv buffer
 *      is not COLOR or STENCIL.
 *
 *      Check that INVALID_ENUM is generated by ClearNamedFramebufferuiv buffer
 *      is not COLOR.
 *
 *      Check that INVALID_ENUM is generated by ClearNamedFramebufferfv buffer
 *      is not COLOR or DEPTH.
 *
 *      Check that INVALID_ENUM is generated by ClearNamedFramebufferfi buffer
 *      is not DEPTH_STENCIL.
 *
 *      Check that INVALID_VALUE is generated if buffer is COLOR drawbuffer is
 *      negative, or greater than the value of MAX_DRAW_BUFFERS minus one.
 *
 *      Check that INVALID_VALUE is generated if buffer is DEPTH, STENCIL or
 *      DEPTH_STENCIL and drawbuffer is not zero.
 */
class ClearNamedFramebufferErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ClearNamedFramebufferErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	ClearNamedFramebufferErrorsTest(const ClearNamedFramebufferErrorsTest& other);
	ClearNamedFramebufferErrorsTest& operator=(const ClearNamedFramebufferErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_valid;
	glw::GLuint m_rbo_color;
	glw::GLuint m_rbo_depth_stencil;
	glw::GLuint m_fbo_invalid;
};
/* ClearNamedFramebufferErrorsTest class */

/** Check Named Framebuffer Status Errors
 *
 *      Check that INVALID_ENUM is generated by CheckNamedFramebufferStatus if
 *      target is not DRAW_FRAMEBUFFER, READ_FRAMEBUFFER or FRAMEBUFFER.
 *
 *      Check that INVALID_OPERATION is generated by CheckNamedFramebufferStatus
 *      if framebuffer is not zero or the name of an existing framebuffer
 *      object.
 */
class CheckStatusErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CheckStatusErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CheckStatusErrorsTest(const CheckStatusErrorsTest& other);
	CheckStatusErrorsTest& operator=(const CheckStatusErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_valid;
	glw::GLuint m_fbo_invalid;
	glw::GLuint m_target_invalid;
};
/* CheckStatusErrorsTest class */

/** Get Named Framebuffer Parameter Errors
 *
 *      Check that INVALID_OPERATION is generated by
 *      GetNamedFramebufferParameteriv if framebuffer is not zero or the name of
 *      an existing framebuffer object.
 *
 *      Check that INVALID_ENUM is generated by GetNamedFramebufferParameteriv
 *      if pname is not one of the accepted parameter names.
 *
 *      Check that INVALID_OPERATION is generated if a default framebuffer is
 *      queried, and pname is not one of DOUBLEBUFFER,
 *      IMPLEMENTATION_COLOR_READ_FORMAT, IMPLEMENTATION_COLOR_READ_TYPE,
 *      SAMPLES, SAMPLE_BUFFERS or STEREO.
 */
class GetParameterErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetParameterErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	GetParameterErrorsTest(const GetParameterErrorsTest& other);
	GetParameterErrorsTest& operator=(const GetParameterErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_valid;
	glw::GLuint m_fbo_invalid;
	glw::GLuint m_parameter_invalid;
};
/* GetParameterErrorsTest class */

/** Get Named Framebuffer Attachment Parameter Errors
 *
 *      Check that GL_INVALID_OPERATION is generated by
 *      GetNamedFramebufferAttachmentParameteriv if framebuffer is not zero or
 *      the name of an existing framebuffer object.
 *
 *      Check that INVALID_ENUM is generated by
 *      GetNamedFramebufferAttachmentParameteriv if pname is not valid for the
 *      value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, as described above.
 *
 *      Check that INVALID_ENUM error is generated if a framebuffer object is queried, attachment
 *      is not one of the attachments in table 9.2 (COLOR_ATTACHMENTi, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT), and attachment is not
 *      COLOR_ATTACHMENTm where m is greater than or equal to the value of MAX_COLOR_ATTACHMENTS.
 *
 *      Check that INVALID_OPERATION is generated by
 *      GetNamedFramebufferAttachmentParameteriv if the value of
 *      FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE and pname is not
 *      FRAMEBUFFER_ATTACHMENT_OBJECT_NAME or
 *      FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE.
 *
 *      Check that INVALID_OPERATION is generated by
 *      GetNamedFramebufferAttachmentParameteriv if attachment is
 *      DEPTH_STENCIL_ATTACHMENT and pname is
 *      FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE.
 *
 *      Check that an INVALID_ENUM error is generated if the default framebuffer is
 *      queried and attachment is not one the values specified in table 9.1.
 *
 *      Check that an INVALID_OPERATION error is generated if a framebuffer object is
 *      bound to target and attachment is COLOR_ATTACHMENTm where m is greater than or
 *      equal to the value of MAX_COLOR_ATTACHMENTS.
 *
 *      Check that an INVALID_ENUM error is generated if a framebuffer object is
 *      queried, attachment is not one of the attachments in table 9.2, and attachment
 *      is not COLOR_ATTACHMENTm where m is greater than or equal to the value of
 *      MAX_COLOR_ATTACHMENTS.
 */
class GetAttachmentParameterErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetAttachmentParameterErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	GetAttachmentParameterErrorsTest(const GetAttachmentParameterErrorsTest& other);
	GetAttachmentParameterErrorsTest& operator=(const GetAttachmentParameterErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_valid;
	glw::GLuint m_rbo_color;
	glw::GLuint m_rbo_depth_stencil;
	glw::GLuint m_fbo_invalid;
	glw::GLuint m_parameter_invalid;
	glw::GLenum m_attachment_invalid;
	glw::GLenum m_default_attachment_invalid;
	glw::GLint  m_max_color_attachments;
};
/* GetAttachmentParameterErrorsTest class */

/** Framebuffer and Renderbuffer Functional
 *
 *      Create two framebuffer objects using CreateFramebuffers.
 *
 *      Setup first framebuffer with renderbuffer color, depth and stencil
 *      attachments. Setup storage size with width and height equal to 8. Set
 *      them as draw and read buffers. Clean up it, use black color. Check the
 *      framebuffer status.
 *
 *      Setup second framebuffer with texture color attachment. Setup storage
 *      size with width equal to 4 and height equal to 3.  Check the framebuffer
 *      status.
 *
 *      Prepare GLSL program which can draw triangles using orthographic
 *      projection. Fragment shader
 *
 *      Clean both framebuffers using ClearNamedFramebuffer* functions.
 *
 *      Use first framebuffer.
 *
 *      Draw to stencil a quad with screen positions [-0.5, -0.5], [-0.5, 0.5],
 *      [0.5, -0.5] and [0.5, 0.5].
 *
 *      Draw to depth buffer a quad with positions [-1, -1, -1], [-1, 1, -1],
 *      [1, -1, 0] and [1, 1, 0].
 *
 *      Turn on depth and stencil tests. Depth test shall pass if incoming depth
 *      value is LESS than stored. The Stencil test shall pass only for any
 *      stencil pass.
 *
 *      Draw Full screen quad to draw buffer with z = 0.5.
 *
 *      Blit the color content of the first framebuffer to the second with
 *      nearest filter.
 *
 *      Fetch data. Expect that second framebuffer contain following data
 *          black,  black,  black,  black,
 *          black,  black,  white,  black,
 *          black,  black,  black,  black.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions */
	FunctionalTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	FunctionalTest(const FunctionalTest& other);
	FunctionalTest& operator=(const FunctionalTest& other);

	bool PrepareFirstFramebuffer();
	bool PrepareSecondFramebuffer();
	bool ClearFramebuffers();
	void PrepareProgram();
	void PrepareBuffersAndVertexArrays();
	bool DrawAndBlit();
	bool CheckSecondFramebufferContent();
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo_1st;
	glw::GLuint m_fbo_2nd;
	glw::GLuint m_rbo_color;
	glw::GLuint m_rbo_depth_stencil;
	glw::GLuint m_to_color;
	glw::GLuint m_po;
	glw::GLuint m_vao_stencil_pass_quad;
	glw::GLuint m_vao_depth_pass_quad;
	glw::GLuint m_vao_color_pass_quad;
	glw::GLuint m_bo_stencil_pass_quad;
	glw::GLuint m_bo_depth_pass_quad;
	glw::GLuint m_bo_color_pass_quad;

	/* Private static variables. */
	static const glw::GLchar  s_vertex_shader[];
	static const glw::GLchar  s_fragment_shader[];
	static const glw::GLchar  s_attribute[];
	static const glw::GLfloat s_stencil_pass_quad[];
	static const glw::GLfloat s_depth_pass_quad[];
	static const glw::GLfloat s_color_pass_quad[];
	static const glw::GLuint  s_stencil_pass_quad_size;
	static const glw::GLuint  s_depth_pass_quad_size;
	static const glw::GLuint  s_color_pass_quad_size;
};
/* FunctionalTest class */
} /* Framebuffers namespace */

namespace Renderbuffers
{
/** Renderbuffer Creation
 *
 *      Create at least two renderbuffer objects using GenRenderbuffers
 *      function. Check them without binding, using IsRenderbuffer function.
 *      Expect FALSE.
 *
 *      Create at least two renderbuffer objects using CreateRenderbuffers
 *      function. Check them without binding, using IsRenderbuffer function.
 *      Expect TRUE.
 *
 *      Release objects.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

/** Named Renderbuffer Storage
 *
 *      Create renderbuffer object. Prepare its storage using
 *      NamedRenderbufferStorage function.
 *
 *      Create framebuffer object. Attach renderbuffer to proper attachment
 *      point.
 *
 *      Clear framebuffer's renderbuffer attachment with reference value. Fetch
 *      the data from framebuffer's attachment using ReadPixels. Compare the
 *      fetched values with the reference.
 *
 *      Release all objects.
 *
 *      Repeat the test for following internal formats:
 *
 *          R8, R16, RG8, RG16, RGB565, RGBA4, RGB5_A1, RGBA8, RGB10_A2,
 *          RGB10_A2UI, RGBA16, SRGB8_ALPHA8, R16F, RG16F, RGBA16F, R32F, RG32F,
 *          RGBA32F, R11F_G11F_B10F, R8I, R8UI, R16I, R16UI, R32I, R32UI, RG8I,
 *          RG8UI, RG16I, RG16UI, RG32I, RG32UI, RGBA8I, RGBA8UI, RGBA16I,
 *          RGBA16UI, RGBA32I, RGBA32UI, DEPTH_COMPONENT16, DEPTH_COMPONENT24,
 *          DEPTH_COMPONENT32F, DEPTH24_STENCIL8, DEPTH32F_STENCIL8 and
 *          STENCIL_INDEX8.
 *
 *      Repeat the test for following width and height:
 *          width = 1 and height = 1;
 *          width = 256 and height = 512;
 *          width = 1280 and height = 720;
 *          width = value of MAX_RENDERBUFFER_SIZE and height = 1;
 *          width = 1 and height = value of MAX_RENDERBUFFER_SIZE.
 */
class StorageTest : public deqp::TestCase
{
public:
	/* Public member functions */
	StorageTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private static variables. */
	static const struct RenderbufferInternalFormatConfiguration
	{
		glw::GLenum		   internalformat;
		const glw::GLchar* internalformat_name;
		bool			   hasRedComponent;
		bool			   hasGreenComponent;
		bool			   hasBlueComponent;
		bool			   hasAlphaComponent;
		bool			   hasDepthComponent;
		bool			   hasStencilComponent;
		bool			   isColorIntegralFormat;
	} s_renderbuffer_internalformat_configuration[];

	static const glw::GLuint s_renderbuffer_internalformat_configuration_count;

	static const glw::GLfloat s_reference_color[4];
	static const glw::GLint   s_reference_color_integer[4];
	static const glw::GLfloat s_reference_depth;
	static const glw::GLint   s_reference_stencil;

	/* Private member functions */
	StorageTest(const StorageTest& other);
	StorageTest& operator=(const StorageTest& other);

	bool PrepareRenderbuffer(StorageTest::RenderbufferInternalFormatConfiguration format, glw::GLuint width,
							 glw::GLuint height);
	void Clear(bool isIntegralFormat);
	bool Check(StorageTest::RenderbufferInternalFormatConfiguration format, glw::GLuint width, glw::GLuint height);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
};
/* StorageTest class */

/** Named Renderbuffer Storage Multisample
 *
 *      Create two renderbuffer objects. Prepare storage of the first one using
 *      NamedRenderbufferStorageMultisample function. Prepare storage of the
 *      second one using NamedRenderbufferStorage function.
 *
 *      Create two framebuffer objects. Attach multisampled renderbuffer to
 *      proper attachment points of the first framebuffer. Attach second
 *      renderbuffer to proper attachment points of the second framebuffer.
 *
 *      Clear framebuffer's renderbuffer attachment with reference value. Blit
 *      surface of the first framebuffer (multisampled renderbuffer) to the
 *      second framebuffer. Fetch the data from the second framebuffer using
 *      ReadPixels function. Compare the fetched values with the reference.
 *
 *      Release all objects.
 *
 *      Repeat the test for following internal formats:
 *
 *          R8, R16, RG8, RG16, RGB565, RGBA4, RGB5_A1, RGBA8, RGB10_A2,
 *          RGB10_A2UI, RGBA16, SRGB8_ALPHA8, R16F, RG16F, RGBA16F, R32F, RG32F,
 *          RGBA32F, R11F_G11F_B10F, R8I, R8UI, R16I, R16UI, R32I, R32UI, RG8I,
 *          RG8UI, RG16I, RG16UI, RG32I, RG32UI, RGBA8I, RGBA8UI, RGBA16I,
 *          RGBA16UI, RGBA32I, RGBA32UI, DEPTH_COMPONENT16, DEPTH_COMPONENT24,
 *          DEPTH_COMPONENT32F, DEPTH24_STENCIL8, DEPTH32F_STENCIL8 and
 *          STENCIL_INDEX8.
 *
 *      Repeat the test for following width and height:
 *          width = 1 and height = 1;
 *          width = value of MAX_RENDERBUFFER_SIZE and height = 1;
 *          width = 1 and height = value of MAX_RENDERBUFFER_SIZE.
 *
 *      Repeat the test for number of samples in range from 1 to value of
 *      MAX_INTEGER_SAMPLES for signed and unsigned integer internal formats or
 *      in range from 1 to value of MAX_SAMPLES for all other internal formats.
 */
class StorageMultisampleTest : public deqp::TestCase
{
public:
	/* Public member functions */
	StorageMultisampleTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private static variables. */
	static const struct RenderbufferInternalFormatConfiguration
	{
		glw::GLenum		   internalformat;
		const glw::GLchar* internalformat_name;
		bool			   hasRedComponent;
		bool			   hasGreenComponent;
		bool			   hasBlueComponent;
		bool			   hasAlphaComponent;
		bool			   hasDepthComponent;
		bool			   hasStencilComponent;
		bool			   isColorIntegralFormat;
	} s_renderbuffer_internalformat_configuration[];

	static const glw::GLuint s_renderbuffer_internalformat_configuration_count;

	static const glw::GLfloat s_reference_color[4];
	static const glw::GLint   s_reference_color_integer[4];
	static const glw::GLfloat s_reference_depth;
	static const glw::GLint   s_reference_stencil;

	/* Private member functions */
	StorageMultisampleTest(const StorageMultisampleTest& other);
	StorageMultisampleTest& operator=(const StorageMultisampleTest& other);

	bool PrepareRenderbuffer(StorageMultisampleTest::RenderbufferInternalFormatConfiguration format, glw::GLuint width,
							 glw::GLuint height, glw::GLsizei samples);
	void Bind(glw::GLenum target, glw::GLuint selector);
	void Blit(glw::GLuint width, glw::GLuint height);
	void Clear(bool isIntegralFormat);
	bool Check(StorageMultisampleTest::RenderbufferInternalFormatConfiguration format, glw::GLuint width,
			   glw::GLuint height);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_fbo[2];
	glw::GLuint m_rbo[2];
};
/* StorageMultisampleTest class */

/** Get Named Renderbuffer Parameter
 *
 *      Create named renderbuffer object with varying width = 1,
 *      height = 2, and varying internalformat.
 *
 *      For following parameter names:
 *       -  RENDERBUFFER_WIDTH,
 *       -  RENDERBUFFER_HEIGHT,
 *       -  RENDERBUFFER_INTERNAL_FORMAT,
 *       -  RENDERBUFFER_SAMPLES,
 *       -  RENDERBUFFER_RED_SIZE,
 *       -  RENDERBUFFER_GREEN_SIZE,
 *       -  RENDERBUFFER_BLUE_SIZE,
 *       -  RENDERBUFFER_ALPHA_SIZE,
 *       -  RENDERBUFFER_DEPTH_SIZE,
 *       -  RENDERBUFFER_STENCIL_SIZE
 *      query value using GetNamedRenderbufferParameteriv. Expect no error.
 *      Compare it with value returned in non-DSA way. Expect equality.
 *
 *      Repeat test for following internalformats:
 *       -  RGBA8,
 *       -  DEPTH_COMPONENT24,
 *       -  STENCIL_INDEX8,
 *       -  DEPTH24_STENCIL8.
 *
 *      Release objects.
 */
class GetParametersTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetParametersTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	GetParametersTest(const GetParametersTest& other);
	GetParametersTest& operator=(const GetParametersTest& other);

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
};
/* GetParametersTest class */

/** Create Renderbuffer Errors
 *
 *      Check that INVALID_VALUE is generated by CreateRenderbuffers if n is
 *      negative.
 */
class CreationErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationErrorsTest(const CreationErrorsTest& other);
	CreationErrorsTest& operator=(const CreationErrorsTest& other);
};
/* CreationErrorsTest class */

/** Named Renderbuffer Storage Errors
 *
 *      Check that INVALID_OPERATION is generated by NamedRenderbufferStorage if
 *      renderbuffer is not the name of an existing renderbuffer object.
 *
 *      Check that INVALID_VALUE is generated by NamedRenderbufferStorage if
 *      either of width or height is negative, or greater than the value of
 *      MAX_RENDERBUFFER_SIZE.
 */
class StorageErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	StorageErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	StorageErrorsTest(const StorageErrorsTest& other);
	StorageErrorsTest& operator=(const StorageErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_rbo_valid;
	glw::GLuint m_rbo_invalid;
	glw::GLenum m_internalformat_invalid;
};
/* StorageErrorsTest class */

/** Named Renderbuffer Storage Multisample Errors
 *
 *      Check that INVALID_OPERATION is generated by
 *      NamedRenderbufferStorageMultisample function if renderbuffer is not the
 *      name of an existing renderbuffer object.
 *
 *      Check that INVALID_VALUE is generated by
 *      NamedRenderbufferStorageMultisample if samples is greater than
 *      MAX_SAMPLES.
 *
 *      Check that INVALID_ENUM is generated by
 *      NamedRenderbufferStorageMultisample if internalformat is not a
 *      color-renderable, depth-renderable, or stencil-renderable format.
 *
 *      Check that INVALID_OPERATION is generated by
 *      NamedRenderbufferStorageMultisample if internalformat is a signed or
 *      unsigned integer format and samples is greater than the value of
 *      MAX_INTEGER_SAMPLES.
 *
 *      Check that INVALID_VALUE is generated by
 *      NamedRenderbufferStorageMultisample if either of width or height is
 *      negative, or greater than the value of GL_MAX_RENDERBUFFER_SIZE.
 */
class StorageMultisampleErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	StorageMultisampleErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	StorageMultisampleErrorsTest(const StorageMultisampleErrorsTest& other);
	StorageMultisampleErrorsTest& operator=(const StorageMultisampleErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_rbo_valid;
	glw::GLuint m_rbo_invalid;
	glw::GLenum m_internalformat_invalid;
	glw::GLint  m_max_samples;
	glw::GLint  m_max_integer_samples;
};
/* StorageMultisampleErrorsTest class */

/** Get Named Renderbuffer Parameter Errors
 *
 *      Check that INVALID_OPERATION is generated by
 *      GetNamedRenderbufferParameteriv if renderbuffer is not the name of an
 *      existing renderbuffer object.
 *
 *      Check that INVALID_ENUM is generated by GetNamedRenderbufferParameteriv
 *      if parameter name is not one of the accepted parameter names described
 *      in specification.
 */
class GetParameterErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetParameterErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	GetParameterErrorsTest(const GetParameterErrorsTest& other);
	GetParameterErrorsTest& operator=(const GetParameterErrorsTest& other);

	void PrepareObjects();
	bool ExpectError(glw::GLenum expected_error, const glw::GLchar* function, const glw::GLchar* conditions);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_rbo_valid;
	glw::GLuint m_rbo_invalid;
	glw::GLenum m_parameter_invalid;
};
/* GetParameterErrorsTest class */
} /* Renderbuffers namespace */

namespace VertexArrays
{
/** Vertex Array Object Creation
 *
 *     Create at least two vertex array objects using GenVertexArrays function.
 *     Check them without binding, using IsVertexArray function. Expect FALSE.
 *
 *     Create at least two vertex array objects using CreateVertexArrays
 *     function. Check them without binding, using IsVertexArray function.
 *     Expect TRUE.
 *
 *     Release objects.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

/** Vertex Array Object Enable Disable Attributes
 *
 *      Prepare vertex shader with (MAX_VERTEX_ATTRIBS / 2) attribute variables.
 *      Vertex shader shall sum all input attribute variables and pass the sum
 *      to transform feedback varying. Build program in two versions:
 *          1) with attribute variable names bound to even attribute indexes;
 *          2) with attribute variable names bound to odd attribute indexes.
 *
 *      Prepare and bind vertex array object.
 *
 *      Prepare buffer object with MAX_VERTEX_ATTRIBS of consecutive numbers.
 *      Bound each of the numbers to separate index. Prepare second object for
 *      transform feedback result.
 *
 *      Unbind vertex array object.
 *
 *      Enable even attribute indexes using EnableVertexArrayAttrib. Expect no
 *      error.
 *
 *      Bind vertex array object.
 *
 *      Use first program. Draw single point using transform feedback. Expect
 *      sum of numbers at even positions in the input (reference) buffer object.
 *
 *      Unbind vertex array object.
 *
 *      Disable even attribute indexes using DisableVertexArrayAttrib. Expect no
 *      error.
 *
 *      Enable odd attribute indexes using EnableVertexArrayAttrib. Expect no
 *      error.
 *
 *      Bind vertex array object.
 *
 *      Use second program. Draw single point using transform feedback. Expect
 *      sum of numbers at odd positions in the input (reference) buffer object.
 *
 *      Unbind vertex array object.
 *
 *      Release all objects.
 */
class EnableDisableAttributesTest : public deqp::TestCase
{
public:
	/* Public member functions */
	EnableDisableAttributesTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	EnableDisableAttributesTest(const EnableDisableAttributesTest& other);
	EnableDisableAttributesTest& operator=(const EnableDisableAttributesTest& other);

	glw::GLuint PrepareProgram(const bool bind_even_or_odd);
	glw::GLuint BuildProgram(const char* vertex_shader, const bool bind_even_or_odd);
	void PrepareVAO();
	void PrepareXFB();
	bool DrawAndCheck(bool bind_even_or_odd);
	bool TurnOnAttributes(bool enable_even, bool enable_odd);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_po_even;
	glw::GLuint m_po_odd;
	glw::GLuint m_vao;
	glw::GLuint m_bo;
	glw::GLuint m_bo_xfb;
	glw::GLint  m_max_attributes;

	/* Private static constants. */
	static const glw::GLchar s_vertex_shader_template[];
	static const glw::GLchar s_fragment_shader[];
};
/* EnableDisableAttributesTest class */

/** Vertex Array Object Element Buffer
 *
 *      Prepare GLSL program which passes input attribute to transform feedback
 *      varying.
 *
 *      Create and bind vertex array object.
 *
 *      Prepare buffer object with integer data {2, 1, 0}. Set this buffer as an
 *      input attribute. Use non-DSA functions.
 *
 *      Unbind vertex array object.
 *
 *      Prepare buffer object with integer data {2, 1, 0}. Set this buffer as an
 *      element buffer using VertexArrayElementBuffer function.
 *
 *      Bind vertex array object.
 *
 *      Use the program. Draw three points using transform feedback. Expect
 *      result equal to {0, 1, 2}.
 *
 *      Release all objects.
 */
class ElementBufferTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ElementBufferTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	ElementBufferTest(const ElementBufferTest& other);
	ElementBufferTest& operator=(const ElementBufferTest& other);

	void PrepareProgram();
	bool PrepareVAO();
	void PrepareXFB();
	bool DrawAndCheck();
	void Clean();

	/* Private member variables. */
	glw::GLuint m_po;
	glw::GLuint m_vao;
	glw::GLuint m_bo_array;
	glw::GLuint m_bo_elements;
	glw::GLuint m_bo_xfb;

	/* Private static constants. */
	static const glw::GLchar s_vertex_shader[];
	static const glw::GLchar s_fragment_shader[];
};
/* ElementBufferTest class */

/** Vertex Array Object Vertex Buffer and Buffers
 *
 *      Prepare GLSL program which passes sum of three input integer attributes
 *      to the transform feedback varying.
 *
 *      Prepare two vertex buffer objects. Setup first buffer with data {0, 1,
 *      2, 3}.
 *      Setup second buffer with data {4, 5}.
 *
 *      Create vertex array object. Setup three vertex attributes. Set first
 *      buffer object as an input attribute 0 and 1 in interleaved way using
 *      VertexArrayVertexBuffer function. Set second buffer object as an input
 *      attribute 2 using VertexArrayVertexBuffer function.
 *
 *      Use program. Draw 2 points using transform feedback. Query results.
 *      Expect two values {0+2+4, 1+3+5}.
 *
 *      Release all data.
 *
 *      Repeat the test using VertexArrayVertexBuffers function instead of
 *      VertexArrayVertexBuffer.
 */
class VertexBuffersTest : public deqp::TestCase
{
public:
	/* Public member functions */
	VertexBuffersTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	VertexBuffersTest(const VertexBuffersTest& other);
	VertexBuffersTest& operator=(const VertexBuffersTest& other);

	void PrepareProgram();
	bool PrepareVAO(bool use_multiple_buffers_function);
	void PrepareXFB();
	bool DrawAndCheck();
	void Clean();

	/* Private member variables. */
	glw::GLuint m_po;
	glw::GLuint m_vao;
	glw::GLuint m_bo_array_0;
	glw::GLuint m_bo_array_1;
	glw::GLuint m_bo_xfb;

	/* Private static constants. */
	static const glw::GLchar s_vertex_shader[];
	static const glw::GLchar s_fragment_shader[];
};
/* VertexBuffersTest class */

/** Vertex Array Object Attribute Format
 *
 *      Prepare GLSL program which passes sum of two input attributes to the
 *      transform feedback varying.
 *
 *      Create vertex array object.
 *
 *      Prepare vertex buffer object with reference data of two interleaved
 *      arrays. Setup it as input interleaved attributes.
 *
 *      Setup two consecutive attributes using VertexArrayAttribFormat function.
 *
 *      Use program. Draw 2 points using transform feedback. Query results.
 *      Expect sum of adequate reference values.
 *
 *      Release all data.
 *
 *      Repeat the test using VertexArrayAttribIFormat VertexArrayAttribLFormat
 *      function instead of VertexArrayAttribFormat.
 *
 *      Repeat the test using size 1, 2, 3, and 4 (if possible by type).
 *
 *      Repeat the test using type BYTE, SHORT, INT, FLOAT, DOUBLE,
 *      UNSIGNED_BYTE, UNSIGNED_SHORT, UNSIGNED_INT.
 *
 *      For test with VertexArrayAttribFormat function repeat for normalized and
 *      not normalized values.
 */
class AttributeFormatTest : public deqp::TestCase
{
public:
	/* Public member functions */
	AttributeFormatTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private state enumerations. */
	enum AtributeFormatFunctionType
	{
		ATTRIBUTE_FORMAT_FUNCTION_FLOAT,
		ATTRIBUTE_FORMAT_FUNCTION_DOUBLE,
		ATTRIBUTE_FORMAT_FUNCTION_INTEGER,
		ATTRIBUTE_FORMAT_FUNCTION_COUNT /* Must be last */
	};

	/* Private member functions. */
	AttributeFormatTest(const AttributeFormatTest& other);
	AttributeFormatTest& operator=(const AttributeFormatTest& other);

	template <typename T>
	bool compare(T a, T b);

	void PrepareProgram(glw::GLint size, AtributeFormatFunctionType function_selector);

	template <typename T>
	glw::GLdouble NormalizationScaleFactor();

	template <typename T>
	bool PrepareVAO(glw::GLint size, glw::GLenum type_gl_name, bool normalized,
					AtributeFormatFunctionType function_selector);

	void PrepareXFB();

	template <typename T>
	bool DrawAndCheck(glw::GLint size, bool normalized);

	void CleanVAO();
	void CleanProgram();
	void CleanXFB();

	/* Private member variables. */
	glw::GLuint m_po;
	glw::GLuint m_vao;
	glw::GLuint m_bo_array;
	glw::GLuint m_bo_xfb;

	/* Private static constants. */
	static const glw::GLchar* s_vertex_shader_head;
	static const glw::GLchar* s_vertex_shader_body;
	static const glw::GLchar* s_vertex_shader_declaration[ATTRIBUTE_FORMAT_FUNCTION_COUNT][4 /* sizes count */];
	static const glw::GLchar* s_fragment_shader;
};
/* AttributeFormatTest class */

/** Vertex Array Attribute Binding
 *
 *      Prepare GLSL program which passes two integer input attributes to the
 *      two-component transform feedback varying vector. Bind first attribute
 *      to attribute index 0. Bind second attribute to attribute index 1.
 *
 *      Create vertex array object.
 *
 *      Prepare vertex buffer object. Setup the buffer with data {1, 0}.
 *      Setup two integer attribute pointers in consecutive way.
 *
 *      Using VertexArrayAttribBinding function, set up binding index 0 to the
 *      attribute index 1. Using VertexArrayAttribBinding function, set up
 *      binding index 1 to the attribute index 0.
 *
 *      Prepare transform feedback buffer object.
 *
 *      Release all data.
 */
class AttributeBindingTest : public deqp::TestCase
{
public:
	/* Public member functions */
	AttributeBindingTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	AttributeBindingTest(const AttributeBindingTest& other);
	AttributeBindingTest& operator=(const AttributeBindingTest& other);

	void PrepareProgram();
	bool PrepareVAO();
	void PrepareXFB();
	bool DrawAndCheck();
	void Clean();

	/* Private member variables. */
	glw::GLuint m_po;
	glw::GLuint m_vao;
	glw::GLuint m_bo_array;
	glw::GLuint m_bo_xfb;

	/* Private static constants. */
	static const glw::GLchar s_vertex_shader[];
	static const glw::GLchar s_fragment_shader[];
};
/* AttributeBindingTest class */

class AttributeBindingDivisorTest : public deqp::TestCase
{
public:
	/* Public member functions */
	AttributeBindingDivisorTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	AttributeBindingDivisorTest(const AttributeBindingDivisorTest& other);
	AttributeBindingDivisorTest& operator=(const AttributeBindingDivisorTest& other);

	void PrepareProgram();
	void PrepareVAO();
	void PrepareXFB();
	void Draw(glw::GLuint number_of_points, glw::GLuint number_of_instances);
	bool SetDivisor(glw::GLuint divisor);
	bool CheckXFB(const glw::GLuint count, const glw::GLint expected[], const glw::GLchar* log_message);
	void Clean();

	/* Private member variables. */
	glw::GLuint m_po;
	glw::GLuint m_vao;
	glw::GLuint m_bo_array;
	glw::GLuint m_bo_xfb;

	/* Private static constants. */
	static const glw::GLchar s_vertex_shader[];
	static const glw::GLchar s_fragment_shader[];
};
/* AttributeBindingDivisorTest class */

/* Get Vertex Array
 *
 *      Create vertex array object.
 *
 *      Create buffer object. Set this buffer as an element buffer of the vertex
 *      array object.
 *
 *      Query ELEMENT_ARRAY_BUFFER_BINDING using GetVertexArrayiv. Expect buffer
 *      ID.
 *
 *      Release all objects.
 */
class GetVertexArrayTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetVertexArrayTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	GetVertexArrayTest(const GetVertexArrayTest& other);
	GetVertexArrayTest& operator=(const GetVertexArrayTest& other);
};
/* GetVertexArrayTest class */

/** Get Vertex Array Indexed
 *
 *      Create vertex array object.
 *
 *      Enable attribute indexes 0, 1, 2 and 3.
 *
 *      Create 4 buffer objects. Set these buffer as attribute arrays:
 *          -  attribute 0 with size 1, type BYTE, normalized, stride 0, offset 0,
 *          relative offset 0, binding divisor to 3;
 *          -  integer attribute 1 with size 2, type SHORT, stride 2, offset 2,
 *          relative offset 0, binding divisor to 2;
 *          -  attribute 2 with size 3, type FLOAT, not normalized, stride 0,
 *          offset 8, relative offset 4, binding divisor to 1;
 *          -  attribute 3 with size 4, type UNSIGNED_INT_2_10_10_10_REV, not
 *          normalized, stride 8, offset 4, relative offset 0,
 *          binding divisor to 0.
 *
 *      Query VERTEX_ATTRIB_ARRAY_ENABLED using GetVertexArrayIndexediv. Expect
 *      TRUE for consecutive indexes 0-3 and FALSE for index 4.
 *
 *      Query VERTEX_ATTRIB_ARRAY_SIZE using GetVertexArrayIndexediv. Expect
 *      1, 2, 3, 4 for consecutive indexes.
 *
 *      Query VERTEX_ATTRIB_ARRAY_STRIDE using GetVertexArrayIndexediv. Expect
 *      0, 2, 0, 8 for consecutive indexes.
 *
 *      Query VERTEX_ATTRIB_ARRAY_TYPE using GetVertexArrayIndexediv. Expect
 *      BYTE, SHORT, FLOAT, UNSIGNED_INT_2_10_10_10_REV for consecutive indexes.
 *
 *      Query VERTEX_ATTRIB_ARRAY_NORMALIZED using GetVertexArrayIndexediv.
 *      Expect true, false, false, false for consecutive indexes.
 *
 *      Query VERTEX_ATTRIB_ARRAY_INTEGER using GetVertexArrayIndexediv.
 *      Expect true, true, false, true for consecutive indexes.
 *
 *      Query VERTEX_ATTRIB_ARRAY_LONG using GetVertexArrayIndexediv. Expect
 *      false for consecutive indexes.
 *
 *      Query VERTEX_ATTRIB_ARRAY_DIVISOR using GetVertexArrayIndexediv. Expect
 *      3, 2, 1, 0 for consecutive indexes.
 *
 *      Query VERTEX_ATTRIB_RELATIVE_OFFSET using GetVertexArrayIndexediv.
 *      Expect 0, 0, 4, 0 for consecutive indexes.
 *
 *      Query VERTEX_BINDING_OFFSET using GetVertexArrayIndexed64iv. Expect 0,
 *      2, 8, 4 for consecutive indexes.
 *
 *      Release all objects.
 */
class GetVertexArrayIndexedTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetVertexArrayIndexedTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	GetVertexArrayIndexedTest(const GetVertexArrayIndexedTest& other);
	GetVertexArrayIndexedTest& operator=(const GetVertexArrayIndexedTest& other);

	void PrepareVAO();
	bool Check(const glw::GLenum pname, const glw::GLuint index, const glw::GLint expected);
	bool Check64(const glw::GLenum pname, const glw::GLuint index, const glw::GLint64 expected);

	/* Private member variables. */
	glw::GLuint m_vao;
	glw::GLuint m_bo[4];
};
/* GetVertexArrayIndexedTest class */

/** Vertex Array Defaults
 *
 *      Create empty vertex array object using CreateVertexArrays function.
 *
 *      Check that parameter VERTEX_ATTRIB_ARRAY_ENABLED for all possible
 *      attributes is equal to value FALSE.
 *
 *      Check that parameter VERTEX_ATTRIB_ARRAY_SIZE for all possible
 *      attributes is equal to value 4.
 *
 *      Check that parameter VERTEX_ATTRIB_ARRAY_STRIDE for all possible
 *      attributes is equal to value 0.
 *
 *      Check that parameter VERTEX_ATTRIB_ARRAY_TYPE for all possible
 *      attributes is equal to value FLOAT.
 *
 *      Check that parameter VERTEX_ATTRIB_ARRAY_NORMALIZED for all possible
 *      attributes is equal to value FALSE.
 *
 *      Check that parameter VERTEX_ATTRIB_ARRAY_INTEGER for all possible
 *      attributes is equal to value FALSE.
 *
 *      Check that parameter VERTEX_ATTRIB_ARRAY_LONG for all possible
 *      attributes is equal to value FALSE.
 *
 *      Check that parameter VERTEX_ATTRIB_ARRAY_DIVISOR for all possible
 *      attributes is equal to value 0.
 *
 *      Check that parameter VERTEX_ATTRIB_RELATIVE_OFFSET for all possible
 *      attributes is equal to value 0.
 *
 *      Check that parameter VERTEX_BINDING_OFFSET for all possible attributes
 *      is equal to value 0.
 *
 *      Check that parameter ELEMENT_ARRAY_BUFFER_BINDING is equal to value 0.
 *
 *      Release vertex array object.
 */
class DefaultsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	DefaultsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	DefaultsTest(const DefaultsTest& other);
	DefaultsTest& operator=(const DefaultsTest& other);

	void PrepareVAO();
	bool Check(const glw::GLenum pname, const glw::GLint expected);
	bool CheckIndexed(const glw::GLenum pname, const glw::GLuint index, const glw::GLint expected);
	bool CheckIndexed64(const glw::GLenum pname, const glw::GLuint index, const glw::GLint64 expected);

	/* Private member variables. */
	glw::GLuint m_vao;
};
/* DefaultsTest class */

/** Vertex Array Object Creation Error
 *
 *      Check that INVALID_VALUE is generated if n is negative.
 */
class CreationErrorTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationErrorTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	CreationErrorTest(const CreationErrorTest& other);
	CreationErrorTest& operator=(const CreationErrorTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* CreationErrorTest class */

/** Vertex Array Object Enable Disable Attribute Errors
 *
 *      Check that INVALID_OPERATION is generated by EnableVertexArrayAttrib and
 *      DisableVertexArrayAttrib if vaobj is not the name of an existing vertex
 *      array object.
 *
 *      Check that INVALID_VALUE is generated if index is greater than or equal
 *      to MAX_VERTEX_ATTRIBS.
 */
class EnableDisableAttributeErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	EnableDisableAttributeErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	EnableDisableAttributeErrorsTest(const EnableDisableAttributeErrorsTest& other);
	EnableDisableAttributeErrorsTest& operator=(const EnableDisableAttributeErrorsTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* EnableDisableAttributeErrorsTest class */

/** Vertex Array Object Element Buffer Errors
 *
 *      Check that INVALID_OPERATION error is generated by VertexArrayElementBuffer if vaobj is not the name
 *      of an existing vertex array object.
 *
 *      Check that INVALID_OPERATION error is generated by VertexArrayElementBuffer if buffer is not zero or
 *      the name of an existing buffer object.
 */
class ElementBufferErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	ElementBufferErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	ElementBufferErrorsTest(const ElementBufferErrorsTest& other);
	ElementBufferErrorsTest& operator=(const ElementBufferErrorsTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* ElementBuffersErrorsTest class */

/** Vertex Array Object Buffer and Buffers Errors
 *
 *      Check that INVALID_OPERATION is generated by VertexArrayVertexBuffer and
 *      VertexArrayVertexBuffers if vaobj is not the name of an existing vertex
 *      array object.
 *
 *      Check that INVALID_VALUE is generated by VertexArrayVertexBuffer if
 *      buffer is not zero or the name of an existing buffer object (as returned
 *      by GenBuffers or CreateBuffers).
 *
 *      Check that INVALID_OPERATION is generated by VertexArrayVertexBuffers if
 *      any value in buffers is not zero or the name of an existing buffer
 *      object.
 *
 *      Check that INVALID_VALUE is generated by VertexArrayVertexBuffer if
 *      bindingindex is greater than or equal to the value of
 *      MAX_VERTEX_ATTRIB_BINDINGS.
 *
 *      Check that INVALID_OPERATION is generated by VertexArrayVertexBuffers if
 *      first+count is greater than the value of MAX_VERTEX_ATTRIB_BINDINGS.
 *
 *      Check that INVALID_VALUE is generated by VertexArrayVertexBuffer if
 *      offset or stride is less than zero, or if stride is greater than the
 *      value of MAX_VERTEX_ATTRIB_STRIDE.
 *
 *      Check that INVALID_VALUE is generated by VertexArrayVertexBuffers if any
 *      value in offsets or strides is negative, or if a value is stride is
 *      greater than the value of MAX_VERTEX_ATTRIB_STRIDE.
 */
class VertexBuffersErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	VertexBuffersErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	VertexBuffersErrorsTest(const VertexBuffersErrorsTest& other);
	VertexBuffersErrorsTest& operator=(const VertexBuffersErrorsTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* VertexBuffersErrorsTest class */

/** Vertex Array Object Attribute Format Errors
 *
 *      Check that INVALID_VALUE is generated by VertexArrayAttrib*Format if
 *      attribindex is greater than or equal to the value of MAX_VERTEX_ATTRIBS.
 *
 *      Check that INVALID_VALUE is generated by VertexArrayAttrib*Format if
 *      size is not one of the accepted values.
 *
 *      Check that INVALID_VALUE is generated by VertexArrayAttrib*Format if
 *      relativeoffset is greater than the value of
 *      MAX_VERTEX_ATTRIB_RELATIVE_OFFSET.
 *
 *      Check that INVALID_ENUM is generated by VertexArrayAttrib*Format if type
 *      is not one of the accepted tokens.
 *
 *      Check that INVALID_ENUM is generated by VertexArrayAttrib{IL}Format if
 *      type is UNSIGNED_INT_10F_11F_11F_REV.
 *
 *      Check that INVALID_OPERATION is generated by VertexArrayAttrib*Format if
 *      vaobj is not the name of an existing vertex array object.
 *
 *      Check that INVALID_OPERATION is generated by VertexArrayAttribFormat
 *      under any of the following conditions:
 *       -  size is BGRA and type is not UNSIGNED_BYTE, INT_2_10_10_10_REV or
 *          UNSIGNED_INT_2_10_10_10_REV,
 *       -  type is INT_2_10_10_10_REV or UNSIGNED_INT_2_10_10_10_REV, and size
 *          is neither 4 nor BGRA,
 *       -  type is UNSIGNED_INT_10F_11F_11F_REV and size is not 3,
 *       -  size is BGRA and normalized is FALSE.
 */
class AttributeFormatErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	AttributeFormatErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	AttributeFormatErrorsTest(const AttributeFormatErrorsTest& other);
	AttributeFormatErrorsTest& operator=(const AttributeFormatErrorsTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* AttributeFormatErrorsTest class */

/** Vertex Array Attribute Binding Errors
 *
 *      Check that INVALID_OPERATION is generated by VertexArrayAttribBinding if
 *      vaobj is not the name of an existing vertex array object.
 *
 *      Check that INVALID_VALUE is generated by VertexArrayAttribBinding if
 *      attribindex is greater than or equal to the value of MAX_VERTEX_ATTRIBS.
 *
 *      Check that INVALID_VALUE is generated by VertexArrayAttribBinding if
 *      bindingindex is greater than or equal to the value of
 *      MAX_VERTEX_ATTRIB_BINDINGS.
 */
class AttributeBindingErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	AttributeBindingErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	AttributeBindingErrorsTest(const AttributeBindingErrorsTest& other);
	AttributeBindingErrorsTest& operator=(const AttributeBindingErrorsTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* AttributeBindingErrorsTest class */

/** Vertex Array Binding Divisor Errors
 *
 *      Check that INVALID_VALUE is generated by VertexArrayBindingDivisor if
 *      bindingindex is greater than or equal to the value of
 *      MAX_VERTEX_ATTRIB_BINDINGS.
 *
 *      Check that INVALID_OPERATION is generated by VertexArrayBindingDivisor
 *      if vaobj is not the name of an existing vertex array object.
 */
class AttributeBindingDivisorErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	AttributeBindingDivisorErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	AttributeBindingDivisorErrorsTest(const AttributeBindingDivisorErrorsTest& other);
	AttributeBindingDivisorErrorsTest& operator=(const AttributeBindingDivisorErrorsTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* AttributeBindingDivisorErrorsTest class */

/** Get Vertex Array Errors
 *
 *      Check that INVALID_OPERATION error is generated by GetVertexArrayiv if
 *      vaobj is not the name of an existing vertex array object.
 *
 *      Check that INVALID_ENUM error is generated by GetVertexArrayiv if pname
 *      is not ELEMENT_ARRAY_BUFFER_BINDING.
 */
class GetVertexArrayErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetVertexArrayErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	GetVertexArrayErrorsTest(const GetVertexArrayErrorsTest& other);
	GetVertexArrayErrorsTest& operator=(const GetVertexArrayErrorsTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* GetVertexArrayErrorsTest class */

/** Get Vertex Array Indexed Errors
 *
 *      Check that INVALID_OPERATION error is generated by
 *      GetVertexArrayIndexediv and GetVertexArrayIndexed64iv if vaobj is not
 *      the name of an existing vertex array object.
 *
 *      Check that INVALID_VALUE error is generated by GetVertexArrayIndexediv
 *      and GetVertexArrayIndexed64iv if index is greater than or equal to the
 *      value of MAX_VERTEX_ATTRIBS.
 *
 *      Check that INVALID_ENUM error is generated by GetVertexArrayIndexediv if
 *      pname is not one of the valid values:
 *       -  VERTEX_ATTRIB_ARRAY_ENABLED,
 *       -  VERTEX_ATTRIB_ARRAY_SIZE,
 *       -  VERTEX_ATTRIB_ARRAY_STRIDE,
 *       -  VERTEX_ATTRIB_ARRAY_TYPE,
 *       -  VERTEX_ATTRIB_ARRAY_NORMALIZED,
 *       -  VERTEX_ATTRIB_ARRAY_INTEGER,
 *       -  VERTEX_ATTRIB_ARRAY_LONG,
 *       -  VERTEX_ATTRIB_ARRAY_DIVISOR,
 *       -  VERTEX_ATTRIB_RELATIVE_OFFSET.
 *
 *      Check that INVALID_ENUM error is generated by GetVertexArrayIndexed64iv
 *      if pname is not VERTEX_BINDING_OFFSET.
 */
class GetVertexArrayIndexedErrorsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	GetVertexArrayIndexedErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	GetVertexArrayIndexedErrorsTest(const GetVertexArrayIndexedErrorsTest& other);
	GetVertexArrayIndexedErrorsTest& operator=(const GetVertexArrayIndexedErrorsTest& other);

	bool CheckError(const glw::GLenum expected, const glw::GLchar* log_message);
};
/* GetVertexArrayIndexedErrorsTest class */

namespace Utilities
{
std::string itoa(glw::GLuint i);
std::string replace(const std::string& src, const std::string& key, const std::string& value);
} /* Vertex Arrays utilities class */
} /* VertexArrays namespace */

/* Direct State Access Textures Tests */
namespace Textures
{
/** @class CreationTest
 *
 *  @brief Direct State Access Texture Creation test cases.
 *
 *  Test follows the steps:
 *
 *      Create at least two texture objects using GenTextures function. Check
 *      them without binding, using IsTexture function. Expect FALSE.
 *
 *      Create at least two texture objects using CreateTextures function. Check
 *      them without binding, using IsTexture function. Expect TRUE. Repeat this
 *      step for all targets:
 *          -  TEXTURE_1D,
 *          -  TEXTURE_2D,
 *          -  TEXTURE_3D,
 *          -  TEXTURE_1D_ARRAY,
 *          -  TEXTURE_2D_ARRAY,
 *          -  TEXTURE_RECTANGLE,
 *          -  TEXTURE_CUBE_MAP,
 *          -  TEXTURE_CUBE_MAP_ARRAY,
 *          -  TEXTURE_BUFFER,
 *          -  TEXTURE_2D_MULTISAMPLE and
 *          -  TEXTURE_2D_MULTISAMPLE_ARRAY.
 *
 *      Release objects.
 */
class CreationTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CreationTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CreationTest(const CreationTest& other);
	CreationTest& operator=(const CreationTest& other);
};
/* CreationTest class */

class Reference
{
public:
	template <typename T, glw::GLint S, bool N>
	static glw::GLenum InternalFormat();

	template <glw::GLint S, bool N>
	static glw::GLenum Format();

	template <typename T>
	static glw::GLenum Type();

	template <typename T, bool N>
	static const T* ReferenceData();

	static glw::GLuint ReferenceDataCount();

	template <typename T>
	static glw::GLuint ReferenceDataSize();

	template <typename T>
	static bool Compare(const T a, const T b);

private:
	static const glw::GLuint s_reference_count = 2 /* 1D */ * 3 /* 2D */ * 4 /* 3D */ * 4 /* components */;
};

/** @class BufferTest
 *
 *  @brief Direct State Access of texture buffers.
 *
 *  @tparam T      Type.
 *  @tparam S      Size.
 *  @tparam N      Is normalized.
 *
 *  Test follows the steps:
 *
 *      Make test for following DSA functions:
 *        -  TextureBuffer,
 *        -  TextureBufferRange
 *       and following texture internal formats:
 *        -  R8,
 *        -  R16,
 *        -  R16F,
 *        -  R32F,
 *        -  R8I,
 *        -  R16I,
 *        -  R32I,
 *        -  R8UI,
 *        -  R16UI,
 *        -  R32UI,
 *        -  RG8,
 *        -  RG16,
 *        -  RG16F,
 *        -  RG32F,
 *        -  RG8I,
 *        -  RG16I,
 *        -  RG32I,
 *        -  RG8UI,
 *        -  RG16UI,
 *        -  RG32UI,
 *        -  RGB32F,
 *        -  RGB32I,
 *        -  RGB32UI,
 *        -  RGBA8,
 *        -  RGBA16,
 *        -  RGBA16F,
 *        -  RGBA32F,
 *        -  RGBA8I,
 *        -  RGBA16I,
 *        -  RGBA32I,
 *        -  RGBA8UI,
 *        -  RGBA16UI,
 *        -  RGBA32UI.
 *
 *          Prepare program which draws textured quad 6 x 1 pixels in size. The
 *          sampled texture shall be buffer texture which linearly store two rows
 *          of three pixels.
 *
 *          Prepare framebuffer 6 x 1 pixels in size.
 *
 *          Prepare texture object with attached buffer object as a storage using
 *          TextureBuffer or TextureBufferRange function. When TextureBufferRange is
 *          being used, test non-zero offset setup. The buffer object shall contain
 *          unique reference values. Texture filtering shall be set to NEAREST.
 *
 *          Using prepared GL objects draw a quad. Fetch framebuffer data using
 *          ReadPixels function. Compare the results with the reference data. Expect
 *          equality.
 *
 *          Release all objects.
 */
template <typename T, glw::GLint S, bool N>
class BufferTest : public deqp::TestCase, Reference
{
public:
	/* Public member functions. */
	BufferTest(deqp::Context& context, const char* name);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	BufferTest(const BufferTest& other);
	BufferTest& operator=(const BufferTest& other);

	/* Private member functions. */
	static glw::GLuint   TestReferenceDataCount();

	static glw::GLuint TestReferenceDataSize();

	static const glw::GLchar* FragmentShaderDeclaration();

	bool CreateBufferTexture(bool use_range_version);

	bool Check();

	bool Test(bool use_range_version);

	bool PrepareFramebuffer(const glw::GLenum internal_format);
	void PrepareProgram(const glw::GLchar* variable_declaration);
	void PrepareVertexArray();
	void Draw();
	void CleanBufferTexture();
	void CleanFramebuffer();
	void CleanProgram();
	void CleanErrors();
	void CleanVertexArray();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
	glw::GLuint m_po;
	glw::GLuint m_to;
	glw::GLuint m_bo;
	glw::GLuint m_vao;

	/* Private static constants. */
	static const glw::GLuint  s_fbo_size_x = 6;
	static const glw::GLuint  s_fbo_size_y = 1;
	static const glw::GLchar* s_vertex_shader;
	static const glw::GLchar* s_fragment_shader_head;
	static const glw::GLchar* s_fragment_shader_fdecl_lowp;
	static const glw::GLchar* s_fragment_shader_idecl_lowp;
	static const glw::GLchar* s_fragment_shader_udecl_lowp;
	static const glw::GLchar* s_fragment_shader_fdecl_mediump;
	static const glw::GLchar* s_fragment_shader_idecl_mediump;
	static const glw::GLchar* s_fragment_shader_udecl_mediump;
	static const glw::GLchar* s_fragment_shader_fdecl_highp;
	static const glw::GLchar* s_fragment_shader_idecl_highp;
	static const glw::GLchar* s_fragment_shader_udecl_highp;
	static const glw::GLchar* s_fragment_shader_tail;
};

/** @brief Fragment shader part selector.
 *
 *  @return Array of characters with source code.
 */
template <typename T, glw::GLint S, bool N>
const glw::GLchar* BufferTest<T, S, N>::FragmentShaderDeclaration()
{
	if (typeid(T) == typeid(glw::GLbyte))
	{
		return s_fragment_shader_idecl_lowp;
	}

	if (typeid(T) == typeid(glw::GLubyte))
	{
		return N ? s_fragment_shader_fdecl_lowp : s_fragment_shader_udecl_lowp;
	}

	if (typeid(T) == typeid(glw::GLshort))
	{
		return s_fragment_shader_idecl_mediump;
	}

	if (typeid(T) == typeid(glw::GLushort))
	{
		return N ? s_fragment_shader_fdecl_mediump : s_fragment_shader_udecl_mediump;
	}

	if (typeid(T) == typeid(glw::GLint))
	{
		return s_fragment_shader_idecl_highp;
	}

	if (typeid(T) == typeid(glw::GLuint))
	{
		return s_fragment_shader_udecl_highp;
	}

	return s_fragment_shader_fdecl_highp;
}

/* BufferTest class */

/** @class StorageAndSubImageTest
 *
 *  @tparam T      Type.
 *  @tparam S      Size.
 *  @tparam N      Is normalized.
 *  @tparam D      Texture dimension.
 *  @tparam I      Choose between SubImage and Storage tests.
 *
 *      Make test for following DSA storage functions:
 *       -  TextureStorage1D,
 *       -  TextureStorage2D,
 *       -  TextureStorage3D
 *      and DSA SubImage functions:
 *       -  TextureSubImage1D,
 *       -  TextureSubImage2D,
 *       -  TextureSubImage3D.
 *
 *      Test following internal formats:
 *       -  R8,
 *       -  R16,
 *       -  R16F,
 *       -  R32F,
 *       -  R8I,
 *       -  R16I,
 *       -  R32I,
 *       -  R8UI,
 *       -  R16UI,
 *       -  R32UI,
 *       -  RG8,
 *       -  RG16,
 *       -  RG16F,
 *       -  RG32F,
 *       -  RG8I,
 *       -  RG16I,
 *       -  RG32I,
 *       -  RG8UI,
 *       -  RG16UI,
 *       -  RG32UI,
 *       -  RGB32F,
 *       -  RGB32I,
 *       -  RGB32UI,
 *       -  RGBA8,
 *       -  RGBA16,
 *       -  RGBA16F,
 *       -  RGBA32F,
 *       -  RGBA8I,
 *       -  RGBA16I,
 *       -  RGBA32I,
 *       -  RGBA8UI,
 *       -  RGBA16UI,
 *       -  RGBA32UI.
 *
 *      Create texture and prepare its storage with the tested function and
 *      reference data. The texture dimensions shall be 2x3x4 texels in
 *      corresponding directions (if available).
 *
 *      Prepare GLSL program with fragment shader which fetches texture and passes
 *      it to the framebuffer in serialized way.
 *
 *      Prepare framebuffer 24 x 1 pixels in size.
 *
 *      Make draw call with prepared texture and program. Fetch framebuffer and
 *      compare values with the reference data. Expect equality.
 *
 *      Release all objects.
 */
template <typename T, glw::GLint S, bool N, glw::GLuint D, bool I>
class StorageAndSubImageTest : public deqp::TestCase, Reference
{
public:
	/* Public member functions. */
	StorageAndSubImageTest(deqp::Context& context, const char* name);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	StorageAndSubImageTest(const StorageAndSubImageTest& other);
	StorageAndSubImageTest& operator=(const StorageAndSubImageTest& other);

	/* Private member functions. */
	static glw::GLuint TestReferenceDataCount();

	static glw::GLuint	TestReferenceDataWidth();

	static glw::GLuint	TestReferenceDataHeight();

	static glw::GLuint	TestReferenceDataDepth();

	static glw::GLuint TestReferenceDataSize();

	static const glw::GLchar* FragmentShaderDeclaration();

	static const glw::GLchar* FragmentShaderTail();

	static glw::GLenum	TextureTarget();

	bool TextureSubImage(glw::GLenum target, glw::GLuint texture, glw::GLint level,
						 glw::GLsizei width, glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
						 glw::GLenum type, const glw::GLvoid* data);

	bool TextureStorage(glw::GLenum target, glw::GLuint texture, glw::GLsizei levels, glw::GLenum internalformat,
						glw::GLsizei width, glw::GLsizei height, glw::GLsizei depth);

	bool CreateTexture();

	bool Check();

	bool Test();

	void PrepareFramebuffer(const glw::GLenum internal_format);
	void PrepareProgram(const glw::GLchar* variable_declaration, const glw::GLchar* tail);
	void PrepareVertexArray();
	void Draw();
	void CleanTexture();
	void CleanFramebuffer();
	void CleanProgram();
	void CleanErrors();
	void CleanVertexArray();

	/* Private member variables. */
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
	glw::GLuint m_po;
	glw::GLuint m_to;
	glw::GLuint m_vao;

	/* Private static constants. */
	static const glw::GLchar* s_vertex_shader;
	static const glw::GLchar* s_fragment_shader_head;
	static const glw::GLchar* s_fragment_shader_1D_fdecl_lowp;
	static const glw::GLchar* s_fragment_shader_1D_idecl_lowp;
	static const glw::GLchar* s_fragment_shader_1D_udecl_lowp;
	static const glw::GLchar* s_fragment_shader_1D_fdecl_mediump;
	static const glw::GLchar* s_fragment_shader_1D_idecl_mediump;
	static const glw::GLchar* s_fragment_shader_1D_udecl_mediump;
	static const glw::GLchar* s_fragment_shader_1D_fdecl_highp;
	static const glw::GLchar* s_fragment_shader_1D_idecl_highp;
	static const glw::GLchar* s_fragment_shader_1D_udecl_highp;
	static const glw::GLchar* s_fragment_shader_2D_fdecl_lowp;
	static const glw::GLchar* s_fragment_shader_2D_idecl_lowp;
	static const glw::GLchar* s_fragment_shader_2D_udecl_lowp;
	static const glw::GLchar* s_fragment_shader_2D_fdecl_mediump;
	static const glw::GLchar* s_fragment_shader_2D_idecl_mediump;
	static const glw::GLchar* s_fragment_shader_2D_udecl_mediump;
	static const glw::GLchar* s_fragment_shader_2D_fdecl_highp;
	static const glw::GLchar* s_fragment_shader_2D_idecl_highp;
	static const glw::GLchar* s_fragment_shader_2D_udecl_highp;
	static const glw::GLchar* s_fragment_shader_3D_fdecl_lowp;
	static const glw::GLchar* s_fragment_shader_3D_idecl_lowp;
	static const glw::GLchar* s_fragment_shader_3D_udecl_lowp;
	static const glw::GLchar* s_fragment_shader_3D_fdecl_mediump;
	static const glw::GLchar* s_fragment_shader_3D_idecl_mediump;
	static const glw::GLchar* s_fragment_shader_3D_udecl_mediump;
	static const glw::GLchar* s_fragment_shader_3D_fdecl_highp;
	static const glw::GLchar* s_fragment_shader_3D_idecl_highp;
	static const glw::GLchar* s_fragment_shader_3D_udecl_highp;
	static const glw::GLchar* s_fragment_shader_1D_tail;
	static const glw::GLchar* s_fragment_shader_2D_tail;
	static const glw::GLchar* s_fragment_shader_3D_tail;
};
/* StorageAndSubImageTest class */

/** class StorageMultisampleTest
 *
 *      @tparam T      Type.
 *      @tparam S      Size.
 *      @tparam N      Is normalized.
 *      @tparam D      Texture dimension.
 *
 *      Make test for following DSA functions:
 *       -  TextureStorage2DMultisample and
 *       -  TextureStorage3DMultisample.
 *
 *      Test following internal formats:
 *       -  R8,
 *       -  R16,
 *       -  R16F,
 *       -  R32F,
 *       -  R8I,
 *       -  R16I,
 *       -  R32I,
 *       -  R8UI,
 *       -  R16UI,
 *       -  R32UI,
 *       -  RG8,
 *       -  RG16,
 *       -  RG16F,
 *       -  RG32F,
 *       -  RG8I,
 *       -  RG16I,
 *       -  RG32I,
 *       -  RG8UI,
 *       -  RG16UI,
 *       -  RG32UI,
 *       -  RGB32F,
 *       -  RGB32I,
 *       -  RGB32UI,
 *       -  RGBA8,
 *       -  RGBA16,
 *       -  RGBA16F,
 *       -  RGBA32F,
 *       -  RGBA8I,
 *       -  RGBA16I,
 *       -  RGBA32I,
 *       -  RGBA8UI,
 *       -  RGBA16UI,
 *       -  RGBA32UI.
 *
 *      Create multisample texture and prepare its storage with the tested
 *      function. The texture dimensions shall be 2x3x4 texels in corresponding
 *      directions (if available) and two samples per texel.
 *
 *      Prepare two framebuffers. The first one with the multisample texture
 *      as a color attachment with size 2x3 pixels and 4 color attachments
 *      (layers). The second one with non-multisample renderbuffer storage
 *      similar in size.
 *
 *      Prepare GLSL program which draws explicitly reference data to
 *      multisample texture framebuffer.
 *
 *      Use program to draw the reference data into multisample texture.
 *
 *      Prepare second GLSL program with fragment shader which passes samples of
 *      the input texture to the separate framebuffer pixels.
 *
 *      Use the second program to draw the multisample texture into
 *      renderbuffer.
 *
 *      Fetch framebuffer data and compare with the reference values. Expect
 *      equality.
 *
 *      Release all objects.
 */
template <typename T, glw::GLint S, bool N, glw::GLuint D>
class StorageMultisampleTest : public deqp::TestCase, Reference
{
public:
	/* Public member functions. */
	StorageMultisampleTest(deqp::Context& context, const char *name);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	StorageMultisampleTest(const StorageMultisampleTest& other);
	StorageMultisampleTest& operator=(const StorageMultisampleTest& other);

	/* Private member functions. */
	static glw::GLuint TestReferenceDataCount();

	static glw::GLuint	TestReferenceDataWidth();

	static glw::GLuint	TestReferenceDataHeight();

	static glw::GLuint	TestReferenceDataDepth();

	static glw::GLuint TestReferenceDataSize();

	static const glw::GLchar* FragmentShaderDeclarationMultisample();

	static const glw::GLchar* FragmentShaderDeclarationAuxiliary();

	static const glw::GLchar* FragmentShaderTail();

	static glw::GLenum	InputTextureTarget();

	static glw::GLenum	MultisampleTextureTarget();

	void InputTextureImage(const glw::GLenum internal_format, const glw::GLuint width, const glw::GLuint height,
						   const glw::GLuint depth, const glw::GLenum format, const glw::GLenum type,
						   const glw::GLvoid* data);

	void CreateInputTexture();

	bool Check();

	bool Test();

	bool PrepareFramebufferMultisample(const glw::GLenum internal_format);

	void PrepareFramebufferAuxiliary(const glw::GLenum internal_format);

	glw::GLuint PrepareProgram(const glw::GLchar* variable_declaration, const glw::GLchar* tail);
	void PrepareVertexArray();

	void Draw();

	void CleanInputTexture();
	void CleanAuxiliaryTexture();
	void CleanFramebuffers();
	void CleanPrograms();
	void CleanErrors();
	void CleanVertexArray();

	/* Private member variables. */
	glw::GLuint m_fbo_ms;
	glw::GLuint m_fbo_aux;
	glw::GLuint m_to_ms;
	glw::GLuint m_po_ms;
	glw::GLuint m_po_aux;
	glw::GLuint m_to;
	glw::GLuint m_to_aux;
	glw::GLuint m_vao;

	/* Private static constants. */
	static const glw::GLchar* s_vertex_shader;
	static const glw::GLchar* s_fragment_shader_head;
	static const glw::GLchar* s_fragment_shader_ms_2D_fdecl_lowp;
	static const glw::GLchar* s_fragment_shader_ms_2D_idecl_lowp;
	static const glw::GLchar* s_fragment_shader_ms_2D_udecl_lowp;
	static const glw::GLchar* s_fragment_shader_ms_2D_fdecl_mediump;
	static const glw::GLchar* s_fragment_shader_ms_2D_idecl_mediump;
	static const glw::GLchar* s_fragment_shader_ms_2D_udecl_mediump;
	static const glw::GLchar* s_fragment_shader_ms_2D_fdecl_highp;
	static const glw::GLchar* s_fragment_shader_ms_2D_idecl_highp;
	static const glw::GLchar* s_fragment_shader_ms_2D_udecl_highp;

	static const glw::GLchar* s_fragment_shader_ms_3D_fdecl_lowp;
	static const glw::GLchar* s_fragment_shader_ms_3D_idecl_lowp;
	static const glw::GLchar* s_fragment_shader_ms_3D_udecl_lowp;
	static const glw::GLchar* s_fragment_shader_ms_3D_fdecl_mediump;
	static const glw::GLchar* s_fragment_shader_ms_3D_idecl_mediump;
	static const glw::GLchar* s_fragment_shader_ms_3D_udecl_mediump;
	static const glw::GLchar* s_fragment_shader_ms_3D_fdecl_highp;
	static const glw::GLchar* s_fragment_shader_ms_3D_idecl_highp;
	static const glw::GLchar* s_fragment_shader_ms_3D_udecl_highp;

	static const glw::GLchar* s_fragment_shader_aux_2D_fdecl_lowp;
	static const glw::GLchar* s_fragment_shader_aux_2D_idecl_lowp;
	static const glw::GLchar* s_fragment_shader_aux_2D_udecl_lowp;
	static const glw::GLchar* s_fragment_shader_aux_2D_fdecl_mediump;
	static const glw::GLchar* s_fragment_shader_aux_2D_idecl_mediump;
	static const glw::GLchar* s_fragment_shader_aux_2D_udecl_mediump;
	static const glw::GLchar* s_fragment_shader_aux_2D_fdecl_highp;
	static const glw::GLchar* s_fragment_shader_aux_2D_idecl_highp;
	static const glw::GLchar* s_fragment_shader_aux_2D_udecl_highp;

	static const glw::GLchar* s_fragment_shader_aux_3D_fdecl_lowp;
	static const glw::GLchar* s_fragment_shader_aux_3D_idecl_lowp;
	static const glw::GLchar* s_fragment_shader_aux_3D_udecl_lowp;
	static const glw::GLchar* s_fragment_shader_aux_3D_fdecl_mediump;
	static const glw::GLchar* s_fragment_shader_aux_3D_idecl_mediump;
	static const glw::GLchar* s_fragment_shader_aux_3D_udecl_mediump;
	static const glw::GLchar* s_fragment_shader_aux_3D_fdecl_highp;
	static const glw::GLchar* s_fragment_shader_aux_3D_idecl_highp;
	static const glw::GLchar* s_fragment_shader_aux_3D_udecl_highp;
	static const glw::GLchar* s_fragment_shader_tail_2D;
	static const glw::GLchar* s_fragment_shader_tail_3D;
};
/* StorageMultisampleTest class */

/**  @class CompressedSubImage
 *
 *    Make test for following DSA functions:
 *        -  CompressedTextureSubImage1D,
 *        -  CompressedTextureSubImage2D,
 *        -  CompressedTextureSubImage3D.
 *
 *    Make test for following uncompressed internal formats:
 *        -  R8,
 *        -  R8_SNORM,
 *        -  R16,
 *        -  R16_SNORM,
 *        -  RG8,
 *        -  RG8_SNORM,
 *        -  RG16,
 *        -  RG16_SNORM,
 *        -  R3_G3_B2,
 *        -  RGB4,
 *        -  RGB5,
 *        -  RGB8,
 *        -  RGB8_SNORM,
 *        -  RGB10,
 *        -  RGB12,
 *        -  RGB16_SNORM,
 *        -  RGBA2,
 *        -  RGBA4,
 *        -  RGB5_A1,
 *        -  RGBA8,
 *        -  RGBA8_SNORM,
 *        -  RGB10_A2,
 *        -  RGB10_A2UI,
 *        -  RGBA12,
 *        -  RGBA16,
 *        -  SRGB8,
 *        -  SRGB8_ALPHA8,
 *        -  R16F,
 *        -  RG16F,
 *        -  RGB16F,
 *        -  RGBA16F,
 *        -  R32F,
 *        -  RG32F,
 *        -  RGB32F,
 *        -  RGBA32F,
 *        -  R11F_G11F_B10F,
 *        -  RGB9_E5,
 *        -  R8I,
 *        -  R8UI,
 *        -  R16I,
 *        -  R16UI,
 *        -  R32I,
 *        -  R32UI,
 *        -  RG8I,
 *        -  RG8UI,
 *        -  RG16I,
 *        -  RG16UI,
 *        -  RG32I,
 *        -  RG32UI,
 *        -  RGB8I,
 *        -  RGB8UI,
 *        -  RGB16I,
 *        -  RGB16UI,
 *        -  RGB32I,
 *        -  RGB32UI,
 *        -  RGBA8I,
 *        -  RGBA8UI,
 *        -  RGBA16I,
 *        -  RGBA16UI,
 *        -  RGBA32I,
 *        -  RGBA32UI.
 *    and compressed internal formats:
 *        -  COMPRESSED_RGBA8_ETC2_EAC.
 *
 *    Create texture and setup its storage and data using tested function with
 *    size 2x3 pixels.
 *
 *    Prepare framebuffer with renderbuffer color attachment with floating
 *    point internal format and with size 2x3 pixels
 *
 *    Prepare GLSL program with fragment shader which passes input texture to
 *    the framebuffer.
 *
 *    Draw a full screen quad with the prepared texture, program and
 *    framebuffer. Read the framebuffer content. Compare framebuffer's values
 *    with the reference values. Take normalization and precision into
 *    account. Expect equality.
 *
 *    Release all objects.
 */
class CompressedSubImageTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	CompressedSubImageTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	CompressedSubImageTest(const CompressedSubImageTest& other);
	CompressedSubImageTest& operator=(const CompressedSubImageTest& other);

	void CreateTextures(glw::GLenum target);

	template <glw::GLuint D>
	glw::GLenum			  TextureTarget();

	template <glw::GLuint D>
	bool TextureImage(glw::GLint internalformat);

	template <glw::GLuint D>
	void CompressedTexImage(glw::GLint internalformat);

	template <glw::GLuint D>
	bool CompressedTextureSubImage(glw::GLint internalformat);

	template <glw::GLuint D>
	bool PrepareReferenceData(glw::GLenum internalformat);

	template <glw::GLuint D>
	void PrepareStorage(glw::GLenum internalformat);

	template <glw::GLuint D>
	void PrepareCompressedStorage(glw::GLenum internalformat);

	template <glw::GLuint D>
	bool CheckData(glw::GLenum internalformat);
	void		CleanAll();
	std::string DataToString(glw::GLuint count, const glw::GLubyte data[]);

	template <glw::GLuint D>
	bool Test(glw::GLenum internalformat, bool can_be_unsupported);

	/* Private member variables. */
	glw::GLuint   m_to;
	glw::GLuint   m_to_aux;
	glw::GLubyte* m_compressed_texture_data;
	glw::GLubyte* m_reference;
	glw::GLubyte* m_result;
	glw::GLuint   m_reference_size;
	glw::GLuint   m_reference_internalformat;

	/* Private static constants. */
	static const glw::GLubyte s_texture_data[];
	static const glw::GLuint  s_texture_width;
	static const glw::GLuint  s_texture_height;
	static const glw::GLuint  s_texture_depth;
	static const glw::GLuint  s_block_count;
	static const glw::GLuint  s_block_2d_size_x;
	static const glw::GLuint  s_block_2d_size_y;
	static const glw::GLuint  s_block_3d_size;
};
/* CompressedSubImageTest class */

/** @class CopyTest
 *
 *      Make test for following DSA functions:
 *       -  CopyTextureSubImage1D,
 *       -  CopyTextureSubImage2D and
 *       -  CopyTextureSubImage3D.
 *
 *      Prepare two textures 2x3x4 texels in size for corresponding directions
 *      (if available). Setup the first one with reference data.
 *
 *      Prepare framebuffer with the first texture attached to the a color
 *      attachment point. Bind the framebuffer.
 *
 *      Copy framebuffer content to the texture using tested function. The
 *      images shall be copied in ranges, two per direction (to test offsets,
 *      positions and size variables). For 3D textures copy each layer
 *      substituting the framebuffer attachment.
 *
 *      After the copy fetch texture data and compare it with the reference
 *      values. Expect equality.
 *
 *      Release all objects.
 */
class CopyTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	CopyTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	CopyTest(const CopyTest& other);
	CopyTest& operator=(const CopyTest& other);

	/* Private member functions. */
	template <glw::GLuint D>
	glw::GLenum			  TextureTarget();

	bool CopyTextureSubImage1DAndCheckErrors(glw::GLuint texture, glw::GLint level, glw::GLint xoffset, glw::GLint x,
											 glw::GLint y, glw::GLsizei width);
	bool CopyTextureSubImage2DAndCheckErrors(glw::GLuint texture, glw::GLint level, glw::GLint xoffset,
											 glw::GLint yoffset, glw::GLint x, glw::GLint y, glw::GLsizei width,
											 glw::GLsizei height);
	bool CopyTextureSubImage3DAndCheckErrors(glw::GLuint texture, glw::GLint level, glw::GLint xoffset,
											 glw::GLint yoffset, glw::GLint zoffset, glw::GLint x, glw::GLint y,
											 glw::GLsizei width, glw::GLsizei height);

	template <glw::GLuint D>
	void				  CreateSourceTexture();

	template <glw::GLuint D>
	void				  CreateDestinationTexture();

	template <glw::GLuint D>
	void				  CreateSourceFramebuffer();

	template <glw::GLuint D>
	void				  CreateAll();

	template <glw::GLuint D>
	bool				  Test();

	bool CheckData(glw::GLenum target, glw::GLuint size);
	std::string DataToString(glw::GLuint count, const glw::GLubyte data[]);
	void CleanAll();

	/* Private member variables. */
	glw::GLuint   m_fbo;
	glw::GLuint   m_to_src;
	glw::GLuint   m_to_dst;
	glw::GLubyte* m_result;

	/* Private static constants. */
	static const glw::GLubyte s_texture_data[];
	static const glw::GLuint  s_texture_width;
	static const glw::GLuint  s_texture_height;
	static const glw::GLuint  s_texture_depth;
};
/* CopyTest class */

/** @class GetSetParameterTest
 *
 *  Do following:
 *      Prepare texture object.
 *
 *      Prepare the following test case.
 *
 *          Prepare test case which sets a parameter to the desired value using
 *          one of the following functions (depending on the parameter type):
 *           -  TextureParameterf,
 *           -  TextureParameterfv,
 *           -  TextureParameteri,
 *           -  TextureParameterIiv,
 *           -  TextureParameterIuiv,
 *           -  TextureParameteriv.
 *
 *          Read back the texture parameter using one of the DSA-like functions
 *           -  GetTextureParameterfv,
 *           -  GetTextureParameteriv,
 *           -  GetTextureParameterIiv,
 *           -  GetTextureParameterIuiv.
 *          Expect equality.
 *
 *      Run the test case for following parameters and values:
 *       -  parameter DEPTH_STENCIL_TEXTURE_MODE with value DEPTH_COMPONENT;
 *       -  parameter TEXTURE_BASE_LEVEL with value 2;
 *       -  parameter TEXTURE_BORDER_COLOR with value {0.25, 0.5, 0.75, 1.0}
 *       -  parameter TEXTURE_COMPARE_FUNC with value LEQUAL;
 *       -  parameter TEXTURE_COMPARE_MODE with value COMPARE_REF_TO_TEXTURE;
 *       -  parameter TEXTURE_LOD_BIAS with value -2.0 (which is
 *          minimum required implementation maximum value);
 *       -  parameter TEXTURE_MIN_FILTER with value LINEAR_MIPMAP_NEAREST;
 *       -  parameter TEXTURE_MAG_FILTER with value NEAREST;
 *       -  parameter TEXTURE_MIN_LOD with value -100;
 *       -  parameter TEXTURE_MAX_LOD with value 100;
 *       -  parameter TEXTURE_MAX_LEVEL with value 100;
 *       -  parameter TEXTURE_SWIZZLE_R with value BLUE;
 *       -  parameter TEXTURE_SWIZZLE_G with value ALPHA;
 *       -  parameter TEXTURE_SWIZZLE_B with value RED;
 *       -  parameter TEXTURE_SWIZZLE_A with value GREEN;
 *       -  parameter TEXTURE_SWIZZLE_RGBA with value { ZERO, ONE, ZERO, ONE };
 *       -  parameter TEXTURE_WRAP_S with value MIRROR_CLAMP_TO_EDGE;
 *       -  parameter TEXTURE_WRAP_T with value CLAMP_TO_EDGE;
 *       -  parameter TEXTURE_WRAP_R with value CLAMP_TO_EDGE.
 *
 *      Release the texture object.
 */
class GetSetParameterTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	GetSetParameterTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	GetSetParameterTest(const GetSetParameterTest& other);
	GetSetParameterTest& operator=(const GetSetParameterTest& other);

	bool CheckErrorAndLog(const glw::GLchar* fname, glw::GLenum pname);
	bool CompareAndLog(glw::GLint value_src, glw::GLint value_dst, glw::GLenum pname);
	bool CompareAndLog(glw::GLuint value_src, glw::GLuint value_dst, glw::GLenum pname);
	bool CompareAndLog(glw::GLfloat value_src, glw::GLfloat value_dst, glw::GLenum pname);
	bool CompareAndLog(glw::GLfloat value_src[4], glw::GLfloat value_dst[4], glw::GLenum pname);
	bool CompareAndLog(glw::GLint value_src[4], glw::GLint value_dst[4], glw::GLenum pname);
	bool CompareAndLog(glw::GLuint value_src[4], glw::GLuint value_dst[4], glw::GLenum pname);
};
/* GetSetParameterTest class */

/** @class DefaultsTest
 *
 *      Create texture object with CreateTextures. Do not bind it.
 *
 *      Using one of the functions
 *       -  GetTextureParameterfv,
 *       -  GetTextureParameteriv,
 *       -  GetTextureParameterIiv,
 *       -  GetTextureParameterIuiv
 *      check that initial object parameter values are set to the following
 *      defaults:
 *       -  for parameter DEPTH_STENCIL_TEXTURE_MODE initial value is
 *          DEPTH_COMPONENT;
 *       -  for parameter TEXTURE_BASE_LEVEL initial value is 0;
 *       -  for parameter TEXTURE_BORDER_COLOR initial value is {0.0, 0.0, 0.0,
 *          0.0};
 *       -  for parameter TEXTURE_COMPARE_FUNC initial value is LEQUAL;
 *       -  for parameter TEXTURE_COMPARE_MODE initial value is NONE;
 *       -  for parameter TEXTURE_LOD_BIAS initial value is 0.0;
 *       -  for parameter TEXTURE_MIN_FILTER initial value is
 *          NEAREST_MIPMAP_LINEAR;
 *       -  for parameter TEXTURE_MAG_FILTER initial value is LINEAR;
 *       -  for parameter TEXTURE_MIN_LOD initial value is -1000;
 *       -  for parameter TEXTURE_MAX_LOD initial value is 1000;
 *       -  for parameter TEXTURE_MAX_LEVEL initial value is 1000;
 *       -  for parameter TEXTURE_SWIZZLE_R initial value is RED;
 *       -  for parameter TEXTURE_SWIZZLE_G initial value is GREEN;
 *       -  for parameter TEXTURE_SWIZZLE_B initial value is BLUE;
 *       -  for parameter TEXTURE_SWIZZLE_A initial value is ALPHA;
 *       -  for parameter TEXTURE_WRAP_S initial value is REPEAT;
 *       -  for parameter TEXTURE_WRAP_T initial value is REPEAT;
 *       -  for parameter TEXTURE_WRAP_R initial value is REPEAT.
 */
class DefaultsTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	DefaultsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	DefaultsTest(const DefaultsTest& other);
	DefaultsTest& operator=(const DefaultsTest& other);

	bool CompareAndLog(glw::GLint value_ref, glw::GLint value_dst, glw::GLenum pname);
	bool CompareAndLog(glw::GLuint value_ref, glw::GLuint value_dst, glw::GLenum pname);
	bool CompareAndLog(glw::GLfloat value_ref, glw::GLfloat value_dst, glw::GLenum pname);
	bool CompareAndLog(glw::GLfloat value_ref[4], glw::GLfloat value_dst[4], glw::GLenum pname);
	bool CompareAndLog(glw::GLint value_ref[4], glw::GLint value_dst[4], glw::GLenum pname);
	bool CompareAndLog(glw::GLuint value_ref[4], glw::GLuint value_dst[4], glw::GLenum pname);
};
/* DefaultsTest class */

/** @class GenerateMipmapTest
 *
 *      Create one dimensional texture. Setup its image data with successive
 *      numbers {0..255} stored as red color.
 *
 *      Generate mipmaps for the texture using GenerateTextureMipmap function.
 *
 *      Download each of the generated mipmap levels. Check that each of the
 *      mipmaps contains series of not decreasing values.
 *
 *      Release texture object.
 */
class GenerateMipmapTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	GenerateMipmapTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	GenerateMipmapTest(const GenerateMipmapTest& other);
	GenerateMipmapTest& operator=(const GenerateMipmapTest& other);

	/* Private static constants. */
	static const glw::GLubyte s_texture_data[];
	static const glw::GLuint  s_texture_width;
	static const glw::GLuint  s_texture_width_log;
};
/* GenerateMipmapTest class */

/** @class BindUnitTest
 *
 *      Create four 2D textures, filled with 2x3 texels of reference data in RED
 *      format and R8 internal format.
 *
 *      Create framebuffer 2x3 pixels of size with the same internal format as
 *      textures but RGBA format.
 *
 *      Bind each texture to the separate unit using BindTextureUnit function.
 *
 *      Prepare GLSL program which draws full screen quad. A fragment shader of
 *      the program shall pass each of the four input texture red values into
 *      separate RGBA channel of the output framebuffer.
 *
 *      Make a draw call with prepared objects.
 *
 *      Fetch framebuffer data. Expect interleaved reference data.
 *
 *      Release all objects.
 */
class BindUnitTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	BindUnitTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	BindUnitTest(const BindUnitTest& other);
	BindUnitTest& operator=(const BindUnitTest& other);

	void		CreateProgram();
	void		CreateTextures();
	void		CreateFrambuffer();
	void		CreateVertexArray();
	bool		Draw();
	bool		Check();
	void		CleanAll();
	std::string DataToString(glw::GLuint count, const glw::GLubyte data[]);

	/* Private member variables. */
	glw::GLuint   m_po;
	glw::GLuint   m_to[4];
	glw::GLuint   m_fbo;
	glw::GLuint   m_rbo;
	glw::GLuint   m_vao;
	glw::GLubyte* m_result;

	/* Private static constants. */
	static const glw::GLubyte s_texture_data_r[];
	static const glw::GLubyte s_texture_data_g[];
	static const glw::GLubyte s_texture_data_b[];
	static const glw::GLubyte s_texture_data_a[];
	static const glw::GLubyte s_texture_data_rgba[];
	static const glw::GLuint  s_texture_width;
	static const glw::GLuint  s_texture_height;
	static const glw::GLuint  s_texture_count_rgba;
	static const glw::GLchar* s_vertex_shader;
	static const glw::GLchar* s_fragment_shader;
	static const glw::GLchar* s_fragment_shader_samplers[4];
};
/* GenerateMipmapTest class */

/** @class GetImageTest
 *
 *          Make test for following DSA functions:
 *       -  GetTextureImage,
 *       -  GetCompressedTextureImage.
 *
 *      Create two 2D textures, one with compressed reference image, one with
 *      uncompressed reference image.
 *
 *      Fetch textures with corresponding test functions. Compare fetched values
 *      with the reference data. Expect equality.
 *
 *      Release textures.
 */
class GetImageTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	GetImageTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	GetImageTest(const GetImageTest& other);
	GetImageTest& operator=(const GetImageTest& other);

	std::string DataToString(glw::GLuint count, const glw::GLubyte data[]);

	/* Private static constants. */
	static const glw::GLubyte s_texture_data[];
	static const glw::GLubyte s_texture_data_compressed[];
	static const glw::GLuint  s_texture_width;
	static const glw::GLuint  s_texture_height;
	static const glw::GLuint  s_texture_size;
	static const glw::GLuint  s_texture_size_compressed;
	static const glw::GLuint  s_texture_count;
	static const glw::GLuint  s_texture_count_compressed;
};
/* GetImageTest class */

/** @class GetLevelParameterTest
 *
 *      Make test for following DSA functions:
 *          -  GetTextureLevelParameterfv,
 *          -  GetTextureLevelParameteriv.
 *
 *      Create 3D texture with two levels of detail.
 *
 *      Fetch following parameters with test functions:
 *       -  TEXTURE_WIDTH,
 *       -  TEXTURE_HEIGHT,
 *       -  TEXTURE_DEPTH,
 *       -  TEXTURE_INTERNAL_FORMAT,
 *       -  TEXTURE_RED_TYPE,
 *       -  TEXTURE_GREEN_TYPE,
 *       -  TEXTURE_BLUE_TYPE,
 *       -  TEXTURE_ALPHA_TYPE,
 *       -  TEXTURE_DEPTH_TYPE,
 *       -  TEXTURE_RED_SIZE,
 *       -  TEXTURE_GREEN_SIZE,
 *       -  TEXTURE_BLUE_SIZE,
 *       -  TEXTURE_ALPHA_SIZE,
 *       -  TEXTURE_DEPTH_SIZE and
 *       -  TEXTURE_COMPRESSED
 *      and compare values with expected set.
 *
 *      Release texture.
 */
class GetLevelParameterTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	GetLevelParameterTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	GetLevelParameterTest(const GetLevelParameterTest& other);
	GetLevelParameterTest& operator=(const GetLevelParameterTest& other);

	/* Private static constants. */
	static const glw::GLubyte s_texture_data[];
	static const glw::GLuint  s_texture_width;
	static const glw::GLuint  s_texture_height;
	static const glw::GLuint  s_texture_depth;
};
/* GetLevelParameterTest class */

/** @class ErrorsUtilities
 *
 *      This class contain utility methods for all negative tests.
 */
class ErrorsUtilities
{
public:
	bool CheckErrorAndLog(deqp::Context& context, glw::GLuint expected_error, const glw::GLchar* function_name,
						  const glw::GLchar* log);
};
/* ErrorsUtilities  */

/** @class CreationErrorsTest
 *
 *      Check that INVALID_ENUM is generated if target is not one of the
 *      allowable values.
 *
 *      Check that INVALID_VALUE is generated if n is negative.
 */
class CreationErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	CreationErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	CreationErrorsTest(const CreationErrorsTest& other);
	CreationErrorsTest& operator=(const CreationErrorsTest& other);

	glw::GLenum NotATarget();
};
/* CreationErrorsTest class */

/** @class BufferErrorsTest
 *
 *      Check that INVALID_OPERATION is generated by glTextureBuffer if texture
 *      is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM is generated by glTextureBuffer if the effective
 *      target of texture is not TEXTURE_BUFFER.
 *
 *      Check that INVALID_ENUM is generated if internalformat is not one of the
 *      sized internal formats described above.
 *
 *      Check that INVALID_OPERATION is generated if buffer is not zero and is
 *      not the name of an existing buffer object.
 */
class BufferErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	BufferErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	BufferErrorsTest(const BufferErrorsTest& other);
	BufferErrorsTest& operator=(const BufferErrorsTest& other);
};
/* BufferErrorsTest class */

/** @class BufferRangeErrorsTest
 *
 *      Check that INVALID_OPERATION is generated by TextureBufferRange if
 *      texture is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM is generated by TextureBufferRange if the
 *      effective target of texture is not TEXTURE_BUFFER.
 *
 *      Check that INVALID_ENUM is generated by TextureBufferRange if
 *      internalformat is not one of the sized internal formats described above.
 *
 *      Check that INVALID_OPERATION is generated by TextureBufferRange if
 *      buffer is not zero and is not the name of an existing buffer object.
 *
 *      Check that INVALID_VALUE is generated by TextureBufferRange if offset
 *      is negative, if size is less than or equal to zero, or if offset + size
 *      is greater than the value of BUFFER_SIZE for buffer.
 *
 *      Check that INVALID_VALUE is generated by TextureBufferRange if offset is
 *      not an integer multiple of the value of TEXTURE_BUFFER_OFFSET_ALIGNMENT.
 */
class BufferRangeErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	BufferRangeErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	BufferRangeErrorsTest(const BufferRangeErrorsTest& other);
	BufferRangeErrorsTest& operator=(const BufferRangeErrorsTest& other);
};
/* BufferErrorsTest class */

/** @class StorageErrorsTest
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage1D if texture
 *      is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage1D if
 *      internalformat is not a valid sized internal format.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage1D if target or
 *      the effective target of texture is not one of the accepted targets
 *      described above.
 *
 *      Check that INVALID_VALUE is generated by TextureStorage1D if width or
 *      levels are less than 1.
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage1D if levels
 *      is greater than log2(width)+1.
 *
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage2D if
 *      texture is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage2D if
 *      internalformat is not a valid sized internal format.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage2D if target or
 *      the effective target of texture is not one of the accepted targets
 *      described above.
 *
 *      Check that INVALID_VALUE is generated by TextureStorage2D if width,
 *      height or levels are less than 1.
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage2D if target
 *      is TEXTURE_1D_ARRAY or PROXY_TEXTURE_1D_ARRAY and levels is greater than
 *      log2(width)+1.
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage2D if target
 *      is not TEXTURE_1D_ARRAY or PROXY_TEXTURE_1D_ARRAY and levels is greater
 *      than log2(max(width, height))+1.
 *
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage2DMultisample
 *      if texture is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage2DMultisample if
 *      internalformat is not a valid color-renderable, depth-renderable or
 *      stencil-renderable format.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage2DMultisample if
 *      target or the effective target of texture is not one of the accepted
 *      targets described above.
 *
 *      Check that INVALID_VALUE is generated by TextureStorage2DMultisample if
 *      width or height are less than 1 or greater than the value of
 *      MAX_TEXTURE_SIZE.
 *
 *
 *      Check that INVALID_VALUE is generated by TextureStorage2DMultisample if
 *      samples is greater than the value of MAX_SAMPLES.
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage2DMultisample
 *      if the value of TEXTURE_IMMUTABLE_FORMAT for the texture bound to target
 *      is not FALSE.
 *
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage3D if texture
 *      is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage3D if
 *      internalformat is not a valid sized internal format.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage3D if target or
 *      the effective target of texture is not one of the accepted targets
 *      described above.
 *
 *      Check that INVALID_VALUE is generated by TextureStorage3D if width,
 *      height, depth or levels are less than 1.
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage3D if target
 *      is TEXTURE_3D or PROXY_TEXTURE_3D and levels is greater than
 *      log2(max(width, height, depth))+1.
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage3D if target
 *      is TEXTURE_2D_ARRAY, PROXY_TEXTURE_2D_ARRAY, TEXURE_CUBE_ARRAY,
 *      or PROXY_TEXTURE_CUBE_MAP_ARRAY and levels is greater than
 *      log2(max(width, height))+1.
 *
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage3DMultisample
 *      if texture is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage3DMultisample if
 *      internalformat is not a valid color-renderable, depth-renderable or
 *      stencil-renderable format.
 *
 *      Check that INVALID_ENUM is generated by TextureStorage3DMultisample if
 *      target or the effective target of texture is not one of the accepted
 *      targets described above.
 *
 *      Check that INVALID_VALUE is generated by TextureStorage3DMultisample if
 *      width or height are less than 1 or greater than the value of
 *      MAX_TEXTURE_SIZE.
 *
 *      Check that INVALID_VALUE is generated by TextureStorage3DMultisample if
 *      depth is less than 1 or greater than the value of
 *      MAX_ARRAY_TEXTURE_LAYERS.
 *
 *      Check that INVALID_VALUE is generated by TextureStorage3DMultisample if
 *      samples is greater than the value of MAX_SAMPLES.
 *
 *      Check that INVALID_OPERATION is generated by TextureStorage3DMultisample
 *      if the value of TEXTURE_IMMUTABLE_FORMAT for the texture bound to
 *      target is not FALSE.
 */
class StorageErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	StorageErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	StorageErrorsTest(const StorageErrorsTest& other);
	StorageErrorsTest& operator=(const StorageErrorsTest& other);

	glw::GLuint m_to_1D;
	glw::GLuint m_to_1D_array;
	glw::GLuint m_to_2D;
	glw::GLuint m_to_2D_array;
	glw::GLuint m_to_3D;
	glw::GLuint m_to_2D_ms;
	glw::GLuint m_to_2D_ms_immutable;
	glw::GLuint m_to_3D_ms;
	glw::GLuint m_to_3D_ms_immutable;
	glw::GLuint m_to_invalid;
	glw::GLuint m_internalformat_invalid;
	glw::GLint  m_max_texture_size;
	glw::GLint  m_max_samples;
	glw::GLint  m_max_array_texture_layers;

	void Prepare();
	bool Test1D();
	bool Test2D();
	bool Test3D();
	bool Test2DMultisample();
	bool Test3DMultisample();
	void Clean();
};
/* StorageErrorsTest class */

/** @class SubImageErrorsTest
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage1D if
 *          texture is not the name of an existing texture object.
 *
 *          Check that INVALID_ENUM is generated by TextureSubImage1D if format is
 *          not an accepted format constant.
 *
 *          Check that INVALID_ENUM is generated by TextureSubImage1D if type is not
 *          a type constant.
 *
 *          Check that INVALID_VALUE is generated by TextureSubImage1D if level is
 *          less than 0.
 *
 *          Check that INVALID_VALUE may be generated by TextureSubImage1D if level
 *          is greater than log2 max, where max is the returned value of
 *          MAX_TEXTURE_SIZE.
 *
 *          Check that INVALID_VALUE is generated by TextureSubImage1D if
 *          xoffset<â�’b, or if (xoffset+width)>(wâ�’b), where w is the TEXTURE_WIDTH,
 *          and b is the width of the TEXTURE_BORDER of the texture image being
 *          modified. Note that w includes twice the border width.
 *          Check that INVALID_VALUE is generated by TextureSubImage1D if width is
 *          less than 0.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage1D if type
 *          is one of UNSIGNED_BYTE_3_3_2, UNSIGNED_BYTE_2_3_3_REV,
 *          UNSIGNED_SHORT_5_6_5, or UNSIGNED_SHORT_5_6_5_REV and format is not RGB.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage1D if type
 *          is one of UNSIGNED_SHORT_4_4_4_4, UNSIGNED_SHORT_4_4_4_4_REV,
 *          UNSIGNED_SHORT_5_5_5_1, UNSIGNED_SHORT_1_5_5_5_REV,
 *          UNSIGNED_INT_8_8_8_8, UNSIGNED_INT_8_8_8_8_REV, UNSIGNED_INT_10_10_10_2,
 *          or UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA nor BGRA.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage1D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and the buffer object's data store is currently mapped.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage1D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and the data would be unpacked from the buffer object such that the
 *          memory reads required would exceed the data store size.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage1D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and pixels is not evenly divisible into the number of bytes needed to
 *          store in memory a datum indicated by type.
 *
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage2D if
 *          texture is not the name of an existing texture object.
 *
 *          Check that INVALID_ENUM is generated by TextureSubImage2D if format is
 *          not an accepted format constant.
 *
 *          Check that INVALID_ENUM is generated if type is not a type constant.
 *
 *          Check that INVALID_VALUE is generated by TextureSubImage2D if level is
 *          less than 0.
 *
 *          Check that INVALID_VALUE may be generated by TextureSubImage2D if level
 *          is greater than log2 max, where max is the returned value of
 *          MAX_TEXTURE_SIZE.
 *          Check that INVALID_VALUE is generated by TextureSubImage2D if
 *          xoffset<â�’b, (xoffset+width)>(wâ�’b), yoffset<â�’b, or
 *          (yoffset+height)>(hâ�’b), where w is the TEXTURE_WIDTH, h is the
 *          TEXTURE_HEIGHT, and b is the border width of the texture image being
 *          modified. Note that w and h include twice the border width.
 *          Check that INVALID_VALUE is generated by TextureSubImage2D if width or
 *          height is less than 0.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage2D if type
 *          is one of UNSIGNED_BYTE_3_3_2, UNSIGNED_BYTE_2_3_3_REV,
 *          UNSIGNED_SHORT_5_6_5, or UNSIGNED_SHORT_5_6_5_REV and format is not RGB.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage2D if type
 *          is one of UNSIGNED_SHORT_4_4_4_4, UNSIGNED_SHORT_4_4_4_4_REV,
 *          UNSIGNED_SHORT_5_5_5_1, UNSIGNED_SHORT_1_5_5_5_REV, UNSIGNED_INT_8_8_8_8,
 *          UNSIGNED_INT_8_8_8_8_REV, UNSIGNED_INT_10_10_10_2, or
 *          UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA
 *          nor BGRA.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage2D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and the buffer object's data store is currently mapped.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage2D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and the data would be unpacked from the buffer object such that the
 *          memory reads required would exceed the data store size.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage2D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and pixels is not evenly divisible into the number of bytes needed to
 *          store in memory a datum indicated by type.
 *
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage3D if
 *          texture is not the name of an existing texture object.
 *
 *          Check that INVALID_ENUM is generated by TextureSubImage3D if format is
 *          not an accepted format constant.
 *
 *          Check that INVALID_ENUM is generated by TextureSubImage3D if type is
 *          not a type constant.
 *
 *          Check that INVALID_VALUE is generated by TextureSubImage3D if level
 *          is less than 0.
 *
 *          Check that INVALID_VALUE may be generated by TextureSubImage3D if level
 *          is greater than log2 max, where max is the returned value of
 *          MAX_TEXTURE_SIZE.
 *
 *          Check that INVALID_VALUE is generated by TextureSubImage3D if
 *          xoffset<â�’b, (xoffset+width)>(wâ�’b), yoffset<â�’b, or
 *          (yoffset+height)>(hâ�’b), or zoffset<â�’b, or (zoffset+depth)>(dâ�’b), where w
 *          is the TEXTURE_WIDTH, h is the TEXTURE_HEIGHT, d is the TEXTURE_DEPTH
 *          and b is the border width of the texture image being modified. Note
 *          that w, h, and d include twice the border width.
 *
 *          Check that INVALID_VALUE is generated by TextureSubImage3D if width,
 *          height, or depth is less than 0.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage3D if type
 *          is one of UNSIGNED_BYTE_3_3_2, UNSIGNED_BYTE_2_3_3_REV,
 *          UNSIGNED_SHORT_5_6_5, or UNSIGNED_SHORT_5_6_5_REV and format is not RGB.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage3D if type
 *          is one of UNSIGNED_SHORT_4_4_4_4, UNSIGNED_SHORT_4_4_4_4_REV,
 *          UNSIGNED_SHORT_5_5_5_1, UNSIGNED_SHORT_1_5_5_5_REV,
 *          UNSIGNED_INT_8_8_8_8, UNSIGNED_INT_8_8_8_8_REV, UNSIGNED_INT_10_10_10_2,
 *          or UNSIGNED_INT_2_10_10_10_REV and format is neither RGBA nor BGRA.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage3D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and the buffer object's data store is currently mapped.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage3D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and the data would be unpacked from the buffer object such that the
 *          memory reads required would exceed the data store size.
 *
 *          Check that INVALID_OPERATION is generated by TextureSubImage3D if a
 *          non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER target
 *          and pixels is not evenly divisible into the number of bytes needed to
 *          store in memory a datum indicated by type.
 *
 *
 *          Check that INVALID_ENUM is generated by CompressedTextureSubImage1D if
 *          internalformat is not one of the generic compressed internal formats:
 *          COMPRESSED_RED, COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA.
 *          COMPRESSED_SRGB, or COMPRESSED_SRGB_ALPHA.
 *
 *          Check that INVALID_VALUE is generated by CompressedTextureSubImage1D if
 *          imageSize is not consistent with the format, dimensions, and contents of
 *          the specified compressed image data.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage1D
 *          if parameter combinations are not supported by the specific compressed
 *          internal format as specified in the specific texture compression
 *          extension.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage1D
 *          if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
 *          target and the buffer object's data store is currently mapped.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage1D
 *          if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
 *          target and the data would be unpacked from the buffer object such that
 *          the memory reads required would exceed the data store size.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage1D
 *          function if texture is not the name of an existing texture object.
 *
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
 *          if texture is not the name of an existing texture object.
 *
 *          Check that INVALID_ENUM is generated by CompressedTextureSubImage2D if
 *          internalformat is of the generic compressed internal formats:
 *          COMPRESSED_RED, COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA.
 *          COMPRESSED_SRGB, or COMPRESSED_SRGB_ALPHA.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
 *          if format does not match the internal format of the texture image being
 *          modified, since these commands do not provide for image format
 *          conversion.
 *
 *          Check that INVALID_VALUE is generated by CompressedTextureSubImage2D if
 *          imageSize is not consistent with the format, dimensions, and contents of
 *          the specified compressed image data.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
 *          if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
 *          target and the buffer object's data store is currently mapped.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
 *          if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
 *          target and the data would be unpacked from the buffer object such that
 *          the memory reads required would exceed the data store size.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage2D
 *          if the effective target is TEXTURE_RECTANGLE.
 *
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage3D
 *          if texture is not the name of an existing texture object.
 *
 *          Check that INVALID_ENUM is generated by CompressedTextureSubImage3D if
 *          internalformat is one of the generic compressed internal formats:
 *          COMPRESSED_RED, COMPRESSED_RG, COMPRESSED_RGB, COMPRESSED_RGBA.
 *          COMPRESSED_SRGB, or COMPRESSED_SRGB_ALPHA.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage3D
 *          if format does not match the internal format of the texture image being
 *          modified, since these commands do not provide for image format
 *          conversion.
 *
 *          Check that INVALID_VALUE is generated by CompressedTextureSubImage3D if
 *          imageSize is not consistent with the format, dimensions, and contents of
 *          the specified compressed image data.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage3D
 *          if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
 *          target and the buffer object's data store is currently mapped.
 *
 *          Check that INVALID_OPERATION is generated by CompressedTextureSubImage3D
 *          if a non-zero buffer object name is bound to the PIXEL_UNPACK_BUFFER
 *          target and the data would be unpacked from the buffer object such that
 *          the memory reads required would exceed the data store size.
 */
class SubImageErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	SubImageErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	SubImageErrorsTest(const SubImageErrorsTest& other);
	SubImageErrorsTest& operator=(const SubImageErrorsTest& other);

	glw::GLuint   m_to_1D_empty;
	glw::GLuint   m_to_2D_empty;
	glw::GLuint   m_to_3D_empty;
	glw::GLuint   m_to_1D;
	glw::GLuint   m_to_2D;
	glw::GLuint   m_to_3D;
	glw::GLuint   m_to_1D_compressed;
	glw::GLuint   m_to_2D_compressed;
	glw::GLuint   m_to_3D_compressed;
	glw::GLuint   m_to_rectangle_compressed;
	glw::GLuint   m_to_invalid;
	glw::GLuint   m_bo;
	glw::GLuint   m_format_invalid;
	glw::GLuint   m_type_invalid;
	glw::GLint	m_max_texture_size;
	glw::GLubyte* m_reference_compressed_1D;
	glw::GLubyte* m_reference_compressed_2D;
	glw::GLubyte* m_reference_compressed_3D;
	glw::GLubyte* m_reference_compressed_rectangle;
	glw::GLint	m_reference_compressed_1D_size;
	glw::GLint	m_reference_compressed_2D_size;
	glw::GLint	m_reference_compressed_3D_size;
	glw::GLint	m_reference_compressed_rectangle_size;
	glw::GLint	m_reference_compressed_1D_format;
	glw::GLint	m_reference_compressed_2D_format;
	glw::GLint	m_reference_compressed_3D_format;
	glw::GLint	m_reference_compressed_rectangle_format;
	glw::GLint	m_not_matching_compressed_1D_format;
	glw::GLint	m_not_matching_compressed_1D_size;
	glw::GLint	m_not_matching_compressed_2D_format;
	glw::GLint	m_not_matching_compressed_2D_size;
	glw::GLint	m_not_matching_compressed_3D_format;
	glw::GLint	m_not_matching_compressed_3D_size;

	void Prepare();
	bool Test1D();
	bool Test2D();
	bool Test3D();
	bool Test1DCompressed();
	bool Test2DCompressed();
	bool Test3DCompressed();
	void Clean();

	static const glw::GLushort s_reference[];
	static const glw::GLuint   s_reference_width;
	static const glw::GLuint   s_reference_height;
	static const glw::GLuint   s_reference_depth;
	static const glw::GLuint   s_reference_size;
	static const glw::GLenum   s_reference_internalformat;
	static const glw::GLenum   s_reference_internalformat_compressed;
	static const glw::GLenum   s_reference_format;
	static const glw::GLenum   s_reference_type;
};
/* SubImageErrorsTest class */

/** @class CopyErrorsTest
 *
 *          Check that INVALID_FRAMEBUFFER_OPERATION is generated by
 *          CopyTextureSubImage1D if the object bound to READ_FRAMEBUFFER_BINDING is
 *          not framebuffer complete.
 *
 *          Check that INVALID_OPERATION is generated by CopyTextureSubImage1D if
 *          texture is not the name of an existing texture object, or if the
 *          effective target of texture is not TEXTURE_1D.
 *
 *          Check that INVALID_VALUE is generated by CopyTextureSubImage1D if level
 *          is less than 0.
 *
 *          Check that INVALID_VALUE is generated by CopyTextureSubImage1D if
 *          xoffset<0, or (xoffset+width)>w, where w is the TEXTURE_WIDTH of the
 *          texture image being modified.
 *
 *          Check that INVALID_OPERATION is generated by CopyTextureSubImage1D if
 *          the read buffer is NONE, or the value of READ_FRAMEBUFFER_BINDING is
 *          non-zero, and: the read buffer selects an attachment that has no image
 *          attached, or the effective value of SAMPLE_BUFFERS for the read
 *          framebuffer is one.
 *
 *
 *          Check that INVALID_FRAMEBUFFER_OPERATION is generated by
 *          CopyTextureSubImage2D if the object bound to READ_FRAMEBUFFER_BINDING is
 *          not framebuffer complete.
 *
 *          Check that INVALID_OPERATION is generated by CopyTextureSubImage2D if
 *          texture is not the name of an existing texture object.
 *
 *          Check that INVALID_OPERATION is generated by CopyTextureSubImage2D if
 *          the effective target of texture does not correspond to one of the
 *          texture targets supported by the function.
 *
 *          Check that INVALID_VALUE is generated by CopyTextureSubImage2D if level
 *          is less than 0.
 *
 *          Check that INVALID_VALUE is generated by CopyTextureSubImage2D if
 *          xoffset<0, (xoffset+width)>w, yoffset<0, or (yoffset+height)>0, where w
 *          is the TEXTURE_WIDTH, h is the TEXTURE_HEIGHT and of the texture image
 *          being modified.
 *
 *          Check that INVALID_OPERATION is generated by CopyTextureSubImage2D if:
 *          the read buffer is NONE, or the value of READ_FRAMEBUFFER_BINDING is
 *          non-zero, and: the read buffer selects an attachment that has no image
 *          attached, or the effective value of SAMPLE_BUFFERS for the read
 *          framebuffer is one.
 *
 *
 *          Check that INVALID_OPERATION is generated by CopyTextureSubImage3D if
 *          the effective target is not TEXTURE_3D, TEXTURE_2D_ARRAY,
 *          TEXTURE_CUBE_MAP_ARRAY or TEXTURE_CUBE_MAP.
 *
 *          Check that INVALID_FRAMEBUFFER_OPERATION is generated by
 *          CopyTextureSubImage3D if the object bound to READ_FRAMEBUFFER_BINDING is
 *          not framebuffer complete.
 *
 *          Check that INVALID_OPERATION is generated by CopyTextureSubImage3D if
 *          texture is not the name of an existing texture object.
 *
 *          Check that INVALID_VALUE is generated by CopyTextureSubImage3D if level
 *          is less than 0.
 *
 *          Check that INVALID_VALUE is generated by CopyTextureSubImage3D if
 *          xoffset<0, (xoffset+width)>w, yoffset<0, (yoffset+height)>h, zoffset<0,
 *          or (zoffset+1)>d, where w is the TEXTURE_WIDTH, h is the TEXTURE_HEIGHT,
 *          d is the TEXTURE_DEPTH and of the texture image being modified. Note
 *          that w, h, and d include twice the border width.
 *
 *          Check that INVALID_OPERATION is generated by CopyTextureSubImage3D if:
 *          the read buffer is NONE, or the value of READ_FRAMEBUFFER_BINDING is
 *          non-zero, and: the read buffer selects an attachment that has no image
 *          attached, or the effective value of SAMPLE_BUFFERS for the read
 *          framebuffer is one.
 */
class CopyErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	CopyErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	CopyErrorsTest(const CopyErrorsTest& other);
	CopyErrorsTest& operator=(const CopyErrorsTest& other);

	glw::GLuint m_fbo;
	glw::GLuint m_fbo_ms;
	glw::GLuint m_fbo_incomplete;
	glw::GLuint m_to_src;
	glw::GLuint m_to_src_ms;
	glw::GLuint m_to_1D_dst;
	glw::GLuint m_to_2D_dst;
	glw::GLuint m_to_3D_dst;
	glw::GLuint m_to_invalid;

	void Prepare();
	bool Test1D();
	bool Test2D();
	bool Test3D();
	void Clean();

	static const glw::GLuint s_width;
	static const glw::GLuint s_height;
	static const glw::GLuint s_depth;
	static const glw::GLuint s_internalformat;
};
/* CopyErrorsTest class */

/** @class ParameterSetupErrorsTest
 *
 *      Check that INVALID_ENUM is generated by TextureParameter* if pname is
 *      not one of the accepted defined values.
 *
 *      Check that INVALID_ENUM is generated by TextureParameter* if params
 *      should have a defined constant value (based on the value of pname) and
 *       does not.
 *
 *      Check that INVALID_ENUM is generated if TextureParameter{if} is called
 *      for a non-scalar parameter (pname TEXTURE_BORDER_COLOR or
 *      TEXTURE_SWIZZLE_RGBA).
 *
 *      Check that INVALID_ENUM is generated by TextureParameter* if the
 *      effective target is either TEXTURE_2D_MULTISAMPLE or
 *      TEXTURE_2D_MULTISAMPLE_ARRAY, and pname is any of the sampler states.
 *
 *      Check that INVALID_ENUM is generated by TextureParameter* if the
 *      effective target is TEXTURE_RECTANGLE and either of pnames
 *      TEXTURE_WRAP_S or TEXTURE_WRAP_T is set to either MIRROR_CLAMP_TO_EDGE,
 *      MIRRORED_REPEAT or REPEAT.
 *
 *      Check that INVALID_ENUM is generated by TextureParameter* if the
 *      effective target is TEXTURE_RECTANGLE and pname TEXTURE_MIN_FILTER is
 *      set to a value other than NEAREST or LINEAR (no mipmap filtering is
 *      permitted).
 *
 *      Check that INVALID_OPERATION is generated by TextureParameter* if the
 *      effective target is either TEXTURE_2D_MULTISAMPLE or
 *      TEXTURE_2D_MULTISAMPLE_ARRAY, and pname TEXTURE_BASE_LEVEL is set to a
 *      value other than zero.
 *
 *      Check that INVALID_OPERATION is generated by TextureParameter* if
 *      texture is not the name of an existing texture object.
 *
 *      Check that INVALID_OPERATION is generated by TextureParameter* if the
 *      effective target is TEXTURE_RECTANGLE and pname TEXTURE_BASE_LEVEL is
 *      set to any value other than zero.
 *
 *      Check that INVALID_VALUE is generated by TextureParameter* if pname is
 *      TEXTURE_BASE_LEVEL or TEXTURE_MAX_LEVEL, and param or params is
 *      negative.
 */
class ParameterSetupErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	ParameterSetupErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	ParameterSetupErrorsTest(const ParameterSetupErrorsTest& other);
	ParameterSetupErrorsTest& operator=(const ParameterSetupErrorsTest& other);

	glw::GLuint m_to_2D;
	glw::GLuint m_to_2D_ms;
	glw::GLuint m_to_rectangle;
	glw::GLuint m_to_invalid;
	glw::GLenum m_pname_invalid;
	glw::GLenum m_depth_stencil_mode_invalid;

	void Prepare();
	bool Testf();
	bool Testi();
	bool Testfv();
	bool Testiv();
	bool TestIiv();
	bool TestIuiv();
	void Clean();

	static const glw::GLuint s_width;
	static const glw::GLuint s_height;
	static const glw::GLuint s_depth;
	static const glw::GLuint s_internalformat;
};
/* ParameterSetupErrorsTest class */

/** @class GenerateMipmapErrorsTest
 *
 *      Check that INVALID_OPERATION is generated by GenerateTextureMipmap if
 *      texture is not the name of an existing texture object.
 *
 *      Check that INVALID_OPERATION is generated by GenerateTextureMipmap if
 *      target is TEXTURE_CUBE_MAP or TEXTURE_CUBE_MAP_ARRAY, and the specified
 *      texture object is not cube complete or cube array complete,
 *      respectively.
 */
class GenerateMipmapErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	GenerateMipmapErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	GenerateMipmapErrorsTest(const GenerateMipmapErrorsTest& other);
	GenerateMipmapErrorsTest& operator=(const GenerateMipmapErrorsTest& other);

	static const glw::GLubyte s_reference_data[];
	static const glw::GLuint  s_reference_width;
	static const glw::GLuint  s_reference_height;
	static const glw::GLenum  s_reference_internalformat;
	static const glw::GLenum  s_reference_format;
	static const glw::GLenum  s_reference_type;
};
/* GenerateMipmapErrorsTest class */

/** @class BindUnitErrorsTest
 *
 *      Check that INVALID_OPERATION error is generated if texture is not zero
 *      or the name of an existing texture object.
 */
class BindUnitErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	BindUnitErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	BindUnitErrorsTest(const BindUnitErrorsTest& other);
	BindUnitErrorsTest& operator=(const BindUnitErrorsTest& other);
};
/* BindUnitErrorsTest class */

/** @class ImageQueryErrorsTest
 *
 *      Check that INVALID_OPERATION is generated by GetTextureImage if texture
 *      is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM is generated by GetTextureImage functions if
 *      resulting texture target is not an accepted value TEXTURE_1D,
 *      TEXTURE_2D, TEXTURE_3D, TEXTURE_1D_ARRAY, TEXTURE_2D_ARRAY,
 *      TEXTURE_CUBE_MAP_ARRAY, TEXTURE_RECTANGLE, and TEXTURE_CUBE_MAP.
 *
 *      Check that INVALID_OPERATION error is generated by GetTextureImage if
 *      the effective target is TEXTURE_CUBE_MAP or TEXTURE_CUBE_MAP_ARRAY, and
 *      the texture object is not cube complete or cube array complete,
 *      respectively.
 *
 *      Check that GL_INVALID_VALUE is generated if level is less than 0 or
 *      larger than the maximum allowable level.
 *
 *      Check that INVALID_VALUE error is generated if level is non-zero and the
 *      effective target is TEXTURE_RECTANGLE.
 *
 *      Check that INVALID_OPERATION error is generated if any of the following
 *      mismatches between format and the internal format of the texture image
 *      exist:
 *       -  format is a color format (one of the formats in table 8.3 whose
 *          target is the color buffer) and the base internal format of the
 *          texture image is not a color format.
 *       -  format is DEPTH_COMPONENT and the base internal format is  not
 *          DEPTH_COMPONENT or DEPTH_STENCIL
 *       -  format is DEPTH_STENCIL and the base internal format is not
 *          DEPTH_STENCIL
 *       -  format is STENCIL_INDEX and the base internal format is not
 *          STENCIL_INDEX or DEPTH_STENCIL
 *       -  format is one of the integer formats in table 8.3 and the internal
 *          format of the texture image is not integer, or format is not one of
 *          the integer formats in table 8.3 and the internal format is integer.
 *
 *      Check that INVALID_OPERATION error is generated if a pixel pack buffer
 *      object is bound and packing the texture image into the bufferâ€™s memory
 *      would exceed the size of the buffer.
 *
 *      Check that INVALID_OPERATION error is generated if a pixel pack buffer
 *      object is bound and pixels is not evenly divisible by the number of
 *      basic machine units needed to store in memory the GL data type
 *      corresponding to type (see table 8.2).
 *
 *      Check that INVALID_OPERATION error is generated by GetTextureImage if
 *      the buffer size required to store the requested data is greater than
 *      bufSize.
 *
 *
 *      Check that INVALID_OPERATION is generated by GetCompressedTextureImage
 *      if texture is not the name of an existing texture object.
 *
 *      Check that INVALID_VALUE is generated by GetCompressedTextureImage if
 *      level is less than zero or greater than the maximum number of LODs
 *      permitted by the implementation.
 *
 *      Check that INVALID_OPERATION is generated if GetCompressedTextureImage
 *      is used to retrieve a texture that is in an uncompressed internal
 *      format.
 *
 *      Check that INVALID_OPERATION is generated by GetCompressedTextureImage
 *      if a non-zero buffer object name is bound to the PIXEL_PACK_BUFFER
 *      target, the buffer storage was not initialized with BufferStorage using
 *      MAP_PERSISTENT_BIT flag, and the buffer object's data store is currently
 *      mapped.
 *
 *      Check that INVALID_OPERATION is generated by GetCompressedTextureImage
 *      if a non-zero buffer object name is bound to the PIXEL_PACK_BUFFER
 *      target and the data would be packed to the buffer object such that the
 *      memory writes required would exceed the data store size.
 */
class ImageQueryErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	ImageQueryErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	ImageQueryErrorsTest(const ImageQueryErrorsTest& other);
	ImageQueryErrorsTest& operator=(const ImageQueryErrorsTest& other);

	static const glw::GLuint s_reference_data[];
	static const glw::GLuint s_reference_width;
	static const glw::GLuint s_reference_height;
	static const glw::GLuint s_reference_size;
	static const glw::GLenum s_reference_internalformat;
	static const glw::GLenum s_reference_internalformat_int;
	static const glw::GLenum s_reference_internalformat_compressed;
	static const glw::GLenum s_reference_format;
	static const glw::GLenum s_reference_type;
};
/* ImageQueryErrorsTest class */

/** @class LevelParameterErrorsTest
 *
 *      Check that INVALID_OPERATION is generated by GetTextureLevelParameterfv
 *      and GetTextureLevelParameteriv functions if texture is not the name of
 *      an existing texture object.
 *
 *      Check that INVALID_VALUE is generated by GetTextureLevelParameter* if
 *      level is less than 0.
 *
 *      Check that INVALID_VALUE may be generated if level is greater than
 *      log2 max, where max is the returned value of MAX_TEXTURE_SIZE.
 *
 *      Check that INVALID_OPERATION is generated by GetTextureLevelParameter*
 *      if TEXTURE_COMPRESSED_IMAGE_SIZE is queried on texture images with an
 *      uncompressed internal format or on proxy targets.
 *
 *      Check that INVALID_ENUM error is generated by GetTextureLevelParameter*
 *      if pname is not one of supported constants.
 */
class LevelParameterErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	LevelParameterErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	LevelParameterErrorsTest(const LevelParameterErrorsTest& other);
	LevelParameterErrorsTest& operator=(const LevelParameterErrorsTest& other);
};
/* LevelParameterErrorsTest class */

/** @class
 *      Check that INVALID_ENUM is generated by glGetTextureParameter* if pname
 *      is not an accepted value.
 *
 *      Check that INVALID_OPERATION is generated by glGetTextureParameter* if
 *      texture is not the name of an existing texture object.
 *
 *      Check that INVALID_ENUM error is generated if the effective target is
 *      not one of the supported texture targets (eg. TEXTURE_BUFFER).
 */
class ParameterErrorsTest : public deqp::TestCase, ErrorsUtilities
{
public:
	/* Public member functions. */
	ParameterErrorsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private constructors. */
	ParameterErrorsTest(const ParameterErrorsTest& other);
	ParameterErrorsTest& operator=(const ParameterErrorsTest& other);
};
/* ParameterErrorsTest class */
} /* Textures namespace */
} /* DirectStateAccess namespace */
} /* gl4cts namespace */

#endif // _GL4CDIRECTSTATEACCESSTESTS_HPP
