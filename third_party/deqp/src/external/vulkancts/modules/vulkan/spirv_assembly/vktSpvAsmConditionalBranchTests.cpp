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
 * \brief SPIR-V Assembly Tests for OpBranchConditional instruction.
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmConditionalBranchTests.hpp"
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
using tcu::RGBA;
using tcu::IVec3;
using tcu::StringTemplate;

namespace
{

static const string conditions[] = { "true", "false" };

void addComputeSameLabelsTest (tcu::TestCaseGroup* group)
{
	tcu::TestContext&		testCtx			= group->getTestContext();
	const int				numItems		= 128;
	vector<deUint32>		outputData;

	outputData.reserve(numItems);
	for (deUint32 numIdx = 0; numIdx < numItems; ++numIdx)
		outputData.push_back(numIdx);

	for (int conditionIdx = 0; conditionIdx < DE_LENGTH_OF_ARRAY(conditions); ++conditionIdx)
	{
		ComputeShaderSpec		spec;
		map<string, string>		specs;
		string					testName		= string("same_labels_") + conditions[conditionIdx];

		const StringTemplate	shaderSource	(
			"                         OpCapability Shader\n"
			"                    %1 = OpExtInstImport \"GLSL.std.450\"\n"
			"                         OpMemoryModel Logical GLSL450\n"
			"                         OpEntryPoint GLCompute %main \"main\" %gl_GlobalInvocationID\n"
			"                         OpExecutionMode %main LocalSize 1 1 1\n"
			"                         OpSource GLSL 430\n"
			"                         OpDecorate %gl_GlobalInvocationID BuiltIn GlobalInvocationId\n"
			"                         OpDecorate %_arr_uint_uint_128 ArrayStride 4\n"
			"                         OpMemberDecorate %Output 0 Offset 0\n"
			"                         OpDecorate %Output BufferBlock\n"
			"                         OpDecorate %dataOutput DescriptorSet 0\n"
			"                         OpDecorate %dataOutput Binding 0\n"
			"                 %void = OpTypeVoid\n"
			"                    %3 = OpTypeFunction %void\n"
			"                 %uint = OpTypeInt 32 0\n"
			"   %_ptr_Function_uint = OpTypePointer Function %uint\n"
			"               %v3uint = OpTypeVector %uint 3\n"
			"    %_ptr_Input_v3uint = OpTypePointer Input %v3uint\n"
			"%gl_GlobalInvocationID = OpVariable %_ptr_Input_v3uint Input\n"
			"               %uint_0 = OpConstant %uint 0\n"
			"      %_ptr_Input_uint = OpTypePointer Input %uint\n"
			"                 %bool = OpTypeBool\n"
			"                 %true = OpConstantTrue %bool\n"
			"                %false = OpConstantFalse %bool\n"
			"             %uint_128 = OpConstant %uint 128\n"
			"   %_arr_uint_uint_128 = OpTypeArray %uint %uint_128\n"
			"               %Output = OpTypeStruct %_arr_uint_uint_128\n"
			"  %_ptr_Uniform_Output = OpTypePointer Uniform %Output\n"
			"           %dataOutput = OpVariable %_ptr_Uniform_Output Uniform\n"
			"    %_ptr_Uniform_uint = OpTypePointer Uniform %uint\n"
			"           %uint_dummy = OpConstant %uint 2863311530\n"
			"                 %main = OpFunction %void None %3\n"
			"                    %5 = OpLabel\n"
			"                    %i = OpVariable %_ptr_Function_uint Function\n"
			"                   %14 = OpAccessChain %_ptr_Input_uint %gl_GlobalInvocationID %uint_0\n"
			"                   %15 = OpLoad %uint %14\n"
			"                         OpStore %i %15\n"
			"               %uint_i = OpLoad %uint %i\n"
			"                         OpSelectionMerge %merge None\n"
			"                         OpBranchConditional %${condition} %live %live\n"
			"                 %live = OpLabel\n"
			"                   %31 = OpAccessChain %_ptr_Uniform_uint %dataOutput %uint_0 %uint_i\n"
			"                         OpStore %31 %uint_i\n"
			"                         OpBranch %merge\n"
			"                 %dead = OpLabel\n"
			"                   %35 = OpAccessChain %_ptr_Uniform_uint %dataOutput %uint_0 %uint_i\n"
			"                         OpStore %35 %uint_dummy\n"
			"                         OpBranch %merge\n"
			"                %merge = OpLabel\n"
			"                         OpReturn\n"
			"                         OpFunctionEnd\n");

		specs["condition"]		= conditions[conditionIdx];
		spec.assembly			= shaderSource.specialize(specs);
		spec.numWorkGroups		= IVec3(numItems, 1, 1);

		spec.outputs.push_back(BufferSp(new Buffer<deUint32>(outputData)));

		group->addChild(new SpvAsmComputeShaderCase(testCtx, testName.c_str(), "Tests both labels pointing to a same branch.", spec));
	}
}

void addGraphicsSameLabelsTest (tcu::TestCaseGroup* group)
{
	const deUint32			numItems			= 128;
	RGBA					defaultColors[4];
	GraphicsResources		resources;
	vector<deUint32>		outputData;

	outputData.reserve(numItems);
	for (deUint32 numIdx = 0; numIdx < numItems; ++numIdx)
		outputData.push_back(numIdx);

	resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Buffer<deUint32>(outputData))));

	getDefaultColors(defaultColors);

	for (int conditionIdx = 0; conditionIdx < DE_LENGTH_OF_ARRAY(conditions); ++conditionIdx)
	{
		map<string, string>		fragments;
		map<string, string>		specs;
		string					testName	= string("same_labels_") + conditions[conditionIdx];

		fragments["pre_main"]				=
			"          %c_u32_128 = OpConstant %u32 128\n"
			"               %true = OpConstantTrue %bool\n"
			"              %false = OpConstantFalse %bool\n"
			" %_arr_uint_uint_128 = OpTypeArray %u32 %c_u32_128\n"
			"             %Output = OpTypeStruct %_arr_uint_uint_128\n"
			"%_ptr_Uniform_Output = OpTypePointer Uniform %Output\n"
			"         %dataOutput = OpVariable %_ptr_Uniform_Output Uniform\n"
			"  %_ptr_Uniform_uint = OpTypePointer Uniform %u32\n"
			"             %fp_u32 = OpTypePointer Function %u32\n"
			"         %uint_dummy = OpConstant %u32 2863311530\n";

		fragments["decoration"]				=
			"                       OpDecorate %_arr_uint_uint_128 ArrayStride 4\n"
			"                       OpMemberDecorate %Output 0 Offset 0\n"
			"                       OpDecorate %Output BufferBlock\n"
			"                       OpDecorate %dataOutput DescriptorSet 0\n"
			"                       OpDecorate %dataOutput Binding 0\n";

		const StringTemplate	testFun		(
			"          %test_code = OpFunction %v4f32 None %v4f32_function\n"
			"              %param = OpFunctionParameter %v4f32\n"

			"              %entry = OpLabel\n"
			"                  %i = OpVariable %fp_u32 Function\n"
			"                       OpStore %i %c_u32_0\n"
			"                       OpBranch %loop\n"

			"               %loop = OpLabel\n"
			"                 %15 = OpLoad %u32 %i\n"
			"                 %lt = OpSLessThan %bool %15 %c_u32_128\n"
			"                       OpLoopMerge %merge %inc None\n"
			"                       OpBranchConditional %lt %write %merge\n"

			"              %write = OpLabel\n"
			"             %uint_i = OpLoad %u32 %i\n"
			"                       OpSelectionMerge %condmerge None\n"
			"                       OpBranchConditional %${condition} %live %live\n"
			"               %live = OpLabel\n"
			"                 %31 = OpAccessChain %_ptr_Uniform_uint %dataOutput %c_u32_0 %uint_i\n"
			"                       OpStore %31 %uint_i\n"
			"                       OpBranch %condmerge\n"
			"               %dead = OpLabel\n"
			"                 %35 = OpAccessChain %_ptr_Uniform_uint %dataOutput %c_u32_0 %uint_i\n"
			"                       OpStore %35 %uint_dummy\n"
			"                       OpBranch %condmerge\n"
			"          %condmerge = OpLabel\n"
			"                       OpBranch %inc\n"

			"                %inc = OpLabel\n"
			"                 %37 = OpLoad %u32 %i\n"
			"                 %39 = OpIAdd %u32 %37 %c_u32_1\n"
			"                       OpStore %i %39\n"
			"                       OpBranch %loop\n"

			"              %merge = OpLabel\n"
			"                       OpReturnValue %param\n"

			"                       OpFunctionEnd\n");

		specs["condition"]		= conditions[conditionIdx];
		fragments["testfun"]	= testFun.specialize(specs);

		createTestsForAllStages(testName.c_str(), defaultColors, defaultColors, fragments, resources, vector<string>(), group);
	}
}

} // anonymous

tcu::TestCaseGroup* createConditionalBranchComputeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "conditional_branch", "Compute tests for OpBranchConditional."));
	addComputeSameLabelsTest(group.get());

	return group.release();
}

tcu::TestCaseGroup* createConditionalBranchGraphicsGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "conditional_branch", "Graphics tests for OpBranchConditional."));
	addGraphicsSameLabelsTest(group.get());

	return group.release();
}

} // SpirVAssembly
} // vkt
