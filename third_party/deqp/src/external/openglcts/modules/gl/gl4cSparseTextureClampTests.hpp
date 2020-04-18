#ifndef _GL4CSPARSETEXTURECLAMPTESTS_HPP
#define _GL4CSPARSETEXTURECLAMPTESTS_HPP
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
 * \file  gl4cSparseTextureClampTests.hpp
 * \brief Conformance tests for the GL_ARB_sparse_texture_clamp functionality.
 */ /*-------------------------------------------------------------------*/
#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

#include "gl4cSparseTexture2Tests.hpp"
#include "gl4cSparseTextureTests.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{

/** Test verifies if sparse texture clamp lookup functions generates access residency information
 **/
class SparseTextureClampLookupResidencyTestCase : public SparseTexture2LookupTestCase
{
public:
	/* Public methods */
	SparseTextureClampLookupResidencyTestCase(deqp::Context& context);

	SparseTextureClampLookupResidencyTestCase(deqp::Context& context, const char* name, const char* description);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected methods */
	virtual bool funcAllowed(GLint target, GLint format, FunctionToken& funcToken);

	virtual bool verifyLookupTextureData(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level,
										 FunctionToken& funcToken);

	virtual void draw(GLint target, GLint layer, const ShaderProgram& program);
};

/** Test verifies if sparse and non-sparse texture clamp lookup functions works as expected
 **/
class SparseTextureClampLookupColorTestCase : public SparseTextureClampLookupResidencyTestCase
{
public:
	/* Public methods */
	SparseTextureClampLookupColorTestCase(deqp::Context& context);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	virtual bool writeDataToTexture(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);
	virtual bool verifyLookupTextureData(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level,
										 FunctionToken& funcToken);

	virtual bool prepareTexture(const Functions& gl, GLint target, GLint format, GLuint& texture);
	virtual bool commitTexturePage(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);

	virtual bool isInPageSizesRange(GLint target, GLint level);
	virtual bool isPageSizesMultiplication(GLint target, GLint level);

private:
	/* Private methods */
	std::string generateFunctionDef(std::string funcName);
	std::string generateExpectedResult(std::string returnType, GLint level, GLint format);
};

/** Test group which encapsulates all sparse texture conformance tests */
class SparseTextureClampTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	SparseTextureClampTests(deqp::Context& context);

	void init();

private:
	SparseTextureClampTests(const SparseTextureClampTests& other);
	SparseTextureClampTests& operator=(const SparseTextureClampTests& other);
};

} /* glcts namespace */

#endif // _GL4CSPARSETEXTURECLAMPTESTS_HPP
