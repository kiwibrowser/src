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
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file  glcAggressiveShaderOptimizationsTests.cpp
 * \brief Conformance tests that checks if shader optimizations are not
 *		  overly aggressive. This is done by compering result of complex
 *		  trigonometric functions aproximation to shader buil
 */ /*-------------------------------------------------------------------*/

#include "glcAggressiveShaderOptimizationsTests.hpp"
#include "deSharedPtr.hpp"
#include "glsShaderExecUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "glwFunctions.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

using namespace glw;

namespace glcts
{

enum ShaderType
{
	TEST_VERTEX_SHADER,
	TEST_FRAGMENT_SHADER
};

struct TrigonometryCaseData
{
	const char* testedFunction;
	const char* testedType;
	const char* colorComponents;
	ShaderType  shaderType;
};

class TrigonometryTestCase : public deqp::TestCase
{
public:
	TrigonometryTestCase(deqp::Context& context, const std::string& name, const TrigonometryCaseData& data);
	virtual ~TrigonometryTestCase();

	IterateResult iterate(void);

protected:
	glu::ProgramSources prepareSineSources(bool useBuiltin);
	glu::ProgramSources prepareCosineSources(bool useBuiltin);

	void renderAndGrabSurface(glu::ProgramSources sources, tcu::Surface& result) const;

private:
	ShaderType  m_shaderType;
	const char* m_testedFunction;
	std::map<std::string, std::string> m_specializationMap;
};

TrigonometryTestCase::TrigonometryTestCase(deqp::Context& context, const std::string& name,
										   const TrigonometryCaseData& data)
	: deqp::TestCase(context, name.c_str(), ""), m_shaderType(data.shaderType), m_testedFunction(data.testedFunction)
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(contextType);

	m_specializationMap["VERSION"]			= glu::getGLSLVersionDeclaration(glslVersion);
	m_specializationMap["TYPE"]				= data.testedType;
	m_specializationMap["COLOR_COMPONENTS"] = data.colorComponents;

	if (glu::contextSupports(contextType, glu::ApiType::es(3, 0)) || glu::isContextTypeGLCore(contextType))
	{
		m_specializationMap["IN"]						= "in";
		m_specializationMap["OUT"]						= "out";
		m_specializationMap["ATTRIBUTE"]				= "in";
		m_specializationMap["FS_OUT_COLOR_NAME"]		= "fragColor";
		m_specializationMap["FS_OUT_COLOR_DECLARATION"] = "out vec4 fragColor;";
	}
	else
	{
		m_specializationMap["IN"]						= "varying";
		m_specializationMap["OUT"]						= "varying";
		m_specializationMap["ATTRIBUTE"]				= "attribute";
		m_specializationMap["FS_OUT_COLOR_NAME"]		= "gl_FragColor";
		m_specializationMap["FS_OUT_COLOR_DECLARATION"] = "";
	}
}

TrigonometryTestCase::~TrigonometryTestCase()
{
}

glu::ProgramSources TrigonometryTestCase::prepareSineSources(bool useBuiltinSin)
{
	const char* vsDefault = "${VERSION}\n"
							"${ATTRIBUTE} highp vec2 position;\n"
							"${ATTRIBUTE} highp vec3 baseColor;\n"
							"${OUT} vec4 color;\n"
							"void main (void) {\n"
							"  color = vec4(baseColor, 1.0);\n"
							"  gl_Position = vec4(position, 0.0, 1.0);\n"
							"}\n";

	const char* vsCalculateSin = "${VERSION}\n"
								 "${ATTRIBUTE} highp vec2 position;\n"
								 "${ATTRIBUTE} highp vec3 baseColor;\n"
								 "${OUT} vec4 color;\n"
								 "${SIN_FUNCTION_DEFINITION_VS}\n"
								 "void main (void) {\n"
								 "  const float M_2PI = 2.0 * 3.14159265358979323846;\n"
								 "  ${TYPE} c = baseColor.${COLOR_COMPONENTS} * M_2PI;\n"
								 "  ${TYPE} sin_c = ${SIN_FUNCTION_NAME}(c);\n"
								 "  \n"
								 "  color = vec4(0.0, 0.0, 0.0, 1.0);\n"
								 "  color.${COLOR_COMPONENTS} = sin_c * 0.5 + 0.5;\n"
								 "  gl_Position = vec4(position, 0.0, 1.0);\n"
								 "}\n";

	const char* fsDefault = "${VERSION}\n"
							"precision mediump float;\n"
							"${IN} vec4 color;\n"
							"${FS_OUT_COLOR_DECLARATION}\n"
							"void main (void) {\n"
							"  ${FS_OUT_COLOR_NAME} = color;\n"
							"}\n";

	const char* fsCalculateSin = "${VERSION}\n"
								 "precision mediump float;\n"
								 "${IN} vec4 color;\n"
								 "${FS_OUT_COLOR_DECLARATION}\n\n"
								 "${SIN_FUNCTION_DEFINITION_FS}\n"
								 "void main (void) {\n"
								 "  const float M_2PI = 2.0 * 3.14159265358979323846;\n"
								 "  ${TYPE} c = color.${COLOR_COMPONENTS};\n"
								 "  ${TYPE} sin_c = ${SIN_FUNCTION_NAME}(c * M_2PI);\n"
								 "  \n"
								 "  ${FS_OUT_COLOR_NAME} =vec4(0.0, 0.0, 0.0, 1.0);\n"
								 "  ${FS_OUT_COLOR_NAME}.${COLOR_COMPONENTS} = sin_c * 0.5 + 0.5;\n"
								 "}\n";

	std::string vsTemplate;
	std::string fsTemplate;

	if (m_shaderType == TEST_VERTEX_SHADER)
	{
		vsTemplate = vsCalculateSin;
		fsTemplate = fsDefault;
	}
	else
	{
		vsTemplate = vsDefault;
		fsTemplate = fsCalculateSin;
	}

	if (useBuiltinSin)
	{
		m_specializationMap["SIN_FUNCTION_NAME"]		  = "sin";
		m_specializationMap["SIN_FUNCTION_DEFINITION_VS"] = "";
		m_specializationMap["SIN_FUNCTION_DEFINITION_FS"] = "";
	}
	else
	{
		std::string sinFunctionDefinitionVS = "${TYPE} ${SIN_FUNCTION_NAME}(${TYPE} c) {\n"
											  "  ${TYPE} sin_c = ${TYPE}(0.0);\n"
											  "  float sign = 1.0;\n"
											  "  float fact;\n"
											  "  float fact_of;\n"
											  "  \n"
											  "  // Taylors series expansion for sin \n"
											  "  for(int i = 0; i < 12; i++) {\n"
											  "    fact = 1.0;\n"
											  "    for(int j = 2; j <= 23; j++)\n"
											  "      if (j <= 2 * i + 1)\n"
											  "        fact *= float(j);\n"
											  "    \n"
											  "    sin_c += sign * pow(c, ${TYPE}(2.0 * float(i) + 1.0)) / fact;\n"
											  "    sign *= -1.0;\n"
											  "  }\n"
											  "  return sin_c;\n"
											  "}";
		std::string sinFunctionDefinitionFS = "float lerpHelper(float a, float b, float weight) {\n"
											  "  return a + (b - a) * weight;\n"
											  "}\n"
											  "float sinLerpHelper(int index, float weight) {\n"
											  "  float sArray[17];\n"
											  "  sArray[0] = 0.0;\n"
											  "  sArray[1] = 0.382683;\n"
											  "  sArray[2] = 0.707107;\n"
											  "  sArray[3] = 0.92388;\n"
											  "  sArray[4] = 1.0;\n"
											  "  sArray[5] = 0.92388;\n"
											  "  sArray[6] = 0.707107;\n"
											  "  sArray[7] = 0.382683;\n"
											  "  sArray[8] = 0.0;\n"
											  "  sArray[9] = -0.382683;\n"
											  "  sArray[10] = -0.707107;\n"
											  "  sArray[11] = -0.92388;\n"
											  "  sArray[12] = -1.0;\n"
											  "  sArray[13] = -0.923879;\n"
											  "  sArray[14] = -0.707107;\n"
											  "  sArray[15] = -0.382683;\n"
											  "  sArray[16] = 0.0;\n"
											  "  \n"
											  "  if (index == 0)\n"
											  "    return lerpHelper(sArray[0], sArray[1], weight);\n"
											  "  if (index == 1)\n"
											  "    return lerpHelper(sArray[1], sArray[2], weight);\n"
											  "  if (index == 2)\n"
											  "    return lerpHelper(sArray[2], sArray[3], weight);\n"
											  "  if (index == 3)\n"
											  "    return lerpHelper(sArray[3], sArray[4], weight);\n"
											  "  if (index == 4)\n"
											  "    return lerpHelper(sArray[4], sArray[5], weight);\n"
											  "  if (index == 5)\n"
											  "    return lerpHelper(sArray[5], sArray[6], weight);\n"
											  "  if (index == 6)\n"
											  "    return lerpHelper(sArray[6], sArray[7], weight);\n"
											  "  if (index == 7)\n"
											  "    return lerpHelper(sArray[7], sArray[8], weight);\n"
											  "  if (index == 8)\n"
											  "    return lerpHelper(sArray[8], sArray[9], weight);\n"
											  "  if (index == 9)\n"
											  "    return lerpHelper(sArray[9], sArray[10], weight);\n"
											  "  if (index == 10)\n"
											  "    return lerpHelper(sArray[10], sArray[11], weight);\n"
											  "  if (index == 11)\n"
											  "    return lerpHelper(sArray[11], sArray[12], weight);\n"
											  "  if (index == 12)\n"
											  "    return lerpHelper(sArray[12], sArray[13], weight);\n"
											  "  if (index == 13)\n"
											  "    return lerpHelper(sArray[13], sArray[14], weight);\n"
											  "  if (index == 14)\n"
											  "    return lerpHelper(sArray[14], sArray[15], weight);\n"
											  "  if (index == 15)\n"
											  "    return lerpHelper(sArray[15], sArray[16], weight);\n"
											  "  return sArray[16];\n"
											  "}\n"
											  "${TYPE} ${SIN_FUNCTION_NAME}(${TYPE} c) {\n"
											  "  ${TYPE} arrVal = c * 2.546478971;\n"
											  "  ${TYPE} weight = arrVal - floor(arrVal);\n"
											  "  ${TYPE} sin_c = ${TYPE}(0.0);\n"
											  "  ${INTERPOLATE_SIN}"
											  "  return sin_c;\n"
											  "}";

		if (m_specializationMap["TYPE"] == "float")
		{
			m_specializationMap["INTERPOLATE_SIN"] = "\n"
													 "  int index = int(floor(arrVal));\n"
													 "  sin_c = sinLerpHelper(index, weight);\n";
		}
		else if (m_specializationMap["TYPE"] == "vec2")
		{
			m_specializationMap["INTERPOLATE_SIN"] = "\n"
													 "  int indexX = int(floor(arrVal.x));\n"
													 "  sin_c.x = sinLerpHelper(indexX, weight.x);\n"
													 "  int indexY = int(floor(arrVal.y));\n"
													 "  sin_c.y = sinLerpHelper(indexY, weight.y);\n";
		}
		else if (m_specializationMap["TYPE"] == "vec3")
		{
			m_specializationMap["INTERPOLATE_SIN"] = "\n"
													 "  int indexX = int(floor(arrVal.x));\n"
													 "  sin_c.x = sinLerpHelper(indexX, weight.x);\n"
													 "  int indexY = int(floor(arrVal.y));\n"
													 "  sin_c.y = sinLerpHelper(indexY, weight.y);\n"
													 "  int indexZ = int(floor(arrVal.z));\n"
													 "  sin_c.z = sinLerpHelper(indexZ, weight.z);\n";
		}

		m_specializationMap["SIN_FUNCTION_NAME"] = "calculateSin";
		m_specializationMap["SIN_FUNCTION_DEFINITION_VS"] =
			tcu::StringTemplate(sinFunctionDefinitionVS).specialize(m_specializationMap);
		m_specializationMap["SIN_FUNCTION_DEFINITION_FS"] =
			tcu::StringTemplate(sinFunctionDefinitionFS).specialize(m_specializationMap);
	}

	// Specialize shader templates
	vsTemplate = tcu::StringTemplate(vsTemplate).specialize(m_specializationMap);
	fsTemplate = tcu::StringTemplate(fsTemplate).specialize(m_specializationMap);
	return glu::makeVtxFragSources(vsTemplate.c_str(), fsTemplate.c_str());
}

glu::ProgramSources TrigonometryTestCase::prepareCosineSources(bool useBuiltinCos)
{
	const char* vsDefault = "${VERSION}\n"
							"${ATTRIBUTE} highp vec2 position;\n"
							"${ATTRIBUTE} highp vec3 baseColor;\n"
							"${OUT} vec4 color;\n"
							"void main (void) {\n"
							"  color = vec4(baseColor, 1.0);\n"
							"  gl_Position = vec4(position, 0.0, 1.0);\n"
							"}\n";

	const char* vsCalculateCos = "${VERSION}\n"
								 "${ATTRIBUTE} highp vec2 position;\n"
								 "${ATTRIBUTE} highp vec3 baseColor;\n"
								 "${OUT} vec4 color;\n"
								 "${COS_FUNCTION_DEFINITION_VS}\n"
								 "void main (void) {\n"
								 "  const float M_2PI = 2.0 * 3.14159265358979323846;\n"
								 "  ${TYPE} c = baseColor.${COLOR_COMPONENTS};\n"
								 "  ${TYPE} cos_c = ${COS_FUNCTION_NAME}(c * M_2PI);\n"
								 "  \n"
								 "  color = vec4(0.0, 0.0, 0.0, 1.0);\n"
								 "  color.${COLOR_COMPONENTS} = cos_c * 0.5 + 0.5;\n"
								 "  gl_Position = vec4(position, 0.0, 1.0);\n"
								 "}\n";

	const char* fsDefault = "${VERSION}\n"
							"precision mediump float;\n"
							"${IN} vec4 color;\n"
							"${FS_OUT_COLOR_DECLARATION}\n"
							"void main (void) {\n"
							"  ${FS_OUT_COLOR_NAME} = color;\n"
							"}\n";

	const char* fsCalculateCos = "${VERSION}\n"
								 "precision mediump float;\n"
								 "${IN} vec4 color;\n"
								 "${FS_OUT_COLOR_DECLARATION}\n\n"
								 "// function definitions \n"
								 "${COS_FUNCTION_DEFINITION_FS}\n"
								 "${TYPE} preprocessColor(${TYPE} c) {\n"
								 "  ${PREPROCESS_COLOR};\n"
								 "  return c;\n"
								 "}\n\n"
								 "void main (void) {\n"
								 "  const float M_2PI = 2.0 * 3.14159265358979323846;\n"
								 "  ${TYPE} c = preprocessColor(color.${COLOR_COMPONENTS});\n"
								 "  ${TYPE} cos_c = ${COS_FUNCTION_NAME}(c * M_2PI);\n"
								 "  \n"
								 "  ${FS_OUT_COLOR_NAME} = vec4(0.0, 0.0, 0.0, 1.0);\n"
								 "  ${FS_OUT_COLOR_NAME}.${COLOR_COMPONENTS} = cos_c * 0.5 + 0.5;\n"
								 "}\n";

	std::string vsTemplate;
	std::string fsTemplate;

	if (m_shaderType == TEST_VERTEX_SHADER)
	{
		vsTemplate = vsCalculateCos;
		fsTemplate = fsDefault;
	}
	else
	{
		vsTemplate = vsDefault;
		fsTemplate = fsCalculateCos;
	}

	if (useBuiltinCos)
	{
		m_specializationMap["PREPROCESS_COLOR"]			  = "";
		m_specializationMap["COS_FUNCTION_NAME"]		  = "cos";
		m_specializationMap["COS_FUNCTION_DEFINITION_VS"] = "";
		m_specializationMap["COS_FUNCTION_DEFINITION_FS"] = "";
	}
	else
	{
		std::string cosFunctionDefinitionVS = "${TYPE} ${COS_FUNCTION_NAME}(${TYPE} c) {\n"
											  "  ${TYPE} cos_c = ${TYPE}(1.0);\n"
											  "  float sign = -1.0;\n"
											  "  float fact =  1.0;\n"
											  "  \n"
											  "  for(int i = 2; i <= 20; i += 2) {\n"
											  "    fact  *= float(i)*float(i-1);\n"
											  "    cos_c += sign*pow(c, ${TYPE}(float(i)))/fact;\n"
											  "    sign = -sign;\n"
											  "  }\n"
											  "  return cos_c;\n"
											  "}";
		std::string cosFunctionDefinitionFS = "${TYPE} ${COS_FUNCTION_NAME}(${TYPE} c) {\n"
											  "  ${TYPE} cos_c = ${TYPE}(-1.0);\n"
											  "  float sign      = 1.0;\n"
											  "  float fact_even = 1.0;\n"
											  "  float fact_odd  = 1.0;\n"
											  "  ${TYPE} sum;\n"
											  "  ${TYPE} exp;\n"
											  "  \n"
											  "  for(int i = 2; i <= 10; i += 2) {\n"
											  "    fact_even *= float(i);\n"
											  "    fact_odd  *= float(i-1);\n"
											  "    exp = ${TYPE}(float(i/2));\n"
											  "    sum = sign * pow(abs(c), exp)/fact_even;\n"
											  "    cos_c += pow(abs(c), exp)*(sum/fact_odd);\n"
											  "    sign = -sign;\n"
											  "  }\n"
											  "  return cos_c;\n"
											  "}";

		m_specializationMap["PREPROCESS_COLOR"]  = "c = (fract(abs(c)) - 0.5)";
		m_specializationMap["COS_FUNCTION_NAME"] = "calculateCos";
		m_specializationMap["COS_FUNCTION_DEFINITION_VS"] =
			tcu::StringTemplate(cosFunctionDefinitionVS).specialize(m_specializationMap);
		m_specializationMap["COS_FUNCTION_DEFINITION_FS"] =
			tcu::StringTemplate(cosFunctionDefinitionFS).specialize(m_specializationMap);
	}

	// Specialize shader templates
	vsTemplate = tcu::StringTemplate(vsTemplate).specialize(m_specializationMap);
	fsTemplate = tcu::StringTemplate(fsTemplate).specialize(m_specializationMap);
	return glu::makeVtxFragSources(vsTemplate.c_str(), fsTemplate.c_str());
}

void TrigonometryTestCase::renderAndGrabSurface(glu::ProgramSources sources, tcu::Surface& result) const
{
	static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };
	static const float	positions[]   = { -1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0 };
	static const float	baseColors[]  = { 1.0, 0.0, 0.25, 0.75, 0.25, 1.0, 0.0, 1.0, 0.75, 0.25, 0.5, 0.0 };

	glu::RenderContext&   renderContext = m_context.getRenderContext();
	const glw::Functions& gl			= renderContext.getFunctions();
	glu::ShaderProgram	testProgram(renderContext, sources);
	if (!testProgram.isOk())
	{
		m_testCtx.getLog() << testProgram;
		TCU_FAIL("Test program compilation failed");
	}

	// Render
	gl.useProgram(testProgram.getProgram());
	const glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("position", 2, 4, 0, positions),
													 glu::va::Float("baseColor", 3, 4, 0, baseColors) };
	glu::draw(renderContext, testProgram.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices));

	// Grab surface
	glu::readPixels(renderContext, 0, 0, result.getAccess());
}

tcu::TestNode::IterateResult TrigonometryTestCase::iterate(void)
{
	glu::RenderContext&   renderContext = m_context.getRenderContext();
	const glw::Functions& gl			= renderContext.getFunctions();

	int  renderWidth  = 64;
	int  renderHeight = 64;
	bool isSin		  = std::string(m_testedFunction) == "sin";

	gl.viewport(0, 0, renderWidth, renderHeight);

	// Use program that will call trigonometric function aproximation
	tcu::Surface testSurface(renderWidth, renderHeight);
	if (isSin)
		renderAndGrabSurface(prepareSineSources(false), testSurface);
	else
		renderAndGrabSurface(prepareCosineSources(false), testSurface);

	// Use reference program that will call builtin function
	tcu::Surface referenceSurface(renderWidth, renderHeight);
	if (isSin)
		renderAndGrabSurface(prepareSineSources(true), referenceSurface);
	else
		renderAndGrabSurface(prepareCosineSources(true), referenceSurface);

	// Compare surfaces
	qpTestResult testResult = QP_TEST_RESULT_FAIL;
	if (tcu::fuzzyCompare(m_testCtx.getLog(), "Result", "Image comparison result", referenceSurface, testSurface, 0.05f,
						  tcu::COMPARE_LOG_RESULT))
		testResult = QP_TEST_RESULT_PASS;

	m_testCtx.setTestResult(testResult, qpGetTestResultName(testResult));
	return STOP;
}

AggressiveShaderOptimizationsTests::AggressiveShaderOptimizationsTests(deqp::Context& context)
	: TestCaseGroup(context, "aggressive_optimizations", "checks if shader optimizations are not overly aggressive")
{
}

AggressiveShaderOptimizationsTests::~AggressiveShaderOptimizationsTests()
{
}

void AggressiveShaderOptimizationsTests::init(void)
{
	TrigonometryCaseData trigonometryCases[] = {
		{ "sin", "float", "r", TEST_VERTEX_SHADER },  { "sin", "float", "r", TEST_FRAGMENT_SHADER },
		{ "sin", "vec2", "rg", TEST_VERTEX_SHADER },  { "sin", "vec2", "rg", TEST_FRAGMENT_SHADER },
		{ "sin", "vec3", "rgb", TEST_VERTEX_SHADER }, { "sin", "vec3", "rgb", TEST_FRAGMENT_SHADER },
		{ "cos", "float", "r", TEST_VERTEX_SHADER },  { "cos", "float", "r", TEST_FRAGMENT_SHADER },
		{ "cos", "vec2", "rg", TEST_VERTEX_SHADER },  { "cos", "vec2", "rg", TEST_FRAGMENT_SHADER },
		{ "cos", "vec3", "rgb", TEST_VERTEX_SHADER }, { "cos", "vec3", "rgb", TEST_FRAGMENT_SHADER },
	};

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(trigonometryCases); ++i)
	{
		const TrigonometryCaseData& tcd		   = trigonometryCases[i];
		std::string					shaderType = (tcd.shaderType == TEST_VERTEX_SHADER) ? "_vert" : "_frag";
		std::string					name	   = std::string(tcd.testedFunction) + "_" + tcd.testedType + shaderType;
		addChild(new TrigonometryTestCase(m_context, name, tcd));
	}
}

} // glcts namespace
