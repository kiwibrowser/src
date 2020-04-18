#ifndef _GL4CTESTPACKAGES_HPP
#define _GL4CTESTPACKAGES_HPP
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
 * \brief OpenGL 4.x Test Packages.
 */ /*-------------------------------------------------------------------*/

#include "gl3cTestPackages.hpp"
#include "tcuDefs.hpp"

namespace gl4cts
{

class GL40TestPackage : public gl3cts::GL33TestPackage
{
public:
	GL40TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 4.0 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(4, 0, glu::PROFILE_CORE));

	~GL40TestPackage(void);

	void init(void);
};

class GL41TestPackage : public GL40TestPackage
{
public:
	GL41TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 4.1 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(4, 1, glu::PROFILE_CORE));

	~GL41TestPackage(void);

	void init(void);
};

class GL42TestPackage : public GL41TestPackage
{
public:
	GL42TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 4.2 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(4, 2, glu::PROFILE_CORE));

	~GL42TestPackage(void);

	void init(void);
};

class GL43TestPackage : public GL42TestPackage
{
public:
	GL43TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 4.3 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(4, 3, glu::PROFILE_CORE));

	~GL43TestPackage(void);

	void init(void);
};

class GL44TestPackage : public GL43TestPackage
{
public:
	GL44TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 4.4 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(4, 4, glu::PROFILE_CORE));

	~GL44TestPackage(void);

	void init(void);
};

class GL45TestPackage : public GL44TestPackage
{
public:
	GL45TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 4.5 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(4, 5, glu::PROFILE_CORE));

	~GL45TestPackage(void);

	void init(void);
};

class GL46TestPackage : public GL45TestPackage
{
public:
	GL46TestPackage(tcu::TestContext& testCtx, const char* packageName,
					const char*		 description	   = "OpenGL 4.6 Conformance Tests",
					glu::ContextType renderContextType = glu::ContextType(4, 6, glu::PROFILE_CORE));

	~GL46TestPackage(void);

	void init(void);
};

} // gl4cts

#endif // _GL4CTESTPACKAGES_HPP
