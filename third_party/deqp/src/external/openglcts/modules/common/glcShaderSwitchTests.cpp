/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief Shader switch statement tests.
 */ /*-------------------------------------------------------------------*/

#include "glcShaderSwitchTests.hpp"
#include "deMath.h"
#include "glcShaderLibrary.hpp"
#include "glcShaderRenderCase.hpp"
#include "tcuStringTemplate.hpp"

namespace deqp
{

using std::string;
using std::map;
using std::vector;

class ShaderSwitchCase : public ShaderRenderCase
{
public:
	ShaderSwitchCase(Context& context, const char* name, const char* description, bool isVertexCase,
					 const char* vtxSource, const char* fragSource, ShaderEvalFunc evalFunc);
	virtual ~ShaderSwitchCase(void);
};

ShaderSwitchCase::ShaderSwitchCase(Context& context, const char* name, const char* description, bool isVertexCase,
								   const char* vtxSource, const char* fragSource, ShaderEvalFunc evalFunc)
	: ShaderRenderCase(context.getTestContext(), context.getRenderContext(), context.getContextInfo(), name,
					   description, isVertexCase, evalFunc)
{
	m_vertShaderSource = vtxSource;
	m_fragShaderSource = fragSource;
}

ShaderSwitchCase::~ShaderSwitchCase(void)
{
}

enum SwitchType
{
	SWITCHTYPE_STATIC = 0,
	SWITCHTYPE_UNIFORM,
	SWITCHTYPE_DYNAMIC,

	SWITCHTYPE_LAST
};

static void evalSwitchStatic(ShaderEvalContext& evalCtx)
{
	evalCtx.color.xyz() = evalCtx.coords.swizzle(1, 2, 3);
}
static void evalSwitchUniform(ShaderEvalContext& evalCtx)
{
	evalCtx.color.xyz() = evalCtx.coords.swizzle(1, 2, 3);
}
static void evalSwitchDynamic(ShaderEvalContext& evalCtx)
{
	switch (int(deFloatFloor(evalCtx.coords.z() * 1.5f + 2.0f)))
	{
	case 0:
		evalCtx.color.xyz() = evalCtx.coords.swizzle(0, 1, 2);
		break;
	case 1:
		evalCtx.color.xyz() = evalCtx.coords.swizzle(3, 2, 1);
		break;
	case 2:
		evalCtx.color.xyz() = evalCtx.coords.swizzle(1, 2, 3);
		break;
	case 3:
		evalCtx.color.xyz() = evalCtx.coords.swizzle(2, 1, 0);
		break;
	default:
		evalCtx.color.xyz() = evalCtx.coords.swizzle(0, 0, 0);
		break;
	}
}

static tcu::TestCase* makeSwitchCase(Context& context, glu::GLSLVersion glslVersion, const char* name, const char* desc,
									 SwitchType type, bool isVertex, const LineStream& switchBody)
{
	std::ostringstream  vtx;
	std::ostringstream  frag;
	std::ostringstream& op = isVertex ? vtx : frag;

	vtx << glu::getGLSLVersionDeclaration(glslVersion) << "\n"
		<< "in highp vec4 a_position;\n"
		<< "in highp vec4 a_coords;\n";
	frag << glu::getGLSLVersionDeclaration(glslVersion) << "\n"
		 << "layout(location = 0) out mediump vec4 o_color;\n";

	if (isVertex)
	{
		vtx << "out mediump vec4 v_color;\n";
		frag << "in mediump vec4 v_color;\n";
	}
	else
	{
		vtx << "out highp vec4 v_coords;\n";
		frag << "in highp vec4 v_coords;\n";
	}

	if (type == SWITCHTYPE_UNIFORM)
		op << "uniform highp int ui_two;\n";

	vtx << "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "    gl_Position = a_position;\n";
	frag << "\n"
		 << "void main (void)\n"
		 << "{\n";

	// Setup.
	op << " highp vec4 coords = " << (isVertex ? "a_coords" : "v_coords") << ";\n";
	op << " mediump vec3 res = vec3(0.0);\n\n";

	// Switch body.
	map<string, string> params;
	params["CONDITION"] = type == SWITCHTYPE_STATIC ?
							  "2" :
							  type == SWITCHTYPE_UNIFORM ?
							  "ui_two" :
							  type == SWITCHTYPE_DYNAMIC ? "int(floor(coords.z*1.5 + 2.0))" : "???";

	op << tcu::StringTemplate(switchBody.str()).specialize(params).c_str();
	op << "\n";

	if (isVertex)
	{
		vtx << "    v_color = vec4(res, 1.0);\n";
		frag << "   o_color = v_color;\n";
	}
	else
	{
		vtx << "    v_coords = a_coords;\n";
		frag << "   o_color = vec4(res, 1.0);\n";
	}

	vtx << "}\n";
	frag << "}\n";

	return new ShaderSwitchCase(context, name, desc, isVertex, vtx.str().c_str(), frag.str().c_str(),
								type == SWITCHTYPE_STATIC ?
									evalSwitchStatic :
									type == SWITCHTYPE_UNIFORM ?
									evalSwitchUniform :
									type == SWITCHTYPE_DYNAMIC ? evalSwitchDynamic : (ShaderEvalFunc)DE_NULL);
}

static void makeSwitchCases(TestCaseGroup* group, glu::GLSLVersion glslVersion, const char* name, const char* desc,
							const LineStream& switchBody)
{
	static const char* switchTypeNames[] = { "static", "uniform", "dynamic" };
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(switchTypeNames) == SWITCHTYPE_LAST);

	for (int type = 0; type < SWITCHTYPE_LAST; type++)
	{
		group->addChild(makeSwitchCase(group->getContext(), glslVersion,
									   (string(name) + "_" + switchTypeNames[type] + "_vertex").c_str(), desc,
									   (SwitchType)type, true, switchBody));
		group->addChild(makeSwitchCase(group->getContext(), glslVersion,
									   (string(name) + "_" + switchTypeNames[type] + "_fragment").c_str(), desc,
									   (SwitchType)type, false, switchBody));
	}
}

ShaderSwitchTests::ShaderSwitchTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "switch", "Switch statement tests"), m_glslVersion(glslVersion)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_300_ES || glslVersion == glu::GLSL_VERSION_310_ES ||
			  glslVersion == glu::GLSL_VERSION_330);
}

ShaderSwitchTests::~ShaderSwitchTests(void)
{
}

void ShaderSwitchTests::init(void)
{
	// Expected swizzles:
	// 0: xyz
	// 1: wzy
	// 2: yzw
	// 3: zyx

	makeSwitchCases(this, m_glslVersion, "basic", "Basic switch statement usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 2:     res = coords.yzw;   break;"
								  << "    case 3:     res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "const_expr_in_label", "Constant expression in label",
					LineStream(1) << "const int t = 2;"
								  << "switch (${CONDITION})"
								  << "{"
								  << "    case int(0.0):  res = coords.xyz;   break;"
								  << "    case 2-1:       res = coords.wzy;   break;"
								  << "    case 3&(1<<1):  res = coords.yzw;   break;"
								  << "    case t+1:       res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "default_label", "Default label usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 3:     res = coords.zyx;   break;"
								  << "    default:    res = coords.yzw;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "default_not_last", "Default label usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    default:    res = coords.yzw;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 3:     res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "no_default_label", "No match in switch without default label",
					LineStream(1) << "res = coords.yzw;\n"
								  << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 3:     res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "fall_through", "Fall-through",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 2:     coords = coords.yzwx;"
								  << "    case 4:     res = vec3(coords); break;"
								  << "    case 3:     res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "fall_through_default", "Fall-through",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 3:     res = coords.zyx;   break;"
								  << "    case 2:     coords = coords.yzwx;"
								  << "    default:    res = vec3(coords);"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "conditional_fall_through", "Fall-through",
					LineStream(1) << "highp vec4 tmp = coords;"
								  << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 2:"
								  << "        tmp = coords.yzwx;"
								  << "    case 3:"
								  << "        res = vec3(tmp);"
								  << "        if (${CONDITION} != 3)"
								  << "            break;"
								  << "    default:    res = tmp.zyx;      break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "conditional_fall_through_2", "Fall-through",
					LineStream(1) << "highp vec4 tmp = coords;"
								  << "mediump int c = ${CONDITION};"
								  << "switch (c)"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 2:"
								  << "        c += ${CONDITION};"
								  << "        tmp = coords.yzwx;"
								  << "    case 3:"
								  << "        res = vec3(tmp);"
								  << "        if (c == 4)"
								  << "            break;"
								  << "    default:    res = tmp.zyx;      break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "scope", "Basic switch statement usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    case 2:"
								  << "    {"
								  << "        mediump vec3 t = coords.yzw;"
								  << "        res = t;"
								  << "        break;"
								  << "    }"
								  << "    case 3:     res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "switch_in_if", "Switch in for loop",
					LineStream(1) << "if (${CONDITION} >= 0)"
								  << "{"
								  << "    switch (${CONDITION})"
								  << "    {"
								  << "        case 0:     res = coords.xyz;   break;"
								  << "        case 1:     res = coords.wzy;   break;"
								  << "        case 2:     res = coords.yzw;   break;"
								  << "        case 3:     res = coords.zyx;   break;"
								  << "    }"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "switch_in_for_loop", "Switch in for loop",
					LineStream(1) << "for (int i = 0; i <= ${CONDITION}; i++)"
								  << "{"
								  << "    switch (i)"
								  << "    {"
								  << "        case 0:     res = coords.xyz;   break;"
								  << "        case 1:     res = coords.wzy;   break;"
								  << "        case 2:     res = coords.yzw;   break;"
								  << "        case 3:     res = coords.zyx;   break;"
								  << "    }"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "switch_in_while_loop", "Switch in while loop",
					LineStream(1) << "int i = 0;"
								  << "while (i <= ${CONDITION})"
								  << "{"
								  << "    switch (i)"
								  << "    {"
								  << "        case 0:     res = coords.xyz;   break;"
								  << "        case 1:     res = coords.wzy;   break;"
								  << "        case 2:     res = coords.yzw;   break;"
								  << "        case 3:     res = coords.zyx;   break;"
								  << "    }"
								  << "    i += 1;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "switch_in_do_while_loop", "Switch in do-while loop",
					LineStream(1) << "int i = 0;"
								  << "do"
								  << "{"
								  << "    switch (i)"
								  << "    {"
								  << "        case 0:     res = coords.xyz;   break;"
								  << "        case 1:     res = coords.wzy;   break;"
								  << "        case 2:     res = coords.yzw;   break;"
								  << "        case 3:     res = coords.zyx;   break;"
								  << "    }"
								  << "    i += 1;"
								  << "} while (i <= ${CONDITION});");

	makeSwitchCases(this, m_glslVersion, "if_in_switch", "Basic switch statement usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:     res = coords.wzy;   break;"
								  << "    default:"
								  << "        if (${CONDITION} == 2)"
								  << "            res = coords.yzw;"
								  << "        else"
								  << "            res = coords.zyx;"
								  << "        break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "for_loop_in_switch", "Basic switch statement usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:"
								  << "    case 2:"
								  << "    {"
								  << "        highp vec3 t = coords.yzw;"
								  << "        for (int i = 0; i < ${CONDITION}; i++)"
								  << "            t = t.zyx;"
								  << "        res = t;"
								  << "        break;"
								  << "    }"
								  << "    default:    res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "while_loop_in_switch", "Basic switch statement usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:"
								  << "    case 2:"
								  << "    {"
								  << "        highp vec3 t = coords.yzw;"
								  << "        int i = 0;"
								  << "        while (i < ${CONDITION})"
								  << "        {"
								  << "            t = t.zyx;"
								  << "            i += 1;"
								  << "        }"
								  << "        res = t;"
								  << "        break;"
								  << "    }"
								  << "    default:    res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "do_while_loop_in_switch", "Basic switch statement usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:"
								  << "    case 2:"
								  << "    {"
								  << "        highp vec3 t = coords.yzw;"
								  << "        int i = 0;"
								  << "        do"
								  << "        {"
								  << "            t = t.zyx;"
								  << "            i += 1;"
								  << "        } while (i < ${CONDITION});"
								  << "        res = t;"
								  << "        break;"
								  << "    }"
								  << "    default:    res = coords.zyx;   break;"
								  << "}");

	makeSwitchCases(this, m_glslVersion, "switch_in_switch", "Basic switch statement usage",
					LineStream(1) << "switch (${CONDITION})"
								  << "{"
								  << "    case 0:     res = coords.xyz;   break;"
								  << "    case 1:"
								  << "    case 2:"
								  << "        switch (${CONDITION} - 1)"
								  << "        {"
								  << "            case 0:     res = coords.wzy;   break;"
								  << "            case 1:     res = coords.yzw;   break;"
								  << "        }"
								  << "        break;"
								  << "    default:    res = coords.zyx;   break;"
								  << "}");

	// Negative cases.
	{
		ShaderLibrary library(m_testCtx, m_context.getRenderContext());
		bool		  isES3 = m_glslVersion == glu::GLSL_VERSION_300_ES || m_glslVersion == glu::GLSL_VERSION_310_ES;
		std::string   path  = "";

		if (!isES3)
		{
			path += "gl33/";
		}
		path += "switch.test";
		vector<tcu::TestNode*> negativeCases = library.loadShaderFile(path.c_str());

		for (vector<tcu::TestNode*>::iterator i = negativeCases.begin(); i != negativeCases.end(); i++)
			addChild(*i);
	}
}

} // deqp
