/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Drawing tests.
 *//*--------------------------------------------------------------------*/

#include "es31fDrawTests.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deMemory.h"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"
#include "sglrGLContext.hpp"
#include "glsDrawTest.hpp"
#include "gluStrUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluCallLogWrapper.hpp"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include <set>

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

static const char* s_commonVertexShaderSource =		"#version 310 es\n"
													"in highp vec4 a_position;\n"
													"void main (void)\n"
													"{\n"
													"	gl_Position = a_position;\n"
													"}\n";
static const char* s_commonFragmentShaderSource	=	"#version 310 es\n"
													"layout(location = 0) out highp vec4 fragColor;\n"
													"void main (void)\n"
													"{\n"
													"	fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
													"}\n";

static const char* s_colorVertexShaderSource =		"#version 310 es\n"
													"in highp vec4 a_position;\n"
													"in highp vec4 a_color;\n"
													"out highp vec4 v_color;\n"
													"void main (void)\n"
													"{\n"
													"	gl_Position = a_position;\n"
													"	v_color = a_color;\n"
													"}\n";
static const char* s_colorFragmentShaderSource	=	"#version 310 es\n"
													"layout(location = 0) out highp vec4 fragColor;\n"
													"in highp vec4 v_color;\n"
													"void main (void)\n"
													"{\n"
													"	fragColor = v_color;\n"
													"}\n";
struct DrawElementsCommand
{
	deUint32 count;
	deUint32 primCount;
	deUint32 firstIndex;
	deInt32  baseVertex;
	deUint32 reservedMustBeZero;
};
DE_STATIC_ASSERT(5 * sizeof(deUint32) == sizeof(DrawElementsCommand)); // tight packing

struct DrawArraysCommand
{
	deUint32 count;
	deUint32 primCount;
	deUint32 first;
	deUint32 reservedMustBeZero;
};
DE_STATIC_ASSERT(4 * sizeof(deUint32) == sizeof(DrawArraysCommand)); // tight packing

// Verifies image contains only yellow or greeen, or a linear combination
// of these colors.
static bool verifyImageYellowGreen (const tcu::Surface& image, tcu::TestLog& log)
{
	using tcu::TestLog;

	const int colorThreshold	= 20;

	tcu::Surface error			(image.getWidth(), image.getHeight());
	bool isOk					= true;

	for (int y = 0; y < image.getHeight(); y++)
	for (int x = 0; x < image.getWidth(); x++)
	{
		const tcu::RGBA pixel = image.getPixel(x, y);
		bool pixelOk = true;

		// Any pixel with !(G ~= 255) is faulty (not a linear combinations of green and yellow)
		if (de::abs(pixel.getGreen() - 255) > colorThreshold)
			pixelOk = false;

		// Any pixel with !(B ~= 0) is faulty (not a linear combinations of green and yellow)
		if (de::abs(pixel.getBlue() - 0) > colorThreshold)
			pixelOk = false;

		error.setPixel(x, y, (pixelOk) ? (tcu::RGBA(0, 255, 0, 255)) : (tcu::RGBA(255, 0, 0, 255)));
		isOk = isOk && pixelOk;
	}

	if (!isOk)
	{
		log << TestLog::Message << "Image verification failed." << TestLog::EndMessage;
		log << TestLog::ImageSet("Verfication result", "Result of rendering")
			<< TestLog::Image("Result",		"Result",		image)
			<< TestLog::Image("ErrorMask",	"Error mask",	error)
			<< TestLog::EndImageSet;
	}
	else
	{
		log << TestLog::ImageSet("Verfication result", "Result of rendering")
			<< TestLog::Image("Result", "Result", image)
			<< TestLog::EndImageSet;
	}

	return isOk;
}

static void addTestIterations (gls::DrawTest* test, gls::DrawTestSpec& spec, TestIterationType type)
{
	if (type == TYPE_DRAW_COUNT)
	{
		spec.primitiveCount = 1;
		test->addIteration(spec, "draw count = 1");

		spec.primitiveCount = 5;
		test->addIteration(spec, "draw count = 5");

		spec.primitiveCount = 25;
		test->addIteration(spec, "draw count = 25");
	}
	else if (type == TYPE_INSTANCE_COUNT)
	{
		spec.instanceCount = 1;
		test->addIteration(spec, "instance count = 1");

		spec.instanceCount = 4;
		test->addIteration(spec, "instance count = 4");

		spec.instanceCount = 11;
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
}

static std::string sizeToString (int size)
{
	if (size < 1024)
		return de::toString(size) + " byte(s)";
	if (size < 1024*1024)
		return de::toString(size / 1024) + " KB";
	return de::toString(size / 1024 / 1024) + " MB";
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
			{ gls::DrawTestSpec::INPUTTYPE_FLOAT,        gls::DrawTestSpec::OUTPUTTYPE_VEC2,  4 },
			{ gls::DrawTestSpec::INPUTTYPE_FLOAT,        gls::DrawTestSpec::OUTPUTTYPE_VEC4,  2 },
			{ gls::DrawTestSpec::INPUTTYPE_INT,          gls::DrawTestSpec::OUTPUTTYPE_IVEC3, 4 },
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
			test->addIteration(spec, iterationDesc.c_str());
		}

		addChild(test);
	}
}

class FirstGroup : public TestCaseGroup
{
public:
									FirstGroup		(Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod);
									~FirstGroup		(void);

	void							init			(void);

private:
	gls::DrawTestSpec::DrawMethod	m_method;
};

FirstGroup::FirstGroup (Context& context, const char* name, const char* descr, gls::DrawTestSpec::DrawMethod drawMethod)
	: TestCaseGroup		(context, name, descr)
	, m_method			(drawMethod)
{
}

FirstGroup::~FirstGroup (void)
{
}

void FirstGroup::init (void)
{
	const int firsts[] =
	{
		1, 3, 17
	};

	gls::DrawTestSpec spec;
	genBasicSpec(spec, m_method);

	for (int firstNdx = 0; firstNdx < DE_LENGTH_OF_ARRAY(firsts); ++firstNdx)
	{
		const std::string	name = std::string("first_") + de::toString(firsts[firstNdx]);
		const std::string	desc = std::string("first ") + de::toString(firsts[firstNdx]);
		gls::DrawTest*		test = new gls::DrawTest(m_testCtx, m_context.getRenderContext(), name.c_str(), desc.c_str());

		spec.first = firsts[firstNdx];

		addTestIterations(test, spec, TYPE_DRAW_COUNT);

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
	const bool indexed		= (m_method == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_INDIRECT);
	const bool hasFirst		= (m_method == gls::DrawTestSpec::DRAWMETHOD_DRAWARRAYS_INDIRECT);

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

	if (hasFirst)
	{
		// First-tests
		this->addChild(new FirstGroup(m_context, "first", "First tests", m_method));
	}

	if (indexed)
	{
		// Index-tests
		this->addChild(new IndexGroup(m_context, "indices", "Index tests", m_method));
		this->addChild(new BaseVertexGroup(m_context, "base_vertex", "Base vertex tests", m_method));
	}

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(primitive); ++ndx)
	{
		const std::string name = gls::DrawTestSpec::primitiveToString(primitive[ndx]);
		const std::string desc = gls::DrawTestSpec::primitiveToString(primitive[ndx]);

		this->addChild(new AttributeGroup(m_context, name.c_str(), desc.c_str(), m_method, primitive[ndx], gls::DrawTestSpec::INDEXTYPE_SHORT, gls::DrawTestSpec::STORAGE_BUFFER));
	}
}

class GridProgram : public sglr::ShaderProgram
{
public:
			GridProgram		(void);

	void	shadeVertices	(const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const;
	void	shadeFragments	(rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const;
};

GridProgram::GridProgram (void)
	: sglr::ShaderProgram(sglr::pdec::ShaderProgramDeclaration()
							<< sglr::pdec::VertexAttribute("a_position", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute("a_offset", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexAttribute("a_color", rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexToFragmentVarying(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::FragmentOutput(rr::GENERICVECTYPE_FLOAT)
							<< sglr::pdec::VertexSource("#version 310 es\n"
														"in highp vec4 a_position;\n"
														"in highp vec4 a_offset;\n"
														"in highp vec4 a_color;\n"
														"out highp vec4 v_color;\n"
														"void main(void)\n"
														"{\n"
														"	gl_Position = a_position + a_offset;\n"
														"	v_color = a_color;\n"
														"}\n")
							<< sglr::pdec::FragmentSource(
														"#version 310 es\n"
														"layout(location = 0) out highp vec4 dEQP_FragColor;\n"
														"in highp vec4 v_color;\n"
														"void main(void)\n"
														"{\n"
														"	dEQP_FragColor = v_color;\n"
														"}\n"))
{
}

void GridProgram::shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
{
	for (int ndx = 0; ndx < numPackets; ++ndx)
	{
		packets[ndx]->position = rr::readVertexAttribFloat(inputs[0], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx) + rr::readVertexAttribFloat(inputs[1], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
		packets[ndx]->outputs[0] = rr::readVertexAttribFloat(inputs[2], packets[ndx]->instanceNdx, packets[ndx]->vertexNdx);
	}
}

void GridProgram::shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
{
	for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
	for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
		rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readTriangleVarying<float>(packets[packetNdx], context, 0, fragNdx));
}

class InstancedGridRenderTest : public TestCase
{
public:
					InstancedGridRenderTest		(Context& context, const char* name, const char* desc, int gridSide, bool useIndices);
					~InstancedGridRenderTest	(void);

	IterateResult	iterate						(void);

private:
	void			renderTo					(sglr::Context& ctx, sglr::ShaderProgram& program, tcu::Surface& dst);

	const int		m_gridSide;
	const bool		m_useIndices;
};

InstancedGridRenderTest::InstancedGridRenderTest (Context& context, const char* name, const char* desc, int gridSide, bool useIndices)
	: TestCase		(context, name, desc)
	, m_gridSide	(gridSide)
	, m_useIndices	(useIndices)
{
}

InstancedGridRenderTest::~InstancedGridRenderTest (void)
{
}

InstancedGridRenderTest::IterateResult InstancedGridRenderTest::iterate (void)
{
	const int renderTargetWidth  = de::min(1024, m_context.getRenderTarget().getWidth());
	const int renderTargetHeight = de::min(1024, m_context.getRenderTarget().getHeight());

	sglr::GLContext ctx		(m_context.getRenderContext(), m_testCtx.getLog(), sglr::GLCONTEXT_LOG_CALLS | sglr::GLCONTEXT_LOG_PROGRAMS, tcu::IVec4(0, 0, renderTargetWidth, renderTargetHeight));
	tcu::Surface	surface	(renderTargetWidth, renderTargetHeight);
	GridProgram		program;

	// render

	renderTo(ctx, program, surface);

	// verify image
	// \note the green/yellow pattern is only for clarity. The test will only verify that all instances were drawn by looking for anything non-green/yellow.
	if (verifyImageYellowGreen(surface, m_testCtx.getLog()))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result image invalid");
	return STOP;
}

void InstancedGridRenderTest::renderTo (sglr::Context& ctx, sglr::ShaderProgram& program, tcu::Surface& dst)
{
	const tcu::Vec4 green	(0, 1, 0, 1);
	const tcu::Vec4 yellow	(1, 1, 0, 1);

	deUint32 vaoID			= 0;
	deUint32 positionBuf	= 0;
	deUint32 offsetBuf		= 0;
	deUint32 colorBuf		= 0;
	deUint32 indexBuf		= 0;
	deUint32 drawIndirectBuf= 0;
	deUint32 programID		= ctx.createProgram(&program);
	deInt32 posLocation		= ctx.getAttribLocation(programID, "a_position");
	deInt32 offsetLocation	= ctx.getAttribLocation(programID, "a_offset");
	deInt32 colorLocation	= ctx.getAttribLocation(programID, "a_color");

	float cellW	= 2.0f / (float)m_gridSide;
	float cellH	= 2.0f / (float)m_gridSide;
	const tcu::Vec4 vertexPositions[] =
	{
		tcu::Vec4(0,		0,		0, 1),
		tcu::Vec4(cellW,	0,		0, 1),
		tcu::Vec4(0,		cellH,	0, 1),

		tcu::Vec4(0,		cellH,	0, 1),
		tcu::Vec4(cellW,	0,		0, 1),
		tcu::Vec4(cellW,	cellH,	0, 1),
	};

	const deUint16 indices[] =
	{
		0, 4, 3,
		2, 1, 5
	};

	std::vector<tcu::Vec4> offsets;
	for (int x = 0; x < m_gridSide; ++x)
	for (int y = 0; y < m_gridSide; ++y)
		offsets.push_back(tcu::Vec4((float)x * cellW - 1.0f, (float)y * cellW - 1.0f, 0, 0));

	std::vector<tcu::Vec4> colors;
	for (int x = 0; x < m_gridSide; ++x)
	for (int y = 0; y < m_gridSide; ++y)
		colors.push_back(((x + y) % 2 == 0) ? (green) : (yellow));

	ctx.genVertexArrays(1, &vaoID);
	ctx.bindVertexArray(vaoID);

	ctx.genBuffers(1, &positionBuf);
	ctx.bindBuffer(GL_ARRAY_BUFFER, positionBuf);
	ctx.bufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	ctx.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	ctx.vertexAttribDivisor(posLocation, 0);
	ctx.enableVertexAttribArray(posLocation);

	ctx.genBuffers(1, &offsetBuf);
	ctx.bindBuffer(GL_ARRAY_BUFFER, offsetBuf);
	ctx.bufferData(GL_ARRAY_BUFFER, offsets.size() * sizeof(tcu::Vec4), &offsets[0], GL_STATIC_DRAW);
	ctx.vertexAttribPointer(offsetLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	ctx.vertexAttribDivisor(offsetLocation, 1);
	ctx.enableVertexAttribArray(offsetLocation);

	ctx.genBuffers(1, &colorBuf);
	ctx.bindBuffer(GL_ARRAY_BUFFER, colorBuf);
	ctx.bufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(tcu::Vec4), &colors[0], GL_STATIC_DRAW);
	ctx.vertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	ctx.vertexAttribDivisor(colorLocation, 1);
	ctx.enableVertexAttribArray(colorLocation);

	if (m_useIndices)
	{
		ctx.genBuffers(1, &indexBuf);
		ctx.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
		ctx.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	}

	ctx.genBuffers(1, &drawIndirectBuf);
	ctx.bindBuffer(GL_DRAW_INDIRECT_BUFFER, drawIndirectBuf);

	if (m_useIndices)
	{
		DrawElementsCommand command;
		command.count				= 6;
		command.primCount			= m_gridSide * m_gridSide;
		command.firstIndex			= 0;
		command.baseVertex			= 0;
		command.reservedMustBeZero	= 0;

		ctx.bufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(command), &command, GL_STATIC_DRAW);
	}
	else
	{
		DrawArraysCommand command;
		command.count				= 6;
		command.primCount			= m_gridSide * m_gridSide;
		command.first				= 0;
		command.reservedMustBeZero	= 0;

		ctx.bufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(command), &command, GL_STATIC_DRAW);
	}

	ctx.clearColor(0, 0, 0, 1);
	ctx.clear(GL_COLOR_BUFFER_BIT);

	ctx.viewport(0, 0, dst.getWidth(), dst.getHeight());

	ctx.useProgram(programID);
	if (m_useIndices)
		ctx.drawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, DE_NULL);
	else
		ctx.drawArraysIndirect(GL_TRIANGLES, DE_NULL);
	ctx.useProgram(0);

	glu::checkError(ctx.getError(), "", __FILE__, __LINE__);

	ctx.deleteBuffers(1, &drawIndirectBuf);
	if (m_useIndices)
		ctx.deleteBuffers(1, &indexBuf);
	ctx.deleteBuffers(1, &colorBuf);
	ctx.deleteBuffers(1, &offsetBuf);
	ctx.deleteBuffers(1, &positionBuf);
	ctx.deleteVertexArrays(1, &vaoID);
	ctx.deleteProgram(programID);

	ctx.finish();
	ctx.readPixels(dst, 0, 0, dst.getWidth(), dst.getHeight());

	glu::checkError(ctx.getError(), "", __FILE__, __LINE__);
}

class InstancingGroup : public TestCaseGroup
{
public:
			InstancingGroup		(Context& context, const char* name, const char* descr);
			~InstancingGroup	(void);

	void	init				(void);
};

InstancingGroup::InstancingGroup (Context& context, const char* name, const char* descr)
	: TestCaseGroup	(context, name, descr)
{
}

InstancingGroup::~InstancingGroup (void)
{
}

void InstancingGroup::init (void)
{
	const int gridWidths[] =
	{
		2,
		5,
		10,
		32,
		100,
	};

	// drawArrays
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(gridWidths); ++ndx)
	{
		const std::string name = std::string("draw_arrays_indirect_grid_") + de::toString(gridWidths[ndx]) + "x" + de::toString(gridWidths[ndx]);
		const std::string desc = std::string("DrawArraysIndirect, Grid size ") + de::toString(gridWidths[ndx]) + "x" + de::toString(gridWidths[ndx]);

		this->addChild(new InstancedGridRenderTest(m_context, name.c_str(), desc.c_str(), gridWidths[ndx], false));
	}

	// drawElements
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(gridWidths); ++ndx)
	{
		const std::string name = std::string("draw_elements_indirect_grid_") + de::toString(gridWidths[ndx]) + "x" + de::toString(gridWidths[ndx]);
		const std::string desc = std::string("DrawElementsIndirect, Grid size ") + de::toString(gridWidths[ndx]) + "x" + de::toString(gridWidths[ndx]);

		this->addChild(new InstancedGridRenderTest(m_context, name.c_str(), desc.c_str(), gridWidths[ndx], true));
	}
}

class ComputeShaderGeneratedCase : public TestCase
{
public:
	enum DrawMethod
	{
		DRAWMETHOD_DRAWARRAYS,
		DRAWMETHOD_DRAWELEMENTS,
		DRAWMETHOD_LAST
	};

						ComputeShaderGeneratedCase	(Context& context, const char* name, const char* desc, DrawMethod method, bool computeCmd, bool computeData, bool computeIndices, int gridSize, int drawCallCount);
						~ComputeShaderGeneratedCase	(void);
	void				init						(void);
	void				deinit						(void);

	IterateResult		iterate						(void);
	std::string			genComputeSource			(bool computeCmd, bool computeData, bool computeIndices) const;

private:
	void				createDrawCommand			(void);
	void				createDrawData				(void);
	void				createDrawIndices			(void);

	virtual void		runComputeShader			(void) = 0;
	void				renderTo					(tcu::Surface& image);

protected:
	deUint32			calcDrawBufferSize			(void) const;
	deUint32			calcIndexBufferSize			(void) const;

	const DrawMethod	m_drawMethod;
	const bool			m_computeCmd;
	const bool			m_computeData;
	const bool			m_computeIndices;
	const int			m_commandSize;
	const int			m_numDrawCmds;
	const int			m_gridSize;

	glw::GLuint			m_cmdBufferID;
	glw::GLuint			m_dataBufferID;
	glw::GLuint			m_indexBufferID;

private:
	glu::ShaderProgram*	m_shaderProgram;
};

ComputeShaderGeneratedCase::ComputeShaderGeneratedCase (Context& context, const char* name, const char* desc, DrawMethod method, bool computeCmd, bool computeData, bool computeIndices, int gridSize, int drawCallCount)
	: TestCase			(context, name, desc)
	, m_drawMethod		(method)
	, m_computeCmd		(computeCmd)
	, m_computeData		(computeData)
	, m_computeIndices	(computeIndices)
	, m_commandSize		((method==DRAWMETHOD_DRAWARRAYS) ? ((int)sizeof(DrawArraysCommand)) : ((int)sizeof(DrawElementsCommand)))
	, m_numDrawCmds		(drawCallCount)
	, m_gridSize		(gridSize)
	, m_cmdBufferID		(0)
	, m_dataBufferID	(0)
	, m_indexBufferID	(0)
	, m_shaderProgram	(DE_NULL)
{
    const int triangleCount	= m_gridSize * m_gridSize * 2;

	DE_ASSERT(method < DRAWMETHOD_LAST);
	DE_ASSERT(!computeIndices || method == DRAWMETHOD_DRAWELEMENTS);
	DE_ASSERT(triangleCount % m_numDrawCmds == 0);
	DE_UNREF(triangleCount);
}

ComputeShaderGeneratedCase::~ComputeShaderGeneratedCase (void)
{
	deinit();
}

void ComputeShaderGeneratedCase::init (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// generate basic shader

	m_shaderProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_colorVertexShaderSource) << glu::FragmentSource(s_colorFragmentShaderSource));
	m_testCtx.getLog() << *m_shaderProgram;

	if (!m_shaderProgram->isOk())
		throw tcu::TestError("Failed to compile shader.");

	// gen buffers
	gl.genBuffers(1, &m_cmdBufferID);
	gl.genBuffers(1, &m_dataBufferID);
	gl.genBuffers(1, &m_indexBufferID);

	// check the SSBO buffers are of legal size
	{
		const deUint64	drawBufferElementSize	= sizeof(tcu::Vec4);
		const deUint64	indexBufferElementSize	= sizeof(deUint32);
		const int		commandBufferSize		= m_commandSize * m_numDrawCmds;
		deInt64			maxSSBOSize				= 0;

		gl.getInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize);

		if (m_computeData && (deUint64)calcDrawBufferSize()*drawBufferElementSize > (deUint64)maxSSBOSize)
			throw tcu::NotSupportedError("GL_MAX_SHADER_STORAGE_BLOCK_SIZE is too small for vertex attrib buffers");
		if (m_computeIndices && (deUint64)calcIndexBufferSize()*indexBufferElementSize > (deUint64)maxSSBOSize)
			throw tcu::NotSupportedError("GL_MAX_SHADER_STORAGE_BLOCK_SIZE is too small for index buffers");
		if (m_computeCmd && (deUint64)commandBufferSize > (deUint64)maxSSBOSize)
			throw tcu::NotSupportedError("GL_MAX_SHADER_STORAGE_BLOCK_SIZE is too small for command buffers");
	}
}

void ComputeShaderGeneratedCase::deinit (void)
{
	if (m_cmdBufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_cmdBufferID);
		m_cmdBufferID = 0;
	}
	if (m_dataBufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_dataBufferID);
		m_dataBufferID = 0;
	}
	if (m_indexBufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_indexBufferID);
		m_indexBufferID = 0;
	}

	if (m_shaderProgram)
	{
		delete m_shaderProgram;
		m_shaderProgram = DE_NULL;
	}
}

ComputeShaderGeneratedCase::IterateResult ComputeShaderGeneratedCase::iterate (void)
{
	const int				renderTargetWidth	= de::min(1024, m_context.getRenderTarget().getWidth());
	const int				renderTargetHeight	= de::min(1024, m_context.getRenderTarget().getHeight());
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	tcu::Surface			surface				(renderTargetWidth, renderTargetHeight);

	m_testCtx.getLog() << tcu::TestLog::Message << "Preparing to draw " << m_gridSize << " x " << m_gridSize << " grid." << tcu::TestLog::EndMessage;

	try
	{
		// Gen command buffer
		if (!m_computeCmd)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Uploading draw command buffer." << tcu::TestLog::EndMessage;
			createDrawCommand();
		}

		// Gen data buffer
		if (!m_computeData)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Uploading draw data buffer." << tcu::TestLog::EndMessage;
			createDrawData();
		}

		// Gen index buffer
		if (!m_computeIndices && m_drawMethod == DRAWMETHOD_DRAWELEMENTS)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Uploading draw index buffer." << tcu::TestLog::EndMessage;
			createDrawIndices();
		}

		// Run compute shader
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message << "Filling following buffers using compute shader:\n"
				<< ((m_computeCmd)		? ("\tcommand buffer\n")	: (""))
				<< ((m_computeData)		? ("\tdata buffer\n")		: (""))
				<< ((m_computeIndices)	? ("\tindex buffer\n")		: (""))
				<< tcu::TestLog::EndMessage;
			runComputeShader();
		}

		// Ensure data is written to the buffers before we try to read it
		{
			const glw::GLuint barriers = ((m_computeCmd)     ? (GL_COMMAND_BARRIER_BIT)             : (0)) |
										 ((m_computeData)    ? (GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT) : (0)) |
										 ((m_computeIndices) ? (GL_ELEMENT_ARRAY_BARRIER_BIT)       : (0));

			m_testCtx.getLog() << tcu::TestLog::Message << "Memory barrier. Barriers = " << glu::getMemoryBarrierFlagsStr(barriers) << tcu::TestLog::EndMessage;
			gl.memoryBarrier(barriers);
		}

		// Draw from buffers

		m_testCtx.getLog() << tcu::TestLog::Message << "Drawing from buffers with " << m_numDrawCmds << " draw call(s)." << tcu::TestLog::EndMessage;
		renderTo(surface);
	}
	catch (glu::OutOfMemoryError&)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Got GL_OUT_OF_MEMORY." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Got GL_OUT_OF_MEMORY");
		m_testCtx.setTerminateAfter(true); // Do not rely on implementation to be able to recover from OOM
		return STOP;
	}


	// verify image
	// \note the green/yellow pattern is only for clarity. The test will only verify that all grid cells were drawn by looking for anything non-green/yellow.
	if (verifyImageYellowGreen(surface, m_testCtx.getLog()))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result image invalid");
	return STOP;
}

std::string ComputeShaderGeneratedCase::genComputeSource (bool computeCmd, bool computeData, bool computeIndices) const
{
	const int cmdLayoutBinding				= 0;
	const int dataLayoutBinding				= (computeCmd) ? (1) : (0);
	const int indexLayoutBinding			= (computeCmd && computeData) ? (2) : (computeCmd || computeData) ? (1) : (0);

	std::ostringstream buf;

	buf << "#version 310 es\n\n"
		<< "precision highp int;\n"
		<< "precision highp float;\n\n";

	if (computeCmd && m_drawMethod==DRAWMETHOD_DRAWARRAYS)
		buf	<< "struct DrawArraysIndirectCommand {\n"
			<< "    uint count;\n"
			<< "    uint primCount;\n"
			<< "    uint first;\n"
			<< "    uint reservedMustBeZero;\n"
			<< "};\n\n";
	else if (computeCmd && m_drawMethod==DRAWMETHOD_DRAWELEMENTS)
		buf	<< "struct DrawElementsIndirectCommand {\n"
			<< "    uint count;\n"
			<< "    uint primCount;\n"
			<< "    uint firstIndex;\n"
			<< "    int  baseVertex;\n"
			<< "    uint reservedMustBeZero;\n"
			<< "};\n\n";

	buf << "layout(local_size_x = 1, local_size_y = 1) in;\n"
		<< "layout(std430) buffer;\n\n";

	if (computeCmd)
		buf	<< "layout(binding = " << cmdLayoutBinding << ") writeonly buffer CommandBuffer {\n"
			<< "    " << ((m_drawMethod==DRAWMETHOD_DRAWARRAYS) ? ("DrawArraysIndirectCommand") : ("DrawElementsIndirectCommand")) << " commands[];\n"
			<< "};\n";
	if (computeData)
		buf	<< "layout(binding = " << dataLayoutBinding << ") writeonly buffer DataBuffer {\n"
			<< "    vec4 attribs[];\n"
			<< "};\n";
	if (computeIndices)
		buf	<< "layout(binding = " << indexLayoutBinding << ") writeonly buffer IndexBuffer {\n"
			<< "    uint indices[];\n"
			<< "};\n";

	buf	<< "\n"
		<< "void main() {\n"
		<< "    const uint gridSize      = " << m_gridSize << "u;\n"
		<< "    const uint triangleCount = gridSize * gridSize * 2u;\n"
		<< "\n";

	if (computeCmd)
	{
		buf	<< "    // command\n"
			<< "    if (gl_GlobalInvocationID.x < " << m_numDrawCmds << "u && gl_GlobalInvocationID.y == 0u && gl_GlobalInvocationID.z == 0u) {\n"
			<< "        const uint numDrawCallTris = triangleCount / " << m_numDrawCmds << "u;\n"
			<< "        uint firstTri              = gl_GlobalInvocationID.x * numDrawCallTris;\n\n"
			<< "        commands[gl_GlobalInvocationID.x].count                 = numDrawCallTris*3u;\n"
			<< "        commands[gl_GlobalInvocationID.x].primCount             = 1u;\n";

		if (m_drawMethod==DRAWMETHOD_DRAWARRAYS)
		{
			buf	<< "        commands[gl_GlobalInvocationID.x].first                 = firstTri*3u;\n";
		}
		else if (m_drawMethod==DRAWMETHOD_DRAWELEMENTS)
		{
			buf	<< "        commands[gl_GlobalInvocationID.x].firstIndex            = firstTri*3u;\n";
			buf	<< "        commands[gl_GlobalInvocationID.x].baseVertex            = 0;\n";
		}

		buf	<< "        commands[gl_GlobalInvocationID.x].reservedMustBeZero    = 0u;\n"
			<< "    }\n"
			<< "\n";
	}

	if (computeData)
	{
		buf	<< "    // vertex attribs\n"
			<< "    const vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
			<< "    const vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n";

		if (m_drawMethod == DRAWMETHOD_DRAWARRAYS)
		{
			buf	<< "    if (gl_GlobalInvocationID.x < gridSize && gl_GlobalInvocationID.y < gridSize && gl_GlobalInvocationID.z == 0u) {\n"
				<< "        uint        y           = gl_GlobalInvocationID.x;\n"
				<< "        uint        x           = gl_GlobalInvocationID.y;\n"
				<< "        float       posX        = (float(x)    / float(gridSize)) * 2.0 - 1.0;\n"
				<< "        float       posXp1      = (float(x+1u) / float(gridSize)) * 2.0 - 1.0;\n"
				<< "        float       posY        = (float(y)    / float(gridSize)) * 2.0 - 1.0;\n"
				<< "        float       posYp1      = (float(y+1u) / float(gridSize)) * 2.0 - 1.0;\n"
				<< "        vec4        color       = ((x + y)%2u != 0u) ? (yellow) : (green);\n"
				<< "\n"
				<< "        attribs[((y * gridSize + x) * 6u + 0u) * 2u + 0u] = vec4(posX,   posY,   0.0, 1.0);\n"
				<< "        attribs[((y * gridSize + x) * 6u + 1u) * 2u + 0u] = vec4(posXp1, posY,   0.0, 1.0);\n"
				<< "        attribs[((y * gridSize + x) * 6u + 2u) * 2u + 0u] = vec4(posXp1, posYp1, 0.0, 1.0);\n"
				<< "        attribs[((y * gridSize + x) * 6u + 3u) * 2u + 0u] = vec4(posX,   posY,   0.0, 1.0);\n"
				<< "        attribs[((y * gridSize + x) * 6u + 4u) * 2u + 0u] = vec4(posXp1, posYp1, 0.0, 1.0);\n"
				<< "        attribs[((y * gridSize + x) * 6u + 5u) * 2u + 0u] = vec4(posX,   posYp1, 0.0, 1.0);\n"
				<< "\n"
				<< "        attribs[((y * gridSize + x) * 6u + 0u) * 2u + 1u] = color;\n"
				<< "        attribs[((y * gridSize + x) * 6u + 1u) * 2u + 1u] = color;\n"
				<< "        attribs[((y * gridSize + x) * 6u + 2u) * 2u + 1u] = color;\n"
				<< "        attribs[((y * gridSize + x) * 6u + 3u) * 2u + 1u] = color;\n"
				<< "        attribs[((y * gridSize + x) * 6u + 4u) * 2u + 1u] = color;\n"
				<< "        attribs[((y * gridSize + x) * 6u + 5u) * 2u + 1u] = color;\n"
				<< "    }\n";
		}
		else if (m_drawMethod == DRAWMETHOD_DRAWELEMENTS)
		{
			buf	<< "    if (gl_GlobalInvocationID.x < gridSize+1u && gl_GlobalInvocationID.y < gridSize+1u && gl_GlobalInvocationID.z == 0u) {\n"
				<< "        uint        y           = gl_GlobalInvocationID.x;\n"
				<< "        uint        x           = gl_GlobalInvocationID.y;\n"
				<< "        float       posX        = (float(x) / float(gridSize)) * 2.0 - 1.0;\n"
				<< "        float       posY        = (float(y) / float(gridSize)) * 2.0 - 1.0;\n"
				<< "\n"
				<< "        attribs[(y * (gridSize+1u) + x) * 4u + 0u] = vec4(posX, posY, 0.0, 1.0);\n"
				<< "        attribs[(y * (gridSize+1u) + x) * 4u + 1u] = green;\n"
				<< "        attribs[(y * (gridSize+1u) + x) * 4u + 2u] = vec4(posX, posY, 0.0, 1.0);\n"
				<< "        attribs[(y * (gridSize+1u) + x) * 4u + 3u] = yellow;\n"
				<< "    }\n";
		}

		buf << "\n";
	}

	if (computeIndices)
	{
		buf	<< "    // indices\n"
			<< "    if (gl_GlobalInvocationID.x < gridSize && gl_GlobalInvocationID.y < gridSize && gl_GlobalInvocationID.z == 0u) {\n"
			<< "        uint    y       = gl_GlobalInvocationID.x;\n"
			<< "        uint    x       = gl_GlobalInvocationID.y;\n"
			<< "        uint    color   = ((x + y)%2u);\n"
			<< "\n"
			<< "        indices[(y * gridSize + x) * 6u + 0u] = ((y+0u) * (gridSize+1u) + (x+0u)) * 2u + color;\n"
			<< "        indices[(y * gridSize + x) * 6u + 1u] = ((y+1u) * (gridSize+1u) + (x+0u)) * 2u + color;\n"
			<< "        indices[(y * gridSize + x) * 6u + 2u] = ((y+1u) * (gridSize+1u) + (x+1u)) * 2u + color;\n"
			<< "        indices[(y * gridSize + x) * 6u + 3u] = ((y+0u) * (gridSize+1u) + (x+0u)) * 2u + color;\n"
			<< "        indices[(y * gridSize + x) * 6u + 4u] = ((y+1u) * (gridSize+1u) + (x+1u)) * 2u + color;\n"
			<< "        indices[(y * gridSize + x) * 6u + 5u] = ((y+0u) * (gridSize+1u) + (x+1u)) * 2u + color;\n"
			<< "    }\n"
			<< "\n";
	}

	buf	<< "}\n";

	return buf.str();
}

void ComputeShaderGeneratedCase::createDrawCommand (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const int				triangleCount	= m_gridSize * m_gridSize * 2;
	const deUint32			numDrawCallTris	= triangleCount / m_numDrawCmds;

	if (m_drawMethod == DRAWMETHOD_DRAWARRAYS)
	{
		std::vector<DrawArraysCommand> cmds;

		for (int ndx = 0; ndx < m_numDrawCmds; ++ndx)
		{
			const deUint32				firstTri = ndx * numDrawCallTris;
			DrawArraysCommand			data;

			data.count					= numDrawCallTris*3;
			data.primCount				= 1;
			data.first					= firstTri*3;
			data.reservedMustBeZero		= 0;

			cmds.push_back(data);
		}

		DE_ASSERT((int)(sizeof(DrawArraysCommand)*cmds.size()) == m_numDrawCmds * m_commandSize);

		gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_cmdBufferID);
		gl.bufferData(GL_DRAW_INDIRECT_BUFFER, (glw::GLsizeiptr)(sizeof(DrawArraysCommand)*cmds.size()), &cmds[0], GL_STATIC_DRAW);
	}
	else if (m_drawMethod == DRAWMETHOD_DRAWELEMENTS)
	{
		std::vector<DrawElementsCommand> cmds;

		for (int ndx = 0; ndx < m_numDrawCmds; ++ndx)
		{
			const deUint32			firstTri = ndx * numDrawCallTris;
			DrawElementsCommand		data;

			data.count				= numDrawCallTris*3;
			data.primCount			= 1;
			data.firstIndex			= firstTri*3;
			data.baseVertex			= 0;
			data.reservedMustBeZero	= 0;

			cmds.push_back(data);
		}

		DE_ASSERT((int)(sizeof(DrawElementsCommand)*cmds.size()) == m_numDrawCmds * m_commandSize);

		gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_cmdBufferID);
		gl.bufferData(GL_DRAW_INDIRECT_BUFFER, (glw::GLsizeiptr)(sizeof(DrawElementsCommand)*cmds.size()), &cmds[0], GL_STATIC_DRAW);
	}
	else
		DE_ASSERT(false);

	glu::checkError(gl.getError(), "create draw command", __FILE__, __LINE__);
}

void ComputeShaderGeneratedCase::createDrawData (void)
{
	const tcu::Vec4			yellow	(1.0f, 1.0f, 0.0f, 1.0f);
	const tcu::Vec4			green	(0.0f, 1.0f, 0.0f, 1.0f);
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();

	if (m_drawMethod == DRAWMETHOD_DRAWARRAYS)
	{
		// Store elements in the order they are drawn. Interleave color.
		std::vector<tcu::Vec4> buffer(m_gridSize*m_gridSize*6*2);

		DE_ASSERT(buffer.size() == calcDrawBufferSize());

		for (int y = 0; y < m_gridSize; ++y)
		for (int x = 0; x < m_gridSize; ++x)
		{
			const float			posX		= ((float)x / (float)m_gridSize) * 2.0f - 1.0f;
			const float			posY		= ((float)y / (float)m_gridSize) * 2.0f - 1.0f;
			const float			cellSize	= 2.0f / (float)m_gridSize;
			const tcu::Vec4&	color		= ((x + y)%2) ? (yellow) : (green);

			buffer[((y * m_gridSize + x) * 6 + 0) * 2 + 0] = tcu::Vec4(posX,			posY,				0.0f, 1.0f);
			buffer[((y * m_gridSize + x) * 6 + 1) * 2 + 0] = tcu::Vec4(posX + cellSize,	posY,				0.0f, 1.0f);
			buffer[((y * m_gridSize + x) * 6 + 2) * 2 + 0] = tcu::Vec4(posX + cellSize,	posY + cellSize,	0.0f, 1.0f);
			buffer[((y * m_gridSize + x) * 6 + 3) * 2 + 0] = tcu::Vec4(posX,			posY,				0.0f, 1.0f);
			buffer[((y * m_gridSize + x) * 6 + 4) * 2 + 0] = tcu::Vec4(posX + cellSize, posY + cellSize,	0.0f, 1.0f);
			buffer[((y * m_gridSize + x) * 6 + 5) * 2 + 0] = tcu::Vec4(posX,			posY + cellSize,	0.0f, 1.0f);

			buffer[((y * m_gridSize + x) * 6 + 0) * 2 + 1] = color;
			buffer[((y * m_gridSize + x) * 6 + 1) * 2 + 1] = color;
			buffer[((y * m_gridSize + x) * 6 + 2) * 2 + 1] = color;
			buffer[((y * m_gridSize + x) * 6 + 3) * 2 + 1] = color;
			buffer[((y * m_gridSize + x) * 6 + 4) * 2 + 1] = color;
			buffer[((y * m_gridSize + x) * 6 + 5) * 2 + 1] = color;
		}

		gl.bindBuffer(GL_ARRAY_BUFFER, m_dataBufferID);
		gl.bufferData(GL_ARRAY_BUFFER, (int)(buffer.size() * sizeof(tcu::Vec4)), buffer[0].getPtr(), GL_STATIC_DRAW);
	}
	else if (m_drawMethod == DRAWMETHOD_DRAWELEMENTS)
	{
		// Elements are indexed by index buffer. Interleave color. Two vertices per position since 2 colors

		std::vector<tcu::Vec4> buffer((m_gridSize+1)*(m_gridSize+1)*4);

		DE_ASSERT(buffer.size() == calcDrawBufferSize());

		for (int y = 0; y < m_gridSize+1; ++y)
		for (int x = 0; x < m_gridSize+1; ++x)
		{
			const float			posX		= ((float)x / (float)m_gridSize) * 2.0f - 1.0f;
			const float			posY		= ((float)y / (float)m_gridSize) * 2.0f - 1.0f;

			buffer[(y * (m_gridSize+1) + x) * 4 + 0] = tcu::Vec4(posX, posY, 0.0f, 1.0f);
			buffer[(y * (m_gridSize+1) + x) * 4 + 1] = green;
			buffer[(y * (m_gridSize+1) + x) * 4 + 2] = tcu::Vec4(posX, posY, 0.0f, 1.0f);
			buffer[(y * (m_gridSize+1) + x) * 4 + 3] = yellow;
		}

		gl.bindBuffer(GL_ARRAY_BUFFER, m_dataBufferID);
		gl.bufferData(GL_ARRAY_BUFFER, (int)(buffer.size() * sizeof(tcu::Vec4)), buffer[0].getPtr(), GL_STATIC_DRAW);
	}
	else
		DE_ASSERT(false);

	glu::checkError(gl.getError(), "", __FILE__, __LINE__);
}

void ComputeShaderGeneratedCase::createDrawIndices (void)
{
	DE_ASSERT(m_drawMethod == DRAWMETHOD_DRAWELEMENTS);

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	std::vector<deUint32>	buffer	(m_gridSize*m_gridSize*6);

	DE_ASSERT(buffer.size() == calcIndexBufferSize());

	for (int y = 0; y < m_gridSize; ++y)
	for (int x = 0; x < m_gridSize; ++x)
	{
		const int color = ((x + y)%2);

		buffer[(y * m_gridSize + x) * 6 + 0] = ((y+0) * (m_gridSize+1) + (x+0)) * 2 + color;
		buffer[(y * m_gridSize + x) * 6 + 1] = ((y+1) * (m_gridSize+1) + (x+0)) * 2 + color;
		buffer[(y * m_gridSize + x) * 6 + 2] = ((y+1) * (m_gridSize+1) + (x+1)) * 2 + color;
		buffer[(y * m_gridSize + x) * 6 + 3] = ((y+0) * (m_gridSize+1) + (x+0)) * 2 + color;
		buffer[(y * m_gridSize + x) * 6 + 4] = ((y+1) * (m_gridSize+1) + (x+1)) * 2 + color;
		buffer[(y * m_gridSize + x) * 6 + 5] = ((y+0) * (m_gridSize+1) + (x+1)) * 2 + color;
	}

	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferID);
	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, (int)(buffer.size() * sizeof(deUint32)), &buffer[0], GL_STATIC_DRAW);
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);
}

void ComputeShaderGeneratedCase::renderTo (tcu::Surface& dst)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	const deInt32			positionLoc = gl.getAttribLocation(m_shaderProgram->getProgram(), "a_position");
	const deInt32			colorLoc	= gl.getAttribLocation(m_shaderProgram->getProgram(), "a_color");
	deUint32				vaoID		= 0;

	gl.genVertexArrays(1, &vaoID);
	gl.bindVertexArray(vaoID);

	// Setup buffers

	gl.bindBuffer(GL_ARRAY_BUFFER, m_dataBufferID);
	gl.vertexAttribPointer(positionLoc, 4, GL_FLOAT, GL_FALSE, 8 * (int)sizeof(float), DE_NULL);
	gl.vertexAttribPointer(colorLoc,    4, GL_FLOAT, GL_FALSE, 8 * (int)sizeof(float), ((const deUint8*)DE_NULL) + 4*sizeof(float));
	gl.enableVertexAttribArray(positionLoc);
	gl.enableVertexAttribArray(colorLoc);

	DE_ASSERT(positionLoc != -1);
	DE_ASSERT(colorLoc != -1);

	if (m_drawMethod == DRAWMETHOD_DRAWELEMENTS)
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferID);

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_cmdBufferID);

	// draw

	gl.clearColor(0, 0, 0, 1);
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl.viewport(0, 0, dst.getWidth(), dst.getHeight());

	gl.useProgram(m_shaderProgram->getProgram());
	for (int drawCmdNdx = 0; drawCmdNdx < m_numDrawCmds; ++drawCmdNdx)
	{
		const void* offset = ((deUint8*)DE_NULL) + drawCmdNdx*m_commandSize;

		if (m_drawMethod == DRAWMETHOD_DRAWELEMENTS)
			gl.drawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, offset);
		else if (m_drawMethod == DRAWMETHOD_DRAWARRAYS)
			gl.drawArraysIndirect(GL_TRIANGLES, offset);
		else
			DE_ASSERT(DE_FALSE);
	}
	gl.useProgram(0);

	// free

	gl.deleteVertexArrays(1, &vaoID);
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	gl.finish();
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);
}

deUint32 ComputeShaderGeneratedCase::calcDrawBufferSize (void) const
{
	// returns size in "vec4"s
	if (m_drawMethod == DRAWMETHOD_DRAWARRAYS)
		return m_gridSize*m_gridSize*6*2;
	else if (m_drawMethod == DRAWMETHOD_DRAWELEMENTS)
		return (m_gridSize+1)*(m_gridSize+1)*4;
	else
		DE_ASSERT(DE_FALSE);

	return 0;
}

deUint32 ComputeShaderGeneratedCase::calcIndexBufferSize (void) const
{
	if (m_drawMethod == DRAWMETHOD_DRAWELEMENTS)
		return m_gridSize*m_gridSize*6;
	else
		return 0;
}

class ComputeShaderGeneratedCombinedCase : public ComputeShaderGeneratedCase
{
public:
						ComputeShaderGeneratedCombinedCase	(Context& context, const char* name, const char* desc, DrawMethod method, bool computeCmd, bool computeData, bool computeIndices, int gridSize, int numDrawCalls);
						~ComputeShaderGeneratedCombinedCase	(void);

	void				init								(void);
	void				deinit								(void);

private:
	void				runComputeShader					(void);

	glu::ShaderProgram*	m_computeProgram;
};

ComputeShaderGeneratedCombinedCase::ComputeShaderGeneratedCombinedCase (Context& context, const char* name, const char* desc, DrawMethod method, bool computeCmd, bool computeData, bool computeIndices, int gridSize, int numDrawCalls)
	: ComputeShaderGeneratedCase(context, name, desc, method, computeCmd, computeData, computeIndices, gridSize, numDrawCalls)
	, m_computeProgram			(DE_NULL)
{
}

ComputeShaderGeneratedCombinedCase::~ComputeShaderGeneratedCombinedCase (void)
{
	deinit();
}

void ComputeShaderGeneratedCombinedCase::init (void)
{
	// generate compute shader

	m_computeProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genComputeSource(m_computeCmd, m_computeData, m_computeIndices)));
	m_testCtx.getLog() << *m_computeProgram;

	if (!m_computeProgram->isOk())
		throw tcu::TestError("Failed to compile compute shader.");

	// init parent
	ComputeShaderGeneratedCase::init();
}

void ComputeShaderGeneratedCombinedCase::deinit (void)
{
	// deinit parent
	ComputeShaderGeneratedCase::deinit();

	if (m_computeProgram)
	{
		delete m_computeProgram;
		m_computeProgram = DE_NULL;
	}
}

void ComputeShaderGeneratedCombinedCase::runComputeShader (void)
{
	const glw::Functions&	gl									= m_context.getRenderContext().getFunctions();
	const bool				indexed								= (m_drawMethod == DRAWMETHOD_DRAWELEMENTS);
	const tcu::IVec3		nullSize							(0, 0, 0);
	const tcu::IVec3		commandDispatchSize					= (m_computeCmd)				? (tcu::IVec3(m_numDrawCmds, 1, 1))				: (nullSize);
	const tcu::IVec3		drawElementsDataBufferDispatchSize	= (m_computeData)				? (tcu::IVec3(m_gridSize+1, m_gridSize+1, 1))	: (nullSize);
	const tcu::IVec3		drawArraysDataBufferDispatchSize	= (m_computeData)				? (tcu::IVec3(m_gridSize,   m_gridSize,   1))	: (nullSize);
	const tcu::IVec3		indexBufferDispatchSize				= (m_computeIndices && indexed)	? (tcu::IVec3(m_gridSize,   m_gridSize,   1))	: (nullSize);

	const tcu::IVec3		dataBufferDispatchSize				= (m_drawMethod == DRAWMETHOD_DRAWELEMENTS) ? (drawElementsDataBufferDispatchSize) : (drawArraysDataBufferDispatchSize);
	const tcu::IVec3		dispatchSize						= tcu::max(tcu::max(commandDispatchSize, dataBufferDispatchSize), indexBufferDispatchSize);

	gl.useProgram(m_computeProgram->getProgram());
	glu::checkError(gl.getError(), "use compute shader", __FILE__, __LINE__);

	// setup buffers

	if (m_computeCmd)
	{
		const int			bindingPoint	= 0;
		const int			bufferSize		= m_commandSize * m_numDrawCmds;

		m_testCtx.getLog() << tcu::TestLog::Message << "Binding command buffer to binding point " << bindingPoint << tcu::TestLog::EndMessage;
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_cmdBufferID);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating memory for command buffer, size " << sizeToString(bufferSize) << "." << tcu::TestLog::EndMessage;
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, DE_NULL, GL_DYNAMIC_DRAW);
	}

	if (m_computeData)
	{
		const int			bindingPoint	= (m_computeCmd) ? (1) : (0);
		const int			bufferSize		= (int)(calcDrawBufferSize()*sizeof(tcu::Vec4));

		m_testCtx.getLog() << tcu::TestLog::Message << "Binding data buffer to binding point " << bindingPoint << tcu::TestLog::EndMessage;
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_dataBufferID);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating memory for data buffer, size " << sizeToString(bufferSize) << "." << tcu::TestLog::EndMessage;
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, DE_NULL, GL_DYNAMIC_DRAW);
	}

	if (m_computeIndices)
	{
		const int			bindingPoint	= (m_computeCmd && m_computeData) ? (2) : (m_computeCmd || m_computeData) ? (1) : (0);
		const int			bufferSize		= (int)(calcIndexBufferSize()*sizeof(deUint32));

		m_testCtx.getLog() << tcu::TestLog::Message << "Binding index buffer to binding point " << bindingPoint << tcu::TestLog::EndMessage;
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_indexBufferID);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating memory for index buffer, size " << sizeToString(bufferSize) << "." << tcu::TestLog::EndMessage;
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, DE_NULL, GL_DYNAMIC_DRAW);
	}

	glu::checkError(gl.getError(), "setup buffers", __FILE__, __LINE__);

	// calculate

	m_testCtx.getLog() << tcu::TestLog::Message << "Dispatching compute, size = " << dispatchSize << tcu::TestLog::EndMessage;
	gl.dispatchCompute(dispatchSize.x(), dispatchSize.y(), dispatchSize.z());

	glu::checkError(gl.getError(), "calculate", __FILE__, __LINE__);
}

class ComputeShaderGeneratedSeparateCase : public ComputeShaderGeneratedCase
{
public:
						ComputeShaderGeneratedSeparateCase	(Context& context, const char* name, const char* desc, DrawMethod method, bool computeCmd, bool computeData, bool computeIndices, int gridSize, int numDrawCalls);
						~ComputeShaderGeneratedSeparateCase	(void);

	void				init								(void);
	void				deinit								(void);

private:
	std::string			genCmdComputeSource					(void);
	std::string			genDataComputeSource				(void);
	std::string			genIndexComputeSource				(void);
	void				runComputeShader					(void);

	glu::ShaderProgram*	m_computeCmdProgram;
	glu::ShaderProgram*	m_computeDataProgram;
	glu::ShaderProgram*	m_computeIndicesProgram;
};

ComputeShaderGeneratedSeparateCase::ComputeShaderGeneratedSeparateCase (Context& context, const char* name, const char* desc, DrawMethod method, bool computeCmd, bool computeData, bool computeIndices, int gridSize, int numDrawCalls)
	: ComputeShaderGeneratedCase	(context, name, desc, method, computeCmd, computeData, computeIndices, gridSize, numDrawCalls)
	, m_computeCmdProgram			(DE_NULL)
	, m_computeDataProgram			(DE_NULL)
	, m_computeIndicesProgram		(DE_NULL)
{
}

ComputeShaderGeneratedSeparateCase::~ComputeShaderGeneratedSeparateCase (void)
{
	deinit();
}

void ComputeShaderGeneratedSeparateCase::init (void)
{
	// generate cmd compute shader

	if (m_computeCmd)
	{
		m_computeCmdProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genCmdComputeSource()));
		m_testCtx.getLog() << *m_computeCmdProgram;

		if (!m_computeCmdProgram->isOk())
			throw tcu::TestError("Failed to compile command compute shader.");
	}

	// generate data compute shader

	if (m_computeData)
	{
		m_computeDataProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genDataComputeSource()));
		m_testCtx.getLog() << *m_computeDataProgram;

		if (!m_computeDataProgram->isOk())
			throw tcu::TestError("Failed to compile data compute shader.");
	}

	// generate index compute shader

	if (m_computeIndices)
	{
		m_computeIndicesProgram = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(genIndexComputeSource()));
		m_testCtx.getLog() << *m_computeIndicesProgram;

		if (!m_computeIndicesProgram->isOk())
			throw tcu::TestError("Failed to compile data compute shader.");
	}

	// init parent
	ComputeShaderGeneratedCase::init();
}

void ComputeShaderGeneratedSeparateCase::deinit (void)
{
	// deinit parent
	ComputeShaderGeneratedCase::deinit();

	if (m_computeCmdProgram)
	{
		delete m_computeCmdProgram;
		m_computeCmdProgram = DE_NULL;
	}
	if (m_computeDataProgram)
	{
		delete m_computeDataProgram;
		m_computeDataProgram = DE_NULL;
	}
	if (m_computeIndicesProgram)
	{
		delete m_computeIndicesProgram;
		m_computeIndicesProgram = DE_NULL;
	}
}

std::string ComputeShaderGeneratedSeparateCase::genCmdComputeSource (void)
{
	return ComputeShaderGeneratedCase::genComputeSource(true, false, false);
}

std::string ComputeShaderGeneratedSeparateCase::genDataComputeSource (void)
{
	return ComputeShaderGeneratedCase::genComputeSource(false, true, false);
}

std::string ComputeShaderGeneratedSeparateCase::genIndexComputeSource (void)
{
	return ComputeShaderGeneratedCase::genComputeSource(false, false, true);
}

void ComputeShaderGeneratedSeparateCase::runComputeShader (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// Compute command

	if (m_computeCmd)
	{
		const int				bindingPoint			= 0;
		const tcu::IVec3		commandDispatchSize		(m_numDrawCmds, 1, 1);
		const int				bufferSize				= m_commandSize * m_numDrawCmds;

		gl.useProgram(m_computeCmdProgram->getProgram());

		// setup buffers

		m_testCtx.getLog() << tcu::TestLog::Message << "Binding command buffer to binding point " << bindingPoint << tcu::TestLog::EndMessage;
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_cmdBufferID);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating memory for command buffer, size " << sizeToString(bufferSize) << "." << tcu::TestLog::EndMessage;
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, DE_NULL, GL_DYNAMIC_DRAW);

		// calculate

		m_testCtx.getLog() << tcu::TestLog::Message << "Dispatching command compute, size = " << commandDispatchSize << tcu::TestLog::EndMessage;
		gl.dispatchCompute(commandDispatchSize.x(), commandDispatchSize.y(), commandDispatchSize.z());

		glu::checkError(gl.getError(), "calculate cmd", __FILE__, __LINE__);
	}

	// Compute data

	if (m_computeData)
	{
		const int				bindingPoint						= 0;
		const tcu::IVec3		drawElementsDataBufferDispatchSize	(m_gridSize+1, m_gridSize+1, 1);
		const tcu::IVec3		drawArraysDataBufferDispatchSize	(m_gridSize,   m_gridSize,   1);
		const tcu::IVec3		dataBufferDispatchSize				= (m_drawMethod == DRAWMETHOD_DRAWELEMENTS) ? (drawElementsDataBufferDispatchSize) : (drawArraysDataBufferDispatchSize);
		const int				bufferSize							= (int)(calcDrawBufferSize()*sizeof(tcu::Vec4));

		gl.useProgram(m_computeDataProgram->getProgram());

		// setup buffers

		m_testCtx.getLog() << tcu::TestLog::Message << "Binding data buffer to binding point " << bindingPoint << tcu::TestLog::EndMessage;
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_dataBufferID);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating memory for data buffer, size " << sizeToString(bufferSize) << "." << tcu::TestLog::EndMessage;
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, DE_NULL, GL_DYNAMIC_DRAW);

		// calculate

		m_testCtx.getLog() << tcu::TestLog::Message << "Dispatching data compute, size = " << dataBufferDispatchSize << tcu::TestLog::EndMessage;
		gl.dispatchCompute(dataBufferDispatchSize.x(), dataBufferDispatchSize.y(), dataBufferDispatchSize.z());

		glu::checkError(gl.getError(), "calculate data", __FILE__, __LINE__);
	}

	// Compute indices

	if (m_computeIndices)
	{
		const int				bindingPoint				= 0;
		const tcu::IVec3		indexBufferDispatchSize		(m_gridSize, m_gridSize, 1);
		const int				bufferSize					= (int)(calcIndexBufferSize()*sizeof(deUint32));

		DE_ASSERT(m_drawMethod == DRAWMETHOD_DRAWELEMENTS);

		gl.useProgram(m_computeIndicesProgram->getProgram());

		// setup buffers

		m_testCtx.getLog() << tcu::TestLog::Message << "Binding index buffer to binding point " << bindingPoint << tcu::TestLog::EndMessage;
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_indexBufferID);

		m_testCtx.getLog() << tcu::TestLog::Message << "Allocating memory for index buffer, size " << sizeToString(bufferSize) << "." << tcu::TestLog::EndMessage;
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, DE_NULL, GL_DYNAMIC_DRAW);

		// calculate

		m_testCtx.getLog() << tcu::TestLog::Message << "Dispatching index compute, size = " << indexBufferDispatchSize << tcu::TestLog::EndMessage;
		gl.dispatchCompute(indexBufferDispatchSize.x(), indexBufferDispatchSize.y(), indexBufferDispatchSize.z());

		glu::checkError(gl.getError(), "calculate indices", __FILE__, __LINE__);
	}

	glu::checkError(gl.getError(), "post dispatch", __FILE__, __LINE__);
}

class ComputeShaderGeneratedGroup : public TestCaseGroup
{
public:
			ComputeShaderGeneratedGroup		(Context& context, const char* name, const char* descr);
			~ComputeShaderGeneratedGroup	(void);

	void	init							(void);
};

ComputeShaderGeneratedGroup::ComputeShaderGeneratedGroup (Context& context, const char* name, const char* descr)
	: TestCaseGroup	(context, name, descr)
{
}

ComputeShaderGeneratedGroup::~ComputeShaderGeneratedGroup (void)
{
}

void ComputeShaderGeneratedGroup::init (void)
{
	const int					gridSize		= 8;
	tcu::TestCaseGroup* const	separateGroup	= new tcu::TestCaseGroup(m_testCtx, "separate", "Use separate compute shaders for each buffer");
	tcu::TestCaseGroup* const	combinedGroup	= new tcu::TestCaseGroup(m_testCtx, "combined", "Use combined compute shader for all buffers");
	tcu::TestCaseGroup* const	largeGroup		= new tcu::TestCaseGroup(m_testCtx, "large",   "Draw shapes with large buffers");

	this->addChild(separateGroup);
	this->addChild(combinedGroup);
	this->addChild(largeGroup);

	// .separate
	{
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawarrays_compute_cmd",							"Command from compute shader",						ComputeShaderGeneratedCase::DRAWMETHOD_DRAWARRAYS,		true,	false,	false,	gridSize,	1));
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawarrays_compute_data",						"Data from compute shader",							ComputeShaderGeneratedCase::DRAWMETHOD_DRAWARRAYS,		false,	true,	false,	gridSize,	1));
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawarrays_compute_cmd_and_data",				"Command and data from compute shader",				ComputeShaderGeneratedCase::DRAWMETHOD_DRAWARRAYS,		true,	true,	false,	gridSize,	1));

		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawelements_compute_cmd",						"Command from compute shader",						ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	true,	false,	false,	gridSize,	1));
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawelements_compute_data",						"Data from compute shader",							ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	false,	true,	false,	gridSize,	1));
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawelements_compute_indices",					"Indices from compute shader",						ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	false,	false,	true,	gridSize,	1));
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawelements_compute_cmd_and_data",				"Command and data from compute shader",				ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	true,	true,	false,	gridSize,	1));
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawelements_compute_cmd_and_indices",			"Command and indices from compute shader",			ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	true,	false,	true,	gridSize,	1));
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawelements_compute_data_and_indices",			"Data and indices from compute shader",				ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	false,	true,	true,	gridSize,	1));
		separateGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, "drawelements_compute_cmd_and_data_and_indices",	"Command, data and indices from compute shader",	ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	true,	true,	true,	gridSize,	1));
	}

	// .combined
	{
		combinedGroup->addChild(new ComputeShaderGeneratedCombinedCase(m_context, "drawarrays_compute_cmd_and_data",				"Command and data from compute shader",				ComputeShaderGeneratedCase::DRAWMETHOD_DRAWARRAYS,		true,	true,	false,	gridSize,	1));
		combinedGroup->addChild(new ComputeShaderGeneratedCombinedCase(m_context, "drawelements_compute_cmd_and_data",				"Command and data from compute shader",				ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	true,	true,	false,	gridSize,	1));
		combinedGroup->addChild(new ComputeShaderGeneratedCombinedCase(m_context, "drawelements_compute_cmd_and_indices",			"Command and indices from compute shader",			ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	true,	false,	true,	gridSize,	1));
		combinedGroup->addChild(new ComputeShaderGeneratedCombinedCase(m_context, "drawelements_compute_data_and_indices",			"Data and indices from compute shader",				ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	false,	true,	true,	gridSize,	1));
		combinedGroup->addChild(new ComputeShaderGeneratedCombinedCase(m_context, "drawelements_compute_cmd_and_data_and_indices",	"Command, data and indices from compute shader",	ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	true,	true,	true,	gridSize,	1));
	}

	// .large
	{
		struct TestSpec
		{
			int gridSize;
			int numDrawCommands;
		};
		struct TestMethod
		{
			ComputeShaderGeneratedCase::DrawMethod method;
			bool                                   separateCompute;
		};

		static const TestSpec specs[] =
		{
			{ 100,	1 },		// !< drawArrays array size ~ 1.9 MB
			{ 200,	1 },		// !< drawArrays array size ~ 7.7 MB
			{ 500,	1 },		// !< drawArrays array size ~ 48 MB
			{ 1000,	1 },		// !< drawArrays array size ~ 192 MB
			{ 1200,	1 },		// !< drawArrays array size ~ 277 MB
			{ 1500,	1 },		// !< drawArrays array size ~ 430 MB

			{ 100,	8 },		// !< drawArrays array size ~ 1.9 MB
			{ 200,	8 },		// !< drawArrays array size ~ 7.7 MB
			{ 500,	8 },		// !< drawArrays array size ~ 48 MB
			{ 1000,	8 },		// !< drawArrays array size ~ 192 MB
			{ 1200,	8 },		// !< drawArrays array size ~ 277 MB
			{ 1500,	8 },		// !< drawArrays array size ~ 430 MB

			{ 100,	200  },		// !< 50 cells per draw call
			{ 200,	800  },		// !< 50 cells per draw call
			{ 500,	2500 },		// !< 100 cells per draw call
			{ 1000,	5000 },		// !< 250 cells per draw call
		};
		static const TestMethod methods[] =
		{
			{ ComputeShaderGeneratedCase::DRAWMETHOD_DRAWARRAYS,	true	},
			{ ComputeShaderGeneratedCase::DRAWMETHOD_DRAWARRAYS,	false	},
			{ ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	true	},
			{ ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS,	false	},
		};

		for (int methodNdx = 0; methodNdx < DE_LENGTH_OF_ARRAY(methods); ++methodNdx)
		for (int specNdx = 0; specNdx < DE_LENGTH_OF_ARRAY(specs); ++specNdx)
		{
			const std::string name = std::string("")
									+ ((methods[methodNdx].method == ComputeShaderGeneratedCase::DRAWMETHOD_DRAWARRAYS) ? ("drawarrays") : ("drawelements"))
									+ ((methods[methodNdx].separateCompute) ? ("_separate") : ("_combined"))
									+ "_grid_" + de::toString(specs[specNdx].gridSize) + "x" + de::toString(specs[specNdx].gridSize)
									+ "_drawcount_" + de::toString(specs[specNdx].numDrawCommands);

			const std::string desc = std::string("Draw grid with ")
									+ ((methods[methodNdx].method == ComputeShaderGeneratedCase::DRAWMETHOD_DRAWARRAYS) ? ("drawarrays indirect") : ("drawelements indirect"))
									+ " calculating buffers in " + ((methods[methodNdx].separateCompute) ? ("separate") : ("combined")) + " compute shader."
									+ " Grid size is " + de::toString(specs[specNdx].gridSize) + "x" + de::toString(specs[specNdx].gridSize)
									+ ", draw count is "  + de::toString(specs[specNdx].numDrawCommands);

			const bool computeIndices = (methods[methodNdx].method == ComputeShaderGeneratedCase::DRAWMETHOD_DRAWELEMENTS);

			if (methods[methodNdx].separateCompute)
				largeGroup->addChild(new ComputeShaderGeneratedSeparateCase(m_context, name.c_str(), desc.c_str(), methods[methodNdx].method, false, true, computeIndices, specs[specNdx].gridSize, specs[specNdx].numDrawCommands));
			else
				largeGroup->addChild(new ComputeShaderGeneratedCombinedCase(m_context, name.c_str(), desc.c_str(), methods[methodNdx].method, false, true, computeIndices, specs[specNdx].gridSize, specs[specNdx].numDrawCommands));
		}
	}
}

class RandomGroup : public TestCaseGroup
{
public:
			RandomGroup		(Context& context, const char* name, const char* descr);
			~RandomGroup	(void);

	void	init			(void);
};

template <int SIZE>
struct UniformWeightArray
{
	float weights[SIZE];

	UniformWeightArray (void)
	{
		for (int i=0; i<SIZE; ++i)
			weights[i] = 1.0f;
	}
};

RandomGroup::RandomGroup (Context& context, const char* name, const char* descr)
	: TestCaseGroup	(context, name, descr)
{
}

RandomGroup::~RandomGroup (void)
{
}

void RandomGroup::init (void)
{
	const int	numAttempts				= 100;

	const int	attribCounts[]			= { 1,   2,   5 };
	const float	attribWeights[]			= { 30, 10,   1 };
	const int	primitiveCounts[]		= { 1,   5,  64 };
	const float	primitiveCountWeights[]	= { 20, 10,   1 };
	const int	indexOffsets[]			= { 0,   7,  13 };
	const float	indexOffsetWeights[]	= { 20, 20,   1 };
	const int	firsts[]				= { 0,   7,  13 };
	const float	firstWeights[]			= { 20, 20,   1 };

	const int	instanceCounts[]		= { 1,   2,  16,  17 };
	const float	instanceWeights[]		= { 20, 10,   5,   1 };
	const int	indexMins[]				= { 0,   1,   3,   8 };
	const int	indexMaxs[]				= { 4,   8, 128, 257 };
	const float	indexWeights[]			= { 50, 50,  50,  50 };
	const int	offsets[]				= { 0,   1,   5,  12 };
	const float	offsetWeights[]			= { 50, 10,  10,  10 };
	const int	strides[]				= { 0,   7,  16,  17 };
	const float	strideWeights[]			= { 50, 10,  10,  10 };
	const int	instanceDivisors[]		= { 0,   1,   3, 129 };
	const float	instanceDivisorWeights[]= { 70, 30,  10,  10 };

	const int	indirectOffsets[]		= { 0,   1,   2 };
	const float indirectOffsetWeigths[]	= { 2,   1,   1 };
	const int	baseVertices[]			= { 0,   1,  -2,   4,  3 };
	const float baseVertexWeigths[]		= { 4,   1,   1,   1,  1 };

	gls::DrawTestSpec::Primitive primitives[] =
	{
		gls::DrawTestSpec::PRIMITIVE_POINTS,
		gls::DrawTestSpec::PRIMITIVE_TRIANGLES,
		gls::DrawTestSpec::PRIMITIVE_TRIANGLE_FAN,
		gls::DrawTestSpec::PRIMITIVE_TRIANGLE_STRIP,
		gls::DrawTestSpec::PRIMITIVE_LINES,
		gls::DrawTestSpec::PRIMITIVE_LINE_STRIP,
		gls::DrawTestSpec::PRIMITIVE_LINE_LOOP
	};
	const UniformWeightArray<DE_LENGTH_OF_ARRAY(primitives)> primitiveWeights;

	gls::DrawTestSpec::DrawMethod drawMethods[] =
	{
		gls::DrawTestSpec::DRAWMETHOD_DRAWARRAYS_INDIRECT,
		gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_INDIRECT,
	};
	const UniformWeightArray<DE_LENGTH_OF_ARRAY(drawMethods)> drawMethodWeights;

	gls::DrawTestSpec::IndexType indexTypes[] =
	{
		gls::DrawTestSpec::INDEXTYPE_BYTE,
		gls::DrawTestSpec::INDEXTYPE_SHORT,
		gls::DrawTestSpec::INDEXTYPE_INT,
	};
	const UniformWeightArray<DE_LENGTH_OF_ARRAY(indexTypes)> indexTypeWeights;

	gls::DrawTestSpec::InputType inputTypes[] =
	{
		gls::DrawTestSpec::INPUTTYPE_FLOAT,
		gls::DrawTestSpec::INPUTTYPE_FIXED,
		gls::DrawTestSpec::INPUTTYPE_BYTE,
		gls::DrawTestSpec::INPUTTYPE_SHORT,
		gls::DrawTestSpec::INPUTTYPE_UNSIGNED_BYTE,
		gls::DrawTestSpec::INPUTTYPE_UNSIGNED_SHORT,
		gls::DrawTestSpec::INPUTTYPE_INT,
		gls::DrawTestSpec::INPUTTYPE_UNSIGNED_INT,
		gls::DrawTestSpec::INPUTTYPE_HALF,
		gls::DrawTestSpec::INPUTTYPE_UNSIGNED_INT_2_10_10_10,
		gls::DrawTestSpec::INPUTTYPE_INT_2_10_10_10,
	};
	const UniformWeightArray<DE_LENGTH_OF_ARRAY(inputTypes)> inputTypeWeights;

	gls::DrawTestSpec::OutputType outputTypes[] =
	{
		gls::DrawTestSpec::OUTPUTTYPE_FLOAT,
		gls::DrawTestSpec::OUTPUTTYPE_VEC2,
		gls::DrawTestSpec::OUTPUTTYPE_VEC3,
		gls::DrawTestSpec::OUTPUTTYPE_VEC4,
		gls::DrawTestSpec::OUTPUTTYPE_INT,
		gls::DrawTestSpec::OUTPUTTYPE_UINT,
		gls::DrawTestSpec::OUTPUTTYPE_IVEC2,
		gls::DrawTestSpec::OUTPUTTYPE_IVEC3,
		gls::DrawTestSpec::OUTPUTTYPE_IVEC4,
		gls::DrawTestSpec::OUTPUTTYPE_UVEC2,
		gls::DrawTestSpec::OUTPUTTYPE_UVEC3,
		gls::DrawTestSpec::OUTPUTTYPE_UVEC4,
	};
	const UniformWeightArray<DE_LENGTH_OF_ARRAY(outputTypes)> outputTypeWeights;

	gls::DrawTestSpec::Usage usages[] =
	{
		gls::DrawTestSpec::USAGE_DYNAMIC_DRAW,
		gls::DrawTestSpec::USAGE_STATIC_DRAW,
		gls::DrawTestSpec::USAGE_STREAM_DRAW,
		gls::DrawTestSpec::USAGE_STREAM_READ,
		gls::DrawTestSpec::USAGE_STREAM_COPY,
		gls::DrawTestSpec::USAGE_STATIC_READ,
		gls::DrawTestSpec::USAGE_STATIC_COPY,
		gls::DrawTestSpec::USAGE_DYNAMIC_READ,
		gls::DrawTestSpec::USAGE_DYNAMIC_COPY,
	};
	const UniformWeightArray<DE_LENGTH_OF_ARRAY(usages)> usageWeights;

	std::set<deUint32>	insertedHashes;
	size_t				insertedCount = 0;

	for (int ndx = 0; ndx < numAttempts; ++ndx)
	{
		de::Random random(0xc551393 + ndx); // random does not depend on previous cases

		int					attributeCount = random.chooseWeighted<int, const int*, const float*>(DE_ARRAY_BEGIN(attribCounts), DE_ARRAY_END(attribCounts), attribWeights);
		int					drawCommandSize;
		gls::DrawTestSpec	spec;

		spec.apiType				= glu::ApiType::es(3,1);
		spec.primitive				= random.chooseWeighted<gls::DrawTestSpec::Primitive>	(DE_ARRAY_BEGIN(primitives),		DE_ARRAY_END(primitives),		primitiveWeights.weights);
		spec.primitiveCount			= random.chooseWeighted<int, const int*, const float*>	(DE_ARRAY_BEGIN(primitiveCounts),	DE_ARRAY_END(primitiveCounts),	primitiveCountWeights);
		spec.drawMethod				= random.chooseWeighted<gls::DrawTestSpec::DrawMethod>	(DE_ARRAY_BEGIN(drawMethods),		DE_ARRAY_END(drawMethods),		drawMethodWeights.weights);

		if (spec.drawMethod == gls::DrawTestSpec::DRAWMETHOD_DRAWARRAYS_INDIRECT)
			drawCommandSize = sizeof(deUint32[4]);
		else if (spec.drawMethod == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_INDIRECT)
			drawCommandSize = sizeof(deUint32[5]);
		else
		{
			DE_ASSERT(DE_FALSE);
			return;
		}

		spec.indexType				= random.chooseWeighted<gls::DrawTestSpec::IndexType>	(DE_ARRAY_BEGIN(indexTypes),		DE_ARRAY_END(indexTypes),		indexTypeWeights.weights);
		spec.indexPointerOffset		= random.chooseWeighted<int, const int*, const float*>	(DE_ARRAY_BEGIN(indexOffsets),		DE_ARRAY_END(indexOffsets),		indexOffsetWeights);
		spec.indexStorage			= gls::DrawTestSpec::STORAGE_BUFFER;
		spec.first					= random.chooseWeighted<int, const int*, const float*>	(DE_ARRAY_BEGIN(firsts),			DE_ARRAY_END(firsts),			firstWeights);
		spec.indexMin				= random.chooseWeighted<int, const int*, const float*>	(DE_ARRAY_BEGIN(indexMins),			DE_ARRAY_END(indexMins),		indexWeights);
		spec.indexMax				= random.chooseWeighted<int, const int*, const float*>	(DE_ARRAY_BEGIN(indexMaxs),			DE_ARRAY_END(indexMaxs),		indexWeights);
		spec.instanceCount			= random.chooseWeighted<int, const int*, const float*>	(DE_ARRAY_BEGIN(instanceCounts),	DE_ARRAY_END(instanceCounts),	instanceWeights);
		spec.indirectOffset			= random.chooseWeighted<int, const int*, const float*>	(DE_ARRAY_BEGIN(indirectOffsets),	DE_ARRAY_END(indirectOffsets),	indirectOffsetWeigths) * drawCommandSize;
		spec.baseVertex				= random.chooseWeighted<int, const int*, const float*>	(DE_ARRAY_BEGIN(baseVertices),		DE_ARRAY_END(baseVertices),		baseVertexWeigths);

		// check spec is legal
		if (!spec.valid())
			continue;

		for (int attrNdx = 0; attrNdx < attributeCount;)
		{
			bool valid;
			gls::DrawTestSpec::AttributeSpec attribSpec;

			attribSpec.inputType			= random.chooseWeighted<gls::DrawTestSpec::InputType>	(DE_ARRAY_BEGIN(inputTypes),		DE_ARRAY_END(inputTypes),		inputTypeWeights.weights);
			attribSpec.outputType			= random.chooseWeighted<gls::DrawTestSpec::OutputType>	(DE_ARRAY_BEGIN(outputTypes),		DE_ARRAY_END(outputTypes),		outputTypeWeights.weights);
			attribSpec.storage				= gls::DrawTestSpec::STORAGE_BUFFER;
			attribSpec.usage				= random.chooseWeighted<gls::DrawTestSpec::Usage>		(DE_ARRAY_BEGIN(usages),			DE_ARRAY_END(usages),			usageWeights.weights);
			attribSpec.componentCount		= random.getInt(1, 4);
			attribSpec.offset				= random.chooseWeighted<int, const int*, const float*>(DE_ARRAY_BEGIN(offsets), DE_ARRAY_END(offsets), offsetWeights);
			attribSpec.stride				= random.chooseWeighted<int, const int*, const float*>(DE_ARRAY_BEGIN(strides), DE_ARRAY_END(strides), strideWeights);
			attribSpec.normalize			= random.getBool();
			attribSpec.instanceDivisor		= random.chooseWeighted<int, const int*, const float*>(DE_ARRAY_BEGIN(instanceDivisors), DE_ARRAY_END(instanceDivisors), instanceDivisorWeights);
			attribSpec.useDefaultAttribute	= random.getBool();

			// check spec is legal
			valid = attribSpec.valid(spec.apiType);

			// we do not want interleaved elements. (Might result in some weird floating point values)
			if (attribSpec.stride && attribSpec.componentCount * gls::DrawTestSpec::inputTypeSize(attribSpec.inputType) > attribSpec.stride)
				valid = false;

			// try again if not valid
			if (valid)
			{
				spec.attribs.push_back(attribSpec);
				++attrNdx;
			}
		}

		// Do not collapse all vertex positions to a single positions
		if (spec.primitive != gls::DrawTestSpec::PRIMITIVE_POINTS)
			spec.attribs[0].instanceDivisor = 0;

		// Is render result meaningful?
		{
			// Only one vertex
			if (spec.drawMethod == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_RANGED && spec.indexMin == spec.indexMax && spec.primitive != gls::DrawTestSpec::PRIMITIVE_POINTS)
				continue;
			if (spec.attribs[0].useDefaultAttribute && spec.primitive != gls::DrawTestSpec::PRIMITIVE_POINTS)
				continue;

			// Triangle only on one axis
			if (spec.primitive == gls::DrawTestSpec::PRIMITIVE_TRIANGLES || spec.primitive == gls::DrawTestSpec::PRIMITIVE_TRIANGLE_FAN || spec.primitive == gls::DrawTestSpec::PRIMITIVE_TRIANGLE_STRIP)
			{
				if (spec.attribs[0].componentCount == 1)
					continue;
				if (spec.attribs[0].outputType == gls::DrawTestSpec::OUTPUTTYPE_FLOAT || spec.attribs[0].outputType == gls::DrawTestSpec::OUTPUTTYPE_INT || spec.attribs[0].outputType == gls::DrawTestSpec::OUTPUTTYPE_UINT)
					continue;
				if (spec.drawMethod == gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_RANGED && (spec.indexMax - spec.indexMin) < 2)
					continue;
			}
		}

		// Add case
		{
			deUint32 hash = spec.hash();
			for (int attrNdx = 0; attrNdx < attributeCount; ++attrNdx)
				hash = (hash << 2) ^ (deUint32)spec.attribs[attrNdx].hash();

			if (insertedHashes.find(hash) == insertedHashes.end())
			{
				// Only aligned cases
				if (spec.isCompatibilityTest() != gls::DrawTestSpec::COMPATIBILITY_UNALIGNED_OFFSET &&
					spec.isCompatibilityTest() != gls::DrawTestSpec::COMPATIBILITY_UNALIGNED_STRIDE)
					this->addChild(new gls::DrawTest(m_testCtx, m_context.getRenderContext(), spec, de::toString(insertedCount).c_str(), spec.getDesc().c_str()));
				insertedHashes.insert(hash);

				++insertedCount;
			}
		}
	}
}

class BadCommandBufferCase : public TestCase
{
public:
	enum
	{
		CommandSize = 20
	};

					BadCommandBufferCase	(Context& context, const char* name, const char* desc, deUint32 alignment, deUint32 bufferSize, bool writeCommandToBuffer, deUint32 m_expectedError);
					~BadCommandBufferCase	(void);

	IterateResult	iterate					(void);

private:
	const deUint32	m_alignment;
	const deUint32	m_bufferSize;
	const bool		m_writeCommandToBuffer;
	const deUint32	m_expectedError;
};

BadCommandBufferCase::BadCommandBufferCase (Context& context, const char* name, const char* desc, deUint32 alignment, deUint32 bufferSize, bool writeCommandToBuffer, deUint32 expectedError)
	: TestCase					(context, name, desc)
	, m_alignment				(alignment)
	, m_bufferSize				(bufferSize)
	, m_writeCommandToBuffer	(writeCommandToBuffer)
	, m_expectedError			(expectedError)
{
}

BadCommandBufferCase::~BadCommandBufferCase (void)
{
}

BadCommandBufferCase::IterateResult	BadCommandBufferCase::iterate (void)
{
	const tcu::Vec4 vertexPositions[] =
	{
		tcu::Vec4(0,	0,		0, 1),
		tcu::Vec4(1,	0,		0, 1),
		tcu::Vec4(0,	1,		0, 1),
	};

	const deUint16 indices[] =
	{
		0, 2, 1,
	};

	DE_STATIC_ASSERT(CommandSize == sizeof(DrawElementsCommand));

	sglr::GLContext gl(m_context.getRenderContext(), m_testCtx.getLog(), sglr::GLCONTEXT_LOG_CALLS, tcu::IVec4(0, 0, 1, 1));

	deUint32 vaoID			= 0;
	deUint32 positionBuf	= 0;
	deUint32 indexBuf		= 0;
	deUint32 drawIndirectBuf= 0;
	deUint32 error;

	glu::ShaderProgram	program			(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_commonVertexShaderSource) << glu::FragmentSource(s_commonFragmentShaderSource));
	deUint32			programID		= program.getProgram();
	deInt32				posLocation		= gl.getAttribLocation(programID, "a_position");

	DrawElementsCommand drawCommand;
	drawCommand.count				= 3;
	drawCommand.primCount			= 1;
	drawCommand.firstIndex			= 0;
	drawCommand.baseVertex			= 0;
	drawCommand.reservedMustBeZero	= 0;

	std::vector<deInt8> drawCommandBuffer;
	drawCommandBuffer.resize(m_bufferSize);

	deMemset(&drawCommandBuffer[0], 0, (int)drawCommandBuffer.size());

	if (m_writeCommandToBuffer)
	{
		DE_ASSERT(drawCommandBuffer.size() >= sizeof(drawCommand) + m_alignment);
		deMemcpy(&drawCommandBuffer[m_alignment], &drawCommand, sizeof(drawCommand));
	}

	glu::checkError(gl.getError(), "", __FILE__, __LINE__);
	gl.genVertexArrays(1, &vaoID);
	gl.bindVertexArray(vaoID);

	gl.genBuffers(1, &positionBuf);
	gl.bindBuffer(GL_ARRAY_BUFFER, positionBuf);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.vertexAttribDivisor(posLocation, 0);
	gl.enableVertexAttribArray(posLocation);
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	gl.genBuffers(1, &indexBuf);
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	gl.genBuffers(1, &drawIndirectBuf);
	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, drawIndirectBuf);
	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, drawCommandBuffer.size(), &drawCommandBuffer[0], GL_STATIC_DRAW);
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	gl.viewport(0, 0, 1, 1);

	gl.useProgram(programID);
	gl.drawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (const void*)(deUintptr)m_alignment);

	error = gl.getError();

	gl.useProgram(0);

	gl.deleteBuffers(1, &drawIndirectBuf);
	gl.deleteBuffers(1, &indexBuf);
	gl.deleteBuffers(1, &positionBuf);
	gl.deleteVertexArrays(1, &vaoID);

	m_testCtx.getLog() << tcu::TestLog::Message << "drawElementsIndirect generated " << glu::getErrorStr(error) << ", expecting " << glu::getErrorStr(m_expectedError) << "." << tcu::TestLog::EndMessage;

	if (error == m_expectedError)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "\tUnexpected error." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got unexpected error.");
	}

	return STOP;
}

class BadAlignmentCase : public BadCommandBufferCase
{
public:
					BadAlignmentCase	(Context& context, const char* name, const char* desc, deUint32 alignment);
					~BadAlignmentCase	(void);
};

BadAlignmentCase::BadAlignmentCase (Context& context, const char* name, const char* desc, deUint32 alignment)
	: BadCommandBufferCase(context, name, desc, alignment, CommandSize+alignment, true, GL_INVALID_VALUE)
{
}

BadAlignmentCase::~BadAlignmentCase (void)
{
}

class BadBufferRangeCase : public BadCommandBufferCase
{
public:
					BadBufferRangeCase	(Context& context, const char* name, const char* desc, deUint32 offset);
					~BadBufferRangeCase	(void);
};

BadBufferRangeCase::BadBufferRangeCase (Context& context, const char* name, const char* desc, deUint32 offset)
	: BadCommandBufferCase(context, name, desc, offset, CommandSize, false, GL_INVALID_OPERATION)
{
}

BadBufferRangeCase::~BadBufferRangeCase (void)
{
}

class BadStateCase : public TestCase
{
public:
	enum CaseType
	{
		CASE_CLIENT_BUFFER_VERTEXATTR = 0,
		CASE_CLIENT_BUFFER_COMMAND,
		CASE_DEFAULT_VAO,

		CASE_CLIENT_LAST
	};

						BadStateCase	(Context& context, const char* name, const char* desc, CaseType type);
						~BadStateCase	(void);

	void				init			(void);
	void				deinit			(void);
	IterateResult		iterate			(void);

private:
	const CaseType		m_caseType;
};

BadStateCase::BadStateCase (Context& context, const char* name, const char* desc, CaseType type)
	: TestCase			(context, name, desc)
	, m_caseType		(type)
{
	DE_ASSERT(type < CASE_CLIENT_LAST);
}

BadStateCase::~BadStateCase (void)
{
	deinit();
}

void BadStateCase::init (void)
{
}

void BadStateCase::deinit (void)
{
}

BadStateCase::IterateResult BadStateCase::iterate (void)
{
	const tcu::Vec4 vertexPositions[] =
	{
		tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f),
	};

	const deUint16 indices[] =
	{
		0, 2, 1,
	};

	sglr::GLContext gl(m_context.getRenderContext(), m_testCtx.getLog(), sglr::GLCONTEXT_LOG_CALLS, tcu::IVec4(0, 0, 1, 1));

	deUint32			error;
	glu::ShaderProgram	program			(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_commonVertexShaderSource) << glu::FragmentSource(s_commonFragmentShaderSource));
	deUint32			vaoID			= 0;
	deUint32			dataBufferID	= 0;
	deUint32			indexBufferID	= 0;
	deUint32			cmdBufferID		= 0;

	const deUint32		programID		= program.getProgram();
	const deInt32		posLocation		= gl.getAttribLocation(programID, "a_position");

	DrawElementsCommand drawCommand;
	drawCommand.count				= 3;
	drawCommand.primCount			= 1;
	drawCommand.firstIndex			= 0;
	drawCommand.baseVertex			= 0;
	drawCommand.reservedMustBeZero	= 0;

	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	if (m_caseType == CASE_CLIENT_BUFFER_VERTEXATTR)
	{
		// \note We use default VAO since we use client pointers. Trying indirect draw with default VAO is also an error. => This test does two illegal operations

		gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, vertexPositions);
		gl.enableVertexAttribArray(posLocation);
		glu::checkError(gl.getError(), "", __FILE__, __LINE__);
	}
	else if (m_caseType == CASE_CLIENT_BUFFER_COMMAND)
	{
		gl.genVertexArrays(1, &vaoID);
		gl.bindVertexArray(vaoID);

		gl.genBuffers(1, &dataBufferID);
		gl.bindBuffer(GL_ARRAY_BUFFER, dataBufferID);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
		gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		gl.enableVertexAttribArray(posLocation);
		glu::checkError(gl.getError(), "", __FILE__, __LINE__);
	}
	else if (m_caseType == CASE_DEFAULT_VAO)
	{
		gl.genBuffers(1, &dataBufferID);
		gl.bindBuffer(GL_ARRAY_BUFFER, dataBufferID);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
		gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		gl.enableVertexAttribArray(posLocation);
		glu::checkError(gl.getError(), "", __FILE__, __LINE__);
	}
	else
		DE_ASSERT(DE_FALSE);

	gl.genBuffers(1, &indexBufferID);
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	if (m_caseType != CASE_CLIENT_BUFFER_COMMAND)
	{
		gl.genBuffers(1, &cmdBufferID);
		gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBufferID);
		gl.bufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(drawCommand), &drawCommand, GL_STATIC_DRAW);
		glu::checkError(gl.getError(), "", __FILE__, __LINE__);
	}

	gl.viewport(0, 0, 1, 1);

	gl.useProgram(programID);
	gl.drawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (m_caseType != CASE_CLIENT_BUFFER_COMMAND) ? (DE_NULL) : (&drawCommand));

	error = gl.getError();

	gl.bindVertexArray(0);
	gl.useProgram(0);

	if (error == GL_INVALID_OPERATION)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Unexpected error. Expected GL_INVALID_OPERATION, got " << glu::getErrorStr(error) << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got unexpected error.");
	}

	return STOP;
}

class BadDrawModeCase : public TestCase
{
public:
	enum DrawType
	{
		DRAW_ARRAYS = 0,
		DRAW_ELEMENTS,
		DRAW_ELEMENTS_BAD_INDEX,

		DRAW_LAST
	};

						BadDrawModeCase	(Context& context, const char* name, const char* desc, DrawType type);
						~BadDrawModeCase(void);

	void				init			(void);
	void				deinit			(void);
	IterateResult		iterate			(void);

private:
	const DrawType		m_drawType;
};

BadDrawModeCase::BadDrawModeCase (Context& context, const char* name, const char* desc, DrawType type)
	: TestCase			(context, name, desc)
	, m_drawType		(type)
{
	DE_ASSERT(type < DRAW_LAST);
}

BadDrawModeCase::~BadDrawModeCase (void)
{
	deinit();
}

void BadDrawModeCase::init (void)
{
}

void BadDrawModeCase::deinit (void)
{
}

BadDrawModeCase::IterateResult BadDrawModeCase::iterate (void)
{
	const tcu::Vec4 vertexPositions[] =
	{
		tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
		tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f),
	};

	const deUint16 indices[] =
	{
		0, 2, 1,
	};

	sglr::GLContext		gl				(m_context.getRenderContext(), m_testCtx.getLog(), sglr::GLCONTEXT_LOG_CALLS, tcu::IVec4(0, 0, 1, 1));

	deUint32			error;
	glu::ShaderProgram	program			(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_commonVertexShaderSource) << glu::FragmentSource(s_commonFragmentShaderSource));
	deUint32			vaoID			= 0;
	deUint32			dataBufferID	= 0;
	deUint32			indexBufferID	= 0;
	deUint32			cmdBufferID		= 0;

	const deUint32		programID		= program.getProgram();
	const deInt32		posLocation		= gl.getAttribLocation(programID, "a_position");
	const glw::GLenum	mode			= (m_drawType == DRAW_ELEMENTS_BAD_INDEX) ? (GL_TRIANGLES) : (0x123);
	const glw::GLenum	indexType		= (m_drawType == DRAW_ELEMENTS_BAD_INDEX) ? (0x123) : (GL_UNSIGNED_SHORT);

	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	// vao

	gl.genVertexArrays(1, &vaoID);
	gl.bindVertexArray(vaoID);

	// va

	gl.genBuffers(1, &dataBufferID);
	gl.bindBuffer(GL_ARRAY_BUFFER, dataBufferID);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(posLocation);
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	// index

	if (m_drawType == DRAW_ELEMENTS || m_drawType == DRAW_ELEMENTS_BAD_INDEX)
	{
		gl.genBuffers(1, &indexBufferID);
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		glu::checkError(gl.getError(), "", __FILE__, __LINE__);
	}

	// cmd

	gl.genBuffers(1, &cmdBufferID);
	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBufferID);
	if (m_drawType == DRAW_ELEMENTS || m_drawType == DRAW_ELEMENTS_BAD_INDEX)
	{
		DrawElementsCommand drawCommand;
		drawCommand.count				= 3;
		drawCommand.primCount			= 1;
		drawCommand.firstIndex			= 0;
		drawCommand.baseVertex			= 0;
		drawCommand.reservedMustBeZero	= 0;

		gl.bufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(drawCommand), &drawCommand, GL_STATIC_DRAW);
	}
	else if (m_drawType == DRAW_ARRAYS)
	{
		DrawArraysCommand drawCommand;
		drawCommand.count				= 3;
		drawCommand.primCount			= 1;
		drawCommand.first				= 0;
		drawCommand.reservedMustBeZero	= 0;

		gl.bufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(drawCommand), &drawCommand, GL_STATIC_DRAW);
	}
	else
		DE_ASSERT(DE_FALSE);
	glu::checkError(gl.getError(), "", __FILE__, __LINE__);

	gl.viewport(0, 0, 1, 1);
	gl.useProgram(programID);
	if (m_drawType == DRAW_ELEMENTS || m_drawType == DRAW_ELEMENTS_BAD_INDEX)
		gl.drawElementsIndirect(mode, indexType, DE_NULL);
	else if (m_drawType == DRAW_ARRAYS)
		gl.drawArraysIndirect(mode, DE_NULL);
	else
		DE_ASSERT(DE_FALSE);

	error = gl.getError();
	gl.useProgram(0);

	if (error == GL_INVALID_ENUM)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Unexpected error. Expected GL_INVALID_ENUM, got " << glu::getErrorStr(error) << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got unexpected error.");
	}

	return STOP;
}

class NegativeGroup : public TestCaseGroup
{
public:
			NegativeGroup	(Context& context, const char* name, const char* descr);
			~NegativeGroup	(void);

	void	init			(void);
};

NegativeGroup::NegativeGroup (Context& context, const char* name, const char* descr)
	: TestCaseGroup	(context, name, descr)
{
}

NegativeGroup::~NegativeGroup (void)
{
}

void NegativeGroup::init (void)
{
	// invalid alignment
	addChild(new BadAlignmentCase	(m_context, "command_bad_alignment_1",								"Bad command alignment",					1));
	addChild(new BadAlignmentCase	(m_context, "command_bad_alignment_2",								"Bad command alignment",					2));
	addChild(new BadAlignmentCase	(m_context, "command_bad_alignment_3",								"Bad command alignment",					3));

	// command only partially or not at all in the buffer
	addChild(new BadBufferRangeCase	(m_context, "command_offset_partially_in_buffer",					"Command not fully in the buffer range",	BadBufferRangeCase::CommandSize - 16));
	addChild(new BadBufferRangeCase	(m_context, "command_offset_not_in_buffer",							"Command not in the buffer range",			BadBufferRangeCase::CommandSize));
	addChild(new BadBufferRangeCase	(m_context, "command_offset_not_in_buffer_unsigned32_wrap",			"Command not in the buffer range",			0xFFFFFFFC));
	addChild(new BadBufferRangeCase	(m_context, "command_offset_not_in_buffer_signed32_wrap",			"Command not in the buffer range",			0x7FFFFFFC));

	// use with client data and default vao
	addChild(new BadStateCase		(m_context, "client_vertex_attrib_array",							"Vertex attrib array in the client memory",	BadStateCase::CASE_CLIENT_BUFFER_VERTEXATTR));
	addChild(new BadStateCase		(m_context, "client_command_array",									"Command array in the client memory",		BadStateCase::CASE_CLIENT_BUFFER_COMMAND));
	addChild(new BadStateCase		(m_context, "default_vao",											"Use with default vao",						BadStateCase::CASE_DEFAULT_VAO));

	// invalid mode & type
	addChild(new BadDrawModeCase	(m_context, "invalid_mode_draw_arrays",								"Call DrawArraysIndirect with bad mode",	BadDrawModeCase::DRAW_ARRAYS));
	addChild(new BadDrawModeCase	(m_context, "invalid_mode_draw_elements",							"Call DrawelementsIndirect with bad mode",	BadDrawModeCase::DRAW_ELEMENTS));
	addChild(new BadDrawModeCase	(m_context, "invalid_type_draw_elements",							"Call DrawelementsIndirect with bad type",	BadDrawModeCase::DRAW_ELEMENTS_BAD_INDEX));
}

} // anonymous

DrawTests::DrawTests (Context& context)
	: TestCaseGroup(context, "draw_indirect", "Indirect drawing tests")
{
}

DrawTests::~DrawTests (void)
{
}

void DrawTests::init (void)
{
	// Basic
	{
		const gls::DrawTestSpec::DrawMethod basicMethods[] =
		{
			gls::DrawTestSpec::DRAWMETHOD_DRAWARRAYS_INDIRECT,
			gls::DrawTestSpec::DRAWMETHOD_DRAWELEMENTS_INDIRECT,
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(basicMethods); ++ndx)
		{
			const std::string name = gls::DrawTestSpec::drawMethodToString(basicMethods[ndx]);
			const std::string desc = gls::DrawTestSpec::drawMethodToString(basicMethods[ndx]);

			this->addChild(new MethodGroup(m_context, name.c_str(), desc.c_str(), basicMethods[ndx]));
		}
	}

	// extreme instancing

	this->addChild(new InstancingGroup(m_context, "instancing", "draw tests with a large instance count."));

	// compute shader generated commands

	this->addChild(new ComputeShaderGeneratedGroup(m_context, "compute_interop", "draw tests with a draw command generated in compute shader."));

	// Random

	this->addChild(new RandomGroup(m_context, "random", "random draw commands."));

	// negative

	this->addChild(new NegativeGroup(m_context, "negative", "invalid draw commands with defined error codes."));
}

} // Functional
} // gles31
} // deqp
