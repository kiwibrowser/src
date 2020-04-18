#ifndef _VKTROBUSTNESSVERTEXACCESSTESTS_HPP
#define _VKTROBUSTNESSVERTEXACCESSTESTS_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Imagination Technologies Ltd.
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
 * \brief Robust Vertex Buffer Access Tests
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vkDefs.hpp"
#include "vktTestCase.hpp"

namespace vkt
{
namespace robustness
{

tcu::TestCaseGroup* createVertexAccessTests (tcu::TestContext& testCtx);

} // robustness
} // vkt

#endif // _VKTROBUSTNESSVERTEXACCESSTESTS_HPP
