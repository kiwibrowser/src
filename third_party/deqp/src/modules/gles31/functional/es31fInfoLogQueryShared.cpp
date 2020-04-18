/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Info log query shared utilities
 *//*--------------------------------------------------------------------*/

#include "es31fInfoLogQueryShared.hpp"
#include "glsStateQueryUtil.hpp"
#include "tcuTestLog.hpp"
#include "glwEnums.hpp"
#include "gluStrUtil.hpp"

#include <string>

namespace deqp
{
namespace gles31
{
namespace Functional
{

void verifyInfoLogQuery (tcu::ResultCollector&			result,
						 glu::CallLogWrapper&			gl,
						 int							logLen,
						 glw::GLuint					object,
						 void (glu::CallLogWrapper::*	getInfoLog)(glw::GLuint, glw::GLsizei, glw::GLsizei*, glw::GLchar*),
						 const char*					getterName)
{
	{
		const tcu::ScopedLogSection	section	(gl.getLog(), "QueryAll", "Query all");
		std::string					buf		(logLen + 2, 'X');

		buf[logLen + 1] = '\0';
		(gl.*getInfoLog)(object, logLen, DE_NULL, &buf[0]);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), getterName);

		if (logLen > 0 && buf[logLen-1] != '\0')
			result.fail("Return buffer was not INFO_LOG_LENGTH sized and null-terminated");
		else if (buf[logLen] != 'X' && buf[logLen+1] != '\0')
			result.fail("Buffer end guard modified, query wrote over the end of the buffer.");
	}

	{
		const tcu::ScopedLogSection section	(gl.getLog(), "QueryMore", "Query more");
		std::string					buf		(logLen + 4, 'X');
		int							written	= -1;

		buf[logLen + 3] = '\0';
		(gl.*getInfoLog)(object, logLen+2, &written, &buf[0]);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), getterName);

		if (written == -1)
			result.fail("'length' was not written to");
		else if (buf[written] != '\0')
			result.fail("Either length was incorrect or result was not null-terminated");
		else if (logLen != 0 && (written + 1) > logLen)
			result.fail("'length' characters + null terminator is larger than INFO_LOG_LENGTH");
		else if ((written + 1) < logLen)
			result.fail("'length' is not consistent with INFO_LOG_LENGTH");
		else if (buf[logLen+2] != 'X' && buf[logLen+3] != '\0')
			result.fail("Buffer end guard modified, query wrote over the end of the buffer.");
		else if (written != (int)strlen(&buf[0]))
			result.fail("'length' and written string length do not match");
	}

	if (logLen > 2)
	{
		const tcu::ScopedLogSection section	(gl.getLog(), "QueryLess", "Query less");
		std::string					buf		(logLen + 2, 'X');
		int							written	= -1;

		(gl.*getInfoLog)(object, 2, &written, &buf[0]);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), getterName);

		if (written == -1)
			result.fail("'length' was not written to");
		else if (written != 1)
			result.fail("Expected 'length' = 1");
		else if (buf[1] != '\0')
			result.fail("Expected null terminator at index 1");
	}

	if (logLen > 0)
	{
		const tcu::ScopedLogSection section	(gl.getLog(), "QueryOne", "Query one character");
		std::string					buf		(logLen + 2, 'X');
		int							written	= -1;

		(gl.*getInfoLog)(object, 1, &written, &buf[0]);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), getterName);

		if (written == -1)
			result.fail("'length' was not written to");
		else if (written != 0)
			result.fail("Expected 'length' = 0");
		else if (buf[0] != '\0')
			result.fail("Expected null terminator at index 0");
	}

	{
		const tcu::ScopedLogSection section	(gl.getLog(), "QueryNone", "Query to zero-sized buffer");
		std::string					buf		(logLen + 2, 'X');
		int							written	= -1;

		(gl.*getInfoLog)(object, 0, &written, &buf[0]);
		GLS_COLLECT_GL_ERROR(result, gl.glGetError(), getterName);

		if (written == -1)
			result.fail("'length' was not written to");
		else if (written != 0)
			result.fail("Expected 'length' = 0");
		else if (buf[0] != 'X')
			result.fail("Unexpected buffer mutation at index 0");
	}
}

} // Functional
} // gles31
} // deqp
