/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2017 The Android Open Source Project
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
 *//*!
 * \file
 * \brief GL_EXT_draw_elements_base_vertex tests.
 *//*--------------------------------------------------------------------*/

#include "es31fDrawElementsBaseVertexTests.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"
#include "sglrGLContext.hpp"
#include "glsDrawTest.hpp"
#include "gluStrUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluContextInfo.hpp"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include <string>
#include <set>

using std::vector;
using std::string;
using tcu::TestLog;

using namespace glw;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

enum TestIterationType
{
	TYPE_DRAW_COUNT,		// !< test with 1, 5, and 25 primitives
	TYPE_INSTANCE_COUNT,	// !< test with 1, 4, and 11 instances

	TYPE_LAST
};

static size_t getElementCount (gls::DrawTestSpec::Primitive primitive, size_t primitiveCount)
{
	switch (primitive)
	{
		case gls::DrawTestSpec::PRIMITIVE_POINTS:						return primitiveCount;
		case gls::DrawTestSpec::PRIMITIVE_TRIANGLES:					return primitiveCount * 3;
		case gls::DrawTestSpec::PRIMITIVE_TRIANGLE_FAN:					return primitiveCount + 2;
		case gls::DrawTestSpec::PRIMITIVE_TRIANGLE_STRIP:				return primitiveCount + 2;
		case gls::DrawTestSpec::PRIMITIVE_LINES:						return primitiveCount * 2;
		case gls::DrawTestSpec::PRIMITIVE_LINE_STRIP:					return primitiveCount + 1;
		case gls::DrawTestSpec::PRIMITIVE_LINE_LOOP:					return (primitiveCount==1) ? (2) : (primitiveCount);
		case gls::DrawTestSpec::PRIMITIVE_LINES_ADJACENCY:				return primitiveCount * 4;
		case gls::DrawTestSpec::PRIMITIVE_LINE_STRIP_ADJACENCY:			return primitiveCount + 3;
		case gls::DrawTestSpec::PRIMITIVE_TRIANGLES_ADJACENCY:			return primitiveCount * 6;
		case gls::DrawTestSpec::PRIMITIVE_TRIANGLE_STRIP_ADJACENCY:		return primitiveCount * 2 + 4;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

static void addRangeElementsToSpec (gls::DrawTestSpec& spec)
{
	if (spec.drawMethod == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_RANGED_BASEVERTEX)
	{
		spec.indexMin = 0;
		spec.indexMax = (int)getElementCount(spec.primitive, spec.primitiveCount);
	}
}

static void addTestIterations (gls::DrawTest* test, gls::DrawTestSpec& spec, TestIterationType type)
{
	if (type == TYPE_DRAW_COUNT)
	{
		spec.primitiveCount = 1;
		addRangeElementsToSpec(spec);
		test->addIteration(spec, "draw count = 1");

		spec.primitiveCount = 5;
		addRangeElementsToSpec(spec);
		test->addIteration(spec, "draw count = 5");

		spec.primitiveCount = 25;
		addRangeElementsToSpec(spec);
		test->addIteration(spec, "draw count = 25");
	}
	else if (type == TYPE_INSTANCE_COUNT)
	{
		spec.instanceCount = 1;
		addRangeElementsToSpec(spec);
		test->addIteration(spec, "instance count = 1");

		spec.instanceCount = 4;
		addRangeElementsToSpec(spec);
		test->addIteration(spec, "instance count = 4");

		spec.instanceCount = 11;
		addRangeElementsToSpec(spec);
		test->addIteration(spec, "instance count = 11");
	}
	else
		DE_ASSERT(false);
}

static void genBasicSpec (gls::DrawTestSpec& spec, gls::DrawTestSpec::DrawMethod method)
{
	spec.apiType							= glu::ApiType::es(3,1);
	spec.primitive							= gls::DrawTestSpec::PRIMITIVE_TRIANGLES;
	spec.primitiveCount						= 5;
	spec.drawMethod							= method;
	spec.indexType							= gls::DrawTestSpec::INDEXTYPE_LAST;
	spec.indexPointerOffset					= 0;
	spec.indexStorage						= gls::DrawTestSpec::STORAGE_LAST;
	spec.first								= 0;
	spec.indexMin							= 0;
	spec.indexMax							= 0;
	spec.instanceCount						= 1;
	spec.indirectOffset						= 0;

	spec.attribs.resize(2);

	spec.attribs[0].inputType				= gls::DrawTestSpec::INPUTTYPE_FLOAT;
	spec.attribs[0].outputType				= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
	spec.attribs[0].storage					= gls::DrawTestSpec::STORAGE_BUFFER;
	spec.attribs[0].usage					= gls::DrawTestSpec::USAGE_STATIC_DRAW;
	spec.attribs[0].componentCount			= 4;
	spec.attribs[0].offset					= 0;
	spec.attribs[0].stride					= 0;
	spec.attribs[0].normalize				= false;
	spec.attribs[0].instanceDivisor			= 0;
	spec.attribs[0].useDefaultAttribute		= false;

	spec.attribs[1].inputType				= gls::DrawTestSpec::INPUTTYPE_FLOAT;
	spec.attribs[1].outputType				= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
	spec.attribs[1].storage					= gls::DrawTestSpec::STORAGE_BUFFER;
	spec.attribs[1].usage					= gls::DrawTestSpec::USAGE_STATIC_DRAW;
	spec.attribs[1].componentCount			= 2;
	spec.attribs[1].offset					= 0;
	spec.attribs[1].stride					= 0;
	spec.attribs[1].normalize				= false;
	spec.attribs[1].instanceDivisor			= 0;
	spec.attribs[1].useDefaultAttribute		= false;

	addRangeElementsToSpec(spec);
}

class VertexIDCase : public TestCase
{
public:
									VertexIDCase			(Context& context, gls::DrawTestSpec::DrawMethod drawMethod);
									~VertexIDCase			(void);

	void							init					(void);
	void							deinit					(void);
	IterateResult					iterate					(void);

	void							draw					(GLenum mode, GLsizei count, GLenum type, GLvoid* indices, GLint baseVertex);
	void							verifyImage				(const tcu::Surface& image);

private:
	const glw::Functions&			m_gl;
	glu::ShaderProgram*				m_program;
	GLuint							m_coordinatesBuffer;
	GLuint							m_elementsBuffer;
	int								m_iterNdx;
	gls::DrawTestSpec::DrawMethod	m_method;

	enum
	{
		VIEWPORT_WIDTH = 64,
		VIEWPORT_HEIGHT = 64
	};

	enum
	{
		MAX_VERTICES = 2*3	//!< 2 triangles, totals 6 vertices
	};
};

VertexIDCase::VertexIDCase (Context& context,  gls::DrawTestSpec::DrawMethod drawMethod)
	: TestCase				(context, "vertex_id", "gl_VertexID Test")
	, m_gl					(m_context.getRenderContext().getFunctions())
	, m_program				(DE_NULL)
	, m_coordinatesBuffer	(0)
	, m_elementsBuffer		(0)
	, m_iterNdx				(0)
	, m_method				(drawMethod)
{
}

VertexIDCase::~VertexIDCase (void)
{
	VertexIDCase::deinit();
}

void VertexIDCase::init (void)
{
	if (m_method == deqp::gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_BASEVERTEX ||
		m_method == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_RANGED_BASEVERTEX ||
		m_method == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_INSTANCED_BASEVERTEX)
	{
		const bool supportsES32 = contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
		TCU_CHECK_AND_THROW(NotSupportedError, supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_draw_elements_base_vertex"), "GL_EXT_draw_elements_base_vertex is not supported.");
	}

	m_testCtx.getLog()	<< TestLog::Message
						<< "gl_VertexID should be the index of the vertex that is being passed to the shader. i.e. indices[i] + basevertex"
						<< TestLog::EndMessage;

	DE_ASSERT(!m_program);
	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(
		"#version 310 es\n"
		"in highp vec4 a_position;\n"
		"out mediump vec4 v_color;\n"
		"uniform highp vec4 u_colors[8];\n"
		"void main (void)\n"
		"{\n"
		"	gl_Position = a_position;\n"
		"	v_color = u_colors[gl_VertexID];\n"
		"}\n",

		"#version 310 es\n"
		"in mediump vec4 v_color;\n"
		"layout(location = 0) out mediump vec4 o_color;\n"
		"void main (void)\n"
		"{\n"
		"	o_color = v_color;\n"
		"}\n"));

	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
	{
		delete m_program;
		m_program = DE_NULL;
		TCU_FAIL("Failed to compile shader program");
	}

	GLU_CHECK_GLW_CALL(m_gl, useProgram(m_program->getProgram()));

	GLU_CHECK_GLW_CALL(m_gl, genBuffers(1, &m_coordinatesBuffer));
	GLU_CHECK_GLW_CALL(m_gl, genBuffers(1, &m_elementsBuffer));
}

void VertexIDCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;

	if (m_elementsBuffer)
	{
		GLU_CHECK_GLW_CALL(m_gl, deleteBuffers(1, &m_elementsBuffer));
		m_elementsBuffer = 0;
	}

	if (m_coordinatesBuffer)
	{
		GLU_CHECK_GLW_CALL(m_gl, deleteBuffers(1, &m_coordinatesBuffer));
		m_coordinatesBuffer = 0;
	}
}

void VertexIDCase::draw (GLenum mode, GLsizei count, GLenum type, GLvoid* indices, GLint baseVertex)
{
	switch (m_method)
	{
		case gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_BASEVERTEX:
			GLU_CHECK_GLW_CALL(m_gl, drawElementsBaseVertex(mode, count, type, indices, baseVertex));
			break;

		case gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_RANGED_BASEVERTEX:
		{
			GLint maxElementsVertices = 0;
			GLU_CHECK_GLW_CALL(m_gl, getIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxElementsVertices));
			GLU_CHECK_GLW_CALL(m_gl, drawRangeElementsBaseVertex(mode, 0, maxElementsVertices, count, type, indices, baseVertex));
			break;
		}

		case gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_INSTANCED_BASEVERTEX:
				GLU_CHECK_GLW_CALL(m_gl, drawElementsInstancedBaseVertex(mode, count, type, indices, 1, baseVertex));
				break;

		default:
			DE_FATAL("Draw method not supported");
	}
}

void VertexIDCase::verifyImage (const tcu::Surface& image)
{
	tcu::TestLog&	log				= m_testCtx.getLog();
	bool			isOk			= true;

	const int		colorThreshold	= 0; // expect perfect match
	tcu::Surface	error			(image.getWidth(), image.getHeight());

	for (int y = 0; y < image.getHeight(); y++)
	for (int x = 0; x < image.getWidth(); x++)
	{
		const tcu::RGBA pixel = image.getPixel(x, y);
		bool pixelOk = true;

		// Ignore pixels not drawn with basevertex
		if ((x < image.getWidth()* 1/4) || (x > image.getWidth()  * 3/4)
			|| (y < image.getHeight() * 1/4) || (y > image.getHeight() * 3/4))
			continue;

		// Any pixel with !(B ~= 255) is faulty
		if (de::abs(pixel.getBlue() - 255) > colorThreshold)
			pixelOk = false;

		error.setPixel(x, y, (pixelOk) ? (tcu::RGBA(0, 255, 0, 255)) : (tcu::RGBA(255, 0, 0, 255)));
		isOk = isOk && pixelOk;
	}

	if (!isOk)
	{
		log << TestLog::Message << "Image verification failed." << TestLog::EndMessage;
		log << TestLog::ImageSet("Verification result", "Result of rendering")
			<< TestLog::Image("Result",		"Result",		image)
			<< TestLog::Image("Error Mask",	"Error mask",	error)
			<< TestLog::EndImageSet;
	}
	else
	{
		log << TestLog::ImageSet("Verification result", "Result of rendering")
			<< TestLog::Image("Result", "Result", image)
			<< TestLog::EndImageSet;
	}

	if (isOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result image invalid");
}

VertexIDCase::IterateResult VertexIDCase::iterate (void)
{
	const GLuint			drawCount			= 6;
	const GLuint			baseVertex			= 4;
	const GLuint			coordLocation		= m_gl.getAttribLocation(m_program->getProgram(), "a_position");
	const GLuint			colorLocation		= m_gl.getUniformLocation(m_program->getProgram(), "u_colors[0]");

	tcu::Surface			surface(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	const GLfloat coords[] =
	{
		// full viewport quad
		-1.0f, -1.0f,
		+1.0f, -1.0f,
		+1.0f, +1.0f,
		-1.0f, +1.0f,

		// half viewport quad centred
		-0.5f, -0.5f,
		+0.5f, -0.5f,
		+0.5f, +0.5f,
		-0.5f, +0.5f,
	};

	const GLushort indices[] =
	{
		0, 1, 2, 2, 3, 0,
	};

	const GLfloat colors[] =
	{
		0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, 1.0f, 0.5f, 1.0f,
		0.0f, 0.5f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,

		0.0f, 0.0f, 1.0f, 1.0f, // blue
		0.0f, 0.0f, 1.0f, 1.0f, // blue
		0.0f, 0.0f, 1.0f, 1.0f, // blue
		0.0f, 0.0f, 1.0f, 1.0f, // blue
	};

	GLU_CHECK_GLW_CALL(m_gl, viewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT));
	GLU_CHECK_GLW_CALL(m_gl, clearColor(1.0f, 1.0f, 1.0f, 1.0f)); // white
	GLU_CHECK_GLW_CALL(m_gl, clear(GL_COLOR_BUFFER_BIT));

	GLU_CHECK_GLW_CALL(m_gl, uniform4fv(colorLocation, DE_LENGTH_OF_ARRAY(colors), &colors[0]));

	GLU_CHECK_GLW_CALL(m_gl, bindBuffer(GL_ARRAY_BUFFER, m_coordinatesBuffer));
	GLU_CHECK_GLW_CALL(m_gl, bufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(coords), coords, GL_STATIC_DRAW));
	GLU_CHECK_GLW_CALL(m_gl, enableVertexAttribArray(coordLocation));
	GLU_CHECK_GLW_CALL(m_gl, vertexAttribPointer(coordLocation, 2, GL_FLOAT, GL_FALSE, 0, DE_NULL));

	if (m_iterNdx == 0)
	{
		tcu::ScopedLogSection	logSection	(m_testCtx.getLog(), "Iter0", "Indices in client-side array");
		draw(GL_TRIANGLES, drawCount, GL_UNSIGNED_SHORT, (GLvoid*)indices, baseVertex);
	}

	if (m_iterNdx == 1)
	{
		tcu::ScopedLogSection	logSection	(m_testCtx.getLog(), "Iter1", "Indices in element array buffer");
		GLU_CHECK_GLW_CALL(m_gl, bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementsBuffer));
		GLU_CHECK_GLW_CALL(m_gl, bufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)sizeof(indices), &indices[0], GL_STATIC_DRAW));
		draw(GL_TRIANGLES, drawCount, GL_UNSIGNED_SHORT, DE_NULL, baseVertex);
	}

	glu::readPixels(m_context.getRenderContext(), 0, 0, surface.getAccess());
	verifyImage(surface);

	m_iterNdx += 1;

	return (m_iterNdx < 2) ? CONTINUE : STOP;
}

class BuiltInVariableGroup : public TestCaseGroup
{
public:
									BuiltInVariableGroup		(Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod);
									~BuiltInVariableGroup		(void);

	void							init						(void);

private:
	gls::DrawTestSpec::DrawMethod	m_method;
};

BuiltInVariableGroup::BuiltInVariableGroup (Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod)
	: TestCaseGroup		(context, name, descr)
	, m_method			(drawMethod)
{
}

BuiltInVariableGroup::~BuiltInVariableGroup (void)
{
}

void BuiltInVariableGroup::init (void)
{
	addChild(new VertexIDCase(m_context, m_method));
}

class IndexGroup : public TestCaseGroup
{
public:
									IndexGroup		(Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod);
									~IndexGroup		(void);

	void							init			(void);

private:
	gls::DrawTestSpec::DrawMethod	m_method;
};

IndexGroup::IndexGroup (Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod)
	: TestCaseGroup		(context, name, descr)
	, m_method			(drawMethod)
{
}

IndexGroup::~IndexGroup (void)
{
}

void IndexGroup::init (void)
{
	struct IndexTest
	{
		gls::DrawTestSpec::IndexType	type;
		int								offsets[3];
	};

	const IndexTest tests[] =
	{
		{ gls::DrawTestSpec::INDEXTYPE_BYTE,	{ 0, 1, -1 } },
		{ gls::DrawTestSpec::INDEXTYPE_SHORT,	{ 0, 2, -1 } },
		{ gls::DrawTestSpec::INDEXTYPE_INT,		{ 0, 4, -1 } },
	};

	gls::DrawTestSpec spec;
	genBasicSpec(spec, m_method);

	spec.indexStorage = gls::DrawTestSpec::STORAGE_BUFFER;

	for (int testNdx = 0; testNdx < DE_LENGTH_OF_ARRAY(tests); ++testNdx)
	{
		const IndexTest&	indexTest	= tests[testNdx];

		const std::string	name		= std::string("index_") + gls::DrawTestSpec::indexTypeToString(indexTest.type);
		const std::string	desc		= std::string("index ") + gls::DrawTestSpec::indexTypeToString(indexTest.type);
		gls::DrawTest*		test		= new gls::DrawTest(m_testCtx, m_context.getRenderContext(), name.c_str(), desc.c_str());

		spec.indexType			= indexTest.type;

		for (int iterationNdx = 0; iterationNdx < DE_LENGTH_OF_ARRAY(indexTest.offsets) && indexTest.offsets[iterationNdx] != -1; ++iterationNdx)
		{
			const std::string iterationDesc = std::string("first vertex ") + de::toString(indexTest.offsets[iterationNdx] / gls::DrawTestSpec::indexTypeSize(indexTest.type));
			spec.indexPointerOffset	= indexTest.offsets[iterationNdx];
			test->addIteration(spec, iterationDesc.c_str());
		}

		addChild(test);
	}
}

class BaseVertexGroup : public TestCaseGroup
{
public:
									BaseVertexGroup		(Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod);
									~BaseVertexGroup	(void);

	void							init				(void);

private:
	gls::DrawTestSpec::DrawMethod	m_method;
};

BaseVertexGroup::BaseVertexGroup (Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod)
	: TestCaseGroup		(context, name, descr)
	, m_method			(drawMethod)
{
}

BaseVertexGroup::~BaseVertexGroup (void)
{
}

void BaseVertexGroup::init (void)
{
	struct IndexTest
	{
		bool							positiveBase;
		gls::DrawTestSpec::IndexType	type;
		int								baseVertex[2];
	};

	const IndexTest tests[] =
	{
		{ true,  gls::DrawTestSpec::INDEXTYPE_BYTE,		{  1,  2 } },
		{ true,  gls::DrawTestSpec::INDEXTYPE_SHORT,	{  1,  2 } },
		{ true,  gls::DrawTestSpec::INDEXTYPE_INT,		{  1,  2 } },
		{ false, gls::DrawTestSpec::INDEXTYPE_BYTE,		{ -1, -2 } },
		{ false, gls::DrawTestSpec::INDEXTYPE_SHORT,	{ -1, -2 } },
		{ false, gls::DrawTestSpec::INDEXTYPE_INT,		{ -1, -2 } },
	};

	gls::DrawTestSpec spec;
	genBasicSpec(spec, m_method);

	spec.indexStorage = gls::DrawTestSpec::STORAGE_BUFFER;

	for (int testNdx = 0; testNdx < DE_LENGTH_OF_ARRAY(tests); ++testNdx)
	{
		const IndexTest&	indexTest	= tests[testNdx];

		const std::string	name		= std::string("index_") + (indexTest.positiveBase ? "" : "neg_") + gls::DrawTestSpec::indexTypeToString(indexTest.type);
		const std::string	desc		= std::string("index ") + gls::DrawTestSpec::indexTypeToString(indexTest.type);
		gls::DrawTest*		test		= new gls::DrawTest(m_testCtx, m_context.getRenderContext(), name.c_str(), desc.c_str());

		spec.indexType			= indexTest.type;

		for (int iterationNdx = 0; iterationNdx < DE_LENGTH_OF_ARRAY(indexTest.baseVertex); ++iterationNdx)
		{
			const std::string iterationDesc = std::string("base vertex ") + de::toString(indexTest.baseVertex[iterationNdx]);
			spec.baseVertex	= indexTest.baseVertex[iterationNdx];
			// spec.indexMin + spec.baseVertex can not be a negative value
			if (spec.indexMin + spec.baseVertex < 0)
			{
				spec.indexMax -= (spec.indexMin + spec.baseVertex);
				spec.indexMin -= (spec.indexMin + spec.baseVertex);
			}
			test->addIteration(spec, iterationDesc.c_str());
		}

		addChild(test);
	}
}

class AttributeGroup : public TestCaseGroup
{
public:
									AttributeGroup	(Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod, gls::DrawTestSpec::Primitive primitive, gls::DrawTestSpec::IndexType indexType, gls::DrawTestSpec::Storage indexStorage);
									~AttributeGroup	(void);

	void							init			(void);

private:
	gls::DrawTestSpec::DrawMethod	m_method;
	gls::DrawTestSpec::Primitive	m_primitive;
	gls::DrawTestSpec::IndexType	m_indexType;
	gls::DrawTestSpec::Storage		m_indexStorage;
};

AttributeGroup::AttributeGroup (Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod, gls::DrawTestSpec::Primitive primitive, gls::DrawTestSpec::IndexType indexType, gls::DrawTestSpec::Storage indexStorage)
	: TestCaseGroup		(context, name, descr)
	, m_method			(drawMethod)
	, m_primitive		(primitive)
	, m_indexType		(indexType)
	, m_indexStorage	(indexStorage)
{
}

AttributeGroup::~AttributeGroup (void)
{
}

void AttributeGroup::init (void)
{
	// Single attribute
	{
		gls::DrawTest*		test				= new gls::DrawTest(m_testCtx, m_context.getRenderContext(), "single_attribute", "Single attribute array.");
		gls::DrawTestSpec	spec;

		spec.apiType							= glu::ApiType::es(3,1);
		spec.primitive							= m_primitive;
		spec.primitiveCount						= 5;
		spec.drawMethod							= m_method;
		spec.indexType							= m_indexType;
		spec.indexPointerOffset					= 0;
		spec.indexStorage						= m_indexStorage;
		spec.first								= 0;
		spec.indexMin							= 0;
		spec.indexMax							= 0;
		spec.instanceCount						= 1;
		spec.indirectOffset						= 0;

		spec.attribs.resize(1);

		spec.attribs[0].inputType				= gls::DrawTestSpec::INPUTTYPE_FLOAT;
		spec.attribs[0].outputType				= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
		spec.attribs[0].storage					= gls::DrawTestSpec::STORAGE_BUFFER;
		spec.attribs[0].usage					= gls::DrawTestSpec::USAGE_STATIC_DRAW;
		spec.attribs[0].componentCount			= 2;
		spec.attribs[0].offset					= 0;
		spec.attribs[0].stride					= 0;
		spec.attribs[0].normalize				= false;
		spec.attribs[0].instanceDivisor			= 0;
		spec.attribs[0].useDefaultAttribute		= false;

		addTestIterations(test, spec, TYPE_DRAW_COUNT);

		this->addChild(test);
	}

	// Multiple attribute
	{
		gls::DrawTest*		test				= new gls::DrawTest(m_testCtx, m_context.getRenderContext(), "multiple_attributes", "Multiple attribute arrays.");
		gls::DrawTestSpec	spec;

		spec.apiType							= glu::ApiType::es(3,1);
		spec.primitive							= m_primitive;
		spec.primitiveCount						= 5;
		spec.drawMethod							= m_method;
		spec.indexType							= m_indexType;
		spec.indexPointerOffset					= 0;
		spec.indexStorage						= m_indexStorage;
		spec.first								= 0;
		spec.indexMin							= 0;
		spec.indexMax							= 0;
		spec.instanceCount						= 1;
		spec.indirectOffset						= 0;

		spec.attribs.resize(2);

		spec.attribs[0].inputType				= gls::DrawTestSpec::INPUTTYPE_FLOAT;
		spec.attribs[0].outputType				= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
		spec.attribs[0].storage					= gls::DrawTestSpec::STORAGE_BUFFER;
		spec.attribs[0].usage					= gls::DrawTestSpec::USAGE_STATIC_DRAW;
		spec.attribs[0].componentCount			= 4;
		spec.attribs[0].offset					= 0;
		spec.attribs[0].stride					= 0;
		spec.attribs[0].normalize				= false;
		spec.attribs[0].instanceDivisor			= 0;
		spec.attribs[0].useDefaultAttribute		= false;

		spec.attribs[1].inputType				= gls::DrawTestSpec::INPUTTYPE_FLOAT;
		spec.attribs[1].outputType				= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
		spec.attribs[1].storage					= gls::DrawTestSpec::STORAGE_BUFFER;
		spec.attribs[1].usage					= gls::DrawTestSpec::USAGE_STATIC_DRAW;
		spec.attribs[1].componentCount			= 2;
		spec.attribs[1].offset					= 0;
		spec.attribs[1].stride					= 0;
		spec.attribs[1].normalize				= false;
		spec.attribs[1].instanceDivisor			= 0;
		spec.attribs[1].useDefaultAttribute		= false;

		addTestIterations(test, spec, TYPE_DRAW_COUNT);

		this->addChild(test);
	}

	// Multiple attribute, second one divided
	{
		gls::DrawTest*		test					= new gls::DrawTest(m_testCtx, m_context.getRenderContext(), "instanced_attributes", "Instanced attribute array.");
		gls::DrawTestSpec	spec;

		spec.apiType								= glu::ApiType::es(3,1);
		spec.primitive								= m_primitive;
		spec.primitiveCount							= 5;
		spec.drawMethod								= m_method;
		spec.indexType								= m_indexType;
		spec.indexPointerOffset						= 0;
		spec.indexStorage							= m_indexStorage;
		spec.first									= 0;
		spec.indexMin								= 0;
		spec.indexMax								= 0;
		spec.instanceCount							= 1;
		spec.indirectOffset							= 0;

		spec.attribs.resize(3);

		spec.attribs[0].inputType					= gls::DrawTestSpec::INPUTTYPE_FLOAT;
		spec.attribs[0].outputType					= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
		spec.attribs[0].storage						= gls::DrawTestSpec::STORAGE_BUFFER;
		spec.attribs[0].usage						= gls::DrawTestSpec::USAGE_STATIC_DRAW;
		spec.attribs[0].componentCount				= 4;
		spec.attribs[0].offset						= 0;
		spec.attribs[0].stride						= 0;
		spec.attribs[0].normalize					= false;
		spec.attribs[0].instanceDivisor				= 0;
		spec.attribs[0].useDefaultAttribute			= false;

		// Add another position component so the instances wont be drawn on each other
		spec.attribs[1].inputType					= gls::DrawTestSpec::INPUTTYPE_FLOAT;
		spec.attribs[1].outputType					= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
		spec.attribs[1].storage						= gls::DrawTestSpec::STORAGE_BUFFER;
		spec.attribs[1].usage						= gls::DrawTestSpec::USAGE_STATIC_DRAW;
		spec.attribs[1].componentCount				= 2;
		spec.attribs[1].offset						= 0;
		spec.attribs[1].stride						= 0;
		spec.attribs[1].normalize					= false;
		spec.attribs[1].instanceDivisor				= 1;
		spec.attribs[1].useDefaultAttribute			= false;
		spec.attribs[1].additionalPositionAttribute	= true;

		// Instanced color
		spec.attribs[2].inputType					= gls::DrawTestSpec::INPUTTYPE_FLOAT;
		spec.attribs[2].outputType					= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
		spec.attribs[2].storage						= gls::DrawTestSpec::STORAGE_BUFFER;
		spec.attribs[2].usage						= gls::DrawTestSpec::USAGE_STATIC_DRAW;
		spec.attribs[2].componentCount				= 3;
		spec.attribs[2].offset						= 0;
		spec.attribs[2].stride						= 0;
		spec.attribs[2].normalize					= false;
		spec.attribs[2].instanceDivisor				= 1;
		spec.attribs[2].useDefaultAttribute			= false;

		addTestIterations(test, spec, TYPE_INSTANCE_COUNT);

		this->addChild(test);
	}

	// Multiple attribute, second one default
	{
		gls::DrawTest*		test				= new gls::DrawTest(m_testCtx, m_context.getRenderContext(), "default_attribute", "Attribute specified with glVertexAttrib*.");
		gls::DrawTestSpec	spec;

		spec.apiType							= glu::ApiType::es(3,1);
		spec.primitive							= m_primitive;
		spec.primitiveCount						= 5;
		spec.drawMethod							= m_method;
		spec.indexType							= m_indexType;
		spec.indexPointerOffset					= 0;
		spec.indexStorage						= m_indexStorage;
		spec.first								= 0;
		spec.indexMin							= 0;
		spec.indexMax							= 0;
		spec.instanceCount						= 1;
		spec.indirectOffset						= 0;

		spec.attribs.resize(2);

		spec.attribs[0].inputType				= gls::DrawTestSpec::INPUTTYPE_FLOAT;
		spec.attribs[0].outputType				= gls::DrawTestSpec::OUTPUTTYPE_VEC2;
		spec.attribs[0].storage					= gls::DrawTestSpec::STORAGE_BUFFER;
		spec.attribs[0].usage					= gls::DrawTestSpec::USAGE_STATIC_DRAW;
		spec.attribs[0].componentCount			= 2;
		spec.attribs[0].offset					= 0;
		spec.attribs[0].stride					= 0;
		spec.attribs[0].normalize				= false;
		spec.attribs[0].instanceDivisor			= 0;
		spec.attribs[0].useDefaultAttribute		= false;

		struct IOPair
		{
			gls::DrawTestSpec::InputType  input;
			gls::DrawTestSpec::OutputType output;
			int							  componentCount;
		} iopairs[] =
		{
			{ gls::DrawTestSpec::INPUTTYPE_FLOAT,		 gls::DrawTestSpec::OUTPUTTYPE_VEC2,  4 },
			{ gls::DrawTestSpec::INPUTTYPE_FLOAT,		 gls::DrawTestSpec::OUTPUTTYPE_VEC4,  2 },
			{ gls::DrawTestSpec::INPUTTYPE_INT,			 gls::DrawTestSpec::OUTPUTTYPE_IVEC3, 4 },
			{ gls::DrawTestSpec::INPUTTYPE_UNSIGNED_INT, gls::DrawTestSpec::OUTPUTTYPE_UVEC2, 4 },
		};

		for (int ioNdx = 0; ioNdx < DE_LENGTH_OF_ARRAY(iopairs); ++ioNdx)
		{
			const std::string desc = gls::DrawTestSpec::inputTypeToString(iopairs[ioNdx].input) + de::toString(iopairs[ioNdx].componentCount) + " to " + gls::DrawTestSpec::outputTypeToString(iopairs[ioNdx].output);

			spec.attribs[1].inputType			= iopairs[ioNdx].input;
			spec.attribs[1].outputType			= iopairs[ioNdx].output;
			spec.attribs[1].storage				= gls::DrawTestSpec::STORAGE_BUFFER;
			spec.attribs[1].usage				= gls::DrawTestSpec::USAGE_STATIC_DRAW;
			spec.attribs[1].componentCount		= iopairs[ioNdx].componentCount;
			spec.attribs[1].offset				= 0;
			spec.attribs[1].stride				= 0;
			spec.attribs[1].normalize			= false;
			spec.attribs[1].instanceDivisor		= 0;
			spec.attribs[1].useDefaultAttribute	= true;

			test->addIteration(spec, desc.c_str());
		}

		this->addChild(test);
	}
}

class MethodGroup : public TestCaseGroup
{
public:
									MethodGroup			(Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod);
									~MethodGroup		(void);

	void							init				(void);

private:
	gls::DrawTestSpec::DrawMethod	m_method;
};

MethodGroup::MethodGroup (Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod)
	: TestCaseGroup		(context, name, descr)
	, m_method			(drawMethod)
{
}

MethodGroup::~MethodGroup (void)
{
}

void MethodGroup::init (void)
{
	const bool indexed		=	(m_method == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_BASEVERTEX)
							||	(m_method == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_RANGED_BASEVERTEX)
							||	(m_method == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_INSTANCED_BASEVERTEX);

	const gls::DrawTestSpec::Primitive primitive[] =
	{
		gls::DrawTestSpec::PRIMITIVE_POINTS,
		gls::DrawTestSpec::PRIMITIVE_TRIANGLES,
		gls::DrawTestSpec::PRIMITIVE_TRIANGLE_FAN,
		gls::DrawTestSpec::PRIMITIVE_TRIANGLE_STRIP,
		gls::DrawTestSpec::PRIMITIVE_LINES,
		gls::DrawTestSpec::PRIMITIVE_LINE_STRIP,
		gls::DrawTestSpec::PRIMITIVE_LINE_LOOP
	};

	if (indexed)
	{
		// Index-tests
		this->addChild(new IndexGroup(m_context, "indices", "Index tests", m_method));
		this->addChild(new BaseVertexGroup(m_context, "base_vertex", "Base vertex tests", m_method));
		this->addChild(new BuiltInVariableGroup(m_context, "builtin_variable", "Built in shader variable tests", m_method));
	}

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(primitive); ++ndx)
	{
		const std::string name = gls::DrawTestSpec::primitiveToString(primitive[ndx]);
		const std::string desc = gls::DrawTestSpec::primitiveToString(primitive[ndx]);

		this->addChild(new AttributeGroup(m_context, name.c_str(), desc.c_str(), m_method, primitive[ndx], gls::DrawTestSpec::INDEXTYPE_SHORT, gls::DrawTestSpec::STORAGE_BUFFER));
	}
}

} // anonymous

DrawElementsBaseVertexTests::DrawElementsBaseVertexTests (Context& context)
	: TestCaseGroup(context, "draw_base_vertex", "Base vertex extension drawing tests")
{
}

DrawElementsBaseVertexTests::~DrawElementsBaseVertexTests (void)
{
}

void DrawElementsBaseVertexTests::init (void)
{
	const gls::DrawTestSpec::DrawMethod basicMethods[] =
	{
		gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_BASEVERTEX,
		gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_RANGED_BASEVERTEX,
		gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_INSTANCED_BASEVERTEX,
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(basicMethods); ++ndx)
	{
		const std::string name = gls::DrawTestSpec::drawMethodToString(basicMethods[ndx]);
		const std::string desc = gls::DrawTestSpec::drawMethodToString(basicMethods[ndx]);

		this->addChild(new MethodGroup(m_context, name.c_str(), desc.c_str(), basicMethods[ndx]));
	}
}

} // Functional
} // gles31
} // deqp
