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
 * \brief Tessellation Fractional Spacing Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationFractionalSpacingTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"

#include <string>
#include <vector>

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

template <typename T, typename MembT>
std::vector<MembT> members (const std::vector<T>& objs, MembT T::* membP)
{
	std::vector<MembT> result(objs.size());
	for (int i = 0; i < static_cast<int>(objs.size()); ++i)
		result[i] = objs[i].*membP;
	return result;
}

//! Predicate functor for comparing structs by their members.
template <typename Pred, typename T, typename MembT>
class MemberPred
{
public:
				MemberPred	(MembT T::* membP) : m_membP(membP), m_pred(Pred()) {}
	bool		operator()	(const T& a, const T& b) const { return m_pred(a.*m_membP, b.*m_membP); }

private:
	MembT T::*	m_membP;
	Pred		m_pred;
};

//! Convenience wrapper for MemberPred, because class template arguments aren't deduced based on constructor arguments.
template <template <typename> class Pred, typename T, typename MembT>
inline MemberPred<Pred<MembT>, T, MembT> memberPred (MembT T::* membP) { return MemberPred<Pred<MembT>, T, MembT>(membP); }

struct Segment
{
	int		index; //!< Index of left coordinate in sortedXCoords.
	float	length;

			Segment (void)						: index(-1),		length(-1.0f)	{}
			Segment (int index_, float length_)	: index(index_),	length(length_)	{}
};

inline std::vector<float> lengths (const std::vector<Segment>& segments) { return members(segments, &Segment::length); }

struct LineData
{
	float	tessLevel;
	float	additionalSegmentLength;
	int		additionalSegmentLocation;

			LineData (float lev, float len, int loc) : tessLevel(lev), additionalSegmentLength(len), additionalSegmentLocation(loc) {}
};

struct TestParams
{
	ShaderLanguage	shaderLanguage;
	SpacingMode		spacingMode;

					TestParams(ShaderLanguage sl, SpacingMode sm) : shaderLanguage(sl), spacingMode(sm) {}
};

/*--------------------------------------------------------------------*//*!
 * \brief Verify fractional spacing conditions for a single line
 *
 * Verify that the splitting of an edge (resulting from e.g. an isoline
 * with outer levels { 1.0, tessLevel }) with a given fractional spacing
 * mode fulfills certain conditions given in the spec.
 *
 * Note that some conditions can't be checked from just one line
 * (specifically, that the additional segment decreases monotonically
 * length and the requirement that the additional segments be placed
 * identically for identical values of clamped level).
 *
 * Therefore, the function stores some values to additionalSegmentLengthDst
 * and additionalSegmentLocationDst that can later be given to
 * verifyFractionalSpacingMultiple(). A negative value in length means that
 * no additional segments are present, i.e. there's just one segment.
 * A negative value in location means that the value wasn't determinable,
 * i.e. all segments had same length.
 * The values are not stored if false is returned.
 *//*--------------------------------------------------------------------*/
bool verifyFractionalSpacingSingle (tcu::TestLog&				log,
									const SpacingMode			spacingMode,
									const float					tessLevel,
									const std::vector<float>&	coords,
									float* const				pOutAdditionalSegmentLength,
									int* const					pOutAdditionalSegmentLocation)
{
	DE_ASSERT(spacingMode == SPACINGMODE_FRACTIONAL_ODD || spacingMode == SPACINGMODE_FRACTIONAL_EVEN);

	const float					clampedLevel	= getClampedTessLevel(spacingMode, tessLevel);
	const int					finalLevel		= getRoundedTessLevel(spacingMode, clampedLevel);
	const std::vector<float>	sortedCoords	= sorted(coords);
	std::string					failNote		= "Note: tessellation level is " + de::toString(tessLevel) + "\nNote: sorted coordinates are:\n    " + containerStr(sortedCoords);

	if (static_cast<int>(coords.size()) != finalLevel + 1)
	{
		log << tcu::TestLog::Message << "Failure: number of vertices is " << coords.size() << "; expected " << finalLevel + 1
			<< " (clamped tessellation level is " << clampedLevel << ")"
			<< "; final level (clamped level rounded up to " << (spacingMode == SPACINGMODE_FRACTIONAL_EVEN ? "even" : "odd") << ") is " << finalLevel
			<< " and should equal the number of segments, i.e. number of vertices minus 1" << tcu::TestLog::EndMessage
			<< tcu::TestLog::Message << failNote << tcu::TestLog::EndMessage;
		return false;
	}

	if (sortedCoords[0] != 0.0f || sortedCoords.back() != 1.0f)
	{
		log << tcu::TestLog::Message << "Failure: smallest coordinate should be 0.0 and biggest should be 1.0" << tcu::TestLog::EndMessage
			<< tcu::TestLog::Message << failNote << tcu::TestLog::EndMessage;
		return false;
	}

	{
		std::vector<Segment> segments(finalLevel);
		for (int i = 0; i < finalLevel; ++i)
			segments[i] = Segment(i, sortedCoords[i+1] - sortedCoords[i]);

		failNote += "\nNote: segment lengths are, from left to right:\n    " + containerStr(lengths(segments));

		{
			// Divide segments to two different groups based on length.

			std::vector<Segment> segmentsA;
			std::vector<Segment> segmentsB;
			segmentsA.push_back(segments[0]);

			for (int segNdx = 1; segNdx < static_cast<int>(segments.size()); ++segNdx)
			{
				const float		epsilon		= 0.001f;
				const Segment&	seg			= segments[segNdx];

				if (de::abs(seg.length - segmentsA[0].length) < epsilon)
					segmentsA.push_back(seg);
				else if (segmentsB.empty() || de::abs(seg.length - segmentsB[0].length) < epsilon)
					segmentsB.push_back(seg);
				else
				{
					log << tcu::TestLog::Message << "Failure: couldn't divide segments to 2 groups by length; "
												 << "e.g. segment of length " << seg.length << " isn't approximately equal to either "
												 << segmentsA[0].length << " or " << segmentsB[0].length << tcu::TestLog::EndMessage
												 << tcu::TestLog::Message << failNote << tcu::TestLog::EndMessage;
					return false;
				}
			}

			if (clampedLevel == static_cast<float>(finalLevel))
			{
				// All segments should be of equal length.
				if (!segmentsA.empty() && !segmentsB.empty())
				{
					log << tcu::TestLog::Message << "Failure: clamped and final tessellation level are equal, but not all segments are of equal length." << tcu::TestLog::EndMessage
						<< tcu::TestLog::Message << failNote << tcu::TestLog::EndMessage;
					return false;
				}
			}

			if (segmentsA.empty() || segmentsB.empty()) // All segments have same length. This is ok.
			{
				*pOutAdditionalSegmentLength   = (segments.size() == 1 ? -1.0f : segments[0].length);
				*pOutAdditionalSegmentLocation = -1;
				return true;
			}

			if (segmentsA.size() != 2 && segmentsB.size() != 2)
			{
				log << tcu::TestLog::Message << "Failure: when dividing the segments to 2 groups by length, neither of the two groups has exactly 2 or 0 segments in it" << tcu::TestLog::EndMessage
					<< tcu::TestLog::Message << failNote << tcu::TestLog::EndMessage;
				return false;
			}

			// For convenience, arrange so that the 2-segment group is segmentsB.
			if (segmentsB.size() != 2)
				std::swap(segmentsA, segmentsB);

			// \note For 4-segment lines both segmentsA and segmentsB have 2 segments each.
			//		 Thus, we can't be sure which ones were meant as the additional segments.
			//		 We give the benefit of the doubt by assuming that they're the shorter
			//		 ones (as they should).

			if (segmentsA.size() != 2)
			{
				if (segmentsB[0].length > segmentsA[0].length + 0.001f)
				{
					log << tcu::TestLog::Message << "Failure: the two additional segments are longer than the other segments" << tcu::TestLog::EndMessage
						<< tcu::TestLog::Message << failNote << tcu::TestLog::EndMessage;
					return false;
				}
			}
			else
			{
				// We have 2 segmentsA and 2 segmentsB, ensure segmentsB has the shorter lengths
				if (segmentsB[0].length > segmentsA[0].length)
					std::swap(segmentsA, segmentsB);
			}

			// Check that the additional segments are placed symmetrically.
			if (segmentsB[0].index + segmentsB[1].index + 1 != static_cast<int>(segments.size()))
			{
				log << tcu::TestLog::Message << "Failure: the two additional segments aren't placed symmetrically; "
										<< "one is at index " << segmentsB[0].index << " and other is at index " << segmentsB[1].index
										<< " (note: the two indexes should sum to " << static_cast<int>(segments.size())-1 << ", i.e. numberOfSegments-1)" << tcu::TestLog::EndMessage
					<< tcu::TestLog::Message << failNote << tcu::TestLog::EndMessage;
				return false;
			}

			*pOutAdditionalSegmentLength = segmentsB[0].length;
			if (segmentsA.size() != 2)
				*pOutAdditionalSegmentLocation = de::min(segmentsB[0].index, segmentsB[1].index);
			else
				*pOutAdditionalSegmentLocation = segmentsB[0].length < segmentsA[0].length - 0.001f ? de::min(segmentsB[0].index, segmentsB[1].index)
												 : -1; // \note -1 when can't reliably decide which ones are the additional segments, a or b.

			return true;
		}
	}
}

/*--------------------------------------------------------------------*//*!
 * \brief Verify fractional spacing conditions between multiple lines
 *
 * Verify the fractional spacing conditions that are not checked in
 * verifyFractionalSpacingSingle(). Uses values given by said function
 * as parameters, in addition to the spacing mode and tessellation level.
 *//*--------------------------------------------------------------------*/
static bool verifyFractionalSpacingMultiple (tcu::TestLog&				log,
											 const SpacingMode			spacingMode,
											 const std::vector<float>&	tessLevels,
											 const std::vector<float>&	additionalSegmentLengths,
											 const std::vector<int>&	additionalSegmentLocations)
{
	DE_ASSERT(spacingMode == SPACINGMODE_FRACTIONAL_ODD || spacingMode == SPACINGMODE_FRACTIONAL_EVEN);
	DE_ASSERT(tessLevels.size() == additionalSegmentLengths.size() && tessLevels.size() == additionalSegmentLocations.size());

	std::vector<LineData> lineDatas;

	for (int i = 0; i < static_cast<int>(tessLevels.size()); ++i)
		lineDatas.push_back(LineData(tessLevels[i], additionalSegmentLengths[i], additionalSegmentLocations[i]));

	{
		const std::vector<LineData> lineDatasSortedByLevel = sorted(lineDatas, memberPred<std::less>(&LineData::tessLevel));

		// Check that lines with identical clamped tessellation levels have identical additionalSegmentLocation.

		for (int lineNdx = 1; lineNdx < static_cast<int>(lineDatasSortedByLevel.size()); ++lineNdx)
		{
			const LineData& curData		= lineDatasSortedByLevel[lineNdx];
			const LineData& prevData	= lineDatasSortedByLevel[lineNdx-1];

			if (curData.additionalSegmentLocation < 0 || prevData.additionalSegmentLocation < 0)
				continue; // Unknown locations, skip.

			if (getClampedTessLevel(spacingMode, curData.tessLevel) == getClampedTessLevel(spacingMode, prevData.tessLevel) &&
				curData.additionalSegmentLocation != prevData.additionalSegmentLocation)
			{
				log << tcu::TestLog::Message << "Failure: additional segments not located identically for two edges with identical clamped tessellation levels" << tcu::TestLog::EndMessage
					<< tcu::TestLog::Message << "Note: tessellation levels are " << curData.tessLevel << " and " << prevData.tessLevel
											 << " (clamped level " << getClampedTessLevel(spacingMode, curData.tessLevel) << ")"
											 << "; but first additional segments located at indices "
											 << curData.additionalSegmentLocation << " and " << prevData.additionalSegmentLocation << ", respectively" << tcu::TestLog::EndMessage;
				return false;
			}
		}

		// Check that, among lines with same clamped rounded tessellation level, additionalSegmentLength is monotonically decreasing with "clampedRoundedTessLevel - clampedTessLevel" (the "fraction").

		for (int lineNdx = 1; lineNdx < static_cast<int>(lineDatasSortedByLevel.size()); ++lineNdx)
		{
			const LineData&		curData				= lineDatasSortedByLevel[lineNdx];
			const LineData&		prevData			= lineDatasSortedByLevel[lineNdx-1];

			if (curData.additionalSegmentLength < 0.0f || prevData.additionalSegmentLength < 0.0f)
				continue; // Unknown segment lengths, skip.

			const float			curClampedLevel		= getClampedTessLevel(spacingMode, curData.tessLevel);
			const float			prevClampedLevel	= getClampedTessLevel(spacingMode, prevData.tessLevel);
			const int			curFinalLevel		= getRoundedTessLevel(spacingMode, curClampedLevel);
			const int			prevFinalLevel		= getRoundedTessLevel(spacingMode, prevClampedLevel);

			if (curFinalLevel != prevFinalLevel)
				continue;

			const float			curFraction		= static_cast<float>(curFinalLevel) - curClampedLevel;
			const float			prevFraction	= static_cast<float>(prevFinalLevel) - prevClampedLevel;

			if (curData.additionalSegmentLength < prevData.additionalSegmentLength ||
				(curClampedLevel == prevClampedLevel && curData.additionalSegmentLength != prevData.additionalSegmentLength))
			{
				log << tcu::TestLog::Message << "Failure: additional segment length isn't monotonically decreasing with the fraction <n> - <f>, among edges with same final tessellation level" << tcu::TestLog::EndMessage
					<< tcu::TestLog::Message << "Note: <f> stands for the clamped tessellation level and <n> for the final (rounded and clamped) tessellation level" << tcu::TestLog::EndMessage
					<< tcu::TestLog::Message << "Note: two edges have tessellation levels " << prevData.tessLevel << " and " << curData.tessLevel << " respectively"
											 << ", clamped " << prevClampedLevel << " and " << curClampedLevel << ", final " << prevFinalLevel << " and " << curFinalLevel
											 << "; fractions are " << prevFraction << " and " << curFraction
											 << ", but resulted in segment lengths " << prevData.additionalSegmentLength << " and " << curData.additionalSegmentLength << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	return true;
}

std::vector<float> genTessLevelCases (void)
{
	std::vector<float> result;

	// Ranges [7.0 .. 8.0), [8.0 .. 9.0) and [9.0 .. 10.0)
	{
		static const float	rangeStarts[]		= { 7.0f, 8.0f, 9.0f };
		const int			numSamplesPerRange	= 10;

		for (int rangeNdx = 0; rangeNdx < DE_LENGTH_OF_ARRAY(rangeStarts); ++rangeNdx)
			for (int i = 0; i < numSamplesPerRange; ++i)
				result.push_back(rangeStarts[rangeNdx] + static_cast<float>(i)/numSamplesPerRange);
	}

	// 0.3, 1.3, 2.3,  ... , 62.3
	for (int i = 0; i <= 62; ++i)
		result.push_back(static_cast<float>(i) + 0.3f);

	return result;
}

//! Create a vector of floats from an array of floats. Offset is in bytes.
std::vector<float> readFloatArray(const int count, const void* memory, const int offset)
{
	std::vector<float> results(count);

	if (count != 0)
	{
		const float* pFloatData = reinterpret_cast<const float*>(static_cast<const deUint8*>(memory) + offset);
		deMemcpy(&results[0], pFloatData, sizeof(float) * count);
	}

	return results;
}

void initPrograms (vk::SourceCollections& programCollection, TestParams testParams)
{
	if (testParams.shaderLanguage == SHADER_LANGUAGE_GLSL)
	{
		// Vertex shader: no inputs
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
				<< "    float outer1;\n"
				<< "} sb_levels;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    gl_TessLevelOuter[0] = 1.0;\n"
				<< "    gl_TessLevelOuter[1] = sb_levels.outer1;\n"
				<< "}\n";

			programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
		}

		// Tessellation evaluation shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "#extension GL_EXT_tessellation_shader : require\n"
				<< "\n"
				<< "layout(" << getTessPrimitiveTypeShaderName(TESSPRIMITIVETYPE_ISOLINES) << ", "
							 << getSpacingModeShaderName(testParams.spacingMode) << ", point_mode) in;\n"
				<< "\n"
				<< "layout(set = 0, binding = 1, std430) coherent restrict buffer Output {\n"
				<< "    int   numInvocations;\n"
				<< "    float tessCoord[];\n"
				<< "} sb_out;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    int index = atomicAdd(sb_out.numInvocations, 1);\n"
				<< "    sb_out.tessCoord[index] = gl_TessCoord.x;\n"
				<< "}\n";

			programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
		}
	}
	else
	{
		// Vertex shader - no inputs
		{
			std::ostringstream src;
			src << "void main (void)\n"
				<< "{\n"
				<< "}\n";

			programCollection.hlslSources.add("vert") << glu::VertexSource(src.str());
		}

		// Tessellation control shader
		{
			std::ostringstream src;
			src << "struct HS_CONSTANT_OUT\n"
				<< "{\n"
				<< "    float tessLevelsOuter[2] : SV_TessFactor;\n"
				<< "};\n"
				<< "\n"
				<< "tbuffer TessLevels : register(b0)\n"
				<< "{\n"
				<< "    float outer1;\n"
				<< "}\n"
				<< "\n"
				<< "[domain(\"isoline\")]\n"
				<< "[partitioning(\"" << getPartitioningShaderName(testParams.spacingMode) << "\")]\n"
				<< "[outputtopology(\"point\")]\n"
				<< "[outputcontrolpoints(1)]\n"
				<< "[patchconstantfunc(\"PCF\")]\n"
				<< "void main()\n"
				<< "{\n"
				<< "}\n"
				<< "\n"
				<< "HS_CONSTANT_OUT PCF()\n"
				<< "{\n"
				<< "    HS_CONSTANT_OUT output;\n"
				<< "    output.tessLevelsOuter[0] = 1.0;\n"
				<< "    output.tessLevelsOuter[1] = outer1;\n"
				<< "    return output;\n"
				<< "}\n";

			programCollection.hlslSources.add("tesc") << glu::TessellationControlSource(src.str());
		}

		// Tessellation evaluation shader
		{
			std::ostringstream src;

			src	<< "struct OutputStruct\n"
				<< "{\n"
				<< "    int numInvocations;\n"
				<< "    float tessCoord[];\n"
				<< "};\n"
				<< "globallycoherent RWStructuredBuffer <OutputStruct> Output : register(b1);\n"
				<< "\n"
				<< "void main(float2 tessCoords : SV_DOMAINLOCATION)\n"
				<< "{\n"
				<< "    int index;\n"
				<< "    InterlockedAdd(Output[0].numInvocations, 1, index);\n"
				<< "    Output[0].tessCoord[index] = tessCoords.x;\n"
				<< "}\n";

			programCollection.hlslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
		}
	}
}

tcu::TestStatus test (Context& context, TestParams testParams)
{
	DE_ASSERT(testParams.spacingMode == SPACINGMODE_FRACTIONAL_ODD || testParams.spacingMode == SPACINGMODE_FRACTIONAL_EVEN);
	DE_ASSERT(testParams.shaderLanguage == SHADER_LANGUAGE_GLSL || testParams.shaderLanguage == SHADER_LANGUAGE_HLSL);

	requireFeatures(context.getInstanceInterface(), context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER | FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS);

	const DeviceInterface&	vk					= context.getDeviceInterface();
	const VkDevice			device				= context.getDevice();
	const VkQueue			queue				= context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= context.getDefaultAllocator();

	const std::vector<float>	tessLevelCases = genTessLevelCases();
	const int					maxNumVertices = 1 + getClampedRoundedTessLevel(testParams.spacingMode, *std::max_element(tessLevelCases.begin(), tessLevelCases.end()));

	// Result buffer: generated tess coords go here.

	const VkDeviceSize resultBufferSizeBytes = sizeof(int) + sizeof(float) * maxNumVertices;
	const Buffer	   resultBuffer			 (vk, device, allocator, makeBufferCreateInfo(resultBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Outer1 tessellation level constant buffer.

	const VkDeviceSize tessLevelsBufferSizeBytes = sizeof(float);  // we pass only outer1
	const Buffer	   tessLevelsBuffer			 (vk, device, allocator, makeBufferCreateInfo(tessLevelsBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Descriptors

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet			(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
	const VkDescriptorBufferInfo  tessLevelsBufferInfo	= makeDescriptorBufferInfo(tessLevelsBuffer.get(), 0ull, tessLevelsBufferSizeBytes);
	const VkDescriptorBufferInfo  resultBufferInfo		= makeDescriptorBufferInfo(resultBuffer.get(), 0ull, resultBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &tessLevelsBufferInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultBufferInfo)
		.update(vk, device);

	// Pipeline

	const Unique<VkRenderPass>		renderPass	  (makeRenderPassWithoutAttachments	(vk, device));
	const Unique<VkFramebuffer>		framebuffer	  (makeFramebufferWithoutAttachments(vk, device, *renderPass));
	const Unique<VkPipelineLayout>	pipelineLayout(makePipelineLayout				(vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>		cmdPool		  (makeCommandPool					(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer	  (allocateCommandBuffer			(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
		.setShader(vk, device, VK_SHADER_STAGE_VERTEX_BIT,					context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader(vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader(vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, context.getBinaryCollection().get("tese"), DE_NULL)
		.build(vk, device, *pipelineLayout, *renderPass));

	// Data that will be verified across all cases
	std::vector<float> additionalSegmentLengths;
	std::vector<int>   additionalSegmentLocations;

	bool success = false;

	// Repeat the test for all tessellation coords cases
	for (deUint32 tessLevelCaseNdx = 0; tessLevelCaseNdx < tessLevelCases.size(); ++tessLevelCaseNdx)
	{
		// Upload tessellation levels data to the input buffer
		{
			const Allocation& alloc			  = tessLevelsBuffer.getAllocation();
			float* const	  tessLevelOuter1 = static_cast<float*>(alloc.getHostPtr());

			*tessLevelOuter1 = tessLevelCases[tessLevelCaseNdx];
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), tessLevelsBufferSizeBytes);
		}

		// Clear the results buffer
		{
			const Allocation& alloc = resultBuffer.getAllocation();
			deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(resultBufferSizeBytes));
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), resultBufferSizeBytes);
		}

		beginCommandBuffer(vk, *cmdBuffer);

		// Begin render pass
		beginRenderPassWithRasterizationDisabled(vk, *cmdBuffer, *renderPass, *framebuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

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

		// Verify the result.
		{
			tcu::TestLog& log = context.getTestContext().getLog();

			const Allocation& resultAlloc = resultBuffer.getAllocation();
			invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), resultBufferSizeBytes);

			const deInt32 numResults = *static_cast<deInt32*>(resultAlloc.getHostPtr());
			const std::vector<float> resultTessCoords = readFloatArray(numResults, resultAlloc.getHostPtr(), sizeof(deInt32));

			// Outputs
			float additionalSegmentLength;
			int   additionalSegmentLocation;

			success = verifyFractionalSpacingSingle(log, testParams.spacingMode, tessLevelCases[tessLevelCaseNdx], resultTessCoords,
													&additionalSegmentLength, &additionalSegmentLocation);

			if (!success)
				break;

			additionalSegmentLengths.push_back(additionalSegmentLength);
			additionalSegmentLocations.push_back(additionalSegmentLocation);
		}
	} // for tessLevelCaseNdx

	if (success)
		success = verifyFractionalSpacingMultiple(context.getTestContext().getLog(), testParams.spacingMode, tessLevelCases, additionalSegmentLengths, additionalSegmentLocations);

	return (success ? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Failure"));
}

} // anonymous

//! These tests correspond to dEQP-GLES31.functional.tessellation.fractional_spacing.*
//! Check validity of fractional spacing modes. Draws a single isoline, reads tess coords with SSBO.
tcu::TestCaseGroup* createFractionalSpacingTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "fractional_spacing", "Test fractional spacing modes"));

	addFunctionCaseWithPrograms(group.get(), "glsl_odd",  "", initPrograms, test, TestParams(SHADER_LANGUAGE_GLSL, SPACINGMODE_FRACTIONAL_ODD));
	addFunctionCaseWithPrograms(group.get(), "glsl_even", "", initPrograms, test, TestParams(SHADER_LANGUAGE_GLSL, SPACINGMODE_FRACTIONAL_EVEN));
	addFunctionCaseWithPrograms(group.get(), "hlsl_odd",  "", initPrograms, test, TestParams(SHADER_LANGUAGE_HLSL, SPACINGMODE_FRACTIONAL_ODD));
	addFunctionCaseWithPrograms(group.get(), "hlsl_even", "", initPrograms, test, TestParams(SHADER_LANGUAGE_HLSL, SPACINGMODE_FRACTIONAL_EVEN));

	return group.release();
}

} // tessellation
} // vkt
