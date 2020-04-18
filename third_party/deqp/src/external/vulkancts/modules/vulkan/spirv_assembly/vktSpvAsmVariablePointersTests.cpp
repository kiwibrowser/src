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
 * \brief SPIR-V Assembly Tests for the SPV_KHR_variable_pointers extension
 *//*--------------------------------------------------------------------*/

#include "tcuFloat.hpp"
#include "tcuRGBA.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"

#include "vkDefs.hpp"
#include "vkDeviceUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkStrUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deMath.h"

#include "vktSpvAsmComputeShaderCase.hpp"
#include "vktSpvAsmComputeShaderTestUtil.hpp"
#include "vktSpvAsmGraphicsShaderTestUtil.hpp"
#include "vktSpvAsmVariablePointersTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include <limits>
#include <map>
#include <string>
#include <sstream>
#include <utility>

namespace vkt
{
namespace SpirVAssembly
{

using namespace vk;
using std::map;
using std::string;
using std::vector;
using tcu::IVec3;
using tcu::IVec4;
using tcu::RGBA;
using tcu::TestLog;
using tcu::TestStatus;
using tcu::Vec4;
using de::UniquePtr;
using tcu::StringTemplate;
using tcu::Vec4;

namespace
{

template<typename T>
void fillRandomScalars (de::Random& rnd, T minValue, T maxValue, void* dst, int numValues, int offset = 0)
{
	T* const typedPtr = (T*)dst;
	for (int ndx = 0; ndx < numValues; ndx++)
		typedPtr[offset + ndx] = randomScalar<T>(rnd, minValue, maxValue);
}

// The following structure (outer_struct) is passed as a vector of 64 32-bit floats into some shaders.
//
// struct struct inner_struct {
//   vec4 x[2];
//   vec4 y[2];
// };
//
// struct outer_struct {
//   inner_struct r[2][2];
// };
//
// This method finds the correct offset from the base of a vector<float32> given the indexes into the structure.
// Returns the index in the inclusive range of 0 and 63. Each unit of the offset represents offset by the size of a 32-bit float.
deUint32 getBaseOffset(	deUint32 indexOuterStruct,
						deUint32 indexMatrixRow,
						deUint32 indexMatrixCol,
						deUint32 indexInnerStruct,
						deUint32 indexVec4Array,
						deUint32 indexVec4)
{
	// index into the outer structure must be zero since the outer structure has only 1 member.
	if(indexOuterStruct != 0)
		DE_ASSERT(indexOuterStruct == 0);

	DE_ASSERT(indexMatrixRow < 2);
	DE_ASSERT(indexMatrixCol < 2);
	DE_ASSERT(indexInnerStruct < 2);
	DE_ASSERT(indexVec4Array < 2);
	DE_ASSERT(indexVec4 < 4);

	deUint32 offset = 0;

	// We have a matrix of 2 rows and 2 columns (total of 4 inner_structs). Each inner_struct contains 16 floats.
	// So, offset by 1 row means offset by 32 floats, and offset by 1 column means offset by 16 floats.
	offset += indexMatrixRow * 32;
	offset += indexMatrixCol * 16;

	// The inner structure contains 2 members, each having 8 floats.
	// So offset by 1 in the inner struct means offset by 8 floats.
	offset += indexInnerStruct * 8;

	// Each member (x|y) have 2 vectors of 4 floats. So, offset by 1 int the vec4 array means an offset by 4 floats.
	offset += indexVec4Array * 4;

	// Each vec4 contains 4 floats, so each offset in the vec4 means offset by 1 float.
	offset += indexVec4;

	return offset;
}

// The following structure (input_buffer) is passed as a vector of 128 32-bit floats into some shaders.
//
// struct struct inner_struct {
//   vec4 x[2];
//   vec4 y[2];
// };
//
// struct outer_struct {
//   inner_struct r[2][2];
// };
//
// struct input_buffer {
//   outer_struct a;
//   outer_struct b;
// }
//
// This method finds the correct offset from the base of a vector<float32> given the indexes into the structure.
// Returns the index in the inclusive range of 0 and 127.
deUint32 getBaseOffsetForSingleInputBuffer(	deUint32 indexOuterStruct,
											deUint32 indexMatrixRow,
											deUint32 indexMatrixCol,
											deUint32 indexInnerStruct,
											deUint32 indexVec4Array,
											deUint32 indexVec4)
{
	DE_ASSERT(indexOuterStruct < 2);
	DE_ASSERT(indexMatrixRow < 2);
	DE_ASSERT(indexMatrixCol < 2);
	DE_ASSERT(indexInnerStruct < 2);
	DE_ASSERT(indexVec4Array < 2);
	DE_ASSERT(indexVec4 < 4);

	// Get the offset assuming you have only one outer_struct. (use index 0 for outer_struct)
	deUint32 offset = getBaseOffset(0, indexMatrixRow, indexMatrixCol, indexInnerStruct, indexVec4Array, indexVec4);

	// If the second outer structure (b) is chosen in the input_buffer, we need to add an offset of 64 since
	// each outer_struct contains 64 floats.
	if (indexOuterStruct == 1)
		offset += 64;

	return offset;
}

void addVariablePointersComputeGroup (tcu::TestCaseGroup* group)
{
	tcu::TestContext&				testCtx					= group->getTestContext();
	de::Random						rnd						(deStringHash(group->getName()));
	const int						seed					= testCtx.getCommandLine().getBaseSeed();
	const int						numMuxes				= 100;
	std::string						inputArraySize			= "200";
	vector<float>					inputAFloats			(2*numMuxes, 0);
	vector<float>					inputBFloats			(2*numMuxes, 0);
	vector<float>					inputSFloats			(numMuxes, 0);
	vector<float>					AmuxAOutputFloats		(numMuxes, 0);
	vector<float>					AmuxBOutputFloats		(numMuxes, 0);
	vector<float>					incrAmuxAOutputFloats	(numMuxes, 0);
	vector<float>					incrAmuxBOutputFloats	(numMuxes, 0);
	VulkanFeatures					requiredFeatures;

	// Each output entry is chosen as follows: ( 0 <= i < numMuxes)
	// 1) For tests with one input buffer:  output[i] = (s[i] < 0) ? A[2*i] : A[2*i+1];
	// 2) For tests with two input buffers: output[i] = (s[i] < 0) ? A[i]   : B[i];

	fillRandomScalars(rnd, -100.f, 100.f, &inputAFloats[0], 2*numMuxes);
	fillRandomScalars(rnd, -100.f, 100.f, &inputBFloats[0], 2*numMuxes);

	// We want to guarantee that the S input has some positive and some negative values.
	// We choose random negative numbers for the first half, random positive numbers for the second half, and then shuffle.
	fillRandomScalars(rnd, -100.f, -1.f , &inputSFloats[0], numMuxes / 2);
	fillRandomScalars(rnd, 1.f   , 100.f, &inputSFloats[numMuxes / 2], numMuxes / 2);
	de::Random(seed).shuffle(inputSFloats.begin(), inputSFloats.end());

	for (size_t i = 0; i < numMuxes; ++i)
	{
		AmuxAOutputFloats[i]     = (inputSFloats[i] < 0) ? inputAFloats[2*i]     : inputAFloats[2*i+1];
		AmuxBOutputFloats[i]	 = (inputSFloats[i] < 0) ? inputAFloats[i]		 : inputBFloats[i];
		incrAmuxAOutputFloats[i] = (inputSFloats[i] < 0) ? 1 + inputAFloats[2*i] : 1 + inputAFloats[2*i+1];
		incrAmuxBOutputFloats[i] = (inputSFloats[i] < 0) ? 1 + inputAFloats[i]	 : 1 + inputBFloats[i];
	}

	const StringTemplate shaderTemplate (
		"OpCapability Shader\n"

		"${ExtraCapability}\n"

		"OpExtension \"SPV_KHR_variable_pointers\"\n"
		"OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		// Decorations
		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %indata_a DescriptorSet 0\n"
		"OpDecorate %indata_a Binding 0\n"
		"OpDecorate %indata_b DescriptorSet 0\n"
		"OpDecorate %indata_b Binding 1\n"
		"OpDecorate %indata_s DescriptorSet 0\n"
		"OpDecorate %indata_s Binding 2\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 3\n"
		"OpDecorate %f32arr ArrayStride 4\n"
		"OpDecorate %sb_f32ptr ArrayStride 4\n"
		"OpDecorate %buf Block\n"
		"OpMemberDecorate %buf 0 Offset 0\n"

		+ string(getComputeAsmCommonTypes()) +

		"%sb_f32ptr				= OpTypePointer StorageBuffer %f32\n"
		"%buf					= OpTypeStruct %f32arr\n"
		"%bufptr				= OpTypePointer StorageBuffer %buf\n"
		"%indata_a				= OpVariable %bufptr StorageBuffer\n"
		"%indata_b				= OpVariable %bufptr StorageBuffer\n"
		"%indata_s				= OpVariable %bufptr StorageBuffer\n"
		"%outdata				= OpVariable %bufptr StorageBuffer\n"
		"%id					= OpVariable %uvec3ptr Input\n"
		"%zero				    = OpConstant %i32 0\n"
		"%one					= OpConstant %i32 1\n"
		"%fzero					= OpConstant %f32 0\n"
		"%fone					= OpConstant %f32 1\n"

		"${ExtraTypes}"

		"${ExtraGlobalScopeVars}"

		// We're going to put the "selector" function here.
		// This function type is needed tests that use OpFunctionCall.
		"%selector_func_type	= OpTypeFunction %sb_f32ptr %bool %sb_f32ptr %sb_f32ptr\n"
		"%choose_input_func		= OpFunction %sb_f32ptr None %selector_func_type\n"
		"%is_neg_param			= OpFunctionParameter %bool\n"
		"%first_ptr_param		= OpFunctionParameter %sb_f32ptr\n"
		"%second_ptr_param		= OpFunctionParameter %sb_f32ptr\n"
		"%selector_func_begin	= OpLabel\n"
		"%result_ptr			= OpSelect %sb_f32ptr %is_neg_param %first_ptr_param %second_ptr_param\n"
		"OpReturnValue %result_ptr\n"
		"OpFunctionEnd\n"

		// main function is the entry_point
		"%main					= OpFunction %void None %voidf\n"
		"%label					= OpLabel\n"

		"${ExtraFunctionScopeVars}"

		"%idval					= OpLoad %uvec3 %id\n"
		"%i						= OpCompositeExtract %u32 %idval 0\n"
		"%two_i					= OpIAdd %u32 %i %i\n"
		"%two_i_plus_1			= OpIAdd %u32 %two_i %one\n"
		"%inloc_a_i				= OpAccessChain %sb_f32ptr %indata_a %zero %i\n"
		"%inloc_b_i				= OpAccessChain %sb_f32ptr %indata_b %zero %i\n"
		"%inloc_s_i             = OpAccessChain %sb_f32ptr %indata_s %zero %i\n"
		"%outloc_i              = OpAccessChain %sb_f32ptr %outdata  %zero %i\n"
		"%inloc_a_2i			= OpAccessChain %sb_f32ptr %indata_a %zero %two_i\n"
		"%inloc_a_2i_plus_1		= OpAccessChain %sb_f32ptr %indata_a %zero %two_i_plus_1\n"
		"%inval_s_i				= OpLoad %f32 %inloc_s_i\n"
		"%is_neg				= OpFOrdLessThan %bool %inval_s_i %fzero\n"

		"${ExtraSetupComputations}"

		"${ResultStrategy}"

		"%mux_output			= OpLoad %f32 ${VarPtrName}\n"
		"						  OpStore %outloc_i %mux_output\n"
		"						  OpReturn\n"
		"						  OpFunctionEnd\n");

	const bool singleInputBuffer[]  = { true, false };
	for (int inputBufferTypeIndex = 0 ; inputBufferTypeIndex < 2; ++inputBufferTypeIndex)
	{
		const bool isSingleInputBuffer			= singleInputBuffer[inputBufferTypeIndex];
		const string extraCap					= isSingleInputBuffer	? "OpCapability VariablePointersStorageBuffer\n" : "OpCapability VariablePointers\n";
		const vector<float>& expectedOutput		= isSingleInputBuffer	? AmuxAOutputFloats		 : AmuxBOutputFloats;
		const vector<float>& expectedIncrOutput	= isSingleInputBuffer	? incrAmuxAOutputFloats	 : incrAmuxBOutputFloats;
		const string bufferType					= isSingleInputBuffer	? "single_buffer"	 : "two_buffers";
		const string muxInput1					= isSingleInputBuffer	? " %inloc_a_2i "		 : " %inloc_a_i ";
		const string muxInput2					= isSingleInputBuffer	? " %inloc_a_2i_plus_1 " : " %inloc_b_i ";

		// Set the proper extension features required for the test
		if (isSingleInputBuffer)
			requiredFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS_STORAGEBUFFER;
		else
			requiredFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS;

		{ // Variable Pointer Reads (using OpSelect)
			ComputeShaderSpec				spec;
			map<string, string>				specs;
			string name						= "reads_opselect_" + bufferType;
			specs["ExtraCapability"]		= extraCap;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraSetupComputations"]	= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			= "%mux_output_var_ptr	= OpSelect %sb_f32ptr %is_neg" + muxInput1 + muxInput2 + "\n";
			spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.assembly					= shaderTemplate.specialize(specs);
			spec.numWorkGroups				= IVec3(numMuxes, 1, 1);
			spec.requestedVulkanFeatures	= requiredFeatures;
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputAFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputBFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputSFloats)));
			spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
			spec.extensions.push_back("VK_KHR_variable_pointers");
			group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
		}
		{ // Variable Pointer Reads (using OpFunctionCall)
			ComputeShaderSpec				spec;
			map<string, string>				specs;
			string name						= "reads_opfunctioncall_" + bufferType;
			specs["ExtraCapability"]		= extraCap;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraSetupComputations"]	= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			= "%mux_output_var_ptr = OpFunctionCall %sb_f32ptr %choose_input_func %is_neg" + muxInput1 + muxInput2 + "\n";
			spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.assembly					= shaderTemplate.specialize(specs);
			spec.numWorkGroups				= IVec3(numMuxes, 1, 1);
			spec.requestedVulkanFeatures	= requiredFeatures;
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputAFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputBFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputSFloats)));
			spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
			spec.extensions.push_back("VK_KHR_variable_pointers");
			group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
		}
		{ // Variable Pointer Reads (using OpPhi)
			ComputeShaderSpec				spec;
			map<string, string>				specs;
			string name						= "reads_opphi_" + bufferType;
			specs["ExtraCapability"]		= extraCap;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraSetupComputations"]	= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			=
				"							  OpSelectionMerge %end_label None\n"
				"							  OpBranchConditional %is_neg %take_mux_input_1 %take_mux_input_2\n"
				"%take_mux_input_1			= OpLabel\n"
				"							  OpBranch %end_label\n"
				"%take_mux_input_2			= OpLabel\n"
				"						      OpBranch %end_label\n"
				"%end_label					= OpLabel\n"
				"%mux_output_var_ptr		= OpPhi %sb_f32ptr" + muxInput1 + "%take_mux_input_1" + muxInput2 + "%take_mux_input_2\n";
			spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.assembly					= shaderTemplate.specialize(specs);
			spec.numWorkGroups				= IVec3(numMuxes, 1, 1);
			spec.requestedVulkanFeatures	= requiredFeatures;
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputAFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputBFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputSFloats)));
			spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
			spec.extensions.push_back("VK_KHR_variable_pointers");
			group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
		}
		{ // Variable Pointer Reads (using OpCopyObject)
			ComputeShaderSpec				spec;
			map<string, string>				specs;
			string name						= "reads_opcopyobject_" + bufferType;
			specs["ExtraCapability"]		= extraCap;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraSetupComputations"]	= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			=
				"%mux_input_1_copy			= OpCopyObject %sb_f32ptr" + muxInput1 + "\n"
				"%mux_input_2_copy			= OpCopyObject %sb_f32ptr" + muxInput2 + "\n"
				"%mux_output_var_ptr		= OpSelect %sb_f32ptr %is_neg %mux_input_1_copy %mux_input_2_copy\n";
			spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.assembly					= shaderTemplate.specialize(specs);
			spec.numWorkGroups				= IVec3(numMuxes, 1, 1);
			spec.requestedVulkanFeatures	= requiredFeatures;
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputAFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputBFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputSFloats)));
			spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
			spec.extensions.push_back("VK_KHR_variable_pointers");
			group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
		}
		{ // Test storing into Private variables.
			const char* storageClasses[]		= {"Private", "Function"};
			for (int classId = 0; classId < 2; ++classId)
			{
				ComputeShaderSpec				spec;
				map<string, string>				specs;
				std::string storageClass		= storageClasses[classId];
				std::string name				= "stores_" + string(de::toLower(storageClass)) + "_" + bufferType;
				std::string description			= "Test storing variable pointer into " + storageClass + " variable.";
				std::string extraVariable		= "%mux_output_copy	= OpVariable %sb_f32ptrptr " + storageClass + "\n";
				specs["ExtraTypes"]				= "%sb_f32ptrptr = OpTypePointer " + storageClass + " %sb_f32ptr\n";
				specs["ExtraCapability"]		= extraCap;
				specs["ExtraGlobalScopeVars"]	= (classId == 0) ? extraVariable : "";
				specs["ExtraFunctionScopeVars"]	= (classId == 1) ? extraVariable : "";
				specs["ExtraSetupComputations"]	= "";
				specs["VarPtrName"]				= "%mux_output_var_ptr";
				specs["ResultStrategy"]			=
					"%opselect_result			= OpSelect %sb_f32ptr %is_neg" + muxInput1 + muxInput2 + "\n"
					"							  OpStore %mux_output_copy %opselect_result\n"
					"%mux_output_var_ptr		= OpLoad %sb_f32ptr %mux_output_copy\n";
				spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				spec.assembly					= shaderTemplate.specialize(specs);
				spec.numWorkGroups				= IVec3(numMuxes, 1, 1);
				spec.requestedVulkanFeatures	= requiredFeatures;
				spec.inputs.push_back(BufferSp(new Float32Buffer(inputAFloats)));
				spec.inputs.push_back(BufferSp(new Float32Buffer(inputBFloats)));
				spec.inputs.push_back(BufferSp(new Float32Buffer(inputSFloats)));
				spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
				spec.extensions.push_back("VK_KHR_variable_pointers");
				group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), description.c_str(), spec));
			}
		}
		{ // Variable Pointer Reads (Using OpPtrAccessChain)
			ComputeShaderSpec				spec;
			map<string, string>				specs;
			std::string name				= "reads_opptraccesschain_" + bufferType;
			std::string in_1				= isSingleInputBuffer ? " %a_2i_ptr "		 : " %a_i_ptr ";
			std::string in_2				= isSingleInputBuffer ? " %a_2i_plus_1_ptr " : " %b_i_ptr ";
			specs["ExtraTypes"]				= "";
			specs["ExtraCapability"]		= extraCap;
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraSetupComputations"]	= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			=
					"%a_ptr					= OpAccessChain %sb_f32ptr %indata_a %zero %zero\n"
					"%b_ptr					= OpAccessChain %sb_f32ptr %indata_b %zero %zero\n"
					"%s_ptr					= OpAccessChain %sb_f32ptr %indata_s %zero %zero\n"
					"%out_ptr               = OpAccessChain %sb_f32ptr %outdata  %zero %zero\n"
					"%a_i_ptr               = OpPtrAccessChain %sb_f32ptr %a_ptr %i\n"
					"%b_i_ptr               = OpPtrAccessChain %sb_f32ptr %b_ptr %i\n"
					"%s_i_ptr               = OpPtrAccessChain %sb_f32ptr %s_ptr %i\n"
					"%a_2i_ptr              = OpPtrAccessChain %sb_f32ptr %a_ptr %two_i\n"
					"%a_2i_plus_1_ptr       = OpPtrAccessChain %sb_f32ptr %a_ptr %two_i_plus_1\n"
					"%mux_output_var_ptr    = OpSelect %sb_f32ptr %is_neg " + in_1 + in_2 + "\n";
			spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.assembly					= shaderTemplate.specialize(specs);
			spec.numWorkGroups				= IVec3(numMuxes, 1, 1);
			spec.requestedVulkanFeatures	= requiredFeatures;
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputAFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputBFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputSFloats)));
			spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
			spec.extensions.push_back("VK_KHR_variable_pointers");
			group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
		}
		{   // Variable Pointer Writes
			ComputeShaderSpec				spec;
			map<string, string>				specs;
			std::string	name				= "writes_" + bufferType;
			specs["ExtraCapability"]		= extraCap;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraSetupComputations"]	= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			= "%mux_output_var_ptr = OpSelect %sb_f32ptr %is_neg" + muxInput1 + muxInput2 + "\n" +
											  "               %val = OpLoad %f32 %mux_output_var_ptr\n"
											  "        %val_plus_1 = OpFAdd %f32 %val %fone\n"
											  "						 OpStore %mux_output_var_ptr %val_plus_1\n";
			spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.assembly					= shaderTemplate.specialize(specs);
			spec.numWorkGroups				= IVec3(numMuxes, 1, 1);
			spec.requestedVulkanFeatures	= requiredFeatures;
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputAFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputBFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputSFloats)));
			spec.outputs.push_back(BufferSp(new Float32Buffer(expectedIncrOutput)));
			spec.extensions.push_back("VK_KHR_variable_pointers");
			group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
		}

		// If we only have VariablePointersStorageBuffer, then the extension does not apply to Workgroup storage class.
		// Therefore the Workgroup tests apply to cases where the VariablePointers capability is used (when 2 input buffers are used).
		if (!isSingleInputBuffer)
		{
			// VariablePointers on Workgroup
			ComputeShaderSpec				spec;
			map<string, string>				specs;
			std::string name				= "workgroup_" + bufferType;
			specs["ExtraCapability"]		= extraCap;
			specs["ExtraTypes"]				=
					"%c_i32_N				= OpConstant %i32 " + inputArraySize + " \n"
					"%f32arr_N				= OpTypeArray %f32 %c_i32_N\n"
					"%f32arr_wrkgrp_ptr		= OpTypePointer Workgroup %f32arr_N\n"
					"%f32_wrkgrp_ptr		= OpTypePointer Workgroup %f32\n";
			specs["ExtraGlobalScopeVars"]	=
					"%AW					= OpVariable %f32arr_wrkgrp_ptr Workgroup\n"
					"%BW					= OpVariable %f32arr_wrkgrp_ptr Workgroup\n";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraSetupComputations"]	=
					"%loc_AW_i				= OpAccessChain %f32_wrkgrp_ptr %AW %i\n"
					"%loc_BW_i				= OpAccessChain %f32_wrkgrp_ptr %BW %i\n"
					"%inval_a_i				= OpLoad %f32 %inloc_a_i\n"
					"%inval_b_i				= OpLoad %f32 %inloc_b_i\n"
					"%inval_a_2i			= OpLoad %f32 %inloc_a_2i\n"
					"%inval_a_2i_plus_1		= OpLoad %f32 %inloc_a_2i_plus_1\n";
			specs["VarPtrName"]				= "%output_var_ptr";
			specs["ResultStrategy"]			=
					"						  OpStore %loc_AW_i %inval_a_i\n"
					"						  OpStore %loc_BW_i %inval_b_i\n"
					"%output_var_ptr		= OpSelect %f32_wrkgrp_ptr %is_neg %loc_AW_i %loc_BW_i\n";
			spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			spec.assembly					= shaderTemplate.specialize(specs);
			spec.numWorkGroups				= IVec3(numMuxes, 1, 1);
			spec.requestedVulkanFeatures	= requiredFeatures;
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputAFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputBFloats)));
			spec.inputs.push_back(BufferSp(new Float32Buffer(inputSFloats)));
			spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
			spec.extensions.push_back("VK_KHR_variable_pointers");
			group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
		}
	}
}

void addComplexTypesVariablePointersComputeGroup (tcu::TestCaseGroup* group)
{
	tcu::TestContext&				testCtx					= group->getTestContext();
	const int						numFloats				= 64;
	vector<float>					inputA					(numFloats, 0);
	vector<float>					inputB					(numFloats, 0);
	vector<float>					inputC					(2*numFloats, 0);
	vector<float>					expectedOutput			(1, 0);
	VulkanFeatures					requiredFeatures;

	// These tests exercise variable pointers into various levels of the following data-structures.
	//
	// struct struct inner_struct {
	//   vec4 x[2]; // array of 2 vectors. Each vector is 4 floats.
	//   vec4 y[2]; // array of 2 vectors. Each vector is 4 floats.
	// };
	//
	// struct outer_struct {
	//   inner_struct r[2][2];
	// };
	//
	// struct input_buffer {
	//   outer_struct a;
	//   outer_struct b;
	// }
	//
	// inputA is of type outer_struct.
	// inputB is of type outer_struct.
	// inputC is of type input_buffer.
	//
	// inputA and inputB are of the same size. When testing variable pointers pointing to
	// two different input buffers, we use inputA and inputB.
	//
	// inputC is twice the size of inputA. When testing the VariablePointersStorageBuffer capability,
	// the variable pointer must be confined to a single buffer. These tests will use inputC.
	//
	// The inner_struct contains 16 floats.
	// The outer_struct contains 64 floats.
	// The input_buffer contains 128 floats.
	// Populate the first input (inputA) to contain:  {0, 4, ... , 252}
	// Populate the second input (inputB) to contain: {3, 7, ... , 255}
	// Populate the third input (inputC) to contain:  {0, 4, ... , 252, 3, 7, ... , 255}
	// Note that the first half of inputC is the same as inputA and the second half is the same as inputB.
	for (size_t i = 0; i < numFloats; ++i)
	{
		inputA[i] = 4*float(i) / 255;
		inputB[i] = ((4*float(i)) + 3) / 255;
		inputC[i] = inputA[i];
		inputC[i + numFloats] = inputB[i];
	}

	// In the following tests we use variable pointers to point to different types:
	// nested structures, matrices of structures, arrays of structures, arrays of vectors, vectors of scalars, and scalars.
	// outer_structure.inner_structure[?][?].x[?][?];
	//   ^                    ^        ^  ^  ^ ^  ^
	//
	// For tests with 2 input buffers:
	// 1. inputA						or	inputB							= nested structure
	// 2. inputA.r						or	inputB.r						= matrices of structures
	// 3. inputA.r[?]					or	inputB.r[?]						= arrays of structures
	// 4. inputA.r[?][?]				or	inputB.r[?][?]					= structures
	// 5. inputA.r[?][?].(x|y)			or	inputB.r[?][?].(x|y)			= arrays of vectors
	// 6. inputA.r[?][?].(x|y)[?]		or	inputB.r[?][?].(x|y)[?]			= vectors of scalars
	// 7. inputA.r[?][?].(x|y)[?][?]	or	inputB.r[?][?].(x|y)[?][?]		= scalars
	// For tests with 1 input buffer:
	// 1. inputC.a						or	inputC.b						= nested structure
	// 2. inputC.a.r					or	inputC.b.r						= matrices of structures
	// 3. inputC.a.r[?]					or	inputC.b.r[?]					= arrays of structures
	// 4. inputC.a.r[?][?]				or	inputC.b.r[?][?]				= structures
	// 5. inputC.a.r[?][?].(x|y)		or	inputC.b.r[?][?].(x|y)			= arrays of vectors
	// 6. inputC.a.r[?][?].(x|y)[?]		or	inputC.b.r[?][?].(x|y)[?]		= vectors of scalars
	// 7. inputC.a.r[?][?].(x|y)[?][?]	or	inputC.b.r[?][?].(x|y)[?][?]	= scalars
	const int numLevels = 7;

	const string decorations (
		// Decorations
		"OpDecorate %id BuiltIn GlobalInvocationId		\n"
		"OpDecorate %inputA DescriptorSet 0				\n"
		"OpDecorate %inputB DescriptorSet 0				\n"
		"OpDecorate %inputC DescriptorSet 0				\n"
		"OpDecorate %outdata DescriptorSet 0			\n"
		"OpDecorate %inputA Binding 0					\n"
		"OpDecorate %inputB Binding 1					\n"
		"OpDecorate %inputC Binding 2					\n"
		"OpDecorate %outdata Binding 3					\n"

		// Set the Block decoration
		"OpDecorate %outer_struct	Block				\n"
		"OpDecorate %input_buffer	Block				\n"
		"OpDecorate %output_buffer	Block				\n"

		// Set the Offsets
		"OpMemberDecorate %output_buffer 0 Offset 0		\n"
		"OpMemberDecorate %input_buffer  0 Offset 0		\n"
		"OpMemberDecorate %input_buffer  1 Offset 256	\n"
		"OpMemberDecorate %outer_struct  0 Offset 0		\n"
		"OpMemberDecorate %inner_struct  0 Offset 0		\n"
		"OpMemberDecorate %inner_struct  1 Offset 32	\n"

		// Set the ArrayStrides
		"OpDecorate %arr2_v4float        ArrayStride 16		\n"
		"OpDecorate %arr2_inner_struct   ArrayStride 64		\n"
		"OpDecorate %mat2x2_inner_struct ArrayStride 128	\n"
		"OpDecorate %outer_struct_ptr    ArrayStride 256	\n"
		"OpDecorate %v4f32_ptr           ArrayStride 16		\n"
	);

	const string types (
		///////////////
		// CONSTANTS //
		///////////////
		"%c_bool_true			= OpConstantTrue	%bool							\n"
		"%c_bool_false			= OpConstantFalse	%bool							\n"
		"%c_i32_0				= OpConstant		%i32		0					\n"
		"%c_i32_1				= OpConstant		%i32		1					\n"
		"%c_i32_2				= OpConstant		%i32		2					\n"
		"%c_i32_3				= OpConstant		%i32		3					\n"
		"%c_u32_2				= OpConstant		%u32		2					\n"

		///////////
		// TYPES //
		///////////
		"%v4f32                 = OpTypeVector %f32 4                               \n"

		// struct struct inner_struct {
		//   vec4 x[2]; // array of 2 vectors
		//   vec4 y[2]; // array of 2 vectors
		// };
		"%arr2_v4float			= OpTypeArray %v4f32 %c_u32_2						\n"
		"%inner_struct			= OpTypeStruct %arr2_v4float %arr2_v4float			\n"

		// struct outer_struct {
		//   inner_struct r[2][2];
		// };
		"%arr2_inner_struct		= OpTypeArray %inner_struct %c_u32_2				\n"
		"%mat2x2_inner_struct	= OpTypeArray %arr2_inner_struct %c_u32_2			\n"
		"%outer_struct			= OpTypeStruct %mat2x2_inner_struct					\n"

		// struct input_buffer {
		//   outer_struct a;
		//   outer_struct b;
		// }
		"%input_buffer			= OpTypeStruct %outer_struct %outer_struct			\n"

		// struct output_struct {
		//   float out;
		// }
		"%output_buffer			= OpTypeStruct %f32									\n"

		///////////////////
		// POINTER TYPES //
		///////////////////
		"%output_buffer_ptr		= OpTypePointer StorageBuffer %output_buffer		\n"
		"%input_buffer_ptr		= OpTypePointer StorageBuffer %input_buffer			\n"
		"%outer_struct_ptr		= OpTypePointer StorageBuffer %outer_struct			\n"
		"%mat2x2_ptr			= OpTypePointer StorageBuffer %mat2x2_inner_struct	\n"
		"%arr2_ptr				= OpTypePointer StorageBuffer %arr2_inner_struct	\n"
		"%inner_struct_ptr		= OpTypePointer StorageBuffer %inner_struct			\n"
		"%arr_v4f32_ptr			= OpTypePointer StorageBuffer %arr2_v4float			\n"
		"%v4f32_ptr				= OpTypePointer StorageBuffer %v4f32				\n"
		"%sb_f32ptr				= OpTypePointer StorageBuffer %f32					\n"

		///////////////
		// VARIABLES //
		///////////////
		"%id					= OpVariable %uvec3ptr			Input				\n"
		"%inputA				= OpVariable %outer_struct_ptr	StorageBuffer		\n"
		"%inputB				= OpVariable %outer_struct_ptr	StorageBuffer		\n"
		"%inputC				= OpVariable %input_buffer_ptr	StorageBuffer		\n"
		"%outdata				= OpVariable %output_buffer_ptr	StorageBuffer		\n"
	);

	const StringTemplate shaderTemplate (
		"OpCapability Shader\n"

		"${extra_capability}\n"

		"OpExtension \"SPV_KHR_variable_pointers\"\n"
		"OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		+ decorations

		+ string(getComputeAsmCommonTypes())

		+ types +

		// These selector functions return variable pointers.
		// These functions are used by tests that use OpFunctionCall to obtain the variable pointer
		"%selector_func_type	= OpTypeFunction ${selected_type} %bool ${selected_type} ${selected_type}\n"
		"%choose_input_func		= OpFunction ${selected_type} None %selector_func_type\n"
		"%choose_first_param	= OpFunctionParameter %bool\n"
		"%first_param			= OpFunctionParameter ${selected_type}\n"
		"%second_param			= OpFunctionParameter ${selected_type}\n"
		"%selector_func_begin	= OpLabel\n"
		"%result_ptr			= OpSelect ${selected_type} %choose_first_param %first_param %second_param\n"
		"OpReturnValue %result_ptr\n"
		"OpFunctionEnd\n"

		// main function is the entry_point
		"%main					= OpFunction %void None %voidf\n"
		"%label					= OpLabel\n"

		// Here are the 2 nested structures within InputC.
		"%inputC_a				= OpAccessChain %outer_struct_ptr %inputC %c_i32_0\n"
		"%inputC_b				= OpAccessChain %outer_struct_ptr %inputC %c_i32_1\n"

		// Define the 2 pointers from which we're going to choose one.
		"${a_loc} \n"
		"${b_loc} \n"

		// Choose between the 2 pointers / variable pointers.
		"${selection_strategy} \n"

		// OpAccessChain into the variable pointer until you get to the float.
		"%result_loc			= OpAccessChain %sb_f32ptr %var_ptr  ${remaining_indexes} \n"

		// Now load from the result_loc
		"%result_val			= OpLoad %f32 %result_loc \n"

		// Store the chosen value to the output buffer.
		"%outdata_loc			= OpAccessChain %sb_f32ptr %outdata %c_i32_0\n"
		"						  OpStore %outdata_loc %result_val\n"
		"						  OpReturn\n"
		"						  OpFunctionEnd\n");

	for (int isSingleInputBuffer = 0 ; isSingleInputBuffer < 2; ++isSingleInputBuffer)
	{
		// Set the proper extension features required for the test
		if (isSingleInputBuffer)
			requiredFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS_STORAGEBUFFER;
		else
			requiredFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS;

		for (int selectInputA = 0; selectInputA < 2; ++selectInputA)
		{
			const string extraCap					= isSingleInputBuffer	? "OpCapability VariablePointersStorageBuffer\n" : "OpCapability VariablePointers\n";
			const vector<float>& selectedInput		= isSingleInputBuffer	? inputC										 : (selectInputA ? inputA : inputB);
			const string bufferType					= isSingleInputBuffer	? "single_buffer_"								 : "two_buffers_";
			const string baseA						= isSingleInputBuffer	? "%inputC_a"									 : "%inputA";
			const string baseB						= isSingleInputBuffer	? "%inputC_b"									 : "%inputB";
			const string selectedInputStr			= selectInputA			? "first_input"									 : "second_input";
			const string spirvSelectInputA			= selectInputA			? "%c_bool_true"								 : "%c_bool_false";
			const int outerStructIndex				= isSingleInputBuffer	? (selectInputA ? 0 : 1)						 : 0;

			// The indexes chosen at each level. At any level, any given offset is exercised.
			// outerStructIndex is 0 for inputA and inputB (because outer_struct has only 1 member).
			// outerStructIndex is 0 for member <a> of inputC and 1 for member <b>.
			const int indexesForLevel[numLevels][6]= {{outerStructIndex, 0, 0, 0, 0, 1},
													  {outerStructIndex, 1, 0, 1, 0, 2},
													  {outerStructIndex, 0, 1, 0, 1, 3},
													  {outerStructIndex, 1, 1, 1, 0, 0},
													  {outerStructIndex, 0, 0, 1, 1, 1},
													  {outerStructIndex, 1, 0, 0, 0, 2},
													  {outerStructIndex, 1, 1, 1, 1, 3}};

			const string indexLevelNames[]		= {"_outer_struct_", "_matrices_", "_arrays_", "_inner_structs_", "_vec4arr_", "_vec4_", "_float_"};
			const string inputALocations[]		= {	"",
												"%a_loc = OpAccessChain %mat2x2_ptr       " + baseA + " %c_i32_0",
												"%a_loc = OpAccessChain %arr2_ptr         " + baseA + " %c_i32_0 %c_i32_0",
												"%a_loc = OpAccessChain %inner_struct_ptr " + baseA + " %c_i32_0 %c_i32_1 %c_i32_1",
												"%a_loc = OpAccessChain %arr_v4f32_ptr    " + baseA + " %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
												"%a_loc = OpAccessChain %v4f32_ptr        " + baseA + " %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
												"%a_loc = OpAccessChain %sb_f32ptr        " + baseA + " %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3"};

			const string inputBLocations[]		= {	"",
												"%b_loc = OpAccessChain %mat2x2_ptr       " + baseB + " %c_i32_0",
												"%b_loc = OpAccessChain %arr2_ptr         " + baseB + " %c_i32_0 %c_i32_0",
												"%b_loc = OpAccessChain %inner_struct_ptr " + baseB + " %c_i32_0 %c_i32_1 %c_i32_1",
												"%b_loc = OpAccessChain %arr_v4f32_ptr    " + baseB + " %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
												"%b_loc = OpAccessChain %v4f32_ptr        " + baseB + " %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
												"%b_loc = OpAccessChain %sb_f32ptr        " + baseB + " %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3"};

			const string inputAPtrAccessChain[]	= {	"",
												"%a_loc = OpPtrAccessChain %mat2x2_ptr       " + baseA + " %c_i32_0 %c_i32_0",
												"%a_loc = OpPtrAccessChain %arr2_ptr         " + baseA + " %c_i32_0 %c_i32_0 %c_i32_0",
												"%a_loc = OpPtrAccessChain %inner_struct_ptr " + baseA + " %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1",
												"%a_loc = OpPtrAccessChain %arr_v4f32_ptr    " + baseA + " %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
												"%a_loc = OpPtrAccessChain %v4f32_ptr        " + baseA + " %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
												// Next case emulates:
												// %a_loc = OpPtrAccessChain %sb_f32ptr          baseA     %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3
												// But rewrite it to exercise OpPtrAccessChain with a non-zero first index:
												//    %a_loc_arr is a pointer to an array that we want to index with 1.
												// But instead of just using OpAccessChain with first index 1, use OpAccessChain with index 0 to
												// get a pointer to the first element, then send that into OpPtrAccessChain with index 1.
												"%a_loc_arr = OpPtrAccessChain %arr_v4f32_ptr " + baseA + " %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 "
												"%a_loc_first_elem = OpAccessChain %v4f32_ptr %a_loc_arr %c_i32_0 "
												"%a_loc = OpPtrAccessChain %sb_f32ptr %a_loc_first_elem %c_i32_1 %c_i32_3"};

			const string inputBPtrAccessChain[]	= {	"",
												"%b_loc = OpPtrAccessChain %mat2x2_ptr       " + baseB + " %c_i32_0 %c_i32_0",
												"%b_loc = OpPtrAccessChain %arr2_ptr         " + baseB + " %c_i32_0 %c_i32_0 %c_i32_0",
												"%b_loc = OpPtrAccessChain %inner_struct_ptr " + baseB + " %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1",
												"%b_loc = OpPtrAccessChain %arr_v4f32_ptr    " + baseB + " %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
												"%b_loc = OpPtrAccessChain %v4f32_ptr        " + baseB + " %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
												// Next case emulates:
												// %b_loc = OpPtrAccessChain %sb_f32ptr          basseB     %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3
												// But rewrite it to exercise OpPtrAccessChain with a non-zero first index:
												//    %b_loc_arr is a pointer to an array that we want to index with 1.
												// But instead of just using OpAccessChain with first index 1, use OpAccessChain with index 0 to
												// get a pointer to the first element, then send that into OpPtrAccessChain with index 1.
												"%b_loc_arr = OpPtrAccessChain %arr_v4f32_ptr " + baseB + " %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 "
												"%b_loc_first_elem = OpAccessChain %v4f32_ptr %b_loc_arr %c_i32_0 "
												"%b_loc = OpPtrAccessChain %sb_f32ptr %b_loc_first_elem %c_i32_1 %c_i32_3"};


			const string remainingIndexesAtLevel[]= {"%c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
													 "%c_i32_1 %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_2",
													 "%c_i32_1 %c_i32_0 %c_i32_1 %c_i32_3",
													 "%c_i32_1 %c_i32_0 %c_i32_0",
													 "%c_i32_1 %c_i32_1",
													 "%c_i32_2",
													 ""};

			const string pointerTypeAtLevel[] = {"%outer_struct_ptr", "%mat2x2_ptr", "%arr2_ptr", "%inner_struct_ptr", "%arr_v4f32_ptr", "%v4f32_ptr", "%sb_f32ptr"};
			const string baseANameAtLevel[]	  = {baseA, "%a_loc", "%a_loc", "%a_loc", "%a_loc", "%a_loc", "%a_loc"};
			const string baseBNameAtLevel[]	  = {baseB, "%b_loc", "%b_loc", "%b_loc", "%b_loc", "%b_loc", "%b_loc"};

			for (int indexLevel = 0; indexLevel < numLevels; ++indexLevel)
			{
				const int baseOffset	= getBaseOffsetForSingleInputBuffer(indexesForLevel[indexLevel][0],
																			indexesForLevel[indexLevel][1],
																			indexesForLevel[indexLevel][2],
																			indexesForLevel[indexLevel][3],
																			indexesForLevel[indexLevel][4],
																			indexesForLevel[indexLevel][5]);
				// Use OpSelect to choose between 2 pointers
				{
					ComputeShaderSpec				spec;
					map<string, string>				specs;
					string opCodeForTests			= "opselect";
					string name						= opCodeForTests + indexLevelNames[indexLevel] + bufferType + selectedInputStr;
					specs["extra_capability"]		= extraCap;
					specs["selected_type"]			= pointerTypeAtLevel[indexLevel];
					specs["select_inputA"]			= spirvSelectInputA;
					specs["a_loc"]					= inputALocations[indexLevel];
					specs["b_loc"]					= inputBLocations[indexLevel];
					specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
					specs["selection_strategy"]		= "%var_ptr	= OpSelect "
														+ pointerTypeAtLevel[indexLevel] + " "
														+ spirvSelectInputA + " "
														+ baseANameAtLevel[indexLevel] + " "
														+ baseBNameAtLevel[indexLevel] + "\n";
					expectedOutput[0]				= selectedInput[baseOffset];
					spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.assembly					= shaderTemplate.specialize(specs);
					spec.numWorkGroups				= IVec3(1, 1, 1);
					spec.requestedVulkanFeatures	= requiredFeatures;
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputA)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputB)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputC)));
					spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
					spec.extensions.push_back("VK_KHR_variable_pointers");
					group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
				}

				// Use OpFunctionCall to choose between 2 pointers
				{
					ComputeShaderSpec				spec;
					map<string, string>				specs;
					string opCodeForTests			= "opfunctioncall";
					string name						= opCodeForTests + indexLevelNames[indexLevel] + bufferType + selectedInputStr;
					specs["extra_capability"]		= extraCap;
					specs["selected_type"]			= pointerTypeAtLevel[indexLevel];
					specs["select_inputA"]			= spirvSelectInputA;
					specs["a_loc"]					= inputALocations[indexLevel];
					specs["b_loc"]					= inputBLocations[indexLevel];
					specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
					specs["selection_strategy"]		= "%var_ptr	= OpFunctionCall "
														+ pointerTypeAtLevel[indexLevel]
														+ " %choose_input_func "
														+ spirvSelectInputA + " "
														+ baseANameAtLevel[indexLevel] + " "
														+ baseBNameAtLevel[indexLevel] + "\n";
					expectedOutput[0]				= selectedInput[baseOffset];
					spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.assembly					= shaderTemplate.specialize(specs);
					spec.numWorkGroups				= IVec3(1, 1, 1);
					spec.requestedVulkanFeatures	= requiredFeatures;
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputA)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputB)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputC)));
					spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
					spec.extensions.push_back("VK_KHR_variable_pointers");
					group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
				}

				// Use OpPhi to choose between 2 pointers
				{

					ComputeShaderSpec				spec;
					map<string, string>				specs;
					string opCodeForTests			= "opphi";
					string name						= opCodeForTests + indexLevelNames[indexLevel] + bufferType + selectedInputStr;
					specs["extra_capability"]		= extraCap;
					specs["selected_type"]			= pointerTypeAtLevel[indexLevel];
					specs["select_inputA"]			= spirvSelectInputA;
					specs["a_loc"]					= inputALocations[indexLevel];
					specs["b_loc"]					= inputBLocations[indexLevel];
					specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
					specs["selection_strategy"]		=
									"				  OpSelectionMerge %end_label None\n"
									"				  OpBranchConditional " + spirvSelectInputA + " %take_input_a %take_input_b\n"
									"%take_input_a	= OpLabel\n"
									"				  OpBranch %end_label\n"
									"%take_input_b	= OpLabel\n"
									"			      OpBranch %end_label\n"
									"%end_label		= OpLabel\n"
									"%var_ptr		= OpPhi "
														+ pointerTypeAtLevel[indexLevel] + " "
														+ baseANameAtLevel[indexLevel]
														+ " %take_input_a "
														+ baseBNameAtLevel[indexLevel]
														+ " %take_input_b\n";
					expectedOutput[0]				= selectedInput[baseOffset];
					spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.assembly					= shaderTemplate.specialize(specs);
					spec.numWorkGroups				= IVec3(1, 1, 1);
					spec.requestedVulkanFeatures	= requiredFeatures;
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputA)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputB)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputC)));
					spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
					spec.extensions.push_back("VK_KHR_variable_pointers");
					group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
				}

				// Use OpCopyObject to get variable pointers
				{
					ComputeShaderSpec				spec;
					map<string, string>				specs;
					string opCodeForTests			= "opcopyobject";
					string name						= opCodeForTests + indexLevelNames[indexLevel] + bufferType + selectedInputStr;
					specs["extra_capability"]		= extraCap;
					specs["selected_type"]			= pointerTypeAtLevel[indexLevel];
					specs["select_inputA"]			= spirvSelectInputA;
					specs["a_loc"]					= inputALocations[indexLevel];
					specs["b_loc"]					= inputBLocations[indexLevel];
					specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
					specs["selection_strategy"]		=
										"%in_a_copy = OpCopyObject " + pointerTypeAtLevel[indexLevel] + " " + baseANameAtLevel[indexLevel] + "\n"
										"%in_b_copy = OpCopyObject " + pointerTypeAtLevel[indexLevel] + " " + baseBNameAtLevel[indexLevel] + "\n"
										"%var_ptr	= OpSelect " + pointerTypeAtLevel[indexLevel] + " " + spirvSelectInputA + " %in_a_copy %in_b_copy\n";
					expectedOutput[0]				= selectedInput[baseOffset];
					spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.assembly					= shaderTemplate.specialize(specs);
					spec.numWorkGroups				= IVec3(1, 1, 1);
					spec.requestedVulkanFeatures	= requiredFeatures;
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputA)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputB)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputC)));
					spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
					spec.extensions.push_back("VK_KHR_variable_pointers");
					group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
				}

				// Use OpPtrAccessChain to get variable pointers
				{
					ComputeShaderSpec				spec;
					map<string, string>				specs;
					string opCodeForTests			= "opptraccesschain";
					string name						= opCodeForTests + indexLevelNames[indexLevel] + bufferType + selectedInputStr;
					specs["extra_capability"]		= extraCap;
					specs["selected_type"]			= pointerTypeAtLevel[indexLevel];
					specs["select_inputA"]			= spirvSelectInputA;
					specs["a_loc"]					= inputAPtrAccessChain[indexLevel];
					specs["b_loc"]					= inputBPtrAccessChain[indexLevel];
					specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
					specs["selection_strategy"]		= "%var_ptr	= OpSelect "
														+ pointerTypeAtLevel[indexLevel] + " "
														+ spirvSelectInputA + " "
														+ baseANameAtLevel[indexLevel] + " "
														+ baseBNameAtLevel[indexLevel] + "\n";
					expectedOutput[0]				= selectedInput[baseOffset];
					spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[1]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.inputTypes[2]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					spec.assembly					= shaderTemplate.specialize(specs);
					spec.numWorkGroups				= IVec3(1, 1, 1);
					spec.requestedVulkanFeatures	= requiredFeatures;
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputA)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputB)));
					spec.inputs.push_back(BufferSp(new Float32Buffer(inputC)));
					spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
					spec.extensions.push_back("VK_KHR_variable_pointers");
					group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
				}
			}
		}
	}
}

void addNullptrVariablePointersComputeGroup (tcu::TestCaseGroup* group)
{
	tcu::TestContext&				testCtx					= group->getTestContext();
	float							someFloat				= 78;
	vector<float>					input					(1, someFloat);
	vector<float>					expectedOutput			(1, someFloat);
	VulkanFeatures					requiredFeatures;

	// Requires the variable pointers feature.
	requiredFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS;

	const string decorations (
		// Decorations
		"OpDecorate %id BuiltIn GlobalInvocationId		\n"
		"OpDecorate %input DescriptorSet 0				\n"
		"OpDecorate %outdata DescriptorSet 0			\n"
		"OpDecorate %input Binding 0					\n"
		"OpDecorate %outdata Binding 1					\n"

		// Set the Block decoration
		"OpDecorate %float_struct	Block				\n"

		// Set the Offsets
		"OpMemberDecorate %float_struct 0 Offset 0		\n"
	);

	const string types (
		///////////
		// TYPES //
		///////////
		// struct float_struct {
		//   float x;
		// };
		"%float_struct		= OpTypeStruct %f32											\n"

		///////////////////
		// POINTER TYPES //
		///////////////////
		"%float_struct_ptr	= OpTypePointer StorageBuffer %float_struct					\n"
		"%sb_f32ptr			= OpTypePointer StorageBuffer %f32							\n"
		"%func_f32ptrptr	= OpTypePointer Function %sb_f32ptr							\n"

		///////////////
		// CONSTANTS //
		///////////////
		"%c_bool_true		= OpConstantTrue	%bool									\n"
		"%c_i32_0			= OpConstant		%i32		0							\n"
		"%c_null_ptr		= OpConstantNull	%sb_f32ptr								\n"

		///////////////
		// VARIABLES //
		///////////////
		"%id				= OpVariable %uvec3ptr			Input						\n"
		"%input				= OpVariable %float_struct_ptr	StorageBuffer				\n"
		"%outdata			= OpVariable %float_struct_ptr	StorageBuffer				\n"
	);

	const StringTemplate shaderTemplate (
		"OpCapability Shader\n"
		"OpCapability VariablePointers\n"

		"OpExtension \"SPV_KHR_variable_pointers\"\n"
		"OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		+ decorations

		+ string(getComputeAsmCommonTypes())

		+ types +

		// main function is the entry_point
		"%main					= OpFunction %void None %voidf\n"
		"%label					= OpLabel\n"

		// Note that the Variable Pointers extension allows creation
		// of a pointer variable with storage class of Private or Function.
		"%f32_ptr_var		    = OpVariable %func_f32ptrptr Function %c_null_ptr\n"

		"%input_loc				= OpAccessChain %sb_f32ptr %input %c_i32_0\n"
		"%output_loc			= OpAccessChain %sb_f32ptr %outdata %c_i32_0\n"

		"${NullptrTestingStrategy}\n"

		"						  OpReturn\n"
		"						  OpFunctionEnd\n");

	// f32_ptr_var has been inintialized to NULL.
	// Now set it to point to the float variable that holds the input value
	{
		ComputeShaderSpec				spec;
		map<string, string>				specs;
		string name						= "opvariable_initialized_null";
		specs["NullptrTestingStrategy"]	=
							"                  OpStore %f32_ptr_var %input_loc       \n"
							"%loaded_f32_ptr = OpLoad  %sb_f32ptr   %f32_ptr_var    \n"
							"%loaded_f32     = OpLoad  %f32         %loaded_f32_ptr \n"
							"                  OpStore %output_loc  %loaded_f32     \n";

		spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		spec.assembly					= shaderTemplate.specialize(specs);
		spec.numWorkGroups				= IVec3(1, 1, 1);
		spec.requestedVulkanFeatures	= requiredFeatures;
		spec.inputs.push_back(BufferSp(new Float32Buffer(input)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
		spec.extensions.push_back("VK_KHR_variable_pointers");
		group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
	}
	// Use OpSelect to choose between nullptr and valid pointer. Since we can't dereference nullptr,
	// it is forced to always choose the valid pointer.
	{
		ComputeShaderSpec				spec;
		map<string, string>				specs;
		string name						= "opselect_null_or_valid_ptr";
		specs["NullptrTestingStrategy"]	=
							"%selected_ptr = OpSelect %sb_f32ptr %c_bool_true %input_loc %c_null_ptr\n"
							"%loaded_var = OpLoad %f32 %selected_ptr\n"
							"OpStore %output_loc %loaded_var\n";

		spec.inputTypes[0]				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		spec.assembly					= shaderTemplate.specialize(specs);
		spec.numWorkGroups				= IVec3(1, 1, 1);
		spec.requestedVulkanFeatures	= requiredFeatures;
		spec.inputs.push_back(BufferSp(new Float32Buffer(input)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(expectedOutput)));
		spec.extensions.push_back("VK_KHR_variable_pointers");
		group->addChild(new SpvAsmComputeShaderCase(testCtx, name.c_str(), name.c_str(), spec));
	}
}

void addVariablePointersGraphicsGroup (tcu::TestCaseGroup* testGroup)
{
	tcu::TestContext&				testCtx					= testGroup->getTestContext();
	de::Random						rnd						(deStringHash(testGroup->getName()));
	map<string, string>				fragments;
	RGBA							defaultColors[4];
	vector<string>					extensions;
	const int						seed					= testCtx.getCommandLine().getBaseSeed();
	const int						numMuxes				= 100;
	const std::string				numMuxesStr				= "100";
	vector<float>					inputAFloats			(2*numMuxes, 0);
	vector<float>					inputBFloats			(2*numMuxes, 0);
	vector<float>					inputSFloats			(numMuxes, 0);
	vector<float>					AmuxAOutputFloats		(numMuxes, 0);
	vector<float>					AmuxBOutputFloats		(numMuxes, 0);
	vector<float>					incrAmuxAOutputFloats	(numMuxes, 0);
	vector<float>					incrAmuxBOutputFloats	(numMuxes, 0);
	VulkanFeatures					requiredFeatures;

	extensions.push_back("VK_KHR_variable_pointers");
	getDefaultColors(defaultColors);

	// Each output entry is chosen as follows: ( 0 <= i < numMuxes)
	// 1) For tests with one input buffer:  output[i] = (s[i] < 0) ? A[2*i] : A[2*i+1];
	// 2) For tests with two input buffers: output[i] = (s[i] < 0) ? A[i]   : B[i];

	fillRandomScalars(rnd, -100.f, 100.f, &inputAFloats[0], 2*numMuxes);
	fillRandomScalars(rnd, -100.f, 100.f, &inputBFloats[0], 2*numMuxes);

	// We want to guarantee that the S input has some positive and some negative values.
	// We choose random negative numbers for the first half, random positive numbers for the second half, and then shuffle.
	fillRandomScalars(rnd, -100.f, -1.f , &inputSFloats[0], numMuxes / 2);
	fillRandomScalars(rnd, 1.f   , 100.f, &inputSFloats[numMuxes / 2], numMuxes / 2);
	de::Random(seed).shuffle(inputSFloats.begin(), inputSFloats.end());

	for (size_t i = 0; i < numMuxes; ++i)
	{
		AmuxAOutputFloats[i]	 = (inputSFloats[i] < 0) ? inputAFloats[2*i]	 : inputAFloats[2*i+1];
		AmuxBOutputFloats[i]	 = (inputSFloats[i] < 0) ? inputAFloats[i]		 : inputBFloats[i];
		incrAmuxAOutputFloats[i] = (inputSFloats[i] < 0) ? 1 + inputAFloats[2*i] : 1 + inputAFloats[2*i+1];
		incrAmuxBOutputFloats[i] = (inputSFloats[i] < 0) ? 1 + inputAFloats[i]	 : 1 + inputBFloats[i];
	}

	fragments["extension"]		= "OpExtension \"SPV_KHR_variable_pointers\"\n"
								  "OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n";

	const StringTemplate preMain		(
		"%c_i32_limit = OpConstant %i32 " + numMuxesStr + "\n"
		"     %sb_f32 = OpTypePointer StorageBuffer %f32\n"
		"     %ra_f32 = OpTypeRuntimeArray %f32\n"
		"        %buf = OpTypeStruct %ra_f32\n"
		"     %sb_buf = OpTypePointer StorageBuffer %buf\n"

		" ${ExtraTypes}"

		" ${ExtraGlobalScopeVars}"

		"   %indata_a = OpVariable %sb_buf StorageBuffer\n"
		"   %indata_b = OpVariable %sb_buf StorageBuffer\n"
		"   %indata_s = OpVariable %sb_buf StorageBuffer\n"
		"    %outdata = OpVariable %sb_buf StorageBuffer\n"

		" ${ExtraFunctions} ");

	const std::string selectorFunction	(
		// We're going to put the "selector" function here.
		// This function type is needed for tests that use OpFunctionCall.
		"%selector_func_type	= OpTypeFunction %sb_f32 %bool %sb_f32 %sb_f32\n"
		"%choose_input_func		= OpFunction %sb_f32 None %selector_func_type\n"
		"%is_neg_param			= OpFunctionParameter %bool\n"
		"%first_ptr_param		= OpFunctionParameter %sb_f32\n"
		"%second_ptr_param		= OpFunctionParameter %sb_f32\n"
		"%selector_func_begin	= OpLabel\n"
		"%result_ptr			= OpSelect %sb_f32 %is_neg_param %first_ptr_param %second_ptr_param\n"
		"OpReturnValue %result_ptr\n"
		"OpFunctionEnd\n");

	const StringTemplate decoration		(
		"OpMemberDecorate %buf 0 Offset 0\n"
		"OpDecorate %buf Block\n"
		"OpDecorate %ra_f32 ArrayStride 4\n"
		"OpDecorate %sb_f32 ArrayStride 4\n"
		"OpDecorate %indata_a DescriptorSet 0\n"
		"OpDecorate %indata_b DescriptorSet 0\n"
		"OpDecorate %indata_s DescriptorSet 0\n"
		"OpDecorate %outdata  DescriptorSet 0\n"
		"OpDecorate %indata_a Binding 0\n"
		"OpDecorate %indata_b Binding 1\n"
		"OpDecorate %indata_s Binding 2\n"
		"OpDecorate %outdata  Binding 3\n");

	const StringTemplate testFunction	(
		"%test_code		= OpFunction %v4f32 None %v4f32_function\n"
		"%param			= OpFunctionParameter %v4f32\n"
		"%entry			= OpLabel\n"

		"${ExtraFunctionScopeVars}"

		"%i				= OpVariable %fp_i32 Function\n"

		"%should_run    = OpFunctionCall %bool %isUniqueIdZero\n"
		"                 OpSelectionMerge %end_if None\n"
		"                 OpBranchConditional %should_run %run_test %end_if\n"

		"%run_test      = OpLabel\n"
		"				OpStore %i %c_i32_0\n"
		"				OpBranch %loop\n"
		// loop header
		"%loop			= OpLabel\n"
		"%15			= OpLoad %i32 %i\n"
		"%lt			= OpSLessThan %bool %15 %c_i32_limit\n"
		"				OpLoopMerge %merge %inc None\n"
		"				OpBranchConditional %lt %write %merge\n"
		// loop body
		"%write				= OpLabel\n"
		"%30				= OpLoad %i32 %i\n"
		"%two_i				= OpIAdd %i32 %30 %30\n"
		"%two_i_plus_1		= OpIAdd %i32 %two_i %c_i32_1\n"
		"%loc_s_i			= OpAccessChain %sb_f32 %indata_s %c_i32_0 %30\n"
		"%loc_a_i			= OpAccessChain %sb_f32 %indata_a %c_i32_0 %30\n"
		"%loc_b_i			= OpAccessChain %sb_f32 %indata_b %c_i32_0 %30\n"
		"%loc_a_2i			= OpAccessChain %sb_f32 %indata_a %c_i32_0 %two_i\n"
		"%loc_a_2i_plus_1	= OpAccessChain %sb_f32 %indata_a %c_i32_0 %two_i_plus_1\n"
		"%loc_outdata_i		= OpAccessChain %sb_f32 %outdata  %c_i32_0 %30\n"
		"%val_s_i			= OpLoad %f32 %loc_s_i\n"
		"%is_neg			= OpFOrdLessThan %bool %val_s_i %c_f32_0\n"

		// select using a strategy.
		"${ResultStrategy}"

		// load through the variable pointer
		"%mux_output	= OpLoad %f32 ${VarPtrName}\n"

		// store to the output vector.
		"				OpStore %loc_outdata_i %mux_output\n"
		"				OpBranch %inc\n"
		// ++i
		"  %inc			= OpLabel\n"
		"   %37			= OpLoad %i32 %i\n"
		"   %39			= OpIAdd %i32 %37 %c_i32_1\n"
		"         OpStore %i %39\n"
		"         OpBranch %loop\n"

		// Return and FunctionEnd
		"%merge			= OpLabel\n"
		"                 OpBranch %end_if\n"
		"%end_if		= OpLabel\n"
		"OpReturnValue %param\n"
		"OpFunctionEnd\n");

	const bool singleInputBuffer[] = { true, false };
	for (int inputBufferTypeIndex = 0 ; inputBufferTypeIndex < 2; ++inputBufferTypeIndex)
	{
		const bool isSingleInputBuffer			= singleInputBuffer[inputBufferTypeIndex];
		const string cap						= isSingleInputBuffer	? "OpCapability VariablePointersStorageBuffer\n" : "OpCapability VariablePointers\n";
		const vector<float>& expectedOutput		= isSingleInputBuffer	? AmuxAOutputFloats		 : AmuxBOutputFloats;
		const vector<float>& expectedIncrOutput = isSingleInputBuffer	? incrAmuxAOutputFloats	 : incrAmuxBOutputFloats;
		const string bufferType					= isSingleInputBuffer	? "single_buffer"		 : "two_buffers";
		const string muxInput1					= isSingleInputBuffer	? " %loc_a_2i "			 : " %loc_a_i ";
		const string muxInput2					= isSingleInputBuffer	? " %loc_a_2i_plus_1 "	 : " %loc_b_i ";

		// Set the proper extension features required for the test
		if (isSingleInputBuffer)
			requiredFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS_STORAGEBUFFER;
		else
			requiredFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS;

		// All of the following tests write their results into an output SSBO, therefore they require the following features.
		requiredFeatures.coreFeatures.vertexPipelineStoresAndAtomics = DE_TRUE;
		requiredFeatures.coreFeatures.fragmentStoresAndAtomics		 = DE_TRUE;

		{ // Variable Pointer Reads (using OpSelect)
			GraphicsResources				resources;
			map<string, string>				specs;
			string name						= "reads_opselect_" + bufferType;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraFunctions"]			= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			= "%mux_output_var_ptr	= OpSelect %sb_f32 %is_neg" + muxInput1 + muxInput2 + "\n";

			fragments["capability"]			= cap;
			fragments["decoration"]			= decoration.specialize(specs);
			fragments["pre_main"]			= preMain.specialize(specs);
			fragments["testfun"]			= testFunction.specialize(specs);

			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputAFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputSFloats))));
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedOutput))));
			createTestsForAllStages(name.c_str(), defaultColors, defaultColors, fragments, resources, extensions, testGroup, requiredFeatures);
		}
		{ // Variable Pointer Reads (using OpFunctionCall)
			GraphicsResources				resources;
			map<string, string>				specs;
			string name						= "reads_opfunctioncall_" + bufferType;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraFunctions"]			= selectorFunction;
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			= "%mux_output_var_ptr = OpFunctionCall %sb_f32 %choose_input_func %is_neg" + muxInput1 + muxInput2 + "\n";

			fragments["capability"]			= cap;
			fragments["decoration"]			= decoration.specialize(specs);
			fragments["pre_main"]			= preMain.specialize(specs);
			fragments["testfun"]			= testFunction.specialize(specs);

			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputAFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputSFloats))));
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedOutput))));
			createTestsForAllStages(name.c_str(), defaultColors, defaultColors, fragments, resources, extensions, testGroup, requiredFeatures);
		}
		{ // Variable Pointer Reads (using OpPhi)
			GraphicsResources				resources;
			map<string, string>				specs;
			string name						= "reads_opphi_" + bufferType;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraFunctions"]			= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			=
				"							  OpSelectionMerge %end_label None\n"
				"							  OpBranchConditional %is_neg %take_mux_input_1 %take_mux_input_2\n"
				"%take_mux_input_1			= OpLabel\n"
				"							  OpBranch %end_label\n"
				"%take_mux_input_2			= OpLabel\n"
				"						      OpBranch %end_label\n"
				"%end_label					= OpLabel\n"
				"%mux_output_var_ptr		= OpPhi %sb_f32" + muxInput1 + "%take_mux_input_1" + muxInput2 + "%take_mux_input_2\n";

			fragments["capability"]			= cap;
			fragments["decoration"]			= decoration.specialize(specs);
			fragments["pre_main"]			= preMain.specialize(specs);
			fragments["testfun"]			= testFunction.specialize(specs);

			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputAFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputSFloats))));
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedOutput))));
			createTestsForAllStages(name.c_str(), defaultColors, defaultColors, fragments, resources, extensions, testGroup, requiredFeatures);
		}
		{ // Variable Pointer Reads (using OpCopyObject)
			GraphicsResources				resources;
			map<string, string>				specs;
			string name						= "reads_opcopyobject_" + bufferType;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraFunctions"]			= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			=
				"%mux_input_1_copy			= OpCopyObject %sb_f32" + muxInput1 + "\n"
				"%mux_input_2_copy			= OpCopyObject %sb_f32" + muxInput2 + "\n"
				"%mux_output_var_ptr		= OpSelect %sb_f32 %is_neg %mux_input_1_copy %mux_input_2_copy\n";

			fragments["capability"]			= cap;
			fragments["decoration"]			= decoration.specialize(specs);
			fragments["pre_main"]			= preMain.specialize(specs);
			fragments["testfun"]			= testFunction.specialize(specs);

			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputAFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputSFloats))));
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedOutput))));
			createTestsForAllStages(name.c_str(), defaultColors, defaultColors, fragments, resources, extensions, testGroup, requiredFeatures);
		}
		{ // Test storing into Private variables.
			const char* storageClasses[]		= {"Private", "Function"};
			for (int classId = 0; classId < 2; ++classId)
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				std::string storageClass		= storageClasses[classId];
				std::string name				= "stores_" + string(de::toLower(storageClass)) + "_" + bufferType;
				std::string extraVariable		= "%mux_output_copy	= OpVariable %sb_f32ptrptr " + storageClass + "\n";
				specs["ExtraTypes"]				= "%sb_f32ptrptr = OpTypePointer " + storageClass + " %sb_f32\n";
				specs["ExtraGlobalScopeVars"]	= (classId == 0) ? extraVariable : "";
				specs["ExtraFunctionScopeVars"]	= (classId == 1) ? extraVariable : "";
				specs["ExtraFunctions"]			= "";
				specs["VarPtrName"]				= "%mux_output_var_ptr";
				specs["ResultStrategy"]			=
					"%opselect_result			= OpSelect %sb_f32 %is_neg" + muxInput1 + muxInput2 + "\n"
					"							  OpStore %mux_output_copy %opselect_result\n"
					"%mux_output_var_ptr		= OpLoad %sb_f32 %mux_output_copy\n";

				fragments["capability"]			= cap;
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);

				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputAFloats))));
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBFloats))));
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputSFloats))));
				resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedOutput))));
				createTestsForAllStages(name.c_str(), defaultColors, defaultColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
		}
		{ // Variable Pointer Reads (using OpPtrAccessChain)
			GraphicsResources				resources;
			map<string, string>				specs;
			std::string name				= "reads_opptraccesschain_" + bufferType;
			std::string in_1				= isSingleInputBuffer ? " %a_2i_ptr "		 : " %a_i_ptr ";
			std::string in_2				= isSingleInputBuffer ? " %a_2i_plus_1_ptr " : " %b_i_ptr ";
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraFunctions"]			= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			=
					"%a_ptr					= OpAccessChain %sb_f32 %indata_a %c_i32_0 %c_i32_0\n"
					"%b_ptr					= OpAccessChain %sb_f32 %indata_b %c_i32_0 %c_i32_0\n"
					"%s_ptr					= OpAccessChain %sb_f32 %indata_s %c_i32_0 %c_i32_0\n"
					"%out_ptr               = OpAccessChain %sb_f32 %outdata  %c_i32_0 %c_i32_0\n"
					"%a_i_ptr               = OpPtrAccessChain %sb_f32 %a_ptr %30\n"
					"%b_i_ptr               = OpPtrAccessChain %sb_f32 %b_ptr %30\n"
					"%s_i_ptr               = OpPtrAccessChain %sb_f32 %s_ptr %30\n"
					"%a_2i_ptr              = OpPtrAccessChain %sb_f32 %a_ptr %two_i\n"
					"%a_2i_plus_1_ptr       = OpPtrAccessChain %sb_f32 %a_ptr %two_i_plus_1\n"
					"%mux_output_var_ptr    = OpSelect %sb_f32 %is_neg " + in_1 + in_2 + "\n";

			fragments["decoration"]			= decoration.specialize(specs);
			fragments["pre_main"]			= preMain.specialize(specs);
			fragments["testfun"]			= testFunction.specialize(specs);
			fragments["capability"]			= cap;

			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputAFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputSFloats))));
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedOutput))));
			createTestsForAllStages(name.c_str(), defaultColors, defaultColors, fragments, resources, extensions, testGroup, requiredFeatures);
		}
		{   // Variable Pointer Writes
			GraphicsResources				resources;
			map<string, string>				specs;
			std::string	name				= "writes_" + bufferType;
			specs["ExtraTypes"]				= "";
			specs["ExtraGlobalScopeVars"]	= "";
			specs["ExtraFunctionScopeVars"]	= "";
			specs["ExtraFunctions"]			= "";
			specs["VarPtrName"]				= "%mux_output_var_ptr";
			specs["ResultStrategy"]			=
					   "%mux_output_var_ptr = OpSelect %sb_f32 %is_neg" + muxInput1 + muxInput2 + "\n" +
					   "               %val = OpLoad %f32 %mux_output_var_ptr\n"
					   "        %val_plus_1 = OpFAdd %f32 %val %c_f32_1\n"
					   "					  OpStore %mux_output_var_ptr %val_plus_1\n";
			fragments["capability"]			= cap;
			fragments["decoration"]			= decoration.specialize(specs);
			fragments["pre_main"]			= preMain.specialize(specs);
			fragments["testfun"]			= testFunction.specialize(specs);

			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputAFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBFloats))));
			resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputSFloats))));
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(expectedIncrOutput))));
			createTestsForAllStages(name.c_str(), defaultColors, defaultColors, fragments, resources, extensions, testGroup, requiredFeatures);
		}
	}
}

// Modifies the 'red channel' of the input color to the given float value.
// Returns the modified color.
void getExpectedOutputColor(RGBA (&inputColors)[4], RGBA (&expectedOutputColors)[4], float val)
{
	Vec4 inColor0 = inputColors[0].toVec();
	Vec4 inColor1 = inputColors[1].toVec();
	Vec4 inColor2 = inputColors[2].toVec();
	Vec4 inColor3 = inputColors[3].toVec();
	inColor0[0] = val;
	inColor1[0] = val;
	inColor2[0] = val;
	inColor3[0] = val;
	expectedOutputColors[0] = RGBA(inColor0);
	expectedOutputColors[1] = RGBA(inColor1);
	expectedOutputColors[2] = RGBA(inColor2);
	expectedOutputColors[3] = RGBA(inColor3);
}

void addTwoInputBufferReadOnlyVariablePointersGraphicsGroup (tcu::TestCaseGroup* testGroup)
{
	const int						numFloatsPerInput		= 64;
	vector<float>					inputA					(numFloatsPerInput, 0);
	vector<float>					inputB					(numFloatsPerInput, 0);
	deUint32						baseOffset				= -1;
	VulkanFeatures					requiredFeatures;
	map<string, string>				fragments;
	RGBA							defaultColors[4];
	RGBA							expectedColors[4];
	vector<string>					extensions;

	getDefaultColors(defaultColors);

	// Set the proper extension features required for the tests.
	requiredFeatures.extVariablePointers = EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS;

	// Set the required extension.
	extensions.push_back("VK_KHR_variable_pointers");

	// These tests exercise variable pointers into various levels of the following data-structure:
	// struct struct inner_struct {
	//   vec4 x[2]; // array of 2 vectors. Each vector is 4 floats.
	//   vec4 y[2]; // array of 2 vectors. Each vector is 4 floats.
	// };
	//
	// struct outer_struct {
	//   inner_struct r[2][2];
	// };
	//
	// The inner_struct contains 16 floats, and the outer_struct contains 64 floats.
	// Therefore the input can be an array of 64 floats.

	// Populate the first input (inputA) to contain:  {0, 4, ... , 252} / 255.f
	// Populate the second input (inputB) to contain: {3, 7, ... , 255} / 255.f
	for (size_t i = 0; i < numFloatsPerInput; ++i)
	{
		inputA[i] = 4*float(i) / 255;
		inputB[i] = ((4*float(i)) + 3) / 255;
	}

	// In the following tests we use variable pointers to point to different types:
	// nested structures, matrices of structures, arrays of structures, arrays of vectors, vectors of scalars, and scalars.
	// outer_structure.inner_structure[?][?].x[?][?];
	//   ^                    ^        ^  ^  ^ ^  ^
	//
	// 1. inputA						or	inputB						= nested structure
	// 2. inputA.r						or	inputB.r					= matrices of structures
	// 3. inputA.r[?]					or	inputB.r[?]					= arrays of structures
	// 4. inputA.r[?][?]				or	inputB.r[?][?]				= structures
	// 5. inputA.r[?][?].(x|y)			or	inputB.r[?][?].(x|y)		= arrays of vectors
	// 6. inputA.r[?][?].(x|y)[?]		or	inputB.r[?][?].(x|y)[?]		= vectors of scalars
	// 7. inputA.r[?][?].(x|y)[?][?]	or	inputB.r[?][?].(x|y)[?][?]	= scalars
	const int numLevels	= 7;

	fragments["capability"]		=	"OpCapability VariablePointers							\n";
	fragments["extension"]		=	"OpExtension \"SPV_KHR_variable_pointers\"				\n"
									"OpExtension \"SPV_KHR_storage_buffer_storage_class\"	\n";

	const StringTemplate decoration		(
		// Set the Offsets
		"OpMemberDecorate			%outer_struct 0 Offset 0	\n"
		"OpMemberDecorate			%inner_struct 0 Offset 0	\n"
		"OpMemberDecorate			%inner_struct 1 Offset 32	\n"

		// Set the ArrayStrides
		"OpDecorate %arr2_v4float        ArrayStride 16			\n"
		"OpDecorate %arr2_inner_struct   ArrayStride 64			\n"
		"OpDecorate %mat2x2_inner_struct ArrayStride 128		\n"
		"OpDecorate %mat2x2_ptr			 ArrayStride 128		\n"
		"OpDecorate %sb_buf              ArrayStride 256		\n"
		"OpDecorate %v4f32_ptr           ArrayStride 16			\n"

		"OpDecorate					%outer_struct Block			\n"

		"OpDecorate %in_a			DescriptorSet 0				\n"
		"OpDecorate %in_b			DescriptorSet 0				\n"
		"OpDecorate %in_a			Binding 0					\n"
		"OpDecorate %in_b			Binding 1					\n"
	);

	const StringTemplate preMain		(
		///////////
		// TYPES //
		///////////

		// struct struct inner_struct {
		//   vec4 x[2]; // array of 2 vectors
		//   vec4 y[2]; // array of 2 vectors
		// };
		"%arr2_v4float			= OpTypeArray %v4f32 %c_u32_2						\n"
		"%inner_struct			= OpTypeStruct %arr2_v4float %arr2_v4float			\n"

		// struct outer_struct {
		//   inner_struct r[2][2];
		// };
		"%arr2_inner_struct		= OpTypeArray %inner_struct %c_u32_2				\n"
		"%mat2x2_inner_struct	= OpTypeArray %arr2_inner_struct %c_u32_2			\n"
		"%outer_struct			= OpTypeStruct %mat2x2_inner_struct					\n"

		///////////////////
		// POINTER TYPES //
		///////////////////
		"%sb_buf				= OpTypePointer StorageBuffer %outer_struct			\n"
		"%mat2x2_ptr			= OpTypePointer StorageBuffer %mat2x2_inner_struct	\n"
		"%arr2_ptr				= OpTypePointer StorageBuffer %arr2_inner_struct	\n"
		"%inner_struct_ptr		= OpTypePointer StorageBuffer %inner_struct			\n"
		"%arr_v4f32_ptr			= OpTypePointer StorageBuffer %arr2_v4float			\n"
		"%v4f32_ptr				= OpTypePointer StorageBuffer %v4f32				\n"
		"%sb_f32ptr				= OpTypePointer StorageBuffer %f32					\n"

		///////////////
		// VARIABLES //
		///////////////
		"%in_a					= OpVariable %sb_buf StorageBuffer					\n"
		"%in_b					= OpVariable %sb_buf StorageBuffer					\n"

		///////////////
		// CONSTANTS //
		///////////////
		"%c_bool_true			= OpConstantTrue %bool								\n"
		"%c_bool_false			= OpConstantFalse %bool								\n"

		//////////////////////
		// HELPER FUNCTIONS //
		//////////////////////
		"${helper_functions} \n"
	);

	const StringTemplate selectorFunctions	(
		// This selector function returns a variable pointer.
		// These functions are used by tests that use OpFunctionCall to obtain the variable pointer.
		"%selector_func_type	= OpTypeFunction ${selected_type} %bool ${selected_type} ${selected_type}\n"
		"%choose_input_func		= OpFunction ${selected_type} None %selector_func_type\n"
		"%choose_first_param	= OpFunctionParameter %bool\n"
		"%first_param			= OpFunctionParameter ${selected_type}\n"
		"%second_param			= OpFunctionParameter ${selected_type}\n"
		"%selector_func_begin	= OpLabel\n"
		"%result_ptr			= OpSelect ${selected_type} %choose_first_param %first_param %second_param\n"
		"						  OpReturnValue %result_ptr\n"
		"						  OpFunctionEnd\n"
	);

	const StringTemplate testFunction	(
		"%test_code		= OpFunction %v4f32 None %v4f32_function\n"
		"%param			= OpFunctionParameter %v4f32\n"
		"%entry			= OpLabel\n"

		// Define base pointers for OpPtrAccessChain
		"%in_a_matptr	= OpAccessChain %mat2x2_ptr %in_a %c_i32_0\n"
		"%in_b_matptr	= OpAccessChain %mat2x2_ptr %in_b %c_i32_0\n"

		// Define the 2 pointers from which we're going to choose one.
		"${a_loc} \n"
		"${b_loc} \n"

		// Choose between the 2 pointers / variable pointers
		"${selection_strategy} \n"

		// OpAccessChain into the variable pointer until you get to the float.
		"%result_loc	= OpAccessChain %sb_f32ptr %var_ptr  ${remaining_indexes} \n"

		// Now load from the result_loc
		"%result_val	= OpLoad %f32 %result_loc\n"

		// Modify the 'RED channel' of the output color to the chosen value
		"%output_color	= OpCompositeInsert %v4f32 %result_val %param 0 \n"

		// Return and FunctionEnd
		"OpReturnValue %output_color\n"
		"OpFunctionEnd\n");

	// When select is 0, the variable pointer should point to a value in the first input (inputA).
	// When select is 1, the variable pointer should point to a value in the second input (inputB).
	for (int selectInputA = 0; selectInputA < 2; ++selectInputA)
	{
		const string selectedInputStr		= selectInputA ? "first_input"	: "second_input";
		const string spirvSelectInputA		= selectInputA ? "%c_bool_true"	: "%c_bool_false";
		vector<float>& selectedInput		= selectInputA ? inputA			: inputB;

		// The indexes chosen at each level. At any level, any given offset is exercised.
		// The first index is always zero as the outer structure has only 1 member.
		const int indexesForLevel[numLevels][6]= {{0, 0, 0, 0, 0, 1},
												  {0, 1, 0, 1, 0, 2},
												  {0, 0, 1, 0, 1, 3},
												  {0, 1, 1, 1, 0, 0},
												  {0, 0, 0, 1, 1, 1},
												  {0, 1, 0, 0, 0, 2},
												  {0, 1, 1, 1, 1, 3}};

		const string indexLevelNames[]		= {"_outer_struct_", "_matrices_", "_arrays_", "_inner_structs_", "_vec4arr_", "_vec4_", "_float_"};
		const string inputALocations[]		= {	"",
											"%a_loc = OpAccessChain %mat2x2_ptr       %in_a %c_i32_0",
											"%a_loc = OpAccessChain %arr2_ptr         %in_a %c_i32_0 %c_i32_0",
											"%a_loc = OpAccessChain %inner_struct_ptr %in_a %c_i32_0 %c_i32_1 %c_i32_1",
											"%a_loc = OpAccessChain %arr_v4f32_ptr    %in_a %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
											"%a_loc = OpAccessChain %v4f32_ptr        %in_a %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
											"%a_loc = OpAccessChain %sb_f32ptr        %in_a %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3"};

		const string inputBLocations[]		= {	"",
											"%b_loc = OpAccessChain %mat2x2_ptr       %in_b %c_i32_0",
											"%b_loc = OpAccessChain %arr2_ptr         %in_b %c_i32_0 %c_i32_0",
											"%b_loc = OpAccessChain %inner_struct_ptr %in_b %c_i32_0 %c_i32_1 %c_i32_1",
											"%b_loc = OpAccessChain %arr_v4f32_ptr    %in_b %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
											"%b_loc = OpAccessChain %v4f32_ptr        %in_b %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
											"%b_loc = OpAccessChain %sb_f32ptr        %in_b %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3"};

		const string inputAPtrAccessChain[]	= {	"",
											"%a_loc = OpPtrAccessChain %mat2x2_ptr       %in_a_matptr %c_i32_0",
											"%a_loc = OpPtrAccessChain %arr2_ptr         %in_a_matptr %c_i32_0 %c_i32_0",
											"%a_loc = OpPtrAccessChain %inner_struct_ptr %in_a_matptr %c_i32_0 %c_i32_1 %c_i32_1",
											"%a_loc = OpPtrAccessChain %arr_v4f32_ptr    %in_a_matptr %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
											"%a_loc = OpPtrAccessChain %v4f32_ptr        %in_a_matptr %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
											// Next case emulates:
											// %a_loc = OpPtrAccessChain %sb_f32ptr      %in_a_matptr %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3
											// But rewrite it to exercise OpPtrAccessChain with a non-zero first index:
											//    %a_loc_arr is a pointer to an array that we want to index with 1.
											// But instead of just using OpAccessChain with first index 1, use OpAccessChain with index 0 to
											// get a pointer to the first element, then send that into OpPtrAccessChain with index 1.
											"%a_loc_arr = OpPtrAccessChain %arr_v4f32_ptr %in_a_matptr %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 "
											"%a_loc_first_elem = OpAccessChain %v4f32_ptr %a_loc_arr %c_i32_0 "
											"%a_loc = OpPtrAccessChain %sb_f32ptr %a_loc_first_elem %c_i32_1 %c_i32_3"};

		const string inputBPtrAccessChain[]	= {	"",
											"%b_loc = OpPtrAccessChain %mat2x2_ptr       %in_b_matptr %c_i32_0",
											"%b_loc = OpPtrAccessChain %arr2_ptr         %in_b_matptr %c_i32_0 %c_i32_0",
											"%b_loc = OpPtrAccessChain %inner_struct_ptr %in_b_matptr %c_i32_0 %c_i32_1 %c_i32_1",
											"%b_loc = OpPtrAccessChain %arr_v4f32_ptr    %in_b_matptr %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
											"%b_loc = OpPtrAccessChain %v4f32_ptr        %in_b_matptr %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
											// Next case emulates:
											// %b_loc = OpPtrAccessChain %sb_f32ptr      %in_b_matptr %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3
											// But rewrite it to exercise OpPtrAccessChain with a non-zero first index:
											//    %b_loc_arr is a pointer to an array that we want to index with 1.
											// But instead of just using OpAccessChain with first index 1, use OpAccessChain with index 0 to
											// get a pointer to the first element, then send that into OpPtrAccessChain with index 1.
											"%b_loc_arr = OpPtrAccessChain %arr_v4f32_ptr %in_b_matptr %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 "
											"%b_loc_first_elem = OpAccessChain %v4f32_ptr %b_loc_arr %c_i32_0 "
											"%b_loc = OpPtrAccessChain %sb_f32ptr %b_loc_first_elem %c_i32_1 %c_i32_3"};


		const string remainingIndexesAtLevel[]= {"%c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
												 "%c_i32_1 %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_2",
												 "%c_i32_1 %c_i32_0 %c_i32_1 %c_i32_3",
												 "%c_i32_1 %c_i32_0 %c_i32_0",
												 "%c_i32_1 %c_i32_1",
												 "%c_i32_2",
												 ""};

		const string pointerTypeAtLevel[]	= {"%sb_buf", "%mat2x2_ptr", "%arr2_ptr", "%inner_struct_ptr", "%arr_v4f32_ptr", "%v4f32_ptr", "%sb_f32ptr"};
		const string baseANameAtLevel[]		= {"%in_a", "%a_loc", "%a_loc", "%a_loc", "%a_loc", "%a_loc", "%a_loc"};
		const string baseBNameAtLevel[]			= {"%in_b", "%b_loc", "%b_loc", "%b_loc", "%b_loc", "%b_loc", "%b_loc"};

		for (int indexLevel = 0; indexLevel < numLevels; ++indexLevel)
		{
			baseOffset						= getBaseOffset(indexesForLevel[indexLevel][0],
															indexesForLevel[indexLevel][1],
															indexesForLevel[indexLevel][2],
															indexesForLevel[indexLevel][3],
															indexesForLevel[indexLevel][4],
															indexesForLevel[indexLevel][5]);

			// Use OpSelect to choose between 2 pointers
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				string opCodeForTests			= "opselect";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				specs["select_inputA"]			= spirvSelectInputA;
				specs["helper_functions"]		= "";
				specs["a_loc"]					= inputALocations[indexLevel];
				specs["b_loc"]					= inputBLocations[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		= "%var_ptr	= OpSelect " +
													pointerTypeAtLevel[indexLevel] + " " +
													spirvSelectInputA + " " +
													baseANameAtLevel[indexLevel] + " " +
													baseBNameAtLevel[indexLevel] + "\n";
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputA))));
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputB))));
				getExpectedOutputColor(defaultColors, expectedColors, selectedInput[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
			// Use OpCopyObject to get variable pointers
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				string opCodeForTests			= "opcopyobject";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				specs["select_inputA"]			= spirvSelectInputA;
				specs["helper_functions"]		= "";
				specs["a_loc"]					= inputALocations[indexLevel];
				specs["b_loc"]					= inputBLocations[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		=
									"%in_a_copy = OpCopyObject " + pointerTypeAtLevel[indexLevel] + " " + baseANameAtLevel[indexLevel] + "\n"
									"%in_b_copy = OpCopyObject " + pointerTypeAtLevel[indexLevel] + " " + baseBNameAtLevel[indexLevel] + "\n"
									"%var_ptr	= OpSelect " + pointerTypeAtLevel[indexLevel] + " " + spirvSelectInputA + " %in_a_copy %in_b_copy\n";
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputA))));
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputB))));
				getExpectedOutputColor(defaultColors, expectedColors, selectedInput[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
			// Use OpPhi to choose between 2 pointers
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				string opCodeForTests			= "opphi";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				specs["select_inputA"]			= spirvSelectInputA;
				specs["helper_functions"]		= "";
				specs["a_loc"]					= inputALocations[indexLevel];
				specs["b_loc"]					= inputBLocations[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		=
								"				  OpSelectionMerge %end_label None\n"
								"				  OpBranchConditional " + spirvSelectInputA + " %take_input_a %take_input_b\n"
								"%take_input_a	= OpLabel\n"
								"				  OpBranch %end_label\n"
								"%take_input_b	= OpLabel\n"
								"			      OpBranch %end_label\n"
								"%end_label		= OpLabel\n"
								"%var_ptr		= OpPhi "
													+ pointerTypeAtLevel[indexLevel] + " "
													+ baseANameAtLevel[indexLevel]
													+ " %take_input_a "
													+ baseBNameAtLevel[indexLevel]
													+ " %take_input_b\n";
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputA))));
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputB))));
				getExpectedOutputColor(defaultColors, expectedColors, selectedInput[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
			// Use OpFunctionCall to choose between 2 pointers
			{
				GraphicsResources				resources;
				map<string, string>				functionSpecs;
				map<string, string>				specs;
				string opCodeForTests			= "opfunctioncall";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				functionSpecs["selected_type"]	= pointerTypeAtLevel[indexLevel];
				specs["helper_functions"]		= selectorFunctions.specialize(functionSpecs);
				specs["select_inputA"]			= spirvSelectInputA;
				specs["a_loc"]					= inputALocations[indexLevel];
				specs["b_loc"]					= inputBLocations[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		= "%var_ptr	= OpFunctionCall "
													+ pointerTypeAtLevel[indexLevel]
													+ " %choose_input_func "
													+ spirvSelectInputA + " "
													+ baseANameAtLevel[indexLevel] + " "
													+ baseBNameAtLevel[indexLevel] + "\n";
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputA))));
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputB))));
				getExpectedOutputColor(defaultColors, expectedColors, selectedInput[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
			// Use OpPtrAccessChain to get variable pointers
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				string opCodeForTests			= "opptraccesschain";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				specs["select_inputA"]			= spirvSelectInputA;
				specs["helper_functions"]		= "";
				specs["a_loc"]					= inputAPtrAccessChain[indexLevel];
				specs["b_loc"]					= inputBPtrAccessChain[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		= "%var_ptr	= OpSelect "
													+ pointerTypeAtLevel[indexLevel] + " "
													+ spirvSelectInputA + " "
													+ baseANameAtLevel[indexLevel] + " "
													+ baseBNameAtLevel[indexLevel] + "\n";
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputA))));
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputB))));
				getExpectedOutputColor(defaultColors, expectedColors, selectedInput[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
		}
	}
}

void addSingleInputBufferReadOnlyVariablePointersGraphicsGroup (tcu::TestCaseGroup* testGroup)
{
	const int						numFloatsPerInnerStruct	= 64;
	vector<float>					inputBuffer				(2 * numFloatsPerInnerStruct, 0);
	deUint32						baseOffset				= -1;
	VulkanFeatures					requiredFeatures;
	map<string, string>				fragments;
	RGBA							defaultColors[4];
	RGBA							expectedColors[4];
	vector<string>					extensions;

	// Set the proper extension features required for the tests.
	// The following tests use variable pointers confined withing a single buffer.
	requiredFeatures.extVariablePointers = EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS_STORAGEBUFFER;

	// Set the required extension.
	extensions.push_back("VK_KHR_variable_pointers");

	getDefaultColors(defaultColors);

	// These tests exercise variable pointers into various levels of the following data-structure:
	// struct struct inner_struct {
	//   vec4 x[2]; // array of 2 vectors. Each vector is 4 floats.
	//   vec4 y[2]; // array of 2 vectors. Each vector is 4 floats.
	// };
	//
	// struct outer_struct {
	//   inner_struct r[2][2];
	// };
	//
	// struct input_buffer {
	//   outer_struct a;
	//   outer_struct b;
	// }
	//
	// The inner_struct contains 16 floats, and the outer_struct contains 64 floats.
	// Therefore the input_buffer can be an array of 128 floats.

	// Populate input_buffer's first member (a) to contain:  {0, 4, ... , 252} / 255.f
	// Populate input_buffer's second member (b) to contain: {3, 7, ... , 255} / 255.f
	for (size_t i = 0; i < numFloatsPerInnerStruct; ++i)
	{
		inputBuffer[i] = 4*float(i) / 255;
		inputBuffer[i + numFloatsPerInnerStruct] = ((4*float(i)) + 3) / 255;
	}

	// In the following tests we use variable pointers to point to different types:
	// nested structures, matrices of structures, arrays of structures, arrays of vectors, vectors of scalars, and scalars.
	// outer_struct.inner_struct[?][?].x[?][?];
	//   ^              ^        ^  ^  ^ ^  ^
	//
	// 1. inputBuffer.a						or	inputBuffer.b						= nested structure
	// 2. inputBuffer.a.r					or	inputBuffer.b.r						= matrices of structures
	// 3. inputBuffer.a.r[?]				or	inputBuffer.b.r[?]					= arrays of structures
	// 4. inputBuffer.a.r[?][?]				or	inputBuffer.b.r[?][?]				= structures
	// 5. inputBuffer.a.r[?][?].(x|y)		or	inputBuffer.b.r[?][?].(x|y)			= arrays of vectors
	// 6. inputBuffer.a.r[?][?].(x|y)[?]	or	inputBuffer.b.r[?][?].(x|y)[?]		= vectors of scalars
	// 7. inputBuffer.a.r[?][?].(x|y)[?][?]	or	inputBuffer.b.r[?][?].(x|y)[?][?]	= scalars
	const int numLevels	=	7;

	fragments["capability"]		=	"OpCapability VariablePointersStorageBuffer				\n";
	fragments["extension"]		=	"OpExtension \"SPV_KHR_variable_pointers\"				\n"
									"OpExtension \"SPV_KHR_storage_buffer_storage_class\"	\n";
	const StringTemplate decoration		(
		// Set the ArrayStrides
		"OpDecorate %arr2_v4float        ArrayStride 16			\n"
		"OpDecorate %arr2_inner_struct   ArrayStride 64			\n"
		"OpDecorate %mat2x2_inner_struct ArrayStride 128		\n"
		"OpDecorate %outer_struct_ptr    ArrayStride 256		\n"
		"OpDecorate %v4f32_ptr           ArrayStride 16			\n"

		// Set the Offsets
		"OpMemberDecorate			%input_buffer 0 Offset 0	\n"
		"OpMemberDecorate			%input_buffer 1 Offset 256	\n"
		"OpMemberDecorate			%outer_struct 0 Offset 0	\n"
		"OpMemberDecorate			%inner_struct 0 Offset 0	\n"
		"OpMemberDecorate			%inner_struct 1 Offset 32	\n"

		"OpDecorate					%input_buffer Block			\n"

		"OpDecorate %input			DescriptorSet 0				\n"
		"OpDecorate %input			Binding 0					\n"
	);

	const StringTemplate preMain		(
		///////////
		// TYPES //
		///////////

		// struct struct inner_struct {
		//   vec4 x[2]; // array of 2 vectors
		//   vec4 y[2]; // array of 2 vectors
		// };
		"%arr2_v4float			= OpTypeArray %v4f32 %c_u32_2						\n"
		"%inner_struct			= OpTypeStruct %arr2_v4float %arr2_v4float			\n"

		// struct outer_struct {
		//   inner_struct r[2][2];
		// };
		"%arr2_inner_struct		= OpTypeArray %inner_struct %c_u32_2				\n"
		"%mat2x2_inner_struct	= OpTypeArray %arr2_inner_struct %c_u32_2			\n"
		"%outer_struct			= OpTypeStruct %mat2x2_inner_struct					\n"

		// struct input_buffer {
		//   outer_struct a;
		//   outer_struct b;
		// }
		"%input_buffer			= OpTypeStruct %outer_struct %outer_struct			\n"

		///////////////////
		// POINTER TYPES //
		///////////////////
		"%input_buffer_ptr		= OpTypePointer StorageBuffer %input_buffer			\n"
		"%outer_struct_ptr		= OpTypePointer StorageBuffer %outer_struct			\n"
		"%mat2x2_ptr			= OpTypePointer StorageBuffer %mat2x2_inner_struct	\n"
		"%arr2_ptr				= OpTypePointer StorageBuffer %arr2_inner_struct	\n"
		"%inner_struct_ptr		= OpTypePointer StorageBuffer %inner_struct			\n"
		"%arr_v4f32_ptr			= OpTypePointer StorageBuffer %arr2_v4float			\n"
		"%v4f32_ptr				= OpTypePointer StorageBuffer %v4f32				\n"
		"%sb_f32ptr				= OpTypePointer StorageBuffer %f32					\n"

		///////////////
		// VARIABLES //
		///////////////
		"%input					= OpVariable %input_buffer_ptr StorageBuffer		\n"

		///////////////
		// CONSTANTS //
		///////////////
		"%c_bool_true			= OpConstantTrue %bool								\n"
		"%c_bool_false			= OpConstantFalse %bool								\n"

		//////////////////////
		// HELPER FUNCTIONS //
		//////////////////////
		"${helper_functions} \n"
	);

	const StringTemplate selectorFunctions	(
		// These selector functions return variable pointers.
		// These functions are used by tests that use OpFunctionCall to obtain the variable pointer
		"%selector_func_type	= OpTypeFunction ${selected_type} %bool ${selected_type} ${selected_type}\n"
		"%choose_input_func		= OpFunction ${selected_type} None %selector_func_type\n"
		"%choose_first_param	= OpFunctionParameter %bool\n"
		"%first_param			= OpFunctionParameter ${selected_type}\n"
		"%second_param			= OpFunctionParameter ${selected_type}\n"
		"%selector_func_begin	= OpLabel\n"
		"%result_ptr			= OpSelect ${selected_type} %choose_first_param %first_param %second_param\n"
		"OpReturnValue %result_ptr\n"
		"OpFunctionEnd\n"
	);

	const StringTemplate testFunction	(
		"%test_code		= OpFunction %v4f32 None %v4f32_function\n"
		"%param			= OpFunctionParameter %v4f32\n"
		"%entry			= OpLabel\n"

		// Here are the 2 nested structures:
		"%in_a			= OpAccessChain %outer_struct_ptr %input %c_i32_0\n"
		"%in_b			= OpAccessChain %outer_struct_ptr %input %c_i32_1\n"

		// Define the 2 pointers from which we're going to choose one.
		"${a_loc} \n"
		"${b_loc} \n"

		// Choose between the 2 pointers / variable pointers
		"${selection_strategy} \n"

		// OpAccessChain into the variable pointer until you get to the float.
		"%result_loc	= OpAccessChain %sb_f32ptr %var_ptr  ${remaining_indexes} \n"


		// Now load from the result_loc
		"%result_val	= OpLoad %f32 %result_loc\n"

		// Modify the 'RED channel' of the output color to the chosen value
		"%output_color	= OpCompositeInsert %v4f32 %result_val %param 0 \n"

		// Return and FunctionEnd
		"OpReturnValue %output_color\n"
		"OpFunctionEnd\n");

	// When select is 0, the variable pointer should point to a value in the first input_buffer member (a).
	// When select is 1, the variable pointer should point to a value in the second input_buffer member (b).
	// Since the 2 members of the input_buffer (a and b) are of type outer_struct, we can conveniently use
	// the same indexing scheme that we used for the 2-input-buffer tests.
	for (int selectInputA = 0; selectInputA < 2; ++selectInputA)
	{
		const string selectedInputStr	= selectInputA ? "first_input"	: "second_input";
		const string spirvSelectInputA	= selectInputA ? "%c_bool_true"	: "%c_bool_false";
		const int outerStructIndex		= selectInputA ? 0				: 1;

		// The indexes chosen at each level. At any level, any given offset is exercised.
		// outerStructIndex is 0 for member (a) and 1 for member (b).
		const int indexesForLevel[numLevels][6]= {{outerStructIndex, 0, 0, 0, 0, 1},
												  {outerStructIndex, 1, 0, 1, 0, 2},
												  {outerStructIndex, 0, 1, 0, 1, 3},
												  {outerStructIndex, 1, 1, 1, 0, 0},
												  {outerStructIndex, 0, 0, 1, 1, 1},
												  {outerStructIndex, 1, 0, 0, 0, 2},
												  {outerStructIndex, 1, 1, 1, 1, 3}};

		const string indexLevelNames[]		= {"_outer_struct_", "_matrices_", "_arrays_", "_inner_structs_", "_vec4arr_", "_vec4_", "_float_"};
		const string inputALocations[]		= {	"",
											"%a_loc = OpAccessChain %mat2x2_ptr       %in_a %c_i32_0",
											"%a_loc = OpAccessChain %arr2_ptr         %in_a %c_i32_0 %c_i32_0",
											"%a_loc = OpAccessChain %inner_struct_ptr %in_a %c_i32_0 %c_i32_1 %c_i32_1",
											"%a_loc = OpAccessChain %arr_v4f32_ptr    %in_a %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
											"%a_loc = OpAccessChain %v4f32_ptr        %in_a %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
											"%a_loc = OpAccessChain %sb_f32ptr        %in_a %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3"};

		const string inputBLocations[]		= {	"",
											"%b_loc = OpAccessChain %mat2x2_ptr       %in_b %c_i32_0",
											"%b_loc = OpAccessChain %arr2_ptr         %in_b %c_i32_0 %c_i32_0",
											"%b_loc = OpAccessChain %inner_struct_ptr %in_b %c_i32_0 %c_i32_1 %c_i32_1",
											"%b_loc = OpAccessChain %arr_v4f32_ptr    %in_b %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
											"%b_loc = OpAccessChain %v4f32_ptr        %in_b %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
											"%b_loc = OpAccessChain %sb_f32ptr        %in_b %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3"};

		const string inputAPtrAccessChain[]	= {	"",
											"%a_loc = OpPtrAccessChain %mat2x2_ptr       %in_a %c_i32_0 %c_i32_0",
											"%a_loc = OpPtrAccessChain %arr2_ptr         %in_a %c_i32_0 %c_i32_0 %c_i32_0",
											"%a_loc = OpPtrAccessChain %inner_struct_ptr %in_a %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1",
											"%a_loc = OpPtrAccessChain %arr_v4f32_ptr    %in_a %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
											"%a_loc = OpPtrAccessChain %v4f32_ptr        %in_a %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
											// Next case emulates:
											// %a_loc = OpPtrAccessChain %sb_f32ptr      %in_a %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3
											// But rewrite it to exercise OpPtrAccessChain with a non-zero first index:
											//    %a_loc_arr is a pointer to an array that we want to index with 1.
											// But instead of just using OpAccessChain with first index 1, use OpAccessChain with index 0 to
											// get a pointer to the first element, then send that into OpPtrAccessChain with index 1.
											"%a_loc_arr = OpPtrAccessChain %arr_v4f32_ptr %in_a %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 "
											"%a_loc_first_elem = OpAccessChain %v4f32_ptr %a_loc_arr %c_i32_0 "
											"%a_loc = OpPtrAccessChain %sb_f32ptr %a_loc_first_elem %c_i32_1 %c_i32_3"};

		const string inputBPtrAccessChain[]	= {	"",
											"%b_loc = OpPtrAccessChain %mat2x2_ptr       %in_b %c_i32_0 %c_i32_0",
											"%b_loc = OpPtrAccessChain %arr2_ptr         %in_b %c_i32_0 %c_i32_0 %c_i32_0",
											"%b_loc = OpPtrAccessChain %inner_struct_ptr %in_b %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1",
											"%b_loc = OpPtrAccessChain %arr_v4f32_ptr    %in_b %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
											"%b_loc = OpPtrAccessChain %v4f32_ptr        %in_b %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_0 %c_i32_0",
											// Next case emulates:
											// %b_loc = OpPtrAccessChain %sb_f32ptr      %in_b %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_1 %c_i32_3
											// But rewrite it to exercise OpPtrAccessChain with a non-zero first index:
											//    %b_loc_arr is a pointer to an array that we want to index with 1.
											// But instead of just using OpAccessChain with first index 1, use OpAccessChain with index 0 to
											// get a pointer to the first element, then send that into OpPtrAccessChain with index 1.
											"%b_loc_arr = OpPtrAccessChain %arr_v4f32_ptr %in_b %c_i32_0 %c_i32_0 %c_i32_1 %c_i32_1 %c_i32_1 "
											"%b_loc_first_elem = OpAccessChain %v4f32_ptr %b_loc_arr %c_i32_0 "
											"%b_loc = OpPtrAccessChain %sb_f32ptr %b_loc_first_elem %c_i32_1 %c_i32_3"};


		const string remainingIndexesAtLevel[]= {"%c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_0 %c_i32_1",
												 "%c_i32_1 %c_i32_0 %c_i32_1 %c_i32_0 %c_i32_2",
												 "%c_i32_1 %c_i32_0 %c_i32_1 %c_i32_3",
												 "%c_i32_1 %c_i32_0 %c_i32_0",
												 "%c_i32_1 %c_i32_1",
												 "%c_i32_2",
												 ""};

		const string pointerTypeAtLevel[] = {"%outer_struct_ptr", "%mat2x2_ptr", "%arr2_ptr", "%inner_struct_ptr", "%arr_v4f32_ptr", "%v4f32_ptr", "%sb_f32ptr"};
		const string baseANameAtLevel[]	  = {"%in_a", "%a_loc", "%a_loc", "%a_loc", "%a_loc", "%a_loc", "%a_loc"};
		const string baseBNameAtLevel[]	  = {"%in_b", "%b_loc", "%b_loc", "%b_loc", "%b_loc", "%b_loc", "%b_loc"};

		for (int indexLevel = 0; indexLevel < numLevels; ++indexLevel)
		{
			// Use OpSelect to choose between 2 pointers
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				string opCodeForTests			= "opselect";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				specs["select_inputA"]			= spirvSelectInputA;
				specs["helper_functions"]		= "";
				specs["a_loc"]					= inputALocations[indexLevel];
				specs["b_loc"]					= inputBLocations[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		= "%var_ptr	= OpSelect " +
													pointerTypeAtLevel[indexLevel] + " " +
													spirvSelectInputA + " " +
													baseANameAtLevel[indexLevel] + " " +
													baseBNameAtLevel[indexLevel] + "\n";
				baseOffset						= getBaseOffsetForSingleInputBuffer(indexesForLevel[indexLevel][0],
																					indexesForLevel[indexLevel][1],
																					indexesForLevel[indexLevel][2],
																					indexesForLevel[indexLevel][3],
																					indexesForLevel[indexLevel][4],
																					indexesForLevel[indexLevel][5]);
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBuffer))));
				getExpectedOutputColor(defaultColors, expectedColors, inputBuffer[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
			// Use OpCopyObject to get variable pointers
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				string opCodeForTests			= "opcopyobject";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				specs["select_inputA"]			= spirvSelectInputA;
				specs["helper_functions"]		= "";
				specs["a_loc"]					= inputALocations[indexLevel];
				specs["b_loc"]					= inputBLocations[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		=
									"%in_a_copy = OpCopyObject " + pointerTypeAtLevel[indexLevel] + " " + baseANameAtLevel[indexLevel] + "\n"
									"%in_b_copy = OpCopyObject " + pointerTypeAtLevel[indexLevel] + " " + baseBNameAtLevel[indexLevel] + "\n"
									"%var_ptr	= OpSelect " + pointerTypeAtLevel[indexLevel] + " " + spirvSelectInputA + " %in_a_copy %in_b_copy\n";
				baseOffset						= getBaseOffsetForSingleInputBuffer(indexesForLevel[indexLevel][0],
																					indexesForLevel[indexLevel][1],
																					indexesForLevel[indexLevel][2],
																					indexesForLevel[indexLevel][3],
																					indexesForLevel[indexLevel][4],
																					indexesForLevel[indexLevel][5]);
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBuffer))));
				getExpectedOutputColor(defaultColors, expectedColors, inputBuffer[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
			// Use OpPhi to choose between 2 pointers
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				string opCodeForTests			= "opphi";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				specs["select_inputA"]			= spirvSelectInputA;
				specs["helper_functions"]		= "";
				specs["a_loc"]					= inputALocations[indexLevel];
				specs["b_loc"]					= inputBLocations[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		=
								"				  OpSelectionMerge %end_label None\n"
								"				  OpBranchConditional " + spirvSelectInputA + " %take_input_a %take_input_b\n"
								"%take_input_a	= OpLabel\n"
								"				  OpBranch %end_label\n"
								"%take_input_b	= OpLabel\n"
								"			      OpBranch %end_label\n"
								"%end_label		= OpLabel\n"
								"%var_ptr		= OpPhi "
													+ pointerTypeAtLevel[indexLevel] + " "
													+ baseANameAtLevel[indexLevel]
													+ " %take_input_a "
													+ baseBNameAtLevel[indexLevel]
													+ " %take_input_b\n";
				baseOffset						= getBaseOffsetForSingleInputBuffer(indexesForLevel[indexLevel][0],
																					indexesForLevel[indexLevel][1],
																					indexesForLevel[indexLevel][2],
																					indexesForLevel[indexLevel][3],
																					indexesForLevel[indexLevel][4],
																					indexesForLevel[indexLevel][5]);
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBuffer))));
				getExpectedOutputColor(defaultColors, expectedColors, inputBuffer[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
			// Use OpFunctionCall to choose between 2 pointers
			{
				GraphicsResources				resources;
				map<string, string>				functionSpecs;
				map<string, string>				specs;
				string opCodeForTests			= "opfunctioncall";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				//string selectedType				= "%mat2x2_ptr";
				functionSpecs["selected_type"]	= pointerTypeAtLevel[indexLevel];
				specs["helper_functions"]		= selectorFunctions.specialize(functionSpecs);
				specs["select_inputA"]			= spirvSelectInputA;
				specs["a_loc"]					= inputALocations[indexLevel];
				specs["b_loc"]					= inputBLocations[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		= "%var_ptr	= OpFunctionCall "
													+ pointerTypeAtLevel[indexLevel]
													+ " %choose_input_func "
													+ spirvSelectInputA + " "
													+ baseANameAtLevel[indexLevel] + " "
													+ baseBNameAtLevel[indexLevel] + "\n";
				baseOffset						= getBaseOffsetForSingleInputBuffer(indexesForLevel[indexLevel][0],
																					indexesForLevel[indexLevel][1],
																					indexesForLevel[indexLevel][2],
																					indexesForLevel[indexLevel][3],
																					indexesForLevel[indexLevel][4],
																					indexesForLevel[indexLevel][5]);
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBuffer))));
				getExpectedOutputColor(defaultColors, expectedColors, inputBuffer[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
			// Use OpPtrAccessChain to get variable pointers
			{
				GraphicsResources				resources;
				map<string, string>				specs;
				string opCodeForTests			= "opptraccesschain";
				string name						= opCodeForTests + indexLevelNames[indexLevel] + selectedInputStr;
				specs["select_inputA"]			= spirvSelectInputA;
				specs["helper_functions"]		= "";
				specs["a_loc"]					= inputAPtrAccessChain[indexLevel];
				specs["b_loc"]					= inputBPtrAccessChain[indexLevel];
				specs["remaining_indexes"]		= remainingIndexesAtLevel[indexLevel];
				specs["selection_strategy"]		= "%var_ptr	= OpSelect "
													+ pointerTypeAtLevel[indexLevel] + " "
													+ spirvSelectInputA + " "
													+ baseANameAtLevel[indexLevel] + " "
													+ baseBNameAtLevel[indexLevel] + "\n";
				baseOffset						= getBaseOffsetForSingleInputBuffer(indexesForLevel[indexLevel][0],
																					indexesForLevel[indexLevel][1],
																					indexesForLevel[indexLevel][2],
																					indexesForLevel[indexLevel][3],
																					indexesForLevel[indexLevel][4],
																					indexesForLevel[indexLevel][5]);
				fragments["decoration"]			= decoration.specialize(specs);
				fragments["pre_main"]			= preMain.specialize(specs);
				fragments["testfun"]			= testFunction.specialize(specs);
				resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(inputBuffer))));
				getExpectedOutputColor(defaultColors, expectedColors, inputBuffer[baseOffset]);
				createTestsForAllStages(name.c_str(), defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
			}
		}
	}
}

void addNullptrVariablePointersGraphicsGroup (tcu::TestCaseGroup* testGroup)
{
	float							someFloat				= 78 / 255.f;
	vector<float>					input					(1, someFloat);
	vector<float>					expectedOutput			(1, someFloat);
	VulkanFeatures					requiredFeatures;
	map<string, string>				fragments;
	RGBA							defaultColors[4];
	RGBA							expectedColors[4];
	vector<string>					extensions;

	getDefaultColors(defaultColors);
	getExpectedOutputColor(defaultColors, expectedColors, someFloat);

	// Set the required extension.
	extensions.push_back("VK_KHR_variable_pointers");

	// Requires the variable pointers feature.
	requiredFeatures.extVariablePointers	= EXTVARIABLEPOINTERSFEATURES_VARIABLE_POINTERS;

	fragments["capability"]		=	"OpCapability VariablePointers							\n";
	fragments["extension"]		=	"OpExtension \"SPV_KHR_variable_pointers\"				\n"
									"OpExtension \"SPV_KHR_storage_buffer_storage_class\"	\n";
	const StringTemplate decoration		(
		// Decorations
		"OpDecorate %input DescriptorSet 0				\n"
		"OpDecorate %input Binding 0					\n"

		// Set the Block decoration
		"OpDecorate %float_struct	Block				\n"

		// Set the Offsets
		"OpMemberDecorate %float_struct 0 Offset 0		\n"
	);

	const StringTemplate preMain		(
		// struct float_struct {
		//   float x;
		// };
		"%float_struct		= OpTypeStruct %f32											\n"

		// POINTER TYPES
		"%float_struct_ptr	= OpTypePointer StorageBuffer %float_struct					\n"
		"%sb_f32ptr			= OpTypePointer StorageBuffer %f32							\n"
		"%func_f32ptrptr	= OpTypePointer Function %sb_f32ptr							\n"

		// CONSTANTS
		"%c_bool_true		= OpConstantTrue	%bool									\n"
		"%c_null_ptr		= OpConstantNull	%sb_f32ptr								\n"

		// VARIABLES
		"%input				= OpVariable %float_struct_ptr	StorageBuffer				\n"
	);

	const StringTemplate testFunction	(
		"%test_code		= OpFunction %v4f32 None %v4f32_function\n"
		"%param			= OpFunctionParameter %v4f32\n"
		"%entry			= OpLabel\n"

		// Note that the Variable Pointers extension allows creation
		// of a pointer variable with storage class of Private or Function.
		"%f32_ptr_var	= OpVariable %func_f32ptrptr Function %c_null_ptr\n"

		"%input_loc		= OpAccessChain %sb_f32ptr %input %c_i32_0\n"

		// Null testing strategy
		"${NullptrTestingStrategy}\n"
		// Modify the 'RED channel' of the output color to the chosen value
		"%output_color	= OpCompositeInsert %v4f32 %result_val %param 0 \n"
		// Return and FunctionEnd
		"OpReturnValue %output_color\n"
		"OpFunctionEnd\n");

	// f32_ptr_var has been inintialized to NULL.
	// Now set it to the input variable and return it as output
	{
		GraphicsResources				resources;
		map<string, string>				specs;
		specs["NullptrTestingStrategy"]	=
							"                  OpStore %f32_ptr_var %input_loc      \n"
							"%loaded_f32_ptr = OpLoad  %sb_f32ptr   %f32_ptr_var    \n"
							"%result_val     = OpLoad  %f32         %loaded_f32_ptr \n";
		fragments["decoration"]			= decoration.specialize(specs);
		fragments["pre_main"]			= preMain.specialize(specs);
		fragments["testfun"]			= testFunction.specialize(specs);
		resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(input))));
		createTestsForAllStages("opvariable_initialized_null", defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
	}
	// Use OpSelect to choose between nullptr and a valid pointer. Since we can't dereference nullptr,
	// it is forced to always choose the valid pointer.
	{
		GraphicsResources				resources;
		map<string, string>				specs;
		specs["NullptrTestingStrategy"]	= "%selected_ptr  = OpSelect %sb_f32ptr %c_bool_true %input_loc %c_null_ptr\n"
										  "%result_val    = OpLoad %f32 %selected_ptr\n";
		fragments["decoration"]			= decoration.specialize(specs);
		fragments["pre_main"]			= preMain.specialize(specs);
		fragments["testfun"]			= testFunction.specialize(specs);
		resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(input))));
		createTestsForAllStages("opselect_null_or_valid_ptr", defaultColors, expectedColors, fragments, resources, extensions, testGroup, requiredFeatures);
	}
}

} // anonymous

tcu::TestCaseGroup* createVariablePointersComputeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group	(new tcu::TestCaseGroup(testCtx, "variable_pointers", "Compute tests for SPV_KHR_variable_pointers extension"));
	addTestGroup(group.get(), "compute", "Test the variable pointer extension using a compute shader", addVariablePointersComputeGroup);
	addTestGroup(group.get(),
				 "complex_types_compute",
				 "Testing Variable Pointers pointing to various types in different input buffers",
				 addComplexTypesVariablePointersComputeGroup);
	addTestGroup(group.get(),
				 "nullptr_compute",
				 "Test the usage of nullptr using the variable pointers extension in a compute shader",
				 addNullptrVariablePointersComputeGroup);

	return group.release();
}

tcu::TestCaseGroup* createVariablePointersGraphicsGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group	(new tcu::TestCaseGroup(testCtx, "variable_pointers", "Graphics tests for SPV_KHR_variable_pointers extension"));

	addTestGroup(group.get(), "graphics", "Testing Variable Pointers in graphics pipeline", addVariablePointersGraphicsGroup);
	addTestGroup(group.get(),
				 "multi_buffer_read_only_graphics",
				 "Testing Variable Pointers pointing to different input buffers in graphics pipeline (no SSBO writes)",
				 addTwoInputBufferReadOnlyVariablePointersGraphicsGroup);
	addTestGroup(group.get(),
				 "single_buffer_read_only_graphics",
				 "Testing Variable Pointers confined to a single input buffer in graphics pipeline (no SSBO writes)",
				 addSingleInputBufferReadOnlyVariablePointersGraphicsGroup);
	addTestGroup(group.get(),
				 "nullptr_graphics",
				 "Test the usage of nullptr using the variable pointers extension in graphics pipeline",
				 addNullptrVariablePointersGraphicsGroup);

	return group.release();
}

} // SpirVAssembly
} // vkt
