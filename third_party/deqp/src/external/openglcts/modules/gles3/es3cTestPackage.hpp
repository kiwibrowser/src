#ifndef _ES3CTESTPACKAGE_HPP
#define _ES3CTESTPACKAGE_HPP
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
 * \brief OpenGL ES 3 Test Package.
 */ /*-------------------------------------------------------------------*/

#include "glcTestPackage.hpp"
#include "tcuDefs.hpp"

namespace es3cts
{

class ES30TestPackage : public deqp::TestPackage
{
public:
	ES30TestPackage(tcu::TestContext& testCtx, const char* packageName);
	~ES30TestPackage(void);

	void init(void);

	virtual tcu::TestCaseExecutor* createExecutor(void) const;

private:
	ES30TestPackage(const ES30TestPackage& other);
	ES30TestPackage& operator=(const ES30TestPackage& other);
};

} // es3cts

#endif // _ES3CTESTPACKAGE_HPP
