#ifndef _GL4CSPARSETEXTURE2TESTS_HPP
#define _GL4CSPARSETEXTURE2TESTS_HPP
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
 * \file  gl4cSparseTexture2Tests.hpp
 * \brief Conformance tests for the GL_ARB_sparse_texture2 functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"
#include <map>
#include <set>
#include <vector>

#include "gl4cSparseTextureTests.hpp"
#include "gluDrawUtil.hpp"
#include "gluShaderProgram.hpp"
#include "gluTextureUtil.hpp"
#include "tcuTextureUtil.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{

struct PageSizeStruct
{
	GLint xSize;
	GLint ySize;
	GLint zSize;

	PageSizeStruct() : xSize(0), ySize(0), zSize(0)
	{
	}
	PageSizeStruct(GLint x, GLint y, GLint z) : xSize(x), ySize(y), zSize(z)
	{
	}
};

typedef std::pair<GLint, PageSizeStruct> PageSizePair;

/** Test verifies if extension is available for GLSL
 **/
class ShaderExtensionTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	ShaderExtensionTestCase(deqp::Context& context, const std::string extension);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private members */
	std::string mExtension;
};

/** Test verifies if values returned by GetInternalFormat* query matches Standard Virtual Page Sizes for <pname>:
 *   - VIRTUAL_PAGE_SIZE_X_ARB,
 *   - VIRTUAL_PAGE_SIZE_Y_ARB,
 *   - VIRTUAL_PAGE_SIZE_Z_ARB.
 **/
class StandardPageSizesTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	StandardPageSizesTestCase(deqp::Context& context);

	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	std::vector<GLint> mSupportedTargets;
	std::map<GLint, PageSizeStruct> mStandardVirtualPageSizesTable;

	/* Private members */
};

/** Test verifies glTexStorage* functionality added by ARB_sparse_texture2 extension
 **/
class SparseTexture2AllocationTestCase : public SparseTextureAllocationTestCase
{
public:
	/* Public methods */
	SparseTexture2AllocationTestCase(deqp::Context& context);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
};

/** Test verifies glTexPageCommitmentARB functionality added by ARB_sparse_texture2 extension
 **/
class SparseTexture2CommitmentTestCase : public SparseTextureCommitmentTestCase
{
public:
	/* Public methods */
	SparseTexture2CommitmentTestCase(deqp::Context& context);

	SparseTexture2CommitmentTestCase(deqp::Context& context, const char* name, const char* description);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected members */
	struct TokenStrings
	{
		std::string format;
		std::string pointType;
		std::string pointDef;
		std::string outputType;
		std::string inputType;
		std::string returnType;
		std::string resultExpected;
		std::string resultDefault;
		std::string epsilon;
		std::string sampleDef;
	};

	/* Protected methods */
	TokenStrings createShaderTokens(GLint target, GLint format, GLint sample, const std::string outputBase = "image",
									const std::string inputBase = "image");

	virtual bool caseAllowed(GLint target, GLint format);

	virtual bool sparseAllocateTexture(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint levels);
	virtual bool allocateTexture(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint levels);
	virtual bool writeDataToTexture(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);
	virtual bool verifyTextureData(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);
};

/** Test verifies if access to uncommitted regions of sparse texture works as expected
 **/
class UncommittedRegionsAccessTestCase : public SparseTexture2CommitmentTestCase
{
public:
	/* Public methods */
	UncommittedRegionsAccessTestCase(deqp::Context& context);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private members */
	GLuint mFramebuffer;
	GLuint mRenderbuffer;

	/* Private methods */
	bool readsAllowed(GLint target, GLint format, bool shaderOnly = false);
	bool atomicAllowed(GLint target, GLint format);
	bool depthStencilAllowed(GLint target, GLint format);

	bool UncommittedReads(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);
	bool UncommittedAtomicOperations(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);
	bool UncommittedDepthStencil(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);

	void prepareDepthStencilFramebuffer(const Functions& gl, GLint width, GLint height);
	void cleanupDepthStencilFramebuffer(const Functions& gl);
	bool verifyTextureDataExtended(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level,
								   bool shaderOnly = false);
	bool verifyAtomicOperations(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);
	bool verifyStencilTest(const Functions& gl, ShaderProgram& program, GLint width, GLint height,
						   GLint widthCommitted);
	bool verifyDepthTest(const Functions& gl, ShaderProgram& program, GLint width, GLint height, GLint widthCommitted);
	bool verifyDepthBoundsTest(const Functions& gl, ShaderProgram& program, GLint width, GLint height,
							   GLint widthCommitted);
};

/** Test verifies if sparse texture lookup functions for GLSL works as expected
 **/
class SparseTexture2LookupTestCase : public SparseTexture2CommitmentTestCase
{
public:
	/* Public methods */
	SparseTexture2LookupTestCase(deqp::Context& context);

	SparseTexture2LookupTestCase(deqp::Context& context, const char* name, const char* description);

	void						 init();
	tcu::TestNode::IterateResult iterate();

protected:
	/* Protected types */
	struct FunctionToken
	{
		std::string name;
		std::string arguments;

		std::set<GLint> allowedTargets;

		FunctionToken()
		{
		}

		FunctionToken(std::string fname, std::string fargs) : name(fname), arguments(fargs)
		{
		}
	};

	struct TokenStringsExt : public TokenStrings
	{
		std::string formatDef;
		std::string sizeDef;
		std::string lod;
		std::string lodDef;
		std::string coordType;
		std::string iCoordType;
		std::string coordDef;
		std::string cubeMapCoordDef;
		std::string refZDef;
		std::string cubeMapArrayRefZDef;
		std::string offsetDim;
		std::string offsetType;
		std::string nOffsetType;
		std::string componentDef;
		std::string offsetArrayDef;
		std::string pointCoord;
	};

	/* Protected members */
	std::vector<FunctionToken> mFunctions;

	/* Protected methods */
	TokenStringsExt createLookupShaderTokens(GLint target, GLint format, GLint level, GLint sample,
											 FunctionToken& funcToken);

	virtual bool caseAllowed(GLint target, GLint format);
	virtual bool funcAllowed(GLint target, GLint format, FunctionToken& funcToken);

	virtual bool writeDataToTexture(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);

	virtual void setupDepthMode(const Functions& gl, GLint target, GLuint& texture);
	virtual bool verifyLookupTextureData(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level,
										 FunctionToken& funcToken);
};

/** Test group which encapsulates all sparse texture conformance tests */
class SparseTexture2Tests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	SparseTexture2Tests(deqp::Context& context);

	void init();

private:
	SparseTexture2Tests(const SparseTexture2Tests& other);
	SparseTexture2Tests& operator=(const SparseTexture2Tests& other);
};

void replaceToken(const GLchar* token, const GLchar* text, std::string& string);

} /* glcts namespace */

#endif // _GL4CSPARSETEXTURE2TESTS_HPP
