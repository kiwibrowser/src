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
 * \file  gl4cShaderAtomicCounterOpsTests.cpp
 * \brief Conformance tests for the ARB_shader_atomic_counter_ops functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cShaderAtomicCounterOpsTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"

#include <algorithm>
#include <sstream>
#include <string>

using namespace glw;

namespace gl4cts
{

ShaderAtomicCounterOpsTestBase::ShaderPipeline::ShaderPipeline(glu::ShaderType testedShader, AtomicOperation* newOp,
															   bool contextGL46)
	: m_program(NULL), m_programCompute(NULL), m_testedShader(testedShader), m_atomicOp(newOp)
{
	m_shaders[glu::SHADERTYPE_VERTEX] = "<version>\n"
										"<head>"
										"in highp vec2 inPosition;\n"
										"out highp vec3 vsPosition;\n"
										"out highp vec4 vsColor;\n"
										"void main()\n"
										"{\n"
										"	gl_Position = vec4(inPosition, 0.0, 1.0);\n"
										"	vsPosition = vec3(inPosition, 0.0);\n"
										"	vec4 outColor = vec4(1.0);\n"
										"<atomic_operation>"
										"	vsColor = outColor;\n"
										"}\n";

	m_shaders[glu::SHADERTYPE_FRAGMENT] = "<version>\n"
										  "<head>"
										  "in highp vec4 gsColor;\n"
										  "out highp vec4 fsColor;\n"
										  "void main()\n"
										  "{\n"
										  "	vec4 outColor = gsColor; \n"
										  "<atomic_operation>"
										  "	fsColor = outColor;\n"
										  "}\n";

	m_shaders[glu::SHADERTYPE_TESSELLATION_CONTROL] = "<version>\n"
													  "<head>"
													  "layout(vertices = 3) out;\n"
													  "in highp vec4 vsColor[];\n"
													  "in highp vec3 vsPosition[];\n"
													  "out highp vec3 tcsPosition[];\n"
													  "out highp vec4 tcsColor[];\n"
													  "void main()\n"
													  "{\n"
													  "	tcsPosition[gl_InvocationID] = vsPosition[gl_InvocationID];\n"
													  "	vec4 outColor = vsColor[gl_InvocationID];\n"
													  "<atomic_operation>"
													  "	tcsColor[gl_InvocationID] = outColor;\n"
													  "	gl_TessLevelInner[0] = 3;\n"
													  "	gl_TessLevelOuter[0] = 3;\n"
													  "	gl_TessLevelOuter[1] = 3;\n"
													  "	gl_TessLevelOuter[2] = 3;\n"
													  "}\n";

	m_shaders[glu::SHADERTYPE_TESSELLATION_EVALUATION] = "<version>\n"
														 "<head>"
														 "layout(triangles, equal_spacing, cw) in;\n"
														 "in highp vec3 tcsPosition[];\n"
														 "in highp vec4 tcsColor[];\n"
														 "out highp vec4 tesColor;\n"
														 "void main()\n"
														 "{\n"
														 "	vec3 p0 = gl_TessCoord.x * tcsPosition[0];\n"
														 "	vec3 p1 = gl_TessCoord.y * tcsPosition[1];\n"
														 "	vec3 p2 = gl_TessCoord.z * tcsPosition[2];\n"
														 "	vec4 outColor = tcsColor[0];\n"
														 "<atomic_operation>"
														 "	tesColor = outColor;\n"
														 "	gl_Position = vec4(normalize(p0 + p1 + p2), 1.0);\n"
														 "}\n";

	m_shaders[glu::SHADERTYPE_GEOMETRY] = "<version>\n"
										  "<head>"
										  "layout(triangles) in;\n"
										  "layout(triangle_strip, max_vertices = 3) out;\n"
										  "in highp vec4 tesColor[];\n"
										  "out highp vec4 gsColor;\n"
										  "void main()\n"
										  "{\n"
										  "	for (int i = 0; i<3; i++)\n"
										  "	{\n"
										  "		gl_Position = gl_in[i].gl_Position;\n"
										  "		vec4 outColor = tesColor[i];\n"
										  "<atomic_operation>"
										  "		gsColor = outColor;\n"
										  "		EmitVertex();\n"
										  "	}\n"
										  "	EndPrimitive();\n"
										  "}\n";

	m_shaders[glu::SHADERTYPE_COMPUTE] = "<version>\n"
										 "<head>"
										 "layout(rgba32f, binding = 2) writeonly uniform highp image2D destImage;\n"
										 "layout (local_size_x = 16, local_size_y = 16) in;\n"
										 "void main (void)\n"
										 "{\n"
										 "	vec4 outColor = vec4(1.0);\n"
										 "<atomic_operation>"
										 "	imageStore(destImage, ivec2(gl_GlobalInvocationID.xy), outColor);\n"
										 "}\n";

	// prepare shaders

	std::string		  postfix(contextGL46 ? "" : "ARB");
	std::stringstream atomicOperationStream;
	atomicOperationStream << "uint returned = " << m_atomicOp->getFunction() + postfix + "(counter, ";
	if (m_atomicOp->getCompareValue() != 0)
	{
		atomicOperationStream << m_atomicOp->getCompareValue();
		atomicOperationStream << "u, ";
	}
	atomicOperationStream << m_atomicOp->getParamValue();
	atomicOperationStream << "u);\n";
	atomicOperationStream << "uint after = atomicCounter(counter);\n";

	if (m_atomicOp->shouldTestReturnValue())
	{
		atomicOperationStream << "if(after == returned) outColor = vec4(0.0f);\n";
	}

	atomicOperationStream << "atomicCounterIncrement(calls);\n";

	std::string versionString;
	std::string headString;
	if (contextGL46)
	{
		versionString = "#version 460 core";
		headString	= "layout (binding=0) uniform atomic_uint counter;\n"
					 "layout (binding=1) uniform atomic_uint calls;\n";
	}
	else
	{
		versionString = "#version 450 core";
		headString	= "#extension GL_ARB_shader_atomic_counters: enable\n"
					 "#extension GL_ARB_shader_atomic_counter_ops: enable\n"
					 "layout (binding=0) uniform atomic_uint counter;\n"
					 "layout (binding=1) uniform atomic_uint calls;\n";
	}

	for (unsigned int i = 0; i < glu::SHADERTYPE_LAST; ++i)
	{
		prepareShader(m_shaders[i], "<version>", versionString);
		prepareShader(m_shaders[i], "<head>", i == testedShader ? headString : "");
		prepareShader(m_shaders[i], "<atomic_operation>", i == testedShader ? atomicOperationStream.str() : "");
	}
}

ShaderAtomicCounterOpsTestBase::ShaderPipeline::~ShaderPipeline()
{
	if (m_program)
	{
		delete m_program;
	}

	if (m_programCompute)
	{
		delete m_programCompute;
	}
}

void ShaderAtomicCounterOpsTestBase::ShaderPipeline::prepareShader(std::string& shader, const std::string& tag,
																   const std::string& value)
{
	size_t tagPos = shader.find(tag);

	if (tagPos != std::string::npos)
		shader.replace(tagPos, tag.length(), value);
}

void ShaderAtomicCounterOpsTestBase::ShaderPipeline::create(deqp::Context& context)
{
	glu::ProgramSources sources;
	for (unsigned int i = 0; i < glu::SHADERTYPE_COMPUTE; ++i)
	{
		if (!m_shaders[i].empty())
		{
			sources.sources[i].push_back(m_shaders[i]);
		}
	}
	m_program = new glu::ShaderProgram(context.getRenderContext(), sources);

	if (!m_program->isOk())
	{
		TCU_FAIL("Shader compilation failed");
	}

	glu::ProgramSources sourcesCompute;
	sourcesCompute.sources[glu::SHADERTYPE_COMPUTE].push_back(m_shaders[glu::SHADERTYPE_COMPUTE]);
	m_programCompute = new glu::ShaderProgram(context.getRenderContext(), sourcesCompute);

	if (!m_programCompute->isOk())
	{
		TCU_FAIL("Shader compilation failed");
	}
}

void ShaderAtomicCounterOpsTestBase::ShaderPipeline::use(deqp::Context& context)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();
	gl.useProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram failed");
}

void ShaderAtomicCounterOpsTestBase::ShaderPipeline::test(deqp::Context& context)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	gl.clearColor(0.5f, 0.5f, 0.5f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	if (m_testedShader == glu::SHADERTYPE_COMPUTE)
	{
		executeComputeShader(context);
	}
	else
	{
		renderQuad(context);
	}

	gl.flush();
}

void ShaderAtomicCounterOpsTestBase::ShaderPipeline::renderQuad(deqp::Context& context)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	float const position[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };

	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("inPosition", 2, 4, 0, position) };

	this->use(context);

	glu::PrimitiveList primitiveList = glu::pr::Patches(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices);

	glu::draw(context.getRenderContext(), this->getShaderProgram()->getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
			  vertexArrays, primitiveList);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::draw error");

	gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier() error");
}

void ShaderAtomicCounterOpsTestBase::ShaderPipeline::executeComputeShader(deqp::Context& context)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	const glu::Texture outputTexture(context.getRenderContext());

	gl.useProgram(m_programCompute->getProgram());

	// output image
	gl.bindTexture(GL_TEXTURE_2D, *outputTexture);
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, 16, 16);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading image data failed");

	// bind image
	gl.bindImageTexture(2, *outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Image setup failed");

	// dispatch compute
	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute() error");
	gl.memoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier() error");

	// render output texture

	std::string vs = "#version 450 core\n"
					 "in highp vec2 position;\n"
					 "in vec2 inTexcoord;\n"
					 "out vec2 texcoord;\n"
					 "void main()\n"
					 "{\n"
					 "	texcoord = inTexcoord;\n"
					 "	gl_Position = vec4(position, 0.0, 1.0);\n"
					 "}\n";

	std::string fs = "#version 450 core\n"
					 "uniform sampler2D sampler;\n"
					 "in vec2 texcoord;\n"
					 "out vec4 color;\n"
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

	glu::draw(context.getRenderContext(), renderShader.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices));

	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::draw error");
}

void ShaderAtomicCounterOpsTestBase::fillAtomicCounterBuffer(AtomicOperation* atomicOp)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLuint* dataPtr;

	// fill values buffer

	GLuint inputValue = atomicOp->getInputValue();

	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer() call failed.");

	dataPtr = (GLuint*)gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
										 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "mapBufferRange() call failed.");

	*dataPtr = inputValue;

	gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "unmapBuffer() call failed.");

	// fill calls buffer

	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterCallsBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer() call failed.");

	dataPtr = (GLuint*)gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
										 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "mapBufferRange() call failed.");

	*dataPtr = 0;

	gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "unmapBuffer() call failed.");
}

bool ShaderAtomicCounterOpsTestBase::checkAtomicCounterBuffer(AtomicOperation* atomicOp)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLuint* dataPtr;

	// get value

	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer() call failed.");

	dataPtr = (GLuint*)gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "mapBufferRange() call failed.");

	GLuint finalValue = *dataPtr;

	gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "unmapBuffer() call failed.");

	// get calls

	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterCallsBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer() call failed.");

	dataPtr = (GLuint*)gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "mapBufferRange() call failed.");

	GLuint numberOfCalls = *dataPtr;

	gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "unmapBuffer() call failed.");

	// validate

	GLuint expectedValue = atomicOp->getResult(numberOfCalls);

	return finalValue == expectedValue;
}

void ShaderAtomicCounterOpsTestBase::bindBuffers()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomicCounterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBufferBase() call failed.");

	gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, m_atomicCounterCallsBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBufferBase() call failed.");
}

bool ShaderAtomicCounterOpsTestBase::validateColor(tcu::Vec4 testedColor, tcu::Vec4 desiredColor)
{
	const float epsilon = 0.008f;
	return de::abs(testedColor.x() - desiredColor.x()) < epsilon &&
		   de::abs(testedColor.y() - desiredColor.y()) < epsilon &&
		   de::abs(testedColor.z() - desiredColor.z()) < epsilon;
}

bool ShaderAtomicCounterOpsTestBase::validateScreenPixels(tcu::Vec4 desiredColor, tcu::Vec4 ignoredColor)
{
	const glw::Functions&   gl			 = m_context.getRenderContext().getFunctions();
	const tcu::RenderTarget renderTarget = m_context.getRenderContext().getRenderTarget();
	tcu::IVec2				size(renderTarget.getWidth(), renderTarget.getHeight());

	glw::GLfloat* pixels = new glw::GLfloat[size.x() * size.y() * 4];

	// clear buffer
	for (int x = 0; x < size.x(); ++x)
	{
		for (int y = 0; y < size.y(); ++y)
		{
			int mappedPixelPosition = y * size.x() + x;

			pixels[mappedPixelPosition * 4 + 0] = -1.0f;
			pixels[mappedPixelPosition * 4 + 1] = -1.0f;
			pixels[mappedPixelPosition * 4 + 2] = -1.0f;
			pixels[mappedPixelPosition * 4 + 3] = -1.0f;
		}
	}

	// read pixels
	gl.readPixels(0, 0, size.x(), size.y(), GL_RGBA, GL_FLOAT, pixels);

	// validate pixels
	bool rendered = false;
	for (int x = 0; x < size.x(); ++x)
	{
		for (int y = 0; y < size.y(); ++y)
		{
			int mappedPixelPosition = y * size.x() + x;

			tcu::Vec4 color(pixels[mappedPixelPosition * 4 + 0], pixels[mappedPixelPosition * 4 + 1],
							pixels[mappedPixelPosition * 4 + 2], pixels[mappedPixelPosition * 4 + 3]);

			if (!validateColor(color, ignoredColor))
			{
				rendered = true;
				if (!validateColor(color, desiredColor))
				{
					delete[] pixels;
					return false;
				}
			}
		}
	}

	delete[] pixels;

	return rendered;
}

ShaderAtomicCounterOpsTestBase::ShaderAtomicCounterOpsTestBase(deqp::Context& context, const char* name,
															   const char* description)
	: TestCase(context, name, description), m_atomicCounterBuffer(0), m_atomicCounterCallsBuffer(0)
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	m_contextSupportsGL46		 = glu::contextSupports(contextType, glu::ApiType::core(4, 6));
}

void ShaderAtomicCounterOpsTestBase::init()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// generate atomic counter buffer

	gl.genBuffers(1, &m_atomicCounterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genBuffers() call failed.");

	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer() call failed.");

	gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bufferData() call failed.");

	// generate atomic counter calls buffer

	gl.genBuffers(1, &m_atomicCounterCallsBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genBuffers() call failed.");

	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounterCallsBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer() call failed.");

	gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bufferData() call failed.");

	// setup tested atomic operations

	setOperations();

	// setup shaders

	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		iter->create(m_context);
	}
}

void ShaderAtomicCounterOpsTestBase::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// delete atomic counter buffer

	gl.deleteBuffers(1, &m_atomicCounterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "deleteBuffers() call failed.");

	// delete atomic counter calls buffer

	gl.deleteBuffers(1, &m_atomicCounterCallsBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "deleteBuffers() call failed.");

	// delete operations

	for (AtomicOperationIter iter = m_operations.begin(); iter != m_operations.end(); ++iter)
	{
		delete *iter;
	}
}

tcu::TestNode::IterateResult ShaderAtomicCounterOpsTestBase::iterate()
{
	if (!m_contextSupportsGL46)
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_atomic_counters") ||
			!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_atomic_counter_ops"))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
			return STOP;
		}
	}

	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		fillAtomicCounterBuffer(iter->getAtomicOperation());
		bindBuffers();
		iter->test(m_context);

		bool		operationValueValid = checkAtomicCounterBuffer(iter->getAtomicOperation());
		std::string operationFailMsg	= "Result of atomic operation was different than expected (" +
									   iter->getAtomicOperation()->getFunction() + ").";
		TCU_CHECK_MSG(operationValueValid, operationFailMsg.c_str());

		bool		returnValueValid = validateScreenPixels(tcu::Vec4(1.0f), tcu::Vec4(0.5f));
		std::string returnFailMsg	= "Result of atomic operation return value was different than expected (" +
									iter->getAtomicOperation()->getFunction() + ").";
		TCU_CHECK_MSG(returnValueValid, returnFailMsg.c_str());
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderAtomicCounterOpsAdditionSubstractionTestCase::ShaderAtomicCounterOpsAdditionSubstractionTestCase(
	deqp::Context& context)
	: ShaderAtomicCounterOpsTestBase(
		  context, "ShaderAtomicCounterOpsAdditionSubstractionTestCase",
		  "Implements verification of new built-in addition and substraction atomic counter operations")
{
}

void ShaderAtomicCounterOpsAdditionSubstractionTestCase::setOperations()
{
	glw::GLuint input = 12;
	glw::GLuint param = 4;

	addOperation(new AtomicOperationAdd(input, param));
	addOperation(new AtomicOperationSubtract(input, param));
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderAtomicCounterOpsMinMaxTestCase::ShaderAtomicCounterOpsMinMaxTestCase(deqp::Context& context)
	: ShaderAtomicCounterOpsTestBase(
		  context, "ShaderAtomicCounterOpsMinMaxTestCase",
		  "Implements verification of new built-in minimum and maximum atomic counter operations")
{
}

void ShaderAtomicCounterOpsMinMaxTestCase::setOperations()
{
	glw::GLuint input	= 12;
	glw::GLuint params[] = { 4, 16 };

	addOperation(new AtomicOperationMin(input, params[0]));
	addOperation(new AtomicOperationMin(input, params[1]));
	addOperation(new AtomicOperationMax(input, params[0]));
	addOperation(new AtomicOperationMax(input, params[1]));
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderAtomicCounterOpsBitwiseTestCase::ShaderAtomicCounterOpsBitwiseTestCase(deqp::Context& context)
	: ShaderAtomicCounterOpsTestBase(context, "ShaderAtomicCounterOpsBitwiseTestCase",
									 "Implements verification of new built-in bitwise atomic counter operations")
{
}

void ShaderAtomicCounterOpsBitwiseTestCase::setOperations()
{
	glw::GLuint input = 0x2ED; // 0b1011101101;
	glw::GLuint param = 0x3A9; // 0b1110101001;

	addOperation(new AtomicOperationAnd(input, param));
	addOperation(new AtomicOperationOr(input, param));
	addOperation(new AtomicOperationXor(input, param));
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderAtomicCounterOpsExchangeTestCase::ShaderAtomicCounterOpsExchangeTestCase(deqp::Context& context)
	: ShaderAtomicCounterOpsTestBase(
		  context, "ShaderAtomicCounterOpsExchangeTestCase",
		  "Implements verification of new built-in exchange and swap atomic counter operations")
{
}

void ShaderAtomicCounterOpsExchangeTestCase::setOperations()
{
	glw::GLuint input	 = 5;
	glw::GLuint param	 = 10;
	glw::GLuint compare[] = { 5, 20 };

	addOperation(new AtomicOperationExchange(input, param));
	addOperation(new AtomicOperationCompSwap(input, param, compare[0]));
	addOperation(new AtomicOperationCompSwap(input, param, compare[1]));
}

/** Constructor.
*
*  @param context Rendering context.
*/
ShaderAtomicCounterOps::ShaderAtomicCounterOps(deqp::Context& context)
	: TestCaseGroup(context, "shader_atomic_counter_ops_tests",
					"Verify conformance of CTS_ARB_shader_atomic_counter_ops implementation")
{
}

/** Initializes the test group contents. */
void ShaderAtomicCounterOps::init()
{
	addChild(new ShaderAtomicCounterOpsAdditionSubstractionTestCase(m_context));
	addChild(new ShaderAtomicCounterOpsMinMaxTestCase(m_context));
	addChild(new ShaderAtomicCounterOpsBitwiseTestCase(m_context));
	addChild(new ShaderAtomicCounterOpsExchangeTestCase(m_context));
}
} /* gl4cts namespace */
