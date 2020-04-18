#ifndef _GL4CSPARSETEXTURETESTS_HPP
#define _GL4CSPARSETEXTURETESTS_HPP
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
 * \file  gl4cSparseTextureTests.hpp
 * \brief Conformance tests for the GL_ARB_sparse_texture functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"
#include <sstream>
#include <vector>

#include "gluTextureUtil.hpp"
#include "tcuTextureUtil.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{

struct TextureState
{
	GLint			   width;
	GLint			   height;
	GLint			   depth;
	GLint			   levels;
	GLint			   samples;
	tcu::TextureFormat format;

	GLint minDepth;

	GLint pageSizeX;
	GLint pageSizeY;
	GLint pageSizeZ;
};

class SparseTextureUtils
{
public:
	static bool verifyQueryError(std::stringstream& log, const char* funcName, GLint target, GLint pname, GLint error,
								 GLint expectedError);

	static bool verifyError(std::stringstream& log, const char* funcName, GLint error, GLint expectedError);

	static GLint getTargetDepth(GLint target);

	static void getTexturePageSizes(const Functions& gl, GLint target, GLint format, GLint& pageSizeX, GLint& pageSizeY,
									GLint& pageSizeZ);

	static void getTextureLevelSize(GLint target, TextureState& state, GLint level, GLint& width, GLint& height,
									GLint& depth);
};

/** Represents texture static helper
 **/
class Texture
{
public:
	/* Public static routines */
	/* Functionality */
	static void Bind(const Functions& gl, GLuint id, GLenum target);

	static void Generate(const Functions& gl, GLuint& out_id);

	static void Delete(const Functions& gl, GLuint& id);

	static void Storage(const Functions& gl, GLenum target, GLsizei levels, GLenum internal_format, GLuint width,
						GLuint height, GLuint depth);

	static void GetData(const Functions& gl, GLint level, GLenum target, GLenum format, GLenum type, GLvoid* out_data);

	static void SubImage(const Functions& gl, GLenum target, GLint level, GLint x, GLint y, GLint z, GLsizei width,
						 GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels);

	/* Public fields */
	GLuint m_id;

	/* Public constants */
	static const GLuint m_invalid_id;
};

/** Test verifies TexParameter{if}{v}, TexParameterI{u}v, GetTexParameter{if}v
 * and GetTexParameterIi{u}v queries for <pname>:
 *   - TEXTURE_SPARSE_ARB,
 *   - VIRTUAL_PAGE_SIZE_INDEX_ARB.
 * Test verifies also GetTexParameter{if}v and GetTexParameterIi{u}v queries for <pname>:
 *   - NUM_SPARSE_LEVELS_ARB
 **/
class TextureParameterQueriesTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	TextureParameterQueriesTestCase(deqp::Context& context);

	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private members */
	std::stringstream mLog;

	std::vector<GLint> mSupportedTargets;
	std::vector<GLint> mNotSupportedTargets;

	/* Private methods */
	bool testTextureSparseARB(const Functions& gl, GLint target, GLint expectedError = GL_NO_ERROR);
	bool testVirtualPageSizeIndexARB(const Functions& gl, GLint target, GLint expectedError = GL_NO_ERROR);
	bool testNumSparseLevelsARB(const Functions& gl, GLint target);

	bool checkGetTexParameter(const Functions& gl, GLint target, GLint pname, GLint expected);
};

/** Test verifies GetInternalformativ query for formats from Table 8.12 and <pname>:
 *   - NUM_VIRTUAL_PAGE_SIZES_ARB,
 *   - VIRTUAL_PAGE_SIZE_X_ARB,
 *   - VIRTUAL_PAGE_SIZE_Y_ARB,
 *   - VIRTUAL_PAGE_SIZE_Z_ARB.
 **/
class InternalFormatQueriesTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	InternalFormatQueriesTestCase(deqp::Context& context);

	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	std::stringstream mLog;

	std::vector<GLint> mSupportedTargets;
	std::vector<GLint> mSupportedInternalFormats;

	/* Private members */
};

/** Test verifies GetIntegerv, GetFloatv, GetDoublev, GetInteger64v,
 * and GetBooleanv queries for <pname>:
 *   - MAX_SPARSE_TEXTURE_SIZE_ARB,
 *   - MAX_SPARSE_3D_TEXTURE_SIZE_ARB,
 *   - MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB,
 *   - SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB.
 **/
class SimpleQueriesTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	SimpleQueriesTestCase(deqp::Context& context);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	/* Private members */
	void testSipmleQueries(const Functions& gl, GLint pname);
};

/** Test verifies glTexStorage* functionality added by ARB_sparse_texture extension
 **/
class SparseTextureAllocationTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	SparseTextureAllocationTestCase(deqp::Context& context);

	SparseTextureAllocationTestCase(deqp::Context& context, const char* name, const char* description);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected methods */
	std::stringstream mLog;

	std::vector<GLint> mSupportedTargets;
	std::vector<GLint> mFullArrayTargets;
	std::vector<GLint> mSupportedInternalFormats;

	/* Protected members */
	bool positiveTesting(const Functions& gl, GLint target, GLint format);
	bool verifyTexParameterErrors(const Functions& gl, GLint target, GLint format);
	bool verifyTexStorageVirtualPageSizeIndexError(const Functions& gl, GLint target, GLint format);
	bool verifyTexStorageFullArrayCubeMipmapsError(const Functions& gl, GLint target, GLint format);
	bool verifyTexStorageInvalidValueErrors(const Functions& gl, GLint target, GLint format);
};

/** Test verifies glTexPageCommitmentARB functionality added by ARB_sparse_texture extension
 **/
class SparseTextureCommitmentTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	SparseTextureCommitmentTestCase(deqp::Context& context);

	SparseTextureCommitmentTestCase(deqp::Context& context, const char* name, const char* description);

	virtual void						 init();
	virtual tcu::TestNode::IterateResult iterate();

protected:
	/* Protected members */
	std::stringstream mLog;

	std::vector<GLint> mSupportedTargets;
	std::vector<GLint> mSupportedInternalFormats;

	TextureState mState;

	/* Protected methods */
	virtual void texPageCommitment(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level,
								   GLint xOffset, GLint yOffset, GLint zOffset, GLint width, GLint height, GLint depth,
								   GLboolean committ);

	virtual bool caseAllowed(GLint target, GLint format);

	virtual bool prepareTexture(const Functions& gl, GLint target, GLint format, GLuint& texture);
	virtual bool sparseAllocateTexture(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint levels);
	virtual bool allocateTexture(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint levels);
	virtual bool writeDataToTexture(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);
	virtual bool verifyTextureData(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);
	virtual bool commitTexturePage(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level);

	virtual bool isInPageSizesRange(GLint target, GLint level);
	virtual bool isPageSizesMultiplication(GLint target, GLint level);

	virtual bool verifyInvalidOperationErrors(const Functions& gl, GLint target, GLint format, GLuint& texture);
	virtual bool verifyInvalidValueErrors(const Functions& gl, GLint target, GLint format, GLuint& texture);
};

/** Test verifies glTexturePageCommitmentEXT functionality added by ARB_sparse_texture extension
 **/
class SparseDSATextureCommitmentTestCase : public SparseTextureCommitmentTestCase
{
public:
	/* Public methods */
	SparseDSATextureCommitmentTestCase(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	virtual void texPageCommitment(const Functions& gl, GLint target, GLint format, GLuint& texture, GLint level,
								   GLint xOffset, GLint yOffset, GLint zOffset, GLint width, GLint height, GLint depth,
								   GLboolean committ);
};

/** Test group which encapsulates all sparse texture conformance tests */
class SparseTextureTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	SparseTextureTests(deqp::Context& context);

	void init();

private:
	SparseTextureTests(const SparseTextureTests& other);
	SparseTextureTests& operator=(const SparseTextureTests& other);
};

} /* glcts namespace */

#endif // _GL4CSPARSETEXTURETESTS_HPP
