#ifndef _VKTPIPELINEMULTISAMPLEBASE_HPP
#define _VKTPIPELINEMULTISAMPLEBASE_HPP
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
 * \file vktPipelineMultisampleBase.hpp
 * \brief Multisample Tests Base Classes
 *//*--------------------------------------------------------------------*/

#include "vktPipelineMultisampleTestsUtil.hpp"
#include "vktTestCase.hpp"
#include "tcuVector.hpp"

namespace vkt
{
namespace pipeline
{
namespace multisample
{

struct ImageMSParams
{
	ImageMSParams(const vk::VkSampleCountFlagBits samples, const tcu::UVec3& size) : numSamples(samples), imageSize(size) {}

	vk::VkSampleCountFlagBits	numSamples;
	tcu::UVec3					imageSize;
};

class MultisampleCaseBase : public TestCase
{
public:
	MultisampleCaseBase	(tcu::TestContext&		testCtx,
						 const std::string&		name,
						 const ImageMSParams&	imageMSParams)
		: TestCase(testCtx, name, "")
		, m_imageMSParams(imageMSParams)
	{}

protected:
	const ImageMSParams m_imageMSParams;
};

typedef MultisampleCaseBase* (*MultisampleCaseFuncPtr)(tcu::TestContext& testCtx, const std::string& name, const ImageMSParams& imageMSParams);

class MultisampleInstanceBase : public TestInstance
{
public:
								MultisampleInstanceBase		(Context&							context,
															 const ImageMSParams&				imageMSParams)
		: TestInstance		(context)
		, m_imageMSParams	(imageMSParams)
		, m_imageType		(IMAGE_TYPE_2D)
		, m_imageFormat		(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8))
	{}

	typedef std::vector<vk::VkVertexInputAttributeDescription> VertexAttribDescVec;

	struct VertexDataDesc
	{
		vk::VkPrimitiveTopology	primitiveTopology;
		deUint32				verticesCount;
		deUint32				dataStride;
		vk::VkDeviceSize		dataSize;
		VertexAttribDescVec		vertexAttribDescVec;
	};

protected:

	void					validateImageSize			(const vk::InstanceInterface&	instance,
														 const vk::VkPhysicalDevice		physicalDevice,
														 const ImageType				imageType,
														 const tcu::UVec3&				imageSize) const;

	void					validateImageFeatureFlags	(const vk::InstanceInterface&	instance,
														 const vk::VkPhysicalDevice		physicalDevice,
														 const vk::VkFormat				format,
														 const vk::VkFormatFeatureFlags	featureFlags) const;

	void					validateImageInfo			(const vk::InstanceInterface&	instance,
														 const vk::VkPhysicalDevice		physicalDevice,
														 const vk::VkImageCreateInfo&	imageInfo) const;

	virtual VertexDataDesc	getVertexDataDescripton		(void) const = 0;

	virtual void			uploadVertexData			(const vk::Allocation&			vertexBufferAllocation,
														 const VertexDataDesc&			vertexDataDescripton) const = 0;
protected:
	const ImageMSParams			m_imageMSParams;
	const ImageType				m_imageType;
	const tcu::TextureFormat	m_imageFormat;
};

} // multisample

template <class CaseClass>
tcu::TestCaseGroup* makeMSGroup	(tcu::TestContext&							testCtx,
								 const std::string							groupName,
								 const tcu::UVec3							imageSizes[],
								 const deUint32								imageSizesElemCount,
								 const vk::VkSampleCountFlagBits			imageSamples[],
								 const deUint32								imageSamplesElemCount)
{
	de::MovePtr<tcu::TestCaseGroup> caseGroup(new tcu::TestCaseGroup(testCtx, groupName.c_str(), ""));

	for (deUint32 imageSizeNdx = 0u; imageSizeNdx < imageSizesElemCount; ++imageSizeNdx)
	{
		const tcu::UVec3	imageSize = imageSizes[imageSizeNdx];
		std::ostringstream	imageSizeStream;

		imageSizeStream << imageSize.x() << "_" << imageSize.y() << "_" << imageSize.z();

		de::MovePtr<tcu::TestCaseGroup> sizeGroup(new tcu::TestCaseGroup(testCtx, imageSizeStream.str().c_str(), ""));

		for (deUint32 imageSamplesNdx = 0u; imageSamplesNdx < imageSamplesElemCount; ++imageSamplesNdx)
		{
			const vk::VkSampleCountFlagBits		samples = imageSamples[imageSamplesNdx];
			const multisample::ImageMSParams	imageMSParams = multisample::ImageMSParams(samples, imageSize);

			sizeGroup->addChild(CaseClass::createCase(testCtx, "samples_" + de::toString(samples), imageMSParams));
		}

		caseGroup->addChild(sizeGroup.release());
	}
	return caseGroup.release();
}

} // pipeline
} // vkt

#endif // _VKTPIPELINEMULTISAMPLEBASE_HPP
