#ifndef _VKTSHADERCOMMONFUNCTIONTESTS_HPP
#define _VKTSHADERCOMMONFUNCTIONTESTS_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
 * \brief Common built-in function tests.
 *//*--------------------------------------------------------------------*/

#include "tcuTestCase.hpp"

namespace vkt
{
namespace shaderexecutor
{

// ShaderCommonFunctionTests

class ShaderCommonFunctionTests : public tcu::TestCaseGroup
{
public:
										ShaderCommonFunctionTests	(tcu::TestContext& testCtx);
	virtual								~ShaderCommonFunctionTests	(void);

	virtual void						init						(void);

private:
										ShaderCommonFunctionTests	(const ShaderCommonFunctionTests&);		// not allowed!
	ShaderCommonFunctionTests&			operator=					(const ShaderCommonFunctionTests&);		// not allowed!
};

} // shaderexecutor
} // vkt

#endif // _VKTSHADERCOMMONFUNCTIONTESTS_HPP
