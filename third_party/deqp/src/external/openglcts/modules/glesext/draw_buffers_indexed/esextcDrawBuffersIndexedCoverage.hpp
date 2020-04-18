#ifndef _ESEXTCDRAWBUFFERSINDEXEDCOVERAGE_HPP
#define _ESEXTCDRAWBUFFERSINDEXEDCOVERAGE_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \brief
 */ /*-------------------------------------------------------------------*/

/*!
 * \file  esextcDrawBuffersIndexedCoverage.hpp
 * \brief Draw Buffers Indexed tests 1. Coverage
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

/** 1. Coverage
 **/
class DrawBuffersIndexedCoverage : public TestCaseBase
{
public:
	/** Public methods
	 **/
	DrawBuffersIndexedCoverage(Context& context, const ExtParameters& extParams, const char* name,
							   const char* description);

	virtual ~DrawBuffersIndexedCoverage()
	{
	}

private:
	/** Private methods
	 **/
	virtual void		  init();
	virtual IterateResult iterate();
};

} // namespace glcts

#endif // _ESEXTCDRAWBUFFERSINDEXEDCOVERAGE_HPP
