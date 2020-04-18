#ifndef _GLCTEXTUREFILTERANISOTROPICTESTS_HPP
#define _GLCTEXTUREFILTERANISOTROPICTESTS_HPP
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
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  glcTextureFilterAnisotropicTests.hpp
 * \brief Conformance tests for the GL_EXT_texture_filter_anisotropic functionality.
 */ /*-------------------------------------------------------------------*/
#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

using namespace glw;
using namespace glu;

namespace glcts
{

class TextureFilterAnisotropicQueriesTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	TextureFilterAnisotropicQueriesTestCase(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	bool verifyTexParameters(const glw::Functions& gl);
	bool verifyGet(const glw::Functions& gl);

	/* Private members */
};

class TextureFilterAnisotropicDrawingTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	TextureFilterAnisotropicDrawingTestCase(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void generateTexture(const glw::Functions& gl, GLenum target);
	void fillTexture(const glw::Functions& gl, GLenum target, GLenum internalFormat);
	bool drawTexture(const glw::Functions& gl, GLenum target, GLfloat anisoDegree);
	GLuint verifyScene(const glw::Functions& gl);
	void releaseTexture(const glw::Functions& gl);

	void generateTokens(GLenum target, std::string& refTexCoordType, std::string& refSamplerType);

	/* Private members */
	const char* m_vertex;
	const char* m_fragment;

	std::vector<GLenum> m_supportedTargets;
	std::vector<GLenum> m_supportedInternalFormats;

	GLuint m_texture;
};

/** Test group which encapsulates all conformance tests */
class TextureFilterAnisotropicTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	TextureFilterAnisotropicTests(deqp::Context& context);

	void init();

private:
	TextureFilterAnisotropicTests(const TextureFilterAnisotropicTests& other);
	TextureFilterAnisotropicTests& operator=(const TextureFilterAnisotropicTests& other);
};

} /* glcts namespace */

#endif // _GLCTEXTUREFILTERANISOTROPICTESTS_HPP
