#ifndef _ES31FSHADERBUILTINCONSTANTTESTS_HPP
#define _ES31FSHADERBUILTINCONSTANTTESTS_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Shader built-in constant tests.
 *//*--------------------------------------------------------------------*/

#include "tes31TestCase.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

class ShaderBuiltinConstantTests : public TestCaseGroup
{
public:
								ShaderBuiltinConstantTests	(Context& context);
	virtual						~ShaderBuiltinConstantTests	(void);

	virtual void				init						(void);

private:
								ShaderBuiltinConstantTests	(const ShaderBuiltinConstantTests&);
	ShaderBuiltinConstantTests&	operator=					(const ShaderBuiltinConstantTests&);
};

} // Functional
} // gles31
} // deqp

#endif // _ES31FSHADERBUILTINCONSTANTTESTS_HPP
