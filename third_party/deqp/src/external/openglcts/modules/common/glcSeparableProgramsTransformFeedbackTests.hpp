#ifndef _GLCSEPARABLEPROGRAMSTRANSFORMFEEDBACKTESTS_HPP
#define _GLCSEPARABLEPROGRAMSTRANSFORMFEEDBACKTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file  glcSeparableProgramXFBTests.hpp
 * \brief
 */ /*--------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

namespace glcts
{

/** Test group which encapsulates conformance tests that verify if the set of
	 *  attributes captured in transform feedback mode is taken from the program object
	 *  active on the upstream shader when the separable program objects are in use.
*/
class SeparableProgramsTransformFeedbackTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	SeparableProgramsTransformFeedbackTests(deqp::Context& context);

	void init(void);

private:
	SeparableProgramsTransformFeedbackTests(const SeparableProgramsTransformFeedbackTests& other);
	SeparableProgramsTransformFeedbackTests& operator=(const SeparableProgramsTransformFeedbackTests& other);
};

} /* glcts namespace */

#endif // _GLCSEPARABLEPROGRAMSTRANSFORMFEEDBACKTESTS_HPP
