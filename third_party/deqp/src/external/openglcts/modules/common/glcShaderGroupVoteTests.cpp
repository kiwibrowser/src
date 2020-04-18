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
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/ /*!
* \file  glcShaderGroupVoteTests.cpp
* \brief Conformance tests for the ARB_shader_group_vote functionality.
*/ /*-------------------------------------------------------------------*/

#include "glcShaderGroupVoteTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

using namespace glw;

namespace glcts
{

// Helper structure that wpraps workgroup size
struct WorkGroupSize
{
	WorkGroupSize(deqp::Context& context)
	{
		width  = 16;
		height = 16;
		if (glu::isContextTypeES(context.getRenderContext().getType()))
			height = 8;
	}

	GLsizei width;
	GLsizei height;
};

ShaderGroupVoteTestCaseBase::ComputeShader::ComputeShader(const std::string& name, const std::string& shader)
	: m_name(name), m_shader(shader), m_program(NULL), m_compileOnly(true)
{
}

ShaderGroupVoteTestCaseBase::ComputeShader::ComputeShader(const std::string& name, const std::string& shader,
														  const tcu::IVec4& desiredColor)
	: m_name(name), m_shader(shader), m_program(NULL), m_desiredColor(desiredColor), m_compileOnly(false)
{
}

ShaderGroupVoteTestCaseBase::ComputeShader::~ComputeShader()
{
	if (m_program)
	{
		delete m_program;
	}
}

void ShaderGroupVoteTestCaseBase::ComputeShader::create(deqp::Context& context)
{
	glu::ProgramSources sourcesCompute;
	sourcesCompute.sources[glu::SHADERTYPE_COMPUTE].push_back(m_shader);
	m_program = new glu::ShaderProgram(context.getRenderContext(), sourcesCompute);

	if (!m_program->isOk())
	{
		context.getTestContext().getLog()
			<< tcu::TestLog::Message << m_shader << m_program->getShaderInfo(glu::SHADERTYPE_COMPUTE).infoLog
			<< m_program->getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		TCU_FAIL("Shader compilation failed");
	}
}

void ShaderGroupVoteTestCaseBase::ComputeShader::execute(deqp::Context& context)
{
	if (m_compileOnly)
	{
		return;
	}

	const glw::Functions& gl = context.getRenderContext().getFunctions();
	const glu::Texture	outputTexture(context.getRenderContext());
	const WorkGroupSize   renderSize(context);

	gl.clearColor(0.5f, 0.5f, 0.5f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.useProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "useProgram failed");

	// output image
	gl.bindTexture(GL_TEXTURE_2D, *outputTexture);
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, renderSize.width, renderSize.height);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading image data failed");

	// bind image
	gl.bindImageTexture(2, *outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindImageTexture failed");

	// dispatch compute
	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "dispatchCompute failed");

	glu::GLSLVersion glslVersion		= glu::getContextTypeGLSLVersion(context.getRenderContext().getType());
	const char*		 versionDeclaration = glu::getGLSLVersionDeclaration(glslVersion);

	// render output texture
	std::string vs = versionDeclaration;
	vs += "\n"
		  "in highp vec2 position;\n"
		  "in highp vec2 inTexcoord;\n"
		  "out highp vec2 texcoord;\n"
		  "void main()\n"
		  "{\n"
		  "	texcoord = inTexcoord;\n"
		  "	gl_Position = vec4(position, 0.0, 1.0);\n"
		  "}\n";

	std::string fs = versionDeclaration;
	fs += "\n"
		  "uniform highp sampler2D sampler;\n"
		  "in highp vec2 texcoord;\n"
		  "out highp vec4 color;\n"
		  "void main()\n"
		  "{\n"
		  "	color = texture(sampler, texcoord);\n"
		  "}\n";

	glu::ProgramSources sources;
	sources.sources[glu::SHADERTYPE_VERTEX].push_back(vs);
	sources.sources[glu::SHADERTYPE_FRAGMENT].push_back(fs);
	glu::ShaderProgram renderShader(context.getRenderContext(), sources);

	if (!m_program->isOk())
	{
		TCU_FAIL("Shader compilation failed");
	}

	gl.bindTexture(GL_TEXTURE_2D, *outputTexture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri failed");

	gl.useProgram(renderShader.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "useProgram failed");

	gl.uniform1i(gl.getUniformLocation(renderShader.getProgram(), "sampler"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i failed");

	deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	float const position[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };

	float const texCoord[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };

	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("position", 2, 4, 0, position),
											   glu::va::Float("inTexcoord", 2, 4, 0, texCoord) };

	gl.viewport(0, 0, renderSize.width, renderSize.height);
	glu::draw(context.getRenderContext(), renderShader.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices));

	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::draw error");

	gl.flush();
}

void ShaderGroupVoteTestCaseBase::ComputeShader::validate(deqp::Context& context)
{
	if (m_compileOnly)
	{
		return;
	}

	bool		validationResult   = validateScreenPixels(context, m_desiredColor);
	std::string validationErrorMsg = "Validation failed for " + m_name + " test";

	TCU_CHECK_MSG(validationResult, validationErrorMsg.c_str());
}

bool ShaderGroupVoteTestCaseBase::ComputeShader::validateScreenPixels(deqp::Context& context, tcu::IVec4 desiredColor)
{
	const glw::Functions&	 gl		= context.getRenderContext().getFunctions();
	const WorkGroupSize		  renderSize(context);
	std::size_t				  totalSize = renderSize.width * renderSize.height * 4;
	std::vector<glw::GLubyte> pixels(totalSize, 128);

	// read pixels
	gl.readPixels(0, 0, renderSize.width, renderSize.height, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

	// compare pixels to desired color
	for (std::size_t i = 0; i < totalSize; i += 4)
	{
		if ((pixels[i + 0] != desiredColor.x()) || (pixels[i + 1] != desiredColor.y()) ||
			(pixels[i + 2] != desiredColor.z()))
			return false;
	}

	return true;
}

/** Constructor.
*
*  @param context Rendering context
*  @param name Test name
*  @param description Test description
*/
ShaderGroupVoteTestCaseBase::ShaderGroupVoteTestCaseBase(deqp::Context& context, ExtParameters& extParam,
														 const char* name, const char* description)
	: TestCaseBase(context, glcts::ExtParameters(glu::GLSL_VERSION_450, glcts::EXTENSIONTYPE_EXT), name, description)
	, m_extensionSupported(true)
{
	const WorkGroupSize workGroupSize(context);
	glu::ContextType contextType   = m_context.getRenderContext().getType();
	m_specializationMap["VERSION"] = glu::getGLSLVersionDeclaration(extParam.glslVersion);

	std::stringstream stream;
	stream << workGroupSize.width << " " << workGroupSize.height;
	stream >> m_specializationMap["SIZE_X"] >> m_specializationMap["SIZE_Y"];

	if (glu::contextSupports(contextType, glu::ApiType::core(4, 6)))
	{
		m_specializationMap["GROUP_VOTE_EXTENSION"] = "";
		m_specializationMap["EXT_TYPE"]				= "";
	}
	else
	{
		bool		isCoreGL	  = glu::isContextTypeGLCore(contextType);
		std::string extensionName = isCoreGL ? "GL_ARB_shader_group_vote" : "GL_EXT_shader_group_vote";
		m_extensionSupported	  = context.getContextInfo().isExtensionSupported(extensionName.c_str());
		std::stringstream extensionString;
		extensionString << "#extension " + extensionName + " : enable";

		m_specializationMap["GROUP_VOTE_EXTENSION"] = extensionString.str();
		m_specializationMap["EXT_TYPE"]				= isCoreGL ? "ARB" : "EXT";
	}
}

void ShaderGroupVoteTestCaseBase::init()
{
	if (m_extensionSupported)
	{
		for (ComputeShaderIter iter = m_shaders.begin(); iter != m_shaders.end(); ++iter)
		{
			(*iter)->create(m_context);
		}
	}
}

void ShaderGroupVoteTestCaseBase::deinit()
{
	for (ComputeShaderIter iter = m_shaders.begin(); iter != m_shaders.end(); ++iter)
	{
		delete (*iter);
	}
}

tcu::TestNode::IterateResult ShaderGroupVoteTestCaseBase::iterate()
{
	if (!m_extensionSupported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	for (ComputeShaderIter iter = m_shaders.begin(); iter != m_shaders.end(); ++iter)
	{
		(*iter)->execute(m_context);
		(*iter)->validate(m_context);
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderGroupVoteAvailabilityTestCase::ShaderGroupVoteAvailabilityTestCase(deqp::Context& context,
																		 ExtParameters& extParam)
	: ShaderGroupVoteTestCaseBase(context, extParam, "availability", "Implements ...")
{
	const char* shader = "${VERSION}\n"
						 "${GROUP_VOTE_EXTENSION}\n"
						 "layout(rgba8, binding = 2) writeonly uniform highp image2D destImage;\n"
						 "layout(local_size_x = ${SIZE_X}, local_size_y = ${SIZE_Y}) in;\n"
						 "void main (void)\n"
						 "{\n"
						 "	vec4 outColor = vec4(0.0);\n"
						 "	outColor.r = allInvocations${EXT_TYPE}(true) ? 1.0 : 0.0;\n"
						 "	outColor.g = anyInvocation${EXT_TYPE}(true) ? 1.0 : 0.0;\n"
						 "	outColor.b = allInvocationsEqual${EXT_TYPE}(true) ? 1.0 : 0.0;\n"
						 "	imageStore(destImage, ivec2(gl_GlobalInvocationID.xy), outColor);\n"
						 "}\n";

	m_shaders.push_back(new ComputeShader("availability", specializeShader(1, &shader)));
}

/** Constructor.
*
*  @param context Rendering context
*  @param name Test name
*  @param description Test description
*/
ShaderGroupVoteFunctionTestCaseBase::ShaderGroupVoteFunctionTestCaseBase(deqp::Context& context,
																		 ExtParameters& extParam, const char* name,
																		 const char* description)
	: ShaderGroupVoteTestCaseBase(context, extParam, name, description)
{
	m_shaderBase += "${VERSION}\n"
					"${GROUP_VOTE_EXTENSION}\n"
					"layout(rgba8, binding = 2) writeonly uniform highp image2D destImage;\n"
					"layout(local_size_x = ${SIZE_X}, local_size_y = ${SIZE_Y}) in;\n"
					"void main (void)\n"
					"{\n"
					"	bool result = ${FUNC}${EXT_TYPE}(${FUNC_PARAMETER});\n"
					"	vec4 outColor = vec4(vec3(result ? 1.0 : 0.0), 1.0);\n"
					"	imageStore(destImage, ivec2(gl_GlobalInvocationID.xy), outColor);\n"
					"}\n";
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderGroupVoteAllInvocationsTestCase::ShaderGroupVoteAllInvocationsTestCase(deqp::Context& context,
																			 ExtParameters& extParam)
	: ShaderGroupVoteFunctionTestCaseBase(context, extParam, "all_invocations", "Implements ...")
{
	const char* shaderBase				  = m_shaderBase.c_str();
	m_specializationMap["FUNC"]			  = "allInvocations";
	m_specializationMap["FUNC_PARAMETER"] = "true";

	m_shaders.push_back(
		new ComputeShader("allInvocationsARB", specializeShader(1, &shaderBase), tcu::IVec4(255, 255, 255, 255)));
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderGroupVoteAnyInvocationTestCase::ShaderGroupVoteAnyInvocationTestCase(deqp::Context& context,
																		   ExtParameters& extParam)
	: ShaderGroupVoteFunctionTestCaseBase(context, extParam, "any_invocation", "Implements ...")
{
	const char* shaderBase				  = m_shaderBase.c_str();
	m_specializationMap["FUNC"]			  = "anyInvocation";
	m_specializationMap["FUNC_PARAMETER"] = "false";

	m_shaders.push_back(
		new ComputeShader("anyInvocationARB", specializeShader(1, &shaderBase), tcu::IVec4(0, 0, 0, 255)));
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderGroupVoteAllInvocationsEqualTestCase::ShaderGroupVoteAllInvocationsEqualTestCase(deqp::Context& context,
																					   ExtParameters& extParam)
	: ShaderGroupVoteFunctionTestCaseBase(context, extParam, "all_invocations_equal", "Implements ...")
{
	const char* shaderBase				  = m_shaderBase.c_str();
	m_specializationMap["FUNC"]			  = "allInvocationsEqual";
	m_specializationMap["FUNC_PARAMETER"] = "true";
	m_shaders.push_back(
		new ComputeShader("allInvocationsEqualARB", specializeShader(1, &shaderBase), tcu::IVec4(255, 255, 255, 255)));

	m_specializationMap["FUNC"]			  = "allInvocationsEqual";
	m_specializationMap["FUNC_PARAMETER"] = "false";
	m_shaders.push_back(
		new ComputeShader("allInvocationsEqualARB", specializeShader(1, &shaderBase), tcu::IVec4(255, 255, 255, 255)));
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderGroupVoteWithVariablesTestCase::ShaderGroupVoteWithVariablesTestCase(deqp::Context& context,
																		   ExtParameters& extParam)
	: ShaderGroupVoteTestCaseBase(context, extParam, "invocations_with_variables", "Implements ...")
{
	const char* shaderBase = "${VERSION}\n"
							 "${GROUP_VOTE_EXTENSION}\n"
							 "layout(rgba8, binding = 2) writeonly uniform highp image2D destImage;\n"
							 "layout(local_size_x = ${SIZE_X}, local_size_y = ${SIZE_Y}) in;\n"
							 "void main (void)\n"
							 "{\n"
							 "	bool result = ${EXPRESSION};\n"
							 "	vec4 outColor = vec4(vec3(result ? 1.0 : 0.0), 1.0);\n"
							 "	imageStore(destImage, ivec2(gl_GlobalInvocationID.xy), outColor);\n"
							 "}\n";

	// first specialization EXPRESSION and then whole shader
	const char* expression1 = "allInvocations${EXT_TYPE}((gl_LocalInvocationIndex % 2u) == 1u) && "
							  "anyInvocation${EXT_TYPE}((gl_LocalInvocationIndex % 2u) == 0u) && "
							  "anyInvocation${EXT_TYPE}((gl_LocalInvocationIndex % 2u) == 1u)";
	m_specializationMap["EXPRESSION"] = specializeShader(1, &expression1);
	m_shaders.push_back(
		new ComputeShader("allInvocations", specializeShader(1, &shaderBase), tcu::IVec4(0, 0, 0, 255)));

	const char* expression2			  = "anyInvocation${EXT_TYPE}(gl_LocalInvocationIndex < 256u)";
	m_specializationMap["EXPRESSION"] = specializeShader(1, &expression2);
	m_shaders.push_back(
		new ComputeShader("anyInvocation", specializeShader(1, &shaderBase), tcu::IVec4(255, 255, 255, 255)));

	const char* expression3			  = "allInvocationsEqual${EXT_TYPE}(gl_WorkGroupID.x == 0u)";
	m_specializationMap["EXPRESSION"] = specializeShader(1, &expression3);
	m_shaders.push_back(
		new ComputeShader("anyInvocation", specializeShader(1, &shaderBase), tcu::IVec4(255, 255, 255, 255)));
}

/** Constructor.
*
*  @param context Rendering context.
*/
ShaderGroupVote::ShaderGroupVote(deqp::Context& context)
	: TestCaseGroup(context, "shader_group_vote",
					"Verify conformance of shader_group_vote functionality implementation")
{
}

/** Initializes the test group contents. */
void ShaderGroupVote::init()
{
	glu::GLSLVersion glslVersion = getContextTypeGLSLVersion(m_context.getRenderContext().getType());
	ExtParameters	extParam	= glcts::ExtParameters(glslVersion, glcts::EXTENSIONTYPE_EXT);

	addChild(new ShaderGroupVoteAvailabilityTestCase(m_context, extParam));
	addChild(new ShaderGroupVoteAllInvocationsTestCase(m_context, extParam));
	addChild(new ShaderGroupVoteAnyInvocationTestCase(m_context, extParam));
	addChild(new ShaderGroupVoteAllInvocationsEqualTestCase(m_context, extParam));
	addChild(new ShaderGroupVoteWithVariablesTestCase(m_context, extParam));
}
} /* glcts namespace */
