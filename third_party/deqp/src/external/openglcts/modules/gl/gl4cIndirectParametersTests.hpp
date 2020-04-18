#ifndef _GL4CINDIRECTPARAMETERSTESTS_HPP
#define _GL4CINDIRECTPARAMETERSTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file  gl4cIndirectParametersTests.hpp
 * \brief Conformance tests for the GL_ARB_indirect_parameters functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{

typedef struct
{
	GLuint count;
	GLuint instanceCount;
	GLuint first;
	GLuint baseInstance;
} DrawArraysIndirectCommand;

typedef struct
{
	GLuint count;
	GLuint instanceCount;
	GLuint firstIndex;
	GLuint baseVertex;
	GLuint baseInstance;
} DrawElementsIndirectCommand;

/** Test verifies if operations on new buffer object PARAMETER_BUFFER_ARB works as expected.
 **/
class ParameterBufferOperationsCase : public deqp::TestCase
{
public:
	/* Public methods */
	ParameterBufferOperationsCase(deqp::Context& context);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	/* Private members */
};

/** Base class for specific vertex array indirect drawing classes.
 **/
class VertexArrayIndirectDrawingBaseCase : public deqp::TestCase
{
public:
	/* Public methods */
	VertexArrayIndirectDrawingBaseCase(deqp::Context& context, const char* name, const char* description);

	virtual void						 init()   = DE_NULL;
	virtual void						 deinit() = DE_NULL;
	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected methods */
	virtual bool draw() = DE_NULL;
	virtual bool verify();
	virtual bool verifyErrors() = DE_NULL;
};

/** Test verifies if MultiDrawArraysIndirectCountARB function works properly.
 **/
class MultiDrawArraysIndirectCountCase : public VertexArrayIndirectDrawingBaseCase
{
public:
	/* Public methods */
	MultiDrawArraysIndirectCountCase(deqp::Context& context);

	virtual void init();
	virtual void deinit();

protected:
	/* Protected methods */
	virtual bool draw();
	virtual bool verifyErrors();

	/* Protected methods */
	GLuint m_vao;
	GLuint m_arrayBuffer;
	GLuint m_drawIndirectBuffer;
	GLuint m_parameterBuffer;
};

/** Test verifies if MultiDrawArraysIndirectCountARB function works properly.
 **/
class MultiDrawElementsIndirectCountCase : public VertexArrayIndirectDrawingBaseCase
{
public:
	/* Public methods */
	MultiDrawElementsIndirectCountCase(deqp::Context& context);

	virtual void init();
	virtual void deinit();

protected:
	/* Protected methods */
	virtual bool draw();
	virtual bool verifyErrors();

	/* Protected methods */
	GLuint m_vao;
	GLuint m_arrayBuffer;
	GLuint m_elementBuffer;
	GLuint m_drawIndirectBuffer;
	GLuint m_parameterBuffer;
};

/** Test group which encapsulates all sparse buffer conformance tests */
class IndirectParametersTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	IndirectParametersTests(deqp::Context& context);

	void init();

private:
	IndirectParametersTests(const IndirectParametersTests& other);
	IndirectParametersTests& operator=(const IndirectParametersTests& other);
};

} /* glcts namespace */

#endif // _GL4CINDIRECTPARAMETERSTESTS_HPP
