#ifndef _VKTPIPELINEMULTISAMPLEBASERESOLVEANDPERSAMPLEFETCH_HPP
#define _VKTPIPELINEMULTISAMPLEBASERESOLVEANDPERSAMPLEFETCH_HPP
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
 * \file vktPipelineMultisampleBaseResolveAndPerSampleFetch.hpp
 * \brief Base class for tests that check results of multisample resolve
 *		  and/or values of individual samples
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

class MSCaseBaseResolveAndPerSampleFetch : public MultisampleCaseBase
{
public:
		MSCaseBaseResolveAndPerSampleFetch	(tcu::TestContext&		testCtx,
											 const std::string&		name,
											 const ImageMSParams&	imageMSParams)
		: MultisampleCaseBase(testCtx, name, imageMSParams) {}

	void initPrograms						(vk::SourceCollections&	programCollection) const;
};

class MSInstanceBaseResolveAndPerSampleFetch : public MultisampleInstanceBase
{
public:
							MSInstanceBaseResolveAndPerSampleFetch					(Context&											context,
																					 const ImageMSParams&								imageMSParams)
							: MultisampleInstanceBase(context, imageMSParams) {}

protected:

	tcu::TestStatus										iterate						(void);

	virtual vk::VkPipelineMultisampleStateCreateInfo	getMSStateCreateInfo		(const ImageMSParams&								imageMSParams) const;

	virtual const vk::VkDescriptorSetLayout*			createMSPassDescSetLayout	(const ImageMSParams&								imageMSParams);

	virtual const vk::VkDescriptorSet*					createMSPassDescSet			(const ImageMSParams&								imageMSParams,
																					 const vk::VkDescriptorSetLayout*					descSetLayout);

	virtual tcu::TestStatus								verifyImageData				(const vk::VkImageCreateInfo&						imageMSInfo,
																					 const vk::VkImageCreateInfo&						imageRSInfo,
																					 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																					 const tcu::ConstPixelBufferAccess&					dataRS) const = 0;
};

} // multisample
} // pipeline
} // vkt

#endif // _VKTPIPELINEMULTISAMPLEBASERESOLVEANDPERSAMPLEFETCH_HPP
