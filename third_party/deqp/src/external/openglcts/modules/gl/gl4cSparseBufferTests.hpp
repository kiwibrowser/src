#ifndef _GL4CSPARSEBUFFERTESTS_HPP
#define _GL4CSPARSEBUFFERTESTS_HPP
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
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  gl4cSparseBufferTests.hpp
 * \brief Conformance tests for the GL_ARB_sparse_buffer functionality.
 */ /*-------------------------------------------------------------------*/
#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"
#include <vector>

namespace gl4cts
{
/** Utility functions, used across many sparse buffer conformance test classes. */
class SparseBufferTestUtilities
{
public:
	/* Public methods */
	static unsigned int alignOffset(const unsigned int& offset, const unsigned int& value);

	static glw::GLuint createComputeProgram(const glw::Functions& gl, const char** cs_body_parts,
											unsigned int n_cs_body_parts);

	static glw::GLuint createProgram(const glw::Functions& gl, const char** fs_body_parts, unsigned int n_fs_body_parts,
									 const char** vs_body_parts, unsigned int n_vs_body_parts,
									 const char** attribute_names, const unsigned int* attribute_locations,
									 unsigned int			   n_attribute_properties,
									 const glw::GLchar* const* tf_varyings = DE_NULL, unsigned int n_tf_varyings = 0,
									 glw::GLenum tf_varying_mode = GL_NONE);

	static std::string getSparseBOFlagsString(glw::GLenum flags);
};

/** * Verify glBufferPageCommitmentARB() returns GL_INVALID_ENUM if <target> is
 *    set to GL_INTERLEAVED_ATTRIBS.
 *
 *  * Verify glBufferStorage() throws a GL_INVALID_VALUE error if <flags> is
 *    set to (GL_SPARSE_STORAGE_BIT_ARB | GL_MAP_READ_BIT) or
 *    (GL_SPARSE_STORAGE_BIT_ARB | GL_MAP_WRITE_BIT).
 *
 *  * Verify glBufferPageCommitmentARB() generates a GL_INVALID_OPERATION error if
 *    it is called for an immutable BO, which has not been initialized with the
 *    GL_SPARSE_STORAGE_BIT_ARB flag.
 *
 *  * Verify glBufferPageCommitmentARB() issues a GL_INVALID_VALUE error if <offset>
 *    is set to (0.5 * GL_SPARSE_BUFFER_PAGE_SIZE_ARB). Skip if the constant's value
 *    is equal to 1.
 *
 *  * Verify glBufferPageCommitmentARB() emits a GL_INVALID_VALUE error if <size>
 *    is set to (0.5 * GL_SPARSE_BUFFER_PAGE_SIZE_ARB). Skip if the constant's value
 *    is equal to 1.
 *
 *  * Verify glBufferPageCommitmentARB() returns GL_INVALID_VALUE if <offset> is
 *    set to -1, but all other arguments are valid.
 *
 *  * Verify glBufferPageCommitmentARB() returns GL_INVALID_VALUE if <size> is
 *    set to -1, but all other arguments are valid.
 *
 *  * Verify glBufferPageCommitmentARB() returns GL_INVALID_VALUE if BO's size is
 *    GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 3, but the <offset> is set to 0 and <size>
 *    argument used for the call is set to GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 4.
 *
 *  * Verify glBufferPageCommitmentARB() returns GL_INVALID_VALUE if BO's size is
 *    GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 3, but the <offset> is set to
 *    GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 1 and <size> argument used for the call
 *    is set to GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 3.
 *
 *  * Verify that calling glMapBuffer() or glMapBufferRange() against a sparse
 *    buffer generates a GL_INVALID_OPERATION error.
 **/
class NegativeTests : public deqp::TestCase
{
public:
	/* Public methods */
	NegativeTests(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */

	/* Private members */
	glw::GLuint		   m_helper_bo_id;	/* never allocated actual storage; bound to GL_ELEMENT_ARRAY_BUFFER */
	glw::GLuint		   m_immutable_bo_id; /* bound to GL_COPY_READ_BUFFER */
	const unsigned int m_immutable_bo_size;

	glw::GLuint m_sparse_bo_id; /* bound to GL_ARRAY_BUFFER */
};

/** 1. Make sure glGetBooleanv(), glGetDoublev(), glGetFloatv(), glGetIntegerv()
 *     and glGetInteger64v() recognize the new GL_SPARSE_BUFFER_PAGE_SIZE_ARB
 *     pname and return a value equal to or larger than 1, but no bigger than 65536
 */
class PageSizeGetterTest : public deqp::TestCase
{
public:
	/* Public methods */
	PageSizeGetterTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();
};

/** Interface class for test case implementation for the functional test 2. */
class BufferStorageTestCase
{
public:
	virtual ~BufferStorageTestCase()
	{
	}

	/* Public methods */
	virtual void deinitTestCaseGlobal()						  = 0;
	virtual bool execute(glw::GLuint sparse_bo_storage_flags) = 0;
	virtual const char* getName()							  = 0;
	virtual bool		initTestCaseGlobal()				  = 0;
	virtual bool initTestCaseIteration(glw::GLuint sparse_bo) = 0;

	virtual void deinitTestCaseIteration()
	{
		/* Stub by default */
	}
};

/** Implements the test case e for the test 2:
 *
 * e.  Use the committed sparse buffer storage to store atomic counter values.
 *     The vertex shader used for the test case should define as many ACs as
 *     supported by the platform (GL_MAX_VERTEX_ATOMIC_COUNTERS). The condition,
 *     under which each of the ACs should be incremented, can be based on
 *     gl_VertexID's value (eg. increment AC0 if gl_VertexID % 2 == 0, increment
 *     AC1 if gl_VertexID % 3 == 0, and so on).
 *
 *     Use regular draw calls, issued consecutively for three times, for the
 *     test.
 *     Verify that both atomic counter buffer binding commands (glBindBufferBase()
 *     and glBindBufferRange() ) work correctly.
 *
 *     The test passes if the result values are correct.
 *
 *     The test should run in two iterations:
 *     a) All required pages are committed.
 *     b) Only half of the pages are committed. If only a single page is needed,
 *        de-commit that page before issuing the draw call.
 */
class AtomicCounterBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	AtomicCounterBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size,
									   bool all_pages_committed);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();

	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case e";
	}

private:
	/* Private fields */
	bool				  m_all_pages_committed;
	const glw::Functions& m_gl;
	glw::GLint			  m_gl_atomic_counter_uniform_array_stride;
	glw::GLint			  m_gl_max_vertex_atomic_counters_value;
	glw::GLuint			  m_helper_bo;
	unsigned int		  m_helper_bo_size;
	unsigned int		  m_helper_bo_size_rounded;
	const unsigned int	m_n_draw_calls;
	glw::GLint			  m_page_size;
	glw::GLuint			  m_po;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_data_size;
	unsigned int		  m_sparse_bo_data_size_rounded; /* aligned to page size */
	unsigned int		  m_sparse_bo_data_start_offset;
	unsigned int m_sparse_bo_data_start_offset_rounded; /* <= m_sparse_bo_data_start_offset, aligned to page size */
	tcu::TestContext& m_testCtx;
	glw::GLuint		  m_vao;
};

/** Implements the test case f for the test 2:
 *
 * f.  Use the committed sparse buffer storage as a backing for a buffer texture
 *     object. A compute shader should inspect the contents of the texture and,
 *     for invocation-specific texels, write out 1 to a SSBO if the fetched texel
 *     was correct. Otherwise, it should write out 0.
 *
 *     The shader storage block needs not be backed by a sparse buffer.
 *
 *     As with previous cases, make sure both of the following scenarios are
 *     tested:
 *
 *     a) All required pages are committed.
 *     b) Only half of the pages are committed. If only a single page is needed,
 *        de-commit that page before issuing the dispatch call.
 *
 *     Both glTexBuffer() and glTexBufferRange() should be tested.
 *
 */
class BufferTextureStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	BufferTextureStorageTestCase(const glw::Functions& gl, deqp::Context& context, tcu::TestContext& testContext,
								 glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case f";
	}

private:
	/* Private fields */
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo;
	unsigned char*		  m_helper_bo_data;
	unsigned int		  m_helper_bo_data_size;
	bool				  m_is_texture_buffer_range_supported;
	glw::GLint			  m_page_size;
	glw::GLuint			  m_po;
	const unsigned int	m_po_local_wg_size;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
	glw::GLuint			  m_ssbo;
	unsigned char*		  m_ssbo_zero_data;
	unsigned int		  m_ssbo_zero_data_size;
	tcu::TestContext&	 m_testCtx;
	glw::GLuint			  m_to;
	const unsigned int	m_to_width;
};

/** Implements the test case c for the test 2:
 *
 * c.  Issue glClearBufferData() and glClearBufferSubData() calls
 *     over a sparse buffer. Make sure that all committed pages, which should
 *     have been affected by the calls, have been reset to the requested
 *     values.
 *     Try issuing glClearNamedBufferSubData() over a region, for which one
 *     of the halves is committed, and the other is not. Make sure the former
 *     has been touched, and that no crash has occurred.
 *
 */
class ClearOpsBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	ClearOpsBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case c";
	}

private:
	/* Private fields */
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo;	/* holds m_sparse_bo_size_rounded bytes */
	unsigned char*		  m_initial_data; /* holds m_sparse_bo_size_rounded bytes */
	unsigned int		  m_n_pages_to_use;
	glw::GLint			  m_page_size;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_size_rounded;
	tcu::TestContext&	 m_testCtx;
};

/** Implements the test case g for the test 2:
 *
 * g.  Verify copy operations work correctly for cases where:
 *
 *     I)   Destination and source are different sparse BOs.
 *     II)  Destination is a sparse buffer object, source is an immutable BO.
 *     III) Destination is an immutable BO, source is a sparse BO.
 *     IV)  Destination and source are the same sparse BO, but refer to
 *          different, non-overlapping memory regions.
 *
 *     and
 *
 *     *)   All pages of the source region are not committed
 *     **)  Half of the pages of the source region is not committed
 *     ***) None of the pages of the source region are committed.
 *
 *     and
 *
 *     +)   All pages of the destination region are not committed
 *     ++)  Half of the pages of the destination region is not committed
 *     +++) None of the pages of the destination region are committed.
 *
 *     Test all combinations of I-IV, *-***, and +-+++ bearing in mind that:
 *
 *     a) reads executed on non-committed memory regions return meaningless
 *        values but MUST NOT crash GL
 *     b) writes performed on non-committed memory regions are silently
 *        ignored.
 */
class CopyOpsBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	CopyOpsBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case g";
	}

private:
	/* Private type definitions */
	typedef struct _test_case
	{
		glw::GLint		dst_bo_commit_size;
		glw::GLint		dst_bo_commit_start_offset;
		glw::GLuint		dst_bo_sparse_id;
		bool			dst_bo_is_sparse;
		unsigned short* dst_bo_ref_data;
		glw::GLint		dst_bo_start_offset;

		glw::GLint n_bytes_to_copy;

		glw::GLint		src_bo_commit_size;
		glw::GLint		src_bo_commit_start_offset;
		glw::GLuint		src_bo_sparse_id;
		bool			src_bo_is_sparse;
		unsigned short* src_bo_ref_data;
		glw::GLint		src_bo_start_offset;
	} _test_case;

	typedef std::vector<_test_case>		_test_cases;
	typedef _test_cases::const_iterator _test_cases_const_iterator;
	typedef _test_cases::iterator		_test_cases_iterator;

	/* Private methods */
	void initReferenceData();
	void initTestCases();

	/* Private fields */
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo;
	glw::GLuint			  m_immutable_bo;
	glw::GLint			  m_page_size;
	unsigned short*		  m_ref_data[3];   /* [0] - immutable bo data, [1] - sparse bo[0] data, [2] - sparse bo[1] data.
	 *
	 * Each data buffer holds m_sparse_bo_size_rounded bytes.
	 */
	glw::GLuint			  m_sparse_bos[2]; /* [0] - provided by BufferStorageTest[0], [1] - managed by the test case */
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
	_test_cases			  m_test_cases;
	tcu::TestContext&	 m_testCtx;
};

/** Implements the test case h for the test 2:
 *
 *  h.  Verify indirect dispatch calls work correctly for the following cases:
 *
 *  a) The arguments are taken from a committed memory page.
 *  b) The arguments are taken from a de-committed memory page. We expect
 *     the dispatch request to be silently ignored in this case.
 *  c) Half of the arguments are taken from a committed memory page,
 *     and the other half come from a de-committed memory page. Anticipated
 *     result is as per b).
 *
 *  Each spawned compute shader invocation should increment an atomic
 *  counter.
 *
 */
class IndirectDispatchBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	IndirectDispatchBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext,
										  glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case h";
	}

private:
	/* Private fields */
	unsigned int		  m_dispatch_draw_call_args_start_offset;
	unsigned int		  m_expected_ac_value;
	const glw::Functions& m_gl;
	const unsigned int	m_global_wg_size_x;
	glw::GLuint			  m_helper_bo; /* stores AC value + indirect dispatch call args */
	const unsigned int	m_local_wg_size_x;
	glw::GLint			  m_page_size;
	glw::GLuint			  m_po;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
	tcu::TestContext&	 m_testCtx;
};

/** Implements the test case d for the test 2:
 *
 * d.  Issue glInvalidateBufferData() and glInvalidateBufferSubData() calls for
 *     sparse buffers. For the *SubData() case, make sure you test both of
 *     cases:
 *
 *     * the whole touched region has been committed
 *     * only half of the pages have physical backing.
 */
class InvalidateBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	InvalidateBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case d";
	}

private:
	/* Private fields */
	const glw::Functions& m_gl;
	unsigned int		  m_n_pages_to_use;
	const glw::GLint	  m_page_size;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
};

/** Implement the test case k from CTS_ARB_sparse_buffer:
 *
 *  k. Verify pixel pack functionality works correctly, when a sparse buffer
 *     is bound to the pixel pack buffer binding point. Render a black-to-white
 *     RGBA8 gradient and use glReadPixels() to read & verify the rendered
 *     data. The color attachment should be of 1024x1024 resolution.
 *
 *     Consider three scenarios:
 *
 *     a) All pages, to which the data is to be written to, have been committed.
 *     b) Use the same memory page commitment layout as proposed in b2. The
 *        committed pages should contain correct data. Contents the pages
 *        without the physical backing should not be verified.
 *     c) No pages have been committed. The draw & read call should not crash
 *        the driver, but the actual contents is of no relevance.
 *
 **/
class PixelPackBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	PixelPackBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case k";
	}

private:
	/* Private fields */
	glw::GLuint			  m_color_rb;
	const unsigned int	m_color_rb_height;
	const unsigned int	m_color_rb_width;
	glw::GLuint			  m_fbo;
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo;
	glw::GLint			  m_page_size;
	glw::GLuint			  m_po;
	unsigned char*		  m_ref_data_ptr;
	unsigned int		  m_ref_data_size;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
	tcu::TestContext&	 m_testCtx;
	glw::GLuint			  m_vao;
};

/** Implements the test case l for the test 2:
 *
 * l. Verify pixel unpack functionality works correctly, when a sparse buffer
 *     is bound to the pixel unpack buffer binding point. Use a black-to-white
 *     gradient texture data for a glTexSubImage2D() call applied against an
 *     immutable texture object's base mip-map. Read back the data with
 *     a glGetTexImage() call and verify the contents is valid.
 *
 *     Consider three scenarios:
 *
 *     a) All pages, from which the texture data were read from, have been
 *        committed at the glTexSubImage2D() call time.
 *     b) Use the same memory page commitment layout as proposed in b2. The
 *        test should only check contents of the committed memory pages.
 *     c) No pages have been committed at the glTexSubImage2D() call time.
 *        The upload & getter calls should not crash, but the returned
 *        contents are irrelevant in this case.
 */
class PixelUnpackBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	PixelUnpackBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case l";
	}

private:
	/* Private fields */
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo;
	glw::GLint			  m_page_size;
	unsigned char*		  m_read_data_ptr;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
	tcu::TestContext&	 m_testCtx;
	unsigned char*		  m_texture_data_ptr;
	unsigned int		  m_texture_data_size;
	glw::GLuint			  m_to;
	unsigned char*		  m_to_data_zero;
	const unsigned int	m_to_height;
	const unsigned int	m_to_width;
};

/** Implements test cases a1-a6 for the test 2:
 *
 * a1. Use the sparse buffer as a VBO.
 *
 *     The render-target should be drawn a total of 100 x 100 green quads
 *     (built of triangles). Fill the buffer with vertex data (use four
 *     components, even though we need the rectangles to be rendered in
 *     screen space, in order to assure that the data-set spans across
 *     multiple pages by exceeding the maximum permitted page size of 64KB).
 *
 *     The quads should be 5px in width & height, and be separated from each
 *     other by a delta of 5px.
 *
 *     Render the quads to a render-target of 1024x1024 resolution.
 *
 *     All the pages, to which the vertex data has been submitted, should
 *     be committed. The test case passes if the rendered data is correct.
 *
 * a2. Follow the same approach as described for a1. However, this time,
 *     after the vertex data is uploaded, the test should de-commit all the
 *     pages and attempt to do the draw call.
 *
 *     The test passes if the GL implementation does not crash. Do not
 *     validate the rendered data.
 *
 * a3. Follow the same approach as described for a1. However, this time,
 *     make sure to also provide an IBO and issue an indexed draw call
 *     (both ranged and non-ranged). All required VBO and IBO pages should
 *     be committed.
 *
 *     The pass condition described in a1 is not changed.
 *
 * a4. Follow the same approach as described for a2. However, this time,
 *     after the vertex and index data is uploaded, the test should de-commit
 *     pages storing both IBO and VBO data. Both draw calls should be issued
 *     then.
 *
 *     The pass condition described in a2 is not changed.
 *
 * a5. Follow the same approach as described for a1. Apply the following
 *     change:
 *
 *     - Each rectangle should now be assigned a color, exposed to the VS
 *       via a Vertex Attribute Array. The color data should come from committed
 *       sparse buffer pages.
 *
 * a6. Follow the same approach as described for a5. Apply the following
 *     change:
 *
 *     - De-commit color data, after it has been uploaded. Try to execute the
 *       draw call.
 *
 *     The test passes if the GL implementation does not crash. Do not
 *     validate the rendered data.
 */
class QuadsBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Type definitions */
	enum _ibo_usage
	{
		/* Use glDrawArrays() for the draw call */
		IBO_USAGE_NONE,
		/* Use glDrawElements() for the draw call */
		IBO_USAGE_INDEXED_DRAW_CALL,
		/* Use glDrawRangeElements() for the draw call */
		IBO_USAGE_INDEXED_RANGED_DRAW_CALL
	};

	/* Public methods */
	QuadsBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size,
							   _ibo_usage ibo_usage, bool use_color_data);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return (!m_use_color_data && m_ibo_usage == IBO_USAGE_NONE) ?
				   "cases a1-a2" :
				   (!m_use_color_data && m_ibo_usage != IBO_USAGE_NONE) ?
				   "cases a3-a4" :
				   (m_use_color_data && m_ibo_usage != IBO_USAGE_NONE) ? "casea a5-a6" : "?!";
	}

private:
	/* Private methods */
	void createTestData(unsigned char** out_data, unsigned int* out_vbo_data_offset, unsigned int* out_ibo_data_offset,
						unsigned int* out_color_data_offset) const;

	void initHelperBO();

	void initSparseBO(bool decommit_data_pages_after_upload, bool is_dynamic_storage);

	/* Private fields */
	glw::GLuint			  m_attribute_color_location;
	glw::GLuint			  m_attribute_position_location;
	glw::GLuint			  m_color_data_offset;
	unsigned char*		  m_data;
	glw::GLuint			  m_data_size;		   /* ibo, vbo, color data  */
	glw::GLuint			  m_data_size_rounded; /* rounded up to page size */
	glw::GLuint			  m_fbo;
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo;
	glw::GLuint			  m_ibo_data_offset;
	_ibo_usage			  m_ibo_usage;
	const unsigned int	m_n_quad_delta_x;
	const unsigned int	m_n_quad_delta_y;
	const unsigned int	m_n_quad_height;
	const unsigned int	m_n_quad_width;
	const unsigned int	m_n_quads_x;
	const unsigned int	m_n_quads_y;
	unsigned int		  m_n_vertices_to_draw;
	bool				  m_pages_committed;
	glw::GLuint			  m_po;
	glw::GLuint			  m_sparse_bo;
	tcu::TestContext&	 m_testCtx;
	glw::GLuint			  m_to;
	const unsigned int	m_to_height;
	const unsigned int	m_to_width;
	bool				  m_use_color_data;
	glw::GLuint			  m_vao;
	glw::GLuint			  m_vbo_data_offset;
};

/** Implements test case m for the test 2:
 *
 * m. Verify query functionality works correctly, when a sparse buffer is bound
 *    to the query buffer binding point. Render a number of triangles while
 *    a GL_PRIMITIVES_GENERATED query is enabled and the BO is bound to the
 *    GL_QUERY_BUFFER binding point. Read back the value of the query from
 *    the BO and verify it is correct using glGetQueryObjectiv(),
 *    glGetQueryObjectuiv(), glGetQueryObjecti64v() and glGetQueryObjectui64v()
 *    functions.
 *
 *    Consider two scenarios:
 *
 *    a) The page holding the result value is committed.
 *    b) The page holding the result value is NOT committed. In this case,
 *       the draw call glGetQueryObjectuiv() and all the getter functions should
 *       not crash, but the reported values are irrelevant.
 */
class QueryBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	QueryBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case m";
	}

private:
	/* Private fields */
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo;
	const unsigned int	m_n_triangles;
	glw::GLint			  m_page_size;
	glw::GLuint			  m_po;
	glw::GLuint			  m_qo;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
	tcu::TestContext&	 m_testCtx;
	glw::GLuint			  m_vao;
};

/** Implements test case i for the test 2:
 *
 * i. Verify a SSBO, holding an unsized array, accessed from a compute shader,
 *    contains anticipated values. Each CS invocation should only fetch
 *    a single invocation-specific value. If the value is found correct, it
 *    should increment it.
 *
 *    The test passes if all values accessed by the CS invocations are found
 *    valid after the dispatch call.
 *
 *    Make sure to test three scenarios:
 *
 *    a) All values come from the committed memory pages.
 *    b) Use the same memory page commitment layout as proposed in b2. Verify
 *       only those values, which were available to the compute shader.
 *    c) None of the value exposed via SSBO are backed by physical memory.
 *       In this case, we do not really care about the outputs of the CS.
 *       We only need to ensure that GL (or the GPU) does not crash.
 */
class SSBOStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	SSBOStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case i";
	}

private:
	/* Private fields */
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo; /* holds m_sparse_bo_size bytes */
	glw::GLint			  m_page_size;
	glw::GLuint			  m_po;
	const unsigned int	m_po_local_wg_size;
	glw::GLuint			  m_result_bo;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
	unsigned int*		  m_ssbo_data; /* holds m_sparse_bo_size bytes */
	tcu::TestContext&	 m_testCtx;
};

/** Implements test cases b1-b2 for the test 2:
 *
 * b1. Use a sparse buffer as a target for separate & interleaved transform
 *     feed-back (in separate iterations). A sufficient number of pages should
 *     have been committed prior to issuing any draw call.
 *
 *     The vertex shader should output vertex ID & instance ID data to two
 *     different output variables captured by the TF.
 *
 *     The test should only pass if the generated output is correct.
 *
 *     For the purpose of this test, use the following draw call types:
 *
 *     * regular
 *     * regular indirect
 *     * regular indirect multi
 *     * regular instanced
 *     * regular instanced + base instance
 *     * regular multi
 *     * indexed
 *     * indexed indirect
 *     * indexed indirect multi
 *     * indexed multi
 *     * indexed multi + base vertex
 *     * indexed + base vertex
 *     * indexed + base vertex + base instance
 *     * instanced indexed
 *     * instanced indexed + base vertex
 *     * instanced indexed + base vertex + base instance
 *
 *
 * b2. Follow the same approach as described for b1. However, the commitment
 *     state of memory pages used for the TF process should be laid out in
 *     the following order:
 *
 *     1st       page:     committed
 *     2nd       page: NOT committed
 *     ...
 *     (2N)  -th page:     committed
 *     (2N+1)-th page: NOT committed
 *
 *     Make sure to use at least 4 memory pages in this test case.
 *
 *     Execute the test as described in b1, and make sure the results stored
 *     in the committed pages used by the TF process holds valid result data.
 */
class TransformFeedbackBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Type definitions */
	enum _draw_call
	{
		/* glDrawElements() */
		DRAW_CALL_INDEXED,
		/* glDrawElementsBaseVertex() */
		DRAW_CALL_INDEXED_BASE_VERTEX,
		/* glDrawElementsIndirect() */
		DRAW_CALL_INDEXED_INDIRECT,
		/* glMultiDrawElementIndirect() */
		DRAW_CALL_INDEXED_INDIRECT_MULTI,
		/* glMultiDrawElements() */
		DRAW_CALL_INDEXED_MULTI,
		/* glMultiDrawElementsBaseVertex() */
		DRAW_CALL_INDEXED_MULTI_BASE_VERTEX,
		/* glDrawElementsInstanced() */
		DRAW_CALL_INSTANCED_INDEXED,
		/* glDrawElementsInstancedBaseVertex() */
		DRAW_CALL_INSTANCED_INDEXED_BASE_VERTEX,
		/* glDrawElementsInstancedBaseVertexBaseInstance() */
		DRAW_CALL_INSTANCED_INDEXED_BASE_VERTEX_BASE_INSTANCE,
		/* glDrawArrays() */
		DRAW_CALL_REGULAR,
		/* glDrawArraysIndirect() */
		DRAW_CALL_REGULAR_INDIRECT,
		/* glMultiDrawArraysIndirect() */
		DRAW_CALL_REGULAR_INDIRECT_MULTI,
		/* glDrawArraysInstanced() */
		DRAW_CALL_REGULAR_INSTANCED,
		/* glDrawArraysInstancedBaseInstance() */
		DRAW_CALL_REGULAR_INSTANCED_BASE_INSTANCE,
		/* glMultiDrawArrays() */
		DRAW_CALL_REGULAR_MULTI,

		/* Always last */
		DRAW_CALL_COUNT
	};

	/* Public methods */
	TransformFeedbackBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext,
										   glw::GLint page_size, bool all_pages_committed);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return (m_all_pages_committed) ? "case b1" : "case b2";
	}

private:
	/* Private methods */
	const char* getDrawCallTypeString(_draw_call draw_call);
	void initDataBO();
	void initTestData();

	/* Private fields */
	bool				  m_all_pages_committed;
	glw::GLuint			  m_data_bo;
	unsigned int		  m_data_bo_index_data_offset;
	unsigned int		  m_data_bo_indexed_indirect_arg_offset;
	unsigned int		  m_data_bo_indexed_mdi_arg_offset;
	unsigned int		  m_data_bo_regular_indirect_arg_offset;
	unsigned int		  m_data_bo_regular_mdi_arg_offset;
	glw::GLuint			  m_data_bo_size;
	const unsigned int	m_draw_call_baseInstance;
	const unsigned int	m_draw_call_baseVertex;
	const unsigned int	m_draw_call_first;
	const unsigned int	m_draw_call_firstIndex;
	const glw::Functions& m_gl;
	glw::GLuint			  m_helper_bo; /* of m_result_bo_size size */
	glw::GLuint*		  m_index_data;
	glw::GLuint			  m_index_data_size;
	glw::GLuint*		  m_indirect_arg_data;
	glw::GLuint			  m_indirect_arg_data_size;
	const unsigned int	m_min_memory_page_span;
	glw::GLint			  m_multidrawcall_basevertex[2];
	glw::GLsizei		  m_multidrawcall_count[2];
	unsigned int		  m_multidrawcall_drawcount;
	glw::GLint			  m_multidrawcall_first[2];
	glw::GLvoid*		  m_multidrawcall_index[2];
	unsigned int		  m_multidrawcall_primcount;
	const unsigned int	m_n_instances_to_test;
	unsigned int		  m_n_vertices_per_instance;
	glw::GLint			  m_page_size;
	glw::GLuint			  m_po_ia; /* interleave attribs TF */
	glw::GLuint			  m_po_sa; /* separate attribs TF */
	glw::GLuint			  m_result_bo;
	glw::GLuint			  m_result_bo_size;
	glw::GLuint			  m_result_bo_size_rounded;
	tcu::TestContext&	 m_testCtx;
	glw::GLuint			  m_vao;
};

/** Implements test case j for the test 2:
 *
 * j. Verify an UBO, backed by a sparse buffer, accessed from a vertex shader,
 *    holds values as expected. Each VS invocation should only check
 *    an invocation-specific arrayed member item and set gl_Position to
 *    vec4(1.0), if the retrieved value is valid.
 *
 *    Make sure to test three scenarios as described for case i).
 */
class UniformBufferStorageTestCase : public BufferStorageTestCase
{
public:
	/* Public methods */
	UniformBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size);

	/* BufferStorageTestCase implementation */
	void deinitTestCaseGlobal();
	void deinitTestCaseIteration();
	bool execute(glw::GLuint sparse_bo_storage_flags);
	bool initTestCaseGlobal();
	bool initTestCaseIteration(glw::GLuint sparse_bo);

	const char* getName()
	{
		return "case j";
	}

private:
	/* Private fields */
	const glw::Functions& m_gl;
	glw::GLint			  m_gl_uniform_buffer_offset_alignment_value;
	glw::GLuint			  m_helper_bo;
	const unsigned int	m_n_pages_to_use;
	unsigned int		  m_n_ubo_uints;
	glw::GLint			  m_page_size;
	glw::GLuint			  m_po;
	glw::GLuint			  m_sparse_bo;
	unsigned int		  m_sparse_bo_data_size;
	unsigned int		  m_sparse_bo_data_start_offset;
	unsigned int		  m_sparse_bo_size;
	unsigned int		  m_sparse_bo_size_rounded;
	tcu::TestContext&	 m_testCtx;
	glw::GLuint			  m_tf_bo;
	unsigned char*		  m_ubo_data;
	glw::GLuint			  m_vao;
};

/** Implements conformance test 2 from the test specification:
 *
 * 2. Make sure glBufferStorage() accepts the new GL_SPARSE_STORAGE_BIT_ARB flag
 *    in all valid flag combinations. For each such combination, allocate
 *    a sparse buffer of 1GB size and verify the following test cases work as
 *    expected. After all tests have been run for a particular flag combination,
 *    the sparse buffer should be deleted, and a new sparse buffer should be
 *    created, if there are any outstanding flag combinations.
 *
 *    Test cases, whose verification behavior is incompatible with
 *    the requested flag combination should skip the validation part:
 *
 *    (for test case descriptions, please check test case class prototypes)
 */
class BufferStorageTest : public deqp::TestCase
{
public:
	/* Public methods */
	BufferStorageTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	typedef std::vector<BufferStorageTestCase*> TestCasesVector;
	typedef TestCasesVector::const_iterator		TestCasesVectorConstIterator;
	typedef TestCasesVector::iterator			TestCasesVectorIterator;

	/* Private methods */
	void initTestCases();

	/* Private members */
	glw::GLuint		m_sparse_bo;
	TestCasesVector m_testCases;
};

/** Test group which encapsulates all sparse buffer conformance tests */
class SparseBufferTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	SparseBufferTests(deqp::Context& context);

	void init();

private:
	SparseBufferTests(const SparseBufferTests& other);
	SparseBufferTests& operator=(const SparseBufferTests& other);
};

} /* glcts namespace */

#endif // _GL4CSPARSEBUFFERTESTS_HPP
