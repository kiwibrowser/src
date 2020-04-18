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
 * \brief SPIR-V Assembly Tests for indexing with different bit sizes.
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmIndexingTests.hpp"
#include "vktSpvAsmComputeShaderCase.hpp"
#include "vktSpvAsmComputeShaderTestUtil.hpp"
#include "vktSpvAsmGraphicsShaderTestUtil.hpp"

#include "tcuStringTemplate.hpp"

namespace vkt
{
namespace SpirVAssembly
{

using namespace vk;
using std::map;
using std::string;
using std::vector;
using std::pair;
using tcu::IVec3;
using tcu::RGBA;
using tcu::UVec4;
using tcu::Mat4;
using tcu::StringTemplate;

namespace
{

enum ChainOp
{
	CHAIN_OP_ACCESS_CHAIN = 0,
	CHAIN_OP_IN_BOUNDS_ACCESS_CHAIN,
	CHAIN_OP_PTR_ACCESS_CHAIN,

	CHAIN_OP_LAST
};
static const int					idxSizes[]				= { 16, 32, 64 };
static const ComputeTestFeatures	computeTestFeatures[]	= { COMPUTE_TEST_USES_INT16, COMPUTE_TEST_USES_NONE, COMPUTE_TEST_USES_INT64 };
static const string					chainOpTestNames[]		= { "opaccesschain", "opinboundsaccesschain", "opptraccesschain" };

struct InputData
{
	Mat4	matrix[32][32];
};

void addComputeIndexingTests (tcu::TestCaseGroup* group)
{
	tcu::TestContext&	testCtx			= group->getTestContext();
	de::Random			rnd				(deStringHash(group->getName()));
	const int			numItems		= 128;
	const int			numStructs		= 2;
	const int			numInputFloats	= (int)sizeof(InputData) / 4 * numStructs;
	vector<float>		inputData;
	vector<UVec4>		indexSelectorData;

	inputData.reserve(numInputFloats);
	for (deUint32 numIdx = 0; numIdx < numInputFloats; ++numIdx)
		inputData.push_back(rnd.getFloat());

	indexSelectorData.reserve(numItems);
	for (deUint32 numIdx = 0; numIdx < numItems; ++numIdx)
		indexSelectorData.push_back(UVec4(rnd.getUint32() % 32, rnd.getUint32() % 32, rnd.getUint32() % 4, rnd.getUint32() % 4));

	for (int chainOpIdx = 0; chainOpIdx < CHAIN_OP_LAST; ++chainOpIdx)
	{
		for (int idxSizeIdx = 0; idxSizeIdx < DE_LENGTH_OF_ARRAY(idxSizes); ++idxSizeIdx)
		{
			for (int sign = 0; sign < 2; ++sign)
			{
				const int					idxSize			= idxSizes[idxSizeIdx];
				const string				testName		= chainOpTestNames[chainOpIdx] + string(sign == 0 ? "_u" : "_s") + de::toString(idxSize);
				VulkanFeatures				vulkanFeatures;
				map<string, string>			specs;
				vector<float>				outputData;
				ComputeShaderSpec			spec;
				const ComputeTestFeatures	features		= computeTestFeatures[idxSizeIdx];
				int							element			= 0;

				// Index an input buffer containing 2D array of 4x4 matrices. The indices are read from another
				// input and converted to the desired bit size and sign.
				const StringTemplate		shaderSource(
					"                             OpCapability Shader\n"
					"                             ${intcaps:opt}\n"
					"                             ${variablepointercaps:opt}\n"
					"                             ${extensions:opt}\n"
					"                        %1 = OpExtInstImport \"GLSL.std.450\"\n"
					"                             OpMemoryModel Logical GLSL450\n"
					"                             OpEntryPoint GLCompute %main \"main\" %gl_GlobalInvocationID\n"
					"                             OpExecutionMode %main LocalSize 1 1 1\n"
					"                             OpSource GLSL 430\n"
					"                             OpDecorate %gl_GlobalInvocationID BuiltIn GlobalInvocationId\n"
					"                             OpDecorate %_arr_float_uint_128 ArrayStride 4\n"
					"                             OpMemberDecorate %Output 0 Offset 0\n"
					"                             OpDecorate %Output BufferBlock\n"
					"                             OpDecorate %dataOutput DescriptorSet 0\n"
					"                             OpDecorate %dataOutput Binding 2\n"
					"                             OpDecorate %_arr_mat4v4float_uint_32 ArrayStride 64\n"
					"                             OpDecorate %_arr__arr_mat4v4float_uint_32_uint_32 ArrayStride 2048\n"
					"                             OpMemberDecorate %InputStruct 0 ColMajor\n"
					"                             OpMemberDecorate %InputStruct 0 Offset 0\n"
					"                             OpMemberDecorate %InputStruct 0 MatrixStride 16\n"
					"                             OpDecorate %InputStructArr ArrayStride 65536\n"
					"                             OpDecorate %Input ${inputdecoration}\n"
					"                             OpMemberDecorate %Input 0 Offset 0\n"
					"                             OpDecorate %dataInput DescriptorSet 0\n"
					"                             OpDecorate %dataInput Binding 0\n"
					"                             OpDecorate %_ptr_buffer_InputStruct ArrayStride 65536\n"
					"                             OpDecorate %_arr_v4uint_uint_128 ArrayStride 16\n"
					"                             OpMemberDecorate %DataSelector 0 Offset 0\n"
					"                             OpDecorate %DataSelector BufferBlock\n"
					"                             OpDecorate %selector DescriptorSet 0\n"
					"                             OpDecorate %selector Binding 1\n"
					"                     %void = OpTypeVoid\n"
					"                        %3 = OpTypeFunction %void\n"
					"                      %u32 = OpTypeInt 32 0\n"
					"                      %i32 = OpTypeInt 32 1\n"
					"${intdecl:opt}"
					"                    %idx_0 = OpConstant ${idx_int} 0\n"
					"                    %idx_1 = OpConstant ${idx_int} 1\n"
					"                    %idx_2 = OpConstant ${idx_int} 2\n"
					"                    %idx_3 = OpConstant ${idx_int} 3\n"
					"     %_ptr_Function_uint32 = OpTypePointer Function %u32\n"
					"                 %v3uint32 = OpTypeVector %u32 3\n"
					"      %_ptr_Input_v3uint32 = OpTypePointer Input %v3uint32\n"
					"    %gl_GlobalInvocationID = OpVariable %_ptr_Input_v3uint32 Input\n"
					"        %_ptr_Input_uint32 = OpTypePointer Input %u32\n"
					"                    %float = OpTypeFloat 32\n"
					"                 %uint_128 = OpConstant %u32 128\n"
					"                  %uint_32 = OpConstant %u32 32\n"
					"                   %uint_2 = OpConstant %u32 2\n"
					"      %_arr_float_uint_128 = OpTypeArray %float %uint_128\n"
					"                   %Output = OpTypeStruct %_arr_float_uint_128\n"
					"      %_ptr_Uniform_Output = OpTypePointer Uniform %Output\n"
					"               %dataOutput = OpVariable %_ptr_Uniform_Output Uniform\n"
					"                  %v4float = OpTypeVector %float 4\n"
					"              %mat4v4float = OpTypeMatrix %v4float 4\n"
					" %_arr_mat4v4float_uint_32 = OpTypeArray %mat4v4float %uint_32\n"
					" %_arr__arr_mat4v4float_uint_32_uint_32 = OpTypeArray %_arr_mat4v4float_uint_32 %uint_32\n"
					"              %InputStruct = OpTypeStruct %_arr__arr_mat4v4float_uint_32_uint_32\n"
					"           %InputStructArr = OpTypeArray %InputStruct %uint_2\n"
					"                    %Input = OpTypeStruct %InputStructArr\n"
					"        %_ptr_buffer_Input = OpTypePointer ${inputstorageclass} %Input\n"
					"                %dataInput = OpVariable %_ptr_buffer_Input ${inputstorageclass}\n"
					"  %_ptr_buffer_InputStruct = OpTypePointer ${inputstorageclass} %InputStruct\n"
					"                 %v4uint32 = OpTypeVector %u32 4\n"
					"     %_arr_v4uint_uint_128 = OpTypeArray %v4uint32 %uint_128\n"
					"             %DataSelector = OpTypeStruct %_arr_v4uint_uint_128\n"
					"%_ptr_Uniform_DataSelector = OpTypePointer Uniform %DataSelector\n"
					"                 %selector = OpVariable %_ptr_Uniform_DataSelector Uniform\n"
					"      %_ptr_Uniform_uint32 = OpTypePointer Uniform %u32\n"
					"       %_ptr_Uniform_float = OpTypePointer Uniform %float\n"
					"       ${ptr_buffer_float:opt}\n"

					"                     %main = OpFunction %void None %3\n"
					"                        %5 = OpLabel\n"
					"                        %i = OpVariable %_ptr_Function_uint32 Function\n"
					"                       %14 = OpAccessChain %_ptr_Input_uint32 %gl_GlobalInvocationID %idx_0\n"
					"                       %15 = OpLoad %u32 %14\n"
					"                             OpStore %i %15\n"
					"                   %uint_i = OpLoad %u32 %i\n"
					"                       %39 = OpAccessChain %_ptr_Uniform_uint32 %selector %idx_0 %uint_i %idx_0\n"
					"                       %40 = OpLoad %u32 %39\n"
					"                       %43 = OpAccessChain %_ptr_Uniform_uint32 %selector %idx_0 %uint_i %idx_1\n"
					"                       %44 = OpLoad %u32 %43\n"
					"                       %47 = OpAccessChain %_ptr_Uniform_uint32 %selector %idx_0 %uint_i %idx_2\n"
					"                       %48 = OpLoad %u32 %47\n"
					"                       %51 = OpAccessChain %_ptr_Uniform_uint32 %selector %idx_0 %uint_i %idx_3\n"
					"                       %52 = OpLoad %u32 %51\n"
					"                       %i0 = ${convert} ${idx_int} %40\n"
					"                       %i1 = ${convert} ${idx_int} %44\n"
					"                       %i2 = ${convert} ${idx_int} %48\n"
					"                       %i3 = ${convert} ${idx_int} %52\n"
					"        %inputFirstElement = OpAccessChain %_ptr_buffer_InputStruct %dataInput %idx_0 %idx_0\n"
					"                       %54 = ${accesschain}\n"
					"                       %55 = OpLoad %float %54\n"
					"                       %56 = OpAccessChain %_ptr_Uniform_float %dataOutput %idx_0 %uint_i\n"
					"                             OpStore %56 %55\n"
					"                             OpReturn\n"
					"                             OpFunctionEnd\n");


				switch (chainOpIdx)
				{
					case CHAIN_OP_ACCESS_CHAIN:
						specs["accesschain"]			= "OpAccessChain %_ptr_Uniform_float %inputFirstElement %idx_0 %i0 %i1 %i2 %i3\n";
						specs["inputdecoration"]		= "BufferBlock";
						specs["inputstorageclass"]		= "Uniform";
						break;
					case CHAIN_OP_IN_BOUNDS_ACCESS_CHAIN:
						specs["accesschain"]			= "OpInBoundsAccessChain %_ptr_Uniform_float %inputFirstElement %idx_0 %i0 %i1 %i2 %i3\n";
						specs["inputdecoration"]		= "BufferBlock";
						specs["inputstorageclass"]		= "Uniform";
						break;
					default:
						DE_ASSERT(chainOpIdx == CHAIN_OP_PTR_ACCESS_CHAIN);
						specs["accesschain"]			= "OpPtrAccessChain %_ptr_StorageBuffer_float %inputFirstElement %idx_1 %idx_0 %i0 %i1 %i2 %i3\n";
						specs["inputdecoration"]		= "Block";
						specs["inputstorageclass"]		= "StorageBuffer";
						specs["variablepointercaps"]		= "OpCapability VariablePointersStorageBuffer";
						specs["ptr_buffer_float"]		= "%_ptr_StorageBuffer_float = OpTypePointer StorageBuffer %float";
						specs["extensions"]			= "OpExtension \"SPV_KHR_variable_pointers\"\n                             "
														  "OpExtension \"SPV_KHR_storage_buffer_storage_class\"";
						element = 1;
						vulkanFeatures.extVariablePointers = EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS_STORAGEBUFFER;
						spec.extensions.push_back("VK_KHR_variable_pointers");
						break;
				};

				spec.inputs.push_back(BufferSp(new Float32Buffer(inputData)));
				spec.inputs.push_back(BufferSp(new Buffer<UVec4>(indexSelectorData)));

				outputData.reserve(numItems);
				for (deUint32 numIdx = 0; numIdx < numItems; ++numIdx)
				{
					// Determine the selected output float for the selected indices.
					const UVec4 vec = indexSelectorData[numIdx];
					outputData.push_back(inputData[element * sizeof(InputData) / 4 + vec.x() * (32 * 4 * 4) + vec.y() * 4 * 4 + vec.z() * 4 + vec.w()]);
				}

				if (idxSize == 16)
				{
					specs["intcaps"] = "OpCapability Int16";
					specs["convert"] = "OpSConvert";
					specs["intdecl"] =	"                      %u16 = OpTypeInt 16 0\n"
								"                      %i16 = OpTypeInt 16 1\n";
				}
				else if (idxSize == 64)
				{
					specs["intcaps"] = "OpCapability Int64";
					specs["convert"] = "OpSConvert";
					specs["intdecl"] =	"                      %u64 = OpTypeInt 64 0\n"
								"                      %i64 = OpTypeInt 64 1\n";
				} else {
					specs["convert"] = "OpBitcast";
				}

				specs["idx_uint"] = "%u" + de::toString(idxSize);
				specs["idx_int"] = (sign ? "%i" : "%u") + de::toString(idxSize);

				spec.assembly					= shaderSource.specialize(specs);
				spec.numWorkGroups				= IVec3(numItems, 1, 1);
				spec.requestedVulkanFeatures	= vulkanFeatures;
				spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

				spec.outputs.push_back(BufferSp(new Float32Buffer(outputData)));

				group->addChild(new SpvAsmComputeShaderCase(testCtx, testName.c_str(), testName.c_str(), spec, features));
			}
		}
	}
}

void addGraphicsIndexingTests (tcu::TestCaseGroup* group)
{
	de::Random			rnd				(deStringHash(group->getName()));
	const int			numItems		= 128;
	const int			numStructs		= 2;
	const int			numInputFloats	= (int)sizeof(InputData) / 4 * numStructs;
	RGBA				defaultColors[4];
	vector<float>		inputData;
	vector<UVec4>		indexSelectorData;

	inputData.reserve(numInputFloats);
	for (deUint32 numIdx = 0; numIdx < numInputFloats; ++numIdx)
		inputData.push_back(rnd.getFloat());

	indexSelectorData.reserve(numItems);
	for (deUint32 numIdx = 0; numIdx < numItems; ++numIdx)
		indexSelectorData.push_back(UVec4(rnd.getUint32() % 32, rnd.getUint32() % 32, rnd.getUint32() % 4, rnd.getUint32() % 4));

	getDefaultColors(defaultColors);

	for (int chainOpIdx = 0; chainOpIdx < CHAIN_OP_LAST; ++chainOpIdx)
	{
		for (int idxSizeIdx = 0; idxSizeIdx < DE_LENGTH_OF_ARRAY(idxSizes); ++idxSizeIdx)
		{
			for (int sign = 0; sign < 2; sign++)
			{
				const int					idxSize			= idxSizes[idxSizeIdx];
				const string				testName		= chainOpTestNames[chainOpIdx] + string(sign == 0 ? "_u" : "_s") + de::toString(idxSize);
				VulkanFeatures				vulkanFeatures;
				vector<string>				extensions;
				vector<string>				features;
				vector<deInt32>				noSpecConstants;
				PushConstants				noPushConstants;
				GraphicsInterfaces			noInterfaces;
				map<string, string>			specs;
				map<string, string>			fragments;
				vector<float>				outputData;
				ComputeShaderSpec			spec;
				int							element			= 0;
				GraphicsResources			resources;

				const StringTemplate		preMain(
					"${intdecl:opt}"
					"                %c_i32_128 = OpConstant %i32 128\n"
					"                   %uint_0 = OpConstant ${idx_uint} 0\n"
					"                 %uint_128 = OpConstant %u32 128\n"
					"                  %uint_32 = OpConstant %u32 32\n"
					"                   %uint_1 = OpConstant ${idx_uint} 1\n"
					"                   %uint_2 = OpConstant ${idx_uint} 2\n"
					"                   %uint_3 = OpConstant ${idx_uint} 3\n"
					"      %_arr_float_uint_128 = OpTypeArray %f32 %uint_128\n"
					"                   %Output = OpTypeStruct %_arr_float_uint_128\n"
					"      %_ptr_Uniform_Output = OpTypePointer Uniform %Output\n"
					"               %dataOutput = OpVariable %_ptr_Uniform_Output Uniform\n"
					"                    %int_0 = OpConstant ${idx_int} 0\n"
					"              %mat4v4float = OpTypeMatrix %v4f32 4\n"
					" %_arr_mat4v4float_uint_32 = OpTypeArray %mat4v4float %uint_32\n"
					" %_arr__arr_mat4v4float_uint_32_uint_32 = OpTypeArray %_arr_mat4v4float_uint_32 %uint_32\n"
					"              %InputStruct = OpTypeStruct %_arr__arr_mat4v4float_uint_32_uint_32\n"
					"           %InputStructArr = OpTypeArray %InputStruct %uint_2\n"
					"                    %Input = OpTypeStruct %InputStructArr\n"
					"        %_ptr_buffer_Input = OpTypePointer ${inputstorageclass} %Input\n"
					"                %dataInput = OpVariable %_ptr_buffer_Input ${inputstorageclass}\n"
					"  %_ptr_buffer_InputStruct = OpTypePointer ${inputstorageclass} %InputStruct\n"
					"     %_arr_v4uint_uint_128 = OpTypeArray %v4u32 %uint_128\n"
					"             %DataSelector = OpTypeStruct %_arr_v4uint_uint_128\n"
					"%_ptr_Uniform_DataSelector = OpTypePointer Uniform %DataSelector\n"
					"                 %selector = OpVariable %_ptr_Uniform_DataSelector Uniform\n"
					"      %_ptr_Uniform_uint32 = OpTypePointer Uniform %u32\n"
					"       %_ptr_Uniform_float = OpTypePointer Uniform %f32\n"
					"       ${ptr_buffer_float:opt}\n");


				const StringTemplate		decoration(
					"OpDecorate %_arr_float_uint_128 ArrayStride 4\n"
					"OpMemberDecorate %Output 0 Offset 0\n"
					"OpDecorate %Output BufferBlock\n"
					"OpDecorate %dataOutput DescriptorSet 0\n"
					"OpDecorate %dataOutput Binding 2\n"
					"OpDecorate %_arr_mat4v4float_uint_32 ArrayStride 64\n"
					"OpDecorate %_arr__arr_mat4v4float_uint_32_uint_32 ArrayStride 2048\n"
					"OpMemberDecorate %InputStruct 0 ColMajor\n"
					"OpMemberDecorate %InputStruct 0 Offset 0\n"
					"OpMemberDecorate %InputStruct 0 MatrixStride 16\n"
					"OpDecorate %InputStructArr ArrayStride 65536\n"
					"OpDecorate %Input ${inputdecoration}\n"
					"OpMemberDecorate %Input 0 Offset 0\n"
					"OpDecorate %dataInput DescriptorSet 0\n"
					"OpDecorate %dataInput Binding 0\n"
					"OpDecorate %_ptr_buffer_InputStruct ArrayStride 65536\n"
					"OpDecorate %_arr_v4uint_uint_128 ArrayStride 16\n"
					"OpMemberDecorate %DataSelector 0 Offset 0\n"
					"OpDecorate %DataSelector BufferBlock\n"
					"OpDecorate %selector DescriptorSet 0\n"
					"OpDecorate %selector Binding 1\n");

				// Index an input buffer containing 2D array of 4x4 matrices. The indices are read from another
				// input and converted to the desired bit size and sign.
				const StringTemplate		testFun(
					"        %test_code = OpFunction %v4f32 None %v4f32_function\n"
					"            %param = OpFunctionParameter %v4f32\n"

					"            %entry = OpLabel\n"
					"                %i = OpVariable %fp_i32 Function\n"
					"                     OpStore %i %c_i32_0\n"
					"                     OpBranch %loop\n"

					"             %loop = OpLabel\n"
					"               %15 = OpLoad %i32 %i\n"
					"               %lt = OpSLessThan %bool %15 %c_i32_128\n"
					"                     OpLoopMerge %merge %inc None\n"
					"                     OpBranchConditional %lt %write %merge\n"

					"            %write = OpLabel\n"
					"            %int_i = OpLoad %i32 %i\n"
					"               %39 = OpAccessChain %_ptr_Uniform_uint32 %selector %int_0 %int_i %uint_0\n"
					"               %40 = OpLoad %u32 %39\n"
					"               %43 = OpAccessChain %_ptr_Uniform_uint32 %selector %int_0 %int_i %uint_1\n"
					"               %44 = OpLoad %u32 %43\n"
					"               %47 = OpAccessChain %_ptr_Uniform_uint32 %selector %int_0 %int_i %uint_2\n"
					"               %48 = OpLoad %u32 %47\n"
					"               %51 = OpAccessChain %_ptr_Uniform_uint32 %selector %int_0 %int_i %uint_3\n"
					"               %52 = OpLoad %u32 %51\n"
					"               %i0 = ${convert} ${idx_uint} %40\n"
					"               %i1 = ${convert} ${idx_uint} %44\n"
					"               %i2 = ${convert} ${idx_uint} %48\n"
					"               %i3 = ${convert} ${idx_uint} %52\n"
					"%inputFirstElement = OpAccessChain %_ptr_buffer_InputStruct %dataInput %uint_0 %uint_0\n"
					"               %54 = ${accesschain}\n"
					"               %55 = OpLoad %f32 %54\n"
					"               %56 = OpAccessChain %_ptr_Uniform_float %dataOutput %int_0 %int_i\n"
					"                     OpStore %56 %55\n"
					"                     OpBranch %inc\n"

					"              %inc = OpLabel\n"
					"               %67 = OpLoad %i32 %i\n"
					"               %69 = OpIAdd %i32 %67 %c_i32_1\n"
					"                     OpStore %i %69\n"
					"                     OpBranch %loop\n"

					"            %merge = OpLabel\n"
					"                     OpReturnValue %param\n"

					"                     OpFunctionEnd\n");

				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputData))));
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Buffer<UVec4>(indexSelectorData))));

				if (idxSize == 16)
				{
					fragments["capability"] = "OpCapability Int16\n";
					features.push_back("shaderInt16");
					specs["convert"] = "OpUConvert";
					specs["intdecl"] =	"                      %u16 = OpTypeInt 16 0\n"
								"                      %i16 = OpTypeInt 16 1\n";
				}
				else if (idxSize == 64)
				{
					fragments["capability"] = "OpCapability Int64\n";
					features.push_back("shaderInt64");
					specs["convert"] = "OpUConvert";
					specs["intdecl"] =	"                      %u64 = OpTypeInt 64 0\n"
								"                      %i64 = OpTypeInt 64 1\n";
				} else {
					specs["convert"] = "OpCopyObject";
				}

				specs["idx_uint"] = "%u" + de::toString(idxSize);
				specs["idx_int"] = (sign ? "%i" : "%u") + de::toString(idxSize);

				switch (chainOpIdx)
				{
					case CHAIN_OP_ACCESS_CHAIN:
						specs["accesschain"]				= "OpAccessChain %_ptr_Uniform_float %inputFirstElement %int_0 %i0 %i1 %i2 %i3\n";
						specs["inputdecoration"]			= "BufferBlock";
						specs["inputstorageclass"]			= "Uniform";
						break;
					case CHAIN_OP_IN_BOUNDS_ACCESS_CHAIN:
						specs["accesschain"]				= "OpInBoundsAccessChain %_ptr_Uniform_float %inputFirstElement %int_0 %i0 %i1 %i2 %i3\n";
						specs["inputdecoration"]			= "BufferBlock";
						specs["inputstorageclass"]			= "Uniform";
						break;
					default:
						DE_ASSERT(chainOpIdx == CHAIN_OP_PTR_ACCESS_CHAIN);
						specs["accesschain"]				= "OpPtrAccessChain %_ptr_StorageBuffer_float %inputFirstElement %uint_1 %int_0 %i0 %i1 %i2 %i3\n";
						specs["inputdecoration"]			= "Block";
						specs["inputstorageclass"]			= "StorageBuffer";
						specs["ptr_buffer_float"]			= "%_ptr_StorageBuffer_float = OpTypePointer StorageBuffer %f32";
						fragments["capability"]				+= "OpCapability VariablePointersStorageBuffer";
						fragments["extension"]				= "OpExtension \"SPV_KHR_variable_pointers\"\nOpExtension \"SPV_KHR_storage_buffer_storage_class\"";
						extensions.push_back				("VK_KHR_variable_pointers");
						vulkanFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS_STORAGEBUFFER;
						element = 1;
						break;
				};

				outputData.reserve(numItems);
				for (deUint32 numIdx = 0; numIdx < numItems; ++numIdx)
				{
					// Determine the selected output float for the selected indices.
					const UVec4 vec = indexSelectorData[numIdx];
					outputData.push_back(inputData[element * sizeof(InputData) / 4 + vec.x() * (32 * 4 * 4) + vec.y() * 4 * 4 + vec.z() * 4 + vec.w()]);
				}

				resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(outputData))));

				fragments["pre_main"]	= preMain.specialize(specs);
				fragments["decoration"]	= decoration.specialize(specs);
				fragments["testfun"]	= testFun.specialize(specs);

				createTestsForAllStages(
						testName.c_str(), defaultColors, defaultColors, fragments, noSpecConstants,
						noPushConstants, resources, noInterfaces, extensions, features, vulkanFeatures, group);
			}
		}
	}
}

} // anonymous

tcu::TestCaseGroup* createIndexingComputeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "indexing", "Compute tests for data indexing."));
	addComputeIndexingTests(group.get());

	return group.release();
}

tcu::TestCaseGroup* createIndexingGraphicsGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "indexing", "Graphics tests for data indexing."));
	addGraphicsIndexingTests(group.get());

	return group.release();
}

} // SpirVAssembly
} // vkt
