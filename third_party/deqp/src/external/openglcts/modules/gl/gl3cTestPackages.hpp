#ifndef _GL3CTESTPACKAGES_HPP
#define _GL3CTESTPACKAGES_HPP
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
 * \brief OpenGL 3.x Test Packages.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glcTestPackage.hpp"
#include "gluRenderContext.hpp"
#include "tcuDefs.hpp"

namespace gl3cts
{

class GL30TestPackage : public deqp::TestPackage
{
public:
	GL30TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 3.0 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(3, 0, glu::PROFILE_CORE));

	~GL30TestPackage(void);

	tcu::TestCaseExecutor* createExecutor(void) const;

	void init(void);

	using deqp::TestPackage::getContext;
};

class GL31TestPackage : public GL30TestPackage
{
public:
	GL31TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 3.1 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(3, 1, glu::PROFILE_CORE));

	~GL31TestPackage(void);

	void init(void);
};

class GL32TestPackage : public GL31TestPackage
{
public:
	GL32TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 3.2 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(3, 2, glu::PROFILE_CORE));
	~GL32TestPackage(void);

	void init(void);
};

class GL33TestPackage : public GL32TestPackage
{
public:
	GL33TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 3.3 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(3, 3, glu::PROFILE_CORE));
	~GL33TestPackage(void);

	void init(void);
};

} // gl3cts

#endif // _GL3CTESTPACKAGES_HPP
