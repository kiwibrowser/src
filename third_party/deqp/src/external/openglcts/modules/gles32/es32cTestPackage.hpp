#ifndef _ES32CTESTPACKAGE_HPP
#define _ES32CTESTPACKAGE_HPP
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
 * \brief OpenGL ES 3.2 Test Package.
 */ /*-------------------------------------------------------------------*/

#include "glcTestPackage.hpp"
#include "tcuDefs.hpp"

namespace es32cts
{

class ES32TestPackage : public deqp::TestPackage
{
public:
	ES32TestPackage(tcu::TestContext& testCtx, const char* packageName);
	~ES32TestPackage(void);

	void init(void);

	virtual tcu::TestCaseExecutor* createExecutor(void) const;

private:
	ES32TestPackage(const ES32TestPackage& other);
	ES32TestPackage& operator=(const ES32TestPackage& other);
};

} // es3cts

#endif // _ES32CTESTPACKAGE_HPP
