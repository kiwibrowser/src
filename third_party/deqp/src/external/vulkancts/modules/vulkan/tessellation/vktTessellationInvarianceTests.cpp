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
 * \brief Tessellation Invariance Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationInvarianceTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"

#include <string>
#include <vector>
#include <set>

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

enum Constants
{
	NUM_TESS_LEVELS = 6,  // two inner and four outer levels
};

enum WindingUsage
{
	WINDING_USAGE_CCW = 0,
	WINDING_USAGE_CW,
	WINDING_USAGE_VARY,

	WINDING_USAGE_LAST,
};

inline WindingUsage getWindingUsage (const Winding winding)
{
	const WindingUsage usage = winding == WINDING_CCW ? WINDING_USAGE_CCW :
							   winding == WINDING_CW  ? WINDING_USAGE_CW  : WINDING_USAGE_LAST;
	DE_ASSERT(usage !=  WINDING_USAGE_LAST);
	return usage;
}

std::vector<Winding> getWindingCases (const WindingUsage windingUsage)
{
	std::vector<Winding> cases;
	switch (windingUsage)
	{
		case WINDING_USAGE_CCW:
			cases.push_back(WINDING_CCW);
			break;
		case WINDING_USAGE_CW:
			cases.push_back(WINDING_CW);
			break;
		case WINDING_USAGE_VARY:
			cases.push_back(WINDING_CCW);
			cases.push_back(WINDING_CW);
			break;
		default:
			DE_ASSERT(false);
			break;
	}
	return cases;
}

enum PointModeUsage
{
	POINT_MODE_USAGE_DONT_USE = 0,
	POINT_MODE_USAGE_USE,
	POINT_MODE_USAGE_VARY,

	POINT_MODE_USAGE_LAST,
};

inline PointModeUsage getPointModeUsage (const bool usePointMode)
{
	return usePointMode ? POINT_MODE_USAGE_USE : POINT_MODE_USAGE_DONT_USE;
}

std::vector<bool> getUsePointModeCases (const PointModeUsage pointModeUsage)
{
	std::vector<bool> cases;
	switch (pointModeUsage)
	{
		case POINT_MODE_USAGE_DONT_USE:
			cases.push_back(false);
			break;
		case POINT_MODE_USAGE_USE:
			cases.push_back(true);
			break;
		case POINT_MODE_USAGE_VARY:
			cases.push_back(false);
			cases.push_back(true);
			break;
		default:
			DE_ASSERT(false);
			break;
	}
	return cases;
}

//! Data captured in the shader per output primitive (in geometry stage).
struct PerPrimitive
{
	deInt32		patchPrimitiveID;	//!< gl_PrimitiveID in tessellation evaluation shader
	deInt32		primitiveID;		//!< ID of an output primitive in geometry shader (user-defined)

	deInt32		unused_padding[2];

	tcu::Vec4	tessCoord[3];		//!< 3 coords for triangles/quads, 2 for isolines, 1 for point mode. Vec4 due to alignment.
};

typedef std::vector<PerPrimitive> PerPrimitiveVec;

inline bool byPatchPrimitiveID (const PerPrimitive& a, const PerPrimitive& b)
{
	return a.patchPrimitiveID < b.patchPrimitiveID;
}

inline std::string getProgramName (const std::string& baseName, const Winding winding, const bool usePointMode)
{
	std::ostringstream str;
	str << baseName << "_" << getWindingShaderName(winding) << (usePointMode ? "_point_mode" : "");
	return str.str();
}

inline std::string getProgramName (const std::string& baseName, const bool usePointMode)
{
	std::ostringstream str;
	str << baseName << (usePointMode ? "_point_mode" : "");
	return str.str();
}

inline std::string getProgramDescription (const Winding winding, const bool usePointMode)
{
	std::ostringstream str;
	str << "winding mode " << getWindingShaderName(winding) << ", " << (usePointMode ? "" : "don't ") << "use point mode";
	return str.str();
};

template <typename T, int N>
std::vector<T> arrayToVector (const T (&arr)[N])
{
	return std::vector<T>(DE_ARRAY_BEGIN(arr), DE_ARRAY_END(arr));
}

template <typename T, int N>
T arrayMax (const T (&arr)[N])
{
	return *std::max_element(DE_ARRAY_BEGIN(arr), DE_ARRAY_END(arr));
}

template <int Size>
inline tcu::Vector<bool, Size> singleTrueMask (int index)
{
	DE_ASSERT(de::inBounds(index, 0, Size));
	tcu::Vector<bool, Size> result;
	result[index] = true;
	return result;
}

template <typename ContainerT, typename T>
inline bool contains (const ContainerT& c, const T& key)
{
	return c.find(key) != c.end();
}

template <typename SeqT, int Size, typename Pred>
class LexCompare
{
public:
	LexCompare (void) : m_pred(Pred()) {}

	bool operator() (const SeqT& a, const SeqT& b) const
	{
		for (int i = 0; i < Size; ++i)
		{
			if (m_pred(a[i], b[i]))
				return true;
			if (m_pred(b[i], a[i]))
				return false;
		}
		return false;
	}

private:
	Pred m_pred;
};

template <int Size>
class VecLexLessThan : public LexCompare<tcu::Vector<float, Size>, Size, std::less<float> >
{
};

//! Add default programs for invariance tests.
//! Creates multiple shader programs for combinations of winding and point mode.
//! mirrorCoords - special mode where some tessellation coordinates are mirrored in tessellation evaluation shader.
//!                This is used by symmetric outer edge test.
void addDefaultPrograms (vk::SourceCollections&  programCollection,
						 const TessPrimitiveType primitiveType,
						 const SpacingMode       spacingMode,
						 const WindingUsage      windingUsage,
						 const PointModeUsage    pointModeUsage,
						 const bool				 mirrorCoords = false)
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp float in_v_attr;\n"
			<< "layout(location = 0) out highp float in_tc_attr;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    in_tc_attr = in_v_attr;\n"
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
			<< "layout(location = 0) in highp float in_tc_attr[];\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_TessLevelInner[0] = in_tc_attr[0];\n"
			<< "    gl_TessLevelInner[1] = in_tc_attr[1];\n"
			<< "\n"
			<< "    gl_TessLevelOuter[0] = in_tc_attr[2];\n"
			<< "    gl_TessLevelOuter[1] = in_tc_attr[3];\n"
			<< "    gl_TessLevelOuter[2] = in_tc_attr[4];\n"
			<< "    gl_TessLevelOuter[3] = in_tc_attr[5];\n"
			<< "}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	const std::string perVertexInterfaceBlock = \
		"VertexData {\n"					// no in/out qualifier
		"    vec4 in_gs_tessCoord;\n"		// w component is used by mirroring test
		"    int  in_gs_primitiveID;\n"
		"}";								// no newline nor semicolon

	// Alternative tess coordinates handling code
	std::ostringstream tessEvalCoordSrc;
	if (mirrorCoords)
		switch (primitiveType)
		{
			case TESSPRIMITIVETYPE_TRIANGLES:
				tessEvalCoordSrc << "    float x = gl_TessCoord.x;\n"
								 << "    float y = gl_TessCoord.y;\n"
								 << "    float z = gl_TessCoord.z;\n"
								 << "\n"
								 << "    // Mirror one half of each outer edge onto the other half, except the endpoints (because they belong to two edges)\n"
								 << "    ib_out.in_gs_tessCoord   = z == 0.0 && x > 0.5 && x != 1.0 ? vec4(1.0-x,  1.0-y,    0.0, 1.0)\n"
								 << "                             : y == 0.0 && z > 0.5 && z != 1.0 ? vec4(1.0-x,    0.0,  1.0-z, 1.0)\n"
								 << "                             : x == 0.0 && y > 0.5 && y != 1.0 ? vec4(  0.0,  1.0-y,  1.0-z, 1.0)\n"
								 << "                             : vec4(x, y, z, 0.0);\n";
				break;
			case TESSPRIMITIVETYPE_QUADS:
				tessEvalCoordSrc << "    float x = gl_TessCoord.x;\n"
								 << "    float y = gl_TessCoord.y;\n"
								 << "\n"
								 << "    // Mirror one half of each outer edge onto the other half, except the endpoints (because they belong to two edges)\n"
								 << "    ib_out.in_gs_tessCoord   = (x == 0.0 || x == 1.0) && y > 0.5 && y != 1.0 ? vec4(    x, 1.0-y, 0.0, 1.0)\n"
								 << "                             : (y == 0.0 || y == 1.0) && x > 0.5 && x != 1.0 ? vec4(1.0-x,     y, 0.0, 1.0)\n"
								 << "                             : vec4(x, y, 0.0, 0.0);\n";
				break;
			case TESSPRIMITIVETYPE_ISOLINES:
				tessEvalCoordSrc << "    float x = gl_TessCoord.x;\n"
								 << "    float y = gl_TessCoord.y;\n"
								 << "\n"
								 << "    // Mirror one half of each outer edge onto the other half\n"
								 << "    ib_out.in_gs_tessCoord   = (x == 0.0 || x == 1.0) && y > 0.5 ? vec4(x, 1.0-y, 0.0, 1.0)\n"
								 << "                             : vec4(x, y, 0.0, 0.0);\n";
				break;
			default:
				DE_ASSERT(false);
				return;
		}
	else
		tessEvalCoordSrc << "    ib_out.in_gs_tessCoord   = vec4(gl_TessCoord, 0.0);\n";

	const std::vector<Winding> windingCases      = getWindingCases(windingUsage);
	const std::vector<bool>    usePointModeCases = getUsePointModeCases(pointModeUsage);

	for (std::vector<Winding>::const_iterator windingIter = windingCases.begin(); windingIter != windingCases.end(); ++windingIter)
	for (std::vector<bool>::const_iterator usePointModeIter = usePointModeCases.begin(); usePointModeIter != usePointModeCases.end(); ++usePointModeIter)
	{
		// Tessellation evaluation shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "#extension GL_EXT_tessellation_shader : require\n"
				<< "\n"
				<< "layout(" << getTessPrimitiveTypeShaderName(primitiveType) << ", "
							 << getSpacingModeShaderName(spacingMode) << ", "
							 << getWindingShaderName(*windingIter)
							 << (*usePointModeIter ? ", point_mode" : "") << ") in;\n"
				<< "\n"
				<< "layout(location = 0) out " << perVertexInterfaceBlock << " ib_out;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< tessEvalCoordSrc.str()
				<< "    ib_out.in_gs_primitiveID = gl_PrimitiveID;\n"
				<< "}\n";

			programCollection.glslSources.add(getProgramName("tese", *windingIter, *usePointModeIter)) << glu::TessellationEvaluationSource(src.str());
		}
	}  // for windingNdx, usePointModeNdx

	// Geometry shader: data is captured here.
	{
		for (std::vector<bool>::const_iterator usePointModeIter = usePointModeCases.begin(); usePointModeIter != usePointModeCases.end(); ++usePointModeIter)
		{
			const int numVertices = numVerticesPerPrimitive(primitiveType, *usePointModeIter);  // Primitives that the tessellated patch comprises of.

			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "#extension GL_EXT_geometry_shader : require\n"
				<< "\n"
				<< "layout(" << getGeometryShaderInputPrimitiveTypeShaderName(primitiveType, *usePointModeIter) << ") in;\n"
				<< "layout(" << getGeometryShaderOutputPrimitiveTypeShaderName(primitiveType, *usePointModeIter) << ", max_vertices = " << numVertices << ") out;\n"
				<< "\n"
				<< "layout(location = 0) in " << perVertexInterfaceBlock << " ib_in[];\n"
				<< "\n"
				<< "struct PerPrimitive {\n"
				<< "    int  patchPrimitiveID;\n"
				<< "    int  primitiveID;\n"
				<< "    vec4 tessCoord[3];\n"
				<< "};\n"
				<< "\n"
				<< "layout(set = 0, binding = 0, std430) coherent restrict buffer Output {\n"
				<< "    int          numPrimitives;\n"
				<< "    PerPrimitive primitive[];\n"
				<< "} sb_out;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    int index = atomicAdd(sb_out.numPrimitives, 1);\n"
				<< "    sb_out.primitive[index].patchPrimitiveID = ib_in[0].in_gs_primitiveID;\n"
				<< "    sb_out.primitive[index].primitiveID      = index;\n";
			for (int i = 0; i < numVertices; ++i)
				src << "    sb_out.primitive[index].tessCoord[" << i << "]     = ib_in[" << i << "].in_gs_tessCoord;\n";
			for (int i = 0; i < numVertices; ++i)
				src << "\n"
					<< "    gl_Position = vec4(0.0);\n"
					<< "    EmitVertex();\n";
			src << "}\n";

			programCollection.glslSources.add(getProgramName("geom", *usePointModeIter)) << glu::GeometrySource(src.str());
		}
	}
}

//! A description of an outer edge of a triangle, quad or isolines.
//! An outer edge can be described by the index of a u/v/w coordinate
//! and the coordinate's value along that edge.
struct OuterEdgeDescription
{
	int		constantCoordinateIndex;
	float	constantCoordinateValueChoices[2];
	int		numConstantCoordinateValueChoices;

	OuterEdgeDescription (const int i, const float c0)					: constantCoordinateIndex(i), numConstantCoordinateValueChoices(1) { constantCoordinateValueChoices[0] = c0; }
	OuterEdgeDescription (const int i, const float c0, const float c1)	: constantCoordinateIndex(i), numConstantCoordinateValueChoices(2) { constantCoordinateValueChoices[0] = c0; constantCoordinateValueChoices[1] = c1; }

	std::string description (void) const
	{
		static const char* const	coordinateNames[] = { "u", "v", "w" };
		std::string					result;
		for (int i = 0; i < numConstantCoordinateValueChoices; ++i)
			result += std::string() + (i > 0 ? " or " : "") + coordinateNames[constantCoordinateIndex] + "=" + de::toString(constantCoordinateValueChoices[i]);
		return result;
	}

	bool contains (const tcu::Vec3& v) const
	{
		for (int i = 0; i < numConstantCoordinateValueChoices; ++i)
			if (v[constantCoordinateIndex] == constantCoordinateValueChoices[i])
				return true;
		return false;
	}
};

std::vector<OuterEdgeDescription> outerEdgeDescriptions (const TessPrimitiveType primType)
{
	static const OuterEdgeDescription triangleOuterEdgeDescriptions[3] =
	{
		OuterEdgeDescription(0, 0.0f),
		OuterEdgeDescription(1, 0.0f),
		OuterEdgeDescription(2, 0.0f)
	};

	static const OuterEdgeDescription quadOuterEdgeDescriptions[4] =
	{
		OuterEdgeDescription(0, 0.0f),
		OuterEdgeDescription(1, 0.0f),
		OuterEdgeDescription(0, 1.0f),
		OuterEdgeDescription(1, 1.0f)
	};

	static const OuterEdgeDescription isolinesOuterEdgeDescriptions[1] =
	{
		OuterEdgeDescription(0, 0.0f, 1.0f),
	};

	switch (primType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:	return arrayToVector(triangleOuterEdgeDescriptions);
		case TESSPRIMITIVETYPE_QUADS:		return arrayToVector(quadOuterEdgeDescriptions);
		case TESSPRIMITIVETYPE_ISOLINES:	return arrayToVector(isolinesOuterEdgeDescriptions);

		default:
			DE_ASSERT(false);
			return std::vector<OuterEdgeDescription>();
	}
}

namespace InvariantOuterEdge
{

struct CaseDefinition
{
	TessPrimitiveType	primitiveType;
	SpacingMode			spacingMode;
	Winding				winding;
	bool				usePointMode;
};

typedef std::set<tcu::Vec3, VecLexLessThan<3> > Vec3Set;

std::vector<float> generateRandomPatchTessLevels (const int numPatches, const int constantOuterLevelIndex, const float constantOuterLevel, de::Random& rnd)
{
	std::vector<float> tessLevels(numPatches*NUM_TESS_LEVELS);

	for (int patchNdx = 0; patchNdx < numPatches; ++patchNdx)
	{
		float* const inner = &tessLevels[patchNdx*NUM_TESS_LEVELS + 0];
		float* const outer = &tessLevels[patchNdx*NUM_TESS_LEVELS + 2];

		for (int j = 0; j < 2; ++j)
			inner[j] = rnd.getFloat(1.0f, 62.0f);
		for (int j = 0; j < 4; ++j)
			outer[j] = j == constantOuterLevelIndex ? constantOuterLevel : rnd.getFloat(1.0f, 62.0f);
	}

	return tessLevels;
}

std::vector<float> generatePatchTessLevels (const int numPatches, const int constantOuterLevelIndex, const float constantOuterLevel)
{
	de::Random rnd(123);
	return generateRandomPatchTessLevels(numPatches, constantOuterLevelIndex, constantOuterLevel, rnd);
}

int multiplePatchReferencePrimitiveCount (const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const bool usePointMode, const float* levels, int numPatches)
{
	int result = 0;
	for (int patchNdx = 0; patchNdx < numPatches; ++patchNdx)
		result += referencePrimitiveCount(primitiveType, spacingMode, usePointMode, &levels[NUM_TESS_LEVELS*patchNdx + 0], &levels[NUM_TESS_LEVELS*patchNdx + 2]);
	return result;
}

template<std::size_t N>
int computeMaxPrimitiveCount (const int numPatchesToDraw, const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const bool usePointMode, const float (&singleOuterEdgeLevels)[N])
{
	const int                outerEdgeIndex  = 0; // outer-edge index doesn't affect vertex count
	const std::vector<float> patchTessLevels = generatePatchTessLevels(numPatchesToDraw, outerEdgeIndex, arrayMax(singleOuterEdgeLevels));
	return multiplePatchReferencePrimitiveCount(primitiveType, spacingMode, usePointMode, &patchTessLevels[0], numPatchesToDraw);
}

void logOuterTessellationLevel (tcu::TestLog& log, const float tessLevel, const OuterEdgeDescription& edgeDesc)
{
	log << tcu::TestLog::Message
		<< "Testing with outer tessellation level " << tessLevel << " for the " << edgeDesc.description() << " edge, and with various levels for other edges, and with all programs"
		<< tcu::TestLog::EndMessage;
}

void logPrimitiveCountError (tcu::TestLog& log, const int numPatchesToDraw, int numPrimitives, const int refNumPrimitives, const std::vector<float>& patchTessLevels)
{
	log << tcu::TestLog::Message
		<< "Failure: the number of generated primitives is " << numPrimitives << ", expected at least " << refNumPrimitives
		<< tcu::TestLog::EndMessage;

	if (numPatchesToDraw == 1)
		log << tcu::TestLog::Message
			<< "Note: rendered one patch; tessellation levels are (in order [inner0, inner1, outer0, outer1, outer2, outer3]):\n"
			<< containerStr(patchTessLevels, NUM_TESS_LEVELS)
			<< tcu::TestLog::EndMessage;
	else
		log << tcu::TestLog::Message
			<< "Note: rendered " << numPatchesToDraw << " patches in one draw call; "
			<< "tessellation levels for each patch are (in order [inner0, inner1, outer0, outer1, outer2, outer3]):\n"
			<< containerStr(patchTessLevels, NUM_TESS_LEVELS)
			<< tcu::TestLog::EndMessage;
}

class BaseTestInstance : public TestInstance
{
public:
	struct DrawResult
	{
		bool			success;
		int				refNumPrimitives;
		int				numPrimitiveVertices;
		deInt32			numPrimitives;
		PerPrimitiveVec	primitives;
	};

											BaseTestInstance		(Context& context, const CaseDefinition caseDef, const int numPatchesToDraw);
	DrawResult								draw					(const deUint32 vertexCount, const std::vector<float>& patchTessLevels, const Winding winding, const bool usePointMode);
	void									uploadVertexAttributes	(const std::vector<float>& vertexData);

protected:
	static const float						m_singleOuterEdgeLevels[];

	const CaseDefinition					m_caseDef;
	const int								m_numPatchesToDraw;
	const VkFormat							m_vertexFormat;
	const deUint32							m_vertexStride;
	const std::vector<OuterEdgeDescription>	m_edgeDescriptions;
	const int								m_maxNumPrimitivesInDrawCall;
	const VkDeviceSize						m_vertexDataSizeBytes;
	const Buffer							m_vertexBuffer;
	const int								m_resultBufferPrimitiveDataOffset;
	const VkDeviceSize						m_resultBufferSizeBytes;
	const Buffer							m_resultBuffer;
	Unique<VkDescriptorSetLayout>			m_descriptorSetLayout;
	Unique<VkDescriptorPool>				m_descriptorPool;
	Unique<VkDescriptorSet>					m_descriptorSet;
	Unique<VkRenderPass>					m_renderPass;
	Unique<VkFramebuffer>					m_framebuffer;
	Unique<VkPipelineLayout>				m_pipelineLayout;
	Unique<VkCommandPool>					m_cmdPool;
	Unique<VkCommandBuffer>					m_cmdBuffer;
};

const float BaseTestInstance::m_singleOuterEdgeLevels[] = { 1.0f, 1.2f, 1.9f, 2.3f, 2.8f, 3.3f, 3.8f, 10.2f, 1.6f, 24.4f, 24.7f, 63.0f };

BaseTestInstance::BaseTestInstance (Context& context, const CaseDefinition caseDef, const int numPatchesToDraw)
	: TestInstance							(context)
	, m_caseDef								(caseDef)
	, m_numPatchesToDraw					(numPatchesToDraw)
	, m_vertexFormat						(VK_FORMAT_R32_SFLOAT)
	, m_vertexStride						(tcu::getPixelSize(mapVkFormat(m_vertexFormat)))
	, m_edgeDescriptions					(outerEdgeDescriptions(m_caseDef.primitiveType))
	, m_maxNumPrimitivesInDrawCall			(computeMaxPrimitiveCount(m_numPatchesToDraw, caseDef.primitiveType, caseDef.spacingMode, caseDef.usePointMode, m_singleOuterEdgeLevels))
	, m_vertexDataSizeBytes					(NUM_TESS_LEVELS * m_numPatchesToDraw * m_vertexStride)
	, m_vertexBuffer						(m_context.getDeviceInterface(), m_context.getDevice(), m_context.getDefaultAllocator(),
											makeBufferCreateInfo(m_vertexDataSizeBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible)
	, m_resultBufferPrimitiveDataOffset		((int)sizeof(deInt32) * 4)
	, m_resultBufferSizeBytes				(m_resultBufferPrimitiveDataOffset + m_maxNumPrimitivesInDrawCall * sizeof(PerPrimitive))
	, m_resultBuffer						(m_context.getDeviceInterface(), m_context.getDevice(), m_context.getDefaultAllocator(),
											makeBufferCreateInfo(m_resultBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible)
	, m_descriptorSetLayout					(DescriptorSetLayoutBuilder()
											.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT)
											.build(m_context.getDeviceInterface(), m_context.getDevice()))
	, m_descriptorPool						(DescriptorPoolBuilder()
											.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
											.build(m_context.getDeviceInterface(), m_context.getDevice(), VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u))
	, m_descriptorSet						(makeDescriptorSet(m_context.getDeviceInterface(), m_context.getDevice(), *m_descriptorPool, *m_descriptorSetLayout))
	, m_renderPass							(makeRenderPassWithoutAttachments (m_context.getDeviceInterface(), m_context.getDevice()))
	, m_framebuffer							(makeFramebufferWithoutAttachments(m_context.getDeviceInterface(), m_context.getDevice(), *m_renderPass))
	, m_pipelineLayout						(makePipelineLayout               (m_context.getDeviceInterface(), m_context.getDevice(), *m_descriptorSetLayout))
	, m_cmdPool								(makeCommandPool                  (m_context.getDeviceInterface(), m_context.getDevice(), m_context.getUniversalQueueFamilyIndex()))
	, m_cmdBuffer							(allocateCommandBuffer            (m_context.getDeviceInterface(), m_context.getDevice(), *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY))
{
	requireFeatures(context.getInstanceInterface(), context.getPhysicalDevice(),
					FEATURE_TESSELLATION_SHADER | FEATURE_GEOMETRY_SHADER | FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS);

	const VkDescriptorBufferInfo resultBufferInfo = makeDescriptorBufferInfo(m_resultBuffer.get(), 0ull, m_resultBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultBufferInfo)
		.update(m_context.getDeviceInterface(), m_context.getDevice());
}

//! patchTessLevels are tessellation levels for all drawn patches.
BaseTestInstance::DrawResult BaseTestInstance::draw (const deUint32 vertexCount, const std::vector<float>& patchTessLevels, const Winding winding, const bool usePointMode)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();
	const VkQueue			queue	= m_context.getUniversalQueue();

	const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
		.setPatchControlPoints        (NUM_TESS_LEVELS)
		.setVertexInputSingleAttribute(m_vertexFormat, m_vertexStride)
		.setShader                    (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					m_context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader                    (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	m_context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader                    (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_context.getBinaryCollection().get(getProgramName("tese", winding, usePointMode)), DE_NULL)
		.setShader                    (vk, device, VK_SHADER_STAGE_GEOMETRY_BIT,                m_context.getBinaryCollection().get(getProgramName("geom", usePointMode)), DE_NULL)
		.build                        (vk, device, *m_pipelineLayout, *m_renderPass));

	{
		const Allocation& alloc = m_resultBuffer.getAllocation();
		deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(m_resultBufferSizeBytes));
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), m_resultBufferSizeBytes);
	}

	beginCommandBuffer(vk, *m_cmdBuffer);
	beginRenderPassWithRasterizationDisabled(vk, *m_cmdBuffer, *m_renderPass, *m_framebuffer);

	vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	vk.cmdBindDescriptorSets(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);
	{
		const VkDeviceSize vertexBufferOffset = 0ull;
		vk.cmdBindVertexBuffers(*m_cmdBuffer, 0u, 1u, &m_vertexBuffer.get(), &vertexBufferOffset);
	}

	vk.cmdDraw(*m_cmdBuffer, vertexCount, 1u, 0u, 0u);
	endRenderPass(vk, *m_cmdBuffer);

	{
		const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *m_resultBuffer, 0ull, m_resultBufferSizeBytes);

		vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
			0u, DE_NULL, 1u, &shaderWriteBarrier, 0u, DE_NULL);
	}

	endCommandBuffer(vk, *m_cmdBuffer);
	submitCommandsAndWait(vk, device, queue, *m_cmdBuffer);

	// Read back and check results

	const Allocation& resultAlloc = m_resultBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), m_resultBufferSizeBytes);

	DrawResult result;
	result.success				= true;
	result.refNumPrimitives     = multiplePatchReferencePrimitiveCount(m_caseDef.primitiveType, m_caseDef.spacingMode, usePointMode, &patchTessLevels[0], m_numPatchesToDraw);
	result.numPrimitiveVertices = numVerticesPerPrimitive(m_caseDef.primitiveType, usePointMode);
	result.numPrimitives        = *static_cast<deInt32*>(resultAlloc.getHostPtr());
	result.primitives           = sorted(readInterleavedData<PerPrimitive>(result.numPrimitives, resultAlloc.getHostPtr(), m_resultBufferPrimitiveDataOffset, sizeof(PerPrimitive)),
										 byPatchPrimitiveID);

	// If this fails then we didn't read all vertices from shader and test must be changed to allow more.
	DE_ASSERT(result.numPrimitives <= m_maxNumPrimitivesInDrawCall);

	tcu::TestLog& log = m_context.getTestContext().getLog();
	if (result.numPrimitives != result.refNumPrimitives)
	{
		logPrimitiveCountError(log, m_numPatchesToDraw, result.numPrimitives, result.refNumPrimitives, patchTessLevels);
		result.success = false;
	}
	return result;
}

void BaseTestInstance::uploadVertexAttributes (const std::vector<float>& vertexData)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	const Allocation& alloc = m_vertexBuffer.getAllocation();
	deMemcpy(alloc.getHostPtr(), &vertexData[0], sizeInBytes(vertexData));
	flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), sizeInBytes(vertexData));
}

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #2
 *
 * Test that the set of vertices along an outer edge of a quad or triangle
 * only depends on that edge's tessellation level, and spacing.
 *
 * For each (outer) edge in the quad or triangle, draw multiple patches
 * with identical tessellation levels for that outer edge but with
 * different values for the other outer edges; compare, among the
 * primitives, the vertices generated for that outer edge. Repeat with
 * different programs, using different winding etc. settings. Compare
 * the edge's vertices between different programs.
 *//*--------------------------------------------------------------------*/
class OuterEdgeDivisionTestInstance : public BaseTestInstance
{
public:
						OuterEdgeDivisionTestInstance	(Context& context, const CaseDefinition caseDef) : BaseTestInstance (context, caseDef, 10) {}
	tcu::TestStatus		iterate							(void);
};

tcu::TestStatus OuterEdgeDivisionTestInstance::iterate (void)
{
	for (int outerEdgeIndex = 0; outerEdgeIndex < static_cast<int>(m_edgeDescriptions.size()); ++outerEdgeIndex)
	for (int outerEdgeLevelCaseNdx = 0; outerEdgeLevelCaseNdx < DE_LENGTH_OF_ARRAY(m_singleOuterEdgeLevels); ++outerEdgeLevelCaseNdx)
	{
		const OuterEdgeDescription& edgeDesc        = m_edgeDescriptions[outerEdgeIndex];
		const std::vector<float>    patchTessLevels = generatePatchTessLevels(m_numPatchesToDraw, outerEdgeIndex, m_singleOuterEdgeLevels[outerEdgeLevelCaseNdx]);

		Vec3Set firstOuterEdgeVertices; // Vertices of the outer edge of the first patch of the first program's draw call; used for comparison with other patches.

		uploadVertexAttributes(patchTessLevels);
		logOuterTessellationLevel(m_context.getTestContext().getLog(), m_singleOuterEdgeLevels[outerEdgeLevelCaseNdx], edgeDesc);

		for (int windingNdx = 0; windingNdx < WINDING_LAST; ++windingNdx)
		for (int usePointModeNdx = 0; usePointModeNdx <= 1; ++usePointModeNdx)
		{
			const Winding winding	     = static_cast<Winding>(windingNdx);
			const bool	  usePointMode   = (usePointModeNdx != 0);
			const bool    isFirstProgram = (windingNdx == 0 && usePointModeNdx == 0);

			const DrawResult result = draw(static_cast<deUint32>(patchTessLevels.size()), patchTessLevels, winding, usePointMode);

			if (!result.success)
				return tcu::TestStatus::fail("Invalid set of vertices");

			// Check the vertices of each patch.

			int primitiveNdx = 0;
			for (int patchNdx = 0; patchNdx < m_numPatchesToDraw; ++patchNdx)
			{
				DE_ASSERT(primitiveNdx < result.numPrimitives);

				const float* const	innerLevels	= &patchTessLevels[NUM_TESS_LEVELS*patchNdx + 0];
				const float* const	outerLevels	= &patchTessLevels[NUM_TESS_LEVELS*patchNdx + 2];

				Vec3Set outerEdgeVertices;

				// We're interested in just the vertices on the current outer edge.
				for (; primitiveNdx < result.numPrimitives && result.primitives[primitiveNdx].patchPrimitiveID == patchNdx; ++primitiveNdx)
				for (int i = 0; i < result.numPrimitiveVertices; ++i)
				{
					const tcu::Vec3& coord = result.primitives[primitiveNdx].tessCoord[i].swizzle(0, 1, 2);
					if (edgeDesc.contains(coord))
						outerEdgeVertices.insert(coord);
				}

				// Compare the vertices to those of the first patch (unless this is the first patch).

				if (isFirstProgram && patchNdx == 0)
					firstOuterEdgeVertices = outerEdgeVertices;
				else if (firstOuterEdgeVertices != outerEdgeVertices)
				{
					tcu::TestLog& log = m_context.getTestContext().getLog();

					log << tcu::TestLog::Message
						<< "Failure: vertices generated for the edge differ between the following cases:\n"
						<< "  - case A: " << getProgramDescription((Winding)0, (bool)0) << ", tessellation levels: "
						<< getTessellationLevelsString(&patchTessLevels[0], &patchTessLevels[2]) << "\n"
						<< "  - case B: " << getProgramDescription(winding, usePointMode) << ", tessellation levels: "
						<< getTessellationLevelsString(innerLevels, outerLevels)
						<< tcu::TestLog::EndMessage;

					log << tcu::TestLog::Message
						<< "Note: resulting vertices for the edge for the cases were:\n"
						<< "  - case A: " << containerStr(firstOuterEdgeVertices, 5, 14) << "\n"
						<< "  - case B: " << containerStr(outerEdgeVertices, 5, 14)
						<< tcu::TestLog::EndMessage;

					return tcu::TestStatus::fail("Invalid set of vertices");
				}
			}
			DE_ASSERT(primitiveNdx == result.numPrimitives);
		} // for windingNdx, usePointModeNdx
	} // for outerEdgeIndex, outerEdgeLevelCaseNdx

	return tcu::TestStatus::pass("OK");
}

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #4
 *
 * Test that the vertices on an outer edge don't depend on which of the
 * edges it is, other than with respect to component order.
 *//*--------------------------------------------------------------------*/
class OuterEdgeIndexIndependenceTestInstance : public BaseTestInstance
{
public:
						OuterEdgeIndexIndependenceTestInstance	(Context& context, const CaseDefinition caseDef) : BaseTestInstance (context, caseDef, 1) {}
	tcu::TestStatus		iterate									(void);
};

tcu::TestStatus OuterEdgeIndexIndependenceTestInstance::iterate (void)
{
	for (int outerEdgeLevelCaseNdx = 0; outerEdgeLevelCaseNdx < DE_LENGTH_OF_ARRAY(m_singleOuterEdgeLevels); ++outerEdgeLevelCaseNdx)
	{
		Vec3Set firstEdgeVertices;

		for (int outerEdgeIndex = 0; outerEdgeIndex < static_cast<int>(m_edgeDescriptions.size()); ++outerEdgeIndex)
		{
			const OuterEdgeDescription& edgeDesc        = m_edgeDescriptions[outerEdgeIndex];
			const std::vector<float>    patchTessLevels = generatePatchTessLevels(m_numPatchesToDraw, outerEdgeIndex, m_singleOuterEdgeLevels[outerEdgeLevelCaseNdx]);

			uploadVertexAttributes(patchTessLevels);
			logOuterTessellationLevel(m_context.getTestContext().getLog(), m_singleOuterEdgeLevels[outerEdgeLevelCaseNdx], edgeDesc);
			const DrawResult result = draw(static_cast<deUint32>(patchTessLevels.size()), patchTessLevels, m_caseDef.winding, m_caseDef.usePointMode);

			// Verify case result

			if (!result.success)
				return tcu::TestStatus::fail("Invalid set of vertices");

			Vec3Set currentEdgeVertices;

			// Get the vertices on the current outer edge.
			for (int primitiveNdx = 0; primitiveNdx < result.numPrimitives; ++primitiveNdx)
			for (int i = 0; i < result.numPrimitiveVertices; ++i)
			{
				const tcu::Vec3& coord = result.primitives[primitiveNdx].tessCoord[i].swizzle(0, 1, 2);
				if (edgeDesc.contains(coord))
				{
					// Swizzle components to match the order of the first edge.
					if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
						currentEdgeVertices.insert(outerEdgeIndex == 0 ? coord :
												   outerEdgeIndex == 1 ? coord.swizzle(1, 0, 2) :
												   outerEdgeIndex == 2 ? coord.swizzle(2, 1, 0) : tcu::Vec3(-1.0f));
					else if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS)
						currentEdgeVertices.insert(tcu::Vec3(outerEdgeIndex == 0 ? coord.y() :
															 outerEdgeIndex == 1 ? coord.x() :
															 outerEdgeIndex == 2 ? coord.y() :
															 outerEdgeIndex == 3 ? coord.x() : -1.0f,
															 0.0f, 0.0f));
					else
						DE_ASSERT(false);
				}
			}

			if (outerEdgeIndex == 0)
				firstEdgeVertices = currentEdgeVertices;
			else
			{
				// Compare vertices of this edge to those of the first edge.
				if (currentEdgeVertices != firstEdgeVertices)
				{
					const char* const swizzleDesc =
						m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? (outerEdgeIndex == 1 ? "(y, x, z)" :
																				  outerEdgeIndex == 2 ? "(z, y, x)" : DE_NULL) :
						m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS ? (outerEdgeIndex == 1 ? "(x, 0)" :
																			  outerEdgeIndex == 2 ? "(y, 0)" :
																			  outerEdgeIndex == 3 ? "(x, 0)" : DE_NULL)
						: DE_NULL;

					tcu::TestLog& log = m_context.getTestContext().getLog();
					log << tcu::TestLog::Message
						<< "Failure: the set of vertices on the " << edgeDesc.description() << " edge"
						<< " doesn't match the set of vertices on the " << m_edgeDescriptions[0].description() << " edge"
						<< tcu::TestLog::EndMessage;

					log << tcu::TestLog::Message
						<< "Note: set of vertices on " << edgeDesc.description() << " edge, components swizzled like " << swizzleDesc
						<< " to match component order on first edge:\n" << containerStr(currentEdgeVertices, 5)
						<< "\non " << m_edgeDescriptions[0].description() << " edge:\n" << containerStr(firstEdgeVertices, 5)
						<< tcu::TestLog::EndMessage;

					return tcu::TestStatus::fail("Invalid set of vertices");
				}
			}
		}
	}
	return tcu::TestStatus::pass("OK");
}

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #3
 *
 * Test that the vertices along an outer edge are placed symmetrically.
 *
 * Draw multiple patches with different tessellation levels and different
 * point_mode, winding etc. Before outputting tesscoords from shader, mirror
 * the vertices in the TES such that every vertex on an outer edge -
 * except the possible middle vertex - should be duplicated in the output.
 * Check that appropriate duplicates exist.
 *//*--------------------------------------------------------------------*/
class SymmetricOuterEdgeTestInstance : public BaseTestInstance
{
public:
						SymmetricOuterEdgeTestInstance	(Context& context, const CaseDefinition caseDef) : BaseTestInstance (context, caseDef, 1) {}
	tcu::TestStatus		iterate							(void);
};

tcu::TestStatus SymmetricOuterEdgeTestInstance::iterate (void)
{
	for (int outerEdgeIndex = 0; outerEdgeIndex < static_cast<int>(m_edgeDescriptions.size()); ++outerEdgeIndex)
	for (int outerEdgeLevelCaseNdx = 0; outerEdgeLevelCaseNdx < DE_LENGTH_OF_ARRAY(m_singleOuterEdgeLevels); ++outerEdgeLevelCaseNdx)
	{
		const OuterEdgeDescription& edgeDesc        = m_edgeDescriptions[outerEdgeIndex];
		const std::vector<float>    patchTessLevels = generatePatchTessLevels(m_numPatchesToDraw, outerEdgeIndex, m_singleOuterEdgeLevels[outerEdgeLevelCaseNdx]);

		uploadVertexAttributes(patchTessLevels);
		logOuterTessellationLevel(m_context.getTestContext().getLog(), m_singleOuterEdgeLevels[outerEdgeLevelCaseNdx], edgeDesc);
		const DrawResult result = draw(static_cast<deUint32>(patchTessLevels.size()), patchTessLevels, m_caseDef.winding, m_caseDef.usePointMode);

		// Verify case result

		if (!result.success)
			return tcu::TestStatus::fail("Invalid set of vertices");

		Vec3Set nonMirroredEdgeVertices;
		Vec3Set mirroredEdgeVertices;

		// Get the vertices on the current outer edge.
		for (int primitiveNdx = 0; primitiveNdx < result.numPrimitives; ++primitiveNdx)
		for (int i = 0; i < result.numPrimitiveVertices; ++i)
		{
			const tcu::Vec3& coord = result.primitives[primitiveNdx].tessCoord[i].swizzle(0, 1, 2);
			if (edgeDesc.contains(coord))
			{
				// Ignore the middle vertex of the outer edge, as it's exactly at the mirroring point;
				// for isolines, also ignore (0, 0) and (1, 0) because there's no mirrored counterpart for them.
				if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES &&
					coord == tcu::select(tcu::Vec3(0.0f), tcu::Vec3(0.5f), singleTrueMask<3>(edgeDesc.constantCoordinateIndex)))
					continue;
				if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS &&
					coord.swizzle(0,1) == tcu::select(tcu::Vec2(edgeDesc.constantCoordinateValueChoices[0]), tcu::Vec2(0.5f), singleTrueMask<2>(edgeDesc.constantCoordinateIndex)))
					continue;
				if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_ISOLINES &&
					(coord == tcu::Vec3(0.0f, 0.5f, 0.0f) || coord == tcu::Vec3(1.0f, 0.5f, 0.0f) || coord == tcu::Vec3(0.0f, 0.0f, 0.0f) || coord == tcu::Vec3(1.0f, 0.0f, 0.0f)))
					continue;

				const bool isMirrored = result.primitives[primitiveNdx].tessCoord[i].w() > 0.5f;
				if (isMirrored)
					mirroredEdgeVertices.insert(coord);
				else
					nonMirroredEdgeVertices.insert(coord);
			}
		}

		if (m_caseDef.primitiveType != TESSPRIMITIVETYPE_ISOLINES)
		{
			// Check that both endpoints are present. Note that endpoints aren't mirrored by the shader, since they belong to more than one edge.

			tcu::Vec3 endpointA;
			tcu::Vec3 endpointB;

			if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
			{
				endpointA = tcu::select(tcu::Vec3(1.0f), tcu::Vec3(0.0f), singleTrueMask<3>((edgeDesc.constantCoordinateIndex + 1) % 3));
				endpointB = tcu::select(tcu::Vec3(1.0f), tcu::Vec3(0.0f), singleTrueMask<3>((edgeDesc.constantCoordinateIndex + 2) % 3));
			}
			else if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS)
			{
				endpointA.xy() = tcu::select(tcu::Vec2(edgeDesc.constantCoordinateValueChoices[0]), tcu::Vec2(0.0f), singleTrueMask<2>(edgeDesc.constantCoordinateIndex));
				endpointB.xy() = tcu::select(tcu::Vec2(edgeDesc.constantCoordinateValueChoices[0]), tcu::Vec2(1.0f), singleTrueMask<2>(edgeDesc.constantCoordinateIndex));
			}
			else
				DE_ASSERT(false);

			if (!contains(nonMirroredEdgeVertices, endpointA) ||
				!contains(nonMirroredEdgeVertices, endpointB))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Failure: edge doesn't contain both endpoints, " << endpointA << " and " << endpointB << tcu::TestLog::EndMessage
					<< tcu::TestLog::Message << "Note: non-mirrored vertices:\n" << containerStr(nonMirroredEdgeVertices, 5)
											 << "\nmirrored vertices:\n" << containerStr(mirroredEdgeVertices, 5) << tcu::TestLog::EndMessage;

				return tcu::TestStatus::fail("Invalid set of vertices");
			}
			nonMirroredEdgeVertices.erase(endpointA);
			nonMirroredEdgeVertices.erase(endpointB);
		}

		if (nonMirroredEdgeVertices != mirroredEdgeVertices)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Failure: the set of mirrored edges isn't equal to the set of non-mirrored edges (ignoring endpoints and possible middle)" << tcu::TestLog::EndMessage
				<< tcu::TestLog::Message << "Note: non-mirrored vertices:\n" << containerStr(nonMirroredEdgeVertices, 5)
										 << "\nmirrored vertices:\n" << containerStr(mirroredEdgeVertices, 5) << tcu::TestLog::EndMessage;

			return tcu::TestStatus::fail("Invalid set of vertices");
		}
	}
	return tcu::TestStatus::pass("OK");
}

class OuterEdgeDivisionTest : public TestCase
{
public:
	OuterEdgeDivisionTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const CaseDefinition caseDef)
		: TestCase	(testCtx, name, description)
		, m_caseDef	(caseDef)
	{
	}

	void initPrograms (vk::SourceCollections& programCollection) const
	{
		addDefaultPrograms(programCollection, m_caseDef.primitiveType, m_caseDef.spacingMode, WINDING_USAGE_VARY, POINT_MODE_USAGE_VARY);
	}

	TestInstance* createInstance (Context& context) const
	{
		return new OuterEdgeDivisionTestInstance(context, m_caseDef);
	};

private:
	const CaseDefinition m_caseDef;
};

class OuterEdgeIndexIndependenceTest : public TestCase
{
public:
	OuterEdgeIndexIndependenceTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const CaseDefinition caseDef)
		: TestCase	(testCtx, name, description)
		, m_caseDef	(caseDef)
	{
		DE_ASSERT(m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES || m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS);
	}

	void initPrograms (vk::SourceCollections& programCollection) const
	{
		addDefaultPrograms(programCollection, m_caseDef.primitiveType, m_caseDef.spacingMode, getWindingUsage(m_caseDef.winding), getPointModeUsage(m_caseDef.usePointMode));
	}

	TestInstance* createInstance (Context& context) const
	{
		return new OuterEdgeIndexIndependenceTestInstance(context, m_caseDef);
	};

private:
	const CaseDefinition m_caseDef;
};

class SymmetricOuterEdgeTest : public TestCase
{
public:
	SymmetricOuterEdgeTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const CaseDefinition caseDef)
		: TestCase	(testCtx, name, description)
		, m_caseDef	(caseDef)
	{
	}

	void initPrograms (vk::SourceCollections& programCollection) const
	{
		const bool mirrorCoords = true;
		addDefaultPrograms(programCollection, m_caseDef.primitiveType, m_caseDef.spacingMode, getWindingUsage(m_caseDef.winding), getPointModeUsage(m_caseDef.usePointMode), mirrorCoords);
	}

	TestInstance* createInstance (Context& context) const
	{
		return new SymmetricOuterEdgeTestInstance(context, m_caseDef);
	};

private:
	const CaseDefinition m_caseDef;
};

tcu::TestCase* makeOuterEdgeDivisionTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode)
{
	const CaseDefinition caseDef = { primitiveType, spacingMode, WINDING_LAST, false };  // winding is ignored by this test
	return new OuterEdgeDivisionTest(testCtx, name, description, caseDef);
}

tcu::TestCase* makeOuterEdgeIndexIndependenceTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const Winding winding, const bool usePointMode)
{
	const CaseDefinition caseDef = { primitiveType, spacingMode, winding, usePointMode };
	return new OuterEdgeIndexIndependenceTest(testCtx, name, description, caseDef);
}

tcu::TestCase* makeSymmetricOuterEdgeTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const Winding winding, const bool usePointMode)
{
	const CaseDefinition caseDef = { primitiveType, spacingMode, winding, usePointMode };
	return new SymmetricOuterEdgeTest(testCtx, name, description, caseDef);
}

} // InvariantOuterEdge ns

namespace PrimitiveSetInvariance
{

enum CaseType
{
	CASETYPE_INVARIANT_PRIMITIVE_SET,
	CASETYPE_INVARIANT_TRIANGLE_SET,
	CASETYPE_INVARIANT_OUTER_TRIANGLE_SET,
	CASETYPE_INVARIANT_INNER_TRIANGLE_SET,
};

struct CaseDefinition
{
	CaseType				caseType;
	TessPrimitiveType		primitiveType;
	SpacingMode				spacingMode;
	WindingUsage			windingUsage;
	bool					usePointMode;
};

struct LevelCase
{
	std::vector<TessLevels>	levels;
	int						mem; //!< Subclass-defined arbitrary piece of data, for type of the levelcase, if needed.

	LevelCase (const TessLevels& lev) : levels(std::vector<TessLevels>(1, lev)), mem(0) {}
	LevelCase (void) : mem(0) {}
};

typedef tcu::Vector<tcu::Vec3, 3> Triangle;

inline Triangle makeTriangle (const PerPrimitive& primitive)
{
	return Triangle(primitive.tessCoord[0].swizzle(0, 1, 2),
					primitive.tessCoord[1].swizzle(0, 1, 2),
					primitive.tessCoord[2].swizzle(0, 1, 2));
}

//! Compare triangle sets, ignoring triangle order and vertex order within triangle, and possibly exclude some triangles too.
template <typename IsTriangleRelevantT>
bool compareTriangleSets (const PerPrimitiveVec&		primitivesA,
						  const PerPrimitiveVec&		primitivesB,
						  tcu::TestLog&					log,
						  const IsTriangleRelevantT&	isTriangleRelevant,
						  const char*					ignoredTriangleDescription = DE_NULL)
{
	typedef LexCompare<Triangle, 3, VecLexLessThan<3> >		TriangleLexLessThan;
	typedef std::set<Triangle, TriangleLexLessThan>			TriangleSet;

	const int		numTrianglesA = static_cast<int>(primitivesA.size());
	const int		numTrianglesB = static_cast<int>(primitivesB.size());
	TriangleSet		trianglesA;
	TriangleSet		trianglesB;

	for (int aOrB = 0; aOrB < 2; ++aOrB)
	{
		const PerPrimitiveVec& primitives	= aOrB == 0 ? primitivesA	: primitivesB;
		const int			   numTriangles	= aOrB == 0 ? numTrianglesA	: numTrianglesB;
		TriangleSet&		   triangles	= aOrB == 0 ? trianglesA	: trianglesB;

		for (int triNdx = 0; triNdx < numTriangles; ++triNdx)
		{
			Triangle triangle = makeTriangle(primitives[triNdx]);

			if (isTriangleRelevant(triangle.getPtr()))
			{
				std::sort(triangle.getPtr(), triangle.getPtr()+3, VecLexLessThan<3>());
				triangles.insert(triangle);
			}
		}
	}
	{
		TriangleSet::const_iterator aIt = trianglesA.begin();
		TriangleSet::const_iterator bIt = trianglesB.begin();

		while (aIt != trianglesA.end() || bIt != trianglesB.end())
		{
			const bool aEnd = aIt == trianglesA.end();
			const bool bEnd = bIt == trianglesB.end();

			if (aEnd || bEnd || *aIt != *bIt)
			{
				log << tcu::TestLog::Message << "Failure: triangle sets in two cases are not equal (when ignoring triangle and vertex order"
					<< (ignoredTriangleDescription == DE_NULL ? "" : std::string() + ", and " + ignoredTriangleDescription) << ")" << tcu::TestLog::EndMessage;

				if (!aEnd && (bEnd || TriangleLexLessThan()(*aIt, *bIt)))
					log << tcu::TestLog::Message << "Note: e.g. triangle " << *aIt << " exists for first case but not for second" << tcu::TestLog::EndMessage;
				else
					log << tcu::TestLog::Message << "Note: e.g. triangle " << *bIt << " exists for second case but not for first" << tcu::TestLog::EndMessage;

				return false;
			}

			++aIt;
			++bIt;
		}

		return true;
	}
}

template <typename ArgT, bool res>
struct ConstantUnaryPredicate
{
	bool operator() (const ArgT&) const { return res; }
};

bool compareTriangleSets (const PerPrimitiveVec& primitivesA, const PerPrimitiveVec& primitivesB, tcu::TestLog& log)
{
	return compareTriangleSets(primitivesA, primitivesB, log, ConstantUnaryPredicate<const tcu::Vec3*, true>());
}

//! Compare two sets of primitives. Order of primitives in each set is undefined, but within each primitive
//! vertex order and coordinates are expected to match exactly.
bool comparePrimitivesExact (const PerPrimitive* const primitivesA, const PerPrimitive* const primitivesB, const int numPrimitivesPerPatch)
{
	int ndxB = 0;
	for (int ndxA = 0; ndxA < numPrimitivesPerPatch; ++ndxA)
	{
		const tcu::Vec4 (&coordsA)[3] = primitivesA[ndxA].tessCoord;
		bool match = false;

		// Actually both sets are usually somewhat sorted, so don't reset ndxB after each match. Instead, continue from the next index.
		for (int i = 0; i < numPrimitivesPerPatch; ++i)
		{
			const tcu::Vec4 (&coordsB)[3] = primitivesB[ndxB].tessCoord;
			ndxB = (ndxB + 1) % numPrimitivesPerPatch;

			if (coordsA[0] == coordsB[0] && coordsA[1] == coordsB[1] && coordsA[2] == coordsB[2])
			{
				match = true;
				break;
			}
		}

		if (!match)
			return false;
	}
	return true;
}

/*--------------------------------------------------------------------*//*!
 * \brief Base class for testing invariance of entire primitive set
 *
 * Draws two patches with identical tessellation levels and compares the
 * results. Repeats the same with other programs that are only different
 * in irrelevant ways; compares the results between these two programs.
 * Also potentially compares to results produced by different tessellation
 * levels (see e.g. invariance rule #6).
 * Furthermore, repeats the above with multiple different tessellation
 * value sets.
 *
 * The manner of primitive set comparison is defined by subclass. E.g.
 * case for invariance rule #1 tests that same vertices come out, in same
 * order; rule #5 only requires that the same triangles are output, but
 * not necessarily in the same order.
 *//*--------------------------------------------------------------------*/
class InvarianceTestCase : public TestCase
{
public:
									InvarianceTestCase			(tcu::TestContext& context, const std::string& name, const std::string& description, const CaseDefinition& caseDef)
										: TestCase	(context, name, description)
										, m_caseDef	(caseDef) {}

	virtual							~InvarianceTestCase			(void) {}

	void							initPrograms				(SourceCollections& programCollection) const;
	TestInstance*					createInstance				(Context& context) const;

private:
	const CaseDefinition			m_caseDef;
};

void InvarianceTestCase::initPrograms (SourceCollections& programCollection) const
{
	addDefaultPrograms(programCollection, m_caseDef.primitiveType, m_caseDef.spacingMode, m_caseDef.windingUsage, getPointModeUsage(m_caseDef.usePointMode));
}

class InvarianceTestInstance : public TestInstance
{
public:
									InvarianceTestInstance		(Context& context, const CaseDefinition& caseDef) : TestInstance(context), m_caseDef(caseDef) {}
	virtual							~InvarianceTestInstance		(void) {}

	tcu::TestStatus					iterate						(void);

protected:
	virtual std::vector<LevelCase>	genTessLevelCases			(void) const;
	virtual bool					compare						(const PerPrimitiveVec& primitivesA, const PerPrimitiveVec& primitivesB, const int levelCaseMem) const = 0;

	const CaseDefinition			m_caseDef;
};

std::vector<LevelCase> InvarianceTestInstance::genTessLevelCases (void) const
{
	static const TessLevels basicTessLevelCases[] =
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

	std::vector<LevelCase> result;
	for (int i = 0; i < DE_LENGTH_OF_ARRAY(basicTessLevelCases); ++i)
		result.push_back(LevelCase(basicTessLevelCases[i]));

	{
		de::Random rnd(123);
		for (int i = 0; i < 10; ++i)
		{
			TessLevels levels;
			for (int j = 0; j < DE_LENGTH_OF_ARRAY(levels.inner); ++j)
				levels.inner[j] = rnd.getFloat(1.0f, 16.0f);
			for (int j = 0; j < DE_LENGTH_OF_ARRAY(levels.outer); ++j)
				levels.outer[j] = rnd.getFloat(1.0f, 16.0f);
			result.push_back(LevelCase(levels));
		}
	}

	return result;
}

tcu::TestStatus InvarianceTestInstance::iterate (void)
{
	requireFeatures(m_context.getInstanceInterface(), m_context.getPhysicalDevice(),
					FEATURE_TESSELLATION_SHADER | FEATURE_GEOMETRY_SHADER | FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS);

	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const std::vector<LevelCase>	tessLevelCases				= genTessLevelCases();
	const int						numPatchesPerDrawCall		= 2;
	int								maxNumPrimitivesPerPatch	= 0;  // computed below
	std::vector<std::vector<int> >	primitiveCounts;

	for (int caseNdx = 0; caseNdx < static_cast<int>(tessLevelCases.size()); ++caseNdx)
	{
		primitiveCounts.push_back(std::vector<int>());
		for (int levelNdx = 0; levelNdx < static_cast<int>(tessLevelCases[caseNdx].levels.size()); ++levelNdx)
		{
			const int primitiveCount = referencePrimitiveCount(m_caseDef.primitiveType, m_caseDef.spacingMode, m_caseDef.usePointMode,
															   &tessLevelCases[caseNdx].levels[levelNdx].inner[0], &tessLevelCases[caseNdx].levels[levelNdx].outer[0]);
			primitiveCounts.back().push_back(primitiveCount);
			maxNumPrimitivesPerPatch = de::max(maxNumPrimitivesPerPatch, primitiveCount);
		}
	}

	// Vertex input attributes buffer: to pass tessellation levels

	const VkFormat     vertexFormat        = VK_FORMAT_R32_SFLOAT;
	const deUint32     vertexStride        = tcu::getPixelSize(mapVkFormat(vertexFormat));
	const VkDeviceSize vertexDataSizeBytes = NUM_TESS_LEVELS * numPatchesPerDrawCall * vertexStride;
	const Buffer       vertexBuffer        (vk, device, allocator, makeBufferCreateInfo(vertexDataSizeBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Output buffer: number of primitives and an array of PerPrimitive structures

	const int		   resultBufferMaxVertices		= numPatchesPerDrawCall * maxNumPrimitivesPerPatch * numVerticesPerPrimitive(m_caseDef.primitiveType, m_caseDef.usePointMode);
	const int		   resultBufferTessCoordsOffset = (int)sizeof(deInt32) * 4;
	const VkDeviceSize resultBufferSizeBytes        = resultBufferTessCoordsOffset + resultBufferMaxVertices * sizeof(PerPrimitive);
	const Buffer       resultBuffer                 (vk, device, allocator, makeBufferCreateInfo(resultBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Descriptors

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet    (makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
	const VkDescriptorBufferInfo  resultBufferInfo = makeDescriptorBufferInfo(resultBuffer.get(), 0ull, resultBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultBufferInfo)
		.update(vk, device);

	const Unique<VkRenderPass>     renderPass    (makeRenderPassWithoutAttachments (vk, device));
	const Unique<VkFramebuffer>    framebuffer   (makeFramebufferWithoutAttachments(vk, device, *renderPass));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout               (vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>    cmdPool       (makeCommandPool                  (vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>  cmdBuffer     (allocateCommandBuffer            (vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < static_cast<int>(tessLevelCases.size()); ++tessLevelCaseNdx)
	{
		const LevelCase& levelCase = tessLevelCases[tessLevelCaseNdx];
		PerPrimitiveVec  firstPrim;

		{
			tcu::TestLog& log = m_context.getTestContext().getLog();
			std::ostringstream tessLevelsStr;

			for (int i = 0; i < static_cast<int>(levelCase.levels.size()); ++i)
				tessLevelsStr << (levelCase.levels.size() > 1u ? "\n" : "") << getTessellationLevelsString(levelCase.levels[i], m_caseDef.primitiveType);

			log << tcu::TestLog::Message << "Tessellation level sets: " << tessLevelsStr.str() << tcu::TestLog::EndMessage;
		}

		for (int subTessLevelCaseNdx = 0; subTessLevelCaseNdx < static_cast<int>(levelCase.levels.size()); ++subTessLevelCaseNdx)
		{
			const TessLevels& tessLevels = levelCase.levels[subTessLevelCaseNdx];
			{
				TessLevels data[2];
				data[0] = tessLevels;
				data[1] = tessLevels;

				const Allocation& alloc = vertexBuffer.getAllocation();
				deMemcpy(alloc.getHostPtr(), data, sizeof(data));
				flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), sizeof(data));
			}

			int programNdx = 0;
			const std::vector<Winding> windingCases = getWindingCases(m_caseDef.windingUsage);
			for (std::vector<Winding>::const_iterator windingIter = windingCases.begin(); windingIter != windingCases.end(); ++windingIter)
			{
				const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
					.setPatchControlPoints        (NUM_TESS_LEVELS)
					.setVertexInputSingleAttribute(vertexFormat, vertexStride)
					.setShader                    (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					m_context.getBinaryCollection().get("vert"), DE_NULL)
					.setShader                    (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	m_context.getBinaryCollection().get("tesc"), DE_NULL)
					.setShader                    (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_context.getBinaryCollection().get(getProgramName("tese", *windingIter, m_caseDef.usePointMode)), DE_NULL)
					.setShader                    (vk, device, VK_SHADER_STAGE_GEOMETRY_BIT,                m_context.getBinaryCollection().get(getProgramName("geom", m_caseDef.usePointMode)), DE_NULL)
					.build                        (vk, device, *pipelineLayout, *renderPass));

				{
					const Allocation& alloc = resultBuffer.getAllocation();
					deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(resultBufferSizeBytes));
					flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), resultBufferSizeBytes);
				}

				beginCommandBuffer(vk, *cmdBuffer);
				beginRenderPassWithRasterizationDisabled(vk, *cmdBuffer, *renderPass, *framebuffer);

				vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
				vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
				{
					const VkDeviceSize vertexBufferOffset = 0ull;
					vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
				}

				vk.cmdDraw(*cmdBuffer, numPatchesPerDrawCall * NUM_TESS_LEVELS, 1u, 0u, 0u);
				endRenderPass(vk, *cmdBuffer);

				{
					const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(
						VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *resultBuffer, 0ull, resultBufferSizeBytes);

					vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
						0u, DE_NULL, 1u, &shaderWriteBarrier, 0u, DE_NULL);
				}

				endCommandBuffer(vk, *cmdBuffer);
				submitCommandsAndWait(vk, device, queue, *cmdBuffer);

				// Verify case result
				{
					const Allocation& resultAlloc = resultBuffer.getAllocation();
					invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), resultBufferSizeBytes);

					const int				refNumPrimitives     = numPatchesPerDrawCall * primitiveCounts[tessLevelCaseNdx][subTessLevelCaseNdx];
					const int				numPrimitiveVertices = numVerticesPerPrimitive(m_caseDef.primitiveType, m_caseDef.usePointMode);
					const deInt32			numPrimitives        = *static_cast<deInt32*>(resultAlloc.getHostPtr());
					const PerPrimitiveVec	primitives           = sorted(readInterleavedData<PerPrimitive>(numPrimitives, resultAlloc.getHostPtr(), resultBufferTessCoordsOffset, sizeof(PerPrimitive)),
																		  byPatchPrimitiveID);

					// If this fails then we didn't read all vertices from shader and test must be changed to allow more.
					DE_ASSERT(numPrimitiveVertices * numPrimitives <= resultBufferMaxVertices);
					DE_UNREF(numPrimitiveVertices);

					tcu::TestLog& log = m_context.getTestContext().getLog();

					if (numPrimitives != refNumPrimitives)
					{
						log << tcu::TestLog::Message << "Failure: got " << numPrimitives << " primitives, but expected " << refNumPrimitives << tcu::TestLog::EndMessage;

						return tcu::TestStatus::fail("Invalid set of primitives");
					}

					const int					half  = static_cast<int>(primitives.size() / 2);
					const PerPrimitiveVec		prim0 = PerPrimitiveVec(primitives.begin(), primitives.begin() + half);
					const PerPrimitive* const	prim1 = &primitives[half];

					if (!comparePrimitivesExact(&prim0[0], prim1, half))
					{
							log << tcu::TestLog::Message << "Failure: tessellation coordinates differ between two primitives drawn in one draw call" << tcu::TestLog::EndMessage
								<< tcu::TestLog::Message << "Note: tessellation levels for both primitives were: " << getTessellationLevelsString(tessLevels, m_caseDef.primitiveType) << tcu::TestLog::EndMessage;

							return tcu::TestStatus::fail("Invalid set of primitives");
					}

					if (programNdx == 0 && subTessLevelCaseNdx == 0)
						firstPrim = prim0;
					else
					{
						const bool compareOk = compare(firstPrim, prim0, levelCase.mem);
						if (!compareOk)
						{
							log << tcu::TestLog::Message
								<< "Note: comparison of tessellation coordinates failed; comparison was made between following cases:\n"
								<< "  - case A: program 0, tessellation levels: " << getTessellationLevelsString(tessLevelCases[tessLevelCaseNdx].levels[0], m_caseDef.primitiveType) << "\n"
								<< "  - case B: program " << programNdx << ", tessellation levels: " << getTessellationLevelsString(tessLevels, m_caseDef.primitiveType)
								<< tcu::TestLog::EndMessage;

							return tcu::TestStatus::fail("Invalid set of primitives");
						}
					}
				}
				++programNdx;
			}
		}
	}
	return tcu::TestStatus::pass("OK");
}

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #1
 *
 * Test that the sequence of primitives input to the TES only depends on
 * the tessellation levels, tessellation mode, spacing mode, winding, and
 * point mode.
 *//*--------------------------------------------------------------------*/
class InvariantPrimitiveSetTestInstance : public InvarianceTestInstance
{
public:
	InvariantPrimitiveSetTestInstance (Context& context, const CaseDefinition& caseDef) : InvarianceTestInstance(context, caseDef) {}

protected:
	bool compare (const PerPrimitiveVec& primitivesA, const PerPrimitiveVec& primitivesB, const int) const
	{
		if (!comparePrimitivesExact(&primitivesA[0], &primitivesB[0], static_cast<int>(primitivesA.size())))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Failure: tessellation coordinates differ between two programs" << tcu::TestLog::EndMessage;

			return false;
		}
		return true;
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #5
 *
 * Test that the set of triangles input to the TES only depends on the
 * tessellation levels, tessellation mode and spacing mode. Specifically,
 * winding doesn't change the set of triangles, though it can change the
 * order in which they are input to TES, and can (and will) change the
 * vertex order within a triangle.
 *//*--------------------------------------------------------------------*/
class InvariantTriangleSetTestInstance : public InvarianceTestInstance
{
public:
	InvariantTriangleSetTestInstance (Context& context, const CaseDefinition& caseDef) : InvarianceTestInstance(context, caseDef) {}

protected:
	bool compare (const PerPrimitiveVec& primitivesA, const PerPrimitiveVec& primitivesB, const int) const
	{
		return compareTriangleSets(primitivesA, primitivesB, m_context.getTestContext().getLog());
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #6
 *
 * Test that the set of inner triangles input to the TES only depends on
 * the inner tessellation levels, tessellation mode and spacing mode.
 *//*--------------------------------------------------------------------*/
class InvariantInnerTriangleSetTestInstance : public InvarianceTestInstance
{
public:
	InvariantInnerTriangleSetTestInstance (Context& context, const CaseDefinition& caseDef) : InvarianceTestInstance(context, caseDef) {}

protected:
	std::vector<LevelCase> genTessLevelCases (void) const
	{
		const int						numSubCases		= 4;
		const std::vector<LevelCase>	baseResults		= InvarianceTestInstance::genTessLevelCases();
		std::vector<LevelCase>			result;
		de::Random						rnd				(123);

		// Generate variants with different values for irrelevant levels.
		for (int baseNdx = 0; baseNdx < static_cast<int>(baseResults.size()); ++baseNdx)
		{
			const TessLevels&	base	= baseResults[baseNdx].levels[0];
			TessLevels			levels	= base;
			LevelCase			levelCase;

			for (int subNdx = 0; subNdx < numSubCases; ++subNdx)
			{
				levelCase.levels.push_back(levels);

				for (int i = 0; i < DE_LENGTH_OF_ARRAY(levels.outer); ++i)
					levels.outer[i] = rnd.getFloat(2.0f, 16.0f);
				if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
					levels.inner[1] = rnd.getFloat(2.0f, 16.0f);
			}

			result.push_back(levelCase);
		}

		return result;
	}

	struct IsInnerTriangleTriangle
	{
		bool operator() (const tcu::Vec3* vertices) const
		{
			for (int v = 0; v < 3; ++v)
				for (int c = 0; c < 3; ++c)
					if (vertices[v][c] == 0.0f)
						return false;
			return true;
		}
	};

	struct IsInnerQuadTriangle
	{
		bool operator() (const tcu::Vec3* vertices) const
		{
			for (int v = 0; v < 3; ++v)
				for (int c = 0; c < 2; ++c)
					if (vertices[v][c] == 0.0f || vertices[v][c] == 1.0f)
						return false;
			return true;
		}
	};

	bool compare (const PerPrimitiveVec& primitivesA, const PerPrimitiveVec& primitivesB, const int) const
	{
		if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
			return compareTriangleSets(primitivesA, primitivesB, m_context.getTestContext().getLog(), IsInnerTriangleTriangle(), "outer triangles");
		else if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS)
			return compareTriangleSets(primitivesA, primitivesB, m_context.getTestContext().getLog(), IsInnerQuadTriangle(), "outer triangles");
		else
		{
			DE_ASSERT(false);
			return false;
		}
	}
};

/*--------------------------------------------------------------------*//*!
 * \brief Test invariance rule #7
 *
 * Test that the set of outer triangles input to the TES only depends on
 * tessellation mode, spacing mode and the inner and outer tessellation
 * levels corresponding to the inner and outer edges relevant to that
 * triangle.
 *//*--------------------------------------------------------------------*/
class InvariantOuterTriangleSetTestInstance : public InvarianceTestInstance
{
public:
	InvariantOuterTriangleSetTestInstance (Context& context, const CaseDefinition& caseDef) : InvarianceTestInstance(context, caseDef) {}

protected:
	std::vector<LevelCase> genTessLevelCases (void) const
	{
		const int						numSubCasesPerEdge	= 4;
		const int						numEdges			= m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES	? 3
															: m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS		? 4 : 0;
		const std::vector<LevelCase>	baseResult			= InvarianceTestInstance::genTessLevelCases();
		std::vector<LevelCase>			result;
		de::Random						rnd					(123);

		// Generate variants with different values for irrelevant levels.
		for (int baseNdx = 0; baseNdx < static_cast<int>(baseResult.size()); ++baseNdx)
		{
			const TessLevels& base = baseResult[baseNdx].levels[0];
			if (base.inner[0] == 1.0f || (m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS && base.inner[1] == 1.0f))
				continue;

			for (int edgeNdx = 0; edgeNdx < numEdges; ++edgeNdx)
			{
				TessLevels	levels = base;
				LevelCase	levelCase;
				levelCase.mem = edgeNdx;

				for (int subCaseNdx = 0; subCaseNdx < numSubCasesPerEdge; ++subCaseNdx)
				{
					levelCase.levels.push_back(levels);

					for (int i = 0; i < DE_LENGTH_OF_ARRAY(levels.outer); ++i)
					{
						if (i != edgeNdx)
							levels.outer[i] = rnd.getFloat(2.0f, 16.0f);
					}

					if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
						levels.inner[1] = rnd.getFloat(2.0f, 16.0f);
				}

				result.push_back(levelCase);
			}
		}

		return result;
	}

	class IsTriangleTriangleOnOuterEdge
	{
	public:
		IsTriangleTriangleOnOuterEdge (int edgeNdx) : m_edgeNdx(edgeNdx) {}
		bool operator() (const tcu::Vec3* vertices) const
		{
			bool touchesAppropriateEdge = false;
			for (int v = 0; v < 3; ++v)
				if (vertices[v][m_edgeNdx] == 0.0f)
					touchesAppropriateEdge = true;

			if (touchesAppropriateEdge)
			{
				const tcu::Vec3 avg = (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
				return avg[m_edgeNdx] < avg[(m_edgeNdx+1)%3] &&
					   avg[m_edgeNdx] < avg[(m_edgeNdx+2)%3];
			}
			return false;
		}

	private:
		const int m_edgeNdx;
	};

	class IsQuadTriangleOnOuterEdge
	{
	public:
		IsQuadTriangleOnOuterEdge (int edgeNdx) : m_edgeNdx(edgeNdx) {}

		bool onEdge (const tcu::Vec3& v) const
		{
			return v[m_edgeNdx%2] == (m_edgeNdx <= 1 ? 0.0f : 1.0f);
		}

		static inline bool onAnyEdge (const tcu::Vec3& v)
		{
			return v[0] == 0.0f || v[0] == 1.0f || v[1] == 0.0f || v[1] == 1.0f;
		}

		bool operator() (const tcu::Vec3* vertices) const
		{
			for (int v = 0; v < 3; ++v)
			{
				const tcu::Vec3& a = vertices[v];
				const tcu::Vec3& b = vertices[(v+1)%3];
				const tcu::Vec3& c = vertices[(v+2)%3];
				if (onEdge(a) && onEdge(b))
					return true;
				if (onEdge(c) && !onAnyEdge(a) && !onAnyEdge(b) && a[m_edgeNdx%2] == b[m_edgeNdx%2])
					return true;
			}

			return false;
		}

	private:
		const int m_edgeNdx;
	};

	bool compare (const PerPrimitiveVec& primitivesA, const PerPrimitiveVec& primitivesB, const int outerEdgeNdx) const
	{
		if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
		{
			return compareTriangleSets(primitivesA, primitivesB, m_context.getTestContext().getLog(),
									   IsTriangleTriangleOnOuterEdge(outerEdgeNdx),
									   ("inner triangles, and outer triangles corresponding to other edge than edge "
										+ outerEdgeDescriptions(m_caseDef.primitiveType)[outerEdgeNdx].description()).c_str());
		}
		else if (m_caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS)
		{
			return compareTriangleSets(primitivesA, primitivesB, m_context.getTestContext().getLog(),
									   IsQuadTriangleOnOuterEdge(outerEdgeNdx),
									   ("inner triangles, and outer triangles corresponding to other edge than edge "
										+ outerEdgeDescriptions(m_caseDef.primitiveType)[outerEdgeNdx].description()).c_str());
		}
		else
			DE_ASSERT(false);

		return true;
	}
};

TestInstance* InvarianceTestCase::createInstance (Context& context) const
{
	switch (m_caseDef.caseType)
	{
		case CASETYPE_INVARIANT_PRIMITIVE_SET:			return new InvariantPrimitiveSetTestInstance    (context, m_caseDef);
		case CASETYPE_INVARIANT_TRIANGLE_SET:			return new InvariantTriangleSetTestInstance     (context, m_caseDef);
		case CASETYPE_INVARIANT_OUTER_TRIANGLE_SET:		return new InvariantOuterTriangleSetTestInstance(context, m_caseDef);
		case CASETYPE_INVARIANT_INNER_TRIANGLE_SET:		return new InvariantInnerTriangleSetTestInstance(context, m_caseDef);
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

TestCase* makeInvariantPrimitiveSetTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const Winding winding, const bool usePointMode)
{
	const CaseDefinition caseDef = { CASETYPE_INVARIANT_PRIMITIVE_SET, primitiveType, spacingMode, getWindingUsage(winding), usePointMode };
	return new InvarianceTestCase(testCtx, name, description, caseDef);
}

TestCase* makeInvariantTriangleSetTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode)
{
	DE_ASSERT(primitiveType == TESSPRIMITIVETYPE_TRIANGLES || primitiveType == TESSPRIMITIVETYPE_QUADS);
	const CaseDefinition caseDef = { CASETYPE_INVARIANT_TRIANGLE_SET, primitiveType, spacingMode, WINDING_USAGE_VARY, false };
	return new InvarianceTestCase(testCtx, name, description, caseDef);
}

TestCase* makeInvariantInnerTriangleSetTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode)
{
	DE_ASSERT(primitiveType == TESSPRIMITIVETYPE_TRIANGLES || primitiveType == TESSPRIMITIVETYPE_QUADS);
	const CaseDefinition caseDef = { CASETYPE_INVARIANT_INNER_TRIANGLE_SET, primitiveType, spacingMode, WINDING_USAGE_VARY, false };
	return new InvarianceTestCase(testCtx, name, description, caseDef);
}

TestCase* makeInvariantOuterTriangleSetTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode)
{
	DE_ASSERT(primitiveType == TESSPRIMITIVETYPE_TRIANGLES || primitiveType == TESSPRIMITIVETYPE_QUADS);
	const CaseDefinition caseDef = { CASETYPE_INVARIANT_OUTER_TRIANGLE_SET, primitiveType, spacingMode, WINDING_USAGE_VARY, false };
	return new InvarianceTestCase(testCtx, name, description, caseDef);
}

} // PrimitiveSetInvariance ns

namespace TessCoordComponent
{

enum CaseType
{
	CASETYPE_TESS_COORD_RANGE = 0,		//!< Test that all (relevant) components of tess coord are in [0,1].
	CASETYPE_ONE_MINUS_TESS_COORD,		//!< Test that for every (relevant) component c of a tess coord, 1.0-c is exact.

	CASETYPE_LAST
};

struct CaseDefinition
{
	CaseType			caseType;
	TessPrimitiveType	primitiveType;
	SpacingMode			spacingMode;
	Winding				winding;
	bool				usePointMode;
};

std::vector<TessLevels> genTessLevelCases (const int numCases)
{
	de::Random				rnd(123);
	std::vector<TessLevels>	result;

	for (int i = 0; i < numCases; ++i)
	{
		TessLevels levels;
		levels.inner[0] = rnd.getFloat(1.0f, 63.0f);
		levels.inner[1] = rnd.getFloat(1.0f, 63.0f);
		levels.outer[0] = rnd.getFloat(1.0f, 63.0f);
		levels.outer[1] = rnd.getFloat(1.0f, 63.0f);
		levels.outer[2] = rnd.getFloat(1.0f, 63.0f);
		levels.outer[3] = rnd.getFloat(1.0f, 63.0f);
		result.push_back(levels);
	}

	return result;
}

typedef bool (*CompareFunc)(tcu::TestLog& log, const float value);

bool compareTessCoordRange (tcu::TestLog& log, const float value)
{
	if (!de::inRange(value, 0.0f, 1.0f))
	{
		log << tcu::TestLog::Message << "Failure: tess coord component isn't in range [0,1]" << tcu::TestLog::EndMessage;
		return false;
	}
	return true;
}

bool compareOneMinusTessCoord (tcu::TestLog& log, const float value)
{
	if (value != 1.0f)
	{
		log << tcu::TestLog::Message << "Failure: comp + (1.0-comp) doesn't equal 1.0 for some component of tessellation coordinate" << tcu::TestLog::EndMessage;
		return false;
	}
	return true;
}

void initPrograms (vk::SourceCollections& programCollection, const CaseDefinition caseDef)
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp float in_v_attr;\n"
			<< "layout(location = 0) out highp float in_tc_attr;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    in_tc_attr = in_v_attr;\n"
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
			<< "layout(location = 0) in highp float in_tc_attr[];\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_TessLevelInner[0] = in_tc_attr[0];\n"
			<< "    gl_TessLevelInner[1] = in_tc_attr[1];\n"
			<< "\n"
			<< "    gl_TessLevelOuter[0] = in_tc_attr[2];\n"
			<< "    gl_TessLevelOuter[1] = in_tc_attr[3];\n"
			<< "    gl_TessLevelOuter[2] = in_tc_attr[4];\n"
			<< "    gl_TessLevelOuter[3] = in_tc_attr[5];\n"
			<< "}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Tessellation evaluation shader
	{
		std::ostringstream tessCoordSrc;

		if (caseDef.caseType == CASETYPE_TESS_COORD_RANGE)
			tessCoordSrc << "    sb_out.tessCoord[index] = gl_TessCoord;\n";
		else if (caseDef.caseType == CASETYPE_ONE_MINUS_TESS_COORD)
		{
			const char* components[]  = { "x" , "y", "z" };
			const int   numComponents = (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 2);

			for (int i = 0; i < numComponents; ++i)
				tessCoordSrc << "    {\n"
							 << "        float oneMinusComp        = 1.0 - gl_TessCoord." << components[i] << ";\n"
							 << "        sb_out.tessCoord[index]." << components[i] << " = gl_TessCoord." << components[i] << " + oneMinusComp;\n"
							 << "    }\n";
		}
		else
		{
			DE_ASSERT(false);
			return;
		}

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(" << getTessPrimitiveTypeShaderName(caseDef.primitiveType) << ", "
						 << getSpacingModeShaderName(caseDef.spacingMode) << ", "
						 << getWindingShaderName(caseDef.winding)
						 << (caseDef.usePointMode ? ", point_mode" : "") << ") in;\n"
			<< "\n"
			<< "layout(set = 0, binding = 0, std430) coherent restrict buffer Output {\n"
			<< "    int  numInvocations;\n"
			<< "    vec3 tessCoord[];\n"
			<< "} sb_out;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    int index = atomicAdd(sb_out.numInvocations, 1);\n"
			<< tessCoordSrc.str()
			<< "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}
}

tcu::TestStatus test (Context& context, const CaseDefinition caseDef)
{
	requireFeatures(context.getInstanceInterface(), context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER | FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS);

	const DeviceInterface&	vk					= context.getDeviceInterface();
	const VkDevice			device				= context.getDevice();
	const VkQueue			queue				= context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= context.getDefaultAllocator();

	const int						numTessLevelCases	= 32;
	const std::vector<TessLevels>	tessLevelCases		= genTessLevelCases(numTessLevelCases);

	int maxNumVerticesInDrawCall = 0;
	for (int i = 0; i < numTessLevelCases; ++i)
		maxNumVerticesInDrawCall = de::max(maxNumVerticesInDrawCall, referenceVertexCount(caseDef.primitiveType, caseDef.spacingMode, caseDef.usePointMode,
										   &tessLevelCases[i].inner[0], &tessLevelCases[i].outer[0]));

	// We may get more invocations than expected, so add some more space (arbitrary number).
	maxNumVerticesInDrawCall += 4;

	// Vertex input attributes buffer: to pass tessellation levels

	const VkFormat		vertexFormat        = VK_FORMAT_R32_SFLOAT;
	const deUint32		vertexStride        = tcu::getPixelSize(mapVkFormat(vertexFormat));
	const VkDeviceSize	vertexDataSizeBytes = NUM_TESS_LEVELS * vertexStride;
	const Buffer		vertexBuffer        (vk, device, allocator, makeBufferCreateInfo(vertexDataSizeBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible);

	DE_ASSERT(vertexDataSizeBytes == sizeof(TessLevels));

	// Output buffer: number of invocations and array of tess coords

	const int		   resultBufferTessCoordsOffset = (int)sizeof(deInt32) * 4;
	const VkDeviceSize resultBufferSizeBytes        = resultBufferTessCoordsOffset + maxNumVerticesInDrawCall * sizeof(tcu::Vec4);
	const Buffer       resultBuffer                 (vk, device, allocator, makeBufferCreateInfo(resultBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Descriptors

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet    (makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
	const VkDescriptorBufferInfo  resultBufferInfo = makeDescriptorBufferInfo(resultBuffer.get(), 0ull, resultBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultBufferInfo)
		.update(vk, device);

	const Unique<VkRenderPass>     renderPass    (makeRenderPassWithoutAttachments (vk, device));
	const Unique<VkFramebuffer>    framebuffer   (makeFramebufferWithoutAttachments(vk, device, *renderPass));
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout               (vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>    cmdPool       (makeCommandPool                  (vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>  cmdBuffer     (allocateCommandBuffer            (vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
		.setPatchControlPoints        (NUM_TESS_LEVELS)
		.setVertexInputSingleAttribute(vertexFormat, vertexStride)
		.setShader                    (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader                    (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader                    (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, context.getBinaryCollection().get("tese"), DE_NULL)
		.build                        (vk, device, *pipelineLayout, *renderPass));

	for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < numTessLevelCases; ++tessLevelCaseNdx)
	{
		context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Testing with tessellation levels: " << getTessellationLevelsString(tessLevelCases[tessLevelCaseNdx], caseDef.primitiveType)
			<< tcu::TestLog::EndMessage;

		{
			const Allocation& alloc = vertexBuffer.getAllocation();
			deMemcpy(alloc.getHostPtr(), &tessLevelCases[tessLevelCaseNdx], sizeof(TessLevels));
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), sizeof(TessLevels));
		}
		{
			const Allocation& alloc = resultBuffer.getAllocation();
			deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(resultBufferSizeBytes));
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), resultBufferSizeBytes);
		}

		beginCommandBuffer(vk, *cmdBuffer);
		beginRenderPassWithRasterizationDisabled(vk, *cmdBuffer, *renderPass, *framebuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
		{
			const VkDeviceSize vertexBufferOffset = 0ull;
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
		}

		vk.cmdDraw(*cmdBuffer, NUM_TESS_LEVELS, 1u, 0u, 0u);
		endRenderPass(vk, *cmdBuffer);

		{
			const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *resultBuffer, 0ull, resultBufferSizeBytes);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, 1u, &shaderWriteBarrier, 0u, DE_NULL);
		}

		endCommandBuffer(vk, *cmdBuffer);
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);

		// Verify case result
		{
			const Allocation& resultAlloc = resultBuffer.getAllocation();
			invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), resultBufferSizeBytes);

			const deInt32				 numVertices = *static_cast<deInt32*>(resultAlloc.getHostPtr());
			const std::vector<tcu::Vec3> vertices    = readInterleavedData<tcu::Vec3>(numVertices, resultAlloc.getHostPtr(), resultBufferTessCoordsOffset, sizeof(tcu::Vec4));

			// If this fails then we didn't read all vertices from shader and test must be changed to allow more.
			DE_ASSERT(numVertices <= maxNumVerticesInDrawCall);

			tcu::TestLog& log           = context.getTestContext().getLog();
			const int     numComponents = (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 2);

			CompareFunc compare = (caseDef.caseType == CASETYPE_TESS_COORD_RANGE     ? compareTessCoordRange :
								   caseDef.caseType == CASETYPE_ONE_MINUS_TESS_COORD ? compareOneMinusTessCoord : DE_NULL);

			DE_ASSERT(compare != DE_NULL);

			for (std::vector<tcu::Vec3>::const_iterator vertexIter = vertices.begin(); vertexIter != vertices.end(); ++vertexIter)
			for (int i = 0; i < numComponents; ++i)
				if (!compare(log, (*vertexIter)[i]))
				{
						log << tcu::TestLog::Message << "Note: got a wrong tessellation coordinate "
							<< (numComponents == 3 ? de::toString(*vertexIter) : de::toString(vertexIter->swizzle(0,1))) << tcu::TestLog::EndMessage;

						tcu::TestStatus::fail("Invalid tessellation coordinate component");
				}
		}
	}
	return tcu::TestStatus::pass("OK");
}

tcu::TestCase* makeTessCoordRangeTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const Winding winding, const bool usePointMode)
{
	const CaseDefinition caseDef = { CASETYPE_TESS_COORD_RANGE, primitiveType, spacingMode, winding, usePointMode };
	return createFunctionCaseWithPrograms(testCtx, tcu::NODETYPE_SELF_VALIDATE, name, description, initPrograms, test, caseDef);
}

tcu::TestCase* makeOneMinusTessCoordTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const Winding winding, const bool usePointMode)
{
	const CaseDefinition caseDef = { CASETYPE_ONE_MINUS_TESS_COORD, primitiveType, spacingMode, winding, usePointMode };
	return createFunctionCaseWithPrograms(testCtx, tcu::NODETYPE_SELF_VALIDATE, name, description, initPrograms, test, caseDef);
}

} // TessCoordComponent ns

} // anonymous

//! These tests correspond to dEQP-GLES31.functional.tessellation.invariance.*
//! Original OpenGL ES tests used transform feedback to get vertices in primitive order. To emulate this behavior we have to use geometry shader,
//! which allows us to intercept verticess of final output primitives. This can't be done with tessellation shaders alone as number and order of
//! invocation is undefined.
tcu::TestCaseGroup* createInvarianceTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "invariance", "Test tessellation invariance rules"));

	de::MovePtr<tcu::TestCaseGroup> invariantPrimitiveSetGroup              (new tcu::TestCaseGroup(testCtx, "primitive_set",					"Test invariance rule #1"));
	de::MovePtr<tcu::TestCaseGroup> invariantOuterEdgeGroup					(new tcu::TestCaseGroup(testCtx, "outer_edge_division",				"Test invariance rule #2"));
	de::MovePtr<tcu::TestCaseGroup> symmetricOuterEdgeGroup					(new tcu::TestCaseGroup(testCtx, "outer_edge_symmetry",				"Test invariance rule #3"));
	de::MovePtr<tcu::TestCaseGroup> outerEdgeVertexSetIndexIndependenceGroup(new tcu::TestCaseGroup(testCtx, "outer_edge_index_independence",	"Test invariance rule #4"));
	de::MovePtr<tcu::TestCaseGroup> invariantTriangleSetGroup				(new tcu::TestCaseGroup(testCtx, "triangle_set",					"Test invariance rule #5"));
	de::MovePtr<tcu::TestCaseGroup> invariantInnerTriangleSetGroup			(new tcu::TestCaseGroup(testCtx, "inner_triangle_set",				"Test invariance rule #6"));
	de::MovePtr<tcu::TestCaseGroup> invariantOuterTriangleSetGroup			(new tcu::TestCaseGroup(testCtx, "outer_triangle_set",				"Test invariance rule #7"));
	de::MovePtr<tcu::TestCaseGroup> tessCoordComponentRangeGroup			(new tcu::TestCaseGroup(testCtx, "tess_coord_component_range",		"Test invariance rule #8, first part"));
	de::MovePtr<tcu::TestCaseGroup> oneMinusTessCoordComponentGroup			(new tcu::TestCaseGroup(testCtx, "one_minus_tess_coord_component",	"Test invariance rule #8, second part"));

	for (int primitiveTypeNdx = 0; primitiveTypeNdx < TESSPRIMITIVETYPE_LAST; ++primitiveTypeNdx)
	for (int spacingModeNdx = 0; spacingModeNdx < SPACINGMODE_LAST; ++spacingModeNdx)
	{
		const TessPrimitiveType primitiveType = static_cast<TessPrimitiveType>(primitiveTypeNdx);
		const SpacingMode       spacingMode   = static_cast<SpacingMode>(spacingModeNdx);
		const bool              triOrQuad     = primitiveType == TESSPRIMITIVETYPE_TRIANGLES || primitiveType == TESSPRIMITIVETYPE_QUADS;
		const std::string       primName      = getTessPrimitiveTypeShaderName(primitiveType);
		const std::string       primSpacName  = primName + "_" + getSpacingModeShaderName(spacingMode);

		if (triOrQuad)
		{
			invariantOuterEdgeGroup->addChild       (    InvariantOuterEdge::makeOuterEdgeDivisionTest        (testCtx, primSpacName, "", primitiveType, spacingMode));
			invariantTriangleSetGroup->addChild     (PrimitiveSetInvariance::makeInvariantTriangleSetTest     (testCtx, primSpacName, "", primitiveType, spacingMode));
			invariantInnerTriangleSetGroup->addChild(PrimitiveSetInvariance::makeInvariantInnerTriangleSetTest(testCtx, primSpacName, "", primitiveType, spacingMode));
			invariantOuterTriangleSetGroup->addChild(PrimitiveSetInvariance::makeInvariantOuterTriangleSetTest(testCtx, primSpacName, "", primitiveType, spacingMode));
		}

		for (int windingNdx = 0; windingNdx < WINDING_LAST; ++windingNdx)
		for (int usePointModeNdx = 0; usePointModeNdx <= 1; ++usePointModeNdx)
		{
			const Winding     winding               = static_cast<Winding>(windingNdx);
			const bool        usePointMode          = (usePointModeNdx != 0);
			const std::string primSpacWindPointName = primSpacName + "_" + getWindingShaderName(winding) + (usePointMode ? "_point_mode" : "");

			invariantPrimitiveSetGroup->addChild     (PrimitiveSetInvariance::makeInvariantPrimitiveSetTest(testCtx, primSpacWindPointName, "", primitiveType, spacingMode, winding, usePointMode));
			tessCoordComponentRangeGroup->addChild   (    TessCoordComponent::makeTessCoordRangeTest       (testCtx, primSpacWindPointName, "", primitiveType, spacingMode, winding, usePointMode));
			oneMinusTessCoordComponentGroup->addChild(    TessCoordComponent::makeOneMinusTessCoordTest    (testCtx, primSpacWindPointName, "", primitiveType, spacingMode, winding, usePointMode));
			symmetricOuterEdgeGroup->addChild        (    InvariantOuterEdge::makeSymmetricOuterEdgeTest   (testCtx, primSpacWindPointName, "", primitiveType, spacingMode, winding, usePointMode));

			if (triOrQuad)
				outerEdgeVertexSetIndexIndependenceGroup->addChild(InvariantOuterEdge::makeOuterEdgeIndexIndependenceTest(testCtx, primSpacWindPointName, "", primitiveType, spacingMode, winding, usePointMode));
		}
	}

	group->addChild(invariantPrimitiveSetGroup.release());
	group->addChild(invariantOuterEdgeGroup.release());
	group->addChild(symmetricOuterEdgeGroup.release());
	group->addChild(outerEdgeVertexSetIndexIndependenceGroup.release());
	group->addChild(invariantTriangleSetGroup.release());
	group->addChild(invariantInnerTriangleSetGroup.release());
	group->addChild(invariantOuterTriangleSetGroup.release());
	group->addChild(tessCoordComponentRangeGroup.release());
	group->addChild(oneMinusTessCoordComponentGroup.release());

	return group.release();
}

} // tessellation
} // vkt
