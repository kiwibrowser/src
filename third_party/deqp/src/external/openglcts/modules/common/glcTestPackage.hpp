#ifndef _GLCTESTPACKAGE_HPP
#define _GLCTESTPACKAGE_HPP
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
 * \brief OpenGL Conformance Test Package Base Class
 */ /*-------------------------------------------------------------------*/

#include "glcContext.hpp"
#include "glcTestCaseWrapper.hpp"
#include "tcuDefs.hpp"
#include "tcuResource.hpp"
#include "tcuTestPackage.hpp"

namespace deqp
{

class PackageContext
{
public:
	PackageContext(tcu::TestContext& testCtx, glu::ContextType renderContextType);
	~PackageContext(void);

	Context& getContext(void)
	{
		return m_context;
	}
	TestCaseWrapper& getTestCaseWrapper(void)
	{
		return m_caseWrapper;
	}

private:
	Context			m_context;
	TestCaseWrapper m_caseWrapper;
};

class TestPackage : public tcu::TestPackage
{
public:
	TestPackage(tcu::TestContext& testCtx, const char* name, const char* description,
				glu::ContextType renderContextType, const char* resourcesPath);
	virtual ~TestPackage(void);

	void init(void);
	void deinit(void);

	TestCaseWrapper& getTestCaseWrapper(void)
	{
		return m_packageCtx->getTestCaseWrapper();
	}
	tcu::Archive* getArchive(void)
	{
		return &m_archive;
	}

	Context& getContext(void)
	{
		return m_packageCtx->getContext();
	}

private:
	TestPackage(const TestPackage& other);
	TestPackage& operator=(const TestPackage& other);

	glu::ContextType m_renderContextType;

	PackageContext*		m_packageCtx;
	tcu::ResourcePrefix m_archive;
};

} // deqp

#endif // _GLCTESTPACKAGE_HPP
