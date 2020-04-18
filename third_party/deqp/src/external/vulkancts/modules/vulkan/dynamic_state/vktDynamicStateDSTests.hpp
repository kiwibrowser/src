#ifndef _VKTDYNAMICSTATEDSTESTS_HPP
#define _VKTDYNAMICSTATEDSTESTS_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Intel Corporation
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
 * \brief Dynamic State Depth Stencil Tests
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vktTestCase.hpp"

namespace vkt
{
namespace DynamicState
{

class DynamicStateDSTests : public tcu::TestCaseGroup
{
public:
							DynamicStateDSTests		(tcu::TestContext& testCtx);
							~DynamicStateDSTests	(void);
	void					init					(void);

private:
	DynamicStateDSTests								(const DynamicStateDSTests& other);
	DynamicStateDSTests&	operator=				(const DynamicStateDSTests& other);
};

} // DynamicState
} // vkt

#endif // _VKTDYNAMICSTATEDSTESTS_HPP
