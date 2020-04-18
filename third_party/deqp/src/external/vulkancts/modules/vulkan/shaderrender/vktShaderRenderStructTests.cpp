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
 * \brief Shader struct tests.
 *//*--------------------------------------------------------------------*/

#include "vktShaderRenderStructTests.hpp"
#include "vktShaderRender.hpp"
#include "tcuStringTemplate.hpp"
#include "deMath.h"

namespace vkt
{
namespace sr
{
namespace
{

class ShaderStructCase : public ShaderRenderCase
{
public:
						ShaderStructCase		(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 bool				isVertexCase,
												 ShaderEvalFunc		evalFunc,
												 UniformSetupFunc	setupUniformsFunc,
												 const std::string&	vertShaderSource,
												 const std::string&	fragShaderSource);
						~ShaderStructCase		(void);

private:
						ShaderStructCase		(const ShaderStructCase&);
	ShaderStructCase&	operator=				(const ShaderStructCase&);
};

ShaderStructCase::ShaderStructCase (tcu::TestContext&	testCtx,
									const std::string&	name,
									const std::string&	description,
									bool				isVertexCase,
									ShaderEvalFunc		evalFunc,
									UniformSetupFunc	setupUniformsFunc,
									const std::string&	vertShaderSource,
									const std::string&	fragShaderSource)
	: ShaderRenderCase	(testCtx, name, description, isVertexCase, evalFunc, new UniformSetup(setupUniformsFunc), DE_NULL)
{
	m_vertShaderSource	= vertShaderSource;
	m_fragShaderSource	= fragShaderSource;
}

ShaderStructCase::~ShaderStructCase (void)
{
}

static de::MovePtr<ShaderStructCase> createStructCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, bool isVertexCase, ShaderEvalFunc evalFunc, UniformSetupFunc uniformFunc, const LineStream& shaderSrc)
{
	static std::string defaultVertSrc =
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 a_position;\n"
		"layout(location = 1) in highp vec4 a_coords;\n"
		"layout(location = 0) out mediump vec4 v_coords;\n\n"
		"void main (void)\n"
		"{\n"
		"	v_coords = a_coords;\n"
		"	gl_Position = a_position;\n"
		"}\n";
	static std::string defaultFragSrc =
		"#version 310 es\n"
		"layout(location = 0) in mediump vec4 v_color;\n"
		"layout(location = 0) out mediump vec4 o_color;\n\n"
		"void main (void)\n"
		"{\n"
		"	o_color = v_color;\n"
		"}\n";

	// Fill in specialization parameters and build the shader source.
	std::string vertSrc;
	std::string fragSrc;
	std::map<std::string, std::string> spParams;

	if (isVertexCase)
	{
		spParams["HEADER"] =
			"#version 310 es\n"
			"layout(location = 0) in highp vec4 a_position;\n"
			"layout(location = 1) in highp vec4 a_coords;\n"
			"layout(location = 0) out mediump vec4 v_color;";
		spParams["COORDS"]		= "a_coords";
		spParams["DST"]			= "v_color";
		spParams["ASSIGN_POS"]	= "gl_Position = a_position;";

		vertSrc = tcu::StringTemplate(shaderSrc.str()).specialize(spParams);
		fragSrc = defaultFragSrc;
	}
	else
	{
		spParams["HEADER"]	=
			"#version 310 es\n"
			"layout(location = 0) in mediump vec4 v_coords;\n"
			"layout(location = 0) out mediump vec4 o_color;";
		spParams["COORDS"]			= "v_coords";
		spParams["DST"]				= "o_color";
		spParams["ASSIGN_POS"]		= "";

		vertSrc = defaultVertSrc;
		fragSrc = tcu::StringTemplate(shaderSrc.str()).specialize(spParams);
	}

	return de::MovePtr<ShaderStructCase>(new ShaderStructCase(testCtx, name, description, isVertexCase, evalFunc, uniformFunc, vertSrc, fragSrc));
}

class LocalStructTests : public tcu::TestCaseGroup
{
public:
	LocalStructTests (tcu::TestContext& testCtx)
		: TestCaseGroup(testCtx, "local", "Local structs")
	{
	}

	~LocalStructTests (void)
	{
	}

	virtual void init (void);
};

void LocalStructTests::init (void)
{
	#define LOCAL_STRUCT_CASE(NAME, DESCRIPTION, SHADER_SRC, SET_UNIFORMS_BODY, EVAL_FUNC_BODY)																				\
		do {																																								\
			struct SetUniforms_##NAME { static void setUniforms (ShaderRenderCaseInstance& instance, const tcu::Vec4&) SET_UNIFORMS_BODY }; /* NOLINT(SET_UNIFORMS_BODY) */ \
			struct Eval_##NAME { static void eval (ShaderEvalContext& c) EVAL_FUNC_BODY };	/* NOLINT(EVAL_FUNC_BODY) */													\
			addChild(createStructCase(m_testCtx, #NAME "_vertex", DESCRIPTION, true, &Eval_##NAME::eval, &SetUniforms_##NAME::setUniforms, SHADER_SRC).release());			\
			addChild(createStructCase(m_testCtx, #NAME "_fragment", DESCRIPTION, false, &Eval_##NAME::eval, &SetUniforms_##NAME::setUniforms, SHADER_SRC).release());		\
		} while (deGetFalse())

	LOCAL_STRUCT_CASE(basic, "Basic struct usage",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, vec3(0.0), ui_one);"
		<< "	s.b = ${COORDS}.yzw;"
		<< "	${DST} = vec4(s.a, s.b.x, s.b.y, s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 1, 2);
		});

	LOCAL_STRUCT_CASE(nested, "Nested struct",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< ""
		<< "struct T {"
		<< "	int				a;"
		<< "	mediump vec2	b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, T(0, vec2(0.0)), ui_one);"
		<< "	s.b = T(ui_zero, ${COORDS}.yz);"
		<< "	${DST} = vec4(s.a, s.b.b, s.b.a + s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 1, 2);
		});

	LOCAL_STRUCT_CASE(array_member, "Struct with array member",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump float	b[3];"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s;"
		<< "	s.a = ${COORDS}.w;"
		<< "	s.c = ui_one;"
		<< "	s.b[0] = ${COORDS}.z;"
		<< "	s.b[1] = ${COORDS}.y;"
		<< "	s.b[2] = ${COORDS}.x;"
		<< "	${DST} = vec4(s.a, s.b[0], s.b[1], s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(3, 2, 1);
		});

	LOCAL_STRUCT_CASE(array_member_dynamic_index, "Struct with array member, dynamic indexing",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump float	b[3];"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s;"
		<< "	s.a = ${COORDS}.w;"
		<< "	s.c = ui_one;"
		<< "	s.b[0] = ${COORDS}.z;"
		<< "	s.b[1] = ${COORDS}.y;"
		<< "	s.b[2] = ${COORDS}.x;"
		<< "	${DST} = vec4(s.b[ui_one], s.b[ui_zero], s.b[ui_two], s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
		},
		{
			c.color.xyz() = c.coords.swizzle(1,2,0);
		});

	LOCAL_STRUCT_CASE(struct_array, "Struct array",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump int		b;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s[3];"
		<< "	s[0] = S(${COORDS}.x, ui_zero);"
		<< "	s[1].a = ${COORDS}.y;"
		<< "	s[1].b = ui_one;"
		<< "	s[2] = S(${COORDS}.z, ui_two);"
		<< "	${DST} = vec4(s[2].a, s[1].a, s[0].a, s[2].b - s[1].b + s[0].b);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
		},
		{
			c.color.xyz() = c.coords.swizzle(2, 1, 0);
		});

	LOCAL_STRUCT_CASE(struct_array_dynamic_index, "Struct array with dynamic indexing",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump int		b;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s[3];"
		<< "	s[0] = S(${COORDS}.x, ui_zero);"
		<< "	s[1].a = ${COORDS}.y;"
		<< "	s[1].b = ui_one;"
		<< "	s[2] = S(${COORDS}.z, ui_two);"
		<< "	${DST} = vec4(s[ui_two].a, s[ui_one].a, s[ui_zero].a, s[ui_two].b - s[ui_one].b + s[ui_zero].b);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
		},
		{
			c.color.xyz() = c.coords.swizzle(2, 1, 0);
		});

	LOCAL_STRUCT_CASE(nested_struct_array, "Nested struct array",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { mediump float uf_two; };"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { mediump float uf_three; };"
		<< "layout (std140, set = 0, binding = 5) uniform buffer5 { mediump float uf_four; };"
		<< "layout (std140, set = 0, binding = 6) uniform buffer6 { mediump float uf_half; };"
		<< "layout (std140, set = 0, binding = 7) uniform buffer7 { mediump float uf_third; };"
		<< "layout (std140, set = 0, binding = 8) uniform buffer8 { mediump float uf_fourth; };"
		<< ""
		<< "struct T {"
		<< "	mediump float	a;"
		<< "	mediump vec2	b[2];"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b[3];"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s[2];"
		<< ""
		<< "	// S[0]"
		<< "	s[0].a         = ${COORDS}.x;"
		<< "	s[0].b[0].a    = uf_half;"
		<< "	s[0].b[0].b[0] = ${COORDS}.xy;"
		<< "	s[0].b[0].b[1] = ${COORDS}.zw;"
		<< "	s[0].b[1].a    = uf_third;"
		<< "	s[0].b[1].b[0] = ${COORDS}.zw;"
		<< "	s[0].b[1].b[1] = ${COORDS}.xy;"
		<< "	s[0].b[2].a    = uf_fourth;"
		<< "	s[0].b[2].b[0] = ${COORDS}.xz;"
		<< "	s[0].b[2].b[1] = ${COORDS}.yw;"
		<< "	s[0].c         = ui_zero;"
		<< ""
		<< "	// S[1]"
		<< "	s[1].a         = ${COORDS}.w;"
		<< "	s[1].b[0].a    = uf_two;"
		<< "	s[1].b[0].b[0] = ${COORDS}.xx;"
		<< "	s[1].b[0].b[1] = ${COORDS}.yy;"
		<< "	s[1].b[1].a    = uf_three;"
		<< "	s[1].b[1].b[0] = ${COORDS}.zz;"
		<< "	s[1].b[1].b[1] = ${COORDS}.ww;"
		<< "	s[1].b[2].a    = uf_four;"
		<< "	s[1].b[2].b[0] = ${COORDS}.yx;"
		<< "	s[1].b[2].b[1] = ${COORDS}.wz;"
		<< "	s[1].c         = ui_one;"
		<< ""
		<< "	mediump float r = (s[0].b[1].b[0].x + s[1].b[2].b[1].y) * s[0].b[0].a; // (z + z) * 0.5"
		<< "	mediump float g = s[1].b[0].b[0].y * s[0].b[2].a * s[1].b[2].a; // x * 0.25 * 4"
		<< "	mediump float b = (s[0].b[2].b[1].y + s[0].b[1].b[0].y + s[1].a) * s[0].b[1].a; // (w + w + w) * 0.333"
		<< "	mediump float a = float(s[0].c) + s[1].b[2].a - s[1].b[1].a; // 0 + 4.0 - 3.0"
		<< "	${DST} = vec4(r, g, b, a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.useUniform(3u, UF_TWO);
			instance.useUniform(4u, UF_THREE);
			instance.useUniform(5u, UF_FOUR);
			instance.useUniform(6u, UF_HALF);
			instance.useUniform(7u, UF_THIRD);
			instance.useUniform(8u, UF_FOURTH);
		},
		{
			c.color.xyz() = c.coords.swizzle(2, 0, 3);
		});

	LOCAL_STRUCT_CASE(nested_struct_array_dynamic_index, "Nested struct array with dynamic indexing",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { mediump float uf_two; };"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { mediump float uf_three; };"
		<< "layout (std140, set = 0, binding = 5) uniform buffer5 { mediump float uf_four; };"
		<< "layout (std140, set = 0, binding = 6) uniform buffer6 { mediump float uf_half; };"
		<< "layout (std140, set = 0, binding = 7) uniform buffer7 { mediump float uf_third; };"
		<< "layout (std140, set = 0, binding = 8) uniform buffer8 { mediump float uf_fourth; };"
		<< ""
		<< "struct T {"
		<< "	mediump float	a;"
		<< "	mediump vec2	b[2];"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b[3];"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s[2];"
		<< ""
		<< "	// S[0]"
		<< "	s[0].a         = ${COORDS}.x;"
		<< "	s[0].b[0].a    = uf_half;"
		<< "	s[0].b[0].b[0] = ${COORDS}.xy;"
		<< "	s[0].b[0].b[1] = ${COORDS}.zw;"
		<< "	s[0].b[1].a    = uf_third;"
		<< "	s[0].b[1].b[0] = ${COORDS}.zw;"
		<< "	s[0].b[1].b[1] = ${COORDS}.xy;"
		<< "	s[0].b[2].a    = uf_fourth;"
		<< "	s[0].b[2].b[0] = ${COORDS}.xz;"
		<< "	s[0].b[2].b[1] = ${COORDS}.yw;"
		<< "	s[0].c         = ui_zero;"
		<< ""
		<< "	// S[1]"
		<< "	s[1].a         = ${COORDS}.w;"
		<< "	s[1].b[0].a    = uf_two;"
		<< "	s[1].b[0].b[0] = ${COORDS}.xx;"
		<< "	s[1].b[0].b[1] = ${COORDS}.yy;"
		<< "	s[1].b[1].a    = uf_three;"
		<< "	s[1].b[1].b[0] = ${COORDS}.zz;"
		<< "	s[1].b[1].b[1] = ${COORDS}.ww;"
		<< "	s[1].b[2].a    = uf_four;"
		<< "	s[1].b[2].b[0] = ${COORDS}.yx;"
		<< "	s[1].b[2].b[1] = ${COORDS}.wz;"
		<< "	s[1].c         = ui_one;"
		<< ""
		<< "	mediump float r = (s[0].b[ui_one].b[ui_one-1].x + s[ui_one].b[ui_two].b[ui_zero+1].y) * s[0].b[0].a; // (z + z) * 0.5"
		<< "	mediump float g = s[ui_two-1].b[ui_two-2].b[ui_zero].y * s[0].b[ui_two].a * s[ui_one].b[2].a; // x * 0.25 * 4"
		<< "	mediump float b = (s[ui_zero].b[ui_one+1].b[1].y + s[0].b[ui_one*ui_one].b[0].y + s[ui_one].a) * s[0].b[ui_two-ui_one].a; // (w + w + w) * 0.333"
		<< "	mediump float a = float(s[ui_zero].c) + s[ui_one-ui_zero].b[ui_two].a - s[ui_zero+ui_one].b[ui_two-ui_one].a; // 0 + 4.0 - 3.0"
		<< "	${DST} = vec4(r, g, b, a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.useUniform(3u, UF_TWO);
			instance.useUniform(4u, UF_THREE);
			instance.useUniform(5u, UF_FOUR);
			instance.useUniform(6u, UF_HALF);
			instance.useUniform(7u, UF_THIRD);
			instance.useUniform(8u, UF_FOURTH);
		},
		{
			c.color.xyz() = c.coords.swizzle(2, 0, 3);
		});

	LOCAL_STRUCT_CASE(parameter, "Struct as a function parameter",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "mediump vec4 myFunc (S s)"
		<< "{"
		<< "	return vec4(s.a, s.b.x, s.b.y, s.c);"
		<< "}"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, vec3(0.0), ui_one);"
		<< "	s.b = ${COORDS}.yzw;"
		<< "	${DST} = myFunc(s);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 1, 2);
		});

	LOCAL_STRUCT_CASE(parameter_nested, "Nested struct as a function parameter",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< ""
		<< "struct T {"
		<< "	int				a;"
		<< "	mediump vec2	b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "mediump vec4 myFunc (S s)"
		<< "{"
		<< "	return vec4(s.a, s.b.b, s.b.a + s.c);"
		<< "}"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, T(0, vec2(0.0)), ui_one);"
		<< "	s.b = T(ui_zero, ${COORDS}.yz);"
		<< "	${DST} = myFunc(s);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 1, 2);
		});

	LOCAL_STRUCT_CASE(return, "Struct as a return value",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "S myFunc (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, vec3(0.0), ui_one);"
		<< "	s.b = ${COORDS}.yzw;"
		<< "	return s;"
		<< "}"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = myFunc();"
		<< "	${DST} = vec4(s.a, s.b.x, s.b.y, s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 1, 2);
		});

	LOCAL_STRUCT_CASE(return_nested, "Nested struct",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< ""
		<< "struct T {"
		<< "	int				a;"
		<< "	mediump vec2	b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "S myFunc (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, T(0, vec2(0.0)), ui_one);"
		<< "	s.b = T(ui_zero, ${COORDS}.yz);"
		<< "	return s;"
		<< "}"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = myFunc();"
		<< "	${DST} = vec4(s.a, s.b.b, s.b.a + s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 1, 2);
		});

	LOCAL_STRUCT_CASE(conditional_assignment, "Conditional struct assignment",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { mediump float uf_one; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, ${COORDS}.yzw, ui_zero);"
		<< "	if (uf_one > 0.0)"
		<< "		s = S(${COORDS}.w, ${COORDS}.zyx, ui_one);"
		<< "	${DST} = vec4(s.a, s.b.xy, s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UF_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(3, 2, 1);
		});

	LOCAL_STRUCT_CASE(loop_assignment, "Struct assignment in loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, ${COORDS}.yzw, ui_zero);"
		<< "	for (int i = 0; i < 3; i++)"
		<< "	{"
		<< "		if (i == 1)"
		<< "			s = S(${COORDS}.w, ${COORDS}.zyx, ui_one);"
		<< "	}"
		<< "	${DST} = vec4(s.a, s.b.xy, s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(3, 2, 1);
		});

	LOCAL_STRUCT_CASE(dynamic_loop_assignment, "Struct assignment in loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_three; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, ${COORDS}.yzw, ui_zero);"
		<< "	for (int i = 0; i < ui_three; i++)"
		<< "	{"
		<< "		if (i == ui_one)"
		<< "			s = S(${COORDS}.w, ${COORDS}.zyx, ui_one);"
		<< "	}"
		<< "	${DST} = vec4(s.a, s.b.xy, s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_THREE);
		},
		{
			c.color.xyz() = c.coords.swizzle(3, 2, 1);
		});

	LOCAL_STRUCT_CASE(nested_conditional_assignment, "Conditional assignment of nested struct",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { mediump float uf_one; };"
		<< ""
		<< "struct T {"
		<< "	int				a;"
		<< "	mediump vec2	b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, T(ui_one, ${COORDS}.yz), ui_one);"
		<< "	if (uf_one > 0.0)"
		<< "		s.b = T(ui_zero, ${COORDS}.zw);"
		<< "	${DST} = vec4(s.a, s.b.b, s.c - s.b.a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UF_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 2, 3);
		});

	LOCAL_STRUCT_CASE(nested_loop_assignment, "Nested struct assignment in loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { mediump float uf_one; };"
		<< ""
		<< "struct T {"
		<< "	int				a;"
		<< "	mediump vec2	b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, T(ui_one, ${COORDS}.yz), ui_one);"
		<< "	for (int i = 0; i < 3; i++)"
		<< "	{"
		<< "		if (i == 1)"
		<< "			s.b = T(ui_zero, ${COORDS}.zw);"
		<< "	}"
		<< "	${DST} = vec4(s.a, s.b.b, s.c - s.b.a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UF_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 2, 3);
		});

	LOCAL_STRUCT_CASE(nested_dynamic_loop_assignment, "Nested struct assignment in dynamic loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_three; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { mediump float uf_one; };"
		<< ""
		<< "struct T {"
		<< "	int				a;"
		<< "	mediump vec2	b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s = S(${COORDS}.x, T(ui_one, ${COORDS}.yz), ui_one);"
		<< "	for (int i = 0; i < ui_three; i++)"
		<< "	{"
		<< "		if (i == ui_one)"
		<< "			s.b = T(ui_zero, ${COORDS}.zw);"
		<< "	}"
		<< "	${DST} = vec4(s.a, s.b.b, s.c - s.b.a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_THREE);
			instance.useUniform(3u, UF_ONE);
		},
		{
			c.color.xyz() = c.coords.swizzle(0, 2, 3);
		});

	LOCAL_STRUCT_CASE(loop_struct_array, "Struct array usage in loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump int		b;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s[3];"
		<< "	s[0] = S(${COORDS}.x, ui_zero);"
		<< "	s[1].a = ${COORDS}.y;"
		<< "	s[1].b = -ui_one;"
		<< "	s[2] = S(${COORDS}.z, ui_two);"
		<< ""
		<< "	mediump float rgb[3];"
		<< "	int alpha = 0;"
		<< "	for (int i = 0; i < 3; i++)"
		<< "	{"
		<< "		rgb[i] = s[2-i].a;"
		<< "		alpha += s[i].b;"
		<< "	}"
		<< "	${DST} = vec4(rgb[0], rgb[1], rgb[2], alpha);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
		},
		{
			c.color.xyz() = c.coords.swizzle(2, 1, 0);
		});

	LOCAL_STRUCT_CASE(loop_nested_struct_array, "Nested struct array usage in loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { mediump float uf_two; };"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { mediump float uf_three; };"
		<< "layout (std140, set = 0, binding = 5) uniform buffer5 { mediump float uf_four; };"
		<< "layout (std140, set = 0, binding = 6) uniform buffer6 { mediump float uf_half; };"
		<< "layout (std140, set = 0, binding = 7) uniform buffer7 { mediump float uf_third; };"
		<< "layout (std140, set = 0, binding = 8) uniform buffer8 { mediump float uf_fourth; };"
		<< "layout (std140, set = 0, binding = 9) uniform buffer9 { mediump float uf_sixth; };"
		<< ""
		<< "struct T {"
		<< "	mediump float	a;"
		<< "	mediump vec2	b[2];"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b[3];"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s[2];"
		<< ""
		<< "	// S[0]"
		<< "	s[0].a         = ${COORDS}.x;"
		<< "	s[0].b[0].a    = uf_half;"
		<< "	s[0].b[0].b[0] = ${COORDS}.yx;"
		<< "	s[0].b[0].b[1] = ${COORDS}.zx;"
		<< "	s[0].b[1].a    = uf_third;"
		<< "	s[0].b[1].b[0] = ${COORDS}.yy;"
		<< "	s[0].b[1].b[1] = ${COORDS}.wy;"
		<< "	s[0].b[2].a    = uf_fourth;"
		<< "	s[0].b[2].b[0] = ${COORDS}.zx;"
		<< "	s[0].b[2].b[1] = ${COORDS}.zy;"
		<< "	s[0].c         = ui_zero;"
		<< ""
		<< "	// S[1]"
		<< "	s[1].a         = ${COORDS}.w;"
		<< "	s[1].b[0].a    = uf_two;"
		<< "	s[1].b[0].b[0] = ${COORDS}.zx;"
		<< "	s[1].b[0].b[1] = ${COORDS}.zy;"
		<< "	s[1].b[1].a    = uf_three;"
		<< "	s[1].b[1].b[0] = ${COORDS}.zz;"
		<< "	s[1].b[1].b[1] = ${COORDS}.ww;"
		<< "	s[1].b[2].a    = uf_four;"
		<< "	s[1].b[2].b[0] = ${COORDS}.yx;"
		<< "	s[1].b[2].b[1] = ${COORDS}.wz;"
		<< "	s[1].c         = ui_one;"
		<< ""
		<< "	mediump float r = 0.0; // (x*3 + y*3) / 6.0"
		<< "	mediump float g = 0.0; // (y*3 + z*3) / 6.0"
		<< "	mediump float b = 0.0; // (z*3 + w*3) / 6.0"
		<< "	mediump float a = 1.0;"
		<< "	for (int i = 0; i < 2; i++)"
		<< "	{"
		<< "		for (int j = 0; j < 3; j++)"
		<< "		{"
		<< "			r += s[0].b[j].b[i].y;"
		<< "			g += s[i].b[j].b[0].x;"
		<< "			b += s[i].b[j].b[1].x;"
		<< "			a *= s[i].b[j].a;"
		<< "		}"
		<< "	}"
		<< "	${DST} = vec4(r*uf_sixth, g*uf_sixth, b*uf_sixth, a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.useUniform(3u, UF_TWO);
			instance.useUniform(4u, UF_THREE);
			instance.useUniform(5u, UF_FOUR);
			instance.useUniform(6u, UF_HALF);
			instance.useUniform(7u, UF_THIRD);
			instance.useUniform(8u, UF_FOURTH);
			instance.useUniform(9u, UF_SIXTH);
		},
		{
			c.color.xyz() = (c.coords.swizzle(0, 1, 2) + c.coords.swizzle(1, 2, 3)) * 0.5f;
		});

	LOCAL_STRUCT_CASE(dynamic_loop_struct_array, "Struct array usage in dynamic loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { int ui_three; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump int		b;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s[3];"
		<< "	s[0] = S(${COORDS}.x, ui_zero);"
		<< "	s[1].a = ${COORDS}.y;"
		<< "	s[1].b = -ui_one;"
		<< "	s[2] = S(${COORDS}.z, ui_two);"
		<< ""
		<< "	mediump float rgb[3];"
		<< "	int alpha = 0;"
		<< "	for (int i = 0; i < ui_three; i++)"
		<< "	{"
		<< "		rgb[i] = s[2-i].a;"
		<< "		alpha += s[i].b;"
		<< "	}"
		<< "	${DST} = vec4(rgb[0], rgb[1], rgb[2], alpha);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.useUniform(3u, UI_THREE);
		},
		{
			c.color.xyz() = c.coords.swizzle(2, 1, 0);
		});

	LOCAL_STRUCT_CASE(dynamic_loop_nested_struct_array, "Nested struct array usage in dynamic loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { int ui_three; };"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { mediump float uf_two; };"
		<< "layout (std140, set = 0, binding = 5) uniform buffer5 { mediump float uf_three; };"
		<< "layout (std140, set = 0, binding = 6) uniform buffer6 { mediump float uf_four; };"
		<< "layout (std140, set = 0, binding = 7) uniform buffer7 { mediump float uf_half; };"
		<< "layout (std140, set = 0, binding = 8) uniform buffer8 { mediump float uf_third; };"
		<< "layout (std140, set = 0, binding = 9) uniform buffer9 { mediump float uf_fourth; };"
		<< "layout (std140, set = 0, binding = 10) uniform buffer10 { mediump float uf_sixth; };"
		<< ""
		<< "struct T {"
		<< "	mediump float	a;"
		<< "	mediump vec2	b[2];"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b[3];"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S s[2];"
		<< ""
		<< "	s[0].a         = ${COORDS}.x;"
		<< "	s[0].b[0].a    = uf_half;"
		<< "	s[0].b[0].b[0] = ${COORDS}.yx;"
		<< "	s[0].b[0].b[1] = ${COORDS}.zx;"
		<< "	s[0].b[1].a    = uf_third;"
		<< "	s[0].b[1].b[0] = ${COORDS}.yy;"
		<< "	s[0].b[1].b[1] = ${COORDS}.wy;"
		<< "	s[0].b[2].a    = uf_fourth;"
		<< "	s[0].b[2].b[0] = ${COORDS}.zx;"
		<< "	s[0].b[2].b[1] = ${COORDS}.zy;"
		<< "	s[0].c         = ui_zero;"
		<< ""
		<< "	s[1].a         = ${COORDS}.w;"
		<< "	s[1].b[0].a    = uf_two;"
		<< "	s[1].b[0].b[0] = ${COORDS}.zx;"
		<< "	s[1].b[0].b[1] = ${COORDS}.zy;"
		<< "	s[1].b[1].a    = uf_three;"
		<< "	s[1].b[1].b[0] = ${COORDS}.zz;"
		<< "	s[1].b[1].b[1] = ${COORDS}.ww;"
		<< "	s[1].b[2].a    = uf_four;"
		<< "	s[1].b[2].b[0] = ${COORDS}.yx;"
		<< "	s[1].b[2].b[1] = ${COORDS}.wz;"
		<< "	s[1].c         = ui_one;"
		<< ""
		<< "	mediump float r = 0.0; // (x*3 + y*3) / 6.0"
		<< "	mediump float g = 0.0; // (y*3 + z*3) / 6.0"
		<< "	mediump float b = 0.0; // (z*3 + w*3) / 6.0"
		<< "	mediump float a = 1.0;"
		<< "	for (int i = 0; i < ui_two; i++)"
		<< "	{"
		<< "		for (int j = 0; j < ui_three; j++)"
		<< "		{"
		<< "			r += s[0].b[j].b[i].y;"
		<< "			g += s[i].b[j].b[0].x;"
		<< "			b += s[i].b[j].b[1].x;"
		<< "			a *= s[i].b[j].a;"
		<< "		}"
		<< "	}"
		<< "	${DST} = vec4(r*uf_sixth, g*uf_sixth, b*uf_sixth, a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.useUniform(3u, UI_THREE);
			instance.useUniform(4u, UF_TWO);
			instance.useUniform(5u, UF_THREE);
			instance.useUniform(6u, UF_FOUR);
			instance.useUniform(7u, UF_HALF);
			instance.useUniform(8u, UF_THIRD);
			instance.useUniform(9u, UF_FOURTH);
			instance.useUniform(10u, UF_SIXTH);
		},
		{
			c.color.xyz() = (c.coords.swizzle(0, 1, 2) + c.coords.swizzle(1, 2, 3)) * 0.5f;
		});

	LOCAL_STRUCT_CASE(basic_equal, "Basic struct equality",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S a = S(floor(${COORDS}.x), vec3(0.0, floor(${COORDS}.y), 2.3), ui_one);"
		<< "	S b = S(floor(${COORDS}.x+0.5), vec3(0.0, floor(${COORDS}.y), 2.3), ui_one);"
		<< "	S c = S(floor(${COORDS}.x), vec3(0.0, floor(${COORDS}.y+0.5), 2.3), ui_one);"
		<< "	S d = S(floor(${COORDS}.x), vec3(0.0, floor(${COORDS}.y), 2.3), ui_two);"
		<< "	${DST} = vec4(0.0, 0.0, 0.0, 1.0);"
		<< "	if (a == b) ${DST}.x = 1.0;"
		<< "	if (a == c) ${DST}.y = 1.0;"
		<< "	if (a == d) ${DST}.z = 1.0;"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);
			instance.useUniform(1u, UI_TWO);
		},
		{
			if (deFloatFloor(c.coords[0]) == deFloatFloor(c.coords[0] + 0.5f))
				c.color.x() = 1.0f;
			if (deFloatFloor(c.coords[1]) == deFloatFloor(c.coords[1] + 0.5f))
				c.color.y() = 1.0f;
		});

	LOCAL_STRUCT_CASE(basic_not_equal, "Basic struct equality",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S a = S(floor(${COORDS}.x), vec3(0.0, floor(${COORDS}.y), 2.3), ui_one);"
		<< "	S b = S(floor(${COORDS}.x+0.5), vec3(0.0, floor(${COORDS}.y), 2.3), ui_one);"
		<< "	S c = S(floor(${COORDS}.x), vec3(0.0, floor(${COORDS}.y+0.5), 2.3), ui_one);"
		<< "	S d = S(floor(${COORDS}.x), vec3(0.0, floor(${COORDS}.y), 2.3), ui_two);"
		<< "	${DST} = vec4(0.0, 0.0, 0.0, 1.0);"
		<< "	if (a != b) ${DST}.x = 1.0;"
		<< "	if (a != c) ${DST}.y = 1.0;"
		<< "	if (a != d) ${DST}.z = 1.0;"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);
			instance.useUniform(1u, UI_TWO);
		},
		{
			if (deFloatFloor(c.coords[0]) != deFloatFloor(c.coords[0] + 0.5f))
				c.color.x() = 1.0f;
			if (deFloatFloor(c.coords[1]) != deFloatFloor(c.coords[1] + 0.5f))
				c.color.y() = 1.0f;
			c.color.z() = 1.0f;
		});

	LOCAL_STRUCT_CASE(nested_equal, "Nested struct struct equality",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_two; };"
		<< ""
		<< "struct T {"
		<< "	mediump vec3	a;"
		<< "	int				b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S a = S(floor(${COORDS}.x), T(vec3(0.0, floor(${COORDS}.y), 2.3), ui_one), 1);"
		<< "	S b = S(floor(${COORDS}.x+0.5), T(vec3(0.0, floor(${COORDS}.y), 2.3), ui_one), 1);"
		<< "	S c = S(floor(${COORDS}.x), T(vec3(0.0, floor(${COORDS}.y+0.5), 2.3), ui_one), 1);"
		<< "	S d = S(floor(${COORDS}.x), T(vec3(0.0, floor(${COORDS}.y), 2.3), ui_two), 1);"
		<< "	${DST} = vec4(0.0, 0.0, 0.0, 1.0);"
		<< "	if (a == b) ${DST}.x = 1.0;"
		<< "	if (a == c) ${DST}.y = 1.0;"
		<< "	if (a == d) ${DST}.z = 1.0;"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);
			instance.useUniform(1u, UI_TWO);
		},
		{
			if (deFloatFloor(c.coords[0]) == deFloatFloor(c.coords[0] + 0.5f))
				c.color.x() = 1.0f;
			if (deFloatFloor(c.coords[1]) == deFloatFloor(c.coords[1] + 0.5f))
				c.color.y() = 1.0f;
		});

	LOCAL_STRUCT_CASE(nested_not_equal, "Nested struct struct equality",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_two; };"
		<< ""
		<< "struct T {"
		<< "	mediump vec3	a;"
		<< "	int				b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S a = S(floor(${COORDS}.x), T(vec3(0.0, floor(${COORDS}.y), 2.3), ui_one), 1);"
		<< "	S b = S(floor(${COORDS}.x+0.5), T(vec3(0.0, floor(${COORDS}.y), 2.3), ui_one), 1);"
		<< "	S c = S(floor(${COORDS}.x), T(vec3(0.0, floor(${COORDS}.y+0.5), 2.3), ui_one), 1);"
		<< "	S d = S(floor(${COORDS}.x), T(vec3(0.0, floor(${COORDS}.y), 2.3), ui_two), 1);"
		<< "	${DST} = vec4(0.0, 0.0, 0.0, 1.0);"
		<< "	if (a != b) ${DST}.x = 1.0;"
		<< "	if (a != c) ${DST}.y = 1.0;"
		<< "	if (a != d) ${DST}.z = 1.0;"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);
			instance.useUniform(1u, UI_TWO);
		},
		{
			if (deFloatFloor(c.coords[0]) != deFloatFloor(c.coords[0] + 0.5f))
				c.color.x() = 1.0f;
			if (deFloatFloor(c.coords[1]) != deFloatFloor(c.coords[1] + 0.5f))
				c.color.y() = 1.0f;
			c.color.z() = 1.0f;
		});
}

class UniformStructTests : public tcu::TestCaseGroup
{
public:
	UniformStructTests (tcu::TestContext& testCtx)
		: TestCaseGroup(testCtx, "uniform", "Uniform structs")
	{
	}

	~UniformStructTests (void)
	{
	}

	virtual void init (void);
};

void UniformStructTests::init (void)
{
	#define UNIFORM_STRUCT_CASE(NAME, DESCRIPTION, SHADER_SRC, SET_UNIFORMS_BODY, EVAL_FUNC_BODY)																\
		do {																																							\
			struct SetUniforms_##NAME {																																	\
				 static void setUniforms (ShaderRenderCaseInstance& instance, const tcu::Vec4& constCoords) SET_UNIFORMS_BODY  /* NOLINT(SET_UNIFORMS_BODY) */			\
			};																																							\
			struct Eval_##NAME { static void eval (ShaderEvalContext& c) EVAL_FUNC_BODY };  /* NOLINT(EVAL_FUNC_BODY) */												\
			addChild(createStructCase(m_testCtx, #NAME "_vertex", DESCRIPTION, true, Eval_##NAME::eval, SetUniforms_##NAME::setUniforms, SHADER_SRC).release());		\
			addChild(createStructCase(m_testCtx, #NAME "_fragment", DESCRIPTION, false, Eval_##NAME::eval, SetUniforms_##NAME::setUniforms, SHADER_SRC).release());		\
		} while (deGetFalse())

	UNIFORM_STRUCT_CASE(basic, "Basic struct usage",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { S s; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	${DST} = vec4(s.a, s.b.x, s.b.y, s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);

			struct S {
				float			a;
				float			_padding1[3];
				tcu::Vec3		b;
				int				c;
			};

			S s;
			s.a = constCoords.x();
			s.b = constCoords.swizzle(1, 2, 3);
			s.c = 1;
			instance.addUniform(1u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(S), &s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(0, 1, 2);
		});

	UNIFORM_STRUCT_CASE(nested, "Nested struct",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< ""
		<< "struct T {"
		<< "	int				a;"
		<< "	mediump vec2	b;"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b;"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { S s; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	${DST} = vec4(s.a, s.b.b, s.b.a + s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);

			struct T {
				int				a;
				float			_padding1[1];
				tcu::Vec2		b;
			};

			struct S {
				float			a;
				float			_padding1[3];
				T				b;
				int				c;
				float			_padding2[3];
			};

			S s;
			s.a = constCoords.x();
			s.b.a = 0;
			s.b.b = constCoords.swizzle(1, 2);
			s.c = 1;
			instance.addUniform(2u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,sizeof(S), &s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(0, 1, 2);
		});

	UNIFORM_STRUCT_CASE(array_member, "Struct with array member",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_one; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump float	b[3];"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { S s; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	${DST} = vec4(s.a, s.b[0], s.b[1], s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ONE);

			struct paddedFloat {
				float value;
				float _padding[3];
			};

			struct S {
				paddedFloat	a;
				paddedFloat	b[3];
				int			c;
			};

			S s;
			s.a.value = constCoords.w();
			s.b[0].value = constCoords.z();
			s.b[1].value = constCoords.y();
			s.b[2].value = constCoords.x();
			s.c = 1;
			instance.addUniform(1u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,sizeof(S), &s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(3, 2, 1);
		});

	UNIFORM_STRUCT_CASE(array_member_dynamic_index, "Struct with array member, dynamic indexing",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump float	b[3];"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { S s; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	${DST} = vec4(s.b[ui_one], s.b[ui_zero], s.b[ui_two], s.c);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);

			struct paddedFloat {
				float value;
				float _padding[3];
			};

			struct S {
				paddedFloat	a;
				paddedFloat	b[3];
				int			c;
			};

			S s;
			s.a.value = constCoords.w();
			s.b[0].value = constCoords.z();
			s.b[1].value = constCoords.y();
			s.b[2].value = constCoords.x();
			s.c = 1;
			instance.addUniform(3u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(S), &s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(1, 2, 0);
		});

	UNIFORM_STRUCT_CASE(struct_array, "Struct array",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump int		b;"
		<< "};"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { S s[3]; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	${DST} = vec4(s[2].a, s[1].a, s[0].a, s[2].b - s[1].b + s[0].b);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);

			struct S {
				float	a;
				int		b;
				float	_padding1[2];
			};

			S s[3];
			s[0].a = constCoords.x();
			s[0].b = 0;
			s[1].a = constCoords.y();
			s[1].b = 1;
			s[2].a = constCoords.z();
			s[2].b = 2;
			instance.addUniform(3u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 * sizeof(S), s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(2, 1, 0);
		});

	UNIFORM_STRUCT_CASE(struct_array_dynamic_index, "Struct array with dynamic indexing",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump int		b;"
		<< "};"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { S s[3]; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	${DST} = vec4(s[ui_two].a, s[ui_one].a, s[ui_zero].a, s[ui_two].b - s[ui_one].b + s[ui_zero].b);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);

			struct S {
				float	a;
				int		b;
				float	_padding1[2];
			};

			S s[3];
			s[0].a = constCoords.x();
			s[0].b = 0;
			s[1].a = constCoords.y();
			s[1].b = 1;
			s[2].a = constCoords.z();
			s[2].b = 2;
			instance.addUniform(3u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 * sizeof(S), s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(2, 1, 0);
		});

	UNIFORM_STRUCT_CASE(nested_struct_array, "Nested struct array",
		LineStream()
		<< "${HEADER}"
		<< "struct T {"
		<< "	mediump float	a;"
		<< "	mediump vec2	b[2];"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b[3];"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { S s[2]; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	mediump float r = (s[0].b[1].b[0].x + s[1].b[2].b[1].y) * s[0].b[0].a; // (z + z) * 0.5"
		<< "	mediump float g = s[1].b[0].b[0].y * s[0].b[2].a * s[1].b[2].a; // x * 0.25 * 4"
		<< "	mediump float b = (s[0].b[2].b[1].y + s[0].b[1].b[0].y + s[1].a) * s[0].b[1].a; // (w + w + w) * 0.333"
		<< "	mediump float a = float(s[0].c) + s[1].b[2].a - s[1].b[1].a; // 0 + 4.0 - 3.0"
		<< "	${DST} = vec4(r, g, b, a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{

			struct T {
				float		a;
				float		_padding1[3];
				tcu::Vec4	b[2];
			};

			struct S {
				float	a;
				float	_padding1[3];
				T		b[3];
				int		c;
				float	_padding2[3];
			};

			S s[2];
			s[0].a = constCoords.x();
			s[0].b[0].a = 0.5f;
			s[0].b[0].b[0] = constCoords.swizzle(0,1,0,0);
			s[0].b[0].b[1] = constCoords.swizzle(2,3,0,0);
			s[0].b[1].a = 1.0f / 3.0f;
			s[0].b[1].b[0] = constCoords.swizzle(2,3,0,0);
			s[0].b[1].b[1] = constCoords.swizzle(0,1,0,0);
			s[0].b[2].a = 1.0f / 4.0f;
			s[0].b[2].b[0] = constCoords.swizzle(0,2,0,0);
			s[0].b[2].b[1] = constCoords.swizzle(1,3,0,0);
			s[0].c = 0;

			s[1].a = constCoords.w();
			s[1].b[0].a = 2.0f;
			s[1].b[0].b[0] = constCoords.swizzle(0,0,0,0);
			s[1].b[0].b[1] = constCoords.swizzle(1,1,0,0);
			s[1].b[1].a = 3.0f;
			s[1].b[1].b[0] = constCoords.swizzle(2,2,0,0);
			s[1].b[1].b[1] = constCoords.swizzle(3,3,0,0);
			s[1].b[2].a = 4.0f;
			s[1].b[2].b[0] = constCoords.swizzle(1,0,0,0);
			s[1].b[2].b[1] = constCoords.swizzle(3,2,0,0);
			s[1].c = 1;

			instance.addUniform(0u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * sizeof(S), s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(2, 0, 3);
		});

	UNIFORM_STRUCT_CASE(nested_struct_array_dynamic_index, "Nested struct array with dynamic indexing",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct T {"
		<< "	mediump float	a;"
		<< "	mediump vec2	b[2];"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b[3];"
		<< "	int				c;"
		<< "};"
		<< "layout (set = 0, binding = 3) uniform buffer3 { S s[2]; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	mediump float r = (s[0].b[ui_one].b[ui_one-1].x + s[ui_one].b[ui_two].b[ui_zero+1].y) * s[0].b[0].a; // (z + z) * 0.5"
		<< "	mediump float g = s[ui_two-1].b[ui_two-2].b[ui_zero].y * s[0].b[ui_two].a * s[ui_one].b[2].a; // x * 0.25 * 4"
		<< "	mediump float b = (s[ui_zero].b[ui_one+1].b[1].y + s[0].b[ui_one*ui_one].b[0].y + s[ui_one].a) * s[0].b[ui_two-ui_one].a; // (w + w + w) * 0.333"
		<< "	mediump float a = float(s[ui_zero].c) + s[ui_one-ui_zero].b[ui_two].a - s[ui_zero+ui_one].b[ui_two-ui_one].a; // 0 + 4.0 - 3.0"
		<< "	${DST} = vec4(r, g, b, a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			struct T {
				float		a;
				float		_padding1[3];
				tcu::Vec4	b[2];
			};

			struct S {
				float	a;
				float	_padding1[3];
				T		b[3];
				int		c;
				float	_padding2[3];
			};

			S s[2];
			s[0].a = constCoords.x();
			s[0].b[0].a = 0.5f;
			s[0].b[0].b[0] = constCoords.swizzle(0,1,0,0);
			s[0].b[0].b[1] = constCoords.swizzle(2,3,0,0);
			s[0].b[1].a = 1.0f / 3.0f;
			s[0].b[1].b[0] = constCoords.swizzle(2,3,0,0);
			s[0].b[1].b[1] = constCoords.swizzle(0,1,0,0);
			s[0].b[2].a = 1.0f / 4.0f;
			s[0].b[2].b[0] = constCoords.swizzle(0,2,0,0);
			s[0].b[2].b[1] = constCoords.swizzle(1,3,0,0);
			s[0].c = 0;

			s[1].a = constCoords.w();
			s[1].b[0].a = 2.0f;
			s[1].b[0].b[0] = constCoords.swizzle(0,0,0,0);
			s[1].b[0].b[1] = constCoords.swizzle(1,1,0,0);
			s[1].b[1].a = 3.0f;
			s[1].b[1].b[0] = constCoords.swizzle(2,2,0,0);
			s[1].b[1].b[1] = constCoords.swizzle(3,3,0,0);
			s[1].b[2].a = 4.0f;
			s[1].b[2].b[0] = constCoords.swizzle(1,0,0,0);
			s[1].b[2].b[1] = constCoords.swizzle(3,2,0,0);
			s[1].c = 1;

			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.addUniform(3u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * sizeof(S), s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(2, 0, 3);
		});
	UNIFORM_STRUCT_CASE(loop_struct_array, "Struct array usage in loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump int		b;"
		<< "};"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { S s[3]; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	mediump float rgb[3];"
		<< "	int alpha = 0;"
		<< "	for (int i = 0; i < 3; i++)"
		<< "	{"
		<< "		rgb[i] = s[2-i].a;"
		<< "		alpha += s[i].b;"
		<< "	}"
		<< "	${DST} = vec4(rgb[0], rgb[1], rgb[2], alpha);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);

			struct S {
				float	a;
				int		b;
				float	_padding1[2];
			};

			S s[3];
			s[0].a = constCoords.x();
			s[0].b = 0;
			s[1].a = constCoords.y();
			s[1].b = -1;
			s[2].a = constCoords.z();
			s[2].b = 2;
			instance.addUniform(3u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3u * sizeof(S), s);
		},
		{
			c.color.xyz() = c.constCoords.swizzle(2, 1, 0);
		});

	UNIFORM_STRUCT_CASE(loop_nested_struct_array, "Nested struct array usage in loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { mediump float uf_two; };"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { mediump float uf_three; };"
		<< "layout (std140, set = 0, binding = 5) uniform buffer5 { mediump float uf_four; };"
		<< "layout (std140, set = 0, binding = 6) uniform buffer6 { mediump float uf_half; };"
		<< "layout (std140, set = 0, binding = 7) uniform buffer7 { mediump float uf_third; };"
		<< "layout (std140, set = 0, binding = 8) uniform buffer8 { mediump float uf_fourth; };"
		<< "layout (std140, set = 0, binding = 9) uniform buffer9 { mediump float uf_sixth; };"
		<< ""
		<< "struct T {"
		<< "	mediump float	a;"
		<< "	mediump vec2	b[2];"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b[3];"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 10) uniform buffer10 { S s[2]; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	mediump float r = 0.0; // (x*3 + y*3) / 6.0"
		<< "	mediump float g = 0.0; // (y*3 + z*3) / 6.0"
		<< "	mediump float b = 0.0; // (z*3 + w*3) / 6.0"
		<< "	mediump float a = 1.0;"
		<< "	for (int i = 0; i < 2; i++)"
		<< "	{"
		<< "		for (int j = 0; j < 3; j++)"
		<< "		{"
		<< "			r += s[0].b[j].b[i].y;"
		<< "			g += s[i].b[j].b[0].x;"
		<< "			b += s[i].b[j].b[1].x;"
		<< "			a *= s[i].b[j].a;"
		<< "		}"
		<< "	}"
		<< "	${DST} = vec4(r*uf_sixth, g*uf_sixth, b*uf_sixth, a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.useUniform(3u, UF_TWO);
			instance.useUniform(4u, UF_THREE);
			instance.useUniform(5u, UF_FOUR);
			instance.useUniform(6u, UF_HALF);
			instance.useUniform(7u, UF_THIRD);
			instance.useUniform(8u, UF_FOURTH);
			instance.useUniform(9u, UF_SIXTH);

			struct T {
				float		a;
				float		_padding1[3];
				tcu::Vec4	b[2];
			};

			struct S {
				float	a;
				float	_padding1[3];
				T		b[3];
				int		c;
				float	_padding2[3];
			};

			S s[2];
			s[0].a = constCoords.x();
			s[0].b[0].a = 0.5f;
			s[0].b[0].b[0] = constCoords.swizzle(1,0,0,0);
			s[0].b[0].b[1] = constCoords.swizzle(2,0,0,0);
			s[0].b[1].a = 1.0f / 3.0f;
			s[0].b[1].b[0] = constCoords.swizzle(1,1,0,0);
			s[0].b[1].b[1] = constCoords.swizzle(3,1,0,0);
			s[0].b[2].a = 1.0f / 4.0f;
			s[0].b[2].b[0] = constCoords.swizzle(2,1,0,0);
			s[0].b[2].b[1] = constCoords.swizzle(2,1,0,0);
			s[0].c = 0;

			s[1].a = constCoords.w();
			s[1].b[0].a = 2.0f;
			s[1].b[0].b[0] = constCoords.swizzle(2,0,0,0);
			s[1].b[0].b[1] = constCoords.swizzle(2,1,0,0);
			s[1].b[1].a = 3.0f;
			s[1].b[1].b[0] = constCoords.swizzle(2,2,0,0);
			s[1].b[1].b[1] = constCoords.swizzle(3,3,0,0);
			s[1].b[2].a = 4.0f;
			s[1].b[2].b[0] = constCoords.swizzle(1,0,0,0);
			s[1].b[2].b[1] = constCoords.swizzle(3,2,0,0);
			s[1].c = 1;

			instance.addUniform(10u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * sizeof(S), s);

		},
		{
			c.color.xyz() = (c.constCoords.swizzle(0, 1, 2) + c.constCoords.swizzle(1, 2, 3)) * 0.5f;
		});

	UNIFORM_STRUCT_CASE(dynamic_loop_struct_array, "Struct array usage in dynamic loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { int ui_three; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump int		b;"
		<< "};"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { S s[3]; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	mediump float rgb[3];"
		<< "	int alpha = 0;"
		<< "	for (int i = 0; i < ui_three; i++)"
		<< "	{"
		<< "		rgb[i] = s[2-i].a;"
		<< "		alpha += s[i].b;"
		<< "	}"
		<< "	${DST} = vec4(rgb[0], rgb[1], rgb[2], alpha);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.useUniform(3u, UI_THREE);

			struct S {
				float	a;
				int		b;
				float	_padding1[2];
			};

			S s[3];
			s[0].a = constCoords.x();
			s[0].b = 0;
			s[1].a = constCoords.y();
			s[1].b = -1;
			s[2].a = constCoords.z();
			s[2].b = 2;
			instance.addUniform(4u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3u * sizeof(S), s);

		},
		{
			c.color.xyz() = c.constCoords.swizzle(2, 1, 0);
		});

	UNIFORM_STRUCT_CASE(dynamic_loop_nested_struct_array, "Nested struct array usage in dynamic loop",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { int ui_zero; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_one; };"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { int ui_two; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { int ui_three; };"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { mediump float uf_two; };"
		<< "layout (std140, set = 0, binding = 5) uniform buffer5 { mediump float uf_three; };"
		<< "layout (std140, set = 0, binding = 6) uniform buffer6 { mediump float uf_four; };"
		<< "layout (std140, set = 0, binding = 7) uniform buffer7 { mediump float uf_half; };"
		<< "layout (std140, set = 0, binding = 8) uniform buffer8 { mediump float uf_third; };"
		<< "layout (std140, set = 0, binding = 9) uniform buffer9 { mediump float uf_fourth; };"
		<< "layout (std140, set = 0, binding = 10) uniform buffer10 { mediump float uf_sixth; };"
		<< ""
		<< "struct T {"
		<< "	mediump float	a;"
		<< "	mediump vec2	b[2];"
		<< "};"
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	T				b[3];"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 11) uniform buffer11 { S s[2]; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	mediump float r = 0.0; // (x*3 + y*3) / 6.0"
		<< "	mediump float g = 0.0; // (y*3 + z*3) / 6.0"
		<< "	mediump float b = 0.0; // (z*3 + w*3) / 6.0"
		<< "	mediump float a = 1.0;"
		<< "	for (int i = 0; i < ui_two; i++)"
		<< "	{"
		<< "		for (int j = 0; j < ui_three; j++)"
		<< "		{"
		<< "			r += s[0].b[j].b[i].y;"
		<< "			g += s[i].b[j].b[0].x;"
		<< "			b += s[i].b[j].b[1].x;"
		<< "			a *= s[i].b[j].a;"
		<< "		}"
		<< "	}"
		<< "	${DST} = vec4(r*uf_sixth, g*uf_sixth, b*uf_sixth, a);"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			instance.useUniform(0u, UI_ZERO);
			instance.useUniform(1u, UI_ONE);
			instance.useUniform(2u, UI_TWO);
			instance.useUniform(3u, UI_THREE);
			instance.useUniform(4u, UF_TWO);
			instance.useUniform(5u, UF_THREE);
			instance.useUniform(6u, UF_FOUR);
			instance.useUniform(7u, UF_HALF);
			instance.useUniform(8u, UF_THIRD);
			instance.useUniform(9u, UF_FOURTH);
			instance.useUniform(10u, UF_SIXTH);

			struct T {
				float		a;
				float		_padding1[3];
				tcu::Vec4	b[2];
			};

			struct S {
				float	a;
				float	_padding1[3];
				T		b[3];
				int		c;
				float	_padding2[3];
			};

			S s[2];
			s[0].a = constCoords.x();
			s[0].b[0].a = 0.5f;
			s[0].b[0].b[0] = constCoords.swizzle(1,0,0,0);
			s[0].b[0].b[1] = constCoords.swizzle(2,0,0,0);
			s[0].b[1].a = 1.0f / 3.0f;
			s[0].b[1].b[0] = constCoords.swizzle(1,1,0,0);
			s[0].b[1].b[1] = constCoords.swizzle(3,1,0,0);
			s[0].b[2].a = 1.0f / 4.0f;
			s[0].b[2].b[0] = constCoords.swizzle(2,1,0,0);
			s[0].b[2].b[1] = constCoords.swizzle(2,1,0,0);
			s[0].c = 0;

			s[1].a = constCoords.w();
			s[1].b[0].a = 2.0f;
			s[1].b[0].b[0] = constCoords.swizzle(2,0,0,0);
			s[1].b[0].b[1] = constCoords.swizzle(2,1,0,0);
			s[1].b[1].a = 3.0f;
			s[1].b[1].b[0] = constCoords.swizzle(2,2,0,0);
			s[1].b[1].b[1] = constCoords.swizzle(3,3,0,0);
			s[1].b[2].a = 4.0f;
			s[1].b[2].b[0] = constCoords.swizzle(1,0,0,0);
			s[1].b[2].b[1] = constCoords.swizzle(3,2,0,0);
			s[1].c = 1;

			instance.addUniform(11u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * sizeof(S), s);

		},
		{
			c.color.xyz() = (c.constCoords.swizzle(0, 1, 2) + c.constCoords.swizzle(1, 2, 3)) * 0.5f;
		});

	UNIFORM_STRUCT_CASE(equal, "Struct equality",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { mediump float uf_one; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { S a; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { S b; };"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { S c; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S d = S(uf_one, vec3(0.0, floor(${COORDS}.y+1.0), 2.0), ui_two);"
		<< "	${DST} = vec4(0.0, 0.0, 0.0, 1.0);"
		<< "	if (a == b) ${DST}.x = 1.0;"
		<< "	if (a == c) ${DST}.y = 1.0;"
		<< "	if (a == d) ${DST}.z = 1.0;"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			DE_UNREF(constCoords);
			instance.useUniform(0u, UF_ONE);
			instance.useUniform(1u, UI_TWO);

			struct S {
				float			a;
				float			_padding1[3];
				tcu::Vec3		b;
				int				c;
			};

			S sa;
			sa.a = 1.0f;
			sa.b = tcu::Vec3(0.0f, 1.0f, 2.0f);
			sa.c = 2;
			instance.addUniform(2u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(S), &sa);

			S sb;
			sb.a = 1.0f;
			sb.b = tcu::Vec3(0.0f, 1.0f, 2.0f);
			sb.c = 2;
			instance.addUniform(3u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(S), &sb);

			S sc;
			sc.a = 1.0f;
			sc.b = tcu::Vec3(0.0f, 1.1f, 2.0f);
			sc.c = 2;
			instance.addUniform(4u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(S), &sc);
		},
		{
			c.color.xy() = tcu::Vec2(1.0f, 0.0f);
			if (deFloatFloor(c.coords[1] + 1.0f) == deFloatFloor(1.1f))
				c.color.z() = 1.0f;
		});

	UNIFORM_STRUCT_CASE(not_equal, "Struct equality",
		LineStream()
		<< "${HEADER}"
		<< "layout (std140, set = 0, binding = 0) uniform buffer0 { mediump float uf_one; };"
		<< "layout (std140, set = 0, binding = 1) uniform buffer1 { int ui_two; };"
		<< ""
		<< "struct S {"
		<< "	mediump float	a;"
		<< "	mediump vec3	b;"
		<< "	int				c;"
		<< "};"
		<< "layout (std140, set = 0, binding = 2) uniform buffer2 { S a; };"
		<< "layout (std140, set = 0, binding = 3) uniform buffer3 { S b; };"
		<< "layout (std140, set = 0, binding = 4) uniform buffer4 { S c; };"
		<< ""
		<< "void main (void)"
		<< "{"
		<< "	S d = S(uf_one, vec3(0.0, floor(${COORDS}.y+1.0), 2.0), ui_two);"
		<< "	${DST} = vec4(0.0, 0.0, 0.0, 1.0);"
		<< "	if (a != b) ${DST}.x = 1.0;"
		<< "	if (a != c) ${DST}.y = 1.0;"
		<< "	if (a != d) ${DST}.z = 1.0;"
		<< "	${ASSIGN_POS}"
		<< "}",
		{
			DE_UNREF(constCoords);
			instance.useUniform(0u, UF_ONE);
			instance.useUniform(1u, UI_TWO);

			struct S {
				float			a;
				float			_padding1[3];
				tcu::Vec3		b;
				int				c;
			};

			S sa;
			sa.a = 1.0f;
			sa.b = tcu::Vec3(0.0f, 1.0f, 2.0f);
			sa.c = 2;
			instance.addUniform(2u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(S), &sa);

			S sb;
			sb.a = 1.0f;
			sb.b = tcu::Vec3(0.0f, 1.0f, 2.0f);
			sb.c = 2;
			instance.addUniform(3u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(S), &sb);

			S sc;
			sc.a = 1.0f;
			sc.b = tcu::Vec3(0.0f, 1.1f, 2.0f);
			sc.c = 2;
			instance.addUniform(4u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(S), &sc);
		},
		{
			c.color.xy() = tcu::Vec2(0.0f, 1.0f);
			if (deFloatFloor(c.coords[1] + 1.0f) != deFloatFloor(1.1f))
				c.color.z() = 1.0f;
		});
}

class ShaderStructTests : public tcu::TestCaseGroup
{
public:
							ShaderStructTests		(tcu::TestContext& context);
	virtual					~ShaderStructTests		(void);

	virtual void			init					(void);

private:
							ShaderStructTests		(const ShaderStructTests&);		// not allowed!
	ShaderStructTests&		operator=				(const ShaderStructTests&);		// not allowed!
};

ShaderStructTests::ShaderStructTests (tcu::TestContext& testCtx)
	: TestCaseGroup(testCtx, "struct", "Struct Tests")
{
}

ShaderStructTests::~ShaderStructTests (void)
{
}

void ShaderStructTests::init (void)
{
	addChild(new LocalStructTests(m_testCtx));
	addChild(new UniformStructTests(m_testCtx));
}

} // anonymous

tcu::TestCaseGroup* createStructTests (tcu::TestContext& testCtx)
{
	return new ShaderStructTests(testCtx);
}

} // sr
} // vkt
