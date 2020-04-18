#ifndef _GLCCONFIGLISTCASE_HPP
#define _GLCCONFIGLISTCASE_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Test config list case.
 */ /*-------------------------------------------------------------------*/

#include "gluRenderContext.hpp"
#include "tcuDefs.hpp"
#include "tcuTestCase.hpp"

namespace glcts
{

class ConfigListCase : public tcu::TestCase
{
public:
	ConfigListCase(tcu::TestContext& testCtx, const char* name, const char* description, glu::ApiType type);
	~ConfigListCase(void);

	IterateResult iterate(void);

private:
	glu::ApiType m_ctxType;
};

} // glcts

#endif // _GLCCONFIGLISTCASE_HPP
