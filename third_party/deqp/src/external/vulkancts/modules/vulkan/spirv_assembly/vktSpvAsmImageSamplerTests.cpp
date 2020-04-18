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
 * \brief SPIR-V Assembly Tests for images and samplers.
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmImageSamplerTests.hpp"
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
enum TestType
{
	TESTTYPE_LOCAL_VARIABLES = 0,
	TESTTYPE_PASS_IMAGE_TO_FUNCTION,
	TESTTYPE_PASS_SAMPLER_TO_FUNCTION,
	TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION,

	TESTTYPE_LAST
};

enum ReadOp
{
	READOP_IMAGEREAD = 0,
	READOP_IMAGEFETCH,
	READOP_IMAGESAMPLE,

	READOP_LAST
};

enum DescriptorType
{
	DESCRIPTOR_TYPE_STORAGE_IMAGE = 0,
	DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,

	DESCRIPTOR_TYPE_LAST
};

bool isValidTestCase (TestType testType, DescriptorType descriptorType, ReadOp readOp)
{
	// Check valid descriptor type and test type combinations
	switch (testType)
	{
		case TESTTYPE_PASS_IMAGE_TO_FUNCTION:
			if (descriptorType != DESCRIPTOR_TYPE_STORAGE_IMAGE				&&
				descriptorType != DESCRIPTOR_TYPE_SAMPLED_IMAGE				)
					return false;
			break;

		case TESTTYPE_PASS_SAMPLER_TO_FUNCTION:
			if (descriptorType != DESCRIPTOR_TYPE_SAMPLED_IMAGE)
				return false;
			break;

		case TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION:
			if (descriptorType != DESCRIPTOR_TYPE_SAMPLED_IMAGE)
				return false;
			break;

		default:
			break;
	}

	// Check valid descriptor type and read operation combinations
	switch (readOp)
	{
		case READOP_IMAGEREAD:
			if (descriptorType != DESCRIPTOR_TYPE_STORAGE_IMAGE)
				return false;
			break;

		case READOP_IMAGEFETCH:
			if (descriptorType != DESCRIPTOR_TYPE_SAMPLED_IMAGE				&&
				descriptorType != DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER	)
				return false;
			break;

		case READOP_IMAGESAMPLE:
			if (descriptorType != DESCRIPTOR_TYPE_SAMPLED_IMAGE				&&
				descriptorType != DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER	)
				return false;
			break;

		default:
			break;
	}

	return true;
}

const char* getTestTypeName (TestType testType)
{
	switch (testType)
	{
		case TESTTYPE_LOCAL_VARIABLES:
			return "all_local_variables";

		case TESTTYPE_PASS_IMAGE_TO_FUNCTION:
			return "pass_image_to_function";

		case TESTTYPE_PASS_SAMPLER_TO_FUNCTION:
			return "pass_sampler_to_function";

		case TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION:
			return "pass_image_and_sampler_to_function";

		default:
			DE_FATAL("Unknown test type");
			return "";
	}
}

const char* getReadOpName (ReadOp readOp)
{
	switch (readOp)
	{
		case READOP_IMAGEREAD:
			return "imageread";

		case READOP_IMAGEFETCH:
			return "imagefetch";

		case READOP_IMAGESAMPLE:
			return "imagesample";

		default:
			DE_FATAL("Unknown readop");
			return "";
	}
}

const char* getDescriptorName (DescriptorType descType)
{
	switch (descType)
	{
		case DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return "storage_image";

		case DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return "sampled_image";

		case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return "combined_image_sampler";

		default:
			DE_FATAL("Unknown descriptor type");
			return "";
	}
}

VkDescriptorType getVkDescriptorType (DescriptorType descType)
{
	switch (descType)
	{
		case DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		case DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		default:
			DE_FATAL("Unknown descriptor type");
			return VK_DESCRIPTOR_TYPE_LAST;
	}
}

// Get variables that are declared in the read function, ie. not passed as parameters
std::string getFunctionDstVariableStr (ReadOp readOp, DescriptorType descType, TestType testType)
{
	const bool passNdx = (testType == TESTTYPE_LOCAL_VARIABLES);
	const bool passImg = ((testType == TESTTYPE_PASS_IMAGE_TO_FUNCTION)			|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));
	const bool passSmp = ((testType == TESTTYPE_PASS_SAMPLER_TO_FUNCTION)		|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));

	std::string result = "";

	switch (descType)
	{
		case DESCRIPTOR_TYPE_STORAGE_IMAGE:
		{
			switch (readOp)
			{
				case READOP_IMAGEREAD:
					if (passNdx)
						return	"           %func_img = OpLoad %Image %InputData\n";
					break;

				default:
					DE_FATAL("Not possible");
					break;
			}
			break;
		}
		case DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		{
			switch (readOp)
			{
				case READOP_IMAGEFETCH:
					if (passNdx)
						return	"           %func_img = OpLoad %Image %InputData\n";

					if (passSmp && !passImg)
						return	"           %func_tmp = OpLoad %Image %InputData\n"
								"           %func_smi = OpSampledImage %SampledImage %func_tmp %func_smp\n"
								"           %func_img = OpImage %Image %func_smi\n";

					if (passSmp && passImg)
						return	"           %func_smi = OpSampledImage %SampledImage %func_tmp %func_smp\n"
								"           %func_img = OpImage %Image %func_smi\n";
					break;

				case READOP_IMAGESAMPLE:
					if (passNdx)
						return	"           %func_img = OpLoad %Image %InputData\n"
								"           %func_smp = OpLoad %Sampler %SamplerData\n"
								"           %func_smi = OpSampledImage %SampledImage %func_img %func_smp\n";

					if (passImg && !passSmp)
						return	"           %func_smp = OpLoad %Sampler %SamplerData\n"
								"           %func_smi = OpSampledImage %SampledImage %func_img %func_smp\n";

					if (passSmp && !passImg)
						return	"           %func_img = OpLoad %Image %InputData\n"
								"           %func_smi = OpSampledImage %SampledImage %func_img %func_smp\n";

					if (passSmp && passImg)
						return	"           %func_smi = OpSampledImage %SampledImage %func_img %func_smp\n";
					break;

				default:
					DE_FATAL("Not possible");
			}
			break;
		}

		case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		{
			switch (readOp)
			{
				case READOP_IMAGEFETCH:
					if (passNdx)
						return	"           %func_smi = OpLoad %SampledImage %InputData\n"
								"           %func_img = OpImage %Image %func_smi\n";
					break;

				case READOP_IMAGESAMPLE:
					if (passNdx)
						return	"           %func_smi = OpLoad %SampledImage %InputData\n";
					break;

				default:
					DE_FATAL("Not possible");
			}
			break;
		}

		default:
			DE_FATAL("Unknown descriptor type");
	}

	return result;
}

// Get variables that are passed to the read function
std::string getFunctionSrcVariableStr (ReadOp readOp, DescriptorType descType, TestType testType)
{
	const bool passImg = ((testType == TESTTYPE_PASS_IMAGE_TO_FUNCTION)			|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));
	const bool passSmp = ((testType == TESTTYPE_PASS_SAMPLER_TO_FUNCTION)		|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));

	string result = "";

	switch (descType)
	{
		case DESCRIPTOR_TYPE_STORAGE_IMAGE:
		{
			switch (readOp)
			{
				case READOP_IMAGEREAD:
					if (passImg)
						result +=	"           %call_img = OpLoad %Image %InputData\n";
					break;

				default:
					DE_FATAL("Not possible");
			}
			break;
		}
		case DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		{
			switch (readOp)
			{
				case READOP_IMAGEFETCH:
				case READOP_IMAGESAMPLE:
					if (passImg)
						result +=	"           %call_img = OpLoad %Image %InputData\n";

					if (passSmp)
						result +=	"           %call_smp = OpLoad %Sampler %SamplerData\n";
					break;

				default:
					DE_FATAL("Not possible");
			}
			break;
		}
		case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		{
			break;
		}
		default:
			DE_FATAL("Unknown descriptor type");
	}

	return result;
}

// Get parameter types for OpTypeFunction
std::string getFunctionParamTypeStr (TestType testType)
{
	const bool passImg = ((testType == TESTTYPE_PASS_IMAGE_TO_FUNCTION)			|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));
	const bool passSmp = ((testType == TESTTYPE_PASS_SAMPLER_TO_FUNCTION)		|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));

	string result = "";

	if (passImg)
		result += " %Image";

	if (passSmp)
		result += " %Sampler";

	return result;
}

// Get argument names for OpFunctionCall
std::string getFunctionSrcParamStr (TestType testType)
{
	const bool passImg = ((testType == TESTTYPE_PASS_IMAGE_TO_FUNCTION)			|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));
	const bool passSmp = ((testType == TESTTYPE_PASS_SAMPLER_TO_FUNCTION)		|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));

	string result = "";

	if (passImg)
		result += " %call_img";

	if (passSmp)
		result += " %call_smp";

	return result;
}

// Get OpFunctionParameters
std::string getFunctionDstParamStr (ReadOp readOp, TestType testType)
{
	const bool passImg = ((testType == TESTTYPE_PASS_IMAGE_TO_FUNCTION)			|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));
	const bool passSmp = ((testType == TESTTYPE_PASS_SAMPLER_TO_FUNCTION)		|| (testType == TESTTYPE_PASS_IMAGE_AND_SAMPLER_TO_FUNCTION));

	string result = "";

	if (readOp == READOP_IMAGESAMPLE)
	{
		if (passImg)
			result +=	"           %func_img = OpFunctionParameter %Image\n";

		if (passSmp)
			result +=	"           %func_smp = OpFunctionParameter %Sampler\n";
	}
	else
	{
		if (passImg && !passSmp)
			result +=	"           %func_img = OpFunctionParameter %Image\n";

		if (passSmp && !passImg)
			result +=	"           %func_smp = OpFunctionParameter %Sampler\n";

		if (passImg && passSmp)
			result +=	"           %func_tmp = OpFunctionParameter %Image\n"
						"           %func_smp = OpFunctionParameter %Sampler\n";
	}

	return result;
}

// Get read operation
std::string getImageReadOpStr (ReadOp readOp)
{
	switch (readOp)
	{
		case READOP_IMAGEREAD:
			return "OpImageRead %v4f32 %func_img %coord";

		case READOP_IMAGEFETCH:
			return "OpImageFetch %v4f32 %func_img %coord";

		case READOP_IMAGESAMPLE:
			return "OpImageSampleExplicitLod %v4f32 %func_smi %normalcoordf Lod %c_f32_0";

		default:
			DE_FATAL("Unknown readop");
			return "";
	}
}

// Get types and pointers for input images and samplers
std::string getImageSamplerTypeStr (DescriptorType descType)
{
	switch (descType)
	{
		case DESCRIPTOR_TYPE_STORAGE_IMAGE:
			return	"              %Image = OpTypeImage %f32 2D 0 0 0 2 Rgba32f\n"
					"           %ImagePtr = OpTypePointer UniformConstant %Image\n"
					"          %InputData = OpVariable %ImagePtr UniformConstant\n";

		case DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return	"              %Image = OpTypeImage %f32 2D 0 0 0 1 Rgba32f\n"
					"           %ImagePtr = OpTypePointer UniformConstant %Image\n"
					"          %InputData = OpVariable %ImagePtr UniformConstant\n"

					"            %Sampler = OpTypeSampler\n"
					"         %SamplerPtr = OpTypePointer UniformConstant %Sampler\n"
					"        %SamplerData = OpVariable %SamplerPtr UniformConstant\n"
					"       %SampledImage = OpTypeSampledImage %Image\n";

		case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			return	"              %Image = OpTypeImage %f32 2D 0 0 0 1 Rgba32f\n"
					"       %SampledImage = OpTypeSampledImage %Image\n"
					"         %SamplerPtr = OpTypePointer UniformConstant %SampledImage\n"
					"          %InputData = OpVariable %SamplerPtr UniformConstant\n";

		default:
			DE_FATAL("Unknown descriptor type");
			return "";
	}
}

void addComputeImageSamplerTest (tcu::TestCaseGroup* group)
{
	tcu::TestContext& testCtx = group->getTestContext();

	de::Random				rnd					(deStringHash(group->getName()));
	const deUint32			numDataPoints		= 64;
	RGBA					defaultColors[4];
	vector<tcu::Vec4>		inputData;
	vector<tcu::Vec4>		outputData;

	inputData.reserve(numDataPoints);
	outputData.reserve(numDataPoints);

	for (deUint32 numIdx = 0; numIdx < numDataPoints; ++numIdx)
		inputData.push_back(tcu::Vec4(rnd.getFloat(), rnd.getFloat(), rnd.getFloat(), rnd.getFloat()));

	for (deUint32 numIdx = 0; numIdx < numDataPoints; ++numIdx)
		outputData.push_back(inputData.at(numIdx));

	for (deUint32 opNdx = 0u; opNdx < READOP_LAST; opNdx++)
	{
		de::MovePtr<tcu::TestCaseGroup> readOpGroup	(new tcu::TestCaseGroup(testCtx, getReadOpName((ReadOp)opNdx), ""));

		for (deUint32 descNdx = 0u; descNdx < DESCRIPTOR_TYPE_LAST; descNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup> descGroup (new tcu::TestCaseGroup(testCtx, getDescriptorName((DescriptorType)descNdx), ""));

			for (deUint32 testNdx = 0u; testNdx < TESTTYPE_LAST; testNdx++)
			{
				if (!isValidTestCase((TestType)testNdx, (DescriptorType)descNdx, (ReadOp)opNdx))
					continue;

				const std::string	imageReadOp				= getImageReadOpStr((ReadOp)opNdx);

				const std::string	imageSamplerTypes		= getImageSamplerTypeStr((DescriptorType)descNdx);
				const std::string	functionParamTypes		= getFunctionParamTypeStr((TestType)testNdx);

				const std::string	functionSrcVariables	= getFunctionSrcVariableStr((ReadOp)opNdx, (DescriptorType)descNdx, (TestType)testNdx);
				const std::string	functionDstVariables	= getFunctionDstVariableStr((ReadOp)opNdx, (DescriptorType)descNdx, (TestType)testNdx);

				const std::string	functionSrcParams		= getFunctionSrcParamStr(TestType(testNdx));
				const std::string	functionDstParams		= getFunctionDstParamStr((ReadOp)opNdx, TestType(testNdx));

				getDefaultColors(defaultColors);

				ComputeShaderSpec	spec;

				spec.numWorkGroups	= IVec3(numDataPoints, 1, 1);
				spec.inputTypes[0]	= getVkDescriptorType((DescriptorType)descNdx);

				spec.inputs.push_back(BufferSp(new Vec4Buffer(inputData)));

				// Separate sampler for sampled images
				if ((DescriptorType)descNdx == DESCRIPTOR_TYPE_SAMPLED_IMAGE)
				{
					vector<tcu::Vec4> dummyData;
					spec.inputTypes[1] = VK_DESCRIPTOR_TYPE_SAMPLER;
					spec.inputs.push_back(BufferSp(new Vec4Buffer(dummyData)));
				}

				// Shader is expected to pass the input image data to the output buffer
				spec.outputs.push_back(BufferSp(new Vec4Buffer(inputData)));

				const std::string	samplerDecoration		= spec.inputs.size() > 1?
					"                       OpDecorate %SamplerData DescriptorSet 0\n"
					"                       OpDecorate %SamplerData Binding 1\n"
					: "";

				const string		shaderSource	=
					"                       OpCapability Shader\n"
					"                  %1 = OpExtInstImport \"GLSL.std.450\"\n"
					"                       OpMemoryModel Logical GLSL450\n"
					"                       OpEntryPoint GLCompute %main \"main\" %id\n"
					"                       OpExecutionMode %main LocalSize 1 1 1\n"
					"                       OpSource GLSL 430\n"
					"                       OpDecorate %id BuiltIn GlobalInvocationId\n"
					"                       OpDecorate %_arr_v4f_u32_64 ArrayStride 16\n"
					"                       OpMemberDecorate %Output 0 Offset 0\n"
					"                       OpDecorate %Output BufferBlock\n"
					"                       OpDecorate %InputData DescriptorSet 0\n"
					"                       OpDecorate %InputData Binding 0\n"

					+ samplerDecoration +

					"                       OpDecorate %OutputData DescriptorSet 0\n"
					"                       OpDecorate %OutputData Binding " + de::toString(spec.inputs.size()) + "\n"

					"               %void = OpTypeVoid\n"
					"                  %3 = OpTypeFunction %void\n"
					"                %u32 = OpTypeInt 32 0\n"
					"                %i32 = OpTypeInt 32 1\n"
					"                %f32 = OpTypeFloat 32\n"
					" %_ptr_Function_uint = OpTypePointer Function %u32\n"
					"              %v3u32 = OpTypeVector %u32 3\n"
					"   %_ptr_Input_v3u32 = OpTypePointer Input %v3u32\n"
					"                 %id = OpVariable %_ptr_Input_v3u32 Input\n"
					"            %c_f32_0 = OpConstant %f32 0.0\n"
					"            %c_u32_0 = OpConstant %u32 0\n"
					"            %c_i32_0 = OpConstant %i32 0\n"
					"    %_ptr_Input_uint = OpTypePointer Input %u32\n"
					"              %v2u32 = OpTypeVector %u32 2\n"
					"              %v2f32 = OpTypeVector %f32 2\n"
					"              %v4f32 = OpTypeVector %f32 4\n"
					"           %uint_128 = OpConstant %u32 128\n"
					"           %c_u32_64 = OpConstant %u32 64\n"
					"            %c_u32_8 = OpConstant %u32 8\n"
					"            %c_f32_8 = OpConstant %f32 8.0\n"
					"        %c_v2f32_8_8 = OpConstantComposite %v2f32 %c_f32_8 %c_f32_8\n"
					"    %_arr_v4f_u32_64 = OpTypeArray %v4f32 %c_u32_64\n"
					"   %_ptr_Uniform_v4f = OpTypePointer Uniform %v4f32\n"
					"             %Output = OpTypeStruct %_arr_v4f_u32_64\n"
					"%_ptr_Uniform_Output = OpTypePointer Uniform %Output\n"
					"         %OutputData = OpVariable %_ptr_Uniform_Output Uniform\n"

					+ imageSamplerTypes +

					"     %read_func_type = OpTypeFunction %void %u32" + functionParamTypes + "\n"

					"          %read_func = OpFunction %void None %read_func_type\n"
					"           %func_ndx = OpFunctionParameter %u32\n"

					+ functionDstParams +

					"          %funcentry = OpLabel\n"
					"                %row = OpUMod %u32 %func_ndx %c_u32_8\n"
					"                %col = OpUDiv %u32 %func_ndx %c_u32_8\n"
					"              %coord = OpCompositeConstruct %v2u32 %row %col\n"
					"             %coordf = OpConvertUToF %v2f32 %coord\n"
					"       %normalcoordf = OpFDiv %v2f32 %coordf %c_v2f32_8_8\n"

					+ functionDstVariables +

					"              %color = " + imageReadOp + "\n"
					"                 %36 = OpAccessChain %_ptr_Uniform_v4f %OutputData %c_u32_0 %func_ndx\n"
					"                       OpStore %36 %color\n"
					"                       OpReturn\n"
					"                       OpFunctionEnd\n"

					"               %main = OpFunction %void None %3\n"
					"                  %5 = OpLabel\n"
					"                  %i = OpVariable %_ptr_Function_uint Function\n"
					"                 %14 = OpAccessChain %_ptr_Input_uint %id %c_u32_0\n"
					"                 %15 = OpLoad %u32 %14\n"
					"                       OpStore %i %15\n"
					"              %index = OpLoad %u32 %14\n"

					+ functionSrcVariables +

					"                %res = OpFunctionCall %void %read_func %index" + functionSrcParams + "\n"
					"                       OpReturn\n"
					"                       OpFunctionEnd\n";

				spec.assembly			= shaderSource;

				descGroup->addChild(new SpvAsmComputeShaderCase(testCtx, getTestTypeName((TestType)testNdx), "", spec));
			}
			readOpGroup->addChild(descGroup.release());
		}
		group->addChild(readOpGroup.release());
	}
}

void addGraphicsImageSamplerTest (tcu::TestCaseGroup* group)
{
	tcu::TestContext& testCtx = group->getTestContext();

	de::Random				rnd					(deStringHash(group->getName()));
	const deUint32			numDataPoints		= 64;
	RGBA					defaultColors[4];
	vector<tcu::Vec4>		inputData;
	vector<tcu::Vec4>		outputData;

	inputData.reserve(numDataPoints);
	outputData.reserve(numDataPoints);

	for (deUint32 numIdx = 0; numIdx < numDataPoints; ++numIdx)
		inputData.push_back(tcu::Vec4(rnd.getFloat(), rnd.getFloat(), rnd.getFloat(), rnd.getFloat()));

	for (deUint32 numIdx = 0; numIdx < numDataPoints; ++numIdx)
		outputData.push_back(inputData.at(numIdx));

	for (deUint32 opNdx = 0u; opNdx < READOP_LAST; opNdx++)
	{
		de::MovePtr<tcu::TestCaseGroup> readOpGroup	(new tcu::TestCaseGroup(testCtx, getReadOpName((ReadOp)opNdx), ""));

		for (deUint32 descNdx = 0u; descNdx < DESCRIPTOR_TYPE_LAST; descNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup> descGroup (new tcu::TestCaseGroup(testCtx, getDescriptorName((DescriptorType)descNdx), ""));

			for (deUint32 testNdx = 0u; testNdx < TESTTYPE_LAST; testNdx++)
			{
				if (!isValidTestCase((TestType)testNdx, (DescriptorType)descNdx, (ReadOp)opNdx))
					continue;

				const std::string				imageReadOp				= getImageReadOpStr((ReadOp)opNdx);

				const std::string				imageSamplerTypes		= getImageSamplerTypeStr((DescriptorType)descNdx);
				const std::string				functionParamTypes		= getFunctionParamTypeStr((TestType)testNdx);

				const std::string				functionSrcVariables	= getFunctionSrcVariableStr((ReadOp)opNdx, (DescriptorType)descNdx, (TestType)testNdx);
				const std::string				functionDstVariables	= getFunctionDstVariableStr((ReadOp)opNdx, (DescriptorType)descNdx, (TestType)testNdx);

				const std::string				functionSrcParams		= getFunctionSrcParamStr(TestType(testNdx));
				const std::string				functionDstParams		= getFunctionDstParamStr((ReadOp)opNdx, TestType(testNdx));

				de::MovePtr<tcu::TestCaseGroup>	typeGroup				(new tcu::TestCaseGroup(testCtx, getTestTypeName((TestType)testNdx), ""));

				map<string, string>				fragments;
				GraphicsResources				resources;

				resources.inputs.push_back(std::make_pair(getVkDescriptorType((DescriptorType)descNdx),	BufferSp(new Vec4Buffer(inputData))));

				// Separate sampler for sampled images
				if ((DescriptorType)descNdx == DESCRIPTOR_TYPE_SAMPLED_IMAGE)
				{
					vector<tcu::Vec4> dummyData;
					resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_SAMPLER, BufferSp(new Vec4Buffer(dummyData))));
				}

				// Shader is expected to pass the input image data to output buffer
				resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			BufferSp(new Vec4Buffer(outputData))));

				const std::string				samplerDecoration		= resources.inputs.size() > 1?
					"                       OpDecorate %SamplerData DescriptorSet 0\n"
					"                       OpDecorate %SamplerData Binding 1\n"
					: "";

				getDefaultColors(defaultColors);

				fragments["pre_main"]	=
					"           %c_u32_64 = OpConstant %u32 64\n"
					"           %c_i32_64 = OpConstant %i32 64\n"
					"            %c_i32_8 = OpConstant %i32 8\n"
					"        %c_v2f32_8_8 = OpConstantComposite %v2f32 %c_f32_8 %c_f32_8\n"

					"    %_arr_v4f_u32_64 = OpTypeArray %v4f32 %c_u32_64\n"
					"   %_ptr_Uniform_v4f = OpTypePointer Uniform %v4f32\n"

					"             %Output = OpTypeStruct %_arr_v4f_u32_64\n"
					"%_ptr_Uniform_Output = OpTypePointer Uniform %Output\n"
					"         %OutputData = OpVariable %_ptr_Uniform_Output Uniform\n"

					+ imageSamplerTypes +

					"     %read_func_type = OpTypeFunction %void %i32" + functionParamTypes + "\n";

				fragments["decoration"]	=
					"                       OpDecorate %_arr_v4f_u32_64 ArrayStride 16\n"
					"                       OpMemberDecorate %Output 0 Offset 0\n"
					"                       OpDecorate %Output BufferBlock\n"
					"                       OpDecorate %InputData DescriptorSet 0\n"
					"                       OpDecorate %InputData Binding 0\n"

					+ samplerDecoration +

					"OpDecorate %OutputData DescriptorSet 0\n"
					"OpDecorate %OutputData Binding " + de::toString(resources.inputs.size()) + "\n";

				fragments["testfun"]	=
					"          %read_func = OpFunction %void None %read_func_type\n"
					"           %func_ndx = OpFunctionParameter %i32\n"

					+ functionDstParams +

					"          %funcentry = OpLabel\n"

					"                %row = OpSRem %i32 %func_ndx %c_i32_8\n"
					"                %col = OpSDiv %i32 %func_ndx %c_i32_8\n"
					"              %coord = OpCompositeConstruct %v2i32 %row %col\n"
					"             %coordf = OpConvertSToF %v2f32 %coord\n"
					"       %normalcoordf = OpFDiv %v2f32 %coordf %c_v2f32_8_8\n"

					+ functionDstVariables +

					"              %color = " + imageReadOp + "\n"
					"                 %36 = OpAccessChain %_ptr_Uniform_v4f %OutputData %c_i32_0 %func_ndx\n"
					"                       OpStore %36 %color\n"

					"                       OpReturn\n"
					"                       OpFunctionEnd\n"

					"          %test_code = OpFunction %v4f32 None %v4f32_function\n"
					"              %param = OpFunctionParameter %v4f32\n"

					"              %entry = OpLabel\n"

					"                  %i = OpVariable %fp_i32 Function\n"
					"                       OpStore %i %c_i32_0\n"
					"                       OpBranch %loop\n"

					"               %loop = OpLabel\n"
					"                 %15 = OpLoad %i32 %i\n"
					"                 %lt = OpSLessThan %bool %15 %c_i32_64\n"
					"                       OpLoopMerge %merge %inc None\n"
					"                       OpBranchConditional %lt %write %merge\n"

					"              %write = OpLabel\n"
					"              %index = OpLoad %i32 %i\n"

					+ functionSrcVariables +

					"                %res = OpFunctionCall %void %read_func %index" + functionSrcParams + "\n"
					"                       OpBranch %inc\n"

					"                %inc = OpLabel\n"

					"                 %37 = OpLoad %i32 %i\n"
					"                 %39 = OpIAdd %i32 %37 %c_i32_1\n"
					"                       OpStore %i %39\n"
					"                       OpBranch %loop\n"

					"              %merge = OpLabel\n"
					"                       OpReturnValue %param\n"
					"                       OpFunctionEnd\n"

					"";

				createTestsForAllStages("shader", defaultColors, defaultColors, fragments, resources, vector<string>(), typeGroup.get());

				descGroup->addChild(typeGroup.release());
			}
			readOpGroup->addChild(descGroup.release());
		}
		group->addChild(readOpGroup.release());
	}
}
} // anonymous

tcu::TestCaseGroup* createImageSamplerComputeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "image_sampler", "Compute tests for combining images and samplers."));
	addComputeImageSamplerTest(group.get());

	return group.release();
}

tcu::TestCaseGroup* createImageSamplerGraphicsGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "image_sampler", "Graphics tests for combining images and samplers."));
	addGraphicsImageSamplerTest(group.get());

	return group.release();
}

} // SpirVAssembly
} // vkt
