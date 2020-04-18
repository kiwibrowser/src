#ifndef _GL4CSHADERDRAWPARAMETERSTESTS_HPP
#define _GL4CSHADERDRAWPARAMETERSTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \file  gl4cShaderDrawParametersTests.hpp
 * \brief Conformance tests for the GL_ARB_shader_draw_parameters functionality.
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
} SDPDrawArraysIndirectCommand;

typedef struct
{
	GLuint count;
	GLuint instanceCount;
	GLuint firstIndex;
	GLuint baseVertex;
	GLuint baseInstance;
} SDPDrawElementsIndirectCommand;

struct ResultPoint
{
	GLfloat x;
	GLfloat y;
	GLfloat red;
	GLfloat green;
	GLfloat blue;

	ResultPoint(GLfloat _x, GLfloat _y, GLfloat _red, GLfloat _green, GLfloat _blue)
		: x(_x), y(_y), red(_red), green(_green), blue(_blue)
	{
		// Left blank
	}
};

/** Test verifies if extension is available for GLSL
 **/
class ShaderDrawParametersExtensionTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	ShaderDrawParametersExtensionTestCase(deqp::Context& context);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private members */
};

/** This is base class for drawing commands tests
 **/
class ShaderDrawParametersTestBase : public deqp::TestCase
{
public:
	/* Public methods */
	ShaderDrawParametersTestBase(deqp::Context& context, const char* name, const char* description);

	virtual void				 init();
	virtual void				 deinit();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	bool draw();
	bool verify();

protected:
	/* Protected members */
	GLuint m_vao;
	GLuint m_arrayBuffer;
	GLuint m_elementBuffer;
	GLuint m_drawIndirectBuffer;
	GLuint m_parameterBuffer;

	std::vector<ResultPoint> m_resultPoints;

	/* Protected methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderDrawArraysParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderDrawArraysParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderDrawArraysParameters",
									   "Verifies shader draw parameters with DrawArrays command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderDrawElementsParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderDrawElementsParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderDrawElementsParameters",
									   "Verifies shader draw parameters with DrawElements command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderDrawArraysIndirectParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderDrawArraysIndirectParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderDrawArraysIndirectParameters",
									   "Verifies shader draw parameters with DrawArraysIndirect command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderDrawElementsIndirectParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderDrawElementsIndirectParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderDrawElementsIndirectParameters",
									   "Verifies shader draw parameters with DrawElementsIndirect command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderDrawArraysInstancedParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderDrawArraysInstancedParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderDrawArraysInstancedParameters",
									   "Verifies shader draw parameters with DrawArraysInstanced command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderDrawElementsInstancedParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderDrawElementsInstancedParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderDrawElementsInstancedParameters",
									   "Verifies shader draw parameters with DrawElementsInstanced command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderMultiDrawArraysParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderMultiDrawArraysParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderMultiDrawArraysParameters",
									   "Verifies shader draw parameters with MultiDrawArrays command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderMultiDrawElementsParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderMultiDrawElementsParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderMultiDrawElementsParameters",
									   "Verifies shader draw parameters with MultiDrawElements command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderMultiDrawArraysIndirectParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderMultiDrawArraysIndirectParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderMultiDrawArraysIndirectParameters",
									   "Verifies shader draw parameters with MultiDrawArraysIndirect command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderMultiDrawElementsIndirectParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderMultiDrawElementsIndirectParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "ShaderMultiDrawElementsIndirectParameters",
									   "Verifies shader draw parameters with MultiDrawElementsIndirect command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderMultiDrawArraysIndirectCountParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderMultiDrawArraysIndirectCountParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "MultiDrawArraysIndirectCountParameters",
									   "Verifies shader draw parameters with MultiDrawArraysIndirectCount command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

class ShaderMultiDrawElementsIndirectCountParametersTestCase : public ShaderDrawParametersTestBase
{
public:
	/* Public methods */
	ShaderMultiDrawElementsIndirectCountParametersTestCase(deqp::Context& context)
		: ShaderDrawParametersTestBase(context, "MultiDrawElementIndirectCountParameters",
									   "Verifies shader draw parameters with MultiDrawElementIndirectCount command")
	{
	}

private:
	/* Private methods */
	virtual void initChild();
	virtual void deinitChild();
	virtual void drawCommand();
};

/** Test group which encapsulates all sparse buffer conformance tests */
class ShaderDrawParametersTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	ShaderDrawParametersTests(deqp::Context& context);

	void init();

private:
	ShaderDrawParametersTests(const ShaderDrawParametersTests& other);
	ShaderDrawParametersTests& operator=(const ShaderDrawParametersTests& other);
};

} /* gl4cts namespace */

#endif // _GL4CSHADERDRAWPARAMETERSTESTS_HPP
