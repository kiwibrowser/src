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
 * \file  gl4cDirectStateAccessBuffersTests.cpp
 * \brief Conformance tests for the Direct State Access feature functionality (Buffer access part).
 */ /*-----------------------------------------------------------------------------------------------------------*/

/* Includes. */
#include "gl4cDirectStateAccessTests.hpp"

#include "deSharedPtr.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "gluStrUtil.hpp"

#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

#include "glw.h"
#include "glwFunctions.hpp"

#include <algorithm>
#include <climits>
#include <set>
#include <sstream>
#include <stack>
#include <string>

namespace gl4cts
{
namespace DirectStateAccess
{
namespace Buffers
{
/******************************** Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_creation", "Buffer Objects Creation Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Buffers' objects */
	static const glw::GLuint buffers_count = 2;

	glw::GLuint buffers_legacy[buffers_count] = {};
	glw::GLuint buffers_dsa[buffers_count]	= {};

	try
	{
		/* Check legacy state creation. */
		gl.genBuffers(buffers_count, buffers_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

		for (glw::GLuint i = 0; i < buffers_count; ++i)
		{
			if (gl.isBuffer(buffers_legacy[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GenBuffers has created default objects, but it should create only a names."
					<< tcu::TestLog::EndMessage;
			}
		}

		/* Check direct state creation. */
		gl.createBuffers(buffers_count, buffers_dsa);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers have failed");

		for (glw::GLuint i = 0; i < buffers_count; ++i)
		{
			if (!gl.isBuffer(buffers_dsa[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "CreateBuffers has not created default objects."
													<< tcu::TestLog::EndMessage;
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	for (glw::GLuint i = 0; i < buffers_count; ++i)
	{
		if (buffers_legacy[i])
		{
			gl.deleteBuffers(1, &buffers_legacy[i]);

			buffers_legacy[i] = 0;
		}

		if (buffers_dsa[i])
		{
			gl.deleteBuffers(1, &buffers_dsa[i]);

			buffers_dsa[i] = 0;
		}
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Data Test Implementation   ********************************/

/** @brief Data Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
DataTest::DataTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_data", "Buffer Objects Data Test")
	, m_pNamedBufferData(DE_NULL)
	, m_pNamedBufferSubData(DE_NULL)
	, m_pNamedBufferStorage(DE_NULL)
	, m_pCopyNamedBufferSubData(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Data Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult DataTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	m_pNamedBufferData		  = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pNamedBufferSubData	 = (PFNGLNAMEDBUFFERSUBDATA)gl.namedBufferSubData;
	m_pNamedBufferStorage	 = (PFNGLNAMEDBUFFERSTORAGE)gl.namedBufferStorage;
	m_pCopyNamedBufferSubData = (PFNGLCOPYNAMEDBUFFERSUBDATA)gl.copyNamedBufferSubData;

	try
	{
		if ((DE_NULL == m_pNamedBufferData) || (DE_NULL == m_pNamedBufferSubData) ||
			(DE_NULL == m_pNamedBufferStorage) || (DE_NULL == m_pCopyNamedBufferSubData))
		{
			throw 0;
		}

		/* BufferData tests */
		static const glw::GLenum hints[] = { GL_STREAM_DRAW,  GL_STREAM_READ,  GL_STREAM_COPY,
											 GL_STATIC_DRAW,  GL_STATIC_READ,  GL_STATIC_COPY,
											 GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, GL_DYNAMIC_COPY };
		static const glw::GLuint hints_count = sizeof(hints) / sizeof(hints[0]);

		for (glw::GLuint i = 0; i < hints_count; ++i)
		{
			is_ok &= TestCase(&DataTest::UploadUsingNamedBufferData, hints[i]);
			is_ok &= TestCase(&DataTest::UploadUsingNamedBufferSubData, hints[i]);
			is_ok &= TestCase(&DataTest::UploadUsingCopyNamedBufferSubData, hints[i]);
		}

		/* BufferStorage Tests */
		static const glw::GLenum bits[] = { GL_MAP_READ_BIT | GL_DYNAMIC_STORAGE_BIT,
											GL_MAP_READ_BIT | GL_MAP_WRITE_BIT, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT,
											GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT,
											GL_MAP_READ_BIT | GL_CLIENT_STORAGE_BIT };
		static const glw::GLuint bits_count = sizeof(bits) / sizeof(bits[0]);

		for (glw::GLuint i = 0; i < bits_count; ++i)
		{
			is_ok &= TestCase(&DataTest::UploadUsingNamedBufferStorage, bits[i]);
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Data uploading test case function.
 *
 *  @param [in] UploadDataFunction      Function pointer to the tested data uploading function.
 *  @param [in] parameter               Storage Parameter to be used with the function (function dependent).
 *
 *  @return True if test case succeeded, false otherwise.
 */
bool DataTest::TestCase(void (DataTest::*UploadDataFunction)(glw::GLuint, glw::GLenum), glw::GLenum parameter)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint buffer   = 0;
	bool		is_ok	= true;
	bool		is_error = false;

	try
	{
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		(this->*UploadDataFunction)(buffer, parameter);

		gl.bindBuffer(GL_ARRAY_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer failed.");

		glw::GLuint* data = (glw::GLuint*)gl.mapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer failed.");

		is_ok = compare(data, s_reference, s_reference_count);

		if (!is_ok)
		{
			LogFail(UploadDataFunction, parameter, data, s_reference, s_reference_count);
		}

		gl.unmapBuffer(GL_ARRAY_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer failed.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;

		LogError(UploadDataFunction, parameter);
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers failed.");
	}

	if (is_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief NamedBufferData data upload function.
 *
 *  @param [in] id                      Buffer id to be uploaded.
 *  @param [in] parameter               Storage Parameter to be used with the function, one of:
 *                                       -  GL_STREAM_DRAW,
 *                                       -  GL_STREAM_READ,
 *                                       -  GL_STREAM_COPY,
 *                                       -  GL_STATIC_DRAW,
 *                                       -  GL_STATIC_READ,
 *                                       -  GL_STATIC_COPY,
 *                                       -  GL_DYNAMIC_DRAW,
 *                                       -  GL_DYNAMIC_READ and
 *                                       -  GL_DYNAMIC_COPY.
 */
void DataTest::UploadUsingNamedBufferData(glw::GLuint id, glw::GLenum parameter)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_pNamedBufferData(id, s_reference_size, s_reference, parameter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");
}

/** @brief NamedBufferSubData data upload function.
 *
 *  @param [in] id                      Buffer id to be uploaded.
 *  @param [in] parameter               Storage parameter to be used with the NamedBufferData for
 *                                      the storage allocation (before call to NamedBufferSubData), one of:
 *                                       -  GL_STREAM_DRAW,
 *                                       -  GL_STREAM_READ,
 *                                       -  GL_STREAM_COPY,
 *                                       -  GL_STATIC_DRAW,
 *                                       -  GL_STATIC_READ,
 *                                       -  GL_STATIC_COPY,
 *                                       -  GL_DYNAMIC_DRAW,
 *                                       -  GL_DYNAMIC_READ and
 *                                       -  GL_DYNAMIC_COPY.
 */
void DataTest::UploadUsingNamedBufferSubData(glw::GLuint id, glw::GLenum parameter)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_pNamedBufferData(id, s_reference_size, DE_NULL, parameter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");

	m_pNamedBufferSubData(id, 0, s_reference_size, s_reference);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferSubData failed.");
}

/** @brief NamedBufferStorage data upload function.
 *
 *  @param [in] id                      Buffer id to be uploaded.
 *  @param [in] parameter               Storage Parameter to be used with the function, one of:
 *                                       - GL_MAP_READ_BIT | GL_DYNAMIC_STORAGE_BIT,
 *                                       - GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
 *                                       - GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT,
 *                                       - GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT,
 *                                       - GL_MAP_READ_BIT | GL_CLIENT_STORAGE_BIT
 */
void DataTest::UploadUsingNamedBufferStorage(glw::GLuint id, glw::GLenum parameter)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_pNamedBufferStorage(id, s_reference_size, s_reference, parameter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferStorage failed.");
}

/** @brief CopyNamedBufferSubData data upload function (uses auxiliary buffer object).
 *
 *  @param [in] id                      Buffer id to be uploaded.
 *  @param [in] parameter               Storage parameter to be used with the NamedBufferData for
 *                                      the auxiliary buffer object storage allocation
 *                                      (before call to CopyNamedBufferSubData), one of:
 *                                       -  GL_STREAM_DRAW,
 *                                       -  GL_STREAM_READ,
 *                                       -  GL_STREAM_COPY,
 *                                       -  GL_STATIC_DRAW,
 *                                       -  GL_STATIC_READ,
 *                                       -  GL_STATIC_COPY,
 *                                       -  GL_DYNAMIC_DRAW,
 *                                       -  GL_DYNAMIC_READ and
 *                                       -  GL_DYNAMIC_COPY.
 */
void DataTest::UploadUsingCopyNamedBufferSubData(glw::GLuint id, glw::GLenum parameter)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_pNamedBufferData(id, s_reference_size, DE_NULL, parameter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");

	glw::GLuint auxiliary_buffer	   = 0;
	bool		auxiliary_buffer_is_ok = true;

	try
	{
		gl.genBuffers(1, &auxiliary_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers failed.");

		gl.bindBuffer(GL_ARRAY_BUFFER, auxiliary_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer failed.");

		gl.bufferData(GL_ARRAY_BUFFER, s_reference_size, s_reference, parameter);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData failed.");

		m_pCopyNamedBufferSubData(auxiliary_buffer, id, 0, 0, s_reference_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCopyNamedBufferSubData failed.");
	}
	catch (...)
	{
		auxiliary_buffer_is_ok = false;
	}

	if (auxiliary_buffer)
	{
		gl.deleteBuffers(1, &auxiliary_buffer);
	}

	if (!auxiliary_buffer_is_ok)
	{
		throw 0;
	}
}

/** @brief Compare two unsigned integer arrays.
 *
 *  @param [in] data                    Data to be compared.
 *  @param [in] reference               Reference data to be compared to.
 *  @param [in] count                   Number of elements to be compared.
 *
 *  @return True if count of data the elements are equal to reference counterparts, false otherwise.
 */
bool DataTest::compare(const glw::GLuint* data, const glw::GLuint* reference, const glw::GLsizei count)
{
	for (glw::GLsizei i = 0; i < count; ++i)
	{
		if (data[i] != reference[i])
		{
			return false;
		}
	}
	return true;
}

/** @brief Prepare error message and log it.
 *
 *  @param [in] UploadDataFunction      Upload function pointer which have failed, one of:
 *                                       -  DataTest::UploadUsingNamedBufferData,
 *                                       -  DataTest::UploadUsingNamedBufferSubData
 *                                       -  DataTest::UploadUsingNamedBufferStorage and
 *                                       -  DataTest::UploadUsingCopyNamedBufferSubData.
 *  @param [in] parameter               Parameter which was passed to function.
 *  @param [in] data                    Data which was downloaded.
 *  @param [in] reference               Reference data.
 *  @param [in] count                   Number of elements compared.
 */
void DataTest::LogFail(void (DataTest::*UploadDataFunction)(glw::GLuint, glw::GLenum), glw::GLenum parameter,
					   const glw::GLuint* data, const glw::GLuint* reference, const glw::GLsizei count)
{
	std::string the_log = "The test of ";

	if (UploadDataFunction == &DataTest::UploadUsingNamedBufferData)
	{
		the_log.append("glNamedBufferData");
	}
	else
	{
		if (UploadDataFunction == &DataTest::UploadUsingNamedBufferSubData)
		{
			the_log.append("glNamedBufferSubData");
		}
		else
		{
			if (UploadDataFunction == &DataTest::UploadUsingNamedBufferStorage)
			{
				the_log.append("glNamedBufferStorage");
			}
			else
			{
				if (UploadDataFunction == &DataTest::UploadUsingCopyNamedBufferSubData)
				{
					the_log.append("glCopyNamedBufferSubData");
				}
				else
				{
					the_log.append("uknown upload function");
				}
			}
		}
	}

	if (UploadDataFunction == &DataTest::UploadUsingNamedBufferStorage)
	{
		the_log.append(" called with usage parameter ");

		std::stringstream bitfield_string_stream;
		bitfield_string_stream << glu::getBufferMapFlagsStr(parameter);
		the_log.append(bitfield_string_stream.str());
	}
	else
	{
		the_log.append(" called with usage parameter ");
		the_log.append(glu::getUsageName(parameter));
	}
	the_log.append(". Buffer data is equal to [");

	for (glw::GLsizei i = 0; i < count; ++i)
	{
		std::stringstream number;

		number << data[i];

		the_log.append(number.str());

		if (i != count - 1)
		{
			the_log.append(", ");
		}
	}

	the_log.append("], but [");

	for (glw::GLsizei i = 0; i < count; ++i)
	{
		std::stringstream number;

		number << reference[i];

		the_log.append(number.str());

		if (i != count - 1)
		{
			the_log.append(", ");
		}
	}

	the_log.append("] was expected.");

	m_context.getTestContext().getLog() << tcu::TestLog::Message << the_log << tcu::TestLog::EndMessage;
}

void DataTest::LogError(void (DataTest::*UploadDataFunction)(glw::GLuint, glw::GLenum), glw::GLenum parameter)
{
	std::string the_log = "Unexpected error occurred during the test of ";

	if (UploadDataFunction == &DataTest::UploadUsingNamedBufferData)
	{
		the_log.append("glNamedBufferData");
	}
	else
	{
		if (UploadDataFunction == &DataTest::UploadUsingNamedBufferSubData)
		{
			the_log.append("glNamedBufferSubData");
		}
		else
		{
			if (UploadDataFunction == &DataTest::UploadUsingNamedBufferStorage)
			{
				the_log.append("glNamedBufferStorage");
			}
			else
			{
				if (UploadDataFunction == &DataTest::UploadUsingCopyNamedBufferSubData)
				{
					the_log.append("glCopyNamedBufferSubData");
				}
				else
				{
					the_log.append("uknown upload function");
				}
			}
		}
	}

	if (UploadDataFunction == &DataTest::UploadUsingNamedBufferStorage)
	{
		the_log.append(" called with usage parameter ");

		std::stringstream bitfield_string_stream;
		bitfield_string_stream << glu::getBufferMapFlagsStr(parameter);
		the_log.append(bitfield_string_stream.str());
	}
	else
	{
		the_log.append(" called with usage parameter ");
		the_log.append(glu::getUsageName(parameter));
	}
	the_log.append(".");

	m_context.getTestContext().getLog() << tcu::TestLog::Message << the_log << tcu::TestLog::EndMessage;
}

const glw::GLuint DataTest::s_reference[] = {
	0, 1, 2, 4, 8, 16, 64, 128, 256, 512, 1024, 2048, 4096
};																	 //!< Reference data.
const glw::GLsizei DataTest::s_reference_size = sizeof(s_reference); //!< Size of the reference data.
const glw::GLsizei DataTest::s_reference_count =
	s_reference_size / sizeof(s_reference[0]); //!< NUmber of elements of the reference data.

/******************************** Clear Test Implementation   ********************************/

/** @brief Data Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ClearTest::ClearTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_clear", "Buffer Objects Clear Test")
	, m_pNamedBufferData(DE_NULL)
	, m_pClearNamedBufferData(DE_NULL)
	, m_pClearNamedBufferSubData(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief ClearNamedBufferData wrapper implementation.
 *
 *  @note USE_SUB_DATA == false, so ClearNamedBufferData will be used.
 *
 *  @param [in] buffer                  ID of the buffer to be cleared.
 *  @param [in] internalformat          GL internal format for clearing, one of the listed in test class description.
 *  @param [in] size                    Size of the data.
 *  @param [in] format                  GL Format of the data.
 *  @param [in] type                    GL Type of the data element.
 *  @param [in] data                    Data to be cleared with.
 */
template <>
void ClearTest::ClearNamedBuffer<false>(glw::GLuint buffer, glw::GLenum internalformat, glw::GLsizei size,
										glw::GLenum format, glw::GLenum type, glw::GLvoid* data)
{
	(void)size;
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_pClearNamedBufferData(buffer, internalformat, format, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearNamedBufferData failed.");
}

/** @brief ClearNamedBufferSubData wrapper implementation.
 *
 *  @note USE_SUB_DATA == true, so ClearNamedBufferSubData will be used.
 *
 *  @param [in] buffer                  ID of the buffer to be cleared.
 *  @param [in] internalformat          GL internal format for clearing, one of the listed in test class description.
 *  @param [in] size                    Size of the data.
 *  @param [in] format                  GL Format of the data.
 *  @param [in] type                    GL Type of the data element.
 *  @param [in] data                    Data to be cleared with.
 */
template <>
void ClearTest::ClearNamedBuffer<true>(glw::GLuint buffer, glw::GLenum internalformat, glw::GLsizei size,
									   glw::GLenum format, glw::GLenum type, glw::GLvoid* data)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_pClearNamedBufferSubData(buffer, internalformat, 0, size, format, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearNamedBufferData failed.");
}

/** @brief Compare two arrays with elements of type T == GLfloat (specialized).
 *
 *  @param [in] data                    Data to be compared.
 *  @param [in] reference               Reference data to be compared to.
 *  @param [in] count                   Number of elements to be compared.
 *
 *  @return True if count of data the elements are equal to reference counterparts, false otherwise.
 */
template <>
bool ClearTest::Compare<glw::GLfloat>(const glw::GLfloat* data, const glw::GLfloat* reference, const glw::GLsizei count)
{
	for (glw::GLsizei i = 0; i < count; ++i)
	{
		if (de::abs(data[i] - reference[i]) > 0.00001 /* Precision. */)
		{
			return false;
		}
	}
	return true;
}

/** @brief Compare two arrays with elements of type T.
 *
 *  @tparam     T                       Type of data to be compared (anything which is not GLfloat.
 *                                      Floating point numbers have another specialized implementation,
 *                                      which accounts the precision issues.
 *
 *  @param [in] data                    Data to be compared.
 *  @param [in] reference               Reference data to be compared to.
 *  @param [in] count                   Number of elements to be compared.
 *
 *  @return True if count of data the elements are equal to reference counterparts, false otherwise.
 */
template <typename T>
bool ClearTest::Compare(const T* data, const T* reference, const glw::GLsizei count)
{
	for (glw::GLsizei i = 0; i < count; ++i)
	{
		if (data[i] != reference[i])
		{
			return false;
		}
	}
	return true;
}

/** @brief Prepare error message and log it.
 *
 *  @tparam     T                       Type of data to which was tested.
 *
 *  @param [in] internalformat          Internal format used for clearing, one of the listed in test class description.
 *  @param [in] data                    Data which was used for clear test.
 *  @param [in] reference               Reference data.
 *  @param [in] count                   Number of elements to be compared.
 */
template <typename T>
void ClearTest::LogFail(bool use_sub_data, glw::GLenum internalformat, const T* data, const T* reference,
						const glw::GLsizei count)
{
	(void)internalformat;
	std::string the_log = "The test of ";

	if (use_sub_data)
	{
		the_log.append("ClearNamedBufferSubData has failed for internalformat ");
	}
	else
	{
		the_log.append("ClearNamedBufferData has failed for internalformat ");
	}

	//the_log.append(glu::getPixelFormatName(internalformat));
	the_log.append(". Cleared buffer data is equal to [");

	for (glw::GLsizei i = 0; i < count; ++i)
	{
		std::stringstream number;

		number << data[i];

		the_log.append(number.str());

		if (i != count - 1)
		{
			the_log.append(", ");
		}
	}

	the_log.append("], but [");

	for (glw::GLsizei i = 0; i < count; ++i)
	{
		std::stringstream number;

		number << reference[i];

		the_log.append(number.str());

		if (i != count - 1)
		{
			the_log.append(", ");
		}
	}

	the_log.append("] was expected.");

	m_context.getTestContext().getLog() << tcu::TestLog::Message << the_log << tcu::TestLog::EndMessage;
}

void ClearTest::LogError(bool use_sub_data, glw::GLenum internalformat)
{
	(void)internalformat;
	std::string the_log = "Unexpected error occurred during Test of ";

	if (use_sub_data)
	{
		the_log.append("ClearNamedBufferSubData with internalformat ");
	}
	else
	{
		the_log.append("ClearNamedBufferData with internalformat ");
	}

	//the_log.append(glu::getPixelFormatName(internalformat));

	m_context.getTestContext().getLog() << tcu::TestLog::Message << the_log << tcu::TestLog::EndMessage;
}

/** @brief Run CLearing test case.
 *
 *  @tparam     T                       Type of data to which to be tested.
 *  @tparam     USE_SUB_DATA            If true ClearNamedBufferSubData will be used, ClearNamedBufferData otherwise.
 *
 *  @param [in] internalformat          Internal format used for clearing, one of the listed in test class description.
 *  @param [in] count                   Number of elements.
 *  @param [in] internalformat          Data format to be used for clearing.
 *  @param [in] data                    Data to be used with clear test.
 *
 *  @return True if test case succeeded, false otherwise.
 */
template <typename T, bool USE_SUB_DATA>
bool ClearTest::TestClearNamedBufferData(glw::GLenum internalformat, glw::GLsizei count, glw::GLenum format,
										 glw::GLenum type, T* data)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint buffer   = 0;
	bool		is_ok	= true;
	bool		is_error = false;

	try
	{
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferData(buffer, static_cast<glw::GLsizei>(count * sizeof(T)), NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");

		ClearNamedBuffer<USE_SUB_DATA>(buffer, internalformat, static_cast<glw::GLsizei>(count * sizeof(T)), format,
									   type, data);

		gl.bindBuffer(GL_ARRAY_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer failed.");

		T* _data = (T*)gl.mapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer failed.");

		is_ok = Compare<T>(_data, data, count);

		if (!is_ok)
		{
			/* Log. */
			LogFail<T>(USE_SUB_DATA, internalformat, _data, data, count);
		}

		gl.unmapBuffer(GL_ARRAY_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer failed.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;

		LogError(USE_SUB_DATA, internalformat);
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers failed.");
	}

	if (is_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Iterate Data Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ClearTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	m_pNamedBufferData		   = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pClearNamedBufferData	= (PFNGLCLEARNAMEDBUFFERDATA)gl.clearNamedBufferData;
	m_pClearNamedBufferSubData = (PFNGLCLEARNAMEDBUFFERSUBDATA)gl.clearNamedBufferSubData;

	try
	{
		if ((DE_NULL == m_pNamedBufferData) || (DE_NULL == m_pClearNamedBufferData) ||
			(DE_NULL == m_pClearNamedBufferSubData))
		{
			throw 0;
		}

		{
			/* unsigned byte norm component ClearNamedBufferData tests */
			glw::GLubyte reference[4] = { 5, 1, 2, 3 };

			is_ok &= TestClearNamedBufferData<glw::GLubyte, false>(GL_R8, 1, GL_RED, GL_UNSIGNED_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLubyte, false>(GL_RG8, 2, GL_RG, GL_UNSIGNED_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLubyte, false>(GL_RGBA8, 4, GL_RGBA, GL_UNSIGNED_BYTE, reference);

			/* unsigned byte norm component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLubyte, true>(GL_R8, 1, GL_RED, GL_UNSIGNED_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLubyte, true>(GL_RG8, 2, GL_RG, GL_UNSIGNED_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLubyte, true>(GL_RGBA8, 4, GL_RGBA, GL_UNSIGNED_BYTE, reference);

			/* unsigned byte component ClearNamedBufferData tests */
			is_ok &= TestClearNamedBufferData<glw::GLubyte, false>(GL_R8UI, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLubyte, false>(GL_RG8UI, 2, GL_RG_INTEGER, GL_UNSIGNED_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLubyte, false>(GL_RGBA8UI, 4, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, reference);

			/* unsigned byte component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLubyte, true>(GL_R8UI, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLubyte, true>(GL_RG8UI, 2, GL_RG_INTEGER, GL_UNSIGNED_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLubyte, true>(GL_RGBA8UI, 4, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, reference);
		}

		{
			/* signed byte component ClearNamedBufferData tests */
			glw::GLbyte reference[4] = { 5, 1, -2, 3 };

			is_ok &= TestClearNamedBufferData<glw::GLbyte, false>(GL_R8I, 1, GL_RED_INTEGER, GL_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLbyte, false>(GL_RG8I, 2, GL_RG_INTEGER, GL_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLbyte, false>(GL_RGBA8I, 4, GL_RGBA_INTEGER, GL_BYTE, reference);

			/* signed byte component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLbyte, true>(GL_R8I, 1, GL_RED_INTEGER, GL_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLbyte, true>(GL_RG8I, 2, GL_RG_INTEGER, GL_BYTE, reference);
			is_ok &= TestClearNamedBufferData<glw::GLbyte, true>(GL_RGBA8I, 4, GL_RGBA_INTEGER, GL_BYTE, reference);
		}

		{
			/* unsigned short norm component ClearNamedBufferData tests */
			glw::GLushort reference[4] = { 5, 1, 2, 3 };

			is_ok &= TestClearNamedBufferData<glw::GLushort, false>(GL_R16, 1, GL_RED, GL_UNSIGNED_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLushort, false>(GL_RG16, 2, GL_RG, GL_UNSIGNED_SHORT, reference);
			is_ok &=
				TestClearNamedBufferData<glw::GLushort, false>(GL_RGBA16, 4, GL_RGBA, GL_UNSIGNED_SHORT, reference);

			/* unsigned short norm component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLushort, true>(GL_R16, 1, GL_RED, GL_UNSIGNED_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLushort, true>(GL_RG16, 2, GL_RG, GL_UNSIGNED_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLushort, true>(GL_RGBA16, 4, GL_RGBA, GL_UNSIGNED_SHORT, reference);

			/* unsigned short component ClearNamedBufferData tests */
			is_ok &= TestClearNamedBufferData<glw::GLushort, false>(GL_R16UI, 1, GL_RED_INTEGER, GL_UNSIGNED_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLushort, false>(GL_RG16UI, 2, GL_RG_INTEGER, GL_UNSIGNED_SHORT, reference);
			is_ok &=
				TestClearNamedBufferData<glw::GLushort, false>(GL_RGBA16UI, 4, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, reference);

			/* unsigned short component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLushort, true>(GL_R16UI, 1, GL_RED_INTEGER, GL_UNSIGNED_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLushort, true>(GL_RG16UI, 2, GL_RG_INTEGER, GL_UNSIGNED_SHORT, reference);
			is_ok &=
				TestClearNamedBufferData<glw::GLushort, true>(GL_RGBA16UI, 4, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, reference);
		}

		{
			/* signed short component ClearNamedBufferData tests */
			glw::GLshort reference[4] = { 5, 1, -2, 3 };

			is_ok &= TestClearNamedBufferData<glw::GLshort, false>(GL_R16I, 1, GL_RED_INTEGER, GL_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLshort, false>(GL_RG16I, 2, GL_RG_INTEGER, GL_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLshort, false>(GL_RGBA16I, 4, GL_RGBA_INTEGER, GL_SHORT, reference);

			/* signed short component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLshort, true>(GL_R16I, 1, GL_RED_INTEGER, GL_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLshort, true>(GL_RG16I, 2, GL_RG_INTEGER, GL_SHORT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLshort, true>(GL_RGBA16I, 4, GL_RGBA_INTEGER, GL_SHORT, reference);
		}

		{
			/* unsigned int component ClearNamedBufferData tests */
			glw::GLuint reference[4] = { 5, 1, 2, 3 };

			is_ok &= TestClearNamedBufferData<glw::GLuint, false>(GL_R32UI, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLuint, false>(GL_RG32UI, 2, GL_RG_INTEGER, GL_UNSIGNED_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLuint, false>(GL_RGB32UI, 3, GL_RGB_INTEGER, GL_UNSIGNED_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLuint, false>(GL_RGBA32UI, 4, GL_RGBA_INTEGER, GL_UNSIGNED_INT, reference);

			/* unsigned int component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLuint, true>(GL_R32UI, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLuint, true>(GL_RG32UI, 2, GL_RG_INTEGER, GL_UNSIGNED_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLuint, true>(GL_RGB32UI, 3, GL_RGB_INTEGER, GL_UNSIGNED_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLuint, true>(GL_RGBA32UI, 4, GL_RGBA_INTEGER, GL_UNSIGNED_INT, reference);
		}

		{
			/* signed int component ClearNamedBufferData tests */
			glw::GLint reference[4] = { 5, 1, -2, 3 };

			is_ok &= TestClearNamedBufferData<glw::GLint, false>(GL_R32I, 1, GL_RED_INTEGER, GL_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLint, false>(GL_RG32I, 2, GL_RG_INTEGER, GL_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLint, false>(GL_RGB32I, 3, GL_RGB_INTEGER, GL_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLint, false>(GL_RGBA32I, 4, GL_RGBA_INTEGER, GL_INT, reference);

			/* signed int component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLint, true>(GL_R32I, 1, GL_RED_INTEGER, GL_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLint, true>(GL_RG32I, 2, GL_RG_INTEGER, GL_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLint, true>(GL_RGB32I, 3, GL_RGB_INTEGER, GL_INT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLint, true>(GL_RGBA32I, 4, GL_RGBA_INTEGER, GL_INT, reference);
		}

		{
			/* half float component ClearNamedBufferData tests */
			glw::GLhalf reference[4] = { 0x3C00 /* 1.0hf */, 0x0000 /* 0.0hf */, 0xC000 /* -2.0hf */,
										 0x3555 /* 0.333333333hf */ };

			is_ok &= TestClearNamedBufferData<glw::GLhalf, false>(GL_R16F, 1, GL_RED, GL_HALF_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLhalf, false>(GL_RG16F, 2, GL_RG, GL_HALF_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLhalf, false>(GL_RGBA16F, 4, GL_RGBA, GL_HALF_FLOAT, reference);

			/* half float component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLhalf, true>(GL_R16F, 1, GL_RED, GL_HALF_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLhalf, true>(GL_RG16F, 2, GL_RG, GL_HALF_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLhalf, true>(GL_RGBA16F, 4, GL_RGBA, GL_HALF_FLOAT, reference);
		}

		{
			/* float component ClearNamedBufferData tests */
			glw::GLfloat reference[4] = { 1.f, 0.f, -2.f, 0.3333333333f };

			is_ok &= TestClearNamedBufferData<glw::GLfloat, false>(GL_R32F, 1, GL_RED, GL_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLfloat, false>(GL_RG32F, 2, GL_RG, GL_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLfloat, false>(GL_RGB32F, 3, GL_RGB, GL_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLfloat, false>(GL_RGBA32F, 4, GL_RGBA, GL_FLOAT, reference);

			/* float component ClearNamedBufferSubData tests */
			is_ok &= TestClearNamedBufferData<glw::GLfloat, true>(GL_R32F, 1, GL_RED, GL_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLfloat, true>(GL_RG32F, 2, GL_RG, GL_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLfloat, true>(GL_RGB32F, 3, GL_RGB, GL_FLOAT, reference);
			is_ok &= TestClearNamedBufferData<glw::GLfloat, true>(GL_RGBA32F, 4, GL_RGBA, GL_FLOAT, reference);
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Map Read Only Test Implementation   ********************************/

/** @brief Map Read Only Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
MapReadOnlyTest::MapReadOnlyTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_map_read_only", "Buffer Objects Map Read Only Test")
	, m_pNamedBufferData(DE_NULL)
	, m_pMapNamedBuffer(DE_NULL)
	, m_pUnmapNamedBuffer(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Map Read Only Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult MapReadOnlyTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint buffer = 0;

	m_pNamedBufferData  = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pMapNamedBuffer   = (PFNGLMAPNAMEDBUFFER)gl.mapNamedBuffer;
	m_pUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFER)gl.unmapNamedBuffer;

	try
	{
		if ((DE_NULL == m_pNamedBufferData) || (DE_NULL == m_pMapNamedBuffer) || (DE_NULL == m_pUnmapNamedBuffer))
		{
			throw 0;
		}

		/* Buffer creation. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		/* Buffer's storage allocation and reference data upload. */
		m_pNamedBufferData(buffer, s_reference_size, s_reference, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");

		/* Mapping with new named buffer map function. */
		glw::GLuint* data = (glw::GLuint*)m_pMapNamedBuffer(buffer, GL_READ_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

		if (DE_NULL == data)
		{
			/* Log. */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glMapNamedBuffer returned NULL pointer, but buffer's data was expected."
				<< tcu::TestLog::EndMessage;
		}
		else
		{
			/* Comparison results with reference data. */
			for (glw::GLsizei i = 0; i < s_reference_count; ++i)
			{
				is_ok &= (data[i] == s_reference[i]);
			}

			if (!is_ok)
			{
				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glMapNamedBuffer returned pointer to data which is not identical to reference data."
					<< tcu::TestLog::EndMessage;
			}

			/* Unmapping with new named buffer unmap function. */
			if (GL_TRUE != m_pUnmapNamedBuffer(buffer))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glUnmapNamedBuffer called on mapped buffer has returned GL_FALSE, but GL_TRUE was expected."
					<< tcu::TestLog::EndMessage;
			}
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = false;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

const glw::GLuint  MapReadOnlyTest::s_reference[]	 = { 0, 1, 2, 4, 8, 16, 64, 128, 256, 512, 1024, 2048, 4096 };
const glw::GLsizei MapReadOnlyTest::s_reference_size  = sizeof(s_reference);
const glw::GLsizei MapReadOnlyTest::s_reference_count = s_reference_size / sizeof(s_reference[0]);

/******************************** Map Read Write Test Implementation   ********************************/

/** @brief Map Read Write Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
MapReadWriteTest::MapReadWriteTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_map_read_write", "Buffer Objects Map Read Write Test")
	, m_pNamedBufferData(DE_NULL)
	, m_pMapNamedBuffer(DE_NULL)
	, m_pUnmapNamedBuffer(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Map Read Write Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult MapReadWriteTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint buffer = 0;

	m_pNamedBufferData  = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pMapNamedBuffer   = (PFNGLMAPNAMEDBUFFER)gl.mapNamedBuffer;
	m_pUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFER)gl.unmapNamedBuffer;

	try
	{
		if ((DE_NULL == m_pNamedBufferData) || (DE_NULL == m_pMapNamedBuffer) || (DE_NULL == m_pUnmapNamedBuffer))
		{
			throw 0;
		}

		/* Buffer creation. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		/* Buffer's storage allocation and reference data upload. */
		m_pNamedBufferData(buffer, s_reference_size, s_reference, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");

		/* Mapping with new named buffer map function. */
		glw::GLuint* data = (glw::GLuint*)m_pMapNamedBuffer(buffer, GL_READ_WRITE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

		if (DE_NULL == data)
		{
			/* Log. */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glMapNamedBuffer returned NULL pointer, but buffer's data was expected."
				<< tcu::TestLog::EndMessage;
		}
		else
		{
			/* Comparison results with reference data. */
			for (glw::GLsizei i = 0; i < s_reference_count; ++i)
			{
				is_ok &= (data[i] == s_reference[i]);
			}

			if (!is_ok)
			{
				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glMapNamedBuffer returned pointer to data which is not identical to reference data."
					<< tcu::TestLog::EndMessage;
			}

			/* Writting inverted reference data. */
			for (glw::GLsizei i = 0; i < s_reference_count; ++i)
			{
				data[i] = s_reference[s_reference_count - i - 1];
			}

			/* Unmapping with new named buffer unmap function. */
			if (GL_TRUE != m_pUnmapNamedBuffer(buffer))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glUnmapNamedBuffer called on mapped buffer has returned GL_FALSE, but GL_TRUE was expected."
					<< tcu::TestLog::EndMessage;
			}
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");

			data = DE_NULL;

			data = (glw::GLuint*)m_pMapNamedBuffer(buffer, GL_READ_WRITE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			if (DE_NULL == data)
			{
				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glMapNamedBuffer returned NULL pointer, but buffer's data was expected."
					<< tcu::TestLog::EndMessage;
			}
			else
			{
				/* Comparison results with inverted reference data. */
				for (glw::GLsizei i = 0; i < s_reference_count; ++i)
				{
					is_ok &= (data[i] == s_reference[s_reference_count - i - 1]);
				}

				/* Unmapping with new named buffer unmap function. */
				if (GL_TRUE != m_pUnmapNamedBuffer(buffer))
				{
					is_ok = false;

					/* Log. */
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message
						<< "glUnmapNamedBuffer called on mapped buffer has returned GL_FALSE, but GL_TRUE was expected."
						<< tcu::TestLog::EndMessage;
				}
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = false;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

const glw::GLuint MapReadWriteTest::s_reference[] = {
	0, 1, 2, 4, 8, 16, 64, 128, 256, 512, 1024, 2048, 4096
};																			 //!< Reference data.
const glw::GLsizei MapReadWriteTest::s_reference_size = sizeof(s_reference); //!< Reference data size.
const glw::GLsizei MapReadWriteTest::s_reference_count =
	s_reference_size / sizeof(s_reference[0]); //!< Reference data elements' count.

/******************************** Map Write Only Test Implementation   ********************************/

/** @brief Map Write Only Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
MapWriteOnlyTest::MapWriteOnlyTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_map_write_only", "Buffer Objects Map Write Only Test")
	, m_pNamedBufferData(DE_NULL)
	, m_pMapNamedBuffer(DE_NULL)
	, m_pUnmapNamedBuffer(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Map Write Only Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult MapWriteOnlyTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint buffer = 0;

	m_pNamedBufferData  = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pMapNamedBuffer   = (PFNGLMAPNAMEDBUFFER)gl.mapNamedBuffer;
	m_pUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFER)gl.unmapNamedBuffer;

	try
	{
		if ((DE_NULL == m_pNamedBufferData) || (DE_NULL == m_pMapNamedBuffer) || (DE_NULL == m_pUnmapNamedBuffer))
		{
			throw 0;
		}

		/* Buffer creation. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		/* Buffer's storage allocation. */
		m_pNamedBufferData(buffer, s_reference_size, NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");

		/* Mapping with new named buffer map function. */
		glw::GLuint* data = (glw::GLuint*)m_pMapNamedBuffer(buffer, GL_WRITE_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

		if (DE_NULL == data)
		{
			/* Log. */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glMapNamedBuffer returned NULL pointer, but buffer's data was expected."
				<< tcu::TestLog::EndMessage;
		}
		else
		{
			/* Reference data upload. */
			for (glw::GLsizei i = 0; i < s_reference_count; ++i)
			{
				data[i] = s_reference[i];
			}

			/* Unmapping with new named buffer unmap function. */
			if (GL_TRUE != m_pUnmapNamedBuffer(buffer))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glUnmapNamedBuffer called on mapped buffer has returned GL_FALSE, but GL_TRUE was expected."
					<< tcu::TestLog::EndMessage;
			}
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");

			/* Mapping data, the old way. */
			gl.bindBuffer(GL_ARRAY_BUFFER, buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer failed.");

			data = DE_NULL;

			data = (glw::GLuint*)gl.mapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer failed.");

			/* Comparison results with reference data. */
			for (glw::GLsizei i = 0; i < s_reference_count; ++i)
			{
				is_ok &= (data[i] == s_reference[i]);
			}

			if (!is_ok)
			{
				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glMapNamedBuffer, called with GL_WRITE_ONLY access flag, had not stored the reference data."
					<< tcu::TestLog::EndMessage;
			}

			gl.unmapBuffer(GL_ARRAY_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer failed.");
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = false;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

const glw::GLuint MapWriteOnlyTest::s_reference[] = {
	0, 1, 2, 4, 8, 16, 64, 128, 256, 512, 1024, 2048, 4096
};																			 //!< Reference data.
const glw::GLsizei MapWriteOnlyTest::s_reference_size = sizeof(s_reference); //!< Reference data size.
const glw::GLsizei MapWriteOnlyTest::s_reference_count =
	s_reference_size / sizeof(s_reference[0]); //!< Reference data elements' count.

/******************************** Buffers Range Map Read Bit Test Implementation   ********************************/

/** @brief Buffers Range Map Read Bit Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
MapRangeReadBitTest::MapRangeReadBitTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_map_range_read_bit", "Buffer Objects Map Range Read Bit Test")
	, m_pNamedBufferStorage(DE_NULL)
	, m_pMapNamedBufferRange(DE_NULL)
	, m_pUnmapNamedBuffer(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Buffers Range Map Read Bit Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult MapRangeReadBitTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint buffer = 0;

	m_pNamedBufferStorage  = (PFNGLNAMEDBUFFERSTORAGE)gl.namedBufferStorage;
	m_pMapNamedBufferRange = (PFNGLMAPNAMEDBUFFERRANGE)gl.mapNamedBufferRange;
	m_pUnmapNamedBuffer	= (PFNGLUNMAPNAMEDBUFFER)gl.unmapNamedBuffer;

	try
	{
		if ((DE_NULL == m_pNamedBufferStorage) || (DE_NULL == m_pMapNamedBufferRange) ||
			(DE_NULL == m_pUnmapNamedBuffer))
		{
			throw 0;
		}

		glw::GLbitfield access_flags[] = { GL_MAP_READ_BIT, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT,
										   GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT };

		glw::GLuint access_flags_count = sizeof(access_flags) / sizeof(access_flags[0]);

		for (glw::GLuint i = 0; i < access_flags_count; ++i)
		{
			/* Buffer creation. */
			gl.createBuffers(1, &buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

			/* Buffer's storage allocation and reference data upload. */
			m_pNamedBufferStorage(buffer, s_reference_size, s_reference, access_flags[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");

			/* Mapping with first half of named buffer. */
			glw::GLuint* data = (glw::GLuint*)m_pMapNamedBufferRange(buffer, 0, s_reference_size / 2, access_flags[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBufferRange failed.");

			/* Check with reference. */
			is_ok &= CompareWithReference(data, 0, s_reference_size / 2);

			/* Unmapping with new named buffer unmap function. */
			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");

			/* Mapping with second half of named buffer. */
			data = (glw::GLuint*)m_pMapNamedBufferRange(buffer, s_reference_size / 2, s_reference_size / 2,
														access_flags[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBufferRange failed.");

			/* Check with reference. */
			is_ok &= CompareWithReference(data, s_reference_size / 2, s_reference_size / 2);

			/* Unmapping with new named buffer unmap function. */
			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");

			/* Clean up. */
			if (buffer)
			{
				gl.deleteBuffers(1, &buffer);

				buffer = 0;
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Compare array of unsigned integers with subrange of reference values (s_reference).
 *
 *  @param [in] data        Data to be compared.
 *  @param [in] offset      Offset in the reference data.
 *  @param [in] length      Length of the data to be compared.
 *
 *  @return True if comparison succeeded, false otherwise.
 */
bool MapRangeReadBitTest::CompareWithReference(glw::GLuint* data, glw::GLintptr offset, glw::GLsizei length)
{
	if (DE_NULL == data)
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glMapNamedBufferRange called with offset " << offset << " and length "
			<< length << " returned NULL pointer, but buffer's data was expected." << tcu::TestLog::EndMessage;
	}
	else
	{
		glw::GLuint start = static_cast<glw::GLuint>((offset) / sizeof(s_reference[0]));
		glw::GLuint end   = static_cast<glw::GLuint>((offset + length) / sizeof(s_reference[0]));

		/* Comparison results with reference data. */
		for (glw::GLuint i = start; i < end; ++i)
		{
#if (DE_COMPILER == DE_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
			if (data[i - start] != s_reference[i])
			{
				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "glMapNamedBufferRange called with offset " << offset << " and length "
					<< length << " returned pointer to data which is not identical to reference data."
					<< tcu::TestLog::EndMessage;

				return false;
			}
#if (DE_COMPILER == DE_COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
		}
	}

	return true;
}

const glw::GLuint MapRangeReadBitTest::s_reference[] = {
	1, 2, 4, 8, 16, 64, 128, 256, 512, 1024, 2048, 4096
};																				//!< Reference data.
const glw::GLsizei MapRangeReadBitTest::s_reference_size = sizeof(s_reference); //!< Reference data size.
const glw::GLsizei MapRangeReadBitTest::s_reference_count =
	s_reference_size / sizeof(s_reference[0]); //!< Reference data elements' count.

/******************************** Buffers Range Map Write Bit Test Implementation   ********************************/

/** @brief Buffers Range Map Write Bit Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
MapRangeWriteBitTest::MapRangeWriteBitTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_map_range_write_bit", "Buffer Objects Map Range Write Bit Test")
	, m_pNamedBufferStorage(DE_NULL)
	, m_pMapNamedBufferRange(DE_NULL)
	, m_pUnmapNamedBuffer(DE_NULL)
	, m_pFlushMappedNamedBufferRange(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Buffers Range Map Read Bit Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult MapRangeWriteBitTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint buffer = 0;

	m_pNamedBufferStorage		   = (PFNGLNAMEDBUFFERSTORAGE)gl.namedBufferStorage;
	m_pMapNamedBufferRange		   = (PFNGLMAPNAMEDBUFFERRANGE)gl.mapNamedBufferRange;
	m_pUnmapNamedBuffer			   = (PFNGLUNMAPNAMEDBUFFER)gl.unmapNamedBuffer;
	m_pFlushMappedNamedBufferRange = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE)gl.flushMappedNamedBufferRange;

	try
	{
		if ((DE_NULL == m_pNamedBufferStorage) || (DE_NULL == m_pMapNamedBufferRange) ||
			(DE_NULL == m_pUnmapNamedBuffer) || (DE_NULL == m_pFlushMappedNamedBufferRange))
		{
			throw 0;
		}

		struct
		{
			glw::GLbitfield creation;
			glw::GLbitfield first_mapping;
			glw::GLbitfield second_mapping;
		} access_flags[] = {
			{ GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_WRITE_BIT },
			{ GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT,
			  GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT },
			{ GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT, GL_MAP_WRITE_BIT },
			{ GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT,
			  GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT },
			{ GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT }
		};

		glw::GLuint access_flags_count = sizeof(access_flags) / sizeof(access_flags[0]);

		for (glw::GLuint i = 0; i < access_flags_count; ++i)
		{
			/* Buffer creation. */
			gl.createBuffers(1, &buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

			/* Buffer's storage allocation and reference data upload. */
			m_pNamedBufferStorage(buffer, s_reference_size, DE_NULL, access_flags[i].creation);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferStorage failed.");

			/* Mapping with first half of named buffer. */
			glw::GLuint* data =
				(glw::GLuint*)m_pMapNamedBufferRange(buffer, 0, s_reference_size / 2, access_flags[i].first_mapping);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBufferRange failed.");

			/* Write to mapped buffer. */
			for (glw::GLsizei j = 0; j < s_reference_count / 2; ++j)
			{
				data[j] = s_reference[j];
			}

			/* Flush, if needed. */
			glw::GLenum flush_error = GL_NO_ERROR;

			if (GL_MAP_FLUSH_EXPLICIT_BIT & access_flags[i].first_mapping)
			{
				m_pFlushMappedNamedBufferRange(buffer, 0, s_reference_size / 2);

				flush_error = gl.getError();
			}

			/* Unmapping with new named buffer unmap function. */
			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(flush_error, "glFlushMappedNamedBufferRange failed.");
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");

			/* Mapping with second half of named buffer. */
			data = (glw::GLuint*)m_pMapNamedBufferRange(buffer, s_reference_size / 2, s_reference_size / 2,
														access_flags[i].second_mapping);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBufferRange failed.");

			/* Write to mapped buffer. */
			for (glw::GLsizei j = 0; j < s_reference_count / 2; ++j)
			{
				data[j] = s_reference[j + s_reference_count / 2];
			}

			/* Flush, if needed. */
			flush_error = GL_NO_ERROR;

			if (GL_MAP_FLUSH_EXPLICIT_BIT & access_flags[i].second_mapping)
			{
				m_pFlushMappedNamedBufferRange(buffer, 0, s_reference_size / 2);

				flush_error = gl.getError();
			}

			/* Unmapping with new named buffer unmap function. */
			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(flush_error, "glFlushMappedNamedBufferRange failed.");
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");

			/* Check that previous mappings correctly filled buffer with reference data. */
			is_ok &= CompareWithReference(buffer, access_flags[i].first_mapping | access_flags[i].second_mapping);

			/* Clean up. */
			if (buffer)
			{
				gl.deleteBuffers(1, &buffer);

				buffer = 0;
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Compare buffer's content with the reference values (s_reference) and log possible failure.
 *
 *  @param [in] buffer          Buffer to be tested.
 *  @param [in] access_flag     Access flag used during test's mapping (for failure logging purposes).
 *
 *  @return True if comparison succeeded, false otherwise.
 */
bool MapRangeWriteBitTest::CompareWithReference(glw::GLuint buffer, glw::GLbitfield access_flag)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Map buffer with legacy API. */
	gl.bindBuffer(GL_ARRAY_BUFFER, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer failed.");

	glw::GLuint* data = (glw::GLuint*)gl.mapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer failed.");

	/* Default return value. */
	bool is_ok = true;

	if (DE_NULL != data)
	{
		/* Comparison results with reference data. */
		for (glw::GLsizei i = 0; i < s_reference_count; ++i)
		{
			if (data[i] != s_reference[i])
			{
				std::string access_string = "GL_MAP_WRITE_BIT";

				if (GL_MAP_INVALIDATE_RANGE_BIT & access_flag)
				{
					access_string = "(GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT)";
				}

				if (GL_MAP_INVALIDATE_BUFFER_BIT & access_flag)
				{
					access_string = "(GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)";
				}

				if (GL_MAP_FLUSH_EXPLICIT_BIT & access_flag)
				{
					access_string = "(GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT)";
				}

				if (GL_MAP_UNSYNCHRONIZED_BIT & access_flag)
				{
					access_string = "(GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT)";
				}

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Test of glMapNamedBufferRange with access flag " << access_string
					<< " failed to fill the buffer with reference data." << tcu::TestLog::EndMessage;

				is_ok = false;

				break;
			}
		}
	}

	/* Unmap buffer. */
	gl.unmapBuffer(GL_ARRAY_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer failed.");

	return is_ok;
}

const glw::GLuint MapRangeWriteBitTest::s_reference[] = {
	1, 2, 4, 8, 16, 64, 128, 256, 512, 1024, 2048, 4096
};																				 //!< Reference data.
const glw::GLsizei MapRangeWriteBitTest::s_reference_size = sizeof(s_reference); //!< Reference data size.
const glw::GLsizei MapRangeWriteBitTest::s_reference_count =
	s_reference_size / sizeof(s_reference[0]); //!< Reference data elements' count.

/******************************** Get Named Buffer SubData Query Test Implementation   ********************************/

/** @brief Get Named Buffer SubData Query Test's static constants. */
const glw::GLuint SubDataQueryTest::s_reference[] = {
	1, 2, 4, 8, 16, 64, 128, 256, 512, 1024, 2048, 4096
};																			 //!< Reference data.
const glw::GLsizei SubDataQueryTest::s_reference_size = sizeof(s_reference); //!< Reference data size.
const glw::GLsizei SubDataQueryTest::s_reference_count =
	s_reference_size / sizeof(s_reference[0]); //!< Reference data elements' count.

/** @brief Get Named Buffer SubData Query Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
SubDataQueryTest::SubDataQueryTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_get_named_buffer_subdata", "Buffer Objects Get Named Buffer SubData Query Test")
	, m_pNamedBufferData(DE_NULL)
	, m_pGetNamedBufferSubData(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Get Named Buffer SubData Query Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult SubDataQueryTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint buffer = 0;

	m_pNamedBufferData		 = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pGetNamedBufferSubData = (PFNGLGETNAMEDBUFFERSUBDATA)gl.getNamedBufferSubData;

	try
	{
		if ((DE_NULL == m_pNamedBufferData) || (DE_NULL == m_pGetNamedBufferSubData))
		{
			throw 0;
		}

		/* Buffer creation. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		/* Buffer's storage allocation and reference data upload. */
		m_pNamedBufferData(buffer, s_reference_size, s_reference, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData failed.");

		/* Mapping with new named buffer map function. */
		glw::GLuint data[s_reference_count] = {};
		m_pGetNamedBufferSubData(buffer, 0, s_reference_size / 2, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetNamedBufferSubData failed.");

		m_pGetNamedBufferSubData(buffer, s_reference_size / 2, s_reference_size / 2, &data[s_reference_count / 2]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetNamedBufferSubData failed.");

		/* Comparison results with reference data. */
		for (glw::GLsizei i = 0; i < s_reference_count; ++i)
		{
			is_ok &= (data[i] == s_reference[i]);
		}

		if (!is_ok)
		{
			/* Log. */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetNamedBufferSubData returned data which is not identical to reference data."
				<< tcu::TestLog::EndMessage;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = false;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Defaults Test Implementation   ********************************/

/** @brief Defaults Query Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
DefaultsTest::DefaultsTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_defaults", "Buffer Objects Defaults Test")
	, m_pNamedBufferData(DE_NULL)
	, m_pGetNamedBufferParameteri64v(DE_NULL)
	, m_pGetNamedBufferParameteriv(DE_NULL)
	, m_pGetNamedBufferPointerv(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Compare value with the reference.
 *
 *  @param [in] value               Value to be compared.
 *  @param [in] reference_value     Reference value for comparison.
 *  @param [in] pname_string        String of parameter name of the value (for logging).
 *  @param [in] function_string     String of function which returned the value (for logging).
 *
 *  @return True if value is equal to reference value, false otherwise. False solution is logged.
 */
template <typename T>
bool DefaultsTest::CheckValue(const T value, const T reference_value, const glw::GLchar* pname_string,
							  const glw::GLchar* function_string)
{
	if (reference_value != value)
	{
		/* Log. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function_string << " called with "
											<< pname_string << " parameter name returned " << value << ", but "
											<< reference_value << " was expected." << tcu::TestLog::EndMessage;

		return false;
	}
	return true;
}

/** @brief Iterate Defaults Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult DefaultsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint buffer = 0;

	m_pNamedBufferData			   = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pGetNamedBufferParameteri64v = (PFNGLGETNAMEDBUFFERPARAMETERI64V)gl.getNamedBufferParameteri64v;
	m_pGetNamedBufferParameteriv   = (PFNGLGETNAMEDBUFFERPARAMETERIV)gl.getNamedBufferParameteriv;
	m_pGetNamedBufferPointerv	  = (PFNGLGETNAMEDBUFFERPOINTERV)gl.getNamedBufferPointerv;

	try
	{
		if ((DE_NULL == m_pNamedBufferData) || (DE_NULL == m_pGetNamedBufferParameteri64v) ||
			(DE_NULL == m_pGetNamedBufferParameteriv) || (DE_NULL == m_pGetNamedBufferPointerv))
		{
			throw 0;
		}

		/* Buffer creation. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		/* Test data for glGetNamedBufferParameteri*v. */
		static const struct
		{
			glw::GLenum		   pname;
			const glw::GLchar* pname_string;
			glw::GLint		   expected_data;
		} test_values[] = { { GL_BUFFER_SIZE, "GL_BUFFER_SIZE", 0 },
							{ GL_BUFFER_USAGE, "GL_BUFFER_USAGE", GL_STATIC_DRAW },
							{ GL_BUFFER_ACCESS, "GL_BUFFER_ACCESS", GL_READ_WRITE },
							{ GL_BUFFER_ACCESS_FLAGS, "GL_BUFFER_ACCESS_FLAGS", 0 },
							{ GL_BUFFER_IMMUTABLE_STORAGE, "GL_BUFFER_IMMUTABLE_STORAGE", GL_FALSE },
							{ GL_BUFFER_MAPPED, "GL_BUFFER_MAPPED", GL_FALSE },
							{ GL_BUFFER_MAP_OFFSET, "GL_BUFFER_MAP_OFFSET", 0 },
							{ GL_BUFFER_MAP_LENGTH, "GL_BUFFER_MAP_LENGTH", 0 },
							{ GL_BUFFER_STORAGE_FLAGS, "GL_BUFFER_STORAGE_FLAGS", 0 } };

		static const glw::GLuint test_dictionary_count = sizeof(test_values) / sizeof(test_values[0]);

		/* Test glGetNamedBufferParameteriv. */
		for (glw::GLuint i = 0; i < test_dictionary_count; ++i)
		{
			glw::GLint data = -1;

			m_pGetNamedBufferParameteriv(buffer, test_values[i].pname, &data);

			is_ok &= CheckParameterError(test_values[i].pname_string, "glGetNamedBufferParameteriv");

			is_ok &= CheckValue<glw::GLint>(data, test_values[i].expected_data, test_values[i].pname_string,
											"glGetNamedBufferParameteriv");
		}

		/* Test glGetNamedBufferParameteri64v. */
		for (glw::GLuint i = 0; i < test_dictionary_count; ++i)
		{
			glw::GLint64 data = -1;

			m_pGetNamedBufferParameteri64v(buffer, test_values[i].pname, &data);

			is_ok &= CheckParameterError(test_values[i].pname_string, "glGetNamedBufferParameteri64v");

			is_ok &= CheckValue<glw::GLint64>(data, (glw::GLint64)test_values[i].expected_data,
											  test_values[i].pname_string, "glGetNamedBufferParameteri64v");
		}

		/* Test glGetNamedBufferPointerv. */
		{
			glw::GLvoid* data = (glw::GLvoid*)1;

			m_pGetNamedBufferPointerv(buffer, GL_BUFFER_MAP_POINTER, &data);

			is_ok &= CheckParameterError("GL_BUFFER_MAP_POINTER", "glGetNamedBufferPointer");

			is_ok &= CheckValue<glw::GLvoid*>(data, (glw::GLvoid*)DE_NULL, "GL_BUFFER_MAP_POINTER",
											  "glGetNamedBufferParameteriv");
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Check for GL error and log.
 *
 *  @param [in] pname_string        String of parameter name of the value (for logging).
 *  @param [in] function_string     String of function which returned the value (for logging).
 *
 *  @return True if error was generated, false otherwise. False solution is logged.
 */
bool DefaultsTest::CheckParameterError(const glw::GLchar* pname_string, const glw::GLchar* function_string)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Error check. */
	if (glw::GLenum error = gl.getError())
	{
		/* Log. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function_string << " called with "
											<< pname_string << " parameter name unexpectedly returned "
											<< glu::getErrorStr(error) << "error." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Errors Test Implementation   ********************************/

/** @brief Errors Query Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ErrorsTest::ErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_errors", "Buffer Objects Errors Test")
	, m_pClearNamedBufferData(DE_NULL)
	, m_pClearNamedBufferSubData(DE_NULL)
	, m_pCopyNamedBufferSubData(DE_NULL)
	, m_pFlushMappedNamedBufferRange(DE_NULL)
	, m_pGetNamedBufferParameteri64v(DE_NULL)
	, m_pGetNamedBufferParameteriv(DE_NULL)
	, m_pGetNamedBufferPointerv(DE_NULL)
	, m_pGetNamedBufferSubData(DE_NULL)
	, m_pMapNamedBuffer(DE_NULL)
	, m_pMapNamedBufferRange(DE_NULL)
	, m_pNamedBufferData(DE_NULL)
	, m_pNamedBufferStorage(DE_NULL)
	, m_pNamedBufferSubData(DE_NULL)
	, m_pUnmapNamedBuffer(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* API function pointers setup. */
	m_pClearNamedBufferData		   = (PFNGLCLEARNAMEDBUFFERDATA)gl.clearNamedBufferData;
	m_pClearNamedBufferSubData	 = (PFNGLCLEARNAMEDBUFFERSUBDATA)gl.clearNamedBufferSubData;
	m_pCopyNamedBufferSubData	  = (PFNGLCOPYNAMEDBUFFERSUBDATA)gl.copyNamedBufferSubData;
	m_pFlushMappedNamedBufferRange = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE)gl.flushMappedNamedBufferRange;
	m_pGetNamedBufferParameteri64v = (PFNGLGETNAMEDBUFFERPARAMETERI64V)gl.getNamedBufferParameteri64v;
	m_pGetNamedBufferParameteriv   = (PFNGLGETNAMEDBUFFERPARAMETERIV)gl.getNamedBufferParameteriv;
	m_pGetNamedBufferPointerv	  = (PFNGLGETNAMEDBUFFERPOINTERV)gl.getNamedBufferPointerv;
	m_pGetNamedBufferSubData	   = (PFNGLGETNAMEDBUFFERSUBDATA)gl.getNamedBufferSubData;
	m_pMapNamedBuffer			   = (PFNGLMAPNAMEDBUFFER)gl.mapNamedBuffer;
	m_pMapNamedBufferRange		   = (PFNGLMAPNAMEDBUFFERRANGE)gl.mapNamedBufferRange;
	m_pNamedBufferData			   = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pNamedBufferStorage		   = (PFNGLNAMEDBUFFERSTORAGE)gl.namedBufferStorage;
	m_pNamedBufferSubData		   = (PFNGLNAMEDBUFFERSUBDATA)gl.namedBufferSubData;
	m_pUnmapNamedBuffer			   = (PFNGLUNMAPNAMEDBUFFER)gl.unmapNamedBuffer;

	try
	{
		/* API function pointers check. */
		if ((DE_NULL == m_pClearNamedBufferData) || (DE_NULL == m_pClearNamedBufferSubData) ||
			(DE_NULL == m_pCopyNamedBufferSubData) || (DE_NULL == m_pFlushMappedNamedBufferRange) ||
			(DE_NULL == m_pGetNamedBufferParameteri64v) || (DE_NULL == m_pGetNamedBufferParameteriv) ||
			(DE_NULL == m_pGetNamedBufferPointerv) || (DE_NULL == m_pGetNamedBufferSubData) ||
			(DE_NULL == m_pMapNamedBuffer) || (DE_NULL == m_pMapNamedBufferRange) || (DE_NULL == m_pNamedBufferData) ||
			(DE_NULL == m_pNamedBufferStorage) || (DE_NULL == m_pNamedBufferSubData) ||
			(DE_NULL == m_pUnmapNamedBuffer))
		{
			throw 0;
		}

		/* Running test cases.                              Cleaning errors. */
		is_ok &= TestErrorsOfClearNamedBufferData();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfClearNamedBufferSubData();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfCopyNamedBufferSubData();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfCreateBuffers();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfFlushMappedNamedBufferRange();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfGetNamedBufferParameter();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfGetNamedBufferPointerv();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfGetNamedBufferSubData();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfMapNamedBuffer();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfMapNamedBufferRange();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfNamedBufferData();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfNamedBufferStorage();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfNamedBufferSubData();
		while (gl.getError())
			;
		is_ok &= TestErrorsOfUnmapNamedBuffer();
		while (gl.getError())
			;
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check if error was generated and if it is equal to expected value. Log possible failure.
 *
 *  @param [in] function_name               Tested Function.
 *  @param [in] expected_error              Expected error function.
 *  @param [in] when_shall_be_generated     Description when shall the error occure.
 *
 *  @return True if GL error is equal to the expected value, false otherwise.
 */
bool ErrorsTest::ErrorCheckAndLog(const glw::GLchar* function_name, const glw::GLenum expected_error,
								  const glw::GLchar* when_shall_be_generated)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Error value storage. */
	glw::GLenum error = GL_NO_ERROR;

	/* Error comparision. */
	if (expected_error != (error = gl.getError()))
	{
		/* Log. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function_name << " does not generate "
											<< glu::getErrorStr(expected_error) << when_shall_be_generated
											<< "The error value of " << glu::getErrorStr(error) << " was observed."
											<< tcu::TestLog::EndMessage;

		/* Error cleanup. */
		while (gl.getError())
			;

		/* Check failed. */
		return false;
	}

	/* Error was equal to expected. */
	return true;
}

/** @brief Test Errors Of ClearNamedBufferData function.
 *
 *  Check that INVALID_OPERATION is generated by ClearNamedBufferData if
 *  buffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_ENUM is generated by ClearNamedBufferData if
 *  internal format is not one of the valid sized internal formats listed in
 *  the table above.
 *
 *  Check that INVALID_OPERATION is generated by ClearNamedBufferData if
 *  any part of the specified range of the buffer object is mapped with
 *  MapBufferRange or MapBuffer, unless it was mapped with the
 *  MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.
 *
 *  Check that INVALID_VALUE is generated by ClearNamedBufferData if
 *  format is not a valid format, or type is not a valid type.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfClearNamedBufferData()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint buffer	 = 0;
	glw::GLbyte dummy_data = 0;

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data,
							  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferStorage failed.");

		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pClearNamedBufferData(not_a_buffer_name, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferData", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test invalid sized internal format error behavior. */
		{
			/* Prepare for invalid sized internal format error behavior verification. */
			static const glw::GLenum valid_internal_formats[] = {
				GL_R8,		GL_R16,		GL_R16F,	GL_R32F,	 GL_R8I,	  GL_R16I,	GL_R32I,
				GL_R8UI,	GL_R16UI,   GL_R32UI,   GL_RG8,		 GL_RG16,	 GL_RG16F,   GL_RG32F,
				GL_RG8I,	GL_RG16I,   GL_RG32I,   GL_RG8UI,	GL_RG16UI,   GL_RG32UI,  GL_RGB32F,
				GL_RGB32I,  GL_RGB32UI, GL_RGBA8,   GL_RGBA16,   GL_RGBA16F,  GL_RGBA32F, GL_RGBA8I,
				GL_RGBA16I, GL_RGBA32I, GL_RGBA8UI, GL_RGBA16UI, GL_RGBA32UI, GL_NONE
			};
			static const glw::GLenum valid_internal_formats_last =
				sizeof(valid_internal_formats) / sizeof(valid_internal_formats[0]) - 1;

			glw::GLenum invalid_internal_format = 0;

			while (&valid_internal_formats[valid_internal_formats_last] !=
				   std::find(&valid_internal_formats[0], &valid_internal_formats[valid_internal_formats_last],
							 (++invalid_internal_format)))
				;

			/* Test. */
			m_pClearNamedBufferData(buffer, invalid_internal_format, GL_RED, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferData", GL_INVALID_ENUM,
									  " if internal format is not one of the valid sized internal formats "
									  "(GL_R8, GL_R16, GL_R16F, GL_R32F, GL_R8I, GL_R16I, GL_R32I, GL_R8UI,"
									  " GL_R16UI, GL_R32UI, GL_RG8, GL_RG16, GL_RG16F, GL_RG32F, GL_RG8I, GL_RG16I,"
									  " GL_RG32I, GL_RG8UI, GL_RG16UI, GL_RG32UI, GL_RGB32F, GL_RGB32I, GL_RGB32UI,"
									  " GL_RGBA8, GL_RGBA16, GL_RGBA16F, GL_RGBA32F, GL_RGBA8I, GL_RGBA16I, GL_RGBA32I,"
									  " GL_RGBA8UI, GL_RGBA16UI, GL_RGBA32UI).");
		}

		/* Test of mapped buffer clear error behavior verification (glMapNamedBuffer version). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBuffer(buffer, GL_READ_ONLY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pClearNamedBufferData(buffer, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferData", GL_INVALID_OPERATION,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBuffer, unless it was mapped with "
									  "the MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of mapped buffer clear error behavior verification (glMapNamedBufferRange version). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pClearNamedBufferData(buffer, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferData", GL_INVALID_OPERATION,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBufferRange, unless it was mapped with "
									  "the MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of persistently mapped buffer clear error with behavior verification (glMapNamedBufferRange version). */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pClearNamedBufferData(buffer, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferData", GL_NO_ERROR,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBuffer with the MAP_PERSISTENT_BIT"
									  " bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test invalid format error behavior. */
		{
			/* Prepare for invalid format error behavior verification. */
			static const glw::GLenum valid_formats[] = { GL_RED,		   GL_RG,
														 GL_RGB,		   GL_BGR,
														 GL_RGBA,		   GL_BGRA,
														 GL_RED_INTEGER,   GL_RG_INTEGER,
														 GL_RGB_INTEGER,   GL_BGR_INTEGER,
														 GL_RGBA_INTEGER,  GL_BGRA_INTEGER,
														 GL_STENCIL_INDEX, GL_DEPTH_COMPONENT,
														 GL_DEPTH_STENCIL };
			static const glw::GLenum valid_formats_last = sizeof(valid_formats) / sizeof(valid_formats[0]) - 1;

			glw::GLenum invalid_format = 0;

			while (&valid_formats[valid_formats_last] !=
				   std::find(&valid_formats[0], &valid_formats[valid_formats_last], (++invalid_format)))
				;

			/* Test. */
			m_pClearNamedBufferData(buffer, GL_R8, invalid_format, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog(
				"glClearNamedBufferData", GL_INVALID_ENUM,
				" if format is not a valid format "
				"(one of GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, "
				"GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, GL_BGR_INTEGER, GL_RGBA_INTEGER, GL_BGRA_INTEGER, "
				"GL_STENCIL_INDEX, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL).");
		}

		/* Test invalid type error behavior. */
		{
			/* Prepare for invalid type error behavior verification. */
			static const glw::GLenum valid_types[] = { GL_RED,			 GL_RG,
													   GL_RGB,			 GL_BGR,
													   GL_RGBA,			 GL_BGRA,
													   GL_RED_INTEGER,   GL_RG_INTEGER,
													   GL_RGB_INTEGER,   GL_BGR_INTEGER,
													   GL_RGBA_INTEGER,  GL_BGRA_INTEGER,
													   GL_STENCIL_INDEX, GL_DEPTH_COMPONENT,
													   GL_DEPTH_STENCIL };
			static const glw::GLenum valid_types_last = sizeof(valid_types) / sizeof(valid_types[0]) - 1;

			glw::GLenum invalid_type = 0;

			while (&valid_types[valid_types_last] !=
				   std::find(&valid_types[0], &valid_types[valid_types_last], (++invalid_type)))
				;

			/* Test. */
			m_pClearNamedBufferData(buffer, GL_R8, GL_RED, invalid_type, &dummy_data);

			is_ok &= ErrorCheckAndLog(
				"glClearNamedBufferData", GL_INVALID_ENUM,
				" if format is not a valid type "
				"(one of GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, "
				"GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_FLOAT, GL_UNSIGNED_BYTE_3_3_2, "
				"GL_UNSIGNED_BYTE_2_3_3_REV, GL_UNSIGNED_SHORT_5_6_5, "
				"GL_UNSIGNED_SHORT_5_6_5_REV, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_4_4_4_4_REV, "
				"GL_UNSIGNED_SHORT_5_5_5_1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_UNSIGNED_INT_8_8_8_8, "
				"GL_UNSIGNED_INT_8_8_8_8_REV, GL_UNSIGNED_INT_10_10_10_2, and GL_UNSIGNED_INT_2_10_10_10_REV).");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of ClearNamedBufferSubData function.
 *
 *  Check that INVALID_OPERATION is generated by ClearNamedBufferSubData
 *  if buffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_ENUM is generated by ClearNamedBufferSubData if
 *  internal format is not one of the valid sized internal formats listed in
 *  the table above.
 *
 *  Check that INVALID_VALUE is generated by ClearNamedBufferSubData if
 *  offset or range are not multiples of the number of basic machine units
 *  per-element for the internal format specified by internal format. This
 *  value may be computed by multiplying the number of components for
 *  internal format from the table by the size of the base type from the
 *  specification table.
 *
 *  Check that INVALID_VALUE is generated by ClearNamedBufferSubData if
 *  offset or size is negative, or if offset+size is greater than the value
 *  of BUFFER_SIZE for the buffer object.
 *
 *  Check that INVALID_OPERATION is generated by ClearNamedBufferSubData
 *  if any part of the specified range of the buffer object is mapped with
 *  MapBufferRange or MapBuffer, unless it was mapped with the
 *  MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.
 *
 *  Check that INVALID_VALUE is generated by ClearNamedBufferSubData if format is not
 *  a valid format, or type is not a valid type.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfClearNamedBufferSubData()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer		   = 0;
	glw::GLubyte dummy_data[4] = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data,
							  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pClearNamedBufferSubData(not_a_buffer_name, GL_R8, 0, sizeof(dummy_data), GL_RGBA, GL_UNSIGNED_BYTE,
									   &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test invalid sized internal format error behavior. */
		{
			/* Prepare for invalid sized internal format error behavior verification. */
			static const glw::GLenum valid_internal_formats[] = {
				GL_R8,		GL_R16,		GL_R16F,	GL_R32F,	 GL_R8I,	  GL_R16I,	GL_R32I,
				GL_R8UI,	GL_R16UI,   GL_R32UI,   GL_RG8,		 GL_RG16,	 GL_RG16F,   GL_RG32F,
				GL_RG8I,	GL_RG16I,   GL_RG32I,   GL_RG8UI,	GL_RG16UI,   GL_RG32UI,  GL_RGB32F,
				GL_RGB32I,  GL_RGB32UI, GL_RGBA8,   GL_RGBA16,   GL_RGBA16F,  GL_RGBA32F, GL_RGBA8I,
				GL_RGBA16I, GL_RGBA32I, GL_RGBA8UI, GL_RGBA16UI, GL_RGBA32UI, GL_NONE
			};
			static const glw::GLenum valid_internal_formats_last =
				sizeof(valid_internal_formats) / sizeof(valid_internal_formats[0]) - 1;

			glw::GLenum invalid_internal_format = 0;

			while (&valid_internal_formats[valid_internal_formats_last] !=
				   std::find(&valid_internal_formats[0], &valid_internal_formats[valid_internal_formats_last],
							 (++invalid_internal_format)))
				;

			/* Test. */
			m_pClearNamedBufferData(buffer, invalid_internal_format, GL_RGBA, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_INVALID_ENUM,
									  " if internal format is not one of the valid sized internal formats "
									  "(GL_R8, GL_R16, GL_R16F, GL_R32F, GL_R8I, GL_R16I, GL_R32I, GL_R8UI,"
									  " GL_R16UI, GL_R32UI, GL_RG8, GL_RG16, GL_RG16F, GL_RG32F, GL_RG8I, GL_RG16I,"
									  " GL_RG32I, GL_RG8UI, GL_RG16UI, GL_RG32UI, GL_RGB32F, GL_RGB32I, GL_RGB32UI,"
									  " GL_RGBA8, GL_RGBA16, GL_RGBA16F, GL_RGBA32F, GL_RGBA8I, GL_RGBA16I, GL_RGBA32I,"
									  " GL_RGBA8UI, GL_RGBA16UI, GL_RGBA32UI).");
		}

		/* Test incorrect offset alignment error behavior. */
		{
			/* Test. */
			m_pClearNamedBufferSubData(buffer, GL_RGBA8, sizeof(dummy_data[0]), sizeof(dummy_data), GL_RGBA,
									   GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_INVALID_VALUE,
									  "if offset is not multiples of the number of basic machine units (GLubyte)"
									  "per-element for the internal format (GL_RGBA) specified by internal format.");
		}

		/* Test incorrect range alignment error behavior. */
		{
			m_pClearNamedBufferSubData(buffer, GL_RGBA8, 0, sizeof(dummy_data) - sizeof(dummy_data[0]), GL_RGBA,
									   GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_INVALID_VALUE,
									  "if range is not multiples of the number of basic machine units (GLubyte)"
									  "per-element for the internal format (GL_RGBA) specified by internal format.");
		}

		/* Test negative offset error behavior. */
		{
			/* Test. */
			m_pClearNamedBufferSubData(buffer, GL_R8, -1, sizeof(dummy_data), GL_RGBA, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_INVALID_VALUE, " if offset or size is negative.");
		}

		/* Test negative size error behavior. */
		{
			/* Test. */
			m_pClearNamedBufferSubData(buffer, GL_R8, 0, -((glw::GLsizei)sizeof(dummy_data)), GL_RGBA, GL_UNSIGNED_BYTE,
									   &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_INVALID_VALUE, " if offset or size is negative.");
		}

		/* Test size overflow error behavior. */
		{
			/* Test. */
			m_pClearNamedBufferSubData(buffer, GL_R8, 0, 2 * sizeof(dummy_data), GL_RGBA, GL_UNSIGNED_BYTE,
									   &dummy_data);

			is_ok &= ErrorCheckAndLog(
				"glClearNamedBufferSubData", GL_INVALID_VALUE,
				" if offset+size is greater than the value of BUFFER_SIZE for the specified buffer object.");
		}

		/* Test of mapped buffer clear error behavior verification (glMapNamedBuffer version). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBuffer(buffer, GL_READ_ONLY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pClearNamedBufferSubData(buffer, GL_R8, 0, sizeof(dummy_data), GL_RGBA, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_INVALID_OPERATION,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBuffer, unless it was mapped with "
									  "the MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of mapped buffer clear error behavior verification (glMapNamedBufferRange version). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pClearNamedBufferSubData(buffer, GL_R8, 0, sizeof(dummy_data), GL_RGBA, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_INVALID_OPERATION,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBufferRange, unless it was mapped with "
									  "the MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of persistently mapped buffer clear error with behavior verification (glMapNamedBufferRange version). */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pClearNamedBufferSubData(buffer, GL_R8, 0, sizeof(dummy_data), GL_RGBA, GL_UNSIGNED_BYTE, &dummy_data);

			is_ok &= ErrorCheckAndLog("glClearNamedBufferSubData", GL_NO_ERROR,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBuffer with the MAP_PERSISTENT_BIT"
									  " bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test invalid format error behavior. */
		{
			/* Prepare for invalid format error behavior verification. */
			static const glw::GLenum valid_formats[] = { GL_RED,		   GL_RG,
														 GL_RGB,		   GL_BGR,
														 GL_RGBA,		   GL_BGRA,
														 GL_RED_INTEGER,   GL_RG_INTEGER,
														 GL_RGB_INTEGER,   GL_BGR_INTEGER,
														 GL_RGBA_INTEGER,  GL_BGRA_INTEGER,
														 GL_STENCIL_INDEX, GL_DEPTH_COMPONENT,
														 GL_DEPTH_STENCIL };
			static const glw::GLenum valid_formats_last = sizeof(valid_formats) / sizeof(valid_formats[0]) - 1;

			glw::GLenum invalid_format = 0;

			while (&valid_formats[valid_formats_last] !=
				   std::find(&valid_formats[0], &valid_formats[valid_formats_last], (++invalid_format)))
				;

			/* Test. */
			m_pClearNamedBufferSubData(buffer, GL_R8, 0, sizeof(dummy_data), invalid_format, GL_UNSIGNED_BYTE,
									   &dummy_data);

			is_ok &= ErrorCheckAndLog(
				"glClearNamedBufferSubData", GL_INVALID_ENUM,
				" if format is not a valid format "
				"(one of GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, "
				"GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, GL_BGR_INTEGER, GL_RGBA_INTEGER, GL_BGRA_INTEGER, "
				"GL_STENCIL_INDEX, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL).");
		}

		/* Test invalid type error behavior. */
		{
			/* Prepare for invalid type error behavior verification. */
			static const glw::GLenum valid_types[] = { GL_RED,			 GL_RG,
													   GL_RGB,			 GL_BGR,
													   GL_RGBA,			 GL_BGRA,
													   GL_RED_INTEGER,   GL_RG_INTEGER,
													   GL_RGB_INTEGER,   GL_BGR_INTEGER,
													   GL_RGBA_INTEGER,  GL_BGRA_INTEGER,
													   GL_STENCIL_INDEX, GL_DEPTH_COMPONENT,
													   GL_DEPTH_STENCIL, GL_NONE };
			static const glw::GLenum valid_types_last = sizeof(valid_types) / sizeof(valid_types[0]) - 1;

			glw::GLenum invalid_type = 0;

			while (&valid_types[valid_types_last] !=
				   std::find(&valid_types[0], &valid_types[valid_types_last], (++invalid_type)))
				;

			/* Test. */
			m_pClearNamedBufferSubData(buffer, GL_R8, 0, sizeof(dummy_data), GL_RGBA, invalid_type, &dummy_data);

			is_ok &= ErrorCheckAndLog(
				"glClearNamedBufferSubData", GL_INVALID_ENUM,
				" if format is not a valid type "
				"(one of GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, "
				"GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_FLOAT, GL_UNSIGNED_BYTE_3_3_2, "
				"GL_UNSIGNED_BYTE_2_3_3_REV, GL_UNSIGNED_SHORT_5_6_5, "
				"GL_UNSIGNED_SHORT_5_6_5_REV, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_4_4_4_4_REV, "
				"GL_UNSIGNED_SHORT_5_5_5_1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_UNSIGNED_INT_8_8_8_8, "
				"GL_UNSIGNED_INT_8_8_8_8_REV, GL_UNSIGNED_INT_10_10_10_2, and GL_UNSIGNED_INT_2_10_10_10_REV).");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of CopyNamedBufferSubData function.
 *
 *  Check that INVALID_OPERATION is generated by CopyNamedBufferSubData if readBuffer
 *  or writeBuffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_VALUE is generated by CopyNamedBufferSubData if any of
 *  readOffset, writeOffset or size is negative, if readOffset+size is
 *  greater than the size of the source buffer object (its value of
 *  BUFFER_SIZE), or if writeOffset+size is greater than the size of the
 *  destination buffer object.
 *
 *  Check that INVALID_VALUE is generated by CopyNamedBufferSubData if the
 *  source and destination are the same buffer object, and the ranges
 *  [readOffset,readOffset+size) and [writeOffset,writeOffset+size) overlap.
 *
 *  Check that INVALID_OPERATION is generated by CopyNamedBufferSubData if
 *  either the source or destination buffer object is mapped with
 *  MapBufferRange or MapBuffer, unless they were mapped with the
 *  MAP_PERSISTENT bit set in the MapBufferRange access flags.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfCopyNamedBufferSubData()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer_r	  = 0;
	glw::GLuint  buffer_w	  = 0;
	glw::GLubyte dummy_data[4] = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer_r);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer_r, sizeof(dummy_data), &dummy_data,
							  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		gl.createBuffers(1, &buffer_w);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer_w, sizeof(dummy_data), &dummy_data,
							  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pCopyNamedBufferSubData(not_a_buffer_name, buffer_w, 0, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_OPERATION,
									  " if readBuffer is not the name of an existing buffer object.");

			m_pCopyNamedBufferSubData(buffer_r, not_a_buffer_name, 0, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_OPERATION,
									  " if writeBuffer is not the name of an existing buffer object.");
		}

		/* Test negative read offset error behavior. */
		{
			/* Test. */
			m_pCopyNamedBufferSubData(buffer_r, buffer_w, -1, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_VALUE, "if readOffset is negative.");
		}

		/* Test negative write offset error behavior. */
		{
			/* Test. */
			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, -1, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_VALUE, "if writeOffset is negative.");
		}

		/* Test negative size error behavior. */
		{
			/* Test. */
			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, 0, -1);

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_VALUE, "if size is negative.");
		}

		/* Test overflow size error behavior. */
		{
			/* Test. */
			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, 0, 2 * sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_VALUE,
									  " if size is greater than the size of the source buffer object.");
		}

		/* Test overflow read offset and size error behavior. */
		{
			/* Test. */
			m_pCopyNamedBufferSubData(buffer_r, buffer_w, sizeof(dummy_data) / 2, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_VALUE,
									  " if readOffset+size is greater than the size of the source buffer object.");
		}

		/* Test overflow write offset and size error behavior. */
		{
			/* Test. */
			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, sizeof(dummy_data) / 2, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_VALUE,
									  " if writeOffset+size is greater than the size of the source buffer object.");
		}

		/* Test same buffer overlapping error behavior. */
		{
			/* Test. */
			m_pCopyNamedBufferSubData(buffer_w, buffer_w, 0, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_VALUE,
									  " if the source and destination are the same buffer object, and the ranges"
									  " [readOffset,readOffset+size) and [writeOffset,writeOffset+size) overlap.");
		}

		/* Test of mapped read buffer copy error behavior verification (glMapNamedBuffer version). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBuffer(buffer_r, GL_READ_ONLY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_OPERATION,
									  " if the source buffer object is mapped with MapBuffer.");

			m_pUnmapNamedBuffer(buffer_r);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of mapped write buffer copy error behavior verification (glMapNamedBuffer version). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBuffer(buffer_w, GL_READ_ONLY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_OPERATION,
									  " if the destination buffer object is mapped with MapBuffer.");

			m_pUnmapNamedBuffer(buffer_w);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of mapped read buffer copy error behavior verification (glMapNamedBufferRange version). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBufferRange(buffer_r, 0, sizeof(dummy_data), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_OPERATION,
									  " if the source buffer object is mapped with MapBuffer.");

			m_pUnmapNamedBuffer(buffer_r);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of mapped write buffer copy error behavior verification (glMapNamedBufferRange version). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBufferRange(buffer_w, 0, sizeof(dummy_data), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_INVALID_OPERATION,
									  " if the destination buffer object is mapped with MapBuffer.");

			m_pUnmapNamedBuffer(buffer_w);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of persistently mapped read buffer copy error with behavior verification. */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer_r, 0, sizeof(dummy_data), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_NO_ERROR,
									  " if the source buffer object is mapped using "
									  "MapBufferRange with the MAP_PERSISTENT bit "
									  "set in the access flags.");

			m_pUnmapNamedBuffer(buffer_r);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of persistently mapped write buffer copy error with behavior verification. */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer_w, 0, sizeof(dummy_data), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pCopyNamedBufferSubData(buffer_r, buffer_w, 0, 0, sizeof(dummy_data));

			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");
			is_ok &= ErrorCheckAndLog("glCopyNamedBufferSubData", GL_NO_ERROR,
									  " if the destination buffer object is mapped using "
									  "MapBufferRange with the MAP_PERSISTENT bit "
									  "set in the access flags.");

			m_pUnmapNamedBuffer(buffer_w);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer_r)
	{
		gl.deleteBuffers(1, &buffer_r);

		buffer_r = 0;
	}

	if (buffer_r)
	{
		gl.deleteBuffers(1, &buffer_r);

		buffer_r = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of CreateBuffers function.
 *
 *  Check that INVALID_VALUE is generated by CreateBuffers if n is negative.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfCreateBuffers()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok = true;

	/* Test. */
	glw::GLuint buffer = 0;

	gl.createBuffers(-1, &buffer);

	is_ok &= ErrorCheckAndLog("glCreateBuffers", GL_INVALID_VALUE, " if n is negative.");

	/* Sanity check. */
	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		/* Possible error cleanup. */
		while (gl.getError())
			;
	}

	return is_ok;
}

/** @brief Test Errors Of FlushMappedNamedBufferRange function.
 *
 *  Check that INVALID_OPERATION is generated by FlushMappedNamedBufferRange
 *  if buffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_VALUE is generated by FlushMappedNamedBufferRange if
 *  offset or length is negative, or if offset + length exceeds the size of
 *  the mapping.
 *
 *  Check that INVALID_OPERATION is generated by FlushMappedNamedBufferRange
 *  if the buffer object is not mapped, or is mapped without the
 *  MAP_FLUSH_EXPLICIT_BIT flag.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfFlushMappedNamedBufferRange()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer		   = 0;
	glw::GLubyte dummy_data[4] = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name flush error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pFlushMappedNamedBufferRange(not_a_buffer_name, 0, 1);

			is_ok &= ErrorCheckAndLog("glFlushMappedNamedBufferRange", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test negative offset flush error behavior. */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pFlushMappedNamedBufferRange(buffer, -1, 1);

			is_ok &= ErrorCheckAndLog("glFlushMappedNamedBufferRange", GL_INVALID_VALUE, " if offset is negative.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test negative length flush error behavior. */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pFlushMappedNamedBufferRange(buffer, 0, -1);

			is_ok &= ErrorCheckAndLog("glFlushMappedNamedBufferRange", GL_INVALID_VALUE, " if length is negative.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test length exceeds the mapping size flush error behavior. */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data) / 2, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pFlushMappedNamedBufferRange(buffer, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glFlushMappedNamedBufferRange", GL_INVALID_VALUE,
									  " if length exceeds the size of the mapping.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test offset + length exceeds the mapping size flush error behavior. */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pFlushMappedNamedBufferRange(buffer, 1, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glFlushMappedNamedBufferRange", GL_INVALID_VALUE,
									  " if offset + length exceeds the size of the mapping.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test not mapped buffer flush error behavior. */
		{
			m_pFlushMappedNamedBufferRange(buffer, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glFlushMappedNamedBufferRange", GL_INVALID_OPERATION,
									  " if the buffer object is not mapped.");
		}

		/* Test buffer flush without the MAP_FLUSH_EXPLICIT_BIT error behavior. */
		{
			(void)(glw::GLbyte*) m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_WRITE_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pFlushMappedNamedBufferRange(buffer, 0, sizeof(dummy_data));

			is_ok &= ErrorCheckAndLog("glFlushMappedNamedBufferRange", GL_INVALID_OPERATION,
									  " if the buffer is mapped without the MAP_FLUSH_EXPLICIT_BIT flag.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of GetNamedBufferParameteriv
 *         and GetNamedBufferParameteri64v functions.
 *
 *  Check that INVALID_OPERATION is generated by GetNamedBufferParameter* if
 *  buffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_ENUM is generated by GetNamedBufferParameter* if
 *  pname is not one of the buffer object parameter names: BUFFER_ACCESS,
 *  BUFFER_ACCESS_FLAGS, BUFFER_IMMUTABLE_STORAGE, BUFFER_MAPPED,
 *  BUFFER_MAP_LENGTH, BUFFER_MAP_OFFSET, BUFFER_SIZE, BUFFER_STORAGE_FLAGS,
 *  BUFFER_USAGE.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfGetNamedBufferParameter()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer		   = 0;
	glw::GLubyte dummy_data[4] = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name in GetNamedBufferParameteriv function error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			glw::GLint value = 0;

			/* Test. */
			m_pGetNamedBufferParameteriv(not_a_buffer_name, GL_BUFFER_MAPPED, &value);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferParameteriv", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test invalid buffer name in GetNamedBufferParameteri64v function error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			glw::GLint64 value = 0;

			/* Test. */
			m_pGetNamedBufferParameteri64v(not_a_buffer_name, GL_BUFFER_MAPPED, &value);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferParameteri64v", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test invalid parameter name in GetNamedBufferParameteriv function error behavior. */
		{
			/* Prepare for invalid parameter name error behavior verification. */
			static const glw::GLenum valid_parameters[] = {
				GL_BUFFER_ACCESS, GL_BUFFER_ACCESS_FLAGS,  GL_BUFFER_IMMUTABLE_STORAGE,
				GL_BUFFER_MAPPED, GL_BUFFER_MAP_LENGTH,	GL_BUFFER_MAP_OFFSET,
				GL_BUFFER_SIZE,   GL_BUFFER_STORAGE_FLAGS, GL_BUFFER_USAGE,
				GL_NONE
			};
			static const glw::GLenum valid_parameters_last = sizeof(valid_parameters) / sizeof(valid_parameters[0]) - 1;

			glw::GLint value = 0;

			glw::GLenum invalid_parameter = 0;

			while (&valid_parameters[valid_parameters_last] !=
				   std::find(&valid_parameters[0], &valid_parameters[valid_parameters_last], (++invalid_parameter)))
				;

			/* Test. */
			m_pGetNamedBufferParameteriv(buffer, invalid_parameter, &value);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferParameteriv", GL_INVALID_ENUM,
									  " if pname is not one of the buffer object parameter names: BUFFER_ACCESS,"
									  " BUFFER_ACCESS_FLAGS, BUFFER_IMMUTABLE_STORAGE, BUFFER_MAPPED,"
									  " BUFFER_MAP_LENGTH, BUFFER_MAP_OFFSET, BUFFER_SIZE, BUFFER_STORAGE_FLAGS,"
									  " BUFFER_USAGE.");
		}

		/* Test invalid parameter name in GetNamedBufferParameteri64v function error behavior. */
		{
			/* Prepare for invalid parameter name error behavior verification. */
			static const glw::GLenum valid_parameters[] = {
				GL_BUFFER_ACCESS, GL_BUFFER_ACCESS_FLAGS,  GL_BUFFER_IMMUTABLE_STORAGE,
				GL_BUFFER_MAPPED, GL_BUFFER_MAP_LENGTH,	GL_BUFFER_MAP_OFFSET,
				GL_BUFFER_SIZE,   GL_BUFFER_STORAGE_FLAGS, GL_BUFFER_USAGE,
				GL_NONE
			};
			static const glw::GLenum valid_parameters_last = sizeof(valid_parameters) / sizeof(valid_parameters[0]) - 1;

			glw::GLint64 value = 0;

			glw::GLenum invalid_parameter = 0;

			while (&valid_parameters[valid_parameters_last] !=
				   std::find(&valid_parameters[0], &valid_parameters[valid_parameters_last], (++invalid_parameter)))
				;

			/* Test. */
			m_pGetNamedBufferParameteri64v(buffer, invalid_parameter, &value);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferParameteri64v", GL_INVALID_ENUM,
									  " if pname is not one of the buffer object parameter names: BUFFER_ACCESS,"
									  " BUFFER_ACCESS_FLAGS, BUFFER_IMMUTABLE_STORAGE, BUFFER_MAPPED,"
									  " BUFFER_MAP_LENGTH, BUFFER_MAP_OFFSET, BUFFER_SIZE, BUFFER_STORAGE_FLAGS,"
									  " BUFFER_USAGE.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of GetNamedBufferPointerv function.
 *
 *  Check that INVALID_OPERATION is generated by GetNamedBufferPointerv
 *  if buffer is not the name of an existing buffer object.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfGetNamedBufferPointerv()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer		   = 0;
	glw::GLubyte dummy_data[4] = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name in GetNamedBufferPointerv function error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			glw::GLvoid* pointer = DE_NULL;

			/* Test. */
			m_pGetNamedBufferPointerv(not_a_buffer_name, GL_BUFFER_MAP_POINTER, &pointer);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferPointerv", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of GetNamedBufferSubData function.
 *
 *  Check that INVALID_OPERATION is generated by GetNamedBufferSubData if
 *  buffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_VALUE is generated by GetNamedBufferSubData if offset
 *  or size is negative, or if offset+size is greater than the value of
 *  BUFFER_SIZE for the buffer object.
 *
 *  Check that INVALID_OPERATION is generated by GetNamedBufferSubData if
 *  the buffer object is mapped with MapBufferRange or MapBuffer, unless it
 *  was mapped with the MAP_PERSISTENT_BIT bit set in the MapBufferRange
 *  access flags.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfGetNamedBufferSubData()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer		   = 0;
	glw::GLubyte dummy_data[4] = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name in pGetNamedBufferSubData function error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			m_pGetNamedBufferSubData(not_a_buffer_name, 0, sizeof(dummy_data_query), dummy_data_query);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferSubData", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test negative offset error behavior. */
		{
			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			m_pGetNamedBufferSubData(buffer, -1, sizeof(dummy_data_query), dummy_data_query);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferSubData", GL_INVALID_VALUE, " if offset is negative.");
		}

		/* Test negative size error behavior. */
		{
			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			m_pGetNamedBufferSubData(buffer, 0, -1, dummy_data_query);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferSubData", GL_INVALID_VALUE, " if size is negative.");
		}

		/* Test size overflow error behavior. */
		{
			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			m_pGetNamedBufferSubData(buffer, 0, 2 * sizeof(dummy_data_query), dummy_data_query);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferSubData", GL_INVALID_VALUE,
									  " if size is greater than the value of BUFFER_SIZE for the buffer object.");
		}

		/* Test offset+size overflow error behavior. */
		{
			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			m_pGetNamedBufferSubData(buffer, sizeof(dummy_data_query) / 2, sizeof(dummy_data_query), dummy_data_query);

			is_ok &=
				ErrorCheckAndLog("glGetNamedBufferSubData", GL_INVALID_VALUE,
								 " if offset+size is greater than the value of BUFFER_SIZE for the buffer object.");
		}

		/* Test offset overflow error behavior. */
		{
			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			m_pGetNamedBufferSubData(buffer, sizeof(dummy_data_query) + 1, 0, dummy_data_query);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferSubData", GL_INVALID_VALUE,
									  " if offset is greater than the value of BUFFER_SIZE for the buffer object.");
		}

		/* Test mapped buffer query error behavior. */
		{
			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			(void)(glw::GLbyte*) m_pMapNamedBuffer(buffer, GL_WRITE_ONLY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pGetNamedBufferSubData(buffer, 0, sizeof(dummy_data_query), dummy_data_query);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferSubData", GL_INVALID_OPERATION,
									  " if the buffer object is mapped with MapBufferRange.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test mapped buffer query error behavior. */
		{
			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			(void)(glw::GLbyte*) m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_WRITE_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pGetNamedBufferSubData(buffer, 0, sizeof(dummy_data_query), dummy_data_query);

			is_ok &= ErrorCheckAndLog("glGetNamedBufferSubData", GL_INVALID_OPERATION,
									  " if the buffer object is mapped with MapBufferRange.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test persistently mapped buffer query behavior. */
		{
			/* Query storage. */
			glw::GLubyte dummy_data_query[sizeof(dummy_data) / sizeof(dummy_data[0])] = {};

			/* Test. */
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pGetNamedBufferSubData(buffer, 0, sizeof(dummy_data_query), dummy_data_query);

			is_ok &= ErrorCheckAndLog(
				"glGetNamedBufferSubData", GL_NO_ERROR,
				" if the buffer object is mapped with MapBufferRange with GL_MAP_PERSISTENT_BIT flag.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of MapNamedBuffer function.
 *
 *  Check that INVALID_OPERATION is generated by MapNamedBuffer if buffer is
 *  not the name of an existing buffer object.
 *
 *  Check that INVALID_ENUM is generated by MapNamedBuffer if access is not
 *  READ_ONLY, WRITE_ONLY, or READ_WRITE.
 *
 *  Check that INVALID_OPERATION is generated by MapNamedBuffer if the
 *  buffer object is in a mapped state.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfMapNamedBuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer		   = 0;
	glw::GLubyte dummy_data[4] = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pMapNamedBuffer(not_a_buffer_name, GL_READ_ONLY);

			is_ok &= ErrorCheckAndLog("glMapNamedBuffer", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test access flag error behavior. */
		{
			/* Prepare for invalid type error behavior verification. */
			static const glw::GLenum valid_access_flags[] = { GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE, GL_NONE };
			static const glw::GLenum valid_access_flags_last =
				sizeof(valid_access_flags) / sizeof(valid_access_flags[0]) - 1;

			glw::GLenum invalid_access_flags = 0;

			while (&valid_access_flags[valid_access_flags_last] !=
				   std::find(&valid_access_flags[0], &valid_access_flags[valid_access_flags_last],
							 (++invalid_access_flags)))
				;

			/* Test. */
			glw::GLbyte* mapped_data = (glw::GLbyte*)m_pMapNamedBuffer(buffer, invalid_access_flags);

			is_ok &= ErrorCheckAndLog("glMapNamedBuffer", GL_INVALID_ENUM,
									  " if access is not READ_ONLY, WRITE_ONLY, or READ_WRITE.");

			/* Sanity unmapping. */
			if (DE_NULL != mapped_data)
			{
				m_pUnmapNamedBuffer(buffer);
				while (gl.getError())
					;
			}
		}

		/* Test mapping of mapped buffer error behavior. */
		{
			(void)(glw::GLbyte*) m_pMapNamedBuffer(buffer, GL_READ_ONLY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer.");

			glw::GLbyte* subsequent_mapped_data = (glw::GLbyte*)m_pMapNamedBuffer(buffer, GL_READ_ONLY);

			is_ok &= ErrorCheckAndLog("glMapNamedBuffer", GL_INVALID_OPERATION,
									  " if the buffer object is in a mapped state.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");

			if (subsequent_mapped_data)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glMapNamedBuffer called on mapped buffer returned non-NULL pointer when error shall occure."
					   "This may lead to undefined behavior during next tests (object still may be mapped). "
					   "Test was terminated prematurely."
					<< tcu::TestLog::EndMessage;
				throw 0;
			}
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of MapNamedBufferRange function.
 *
 *  Check that INVALID_OPERATION is generated by MapNamedBufferRange if
 *  buffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_VALUE is generated by MapNamedBufferRange if offset
 *  or length is negative, if offset+length is greater than the value of
 *  BUFFER_SIZE for the buffer object, or if access has any bits set other
 *  than those defined above.
 *
 *  Check that INVALID_OPERATION is generated by MapNamedBufferRange for any
 *  of the following conditions:
 *   -  length is zero.
 *   -  The buffer object is already in a mapped state.
 *   -  Neither MAP_READ_BIT nor MAP_WRITE_BIT is set.
 *   -  MAP_READ_BIT is set and any of MAP_INVALIDATE_RANGE_BIT,
 *      MAP_INVALIDATE_BUFFER_BIT or MAP_UNSYNCHRONIZED_BIT is set.
 *   -  MAP_FLUSH_EXPLICIT_BIT is set and MAP_WRITE_BIT is not set.
 *   -  Any of MAP_READ_BIT, MAP_WRITE_BIT, MAP_PERSISTENT_BIT, or
 *      MAP_COHERENT_BIT are set, but the same bit is not included in the
 *      buffer's storage flags.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfMapNamedBufferRange()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer				  = 0;
	glw::GLuint  buffer_special_flags = 0;
	glw::GLubyte dummy_data[4]		  = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data,
							  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pMapNamedBufferRange(not_a_buffer_name, 0, sizeof(dummy_data), GL_MAP_READ_BIT);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test negative offset error behavior. */
		{
			glw::GLvoid* mapped_data = m_pMapNamedBufferRange(buffer, -1, sizeof(dummy_data), GL_MAP_READ_BIT);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_VALUE, " if offset is negative.");

			/* Sanity unmapping. */
			if (DE_NULL != mapped_data)
			{
				m_pUnmapNamedBuffer(buffer);
				while (gl.getError())
					;
			}
		}

		/* Test negative length error behavior. */
		{
			glw::GLvoid* mapped_data = m_pMapNamedBufferRange(buffer, 0, -1, GL_MAP_READ_BIT);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_VALUE, " if length is negative.");

			/* Sanity unmapping. */
			if (DE_NULL != mapped_data)
			{
				m_pUnmapNamedBuffer(buffer);
				while (gl.getError())
					;
			}
		}

		/* Test length overflow error behavior. */
		{
			glw::GLvoid* mapped_data = m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data) * 2, GL_MAP_READ_BIT);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_VALUE,
									  " if length is greater than the value of BUFFER_SIZE"
									  " for the buffer object, or if access has any bits set other"
									  " than those defined above.");

			/* Sanity unmapping. */
			if (DE_NULL != mapped_data)
			{
				m_pUnmapNamedBuffer(buffer);
				while (gl.getError())
					;
			}
		}

		/* Test (offset+length) overflow error behavior. */
		{
			glw::GLvoid* mapped_data =
				m_pMapNamedBufferRange(buffer, sizeof(dummy_data) / 2, sizeof(dummy_data), GL_MAP_READ_BIT);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_VALUE,
									  " if offset+length is greater than the value of BUFFER_SIZE"
									  " for the buffer object, or if access has any bits set other"
									  " than those defined above.");

			/* Sanity unmapping. */
			if (DE_NULL != mapped_data)
			{
				m_pUnmapNamedBuffer(buffer);
				while (gl.getError())
					;
			}
		}

		/* Test zero length error behavior. */
		{
			glw::GLvoid* mapped_data = m_pMapNamedBufferRange(buffer, 0, 0, GL_MAP_READ_BIT);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_OPERATION, " if length is zero.");

			/* Sanity unmapping. */
			if (DE_NULL != mapped_data)
			{
				m_pUnmapNamedBuffer(buffer);
				while (gl.getError())
					;
			}
		}

		/* Test mapping of mapped buffer error behavior. */
		{
			m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer.");

			glw::GLvoid* subsequent_mapped_data =
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_OPERATION,
									  " if the buffer object is in a mapped state.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");

			if (subsequent_mapped_data)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glMapNamedBufferRange called on mapped buffer returned non-NULL pointer when error shall "
					   "occure."
					   "This may lead to undefined behavior during next tests (object still may be mapped). "
					   "Test was terminated prematurely."
					<< tcu::TestLog::EndMessage;
				throw 0;
			}
		}

		/* Test access flag read and write bits are not set error behavior. */
		{
			glw::GLvoid* mapped_data = m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), 0);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_OPERATION,
									  " if neither MAP_READ_BIT nor MAP_WRITE_BIT is set.");

			/* Sanity unmapping. */
			if (DE_NULL != mapped_data)
			{
				m_pUnmapNamedBuffer(buffer);
				while (gl.getError())
					;
			}
		}

		/* Test read access invalid flags error behavior. */
		{
			glw::GLenum read_access_invalid_flags[] = { GL_MAP_INVALIDATE_RANGE_BIT, GL_MAP_INVALIDATE_BUFFER_BIT,
														GL_MAP_UNSYNCHRONIZED_BIT };
			const glw::GLchar* read_access_invalid_flags_log[] = {
				" if MAP_READ_BIT is set with MAP_INVALIDATE_RANGE_BIT.",
				" if MAP_READ_BIT is set with MAP_INVALIDATE_BUFFER_BIT.",
				" if MAP_READ_BIT is set with MAP_UNSYNCHRONIZED_BIT."
			};
			glw::GLuint read_access_invalid_flags_count =
				sizeof(read_access_invalid_flags) / sizeof(read_access_invalid_flags[0]);

			for (glw::GLuint i = 0; i < read_access_invalid_flags_count; ++i)
			{
				glw::GLvoid* mapped_data = m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data),
																  GL_MAP_READ_BIT | read_access_invalid_flags[i]);

				is_ok &=
					ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_OPERATION, read_access_invalid_flags_log[i]);

				/* Sanity unmapping. */
				if (DE_NULL != mapped_data)
				{
					m_pUnmapNamedBuffer(buffer);
					while (gl.getError())
						;
				}
			}
		}

		/* Test access flush bit without write bit error behavior. */
		{
			glw::GLvoid* mapped_data =
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

			is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_OPERATION,
									  " if MAP_FLUSH_EXPLICIT_BIT is set and MAP_WRITE_BIT is not set.");

			/* Sanity unmapping. */
			if (DE_NULL != mapped_data)
			{
				m_pUnmapNamedBuffer(buffer);
				while (gl.getError())
					;
			}
		}

		/* Test incompatible buffer flag error behavior. */
		{
			glw::GLenum buffer_flags[] = { GL_MAP_WRITE_BIT, GL_MAP_READ_BIT, GL_MAP_WRITE_BIT,
										   GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT };
			glw::GLenum mapping_flags[] = { GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT,
											GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT };
			const glw::GLchar* mapping_flags_log[] = {
				" if MAP_READ_BIT is set, but the same bit is not included in the buffer's storage flags.",
				" if MAP_WRITE_BIT is set, but the same bit is not included in the buffer's storage flags.",
				" if MAP_PERSISTENT_BIT is set, but the same bit is not included in the buffer's storage flags.",
				" if MAP_COHERENT_BIT is set, but the same bit is not included in the buffer's storage flags."
			};
			glw::GLuint flags_count = sizeof(mapping_flags) / sizeof(mapping_flags[0]);

			for (glw::GLuint i = 0; i < flags_count; ++i)
			{
				/* Create buffer. */
				gl.createBuffers(1, &buffer_special_flags);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

				m_pNamedBufferStorage(buffer_special_flags, sizeof(dummy_data), &dummy_data, buffer_flags[i]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

				/* Test mapping. */
				glw::GLvoid* mapped_data =
					m_pMapNamedBufferRange(buffer_special_flags, 0, sizeof(dummy_data), mapping_flags[i]);

				is_ok &= ErrorCheckAndLog("glMapNamedBufferRange", GL_INVALID_OPERATION, mapping_flags_log[i]);

				/* Sanity unmapping. */
				if (DE_NULL != mapped_data)
				{
					m_pUnmapNamedBuffer(buffer);
					while (gl.getError())
						;
				}

				/* Releasing buffer. */
				if (buffer_special_flags)
				{
					gl.deleteBuffers(1, &buffer_special_flags);

					buffer_special_flags = 0;
				}
			}
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (buffer_special_flags)
	{
		gl.deleteBuffers(1, &buffer_special_flags);

		buffer_special_flags = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of NamedBufferData function.
 *
 *          Check that INVALID_OPERATION is generated by NamedBufferData if buffer
 *          is not the name of an existing buffer object.
 *
 *          Check that INVALID_ENUM is generated by NamedBufferData if usage is not
 *          STREAM_DRAW, STREAM_READ, STREAM_COPY, STATIC_DRAW, STATIC_READ,
 *          STATIC_COPY, DYNAMIC_DRAW, DYNAMIC_READ or DYNAMIC_COPY.
 *
 *          Check that INVALID_VALUE is generated by NamedBufferData if size is
 *          negative.
 *
 *          Check that INVALID_OPERATION is generated by NamedBufferData if the
 *          BUFFER_IMMUTABLE_STORAGE flag of the buffer object is TRUE.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfNamedBufferData()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint				buffer			 = 0;
	glw::GLuint				immutable_buffer = 0;
	glw::GLubyte			dummy_data[4]	= {};
	std::stack<glw::GLuint> too_much_buffers;

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		gl.createBuffers(1, &immutable_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(immutable_buffer, sizeof(dummy_data), &dummy_data, GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pNamedBufferData(not_a_buffer_name, sizeof(dummy_data), dummy_data, GL_DYNAMIC_COPY);

			is_ok &= ErrorCheckAndLog("glNamedBufferData", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test invalid usage error behavior. */
		{
			/* Prepare for invalid type error behavior verification. */
			static const glw::GLenum valid_usages[] = { GL_STREAM_DRAW,  GL_STREAM_READ,  GL_STREAM_COPY,
														GL_STATIC_DRAW,  GL_STATIC_READ,  GL_STATIC_COPY,
														GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, GL_DYNAMIC_COPY,
														GL_NONE };
			static const glw::GLenum valid_usages_last = sizeof(valid_usages) / sizeof(valid_usages[0]) - 1;

			glw::GLenum invalid_usage = 0;

			while (&valid_usages[valid_usages_last] !=
				   std::find(&valid_usages[0], &valid_usages[valid_usages_last], (++invalid_usage)))
				;

			/* Test. */
			m_pNamedBufferData(buffer, sizeof(dummy_data), dummy_data, invalid_usage);

			is_ok &=
				ErrorCheckAndLog("glNamedBufferData", GL_INVALID_ENUM,
								 " if usage is not STREAM_DRAW, STREAM_READ, STREAM_COPY, STATIC_DRAW, STATIC_READ,"
								 " STATIC_COPY, DYNAMIC_DRAW, DYNAMIC_READ or DYNAMIC_COPY.");
		}

		/* Test negative size error behavior. */
		{
			m_pNamedBufferData(buffer, -1, dummy_data, GL_DYNAMIC_COPY);

			is_ok &= ErrorCheckAndLog("glNamedBufferData", GL_INVALID_VALUE, " if size is negative.");
		}

		/* Test immutable buffer error behavior. */
		{
			m_pNamedBufferData(immutable_buffer, sizeof(dummy_data) / 2, dummy_data, GL_DYNAMIC_COPY);

			is_ok &= ErrorCheckAndLog("glNamedBufferData", GL_INVALID_OPERATION,
									  " if the BUFFER_IMMUTABLE_STORAGE flag of the buffer object is TRUE.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	while (!too_much_buffers.empty())
	{
		glw::GLuint tmp_buffer = too_much_buffers.top();

		if (tmp_buffer)
		{
			gl.deleteBuffers(1, &tmp_buffer);
		}

		too_much_buffers.pop();
	}

	if (immutable_buffer)
	{
		gl.deleteBuffers(1, &immutable_buffer);

		immutable_buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of NamedBufferStorage function.
 *
 *  Check that INVALID_OPERATION is generated by NamedBufferStorage if
 *  buffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_VALUE is generated by NamedBufferStorage if size is
 *  less than or equal to zero.
 *
 *  Check that INVALID_VALUE is generated by NamedBufferStorage if flags has
 *  any bits set other than DYNAMIC_STORAGE_BIT, MAP_READ_BIT,
 *  MAP_WRITE_BIT, MAP_PERSISTENT_BIT, MAP_COHERENT_BIT or
 *  CLIENT_STORAGE_BIT.
 *
 *  Check that INVALID_VALUE error is generated by NamedBufferStorage if
 *  flags contains MAP_PERSISTENT_BIT but does not contain at least one of
 *  MAP_READ_BIT or MAP_WRITE_BIT.
 *
 *  Check that INVALID_VALUE is generated by NamedBufferStorage if flags
 *  contains MAP_COHERENT_BIT, but does not also contain MAP_PERSISTENT_BIT.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfNamedBufferStorage()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint				buffer		  = 0;
	glw::GLubyte			dummy_data[4] = {};
	std::stack<glw::GLuint> too_much_buffers;

	try
	{
		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pNamedBufferStorage(not_a_buffer_name, sizeof(dummy_data), dummy_data, GL_MAP_WRITE_BIT);

			is_ok &= ErrorCheckAndLog("glNamedBufferStorage", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test negative or zero size error behavior. */
		{
			/* Object creation. */
			gl.createBuffers(1, &buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

			/* Test negative size. */
			m_pNamedBufferStorage(buffer, -1, dummy_data, GL_DYNAMIC_COPY);

			is_ok &= ErrorCheckAndLog("glNamedBufferStorage", GL_INVALID_VALUE, " if size is negative.");

			/* Test zero size. */
			m_pNamedBufferStorage(buffer, 0, dummy_data, GL_DYNAMIC_COPY);

			is_ok &= ErrorCheckAndLog("glNamedBufferStorage", GL_INVALID_VALUE, " if size zero.");

			/* Clean-up. */
			gl.deleteBuffers(1, &buffer);

			buffer = 0;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers failed.");
		}

		/* Test invalid usage bit error behavior. */
		{
			/* Prepare for invalid type error behavior verification. */
			glw::GLuint valid_bits = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT |
									 GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_CLIENT_STORAGE_BIT;

			if (m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_buffer"))
			{
				valid_bits |= GL_SPARSE_STORAGE_BIT_ARB;
			}

			if (m_context.getContextInfo().isExtensionSupported("GL_NV_gpu_multicast") ||
				m_context.getContextInfo().isExtensionSupported("GL_NVX_linked_gpu_multicast"))
			{
				valid_bits |= GL_PER_GPU_STORAGE_BIT_NV;
			}

			if (m_context.getContextInfo().isExtensionSupported("GL_NVX_cross_process_interop"))
			{
				valid_bits |= GL_EXTERNAL_STORAGE_BIT_NVX;
			}

			glw::GLuint invalid_bits = ~valid_bits;

			glw::GLuint bits_count = CHAR_BIT * sizeof(invalid_bits);

			/* Test. */
			for (glw::GLuint i = 0; i < bits_count; ++i)
			{
				glw::GLuint possibly_invalid_bit = (1 << i);

				if (invalid_bits & possibly_invalid_bit)
				{
					/* Object creation. */
					gl.createBuffers(1, &buffer);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

					/* Test invalid bit. */
					m_pNamedBufferStorage(buffer, sizeof(dummy_data), dummy_data, possibly_invalid_bit);

					is_ok &=
						ErrorCheckAndLog("glNamedBufferStorage", GL_INVALID_VALUE,
										 " if flags has any bits set other than DYNAMIC_STORAGE_BIT, MAP_READ_BIT,"
										 " MAP_WRITE_BIT, MAP_PERSISTENT_BIT, MAP_COHERENT_BIT or CLIENT_STORAGE_BIT.");

					/* Release object. */
					gl.deleteBuffers(1, &buffer);

					buffer = 0;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers failed.");
				}
			}
		}

		/* Test improper persistent bit behavior error behavior. */
		{
			/* Object creation. */
			gl.createBuffers(1, &buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

			/* Test. */
			m_pNamedBufferStorage(buffer, sizeof(dummy_data), dummy_data, GL_MAP_PERSISTENT_BIT);

			is_ok &= ErrorCheckAndLog("glNamedBufferStorage", GL_INVALID_VALUE, " if flags contains MAP_PERSISTENT_BIT "
																				"but does not contain at least one of "
																				"MAP_READ_BIT or MAP_WRITE_BIT.");

			/* Clean-up. */
			gl.deleteBuffers(1, &buffer);

			buffer = 0;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers failed.");
		}

		/* Test improper persistent bit behavior error behavior. */
		{
			/* Object creation. */
			gl.createBuffers(1, &buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

			/* Test. */
			m_pNamedBufferStorage(buffer, sizeof(dummy_data), dummy_data,
								  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT);

			is_ok &=
				ErrorCheckAndLog("glNamedBufferStorage", GL_INVALID_VALUE,
								 " if flags contains MAP_COHERENT_BIT, but does not also contain MAP_PERSISTENT_BIT.");

			/* Clean-up. */
			gl.deleteBuffers(1, &buffer);

			buffer = 0;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers failed.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	while (!too_much_buffers.empty())
	{
		glw::GLuint tmp_buffer = too_much_buffers.top();

		if (tmp_buffer)
		{
			gl.deleteBuffers(1, &tmp_buffer);
		}

		too_much_buffers.pop();
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of NamedBufferSubData function.
 *
 *  Check that INVALID_OPERATION is generated by NamedBufferSubData if
 *  buffer is not the name of an existing buffer object.
 *
 *  Check that INVALID_VALUE is generated by NamedBufferSubData if offset or
 *  size is negative, or if offset+size is greater than the value of
 *  BUFFER_SIZE for the specified buffer object.
 *
 *  Check that INVALID_OPERATION is generated by NamedBufferSubData if any
 *  part of the specified range of the buffer object is mapped with
 *  MapBufferRange or MapBuffer, unless it was mapped with the
 *  MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.
 *
 *  Check that INVALID_OPERATION is generated by NamedBufferSubData if the
 *  value of the BUFFER_IMMUTABLE_STORAGE flag of the buffer object is TRUE
 *  and the value of BUFFER_STORAGE_FLAGS for the buffer object does not
 *  have the DYNAMIC_STORAGE_BIT bit set.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfNamedBufferSubData()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer					  = 0;
	glw::GLuint  immutable_storage_buffer = 0;
	glw::GLubyte dummy_data[4]			  = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data,
							  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		gl.createBuffers(1, &immutable_storage_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(immutable_storage_buffer, sizeof(dummy_data), &dummy_data, GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pNamedBufferSubData(not_a_buffer_name, 0, sizeof(dummy_data), &dummy_data);

			is_ok &= ErrorCheckAndLog("glNamedBufferSubData", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test negative offset error behavior. */
		{
			/* Test. */
			m_pNamedBufferSubData(buffer, -1, sizeof(dummy_data), &dummy_data);

			is_ok &= ErrorCheckAndLog("glNamedBufferSubData", GL_INVALID_VALUE, " if offset or size is negative.");
		}

		/* Test negative size error behavior. */
		{
			/* Test. */
			m_pNamedBufferSubData(buffer, 0, -1, &dummy_data);

			is_ok &= ErrorCheckAndLog("glNamedBufferSubData", GL_INVALID_VALUE, " if offset or size is negative.");
		}

		/* Test size overflow error behavior. */
		{
			/* Test. */
			m_pNamedBufferSubData(buffer, 0, sizeof(dummy_data) * 2, &dummy_data);

			is_ok &= ErrorCheckAndLog(
				"glNamedBufferSubData", GL_INVALID_VALUE,
				" if offset+size is greater than the value of BUFFER_SIZE for the specified buffer object.");
		}

		/* Test offset+size overflow error behavior. */
		{
			/* Test. */
			m_pNamedBufferSubData(buffer, sizeof(dummy_data) / 2, sizeof(dummy_data), &dummy_data);

			is_ok &= ErrorCheckAndLog(
				"glNamedBufferSubData", GL_INVALID_VALUE,
				" if offset+size is greater than the value of BUFFER_SIZE for the specified buffer object.");
		}

		/* Test of mapped buffer subdata error behavior verification (with glMapBuffer). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBuffer(buffer, GL_READ_ONLY);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pNamedBufferSubData(buffer, 0, sizeof(dummy_data), &dummy_data);

			is_ok &= ErrorCheckAndLog("glNamedBufferSubData", GL_INVALID_OPERATION,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBuffer, unless it was mapped with "
									  "the MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of mapped buffer subdata error behavior verification (with glMapBufferRange). */
		{
			(void)(glw::GLbyte*) m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pNamedBufferSubData(buffer, 0, sizeof(dummy_data), &dummy_data);

			is_ok &= ErrorCheckAndLog("glNamedBufferSubData", GL_INVALID_OPERATION,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBufferRange, unless it was mapped with "
									  "the MAP_PERSISTENT_BIT bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test of persistently mapped buffer clear error with behavior verification. */
		{
			(void)(glw::GLbyte*)
				m_pMapNamedBufferRange(buffer, 0, sizeof(dummy_data), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBuffer failed.");

			m_pNamedBufferSubData(buffer, 0, sizeof(dummy_data), &dummy_data);

			is_ok &= ErrorCheckAndLog("glNamedBufferSubData", GL_NO_ERROR,
									  " if any part of the specified range of the buffer"
									  " object is mapped with MapBuffer with the MAP_PERSISTENT_BIT"
									  " bit set in the MapBufferRange access flags.");

			m_pUnmapNamedBuffer(buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapNamedBuffer failed.");
		}

		/* Test DYNAMIC_STORAGE_BIT bit off immutable buffer not set error behavior. */
		{
			/* Test. */
			m_pNamedBufferSubData(immutable_storage_buffer, 0, sizeof(dummy_data), &dummy_data);

			is_ok &= ErrorCheckAndLog("glNamedBufferSubData", GL_INVALID_OPERATION,
									  " if the value of the BUFFER_IMMUTABLE_STORAGE flag of the buffer object is TRUE"
									  " and the value of BUFFER_STORAGE_FLAGS for the buffer object does not"
									  " have the DYNAMIC_STORAGE_BIT bit set.");
		}

		/* Test DYNAMIC_STORAGE_BIT bit off immutable buffer set no error behavior. */
		{
			/* Test. */
			m_pNamedBufferSubData(buffer, 0, sizeof(dummy_data), &dummy_data);

			is_ok &= ErrorCheckAndLog("glNamedBufferSubData", GL_NO_ERROR,
									  " if the value of the BUFFER_IMMUTABLE_STORAGE flag of the buffer object is TRUE"
									  " and the value of BUFFER_STORAGE_FLAGS for the buffer object"
									  " have the DYNAMIC_STORAGE_BIT bit set.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (immutable_storage_buffer)
	{
		gl.deleteBuffers(1, &immutable_storage_buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Test Errors Of UnmapNamedBuffer function.
 *
 *  Check that INVALID_OPERATION is generated by UnmapNamedBuffer if buffer
 *  is not the name of an existing buffer object.
 *
 *  Check that INVALID_OPERATION is generated by UnmapNamedBuffer if the
 *  buffer object is not in a mapped state.
 *
 *  True if test case succeeded, false otherwise.
 */
bool ErrorsTest::TestErrorsOfUnmapNamedBuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return value. */
	bool is_ok			= true;
	bool internal_error = false;

	/* Common variables. */
	glw::GLuint  buffer		   = 0;
	glw::GLubyte dummy_data[4] = {};

	try
	{
		/* Common preparations. */
		gl.createBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers failed.");

		m_pNamedBufferStorage(buffer, sizeof(dummy_data), &dummy_data,
							  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_PERSISTENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBuffeStorage failed.");

		/* Test invalid buffer name error behavior. */
		{
			/* Prepare for invalid buffer name error behavior verification. */
			glw::GLuint not_a_buffer_name = 0;

			while (gl.isBuffer(++not_a_buffer_name))
				;

			/* Test. */
			m_pUnmapNamedBuffer(not_a_buffer_name);

			is_ok &= ErrorCheckAndLog("glUnmapNamedBuffer", GL_INVALID_OPERATION,
									  " if buffer is not the name of an existing buffer object.");
		}

		/* Test not mapped buffer error behavior verification. */
		{
			m_pUnmapNamedBuffer(buffer);

			is_ok &= ErrorCheckAndLog("glUnmapNamedBuffer", GL_INVALID_OPERATION,
									  " if the buffer object is not in a mapped state.");
		}
	}
	catch (...)
	{
		is_ok		   = false;
		internal_error = true;
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);

		buffer = 0;
	}

	if (internal_error)
	{
		throw 0;
	}

	return is_ok;
}

/******************************** Functional Test Implementation   ********************************/

/** @brief Vertex shader source code */
const glw::GLchar FunctionalTest::s_vertex_shader[] = "#version 450\n"
													  "\n"
													  "in  int data_in;\n"
													  "out int data_out;\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
													  "\n"
													  "    data_out = data_in * data_in;\n"
													  "}\n";

/** @brief Fragment shader source code */
const glw::GLchar FunctionalTest::s_fragment_shader[] = "#version 450\n"
														"\n"
														"out vec4 color;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    color = vec4(0.0);\n"
														"}\n";

const glw::GLchar FunctionalTest::s_vertex_shader_input_name[] =
	"data_in"; //!< Vertex shader's name of the input attribute.

const glw::GLchar* FunctionalTest::s_vertex_shader_output_name =
	"data_out"; //!< Vertex shader's name of the transform feedback varying.

const glw::GLint FunctionalTest::s_initial_data_a[] = {
	1, 2, 3, 4, 5, 5
}; //!< Initial data to be uploaded for the input buffer.

const glw::GLint FunctionalTest::s_initial_data_b[] = {
	0, 0, 0, 0, 0, 0, 36
}; //!< Initial data to be uploaded for the output buffer.

const glw::GLint FunctionalTest::s_expected_data[] = {
	0, 1, 4, 9, 16, 25, 36
}; //!< Expected result which shall be read from output buffer.

/** @brief Functional Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
FunctionalTest::FunctionalTest(deqp::Context& context)
	: deqp::TestCase(context, "buffers_functional", "Buffer Objects Functional Test")
	, m_pClearNamedBufferData(DE_NULL)
	, m_pClearNamedBufferSubData(DE_NULL)
	, m_pCopyNamedBufferSubData(DE_NULL)
	, m_pFlushMappedNamedBufferRange(DE_NULL)
	, m_pGetNamedBufferParameteri64v(DE_NULL)
	, m_pGetNamedBufferParameteriv(DE_NULL)
	, m_pGetNamedBufferPointerv(DE_NULL)
	, m_pGetNamedBufferSubData(DE_NULL)
	, m_pMapNamedBuffer(DE_NULL)
	, m_pMapNamedBufferRange(DE_NULL)
	, m_pNamedBufferData(DE_NULL)
	, m_pNamedBufferStorage(DE_NULL)
	, m_pNamedBufferSubData(DE_NULL)
	, m_pUnmapNamedBuffer(DE_NULL)
	, m_po(0)
	, m_vao(0)
	, m_bo_in(0)
	, m_bo_out(0)
	, m_attrib_location(-1)
{
	/* Intentionally left blank. */
}

/** @brief Run Functional Test.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult FunctionalTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* API function pointers setup. */
	m_pClearNamedBufferData		   = (PFNGLCLEARNAMEDBUFFERDATA)gl.clearNamedBufferData;
	m_pClearNamedBufferSubData	 = (PFNGLCLEARNAMEDBUFFERSUBDATA)gl.clearNamedBufferSubData;
	m_pCopyNamedBufferSubData	  = (PFNGLCOPYNAMEDBUFFERSUBDATA)gl.copyNamedBufferSubData;
	m_pFlushMappedNamedBufferRange = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGE)gl.flushMappedNamedBufferRange;
	m_pGetNamedBufferParameteri64v = (PFNGLGETNAMEDBUFFERPARAMETERI64V)gl.getNamedBufferParameteri64v;
	m_pGetNamedBufferParameteriv   = (PFNGLGETNAMEDBUFFERPARAMETERIV)gl.getNamedBufferParameteriv;
	m_pGetNamedBufferPointerv	  = (PFNGLGETNAMEDBUFFERPOINTERV)gl.getNamedBufferPointerv;
	m_pGetNamedBufferSubData	   = (PFNGLGETNAMEDBUFFERSUBDATA)gl.getNamedBufferSubData;
	m_pMapNamedBuffer			   = (PFNGLMAPNAMEDBUFFER)gl.mapNamedBuffer;
	m_pMapNamedBufferRange		   = (PFNGLMAPNAMEDBUFFERRANGE)gl.mapNamedBufferRange;
	m_pNamedBufferData			   = (PFNGLNAMEDBUFFERDATA)gl.namedBufferData;
	m_pNamedBufferStorage		   = (PFNGLNAMEDBUFFERSTORAGE)gl.namedBufferStorage;
	m_pNamedBufferSubData		   = (PFNGLNAMEDBUFFERSUBDATA)gl.namedBufferSubData;
	m_pUnmapNamedBuffer			   = (PFNGLUNMAPNAMEDBUFFER)gl.unmapNamedBuffer;

	try
	{
		/* API function pointers check. */
		if ((DE_NULL == m_pClearNamedBufferData) || (DE_NULL == m_pClearNamedBufferSubData) ||
			(DE_NULL == m_pCopyNamedBufferSubData) || (DE_NULL == m_pFlushMappedNamedBufferRange) ||
			(DE_NULL == m_pGetNamedBufferParameteri64v) || (DE_NULL == m_pGetNamedBufferParameteriv) ||
			(DE_NULL == m_pGetNamedBufferPointerv) || (DE_NULL == m_pGetNamedBufferSubData) ||
			(DE_NULL == m_pMapNamedBuffer) || (DE_NULL == m_pMapNamedBufferRange) || (DE_NULL == m_pNamedBufferData) ||
			(DE_NULL == m_pNamedBufferStorage) || (DE_NULL == m_pNamedBufferSubData) ||
			(DE_NULL == m_pUnmapNamedBuffer))
		{
			throw 0;
		}

		/* Running test. */
		BuildProgram();
		PrepareVertexArrayObject();

		is_ok = is_ok && PrepareInputBuffer();
		is_ok = is_ok && PrepareOutputBuffer();

		if (is_ok)
		{
			Draw();
		}

		is_ok = is_ok && CheckArrayBufferImmutableFlag();
		is_ok = is_ok && CheckTransformFeedbackBufferSize();
		is_ok = is_ok && CheckTransformFeedbackResult();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean Up. */
	Cleanup();

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Build test's GL program */
void FunctionalTest::BuildProgram()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* const source;
		glw::GLenum const		 type;
		glw::GLuint				 id;
	} shader[] = { { s_vertex_shader, GL_VERTEX_SHADER, 0 }, { s_fragment_shader, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		m_po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, 1, &(shader[i].source), NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader compilation has failed.\n"
														<< "Shader type: " << glu::getShaderTypeStr(shader[i].type)
														<< "\n"
														<< "Shader compilation error log:\n"
														<< log_text << "\n"
														<< "Shader source code:\n"
														<< shader[i].source << "\n"
														<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

					throw 0;
				}
			}
		}

		/* Tranform feedback varying */
		gl.transformFeedbackVaryings(m_po, 1, &s_vertex_shader_output_name, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		/* Link. */
		gl.linkProgram(m_po);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_po, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_po, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(m_po, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(m_po, log_size, NULL, &log_text[0]);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linkage has failed due to:\n"
												<< log_text << "\n"
												<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			throw 0;
		}
	}
	catch (...)
	{
		if (m_po)
		{
			gl.deleteProgram(m_po);

			m_po = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	if (m_po)
	{
		gl.useProgram(m_po);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");
	}

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Prepare empty vertex array object and bind it. */
void FunctionalTest::PrepareVertexArrayObject()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

/** Prepare input buffer in the way described in test specification (see class comment). */
bool FunctionalTest::PrepareInputBuffer()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Constants. */
	static const glw::GLint zero = 0;
	static const glw::GLint one  = 1;

	/* Buffer preparation */
	gl.createBuffers(1, &m_bo_in);

	if (GL_NO_ERROR == gl.getError())
	{
		/* Storage and last (6th) element preparation. */
		m_pNamedBufferStorage(m_bo_in, sizeof(s_initial_data_a), s_initial_data_a,
							  GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_PERSISTENT_BIT);

		if (GL_NO_ERROR == gl.getError())
		{
			/* First element preparation. */
			m_pClearNamedBufferSubData(m_bo_in, GL_R8, 0, sizeof(glw::GLint), GL_RED, GL_INT, &zero);

			if (GL_NO_ERROR == gl.getError())
			{
				/* Second element preparation. */
				m_pNamedBufferSubData(m_bo_in, 1 /* 2nd element */ * sizeof(glw::GLint), sizeof(glw::GLint), &one);

				if (GL_NO_ERROR == gl.getError())
				{
					/* Third element preparation. */
					glw::GLint* p = (glw::GLint*)m_pMapNamedBuffer(m_bo_in, GL_WRITE_ONLY);

					if ((GL_NO_ERROR == gl.getError()) || (DE_NULL == p))
					{
						p[2] = 2;

						m_pUnmapNamedBuffer(m_bo_in);

						if (GL_NO_ERROR == gl.getError())
						{
							/* Fifth element preparation. */
							m_pCopyNamedBufferSubData(m_bo_in, m_bo_in, sizeof(glw::GLint) * 3, sizeof(glw::GLint) * 4,
													  sizeof(glw::GLint));

							if (GL_NO_ERROR == gl.getError())
							{
								/* Fourth element preparation. */
								p = (glw::GLint*)m_pMapNamedBufferRange(
									m_bo_in, sizeof(glw::GLint) * 3, sizeof(glw::GLint),
									GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

								if (GL_NO_ERROR == gl.getError())
								{
									/* Write to mapped buffer. */
									*p = 3;

									/* Flush test. */
									m_pFlushMappedNamedBufferRange(m_bo_in, 0, sizeof(glw::GLint));

									if (GL_NO_ERROR == gl.getError())
									{
										/* Mapped Buffer Pointer query. */
										glw::GLvoid* is_p = DE_NULL;
										m_pGetNamedBufferPointerv(m_bo_in, GL_BUFFER_MAP_POINTER, &is_p);

										if (GL_NO_ERROR == gl.getError())
										{
											/* Mapped Buffer pointer query check. */
											if (p == is_p)
											{
												/* Unmapping. */
												m_pUnmapNamedBuffer(m_bo_in);

												if (GL_NO_ERROR == gl.getError())
												{
													/* Setup buffer as input for vertex shader. */
													m_attrib_location =
														gl.getAttribLocation(m_po, s_vertex_shader_input_name);
													GLU_EXPECT_NO_ERROR(gl.getError(),
																		"glGetAttribLocation call failed.");

													gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_in);
													GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

													gl.vertexAttribIPointer(m_attrib_location, 1, GL_INT, 0, NULL);
													GLU_EXPECT_NO_ERROR(gl.getError(),
																		"glVertexAttribIPointer call failed.");

													gl.enableVertexAttribArray(m_attrib_location);
													GLU_EXPECT_NO_ERROR(gl.getError(),
																		"glEnableVertexAttribArray call failed.");

													return true;
												}
												else
												{
													m_context.getTestContext().getLog()
														<< tcu::TestLog::Message << "UnmapNamedBuffer has failed."
														<< tcu::TestLog::EndMessage;
												}
											}
											else
											{
												m_pUnmapNamedBuffer(m_bo_in);
												m_context.getTestContext().getLog()
													<< tcu::TestLog::Message
													<< "Pointer returned by GetNamedBufferPointerv is not proper."
													<< tcu::TestLog::EndMessage;
											}
										}
										else
										{
											m_pUnmapNamedBuffer(m_bo_in);
											m_context.getTestContext().getLog() << tcu::TestLog::Message
																				<< "GetNamedBufferPointerv has failed."
																				<< tcu::TestLog::EndMessage;
										}
									}
									else
									{
										m_pUnmapNamedBuffer(m_bo_in);
										m_context.getTestContext().getLog() << tcu::TestLog::Message
																			<< "FlushMappedNamedBufferRange has failed."
																			<< tcu::TestLog::EndMessage;
									}
								}
								else
								{
									m_context.getTestContext().getLog() << tcu::TestLog::Message
																		<< "MapNamedBufferRange has failed."
																		<< tcu::TestLog::EndMessage;
								}
							}
							else
							{
								m_context.getTestContext().getLog() << tcu::TestLog::Message
																	<< "CopyNamedBufferSubData has failed."
																	<< tcu::TestLog::EndMessage;
							}
						}
						else
						{
							m_context.getTestContext().getLog()
								<< tcu::TestLog::Message << "UnmapNamedBuffer has failed." << tcu::TestLog::EndMessage;
						}
					}
					else
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "MapNamedBuffer has failed."
															<< tcu::TestLog::EndMessage;
					}
				}
				else
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "NamedBufferSubData has failed."
														<< tcu::TestLog::EndMessage;
				}
			}
			else
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "ClearNamedBufferSubData has failed."
													<< tcu::TestLog::EndMessage;
			}
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "NamedBufferStorage has failed."
												<< tcu::TestLog::EndMessage;
		}
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "CreateBuffers has failed."
											<< tcu::TestLog::EndMessage;
	}

	return false;
}

/** Prepare output buffer in the way described in test specification (see class comment). */
bool FunctionalTest::PrepareOutputBuffer()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer preparation */
	gl.genBuffers(1, &m_bo_out);

	if (GL_NO_ERROR == gl.getError())
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_out);

		if (GL_NO_ERROR == gl.getError())
		{
			m_pNamedBufferData(m_bo_out, sizeof(s_initial_data_b), s_initial_data_b, GL_DYNAMIC_COPY);

			if (GL_NO_ERROR == gl.getError())
			{
				gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_out, 0,
								   sizeof(s_initial_data_a) /* intentionally sizeof(a) < sizeof(b) */);

				if (GL_NO_ERROR == gl.getError())
				{
					return true;
				}
				else
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "BindBufferRange has failed."
														<< tcu::TestLog::EndMessage;
					throw 0; /* This function is not being tested, throw test internal error */
				}
			}
			else
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "NamedBufferData has failed."
													<< tcu::TestLog::EndMessage;
			}
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "BindBuffer has failed."
												<< tcu::TestLog::EndMessage;
			throw 0; /* This function is not being tested, throw test internal error */
		}
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GenBuffers has failed."
											<< tcu::TestLog::EndMessage;
		throw 0; /* This function is not being tested, throw test internal error */
	}

	return false;
}

/** Draw with the test program and transform feedback. */
void FunctionalTest::Draw()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Draw using transform feedback. */
	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_POINTS, 0, sizeof(s_initial_data_a) / sizeof(s_initial_data_a[0]));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");
}

/** @brief Check that input buffer is immutable using GetNamedBufferParameteriv function. */
bool FunctionalTest::CheckArrayBufferImmutableFlag()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Local query storage. */
	glw::GLint is_storage_immutable = -1;

	/* Querry. */
	m_pGetNamedBufferParameteriv(m_bo_in, GL_BUFFER_IMMUTABLE_STORAGE, &is_storage_immutable);

	/* Error checking. */
	if (GL_NO_ERROR == gl.getError())
	{
		/* Return value checking. */
		if (-1 != is_storage_immutable)
		{
			/* Test. */
			if (GL_TRUE == is_storage_immutable)
			{
				return true;
			}
			else
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "Input buffer storage is unexpectedly mutable."
													<< tcu::TestLog::EndMessage;
			}
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "GetNamedBufferParameteriv has not returned a data."
												<< tcu::TestLog::EndMessage;
		}
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetNamedBufferParameteriv has failed."
											<< tcu::TestLog::EndMessage;
	}

	return false;
}

/** @brief Check that output buffer size using GetNamedBufferParameteri64v function. */
bool FunctionalTest::CheckTransformFeedbackBufferSize()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Local query storage. */
	glw::GLint64 size = -1;

	/* Querry. */
	m_pGetNamedBufferParameteri64v(m_bo_out, GL_BUFFER_SIZE, &size);

	/* Error checking. */
	if (GL_NO_ERROR == gl.getError())
	{
		/* Return value checking. */
		if (-1 != size)
		{
			/* Test. */
			if (sizeof(s_initial_data_b) == size)
			{
				return true;
			}
			else
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Output buffer size is " << size
													<< ", but " << sizeof(s_initial_data_b) << " was expected."
													<< tcu::TestLog::EndMessage;
			}
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "GetNamedBufferParameteri64v has not returned a data."
												<< tcu::TestLog::EndMessage;
		}
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetNamedBufferParameteri64v has failed."
											<< tcu::TestLog::EndMessage;
	}

	return false;
}

/** @brief Check that results of the test are equal to the expected reference values. */
bool FunctionalTest::CheckTransformFeedbackResult()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Local data storage. */
	glw::GLint output_data[sizeof(s_initial_data_b) / sizeof(s_initial_data_b[0])] = {};

	/* Fetch data. */
	m_pGetNamedBufferSubData(m_bo_out, 0, sizeof(output_data), output_data);

	/* Error checking. */
	if (GL_NO_ERROR == gl.getError())
	{
		for (glw::GLuint i = 0; i < sizeof(s_initial_data_b) / sizeof(s_initial_data_b[0]); ++i)
		{
			if (s_expected_data[i] != output_data[i])
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Expected data is not equal to results."
													<< tcu::TestLog::EndMessage;
				return false;
			}
		}

		return true;
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetNamedBufferSubData has failed."
											<< tcu::TestLog::EndMessage;
	}
	return false;
}

/** Clean all test's GL objects and state. */
void FunctionalTest::Cleanup()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleanup objects. */
	if (m_po)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	if (0 <= m_attrib_location)
	{
		gl.disableVertexAttribArray(m_attrib_location);

		m_attrib_location = -1;
	}

	if (m_bo_in)
	{
		gl.deleteBuffers(1, &m_bo_in);

		m_bo_in = 0;
	}

	if (m_bo_out)
	{
		gl.deleteBuffers(1, &m_bo_out);

		m_bo_out = 0;
	}

	/* Make sure that rasterizer is turned on (default). */
	gl.enable(GL_RASTERIZER_DISCARD);

	/* Clean all errors. */
	while (gl.getError())
		;
}

} /* Buffers namespace. */
} /* DirectStateAccess namespace. */
} /* gl4cts namespace. */
