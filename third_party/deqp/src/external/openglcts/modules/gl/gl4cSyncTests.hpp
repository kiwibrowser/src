#ifndef _GL4CSYNCTESTS_HPP
#define _GL4CSYNCTESTS_HPP
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
 * \file  gl4cSyncTests.hpp
 * \brief Declares test classes for synchronization functionality.
 */ /*-------------------------------------------------------------------*/

/* Includes. */

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

namespace gl4cts
{
namespace Sync
{
/** @class Tests
 *
 *  @brief Direct State Access test group.
 */
class Tests : public deqp::TestCaseGroup
{
public:
	/* Public member functions */
	Tests(deqp::Context& context);

	void init();

private:
	/* Private member functions */
	Tests(const Tests& other);
	Tests& operator=(const Tests& other);
};
/* Tests class */

/** @class SyncFlushCommandsTest
 *
 *  Description:
 *
 *      This test verifies that ClientWaitSync called with SYNC_FLUSH_COMMANDS_BIT flag
 *      behaves like Flush was inserted immediately after the creation of sync. This shall
 *      happen in finite time (OpenGL 4.5 Core Profile, Chapter 4.1.2).
 *
 *  Steps:
 *
 *      Prepare first buffer with reference data.
 *
 *      Create second buffer with null data and persistent coherent storage.
 *
 *      Map second buffer for read with persistent and coherent flags.
 *
 *      Copy first buffer to second buffer using Copy*BufferSubData function.
 *
 *      Create synchronization object using FenceSync.
 *
 *      Use ClientWaitSync, with SYNC_FLUSH_COMMANDS_BIT flag and 16 seconds timeout, to
 *      wait for synchronization being done. Check for errors - expect NO_ERROR. Expect
 *      no timeout, but if it happen return test timeout result with indication that
 *      tests possibly fails due to not returning after finite time.
 *
 *      If ClientWaitSync succeeded, compare queried data with the reference. Expect
 *      equality.
 *
 *      Unmap second buffer.
 */
class FlushCommandsTest : public deqp::TestCase
{
public:
	/* Public member functions */
	FlushCommandsTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	FlushCommandsTest(const FlushCommandsTest& other);
	FlushCommandsTest& operator=(const FlushCommandsTest& other);
};

/* FlushCommandsTest class */
} /* Sync namespace */
} /* gl4cts */

#endif // _GL4CSYNCTESTS_HPP
