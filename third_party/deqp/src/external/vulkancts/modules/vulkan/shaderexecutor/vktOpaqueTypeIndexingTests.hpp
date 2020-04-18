#ifndef _VKTOPAQUETYPEINDEXINGTESTS_HPP
#define _VKTOPAQUETYPEINDEXINGTESTS_HPP
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
 * \brief Opaque type (sampler, buffer, atomic counter, ...) indexing tests.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuTestCase.hpp"

namespace vkt
{
namespace shaderexecutor
{

tcu::TestCaseGroup*		createOpaqueTypeIndexingTests	(tcu::TestContext& testCtx);

} // shaderexecutor
} // vkt

#endif // _VKTOPAQUETYPEINDEXINGTESTS_HPP
