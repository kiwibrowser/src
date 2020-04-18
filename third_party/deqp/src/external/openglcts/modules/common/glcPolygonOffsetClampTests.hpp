#ifndef _GLCPOLYGONOFFSETCLAMPTESTS_HPP
#define _GLCPOLYGONOFFSETCLAMPTESTS_HPP
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
* \file  glcPolygonOffsetClampTests.hpp
* \brief Conformance tests for the EXT_polygon_offset_clamp functionality.
*/ /*-------------------------------------------------------------------*/

#include "esextcTestCaseBase.hpp"
#include "glcTestCase.hpp"
#include "gluShaderProgram.hpp"

#include <string>

using namespace glw;

namespace glcts
{

struct PolygonOffsetClampValues
{
	GLfloat factor;
	GLfloat units;
	GLfloat clamp;

	PolygonOffsetClampValues(GLfloat _f, GLfloat _u, GLfloat _c) : factor(_f), units(_u), clamp(_c)
	{
	}
};

/** Tests base class
**/
class PolygonOffsetClampTestCaseBase : public deqp::TestCase
{
public:
	/* Public methods */
	PolygonOffsetClampTestCaseBase(deqp::Context& context, const char* name, const char* description);

	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected methods */
	virtual void test(const glw::Functions& gl) = DE_NULL;

	/* Protected members */
	bool m_extensionSupported;
};

/** Test verifies if polygon offset clamp works as expected for non-zero, finite clamp values
**/
class PolygonOffsetClampAvailabilityTestCase : public PolygonOffsetClampTestCaseBase
{
public:
	/* Public methods */
	PolygonOffsetClampAvailabilityTestCase(deqp::Context& context);

protected:
	/* Protected methods */
	void test(const glw::Functions& gl);
};

/** Base class for polygon offset clamp depth values verifying
**/
class PolygonOffsetClampValueTestCaseBase : public PolygonOffsetClampTestCaseBase
{
public:
	/* Public methods */
	PolygonOffsetClampValueTestCaseBase(deqp::Context& context, const char* name, const char* description);

	virtual void init();
	virtual void deinit();

protected:
	/* Protected members */
	GLuint m_fbo;
	GLuint m_depthBuf;
	GLuint m_colorBuf;
	GLuint m_fboReadback;
	GLuint m_colorBufReadback;

	std::vector<PolygonOffsetClampValues> m_testValues;

	/* Protected methods */
	void test(const glw::Functions& gl);

	float readDepthValue(const glw::Functions& gl, const GLuint readDepthProgramId);

	virtual bool verify(GLuint caseNo, GLfloat depth, GLfloat offsetDepth, GLfloat offsetClampDepth) = DE_NULL;
};

/** Test verifies if polygon offset clamp works as expected for zero and infinite clamp values
**/
class PolygonOffsetClampMinMaxTestCase : public PolygonOffsetClampValueTestCaseBase
{
public:
	/* Public methods */
	PolygonOffsetClampMinMaxTestCase(deqp::Context& context);

	void init();

protected:
	/* Protected methods */
	bool verify(GLuint caseNo, GLfloat depth, GLfloat offsetDepth, GLfloat offsetClampDepth);
};

/** Test verifies if polygon offset clamp works as expected for zero and infinite clamp values
**/
class PolygonOffsetClampZeroInfinityTestCase : public PolygonOffsetClampValueTestCaseBase
{
public:
	/* Public methods */
	PolygonOffsetClampZeroInfinityTestCase(deqp::Context& context);

	void init();

protected:
	/* Protected methods */
	bool verify(GLuint caseNo, GLfloat depth, GLfloat offsetDepth, GLfloat offsetClampDepth);
};

/** Test group which encapsulates all ARB_shader_group_vote conformance tests */
class PolygonOffsetClamp : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	PolygonOffsetClamp(deqp::Context& context);

	void init();

private:
	PolygonOffsetClamp(const PolygonOffsetClamp& other);
};

} /* glcts namespace */

#endif // _GLCPOLYGONOFFSETCLAMPTESTS_HPP
