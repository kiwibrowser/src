#ifndef _GLCNODEFAULTCONTEXTPACKAGE_HPP
#define _GLCNODEFAULTCONTEXTPACKAGE_HPP
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
 * \file
 * \brief OpenGL Conformance Test Package that does not have predefined GL context.
 */ /*-------------------------------------------------------------------*/

#include "tcuTestPackage.hpp"

namespace glcts
{

class NoDefaultContextPackage : public tcu::TestPackage
{
public:
	NoDefaultContextPackage(tcu::TestContext& testCtx, const char* name);
	virtual ~NoDefaultContextPackage(void);

	void init(void);

	tcu::Archive* getArchive(void)
	{
		return &m_testCtx.getRootArchive();
	}

	virtual tcu::TestCaseExecutor* createExecutor(void) const;

private:
	NoDefaultContextPackage(const NoDefaultContextPackage& other);
	NoDefaultContextPackage& operator=(const NoDefaultContextPackage& other);
};

} // glcts

#endif // _GLCNODEFAULTCONTEXTPACKAGE_HPP
