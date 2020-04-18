#ifndef _VKTPIPELINEMULTISAMPLEBASERESOLVE_HPP
#define _VKTPIPELINEMULTISAMPLEBASERESOLVE_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \file vktPipelineMultisampleBaseResolve.hpp
 * \brief Base class for tests that check results of multisample resolve
 *//*--------------------------------------------------------------------*/

#include "vktPipelineMultisampleBase.hpp"
#include "vktTestCase.hpp"
#include "tcuVector.hpp"

namespace vkt
{
namespace pipeline
{
namespace multisample
{

class MSCaseBaseResolve : public MultisampleCaseBase
{
public:
	MSCaseBaseResolve	(tcu::TestContext&		testCtx,
						 const std::string&		name,
						 const ImageMSParams&	imageMSParams)
		: MultisampleCaseBase(testCtx, name, imageMSParams)
	{}
};

class MSInstanceBaseResolve : public MultisampleInstanceBase
{
public:
							MSInstanceBaseResolve	(Context&							context,
													 const ImageMSParams&				imageMSParams)
								: MultisampleInstanceBase(context, imageMSParams)
							{}

protected:

	tcu::TestStatus			iterate					(void);

	virtual tcu::TestStatus	verifyImageData			(const vk::VkImageCreateInfo&		imageRSInfo,
													 const tcu::ConstPixelBufferAccess&	dataRS) const = 0;
};

} // multisample
} // pipeline
} // vkt

#endif // _VKTPIPELINEMULTISAMPLEBASERESOLVE_HPP
