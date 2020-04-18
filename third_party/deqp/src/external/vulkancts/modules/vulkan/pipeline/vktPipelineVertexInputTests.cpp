/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
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
 * \brief Vertex Input Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineVertexInputTests.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktPipelineClearUtil.hpp"
#include "vktPipelineImageUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktPipelineReferenceRenderer.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "tcuFloat.hpp"
#include "tcuImageCompare.hpp"
#include "deFloat16.h"
#include "deMemory.h"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include <sstream>
#include <vector>

namespace vkt
{
namespace pipeline
{

using namespace vk;

namespace
{

bool isSupportedVertexFormat (Context& context, VkFormat format)
{
	if (isVertexFormatDouble(format) && !context.getDeviceFeatures().shaderFloat64)
		return false;

	VkFormatProperties  formatProps;
	deMemset(&formatProps, 0, sizeof(VkFormatProperties));
	context.getInstanceInterface().getPhysicalDeviceFormatProperties(context.getPhysicalDevice(), format, &formatProps);

	return (formatProps.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) != 0u;
}

float getRepresentableDifferenceUnorm (VkFormat format)
{
	DE_ASSERT(isVertexFormatUnorm(format) || isVertexFormatSRGB(format));

	return 1.0f / float((1 << (getVertexFormatComponentSize(format) * 8)) - 1);
}

float getRepresentableDifferenceSnorm (VkFormat format)
{
	DE_ASSERT(isVertexFormatSnorm(format));

	return 1.0f / float((1 << (getVertexFormatComponentSize(format) * 8 - 1)) - 1);
}

deUint32 getNextMultipleOffset (deUint32 divisor, deUint32 value)
{
	if (value % divisor == 0)
		return 0;
	else
		return divisor - (value % divisor);
}

class VertexInputTest : public vkt::TestCase
{
public:
	enum GlslType
	{
		GLSL_TYPE_INT,
		GLSL_TYPE_IVEC2,
		GLSL_TYPE_IVEC3,
		GLSL_TYPE_IVEC4,

		GLSL_TYPE_UINT,
		GLSL_TYPE_UVEC2,
		GLSL_TYPE_UVEC3,
		GLSL_TYPE_UVEC4,

		GLSL_TYPE_FLOAT,
		GLSL_TYPE_VEC2,
		GLSL_TYPE_VEC3,
		GLSL_TYPE_VEC4,
		GLSL_TYPE_MAT2,
		GLSL_TYPE_MAT3,
		GLSL_TYPE_MAT4,

		GLSL_TYPE_DOUBLE,
		GLSL_TYPE_DVEC2,
		GLSL_TYPE_DVEC3,
		GLSL_TYPE_DVEC4,
		GLSL_TYPE_DMAT2,
		GLSL_TYPE_DMAT3,
		GLSL_TYPE_DMAT4,

		GLSL_TYPE_COUNT
	};

	enum GlslBasicType
	{
		GLSL_BASIC_TYPE_INT,
		GLSL_BASIC_TYPE_UINT,
		GLSL_BASIC_TYPE_FLOAT,
		GLSL_BASIC_TYPE_DOUBLE
	};

	enum BindingMapping
	{
		BINDING_MAPPING_ONE_TO_ONE,		//!< Vertex input bindings will not contain data for more than one attribute.
		BINDING_MAPPING_ONE_TO_MANY		//!< Vertex input bindings can contain data for more than one attribute.
	};

	enum AttributeLayout
	{
		ATTRIBUTE_LAYOUT_INTERLEAVED,	//!< Attribute data is bundled together as if in a structure: [pos 0][color 0][pos 1][color 1]...
		ATTRIBUTE_LAYOUT_SEQUENTIAL		//!< Data for each attribute is laid out separately: [pos 0][pos 1]...[color 0][color 1]...
										//   Sequential only makes a difference if ONE_TO_MANY mapping is used (more than one attribute in a binding).
	};

	struct AttributeInfo
	{
		GlslType				glslType;
		VkFormat				vkType;
		VkVertexInputRate		inputRate;
	};

	struct GlslTypeDescription
	{
		const char*		name;
		int				vertexInputComponentCount;
		int				vertexInputCount;
		GlslBasicType	basicType;
	};

	static const GlslTypeDescription		s_glslTypeDescriptions[GLSL_TYPE_COUNT];

											VertexInputTest				(tcu::TestContext&					testContext,
																		 const std::string&					name,
																		 const std::string&					description,
																		 const std::vector<AttributeInfo>&	attributeInfos,
																		 BindingMapping						bindingMapping,
																		 AttributeLayout					attributeLayout);

	virtual									~VertexInputTest			(void) {}
	virtual void							initPrograms				(SourceCollections& programCollection) const;
	virtual TestInstance*					createInstance				(Context& context) const;
	static bool								isCompatibleType			(VkFormat format, GlslType glslType);

private:
	AttributeInfo							getAttributeInfo			(size_t attributeNdx) const;
	size_t									getNumAttributes			(void) const;
	std::string								getGlslInputDeclarations	(void) const;
	std::string								getGlslVertexCheck			(void) const;
	std::string								getGlslAttributeConditions	(const AttributeInfo& attributeInfo, const std::string attributeIndex) const;
	static tcu::Vec4						getFormatThreshold			(VkFormat format);

	const std::vector<AttributeInfo>		m_attributeInfos;
	const BindingMapping					m_bindingMapping;
	const AttributeLayout					m_attributeLayout;
	const bool								m_queryMaxAttributes;
	bool									m_usesDoubleType;
	mutable size_t							m_maxAttributes;
};

class VertexInputInstance : public vkt::TestInstance
{
public:
	struct VertexInputAttributeDescription
	{
		VertexInputTest::GlslType			glslType;
		int									vertexInputIndex;
		VkVertexInputAttributeDescription	vkDescription;
	};

	typedef	std::vector<VertexInputAttributeDescription>	AttributeDescriptionList;

											VertexInputInstance			(Context&												context,
																		 const AttributeDescriptionList&						attributeDescriptions,
																		 const std::vector<VkVertexInputBindingDescription>&	bindingDescriptions,
																		 const std::vector<VkDeviceSize>&						bindingOffsets);

	virtual									~VertexInputInstance		(void);
	virtual tcu::TestStatus					iterate						(void);


	static void								writeVertexInputData		(deUint8* destPtr, const VkVertexInputBindingDescription& bindingDescription, const VkDeviceSize bindingOffset, const AttributeDescriptionList& attributes);
	static void								writeVertexInputValue		(deUint8* destPtr, const VertexInputAttributeDescription& attributes, int indexId);

private:
	tcu::TestStatus							verifyImage					(void);

private:
	std::vector<VkBuffer>					m_vertexBuffers;
	std::vector<Allocation*>				m_vertexBufferAllocs;

	const tcu::UVec2						m_renderSize;
	const VkFormat							m_colorFormat;

	Move<VkImage>							m_colorImage;
	de::MovePtr<Allocation>					m_colorImageAlloc;
	Move<VkImage>							m_depthImage;
	Move<VkImageView>						m_colorAttachmentView;
	Move<VkRenderPass>						m_renderPass;
	Move<VkFramebuffer>						m_framebuffer;

	Move<VkShaderModule>					m_vertexShaderModule;
	Move<VkShaderModule>					m_fragmentShaderModule;

	Move<VkPipelineLayout>					m_pipelineLayout;
	Move<VkPipeline>						m_graphicsPipeline;

	Move<VkCommandPool>						m_cmdPool;
	Move<VkCommandBuffer>					m_cmdBuffer;

	Move<VkFence>							m_fence;
};

const VertexInputTest::GlslTypeDescription VertexInputTest::s_glslTypeDescriptions[GLSL_TYPE_COUNT] =
{
	{ "int",	1, 1, GLSL_BASIC_TYPE_INT },
	{ "ivec2",	2, 1, GLSL_BASIC_TYPE_INT },
	{ "ivec3",	3, 1, GLSL_BASIC_TYPE_INT },
	{ "ivec4",	4, 1, GLSL_BASIC_TYPE_INT },

	{ "uint",	1, 1, GLSL_BASIC_TYPE_UINT },
	{ "uvec2",	2, 1, GLSL_BASIC_TYPE_UINT },
	{ "uvec3",	3, 1, GLSL_BASIC_TYPE_UINT },
	{ "uvec4",	4, 1, GLSL_BASIC_TYPE_UINT },

	{ "float",	1, 1, GLSL_BASIC_TYPE_FLOAT },
	{ "vec2",	2, 1, GLSL_BASIC_TYPE_FLOAT },
	{ "vec3",	3, 1, GLSL_BASIC_TYPE_FLOAT },
	{ "vec4",	4, 1, GLSL_BASIC_TYPE_FLOAT },
	{ "mat2",	2, 2, GLSL_BASIC_TYPE_FLOAT },
	{ "mat3",	3, 3, GLSL_BASIC_TYPE_FLOAT },
	{ "mat4",	4, 4, GLSL_BASIC_TYPE_FLOAT },

	{ "double",	1, 1, GLSL_BASIC_TYPE_DOUBLE },
	{ "dvec2",	2, 1, GLSL_BASIC_TYPE_DOUBLE },
	{ "dvec3",	3, 1, GLSL_BASIC_TYPE_DOUBLE },
	{ "dvec4",	4, 1, GLSL_BASIC_TYPE_DOUBLE },
	{ "dmat2",	2, 2, GLSL_BASIC_TYPE_DOUBLE },
	{ "dmat3",	3, 3, GLSL_BASIC_TYPE_DOUBLE },
	{ "dmat4",	4, 4, GLSL_BASIC_TYPE_DOUBLE }
};


VertexInputTest::VertexInputTest (tcu::TestContext&						testContext,
								  const std::string&					name,
								  const std::string&					description,
								  const std::vector<AttributeInfo>&		attributeInfos,
								  BindingMapping						bindingMapping,
								  AttributeLayout						attributeLayout)

	: vkt::TestCase			(testContext, name, description)
	, m_attributeInfos		(attributeInfos)
	, m_bindingMapping		(bindingMapping)
	, m_attributeLayout		(attributeLayout)
	, m_queryMaxAttributes	(attributeInfos.size() == 0)
	, m_maxAttributes		(16)
{
	DE_ASSERT(m_attributeLayout == ATTRIBUTE_LAYOUT_INTERLEAVED || m_bindingMapping == BINDING_MAPPING_ONE_TO_MANY);

	m_usesDoubleType = false;

	for (size_t attributeNdx = 0; attributeNdx < m_attributeInfos.size(); attributeNdx++)
	{
		if (s_glslTypeDescriptions[m_attributeInfos[attributeNdx].glslType].basicType == GLSL_BASIC_TYPE_DOUBLE)
		{
			m_usesDoubleType = true;
			break;
		}
	}
}

deUint32 getAttributeBinding (const VertexInputTest::BindingMapping bindingMapping, const VkVertexInputRate firstInputRate, const VkVertexInputRate inputRate, const deUint32 attributeNdx)
{
	if (bindingMapping == VertexInputTest::BINDING_MAPPING_ONE_TO_ONE)
	{
		// Each attribute uses a unique binding
		return attributeNdx;
	}
	else // bindingMapping == BINDING_MAPPING_ONE_TO_MANY
	{
		// Alternate between two bindings
		return deUint32(firstInputRate + inputRate) % 2u;
	}
}

//! Number of locations used up by an attribute.
deUint32 getConsumedLocations (const VertexInputTest::AttributeInfo& attributeInfo)
{
	// double formats with more than 2 components will take 2 locations
	const VertexInputTest::GlslType type = attributeInfo.glslType;
	if ((type == VertexInputTest::GLSL_TYPE_DMAT2 || type == VertexInputTest::GLSL_TYPE_DMAT3 || type == VertexInputTest::GLSL_TYPE_DMAT4) &&
		(attributeInfo.vkType == VK_FORMAT_R64G64B64_SFLOAT || attributeInfo.vkType == VK_FORMAT_R64G64B64A64_SFLOAT))
	{
		return 2u;
	}
	else
		return 1u;
}

VertexInputTest::AttributeInfo VertexInputTest::getAttributeInfo (size_t attributeNdx) const
{
	if (m_queryMaxAttributes)
	{
		AttributeInfo attributeInfo =
		{
			GLSL_TYPE_VEC4,
			VK_FORMAT_R8G8B8A8_SNORM,
			(attributeNdx % 2 == 0) ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE
		};

		return attributeInfo;
	}
	else
	{
		return m_attributeInfos.at(attributeNdx);
	}
}

size_t VertexInputTest::getNumAttributes (void) const
{
	if (m_queryMaxAttributes)
		return m_maxAttributes;
	else
		return m_attributeInfos.size();
}

TestInstance* VertexInputTest::createInstance (Context& context) const
{
	typedef VertexInputInstance::VertexInputAttributeDescription VertexInputAttributeDescription;

	// Check upfront for maximum number of vertex input attributes
	{
		const InstanceInterface&		vki				= context.getInstanceInterface();
		const VkPhysicalDevice			physDevice		= context.getPhysicalDevice();
		const VkPhysicalDeviceLimits	limits			= getPhysicalDeviceProperties(vki, physDevice).limits;

		const deUint32					maxAttributes	= limits.maxVertexInputAttributes;

		if (m_attributeInfos.size() > maxAttributes)
		{
			const std::string notSupportedStr = "Unsupported number of vertex input attributes, maxVertexInputAttributes: " + de::toString(maxAttributes);
			TCU_THROW(NotSupportedError, notSupportedStr.c_str());
		}

		// Use VkPhysicalDeviceLimits::maxVertexInputAttributes
		if (m_queryMaxAttributes)
			m_maxAttributes = maxAttributes;
	}

	// Create enough binding descriptions with random offsets
	std::vector<VkVertexInputBindingDescription>	bindingDescriptions;
	std::vector<VkDeviceSize>						bindingOffsets;
	const size_t									numAttributes		= getNumAttributes();
	const size_t									numBindings			= (m_bindingMapping == BINDING_MAPPING_ONE_TO_ONE) ? numAttributes : ((numAttributes > 1) ? 2 : 1);
	const VkVertexInputRate							firstInputrate		= getAttributeInfo(0).inputRate;

	for (size_t bindingNdx = 0; bindingNdx < numBindings; ++bindingNdx)
	{
		// Bindings alternate between STEP_RATE_VERTEX and STEP_RATE_INSTANCE
		const VkVertexInputRate						inputRate			= ((firstInputrate + bindingNdx) % 2 == 0) ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;

		// Stride will be updated when creating the attribute descriptions
		const VkVertexInputBindingDescription		bindingDescription	=
		{
			static_cast<deUint32>(bindingNdx),		// deUint32				binding;
			0u,										// deUint32				stride;
			inputRate								// VkVertexInputRate	inputRate;
		};

		bindingDescriptions.push_back(bindingDescription);
		bindingOffsets.push_back(4 * bindingNdx);
	}

	std::vector<VertexInputAttributeDescription>	attributeDescriptions;
	deUint32										attributeLocation		= 0;
	std::vector<deUint32>							attributeOffsets		(bindingDescriptions.size(), 0);
	std::vector<deUint32>							attributeMaxSizes		(bindingDescriptions.size(), 0);	// max component or vector size, depending on which layout we are using

	// To place the attributes sequentially we need to know the largest attribute and use its size in stride and offset calculations.
	if (m_attributeLayout == ATTRIBUTE_LAYOUT_SEQUENTIAL)
		for (size_t attributeNdx = 0; attributeNdx < numAttributes; ++attributeNdx)
		{
			const AttributeInfo&	attributeInfo			= getAttributeInfo(attributeNdx);
			const deUint32			attributeBinding		= getAttributeBinding(m_bindingMapping, firstInputrate, attributeInfo.inputRate, static_cast<deUint32>(attributeNdx));
			const deUint32			inputSize				= getVertexFormatSize(attributeInfo.vkType);

			attributeMaxSizes[attributeBinding]				= de::max(attributeMaxSizes[attributeBinding], inputSize);
		}

	// Create attribute descriptions, assign them to bindings and update stride.
	for (size_t attributeNdx = 0; attributeNdx < numAttributes; ++attributeNdx)
	{
		const AttributeInfo&		attributeInfo			= getAttributeInfo(attributeNdx);
		const GlslTypeDescription&	glslTypeDescription		= s_glslTypeDescriptions[attributeInfo.glslType];
		const deUint32				inputSize				= getVertexFormatSize(attributeInfo.vkType);
		const deUint32				attributeBinding		= getAttributeBinding(m_bindingMapping, firstInputrate, attributeInfo.inputRate, static_cast<deUint32>(attributeNdx));
		const deUint32				vertexCount				= (attributeInfo.inputRate == VK_VERTEX_INPUT_RATE_VERTEX) ? (4 * 2) : 2;

		VertexInputAttributeDescription attributeDescription =
		{
			attributeInfo.glslType,							// GlslType		glslType;
			0,												// int			vertexInputIndex;
			{
				0u,											// uint32_t    location;
				attributeBinding,							// uint32_t    binding;
				attributeInfo.vkType,						// VkFormat    format;
				0u,											// uint32_t    offset;
			},
		};

		// Matrix types add each column as a separate attribute.
		for (int descNdx = 0; descNdx < glslTypeDescription.vertexInputCount; ++descNdx)
		{
			attributeDescription.vertexInputIndex		= descNdx;
			attributeDescription.vkDescription.location	= attributeLocation;

			if (m_attributeLayout == ATTRIBUTE_LAYOUT_INTERLEAVED)
			{
				const deUint32	offsetToComponentAlignment		 = getNextMultipleOffset(getVertexFormatComponentSize(attributeInfo.vkType),
																						 (deUint32)bindingOffsets[attributeBinding] + attributeOffsets[attributeBinding]);
				attributeOffsets[attributeBinding]				+= offsetToComponentAlignment;

				attributeDescription.vkDescription.offset		 = attributeOffsets[attributeBinding];
				attributeDescriptions.push_back(attributeDescription);

				bindingDescriptions[attributeBinding].stride	+= offsetToComponentAlignment + inputSize;
				attributeOffsets[attributeBinding]				+= inputSize;
				attributeMaxSizes[attributeBinding]				 = de::max(attributeMaxSizes[attributeBinding], getVertexFormatComponentSize(attributeInfo.vkType));
			}
			else // m_attributeLayout == ATTRIBUTE_LAYOUT_SEQUENTIAL
			{
				attributeDescription.vkDescription.offset		 = attributeOffsets[attributeBinding];
				attributeDescriptions.push_back(attributeDescription);

				attributeOffsets[attributeBinding]				+= vertexCount * attributeMaxSizes[attributeBinding];
			}

			attributeLocation += getConsumedLocations(attributeInfo);
		}

		if (m_attributeLayout == ATTRIBUTE_LAYOUT_SEQUENTIAL)
			bindingDescriptions[attributeBinding].stride = attributeMaxSizes[attributeBinding];
	}

	// Make sure the stride results in aligned access
	for (size_t bindingNdx = 0; bindingNdx < bindingDescriptions.size(); ++bindingNdx)
	{
		if (attributeMaxSizes[bindingNdx] > 0)
			bindingDescriptions[bindingNdx].stride += getNextMultipleOffset(attributeMaxSizes[bindingNdx], bindingDescriptions[bindingNdx].stride);
	}

	// Check upfront for maximum number of vertex input bindings
	{
		const InstanceInterface&		vki				= context.getInstanceInterface();
		const VkPhysicalDevice			physDevice		= context.getPhysicalDevice();
		const VkPhysicalDeviceLimits	limits			= getPhysicalDeviceProperties(vki, physDevice).limits;

		const deUint32					maxBindings		= limits.maxVertexInputBindings;

		if (bindingDescriptions.size() > maxBindings)
		{
			const std::string notSupportedStr = "Unsupported number of vertex input bindings, maxVertexInputBindings: " + de::toString(maxBindings);
			TCU_THROW(NotSupportedError, notSupportedStr.c_str());
		}
	}

	return new VertexInputInstance(context, attributeDescriptions, bindingDescriptions, bindingOffsets);
}

void VertexInputTest::initPrograms (SourceCollections& programCollection) const
{
	std::ostringstream vertexSrc;

	vertexSrc << "#version 440\n"
			  << "layout(constant_id = 0) const int numAttributes = " << m_maxAttributes << ";\n"
			  << getGlslInputDeclarations()
			  << "layout(location = 0) out highp vec4 vtxColor;\n"
			  << "out gl_PerVertex {\n"
			  << "  vec4 gl_Position;\n"
			  << "};\n";

	// NOTE: double abs(double x) undefined in glslang ??
	if (m_usesDoubleType)
		vertexSrc << "double abs (double x) { if (x < 0.0LF) return -x; else return x; }\n";

	vertexSrc << "void main (void)\n"
			  << "{\n"
			  << getGlslVertexCheck()
			  << "}\n";

	programCollection.glslSources.add("attribute_test_vert") << glu::VertexSource(vertexSrc.str());

	programCollection.glslSources.add("attribute_test_frag") << glu::FragmentSource(
		"#version 440\n"
		"layout(location = 0) in highp vec4 vtxColor;\n"
		"layout(location = 0) out highp vec4 fragColor;\n"
		"void main (void)\n"
		"{\n"
		"	fragColor = vtxColor;\n"
		"}\n");
}

std::string VertexInputTest::getGlslInputDeclarations (void) const
{
	std::ostringstream	glslInputs;
	deUint32			location = 0;

	if (m_queryMaxAttributes)
	{
		const GlslTypeDescription& glslTypeDesc = s_glslTypeDescriptions[GLSL_TYPE_VEC4];
		glslInputs << "layout(location = 0) in " << glslTypeDesc.name << " attr[numAttributes];\n";
	}
	else
	{
		for (size_t attributeNdx = 0; attributeNdx < m_attributeInfos.size(); attributeNdx++)
		{
			const GlslTypeDescription& glslTypeDesc = s_glslTypeDescriptions[m_attributeInfos[attributeNdx].glslType];

			glslInputs << "layout(location = " << location << ") in " << glslTypeDesc.name << " attr" << attributeNdx << ";\n";
			location += glslTypeDesc.vertexInputCount;
		}
	}

	return glslInputs.str();
}

std::string VertexInputTest::getGlslVertexCheck (void) const
{
	std::ostringstream	glslCode;
	int					totalInputComponentCount	= 0;

	glslCode << "	int okCount = 0;\n";

	if (m_queryMaxAttributes)
	{
		const AttributeInfo attributeInfo = getAttributeInfo(0);

		glslCode << "	for (int checkNdx = 0; checkNdx < numAttributes; checkNdx++)\n"
				 <<	"	{\n"
				 << "		uint index = (checkNdx % 2 == 0) ? gl_VertexIndex : gl_InstanceIndex;\n";

		glslCode << getGlslAttributeConditions(attributeInfo, "checkNdx")
				 << "	}\n";

			const int vertexInputCount	= VertexInputTest::s_glslTypeDescriptions[attributeInfo.glslType].vertexInputCount;
			totalInputComponentCount	+= vertexInputCount * VertexInputTest::s_glslTypeDescriptions[attributeInfo.glslType].vertexInputComponentCount;

		glslCode <<
			"	if (okCount == " << totalInputComponentCount << " * numAttributes)\n"
			"	{\n"
			"		if (gl_InstanceIndex == 0)\n"
			"			vtxColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"		else\n"
			"			vtxColor = vec4(0.0, 0.0, 1.0, 1.0);\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		vtxColor = vec4(okCount / float(" << totalInputComponentCount << " * numAttributes), 0.0f, 0.0f, 1.0);\n" <<
			"	}\n\n"
			"	if (gl_InstanceIndex == 0)\n"
			"	{\n"
			"		if (gl_VertexIndex == 0) gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 1) gl_Position = vec4(0.0, -1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 2) gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 3) gl_Position = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"		else gl_Position = vec4(0.0);\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		if (gl_VertexIndex == 0) gl_Position = vec4(0.0, -1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 1) gl_Position = vec4(1.0, -1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 2) gl_Position = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 3) gl_Position = vec4(1.0, 1.0, 0.0, 1.0);\n"
			"		else gl_Position = vec4(0.0);\n"
			"	}\n";
	}
	else
	{
		for (size_t attributeNdx = 0; attributeNdx < m_attributeInfos.size(); attributeNdx++)
		{
			glslCode << getGlslAttributeConditions(m_attributeInfos[attributeNdx], de::toString(attributeNdx));

			const int vertexInputCount	= VertexInputTest::s_glslTypeDescriptions[m_attributeInfos[attributeNdx].glslType].vertexInputCount;
			totalInputComponentCount	+= vertexInputCount * VertexInputTest::s_glslTypeDescriptions[m_attributeInfos[attributeNdx].glslType].vertexInputComponentCount;
		}

		glslCode <<
			"	if (okCount == " << totalInputComponentCount << ")\n"
			"	{\n"
			"		if (gl_InstanceIndex == 0)\n"
			"			vtxColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"		else\n"
			"			vtxColor = vec4(0.0, 0.0, 1.0, 1.0);\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		vtxColor = vec4(okCount / float(" << totalInputComponentCount << "), 0.0f, 0.0f, 1.0);\n" <<
			"	}\n\n"
			"	if (gl_InstanceIndex == 0)\n"
			"	{\n"
			"		if (gl_VertexIndex == 0) gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 1) gl_Position = vec4(0.0, -1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 2) gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 3) gl_Position = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"		else gl_Position = vec4(0.0);\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		if (gl_VertexIndex == 0) gl_Position = vec4(0.0, -1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 1) gl_Position = vec4(1.0, -1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 2) gl_Position = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"		else if (gl_VertexIndex == 3) gl_Position = vec4(1.0, 1.0, 0.0, 1.0);\n"
			"		else gl_Position = vec4(0.0);\n"
			"	}\n";
	}
	return glslCode.str();
}

std::string VertexInputTest::getGlslAttributeConditions (const AttributeInfo& attributeInfo, const std::string attributeIndex) const
{
	std::ostringstream	glslCode;
	std::ostringstream	attributeVar;
	const int			componentCount		= VertexInputTest::s_glslTypeDescriptions[attributeInfo.glslType].vertexInputComponentCount;
	const int			vertexInputCount	= VertexInputTest::s_glslTypeDescriptions[attributeInfo.glslType].vertexInputCount;
	const deUint32		totalComponentCount	= componentCount * vertexInputCount;
	const tcu::Vec4		threshold			= getFormatThreshold(attributeInfo.vkType);
	deUint32			componentIndex		= 0;
	const std::string	indexStr			= m_queryMaxAttributes ? "[" + attributeIndex + "]" : attributeIndex;
	const std::string	indentStr			= m_queryMaxAttributes ? "\t\t" : "\t";
	std::string			indexId;

	if (m_queryMaxAttributes)
		indexId	= "index";
	else
		indexId	= (attributeInfo.inputRate == VK_VERTEX_INPUT_RATE_VERTEX) ? "gl_VertexIndex" : "gl_InstanceIndex";

	attributeVar << "attr" << indexStr;

	glslCode << std::fixed;

	for (int columnNdx = 0; columnNdx< vertexInputCount; columnNdx++)
	{
		for (int rowNdx = 0; rowNdx < componentCount; rowNdx++)
		{
			std::string accessStr;
			{
				// Build string representing the access to the attribute component
				std::ostringstream accessStream;
				accessStream << attributeVar.str();

				if (vertexInputCount == 1)
				{
					if (componentCount > 1)
						accessStream << "[" << rowNdx << "]";
				}
				else
				{
					accessStream << "[" << columnNdx << "][" << rowNdx << "]";
				}

				accessStr = accessStream.str();
			}

			if (isVertexFormatSint(attributeInfo.vkType))
			{
				glslCode << indentStr <<  "if (" << accessStr << " == -(" << totalComponentCount << " * " << indexId << " + " << componentIndex << "))\n";
			}
			else if (isVertexFormatUint(attributeInfo.vkType))
			{
				glslCode << indentStr << "if (" << accessStr << " == uint(" << totalComponentCount << " * " << indexId << " + " << componentIndex << "))\n";
			}
			else if (isVertexFormatSfloat(attributeInfo.vkType))
			{
				if (VertexInputTest::s_glslTypeDescriptions[attributeInfo.glslType].basicType == VertexInputTest::GLSL_BASIC_TYPE_DOUBLE)
				{
					glslCode << indentStr << "if (abs(" << accessStr << " + double(0.01 * (" << totalComponentCount << ".0 * float(" << indexId << ") + " << componentIndex << ".0))) < double(" << threshold[rowNdx] << "))\n";
				}
				else
				{
					glslCode << indentStr << "if (abs(" << accessStr << " + (0.01 * (" << totalComponentCount << ".0 * float(" << indexId << ") + " << componentIndex << ".0))) < " << threshold[rowNdx] << ")\n";
				}
			}
			else if (isVertexFormatSscaled(attributeInfo.vkType))
			{
				glslCode << indentStr << "if (abs(" << accessStr << " + (" << totalComponentCount << ".0 * float(" << indexId << ") + " << componentIndex << ".0)) < " << threshold[rowNdx] << ")\n";
			}
			else if (isVertexFormatUscaled(attributeInfo.vkType))
			{
				glslCode << indentStr << "if (abs(" << accessStr << " - (" << totalComponentCount << ".0 * float(" << indexId << ") + " << componentIndex << ".0)) < " << threshold[rowNdx] << ")\n";
			}
			else if (isVertexFormatSnorm(attributeInfo.vkType))
			{
				const float representableDiff = getRepresentableDifferenceSnorm(attributeInfo.vkType);

				glslCode << indentStr << "if (abs(" << accessStr << " - (-1.0 + " << representableDiff << " * (" << totalComponentCount << ".0 * float(" << indexId << ") + " << componentIndex << ".0))) < " << threshold[rowNdx] << ")\n";
			}
			else if (isVertexFormatUnorm(attributeInfo.vkType) || isVertexFormatSRGB(attributeInfo.vkType))
			{
				const float representableDiff = getRepresentableDifferenceUnorm(attributeInfo.vkType);

				glslCode << indentStr << "if (abs(" << accessStr << " - " << "(" << representableDiff << " * (" << totalComponentCount << ".0 * float(" << indexId << ") + " << componentIndex << ".0))) < " << threshold[rowNdx] << ")\n";
			}
			else
			{
				DE_ASSERT(false);
			}

			glslCode << indentStr << "\tokCount++;\n\n";

			componentIndex++;
		}
	}
	return glslCode.str();
}

tcu::Vec4 VertexInputTest::getFormatThreshold (VkFormat format)
{
	using tcu::Vec4;

	switch (format)
	{
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R32G32B32_SFLOAT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		case VK_FORMAT_R64_SFLOAT:
		case VK_FORMAT_R64G64_SFLOAT:
		case VK_FORMAT_R64G64B64_SFLOAT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return Vec4(0.00001f);

		default:
			break;
	}

	if (isVertexFormatSnorm(format))
	{
		return Vec4(1.5f * getRepresentableDifferenceSnorm(format));
	}
	else if (isVertexFormatUnorm(format))
	{
		return Vec4(1.5f * getRepresentableDifferenceUnorm(format));
	}

	return Vec4(0.001f);
}

VertexInputInstance::VertexInputInstance (Context&												context,
										  const AttributeDescriptionList&						attributeDescriptions,
										  const std::vector<VkVertexInputBindingDescription>&	bindingDescriptions,
										  const std::vector<VkDeviceSize>&						bindingOffsets)
	: vkt::TestInstance			(context)
	, m_renderSize				(16, 16)
	, m_colorFormat				(VK_FORMAT_R8G8B8A8_UNORM)
{
	DE_ASSERT(bindingDescriptions.size() == bindingOffsets.size());

	const DeviceInterface&		vk						= context.getDeviceInterface();
	const VkDevice				vkDevice				= context.getDevice();
	const deUint32				queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	SimpleAllocator				memAlloc				(vk, vkDevice, getPhysicalDeviceMemoryProperties(context.getInstanceInterface(), context.getPhysicalDevice()));
	const VkComponentMapping	componentMappingRGBA	= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	// Check upfront for unsupported features
	for (size_t attributeNdx = 0; attributeNdx < attributeDescriptions.size(); attributeNdx++)
	{
		const VkVertexInputAttributeDescription& attributeDescription = attributeDescriptions[attributeNdx].vkDescription;
		if (!isSupportedVertexFormat(context, attributeDescription.format))
		{
			throw tcu::NotSupportedError(std::string("Unsupported format for vertex input: ") + getFormatName(attributeDescription.format));
		}
	}

	// Create color image
	{
		const VkImageCreateInfo colorImageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,										// VkStructureType			sType;
			DE_NULL,																	// const void*				pNext;
			0u,																			// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,															// VkImageType				imageType;
			m_colorFormat,																// VkFormat					format;
			{ m_renderSize.x(), m_renderSize.y(), 1u },									// VkExtent3D				extent;
			1u,																			// deUint32					mipLevels;
			1u,																			// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,														// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,													// VkImageTiling			tiling;
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,		// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,													// VkSharingMode			sharingMode;
			1u,																			// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,															// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED,													// VkImageLayout			initialLayout;
		};

		m_colorImage			= createImage(vk, vkDevice, &colorImageParams);

		// Allocate and bind color image memory
		m_colorImageAlloc		= memAlloc.allocate(getImageMemoryRequirements(vk, vkDevice, *m_colorImage), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(vkDevice, *m_colorImage, m_colorImageAlloc->getMemory(), m_colorImageAlloc->getOffset()));
	}

	// Create color attachment view
	{
		const VkImageViewCreateInfo colorAttachmentViewParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			0u,												// VkImageViewCreateFlags	flags;
			*m_colorImage,									// VkImage					image;
			VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType			viewType;
			m_colorFormat,									// VkFormat					format;
			componentMappingRGBA,							// VkComponentMapping		components;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },  // VkImageSubresourceRange	subresourceRange;
		};

		m_colorAttachmentView = createImageView(vk, vkDevice, &colorAttachmentViewParams);
	}

	// Create render pass
	{
		const VkAttachmentDescription colorAttachmentDescription =
		{
			0u,													// VkAttachmentDescriptionFlags		flags;
			m_colorFormat,										// VkFormat							format;
			VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout					finalLayout;
		};

		const VkAttachmentReference colorAttachmentReference =
		{
			0u,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
		};

		const VkSubpassDescription subpassDescription =
		{
			0u,													// VkSubpassDescriptionFlags	flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint			pipelineBindPoint;
			0u,													// deUint32						inputAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*	pInputAttachments;
			1u,													// deUint32						colorAttachmentCount;
			&colorAttachmentReference,							// const VkAttachmentReference*	pColorAttachments;
			DE_NULL,											// const VkAttachmentReference*	pResolveAttachments;
			DE_NULL,											// const VkAttachmentReference*	pDepthStencilAttachment;
			0u,													// deUint32						preserveAttachmentCount;
			DE_NULL												// const VkAttachmentReference*	pPreserveAttachments;
		};

		const VkRenderPassCreateInfo renderPassParams =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkRenderPassCreateFlags			flags;
			1u,													// deUint32							attachmentCount;
			&colorAttachmentDescription,						// const VkAttachmentDescription*	pAttachments;
			1u,													// deUint32							subpassCount;
			&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
			0u,													// deUint32							dependencyCount;
			DE_NULL												// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass = createRenderPass(vk, vkDevice, &renderPassParams);
	}

	// Create framebuffer
	{
		const VkFramebufferCreateInfo framebufferParams =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkFramebufferCreateFlags	flags;
			*m_renderPass,										// VkRenderPass				renderPass;
			1u,													// deUint32					attachmentCount;
			&m_colorAttachmentView.get(),						// const VkImageView*		pAttachments;
			(deUint32)m_renderSize.x(),							// deUint32					width;
			(deUint32)m_renderSize.y(),							// deUint32					height;
			1u													// deUint32					layers;
		};

		m_framebuffer = createFramebuffer(vk, vkDevice, &framebufferParams);
	}

	// Create pipeline layout
	{
		const VkPipelineLayoutCreateInfo pipelineLayoutParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkPipelineLayoutCreateFlags		flags;
			0u,													// deUint32							setLayoutCount;
			DE_NULL,											// const VkDescriptorSetLayout*		pSetLayouts;
			0u,													// deUint32							pushConstantRangeCount;
			DE_NULL												// const VkPushConstantRange*		pPushConstantRanges;
		};

		m_pipelineLayout = createPipelineLayout(vk, vkDevice, &pipelineLayoutParams);
	}

	m_vertexShaderModule	= createShaderModule(vk, vkDevice, m_context.getBinaryCollection().get("attribute_test_vert"), 0);
	m_fragmentShaderModule	= createShaderModule(vk, vkDevice, m_context.getBinaryCollection().get("attribute_test_frag"), 0);

	// Create specialization constant
	deUint32 specializationData = static_cast<deUint32>(attributeDescriptions.size());

	const VkSpecializationMapEntry specializationMapEntry =
	{
		0,														// uint32_t							constantID
		0,														// uint32_t							offset
		sizeof(specializationData),								// uint32_t							size
	};

	const VkSpecializationInfo specializationInfo =
	{
		1,														// uint32_t							mapEntryCount
		&specializationMapEntry,								// const void*						pMapEntries
		sizeof(specializationData),								// size_t							dataSize
		&specializationData										// const void*						pData
	};

	// Create pipeline
	{
		const VkPipelineShaderStageCreateInfo shaderStageParams[2] =
		{
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits				stage;
				*m_vertexShaderModule,										// VkShaderModule						module;
				"main",														// const char*							pName;
				&specializationInfo											// const VkSpecializationInfo*			pSpecializationInfo;
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits				stage;
				*m_fragmentShaderModule,									// VkShaderModule						module;
				"main",														// const char*							pName;
				DE_NULL														// const VkSpecializationInfo*			pSpecializationInfo;
			}
		};

		// Create vertex attribute array and check if their VK formats are supported
		std::vector<VkVertexInputAttributeDescription> vkAttributeDescriptions;
		for (size_t attributeNdx = 0; attributeNdx < attributeDescriptions.size(); attributeNdx++)
		{
			const VkVertexInputAttributeDescription& attributeDescription = attributeDescriptions[attributeNdx].vkDescription;
			vkAttributeDescriptions.push_back(attributeDescription);
		}

		const VkPipelineVertexInputStateCreateInfo	vertexInputStateParams	=
		{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineVertexInputStateCreateFlags	flags;
			(deUint32)bindingDescriptions.size(),							// deUint32									vertexBindingDescriptionCount;
			bindingDescriptions.data(),										// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
			(deUint32)vkAttributeDescriptions.size(),						// deUint32									vertexAttributeDescriptionCount;
			vkAttributeDescriptions.data()									// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
		};

		const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineInputAssemblyStateCreateFlags	flags;
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,							// VkPrimitiveTopology						topology;
			false															// VkBool32									primitiveRestartEnable;
		};

		const VkViewport viewport =
		{
			0.0f,						// float	x;
			0.0f,						// float	y;
			(float)m_renderSize.x(),	// float	width;
			(float)m_renderSize.y(),	// float	height;
			0.0f,						// float	minDepth;
			1.0f						// float	maxDepth;
		};

		const VkRect2D scissor = { { 0, 0 }, { m_renderSize.x(), m_renderSize.y() } };

		const VkPipelineViewportStateCreateInfo viewportStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,														// const void*							pNext;
			0u,																// VkPipelineViewportStateCreateFlags	flags;
			1u,																// deUint32								viewportCount;
			&viewport,														// const VkViewport*					pViewports;
			1u,																// deUint32								scissorCount;
			&scissor														// const VkRect2D*						pScissors;
		};

		const VkPipelineRasterizationStateCreateInfo rasterStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineRasterizationStateCreateFlags	flags;
			false,															// VkBool32									depthClampEnable;
			false,															// VkBool32									rasterizerDiscardEnable;
			VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
			VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
			VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
			VK_FALSE,														// VkBool32									depthBiasEnable;
			0.0f,															// float									depthBiasConstantFactor;
			0.0f,															// float									depthBiasClamp;
			0.0f,															// float									depthBiasSlopeFactor;
			1.0f,															// float									lineWidth;
		};

		const VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
		{
			false,																		// VkBool32					blendEnable;
			VK_BLEND_FACTOR_ONE,														// VkBlendFactor			srcColorBlendFactor;
			VK_BLEND_FACTOR_ZERO,														// VkBlendFactor			dstColorBlendFactor;
			VK_BLEND_OP_ADD,															// VkBlendOp				colorBlendOp;
			VK_BLEND_FACTOR_ONE,														// VkBlendFactor			srcAlphaBlendFactor;
			VK_BLEND_FACTOR_ZERO,														// VkBlendFactor			dstAlphaBlendFactor;
			VK_BLEND_OP_ADD,															// VkBlendOp				alphaBlendOp;
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |						// VkColorComponentFlags	colorWriteMask;
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		const VkPipelineColorBlendStateCreateInfo colorBlendStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
			DE_NULL,													// const void*									pNext;
			0u,															// VkPipelineColorBlendStateCreateFlags			flags;
			false,														// VkBool32										logicOpEnable;
			VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
			1u,															// deUint32										attachmentCount;
			&colorBlendAttachmentState,									// const VkPipelineColorBlendAttachmentState*	pAttachments;
			{ 0.0f, 0.0f, 0.0f, 0.0f },									// float										blendConstants[4];
		};

		const VkPipelineMultisampleStateCreateInfo	multisampleStateParams	=
		{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineMultisampleStateCreateFlags	flags;
			VK_SAMPLE_COUNT_1_BIT,										// VkSampleCountFlagBits					rasterizationSamples;
			false,														// VkBool32									sampleShadingEnable;
			0.0f,														// float									minSampleShading;
			DE_NULL,													// const VkSampleMask*						pSampleMask;
			false,														// VkBool32									alphaToCoverageEnable;
			false														// VkBool32									alphaToOneEnable;
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineDepthStencilStateCreateFlags	flags;
			false,														// VkBool32									depthTestEnable;
			false,														// VkBool32									depthWriteEnable;
			VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
			false,														// VkBool32									depthBoundsTestEnable;
			false,														// VkBool32									stencilTestEnable;
			// VkStencilOpState	front;
			{
				VK_STENCIL_OP_KEEP,		// VkStencilOp	failOp;
				VK_STENCIL_OP_KEEP,		// VkStencilOp	passOp;
				VK_STENCIL_OP_KEEP,		// VkStencilOp	depthFailOp;
				VK_COMPARE_OP_NEVER,	// VkCompareOp	compareOp;
				0u,						// deUint32		compareMask;
				0u,						// deUint32		writeMask;
				0u,						// deUint32		reference;
			},
			// VkStencilOpState	back;
			{
				VK_STENCIL_OP_KEEP,		// VkStencilOp	failOp;
				VK_STENCIL_OP_KEEP,		// VkStencilOp	passOp;
				VK_STENCIL_OP_KEEP,		// VkStencilOp	depthFailOp;
				VK_COMPARE_OP_NEVER,	// VkCompareOp	compareOp;
				0u,						// deUint32		compareMask;
				0u,						// deUint32		writeMask;
				0u,						// deUint32		reference;
			},
			0.0f,														// float			minDepthBounds;
			1.0f,														// float			maxDepthBounds;
		};

		const VkGraphicsPipelineCreateInfo graphicsPipelineParams =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
			DE_NULL,											// const void*										pNext;
			0u,													// VkPipelineCreateFlags							flags;
			2u,													// deUint32											stageCount;
			shaderStageParams,									// const VkPipelineShaderStageCreateInfo*			pStages;
			&vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
			&inputAssemblyStateParams,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
			DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
			&viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
			&rasterStateParams,									// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
			&multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
			&depthStencilStateParams,							// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
			&colorBlendStateParams,								// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
			(const VkPipelineDynamicStateCreateInfo*)DE_NULL,	// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
			*m_pipelineLayout,									// VkPipelineLayout									layout;
			*m_renderPass,										// VkRenderPass										renderPass;
			0u,													// deUint32											subpass;
			0u,													// VkPipeline										basePipelineHandle;
			0u													// deInt32											basePipelineIndex;
		};

		m_graphicsPipeline	= createGraphicsPipeline(vk, vkDevice, DE_NULL, &graphicsPipelineParams);
	}

	// Create vertex buffer
	{
		const VkBufferCreateInfo vertexBufferParams =
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			4096u,										// VkDeviceSize			size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			// VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex							// const deUint32*		pQueueFamilyIndices;
		};

		// Upload data for each vertex input binding
		for (deUint32 bindingNdx = 0; bindingNdx < bindingDescriptions.size(); bindingNdx++)
		{
			Move<VkBuffer>			vertexBuffer		= createBuffer(vk, vkDevice, &vertexBufferParams);
			de::MovePtr<Allocation>	vertexBufferAlloc	= memAlloc.allocate(getBufferMemoryRequirements(vk, vkDevice, *vertexBuffer), MemoryRequirement::HostVisible);

			VK_CHECK(vk.bindBufferMemory(vkDevice, *vertexBuffer, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset()));

			writeVertexInputData((deUint8*)vertexBufferAlloc->getHostPtr(), bindingDescriptions[bindingNdx], bindingOffsets[bindingNdx], attributeDescriptions);
			flushMappedMemoryRange(vk, vkDevice, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset(), vertexBufferParams.size);

			m_vertexBuffers.push_back(vertexBuffer.disown());
			m_vertexBufferAllocs.push_back(vertexBufferAlloc.release());
		}
	}

	// Create command pool
	m_cmdPool = createCommandPool(vk, vkDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndex);

	// Create command buffer
	{
		const VkCommandBufferBeginInfo cmdBufferBeginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType					sType;
			DE_NULL,										// const void*						pNext;
			0u,												// VkCommandBufferUsageFlags		flags;
			(const VkCommandBufferInheritanceInfo*)DE_NULL,
		};

		const VkClearValue attachmentClearValue = defaultClearValue(m_colorFormat);

		const VkRenderPassBeginInfo renderPassBeginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,				// VkStructureType		sType;
			DE_NULL,												// const void*			pNext;
			*m_renderPass,											// VkRenderPass			renderPass;
			*m_framebuffer,											// VkFramebuffer		framebuffer;
			{ { 0, 0 }, { m_renderSize.x(), m_renderSize.y() } },	// VkRect2D				renderArea;
			1u,														// deUint32				clearValueCount;
			&attachmentClearValue									// const VkClearValue*	pClearValues;
		};

		const VkImageMemoryBarrier attachmentLayoutBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			0u,												// VkAccessFlags			srcAccessMask;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					dstQueueFamilyIndex;
			*m_colorImage,									// VkImage					image;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },	// VkImageSubresourceRange	subresourceRange;
		};

		m_cmdBuffer = allocateCommandBuffer(vk, vkDevice, *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

		vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0,
			0u, DE_NULL, 0u, DE_NULL, 1u, &attachmentLayoutBarrier);

		vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipeline);

		std::vector<VkBuffer> vertexBuffers;
		for (size_t bufferNdx = 0; bufferNdx < m_vertexBuffers.size(); bufferNdx++)
			vertexBuffers.push_back(m_vertexBuffers[bufferNdx]);

		if (vertexBuffers.size() <= 1)
		{
			// One vertex buffer
			vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, (deUint32)vertexBuffers.size(), vertexBuffers.data(), bindingOffsets.data());
		}
		else
		{
			// Smoke-test vkCmdBindVertexBuffers(..., startBinding, ... )

			const deUint32 firstHalfLength = (deUint32)vertexBuffers.size() / 2;
			const deUint32 secondHalfLength = firstHalfLength + (deUint32)(vertexBuffers.size() % 2);

			// Bind first half of vertex buffers
			vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, firstHalfLength, vertexBuffers.data(), bindingOffsets.data());

			// Bind second half of vertex buffers
			vk.cmdBindVertexBuffers(*m_cmdBuffer, firstHalfLength, secondHalfLength,
									vertexBuffers.data() + firstHalfLength,
									bindingOffsets.data() + firstHalfLength);
		}

		vk.cmdDraw(*m_cmdBuffer, 4, 2, 0, 0);

		vk.cmdEndRenderPass(*m_cmdBuffer);
		VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
	}

	// Create fence
	m_fence = createFence(vk, vkDevice);
}

VertexInputInstance::~VertexInputInstance (void)
{
	const DeviceInterface&		vk			= m_context.getDeviceInterface();
	const VkDevice				vkDevice	= m_context.getDevice();

	for (size_t bufferNdx = 0; bufferNdx < m_vertexBuffers.size(); bufferNdx++)
		vk.destroyBuffer(vkDevice, m_vertexBuffers[bufferNdx], DE_NULL);

	for (size_t allocNdx = 0; allocNdx < m_vertexBufferAllocs.size(); allocNdx++)
		delete m_vertexBufferAllocs[allocNdx];
}

void VertexInputInstance::writeVertexInputData(deUint8* destPtr, const VkVertexInputBindingDescription& bindingDescription, const VkDeviceSize bindingOffset, const AttributeDescriptionList& attributes)
{
	const deUint32 vertexCount = (bindingDescription.inputRate == VK_VERTEX_INPUT_RATE_VERTEX) ? (4 * 2) : 2;

	deUint8* destOffsetPtr = ((deUint8 *)destPtr) + bindingOffset;
	for (deUint32 vertexNdx = 0; vertexNdx < vertexCount; vertexNdx++)
	{
		for (size_t attributeNdx = 0; attributeNdx < attributes.size(); attributeNdx++)
		{
			const VertexInputAttributeDescription& attribDesc = attributes[attributeNdx];

			// Only write vertex input data to bindings referenced by attribute descriptions
			if (attribDesc.vkDescription.binding == bindingDescription.binding)
			{
				writeVertexInputValue(destOffsetPtr + attribDesc.vkDescription.offset, attribDesc, vertexNdx);
			}
		}
		destOffsetPtr += bindingDescription.stride;
	}
}

void writeVertexInputValueSint (deUint8* destPtr, VkFormat format, int componentNdx, deInt32 value)
{
	const deUint32	componentSize	= getVertexFormatComponentSize(format);
	deUint8*		destFormatPtr	= ((deUint8*)destPtr) + componentSize * componentNdx;

	switch (componentSize)
	{
		case 1:
			*((deInt8*)destFormatPtr) = (deInt8)value;
			break;

		case 2:
			*((deInt16*)destFormatPtr) = (deInt16)value;
			break;

		case 4:
			*((deInt32*)destFormatPtr) = (deInt32)value;
			break;

		default:
			DE_ASSERT(false);
	}
}

void writeVertexInputValueUint (deUint8* destPtr, VkFormat format, int componentNdx, deUint32 value)
{
	const deUint32	componentSize	= getVertexFormatComponentSize(format);
	deUint8*		destFormatPtr	= ((deUint8*)destPtr) + componentSize * componentNdx;

	switch (componentSize)
	{
		case 1:
			*((deUint8 *)destFormatPtr) = (deUint8)value;
			break;

		case 2:
			*((deUint16 *)destFormatPtr) = (deUint16)value;
			break;

		case 4:
			*((deUint32 *)destFormatPtr) = (deUint32)value;
			break;

		default:
			DE_ASSERT(false);
	}
}

void writeVertexInputValueSfloat (deUint8* destPtr, VkFormat format, int componentNdx, float value)
{
	const deUint32	componentSize	= getVertexFormatComponentSize(format);
	deUint8*		destFormatPtr	= ((deUint8*)destPtr) + componentSize * componentNdx;

	switch (componentSize)
	{
		case 2:
		{
			deFloat16 f16 = deFloat32To16(value);
			deMemcpy(destFormatPtr, &f16, sizeof(f16));
			break;
		}

		case 4:
			deMemcpy(destFormatPtr, &value, sizeof(value));
			break;

		default:
			DE_ASSERT(false);
	}
}

void VertexInputInstance::writeVertexInputValue (deUint8* destPtr, const VertexInputAttributeDescription& attribute, int indexId)
{
	const int		vertexInputCount	= VertexInputTest::s_glslTypeDescriptions[attribute.glslType].vertexInputCount;
	const int		componentCount		= VertexInputTest::s_glslTypeDescriptions[attribute.glslType].vertexInputComponentCount;
	const deUint32	totalComponentCount	= componentCount * vertexInputCount;
	const deUint32	vertexInputIndex	= indexId * totalComponentCount + attribute.vertexInputIndex * componentCount;
	const bool		hasBGROrder			= isVertexFormatComponentOrderBGR(attribute.vkDescription.format);
	int				swizzledNdx;

	for (int componentNdx = 0; componentNdx < componentCount; componentNdx++)
	{
		if (hasBGROrder)
		{
			if (componentNdx == 0)
				swizzledNdx = 2;
			else if (componentNdx == 2)
				swizzledNdx = 0;
			else
				swizzledNdx = componentNdx;
		}
		else
			swizzledNdx = componentNdx;

		switch (attribute.glslType)
		{
			case VertexInputTest::GLSL_TYPE_INT:
			case VertexInputTest::GLSL_TYPE_IVEC2:
			case VertexInputTest::GLSL_TYPE_IVEC3:
			case VertexInputTest::GLSL_TYPE_IVEC4:
				writeVertexInputValueSint(destPtr, attribute.vkDescription.format, componentNdx, -(deInt32)(vertexInputIndex + swizzledNdx));
				break;

			case VertexInputTest::GLSL_TYPE_UINT:
			case VertexInputTest::GLSL_TYPE_UVEC2:
			case VertexInputTest::GLSL_TYPE_UVEC3:
			case VertexInputTest::GLSL_TYPE_UVEC4:
				writeVertexInputValueUint(destPtr, attribute.vkDescription.format, componentNdx, vertexInputIndex + swizzledNdx);
				break;

			case VertexInputTest::GLSL_TYPE_FLOAT:
			case VertexInputTest::GLSL_TYPE_VEC2:
			case VertexInputTest::GLSL_TYPE_VEC3:
			case VertexInputTest::GLSL_TYPE_VEC4:
			case VertexInputTest::GLSL_TYPE_MAT2:
			case VertexInputTest::GLSL_TYPE_MAT3:
			case VertexInputTest::GLSL_TYPE_MAT4:
				if (isVertexFormatSfloat(attribute.vkDescription.format))
				{
					writeVertexInputValueSfloat(destPtr, attribute.vkDescription.format, componentNdx, -(0.01f * (float)(vertexInputIndex + swizzledNdx)));
				}
				else if (isVertexFormatSscaled(attribute.vkDescription.format))
				{
					writeVertexInputValueSint(destPtr, attribute.vkDescription.format, componentNdx, -(deInt32)(vertexInputIndex + swizzledNdx));
				}
				else if (isVertexFormatUscaled(attribute.vkDescription.format) || isVertexFormatUnorm(attribute.vkDescription.format) || isVertexFormatSRGB(attribute.vkDescription.format))
				{
					writeVertexInputValueUint(destPtr, attribute.vkDescription.format, componentNdx, vertexInputIndex + swizzledNdx);
				}
				else if (isVertexFormatSnorm(attribute.vkDescription.format))
				{
					const deInt32 minIntValue = -((1 << (getVertexFormatComponentSize(attribute.vkDescription.format) * 8 - 1))) + 1;
					writeVertexInputValueSint(destPtr, attribute.vkDescription.format, componentNdx, minIntValue + (vertexInputIndex + swizzledNdx));
				}
				else
					DE_ASSERT(false);
				break;

			case VertexInputTest::GLSL_TYPE_DOUBLE:
			case VertexInputTest::GLSL_TYPE_DVEC2:
			case VertexInputTest::GLSL_TYPE_DVEC3:
			case VertexInputTest::GLSL_TYPE_DVEC4:
			case VertexInputTest::GLSL_TYPE_DMAT2:
			case VertexInputTest::GLSL_TYPE_DMAT3:
			case VertexInputTest::GLSL_TYPE_DMAT4:
				*(reinterpret_cast<double *>(destPtr) + componentNdx) = -0.01 * (vertexInputIndex + swizzledNdx);

				break;

			default:
				DE_ASSERT(false);
		}
	}
}

tcu::TestStatus VertexInputInstance::iterate (void)
{
	const DeviceInterface&		vk			= m_context.getDeviceInterface();
	const VkDevice				vkDevice	= m_context.getDevice();
	const VkQueue				queue		= m_context.getUniversalQueue();
	const VkSubmitInfo			submitInfo	=
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
		DE_NULL,						// const void*				pNext;
		0u,								// deUint32					waitSemaphoreCount;
		DE_NULL,						// const VkSemaphore*		pWaitSemaphores;
		(const VkPipelineStageFlags*)DE_NULL,
		1u,								// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),				// const VkCommandBuffer*	pCommandBuffers;
		0u,								// deUint32					signalSemaphoreCount;
		DE_NULL							// const VkSemaphore*		pSignalSemaphores;
	};

	VK_CHECK(vk.resetFences(vkDevice, 1, &m_fence.get()));
	VK_CHECK(vk.queueSubmit(queue, 1, &submitInfo, *m_fence));
	VK_CHECK(vk.waitForFences(vkDevice, 1, &m_fence.get(), true, ~(0ull) /* infinity*/));

	return verifyImage();
}

bool VertexInputTest::isCompatibleType (VkFormat format, GlslType glslType)
{
	const GlslTypeDescription glslTypeDesc = s_glslTypeDescriptions[glslType];

	if ((deUint32)s_glslTypeDescriptions[glslType].vertexInputComponentCount == getVertexFormatComponentCount(format))
	{
		switch (glslTypeDesc.basicType)
		{
			case GLSL_BASIC_TYPE_INT:
				return isVertexFormatSint(format);

			case GLSL_BASIC_TYPE_UINT:
				return isVertexFormatUint(format);

			case GLSL_BASIC_TYPE_FLOAT:
				return getVertexFormatComponentSize(format) <= 4 && (isVertexFormatSfloat(format) || isVertexFormatSnorm(format) || isVertexFormatUnorm(format) || isVertexFormatSscaled(format) || isVertexFormatUscaled(format) || isVertexFormatSRGB(format));

			case GLSL_BASIC_TYPE_DOUBLE:
				return isVertexFormatSfloat(format) && getVertexFormatComponentSize(format) == 8;

			default:
				DE_ASSERT(false);
				return false;
		}
	}
	else
		return false;
}

tcu::TestStatus VertexInputInstance::verifyImage (void)
{
	bool							compareOk			= false;
	const tcu::TextureFormat		tcuColorFormat		= mapVkFormat(m_colorFormat);
	tcu::TextureLevel				reference			(tcuColorFormat, m_renderSize.x(), m_renderSize.y());
	const tcu::PixelBufferAccess	refRedSubregion		(tcu::getSubregion(reference.getAccess(),
																		   deRoundFloatToInt32((float)m_renderSize.x() * 0.0f),
																		   deRoundFloatToInt32((float)m_renderSize.y() * 0.0f),
																		   deRoundFloatToInt32((float)m_renderSize.x() * 0.5f),
																		   deRoundFloatToInt32((float)m_renderSize.y() * 1.0f)));
	const tcu::PixelBufferAccess	refBlueSubregion	(tcu::getSubregion(reference.getAccess(),
																		   deRoundFloatToInt32((float)m_renderSize.x() * 0.5f),
																		   deRoundFloatToInt32((float)m_renderSize.y() * 0.0f),
																		   deRoundFloatToInt32((float)m_renderSize.x() * 0.5f),
																		   deRoundFloatToInt32((float)m_renderSize.y() * 1.0f)));

	// Create reference image
	tcu::clear(reference.getAccess(), defaultClearColor(tcuColorFormat));
	tcu::clear(refRedSubregion, tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	tcu::clear(refBlueSubregion, tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f));

	// Compare result with reference image
	{
		const DeviceInterface&			vk					= m_context.getDeviceInterface();
		const VkDevice					vkDevice			= m_context.getDevice();
		const VkQueue					queue				= m_context.getUniversalQueue();
		const deUint32					queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
		SimpleAllocator					allocator			(vk, vkDevice, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));
		de::MovePtr<tcu::TextureLevel>	result				= readColorAttachment(vk, vkDevice, queue, queueFamilyIndex, allocator, *m_colorImage, m_colorFormat, m_renderSize);

		compareOk = tcu::intThresholdPositionDeviationCompare(m_context.getTestContext().getLog(),
															  "IntImageCompare",
															  "Image comparison",
															  reference.getAccess(),
															  result->getAccess(),
															  tcu::UVec4(2, 2, 2, 2),
															  tcu::IVec3(1, 1, 0),
															  true,
															  tcu::COMPARE_LOG_RESULT);
	}

	if (compareOk)
		return tcu::TestStatus::pass("Result image matches reference");
	else
		return tcu::TestStatus::fail("Image mismatch");
}

std::string getAttributeInfoCaseName (const VertexInputTest::AttributeInfo& attributeInfo)
{
	std::ostringstream	caseName;
	const std::string	formatName	= getFormatName(attributeInfo.vkType);

	caseName << "as_" << de::toLower(formatName.substr(10)) << "_rate_";

	if (attributeInfo.inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
		caseName <<  "vertex";
	else
		caseName <<  "instance";

	return caseName.str();
}

std::string getAttributeInfoDescription (const VertexInputTest::AttributeInfo& attributeInfo)
{
	std::ostringstream caseDesc;

	caseDesc << std::string(VertexInputTest::s_glslTypeDescriptions[attributeInfo.glslType].name) << " from type " << getFormatName(attributeInfo.vkType) <<  " with ";

	if (attributeInfo.inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
		caseDesc <<  "vertex input rate ";
	else
		caseDesc <<  "instance input rate ";

	return caseDesc.str();
}

std::string getAttributeInfosDescription (const std::vector<VertexInputTest::AttributeInfo>& attributeInfos)
{
	std::ostringstream caseDesc;

	caseDesc << "Uses vertex attributes:\n";

	for (size_t attributeNdx = 0; attributeNdx < attributeInfos.size(); attributeNdx++)
		caseDesc << "\t- " << getAttributeInfoDescription (attributeInfos[attributeNdx]) << "\n";

	return caseDesc.str();
}

struct CompatibleFormats
{
	VertexInputTest::GlslType	glslType;
	std::vector<VkFormat>		compatibleVkFormats;
};

void createSingleAttributeCases (tcu::TestCaseGroup* singleAttributeTests, VertexInputTest::GlslType glslType)
{
	const VkFormat vertexFormats[] =
	{
		// Required, unpacked
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,

		// Scaled formats
		VK_FORMAT_R8G8_USCALED,
		VK_FORMAT_R8G8_SSCALED,
		VK_FORMAT_R16_USCALED,
		VK_FORMAT_R16_SSCALED,
		VK_FORMAT_R8G8B8_USCALED,
		VK_FORMAT_R8G8B8_SSCALED,
		VK_FORMAT_B8G8R8_USCALED,
		VK_FORMAT_B8G8R8_SSCALED,
		VK_FORMAT_R8G8B8A8_USCALED,
		VK_FORMAT_R8G8B8A8_SSCALED,
		VK_FORMAT_B8G8R8A8_USCALED,
		VK_FORMAT_B8G8R8A8_SSCALED,
		VK_FORMAT_R16G16_USCALED,
		VK_FORMAT_R16G16_SSCALED,
		VK_FORMAT_R16G16B16_USCALED,
		VK_FORMAT_R16G16B16_SSCALED,
		VK_FORMAT_R16G16B16A16_USCALED,
		VK_FORMAT_R16G16B16A16_SSCALED,

		// SRGB formats
		VK_FORMAT_R8_SRGB,
		VK_FORMAT_R8G8_SRGB,
		VK_FORMAT_R8G8B8_SRGB,
		VK_FORMAT_B8G8R8_SRGB,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_SRGB,

		// Double formats
		VK_FORMAT_R64_SFLOAT,
		VK_FORMAT_R64G64_SFLOAT,
		VK_FORMAT_R64G64B64_SFLOAT,
		VK_FORMAT_R64G64B64A64_SFLOAT,
	};

	for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(vertexFormats); formatNdx++)
	{
		if (VertexInputTest::isCompatibleType(vertexFormats[formatNdx], glslType))
		{
			// Create test case for RATE_VERTEX
			VertexInputTest::AttributeInfo attributeInfo;
			attributeInfo.vkType = vertexFormats[formatNdx];
			attributeInfo.glslType = glslType;
			attributeInfo.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			singleAttributeTests->addChild(new VertexInputTest(singleAttributeTests->getTestContext(),
															   getAttributeInfoCaseName(attributeInfo),
															   getAttributeInfoDescription(attributeInfo),
															   std::vector<VertexInputTest::AttributeInfo>(1, attributeInfo),
															   VertexInputTest::BINDING_MAPPING_ONE_TO_ONE,
															   VertexInputTest::ATTRIBUTE_LAYOUT_INTERLEAVED));

			// Create test case for RATE_INSTANCE
			attributeInfo.inputRate	= VK_VERTEX_INPUT_RATE_INSTANCE;

			singleAttributeTests->addChild(new VertexInputTest(singleAttributeTests->getTestContext(),
															   getAttributeInfoCaseName(attributeInfo),
															   getAttributeInfoDescription(attributeInfo),
															   std::vector<VertexInputTest::AttributeInfo>(1, attributeInfo),
															   VertexInputTest::BINDING_MAPPING_ONE_TO_ONE,
															   VertexInputTest::ATTRIBUTE_LAYOUT_INTERLEAVED));
		}
	}
}

void createSingleAttributeTests (tcu::TestCaseGroup* singleAttributeTests)
{
	for (int glslTypeNdx = 0; glslTypeNdx < VertexInputTest::GLSL_TYPE_COUNT; glslTypeNdx++)
	{
		VertexInputTest::GlslType glslType = (VertexInputTest::GlslType)glslTypeNdx;
		addTestGroup(singleAttributeTests, VertexInputTest::s_glslTypeDescriptions[glslType].name, "", createSingleAttributeCases, glslType);
	}
}

// Create all unique GlslType combinations recursively
void createMultipleAttributeCases (deUint32 depth, deUint32 firstNdx, CompatibleFormats* compatibleFormats, de::Random& randomFunc, tcu::TestCaseGroup& testGroup, VertexInputTest::BindingMapping bindingMapping, VertexInputTest::AttributeLayout attributeLayout, const std::vector<VertexInputTest::AttributeInfo>& attributeInfos = std::vector<VertexInputTest::AttributeInfo>(0))
{
	tcu::TestContext& testCtx = testGroup.getTestContext();

	// Exclude double values, which are not included in vertexFormats
	for (deUint32 currentNdx = firstNdx; currentNdx < VertexInputTest::GLSL_TYPE_DOUBLE - depth; currentNdx++)
	{
		std::vector <VertexInputTest::AttributeInfo> newAttributeInfos = attributeInfos;

		{
			VertexInputTest::AttributeInfo attributeInfo;
			attributeInfo.glslType	= (VertexInputTest::GlslType)currentNdx;
			attributeInfo.inputRate	= (depth % 2 == 0) ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
			attributeInfo.vkType	= VK_FORMAT_UNDEFINED;

			newAttributeInfos.push_back(attributeInfo);
		}

		// Add test case
		if (depth == 0)
		{
			// Select a random compatible format for each attribute
			for (size_t i = 0; i < newAttributeInfos.size(); i++)
			{
				const std::vector<VkFormat>& formats = compatibleFormats[newAttributeInfos[i].glslType].compatibleVkFormats;
				newAttributeInfos[i].vkType = formats[randomFunc.getUint32() % formats.size()];
			}

			const std::string caseName = VertexInputTest::s_glslTypeDescriptions[currentNdx].name;
			const std::string caseDesc = getAttributeInfosDescription(newAttributeInfos);

			testGroup.addChild(new VertexInputTest(testCtx, caseName, caseDesc, newAttributeInfos, bindingMapping, attributeLayout));
		}
		// Add test group
		else
		{
			const std::string				name			= VertexInputTest::s_glslTypeDescriptions[currentNdx].name;
			de::MovePtr<tcu::TestCaseGroup>	newTestGroup	(new tcu::TestCaseGroup(testCtx, name.c_str(), ""));

			createMultipleAttributeCases(depth - 1u, currentNdx + 1u, compatibleFormats, randomFunc, *newTestGroup, bindingMapping, attributeLayout, newAttributeInfos);
			testGroup.addChild(newTestGroup.release());
		}
	}
}

void createMultipleAttributeTests (tcu::TestCaseGroup* multipleAttributeTests)
{
	// Required vertex formats, unpacked
	const VkFormat vertexFormats[] =
	{
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT
	};

	// Find compatible VK formats for each GLSL vertex type
	CompatibleFormats compatibleFormats[VertexInputTest::GLSL_TYPE_COUNT];
	{
		for (int glslTypeNdx = 0; glslTypeNdx < VertexInputTest::GLSL_TYPE_COUNT; glslTypeNdx++)
		{
			for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(vertexFormats); formatNdx++)
			{
				if (VertexInputTest::isCompatibleType(vertexFormats[formatNdx], (VertexInputTest::GlslType)glslTypeNdx))
					compatibleFormats[glslTypeNdx].compatibleVkFormats.push_back(vertexFormats[formatNdx]);
			}
		}
	}

	de::Random                      randomFunc(102030);
	tcu::TestContext&				testCtx = multipleAttributeTests->getTestContext();
	de::MovePtr<tcu::TestCaseGroup> oneToOneAttributeTests(new tcu::TestCaseGroup(testCtx, "attributes", ""));
	de::MovePtr<tcu::TestCaseGroup> oneToManyAttributeTests(new tcu::TestCaseGroup(testCtx, "attributes", ""));
	de::MovePtr<tcu::TestCaseGroup> oneToManySequentialAttributeTests(new tcu::TestCaseGroup(testCtx, "attributes_sequential", ""));

	createMultipleAttributeCases(2u, 0u, compatibleFormats, randomFunc, *oneToOneAttributeTests,			VertexInputTest::BINDING_MAPPING_ONE_TO_ONE,	VertexInputTest::ATTRIBUTE_LAYOUT_INTERLEAVED);
	createMultipleAttributeCases(2u, 0u, compatibleFormats, randomFunc, *oneToManyAttributeTests,			VertexInputTest::BINDING_MAPPING_ONE_TO_MANY,	VertexInputTest::ATTRIBUTE_LAYOUT_INTERLEAVED);
	createMultipleAttributeCases(2u, 0u, compatibleFormats, randomFunc, *oneToManySequentialAttributeTests,	VertexInputTest::BINDING_MAPPING_ONE_TO_MANY,	VertexInputTest::ATTRIBUTE_LAYOUT_SEQUENTIAL);

	de::MovePtr<tcu::TestCaseGroup> bindingOneToOneTests(new tcu::TestCaseGroup(testCtx, "binding_one_to_one", "Each attribute uses a unique binding"));
	bindingOneToOneTests->addChild(oneToOneAttributeTests.release());
	multipleAttributeTests->addChild(bindingOneToOneTests.release());

	de::MovePtr<tcu::TestCaseGroup> bindingOneToManyTests(new tcu::TestCaseGroup(testCtx, "binding_one_to_many", "Attributes share the same binding"));
	bindingOneToManyTests->addChild(oneToManyAttributeTests.release());
	bindingOneToManyTests->addChild(oneToManySequentialAttributeTests.release());
	multipleAttributeTests->addChild(bindingOneToManyTests.release());
}

void createMaxAttributeTests (tcu::TestCaseGroup* maxAttributeTests)
{
	// Required vertex formats, unpacked
	const VkFormat					vertexFormats[]		=
	{
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT
	};

	// VkPhysicalDeviceLimits::maxVertexInputAttributes is used when attributeCount is 0
	const deUint32					attributeCount[]	= { 16, 32, 64, 128, 0 };
	tcu::TestContext&				testCtx				(maxAttributeTests->getTestContext());
	de::Random						randomFunc			(132030);

	// Find compatible VK formats for each GLSL vertex type
	CompatibleFormats compatibleFormats[VertexInputTest::GLSL_TYPE_COUNT];
	{
		for (int glslTypeNdx = 0; glslTypeNdx < VertexInputTest::GLSL_TYPE_COUNT; glslTypeNdx++)
		{
			for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(vertexFormats); formatNdx++)
			{
				if (VertexInputTest::isCompatibleType(vertexFormats[formatNdx], (VertexInputTest::GlslType)glslTypeNdx))
					compatibleFormats[glslTypeNdx].compatibleVkFormats.push_back(vertexFormats[formatNdx]);
			}
		}
	}

	for (deUint32 attributeCountNdx = 0; attributeCountNdx < DE_LENGTH_OF_ARRAY(attributeCount); attributeCountNdx++)
	{
		const std::string							groupName = (attributeCount[attributeCountNdx] == 0 ? "query_max" : de::toString(attributeCount[attributeCountNdx])) + "_attributes";
		const std::string							groupDesc = de::toString(attributeCount[attributeCountNdx]) + " vertex input attributes";

		de::MovePtr<tcu::TestCaseGroup>				numAttributeTests(new tcu::TestCaseGroup(testCtx, groupName.c_str(), groupDesc.c_str()));
		de::MovePtr<tcu::TestCaseGroup>				bindingOneToOneTests(new tcu::TestCaseGroup(testCtx, "binding_one_to_one", "Each attribute uses a unique binding"));
		de::MovePtr<tcu::TestCaseGroup>				bindingOneToManyTests(new tcu::TestCaseGroup(testCtx, "binding_one_to_many", "Attributes share the same binding"));

		std::vector<VertexInputTest::AttributeInfo>	attributeInfos(attributeCount[attributeCountNdx]);

		for (deUint32 attributeNdx = 0; attributeNdx < attributeCount[attributeCountNdx]; attributeNdx++)
		{
			// Use random glslTypes, each consuming one attribute location
			const VertexInputTest::GlslType	glslType	= (VertexInputTest::GlslType)(randomFunc.getUint32() % VertexInputTest::GLSL_TYPE_MAT2);
			const std::vector<VkFormat>&	formats		= compatibleFormats[glslType].compatibleVkFormats;
			const VkFormat					format		= formats[randomFunc.getUint32() % formats.size()];

			attributeInfos[attributeNdx].glslType		= glslType;
			attributeInfos[attributeNdx].inputRate		= ((attributeCountNdx + attributeNdx) % 2 == 0) ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
			attributeInfos[attributeNdx].vkType			= format;
		}

		bindingOneToOneTests->addChild(new VertexInputTest(testCtx, "interleaved", "Interleaved attribute layout", attributeInfos, VertexInputTest::BINDING_MAPPING_ONE_TO_ONE, VertexInputTest::ATTRIBUTE_LAYOUT_INTERLEAVED));
		bindingOneToManyTests->addChild(new VertexInputTest(testCtx, "interleaved", "Interleaved attribute layout", attributeInfos, VertexInputTest::BINDING_MAPPING_ONE_TO_MANY, VertexInputTest::ATTRIBUTE_LAYOUT_INTERLEAVED));
		bindingOneToManyTests->addChild(new VertexInputTest(testCtx, "sequential", "Sequential attribute layout", attributeInfos, VertexInputTest::BINDING_MAPPING_ONE_TO_MANY, VertexInputTest::ATTRIBUTE_LAYOUT_SEQUENTIAL));

		numAttributeTests->addChild(bindingOneToOneTests.release());
		numAttributeTests->addChild(bindingOneToManyTests.release());
		maxAttributeTests->addChild(numAttributeTests.release());
	}
}

} // anonymous

void createVertexInputTests (tcu::TestCaseGroup* vertexInputTests)
{
	addTestGroup(vertexInputTests, "single_attribute", "Uses one attribute", createSingleAttributeTests);
	addTestGroup(vertexInputTests, "multiple_attributes", "Uses more than one attribute", createMultipleAttributeTests);
	addTestGroup(vertexInputTests, "max_attributes", "Implementations can use as many vertex input attributes as they advertise", createMaxAttributeTests);
}

} // pipeline
} // vkt
