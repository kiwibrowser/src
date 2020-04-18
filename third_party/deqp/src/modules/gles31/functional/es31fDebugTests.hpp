#ifndef _ES31FDEBUGTESTS_HPP
#define _ES31FDEBUGTESTS_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Debug output (KHR_debug) tests
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"
#include "glwDefs.hpp"
#include "gluCallLogWrapper.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

class DebugTests : public TestCaseGroup
{
public:
					DebugTests				(Context& context);
	virtual			~DebugTests				(void) {}

	virtual void	init					(void);

private:
					DebugTests				(const DebugTests& other);
	DebugTests&		operator=				(const DebugTests& other);
};

} // Functional
} // gles31
} // deqp

#endif // _ES31FDEBUGTESTS_HPP
