/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 Google Inc.
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
 * \brief Graphics pipeline for SPIR-V assembly tests
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmGraphicsShaderTestUtil.hpp"

#include "tcuFloat.hpp"
#include "tcuStringTemplate.hpp"

#include "vkDefs.hpp"
#include "vkMemUtil.hpp"
#include "vkPlatform.hpp"
#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deRandom.hpp"

namespace vkt
{
namespace SpirVAssembly
{

using namespace vk;
using std::map;
using std::string;
using std::vector;
using tcu::Float16;
using tcu::Float32;
using tcu::IVec3;
using tcu::IVec4;
using tcu::RGBA;
using tcu::TestLog;
using tcu::TestStatus;
using tcu::Vec4;
using de::UniquePtr;
using tcu::StringTemplate;
using tcu::Vec4;

deUint32 IFDataType::getElementNumBytes (void) const
{
	if (elementType < NUMBERTYPE_END32)
		return 4;

	return 2;
}

VkFormat IFDataType::getVkFormat (void) const
{
	if (numElements == 1)
	{
		switch (elementType)
		{
			case NUMBERTYPE_FLOAT32:	return VK_FORMAT_R32_SFLOAT;
			case NUMBERTYPE_INT32:		return VK_FORMAT_R32_SINT;
			case NUMBERTYPE_UINT32:		return VK_FORMAT_R32_UINT;
			case NUMBERTYPE_FLOAT16:	return VK_FORMAT_R16_SFLOAT;
			case NUMBERTYPE_INT16:		return VK_FORMAT_R16_SINT;
			case NUMBERTYPE_UINT16:		return VK_FORMAT_R16_UINT;
			default:					break;
		}
	}
	else if (numElements == 2)
	{
		switch (elementType)
		{
			case NUMBERTYPE_FLOAT32:	return VK_FORMAT_R32G32_SFLOAT;
			case NUMBERTYPE_INT32:		return VK_FORMAT_R32G32_SINT;
			case NUMBERTYPE_UINT32:		return VK_FORMAT_R32G32_UINT;
			case NUMBERTYPE_FLOAT16:	return VK_FORMAT_R16G16_SFLOAT;
			case NUMBERTYPE_INT16:		return VK_FORMAT_R16G16_SINT;
			case NUMBERTYPE_UINT16:		return VK_FORMAT_R16G16_UINT;
			default:					break;
		}
	}
	else if (numElements == 3)
	{
		switch (elementType)
		{
			case NUMBERTYPE_FLOAT32:	return VK_FORMAT_R32G32B32_SFLOAT;
			case NUMBERTYPE_INT32:		return VK_FORMAT_R32G32B32_SINT;
			case NUMBERTYPE_UINT32:		return VK_FORMAT_R32G32B32_UINT;
			case NUMBERTYPE_FLOAT16:	return VK_FORMAT_R16G16B16_SFLOAT;
			case NUMBERTYPE_INT16:		return VK_FORMAT_R16G16B16_SINT;
			case NUMBERTYPE_UINT16:		return VK_FORMAT_R16G16B16_UINT;
			default:					break;
		}
	}
	else if (numElements == 4)
	{
		switch (elementType)
		{
			case NUMBERTYPE_FLOAT32:	return VK_FORMAT_R32G32B32A32_SFLOAT;
			case NUMBERTYPE_INT32:		return VK_FORMAT_R32G32B32A32_SINT;
			case NUMBERTYPE_UINT32:		return VK_FORMAT_R32G32B32A32_UINT;
			case NUMBERTYPE_FLOAT16:	return VK_FORMAT_R16G16B16A16_SFLOAT;
			case NUMBERTYPE_INT16:		return VK_FORMAT_R16G16B16A16_SINT;
			case NUMBERTYPE_UINT16:		return VK_FORMAT_R16G16B16A16_UINT;
			default:					break;
		}
	}

	DE_ASSERT(false);
	return VK_FORMAT_UNDEFINED;
}

tcu::TextureFormat IFDataType::getTextureFormat (void) const
{
	tcu::TextureFormat::ChannelType		ct	= tcu::TextureFormat::CHANNELTYPE_LAST;
	tcu::TextureFormat::ChannelOrder	co	= tcu::TextureFormat::CHANNELORDER_LAST;

	switch (elementType)
	{
		case NUMBERTYPE_FLOAT32:	ct = tcu::TextureFormat::FLOAT;				break;
		case NUMBERTYPE_INT32:		ct = tcu::TextureFormat::SIGNED_INT32;		break;
		case NUMBERTYPE_UINT32:		ct = tcu::TextureFormat::UNSIGNED_INT32;	break;
		case NUMBERTYPE_FLOAT16:	ct = tcu::TextureFormat::HALF_FLOAT;		break;
		case NUMBERTYPE_INT16:		ct = tcu::TextureFormat::SIGNED_INT16;		break;
		case NUMBERTYPE_UINT16:		ct = tcu::TextureFormat::UNSIGNED_INT16;	break;
		default:					DE_ASSERT(false);
	}

	switch (numElements)
	{
		case 1:				co = tcu::TextureFormat::R;					break;
		case 2:				co = tcu::TextureFormat::RG;				break;
		case 3:				co = tcu::TextureFormat::RGB;				break;
		case 4:				co = tcu::TextureFormat::RGBA;				break;
		default:			DE_ASSERT(false);
	}

	return tcu::TextureFormat(co, ct);
}

string IFDataType::str (void) const
{
	string	ret;

	switch (elementType)
	{
		case NUMBERTYPE_FLOAT32:	ret = "f32"; break;
		case NUMBERTYPE_INT32:		ret = "i32"; break;
		case NUMBERTYPE_UINT32:		ret = "u32"; break;
		case NUMBERTYPE_FLOAT16:	ret = "f16"; break;
		case NUMBERTYPE_INT16:		ret = "i16"; break;
		case NUMBERTYPE_UINT16:		ret = "u16"; break;
		default:					DE_ASSERT(false);
	}

	if (numElements == 1)
		return ret;

	return string("v") + numberToString(numElements) + ret;
}

VkBufferUsageFlagBits getMatchingBufferUsageFlagBit(VkDescriptorType dType)
{
	switch (dType)
	{
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:			return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:			return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:			return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:			return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:	return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		default:										DE_ASSERT(0 && "not implemented");
	}
	return (VkBufferUsageFlagBits)0;
}

VkImageUsageFlags getMatchingImageUsageFlags(VkDescriptorType dType)
{
	switch (dType)
	{
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:			return VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:			return VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:	return VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		default:										DE_FATAL("Not implemented");
	}
	return (VkImageUsageFlags)0;
}

static void requireFormatUsageSupport(const InstanceInterface& vki, VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling imageTiling, VkImageUsageFlags requiredUsageFlags)
{
	VkFormatProperties		properties;
	VkFormatFeatureFlags	tilingFeatures	= 0;

	vki.getPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

	switch (imageTiling)
	{
		case VK_IMAGE_TILING_LINEAR:
			tilingFeatures = properties.linearTilingFeatures;
			break;

		case VK_IMAGE_TILING_OPTIMAL:
			tilingFeatures = properties.optimalTilingFeatures;
			break;

		default:
			DE_ASSERT(false);
			break;
	}

	if ((requiredUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0)
	{
		if ((tilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)
			TCU_THROW(NotSupportedError, "Image format cannot be used as color attachment");
		requiredUsageFlags ^= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}


	if ((requiredUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0)
	{
		if ((tilingFeatures & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR) == 0)
			TCU_THROW(NotSupportedError, "Image format cannot be used as transfer source");
		requiredUsageFlags ^= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}


	DE_ASSERT(!requiredUsageFlags && "checking other image usage bits not supported yet");
}

InstanceContext::InstanceContext (const RGBA						(&inputs)[4],
								  const RGBA						(&outputs)[4],
								  const map<string, string>&		testCodeFragments_,
								  const StageToSpecConstantMap&		specConstants_,
								  const PushConstants&				pushConsants_,
								  const GraphicsResources&			resources_,
								  const GraphicsInterfaces&			interfaces_,
								  const vector<string>&				extensions_,
								  const vector<string>&				features_,
								  VulkanFeatures					vulkanFeatures_,
								  VkShaderStageFlags				customizedStages_)
	: testCodeFragments				(testCodeFragments_)
	, specConstants					(specConstants_)
	, hasTessellation				(false)
	, requiredStages				(static_cast<VkShaderStageFlagBits>(0))
	, requiredDeviceExtensions		(extensions_)
	, requiredDeviceFeatures		(features_)
	, requestedFeatures				(vulkanFeatures_)
	, pushConstants					(pushConsants_)
	, customizedStages				(customizedStages_)
	, resources						(resources_)
	, interfaces					(interfaces_)
	, failResult					(QP_TEST_RESULT_FAIL)
	, failMessageTemplate			("${reason}")
{
	inputColors[0]		= inputs[0];
	inputColors[1]		= inputs[1];
	inputColors[2]		= inputs[2];
	inputColors[3]		= inputs[3];

	outputColors[0]		= outputs[0];
	outputColors[1]		= outputs[1];
	outputColors[2]		= outputs[2];
	outputColors[3]		= outputs[3];
}

InstanceContext::InstanceContext (const InstanceContext& other)
	: moduleMap						(other.moduleMap)
	, testCodeFragments				(other.testCodeFragments)
	, specConstants					(other.specConstants)
	, hasTessellation				(other.hasTessellation)
	, requiredStages				(other.requiredStages)
	, requiredDeviceExtensions		(other.requiredDeviceExtensions)
	, requiredDeviceFeatures		(other.requiredDeviceFeatures)
	, requestedFeatures				(other.requestedFeatures)
	, pushConstants					(other.pushConstants)
	, customizedStages				(other.customizedStages)
	, resources						(other.resources)
	, interfaces					(other.interfaces)
	, failResult					(other.failResult)
	, failMessageTemplate			(other.failMessageTemplate)
{
	inputColors[0]		= other.inputColors[0];
	inputColors[1]		= other.inputColors[1];
	inputColors[2]		= other.inputColors[2];
	inputColors[3]		= other.inputColors[3];

	outputColors[0]		= other.outputColors[0];
	outputColors[1]		= other.outputColors[1];
	outputColors[2]		= other.outputColors[2];
	outputColors[3]		= other.outputColors[3];
}

string InstanceContext::getSpecializedFailMessage (const string& failureReason)
{
	map<string, string>		parameters;
	parameters["reason"]	= failureReason;
	return StringTemplate(failMessageTemplate).specialize(parameters);
}

ShaderElement::ShaderElement (const string&				moduleName_,
							  const string&				entryPoint_,
							  VkShaderStageFlagBits		shaderStage_)
		: moduleName(moduleName_)
		, entryName(entryPoint_)
		, stage(shaderStage_)
{
}

void getDefaultColors (RGBA (&colors)[4])
{
	colors[0] = RGBA::white();
	colors[1] = RGBA::red();
	colors[2] = RGBA::green();
	colors[3] = RGBA::blue();
}

void getHalfColorsFullAlpha (RGBA (&colors)[4])
{
	colors[0] = RGBA(127, 127, 127, 255);
	colors[1] = RGBA(127, 0,   0,	255);
	colors[2] = RGBA(0,	  127, 0,	255);
	colors[3] = RGBA(0,	  0,   127, 255);
}

void getInvertedDefaultColors (RGBA (&colors)[4])
{
	colors[0] = RGBA(0,		0,		0,		255);
	colors[1] = RGBA(0,		255,	255,	255);
	colors[2] = RGBA(255,	0,		255,	255);
	colors[3] = RGBA(255,	255,	0,		255);
}

// For the current InstanceContext, constructs the required modules and shader stage create infos.
void createPipelineShaderStages (const DeviceInterface&						vk,
								 const VkDevice								vkDevice,
								 InstanceContext&							instance,
								 Context&									context,
								 vector<ModuleHandleSp>&					modules,
								 vector<VkPipelineShaderStageCreateInfo>&	createInfos)
{
	for (ModuleMap::const_iterator moduleNdx = instance.moduleMap.begin(); moduleNdx != instance.moduleMap.end(); ++moduleNdx)
	{
		const ModuleHandleSp mod(new Unique<VkShaderModule>(createShaderModule(vk, vkDevice, context.getBinaryCollection().get(moduleNdx->first), 0)));
		modules.push_back(ModuleHandleSp(mod));
		for (vector<EntryToStage>::const_iterator shaderNdx = moduleNdx->second.begin(); shaderNdx != moduleNdx->second.end(); ++shaderNdx)
		{
			const EntryToStage&						stage			= *shaderNdx;
			const VkPipelineShaderStageCreateInfo	shaderParam		=
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	//	VkStructureType			sType;
				DE_NULL,												//	const void*				pNext;
				(VkPipelineShaderStageCreateFlags)0,
				stage.second,											//	VkShaderStageFlagBits	stage;
				**modules.back(),										//	VkShaderModule			module;
				stage.first.c_str(),									//	const char*				pName;
				(const VkSpecializationInfo*)DE_NULL,
			};
			createInfos.push_back(shaderParam);
		}
	}
}

#define SPIRV_ASSEMBLY_TYPES																	\
	"%void = OpTypeVoid\n"																		\
	"%bool = OpTypeBool\n"																		\
																								\
	"%i32 = OpTypeInt 32 1\n"																	\
	"%u32 = OpTypeInt 32 0\n"																	\
																								\
	"%f32 = OpTypeFloat 32\n"																	\
	"%v2i32 = OpTypeVector %i32 2\n"															\
	"%v2u32 = OpTypeVector %u32 2\n"															\
	"%v2f32 = OpTypeVector %f32 2\n"															\
	"%v3f32 = OpTypeVector %f32 3\n"															\
	"%v4i32 = OpTypeVector %i32 4\n"															\
	"%v4u32 = OpTypeVector %u32 4\n"															\
	"%v4f32 = OpTypeVector %f32 4\n"															\
	"%v4bool = OpTypeVector %bool 4\n"															\
																								\
	"%v4f32_function = OpTypeFunction %v4f32 %v4f32\n"											\
	"%bool_function = OpTypeFunction %bool\n"													\
	"%fun = OpTypeFunction %void\n"																\
																								\
	"%ip_f32 = OpTypePointer Input %f32\n"														\
	"%ip_i32 = OpTypePointer Input %i32\n"														\
	"%ip_u32 = OpTypePointer Input %u32\n"														\
	"%ip_v3f32 = OpTypePointer Input %v3f32\n"													\
	"%ip_v2f32 = OpTypePointer Input %v2f32\n"													\
	"%ip_v2i32 = OpTypePointer Input %v2i32\n"													\
	"%ip_v2u32 = OpTypePointer Input %v2u32\n"													\
	"%ip_v4f32 = OpTypePointer Input %v4f32\n"													\
	"%ip_v4i32 = OpTypePointer Input %v4i32\n"													\
	"%ip_v4u32 = OpTypePointer Input %v4u32\n"													\
																								\
	"%op_f32 = OpTypePointer Output %f32\n"														\
	"%op_i32 = OpTypePointer Output %i32\n"														\
	"%op_u32 = OpTypePointer Output %u32\n"														\
	"%op_v2f32 = OpTypePointer Output %v2f32\n"													\
	"%op_v2i32 = OpTypePointer Output %v2i32\n"													\
	"%op_v2u32 = OpTypePointer Output %v2u32\n"													\
	"%op_v4f32 = OpTypePointer Output %v4f32\n"													\
	"%op_v4i32 = OpTypePointer Output %v4i32\n"													\
	"%op_v4u32 = OpTypePointer Output %v4u32\n"													\
																								\
	"%fp_f32   = OpTypePointer Function %f32\n"													\
	"%fp_i32   = OpTypePointer Function %i32\n"													\
	"%fp_v4f32 = OpTypePointer Function %v4f32\n"

#define SPIRV_ASSEMBLY_CONSTANTS																\
	"%c_f32_1 = OpConstant %f32 1.0\n"															\
	"%c_f32_0 = OpConstant %f32 0.0\n"															\
	"%c_f32_0_5 = OpConstant %f32 0.5\n"														\
	"%c_f32_n1  = OpConstant %f32 -1.\n"														\
	"%c_f32_7 = OpConstant %f32 7.0\n"															\
	"%c_f32_8 = OpConstant %f32 8.0\n"															\
	"%c_i32_0 = OpConstant %i32 0\n"															\
	"%c_i32_1 = OpConstant %i32 1\n"															\
	"%c_i32_2 = OpConstant %i32 2\n"															\
	"%c_i32_3 = OpConstant %i32 3\n"															\
	"%c_i32_4 = OpConstant %i32 4\n"															\
	"%c_u32_0 = OpConstant %u32 0\n"															\
	"%c_u32_1 = OpConstant %u32 1\n"															\
	"%c_u32_2 = OpConstant %u32 2\n"															\
	"%c_u32_3 = OpConstant %u32 3\n"															\
	"%c_u32_32 = OpConstant %u32 32\n"															\
	"%c_u32_4 = OpConstant %u32 4\n"															\
	"%c_u32_31_bits = OpConstant %u32 0x7FFFFFFF\n"												\
	"%c_v4f32_1_1_1_1 = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_1\n"		\
	"%c_v4f32_1_0_0_1 = OpConstantComposite %v4f32 %c_f32_1 %c_f32_0 %c_f32_0 %c_f32_1\n"		\
	"%c_v4f32_0_5_0_5_0_5_0_5 = OpConstantComposite %v4f32 %c_f32_0_5 %c_f32_0_5 %c_f32_0_5 %c_f32_0_5\n"

#define SPIRV_ASSEMBLY_ARRAYS																	\
	"%a1f32 = OpTypeArray %f32 %c_u32_1\n"														\
	"%a2f32 = OpTypeArray %f32 %c_u32_2\n"														\
	"%a3v4f32 = OpTypeArray %v4f32 %c_u32_3\n"													\
	"%a4f32 = OpTypeArray %f32 %c_u32_4\n"														\
	"%a32v4f32 = OpTypeArray %v4f32 %c_u32_32\n"												\
	"%ip_a3v4f32 = OpTypePointer Input %a3v4f32\n"												\
	"%ip_a32v4f32 = OpTypePointer Input %a32v4f32\n"											\
	"%op_a2f32 = OpTypePointer Output %a2f32\n"													\
	"%op_a3v4f32 = OpTypePointer Output %a3v4f32\n"												\
	"%op_a4f32 = OpTypePointer Output %a4f32\n"

// Creates vertex-shader assembly by specializing a boilerplate StringTemplate
// on fragments, which must (at least) map "testfun" to an OpFunction definition
// for %test_code that takes and returns a %v4f32.  Boilerplate IDs are prefixed
// with "BP_" to avoid collisions with fragments.
//
// It corresponds roughly to this GLSL:
//;
// layout(location = 0) in vec4 position;
// layout(location = 1) in vec4 color;
// layout(location = 1) out highp vec4 vtxColor;
// void main (void) { gl_Position = position; vtxColor = test_func(color); }
string makeVertexShaderAssembly(const map<string, string>& fragments)
{
// \todo [2015-11-23 awoloszyn] Remove OpName once these have stabalized
	static const char vertexShaderBoilerplate[] =
		"OpCapability Shader\n"
		"OpCapability ClipDistance\n"
		"OpCapability CullDistance\n"
		"${capability:opt}\n"
		"${extension:opt}\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint Vertex %main \"main\" %BP_stream %BP_position %BP_vtx_color %BP_color %BP_gl_VertexIndex %BP_gl_InstanceIndex ${IF_entrypoint:opt} \n"
		"${debug:opt}\n"
		"OpName %main \"main\"\n"
		"OpName %BP_gl_PerVertex \"gl_PerVertex\"\n"
		"OpMemberName %BP_gl_PerVertex 0 \"gl_Position\"\n"
		"OpMemberName %BP_gl_PerVertex 1 \"gl_PointSize\"\n"
		"OpMemberName %BP_gl_PerVertex 2 \"gl_ClipDistance\"\n"
		"OpMemberName %BP_gl_PerVertex 3 \"gl_CullDistance\"\n"
		"OpName %test_code \"testfun(vf4;\"\n"
		"OpName %BP_stream \"\"\n"
		"OpName %BP_position \"position\"\n"
		"OpName %BP_vtx_color \"vtxColor\"\n"
		"OpName %BP_color \"color\"\n"
		"OpName %BP_gl_VertexIndex \"gl_VertexIndex\"\n"
		"OpName %BP_gl_InstanceIndex \"gl_InstanceIndex\"\n"
		"OpMemberDecorate %BP_gl_PerVertex 0 BuiltIn Position\n"
		"OpMemberDecorate %BP_gl_PerVertex 1 BuiltIn PointSize\n"
		"OpMemberDecorate %BP_gl_PerVertex 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %BP_gl_PerVertex 3 BuiltIn CullDistance\n"
		"OpDecorate %BP_gl_PerVertex Block\n"
		"OpDecorate %BP_position Location 0\n"
		"OpDecorate %BP_vtx_color Location 1\n"
		"OpDecorate %BP_color Location 1\n"
		"OpDecorate %BP_gl_VertexIndex BuiltIn VertexIndex\n"
		"OpDecorate %BP_gl_InstanceIndex BuiltIn InstanceIndex\n"
		"${IF_decoration:opt}\n"
		"${decoration:opt}\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%BP_gl_PerVertex = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%BP_op_gl_PerVertex = OpTypePointer Output %BP_gl_PerVertex\n"
		"%BP_stream = OpVariable %BP_op_gl_PerVertex Output\n"
		"%BP_position = OpVariable %ip_v4f32 Input\n"
		"%BP_vtx_color = OpVariable %op_v4f32 Output\n"
		"%BP_color = OpVariable %ip_v4f32 Input\n"
		"%BP_gl_VertexIndex = OpVariable %ip_i32 Input\n"
		"%BP_gl_InstanceIndex = OpVariable %ip_i32 Input\n"
		"${pre_main:opt}\n"
		"${IF_variable:opt}\n"
		"%main = OpFunction %void None %fun\n"
		"%BP_label = OpLabel\n"
		"${IF_carryforward:opt}\n"
		"%BP_pos = OpLoad %v4f32 %BP_position\n"
		"%BP_gl_pos = OpAccessChain %op_v4f32 %BP_stream %c_i32_0\n"
		"OpStore %BP_gl_pos %BP_pos\n"
		"%BP_col = OpLoad %v4f32 %BP_color\n"
		"%BP_col_transformed = OpFunctionCall %v4f32 %test_code %BP_col\n"
		"OpStore %BP_vtx_color %BP_col_transformed\n"
		"OpReturn\n"
		"OpFunctionEnd\n"
		"${interface_op_func:opt}\n"

		"%isUniqueIdZero = OpFunction %bool None %bool_function\n"
		"%getId_label = OpLabel\n"
		"%vert_id = OpLoad %i32 %BP_gl_VertexIndex\n"
		"%is_id_0 = OpIEqual %bool %vert_id %c_i32_0\n"
		"OpReturnValue %is_id_0\n"
		"OpFunctionEnd\n"

		"${testfun}\n";
	return tcu::StringTemplate(vertexShaderBoilerplate).specialize(fragments);
}

// Creates tess-control-shader assembly by specializing a boilerplate
// StringTemplate on fragments, which must (at least) map "testfun" to an
// OpFunction definition for %test_code that takes and returns a %v4f32.
// Boilerplate IDs are prefixed with "BP_" to avoid collisions with fragments.
//
// It roughly corresponds to the following GLSL.
//
// #version 450
// layout(vertices = 3) out;
// layout(location = 1) in vec4 in_color[];
// layout(location = 1) out vec4 out_color[];
//
// void main() {
//   out_color[gl_InvocationID] = testfun(in_color[gl_InvocationID]);
//   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
//   if (gl_InvocationID == 0) {
//     gl_TessLevelOuter[0] = 1.0;
//     gl_TessLevelOuter[1] = 1.0;
//     gl_TessLevelOuter[2] = 1.0;
//     gl_TessLevelInner[0] = 1.0;
//   }
// }
string makeTessControlShaderAssembly (const map<string, string>& fragments)
{
	static const char tessControlShaderBoilerplate[] =
		"OpCapability Tessellation\n"
		"OpCapability ClipDistance\n"
		"OpCapability CullDistance\n"
		"${capability:opt}\n"
		"${extension:opt}\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint TessellationControl %BP_main \"main\" %BP_out_color %BP_gl_InvocationID %BP_gl_PrimitiveID %BP_in_color %BP_gl_out %BP_gl_in %BP_gl_TessLevelOuter %BP_gl_TessLevelInner ${IF_entrypoint:opt} \n"
		"OpExecutionMode %BP_main OutputVertices 3\n"
		"${debug:opt}\n"
		"OpName %BP_main \"main\"\n"
		"OpName %test_code \"testfun(vf4;\"\n"
		"OpName %BP_out_color \"out_color\"\n"
		"OpName %BP_gl_InvocationID \"gl_InvocationID\"\n"
		"OpName %BP_gl_PrimitiveID \"gl_PrimitiveID\"\n"
		"OpName %BP_in_color \"in_color\"\n"
		"OpName %BP_gl_PerVertex \"gl_PerVertex\"\n"
		"OpMemberName %BP_gl_PerVertex 0 \"gl_Position\"\n"
		"OpMemberName %BP_gl_PerVertex 1 \"gl_PointSize\"\n"
		"OpMemberName %BP_gl_PerVertex 2 \"gl_ClipDistance\"\n"
		"OpMemberName %BP_gl_PerVertex 3 \"gl_CullDistance\"\n"
		"OpName %BP_gl_out \"gl_out\"\n"
		"OpName %BP_gl_PVOut \"gl_PerVertex\"\n"
		"OpMemberName %BP_gl_PVOut 0 \"gl_Position\"\n"
		"OpMemberName %BP_gl_PVOut 1 \"gl_PointSize\"\n"
		"OpMemberName %BP_gl_PVOut 2 \"gl_ClipDistance\"\n"
		"OpMemberName %BP_gl_PVOut 3 \"gl_CullDistance\"\n"
		"OpName %BP_gl_in \"gl_in\"\n"
		"OpName %BP_gl_TessLevelOuter \"gl_TessLevelOuter\"\n"
		"OpName %BP_gl_TessLevelInner \"gl_TessLevelInner\"\n"
		"OpDecorate %BP_out_color Location 1\n"
		"OpDecorate %BP_gl_InvocationID BuiltIn InvocationId\n"
		"OpDecorate %BP_gl_PrimitiveID BuiltIn PrimitiveId\n"
		"OpDecorate %BP_in_color Location 1\n"
		"OpMemberDecorate %BP_gl_PerVertex 0 BuiltIn Position\n"
		"OpMemberDecorate %BP_gl_PerVertex 1 BuiltIn PointSize\n"
		"OpMemberDecorate %BP_gl_PerVertex 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %BP_gl_PerVertex 3 BuiltIn CullDistance\n"
		"OpDecorate %BP_gl_PerVertex Block\n"
		"OpMemberDecorate %BP_gl_PVOut 0 BuiltIn Position\n"
		"OpMemberDecorate %BP_gl_PVOut 1 BuiltIn PointSize\n"
		"OpMemberDecorate %BP_gl_PVOut 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %BP_gl_PVOut 3 BuiltIn CullDistance\n"
		"OpDecorate %BP_gl_PVOut Block\n"
		"OpDecorate %BP_gl_TessLevelOuter Patch\n"
		"OpDecorate %BP_gl_TessLevelOuter BuiltIn TessLevelOuter\n"
		"OpDecorate %BP_gl_TessLevelInner Patch\n"
		"OpDecorate %BP_gl_TessLevelInner BuiltIn TessLevelInner\n"
		"${IF_decoration:opt}\n"
		"${decoration:opt}\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%BP_out_color = OpVariable %op_a3v4f32 Output\n"
		"%BP_gl_InvocationID = OpVariable %ip_i32 Input\n"
		"%BP_gl_PrimitiveID = OpVariable %ip_i32 Input\n"
		"%BP_in_color = OpVariable %ip_a32v4f32 Input\n"
		"%BP_gl_PerVertex = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%BP_a3_gl_PerVertex = OpTypeArray %BP_gl_PerVertex %c_u32_3\n"
		"%BP_op_a3_gl_PerVertex = OpTypePointer Output %BP_a3_gl_PerVertex\n"
		"%BP_gl_out = OpVariable %BP_op_a3_gl_PerVertex Output\n"
		"%BP_gl_PVOut = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%BP_a32_gl_PVOut = OpTypeArray %BP_gl_PVOut %c_u32_32\n"
		"%BP_ip_a32_gl_PVOut = OpTypePointer Input %BP_a32_gl_PVOut\n"
		"%BP_gl_in = OpVariable %BP_ip_a32_gl_PVOut Input\n"
		"%BP_gl_TessLevelOuter = OpVariable %op_a4f32 Output\n"
		"%BP_gl_TessLevelInner = OpVariable %op_a2f32 Output\n"
		"${pre_main:opt}\n"
		"${IF_variable:opt}\n"

		"%BP_main = OpFunction %void None %fun\n"
		"%BP_label = OpLabel\n"
		"%BP_gl_Invoc = OpLoad %i32 %BP_gl_InvocationID\n"
		"${IF_carryforward:opt}\n"
		"%BP_in_col_loc = OpAccessChain %ip_v4f32 %BP_in_color %BP_gl_Invoc\n"
		"%BP_out_col_loc = OpAccessChain %op_v4f32 %BP_out_color %BP_gl_Invoc\n"
		"%BP_in_col_val = OpLoad %v4f32 %BP_in_col_loc\n"
		"%BP_clr_transformed = OpFunctionCall %v4f32 %test_code %BP_in_col_val\n"
		"OpStore %BP_out_col_loc %BP_clr_transformed\n"

		"%BP_in_pos_loc = OpAccessChain %ip_v4f32 %BP_gl_in %BP_gl_Invoc %c_i32_0\n"
		"%BP_out_pos_loc = OpAccessChain %op_v4f32 %BP_gl_out %BP_gl_Invoc %c_i32_0\n"
		"%BP_in_pos_val = OpLoad %v4f32 %BP_in_pos_loc\n"
		"OpStore %BP_out_pos_loc %BP_in_pos_val\n"

		"%BP_cmp = OpIEqual %bool %BP_gl_Invoc %c_i32_0\n"
		"OpSelectionMerge %BP_merge_label None\n"
		"OpBranchConditional %BP_cmp %BP_if_label %BP_merge_label\n"
		"%BP_if_label = OpLabel\n"
		"%BP_gl_TessLevelOuterPos_0 = OpAccessChain %op_f32 %BP_gl_TessLevelOuter %c_i32_0\n"
		"%BP_gl_TessLevelOuterPos_1 = OpAccessChain %op_f32 %BP_gl_TessLevelOuter %c_i32_1\n"
		"%BP_gl_TessLevelOuterPos_2 = OpAccessChain %op_f32 %BP_gl_TessLevelOuter %c_i32_2\n"
		"%BP_gl_TessLevelInnerPos_0 = OpAccessChain %op_f32 %BP_gl_TessLevelInner %c_i32_0\n"
		"OpStore %BP_gl_TessLevelOuterPos_0 %c_f32_1\n"
		"OpStore %BP_gl_TessLevelOuterPos_1 %c_f32_1\n"
		"OpStore %BP_gl_TessLevelOuterPos_2 %c_f32_1\n"
		"OpStore %BP_gl_TessLevelInnerPos_0 %c_f32_1\n"
		"OpBranch %BP_merge_label\n"
		"%BP_merge_label = OpLabel\n"
		"OpReturn\n"
		"OpFunctionEnd\n"
		"${interface_op_func:opt}\n"

		"%isUniqueIdZero = OpFunction %bool None %bool_function\n"
		"%getId_label = OpLabel\n"
		"%invocation_id = OpLoad %i32 %BP_gl_InvocationID\n"
		"%primitive_id = OpLoad %i32 %BP_gl_PrimitiveID\n"
		"%is_invocation_0 = OpIEqual %bool %invocation_id %c_i32_0\n"
		"%is_primitive_0 = OpIEqual %bool %primitive_id %c_i32_0\n"
		"%is_id_0 = OpLogicalAnd %bool %is_invocation_0 %is_primitive_0\n"
		"OpReturnValue %is_id_0\n"
		"OpFunctionEnd\n"

		"${testfun}\n";
	return tcu::StringTemplate(tessControlShaderBoilerplate).specialize(fragments);
}

// Creates tess-evaluation-shader assembly by specializing a boilerplate
// StringTemplate on fragments, which must (at least) map "testfun" to an
// OpFunction definition for %test_code that takes and returns a %v4f32.
// Boilerplate IDs are prefixed with "BP_" to avoid collisions with fragments.
//
// It roughly corresponds to the following glsl.
//
// #version 450
//
// layout(triangles, equal_spacing, ccw) in;
// layout(location = 1) in vec4 in_color[];
// layout(location = 1) out vec4 out_color;
//
// #define interpolate(val)
//   vec4(gl_TessCoord.x) * val[0] + vec4(gl_TessCoord.y) * val[1] +
//          vec4(gl_TessCoord.z) * val[2]
//
// void main() {
//   gl_Position = vec4(gl_TessCoord.x) * gl_in[0].gl_Position +
//                  vec4(gl_TessCoord.y) * gl_in[1].gl_Position +
//                  vec4(gl_TessCoord.z) * gl_in[2].gl_Position;
//   out_color = testfun(interpolate(in_color));
// }
string makeTessEvalShaderAssembly(const map<string, string>& fragments)
{
	static const char tessEvalBoilerplate[] =
		"OpCapability Tessellation\n"
		"OpCapability ClipDistance\n"
		"OpCapability CullDistance\n"
		"${capability:opt}\n"
		"${extension:opt}\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint TessellationEvaluation %BP_main \"main\" %BP_stream %BP_gl_TessCoord %BP_gl_PrimitiveID %BP_gl_in %BP_out_color %BP_in_color ${IF_entrypoint:opt} \n"
		"OpExecutionMode %BP_main Triangles\n"
		"OpExecutionMode %BP_main SpacingEqual\n"
		"OpExecutionMode %BP_main VertexOrderCcw\n"
		"${debug:opt}\n"
		"OpName %BP_main \"main\"\n"
		"OpName %test_code \"testfun(vf4;\"\n"
		"OpName %BP_gl_PerVertexOut \"gl_PerVertex\"\n"
		"OpMemberName %BP_gl_PerVertexOut 0 \"gl_Position\"\n"
		"OpMemberName %BP_gl_PerVertexOut 1 \"gl_PointSize\"\n"
		"OpMemberName %BP_gl_PerVertexOut 2 \"gl_ClipDistance\"\n"
		"OpMemberName %BP_gl_PerVertexOut 3 \"gl_CullDistance\"\n"
		"OpName %BP_stream \"\"\n"
		"OpName %BP_gl_TessCoord \"gl_TessCoord\"\n"
		"OpName %BP_gl_PerVertexIn \"gl_PerVertex\"\n"
		"OpName %BP_gl_PrimitiveID \"gl_PrimitiveID\"\n"
		"OpMemberName %BP_gl_PerVertexIn 0 \"gl_Position\"\n"
		"OpMemberName %BP_gl_PerVertexIn 1 \"gl_PointSize\"\n"
		"OpMemberName %BP_gl_PerVertexIn 2 \"gl_ClipDistance\"\n"
		"OpMemberName %BP_gl_PerVertexIn 3 \"gl_CullDistance\"\n"
		"OpName %BP_gl_in \"gl_in\"\n"
		"OpName %BP_out_color \"out_color\"\n"
		"OpName %BP_in_color \"in_color\"\n"
		"OpMemberDecorate %BP_gl_PerVertexOut 0 BuiltIn Position\n"
		"OpMemberDecorate %BP_gl_PerVertexOut 1 BuiltIn PointSize\n"
		"OpMemberDecorate %BP_gl_PerVertexOut 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %BP_gl_PerVertexOut 3 BuiltIn CullDistance\n"
		"OpDecorate %BP_gl_PerVertexOut Block\n"
		"OpDecorate %BP_gl_PrimitiveID BuiltIn PrimitiveId\n"
		"OpDecorate %BP_gl_TessCoord BuiltIn TessCoord\n"
		"OpMemberDecorate %BP_gl_PerVertexIn 0 BuiltIn Position\n"
		"OpMemberDecorate %BP_gl_PerVertexIn 1 BuiltIn PointSize\n"
		"OpMemberDecorate %BP_gl_PerVertexIn 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %BP_gl_PerVertexIn 3 BuiltIn CullDistance\n"
		"OpDecorate %BP_gl_PerVertexIn Block\n"
		"OpDecorate %BP_out_color Location 1\n"
		"OpDecorate %BP_in_color Location 1\n"
		"${IF_decoration:opt}\n"
		"${decoration:opt}\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%BP_gl_PerVertexOut = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%BP_op_gl_PerVertexOut = OpTypePointer Output %BP_gl_PerVertexOut\n"
		"%BP_stream = OpVariable %BP_op_gl_PerVertexOut Output\n"
		"%BP_gl_TessCoord = OpVariable %ip_v3f32 Input\n"
		"%BP_gl_PrimitiveID = OpVariable %op_i32 Input\n"
		"%BP_gl_PerVertexIn = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%BP_a32_gl_PerVertexIn = OpTypeArray %BP_gl_PerVertexIn %c_u32_32\n"
		"%BP_ip_a32_gl_PerVertexIn = OpTypePointer Input %BP_a32_gl_PerVertexIn\n"
		"%BP_gl_in = OpVariable %BP_ip_a32_gl_PerVertexIn Input\n"
		"%BP_out_color = OpVariable %op_v4f32 Output\n"
		"%BP_in_color = OpVariable %ip_a32v4f32 Input\n"
		"${pre_main:opt}\n"
		"${IF_variable:opt}\n"
		"%BP_main = OpFunction %void None %fun\n"
		"%BP_label = OpLabel\n"
		"${IF_carryforward:opt}\n"
		"%BP_gl_TC_0 = OpAccessChain %ip_f32 %BP_gl_TessCoord %c_u32_0\n"
		"%BP_gl_TC_1 = OpAccessChain %ip_f32 %BP_gl_TessCoord %c_u32_1\n"
		"%BP_gl_TC_2 = OpAccessChain %ip_f32 %BP_gl_TessCoord %c_u32_2\n"
		"%BP_gl_in_gl_Pos_0 = OpAccessChain %ip_v4f32 %BP_gl_in %c_i32_0 %c_i32_0\n"
		"%BP_gl_in_gl_Pos_1 = OpAccessChain %ip_v4f32 %BP_gl_in %c_i32_1 %c_i32_0\n"
		"%BP_gl_in_gl_Pos_2 = OpAccessChain %ip_v4f32 %BP_gl_in %c_i32_2 %c_i32_0\n"

		"%BP_gl_OPos = OpAccessChain %op_v4f32 %BP_stream %c_i32_0\n"
		"%BP_in_color_0 = OpAccessChain %ip_v4f32 %BP_in_color %c_i32_0\n"
		"%BP_in_color_1 = OpAccessChain %ip_v4f32 %BP_in_color %c_i32_1\n"
		"%BP_in_color_2 = OpAccessChain %ip_v4f32 %BP_in_color %c_i32_2\n"

		"%BP_TC_W_0 = OpLoad %f32 %BP_gl_TC_0\n"
		"%BP_TC_W_1 = OpLoad %f32 %BP_gl_TC_1\n"
		"%BP_TC_W_2 = OpLoad %f32 %BP_gl_TC_2\n"
		"%BP_v4f32_TC_0 = OpCompositeConstruct %v4f32 %BP_TC_W_0 %BP_TC_W_0 %BP_TC_W_0 %BP_TC_W_0\n"
		"%BP_v4f32_TC_1 = OpCompositeConstruct %v4f32 %BP_TC_W_1 %BP_TC_W_1 %BP_TC_W_1 %BP_TC_W_1\n"
		"%BP_v4f32_TC_2 = OpCompositeConstruct %v4f32 %BP_TC_W_2 %BP_TC_W_2 %BP_TC_W_2 %BP_TC_W_2\n"

		"%BP_gl_IP_0 = OpLoad %v4f32 %BP_gl_in_gl_Pos_0\n"
		"%BP_gl_IP_1 = OpLoad %v4f32 %BP_gl_in_gl_Pos_1\n"
		"%BP_gl_IP_2 = OpLoad %v4f32 %BP_gl_in_gl_Pos_2\n"

		"%BP_IP_W_0 = OpFMul %v4f32 %BP_v4f32_TC_0 %BP_gl_IP_0\n"
		"%BP_IP_W_1 = OpFMul %v4f32 %BP_v4f32_TC_1 %BP_gl_IP_1\n"
		"%BP_IP_W_2 = OpFMul %v4f32 %BP_v4f32_TC_2 %BP_gl_IP_2\n"

		"%BP_pos_sum_0 = OpFAdd %v4f32 %BP_IP_W_0 %BP_IP_W_1\n"
		"%BP_pos_sum_1 = OpFAdd %v4f32 %BP_pos_sum_0 %BP_IP_W_2\n"

		"OpStore %BP_gl_OPos %BP_pos_sum_1\n"

		"%BP_IC_0 = OpLoad %v4f32 %BP_in_color_0\n"
		"%BP_IC_1 = OpLoad %v4f32 %BP_in_color_1\n"
		"%BP_IC_2 = OpLoad %v4f32 %BP_in_color_2\n"

		"%BP_IC_W_0 = OpFMul %v4f32 %BP_v4f32_TC_0 %BP_IC_0\n"
		"%BP_IC_W_1 = OpFMul %v4f32 %BP_v4f32_TC_1 %BP_IC_1\n"
		"%BP_IC_W_2 = OpFMul %v4f32 %BP_v4f32_TC_2 %BP_IC_2\n"

		"%BP_col_sum_0 = OpFAdd %v4f32 %BP_IC_W_0 %BP_IC_W_1\n"
		"%BP_col_sum_1 = OpFAdd %v4f32 %BP_col_sum_0 %BP_IC_W_2\n"

		"%BP_clr_transformed = OpFunctionCall %v4f32 %test_code %BP_col_sum_1\n"

		"OpStore %BP_out_color %BP_clr_transformed\n"
		"OpReturn\n"
		"OpFunctionEnd\n"
		"${interface_op_func:opt}\n"

		"%isUniqueIdZero = OpFunction %bool None %bool_function\n"
		"%getId_label = OpLabel\n"
		"%primitive_id = OpLoad %i32 %BP_gl_PrimitiveID\n"
		"%is_primitive_0 = OpIEqual %bool %primitive_id %c_i32_0\n"
		"%TC_0_loc = OpAccessChain %ip_f32 %BP_gl_TessCoord %c_u32_0\n"
		"%TC_1_loc = OpAccessChain %ip_f32 %BP_gl_TessCoord %c_u32_1\n"
		"%TC_2_loc = OpAccessChain %ip_f32 %BP_gl_TessCoord %c_u32_2\n"
		"%TC_W_0 = OpLoad %f32 %TC_0_loc\n"
		"%TC_W_1 = OpLoad %f32 %TC_1_loc\n"
		"%TC_W_2 = OpLoad %f32 %TC_2_loc\n"
		"%is_W_0_1 = OpFOrdEqual %bool %TC_W_0 %c_f32_1\n"
		"%is_W_1_0 = OpFOrdEqual %bool %TC_W_1 %c_f32_0\n"
		"%is_W_2_0 = OpFOrdEqual %bool %TC_W_2 %c_f32_0\n"
		"%is_tessCoord_1_0 = OpLogicalAnd %bool %is_W_0_1 %is_W_1_0\n"
		"%is_tessCoord_1_0_0 = OpLogicalAnd %bool %is_tessCoord_1_0 %is_W_2_0\n"
		"%is_unique_id_0 = OpLogicalAnd %bool %is_tessCoord_1_0_0 %is_primitive_0\n"
		"OpReturnValue %is_unique_id_0\n"
		"OpFunctionEnd\n"

		"${testfun}\n";
	return tcu::StringTemplate(tessEvalBoilerplate).specialize(fragments);
}

// Creates geometry-shader assembly by specializing a boilerplate StringTemplate
// on fragments, which must (at least) map "testfun" to an OpFunction definition
// for %test_code that takes and returns a %v4f32.  Boilerplate IDs are prefixed
// with "BP_" to avoid collisions with fragments.
//
// Derived from this GLSL:
//
// #version 450
// layout(triangles) in;
// layout(triangle_strip, max_vertices = 3) out;
//
// layout(location = 1) in vec4 in_color[];
// layout(location = 1) out vec4 out_color;
//
// void main() {
//   gl_Position = gl_in[0].gl_Position;
//   out_color = test_fun(in_color[0]);
//   EmitVertex();
//   gl_Position = gl_in[1].gl_Position;
//   out_color = test_fun(in_color[1]);
//   EmitVertex();
//   gl_Position = gl_in[2].gl_Position;
//   out_color = test_fun(in_color[2]);
//   EmitVertex();
//   EndPrimitive();
// }
string makeGeometryShaderAssembly(const map<string, string>& fragments)
{
	static const char geometryShaderBoilerplate[] =
		"OpCapability Geometry\n"
		"OpCapability ClipDistance\n"
		"OpCapability CullDistance\n"
		"${capability:opt}\n"
		"${extension:opt}\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint Geometry %BP_main \"main\" %BP_out_gl_position %BP_gl_PrimitiveID %BP_gl_in %BP_out_color %BP_in_color ${IF_entrypoint:opt} \n"
		"OpExecutionMode %BP_main Triangles\n"
		"OpExecutionMode %BP_main OutputTriangleStrip\n"
		"OpExecutionMode %BP_main OutputVertices 3\n"
		"${debug:opt}\n"
		"OpName %BP_main \"main\"\n"
		"OpName %BP_gl_PrimitiveID \"gl_PrimitiveID\"\n"
		"OpName %BP_per_vertex_in \"gl_PerVertex\"\n"
		"OpMemberName %BP_per_vertex_in 0 \"gl_Position\"\n"
		"OpMemberName %BP_per_vertex_in 1 \"gl_PointSize\"\n"
		"OpMemberName %BP_per_vertex_in 2 \"gl_ClipDistance\"\n"
		"OpMemberName %BP_per_vertex_in 3 \"gl_CullDistance\"\n"
		"OpName %BP_gl_in \"gl_in\"\n"
		"OpName %BP_out_color \"out_color\"\n"
		"OpName %BP_in_color \"in_color\"\n"
		"OpName %test_code \"testfun(vf4;\"\n"
		"OpDecorate %BP_gl_PrimitiveID BuiltIn PrimitiveId\n"
		"OpDecorate %BP_out_gl_position BuiltIn Position\n"
		"OpMemberDecorate %BP_per_vertex_in 0 BuiltIn Position\n"
		"OpMemberDecorate %BP_per_vertex_in 1 BuiltIn PointSize\n"
		"OpMemberDecorate %BP_per_vertex_in 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %BP_per_vertex_in 3 BuiltIn CullDistance\n"
		"OpDecorate %BP_per_vertex_in Block\n"
		"OpDecorate %BP_out_color Location 1\n"
		"OpDecorate %BP_in_color Location 1\n"
		"${IF_decoration:opt}\n"
		"${decoration:opt}\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%BP_per_vertex_in = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%BP_a3_per_vertex_in = OpTypeArray %BP_per_vertex_in %c_u32_3\n"
		"%BP_ip_a3_per_vertex_in = OpTypePointer Input %BP_a3_per_vertex_in\n"
		"%BP_pp_i32 = OpTypePointer Private %i32\n"
		"%BP_pp_v4i32 = OpTypePointer Private %v4i32\n"

		"%BP_gl_in = OpVariable %BP_ip_a3_per_vertex_in Input\n"
		"%BP_out_color = OpVariable %op_v4f32 Output\n"
		"%BP_in_color = OpVariable %ip_a3v4f32 Input\n"
		"%BP_gl_PrimitiveID = OpVariable %ip_i32 Input\n"
		"%BP_out_gl_position = OpVariable %op_v4f32 Output\n"
		"%BP_vertexIdInCurrentPatch = OpVariable %BP_pp_v4i32 Private\n"
		"${pre_main:opt}\n"
		"${IF_variable:opt}\n"

		"%BP_main = OpFunction %void None %fun\n"
		"%BP_label = OpLabel\n"

		"${IF_carryforward:opt}\n"

		"%BP_primitiveId = OpLoad %i32 %BP_gl_PrimitiveID\n"
		"%BP_addr_vertexIdInCurrentPatch = OpAccessChain %BP_pp_i32 %BP_vertexIdInCurrentPatch %BP_primitiveId\n"

		"%BP_gl_in_0_gl_position = OpAccessChain %ip_v4f32 %BP_gl_in %c_i32_0 %c_i32_0\n"
		"%BP_gl_in_1_gl_position = OpAccessChain %ip_v4f32 %BP_gl_in %c_i32_1 %c_i32_0\n"
		"%BP_gl_in_2_gl_position = OpAccessChain %ip_v4f32 %BP_gl_in %c_i32_2 %c_i32_0\n"

		"%BP_in_position_0 = OpLoad %v4f32 %BP_gl_in_0_gl_position\n"
		"%BP_in_position_1 = OpLoad %v4f32 %BP_gl_in_1_gl_position\n"
		"%BP_in_position_2 = OpLoad %v4f32 %BP_gl_in_2_gl_position \n"

		"%BP_in_color_0_ptr = OpAccessChain %ip_v4f32 %BP_in_color %c_i32_0\n"
		"%BP_in_color_1_ptr = OpAccessChain %ip_v4f32 %BP_in_color %c_i32_1\n"
		"%BP_in_color_2_ptr = OpAccessChain %ip_v4f32 %BP_in_color %c_i32_2\n"

		"%BP_in_color_0 = OpLoad %v4f32 %BP_in_color_0_ptr\n"
		"%BP_in_color_1 = OpLoad %v4f32 %BP_in_color_1_ptr\n"
		"%BP_in_color_2 = OpLoad %v4f32 %BP_in_color_2_ptr\n"

		"OpStore %BP_addr_vertexIdInCurrentPatch %c_i32_0\n"
		"%BP_transformed_in_color_0 = OpFunctionCall %v4f32 %test_code %BP_in_color_0\n"
		"OpStore %BP_addr_vertexIdInCurrentPatch %c_i32_1\n"
		"%BP_transformed_in_color_1 = OpFunctionCall %v4f32 %test_code %BP_in_color_1\n"
		"OpStore %BP_addr_vertexIdInCurrentPatch %c_i32_2\n"
		"%BP_transformed_in_color_2 = OpFunctionCall %v4f32 %test_code %BP_in_color_2\n"


		"OpStore %BP_out_gl_position %BP_in_position_0\n"
		"OpStore %BP_out_color %BP_transformed_in_color_0\n"
		"OpEmitVertex\n"

		"OpStore %BP_out_gl_position %BP_in_position_1\n"
		"OpStore %BP_out_color %BP_transformed_in_color_1\n"
		"OpEmitVertex\n"

		"OpStore %BP_out_gl_position %BP_in_position_2\n"
		"OpStore %BP_out_color %BP_transformed_in_color_2\n"
		"OpEmitVertex\n"

		"OpEndPrimitive\n"
		"OpReturn\n"
		"OpFunctionEnd\n"
		"${interface_op_func:opt}\n"

		"%isUniqueIdZero = OpFunction %bool None %bool_function\n"
		"%getId_label = OpLabel\n"
		"%primitive_id = OpLoad %i32 %BP_gl_PrimitiveID\n"
		"%addr_vertexIdInCurrentPatch = OpAccessChain %BP_pp_i32 %BP_vertexIdInCurrentPatch %primitive_id\n"
		"%vertexIdInCurrentPatch = OpLoad %i32 %addr_vertexIdInCurrentPatch\n"
		"%is_primitive_0 = OpIEqual %bool %primitive_id %c_i32_0\n"
		"%is_vertex_0 = OpIEqual %bool %vertexIdInCurrentPatch %c_i32_0\n"
		"%is_unique_id_0 = OpLogicalAnd %bool %is_primitive_0 %is_vertex_0\n"
		"OpReturnValue %is_unique_id_0\n"
		"OpFunctionEnd\n"

		"${testfun}\n";
	return tcu::StringTemplate(geometryShaderBoilerplate).specialize(fragments);
}

// Creates fragment-shader assembly by specializing a boilerplate StringTemplate
// on fragments, which must (at least) map "testfun" to an OpFunction definition
// for %test_code that takes and returns a %v4f32.  Boilerplate IDs are prefixed
// with "BP_" to avoid collisions with fragments.
//
// Derived from this GLSL:
//
// layout(location = 1) in highp vec4 vtxColor;
// layout(location = 0) out highp vec4 fragColor;
// highp vec4 testfun(highp vec4 x) { return x; }
// void main(void) { fragColor = testfun(vtxColor); }
//
// with modifications including passing vtxColor by value and ripping out
// testfun() definition.
string makeFragmentShaderAssembly(const map<string, string>& fragments)
{
	static const char fragmentShaderBoilerplate[] =
		"OpCapability Shader\n"
		"${capability:opt}\n"
		"${extension:opt}\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint Fragment %BP_main \"main\" %BP_vtxColor %BP_fragColor %BP_gl_FragCoord ${IF_entrypoint:opt} \n"
		"OpExecutionMode %BP_main OriginUpperLeft\n"
		"${debug:opt}\n"
		"OpName %BP_main \"main\"\n"
		"OpName %BP_gl_FragCoord \"fragCoord\"\n"
		"OpName %BP_fragColor \"fragColor\"\n"
		"OpName %BP_vtxColor \"vtxColor\"\n"
		"OpName %test_code \"testfun(vf4;\"\n"
		"OpDecorate %BP_fragColor Location 0\n"
		"OpDecorate %BP_vtxColor Location 1\n"
		"OpDecorate %BP_gl_FragCoord BuiltIn FragCoord\n"
		"${IF_decoration:opt}\n"
		"${decoration:opt}\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%BP_gl_FragCoord = OpVariable %ip_v4f32 Input\n"
		"%BP_fragColor = OpVariable %op_v4f32 Output\n"
		"%BP_vtxColor = OpVariable %ip_v4f32 Input\n"
		"${pre_main:opt}\n"
		"${IF_variable:opt}\n"
		"%BP_main = OpFunction %void None %fun\n"
		"%BP_label_main = OpLabel\n"
		"${IF_carryforward:opt}\n"
		"%BP_tmp1 = OpLoad %v4f32 %BP_vtxColor\n"
		"%BP_tmp2 = OpFunctionCall %v4f32 %test_code %BP_tmp1\n"
		"OpStore %BP_fragColor %BP_tmp2\n"
		"OpReturn\n"
		"OpFunctionEnd\n"
		"${interface_op_func:opt}\n"

		"%isUniqueIdZero = OpFunction %bool None %bool_function\n"
		"%getId_label = OpLabel\n"
		"%loc_x_coord = OpAccessChain %ip_f32 %BP_gl_FragCoord %c_i32_0\n"
		"%loc_y_coord = OpAccessChain %ip_f32 %BP_gl_FragCoord %c_i32_1\n"
		"%x_coord = OpLoad %f32 %loc_x_coord\n"
		"%y_coord = OpLoad %f32 %loc_y_coord\n"
		"%is_x_idx0 = OpFOrdEqual %bool %x_coord %c_f32_0_5\n"
		"%is_y_idx0 = OpFOrdEqual %bool %y_coord %c_f32_0_5\n"
		"%is_frag_0 = OpLogicalAnd %bool %is_x_idx0 %is_y_idx0\n"
		"OpReturnValue %is_frag_0\n"
		"OpFunctionEnd\n"

		"${testfun}\n";
	return tcu::StringTemplate(fragmentShaderBoilerplate).specialize(fragments);
}

// Creates mappings from placeholders to pass-through shader code which copies
// the input to the output faithfully.
map<string, string> passthruInterface(const IFDataType& data_type)
{
	const string		var_type	= data_type.str();
	map<string, string>	fragments	= passthruFragments();
	const string		functype	= string("%") + var_type + "_" + var_type + "_function";

	fragments["interface_op_func"]	=
		string("%interface_op_func = OpFunction %") + var_type + " None " + functype + "\n"
		"               %io_param1 = OpFunctionParameter %" + var_type + "\n"
		"                %IF_label = OpLabel\n"
		"                            OpReturnValue %io_param1\n"
		"                            OpFunctionEnd\n";
	fragments["input_type"]			= var_type;
	fragments["output_type"]		= var_type;
	fragments["pre_main"]			= "";

	if (!data_type.elementIs32bit())
	{
		if (data_type.elementType == NUMBERTYPE_FLOAT16)
		{
			fragments["pre_main"]	+= "%f16 = OpTypeFloat 16\n";
		}
		else if (data_type.elementType == NUMBERTYPE_INT16)
		{
			fragments["pre_main"]	+= "%i16 = OpTypeInt 16 1\n";
		}
		else
		{
			fragments["pre_main"]	+= "%u16 = OpTypeInt 16 0\n";
		}

		fragments["capability"]		= "OpCapability StorageInputOutput16\n";
		fragments["extension"]		= "OpExtension \"SPV_KHR_16bit_storage\"\n";

		if (data_type.isVector())
		{
			fragments["pre_main"]	+= "%" + var_type + " = OpTypeVector %" + IFDataType(1, data_type.elementType).str() + " " + numberToString(data_type.numElements) + "\n";
		}

		fragments["pre_main"]		+=
			"%ip_" + var_type + " = OpTypePointer Input %" + var_type + "\n"
			"%op_" + var_type + " = OpTypePointer Output %" + var_type + "\n";
	}

	fragments["pre_main"]			+=
		functype + " = OpTypeFunction %" + var_type + " %" + var_type + "\n"
		"%a3" + var_type + " = OpTypeArray %" + var_type + " %c_i32_3\n"
		"%ip_a3" + var_type + " = OpTypePointer Input %a3" + var_type + "\n"
		"%op_a3" + var_type + " = OpTypePointer Output %a3" + var_type + "\n";

	return fragments;
}

// Returns mappings from interface placeholders to their concrete values.
//
// The concrete values should be specialized again to provide ${input_type}
// and ${output_type}.
//
// %ip_${input_type} and %op_${output_type} should also be defined in the final code.
map<string, string> fillInterfacePlaceholderVert (void)
{
	map<string, string>	fragments;

	fragments["IF_entrypoint"]		= "%IF_input %IF_output";
	fragments["IF_variable"]		=
		" %IF_input = OpVariable %ip_${input_type} Input\n"
		"%IF_output = OpVariable %op_${output_type} Output\n";
	fragments["IF_decoration"]		=
		"OpDecorate  %IF_input Location 2\n"
		"OpDecorate %IF_output Location 2\n";
	fragments["IF_carryforward"]	=
		"%IF_input_val = OpLoad %${input_type} %IF_input\n"
		"   %IF_result = OpFunctionCall %${output_type} %interface_op_func %IF_input_val\n"
		"                OpStore %IF_output %IF_result\n";

	// Make sure the rest still need to be instantialized.
	fragments["capability"]			= "${capability:opt}";
	fragments["extension"]			= "${extension:opt}";
	fragments["debug"]				= "${debug:opt}";
	fragments["decoration"]			= "${decoration:opt}";
	fragments["pre_main"]			= "${pre_main:opt}";
	fragments["testfun"]			= "${testfun}";
	fragments["interface_op_func"]	= "${interface_op_func}";

	return fragments;
}

// Returns mappings from interface placeholders to their concrete values.
//
// The concrete values should be specialized again to provide ${input_type}
// and ${output_type}.
//
// %ip_${input_type} and %op_${output_type} should also be defined in the final code.
map<string, string> fillInterfacePlaceholderFrag (void)
{
	map<string, string>	fragments;

	fragments["IF_entrypoint"]		= "%IF_input %IF_output";
	fragments["IF_variable"]		=
		" %IF_input = OpVariable %ip_${input_type} Input\n"
		"%IF_output = OpVariable %op_${output_type} Output\n";
	fragments["IF_decoration"]		=
		"OpDecorate %IF_input Flat\n"
		"OpDecorate %IF_input Location 2\n"
		"OpDecorate %IF_output Location 1\n";  // Fragment shader should write to location #1.
	fragments["IF_carryforward"]	=
		"%IF_input_val = OpLoad %${input_type} %IF_input\n"
		"   %IF_result = OpFunctionCall %${output_type} %interface_op_func %IF_input_val\n"
		"                OpStore %IF_output %IF_result\n";

	// Make sure the rest still need to be instantialized.
	fragments["capability"]			= "${capability:opt}";
	fragments["extension"]			= "${extension:opt}";
	fragments["debug"]				= "${debug:opt}";
	fragments["decoration"]			= "${decoration:opt}";
	fragments["pre_main"]			= "${pre_main:opt}";
	fragments["testfun"]			= "${testfun}";
	fragments["interface_op_func"]	= "${interface_op_func}";

	return fragments;
}

// Returns mappings from interface placeholders to their concrete values.
//
// The concrete values should be specialized again to provide ${input_type}
// and ${output_type}.
//
// %ip_${input_type}, %op_${output_type}, %ip_a3${input_type}, and $op_a3${output_type}
// should also be defined in the final code.
map<string, string> fillInterfacePlaceholderTessCtrl (void)
{
	map<string, string>	fragments;

	fragments["IF_entrypoint"]		= "%IF_input %IF_output";
	fragments["IF_variable"]		=
		" %IF_input = OpVariable %ip_a3${input_type} Input\n"
		"%IF_output = OpVariable %op_a3${output_type} Output\n";
	fragments["IF_decoration"]		=
		"OpDecorate  %IF_input Location 2\n"
		"OpDecorate %IF_output Location 2\n";
	fragments["IF_carryforward"]	=
		" %IF_input_ptr0 = OpAccessChain %ip_${input_type} %IF_input %c_i32_0\n"
		" %IF_input_ptr1 = OpAccessChain %ip_${input_type} %IF_input %c_i32_1\n"
		" %IF_input_ptr2 = OpAccessChain %ip_${input_type} %IF_input %c_i32_2\n"
		"%IF_output_ptr0 = OpAccessChain %op_${output_type} %IF_output %c_i32_0\n"
		"%IF_output_ptr1 = OpAccessChain %op_${output_type} %IF_output %c_i32_1\n"
		"%IF_output_ptr2 = OpAccessChain %op_${output_type} %IF_output %c_i32_2\n"
		"%IF_input_val0 = OpLoad %${input_type} %IF_input_ptr0\n"
		"%IF_input_val1 = OpLoad %${input_type} %IF_input_ptr1\n"
		"%IF_input_val2 = OpLoad %${input_type} %IF_input_ptr2\n"
		"%IF_input_res0 = OpFunctionCall %${output_type} %interface_op_func %IF_input_val0\n"
		"%IF_input_res1 = OpFunctionCall %${output_type} %interface_op_func %IF_input_val1\n"
		"%IF_input_res2 = OpFunctionCall %${output_type} %interface_op_func %IF_input_val2\n"
		"OpStore %IF_output_ptr0 %IF_input_res0\n"
		"OpStore %IF_output_ptr1 %IF_input_res1\n"
		"OpStore %IF_output_ptr2 %IF_input_res2\n";

	// Make sure the rest still need to be instantialized.
	fragments["capability"]			= "${capability:opt}";
	fragments["extension"]			= "${extension:opt}";
	fragments["debug"]				= "${debug:opt}";
	fragments["decoration"]			= "${decoration:opt}";
	fragments["pre_main"]			= "${pre_main:opt}";
	fragments["testfun"]			= "${testfun}";
	fragments["interface_op_func"]	= "${interface_op_func}";

	return fragments;
}

// Returns mappings from interface placeholders to their concrete values.
//
// The concrete values should be specialized again to provide ${input_type}
// and ${output_type}.
//
// %ip_${input_type}, %op_${output_type}, %ip_a3${input_type}, and $op_a3${output_type}
// should also be defined in the final code.
map<string, string> fillInterfacePlaceholderTessEvalGeom (void)
{
	map<string, string>	fragments;

	fragments["IF_entrypoint"]		= "%IF_input %IF_output";
	fragments["IF_variable"]		=
		" %IF_input = OpVariable %ip_a3${input_type} Input\n"
		"%IF_output = OpVariable %op_${output_type} Output\n";
	fragments["IF_decoration"]		=
		"OpDecorate  %IF_input Location 2\n"
		"OpDecorate %IF_output Location 2\n";
	fragments["IF_carryforward"]	=
		// Only get the first value since all three values are the same anyway.
		" %IF_input_ptr0 = OpAccessChain %ip_${input_type} %IF_input %c_i32_0\n"
		" %IF_input_val0 = OpLoad %${input_type} %IF_input_ptr0\n"
		" %IF_input_res0 = OpFunctionCall %${output_type} %interface_op_func %IF_input_val0\n"
		"OpStore %IF_output %IF_input_res0\n";

	// Make sure the rest still need to be instantialized.
	fragments["capability"]			= "${capability:opt}";
	fragments["extension"]			= "${extension:opt}";
	fragments["debug"]				= "${debug:opt}";
	fragments["decoration"]			= "${decoration:opt}";
	fragments["pre_main"]			= "${pre_main:opt}";
	fragments["testfun"]			= "${testfun}";
	fragments["interface_op_func"]	= "${interface_op_func}";

	return fragments;
}

map<string, string> passthruFragments(void)
{
	map<string, string> fragments;
	fragments["testfun"] =
		// A %test_code function that returns its argument unchanged.
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"OpReturnValue %param1\n"
		"OpFunctionEnd\n";
	return fragments;
}

// Adds shader assembly text to dst.spirvAsmSources for all shader kinds.
// Vertex shader gets custom code from context, the rest are pass-through.
void addShaderCodeCustomVertex(vk::SourceCollections& dst, InstanceContext context)
{
	if (!context.interfaces.empty())
	{
		// Inject boilerplate code to wire up additional input/output variables between stages.
		// Just copy the contents in input variable to output variable in all stages except
		// the customized stage.
		dst.spirvAsmSources.add("vert") << StringTemplate(makeVertexShaderAssembly(fillInterfacePlaceholderVert())).specialize(context.testCodeFragments);
		dst.spirvAsmSources.add("frag") << StringTemplate(makeFragmentShaderAssembly(fillInterfacePlaceholderFrag())).specialize(passthruInterface(context.interfaces.getOutputType()));
	} else {
		map<string, string> passthru = passthruFragments();

		dst.spirvAsmSources.add("vert") << makeVertexShaderAssembly(context.testCodeFragments);
		dst.spirvAsmSources.add("frag") << makeFragmentShaderAssembly(passthru);
	}
}

// Adds shader assembly text to dst.spirvAsmSources for all shader kinds.
// Tessellation control shader gets custom code from context, the rest are
// pass-through.
void addShaderCodeCustomTessControl(vk::SourceCollections& dst, InstanceContext context)
{
	if (!context.interfaces.empty())
	{
		// Inject boilerplate code to wire up additional input/output variables between stages.
		// Just copy the contents in input variable to output variable in all stages except
		// the customized stage.
		dst.spirvAsmSources.add("vert") << StringTemplate(makeVertexShaderAssembly(fillInterfacePlaceholderVert())).specialize(passthruInterface(context.interfaces.getInputType()));
		dst.spirvAsmSources.add("tessc") << StringTemplate(makeTessControlShaderAssembly(fillInterfacePlaceholderTessCtrl())).specialize(context.testCodeFragments);
		dst.spirvAsmSources.add("tesse") << StringTemplate(makeTessEvalShaderAssembly(fillInterfacePlaceholderTessEvalGeom())).specialize(passthruInterface(context.interfaces.getOutputType()));
		dst.spirvAsmSources.add("frag") << StringTemplate(makeFragmentShaderAssembly(fillInterfacePlaceholderFrag())).specialize(passthruInterface(context.interfaces.getOutputType()));
	}
	else
	{
		map<string, string> passthru = passthruFragments();

		dst.spirvAsmSources.add("vert") << makeVertexShaderAssembly(passthru);
		dst.spirvAsmSources.add("tessc") << makeTessControlShaderAssembly(context.testCodeFragments);
		dst.spirvAsmSources.add("tesse") << makeTessEvalShaderAssembly(passthru);
		dst.spirvAsmSources.add("frag") << makeFragmentShaderAssembly(passthru);
	}
}

// Adds shader assembly text to dst.spirvAsmSources for all shader kinds.
// Tessellation evaluation shader gets custom code from context, the rest are
// pass-through.
void addShaderCodeCustomTessEval(vk::SourceCollections& dst, InstanceContext context)
{
	if (!context.interfaces.empty())
	{
		// Inject boilerplate code to wire up additional input/output variables between stages.
		// Just copy the contents in input variable to output variable in all stages except
		// the customized stage.
		dst.spirvAsmSources.add("vert") << StringTemplate(makeVertexShaderAssembly(fillInterfacePlaceholderVert())).specialize(passthruInterface(context.interfaces.getInputType()));
		dst.spirvAsmSources.add("tessc") << StringTemplate(makeTessControlShaderAssembly(fillInterfacePlaceholderTessCtrl())).specialize(passthruInterface(context.interfaces.getInputType()));
		dst.spirvAsmSources.add("tesse") << StringTemplate(makeTessEvalShaderAssembly(fillInterfacePlaceholderTessEvalGeom())).specialize(context.testCodeFragments);
		dst.spirvAsmSources.add("frag") << StringTemplate(makeFragmentShaderAssembly(fillInterfacePlaceholderFrag())).specialize(passthruInterface(context.interfaces.getOutputType()));
	}
	else
	{
		map<string, string> passthru = passthruFragments();
		dst.spirvAsmSources.add("vert") << makeVertexShaderAssembly(passthru);
		dst.spirvAsmSources.add("tessc") << makeTessControlShaderAssembly(passthru);
		dst.spirvAsmSources.add("tesse") << makeTessEvalShaderAssembly(context.testCodeFragments);
		dst.spirvAsmSources.add("frag") << makeFragmentShaderAssembly(passthru);
	}
}

// Adds shader assembly text to dst.spirvAsmSources for all shader kinds.
// Geometry shader gets custom code from context, the rest are pass-through.
void addShaderCodeCustomGeometry(vk::SourceCollections& dst, InstanceContext context)
{
	if (!context.interfaces.empty())
	{
		// Inject boilerplate code to wire up additional input/output variables between stages.
		// Just copy the contents in input variable to output variable in all stages except
		// the customized stage.
		dst.spirvAsmSources.add("vert") << StringTemplate(makeVertexShaderAssembly(fillInterfacePlaceholderVert())).specialize(passthruInterface(context.interfaces.getInputType()));
		dst.spirvAsmSources.add("geom") << StringTemplate(makeGeometryShaderAssembly(fillInterfacePlaceholderTessEvalGeom())).specialize(context.testCodeFragments);
		dst.spirvAsmSources.add("frag") << StringTemplate(makeFragmentShaderAssembly(fillInterfacePlaceholderFrag())).specialize(passthruInterface(context.interfaces.getOutputType()));
	}
	else
	{
		map<string, string> passthru = passthruFragments();
		dst.spirvAsmSources.add("vert") << makeVertexShaderAssembly(passthru);
		dst.spirvAsmSources.add("geom") << makeGeometryShaderAssembly(context.testCodeFragments);
		dst.spirvAsmSources.add("frag") << makeFragmentShaderAssembly(passthru);
	}
}

// Adds shader assembly text to dst.spirvAsmSources for all shader kinds.
// Fragment shader gets custom code from context, the rest are pass-through.
void addShaderCodeCustomFragment(vk::SourceCollections& dst, InstanceContext context)
{
	if (!context.interfaces.empty())
	{
		// Inject boilerplate code to wire up additional input/output variables between stages.
		// Just copy the contents in input variable to output variable in all stages except
		// the customized stage.
		dst.spirvAsmSources.add("vert") << StringTemplate(makeVertexShaderAssembly(fillInterfacePlaceholderVert())).specialize(passthruInterface(context.interfaces.getInputType()));
		dst.spirvAsmSources.add("frag") << StringTemplate(makeFragmentShaderAssembly(fillInterfacePlaceholderFrag())).specialize(context.testCodeFragments);
	}
	else
	{
		map<string, string> passthru = passthruFragments();
		dst.spirvAsmSources.add("vert") << makeVertexShaderAssembly(passthru);
		dst.spirvAsmSources.add("frag") << makeFragmentShaderAssembly(context.testCodeFragments);
	}
}

void createCombinedModule(vk::SourceCollections& dst, InstanceContext)
{
	// \todo [2015-12-07 awoloszyn] Make tessellation / geometry conditional
	// \todo [2015-12-07 awoloszyn] Remove OpName and OpMemberName at some point
	dst.spirvAsmSources.add("module") <<
		"OpCapability Shader\n"
		"OpCapability ClipDistance\n"
		"OpCapability CullDistance\n"
		"OpCapability Geometry\n"
		"OpCapability Tessellation\n"
		"OpMemoryModel Logical GLSL450\n"

		"OpEntryPoint Vertex %vert_main \"main\" %vert_Position %vert_vtxColor %vert_color %vert_vtxPosition %vert_vertex_id %vert_instance_id\n"
		"OpEntryPoint Geometry %geom_main \"main\" %geom_out_gl_position %geom_gl_in %geom_out_color %geom_in_color\n"
		"OpEntryPoint TessellationControl %tessc_main \"main\" %tessc_out_color %tessc_gl_InvocationID %tessc_in_color %tessc_out_position %tessc_in_position %tessc_gl_TessLevelOuter %tessc_gl_TessLevelInner\n"
		"OpEntryPoint TessellationEvaluation %tesse_main \"main\" %tesse_stream %tesse_gl_tessCoord %tesse_in_position %tesse_out_color %tesse_in_color \n"
		"OpEntryPoint Fragment %frag_main \"main\" %frag_vtxColor %frag_fragColor\n"

		"OpExecutionMode %geom_main Triangles\n"
		"OpExecutionMode %geom_main OutputTriangleStrip\n"
		"OpExecutionMode %geom_main OutputVertices 3\n"

		"OpExecutionMode %tessc_main OutputVertices 3\n"

		"OpExecutionMode %tesse_main Triangles\n"
		"OpExecutionMode %tesse_main SpacingEqual\n"
		"OpExecutionMode %tesse_main VertexOrderCcw\n"

		"OpExecutionMode %frag_main OriginUpperLeft\n"

		"OpName %vert_main \"main\"\n"
		"OpName %vert_vtxPosition \"vtxPosition\"\n"
		"OpName %vert_Position \"position\"\n"
		"OpName %vert_vtxColor \"vtxColor\"\n"
		"OpName %vert_color \"color\"\n"
		"OpName %vert_vertex_id \"gl_VertexIndex\"\n"
		"OpName %vert_instance_id \"gl_InstanceIndex\"\n"
		"OpName %geom_main \"main\"\n"
		"OpName %geom_per_vertex_in \"gl_PerVertex\"\n"
		"OpMemberName %geom_per_vertex_in 0 \"gl_Position\"\n"
		"OpMemberName %geom_per_vertex_in 1 \"gl_PointSize\"\n"
		"OpMemberName %geom_per_vertex_in 2 \"gl_ClipDistance\"\n"
		"OpMemberName %geom_per_vertex_in 3 \"gl_CullDistance\"\n"
		"OpName %geom_gl_in \"gl_in\"\n"
		"OpName %geom_out_color \"out_color\"\n"
		"OpName %geom_in_color \"in_color\"\n"
		"OpName %tessc_main \"main\"\n"
		"OpName %tessc_out_color \"out_color\"\n"
		"OpName %tessc_gl_InvocationID \"gl_InvocationID\"\n"
		"OpName %tessc_in_color \"in_color\"\n"
		"OpName %tessc_out_position \"out_position\"\n"
		"OpName %tessc_in_position \"in_position\"\n"
		"OpName %tessc_gl_TessLevelOuter \"gl_TessLevelOuter\"\n"
		"OpName %tessc_gl_TessLevelInner \"gl_TessLevelInner\"\n"
		"OpName %tesse_main \"main\"\n"
		"OpName %tesse_per_vertex_out \"gl_PerVertex\"\n"
		"OpMemberName %tesse_per_vertex_out 0 \"gl_Position\"\n"
		"OpMemberName %tesse_per_vertex_out 1 \"gl_PointSize\"\n"
		"OpMemberName %tesse_per_vertex_out 2 \"gl_ClipDistance\"\n"
		"OpMemberName %tesse_per_vertex_out 3 \"gl_CullDistance\"\n"
		"OpName %tesse_stream \"\"\n"
		"OpName %tesse_gl_tessCoord \"gl_TessCoord\"\n"
		"OpName %tesse_in_position \"in_position\"\n"
		"OpName %tesse_out_color \"out_color\"\n"
		"OpName %tesse_in_color \"in_color\"\n"
		"OpName %frag_main \"main\"\n"
		"OpName %frag_fragColor \"fragColor\"\n"
		"OpName %frag_vtxColor \"vtxColor\"\n"

		"; Vertex decorations\n"
		"OpDecorate %vert_vtxPosition Location 2\n"
		"OpDecorate %vert_Position Location 0\n"
		"OpDecorate %vert_vtxColor Location 1\n"
		"OpDecorate %vert_color Location 1\n"
		"OpDecorate %vert_vertex_id BuiltIn VertexIndex\n"
		"OpDecorate %vert_instance_id BuiltIn InstanceIndex\n"

		"; Geometry decorations\n"
		"OpDecorate %geom_out_gl_position BuiltIn Position\n"
		"OpMemberDecorate %geom_per_vertex_in 0 BuiltIn Position\n"
		"OpMemberDecorate %geom_per_vertex_in 1 BuiltIn PointSize\n"
		"OpMemberDecorate %geom_per_vertex_in 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %geom_per_vertex_in 3 BuiltIn CullDistance\n"
		"OpDecorate %geom_per_vertex_in Block\n"
		"OpDecorate %geom_out_color Location 1\n"
		"OpDecorate %geom_in_color Location 1\n"

		"; Tessellation Control decorations\n"
		"OpDecorate %tessc_out_color Location 1\n"
		"OpDecorate %tessc_gl_InvocationID BuiltIn InvocationId\n"
		"OpDecorate %tessc_in_color Location 1\n"
		"OpDecorate %tessc_out_position Location 2\n"
		"OpDecorate %tessc_in_position Location 2\n"
		"OpDecorate %tessc_gl_TessLevelOuter Patch\n"
		"OpDecorate %tessc_gl_TessLevelOuter BuiltIn TessLevelOuter\n"
		"OpDecorate %tessc_gl_TessLevelInner Patch\n"
		"OpDecorate %tessc_gl_TessLevelInner BuiltIn TessLevelInner\n"

		"; Tessellation Evaluation decorations\n"
		"OpMemberDecorate %tesse_per_vertex_out 0 BuiltIn Position\n"
		"OpMemberDecorate %tesse_per_vertex_out 1 BuiltIn PointSize\n"
		"OpMemberDecorate %tesse_per_vertex_out 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %tesse_per_vertex_out 3 BuiltIn CullDistance\n"
		"OpDecorate %tesse_per_vertex_out Block\n"
		"OpDecorate %tesse_gl_tessCoord BuiltIn TessCoord\n"
		"OpDecorate %tesse_in_position Location 2\n"
		"OpDecorate %tesse_out_color Location 1\n"
		"OpDecorate %tesse_in_color Location 1\n"

		"; Fragment decorations\n"
		"OpDecorate %frag_fragColor Location 0\n"
		"OpDecorate %frag_vtxColor Location 1\n"

		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS

		"; Vertex Variables\n"
		"%vert_vtxPosition = OpVariable %op_v4f32 Output\n"
		"%vert_Position = OpVariable %ip_v4f32 Input\n"
		"%vert_vtxColor = OpVariable %op_v4f32 Output\n"
		"%vert_color = OpVariable %ip_v4f32 Input\n"
		"%vert_vertex_id = OpVariable %ip_i32 Input\n"
		"%vert_instance_id = OpVariable %ip_i32 Input\n"

		"; Geometry Variables\n"
		"%geom_per_vertex_in = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%geom_a3_per_vertex_in = OpTypeArray %geom_per_vertex_in %c_u32_3\n"
		"%geom_ip_a3_per_vertex_in = OpTypePointer Input %geom_a3_per_vertex_in\n"
		"%geom_gl_in = OpVariable %geom_ip_a3_per_vertex_in Input\n"
		"%geom_out_color = OpVariable %op_v4f32 Output\n"
		"%geom_in_color = OpVariable %ip_a3v4f32 Input\n"
		"%geom_out_gl_position = OpVariable %op_v4f32 Output\n"

		"; Tessellation Control Variables\n"
		"%tessc_out_color = OpVariable %op_a3v4f32 Output\n"
		"%tessc_gl_InvocationID = OpVariable %ip_i32 Input\n"
		"%tessc_in_color = OpVariable %ip_a32v4f32 Input\n"
		"%tessc_out_position = OpVariable %op_a3v4f32 Output\n"
		"%tessc_in_position = OpVariable %ip_a32v4f32 Input\n"
		"%tessc_gl_TessLevelOuter = OpVariable %op_a4f32 Output\n"
		"%tessc_gl_TessLevelInner = OpVariable %op_a2f32 Output\n"

		"; Tessellation Evaluation Decorations\n"
		"%tesse_per_vertex_out = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%tesse_op_per_vertex_out = OpTypePointer Output %tesse_per_vertex_out\n"
		"%tesse_stream = OpVariable %tesse_op_per_vertex_out Output\n"
		"%tesse_gl_tessCoord = OpVariable %ip_v3f32 Input\n"
		"%tesse_in_position = OpVariable %ip_a32v4f32 Input\n"
		"%tesse_out_color = OpVariable %op_v4f32 Output\n"
		"%tesse_in_color = OpVariable %ip_a32v4f32 Input\n"

		"; Fragment Variables\n"
		"%frag_fragColor = OpVariable %op_v4f32 Output\n"
		"%frag_vtxColor = OpVariable %ip_v4f32 Input\n"

		"; Vertex Entry\n"
		"%vert_main = OpFunction %void None %fun\n"
		"%vert_label = OpLabel\n"
		"%vert_tmp_position = OpLoad %v4f32 %vert_Position\n"
		"OpStore %vert_vtxPosition %vert_tmp_position\n"
		"%vert_tmp_color = OpLoad %v4f32 %vert_color\n"
		"OpStore %vert_vtxColor %vert_tmp_color\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"; Geometry Entry\n"
		"%geom_main = OpFunction %void None %fun\n"
		"%geom_label = OpLabel\n"
		"%geom_gl_in_0_gl_position = OpAccessChain %ip_v4f32 %geom_gl_in %c_i32_0 %c_i32_0\n"
		"%geom_gl_in_1_gl_position = OpAccessChain %ip_v4f32 %geom_gl_in %c_i32_1 %c_i32_0\n"
		"%geom_gl_in_2_gl_position = OpAccessChain %ip_v4f32 %geom_gl_in %c_i32_2 %c_i32_0\n"
		"%geom_in_position_0 = OpLoad %v4f32 %geom_gl_in_0_gl_position\n"
		"%geom_in_position_1 = OpLoad %v4f32 %geom_gl_in_1_gl_position\n"
		"%geom_in_position_2 = OpLoad %v4f32 %geom_gl_in_2_gl_position \n"
		"%geom_in_color_0_ptr = OpAccessChain %ip_v4f32 %geom_in_color %c_i32_0\n"
		"%geom_in_color_1_ptr = OpAccessChain %ip_v4f32 %geom_in_color %c_i32_1\n"
		"%geom_in_color_2_ptr = OpAccessChain %ip_v4f32 %geom_in_color %c_i32_2\n"
		"%geom_in_color_0 = OpLoad %v4f32 %geom_in_color_0_ptr\n"
		"%geom_in_color_1 = OpLoad %v4f32 %geom_in_color_1_ptr\n"
		"%geom_in_color_2 = OpLoad %v4f32 %geom_in_color_2_ptr\n"
		"OpStore %geom_out_gl_position %geom_in_position_0\n"
		"OpStore %geom_out_color %geom_in_color_0\n"
		"OpEmitVertex\n"
		"OpStore %geom_out_gl_position %geom_in_position_1\n"
		"OpStore %geom_out_color %geom_in_color_1\n"
		"OpEmitVertex\n"
		"OpStore %geom_out_gl_position %geom_in_position_2\n"
		"OpStore %geom_out_color %geom_in_color_2\n"
		"OpEmitVertex\n"
		"OpEndPrimitive\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"; Tessellation Control Entry\n"
		"%tessc_main = OpFunction %void None %fun\n"
		"%tessc_label = OpLabel\n"
		"%tessc_invocation_id = OpLoad %i32 %tessc_gl_InvocationID\n"
		"%tessc_in_color_ptr = OpAccessChain %ip_v4f32 %tessc_in_color %tessc_invocation_id\n"
		"%tessc_in_position_ptr = OpAccessChain %ip_v4f32 %tessc_in_position %tessc_invocation_id\n"
		"%tessc_in_color_val = OpLoad %v4f32 %tessc_in_color_ptr\n"
		"%tessc_in_position_val = OpLoad %v4f32 %tessc_in_position_ptr\n"
		"%tessc_out_color_ptr = OpAccessChain %op_v4f32 %tessc_out_color %tessc_invocation_id\n"
		"%tessc_out_position_ptr = OpAccessChain %op_v4f32 %tessc_out_position %tessc_invocation_id\n"
		"OpStore %tessc_out_color_ptr %tessc_in_color_val\n"
		"OpStore %tessc_out_position_ptr %tessc_in_position_val\n"
		"%tessc_is_first_invocation = OpIEqual %bool %tessc_invocation_id %c_i32_0\n"
		"OpSelectionMerge %tessc_merge_label None\n"
		"OpBranchConditional %tessc_is_first_invocation %tessc_first_invocation %tessc_merge_label\n"
		"%tessc_first_invocation = OpLabel\n"
		"%tessc_tess_outer_0 = OpAccessChain %op_f32 %tessc_gl_TessLevelOuter %c_i32_0\n"
		"%tessc_tess_outer_1 = OpAccessChain %op_f32 %tessc_gl_TessLevelOuter %c_i32_1\n"
		"%tessc_tess_outer_2 = OpAccessChain %op_f32 %tessc_gl_TessLevelOuter %c_i32_2\n"
		"%tessc_tess_inner = OpAccessChain %op_f32 %tessc_gl_TessLevelInner %c_i32_0\n"
		"OpStore %tessc_tess_outer_0 %c_f32_1\n"
		"OpStore %tessc_tess_outer_1 %c_f32_1\n"
		"OpStore %tessc_tess_outer_2 %c_f32_1\n"
		"OpStore %tessc_tess_inner %c_f32_1\n"
		"OpBranch %tessc_merge_label\n"
		"%tessc_merge_label = OpLabel\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"; Tessellation Evaluation Entry\n"
		"%tesse_main = OpFunction %void None %fun\n"
		"%tesse_label = OpLabel\n"
		"%tesse_tc_0_ptr = OpAccessChain %ip_f32 %tesse_gl_tessCoord %c_u32_0\n"
		"%tesse_tc_1_ptr = OpAccessChain %ip_f32 %tesse_gl_tessCoord %c_u32_1\n"
		"%tesse_tc_2_ptr = OpAccessChain %ip_f32 %tesse_gl_tessCoord %c_u32_2\n"
		"%tesse_tc_0 = OpLoad %f32 %tesse_tc_0_ptr\n"
		"%tesse_tc_1 = OpLoad %f32 %tesse_tc_1_ptr\n"
		"%tesse_tc_2 = OpLoad %f32 %tesse_tc_2_ptr\n"
		"%tesse_in_pos_0_ptr = OpAccessChain %ip_v4f32 %tesse_in_position %c_i32_0\n"
		"%tesse_in_pos_1_ptr = OpAccessChain %ip_v4f32 %tesse_in_position %c_i32_1\n"
		"%tesse_in_pos_2_ptr = OpAccessChain %ip_v4f32 %tesse_in_position %c_i32_2\n"
		"%tesse_in_pos_0 = OpLoad %v4f32 %tesse_in_pos_0_ptr\n"
		"%tesse_in_pos_1 = OpLoad %v4f32 %tesse_in_pos_1_ptr\n"
		"%tesse_in_pos_2 = OpLoad %v4f32 %tesse_in_pos_2_ptr\n"
		"%tesse_in_pos_0_weighted = OpVectorTimesScalar %v4f32 %tesse_in_pos_0 %tesse_tc_0\n"
		"%tesse_in_pos_1_weighted = OpVectorTimesScalar %v4f32 %tesse_in_pos_1 %tesse_tc_1\n"
		"%tesse_in_pos_2_weighted = OpVectorTimesScalar %v4f32 %tesse_in_pos_2 %tesse_tc_2\n"
		"%tesse_out_pos_ptr = OpAccessChain %op_v4f32 %tesse_stream %c_i32_0\n"
		"%tesse_in_pos_0_plus_pos_1 = OpFAdd %v4f32 %tesse_in_pos_0_weighted %tesse_in_pos_1_weighted\n"
		"%tesse_computed_out = OpFAdd %v4f32 %tesse_in_pos_0_plus_pos_1 %tesse_in_pos_2_weighted\n"
		"OpStore %tesse_out_pos_ptr %tesse_computed_out\n"
		"%tesse_in_clr_0_ptr = OpAccessChain %ip_v4f32 %tesse_in_color %c_i32_0\n"
		"%tesse_in_clr_1_ptr = OpAccessChain %ip_v4f32 %tesse_in_color %c_i32_1\n"
		"%tesse_in_clr_2_ptr = OpAccessChain %ip_v4f32 %tesse_in_color %c_i32_2\n"
		"%tesse_in_clr_0 = OpLoad %v4f32 %tesse_in_clr_0_ptr\n"
		"%tesse_in_clr_1 = OpLoad %v4f32 %tesse_in_clr_1_ptr\n"
		"%tesse_in_clr_2 = OpLoad %v4f32 %tesse_in_clr_2_ptr\n"
		"%tesse_in_clr_0_weighted = OpVectorTimesScalar %v4f32 %tesse_in_clr_0 %tesse_tc_0\n"
		"%tesse_in_clr_1_weighted = OpVectorTimesScalar %v4f32 %tesse_in_clr_1 %tesse_tc_1\n"
		"%tesse_in_clr_2_weighted = OpVectorTimesScalar %v4f32 %tesse_in_clr_2 %tesse_tc_2\n"
		"%tesse_in_clr_0_plus_col_1 = OpFAdd %v4f32 %tesse_in_clr_0_weighted %tesse_in_clr_1_weighted\n"
		"%tesse_computed_clr = OpFAdd %v4f32 %tesse_in_clr_0_plus_col_1 %tesse_in_clr_2_weighted\n"
		"OpStore %tesse_out_color %tesse_computed_clr\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"; Fragment Entry\n"
		"%frag_main = OpFunction %void None %fun\n"
		"%frag_label_main = OpLabel\n"
		"%frag_tmp1 = OpLoad %v4f32 %frag_vtxColor\n"
		"OpStore %frag_fragColor %frag_tmp1\n"
		"OpReturn\n"
		"OpFunctionEnd\n";
}

void createMultipleEntries(vk::SourceCollections& dst, InstanceContext)
{
	dst.spirvAsmSources.add("vert") <<
	// This module contains 2 vertex shaders. One that is a passthrough
	// and a second that inverts the color of the output (1.0 - color).
		"OpCapability Shader\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint Vertex %main \"vert1\" %Position %vtxColor %color %vtxPosition %vertex_id %instance_id\n"
		"OpEntryPoint Vertex %main2 \"vert2\" %Position %vtxColor %color %vtxPosition %vertex_id %instance_id\n"

		"OpName %main \"vert1\"\n"
		"OpName %main2 \"vert2\"\n"
		"OpName %vtxPosition \"vtxPosition\"\n"
		"OpName %Position \"position\"\n"
		"OpName %vtxColor \"vtxColor\"\n"
		"OpName %color \"color\"\n"
		"OpName %vertex_id \"gl_VertexIndex\"\n"
		"OpName %instance_id \"gl_InstanceIndex\"\n"

		"OpDecorate %vtxPosition Location 2\n"
		"OpDecorate %Position Location 0\n"
		"OpDecorate %vtxColor Location 1\n"
		"OpDecorate %color Location 1\n"
		"OpDecorate %vertex_id BuiltIn VertexIndex\n"
		"OpDecorate %instance_id BuiltIn InstanceIndex\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%cval = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_0\n"
		"%vtxPosition = OpVariable %op_v4f32 Output\n"
		"%Position = OpVariable %ip_v4f32 Input\n"
		"%vtxColor = OpVariable %op_v4f32 Output\n"
		"%color = OpVariable %ip_v4f32 Input\n"
		"%vertex_id = OpVariable %ip_i32 Input\n"
		"%instance_id = OpVariable %ip_i32 Input\n"

		"%main = OpFunction %void None %fun\n"
		"%label = OpLabel\n"
		"%tmp_position = OpLoad %v4f32 %Position\n"
		"OpStore %vtxPosition %tmp_position\n"
		"%tmp_color = OpLoad %v4f32 %color\n"
		"OpStore %vtxColor %tmp_color\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"%main2 = OpFunction %void None %fun\n"
		"%label2 = OpLabel\n"
		"%tmp_position2 = OpLoad %v4f32 %Position\n"
		"OpStore %vtxPosition %tmp_position2\n"
		"%tmp_color2 = OpLoad %v4f32 %color\n"
		"%tmp_color3 = OpFSub %v4f32 %cval %tmp_color2\n"
		"%tmp_color4 = OpVectorInsertDynamic %v4f32 %tmp_color3 %c_f32_1 %c_i32_3\n"
		"OpStore %vtxColor %tmp_color4\n"
		"OpReturn\n"
		"OpFunctionEnd\n";

	dst.spirvAsmSources.add("frag") <<
		// This is a single module that contains 2 fragment shaders.
		// One that passes color through and the other that inverts the output
		// color (1.0 - color).
		"OpCapability Shader\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint Fragment %main \"frag1\" %vtxColor %fragColor\n"
		"OpEntryPoint Fragment %main2 \"frag2\" %vtxColor %fragColor\n"
		"OpExecutionMode %main OriginUpperLeft\n"
		"OpExecutionMode %main2 OriginUpperLeft\n"

		"OpName %main \"frag1\"\n"
		"OpName %main2 \"frag2\"\n"
		"OpName %fragColor \"fragColor\"\n"
		"OpName %vtxColor \"vtxColor\"\n"
		"OpDecorate %fragColor Location 0\n"
		"OpDecorate %vtxColor Location 1\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%cval = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_0\n"
		"%fragColor = OpVariable %op_v4f32 Output\n"
		"%vtxColor = OpVariable %ip_v4f32 Input\n"

		"%main = OpFunction %void None %fun\n"
		"%label_main = OpLabel\n"
		"%tmp1 = OpLoad %v4f32 %vtxColor\n"
		"OpStore %fragColor %tmp1\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"%main2 = OpFunction %void None %fun\n"
		"%label_main2 = OpLabel\n"
		"%tmp2 = OpLoad %v4f32 %vtxColor\n"
		"%tmp3 = OpFSub %v4f32 %cval %tmp2\n"
		"%tmp4 = OpVectorInsertDynamic %v4f32 %tmp3 %c_f32_1 %c_i32_3\n"
		"OpStore %fragColor %tmp4\n"
		"OpReturn\n"
		"OpFunctionEnd\n";

	dst.spirvAsmSources.add("geom") <<
		"OpCapability Geometry\n"
		"OpCapability ClipDistance\n"
		"OpCapability CullDistance\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint Geometry %geom1_main \"geom1\" %out_gl_position %gl_in %out_color %in_color\n"
		"OpEntryPoint Geometry %geom2_main \"geom2\" %out_gl_position %gl_in %out_color %in_color\n"
		"OpExecutionMode %geom1_main Triangles\n"
		"OpExecutionMode %geom2_main Triangles\n"
		"OpExecutionMode %geom1_main OutputTriangleStrip\n"
		"OpExecutionMode %geom2_main OutputTriangleStrip\n"
		"OpExecutionMode %geom1_main OutputVertices 3\n"
		"OpExecutionMode %geom2_main OutputVertices 3\n"
		"OpName %geom1_main \"geom1\"\n"
		"OpName %geom2_main \"geom2\"\n"
		"OpName %per_vertex_in \"gl_PerVertex\"\n"
		"OpMemberName %per_vertex_in 0 \"gl_Position\"\n"
		"OpMemberName %per_vertex_in 1 \"gl_PointSize\"\n"
		"OpMemberName %per_vertex_in 2 \"gl_ClipDistance\"\n"
		"OpMemberName %per_vertex_in 3 \"gl_CullDistance\"\n"
		"OpName %gl_in \"gl_in\"\n"
		"OpName %out_color \"out_color\"\n"
		"OpName %in_color \"in_color\"\n"
		"OpDecorate %out_gl_position BuiltIn Position\n"
		"OpMemberDecorate %per_vertex_in 0 BuiltIn Position\n"
		"OpMemberDecorate %per_vertex_in 1 BuiltIn PointSize\n"
		"OpMemberDecorate %per_vertex_in 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %per_vertex_in 3 BuiltIn CullDistance\n"
		"OpDecorate %per_vertex_in Block\n"
		"OpDecorate %out_color Location 1\n"
		"OpDecorate %in_color Location 1\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%cval = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_0\n"
		"%per_vertex_in = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%a3_per_vertex_in = OpTypeArray %per_vertex_in %c_u32_3\n"
		"%ip_a3_per_vertex_in = OpTypePointer Input %a3_per_vertex_in\n"
		"%gl_in = OpVariable %ip_a3_per_vertex_in Input\n"
		"%out_color = OpVariable %op_v4f32 Output\n"
		"%in_color = OpVariable %ip_a3v4f32 Input\n"
		"%out_gl_position = OpVariable %op_v4f32 Output\n"

		"%geom1_main = OpFunction %void None %fun\n"
		"%geom1_label = OpLabel\n"
		"%geom1_gl_in_0_gl_position = OpAccessChain %ip_v4f32 %gl_in %c_i32_0 %c_i32_0\n"
		"%geom1_gl_in_1_gl_position = OpAccessChain %ip_v4f32 %gl_in %c_i32_1 %c_i32_0\n"
		"%geom1_gl_in_2_gl_position = OpAccessChain %ip_v4f32 %gl_in %c_i32_2 %c_i32_0\n"
		"%geom1_in_position_0 = OpLoad %v4f32 %geom1_gl_in_0_gl_position\n"
		"%geom1_in_position_1 = OpLoad %v4f32 %geom1_gl_in_1_gl_position\n"
		"%geom1_in_position_2 = OpLoad %v4f32 %geom1_gl_in_2_gl_position \n"
		"%geom1_in_color_0_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_0\n"
		"%geom1_in_color_1_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_1\n"
		"%geom1_in_color_2_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_2\n"
		"%geom1_in_color_0 = OpLoad %v4f32 %geom1_in_color_0_ptr\n"
		"%geom1_in_color_1 = OpLoad %v4f32 %geom1_in_color_1_ptr\n"
		"%geom1_in_color_2 = OpLoad %v4f32 %geom1_in_color_2_ptr\n"
		"OpStore %out_gl_position %geom1_in_position_0\n"
		"OpStore %out_color %geom1_in_color_0\n"
		"OpEmitVertex\n"
		"OpStore %out_gl_position %geom1_in_position_1\n"
		"OpStore %out_color %geom1_in_color_1\n"
		"OpEmitVertex\n"
		"OpStore %out_gl_position %geom1_in_position_2\n"
		"OpStore %out_color %geom1_in_color_2\n"
		"OpEmitVertex\n"
		"OpEndPrimitive\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"%geom2_main = OpFunction %void None %fun\n"
		"%geom2_label = OpLabel\n"
		"%geom2_gl_in_0_gl_position = OpAccessChain %ip_v4f32 %gl_in %c_i32_0 %c_i32_0\n"
		"%geom2_gl_in_1_gl_position = OpAccessChain %ip_v4f32 %gl_in %c_i32_1 %c_i32_0\n"
		"%geom2_gl_in_2_gl_position = OpAccessChain %ip_v4f32 %gl_in %c_i32_2 %c_i32_0\n"
		"%geom2_in_position_0 = OpLoad %v4f32 %geom2_gl_in_0_gl_position\n"
		"%geom2_in_position_1 = OpLoad %v4f32 %geom2_gl_in_1_gl_position\n"
		"%geom2_in_position_2 = OpLoad %v4f32 %geom2_gl_in_2_gl_position \n"
		"%geom2_in_color_0_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_0\n"
		"%geom2_in_color_1_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_1\n"
		"%geom2_in_color_2_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_2\n"
		"%geom2_in_color_0 = OpLoad %v4f32 %geom2_in_color_0_ptr\n"
		"%geom2_in_color_1 = OpLoad %v4f32 %geom2_in_color_1_ptr\n"
		"%geom2_in_color_2 = OpLoad %v4f32 %geom2_in_color_2_ptr\n"
		"%geom2_transformed_in_color_0 = OpFSub %v4f32 %cval %geom2_in_color_0\n"
		"%geom2_transformed_in_color_1 = OpFSub %v4f32 %cval %geom2_in_color_1\n"
		"%geom2_transformed_in_color_2 = OpFSub %v4f32 %cval %geom2_in_color_2\n"
		"%geom2_transformed_in_color_0_a = OpVectorInsertDynamic %v4f32 %geom2_transformed_in_color_0 %c_f32_1 %c_i32_3\n"
		"%geom2_transformed_in_color_1_a = OpVectorInsertDynamic %v4f32 %geom2_transformed_in_color_1 %c_f32_1 %c_i32_3\n"
		"%geom2_transformed_in_color_2_a = OpVectorInsertDynamic %v4f32 %geom2_transformed_in_color_2 %c_f32_1 %c_i32_3\n"
		"OpStore %out_gl_position %geom2_in_position_0\n"
		"OpStore %out_color %geom2_transformed_in_color_0_a\n"
		"OpEmitVertex\n"
		"OpStore %out_gl_position %geom2_in_position_1\n"
		"OpStore %out_color %geom2_transformed_in_color_1_a\n"
		"OpEmitVertex\n"
		"OpStore %out_gl_position %geom2_in_position_2\n"
		"OpStore %out_color %geom2_transformed_in_color_2_a\n"
		"OpEmitVertex\n"
		"OpEndPrimitive\n"
		"OpReturn\n"
		"OpFunctionEnd\n";

	dst.spirvAsmSources.add("tessc") <<
		"OpCapability Tessellation\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint TessellationControl %tessc1_main \"tessc1\" %out_color %gl_InvocationID %in_color %out_position %in_position %gl_TessLevelOuter %gl_TessLevelInner\n"
		"OpEntryPoint TessellationControl %tessc2_main \"tessc2\" %out_color %gl_InvocationID %in_color %out_position %in_position %gl_TessLevelOuter %gl_TessLevelInner\n"
		"OpExecutionMode %tessc1_main OutputVertices 3\n"
		"OpExecutionMode %tessc2_main OutputVertices 3\n"
		"OpName %tessc1_main \"tessc1\"\n"
		"OpName %tessc2_main \"tessc2\"\n"
		"OpName %out_color \"out_color\"\n"
		"OpName %gl_InvocationID \"gl_InvocationID\"\n"
		"OpName %in_color \"in_color\"\n"
		"OpName %out_position \"out_position\"\n"
		"OpName %in_position \"in_position\"\n"
		"OpName %gl_TessLevelOuter \"gl_TessLevelOuter\"\n"
		"OpName %gl_TessLevelInner \"gl_TessLevelInner\"\n"
		"OpDecorate %out_color Location 1\n"
		"OpDecorate %gl_InvocationID BuiltIn InvocationId\n"
		"OpDecorate %in_color Location 1\n"
		"OpDecorate %out_position Location 2\n"
		"OpDecorate %in_position Location 2\n"
		"OpDecorate %gl_TessLevelOuter Patch\n"
		"OpDecorate %gl_TessLevelOuter BuiltIn TessLevelOuter\n"
		"OpDecorate %gl_TessLevelInner Patch\n"
		"OpDecorate %gl_TessLevelInner BuiltIn TessLevelInner\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%cval = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_0\n"
		"%out_color = OpVariable %op_a3v4f32 Output\n"
		"%gl_InvocationID = OpVariable %ip_i32 Input\n"
		"%in_color = OpVariable %ip_a32v4f32 Input\n"
		"%out_position = OpVariable %op_a3v4f32 Output\n"
		"%in_position = OpVariable %ip_a32v4f32 Input\n"
		"%gl_TessLevelOuter = OpVariable %op_a4f32 Output\n"
		"%gl_TessLevelInner = OpVariable %op_a2f32 Output\n"

		"%tessc1_main = OpFunction %void None %fun\n"
		"%tessc1_label = OpLabel\n"
		"%tessc1_invocation_id = OpLoad %i32 %gl_InvocationID\n"
		"%tessc1_in_color_ptr = OpAccessChain %ip_v4f32 %in_color %tessc1_invocation_id\n"
		"%tessc1_in_position_ptr = OpAccessChain %ip_v4f32 %in_position %tessc1_invocation_id\n"
		"%tessc1_in_color_val = OpLoad %v4f32 %tessc1_in_color_ptr\n"
		"%tessc1_in_position_val = OpLoad %v4f32 %tessc1_in_position_ptr\n"
		"%tessc1_out_color_ptr = OpAccessChain %op_v4f32 %out_color %tessc1_invocation_id\n"
		"%tessc1_out_position_ptr = OpAccessChain %op_v4f32 %out_position %tessc1_invocation_id\n"
		"OpStore %tessc1_out_color_ptr %tessc1_in_color_val\n"
		"OpStore %tessc1_out_position_ptr %tessc1_in_position_val\n"
		"%tessc1_is_first_invocation = OpIEqual %bool %tessc1_invocation_id %c_i32_0\n"
		"OpSelectionMerge %tessc1_merge_label None\n"
		"OpBranchConditional %tessc1_is_first_invocation %tessc1_first_invocation %tessc1_merge_label\n"
		"%tessc1_first_invocation = OpLabel\n"
		"%tessc1_tess_outer_0 = OpAccessChain %op_f32 %gl_TessLevelOuter %c_i32_0\n"
		"%tessc1_tess_outer_1 = OpAccessChain %op_f32 %gl_TessLevelOuter %c_i32_1\n"
		"%tessc1_tess_outer_2 = OpAccessChain %op_f32 %gl_TessLevelOuter %c_i32_2\n"
		"%tessc1_tess_inner = OpAccessChain %op_f32 %gl_TessLevelInner %c_i32_0\n"
		"OpStore %tessc1_tess_outer_0 %c_f32_1\n"
		"OpStore %tessc1_tess_outer_1 %c_f32_1\n"
		"OpStore %tessc1_tess_outer_2 %c_f32_1\n"
		"OpStore %tessc1_tess_inner %c_f32_1\n"
		"OpBranch %tessc1_merge_label\n"
		"%tessc1_merge_label = OpLabel\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"%tessc2_main = OpFunction %void None %fun\n"
		"%tessc2_label = OpLabel\n"
		"%tessc2_invocation_id = OpLoad %i32 %gl_InvocationID\n"
		"%tessc2_in_color_ptr = OpAccessChain %ip_v4f32 %in_color %tessc2_invocation_id\n"
		"%tessc2_in_position_ptr = OpAccessChain %ip_v4f32 %in_position %tessc2_invocation_id\n"
		"%tessc2_in_color_val = OpLoad %v4f32 %tessc2_in_color_ptr\n"
		"%tessc2_in_position_val = OpLoad %v4f32 %tessc2_in_position_ptr\n"
		"%tessc2_out_color_ptr = OpAccessChain %op_v4f32 %out_color %tessc2_invocation_id\n"
		"%tessc2_out_position_ptr = OpAccessChain %op_v4f32 %out_position %tessc2_invocation_id\n"
		"%tessc2_transformed_color = OpFSub %v4f32 %cval %tessc2_in_color_val\n"
		"%tessc2_transformed_color_a = OpVectorInsertDynamic %v4f32 %tessc2_transformed_color %c_f32_1 %c_i32_3\n"
		"OpStore %tessc2_out_color_ptr %tessc2_transformed_color_a\n"
		"OpStore %tessc2_out_position_ptr %tessc2_in_position_val\n"
		"%tessc2_is_first_invocation = OpIEqual %bool %tessc2_invocation_id %c_i32_0\n"
		"OpSelectionMerge %tessc2_merge_label None\n"
		"OpBranchConditional %tessc2_is_first_invocation %tessc2_first_invocation %tessc2_merge_label\n"
		"%tessc2_first_invocation = OpLabel\n"
		"%tessc2_tess_outer_0 = OpAccessChain %op_f32 %gl_TessLevelOuter %c_i32_0\n"
		"%tessc2_tess_outer_1 = OpAccessChain %op_f32 %gl_TessLevelOuter %c_i32_1\n"
		"%tessc2_tess_outer_2 = OpAccessChain %op_f32 %gl_TessLevelOuter %c_i32_2\n"
		"%tessc2_tess_inner = OpAccessChain %op_f32 %gl_TessLevelInner %c_i32_0\n"
		"OpStore %tessc2_tess_outer_0 %c_f32_1\n"
		"OpStore %tessc2_tess_outer_1 %c_f32_1\n"
		"OpStore %tessc2_tess_outer_2 %c_f32_1\n"
		"OpStore %tessc2_tess_inner %c_f32_1\n"
		"OpBranch %tessc2_merge_label\n"
		"%tessc2_merge_label = OpLabel\n"
		"OpReturn\n"
		"OpFunctionEnd\n";

	dst.spirvAsmSources.add("tesse") <<
		"OpCapability Tessellation\n"
		"OpCapability ClipDistance\n"
		"OpCapability CullDistance\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint TessellationEvaluation %tesse1_main \"tesse1\" %stream %gl_tessCoord %in_position %out_color %in_color \n"
		"OpEntryPoint TessellationEvaluation %tesse2_main \"tesse2\" %stream %gl_tessCoord %in_position %out_color %in_color \n"
		"OpExecutionMode %tesse1_main Triangles\n"
		"OpExecutionMode %tesse1_main SpacingEqual\n"
		"OpExecutionMode %tesse1_main VertexOrderCcw\n"
		"OpExecutionMode %tesse2_main Triangles\n"
		"OpExecutionMode %tesse2_main SpacingEqual\n"
		"OpExecutionMode %tesse2_main VertexOrderCcw\n"
		"OpName %tesse1_main \"tesse1\"\n"
		"OpName %tesse2_main \"tesse2\"\n"
		"OpName %per_vertex_out \"gl_PerVertex\"\n"
		"OpMemberName %per_vertex_out 0 \"gl_Position\"\n"
		"OpMemberName %per_vertex_out 1 \"gl_PointSize\"\n"
		"OpMemberName %per_vertex_out 2 \"gl_ClipDistance\"\n"
		"OpMemberName %per_vertex_out 3 \"gl_CullDistance\"\n"
		"OpName %stream \"\"\n"
		"OpName %gl_tessCoord \"gl_TessCoord\"\n"
		"OpName %in_position \"in_position\"\n"
		"OpName %out_color \"out_color\"\n"
		"OpName %in_color \"in_color\"\n"
		"OpMemberDecorate %per_vertex_out 0 BuiltIn Position\n"
		"OpMemberDecorate %per_vertex_out 1 BuiltIn PointSize\n"
		"OpMemberDecorate %per_vertex_out 2 BuiltIn ClipDistance\n"
		"OpMemberDecorate %per_vertex_out 3 BuiltIn CullDistance\n"
		"OpDecorate %per_vertex_out Block\n"
		"OpDecorate %gl_tessCoord BuiltIn TessCoord\n"
		"OpDecorate %in_position Location 2\n"
		"OpDecorate %out_color Location 1\n"
		"OpDecorate %in_color Location 1\n"
		SPIRV_ASSEMBLY_TYPES
		SPIRV_ASSEMBLY_CONSTANTS
		SPIRV_ASSEMBLY_ARRAYS
		"%cval = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_0\n"
		"%per_vertex_out = OpTypeStruct %v4f32 %f32 %a1f32 %a1f32\n"
		"%op_per_vertex_out = OpTypePointer Output %per_vertex_out\n"
		"%stream = OpVariable %op_per_vertex_out Output\n"
		"%gl_tessCoord = OpVariable %ip_v3f32 Input\n"
		"%in_position = OpVariable %ip_a32v4f32 Input\n"
		"%out_color = OpVariable %op_v4f32 Output\n"
		"%in_color = OpVariable %ip_a32v4f32 Input\n"

		"%tesse1_main = OpFunction %void None %fun\n"
		"%tesse1_label = OpLabel\n"
		"%tesse1_tc_0_ptr = OpAccessChain %ip_f32 %gl_tessCoord %c_u32_0\n"
		"%tesse1_tc_1_ptr = OpAccessChain %ip_f32 %gl_tessCoord %c_u32_1\n"
		"%tesse1_tc_2_ptr = OpAccessChain %ip_f32 %gl_tessCoord %c_u32_2\n"
		"%tesse1_tc_0 = OpLoad %f32 %tesse1_tc_0_ptr\n"
		"%tesse1_tc_1 = OpLoad %f32 %tesse1_tc_1_ptr\n"
		"%tesse1_tc_2 = OpLoad %f32 %tesse1_tc_2_ptr\n"
		"%tesse1_in_pos_0_ptr = OpAccessChain %ip_v4f32 %in_position %c_i32_0\n"
		"%tesse1_in_pos_1_ptr = OpAccessChain %ip_v4f32 %in_position %c_i32_1\n"
		"%tesse1_in_pos_2_ptr = OpAccessChain %ip_v4f32 %in_position %c_i32_2\n"
		"%tesse1_in_pos_0 = OpLoad %v4f32 %tesse1_in_pos_0_ptr\n"
		"%tesse1_in_pos_1 = OpLoad %v4f32 %tesse1_in_pos_1_ptr\n"
		"%tesse1_in_pos_2 = OpLoad %v4f32 %tesse1_in_pos_2_ptr\n"
		"%tesse1_in_pos_0_weighted = OpVectorTimesScalar %v4f32 %tesse1_in_pos_0 %tesse1_tc_0\n"
		"%tesse1_in_pos_1_weighted = OpVectorTimesScalar %v4f32 %tesse1_in_pos_1 %tesse1_tc_1\n"
		"%tesse1_in_pos_2_weighted = OpVectorTimesScalar %v4f32 %tesse1_in_pos_2 %tesse1_tc_2\n"
		"%tesse1_out_pos_ptr = OpAccessChain %op_v4f32 %stream %c_i32_0\n"
		"%tesse1_in_pos_0_plus_pos_1 = OpFAdd %v4f32 %tesse1_in_pos_0_weighted %tesse1_in_pos_1_weighted\n"
		"%tesse1_computed_out = OpFAdd %v4f32 %tesse1_in_pos_0_plus_pos_1 %tesse1_in_pos_2_weighted\n"
		"OpStore %tesse1_out_pos_ptr %tesse1_computed_out\n"
		"%tesse1_in_clr_0_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_0\n"
		"%tesse1_in_clr_1_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_1\n"
		"%tesse1_in_clr_2_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_2\n"
		"%tesse1_in_clr_0 = OpLoad %v4f32 %tesse1_in_clr_0_ptr\n"
		"%tesse1_in_clr_1 = OpLoad %v4f32 %tesse1_in_clr_1_ptr\n"
		"%tesse1_in_clr_2 = OpLoad %v4f32 %tesse1_in_clr_2_ptr\n"
		"%tesse1_in_clr_0_weighted = OpVectorTimesScalar %v4f32 %tesse1_in_clr_0 %tesse1_tc_0\n"
		"%tesse1_in_clr_1_weighted = OpVectorTimesScalar %v4f32 %tesse1_in_clr_1 %tesse1_tc_1\n"
		"%tesse1_in_clr_2_weighted = OpVectorTimesScalar %v4f32 %tesse1_in_clr_2 %tesse1_tc_2\n"
		"%tesse1_in_clr_0_plus_col_1 = OpFAdd %v4f32 %tesse1_in_clr_0_weighted %tesse1_in_clr_1_weighted\n"
		"%tesse1_computed_clr = OpFAdd %v4f32 %tesse1_in_clr_0_plus_col_1 %tesse1_in_clr_2_weighted\n"
		"OpStore %out_color %tesse1_computed_clr\n"
		"OpReturn\n"
		"OpFunctionEnd\n"

		"%tesse2_main = OpFunction %void None %fun\n"
		"%tesse2_label = OpLabel\n"
		"%tesse2_tc_0_ptr = OpAccessChain %ip_f32 %gl_tessCoord %c_u32_0\n"
		"%tesse2_tc_1_ptr = OpAccessChain %ip_f32 %gl_tessCoord %c_u32_1\n"
		"%tesse2_tc_2_ptr = OpAccessChain %ip_f32 %gl_tessCoord %c_u32_2\n"
		"%tesse2_tc_0 = OpLoad %f32 %tesse2_tc_0_ptr\n"
		"%tesse2_tc_1 = OpLoad %f32 %tesse2_tc_1_ptr\n"
		"%tesse2_tc_2 = OpLoad %f32 %tesse2_tc_2_ptr\n"
		"%tesse2_in_pos_0_ptr = OpAccessChain %ip_v4f32 %in_position %c_i32_0\n"
		"%tesse2_in_pos_1_ptr = OpAccessChain %ip_v4f32 %in_position %c_i32_1\n"
		"%tesse2_in_pos_2_ptr = OpAccessChain %ip_v4f32 %in_position %c_i32_2\n"
		"%tesse2_in_pos_0 = OpLoad %v4f32 %tesse2_in_pos_0_ptr\n"
		"%tesse2_in_pos_1 = OpLoad %v4f32 %tesse2_in_pos_1_ptr\n"
		"%tesse2_in_pos_2 = OpLoad %v4f32 %tesse2_in_pos_2_ptr\n"
		"%tesse2_in_pos_0_weighted = OpVectorTimesScalar %v4f32 %tesse2_in_pos_0 %tesse2_tc_0\n"
		"%tesse2_in_pos_1_weighted = OpVectorTimesScalar %v4f32 %tesse2_in_pos_1 %tesse2_tc_1\n"
		"%tesse2_in_pos_2_weighted = OpVectorTimesScalar %v4f32 %tesse2_in_pos_2 %tesse2_tc_2\n"
		"%tesse2_out_pos_ptr = OpAccessChain %op_v4f32 %stream %c_i32_0\n"
		"%tesse2_in_pos_0_plus_pos_1 = OpFAdd %v4f32 %tesse2_in_pos_0_weighted %tesse2_in_pos_1_weighted\n"
		"%tesse2_computed_out = OpFAdd %v4f32 %tesse2_in_pos_0_plus_pos_1 %tesse2_in_pos_2_weighted\n"
		"OpStore %tesse2_out_pos_ptr %tesse2_computed_out\n"
		"%tesse2_in_clr_0_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_0\n"
		"%tesse2_in_clr_1_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_1\n"
		"%tesse2_in_clr_2_ptr = OpAccessChain %ip_v4f32 %in_color %c_i32_2\n"
		"%tesse2_in_clr_0 = OpLoad %v4f32 %tesse2_in_clr_0_ptr\n"
		"%tesse2_in_clr_1 = OpLoad %v4f32 %tesse2_in_clr_1_ptr\n"
		"%tesse2_in_clr_2 = OpLoad %v4f32 %tesse2_in_clr_2_ptr\n"
		"%tesse2_in_clr_0_weighted = OpVectorTimesScalar %v4f32 %tesse2_in_clr_0 %tesse2_tc_0\n"
		"%tesse2_in_clr_1_weighted = OpVectorTimesScalar %v4f32 %tesse2_in_clr_1 %tesse2_tc_1\n"
		"%tesse2_in_clr_2_weighted = OpVectorTimesScalar %v4f32 %tesse2_in_clr_2 %tesse2_tc_2\n"
		"%tesse2_in_clr_0_plus_col_1 = OpFAdd %v4f32 %tesse2_in_clr_0_weighted %tesse2_in_clr_1_weighted\n"
		"%tesse2_computed_clr = OpFAdd %v4f32 %tesse2_in_clr_0_plus_col_1 %tesse2_in_clr_2_weighted\n"
		"%tesse2_clr_transformed = OpFSub %v4f32 %cval %tesse2_computed_clr\n"
		"%tesse2_clr_transformed_a = OpVectorInsertDynamic %v4f32 %tesse2_clr_transformed %c_f32_1 %c_i32_3\n"
		"OpStore %out_color %tesse2_clr_transformed_a\n"
		"OpReturn\n"
		"OpFunctionEnd\n";
}

bool compare16BitFloat (float original, deUint16 returned, RoundingModeFlags flags, tcu::TestLog& log)
{
	// We only support RTE, RTZ, or both.
	DE_ASSERT(static_cast<int>(flags) > 0 && static_cast<int>(flags) < 4);

	const Float32	originalFloat	(original);
	const Float16	returnedFloat	(returned);

	// Zero are turned into zero under both RTE and RTZ.
	if (originalFloat.isZero())
	{
		if (returnedFloat.isZero())
			return true;

		log << TestLog::Message << "Error: expected zero but returned " << returned << TestLog::EndMessage;
		return false;
	}

	// Any denormalized value input into a shader may be flushed to 0.
	if (originalFloat.isDenorm() && returnedFloat.isZero())
		return true;

	// Inf are always turned into Inf with the same sign, too.
	if (originalFloat.isInf())
	{
		if (returnedFloat.isInf() && originalFloat.signBit() == returnedFloat.signBit())
			return true;

		log << TestLog::Message << "Error: expected Inf but returned " << returned << TestLog::EndMessage;
		return false;
	}

	// NaN are always turned into NaN, too.
	if (originalFloat.isNaN())
	{
		if (returnedFloat.isNaN())
			return true;

		log << TestLog::Message << "Error: expected NaN but returned " << returned << TestLog::EndMessage;
		return false;
	}

	// Check all rounding modes
	for (int bitNdx = 0; bitNdx < 2; ++bitNdx)
	{
		if ((flags & (1u << bitNdx)) == 0)
			continue;	// This rounding mode is not selected.

		const Float16	expectedFloat	(deFloat32To16Round(original, deRoundingMode(bitNdx)));

		// Any denormalized value potentially generated by any instruction in a shader may be flushed to 0.
		if (expectedFloat.isDenorm() && returnedFloat.isZero())
			return true;

		// If not matched in the above cases, they should have the same bit pattern.
		if (expectedFloat.bits() == returnedFloat.bits())
			return true;
	}

	log << TestLog::Message << "Error: found unmatched 32-bit and 16-bit floats: " << originalFloat.bits() << " vs " << returned << TestLog::EndMessage;
	return false;
}

bool compare32BitFloat (float expected, float returned, tcu::TestLog& log)
{
	const Float32	expectedFloat	(expected);
	const Float32	returnedFloat	(returned);

	// Any denormalized value potentially generated by any instruction in a shader may be flushed to 0.
	if (expectedFloat.isDenorm() && returnedFloat.isZero())
		return true;

	{
		const Float16	originalFloat	(deFloat32To16(expected));

		// Any denormalized value input into a shader may be flushed to 0.
		if (originalFloat.isDenorm() && returnedFloat.isZero())
			return true;
	}

	if (expectedFloat.isNaN())
	{
		if (returnedFloat.isNaN())
			return true;

		log << TestLog::Message << "Error: expected NaN but returned " << returned << TestLog::EndMessage;
		return false;
	}

	if (returned == expected)
		return true;

	log << TestLog::Message << "Error: found unmatched 32-bit float: expected " << expectedFloat.bits() << " vs. returned " << returnedFloat.bits() << TestLog::EndMessage;
	return false;
}

Move<VkBuffer> createBufferForResource (const DeviceInterface& vk, const VkDevice vkDevice, const Resource& resource, deUint32 queueFamilyIndex)
{
	vector<deUint8>	resourceBytes;
	resource.second->getBytes(resourceBytes);

	const VkBufferCreateInfo	resourceBufferParams	=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,								// sType
		DE_NULL,															// pNext
		(VkBufferCreateFlags)0,												// flags
		(VkDeviceSize)resourceBytes.size(),									// size
		(VkBufferUsageFlags)getMatchingBufferUsageFlagBit(resource.first),	// usage
		VK_SHARING_MODE_EXCLUSIVE,											// sharingMode
		1u,																	// queueFamilyCount
		&queueFamilyIndex,													// pQueueFamilyIndices
	};

	return createBuffer(vk, vkDevice, &resourceBufferParams);
}

Move<VkImage> createImageForResource (const DeviceInterface& vk, const VkDevice vkDevice, const Resource& resource, deUint32 queueFamilyIndex)
{
	const VkImageCreateInfo	resourceImageParams	=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,								//	VkStructureType		sType;
		DE_NULL,															//	const void*			pNext;
		0u,																	//	VkImageCreateFlags	flags;
		VK_IMAGE_TYPE_2D,													//	VkImageType			imageType;
		VK_FORMAT_R32G32B32A32_SFLOAT,										//	VkFormat			format;
		{ 8, 8, 1 },														//	VkExtent3D			extent;
		1u,																	//	deUint32			mipLevels;
		1u,																	//	deUint32			arraySize;
		VK_SAMPLE_COUNT_1_BIT,												//	deUint32			samples;
		VK_IMAGE_TILING_OPTIMAL,											//	VkImageTiling		tiling;
		getMatchingImageUsageFlags(resource.first),							//	VkImageUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,											//	VkSharingMode		sharingMode;
		1u,																	//	deUint32			queueFamilyCount;
		&queueFamilyIndex,													//	const deUint32*		pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED											//	VkImageLayout		initialLayout;
	};

	return createImage(vk, vkDevice, &resourceImageParams);
}

void copyBufferToImage (const DeviceInterface& vk, const VkDevice& device, const VkQueue& queue, VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image)
{
	const VkBufferImageCopy			copyRegion			=
	{
		0u,												// VkDeviceSize				bufferOffset;
		0u,												// deUint32					bufferRowLength;
		0u,												// deUint32					bufferImageHeight;
		{
			VK_IMAGE_ASPECT_COLOR_BIT,						// VkImageAspectFlags		aspect;
			0u,												// deUint32					mipLevel;
			0u,												// deUint32					baseArrayLayer;
			1u,												// deUint32					layerCount;
		},												// VkImageSubresourceLayers	imageSubresource;
		{ 0, 0, 0 },									// VkOffset3D				imageOffset;
		{ 8, 8, 1 }										// VkExtent3D				imageExtent;
	};

	// Copy buffer to image
	const VkCommandBufferBeginInfo	cmdBufferBeginInfo	=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType							sType;
		DE_NULL,										// const void*								pNext;
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,	// VkCommandBufferUsageFlags				flags;
		DE_NULL											// const VkCommandBufferInheritanceInfo*	pInheritanceInfo;
	};

	const VkImageMemoryBarrier		imageBarriers[]		=
	{
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			DE_NULL,									// VkAccessFlags			srcAccessMask;
			VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,					// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32					dstQueueFamilyIndex;
			image,										// VkImage					image;
			{											// VkImageSubresourceRange	subresourceRange;
				VK_IMAGE_ASPECT_COLOR_BIT,		// VkImageAspectFlags	aspectMask;
				0u,								// deUint32				baseMipLevel;
				1u,								// deUint32				mipLevels;
				0u,								// deUint32				baseArraySlice;
				1u								// deUint32				arraySize;
			}
		},
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags			srcAccessMask;
			VK_ACCESS_SHADER_READ_BIT,					// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,		// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_GENERAL,					// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,					// deUint32					dstQueueFamilyIndex;
			image,										// VkImage					image;
			{											// VkImageSubresourceRange	subresourceRange;
				VK_IMAGE_ASPECT_COLOR_BIT,		// VkImageAspectFlags	aspectMask;
				0u,								// deUint32				baseMipLevel;
				1u,								// deUint32				mipLevels;
				0u,								// deUint32				baseArraySlice;
				1u								// deUint32				arraySize;
			}
		},
	};

	VK_CHECK(vk.beginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
	vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL,
		0u, DE_NULL, 1u, &imageBarriers[0]);
	vk.cmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copyRegion);
	vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL,
		0, (const VkBufferMemoryBarrier*)DE_NULL, 1, &imageBarriers[1]);

	VK_CHECK(vk.endCommandBuffer(cmdBuffer));

	{
		const VkFenceCreateInfo	fenceParams	=
		{
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,	//	VkStructureType		sType;
			DE_NULL,								//	const void*			pNext;
			0u,										//	VkFenceCreateFlags	flags;
		};

		const Unique<VkFence>	fence		(createFence(vk, device, &fenceParams));
		const VkSubmitInfo		submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType				sType;
			DE_NULL,								// const void*					pNext;
			0u,										// deUint32						waitSemaphoreCount;
			DE_NULL,								// const VkSemaphore*			pWaitSemaphores;
			DE_NULL,								// const VkPipelineStageFlags*	pWaitDstStageMask;
			1u,										// deUint32						commandBufferCount;
			&cmdBuffer,								// const VkCommandBuffer*		pCommandBuffers;
			0u,										// deUint32						signalSemaphoreCount;
			DE_NULL									// const VkSemaphore*			pSignalSemaphores;
		};

		VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
	}
}

TestStatus runAndVerifyDefaultPipeline (Context& context, InstanceContext instance)
{
	const InstanceInterface&					vkInstance				= context.getInstanceInterface();
	const VkPhysicalDevice						vkPhysicalDevice		= context.getPhysicalDevice();
	const deUint32								queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	// Create a dedicated logic device with required extensions enabled for this test case.
	const tcu::UVec2							renderSize				(256, 256);
	const int									testSpecificSeed		= 31354125;
	const int									seed					= context.getTestContext().getCommandLine().getBaseSeed() ^ testSpecificSeed;
	bool										supportsGeometry		= false;
	bool										supportsTessellation	= false;
	bool										hasTessellation         = false;
	const bool									hasPushConstants		= !instance.pushConstants.empty();
	const deUint32								numResources			= static_cast<deUint32>(instance.resources.inputs.size() + instance.resources.outputs.size());
	const bool									needInterface			= !instance.interfaces.empty();
	const VkPhysicalDeviceFeatures&				features				= context.getDeviceFeatures();

	supportsGeometry		= features.geometryShader == VK_TRUE;
	supportsTessellation	= features.tessellationShader == VK_TRUE;
	hasTessellation			= (instance.requiredStages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) ||
								(instance.requiredStages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

	if (hasTessellation && !supportsTessellation)
	{
		TCU_THROW(NotSupportedError, "Tessellation not supported");
	}

	if ((instance.requiredStages & VK_SHADER_STAGE_GEOMETRY_BIT) &&
		!supportsGeometry)
	{
		TCU_THROW(NotSupportedError, "Geometry not supported");
	}

	{
		for (deUint32 featureNdx = 0; featureNdx < instance.requiredDeviceFeatures.size(); ++featureNdx)
		{
			const string& feature = instance.requiredDeviceFeatures[featureNdx];

			if (feature == "shaderInt16")
			{
				if (features.shaderInt16 != VK_TRUE)
					TCU_THROW(NotSupportedError, "Device feature not supported: shaderInt16");
			}
			else if (feature == "shaderInt64")
			{
				if (features.shaderInt64 != VK_TRUE)
					TCU_THROW(NotSupportedError, "Device feature not supported: shaderInt64");
			}
			else if (feature == "shaderFloat64")
			{
				if (features.shaderFloat64 != VK_TRUE)
					TCU_THROW(NotSupportedError, "Device feature not supported: shaderFloat64");
			}
			else
			{
				TCU_THROW(InternalError, (std::string("Unimplemented physical device feature: ") + feature).c_str());
			}
		}
	}

	// 16bit storage features
	{
		if (!is16BitStorageFeaturesSupported(vkInstance, vkPhysicalDevice, context.getInstanceExtensions(), instance.requestedFeatures.ext16BitStorage))
			TCU_THROW(NotSupportedError, "Requested 16bit storage features not supported");
	}

	// Variable Pointers features
	{
		if (!isVariablePointersFeaturesSupported(vkInstance, vkPhysicalDevice, context.getInstanceExtensions(), instance.requestedFeatures.extVariablePointers))
			TCU_THROW(NotSupportedError, "Requested Variable Pointer features not supported");

		if (instance.requestedFeatures.extVariablePointers)
		{
			// The device doesn't have the vertexPipelineStoresAndAtomics feature, but the test requires the feature for
			// vertex, tesselation, and geometry stages.
			if (features.vertexPipelineStoresAndAtomics == DE_FALSE &&
				instance.requestedFeatures.coreFeatures.vertexPipelineStoresAndAtomics == DE_TRUE &&
			    (instance.customizedStages & vk::VK_SHADER_STAGE_VERTEX_BIT ||
				 instance.customizedStages & vk::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT ||
				 instance.customizedStages & vk::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT ||
				 instance.customizedStages & vk::VK_SHADER_STAGE_GEOMETRY_BIT))
				TCU_THROW(NotSupportedError, "This VK_KHR_variable_pointers extension test requires vertexPipelineStoresAndAtomics device feature.");

			// The device doesn't have the fragmentStoresAndAtomics feature, but the test requires this feature for the fragment stage.
			if (features.fragmentStoresAndAtomics == DE_FALSE &&
			    instance.requestedFeatures.coreFeatures.fragmentStoresAndAtomics == DE_TRUE &&
				instance.customizedStages & vk::VK_SHADER_STAGE_FRAGMENT_BIT)
				TCU_THROW(NotSupportedError, "This VK_KHR_variable_pointers extension test requires fragmentStoresAndAtomics device feature.");
		}
	}

	// defer device and other resource creation until after feature checks
	const Unique<VkDevice>						vkDevice				(createDeviceWithExtensions(context, queueFamilyIndex, context.getDeviceExtensions(), instance.requiredDeviceExtensions));
	const DeviceDriver							vk						(vkInstance, *vkDevice);
	const VkQueue								queue					= getDeviceQueue(vk, *vkDevice, queueFamilyIndex, 0);
	const de::UniquePtr<Allocator>				allocatorUptr			(createAllocator(vkInstance, vkPhysicalDevice, vk, *vkDevice));
	Allocator&									allocator				= *allocatorUptr;
	vector<ModuleHandleSp>						modules;
	map<VkShaderStageFlagBits, VkShaderModule>	moduleByStage;


	de::Random(seed).shuffle(instance.inputColors, instance.inputColors+4);
	de::Random(seed).shuffle(instance.outputColors, instance.outputColors+4);
	const Vec4								vertexData[]			=
	{
		// Upper left corner:
		Vec4(-1.0f, -1.0f, 0.0f, 1.0f), instance.inputColors[0].toVec(),
		Vec4(-0.5f, -1.0f, 0.0f, 1.0f), instance.inputColors[0].toVec(),
		Vec4(-1.0f, -0.5f, 0.0f, 1.0f), instance.inputColors[0].toVec(),

		// Upper right corner:
		Vec4(+0.5f, -1.0f, 0.0f, 1.0f), instance.inputColors[1].toVec(),
		Vec4(+1.0f, -1.0f, 0.0f, 1.0f), instance.inputColors[1].toVec(),
		Vec4(+1.0f, -0.5f, 0.0f, 1.0f), instance.inputColors[1].toVec(),

		// Lower left corner:
		Vec4(-1.0f, +0.5f, 0.0f, 1.0f), instance.inputColors[2].toVec(),
		Vec4(-0.5f, +1.0f, 0.0f, 1.0f), instance.inputColors[2].toVec(),
		Vec4(-1.0f, +1.0f, 0.0f, 1.0f), instance.inputColors[2].toVec(),

		// Lower right corner:
		Vec4(+1.0f, +0.5f, 0.0f, 1.0f), instance.inputColors[3].toVec(),
		Vec4(+1.0f, +1.0f, 0.0f, 1.0f), instance.inputColors[3].toVec(),
		Vec4(+0.5f, +1.0f, 0.0f, 1.0f), instance.inputColors[3].toVec()
	};
	const size_t							singleVertexDataSize	= 2 * sizeof(Vec4);
	const size_t							vertexCount				= sizeof(vertexData) / singleVertexDataSize;

	Move<VkBuffer>							vertexInputBuffer;
	de::MovePtr<Allocation>					vertexInputMemory;
	Move<VkBuffer>							fragOutputBuffer;
	de::MovePtr<Allocation>					fragOutputMemory;
	Move<VkImage>							fragOutputImage;
	de::MovePtr<Allocation>					fragOutputImageMemory;
	Move<VkImageView>						fragOutputImageView;

	const VkBufferCreateInfo				vertexBufferParams		=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	//	VkStructureType		sType;
		DE_NULL,								//	const void*			pNext;
		0u,										//	VkBufferCreateFlags	flags;
		(VkDeviceSize)sizeof(vertexData),		//	VkDeviceSize		size;
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,		//	VkBufferUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,				//	VkSharingMode		sharingMode;
		1u,										//	deUint32			queueFamilyCount;
		&queueFamilyIndex,						//	const deUint32*		pQueueFamilyIndices;
	};
	const Unique<VkBuffer>					vertexBuffer			(createBuffer(vk, *vkDevice, &vertexBufferParams));
	const UniquePtr<Allocation>				vertexBufferMemory		(allocator.allocate(getBufferMemoryRequirements(vk, *vkDevice, *vertexBuffer), MemoryRequirement::HostVisible));

	VK_CHECK(vk.bindBufferMemory(*vkDevice, *vertexBuffer, vertexBufferMemory->getMemory(), vertexBufferMemory->getOffset()));

	const VkDeviceSize						imageSizeBytes			= (VkDeviceSize)(sizeof(deUint32)*renderSize.x()*renderSize.y());
	const VkBufferCreateInfo				readImageBufferParams	=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		//	VkStructureType		sType;
		DE_NULL,									//	const void*			pNext;
		0u,											//	VkBufferCreateFlags	flags;
		imageSizeBytes,								//	VkDeviceSize		size;
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,			//	VkBufferUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,					//	VkSharingMode		sharingMode;
		1u,											//	deUint32			queueFamilyCount;
		&queueFamilyIndex,							//	const deUint32*		pQueueFamilyIndices;
	};
	const Unique<VkBuffer>					readImageBuffer			(createBuffer(vk, *vkDevice, &readImageBufferParams));
	const UniquePtr<Allocation>				readImageBufferMemory	(allocator.allocate(getBufferMemoryRequirements(vk, *vkDevice, *readImageBuffer), MemoryRequirement::HostVisible));

	VK_CHECK(vk.bindBufferMemory(*vkDevice, *readImageBuffer, readImageBufferMemory->getMemory(), readImageBufferMemory->getOffset()));

	VkImageCreateInfo						imageParams				=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,									//	VkStructureType		sType;
		DE_NULL,																//	const void*			pNext;
		0u,																		//	VkImageCreateFlags	flags;
		VK_IMAGE_TYPE_2D,														//	VkImageType			imageType;
		VK_FORMAT_R8G8B8A8_UNORM,												//	VkFormat			format;
		{ renderSize.x(), renderSize.y(), 1 },									//	VkExtent3D			extent;
		1u,																		//	deUint32			mipLevels;
		1u,																		//	deUint32			arraySize;
		VK_SAMPLE_COUNT_1_BIT,													//	deUint32			samples;
		VK_IMAGE_TILING_OPTIMAL,												//	VkImageTiling		tiling;
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,	//	VkImageUsageFlags	usage;
		VK_SHARING_MODE_EXCLUSIVE,												//	VkSharingMode		sharingMode;
		1u,																		//	deUint32			queueFamilyCount;
		&queueFamilyIndex,														//	const deUint32*		pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,												//	VkImageLayout		initialLayout;
	};

	const Unique<VkImage>					image					(createImage(vk, *vkDevice, &imageParams));
	const UniquePtr<Allocation>				imageMemory				(allocator.allocate(getImageMemoryRequirements(vk, *vkDevice, *image), MemoryRequirement::Any));

	VK_CHECK(vk.bindImageMemory(*vkDevice, *image, imageMemory->getMemory(), imageMemory->getOffset()));

	if (needInterface)
	{
		// The pipeline renders four triangles, each with three vertexes.
		// Test instantialization only provides four data points, each
		// for one triangle. So we need allocate space of three times of
		// input buffer's size.
		vector<deUint8>							inputBufferBytes;
		instance.interfaces.getInputBuffer()->getBytes(inputBufferBytes);

		const deUint32							inputNumBytes			= deUint32(inputBufferBytes.size() * 3);
		// Create an additional buffer and backing memory for one input variable.
		const VkBufferCreateInfo				vertexInputParams		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		//	VkStructureType		sType;
			DE_NULL,									//	const void*			pNext;
			0u,											//	VkBufferCreateFlags	flags;
			inputNumBytes,								//	VkDeviceSize		size;
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,			//	VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					//	VkSharingMode		sharingMode;
			1u,											//	deUint32			queueFamilyCount;
			&queueFamilyIndex,							//	const deUint32*		pQueueFamilyIndices;
		};

		vertexInputBuffer = createBuffer(vk, *vkDevice, &vertexInputParams);
		vertexInputMemory = allocator.allocate(getBufferMemoryRequirements(vk, *vkDevice, *vertexInputBuffer), MemoryRequirement::HostVisible);
		VK_CHECK(vk.bindBufferMemory(*vkDevice, *vertexInputBuffer, vertexInputMemory->getMemory(), vertexInputMemory->getOffset()));

		// Create an additional buffer and backing memory for an output variable.
		const VkDeviceSize						fragOutputImgSize		= (VkDeviceSize)(instance.interfaces.getOutputType().getNumBytes() * renderSize.x() * renderSize.y());
		const VkBufferCreateInfo				fragOutputParams		=
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		//	VkStructureType		sType;
			DE_NULL,									//	const void*			pNext;
			0u,											//	VkBufferCreateFlags	flags;
			fragOutputImgSize,							//	VkDeviceSize		size;
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,			//	VkBufferUsageFlags	usage;
			VK_SHARING_MODE_EXCLUSIVE,					//	VkSharingMode		sharingMode;
			1u,											//	deUint32			queueFamilyCount;
			&queueFamilyIndex,							//	const deUint32*		pQueueFamilyIndices;
		};
		fragOutputBuffer = createBuffer(vk, *vkDevice, &fragOutputParams);
		fragOutputMemory = allocator.allocate(getBufferMemoryRequirements(vk, *vkDevice, *fragOutputBuffer), MemoryRequirement::HostVisible);
		VK_CHECK(vk.bindBufferMemory(*vkDevice, *fragOutputBuffer, fragOutputMemory->getMemory(), fragOutputMemory->getOffset()));

		// Create an additional image and backing memory for attachment.
		// Reuse the previous imageParams since we only need to change the image format.
		imageParams.format		= instance.interfaces.getOutputType().getVkFormat();

		// Check the usage bits on the given image format are supported.
		requireFormatUsageSupport(vkInstance, vkPhysicalDevice, imageParams.format, imageParams.tiling, imageParams.usage);

		fragOutputImage			= createImage(vk, *vkDevice, &imageParams);
		fragOutputImageMemory	= allocator.allocate(getImageMemoryRequirements(vk, *vkDevice, *fragOutputImage), MemoryRequirement::Any);

		VK_CHECK(vk.bindImageMemory(*vkDevice, *fragOutputImage, fragOutputImageMemory->getMemory(), fragOutputImageMemory->getOffset()));
	}

	vector<VkAttachmentDescription>			colorAttDescs;
	vector<VkAttachmentReference>			colorAttRefs;
	{
		const VkAttachmentDescription		attDesc					=
		{
			0u,												//	VkAttachmentDescriptionFlags	flags;
			VK_FORMAT_R8G8B8A8_UNORM,						//	VkFormat						format;
			VK_SAMPLE_COUNT_1_BIT,							//	deUint32						samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,					//	VkAttachmentLoadOp				loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,					//	VkAttachmentStoreOp				storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,				//	VkAttachmentLoadOp				stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,				//	VkAttachmentStoreOp				stencilStoreOp;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		//	VkImageLayout					initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		//	VkImageLayout					finalLayout;
		};
		colorAttDescs.push_back(attDesc);

		const VkAttachmentReference			attRef					=
		{
			0u,												//	deUint32		attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		//	VkImageLayout	layout;
		};
		colorAttRefs.push_back(attRef);
	}

	if (needInterface)
	{
		const VkAttachmentDescription		attDesc					=
		{
			0u,													//	VkAttachmentDescriptionFlags	flags;
			instance.interfaces.getOutputType().getVkFormat(),	//	VkFormat						format;
			VK_SAMPLE_COUNT_1_BIT,								//	deUint32						samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,						//	VkAttachmentLoadOp				loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,						//	VkAttachmentStoreOp				storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,					//	VkAttachmentLoadOp				stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,					//	VkAttachmentStoreOp				stencilStoreOp;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			//	VkImageLayout					initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			//	VkImageLayout					finalLayout;
		};
		colorAttDescs.push_back(attDesc);

		const VkAttachmentReference			attRef					=
		{
			1u,												//	deUint32		attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		//	VkImageLayout	layout;
		};
		colorAttRefs.push_back(attRef);
	}

	VkSubpassDescription					subpassDesc				=
	{
		0u,												//	VkSubpassDescriptionFlags		flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,				//	VkPipelineBindPoint				pipelineBindPoint;
		0u,												//	deUint32						inputCount;
		DE_NULL,										//	const VkAttachmentReference*	pInputAttachments;
		1u,												//	deUint32						colorCount;
		colorAttRefs.data(),							//	const VkAttachmentReference*	pColorAttachments;
		DE_NULL,										//	const VkAttachmentReference*	pResolveAttachments;
		DE_NULL,										//	const VkAttachmentReference*	pDepthStencilAttachment;
		0u,												//	deUint32						preserveCount;
		DE_NULL,										//	const VkAttachmentReference*	pPreserveAttachments;

	};
	VkRenderPassCreateInfo					renderPassParams		=
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,		//	VkStructureType					sType;
		DE_NULL,										//	const void*						pNext;
		(VkRenderPassCreateFlags)0,
		1u,												//	deUint32						attachmentCount;
		colorAttDescs.data(),							//	const VkAttachmentDescription*	pAttachments;
		1u,												//	deUint32						subpassCount;
		&subpassDesc,									//	const VkSubpassDescription*		pSubpasses;
		0u,												//	deUint32						dependencyCount;
		DE_NULL,										//	const VkSubpassDependency*		pDependencies;
	};

	if (needInterface)
	{
		subpassDesc.colorAttachmentCount += 1;
		renderPassParams.attachmentCount += 1;
	}

	const Unique<VkRenderPass>				renderPass				(createRenderPass(vk, *vkDevice, &renderPassParams));

	const VkImageViewCreateInfo				colorAttViewParams		=
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		//	VkStructureType				sType;
		DE_NULL,										//	const void*					pNext;
		0u,												//	VkImageViewCreateFlags		flags;
		*image,											//	VkImage						image;
		VK_IMAGE_VIEW_TYPE_2D,							//	VkImageViewType				viewType;
		VK_FORMAT_R8G8B8A8_UNORM,						//	VkFormat					format;
		{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		},												//	VkChannelMapping			channels;
		{
			VK_IMAGE_ASPECT_COLOR_BIT,						//	VkImageAspectFlags	aspectMask;
			0u,												//	deUint32			baseMipLevel;
			1u,												//	deUint32			mipLevels;
			0u,												//	deUint32			baseArrayLayer;
			1u,												//	deUint32			arraySize;
		},												//	VkImageSubresourceRange		subresourceRange;
	};
	const Unique<VkImageView>				colorAttView			(createImageView(vk, *vkDevice, &colorAttViewParams));

	vector<VkImageView>						attViews;
	attViews.push_back(*colorAttView);

	// Handle resources requested by the test instantiation.
	const deUint32							numInResources			= static_cast<deUint32>(instance.resources.inputs.size());
	const deUint32							numOutResources			= static_cast<deUint32>(instance.resources.outputs.size());
	// These variables should be placed out of the following if block to avoid deallocation after out of scope.
	vector<AllocationSp>					inResourceMemories;
	vector<AllocationSp>					outResourceMemories;
	vector<BufferHandleSp>					inResourceBuffers;
	vector<BufferHandleSp>					outResourceBuffers;
	vector<ImageHandleSp>					inResourceImages;
	vector<ImageViewHandleSp>				inResourceImageViews;
	vector<SamplerHandleSp>					inResourceSamplers;
	Move<VkDescriptorPool>					descriptorPool;
	Move<VkDescriptorSetLayout>				setLayout;
	VkDescriptorSetLayout					rawSetLayout			= DE_NULL;
	VkDescriptorSet							rawSet					= DE_NULL;

	const Unique<VkCommandPool>				cmdPool					(createCommandPool(vk, *vkDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));

	// Command buffer
	const Unique<VkCommandBuffer>			cmdBuf					(allocateCommandBuffer(vk, *vkDevice, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	if (numResources != 0)
	{
		vector<VkDescriptorSetLayoutBinding>	setLayoutBindings;
		vector<VkDescriptorPoolSize>			poolSizes;

		setLayoutBindings.reserve(numResources);
		poolSizes.reserve(numResources);

		// Process all input resources.
		for (deUint32 inputNdx = 0; inputNdx < numInResources; ++inputNdx)
		{
			const Resource&	resource	= instance.resources.inputs[inputNdx];

			const bool		hasImage	= (resource.first == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)	||
										  (resource.first == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)	||
										  (resource.first == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			const bool		hasSampler	= (resource.first == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)	||
										  (resource.first == VK_DESCRIPTOR_TYPE_SAMPLER)		||
										  (resource.first == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			// Resource is a buffer
			if (!hasImage && !hasSampler)
			{
				Move<VkBuffer>					resourceBuffer			= createBufferForResource(vk, *vkDevice, resource, queueFamilyIndex);
				de::MovePtr<Allocation>			resourceMemory			= allocator.allocate(getBufferMemoryRequirements(vk, *vkDevice, *resourceBuffer), MemoryRequirement::HostVisible);

				VK_CHECK(vk.bindBufferMemory(*vkDevice, *resourceBuffer, resourceMemory->getMemory(), resourceMemory->getOffset()));

				// Copy data to memory.
				{
					const VkMappedMemoryRange		range					=
					{
						VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,				//	VkStructureType	sType;
						DE_NULL,											//	const void*		pNext;
						resourceMemory->getMemory(),						//	VkDeviceMemory	mem;
						0,													//	VkDeviceSize	offset;
						VK_WHOLE_SIZE,										//	VkDeviceSize	size;
					};

					vector<deUint8>					resourceBytes;
					resource.second->getBytes(resourceBytes);

					deMemcpy(resourceMemory->getHostPtr(), &resourceBytes.front(), resourceBytes.size());
					VK_CHECK(vk.flushMappedMemoryRanges(*vkDevice, 1u, &range));
				}

				inResourceMemories.push_back(AllocationSp(resourceMemory.release()));
				inResourceBuffers.push_back(BufferHandleSp(new BufferHandleUp(resourceBuffer)));
			}
			// Resource is an image
			else if (hasImage)
			{
				Move<VkBuffer>					resourceBuffer			= createBufferForResource(vk, *vkDevice, resource, queueFamilyIndex);
				de::MovePtr<Allocation>			resourceMemory			= allocator.allocate(getBufferMemoryRequirements(vk, *vkDevice, *resourceBuffer), MemoryRequirement::HostVisible);

				VK_CHECK(vk.bindBufferMemory(*vkDevice, *resourceBuffer, resourceMemory->getMemory(), resourceMemory->getOffset()));

				// Copy data to memory.
				{
					const VkMappedMemoryRange		range					=
					{
						VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,				//	VkStructureType	sType;
						DE_NULL,											//	const void*		pNext;
						resourceMemory->getMemory(),						//	VkDeviceMemory	mem;
						0,													//	VkDeviceSize	offset;
						VK_WHOLE_SIZE,										//	VkDeviceSize	size;
					};

					vector<deUint8>					resourceBytes;
					resource.second->getBytes(resourceBytes);

					deMemcpy(resourceMemory->getHostPtr(), &resourceBytes.front(), resourceBytes.size());
					VK_CHECK(vk.flushMappedMemoryRanges(*vkDevice, 1u, &range));
				}

				Move<VkImage>					resourceImage			= createImageForResource(vk, *vkDevice, resource, queueFamilyIndex);
				de::MovePtr<Allocation>			resourceImageMemory		= allocator.allocate(getImageMemoryRequirements(vk, *vkDevice, *resourceImage), MemoryRequirement::Any);

				VK_CHECK(vk.bindImageMemory(*vkDevice, *resourceImage, resourceImageMemory->getMemory(), resourceImageMemory->getOffset()));

				copyBufferToImage(vk, *vkDevice, queue, *cmdBuf, resourceBuffer.get(), resourceImage.get());

				inResourceMemories.push_back(AllocationSp(resourceImageMemory.release()));
				inResourceImages.push_back(ImageHandleSp(new ImageHandleUp(resourceImage)));
			}

			// Prepare descriptor bindings and pool sizes for creating descriptor set layout and pool.
			const VkDescriptorSetLayoutBinding	binding				=
			{
				inputNdx,											// binding
				resource.first,										// descriptorType
				1u,													// descriptorCount
				VK_SHADER_STAGE_ALL_GRAPHICS,						// stageFlags
				DE_NULL,											// pImmutableSamplers
			};
			setLayoutBindings.push_back(binding);

			// Note: the following code doesn't check and unify descriptors of the same type.
			const VkDescriptorPoolSize		poolSize				=
			{
				resource.first,										// type
				1u,													// descriptorCount
			};
			poolSizes.push_back(poolSize);
		}

		// Process all output resources.
		for (deUint32 outputNdx = 0; outputNdx < numOutResources; ++outputNdx)
		{
			const Resource&					resource				= instance.resources.outputs[outputNdx];
			// Create buffer and allocate memory.
			Move<VkBuffer>					resourceBuffer			= createBufferForResource(vk, *vkDevice, resource, queueFamilyIndex);
			de::MovePtr<Allocation>			resourceMemory			= allocator.allocate(getBufferMemoryRequirements(vk, *vkDevice, *resourceBuffer), MemoryRequirement::HostVisible);
			vector<deUint8>					resourceBytes;

			VK_CHECK(vk.bindBufferMemory(*vkDevice, *resourceBuffer, resourceMemory->getMemory(), resourceMemory->getOffset()));

			// Fill memory with all ones.
			const VkMappedMemoryRange		range					=
			{
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,				//	VkStructureType	sType;
				DE_NULL,											//	const void*		pNext;
				resourceMemory->getMemory(),						//	VkDeviceMemory	mem;
				0,													//	VkDeviceSize	offset;
				VK_WHOLE_SIZE,										//	VkDeviceSize	size;
			};

			resource.second->getBytes(resourceBytes);
			deMemset((deUint8*)resourceMemory->getHostPtr(), 0xff, resourceBytes.size());
			VK_CHECK(vk.flushMappedMemoryRanges(*vkDevice, 1u, &range));

			outResourceMemories.push_back(AllocationSp(resourceMemory.release()));
			outResourceBuffers.push_back(BufferHandleSp(new BufferHandleUp(resourceBuffer)));

			// Prepare descriptor bindings and pool sizes for creating descriptor set layout and pool.
			const VkDescriptorSetLayoutBinding	binding				=
			{
				numInResources  + outputNdx,						// binding
				resource.first,										// descriptorType
				1u,													// descriptorCount
				VK_SHADER_STAGE_ALL_GRAPHICS,						// stageFlags
				DE_NULL,											// pImmutableSamplers
			};
			setLayoutBindings.push_back(binding);

			// Note: the following code doesn't check and unify descriptors of the same type.
			const VkDescriptorPoolSize		poolSize				=
			{
				resource.first,										// type
				1u,													// descriptorCount
			};
			poolSizes.push_back(poolSize);
		}

		// Create descriptor set layout, descriptor pool, and allocate descriptor set.
		const VkDescriptorSetLayoutCreateInfo	setLayoutParams		=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,	// sType
			DE_NULL,												// pNext
			(VkDescriptorSetLayoutCreateFlags)0,					// flags
			numResources,											// bindingCount
			setLayoutBindings.data(),								// pBindings
		};
		setLayout													= createDescriptorSetLayout(vk, *vkDevice, &setLayoutParams);
		rawSetLayout												= *setLayout;

		const VkDescriptorPoolCreateInfo		poolParams			=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,			// sType
			DE_NULL,												// pNext
			(VkDescriptorPoolCreateFlags)0,							// flags
			1u,														// maxSets
			numResources,											// poolSizeCount
			poolSizes.data(),										// pPoolSizes
		};
		descriptorPool												= createDescriptorPool(vk, *vkDevice, &poolParams);

		const VkDescriptorSetAllocateInfo		setAllocParams		=
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,			// sType
			DE_NULL,												// pNext
			*descriptorPool,										// descriptorPool
			1u,														// descriptorSetCount
			&rawSetLayout,											// pSetLayouts
		};
		VK_CHECK(vk.allocateDescriptorSets(*vkDevice, &setAllocParams, &rawSet));

		// Update descriptor set.
		vector<VkWriteDescriptorSet>			writeSpecs;
		vector<VkDescriptorBufferInfo>			dBufferInfos;
		vector<VkDescriptorImageInfo>			dImageInfos;

		writeSpecs.reserve(numResources);
		dBufferInfos.reserve(numResources);
		dImageInfos.reserve(numResources);

		deUint32								imgResourceNdx		= 0u;
		deUint32								bufResourceNdx		= 0u;

		for (deUint32 inputNdx = 0; inputNdx < numInResources; ++inputNdx)
		{
			const Resource&	resource	= instance.resources.inputs[inputNdx];

			const bool		hasImage	= (resource.first == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)	||
										  (resource.first == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)	||
										  (resource.first == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			const bool		hasSampler	= (resource.first == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)	||
										  (resource.first == VK_DESCRIPTOR_TYPE_SAMPLER)		||
										  (resource.first == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			// Create image view and sampler
			if (hasImage || hasSampler)
			{
				if (resource.first != VK_DESCRIPTOR_TYPE_SAMPLER)
				{
					const VkImageViewCreateInfo	imgViewParams	=
					{
						VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,			//	VkStructureType				sType;
						DE_NULL,											//	const void*					pNext;
						0u,													//	VkImageViewCreateFlags		flags;
						**inResourceImages[imgResourceNdx++],				//	VkImage						image;
						VK_IMAGE_VIEW_TYPE_2D,								//	VkImageViewType				viewType;
						VK_FORMAT_R32G32B32A32_SFLOAT,						//	VkFormat					format;
						{
							VK_COMPONENT_SWIZZLE_R,
							VK_COMPONENT_SWIZZLE_G,
							VK_COMPONENT_SWIZZLE_B,
							VK_COMPONENT_SWIZZLE_A
						},													//	VkChannelMapping			channels;
						{
							VK_IMAGE_ASPECT_COLOR_BIT,						//	VkImageAspectFlags	aspectMask;
							0u,												//	deUint32			baseMipLevel;
							1u,												//	deUint32			mipLevels;
							0u,												//	deUint32			baseArrayLayer;
							1u,												//	deUint32			arraySize;
						},													//	VkImageSubresourceRange		subresourceRange;
					};

					Move<VkImageView>			imgView			(createImageView(vk, *vkDevice, &imgViewParams));
					inResourceImageViews.push_back(ImageViewHandleSp(new ImageViewHandleUp(imgView)));
				}

				if (hasSampler)
				{
					const VkSamplerCreateInfo	samplerParams	=
					{
						VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,		// VkStructureType			sType;
						DE_NULL,									// const void*				pNext;
						0,											// VkSamplerCreateFlags		flags;
						VK_FILTER_NEAREST,							// VkFilter					magFilter:
						VK_FILTER_NEAREST,							// VkFilter					minFilter;
						VK_SAMPLER_MIPMAP_MODE_NEAREST,				// VkSamplerMipmapMode		mipmapMode;
						VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeU;
						VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeV;
						VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,		// VkSamplerAddressMode		addressModeW;
						0.0f,										// float					mipLodBias;
						VK_FALSE,									// VkBool32					anistoropyÉnable;
						1.0f,										// float					maxAnisotropy;
						VK_FALSE,									// VkBool32					compareEnable;
						VK_COMPARE_OP_ALWAYS,						// VkCompareOp				compareOp;
						0.0f,										// float					minLod;
						0.0f,										// float					maxLod;
						VK_BORDER_COLOR_INT_OPAQUE_BLACK,			// VkBorderColor			borderColor;
						VK_FALSE									// VkBool32					unnormalizedCoordinates;
					};

					Move<VkSampler>				sampler			(createSampler(vk, *vkDevice, &samplerParams));
					inResourceSamplers.push_back(SamplerHandleSp(new SamplerHandleUp(sampler)));
				}
			}

			// Create descriptor buffer and image infos
			switch (resource.first)
			{
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				{
					const VkDescriptorBufferInfo	bufInfo	=
					{
						**inResourceBuffers[bufResourceNdx++],				// buffer
						0,													// offset
						VK_WHOLE_SIZE,										// size
					};
					dBufferInfos.push_back(bufInfo);
					break;
				}
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				{
					const VkDescriptorImageInfo		imgInfo	=
					{
						DE_NULL,												// sampler
						**inResourceImageViews.back(),							// imageView
						VK_IMAGE_LAYOUT_GENERAL									// imageLayout
					};
					dImageInfos.push_back(imgInfo);
					break;
				}
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				{
					const VkDescriptorImageInfo		imgInfo	=
					{
						**inResourceSamplers.back(),							// sampler
						DE_NULL,												// imageView
						VK_IMAGE_LAYOUT_GENERAL									// imageLayout
					};
					dImageInfos.push_back(imgInfo);
					break;
				}
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				{

					const VkDescriptorImageInfo		imgInfo	=
					{
						**inResourceSamplers.back(),							// sampler
						**inResourceImageViews.back(),							// imageView
						VK_IMAGE_LAYOUT_GENERAL									// imageLayout
					};
					dImageInfos.push_back(imgInfo);
					break;
				}
				default:
					DE_FATAL("Not implemented");
			}

			const VkWriteDescriptorSet			writeSpec			= {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,							// sType
				DE_NULL,														// pNext
				rawSet,															// dstSet
				inputNdx,														// binding
				0,																// dstArrayElement
				1u,																// descriptorCount
				instance.resources.inputs[inputNdx].first,						// descriptorType
				( (hasImage | hasSampler)	? &dImageInfos.back()	: DE_NULL),	// pImageInfo
				(!(hasImage | hasSampler)	? &dBufferInfos.back()	: DE_NULL),	// pBufferInfo
				DE_NULL,														// pTexelBufferView
			};
			writeSpecs.push_back(writeSpec);
		}

		for (deUint32 outputNdx = 0; outputNdx < numOutResources; ++outputNdx)
		{
			const VkDescriptorBufferInfo		bufInfo				=
			{
				**outResourceBuffers[outputNdx],					// buffer
				0,													// offset
				VK_WHOLE_SIZE,										// size
			};
			dBufferInfos.push_back(bufInfo);

			const VkWriteDescriptorSet			writeSpec			= {
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,				// sType
				DE_NULL,											// pNext
				rawSet,												// dstSet
				numInResources + outputNdx,							// binding
				0,													// dstArrayElement
				1u,													// descriptorCount
				instance.resources.outputs[outputNdx].first,		// descriptorType
				DE_NULL,											// pImageInfo
				&dBufferInfos.back(),								// pBufferInfo
				DE_NULL,											// pTexelBufferView
			};
			writeSpecs.push_back(writeSpec);
		}
		vk.updateDescriptorSets(*vkDevice, numResources, writeSpecs.data(), 0, DE_NULL);
	}

	// Pipeline layout
	VkPipelineLayoutCreateInfo				pipelineLayoutParams	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,			//	VkStructureType					sType;
		DE_NULL,												//	const void*						pNext;
		(VkPipelineLayoutCreateFlags)0,
		0u,														//	deUint32						descriptorSetCount;
		DE_NULL,												//	const VkDescriptorSetLayout*	pSetLayouts;
		0u,														//	deUint32						pushConstantRangeCount;
		DE_NULL,												//	const VkPushConstantRange*		pPushConstantRanges;
	};

	VkPushConstantRange						pushConstantRange		=
	{
		VK_SHADER_STAGE_ALL_GRAPHICS,							// VkShaderStageFlags    stageFlags;
		0,														// uint32_t              offset;
		0,														// uint32_t              size;
	};
	if (hasPushConstants)
	{
		vector<deUint8> pushConstantsBytes;
		instance.pushConstants.getBuffer()->getBytes(pushConstantsBytes);

		pushConstantRange.size						= static_cast<deUint32>(pushConstantsBytes.size());
		pipelineLayoutParams.pushConstantRangeCount	= 1;
		pipelineLayoutParams.pPushConstantRanges	= &pushConstantRange;
	}
	if (numResources != 0)
	{
		// Update pipeline layout with the descriptor set layout.
		pipelineLayoutParams.setLayoutCount								= 1;
		pipelineLayoutParams.pSetLayouts								= &rawSetLayout;
	}
	const Unique<VkPipelineLayout>			pipelineLayout			(createPipelineLayout(vk, *vkDevice, &pipelineLayoutParams));

	// Pipeline
	vector<VkPipelineShaderStageCreateInfo>		shaderStageParams;
	// We need these vectors to make sure that information about specialization constants for each stage can outlive createGraphicsPipeline().
	vector<vector<VkSpecializationMapEntry> >	specConstantEntries;
	vector<VkSpecializationInfo>				specializationInfos;
	createPipelineShaderStages(vk, *vkDevice, instance, context, modules, shaderStageParams);

	// And we don't want the reallocation of these vectors to invalidate pointers pointing to their contents.
	specConstantEntries.reserve(shaderStageParams.size());
	specializationInfos.reserve(shaderStageParams.size());

	// Patch the specialization info field in PipelineShaderStageCreateInfos.
	for (vector<VkPipelineShaderStageCreateInfo>::iterator stageInfo = shaderStageParams.begin(); stageInfo != shaderStageParams.end(); ++stageInfo)
	{
		const StageToSpecConstantMap::const_iterator stageIt = instance.specConstants.find(stageInfo->stage);

		if (stageIt != instance.specConstants.end())
		{
			const size_t						numSpecConstants	= stageIt->second.size();
			vector<VkSpecializationMapEntry>	entries;
			VkSpecializationInfo				specInfo;

			entries.resize(numSpecConstants);

			// Only support 32-bit integers as spec constants now. And their constant IDs are numbered sequentially starting from 0.
			for (size_t ndx = 0; ndx < numSpecConstants; ++ndx)
			{
				entries[ndx].constantID	= (deUint32)ndx;
				entries[ndx].offset		= deUint32(ndx * sizeof(deInt32));
				entries[ndx].size		= sizeof(deInt32);
			}

			specConstantEntries.push_back(entries);

			specInfo.mapEntryCount	= (deUint32)numSpecConstants;
			specInfo.pMapEntries	= specConstantEntries.back().data();
			specInfo.dataSize		= numSpecConstants * sizeof(deInt32);
			specInfo.pData			= stageIt->second.data();
			specializationInfos.push_back(specInfo);

			stageInfo->pSpecializationInfo = &specializationInfos.back();
		}
	}
	const VkPipelineDepthStencilStateCreateInfo	depthStencilParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	//	VkStructureType		sType;
		DE_NULL,													//	const void*			pNext;
		(VkPipelineDepthStencilStateCreateFlags)0,
		DE_FALSE,													//	deUint32			depthTestEnable;
		DE_FALSE,													//	deUint32			depthWriteEnable;
		VK_COMPARE_OP_ALWAYS,										//	VkCompareOp			depthCompareOp;
		DE_FALSE,													//	deUint32			depthBoundsTestEnable;
		DE_FALSE,													//	deUint32			stencilTestEnable;
		{
			VK_STENCIL_OP_KEEP,											//	VkStencilOp	stencilFailOp;
			VK_STENCIL_OP_KEEP,											//	VkStencilOp	stencilPassOp;
			VK_STENCIL_OP_KEEP,											//	VkStencilOp	stencilDepthFailOp;
			VK_COMPARE_OP_ALWAYS,										//	VkCompareOp	stencilCompareOp;
			0u,															//	deUint32	stencilCompareMask;
			0u,															//	deUint32	stencilWriteMask;
			0u,															//	deUint32	stencilReference;
		},															//	VkStencilOpState	front;
		{
			VK_STENCIL_OP_KEEP,											//	VkStencilOp	stencilFailOp;
			VK_STENCIL_OP_KEEP,											//	VkStencilOp	stencilPassOp;
			VK_STENCIL_OP_KEEP,											//	VkStencilOp	stencilDepthFailOp;
			VK_COMPARE_OP_ALWAYS,										//	VkCompareOp	stencilCompareOp;
			0u,															//	deUint32	stencilCompareMask;
			0u,															//	deUint32	stencilWriteMask;
			0u,															//	deUint32	stencilReference;
		},															//	VkStencilOpState	back;
		-1.0f,														//	float				minDepthBounds;
		+1.0f,														//	float				maxDepthBounds;
	};
	const VkViewport						viewport0				=
	{
		0.0f,														//	float	originX;
		0.0f,														//	float	originY;
		(float)renderSize.x(),										//	float	width;
		(float)renderSize.y(),										//	float	height;
		0.0f,														//	float	minDepth;
		1.0f,														//	float	maxDepth;
	};
	const VkRect2D							scissor0				=
	{
		{
			0u,															//	deInt32	x;
			0u,															//	deInt32	y;
		},															//	VkOffset2D	offset;
		{
			renderSize.x(),												//	deInt32	width;
			renderSize.y(),												//	deInt32	height;
		},															//	VkExtent2D	extent;
	};
	const VkPipelineViewportStateCreateInfo		viewportParams			=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,		//	VkStructureType		sType;
		DE_NULL,													//	const void*			pNext;
		(VkPipelineViewportStateCreateFlags)0,
		1u,															//	deUint32			viewportCount;
		&viewport0,
		1u,
		&scissor0
	};
	const VkSampleMask							sampleMask				= ~0u;
	const VkPipelineMultisampleStateCreateInfo	multisampleParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	//	VkStructureType			sType;
		DE_NULL,													//	const void*				pNext;
		(VkPipelineMultisampleStateCreateFlags)0,
		VK_SAMPLE_COUNT_1_BIT,										//	VkSampleCountFlagBits	rasterSamples;
		DE_FALSE,													//	deUint32				sampleShadingEnable;
		0.0f,														//	float					minSampleShading;
		&sampleMask,												//	const VkSampleMask*		pSampleMask;
		DE_FALSE,													//	VkBool32				alphaToCoverageEnable;
		DE_FALSE,													//	VkBool32				alphaToOneEnable;
	};
	const VkPipelineRasterizationStateCreateInfo	rasterParams		=
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,	//	VkStructureType	sType;
		DE_NULL,													//	const void*		pNext;
		(VkPipelineRasterizationStateCreateFlags)0,
		DE_FALSE,													//	deUint32		depthClampEnable;
		DE_FALSE,													//	deUint32		rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,										//	VkFillMode		fillMode;
		VK_CULL_MODE_NONE,											//	VkCullMode		cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,							//	VkFrontFace		frontFace;
		VK_FALSE,													//	VkBool32		depthBiasEnable;
		0.0f,														//	float			depthBias;
		0.0f,														//	float			depthBiasClamp;
		0.0f,														//	float			slopeScaledDepthBias;
		1.0f,														//	float			lineWidth;
	};
	const VkPrimitiveTopology topology = hasTessellation? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST: VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	const VkPipelineInputAssemblyStateCreateInfo	inputAssemblyParams	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	//	VkStructureType		sType;
		DE_NULL,														//	const void*			pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,
		topology,														//	VkPrimitiveTopology	topology;
		DE_FALSE,														//	deUint32			primitiveRestartEnable;
	};

	vector<VkVertexInputBindingDescription>		vertexBindings;
	vector<VkVertexInputAttributeDescription>	vertexAttribs;

	const VkVertexInputBindingDescription		vertexBinding0			=
	{
		0u,									// deUint32					binding;
		deUint32(singleVertexDataSize),		// deUint32					strideInBytes;
		VK_VERTEX_INPUT_RATE_VERTEX			// VkVertexInputStepRate	stepRate;
	};
	vertexBindings.push_back(vertexBinding0);

	{
		VkVertexInputAttributeDescription		attr0					=
		{
			0u,									// deUint32	location;
			0u,									// deUint32	binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat	format;
			0u									// deUint32	offsetInBytes;
		};
		vertexAttribs.push_back(attr0);

		VkVertexInputAttributeDescription		attr1					=
		{
			1u,									// deUint32	location;
			0u,									// deUint32	binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat	format;
			sizeof(Vec4),						// deUint32	offsetInBytes;
		};
		vertexAttribs.push_back(attr1);
	};

	// If the test instantiation has additional input/output interface variables, we need to create additional bindings.
	// Right now we only support one additional input varible for the vertex stage, and that will be bound to binding #1
	// with location #2.
	if (needInterface)
	{
		const VkVertexInputBindingDescription	vertexBinding1			=
		{
			1u,													// deUint32					binding;
			instance.interfaces.getInputType().getNumBytes(),	// deUint32					strideInBytes;
			VK_VERTEX_INPUT_RATE_VERTEX							// VkVertexInputStepRate	stepRate;
		};
		vertexBindings.push_back(vertexBinding1);

		VkVertexInputAttributeDescription		attr					=
		{
			2u,													// deUint32	location;
			1u,													// deUint32	binding;
			instance.interfaces.getInputType().getVkFormat(),	// VkFormat	format;
			0,													// deUint32	offsetInBytes;
		};
		vertexAttribs.push_back(attr);
	}

	VkPipelineVertexInputStateCreateInfo		vertexInputStateParams	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,	//	VkStructureType								sType;
		DE_NULL,													//	const void*									pNext;
		(VkPipelineVertexInputStateCreateFlags)0,
		1u,															//	deUint32									bindingCount;
		vertexBindings.data(),										//	const VkVertexInputBindingDescription*		pVertexBindingDescriptions;
		2u,															//	deUint32									attributeCount;
		vertexAttribs.data(),										//	const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
	};

	if (needInterface)
	{
		vertexInputStateParams.vertexBindingDescriptionCount += 1;
		vertexInputStateParams.vertexAttributeDescriptionCount += 1;
	}

	vector<VkPipelineColorBlendAttachmentState>	attBlendStates;
	const VkPipelineColorBlendAttachmentState	attBlendState			=
	{
		DE_FALSE,													//	deUint32		blendEnable;
		VK_BLEND_FACTOR_ONE,										//	VkBlend			srcBlendColor;
		VK_BLEND_FACTOR_ZERO,										//	VkBlend			destBlendColor;
		VK_BLEND_OP_ADD,											//	VkBlendOp		blendOpColor;
		VK_BLEND_FACTOR_ONE,										//	VkBlend			srcBlendAlpha;
		VK_BLEND_FACTOR_ZERO,										//	VkBlend			destBlendAlpha;
		VK_BLEND_OP_ADD,											//	VkBlendOp		blendOpAlpha;
		(VK_COLOR_COMPONENT_R_BIT|
		 VK_COLOR_COMPONENT_G_BIT|
		 VK_COLOR_COMPONENT_B_BIT|
		 VK_COLOR_COMPONENT_A_BIT),									//	VkChannelFlags	channelWriteMask;
	};
	attBlendStates.push_back(attBlendState);

	if (needInterface)
		attBlendStates.push_back(attBlendState);

	VkPipelineColorBlendStateCreateInfo		blendParams				=
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	//	VkStructureType								sType;
		DE_NULL,													//	const void*									pNext;
		(VkPipelineColorBlendStateCreateFlags)0,
		DE_FALSE,													//	VkBool32									logicOpEnable;
		VK_LOGIC_OP_COPY,											//	VkLogicOp									logicOp;
		1u,															//	deUint32									attachmentCount;
		attBlendStates.data(),										//	const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },									//	float										blendConst[4];
	};
	if (needInterface)
	{
		blendParams.attachmentCount += 1;
	}
	const VkPipelineTessellationStateCreateInfo	tessellationState	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineTessellationStateCreateFlags)0,
		3u
	};

	const VkPipelineTessellationStateCreateInfo* tessellationInfo	=	hasTessellation ? &tessellationState: DE_NULL;
	const VkGraphicsPipelineCreateInfo		pipelineParams			=
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,		//	VkStructureType									sType;
		DE_NULL,												//	const void*										pNext;
		0u,														//	VkPipelineCreateFlags							flags;
		(deUint32)shaderStageParams.size(),						//	deUint32										stageCount;
		&shaderStageParams[0],									//	const VkPipelineShaderStageCreateInfo*			pStages;
		&vertexInputStateParams,								//	const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
		&inputAssemblyParams,									//	const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
		tessellationInfo,										//	const VkPipelineTessellationStateCreateInfo*	pTessellationState;
		&viewportParams,										//	const VkPipelineViewportStateCreateInfo*		pViewportState;
		&rasterParams,											//	const VkPipelineRasterStateCreateInfo*			pRasterState;
		&multisampleParams,										//	const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
		&depthStencilParams,									//	const VkPipelineDepthStencilStateCreateInfo*	pDepthStencilState;
		&blendParams,											//	const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
		(const VkPipelineDynamicStateCreateInfo*)DE_NULL,		//	const VkPipelineDynamicStateCreateInfo*			pDynamicState;
		*pipelineLayout,										//	VkPipelineLayout								layout;
		*renderPass,											//	VkRenderPass									renderPass;
		0u,														//	deUint32										subpass;
		DE_NULL,												//	VkPipeline										basePipelineHandle;
		0u,														//	deInt32											basePipelineIndex;
	};

	const Unique<VkPipeline>				pipeline				(createGraphicsPipeline(vk, *vkDevice, DE_NULL, &pipelineParams));

	if (needInterface)
	{
		const VkImageViewCreateInfo			fragOutputViewParams	=
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,			//	VkStructureType				sType;
			DE_NULL,											//	const void*					pNext;
			0u,													//	VkImageViewCreateFlags		flags;
			*fragOutputImage,									//	VkImage						image;
			VK_IMAGE_VIEW_TYPE_2D,								//	VkImageViewType				viewType;
			instance.interfaces.getOutputType().getVkFormat(),	//	VkFormat					format;
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			},													//	VkChannelMapping			channels;
			{
				VK_IMAGE_ASPECT_COLOR_BIT,						//	VkImageAspectFlags	aspectMask;
				0u,												//	deUint32			baseMipLevel;
				1u,												//	deUint32			mipLevels;
				0u,												//	deUint32			baseArrayLayer;
				1u,												//	deUint32			arraySize;
			},													//	VkImageSubresourceRange		subresourceRange;
		};
		fragOutputImageView = createImageView(vk, *vkDevice, &fragOutputViewParams);
		attViews.push_back(*fragOutputImageView);
	}

	// Framebuffer
	VkFramebufferCreateInfo					framebufferParams		=
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,				//	VkStructureType		sType;
		DE_NULL,												//	const void*			pNext;
		(VkFramebufferCreateFlags)0,
		*renderPass,											//	VkRenderPass		renderPass;
		1u,														//	deUint32			attachmentCount;
		attViews.data(),										//	const VkImageView*	pAttachments;
		(deUint32)renderSize.x(),								//	deUint32			width;
		(deUint32)renderSize.y(),								//	deUint32			height;
		1u,														//	deUint32			layers;
	};

	if (needInterface)
		framebufferParams.attachmentCount += 1;

	const Unique<VkFramebuffer>				framebuffer				(createFramebuffer(vk, *vkDevice, &framebufferParams));

	const VkCommandBufferBeginInfo			cmdBufBeginParams		=
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,			//	VkStructureType				sType;
		DE_NULL,												//	const void*					pNext;
		(VkCommandBufferUsageFlags)0,
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	// Record commands
	VK_CHECK(vk.beginCommandBuffer(*cmdBuf, &cmdBufBeginParams));

	{
		const VkMemoryBarrier			vertFlushBarrier	=
		{
			VK_STRUCTURE_TYPE_MEMORY_BARRIER,			//	VkStructureType		sType;
			DE_NULL,									//	const void*			pNext;
			VK_ACCESS_HOST_WRITE_BIT,					//	VkMemoryOutputFlags	outputMask;
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,		//	VkMemoryInputFlags	inputMask;
		};
		vector<VkImageMemoryBarrier>	colorAttBarriers;

		VkImageMemoryBarrier			imgBarrier          =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		//	VkStructureType			sType;
			DE_NULL,									//	const void*				pNext;
			0u,											//	VkMemoryOutputFlags		outputMask;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		//	VkMemoryInputFlags		inputMask;
			VK_IMAGE_LAYOUT_UNDEFINED,					//	VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	//	VkImageLayout			newLayout;
			queueFamilyIndex,							//	deUint32				srcQueueFamilyIndex;
			queueFamilyIndex,							//	deUint32				destQueueFamilyIndex;
			*image,										//	VkImage					image;
			{
				VK_IMAGE_ASPECT_COLOR_BIT,					//	VkImageAspect	aspect;
				0u,											//	deUint32		baseMipLevel;
				1u,											//	deUint32		mipLevels;
				0u,											//	deUint32		baseArraySlice;
				1u,											//	deUint32		arraySize;
			}											//	VkImageSubresourceRange	subresourceRange;
		};
		colorAttBarriers.push_back(imgBarrier);
		if (needInterface)
		{
			imgBarrier.image = *fragOutputImage;
			colorAttBarriers.push_back(imgBarrier);
			vk.cmdPipelineBarrier(*cmdBuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, (VkDependencyFlags)0, 1, &vertFlushBarrier, 0, (const VkBufferMemoryBarrier*)DE_NULL, 2, colorAttBarriers.data());
		}
		else
		{
			vk.cmdPipelineBarrier(*cmdBuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, (VkDependencyFlags)0, 1, &vertFlushBarrier, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, colorAttBarriers.data());
		}
	}

	{
		vector<VkClearValue>			clearValue;
		clearValue.push_back(makeClearValueColorF32(0.125f, 0.25f, 0.75f, 1.0f));
		if (needInterface)
		{
			clearValue.push_back(makeClearValueColorU32(0, 0, 0, 0));
		}
		VkRenderPassBeginInfo			passBeginParams	=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,			//	VkStructureType		sType;
			DE_NULL,											//	const void*			pNext;
			*renderPass,										//	VkRenderPass		renderPass;
			*framebuffer,										//	VkFramebuffer		framebuffer;
			{ { 0, 0 }, { renderSize.x(), renderSize.y() } },	//	VkRect2D			renderArea;
			1u,													//	deUint32			clearValueCount;
			clearValue.data(),									//	const VkClearValue*	pClearValues;
		};
		if (needInterface)
		{
			passBeginParams.clearValueCount += 1;
		}
		vk.cmdBeginRenderPass(*cmdBuf, &passBeginParams, VK_SUBPASS_CONTENTS_INLINE);
	}

	vk.cmdBindPipeline(*cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	{
		const VkDeviceSize bindingOffset = 0;
		vk.cmdBindVertexBuffers(*cmdBuf, 0u, 1u, &vertexBuffer.get(), &bindingOffset);
	}
	if (needInterface)
	{
		const VkDeviceSize bindingOffset = 0;
		vk.cmdBindVertexBuffers(*cmdBuf, 1u, 1u, &vertexInputBuffer.get(), &bindingOffset);
	}
	if (hasPushConstants)
	{
		vector<deUint8> pushConstantsBytes;
		instance.pushConstants.getBuffer()->getBytes(pushConstantsBytes);

		const deUint32	size	= static_cast<deUint32>(pushConstantsBytes.size());
		const void*		data	= &pushConstantsBytes.front();

		vk.cmdPushConstants(*cmdBuf, *pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, size, data);
	}
	if (numResources != 0)
	{
		// Bind to set number 0.
		vk.cmdBindDescriptorSets(*cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0, 1, &rawSet, 0, DE_NULL);
	}
	vk.cmdDraw(*cmdBuf, deUint32(vertexCount), 1u /*run pipeline once*/, 0u /*first vertex*/, 0u /*first instanceIndex*/);
	vk.cmdEndRenderPass(*cmdBuf);

	{
		vector<VkImageMemoryBarrier>	renderFinishBarrier;
		VkImageMemoryBarrier			imgBarrier				=
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		//	VkStructureType			sType;
			DE_NULL,									//	const void*				pNext;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,		//	VkMemoryOutputFlags		outputMask;
			VK_ACCESS_TRANSFER_READ_BIT,				//	VkMemoryInputFlags		inputMask;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	//	VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		//	VkImageLayout			newLayout;
			queueFamilyIndex,							//	deUint32				srcQueueFamilyIndex;
			queueFamilyIndex,							//	deUint32				destQueueFamilyIndex;
			*image,										//	VkImage					image;
			{
				VK_IMAGE_ASPECT_COLOR_BIT,					//	VkImageAspectFlags	aspectMask;
				0u,											//	deUint32			baseMipLevel;
				1u,											//	deUint32			mipLevels;
				0u,											//	deUint32			baseArraySlice;
				1u,											//	deUint32			arraySize;
			}											//	VkImageSubresourceRange	subresourceRange;
		};
		renderFinishBarrier.push_back(imgBarrier);

		if (needInterface)
		{
			imgBarrier.image = *fragOutputImage;
			renderFinishBarrier.push_back(imgBarrier);
			vk.cmdPipelineBarrier(*cmdBuf, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 2, renderFinishBarrier.data());
		}
		else
		{
			vk.cmdPipelineBarrier(*cmdBuf, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 0, (const VkBufferMemoryBarrier*)DE_NULL, 1, renderFinishBarrier.data());
		}
	}

	{
		const VkBufferImageCopy	copyParams	=
		{
			(VkDeviceSize)0u,						//	VkDeviceSize			bufferOffset;
			(deUint32)renderSize.x(),				//	deUint32				bufferRowLength;
			(deUint32)renderSize.y(),				//	deUint32				bufferImageHeight;
			{
				VK_IMAGE_ASPECT_COLOR_BIT,				//	VkImageAspect		aspect;
				0u,										//	deUint32			mipLevel;
				0u,										//	deUint32			arrayLayer;
				1u,										//	deUint32			arraySize;
			},										//	VkImageSubresourceCopy	imageSubresource;
			{ 0u, 0u, 0u },							//	VkOffset3D				imageOffset;
			{ renderSize.x(), renderSize.y(), 1u }	//	VkExtent3D				imageExtent;
		};
		vk.cmdCopyImageToBuffer(*cmdBuf, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *readImageBuffer, 1u, &copyParams);

		if (needInterface)
		{
			vk.cmdCopyImageToBuffer(*cmdBuf, *fragOutputImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *fragOutputBuffer, 1u, &copyParams);
		}
	}

	{
		vector<VkBufferMemoryBarrier> cpFinishBarriers;
		VkBufferMemoryBarrier			copyFinishBarrier	=
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	//	VkStructureType		sType;
			DE_NULL,									//	const void*			pNext;
			VK_ACCESS_TRANSFER_WRITE_BIT,				//	VkMemoryOutputFlags	outputMask;
			VK_ACCESS_HOST_READ_BIT,					//	VkMemoryInputFlags	inputMask;
			queueFamilyIndex,							//	deUint32			srcQueueFamilyIndex;
			queueFamilyIndex,							//	deUint32			destQueueFamilyIndex;
			*readImageBuffer,							//	VkBuffer			buffer;
			0u,											//	VkDeviceSize		offset;
			imageSizeBytes								//	VkDeviceSize		size;
		};
		cpFinishBarriers.push_back(copyFinishBarrier);

		if (needInterface)
		{
			copyFinishBarrier.buffer	= *fragOutputBuffer;
			copyFinishBarrier.size		= VK_WHOLE_SIZE;
			cpFinishBarriers.push_back(copyFinishBarrier);

			vk.cmdPipelineBarrier(*cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 2, cpFinishBarriers.data(), 0, (const VkImageMemoryBarrier*)DE_NULL);
		}
		else
		{
			vk.cmdPipelineBarrier(*cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0, (const VkMemoryBarrier*)DE_NULL, 1, cpFinishBarriers.data(), 0, (const VkImageMemoryBarrier*)DE_NULL);
		}
	}

	VK_CHECK(vk.endCommandBuffer(*cmdBuf));

	// Upload vertex data
	{
		const VkMappedMemoryRange	range			=
		{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,	//	VkStructureType	sType;
			DE_NULL,								//	const void*		pNext;
			vertexBufferMemory->getMemory(),		//	VkDeviceMemory	mem;
			0,										//	VkDeviceSize	offset;
			(VkDeviceSize)sizeof(vertexData),		//	VkDeviceSize	size;
		};
		void*						vertexBufPtr	= vertexBufferMemory->getHostPtr();

		deMemcpy(vertexBufPtr, &vertexData[0], sizeof(vertexData));
		VK_CHECK(vk.flushMappedMemoryRanges(*vkDevice, 1u, &range));
	}

	if (needInterface)
	{
		vector<deUint8> inputBufferBytes;
		instance.interfaces.getInputBuffer()->getBytes(inputBufferBytes);

		const deUint32				typNumBytes		= instance.interfaces.getInputType().getNumBytes();
		const deUint32				bufNumBytes		= static_cast<deUint32>(inputBufferBytes.size());

		// Require that the test instantation provides four output values.
		DE_ASSERT(bufNumBytes == 4 * typNumBytes);

		// We have four triangles. Because interpolation happens before executing the fragment shader,
		// we need to provide the same vertex attribute for the same triangle. That means, duplicate each
		// value three times for all four values.

		const deUint8*				provided		= static_cast<const deUint8*>(&inputBufferBytes.front());
		vector<deUint8>				data;

		data.reserve(3 * bufNumBytes);

		for (deUint32 offset = 0; offset < bufNumBytes; offset += typNumBytes)
			for (deUint32 vertexNdx = 0; vertexNdx < 3; ++vertexNdx)
				for (deUint32 byteNdx = 0; byteNdx < typNumBytes; ++byteNdx)
					data.push_back(provided[offset + byteNdx]);

		deMemcpy(vertexInputMemory->getHostPtr(), data.data(), data.size());

		const VkMappedMemoryRange	range			=
		{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,	//	VkStructureType	sType;
			DE_NULL,								//	const void*		pNext;
			vertexInputMemory->getMemory(),			//	VkDeviceMemory	mem;
			0,										//	VkDeviceSize	offset;
			VK_WHOLE_SIZE,							//	VkDeviceSize	size;
		};

		VK_CHECK(vk.flushMappedMemoryRanges(*vkDevice, 1u, &range));
	}

	// Submit & wait for completion
	{
		const VkFenceCreateInfo	fenceParams	=
		{
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,	//	VkStructureType		sType;
			DE_NULL,								//	const void*			pNext;
			0u,										//	VkFenceCreateFlags	flags;
		};
		const Unique<VkFence>	fence		(createFence(vk, *vkDevice, &fenceParams));
		const VkSubmitInfo		submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			DE_NULL,
			0u,
			(const VkSemaphore*)DE_NULL,
			(const VkPipelineStageFlags*)DE_NULL,
			1u,
			&cmdBuf.get(),
			0u,
			(const VkSemaphore*)DE_NULL,
		};

		VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *fence));
		VK_CHECK(vk.waitForFences(*vkDevice, 1u, &fence.get(), DE_TRUE, ~0ull));
	}

	const void* imagePtr	= readImageBufferMemory->getHostPtr();
	const tcu::ConstPixelBufferAccess pixelBuffer(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8),
												  renderSize.x(), renderSize.y(), 1, imagePtr);
	// Log image
	{
		const VkMappedMemoryRange	range		=
		{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,	//	VkStructureType	sType;
			DE_NULL,								//	const void*		pNext;
			readImageBufferMemory->getMemory(),		//	VkDeviceMemory	mem;
			0,										//	VkDeviceSize	offset;
			imageSizeBytes,							//	VkDeviceSize	size;
		};

		VK_CHECK(vk.invalidateMappedMemoryRanges(*vkDevice, 1u, &range));
		context.getTestContext().getLog() << TestLog::Image("Result", "Result", pixelBuffer);
	}

	if (needInterface)
	{
		const VkDeviceSize			fragOutputImgSize	= (VkDeviceSize)(instance.interfaces.getOutputType().getNumBytes() * renderSize.x() * renderSize.y());
		const VkMappedMemoryRange	range				=
		{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,	//	VkStructureType	sType;
			DE_NULL,								//	const void*		pNext;
			fragOutputMemory->getMemory(),			//	VkDeviceMemory	mem;
			0,										//	VkDeviceSize	offset;
			fragOutputImgSize,						//	VkDeviceSize	size;
		};

		VK_CHECK(vk.invalidateMappedMemoryRanges(*vkDevice, 1u, &range));
	}

	{ // Make sure all output resources are ready.
		for (deUint32 outputNdx = 0; outputNdx < numOutResources; ++outputNdx)
		{
			const VkMappedMemoryRange	range	=
			{
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,			//	VkStructureType	sType;
				DE_NULL,										//	const void*		pNext;
				outResourceMemories[outputNdx]->getMemory(),	//	VkDeviceMemory	mem;
				0,												//	VkDeviceSize	offset;
				VK_WHOLE_SIZE,									//	VkDeviceSize	size;
			};

			VK_CHECK(vk.invalidateMappedMemoryRanges(*vkDevice, 1u, &range));
		}
	}

	const RGBA threshold(1, 1, 1, 1);

	const RGBA upperLeft(pixelBuffer.getPixel(1, 1));
	if (!tcu::compareThreshold(upperLeft, instance.outputColors[0], threshold))
		return TestStatus(instance.failResult, instance.getSpecializedFailMessage("Upper left corner mismatch"));

	const RGBA upperRight(pixelBuffer.getPixel(pixelBuffer.getWidth() - 1, 1));
	if (!tcu::compareThreshold(upperRight, instance.outputColors[1], threshold))
		return TestStatus(instance.failResult, instance.getSpecializedFailMessage("Upper right corner mismatch"));

	const RGBA lowerLeft(pixelBuffer.getPixel(1, pixelBuffer.getHeight() - 1));
	if (!tcu::compareThreshold(lowerLeft, instance.outputColors[2], threshold))
		return TestStatus(instance.failResult, instance.getSpecializedFailMessage("Lower left corner mismatch"));

	const RGBA lowerRight(pixelBuffer.getPixel(pixelBuffer.getWidth() - 1, pixelBuffer.getHeight() - 1));
	if (!tcu::compareThreshold(lowerRight, instance.outputColors[3], threshold))
		return TestStatus(instance.failResult, instance.getSpecializedFailMessage("Lower right corner mismatch"));

	// Check that the contents in the ouput variable matches expected.
	if (needInterface)
	{
		vector<deUint8>						inputBufferBytes;
		vector<deUint8>						outputBufferBytes;

		instance.interfaces.getInputBuffer()->getBytes(inputBufferBytes);
		instance.interfaces.getOutputBuffer()->getBytes(outputBufferBytes);

		const IFDataType&					outputType				= instance.interfaces.getOutputType();
		const void*							inputData				= &inputBufferBytes.front();
		const void*							outputData				= &outputBufferBytes.front();
		vector<std::pair<int, int> >		positions;
		const tcu::ConstPixelBufferAccess	fragOutputBufferAccess	(outputType.getTextureFormat(), renderSize.x(), renderSize.y(), 1, fragOutputMemory->getHostPtr());

		positions.push_back(std::make_pair(1, 1));
		positions.push_back(std::make_pair(fragOutputBufferAccess.getWidth() - 1, 1));
		positions.push_back(std::make_pair(1, fragOutputBufferAccess.getHeight() - 1));
		positions.push_back(std::make_pair(fragOutputBufferAccess.getWidth() - 1, fragOutputBufferAccess.getHeight() - 1));

		for (deUint32 posNdx = 0; posNdx < positions.size(); ++posNdx)
		{
			const int	x		= positions[posNdx].first;
			const int	y		= positions[posNdx].second;
			bool		equal	= true;

			if (outputType.elementType == NUMBERTYPE_FLOAT32)
			{
				const float*		expected	= static_cast<const float*>(outputData) + posNdx * outputType.numElements;
				const float*		actual		= static_cast<const float*>(fragOutputBufferAccess.getPixelPtr(x, y));

				for (deUint32 eleNdx = 0; eleNdx < outputType.numElements; ++eleNdx)
					if (!compare32BitFloat(expected[eleNdx], actual[eleNdx], context.getTestContext().getLog()))
						equal = false;
			}
			else if (outputType.elementType == NUMBERTYPE_INT32)
			{
				const deInt32*		expected	= static_cast<const deInt32*>(outputData) + posNdx * outputType.numElements;
				const deInt32*		actual		= static_cast<const deInt32*>(fragOutputBufferAccess.getPixelPtr(x, y));

				for (deUint32 eleNdx = 0; eleNdx < outputType.numElements; ++eleNdx)
					if (expected[eleNdx] != actual[eleNdx])
						equal = false;
			}
			else if (outputType.elementType == NUMBERTYPE_UINT32)
			{
				const deUint32*		expected	= static_cast<const deUint32*>(outputData) + posNdx * outputType.numElements;
				const deUint32*		actual		= static_cast<const deUint32*>(fragOutputBufferAccess.getPixelPtr(x, y));

				for (deUint32 eleNdx = 0; eleNdx < outputType.numElements; ++eleNdx)
					if (expected[eleNdx] != actual[eleNdx])
						equal = false;
			}
			else if (outputType.elementType == NUMBERTYPE_FLOAT16)
			{
				const float*		original	= static_cast<const float*>(inputData) + posNdx * outputType.numElements;
				const deFloat16*	actual		= static_cast<const deFloat16*>(fragOutputBufferAccess.getPixelPtr(x, y));

				for (deUint32 eleNdx = 0; eleNdx < outputType.numElements; ++eleNdx)
					if (!compare16BitFloat(original[eleNdx], actual[eleNdx], instance.interfaces.getRoundingMode(), context.getTestContext().getLog()))
						equal = false;
			}
			else if (outputType.elementType == NUMBERTYPE_INT16)
			{
				const deInt16*		expected	= static_cast<const deInt16*>(outputData) + posNdx * outputType.numElements;
				const deInt16*		actual		= static_cast<const deInt16*>(fragOutputBufferAccess.getPixelPtr(x, y));

				for (deUint32 eleNdx = 0; eleNdx < outputType.numElements; ++eleNdx)
					if (expected[eleNdx] != actual[eleNdx])
						equal = false;
			}
			else if (outputType.elementType == NUMBERTYPE_UINT16)
			{
				const deUint16*		expected	= static_cast<const deUint16*>(outputData) + posNdx * outputType.numElements;
				const deUint16*		actual		= static_cast<const deUint16*>(fragOutputBufferAccess.getPixelPtr(x, y));

				for (deUint32 eleNdx = 0; eleNdx < outputType.numElements; ++eleNdx)
					if (expected[eleNdx] != actual[eleNdx])
						equal = false;
			}
			else {
				DE_ASSERT(0 && "unhandled type");
			}

			if (!equal)
				return TestStatus(instance.failResult, instance.getSpecializedFailMessage("fragment output dat point #" + numberToString(posNdx) + " mismatch"));
		}
	}

	// Check the contents in output resources match with expected.
	for (deUint32 outputNdx = 0; outputNdx < numOutResources; ++outputNdx)
	{
		const BufferSp& expected = instance.resources.outputs[outputNdx].second;

		if (instance.resources.verifyIO != DE_NULL)
		{
			if (!(*instance.resources.verifyIO)(instance.resources.inputs, outResourceMemories, instance.resources.outputs, context.getTestContext().getLog()))
				return tcu::TestStatus::fail("Resource returned doesn't match with expected");
		}
		else
		{
			vector<deUint8> expectedBytes;
			expected->getBytes(expectedBytes);

			if (deMemCmp(&expectedBytes.front(), outResourceMemories[outputNdx]->getHostPtr(), expectedBytes.size()))
				return tcu::TestStatus::fail("Resource returned doesn't match bitwisely with expected");
		}
	}

	return TestStatus::pass("Rendered output matches input");
}

void createTestsForAllStages (const std::string&			name,
							  const RGBA					(&inputColors)[4],
							  const RGBA					(&outputColors)[4],
							  const map<string, string>&	testCodeFragments,
							  const vector<deInt32>&		specConstants,
							  const PushConstants&			pushConstants,
							  const GraphicsResources&		resources,
							  const GraphicsInterfaces&		interfaces,
							  const vector<string>&			extensions,
							  const vector<string>&			features,
							  VulkanFeatures				vulkanFeatures,
							  tcu::TestCaseGroup*			tests,
							  const qpTestResult			failResult,
							  const string&					failMessageTemplate)
{
	const ShaderElement		vertFragPipelineStages[]		=
	{
		ShaderElement("vert", "main", VK_SHADER_STAGE_VERTEX_BIT),
		ShaderElement("frag", "main", VK_SHADER_STAGE_FRAGMENT_BIT),
	};

	const ShaderElement		tessPipelineStages[]			=
	{
		ShaderElement("vert", "main", VK_SHADER_STAGE_VERTEX_BIT),
		ShaderElement("tessc", "main", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT),
		ShaderElement("tesse", "main", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT),
		ShaderElement("frag", "main", VK_SHADER_STAGE_FRAGMENT_BIT),
	};

	const ShaderElement		geomPipelineStages[]				=
	{
		ShaderElement("vert", "main", VK_SHADER_STAGE_VERTEX_BIT),
		ShaderElement("geom", "main", VK_SHADER_STAGE_GEOMETRY_BIT),
		ShaderElement("frag", "main", VK_SHADER_STAGE_FRAGMENT_BIT),
	};

	StageToSpecConstantMap	specConstantMap;

	specConstantMap[VK_SHADER_STAGE_VERTEX_BIT] = specConstants;
	addFunctionCaseWithPrograms<InstanceContext>(
			tests, name + "_vert", "", addShaderCodeCustomVertex, runAndVerifyDefaultPipeline,
			createInstanceContext(vertFragPipelineStages, inputColors, outputColors, testCodeFragments,
				specConstantMap, pushConstants, resources, interfaces, extensions, features, vulkanFeatures, vk::VK_SHADER_STAGE_VERTEX_BIT, failResult, failMessageTemplate));

	specConstantMap.clear();
	specConstantMap[VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT] = specConstants;
	addFunctionCaseWithPrograms<InstanceContext>(
			tests, name + "_tessc", "", addShaderCodeCustomTessControl, runAndVerifyDefaultPipeline,
			createInstanceContext(tessPipelineStages, inputColors, outputColors, testCodeFragments,
				specConstantMap, pushConstants, resources, interfaces, extensions, features, vulkanFeatures, vk::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, failResult, failMessageTemplate));

	specConstantMap.clear();
	specConstantMap[VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT] = specConstants;
	addFunctionCaseWithPrograms<InstanceContext>(
			tests, name + "_tesse", "", addShaderCodeCustomTessEval, runAndVerifyDefaultPipeline,
			createInstanceContext(tessPipelineStages, inputColors, outputColors, testCodeFragments,
				specConstantMap, pushConstants, resources, interfaces, extensions, features, vulkanFeatures, vk::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, failResult, failMessageTemplate));

	specConstantMap.clear();
	specConstantMap[VK_SHADER_STAGE_GEOMETRY_BIT] = specConstants;
	addFunctionCaseWithPrograms<InstanceContext>(
			tests, name + "_geom", "", addShaderCodeCustomGeometry, runAndVerifyDefaultPipeline,
			createInstanceContext(geomPipelineStages, inputColors, outputColors, testCodeFragments,
				specConstantMap, pushConstants, resources, interfaces, extensions, features, vulkanFeatures, vk::VK_SHADER_STAGE_GEOMETRY_BIT, failResult, failMessageTemplate));

	specConstantMap.clear();
	specConstantMap[VK_SHADER_STAGE_FRAGMENT_BIT] = specConstants;
	addFunctionCaseWithPrograms<InstanceContext>(
			tests, name + "_frag", "", addShaderCodeCustomFragment, runAndVerifyDefaultPipeline,
			createInstanceContext(vertFragPipelineStages, inputColors, outputColors, testCodeFragments,
				specConstantMap, pushConstants, resources, interfaces, extensions, features, vulkanFeatures, vk::VK_SHADER_STAGE_FRAGMENT_BIT, failResult, failMessageTemplate));
}

void addTessCtrlTest(tcu::TestCaseGroup* group, const char* name, const map<string, string>& fragments)
{
	RGBA defaultColors[4];
	getDefaultColors(defaultColors);
	const ShaderElement pipelineStages[] =
	{
		ShaderElement("vert", "main", VK_SHADER_STAGE_VERTEX_BIT),
		ShaderElement("tessc", "main", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT),
		ShaderElement("tesse", "main", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT),
		ShaderElement("frag", "main", VK_SHADER_STAGE_FRAGMENT_BIT),
	};

	addFunctionCaseWithPrograms<InstanceContext>(
			group, name, "", addShaderCodeCustomTessControl,
			runAndVerifyDefaultPipeline, createInstanceContext(
				pipelineStages, defaultColors, defaultColors, fragments,
				StageToSpecConstantMap(), PushConstants(), GraphicsResources(),
				GraphicsInterfaces(), vector<string>(), vector<string>(),
				VulkanFeatures(), vk::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT));
}

} // SpirVAssembly
} // vkt
