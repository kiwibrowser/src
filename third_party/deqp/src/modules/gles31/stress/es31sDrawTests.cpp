
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
 * \brief Drawing stress tests.
 *//*--------------------------------------------------------------------*/

#include "es31sDrawTests.hpp"
#include "glsDrawTest.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include <set>

namespace deqp
{
namespace gles31
{
namespace Stress
{
namespace
{

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

class InvalidDrawCase : public TestCase
{
public:
	enum DrawType
	{
		DRAW_ARRAYS,
		DRAW_ELEMENTS,

		DRAW_LAST
	};
	enum InvalidOperation
	{
		INVALID_DATA_COUNT = 0,
		INVALID_DATA_FIRST,
		INVALID_DATA_INSTANCED,
		INVALID_INDEX_COUNT,
		INVALID_INDEX_FIRST,
		INVALID_RESERVED,
		INVALID_INDEX,

		INVALID_LAST
	};

							InvalidDrawCase		(Context& context, const char* name, const char* desc, DrawType type, InvalidOperation op);
							~InvalidDrawCase	(void);

	void					init				(void);
	void					deinit				(void);
	IterateResult			iterate				(void);

private:
	const DrawType			m_drawType;
	const InvalidOperation	m_op;
	glw::GLuint				m_dataBufferID;
	glw::GLuint				m_indexBufferID;
	glw::GLuint				m_cmdBufferID;
	glw::GLuint				m_colorBufferID;
	glw::GLuint				m_vao;
};

InvalidDrawCase::InvalidDrawCase (Context& context, const char* name, const char* desc, DrawType type, InvalidOperation op)
	: TestCase			(context, name, desc)
	, m_drawType		(type)
	, m_op				(op)
	, m_dataBufferID	(0)
	, m_indexBufferID	(0)
	, m_cmdBufferID		(0)
	, m_colorBufferID	(0)
	, m_vao				(0)
{
	DE_ASSERT(type < DRAW_LAST);
	DE_ASSERT(op < INVALID_LAST);
}

InvalidDrawCase::~InvalidDrawCase (void)
{
	deinit();
}

void InvalidDrawCase::init (void)
{
}

void InvalidDrawCase::deinit (void)
{
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
	if (m_cmdBufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_cmdBufferID);
		m_cmdBufferID = 0;
	}
	if (m_colorBufferID)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_colorBufferID);
		m_colorBufferID = 0;
	}
	if (m_vao)
	{
		m_context.getRenderContext().getFunctions().deleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}
}

InvalidDrawCase::IterateResult InvalidDrawCase::iterate (void)
{
	const int drawCount				= 10;		//!< number of elements safe to draw (all buffers have this)
	const int overBoundDrawCount	= 10000;	//!< number of elements in all other buffers than our target buffer
	const int drawInstances			= 1;
	const int overBoundInstances	= 1000;

	glu::CallLogWrapper gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::ShaderProgram	program			(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(s_colorVertexShaderSource) << glu::FragmentSource(s_colorFragmentShaderSource));
	const deUint32		programID		= program.getProgram();
	const deInt32		posLocation		= gl.glGetAttribLocation(programID, "a_position");
	const deInt32		colorLocation	= gl.glGetAttribLocation(programID, "a_color");

	gl.enableLogging(true);

	gl.glGenVertexArrays(1, &m_vao);
	gl.glBindVertexArray(m_vao);
	glu::checkError(gl.glGetError(), "gen vao", __FILE__, __LINE__);

	// indices
	if (m_drawType == DRAW_ELEMENTS)
	{
		const int				indexBufferSize = (m_op == INVALID_INDEX_COUNT) ? (drawCount) : (overBoundDrawCount);
		std::vector<deUint16>	indices			(indexBufferSize);

		for (int ndx = 0; ndx < (int)indices.size(); ++ndx)
			indices[ndx] = (deUint16)((m_op == INVALID_INDEX) ? (overBoundDrawCount + ndx) : (ndx));

		gl.glGenBuffers(1, &m_indexBufferID);
		gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferID);
		gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, (int)(indices.size() * sizeof(deUint16)), &indices[0], GL_STATIC_DRAW);
		glu::checkError(gl.glGetError(), "", __FILE__, __LINE__);
	}

	// data
	{
		const int dataSize = (m_op == INVALID_DATA_COUNT) ? (drawCount) : (overBoundDrawCount);

		// any data is ok
		gl.glGenBuffers(1, &m_dataBufferID);
		gl.glBindBuffer(GL_ARRAY_BUFFER, m_dataBufferID);
		gl.glBufferData(GL_ARRAY_BUFFER, dataSize * sizeof(float[4]), DE_NULL, GL_STATIC_DRAW);
		gl.glVertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		gl.glEnableVertexAttribArray(posLocation);
		glu::checkError(gl.glGetError(), "", __FILE__, __LINE__);
	}

	// potentially instanced data
	{
		const int dataSize = drawInstances;

		gl.glGenBuffers(1, &m_colorBufferID);
		gl.glBindBuffer(GL_ARRAY_BUFFER, m_colorBufferID);
		gl.glBufferData(GL_ARRAY_BUFFER, dataSize * sizeof(float[4]), DE_NULL, GL_STATIC_DRAW);
		gl.glVertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		gl.glEnableVertexAttribArray(colorLocation);
		gl.glVertexAttribDivisor(colorLocation, 1);
		glu::checkError(gl.glGetError(), "", __FILE__, __LINE__);
	}

	// command
	if (m_drawType == DRAW_ARRAYS)
	{
		DrawArraysCommand drawCommand;
		drawCommand.count				= overBoundDrawCount;
		drawCommand.primCount			= (m_op == INVALID_DATA_INSTANCED)	? (overBoundInstances)	: (drawInstances);
		drawCommand.first				= (m_op == INVALID_DATA_FIRST)		? (overBoundDrawCount)	: (0);
		drawCommand.reservedMustBeZero	= (m_op == INVALID_RESERVED)		? (5)					: (0);

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "drawCommand:"
			<< "\n\tcount:\t" << drawCommand.count
			<< "\n\tprimCount\t" << drawCommand.primCount
			<< "\n\tfirst\t" << drawCommand.first
			<< "\n\treservedMustBeZero\t" << drawCommand.reservedMustBeZero
			<< tcu::TestLog::EndMessage;

		gl.glGenBuffers(1, &m_cmdBufferID);
		gl.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_cmdBufferID);
		gl.glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(drawCommand), &drawCommand, GL_STATIC_DRAW);
		glu::checkError(gl.glGetError(), "", __FILE__, __LINE__);
	}
	else if (m_drawType == DRAW_ELEMENTS)
	{
		DrawElementsCommand drawCommand;
		drawCommand.count				= overBoundDrawCount;
		drawCommand.primCount			= (m_op == INVALID_DATA_INSTANCED)	? (overBoundInstances)	: (drawInstances);
		drawCommand.firstIndex			= (m_op == INVALID_INDEX_FIRST)		? (overBoundDrawCount)	: (0);
		drawCommand.baseVertex			= (m_op == INVALID_DATA_FIRST)		? (overBoundDrawCount)	: (0);
		drawCommand.reservedMustBeZero	= (m_op == INVALID_RESERVED)		? (5)					: (0);

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "drawCommand:"
			<< "\n\tcount:\t" << drawCommand.count
			<< "\n\tprimCount\t" << drawCommand.primCount
			<< "\n\tfirstIndex\t" << drawCommand.firstIndex
			<< "\n\tbaseVertex\t" << drawCommand.baseVertex
			<< "\n\treservedMustBeZero\t" << drawCommand.reservedMustBeZero
			<< tcu::TestLog::EndMessage;

		gl.glGenBuffers(1, &m_cmdBufferID);
		gl.glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_cmdBufferID);
		gl.glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(drawCommand), &drawCommand, GL_STATIC_DRAW);
		glu::checkError(gl.glGetError(), "", __FILE__, __LINE__);
	}
	else
		DE_ASSERT(DE_FALSE);

	gl.glViewport(0, 0, 1, 1);
	gl.glUseProgram(programID);

	if (m_drawType == DRAW_ELEMENTS)
		gl.glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, DE_NULL);
	else if (m_drawType == DRAW_ARRAYS)
		gl.glDrawArraysIndirect(GL_TRIANGLES, DE_NULL);
	else
		DE_ASSERT(DE_FALSE);

	gl.glUseProgram(0);
	gl.glFinish();

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
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
				// Only unaligned cases
				if (spec.isCompatibilityTest() == gls::DrawTestSpec::COMPATIBILITY_UNALIGNED_OFFSET ||
					spec.isCompatibilityTest() == gls::DrawTestSpec::COMPATIBILITY_UNALIGNED_STRIDE)
					this->addChild(new gls::DrawTest(m_testCtx, m_context.getRenderContext(), spec, de::toString(insertedCount).c_str(), spec.getDesc().c_str()));
				insertedHashes.insert(hash);

				++insertedCount;
			}
		}
	}
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
	tcu::TestCaseGroup* const unalignedGroup	= new tcu::TestCaseGroup(m_testCtx, "unaligned_data", "Test with unaligned data");
	tcu::TestCaseGroup* const drawArraysGroup	= new tcu::TestCaseGroup(m_testCtx, "drawarrays", "draw arrays");
	tcu::TestCaseGroup* const drawElementsGroup	= new tcu::TestCaseGroup(m_testCtx, "drawelements", "draw elements");

	addChild(unalignedGroup);
	addChild(drawArraysGroup);
	addChild(drawElementsGroup);

	// .unaligned_data
	{
		unalignedGroup->addChild(new RandomGroup(m_context, "random", "random draw commands."));
	}

	// .drawarrays
	{
		drawArraysGroup->addChild(new InvalidDrawCase(m_context, "data_over_bounds_with_count",			"Draw arrays vertex elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ARRAYS,	InvalidDrawCase::INVALID_DATA_COUNT));
		drawArraysGroup->addChild(new InvalidDrawCase(m_context, "data_over_bounds_with_first",			"Draw arrays vertex elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ARRAYS,	InvalidDrawCase::INVALID_DATA_FIRST));
		drawArraysGroup->addChild(new InvalidDrawCase(m_context, "data_over_bounds_with_primcount",		"Draw arrays vertex elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ARRAYS,	InvalidDrawCase::INVALID_DATA_INSTANCED));
		drawArraysGroup->addChild(new InvalidDrawCase(m_context, "reserved_non_zero",					"reservedMustBeZero is set to non-zero value",						InvalidDrawCase::DRAW_ARRAYS,	InvalidDrawCase::INVALID_RESERVED));
	}

	// .drawelements
	{
		drawElementsGroup->addChild(new InvalidDrawCase(m_context, "data_over_bounds_with_count",		"Draw elements vertex elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ELEMENTS, InvalidDrawCase::INVALID_DATA_COUNT));
		drawElementsGroup->addChild(new InvalidDrawCase(m_context, "data_over_bounds_with_basevertex",	"Draw elements vertex elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ELEMENTS,	InvalidDrawCase::INVALID_DATA_FIRST));
		drawElementsGroup->addChild(new InvalidDrawCase(m_context, "data_over_bounds_with_indices",		"Draw elements vertex elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ELEMENTS,	InvalidDrawCase::INVALID_INDEX));
		drawElementsGroup->addChild(new InvalidDrawCase(m_context, "data_over_bounds_with_primcount",	"Draw elements vertex elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ELEMENTS,	InvalidDrawCase::INVALID_DATA_INSTANCED));
		drawElementsGroup->addChild(new InvalidDrawCase(m_context, "index_over_bounds_with_count",		"Draw elements index elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ELEMENTS,	InvalidDrawCase::INVALID_INDEX_COUNT));
		drawElementsGroup->addChild(new InvalidDrawCase(m_context, "index_over_bounds_with_firstindex",	"Draw elements index elements beyond the array end are accessed",	InvalidDrawCase::DRAW_ELEMENTS,	InvalidDrawCase::INVALID_INDEX_FIRST));
		drawElementsGroup->addChild(new InvalidDrawCase(m_context, "reserved_non_zero",					"reservedMustBeZero is set to non-zero value",						InvalidDrawCase::DRAW_ELEMENTS,	InvalidDrawCase::INVALID_RESERVED));
	}
}

} // Stress
} // gles31
} // deqp
