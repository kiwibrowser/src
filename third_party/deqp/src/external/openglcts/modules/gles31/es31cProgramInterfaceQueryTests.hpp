#ifndef _ES31CPROGRAMINTERFACEQUERYTESTS_HPP
#define _ES31CPROGRAMINTERFACEQUERYTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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

#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"

#define ARRAY_SIZE(OBJ) (sizeof(OBJ) / sizeof(OBJ)[0])

namespace glcts
{
class ProgramInterfaceQueryTests : public glcts::TestCaseGroup
{
public:
	ProgramInterfaceQueryTests(glcts::Context& context);
	~ProgramInterfaceQueryTests(void);

	void init(void);

private:
	ProgramInterfaceQueryTests(const ProgramInterfaceQueryTests& other);
	ProgramInterfaceQueryTests& operator=(const ProgramInterfaceQueryTests& other);
};
}

#endif // _ES31CPROGRAMINTERFACEQUERYTESTS_HPP
