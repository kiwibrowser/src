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
 * \brief Shader indexing (arrays, vector, matrices) tests.
 *//*--------------------------------------------------------------------*/

#include "vktShaderRenderIndexingTests.hpp"
#include "vktShaderRender.hpp"
#include "gluShaderUtil.hpp"
#include "tcuStringTemplate.hpp"

#include <map>

using namespace std;
using namespace tcu;
using namespace glu;

namespace vkt
{
namespace sr
{

namespace
{

enum IndexAccessType
{
	INDEXACCESS_STATIC = 0,
	INDEXACCESS_DYNAMIC,
	INDEXACCESS_STATIC_LOOP,
	INDEXACCESS_DYNAMIC_LOOP,

	INDEXACCESS_LAST
};

static const char* getIndexAccessTypeName (IndexAccessType accessType)
{
	static const char* s_names[INDEXACCESS_LAST] =
	{
		"static",
		"dynamic",
		"static_loop",
		"dynamic_loop"
	};

	DE_ASSERT(deInBounds32((int)accessType, 0, INDEXACCESS_LAST));
	return s_names[(int)accessType];
}

enum VectorAccessType
{
	DIRECT = 0,
	COMPONENT,
	SUBSCRIPT_STATIC,
	SUBSCRIPT_DYNAMIC,
	SUBSCRIPT_STATIC_LOOP,
	SUBSCRIPT_DYNAMIC_LOOP,

	VECTORACCESS_LAST
};

static const char* getVectorAccessTypeName (VectorAccessType accessType)
{
	static const char* s_names[VECTORACCESS_LAST] =
	{
		"direct",
		"component",
		"static_subscript",
		"dynamic_subscript",
		"static_loop_subscript",
		"dynamic_loop_subscript"
	};

	DE_ASSERT(deInBounds32((int)accessType, 0, VECTORACCESS_LAST));
	return s_names[(int)accessType];
}

void evalArrayCoordsFloat		(ShaderEvalContext& c) { c.color.x()	= 1.875f * c.coords.x(); }
void evalArrayCoordsVec2		(ShaderEvalContext& c) { c.color.xy()	= 1.875f * c.coords.swizzle(0,1); }
void evalArrayCoordsVec3		(ShaderEvalContext& c) { c.color.xyz()	= 1.875f * c.coords.swizzle(0,1,2); }
void evalArrayCoordsVec4		(ShaderEvalContext& c) { c.color		= 1.875f * c.coords; }

static ShaderEvalFunc getArrayCoordsEvalFunc (DataType dataType)
{
	if (dataType == TYPE_FLOAT)				return evalArrayCoordsFloat;
	else if (dataType == TYPE_FLOAT_VEC2)	return evalArrayCoordsVec2;
	else if (dataType == TYPE_FLOAT_VEC3)	return evalArrayCoordsVec3;
	else if (dataType == TYPE_FLOAT_VEC4)	return evalArrayCoordsVec4;

	DE_FATAL("Invalid data type.");
	return NULL;
}

void evalArrayUniformFloat		(ShaderEvalContext& c) { c.color.x()	= 1.875f * c.constCoords.x(); }
void evalArrayUniformVec2		(ShaderEvalContext& c) { c.color.xy()	= 1.875f * c.constCoords.swizzle(0,1); }
void evalArrayUniformVec3		(ShaderEvalContext& c) { c.color.xyz()	= 1.875f * c.constCoords.swizzle(0,1,2); }
void evalArrayUniformVec4		(ShaderEvalContext& c) { c.color		= 1.875f * c.constCoords; }

static ShaderEvalFunc getArrayUniformEvalFunc (DataType dataType)
{
	if (dataType == TYPE_FLOAT)				return evalArrayUniformFloat;
	else if (dataType == TYPE_FLOAT_VEC2)	return evalArrayUniformVec2;
	else if (dataType == TYPE_FLOAT_VEC3)	return evalArrayUniformVec3;
	else if (dataType == TYPE_FLOAT_VEC4)	return evalArrayUniformVec4;

	DE_FATAL("Invalid data type.");
	return NULL;
}

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

class IndexingTestUniformSetup : public UniformSetup
{
public:
							IndexingTestUniformSetup	(const DataType varType, bool usesArray)
								: m_varType(varType)
								, m_usesArray(usesArray)
							{}
	virtual					~IndexingTestUniformSetup	(void)
							{}

	virtual void			setup						(ShaderRenderCaseInstance& instance, const tcu::Vec4& constCoords) const;

private:
	const DataType			m_varType;
	const bool				m_usesArray;
};

void IndexingTestUniformSetup::setup (ShaderRenderCaseInstance& instance, const tcu::Vec4& constCoords) const
{
	instance.useUniform(0u, UI_ZERO);
	instance.useUniform(1u, UI_ONE);
	instance.useUniform(2u, UI_TWO);
	instance.useUniform(3u, UI_THREE);
	instance.useUniform(4u, UI_FOUR);

	if (m_usesArray)
	{
		Vec4 arr[4];
		if (m_varType == TYPE_FLOAT)
		{
			arr[0] = Vec4(constCoords.x());
			arr[1] = Vec4(constCoords.x() * 0.5f);
			arr[2] = Vec4(constCoords.x() * 0.25f);
			arr[3] = Vec4(constCoords.x() * 0.125f);
		}
		else if (m_varType == TYPE_FLOAT_VEC2)
		{
			arr[0] = constCoords.swizzle(0, 1).toWidth<4>();
			arr[1] = (constCoords.swizzle(0, 1) * 0.5f).toWidth<4>();
			arr[2] = (constCoords.swizzle(0, 1) * 0.25f).toWidth<4>();
			arr[3] = (constCoords.swizzle(0, 1) * 0.125f).toWidth<4>();
		}
		else if (m_varType == TYPE_FLOAT_VEC3)
		{
			arr[0] = constCoords.swizzle(0, 1, 2).toWidth<4>();
			arr[1] = (constCoords.swizzle(0, 1, 2) * 0.5f).toWidth<4>();
			arr[2] = (constCoords.swizzle(0, 1, 2) * 0.25f).toWidth<4>();
			arr[3] = (constCoords.swizzle(0, 1, 2) * 0.125f).toWidth<4>();
		}
		else if (m_varType == TYPE_FLOAT_VEC4)
		{
			arr[0] = constCoords;
			arr[1] = constCoords * 0.5f;
			arr[2] = constCoords * 0.25f;
			arr[3] = constCoords * 0.125f;
		}
		else
			throw tcu::TestError("invalid data type for u_arr");

		instance.addUniform(5u, vk::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(Vec4) * 4, arr[0].getPtr());
	}
}

// ShaderIndexingCase

class ShaderIndexingCase : public ShaderRenderCase
{
public:
								ShaderIndexingCase		(tcu::TestContext&			testCtx,
														const std::string&			name,
														const std::string&			description,
														bool						isVertexCase,
														const ShaderEvalFunc		evalFunc,
														const std::string&			vertShaderSource,
														const std::string&			fragShaderSource,
														const DataType				varType,
														const bool					usesArray);
	virtual						~ShaderIndexingCase		(void);

private:
								ShaderIndexingCase		(const ShaderIndexingCase&);	// not allowed!
	ShaderIndexingCase&			operator=				(const ShaderIndexingCase&);	// not allowed!
};

ShaderIndexingCase::ShaderIndexingCase (tcu::TestContext&			testCtx,
										const std::string&			name,
										const std::string&			description,
										const bool					isVertexCase,
										const ShaderEvalFunc		evalFunc,
										const std::string&			vertShaderSource,
										const std::string&			fragShaderSource,
										const DataType				varType,
										const bool					usesArray)
	: ShaderRenderCase(testCtx, name, description, isVertexCase, evalFunc, new IndexingTestUniformSetup(varType, usesArray), DE_NULL)
{
	m_vertShaderSource	= vertShaderSource;
	m_fragShaderSource	= fragShaderSource;
}

ShaderIndexingCase::~ShaderIndexingCase (void)
{
}

// Test case builders.

static de::MovePtr<ShaderIndexingCase> createVaryingArrayCase (tcu::TestContext&	context,
															const std::string&		caseName,
															const std::string&		description,
															DataType				varType,
															IndexAccessType			vertAccess,
															IndexAccessType			fragAccess)
{
	std::ostringstream vtx;
	vtx << "#version 310 es\n";
	vtx << "layout(location = 0) in highp vec4 a_position;\n";
	vtx << "layout(location = 1) in highp vec4 a_coords;\n";
	if (vertAccess == INDEXACCESS_DYNAMIC)
	{
		vtx << "layout(std140, binding = 0) uniform something0 { mediump int ui_zero; };\n";
		vtx << "layout(std140, binding = 1) uniform something1 { mediump int ui_one; };\n";
		vtx << "layout(std140, binding = 2) uniform something2 { mediump int ui_two; };\n";
		vtx << "layout(std140, binding = 3) uniform something3 { mediump int ui_three; };\n";
	}
	else if (vertAccess == INDEXACCESS_DYNAMIC_LOOP)
		vtx << "layout(std140, binding = 4) uniform something { mediump int ui_four; };\n";
	vtx << "layout(location = 0) out ${PRECISION} ${VAR_TYPE} var[${ARRAY_LEN}];\n";
	vtx << "\n";
	vtx << "void main()\n";
	vtx << "{\n";
	vtx << "	gl_Position = a_position;\n";
	if (vertAccess == INDEXACCESS_STATIC)
	{
		vtx << "	var[0] = ${VAR_TYPE}(a_coords);\n";
		vtx << "	var[1] = ${VAR_TYPE}(a_coords) * 0.5;\n";
		vtx << "	var[2] = ${VAR_TYPE}(a_coords) * 0.25;\n";
		vtx << "	var[3] = ${VAR_TYPE}(a_coords) * 0.125;\n";
	}
	else if (vertAccess == INDEXACCESS_DYNAMIC)
	{
		vtx << "	var[ui_zero]  = ${VAR_TYPE}(a_coords);\n";
		vtx << "	var[ui_one]   = ${VAR_TYPE}(a_coords) * 0.5;\n";
		vtx << "	var[ui_two]   = ${VAR_TYPE}(a_coords) * 0.25;\n";
		vtx << "	var[ui_three] = ${VAR_TYPE}(a_coords) * 0.125;\n";
	}
	else if (vertAccess == INDEXACCESS_STATIC_LOOP)
	{
		vtx << "	${PRECISION} ${VAR_TYPE} coords = ${VAR_TYPE}(a_coords);\n";
		vtx << "	for (int i = 0; i < 4; i++)\n";
		vtx << "	{\n";
		vtx << "		var[i] = ${VAR_TYPE}(coords);\n";
		vtx << "		coords = coords * 0.5;\n";
		vtx << "	}\n";
	}
	else
	{
		DE_ASSERT(vertAccess == INDEXACCESS_DYNAMIC_LOOP);
		vtx << "	${PRECISION} ${VAR_TYPE} coords = ${VAR_TYPE}(a_coords);\n";
		vtx << "	for (int i = 0; i < ui_four; i++)\n";
		vtx << "	{\n";
		vtx << "		var[i] = ${VAR_TYPE}(coords);\n";
		vtx << "		coords = coords * 0.5;\n";
		vtx << "	}\n";
	}
	vtx << "}\n";

	std::ostringstream frag;
	frag << "#version 310 es\n";
	frag << "precision mediump int;\n";
	frag << "layout(location = 0) out mediump vec4 o_color;\n";
	if (fragAccess == INDEXACCESS_DYNAMIC)
	{
		frag << "layout(std140, binding = 0) uniform something0 { mediump int ui_zero; };\n";
		frag << "layout(std140, binding = 1) uniform something1 { mediump int ui_one; };\n";
		frag << "layout(std140, binding = 2) uniform something2 { mediump int ui_two; };\n";
		frag << "layout(std140, binding = 3) uniform something3 { mediump int ui_three; };\n";
	}
	else if (fragAccess == INDEXACCESS_DYNAMIC_LOOP)
		frag << "layout(std140, binding = 4) uniform something4 { mediump int ui_four; };\n";
	frag << "layout(location = 0) in ${PRECISION} ${VAR_TYPE} var[${ARRAY_LEN}];\n";
	frag << "\n";
	frag << "void main()\n";
	frag << "{\n";
	frag << "	${PRECISION} ${VAR_TYPE} res = ${VAR_TYPE}(0.0);\n";
	if (fragAccess == INDEXACCESS_STATIC)
	{
		frag << "	res += var[0];\n";
		frag << "	res += var[1];\n";
		frag << "	res += var[2];\n";
		frag << "	res += var[3];\n";
	}
	else if (fragAccess == INDEXACCESS_DYNAMIC)
	{
		frag << "	res += var[ui_zero];\n";
		frag << "	res += var[ui_one];\n";
		frag << "	res += var[ui_two];\n";
		frag << "	res += var[ui_three];\n";
	}
	else if (fragAccess == INDEXACCESS_STATIC_LOOP)
	{
		frag << "	for (int i = 0; i < 4; i++)\n";
		frag << "		res += var[i];\n";
	}
	else
	{
		DE_ASSERT(fragAccess == INDEXACCESS_DYNAMIC_LOOP);
		frag << "	for (int i = 0; i < ui_four; i++)\n";
		frag << "		res += var[i];\n";
	}
	frag << "	o_color = vec4(res${PADDING});\n";
	frag << "}\n";

	// Fill in shader templates.
	map<string, string> params;
	params.insert(pair<string, string>("VAR_TYPE", getDataTypeName(varType)));
	params.insert(pair<string, string>("ARRAY_LEN", "4"));
	params.insert(pair<string, string>("PRECISION", "mediump"));

	if (varType == TYPE_FLOAT)
		params.insert(pair<string, string>("PADDING", ", 0.0, 0.0, 1.0"));
	else if (varType == TYPE_FLOAT_VEC2)
		params.insert(pair<string, string>("PADDING", ", 0.0, 1.0"));
	else if (varType == TYPE_FLOAT_VEC3)
		params.insert(pair<string, string>("PADDING", ", 1.0"));
	else
		params.insert(pair<string, string>("PADDING", ""));

	StringTemplate vertTemplate(vtx.str());
	StringTemplate fragTemplate(frag.str());
	string vertexShaderSource = vertTemplate.specialize(params);
	string fragmentShaderSource = fragTemplate.specialize(params);

	ShaderEvalFunc evalFunc = getArrayCoordsEvalFunc(varType);
	return de::MovePtr<ShaderIndexingCase>(new ShaderIndexingCase(context, caseName, description, true, evalFunc, vertexShaderSource, fragmentShaderSource, varType, false));
}

static de::MovePtr<ShaderIndexingCase> createUniformArrayCase (tcu::TestContext&	context,
															const std::string&		caseName,
															const std::string&		description,
															bool					isVertexCase,
															DataType				varType,
															IndexAccessType			readAccess)
{
	std::ostringstream vtx;
	std::ostringstream frag;
	std::ostringstream& op = isVertexCase ? vtx : frag;

	vtx << "#version 310 es\n";
	frag << "#version 310 es\n";

	vtx << "layout(location = 0) in highp vec4 a_position;\n";
	vtx << "layout(location = 1) in highp vec4 a_coords;\n";
	frag << "layout(location = 0) out mediump vec4 o_color;\n";

	if (isVertexCase)
	{
		vtx << "layout(location = 0) out mediump vec4 v_color;\n";
		frag << "layout(location = 0) in mediump vec4 v_color;\n";
	}
	else
	{
		vtx << "layout(location = 0) out mediump vec4 v_coords;\n";
		frag << "layout(location = 0) in mediump vec4 v_coords;\n";
	}

	if (readAccess == INDEXACCESS_DYNAMIC)
	{
		op << "layout(std140, binding = 0) uniform something0 { mediump int ui_zero; };\n";
		op << "layout(std140, binding = 1) uniform something1 { mediump int ui_one; };\n";
		op << "layout(std140, binding = 2) uniform something2 { mediump int ui_two; };\n";
		op << "layout(std140, binding = 3) uniform something3 { mediump int ui_three; };\n";
	}
	else if (readAccess == INDEXACCESS_DYNAMIC_LOOP)
		op << "layout(std140, binding = 4) uniform something4 { mediump int ui_four; };\n";

	op << "layout(std140, binding = 5) uniform something5 { ${PRECISION} ${VAR_TYPE} u_arr[${ARRAY_LEN}]; };\n";

	vtx << "\n";
	vtx << "void main()\n";
	vtx << "{\n";
	vtx << "	gl_Position = a_position;\n";

	frag << "\n";
	frag << "void main()\n";
	frag << "{\n";

	// Read array.
	op << "	${PRECISION} ${VAR_TYPE} res = ${VAR_TYPE}(0.0);\n";
	if (readAccess == INDEXACCESS_STATIC)
	{
		op << "	res += u_arr[0];\n";
		op << "	res += u_arr[1];\n";
		op << "	res += u_arr[2];\n";
		op << "	res += u_arr[3];\n";
	}
	else if (readAccess == INDEXACCESS_DYNAMIC)
	{
		op << "	res += u_arr[ui_zero];\n";
		op << "	res += u_arr[ui_one];\n";
		op << "	res += u_arr[ui_two];\n";
		op << "	res += u_arr[ui_three];\n";
	}
	else if (readAccess == INDEXACCESS_STATIC_LOOP)
	{
		op << "	for (int i = 0; i < 4; i++)\n";
		op << "		res += u_arr[i];\n";
	}
	else
	{
		DE_ASSERT(readAccess == INDEXACCESS_DYNAMIC_LOOP);
		op << "	for (int i = 0; i < ui_four; i++)\n";
		op << "		res += u_arr[i];\n";
	}

	if (isVertexCase)
	{
		vtx << "	v_color = vec4(res${PADDING});\n";
		frag << "	o_color = v_color;\n";
	}
	else
	{
		vtx << "	v_coords = a_coords;\n";
		frag << "	o_color = vec4(res${PADDING});\n";
	}

	vtx << "}\n";
	frag << "}\n";

	// Fill in shader templates.
	map<string, string> params;
	params.insert(pair<string, string>("VAR_TYPE", getDataTypeName(varType)));
	params.insert(pair<string, string>("ARRAY_LEN", "4"));
	params.insert(pair<string, string>("PRECISION", "mediump"));

	if (varType == TYPE_FLOAT)
		params.insert(pair<string, string>("PADDING", ", 0.0, 0.0, 1.0"));
	else if (varType == TYPE_FLOAT_VEC2)
		params.insert(pair<string, string>("PADDING", ", 0.0, 1.0"));
	else if (varType == TYPE_FLOAT_VEC3)
		params.insert(pair<string, string>("PADDING", ", 1.0"));
	else
		params.insert(pair<string, string>("PADDING", ""));

	StringTemplate vertTemplate(vtx.str());
	StringTemplate fragTemplate(frag.str());
	string vertexShaderSource = vertTemplate.specialize(params);
	string fragmentShaderSource = fragTemplate.specialize(params);

	ShaderEvalFunc evalFunc = getArrayUniformEvalFunc(varType);
	return de::MovePtr<ShaderIndexingCase>(new ShaderIndexingCase(context, caseName, description, isVertexCase, evalFunc, vertexShaderSource, fragmentShaderSource, varType, true));
}

static de::MovePtr<ShaderIndexingCase> createTmpArrayCase (tcu::TestContext&	context,
														const std::string&		caseName,
														const std::string&		description,
														bool					isVertexCase,
														DataType				varType,
														IndexAccessType			writeAccess,
														IndexAccessType			readAccess)
{
	std::ostringstream vtx;
	std::ostringstream frag;
	std::ostringstream& op = isVertexCase ? vtx : frag;

	vtx << "#version 310 es\n";
	frag << "#version 310 es\n";

	vtx << "layout(location = 0) in highp vec4 a_position;\n";
	vtx << "layout(location = 1) in highp vec4 a_coords;\n";
	frag << "layout(location = 0) out mediump vec4 o_color;\n";

	if (isVertexCase)
	{
		vtx << "layout(location = 0) out mediump vec4 v_color;\n";
		frag << "layout(location = 0) in mediump vec4 v_color;\n";
	}
	else
	{
		vtx << "layout(location = 0) out mediump vec4 v_coords;\n";
		frag << "layout(location = 0) in mediump vec4 v_coords;\n";
	}

	if (writeAccess == INDEXACCESS_DYNAMIC || readAccess == INDEXACCESS_DYNAMIC)
	{
		op << "layout(std140, binding = 0) uniform something0 { mediump int ui_zero; };\n";
		op << "layout(std140, binding = 1) uniform something1 { mediump int ui_one; };\n";
		op << "layout(std140, binding = 2) uniform something2 { mediump int ui_two; };\n";
		op << "layout(std140, binding = 3) uniform something3 { mediump int ui_three; };\n";
	}

	if (writeAccess == INDEXACCESS_DYNAMIC_LOOP || readAccess == INDEXACCESS_DYNAMIC_LOOP)
		op << "layout(std140, binding = 4) uniform something4 { mediump int ui_four; };\n";

	vtx << "\n";
	vtx << "void main()\n";
	vtx << "{\n";
	vtx << "	gl_Position = a_position;\n";

	frag << "\n";
	frag << "void main()\n";
	frag << "{\n";

	// Write array.
	if (isVertexCase)
		op << "	${PRECISION} ${VAR_TYPE} coords = ${VAR_TYPE}(a_coords);\n";
	else
		op << "	${PRECISION} ${VAR_TYPE} coords = ${VAR_TYPE}(v_coords);\n";

	op << "	${PRECISION} ${VAR_TYPE} arr[${ARRAY_LEN}];\n";
	if (writeAccess == INDEXACCESS_STATIC)
	{
		op << "	arr[0] = ${VAR_TYPE}(coords);\n";
		op << "	arr[1] = ${VAR_TYPE}(coords) * 0.5;\n";
		op << "	arr[2] = ${VAR_TYPE}(coords) * 0.25;\n";
		op << "	arr[3] = ${VAR_TYPE}(coords) * 0.125;\n";
	}
	else if (writeAccess == INDEXACCESS_DYNAMIC)
	{
		op << "	arr[ui_zero]  = ${VAR_TYPE}(coords);\n";
		op << "	arr[ui_one]   = ${VAR_TYPE}(coords) * 0.5;\n";
		op << "	arr[ui_two]   = ${VAR_TYPE}(coords) * 0.25;\n";
		op << "	arr[ui_three] = ${VAR_TYPE}(coords) * 0.125;\n";
	}
	else if (writeAccess == INDEXACCESS_STATIC_LOOP)
	{
		op << "	for (int i = 0; i < 4; i++)\n";
		op << "	{\n";
		op << "		arr[i] = ${VAR_TYPE}(coords);\n";
		op << "		coords = coords * 0.5;\n";
		op << "	}\n";
	}
	else
	{
		DE_ASSERT(writeAccess == INDEXACCESS_DYNAMIC_LOOP);
		op << "	for (int i = 0; i < ui_four; i++)\n";
		op << "	{\n";
		op << "		arr[i] = ${VAR_TYPE}(coords);\n";
		op << "		coords = coords * 0.5;\n";
		op << "	}\n";
	}

	// Read array.
	op << "	${PRECISION} ${VAR_TYPE} res = ${VAR_TYPE}(0.0);\n";
	if (readAccess == INDEXACCESS_STATIC)
	{
		op << "	res += arr[0];\n";
		op << "	res += arr[1];\n";
		op << "	res += arr[2];\n";
		op << "	res += arr[3];\n";
	}
	else if (readAccess == INDEXACCESS_DYNAMIC)
	{
		op << "	res += arr[ui_zero];\n";
		op << "	res += arr[ui_one];\n";
		op << "	res += arr[ui_two];\n";
		op << "	res += arr[ui_three];\n";
	}
	else if (readAccess == INDEXACCESS_STATIC_LOOP)
	{
		op << "	for (int i = 0; i < 4; i++)\n";
		op << "		res += arr[i];\n";
	}
	else
	{
		DE_ASSERT(readAccess == INDEXACCESS_DYNAMIC_LOOP);
		op << "	for (int i = 0; i < ui_four; i++)\n";
		op << "		res += arr[i];\n";
	}

	if (isVertexCase)
	{
		vtx << "	v_color = vec4(res${PADDING});\n";
		frag << "	o_color = v_color;\n";
	}
	else
	{
		vtx << "	v_coords = a_coords;\n";
		frag << "	o_color = vec4(res${PADDING});\n";
	}

	vtx << "}\n";
	frag << "}\n";

	// Fill in shader templates.
	map<string, string> params;
	params.insert(pair<string, string>("VAR_TYPE", getDataTypeName(varType)));
	params.insert(pair<string, string>("ARRAY_LEN", "4"));
	params.insert(pair<string, string>("PRECISION", "mediump"));

	if (varType == TYPE_FLOAT)
		params.insert(pair<string, string>("PADDING", ", 0.0, 0.0, 1.0"));
	else if (varType == TYPE_FLOAT_VEC2)
		params.insert(pair<string, string>("PADDING", ", 0.0, 1.0"));
	else if (varType == TYPE_FLOAT_VEC3)
		params.insert(pair<string, string>("PADDING", ", 1.0"));
	else
		params.insert(pair<string, string>("PADDING", ""));

	StringTemplate vertTemplate(vtx.str());
	StringTemplate fragTemplate(frag.str());
	string vertexShaderSource = vertTemplate.specialize(params);
	string fragmentShaderSource = fragTemplate.specialize(params);

	ShaderEvalFunc evalFunc = getArrayCoordsEvalFunc(varType);
	return de::MovePtr<ShaderIndexingCase>(new ShaderIndexingCase(context, caseName, description, isVertexCase, evalFunc, vertexShaderSource, fragmentShaderSource, varType, false));
}

// VECTOR SUBSCRIPT.

void evalSubscriptVec2 (ShaderEvalContext& c) { c.color.xyz() = Vec3(c.coords.x() + 0.5f*c.coords.y()); }
void evalSubscriptVec3 (ShaderEvalContext& c) { c.color.xyz() = Vec3(c.coords.x() + 0.5f*c.coords.y() + 0.25f*c.coords.z()); }
void evalSubscriptVec4 (ShaderEvalContext& c) { c.color.xyz() = Vec3(c.coords.x() + 0.5f*c.coords.y() + 0.25f*c.coords.z() + 0.125f*c.coords.w()); }

static ShaderEvalFunc getVectorSubscriptEvalFunc (DataType dataType)
{
	if (dataType == TYPE_FLOAT_VEC2)		return evalSubscriptVec2;
	else if (dataType == TYPE_FLOAT_VEC3)	return evalSubscriptVec3;
	else if (dataType == TYPE_FLOAT_VEC4)	return evalSubscriptVec4;

	DE_FATAL("Invalid data type.");
	return NULL;
}

static de::MovePtr<ShaderIndexingCase> createVectorSubscriptCase (tcu::TestContext&		context,
																const std::string&		caseName,
																const std::string&		description,
																bool					isVertexCase,
																DataType				varType,
																VectorAccessType		writeAccess,
																VectorAccessType		readAccess)
{
	std::ostringstream vtx;
	std::ostringstream frag;
	std::ostringstream& op = isVertexCase ? vtx : frag;

	int			vecLen		= getDataTypeScalarSize(varType);
	const char*	vecLenName	= getIntUniformName(vecLen);

	vtx << "#version 310 es\n";
	frag << "#version 310 es\n";

	vtx << "layout(location = 0) in highp vec4 a_position;\n";
	vtx << "layout(location = 1) in highp vec4 a_coords;\n";
	frag << "layout(location = 0) out mediump vec4 o_color;\n";

	if (isVertexCase)
	{
		vtx << "layout(location = 0) out mediump vec3 v_color;\n";
		frag << "layout(location = 0) in mediump vec3 v_color;\n";
	}
	else
	{
		vtx << "layout(location = 0) out mediump vec4 v_coords;\n";
		frag << "layout(location = 0) in mediump vec4 v_coords;\n";
	}

	if (writeAccess == SUBSCRIPT_DYNAMIC || readAccess == SUBSCRIPT_DYNAMIC)
	{
		op << "layout(std140, binding = 0) uniform something0 { mediump int ui_zero; };\n";
		if (vecLen >= 2) op << "layout(std140, binding = 1) uniform something1 { mediump int ui_one; };\n";
		if (vecLen >= 3) op << "layout(std140, binding = 2) uniform something2 { mediump int ui_two; };\n";
		if (vecLen >= 4) op << "layout(std140, binding = 3) uniform something3 { mediump int ui_three; };\n";
	}

	if (writeAccess == SUBSCRIPT_DYNAMIC_LOOP || readAccess == SUBSCRIPT_DYNAMIC_LOOP)
		op << "layout(std140, binding = " << vecLen << ") uniform something" << vecLen << " { mediump int " << vecLenName << "; };\n";

	vtx << "\n";
	vtx << "void main()\n";
	vtx << "{\n";
	vtx << "	gl_Position = a_position;\n";

	frag << "\n";
	frag << "void main()\n";
	frag << "{\n";

	// Write vector.
	if (isVertexCase)
		op << "	${PRECISION} ${VAR_TYPE} coords = ${VAR_TYPE}(a_coords);\n";
	else
		op << "	${PRECISION} ${VAR_TYPE} coords = ${VAR_TYPE}(v_coords);\n";

	op << "	${PRECISION} ${VAR_TYPE} tmp;\n";
	if (writeAccess == DIRECT)
		op << "	tmp = coords.${SWIZZLE} * vec4(1.0, 0.5, 0.25, 0.125).${SWIZZLE};\n";
	else if (writeAccess == COMPONENT)
	{
		op << "	tmp.x = coords.x;\n";
		if (vecLen >= 2) op << "	tmp.y = coords.y * 0.5;\n";
		if (vecLen >= 3) op << "	tmp.z = coords.z * 0.25;\n";
		if (vecLen >= 4) op << "	tmp.w = coords.w * 0.125;\n";
	}
	else if (writeAccess == SUBSCRIPT_STATIC)
	{
		op << "	tmp[0] = coords.x;\n";
		if (vecLen >= 2) op << "	tmp[1] = coords.y * 0.5;\n";
		if (vecLen >= 3) op << "	tmp[2] = coords.z * 0.25;\n";
		if (vecLen >= 4) op << "	tmp[3] = coords.w * 0.125;\n";
	}
	else if (writeAccess == SUBSCRIPT_DYNAMIC)
	{
		op << "	tmp[ui_zero]  = coords.x;\n";
		if (vecLen >= 2) op << "	tmp[ui_one]   = coords.y * 0.5;\n";
		if (vecLen >= 3) op << "	tmp[ui_two]   = coords.z * 0.25;\n";
		if (vecLen >= 4) op << "	tmp[ui_three] = coords.w * 0.125;\n";
	}
	else if (writeAccess == SUBSCRIPT_STATIC_LOOP)
	{
		op << "	for (int i = 0; i < " << vecLen << "; i++)\n";
		op << "	{\n";
		op << "		tmp[i] = coords.x;\n";
		op << "		coords = coords.${ROT_SWIZZLE} * 0.5;\n";
		op << "	}\n";
	}
	else
	{
		DE_ASSERT(writeAccess == SUBSCRIPT_DYNAMIC_LOOP);
		op << "	for (int i = 0; i < " << vecLenName << "; i++)\n";
		op << "	{\n";
		op << "		tmp[i] = coords.x;\n";
		op << "		coords = coords.${ROT_SWIZZLE} * 0.5;\n";
		op << "	}\n";
	}

	// Read vector.
	op << "	${PRECISION} float res = 0.0;\n";
	if (readAccess == DIRECT)
		op << "	res = dot(tmp, ${VAR_TYPE}(1.0));\n";
	else if (readAccess == COMPONENT)
	{
		op << "	res += tmp.x;\n";
		if (vecLen >= 2) op << "	res += tmp.y;\n";
		if (vecLen >= 3) op << "	res += tmp.z;\n";
		if (vecLen >= 4) op << "	res += tmp.w;\n";
	}
	else if (readAccess == SUBSCRIPT_STATIC)
	{
		op << "	res += tmp[0];\n";
		if (vecLen >= 2) op << "	res += tmp[1];\n";
		if (vecLen >= 3) op << "	res += tmp[2];\n";
		if (vecLen >= 4) op << "	res += tmp[3];\n";
	}
	else if (readAccess == SUBSCRIPT_DYNAMIC)
	{
		op << "	res += tmp[ui_zero];\n";
		if (vecLen >= 2) op << "	res += tmp[ui_one];\n";
		if (vecLen >= 3) op << "	res += tmp[ui_two];\n";
		if (vecLen >= 4) op << "	res += tmp[ui_three];\n";
	}
	else if (readAccess == SUBSCRIPT_STATIC_LOOP)
	{
		op << "	for (int i = 0; i < " << vecLen << "; i++)\n";
		op << "		res += tmp[i];\n";
	}
	else
	{
		DE_ASSERT(readAccess == SUBSCRIPT_DYNAMIC_LOOP);
		op << "	for (int i = 0; i < " << vecLenName << "; i++)\n";
		op << "		res += tmp[i];\n";
	}

	if (isVertexCase)
	{
		vtx << "	v_color = vec3(res);\n";
		frag << "	o_color = vec4(v_color.rgb, 1.0);\n";
	}
	else
	{
		vtx << "	v_coords = a_coords;\n";
		frag << "	o_color = vec4(vec3(res), 1.0);\n";
	}

	vtx << "}\n";
	frag << "}\n";

	// Fill in shader templates.
	static const char* s_swizzles[5]	= { "", "x", "xy", "xyz", "xyzw" };
	static const char* s_rotSwizzles[5]	= { "", "x", "yx", "yzx", "yzwx" };

	map<string, string> params;
	params.insert(pair<string, string>("VAR_TYPE", getDataTypeName(varType)));
	params.insert(pair<string, string>("PRECISION", "mediump"));
	params.insert(pair<string, string>("SWIZZLE", s_swizzles[vecLen]));
	params.insert(pair<string, string>("ROT_SWIZZLE", s_rotSwizzles[vecLen]));

	StringTemplate vertTemplate(vtx.str());
	StringTemplate fragTemplate(frag.str());
	string vertexShaderSource = vertTemplate.specialize(params);
	string fragmentShaderSource = fragTemplate.specialize(params);

	ShaderEvalFunc evalFunc = getVectorSubscriptEvalFunc(varType);
	return de::MovePtr<ShaderIndexingCase>(new ShaderIndexingCase(context, caseName, description, isVertexCase, evalFunc, vertexShaderSource, fragmentShaderSource, varType, false));
}

// MATRIX SUBSCRIPT.

void evalSubscriptMat2		(ShaderEvalContext& c) { c.color.xy()	= c.coords.swizzle(0,1) + 0.5f*c.coords.swizzle(1,2); }
void evalSubscriptMat2x3	(ShaderEvalContext& c) { c.color.xyz()	= c.coords.swizzle(0,1,2) + 0.5f*c.coords.swizzle(1,2,3); }
void evalSubscriptMat2x4	(ShaderEvalContext& c) { c.color		= c.coords.swizzle(0,1,2,3) + 0.5f*c.coords.swizzle(1,2,3,0); }

void evalSubscriptMat3x2	(ShaderEvalContext& c) { c.color.xy()	= c.coords.swizzle(0,1) + 0.5f*c.coords.swizzle(1,2) + 0.25f*c.coords.swizzle(2,3); }
void evalSubscriptMat3		(ShaderEvalContext& c) { c.color.xyz()	= c.coords.swizzle(0,1,2) + 0.5f*c.coords.swizzle(1,2,3) + 0.25f*c.coords.swizzle(2,3,0); }
void evalSubscriptMat3x4	(ShaderEvalContext& c) { c.color		= c.coords.swizzle(0,1,2,3) + 0.5f*c.coords.swizzle(1,2,3,0) + 0.25f*c.coords.swizzle(2,3,0,1); }

void evalSubscriptMat4x2	(ShaderEvalContext& c) { c.color.xy()	= c.coords.swizzle(0,1) + 0.5f*c.coords.swizzle(1,2) + 0.25f*c.coords.swizzle(2,3) + 0.125f*c.coords.swizzle(3,0); }
void evalSubscriptMat4x3	(ShaderEvalContext& c) { c.color.xyz()	= c.coords.swizzle(0,1,2) + 0.5f*c.coords.swizzle(1,2,3) + 0.25f*c.coords.swizzle(2,3,0) + 0.125f*c.coords.swizzle(3,0,1); }
void evalSubscriptMat4		(ShaderEvalContext& c) { c.color		= c.coords + 0.5f*c.coords.swizzle(1,2,3,0) + 0.25f*c.coords.swizzle(2,3,0,1) + 0.125f*c.coords.swizzle(3,0,1,2); }

static ShaderEvalFunc getMatrixSubscriptEvalFunc (DataType dataType)
{
	switch (dataType)
	{
		case TYPE_FLOAT_MAT2:		return evalSubscriptMat2;
		case TYPE_FLOAT_MAT2X3:		return evalSubscriptMat2x3;
		case TYPE_FLOAT_MAT2X4:		return evalSubscriptMat2x4;
		case TYPE_FLOAT_MAT3X2:		return evalSubscriptMat3x2;
		case TYPE_FLOAT_MAT3:		return evalSubscriptMat3;
		case TYPE_FLOAT_MAT3X4:		return evalSubscriptMat3x4;
		case TYPE_FLOAT_MAT4X2:		return evalSubscriptMat4x2;
		case TYPE_FLOAT_MAT4X3:		return evalSubscriptMat4x3;
		case TYPE_FLOAT_MAT4:		return evalSubscriptMat4;

		default:
			DE_FATAL("Invalid data type.");
			return DE_NULL;
	}
}

static de::MovePtr<ShaderIndexingCase> createMatrixSubscriptCase (tcu::TestContext&		context,
																const std::string&		caseName,
																const std::string&		description,
																bool					isVertexCase,
																DataType				varType,
																IndexAccessType			writeAccess,
																IndexAccessType			readAccess)
{
	std::ostringstream vtx;
	std::ostringstream frag;
	std::ostringstream& op = isVertexCase ? vtx : frag;

	int			numCols		= getDataTypeMatrixNumColumns(varType);
	int			numRows		= getDataTypeMatrixNumRows(varType);
	const char*	matSizeName	= getIntUniformName(numCols);
	DataType	vecType		= getDataTypeFloatVec(numRows);

	vtx << "#version 310 es\n";
	frag << "#version 310 es\n";

	vtx << "layout(location = 0) in highp vec4 a_position;\n";
	vtx << "layout(location = 1) in highp vec4 a_coords;\n";
	frag << "layout(location = 0) out mediump vec4 o_color;\n";

	if (isVertexCase)
	{
		vtx << "layout(location = 0) out mediump vec4 v_color;\n";
		frag << "layout(location = 0) in mediump vec4 v_color;\n";
	}
	else
	{
		vtx << "layout(location = 0) out mediump vec4 v_coords;\n";
		frag << "layout(location = 0) in mediump vec4 v_coords;\n";
	}

	if (writeAccess == INDEXACCESS_DYNAMIC || readAccess == INDEXACCESS_DYNAMIC)
	{
		op << "layout(std140, binding = 0) uniform something0 { mediump int ui_zero; };\n";
		if (numCols >= 2) op << "layout(std140, binding = 1) uniform something1 { mediump int ui_one; };\n";
		if (numCols >= 3) op << "layout(std140, binding = 2) uniform something2 { mediump int ui_two; };\n";
		if (numCols >= 4) op << "layout(std140, binding = 3) uniform something3 { mediump int ui_three; };\n";
	}

	if (writeAccess == INDEXACCESS_DYNAMIC_LOOP || readAccess == INDEXACCESS_DYNAMIC_LOOP)
		op << "layout(std140, binding = " << numCols << ") uniform something" << numCols << " { mediump int " << matSizeName << "; };\n";

	vtx << "\n";
	vtx << "void main()\n";
	vtx << "{\n";
	vtx << "	gl_Position = a_position;\n";

	frag << "\n";
	frag << "void main()\n";
	frag << "{\n";

	// Write matrix.
	if (isVertexCase)
		op << "	${PRECISION} vec4 coords = a_coords;\n";
	else
		op << "	${PRECISION} vec4 coords = v_coords;\n";

	op << "	${PRECISION} ${MAT_TYPE} tmp;\n";
	if (writeAccess == INDEXACCESS_STATIC)
	{
		op << "	tmp[0] = ${VEC_TYPE}(coords);\n";
		if (numCols >= 2) op << "	tmp[1] = ${VEC_TYPE}(coords.yzwx) * 0.5;\n";
		if (numCols >= 3) op << "	tmp[2] = ${VEC_TYPE}(coords.zwxy) * 0.25;\n";
		if (numCols >= 4) op << "	tmp[3] = ${VEC_TYPE}(coords.wxyz) * 0.125;\n";
	}
	else if (writeAccess == INDEXACCESS_DYNAMIC)
	{
		op << "	tmp[ui_zero]  = ${VEC_TYPE}(coords);\n";
		if (numCols >= 2) op << "	tmp[ui_one]   = ${VEC_TYPE}(coords.yzwx) * 0.5;\n";
		if (numCols >= 3) op << "	tmp[ui_two]   = ${VEC_TYPE}(coords.zwxy) * 0.25;\n";
		if (numCols >= 4) op << "	tmp[ui_three] = ${VEC_TYPE}(coords.wxyz) * 0.125;\n";
	}
	else if (writeAccess == INDEXACCESS_STATIC_LOOP)
	{
		op << "	for (int i = 0; i < " << numCols << "; i++)\n";
		op << "	{\n";
		op << "		tmp[i] = ${VEC_TYPE}(coords);\n";
		op << "		coords = coords.yzwx * 0.5;\n";
		op << "	}\n";
	}
	else
	{
		DE_ASSERT(writeAccess == INDEXACCESS_DYNAMIC_LOOP);
		op << "	for (int i = 0; i < " << matSizeName << "; i++)\n";
		op << "	{\n";
		op << "		tmp[i] = ${VEC_TYPE}(coords);\n";
		op << "		coords = coords.yzwx * 0.5;\n";
		op << "	}\n";
	}

	// Read matrix.
	op << "	${PRECISION} ${VEC_TYPE} res = ${VEC_TYPE}(0.0);\n";
	if (readAccess == INDEXACCESS_STATIC)
	{
		op << "	res += tmp[0];\n";
		if (numCols >= 2) op << "	res += tmp[1];\n";
		if (numCols >= 3) op << "	res += tmp[2];\n";
		if (numCols >= 4) op << "	res += tmp[3];\n";
	}
	else if (readAccess == INDEXACCESS_DYNAMIC)
	{
		op << "	res += tmp[ui_zero];\n";
		if (numCols >= 2) op << "	res += tmp[ui_one];\n";
		if (numCols >= 3) op << "	res += tmp[ui_two];\n";
		if (numCols >= 4) op << "	res += tmp[ui_three];\n";
	}
	else if (readAccess == INDEXACCESS_STATIC_LOOP)
	{
		op << "	for (int i = 0; i < " << numCols << "; i++)\n";
		op << "		res += tmp[i];\n";
	}
	else
	{
		DE_ASSERT(readAccess == INDEXACCESS_DYNAMIC_LOOP);
		op << "	for (int i = 0; i < " << matSizeName << "; i++)\n";
		op << "		res += tmp[i];\n";
	}

	if (isVertexCase)
	{
		vtx << "	v_color = vec4(res${PADDING});\n";
		frag << "	o_color = v_color;\n";
	}
	else
	{
		vtx << "	v_coords = a_coords;\n";
		frag << "	o_color = vec4(res${PADDING});\n";
	}

	vtx << "}\n";
	frag << "}\n";

	// Fill in shader templates.
	map<string, string> params;
	params.insert(pair<string, string>("MAT_TYPE", getDataTypeName(varType)));
	params.insert(pair<string, string>("VEC_TYPE", getDataTypeName(vecType)));
	params.insert(pair<string, string>("PRECISION", "mediump"));

	if (numRows == 2)
		params.insert(pair<string, string>("PADDING", ", 0.0, 1.0"));
	else if (numRows == 3)
		params.insert(pair<string, string>("PADDING", ", 1.0"));
	else
		params.insert(pair<string, string>("PADDING", ""));

	StringTemplate vertTemplate(vtx.str());
	StringTemplate fragTemplate(frag.str());
	string vertexShaderSource = vertTemplate.specialize(params);
	string fragmentShaderSource = fragTemplate.specialize(params);

	ShaderEvalFunc evalFunc = getMatrixSubscriptEvalFunc(varType);
	return de::MovePtr<ShaderIndexingCase>(new ShaderIndexingCase(context, caseName, description, isVertexCase, evalFunc, vertexShaderSource, fragmentShaderSource, varType, false));
}

// ShaderIndexingTests.

class ShaderIndexingTests : public tcu::TestCaseGroup
{
public:
							ShaderIndexingTests		(tcu::TestContext& context);
	virtual					~ShaderIndexingTests	(void);

	virtual void			init					(void);

private:
							ShaderIndexingTests		(const ShaderIndexingTests&);		// not allowed!
	ShaderIndexingTests&	operator=				(const ShaderIndexingTests&);		// not allowed!
};

ShaderIndexingTests::ShaderIndexingTests(tcu::TestContext& context)
	: TestCaseGroup(context, "indexing", "Indexing Tests")
{
}

ShaderIndexingTests::~ShaderIndexingTests (void)
{
}

void ShaderIndexingTests::init (void)
{
	static const ShaderType s_shaderTypes[] =
	{
		SHADERTYPE_VERTEX,
		SHADERTYPE_FRAGMENT
	};

	static const DataType s_floatAndVecTypes[] =
	{
		TYPE_FLOAT,
		TYPE_FLOAT_VEC2,
		TYPE_FLOAT_VEC3,
		TYPE_FLOAT_VEC4
	};

	// Varying array access cases.
	{
		de::MovePtr<TestCaseGroup> varyingGroup(new TestCaseGroup(m_testCtx, "varying_array", "Varying array access tests."));

		for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(s_floatAndVecTypes); typeNdx++)
		{
			DataType varType = s_floatAndVecTypes[typeNdx];
			for (int vertAccess = 0; vertAccess < INDEXACCESS_LAST; vertAccess++)
			{
				for (int fragAccess = 0; fragAccess < INDEXACCESS_LAST; fragAccess++)
				{
					const char* vertAccessName = getIndexAccessTypeName((IndexAccessType)vertAccess);
					const char* fragAccessName = getIndexAccessTypeName((IndexAccessType)fragAccess);
					string name = string(getDataTypeName(varType)) + "_" + vertAccessName + "_write_" + fragAccessName + "_read";
					string desc = string("Varying array with ") + vertAccessName + " write in vertex shader and " + fragAccessName + " read in fragment shader.";
					de::MovePtr<ShaderIndexingCase> testCase(createVaryingArrayCase(m_testCtx, name, desc, varType, (IndexAccessType)vertAccess, (IndexAccessType)fragAccess));
					varyingGroup->addChild(testCase.release());
				}
			}
		}

		addChild(varyingGroup.release());
	}

	// Uniform array access cases.
	{
		de::MovePtr<TestCaseGroup> uniformGroup(new TestCaseGroup(m_testCtx, "uniform_array", "Uniform array access tests."));

		for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(s_floatAndVecTypes); typeNdx++)
		{
			DataType varType = s_floatAndVecTypes[typeNdx];
			for (int readAccess = 0; readAccess < INDEXACCESS_LAST; readAccess++)
			{
				const char* readAccessName = getIndexAccessTypeName((IndexAccessType)readAccess);
				for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(s_shaderTypes); shaderTypeNdx++)
				{
					ShaderType	shaderType		= s_shaderTypes[shaderTypeNdx];
					const char*	shaderTypeName	= getShaderTypeName(shaderType);
					string		name			= string(getDataTypeName(varType)) + "_" + readAccessName + "_read_" + shaderTypeName;
					string		desc			= string("Uniform array with ") + readAccessName + " read in " + shaderTypeName + " shader.";
					bool isVertexCase = ((ShaderType)shaderType == SHADERTYPE_VERTEX);
					de::MovePtr<ShaderIndexingCase> testCase(createUniformArrayCase(m_testCtx, name, desc, isVertexCase, varType, (IndexAccessType)readAccess));
					uniformGroup->addChild(testCase.release());
				}
			}
		}

		addChild(uniformGroup.release());
	}

	// Temporary array access cases.
	{
		de::MovePtr<TestCaseGroup> tmpGroup(new TestCaseGroup(m_testCtx, "tmp_array", "Temporary array access tests."));

		for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(s_floatAndVecTypes); typeNdx++)
		{
			DataType varType = s_floatAndVecTypes[typeNdx];
			for (int writeAccess = 0; writeAccess < INDEXACCESS_LAST; writeAccess++)
			{
				for (int readAccess = 0; readAccess < INDEXACCESS_LAST; readAccess++)
				{
					const char* writeAccessName = getIndexAccessTypeName((IndexAccessType)writeAccess);
					const char* readAccessName = getIndexAccessTypeName((IndexAccessType)readAccess);

					for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(s_shaderTypes); shaderTypeNdx++)
					{
						ShaderType	shaderType		= s_shaderTypes[shaderTypeNdx];
						const char* shaderTypeName	= getShaderTypeName(shaderType);
						string		name			= string(getDataTypeName(varType)) + "_" + writeAccessName + "_write_" + readAccessName + "_read_" + shaderTypeName;
						string		desc			= string("Temporary array with ") + writeAccessName + " write and " + readAccessName + " read in " + shaderTypeName + " shader.";
						bool		isVertexCase	= ((ShaderType)shaderType == SHADERTYPE_VERTEX);
						de::MovePtr<ShaderIndexingCase> testCase(createTmpArrayCase(m_testCtx, name, desc, isVertexCase, varType, (IndexAccessType)writeAccess, (IndexAccessType)readAccess));
						tmpGroup->addChild(testCase.release());
					}
				}
			}
		}

		addChild(tmpGroup.release());
	}

	// Vector indexing with subscripts.
	{
		de::MovePtr<TestCaseGroup> vecGroup(new TestCaseGroup(m_testCtx, "vector_subscript", "Vector subscript indexing."));

		static const DataType s_vectorTypes[] =
		{
			TYPE_FLOAT_VEC2,
			TYPE_FLOAT_VEC3,
			TYPE_FLOAT_VEC4
		};

		for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(s_vectorTypes); typeNdx++)
		{
			DataType varType = s_vectorTypes[typeNdx];
			for (int writeAccess = 0; writeAccess < VECTORACCESS_LAST; writeAccess++)
			{
				for (int readAccess = 0; readAccess < VECTORACCESS_LAST; readAccess++)
				{
					const char* writeAccessName = getVectorAccessTypeName((VectorAccessType)writeAccess);
					const char* readAccessName = getVectorAccessTypeName((VectorAccessType)readAccess);

					for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(s_shaderTypes); shaderTypeNdx++)
					{
						ShaderType	shaderType		= s_shaderTypes[shaderTypeNdx];
						const char* shaderTypeName	= getShaderTypeName(shaderType);
						string		name			= string(getDataTypeName(varType)) + "_" + writeAccessName + "_write_" + readAccessName + "_read_" + shaderTypeName;
						string		desc			= string("Vector subscript access with ") + writeAccessName + " write and " + readAccessName + " read in " + shaderTypeName + " shader.";
						bool		isVertexCase	= ((ShaderType)shaderType == SHADERTYPE_VERTEX);
						de::MovePtr<ShaderIndexingCase> testCase(createVectorSubscriptCase(m_testCtx, name.c_str(), desc.c_str(), isVertexCase, varType, (VectorAccessType)writeAccess, (VectorAccessType)readAccess));
						vecGroup->addChild(testCase.release());
					}
				}
			}
		}

		addChild(vecGroup.release());
	}

	// Matrix indexing with subscripts.
	{
		de::MovePtr<TestCaseGroup> matGroup(new TestCaseGroup(m_testCtx, "matrix_subscript", "Matrix subscript indexing."));

		static const DataType s_matrixTypes[] =
		{
			TYPE_FLOAT_MAT2,
			TYPE_FLOAT_MAT2X3,
			TYPE_FLOAT_MAT2X4,
			TYPE_FLOAT_MAT3X2,
			TYPE_FLOAT_MAT3,
			TYPE_FLOAT_MAT3X4,
			TYPE_FLOAT_MAT4X2,
			TYPE_FLOAT_MAT4X3,
			TYPE_FLOAT_MAT4
		};

		for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(s_matrixTypes); typeNdx++)
		{
			DataType varType = s_matrixTypes[typeNdx];
			for (int writeAccess = 0; writeAccess < INDEXACCESS_LAST; writeAccess++)
			{
				for (int readAccess = 0; readAccess < INDEXACCESS_LAST; readAccess++)
				{
					const char* writeAccessName = getIndexAccessTypeName((IndexAccessType)writeAccess);
					const char* readAccessName = getIndexAccessTypeName((IndexAccessType)readAccess);

					for (int shaderTypeNdx = 0; shaderTypeNdx < DE_LENGTH_OF_ARRAY(s_shaderTypes); shaderTypeNdx++)
					{
						ShaderType	shaderType		= s_shaderTypes[shaderTypeNdx];
						const char* shaderTypeName	= getShaderTypeName(shaderType);
						string		name			= string(getDataTypeName(varType)) + "_" + writeAccessName + "_write_" + readAccessName + "_read_" + shaderTypeName;
						string		desc			= string("Vector subscript access with ") + writeAccessName + " write and " + readAccessName + " read in " + shaderTypeName + " shader.";
						bool		isVertexCase	= ((ShaderType)shaderType == SHADERTYPE_VERTEX);
						de::MovePtr<ShaderIndexingCase> testCase(createMatrixSubscriptCase(m_testCtx, name.c_str(), desc.c_str(), isVertexCase, varType, (IndexAccessType)writeAccess, (IndexAccessType)readAccess));
						matGroup->addChild(testCase.release());
					}
				}
			}
		}

		addChild(matGroup.release());
	}
}

} // anonymous

tcu::TestCaseGroup* createIndexingTests (tcu::TestContext& testCtx)
{
	return new ShaderIndexingTests(testCtx);
}

} // sr
} // vkt
