#ifndef _GL4CMAPBUFFERALIGNMENTTESTS_HPP
#define _GL4CMAPBUFFERALIGNMENTTESTS_HPP
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
 * \file  gl4cMapBufferAlignmentTests.hpp
 * \brief Declares test classes for "Map Buffer Alignment" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

namespace gl4cts
{
/** Group class for Map Buffer Alignment conformance tests */
class MapBufferAlignmentTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	MapBufferAlignmentTests(deqp::Context& context);

	virtual ~MapBufferAlignmentTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	MapBufferAlignmentTests(const MapBufferAlignmentTests& other);
	MapBufferAlignmentTests& operator=(const MapBufferAlignmentTests& other);
};

} /* gl4cts */

#endif // _GL4CMAPBUFFERALIGNMENTTESTS_HPP
