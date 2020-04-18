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
 * \brief Shader switch statement tests.
 *
 * Variables:
 *  + Selection expression type: static, uniform, dynamic
 *  + Switch layout - fall-through or use of default label
 *  + Switch nested in loop/conditional statement
 *  + Loop/conditional statement nested in switch
 *//*--------------------------------------------------------------------*/

#include "vktShaderRenderSwitchTests.hpp"
#include "vktShaderRender.hpp"
#include "tcuStringTemplate.hpp"
#include "deMath.h"

namespace vkt
{
namespace sr
{
namespace
{

static void setUniforms(ShaderRenderCaseInstance& instance, const tcu::Vec4&)
{
	instance.useUniform(0u, UI_TWO);
}

using std::string;

class ShaderSwitchCase : public ShaderRenderCase
{
public:
						ShaderSwitchCase			(tcu::TestContext&	testCtx,
													 const string&		name,
													 const string&		description,
													 bool				isVertexCase,
													 const string&		vtxSource,
													 const string&		fragSource,
													 ShaderEvalFunc		evalFunc,
													 UniformSetupFunc	setupUniformsFunc);
	virtual				~ShaderSwitchCase			(void);
};

ShaderSwitchCase::ShaderSwitchCase (tcu::TestContext&	testCtx,
									const string&		name,
									const string&		description,
									bool				isVertexCase,
									const string&		vtxSource,
									const string&		fragSource,
									ShaderEvalFunc		evalFunc,
									UniformSetupFunc	setupUniformsFunc)
	: ShaderRenderCase (testCtx, name, description, isVertexCase, evalFunc, new UniformSetup(setupUniformsFunc), DE_NULL)
{
	m_vertShaderSource	= vtxSource;
	m_fragShaderSource	= fragSource;
}

ShaderSwitchCase::~ShaderSwitchCase (void)
{
}

enum SwitchType
{
	SWITCHTYPE_STATIC = 0,
	SWITCHTYPE_UNIFORM,
	SWITCHTYPE_DYNAMIC,

	SWITCHTYPE_LAST
};

static void evalSwitchStatic	(ShaderEvalContext& evalCtx)	{ evalCtx.color.xyz() = evalCtx.coords.swizzle(1,2,3); }
static void evalSwitchUniform	(ShaderEvalContext& evalCtx)	{ evalCtx.color.xyz() = evalCtx.coords.swizzle(1,2,3); }
static void evalSwitchDynamic	(ShaderEvalContext& evalCtx)
{
	switch (int(deFloatFloor(evalCtx.coords.z()*1.5f + 2.0f)))
	{
		case 0:		evalCtx.color.xyz() = evalCtx.coords.swizzle(0,1,2);	break;
		case 1:		evalCtx.color.xyz() = evalCtx.coords.swizzle(3,2,1);	break;
		case 2:		evalCtx.color.xyz() = evalCtx.coords.swizzle(1,2,3);	break;
		case 3:		evalCtx.color.xyz() = evalCtx.coords.swizzle(2,1,0);	break;
		default:	evalCtx.color.xyz() = evalCtx.coords.swizzle(0,0,0);	break;
	}
}

static de::MovePtr<ShaderSwitchCase> makeSwitchCase (tcu::TestContext& testCtx, const string& name, const string& desc, SwitchType type, bool isVertex, const LineStream& switchBody)
{
	std::ostringstream	vtx;
	std::ostringstream	frag;
	std::ostringstream&	op		= isVertex ? vtx : frag;

	vtx << "#version 310 es\n"
		<< "layout(location = 0) in highp vec4 a_position;\n"
		<< "layout(location = 1) in highp vec4 a_coords;\n\n";
	frag	<< "#version 310 es\n"
			<< "layout(location = 0) out mediump vec4 o_color;\n";

	if (isVertex)
	{
		vtx << "layout(location = 0) out mediump vec4 v_color;\n";
		frag << "layout(location = 0) in mediump vec4 v_color;\n";
	}
	else
	{
		vtx << "layout(location = 0) out highp vec4 v_coords;\n";
		frag << "layout(location = 0) in highp vec4 v_coords;\n";
	}

	if (type == SWITCHTYPE_UNIFORM)
		op << "layout (std140, set=0, binding=0) uniform buffer0 { highp int ui_two; };\n";

	vtx << "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position = a_position;\n";
	frag << "\n"
		 << "void main (void)\n"
		 << "{\n";

	// Setup.
	op << "	highp vec4 coords = " << (isVertex ? "a_coords" : "v_coords") << ";\n";
	op << "	mediump vec3 res = vec3(0.0);\n\n";

	// Switch body.
	std::map<string, string> params;
	params["CONDITION"] = type == SWITCHTYPE_STATIC		? "2"								:
						  type == SWITCHTYPE_UNIFORM	? "ui_two"							:
						  type == SWITCHTYPE_DYNAMIC	? "int(floor(coords.z*1.5 + 2.0))"	: "???";

	op << tcu::StringTemplate(switchBody.str()).specialize(params);
	op << "\n";

	if (isVertex)
	{
		vtx << "	v_color = vec4(res, 1.0);\n";
		frag << "	o_color = v_color;\n";
	}
	else
	{
		vtx << "	v_coords = a_coords;\n";
		frag << "	o_color = vec4(res, 1.0);\n";
	}

	vtx << "}\n";
	frag << "}\n";

	return de::MovePtr<ShaderSwitchCase>(new ShaderSwitchCase(testCtx, name, desc, isVertex, vtx.str(), frag.str(),
															type == SWITCHTYPE_STATIC	? evalSwitchStatic	:
															type == SWITCHTYPE_UNIFORM	? evalSwitchUniform	:
															type == SWITCHTYPE_DYNAMIC	? evalSwitchDynamic	: (ShaderEvalFunc)DE_NULL,
															type == SWITCHTYPE_UNIFORM	? setUniforms : DE_NULL));
}

class ShaderSwitchTests : public tcu::TestCaseGroup
{
public:
							ShaderSwitchTests		(tcu::TestContext& context);
	virtual					~ShaderSwitchTests		(void);

	virtual void			init					(void);

private:
							ShaderSwitchTests		(const ShaderSwitchTests&);		// not allowed!
	ShaderSwitchTests&		operator=				(const ShaderSwitchTests&);		// not allowed!

	void					makeSwitchCases			(const string& name, const string& desc, const LineStream& switchBody);
};

ShaderSwitchTests::ShaderSwitchTests (tcu::TestContext& testCtx)
	: tcu::TestCaseGroup (testCtx, "switch", "Switch statement tests")
{
}

ShaderSwitchTests::~ShaderSwitchTests (void)
{
}

void ShaderSwitchTests::makeSwitchCases (const string& name, const string& desc, const LineStream& switchBody)
{
	static const char* switchTypeNames[] = { "static", "uniform", "dynamic" };
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(switchTypeNames) == SWITCHTYPE_LAST);

	for (int type = 0; type < SWITCHTYPE_LAST; type++)
	{
		addChild(makeSwitchCase(m_testCtx, (name + "_" + switchTypeNames[type] + "_vertex"),	desc, (SwitchType)type, true,	switchBody).release());
		addChild(makeSwitchCase(m_testCtx, (name + "_" + switchTypeNames[type] + "_fragment"),	desc, (SwitchType)type, false,	switchBody).release());
	}
}

void ShaderSwitchTests::init (void)
{
	// Expected swizzles:
	// 0: xyz
	// 1: wzy
	// 2: yzw
	// 3: zyx

	makeSwitchCases("basic", "Basic switch statement usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 2:		res = coords.yzw;	break;"
		<< "	case 3:		res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("const_expr_in_label", "Constant expression in label",
		LineStream(1)
		<< "const int t = 2;"
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case int(0.0):	res = coords.xyz;	break;"
		<< "	case 2-1:		res = coords.wzy;	break;"
		<< "	case 3&(1<<1):	res = coords.yzw;	break;"
		<< "	case t+1:		res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("default_label", "Default label usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 3:		res = coords.zyx;	break;"
		<< "	default:	res = coords.yzw;"
		<< "}");

	makeSwitchCases("default_not_last", "Default label usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	default:	res = coords.yzw;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 3:		res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("no_default_label", "No match in switch without default label",
		LineStream(1)
		<< "res = coords.yzw;\n"
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 3:		res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("fall_through", "Fall-through",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 2:		coords = coords.yzwx;"
		<< "	case 4:		res = vec3(coords);	break;"
		<< "	case 3:		res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("fall_through_default", "Fall-through",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 3:		res = coords.zyx;	break;"
		<< "	case 2:		coords = coords.yzwx;"
		<< "	default:	res = vec3(coords);"
		<< "}");

	makeSwitchCases("conditional_fall_through", "Fall-through",
		LineStream(1)
		<< "highp vec4 tmp = coords;"
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 2:"
		<< "		tmp = coords.yzwx;"
		<< "	case 3:"
		<< "		res = vec3(tmp);"
		<< "		if (${CONDITION} != 3)"
		<< "			break;"
		<< "	default:	res = tmp.zyx;		break;"
		<< "}");

	makeSwitchCases("conditional_fall_through_2", "Fall-through",
		LineStream(1)
		<< "highp vec4 tmp = coords;"
		<< "mediump int c = ${CONDITION};"
		<< "switch (c)"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 2:"
		<< "		c += ${CONDITION};"
		<< "		tmp = coords.yzwx;"
		<< "	case 3:"
		<< "		res = vec3(tmp);"
		<< "		if (c == 4)"
		<< "			break;"
		<< "	default:	res = tmp.zyx;		break;"
		<< "}");

	makeSwitchCases("scope", "Basic switch statement usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	case 2:"
		<< "	{"
		<< "		mediump vec3 t = coords.yzw;"
		<< "		res = t;"
		<< "		break;"
		<< "	}"
		<< "	case 3:		res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("switch_in_if", "Switch in for loop",
		LineStream(1)
		<< "if (${CONDITION} >= 0)"
		<< "{"
		<< "	switch (${CONDITION})"
		<< "	{"
		<< "		case 0:		res = coords.xyz;	break;"
		<< "		case 1:		res = coords.wzy;	break;"
		<< "		case 2:		res = coords.yzw;	break;"
		<< "		case 3:		res = coords.zyx;	break;"
		<< "	}"
		<< "}");

	makeSwitchCases("switch_in_for_loop", "Switch in for loop",
		LineStream(1)
		<< "for (int i = 0; i <= ${CONDITION}; i++)"
		<< "{"
		<< "	switch (i)"
		<< "	{"
		<< "		case 0:		res = coords.xyz;	break;"
		<< "		case 1:		res = coords.wzy;	break;"
		<< "		case 2:		res = coords.yzw;	break;"
		<< "		case 3:		res = coords.zyx;	break;"
		<< "	}"
		<< "}");


	makeSwitchCases("switch_in_while_loop", "Switch in while loop",
		LineStream(1)
		<< "int i = 0;"
		<< "while (i <= ${CONDITION})"
		<< "{"
		<< "	switch (i)"
		<< "	{"
		<< "		case 0:		res = coords.xyz;	break;"
		<< "		case 1:		res = coords.wzy;	break;"
		<< "		case 2:		res = coords.yzw;	break;"
		<< "		case 3:		res = coords.zyx;	break;"
		<< "	}"
		<< "	i += 1;"
		<< "}");

	makeSwitchCases("switch_in_do_while_loop", "Switch in do-while loop",
		LineStream(1)
		<< "int i = 0;"
		<< "do"
		<< "{"
		<< "	switch (i)"
		<< "	{"
		<< "		case 0:		res = coords.xyz;	break;"
		<< "		case 1:		res = coords.wzy;	break;"
		<< "		case 2:		res = coords.yzw;	break;"
		<< "		case 3:		res = coords.zyx;	break;"
		<< "	}"
		<< "	i += 1;"
		<< "} while (i <= ${CONDITION});");

	makeSwitchCases("if_in_switch", "Basic switch statement usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:		res = coords.wzy;	break;"
		<< "	default:"
		<< "		if (${CONDITION} == 2)"
		<< "			res = coords.yzw;"
		<< "		else"
		<< "			res = coords.zyx;"
		<< "		break;"
		<< "}");

	makeSwitchCases("for_loop_in_switch", "Basic switch statement usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:"
		<< "	case 2:"
		<< "	{"
		<< "		highp vec3 t = coords.yzw;"
		<< "		for (int i = 0; i < ${CONDITION}; i++)"
		<< "			t = t.zyx;"
		<< "		res = t;"
		<< "		break;"
		<< "	}"
		<< "	default:	res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("while_loop_in_switch", "Basic switch statement usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:"
		<< "	case 2:"
		<< "	{"
		<< "		highp vec3 t = coords.yzw;"
		<< "		int i = 0;"
		<< "		while (i < ${CONDITION})"
		<< "		{"
		<< "			t = t.zyx;"
		<< "			i += 1;"
		<< "		}"
		<< "		res = t;"
		<< "		break;"
		<< "	}"
		<< "	default:	res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("do_while_loop_in_switch", "Basic switch statement usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:"
		<< "	case 2:"
		<< "	{"
		<< "		highp vec3 t = coords.yzw;"
		<< "		int i = 0;"
		<< "		do"
		<< "		{"
		<< "			t = t.zyx;"
		<< "			i += 1;"
		<< "		} while (i < ${CONDITION});"
		<< "		res = t;"
		<< "		break;"
		<< "	}"
		<< "	default:	res = coords.zyx;	break;"
		<< "}");

	makeSwitchCases("switch_in_switch", "Basic switch statement usage",
		LineStream(1)
		<< "switch (${CONDITION})"
		<< "{"
		<< "	case 0:		res = coords.xyz;	break;"
		<< "	case 1:"
		<< "	case 2:"
		<< "		switch (${CONDITION} - 1)"
		<< "		{"
		<< "			case 0:		res = coords.wzy;	break;"
		<< "			case 1:		res = coords.yzw;	break;"
		<< "		}"
		<< "		break;"
		<< "	default:	res = coords.zyx;	break;"
		<< "}");
}

} // anonymous

tcu::TestCaseGroup* createSwitchTests (tcu::TestContext& testCtx)
{
	return new ShaderSwitchTests(testCtx);
}

} // sr
} // vkt
