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
 * \brief SPIR-V Assembly Tests for UBO matrix padding.
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmUboMatrixPaddingTests.hpp"
#include "vktSpvAsmComputeShaderCase.hpp"
#include "vktSpvAsmComputeShaderTestUtil.hpp"
#include "vktSpvAsmGraphicsShaderTestUtil.hpp"

namespace vkt
{
namespace SpirVAssembly
{

using namespace vk;
using std::map;
using std::string;
using std::vector;
using tcu::IVec3;
using tcu::RGBA;
using tcu::Vec4;

namespace
{

void addComputeUboMatrixPaddingTest (tcu::TestCaseGroup* group)
{
	tcu::TestContext&	testCtx			= group->getTestContext();
	de::Random			rnd				(deStringHash(group->getName()));
	const int			numElements		= 128;

	// Read input UBO containing and array of mat2x2 using no padding inside matrix. Output
	// into output buffer containing floats. The input and output buffer data should match.
	const string		shaderSource	=
		"                       OpCapability Shader\n"
		"                  %1 = OpExtInstImport \"GLSL.std.450\"\n"
		"                       OpMemoryModel Logical GLSL450\n"
		"                       OpEntryPoint GLCompute %main \"main\" %id\n"
		"                       OpExecutionMode %main LocalSize 1 1 1\n"
		"                       OpSource GLSL 430\n"
		"                       OpDecorate %id BuiltIn GlobalInvocationId\n"
		"                       OpDecorate %_arr_v4 ArrayStride 16\n"
		"                       OpMemberDecorate %Output 0 Offset 0\n"
		"                       OpDecorate %Output BufferBlock\n"
		"                       OpDecorate %dataOutput DescriptorSet 0\n"
		"                       OpDecorate %dataOutput Binding 1\n"
		"                       OpDecorate %_arr_mat2v2 ArrayStride 16\n"
		"                       OpMemberDecorate %Input 0 ColMajor\n"
		"                       OpMemberDecorate %Input 0 Offset 0\n"
		"                       OpMemberDecorate %Input 0 MatrixStride 8\n"
		"                       OpDecorate %Input Block\n"
		"                       OpDecorate %dataInput DescriptorSet 0\n"
		"                       OpDecorate %dataInput Binding 0\n"
		"               %void = OpTypeVoid\n"
		"                  %3 = OpTypeFunction %void\n"
		"                %u32 = OpTypeInt 32 0\n"
		" %_ptr_Function_uint = OpTypePointer Function %u32\n"
		"             %v3uint = OpTypeVector %u32 3\n"
		"  %_ptr_Input_v3uint = OpTypePointer Input %v3uint\n"
		"                 %id = OpVariable %_ptr_Input_v3uint Input\n"
		"                %i32 = OpTypeInt 32 1\n"
		"              %int_0 = OpConstant %i32 0\n"
		"              %int_1 = OpConstant %i32 1\n"
		"             %uint_0 = OpConstant %u32 0\n"
		"             %uint_1 = OpConstant %u32 1\n"
		"             %uint_2 = OpConstant %u32 2\n"
		"             %uint_3 = OpConstant %u32 3\n"
		"    %_ptr_Input_uint = OpTypePointer Input %u32\n"
		"                %f32 = OpTypeFloat 32\n"
		"            %v4float = OpTypeVector %f32 4\n"
		"           %uint_128 = OpConstant %u32 128\n"
		"            %_arr_v4 = OpTypeArray %v4float %uint_128\n"
		"             %Output = OpTypeStruct %_arr_v4\n"
		"%_ptr_Uniform_Output = OpTypePointer Uniform %Output\n"
		"         %dataOutput = OpVariable %_ptr_Uniform_Output Uniform\n"
		"            %v2float = OpTypeVector %f32 2\n"
		"        %mat2v2float = OpTypeMatrix %v2float 2\n"
		"        %_arr_mat2v2 = OpTypeArray %mat2v2float %uint_128\n"
		"              %Input = OpTypeStruct %_arr_mat2v2\n"
		" %_ptr_Uniform_Input = OpTypePointer Uniform %Input\n"
		"          %dataInput = OpVariable %_ptr_Uniform_Input Uniform\n"
		" %_ptr_Uniform_float = OpTypePointer Uniform %f32\n"
		"               %main = OpFunction %void None %3\n"
		"                  %5 = OpLabel\n"
		"                  %i = OpVariable %_ptr_Function_uint Function\n"
		"                 %14 = OpAccessChain %_ptr_Input_uint %id %uint_0\n"
		"                 %15 = OpLoad %u32 %14\n"
		"                       OpStore %i %15\n"
		"                %idx = OpLoad %u32 %i\n"
		"                 %34 = OpAccessChain %_ptr_Uniform_float %dataInput %int_0 %idx %int_0 %uint_0\n"
		"                 %35 = OpLoad %f32 %34\n"
		"                 %36 = OpAccessChain %_ptr_Uniform_float %dataOutput %int_0 %idx %uint_0\n"
		"                       OpStore %36 %35\n"
		"                 %40 = OpAccessChain %_ptr_Uniform_float %dataInput %int_0 %idx %int_0 %uint_1\n"
		"                 %41 = OpLoad %f32 %40\n"
		"                 %42 = OpAccessChain %_ptr_Uniform_float %dataOutput %int_0 %idx %uint_1\n"
		"                       OpStore %42 %41\n"
		"                 %46 = OpAccessChain %_ptr_Uniform_float %dataInput %int_0 %idx %int_1 %uint_0\n"
		"                 %47 = OpLoad %f32 %46\n"
		"                 %49 = OpAccessChain %_ptr_Uniform_float %dataOutput %int_0 %idx %uint_2\n"
		"                       OpStore %49 %47\n"
		"                 %52 = OpAccessChain %_ptr_Uniform_float %dataInput %int_0 %idx %int_1 %uint_1\n"
		"                 %53 = OpLoad %f32 %52\n"
		"                 %55 = OpAccessChain %_ptr_Uniform_float %dataOutput %int_0 %idx %uint_3\n"
		"                       OpStore %55 %53\n"
		"                       OpReturn\n"
		"                       OpFunctionEnd\n";

		vector<tcu::Vec4>		inputData;
		ComputeShaderSpec		spec;

		inputData.reserve(numElements);
		for (deUint32 numIdx = 0; numIdx < numElements; ++numIdx)
			inputData.push_back(tcu::Vec4(rnd.getFloat(), rnd.getFloat(), rnd.getFloat(), rnd.getFloat()));

		spec.assembly			= shaderSource;
		spec.numWorkGroups		= IVec3(numElements, 1, 1);
		spec.inputTypes[0]		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		spec.inputs.push_back(BufferSp(new Vec4Buffer(inputData)));
		// Shader is expected to pass the input data by treating the input vec4 as mat2x2
		spec.outputs.push_back(BufferSp(new Vec4Buffer(inputData)));

		group->addChild(new SpvAsmComputeShaderCase(testCtx, "mat2x2", "Tests mat2x2 member in UBO struct without padding (treated as vec4).", spec));
	}
}

void addGraphicsUboMatrixPaddingTest (tcu::TestCaseGroup* group)
{
	de::Random				rnd					(deStringHash(group->getName()));
	map<string, string>		fragments;
	const deUint32			numDataPoints		= 128;
	RGBA					defaultColors[4];
	GraphicsResources		resources;
	vector<tcu::Vec4>		inputData;

	inputData.reserve(numDataPoints);
	for (deUint32 numIdx = 0; numIdx < numDataPoints; ++numIdx)
		inputData.push_back(tcu::Vec4(rnd.getFloat(), rnd.getFloat(), rnd.getFloat(), rnd.getFloat()));

	resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, BufferSp(new Vec4Buffer(inputData))));
	// Shader is expected to pass the input data by treating the input vec4 as mat2x2
	resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Vec4Buffer(inputData))));

	getDefaultColors(defaultColors);

	fragments["pre_main"]	=
		"             %uint_128 = OpConstant %u32 128\n"
		"    %_arr_v4f_uint_128 = OpTypeArray %v4f32 %uint_128\n"
		"               %Output = OpTypeStruct %_arr_v4f_uint_128\n"
		"  %_ptr_Uniform_Output = OpTypePointer Uniform %Output\n"
		"           %dataOutput = OpVariable %_ptr_Uniform_Output Uniform\n"
		"              %mat2v2f = OpTypeMatrix %v2f32 2\n"
		"%_arr_mat2v2f_uint_128 = OpTypeArray %mat2v2f %uint_128\n"
		"                %Input = OpTypeStruct %_arr_mat2v2f_uint_128\n"
		"   %_ptr_Uniform_Input = OpTypePointer Uniform %Input\n"
		"            %dataInput = OpVariable %_ptr_Uniform_Input Uniform\n"
		"       %_ptr_Uniform_f = OpTypePointer Uniform %f32\n"
		"            %c_i32_128 = OpConstant %i32 128\n";

	fragments["decoration"]	=
		"                         OpDecorate %_arr_v4f_uint_128 ArrayStride 16\n"
		"                         OpMemberDecorate %Output 0 Offset 0\n"
		"                         OpDecorate %Output BufferBlock\n"
		"                         OpDecorate %dataOutput DescriptorSet 0\n"
		"                         OpDecorate %dataOutput Binding 1\n"
		"                         OpDecorate %_arr_mat2v2f_uint_128 ArrayStride 16\n"
		"                         OpMemberDecorate %Input 0 ColMajor\n"
		"                         OpMemberDecorate %Input 0 Offset 0\n"
		"                         OpMemberDecorate %Input 0 MatrixStride 8\n"
		"                         OpDecorate %Input Block\n"
		"                         OpDecorate %dataInput DescriptorSet 0\n"
		"                         OpDecorate %dataInput Binding 0\n";

	// Read input UBO containing and array of mat2x2 using no padding inside matrix. Output
	// into output buffer containing floats. The input and output buffer data should match.
	// The whole array is handled inside a for loop.
	fragments["testfun"]	=
		"            %test_code = OpFunction %v4f32 None %v4f32_function\n"
		"                %param = OpFunctionParameter %v4f32\n"

		"                %entry = OpLabel\n"
		"                    %i = OpVariable %fp_i32 Function\n"
		"                         OpStore %i %c_i32_0\n"
		"                         OpBranch %loop\n"

		"                 %loop = OpLabel\n"
		"                   %15 = OpLoad %i32 %i\n"
		"                   %lt = OpSLessThan %bool %15 %c_i32_128\n"
		"                         OpLoopMerge %merge %inc None\n"
		"                         OpBranchConditional %lt %write %merge\n"

		"                %write = OpLabel\n"
		"                   %30 = OpLoad %i32 %i\n"
		"                   %34 = OpAccessChain %_ptr_Uniform_f %dataInput %c_i32_0 %30 %c_i32_0 %c_u32_0\n"
		"                   %35 = OpLoad %f32 %34\n"
		"                   %36 = OpAccessChain %_ptr_Uniform_f %dataOutput %c_i32_0 %30 %c_u32_0\n"
		"                         OpStore %36 %35\n"
		"                   %40 = OpAccessChain %_ptr_Uniform_f %dataInput %c_i32_0 %30 %c_i32_0 %c_u32_1\n"
		"                   %41 = OpLoad %f32 %40\n"
		"                   %42 = OpAccessChain %_ptr_Uniform_f %dataOutput %c_i32_0 %30 %c_u32_1\n"
		"                         OpStore %42 %41\n"
		"                   %46 = OpAccessChain %_ptr_Uniform_f %dataInput %c_i32_0 %30 %c_i32_1 %c_u32_0\n"
		"                   %47 = OpLoad %f32 %46\n"
		"                   %49 = OpAccessChain %_ptr_Uniform_f %dataOutput %c_i32_0 %30 %c_u32_2\n"
		"                         OpStore %49 %47\n"
		"                   %52 = OpAccessChain %_ptr_Uniform_f %dataInput %c_i32_0 %30 %c_i32_1 %c_u32_1\n"
		"                   %53 = OpLoad %f32 %52\n"
		"                   %55 = OpAccessChain %_ptr_Uniform_f %dataOutput %c_i32_0 %30 %c_u32_3\n"
		"                         OpStore %55 %53\n"
		"                         OpBranch %inc\n"

		"                  %inc = OpLabel\n"
		"                   %37 = OpLoad %i32 %i\n"
		"                   %39 = OpIAdd %i32 %37 %c_i32_1\n"
		"                         OpStore %i %39\n"
		"                         OpBranch %loop\n"

		"                %merge = OpLabel\n"
		"                         OpReturnValue %param\n"

		"                         OpFunctionEnd\n";

	resources.inputs.back().first	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	createTestsForAllStages("mat2x2", defaultColors, defaultColors, fragments, resources, vector<string>(), group);
}

tcu::TestCaseGroup* createUboMatrixPaddingComputeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "ubo_padding", "Compute tests for UBO struct member packing."));
	addComputeUboMatrixPaddingTest(group.get());

	return group.release();
}

tcu::TestCaseGroup* createUboMatrixPaddingGraphicsGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "ubo_padding", "Graphics tests for UBO struct member packing."));
	addGraphicsUboMatrixPaddingTest(group.get());

	return group.release();
}

} // SpirVAssembly
} // vkt
