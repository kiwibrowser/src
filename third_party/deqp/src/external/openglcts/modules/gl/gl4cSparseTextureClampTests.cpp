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
 * \file  gl4cSparseTextureClampTests.cpp
 * \brief Conformance tests for the GL_ARB_sparse_texture2 functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cSparseTextureClampTests.hpp"
#include "deStringUtil.hpp"
#include "gl4cSparseTexture2Tests.hpp"
#include "gl4cSparseTextureTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageIO.hpp"
#include "tcuTestLog.hpp"

#include <cmath>
#include <string.h>
#include <vector>

using namespace glw;
using namespace glu;

namespace gl4cts
{

const char* stc_compute_textureFill = "#version 430 core\n"
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

const char* stc_vertex_common = "#version 450\n"
								"\n"
								"in vec3 vertex;\n"
								"in <COORD_TYPE> inCoord;\n"
								"out <COORD_TYPE> texCoord;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    texCoord = inCoord;\n"
								"    gl_Position = vec4(vertex, 1);\n"
								"}\n";

const char* stc_fragment_lookupResidency = "#version 450 core\n"
										   "\n"
										   "#extension GL_ARB_sparse_texture2 : enable\n"
										   "#extension GL_ARB_sparse_texture_clamp : enable\n"
										   "\n"
										   "in <COORD_TYPE> texCoord;\n"
										   "out vec4 fragColor;\n"
										   "\n"
										   "layout (location = 1<FORMAT_DEF>) uniform <INPUT_TYPE> uni_in;\n"
										   "layout (location = 2) uniform int widthCommitted;\n"
										   "\n"
										   "void main()\n"
										   "{\n"
										   "    <COORD_TYPE> coord = texCoord;\n"
										   "    <ICOORD_TYPE> texSize = <ICOORD_TYPE>(<SIZE_DEF>);\n"
										   "    <POINT_TYPE> point = <POINT_TYPE>(coord * texSize);\n"
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
										   "\n"
										   "    fragColor = vec4(1);\n"
										   "\n"
										   "    if (point.x > corner1.x && point.y > corner1.y &&\n"
										   "        point.x < corner2.x && point.y < corner2.y &&\n"
										   "        point.x < widthCommitted - 1)\n"
										   "    {\n"
										   "        if (!sparseTexelsResidentARB(code) ||\n"
										   "            any(greaterThan(retValue, expValue + epsilon)) ||\n"
										   "            any(lessThan(retValue, expValue - epsilon)))\n"
										   "        {\n"
										   "            fragColor = vec4(0);\n"
										   "        }\n"
										   "    }\n"
										   "\n"
										   "    if (point.x > corner1.x && point.y > corner1.y &&\n"
										   "        point.x < corner2.x && point.y < corner2.y &&\n"
										   "        point.x >= widthCommitted + 1)\n"
										   "    {\n"
										   "        if (sparseTexelsResidentARB(code))\n"
										   "        {\n"
										   "            fragColor = vec4(0);\n"
										   "        }\n"
										   "    }\n"
										   "}\n";

const char* stc_fragment_lookupColor = "#version 450 core\n"
									   "\n"
									   "#extension GL_ARB_sparse_texture2 : enable\n"
									   "#extension GL_ARB_sparse_texture_clamp : enable\n"
									   "\n"
									   "in <COORD_TYPE> texCoord;\n"
									   "out vec4 fragColor;\n"
									   "\n"
									   "layout (location = 1<FORMAT_DEF>) uniform <INPUT_TYPE> uni_in;\n"
									   "\n"
									   "void main()\n"
									   "{\n"
									   "    <COORD_TYPE> coord = texCoord;\n"
									   "    <ICOORD_TYPE> texSize = <ICOORD_TYPE>(<SIZE_DEF>);\n"
									   "    <POINT_TYPE> point = <POINT_TYPE>(coord * texSize);\n"
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
									   "<FUNCTION_DEF>\n"
									   "\n"
									   "    fragColor = vec4(1);\n"
									   "\n"
									   "    if (any(greaterThan(retValue, expValue + epsilon)) ||\n"
									   "        any(lessThan(retValue, expValue - epsilon)))\n"
									   "    {\n"
									   "        fragColor = vec4(0);\n"
									   "    }\n"
									   "}\n";

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTextureClampLookupResidencyTestCase::SparseTextureClampLookupResidencyTestCase(deqp::Context& context)
	: SparseTexture2LookupTestCase(
		  context, "SparseTextureClampLookupResidency",
		  "Verifies if sparse texture clamp lookup functions generates access residency information")
{
	/* Left blank intentionally */
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTextureClampLookupResidencyTestCase::SparseTextureClampLookupResidencyTestCase(deqp::Context& context,
																					 const char*	name,
																					 const char*	description)
	: SparseTexture2LookupTestCase(context, name, description)
{
	/* Left blank intentionally */
}

/** Stub init method */
void SparseTextureClampLookupResidencyTestCase::init()
{
	SparseTextureCommitmentTestCase::init();
	mSupportedInternalFormats.push_back(GL_DEPTH_COMPONENT16);

	FunctionToken f;
	f = FunctionToken("sparseTextureClampARB", "<CUBE_REFZ_DEF>, <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureOffsetClampARB", ", <OFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureGradClampARB",
					  ", <NOFFSET_TYPE><OFFSET_DIM>(0), <NOFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken(
		"sparseTextureGradOffsetClampARB",
		", <NOFFSET_TYPE><OFFSET_DIM>(0), <NOFFSET_TYPE><OFFSET_DIM>(0), <OFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseTextureClampLookupResidencyTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture_clamp"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	return SparseTexture2LookupTestCase::iterate();
}

/** Check if specific lookup function is allowed for specific target and format
 *
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param funcToken    Texture lookup function structure
 *
 * @return Returns true if target/format combination is allowed, false otherwise.
 */
bool SparseTextureClampLookupResidencyTestCase::funcAllowed(GLint target, GLint format, FunctionToken& funcToken)
{
	if (funcToken.allowedTargets.find(target) == funcToken.allowedTargets.end())
		return false;

	if (format == GL_DEPTH_COMPONENT16)
	{
		if (target == GL_TEXTURE_CUBE_MAP_ARRAY &&
			(funcToken.name == "sparseTextureGradClampARB" || funcToken.name == "textureGradClampARB"))
			return false;
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
 * @param funcToken    Lookup function tokenize structure
 *
 * @return Returns true if data is as expected, false if not, throws an exception if error occurred.
 */
bool SparseTextureClampLookupResidencyTestCase::verifyLookupTextureData(const Functions& gl, GLint target, GLint format,
																		GLuint& texture, GLint level,
																		FunctionToken& funcToken)
{
	mLog << "Verify Lookup Residency Texture Data [function: " << funcToken.name << ", level: " << level << "] - ";

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

	GLint texSize = width * height;

	std::vector<GLubyte> vecExpData;
	std::vector<GLubyte> vecOutData;
	vecExpData.resize(texSize);
	vecOutData.resize(texSize);
	GLubyte* exp_data = vecExpData.data();
	GLubyte* out_data = vecOutData.data();

	// Expected data is 255 because
	deMemset(exp_data, 255, texSize);

	// Create verifying texture
	GLint  verifyTarget = GL_TEXTURE_2D;
	GLuint verifyTexture;
	Texture::Generate(gl, verifyTexture);
	Texture::Bind(gl, verifyTexture, verifyTarget);
	Texture::Storage(gl, verifyTarget, 1, GL_R8, width, height, depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::Storage");

	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, verifyTarget, verifyTexture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D");

	gl.viewport(0, 0, width, height);

	for (int sample = 0; sample < mState.samples; ++sample)
	{
		std::string vertex   = stc_vertex_common;
		std::string fragment = stc_fragment_lookupResidency;

		// Make token copy to work on
		FunctionToken f = funcToken;

		// Adjust shader source to texture format
		TokenStringsExt s = createLookupShaderTokens(target, format, level, sample, f);

		replaceToken("<COORD_TYPE>", s.coordType.c_str(), vertex);

		replaceToken("<FUNCTION>", f.name.c_str(), fragment);
		replaceToken("<ARGUMENTS>", f.arguments.c_str(), fragment);

		replaceToken("<OUTPUT_TYPE>", s.outputType.c_str(), fragment);
		replaceToken("<INPUT_TYPE>", s.inputType.c_str(), fragment);
		replaceToken("<SIZE_DEF>", s.sizeDef.c_str(), fragment);
		replaceToken("<LOD>", s.lod.c_str(), fragment);
		replaceToken("<LOD_DEF>", s.lodDef.c_str(), fragment);
		replaceToken("<COORD_TYPE>", s.coordType.c_str(), fragment);
		replaceToken("<ICOORD_TYPE>", s.iCoordType.c_str(), fragment);
		replaceToken("<COORD_DEF>", s.coordDef.c_str(), fragment);
		replaceToken("<POINT_TYPE>", s.pointType.c_str(), fragment);
		replaceToken("<POINT_DEF>", s.pointDef.c_str(), fragment);
		replaceToken("<RETURN_TYPE>", s.returnType.c_str(), fragment);
		replaceToken("<RESULT_EXPECTED>", s.resultExpected.c_str(), fragment);
		replaceToken("<EPSILON>", s.epsilon.c_str(), fragment);
		replaceToken("<SAMPLE_DEF>", s.sampleDef.c_str(), fragment);
		replaceToken("<REFZ_DEF>", s.refZDef.c_str(), fragment);
		replaceToken("<CUBE_REFZ_DEF>", s.cubeMapArrayRefZDef.c_str(), fragment);
		replaceToken("<POINT_COORD>", s.pointCoord.c_str(), fragment);
		replaceToken("<COMPONENT_DEF>", s.componentDef.c_str(), fragment);
		replaceToken("<CUBE_MAP_COORD_DEF>", s.cubeMapCoordDef.c_str(), fragment);
		replaceToken("<OFFSET_ARRAY_DEF>", s.offsetArrayDef.c_str(), fragment);
		replaceToken("<FORMAT_DEF>", s.formatDef.c_str(), fragment);
		replaceToken("<OFFSET_TYPE>", s.offsetType.c_str(), fragment);
		replaceToken("<NOFFSET_TYPE>", s.nOffsetType.c_str(), fragment);
		replaceToken("<OFFSET_DIM>", s.offsetDim.c_str(), fragment);

		replaceToken("<TEX_WIDTH>", de::toString(width).c_str(), fragment);
		replaceToken("<TEX_HEIGHT>", de::toString(height).c_str(), fragment);
		replaceToken("<TEX_DEPTH>", de::toString(depth).c_str(), fragment);

		ProgramSources sources = makeVtxFragSources(vertex.c_str(), fragment.c_str());

		// Build and run shader
		ShaderProgram program(m_context.getRenderContext(), sources);
		if (program.isOk())
		{
			for (GLint z = 0; z < depth; ++z)
			{
				deMemset(out_data, 0, texSize);

				Texture::Bind(gl, verifyTexture, verifyTarget);
				Texture::SubImage(gl, verifyTarget, 0, 0, 0, 0, width, height, 0, GL_RED, GL_UNSIGNED_BYTE,
								  (GLvoid*)out_data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::SubImage");

				// Use shader
				gl.useProgram(program.getProgram());
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

				// Pass input sampler/image to shader
				gl.activeTexture(GL_TEXTURE0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture");
				gl.uniform1i(1, 0 /* sampler_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");

				// Pass committed region width to shader
				gl.uniform1i(2, widthCommitted /* committed region width */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");

				gl.bindTexture(target, texture);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");

				gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				draw(target, z, program);

				Texture::Bind(gl, verifyTexture, verifyTarget);
				Texture::GetData(gl, 0, verifyTarget, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)out_data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

				//Verify only committed region
				for (GLint y = 0; y < height; ++y)
					for (GLint x = 0; x < width; ++x)
					{
						GLubyte* dataRegion	= exp_data + x + y * width;
						GLubyte* outDataRegion = out_data + x + y * width;
						if (dataRegion[0] != outDataRegion[0])
							result = false;
					}
			}
		}
		else
		{
			mLog << "Shader compilation failed (lookup residency) for target: " << target << ", format: " << format
				 << ", vertexInfoLog: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog
				 << ", fragmentInfoLog: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog
				 << ", programInfoLog: " << program.getProgramInfo().infoLog << ", fragmentSource: " << fragment.c_str()
				 << " - ";

			result = false;
		}
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

	Texture::Delete(gl, verifyTexture);

	return result;
}

void SparseTextureClampLookupResidencyTestCase::draw(GLint target, GLint layer, const ShaderProgram& program)
{
	const GLfloat texCoord1D[] = { 0.0f, 1.0f, 0.0f, 1.0f };

	const GLfloat texCoord2D[] = {
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
	};

	const GLfloat texCoord3D[] = { 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.5f, 1.0f, 1.0f, 0.5f };

	const GLfloat texCoordCubeMap[6][12] = {
		{ 0.0f, 0.0f, 0.00f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.17f, 1.0f, 0.0f, 0.17f, 0.0f, 1.0f, 0.17f, 1.0f, 1.0f, 0.17f },
		{ 0.0f, 0.0f, 0.33f, 1.0f, 0.0f, 0.33f, 0.0f, 1.0f, 0.33f, 1.0f, 1.0f, 0.33f },
		{ 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.5f, 1.0f, 1.0f, 0.5f },
		{ 0.0f, 0.0f, 0.67f, 1.0f, 0.0f, 0.67f, 0.0f, 1.0f, 0.67f, 1.0f, 1.0f, 0.67f },
		{ 0.0f, 0.0f, 0.83f, 1.0f, 0.0f, 0.83f, 0.0f, 1.0f, 0.83f, 1.0f, 1.0f, 0.83f }
	};

	const GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
	};

	const GLuint indices[] = { 0, 1, 2, 1, 2, 3 };

	VertexArrayBinding floatCoord;

	if (target == GL_TEXTURE_1D || target == GL_TEXTURE_1D_ARRAY)
		floatCoord = glu::va::Float("inCoord", 1, 4, 0, texCoord1D);
	else if (target == GL_TEXTURE_3D)
		floatCoord = glu::va::Float("inCoord", 3, 4, 0, texCoord3D);
	else if (target == GL_TEXTURE_CUBE_MAP)
		floatCoord = glu::va::Float("inCoord", 3, 4, 0, texCoordCubeMap[layer]);
	else if (target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		GLfloat		  layerCoord			   = GLfloat(layer) / 6.0f + 0.01f;
		const GLfloat texCoordCubeMapArray[16] = { 0.0f, 0.0f, layerCoord, GLfloat(layer),
												   1.0f, 0.0f, layerCoord, GLfloat(layer),
												   0.0f, 1.0f, layerCoord, GLfloat(layer),
												   1.0f, 1.0f, layerCoord, GLfloat(layer) };
		floatCoord = glu::va::Float("inCoord", 4, 4, 0, texCoordCubeMapArray);
	}
	else
		floatCoord = glu::va::Float("inCoord", 2, 4, 0, texCoord2D);

	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("vertex", 3, 4, 0, vertices), floatCoord };

	glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(indices), indices));
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
SparseTextureClampLookupColorTestCase::SparseTextureClampLookupColorTestCase(deqp::Context& context)
	: SparseTextureClampLookupResidencyTestCase(
		  context, "SparseTextureClampLookupColor",
		  "Verifies if sparse and non-sparse texture clamp lookup functions works as expected")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SparseTextureClampLookupColorTestCase::init()
{
	SparseTextureCommitmentTestCase::init();
	mSupportedTargets.push_back(GL_TEXTURE_1D);
	mSupportedTargets.push_back(GL_TEXTURE_1D_ARRAY);
	mSupportedInternalFormats.push_back(GL_DEPTH_COMPONENT16);

	FunctionToken f;
	f = FunctionToken("sparseTextureClampARB", "<CUBE_REFZ_DEF>, <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("textureClampARB", "<CUBE_REFZ_DEF>, <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_1D);
	f.allowedTargets.insert(GL_TEXTURE_1D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureOffsetClampARB", ", <OFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("textureOffsetClampARB", ", <OFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_1D);
	f.allowedTargets.insert(GL_TEXTURE_1D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("sparseTextureGradClampARB",
					  ", <NOFFSET_TYPE><OFFSET_DIM>(0), <NOFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken("textureGradClampARB", ", <NOFFSET_TYPE><OFFSET_DIM>(0), <NOFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_1D);
	f.allowedTargets.insert(GL_TEXTURE_1D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP);
	f.allowedTargets.insert(GL_TEXTURE_CUBE_MAP_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken(
		"sparseTextureGradOffsetClampARB",
		", <NOFFSET_TYPE><OFFSET_DIM>(0), <NOFFSET_TYPE><OFFSET_DIM>(0), <OFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);

	f = FunctionToken(
		"textureGradOffsetClampARB",
		", <NOFFSET_TYPE><OFFSET_DIM>(0), <NOFFSET_TYPE><OFFSET_DIM>(0), <OFFSET_TYPE><OFFSET_DIM>(0), <LOD>");
	f.allowedTargets.insert(GL_TEXTURE_1D);
	f.allowedTargets.insert(GL_TEXTURE_1D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_2D);
	f.allowedTargets.insert(GL_TEXTURE_2D_ARRAY);
	f.allowedTargets.insert(GL_TEXTURE_3D);
	mFunctions.push_back(f);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseTextureClampLookupColorTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture_clamp"))
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

				bool isSparse = false;
				if (funcToken.name.find("sparse", 0) != std::string::npos)
					isSparse = true;

				mLog.str("");
				mLog << "Testing sparse texture lookup color functions for target: " << target << ", format: " << format
					 << " - ";

				if (isSparse)
					sparseAllocateTexture(gl, target, format, texture, 3);
				else
					allocateTexture(gl, target, format, texture, 3);

				if (format == GL_DEPTH_COMPONENT16)
					setupDepthMode(gl, target, texture);

				int l;
				int maxLevels = 0;
				for (l = 0; l < mState.levels; ++l)
				{
					if (!isSparse || commitTexturePage(gl, target, format, texture, l))
					{
						writeDataToTexture(gl, target, format, texture, l);
						maxLevels = l;
					}
				}

				for (l = 0; l <= maxLevels; ++l)
				{
					result = result && verifyLookupTextureData(gl, target, format, texture, l, funcToken);

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

/** Writing data to generated texture using compute shader
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTextureClampLookupColorTestCase::writeDataToTexture(const Functions& gl, GLint target, GLint format,
															   GLuint& texture, GLint level)
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
			std::string shader = stc_compute_textureFill;

			// Adjust shader source to texture format
			TokenStrings s = createShaderTokens(target, format, sample);

			GLint convFormat = format;
			if (format == GL_DEPTH_COMPONENT16)
				convFormat = GL_R16;

			// Change expected result as it has to be adjusted to different levels
			s.resultExpected = generateExpectedResult(s.returnType, level, convFormat);

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
bool SparseTextureClampLookupColorTestCase::verifyLookupTextureData(const Functions& gl, GLint target, GLint format,
																	GLuint& texture, GLint level,
																	FunctionToken& funcToken)
{
	mLog << "Verify Lookup Color Texture Data [function: " << funcToken.name << ", level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (width == 0 || height == 0 || depth < mState.minDepth)
		return true;

	bool result = true;

	if (target == GL_TEXTURE_CUBE_MAP)
		depth = depth * 6;

	GLint texSize = width * height;

	std::vector<GLubyte> vecExpData;
	std::vector<GLubyte> vecOutData;
	vecExpData.resize(texSize);
	vecOutData.resize(texSize);
	GLubyte* exp_data = vecExpData.data();
	GLubyte* out_data = vecOutData.data();

	// Expected data is 255 because
	deMemset(exp_data, 255, texSize);

	// Create verifying texture
	GLint  verifyTarget = GL_TEXTURE_2D;
	GLuint verifyTexture;
	Texture::Generate(gl, verifyTexture);
	Texture::Bind(gl, verifyTexture, verifyTarget);
	Texture::Storage(gl, verifyTarget, 1, GL_R8, width, height, depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::Storage");

	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, verifyTarget, verifyTexture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D");

	gl.viewport(0, 0, width, height);

	for (int sample = 0; sample < mState.samples; ++sample)
	{
		std::string vertex   = stc_vertex_common;
		std::string fragment = stc_fragment_lookupColor;

		// Make token copy to work on
		FunctionToken f = funcToken;

		std::string functionDef = generateFunctionDef(f.name);

		// Adjust shader source to texture format
		TokenStringsExt s = createLookupShaderTokens(target, format, level, sample, f);

		// Change expected result as it has to be adjusted to different levels
		s.resultExpected = generateExpectedResult(s.returnType, level, format);

		replaceToken("<COORD_TYPE>", s.coordType.c_str(), vertex);

		replaceToken("<FUNCTION_DEF>", functionDef.c_str(), fragment);
		replaceToken("<FUNCTION>", f.name.c_str(), fragment);
		replaceToken("<ARGUMENTS>", f.arguments.c_str(), fragment);

		replaceToken("<OUTPUT_TYPE>", s.outputType.c_str(), fragment);
		replaceToken("<INPUT_TYPE>", s.inputType.c_str(), fragment);
		replaceToken("<SIZE_DEF>", s.sizeDef.c_str(), fragment);
		replaceToken("<LOD>", s.lod.c_str(), fragment);
		replaceToken("<LOD_DEF>", s.lodDef.c_str(), fragment);
		replaceToken("<COORD_TYPE>", s.coordType.c_str(), fragment);
		replaceToken("<ICOORD_TYPE>", s.coordType.c_str(), fragment);
		replaceToken("<COORD_DEF>", s.coordDef.c_str(), fragment);
		replaceToken("<POINT_TYPE>", s.pointType.c_str(), fragment);
		replaceToken("<POINT_DEF>", s.pointDef.c_str(), fragment);
		replaceToken("<RETURN_TYPE>", s.returnType.c_str(), fragment);
		replaceToken("<RESULT_EXPECTED>", s.resultExpected.c_str(), fragment);
		replaceToken("<EPSILON>", s.epsilon.c_str(), fragment);
		replaceToken("<SAMPLE_DEF>", s.sampleDef.c_str(), fragment);
		replaceToken("<REFZ_DEF>", s.refZDef.c_str(), fragment);
		replaceToken("<CUBE_REFZ_DEF>", s.cubeMapArrayRefZDef.c_str(), fragment);
		replaceToken("<POINT_COORD>", s.pointCoord.c_str(), fragment);
		replaceToken("<COMPONENT_DEF>", s.componentDef.c_str(), fragment);
		replaceToken("<CUBE_MAP_COORD_DEF>", s.cubeMapCoordDef.c_str(), fragment);
		replaceToken("<OFFSET_ARRAY_DEF>", s.offsetArrayDef.c_str(), fragment);
		replaceToken("<FORMAT_DEF>", s.formatDef.c_str(), fragment);
		replaceToken("<OFFSET_TYPE>", s.offsetType.c_str(), fragment);
		replaceToken("<NOFFSET_TYPE>", s.nOffsetType.c_str(), fragment);
		replaceToken("<OFFSET_DIM>", s.offsetDim.c_str(), fragment);

		replaceToken("<TEX_WIDTH>", de::toString(width).c_str(), fragment);
		replaceToken("<TEX_HEIGHT>", de::toString(height).c_str(), fragment);
		replaceToken("<TEX_DEPTH>", de::toString(depth).c_str(), fragment);

		ProgramSources sources = makeVtxFragSources(vertex.c_str(), fragment.c_str());

		// Build and run shader
		ShaderProgram program(m_context.getRenderContext(), sources);

		if (program.isOk())
		{
			for (GLint z = 0; z < depth; ++z)
			{
				deMemset(out_data, 0, texSize);

				Texture::Bind(gl, verifyTexture, verifyTarget);
				Texture::SubImage(gl, verifyTarget, 0, 0, 0, 0, width, height, 0, GL_RED, GL_UNSIGNED_BYTE,
								  (GLvoid*)out_data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::SubImage");

				// Use shader
				gl.useProgram(program.getProgram());
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

				// Pass input sampler/image to shader
				gl.activeTexture(GL_TEXTURE0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture");
				gl.uniform1i(1, 0 /* sampler_unit */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");

				gl.bindTexture(target, texture);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");

				gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				draw(target, z, program);

				Texture::Bind(gl, verifyTexture, verifyTarget);
				Texture::GetData(gl, 0, verifyTarget, GL_RED, GL_UNSIGNED_BYTE, (GLvoid*)out_data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Texture::GetData");

				//Verify only committed region
				for (GLint y = 0; y < height; ++y)
					for (GLint x = 0; x < width; ++x)
					{
						GLubyte* dataRegion	= exp_data + x + y * width;
						GLubyte* outDataRegion = out_data + x + y * width;
						if (dataRegion[0] != outDataRegion[0])
							result = false;
					}
			}
		}
		else
		{
			mLog << "Shader compilation failed (lookup color) for target: " << target << ", format: " << format
				 << ", vertexInfoLog: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog
				 << ", fragmentInfoLog: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog
				 << ", programInfoLog: " << program.getProgramInfo().infoLog << ", fragmentSource: " << fragment.c_str()
				 << " - ";

			result = false;
		}
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

	Texture::Delete(gl, verifyTexture);

	return result;
}

/** Preparing texture. Function overridden to increase textures size.
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 *
 * @return Returns true if no error occurred, otherwise throws an exception.
 */
bool SparseTextureClampLookupColorTestCase::prepareTexture(const Functions& gl, GLint target, GLint format,
														   GLuint& texture)
{
	Texture::Generate(gl, texture);
	Texture::Bind(gl, texture, target);

	mState.minDepth = SparseTextureUtils::getTargetDepth(target);
	SparseTextureUtils::getTexturePageSizes(gl, target, format, mState.pageSizeX, mState.pageSizeY, mState.pageSizeZ);

	//The <width> and <height> has to be equal for cube map textures
	if (target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_CUBE_MAP_ARRAY)
	{
		if (mState.pageSizeX > mState.pageSizeY)
			mState.pageSizeY = mState.pageSizeX;
		else if (mState.pageSizeX < mState.pageSizeY)
			mState.pageSizeX = mState.pageSizeY;
	}

	mState.width  = 4 * mState.pageSizeX;
	mState.height = 4 * mState.pageSizeY;
	mState.depth  = 4 * mState.pageSizeZ * mState.minDepth;

	mState.format = glu::mapGLInternalFormat(format);

	return true;
}

/** Commit texture page using texPageCommitment function. Function overridden to commit whole texture region.
 *
 * @param gl           GL API functions
 * @param target       Target for which texture is binded
 * @param format       Texture internal format
 * @param texture      Texture object
 * @param level        Texture mipmap level
 *
 * @return Returns true if commitment is done properly, false if commitment is not allowed or throws exception if error occurred.
 */
bool SparseTextureClampLookupColorTestCase::commitTexturePage(const Functions& gl, GLint target, GLint format,
															  GLuint& texture, GLint level)
{
	mLog << "Commit Region [level: " << level << "] - ";

	if (level > mState.levels - 1)
		TCU_FAIL("Invalid level");

	// Avoid not allowed commitments
	if (!isInPageSizesRange(target, level) || !isPageSizesMultiplication(target, level))
	{
		mLog << "Skip commitment [level: " << level << "] - ";
		return false;
	}

	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (target == GL_TEXTURE_CUBE_MAP)
		depth = 6 * depth;

	Texture::Bind(gl, texture, target);
	texPageCommitment(gl, target, format, texture, level, 0, 0, 0, width, height, depth, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texPageCommitment");

	return true;
}

/** Check if current texture size for level is greater or equal page size in a corresponding direction
 *
 * @param target  Target for which texture is binded
 * @param level   Texture mipmap level
 *
 * @return Returns true if the texture size condition is fulfilled, false otherwise.
 */
bool SparseTextureClampLookupColorTestCase::isInPageSizesRange(GLint target, GLint level)
{
	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (target == GL_TEXTURE_CUBE_MAP)
		depth = 6 * depth;

	if (width >= mState.pageSizeX && height >= mState.pageSizeY && (mState.minDepth == 0 || depth >= mState.pageSizeZ))
	{
		return true;
	}

	return false;
}

/** Check if current texture size for level is page size multiplication in a corresponding direction
 *
 * @param target  Target for which texture is binded
 * @param level   Texture mipmap level
 *
 * @return Returns true if the texture size condition is fulfilled, false otherwise.
 */
bool SparseTextureClampLookupColorTestCase::isPageSizesMultiplication(GLint target, GLint level)
{
	GLint width;
	GLint height;
	GLint depth;
	SparseTextureUtils::getTextureLevelSize(target, mState, level, width, height, depth);

	if (target == GL_TEXTURE_CUBE_MAP)
		depth = 6 * depth;

	if ((width % mState.pageSizeX) == 0 && (height % mState.pageSizeY) == 0 && (depth % mState.pageSizeZ) == 0)
	{
		return true;
	}

	return false;
}

/** Constructor.
 *
 * @param funcName Tested function name.
 *
 * @return Returns shader source code part that uses lookup function to fetch texel from texture.
 */
std::string SparseTextureClampLookupColorTestCase::generateFunctionDef(std::string funcName)
{
	if (funcName.find("sparse", 0) != std::string::npos)
	{
		return std::string("    <FUNCTION>(uni_in,\n"
						   "               <POINT_COORD><SAMPLE_DEF><ARGUMENTS>,\n"
						   "               retValue<COMPONENT_DEF>);\n");
	}
	else
	{
		return std::string("    retValue<COMPONENT_DEF> = <FUNCTION>(uni_in,\n"
						   "                                         <POINT_COORD><SAMPLE_DEF><ARGUMENTS>);\n");
	}
}

/** Constructor.
 *
 * @param returnType Expected result variable type.
 *
 * @return Returns shader source token that represent expected lookup result value.
 */
std::string SparseTextureClampLookupColorTestCase::generateExpectedResult(std::string returnType, GLint level,
																		  GLint format)
{
	if (format == GL_DEPTH_COMPONENT16)
		return std::string("(1, 0, 0, 0)");
	else if (returnType == "vec4")
		return std::string("(") + de::toString(0.5f + (float)level / 10) + std::string(", 0, 0, 1)");
	else
		return std::string("(") + de::toString(level * 10) + std::string(", 0, 0, 1)");
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
SparseTextureClampTests::SparseTextureClampTests(deqp::Context& context)
	: TestCaseGroup(context, "sparse_texture_clamp_tests",
					"Verify conformance of CTS_ARB_sparse_texture_clamp implementation")
{
}

/** Initializes the test group contents. */
void SparseTextureClampTests::init()
{
	addChild(new ShaderExtensionTestCase(m_context, "GL_ARB_sparse_texture_clamp"));
	addChild(new SparseTextureClampLookupResidencyTestCase(m_context));
	addChild(new SparseTextureClampLookupColorTestCase(m_context));
}

} /* gl4cts namespace */
