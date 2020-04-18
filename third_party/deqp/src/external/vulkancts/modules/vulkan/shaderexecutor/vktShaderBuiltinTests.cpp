/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Vulkan shader render test cases
 *//*--------------------------------------------------------------------*/

#include "vktShaderBuiltinTests.hpp"

#include "deUniquePtr.hpp"

#include "vktShaderBuiltinPrecisionTests.hpp"
#include "vktShaderCommonFunctionTests.hpp"
#include "vktShaderIntegerFunctionTests.hpp"
#include "vktShaderPackingFunctionTests.hpp"

namespace vkt
{
namespace shaderexecutor
{

tcu::TestCaseGroup* createBuiltinTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	builtinTests			(new tcu::TestCaseGroup(testCtx, "builtin",		"Built-in tests"));
	de::MovePtr<tcu::TestCaseGroup>	builtinFunctionTests	(new tcu::TestCaseGroup(testCtx, "function",	"Built-in Function Tests"));

	builtinFunctionTests->addChild(new ShaderCommonFunctionTests(testCtx));
	builtinFunctionTests->addChild(new ShaderIntegerFunctionTests(testCtx));
	builtinFunctionTests->addChild(new ShaderPackingFunctionTests(testCtx));

	builtinTests->addChild(builtinFunctionTests.release());
	builtinTests->addChild(new BuiltinPrecisionTests(testCtx));

	return builtinTests.release();
}

} // shaderexecutor
} // vkt
