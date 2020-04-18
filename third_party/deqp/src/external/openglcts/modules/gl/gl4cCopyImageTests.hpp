#ifndef _GL4CCOPYIMAGETESTS_HPP
#define _GL4CCOPYIMAGETESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file gl4cCopyImageTests.hpp
 * \brief CopyImageSubData functional tests.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

namespace gl4cts
{
namespace CopyImage
{
/** Implements functional test. Description follows:
 *
 * This test verifies that CopyImageSubData function works as expected.
 *
 * Steps:
 *     - create source and destination image objects;
 *     - fill both image objects with different content, also each pixel should
 *     be unique;
 *     - execute CopyImageSubData on both image objects, to copy region from
 *     source image to a region in destination image;
 *     - inspect content of both image objects;
 *
 * Test pass if:
 *     - no part of destination image, that is outside of destination region,
 *     was modified,
 *     - destination region contains data from source region,
 *     - source image contents were not modified.
 *
 * Use TexImage* routines to create textures. It is required that textures are
 * complete, therefore TEXTURE_BASE_LEVEL and TEXTURE_MAX_LEVEL have to be
 * updated with TexParameter manually.
 *
 * Use draw operation to update contents of Renderbuffers.
 *
 * Tested image dimensions should be NxM where N and M are <7:15>.
 *
 * Multi-layered targets should have twelve slices.
 *
 * Test with the following mipmap levels:
 *     - 0,
 *     - 1,
 *     - 2.
 *
 * Test with the following region locations:
 *     - all four corners,
 *     - all four edge centres,
 *     - image centre,
 *     - whole image.
 *
 * Tested region dimensions should be NxN where N is <1:7>. Depth should be
 * selected so as to affect all image slices. In cases when one image has
 * single layer and second is multi-layered, than copy operation and inspection
 * must be repeated for each layer.
 *
 * This test should iterate over:
 *     - all valid internal format combinations,
 *     - all valid object target combinations,
 *     - all mipmap level combinations,
 *     - all image dimensions combinations,
 *     - all valid copied regions combinations,
 *     - all valid copied region dimensions combinations.
 *
 * Moreover this test should also execute CopyImageSubData to copy image to
 * itself. In this case it expected that source image will be modified.
 *
 *
 * Implementation notes
 * There are some configuration preprocessor flags available in cpp file.
 **/
class FunctionalTest : public deqp::TestCase
{
public:
	FunctionalTest(deqp::Context& context);
	virtual ~FunctionalTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct targetDesc
	{
		glw::GLenum m_target;
		glw::GLuint m_width;
		glw::GLuint m_height;
		glw::GLuint m_level;
		glw::GLenum m_internal_format;
		glw::GLenum m_format;
		glw::GLenum m_type;
	};

	struct testCase
	{
		targetDesc  m_dst;
		glw::GLuint m_dst_x;
		glw::GLuint m_dst_y;
		targetDesc  m_src;
		glw::GLuint m_src_x;
		glw::GLuint m_src_y;
		glw::GLuint m_width;
		glw::GLuint m_height;
	};

	/* Private methods */
	void calculateDimmensions(glw::GLenum target, glw::GLuint level, glw::GLuint width, glw::GLuint height,
							  glw::GLuint* out_widths, glw::GLuint* out_heights, glw::GLuint* out_depths) const;

	void clean();
	void cleanPixels(glw::GLubyte** pixels) const;

	bool compareImages(const targetDesc& left_desc, const glw::GLubyte* left_data, glw::GLuint left_x,
					   glw::GLuint left_y, glw::GLuint left_layer, glw::GLuint left_level, const targetDesc& right_desc,
					   const glw::GLubyte* right_data, glw::GLuint right_x, glw::GLuint right_y,
					   glw::GLuint right_layer, glw::GLuint right_level, glw::GLuint region_width,
					   glw::GLuint region_height) const;

	bool copyAndVerify(const testCase& test_case, const glw::GLubyte** dst_pixels, const glw::GLubyte** src_pixels);

	void getCleanRegions(const testCase& test_case, glw::GLuint dst_level, glw::GLuint out_corners[4][4],
						 glw::GLuint& out_n_corners) const;

	void getPixels(glw::GLuint name, const targetDesc& desc, glw::GLuint level, glw::GLubyte* out_pixels) const;

	void prepareDstPxls(const targetDesc& desc, glw::GLubyte** out_pixels) const;

	void prepareSrcPxls(const targetDesc& desc, glw::GLenum dst_internal_format, glw::GLubyte** out_pixels) const;

	void prepareTestCases(glw::GLenum dst_internal_format, glw::GLenum dst_target, glw::GLenum src_internal_format,
						  glw::GLenum src_target);

	glw::GLuint prepareTexture(const targetDesc& desc, const glw::GLubyte** pixels, glw::GLuint& out_buf_id);

	bool verify(const testCase& test_case, glw::GLuint dst_layer, const glw::GLubyte** dst_pixels,
				glw::GLuint src_layer, const glw::GLubyte** src_pixels, glw::GLuint depth);

	/* Private fields */
	glw::GLuint			  m_dst_buf_name;
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_rb_name;
	glw::GLuint			  m_src_buf_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements functional test. Description follows:
 *
 * Smoke test:
 * Tests if function CopyImageSubData accepts and works correctly with:
 *     - all internal formats,
 *     - all valid targets.
 * Point of the test is to check each internal format and target separately.
 *
 * Steps:
 *     - create source and destination image objects using same format and
 *     target for both;
 *     - fill 0 level mipmap of both image objects with different content, also
 *     each pixel should be unique;
 *     - make both image objects complete;
 *     - execute CopyImageSubData on both image objects, to copy contents from
 *     source image to a destination image;
 *     - inspect content of both image objects;
 *
 * Test pass if:
 *     - destination image contains data from source image,
 *     - source image contents were not modified.
 *
 * Repeat steps for all internal formats, using TEXTURE_2D target.
 *
 * Repeat steps for all valid targets, using RGBA32UI internal format.
 **/
class SmokeTest : public deqp::TestCase
{
public:
	SmokeTest(deqp::Context& context);
	virtual ~SmokeTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_target;
		glw::GLenum m_internal_format;
		glw::GLenum m_format;
		glw::GLenum m_type;
	};

	/* Private methods */
	void clean();
	void cleanPixels(glw::GLubyte*& pixels) const;

	bool compareImages(const testCase& test_case, const glw::GLubyte* left_data, const glw::GLubyte* right_data) const;

	bool copyAndVerify(const testCase& test_case, const glw::GLubyte* src_pixels);

	void getPixels(glw::GLuint name, const testCase& test_case, glw::GLubyte* out_pixels) const;

	void prepareDstPxls(const testCase& test_case, glw::GLubyte*& out_pixels) const;

	void prepareSrcPxls(const testCase& test_case, glw::GLubyte*& out_pixels) const;

	glw::GLuint prepareTexture(const testCase& test_case, const glw::GLubyte* pixels, glw::GLuint& out_buf_id);

	bool verify(const testCase& test_case, const glw::GLubyte* src_pixels);

	/* Private fields */
	glw::GLuint			  m_dst_buf_name;
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_rb_name;
	glw::GLuint			  m_src_buf_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;

	/* Constants */
	static const glw::GLuint m_width;
	static const glw::GLuint m_height;
	static const glw::GLuint m_depth;
};

/** Implements negative test A. Description follows:
 *
 * [A]
 * * Verify that GL_INVALID_ENUM error is generated if either <srcTarget> or
 *   <dstTarget> is set to any of the proxy texture targets, GL_TEXTURE_BUFFER
 *   or one of the cube-map face selectors.
 **/
class InvalidTargetTest : public deqp::TestCase
{
public:
	InvalidTargetTest(deqp::Context& context);
	virtual ~InvalidTargetTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_src_target;
		glw::GLenum m_dst_target;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_buf_name;
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_buf_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements negative test B. Description follows:
 *
 * [B]
 * * Verify that usage of a non-matching target for either the source or
 *   destination objects results in a GL_INVALID_ENUM error.
 **/
class TargetMismatchTest : public deqp::TestCase
{
public:
	TargetMismatchTest(deqp::Context& context);
	virtual ~TargetMismatchTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_tex_target;
		glw::GLenum m_src_target;
		glw::GLenum m_dst_target;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_buf_name;
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_buf_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements negative test C. Description follows:
 *
 * [C]
 * * Verify that INVALID_OPERATION is generated when the texture provided
 *   to CopyImageSubData is incomplete
 **/
class IncompleteTexTest : public deqp::TestCase
{
public:
	IncompleteTexTest(deqp::Context& context);
	virtual ~IncompleteTexTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_tex_target;
		bool		m_is_dst_complete;
		bool		m_is_src_complete;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_buf_name;
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_buf_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements negative test D. Description follows:
 *
 * [D]
 * * Verify that usage of source/destination objects, internal formats of which
 *   are not compatible, results in GL_INVALID_OPERATION error.
 **/
class IncompatibleFormatsTest : public deqp::TestCase
{
public:
	IncompatibleFormatsTest(deqp::Context& context);
	virtual ~IncompatibleFormatsTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_tex_target;
		glw::GLenum m_dst_internal_format;
		glw::GLenum m_dst_format;
		glw::GLenum m_dst_type;
		glw::GLenum m_src_internal_format;
		glw::GLenum m_src_format;
		glw::GLenum m_src_type;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_buf_name;
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_buf_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements negative test E. Description follows:
 *
 * [E]
 * * Verify that usage of source/destination objects, internal formats of which
 *   do not match in terms of number of samples they can hold, results in
 *   GL_INVALID_OPERATION error.
 **/
class SamplesMismatchTest : public deqp::TestCase
{
public:
	SamplesMismatchTest(deqp::Context& context);
	virtual ~SamplesMismatchTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum  m_src_target;
		glw::GLsizei m_src_n_samples;
		glw::GLenum  m_dst_target;
		glw::GLsizei m_dst_n_samples;
		glw::GLenum  m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements negative test F. Description follows:
 *
 * [F]
 * * Verify that usage of a pair of objects, one of which is defined by
 *   compressed data and the other one has a non-compressed representation,
 *   results in a GL_INVALID_OPERATION error, if the block size of compressed
 *   image is not equal to the texel size of the compressed image.
 **/
class IncompatibleFormatsCompressionTest : public deqp::TestCase
{
public:
	IncompatibleFormatsCompressionTest(deqp::Context& context);
	virtual ~IncompatibleFormatsCompressionTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_tex_target;
		glw::GLenum m_dst_internal_format;
		glw::GLenum m_dst_format;
		glw::GLenum m_dst_type;
		glw::GLenum m_src_internal_format;
		glw::GLenum m_src_format;
		glw::GLenum m_src_type;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements negative test G. Description follows:
 *
 * [G]
 * * Verify that usage of an invalid <srcTarget> or <dstTarget> argument
 *   generates GL_INVALID_VALUE error. For the purpose of the test, make sure
 *   to iterate over the set of all objects that can be used as source or
 *   destination objects.
 **/
class InvalidObjectTest : public deqp::TestCase
{
public:
	InvalidObjectTest(deqp::Context& context);
	virtual ~InvalidObjectTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_dst_target;
		bool		m_dst_valid;
		glw::GLenum m_src_target;
		bool		m_src_valid;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_name;
	glw::GLuint			  m_src_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements negative test H. Description follows:
 *
 * [H]
 * * Make sure that GL_INVALID_VALUE error is generated if <level> argument
 *   refers to a non-existent mip-map level for either the source or the
 *   destination object. In particular, make sure that using any value other
 *   than 0 for render-buffers is considered an erroneous action.
 **/
class NonExistentMipMapTest : public deqp::TestCase
{
public:
	NonExistentMipMapTest(deqp::Context& context);
	virtual ~NonExistentMipMapTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_tex_target;
		glw::GLuint m_src_level;
		glw::GLuint m_dst_level;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements negative test I. Description follows:
 *
 * [I]
 * * Make sure that using source/destination subregions that exceed the
 *   boundaries of the relevant object generates a GL_INVALID_VALUE error.
 **/
class ExceedingBoundariesTest : public deqp::TestCase
{
public:
	ExceedingBoundariesTest(deqp::Context& context);
	virtual ~ExceedingBoundariesTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLenum m_tex_target;
		glw::GLuint m_depth;
		glw::GLuint m_height;
		glw::GLuint m_src_x;
		glw::GLuint m_src_y;
		glw::GLuint m_src_z;
		glw::GLuint m_dst_x;
		glw::GLuint m_dst_y;
		glw::GLuint m_dst_z;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;

	/* Private constants */
	static const glw::GLuint m_region_depth;
	static const glw::GLuint m_region_height;
	static const glw::GLuint m_region_width;
};

/** Implements negative test J. Description follows:
 *
 * [J]
 * * Assume the source and/or the destination object(s) use(s) a compressed
 *   internal format. Make sure that copy operations requested to operate on
 *   subregions that do not meet the alignment constraints of the internal
 *   format in question, generate GL_INVALID_VALUE error.
 **/
class InvalidAlignmentTest : public deqp::TestCase
{
public:
	InvalidAlignmentTest(deqp::Context& context);
	virtual ~InvalidAlignmentTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLuint m_height;
		glw::GLuint m_width;
		glw::GLuint m_src_x;
		glw::GLuint m_src_y;
		glw::GLuint m_dst_x;
		glw::GLuint m_dst_y;
		glw::GLenum m_expected_result;
	};

	/* Private methods */
	void clean();

	/* Private fields */
	glw::GLuint			  m_dst_tex_name;
	glw::GLuint			  m_src_tex_name;
	glw::GLuint			  m_test_case_index;
	std::vector<testCase> m_test_cases;
};

/** Implements functional test. Description follows:
 *
 * [B]
 * 1. Create a single level integer texture, with BASE_LEVEL and MAX_LEVEL set to 0.
 * 2. Leave the mipmap filters at the default of GL_NEAREST_MIPMAP_LINEAR and GL_LINEAR.
 * 3. Do glCopyImageSubData to or from that texture.
 * 4. Make sure it succeeds and does not raise GL_INVALID_OPERATION.
 **/
class IntegerTexTest : public deqp::TestCase
{
public:
	IntegerTexTest(deqp::Context& context);
	virtual ~IntegerTexTest()
	{
	}

	/* Implementation of tcu::TestNode methods */
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct testCase
	{
		glw::GLint  m_internal_format;
		glw::GLuint m_type;
	};

	/* Private methods */
	unsigned int createTexture(int width, int height, glw::GLint internalFormat, glw::GLuint type, const void* data,
							   int minFilter, int magFilter);
	void clean();

	/* Private fields */
	glw::GLuint m_dst_buf_name;
	glw::GLuint m_dst_tex_name;
	glw::GLuint m_src_buf_name;
	glw::GLuint m_src_tex_name;
	glw::GLuint m_test_case_index;
};
}

class CopyImageTests : public deqp::TestCaseGroup
{
public:
	CopyImageTests(deqp::Context& context);
	~CopyImageTests(void);

	virtual void init(void);

private:
	CopyImageTests(const CopyImageTests& other);
	CopyImageTests& operator=(const CopyImageTests& other);
};
} /* namespace gl4cts */

#endif // _GL4CCOPYIMAGETESTS_HPP
