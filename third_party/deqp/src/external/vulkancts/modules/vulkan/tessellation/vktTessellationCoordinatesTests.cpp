/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
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
 * \file
 * \brief Tessellation Coordinates Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationCoordinatesTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuRGBA.hpp"
#include "tcuSurface.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deUniquePtr.hpp"

#include <string>
#include <vector>

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

template <typename T>
class SizeLessThan
{
public:
	bool operator() (const T& a, const T& b) const { return a.size() < b.size(); }
};

std::string getCaseName (const TessPrimitiveType primitiveType, const SpacingMode spacingMode)
{
	std::ostringstream str;
	str << getTessPrimitiveTypeShaderName(primitiveType) << "_" << getSpacingModeShaderName(spacingMode);
	return str.str();
}

std::vector<TessLevels> genTessLevelCases (const TessPrimitiveType	primitiveType,
										   const SpacingMode		spacingMode)
{
	static const TessLevels rawTessLevelCases[] =
	{
		{ { 1.0f,	1.0f	},	{ 1.0f,		1.0f,	1.0f,	1.0f	} },
		{ { 63.0f,	24.0f	},	{ 15.0f,	42.0f,	10.0f,	12.0f	} },
		{ { 3.0f,	2.0f	},	{ 6.0f,		8.0f,	7.0f,	9.0f	} },
		{ { 4.0f,	6.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 2.0f,	2.0f	},	{ 6.0f,		8.0f,	7.0f,	9.0f	} },
		{ { 5.0f,	6.0f	},	{ 1.0f,		1.0f,	1.0f,	1.0f	} },
		{ { 1.0f,	6.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 5.0f,	1.0f	},	{ 2.0f,		3.0f,	1.0f,	4.0f	} },
		{ { 5.2f,	1.6f	},	{ 2.9f,		3.4f,	1.5f,	4.1f	} }
	};

	if (spacingMode == SPACINGMODE_EQUAL)
		return std::vector<TessLevels>(DE_ARRAY_BEGIN(rawTessLevelCases), DE_ARRAY_END(rawTessLevelCases));
	else
	{
		std::vector<TessLevels> result;
		result.reserve(DE_LENGTH_OF_ARRAY(rawTessLevelCases));

		for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < DE_LENGTH_OF_ARRAY(rawTessLevelCases); ++tessLevelCaseNdx)
		{
			TessLevels curTessLevelCase = rawTessLevelCases[tessLevelCaseNdx];

			float* const inner = &curTessLevelCase.inner[0];
			float* const outer = &curTessLevelCase.outer[0];

			for (int j = 0; j < 2; ++j) inner[j] = static_cast<float>(getClampedRoundedTessLevel(spacingMode, inner[j]));
			for (int j = 0; j < 4; ++j) outer[j] = static_cast<float>(getClampedRoundedTessLevel(spacingMode, outer[j]));

			if (primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
			{
				if (outer[0] > 1.0f || outer[1] > 1.0f || outer[2] > 1.0f)
				{
					if (inner[0] == 1.0f)
						inner[0] = static_cast<float>(getClampedRoundedTessLevel(spacingMode, inner[0] + 0.1f));
				}
			}
			else if (primitiveType == TESSPRIMITIVETYPE_QUADS)
			{
				if (outer[0] > 1.0f || outer[1] > 1.0f || outer[2] > 1.0f || outer[3] > 1.0f)
				{
					if (inner[0] == 1.0f) inner[0] = static_cast<float>(getClampedRoundedTessLevel(spacingMode, inner[0] + 0.1f));
					if (inner[1] == 1.0f) inner[1] = static_cast<float>(getClampedRoundedTessLevel(spacingMode, inner[1] + 0.1f));
				}
			}

			result.push_back(curTessLevelCase);
		}

		DE_ASSERT(static_cast<int>(result.size()) == DE_LENGTH_OF_ARRAY(rawTessLevelCases));
		return result;
	}
}

std::vector<tcu::Vec3> generateReferenceTessCoords (const TessPrimitiveType	primitiveType,
													const SpacingMode		spacingMode,
													const float*			innerLevels,
													const float*			outerLevels)
{
	if (isPatchDiscarded(primitiveType, outerLevels))
		return std::vector<tcu::Vec3>();

	switch (primitiveType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:
		{
			int inner;
			int outer[3];
			getClampedRoundedTriangleTessLevels(spacingMode, innerLevels, outerLevels, &inner, &outer[0]);

			if (spacingMode != SPACINGMODE_EQUAL)
			{
				// \note For fractional spacing modes, exact results are implementation-defined except in special cases.
				DE_ASSERT(de::abs(innerLevels[0] - static_cast<float>(inner)) < 0.001f);
				for (int i = 0; i < 3; ++i)
					DE_ASSERT(de::abs(outerLevels[i] - static_cast<float>(outer[i])) < 0.001f);
				DE_ASSERT(inner > 1 || (outer[0] == 1 && outer[1] == 1 && outer[2] == 1));
			}

			return generateReferenceTriangleTessCoords(spacingMode, inner, outer[0], outer[1], outer[2]);
		}

		case TESSPRIMITIVETYPE_QUADS:
		{
			int inner[2];
			int outer[4];
			getClampedRoundedQuadTessLevels(spacingMode, innerLevels, outerLevels, &inner[0], &outer[0]);

			if (spacingMode != SPACINGMODE_EQUAL)
			{
				// \note For fractional spacing modes, exact results are implementation-defined except in special cases.
				for (int i = 0; i < 2; ++i)
					DE_ASSERT(de::abs(innerLevels[i] - static_cast<float>(inner[i])) < 0.001f);
				for (int i = 0; i < 4; ++i)
					DE_ASSERT(de::abs(outerLevels[i] - static_cast<float>(outer[i])) < 0.001f);

				DE_ASSERT((inner[0] > 1 && inner[1] > 1) || (inner[0] == 1 && inner[1] == 1 && outer[0] == 1 && outer[1] == 1 && outer[2] == 1 && outer[3] == 1));
			}

			return generateReferenceQuadTessCoords(spacingMode, inner[0], inner[1], outer[0], outer[1], outer[2], outer[3]);
		}

		case TESSPRIMITIVETYPE_ISOLINES:
		{
			int outer[2];
			getClampedRoundedIsolineTessLevels(spacingMode, &outerLevels[0], &outer[0]);

			if (spacingMode != SPACINGMODE_EQUAL)
			{
				// \note For fractional spacing modes, exact results are implementation-defined except in special cases.
				DE_ASSERT(de::abs(outerLevels[1] - static_cast<float>(outer[1])) < 0.001f);
			}

			return generateReferenceIsolineTessCoords(outer[0], outer[1]);
		}

		default:
			DE_ASSERT(false);
			return std::vector<tcu::Vec3>();
	}
}

void drawPoint (tcu::Surface& dst, const int centerX, const int centerY, const tcu::RGBA& color, const int size)
{
	const int width		= dst.getWidth();
	const int height	= dst.getHeight();
	DE_ASSERT(de::inBounds(centerX, 0, width) && de::inBounds(centerY, 0, height));
	DE_ASSERT(size > 0);

	for (int yOff = -((size-1)/2); yOff <= size/2; ++yOff)
	for (int xOff = -((size-1)/2); xOff <= size/2; ++xOff)
	{
		const int pixX = centerX + xOff;
		const int pixY = centerY + yOff;
		if (de::inBounds(pixX, 0, width) && de::inBounds(pixY, 0, height))
			dst.setPixel(pixX, pixY, color);
	}
}

void drawTessCoordPoint (tcu::Surface& dst, const TessPrimitiveType primitiveType, const tcu::Vec3& pt, const tcu::RGBA& color, const int size)
{
	// \note These coordinates should match the description in the log message in TessCoordTestInstance::iterate.

	static const tcu::Vec2 triangleCorners[3] =
	{
		tcu::Vec2(0.95f, 0.95f),
		tcu::Vec2(0.5f,  0.95f - 0.9f*deFloatSqrt(3.0f/4.0f)),
		tcu::Vec2(0.05f, 0.95f)
	};

	static const float quadIsolineLDRU[4] =
	{
		0.1f, 0.9f, 0.9f, 0.1f
	};

	const tcu::Vec2 dstPos = primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? pt.x()*triangleCorners[0]
																		  + pt.y()*triangleCorners[1]
																		  + pt.z()*triangleCorners[2]

					  : primitiveType == TESSPRIMITIVETYPE_QUADS ||
						primitiveType == TESSPRIMITIVETYPE_ISOLINES ? tcu::Vec2((1.0f - pt.x())*quadIsolineLDRU[0] + pt.x()*quadIsolineLDRU[2],
																			    (1.0f - pt.y())*quadIsolineLDRU[1] + pt.y()*quadIsolineLDRU[3])

					  : tcu::Vec2(-1.0f);

	drawPoint(dst,
			  static_cast<int>(dstPos.x() * (float)dst.getWidth()),
			  static_cast<int>(dstPos.y() * (float)dst.getHeight()),
			  color,
			  size);
}

void drawTessCoordVisualization (tcu::Surface& dst, const TessPrimitiveType primitiveType, const std::vector<tcu::Vec3>& coords)
{
	const int imageWidth  = 256;
	const int imageHeight = 256;
	dst.setSize(imageWidth, imageHeight);

	tcu::clear(dst.getAccess(), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	for (int i = 0; i < static_cast<int>(coords.size()); ++i)
		drawTessCoordPoint(dst, primitiveType, coords[i], tcu::RGBA::white(), 2);
}

inline bool vec3XLessThan (const tcu::Vec3& a, const tcu::Vec3& b)
{
	return a.x() < b.x();
}

int binarySearchFirstVec3WithXAtLeast (const std::vector<tcu::Vec3>& sorted, float x)
{
	const tcu::Vec3 ref(x, 0.0f, 0.0f);
	const std::vector<tcu::Vec3>::const_iterator first = std::lower_bound(sorted.begin(), sorted.end(), ref, vec3XLessThan);
	if (first == sorted.end())
		return -1;
	return static_cast<int>(std::distance(sorted.begin(), first));
}

// Check that all points in subset are (approximately) present also in superset.
bool oneWayComparePointSets (tcu::TestLog&					log,
							 tcu::Surface&					errorDst,
							 const TessPrimitiveType		primitiveType,
							 const std::vector<tcu::Vec3>&	subset,
							 const std::vector<tcu::Vec3>&	superset,
							 const char*					subsetName,
							 const char*					supersetName,
							 const tcu::RGBA&				errorColor)
{
	const std::vector<tcu::Vec3> supersetSorted		 = sorted(superset, vec3XLessThan);
	const float					 epsilon			 = 0.01f;
	const int					 maxNumFailurePrints = 5;
	int							 numFailuresDetected = 0;

	for (int subNdx = 0; subNdx < static_cast<int>(subset.size()); ++subNdx)
	{
		const tcu::Vec3& subPt = subset[subNdx];

		bool matchFound = false;

		{
			// Binary search the index of the first point in supersetSorted with x in the [subPt.x() - epsilon, subPt.x() + epsilon] range.
			const tcu::Vec3	matchMin			= subPt - epsilon;
			const tcu::Vec3	matchMax			= subPt + epsilon;
			const int		firstCandidateNdx	= binarySearchFirstVec3WithXAtLeast(supersetSorted, matchMin.x());

			if (firstCandidateNdx >= 0)
			{
				// Compare subPt to all points in supersetSorted with x in the [subPt.x() - epsilon, subPt.x() + epsilon] range.
				for (int superNdx = firstCandidateNdx; superNdx < static_cast<int>(supersetSorted.size()) && supersetSorted[superNdx].x() <= matchMax.x(); ++superNdx)
				{
					const tcu::Vec3& superPt = supersetSorted[superNdx];

					if (tcu::boolAll(tcu::greaterThanEqual	(superPt, matchMin)) &&
						tcu::boolAll(tcu::lessThanEqual		(superPt, matchMax)))
					{
						matchFound = true;
						break;
					}
				}
			}
		}

		if (!matchFound)
		{
			++numFailuresDetected;
			if (numFailuresDetected < maxNumFailurePrints)
				log << tcu::TestLog::Message << "Failure: no matching " << supersetName << " point found for " << subsetName << " point " << subPt << tcu::TestLog::EndMessage;
			else if (numFailuresDetected == maxNumFailurePrints)
				log << tcu::TestLog::Message << "Note: More errors follow" << tcu::TestLog::EndMessage;

			drawTessCoordPoint(errorDst, primitiveType, subPt, errorColor, 4);
		}
	}

	return numFailuresDetected == 0;
}

//! Returns true on matching coordinate sets.
bool compareTessCoords (tcu::TestLog&					log,
						TessPrimitiveType				primitiveType,
						const std::vector<tcu::Vec3>&	refCoords,
						const std::vector<tcu::Vec3>&	resCoords)
{
	tcu::Surface	refVisual;
	tcu::Surface	resVisual;
	bool			success = true;

	drawTessCoordVisualization(refVisual, primitiveType, refCoords);
	drawTessCoordVisualization(resVisual, primitiveType, resCoords);

	// Check that all points in reference also exist in result.
	success = oneWayComparePointSets(log, refVisual, primitiveType, refCoords, resCoords, "reference", "result", tcu::RGBA::blue()) && success;
	// Check that all points in result also exist in reference.
	success = oneWayComparePointSets(log, resVisual, primitiveType, resCoords, refCoords, "result", "reference", tcu::RGBA::red()) && success;

	if (!success)
	{
		log << tcu::TestLog::Message << "Note: in the following reference visualization, points that are missing in result point set are blue (if any)" << tcu::TestLog::EndMessage
			<< tcu::TestLog::Image("RefTessCoordVisualization", "Reference tessCoord visualization", refVisual)
			<< tcu::TestLog::Message << "Note: in the following result visualization, points that are missing in reference point set are red (if any)" << tcu::TestLog::EndMessage;
	}

	log << tcu::TestLog::Image("ResTessCoordVisualization", "Result tessCoord visualization", resVisual);

	return success;
}

class TessCoordTest : public TestCase
{
public:
								TessCoordTest	(tcu::TestContext&			testCtx,
												 const TessPrimitiveType	primitiveType,
												 const SpacingMode			spacingMode);

	void						initPrograms	(SourceCollections&			programCollection) const;
	TestInstance*				createInstance	(Context&					context) const;

private:
	const TessPrimitiveType		m_primitiveType;
	const SpacingMode			m_spacingMode;
};

TessCoordTest::TessCoordTest (tcu::TestContext&			testCtx,
							  const TessPrimitiveType	primitiveType,
							  const SpacingMode			spacingMode)
	: TestCase			(testCtx, getCaseName(primitiveType, spacingMode), "")
	, m_primitiveType	(primitiveType)
	, m_spacingMode		(spacingMode)
{
}

void TessCoordTest::initPrograms (SourceCollections& programCollection) const
{
	// Vertex shader - no inputs
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Tessellation control shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(vertices = 1) out;\n"
			<< "\n"
			<< "layout(set = 0, binding = 0, std430) readonly restrict buffer TessLevels {\n"
			<< "    float inner0;\n"
			<< "    float inner1;\n"
			<< "    float outer0;\n"
			<< "    float outer1;\n"
			<< "    float outer2;\n"
			<< "    float outer3;\n"
			<< "} sb_levels;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_TessLevelInner[0] = sb_levels.inner0;\n"
			<< "    gl_TessLevelInner[1] = sb_levels.inner1;\n"
			<< "\n"
			<< "    gl_TessLevelOuter[0] = sb_levels.outer0;\n"
			<< "    gl_TessLevelOuter[1] = sb_levels.outer1;\n"
			<< "    gl_TessLevelOuter[2] = sb_levels.outer2;\n"
			<< "    gl_TessLevelOuter[3] = sb_levels.outer3;\n"
			<< "}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Tessellation evaluation shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(" << getTessPrimitiveTypeShaderName(m_primitiveType) << ", "
						 << getSpacingModeShaderName(m_spacingMode) << ", point_mode) in;\n"
			<< "\n"
			<< "layout(set = 0, binding = 1, std430) coherent restrict buffer Output {\n"
			<< "    int  numInvocations;\n"
			<< "    vec3 tessCoord[];\n"		// alignment is 16 bytes, same as vec4
			<< "} sb_out;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    int index = atomicAdd(sb_out.numInvocations, 1);\n"
			<< "    sb_out.tessCoord[index] = gl_TessCoord;\n"
			<< "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}
}

class TessCoordTestInstance : public TestInstance
{
public:
								TessCoordTestInstance (Context&					context,
													   const TessPrimitiveType	primitiveType,
													   const SpacingMode		spacingMode);

	tcu::TestStatus				iterate				  (void);

private:
	const TessPrimitiveType		m_primitiveType;
	const SpacingMode			m_spacingMode;
};

TessCoordTestInstance::TessCoordTestInstance (Context&					context,
											  const TessPrimitiveType	primitiveType,
											  const SpacingMode			spacingMode)
	: TestInstance		(context)
	, m_primitiveType	(primitiveType)
	, m_spacingMode		(spacingMode)
{
}

tcu::TestStatus TessCoordTestInstance::iterate (void)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Test data

	const std::vector<TessLevels>		 tessLevelCases			= genTessLevelCases(m_primitiveType, m_spacingMode);
	std::vector<std::vector<tcu::Vec3> > allReferenceTessCoords	(tessLevelCases.size());

	for (deUint32 i = 0; i < tessLevelCases.size(); ++i)
		allReferenceTessCoords[i] = generateReferenceTessCoords(m_primitiveType, m_spacingMode, &tessLevelCases[i].inner[0], &tessLevelCases[i].outer[0]);

	const size_t maxNumVertices = static_cast<int>(std::max_element(allReferenceTessCoords.begin(), allReferenceTessCoords.end(), SizeLessThan<std::vector<tcu::Vec3> >())->size());

	// Input buffer: tessellation levels. Data is filled in later.

	const Buffer tessLevelsBuffer(vk, device, allocator,
		makeBufferCreateInfo(sizeof(TessLevels), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Output buffer: number of invocations + padding + tessellation coordinates. Initialized later.

	const int          resultBufferTessCoordsOffset	 = 4 * (int)sizeof(deInt32);
	const int          extraneousVertices			 = 16;	// allow some room for extraneous vertices from duplicate shader invocations (number is arbitrary)
	const VkDeviceSize resultBufferSizeBytes		 = resultBufferTessCoordsOffset + (maxNumVertices + extraneousVertices)*sizeof(tcu::Vec4);
	const Buffer       resultBuffer					 (vk, device, allocator, makeBufferCreateInfo(resultBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Descriptors

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo tessLevelsBufferInfo = makeDescriptorBufferInfo(tessLevelsBuffer.get(), 0ull, sizeof(TessLevels));
	const VkDescriptorBufferInfo resultBufferInfo     = makeDescriptorBufferInfo(resultBuffer.get(), 0ull, resultBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &tessLevelsBufferInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultBufferInfo)
		.update(vk, device);

	// Pipeline: set up vertex processing without rasterization

	const Unique<VkRenderPass>		renderPass		(makeRenderPassWithoutAttachments (vk, device));
	const Unique<VkFramebuffer>		framebuffer		(makeFramebufferWithoutAttachments(vk, device, *renderPass));
	const Unique<VkPipelineLayout>	pipelineLayout	(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>		cmdPool			(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer		(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
		.setShader(vk, device, VK_SHADER_STAGE_VERTEX_BIT,					m_context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader(vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	m_context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader(vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_context.getBinaryCollection().get("tese"), DE_NULL)
		.build    (vk, device, *pipelineLayout, *renderPass));

	deUint32 numPassedCases = 0;

	// Repeat the test for all tessellation coords cases
	for (deUint32 tessLevelCaseNdx = 0; tessLevelCaseNdx < tessLevelCases.size(); ++tessLevelCaseNdx)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Tessellation levels: " << getTessellationLevelsString(tessLevelCases[tessLevelCaseNdx], m_primitiveType)
			<< tcu::TestLog::EndMessage;

		// Upload tessellation levels data to the input buffer
		{
			const Allocation& alloc = tessLevelsBuffer.getAllocation();
			TessLevels* const bufferTessLevels = static_cast<TessLevels*>(alloc.getHostPtr());
			*bufferTessLevels = tessLevelCases[tessLevelCaseNdx];
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), sizeof(TessLevels));
		}

		// Clear the results buffer
		{
			const Allocation& alloc = resultBuffer.getAllocation();
			deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(resultBufferSizeBytes));
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), resultBufferSizeBytes);
		}

		// Reset the command buffer and begin recording.
		beginCommandBuffer(vk, *cmdBuffer);
		beginRenderPassWithRasterizationDisabled(vk, *cmdBuffer, *renderPass, *framebuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		// Process a single abstract vertex.
		vk.cmdDraw(*cmdBuffer, 1u, 1u, 0u, 0u);
		endRenderPass(vk, *cmdBuffer);

		{
			const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *resultBuffer, 0ull, resultBufferSizeBytes);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, 1u, &shaderWriteBarrier, 0u, DE_NULL);
		}

		endCommandBuffer(vk, *cmdBuffer);
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);

		// Verify results
		{
			const Allocation& resultAlloc = resultBuffer.getAllocation();
			invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), resultBufferSizeBytes);

			const deInt32					numResults			= *static_cast<deInt32*>(resultAlloc.getHostPtr());
			const std::vector<tcu::Vec3>	resultTessCoords    = readInterleavedData<tcu::Vec3>(numResults, resultAlloc.getHostPtr(), resultBufferTessCoordsOffset, sizeof(tcu::Vec4));
			const std::vector<tcu::Vec3>&	referenceTessCoords = allReferenceTessCoords[tessLevelCaseNdx];
			const int						numExpectedResults  = static_cast<int>(referenceTessCoords.size());
			tcu::TestLog&					log					= m_context.getTestContext().getLog();

			if (numResults < numExpectedResults)
			{
				log << tcu::TestLog::Message
					<< "Failure: generated " << numResults << " coordinates, but the expected reference value is " << numExpectedResults
					<< tcu::TestLog::EndMessage;
			}
			else if (numResults == numExpectedResults)
				log << tcu::TestLog::Message << "Note: generated " << numResults << " tessellation coordinates" << tcu::TestLog::EndMessage;
			else
			{
				log << tcu::TestLog::Message
					<< "Note: generated " << numResults << " coordinates (out of which " << numExpectedResults << " must be unique)"
					<< tcu::TestLog::EndMessage;
			}

			if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
				log << tcu::TestLog::Message << "Note: in the following visualization(s), the u=1, v=1, w=1 corners are at the right, top, and left corners, respectively" << tcu::TestLog::EndMessage;
			else if (m_primitiveType == TESSPRIMITIVETYPE_QUADS || m_primitiveType == TESSPRIMITIVETYPE_ISOLINES)
				log << tcu::TestLog::Message << "Note: in the following visualization(s), u and v coordinate go left-to-right and bottom-to-top, respectively" << tcu::TestLog::EndMessage;
			else
				DE_ASSERT(false);

			if (compareTessCoords(log, m_primitiveType, referenceTessCoords, resultTessCoords) && (numResults >= numExpectedResults))
				++numPassedCases;
		}
	}  // for tessLevelCaseNdx

	return (numPassedCases == tessLevelCases.size() ? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Some cases have failed"));
}

TestInstance* TessCoordTest::createInstance (Context& context) const
{
	requireFeatures(context.getInstanceInterface(), context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER | FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS);

	return new TessCoordTestInstance(context, m_primitiveType, m_spacingMode);
}

} // anonymous

//! Based on dEQP-GLES31.functional.tessellation.tesscoord.*
//! \note Transform feedback is replaced with SSBO. Because of that, this version allows duplicate coordinates from shader invocations.
//! The test still fails if not enough coordinates are generated, or if coordinates don't match the reference data.
tcu::TestCaseGroup* createCoordinatesTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "tesscoord", "Tessellation coordinates tests"));

	for (int primitiveTypeNdx = 0; primitiveTypeNdx < TESSPRIMITIVETYPE_LAST; ++primitiveTypeNdx)
	for (int spacingModeNdx = 0; spacingModeNdx < SPACINGMODE_LAST; ++spacingModeNdx)
		group->addChild(new TessCoordTest(testCtx, (TessPrimitiveType)primitiveTypeNdx, (SpacingMode)spacingModeNdx));

	return group.release();
}

} // tessellation
} // vkt
