#ifndef _VKTDRAWINDIRECTTEST_HPP
#define _VKTDRAWINDIRECTTEST_HPP
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
 * \brief Draw Indirect Test
 *//*--------------------------------------------------------------------*/

#include "vktTestCase.hpp"

namespace vkt
{
namespace Draw
{
class IndirectDrawTests : public tcu::TestCaseGroup
{
public:
						IndirectDrawTests		(tcu::TestContext &testCtx);
						~IndirectDrawTests		(void);
	void				init					(void);

private:
	IndirectDrawTests							(const IndirectDrawTests &other);
	IndirectDrawTests&	operator=				(const IndirectDrawTests &other);

};
} // Draw
} // vkt

#endif // _VKTDRAWINDIRECTTEST_HPP
