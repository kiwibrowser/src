#ifndef _GL4CSHADERATOMICCOUNTERSTESTS_HPP
#define _GL4CSHADERATOMICCOUNTERSTESTS_HPP
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

#include "glcTestSubcase.hpp"
#include "tcuDefs.hpp"

namespace gl4cts
{

class ShaderAtomicCountersTests : public deqp::TestCaseGroup
{
public:
	ShaderAtomicCountersTests(deqp::Context& context);
	~ShaderAtomicCountersTests(void);

	void init(void);

private:
	ShaderAtomicCountersTests(const ShaderAtomicCountersTests& other);
	ShaderAtomicCountersTests& operator=(const ShaderAtomicCountersTests& other);
};

} // gl4cts

#endif // _GL4CSHADERATOMICCOUNTERSTESTS_HPP
