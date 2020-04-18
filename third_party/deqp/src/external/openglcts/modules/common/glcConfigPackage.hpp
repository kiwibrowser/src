#ifndef _GLCCONFIGPACKAGE_HPP
#define _GLCCONFIGPACKAGE_HPP
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
 * \brief OpenGL Conformance Test Configuration List Package
 */ /*-------------------------------------------------------------------*/

#include "glcTestCaseWrapper.hpp"
#include "tcuDefs.hpp"
#include "tcuTestPackage.hpp"

namespace glcts
{

class ConfigPackage : public tcu::TestPackage
{
public:
	ConfigPackage(tcu::TestContext& testCtx, const char* name);
	virtual ~ConfigPackage(void);

	void init(void);

	tcu::Archive* getArchive(void)
	{
		return &m_testCtx.getRootArchive();
	}

	virtual tcu::TestCaseExecutor* createExecutor(void) const;

private:
	ConfigPackage(const ConfigPackage& other);
	ConfigPackage& operator=(const ConfigPackage& other);
};

} // deqp

#endif // _GLCCONFIGPACKAGE_HPP
