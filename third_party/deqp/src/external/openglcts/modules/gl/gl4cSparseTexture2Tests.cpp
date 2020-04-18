/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
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
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  gl4cSparseTexture2Tests.cpp
 * \brief Conformance tests for the GL_ARB_sparse_texture2 functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cSparseTexture2Tests.hpp"
#include "deStringUtil.hpp"
#include "gl4cSparseTextureTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <cmath>
#include <stdio.h>
#include <string.h>
#include <vector>

using namespace glw;
using namespace glu;

namespace gl4cts
{

const char* st2_compute_textureFill = "#version 430 core\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "layout (location = 1) writeonly uniform highp <INPUT_TYPE> uni_image;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    <POINT_TYPE> point = <POINT_TYPE>(<POINT_DEF>);\n"
									  "    memoryBarrier();\n"
									  "    <RETURN_TYPE> color = <RETURN_TYPE><RESULT_EXPECTED>;\n"
									  "    imageStore(uni_image, point<SAMPLE_DEF>, color);\n"
									  "}\n";

const char* st2_compute_textureVerify = "#version 430 core\n"
										"\n"
										"#extension GL_ARB_sparse_texture2 : enable\n"
										"\n"
										"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
										"\n"
										"layout (location = 1, r8ui) writeonly uniform <OUTPUT_TYPE> uni_out_image;\n"
										"layout (location = 2, <FORMAT>) readonly uniform <INPUT_TYPE> uni_in_image;\n"
										"\n"
										"void main()\n"
										"{\n"
										"    <POINT_TYPE> point = <POINT_TYPE>(<POINT_DEF>);\n"
										"    memoryBarrier();\n"
										"    highp <RETURN_TYPE> color,\n"
										"                        expected,\n"
										"                        epsilon;\n"
										"    color = imageLoad(uni_in_image, point<SAMPLE_DEF>);\n"
										"    expected = <RETURN_TYPE><RESULT_EXPECTED>;\n"
										"    epsilon = <RETURN_TYPE>(<EPSILON>);\n"
										"\n"
										"    if (all(lessThanEqual(color, expected + epsilon)) &&\n"
										"        all(greaterThanEqual(color, expected - epsilon)))\n"
										"    {\n"
										"        imageStore(uni_out_image, point, uvec4(255));\n"
										"    }\n"
										"    else {\n"
										"        imageStore(uni_out_image, point, uvec4(0));\n"
										"    }\n"
										"}\n";

const char* st2_compute_atomicVerify = "#version 430 core\n"
									   "\n"
									   "#extension GL_ARB_sparse_texture2 : enable\n"
									   "\n"
									   "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									   "\n"
									   "layout (location = 1, r8ui) writeonly uniform <OUTPUT_TYPE> uni_out_image;\n"
									   "layout (location = 2, <FORMAT>) uniform <INPUT_TYPE> uni_in_image;\n"
									   "\n"
									   "layout (location = 3) uniform int widthCommitted;\n"
									   "\n"
									   "void main()\n"
									   "{\n"
									   "    <POINT_TYPE> point,\n"
									   "                 offset;\n"
									   "    point = <POINT_TYPE>(<POINT_DEF>);\n"
									   "    offset = <POINT_TYPE>(0);\n"
									   "    offset.x = widthCommitted;\n"
									   "    memoryBarrier();\n"
									   "    if (point.x >= widthCommitted) {\n"
									   "        uint index = ((point.x - widthCommitted) + point.y * 8) % 8;\n"
									   "        <DATA_TYPE> value = 127;\n"
									   "        if (index == 0)\n"
									   "            value = imageAtomicExchange(uni_in_image, point<SAMPLE_DEF>,\n"
									   "                                        <DATA_TYPE>(0x0F));\n"
									   "        else if (index == 1)\n"
									   "            value = imageAtomicCompSwap(uni_in_image, point<SAMPLE_DEF>,\n"
									   "                                        <DATA_TYPE>(0), <DATA_TYPE>(0x0F));\n"
									   "        else if (index == 2)\n"
									   "            value = imageAtomicAdd(uni_in_image, point<SAMPLE_DEF>,\n"
									   "                                   <DATA_TYPE>(0x0F));\n"
									   "        else if (index == 3)\n"
									   "            value = imageAtomicAnd(uni_in_image, point<SAMPLE_DEF>,\n"
									   "                                   <DATA_TYPE>(0x0F));\n"
									   "        else if (index == 4)\n"
									   "            value = imageAtomicOr(uni_in_image, point<SAMPLE_DEF>,\n"
									   "                                  <DATA_TYPE>(0x0F));\n"
									   "        else if (index == 5)\n"
									   "            value = imageAtomicXor(uni_in_image, point<SAMPLE_DEF>,\n"
									   "                                   <DATA_TYPE>(0x0F));\n"
									   "        else if (index == 6)\n"
									   "            value = imageAtomicMin(uni_in_image, point<SAMPLE_DEF>,\n"
									   "                                   <DATA_TYPE>(0x0F));\n"
									   "        else if (index == 7)\n"
									   "            value = imageAtomicMax(uni_in_image, point<SAMPLE_DEF>,\n"
									   "                                   <DATA_TYPE>(0x0F));\n"
									   "\n"
									   "        <RETURN_TYPE> color = imageLoad(uni_in_image, point<SAMPLE_DEF>);\n"
									   "\n"
									   "        if (value == 0)\n"
									   "            imageStore(uni_out_image, point - offset, uvec4(0));\n"
									   "        else\n"
									   "            imageStore(uni_out_image, point - offset, uvec4(value));\n"
									   "\n"
									   "        if (color.r == 0)\n"
									   "            imageStore(uni_out_image, point, uvec4(0));\n"
									   "        else\n"
									   "            imageStore(uni_out_image, point, uvec4(1));\n"
									   "    }\n"
									   "}\n";

const char* st2_vertex_drawBuffer = "#version 430 core\n"
									"\n"
									"#extension GL_ARB_sparse_texture2 : enable\n"
									"\n"
									"in vec3 vertex;\n"
									"in vec2 inTexCoord;\n"
									"out vec2 texCoord;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    texCoord = inTexCoord;\n"
									"    gl_Position = vec4(vertex, 1);\n"
									"}\n";

const char* st2_fragment_drawBuffer = "#version 430 core\n"
									  "\n"
									  "#extension GL_ARB_sparse_texture2 : enable\n"
									  "\n"
									  "layout (location = 1) uniform sampler2D uni_sampler;\n"
									  "\n"
									  "in vec2 texCoord;\n"
									  "out vec4 fragColor;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    fragColor = texture(uni_sampler, texCoord);\n"
									  "}\n";

const char* st2_compute_extensionCheck = "#version 450 core\n"
										 "\n"
										 "#extension <EXTENSION> : require\n"
										 "\n"
										 "#ifndef <EXTENSION>\n"
										 "  #error <EXTENSION> not defined\n"
										 "#else\n"
										 "  #if (<EXTENSION> != 1)\n"
										 "    #error <EXTENSION> wrong value\n"
										 "  #endif\n"
										 "#endif // <EXTENSION>\n"
										 "\n"
										 "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
										 "\n"
										 "void main()\n"
										 "{\n"
										 "}\n";

const char* st2_compute_lookupVerify = "#version 450 core\n"
									   "\n"
									   "#extension GL_ARB_sparse_texture2 : enable\n"
									   "#extension GL_ARB_sparse_texture_clamp : enable\n"
									   "\n"
									   "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									   "\n"
									   "layout (location = 1, r8ui) writeonly uniform <OUTPUT_TYPE> uni_out;\n"
									   "layout (location = 2<FORMAT_DEF>) uniform <INPUT_TYPE> uni_in;\n"
									   "layout (location = 3) uniform int widthCommitted;\n"
									   "\n"
									   "void main()\n"
									   "{\n"
									   "    <POINT_TYPE> point = <POINT_TYPE>(<POINT_DEF>);\n"
									   "    <ICOORD_TYPE> texSize = <ICOORD_TYPE>(<SIZE_DEF>);\n"
									   "    <ICOORD_TYPE> icoord = <ICOORD_TYPE>(<COORD_DEF>);\n"
									   "    <COORD_TYPE> coord = <COORD_TYPE>(<COORD_DEF>) / <COORD_TYPE>(texSize);\n"
									   "    <RETURN_TYPE> retValue,\n"
									   "                  expValue,\n"
									   "                  epsilon;\n"
									   "    retValue = <RETURN_TYPE>(0);\n"
									   "    expValue = <RETURN_TYPE><RESULT_EXPECTED>;\n"
									   "    epsilon = <RETURN_TYPE>(<EPSILON>);\n"
									   "\n"
									   "<CUBE_MAP_COORD_DEF>\n"
									   "<OFFSET_ARRAY_DEF>\n"
									   "\n"
									   "    ivec2 corner1 = ivec2(1, 1);\n"
									   "    ivec2 corner2 = ivec2(texSize.x - 1, texSize.y - 1);\n"
									   "\n"
									   "    int code = <FUNCTION>(uni_in,\n"
									   "                          <POINT_COORD><SAMPLE_DEF><ARGUMENTS>,\n"
									   "                          retValue<COMPONENT_DEF>);\n"
									   "    memoryBarrier();\n"
									   "\n"
									   "    imageStore(uni_out, point, uvec4(255));\n"
									   "\n"
									   "    if (point.x > corner1.x && point.y > corner1.y &&\n"
									   "        point.x < corner2.x && point.y < corner2.y &&\n"
									   "        point.x < widthCommitted - 1)\n"
									   "    {\n"
									   "        if (!sparseTexelsResidentARB(code) ||\n"
									   "            any(greaterThan(retValue, expValue + epsilon)) ||\n"
									   "            any(lessThan(retValue, expValue - epsilon)))\n"
									   "        {\n"
									   "            imageStore(uni_out, point, uvec4(0));\n"
									   "        }\n"
									   "    }\n"
									   "\n"
									   "    if (point.x > corner1.x && point.y > corner1.y &&\n"
									   "        point.x < corner2.x && point.y < corner2.y &&\n"
									   "        point.x >= widthCommitted + 1)\n"
									   "    {\n"
									   "        if (sparseTexelsResidentARB(code))\n"
									   "        {\n"
									   "            imageStore(uni_out, point, uvec4(0));\n"
									   "        }\n"
									   "    }\n"
									   "}\n";

/** Replace all occurance of <token> with <text> in <string>
 *
 * @param token           Token string
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void replaceToken(const GLchar* token, const GLchar* text, std::string& string)
{
	const size_t text_length  = strlen(text);
	const size_t token_length = strlen(token);

	size_t token_position;
	while ((token_position = string.find(token, 0)) != std::string::npos)
	{
		string.replace(token_position, token_length, text, text_length);
	}
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
ShaderExtensionTestCase::ShaderExtensionTestCase(deqp::Context& context, const std::string extension)
	: TestCase(context, "ShaderExtension", "Verifies if extension is available for GLSL"), mExtension(extension)
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ShaderExtensionTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported(mExtension.c_str()))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	std::string shader = st2_compute_extensionCheck;
	replaceToken("<EXTENSION>", mExtension.c_str(), shader);

	ProgramSources sources;
	sources << ComputeSource(shader);
	ShaderProgram program(gl, sources);

	if (!program.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		m_testCtx.getLog() << tcu::TestLog::Message << "Checking shader preprocessor directives failed. Source:\n"
						   << shader.c_str() << "InfoLog:\n"
						   << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog << "\n"
						   << tcu::TestLog::EndMessage;
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
StandardPageSizesTestCase::StandardPageSizesTestCase(deqp::Context& context)
	: TestCase(context, "StandardPageSizesTestCase",
			   "Verifies if values returned by GetInternalFormativ query matches Standard Virtual Page Sizes")
{
	/* Left blank intentionally */
}

/** Stub init method */
void StandardPageSizesTestCase::init()
{
	mSupportedTargets.push_back(GL_TEXTURE_1D);
	mSupportedTargets.push_back(GL_TEXTURE_1D_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_2D);
	mSupportedTargets.push_back(GL_TEXTURE_2D_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP);
	mSupportedTargets.push_back(GL_TEXTURE_CUBE_MAP_ARRAY);
	mSupportedTargets.push_back(GL_TEXTURE_RECTANGLE);
	mSupportedTargets.push_back(GL_TEXTURE_BUFFER);
	mSupportedTargets.push_back(GL_RENDERBUFFER);

	mStandardVirtualPageSizesTable[GL_R8]			  = PageSizeStruct(256, 256, 1);
	mStandardVirtualPageSizesTable[GL_R8_SNORM]		  = PageSizeStruct(256, 256, 1);
	mStandardVirtualPageSizesTable[GL_R8I]			  = PageSizeStruct(256, 256, 1);
	mStandardVirtualPageSizesTable[GL_R8UI]			  = PageSizeStruct(256, 256, 1);
	mStandardVirtualPageSizesTable[GL_R16]			  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_R16_SNORM]	  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG8]			  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG8_SNORM]	  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGB565]		  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_R16F]			  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_R16I]			  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_R16UI]		  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG8I]			  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG8UI]		  = PageSizeStruct(256, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG16]			  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG16_SNORM]	 = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGBA8]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGBA8_SNORM]	= PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGB10_A2]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGB10_A2UI]	 = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG16F]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_R32F]			  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_R11F_G11F_B10F] = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGB9_E5]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_R32I]			  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_R32UI]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG16I]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RG16UI]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGBA8I]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGBA8UI]		  = PageSizeStruct(128, 128, 1);
	mStandardVirtualPageSizesTable[GL_RGBA16]		  = PageSizeStruct(128, 64, 1);
	mStandardVirtualPageSizesTable[GL_RGBA16_SNORM]   = PageSizeStruct(128, 64, 1);
	mStandardVirtualPageSizesTable[GL_RGBA16F]		  = PageSizeStruct(128, 64, 1);
	mStandardVirtualPageSizesTable[GL_RG32F]		  = PageSizeStruct(128, 64, 1);
	mStandardVirtualPageSizesTable[GL_RG32I]		  = PageSizeStruct(128, 64, 1);
	mStandardVirtualPageSizesTable[GL_RG32UI]		  = PageSizeStruct(128, 64, 1);
	mStandardVirtualPageSizesTable[GL_RGBA16I]		  = PageSizeStruct(128, 64, 1);
	mStandardVirtualPageSizesTable[GL_RGBA16UI]		  = PageSizeStruct(128, 64, 1);
	mStandardVirtualPageSizesTable[GL_RGBA32F]		  = PageSizeStruct(64, 64, 1);
	mStandardVirtualPageSizesTable[GL_RGBA32I]		  = PageSizeStruct(64, 64, 1);
	mStandardVirtualPageSizesTable[GL_RGBA32UI]		  = PageSizeStruct(64, 64, 1);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult StandardPageSizesTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture2"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.getLog() << tcu::TestLog::Message << "Testing getInternalformativ" << tcu::TestLog::EndMessage;

	for (std::vector<glw::GLint>::const_iterator iter = mSupportedTargets.begin(); iter != mSupportedTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		for (std::map<glw::GLint, PageSizeStruct>::const_iterator formIter = mStandardVirtualPageSizesTable.begin();
			 formIter != mStandardVirtualPageSizesTable.end(); ++formIter)
		{
			const PageSizePair&   format = *formIter;
			const PageSizeStruct& page   = format.second;

			GLint pageSizeX;
			GLint pageSizeY;
			GLint pageSizeZ;
			SparseTextureUtils::getTexturePageSizes(gl, target, format.first, pageSizeX, pageSizeY, pageSizeZ);

			if (pageSizeX != page.xSize || pageSizeY != page.ySize || pageSizeZ != page.zSize)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Standard Virtual Page Size mismatch, target: " << target
								   << ", format: " << format.first << ", returned: " << pageSizeX << "/" << pageSizeY
								   << "/" << pageSizeZ << ", expected: " << page.xSize << "/" << page.ySize << "/"
								   << page.zSize << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTexture2AllocationTestCase::SparseTexture2AllocationTestCase(deqp::Context& context)
	: SparseTextureAllocationTestCase(context, "SparseTexture2Allocation",
									  "Verifies TexStorage* functionality added in CTS_ARB_sparse_texture2")
{
	/* Left blank intentionally */
}

/** Initializes the test group contents. */
void SparseTexture2AllocationTestCase::init()
{
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE);
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

	mFullArrayTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

	mSupportedInternalFormats.push_back(GL_R8);
	mSupportedInternalFormats.push_back(GL_R8_SNORM);
	mSupportedInternalFormats.push_back(GL_R16);
	mSupportedInternalFormats.push_back(GL_R16_SNORM);
	mSupportedInternalFormats.push_back(GL_RG8);
	mSupportedInternalFormats.push_back(GL_RG8_SNORM);
	mSupportedInternalFormats.push_back(GL_RG16);
	mSupportedInternalFormats.push_back(GL_RG16_SNORM);
	mSupportedInternalFormats.push_back(GL_RGB565);
	mSupportedInternalFormats.push_back(GL_RGBA8);
	mSupportedInternalFormats.push_back(GL_RGBA8_SNORM);
	mSupportedInternalFormats.push_back(GL_RGB10_A2);
	mSupportedInternalFormats.push_back(GL_RGB10_A2UI);
	mSupportedInternalFormats.push_back(GL_RGBA16);
	mSupportedInternalFormats.push_back(GL_RGBA16_SNORM);
	mSupportedInternalFormats.push_back(GL_R16F);
	mSupportedInternalFormats.push_back(GL_RG16F);
	mSupportedInternalFormats.push_back(GL_RGBA16F);
	mSupportedInternalFormats.push_back(GL_R32F);
	mSupportedInternalFormats.push_back(GL_RG32F);
	mSupportedInternalFormats.push_back(GL_RGBA32F);
	mSupportedInternalFormats.push_back(GL_R11F_G11F_B10F);
	// RGB9_E5 isn't color renderable, and thus isn't valid for multisample
	// textures.
	//mSupportedInternalFormats.push_back(GL_RGB9_E5);
	mSupportedInternalFormats.push_back(GL_R8I);
	mSupportedInternalFormats.push_back(GL_R8UI);
	mSupportedInternalFormats.push_back(GL_R16I);
	mSupportedInternalFormats.push_back(GL_R16UI);
	mSupportedInternalFormats.push_back(GL_R32I);
	mSupportedInternalFormats.push_back(GL_R32UI);
	mSupportedInternalFormats.push_back(GL_RG8I);
	mSupportedInternalFormats.push_back(GL_RG8UI);
	mSupportedInternalFormats.push_back(GL_RG16I);
	mSupportedInternalFormats.push_back(GL_RG16UI);
	mSupportedInternalFormats.push_back(GL_RG32I);
	mSupportedInternalFormats.push_back(GL_RG32UI);
	mSupportedInternalFormats.push_back(GL_RGBA8I);
	mSupportedInternalFormats.push_back(GL_RGBA8UI);
	mSupportedInternalFormats.push_back(GL_RGBA16I);
	mSupportedInternalFormats.push_back(GL_RGBA16UI);
	mSupportedInternalFormats.push_back(GL_RGBA32I);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseTexture2AllocationTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture2"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	return SparseTextureAllocationTestCase::iterate();
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SparseTexture2CommitmentTestCase::SparseTexture2CommitmentTestCase(deqp::Context& context, const char* name,
																   const char* description)
	: SparseTextureCommitmentTestCase(context, name, description)
{
	/* Left blank intentionally */
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTexture2CommitmentTestCase::SparseTexture2CommitmentTestCase(deqp::Context& context)
	: SparseTextureCommitmentTestCase(
		  context, "SparseTexture2Commitment",
		  "Verifies glTexPageCommitmentARB functionality added by ARB_sparse_texture2 extension")
{
	/* Left blank intentionally */
}

/** Initializes the test group contents. */
void SparseTexture2CommitmentTestCase::init()
{
	SparseTextureCommitmentTestCase::init();

	//Verify all targets once again and multisample targets as it was added in ARB_sparse_texture2 extension
	mSupportedTargets.clear();
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE);
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseTexture2CommitmentTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture2"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	return SparseTextureCommitmentTestCase::iterate();
}

/** Create set of token strings fit to texture verifying shader
 *
 * @param target     Target for which texture is binded
 * @param format     Texture internal format
 * @param sample     Texture sample number

 * @return target    Structure of token strings
 */
SparseTexture2CommitmentTestCase::TokenStrings SparseTexture2CommitmentTestCase::createShaderTokens(
	GLint target, GLint format, GLint sample, const std::string outputBase, const std::string inputBase)
{
	TokenStrings s;
	std::string  prefix;

	if (format == GL_R8)
	{
		s.format		 = "r8";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_R8_SNORM)
	{
		s.format		 = "r8_snorm";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_R16)
	{
		s.format		 = "r16";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_R16_SNORM)
	{
		s.format		 = "r16_snorm";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RG8)
	{
		s.format		 = "rg8";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RG8_SNORM)
	{
		s.format		 = "rg8_snorm";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RG16)
	{
		s.format		 = "rg16";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RG16_SNORM)
	{
		s.format		 = "rg16_snorm";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RGBA8)
	{
		s.format		 = "rgba8";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
	}
	else if (format == GL_RGBA8_SNORM)
	{
		s.format		 = "rgba8_snorm";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
	}
	else if (format == GL_RGB10_A2)
	{
		s.format		 = "rgb10_a2";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
	}
	else if (format == GL_RGB10_A2UI)
	{
		s.format		 = "rgb10_a2ui";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
		prefix			 = "u";
	}
	else if (format == GL_RGBA16)
	{
		s.format		 = "rgba16";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
	}
	else if (format == GL_RGBA16_SNORM)
	{
		s.format		 = "rgba16_snorm";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
	}
	else if (format == GL_R16F)
	{
		s.format		 = "r16f";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RG16F)
	{
		s.format		 = "rg16f";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RGBA16F)
	{
		s.format		 = "rgba16f";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
	}
	else if (format == GL_R32F)
	{
		s.format		 = "r32f";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RG32F)
	{
		s.format		 = "rg32f";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_RGBA32F)
	{
		s.format		 = "rgba32f";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
	}
	else if (format == GL_R11F_G11F_B10F)
	{
		s.format		 = "r11f_g11f_b10f";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
	}
	else if (format == GL_R8I)
	{
		s.format		 = "r8i";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "i";
	}
	else if (format == GL_R8UI)
	{
		s.format		 = "r8ui";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "u";
	}
	else if (format == GL_R16I)
	{
		s.format		 = "r16i";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "i";
	}
	else if (format == GL_R16UI)
	{
		s.format		 = "r16ui";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "u";
	}
	else if (format == GL_R32I)
	{
		s.format		 = "r32i";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "i";
	}
	else if (format == GL_R32UI)
	{
		s.format		 = "r32ui";
		s.resultExpected = "(1, 0, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "u";
	}
	else if (format == GL_RG8I)
	{
		s.format		 = "rg8i";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "i";
	}
	else if (format == GL_RG8UI)
	{
		s.format		 = "rg8ui";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "u";
	}
	else if (format == GL_RG16I)
	{
		s.format		 = "rg16i";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "i";
	}
	else if (format == GL_RG16UI)
	{
		s.format		 = "rg16ui";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "u";
	}
	else if (format == GL_RG32I)
	{
		s.format		 = "rg32i";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "i";
	}
	else if (format == GL_RG32UI)
	{
		s.format		 = "rg32ui";
		s.resultExpected = "(1, 1, 0, 1)";
		s.resultDefault  = "(0, 0, 0, 1)";
		prefix			 = "u";
	}
	else if (format == GL_RGBA8I)
	{
		s.format		 = "rgba8i";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
		prefix			 = "i";
	}
	else if (format == GL_RGBA8UI)
	{
		s.format		 = "rgba8ui";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
		prefix			 = "u";
	}
	else if (format == GL_RGBA16I)
	{
		s.format		 = "rgba16i";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
		prefix			 = "i";
	}
	else if (format == GL_RGBA16UI)
	{
		s.format		 = "rgba16ui";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
		prefix			 = "u";
	}
	else if (format == GL_RGBA32I)
	{
		s.format		 = "rgba32i";
		s.resultExpected = "(1, 1, 1, 1)";
		s.resultDefault  = "(0, 0, 0, 0)";
		prefix			 = "i";
	}
	else if (format == GL_DEPTH_COMPONENT16)
	{
		s.format		 = "r16";
		s.resultExpected = "(1, 0, 0, 0)";
		s.resultDefault  = "(0, 0, 0, 0)";
	}

	s.returnType = prefix + "vec4";
	s.outputType = "u" + outputBase + "2D";
	s.inputType  = prefix + inputBase + "2D";
	s.pointType  = "ivec2";
	s.pointDef   = "gl_WorkGroupID.x, gl_WorkGroupID.y";

	if (s.returnType == "vec4")
		s.epsilon = "0.008";
	else
		s.epsilon = "0";

	if (target == GL_TEXTURE_1D)
	{
		s.outputType = "u" + outputBase + "2D";
		s.inputType  = prefix + inputBase + "1D";
		s.pointType  = "int";
		s.pointDef   = "gl_WorkGroupID.x";
	}
	else if (target == GL_TEXTURE_1D_ARRAY)
	{
		s.outputType = "u" + outputBase + "2D_ARRAY";
		s.inputType  = prefix + inputBase + "1DArray";
		s.pointType  = "ivec2";
		s.pointDef   = "gl_WorkGroupID.x, gl_WorkGroupID.z";
	}
	else if (target == GL_TEXTURE_2D_ARRAY)
	{
		s.outputType = "u" + outputBase + "2DArray";
		s.inputType  = prefix + inputBase + "2DArray";
		s.pointType  = "ivec3";
		s.pointDef   = "gl_WorkGroupID.x, gl_WorkGroupID.y, gl_WorkGroupID.z";
	}
	else if (target == GL_TEXTURE_3D)
	{
		s.outputType = "u" + outputBase + "2DArray";
		s.inputType  = prefix + inputBase + "3D";
		s.pointType  = "ivec3";
		s.pointDef   = "gl_WorkGroupID.x, gl_WorkGroupID.y, gl_WorkGroupID.z";
	}
	else if (target == GL_TEXTURE_CUBE_MAP)
	{
		s.outputType = "u" + outputBase + "2DArray";
		s.inputType  = prefix + inputBase + "Cube";
		s.pointType  = "ivec3";
		s.pointDef   = "gl_WorkGroupID.x, gl_WorkGroupID.y, gl_WorkGroupID.z % 6";
	}
	else if (target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		s.outputType = "u" + outputBase + "2DArray";
		s.inputType  = prefix + inputBase + "CubeArray";
		s.pointType  = "ivec3";
		s.pointDef   = "gl_WorkGroupID.x, gl_WorkGroupID.y, gl_WorkGroupID.z";
	}
	else if (target == GL_TEXTURE_RECTANGLE)
	{
		s.inputType = prefix + inputBase + "2DRect";
	}
	else if (target == GL_TEXTURE_2D_MULTISAMPLE)
	{
		s.inputType = prefix + inputBase + "2DMS";
		s.sampleDef = ", " + de::toString(sample);
	}
	else if (target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		s.outputType = "u" + outputBase + "2DArray";
		s.inputType  = prefix + inputBase + "2DMSArray";
		s.pointType  = "ivec3";
		s.pointDef   = "gl_WorkGroupID.x, gl_WorkGroupID.y, gl_WorkGroupID.z";
		s.sampleDef  = ", " + de::toString(sample);
	}

	return s;
}

/** Check if specific combination of target and format is allowed
 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if target/format combination is allowed, false otherwise.
 */
bool SparseTexture2CommitmentTestCase::caseAllowed(GLint target, GLint format)
{
	// Multisample textures are filling with data and verifying using compute shader.
	// As shaders do not support some texture formats it is necessary to exclude them.
	if ((target == GL_TEXTURE_2D_MULTISAMPLE || target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) &&
		(format == GL_RGB565 || format == GL_RGB10_A2UI || format == GL_RGB9_E5))
	{
		return false;
	}

	return true;
}

/** Allocating sparse texture memory using texStorage* function
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param levels       Texture mipmaps level
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTexture2CommitmentTestCase::sparseAllocateTexture(const Functions& gl, GLint target, GLint format,
															 GLuint& texture, GLint levels)
{
	mLog << "Sparse Allocate [levels: " << levels << "] - ";

	prepareTexture(gl, target, format, texture);

	GLint maxLevels;
	gl.texParameteri(target, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri error occurred for GL_TEXTURE_SPARSE_ARB");
	gl.getTexParameteriv(target, GL_NUM_SPARSE_LEVELS_ARB, &maxLevels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexParameteriv");
	if (levels > maxLevels)
		levels = maxLevels;

	//GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY can have only one level
	if (target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_2D_MULTISAMPLE &&
		target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		mState.levels = levels;
	}
	else
		mState.levels = 1;

	if (target != GL_TEXTURE_2D_MULTISAMPLE && target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		mState.samples = 1;
	}
	else
		mState.samples = 2;

	Texture::Storage(gl, target, deMax32(mState.levels, mState.samples), format, mState.width, mState.height,
					 mState.depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage");

	return true;
}

/** Allocating texture memory using texStorage* function
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param levels       Texture mipmaps level
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTexture2CommitmentTestCase::allocateTexture(const Functions& gl, GLint target, GLint format, GLuint& texture,
													   GLint levels)
{
	mLog << "Allocate [levels: " << levels << "] - ";

	prepareTexture(gl, target, format, texture);

	// GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY can have only one level
	if (target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_2D_MULTISAMPLE &&
		target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		mState.levels = levels;
	}
	else
		mState.levels = 1;

	// GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY can use multiple samples
	if (target != GL_TEXTURE_2D_MULTISAMPLE && target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		mState.samples = 1;
	}
	else
		mState.samples = 2;

	Texture::Storage(gl, target, deMax32(mState.levels, mState.samples), format, mState.width, mState.height,
					 mState.depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage");

	return true;
}

/** Writing data to generated texture
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTexture2CommitmentTestCase::writeDataToTexture(const Functions& gl, GLint target, GLint format,
														  GLuint& texture, GLint level)
{
	mLog << "Fill Texture [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	TransferFormat transferFormat = glu::getTransferFormat(mState.format);

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (width > 0 && height > 0 && depth >= mState.minDepth)
	{
		if (target == GL_TEXTURE_CUBE_MAP)
			depth = depth * 6;

		GLint texSize = width * height * depth * mState.format.getPixelSize();

		std::vector<GLubyte> vecData;
		vecData.resize(texSize);
		GLubyte* data = vecData.data();

		deMemset(data, 255, texSize);

		if (target != GL_TEXTURE_2D_MULTISAMPLE && target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
		{
			Texture::SubImage(gl, target, level, 0, 0, 0, width, height, depth, transferFormat.format,
							  transferFormat.dataType, (GLvoid*)data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "SubImage");
		}
		// For multisample texture use compute shader to store image data
		else
		{
			for (GLint sample = 0; sample < mState.samples; ++sample)
			{
				std::string shader = st2_compute_textureFill;

				// Adjust shader source to texture format
				TokenStrings s = createShaderTokens(target, format, sample);

				replaceToken("<INPUT_TYPE>", s.inputType.c_str(), shader);
				replaceToken("<POINT_TYPE>", s.pointType.c_str(), shader);
				replaceToken("<POINT_DEF>", s.pointDef.c_str(), shader);
				replaceToken("<RETURN_TYPE>", s.returnType.c_str(), shader);
				replaceToken("<RESULT_EXPECTED>", s.resultExpected.c_str(), shader);
				replaceToken("<SAMPLE_DEF>", s.sampleDef.c_str(), shader);

				ProgramSources sources;
				sources << ComputeSource(shader);

				// Build and run shader
				ShaderProgram program(m_context.getRenderContext(), sources);
				if (program.isOk())
				{
					gl.useProgram(program.getProgram());
					GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");
					gl.bindImageTexture(0 /* unit */, texture, level /* level */, GL_FALSE /* layered */, 0 /* layer */,
										GL_WRITE_ONLY, format);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
					gl.uniform1i(1, 0 /* image_unit */);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
					gl.dispatchCompute(width, height, depth);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute");
					gl.memoryBarrier(GL_ALL_BARRIER_BITS);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier");
				}
				else
				{
					mLog << "Compute shader compilation failed (writing) for target: " << target
						 << ", format: " << format << ", sample: " << sample
						 << ", infoLog: " << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog
						 << ", shaderSource: " << shader.c_str() << " - ";
				}
			}
		}
	}

	return true;
}

/** Verify if data stored in texture is as expected
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 *
 * @return Returns true if data is as expected, false if not, throws an exception if error occurred.
 */
bool SparseTexture2CommitmentTestCase::verifyTextureData(const Functions& gl, GLint target, GLint format,
														 GLuint& texture, GLint level)
{
	mLog << "Verify Texture [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	TransferFormat transferFormat = glu::getTransferFormat(mState.format);

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	//Committed region is limited to 1/2 of width
	GLint widthCommitted = width / 2;

	if (widthCommitted == 0 || height == 0 || depth < mState.minDepth)
		return true;

	bool result = true;

	if (target != GL_TEXTURE_CUBE_MAP && target != GL_TEXTURE_2D_MULTISAMPLE &&
		target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		GLint texSize = width * height * depth * mState.format.getPixelSize();

		std::vector<GLubyte> vecExpData;
		std::vector<GLubyte> vecOutData;
		vecExpData.resize(texSize);
		vecOutData.resize(texSize);
		GLubyte* exp_data = vecExpData.data();
		GLubyte* out_data = vecOutData.data();

		deMemset(exp_data, 255, texSize);
		deMemset(out_data, 127, texSize);

		Texture::GetData(gl, level, target, transferFormat.format, transferFormat.dataType, (GLvoid*)out_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

		//Verify only committed region
		for (GLint x = 0; x < widthCommitted; ++x)
			for (GLint y = 0; y < height; ++y)
				for (GLint z = 0; z < depth; ++z)
				{
					int		 pixelSize	 = mState.format.getPixelSize();
					GLubyte* dataRegion	= exp_data + ((x + y * width) * pixelSize);
					GLubyte* outDataRegion = out_data + ((x + y * width) * pixelSize);
					if (deMemCmp(dataRegion, outDataRegion, pixelSize) != 0)
						result = false;
				}
	}
	else if (target == GL_TEXTURE_CUBE_MAP)
	{
		std::vector<GLint> subTargets;

		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

		GLint texSize = width * height * mState.format.getPixelSize();

		std::vector<GLubyte> vecExpData;
		std::vector<GLubyte> vecOutData;
		vecExpData.resize(texSize);
		vecOutData.resize(texSize);
		GLubyte* exp_data = vecExpData.data();
		GLubyte* out_data = vecOutData.data();

		deMemset(exp_data, 255, texSize);

		for (size_t i = 0; i < subTargets.size(); ++i)
		{
			GLint subTarget = subTargets[i];

			mLog << "Verify Subtarget [subtarget: " << subTarget << "] - ";

			deMemset(out_data, 127, texSize);

			Texture::GetData(gl, level, subTarget, transferFormat.format, transferFormat.dataType, (GLvoid*)out_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

			//Verify only committed region
			for (GLint x = 0; x < widthCommitted; ++x)
				for (GLint y = 0; y < height; ++y)
					for (GLint z = 0; z < depth; ++z)
					{
						int		 pixelSize	 = mState.format.getPixelSize();
						GLubyte* dataRegion	= exp_data + ((x + y * width) * pixelSize);
						GLubyte* outDataRegion = out_data + ((x + y * width) * pixelSize);
						if (deMemCmp(dataRegion, outDataRegion, pixelSize) != 0)
							result = false;
					}

			if (!result)
				break;
		}
	}
	// For multisample texture use compute shader to verify image data
	else if (target == GL_TEXTURE_2D_MULTISAMPLE || target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		GLint texSize = width * height * depth;

		std::vector<GLubyte> vecExpData;
		std::vector<GLubyte> vecOutData;
		vecExpData.resize(texSize);
		vecOutData.resize(texSize);
		GLubyte* exp_data = vecExpData.data();
		GLubyte* out_data = vecOutData.data();

		deMemset(exp_data, 255, texSize);

		// Create verifying texture
		GLint verifyTarget;
		if (target == GL_TEXTURE_2D_MULTISAMPLE)
			verifyTarget = GL_TEXTURE_2D;
		else
			verifyTarget = GL_TEXTURE_2D_ARRAY;

		GLuint verifyTexture;
		Texture::Generate(gl, verifyTexture);
		Texture::Bind(gl, verifyTexture, verifyTarget);
		Texture::Storage(gl, verifyTarget, 1, GL_R8, width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::Storage");

		for (int sample = 0; sample < mState.samples; ++sample)
		{
			deMemset(out_data, 0, texSize);

			Texture::Bind(gl, verifyTexture, verifyTarget);
			Texture::SubImage(gl, verifyTarget, 0, 0, 0, 0, width, height, depth, GL_RED, GL_UNSIGNED_BYTE,
							  (GLvoid*)out_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::SubImage");

			std::string shader = st2_compute_textureVerify;

			// Adjust shader source to texture format
			TokenStrings s = createShaderTokens(target, format, sample);

			replaceToken("<OUTPUT_TYPE>", s.outputType.c_str(), shader);
			replaceToken("<FORMAT>", s.format.c_str(), shader);
			replaceToken("<INPUT_TYPE>", s.inputType.c_str(), shader);
			replaceToken("<POINT_TYPE>", s.pointType.c_str(), shader);
			replaceToken("<POINT_DEF>", s.pointDef.c_str(), shader);
			replaceToken("<RETURN_TYPE>", s.returnType.c_str(), shader);
			replaceToken("<SAMPLE_DEF>", s.sampleDef.c_str(), shader);
			replaceToken("<RESULT_EXPECTED>", s.resultExpected.c_str(), shader);
			replaceToken("<EPSILON>", s.epsilon.c_str(), shader);

			ProgramSources sources;
			sources << ComputeSource(shader);

			// Build and run shader
			ShaderProgram program(m_context.getRenderContext(), sources);
			if (program.isOk())
			{
				gl.useProgram(program.getProgram());
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");
				gl.bindImageTexture(0, //unit
									verifyTexture,
									0,		  //level
									GL_FALSE, //layered
									0,		  //layer
									GL_WRITE_ONLY, GL_R8UI);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
				gl.bindImageTexture(1, //unit
									texture,
									level,	//level
									GL_FALSE, //layered
									0,		  //layer
									GL_READ_ONLY, format);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
				gl.uniform1i(1, 0 /* image_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
				gl.uniform1i(2, 1 /* image_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
				gl.dispatchCompute(width, height, depth);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute");
				gl.memoryBarrier(GL_ALL_BARRIER_BITS);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier");

				Texture::GetData(gl, 0, verifyTarget, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)out_data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

				//Verify only committed region
				for (GLint x = 0; x < widthCommitted; ++x)
					for (GLint y = 0; y < height; ++y)
						for (GLint z = 0; z < depth; ++z)
						{
							GLubyte* dataRegion	= exp_data + ((x + y * width) + z * width * height);
							GLubyte* outDataRegion = out_data + ((x + y * width) + z * width * height);
							if (dataRegion[0] != outDataRegion[0])
								result = false;
						}
			}
			else
			{
				mLog << "Compute shader compilation failed (reading) for target: " << target << ", format: " << format
					 << ", infoLog: " << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog
					 << ", shaderSource: " << shader.c_str() << " - ";

				result = false;
			}
		}

		Texture::Delete(gl, verifyTexture);
	}

	return result;
}

const GLfloat texCoord[] = {
	0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
};

const GLfloat vertices[] = {
	-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
};

const GLuint indices[] = { 0, 1, 2, 1, 2, 3 };

/** Constructor.
 *
 *  @param context     Rendering context
 */
UncommittedRegionsAccessTestCase::UncommittedRegionsAccessTestCase(deqp::Context& context)
	: SparseTexture2CommitmentTestCase(context, "UncommittedRegionsAccess",
									   "Verifies if access to uncommitted regions of sparse texture works as expected")
{
	/* Left blank intentionally */
}

/** Stub init method */
void UncommittedRegionsAccessTestCase::init()
{
	SparseTextureCommitmentTestCase::init();

	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE);
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult UncommittedRegionsAccessTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture2"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	GLuint texture;

	for (std::vector<glw::GLint>::const_iterator iter = mSupportedTargets.begin(); iter != mSupportedTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		for (std::vector<glw::GLint>::const_iterator formIter = mSupportedInternalFormats.begin();
			 formIter != mSupportedInternalFormats.end(); ++formIter)
		{
			const GLint& format = *formIter;

			if (!caseAllowed(target, format))
				continue;

			mLog.str("");
			mLog << "Testing uncommitted regions access for target: " << target << ", format: " << format << " - ";

			sparseAllocateTexture(gl, target, format, texture, 3);
			for (int l = 0; l < mState.levels; ++l)
			{
				if (commitTexturePage(gl, target, format, texture, l))
				{
					writeDataToTexture(gl, target, format, texture, l);
					result = result && UncommittedReads(gl, target, format, texture, l);
					result = result && UncommittedAtomicOperations(gl, target, format, texture, l);
				}

				if (!result)
					break;
			}

			Texture::Delete(gl, texture);

			if (!result)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail" << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Check if reads from uncommitted regions are allowed
 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if allowed, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::readsAllowed(GLint target, GLint format, bool shaderOnly)
{
	DE_UNREF(target);

	if (shaderOnly && (format == GL_RGB565 || format == GL_RGB10_A2UI || format == GL_RGB9_E5))
	{
		return false;
	}

	return true;
}

/** Check if atomic operations on uncommitted regions are allowed
 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if allowed, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::atomicAllowed(GLint target, GLint format)
{
	DE_UNREF(target);

	if (format == GL_R32I || format == GL_R32UI)
	{
		return true;
	}

	return false;
}

/** Check if depth and stencil test on uncommitted regions are allowed
 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if allowed, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::depthStencilAllowed(GLint target, GLint format)
{
	if (target == GL_TEXTURE_2D && format == GL_RGBA8)
	{
		return true;
	}

	return false;
}

/** Verify reads from uncommitted texture regions works as expected
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 *
 * @return Returns true if data is as expected, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::UncommittedReads(const Functions& gl, GLint target, GLint format,
														GLuint& texture, GLint level)
{
	bool result = true;

	// Verify using API glGetTexImage*
	if (readsAllowed(target, format, false))
	{
		mLog << "API Reads - ";
		result = result && verifyTextureDataExtended(gl, target, format, texture, level, false);
	}

	// Verify using shader imageLoad function
	if (result && readsAllowed(target, format, true))
	{
		mLog << "Shader Reads - ";
		result = result && verifyTextureDataExtended(gl, target, format, texture, level, true);
	}

	// Verify mipmap generating
	if (result && level == 0 && target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_2D_MULTISAMPLE &&
		target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		mLog << "Mipmap Generate - ";
		Texture::Bind(gl, texture, target);
		gl.generateMipmap(target);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenerateMipmap");

		for (int l = 1; l < mState.levels; ++l)
			result = result && verifyTextureDataExtended(gl, target, format, texture, level, false);
	}

	return result;
}

/** Verify atomic operations on uncommitted texture pixels works as expected
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 *
 * @return Returns true if data is as expected, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::UncommittedAtomicOperations(const Functions& gl, GLint target, GLint format,
																   GLuint& texture, GLint level)
{
	bool result = true;

	if (atomicAllowed(target, format))
	{
		mLog << "Atomic Operations - ";
		result = result && verifyAtomicOperations(gl, target, format, texture, level);
	}

	return result;
}

/** Verify depth and stencil tests on uncommitted texture pixels works as expected
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 *
 * @return Returns true if data is as expected, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::UncommittedDepthStencil(const Functions& gl, GLint target, GLint format,
															   GLuint& texture, GLint level)
{
	if (!depthStencilAllowed(target, format))
		return true;

	mLog << "Depth Stencil - ";

	bool result = true;

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	//Committed region is limited to 1/2 of width
	GLuint widthCommitted = width / 2;

	if (widthCommitted == 0 || height == 0 || depth < mState.minDepth)
		return true;

	//Prepare shaders
	std::string vertexSource   = st2_vertex_drawBuffer;
	std::string fragmentSource = st2_fragment_drawBuffer;

	ShaderProgram program(gl, glu::makeVtxFragSources(vertexSource, fragmentSource));
	if (!program.isOk())
	{
		mLog << "Shader compilation failed (depth_stencil) for target: " << target << ", format: " << format
			 << ", vertex_infoLog: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog
			 << ", fragment_infoLog: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog
			 << ", vertexSource: " << vertexSource.c_str() << "\n"
			 << ", fragmentSource: " << fragmentSource.c_str() << " - ";

		return false;
	}

	prepareDepthStencilFramebuffer(gl, width, height);

	gl.useProgram(program.getProgram());

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture");
	Texture::Bind(gl, texture, target);
	gl.uniform1i(1, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");

	// Stencil test
	result = result && verifyStencilTest(gl, program, width, height, widthCommitted);

	// Depth test
	result = result && verifyDepthTest(gl, program, width, height, widthCommitted);

	// Depth bounds test
	if (m_context.getContextInfo().isExtensionSupported("GL_EXT_depth_bounds_test"))
		result = result && verifyDepthBoundsTest(gl, program, width, height, widthCommitted);

	// Resources cleaning
	cleanupDepthStencilFramebuffer(gl);

	return result;
}

/** Prepare gl depth and stencil test resources
 *
 * @param gl      GL API functions
 * @param width   Framebuffer width
 * @param height  Framebuffer height
 */
void UncommittedRegionsAccessTestCase::prepareDepthStencilFramebuffer(const Functions& gl, GLint width, GLint height)
{
	gl.genRenderbuffers(1, &mRenderbuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers");
	gl.bindRenderbuffer(GL_RENDERBUFFER, mRenderbuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer");
	if (mState.samples == 1)
	{
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, width, height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage");
	}
	else
	{
		gl.renderbufferStorageMultisample(GL_RENDERBUFFER, mState.samples, GL_DEPTH_STENCIL, width, height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorageMultisample");
	}

	gl.genFramebuffers(1, &mFramebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRenderbuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer");
	gl.viewport(0, 0, width, height);
}

/** Cleanup gl depth and stencil test resources
 *
 * @param gl   GL API functions
 */
void UncommittedRegionsAccessTestCase::cleanupDepthStencilFramebuffer(const Functions& gl)
{
	gl.deleteFramebuffers(1, &mFramebuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteFramebuffers");
	gl.deleteRenderbuffers(1, &mRenderbuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteRenderbuffers");

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");
}

/** Verify if data stored in texture in uncommitted regions is as expected
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 * @param shaderOnly   Shader texture filling flag, default false
 *
 * @return Returns true if data is as expected, false if not, throws an exception if error occurred.
 */
bool UncommittedRegionsAccessTestCase::verifyTextureDataExtended(const Functions& gl, GLint target, GLint format,
																 GLuint& texture, GLint level, bool shaderOnly)
{
	mLog << "Verify Texture [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	TransferFormat transferFormat = glu::getTransferFormat(mState.format);

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	//Committed region is limited to 1/2 of width
	GLint widthCommitted = width / 2;

	if (widthCommitted == 0 || height == 0 || depth < mState.minDepth)
		return true;

	bool result = true;

	// Verify texture using API glGetTexImage* (Skip multisample textures as it can not be verified using API)
	if (!shaderOnly && target != GL_TEXTURE_CUBE_MAP && target != GL_TEXTURE_2D_MULTISAMPLE &&
		target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		GLint texSize = width * height * depth * mState.format.getPixelSize();

		std::vector<GLubyte> vecExpData;
		std::vector<GLubyte> vecOutData;
		vecExpData.resize(texSize);
		vecOutData.resize(texSize);
		GLubyte* exp_data = vecExpData.data();
		GLubyte* out_data = vecOutData.data();

		// Expected value in this case is 0 because atomic operations result on uncommitted regions are zeros
		deMemset(exp_data, 0, texSize);
		deMemset(out_data, 255, texSize);

		Texture::GetData(gl, level, target, transferFormat.format, transferFormat.dataType, (GLvoid*)out_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

		//Verify only uncommitted region
		for (GLint x = widthCommitted; x < width; ++x)
			for (GLint y = 0; y < height; ++y)
				for (GLint z = 0; z < depth; ++z)
				{
					int		 pixelSize	 = mState.format.getPixelSize();
					GLubyte* dataRegion	= exp_data + ((x + y * width) * pixelSize);
					GLubyte* outDataRegion = out_data + ((x + y * width) * pixelSize);
					if (deMemCmp(dataRegion, outDataRegion, pixelSize) != 0)
						result = false;
				}
	}
	// Verify texture using API glGetTexImage* (Only cube map as it has to be verified for subtargets)
	else if (!shaderOnly && target == GL_TEXTURE_CUBE_MAP)
	{
		std::vector<GLint> subTargets;

		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
		subTargets.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

		GLint texSize = width * height * mState.format.getPixelSize();

		std::vector<GLubyte> vecExpData;
		std::vector<GLubyte> vecOutData;
		vecExpData.resize(texSize);
		vecOutData.resize(texSize);
		GLubyte* exp_data = vecExpData.data();
		GLubyte* out_data = vecOutData.data();

		deMemset(exp_data, 0, texSize);

		for (size_t i = 0; i < subTargets.size(); ++i)
		{
			GLint subTarget = subTargets[i];

			mLog << "Verify Subtarget [subtarget: " << subTarget << "] - ";

			deMemset(out_data, 255, texSize);

			Texture::GetData(gl, level, subTarget, transferFormat.format, transferFormat.dataType, (GLvoid*)out_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

			//Verify only uncommitted region
			for (GLint x = widthCommitted; x < width; ++x)
				for (GLint y = 0; y < height; ++y)
					for (GLint z = 0; z < depth; ++z)
					{
						int		 pixelSize	 = mState.format.getPixelSize();
						GLubyte* dataRegion	= exp_data + ((x + y * width) * pixelSize);
						GLubyte* outDataRegion = out_data + ((x + y * width) * pixelSize);
						if (deMemCmp(dataRegion, outDataRegion, pixelSize) != 0)
							result = false;
					}

			if (!result)
				break;
		}
	}
	// Verify texture using shader imageLoad function
	else if (shaderOnly)
	{
		// Create verifying texture
		GLint verifyTarget;
		if (target == GL_TEXTURE_2D_MULTISAMPLE)
			verifyTarget = GL_TEXTURE_2D;
		else
			verifyTarget = GL_TEXTURE_2D_ARRAY;

		if (target == GL_TEXTURE_CUBE_MAP)
			depth = depth * 6;

		GLint texSize = width * height * depth;

		std::vector<GLubyte> vecExpData;
		std::vector<GLubyte> vecOutData;
		vecExpData.resize(texSize);
		vecOutData.resize(texSize);
		GLubyte* exp_data = vecExpData.data();
		GLubyte* out_data = vecOutData.data();

		// Expected value in this case is 255 because shader fills output texture with 255 if in texture is filled with zeros
		deMemset(exp_data, 255, texSize);

		GLuint verifyTexture;
		Texture::Generate(gl, verifyTexture);
		Texture::Bind(gl, verifyTexture, verifyTarget);
		Texture::Storage(gl, verifyTarget, 1, GL_R8, width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::Storage");

		for (GLint sample = 0; sample < mState.samples; ++sample)
		{
			deMemset(out_data, 0, texSize);

			Texture::Bind(gl, verifyTexture, verifyTarget);
			Texture::SubImage(gl, verifyTarget, 0, 0, 0, 0, width, height, depth, GL_RED, GL_UNSIGNED_BYTE,
							  (GLvoid*)out_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::SubImage");

			std::string shader = st2_compute_textureVerify;

			// Adjust shader source to texture format
			TokenStrings s = createShaderTokens(target, format, sample);

			replaceToken("<OUTPUT_TYPE>", s.outputType.c_str(), shader);
			replaceToken("<FORMAT>", s.format.c_str(), shader);
			replaceToken("<INPUT_TYPE>", s.inputType.c_str(), shader);
			replaceToken("<POINT_TYPE>", s.pointType.c_str(), shader);
			replaceToken("<POINT_DEF>", s.pointDef.c_str(), shader);
			replaceToken("<RETURN_TYPE>", s.returnType.c_str(), shader);
			replaceToken("<SAMPLE_DEF>", s.sampleDef.c_str(), shader);
			replaceToken("<RESULT_EXPECTED>", s.resultDefault.c_str(), shader);
			replaceToken("<EPSILON>", s.epsilon.c_str(), shader);

			ProgramSources sources;
			sources << ComputeSource(shader);

			// Build and run shader
			ShaderProgram program(m_context.getRenderContext(), sources);
			if (program.isOk())
			{
				gl.useProgram(program.getProgram());
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");
				gl.bindImageTexture(0, //unit
									verifyTexture,
									0,		  //level
									GL_FALSE, //layered
									0,		  //layer
									GL_WRITE_ONLY, GL_R8UI);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
				gl.bindImageTexture(1, //unit
									texture,
									level,	//level
									GL_FALSE, //layered
									0,		  //layer
									GL_READ_ONLY, format);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
				gl.uniform1i(1, 0 /* image_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
				gl.uniform1i(2, 1 /* image_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
				gl.dispatchCompute(width, height, depth);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute");
				gl.memoryBarrier(GL_ALL_BARRIER_BITS);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier");

				Texture::GetData(gl, 0, verifyTarget, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)out_data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

				//Verify only committed region
				for (GLint x = widthCommitted; x < width; ++x)
					for (GLint y = 0; y < height; ++y)
						for (GLint z = 0; z < depth; ++z)
						{
							GLubyte* dataRegion	= exp_data + ((x + y * width) + z * width * height);
							GLubyte* outDataRegion = out_data + ((x + y * width) + z * width * height);
							if (dataRegion[0] != outDataRegion[0])
								result = false;
						}
			}
			else
			{
				mLog << "Compute shader compilation failed (reading) for target: " << target << ", format: " << format
					 << ", sample: " << sample << ", infoLog: " << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog
					 << ", shaderSource: " << shader.c_str() << " - ";

				result = false;
			}
		}

		Texture::Delete(gl, verifyTexture);
	}

	return result;
}

/** Verify if atomic operations on uncommitted regions returns zeros and has no effect on texture
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 *
 * @return Returns true if data is as expected, false if not, throws an exception if error occurred.
 */
bool UncommittedRegionsAccessTestCase::verifyAtomicOperations(const Functions& gl, GLint target, GLint format,
															  GLuint& texture, GLint level)
{
	mLog << "Verify Atomic Operations [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	//Committed region is limited to 1/2 of width
	GLint widthCommitted = width / 2;

	if (widthCommitted == 0 || height == 0 || depth < mState.minDepth)
		return true;

	bool result = true;

	// Create verifying texture
	GLint verifyTarget;
	if (target == GL_TEXTURE_2D_MULTISAMPLE)
		verifyTarget = GL_TEXTURE_2D;
	else
		verifyTarget = GL_TEXTURE_2D_ARRAY;

	GLint texSize = width * height * depth;

	std::vector<GLubyte> vecExpData;
	std::vector<GLubyte> vecOutData;
	vecExpData.resize(texSize);
	vecOutData.resize(texSize);
	GLubyte* exp_data = vecExpData.data();
	GLubyte* out_data = vecOutData.data();

	// Expected value in this case is 0 because atomic operations result on uncommitted regions are zeros
	deMemset(exp_data, 0, texSize);

	GLuint verifyTexture;
	Texture::Generate(gl, verifyTexture);
	Texture::Bind(gl, verifyTexture, verifyTarget);
	Texture::Storage(gl, verifyTarget, 1, GL_R8, width, height, depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::Storage");

	for (GLint sample = 0; sample < mState.samples; ++sample)
	{
		deMemset(out_data, 255, texSize);

		Texture::Bind(gl, verifyTexture, verifyTarget);
		Texture::SubImage(gl, verifyTarget, 0, 0, 0, 0, width, height, depth, GL_RED, GL_UNSIGNED_BYTE,
						  (GLvoid*)out_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::SubImage");

		std::string shader = st2_compute_atomicVerify;

		// Adjust shader source to texture format
		TokenStrings s		  = createShaderTokens(target, format, sample);
		std::string  dataType = (s.returnType == "ivec4" ? "int" : "uint");

		replaceToken("<OUTPUT_TYPE>", s.outputType.c_str(), shader);
		replaceToken("<FORMAT>", s.format.c_str(), shader);
		replaceToken("<INPUT_TYPE>", s.inputType.c_str(), shader);
		replaceToken("<POINT_TYPE>", s.pointType.c_str(), shader);
		replaceToken("<POINT_DEF>", s.pointDef.c_str(), shader);
		replaceToken("<DATA_TYPE>", dataType.c_str(), shader);
		replaceToken("<SAMPLE_DEF>", s.sampleDef.c_str(), shader);
		replaceToken("<RETURN_TYPE>", s.returnType.c_str(), shader);

		ProgramSources sources;
		sources << ComputeSource(shader);

		// Build and run shader
		ShaderProgram program(m_context.getRenderContext(), sources);
		if (program.isOk())
		{
			gl.useProgram(program.getProgram());
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");
			gl.bindImageTexture(0, //unit
								verifyTexture,
								0,		  //level
								GL_FALSE, //layered
								0,		  //layer
								GL_WRITE_ONLY, GL_R8UI);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
			gl.bindImageTexture(1, //unit
								texture,
								level,	//level
								GL_FALSE, //layered
								0,		  //layer
								GL_READ_ONLY, format);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
			gl.uniform1i(1, 0 /* image_unit */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
			gl.uniform1i(2, 1 /* image_unit */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
			gl.uniform1i(3, widthCommitted /* committed width */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
			gl.dispatchCompute(width, height, depth);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute");
			gl.memoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier");

			Texture::GetData(gl, 0, verifyTarget, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)out_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

			//Verify only committed region
			for (GLint x = 0; x < width; ++x)
				for (GLint y = 0; y < height; ++y)
					for (GLint z = 0; z < depth; ++z)
					{
						GLubyte* dataRegion	= exp_data + ((x + y * width) + z * width * height);
						GLubyte* outDataRegion = out_data + ((x + y * width) + z * width * height);
						if (dataRegion[0] != outDataRegion[0])
						{
							printf("%d:%d ", dataRegion[0], outDataRegion[0]);
							result = false;
						}
					}
		}
		else
		{
			mLog << "Compute shader compilation failed (atomic) for target: " << target << ", format: " << format
				 << ", sample: " << sample << ", infoLog: " << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog
				 << ", shaderSource: " << shader.c_str() << " - ";

			result = false;
		}
	}

	Texture::Delete(gl, verifyTexture);

	return result;
}

/** Verify if stencil test on uncommitted texture region works as expected texture
 *
 * @param gl              GL API functions
 * @param program         Shader program
 * @param width           Texture width
 * @param height          Texture height
 * @param widthCommitted  Committed region width
 *
 * @return Returns true if stencil data is as expected, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::verifyStencilTest(const Functions& gl, ShaderProgram& program, GLint width,
														 GLint height, GLint widthCommitted)
{
	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("vertex", 3, 4, 0, vertices),
											   glu::va::Float("inTexCoord", 2, 4, 0, texCoord) };

	mLog << "Perform Stencil Test - ";

	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.enable(GL_STENCIL_TEST);
	gl.stencilFunc(GL_GREATER, 1, 0xFF);
	gl.stencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(indices), indices));

	std::vector<GLubyte> dataStencil;
	dataStencil.resize(width * height);
	GLubyte* dataStencilPtr = dataStencil.data();

	gl.readPixels(0, 0, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, (GLvoid*)dataStencilPtr);
	for (int x = widthCommitted; x < width; ++x)
		for (int y = 0; y < height; ++y)
		{
			if (dataStencilPtr[x + y * width] != 0x00)
			{
				gl.disable(GL_STENCIL_TEST);
				return false;
			}
		}

	gl.disable(GL_STENCIL_TEST);
	return true;
}

/** Verify if depth test on uncommitted texture region works as expected texture
 *
 * @param gl              GL API functions
 * @param program         Shader program
 * @param width           Texture width
 * @param height          Texture height
 * @param widthCommitted  Committed region width
 *
 * @return Returns true if depth data is as expected, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::verifyDepthTest(const Functions& gl, ShaderProgram& program, GLint width,
													   GLint height, GLint widthCommitted)
{
	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("vertex", 3, 4, 0, vertices),
											   glu::va::Float("inTexCoord", 2, 4, 0, texCoord) };

	mLog << "Perform Depth Test - ";

	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.enable(GL_DEPTH_TEST);
	gl.depthFunc(GL_LESS);

	glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(indices), indices));

	std::vector<GLuint> dataDepth;
	dataDepth.resize(width * height);
	GLuint* dataDepthPtr = dataDepth.data();

	gl.readPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, (GLvoid*)dataDepthPtr);
	for (int x = widthCommitted; x < width; ++x)
		for (int y = 0; y < height; ++y)
		{
			if (dataDepthPtr[x + y * width] != 0xFFFFFFFF)
			{
				gl.disable(GL_DEPTH_TEST);
				return false;
			}
		}

	gl.disable(GL_DEPTH_TEST);
	return true;
}

/** Verify if depth bounds test on uncommitted texture region works as expected texture
 *
 * @param gl              GL API functions
 * @param program         Shader program
 * @param width           Texture width
 * @param height          Texture height
 * @param widthCommitted  Committed region width
 *
 * @return Returns true if depth data is as expected, false otherwise.
 */
bool UncommittedRegionsAccessTestCase::verifyDepthBoundsTest(const Functions& gl, ShaderProgram& program, GLint width,
															 GLint height, GLint widthCommitted)
{
	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("vertex", 3, 4, 0, vertices),
											   glu::va::Float("inTexCoord", 2, 4, 0, texCoord) };

	mLog << "Perform Depth Bounds Test - ";

	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.enable(GL_DEPTH_BOUNDS_TEST_EXT);
	gl.depthFunc(GL_LESS);

	glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(indices), indices));

	std::vector<GLuint> dataDepth;
	dataDepth.resize(width * height);
	GLuint* dataDepthPtr = dataDepth.data();

	gl.readPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, (GLvoid*)dataDepthPtr);
	for (int x = widthCommitted; x < width; ++x)
		for (int y = 0; y < height; ++y)
		{
			if (dataDepthPtr[x + y * width] != 0xFFFFFFFF)
			{
				gl.disable(GL_DEPTH_BOUNDS_TEST_EXT);
				return false;
			}
		}

	gl.disable(GL_DEPTH_BOUNDS_TEST_EXT);
	return true;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTexture2LookupTestCase::SparseTexture2LookupTestCase(deqp::Context& context)
	: SparseTexture2CommitmentTestCase(context, "SparseTexture2Lookup",
									   "Verifies if sparse texture lookup functions for GLSL works as expected")
{
	/* Left blank intentionally */
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTexture2LookupTestCase::SparseTexture2LookupTestCase(deqp::Context& context, const char* name,
														   const char* description)
	: SparseTexture2CommitmentTestCase(context, name, description)
{
	/* Left blank intentionally */
}

/** Initializes the test group contents. */
void SparseTexture2LookupTestCase::init()
{
	SparseTextureCommitmentTestCase::init();
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE);
	mSupportedTargets.push_back(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

	mSupportedInternalFormats.push_back(GL_DEPTH_COMPONENT16);

	FunctionToken f;
	f = FunctionToken("sparseTextureARB", "<CUBE_REFZ_DEF>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureLodARB", ", <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureOffsetARB", ", <OFFSET_TYPE><OFFSET_DIM>(0)");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTexelFetchARB", "<LOD_DEF>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	f.allowedTargets.insert(GL_TEXTURE_2D_MULTISAMPLE);
	f.allowedTargets.insert(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTexelFetchOffsetARB", "<LOD_DEF>, <OFFSET_TYPE><OFFSET_DIM>(0)");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureLodOffsetARB", ", <LOD>, <OFFSET_TYPE><OFFSET_DIM>(0)");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureGradARB", ", <NOFFSET_TYPE><OFFSET_DIM>(0), <NOFFSET_TYPE><OFFSET_DIM>(0)");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureGradOffsetARB",
					  ", <NOFFSET_TYPE><OFFSET_DIM>(0), <NOFFSET_TYPE><OFFSET_DIM>(0), <OFFSET_TYPE><OFFSET_DIM>(0)");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureGatherARB", "<REFZ_DEF>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureGatherOffsetARB", "<REFZ_DEF>, <OFFSET_TYPE><OFFSET_DIM>(0)");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureGatherOffsetsARB", "<REFZ_DEF>, offsetsArray");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	mFunctions.push_back(f);

	f = FunctionToken("sparseImageLoadARB", "");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	f.allowedTargets.insert(GL_TEXTURE_RECTANGLE);
	f.allowedTargets.insert(GL_TEXTURE_2D_MULTISAMPLE);
	f.allowedTargets.insert(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
	//mFunctions.push_back(f);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseTexture2LookupTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture2"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	GLuint texture;

	for (std::vector<glw::GLint>::const_iterator iter = mSupportedTargets.begin(); iter != mSupportedTargets.end();
		 ++iter)
	{
		const GLint& target = *iter;

		for (std::vector<glw::GLint>::const_iterator formIter = mSupportedInternalFormats.begin();
			 formIter != mSupportedInternalFormats.end(); ++formIter)
		{
			const GLint& format = *formIter;

			if (!caseAllowed(target, format))
				continue;

			for (std::vector<FunctionToken>::const_iterator tokIter = mFunctions.begin(); tokIter != mFunctions.end();
				 ++tokIter)
			{
				// Check if target is allowed for current lookup function
				FunctionToken funcToken = *tokIter;
				if (!funcAllowed(target, format, funcToken))
					continue;

				mLog.str("");
				mLog << "Testing sparse texture lookup functions for target: " << target << ", format: " << format
					 << " - ";

				sparseAllocateTexture(gl, target, format, texture, 3);
				if (format == GL_DEPTH_COMPONENT16)
					setupDepthMode(gl, target, texture);

				for (int l = 0; l < mState.levels; ++l)
				{
					if (commitTexturePage(gl, target, format, texture, l))
					{
						writeDataToTexture(gl, target, format, texture, l);
						result = result && verifyLookupTextureData(gl, target, format, texture, l, funcToken);
					}

					if (!result)
						break;
				}

				Texture::Delete(gl, texture);

				if (!result)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << mLog.str() << "Fail" << tcu::TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
					return STOP;
				}
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Create set of token strings fit to lookup functions verifying shader
 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param level        Texture mipmap level
 * @param sample       Texture sample number
 * @param funcToken    Texture lookup function structure
 *
 * @return Returns extended token strings structure.
 */
SparseTexture2LookupTestCase::TokenStringsExt SparseTexture2LookupTestCase::createLookupShaderTokens(
	GLint target, GLint format, GLint level, GLint sample, FunctionToken& funcToken)
{
	std::string funcName = funcToken.name;

	TokenStringsExt s;

	std::string inputType;
	std::string samplerSufix;

	if (funcName == "sparseImageLoadARB")
		inputType = "image";
	else
		inputType = "sampler";

	// Copy data from TokenStrings to TokenStringsExt
	TokenStrings ss  = createShaderTokens(target, format, sample, "image", inputType);
	s.epsilon		 = ss.epsilon;
	s.format		 = ss.format;
	s.inputType		 = ss.inputType;
	s.outputType	 = ss.outputType;
	s.pointDef		 = ss.pointDef;
	s.pointType		 = ss.pointType;
	s.resultDefault  = ss.resultDefault;
	s.resultExpected = ss.resultExpected;
	s.returnType	 = ss.returnType;
	s.sampleDef		 = ss.sampleDef;

	// Set format definition for image input types
	if (inputType == "image")
		s.formatDef = ", " + s.format;

	// Set tokens for depth texture format
	if (format == GL_DEPTH_COMPONENT16)
	{
		s.refZDef = ", 0.5";

		if (inputType == "sampler" && target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_MULTISAMPLE &&
			target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
		{
			s.inputType = s.inputType + "Shadow";
		}
	}

	// Set coord type, coord definition and offset vector dimensions
	s.coordType   = "vec2";
	s.offsetType  = "ivec";
	s.nOffsetType = "vec";
	s.offsetDim   = "2";
	if (target == GL_TEXTURE_1D)
	{
		s.coordType   = "float";
		s.offsetType  = "int";
		s.nOffsetType = "float";
		s.offsetDim   = "";
	}
	else if (target == GL_TEXTURE_1D_ARRAY)
	{
		s.coordType   = "vec2";
		s.offsetType  = "int";
		s.nOffsetType = "float";
		s.offsetDim   = "";
	}
	else if (target == GL_TEXTURE_2D_ARRAY)
	{
		s.coordType = "vec3";
	}
	else if (target == GL_TEXTURE_3D)
	{
		s.coordType = "vec3";
		s.offsetDim = "3";
	}
	else if (target == GL_TEXTURE_CUBE_MAP)
	{
		s.coordType = "vec3";
		s.offsetDim = "3";
	}
	else if (target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		s.coordType = "vec4";
		s.coordDef  = "gl_WorkGroupID.x, gl_WorkGroupID.y, gl_WorkGroupID.z % 6, floor(gl_WorkGroupID.z / 6)";
		s.offsetDim = "3";
	}
	else if (target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		s.coordType = "vec3";
	}

	if ((target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY) &&
		funcName.find("Fetch", 0) == std::string::npos)
	{
		s.cubeMapCoordDef = "    int face = point.z % 6;\n"
							"    if (face == 0) coord.xyz = vec3(1, coord.y * 2 - 1, -coord.x * 2 + 1);\n"
							"    if (face == 1) coord.xyz = vec3(-1, coord.y * 2 - 1, coord.x * 2 - 1);\n"
							"    if (face == 2) coord.xyz = vec3(coord.x * 2 - 1, 1, coord.y * 2 - 1);\n"
							"    if (face == 3) coord.xyz = vec3(coord.x * 2 - 1, -1, -coord.y * 2 + 1);\n"
							"    if (face == 4) coord.xyz = vec3(coord.x * 2 - 1, coord.y * 2 - 1, 1);\n"
							"    if (face == 5) coord.xyz = vec3(-coord.x * 2 + 1, coord.y * 2 - 1, -1);\n";
	}

	if (s.coordDef.empty())
		s.coordDef = s.pointDef;

	// Set expected result vector, component definition and offset array definition for gather functions
	if (funcName.find("Gather", 0) != std::string::npos)
	{
		if (funcName.find("GatherOffsets", 0) != std::string::npos)
		{
			s.offsetArrayDef = "    <OFFSET_TYPE><OFFSET_DIM> offsetsArray[4];\n"
							   "    offsetsArray[0] = <OFFSET_TYPE><OFFSET_DIM>(0);\n"
							   "    offsetsArray[1] = <OFFSET_TYPE><OFFSET_DIM>(0);\n"
							   "    offsetsArray[2] = <OFFSET_TYPE><OFFSET_DIM>(0);\n"
							   "    offsetsArray[3] = <OFFSET_TYPE><OFFSET_DIM>(0);\n";
		}

		if (format != GL_DEPTH_COMPONENT16)
			s.componentDef = ", 0";
		s.resultExpected   = "(1, 1, 1, 1)";
	}
	// Extend coord type dimension and coord vector definition if shadow sampler and non-cube map array target selected
	// Set component definition to red component
	else if (format == GL_DEPTH_COMPONENT16)
	{
		if (target != GL_TEXTURE_CUBE_MAP_ARRAY)
		{
			if (s.coordType == "float")
				s.coordType = "vec3";
			else if (s.coordType == "vec2")
				s.coordType = "vec3";
			else if (s.coordType == "vec3")
				s.coordType = "vec4";
			s.coordDef += s.refZDef;
		}
		else
			s.cubeMapArrayRefZDef = s.refZDef;

		s.componentDef = ".r";
	}

	// Set level of details definition
	if (target != GL_TEXTURE_RECTANGLE && target != GL_TEXTURE_2D_MULTISAMPLE &&
		target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
	{
		s.lod	= de::toString(level);
		s.lodDef = ", " + de::toString(level);
	}

	// Set proper coord vector
	if (target == GL_TEXTURE_RECTANGLE || funcName.find("Fetch") != std::string::npos ||
		funcName.find("ImageLoad") != std::string::npos)
	{
		s.pointCoord = "icoord";
	}
	else
		s.pointCoord = "coord";

	// Set size vector definition
	if (format != GL_DEPTH_COMPONENT16 || funcName.find("Gather", 0) != std::string::npos)
	{
		if (s.coordType == "float")
			s.sizeDef = "<TEX_WIDTH>";
		else if (s.coordType == "vec2" && target == GL_TEXTURE_1D_ARRAY)
			s.sizeDef = "<TEX_WIDTH>, <TEX_DEPTH>";
		else if (s.coordType == "vec2")
			s.sizeDef = "<TEX_WIDTH>, <TEX_HEIGHT>";
		else if (s.coordType == "vec3")
			s.sizeDef = "<TEX_WIDTH>, <TEX_HEIGHT>, <TEX_DEPTH>";
		else if (s.coordType == "vec4")
			s.sizeDef = "<TEX_WIDTH>, <TEX_HEIGHT>, floor(<TEX_DEPTH> / 6), 6";
	}
	// Set size vector for shadow samplers and non-gether functions selected
	else
	{
		if (s.coordType == "vec3" && target == GL_TEXTURE_1D)
			s.sizeDef = "<TEX_WIDTH>, 1 , 1";
		else if (s.coordType == "vec3" && target == GL_TEXTURE_1D_ARRAY)
			s.sizeDef = "<TEX_WIDTH>, <TEX_DEPTH>, 1";
		else if (s.coordType == "vec3")
			s.sizeDef = "<TEX_WIDTH>, <TEX_HEIGHT>, 1";
		else if (s.coordType == "vec4")
			s.sizeDef = "<TEX_WIDTH>, <TEX_HEIGHT>, <TEX_DEPTH>, 1";
	}

	if (s.coordType != "float")
		s.iCoordType = "i" + s.coordType;
	else
		s.iCoordType = "int";

	return s;
}

/** Check if specific combination of target and format is

 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 *
 * @return Returns true if target/format combination is allowed, false otherwise.
 */
bool SparseTexture2LookupTestCase::caseAllowed(GLint target, GLint format)
{
	DE_UNREF(target);

	// As shaders do not support some texture formats it is necessary to exclude them.
	if (format == GL_RGB565 || format == GL_RGB10_A2UI || format == GL_RGB9_E5)
	{
		return false;
	}

	if ((target == GL_TEXTURE_2D_MULTISAMPLE || target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY || target == GL_TEXTURE_3D) &&
		(format == GL_DEPTH_COMPONENT16))
	{
		return false;
	}

	return true;
}

/** Check if specific lookup function is allowed for specific target and format
 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param funcToken    Texture lookup function structure
 *
 * @return Returns true if target/format combination is allowed, false otherwise.
 */
bool SparseTexture2LookupTestCase::funcAllowed(GLint target, GLint format, FunctionToken& funcToken)
{
	if (funcToken.allowedTargets.find(target) == funcToken.allowedTargets.end())
		return false;

	if (format == GL_DEPTH_COMPONENT16)
	{
		if (funcToken.name == "sparseTextureLodARB" || funcToken.name == "sparseTextureLodOffsetARB")
		{
			if (target != GL_TEXTURE_2D)
				return false;
		}
		else if (funcToken.name == "sparseTextureOffsetARB" || funcToken.name == "sparseTextureGradOffsetARB" ||
				 funcToken.name == "sparseTextureGatherOffsetARB" || funcToken.name == "sparseTextureGatherOffsetsARB")
		{
			if (target != GL_TEXTURE_2D && target != GL_TEXTURE_2D_ARRAY && target != GL_TEXTURE_RECTANGLE)
			{
				return false;
			}
		}
		else if (funcToken.name == "sparseTexelFetchARB" || funcToken.name == "sparseTexelFetchOffsetARB")
		{
			return false;
		}
		else if (funcToken.name == "sparseTextureGradARB")
		{
			if (target == GL_TEXTURE_CUBE_MAP_ARRAY)
				return false;
		}
		else if (funcToken.name == "sparseImageLoadARB")
		{
			return false;
		}
	}

	return true;
}

/** Writing data to generated texture using compute shader
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTexture2LookupTestCase::writeDataToTexture(const Functions& gl, GLint target, GLint format, GLuint& texture,
													  GLint level)
{
	mLog << "Fill Texture with shader [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (width > 0 && height > 0 && depth >= mState.minDepth)
	{
		if (target == GL_TEXTURE_CUBE_MAP)
			depth = depth * 6;

		GLint texSize = width * height * depth * mState.format.getPixelSize();

		std::vector<GLubyte> vecData;
		vecData.resize(texSize);
		GLubyte* data = vecData.data();

		deMemset(data, 255, texSize);

		for (GLint sample = 0; sample < mState.samples; ++sample)
		{
			std::string shader = st2_compute_textureFill;

			// Adjust shader source to texture format
			TokenStrings s = createShaderTokens(target, format, sample);

			replaceToken("<INPUT_TYPE>", s.inputType.c_str(), shader);
			replaceToken("<POINT_TYPE>", s.pointType.c_str(), shader);
			replaceToken("<POINT_DEF>", s.pointDef.c_str(), shader);
			replaceToken("<RETURN_TYPE>", s.returnType.c_str(), shader);
			replaceToken("<RESULT_EXPECTED>", s.resultExpected.c_str(), shader);
			replaceToken("<SAMPLE_DEF>", s.sampleDef.c_str(), shader);

			ProgramSources sources;
			sources << ComputeSource(shader);

			GLint convFormat = format;
			if (format == GL_DEPTH_COMPONENT16)
				convFormat = GL_R16;

			// Build and run shader
			ShaderProgram program(m_context.getRenderContext(), sources);
			if (program.isOk())
			{
				gl.useProgram(program.getProgram());
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");
				gl.bindImageTexture(0 /* unit */, texture, level /* level */, GL_FALSE /* layered */, 0 /* layer */,
									GL_WRITE_ONLY, convFormat);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
				gl.uniform1i(1, 0 /* image_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
				gl.dispatchCompute(width, height, depth);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute");
				gl.memoryBarrier(GL_ALL_BARRIER_BITS);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier");
			}
			else
			{
				mLog << "Compute shader compilation failed (writing) for target: " << target << ", format: " << format
					 << ", sample: " << sample << ", infoLog: " << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog
					 << ", shaderSource: " << shader.c_str() << " - ";
			}
		}
	}

	return true;
}

/** Setup depth compare mode and compare function for depth texture
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param texture      Texture object
 */
void SparseTexture2LookupTestCase::setupDepthMode(const Functions& gl, GLint target, GLuint& texture)
{
	Texture::Bind(gl, texture, target);
	gl.texParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
}

/** Verify if data stored in texture is as expected
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 * @param funcToken    Lookup function tokenize structure
 *
 * @return Returns true if data is as expected, false if not, throws an exception if error occurred.
 */
bool SparseTexture2LookupTestCase::verifyLookupTextureData(const Functions& gl, GLint target, GLint format,
														   GLuint& texture, GLint level, FunctionToken& funcToken)
{
	mLog << "Verify Lookup Texture Data [function: " << funcToken.name << ", level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	//Committed region is limited to 1/2 of width
	GLint widthCommitted = width / 2;

	if (widthCommitted == 0 || height == 0 || depth < mState.minDepth)
		return true;

	bool result = true;

	if (target == GL_TEXTURE_CUBE_MAP)
		depth = depth * 6;

	GLint texSize = width * height * depth;

	std::vector<GLubyte> vecExpData;
	std::vector<GLubyte> vecOutData;
	vecExpData.resize(texSize);
	vecOutData.resize(texSize);
	GLubyte* exp_data = vecExpData.data();
	GLubyte* out_data = vecOutData.data();

	// Expected data is 255 because
	deMemset(exp_data, 255, texSize);

	// Make token copy to work on
	FunctionToken f = funcToken;

	// Create verifying texture
	GLint verifyTarget;
	if (target == GL_TEXTURE_2D_MULTISAMPLE)
		verifyTarget = GL_TEXTURE_2D;
	else
		verifyTarget = GL_TEXTURE_2D_ARRAY;

	GLuint verifyTexture;
	Texture::Generate(gl, verifyTexture);
	Texture::Bind(gl, verifyTexture, verifyTarget);
	Texture::Storage(gl, verifyTarget, 1, GL_R8, width, height, depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::Storage");

	for (int sample = 0; sample < mState.samples; ++sample)
	{
		deMemset(out_data, 0, texSize);

		Texture::Bind(gl, verifyTexture, verifyTarget);
		Texture::SubImage(gl, verifyTarget, 0, 0, 0, 0, width, height, depth, GL_RED, GL_UNSIGNED_BYTE,
						  (GLvoid*)out_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::SubImage");

		std::string shader = st2_compute_lookupVerify;

		// Adjust shader source to texture format
		TokenStringsExt s = createLookupShaderTokens(target, format, level, sample, f);

		replaceToken("<FUNCTION>", f.name.c_str(), shader);
		replaceToken("<ARGUMENTS>", f.arguments.c_str(), shader);

		replaceToken("<OUTPUT_TYPE>", s.outputType.c_str(), shader);
		replaceToken("<INPUT_TYPE>", s.inputType.c_str(), shader);
		replaceToken("<SIZE_DEF>", s.sizeDef.c_str(), shader);
		replaceToken("<LOD>", s.lod.c_str(), shader);
		replaceToken("<LOD_DEF>", s.lodDef.c_str(), shader);
		replaceToken("<COORD_TYPE>", s.coordType.c_str(), shader);
		replaceToken("<ICOORD_TYPE>", s.iCoordType.c_str(), shader);
		replaceToken("<COORD_DEF>", s.coordDef.c_str(), shader);
		replaceToken("<POINT_TYPE>", s.pointType.c_str(), shader);
		replaceToken("<POINT_DEF>", s.pointDef.c_str(), shader);
		replaceToken("<RETURN_TYPE>", s.returnType.c_str(), shader);
		replaceToken("<RESULT_EXPECTED>", s.resultExpected.c_str(), shader);
		replaceToken("<EPSILON>", s.epsilon.c_str(), shader);
		replaceToken("<SAMPLE_DEF>", s.sampleDef.c_str(), shader);
		replaceToken("<REFZ_DEF>", s.refZDef.c_str(), shader);
		replaceToken("<CUBE_REFZ_DEF>", s.cubeMapArrayRefZDef.c_str(), shader);
		replaceToken("<POINT_COORD>", s.pointCoord.c_str(), shader);
		replaceToken("<COMPONENT_DEF>", s.componentDef.c_str(), shader);
		replaceToken("<CUBE_MAP_COORD_DEF>", s.cubeMapCoordDef.c_str(), shader);
		replaceToken("<OFFSET_ARRAY_DEF>", s.offsetArrayDef.c_str(), shader);
		replaceToken("<FORMAT_DEF>", s.formatDef.c_str(), shader);
		replaceToken("<OFFSET_TYPE>", s.offsetType.c_str(), shader);
		replaceToken("<NOFFSET_TYPE>", s.nOffsetType.c_str(), shader);
		replaceToken("<OFFSET_DIM>", s.offsetDim.c_str(), shader);

		replaceToken("<TEX_WIDTH>", de::toString(width).c_str(), shader);
		replaceToken("<TEX_HEIGHT>", de::toString(height).c_str(), shader);
		replaceToken("<TEX_DEPTH>", de::toString(depth).c_str(), shader);

		ProgramSources sources;
		sources << ComputeSource(shader);

		// Build and run shader
		ShaderProgram program(m_context.getRenderContext(), sources);
		if (program.isOk())
		{
			gl.useProgram(program.getProgram());
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

			// Pass output image to shader
			gl.bindImageTexture(1, //unit
								verifyTexture,
								0,		 //level
								GL_TRUE, //layered
								0,		 //layer
								GL_WRITE_ONLY, GL_R8UI);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
			gl.uniform1i(1, 1 /* image_unit */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");

			// Pass input sampler/image to shader
			if (f.name != "sparseImageLoadARB")
			{
				gl.activeTexture(GL_TEXTURE0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture");
				gl.bindTexture(target, texture);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");
				gl.uniform1i(2, 0 /* sampler_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
			}
			else
			{
				gl.bindImageTexture(0, //unit
									texture,
									level,	//level
									GL_FALSE, //layered
									0,		  //layer
									GL_READ_ONLY, format);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture");
				gl.uniform1i(2, 0 /* image_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
			}

			// Pass committed region width to shader
			gl.uniform1i(3, widthCommitted /* committed region width */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");
			gl.dispatchCompute(width, height, depth);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute");
			gl.memoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier");

			Texture::Bind(gl, verifyTexture, verifyTarget);
			Texture::GetData(gl, 0, verifyTarget, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)out_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

			//Verify only committed region
			for (GLint z = 0; z < depth; ++z)
				for (GLint y = 0; y < height; ++y)
					for (GLint x = 0; x < width; ++x)
					{
						GLubyte* dataRegion	= exp_data + x + y * width + z * width * height;
						GLubyte* outDataRegion = out_data + x + y * width + z * width * height;
						if (dataRegion[0] != outDataRegion[0])
							result = false;
					}
		}
		else
		{
			mLog << "Compute shader compilation failed (lookup) for target: " << target << ", format: " << format
				 << ", shaderInfoLog: " << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog
				 << ", programInfoLog: " << program.getProgramInfo().infoLog << ", shaderSource: " << shader.c_str()
				 << " - ";

			result = false;
		}
	}

	Texture::Delete(gl, verifyTexture);

	return result;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
SparseTexture2Tests::SparseTexture2Tests(deqp::Context& context)
	: TestCaseGroup(context, "sparse_texture2_tests", "Verify conformance of CTS_ARB_sparse_texture2 implementation")
{
}

/** Initializes the test group contents. */
void SparseTexture2Tests::init()
{
	addChild(new ShaderExtensionTestCase(m_context, "GL_ARB_sparse_texture2"));
	addChild(new StandardPageSizesTestCase(m_context));
	addChild(new SparseTexture2AllocationTestCase(m_context));
	addChild(new SparseTexture2CommitmentTestCase(m_context));
	addChild(new UncommittedRegionsAccessTestCase(m_context));
	addChild(new SparseTexture2LookupTestCase(m_context));
}

} /* gl4cts namespace */
