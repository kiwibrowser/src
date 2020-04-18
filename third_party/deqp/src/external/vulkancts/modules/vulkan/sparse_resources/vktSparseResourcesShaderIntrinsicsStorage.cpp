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
 * \file  vktSparseResourcesShaderIntrinsicsStorage.cpp
 * \brief Sparse Resources Shader Intrinsics for storage images
 *//*--------------------------------------------------------------------*/

#include "vktSparseResourcesShaderIntrinsicsStorage.hpp"

using namespace vk;

namespace vkt
{
namespace sparse
{

tcu::UVec3 computeWorkGroupSize (const tcu::UVec3& gridSize)
{
	const deUint32		maxComputeWorkGroupInvocations	= 128u;
	const tcu::UVec3	maxComputeWorkGroupSize			= tcu::UVec3(128u, 128u, 64u);

	const deUint32 xWorkGroupSize = std::min(std::min(gridSize.x(), maxComputeWorkGroupSize.x()), maxComputeWorkGroupInvocations);
	const deUint32 yWorkGroupSize = std::min(std::min(gridSize.y(), maxComputeWorkGroupSize.y()), maxComputeWorkGroupInvocations / xWorkGroupSize);
	const deUint32 zWorkGroupSize = std::min(std::min(gridSize.z(), maxComputeWorkGroupSize.z()), maxComputeWorkGroupInvocations / (xWorkGroupSize*yWorkGroupSize));

	return tcu::UVec3(xWorkGroupSize, yWorkGroupSize, zWorkGroupSize);
}

void SparseShaderIntrinsicsCaseStorage::initPrograms (vk::SourceCollections& programCollection) const
{
	const std::string	imageTypeStr	= getShaderImageType(m_format, m_imageType);
	const std::string	formatDataStr	= getShaderImageDataType(m_format);
	const std::string	formatQualStr	= getShaderImageFormatQualifier(m_format);

	const std::string  coordString		= getShaderImageCoordinates(m_imageType,
																	"%local_int_GlobalInvocationID_x",
																	"%local_ivec2_GlobalInvocationID_xy",
																	"%local_ivec3_GlobalInvocationID_xyz");
	// Create compute program
	std::ostringstream	src;

	const std::string	typeImgComp					= getImageComponentTypeName(m_format);
	const std::string	typeImgCompVec4				= getImageComponentVec4TypeName(m_format);
	const std::string	typeImageSparse				= getSparseImageTypeName();
	const std::string	typeUniformConstImageSparse	= getUniformConstSparseImageTypeName();

	src << "OpCapability Shader\n"
		<< "OpCapability ImageCubeArray\n"
		<< "OpCapability SparseResidency\n"
		<< "OpCapability StorageImageExtendedFormats\n"

		<< "%ext_import = OpExtInstImport \"GLSL.std.450\"\n"
		<< "OpMemoryModel Logical GLSL450\n"
		<< "OpEntryPoint GLCompute %func_main \"main\" %input_GlobalInvocationID\n"
		<< "OpExecutionMode %func_main LocalSize 1 1 1\n"
		<< "OpSource GLSL 440\n"

		<< "OpName %func_main \"main\"\n"

		<< "OpName %input_GlobalInvocationID \"gl_GlobalInvocationID\"\n"
		<< "OpName %input_WorkGroupSize \"gl_WorkGroupSize\"\n"

		<< "OpName %uniform_image_sparse \"u_imageSparse\"\n"
		<< "OpName %uniform_image_texels \"u_imageTexels\"\n"
		<< "OpName %uniform_image_residency \"u_imageResidency\"\n"

		<< "OpDecorate %input_GlobalInvocationID BuiltIn GlobalInvocationId\n"

		<< "OpDecorate %input_WorkGroupSize BuiltIn WorkgroupSize\n"

		<< "OpDecorate %constant_uint_grid_x SpecId 1\n"
		<< "OpDecorate %constant_uint_grid_y SpecId 2\n"
		<< "OpDecorate %constant_uint_grid_z SpecId 3\n"

		<< "OpDecorate %constant_uint_work_group_size_x SpecId 4\n"
		<< "OpDecorate %constant_uint_work_group_size_y SpecId 5\n"
		<< "OpDecorate %constant_uint_work_group_size_z SpecId 6\n"

		<< "OpDecorate %uniform_image_sparse DescriptorSet 0\n"
		<< "OpDecorate %uniform_image_sparse Binding " << BINDING_IMAGE_SPARSE << "\n"

		<< "OpDecorate %uniform_image_texels DescriptorSet 0\n"
		<< "OpDecorate %uniform_image_texels Binding " << BINDING_IMAGE_TEXELS << "\n"
		<< "OpDecorate %uniform_image_texels NonReadable\n"

		<< "OpDecorate %uniform_image_residency DescriptorSet 0\n"
		<< "OpDecorate %uniform_image_residency Binding " << BINDING_IMAGE_RESIDENCY << "\n"
		<< "OpDecorate %uniform_image_residency NonReadable\n"

		// Declare data types
		<< "%type_bool						= OpTypeBool\n"
		<< "%type_int						= OpTypeInt 32 1\n"
		<< "%type_uint						= OpTypeInt 32 0\n"
		<< "%type_ivec2						= OpTypeVector %type_int  2\n"
		<< "%type_ivec3						= OpTypeVector %type_int  3\n"
		<< "%type_ivec4						= OpTypeVector %type_int  4\n"
		<< "%type_uvec3						= OpTypeVector %type_uint 3\n"
		<< "%type_uvec4						= OpTypeVector %type_uint 4\n"
		<< "%type_struct_int_img_comp_vec4	= OpTypeStruct %type_int " << typeImgCompVec4 << "\n"

		<< "%type_input_uint		= OpTypePointer Input %type_uint\n"
		<< "%type_input_uvec3		= OpTypePointer Input %type_uvec3\n"

		<< "%type_function_int			 = OpTypePointer Function %type_int\n"
		<< "%type_function_img_comp_vec4 = OpTypePointer Function " << typeImgCompVec4 << "\n"

		<< "%type_void				= OpTypeVoid\n"
		<< "%type_void_func			= OpTypeFunction %type_void\n"

		// Sparse image without sampler type declaration
		<< "%type_image_sparse = " << getOpTypeImageSparse(m_imageType, m_format, typeImgComp, false) << "\n"
		<< "%type_uniformconst_image_sparse = OpTypePointer UniformConstant %type_image_sparse\n"

		// Sparse image with sampler type declaration
		<< "%type_image_sparse_with_sampler = " << getOpTypeImageSparse(m_imageType, m_format, typeImgComp, true) << "\n"
		<< "%type_uniformconst_image_sparse_with_sampler = OpTypePointer UniformConstant %type_image_sparse_with_sampler\n"

		// Residency image type declaration
		<< "%type_image_residency				= " << getOpTypeImageResidency(m_imageType) << "\n"
		<< "%type_uniformconst_image_residency	= OpTypePointer UniformConstant %type_image_residency\n"

		// Declare sparse image variable
		<< "%uniform_image_sparse = OpVariable " << typeUniformConstImageSparse << " UniformConstant\n"

		// Declare output image variable for storing texels
		<< "%uniform_image_texels = OpVariable %type_uniformconst_image_sparse UniformConstant\n"

		// Declare output image variable for storing residency information
		<< "%uniform_image_residency = OpVariable %type_uniformconst_image_residency UniformConstant\n"

		// Declare input variables
		<< "%input_GlobalInvocationID = OpVariable %type_input_uvec3 Input\n"

		<< "%constant_uint_grid_x				= OpSpecConstant %type_uint 1\n"
		<< "%constant_uint_grid_y				= OpSpecConstant %type_uint 1\n"
		<< "%constant_uint_grid_z				= OpSpecConstant %type_uint 1\n"

		<< "%constant_uint_work_group_size_x	= OpSpecConstant %type_uint 1\n"
		<< "%constant_uint_work_group_size_y	= OpSpecConstant %type_uint 1\n"
		<< "%constant_uint_work_group_size_z	= OpSpecConstant %type_uint 1\n"
		<< "%input_WorkGroupSize = OpSpecConstantComposite %type_uvec3 %constant_uint_work_group_size_x %constant_uint_work_group_size_y %constant_uint_work_group_size_z\n"

		// Declare constants
		<< "%constant_uint_0				= OpConstant %type_uint 0\n"
		<< "%constant_uint_1				= OpConstant %type_uint 1\n"
		<< "%constant_uint_2				= OpConstant %type_uint 2\n"
		<< "%constant_int_0					= OpConstant %type_int 0\n"
		<< "%constant_int_1					= OpConstant %type_int 1\n"
		<< "%constant_int_2					= OpConstant %type_int 2\n"
		<< "%constant_bool_true				= OpConstantTrue %type_bool\n"
		<< "%constant_uint_resident			= OpConstant %type_uint " << MEMORY_BLOCK_BOUND_VALUE << "\n"
		<< "%constant_uvec4_resident		= OpConstantComposite %type_uvec4 %constant_uint_resident %constant_uint_resident %constant_uint_resident %constant_uint_resident\n"
		<< "%constant_uint_not_resident		= OpConstant %type_uint " << MEMORY_BLOCK_NOT_BOUND_VALUE << "\n"
		<< "%constant_uvec4_not_resident	= OpConstantComposite %type_uvec4 %constant_uint_not_resident %constant_uint_not_resident %constant_uint_not_resident %constant_uint_not_resident\n"

		// Call main function
		<< "%func_main		 = OpFunction %type_void None %type_void_func\n"
		<< "%label_func_main = OpLabel\n"

		// Load GlobalInvocationID.xyz into local variables
		<< "%access_GlobalInvocationID_x		= OpAccessChain %type_input_uint %input_GlobalInvocationID %constant_uint_0\n"
		<< "%local_uint_GlobalInvocationID_x	= OpLoad %type_uint %access_GlobalInvocationID_x\n"
		<< "%local_int_GlobalInvocationID_x		= OpBitcast %type_int %local_uint_GlobalInvocationID_x\n"

		<< "%access_GlobalInvocationID_y		= OpAccessChain %type_input_uint %input_GlobalInvocationID %constant_uint_1\n"
		<< "%local_uint_GlobalInvocationID_y	= OpLoad %type_uint %access_GlobalInvocationID_y\n"
		<< "%local_int_GlobalInvocationID_y		= OpBitcast %type_int %local_uint_GlobalInvocationID_y\n"

		<< "%access_GlobalInvocationID_z		= OpAccessChain %type_input_uint %input_GlobalInvocationID %constant_uint_2\n"
		<< "%local_uint_GlobalInvocationID_z	= OpLoad %type_uint %access_GlobalInvocationID_z\n"
		<< "%local_int_GlobalInvocationID_z		= OpBitcast %type_int %local_uint_GlobalInvocationID_z\n"

		<< "%local_ivec2_GlobalInvocationID_xy	= OpCompositeConstruct %type_ivec2 %local_int_GlobalInvocationID_x %local_int_GlobalInvocationID_y\n"
		<< "%local_ivec3_GlobalInvocationID_xyz = OpCompositeConstruct %type_ivec3 %local_int_GlobalInvocationID_x %local_int_GlobalInvocationID_y %local_int_GlobalInvocationID_z\n"

		<< "%comparison_range_x = OpULessThan %type_bool %local_uint_GlobalInvocationID_x %constant_uint_grid_x\n"
		<< "OpSelectionMerge %label_out_range_x None\n"
		<< "OpBranchConditional %comparison_range_x %label_in_range_x %label_out_range_x\n"
		<< "%label_in_range_x = OpLabel\n"

		<< "%comparison_range_y = OpULessThan %type_bool %local_uint_GlobalInvocationID_y %constant_uint_grid_y\n"
		<< "OpSelectionMerge %label_out_range_y None\n"
		<< "OpBranchConditional %comparison_range_y %label_in_range_y %label_out_range_y\n"
		<< "%label_in_range_y = OpLabel\n"

		<< "%comparison_range_z = OpULessThan %type_bool %local_uint_GlobalInvocationID_z %constant_uint_grid_z\n"
		<< "OpSelectionMerge %label_out_range_z None\n"
		<< "OpBranchConditional %comparison_range_z %label_in_range_z %label_out_range_z\n"
		<< "%label_in_range_z = OpLabel\n"

		// Load sparse image
		<< "%local_image_sparse = OpLoad " << typeImageSparse << " %uniform_image_sparse\n"

		// Call OpImageSparse*
		<< sparseImageOpString("%local_sparse_op_result", "%type_struct_int_img_comp_vec4", "%local_image_sparse", coordString, "%constant_int_0") << "\n"

		// Load the texel from the sparse image to local variable for OpImageSparse*
		<< "%local_img_comp_vec4 = OpCompositeExtract " << typeImgCompVec4 << " %local_sparse_op_result 1\n"

		// Load residency code for OpImageSparse*
		<< "%local_residency_code = OpCompositeExtract %type_int %local_sparse_op_result 0\n"
		// End Call OpImageSparse*

		// Load texels image
		<< "%local_image_texels = OpLoad %type_image_sparse %uniform_image_texels\n"

		// Write the texel to output image via OpImageWrite
		<< "OpImageWrite %local_image_texels " << coordString << " %local_img_comp_vec4\n"

		// Load residency info image
		<< "%local_image_residency	= OpLoad %type_image_residency %uniform_image_residency\n"

		// Check if loaded texel is placed in resident memory
		<< "%local_texel_resident = OpImageSparseTexelsResident %type_bool %local_residency_code\n"
		<< "OpSelectionMerge %branch_texel_resident None\n"
		<< "OpBranchConditional %local_texel_resident %label_texel_resident %label_texel_not_resident\n"
		<< "%label_texel_resident = OpLabel\n"

		// Loaded texel is in resident memory
		<< "OpImageWrite %local_image_residency " << coordString << " %constant_uvec4_resident\n"

		<< "OpBranch %branch_texel_resident\n"
		<< "%label_texel_not_resident = OpLabel\n"

		// Loaded texel is not in resident memory
		<< "OpImageWrite %local_image_residency " << coordString << " %constant_uvec4_not_resident\n"

		<< "OpBranch %branch_texel_resident\n"
		<< "%branch_texel_resident = OpLabel\n"

		<< "OpBranch %label_out_range_z\n"
		<< "%label_out_range_z = OpLabel\n"

		<< "OpBranch %label_out_range_y\n"
		<< "%label_out_range_y = OpLabel\n"

		<< "OpBranch %label_out_range_x\n"
		<< "%label_out_range_x = OpLabel\n"

		<< "OpReturn\n"
		<< "OpFunctionEnd\n";

	programCollection.spirvAsmSources.add("compute") << src.str();
}

std::string	SparseCaseOpImageSparseFetch::getSparseImageTypeName (void) const
{
	return "%type_image_sparse_with_sampler";
}

std::string	SparseCaseOpImageSparseFetch::getUniformConstSparseImageTypeName (void) const
{
	return "%type_uniformconst_image_sparse_with_sampler";
}

std::string	SparseCaseOpImageSparseFetch::sparseImageOpString  (const std::string& resultVariable,
																const std::string& resultType,
																const std::string& image,
																const std::string& coord,
																const std::string& mipLevel) const
{
	std::ostringstream	src;

	src << resultVariable << " = OpImageSparseFetch " << resultType << " " << image << " " << coord << " Lod " << mipLevel << "\n";

	return src.str();
}

std::string	SparseCaseOpImageSparseRead::getSparseImageTypeName (void) const
{
	return "%type_image_sparse";
}

std::string	SparseCaseOpImageSparseRead::getUniformConstSparseImageTypeName (void) const
{
	return "%type_uniformconst_image_sparse";
}

std::string	SparseCaseOpImageSparseRead::sparseImageOpString (const std::string& resultVariable,
															  const std::string& resultType,
															  const std::string& image,
															  const std::string& coord,
															  const std::string& mipLevel) const
{
	DE_UNREF(mipLevel);

	std::ostringstream	src;

	src << resultVariable << " = OpImageSparseRead " << resultType << " " << image << " " << coord << "\n";

	return src.str();
}

class SparseShaderIntrinsicsInstanceStorage : public SparseShaderIntrinsicsInstanceBase
{
public:
	SparseShaderIntrinsicsInstanceStorage	(Context&					context,
											 const SpirVFunction		function,
											 const ImageType			imageType,
											 const tcu::UVec3&			imageSize,
											 const tcu::TextureFormat&	format)
		: SparseShaderIntrinsicsInstanceBase(context, function, imageType, imageSize, format) {}

	VkImageUsageFlags				imageOutputUsageFlags	(void) const;

	VkQueueFlags					getQueueFlags			(void) const;

	void							recordCommands			(const VkCommandBuffer		commandBuffer,
															 const VkImageCreateInfo&	imageSparseInfo,
															 const VkImage				imageSparse,
															 const VkImage				imageTexels,
															 const VkImage				imageResidency);

	virtual VkDescriptorType		imageSparseDescType		(void) const = 0;
};

VkImageUsageFlags SparseShaderIntrinsicsInstanceStorage::imageOutputUsageFlags (void) const
{
	return VK_IMAGE_USAGE_STORAGE_BIT;
}

VkQueueFlags SparseShaderIntrinsicsInstanceStorage::getQueueFlags (void) const
{
	return VK_QUEUE_COMPUTE_BIT;
}

void SparseShaderIntrinsicsInstanceStorage::recordCommands (const VkCommandBuffer		commandBuffer,
															const VkImageCreateInfo&	imageSparseInfo,
															const VkImage				imageSparse,
															const VkImage				imageTexels,
															const VkImage				imageResidency)
{
	const InstanceInterface&	instance		= m_context.getInstanceInterface();
	const DeviceInterface&		deviceInterface = getDeviceInterface();
	const VkPhysicalDevice		physicalDevice	= m_context.getPhysicalDevice();

	// Check if device supports image format for storage image
	if (!checkImageFormatFeatureSupport(instance, physicalDevice, imageSparseInfo.format, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
		TCU_THROW(NotSupportedError, "Device does not support image format for storage image");

	// Make sure device supports VK_FORMAT_R32_UINT format for storage image
	if (!checkImageFormatFeatureSupport(instance, physicalDevice, mapTextureFormat(m_residencyFormat), VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
		TCU_THROW(TestError, "Device does not support VK_FORMAT_R32_UINT format for storage image");

	pipelines.resize(imageSparseInfo.mipLevels);
	descriptorSets.resize(imageSparseInfo.mipLevels);
	imageSparseViews.resize(imageSparseInfo.mipLevels);
	imageTexelsViews.resize(imageSparseInfo.mipLevels);
	imageResidencyViews.resize(imageSparseInfo.mipLevels);

	// Create descriptor set layout
	DescriptorSetLayoutBuilder descriptorLayerBuilder;

	descriptorLayerBuilder.addSingleBinding(imageSparseDescType(), VK_SHADER_STAGE_COMPUTE_BIT);
	descriptorLayerBuilder.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	descriptorLayerBuilder.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(descriptorLayerBuilder.build(deviceInterface, getDevice()));

	// Create pipeline layout
	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(deviceInterface, getDevice(), *descriptorSetLayout));

	// Create descriptor pool
	DescriptorPoolBuilder descriptorPoolBuilder;

	descriptorPoolBuilder.addType(imageSparseDescType(), imageSparseInfo.mipLevels);
	descriptorPoolBuilder.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imageSparseInfo.mipLevels);
	descriptorPoolBuilder.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imageSparseInfo.mipLevels);

	descriptorPool = descriptorPoolBuilder.build(deviceInterface, getDevice(), VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, imageSparseInfo.mipLevels);

	const VkImageSubresourceRange fullImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, imageSparseInfo.mipLevels, 0u, imageSparseInfo.arrayLayers);

	{
		VkImageMemoryBarrier imageShaderAccessBarriers[3];

		imageShaderAccessBarriers[0] = makeImageMemoryBarrier
		(
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
			imageSparse,
			fullImageSubresourceRange
		);

		imageShaderAccessBarriers[1] = makeImageMemoryBarrier
		(
			0u,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			imageTexels,
			fullImageSubresourceRange
		);

		imageShaderAccessBarriers[2] = makeImageMemoryBarrier
		(
			0u,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			imageResidency,
			fullImageSubresourceRange
		);

		deviceInterface.cmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 3u, imageShaderAccessBarriers);
	}

	const VkSpecializationMapEntry specializationMapEntries[6] =
	{
		{ 1u, 0u * (deUint32)sizeof(deUint32), sizeof(deUint32) }, // GridSize.x
		{ 2u, 1u * (deUint32)sizeof(deUint32), sizeof(deUint32) }, // GridSize.y
		{ 3u, 2u * (deUint32)sizeof(deUint32), sizeof(deUint32) }, // GridSize.z
		{ 4u, 3u * (deUint32)sizeof(deUint32), sizeof(deUint32) }, // WorkGroupSize.x
		{ 5u, 4u * (deUint32)sizeof(deUint32), sizeof(deUint32) }, // WorkGroupSize.y
		{ 6u, 5u * (deUint32)sizeof(deUint32), sizeof(deUint32) }, // WorkGroupSize.z
	};

	Unique<VkShaderModule> shaderModule(createShaderModule(deviceInterface, getDevice(), m_context.getBinaryCollection().get("compute"), 0u));

	for (deUint32 mipLevelNdx = 0u; mipLevelNdx < imageSparseInfo.mipLevels; ++mipLevelNdx)
	{
		const tcu::UVec3  gridSize				= getShaderGridSize(m_imageType, m_imageSize, mipLevelNdx);
		const tcu::UVec3  workGroupSize			= computeWorkGroupSize(gridSize);
		const tcu::UVec3 specializationData[2]	= { gridSize, workGroupSize };

		const VkSpecializationInfo specializationInfo =
		{
			(deUint32)DE_LENGTH_OF_ARRAY(specializationMapEntries),	// mapEntryCount
			specializationMapEntries,								// pMapEntries
			sizeof(specializationData),								// dataSize
			specializationData,										// pData
		};

		// Create and bind compute pipeline
		pipelines[mipLevelNdx] = makeVkSharedPtr(makeComputePipeline(deviceInterface, getDevice(), *pipelineLayout, *shaderModule, &specializationInfo));
		const VkPipeline computePipeline = **pipelines[mipLevelNdx];

		deviceInterface.cmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

		// Create descriptor set
		descriptorSets[mipLevelNdx] = makeVkSharedPtr(makeDescriptorSet(deviceInterface, getDevice(), *descriptorPool, *descriptorSetLayout));
		const VkDescriptorSet descriptorSet = **descriptorSets[mipLevelNdx];

		// Bind resources
		const VkImageSubresourceRange mipLevelRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, mipLevelNdx, 1u, 0u, imageSparseInfo.arrayLayers);

		imageSparseViews[mipLevelNdx] = makeVkSharedPtr(makeImageView(deviceInterface, getDevice(), imageSparse, mapImageViewType(m_imageType), imageSparseInfo.format, mipLevelRange));
		const VkDescriptorImageInfo imageSparseDescInfo = makeDescriptorImageInfo(DE_NULL, **imageSparseViews[mipLevelNdx], VK_IMAGE_LAYOUT_GENERAL);

		imageTexelsViews[mipLevelNdx] = makeVkSharedPtr(makeImageView(deviceInterface, getDevice(), imageTexels, mapImageViewType(m_imageType), imageSparseInfo.format, mipLevelRange));
		const VkDescriptorImageInfo imageTexelsDescInfo = makeDescriptorImageInfo(DE_NULL, **imageTexelsViews[mipLevelNdx], VK_IMAGE_LAYOUT_GENERAL);

		imageResidencyViews[mipLevelNdx] = makeVkSharedPtr(makeImageView(deviceInterface, getDevice(), imageResidency, mapImageViewType(m_imageType), mapTextureFormat(m_residencyFormat), mipLevelRange));
		const VkDescriptorImageInfo imageResidencyDescInfo = makeDescriptorImageInfo(DE_NULL, **imageResidencyViews[mipLevelNdx], VK_IMAGE_LAYOUT_GENERAL);

		DescriptorSetUpdateBuilder descriptorUpdateBuilder;
		descriptorUpdateBuilder.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(BINDING_IMAGE_SPARSE), imageSparseDescType(), &imageSparseDescInfo);
		descriptorUpdateBuilder.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(BINDING_IMAGE_TEXELS), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageTexelsDescInfo);
		descriptorUpdateBuilder.writeSingle(descriptorSet, DescriptorSetUpdateBuilder::Location::binding(BINDING_IMAGE_RESIDENCY), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageResidencyDescInfo);

		descriptorUpdateBuilder.update(deviceInterface, getDevice());

		deviceInterface.cmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet, 0u, DE_NULL);

		const deUint32		xWorkGroupCount = gridSize.x() / workGroupSize.x() + (gridSize.x() % workGroupSize.x() ? 1u : 0u);
		const deUint32		yWorkGroupCount = gridSize.y() / workGroupSize.y() + (gridSize.y() % workGroupSize.y() ? 1u : 0u);
		const deUint32		zWorkGroupCount = gridSize.z() / workGroupSize.z() + (gridSize.z() % workGroupSize.z() ? 1u : 0u);
		const tcu::UVec3	maxWorkGroupCount = tcu::UVec3(65535u, 65535u, 65535u);

		if (maxWorkGroupCount.x() < xWorkGroupCount ||
			maxWorkGroupCount.y() < yWorkGroupCount ||
			maxWorkGroupCount.z() < zWorkGroupCount)
		{
			TCU_THROW(NotSupportedError, "Image size exceeds compute invocations limit");
		}

		deviceInterface.cmdDispatch(commandBuffer, xWorkGroupCount, yWorkGroupCount, zWorkGroupCount);
	}

	{
		VkImageMemoryBarrier imageOutputTransferSrcBarriers[2];

		imageOutputTransferSrcBarriers[0] = makeImageMemoryBarrier
		(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			imageTexels,
			fullImageSubresourceRange
		);

		imageOutputTransferSrcBarriers[1] = makeImageMemoryBarrier
		(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			imageResidency,
			fullImageSubresourceRange
		);

		deviceInterface.cmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 2u, imageOutputTransferSrcBarriers);
	}
}

class SparseShaderIntrinsicsInstanceFetch : public SparseShaderIntrinsicsInstanceStorage
{
public:
	SparseShaderIntrinsicsInstanceFetch			(Context&					context,
												 const SpirVFunction		function,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format)
	: SparseShaderIntrinsicsInstanceStorage (context, function, imageType, imageSize, format) {}

	VkImageUsageFlags	imageSparseUsageFlags	(void) const { return VK_IMAGE_USAGE_SAMPLED_BIT; }
	VkDescriptorType	imageSparseDescType		(void) const { return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; }
};

TestInstance* SparseCaseOpImageSparseFetch::createInstance (Context& context) const
{
	return new SparseShaderIntrinsicsInstanceFetch(context, m_function, m_imageType, m_imageSize, m_format);
}

class SparseShaderIntrinsicsInstanceRead : public SparseShaderIntrinsicsInstanceStorage
{
public:
	SparseShaderIntrinsicsInstanceRead			(Context&					context,
												 const SpirVFunction		function,
												 const ImageType			imageType,
												 const tcu::UVec3&			imageSize,
												 const tcu::TextureFormat&	format)
	: SparseShaderIntrinsicsInstanceStorage (context, function, imageType, imageSize, format) {}

	VkImageUsageFlags	imageSparseUsageFlags	(void) const { return VK_IMAGE_USAGE_STORAGE_BIT; }
	VkDescriptorType	imageSparseDescType		(void) const { return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; }
};

TestInstance* SparseCaseOpImageSparseRead::createInstance (Context& context) const
{
	return new SparseShaderIntrinsicsInstanceRead(context, m_function, m_imageType, m_imageSize, m_format);
}

} // sparse
} // vkt
