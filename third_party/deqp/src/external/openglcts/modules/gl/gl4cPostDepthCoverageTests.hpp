#ifndef _GL4CPOSTDEPTHCOVERAGETESTS_HPP
#define _GL4CPOSTDEPTHCOVERAGETESTS_HPP
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
 * \file  gl4cPostDepthCoverageTests.hpp
 * \brief Conformance tests for the GL_ARB_post_depth_coverage functionality.
 */ /*-------------------------------------------------------------------*/
#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

namespace gl4cts
{
/** Verify if:
 *  * #extension GL_ARB_post_depth_coverage : <behavior> is allowed in the shader.
 *  * #define GL_ARB_post_depth_coverage is available and equal to 1.
 *  * Shader with layout(early_fragment_tests) in; builds without an error.
 *  * Shader with layout(post_depth_coverage) in; builds without an error.
 */
class PostDepthShaderCase : public deqp::TestCase
{
public:
	/* Public methods */
	PostDepthShaderCase(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	/* Private members */
	std::string m_vertShader;
	std::string m_fragShader1;
	std::string m_fragShader2;
	std::string m_fragShader3;
	std::string m_fragShader4;
};

/** Verify if fragment shader is allowed to control whether values in
 *  gl_SampleMaskIn[] reflect the coverage after application of the early
 *  depth and stencil tests.
 */
class PostDepthSampleMaskCase : public deqp::TestCase
{
public:
	/* Public methods */
	PostDepthSampleMaskCase(deqp::Context& context);

	virtual void						 deinit();
	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private enums */
	enum BufferCase
	{
		BUFFERCASE_FIRST   = 0,
		BUFFERCASE_DEPTH   = BUFFERCASE_FIRST,
		BUFFERCASE_STENCIL = BUFFERCASE_FIRST + 1,
		BUFFERCASE_LAST	= BUFFERCASE_STENCIL,
	};

	enum PDCCase
	{
		PDCCASE_FIRST	= 0,
		PDCCASE_DISABLED = PDCCASE_FIRST,
		PDCCASE_ENABLED  = PDCCASE_FIRST + 1,
		PDCCASE_LAST	 = PDCCASE_ENABLED,
	};

	/* Private methods */
	/* Private members */
	deUint32 m_framebufferMS;
	deUint32 m_framebuffer;
	deUint32 m_textureMS;
	deUint32 m_texture;
	deUint32 m_depthStencilRenderbuffer;

	std::string m_vertShader;
	std::string m_fragShader1a;
	std::string m_fragShader1b;
	std::string m_fragShader2;
};

/** Test group which encapsulates all sparse buffer conformance tests */
class PostDepthCoverage : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	PostDepthCoverage(deqp::Context& context);

	void init();

private:
	PostDepthCoverage(const PostDepthCoverage& other);
	PostDepthCoverage& operator=(const PostDepthCoverage& other);
};

} /* glcts namespace */

#endif // _GL4CPOSTDEPTHCOVERAGETESTS_HPP
