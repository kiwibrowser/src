/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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

/*!
 * \file esextcTextureBorderClampSamplingTextureGroup.cpp
 * \brief Test Group for Texture Border Clamp Sampling Texture Tests (Test 7)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBorderClampSamplingTextureGroup.hpp"
#include "esextcTextureBorderClampSamplingTexture.cpp"
#include "glwEnums.hpp"

namespace glcts
{

/** Constructor
 *
 * @param context       Test context
 * @param glslVersion   GLSL version
 **/
TextureBorderClampSamplingTextureGroup::TextureBorderClampSamplingTextureGroup(glcts::Context&		context,
																			   const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "sampling_texture", "Texture Border Clamp Sampling Texture Tests")
{
	/* No implementation needed */
}

/** Initializes test cases for Texture Border Clamp Sampling Texture (Test 7)
 **/
void TextureBorderClampSamplingTextureGroup::init(void)
{
	/* Initialize base class */
	TestCaseGroupBase::init();

	/* Filtering GL_NEAREST */

	/* Target GL_TEXTURE_2D */

	/* GL_RGBA32F */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DRGBA32F(
		4, 4, GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA8, GL_NEAREST, GL_RGBA, GL_RGBA, 256, 256, 1, 0.0f, 1.0f, 0, 255,
		GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DRGBA32F", "Test 7", configurationTexture2DRGBA32F));

	/* GL_R32I */
	TestConfiguration<glw::GLint, glw::GLint> configurationTexture2DR32I(1, 4, GL_TEXTURE_2D, GL_R32I, GL_R32I,
																		 GL_NEAREST, GL_RED_INTEGER, GL_RGBA_INTEGER,
																		 256, 256, 1, 0, 255, 0, 255, GL_INT, GL_INT);

	addChild(new TextureBorderClampSamplingTexture<glw::GLint, glw::GLint>(m_context, m_extParams, "Texture2DR32I",
																		   "Test 7", configurationTexture2DR32I));

	/* GL_R32UI */
	TestConfiguration<glw::GLuint, glw::GLuint> configurationTexture2DR32UI(
		1, 4, GL_TEXTURE_2D, GL_R32UI, GL_R32UI, GL_NEAREST, GL_RED_INTEGER, GL_RGBA_INTEGER, 256, 256, 1, 0, 255, 0,
		255, GL_UNSIGNED_INT, GL_UNSIGNED_INT);

	addChild(new TextureBorderClampSamplingTexture<glw::GLuint, glw::GLuint>(m_context, m_extParams, "Texture2DR32UI",
																			 "Test 7", configurationTexture2DR32UI));

	/* GL_RGBA8 */
	TestConfiguration<glw::GLubyte, glw::GLubyte> configurationTexture2DRGBA8(
		4, 4, GL_TEXTURE_2D, GL_RGBA8, GL_RGBA8, GL_NEAREST, GL_RGBA, GL_RGBA, 256, 256, 1, 0, 255, 0, 255,
		GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLubyte, glw::GLubyte>(m_context, m_extParams, "Texture2DRGBA8",
																			   "Test 7", configurationTexture2DRGBA8));

	/* GL_DEPTH_COMPONENT32F */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DDC32F(
		1, 1, GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_R8, GL_NEAREST, GL_DEPTH_COMPONENT, GL_RED, 256, 256, 1, 0, 255,
		0, 255, GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(m_context, m_extParams, "Texture2DDC32F",
																			   "Test 7", configurationTexture2DDC32F));

	/* GL_DEPTH_COMPONENT16 */
	TestConfiguration<glw::GLushort, glw::GLubyte> configurationTexture2DDC16(
		1, 1, GL_TEXTURE_2D, GL_DEPTH_COMPONENT16, GL_R8, GL_NEAREST, GL_DEPTH_COMPONENT, GL_RED, 256, 256, 1, 0, 255,
		0, 255, GL_UNSIGNED_SHORT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLushort, glw::GLubyte>(m_context, m_extParams, "Texture2DDC16",
																				"Test 7", configurationTexture2DDC16));

	/* GL_COMPRESSED_RGBA8_ETC2_EAC */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DCompressed(
		4, 4, GL_TEXTURE_2D, GL_COMPRESSED_RGBA8_ETC2_EAC, GL_RGBA8, GL_NEAREST, GL_RGBA, GL_RGBA, 64, 64, 1, 0, 1, 0,
		255, GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DCompressed", "Test 7", configurationTexture2DCompressed));

	/* Target GL_TEXTURE_2D_ARRAY */

	/* GL_RGBA32F */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DArrayRGBA32F(
		4, 4, GL_TEXTURE_2D_ARRAY, GL_RGBA32F, GL_RGBA8, GL_NEAREST, GL_RGBA, GL_RGBA, 256, 256, 6, 0.0f, 1.0f, 0, 255,
		GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DArrayRGBA32F", "Test 7", configurationTexture2DArrayRGBA32F));

	/* GL_R32I */
	TestConfiguration<glw::GLint, glw::GLint> configurationTexture2DArrayR32I(
		1, 4, GL_TEXTURE_2D_ARRAY, GL_R32I, GL_R32I, GL_NEAREST, GL_RED_INTEGER, GL_RGBA_INTEGER, 256, 256, 6, 0, 255,
		0, 255, GL_INT, GL_INT);

	addChild(new TextureBorderClampSamplingTexture<glw::GLint, glw::GLint>(m_context, m_extParams, "Texture2DArrayR32I",
																		   "Test 7", configurationTexture2DArrayR32I));

	/* GL_R32UI */
	TestConfiguration<glw::GLuint, glw::GLuint> configurationTexture2DArrayR32UI(
		1, 4, GL_TEXTURE_2D_ARRAY, GL_R32UI, GL_R32UI, GL_NEAREST, GL_RED_INTEGER, GL_RGBA_INTEGER, 256, 256, 6, 0, 255,
		0, 255, GL_UNSIGNED_INT, GL_UNSIGNED_INT);

	addChild(new TextureBorderClampSamplingTexture<glw::GLuint, glw::GLuint>(
		m_context, m_extParams, "Texture2DArrayR32UI", "Test 7", configurationTexture2DArrayR32UI));

	/* GL_RGBA8 */
	TestConfiguration<glw::GLubyte, glw::GLubyte> configurationTexture2DArrayRGBA8(
		4, 4, GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_RGBA8, GL_NEAREST, GL_RGBA, GL_RGBA, 256, 256, 6, 0, 255, 0, 255,
		GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLubyte, glw::GLubyte>(
		m_context, m_extParams, "Texture2DArrayRGBA8", "Test 7", configurationTexture2DArrayRGBA8));

	/* GL_COMPRESSED_RGBA8_ETC2_EAC */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DArrayCompressed(
		4, 4, GL_TEXTURE_2D_ARRAY, GL_COMPRESSED_RGBA8_ETC2_EAC, GL_RGBA8, GL_NEAREST, GL_RGBA, GL_RGBA, 64, 64, 6, 0,
		1, 0, 255, GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DArrayCompressed", "Test 7", configurationTexture2DArrayCompressed));

	/* Target GL_TEXTURE_3D */

	/* GL_RGBA32F */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture3DRGBA32F(
		4, 4, GL_TEXTURE_3D, GL_RGBA32F, GL_RGBA8, GL_NEAREST, GL_RGBA, GL_RGBA, 256, 256, 6, 0.0f, 1.0f, 0, 255,
		GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture3DRGBA32F", "Test 7", configurationTexture3DRGBA32F));

	/* GL_R32I */
	TestConfiguration<glw::GLint, glw::GLint> configurationTexture3DR32I(1, 4, GL_TEXTURE_3D, GL_R32I, GL_R32I,
																		 GL_NEAREST, GL_RED_INTEGER, GL_RGBA_INTEGER,
																		 256, 256, 6, 0, 255, 0, 255, GL_INT, GL_INT);

	addChild(new TextureBorderClampSamplingTexture<glw::GLint, glw::GLint>(m_context, m_extParams, "Texture3DR32I",
																		   "Test 7", configurationTexture3DR32I));

	/* GL_R32UI */
	TestConfiguration<glw::GLuint, glw::GLuint> configurationTexture3DR32UI(
		1, 4, GL_TEXTURE_3D, GL_R32UI, GL_R32UI, GL_NEAREST, GL_RED_INTEGER, GL_RGBA_INTEGER, 256, 256, 6, 0, 255, 0,
		255, GL_UNSIGNED_INT, GL_UNSIGNED_INT);

	addChild(new TextureBorderClampSamplingTexture<glw::GLuint, glw::GLuint>(m_context, m_extParams, "Texture3DR32UI",
																			 "Test 7", configurationTexture3DR32UI));

	/* GL_RGBA8 */
	TestConfiguration<glw::GLubyte, glw::GLubyte> configurationTexture3DRGBA8(
		4, 4, GL_TEXTURE_3D, GL_RGBA8, GL_RGBA8, GL_NEAREST, GL_RGBA, GL_RGBA, 256, 256, 6, 0, 255, 0, 255,
		GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLubyte, glw::GLubyte>(m_context, m_extParams, "Texture3DRGBA8",
																			   "Test 7", configurationTexture3DRGBA8));

	/* Filtering GL_LINEAR */

	/* Target GL_TEXTURE_2D */

	/* GL_RGBA32F */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DRGBA32FLinear(
		4, 4, GL_TEXTURE_2D, GL_RGBA32F, GL_RGBA8, GL_LINEAR, GL_RGBA, GL_RGBA, 256, 256, 1, 0.0f, 1.0f, 0, 255,
		GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DRGBA32FLinear", "Test 7", configurationTexture2DRGBA32FLinear));

	/* GL_RGBA8 */
	TestConfiguration<glw::GLubyte, glw::GLubyte> configurationTexture2DRGBA8Linear(
		4, 4, GL_TEXTURE_2D, GL_RGBA8, GL_RGBA8, GL_LINEAR, GL_RGBA, GL_RGBA, 256, 256, 1, 0, 255, 0, 255,
		GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLubyte, glw::GLubyte>(
		m_context, m_extParams, "Texture2DRGBA8Linear", "Test 7", configurationTexture2DRGBA8Linear));

	/* GL_DEPTH_COMPONENT32F */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DDC32FLinear(
		1, 1, GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_R8, GL_LINEAR, GL_DEPTH_COMPONENT, GL_RED, 256, 256, 1, 0, 255,
		0, 255, GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DDC32FLinear", "Test 7", configurationTexture2DDC32FLinear));

	/* GL_DEPTH_COMPONENT16 */
	TestConfiguration<glw::GLushort, glw::GLubyte> configurationTexture2DDC16Linear(
		1, 1, GL_TEXTURE_2D, GL_DEPTH_COMPONENT16, GL_R8, GL_LINEAR, GL_DEPTH_COMPONENT, GL_RED, 256, 256, 1, 0, 255, 0,
		255, GL_UNSIGNED_SHORT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLushort, glw::GLubyte>(
		m_context, m_extParams, "Texture2DDC16Linear", "Test 7", configurationTexture2DDC16Linear));

	/* GL_COMPRESSED_RGBA8_ETC2_EAC */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DCompressedLinear(
		4, 4, GL_TEXTURE_2D, GL_COMPRESSED_RGBA8_ETC2_EAC, GL_RGBA8, GL_LINEAR, GL_RGBA, GL_RGBA, 64, 64, 1, 0, 1, 0,
		255, GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DCompressedLinear", "Test 7", configurationTexture2DCompressedLinear));

	/* Target GL_TEXTURE_2D_ARRAY */

	/* GL_RGBA32F */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DArrayRGBA32FLinear(
		4, 4, GL_TEXTURE_2D_ARRAY, GL_RGBA32F, GL_RGBA8, GL_LINEAR, GL_RGBA, GL_RGBA, 256, 256, 6, 0.0f, 1.0f, 0, 255,
		GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DArrayRGBA32FLinear", "Test 7", configurationTexture2DArrayRGBA32FLinear));

	/* GL_RGBA8 */
	TestConfiguration<glw::GLubyte, glw::GLubyte> configurationTexture2DArrayRGBA8Linear(
		4, 4, GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_RGBA8, GL_LINEAR, GL_RGBA, GL_RGBA, 256, 256, 6, 0, 255, 0, 255,
		GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLubyte, glw::GLubyte>(
		m_context, m_extParams, "Texture2DArrayRGBA8Linear", "Test 7", configurationTexture2DArrayRGBA8Linear));

	/* GL_COMPRESSED_RGBA8_ETC2_EAC */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture2DArrayCompressedLinear(
		4, 4, GL_TEXTURE_2D_ARRAY, GL_COMPRESSED_RGBA8_ETC2_EAC, GL_RGBA8, GL_LINEAR, GL_RGBA, GL_RGBA, 64, 64, 6, 0, 1,
		0, 255, GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture2DArrayCompressedLinear", "Test 7",
		configurationTexture2DArrayCompressedLinear));

	/* Target GL_TEXTURE_3D*/

	/* GL_RGBA32F */
	TestConfiguration<glw::GLfloat, glw::GLubyte> configurationTexture3DRGBA32FLinear(
		4, 4, GL_TEXTURE_3D, GL_RGBA32F, GL_RGBA8, GL_LINEAR, GL_RGBA, GL_RGBA, 256, 256, 6, 0.0f, 1.0f, 0, 255,
		GL_FLOAT, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLfloat, glw::GLubyte>(
		m_context, m_extParams, "Texture3DRGBA32FLinear", "Test 7", configurationTexture3DRGBA32FLinear));

	/* GL_RGBA8 */
	TestConfiguration<glw::GLubyte, glw::GLubyte> configurationTexture3DRGBA8Linear(
		4, 4, GL_TEXTURE_3D, GL_RGBA8, GL_RGBA8, GL_LINEAR, GL_RGBA, GL_RGBA, 256, 256, 6, 0, 255, 0, 255,
		GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE);

	addChild(new TextureBorderClampSamplingTexture<glw::GLubyte, glw::GLubyte>(
		m_context, m_extParams, "Texture3DRGBA8Linear", "Test 7", configurationTexture3DRGBA8Linear));
}

} // namespace glcts
