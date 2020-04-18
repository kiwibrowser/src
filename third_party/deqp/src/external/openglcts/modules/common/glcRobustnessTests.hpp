#ifndef _GLCROBUSTNESSTESTS_HPP
#define _GLCROBUSTNESSTESTS_HPP
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
 * \file  glcRobustnessTests.hpp
 * \brief Conformance tests for the Robustness feature functionality.
 */ /*-----------------------------------------------------------------------------*/

/* Includes. */

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

#include "gluDefs.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "glcContext.hpp"
#include "glcTestCase.hpp"
#include "glcTestPackage.hpp"

namespace glcts
{

class RobustnessTests : public tcu::TestCaseGroup
{
public:
	RobustnessTests(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~RobustnessTests(void)
	{
	}

	virtual void init(void);

private:
	RobustnessTests(const RobustnessTests& other);
	RobustnessTests& operator=(const RobustnessTests& other);

	glu::ApiType m_ApiType;
};

} // namespace glcts

#endif // _GLCROBUSTNESSTESTS_HPP
