#ifndef _GL4CCONTEXTFLUSHCONTROLTESTS_HPP
#define _GL4CCONTEXTFLUSHCONTROLTESTS_HPP
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
 * \file  gl3cClipDistance.hpp
 * \brief Conformance tests for Context Flush Control feature functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

/* Includes. */
#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

namespace gl4cts
{
namespace ContextFlushControl
{
/** @class Tests
 *
 *  @brief Context Flush Control test group.
 *
 *  The test checks that functions GetIntegerv, GetFloatv, GetBooleanv, GetDoublev and
 *  GetInteger64v accept parameter name GL_CONTEXT_RELEASE_BEHAVIOR and for
 *  default context returns GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH.
 *
 *  If building target supports context flush (GLX framework which supports
 *  GLX_ARB_context_flush_control or WGL framework which supports
 *  WGL_ARB_context_flush_control) test makes new context with enabled context
 *  flush control functionality. Coverage test is repeated with this context and
 *  GL_NONE is expected to be returned.
 *
 *  For reference see KHR_context_flush_control extension.
 *
 *  The Pass result is returned when tests succeeded, Fail is returned otherwise.
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

/** @class CoverageTest
 *
 *  @brief Context Flush Control API Coverage test.
 */
class CoverageTest : public deqp::TestCase
{
public:
	/* Public member functions */
	CoverageTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CoverageTest(const CoverageTest& other);
	CoverageTest& operator=(const CoverageTest& other);

	bool testQuery(glu::RenderContext& context, glw::GLenum expected_value);
	glu::RenderContext* createNoFlushContext();
};

/** @class FunctionalTest
 *
 *  @brief Context Flush Control Functional test.
 *
 *  Test runs only if building target supports context flush (GLX framework
 *  which supports GLX_ARB_context_flush_control or WGL framework which
 *  supports WGL_ARB_context_flush_control). Test prepares 4 contexts: two with
 *  enabled context flush control and two with disabled context flush
 *  control. Next, run-time of following procedure is measured:
 *
 *      for n times do
 *          draw triangle
 *          switch context
 *
 *  The function is running using two contexts with enabled context control flush
 *  and using two contexts with disabled context control flush. It is expected that
 *  case without flush on context switch control will be faster than the with
 *  case which flushes functionality. Test sets pass if expected behavior has
 *  been measured. The quality warning is triggered when test fails. Not supported
 *  result is returned if context does not support contxt flush control.
 *
 *  The test is based on KHR_context_flush_control extension overview, that the main reason
 *  for no-flush context is to increase the performance of the implementation.
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

	glw::GLfloat testTime(bool shall_flush_on_release);

	/** @brief Draw Setup class to encapsulate context with framebuffer and shader program.
	 *
	 *  The context and within framebuffer, renderbuffer, vertex array object,
	 *  shader program is created on the construction. Using makeCurrent() one can swith to
	 *  the encapsulated context. With draw() member function one can draw full screen quad.
	 *  All objects are deallocated during object destruction.
	 *
	 *  Context will flush or will not flush on makeCurrent() switch depending on
	 *  constructor setup shall_flush_on_release.
	 */
	class DrawSetup
	{
	public:
		DrawSetup(deqp::Context& test_context, bool shall_flush_on_release);
		~DrawSetup();

		void makeCurrent();
		void draw();

	private:
		deqp::Context&		m_test_context; //!< Test main context.
		glu::RenderContext* m_context;		//!< Render context of this draw setup.
		glw::GLuint			m_fbo;			//!< OpenGL framebuffer object identifier (in m_context).
		glw::GLuint			m_rbo;			//!< OpenGL renderbuffer object identifier (in m_context).
		glw::GLuint			m_vao;			//!< OpenGL vertex array object identifier (in m_context).
		glw::GLuint			m_po;			//!< OpenGL GLSL program object identifier (in m_context).

		static const glw::GLuint s_view_size; //!< Framebuffer size (default 256).
		static const glw::GLchar s_vertex_shader
			[]; //!< Vertex shader source code (it draws quad using triangle strip depending on gl_VertexID).
		static const glw::GLchar s_fragment_shader[]; //!< Fragment shader source code (setup vec4(1.0) as a color).

		void createContext(bool shall_flush_on_release);
		void createView();
		void createGeometry();
		void createProgram();
	};
};

} /* ContextFlushControl namespace */
} /* gl4cts namespace */
#endif // _GL4CCONTEXTFLUSHCONTROLTESTS_HPP
