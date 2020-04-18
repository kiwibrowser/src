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
 * \file  gl4cMapBufferAlignmentTests.cpp
 * \brief Implements conformance tests for "Map Buffer Alignment" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cMapBufferAlignmentTests.hpp"

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include <algorithm>
#include <vector>

using namespace glw;

namespace gl4cts
{
namespace MapBufferAlignment
{
/** Implementation of Query test, description follows:
 *
 * Verify that GetInteger returns at least 64 when MIN_MAP_BUFFER_ALIGNEMENT is
 * requested.
 **/
class Query : public deqp::TestCase
{
public:
	/* Public methods */
	Query(deqp::Context& context) : TestCase(context, "query", "Verifies value of MIN_MAP_BUFFER_ALIGNEMENT")
	{
		/* Nothing to be done */
	}
	virtual ~Query()
	{
		/* Nothing to be done */
	}

	/** Execute test
	 *
	 * @return tcu::TestNode::STOP
	 **/
	virtual tcu::TestNode::IterateResult iterate(void);

	static const GLint m_min_map_buffer_alignment = 64;
};

/** Implementation of Functional test, description follows:
 *
 * Verifies that results of MapBuffer operations are as required.
 *
 * Steps:
 * - prepare buffer filled with specific content;
 * - map buffer with MapBuffer;
 * - verify that returned data match contents of the buffer;
 * - unmap buffer;
 * - map buffer with MapBufferRange;
 * - verify that returned data match contents of the buffer;
 * - unmap buffer;
 * - verify that pointers returned by map operations fulfil alignment
 * requirements.
 *
 * Repeat steps for all valid:
 * - <buffer> values;
 * - <access> combinations.
 *
 * <offset> should be set to MIN_MAP_BUFFER_ALIGNEMENT - 1.
 **/
class Functional : public deqp::TestCase
{
public:
	/* Public methods */
	Functional(deqp::Context& context)
		: TestCase(context, "functional", "Verifies alignment of memory returned by MapBuffer operations")
	{
		/* Nothing to be done */
	}

	virtual ~Functional()
	{
		/* Nothing to be done */
	}

	/** Execute test
	 *
	 * @return tcu::TestNode::STOP
	 **/
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult Query::iterate()
{
	GLint min_map_buffer_alignment = 0;
	bool  test_result			   = true;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.getIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &min_map_buffer_alignment);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	if (m_min_map_buffer_alignment > min_map_buffer_alignment)
	{
		test_result = false;
	}

	/* Set result */
	if (true == test_result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

struct BufferEnums
{
	GLenum m_target;
	GLenum m_max_size;
};

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult Functional::iterate()
{
	static const GLenum storage_flags[] = {
		GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT,
		GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
		GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT, GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
		GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
		GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT,
		GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT,
		GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
		GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT,
		GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
		GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
		GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT,
		GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT,
		GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
		GL_CLIENT_STORAGE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT,
		GL_CLIENT_STORAGE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
		GL_CLIENT_STORAGE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
		GL_CLIENT_STORAGE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT,
		GL_CLIENT_STORAGE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT,
		GL_CLIENT_STORAGE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT |
			GL_MAP_WRITE_BIT,
	};

	static const size_t n_storage_flags = sizeof(storage_flags) / sizeof(storage_flags[0]);

	static const BufferEnums buffers[] = {
		{ GL_ARRAY_BUFFER, GL_MAX_VARYING_COMPONENTS },
		{ GL_ATOMIC_COUNTER_BUFFER, GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE },
		{ GL_COPY_READ_BUFFER, 0 },
		{ GL_COPY_WRITE_BUFFER, 0 },
		{ GL_DISPATCH_INDIRECT_BUFFER, 0 },
		{ GL_DRAW_INDIRECT_BUFFER, 0 },
		{ GL_ELEMENT_ARRAY_BUFFER, GL_MAX_ELEMENTS_INDICES },
		{ GL_PIXEL_PACK_BUFFER, 0 },
		{ GL_PIXEL_UNPACK_BUFFER, 0 },
		{ GL_QUERY_BUFFER, 0 },
		{ GL_SHADER_STORAGE_BUFFER, GL_MAX_SHADER_STORAGE_BLOCK_SIZE },
		{ GL_TEXTURE_BUFFER, GL_MAX_TEXTURE_BUFFER_SIZE },
		{ GL_TRANSFORM_FEEDBACK_BUFFER, GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS },
		{ GL_UNIFORM_BUFFER, GL_MAX_UNIFORM_BLOCK_SIZE }
	};
	static const size_t n_buffers = sizeof(buffers) / sizeof(buffers[0]);

	const Functions& gl = m_context.getRenderContext().getFunctions();

	std::vector<GLubyte> buffer_data;
	size_t				 buffer_data_size		  = 0;
	GLuint				 buffer_id				  = 0;
	GLint				 buffer_size			  = 0;
	GLint				 min_map_buffer_alignment = 0;
	GLuint				 offset					  = 0;
	bool				 test_result			  = true;

	/* Get min alignment */
	gl.getIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &min_map_buffer_alignment);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Prepare storage */
	buffer_data_size = 2 * min_map_buffer_alignment;
	buffer_data.resize(buffer_data_size);

	/* Prepare data */
	for (size_t i = 0; i < buffer_data_size; ++i)
	{
		buffer_data[i] = (GLubyte)i;
	}

	/* Run test */
	try
	{
		for (size_t buffer_idx = 0; buffer_idx < n_buffers; ++buffer_idx)
		{
			const BufferEnums& buffer = buffers[buffer_idx];

			buffer_size = static_cast<GLint>(buffer_data_size);

			/* Get max size */
			if (0 != buffer.m_max_size)
			{
				gl.getIntegerv(buffer.m_max_size, &buffer_size);
				GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");
			}

			switch (buffer.m_max_size)
			{
			case GL_MAX_VARYING_COMPONENTS:
			case GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:
				buffer_size = static_cast<glw::GLint>(buffer_size * sizeof(GLfloat));
				break;

			case GL_MAX_ELEMENTS_INDICES:
				buffer_size = static_cast<glw::GLint>(buffer_size * sizeof(GLuint));
				break;

			default:
				break;
			}

			buffer_size = std::min(buffer_size, (GLint)buffer_data_size);
			offset		= std::min(buffer_size - 1, min_map_buffer_alignment - 1);

			for (size_t set_idx = 0; set_idx < n_storage_flags; ++set_idx)
			{
				const GLenum& storage_set = storage_flags[set_idx];

				/* Prepare buffer */
				gl.genBuffers(1, &buffer_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

				gl.bindBuffer(buffer.m_target, buffer_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

				gl.bufferStorage(buffer.m_target, buffer_size, &buffer_data[0], storage_set);
				GLU_EXPECT_NO_ERROR(gl.getError(), "BufferStorage");

				/* Test MapBuffer */
				GLenum map_buffer_access = GL_READ_WRITE;
				if (0 == (storage_set & GL_MAP_READ_BIT))
				{
					map_buffer_access = GL_WRITE_ONLY;
				}
				else if (0 == (storage_set & GL_MAP_WRITE_BIT))
				{
					map_buffer_access = GL_READ_ONLY;
				}

				GLubyte* map_buffer_ptr = (GLubyte*)gl.mapBuffer(buffer.m_target, map_buffer_access);
				GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

				if (GL_WRITE_ONLY != map_buffer_access)
				{
					for (size_t i = 0; i < (size_t)buffer_size; ++i)
					{
						if (buffer_data[i] != map_buffer_ptr[i])
						{
							test_result = false;
							break;
						}
					}
				}

				gl.unmapBuffer(buffer.m_target);
				GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

				/* Test MapBufferRange */
				static const GLenum map_buffer_range_access_mask = GL_DYNAMIC_STORAGE_BIT | GL_CLIENT_STORAGE_BIT;
				GLenum				map_buffer_range_access		 = (storage_set & (~map_buffer_range_access_mask));

				GLubyte* map_buffer_range_ptr =
					(GLubyte*)gl.mapBufferRange(buffer.m_target, offset, buffer_size - offset, map_buffer_range_access);
				GLU_EXPECT_NO_ERROR(gl.getError(), "MapBufferRange");

				if (0 != (GL_MAP_READ_BIT & map_buffer_range_access))
				{
					for (size_t i = 0; i < (size_t)buffer_size - (size_t)offset; ++i)
					{
						if (buffer_data[i + offset] != map_buffer_range_ptr[i])
						{
							test_result = false;
							break;
						}
					}
				}

				gl.unmapBuffer(buffer.m_target);
				GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

				gl.bindBuffer(buffer.m_target, 0 /* id */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

				/* Remove buffer */
				gl.deleteBuffers(1, &buffer_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteBuffers");

				buffer_id = 0;

				/* Verify that pointers are properly aligned */
				if (0 != ((GLintptr)map_buffer_ptr % min_map_buffer_alignment))
				{
					test_result = false;
					break;
				}

				if (0 != (((GLintptr)map_buffer_range_ptr - offset) % min_map_buffer_alignment))
				{
					test_result = false;
					break;
				}
			}
		}
	}
	catch (const std::exception& exc)
	{
		if (0 != buffer_id)
		{
			gl.deleteBuffers(1, &buffer_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteBuffers");
		}

		TCU_FAIL(exc.what());
	}

	/* Set result */
	if (true == test_result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}
} /* MapBufferAlignment namespace */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
MapBufferAlignmentTests::MapBufferAlignmentTests(deqp::Context& context)
	: TestCaseGroup(context, "map_buffer_alignment", "Verifies \"map buffer alignment\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void MapBufferAlignmentTests::init(void)
{
	addChild(new MapBufferAlignment::Query(m_context));
	addChild(new MapBufferAlignment::Functional(m_context));
}

} /* gl4cts namespace */
