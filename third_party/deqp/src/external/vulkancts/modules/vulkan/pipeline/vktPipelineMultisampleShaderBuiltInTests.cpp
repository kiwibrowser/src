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
*//*
* \file vktPipelineMultisampleShaderBuiltInTests.cpp
* \brief Multisample Shader BuiltIn Tests
*//*--------------------------------------------------------------------*/

#include "vktPipelineMultisampleShaderBuiltInTests.hpp"
#include "vktPipelineMultisampleBaseResolveAndPerSampleFetch.hpp"
#include "vktPipelineMakeUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkQueryUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTestLog.hpp"

namespace vkt
{
namespace pipeline
{
namespace multisample
{

using namespace vk;

struct VertexDataNdc
{
	VertexDataNdc (const tcu::Vec4& posNdc) : positionNdc(posNdc) {}

	tcu::Vec4 positionNdc;
};

MultisampleInstanceBase::VertexDataDesc getVertexDataDescriptonNdc (void)
{
	MultisampleInstanceBase::VertexDataDesc vertexDataDesc;

	vertexDataDesc.verticesCount	 = 4u;
	vertexDataDesc.dataStride		 = sizeof(VertexDataNdc);
	vertexDataDesc.dataSize			 = vertexDataDesc.verticesCount * vertexDataDesc.dataStride;
	vertexDataDesc.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	const VkVertexInputAttributeDescription vertexAttribPositionNdc =
	{
		0u,											// deUint32	location;
		0u,											// deUint32	binding;
		VK_FORMAT_R32G32B32A32_SFLOAT,				// VkFormat	format;
		DE_OFFSET_OF(VertexDataNdc, positionNdc),	// deUint32	offset;
	};

	vertexDataDesc.vertexAttribDescVec.push_back(vertexAttribPositionNdc);

	return vertexDataDesc;
}

void uploadVertexDataNdc (const Allocation& vertexBufferAllocation, const MultisampleInstanceBase::VertexDataDesc& vertexDataDescripton)
{
	std::vector<VertexDataNdc> vertices;

	vertices.push_back(VertexDataNdc(tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f)));
	vertices.push_back(VertexDataNdc(tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f)));
	vertices.push_back(VertexDataNdc(tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f)));
	vertices.push_back(VertexDataNdc(tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f)));

	deMemcpy(vertexBufferAllocation.getHostPtr(), dataPointer(vertices), static_cast<std::size_t>(vertexDataDescripton.dataSize));
}

struct VertexDataNdcScreen
{
	VertexDataNdcScreen (const tcu::Vec4& posNdc, const tcu::Vec2& posScreen) : positionNdc(posNdc), positionScreen(posScreen) {}

	tcu::Vec4 positionNdc;
	tcu::Vec2 positionScreen;
};

MultisampleInstanceBase::VertexDataDesc getVertexDataDescriptonNdcScreen (void)
{
	MultisampleInstanceBase::VertexDataDesc vertexDataDesc;

	vertexDataDesc.verticesCount	 = 4u;
	vertexDataDesc.dataStride		 = sizeof(VertexDataNdcScreen);
	vertexDataDesc.dataSize			 = vertexDataDesc.verticesCount * vertexDataDesc.dataStride;
	vertexDataDesc.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	const VkVertexInputAttributeDescription vertexAttribPositionNdc =
	{
		0u,													// deUint32	location;
		0u,													// deUint32	binding;
		VK_FORMAT_R32G32B32A32_SFLOAT,						// VkFormat	format;
		DE_OFFSET_OF(VertexDataNdcScreen, positionNdc),		// deUint32	offset;
	};

	vertexDataDesc.vertexAttribDescVec.push_back(vertexAttribPositionNdc);

	const VkVertexInputAttributeDescription vertexAttribPositionScreen =
	{
		1u,													// deUint32	location;
		0u,													// deUint32	binding;
		VK_FORMAT_R32G32_SFLOAT,							// VkFormat	format;
		DE_OFFSET_OF(VertexDataNdcScreen, positionScreen),	// deUint32	offset;
	};

	vertexDataDesc.vertexAttribDescVec.push_back(vertexAttribPositionScreen);

	return vertexDataDesc;
}

void uploadVertexDataNdcScreen (const Allocation& vertexBufferAllocation, const MultisampleInstanceBase::VertexDataDesc& vertexDataDescripton, const tcu::Vec2& screenSize)
{
	std::vector<VertexDataNdcScreen> vertices;

	vertices.push_back(VertexDataNdcScreen(tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f), tcu::Vec2(0.0f, 0.0f)));
	vertices.push_back(VertexDataNdcScreen(tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f), tcu::Vec2(screenSize.x(), 0.0f)));
	vertices.push_back(VertexDataNdcScreen(tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f), tcu::Vec2(0.0f, screenSize.y())));
	vertices.push_back(VertexDataNdcScreen(tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f), tcu::Vec2(screenSize.x(), screenSize.y())));

	deMemcpy(vertexBufferAllocation.getHostPtr(), dataPointer(vertices), static_cast<std::size_t>(vertexDataDescripton.dataSize));
}

bool checkForErrorMS (const vk::VkImageCreateInfo& imageMSInfo, const std::vector<tcu::ConstPixelBufferAccess>& dataPerSample, const deUint32 errorCompNdx)
{
	const deUint32 numSamples = static_cast<deUint32>(imageMSInfo.samples);

	for (deUint32 z = 0u; z < imageMSInfo.extent.depth;  ++z)
	for (deUint32 y = 0u; y < imageMSInfo.extent.height; ++y)
	for (deUint32 x = 0u; x < imageMSInfo.extent.width;  ++x)
	{
		for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
		{
			const deUint32 errorComponent = dataPerSample[sampleNdx].getPixelUint(x, y, z)[errorCompNdx];

			if (errorComponent > 0)
				return true;
		}
	}

	return false;
}

bool checkForErrorRS (const vk::VkImageCreateInfo& imageRSInfo, const tcu::ConstPixelBufferAccess& dataRS, const deUint32 errorCompNdx)
{
	for (deUint32 z = 0u; z < imageRSInfo.extent.depth;  ++z)
	for (deUint32 y = 0u; y < imageRSInfo.extent.height; ++y)
	for (deUint32 x = 0u; x < imageRSInfo.extent.width;  ++x)
	{
		const deUint32 errorComponent = dataRS.getPixelUint(x, y, z)[errorCompNdx];

		if (errorComponent > 0)
			return true;
	}

	return false;
}

template <typename CaseClassName>
class MSCase : public MSCaseBaseResolveAndPerSampleFetch
{
public:
								MSCase			(tcu::TestContext&		testCtx,
												 const std::string&		name,
												 const ImageMSParams&	imageMSParams)
								: MSCaseBaseResolveAndPerSampleFetch(testCtx, name, imageMSParams) {}

	void						init			(void);
	void						initPrograms	(vk::SourceCollections& programCollection) const;
	TestInstance*				createInstance	(Context&				context) const;
	static MultisampleCaseBase*	createCase		(tcu::TestContext&		testCtx,
												 const std::string&		name,
												 const ImageMSParams&	imageMSParams);
};

template <typename CaseClassName>
MultisampleCaseBase* MSCase<CaseClassName>::createCase (tcu::TestContext& testCtx, const std::string& name, const ImageMSParams& imageMSParams)
{
	return new MSCase<CaseClassName>(testCtx, name, imageMSParams);
}

template <typename InstanceClassName>
class MSInstance : public MSInstanceBaseResolveAndPerSampleFetch
{
public:
					MSInstance				(Context&											context,
											 const ImageMSParams&								imageMSParams)
					: MSInstanceBaseResolveAndPerSampleFetch(context, imageMSParams) {}

	VertexDataDesc	getVertexDataDescripton	(void) const;
	void			uploadVertexData		(const Allocation&									vertexBufferAllocation,
											 const VertexDataDesc&								vertexDataDescripton) const;

	tcu::TestStatus	verifyImageData			(const vk::VkImageCreateInfo&						imageMSInfo,
											 const vk::VkImageCreateInfo&						imageRSInfo,
											 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
											 const tcu::ConstPixelBufferAccess&					dataRS) const;
};

class MSInstanceSampleID;

template<> MultisampleInstanceBase::VertexDataDesc MSInstance<MSInstanceSampleID>::getVertexDataDescripton (void) const
{
	return getVertexDataDescriptonNdc();
}

template<> void MSInstance<MSInstanceSampleID>::uploadVertexData (const Allocation& vertexBufferAllocation, const VertexDataDesc& vertexDataDescripton) const
{
	uploadVertexDataNdc(vertexBufferAllocation, vertexDataDescripton);
}

template<> tcu::TestStatus MSInstance<MSInstanceSampleID>::verifyImageData	(const vk::VkImageCreateInfo&						imageMSInfo,
																			 const vk::VkImageCreateInfo&						imageRSInfo,
																			 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																			 const tcu::ConstPixelBufferAccess&					dataRS) const
{
	DE_UNREF(imageRSInfo);
	DE_UNREF(dataRS);

	const deUint32 numSamples = static_cast<deUint32>(imageMSInfo.samples);

	for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
	{
		for (deUint32 z = 0u; z < imageMSInfo.extent.depth;  ++z)
		for (deUint32 y = 0u; y < imageMSInfo.extent.height; ++y)
		for (deUint32 x = 0u; x < imageMSInfo.extent.width;  ++x)
		{
			const deUint32 sampleID = dataPerSample[sampleNdx].getPixelUint(x, y, z).x();

			if (sampleID != sampleNdx)
				return tcu::TestStatus::fail("gl_SampleID does not have correct value");
		}
	}

	return tcu::TestStatus::pass("Passed");
}

class MSCaseSampleID;

template<> void MSCase<MSCaseSampleID>::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Writing gl_SampleID to the red channel of the texture and verifying texture values.\n"
		<< "Expecting value N at sample index N of a multisample texture.\n"
		<< tcu::TestLog::EndMessage;

	MultisampleCaseBase::init();
}

template<> void MSCase<MSCaseSampleID>::initPrograms (vk::SourceCollections& programCollection) const
{
	MSCaseBaseResolveAndPerSampleFetch::initPrograms(programCollection);

	// Create vertex shader
	std::ostringstream vs;

	vs << "#version 440\n"
		<< "layout(location = 0) in vec4 vs_in_position_ndc;\n"
		<< "\n"
		<< "out gl_PerVertex {\n"
		<< "	vec4  gl_Position;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position	= vs_in_position_ndc;\n"
		<< "}\n";

	programCollection.glslSources.add("vertex_shader") << glu::VertexSource(vs.str());

	// Create fragment shader
	std::ostringstream fs;

	fs << "#version 440\n"
		<< "\n"
		<< "layout(location = 0) out vec4 fs_out_color;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	fs_out_color = vec4(float(gl_SampleID) / float(255), 0.0, 0.0, 1.0);\n"
		<< "}\n";

	programCollection.glslSources.add("fragment_shader") << glu::FragmentSource(fs.str());
}

template<> TestInstance* MSCase<MSCaseSampleID>::createInstance (Context& context) const
{
	return new MSInstance<MSInstanceSampleID>(context, m_imageMSParams);
}

class MSInstanceSamplePosDistribution;

template<> MultisampleInstanceBase::VertexDataDesc MSInstance<MSInstanceSamplePosDistribution>::getVertexDataDescripton (void) const
{
	return getVertexDataDescriptonNdc();
}

template<> void MSInstance<MSInstanceSamplePosDistribution>::uploadVertexData (const Allocation& vertexBufferAllocation, const VertexDataDesc& vertexDataDescripton) const
{
	uploadVertexDataNdc(vertexBufferAllocation, vertexDataDescripton);
}

template<> tcu::TestStatus MSInstance<MSInstanceSamplePosDistribution>::verifyImageData	(const vk::VkImageCreateInfo&						imageMSInfo,
																						 const vk::VkImageCreateInfo&						imageRSInfo,
																						 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																						 const tcu::ConstPixelBufferAccess&					dataRS) const
{
	const deUint32 numSamples = static_cast<deUint32>(imageMSInfo.samples);

	// approximate Bates distribution as normal
	const float variance = (1.0f / (12.0f * (float)numSamples));
	const float standardDeviation = deFloatSqrt(variance);

	// 95% of means of sample positions are within 2 standard deviations if
	// they were randomly assigned. Sample patterns are expected to be more
	// uniform than a random pattern.
	const float distanceThreshold = 2.0f * standardDeviation;

	for (deUint32 z = 0u; z < imageRSInfo.extent.depth;  ++z)
	for (deUint32 y = 0u; y < imageRSInfo.extent.height; ++y)
	for (deUint32 x = 0u; x < imageRSInfo.extent.width;  ++x)
	{
		const deUint32 errorComponent = dataRS.getPixelUint(x, y, z).z();

		if (errorComponent > 0)
			return tcu::TestStatus::fail("gl_SamplePosition is not within interval [0,1]");

		if (numSamples >= VK_SAMPLE_COUNT_4_BIT)
		{
			const tcu::Vec2 averageSamplePos	= tcu::Vec2((float)dataRS.getPixelUint(x, y, z).x() / 255.0f, (float)dataRS.getPixelUint(x, y, z).y() / 255.0f);
			const tcu::Vec2	distanceFromCenter	= tcu::abs(averageSamplePos - tcu::Vec2(0.5f, 0.5f));

			if (distanceFromCenter.x() > distanceThreshold || distanceFromCenter.y() > distanceThreshold)
				return tcu::TestStatus::fail("Sample positions are not uniformly distributed within the pixel");
		}
	}

	for (deUint32 z = 0u; z < imageMSInfo.extent.depth;  ++z)
	for (deUint32 y = 0u; y < imageMSInfo.extent.height; ++y)
	for (deUint32 x = 0u; x < imageMSInfo.extent.width;  ++x)
	{
		std::vector<tcu::Vec2> samplePositions(numSamples);

		for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
		{
			const deUint32 errorComponent = dataPerSample[sampleNdx].getPixelUint(x, y, z).z();

			if (errorComponent > 0)
				return tcu::TestStatus::fail("gl_SamplePosition is not within interval [0,1]");

			samplePositions[sampleNdx] = tcu::Vec2( (float)dataPerSample[sampleNdx].getPixelUint(x, y, z).x() / 255.0f,
													(float)dataPerSample[sampleNdx].getPixelUint(x, y, z).y() / 255.0f);
		}

		for (deUint32 sampleNdxA = 0u;				sampleNdxA < numSamples; ++sampleNdxA)
		for (deUint32 sampleNdxB = sampleNdxA + 1u; sampleNdxB < numSamples; ++sampleNdxB)
		{
			if (samplePositions[sampleNdxA] == samplePositions[sampleNdxB])
				return tcu::TestStatus::fail("Two samples have the same position");
		}

		if (numSamples >= VK_SAMPLE_COUNT_4_BIT)
		{
			tcu::Vec2 averageSamplePos(0.0f, 0.0f);

			for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
			{
				averageSamplePos.x() += samplePositions[sampleNdx].x();
				averageSamplePos.y() += samplePositions[sampleNdx].y();
			}

			averageSamplePos.x() /= (float)numSamples;
			averageSamplePos.y() /= (float)numSamples;

			const tcu::Vec2	distanceFromCenter = tcu::abs(averageSamplePos - tcu::Vec2(0.5f, 0.5f));

			if (distanceFromCenter.x() > distanceThreshold || distanceFromCenter.y() > distanceThreshold)
				return tcu::TestStatus::fail("Sample positions are not uniformly distributed within the pixel");
		}
	}

	return tcu::TestStatus::pass("Passed");
}

class MSCaseSamplePosDistribution;

template<> void MSCase<MSCaseSamplePosDistribution>::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SamplePosition value with multisample targets:\n"
		<< "	a) Expect legal sample position.\n"
		<< "	b) Sample position is unique within the set of all sample positions of a pixel.\n"
		<< "	c) Sample position distribution is uniform or almost uniform.\n"
		<< tcu::TestLog::EndMessage;

	MultisampleCaseBase::init();
}

template<> void MSCase<MSCaseSamplePosDistribution>::initPrograms (vk::SourceCollections& programCollection) const
{
	MSCaseBaseResolveAndPerSampleFetch::initPrograms(programCollection);

	// Create vertex shader
	std::ostringstream vs;

	vs << "#version 440\n"
		<< "layout(location = 0) in vec4 vs_in_position_ndc;\n"
		<< "\n"
		<< "out gl_PerVertex {\n"
		<< "	vec4  gl_Position;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position	= vs_in_position_ndc;\n"
		<< "}\n";

	programCollection.glslSources.add("vertex_shader") << glu::VertexSource(vs.str());

	// Create fragment shader
	std::ostringstream fs;

	fs << "#version 440\n"
		<< "\n"
		<< "layout(location = 0) out vec4 fs_out_color;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	if (gl_SamplePosition.x < 0.0 || gl_SamplePosition.x > 1.0 || gl_SamplePosition.y < 0.0 || gl_SamplePosition.y > 1.0)\n"
		"		fs_out_color = vec4(0.0, 0.0, 1.0, 1.0);\n"
		"	else\n"
		"		fs_out_color = vec4(gl_SamplePosition.x, gl_SamplePosition.y, 0.0, 1.0);\n"
		"}\n";

	programCollection.glslSources.add("fragment_shader") << glu::FragmentSource(fs.str());
}

template<> TestInstance* MSCase<MSCaseSamplePosDistribution>::createInstance (Context& context) const
{
	return new MSInstance<MSInstanceSamplePosDistribution>(context, m_imageMSParams);
}

class MSInstanceSamplePosCorrectness;

template<> MultisampleInstanceBase::VertexDataDesc MSInstance<MSInstanceSamplePosCorrectness>::getVertexDataDescripton (void) const
{
	return getVertexDataDescriptonNdcScreen();
}

template<> void MSInstance<MSInstanceSamplePosCorrectness>::uploadVertexData (const Allocation& vertexBufferAllocation, const VertexDataDesc& vertexDataDescripton) const
{
	const tcu::UVec3 layerSize = getLayerSize(IMAGE_TYPE_2D, m_imageMSParams.imageSize);

	uploadVertexDataNdcScreen(vertexBufferAllocation, vertexDataDescripton, tcu::Vec2(static_cast<float>(layerSize.x()), static_cast<float>(layerSize.y())));
}

template<> tcu::TestStatus MSInstance<MSInstanceSamplePosCorrectness>::verifyImageData	(const vk::VkImageCreateInfo&						imageMSInfo,
																						 const vk::VkImageCreateInfo&						imageRSInfo,
																						 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																						 const tcu::ConstPixelBufferAccess&					dataRS) const
{
	if (checkForErrorMS(imageMSInfo, dataPerSample, 0))
		return tcu::TestStatus::fail("Varying values are not sampled at gl_SamplePosition");

	if (checkForErrorRS(imageRSInfo, dataRS, 0))
		return tcu::TestStatus::fail("Varying values are not sampled at gl_SamplePosition");

	return tcu::TestStatus::pass("Passed");
}

class MSCaseSamplePosCorrectness;

template<> void MSCase<MSCaseSamplePosCorrectness>::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SamplePosition correctness:\n"
		<< "	1) Varying values should be sampled at the sample position.\n"
		<< "		=> fract(position_screen) == gl_SamplePosition\n"
		<< tcu::TestLog::EndMessage;

	MultisampleCaseBase::init();
}

template<> void MSCase<MSCaseSamplePosCorrectness>::initPrograms (vk::SourceCollections& programCollection) const
{
	MSCaseBaseResolveAndPerSampleFetch::initPrograms(programCollection);

	// Create vertex shaders
	std::ostringstream vs;

	vs	<< "#version 440\n"
		<< "layout(location = 0) in vec4 vs_in_position_ndc;\n"
		<< "layout(location = 1) in vec2 vs_in_position_screen;\n"
		<< "\n"
		<< "layout(location = 0) sample out vec2 vs_out_position_screen;\n"
		<< "\n"
		<< "out gl_PerVertex {\n"
		<< "	vec4  gl_Position;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position				= vs_in_position_ndc;\n"
		<< "	vs_out_position_screen	= vs_in_position_screen;\n"
		<< "}\n";

	programCollection.glslSources.add("vertex_shader") << glu::VertexSource(vs.str());

	// Create fragment shader
	std::ostringstream fs;

	fs	<< "#version 440\n"
		<< "layout(location = 0) sample in vec2 fs_in_position_screen;\n"
		<< "\n"
		<< "layout(location = 0) out vec4 fs_out_color;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	const float threshold = 0.15625; // 4 subpixel bits. Assume 3 accurate bits + 0.03125 for other errors\n"
		<< "	const ivec2 nearby_pixel = ivec2(floor(fs_in_position_screen));\n"
		<< "	bool ok	= false;\n"
		<< "\n"
		<< "	// sample at edge + inaccuaries may cause us to round to any neighboring pixel\n"
		<< "	// check all neighbors for any match\n"
		<< "	for (int dy = -1; dy <= 1; ++dy)\n"
		<< "	for (int dx = -1; dx <= 1; ++dx)\n"
		<< "	{\n"
		<< "		ivec2 current_pixel			= nearby_pixel + ivec2(dx, dy);\n"
		<< "		vec2 position_inside_pixel	= vec2(current_pixel) + gl_SamplePosition;\n"
		<< "		vec2 position_diff			= abs(position_inside_pixel - fs_in_position_screen);\n"
		<< "\n"
		<< "		if (all(lessThan(position_diff, vec2(threshold))))\n"
		<< "			ok = true;\n"
		<< "	}\n"
		<< "\n"
		<< "	if (ok)\n"
		<< "		fs_out_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
		<< "	else\n"
		<< "		fs_out_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
		<< "}\n";

	programCollection.glslSources.add("fragment_shader") << glu::FragmentSource(fs.str());
}

template<> TestInstance* MSCase<MSCaseSamplePosCorrectness>::createInstance (Context& context) const
{
	return new MSInstance<MSInstanceSamplePosCorrectness>(context, m_imageMSParams);
}

class MSInstanceSampleMaskPattern : public MSInstanceBaseResolveAndPerSampleFetch
{
public:
											MSInstanceSampleMaskPattern	(Context&											context,
																		 const ImageMSParams&								imageMSParams);

	VkPipelineMultisampleStateCreateInfo	getMSStateCreateInfo		(const ImageMSParams&								imageMSParams) const;

	const VkDescriptorSetLayout*			createMSPassDescSetLayout	(const ImageMSParams&								imageMSParams);

	const VkDescriptorSet*					createMSPassDescSet			(const ImageMSParams&								imageMSParams,
																		 const VkDescriptorSetLayout*						descSetLayout);

	VertexDataDesc							getVertexDataDescripton		(void) const;

	void									uploadVertexData			(const Allocation&									vertexBufferAllocation,
																		 const VertexDataDesc&								vertexDataDescripton) const;

	tcu::TestStatus							verifyImageData				(const vk::VkImageCreateInfo&						imageMSInfo,
																		 const vk::VkImageCreateInfo&						imageRSInfo,
																		 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																		 const tcu::ConstPixelBufferAccess&					dataRS) const;
protected:

	VkSampleMask				m_sampleMask;
	Move<VkDescriptorSetLayout>	m_descriptorSetLayout;
	Move<VkDescriptorPool>		m_descriptorPool;
	Move<VkDescriptorSet>		m_descriptorSet;
	de::MovePtr<Buffer>			m_buffer;
};

MSInstanceSampleMaskPattern::MSInstanceSampleMaskPattern (Context& context, const ImageMSParams& imageMSParams) : MSInstanceBaseResolveAndPerSampleFetch(context, imageMSParams)
{
	m_sampleMask = 0xAAAAAAAAu & ((1u << imageMSParams.numSamples) - 1u);
}

VkPipelineMultisampleStateCreateInfo MSInstanceSampleMaskPattern::getMSStateCreateInfo (const ImageMSParams& imageMSParams) const
{
	const VkPipelineMultisampleStateCreateInfo multisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0u,						// VkPipelineMultisampleStateCreateFlags	flags;
		imageMSParams.numSamples,										// VkSampleCountFlagBits					rasterizationSamples;
		VK_TRUE,														// VkBool32									sampleShadingEnable;
		1.0f,															// float									minSampleShading;
		&m_sampleMask,													// const VkSampleMask*						pSampleMask;
		VK_FALSE,														// VkBool32									alphaToCoverageEnable;
		VK_FALSE,														// VkBool32									alphaToOneEnable;
	};

	return multisampleStateInfo;
}

const VkDescriptorSetLayout* MSInstanceSampleMaskPattern::createMSPassDescSetLayout (const ImageMSParams& imageMSParams)
{
	DE_UNREF(imageMSParams);

	const DeviceInterface&		deviceInterface = m_context.getDeviceInterface();
	const VkDevice				device			= m_context.getDevice();

	// Create descriptor set layout
	m_descriptorSetLayout = DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(deviceInterface, device);

	return &m_descriptorSetLayout.get();
}

const VkDescriptorSet* MSInstanceSampleMaskPattern::createMSPassDescSet (const ImageMSParams& imageMSParams, const VkDescriptorSetLayout* descSetLayout)
{
	DE_UNREF(imageMSParams);

	const DeviceInterface&		deviceInterface = m_context.getDeviceInterface();
	const VkDevice				device			= m_context.getDevice();
	Allocator&					allocator		= m_context.getDefaultAllocator();

	// Create descriptor pool
	m_descriptorPool = DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u)
		.build(deviceInterface, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	// Create descriptor set
	m_descriptorSet = makeDescriptorSet(deviceInterface, device, *m_descriptorPool, *descSetLayout);

	const VkBufferCreateInfo bufferSampleMaskInfo = makeBufferCreateInfo(sizeof(VkSampleMask), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	m_buffer = de::MovePtr<Buffer>(new Buffer(deviceInterface, device, allocator, bufferSampleMaskInfo, MemoryRequirement::HostVisible));

	deMemcpy(m_buffer->getAllocation().getHostPtr(), &m_sampleMask, sizeof(VkSampleMask));

	flushMappedMemoryRange(deviceInterface, device, m_buffer->getAllocation().getMemory(), m_buffer->getAllocation().getOffset(), VK_WHOLE_SIZE);

	const VkDescriptorBufferInfo descBufferInfo = makeDescriptorBufferInfo(**m_buffer, 0u, sizeof(VkSampleMask));

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descBufferInfo)
		.update(deviceInterface, device);

	return &m_descriptorSet.get();
}

MultisampleInstanceBase::VertexDataDesc MSInstanceSampleMaskPattern::getVertexDataDescripton (void) const
{
	return getVertexDataDescriptonNdc();
}

void MSInstanceSampleMaskPattern::uploadVertexData (const Allocation& vertexBufferAllocation, const VertexDataDesc& vertexDataDescripton) const
{
	uploadVertexDataNdc(vertexBufferAllocation, vertexDataDescripton);
}

tcu::TestStatus	MSInstanceSampleMaskPattern::verifyImageData	(const vk::VkImageCreateInfo&						imageMSInfo,
																 const vk::VkImageCreateInfo&						imageRSInfo,
																 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																 const tcu::ConstPixelBufferAccess&					dataRS) const
{
	DE_UNREF(imageRSInfo);
	DE_UNREF(dataRS);

	if (checkForErrorMS(imageMSInfo, dataPerSample, 0))
		return tcu::TestStatus::fail("gl_SampleMaskIn bits have not been killed by pSampleMask state");

	return tcu::TestStatus::pass("Passed");
}

class MSCaseSampleMaskPattern;

template<> void MSCase<MSCaseSampleMaskPattern>::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SampleMaskIn value with pSampleMask state. gl_SampleMaskIn does not contain any bits set that are have been killed by pSampleMask state. Expecting:\n"
		<< "Expected result: gl_SampleMaskIn AND ~(pSampleMask) should be zero.\n"
		<< tcu::TestLog::EndMessage;

	MultisampleCaseBase::init();
}

template<> void MSCase<MSCaseSampleMaskPattern>::initPrograms (vk::SourceCollections& programCollection) const
{
	MSCaseBaseResolveAndPerSampleFetch::initPrograms(programCollection);

	// Create vertex shader
	std::ostringstream vs;

	vs << "#version 440\n"
		<< "layout(location = 0) in vec4 vs_in_position_ndc;\n"
		<< "\n"
		<< "out gl_PerVertex {\n"
		<< "	vec4  gl_Position;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position	= vs_in_position_ndc;\n"
		<< "}\n";

	programCollection.glslSources.add("vertex_shader") << glu::VertexSource(vs.str());

	// Create fragment shader
	std::ostringstream fs;

	fs << "#version 440\n"
		<< "\n"
		<< "layout(location = 0) out vec4 fs_out_color;\n"
		<< "\n"
		<< "layout(set = 0, binding = 0, std140) uniform SampleMaskBlock\n"
		<< "{\n"
		<< "	int sampleMaskPattern;\n"
		<< "};"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	if ((gl_SampleMaskIn[0] & ~sampleMaskPattern) != 0)\n"
		<< "		fs_out_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
		<< "	else\n"
		<< "		fs_out_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
		<< "}\n";

	programCollection.glslSources.add("fragment_shader") << glu::FragmentSource(fs.str());
}

template<> TestInstance* MSCase<MSCaseSampleMaskPattern>::createInstance (Context& context) const
{
	return new MSInstanceSampleMaskPattern(context, m_imageMSParams);
}

class MSInstanceSampleMaskBitCount;

template<> MultisampleInstanceBase::VertexDataDesc MSInstance<MSInstanceSampleMaskBitCount>::getVertexDataDescripton (void) const
{
	return getVertexDataDescriptonNdc();
}

template<> void MSInstance<MSInstanceSampleMaskBitCount>::uploadVertexData (const Allocation& vertexBufferAllocation, const VertexDataDesc& vertexDataDescripton) const
{
	uploadVertexDataNdc(vertexBufferAllocation, vertexDataDescripton);
}

template<> tcu::TestStatus MSInstance<MSInstanceSampleMaskBitCount>::verifyImageData	(const vk::VkImageCreateInfo&						imageMSInfo,
																						 const vk::VkImageCreateInfo&						imageRSInfo,
																						 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																						 const tcu::ConstPixelBufferAccess&					dataRS) const
{
	DE_UNREF(imageRSInfo);
	DE_UNREF(dataRS);

	if (checkForErrorMS(imageMSInfo, dataPerSample, 0))
		return tcu::TestStatus::fail("gl_SampleMaskIn has more than one bit set for some shader invocations");

	return tcu::TestStatus::pass("Passed");
}

class MSCaseSampleMaskBitCount;

template<> void MSCase<MSCaseSampleMaskBitCount>::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SampleMaskIn.\n"
		<< "	Fragment shader will be invoked numSamples times.\n"
		<< "	=> gl_SampleMaskIn should have only one bit set for each shader invocation.\n"
		<< tcu::TestLog::EndMessage;

	MultisampleCaseBase::init();
}

template<> void MSCase<MSCaseSampleMaskBitCount>::initPrograms (vk::SourceCollections& programCollection) const
{
	MSCaseBaseResolveAndPerSampleFetch::initPrograms(programCollection);

	// Create vertex shader
	std::ostringstream vs;

	vs << "#version 440\n"
		<< "layout(location = 0) in vec4 vs_in_position_ndc;\n"
		<< "\n"
		<< "out gl_PerVertex {\n"
		<< "	vec4  gl_Position;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position	= vs_in_position_ndc;\n"
		<< "}\n";

	programCollection.glslSources.add("vertex_shader") << glu::VertexSource(vs.str());

	// Create fragment shader
	std::ostringstream fs;

	fs << "#version 440\n"
		<< "\n"
		<< "layout(location = 0) out vec4 fs_out_color;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uint maskBitCount = 0u;\n"
		<< "\n"
		<< "	for (int i = 0; i < 32; ++i)\n"
		<< "		if (((gl_SampleMaskIn[0] >> i) & 0x01) == 0x01)\n"
		<< "			++maskBitCount;\n"
		<< "\n"
		<< "	if (maskBitCount != 1u)\n"
		<< "		fs_out_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
		<< "	else\n"
		<< "		fs_out_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
		<< "}\n";

	programCollection.glslSources.add("fragment_shader") << glu::FragmentSource(fs.str());
}

template<> TestInstance* MSCase<MSCaseSampleMaskBitCount>::createInstance (Context& context) const
{
	return new MSInstance<MSInstanceSampleMaskBitCount>(context, m_imageMSParams);
}

class MSInstanceSampleMaskCorrectBit;

template<> MultisampleInstanceBase::VertexDataDesc MSInstance<MSInstanceSampleMaskCorrectBit>::getVertexDataDescripton (void) const
{
	return getVertexDataDescriptonNdc();
}

template<> void MSInstance<MSInstanceSampleMaskCorrectBit>::uploadVertexData (const Allocation& vertexBufferAllocation, const VertexDataDesc& vertexDataDescripton) const
{
	uploadVertexDataNdc(vertexBufferAllocation, vertexDataDescripton);
}

template<> tcu::TestStatus MSInstance<MSInstanceSampleMaskCorrectBit>::verifyImageData	(const vk::VkImageCreateInfo&						imageMSInfo,
																						 const vk::VkImageCreateInfo&						imageRSInfo,
																						 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																						 const tcu::ConstPixelBufferAccess&					dataRS) const
{
	DE_UNREF(imageRSInfo);
	DE_UNREF(dataRS);

	if (checkForErrorMS(imageMSInfo, dataPerSample, 0))
		return tcu::TestStatus::fail("The bit corresponsing to current gl_SampleID is not set in gl_SampleMaskIn");

	return tcu::TestStatus::pass("Passed");
}

class MSCaseSampleMaskCorrectBit;

template<> void MSCase<MSCaseSampleMaskCorrectBit>::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SampleMaskIn.\n"
		<< "	Fragment shader will be invoked numSamples times.\n"
		<< "	=> In each invocation gl_SampleMaskIn should have the bit set that corresponds to gl_SampleID.\n"
		<< tcu::TestLog::EndMessage;

	MultisampleCaseBase::init();
}

template<> void MSCase<MSCaseSampleMaskCorrectBit>::initPrograms (vk::SourceCollections& programCollection) const
{
	MSCaseBaseResolveAndPerSampleFetch::initPrograms(programCollection);

	// Create vertex shader
	std::ostringstream vs;

	vs << "#version 440\n"
		<< "layout(location = 0) in vec4 vs_in_position_ndc;\n"
		<< "\n"
		<< "out gl_PerVertex {\n"
		<< "	vec4  gl_Position;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position	= vs_in_position_ndc;\n"
		<< "}\n";

	programCollection.glslSources.add("vertex_shader") << glu::VertexSource(vs.str());

	// Create fragment shader
	std::ostringstream fs;

	fs << "#version 440\n"
		<< "\n"
		<< "layout(location = 0) out vec4 fs_out_color;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	if (((gl_SampleMaskIn[0] >> gl_SampleID) & 0x01) == 0x01)\n"
		<< "		fs_out_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
		<< "	else\n"
		<< "		fs_out_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
		<< "}\n";

	programCollection.glslSources.add("fragment_shader") << glu::FragmentSource(fs.str());
}

template<> TestInstance* MSCase<MSCaseSampleMaskCorrectBit>::createInstance (Context& context) const
{
	return new MSInstance<MSInstanceSampleMaskCorrectBit>(context, m_imageMSParams);
}

class MSInstanceSampleMaskWrite;

template<> MultisampleInstanceBase::VertexDataDesc MSInstance<MSInstanceSampleMaskWrite>::getVertexDataDescripton (void) const
{
	return getVertexDataDescriptonNdc();
}

template<> void MSInstance<MSInstanceSampleMaskWrite>::uploadVertexData (const Allocation& vertexBufferAllocation, const VertexDataDesc& vertexDataDescripton) const
{
	uploadVertexDataNdc(vertexBufferAllocation, vertexDataDescripton);
}

template<> tcu::TestStatus MSInstance<MSInstanceSampleMaskWrite>::verifyImageData	(const vk::VkImageCreateInfo&						imageMSInfo,
																					 const vk::VkImageCreateInfo&						imageRSInfo,
																					 const std::vector<tcu::ConstPixelBufferAccess>&	dataPerSample,
																					 const tcu::ConstPixelBufferAccess&					dataRS) const
{
	const deUint32 numSamples = static_cast<deUint32>(imageMSInfo.samples);

	for (deUint32 z = 0u; z < imageMSInfo.extent.depth;  ++z)
	for (deUint32 y = 0u; y < imageMSInfo.extent.height; ++y)
	for (deUint32 x = 0u; x < imageMSInfo.extent.width;  ++x)
	{
		for (deUint32 sampleNdx = 0u; sampleNdx < numSamples; ++sampleNdx)
		{
			const deUint32 firstComponent = dataPerSample[sampleNdx].getPixelUint(x, y, z)[0];

			if (firstComponent != 0u && firstComponent != 255u)
				return tcu::TestStatus::fail("Expected color to be zero or saturated on the first channel");
		}
	}

	for (deUint32 z = 0u; z < imageRSInfo.extent.depth;  ++z)
	for (deUint32 y = 0u; y < imageRSInfo.extent.height; ++y)
	for (deUint32 x = 0u; x < imageRSInfo.extent.width;  ++x)
	{
		const float firstComponent = dataRS.getPixel(x, y, z)[0];

		if (deFloatAbs(firstComponent - 0.5f) > 0.02f)
			return tcu::TestStatus::fail("Expected resolve color to be half intensity on the first channel");
	}

	return tcu::TestStatus::pass("Passed");
}

class MSCaseSampleMaskWrite;

template<> void MSCase<MSCaseSampleMaskWrite>::init (void)
{
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Discarding half of the samples using gl_SampleMask."
		<< "Expecting half intensity on multisample targets (numSamples > 1)\n"
		<< tcu::TestLog::EndMessage;

	MultisampleCaseBase::init();
}

template<> void MSCase<MSCaseSampleMaskWrite>::initPrograms (vk::SourceCollections& programCollection) const
{
	MSCaseBaseResolveAndPerSampleFetch::initPrograms(programCollection);

	// Create vertex shader
	std::ostringstream vs;

	vs << "#version 440\n"
		<< "layout(location = 0) in vec4 vs_in_position_ndc;\n"
		<< "\n"
		<< "out gl_PerVertex {\n"
		<< "	vec4  gl_Position;\n"
		<< "};\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position	= vs_in_position_ndc;\n"
		<< "}\n";

	programCollection.glslSources.add("vertex_shader") << glu::VertexSource(vs.str());

	// Create fragment shader
	std::ostringstream fs;

	fs << "#version 440\n"
		<< "\n"
		<< "layout(location = 0) out vec4 fs_out_color;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	gl_SampleMask[0] = 0xAAAAAAAA;\n"
		<< "\n"
		<< "	fs_out_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
		<< "}\n";

	programCollection.glslSources.add("fragment_shader") << glu::FragmentSource(fs.str());
}

template<> TestInstance* MSCase<MSCaseSampleMaskWrite>::createInstance (Context& context) const
{
	return new MSInstance<MSInstanceSampleMaskWrite>(context, m_imageMSParams);
}

} // multisample

tcu::TestCaseGroup* createMultisampleShaderBuiltInTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "multisample_shader_builtin", "Multisample Shader BuiltIn Tests"));

	const tcu::UVec3 imageSizes[] =
	{
		tcu::UVec3(128u, 128u, 1u),
		tcu::UVec3(137u, 191u, 1u),
	};

	const deUint32 sizesElemCount = static_cast<deUint32>(sizeof(imageSizes) / sizeof(tcu::UVec3));

	const vk::VkSampleCountFlagBits samplesSetFull[] =
	{
		vk::VK_SAMPLE_COUNT_2_BIT,
		vk::VK_SAMPLE_COUNT_4_BIT,
		vk::VK_SAMPLE_COUNT_8_BIT,
		vk::VK_SAMPLE_COUNT_16_BIT,
		vk::VK_SAMPLE_COUNT_32_BIT,
		vk::VK_SAMPLE_COUNT_64_BIT,
	};

	const deUint32 samplesSetFullCount = static_cast<deUint32>(sizeof(samplesSetFull) / sizeof(vk::VkSampleCountFlagBits));

	testGroup->addChild(makeMSGroup<multisample::MSCase<multisample::MSCaseSampleID> >(testCtx, "sample_id", imageSizes, sizesElemCount, samplesSetFull, samplesSetFullCount));

	de::MovePtr<tcu::TestCaseGroup> samplePositionGroup(new tcu::TestCaseGroup(testCtx, "sample_position", "Sample Position Tests"));

	samplePositionGroup->addChild(makeMSGroup<multisample::MSCase<multisample::MSCaseSamplePosDistribution> >(testCtx, "distribution", imageSizes, sizesElemCount, samplesSetFull, samplesSetFullCount));
	samplePositionGroup->addChild(makeMSGroup<multisample::MSCase<multisample::MSCaseSamplePosCorrectness> > (testCtx, "correctness",  imageSizes, sizesElemCount, samplesSetFull, samplesSetFullCount));

	testGroup->addChild(samplePositionGroup.release());

	const vk::VkSampleCountFlagBits samplesSetReduced[] =
	{
		vk::VK_SAMPLE_COUNT_2_BIT,
		vk::VK_SAMPLE_COUNT_4_BIT,
		vk::VK_SAMPLE_COUNT_8_BIT,
		vk::VK_SAMPLE_COUNT_16_BIT,
		vk::VK_SAMPLE_COUNT_32_BIT,
	};

	const deUint32 samplesSetReducedCount = static_cast<deUint32>(sizeof(samplesSetReduced) / sizeof(vk::VkSampleCountFlagBits));

	de::MovePtr<tcu::TestCaseGroup> sampleMaskGroup(new tcu::TestCaseGroup(testCtx, "sample_mask", "Sample Mask Tests"));

	sampleMaskGroup->addChild(makeMSGroup<multisample::MSCase<multisample::MSCaseSampleMaskPattern> >	(testCtx, "pattern",	imageSizes, sizesElemCount, samplesSetReduced, samplesSetReducedCount));
	sampleMaskGroup->addChild(makeMSGroup<multisample::MSCase<multisample::MSCaseSampleMaskBitCount> >	(testCtx, "bit_count",	imageSizes, sizesElemCount, samplesSetReduced, samplesSetReducedCount));
	sampleMaskGroup->addChild(makeMSGroup<multisample::MSCase<multisample::MSCaseSampleMaskCorrectBit> >(testCtx, "correct_bit",imageSizes, sizesElemCount, samplesSetReduced, samplesSetReducedCount));
	sampleMaskGroup->addChild(makeMSGroup<multisample::MSCase<multisample::MSCaseSampleMaskWrite> >		(testCtx, "write",		imageSizes, sizesElemCount, samplesSetReduced, samplesSetReducedCount));

	testGroup->addChild(sampleMaskGroup.release());

	return testGroup.release();
}

} // pipeline
} // vkt
