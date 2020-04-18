/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Shader loop tests.
 *//*--------------------------------------------------------------------*/

#include "vktShaderRenderLoopTests.hpp"

#include "vktShaderRender.hpp"
#include "tcuStringTemplate.hpp"
#include "gluShaderUtil.hpp"
#include "deStringUtil.hpp"

#include <map>

namespace vkt
{
namespace sr
{
namespace
{

static const char* getIntUniformName (int number)
{
	switch (number)
	{
		case 0:		return "ui_zero";
		case 1:		return "ui_one";
		case 2:		return "ui_two";
		case 3:		return "ui_three";
		case 4:		return "ui_four";
		case 5:		return "ui_five";
		case 6:		return "ui_six";
		case 7:		return "ui_seven";
		case 8:		return "ui_eight";
		case 101:	return "ui_oneHundredOne";
		default:
			DE_ASSERT(false);
			return "";
	}
}

static BaseUniformType getIntUniformType(int number)
{
	switch (number)
	{
		case 1:		return UI_ONE;
		case 2:		return UI_TWO;
		case 3:		return UI_THREE;
		case 4:		return UI_FOUR;
		case 5:		return UI_FIVE;
		case 6:		return UI_SIX;
		case 7:		return UI_SEVEN;
		case 8:		return UI_EIGHT;
		default:
			DE_ASSERT(false);
			return UB_FALSE;
	}
}

static const char* getFloatFractionUniformName (int number)
{
	switch (number)
	{
		case 1: return "uf_one";
		case 2: return "uf_half";
		case 3: return "uf_third";
		case 4: return "uf_fourth";
		case 5: return "uf_fifth";
		case 6: return "uf_sixth";
		case 7: return "uf_seventh";
		case 8: return "uf_eight";
		default:
			DE_ASSERT(false);
			return "";
	}
}

static BaseUniformType getFloatFractionUniformType(int number)
{
	switch (number)
	{
		case 1:		return UF_ONE;
		case 2:		return UF_HALF;
		case 3:		return UF_THIRD;
		case 4:		return UF_FOURTH;
		case 5:		return UF_FIFTH;
		case 6:		return UF_SIXTH;
		case 7:		return UF_SEVENTH;
		case 8:		return UF_EIGHTH;
		default:
			DE_ASSERT(false);
			return UB_FALSE;
	}
}

enum LoopType
{
	LOOPTYPE_FOR = 0,
	LOOPTYPE_WHILE,
	LOOPTYPE_DO_WHILE,
	LOOPTYPE_LAST
};

static const char* getLoopTypeName (LoopType loopType)
{
	static const char* s_names[] =
	{
		"for",
		"while",
		"do_while"
	};

	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(s_names) == LOOPTYPE_LAST);
	DE_ASSERT(deInBounds32((int)loopType, 0, LOOPTYPE_LAST));
	return s_names[(int)loopType];
}

enum LoopCountType
{
	LOOPCOUNT_CONSTANT = 0,
	LOOPCOUNT_UNIFORM,
	LOOPCOUNT_DYNAMIC,

	LOOPCOUNT_LAST
};

// Repeated with for, while, do-while. Examples given as 'for' loops.
// Repeated for const, uniform, dynamic loops.
enum LoopCase
{
		LOOPCASE_EMPTY_BODY = 0,							// for (...) { }
		LOOPCASE_INFINITE_WITH_UNCONDITIONAL_BREAK_FIRST,	// for (...) { break; <body>; }
		LOOPCASE_INFINITE_WITH_UNCONDITIONAL_BREAK_LAST,	// for (...) { <body>; break; }
		LOOPCASE_INFINITE_WITH_CONDITIONAL_BREAK,			// for (...) { <body>; if (cond) break; }
		LOOPCASE_SINGLE_STATEMENT,							// for (...) statement;
		LOOPCASE_COMPOUND_STATEMENT,						// for (...) { statement; statement; }
		LOOPCASE_SEQUENCE_STATEMENT,						// for (...) statement, statement;
		LOOPCASE_NO_ITERATIONS,								// for (i=0; i<0; i++) ...
		LOOPCASE_SINGLE_ITERATION,							// for (i=0; i<1; i++) ...
		LOOPCASE_SELECT_ITERATION_COUNT,					// for (i=0; i<a?b:c; i++) ...
		LOOPCASE_CONDITIONAL_CONTINUE,						// for (...) { if (cond) continue; }
		LOOPCASE_UNCONDITIONAL_CONTINUE,					// for (...) { <body>; continue; }
		LOOPCASE_ONLY_CONTINUE,								// for (...) { continue; }
		LOOPCASE_DOUBLE_CONTINUE,							// for (...) { if (cond) continue; <body>; $
		LOOPCASE_CONDITIONAL_BREAK,							// for (...) { if (cond) break; }
		LOOPCASE_UNCONDITIONAL_BREAK,						// for (...) { <body>; break; }
		LOOPCASE_PRE_INCREMENT,								// for (...; ++i) { <body>; }
		LOOPCASE_POST_INCREMENT,							// for (...; i++) { <body>; }
		LOOPCASE_MIXED_BREAK_CONTINUE,
		LOOPCASE_VECTOR_COUNTER,							// for (ivec3 ndx = ...; ndx.x < ndx.y; ndx$
		LOOPCASE_101_ITERATIONS,							// loop for 101 iterations
		LOOPCASE_SEQUENCE,									// two loops in sequence
		LOOPCASE_NESTED,									// two nested loops
		LOOPCASE_NESTED_SEQUENCE,							// two loops in sequence nested inside a th$
		LOOPCASE_NESTED_TRICKY_DATAFLOW_1,					// nested loops with tricky data flow
		LOOPCASE_NESTED_TRICKY_DATAFLOW_2,					// nested loops with tricky data flow

		//LOOPCASE_MULTI_DECLARATION,						// for (int i,j,k; ...) ...  -- illegal?

		LOOPCASE_LAST
};

static const char* getLoopCaseName (LoopCase loopCase)
{
		static const char* s_names[] =
		{
				"empty_body",
				"infinite_with_unconditional_break_first",
				"infinite_with_unconditional_break_last",
				"infinite_with_conditional_break",
				"single_statement",
				"compound_statement",
				"sequence_statement",
				"no_iterations",
				"single_iteration",
				"select_iteration_count",
				"conditional_continue",
				"unconditional_continue",
				"only_continue",
				"double_continue",
				"conditional_break",
				"unconditional_break",
				"pre_increment",
				"post_increment",
				"mixed_break_continue",
				"vector_counter",
				"101_iterations",
				"sequence",
				"nested",
				"nested_sequence",
				"nested_tricky_dataflow_1",
				"nested_tricky_dataflow_2"
				//"multi_declaration",
		};

		DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(s_names) == LOOPCASE_LAST);
		DE_ASSERT(deInBounds32((int)loopCase, 0, LOOPCASE_LAST));
		return s_names[(int)loopCase];
}

static const char* getLoopCountTypeName (LoopCountType countType)
{
	static const char* s_names[] =
	{
		"constant",
		"uniform",
		"dynamic"
	};

	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(s_names) == LOOPCOUNT_LAST);
	DE_ASSERT(deInBounds32((int)countType, 0, LOOPCOUNT_LAST));
	return s_names[(int)countType];
}

static void evalLoop0Iters	(ShaderEvalContext& c) { c.color.xyz()	= c.coords.swizzle(0,1,2); }
static void evalLoop1Iters	(ShaderEvalContext& c) { c.color.xyz()	= c.coords.swizzle(1,2,3); }
static void evalLoop2Iters	(ShaderEvalContext& c) { c.color.xyz()	= c.coords.swizzle(2,3,0); }
static void evalLoop3Iters	(ShaderEvalContext& c) { c.color.xyz()	= c.coords.swizzle(3,0,1); }

static ShaderEvalFunc getLoopEvalFunc (int numIters)
{
	switch (numIters % 4)
	{
		case 0: return evalLoop0Iters;
		case 1:	return evalLoop1Iters;
		case 2:	return evalLoop2Iters;
		case 3:	return evalLoop3Iters;
	}

	DE_FATAL("Invalid loop iteration count.");
	return NULL;
}

// ShaderLoop case

class ShaderLoopCase : public ShaderRenderCase
{
public:
	ShaderLoopCase	(tcu::TestContext&	testCtx,
					 const std::string&	name,
					 const std::string&	description,
					 bool				isVertexCase,
					 ShaderEvalFunc		evalFunc,
					 UniformSetup*		uniformSetup,
					 const std::string&	vertexShaderSource,
					 const std::string&	fragmentShaderSource)
		: ShaderRenderCase		(testCtx, name, description, isVertexCase, evalFunc, uniformSetup, DE_NULL)
	{
		m_vertShaderSource = vertexShaderSource;
		m_fragShaderSource = fragmentShaderSource;
	}
};

// Uniform setup tools

class LoopUniformSetup : public UniformSetup
{
public:
									LoopUniformSetup	(std::vector<BaseUniformType>& types)
										: m_uniformInformations(types)
									{}

	virtual void					setup				(ShaderRenderCaseInstance& instance, const tcu::Vec4& constCoords) const;

private:
	std::vector<BaseUniformType>	m_uniformInformations;
};

void LoopUniformSetup::setup (ShaderRenderCaseInstance& instance, const tcu::Vec4&) const
{
	for (size_t i = 0; i < m_uniformInformations.size(); i++)
	{
		instance.useUniform((deUint32)i, m_uniformInformations[i]);
	}
}

// Testcase builders

static de::MovePtr<ShaderLoopCase> createGenericLoopCase (tcu::TestContext&	testCtx,
														const std::string&	caseName,
														const std::string&	description,
														bool				isVertexCase,
														LoopType			loopType,
														LoopCountType		loopCountType,
														glu::Precision		loopCountPrecision,
														glu::DataType		loopCountDataType)
{
	std::ostringstream vtx;
	std::ostringstream frag;
	std::ostringstream& op = isVertexCase ? vtx : frag;

	vtx << "#version 310 es\n";
	frag << "#version 310 es\n";

	vtx << "layout(location=0) in highp vec4 a_position;\n";
	vtx << "layout(location=1) in highp vec4 a_coords;\n";
	frag << "layout(location=0) out mediump vec4 o_color;\n";

	if (loopCountType == LOOPCOUNT_DYNAMIC)
		vtx << "layout(location=3) in mediump float a_one;\n";

	if (isVertexCase)
	{
		vtx << "layout(location=0) out mediump vec3 v_color;\n";
		frag << "layout(location=0) in mediump vec3 v_color;\n";
	}
	else
	{
		vtx << "layout(location=0) out mediump vec4 v_coords;\n";
		frag << "layout(location=0) in mediump vec4 v_coords;\n";

		if (loopCountType == LOOPCOUNT_DYNAMIC)
		{
			vtx << "layout(location=1) out mediump float v_one;\n";
			frag << "layout(location=1) in mediump float v_one;\n";
		}
	}

	const int	numLoopIters = 3;
	const bool	isIntCounter = isDataTypeIntOrIVec(loopCountDataType);
	deUint32	locationCounter = 0;
	std::vector<BaseUniformType> uniformInformations;

	if (isIntCounter)
	{
		if (loopCountType == LOOPCOUNT_UNIFORM || loopCountType == LOOPCOUNT_DYNAMIC)
		{
			op << "layout(std140, set=0, binding=" << locationCounter << ") uniform buff"<< locationCounter <<" {\n";
			op << " ${COUNTER_PRECISION} int " << getIntUniformName(numLoopIters) << ";\n";
			op << "};\n";
			uniformInformations.push_back(getIntUniformType(numLoopIters));
			locationCounter++;
		}
	}
	else
	{
		if (loopCountType == LOOPCOUNT_UNIFORM || loopCountType == LOOPCOUNT_DYNAMIC){
			op << "layout(std140, set=0, binding=" << locationCounter << ") uniform buff" << locationCounter << " {\n";
			op << "	${COUNTER_PRECISION} float " << getFloatFractionUniformName(numLoopIters) << ";\n";
			op << "};\n";
			uniformInformations.push_back(getFloatFractionUniformType(numLoopIters));
			locationCounter++;
		}

		if (numLoopIters != 1){
			op << "layout(std140, set=0, binding=" << locationCounter << ") uniform buff" << locationCounter << " {\n";
			op << "	${COUNTER_PRECISION} float uf_one;\n";
			op << "};\n";
			uniformInformations.push_back(UF_ONE);
			locationCounter++;
		}
	}

	vtx << "\n";
	vtx << "void main()\n";
	vtx << "{\n";
	vtx << "	gl_Position = a_position;\n";

	frag << "\n";
	frag << "void main()\n";
	frag << "{\n";

	if (isVertexCase)
		vtx << "	${PRECISION} vec4 coords = a_coords;\n";
	else
		frag << "	${PRECISION} vec4 coords = v_coords;\n";


	if (loopCountType == LOOPCOUNT_DYNAMIC)
	{
		if (isIntCounter)
		{
			if (isVertexCase)
				vtx << "	${COUNTER_PRECISION} int one = int(a_one + 0.5);\n";
			else
				frag << "	${COUNTER_PRECISION} int one = int(v_one + 0.5);\n";
		}
		else
		{
			if (isVertexCase)
				vtx << "	${COUNTER_PRECISION} float one = a_one;\n";
			else
				frag << "	${COUNTER_PRECISION} float one = v_one;\n";
		}
	}

	// Read array.
	op << "	${PRECISION} vec4 res = coords;\n";

	// Loop iteration count.
	std::string	iterMaxStr;

	if (isIntCounter)
	{
		if (loopCountType == LOOPCOUNT_CONSTANT)
			iterMaxStr = de::toString(numLoopIters);
		else if (loopCountType == LOOPCOUNT_UNIFORM)
			iterMaxStr = getIntUniformName(numLoopIters);
		else if (loopCountType == LOOPCOUNT_DYNAMIC)
			iterMaxStr = std::string(getIntUniformName(numLoopIters)) + "*one";
		else
			DE_ASSERT(false);
	}
	else
	{
		if (loopCountType == LOOPCOUNT_CONSTANT)
			iterMaxStr = "1.0";
		else if (loopCountType == LOOPCOUNT_UNIFORM)
			iterMaxStr = "uf_one";
		else if (loopCountType == LOOPCOUNT_DYNAMIC)
			iterMaxStr = "uf_one*one";
		else
			DE_ASSERT(false);
	}

	// Loop operations.
	std::string initValue			= isIntCounter ? "0" : "0.05";
	std::string loopCountDeclStr	= "${COUNTER_PRECISION} ${LOOP_VAR_TYPE} ndx = " + initValue;
	std::string loopCmpStr			= ("ndx < " + iterMaxStr);
	std::string incrementStr;
	if (isIntCounter)
		incrementStr = "ndx++";
	else
	{
		if (loopCountType == LOOPCOUNT_CONSTANT)
			incrementStr = std::string("ndx += ") + de::toString(1.0f / (float)numLoopIters);
		else if (loopCountType == LOOPCOUNT_UNIFORM)
			incrementStr = std::string("ndx += ") + getFloatFractionUniformName(numLoopIters);
		else if (loopCountType == LOOPCOUNT_DYNAMIC)
			incrementStr = std::string("ndx += ") + getFloatFractionUniformName(numLoopIters) + "*one";
		else
			DE_ASSERT(false);
	}

	// Loop body.
	std::string loopBody;

	loopBody = "		res = res.yzwx;\n";

	if (loopType == LOOPTYPE_FOR)
	{
		op << "	for (" + loopCountDeclStr + "; " + loopCmpStr + "; " + incrementStr + ")\n";
		op << "	{\n";
		op << loopBody;
		op << "	}\n";
	}
	else if (loopType == LOOPTYPE_WHILE)
	{
		op << "\t" << loopCountDeclStr + ";\n";
		op << "	while (" + loopCmpStr + ")\n";
		op << "	{\n";
		op << loopBody;
		op << "\t\t" + incrementStr + ";\n";
		op << "	}\n";
	}
	else if (loopType == LOOPTYPE_DO_WHILE)
	{
		op << "\t" << loopCountDeclStr + ";\n";
		op << "	do\n";
		op << "	{\n";
		op << loopBody;
		op << "\t\t" + incrementStr + ";\n";
		op << "	} while (" + loopCmpStr + ");\n";
	}
	else
		DE_ASSERT(false);

	if (isVertexCase)
	{
		vtx << "	v_color = res.rgb;\n";
		frag << "	o_color = vec4(v_color.rgb, 1.0);\n";
	}
	else
	{
		vtx << "	v_coords = a_coords;\n";
		frag << "	o_color = vec4(res.rgb, 1.0);\n";

		if (loopCountType == LOOPCOUNT_DYNAMIC)
			vtx << "	v_one = a_one;\n";
	}

	vtx << "}\n";
	frag << "}\n";

	// Fill in shader templates.
	std::map<std::string, std::string> params;
	params.insert(std::pair<std::string, std::string>("LOOP_VAR_TYPE", getDataTypeName(loopCountDataType)));
	params.insert(std::pair<std::string, std::string>("PRECISION", "mediump"));
	params.insert(std::pair<std::string, std::string>("COUNTER_PRECISION", getPrecisionName(loopCountPrecision)));

	tcu::StringTemplate vertTemplate(vtx.str());
	tcu::StringTemplate fragTemplate(frag.str());
	std::string vertexShaderSource = vertTemplate.specialize(params);
	std::string fragmentShaderSource = fragTemplate.specialize(params);

	// Create the case.
	ShaderEvalFunc evalFunc = getLoopEvalFunc(numLoopIters);
	UniformSetup* uniformSetup = new LoopUniformSetup(uniformInformations);
	return de::MovePtr<ShaderLoopCase>(new ShaderLoopCase(testCtx, caseName, description, isVertexCase, evalFunc, uniformSetup, vertexShaderSource, fragmentShaderSource));
}

static de::MovePtr<ShaderLoopCase> createSpecialLoopCase (tcu::TestContext&	testCtx,
														const std::string&	caseName,
														const std::string&	description,
														bool				isVertexCase,
														LoopCase			loopCase,
														LoopType			loopType,
														LoopCountType		loopCountType)
{
	std::ostringstream vtx;
	std::ostringstream frag;
	std::ostringstream& op = isVertexCase ? vtx : frag;

	std::vector<BaseUniformType>	uniformInformations;
	deUint32						locationCounter = 0;

	vtx << "#version 310 es\n";
	frag << "#version 310 es\n";

	vtx << "layout(location=0) in highp vec4 a_position;\n";
	vtx << "layout(location=1) in highp vec4 a_coords;\n";
	frag << "layout(location=0) out mediump vec4 o_color;\n";

	if (loopCountType == LOOPCOUNT_DYNAMIC)
		vtx << "layout(location=3) in mediump float a_one;\n";

	if (isVertexCase)
	{
		vtx << "layout(location=0) out mediump vec3 v_color;\n";
		frag << "layout(location=0) in mediump vec3 v_color;\n";
	}
	else
	{
		vtx << "layout(location=0) out mediump vec4 v_coords;\n";
		frag << "layout(location=0) in mediump vec4 v_coords;\n";

		if (loopCountType == LOOPCOUNT_DYNAMIC)
		{
			vtx << "layout(location=1) out mediump float v_one;\n";
			frag << "layout(location=1) in mediump float v_one;\n";
		}
	}

	if (loopCase == LOOPCASE_SELECT_ITERATION_COUNT) {
		op << "layout(std140, set=0, binding=" << locationCounter << ") uniform buff" << locationCounter << " {\n";
		op << "  bool ub_true;\n";
		op << "};\n";
		uniformInformations.push_back(UB_TRUE);
		locationCounter++;
	}

	struct
	{
		char const*		name;
		BaseUniformType	type;
	} uniforms[] =
	{
		{ "ui_zero",	UI_ZERO },
		{ "ui_one",		UI_ONE },
		{ "ui_two",		UI_TWO },
		{ "ui_three",	UI_THREE },
		{ "ui_four",	UI_FOUR },
		{ "ui_five",	UI_FIVE },
		{ "ui_six",		UI_SIX  },
	};

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(uniforms); ++i)
	{
		op << "layout(std140, set=0, binding=" << locationCounter << ") uniform buff" << locationCounter << " {\n";
		op << "  ${COUNTER_PRECISION} int " << uniforms[i].name << ";\n";
		op << "};\n";
		uniformInformations.push_back(uniforms[i].type);
		locationCounter++;
	}

	if (loopCase == LOOPCASE_101_ITERATIONS) {

		op << "layout(std140, set=0, binding=" << locationCounter <<  ") uniform buff" << locationCounter << " {\n";
		op << "  ${COUNTER_PRECISION} int ui_oneHundredOne;\n";
		op << "};\n";
		uniformInformations.push_back(UI_ONEHUNDREDONE);
		locationCounter++;
	}

	int iterCount	= 3;	// value to use in loop
	int numIters	= 3;	// actual number of iterations

	vtx << "\n";
	vtx << "void main()\n";
	vtx << "{\n";
	vtx << "	gl_Position = a_position;\n";

	frag << "\n";
	frag << "void main()\n";
	frag << "{\n";

	if (loopCountType == LOOPCOUNT_DYNAMIC)
	{
		if (isVertexCase)
			vtx << "	${COUNTER_PRECISION} int one = int(a_one + 0.5);\n";
		else
			frag << "	${COUNTER_PRECISION} int one = int(v_one + 0.5);\n";
	}

	if (isVertexCase)
		vtx << "	${PRECISION} vec4 coords = a_coords;\n";
	else
		frag << "	${PRECISION} vec4 coords = v_coords;\n";

	// Read array.
	op << "	${PRECISION} vec4 res = coords;\n";

	// Handle all loop types.
	std::string counterPrecisionStr = "mediump";
	std::string forLoopStr;
	std::string whileLoopStr;
	std::string doWhileLoopPreStr;
	std::string doWhileLoopPostStr;

	if (loopType == LOOPTYPE_FOR)
	{
		switch (loopCase)
		{
			case LOOPCASE_EMPTY_BODY:
				numIters = 0;
				op << "	${FOR_LOOP} {}\n";
				break;

			case LOOPCASE_INFINITE_WITH_UNCONDITIONAL_BREAK_FIRST:
				numIters = 0;
				op << "	for (;;) { break; res = res.yzwx; }\n";
				break;

			case LOOPCASE_INFINITE_WITH_UNCONDITIONAL_BREAK_LAST:
				numIters = 1;
				op << "	for (;;) { res = res.yzwx; break; }\n";
				break;

			case LOOPCASE_INFINITE_WITH_CONDITIONAL_BREAK:
				numIters = 2;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	for (;;) { res = res.yzwx; if (i == ${ONE}) break; i++; }\n";
				break;

			case LOOPCASE_SINGLE_STATEMENT:
				op << "	${FOR_LOOP} res = res.yzwx;\n";
				break;

			case LOOPCASE_COMPOUND_STATEMENT:
				iterCount	= 2;
				numIters	= 2 * iterCount;
				op << "	${FOR_LOOP} { res = res.yzwx; res = res.yzwx; }\n";
				break;

			case LOOPCASE_SEQUENCE_STATEMENT:
				iterCount	= 2;
				numIters	= 2 * iterCount;
				op << "	${FOR_LOOP} res = res.yzwx, res = res.yzwx;\n";
				break;

			case LOOPCASE_NO_ITERATIONS:
				iterCount	= 0;
				numIters	= 0;
				op << "	${FOR_LOOP} res = res.yzwx;\n";
				break;

			case LOOPCASE_SINGLE_ITERATION:
				iterCount	= 1;
				numIters	= 1;
				op << "	${FOR_LOOP} res = res.yzwx;\n";
				break;

			case LOOPCASE_SELECT_ITERATION_COUNT:
				op << "	for (int i = 0; i < (ub_true ? ${ITER_COUNT} : 0); i++) res = res.yzwx;\n";
				break;

			case LOOPCASE_CONDITIONAL_CONTINUE:
				numIters = iterCount - 1;
				op << "	${FOR_LOOP} { if (i == ${TWO}) continue; res = res.yzwx; }\n";
				break;

			case LOOPCASE_UNCONDITIONAL_CONTINUE:
				op << "	${FOR_LOOP} { res = res.yzwx; continue; }\n";
				break;

			case LOOPCASE_ONLY_CONTINUE:
				numIters = 0;
				op << "	${FOR_LOOP} { continue; }\n";
				break;

			case LOOPCASE_DOUBLE_CONTINUE:
				numIters = iterCount - 1;
				op << "	${FOR_LOOP} { if (i == ${TWO}) continue; res = res.yzwx; continue; }\n";
				break;

			case LOOPCASE_CONDITIONAL_BREAK:
				numIters = 2;
				op << "	${FOR_LOOP} { if (i == ${TWO}) break; res = res.yzwx; }\n";
				break;

			case LOOPCASE_UNCONDITIONAL_BREAK:
				numIters = 1;
				op << "	${FOR_LOOP} { res = res.yzwx; break; }\n";
				break;

			case LOOPCASE_PRE_INCREMENT:
				op << "	for (int i = 0; i < ${ITER_COUNT}; ++i) { res = res.yzwx; }\n";
				break;

			case LOOPCASE_POST_INCREMENT:
				op << "	${FOR_LOOP} { res = res.yzwx; }\n";
				break;

			case LOOPCASE_MIXED_BREAK_CONTINUE:
				numIters	= 2;
				iterCount	= 5;
				op << "	${FOR_LOOP} { if (i == 0) continue; else if (i == 3) break; res = res.yzwx; }\n";
				break;

			case LOOPCASE_VECTOR_COUNTER:
				op << "	for (${COUNTER_PRECISION} ivec4 i = ivec4(0, 1, ${ITER_COUNT}, 0); i.x < i.z; i.x += i.y) { res = res.yzwx; }\n";
				break;

			case LOOPCASE_101_ITERATIONS:
				numIters = iterCount = 101;
				op << "	${FOR_LOOP} res = res.yzwx;\n";
				break;

			case LOOPCASE_SEQUENCE:
				iterCount	= 5;
				numIters	= 5;
				op << "	${COUNTER_PRECISION} int i;\n";
				op << "	for (i = 0; i < ${TWO}; i++) { res = res.yzwx; }\n";
				op << "	for (; i < ${ITER_COUNT}; i++) { res = res.yzwx; }\n";
				break;

			case LOOPCASE_NESTED:
				numIters = 2 * iterCount;
				op << "	for (${COUNTER_PRECISION} int i = 0; i < ${TWO}; i++)\n";
				op << "	{\n";
				op << "		for (${COUNTER_PRECISION} int j = 0; j < ${ITER_COUNT}; j++)\n";
				op << "			res = res.yzwx;\n";
				op << "	}\n";
				break;

			case LOOPCASE_NESTED_SEQUENCE:
				numIters = 3 * iterCount;
				op << "	for (${COUNTER_PRECISION} int i = 0; i < ${ITER_COUNT}; i++)\n";
				op << "	{\n";
				op << "		for (${COUNTER_PRECISION} int j = 0; j < ${TWO}; j++)\n";
				op << "			res = res.yzwx;\n";
				op << "		for (${COUNTER_PRECISION} int j = 0; j < ${ONE}; j++)\n";
				op << "			res = res.yzwx;\n";
				op << "	}\n";
				break;

			case LOOPCASE_NESTED_TRICKY_DATAFLOW_1:
				numIters = 2;
				op << "	${FOR_LOOP}\n";
				op << "	{\n";
				op << "		res = coords; // ignore outer loop effect \n";
				op << "		for (${COUNTER_PRECISION} int j = 0; j < ${TWO}; j++)\n";
				op << "			res = res.yzwx;\n";
				op << "	}\n";
				break;

			case LOOPCASE_NESTED_TRICKY_DATAFLOW_2:
				numIters = iterCount;
				op << "	${FOR_LOOP}\n";
				op << "	{\n";
				op << "		res = coords.wxyz;\n";
				op << "		for (${COUNTER_PRECISION} int j = 0; j < ${TWO}; j++)\n";
				op << "			res = res.yzwx;\n";
				op << "		coords = res;\n";
				op << "	}\n";
				break;

			default:
				DE_ASSERT(false);
		}

		if (loopCountType == LOOPCOUNT_CONSTANT)
			forLoopStr = std::string("for (") + counterPrecisionStr + " int i = 0; i < " + de::toString(iterCount) + "; i++)";
		else if (loopCountType == LOOPCOUNT_UNIFORM)
			forLoopStr = std::string("for (") + counterPrecisionStr + " int i = 0; i < " + getIntUniformName(iterCount) + "; i++)";
		else if (loopCountType == LOOPCOUNT_DYNAMIC)
			forLoopStr = std::string("for (") + counterPrecisionStr + " int i = 0; i < one*" + getIntUniformName(iterCount) + "; i++)";
		else
			DE_ASSERT(false);
	}
	else if (loopType == LOOPTYPE_WHILE)
	{
		switch (loopCase)
		{
			case LOOPCASE_EMPTY_BODY:
				numIters = 0;
				op << "	${WHILE_LOOP} {}\n";
				break;

			case LOOPCASE_INFINITE_WITH_UNCONDITIONAL_BREAK_FIRST:
				numIters = 0;
				op << "	while (true) { break; res = res.yzwx; }\n";
				break;

			case LOOPCASE_INFINITE_WITH_UNCONDITIONAL_BREAK_LAST:
				numIters = 1;
				op << "	while (true) { res = res.yzwx; break; }\n";
				break;

			case LOOPCASE_INFINITE_WITH_CONDITIONAL_BREAK:
				numIters = 2;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	while (true) { res = res.yzwx; if (i == ${ONE}) break; i++; }\n";
				break;

			case LOOPCASE_SINGLE_STATEMENT:
				op << "	${WHILE_LOOP} res = res.yzwx;\n";
				break;

			case LOOPCASE_COMPOUND_STATEMENT:
				iterCount	= 2;
				numIters	= 2 * iterCount;
				op << "	${WHILE_LOOP} { res = res.yzwx; res = res.yzwx; }\n";
				break;

			case LOOPCASE_SEQUENCE_STATEMENT:
				iterCount	= 2;
				numIters	= 2 * iterCount;
				op << "	${WHILE_LOOP} res = res.yzwx, res = res.yzwx;\n";
				break;

			case LOOPCASE_NO_ITERATIONS:
				iterCount	= 0;
				numIters	= 0;
				op << "	${WHILE_LOOP} res = res.yzwx;\n";
				break;

			case LOOPCASE_SINGLE_ITERATION:
				iterCount	= 1;
				numIters	= 1;
				op << "	${WHILE_LOOP} res = res.yzwx;\n";
				break;

			case LOOPCASE_SELECT_ITERATION_COUNT:
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	while (i < (ub_true ? ${ITER_COUNT} : 0)) { res = res.yzwx; i++; }\n";
				break;

			case LOOPCASE_CONDITIONAL_CONTINUE:
				numIters = iterCount - 1;
				op << "	${WHILE_LOOP} { if (i == ${TWO}) continue; res = res.yzwx; }\n";
				break;

			case LOOPCASE_UNCONDITIONAL_CONTINUE:
				op << "	${WHILE_LOOP} { res = res.yzwx; continue; }\n";
				break;

			case LOOPCASE_ONLY_CONTINUE:
				numIters = 0;
				op << "	${WHILE_LOOP} { continue; }\n";
				break;

			case LOOPCASE_DOUBLE_CONTINUE:
				numIters = iterCount - 1;
				op << "	${WHILE_LOOP} { if (i == ${ONE}) continue; res = res.yzwx; continue; }\n";
				break;

			case LOOPCASE_CONDITIONAL_BREAK:
				numIters = 2;
				op << "	${WHILE_LOOP} { if (i == ${THREE}) break; res = res.yzwx; }\n";
				break;

			case LOOPCASE_UNCONDITIONAL_BREAK:
				numIters = 1;
				op << "	${WHILE_LOOP} { res = res.yzwx; break; }\n";
				break;

			case LOOPCASE_PRE_INCREMENT:
				numIters = iterCount - 1;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	while (++i < ${ITER_COUNT}) { res = res.yzwx; }\n";
				break;

			case LOOPCASE_POST_INCREMENT:
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	while (i++ < ${ITER_COUNT}) { res = res.yzwx; }\n";
				break;

			case LOOPCASE_MIXED_BREAK_CONTINUE:
				numIters	= 2;
				iterCount	= 5;
				op << "	${WHILE_LOOP} { if (i == 0) continue; else if (i == 3) break; res = res.yzwx; }\n";
				break;

			case LOOPCASE_VECTOR_COUNTER:
				op << "	${COUNTER_PRECISION} ivec4 i = ivec4(0, 1, ${ITER_COUNT}, 0);\n";
				op << "	while (i.x < i.z) { res = res.yzwx; i.x += i.y; }\n";
				break;

			case LOOPCASE_101_ITERATIONS:
				numIters = iterCount = 101;
				op << "	${WHILE_LOOP} res = res.yzwx;\n";
				break;

			case LOOPCASE_SEQUENCE:
				iterCount	= 6;
				numIters	= iterCount - 1;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	while (i++ < ${TWO}) { res = res.yzwx; }\n";
				op << "	while (i++ < ${ITER_COUNT}) { res = res.yzwx; }\n"; // \note skips one iteration
				break;

			case LOOPCASE_NESTED:
				numIters = 2 * iterCount;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	while (i++ < ${TWO})\n";
				op << "	{\n";
				op << "		${COUNTER_PRECISION} int j = 0;\n";
				op << "		while (j++ < ${ITER_COUNT})\n";
				op << "			res = res.yzwx;\n";
				op << "	}\n";
				break;

			case LOOPCASE_NESTED_SEQUENCE:
				numIters = 2 * iterCount;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	while (i++ < ${ITER_COUNT})\n";
				op << "	{\n";
				op << "		${COUNTER_PRECISION} int j = 0;\n";
				op << "		while (j++ < ${ONE})\n";
				op << "			res = res.yzwx;\n";
				op << "		while (j++ < ${THREE})\n"; // \note skips one iteration
				op << "			res = res.yzwx;\n";
				op << "	}\n";
				break;

			case LOOPCASE_NESTED_TRICKY_DATAFLOW_1:
				numIters = 2;
				op << "	${WHILE_LOOP}\n";
				op << "	{\n";
				op << "		res = coords; // ignore outer loop effect \n";
				op << "		${COUNTER_PRECISION} int j = 0;\n";
				op << "		while (j++ < ${TWO})\n";
				op << "			res = res.yzwx;\n";
				op << "	}\n";
				break;

			case LOOPCASE_NESTED_TRICKY_DATAFLOW_2:
				numIters = iterCount;
				op << "	${WHILE_LOOP}\n";
				op << "	{\n";
				op << "		res = coords.wxyz;\n";
				op << "		${COUNTER_PRECISION} int j = 0;\n";
				op << "		while (j++ < ${TWO})\n";
				op << "			res = res.yzwx;\n";
				op << "		coords = res;\n";
				op << "	}\n";
				break;

			default:
				DE_ASSERT(false);
		}

		if (loopCountType == LOOPCOUNT_CONSTANT)
			whileLoopStr = std::string("\t") + counterPrecisionStr + " int i = 0;\n" + "	while(i++ < " + de::toString(iterCount) + ")";
		else if (loopCountType == LOOPCOUNT_UNIFORM)
			whileLoopStr = std::string("\t") + counterPrecisionStr + " int i = 0;\n" + "	while(i++ < " + getIntUniformName(iterCount) + ")";
		else if (loopCountType == LOOPCOUNT_DYNAMIC)
			whileLoopStr = std::string("\t") + counterPrecisionStr + " int i = 0;\n" + "	while(i++ < one*" + getIntUniformName(iterCount) + ")";
		else
			DE_ASSERT(false);
	}
	else
	{
		DE_ASSERT(loopType == LOOPTYPE_DO_WHILE);

		switch (loopCase)
		{
			case LOOPCASE_EMPTY_BODY:
				numIters = 0;
				op << "	${DO_WHILE_PRE} {} ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_INFINITE_WITH_UNCONDITIONAL_BREAK_FIRST:
				numIters = 0;
				op << "	do { break; res = res.yzwx; } while (true);\n";
				break;

			case LOOPCASE_INFINITE_WITH_UNCONDITIONAL_BREAK_LAST:
				numIters = 1;
				op << "	do { res = res.yzwx; break; } while (true);\n";
				break;

			case LOOPCASE_INFINITE_WITH_CONDITIONAL_BREAK:
				numIters = 2;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	do { res = res.yzwx; if (i == ${ONE}) break; i++; } while (true);\n";
				break;

			case LOOPCASE_SINGLE_STATEMENT:
				op << "	${DO_WHILE_PRE} res = res.yzwx; ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_COMPOUND_STATEMENT:
				iterCount	= 2;
				numIters	= 2 * iterCount;
				op << "	${DO_WHILE_PRE} { res = res.yzwx; res = res.yzwx; } ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_SEQUENCE_STATEMENT:
				iterCount	= 2;
				numIters	= 2 * iterCount;
				op << "	${DO_WHILE_PRE} res = res.yzwx, res = res.yzwx; ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_NO_ITERATIONS:
				DE_ASSERT(false);
				break;

			case LOOPCASE_SINGLE_ITERATION:
				iterCount	= 1;
				numIters	= 1;
				op << "	${DO_WHILE_PRE} res = res.yzwx; ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_SELECT_ITERATION_COUNT:
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	do { res = res.yzwx; } while (++i < (ub_true ? ${ITER_COUNT} : 0));\n";
				break;

			case LOOPCASE_CONDITIONAL_CONTINUE:
				numIters = iterCount - 1;
				op << "	${DO_WHILE_PRE} { if (i == ${TWO}) continue; res = res.yzwx; } ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_UNCONDITIONAL_CONTINUE:
				op << "	${DO_WHILE_PRE} { res = res.yzwx; continue; } ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_ONLY_CONTINUE:
				numIters = 0;
				op << "	${DO_WHILE_PRE} { continue; } ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_DOUBLE_CONTINUE:
				numIters = iterCount - 1;
				op << "	${DO_WHILE_PRE} { if (i == ${TWO}) continue; res = res.yzwx; continue; } ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_CONDITIONAL_BREAK:
				numIters = 2;
				op << "	${DO_WHILE_PRE} { res = res.yzwx; if (i == ${ONE}) break; } ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_UNCONDITIONAL_BREAK:
				numIters = 1;
				op << "	${DO_WHILE_PRE} { res = res.yzwx; break; } ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_PRE_INCREMENT:
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	do { res = res.yzwx; } while (++i < ${ITER_COUNT});\n";
				break;

			case LOOPCASE_POST_INCREMENT:
				numIters = iterCount + 1;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	do { res = res.yzwx; } while (i++ < ${ITER_COUNT});\n";
				break;

			case LOOPCASE_MIXED_BREAK_CONTINUE:
				numIters	= 2;
				iterCount	= 5;
				op << "	${DO_WHILE_PRE} { if (i == 0) continue; else if (i == 3) break; res = res.yzwx; } ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_VECTOR_COUNTER:
				op << "	${COUNTER_PRECISION} ivec4 i = ivec4(0, 1, ${ITER_COUNT}, 0);\n";
				op << "	do { res = res.yzwx; } while ((i.x += i.y) < i.z);\n";
				break;

			case LOOPCASE_101_ITERATIONS:
				numIters = iterCount = 101;
				op << "	${DO_WHILE_PRE} res = res.yzwx; ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_SEQUENCE:
				iterCount	= 5;
				numIters	= 5;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	do { res = res.yzwx; } while (++i < ${TWO});\n";
				op << "	do { res = res.yzwx; } while (++i < ${ITER_COUNT});\n";
				break;

			case LOOPCASE_NESTED:
				numIters = 2 * iterCount;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	do\n";
				op << "	{\n";
				op << "		${COUNTER_PRECISION} int j = 0;\n";
				op << "		do\n";
				op << "			res = res.yzwx;\n";
				op << "		while (++j < ${ITER_COUNT});\n";
				op << "	} while (++i < ${TWO});\n";
				break;

			case LOOPCASE_NESTED_SEQUENCE:
				numIters = 3 * iterCount;
				op << "	${COUNTER_PRECISION} int i = 0;\n";
				op << "	do\n";
				op << "	{\n";
				op << "		${COUNTER_PRECISION} int j = 0;\n";
				op << "		do\n";
				op << "			res = res.yzwx;\n";
				op << "		while (++j < ${TWO});\n";
				op << "		do\n";
				op << "			res = res.yzwx;\n";
				op << "		while (++j < ${THREE});\n";
				op << "	} while (++i < ${ITER_COUNT});\n";
				break;

			case LOOPCASE_NESTED_TRICKY_DATAFLOW_1:
				numIters = 2;
				op << "	${DO_WHILE_PRE}\n";
				op << "	{\n";
				op << "		res = coords; // ignore outer loop effect \n";
				op << "		${COUNTER_PRECISION} int j = 0;\n";
				op << "		do\n";
				op << "			res = res.yzwx;\n";
				op << "		while (++j < ${TWO});\n";
				op << "	} ${DO_WHILE_POST}\n";
				break;

			case LOOPCASE_NESTED_TRICKY_DATAFLOW_2:
				numIters = iterCount;
				op << "	${DO_WHILE_PRE}\n";
				op << "	{\n";
				op << "		res = coords.wxyz;\n";
				op << "		${COUNTER_PRECISION} int j = 0;\n";
				op << "		while (j++ < ${TWO})\n";
				op << "			res = res.yzwx;\n";
				op << "		coords = res;\n";
				op << "	} ${DO_WHILE_POST}\n";
				break;

			default:
				DE_ASSERT(false);
		}

		doWhileLoopPreStr = std::string("\t") + counterPrecisionStr + " int i = 0;\n" + "\tdo ";
		if (loopCountType == LOOPCOUNT_CONSTANT)
			doWhileLoopPostStr = std::string(" while (++i < ") + de::toString(iterCount) + ");\n";
		else if (loopCountType == LOOPCOUNT_UNIFORM)
			doWhileLoopPostStr = std::string(" while (++i < ") + getIntUniformName(iterCount) + ");\n";
		else if (loopCountType == LOOPCOUNT_DYNAMIC)
			doWhileLoopPostStr = std::string(" while (++i < one*") + getIntUniformName(iterCount) + ");\n";
		else
			DE_ASSERT(false);
	}

	// Shader footers.
	if (isVertexCase)
	{
		vtx << "	v_color = res.rgb;\n";
		frag << "	o_color = vec4(v_color.rgb, 1.0);\n";
	}
	else
	{
		vtx << "	v_coords = a_coords;\n";
		frag << "	o_color = vec4(res.rgb, 1.0);\n";

		if (loopCountType == LOOPCOUNT_DYNAMIC)
			vtx << "	v_one = a_one;\n";
	}

	vtx << "}\n";
	frag << "}\n";

	// Constants.
	std::string oneStr;
	std::string twoStr;
	std::string threeStr;
	std::string iterCountStr;

	if (loopCountType == LOOPCOUNT_CONSTANT)
	{
		oneStr			= "1";
		twoStr			= "2";
		threeStr		= "3";
		iterCountStr	= de::toString(iterCount);
	}
	else if (loopCountType == LOOPCOUNT_UNIFORM)
	{
		oneStr			= "ui_one";
		twoStr			= "ui_two";
		threeStr		= "ui_three";
		iterCountStr	= getIntUniformName(iterCount);
	}
	else if (loopCountType == LOOPCOUNT_DYNAMIC)
	{
		oneStr			= "one*ui_one";
		twoStr			= "one*ui_two";
		threeStr		= "one*ui_three";
		iterCountStr	= std::string("one*") + getIntUniformName(iterCount);
	}
	else DE_ASSERT(false);

	// Fill in shader templates.
	std::map<std::string, std::string> params;
	params.insert(std::pair<std::string, std::string>("PRECISION", "mediump"));
	params.insert(std::pair<std::string, std::string>("ITER_COUNT", iterCountStr));
	params.insert(std::pair<std::string, std::string>("COUNTER_PRECISION", counterPrecisionStr));
	params.insert(std::pair<std::string, std::string>("FOR_LOOP", forLoopStr));
	params.insert(std::pair<std::string, std::string>("WHILE_LOOP", whileLoopStr));
	params.insert(std::pair<std::string, std::string>("DO_WHILE_PRE", doWhileLoopPreStr));
	params.insert(std::pair<std::string, std::string>("DO_WHILE_POST", doWhileLoopPostStr));
	params.insert(std::pair<std::string, std::string>("ONE", oneStr));
	params.insert(std::pair<std::string, std::string>("TWO", twoStr));
	params.insert(std::pair<std::string, std::string>("THREE", threeStr));

	tcu::StringTemplate vertTemplate(vtx.str());
	tcu::StringTemplate fragTemplate(frag.str());
	std::string vertexShaderSource = vertTemplate.specialize(params);
	std::string fragmentShaderSource = fragTemplate.specialize(params);

	// Create the case.
	UniformSetup* uniformSetup = new LoopUniformSetup(uniformInformations);
	ShaderEvalFunc evalFunc = getLoopEvalFunc(numIters);
	return de::MovePtr<ShaderLoopCase>(new ShaderLoopCase(testCtx, caseName, description, isVertexCase, evalFunc, uniformSetup, vertexShaderSource, fragmentShaderSource));
}

class ShaderLoopTests : public tcu::TestCaseGroup
{
public:
							ShaderLoopTests			(tcu::TestContext& testCtx);
	virtual					~ShaderLoopTests		(void);

	virtual void			init					(void);

private:
							ShaderLoopTests			(const ShaderLoopTests&);		// not allowed!
	ShaderLoopTests&		operator=				(const ShaderLoopTests&);		// not allowed!
};

ShaderLoopTests::ShaderLoopTests(tcu::TestContext& testCtx)
		: TestCaseGroup(testCtx, "loops", "Loop Tests")
{
}

ShaderLoopTests::~ShaderLoopTests (void)
{
}

void ShaderLoopTests::init (void)
{
	// Loop cases.

	static const glu::ShaderType s_shaderTypes[] =
	{
		glu::SHADERTYPE_VERTEX,
		glu::SHADERTYPE_FRAGMENT
	};

	static const glu::DataType s_countDataType[] =
	{
		glu::TYPE_INT,
		glu::TYPE_FLOAT
	};

	TestCaseGroup* genericGroup = new TestCaseGroup(m_testCtx, "generic", "Generic loop tests.");
	TestCaseGroup* specialGroup = new TestCaseGroup(m_testCtx, "special", "Special loop tests.");
	addChild(genericGroup);
	addChild(specialGroup);

	for (int loopType = 0; loopType < LOOPTYPE_LAST; loopType++)
	{
		const char* loopTypeName = getLoopTypeName((LoopType)loopType);

		for (int loopCountType = 0; loopCountType < LOOPCOUNT_LAST; loopCountType++)
		{
			const char* loopCountName = getLoopCountTypeName((LoopCountType)loopCountType);

			std::string groupName = std::string(loopTypeName) + "_" + std::string(loopCountName) + "_iterations";
			std::string groupDesc = std::string("Loop tests with ") + loopCountName + " loop counter.";
			TestCaseGroup* genericSubGroup = new TestCaseGroup(m_testCtx, groupName.c_str(), groupDesc.c_str());
			TestCaseGroup* specialSubGroup = new TestCaseGroup(m_testCtx, groupName.c_str(), groupDesc.c_str());
			genericGroup->addChild(genericSubGroup);
			specialGroup->addChild(specialSubGroup);

			// Generic cases.

			for (int precision = glu::PRECISION_MEDIUMP; precision < glu::PRECISION_LAST; precision++)
			{
				const char* precisionName = getPrecisionName((glu::Precision)precision);

				for (int dataTypeNdx = 0; dataTypeNdx < DE_LENGTH_OF_ARRAY(s_countDataType); dataTypeNdx++)
				{
					glu::DataType loopDataType = s_countDataType[dataTypeNdx];
					const char* dataTypeName = getDataTypeName(loopDataType);

					for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(s_shaderTypes); shaderTypeNdx++)
					{
						glu::ShaderType	shaderType		= s_shaderTypes[shaderTypeNdx];
						const char*	shaderTypeName	= getShaderTypeName(shaderType);
						bool		isVertexCase	= (shaderType == glu::SHADERTYPE_VERTEX);

						std::string testName = std::string("basic_") + precisionName + "_" + dataTypeName + "_" + shaderTypeName;
						std::string testDesc = std::string(loopTypeName) + " loop with " + precisionName + dataTypeName + " " + loopCountName + " iteration count in " + shaderTypeName + " shader.";
						de::MovePtr<ShaderLoopCase> testCase(createGenericLoopCase(m_testCtx, testName.c_str(), testDesc.c_str(), isVertexCase, (LoopType)loopType, (LoopCountType)loopCountType, (glu::Precision)precision, loopDataType));
						genericSubGroup->addChild(testCase.release());
					}
				}
			}

			// Special cases.

			for (int loopCase = 0; loopCase < LOOPCASE_LAST; loopCase++)
			{
				const char* loopCaseName = getLoopCaseName((LoopCase)loopCase);

				// no-iterations not possible with do-while.
				if ((loopCase == LOOPCASE_NO_ITERATIONS) && (loopType == LOOPTYPE_DO_WHILE))
					continue;

				for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(s_shaderTypes); shaderTypeNdx++)
				{
					glu::ShaderType	shaderType		= s_shaderTypes[shaderTypeNdx];
					const char*	shaderTypeName	= getShaderTypeName(shaderType);
					bool		isVertexCase	= (shaderType == glu::SHADERTYPE_VERTEX);

					std::string name = std::string(loopCaseName) + "_" + shaderTypeName;
					std::string desc = std::string(loopCaseName) + " loop with " + loopTypeName + " iteration count in " + shaderTypeName + " shader.";
					de::MovePtr<ShaderLoopCase> testCase(createSpecialLoopCase(m_testCtx, name.c_str(), desc.c_str(), isVertexCase, (LoopCase)loopCase, (LoopType)loopType, (LoopCountType)loopCountType));
					specialSubGroup->addChild(testCase.release());
				}
			}
		}
	}
}

} // anonymous

tcu::TestCaseGroup* createLoopTests (tcu::TestContext& testCtx)
{
	return new ShaderLoopTests(testCtx);
}


} // sr
} // vkt
