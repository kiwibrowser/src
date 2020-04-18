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
 * \brief Shader return statement tests.
 *//*--------------------------------------------------------------------*/

#include "vktShaderRenderReturnTests.hpp"
#include "vktShaderRender.hpp"
#include "tcuStringTemplate.hpp"

#include <map>
#include <string>

namespace vkt
{
namespace sr
{
namespace
{

enum ReturnMode
{
	RETURNMODE_ALWAYS = 0,
	RETURNMODE_NEVER,
	RETURNMODE_DYNAMIC,

	RETURNMODE_LAST
};

// Evaluation functions
inline void evalReturnAlways	(ShaderEvalContext& c) { c.color.xyz() = c.coords.swizzle(0,1,2); }
inline void evalReturnNever		(ShaderEvalContext& c) { c.color.xyz() = c.coords.swizzle(3,2,1); }
inline void evalReturnDynamic	(ShaderEvalContext& c) { c.color.xyz() = (c.coords.x()+c.coords.y() >= 0.0f) ? c.coords.swizzle(0,1,2) : c.coords.swizzle(3,2,1); }

static ShaderEvalFunc getEvalFunc (ReturnMode mode)
{
	switch (mode)
	{
		case RETURNMODE_ALWAYS:		return evalReturnAlways;
		case RETURNMODE_NEVER:		return evalReturnNever;
		case RETURNMODE_DYNAMIC:	return evalReturnDynamic;
		default:
			DE_ASSERT(DE_FALSE);
			return (ShaderEvalFunc)DE_NULL;
	}
}

class ShaderReturnCase : public ShaderRenderCase
{
public:
								ShaderReturnCase		(tcu::TestContext&			testCtx,
														 const std::string&			name,
														 const std::string&			description,
														 bool						isVertexCase,
														 const std::string&			shaderSource,
														 const ShaderEvalFunc		evalFunc,
														 const UniformSetup*		uniformFunc);
	virtual						~ShaderReturnCase		(void);
};

ShaderReturnCase::ShaderReturnCase (tcu::TestContext&			testCtx,
									const std::string&			name,
									const std::string&			description,
									bool						isVertexCase,
									const std::string&			shaderSource,
									const ShaderEvalFunc		evalFunc,
									const UniformSetup*			uniformFunc)
	: ShaderRenderCase(testCtx, name, description, isVertexCase, evalFunc, uniformFunc, DE_NULL)
{
	if (isVertexCase)
	{
		m_vertShaderSource = shaderSource;
		m_fragShaderSource =
			"#version 310 es\n"
			"layout(location = 0) in mediump vec4 v_color;\n"
			"layout(location = 0) out mediump vec4 o_color;\n\n"
			"void main (void)\n"
			"{\n"
			"    o_color = v_color;\n"
			"}\n";
	}
	else
	{
		m_fragShaderSource = shaderSource;
		m_vertShaderSource =
			"#version 310 es\n"
			"layout(location = 0) in  highp   vec4 a_position;\n"
			"layout(location = 1) in  highp   vec4 a_coords;\n"
			"layout(location = 0) out mediump vec4 v_coords;\n\n"
			"void main (void)\n"
			"{\n"
			"    gl_Position = a_position;\n"
			"    v_coords = a_coords;\n"
			"}\n";
	}
}

ShaderReturnCase::~ShaderReturnCase (void)
{
}

class ReturnTestUniformSetup : public UniformSetup
{
public:
								ReturnTestUniformSetup	(const BaseUniformType uniformType)
									: m_uniformType(uniformType)
								{}
	virtual void				setup					(ShaderRenderCaseInstance& instance, const tcu::Vec4&) const
								{
									instance.useUniform(0u, m_uniformType);
								}

private:
	const BaseUniformType		m_uniformType;
};

// Test case builders.

de::MovePtr<ShaderReturnCase> makeConditionalReturnInFuncCase (tcu::TestContext& context, const std::string& name, const std::string& description, ReturnMode returnMode, bool isVertex)
{
	tcu::StringTemplate tmpl(
		"#version 310 es\n"
		"layout(location = ${COORDLOC}) in ${COORDPREC} vec4 ${COORDS};\n"
		"${EXTRADECL}\n"
		"${COORDPREC} vec4 getColor (void)\n"
		"{\n"
		"    if (${RETURNCOND})\n"
		"        return vec4(${COORDS}.xyz, 1.0);\n"
		"    return vec4(${COORDS}.wzy, 1.0);\n"
		"}\n\n"
		"void main (void)\n"
		"{\n"
		"${POSITIONWRITE}"
		"    ${OUTPUT} = getColor();\n"
		"}\n");

	const char* coords = isVertex ? "a_coords" : "v_coords";

	std::map<std::string, std::string> params;

	params["COORDLOC"]		= isVertex ? "1"			: "0";
	params["COORDPREC"]		= isVertex ? "highp"		: "mediump";
	params["OUTPUT"]		= isVertex ? "v_color"		: "o_color";
	params["COORDS"]		= coords;
	params["EXTRADECL"]		= isVertex ? "layout(location = 0) in highp vec4 a_position;\nlayout(location = 0) out mediump vec4 v_color;\n" : "layout(location = 0) out mediump vec4 o_color;\n";
	params["POSITIONWRITE"]	= isVertex ? "    gl_Position = a_position;\n" : "";

	switch (returnMode)
	{
		case RETURNMODE_ALWAYS:		params["RETURNCOND"] = "true";											break;
		case RETURNMODE_NEVER:		params["RETURNCOND"] = "false";											break;
		case RETURNMODE_DYNAMIC:	params["RETURNCOND"] = std::string(coords) + ".x+" + coords + ".y >= 0.0";	break;
		default:					DE_ASSERT(DE_FALSE);
	}

	return de::MovePtr<ShaderReturnCase>(new ShaderReturnCase(context, name, description, isVertex, tmpl.specialize(params), getEvalFunc(returnMode), DE_NULL));
}

de::MovePtr<ShaderReturnCase> makeOutputWriteReturnCase (tcu::TestContext& context, const std::string& name, const std::string& description, bool inFunction, ReturnMode returnMode, bool isVertex)
{
	tcu::StringTemplate tmpl(
		inFunction
		?
			"#version 310 es\n"
			"layout(location = ${COORDLOC}) in ${COORDPREC} vec4 ${COORDS};\n"
			"${EXTRADECL}\n"
			"void myfunc (void)\n"
			"{\n"
			"    ${OUTPUT} = vec4(${COORDS}.xyz, 1.0);\n"
			"    if (${RETURNCOND})\n"
			"        return;\n"
			"    ${OUTPUT} = vec4(${COORDS}.wzy, 1.0);\n"
			"}\n\n"
			"void main (void)\n"
			"{\n"
			"${POSITIONWRITE}"
			"    myfunc();\n"
			"}\n"
		:
			"#version 310 es\n"
			"layout(location = ${COORDLOC}) in ${COORDPREC} vec4 ${COORDS};\n"
			"${EXTRADECL}\n"
			"void main ()\n"
			"{\n"
			"${POSITIONWRITE}"
			"    ${OUTPUT} = vec4(${COORDS}.xyz, 1.0);\n"
			"    if (${RETURNCOND})\n"
			"        return;\n"
			"    ${OUTPUT} = vec4(${COORDS}.wzy, 1.0);\n"
			"}\n");

	const char* coords = isVertex ? "a_coords" : "v_coords";

	std::map<std::string, std::string> params;

	params["COORDLOC"]		= isVertex ? "1"			: "0";
	params["COORDPREC"]		= isVertex ? "highp"		: "mediump";
	params["COORDS"]		= coords;
	params["OUTPUT"]		= isVertex ? "v_color"		: "o_color";
	params["EXTRADECL"]		= isVertex ? "layout(location = 0) in highp vec4 a_position;\nlayout(location = 0) out mediump vec4 v_color;\n" : "layout(location = 0) out mediump vec4 o_color;\n";
	params["POSITIONWRITE"]	= isVertex ? "    gl_Position = a_position;\n" : "";

	switch (returnMode)
	{
		case RETURNMODE_ALWAYS:		params["RETURNCOND"] = "true";											break;
		case RETURNMODE_NEVER:		params["RETURNCOND"] = "false";											break;
		case RETURNMODE_DYNAMIC:	params["RETURNCOND"] = std::string(coords) + ".x+" + coords + ".y >= 0.0";	break;
		default:					DE_ASSERT(DE_FALSE);
	}

	return de::MovePtr<ShaderReturnCase>(new ShaderReturnCase(context, name, description, isVertex, tmpl.specialize(params), getEvalFunc(returnMode), DE_NULL));
}

de::MovePtr<ShaderReturnCase> makeReturnInLoopCase (tcu::TestContext& context, const std::string& name, const std::string& description, bool isDynamicLoop, ReturnMode returnMode, bool isVertex)
{
	tcu::StringTemplate tmpl(
		"#version 310 es\n"
		"layout(location = ${COORDLOC}) in ${COORDPREC} vec4 ${COORDS};\n"
		"layout(binding = 0, std140) uniform something { mediump int ui_one; };\n"
		"${EXTRADECL}\n"
		"${COORDPREC} vec4 getCoords (void)\n"
		"{\n"
		"    ${COORDPREC} vec4 coords = ${COORDS};\n"
		"    for (int i = 0; i < ${ITERLIMIT}; i++)\n"
		"    {\n"
		"        if (${RETURNCOND})\n"
		"            return coords;\n"
		"        coords = coords.wzyx;\n"
		"    }\n"
		"    return coords;\n"
		"}\n\n"
		"void main (void)\n"
		"{\n"
		"${POSITIONWRITE}"
		"    ${OUTPUT} = vec4(getCoords().xyz, 1.0);\n"
		"}\n");

	const char* coords = isVertex ? "a_coords" : "v_coords";

	std::map<std::string, std::string> params;

	params["COORDLOC"]		= isVertex ? "1"			: "0";
	params["COORDPREC"]		= isVertex ? "highp"		: "mediump";
	params["OUTPUT"]		= isVertex ? "v_color"		: "o_color";
	params["COORDS"]		= coords;
	params["EXTRADECL"]		= isVertex ? "layout(location = 0) in highp vec4 a_position;\nlayout(location = 0) out mediump vec4 v_color;\n" : "layout(location = 0) out mediump vec4 o_color;\n";
	params["POSITIONWRITE"]	= isVertex ? "    gl_Position = a_position;\n" : "";
	params["ITERLIMIT"]		= isDynamicLoop ? "ui_one" : "1";

	switch (returnMode)
	{
		case RETURNMODE_ALWAYS:		params["RETURNCOND"] = "true";											break;
		case RETURNMODE_NEVER:		params["RETURNCOND"] = "false";											break;
		case RETURNMODE_DYNAMIC:	params["RETURNCOND"] = std::string(coords) + ".x+" + coords + ".y >= 0.0";	break;
		default:					DE_ASSERT(DE_FALSE);
	}

	return de::MovePtr<ShaderReturnCase>(new ShaderReturnCase(context, name, description, isVertex, tmpl.specialize(params), getEvalFunc(returnMode), new ReturnTestUniformSetup(UI_ONE)));
}

static const char* getReturnModeName (ReturnMode mode)
{
	switch (mode)
	{
		case RETURNMODE_ALWAYS:		return "always";
		case RETURNMODE_NEVER:		return "never";
		case RETURNMODE_DYNAMIC:	return "dynamic";
		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

static const char* getReturnModeDesc (ReturnMode mode)
{
	switch (mode)
	{
		case RETURNMODE_ALWAYS:		return "Always return";
		case RETURNMODE_NEVER:		return "Never return";
		case RETURNMODE_DYNAMIC:	return "Return based on coords";
		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

class ShaderReturnTests : public tcu::TestCaseGroup
{
public:
							ShaderReturnTests		(tcu::TestContext& context);
	virtual					~ShaderReturnTests		(void);
	virtual void			init					(void);

private:
							ShaderReturnTests		(const ShaderReturnTests&);		// not allowed!
	ShaderReturnTests&		operator=				(const ShaderReturnTests&);		// not allowed!
};

ShaderReturnTests::ShaderReturnTests (tcu::TestContext& context)
	: TestCaseGroup(context, "return", "Return Statement Tests")
{
}

ShaderReturnTests::~ShaderReturnTests (void)
{
}

void ShaderReturnTests::init (void)
{
	addChild(new ShaderReturnCase(m_testCtx, "single_return_vertex", "Single return statement in function", true,
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 a_position;\n"
		"layout(location = 1) in highp vec4 a_coords;\n"
		"layout(location = 0) out mediump vec4 v_color;\n\n"
		"vec4 getColor (void)\n"
		"{\n"
		"    return vec4(a_coords.xyz, 1.0);\n"
		"}\n\n"
		"void main (void)\n"
		"{\n"
		"    gl_Position = a_position;\n"
		"    v_color = getColor();\n"
		"}\n", evalReturnAlways, DE_NULL));
	addChild(new ShaderReturnCase(m_testCtx, "single_return_fragment", "Single return statement in function", false,
		"#version 310 es\n"
		"layout(location = 0) in mediump vec4 v_coords;\n"
		"layout(location = 0) out mediump vec4 o_color;\n"
		"mediump vec4 getColor (void)\n"
		"{\n"
		"    return vec4(v_coords.xyz, 1.0);\n"
		"}\n\n"
		"void main (void)\n"
		"{\n"
		"    o_color = getColor();\n"
		"}\n", evalReturnAlways, DE_NULL));

	// Conditional return statement in function.
	for (int returnMode = 0; returnMode < RETURNMODE_LAST; returnMode++)
	{
		for (int isFragment = 0; isFragment < 2; isFragment++)
		{
			std::string						name		= std::string("conditional_return_") + getReturnModeName((ReturnMode)returnMode) + (isFragment ? "_fragment" : "_vertex");
			std::string						description	= std::string(getReturnModeDesc((ReturnMode)returnMode)) + " in function";
			de::MovePtr<ShaderReturnCase>	testCase	(makeConditionalReturnInFuncCase(m_testCtx, name, description, (ReturnMode)returnMode, isFragment == 0));
			addChild(testCase.release());
		}
	}

	// Unconditional double return in function.
	addChild(new ShaderReturnCase(m_testCtx, "double_return_vertex", "Unconditional double return in function", true,
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 a_position;\n"
		"layout(location = 1) in highp vec4 a_coords;\n"
		"layout(location = 0) out mediump vec4 v_color;\n\n"
		"vec4 getColor (void)\n"
		"{\n"
		"    return vec4(a_coords.xyz, 1.0);\n"
		"    return vec4(a_coords.wzy, 1.0);\n"
		"}\n\n"
		"void main (void)\n"
		"{\n"
		"    gl_Position = a_position;\n"
		"    v_color = getColor();\n"
		"}\n", evalReturnAlways, DE_NULL));
	addChild(new ShaderReturnCase(m_testCtx, "double_return_fragment", "Unconditional double return in function", false,
		"#version 310 es\n"
		"layout(location = 0) in mediump vec4 v_coords;\n"
		"layout(location = 0) out mediump vec4 o_color;\n\n"
		"mediump vec4 getColor (void)\n"
		"{\n"
		"    return vec4(v_coords.xyz, 1.0);\n"
		"    return vec4(v_coords.wzy, 1.0);\n"
		"}\n\n"
		"void main (void)\n"
		"{\n"
		"    o_color = getColor();\n"
		"}\n", evalReturnAlways, DE_NULL));

	// Last statement in main.
	addChild(new ShaderReturnCase(m_testCtx, "last_statement_in_main_vertex", "Return as a final statement in main()", true,
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 a_position;\n"
		"layout(location = 1) in highp vec4 a_coords;\n"
		"layout(location = 0) out mediump vec4 v_color;\n\n"
		"void main (void)\n"
		"{\n"
		"    gl_Position = a_position;\n"
		"    v_color = vec4(a_coords.xyz, 1.0);\n"
		"    return;\n"
		"}\n", evalReturnAlways, DE_NULL));
	addChild(new ShaderReturnCase(m_testCtx, "last_statement_in_main_fragment", "Return as a final statement in main()", false,
		"#version 310 es\n"
		"layout(location = 0) in mediump vec4 v_coords;\n"
		"layout(location = 0) out mediump vec4 o_color;\n\n"
		"void main (void)\n"
		"{\n"
		"    o_color = vec4(v_coords.xyz, 1.0);\n"
		"    return;\n"
		"}\n", evalReturnAlways, DE_NULL));

	// Return between output variable writes.
	for (int inFunc = 0; inFunc < 2; inFunc++)
	{
		for (int returnMode = 0; returnMode < RETURNMODE_LAST; returnMode++)
		{
			for (int isFragment = 0; isFragment < 2; isFragment++)
			{
				std::string						name		= std::string("output_write_") + (inFunc ? "in_func_" : "") + getReturnModeName((ReturnMode)returnMode) + (isFragment ? "_fragment" : "_vertex");
				std::string						desc		= std::string(getReturnModeDesc((ReturnMode)returnMode)) + (inFunc ? " in user-defined function" : " in main()") + " between output writes";
				de::MovePtr<ShaderReturnCase>	testCase	= (makeOutputWriteReturnCase(m_testCtx, name, desc, inFunc != 0, (ReturnMode)returnMode, isFragment == 0));
				addChild(testCase.release());
			}
		}
	}

	// Conditional return statement in loop.
	for (int isDynamicLoop = 0; isDynamicLoop < 2; isDynamicLoop++)
	{
		for (int returnMode = 0; returnMode < RETURNMODE_LAST; returnMode++)
		{
			for (int isFragment = 0; isFragment < 2; isFragment++)
			{
				std::string						name		= std::string("return_in_") + (isDynamicLoop ? "dynamic" : "static") + "_loop_" + getReturnModeName((ReturnMode)returnMode) + (isFragment ? "_fragment" : "_vertex");
				std::string						description	= std::string(getReturnModeDesc((ReturnMode)returnMode)) + " in loop";
				de::MovePtr<ShaderReturnCase>	testCase	(makeReturnInLoopCase(m_testCtx, name, description, isDynamicLoop != 0, (ReturnMode)returnMode, isFragment == 0));
				addChild(testCase.release());
			}
		}
	}

	// Unconditional return in infinite loop.
	addChild(new ShaderReturnCase(m_testCtx, "return_in_infinite_loop_vertex", "Return in infinite loop", true,
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 a_position;\n"
		"layout(location = 1) in highp vec4 a_coords;\n"
		"layout(location = 0) out mediump vec4 v_color;\n"
		"layout(binding = 0, std140) uniform something { int ui_zero; };\n"
		"highp vec4 getCoords (void)\n"
		"{\n"
		"	for (int i = 1; i < 10; i += ui_zero)\n"
		"		return a_coords;\n"
		"	return a_coords.wzyx;\n"
		"}\n\n"
		"void main (void)\n"
		"{\n"
		"    gl_Position = a_position;\n"
		"    v_color = vec4(getCoords().xyz, 1.0);\n"
		"    return;\n"
		"}\n", evalReturnAlways, new ReturnTestUniformSetup(UI_ZERO)));
	addChild(new ShaderReturnCase(m_testCtx, "return_in_infinite_loop_fragment", "Return in infinite loop", false,
		"#version 310 es\n"
		"layout(location = 0) in mediump vec4 v_coords;\n"
		"layout(location = 0) out mediump vec4 o_color;\n"
		"layout(binding = 0, std140) uniform something { int ui_zero; };\n\n"
		"mediump vec4 getCoords (void)\n"
		"{\n"
		"	for (int i = 1; i < 10; i += ui_zero)\n"
		"		return v_coords;\n"
		"	return v_coords.wzyx;\n"
		"}\n\n"
		"void main (void)\n"
		"{\n"
		"    o_color = vec4(getCoords().xyz, 1.0);\n"
		"    return;\n"
		"}\n", evalReturnAlways, new ReturnTestUniformSetup(UI_ZERO)));
}

} // anonymous

tcu::TestCaseGroup* createReturnTests (tcu::TestContext& testCtx)
{
	return new ShaderReturnTests(testCtx);
}

} // sr
} // vkt
