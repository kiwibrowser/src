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
 * \file  gl4cSparseBufferTests.cpp
 * \brief Conformance tests for the GL_ARB_sparse_buffer functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cSparseBufferTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <string.h>
#include <vector>

#ifndef GL_SPARSE_BUFFER_PAGE_SIZE_ARB
#define GL_SPARSE_BUFFER_PAGE_SIZE_ARB 0x82F8
#endif
#ifndef GL_SPARSE_STORAGE_BIT_ARB
#define GL_SPARSE_STORAGE_BIT_ARB 0x0400
#endif

namespace gl4cts
{
/** Rounds up the provided offset so that it is aligned to the specified value (eg. page size).
 *  In other words, the result value meets the following requirements:
 *
 *  1)  result value % input value  = 0
 *  2)  result value               >= offset
 *  3) (result value - offset)     <  input value
 *
 *  @param offset Offset to be used for the rounding operation.
 *  @param value  Value to align the offset to.
 *
 *  @return Result value.
 **/
unsigned int SparseBufferTestUtilities::alignOffset(const unsigned int& offset, const unsigned int& value)
{
	return offset + (value - offset % value) % value;
}

/** Builds a compute program object, using the user-specified CS code snippets.
 *
 *  @param gl                     DEQP CTS GL functions container.
 *  @param cs_body_parts          Code snippets to use for the compute shader. Must hold exactly
 *                                @param n_cs_body_parts null-terminated text strings.
 *  @param n_cs_body_parts        Number of code snippets accessible via @param cs_body_parts.
 *
 *  @return Result PO id if program has been linked successfully, 0 otherwise.
 **/
glw::GLuint SparseBufferTestUtilities::createComputeProgram(const glw::Functions& gl, const char** cs_body_parts,
															unsigned int n_cs_body_parts)
{
	glw::GLint  compile_status = GL_FALSE;
	glw::GLuint cs_id		   = 0;
	glw::GLint  link_status	= GL_FALSE;
	glw::GLuint po_id		   = 0;
	bool		result		   = true;

	if (n_cs_body_parts > 0)
	{
		cs_id = gl.createShader(GL_COMPUTE_SHADER);
	}

	po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() / glCreateShader() call(s) failed.");

	if (n_cs_body_parts > 0)
	{
		gl.attachShader(po_id, cs_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	if (n_cs_body_parts > 0)
	{
		gl.shaderSource(cs_id, n_cs_body_parts, cs_body_parts, NULL); /* length */
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");

	gl.compileShader(cs_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(cs_id, GL_COMPILE_STATUS, &compile_status);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

	char temp[1024];
	gl.getShaderInfoLog(cs_id, 1024, NULL, temp);

	if (GL_TRUE != compile_status)
	{
		result = false;

		goto end;
	}

	gl.linkProgram(po_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (GL_TRUE != link_status)
	{
		result = false;

		goto end;
	}

end:
	if (cs_id != 0)
	{
		gl.deleteShader(cs_id);

		cs_id = 0;
	}

	if (!result)
	{
		if (po_id != 0)
		{
			gl.deleteProgram(po_id);

			po_id = 0;
		}
	} /* if (!result) */

	return po_id;
}

/** Builds a program object, using the user-specified code snippets. Can optionally configure
 *  the PO to use pre-defined attribute locations & transform feed-back varyings.
 *
 *  @param gl                     DEQP CTS GL functions container.
 *  @param fs_body_parts          Code snippets to use for the fragment shader. Must hold exactly
 *                                @param n_fs_body_parts null-terminated text strings. May only
 *                                be NULL if @param n_fs_body_parts is 0.
 *  @param n_fs_body_parts        See @param fs_body_parts definitions.
 *  @param vs_body_parts          Code snippets to use for the vertex shader. Must hold exactly
 *                                @param n_vs_body_parts null-terminated text strings. May only
 *                                be NULL if @param n_vs_body_parts is 0.
 *  @param n_vs_body_parts        See @param vs_body_parts definitions.
 *  @param attribute_names        Null-terminated attribute names to pass to the
 *                                glBindAttribLocation() call.
 *                                May only be NULL if @param n_attribute_properties is 0.
 *  @param attribute_locations    Attribute locations to pass to the glBindAttribLocation() call.
 *                                May only be NULL if @param n_attribute_properties is 0.
 *  @param n_attribute_properties See @param attribute_names and @param attribute_locations definitions.
 *  @param tf_varyings            Transform-feedback varying names to use for the
 *                                glTransformFeedbackVaryings() call. May only be NULL if
 *                                @param n_tf_varyings is 0.
 *  @param n_tf_varyings          See @param tf_varyings definition.
 *  @param tf_varying_mode        Transform feedback mode to use for the
 *                                glTransformFeedbackVaryings() call. Only used if @param n_tf_varyings
 *                                is 0.
 *
 *  @return Result PO id if program has been linked successfully, 0 otherwise.
 **/
glw::GLuint SparseBufferTestUtilities::createProgram(const glw::Functions& gl, const char** fs_body_parts,
													 unsigned int n_fs_body_parts, const char** vs_body_parts,
													 unsigned int n_vs_body_parts, const char** attribute_names,
													 const unsigned int*	   attribute_locations,
													 unsigned int			   n_attribute_properties,
													 const glw::GLchar* const* tf_varyings, unsigned int n_tf_varyings,
													 glw::GLenum tf_varying_mode)
{
	glw::GLint  compile_status = GL_FALSE;
	glw::GLuint fs_id		   = 0;
	glw::GLint  link_status	= GL_FALSE;
	glw::GLuint po_id		   = 0;
	bool		result		   = true;
	glw::GLuint vs_id		   = 0;

	if (n_fs_body_parts > 0)
	{
		fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	}

	po_id = gl.createProgram();

	if (n_vs_body_parts > 0)
	{
		vs_id = gl.createShader(GL_VERTEX_SHADER);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() / glCreateShader() call(s) failed.");

	if (n_fs_body_parts > 0)
	{
		gl.attachShader(po_id, fs_id);
	}

	if (n_vs_body_parts > 0)
	{
		gl.attachShader(po_id, vs_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	if (n_fs_body_parts > 0)
	{
		gl.shaderSource(fs_id, n_fs_body_parts, fs_body_parts, NULL); /* length */
	}

	if (n_vs_body_parts > 0)
	{
		gl.shaderSource(vs_id, n_vs_body_parts, vs_body_parts, NULL); /* length */
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");

	const glw::GLuint  so_ids[] = { fs_id, vs_id };
	const unsigned int n_so_ids = sizeof(so_ids) / sizeof(so_ids[0]);

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		if (so_ids[n_so_id] != 0)
		{
			gl.compileShader(so_ids[n_so_id]);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

			gl.getShaderiv(so_ids[n_so_id], GL_COMPILE_STATUS, &compile_status);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

			char temp[1024];
			gl.getShaderInfoLog(so_ids[n_so_id], 1024, NULL, temp);

			if (GL_TRUE != compile_status)
			{
				result = false;

				goto end;
			}
		} /* if (so_ids[n_so_id] != 0) */
	}	 /* for (all shader object IDs) */

	for (unsigned int n_attribute = 0; n_attribute < n_attribute_properties; ++n_attribute)
	{
		gl.bindAttribLocation(po_id, attribute_locations[n_attribute], attribute_names[n_attribute]);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindAttribLocation() call failed.");
	} /* for (all attributes to configure) */

	if (n_tf_varyings != 0)
	{
		gl.transformFeedbackVaryings(po_id, n_tf_varyings, tf_varyings, tf_varying_mode);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");
	} /* if (n_tf_varyings != 0) */

	gl.linkProgram(po_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (GL_TRUE != link_status)
	{
		result = false;

		goto end;
	}

end:
	if (fs_id != 0)
	{
		gl.deleteShader(fs_id);

		fs_id = 0;
	}

	if (vs_id != 0)
	{
		gl.deleteShader(vs_id);

		vs_id = 0;
	}

	if (!result)
	{

		if (po_id != 0)
		{
			gl.deleteProgram(po_id);

			po_id = 0;
		}
	} /* if (!result) */

	return po_id;
}

/** Returns a string with textual representation of the @param flags bitfield
 *  holding bits applicable to the @param flags argument of glBufferStorage()
 *  calls.
 *
 *  @param flags Flags argument, as supported by the @param flags argument of
 *               glBufferStorage() entry-point.
 *
 *  @return Described string.
 **/
std::string SparseBufferTestUtilities::getSparseBOFlagsString(glw::GLenum flags)
{
	unsigned int	  n_flags_added = 0;
	std::stringstream result_sstream;

	if ((flags & GL_CLIENT_STORAGE_BIT) != 0)
	{
		result_sstream << "GL_CLIENT_STORAGE_BIT";

		++n_flags_added;
	}

	if ((flags & GL_DYNAMIC_STORAGE_BIT) != 0)
	{
		result_sstream << ((n_flags_added) ? " | " : "") << "GL_DYNAMIC_STORAGE_BIT";

		++n_flags_added;
	}

	if ((flags & GL_MAP_COHERENT_BIT) != 0)
	{
		result_sstream << ((n_flags_added) ? " | " : "") << "GL_MAP_COHERENT_BIT";

		++n_flags_added;
	}

	if ((flags & GL_MAP_PERSISTENT_BIT) != 0)
	{
		result_sstream << ((n_flags_added) ? " | " : "") << "GL_MAP_PERSISTENT_BIT";

		++n_flags_added;
	}

	if ((flags & GL_SPARSE_STORAGE_BIT_ARB) != 0)
	{
		result_sstream << ((n_flags_added) ? " | " : "") << "GL_SPARSE_STORAGE_BIT";

		++n_flags_added;
	}

	return result_sstream.str();
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
NegativeTests::NegativeTests(deqp::Context& context)
	: TestCase(context, "NegativeTests", "Implements all negative tests described in CTS_ARB_sparse_buffer")
	, m_helper_bo_id(0)
	, m_immutable_bo_id(0)
	, m_immutable_bo_size(1024768)
	, m_sparse_bo_id(0)
{
	/* Left blank intentionally */
}

/** Stub deinit method. */
void NegativeTests::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_helper_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_helper_bo_id);

		m_helper_bo_id = 0;
	}

	if (m_immutable_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_immutable_bo_id);

		m_immutable_bo_id = 0;
	}

	if (m_sparse_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_sparse_bo_id);

		m_sparse_bo_id = 0;
	}
}

/** Stub init method */
void NegativeTests::init()
{
	/* Nothing to do here */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTests::iterate()
{
	glw::GLvoid*		  data_ptr  = DE_NULL;
	const glw::Functions& gl		= m_context.getRenderContext().getFunctions();
	glw::GLint			  page_size = 0;
	bool				  result	= true;

	/* Only execute if the implementation supports the GL_ARB_sparse_buffer extension */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_buffer"))
	{
		throw tcu::NotSupportedError("GL_ARB_sparse_buffer is not supported");
	}

	/* Set up */
	gl.getIntegerv(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &page_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed.");

	gl.genBuffers(1, &m_helper_bo_id);
	gl.genBuffers(1, &m_immutable_bo_id);
	gl.genBuffers(1, &m_sparse_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call(s) failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo_id);
	gl.bindBuffer(GL_COPY_READ_BUFFER, m_immutable_bo_id);
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_helper_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferStorage(GL_ARRAY_BUFFER, page_size * 3, /* size as per test spec */
					 DE_NULL,						 /* data */
					 GL_SPARSE_STORAGE_BIT_ARB);
	gl.bufferStorage(GL_COPY_READ_BUFFER, m_immutable_bo_size, /* size */
					 DE_NULL,								   /* data */
					 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage() call(s) failed.");

	/** * Verify glBufferPageCommitmentARB() returns GL_INVALID_ENUM if <target> is
	 *    set to GL_INTERLEAVED_ATTRIBS. */
	glw::GLint error_code = GL_NO_ERROR;

	gl.bufferPageCommitmentARB(GL_INTERLEAVED_ATTRIBS, 0, /* offset */
							   page_size, GL_TRUE);		  /* commit */

	error_code = gl.getError();
	if (error_code != GL_INVALID_ENUM)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid <target> value passed to a glBufferPageCommitmentARB() call"
							  " did not generate a GL_INVALID_ENUM error."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/*  * Verify glBufferStorage() throws a GL_INVALID_VALUE error if <flags> is
	 *    set to (GL_SPARSE_STORAGE_BIT_ARB | GL_MAP_READ_BIT) or
	 *    (GL_SPARSE_STORAGE_BIT_ARB | GL_MAP_WRITE_BIT). */
	gl.bufferStorage(GL_ELEMENT_ARRAY_BUFFER, page_size * 3, /* size */
					 DE_NULL,								 /* data */
					 GL_SPARSE_STORAGE_BIT_ARB | GL_MAP_READ_BIT);

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid <flags> value set to GL_SPARSE_STORAGE_BIT_ARB | GL_MAP_READ_BIT "
							  "did not generate a GL_INVALID_VALUE error."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	gl.bufferStorage(GL_ELEMENT_ARRAY_BUFFER, page_size * 3, /* size */
					 DE_NULL,								 /* data */
					 GL_SPARSE_STORAGE_BIT_ARB | GL_MAP_WRITE_BIT);

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid <flags> value set to GL_SPARSE_STORAGE_BIT_ARB | GL_MAP_WRITE_BIT "
							  "did not generate a GL_INVALID_VALUE error."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/*  * Verify glBufferPageCommitmentARB() generates a GL_INVALID_OPERATION error if
	 *    it is called for an immutable BO, which has not been initialized with the
	 *    GL_SPARSE_STORAGE_BIT_ARB flag. */
	gl.bufferPageCommitmentARB(GL_COPY_READ_BUFFER, 0, /* offset */
							   page_size, GL_TRUE);	/* commit */

	error_code = gl.getError();
	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid error code generated by glBufferPageCommitmentARB() "
													   " issued against an immutable, non-sparse buffer object."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/*  * Verify glBufferPageCommitmentARB() issues a GL_INVALID_VALUE error if <offset>
	 *    is set to (0.5 * GL_SPARSE_BUFFER_PAGE_SIZE_ARB). Skip if the constant's value
	 *    is equal to 1. */
	if (page_size != 1)
	{
		gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, page_size / 2, /* offset */
								   page_size, GL_TRUE);			   /* commit */

		error_code = gl.getError();
		if (error_code != GL_INVALID_VALUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid error code generated by glBufferPageCommitmentARB() "
								  "whose <offset> value was set to (page size / 2)."
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* if (page_size != 1) */

	/*  * Verify glBufferPageCommitmentARB() emits a GL_INVALID_VALUE error if <size>
	 *    is set to (0.5 * GL_SPARSE_BUFFER_PAGE_SIZE_ARB). Skip if the constant's value
	 *    is equal to 1. */
	if (page_size != 1)
	{
		gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,		/* offset */
								   page_size / 2, GL_TRUE); /* commit */

		error_code = gl.getError();
		if (error_code != GL_INVALID_VALUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid error code generated by glBufferPageCommitmentARB() "
								  "whose <size> value was set to (page size / 2)."
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* if (page_size != 1) */

	/*  * Verify glBufferPageCommitmentARB() returns GL_INVALID_VALUE if <offset> is
	 *    set to -1, but all other arguments are valid. */
	gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, -1, /* offset */
							   page_size, GL_TRUE); /* commit */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid error code generated by glBufferPageCommitmentARB() "
													   "whose <offset> argument was set to -1."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/*  * Verify glBufferPageCommitmentARB() returns GL_INVALID_VALUE if <size> is
	 *    set to -1, but all other arguments are valid. */
	gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0, /* offset */
							   -1,				   /* size */
							   GL_TRUE);		   /* commit */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid error code generated by glBufferPageCommitmentARB() "
													   "whose <size> argument was set to -1."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/*  * Verify glBufferPageCommitmentARB() returns GL_INVALID_VALUE if BO's size is
	 *    GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 3, but the <offset> is set to 0 and <size>
	 *    argument used for the call is set to GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 4. */
	gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0, /* offset */
							   page_size * 4,	  /* size */
							   GL_TRUE);

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid error code generated by glBufferPageCommitmentARB() "
							  "whose <offset> was set to 0 and <size> was set to (page size * 4), "
							  "when the buffer storage size had been configured to be (page size * 3)."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/*  * Verify glBufferPageCommitmentARB() returns GL_INVALID_VALUE if BO's size is
	 *    GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 3, but the <offset> is set to
	 *    GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 1 and <size> argument used for the call
	 *    is set to GL_SPARSE_BUFFER_PAGE_SIZE_ARB * 3. */
	gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, page_size * 1, /* offset */
							   page_size * 3,				   /* size */
							   GL_TRUE);

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid error code generated by glBufferPageCommitmentARB() "
							  "whose <offset> was set to (page size) and <size> was set to (page size * 3), "
							  "when the buffer storage size had been configured to be (page size * 3)."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/*  * Verify that calling glMapBuffer() or glMapBufferRange() against a sparse
	 *    buffer generates a GL_INVALID_OPERATION error. */
	data_ptr = gl.mapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);

	if (data_ptr != DE_NULL)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Non-NULL pointer returned by an invalid glMapBuffer() call, issued "
							  "against a sparse buffer object"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	error_code = gl.getError();

	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid error code generated by glMapBuffer() call, issued against "
							  "a sparse buffer object"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	data_ptr = gl.mapBufferRange(GL_ARRAY_BUFFER, 0, /* offset */
								 page_size,			 /* length */
								 GL_MAP_READ_BIT);

	if (data_ptr != DE_NULL)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Non-NULL pointer returned by an invalid glMapBufferRange() call, issued "
							  "against a sparse buffer object"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	error_code = gl.getError();

	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid error code generated by glMapBufferRange() call, issued against "
							  "a sparse buffer object"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
PageSizeGetterTest::PageSizeGetterTest(deqp::Context& context)
	: TestCase(context, "PageSizeGetterTest",
			   "Verifies GL_SPARSE_BUFFER_PAGE_SIZE_ARB pname is recognized by the getter functions")
{
	/* Left blank intentionally */
}

/** Stub deinit method. */
void PageSizeGetterTest::deinit()
{
	/* Nothing to be done here */
}

/** Stub init method */
void PageSizeGetterTest::init()
{
	/* Nothing to do here */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult PageSizeGetterTest::iterate()
{
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	glw::GLboolean		  page_size_bool   = false;
	glw::GLdouble		  page_size_double = 0.0;
	glw::GLfloat		  page_size_float  = 0.0f;
	glw::GLint			  page_size_int	= 0;
	glw::GLint64		  page_size_int64  = 0;
	bool				  result		   = true;

	/* Only execute if the implementation supports the GL_ARB_sparse_buffer extension */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_buffer"))
	{
		throw tcu::NotSupportedError("GL_ARB_sparse_buffer is not supported");
	}

	/* glGetIntegerv() */
	gl.getIntegerv(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &page_size_int);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed");

	if (page_size_int < 1 || page_size_int > 65536)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Page size reported by the implementation (" << page_size_int
						   << ")"
							  " by glGetIntegerv() is out of the allowed range."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* glGetBooleanv() */
	gl.getBooleanv(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &page_size_bool);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv() call failed");

	if (!page_size_bool)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid page size reported by glGetBooleanv()"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* glGetDoublev() */
	gl.getDoublev(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &page_size_double);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetDoublev() call failed");

	if (de::abs(page_size_double - page_size_int) > 1e-5)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid page size reported by glGetDoublev()"
													   " (reported value: "
						   << page_size_double << ", expected value: " << page_size_int << ")"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* glGetFloatv() */
	gl.getFloatv(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &page_size_float);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() call failed");

	if (de::abs(page_size_float - static_cast<float>(page_size_int)) > 1e-5f)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid page size reported by glGetFloatv()"
													   " (reported value: "
						   << page_size_float << ", expected value: " << page_size_int << ")"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* glGetInteger64v() */
	gl.getInteger64v(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &page_size_int64);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() call failed");

	if (page_size_int64 != page_size_int)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid page size reported by glGetInteger64v()"
													   " (reported value: "
						   << page_size_int64 << ", expected value: " << page_size_int << ")"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 *  @param all_pages_committed        true to run the test with all data memory pages committed,
 *                                    false to leave some of them without an actual memory backing.
 */
AtomicCounterBufferStorageTestCase::AtomicCounterBufferStorageTestCase(const glw::Functions& gl,
																	   tcu::TestContext&	 testContext,
																	   glw::GLint page_size, bool all_pages_committed)
	: m_all_pages_committed(all_pages_committed)
	, m_gl(gl)
	, m_gl_atomic_counter_uniform_array_stride(0)
	, m_gl_max_vertex_atomic_counters_value(0)
	, m_helper_bo(0)
	, m_helper_bo_size(0)
	, m_helper_bo_size_rounded(0)
	, m_n_draw_calls(3) /* as per test spec */
	, m_page_size(page_size)
	, m_po(0)
	, m_sparse_bo(0)
	, m_sparse_bo_data_size(0)
	, m_sparse_bo_data_size_rounded(0)
	, m_sparse_bo_data_start_offset(0)
	, m_sparse_bo_data_start_offset_rounded(0)
	, m_testCtx(testContext)
	, m_vao(0)
{
	/* Left blank intentionally */
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void AtomicCounterBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_po != 0)
	{
		m_gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_vao != 0)
	{
		m_gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void AtomicCounterBufferStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_helper_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool AtomicCounterBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	static const unsigned char data_zero = 0;
	bool					   result	= true;

	/* Only execute if GL_MAX_VERTEX_ATOMIC_COUNTERS is > 0 */
	if (m_gl_max_vertex_atomic_counters_value == 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "G_MAX_VERTEX_ATOMIC_COUNTERS is 0. Skipping the test."
						   << tcu::TestLog::EndMessage;

		goto end;
	}

	/* Bind the test program object */
	m_gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

	m_gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	/* Try using both ranged and non-ranged AC bindings.
	 *
	 * NOTE: It only makes sense to perform glBindBufferBase() test if all AC pages are
	 *       committed
	 */
	for (unsigned int n_binding_type = (m_all_pages_committed) ? 0 : 1;
		 n_binding_type < 2; /* glBindBufferBase(), glBindBufferRange() */
		 ++n_binding_type)
	{
		bool result_local = true;

		if (n_binding_type == 0)
		{
			m_gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, /* index */
								m_sparse_bo);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferBase() call failed.");
		}
		else
		{
			m_gl.bindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, /* index */
								 m_sparse_bo, m_sparse_bo_data_start_offset, m_helper_bo_size);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferRange() call failed.");
		}

		/* Zero out the sparse buffer's contents */
		m_gl.clearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &data_zero);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearBufferData() call failed.");

		/* Run the test */
		m_gl.drawArraysInstanced(GL_POINTS, 0,							/* first */
								 m_gl_max_vertex_atomic_counters_value, /* count */
								 m_n_draw_calls);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArraysInstanced() call failed");

		/* Retrieve the atomic counter values */
		const glw::GLuint* ac_data = NULL;
		const unsigned int n_expected_written_values =
			(m_all_pages_committed) ? m_gl_max_vertex_atomic_counters_value : m_gl_max_vertex_atomic_counters_value / 2;

		m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_sparse_bo);
		m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_helper_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffeR() call failed");

		m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
							   (n_binding_type == 0) ? 0 : m_sparse_bo_data_start_offset, 0, /* writeOffset */
							   m_sparse_bo_data_size);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

		ac_data = (const glw::GLuint*)m_gl.mapBufferRange(GL_COPY_WRITE_BUFFER, 0, /* offset */
														  m_sparse_bo_data_size, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBufferRange() call failed.");

		for (unsigned int n_counter = 0; n_counter < n_expected_written_values && result_local; ++n_counter)
		{
			const unsigned int expected_value = m_n_draw_calls;
			const unsigned int retrieved_value =
				*((unsigned int*)((unsigned char*)ac_data + m_gl_atomic_counter_uniform_array_stride * n_counter));

			if (expected_value != retrieved_value)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid atomic counter value "
															   "["
								   << retrieved_value << "]"
														 " instead of the expected value "
														 "["
								   << expected_value << "]"
														" at index "
								   << n_counter << " when using "
								   << ((n_binding_type == 0) ? "glBindBufferBase()" : "glBindBufferRange()")
								   << " for AC binding configuration" << tcu::TestLog::EndMessage;

				result_local = false;
			} /* if (expected_value != retrieved_value) */
		}	 /* for (all draw calls that need to be executed) */

		m_gl.unmapBuffer(GL_COPY_WRITE_BUFFER);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");

		result &= result_local;
	} /* for (both binding types) */

end:
	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool AtomicCounterBufferStorageTestCase::initTestCaseGlobal()
{
	const glw::GLuint ac_uniform_index = 0; /* only one uniform is defined in the VS below */
	std::stringstream n_counters_sstream;
	std::string		  n_counters_string;
	bool			  result = true;

	static const char* vs_body_preamble = "#version 430 core\n"
										  "\n";

	static const char* vs_body_core = "layout(binding = 0) uniform atomic_uint counters[N_COUNTERS];\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    for (uint n = 0; n < N_COUNTERS; ++n)\n"
									  "    {\n"
									  "        if (n == gl_VertexID)\n"
									  "        {\n"
									  "            atomicCounterIncrement(counters[n]);\n"
									  "        }\n"
									  "    }\n"
									  "\n"
									  "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
									  "}\n";
	const char* vs_body_parts[] = { vs_body_preamble, DE_NULL, /* will be set to n_counters_string.c_str() */
									vs_body_core };
	const unsigned int n_vs_body_parts = sizeof(vs_body_parts) / sizeof(vs_body_parts[0]);

	/* Retrieve GL_MAX_VERTEX_ATOMIC_COUNTERS value. The test will only be executed if it's >= 1 */
	m_gl.getIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &m_gl_max_vertex_atomic_counters_value);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetIntegerv() call failed.");

	if (m_gl_max_vertex_atomic_counters_value == 0)
	{
		goto end;
	}

	/* Form the N_COUNTERS declaration string */
	n_counters_sstream << "#define N_COUNTERS " << m_gl_max_vertex_atomic_counters_value << "\n";
	n_counters_string = n_counters_sstream.str();

	vs_body_parts[1] = n_counters_string.c_str();

	/* Set up the program object */
	DE_ASSERT(m_po == 0);

	m_po =
		SparseBufferTestUtilities::createProgram(m_gl, DE_NULL,							  /* fs_body_parts   */
												 0,										  /* n_fs_body_parts */
												 vs_body_parts, n_vs_body_parts, DE_NULL, /* attribute_names        */
												 DE_NULL,								  /* attribute_locations    */
												 0);									  /* n_attribute_properties */

	if (m_po == 0)
	{
		result = false;

		goto end;
	}

	/* Helper BO will be used to hold the atomic counter buffer data.
	 * Determine how much space will be needed.
	 *
	 * Min max for the GL constant value is 0. Bail out if that's the
	 * value we are returned - it is pointless to execute the test in
	 * such environment.
	 */
	m_gl.getActiveUniformsiv(m_po, 1, /* uniformCount */
							 &ac_uniform_index, GL_UNIFORM_ARRAY_STRIDE, &m_gl_atomic_counter_uniform_array_stride);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetActiveUniformsiv() call failed.");

	DE_ASSERT(m_gl_atomic_counter_uniform_array_stride >= (int)sizeof(unsigned int));

	m_helper_bo_size		 = m_gl_atomic_counter_uniform_array_stride * m_gl_max_vertex_atomic_counters_value;
	m_helper_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_helper_bo_size, m_page_size);

	/* Set up the helper BO */
	DE_ASSERT(m_helper_bo == 0);

	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_COPY_READ_BUFFER, m_helper_bo_size_rounded, DE_NULL, GL_MAP_READ_BIT); /* flags */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	/* Set up the vertex array object */
	DE_ASSERT(m_vao == 0);

	m_gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenVertexArrays() call failed.");

	m_gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindVertexArray() call failed.");

end:
	return result;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool AtomicCounterBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	/* Set up the sparse bufffer. */
	int sparse_bo_data_size = 0;

	DE_ASSERT(m_helper_bo_size_rounded != 0);

	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	if (m_all_pages_committed)
	{
		/* Commit all required pages */
		sparse_bo_data_size = m_helper_bo_size_rounded;
	}
	else
	{
		/* Only commit the first half of the required pages */
		DE_ASSERT((m_helper_bo_size_rounded % m_page_size) == 0);

		sparse_bo_data_size = (m_helper_bo_size_rounded / m_page_size) * m_page_size / 2;
	}

	/* NOTE: We need to ensure that the memory region assigned to the atomic counter buffer spans
	 *       at least through two separate pages.
	 *
	 * Since we align up, we need to move one page backward and then apply the alignment function
	 * to determine the start page index.
	 */
	const int sparse_bo_data_start_offset			 = m_page_size - m_helper_bo_size_rounded / 2;
	int		  sparse_bo_data_start_offset_minus_page = sparse_bo_data_start_offset - m_page_size;

	if (sparse_bo_data_start_offset_minus_page < 0)
	{
		sparse_bo_data_start_offset_minus_page = 0;
	}

	m_sparse_bo_data_start_offset = sparse_bo_data_start_offset;
	m_sparse_bo_data_start_offset_rounded =
		SparseBufferTestUtilities::alignOffset(sparse_bo_data_start_offset_minus_page, m_page_size);
	m_sparse_bo_data_size = sparse_bo_data_size;
	m_sparse_bo_data_size_rounded =
		SparseBufferTestUtilities::alignOffset(m_sparse_bo_data_start_offset + sparse_bo_data_size, m_page_size);

	DE_ASSERT((m_sparse_bo_data_size_rounded % m_page_size) == 0);
	DE_ASSERT((m_sparse_bo_data_start_offset_rounded % m_page_size) == 0);

	m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, m_sparse_bo_data_start_offset_rounded, m_sparse_bo_data_size_rounded,
								 GL_TRUE); /* commit */

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param context                    CTS rendering context
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
BufferTextureStorageTestCase::BufferTextureStorageTestCase(const glw::Functions& gl, deqp::Context& context,
														   tcu::TestContext& testContext, glw::GLint page_size)
	: m_gl(gl)
	, m_helper_bo(0)
	, m_helper_bo_data(DE_NULL)
	, m_helper_bo_data_size(0)
	, m_is_texture_buffer_range_supported(false)
	, m_page_size(page_size)
	, m_po(0)
	, m_po_local_wg_size(1024)
	, m_sparse_bo(0)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
	, m_ssbo(0)
	, m_ssbo_zero_data(DE_NULL)
	, m_ssbo_zero_data_size(0)
	, m_testCtx(testContext)
	, m_to(0)
	, m_to_width(65536) /* min max for GL_MAX_TEXTURE_BUFFER_SIZE_ARB */
{
	const glu::ContextInfo& context_info   = context.getContextInfo();
	glu::RenderContext&		render_context = context.getRenderContext();

	if (glu::contextSupports(render_context.getType(), glu::ApiType::core(4, 3)) ||
		context_info.isExtensionSupported("GL_ARB_texture_buffer_range"))
	{
		m_is_texture_buffer_range_supported = true;
	}
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void BufferTextureStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_helper_bo_data != DE_NULL)
	{
		delete[] m_helper_bo_data;

		m_helper_bo_data = DE_NULL;
	}

	if (m_po != 0)
	{
		m_gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_ssbo != 0)
	{
		m_gl.deleteBuffers(1, &m_ssbo);

		m_ssbo = 0;
	}

	if (m_ssbo_zero_data != DE_NULL)
	{
		delete[] m_ssbo_zero_data;

		m_ssbo_zero_data = DE_NULL;
	}

	if (m_to != 0)
	{
		m_gl.deleteTextures(1, &m_to);

		m_to = 0;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void BufferTextureStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool BufferTextureStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool result = true;

	/* Bind the program object */
	m_gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

	m_gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, /* index */
						m_ssbo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferBase() call failed.");

	/* Set up bindings for the copy ops */
	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

	/* Run the test in two iterations:
	 *
	 * a) All required pages are committed.
	 * b) Only half of the pages are committed. */
	for (unsigned int n_iteration = 0; n_iteration < 2; ++n_iteration)
	{

		/* Test glTexBuffer() and glTexBufferRange() separately. */
		for (int n_entry_point = 0; n_entry_point < (m_is_texture_buffer_range_supported ? 2 : 1); ++n_entry_point)
		{
			bool result_local = true;

			/* Set up the sparse buffer's memory backing. */
			const unsigned int tbo_commit_start_offset = (n_iteration == 0) ? 0 : m_sparse_bo_size_rounded / 2;
			const unsigned int tbo_commit_size =
				(n_iteration == 0) ? m_sparse_bo_size_rounded : m_sparse_bo_size_rounded / 2;

			m_gl.bindBuffer(GL_TEXTURE_BUFFER, m_sparse_bo);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

			m_gl.bufferPageCommitmentARB(GL_TEXTURE_BUFFER, tbo_commit_start_offset, tbo_commit_size,
										 GL_TRUE); /* commit */
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

			/* Set up the buffer texture's backing */
			if (n_entry_point == 0)
			{
				m_gl.texBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, m_sparse_bo);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glTexBuffer() call failed.");
			}
			else
			{
				m_gl.texBufferRange(GL_TEXTURE_BUFFER, GL_RGBA8, m_sparse_bo, 0, /* offset */
									m_sparse_bo_size);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glTexBufferRange() call failed.");
			}

			/* Set up the sparse buffer's data storage */
			m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
								   0,											 /* writeOffset */
								   m_helper_bo_data_size);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

			/* Run the compute program */
			DE_ASSERT((m_to_width % m_po_local_wg_size) == 0);

			m_gl.dispatchCompute(m_to_width / m_po_local_wg_size, 1, /* num_groups_y */
								 1);								 /* num_groups_z */
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDispatchCompute() call failed.");

			/* Flush the caches */
			m_gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMemoryBarrier() call failed.");

			/* Map the SSBO into process space, so we can check if the texture buffer's
			 * contents was found valid by the compute shader */
			unsigned int		current_tb_offset = 0;
			const unsigned int* ssbo_data_ptr =
				(const unsigned int*)m_gl.mapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBuffer() call failed.");

			for (unsigned int n_texel = 0; n_texel < m_to_width && result_local;
				 ++n_texel, current_tb_offset += 4 /* rgba */)
			{
				/* NOTE: Since the CS uses std140 layout, we need to move by 4 ints for
				 *       each result value */
				if (current_tb_offset >= tbo_commit_start_offset &&
					current_tb_offset < (tbo_commit_start_offset + tbo_commit_size) && ssbo_data_ptr[n_texel * 4] != 1)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "A texel read from the texture buffer at index "
																   "["
									   << n_texel << "]"
													 " was marked as invalid by the CS invocation."
									   << tcu::TestLog::EndMessage;

					result_local = false;
				} /* if (ssbo_data_ptr[n_texel] != 1) */
			}	 /* for (all result values) */

			result &= result_local;

			m_gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");

			/* Remove the physical backing from the sparse buffer  */
			m_gl.bufferPageCommitmentARB(GL_TEXTURE_BUFFER, 0,				  /* offset */
										 m_sparse_bo_size_rounded, GL_FALSE); /* commit */

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

			/* Reset SSBO's contents */
			m_gl.bufferSubData(GL_SHADER_STORAGE_BUFFER, 0, /* offset */
							   m_ssbo_zero_data_size, m_ssbo_zero_data);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferSubData() call failed.");
		} /* for (both entry-points) */
	}	 /* for (both iterations) */

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool BufferTextureStorageTestCase::initTestCaseGlobal()
{
	/* Set up the test program */
	static const char* cs_body =
		"#version 430 core\n"
		"\n"
		"layout(local_size_x = 1024) in;\n"
		"\n"
		"layout(std140, binding = 0) buffer data\n"
		"{\n"
		"    restrict writeonly int result[];\n"
		"};\n"
		"\n"
		"uniform samplerBuffer input_texture;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    uint texel_index = gl_GlobalInvocationID.x;\n"
		"\n"
		"    if (texel_index < 65536)\n"
		"    {\n"
		"        vec4 expected_texel_data = vec4       (float((texel_index)       % 255) / 255.0,\n"
		"                                               float((texel_index + 35)  % 255) / 255.0,\n"
		"                                               float((texel_index + 78)  % 255) / 255.0,\n"
		"                                               float((texel_index + 131) % 255) / 255.0);\n"
		"        vec4 texel_data          = texelFetch(input_texture, int(texel_index) );\n"
		"\n"
		"        if (abs(texel_data.r - expected_texel_data.r) > 1.0 / 255.0 ||\n"
		"            abs(texel_data.g - expected_texel_data.g) > 1.0 / 255.0 ||\n"
		"            abs(texel_data.b - expected_texel_data.b) > 1.0 / 255.0 ||\n"
		"            abs(texel_data.a - expected_texel_data.a) > 1.0 / 255.0)\n"
		"        {\n"
		"            result[texel_index] = 0;\n"
		"        }\n"
		"        else\n"
		"        {\n"
		"            result[texel_index] = 1;\n"
		"        }\n"
		"    }\n"
		"}\n";

	m_po = SparseBufferTestUtilities::createComputeProgram(m_gl, &cs_body, 1); /* n_cs_body_parts */

	/* Set up a data buffer we will use to initialize the SSBO with default data.
	 *
	 * CS uses a std140 layout for the SSBO, so we need to add the additional padding.
	 */
	m_ssbo_zero_data_size = static_cast<unsigned int>(4 * sizeof(int) * m_to_width);
	m_ssbo_zero_data	  = new unsigned char[m_ssbo_zero_data_size];

	memset(m_ssbo_zero_data, 0, m_ssbo_zero_data_size);

	/* Set up the SSBO */
	m_gl.genBuffers(1, &m_ssbo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferData(GL_SHADER_STORAGE_BUFFER, m_ssbo_zero_data_size, m_ssbo_zero_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferData() call failed.");

	/* During execution, we will need to use a helper buffer object. The BO will hold
	 * data we will be copying into the sparse buffer object for each iteration.
	 *
	 * Create an array to hold the helper buffer's data and fill it with info that
	 * the compute shader is going to be expecting */
	unsigned char* helper_bo_data_traveller_ptr = NULL;

	m_helper_bo_data_size = m_to_width * 4; /* rgba */
	m_helper_bo_data	  = new unsigned char[m_helper_bo_data_size];

	helper_bo_data_traveller_ptr = m_helper_bo_data;

	for (unsigned int n_texel = 0; n_texel < m_to_width; ++n_texel)
	{
		/* Red */
		*helper_bo_data_traveller_ptr = static_cast<unsigned char>(n_texel % 255);
		++helper_bo_data_traveller_ptr;

		/* Green */
		*helper_bo_data_traveller_ptr = static_cast<unsigned char>((n_texel + 35) % 255);
		++helper_bo_data_traveller_ptr;

		/* Blue */
		*helper_bo_data_traveller_ptr = static_cast<unsigned char>((n_texel + 78) % 255);
		++helper_bo_data_traveller_ptr;

		/* Alpha */
		*helper_bo_data_traveller_ptr = static_cast<unsigned char>((n_texel + 131) % 255);
		++helper_bo_data_traveller_ptr;
	} /* for (all texels to be accessible via the buffer texture) */

	/* Set up the helper buffer object which we are going to use to copy data into
	 * the sparse buffer object. */
	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferData(GL_COPY_READ_BUFFER, m_helper_bo_data_size, m_helper_bo_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferData() call failed.");

	/* Set up the texture buffer object. We will attach the actual buffer storage
	 * in execute() */
	m_gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenTextures() call failed.");

	m_gl.bindTexture(GL_TEXTURE_BUFFER, m_to);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindTexture() call failed.");

	/* Determine the number of bytes both the helper and the sparse buffer
	 * object need to be able to hold, at maximum */
	m_sparse_bo_size		 = static_cast<unsigned int>(m_to_width * sizeof(int));
	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_sparse_bo_size, m_page_size);

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool BufferTextureStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
ClearOpsBufferStorageTestCase::ClearOpsBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext,
															 glw::GLint page_size)
	: m_gl(gl)
	, m_helper_bo(0)
	, m_initial_data(DE_NULL)
	, m_n_pages_to_use(16)
	, m_page_size(page_size)
	, m_sparse_bo(0)
	, m_sparse_bo_size_rounded(0)
	, m_testCtx(testContext)
{
	/* Left blank intentionally */
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void ClearOpsBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_initial_data != DE_NULL)
	{
		delete[] m_initial_data;

		m_initial_data = DE_NULL;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void ClearOpsBufferStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool ClearOpsBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool			   result	 = true;
	const unsigned int data_rgba8 = 0x12345678;

	for (unsigned int n_clear_op_type = 0; n_clear_op_type < 2; /* glClearBufferData(), glClearBufferSubData() */
		 ++n_clear_op_type)
	{
		const bool use_clear_buffer_data_call = (n_clear_op_type == 0);

		/* We will run the test case in two iterations:
		 *
		 * 1) All pages will have a physical backing.
		 * 2) Half of the pages will have a physical backing.
		 */
		for (unsigned int n_iteration = 0; n_iteration < 2; ++n_iteration)
		{
			/* By default, for each iteration all sparse buffer pages are commited.
			 *
			 * For the last iteration, we need to de-commit the latter half before
			 * proceeding with the test.
			 */
			const bool all_pages_committed = (n_iteration == 0);

			if (!all_pages_committed)
			{
				m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

				m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, m_sparse_bo_size_rounded / 2, /* offset */
											 m_sparse_bo_size_rounded / 2,					/* size   */
											 GL_TRUE);										/* commit */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
			}

			/* Set up the sparse buffer contents */
			m_gl.bindBuffer(GL_ARRAY_BUFFER, m_helper_bo);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

			m_gl.bufferSubData(GL_ARRAY_BUFFER, 0, /* offset */
							   m_sparse_bo_size_rounded, m_initial_data);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferSubData() call failed.");

			m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
			m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_sparse_bo);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

			m_gl.copyBufferSubData(GL_COPY_READ_BUFFER,  /* readTarget  */
								   GL_COPY_WRITE_BUFFER, /* writeTarget */
								   0,					 /* readOffset */
								   0,					 /* writeOffset */
								   m_sparse_bo_size_rounded);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

			/* Issue the clear call */
			unsigned int clear_region_size		   = 0;
			unsigned int clear_region_start_offset = 0;

			if (use_clear_buffer_data_call)
			{
				DE_ASSERT((m_sparse_bo_size_rounded % sizeof(unsigned int)) == 0);

				clear_region_size		  = m_sparse_bo_size_rounded;
				clear_region_start_offset = 0;

				m_gl.clearBufferData(GL_COPY_WRITE_BUFFER, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, &data_rgba8);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearBufferData() call failed.");
			}
			else
			{
				DE_ASSERT(((m_sparse_bo_size_rounded / 2) % sizeof(unsigned int)) == 0);
				DE_ASSERT(((m_sparse_bo_size_rounded) % sizeof(unsigned int)) == 0);

				clear_region_size		  = m_sparse_bo_size_rounded / 2;
				clear_region_start_offset = m_sparse_bo_size_rounded / 2;

				m_gl.clearBufferSubData(GL_COPY_WRITE_BUFFER, GL_RGBA8, clear_region_start_offset, clear_region_size,
										GL_RGBA, GL_UNSIGNED_BYTE, &data_rgba8);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearBufferSubData() call failed.");
			}

			/* Retrieve the modified buffer's contents */
			const unsigned char* result_data = NULL;

			m_gl.copyBufferSubData(GL_COPY_WRITE_BUFFER, /* readTarget  */
								   GL_COPY_READ_BUFFER,  /* writeTarget */
								   0,					 /* readOffset  */
								   0,					 /* writeOffset */
								   m_sparse_bo_size_rounded);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

			result_data = (unsigned char*)m_gl.mapBufferRange(GL_COPY_READ_BUFFER, 0, /* offset */
															  m_sparse_bo_size_rounded, GL_MAP_READ_BIT);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBufferRange() call failed.");

			/* Verify the result data: unmodified region */
			bool			   result_local			  = true;
			const unsigned int unmodified_region_size = (use_clear_buffer_data_call) ? 0 : clear_region_start_offset;
			const unsigned int unmodified_region_start_offset = 0;

			for (unsigned int n_current_byte = unmodified_region_start_offset;
				 (n_current_byte < unmodified_region_start_offset + unmodified_region_size) && result_local;
				 ++n_current_byte)
			{
				const unsigned int  current_initial_data_offset = n_current_byte - unmodified_region_start_offset;
				const unsigned char expected_value				= m_initial_data[current_initial_data_offset];
				const unsigned char found_value					= result_data[n_current_byte];

				if (expected_value != found_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Unmodified buffer object region has invalid contents. Expected byte "
									   << "[" << (int)expected_value << "]"
																		", found byte:"
																		"["
									   << (int)found_value << "]"
															  " at index "
															  "["
									   << n_current_byte << "]; "
															"call type:"
															"["
									   << ((use_clear_buffer_data_call) ? "glClearBufferData()" :
																		  "glClearBufferSubData()")
									   << "]"
										  ", all required pages committed?:"
										  "["
									   << ((all_pages_committed) ? "yes" : "no") << "]" << tcu::TestLog::EndMessage;

					result_local = false;
					break;
				}
			}

			result &= result_local;
			result_local = true;

			/* Verify the result data: modified region (clamped to the memory region
			 * with actual physical backing) */
			const unsigned int modified_region_size			= (all_pages_committed) ? clear_region_size : 0;
			const unsigned int modified_region_start_offset = clear_region_start_offset;

			for (unsigned int n_current_byte = modified_region_start_offset;
				 (n_current_byte < modified_region_start_offset + modified_region_size) && result_local;
				 ++n_current_byte)
			{
				const unsigned char expected_value =
					static_cast<unsigned char>((data_rgba8 & (0xFF << (n_current_byte * 8))) >> (n_current_byte * 8));
				const unsigned char found_value = result_data[n_current_byte];

				if (expected_value != found_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Unmodified buffer object region has invalid contents. Expected byte "
									   << "[" << (int)expected_value << "]"
																		", found byte:"
																		"["
									   << (int)found_value << "]"
															  " at index "
															  "["
									   << n_current_byte << "]; "
															"call type:"
															"["
									   << ((use_clear_buffer_data_call) ? "glClearBufferData()" :
																		  "glClearBufferSubData()")
									   << "]"
										  ", all required pages committed?:"
										  "["
									   << ((all_pages_committed) ? "yes" : "no") << "]" << tcu::TestLog::EndMessage;

					result_local = false;
					break;
				}
			}

			result &= result_local;

			/* Unmap the storage before proceeding */
			m_gl.unmapBuffer(GL_COPY_READ_BUFFER);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");
		} /* for (both iterations) */
	}	 /* for (both clear types) */

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool ClearOpsBufferStorageTestCase::initTestCaseGlobal()
{
	unsigned int	   n_bytes_filled = 0;
	const unsigned int n_bytes_needed = m_n_pages_to_use * m_page_size;

	/* Determine the number of bytes both the helper and the sparse buffer
	 * object need to be able to hold, at maximum */
	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(n_bytes_needed, m_page_size);

	/* Set up the helper BO */
	DE_ASSERT(m_helper_bo == 0);

	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_COPY_READ_BUFFER, m_sparse_bo_size_rounded, DE_NULL,
					   GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT); /* flags */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	/* Set up a client-side data buffer we will use to fill the sparse BO with data,
	 * to be later cleared with the clear ops */
	DE_ASSERT(m_initial_data == DE_NULL);

	m_initial_data = new unsigned char[m_sparse_bo_size_rounded];

	while (n_bytes_filled < m_sparse_bo_size_rounded)
	{
		m_initial_data[n_bytes_filled] = static_cast<unsigned char>(n_bytes_filled % 256);

		++n_bytes_filled;
	}

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool ClearOpsBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	/* Set up the sparse bufffer. */
	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				 /* offset */
								 m_sparse_bo_size_rounded, GL_TRUE); /* commit */

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
CopyOpsBufferStorageTestCase::CopyOpsBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext,
														   glw::GLint page_size)
	: m_gl(gl)
	, m_helper_bo(0)
	, m_immutable_bo(0)
	, m_page_size(page_size)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
	, m_testCtx(testContext)
{
	m_ref_data[0]   = DE_NULL;
	m_ref_data[1]   = DE_NULL;
	m_ref_data[2]   = DE_NULL;
	m_sparse_bos[0] = 0;
	m_sparse_bos[1] = 0;
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */

void CopyOpsBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_immutable_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_immutable_bo);

		m_immutable_bo = 0;
	}

	for (unsigned int n_ref_data_buffer = 0; n_ref_data_buffer < sizeof(m_ref_data) / sizeof(m_ref_data[0]);
		 ++n_ref_data_buffer)
	{
		if (m_ref_data[n_ref_data_buffer] != DE_NULL)
		{
			delete[] m_ref_data[n_ref_data_buffer];

			m_ref_data[n_ref_data_buffer] = DE_NULL;
		}
	}

	/* Only release the test case-owned BO */
	if (m_sparse_bos[1] != 0)
	{
		m_gl.deleteBuffers(1, m_sparse_bos + 1);

		m_sparse_bos[1] = 0;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void CopyOpsBufferStorageTestCase::deinitTestCaseIteration()
{
	for (unsigned int n_sparse_bo = 0; n_sparse_bo < sizeof(m_sparse_bos) / sizeof(m_sparse_bos[0]); ++n_sparse_bo)
	{
		const glw::GLuint sparse_bo_id = m_sparse_bos[n_sparse_bo];

		if (sparse_bo_id != 0)
		{
			m_gl.bindBuffer(GL_ARRAY_BUFFER, sparse_bo_id);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

			m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
										 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
		} /* if (sparse_bo_id != 0) */
	}	 /* for (both BOs) */
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool CopyOpsBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool result = true;

	/* Iterate over all test cases */
	DE_ASSERT(m_immutable_bo != 0);
	DE_ASSERT(m_sparse_bos[0] != 0);
	DE_ASSERT(m_sparse_bos[1] != 0);

	for (_test_cases_const_iterator test_iterator = m_test_cases.begin(); test_iterator != m_test_cases.end();
		 ++test_iterator)
	{
		bool			  result_local = true;
		const _test_case& test_case	= *test_iterator;
		const glw::GLuint dst_bo_id =
			test_case.dst_bo_is_sparse ? m_sparse_bos[test_case.dst_bo_sparse_id] : m_immutable_bo;
		const glw::GLuint src_bo_id =
			test_case.src_bo_is_sparse ? m_sparse_bos[test_case.src_bo_sparse_id] : m_immutable_bo;

		/* Initialize immutable BO data (if used) */
		if (dst_bo_id == m_immutable_bo || src_bo_id == m_immutable_bo)
		{
			m_gl.bindBuffer(GL_ARRAY_BUFFER, m_immutable_bo);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

			m_gl.bufferSubData(GL_ARRAY_BUFFER, 0, /* offset */
							   m_sparse_bo_size_rounded, m_ref_data[0]);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferSubData() call failed.");
		}

		/* Initialize sparse BO data storage */
		for (unsigned int n_sparse_bo = 0; n_sparse_bo < sizeof(m_sparse_bos) / sizeof(m_sparse_bos[0]); ++n_sparse_bo)
		{
			const bool is_dst_bo = (dst_bo_id == m_sparse_bos[n_sparse_bo]);
			const bool is_src_bo = (src_bo_id == m_sparse_bos[n_sparse_bo]);

			if (!is_dst_bo && !is_src_bo)
				continue;

			m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
			m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_sparse_bos[n_sparse_bo]);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

			if (is_dst_bo)
			{
				m_gl.bufferPageCommitmentARB(GL_COPY_WRITE_BUFFER, test_case.dst_bo_commit_start_offset,
											 test_case.dst_bo_commit_size, GL_TRUE); /* commit */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
			}

			if (is_src_bo)
			{
				m_gl.bufferPageCommitmentARB(GL_COPY_WRITE_BUFFER, test_case.src_bo_commit_start_offset,
											 test_case.src_bo_commit_size, GL_TRUE); /* commit */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
			}

			m_gl.bufferSubData(GL_COPY_READ_BUFFER, 0, /* offset */
							   m_sparse_bo_size_rounded, m_ref_data[1 + n_sparse_bo]);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferSubData() call failed.");

			m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
								   0,											 /* writeOffset */
								   m_sparse_bo_size_rounded);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");
		} /* for (both sparse BOs) */

		/* Set up the bindings */
		m_gl.bindBuffer(GL_COPY_READ_BUFFER, src_bo_id);
		m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, dst_bo_id);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		/* Issue the copy op */
		m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, test_case.src_bo_start_offset,
							   test_case.dst_bo_start_offset, test_case.n_bytes_to_copy);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

		/* Retrieve the destination buffer's contents. The BO used for the previous copy op might have
		 * been a sparse BO, so copy its storage to a helper immutable BO */
		const unsigned short* dst_bo_data_ptr = NULL;

		m_gl.bindBuffer(GL_COPY_READ_BUFFER, dst_bo_id);
		m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_helper_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
							   0,											 /* writeOffset */
							   m_sparse_bo_size_rounded);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

		dst_bo_data_ptr = (const unsigned short*)m_gl.mapBufferRange(GL_COPY_WRITE_BUFFER, 0, /* offset */
																	 m_sparse_bo_size_rounded, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBufferRange() call failed.");

		/* Verify the retrieved data:
		 *
		 * 1. Check the bytes which precede the copy op dst offset. These should be equal to
		 *    the destination buffer's reference data within the committed memory region.
		 **/
		if (test_case.dst_bo_start_offset != 0 && test_case.dst_bo_commit_start_offset < test_case.dst_bo_start_offset)
		{
			DE_ASSERT(((test_case.dst_bo_start_offset - test_case.dst_bo_commit_start_offset) % sizeof(short)) == 0);

			const unsigned int n_valid_values = static_cast<unsigned int>(
				(test_case.dst_bo_start_offset - test_case.dst_bo_commit_start_offset) / sizeof(short));

			for (unsigned int n_value = 0; n_value < n_valid_values && result_local; ++n_value)
			{
				const int dst_data_offset = static_cast<int>(sizeof(short) * n_value);

				if (dst_data_offset >= test_case.dst_bo_commit_start_offset &&
					dst_data_offset < test_case.dst_bo_commit_start_offset + test_case.dst_bo_commit_size)
				{
					const unsigned short expected_short_value =
						*(unsigned short*)((unsigned char*)test_case.dst_bo_ref_data + dst_data_offset);
					const unsigned short found_short_value =
						*(unsigned short*)((unsigned char*)dst_bo_data_ptr + dst_data_offset);

					if (expected_short_value != found_short_value)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Malformed data found in the copy op's destination BO, "
														"preceding the region modified by the copy op. "
							<< "Destination BO id:" << dst_bo_id << " ("
							<< ((test_case.dst_bo_is_sparse) ? "sparse buffer)" : "immutable buffer)")
							<< ", commited region: " << test_case.dst_bo_commit_start_offset << ":"
							<< (test_case.dst_bo_commit_start_offset + test_case.dst_bo_commit_size)
							<< ", copy region: " << test_case.dst_bo_start_offset << ":"
							<< (test_case.dst_bo_start_offset + test_case.n_bytes_to_copy)
							<< ". Source BO id:" << src_bo_id << " ("
							<< ((test_case.src_bo_is_sparse) ? "sparse buffer)" : "immutable buffer)")
							<< ", commited region: " << test_case.src_bo_commit_start_offset << ":"
							<< (test_case.src_bo_commit_start_offset + test_case.src_bo_commit_size)
							<< ", copy region: " << test_case.src_bo_start_offset << ":"
							<< (test_case.src_bo_start_offset + test_case.n_bytes_to_copy) << ". Expected value of "
							<< expected_short_value << ", found value of " << found_short_value
							<< " at dst data offset of " << dst_data_offset << "." << tcu::TestLog::EndMessage;

						result_local = false;
					}
				}
			} /* for (all preceding values which should not have been affected by the copy op) */
		}	 /* if (copy op did not modify the beginning of the destination buffer storage) */

		/* 2. Check if the data written to the destination buffer object is correct. */
		for (unsigned int n_copied_short_value = 0;
			 n_copied_short_value < test_case.n_bytes_to_copy / sizeof(short) && result_local; ++n_copied_short_value)
		{
			const int src_data_offset =
				static_cast<unsigned int>(test_case.src_bo_start_offset + sizeof(short) * n_copied_short_value);
			const int dst_data_offset =
				static_cast<unsigned int>(test_case.dst_bo_start_offset + sizeof(short) * n_copied_short_value);

			if (dst_data_offset >= test_case.dst_bo_commit_start_offset &&
				dst_data_offset < test_case.dst_bo_commit_start_offset + test_case.dst_bo_commit_size &&
				src_data_offset >= test_case.src_bo_commit_start_offset &&
				src_data_offset < test_case.src_bo_commit_start_offset + test_case.src_bo_commit_size)
			{
				const unsigned short expected_short_value =
					*(unsigned short*)((unsigned char*)test_case.src_bo_ref_data + src_data_offset);
				const unsigned short found_short_value =
					*(unsigned short*)((unsigned char*)dst_bo_data_ptr + dst_data_offset);

				if (expected_short_value != found_short_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Malformed data found in the copy op's destination BO. "
									   << "Destination BO id:" << dst_bo_id << " ("
									   << ((test_case.dst_bo_is_sparse) ? "sparse buffer)" : "immutable buffer)")
									   << ", commited region: " << test_case.dst_bo_commit_start_offset << ":"
									   << (test_case.dst_bo_commit_start_offset + test_case.dst_bo_commit_size)
									   << ", copy region: " << test_case.dst_bo_start_offset << ":"
									   << (test_case.dst_bo_start_offset + test_case.n_bytes_to_copy)
									   << ". Source BO id:" << src_bo_id << " ("
									   << ((test_case.src_bo_is_sparse) ? "sparse buffer)" : "immutable buffer)")
									   << ", commited region: " << test_case.src_bo_commit_start_offset << ":"
									   << (test_case.src_bo_commit_start_offset + test_case.src_bo_commit_size)
									   << ", copy region: " << test_case.src_bo_start_offset << ":"
									   << (test_case.src_bo_start_offset + test_case.n_bytes_to_copy)
									   << ". Expected value of " << expected_short_value << ", found value of "
									   << found_short_value << " at dst data offset of " << dst_data_offset << "."
									   << tcu::TestLog::EndMessage;

					result_local = false;
				}
			}
		}

		/* 3. Verify the remaining data in the committed part of the destination buffer object is left intact. */
		const unsigned int commit_region_end_offset =
			test_case.dst_bo_commit_start_offset + test_case.dst_bo_commit_size;
		const unsigned int copy_region_end_offset = test_case.dst_bo_start_offset + test_case.n_bytes_to_copy;

		if (commit_region_end_offset > copy_region_end_offset)
		{
			DE_ASSERT(((commit_region_end_offset - copy_region_end_offset) % sizeof(short)) == 0);

			const unsigned int n_valid_values =
				static_cast<unsigned int>((commit_region_end_offset - copy_region_end_offset) / sizeof(short));

			for (unsigned int n_value = 0; n_value < n_valid_values && result_local; ++n_value)
			{
				const int dst_data_offset = static_cast<int>(copy_region_end_offset + sizeof(short) * n_value);

				if (dst_data_offset >= test_case.dst_bo_commit_start_offset &&
					dst_data_offset < test_case.dst_bo_commit_start_offset + test_case.dst_bo_commit_size)
				{
					const unsigned short expected_short_value =
						*(unsigned short*)((unsigned char*)test_case.dst_bo_ref_data + dst_data_offset);
					const unsigned short found_short_value =
						*(unsigned short*)((unsigned char*)dst_bo_data_ptr + dst_data_offset);

					if (expected_short_value != found_short_value)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message << "Malformed data found in the copy op's destination BO, "
														"following the region modified by the copy op. "
							<< "Destination BO id:" << dst_bo_id << " ("
							<< ((test_case.dst_bo_is_sparse) ? "sparse buffer)" : "immutable buffer)")
							<< ", commited region: " << test_case.dst_bo_commit_start_offset << ":"
							<< (test_case.dst_bo_commit_start_offset + test_case.dst_bo_commit_size)
							<< ", copy region: " << test_case.dst_bo_start_offset << ":"
							<< (test_case.dst_bo_start_offset + test_case.n_bytes_to_copy)
							<< ". Source BO id:" << src_bo_id << " ("
							<< ((test_case.src_bo_is_sparse) ? "sparse buffer)" : "immutable buffer)")
							<< ", commited region: " << test_case.src_bo_commit_start_offset << ":"
							<< (test_case.src_bo_commit_start_offset + test_case.src_bo_commit_size)
							<< ", copy region: " << test_case.src_bo_start_offset << ":"
							<< (test_case.src_bo_start_offset + test_case.n_bytes_to_copy) << ". Expected value of "
							<< expected_short_value << ", found value of " << found_short_value
							<< " at dst data offset of " << dst_data_offset << "." << tcu::TestLog::EndMessage;

						result_local = false;
					}
				}
			} /* for (all preceding values which should not have been affected by the copy op) */
		}	 /* if (copy op did not modify the beginning of the destination buffer storage) */

		/* Unmap the buffer storage */
		m_gl.unmapBuffer(GL_COPY_WRITE_BUFFER);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");

		/* Clean up */
		for (unsigned int n_sparse_bo = 0; n_sparse_bo < sizeof(m_sparse_bos) / sizeof(m_sparse_bos[0]); ++n_sparse_bo)
		{
			const bool is_dst_bo = (dst_bo_id == m_sparse_bos[n_sparse_bo]);
			const bool is_src_bo = (src_bo_id == m_sparse_bos[n_sparse_bo]);

			if (is_dst_bo || is_src_bo)
			{
				m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bos[n_sparse_bo]);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

				m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0, m_sparse_bo_size_rounded, GL_FALSE); /* commit */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
			}
		}

		result &= result_local;
	} /* for (all test cases) */

	return result;
}

/** Allocates reference buffers, fills them with data and updates the m_ref_data array. */
void CopyOpsBufferStorageTestCase::initReferenceData()
{
	DE_ASSERT(m_sparse_bo_size_rounded != 0);
	DE_ASSERT((m_sparse_bo_size_rounded % 2) == 0);
	DE_ASSERT(sizeof(short) == 2);

	for (unsigned int n_ref_data_buffer = 0; n_ref_data_buffer < sizeof(m_ref_data) / sizeof(m_ref_data[0]);
		 ++n_ref_data_buffer)
	{
		DE_ASSERT(m_ref_data[n_ref_data_buffer] == DE_NULL);

		m_ref_data[n_ref_data_buffer] = new unsigned short[m_sparse_bo_size_rounded / 2];

		/* Write reference values. */
		for (unsigned int n_short_value = 0; n_short_value < m_sparse_bo_size_rounded / 2; ++n_short_value)
		{
			m_ref_data[n_ref_data_buffer][n_short_value] =
				(unsigned short)((n_ref_data_buffer + 1) * (n_short_value + 1));
		}
	} /* for (all reference data buffers) */
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool CopyOpsBufferStorageTestCase::initTestCaseGlobal()
{
	m_sparse_bo_size		 = 2 * 3 * 4 * m_page_size;
	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_sparse_bo_size, m_page_size);

	initReferenceData();

	/* Initialize the sparse buffer object */
	m_gl.genBuffers(1, m_sparse_bos + 1);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bos[1]);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_ARRAY_BUFFER, m_sparse_bo_size_rounded, DE_NULL, /* data */
					   GL_SPARSE_STORAGE_BIT_ARB);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	/* Initialize the immutable buffer objects used by the test */
	for (unsigned int n_bo = 0; n_bo < 2; /* helper + immutable BO used for the copy ops */
		 ++n_bo)
	{
		glw::GLuint* bo_id_ptr = (n_bo == 0) ? &m_helper_bo : &m_immutable_bo;
		glw::GLenum  flags	 = GL_DYNAMIC_STORAGE_BIT;

		if (n_bo == 0)
		{
			flags |= GL_MAP_READ_BIT;
		}

		/* Initialize the immutable buffer object */
		m_gl.genBuffers(1, bo_id_ptr);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

		m_gl.bindBuffer(GL_ARRAY_BUFFER, *bo_id_ptr);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferStorage(GL_ARRAY_BUFFER, m_sparse_bo_size_rounded, m_ref_data[0], flags);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");
	}

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool CopyOpsBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Remember the BO id */
	m_sparse_bos[0] = sparse_bo;

	/* Initialize test cases, if this is the first call to initTestCaseIteration() */
	if (m_test_cases.size() == 0)
	{
		initTestCases();
	}

	/* Make sure all pages of the provided sparse BO are de-committed before
	 * ::execute() is called. */
	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bos[0]);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
								 m_sparse_bo_size_rounded, GL_FALSE); /* commit */

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

	return result;
}

/** Fills m_test_cases with test case descriptors. Each such descriptor defines
 *  a single copy op use case.
 *
 * The descriptors are then iterated over in ::execute(), defining the testing
 * behavior of the test copy ops buffer storage test case.
 */
void CopyOpsBufferStorageTestCase::initTestCases()
{
	/* We need to use the following destination & source BO configurations:
	 *
	 * Dst: sparse    BO 1;  Src: sparse    BO 2
	 * Dst: sparse    BO 1;  Src: immutable BO
	 * Dst: immutable BO;    Src: sparse    BO 1
	 * Dst: sparse    BO 1;  Src: sparse    BO 1
	 */
	unsigned int n_test_case = 0;

	for (unsigned int n_bo_configuration = 0; n_bo_configuration < 4; /* as per the comment */
		 ++n_bo_configuration, ++n_test_case)
	{
		glw::GLuint		dst_bo_sparse_id = 0;
		bool			dst_bo_is_sparse = false;
		unsigned short* dst_bo_ref_data  = DE_NULL;
		glw::GLuint		src_bo_sparse_id = 0;
		bool			src_bo_is_sparse = false;
		unsigned short* src_bo_ref_data  = DE_NULL;

		switch (n_bo_configuration)
		{
		case 0:
		{
			dst_bo_sparse_id = 0;
			dst_bo_is_sparse = true;
			dst_bo_ref_data  = m_ref_data[1];
			src_bo_sparse_id = 1;
			src_bo_is_sparse = true;
			src_bo_ref_data  = m_ref_data[2];

			break;
		}

		case 1:
		{
			dst_bo_sparse_id = 0;
			dst_bo_is_sparse = true;
			dst_bo_ref_data  = m_ref_data[1];
			src_bo_is_sparse = false;
			src_bo_ref_data  = m_ref_data[0];

			break;
		}

		case 2:
		{
			dst_bo_is_sparse = false;
			dst_bo_ref_data  = m_ref_data[0];
			src_bo_sparse_id = 0;
			src_bo_is_sparse = true;
			src_bo_ref_data  = m_ref_data[1];

			break;
		}

		case 3:
		{
			dst_bo_sparse_id = 0;
			dst_bo_is_sparse = true;
			dst_bo_ref_data  = m_ref_data[1];
			src_bo_sparse_id = 0;
			src_bo_is_sparse = true;
			src_bo_ref_data  = m_ref_data[1];

			break;
		}

		default:
		{
			TCU_FAIL("Invalid BO configuration index");
		}
		} /* switch (n_bo_configuration) */

		/* Need to test the copy operation in three different scenarios,
		 * in regard to the destination buffer:
		 *
		 * a) All pages of the destination region are committed.
		 * b) Half of the pages of the destination region are committed.
		 * c) None of the pages of the destination region are committed.
		 *
		 * Destination region spans from 0 to half of the memory we use
		 * for the testing purposes.
		 */
		DE_ASSERT((m_sparse_bo_size_rounded % m_page_size) == 0);
		DE_ASSERT((m_sparse_bo_size_rounded % 2) == 0);
		DE_ASSERT((m_sparse_bo_size_rounded % 4) == 0);

		for (unsigned int n_dst_region = 0; n_dst_region < 3; /* as per the comment */
			 ++n_dst_region)
		{
			glw::GLuint dst_bo_commit_size		   = 0;
			glw::GLuint dst_bo_commit_start_offset = 0;

			switch (n_dst_region)
			{
			case 0:
			{
				dst_bo_commit_start_offset = 0;
				dst_bo_commit_size		   = m_sparse_bo_size_rounded / 2;

				break;
			}

			case 1:
			{
				dst_bo_commit_start_offset = m_sparse_bo_size_rounded / 4;
				dst_bo_commit_size		   = m_sparse_bo_size_rounded / 4;

				break;
			}

			case 2:
			{
				dst_bo_commit_start_offset = 0;
				dst_bo_commit_size		   = 0;

				break;
			}

			default:
			{
				TCU_FAIL("Invalid destination region configuration index");
			}
			} /* switch (n_dst_region) */

			/* Same goes for the source region.
			 *
			 * Source region spans from m_sparse_bo_size_rounded / 2 to
			 * m_sparse_bo_size_rounded.
			 *
			 **/
			for (unsigned int n_src_region = 0; n_src_region < 3; /* as per the comment */
				 ++n_src_region)
			{
				glw::GLuint src_bo_commit_size		   = 0;
				glw::GLuint src_bo_commit_start_offset = 0;

				switch (n_src_region)
				{
				case 0:
				{
					src_bo_commit_start_offset = m_sparse_bo_size_rounded / 2;
					src_bo_commit_size		   = m_sparse_bo_size_rounded / 2;

					break;
				}

				case 1:
				{
					src_bo_commit_start_offset = 3 * m_sparse_bo_size_rounded / 4;
					src_bo_commit_size		   = m_sparse_bo_size_rounded / 4;

					break;
				}

				case 2:
				{
					src_bo_commit_start_offset = m_sparse_bo_size_rounded / 2;
					src_bo_commit_size		   = 0;

					break;
				}

				default:
				{
					TCU_FAIL("Invalid source region configuration index");
				}
				} /* switch (n_src_region) */

				/* Initialize the test case descriptor */
				_test_case test_case;

				test_case.dst_bo_commit_size		 = dst_bo_commit_size;
				test_case.dst_bo_commit_start_offset = dst_bo_commit_start_offset;
				test_case.dst_bo_sparse_id			 = dst_bo_sparse_id;
				test_case.dst_bo_is_sparse			 = dst_bo_is_sparse;
				test_case.dst_bo_ref_data			 = dst_bo_ref_data;
				test_case.dst_bo_start_offset		 = static_cast<glw::GLint>(sizeof(short) * n_test_case);
				test_case.n_bytes_to_copy			 = static_cast<glw::GLint>(
					m_sparse_bo_size_rounded / 2 - test_case.dst_bo_start_offset - sizeof(short) * n_test_case);
				test_case.src_bo_commit_size		 = src_bo_commit_size;
				test_case.src_bo_commit_start_offset = src_bo_commit_start_offset;
				test_case.src_bo_sparse_id			 = src_bo_sparse_id;
				test_case.src_bo_is_sparse			 = src_bo_is_sparse;
				test_case.src_bo_ref_data			 = src_bo_ref_data;
				test_case.src_bo_start_offset		 = m_sparse_bo_size_rounded / 2;

				DE_ASSERT(test_case.dst_bo_commit_size >= 0);
				DE_ASSERT(test_case.dst_bo_commit_start_offset >= 0);
				DE_ASSERT(test_case.dst_bo_ref_data != DE_NULL);
				DE_ASSERT(test_case.dst_bo_start_offset >= 0);
				DE_ASSERT(test_case.n_bytes_to_copy >= 0);
				DE_ASSERT(test_case.src_bo_commit_size >= 0);
				DE_ASSERT(test_case.src_bo_commit_start_offset >= 0);
				DE_ASSERT(test_case.src_bo_ref_data != DE_NULL);
				DE_ASSERT(test_case.src_bo_start_offset >= 0);

				m_test_cases.push_back(test_case);
			} /* for (all source region commit configurations) */
		}	 /* for (all destination region commit configurations) */
	}		  /* for (all BO configurations which need to be tested) */
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
IndirectDispatchBufferStorageTestCase::IndirectDispatchBufferStorageTestCase(const glw::Functions& gl,
																			 tcu::TestContext&	 testContext,
																			 glw::GLint			   page_size)
	: m_dispatch_draw_call_args_start_offset(-1)
	, m_expected_ac_value(0)
	, m_gl(gl)
	, m_global_wg_size_x(2048)
	, m_helper_bo(0)
	, m_local_wg_size_x(1023) /* must stay in sync with the local work-groups's size hardcoded in m_po's body! */
	, m_page_size(page_size)
	, m_po(0)
	, m_sparse_bo(0)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
	, m_testCtx(testContext)
{
	/* Left blank intentionally */
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void IndirectDispatchBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_po != 0)
	{
		m_gl.deleteProgram(m_po);

		m_po = 0;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void IndirectDispatchBufferStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool IndirectDispatchBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool result = true;

	/* Set up the buffer bindings */
	m_gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_helper_bo);
	m_gl.bindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed");

	m_gl.bindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, /* index */
						 m_helper_bo, 12,			  /* offset */
						 4);						  /* size */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferRange() call failed.");

	/* Bind the compute program */
	m_gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

	/* Zero out atomic counter value. */
	const unsigned int zero_ac_value = 0;

	m_gl.bufferSubData(GL_ATOMIC_COUNTER_BUFFER, 12, /* offset */
					   4,							 /* size */
					   &zero_ac_value);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferSubData() call failed.");

	m_expected_ac_value = zero_ac_value;

	/* Run the test only in a configuration where all arguments are local in
	 * committed memory page(s): reading arguments from uncommitted pages means
	 * reading undefined data, which can result in huge dispatches that
	 * effectively hang the test.
	 */
	m_gl.bufferPageCommitmentARB(GL_DISPATCH_INDIRECT_BUFFER, 0,	 /* offset */
								 m_sparse_bo_size_rounded, GL_TRUE); /* commit */

	m_expected_ac_value += m_global_wg_size_x * m_local_wg_size_x;

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call(s) failed.");

	/* Copy the indirect dispatch call args data from the helper BO to the sparse BO */
	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
						   m_dispatch_draw_call_args_start_offset, sizeof(unsigned int) * 3);

	/* Run the program */
	m_gl.dispatchComputeIndirect(m_dispatch_draw_call_args_start_offset);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDispatchComputeIndirect() call failed.");

	/* Extract the AC value and verify it */
	const unsigned int* ac_data_ptr =
		(const unsigned int*)m_gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 12, /* offset */
												 4,							   /* length */
												 GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBufferRange() call failed.");

	if (*ac_data_ptr != m_expected_ac_value && result)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid atomic counter value encountered. "
													   "Expected value: ["
						   << m_expected_ac_value << "]"
													 ", found:"
													 "["
						   << *ac_data_ptr << "]." << tcu::TestLog::EndMessage;

		result = false;
	}

	/* Unmap the buffer before we move on with the next iteration */
	m_gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool IndirectDispatchBufferStorageTestCase::initTestCaseGlobal()
{
	bool result = true;

	/* One of the cases the test case implementation needs to support is the scenario
	 * where the indirect call arguments are located on the boundary of two (or more) memory pages,
	 * and some of the pages are not committed.
	 *
	 * There are two scenarios which can happen:
	 *
	 * a) page size >= sizeof(uint) * 3: Allocate two pages, arg start offset: (page_size - 4) aligned to 4.
	 *                                   The alignment is a must, since we'll be feeding the offset to an indirect dispatch call.
	 * b) page size <  sizeof(uint) * 3: Allocate as many pages as needed, disable some of the pages.
	 *
	 * For code clarity, the two cases are handled by separate branches, although they could be easily
	 * merged.
	 */
	const int n_indirect_dispatch_call_arg_bytes = sizeof(unsigned int) * 3;

	if (m_page_size >= n_indirect_dispatch_call_arg_bytes)
	{
		/* Indirect dispatch call args must be aligned to 4 */
		DE_ASSERT(m_page_size >= 4);

		m_dispatch_draw_call_args_start_offset = SparseBufferTestUtilities::alignOffset(m_page_size - 4, 4);
		m_sparse_bo_size = m_dispatch_draw_call_args_start_offset + n_indirect_dispatch_call_arg_bytes;
	}
	else
	{
		m_dispatch_draw_call_args_start_offset = 0;
		m_sparse_bo_size					   = n_indirect_dispatch_call_arg_bytes;
	}

	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_sparse_bo_size, m_page_size);

	/* Set up the helper buffer object. Its structure is as follows:
	 *
	 * [ 0-11]: Indirect dispatch call args
	 * [12-15]: Atomic counter value storage
	 */
	unsigned int	   helper_bo_data[4] = { 0 };
	const unsigned int n_helper_bo_bytes = sizeof(helper_bo_data);

	helper_bo_data[0] = m_global_wg_size_x; /* num_groups_x */
	helper_bo_data[1] = 1;					/* num_groups_y */
	helper_bo_data[2] = 1;					/* num_groups_z */
	helper_bo_data[3] = 0;					/* default atomic counter value */

	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferData(GL_ARRAY_BUFFER, n_helper_bo_bytes, helper_bo_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferData() call failed.");

	/* Set up the test compute program object */
	static const char* cs_body = "#version 430 core\n"
								 "\n"
								 "layout(local_size_x = 1023)          in;\n"
								 "layout(binding      = 0, offset = 0) uniform atomic_uint ac;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    atomicCounterIncrement(ac);\n"
								 "}\n";

	m_po = SparseBufferTestUtilities::createComputeProgram(m_gl, &cs_body, 1); /* n_cs_body_parts */

	result = (m_po != 0);

	return result;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool IndirectDispatchBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	/* Set up the sparse bufffer. */
	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				 /* offset */
								 m_sparse_bo_size_rounded, GL_TRUE); /* commit */

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
InvalidateBufferStorageTestCase::InvalidateBufferStorageTestCase(const glw::Functions& gl,
																 tcu::TestContext& testContext, glw::GLint page_size)
	: m_gl(gl)
	, m_n_pages_to_use(4)
	, m_page_size(page_size)
	, m_sparse_bo(0)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
{
	(void)testContext;
	DE_ASSERT((m_n_pages_to_use % 2) == 0);
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void InvalidateBufferStorageTestCase::deinitTestCaseGlobal()
{
	/* Stub */
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void InvalidateBufferStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool InvalidateBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool result = true;

	/* Since we cannot really perform any validation related to whether buffer
	 * storage invalidation works corectly, all this test can really do is to verify
	 * if the implementation does not crash when both entry-points are used against
	 * a sparse buffer object.
	 */
	for (unsigned int n_entry_point = 0; n_entry_point < 2; /* glInvalidateBuffer(), glInvalidateBufferSubData() */
		 ++n_entry_point)
	{
		const bool should_test_invalidate_buffer = (n_entry_point == 0);

		/* For glInvalidateBufferSubData(), we need to test two different ranges. */
		for (int n_iteration = 0; n_iteration < ((should_test_invalidate_buffer) ? 1 : 2); ++n_iteration)
		{
			if (should_test_invalidate_buffer)
			{
				m_gl.invalidateBufferData(m_sparse_bo);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glInvalidateBufferData() call failed.");
			}
			else
			{
				m_gl.invalidateBufferSubData(m_sparse_bo, 0, /* offset */
											 m_sparse_bo_size_rounded * ((n_iteration == 0) ? 1 : 2));
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glInvalidateBufferSubData() call failed.");
			}
		} /* for (all iterations) */
	}	 /* for (both entry-points) */

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool InvalidateBufferStorageTestCase::initTestCaseGlobal()
{
	const unsigned int n_bytes_needed = m_n_pages_to_use * m_page_size;

	/* Determine the number of bytes both the helper and the sparse buffer
	 * object need to be able to hold, at maximum */
	m_sparse_bo_size		 = n_bytes_needed;
	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(n_bytes_needed, m_page_size);

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool InvalidateBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	/* Set up the sparse bufffer. */
	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				 /* offset */
								 m_sparse_bo_size_rounded, GL_TRUE); /* commit */

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
PixelPackBufferStorageTestCase::PixelPackBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext,
															   glw::GLint page_size)
	: m_color_rb(0)
	, m_color_rb_height(1024)
	, m_color_rb_width(1024)
	, m_fbo(0)
	, m_gl(gl)
	, m_helper_bo(0)
	, m_page_size(page_size)
	, m_po(0)
	, m_ref_data_ptr(DE_NULL)
	, m_ref_data_size(0)
	, m_sparse_bo(0)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
	, m_testCtx(testContext)
	, m_vao(0)
{
	m_ref_data_size = m_color_rb_width * m_color_rb_height * 4; /* rgba */
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void PixelPackBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_color_rb != 0)
	{
		m_gl.deleteRenderbuffers(1, &m_color_rb);

		m_color_rb = 0;
	}

	if (m_fbo != 0)
	{
		m_gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_ref_data_ptr != DE_NULL)
	{
		delete[] m_ref_data_ptr;

		m_ref_data_ptr = DE_NULL;
	}

	if (m_po != 0)
	{
		m_gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_vao != 0)
	{
		m_gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void PixelPackBufferStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool PixelPackBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool result = true;

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	m_gl.bindBuffer(GL_PIXEL_PACK_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

	/* Run three separate iterations:
	 *
	 * a) All pages that are going to hold the texture data are committed.
	 * b) Use a zig-zag memory page commitment layout patern.
	 * b) No pages are committed.
	 */
	for (unsigned int n_iteration = 0; n_iteration < 3; ++n_iteration)
	{
		bool result_local = true;

		/* Set up the memory page commitment & the storage contents*/
		switch (n_iteration)
		{
		case 0:
		{
			m_gl.bufferPageCommitmentARB(GL_PIXEL_PACK_BUFFER, 0,			 /* offset */
										 m_sparse_bo_size_rounded, GL_TRUE); /* commit */
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

			break;
		}

		case 1:
		{
			const unsigned int n_pages = 1 + m_ref_data_size / m_page_size;

			DE_ASSERT((m_ref_data_size % m_page_size) == 0);

			for (unsigned int n_page = 0; n_page < n_pages; ++n_page)
			{
				const bool should_commit = ((n_page % 2) == 0);

				m_gl.bufferPageCommitmentARB(GL_PIXEL_PACK_BUFFER, m_page_size * n_page, m_page_size,
											 should_commit ? GL_TRUE : GL_FALSE);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
			} /* for (all relevant memory pages) */

			break;
		}

		case 2:
		{
			/* Do nothing - all pages already de-committed  */
			break;
		}

		default:
		{
			TCU_FAIL("Invalid iteration index");
		}
		} /* switch (n_iteration) */

		/* Draw full screen quad to generate the black-to-white gradient */
		const unsigned char* read_data_ptr = NULL;

		m_gl.useProgram(m_po);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

		m_gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays() call failed.");

		/* Read a framebuffer pixel data */
		m_gl.readPixels(0,																	/* x */
						0,																	/* y */
						m_color_rb_width, m_color_rb_height, GL_RGBA, GL_UNSIGNED_BYTE, 0); /* pixels */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glReadPixels() call failed.");

		m_gl.copyBufferSubData(GL_PIXEL_PACK_BUFFER, GL_COPY_READ_BUFFER, 0, /* readOffset */
							   0,											 /* writeOffset */
							   m_ref_data_size);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

		read_data_ptr = (unsigned char*)m_gl.mapBufferRange(GL_COPY_READ_BUFFER, 0, /* offset */
															m_ref_data_size, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBufferRange() call failed.");

		/* Verify the data */
		unsigned int		 n_current_tex_data_byte	  = 0;
		const unsigned char* read_data_traveller_ptr	  = (const unsigned char*)read_data_ptr;
		const unsigned char* reference_data_traveller_ptr = (const unsigned char*)m_ref_data_ptr;

		for (unsigned int y = 0; y < m_color_rb_height && result_local; ++y)
		{
			for (unsigned int x = 0; x < m_color_rb_width && result_local; ++x)
			{
				for (unsigned int n_component = 0; n_component < 4 /* rgba */ && result_local; ++n_component)
				{
					unsigned char expected_value		 = 0;
					bool		  is_from_committed_page = true;

					if (n_iteration == 1) /* zig-zag */
					{
						is_from_committed_page = ((n_current_tex_data_byte / m_page_size) % 2) == 0;
					}
					else if (n_iteration == 2) /* no pages committed */
					{
						is_from_committed_page = false;
					}

					if (is_from_committed_page)
					{
						expected_value = *reference_data_traveller_ptr;
					}

					if (is_from_committed_page && de::abs(expected_value - *read_data_traveller_ptr) > 1)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid texel data (channel:" << n_component
										   << ")"
											  " found at X:"
										   << x << ", "
												   "Y:"
										   << y << ")."
												   " Expected value:"
										   << expected_value << ","
																" found value:"
										   << *reference_data_traveller_ptr << tcu::TestLog::EndMessage;

						result_local = false;
					}

					n_current_tex_data_byte++;
					read_data_traveller_ptr++;
					reference_data_traveller_ptr++;
				} /* for (all components) */
			}	 /* for (all columns) */
		}		  /* for (all rows) */

		m_gl.unmapBuffer(GL_COPY_READ_BUFFER);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");

		read_data_ptr = DE_NULL;
		result &= result_local;

		/* Clean up */
		m_gl.bufferPageCommitmentARB(GL_PIXEL_PACK_BUFFER, 0,			  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
	} /* for (three iterations) */

	m_gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool PixelPackBufferStorageTestCase::initTestCaseGlobal()
{
	/* Determine dummy vertex shader and fragment shader that will generate black-to-white gradient. */
	const char* gradient_fs_code = "#version 330 core\n"
								   "\n"
								   "out vec4 result;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    float c = 1.0 - (gl_FragCoord.y - 0.5) / 1023.0;\n"
								   "    result  = vec4(c);\n"
								   "}\n";

	const char* gradient_vs_code = "#version 330\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    switch (gl_VertexID)\n"
								   "    {\n"
								   "        case 0: gl_Position = vec4(-1.0, -1.0, 0.0, 1.0); break;\n"
								   "        case 1: gl_Position = vec4( 1.0, -1.0, 0.0, 1.0); break;\n"
								   "        case 2: gl_Position = vec4(-1.0,  1.0, 0.0, 1.0); break;\n"
								   "        case 3: gl_Position = vec4( 1.0,  1.0, 0.0, 1.0); break;\n"
								   "    }\n"
								   "}\n";

	m_po = SparseBufferTestUtilities::createProgram(m_gl, &gradient_fs_code, 1, /* n_fs_body_parts */
													&gradient_vs_code, 1,		/* n_vs_body_parts*/
													NULL,						/* attribute_names */
													NULL,						/* attribute_locations */
													GL_NONE,					/* attribute_properties */
													0,							/* tf_varyings */
													0,							/* n_tf_varyings */
													0);							/* tf_varying_mode */
	if (m_po == 0)
	{
		TCU_FAIL("Failed to link the test program");
	}

	/* Generate and bind VAO */
	m_gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenVertexArrays() call failed.");

	m_gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindVertexArray() call failed.");

	/* Generate and bind FBO */
	m_gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenFramebuffers() call failed.");

	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindFramebuffer() call failed.");

	m_gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glReadBuffer() call failed.");

	/* Generate and bind RBO and attach it to FBO as a color attachment */
	m_gl.genRenderbuffers(1, &m_color_rb);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenRenderbuffers() call failed.");

	m_gl.bindRenderbuffer(GL_RENDERBUFFER, m_color_rb);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindRenderbuffer() call failed.");

	m_gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, m_color_rb_width, m_color_rb_height);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glRenderbufferStorage() call failed.");

	m_gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_color_rb);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glFramebufferRenderbuffer() call failed.");

	if (m_gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw tcu::NotSupportedError("Cannot execute the test - driver does not support rendering"
									 "to a GL_RGBA8 renderbuffer-based color attachment");
	}

	m_gl.viewport(0, /* x */
				  0, /* y */
				  m_color_rb_width, m_color_rb_height);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glViewport() call failed.");

	/* Determine what sparse buffer storage size we are going to need*/
	m_sparse_bo_size		 = m_ref_data_size;
	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_sparse_bo_size, m_page_size);

	/* Prepare the texture data */
	unsigned char* ref_data_traveller_ptr = DE_NULL;

	m_ref_data_ptr		   = new unsigned char[m_ref_data_size];
	ref_data_traveller_ptr = m_ref_data_ptr;

	for (unsigned int y = 0; y < m_color_rb_height; ++y)
	{
		const unsigned char color = (unsigned char)((1.0f - float(y) / float(m_color_rb_height - 1)) * 255.0f);

		for (unsigned int x = 0; x < m_color_rb_width; ++x)
		{
			memset(ref_data_traveller_ptr, color, 4); /* rgba */

			ref_data_traveller_ptr += 4; /* rgba */
		}								 /* for (all columns) */
	}									 /* for (all rows) */

	/* Set up the helper buffer object. */
	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_COPY_READ_BUFFER, m_ref_data_size, m_ref_data_ptr, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool PixelPackBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
PixelUnpackBufferStorageTestCase::PixelUnpackBufferStorageTestCase(const glw::Functions& gl,
																   tcu::TestContext& testContext, glw::GLint page_size)
	: m_gl(gl)
	, m_helper_bo(0)
	, m_page_size(page_size)
	, m_read_data_ptr(DE_NULL)
	, m_sparse_bo(0)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
	, m_testCtx(testContext)
	, m_texture_data_ptr(DE_NULL)
	, m_texture_data_size(0)
	, m_to(0)
	, m_to_data_zero(DE_NULL)
	, m_to_height(1024)
	, m_to_width(1024)
{
	m_texture_data_size = m_to_width * m_to_height * 4; /* rgba */
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void PixelUnpackBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_read_data_ptr != DE_NULL)
	{
		delete[] m_read_data_ptr;

		m_read_data_ptr = DE_NULL;
	}

	if (m_texture_data_ptr != DE_NULL)
	{
		delete[] m_texture_data_ptr;

		m_texture_data_ptr = DE_NULL;
	}

	if (m_to != 0)
	{
		m_gl.deleteTextures(1, &m_to);

		m_to = 0;
	}

	if (m_to_data_zero != DE_NULL)
	{
		delete[] m_to_data_zero;

		m_to_data_zero = DE_NULL;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void PixelUnpackBufferStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool PixelUnpackBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool result = true;

	m_gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bindTexture(GL_TEXTURE_2D, m_to);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindTexture() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindTexture() call failed.");

	/* Run three separate iterations:
	 *
	 * a) All pages holding the source texture data are committed.
	 * b) Use a zig-zag memory page commitment layout patern.
	 * b) No pages are committed.
	 */
	for (unsigned int n_iteration = 0; n_iteration < 3; ++n_iteration)
	{
		bool result_local = true;

		/* Set up the memory page commitment & the storage contents*/
		switch (n_iteration)
		{
		case 0:
		{
			m_gl.bufferPageCommitmentARB(GL_PIXEL_UNPACK_BUFFER, 0,			 /* offset */
										 m_sparse_bo_size_rounded, GL_TRUE); /* commit */
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

			m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_PIXEL_UNPACK_BUFFER, 0, /* readOffset */
								   0,											   /* writeOffset */
								   m_texture_data_size);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

			break;
		}

		case 1:
		{
			const unsigned int n_pages = m_texture_data_size / m_page_size;

			for (unsigned int n_page = 0; n_page < n_pages; ++n_page)
			{
				const bool should_commit = ((n_page % 2) == 0);

				m_gl.bufferPageCommitmentARB(GL_PIXEL_UNPACK_BUFFER, m_page_size * n_page, m_page_size,
											 should_commit ? GL_TRUE : GL_FALSE);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

				if (should_commit)
				{
					m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_PIXEL_UNPACK_BUFFER,
										   m_page_size * n_page, /* readOffset */
										   m_page_size * n_page, /* writeOffset */
										   m_page_size);
					GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");
				}
			} /* for (all relevant memory pages) */

			break;
		}

		case 2:
		{
			/* Do nothing */
			break;
		}

		default:
		{
			TCU_FAIL("Invalid iteration index");
		}
		} /* switch (n_iteration) */

		/* Clean up the base mip-map's contents before we proceeding with updating it
		 * with data downloaded from the BO, in order to avoid situation where silently
		 * failing glTexSubImage2D() calls slip past unnoticed */
		m_gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.texSubImage2D(GL_TEXTURE_2D, 0, /* level */
						   0,				 /* xoffset */
						   0,				 /* yoffset */
						   m_to_width, m_to_height, GL_RGBA, GL_UNSIGNED_BYTE, m_to_data_zero);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glTexSubImage2D() call failed.");

		m_gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		/* Update the base mip-map's contents */
		m_gl.texSubImage2D(GL_TEXTURE_2D, 0, /* level */
						   0,				 /* xoffset */
						   0,				 /* yoffset */
						   m_to_width, m_to_height, GL_RGBA, GL_UNSIGNED_BYTE, (const glw::GLvoid*)0);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glTexSubImage2D() call failed.");

		/* Read back the stored mip-map data */
		memset(m_read_data_ptr, 0xFF, m_texture_data_size);

		m_gl.getTexImage(GL_TEXTURE_2D, 0, /* level */
						 GL_RGBA, GL_UNSIGNED_BYTE, m_read_data_ptr);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetTexImage() call failed.");

		/* Verify the data */
		unsigned int n_current_tex_data_byte	= 0;
		const char*  read_data_traveller_ptr	= (const char*)m_read_data_ptr;
		const char*  texture_data_traveller_ptr = (const char*)m_texture_data_ptr;

		for (unsigned int y = 0; y < m_to_height && result_local; ++y)
		{
			for (unsigned int x = 0; x < m_to_width && result_local; ++x)
			{
				for (unsigned int n_component = 0; n_component < 4 /* rgba */ && result_local; ++n_component)
				{
					char expected_value			= 0;
					bool is_from_committed_page = true;

					if (n_iteration == 1) /* zig-zag */
					{
						is_from_committed_page = ((n_current_tex_data_byte / m_page_size) % 2) == 0;
					}
					else if (n_iteration == 2) /* no pages committed */
					{
						is_from_committed_page = false;
					}

					if (is_from_committed_page)
					{
						expected_value = *texture_data_traveller_ptr;
					}

					if ((is_from_committed_page && de::abs(expected_value - *read_data_traveller_ptr) >= 1) ||
						(!is_from_committed_page && *read_data_traveller_ptr != expected_value))
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid texel data (channel:" << n_component
										   << ")"
											  " found at X:"
										   << x << ", "
												   "Y:"
										   << y << ")."
												   " Expected value:"
										   << expected_value << ","
																" found value:"
										   << *texture_data_traveller_ptr << tcu::TestLog::EndMessage;

						result_local = false;
					}

					n_current_tex_data_byte++;
					read_data_traveller_ptr++;
					texture_data_traveller_ptr++;
				} /* for (all components) */
			}	 /* for (all columns) */
		}		  /* for (all rows) */

		result &= result_local;

		/* Clean up */
		m_gl.bufferPageCommitmentARB(GL_PIXEL_UNPACK_BUFFER, 0,			  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
	} /* for (three iterations) */

	m_gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool PixelUnpackBufferStorageTestCase::initTestCaseGlobal()
{
	/* Determine sparse buffer storage size */
	m_sparse_bo_size		 = m_texture_data_size;
	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_sparse_bo_size, m_page_size);

	/* Prepare the texture data */
	unsigned char* texture_data_traveller_ptr = DE_NULL;

	m_read_data_ptr			   = new unsigned char[m_texture_data_size];
	m_texture_data_ptr		   = new unsigned char[m_texture_data_size];
	texture_data_traveller_ptr = m_texture_data_ptr;

	for (unsigned int y = 0; y < m_to_height; ++y)
	{
		for (unsigned int x = 0; x < m_to_width; ++x)
		{
			const unsigned char color = (unsigned char)(float(x) / float(m_to_width - 1) * 255.0f);

			memset(texture_data_traveller_ptr, color, 4); /* rgba */

			texture_data_traveller_ptr += 4; /* rgba */
		}									 /* for (all columns) */
	}										 /* for (all rows) */

	m_to_data_zero = new unsigned char[m_texture_data_size];

	memset(m_to_data_zero, 0, m_texture_data_size);

	/* Set up the helper buffer object */
	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_COPY_READ_BUFFER, m_texture_data_size, m_texture_data_ptr, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	/* Set up texture object storage */
	m_gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenTextures() call failed.");

	m_gl.bindTexture(GL_TEXTURE_2D, m_to);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindTexture() call failed.");

	m_gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					  GL_RGBA8, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glTexStorage2D() call failed.");

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool PixelUnpackBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	/* Set up the sparse buffer. */
	m_gl.bindBuffer(GL_QUERY_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param ibo_usage                  Specifies if an indexed draw call should be used by the test. For more details,
 *                                    please see documentation for _ibo_usage.
 *  @param use_color_data             true to use the color data for the tested draw call;
 *                                    false to omit usage of attribute data.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
QuadsBufferStorageTestCase::QuadsBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext,
													   glw::GLint page_size, _ibo_usage ibo_usage, bool use_color_data)
	: m_attribute_color_location(0)	/* predefined attribute locations */
	, m_attribute_position_location(1) /* predefined attribute locations */
	, m_color_data_offset(0)
	, m_data(DE_NULL)
	, m_data_size(0)
	, m_data_size_rounded(0)
	, m_fbo(0)
	, m_gl(gl)
	, m_helper_bo(0)
	, m_ibo_data_offset(-1)
	, m_ibo_usage(ibo_usage)
	, m_n_quad_delta_x(5)
	, m_n_quad_delta_y(5)
	, m_n_quad_height(5)
	, m_n_quad_width(5)
	, m_n_quads_x(100) /* as per spec */
	, m_n_quads_y(100) /* as per spec */
	, m_n_vertices_to_draw(0)
	, m_pages_committed(false)
	, m_po(0)
	, m_sparse_bo(0)
	, m_testCtx(testContext)
	, m_to(0)
	, m_to_height(1024) /* as per spec */
	, m_to_width(1024)  /* as per spec */
	, m_use_color_data(use_color_data)
	, m_vao(0)
	, m_vbo_data_offset(-1)
{
	/*
	 * Each quad = 2 triangles, 1 triangle = 3 vertices, 1 vertex = 4 components.
	 * The inefficient representation has been used on purpose - we want the data to take
	 * more than 64KB so that it is guaranteed that it will span over more than 1 page.
	 */
	m_data_size = 0;

	m_n_vertices_to_draw = m_n_quads_x * /* quads in X */
						   m_n_quads_y * /* quads in Y */
						   2 *			 /* triangles */
						   3;			 /* vertices per triangle */

	m_data_size = static_cast<glw::GLuint>(m_n_vertices_to_draw * 4 /* components */ * sizeof(float));

	if (m_ibo_usage != IBO_USAGE_NONE)
	{
		DE_ASSERT(m_n_vertices_to_draw < 65536);

		m_data_size = static_cast<glw::GLuint>(m_data_size + (m_n_vertices_to_draw * sizeof(unsigned short)));
	}

	if (m_use_color_data)
	{
		m_data_size = static_cast<glw::GLuint>(m_data_size +
											   (m_n_vertices_to_draw * sizeof(unsigned char) * 4 * /* rgba components */
												2 *												   /* triangles */
												3)); /* vertices per triangle */
	}

	m_data_size_rounded = SparseBufferTestUtilities::alignOffset(m_data_size, page_size);
}

/** Allocates a data buffer and fills it with vertex/index/color data. Vertex data is always stored,
 *  index data only if m_ibo_usage is different from IBO_USAGE_NONE. Color data is only saved if
 *  m_use_color_data is true.
 *
 *  @param out_data              Deref will be used to store a pointer to the allocated data buffer.
 *                               Ownership is transferred to the caller. Must not be NULL.
 *  @param out_vbo_data_offset   Deref will be used to store an offset, from which VBO data starts,
 *                               relative to the beginning of *out_data. Must not be NULL.
 *  @param out_ibo_data_offset   Deref will be used to store an offset, from which IBO data starts,
 *                               relative to the beginning of *out_data. May be NULL if m_ibo_usage
 *                               is IBO_USAGE_NONE.
 *  @param out_color_data_offset Deref will be used to store na offset, from which color data starts,
 *                               relative to the beginning of *out_data. May be NULL if m_use_color_data
 *                               is false.
 *
 */
void QuadsBufferStorageTestCase::createTestData(unsigned char** out_data, unsigned int* out_vbo_data_offset,
												unsigned int* out_ibo_data_offset,
												unsigned int* out_color_data_offset) const
{
	unsigned char* data_traveller_ptr = NULL;

	*out_data			 = new unsigned char[m_data_size];
	*out_vbo_data_offset = 0;

	data_traveller_ptr = *out_data;

	for (unsigned int n_quad_y = 0; n_quad_y < m_n_quads_y; ++n_quad_y)
	{
		for (unsigned int n_quad_x = 0; n_quad_x < m_n_quads_x; ++n_quad_x)
		{
			const unsigned int quad_start_x_px = n_quad_x * (m_n_quad_delta_x + m_n_quad_width);
			const unsigned int quad_start_y_px = n_quad_y * (m_n_quad_delta_y + m_n_quad_height);
			const unsigned int quad_end_x_px   = quad_start_x_px + m_n_quad_width;
			const unsigned int quad_end_y_px   = quad_start_y_px + m_n_quad_height;

			const float quad_end_x_ss   = float(quad_end_x_px) / float(m_to_width) * 2.0f - 1.0f;
			const float quad_end_y_ss   = float(quad_end_y_px) / float(m_to_height) * 2.0f - 1.0f;
			const float quad_start_x_ss = float(quad_start_x_px) / float(m_to_width) * 2.0f - 1.0f;
			const float quad_start_y_ss = float(quad_start_y_px) / float(m_to_height) * 2.0f - 1.0f;

			/*  1,4--5
			 *  |\   |
			 *  | \  |
			 *  2----3,6
			 */
			const float v1_4[] = {
				quad_start_x_ss, quad_start_y_ss, 0.0f, /* z */
				1.0f,									/* w */
			};
			const float v2[] = {
				quad_start_x_ss, quad_end_y_ss, 0.0f, /* z */
				1.0f								  /* w */
			};
			const float v3_6[] = {
				quad_end_x_ss, quad_end_y_ss, 0.0f, /* z */
				1.0f								/* w */
			};
			const float v5[] = {
				quad_end_x_ss, quad_start_y_ss, 0.0f, /* z */
				1.0f								  /* w */
			};

			memcpy(data_traveller_ptr, v1_4, sizeof(v1_4));
			data_traveller_ptr += sizeof(v1_4);

			memcpy(data_traveller_ptr, v2, sizeof(v2));
			data_traveller_ptr += sizeof(v2);

			memcpy(data_traveller_ptr, v3_6, sizeof(v3_6));
			data_traveller_ptr += sizeof(v3_6);

			memcpy(data_traveller_ptr, v1_4, sizeof(v1_4));
			data_traveller_ptr += sizeof(v1_4);

			memcpy(data_traveller_ptr, v5, sizeof(v5));
			data_traveller_ptr += sizeof(v5);

			memcpy(data_traveller_ptr, v3_6, sizeof(v3_6));
			data_traveller_ptr += sizeof(v3_6);
		} /* for (all quads in X) */
	}	 /* for (all quads in Y) */

	/* Set up index data if needed */
	if (m_ibo_usage != IBO_USAGE_NONE)
	{
		*out_ibo_data_offset = static_cast<unsigned int>(data_traveller_ptr - *out_data);

		for (int index = m_n_vertices_to_draw - 1; index >= 0; --index)
		{
			*(unsigned short*)data_traveller_ptr = (unsigned short)index;
			data_traveller_ptr += sizeof(unsigned short);
		} /* for (all index values) */
	}	 /* if (m_use_ibo) */
	else
	{
		*out_ibo_data_offset = 0;
	}

	/* Set up color data if needed */
	if (m_use_color_data)
	{
		*out_color_data_offset = static_cast<unsigned int>(data_traveller_ptr - *out_data);

		for (unsigned int n_quad = 0; n_quad < m_n_quads_x * m_n_quads_y; ++n_quad)
		{
			/* Use magic formulas to generate a color data set for the quads. The data
			 * needs to be duplicated for 6 vertices forming a single quad. */
			for (unsigned int n_vertex = 0; n_vertex < 6; ++n_vertex)
			{
				/* Red */
				*data_traveller_ptr = static_cast<unsigned char>(n_quad % 256); //((n_quad + 15) * 14) % 256;
				data_traveller_ptr++;

				/* Green */
				*data_traveller_ptr = static_cast<unsigned char>(((n_quad + 32) * 7) % 255);
				data_traveller_ptr++;

				/* Blue */
				*data_traveller_ptr = static_cast<unsigned char>(((n_quad + 7) * 53) % 255);
				data_traveller_ptr++;

				/* Alpha */
				*data_traveller_ptr = static_cast<unsigned char>(((n_quad + 13) * 3) % 255);
				data_traveller_ptr++;
			}
		} /* for (all quads) */
	}
	else
	{
		*out_color_data_offset = 0;
	}
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void QuadsBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_data != DE_NULL)
	{
		delete[] m_data;

		m_data = DE_NULL;
	}

	if (m_fbo != 0)
	{
		m_gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_po != 0)
	{
		m_gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_to != 0)
	{
		m_gl.deleteTextures(1, &m_to);

		m_to = 0;
	}

	if (m_vao != 0)
	{
		m_gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void QuadsBufferStorageTestCase::deinitTestCaseIteration()
{
	/* If the test executed successfully, all pages should've been released by now.
	 * However, if it failed, it's a good idea to de-commit them at this point.
	 * Redundant calls are fine spec-wise, too. */
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,			 /* offset */
									 m_data_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool QuadsBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	bool result = true;

	m_gl.viewport(0, /* x */
				  0, /* y */
				  m_to_width, m_to_height);

	m_gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

	m_gl.clearColor(0.0f,  /* red */
					0.0f,  /* green */
					0.0f,  /* blue */
					0.0f); /* alpha */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearColor() call failed.");

	/* Render the quads.
	 *
	 * Run in two iterations:
	 *
	 * a) Iteration 1 performs the draw call with the VBO & IBO pages committed
	 * b) Iteration 2 performs the draw call with the VBO & IBO pages without any
	 *    physical backing.
	 **/
	for (unsigned int n_iteration = 0; n_iteration < 2; ++n_iteration)
	{
		initSparseBO((n_iteration == 0), /* decommit pages after upload */
					 (sparse_bo_storage_flags & GL_DYNAMIC_STORAGE_BIT) != 0);

		m_gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClear() call failed.");

		switch (m_ibo_usage)
		{
		case IBO_USAGE_NONE:
		{
			m_gl.drawArrays(GL_TRIANGLES, 0, /* first */
							m_n_vertices_to_draw);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays() call failed.");

			break;
		}

		case IBO_USAGE_INDEXED_DRAW_CALL:
		{
			m_gl.drawElements(GL_TRIANGLES, m_n_vertices_to_draw, GL_UNSIGNED_SHORT,
							  (glw::GLvoid*)(intptr_t)m_ibo_data_offset);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawElements() call failed.");

			break;
		}

		case IBO_USAGE_INDEXED_RANGED_DRAW_CALL:
		{
			m_gl.drawRangeElements(GL_TRIANGLES, 0,		 /* start */
								   m_n_vertices_to_draw, /* end */
								   m_n_vertices_to_draw, /* count */
								   GL_UNSIGNED_SHORT, (glw::GLvoid*)(intptr_t)m_ibo_data_offset);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawRangeElements() call failed.");

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized IBO usage value");
		}
		} /* switch (m_ibo_usage) */

		/* Retrieve the rendered output */
		unsigned char* read_data = new unsigned char[m_to_width * m_to_height * sizeof(char) * 4 /* rgba */];

		m_gl.readPixels(0, /* x */
						0, /* y */
						m_to_width, m_to_height, GL_RGBA, GL_UNSIGNED_BYTE, read_data);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glReadPixels() call failed.");

		/* IF the data pages have been committed by the time the draw call was made, validate the data.
		 *
		 * For each quad region (be it filled or not), check the center and make sure the retrieved
		 * color corresponds to the expected value.
		 */
		if (m_pages_committed)
		{
			for (unsigned int n_quad_region_y = 0; n_quad_region_y < m_n_quads_y * 2; /* quad + empty "delta" region */
				 ++n_quad_region_y)
			{
				for (unsigned int n_quad_region_x = 0; n_quad_region_x < m_n_quads_x * 2; ++n_quad_region_x)
				{
					/* Determine the expected texel color */
					unsigned char expected_color[4];
					unsigned char found_color[4];
					bool		  is_delta_region = (n_quad_region_x % 2) != 0 || (n_quad_region_y % 2) != 0;

					if (is_delta_region)
					{
						memset(expected_color, 0, sizeof(expected_color));
					} /* if (is_delta_region) */
					else
					{
						if (m_use_color_data)
						{
							const unsigned int   n_quad_x = n_quad_region_x / 2;
							const unsigned int   n_quad_y = n_quad_region_y / 2;
							const unsigned char* data_ptr =
								m_data + m_color_data_offset +
								(n_quad_y * m_n_quads_x + n_quad_x) * 4 /* rgba */ * 6; /* vertices */

							memcpy(expected_color, data_ptr, sizeof(expected_color));
						} /* if (m_use_color_data) */
						else
						{
							memset(expected_color, 255, sizeof(expected_color));
						}
					}

					/* Do we have a match? */
					DE_ASSERT(m_n_quad_height == m_n_quad_delta_y);
					DE_ASSERT(m_n_quad_width == m_n_quad_delta_x);

					const unsigned int sample_texel_x = m_n_quad_delta_x * n_quad_region_x;
					const unsigned int sample_texel_y = m_n_quad_delta_y * n_quad_region_y;

					memcpy(found_color, read_data + (sample_texel_y * m_to_width + sample_texel_x) * 4, /* rgba */
						   sizeof(found_color));

					if (memcmp(expected_color, found_color, sizeof(expected_color)) != 0)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid color found at "
																	   "("
										   << sample_texel_x << ", " << sample_texel_y << "): "
																						  "Expected color:"
																						  "("
										   << (int)expected_color[0] << ", " << (int)expected_color[1] << ", "
										   << (int)expected_color[2] << ", " << (int)expected_color[3] << "), "
																										  "Found:"
																										  "("
										   << (int)found_color[0] << ", " << (int)found_color[1] << ", "
										   << (int)found_color[2] << ", " << (int)found_color[3] << "), "
										   << tcu::TestLog::EndMessage;

						result = false;
						goto end;
					}
				} /* for (all quads in X) */
			}	 /* for (all quads in Y) */
		}		  /* if (m_pages_committed) */

		delete[] read_data;
		read_data = DE_NULL;
	} /* for (both iterations) */

end:
	return result;
}

/** Creates test data and fills the result buffer object (whose ID is stored under m_helper_bo)
 *  with the data.
 */
void QuadsBufferStorageTestCase::initHelperBO()
{
	DE_ASSERT(m_data == DE_NULL);
	DE_ASSERT(m_helper_bo == 0);

	createTestData(&m_data, &m_vbo_data_offset, &m_ibo_data_offset, &m_color_data_offset);

	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_COPY_READ_BUFFER, m_data_size, m_data, 0); /* flags */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");
}

/** Creates test data (if necessary), configures sparse buffer's memory page commitment
 *  and uploads the test data to the buffer object. Finally, the method configures the
 *  vertex array object, used by ::execute() at the draw call time.
 *
 *  @param decommit_data_pages_after_upload true to de-commit memory pages requested before
 *                                          uploading the vertex/index/color data.
 *  @param is_dynamic_storage               true to upload the data via glBufferSubData() call.
 *                                          false to use a copy op for the operation.
 **/
void QuadsBufferStorageTestCase::initSparseBO(bool decommit_data_pages_after_upload, bool is_dynamic_storage)
{
	/* Set up the vertex buffer object. */
	if (m_data == DE_NULL)
	{
		createTestData(&m_data, &m_vbo_data_offset, &m_ibo_data_offset, &m_color_data_offset);
	}
	else
	{
		/* Sanity checks */
		if (m_ibo_usage != IBO_USAGE_NONE)
		{
			DE_ASSERT(m_vbo_data_offset != m_ibo_data_offset);
		}

		if (m_use_color_data)
		{
			DE_ASSERT(m_vbo_data_offset != m_ibo_data_offset);
			DE_ASSERT(m_ibo_data_offset != m_color_data_offset);
		}
	}

	/* Commit as many pages as we need to upload the data */
	m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,			/* offset */
								 m_data_size_rounded, GL_TRUE); /* commit */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_pages_committed = true;

	/* Upload the data */
	if (is_dynamic_storage)
	{
		m_gl.bufferSubData(GL_ARRAY_BUFFER, 0, /* offset */
						   m_data_size, m_data);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferSubData() call failed.");
	}
	else
	{
		/* Sparse BO cannot be directly uploaded data to. Copy the data from a helper BO */
		m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, /* readOffset */
							   0,										/* writeOffset */
							   m_data_size);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");
	}

	/* Set the VAO up */
	m_gl.vertexAttribPointer(m_attribute_position_location, 4, /* size */
							 GL_FLOAT, GL_FALSE,			   /* normalized */
							 0,								   /* stride */
							 (glw::GLvoid*)(intptr_t)m_vbo_data_offset);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glVertexAttribPointer() call failed.");

	m_gl.enableVertexAttribArray(m_attribute_position_location); /* index */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glEnableVertexAttribPointer() call failed.");

	if (m_use_color_data)
	{
		m_gl.vertexAttribPointer(m_attribute_color_location, 4, /* size */
								 GL_UNSIGNED_BYTE, GL_TRUE,		/* normalized */
								 0,								/* stride */
								 (glw::GLvoid*)(intptr_t)m_color_data_offset);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glVertexAttribPointer() call failed.");

		m_gl.enableVertexAttribArray(m_attribute_color_location); /* index */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glEnableVertexAttribPointer() call failed.");
	}
	else
	{
		m_gl.vertexAttrib4f(m_attribute_color_location, 1.0f, 1.0f, 1.0f, 1.0f);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glVertexAttrib4f() call failed.");

		m_gl.disableVertexAttribArray(m_attribute_color_location);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDisableVertexAttribArray() call failed.");
	}

	if (m_ibo_usage != IBO_USAGE_NONE)
	{
		m_gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");
	} /* if (m_use_ibo) */

	/* If we were requested to do so, decommit the pages we have just uploaded
	 * the data to.
	 */
	if (decommit_data_pages_after_upload)
	{
		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,			 /* offset */
									 m_data_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_pages_committed = false;
	} /* if (decommit_data_pages_after_upload) */
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool QuadsBufferStorageTestCase::initTestCaseGlobal()
{
	bool result = true;

	/* Set up the texture object */
	DE_ASSERT(m_to == 0);

	m_gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenTextures() call failed.");

	m_gl.bindTexture(GL_TEXTURE_2D, m_to);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindTexture() call failed.");

	m_gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					  GL_RGBA8, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glTexStorage2D() call failed.");

	/* Set up the framebuffer object */
	DE_ASSERT(m_fbo == 0);

	m_gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenFramebuffers() call failed.");

	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindFramebuffer() call failed.");

	m_gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to, 0); /* level */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glFramebufferTexture2D() call failed.");

	/* Set up the vertex array object */
	DE_ASSERT(m_vao == 0);

	m_gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenVertexArrays() call failed.");

	m_gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindVertexArray() call failed.");

	/* Init a helper BO */
	initHelperBO();

	/* Set up the program object */
	const char* fs_body = "#version 430 core\n"
						  "\n"
						  "flat in  vec4 fs_color;\n"
						  "     out vec4 color;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    color = fs_color;\n"
						  "}\n";

	const char* vs_body = "#version 430 core\n"
						  "\n"
						  "in vec4 color;\n"
						  "in vec4 position;\n"
						  "\n"
						  "flat out vec4 fs_color;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    fs_color    = color;\n"
						  "    gl_Position = position;\n"
						  "}\n";

	const unsigned int attribute_locations[] = { m_attribute_color_location, m_attribute_position_location };
	const char*		   attribute_names[]	 = { "color", "position" };
	const unsigned int n_attributes			 = sizeof(attribute_locations) / sizeof(attribute_locations[0]);

	DE_ASSERT(m_po == 0);

	m_po = SparseBufferTestUtilities::createProgram(m_gl, &fs_body, 1, /* n_fs_body_parts */
													&vs_body, 1, attribute_names, attribute_locations,
													n_attributes); /* n_vs_body_parts */

	if (m_po == 0)
	{
		result = false;

		goto end;
	}

end:
	return result;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool QuadsBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
QueryBufferStorageTestCase::QueryBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext,
													   glw::GLint page_size)
	: m_gl(gl)
	, m_helper_bo(0)
	, m_n_triangles(15)
	, m_page_size(page_size)
	, m_po(0)
	, m_qo(0)
	, m_sparse_bo(0)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
	, m_testCtx(testContext)
	, m_vao(0)
{
	/* Left blank on purpose */
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void QueryBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_po != 0)
	{
		m_gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_qo != 0)
	{
		m_gl.deleteQueries(1, &m_qo);

		m_qo = 0;
	}

	if (m_vao != 0)
	{
		m_gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void QueryBufferStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool QueryBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	static const unsigned char data_r8_zero = 0;
	bool					   result		= true;

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

	/* Run two separate iterations:
	 *
	 * a) The page holding the query result value is committed.
	 * b) The page is not committed.
	 */
	for (unsigned int n_iteration = 0; n_iteration < 2; ++n_iteration)
	{
		const bool should_commit_page = (n_iteration == 0);

		/* Set up the memory page commitment */
		m_gl.bufferPageCommitmentARB(GL_QUERY_BUFFER, 0, /* offset */
									 m_sparse_bo_size_rounded, should_commit_page ? GL_TRUE : GL_FALSE);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		/* Run the draw call */
		m_gl.beginQuery(GL_PRIMITIVES_GENERATED, m_qo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBeginQuery() call failed.");

		m_gl.drawArrays(GL_TRIANGLES, 0, /* first */
						m_n_triangles * 3);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays() call failed.");

		m_gl.endQuery(GL_PRIMITIVES_GENERATED);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glEndQuery() call failed.");

		/* Copy the query result to the sparse buffer */
		for (unsigned int n_getter_call = 0; n_getter_call < 4; ++n_getter_call)
		{
			glw::GLsizei result_n_bytes;

			switch (n_getter_call)
			{
			case 0:
			{
				result_n_bytes = sizeof(glw::GLint);
				m_gl.getQueryObjectiv(m_qo, GL_QUERY_RESULT, (glw::GLint*)0); /* params */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetQueryObjectiv() call failed.");

				break;
			}

			case 1:
			{
				result_n_bytes = sizeof(glw::GLint);
				m_gl.getQueryObjectuiv(m_qo, GL_QUERY_RESULT, (glw::GLuint*)0); /* params */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetQueryObjectuiv() call failed.");

				break;
			}

			case 2:
			{
				result_n_bytes = sizeof(glw::GLint64);
				m_gl.getQueryObjecti64v(m_qo, GL_QUERY_RESULT, (glw::GLint64*)0);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetQueryObjecti64v() call failed.");

				break;
			}

			case 3:
			{
				result_n_bytes = sizeof(glw::GLint64);
				m_gl.getQueryObjectui64v(m_qo, GL_QUERY_RESULT, (glw::GLuint64*)0);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetQueryObjectui64v() call failed.");

				break;
			}

			default:
			{
				TCU_FAIL("Invalid getter call type");
			}
			} /* switch (n_getter_call) */

			/* Verify the query result */
			if (should_commit_page)
			{
				const glw::GLint64* result_ptr = NULL;

				m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_helper_bo);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

				m_gl.clearBufferData(GL_COPY_WRITE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &data_r8_zero);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearBufferData() call failed.");

				m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
									   0,											 /* writeOffset */
									   result_n_bytes);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

				result_ptr = (const glw::GLint64*)m_gl.mapBuffer(GL_COPY_WRITE_BUFFER, GL_READ_ONLY);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBuffer() call failed.");

				if (*result_ptr != m_n_triangles)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Invalid query result stored in a sparse buffer. Found: "
										  "["
									   << *result_ptr << "]"
														 ", expected: "
														 "["
									   << m_n_triangles << "]" << tcu::TestLog::EndMessage;

					result = false;
				}

				m_gl.unmapBuffer(GL_COPY_WRITE_BUFFER);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");
			} /* for (all query getter call types) */
		}	 /* if (should_commit_page) */
	}		  /* for (both iterations) */

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool QueryBufferStorageTestCase::initTestCaseGlobal()
{
	/* Determine sparse buffer storage size */
	m_sparse_bo_size		 = sizeof(glw::GLuint64);
	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_sparse_bo_size, m_page_size);

	/* Set up the test program object */
	static const char* vs_body = "#version 140\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
								 "}\n";

	m_po = SparseBufferTestUtilities::createProgram(m_gl, DE_NULL, /* fs_body_parts */
													0,			   /* n_fs_body_parts */
													&vs_body, 1,   /* n_vs_body_parts */
													DE_NULL,	   /* attribute_names */
													DE_NULL,	   /* attribute_locations */
													0);			   /* n_attribute_locations */

	if (m_po == 0)
	{
		TCU_FAIL("Test program linking failure");
	}

	/* Set up the helper buffer object */
	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_COPY_WRITE_BUFFER, sizeof(glw::GLint64), DE_NULL, /* data */
					   GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	/* Set up the test query object */
	m_gl.genQueries(1, &m_qo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenQueries() call failed.");

	/* Set up the VAO */
	m_gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenVertexArrays() call failed.");

	m_gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindVertexArray() call failed.");

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool QueryBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	/* Set up the sparse buffer. */
	m_gl.bindBuffer(GL_QUERY_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
SSBOStorageTestCase::SSBOStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext, glw::GLint page_size)
	: m_gl(gl)
	, m_helper_bo(0)
	, m_page_size(page_size)
	, m_po(0)
	, m_po_local_wg_size(1024)
	, m_result_bo(0)
	, m_sparse_bo(0)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
	, m_ssbo_data(DE_NULL)
	, m_testCtx(testContext)
{
	/* min max for SSBO size from GL_ARB_shader_storage_buffer_object is 16mb;
	 *
	 * The specified amount of space lets the test write as many
	 * ints as it's possible, with an assertion that our CS
	 * uses a std140 layout and the SSBO only contains an unsized array.
	 *
	 * NOTE: 16777216 % 1024 = 0, which is awesome because we can hardcode the
	 *       local workgroup size directly in the CS.
	 */
	m_sparse_bo_size		 = (16777216 / (sizeof(int) * 4) /* std140 */) * (sizeof(int) * 4);
	m_sparse_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_sparse_bo_size, m_page_size);
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void SSBOStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_po != 0)
	{
		m_gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_result_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_result_bo);

		m_result_bo = 0;
	}

	if (m_ssbo_data != DE_NULL)
	{
		delete[] m_ssbo_data;

		m_ssbo_data = DE_NULL;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void SSBOStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool SSBOStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool result = true;

	/* Bind the program object */
	m_gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

	/* Set up shader storage buffer bindings */
	m_gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, /* index */
						m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferBase() call failed.");

	/* Run the test in three iterations:
	 *
	 * a) All required pages are committed.
	 * b) Only half of the pages are committed (in a zig-zag layout)
	 * c) None of the pages are committed.
	 */
	for (unsigned int n_iteration = 0; n_iteration < 3; ++n_iteration)
	{
		bool result_local = true;

		/* Set up the shader storage buffer object's memory backing */
		const bool   is_zigzag_ssbo			  = (n_iteration == 1);
		unsigned int ssbo_commit_size		  = 0;
		unsigned int ssbo_commit_start_offset = 0;

		switch (n_iteration)
		{
		case 0:
		case 1:
		{
			ssbo_commit_size		 = m_sparse_bo_size_rounded;
			ssbo_commit_start_offset = 0;

			if (is_zigzag_ssbo)
			{
				const unsigned int n_pages = ssbo_commit_size / m_page_size;

				for (unsigned int n_page = 0; n_page < n_pages; n_page += 2)
				{
					m_gl.bufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, m_page_size * n_page, /* offset */
												 m_page_size,									 /* size */
												 GL_TRUE);										 /* commit */
				} /* for (all memory pages) */
			}
			else
			{
				m_gl.bufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0, /* offset */
											 ssbo_commit_size, GL_TRUE);  /* commit */
			}

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call(s) failed.");

			break;
		}

		case 2:
		{
			/* Use no physical memory backing */
			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized iteration index");
		}
		} /* switch (n_iteration) */

		/* Set up bindings for the copy op */
		m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
		m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

		/* Set up the sparse buffer's data storage */
		m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
							   0,											 /* writeOffset */
							   m_sparse_bo_size);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

		/* Run the compute program */
		DE_ASSERT((m_sparse_bo_size % m_po_local_wg_size) == 0);

		m_gl.dispatchCompute(m_sparse_bo_size / m_po_local_wg_size, 1, /* num_groups_y */
							 1);									   /* num_groups_z */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDispatchCompute() call failed.");

		/* Flush the caches */
		m_gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMemoryBarrier() call failed.");

		/* Copy SSBO's storage to a mappable result BO */
		m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_sparse_bo);
		m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_result_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

		m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
							   0,											 /* writeOffset */
							   m_sparse_bo_size);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

		/* Map the result BO to the process space */
		unsigned int		current_ssbo_offset = 0;
		const unsigned int* ssbo_data_ptr = (const unsigned int*)m_gl.mapBuffer(GL_COPY_WRITE_BUFFER, GL_READ_ONLY);

		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBuffer() call failed.");

		for (unsigned int n_invocation = 0; current_ssbo_offset < m_sparse_bo_size && result_local; ++n_invocation,
						  current_ssbo_offset = static_cast<unsigned int>(current_ssbo_offset +
																		  (sizeof(int) * 4 /* std140 */)))
		{
			const unsigned int n_page = current_ssbo_offset / m_page_size;

			if ((is_zigzag_ssbo && (n_page % 2) == 0) ||
				(!is_zigzag_ssbo && (current_ssbo_offset >= ssbo_commit_start_offset &&
									 current_ssbo_offset < (ssbo_commit_start_offset + ssbo_commit_size))))
			{
				if (ssbo_data_ptr[n_invocation * 4] != (n_invocation + 1))
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Value written to the SSBO at byte "
																   "["
									   << (sizeof(int) * n_invocation) << "]"
																		  " is invalid. Found:"
									   << "[" << ssbo_data_ptr[n_invocation * 4] << "]"
																					", expected:"
									   << "[" << (n_invocation + 1) << "]" << tcu::TestLog::EndMessage;

					result_local = false;
				}
			} /* if (ssbo_data_ptr[n_texel] != 1) */
		}	 /* for (all result values) */

		result &= result_local;

		m_gl.unmapBuffer(GL_COPY_WRITE_BUFFER);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");

		/* Remove the physical backing from the sparse buffer  */
		m_gl.bufferPageCommitmentARB(GL_SHADER_STORAGE_BUFFER, 0,		  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */

		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
	} /* for (three iterations) */

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool SSBOStorageTestCase::initTestCaseGlobal()
{
	/* Set up the test program */
	static const char* cs_body =
		"#version 430 core\n"
		"\n"
		"layout(local_size_x = 1024) in;\n"
		"\n"
		"layout(std140, binding = 0) buffer data\n"
		"{\n"
		"    restrict uint io_values[];\n"
		"};\n"
		"\n"
		"void main()\n"
		"{\n"
		"    uint value_index = gl_GlobalInvocationID.x;\n"
		"    uint new_value   = (io_values[value_index] == value_index) ? (value_index + 1u) : value_index;\n"
		"\n"
		"    io_values[value_index] = new_value;\n"
		"}\n";

	m_po = SparseBufferTestUtilities::createComputeProgram(m_gl, &cs_body, 1); /* n_cs_body_parts */

	/* Set up a data buffer we will use to initialize the SSBO with default data.
	 *
	 * CS uses a std140 layout for the SSBO, so we need to add the additional padding.
	 */
	DE_ASSERT((m_sparse_bo_size) != 0);
	DE_ASSERT((m_sparse_bo_size % (sizeof(int) * 4)) == 0);
	DE_ASSERT((m_sparse_bo_size % 1024) == 0);

	m_ssbo_data = new unsigned int[m_sparse_bo_size / sizeof(int)];

	memset(m_ssbo_data, 0, m_sparse_bo_size);

	for (unsigned int index = 0; index < m_sparse_bo_size / sizeof(int) / 4; ++index)
	{
		/* Mind the std140 rules for arrays of ints */
		m_ssbo_data[4 * index] = index;
	}

	/* During execution, we will need to use a helper buffer object. The BO will hold
	 * data we will be copying into the sparse buffer object for each iteration.
	 */
	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferData(GL_COPY_READ_BUFFER, m_sparse_bo_size, m_ssbo_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferData() call failed.");

	/* To retrieve the data written to a sparse SSBO, we need to use another
	 * non-sparse helper BO.
	 */
	m_gl.genBuffers(1, &m_result_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_result_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferData(GL_ARRAY_BUFFER, m_sparse_bo_size, DE_NULL, /* data */
					GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferData() call failed.");

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool SSBOStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	return result;
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 *  @param all_pages_committed        true to provide memory backing for all memory pages holding data used by the test.
 *                                    false to leave some of them uncommitted.
 */
TransformFeedbackBufferStorageTestCase::TransformFeedbackBufferStorageTestCase(const glw::Functions& gl,
																			   tcu::TestContext&	 testContext,
																			   glw::GLint			 page_size,
																			   bool all_pages_committed)
	: m_all_pages_committed(all_pages_committed)
	, m_data_bo(0)
	, m_data_bo_index_data_offset(0)
	, m_data_bo_indexed_indirect_arg_offset(0)
	, m_data_bo_indexed_mdi_arg_offset(0)
	, m_data_bo_regular_indirect_arg_offset(0)
	, m_data_bo_regular_mdi_arg_offset(0)
	, m_data_bo_size(0)
	, m_draw_call_baseInstance(1231)
	, m_draw_call_baseVertex(65537)
	, m_draw_call_first(913)
	, m_draw_call_firstIndex(4)
	, m_gl(gl)
	, m_helper_bo(0)
	, m_index_data(DE_NULL)
	, m_index_data_size(0)
	, m_indirect_arg_data(DE_NULL)
	, m_indirect_arg_data_size(0)
	, m_min_memory_page_span(4) /* as per test spec */
	, m_multidrawcall_drawcount(-1)
	, m_multidrawcall_primcount(-1)
	, m_n_instances_to_test(4)
	, m_n_vertices_per_instance(0)
	, m_page_size(page_size)
	, m_po_ia(0)
	, m_po_sa(0)
	, m_result_bo(0)
	, m_result_bo_size(0)
	, m_result_bo_size_rounded(0)
	, m_testCtx(testContext)
	, m_vao(0)
{
	/* Left blank on purpose */
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void TransformFeedbackBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_data_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_data_bo);

		m_data_bo = 0;
	}

	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_index_data != DE_NULL)
	{
		delete[] m_index_data;

		m_index_data = DE_NULL;
	}

	if (m_indirect_arg_data != DE_NULL)
	{
		delete[] m_indirect_arg_data;

		m_indirect_arg_data = DE_NULL;
	}

	if (m_po_ia != 0)
	{
		m_gl.deleteProgram(m_po_ia);

		m_po_ia = 0;
	}

	if (m_po_sa != 0)
	{
		m_gl.deleteProgram(m_po_sa);

		m_po_sa = 0;
	}

	if (m_result_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_result_bo);

		m_result_bo = 0;
	}

	if (m_vao != 0)
	{
		m_gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool TransformFeedbackBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	bool result = true;

	/* Iterate through two different transform feedback modes we need to test */
	for (unsigned int n_tf_type = 0; n_tf_type < 2; /* interleaved & separate attribs */
		 ++n_tf_type)
	{
		const bool is_ia_iteration = (n_tf_type == 0);

		/* Bind the test PO to the context */
		m_gl.useProgram(is_ia_iteration ? m_po_ia : m_po_sa);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

		/* Set up TF general binding, which is needed for a glClearBufferData() call
		 * we'll be firing shortly.
		 */
		m_gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, /* needed for the subsequent glClearBufferData() call */
						m_result_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		/* Iterate through all draw call types */
		for (unsigned int n_draw_call_type = 0; n_draw_call_type < DRAW_CALL_COUNT; ++n_draw_call_type)
		{
			int				   draw_call_count					= 0; /* != 1 for multi-draw calls only */
			int				   draw_call_first_instance_id[2]   = { -1 };
			int				   draw_call_first_vertex_id[2]		= { -1 };
			int				   draw_call_n_instances[2]			= { 0 };
			int				   draw_call_n_vertices[2]			= { 0 };
			bool			   draw_call_is_vertex_id_ascending = false;
			const _draw_call   draw_call_type					= (_draw_call)n_draw_call_type;
			unsigned int	   n_result_bytes_per_instance[2]   = { 0 };
			const unsigned int n_result_bytes_per_vertex		= sizeof(unsigned int) * 2;
			unsigned int	   n_result_bytes_total				= 0;
			glw::GLuint*	   result_ptr						= DE_NULL;

			m_gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_data_bo);
			m_gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_data_bo);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

			/* Commit pages needed to execute transform feed-back */
			if (m_all_pages_committed)
			{
				m_gl.bufferPageCommitmentARB(GL_TRANSFORM_FEEDBACK_BUFFER, 0,	/* offset */
											 m_result_bo_size_rounded, GL_TRUE); /* commit */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
			}
			else
			{
				for (unsigned int n_page = 0; n_page < m_result_bo_size_rounded / m_page_size; ++n_page)
				{
					m_gl.bufferPageCommitmentARB(GL_TRANSFORM_FEEDBACK_BUFFER, n_page * m_page_size, /* offset */
												 m_page_size,										 /* size   */
												 (n_page % 2 == 0) ? GL_TRUE : GL_FALSE);			 /* commit */
				}

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
			}

			/* Zero out the target BO before we begin the TF */
			static const unsigned char data_zero = 0;

			m_gl.clearBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &data_zero);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearBufferData() call failed.");

			/* Set up transform feed-back buffer bindings */
			DE_ASSERT(m_result_bo_size != 0);

			if (is_ia_iteration)
			{
				DE_ASSERT(m_result_bo != 0);

				m_gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
									 m_result_bo, 0,				  /* offset */
									 m_result_bo_size);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferRange() call failed.");
			}
			else
			{
				DE_ASSERT(m_result_bo_size % 2 == 0);

				m_gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
									 m_result_bo, 0,				  /* offset */
									 m_result_bo_size / 2);
				m_gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 1, /* index */
									 m_result_bo, m_result_bo_size / 2, m_result_bo_size / 2);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferRange() call(s) failed.");
			}

			m_gl.beginTransformFeedback(GL_POINTS);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBeginTransformFeedback() call failed.");

			/* NOTE: Some discussion about the expected "vertex id" value:
			 *
			 * In GL 4.5 core spec (Feb2/2015 version), we have:
			 *
			 * >>
			 * The index of any element transferred to the GL by DrawElementsOneInstance
			 * is referred to as its vertex ID, and may be read by a vertex shader as
			 * gl_VertexID. The vertex ID of the ith element transferred is the sum of
			 * basevertex and the value stored in the currently bound element array buffer at
			 * offset indices +i.
			 * <<
			 *
			 * So for glDrawElements*() derivatives, we will be expecting gl_VertexID to be set to
			 * (basevertex + index[i] + i)
			 *
			 * DrawArrays does not support the "base vertex" concept at all:
			 *
			 * >>
			 * The index of any element transferred to the GL by DrawArraysOneInstance
			 * is referred to as its vertex ID, and may be read by a vertex shader as gl_VertexID.
			 * The vertex ID of the ith element transferred is first + i.
			 * <<
			 *
			 * For regular draw calls, gl_VertexID should be of form:
			 *
			 * (first + i)
			 *
			 * In both cases, gl_InstanceID does NOT include the baseinstance value, as per:
			 *
			 * >>
			 * If an enabled vertex attribute array is instanced (it has a non-zero divisor as
			 * specified by VertexAttribDivisor), the element index that is transferred to the GL,
			 * for all vertices, is given by
			 *
			 * floor(instance / divisor) + baseinstance
			 *
			 * The value of instance may be read by a vertex shader as gl_InstanceID, as
			 * described in section 11.1.3.9
			 * <<
			 */
			switch (draw_call_type)
			{
			case DRAW_CALL_INDEXED:
			{
				m_gl.drawElements(GL_POINTS, m_n_vertices_per_instance, GL_UNSIGNED_INT,
								  (const glw::GLvoid*)(intptr_t)m_data_bo_index_data_offset);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawElements() call failed.");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_n_vertices_per_instance;
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = 1;
				draw_call_n_vertices[0]			 = m_n_vertices_per_instance;
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_n_vertices_per_instance;
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_INDEXED_BASE_VERTEX:
			{
				m_gl.drawElementsBaseVertex(GL_POINTS, m_n_vertices_per_instance, GL_UNSIGNED_INT,
											(const glw::GLvoid*)(intptr_t)m_data_bo_index_data_offset,
											m_draw_call_baseVertex);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawElementsBaseVertex() call failed.");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_draw_call_baseVertex + m_n_vertices_per_instance;
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = 1;
				draw_call_n_vertices[0]			 = m_n_vertices_per_instance;
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_n_vertices_per_instance;
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_INDEXED_INDIRECT:
			{
				m_gl.drawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT,
										  (const glw::GLvoid*)(intptr_t)m_data_bo_indexed_indirect_arg_offset);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawElementsIndirect() call failed.");

				draw_call_count				   = 1;
				draw_call_first_instance_id[0] = 0;
				draw_call_first_vertex_id[0] =
					m_draw_call_baseVertex +
					m_index_data[((unsigned int)(intptr_t)m_multidrawcall_index[1] - m_data_bo_index_data_offset) /
								 sizeof(unsigned int)];
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_multidrawcall_count[1];
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * draw_call_n_vertices[0];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_INDEXED_INDIRECT_MULTI:
			{
				m_gl.multiDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT,
											   (const glw::GLvoid*)(intptr_t)m_data_bo_indexed_mdi_arg_offset,
											   m_multidrawcall_drawcount, 0); /* stride */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMultiDrawElementsIndirect() call failed.");

				draw_call_count				   = m_multidrawcall_drawcount;
				draw_call_first_instance_id[0] = 0;
				draw_call_first_instance_id[1] = 0;
				draw_call_first_vertex_id[0] =
					m_draw_call_baseVertex +
					m_index_data[((unsigned int)(intptr_t)m_multidrawcall_index[0] - m_data_bo_index_data_offset) /
								 sizeof(unsigned int)];
				draw_call_first_vertex_id[1] =
					m_draw_call_baseVertex +
					m_index_data[((unsigned int)(intptr_t)m_multidrawcall_index[1] - m_data_bo_index_data_offset) /
								 sizeof(unsigned int)];
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = 1;
				draw_call_n_instances[1]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_multidrawcall_count[0];
				draw_call_n_vertices[1]			 = m_multidrawcall_count[1];
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * draw_call_n_vertices[0];
				n_result_bytes_per_instance[1]   = n_result_bytes_per_vertex * draw_call_n_vertices[1];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0] +
									   n_result_bytes_per_instance[1] * draw_call_n_instances[1];

				break;
			}

			case DRAW_CALL_INDEXED_MULTI:
			{
				m_gl.multiDrawElements(GL_POINTS, m_multidrawcall_count, GL_UNSIGNED_INT, m_multidrawcall_index,
									   m_multidrawcall_drawcount);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMultiDrawElements() call failed");

				draw_call_count				   = m_multidrawcall_drawcount;
				draw_call_first_instance_id[0] = 0;
				draw_call_first_instance_id[1] = 0;
				draw_call_first_vertex_id[0] =
					m_index_data[((unsigned int)(intptr_t)m_multidrawcall_index[0] - m_data_bo_index_data_offset) /
								 sizeof(unsigned int)];
				draw_call_first_vertex_id[1] =
					m_index_data[((unsigned int)(intptr_t)m_multidrawcall_index[1] - m_data_bo_index_data_offset) /
								 sizeof(unsigned int)];
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = 1;
				draw_call_n_instances[1]		 = 1;
				draw_call_n_vertices[0]			 = m_multidrawcall_count[0];
				draw_call_n_vertices[1]			 = m_multidrawcall_count[1];
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_multidrawcall_count[0];
				n_result_bytes_per_instance[1]   = n_result_bytes_per_vertex * m_multidrawcall_count[1];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0] +
									   n_result_bytes_per_instance[1] * draw_call_n_instances[1];

				break;
			}

			case DRAW_CALL_INDEXED_MULTI_BASE_VERTEX:
			{
				m_gl.multiDrawElementsBaseVertex(GL_POINTS, m_multidrawcall_count, GL_UNSIGNED_INT,
												 m_multidrawcall_index, m_multidrawcall_drawcount,
												 m_multidrawcall_basevertex);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMultiDrawElementsBaseVertex() call failed.");

				draw_call_count				   = m_multidrawcall_drawcount;
				draw_call_first_instance_id[0] = 0;
				draw_call_first_instance_id[1] = 0;
				draw_call_first_vertex_id[0] =
					m_multidrawcall_basevertex[0] +
					m_index_data[((unsigned int)(intptr_t)m_multidrawcall_index[0] - m_data_bo_index_data_offset) /
								 sizeof(unsigned int)];
				draw_call_first_vertex_id[1] =
					m_multidrawcall_basevertex[1] +
					m_index_data[((unsigned int)(intptr_t)m_multidrawcall_index[1] - m_data_bo_index_data_offset) /
								 sizeof(unsigned int)];
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = 1;
				draw_call_n_instances[1]		 = 1;
				draw_call_n_vertices[0]			 = m_multidrawcall_count[0];
				draw_call_n_vertices[1]			 = m_multidrawcall_count[1];
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_multidrawcall_count[0];
				n_result_bytes_per_instance[1]   = n_result_bytes_per_vertex * m_multidrawcall_count[1];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0] +
									   n_result_bytes_per_instance[1] * draw_call_n_instances[1];

				break;
			}

			case DRAW_CALL_INSTANCED_INDEXED:
			{
				m_gl.drawElementsInstanced(GL_POINTS, m_n_vertices_per_instance, GL_UNSIGNED_INT,
										   (const glw::GLvoid*)(intptr_t)m_data_bo_index_data_offset,
										   m_n_instances_to_test);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawElementsInstanced() call failed.");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_index_data[0];
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_n_vertices_per_instance;
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_n_vertices_per_instance;
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_INSTANCED_INDEXED_BASE_VERTEX:
			{
				m_gl.drawElementsInstancedBaseVertex(GL_POINTS, m_n_vertices_per_instance, GL_UNSIGNED_INT,
													 (const glw::GLvoid*)(intptr_t)m_data_bo_index_data_offset,
													 m_n_instances_to_test, m_draw_call_baseVertex);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawElementsInstancedBaseVertex() call failed.");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_draw_call_baseVertex + m_index_data[0];
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_n_vertices_per_instance;
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_n_vertices_per_instance;
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_INSTANCED_INDEXED_BASE_VERTEX_BASE_INSTANCE:
			{
				m_gl.drawElementsInstancedBaseVertexBaseInstance(
					GL_POINTS, m_n_vertices_per_instance, GL_UNSIGNED_INT,
					(const glw::GLvoid*)(intptr_t)m_data_bo_index_data_offset, m_n_instances_to_test,
					m_draw_call_baseVertex, m_draw_call_baseInstance);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawElementsInstancedBaseVertexBaseInstance() call failed.");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_draw_call_baseVertex + m_index_data[0];
				draw_call_is_vertex_id_ascending = false;
				draw_call_n_instances[0]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_n_vertices_per_instance;
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_n_vertices_per_instance;
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_REGULAR:
			{
				m_gl.drawArrays(GL_POINTS, m_draw_call_first, m_n_vertices_per_instance);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays() call failed");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_draw_call_first;
				draw_call_is_vertex_id_ascending = true;
				draw_call_n_instances[0]		 = 1;
				draw_call_n_vertices[0]			 = m_n_vertices_per_instance;
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_n_vertices_per_instance;
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_REGULAR_INDIRECT:
			{
				m_gl.drawArraysIndirect(GL_POINTS, (glw::GLvoid*)(intptr_t)m_data_bo_regular_indirect_arg_offset);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArraysIndirect() call failed.");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_draw_call_first;
				draw_call_is_vertex_id_ascending = true;
				draw_call_n_instances[0]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_multidrawcall_count[1];
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * draw_call_n_vertices[0];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_REGULAR_INDIRECT_MULTI:
			{
				m_gl.multiDrawArraysIndirect(GL_POINTS, (glw::GLvoid*)(intptr_t)m_data_bo_regular_mdi_arg_offset,
											 m_multidrawcall_drawcount, 0); /* stride */

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMultiDrawArraysIndirect() call failed.");

				draw_call_count					 = 2;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_instance_id[1]   = 0;
				draw_call_first_vertex_id[0]	 = m_draw_call_first;
				draw_call_first_vertex_id[1]	 = m_draw_call_first;
				draw_call_is_vertex_id_ascending = true;
				draw_call_n_instances[0]		 = 1;
				draw_call_n_instances[1]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_multidrawcall_count[0];
				draw_call_n_vertices[1]			 = m_multidrawcall_count[1];
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * draw_call_n_vertices[0];
				n_result_bytes_per_instance[1]   = n_result_bytes_per_vertex * draw_call_n_vertices[1];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0] +
									   n_result_bytes_per_instance[1] * draw_call_n_instances[1];

				break;
			}

			case DRAW_CALL_REGULAR_INSTANCED:
			{
				m_gl.drawArraysInstanced(GL_POINTS, m_draw_call_first, m_n_vertices_per_instance,
										 m_n_instances_to_test);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArraysInstanced() call failed.");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_draw_call_first;
				draw_call_is_vertex_id_ascending = true;
				draw_call_n_instances[0]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_n_vertices_per_instance;
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * draw_call_n_vertices[0];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_REGULAR_INSTANCED_BASE_INSTANCE:
			{
				m_gl.drawArraysInstancedBaseInstance(GL_POINTS, m_draw_call_first, m_n_vertices_per_instance,
													 m_n_instances_to_test, m_draw_call_baseInstance);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArraysInstancedBaseInstance() call failed.");

				draw_call_count					 = 1;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_vertex_id[0]	 = m_draw_call_first;
				draw_call_is_vertex_id_ascending = true;
				draw_call_n_instances[0]		 = m_n_instances_to_test;
				draw_call_n_vertices[0]			 = m_n_vertices_per_instance;
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * draw_call_n_vertices[0];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] * draw_call_n_instances[0];

				break;
			}

			case DRAW_CALL_REGULAR_MULTI:
			{
				m_gl.multiDrawArrays(GL_POINTS, m_multidrawcall_first, m_multidrawcall_count,
									 m_multidrawcall_drawcount);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMultiDrawArrays() call failed.");

				draw_call_count					 = m_multidrawcall_drawcount;
				draw_call_first_instance_id[0]   = 0;
				draw_call_first_instance_id[1]   = 0;
				draw_call_first_vertex_id[0]	 = m_multidrawcall_first[0];
				draw_call_first_vertex_id[1]	 = m_multidrawcall_first[1];
				draw_call_is_vertex_id_ascending = true;
				draw_call_n_instances[0]		 = 1;
				draw_call_n_instances[1]		 = 1;
				draw_call_n_vertices[0]			 = m_multidrawcall_count[0];
				draw_call_n_vertices[1]			 = m_multidrawcall_count[1];
				n_result_bytes_per_instance[0]   = n_result_bytes_per_vertex * m_multidrawcall_count[0];
				n_result_bytes_per_instance[1]   = n_result_bytes_per_vertex * m_multidrawcall_count[1];
				n_result_bytes_total			 = n_result_bytes_per_instance[0] + n_result_bytes_per_instance[1];

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized draw call type");
			}
			} /* switch (draw_call_type) */

			DE_ASSERT(n_result_bytes_total <= m_result_bo_size);

			m_gl.endTransformFeedback();
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glEndTransformFeedback() call failed.");

			/* Retrieve the captured data */
			glw::GLuint  mappable_bo_id			  = m_helper_bo;
			unsigned int mappable_bo_start_offset = 0;

			/* We cannot map the result BO storage directly into process space, since
			 * it's a sparse buffer. Copy the generated data to a helper BO and map
			 * that BO instead. */
			m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_result_bo);
			m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_helper_bo);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

			if (is_ia_iteration)
			{
				m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
									   0,											 /* writeOffset */
									   n_result_bytes_total);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");
			}
			else
			{
				DE_ASSERT((n_result_bytes_total % 2) == 0);

				m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
									   0,											 /* writeOffset */
									   n_result_bytes_total / 2);
				m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
									   m_result_bo_size / 2,	  /* readOffset  */
									   m_result_bo_size / 2,	  /* writeOffset */
									   n_result_bytes_total / 2); /* size        */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");
			}

			m_gl.bindBuffer(GL_ARRAY_BUFFER, mappable_bo_id);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

			result_ptr = (unsigned int*)m_gl.mapBufferRange(GL_ARRAY_BUFFER, mappable_bo_start_offset, m_result_bo_size,
															GL_MAP_READ_BIT);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBufferRange() call failed.");

			/* Verify the generated output */
			bool		continue_checking		  = true;
			glw::GLuint result_instance_id_stride = 0;
			glw::GLuint result_vertex_id_stride   = 0;

			if (is_ia_iteration)
			{
				result_instance_id_stride = 2;
				result_vertex_id_stride   = 2;
			}
			else
			{
				result_instance_id_stride = 1;
				result_vertex_id_stride   = 1;
			}

			/* For all draw calls.. */
			for (int n_draw_call = 0; n_draw_call < draw_call_count && continue_checking; ++n_draw_call)
			{
				/* ..and resulting draw call instances.. */
				for (int n_instance = 0; n_instance < draw_call_n_instances[n_draw_call] && continue_checking;
					 ++n_instance)
				{
					DE_ASSERT((n_result_bytes_per_instance[n_draw_call] % sizeof(unsigned int)) == 0);

					/* Determine where the result TF data start from */
					const glw::GLuint expected_instance_id = draw_call_first_instance_id[n_draw_call] + n_instance;
					glw::GLuint*	  result_instance_id_traveller_ptr = DE_NULL;
					glw::GLuint*	  result_vertex_id_traveller_ptr   = DE_NULL;

					if (is_ia_iteration)
					{
						result_instance_id_traveller_ptr = result_ptr;

						for (int n_prev_draw_call = 0; n_prev_draw_call < n_draw_call; ++n_prev_draw_call)
						{
							result_instance_id_traveller_ptr += draw_call_n_instances[n_prev_draw_call] *
																n_result_bytes_per_instance[n_prev_draw_call] /
																sizeof(unsigned int);
						}

						result_instance_id_traveller_ptr +=
							n_instance * n_result_bytes_per_instance[n_draw_call] / sizeof(unsigned int);
						result_vertex_id_traveller_ptr = result_instance_id_traveller_ptr + 1;
					} /* if (is_ia_iteration) */
					else
					{
						DE_ASSERT((m_result_bo_size % 2) == 0);

						result_instance_id_traveller_ptr = result_ptr;

						for (int n_prev_draw_call = 0; n_prev_draw_call < n_draw_call; ++n_prev_draw_call)
						{
							result_instance_id_traveller_ptr +=
								draw_call_n_instances[n_prev_draw_call] *
								n_result_bytes_per_instance[n_prev_draw_call] /
								2 / /* instance id..instance id data | vertex id..vertex id data */
								sizeof(unsigned int);
						}

						result_instance_id_traveller_ptr +=
							n_instance * n_result_bytes_per_instance[n_draw_call] / 2 / sizeof(unsigned int);
						result_vertex_id_traveller_ptr =
							result_instance_id_traveller_ptr + (m_result_bo_size / 2) / sizeof(unsigned int);
					}

					/* Start checking the generated output */
					for (int n_point = 0; n_point < draw_call_n_vertices[n_draw_call] && continue_checking; ++n_point)
					{
						glw::GLuint expected_vertex_id	= 1;
						glw::GLuint retrieved_instance_id = 2;
						glw::GLuint retrieved_vertex_id   = 3;

						if (draw_call_is_vertex_id_ascending)
						{
							expected_vertex_id = draw_call_first_vertex_id[n_draw_call] + n_point;
						} /* if (draw_call_is_vertex_id_ascending) */
						else
						{
							if (draw_call_first_vertex_id[n_draw_call] >= n_point)
							{
								expected_vertex_id = draw_call_first_vertex_id[n_draw_call] - n_point;
							}
							else
							{
								expected_vertex_id = 0;
							}
						}

						/* Only perform the check if the offsets refer to pages with physical backing.
						 *
						 * Note that, on platforms, whose page size % 4 != 0, the values can land partially in the no-man's land,
						 * and partially in the safe zone. In such cases, skip the verification. */
						const bool result_instance_id_page_has_physical_backing =
							(((((char*)result_instance_id_traveller_ptr - (char*)result_ptr) / m_page_size) % 2) ==
							 0) &&
							((((((char*)result_instance_id_traveller_ptr - (char*)result_ptr) + sizeof(unsigned int) -
								1) /
							   m_page_size) %
							  2) == 0);
						const bool result_vertex_id_page_has_physical_backing =
							(((((char*)result_vertex_id_traveller_ptr - (char*)result_ptr) / m_page_size) % 2) == 0) &&
							((((((char*)result_vertex_id_traveller_ptr - (char*)result_ptr) + sizeof(unsigned int) -
								1) /
							   m_page_size) %
							  2) == 0);

						retrieved_instance_id = *result_instance_id_traveller_ptr;
						result_instance_id_traveller_ptr += result_instance_id_stride;

						retrieved_vertex_id = *result_vertex_id_traveller_ptr;
						result_vertex_id_traveller_ptr += result_vertex_id_stride;

						if ((result_instance_id_page_has_physical_backing &&
							 retrieved_instance_id != expected_instance_id) ||
							(result_vertex_id_page_has_physical_backing && retrieved_vertex_id != expected_vertex_id))
						{
							m_testCtx.getLog()
								<< tcu::TestLog::Message << "For "
															"["
								<< getName() << "]"
												", sparse BO flags "
												"["
								<< SparseBufferTestUtilities::getSparseBOFlagsString(sparse_bo_storage_flags)
								<< "]"
								   ", draw call type "
								<< getDrawCallTypeString(draw_call_type) << " at index "
																			"["
								<< n_draw_call << " / " << (draw_call_count - 1) << "]"
																					", TF mode "
																					"["
								<< ((is_ia_iteration) ? "interleaved attribs" : "separate attribs") << "]"
								<< ", instance "
								   "["
								<< n_instance << " / " << (draw_call_n_instances[n_draw_call] - 1) << "]"
								<< ", point at index "
								   "["
								<< n_point << " / " << (draw_call_n_vertices[n_draw_call] - 1) << "]"
								<< ", VS-level gl_VertexID was equal to "
								   "["
								<< retrieved_vertex_id << "]"
														  " and gl_InstanceID was set to "
														  "["
								<< retrieved_instance_id << "]"
															", whereas gl_VertexID of value "
															"["
								<< expected_vertex_id << "]"
														 " and gl_InstanceID of value "
														 "["
								<< expected_instance_id << "]"
														   " were anticipated."
								<< tcu::TestLog::EndMessage;

							continue_checking = false;
							result			  = false;

							break;
						} /* if (reported gl_InstanceID / gl_VertexID values are wrong) */
					}	 /* for (all drawn points) */
				}		  /* for (all instances) */

				/* Release memory pages we have allocated for the transform feed-back.
				 *
				 * NOTE: For some iterations, this call will attempt to de-commit pages which
				 *       have not been assigned physical backing. This is a valid behavior,
				 *       as per spec.
				 */
				m_gl.bufferPageCommitmentARB(GL_TRANSFORM_FEEDBACK_BUFFER, 0,	 /* offset */
											 m_result_bo_size_rounded, GL_FALSE); /* commit */
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
			} /* for (all draw call) */

			m_gl.unmapBuffer(GL_ARRAY_BUFFER);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");
		} /* for (all draw call types) */
	}	 /* for (both TF modes) */

	return result;
}

/** Converts the internal enum to a null-terminated text string.
 *
 *  @param draw_call Draw call type to return a string for.
 *
 *  @return The requested string or "[?!]", if the enum was not recognized.
 **/
const char* TransformFeedbackBufferStorageTestCase::getDrawCallTypeString(_draw_call draw_call)
{
	const char* result = "[?!]";

	switch (draw_call)
	{
	case DRAW_CALL_INDEXED:
		result = "glDrawElements()";
		break;
	case DRAW_CALL_INDEXED_BASE_VERTEX:
		result = "glDrawElementsBaseVertex()";
		break;
	case DRAW_CALL_INDEXED_INDIRECT:
		result = "glDrawElementsIndirect()";
		break;
	case DRAW_CALL_INDEXED_INDIRECT_MULTI:
		result = "glMultiDrawElementIndirect()";
		break;
	case DRAW_CALL_INDEXED_MULTI:
		result = "glMultiDrawElements()";
		break;
	case DRAW_CALL_INDEXED_MULTI_BASE_VERTEX:
		result = "glMultiDrawElementsBaseVertex()";
		break;
	case DRAW_CALL_INSTANCED_INDEXED:
		result = "glDrawElementsInstanced()";
		break;
	case DRAW_CALL_INSTANCED_INDEXED_BASE_VERTEX:
		result = "glDrawElementsInstancedBaseVertex()";
		break;
	case DRAW_CALL_INSTANCED_INDEXED_BASE_VERTEX_BASE_INSTANCE:
		result = "glDrawElementsInstancedBaseVertexBaseInstance()";
		break;
	case DRAW_CALL_REGULAR:
		result = "glDrawArrays()";
		break;
	case DRAW_CALL_REGULAR_INDIRECT:
		result = "glDrawArraysIndirect()";
		break;
	case DRAW_CALL_REGULAR_INDIRECT_MULTI:
		result = "glMultiDrawArraysIndirect()";
		break;
	case DRAW_CALL_REGULAR_INSTANCED:
		result = "glDrawArraysInstanced()";
		break;
	case DRAW_CALL_REGULAR_INSTANCED_BASE_INSTANCE:
		result = "glDrawArraysInstancedBaseInstance()";
		break;
	case DRAW_CALL_REGULAR_MULTI:
		result = "glMultiDrawArrays()";
		break;

	default:
		break;
	} /* switch (draw_call) */

	return result;
}

/** Initializes test data buffer, and then sets up:
 *
 *  - an immutable buffer object (id stored in m_data_bo), to which the test data
 *    is copied.
 *  - a mappable immutable buffer object (id stored in m_helper_bo)
 **/
void TransformFeedbackBufferStorageTestCase::initDataBO()
{
	initTestData();

	/* Initialize data BO (the BO which holds index + indirect draw call args */
	DE_ASSERT(m_data_bo == 0);

	m_gl.genBuffers(1, &m_data_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_data_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_ARRAY_BUFFER, m_data_bo_size, DE_NULL, GL_DYNAMIC_STORAGE_BIT);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	m_gl.bufferSubData(GL_ARRAY_BUFFER, m_data_bo_indexed_indirect_arg_offset, m_indirect_arg_data_size,
					   m_indirect_arg_data);
	m_gl.bufferSubData(GL_ARRAY_BUFFER, m_data_bo_index_data_offset, m_index_data_size, m_index_data);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferSubData() call(s) failed.");

	/* Generate & bind a helper BO we need to copy the data to from the sparse BO
	 * if direct mapping is not possible.
	 */
	DE_ASSERT(m_result_bo_size != 0);
	DE_ASSERT(m_result_bo == 0);
	DE_ASSERT(m_helper_bo == 0);

	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_ARRAY_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_ARRAY_BUFFER, m_result_bo_size, DE_NULL, /* data */
					   GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool TransformFeedbackBufferStorageTestCase::initTestCaseGlobal()
{
	bool result = true;

	/* Initialize test program object */
	static const char*		  tf_varyings[] = { "instance_id", "vertex_id" };
	static const unsigned int n_tf_varyings = sizeof(tf_varyings) / sizeof(tf_varyings[0]);
	static const char*		  vs_body		= "#version 420 core\n"
								 "\n"
								 "out uint instance_id;\n"
								 "out uint vertex_id;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    instance_id = gl_InstanceID;\n"
								 "    vertex_id   = gl_VertexID;\n"
								 "}\n";

	m_po_ia = SparseBufferTestUtilities::createProgram(m_gl, DE_NULL, /* fs_body_parts */
													   0,			  /* n_fs_body_parts */
													   &vs_body, 1,   /* n_vs_body_parts */
													   DE_NULL,		  /* attribute_names */
													   DE_NULL,		  /* attribute_locations */
													   0,			  /* n_attribute_properties */
													   tf_varyings, n_tf_varyings, GL_INTERLEAVED_ATTRIBS);

	m_po_sa = SparseBufferTestUtilities::createProgram(m_gl, DE_NULL, /* fs_body_parts */
													   0,			  /* n_fs_body_parts */
													   &vs_body, 1,   /* n_vs_body_parts */
													   DE_NULL,		  /* attribute_names */
													   DE_NULL,		  /* attribute_locations */
													   0,			  /* n_attribute_properties */
													   tf_varyings, n_tf_varyings, GL_SEPARATE_ATTRIBS);

	if (m_po_ia == 0 || m_po_sa == 0)
	{
		result = false;

		goto end;
	}

	/* Generate & bind a VAO */
	m_gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenVertexArrays() call failed.");

	m_gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindVertexArray() call failed.");

	initDataBO();

end:
	return result;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool TransformFeedbackBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Initialize buffer objects used by the test case */
	m_result_bo = sparse_bo;

	/* Sanity check */
	DE_ASSERT(m_data_bo != 0);

	return result;
}

/** Sets up client-side data arrays, later uploaded to the test buffer object, used as a source for:
 *
 *  - index data
 *  - indirect draw call arguments
 *  - multi draw call arguments
 **/
void TransformFeedbackBufferStorageTestCase::initTestData()
{
	/* We need the result data to span across at least m_min_memory_page_span memory pages.
	 * Each vertex outputs 2 * sizeof(int) = 8 bytes of data.
	 *
	 * For simplicity, we assume the number of bytes we calculate here is per instance. */
	m_n_vertices_per_instance = static_cast<unsigned int>((m_page_size * m_min_memory_page_span / (sizeof(int) * 2)));

	/* Let:
	 *
	 *     index_data_size       = (n of vertices per a single instance) * sizeof(unsigned int)
	 *     indexed_indirect_size = sizeof(glDrawElementsIndirect()      indirect arguments)
	 *     indexed_mdi_size      = sizeof(glMultiDrawElementsIndirect() indirect arguments) * 2 (single instance & multiple instances case)
	 *     regular_indirect_size = sizeof(glDrawArraysIndirect()        indirect arguments)
	 *     regular_mdi_size      = sizeof(glMultiDrawArraysIndirect()   indirect arguments) * 2 (single instance & multiple instances case)
	 *
	 *
	 * The layout we will use for the data buffer is:
	 *
	 * [indexed indirect arg data // Size: indexed_indirect_size bytes]
	 * [indexed MDI arg data      // Size: indexed_mdi_size      bytes]
	 * [regular indirect arg data // Size: regular_indirect_size bytes]
	 * [regular MDI arg data      // Size: regular_mdi_size      bytes]
	 * [index data                // Size: index_data_size       bytes]
	 */
	const unsigned int indexed_indirect_size = sizeof(unsigned int) * 5 /* as per GL spec */;
	const unsigned int indexed_mdi_size		 = sizeof(unsigned int) * 5 /* as per GL spec */ * 2; /* draw calls */
	const unsigned int regular_indirect_size = sizeof(unsigned int) * 4;						  /* as per GL spec */
	const unsigned int regular_mdi_size		 = sizeof(unsigned int) * 4 /* as per GL spec */ * 2; /* draw calls */

	m_data_bo_indexed_indirect_arg_offset = 0;
	m_data_bo_indexed_mdi_arg_offset	  = m_data_bo_indexed_indirect_arg_offset + indexed_indirect_size;
	m_data_bo_regular_indirect_arg_offset = m_data_bo_indexed_mdi_arg_offset + indexed_mdi_size;
	m_data_bo_regular_mdi_arg_offset	  = m_data_bo_regular_indirect_arg_offset + regular_indirect_size;
	m_data_bo_index_data_offset			  = m_data_bo_regular_mdi_arg_offset + regular_mdi_size;

	/* Form the index data */
	DE_ASSERT(m_index_data == DE_NULL);
	DE_ASSERT(m_draw_call_firstIndex == sizeof(unsigned int));

	m_index_data_size = static_cast<glw::GLuint>(
		(1 /* extra index, as per m_draw_call_firstIndex */ + m_n_vertices_per_instance) * sizeof(unsigned int));
	m_index_data = (unsigned int*)new unsigned char[m_index_data_size];

	for (unsigned int n_index = 0; n_index < m_n_vertices_per_instance + 1; ++n_index)
	{
		m_index_data[n_index] = m_n_vertices_per_instance - n_index;
	} /* for (all available indices) */

	/* Set multi draw-call arguments */
	m_multidrawcall_basevertex[0] = m_draw_call_baseVertex;
	m_multidrawcall_basevertex[1] = 257;
	m_multidrawcall_count[0]	  = m_n_vertices_per_instance;
	m_multidrawcall_count[1]	  = m_n_vertices_per_instance - 16;
	m_multidrawcall_drawcount	 = 2;
	m_multidrawcall_first[0]	  = 0;
	m_multidrawcall_first[1]	  = m_draw_call_first;
	m_multidrawcall_index[0]	  = (glw::GLvoid*)(intptr_t)m_data_bo_index_data_offset;
	m_multidrawcall_index[1]	  = (glw::GLvoid*)(intptr_t)(m_data_bo_index_data_offset + m_draw_call_firstIndex);
	m_multidrawcall_primcount	 = m_n_instances_to_test;

	/* Form the indirect data */
	DE_ASSERT(m_indirect_arg_data == DE_NULL);

	m_indirect_arg_data_size = m_data_bo_index_data_offset - m_data_bo_indexed_indirect_arg_offset;
	m_indirect_arg_data		 = (unsigned int*)new unsigned char[m_indirect_arg_data_size];

	unsigned int* indirect_arg_data_traveller_ptr = m_indirect_arg_data;

	/* 1. Indexed indirect arg data */
	DE_ASSERT(((unsigned int)(intptr_t)(m_multidrawcall_index[1]) % sizeof(unsigned int)) == 0);

	*indirect_arg_data_traveller_ptr = m_multidrawcall_count[1]; /* count */
	indirect_arg_data_traveller_ptr++;

	*indirect_arg_data_traveller_ptr = m_n_instances_to_test; /* primCount */
	indirect_arg_data_traveller_ptr++;

	*indirect_arg_data_traveller_ptr = static_cast<unsigned int>((unsigned int)(intptr_t)(m_multidrawcall_index[1]) /
																 sizeof(unsigned int)); /* firstIndex */
	indirect_arg_data_traveller_ptr++;

	*indirect_arg_data_traveller_ptr = m_draw_call_baseVertex; /* baseVertex */
	indirect_arg_data_traveller_ptr++;

	*indirect_arg_data_traveller_ptr = m_draw_call_baseInstance; /* baseInstance */
	indirect_arg_data_traveller_ptr++;

	/* 2. Indexed MDI arg data */
	for (unsigned int n_draw_call = 0; n_draw_call < 2; ++n_draw_call)
	{
		DE_ASSERT(((unsigned int)(intptr_t)(m_multidrawcall_index[n_draw_call]) % sizeof(unsigned int)) == 0);

		*indirect_arg_data_traveller_ptr = m_multidrawcall_count[n_draw_call]; /* count */
		indirect_arg_data_traveller_ptr++;

		*indirect_arg_data_traveller_ptr = (n_draw_call == 0) ? 1 : m_n_instances_to_test; /* primCount */
		indirect_arg_data_traveller_ptr++;

		*indirect_arg_data_traveller_ptr = static_cast<unsigned int>(
			(unsigned int)(intptr_t)(m_multidrawcall_index[n_draw_call]) / sizeof(unsigned int)); /* firstIndex */
		indirect_arg_data_traveller_ptr++;

		*indirect_arg_data_traveller_ptr = m_draw_call_baseVertex;
		indirect_arg_data_traveller_ptr++;

		*indirect_arg_data_traveller_ptr = m_draw_call_baseInstance;
		indirect_arg_data_traveller_ptr++;
	} /* for (both single-instanced and multi-instanced cases) */

	/* 3. Regular indirect arg data */
	*indirect_arg_data_traveller_ptr = m_multidrawcall_count[1]; /* count */
	indirect_arg_data_traveller_ptr++;

	*indirect_arg_data_traveller_ptr = m_n_instances_to_test; /* primCount */
	indirect_arg_data_traveller_ptr++;

	*indirect_arg_data_traveller_ptr = m_draw_call_first; /* first */
	indirect_arg_data_traveller_ptr++;

	*indirect_arg_data_traveller_ptr = m_draw_call_baseInstance; /* baseInstance */
	indirect_arg_data_traveller_ptr++;

	/* 4. Regular MDI arg data */
	for (unsigned int n_draw_call = 0; n_draw_call < 2; ++n_draw_call)
	{
		*indirect_arg_data_traveller_ptr = m_multidrawcall_count[n_draw_call]; /* count */
		indirect_arg_data_traveller_ptr++;

		*indirect_arg_data_traveller_ptr = (n_draw_call == 0) ? 1 : m_n_instances_to_test; /* instanceCount */
		indirect_arg_data_traveller_ptr++;

		*indirect_arg_data_traveller_ptr = m_draw_call_first; /* first */
		indirect_arg_data_traveller_ptr++;

		*indirect_arg_data_traveller_ptr = m_draw_call_baseInstance; /* baseInstance */
		indirect_arg_data_traveller_ptr++;
	} /* for (both single-instanced and multi-instanced cases) */

	/* Store the number of bytes we will need to allocate for the data BO */
	m_data_bo_size = m_index_data_size + m_indirect_arg_data_size;

	/* Determine the number of bytes we will need to have at hand to hold all the captured TF varyings.
	 * The equation below takes into account the heaviest draw call the test will ever issue.
	 */
	m_result_bo_size =
		static_cast<glw::GLuint>(sizeof(unsigned int) * 2 /* TF varyings per vertex */ *
								 (m_multidrawcall_count[0] + m_multidrawcall_count[1]) * m_multidrawcall_primcount);
	m_result_bo_size_rounded = SparseBufferTestUtilities::alignOffset(m_result_bo_size, m_page_size);

	/* Sanity checks */
	DE_ASSERT(m_min_memory_page_span > 0);
	DE_ASSERT(m_page_size > 0);
	DE_ASSERT(m_result_bo_size >= (m_min_memory_page_span * m_page_size));
}

/** Constructor.
 *
 *  @param gl                         GL entry-points container
 *  @param testContext                CTS test context
 *  @param page_size                  Page size, as reported by implementation for the GL_SPARSE_BUFFER_PAGE_SIZE_ARB query.
 *  @param pGLBufferPageCommitmentARB Func ptr to glBufferPageCommitmentARB() entry-point.
 */
UniformBufferStorageTestCase::UniformBufferStorageTestCase(const glw::Functions& gl, tcu::TestContext& testContext,
														   glw::GLint page_size)
	: m_gl(gl)
	, m_gl_uniform_buffer_offset_alignment_value(0)
	, m_helper_bo(0)
	, m_n_pages_to_use(4)
	, m_n_ubo_uints(0)
	, m_page_size(page_size)
	, m_po(0)
	, m_sparse_bo(0)
	, m_sparse_bo_data_size(0)
	, m_sparse_bo_data_start_offset(0)
	, m_sparse_bo_size(0)
	, m_sparse_bo_size_rounded(0)
	, m_testCtx(testContext)
	, m_tf_bo(0)
	, m_ubo_data(DE_NULL)
	, m_vao(0)
{
	if ((m_n_pages_to_use % 2) != 0)
	{
		DE_ASSERT(DE_FALSE);
	}
}

/** Releases all GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
void UniformBufferStorageTestCase::deinitTestCaseGlobal()
{
	if (m_helper_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_helper_bo);

		m_helper_bo = 0;
	}

	if (m_po != 0)
	{
		m_gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_tf_bo != 0)
	{
		m_gl.deleteBuffers(1, &m_tf_bo);

		m_tf_bo = 0;
	}

	if (m_ubo_data != DE_NULL)
	{
		delete[] m_ubo_data;

		m_ubo_data = DE_NULL;
	}

	if (m_vao != 0)
	{
		m_gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}
}

/** Releases temporary GL objects, created specifically for one test case iteration. */
void UniformBufferStorageTestCase::deinitTestCaseIteration()
{
	if (m_sparse_bo != 0)
	{
		m_gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0,				  /* offset */
									 m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		m_sparse_bo = 0;
	}
}

/** Executes a single test iteration. The BufferStorage test will call this method
 *  numerously during its life-time, testing various valid flag combinations applied
 *  to the tested sparse buffer object at glBufferStorage() call time.
 *
 *  @param sparse_bo_storage_flags <flags> argument, used by the test in the glBufferStorage()
 *                                 call to set up the sparse buffer's storage.
 *
 *  @return true if the test case executed correctly, false otherwise.
 */
bool UniformBufferStorageTestCase::execute(glw::GLuint sparse_bo_storage_flags)
{
	(void)sparse_bo_storage_flags;
	bool result = true;

	m_gl.bindBufferRange(GL_UNIFORM_BUFFER, 0, /* index */
						 m_sparse_bo, m_sparse_bo_data_start_offset, m_sparse_bo_data_size);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferBase() call failed.");

	/* Run the test in three iterations:
	 *
	 * 1) Whole UBO storage is backed by physical backing.
	 * 2) Half the UBO storage is backed by physical backing.
	 * 3) None of the UBO storage is backed by physical backing.
	 */
	for (unsigned int n_iteration = 0; n_iteration < 3; ++n_iteration)
	{
		bool		 result_local			 = true;
		unsigned int ubo_commit_size		 = 0;
		unsigned int ubo_commit_start_offset = 0;

		switch (n_iteration)
		{
		case 0:
		{
			ubo_commit_size			= m_sparse_bo_data_size;
			ubo_commit_start_offset = m_sparse_bo_data_start_offset;

			break;
		}

		case 1:
		{
			DE_ASSERT((m_sparse_bo_data_size % 2) == 0);
			DE_ASSERT((m_sparse_bo_data_size % m_page_size) == 0);

			ubo_commit_size			= m_sparse_bo_data_size / 2;
			ubo_commit_start_offset = m_sparse_bo_data_start_offset;

			break;
		}

		case 2:
		{
			/* The default values do just fine */

			break;
		}

		default:
		{
			TCU_FAIL("Invalid iteration index");
		}
		} /* switch (n_iteration) */

		m_gl.bufferPageCommitmentARB(GL_UNIFORM_BUFFER, ubo_commit_start_offset, ubo_commit_size, GL_TRUE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");

		/* Copy the UBO data */
		m_gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, /* readOffset */
							   ubo_commit_start_offset, ubo_commit_size);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCopyBufferSubData() call failed.");

		/* Issue the draw call to execute the test */
		m_gl.useProgram(m_po);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() call failed.");

		m_gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_tf_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

		m_gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
							m_tf_bo);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBufferBase() call failed.");

		m_gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBeginTransformFeedback() call failed.");

		m_gl.drawArrays(GL_POINTS, 0, /* first */
						m_n_ubo_uints);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays() call failed.");

		m_gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glEndTransformFeedback() call failed.");

		/* Retrieve the data, verify the output */
		const unsigned int* result_data_ptr =
			(const unsigned int*)m_gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
		unsigned int ubo_data_offset = m_sparse_bo_data_start_offset;

		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glMapBuffer() call failed.");

		for (unsigned int n_vertex = 0; n_vertex < m_n_ubo_uints && result_local;
			 ++n_vertex, ubo_data_offset = static_cast<unsigned int>(ubo_data_offset + 4 * sizeof(unsigned int)))
		{
			const bool is_ub_data_physically_backed = (ubo_data_offset >= ubo_commit_start_offset &&
													   ubo_data_offset < (ubo_commit_start_offset + ubo_commit_size)) ?
														  1 :
														  0;
			unsigned int	   expected_value  = -1;
			const unsigned int retrieved_value = result_data_ptr[n_vertex];

			if (is_ub_data_physically_backed)
			{
				expected_value = 1;
			}
			else
			{
				/* Read ops applied against non-committed sparse buffers return an undefined value.
				 */
				continue;
			}

			if (expected_value != retrieved_value)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value "
															   "("
								   << retrieved_value << ") "
														 "found at index "
														 "("
								   << n_vertex << ")"
												  ", instead of the expected value "
												  "("
								   << expected_value << ")"
														". Iteration index:"
														"("
								   << n_iteration << ")" << tcu::TestLog::EndMessage;

				result_local = false;
			}
		}

		result &= result_local;

		/* Clean up in anticipation for the next iteration */
		static const unsigned char data_zero_r8 = 0;

		m_gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUnmapBuffer() call failed.");

		m_gl.clearBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &data_zero_r8);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearBufferData() call failed.");

		m_gl.bufferPageCommitmentARB(GL_UNIFORM_BUFFER, 0, m_sparse_bo_size_rounded, GL_FALSE); /* commit */
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferPageCommitmentARB() call failed.");
	} /* for (all three iterations) */

	return result;
}

/** Initializes GL objects used across all test case iterations.
 *
 *  Called once during BufferStorage test run-time.
 */
bool UniformBufferStorageTestCase::initTestCaseGlobal()
{
	/* Cache GL constant values */
	glw::GLint gl_max_uniform_block_size_value = 0;

	m_gl.getIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &gl_max_uniform_block_size_value);
	m_gl.getIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_gl_uniform_buffer_offset_alignment_value);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetIntegerv() call(s) failed.");

	/* Determine the number of uints we can access at once from a single VS invocation */
	DE_ASSERT(gl_max_uniform_block_size_value >= 1);

	/* Account for the fact that in std140 layout, array elements will be rounded up
	 * to the size of a vec4, i.e. 16 bytes. */
	m_n_ubo_uints = static_cast<unsigned int>(gl_max_uniform_block_size_value / (4 * sizeof(unsigned int)));

	/* Prepare the test program */
	std::stringstream vs_body_define_sstream;
	std::string		  vs_body_define_string;

	const char* tf_varying		 = "result";
	const char* vs_body_preamble = "#version 140\n"
								   "\n";

	const char* vs_body_main = "\n"
							   "layout(std140) uniform data\n"
							   "{\n"
							   "    uint data_input[N_UBO_UINTS];"
							   "};\n"
							   "\n"
							   "out uint result;\n"
							   "\n"
							   "void main()\n"
							   "{\n"
							   "    result = (data_input[gl_VertexID] == uint(gl_VertexID) ) ? 1u : 0u;\n"
							   "}";

	vs_body_define_sstream << "#define N_UBO_UINTS (" << m_n_ubo_uints << ")\n";
	vs_body_define_string = vs_body_define_sstream.str();

	const char*		   vs_body_parts[] = { vs_body_preamble, vs_body_define_string.c_str(), vs_body_main };
	const unsigned int n_vs_body_parts = sizeof(vs_body_parts) / sizeof(vs_body_parts[0]);

	m_po = SparseBufferTestUtilities::createProgram(m_gl, DE_NULL,							 /* fs_body_parts */
													0,										 /* n_fs_body_parts */
													vs_body_parts, n_vs_body_parts, DE_NULL, /* attribute_names */
													DE_NULL,								 /* attribute_locations */
													0,				/* n_attribute_properties */
													&tf_varying, 1, /* n_tf_varyings */
													GL_INTERLEAVED_ATTRIBS);

	if (m_po == 0)
	{
		TCU_FAIL("The test program failed to link");
	}

	/* Determine the number of bytes the sparse buffer needs to be able to have
	 * a physical backing or.
	 *
	 * We will provide physical backing for twice the required size and then use
	 * a region in the centered of the allocated memory block.
	 *
	 * NOTE: We need to be able to use an offset which is aligned to both the page size,
	 *       and the UB offset alignment.
	 * */
	m_sparse_bo_data_size = static_cast<unsigned int>(sizeof(unsigned int) * m_page_size);
	m_sparse_bo_size	  = (m_page_size * m_gl_uniform_buffer_offset_alignment_value) * 2;

	if (m_sparse_bo_size < m_sparse_bo_data_size * 2)
	{
		m_sparse_bo_size = m_sparse_bo_data_size * 2;
	}

	m_sparse_bo_size_rounded	  = m_sparse_bo_size; /* rounded to the page size by default */
	m_sparse_bo_data_start_offset = (m_sparse_bo_size - m_sparse_bo_data_size) / 2;

	/* Set up the TFBO storage */
	const unsigned tfbo_size = static_cast<unsigned int>(sizeof(unsigned int) * m_n_ubo_uints);

	m_gl.genBuffers(1, &m_tf_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_tf_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bufferStorage(GL_TRANSFORM_FEEDBACK_BUFFER, tfbo_size, DE_NULL, /* data */
					   GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	/* Set up the UBO contents. We're actually setting up an immutable BO here,
	 * but we'll use its contents for a copy op, executed at the beginning of
	 * each iteration.
	 */
	unsigned int* ubo_data_traveller_ptr = DE_NULL;

	DE_ASSERT((m_sparse_bo_data_size % sizeof(unsigned int)) == 0);

	m_ubo_data			   = new (std::nothrow) unsigned char[m_sparse_bo_data_size];
	ubo_data_traveller_ptr = (unsigned int*)m_ubo_data;

	for (unsigned int n_vertex = 0; n_vertex < m_sparse_bo_data_size / (4 * sizeof(unsigned int)); ++n_vertex)
	{
		*ubo_data_traveller_ptr = n_vertex;
		ubo_data_traveller_ptr += 4;
	}

	m_gl.genBuffers(1, &m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenBuffers() call failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	/* Set up helper BO storage */
	m_gl.bufferStorage(GL_COPY_READ_BUFFER, m_sparse_bo_data_size, m_ubo_data, 0); /* flags */
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBufferStorage() call failed.");

	/* Set up the VAO */
	m_gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenVertexArrays() call failed.");

	m_gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindVertexArray() call failed.");

	return true;
}

/** Initializes GL objects which are needed for a single test case iteration.
 *
 *  deinitTestCaseIteration() will be called after the test case is executed in ::execute()
 *  to release these objects.
 **/
bool UniformBufferStorageTestCase::initTestCaseIteration(glw::GLuint sparse_bo)
{
	bool result = true;

	/* Cache the BO id, if not cached already */
	DE_ASSERT(m_sparse_bo == 0 || m_sparse_bo == sparse_bo);

	m_sparse_bo = sparse_bo;

	/* Set up the sparse buffer bindings. */
	m_gl.bindBuffer(GL_COPY_WRITE_BUFFER, m_sparse_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call(s) failed.");

	m_gl.bindBuffer(GL_COPY_READ_BUFFER, m_helper_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	m_gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_tf_bo);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindBuffer() call failed.");

	return result;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
BufferStorageTest::BufferStorageTest(deqp::Context& context)
	: TestCase(context, "BufferStorageTest", "Tests various interactions between sparse buffers and other API areas")
	, m_sparse_bo(0)
{
	/* Left blank intentionally */
}

/** Tears down any GL objects set up to run the test. */
void BufferStorageTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* De-initialize all test the test cases */
	for (TestCasesVectorIterator itTestCase = m_testCases.begin(); itTestCase != m_testCases.end(); ++itTestCase)
	{
		(*itTestCase)->deinitTestCaseGlobal();

		delete (*itTestCase);
	} /* for (all registered test case objects) */

	m_testCases.clear();

	if (m_sparse_bo != 0)
	{
		gl.deleteBuffers(1, &m_sparse_bo);

		m_sparse_bo = 0;
	}
}

/** Stub init method */
void BufferStorageTest::init()
{
	/* We cannot initialize the test case objects here as there are cases where there
	 * is no rendering context bound to the thread, when this method is called. */
}

/** Fills m_testCases with BufferStorageTestCase instances which implement the sub-cases
 *  for the second test described in the CTS_ARB_sparse_buffer test specification
 **/
void BufferStorageTest::initTestCases()
{
	const glw::Functions& gl		= m_context.getRenderContext().getFunctions();
	glw::GLint			  page_size = 0;

	/* Retrieve "sparse buffer" GL constant values and entry-point func ptrs */
	gl.getIntegerv(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &page_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_SPARSE_BUFFER_PAGE_SIZE_ARB pname");

	/* Initialize all test case objects:
	 *
	 * Test cases a1-a6 */
	m_testCases.push_back(new QuadsBufferStorageTestCase(
		gl, m_testCtx, page_size, QuadsBufferStorageTestCase::IBO_USAGE_NONE, false)); /* use_color_data */
	m_testCases.push_back(new QuadsBufferStorageTestCase(
		gl, m_testCtx, page_size, QuadsBufferStorageTestCase::IBO_USAGE_INDEXED_DRAW_CALL, false)); /* use_color_data */
	m_testCases.push_back(new QuadsBufferStorageTestCase(gl, m_testCtx, page_size,
														 QuadsBufferStorageTestCase::IBO_USAGE_INDEXED_RANGED_DRAW_CALL,
														 false)); /* use_color_data */
	m_testCases.push_back(new QuadsBufferStorageTestCase(
		gl, m_testCtx, page_size, QuadsBufferStorageTestCase::IBO_USAGE_INDEXED_DRAW_CALL, true)); /* use_color_data */
	m_testCases.push_back(new QuadsBufferStorageTestCase(gl, m_testCtx, page_size,
														 QuadsBufferStorageTestCase::IBO_USAGE_INDEXED_RANGED_DRAW_CALL,
														 true)); /* use_color_data */

	/* Test case b1 */
	m_testCases.push_back(
		new TransformFeedbackBufferStorageTestCase(gl, m_testCtx, page_size, true)); /* all_tf_pages_committed */

	/* Test case b2 */
	m_testCases.push_back(
		new TransformFeedbackBufferStorageTestCase(gl, m_testCtx, page_size, false)); /* all_tf_pages_committed */

	/* Test case c */
	m_testCases.push_back(new ClearOpsBufferStorageTestCase(gl, m_testCtx, page_size));

	/* Test case d */
	m_testCases.push_back(new InvalidateBufferStorageTestCase(gl, m_testCtx, page_size));

	/* Test case e */
	m_testCases.push_back(
		new AtomicCounterBufferStorageTestCase(gl, m_testCtx, page_size, false)); /* all_pages_committed */
	m_testCases.push_back(
		new AtomicCounterBufferStorageTestCase(gl, m_testCtx, page_size, true)); /* all_pages_committed */

	/* Test case f */
	m_testCases.push_back(new BufferTextureStorageTestCase(gl, m_context, m_testCtx, page_size));

	/* Test case g */
	m_testCases.push_back(new CopyOpsBufferStorageTestCase(gl, m_testCtx, page_size));

	/* Test case h */
	m_testCases.push_back(new IndirectDispatchBufferStorageTestCase(gl, m_testCtx, page_size));

	/* Test case i */
	m_testCases.push_back(new SSBOStorageTestCase(gl, m_testCtx, page_size));

	/* Test case j */
	m_testCases.push_back(new UniformBufferStorageTestCase(gl, m_testCtx, page_size));

	/* Test case k */
	m_testCases.push_back(new PixelPackBufferStorageTestCase(gl, m_testCtx, page_size));

	/* Test case l */
	m_testCases.push_back(new PixelUnpackBufferStorageTestCase(gl, m_testCtx, page_size));

	/* Test case m */
	m_testCases.push_back(new QueryBufferStorageTestCase(gl, m_testCtx, page_size));

	/* Initialize all test cases */
	for (TestCasesVectorIterator itTestCase = m_testCases.begin(); itTestCase != m_testCases.end(); ++itTestCase)
	{
		(*itTestCase)->initTestCaseGlobal();
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult BufferStorageTest::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Only execute if the implementation supports the GL_ARB_sparse_buffer extension */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_buffer"))
	{
		throw tcu::NotSupportedError("GL_ARB_sparse_buffer is not supported");
	}

	/* The buffer storage test cases require OpenGL 4.3 feature-set. */
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 3)))
	{
		throw tcu::NotSupportedError("GL_ARB_sparse_buffer conformance tests require OpenGL 4.3 core feature-set");
	}

	/* Register & initialize the test case objects */
	initTestCases();

	/* Iterate over all sparse BO flag combinations. We need to consider a total of 4 flags:
	 *
	 * - GL_CLIENT_STORAGE_BIT  (bit 0)
	 * - GL_DYNAMIC_STORAGE_BIT (bit 1)
	 * - GL_MAP_COHERENT_BIT    (bit 2)
	 * - GL_MAP_PERSISTENT_BIT  (bit 3)
	 *
	 *  GL_MAP_READ_BIT and GL_MAP_WRITE_BIT are excluded, since they are incompatible
	 *  with sparse buffers by definition.
	 *
	 *  GL_SPARSE_STORAGE_BIT_ARB is assumed to be always defined. Some of the combinations are invalid.
	 *  Such loop iterations will be skipped.
	 * */

	for (unsigned int n_flag_combination = 0; n_flag_combination < (1 << 4); ++n_flag_combination)
	{
		const glw::GLint flags = ((n_flag_combination & (1 << 0)) ? GL_CLIENT_STORAGE_BIT : 0) |
								 ((n_flag_combination & (1 << 1)) ? GL_DYNAMIC_STORAGE_BIT : 0) |
								 ((n_flag_combination & (1 << 2)) ? GL_MAP_COHERENT_BIT : 0) |
								 ((n_flag_combination & (1 << 3)) ? GL_MAP_PERSISTENT_BIT : 0) |
								 GL_SPARSE_STORAGE_BIT_ARB;

		if ((flags & GL_MAP_PERSISTENT_BIT) != 0)
		{
			if ((flags & GL_MAP_READ_BIT) == 0 && (flags & GL_MAP_WRITE_BIT) == 0)
			{
				continue;
			}
		}

		if (((flags & GL_MAP_COHERENT_BIT) != 0) && ((flags & GL_MAP_PERSISTENT_BIT) == 0))
		{
			continue;
		}

		/* Set up the sparse BO */
		gl.genBuffers(1, &m_sparse_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

		gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

		gl.bufferStorage(GL_ARRAY_BUFFER, 1024768 * 1024, /* as per test spec */
						 DE_NULL,						  /* data */
						 flags);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage() call failed.");

		for (TestCasesVectorIterator itTestCase = m_testCases.begin(); itTestCase != m_testCases.end(); ++itTestCase)
		{
			gl.bindBuffer(GL_ARRAY_BUFFER, m_sparse_bo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

			if (!(*itTestCase)->initTestCaseIteration(m_sparse_bo))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Test case [" << (*itTestCase)->getName()
								   << "] "
									  "has failed to initialize."
								   << tcu::TestLog::EndMessage;

				result = false;
				goto end;
			}

			if (!(*itTestCase)->execute(flags))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Test case [" << (*itTestCase)->getName()
								   << "] "
									  "has failed to execute correctly."
								   << tcu::TestLog::EndMessage;

				result = false;
			} /* if (!testCaseResult) */

			(*itTestCase)->deinitTestCaseIteration();
		} /* for (all added test cases) */

		/* Release the sparse BO */
		gl.deleteBuffers(1, &m_sparse_bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers() call failed.");

		m_sparse_bo = 0;
	}

end:
	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
SparseBufferTests::SparseBufferTests(deqp::Context& context)
	: TestCaseGroup(context, "sparse_buffer_tests", "Verify conformance of CTS_ARB_sparse_buffer implementation")
{
}

/** Initializes the test group contents. */
void SparseBufferTests::init()
{
	addChild(new BufferStorageTest(m_context));
	addChild(new NegativeTests(m_context));
	addChild(new PageSizeGetterTest(m_context));
}

} /* gl4cts namespace */
