/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief SPIR-V Assembly Tests for Instructions (special opcode/operand)
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmInstructionTests.hpp"

#include "tcuCommandLine.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuFloat.hpp"
#include "tcuRGBA.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuInterval.hpp"

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

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deMath.h"
#include "tcuStringTemplate.hpp"

#include "vktSpvAsm16bitStorageTests.hpp"
#include "vktSpvAsmUboMatrixPaddingTests.hpp"
#include "vktSpvAsmConditionalBranchTests.hpp"
#include "vktSpvAsmIndexingTests.hpp"
#include "vktSpvAsmImageSamplerTests.hpp"
#include "vktSpvAsmComputeShaderCase.hpp"
#include "vktSpvAsmComputeShaderTestUtil.hpp"
#include "vktSpvAsmGraphicsShaderTestUtil.hpp"
#include "vktSpvAsmVariablePointersTests.hpp"
#include "vktTestCaseUtil.hpp"

#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <stack>

namespace vkt
{
namespace SpirVAssembly
{

namespace
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

template<typename T>
static void fillRandomScalars (de::Random& rnd, T minValue, T maxValue, void* dst, int numValues, int offset = 0)
{
	T* const typedPtr = (T*)dst;
	for (int ndx = 0; ndx < numValues; ndx++)
		typedPtr[offset + ndx] = randomScalar<T>(rnd, minValue, maxValue);
}

// Filter is a function that returns true if a value should pass, false otherwise.
template<typename T, typename FilterT>
static void fillRandomScalars (de::Random& rnd, T minValue, T maxValue, void* dst, int numValues, FilterT filter, int offset = 0)
{
	T* const typedPtr = (T*)dst;
	T value;
	for (int ndx = 0; ndx < numValues; ndx++)
	{
		do
			value = randomScalar<T>(rnd, minValue, maxValue);
		while (!filter(value));

		typedPtr[offset + ndx] = value;
	}
}

// Gets a 64-bit integer with a more logarithmic distribution
deInt64 randomInt64LogDistributed (de::Random& rnd)
{
	deInt64 val = rnd.getUint64();
	val &= (1ull << rnd.getInt(1, 63)) - 1;
	if (rnd.getBool())
		val = -val;
	return val;
}

static void fillRandomInt64sLogDistributed (de::Random& rnd, vector<deInt64>& dst, int numValues)
{
	for (int ndx = 0; ndx < numValues; ndx++)
		dst[ndx] = randomInt64LogDistributed(rnd);
}

template<typename FilterT>
static void fillRandomInt64sLogDistributed (de::Random& rnd, vector<deInt64>& dst, int numValues, FilterT filter)
{
	for (int ndx = 0; ndx < numValues; ndx++)
	{
		deInt64 value;
		do {
			value = randomInt64LogDistributed(rnd);
		} while (!filter(value));
		dst[ndx] = value;
	}
}

inline bool filterNonNegative (const deInt64 value)
{
	return value >= 0;
}

inline bool filterPositive (const deInt64 value)
{
	return value > 0;
}

inline bool filterNotZero (const deInt64 value)
{
	return value != 0;
}

static void floorAll (vector<float>& values)
{
	for (size_t i = 0; i < values.size(); i++)
		values[i] = deFloatFloor(values[i]);
}

static void floorAll (vector<Vec4>& values)
{
	for (size_t i = 0; i < values.size(); i++)
		values[i] = floor(values[i]);
}

struct CaseParameter
{
	const char*		name;
	string			param;

	CaseParameter	(const char* case_, const string& param_) : name(case_), param(param_) {}
};

// Assembly code used for testing LocalSize, OpNop, OpConstant{Null|Composite}, Op[No]Line, OpSource[Continued], OpSourceExtension, OpUndef is based on GLSL source code:
//
// #version 430
//
// layout(std140, set = 0, binding = 0) readonly buffer Input {
//   float elements[];
// } input_data;
// layout(std140, set = 0, binding = 1) writeonly buffer Output {
//   float elements[];
// } output_data;
//
// layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
//
// void main() {
//   uint x = gl_GlobalInvocationID.x;
//   output_data.elements[x] = -input_data.elements[x];
// }

static string getAsmForLocalSizeTest(bool useLiteralLocalSize, bool useSpecConstantWorkgroupSize, IVec3 workGroupSize, deUint32 ndx)
{
	std::ostringstream out;
	out << getComputeAsmShaderPreambleWithoutLocalSize();

	if (useLiteralLocalSize)
	{
		out << "OpExecutionMode %main LocalSize "
			<< workGroupSize.x() << " " << workGroupSize.y() << " " << workGroupSize.z() << "\n";
	}

	out << "OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"
		"OpDecorate %id BuiltIn GlobalInvocationId\n";

	if (useSpecConstantWorkgroupSize)
	{
		out << "OpDecorate %spec_0 SpecId 100\n"
			<< "OpDecorate %spec_1 SpecId 101\n"
			<< "OpDecorate %spec_2 SpecId 102\n"
			<< "OpDecorate %gl_WorkGroupSize BuiltIn WorkgroupSize\n";
	}

	out << getComputeAsmInputOutputBufferTraits()
		<< getComputeAsmCommonTypes()
		<< getComputeAsmInputOutputBuffer()
		<< "%id        = OpVariable %uvec3ptr Input\n"
		<< "%zero      = OpConstant %i32 0 \n";

	if (useSpecConstantWorkgroupSize)
	{
		out	<< "%spec_0   = OpSpecConstant %u32 "<< workGroupSize.x() << "\n"
			<< "%spec_1   = OpSpecConstant %u32 "<< workGroupSize.y() << "\n"
			<< "%spec_2   = OpSpecConstant %u32 "<< workGroupSize.z() << "\n"
			<< "%gl_WorkGroupSize = OpSpecConstantComposite %uvec3 %spec_0 %spec_1 %spec_2\n";
	}

	out << "%main      = OpFunction %void None %voidf\n"
		<< "%label     = OpLabel\n"
		<< "%idval     = OpLoad %uvec3 %id\n"
		<< "%ndx         = OpCompositeExtract %u32 %idval " << ndx << "\n"

			"%inloc     = OpAccessChain %f32ptr %indata %zero %ndx\n"
			"%inval     = OpLoad %f32 %inloc\n"
			"%neg       = OpFNegate %f32 %inval\n"
			"%outloc    = OpAccessChain %f32ptr %outdata %zero %ndx\n"
			"             OpStore %outloc %neg\n"
			"             OpReturn\n"
			"             OpFunctionEnd\n";
	return out.str();
}

tcu::TestCaseGroup* createLocalSizeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "localsize", ""));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const deUint32					numElements		= 64u;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));

	spec.numWorkGroups = IVec3(numElements, 1, 1);

	spec.assembly = getAsmForLocalSizeTest(true, false, IVec3(1, 1, 1), 0u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "literal_localsize", "", spec));

	spec.assembly = getAsmForLocalSizeTest(true, true, IVec3(1, 1, 1), 0u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "literal_and_specid_localsize", "", spec));

	spec.assembly = getAsmForLocalSizeTest(false, true, IVec3(1, 1, 1), 0u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "specid_localsize", "", spec));

	spec.numWorkGroups = IVec3(1, 1, 1);

	spec.assembly = getAsmForLocalSizeTest(true, false, IVec3(numElements, 1, 1), 0u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "literal_localsize_x", "", spec));

	spec.assembly = getAsmForLocalSizeTest(true, true, IVec3(numElements, 1, 1), 0u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "literal_and_specid_localsize_x", "", spec));

	spec.assembly = getAsmForLocalSizeTest(false, true, IVec3(numElements, 1, 1), 0u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "specid_localsize_x", "", spec));

	spec.assembly = getAsmForLocalSizeTest(true, false, IVec3(1, numElements, 1), 1u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "literal_localsize_y", "", spec));

	spec.assembly = getAsmForLocalSizeTest(true, true, IVec3(1, numElements, 1), 1u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "literal_and_specid_localsize_y", "", spec));

	spec.assembly = getAsmForLocalSizeTest(false, true, IVec3(1, numElements, 1), 1u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "specid_localsize_y", "", spec));

	spec.assembly = getAsmForLocalSizeTest(true, false, IVec3(1, 1, numElements), 2u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "literal_localsize_z", "", spec));

	spec.assembly = getAsmForLocalSizeTest(true, true, IVec3(1, 1, numElements), 2u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "literal_and_specid_localsize_z", "", spec));

	spec.assembly = getAsmForLocalSizeTest(false, true, IVec3(1, 1, numElements), 2u);
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "specid_localsize_z", "", spec));

	return group.release();
}

tcu::TestCaseGroup* createOpNopGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opnop", "Test the OpNop instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes())

		+ string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"

		"             OpNop\n" // Inside a function body

		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "OpNop appearing at different places", spec));

	return group.release();
}

bool compareFUnord (const std::vector<BufferSp>& inputs, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog& log)
{
	if (outputAllocs.size() != 1)
		return false;

	vector<deUint8>	input1Bytes;
	vector<deUint8>	input2Bytes;
	vector<deUint8>	expectedBytes;

	inputs[0]->getBytes(input1Bytes);
	inputs[1]->getBytes(input2Bytes);
	expectedOutputs[0]->getBytes(expectedBytes);

	const deInt32* const	expectedOutputAsInt		= reinterpret_cast<const deInt32* const>(&expectedBytes.front());
	const deInt32* const	outputAsInt				= static_cast<const deInt32* const>(outputAllocs[0]->getHostPtr());
	const float* const		input1AsFloat			= reinterpret_cast<const float* const>(&input1Bytes.front());
	const float* const		input2AsFloat			= reinterpret_cast<const float* const>(&input2Bytes.front());
	bool returnValue								= true;

	for (size_t idx = 0; idx < expectedBytes.size() / sizeof(deInt32); ++idx)
	{
		if (outputAsInt[idx] != expectedOutputAsInt[idx])
		{
			log << TestLog::Message << "ERROR: Sub-case failed. inputs: " << input1AsFloat[idx] << "," << input2AsFloat[idx] << " output: " << outputAsInt[idx]<< " expected output: " << expectedOutputAsInt[idx] << TestLog::EndMessage;
			returnValue = false;
		}
	}
	return returnValue;
}

typedef VkBool32 (*compareFuncType) (float, float);

struct OpFUnordCase
{
	const char*		name;
	const char*		opCode;
	compareFuncType	compareFunc;

					OpFUnordCase			(const char* _name, const char* _opCode, compareFuncType _compareFunc)
						: name				(_name)
						, opCode			(_opCode)
						, compareFunc		(_compareFunc) {}
};

#define ADD_OPFUNORD_CASE(NAME, OPCODE, OPERATOR) \
do { \
    struct compare_##NAME { static VkBool32 compare(float x, float y) { return (x OPERATOR y) ? VK_TRUE : VK_FALSE; } }; \
    cases.push_back(OpFUnordCase(#NAME, OPCODE, compare_##NAME::compare)); \
} while (deGetFalse())

tcu::TestCaseGroup* createOpFUnordGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opfunord", "Test the OpFUnord* opcodes"));
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<OpFUnordCase>			cases;

	const StringTemplate			shaderTemplate	(

		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"OpDecorate %buf BufferBlock\n"
		"OpDecorate %buf2 BufferBlock\n"
		"OpDecorate %indata1 DescriptorSet 0\n"
		"OpDecorate %indata1 Binding 0\n"
		"OpDecorate %indata2 DescriptorSet 0\n"
		"OpDecorate %indata2 Binding 1\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 2\n"
		"OpDecorate %f32arr ArrayStride 4\n"
		"OpDecorate %i32arr ArrayStride 4\n"
		"OpMemberDecorate %buf 0 Offset 0\n"
		"OpMemberDecorate %buf2 0 Offset 0\n"

		+ string(getComputeAsmCommonTypes()) +

		"%buf        = OpTypeStruct %f32arr\n"
		"%bufptr     = OpTypePointer Uniform %buf\n"
		"%indata1    = OpVariable %bufptr Uniform\n"
		"%indata2    = OpVariable %bufptr Uniform\n"

		"%buf2       = OpTypeStruct %i32arr\n"
		"%buf2ptr    = OpTypePointer Uniform %buf2\n"
		"%outdata    = OpVariable %buf2ptr Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"
		"%consti1   = OpConstant %i32 1\n"
		"%constf1   = OpConstant %f32 1.0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"

		"%inloc1    = OpAccessChain %f32ptr %indata1 %zero %x\n"
		"%inval1    = OpLoad %f32 %inloc1\n"
		"%inloc2    = OpAccessChain %f32ptr %indata2 %zero %x\n"
		"%inval2    = OpLoad %f32 %inloc2\n"
		"%outloc    = OpAccessChain %i32ptr %outdata %zero %x\n"

		"%result    = ${OPCODE} %bool %inval1 %inval2\n"
		"%int_res   = OpSelect %i32 %result %consti1 %zero\n"
		"             OpStore %outloc %int_res\n"

		"             OpReturn\n"
		"             OpFunctionEnd\n");

	ADD_OPFUNORD_CASE(equal, "OpFUnordEqual", ==);
	ADD_OPFUNORD_CASE(less, "OpFUnordLessThan", <);
	ADD_OPFUNORD_CASE(lessequal, "OpFUnordLessThanEqual", <=);
	ADD_OPFUNORD_CASE(greater, "OpFUnordGreaterThan", >);
	ADD_OPFUNORD_CASE(greaterequal, "OpFUnordGreaterThanEqual", >=);
	ADD_OPFUNORD_CASE(notequal, "OpFUnordNotEqual", !=);

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>			specializations;
		ComputeShaderSpec			spec;
		const float					NaN				= std::numeric_limits<float>::quiet_NaN();
		vector<float>				inputFloats1	(numElements, 0);
		vector<float>				inputFloats2	(numElements, 0);
		vector<deInt32>				expectedInts	(numElements, 0);

		specializations["OPCODE"]	= cases[caseNdx].opCode;
		spec.assembly				= shaderTemplate.specialize(specializations);

		fillRandomScalars(rnd, 1.f, 100.f, &inputFloats1[0], numElements);
		for (size_t ndx = 0; ndx < numElements; ++ndx)
		{
			switch (ndx % 6)
			{
				case 0:		inputFloats2[ndx] = inputFloats1[ndx] + 1.0f; break;
				case 1:		inputFloats2[ndx] = inputFloats1[ndx] - 1.0f; break;
				case 2:		inputFloats2[ndx] = inputFloats1[ndx]; break;
				case 3:		inputFloats2[ndx] = NaN; break;
				case 4:		inputFloats2[ndx] = inputFloats1[ndx];	inputFloats1[ndx] = NaN; break;
				case 5:		inputFloats2[ndx] = NaN;				inputFloats1[ndx] = NaN; break;
			}
			expectedInts[ndx] = tcu::Float32(inputFloats1[ndx]).isNaN() || tcu::Float32(inputFloats2[ndx]).isNaN() || cases[caseNdx].compareFunc(inputFloats1[ndx], inputFloats2[ndx]);
		}

		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats1)));
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats2)));
		spec.outputs.push_back(BufferSp(new Int32Buffer(expectedInts)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);
		spec.verifyIO = &compareFUnord;
		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

struct OpAtomicCase
{
	const char*		name;
	const char*		assembly;
	OpAtomicType	opAtomic;
	deInt32			numOutputElements;

					OpAtomicCase			(const char* _name, const char* _assembly, OpAtomicType _opAtomic, deInt32 _numOutputElements)
						: name				(_name)
						, assembly			(_assembly)
						, opAtomic			(_opAtomic)
						, numOutputElements	(_numOutputElements) {}
};

tcu::TestCaseGroup* createOpAtomicGroup (tcu::TestContext& testCtx, bool useStorageBuffer)
{
	de::MovePtr<tcu::TestCaseGroup>	group				(new tcu::TestCaseGroup(testCtx,
																				useStorageBuffer ? "opatomic_storage_buffer" : "opatomic",
																				"Test the OpAtomic* opcodes"));
	const int						numElements			= 65535;
	vector<OpAtomicCase>			cases;

	const StringTemplate			shaderTemplate	(

		string("OpCapability Shader\n") +
		(useStorageBuffer ? "OpExtension \"SPV_KHR_storage_buffer_storage_class\"\n" : "") +
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n" +

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"OpDecorate %buf ${BLOCK_DECORATION}\n"
		"OpDecorate %indata DescriptorSet 0\n"
		"OpDecorate %indata Binding 0\n"
		"OpDecorate %i32arr ArrayStride 4\n"
		"OpMemberDecorate %buf 0 Offset 0\n"

		"OpDecorate %sumbuf ${BLOCK_DECORATION}\n"
		"OpDecorate %sum DescriptorSet 0\n"
		"OpDecorate %sum Binding 1\n"
		"OpMemberDecorate %sumbuf 0 Coherent\n"
		"OpMemberDecorate %sumbuf 0 Offset 0\n"

		+ getComputeAsmCommonTypes("${BLOCK_POINTER_TYPE}") +

		"%buf       = OpTypeStruct %i32arr\n"
		"%bufptr    = OpTypePointer ${BLOCK_POINTER_TYPE} %buf\n"
		"%indata    = OpVariable %bufptr ${BLOCK_POINTER_TYPE}\n"

		"%sumbuf    = OpTypeStruct %i32arr\n"
		"%sumbufptr = OpTypePointer ${BLOCK_POINTER_TYPE} %sumbuf\n"
		"%sum       = OpVariable %sumbufptr ${BLOCK_POINTER_TYPE}\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%minusone  = OpConstant %i32 -1\n"
		"%zero      = OpConstant %i32 0\n"
		"%one       = OpConstant %u32 1\n"
		"%two       = OpConstant %i32 2\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"

		"%inloc     = OpAccessChain %i32ptr %indata %zero %x\n"
		"%inval     = OpLoad %i32 %inloc\n"

		"%outloc    = OpAccessChain %i32ptr %sum %zero ${INDEX}\n"
		"${INSTRUCTION}"

		"             OpReturn\n"
		"             OpFunctionEnd\n");

	#define ADD_OPATOMIC_CASE(NAME, ASSEMBLY, OPATOMIC, NUM_OUTPUT_ELEMENTS) \
	do { \
		DE_STATIC_ASSERT((NUM_OUTPUT_ELEMENTS) == 1 || (NUM_OUTPUT_ELEMENTS) == numElements); \
		cases.push_back(OpAtomicCase(#NAME, ASSEMBLY, OPATOMIC, NUM_OUTPUT_ELEMENTS)); \
	} while (deGetFalse())
	#define ADD_OPATOMIC_CASE_1(NAME, ASSEMBLY, OPATOMIC) ADD_OPATOMIC_CASE(NAME, ASSEMBLY, OPATOMIC, 1)
	#define ADD_OPATOMIC_CASE_N(NAME, ASSEMBLY, OPATOMIC) ADD_OPATOMIC_CASE(NAME, ASSEMBLY, OPATOMIC, numElements)

	ADD_OPATOMIC_CASE_1(iadd,	"%unused    = OpAtomicIAdd %i32 %outloc %one %zero %inval\n", OPATOMIC_IADD );
	ADD_OPATOMIC_CASE_1(isub,	"%unused    = OpAtomicISub %i32 %outloc %one %zero %inval\n", OPATOMIC_ISUB );
	ADD_OPATOMIC_CASE_1(iinc,	"%unused    = OpAtomicIIncrement %i32 %outloc %one %zero\n",  OPATOMIC_IINC );
	ADD_OPATOMIC_CASE_1(idec,	"%unused    = OpAtomicIDecrement %i32 %outloc %one %zero\n",  OPATOMIC_IDEC );
	ADD_OPATOMIC_CASE_N(load,	"%inval2    = OpAtomicLoad %i32 %inloc %zero %zero\n"
								"             OpStore %outloc %inval2\n",  OPATOMIC_LOAD );
	ADD_OPATOMIC_CASE_N(store,	"             OpAtomicStore %outloc %zero %zero %inval\n",  OPATOMIC_STORE );
	ADD_OPATOMIC_CASE_N(compex, "%even      = OpSMod %i32 %inval %two\n"
								"             OpStore %outloc %even\n"
								"%unused    = OpAtomicCompareExchange %i32 %outloc %one %zero %zero %minusone %zero\n",  OPATOMIC_COMPEX );

	#undef ADD_OPATOMIC_CASE
	#undef ADD_OPATOMIC_CASE_1
	#undef ADD_OPATOMIC_CASE_N

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>			specializations;
		ComputeShaderSpec			spec;
		vector<deInt32>				inputInts		(numElements, 0);
		vector<deInt32>				expected		(cases[caseNdx].numOutputElements, -1);

		specializations["INDEX"]				= (cases[caseNdx].numOutputElements == 1) ? "%zero" : "%x";
		specializations["INSTRUCTION"]			= cases[caseNdx].assembly;
		specializations["BLOCK_DECORATION"]		= useStorageBuffer ? "Block" : "BufferBlock";
		specializations["BLOCK_POINTER_TYPE"]	= useStorageBuffer ? "StorageBuffer" : "Uniform";
		spec.assembly							= shaderTemplate.specialize(specializations);

		if (useStorageBuffer)
			spec.extensions.push_back("VK_KHR_storage_buffer_storage_class");

		spec.inputs.push_back(BufferSp(new OpAtomicBuffer(numElements, cases[caseNdx].numOutputElements, cases[caseNdx].opAtomic, BUFFERTYPE_INPUT)));
		spec.outputs.push_back(BufferSp(new OpAtomicBuffer(numElements, cases[caseNdx].numOutputElements, cases[caseNdx].opAtomic, BUFFERTYPE_EXPECTED)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);
		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

tcu::TestCaseGroup* createOpLineGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opline", "Test the OpLine instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"%fname1 = OpString \"negateInputs.comp\"\n"
		"%fname2 = OpString \"negateInputs\"\n"

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) +

		"OpLine %fname1 0 0\n" // At the earliest possible position

		+ string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"OpLine %fname1 0 1\n" // Multiple OpLines in sequence
		"OpLine %fname2 1 0\n" // Different filenames
		"OpLine %fname1 1000 100000\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"OpLine %fname1 1 1\n" // Before a function

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"

		"OpLine %fname1 1 1\n" // In a function

		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "OpLine appearing at different places", spec));

	return group.release();
}

tcu::TestCaseGroup* createOpNoLineGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opnoline", "Test the OpNoLine instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"%fname = OpString \"negateInputs.comp\"\n"

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) +

		"OpNoLine\n" // At the earliest possible position, without preceding OpLine

		+ string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"OpLine %fname 0 1\n"
		"OpNoLine\n" // Immediately following a preceding OpLine

		"OpLine %fname 1000 1\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"OpNoLine\n" // Contents after the previous OpLine

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"

		"OpNoLine\n" // Multiple OpNoLine
		"OpNoLine\n"
		"OpNoLine\n"

		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "OpNoLine appearing at different places", spec));

	return group.release();
}

// Compare instruction for the contraction compute case.
// Returns true if the output is what is expected from the test case.
bool compareNoContractCase(const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog&)
{
	if (outputAllocs.size() != 1)
		return false;

	// Only size is needed because we are not comparing the exact values.
	size_t byteSize = expectedOutputs[0]->getByteSize();

	const float*	outputAsFloat	= static_cast<const float*>(outputAllocs[0]->getHostPtr());

	for(size_t i = 0; i < byteSize / sizeof(float); ++i) {
		if (outputAsFloat[i] != 0.f &&
			outputAsFloat[i] != -ldexp(1, -24)) {
			return false;
		}
	}

	return true;
}

tcu::TestCaseGroup* createNoContractionGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "nocontraction", "Test the NoContraction decoration"));
	vector<CaseParameter>			cases;
	const int						numElements		= 100;
	vector<float>					inputFloats1	(numElements, 0);
	vector<float>					inputFloats2	(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"${DECORATION}\n"

		"OpDecorate %buf BufferBlock\n"
		"OpDecorate %indata1 DescriptorSet 0\n"
		"OpDecorate %indata1 Binding 0\n"
		"OpDecorate %indata2 DescriptorSet 0\n"
		"OpDecorate %indata2 Binding 1\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 2\n"
		"OpDecorate %f32arr ArrayStride 4\n"
		"OpMemberDecorate %buf 0 Offset 0\n"

		+ string(getComputeAsmCommonTypes()) +

		"%buf        = OpTypeStruct %f32arr\n"
		"%bufptr     = OpTypePointer Uniform %buf\n"
		"%indata1    = OpVariable %bufptr Uniform\n"
		"%indata2    = OpVariable %bufptr Uniform\n"
		"%outdata    = OpVariable %bufptr Uniform\n"

		"%id         = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%c_f_m1     = OpConstant %f32 -1.\n"

		"%main       = OpFunction %void None %voidf\n"
		"%label      = OpLabel\n"
		"%idval      = OpLoad %uvec3 %id\n"
		"%x          = OpCompositeExtract %u32 %idval 0\n"
		"%inloc1     = OpAccessChain %f32ptr %indata1 %zero %x\n"
		"%inval1     = OpLoad %f32 %inloc1\n"
		"%inloc2     = OpAccessChain %f32ptr %indata2 %zero %x\n"
		"%inval2     = OpLoad %f32 %inloc2\n"
		"%mul        = OpFMul %f32 %inval1 %inval2\n"
		"%add        = OpFAdd %f32 %mul %c_f_m1\n"
		"%outloc     = OpAccessChain %f32ptr %outdata %zero %x\n"
		"              OpStore %outloc %add\n"
		"              OpReturn\n"
		"              OpFunctionEnd\n");

	cases.push_back(CaseParameter("multiplication",	"OpDecorate %mul NoContraction"));
	cases.push_back(CaseParameter("addition",		"OpDecorate %add NoContraction"));
	cases.push_back(CaseParameter("both",			"OpDecorate %mul NoContraction\nOpDecorate %add NoContraction"));

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		inputFloats1[ndx]	= 1.f + std::ldexp(1.f, -23); // 1 + 2^-23.
		inputFloats2[ndx]	= 1.f - std::ldexp(1.f, -23); // 1 - 2^-23.
		// Result for (1 + 2^-23) * (1 - 2^-23) - 1. With NoContraction, the multiplication will be
		// conducted separately and the result is rounded to 1, or 0x1.fffffcp-1
		// So the final result will be 0.f or 0x1p-24.
		// If the operation is combined into a precise fused multiply-add, then the result would be
		// 2^-46 (0xa8800000).
		outputFloats[ndx]	= 0.f;
	}

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["DECORATION"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats1)));
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats2)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);
		// Check against the two possible answers based on rounding mode.
		spec.verifyIO = &compareNoContractCase;

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}
	return group.release();
}

bool compareFRem(const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog&)
{
	if (outputAllocs.size() != 1)
		return false;

	vector<deUint8>	expectedBytes;
	expectedOutputs[0]->getBytes(expectedBytes);

	const float*	expectedOutputAsFloat	= reinterpret_cast<const float*>(&expectedBytes.front());
	const float*	outputAsFloat			= static_cast<const float*>(outputAllocs[0]->getHostPtr());

	for (size_t idx = 0; idx < expectedBytes.size() / sizeof(float); ++idx)
	{
		const float f0 = expectedOutputAsFloat[idx];
		const float f1 = outputAsFloat[idx];
		// \todo relative error needs to be fairly high because FRem may be implemented as
		// (roughly) frac(a/b)*b, so LSB errors can be magnified. But this should be fine for now.
		if (deFloatAbs((f1 - f0) / f0) > 0.02)
			return false;
	}

	return true;
}

tcu::TestCaseGroup* createOpFRemGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opfrem", "Test the OpFRem instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 200;
	vector<float>					inputFloats1	(numElements, 0);
	vector<float>					inputFloats2	(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);

	fillRandomScalars(rnd, -10000.f, 10000.f, &inputFloats1[0], numElements);
	fillRandomScalars(rnd, -100.f, 100.f, &inputFloats2[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		// Guard against divisors near zero.
		if (std::fabs(inputFloats2[ndx]) < 1e-3)
			inputFloats2[ndx] = 8.f;

		// The return value of std::fmod() has the same sign as its first operand, which is how OpFRem spec'd.
		outputFloats[ndx] = std::fmod(inputFloats1[ndx], inputFloats2[ndx]);
	}

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"OpDecorate %buf BufferBlock\n"
		"OpDecorate %indata1 DescriptorSet 0\n"
		"OpDecorate %indata1 Binding 0\n"
		"OpDecorate %indata2 DescriptorSet 0\n"
		"OpDecorate %indata2 Binding 1\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 2\n"
		"OpDecorate %f32arr ArrayStride 4\n"
		"OpMemberDecorate %buf 0 Offset 0\n"

		+ string(getComputeAsmCommonTypes()) +

		"%buf        = OpTypeStruct %f32arr\n"
		"%bufptr     = OpTypePointer Uniform %buf\n"
		"%indata1    = OpVariable %bufptr Uniform\n"
		"%indata2    = OpVariable %bufptr Uniform\n"
		"%outdata    = OpVariable %bufptr Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc1    = OpAccessChain %f32ptr %indata1 %zero %x\n"
		"%inval1    = OpLoad %f32 %inloc1\n"
		"%inloc2    = OpAccessChain %f32ptr %indata2 %zero %x\n"
		"%inval2    = OpLoad %f32 %inloc2\n"
		"%rem       = OpFRem %f32 %inval1 %inval2\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %rem\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";

	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats1)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats2)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);
	spec.verifyIO = &compareFRem;

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "", spec));

	return group.release();
}

bool compareNMin (const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog&)
{
	if (outputAllocs.size() != 1)
		return false;

	const BufferSp&			expectedOutput			(expectedOutputs[0]);
	std::vector<deUint8>	data;
	expectedOutput->getBytes(data);

	const float* const		expectedOutputAsFloat	= reinterpret_cast<const float*>(&data.front());
	const float* const		outputAsFloat			= static_cast<const float*>(outputAllocs[0]->getHostPtr());

	for (size_t idx = 0; idx < expectedOutput->getByteSize() / sizeof(float); ++idx)
	{
		const float f0 = expectedOutputAsFloat[idx];
		const float f1 = outputAsFloat[idx];

		// For NMin, we accept NaN as output if both inputs were NaN.
		// Otherwise the NaN is the wrong choise, as on architectures that
		// do not handle NaN, those are huge values.
		if (!(tcu::Float32(f1).isNaN() && tcu::Float32(f0).isNaN()) && deFloatAbs(f1 - f0) > 0.00001f)
			return false;
	}

	return true;
}

tcu::TestCaseGroup* createOpNMinGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opnmin", "Test the OpNMin instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 200;
	vector<float>					inputFloats1	(numElements, 0);
	vector<float>					inputFloats2	(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);

	fillRandomScalars(rnd, -10000.f, 10000.f, &inputFloats1[0], numElements);
	fillRandomScalars(rnd, -10000.f, 10000.f, &inputFloats2[0], numElements);

	// Make the first case a full-NAN case.
	inputFloats1[0] = TCU_NAN;
	inputFloats2[0] = TCU_NAN;

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		// By default, pick the smallest
		outputFloats[ndx] = std::min(inputFloats1[ndx], inputFloats2[ndx]);

		// Make half of the cases NaN cases
		if ((ndx & 1) == 0)
		{
			// Alternate between the NaN operand
			if ((ndx & 2) == 0)
			{
				outputFloats[ndx] = inputFloats2[ndx];
				inputFloats1[ndx] = TCU_NAN;
			}
			else
			{
				outputFloats[ndx] = inputFloats1[ndx];
				inputFloats2[ndx] = TCU_NAN;
			}
		}
	}

	spec.assembly =
		"OpCapability Shader\n"
		"%std450	= OpExtInstImport \"GLSL.std.450\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"OpDecorate %buf BufferBlock\n"
		"OpDecorate %indata1 DescriptorSet 0\n"
		"OpDecorate %indata1 Binding 0\n"
		"OpDecorate %indata2 DescriptorSet 0\n"
		"OpDecorate %indata2 Binding 1\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 2\n"
		"OpDecorate %f32arr ArrayStride 4\n"
		"OpMemberDecorate %buf 0 Offset 0\n"

		+ string(getComputeAsmCommonTypes()) +

		"%buf        = OpTypeStruct %f32arr\n"
		"%bufptr     = OpTypePointer Uniform %buf\n"
		"%indata1    = OpVariable %bufptr Uniform\n"
		"%indata2    = OpVariable %bufptr Uniform\n"
		"%outdata    = OpVariable %bufptr Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc1    = OpAccessChain %f32ptr %indata1 %zero %x\n"
		"%inval1    = OpLoad %f32 %inloc1\n"
		"%inloc2    = OpAccessChain %f32ptr %indata2 %zero %x\n"
		"%inval2    = OpLoad %f32 %inloc2\n"
		"%rem       = OpExtInst %f32 %std450 NMin %inval1 %inval2\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %rem\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";

	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats1)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats2)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);
	spec.verifyIO = &compareNMin;

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "", spec));

	return group.release();
}

bool compareNMax (const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog&)
{
	if (outputAllocs.size() != 1)
		return false;

	const BufferSp&			expectedOutput			= expectedOutputs[0];
	std::vector<deUint8>	data;
	expectedOutput->getBytes(data);

	const float* const		expectedOutputAsFloat	= reinterpret_cast<const float*>(&data.front());
	const float* const		outputAsFloat			= static_cast<const float*>(outputAllocs[0]->getHostPtr());

	for (size_t idx = 0; idx < expectedOutput->getByteSize() / sizeof(float); ++idx)
	{
		const float f0 = expectedOutputAsFloat[idx];
		const float f1 = outputAsFloat[idx];

		// For NMax, NaN is considered acceptable result, since in
		// architectures that do not handle NaNs, those are huge values.
		if (!tcu::Float32(f1).isNaN() && deFloatAbs(f1 - f0) > 0.00001f)
			return false;
	}

	return true;
}

tcu::TestCaseGroup* createOpNMaxGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group(new tcu::TestCaseGroup(testCtx, "opnmax", "Test the OpNMax instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 200;
	vector<float>					inputFloats1	(numElements, 0);
	vector<float>					inputFloats2	(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);

	fillRandomScalars(rnd, -10000.f, 10000.f, &inputFloats1[0], numElements);
	fillRandomScalars(rnd, -10000.f, 10000.f, &inputFloats2[0], numElements);

	// Make the first case a full-NAN case.
	inputFloats1[0] = TCU_NAN;
	inputFloats2[0] = TCU_NAN;

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		// By default, pick the biggest
		outputFloats[ndx] = std::max(inputFloats1[ndx], inputFloats2[ndx]);

		// Make half of the cases NaN cases
		if ((ndx & 1) == 0)
		{
			// Alternate between the NaN operand
			if ((ndx & 2) == 0)
			{
				outputFloats[ndx] = inputFloats2[ndx];
				inputFloats1[ndx] = TCU_NAN;
			}
			else
			{
				outputFloats[ndx] = inputFloats1[ndx];
				inputFloats2[ndx] = TCU_NAN;
			}
		}
	}

	spec.assembly =
		"OpCapability Shader\n"
		"%std450	= OpExtInstImport \"GLSL.std.450\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"OpDecorate %buf BufferBlock\n"
		"OpDecorate %indata1 DescriptorSet 0\n"
		"OpDecorate %indata1 Binding 0\n"
		"OpDecorate %indata2 DescriptorSet 0\n"
		"OpDecorate %indata2 Binding 1\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 2\n"
		"OpDecorate %f32arr ArrayStride 4\n"
		"OpMemberDecorate %buf 0 Offset 0\n"

		+ string(getComputeAsmCommonTypes()) +

		"%buf        = OpTypeStruct %f32arr\n"
		"%bufptr     = OpTypePointer Uniform %buf\n"
		"%indata1    = OpVariable %bufptr Uniform\n"
		"%indata2    = OpVariable %bufptr Uniform\n"
		"%outdata    = OpVariable %bufptr Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc1    = OpAccessChain %f32ptr %indata1 %zero %x\n"
		"%inval1    = OpLoad %f32 %inloc1\n"
		"%inloc2    = OpAccessChain %f32ptr %indata2 %zero %x\n"
		"%inval2    = OpLoad %f32 %inloc2\n"
		"%rem       = OpExtInst %f32 %std450 NMax %inval1 %inval2\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %rem\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";

	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats1)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats2)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);
	spec.verifyIO = &compareNMax;

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "", spec));

	return group.release();
}

bool compareNClamp (const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog&)
{
	if (outputAllocs.size() != 1)
		return false;

	const BufferSp&			expectedOutput			= expectedOutputs[0];
	std::vector<deUint8>	data;
	expectedOutput->getBytes(data);

	const float* const		expectedOutputAsFloat	= reinterpret_cast<const float*>(&data.front());
	const float* const		outputAsFloat			= static_cast<const float*>(outputAllocs[0]->getHostPtr());

	for (size_t idx = 0; idx < expectedOutput->getByteSize() / sizeof(float) / 2; ++idx)
	{
		const float e0 = expectedOutputAsFloat[idx * 2];
		const float e1 = expectedOutputAsFloat[idx * 2 + 1];
		const float res = outputAsFloat[idx];

		// For NClamp, we have two possible outcomes based on
		// whether NaNs are handled or not.
		// If either min or max value is NaN, the result is undefined,
		// so this test doesn't stress those. If the clamped value is
		// NaN, and NaNs are handled, the result is min; if NaNs are not
		// handled, they are big values that result in max.
		// If all three parameters are NaN, the result should be NaN.
		if (!((tcu::Float32(e0).isNaN() && tcu::Float32(res).isNaN()) ||
			 (deFloatAbs(e0 - res) < 0.00001f) ||
			 (deFloatAbs(e1 - res) < 0.00001f)))
			return false;
	}

	return true;
}

tcu::TestCaseGroup* createOpNClampGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opnclamp", "Test the OpNClamp instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 200;
	vector<float>					inputFloats1	(numElements, 0);
	vector<float>					inputFloats2	(numElements, 0);
	vector<float>					inputFloats3	(numElements, 0);
	vector<float>					outputFloats	(numElements * 2, 0);

	fillRandomScalars(rnd, -10000.f, 10000.f, &inputFloats1[0], numElements);
	fillRandomScalars(rnd, -10000.f, 10000.f, &inputFloats2[0], numElements);
	fillRandomScalars(rnd, -10000.f, 10000.f, &inputFloats3[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		// Results are only defined if max value is bigger than min value.
		if (inputFloats2[ndx] > inputFloats3[ndx])
		{
			float t = inputFloats2[ndx];
			inputFloats2[ndx] = inputFloats3[ndx];
			inputFloats3[ndx] = t;
		}

		// By default, do the clamp, setting both possible answers
		float defaultRes = std::min(std::max(inputFloats1[ndx], inputFloats2[ndx]), inputFloats3[ndx]);

		float maxResA = std::max(inputFloats1[ndx], inputFloats2[ndx]);
		float maxResB = maxResA;

		// Alternate between the NaN cases
		if (ndx & 1)
		{
			inputFloats1[ndx] = TCU_NAN;
			// If NaN is handled, the result should be same as the clamp minimum.
			// If NaN is not handled, the result should clamp to the clamp maximum.
			maxResA = inputFloats2[ndx];
			maxResB = inputFloats3[ndx];
		}
		else
		{
			// Not a NaN case - only one legal result.
			maxResA = defaultRes;
			maxResB = defaultRes;
		}

		outputFloats[ndx * 2] = maxResA;
		outputFloats[ndx * 2 + 1] = maxResB;
	}

	// Make the first case a full-NAN case.
	inputFloats1[0] = TCU_NAN;
	inputFloats2[0] = TCU_NAN;
	inputFloats3[0] = TCU_NAN;
	outputFloats[0] = TCU_NAN;
	outputFloats[1] = TCU_NAN;

	spec.assembly =
		"OpCapability Shader\n"
		"%std450	= OpExtInstImport \"GLSL.std.450\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"OpDecorate %buf BufferBlock\n"
		"OpDecorate %indata1 DescriptorSet 0\n"
		"OpDecorate %indata1 Binding 0\n"
		"OpDecorate %indata2 DescriptorSet 0\n"
		"OpDecorate %indata2 Binding 1\n"
		"OpDecorate %indata3 DescriptorSet 0\n"
		"OpDecorate %indata3 Binding 2\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 3\n"
		"OpDecorate %f32arr ArrayStride 4\n"
		"OpMemberDecorate %buf 0 Offset 0\n"

		+ string(getComputeAsmCommonTypes()) +

		"%buf        = OpTypeStruct %f32arr\n"
		"%bufptr     = OpTypePointer Uniform %buf\n"
		"%indata1    = OpVariable %bufptr Uniform\n"
		"%indata2    = OpVariable %bufptr Uniform\n"
		"%indata3    = OpVariable %bufptr Uniform\n"
		"%outdata    = OpVariable %bufptr Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc1    = OpAccessChain %f32ptr %indata1 %zero %x\n"
		"%inval1    = OpLoad %f32 %inloc1\n"
		"%inloc2    = OpAccessChain %f32ptr %indata2 %zero %x\n"
		"%inval2    = OpLoad %f32 %inloc2\n"
		"%inloc3    = OpAccessChain %f32ptr %indata3 %zero %x\n"
		"%inval3    = OpLoad %f32 %inloc3\n"
		"%rem       = OpExtInst %f32 %std450 NClamp %inval1 %inval2 %inval3\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %rem\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";

	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats1)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats2)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats3)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);
	spec.verifyIO = &compareNClamp;

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "", spec));

	return group.release();
}

tcu::TestCaseGroup* createOpSRemComputeGroup (tcu::TestContext& testCtx, qpTestResult negFailResult)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opsrem", "Test the OpSRem instruction"));
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 200;

	const struct CaseParams
	{
		const char*		name;
		const char*		failMessage;		// customized status message
		qpTestResult	failResult;			// override status on failure
		int				op1Min, op1Max;		// operand ranges
		int				op2Min, op2Max;
	} cases[] =
	{
		{ "positive",	"Output doesn't match with expected",				QP_TEST_RESULT_FAIL,	0,		65536,	0,		100 },
		{ "all",		"Inconsistent results, but within specification",	negFailResult,			-65536,	65536,	-100,	100 },	// see below
	};
	// If either operand is negative the result is undefined. Some implementations may still return correct values.

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); ++caseNdx)
	{
		const CaseParams&	params		= cases[caseNdx];
		ComputeShaderSpec	spec;
		vector<deInt32>		inputInts1	(numElements, 0);
		vector<deInt32>		inputInts2	(numElements, 0);
		vector<deInt32>		outputInts	(numElements, 0);

		fillRandomScalars(rnd, params.op1Min, params.op1Max, &inputInts1[0], numElements);
		fillRandomScalars(rnd, params.op2Min, params.op2Max, &inputInts2[0], numElements, filterNotZero);

		for (int ndx = 0; ndx < numElements; ++ndx)
		{
			// The return value of std::fmod() has the same sign as its first operand, which is how OpFRem spec'd.
			outputInts[ndx] = inputInts1[ndx] % inputInts2[ndx];
		}

		spec.assembly =
			string(getComputeAsmShaderPreamble()) +

			"OpName %main           \"main\"\n"
			"OpName %id             \"gl_GlobalInvocationID\"\n"

			"OpDecorate %id BuiltIn GlobalInvocationId\n"

			"OpDecorate %buf BufferBlock\n"
			"OpDecorate %indata1 DescriptorSet 0\n"
			"OpDecorate %indata1 Binding 0\n"
			"OpDecorate %indata2 DescriptorSet 0\n"
			"OpDecorate %indata2 Binding 1\n"
			"OpDecorate %outdata DescriptorSet 0\n"
			"OpDecorate %outdata Binding 2\n"
			"OpDecorate %i32arr ArrayStride 4\n"
			"OpMemberDecorate %buf 0 Offset 0\n"

			+ string(getComputeAsmCommonTypes()) +

			"%buf        = OpTypeStruct %i32arr\n"
			"%bufptr     = OpTypePointer Uniform %buf\n"
			"%indata1    = OpVariable %bufptr Uniform\n"
			"%indata2    = OpVariable %bufptr Uniform\n"
			"%outdata    = OpVariable %bufptr Uniform\n"

			"%id        = OpVariable %uvec3ptr Input\n"
			"%zero      = OpConstant %i32 0\n"

			"%main      = OpFunction %void None %voidf\n"
			"%label     = OpLabel\n"
			"%idval     = OpLoad %uvec3 %id\n"
			"%x         = OpCompositeExtract %u32 %idval 0\n"
			"%inloc1    = OpAccessChain %i32ptr %indata1 %zero %x\n"
			"%inval1    = OpLoad %i32 %inloc1\n"
			"%inloc2    = OpAccessChain %i32ptr %indata2 %zero %x\n"
			"%inval2    = OpLoad %i32 %inloc2\n"
			"%rem       = OpSRem %i32 %inval1 %inval2\n"
			"%outloc    = OpAccessChain %i32ptr %outdata %zero %x\n"
			"             OpStore %outloc %rem\n"
			"             OpReturn\n"
			"             OpFunctionEnd\n";

		spec.inputs.push_back	(BufferSp(new Int32Buffer(inputInts1)));
		spec.inputs.push_back	(BufferSp(new Int32Buffer(inputInts2)));
		spec.outputs.push_back	(BufferSp(new Int32Buffer(outputInts)));
		spec.numWorkGroups		= IVec3(numElements, 1, 1);
		spec.failResult			= params.failResult;
		spec.failMessage		= params.failMessage;

		group->addChild(new SpvAsmComputeShaderCase(testCtx, params.name, "", spec));
	}

	return group.release();
}

tcu::TestCaseGroup* createOpSRemComputeGroup64 (tcu::TestContext& testCtx, qpTestResult negFailResult)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opsrem64", "Test the 64-bit OpSRem instruction"));
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 200;

	const struct CaseParams
	{
		const char*		name;
		const char*		failMessage;		// customized status message
		qpTestResult	failResult;			// override status on failure
		bool			positive;
	} cases[] =
	{
		{ "positive",	"Output doesn't match with expected",				QP_TEST_RESULT_FAIL,	true },
		{ "all",		"Inconsistent results, but within specification",	negFailResult,			false },	// see below
	};
	// If either operand is negative the result is undefined. Some implementations may still return correct values.

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); ++caseNdx)
	{
		const CaseParams&	params		= cases[caseNdx];
		ComputeShaderSpec	spec;
		vector<deInt64>		inputInts1	(numElements, 0);
		vector<deInt64>		inputInts2	(numElements, 0);
		vector<deInt64>		outputInts	(numElements, 0);

		if (params.positive)
		{
			fillRandomInt64sLogDistributed(rnd, inputInts1, numElements, filterNonNegative);
			fillRandomInt64sLogDistributed(rnd, inputInts2, numElements, filterPositive);
		}
		else
		{
			fillRandomInt64sLogDistributed(rnd, inputInts1, numElements);
			fillRandomInt64sLogDistributed(rnd, inputInts2, numElements, filterNotZero);
		}

		for (int ndx = 0; ndx < numElements; ++ndx)
		{
			// The return value of std::fmod() has the same sign as its first operand, which is how OpFRem spec'd.
			outputInts[ndx] = inputInts1[ndx] % inputInts2[ndx];
		}

		spec.assembly =
			"OpCapability Int64\n"

			+ string(getComputeAsmShaderPreamble()) +

			"OpName %main           \"main\"\n"
			"OpName %id             \"gl_GlobalInvocationID\"\n"

			"OpDecorate %id BuiltIn GlobalInvocationId\n"

			"OpDecorate %buf BufferBlock\n"
			"OpDecorate %indata1 DescriptorSet 0\n"
			"OpDecorate %indata1 Binding 0\n"
			"OpDecorate %indata2 DescriptorSet 0\n"
			"OpDecorate %indata2 Binding 1\n"
			"OpDecorate %outdata DescriptorSet 0\n"
			"OpDecorate %outdata Binding 2\n"
			"OpDecorate %i64arr ArrayStride 8\n"
			"OpMemberDecorate %buf 0 Offset 0\n"

			+ string(getComputeAsmCommonTypes())
			+ string(getComputeAsmCommonInt64Types()) +

			"%buf        = OpTypeStruct %i64arr\n"
			"%bufptr     = OpTypePointer Uniform %buf\n"
			"%indata1    = OpVariable %bufptr Uniform\n"
			"%indata2    = OpVariable %bufptr Uniform\n"
			"%outdata    = OpVariable %bufptr Uniform\n"

			"%id        = OpVariable %uvec3ptr Input\n"
			"%zero      = OpConstant %i64 0\n"

			"%main      = OpFunction %void None %voidf\n"
			"%label     = OpLabel\n"
			"%idval     = OpLoad %uvec3 %id\n"
			"%x         = OpCompositeExtract %u32 %idval 0\n"
			"%inloc1    = OpAccessChain %i64ptr %indata1 %zero %x\n"
			"%inval1    = OpLoad %i64 %inloc1\n"
			"%inloc2    = OpAccessChain %i64ptr %indata2 %zero %x\n"
			"%inval2    = OpLoad %i64 %inloc2\n"
			"%rem       = OpSRem %i64 %inval1 %inval2\n"
			"%outloc    = OpAccessChain %i64ptr %outdata %zero %x\n"
			"             OpStore %outloc %rem\n"
			"             OpReturn\n"
			"             OpFunctionEnd\n";

		spec.inputs.push_back	(BufferSp(new Int64Buffer(inputInts1)));
		spec.inputs.push_back	(BufferSp(new Int64Buffer(inputInts2)));
		spec.outputs.push_back	(BufferSp(new Int64Buffer(outputInts)));
		spec.numWorkGroups		= IVec3(numElements, 1, 1);
		spec.failResult			= params.failResult;
		spec.failMessage		= params.failMessage;

		group->addChild(new SpvAsmComputeShaderCase(testCtx, params.name, "", spec, COMPUTE_TEST_USES_INT64));
	}

	return group.release();
}

tcu::TestCaseGroup* createOpSModComputeGroup (tcu::TestContext& testCtx, qpTestResult negFailResult)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opsmod", "Test the OpSMod instruction"));
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 200;

	const struct CaseParams
	{
		const char*		name;
		const char*		failMessage;		// customized status message
		qpTestResult	failResult;			// override status on failure
		int				op1Min, op1Max;		// operand ranges
		int				op2Min, op2Max;
	} cases[] =
	{
		{ "positive",	"Output doesn't match with expected",				QP_TEST_RESULT_FAIL,	0,		65536,	0,		100 },
		{ "all",		"Inconsistent results, but within specification",	negFailResult,			-65536,	65536,	-100,	100 },	// see below
	};
	// If either operand is negative the result is undefined. Some implementations may still return correct values.

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); ++caseNdx)
	{
		const CaseParams&	params		= cases[caseNdx];

		ComputeShaderSpec	spec;
		vector<deInt32>		inputInts1	(numElements, 0);
		vector<deInt32>		inputInts2	(numElements, 0);
		vector<deInt32>		outputInts	(numElements, 0);

		fillRandomScalars(rnd, params.op1Min, params.op1Max, &inputInts1[0], numElements);
		fillRandomScalars(rnd, params.op2Min, params.op2Max, &inputInts2[0], numElements, filterNotZero);

		for (int ndx = 0; ndx < numElements; ++ndx)
		{
			deInt32 rem = inputInts1[ndx] % inputInts2[ndx];
			if (rem == 0)
			{
				outputInts[ndx] = 0;
			}
			else if ((inputInts1[ndx] >= 0) == (inputInts2[ndx] >= 0))
			{
				// They have the same sign
				outputInts[ndx] = rem;
			}
			else
			{
				// They have opposite sign.  The remainder operation takes the
				// sign inputInts1[ndx] but OpSMod is supposed to take ths sign
				// of inputInts2[ndx].  Adding inputInts2[ndx] will ensure that
				// the result has the correct sign and that it is still
				// congruent to inputInts1[ndx] modulo inputInts2[ndx]
				//
				// See also http://mathforum.org/library/drmath/view/52343.html
				outputInts[ndx] = rem + inputInts2[ndx];
			}
		}

		spec.assembly =
			string(getComputeAsmShaderPreamble()) +

			"OpName %main           \"main\"\n"
			"OpName %id             \"gl_GlobalInvocationID\"\n"

			"OpDecorate %id BuiltIn GlobalInvocationId\n"

			"OpDecorate %buf BufferBlock\n"
			"OpDecorate %indata1 DescriptorSet 0\n"
			"OpDecorate %indata1 Binding 0\n"
			"OpDecorate %indata2 DescriptorSet 0\n"
			"OpDecorate %indata2 Binding 1\n"
			"OpDecorate %outdata DescriptorSet 0\n"
			"OpDecorate %outdata Binding 2\n"
			"OpDecorate %i32arr ArrayStride 4\n"
			"OpMemberDecorate %buf 0 Offset 0\n"

			+ string(getComputeAsmCommonTypes()) +

			"%buf        = OpTypeStruct %i32arr\n"
			"%bufptr     = OpTypePointer Uniform %buf\n"
			"%indata1    = OpVariable %bufptr Uniform\n"
			"%indata2    = OpVariable %bufptr Uniform\n"
			"%outdata    = OpVariable %bufptr Uniform\n"

			"%id        = OpVariable %uvec3ptr Input\n"
			"%zero      = OpConstant %i32 0\n"

			"%main      = OpFunction %void None %voidf\n"
			"%label     = OpLabel\n"
			"%idval     = OpLoad %uvec3 %id\n"
			"%x         = OpCompositeExtract %u32 %idval 0\n"
			"%inloc1    = OpAccessChain %i32ptr %indata1 %zero %x\n"
			"%inval1    = OpLoad %i32 %inloc1\n"
			"%inloc2    = OpAccessChain %i32ptr %indata2 %zero %x\n"
			"%inval2    = OpLoad %i32 %inloc2\n"
			"%rem       = OpSMod %i32 %inval1 %inval2\n"
			"%outloc    = OpAccessChain %i32ptr %outdata %zero %x\n"
			"             OpStore %outloc %rem\n"
			"             OpReturn\n"
			"             OpFunctionEnd\n";

		spec.inputs.push_back	(BufferSp(new Int32Buffer(inputInts1)));
		spec.inputs.push_back	(BufferSp(new Int32Buffer(inputInts2)));
		spec.outputs.push_back	(BufferSp(new Int32Buffer(outputInts)));
		spec.numWorkGroups		= IVec3(numElements, 1, 1);
		spec.failResult			= params.failResult;
		spec.failMessage		= params.failMessage;

		group->addChild(new SpvAsmComputeShaderCase(testCtx, params.name, "", spec));
	}

	return group.release();
}

tcu::TestCaseGroup* createOpSModComputeGroup64 (tcu::TestContext& testCtx, qpTestResult negFailResult)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opsmod64", "Test the OpSMod instruction"));
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 200;

	const struct CaseParams
	{
		const char*		name;
		const char*		failMessage;		// customized status message
		qpTestResult	failResult;			// override status on failure
		bool			positive;
	} cases[] =
	{
		{ "positive",	"Output doesn't match with expected",				QP_TEST_RESULT_FAIL,	true },
		{ "all",		"Inconsistent results, but within specification",	negFailResult,			false },	// see below
	};
	// If either operand is negative the result is undefined. Some implementations may still return correct values.

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); ++caseNdx)
	{
		const CaseParams&	params		= cases[caseNdx];

		ComputeShaderSpec	spec;
		vector<deInt64>		inputInts1	(numElements, 0);
		vector<deInt64>		inputInts2	(numElements, 0);
		vector<deInt64>		outputInts	(numElements, 0);


		if (params.positive)
		{
			fillRandomInt64sLogDistributed(rnd, inputInts1, numElements, filterNonNegative);
			fillRandomInt64sLogDistributed(rnd, inputInts2, numElements, filterPositive);
		}
		else
		{
			fillRandomInt64sLogDistributed(rnd, inputInts1, numElements);
			fillRandomInt64sLogDistributed(rnd, inputInts2, numElements, filterNotZero);
		}

		for (int ndx = 0; ndx < numElements; ++ndx)
		{
			deInt64 rem = inputInts1[ndx] % inputInts2[ndx];
			if (rem == 0)
			{
				outputInts[ndx] = 0;
			}
			else if ((inputInts1[ndx] >= 0) == (inputInts2[ndx] >= 0))
			{
				// They have the same sign
				outputInts[ndx] = rem;
			}
			else
			{
				// They have opposite sign.  The remainder operation takes the
				// sign inputInts1[ndx] but OpSMod is supposed to take ths sign
				// of inputInts2[ndx].  Adding inputInts2[ndx] will ensure that
				// the result has the correct sign and that it is still
				// congruent to inputInts1[ndx] modulo inputInts2[ndx]
				//
				// See also http://mathforum.org/library/drmath/view/52343.html
				outputInts[ndx] = rem + inputInts2[ndx];
			}
		}

		spec.assembly =
			"OpCapability Int64\n"

			+ string(getComputeAsmShaderPreamble()) +

			"OpName %main           \"main\"\n"
			"OpName %id             \"gl_GlobalInvocationID\"\n"

			"OpDecorate %id BuiltIn GlobalInvocationId\n"

			"OpDecorate %buf BufferBlock\n"
			"OpDecorate %indata1 DescriptorSet 0\n"
			"OpDecorate %indata1 Binding 0\n"
			"OpDecorate %indata2 DescriptorSet 0\n"
			"OpDecorate %indata2 Binding 1\n"
			"OpDecorate %outdata DescriptorSet 0\n"
			"OpDecorate %outdata Binding 2\n"
			"OpDecorate %i64arr ArrayStride 8\n"
			"OpMemberDecorate %buf 0 Offset 0\n"

			+ string(getComputeAsmCommonTypes())
			+ string(getComputeAsmCommonInt64Types()) +

			"%buf        = OpTypeStruct %i64arr\n"
			"%bufptr     = OpTypePointer Uniform %buf\n"
			"%indata1    = OpVariable %bufptr Uniform\n"
			"%indata2    = OpVariable %bufptr Uniform\n"
			"%outdata    = OpVariable %bufptr Uniform\n"

			"%id        = OpVariable %uvec3ptr Input\n"
			"%zero      = OpConstant %i64 0\n"

			"%main      = OpFunction %void None %voidf\n"
			"%label     = OpLabel\n"
			"%idval     = OpLoad %uvec3 %id\n"
			"%x         = OpCompositeExtract %u32 %idval 0\n"
			"%inloc1    = OpAccessChain %i64ptr %indata1 %zero %x\n"
			"%inval1    = OpLoad %i64 %inloc1\n"
			"%inloc2    = OpAccessChain %i64ptr %indata2 %zero %x\n"
			"%inval2    = OpLoad %i64 %inloc2\n"
			"%rem       = OpSMod %i64 %inval1 %inval2\n"
			"%outloc    = OpAccessChain %i64ptr %outdata %zero %x\n"
			"             OpStore %outloc %rem\n"
			"             OpReturn\n"
			"             OpFunctionEnd\n";

		spec.inputs.push_back	(BufferSp(new Int64Buffer(inputInts1)));
		spec.inputs.push_back	(BufferSp(new Int64Buffer(inputInts2)));
		spec.outputs.push_back	(BufferSp(new Int64Buffer(outputInts)));
		spec.numWorkGroups		= IVec3(numElements, 1, 1);
		spec.failResult			= params.failResult;
		spec.failMessage		= params.failMessage;

		group->addChild(new SpvAsmComputeShaderCase(testCtx, params.name, "", spec, COMPUTE_TEST_USES_INT64));
	}

	return group.release();
}

// Copy contents in the input buffer to the output buffer.
tcu::TestCaseGroup* createOpCopyMemoryGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opcopymemory", "Test the OpCopyMemory instruction"));
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;

	// The following case adds vec4(0., 0.5, 1.5, 2.5) to each of the elements in the input buffer and writes output to the output buffer.
	ComputeShaderSpec				spec1;
	vector<Vec4>					inputFloats1	(numElements);
	vector<Vec4>					outputFloats1	(numElements);

	fillRandomScalars(rnd, -200.f, 200.f, &inputFloats1[0], numElements * 4);

	// CPU might not use the same rounding mode as the GPU. Use whole numbers to avoid rounding differences.
	floorAll(inputFloats1);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats1[ndx] = inputFloats1[ndx] + Vec4(0.f, 0.5f, 1.5f, 2.5f);

	spec1.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %vec4arr ArrayStride 16\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"%vec4       = OpTypeVector %f32 4\n"
		"%vec4ptr_u  = OpTypePointer Uniform %vec4\n"
		"%vec4ptr_f  = OpTypePointer Function %vec4\n"
		"%vec4arr    = OpTypeRuntimeArray %vec4\n"
		"%buf        = OpTypeStruct %vec4arr\n"
		"%bufptr     = OpTypePointer Uniform %buf\n"
		"%indata     = OpVariable %bufptr Uniform\n"
		"%outdata    = OpVariable %bufptr Uniform\n"

		"%id         = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%c_f_0      = OpConstant %f32 0.\n"
		"%c_f_0_5    = OpConstant %f32 0.5\n"
		"%c_f_1_5    = OpConstant %f32 1.5\n"
		"%c_f_2_5    = OpConstant %f32 2.5\n"
		"%c_vec4     = OpConstantComposite %vec4 %c_f_0 %c_f_0_5 %c_f_1_5 %c_f_2_5\n"

		"%main       = OpFunction %void None %voidf\n"
		"%label      = OpLabel\n"
		"%v_vec4     = OpVariable %vec4ptr_f Function\n"
		"%idval      = OpLoad %uvec3 %id\n"
		"%x          = OpCompositeExtract %u32 %idval 0\n"
		"%inloc      = OpAccessChain %vec4ptr_u %indata %zero %x\n"
		"%outloc     = OpAccessChain %vec4ptr_u %outdata %zero %x\n"
		"              OpCopyMemory %v_vec4 %inloc\n"
		"%v_vec4_val = OpLoad %vec4 %v_vec4\n"
		"%add        = OpFAdd %vec4 %v_vec4_val %c_vec4\n"
		"              OpStore %outloc %add\n"
		"              OpReturn\n"
		"              OpFunctionEnd\n";

	spec1.inputs.push_back(BufferSp(new Vec4Buffer(inputFloats1)));
	spec1.outputs.push_back(BufferSp(new Vec4Buffer(outputFloats1)));
	spec1.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "vector", "OpCopyMemory elements of vector type", spec1));

	// The following case copies a float[100] variable from the input buffer to the output buffer.
	ComputeShaderSpec				spec2;
	vector<float>					inputFloats2	(numElements);
	vector<float>					outputFloats2	(numElements);

	fillRandomScalars(rnd, -200.f, 200.f, &inputFloats2[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats2[ndx] = inputFloats2[ndx];

	spec2.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %f32arr100 ArrayStride 4\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"%hundred        = OpConstant %u32 100\n"
		"%f32arr100      = OpTypeArray %f32 %hundred\n"
		"%f32arr100ptr_f = OpTypePointer Function %f32arr100\n"
		"%f32arr100ptr_u = OpTypePointer Uniform %f32arr100\n"
		"%buf            = OpTypeStruct %f32arr100\n"
		"%bufptr         = OpTypePointer Uniform %buf\n"
		"%indata         = OpVariable %bufptr Uniform\n"
		"%outdata        = OpVariable %bufptr Uniform\n"

		"%id             = OpVariable %uvec3ptr Input\n"
		"%zero           = OpConstant %i32 0\n"

		"%main           = OpFunction %void None %voidf\n"
		"%label          = OpLabel\n"
		"%var            = OpVariable %f32arr100ptr_f Function\n"
		"%inarr          = OpAccessChain %f32arr100ptr_u %indata %zero\n"
		"%outarr         = OpAccessChain %f32arr100ptr_u %outdata %zero\n"
		"                  OpCopyMemory %var %inarr\n"
		"                  OpCopyMemory %outarr %var\n"
		"                  OpReturn\n"
		"                  OpFunctionEnd\n";

	spec2.inputs.push_back(BufferSp(new Float32Buffer(inputFloats2)));
	spec2.outputs.push_back(BufferSp(new Float32Buffer(outputFloats2)));
	spec2.numWorkGroups = IVec3(1, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "array", "OpCopyMemory elements of array type", spec2));

	// The following case copies a struct{vec4, vec4, vec4, vec4} variable from the input buffer to the output buffer.
	ComputeShaderSpec				spec3;
	vector<float>					inputFloats3	(16);
	vector<float>					outputFloats3	(16);

	fillRandomScalars(rnd, -200.f, 200.f, &inputFloats3[0], 16);

	for (size_t ndx = 0; ndx < 16; ++ndx)
		outputFloats3[ndx] = inputFloats3[ndx];

	spec3.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpMemberDecorate %buf 0 Offset 0\n"
		"OpMemberDecorate %buf 1 Offset 16\n"
		"OpMemberDecorate %buf 2 Offset 32\n"
		"OpMemberDecorate %buf 3 Offset 48\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"%vec4      = OpTypeVector %f32 4\n"
		"%buf       = OpTypeStruct %vec4 %vec4 %vec4 %vec4\n"
		"%bufptr    = OpTypePointer Uniform %buf\n"
		"%indata    = OpVariable %bufptr Uniform\n"
		"%outdata   = OpVariable %bufptr Uniform\n"
		"%vec4stptr = OpTypePointer Function %buf\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%var       = OpVariable %vec4stptr Function\n"
		"             OpCopyMemory %var %indata\n"
		"             OpCopyMemory %outdata %var\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";

	spec3.inputs.push_back(BufferSp(new Float32Buffer(inputFloats3)));
	spec3.outputs.push_back(BufferSp(new Float32Buffer(outputFloats3)));
	spec3.numWorkGroups = IVec3(1, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "struct", "OpCopyMemory elements of struct type", spec3));

	// The following case negates multiple float variables from the input buffer and stores the results to the output buffer.
	ComputeShaderSpec				spec4;
	vector<float>					inputFloats4	(numElements);
	vector<float>					outputFloats4	(numElements);

	fillRandomScalars(rnd, -200.f, 200.f, &inputFloats4[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats4[ndx] = -inputFloats4[ndx];

	spec4.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%f32ptr_f  = OpTypePointer Function %f32\n"
		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%var       = OpVariable %f32ptr_f Function\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpCopyMemory %var %inloc\n"
		"%val       = OpLoad %f32 %var\n"
		"%neg       = OpFNegate %f32 %val\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";

	spec4.inputs.push_back(BufferSp(new Float32Buffer(inputFloats4)));
	spec4.outputs.push_back(BufferSp(new Float32Buffer(outputFloats4)));
	spec4.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "float", "OpCopyMemory elements of float type", spec4));

	return group.release();
}

tcu::TestCaseGroup* createOpCopyObjectGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opcopyobject", "Test the OpCopyObject instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);

	fillRandomScalars(rnd, -200.f, 200.f, &inputFloats[0], numElements);

	// CPU might not use the same rounding mode as the GPU. Use whole numbers to avoid rounding differences.
	floorAll(inputFloats);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats[ndx] = inputFloats[ndx] + 7.5f;

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"%fmat     = OpTypeMatrix %fvec3 3\n"
		"%three    = OpConstant %u32 3\n"
		"%farr     = OpTypeArray %f32 %three\n"
		"%fst      = OpTypeStruct %f32 %f32\n"

		+ string(getComputeAsmInputOutputBuffer()) +

		"%id            = OpVariable %uvec3ptr Input\n"
		"%zero          = OpConstant %i32 0\n"
		"%c_f           = OpConstant %f32 1.5\n"
		"%c_fvec3       = OpConstantComposite %fvec3 %c_f %c_f %c_f\n"
		"%c_fmat        = OpConstantComposite %fmat %c_fvec3 %c_fvec3 %c_fvec3\n"
		"%c_farr        = OpConstantComposite %farr %c_f %c_f %c_f\n"
		"%c_fst         = OpConstantComposite %fst %c_f %c_f\n"

		"%main          = OpFunction %void None %voidf\n"
		"%label         = OpLabel\n"
		"%c_f_copy      = OpCopyObject %f32   %c_f\n"
		"%c_fvec3_copy  = OpCopyObject %fvec3 %c_fvec3\n"
		"%c_fmat_copy   = OpCopyObject %fmat  %c_fmat\n"
		"%c_farr_copy   = OpCopyObject %farr  %c_farr\n"
		"%c_fst_copy    = OpCopyObject %fst   %c_fst\n"
		"%fvec3_elem    = OpCompositeExtract %f32 %c_fvec3_copy 0\n"
		"%fmat_elem     = OpCompositeExtract %f32 %c_fmat_copy 1 2\n"
		"%farr_elem     = OpCompositeExtract %f32 %c_farr_copy 2\n"
		"%fst_elem      = OpCompositeExtract %f32 %c_fst_copy 1\n"
		// Add up. 1.5 * 5 = 7.5.
		"%add1          = OpFAdd %f32 %c_f_copy %fvec3_elem\n"
		"%add2          = OpFAdd %f32 %add1     %fmat_elem\n"
		"%add3          = OpFAdd %f32 %add2     %farr_elem\n"
		"%add4          = OpFAdd %f32 %add3     %fst_elem\n"

		"%idval         = OpLoad %uvec3 %id\n"
		"%x             = OpCompositeExtract %u32 %idval 0\n"
		"%inloc         = OpAccessChain %f32ptr %indata %zero %x\n"
		"%outloc        = OpAccessChain %f32ptr %outdata %zero %x\n"
		"%inval         = OpLoad %f32 %inloc\n"
		"%add           = OpFAdd %f32 %add4 %inval\n"
		"                 OpStore %outloc %add\n"
		"                 OpReturn\n"
		"                 OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "spotcheck", "OpCopyObject on different types", spec));

	return group.release();
}
// Assembly code used for testing OpUnreachable is based on GLSL source code:
//
// #version 430
//
// layout(std140, set = 0, binding = 0) readonly buffer Input {
//   float elements[];
// } input_data;
// layout(std140, set = 0, binding = 1) writeonly buffer Output {
//   float elements[];
// } output_data;
//
// void not_called_func() {
//   // place OpUnreachable here
// }
//
// uint modulo4(uint val) {
//   switch (val % uint(4)) {
//     case 0:  return 3;
//     case 1:  return 2;
//     case 2:  return 1;
//     case 3:  return 0;
//     default: return 100; // place OpUnreachable here
//   }
// }
//
// uint const5() {
//   return 5;
//   // place OpUnreachable here
// }
//
// void main() {
//   uint x = gl_GlobalInvocationID.x;
//   if (const5() > modulo4(1000)) {
//     output_data.elements[x] = -input_data.elements[x];
//   } else {
//     // place OpUnreachable here
//     output_data.elements[x] = input_data.elements[x];
//   }
// }

tcu::TestCaseGroup* createOpUnreachableGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opunreachable", "Test the OpUnreachable instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main            \"main\"\n"
		"OpName %func_not_called_func \"not_called_func(\"\n"
		"OpName %func_modulo4         \"modulo4(u1;\"\n"
		"OpName %func_const5          \"const5(\"\n"
		"OpName %id                   \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"%u32ptr    = OpTypePointer Function %u32\n"
		"%uintfuint = OpTypeFunction %u32 %u32ptr\n"
		"%unitf     = OpTypeFunction %u32\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %u32 0\n"
		"%one       = OpConstant %u32 1\n"
		"%two       = OpConstant %u32 2\n"
		"%three     = OpConstant %u32 3\n"
		"%four      = OpConstant %u32 4\n"
		"%five      = OpConstant %u32 5\n"
		"%hundred   = OpConstant %u32 100\n"
		"%thousand  = OpConstant %u32 1000\n"

		+ string(getComputeAsmInputOutputBuffer()) +

		// Main()
		"%main   = OpFunction %void None %voidf\n"
		"%main_entry  = OpLabel\n"
		"%v_thousand  = OpVariable %u32ptr Function %thousand\n"
		"%idval       = OpLoad %uvec3 %id\n"
		"%x           = OpCompositeExtract %u32 %idval 0\n"
		"%inloc       = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval       = OpLoad %f32 %inloc\n"
		"%outloc      = OpAccessChain %f32ptr %outdata %zero %x\n"
		"%ret_const5  = OpFunctionCall %u32 %func_const5\n"
		"%ret_modulo4 = OpFunctionCall %u32 %func_modulo4 %v_thousand\n"
		"%cmp_gt      = OpUGreaterThan %bool %ret_const5 %ret_modulo4\n"
		"               OpSelectionMerge %if_end None\n"
		"               OpBranchConditional %cmp_gt %if_true %if_false\n"
		"%if_true     = OpLabel\n"
		"%negate      = OpFNegate %f32 %inval\n"
		"               OpStore %outloc %negate\n"
		"               OpBranch %if_end\n"
		"%if_false    = OpLabel\n"
		"               OpUnreachable\n" // Unreachable else branch for if statement
		"%if_end      = OpLabel\n"
		"               OpReturn\n"
		"               OpFunctionEnd\n"

		// not_called_function()
		"%func_not_called_func  = OpFunction %void None %voidf\n"
		"%not_called_func_entry = OpLabel\n"
		"                         OpUnreachable\n" // Unreachable entry block in not called static function
		"                         OpFunctionEnd\n"

		// modulo4()
		"%func_modulo4  = OpFunction %u32 None %uintfuint\n"
		"%valptr        = OpFunctionParameter %u32ptr\n"
		"%modulo4_entry = OpLabel\n"
		"%val           = OpLoad %u32 %valptr\n"
		"%modulo        = OpUMod %u32 %val %four\n"
		"                 OpSelectionMerge %switch_merge None\n"
		"                 OpSwitch %modulo %default 0 %case0 1 %case1 2 %case2 3 %case3\n"
		"%case0         = OpLabel\n"
		"                 OpReturnValue %three\n"
		"%case1         = OpLabel\n"
		"                 OpReturnValue %two\n"
		"%case2         = OpLabel\n"
		"                 OpReturnValue %one\n"
		"%case3         = OpLabel\n"
		"                 OpReturnValue %zero\n"
		"%default       = OpLabel\n"
		"                 OpUnreachable\n" // Unreachable default case for switch statement
		"%switch_merge  = OpLabel\n"
		"                 OpUnreachable\n" // Unreachable merge block for switch statement
		"                 OpFunctionEnd\n"

		// const5()
		"%func_const5  = OpFunction %u32 None %unitf\n"
		"%const5_entry = OpLabel\n"
		"                OpReturnValue %five\n"
		"%unreachable  = OpLabel\n"
		"                OpUnreachable\n" // Unreachable block in function
		"                OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "OpUnreachable appearing at different places", spec));

	return group.release();
}

// Assembly code used for testing decoration group is based on GLSL source code:
//
// #version 430
//
// layout(std140, set = 0, binding = 0) readonly buffer Input0 {
//   float elements[];
// } input_data0;
// layout(std140, set = 0, binding = 1) readonly buffer Input1 {
//   float elements[];
// } input_data1;
// layout(std140, set = 0, binding = 2) readonly buffer Input2 {
//   float elements[];
// } input_data2;
// layout(std140, set = 0, binding = 3) readonly buffer Input3 {
//   float elements[];
// } input_data3;
// layout(std140, set = 0, binding = 4) readonly buffer Input4 {
//   float elements[];
// } input_data4;
// layout(std140, set = 0, binding = 5) writeonly buffer Output {
//   float elements[];
// } output_data;
//
// void main() {
//   uint x = gl_GlobalInvocationID.x;
//   output_data.elements[x] = input_data0.elements[x] + input_data1.elements[x] + input_data2.elements[x] + input_data3.elements[x] + input_data4.elements[x];
// }
tcu::TestCaseGroup* createDecorationGroupGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "decoration_group", "Test the OpDecorationGroup & OpGroupDecorate instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats0	(numElements, 0);
	vector<float>					inputFloats1	(numElements, 0);
	vector<float>					inputFloats2	(numElements, 0);
	vector<float>					inputFloats3	(numElements, 0);
	vector<float>					inputFloats4	(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);

	fillRandomScalars(rnd, -300.f, 300.f, &inputFloats0[0], numElements);
	fillRandomScalars(rnd, -300.f, 300.f, &inputFloats1[0], numElements);
	fillRandomScalars(rnd, -300.f, 300.f, &inputFloats2[0], numElements);
	fillRandomScalars(rnd, -300.f, 300.f, &inputFloats3[0], numElements);
	fillRandomScalars(rnd, -300.f, 300.f, &inputFloats4[0], numElements);

	// CPU might not use the same rounding mode as the GPU. Use whole numbers to avoid rounding differences.
	floorAll(inputFloats0);
	floorAll(inputFloats1);
	floorAll(inputFloats2);
	floorAll(inputFloats3);
	floorAll(inputFloats4);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats[ndx] = inputFloats0[ndx] + inputFloats1[ndx] + inputFloats2[ndx] + inputFloats3[ndx] + inputFloats4[ndx];

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		// Not using group decoration on variable.
		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		// Not using group decoration on type.
		"OpDecorate %f32arr ArrayStride 4\n"

		"OpDecorate %groups BufferBlock\n"
		"OpDecorate %groupm Offset 0\n"
		"%groups = OpDecorationGroup\n"
		"%groupm = OpDecorationGroup\n"

		// Group decoration on multiple structs.
		"OpGroupDecorate %groups %outbuf %inbuf0 %inbuf1 %inbuf2 %inbuf3 %inbuf4\n"
		// Group decoration on multiple struct members.
		"OpGroupMemberDecorate %groupm %outbuf 0 %inbuf0 0 %inbuf1 0 %inbuf2 0 %inbuf3 0 %inbuf4 0\n"

		"OpDecorate %group1 DescriptorSet 0\n"
		"OpDecorate %group3 DescriptorSet 0\n"
		"OpDecorate %group3 NonWritable\n"
		"OpDecorate %group3 Restrict\n"
		"%group0 = OpDecorationGroup\n"
		"%group1 = OpDecorationGroup\n"
		"%group3 = OpDecorationGroup\n"

		// Applying the same decoration group multiple times.
		"OpGroupDecorate %group1 %outdata\n"
		"OpGroupDecorate %group1 %outdata\n"
		"OpGroupDecorate %group1 %outdata\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 5\n"
		// Applying decoration group containing nothing.
		"OpGroupDecorate %group0 %indata0\n"
		"OpDecorate %indata0 DescriptorSet 0\n"
		"OpDecorate %indata0 Binding 0\n"
		// Applying decoration group containing one decoration.
		"OpGroupDecorate %group1 %indata1\n"
		"OpDecorate %indata1 Binding 1\n"
		// Applying decoration group containing multiple decorations.
		"OpGroupDecorate %group3 %indata2 %indata3\n"
		"OpDecorate %indata2 Binding 2\n"
		"OpDecorate %indata3 Binding 3\n"
		// Applying multiple decoration groups (with overlapping).
		"OpGroupDecorate %group0 %indata4\n"
		"OpGroupDecorate %group1 %indata4\n"
		"OpGroupDecorate %group3 %indata4\n"
		"OpDecorate %indata4 Binding 4\n"

		+ string(getComputeAsmCommonTypes()) +

		"%id   = OpVariable %uvec3ptr Input\n"
		"%zero = OpConstant %i32 0\n"

		"%outbuf    = OpTypeStruct %f32arr\n"
		"%outbufptr = OpTypePointer Uniform %outbuf\n"
		"%outdata   = OpVariable %outbufptr Uniform\n"
		"%inbuf0    = OpTypeStruct %f32arr\n"
		"%inbuf0ptr = OpTypePointer Uniform %inbuf0\n"
		"%indata0   = OpVariable %inbuf0ptr Uniform\n"
		"%inbuf1    = OpTypeStruct %f32arr\n"
		"%inbuf1ptr = OpTypePointer Uniform %inbuf1\n"
		"%indata1   = OpVariable %inbuf1ptr Uniform\n"
		"%inbuf2    = OpTypeStruct %f32arr\n"
		"%inbuf2ptr = OpTypePointer Uniform %inbuf2\n"
		"%indata2   = OpVariable %inbuf2ptr Uniform\n"
		"%inbuf3    = OpTypeStruct %f32arr\n"
		"%inbuf3ptr = OpTypePointer Uniform %inbuf3\n"
		"%indata3   = OpVariable %inbuf3ptr Uniform\n"
		"%inbuf4    = OpTypeStruct %f32arr\n"
		"%inbufptr  = OpTypePointer Uniform %inbuf4\n"
		"%indata4   = OpVariable %inbufptr Uniform\n"

		"%main   = OpFunction %void None %voidf\n"
		"%label  = OpLabel\n"
		"%idval  = OpLoad %uvec3 %id\n"
		"%x      = OpCompositeExtract %u32 %idval 0\n"
		"%inloc0 = OpAccessChain %f32ptr %indata0 %zero %x\n"
		"%inloc1 = OpAccessChain %f32ptr %indata1 %zero %x\n"
		"%inloc2 = OpAccessChain %f32ptr %indata2 %zero %x\n"
		"%inloc3 = OpAccessChain %f32ptr %indata3 %zero %x\n"
		"%inloc4 = OpAccessChain %f32ptr %indata4 %zero %x\n"
		"%outloc = OpAccessChain %f32ptr %outdata %zero %x\n"
		"%inval0 = OpLoad %f32 %inloc0\n"
		"%inval1 = OpLoad %f32 %inloc1\n"
		"%inval2 = OpLoad %f32 %inloc2\n"
		"%inval3 = OpLoad %f32 %inloc3\n"
		"%inval4 = OpLoad %f32 %inloc4\n"
		"%add0   = OpFAdd %f32 %inval0 %inval1\n"
		"%add1   = OpFAdd %f32 %add0 %inval2\n"
		"%add2   = OpFAdd %f32 %add1 %inval3\n"
		"%add    = OpFAdd %f32 %add2 %inval4\n"
		"          OpStore %outloc %add\n"
		"          OpReturn\n"
		"          OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats0)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats1)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats2)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats3)));
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats4)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "decoration group cases", spec));

	return group.release();
}

struct SpecConstantTwoIntCase
{
	const char*		caseName;
	const char*		scDefinition0;
	const char*		scDefinition1;
	const char*		scResultType;
	const char*		scOperation;
	deInt32			scActualValue0;
	deInt32			scActualValue1;
	const char*		resultOperation;
	vector<deInt32>	expectedOutput;

					SpecConstantTwoIntCase (const char* name,
											const char* definition0,
											const char* definition1,
											const char* resultType,
											const char* operation,
											deInt32 value0,
											deInt32 value1,
											const char* resultOp,
											const vector<deInt32>& output)
						: caseName			(name)
						, scDefinition0		(definition0)
						, scDefinition1		(definition1)
						, scResultType		(resultType)
						, scOperation		(operation)
						, scActualValue0	(value0)
						, scActualValue1	(value1)
						, resultOperation	(resultOp)
						, expectedOutput	(output) {}
};

tcu::TestCaseGroup* createSpecConstantGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opspecconstantop", "Test the OpSpecConstantOp instruction"));
	vector<SpecConstantTwoIntCase>	cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<deInt32>					inputInts		(numElements, 0);
	vector<deInt32>					outputInts1		(numElements, 0);
	vector<deInt32>					outputInts2		(numElements, 0);
	vector<deInt32>					outputInts3		(numElements, 0);
	vector<deInt32>					outputInts4		(numElements, 0);
	const StringTemplate			shaderTemplate	(
		"${CAPABILITIES:opt}"
		+ string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %sc_0  SpecId 0\n"
		"OpDecorate %sc_1  SpecId 1\n"
		"OpDecorate %i32arr ArrayStride 4\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"${OPTYPE_DEFINITIONS:opt}"
		"%buf     = OpTypeStruct %i32arr\n"
		"%bufptr  = OpTypePointer Uniform %buf\n"
		"%indata    = OpVariable %bufptr Uniform\n"
		"%outdata   = OpVariable %bufptr Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%sc_0      = OpSpecConstant${SC_DEF0}\n"
		"%sc_1      = OpSpecConstant${SC_DEF1}\n"
		"%sc_final  = OpSpecConstantOp ${SC_RESULT_TYPE} ${SC_OP}\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"${TYPE_CONVERT:opt}"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %i32ptr %indata %zero %x\n"
		"%inval     = OpLoad %i32 %inloc\n"
		"%final     = ${GEN_RESULT}\n"
		"%outloc    = OpAccessChain %i32ptr %outdata %zero %x\n"
		"             OpStore %outloc %final\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	fillRandomScalars(rnd, -65536, 65536, &inputInts[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		outputInts1[ndx] = inputInts[ndx] + 42;
		outputInts2[ndx] = inputInts[ndx];
		outputInts3[ndx] = inputInts[ndx] - 11200;
		outputInts4[ndx] = inputInts[ndx] + 1;
	}

	const char addScToInput[]		= "OpIAdd %i32 %inval %sc_final";
	const char addSc32ToInput[]		= "OpIAdd %i32 %inval %sc_final32";
	const char selectTrueUsingSc[]	= "OpSelect %i32 %sc_final %inval %zero";
	const char selectFalseUsingSc[]	= "OpSelect %i32 %sc_final %zero %inval";

	cases.push_back(SpecConstantTwoIntCase("iadd",					" %i32 0",		" %i32 0",		"%i32",		"IAdd                 %sc_0 %sc_1",			62,		-20,	addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("isub",					" %i32 0",		" %i32 0",		"%i32",		"ISub                 %sc_0 %sc_1",			100,	58,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("imul",					" %i32 0",		" %i32 0",		"%i32",		"IMul                 %sc_0 %sc_1",			-2,		-21,	addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("sdiv",					" %i32 0",		" %i32 0",		"%i32",		"SDiv                 %sc_0 %sc_1",			-126,	-3,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("udiv",					" %i32 0",		" %i32 0",		"%i32",		"UDiv                 %sc_0 %sc_1",			126,	3,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("srem",					" %i32 0",		" %i32 0",		"%i32",		"SRem                 %sc_0 %sc_1",			7,		3,		addScToInput,		outputInts4));
	cases.push_back(SpecConstantTwoIntCase("smod",					" %i32 0",		" %i32 0",		"%i32",		"SMod                 %sc_0 %sc_1",			7,		3,		addScToInput,		outputInts4));
	cases.push_back(SpecConstantTwoIntCase("umod",					" %i32 0",		" %i32 0",		"%i32",		"UMod                 %sc_0 %sc_1",			342,	50,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("bitwiseand",			" %i32 0",		" %i32 0",		"%i32",		"BitwiseAnd           %sc_0 %sc_1",			42,		63,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("bitwiseor",				" %i32 0",		" %i32 0",		"%i32",		"BitwiseOr            %sc_0 %sc_1",			34,		8,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("bitwisexor",			" %i32 0",		" %i32 0",		"%i32",		"BitwiseXor           %sc_0 %sc_1",			18,		56,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("shiftrightlogical",		" %i32 0",		" %i32 0",		"%i32",		"ShiftRightLogical    %sc_0 %sc_1",			168,	2,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("shiftrightarithmetic",	" %i32 0",		" %i32 0",		"%i32",		"ShiftRightArithmetic %sc_0 %sc_1",			168,	2,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("shiftleftlogical",		" %i32 0",		" %i32 0",		"%i32",		"ShiftLeftLogical     %sc_0 %sc_1",			21,		1,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("slessthan",				" %i32 0",		" %i32 0",		"%bool",	"SLessThan            %sc_0 %sc_1",			-20,	-10,	selectTrueUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("ulessthan",				" %i32 0",		" %i32 0",		"%bool",	"ULessThan            %sc_0 %sc_1",			10,		20,		selectTrueUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("sgreaterthan",			" %i32 0",		" %i32 0",		"%bool",	"SGreaterThan         %sc_0 %sc_1",			-1000,	50,		selectFalseUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("ugreaterthan",			" %i32 0",		" %i32 0",		"%bool",	"UGreaterThan         %sc_0 %sc_1",			10,		5,		selectTrueUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("slessthanequal",		" %i32 0",		" %i32 0",		"%bool",	"SLessThanEqual       %sc_0 %sc_1",			-10,	-10,	selectTrueUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("ulessthanequal",		" %i32 0",		" %i32 0",		"%bool",	"ULessThanEqual       %sc_0 %sc_1",			50,		100,	selectTrueUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("sgreaterthanequal",		" %i32 0",		" %i32 0",		"%bool",	"SGreaterThanEqual    %sc_0 %sc_1",			-1000,	50,		selectFalseUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("ugreaterthanequal",		" %i32 0",		" %i32 0",		"%bool",	"UGreaterThanEqual    %sc_0 %sc_1",			10,		10,		selectTrueUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("iequal",				" %i32 0",		" %i32 0",		"%bool",	"IEqual               %sc_0 %sc_1",			42,		24,		selectFalseUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("logicaland",			"True %bool",	"True %bool",	"%bool",	"LogicalAnd           %sc_0 %sc_1",			0,		1,		selectFalseUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("logicalor",				"False %bool",	"False %bool",	"%bool",	"LogicalOr            %sc_0 %sc_1",			1,		0,		selectTrueUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("logicalequal",			"True %bool",	"True %bool",	"%bool",	"LogicalEqual         %sc_0 %sc_1",			0,		1,		selectFalseUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("logicalnotequal",		"False %bool",	"False %bool",	"%bool",	"LogicalNotEqual      %sc_0 %sc_1",			1,		0,		selectTrueUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("snegate",				" %i32 0",		" %i32 0",		"%i32",		"SNegate              %sc_0",				-42,	0,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("not",					" %i32 0",		" %i32 0",		"%i32",		"Not                  %sc_0",				-43,	0,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("logicalnot",			"False %bool",	"False %bool",	"%bool",	"LogicalNot           %sc_0",				1,		0,		selectFalseUsingSc,	outputInts2));
	cases.push_back(SpecConstantTwoIntCase("select",				"False %bool",	" %i32 0",		"%i32",		"Select               %sc_0 %sc_1 %zero",	1,		42,		addScToInput,		outputInts1));
	cases.push_back(SpecConstantTwoIntCase("sconvert",				" %i32 0",		" %i32 0",		"%i16",		"SConvert             %sc_0",				-11200,	0,		addSc32ToInput,		outputInts3));
	// -969998336 stored as 32-bit two's complement is the binary representation of -11200 as IEEE-754 Float
	cases.push_back(SpecConstantTwoIntCase("fconvert",				" %f32 0",		" %f32 0",		"%f64",		"FConvert             %sc_0",				-969998336, 0,	addSc32ToInput,		outputInts3));

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;
		ComputeTestFeatures		features = COMPUTE_TEST_USES_NONE;

		specializations["SC_DEF0"]			= cases[caseNdx].scDefinition0;
		specializations["SC_DEF1"]			= cases[caseNdx].scDefinition1;
		specializations["SC_RESULT_TYPE"]	= cases[caseNdx].scResultType;
		specializations["SC_OP"]			= cases[caseNdx].scOperation;
		specializations["GEN_RESULT"]		= cases[caseNdx].resultOperation;

		// Special SPIR-V code for SConvert-case
		if (strcmp(cases[caseNdx].caseName, "sconvert") == 0)
		{
			features								= COMPUTE_TEST_USES_INT16;
			specializations["CAPABILITIES"]			= "OpCapability Int16\n";							// Adds 16-bit integer capability
			specializations["OPTYPE_DEFINITIONS"]	= "%i16 = OpTypeInt 16 1\n";						// Adds 16-bit integer type
			specializations["TYPE_CONVERT"]			= "%sc_final32 = OpSConvert %i32 %sc_final\n";		// Converts 16-bit integer to 32-bit integer
		}

		// Special SPIR-V code for FConvert-case
		if (strcmp(cases[caseNdx].caseName, "fconvert") == 0)
		{
			features								= COMPUTE_TEST_USES_FLOAT64;
			specializations["CAPABILITIES"]			= "OpCapability Float64\n";							// Adds 64-bit float capability
			specializations["OPTYPE_DEFINITIONS"]	= "%f64 = OpTypeFloat 64\n";						// Adds 64-bit float type
			specializations["TYPE_CONVERT"]			= "%sc_final32 = OpConvertFToS %i32 %sc_final\n";	// Converts 64-bit float to 32-bit integer
		}

		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Int32Buffer(inputInts)));
		spec.outputs.push_back(BufferSp(new Int32Buffer(cases[caseNdx].expectedOutput)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);
		spec.specConstants.push_back(cases[caseNdx].scActualValue0);
		spec.specConstants.push_back(cases[caseNdx].scActualValue1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].caseName, cases[caseNdx].caseName, spec, features));
	}

	ComputeShaderSpec				spec;

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %sc_0  SpecId 0\n"
		"OpDecorate %sc_1  SpecId 1\n"
		"OpDecorate %sc_2  SpecId 2\n"
		"OpDecorate %i32arr ArrayStride 4\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"%ivec3       = OpTypeVector %i32 3\n"
		"%buf         = OpTypeStruct %i32arr\n"
		"%bufptr      = OpTypePointer Uniform %buf\n"
		"%indata      = OpVariable %bufptr Uniform\n"
		"%outdata     = OpVariable %bufptr Uniform\n"

		"%id          = OpVariable %uvec3ptr Input\n"
		"%zero        = OpConstant %i32 0\n"
		"%ivec3_0     = OpConstantComposite %ivec3 %zero %zero %zero\n"
		"%vec3_undef  = OpUndef %ivec3\n"

		"%sc_0        = OpSpecConstant %i32 0\n"
		"%sc_1        = OpSpecConstant %i32 0\n"
		"%sc_2        = OpSpecConstant %i32 0\n"
		"%sc_vec3_0   = OpSpecConstantOp %ivec3 CompositeInsert  %sc_0        %ivec3_0     0\n"							// (sc_0, 0, 0)
		"%sc_vec3_1   = OpSpecConstantOp %ivec3 CompositeInsert  %sc_1        %ivec3_0     1\n"							// (0, sc_1, 0)
		"%sc_vec3_2   = OpSpecConstantOp %ivec3 CompositeInsert  %sc_2        %ivec3_0     2\n"							// (0, 0, sc_2)
		"%sc_vec3_0_s = OpSpecConstantOp %ivec3 VectorShuffle    %sc_vec3_0   %vec3_undef  0          0xFFFFFFFF 2\n"	// (sc_0, ???,  0)
		"%sc_vec3_1_s = OpSpecConstantOp %ivec3 VectorShuffle    %sc_vec3_1   %vec3_undef  0xFFFFFFFF 1          0\n"	// (???,  sc_1, 0)
		"%sc_vec3_2_s = OpSpecConstantOp %ivec3 VectorShuffle    %vec3_undef  %sc_vec3_2   5          0xFFFFFFFF 5\n"	// (sc_2, ???,  sc_2)
		"%sc_vec3_01  = OpSpecConstantOp %ivec3 VectorShuffle    %sc_vec3_0_s %sc_vec3_1_s 1 0 4\n"						// (0,    sc_0, sc_1)
		"%sc_vec3_012 = OpSpecConstantOp %ivec3 VectorShuffle    %sc_vec3_01  %sc_vec3_2_s 5 1 2\n"						// (sc_2, sc_0, sc_1)
		"%sc_ext_0    = OpSpecConstantOp %i32   CompositeExtract %sc_vec3_012              0\n"							// sc_2
		"%sc_ext_1    = OpSpecConstantOp %i32   CompositeExtract %sc_vec3_012              1\n"							// sc_0
		"%sc_ext_2    = OpSpecConstantOp %i32   CompositeExtract %sc_vec3_012              2\n"							// sc_1
		"%sc_sub      = OpSpecConstantOp %i32   ISub             %sc_ext_0    %sc_ext_1\n"								// (sc_2 - sc_0)
		"%sc_final    = OpSpecConstantOp %i32   IMul             %sc_sub      %sc_ext_2\n"								// (sc_2 - sc_0) * sc_1

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %i32ptr %indata %zero %x\n"
		"%inval     = OpLoad %i32 %inloc\n"
		"%final     = OpIAdd %i32 %inval %sc_final\n"
		"%outloc    = OpAccessChain %i32ptr %outdata %zero %x\n"
		"             OpStore %outloc %final\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Int32Buffer(inputInts)));
	spec.outputs.push_back(BufferSp(new Int32Buffer(outputInts3)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);
	spec.specConstants.push_back(123);
	spec.specConstants.push_back(56);
	spec.specConstants.push_back(-77);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "vector_related", "VectorShuffle, CompositeExtract, & CompositeInsert", spec));

	return group.release();
}

void createOpPhiVartypeTests (de::MovePtr<tcu::TestCaseGroup>& group, tcu::TestContext& testCtx)
{
	ComputeShaderSpec	specInt;
	ComputeShaderSpec	specFloat;
	ComputeShaderSpec	specVec3;
	ComputeShaderSpec	specMat4;
	ComputeShaderSpec	specArray;
	ComputeShaderSpec	specStruct;
	de::Random			rnd				(deStringHash(group->getName()));
	const int			numElements		= 100;
	vector<float>		inputFloats		(numElements, 0);
	vector<float>		outputFloats	(numElements, 0);

	fillRandomScalars(rnd, -300.f, 300.f, &inputFloats[0], numElements);

	// CPU might not use the same rounding mode as the GPU. Use whole numbers to avoid rounding differences.
	floorAll(inputFloats);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		// Just check if the value is positive or not
		outputFloats[ndx] = (inputFloats[ndx] > 0) ? 1.0f : -1.0f;
	}

	// All of the tests are of the form:
	//
	// testtype r
	//
	// if (inputdata > 0)
	//   r = 1
	// else
	//   r = -1
	//
	// return (float)r

	specFloat.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%float_0    = OpConstant %f32 0.0\n"
		"%float_1    = OpConstant %f32 1.0\n"
		"%float_n1   = OpConstant %f32 -1.0\n"

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"

		"%comp     = OpFOrdGreaterThan %bool %inval %float_0\n"
		"            OpSelectionMerge %cm None\n"
		"            OpBranchConditional %comp %tb %fb\n"
		"%tb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%fb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%cm       = OpLabel\n"
		"%res      = OpPhi %f32 %float_1 %tb %float_n1 %fb\n"

		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %res\n"
		"            OpReturn\n"

		"            OpFunctionEnd\n";
	specFloat.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	specFloat.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	specFloat.numWorkGroups = IVec3(numElements, 1, 1);

	specMat4.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id = OpVariable %uvec3ptr Input\n"
		"%v4f32      = OpTypeVector %f32 4\n"
		"%mat4v4f32  = OpTypeMatrix %v4f32 4\n"
		"%zero       = OpConstant %i32 0\n"
		"%float_0    = OpConstant %f32 0.0\n"
		"%float_1    = OpConstant %f32 1.0\n"
		"%float_n1   = OpConstant %f32 -1.0\n"
		"%m11        = OpConstantComposite %v4f32 %float_1 %float_0 %float_0 %float_0\n"
		"%m12        = OpConstantComposite %v4f32 %float_0 %float_1 %float_0 %float_0\n"
		"%m13        = OpConstantComposite %v4f32 %float_0 %float_0 %float_1 %float_0\n"
		"%m14        = OpConstantComposite %v4f32 %float_0 %float_0 %float_0 %float_1\n"
		"%m1         = OpConstantComposite %mat4v4f32 %m11 %m12 %m13 %m14\n"
		"%m21        = OpConstantComposite %v4f32 %float_n1 %float_0 %float_0 %float_0\n"
		"%m22        = OpConstantComposite %v4f32 %float_0 %float_n1 %float_0 %float_0\n"
		"%m23        = OpConstantComposite %v4f32 %float_0 %float_0 %float_n1 %float_0\n"
		"%m24        = OpConstantComposite %v4f32 %float_0 %float_0 %float_0 %float_n1\n"
		"%m2         = OpConstantComposite %mat4v4f32 %m21 %m22 %m23 %m24\n"

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"

		"%comp     = OpFOrdGreaterThan %bool %inval %float_0\n"
		"            OpSelectionMerge %cm None\n"
		"            OpBranchConditional %comp %tb %fb\n"
		"%tb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%fb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%cm       = OpLabel\n"
		"%mres     = OpPhi %mat4v4f32 %m1 %tb %m2 %fb\n"
		"%res      = OpCompositeExtract %f32 %mres 2 2\n"

		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %res\n"
		"            OpReturn\n"

		"            OpFunctionEnd\n";
	specMat4.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	specMat4.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	specMat4.numWorkGroups = IVec3(numElements, 1, 1);

	specVec3.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%float_0    = OpConstant %f32 0.0\n"
		"%float_1    = OpConstant %f32 1.0\n"
		"%float_n1   = OpConstant %f32 -1.0\n"
		"%v1         = OpConstantComposite %fvec3 %float_1 %float_1 %float_1\n"
		"%v2         = OpConstantComposite %fvec3 %float_n1 %float_n1 %float_n1\n"

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"

		"%comp     = OpFOrdGreaterThan %bool %inval %float_0\n"
		"            OpSelectionMerge %cm None\n"
		"            OpBranchConditional %comp %tb %fb\n"
		"%tb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%fb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%cm       = OpLabel\n"
		"%vres     = OpPhi %fvec3 %v1 %tb %v2 %fb\n"
		"%res      = OpCompositeExtract %f32 %vres 2\n"

		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %res\n"
		"            OpReturn\n"

		"            OpFunctionEnd\n";
	specVec3.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	specVec3.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	specVec3.numWorkGroups = IVec3(numElements, 1, 1);

	specInt.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%float_0    = OpConstant %f32 0.0\n"
		"%i1         = OpConstant %i32 1\n"
		"%i2         = OpConstant %i32 -1\n"

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"

		"%comp     = OpFOrdGreaterThan %bool %inval %float_0\n"
		"            OpSelectionMerge %cm None\n"
		"            OpBranchConditional %comp %tb %fb\n"
		"%tb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%fb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%cm       = OpLabel\n"
		"%ires     = OpPhi %i32 %i1 %tb %i2 %fb\n"
		"%res      = OpConvertSToF %f32 %ires\n"

		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %res\n"
		"            OpReturn\n"

		"            OpFunctionEnd\n";
	specInt.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	specInt.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	specInt.numWorkGroups = IVec3(numElements, 1, 1);

	specArray.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%u7         = OpConstant %u32 7\n"
		"%float_0    = OpConstant %f32 0.0\n"
		"%float_1    = OpConstant %f32 1.0\n"
		"%float_n1   = OpConstant %f32 -1.0\n"
		"%f32a7      = OpTypeArray %f32 %u7\n"
		"%a1         = OpConstantComposite %f32a7 %float_1 %float_1 %float_1 %float_1 %float_1 %float_1 %float_1\n"
		"%a2         = OpConstantComposite %f32a7 %float_n1 %float_n1 %float_n1 %float_n1 %float_n1 %float_n1 %float_n1\n"
		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"

		"%comp     = OpFOrdGreaterThan %bool %inval %float_0\n"
		"            OpSelectionMerge %cm None\n"
		"            OpBranchConditional %comp %tb %fb\n"
		"%tb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%fb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%cm       = OpLabel\n"
		"%ares     = OpPhi %f32a7 %a1 %tb %a2 %fb\n"
		"%res      = OpCompositeExtract %f32 %ares 5\n"

		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %res\n"
		"            OpReturn\n"

		"            OpFunctionEnd\n";
	specArray.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	specArray.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	specArray.numWorkGroups = IVec3(numElements, 1, 1);

	specStruct.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%float_0    = OpConstant %f32 0.0\n"
		"%float_1    = OpConstant %f32 1.0\n"
		"%float_n1   = OpConstant %f32 -1.0\n"

		"%v2f32      = OpTypeVector %f32 2\n"
		"%Data2      = OpTypeStruct %f32 %v2f32\n"
		"%Data       = OpTypeStruct %Data2 %f32\n"

		"%in1a       = OpConstantComposite %v2f32 %float_1 %float_1\n"
		"%in1b       = OpConstantComposite %Data2 %float_1 %in1a\n"
		"%s1         = OpConstantComposite %Data %in1b %float_1\n"
		"%in2a       = OpConstantComposite %v2f32 %float_n1 %float_n1\n"
		"%in2b       = OpConstantComposite %Data2 %float_n1 %in2a\n"
		"%s2         = OpConstantComposite %Data %in2b %float_n1\n"

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"

		"%comp     = OpFOrdGreaterThan %bool %inval %float_0\n"
		"            OpSelectionMerge %cm None\n"
		"            OpBranchConditional %comp %tb %fb\n"
		"%tb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%fb       = OpLabel\n"
		"            OpBranch %cm\n"
		"%cm       = OpLabel\n"
		"%sres     = OpPhi %Data %s1 %tb %s2 %fb\n"
		"%res      = OpCompositeExtract %f32 %sres 0 0\n"

		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %res\n"
		"            OpReturn\n"

		"            OpFunctionEnd\n";
	specStruct.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	specStruct.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	specStruct.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "vartype_int", "OpPhi with int variables", specInt));
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "vartype_float", "OpPhi with float variables", specFloat));
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "vartype_vec3", "OpPhi with vec3 variables", specVec3));
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "vartype_mat4", "OpPhi with mat4 variables", specMat4));
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "vartype_array", "OpPhi with array variables", specArray));
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "vartype_struct", "OpPhi with struct variables", specStruct));
}

string generateConstantDefinitions (int count)
{
	std::ostringstream	r;
	for (int i = 0; i < count; i++)
		r << "%cf" << (i * 10 + 5) << " = OpConstant %f32 " <<(i * 10 + 5) << ".0\n";
	r << "\n";
	return r.str();
}

string generateSwitchCases (int count)
{
	std::ostringstream	r;
	for (int i = 0; i < count; i++)
		r << " " << i << " %case" << i;
	r << "\n";
	return r.str();
}

string generateSwitchTargets (int count)
{
	std::ostringstream	r;
	for (int i = 0; i < count; i++)
		r << "%case" << i << " = OpLabel\n            OpBranch %phi\n";
	r << "\n";
	return r.str();
}

string generateOpPhiParams (int count)
{
	std::ostringstream	r;
	for (int i = 0; i < count; i++)
		r << " %cf" << (i * 10 + 5) << " %case" << i;
	r << "\n";
	return r.str();
}

string generateIntWidth (int value)
{
	std::ostringstream	r;
	r << value;
	return r.str();
}

// Expand input string by injecting "ABC" between the input
// string characters. The acc/add/treshold parameters are used
// to skip some of the injections to make the result less
// uniform (and a lot shorter).
string expandOpPhiCase5 (const string& s, int &acc, int add, int treshold)
{
	std::ostringstream	res;
	const char*			p = s.c_str();

	while (*p)
	{
		res << *p;
		acc += add;
		if (acc > treshold)
		{
			acc -= treshold;
			res << "ABC";
		}
		p++;
	}
	return res.str();
}

// Calculate expected result based on the code string
float calcOpPhiCase5 (float val, const string& s)
{
	const char*		p		= s.c_str();
	float			x[8];
	bool			b[8];
	const float		tv[8]	= { 0.5f, 1.5f, 3.5f, 7.5f, 15.5f, 31.5f, 63.5f, 127.5f };
	const float		v		= deFloatAbs(val);
	float			res		= 0;
	int				depth	= -1;
	int				skip	= 0;

	for (int i = 7; i >= 0; --i)
		x[i] = std::fmod((float)v, (float)(2 << i));
	for (int i = 7; i >= 0; --i)
		b[i] = x[i] > tv[i];

	while (*p)
	{
		if (*p == 'A')
		{
			depth++;
			if (skip == 0 && b[depth])
			{
				res++;
			}
			else
				skip++;
		}
		if (*p == 'B')
		{
			if (skip)
				skip--;
			if (b[depth] || skip)
				skip++;
		}
		if (*p == 'C')
		{
			depth--;
			if (skip)
				skip--;
		}
		p++;
	}
	return res;
}

// In the code string, the letters represent the following:
//
// A:
//     if (certain bit is set)
//     {
//       result++;
//
// B:
//     } else {
//
// C:
//     }
//
// examples:
// AABCBC leads to if(){r++;if(){r++;}else{}}else{}
// ABABCC leads to if(){r++;}else{if(){r++;}else{}}
// ABCABC leads to if(){r++;}else{}if(){r++;}else{}
//
// Code generation gets a bit complicated due to the else-branches,
// which do not generate new values. Thus, the generator needs to
// keep track of the previous variable change seen by the else
// branch.
string generateOpPhiCase5 (const string& s)
{
	std::stack<int>				idStack;
	std::stack<std::string>		value;
	std::stack<std::string>		valueLabel;
	std::stack<std::string>		mergeLeft;
	std::stack<std::string>		mergeRight;
	std::ostringstream			res;
	const char*					p			= s.c_str();
	int							depth		= -1;
	int							currId		= 0;
	int							iter		= 0;

	idStack.push(-1);
	value.push("%f32_0");
	valueLabel.push("%f32_0 %entry");

	while (*p)
	{
		if (*p == 'A')
		{
			depth++;
			currId = iter;
			idStack.push(currId);
			res << "\tOpSelectionMerge %m" << currId << " None\n";
			res << "\tOpBranchConditional %b" << depth << " %t" << currId << " %f" << currId << "\n";
			res << "%t" << currId << " = OpLabel\n";
			res << "%rt" << currId << " = OpFAdd %f32 " << value.top() << " %f32_1\n";
			std::ostringstream tag;
			tag << "%rt" << currId;
			value.push(tag.str());
			tag << " %t" << currId;
			valueLabel.push(tag.str());
		}

		if (*p == 'B')
		{
			mergeLeft.push(valueLabel.top());
			value.pop();
			valueLabel.pop();
			res << "\tOpBranch %m" << currId << "\n";
			res << "%f" << currId << " = OpLabel\n";
			std::ostringstream tag;
			tag << value.top() << " %f" << currId;
			valueLabel.pop();
			valueLabel.push(tag.str());
		}

		if (*p == 'C')
		{
			mergeRight.push(valueLabel.top());
			res << "\tOpBranch %m" << currId << "\n";
			res << "%m" << currId << " = OpLabel\n";
			if (*(p + 1) == 0)
				res << "%res"; // last result goes to %res
			else
				res << "%rm" << currId;
			res << " = OpPhi %f32  " << mergeLeft.top() << "  " << mergeRight.top() << "\n";
			std::ostringstream tag;
			tag << "%rm" << currId;
			value.pop();
			value.push(tag.str());
			tag << " %m" << currId;
			valueLabel.pop();
			valueLabel.push(tag.str());
			mergeLeft.pop();
			mergeRight.pop();
			depth--;
			idStack.pop();
			currId = idStack.top();
		}
		p++;
		iter++;
	}
	return res.str();
}

tcu::TestCaseGroup* createOpPhiGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opphi", "Test the OpPhi instruction"));
	ComputeShaderSpec				spec1;
	ComputeShaderSpec				spec2;
	ComputeShaderSpec				spec3;
	ComputeShaderSpec				spec4;
	ComputeShaderSpec				spec5;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats1	(numElements, 0);
	vector<float>					outputFloats2	(numElements, 0);
	vector<float>					outputFloats3	(numElements, 0);
	vector<float>					outputFloats4	(numElements, 0);
	vector<float>					outputFloats5	(numElements, 0);
	std::string						codestring		= "ABC";
	const int						test4Width		= 1024;

	// Build case 5 code string. Each iteration makes the hierarchy more complicated.
	// 9 iterations with (7, 24) parameters makes the hierarchy 8 deep with about 1500 lines of
	// shader code.
	for (int i = 0, acc = 0; i < 9; i++)
		codestring = expandOpPhiCase5(codestring, acc, 7, 24);

	fillRandomScalars(rnd, -300.f, 300.f, &inputFloats[0], numElements);

	// CPU might not use the same rounding mode as the GPU. Use whole numbers to avoid rounding differences.
	floorAll(inputFloats);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		switch (ndx % 3)
		{
			case 0:		outputFloats1[ndx] = inputFloats[ndx] + 5.5f;	break;
			case 1:		outputFloats1[ndx] = inputFloats[ndx] + 20.5f;	break;
			case 2:		outputFloats1[ndx] = inputFloats[ndx] + 1.75f;	break;
			default:	break;
		}
		outputFloats2[ndx] = inputFloats[ndx] + 6.5f * 3;
		outputFloats3[ndx] = 8.5f - inputFloats[ndx];

		int index4 = (int)deFloor(deAbs((float)ndx * inputFloats[ndx]));
		outputFloats4[ndx] = (float)(index4 % test4Width) * 10.0f + 5.0f;

		outputFloats5[ndx] = calcOpPhiCase5(inputFloats[ndx], codestring);
	}

	spec1.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%three      = OpConstant %u32 3\n"
		"%constf5p5  = OpConstant %f32 5.5\n"
		"%constf20p5 = OpConstant %f32 20.5\n"
		"%constf1p75 = OpConstant %f32 1.75\n"
		"%constf8p5  = OpConstant %f32 8.5\n"
		"%constf6p5  = OpConstant %f32 6.5\n"

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%selector = OpUMod %u32 %x %three\n"
		"            OpSelectionMerge %phi None\n"
		"            OpSwitch %selector %default 0 %case0 1 %case1 2 %case2\n"

		// Case 1 before OpPhi.
		"%case1    = OpLabel\n"
		"            OpBranch %phi\n"

		"%default  = OpLabel\n"
		"            OpUnreachable\n"

		"%phi      = OpLabel\n"
		"%operand  = OpPhi %f32   %constf1p75 %case2   %constf20p5 %case1   %constf5p5 %case0\n" // not in the order of blocks
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"
		"%add      = OpFAdd %f32 %inval %operand\n"
		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %add\n"
		"            OpReturn\n"

		// Case 0 after OpPhi.
		"%case0    = OpLabel\n"
		"            OpBranch %phi\n"


		// Case 2 after OpPhi.
		"%case2    = OpLabel\n"
		"            OpBranch %phi\n"

		"            OpFunctionEnd\n";
	spec1.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec1.outputs.push_back(BufferSp(new Float32Buffer(outputFloats1)));
	spec1.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "block", "out-of-order and unreachable blocks for OpPhi", spec1));

	spec2.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id         = OpVariable %uvec3ptr Input\n"
		"%zero       = OpConstant %i32 0\n"
		"%one        = OpConstant %i32 1\n"
		"%three      = OpConstant %i32 3\n"
		"%constf6p5  = OpConstant %f32 6.5\n"

		"%main       = OpFunction %void None %voidf\n"
		"%entry      = OpLabel\n"
		"%idval      = OpLoad %uvec3 %id\n"
		"%x          = OpCompositeExtract %u32 %idval 0\n"
		"%inloc      = OpAccessChain %f32ptr %indata %zero %x\n"
		"%outloc     = OpAccessChain %f32ptr %outdata %zero %x\n"
		"%inval      = OpLoad %f32 %inloc\n"
		"              OpBranch %phi\n"

		"%phi        = OpLabel\n"
		"%step       = OpPhi %i32 %zero  %entry %step_next  %phi\n"
		"%accum      = OpPhi %f32 %inval %entry %accum_next %phi\n"
		"%step_next  = OpIAdd %i32 %step %one\n"
		"%accum_next = OpFAdd %f32 %accum %constf6p5\n"
		"%still_loop = OpSLessThan %bool %step %three\n"
		"              OpLoopMerge %exit %phi None\n"
		"              OpBranchConditional %still_loop %phi %exit\n"

		"%exit       = OpLabel\n"
		"              OpStore %outloc %accum\n"
		"              OpReturn\n"
		"              OpFunctionEnd\n";
	spec2.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec2.outputs.push_back(BufferSp(new Float32Buffer(outputFloats2)));
	spec2.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "induction", "The usual way induction variables are handled in LLVM IR", spec2));

	spec3.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%f32ptr_f   = OpTypePointer Function %f32\n"
		"%id         = OpVariable %uvec3ptr Input\n"
		"%true       = OpConstantTrue %bool\n"
		"%false      = OpConstantFalse %bool\n"
		"%zero       = OpConstant %i32 0\n"
		"%constf8p5  = OpConstant %f32 8.5\n"

		"%main       = OpFunction %void None %voidf\n"
		"%entry      = OpLabel\n"
		"%b          = OpVariable %f32ptr_f Function %constf8p5\n"
		"%idval      = OpLoad %uvec3 %id\n"
		"%x          = OpCompositeExtract %u32 %idval 0\n"
		"%inloc      = OpAccessChain %f32ptr %indata %zero %x\n"
		"%outloc     = OpAccessChain %f32ptr %outdata %zero %x\n"
		"%a_init     = OpLoad %f32 %inloc\n"
		"%b_init     = OpLoad %f32 %b\n"
		"              OpBranch %phi\n"

		"%phi        = OpLabel\n"
		"%still_loop = OpPhi %bool %true   %entry %false  %phi\n"
		"%a_next     = OpPhi %f32  %a_init %entry %b_next %phi\n"
		"%b_next     = OpPhi %f32  %b_init %entry %a_next %phi\n"
		"              OpLoopMerge %exit %phi None\n"
		"              OpBranchConditional %still_loop %phi %exit\n"

		"%exit       = OpLabel\n"
		"%sub        = OpFSub %f32 %a_next %b_next\n"
		"              OpStore %outloc %sub\n"
		"              OpReturn\n"
		"              OpFunctionEnd\n";
	spec3.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec3.outputs.push_back(BufferSp(new Float32Buffer(outputFloats3)));
	spec3.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "swap", "Swap the values of two variables using OpPhi", spec3));

	spec4.assembly =
		"OpCapability Shader\n"
		"%ext = OpExtInstImport \"GLSL.std.450\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id       = OpVariable %uvec3ptr Input\n"
		"%zero     = OpConstant %i32 0\n"
		"%cimod    = OpConstant %u32 " + generateIntWidth(test4Width) + "\n"

		+ generateConstantDefinitions(test4Width) +

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"
		"%xf       = OpConvertUToF %f32 %x\n"
		"%xm       = OpFMul %f32 %xf %inval\n"
		"%xa       = OpExtInst %f32 %ext FAbs %xm\n"
		"%xi       = OpConvertFToU %u32 %xa\n"
		"%selector = OpUMod %u32 %xi %cimod\n"
		"            OpSelectionMerge %phi None\n"
		"            OpSwitch %selector %default "

		+ generateSwitchCases(test4Width) +

		"%default  = OpLabel\n"
		"            OpUnreachable\n"

		+ generateSwitchTargets(test4Width) +

		"%phi      = OpLabel\n"
		"%result   = OpPhi %f32"

		+ generateOpPhiParams(test4Width) +

		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %result\n"
		"            OpReturn\n"

		"            OpFunctionEnd\n";
	spec4.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec4.outputs.push_back(BufferSp(new Float32Buffer(outputFloats4)));
	spec4.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "wide", "OpPhi with a lot of parameters", spec4));

	spec5.assembly =
		"OpCapability Shader\n"
		"%ext      = OpExtInstImport \"GLSL.std.450\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"
		"%code     = OpString \"" + codestring + "\"\n"

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id       = OpVariable %uvec3ptr Input\n"
		"%zero     = OpConstant %i32 0\n"
		"%f32_0    = OpConstant %f32 0.0\n"
		"%f32_0_5  = OpConstant %f32 0.5\n"
		"%f32_1    = OpConstant %f32 1.0\n"
		"%f32_1_5  = OpConstant %f32 1.5\n"
		"%f32_2    = OpConstant %f32 2.0\n"
		"%f32_3_5  = OpConstant %f32 3.5\n"
		"%f32_4    = OpConstant %f32 4.0\n"
		"%f32_7_5  = OpConstant %f32 7.5\n"
		"%f32_8    = OpConstant %f32 8.0\n"
		"%f32_15_5 = OpConstant %f32 15.5\n"
		"%f32_16   = OpConstant %f32 16.0\n"
		"%f32_31_5 = OpConstant %f32 31.5\n"
		"%f32_32   = OpConstant %f32 32.0\n"
		"%f32_63_5 = OpConstant %f32 63.5\n"
		"%f32_64   = OpConstant %f32 64.0\n"
		"%f32_127_5 = OpConstant %f32 127.5\n"
		"%f32_128  = OpConstant %f32 128.0\n"
		"%f32_256  = OpConstant %f32 256.0\n"

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"

		"%xabs     = OpExtInst %f32 %ext FAbs %inval\n"
		"%x8       = OpFMod %f32 %xabs %f32_256\n"
		"%x7       = OpFMod %f32 %xabs %f32_128\n"
		"%x6       = OpFMod %f32 %xabs %f32_64\n"
		"%x5       = OpFMod %f32 %xabs %f32_32\n"
		"%x4       = OpFMod %f32 %xabs %f32_16\n"
		"%x3       = OpFMod %f32 %xabs %f32_8\n"
		"%x2       = OpFMod %f32 %xabs %f32_4\n"
		"%x1       = OpFMod %f32 %xabs %f32_2\n"

		"%b7       = OpFOrdGreaterThanEqual %bool %x8 %f32_127_5\n"
		"%b6       = OpFOrdGreaterThanEqual %bool %x7 %f32_63_5\n"
		"%b5       = OpFOrdGreaterThanEqual %bool %x6 %f32_31_5\n"
		"%b4       = OpFOrdGreaterThanEqual %bool %x5 %f32_15_5\n"
		"%b3       = OpFOrdGreaterThanEqual %bool %x4 %f32_7_5\n"
		"%b2       = OpFOrdGreaterThanEqual %bool %x3 %f32_3_5\n"
		"%b1       = OpFOrdGreaterThanEqual %bool %x2 %f32_1_5\n"
		"%b0       = OpFOrdGreaterThanEqual %bool %x1 %f32_0_5\n"

		+ generateOpPhiCase5(codestring) +

		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"            OpStore %outloc %res\n"
		"            OpReturn\n"

		"            OpFunctionEnd\n";
	spec5.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec5.outputs.push_back(BufferSp(new Float32Buffer(outputFloats5)));
	spec5.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "nested", "Stress OpPhi with a lot of nesting", spec5));

	createOpPhiVartypeTests(group, testCtx);

	return group.release();
}

// Assembly code used for testing block order is based on GLSL source code:
//
// #version 430
//
// layout(std140, set = 0, binding = 0) readonly buffer Input {
//   float elements[];
// } input_data;
// layout(std140, set = 0, binding = 1) writeonly buffer Output {
//   float elements[];
// } output_data;
//
// void main() {
//   uint x = gl_GlobalInvocationID.x;
//   output_data.elements[x] = input_data.elements[x];
//   if (x > uint(50)) {
//     switch (x % uint(3)) {
//       case 0: output_data.elements[x] += 1.5f; break;
//       case 1: output_data.elements[x] += 42.f; break;
//       case 2: output_data.elements[x] -= 27.f; break;
//       default: break;
//     }
//   } else {
//     output_data.elements[x] = -input_data.elements[x];
//   }
// }
tcu::TestCaseGroup* createBlockOrderGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "block_order", "Test block orders"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);

	fillRandomScalars(rnd, -100.f, 100.f, &inputFloats[0], numElements);

	// CPU might not use the same rounding mode as the GPU. Use whole numbers to avoid rounding differences.
	floorAll(inputFloats);

	for (size_t ndx = 0; ndx <= 50; ++ndx)
		outputFloats[ndx] = -inputFloats[ndx];

	for (size_t ndx = 51; ndx < numElements; ++ndx)
	{
		switch (ndx % 3)
		{
			case 0:		outputFloats[ndx] = inputFloats[ndx] + 1.5f; break;
			case 1:		outputFloats[ndx] = inputFloats[ndx] + 42.f; break;
			case 2:		outputFloats[ndx] = inputFloats[ndx] - 27.f; break;
			default:	break;
		}
	}

	spec.assembly =
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"%u32ptr       = OpTypePointer Function %u32\n"
		"%u32ptr_input = OpTypePointer Input %u32\n"

		+ string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"
		"%const3    = OpConstant %u32 3\n"
		"%const50   = OpConstant %u32 50\n"
		"%constf1p5 = OpConstant %f32 1.5\n"
		"%constf27  = OpConstant %f32 27.0\n"
		"%constf42  = OpConstant %f32 42.0\n"

		"%main = OpFunction %void None %voidf\n"

		// entry block.
		"%entry    = OpLabel\n"

		// Create a temporary variable to hold the value of gl_GlobalInvocationID.x.
		"%xvar     = OpVariable %u32ptr Function\n"
		"%xptr     = OpAccessChain %u32ptr_input %id %zero\n"
		"%x        = OpLoad %u32 %xptr\n"
		"            OpStore %xvar %x\n"

		"%cmp      = OpUGreaterThan %bool %x %const50\n"
		"            OpSelectionMerge %if_merge None\n"
		"            OpBranchConditional %cmp %if_true %if_false\n"

		// False branch for if-statement: placed in the middle of switch cases and before true branch.
		"%if_false = OpLabel\n"
		"%x_f      = OpLoad %u32 %xvar\n"
		"%inloc_f  = OpAccessChain %f32ptr %indata %zero %x_f\n"
		"%inval_f  = OpLoad %f32 %inloc_f\n"
		"%negate   = OpFNegate %f32 %inval_f\n"
		"%outloc_f = OpAccessChain %f32ptr %outdata %zero %x_f\n"
		"            OpStore %outloc_f %negate\n"
		"            OpBranch %if_merge\n"

		// Merge block for if-statement: placed in the middle of true and false branch.
		"%if_merge = OpLabel\n"
		"            OpReturn\n"

		// True branch for if-statement: placed in the middle of swtich cases and after the false branch.
		"%if_true  = OpLabel\n"
		"%xval_t   = OpLoad %u32 %xvar\n"
		"%mod      = OpUMod %u32 %xval_t %const3\n"
		"            OpSelectionMerge %switch_merge None\n"
		"            OpSwitch %mod %default 0 %case0 1 %case1 2 %case2\n"

		// Merge block for switch-statement: placed before the case
                // bodies.  But it must follow OpSwitch which dominates it.
		"%switch_merge = OpLabel\n"
		"                OpBranch %if_merge\n"

		// Case 1 for switch-statement: placed before case 0.
                // It must follow the OpSwitch that dominates it.
		"%case1    = OpLabel\n"
		"%x_1      = OpLoad %u32 %xvar\n"
		"%inloc_1  = OpAccessChain %f32ptr %indata %zero %x_1\n"
		"%inval_1  = OpLoad %f32 %inloc_1\n"
		"%addf42   = OpFAdd %f32 %inval_1 %constf42\n"
		"%outloc_1 = OpAccessChain %f32ptr %outdata %zero %x_1\n"
		"            OpStore %outloc_1 %addf42\n"
		"            OpBranch %switch_merge\n"

		// Case 2 for switch-statement.
		"%case2    = OpLabel\n"
		"%x_2      = OpLoad %u32 %xvar\n"
		"%inloc_2  = OpAccessChain %f32ptr %indata %zero %x_2\n"
		"%inval_2  = OpLoad %f32 %inloc_2\n"
		"%subf27   = OpFSub %f32 %inval_2 %constf27\n"
		"%outloc_2 = OpAccessChain %f32ptr %outdata %zero %x_2\n"
		"            OpStore %outloc_2 %subf27\n"
		"            OpBranch %switch_merge\n"

		// Default case for switch-statement: placed in the middle of normal cases.
		"%default = OpLabel\n"
		"           OpBranch %switch_merge\n"

		// Case 0 for switch-statement: out of order.
		"%case0    = OpLabel\n"
		"%x_0      = OpLoad %u32 %xvar\n"
		"%inloc_0  = OpAccessChain %f32ptr %indata %zero %x_0\n"
		"%inval_0  = OpLoad %f32 %inloc_0\n"
		"%addf1p5  = OpFAdd %f32 %inval_0 %constf1p5\n"
		"%outloc_0 = OpAccessChain %f32ptr %outdata %zero %x_0\n"
		"            OpStore %outloc_0 %addf1p5\n"
		"            OpBranch %switch_merge\n"

		"            OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "all", "various out-of-order blocks", spec));

	return group.release();
}

tcu::TestCaseGroup* createMultipleShaderGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "multiple_shaders", "Test multiple shaders in the same module"));
	ComputeShaderSpec				spec1;
	ComputeShaderSpec				spec2;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats1	(numElements, 0);
	vector<float>					outputFloats2	(numElements, 0);
	fillRandomScalars(rnd, -500.f, 500.f, &inputFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
	{
		outputFloats1[ndx] = inputFloats[ndx] + inputFloats[ndx];
		outputFloats2[ndx] = -inputFloats[ndx];
	}

	const string assembly(
		"OpCapability Shader\n"
		"OpCapability ClipDistance\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %comp_main1 \"entrypoint1\" %id\n"
		"OpEntryPoint GLCompute %comp_main2 \"entrypoint2\" %id\n"
		// A module cannot have two OpEntryPoint instructions with the same Execution Model and the same Name string.
		"OpEntryPoint Vertex    %vert_main  \"entrypoint2\" %vert_builtins %vertexIndex %instanceIndex\n"
		"OpExecutionMode %comp_main1 LocalSize 1 1 1\n"
		"OpExecutionMode %comp_main2 LocalSize 1 1 1\n"

		"OpName %comp_main1              \"entrypoint1\"\n"
		"OpName %comp_main2              \"entrypoint2\"\n"
		"OpName %vert_main               \"entrypoint2\"\n"
		"OpName %id                      \"gl_GlobalInvocationID\"\n"
		"OpName %vert_builtin_st         \"gl_PerVertex\"\n"
		"OpName %vertexIndex             \"gl_VertexIndex\"\n"
		"OpName %instanceIndex           \"gl_InstanceIndex\"\n"
		"OpMemberName %vert_builtin_st 0 \"gl_Position\"\n"
		"OpMemberName %vert_builtin_st 1 \"gl_PointSize\"\n"
		"OpMemberName %vert_builtin_st 2 \"gl_ClipDistance\"\n"

		"OpDecorate %id                      BuiltIn GlobalInvocationId\n"
		"OpDecorate %vertexIndex             BuiltIn VertexIndex\n"
		"OpDecorate %instanceIndex           BuiltIn InstanceIndex\n"
		"OpDecorate %vert_builtin_st         Block\n"
		"OpMemberDecorate %vert_builtin_st 0 BuiltIn Position\n"
		"OpMemberDecorate %vert_builtin_st 1 BuiltIn PointSize\n"
		"OpMemberDecorate %vert_builtin_st 2 BuiltIn ClipDistance\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%zero       = OpConstant %i32 0\n"
		"%one        = OpConstant %u32 1\n"
		"%c_f32_1    = OpConstant %f32 1\n"

		"%i32inputptr         = OpTypePointer Input %i32\n"
		"%vec4                = OpTypeVector %f32 4\n"
		"%vec4ptr             = OpTypePointer Output %vec4\n"
		"%f32arr1             = OpTypeArray %f32 %one\n"
		"%vert_builtin_st     = OpTypeStruct %vec4 %f32 %f32arr1\n"
		"%vert_builtin_st_ptr = OpTypePointer Output %vert_builtin_st\n"
		"%vert_builtins       = OpVariable %vert_builtin_st_ptr Output\n"

		"%id         = OpVariable %uvec3ptr Input\n"
		"%vertexIndex = OpVariable %i32inputptr Input\n"
		"%instanceIndex = OpVariable %i32inputptr Input\n"
		"%c_vec4_1   = OpConstantComposite %vec4 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_1\n"

		// gl_Position = vec4(1.);
		"%vert_main  = OpFunction %void None %voidf\n"
		"%vert_entry = OpLabel\n"
		"%position   = OpAccessChain %vec4ptr %vert_builtins %zero\n"
		"              OpStore %position %c_vec4_1\n"
		"              OpReturn\n"
		"              OpFunctionEnd\n"

		// Double inputs.
		"%comp_main1  = OpFunction %void None %voidf\n"
		"%comp1_entry = OpLabel\n"
		"%idval1      = OpLoad %uvec3 %id\n"
		"%x1          = OpCompositeExtract %u32 %idval1 0\n"
		"%inloc1      = OpAccessChain %f32ptr %indata %zero %x1\n"
		"%inval1      = OpLoad %f32 %inloc1\n"
		"%add         = OpFAdd %f32 %inval1 %inval1\n"
		"%outloc1     = OpAccessChain %f32ptr %outdata %zero %x1\n"
		"               OpStore %outloc1 %add\n"
		"               OpReturn\n"
		"               OpFunctionEnd\n"

		// Negate inputs.
		"%comp_main2  = OpFunction %void None %voidf\n"
		"%comp2_entry = OpLabel\n"
		"%idval2      = OpLoad %uvec3 %id\n"
		"%x2          = OpCompositeExtract %u32 %idval2 0\n"
		"%inloc2      = OpAccessChain %f32ptr %indata %zero %x2\n"
		"%inval2      = OpLoad %f32 %inloc2\n"
		"%neg         = OpFNegate %f32 %inval2\n"
		"%outloc2     = OpAccessChain %f32ptr %outdata %zero %x2\n"
		"               OpStore %outloc2 %neg\n"
		"               OpReturn\n"
		"               OpFunctionEnd\n");

	spec1.assembly = assembly;
	spec1.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec1.outputs.push_back(BufferSp(new Float32Buffer(outputFloats1)));
	spec1.numWorkGroups = IVec3(numElements, 1, 1);
	spec1.entryPoint = "entrypoint1";

	spec2.assembly = assembly;
	spec2.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
	spec2.outputs.push_back(BufferSp(new Float32Buffer(outputFloats2)));
	spec2.numWorkGroups = IVec3(numElements, 1, 1);
	spec2.entryPoint = "entrypoint2";

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "shader1", "multiple shaders in the same module", spec1));
	group->addChild(new SpvAsmComputeShaderCase(testCtx, "shader2", "multiple shaders in the same module", spec2));

	return group.release();
}

inline std::string makeLongUTF8String (size_t num4ByteChars)
{
	// An example of a longest valid UTF-8 character.  Be explicit about the
	// character type because Microsoft compilers can otherwise interpret the
	// character string as being over wide (16-bit) characters. Ideally, we
	// would just use a C++11 UTF-8 string literal, but we want to support older
	// Microsoft compilers.
	const std::basic_string<char> earthAfrica("\xF0\x9F\x8C\x8D");
	std::string longString;
	longString.reserve(num4ByteChars * 4);
	for (size_t count = 0; count < num4ByteChars; count++)
	{
		longString += earthAfrica;
	}
	return longString;
}

tcu::TestCaseGroup* createOpSourceGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opsource", "Tests the OpSource & OpSourceContinued instruction"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		"OpCapability Shader\n"
		"OpMemoryModel Logical GLSL450\n"

		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"${SOURCE}\n"

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	cases.push_back(CaseParameter("unknown_source",							"OpSource Unknown 0"));
	cases.push_back(CaseParameter("wrong_source",							"OpSource OpenCL_C 210"));
	cases.push_back(CaseParameter("normal_filename",						"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname"));
	cases.push_back(CaseParameter("empty_filename",							"%fname = OpString \"\"\n"
																			"OpSource GLSL 430 %fname"));
	cases.push_back(CaseParameter("normal_source_code",						"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"#version 430\nvoid main() {}\""));
	cases.push_back(CaseParameter("empty_source_code",						"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"\""));
	cases.push_back(CaseParameter("long_source_code",						"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"" + makeLongUTF8String(65530) + "ccc\"")); // word count: 65535
	cases.push_back(CaseParameter("utf8_source_code",						"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"\xE2\x98\x82\xE2\x98\x85\"")); // umbrella & black star symbol
	cases.push_back(CaseParameter("normal_sourcecontinued",					"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"#version 430\nvo\"\n"
																			"OpSourceContinued \"id main() {}\""));
	cases.push_back(CaseParameter("empty_sourcecontinued",					"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"#version 430\nvoid main() {}\"\n"
																			"OpSourceContinued \"\""));
	cases.push_back(CaseParameter("long_sourcecontinued",					"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"#version 430\nvoid main() {}\"\n"
																			"OpSourceContinued \"" + makeLongUTF8String(65533) + "ccc\"")); // word count: 65535
	cases.push_back(CaseParameter("utf8_sourcecontinued",					"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"#version 430\nvoid main() {}\"\n"
																			"OpSourceContinued \"\xE2\x98\x8E\xE2\x9A\x91\"")); // white telephone & black flag symbol
	cases.push_back(CaseParameter("multi_sourcecontinued",					"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"#version 430\n\"\n"
																			"OpSourceContinued \"void\"\n"
																			"OpSourceContinued \"main()\"\n"
																			"OpSourceContinued \"{}\""));
	cases.push_back(CaseParameter("empty_source_before_sourcecontinued",	"%fname = OpString \"filename\"\n"
																			"OpSource GLSL 430 %fname \"\"\n"
																			"OpSourceContinued \"#version 430\nvoid main() {}\""));

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["SOURCE"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

tcu::TestCaseGroup* createOpSourceExtensionGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opsourceextension", "Tests the OpSource instruction"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpSourceExtension \"${EXTENSION}\"\n"

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	cases.push_back(CaseParameter("empty_extension",	""));
	cases.push_back(CaseParameter("real_extension",		"GL_ARB_texture_rectangle"));
	cases.push_back(CaseParameter("fake_extension",		"GL_ARB_im_the_ultimate_extension"));
	cases.push_back(CaseParameter("utf8_extension",		"GL_ARB_\xE2\x98\x82\xE2\x98\x85"));
	cases.push_back(CaseParameter("long_extension",		makeLongUTF8String(65533) + "ccc")); // word count: 65535

	fillRandomScalars(rnd, -200.f, 200.f, &inputFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats[ndx] = -inputFloats[ndx];

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["EXTENSION"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

// Checks that a compute shader can generate a constant null value of various types, without exercising a computation on it.
tcu::TestCaseGroup* createOpConstantNullGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opconstantnull", "Tests the OpConstantNull instruction"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +
		"%uvec2     = OpTypeVector %u32 2\n"
		"%bvec3     = OpTypeVector %bool 3\n"
		"%fvec4     = OpTypeVector %f32 4\n"
		"%fmat33    = OpTypeMatrix %fvec3 3\n"
		"%const100  = OpConstant %u32 100\n"
		"%uarr100   = OpTypeArray %i32 %const100\n"
		"%struct    = OpTypeStruct %f32 %i32 %u32\n"
		"%pointer   = OpTypePointer Function %i32\n"
		+ string(getComputeAsmInputOutputBuffer()) +

		"%null      = OpConstantNull ${TYPE}\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	cases.push_back(CaseParameter("bool",			"%bool"));
	cases.push_back(CaseParameter("sint32",			"%i32"));
	cases.push_back(CaseParameter("uint32",			"%u32"));
	cases.push_back(CaseParameter("float32",		"%f32"));
	cases.push_back(CaseParameter("vec4float32",	"%fvec4"));
	cases.push_back(CaseParameter("vec3bool",		"%bvec3"));
	cases.push_back(CaseParameter("vec2uint32",		"%uvec2"));
	cases.push_back(CaseParameter("matrix",			"%fmat33"));
	cases.push_back(CaseParameter("array",			"%uarr100"));
	cases.push_back(CaseParameter("struct",			"%struct"));
	cases.push_back(CaseParameter("pointer",		"%pointer"));

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["TYPE"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

// Checks that a compute shader can generate a constant composite value of various types, without exercising a computation on it.
tcu::TestCaseGroup* createOpConstantCompositeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opconstantcomposite", "Tests the OpConstantComposite instruction"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"${CONSTANT}\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	cases.push_back(CaseParameter("vector",			"%five = OpConstant %u32 5\n"
													"%const = OpConstantComposite %uvec3 %five %zero %five"));
	cases.push_back(CaseParameter("matrix",			"%m3fvec3 = OpTypeMatrix %fvec3 3\n"
													"%ten = OpConstant %f32 10.\n"
													"%fzero = OpConstant %f32 0.\n"
													"%vec = OpConstantComposite %fvec3 %ten %fzero %ten\n"
													"%mat = OpConstantComposite %m3fvec3 %vec %vec %vec"));
	cases.push_back(CaseParameter("struct",			"%m2vec3 = OpTypeMatrix %fvec3 2\n"
													"%struct = OpTypeStruct %i32 %f32 %fvec3 %m2vec3\n"
													"%fzero = OpConstant %f32 0.\n"
													"%one = OpConstant %f32 1.\n"
													"%point5 = OpConstant %f32 0.5\n"
													"%vec = OpConstantComposite %fvec3 %one %one %fzero\n"
													"%mat = OpConstantComposite %m2vec3 %vec %vec\n"
													"%const = OpConstantComposite %struct %zero %point5 %vec %mat"));
	cases.push_back(CaseParameter("nested_struct",	"%st1 = OpTypeStruct %u32 %f32\n"
													"%st2 = OpTypeStruct %i32 %i32\n"
													"%struct = OpTypeStruct %st1 %st2\n"
													"%point5 = OpConstant %f32 0.5\n"
													"%one = OpConstant %u32 1\n"
													"%ten = OpConstant %i32 10\n"
													"%st1val = OpConstantComposite %st1 %one %point5\n"
													"%st2val = OpConstantComposite %st2 %ten %ten\n"
													"%const = OpConstantComposite %struct %st1val %st2val"));

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["CONSTANT"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

// Creates a floating point number with the given exponent, and significand
// bits set. It can only create normalized numbers. Only the least significant
// 24 bits of the significand will be examined. The final bit of the
// significand will also be ignored. This allows alignment to be written
// similarly to C99 hex-floats.
// For example if you wanted to write 0x1.7f34p-12 you would call
// constructNormalizedFloat(-12, 0x7f3400)
float constructNormalizedFloat (deInt32 exponent, deUint32 significand)
{
	float f = 1.0f;

	for (deInt32 idx = 0; idx < 23; ++idx)
	{
		f += ((significand & 0x800000) == 0) ? 0.f : std::ldexp(1.0f, -(idx + 1));
		significand <<= 1;
	}

	return std::ldexp(f, exponent);
}

// Compare instruction for the OpQuantizeF16 compute exact case.
// Returns true if the output is what is expected from the test case.
bool compareOpQuantizeF16ComputeExactCase (const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog&)
{
	if (outputAllocs.size() != 1)
		return false;

	// Only size is needed because we cannot compare Nans.
	size_t byteSize = expectedOutputs[0]->getByteSize();

	const float*	outputAsFloat	= static_cast<const float*>(outputAllocs[0]->getHostPtr());

	if (byteSize != 4*sizeof(float)) {
		return false;
	}

	if (*outputAsFloat != constructNormalizedFloat(8, 0x304000) &&
		*outputAsFloat != constructNormalizedFloat(8, 0x300000)) {
		return false;
	}
	outputAsFloat++;

	if (*outputAsFloat != -constructNormalizedFloat(-7, 0x600000) &&
		*outputAsFloat != -constructNormalizedFloat(-7, 0x604000)) {
		return false;
	}
	outputAsFloat++;

	if (*outputAsFloat != constructNormalizedFloat(2, 0x01C000) &&
		*outputAsFloat != constructNormalizedFloat(2, 0x020000)) {
		return false;
	}
	outputAsFloat++;

	if (*outputAsFloat != constructNormalizedFloat(1, 0xFFC000) &&
		*outputAsFloat != constructNormalizedFloat(2, 0x000000)) {
		return false;
	}

	return true;
}

// Checks that every output from a test-case is a float NaN.
bool compareNan (const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog&)
{
	if (outputAllocs.size() != 1)
		return false;

	// Only size is needed because we cannot compare Nans.
	size_t byteSize = expectedOutputs[0]->getByteSize();

	const float* const	output_as_float	= static_cast<const float* const>(outputAllocs[0]->getHostPtr());

	for (size_t idx = 0; idx < byteSize / sizeof(float); ++idx)
	{
		if (!deFloatIsNaN(output_as_float[idx]))
		{
			return false;
		}
	}

	return true;
}

// Checks that a compute shader can generate a constant composite value of various types, without exercising a computation on it.
tcu::TestCaseGroup* createOpQuantizeToF16Group (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opquantize", "Tests the OpQuantizeToF16 instruction"));

	const std::string shader (
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%quant     = OpQuantizeToF16 %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %quant\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	{
		ComputeShaderSpec	spec;
		const deUint32		numElements		= 100;
		vector<float>		infinities;
		vector<float>		results;

		infinities.reserve(numElements);
		results.reserve(numElements);

		for (size_t idx = 0; idx < numElements; ++idx)
		{
			switch(idx % 4)
			{
				case 0:
					infinities.push_back(std::numeric_limits<float>::infinity());
					results.push_back(std::numeric_limits<float>::infinity());
					break;
				case 1:
					infinities.push_back(-std::numeric_limits<float>::infinity());
					results.push_back(-std::numeric_limits<float>::infinity());
					break;
				case 2:
					infinities.push_back(std::ldexp(1.0f, 16));
					results.push_back(std::numeric_limits<float>::infinity());
					break;
				case 3:
					infinities.push_back(std::ldexp(-1.0f, 32));
					results.push_back(-std::numeric_limits<float>::infinity());
					break;
			}
		}

		spec.assembly = shader;
		spec.inputs.push_back(BufferSp(new Float32Buffer(infinities)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(results)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "infinities", "Check that infinities propagated and created", spec));
	}

	{
		ComputeShaderSpec	spec;
		vector<float>		nans;
		const deUint32		numElements		= 100;

		nans.reserve(numElements);

		for (size_t idx = 0; idx < numElements; ++idx)
		{
			if (idx % 2 == 0)
			{
				nans.push_back(std::numeric_limits<float>::quiet_NaN());
			}
			else
			{
				nans.push_back(-std::numeric_limits<float>::quiet_NaN());
			}
		}

		spec.assembly = shader;
		spec.inputs.push_back(BufferSp(new Float32Buffer(nans)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(nans)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);
		spec.verifyIO = &compareNan;

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "propagated_nans", "Check that nans are propagated", spec));
	}

	{
		ComputeShaderSpec	spec;
		vector<float>		small;
		vector<float>		zeros;
		const deUint32		numElements		= 100;

		small.reserve(numElements);
		zeros.reserve(numElements);

		for (size_t idx = 0; idx < numElements; ++idx)
		{
			switch(idx % 6)
			{
				case 0:
					small.push_back(0.f);
					zeros.push_back(0.f);
					break;
				case 1:
					small.push_back(-0.f);
					zeros.push_back(-0.f);
					break;
				case 2:
					small.push_back(std::ldexp(1.0f, -16));
					zeros.push_back(0.f);
					break;
				case 3:
					small.push_back(std::ldexp(-1.0f, -32));
					zeros.push_back(-0.f);
					break;
				case 4:
					small.push_back(std::ldexp(1.0f, -127));
					zeros.push_back(0.f);
					break;
				case 5:
					small.push_back(-std::ldexp(1.0f, -128));
					zeros.push_back(-0.f);
					break;
			}
		}

		spec.assembly = shader;
		spec.inputs.push_back(BufferSp(new Float32Buffer(small)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(zeros)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "flush_to_zero", "Check that values are zeroed correctly", spec));
	}

	{
		ComputeShaderSpec	spec;
		vector<float>		exact;
		const deUint32		numElements		= 200;

		exact.reserve(numElements);

		for (size_t idx = 0; idx < numElements; ++idx)
			exact.push_back(static_cast<float>(static_cast<int>(idx) - 100));

		spec.assembly = shader;
		spec.inputs.push_back(BufferSp(new Float32Buffer(exact)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(exact)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "exact", "Check that values exactly preserved where appropriate", spec));
	}

	{
		ComputeShaderSpec	spec;
		vector<float>		inputs;
		const deUint32		numElements		= 4;

		inputs.push_back(constructNormalizedFloat(8,	0x300300));
		inputs.push_back(-constructNormalizedFloat(-7,	0x600800));
		inputs.push_back(constructNormalizedFloat(2,	0x01E000));
		inputs.push_back(constructNormalizedFloat(1,	0xFFE000));

		spec.assembly = shader;
		spec.verifyIO = &compareOpQuantizeF16ComputeExactCase;
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputs)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(inputs)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "rounded", "Check that are rounded when needed", spec));
	}

	return group.release();
}

tcu::TestCaseGroup* createSpecConstantOpQuantizeToF16Group (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opspecconstantop_opquantize", "Tests the OpQuantizeToF16 opcode for the OpSpecConstantOp instruction"));

	const std::string shader (
		string(getComputeAsmShaderPreamble()) +

		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"OpDecorate %sc_0  SpecId 0\n"
		"OpDecorate %sc_1  SpecId 1\n"
		"OpDecorate %sc_2  SpecId 2\n"
		"OpDecorate %sc_3  SpecId 3\n"
		"OpDecorate %sc_4  SpecId 4\n"
		"OpDecorate %sc_5  SpecId 5\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"
		"%c_u32_6   = OpConstant %u32 6\n"

		"%sc_0      = OpSpecConstant %f32 0.\n"
		"%sc_1      = OpSpecConstant %f32 0.\n"
		"%sc_2      = OpSpecConstant %f32 0.\n"
		"%sc_3      = OpSpecConstant %f32 0.\n"
		"%sc_4      = OpSpecConstant %f32 0.\n"
		"%sc_5      = OpSpecConstant %f32 0.\n"

		"%sc_0_quant = OpSpecConstantOp %f32 QuantizeToF16 %sc_0\n"
		"%sc_1_quant = OpSpecConstantOp %f32 QuantizeToF16 %sc_1\n"
		"%sc_2_quant = OpSpecConstantOp %f32 QuantizeToF16 %sc_2\n"
		"%sc_3_quant = OpSpecConstantOp %f32 QuantizeToF16 %sc_3\n"
		"%sc_4_quant = OpSpecConstantOp %f32 QuantizeToF16 %sc_4\n"
		"%sc_5_quant = OpSpecConstantOp %f32 QuantizeToF16 %sc_5\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"%selector  = OpUMod %u32 %x %c_u32_6\n"
		"            OpSelectionMerge %exit None\n"
		"            OpSwitch %selector %exit 0 %case0 1 %case1 2 %case2 3 %case3 4 %case4 5 %case5\n"

		"%case0     = OpLabel\n"
		"             OpStore %outloc %sc_0_quant\n"
		"             OpBranch %exit\n"

		"%case1     = OpLabel\n"
		"             OpStore %outloc %sc_1_quant\n"
		"             OpBranch %exit\n"

		"%case2     = OpLabel\n"
		"             OpStore %outloc %sc_2_quant\n"
		"             OpBranch %exit\n"

		"%case3     = OpLabel\n"
		"             OpStore %outloc %sc_3_quant\n"
		"             OpBranch %exit\n"

		"%case4     = OpLabel\n"
		"             OpStore %outloc %sc_4_quant\n"
		"             OpBranch %exit\n"

		"%case5     = OpLabel\n"
		"             OpStore %outloc %sc_5_quant\n"
		"             OpBranch %exit\n"

		"%exit      = OpLabel\n"
		"             OpReturn\n"

		"             OpFunctionEnd\n");

	{
		ComputeShaderSpec	spec;
		const deUint8		numCases	= 4;
		vector<float>		inputs		(numCases, 0.f);
		vector<float>		outputs;

		spec.assembly		= shader;
		spec.numWorkGroups	= IVec3(numCases, 1, 1);

		spec.specConstants.push_back(bitwiseCast<deUint32>(std::numeric_limits<float>::infinity()));
		spec.specConstants.push_back(bitwiseCast<deUint32>(-std::numeric_limits<float>::infinity()));
		spec.specConstants.push_back(bitwiseCast<deUint32>(std::ldexp(1.0f, 16)));
		spec.specConstants.push_back(bitwiseCast<deUint32>(std::ldexp(-1.0f, 32)));

		outputs.push_back(std::numeric_limits<float>::infinity());
		outputs.push_back(-std::numeric_limits<float>::infinity());
		outputs.push_back(std::numeric_limits<float>::infinity());
		outputs.push_back(-std::numeric_limits<float>::infinity());

		spec.inputs.push_back(BufferSp(new Float32Buffer(inputs)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputs)));

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "infinities", "Check that infinities propagated and created", spec));
	}

	{
		ComputeShaderSpec	spec;
		const deUint8		numCases	= 2;
		vector<float>		inputs		(numCases, 0.f);
		vector<float>		outputs;

		spec.assembly		= shader;
		spec.numWorkGroups	= IVec3(numCases, 1, 1);
		spec.verifyIO		= &compareNan;

		outputs.push_back(std::numeric_limits<float>::quiet_NaN());
		outputs.push_back(-std::numeric_limits<float>::quiet_NaN());

		for (deUint8 idx = 0; idx < numCases; ++idx)
			spec.specConstants.push_back(bitwiseCast<deUint32>(outputs[idx]));

		spec.inputs.push_back(BufferSp(new Float32Buffer(inputs)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputs)));

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "propagated_nans", "Check that nans are propagated", spec));
	}

	{
		ComputeShaderSpec	spec;
		const deUint8		numCases	= 6;
		vector<float>		inputs		(numCases, 0.f);
		vector<float>		outputs;

		spec.assembly		= shader;
		spec.numWorkGroups	= IVec3(numCases, 1, 1);

		spec.specConstants.push_back(bitwiseCast<deUint32>(0.f));
		spec.specConstants.push_back(bitwiseCast<deUint32>(-0.f));
		spec.specConstants.push_back(bitwiseCast<deUint32>(std::ldexp(1.0f, -16)));
		spec.specConstants.push_back(bitwiseCast<deUint32>(std::ldexp(-1.0f, -32)));
		spec.specConstants.push_back(bitwiseCast<deUint32>(std::ldexp(1.0f, -127)));
		spec.specConstants.push_back(bitwiseCast<deUint32>(-std::ldexp(1.0f, -128)));

		outputs.push_back(0.f);
		outputs.push_back(-0.f);
		outputs.push_back(0.f);
		outputs.push_back(-0.f);
		outputs.push_back(0.f);
		outputs.push_back(-0.f);

		spec.inputs.push_back(BufferSp(new Float32Buffer(inputs)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputs)));

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "flush_to_zero", "Check that values are zeroed correctly", spec));
	}

	{
		ComputeShaderSpec	spec;
		const deUint8		numCases	= 6;
		vector<float>		inputs		(numCases, 0.f);
		vector<float>		outputs;

		spec.assembly		= shader;
		spec.numWorkGroups	= IVec3(numCases, 1, 1);

		for (deUint8 idx = 0; idx < 6; ++idx)
		{
			const float f = static_cast<float>(idx * 10 - 30) / 4.f;
			spec.specConstants.push_back(bitwiseCast<deUint32>(f));
			outputs.push_back(f);
		}

		spec.inputs.push_back(BufferSp(new Float32Buffer(inputs)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputs)));

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "exact", "Check that values exactly preserved where appropriate", spec));
	}

	{
		ComputeShaderSpec	spec;
		const deUint8		numCases	= 4;
		vector<float>		inputs		(numCases, 0.f);
		vector<float>		outputs;

		spec.assembly		= shader;
		spec.numWorkGroups	= IVec3(numCases, 1, 1);
		spec.verifyIO		= &compareOpQuantizeF16ComputeExactCase;

		outputs.push_back(constructNormalizedFloat(8, 0x300300));
		outputs.push_back(-constructNormalizedFloat(-7, 0x600800));
		outputs.push_back(constructNormalizedFloat(2, 0x01E000));
		outputs.push_back(constructNormalizedFloat(1, 0xFFE000));

		for (deUint8 idx = 0; idx < numCases; ++idx)
			spec.specConstants.push_back(bitwiseCast<deUint32>(outputs[idx]));

		spec.inputs.push_back(BufferSp(new Float32Buffer(inputs)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputs)));

		group->addChild(new SpvAsmComputeShaderCase(
			testCtx, "rounded", "Check that are rounded when needed", spec));
	}

	return group.release();
}

// Checks that constant null/composite values can be used in computation.
tcu::TestCaseGroup* createOpConstantUsageGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opconstantnullcomposite", "Spotcheck the OpConstantNull & OpConstantComposite instruction"));
	ComputeShaderSpec				spec;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	spec.assembly =
		"OpCapability Shader\n"
		"%std450 = OpExtInstImport \"GLSL.std.450\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +

		"%fmat      = OpTypeMatrix %fvec3 3\n"
		"%ten       = OpConstant %u32 10\n"
		"%f32arr10  = OpTypeArray %f32 %ten\n"
		"%fst       = OpTypeStruct %f32 %f32\n"

		+ string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		// Create a bunch of null values
		"%unull     = OpConstantNull %u32\n"
		"%fnull     = OpConstantNull %f32\n"
		"%vnull     = OpConstantNull %fvec3\n"
		"%mnull     = OpConstantNull %fmat\n"
		"%anull     = OpConstantNull %f32arr10\n"
		"%snull     = OpConstantComposite %fst %fnull %fnull\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"

		// Get the abs() of (a certain element of) those null values
		"%unull_cov = OpConvertUToF %f32 %unull\n"
		"%unull_abs = OpExtInst %f32 %std450 FAbs %unull_cov\n"
		"%fnull_abs = OpExtInst %f32 %std450 FAbs %fnull\n"
		"%vnull_0   = OpCompositeExtract %f32 %vnull 0\n"
		"%vnull_abs = OpExtInst %f32 %std450 FAbs %vnull_0\n"
		"%mnull_12  = OpCompositeExtract %f32 %mnull 1 2\n"
		"%mnull_abs = OpExtInst %f32 %std450 FAbs %mnull_12\n"
		"%anull_3   = OpCompositeExtract %f32 %anull 3\n"
		"%anull_abs = OpExtInst %f32 %std450 FAbs %anull_3\n"
		"%snull_1   = OpCompositeExtract %f32 %snull 1\n"
		"%snull_abs = OpExtInst %f32 %std450 FAbs %snull_1\n"

		// Add them all
		"%add1      = OpFAdd %f32 %neg  %unull_abs\n"
		"%add2      = OpFAdd %f32 %add1 %fnull_abs\n"
		"%add3      = OpFAdd %f32 %add2 %vnull_abs\n"
		"%add4      = OpFAdd %f32 %add3 %mnull_abs\n"
		"%add5      = OpFAdd %f32 %add4 %anull_abs\n"
		"%final     = OpFAdd %f32 %add5 %snull_abs\n"

		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %final\n" // write to output
		"             OpReturn\n"
		"             OpFunctionEnd\n";
	spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
	spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
	spec.numWorkGroups = IVec3(numElements, 1, 1);

	group->addChild(new SpvAsmComputeShaderCase(testCtx, "spotcheck", "Check that values constructed via OpConstantNull & OpConstantComposite can be used", spec));

	return group.release();
}

// Assembly code used for testing loop control is based on GLSL source code:
// #version 430
//
// layout(std140, set = 0, binding = 0) readonly buffer Input {
//   float elements[];
// } input_data;
// layout(std140, set = 0, binding = 1) writeonly buffer Output {
//   float elements[];
// } output_data;
//
// void main() {
//   uint x = gl_GlobalInvocationID.x;
//   output_data.elements[x] = input_data.elements[x];
//   for (uint i = 0; i < 4; ++i)
//     output_data.elements[x] += 1.f;
// }
tcu::TestCaseGroup* createLoopControlGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "loop_control", "Tests loop control cases"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%u32ptr      = OpTypePointer Function %u32\n"

		"%id          = OpVariable %uvec3ptr Input\n"
		"%zero        = OpConstant %i32 0\n"
		"%uzero       = OpConstant %u32 0\n"
		"%one         = OpConstant %i32 1\n"
		"%constf1     = OpConstant %f32 1.0\n"
		"%four        = OpConstant %u32 4\n"

		"%main        = OpFunction %void None %voidf\n"
		"%entry       = OpLabel\n"
		"%i           = OpVariable %u32ptr Function\n"
		"               OpStore %i %uzero\n"

		"%idval       = OpLoad %uvec3 %id\n"
		"%x           = OpCompositeExtract %u32 %idval 0\n"
		"%inloc       = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval       = OpLoad %f32 %inloc\n"
		"%outloc      = OpAccessChain %f32ptr %outdata %zero %x\n"
		"               OpStore %outloc %inval\n"
		"               OpBranch %loop_entry\n"

		"%loop_entry  = OpLabel\n"
		"%i_val       = OpLoad %u32 %i\n"
		"%cmp_lt      = OpULessThan %bool %i_val %four\n"
		"               OpLoopMerge %loop_merge %loop_body ${CONTROL}\n"
		"               OpBranchConditional %cmp_lt %loop_body %loop_merge\n"
		"%loop_body   = OpLabel\n"
		"%outval      = OpLoad %f32 %outloc\n"
		"%addf1       = OpFAdd %f32 %outval %constf1\n"
		"               OpStore %outloc %addf1\n"
		"%new_i       = OpIAdd %u32 %i_val %one\n"
		"               OpStore %i %new_i\n"
		"               OpBranch %loop_entry\n"
		"%loop_merge  = OpLabel\n"
		"               OpReturn\n"
		"               OpFunctionEnd\n");

	cases.push_back(CaseParameter("none",				"None"));
	cases.push_back(CaseParameter("unroll",				"Unroll"));
	cases.push_back(CaseParameter("dont_unroll",		"DontUnroll"));
	cases.push_back(CaseParameter("unroll_dont_unroll",	"Unroll|DontUnroll"));

	fillRandomScalars(rnd, -100.f, 100.f, &inputFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats[ndx] = inputFloats[ndx] + 4.f;

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["CONTROL"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

// Assembly code used for testing selection control is based on GLSL source code:
// #version 430
//
// layout(std140, set = 0, binding = 0) readonly buffer Input {
//   float elements[];
// } input_data;
// layout(std140, set = 0, binding = 1) writeonly buffer Output {
//   float elements[];
// } output_data;
//
// void main() {
//   uint x = gl_GlobalInvocationID.x;
//   float val = input_data.elements[x];
//   if (val > 10.f)
//     output_data.elements[x] = val + 1.f;
//   else
//     output_data.elements[x] = val - 1.f;
// }
tcu::TestCaseGroup* createSelectionControlGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "selection_control", "Tests selection control cases"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%id       = OpVariable %uvec3ptr Input\n"
		"%zero     = OpConstant %i32 0\n"
		"%constf1  = OpConstant %f32 1.0\n"
		"%constf10 = OpConstant %f32 10.0\n"

		"%main     = OpFunction %void None %voidf\n"
		"%entry    = OpLabel\n"
		"%idval    = OpLoad %uvec3 %id\n"
		"%x        = OpCompositeExtract %u32 %idval 0\n"
		"%inloc    = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval    = OpLoad %f32 %inloc\n"
		"%outloc   = OpAccessChain %f32ptr %outdata %zero %x\n"
		"%cmp_gt   = OpFOrdGreaterThan %bool %inval %constf10\n"

		"            OpSelectionMerge %if_end ${CONTROL}\n"
		"            OpBranchConditional %cmp_gt %if_true %if_false\n"
		"%if_true  = OpLabel\n"
		"%addf1    = OpFAdd %f32 %inval %constf1\n"
		"            OpStore %outloc %addf1\n"
		"            OpBranch %if_end\n"
		"%if_false = OpLabel\n"
		"%subf1    = OpFSub %f32 %inval %constf1\n"
		"            OpStore %outloc %subf1\n"
		"            OpBranch %if_end\n"
		"%if_end   = OpLabel\n"
		"            OpReturn\n"
		"            OpFunctionEnd\n");

	cases.push_back(CaseParameter("none",					"None"));
	cases.push_back(CaseParameter("flatten",				"Flatten"));
	cases.push_back(CaseParameter("dont_flatten",			"DontFlatten"));
	cases.push_back(CaseParameter("flatten_dont_flatten",	"DontFlatten|Flatten"));

	fillRandomScalars(rnd, -100.f, 100.f, &inputFloats[0], numElements);

	// CPU might not use the same rounding mode as the GPU. Use whole numbers to avoid rounding differences.
	floorAll(inputFloats);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats[ndx] = inputFloats[ndx] + (inputFloats[ndx] > 10.f ? 1.f : -1.f);

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["CONTROL"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

// Assembly code used for testing function control is based on GLSL source code:
//
// #version 430
//
// layout(std140, set = 0, binding = 0) readonly buffer Input {
//   float elements[];
// } input_data;
// layout(std140, set = 0, binding = 1) writeonly buffer Output {
//   float elements[];
// } output_data;
//
// float const10() { return 10.f; }
//
// void main() {
//   uint x = gl_GlobalInvocationID.x;
//   output_data.elements[x] = input_data.elements[x] + const10();
// }
tcu::TestCaseGroup* createFunctionControlGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "function_control", "Tests function control cases"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main \"main\"\n"
		"OpName %func_const10 \"const10(\"\n"
		"OpName %id \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%f32f = OpTypeFunction %f32\n"
		"%id = OpVariable %uvec3ptr Input\n"
		"%zero = OpConstant %i32 0\n"
		"%constf10 = OpConstant %f32 10.0\n"

		"%main         = OpFunction %void None %voidf\n"
		"%entry        = OpLabel\n"
		"%idval        = OpLoad %uvec3 %id\n"
		"%x            = OpCompositeExtract %u32 %idval 0\n"
		"%inloc        = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval        = OpLoad %f32 %inloc\n"
		"%ret_10       = OpFunctionCall %f32 %func_const10\n"
		"%fadd         = OpFAdd %f32 %inval %ret_10\n"
		"%outloc       = OpAccessChain %f32ptr %outdata %zero %x\n"
		"                OpStore %outloc %fadd\n"
		"                OpReturn\n"
		"                OpFunctionEnd\n"

		"%func_const10 = OpFunction %f32 ${CONTROL} %f32f\n"
		"%label        = OpLabel\n"
		"                OpReturnValue %constf10\n"
		"                OpFunctionEnd\n");

	cases.push_back(CaseParameter("none",						"None"));
	cases.push_back(CaseParameter("inline",						"Inline"));
	cases.push_back(CaseParameter("dont_inline",				"DontInline"));
	cases.push_back(CaseParameter("pure",						"Pure"));
	cases.push_back(CaseParameter("const",						"Const"));
	cases.push_back(CaseParameter("inline_pure",				"Inline|Pure"));
	cases.push_back(CaseParameter("const_dont_inline",			"Const|DontInline"));
	cases.push_back(CaseParameter("inline_dont_inline",			"Inline|DontInline"));
	cases.push_back(CaseParameter("pure_inline_dont_inline",	"Pure|Inline|DontInline"));

	fillRandomScalars(rnd, -100.f, 100.f, &inputFloats[0], numElements);

	// CPU might not use the same rounding mode as the GPU. Use whole numbers to avoid rounding differences.
	floorAll(inputFloats);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats[ndx] = inputFloats[ndx] + 10.f;

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["CONTROL"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

tcu::TestCaseGroup* createMemoryAccessGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "memory_access", "Tests memory access cases"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					inputFloats		(numElements, 0);
	vector<float>					outputFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) + string(getComputeAsmInputOutputBuffer()) +

		"%f32ptr_f  = OpTypePointer Function %f32\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"
		"%four      = OpConstant %i32 4\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%copy      = OpVariable %f32ptr_f Function\n"
		"%idval     = OpLoad %uvec3 %id ${ACCESS}\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %f32ptr %indata  %zero %x\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpCopyMemory %copy %inloc ${ACCESS}\n"
		"%val1      = OpLoad %f32 %copy\n"
		"%val2      = OpLoad %f32 %inloc\n"
		"%add       = OpFAdd %f32 %val1 %val2\n"
		"             OpStore %outloc %add ${ACCESS}\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	cases.push_back(CaseParameter("null",					""));
	cases.push_back(CaseParameter("none",					"None"));
	cases.push_back(CaseParameter("volatile",				"Volatile"));
	cases.push_back(CaseParameter("aligned",				"Aligned 4"));
	cases.push_back(CaseParameter("nontemporal",			"Nontemporal"));
	cases.push_back(CaseParameter("aligned_nontemporal",	"Aligned|Nontemporal 4"));
	cases.push_back(CaseParameter("aligned_volatile",		"Volatile|Aligned 4"));

	fillRandomScalars(rnd, -100.f, 100.f, &inputFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		outputFloats[ndx] = inputFloats[ndx] + inputFloats[ndx];

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["ACCESS"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(inputFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(outputFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

	return group.release();
}

// Checks that we can get undefined values for various types, without exercising a computation with it.
tcu::TestCaseGroup* createOpUndefGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opundef", "Tests the OpUndef instruction"));
	vector<CaseParameter>			cases;
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 100;
	vector<float>					positiveFloats	(numElements, 0);
	vector<float>					negativeFloats	(numElements, 0);
	const StringTemplate			shaderTemplate	(
		string(getComputeAsmShaderPreamble()) +

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		+ string(getComputeAsmInputOutputBufferTraits()) + string(getComputeAsmCommonTypes()) +
		"%uvec2     = OpTypeVector %u32 2\n"
		"%fvec4     = OpTypeVector %f32 4\n"
		"%fmat33    = OpTypeMatrix %fvec3 3\n"
		"%image     = OpTypeImage %f32 2D 0 0 0 1 Unknown\n"
		"%sampler   = OpTypeSampler\n"
		"%simage    = OpTypeSampledImage %image\n"
		"%const100  = OpConstant %u32 100\n"
		"%uarr100   = OpTypeArray %i32 %const100\n"
		"%struct    = OpTypeStruct %f32 %i32 %u32\n"
		"%pointer   = OpTypePointer Function %i32\n"
		+ string(getComputeAsmInputOutputBuffer()) +

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"

		"%undef     = OpUndef ${TYPE}\n"

		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"

		"%inloc     = OpAccessChain %f32ptr %indata %zero %x\n"
		"%inval     = OpLoad %f32 %inloc\n"
		"%neg       = OpFNegate %f32 %inval\n"
		"%outloc    = OpAccessChain %f32ptr %outdata %zero %x\n"
		"             OpStore %outloc %neg\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	cases.push_back(CaseParameter("bool",			"%bool"));
	cases.push_back(CaseParameter("sint32",			"%i32"));
	cases.push_back(CaseParameter("uint32",			"%u32"));
	cases.push_back(CaseParameter("float32",		"%f32"));
	cases.push_back(CaseParameter("vec4float32",	"%fvec4"));
	cases.push_back(CaseParameter("vec2uint32",		"%uvec2"));
	cases.push_back(CaseParameter("matrix",			"%fmat33"));
	cases.push_back(CaseParameter("image",			"%image"));
	cases.push_back(CaseParameter("sampler",		"%sampler"));
	cases.push_back(CaseParameter("sampledimage",	"%simage"));
	cases.push_back(CaseParameter("array",			"%uarr100"));
	cases.push_back(CaseParameter("runtimearray",	"%f32arr"));
	cases.push_back(CaseParameter("struct",			"%struct"));
	cases.push_back(CaseParameter("pointer",		"%pointer"));

	fillRandomScalars(rnd, 1.f, 100.f, &positiveFloats[0], numElements);

	for (size_t ndx = 0; ndx < numElements; ++ndx)
		negativeFloats[ndx] = -positiveFloats[ndx];

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>		specializations;
		ComputeShaderSpec		spec;

		specializations["TYPE"] = cases[caseNdx].param;
		spec.assembly = shaderTemplate.specialize(specializations);
		spec.inputs.push_back(BufferSp(new Float32Buffer(positiveFloats)));
		spec.outputs.push_back(BufferSp(new Float32Buffer(negativeFloats)));
		spec.numWorkGroups = IVec3(numElements, 1, 1);

		group->addChild(new SpvAsmComputeShaderCase(testCtx, cases[caseNdx].name, cases[caseNdx].name, spec));
	}

		return group.release();
}

} // anonymous

tcu::TestCaseGroup* createOpSourceTests (tcu::TestContext& testCtx)
{
	struct NameCodePair { string name, code; };
	RGBA							defaultColors[4];
	de::MovePtr<tcu::TestCaseGroup> opSourceTests			(new tcu::TestCaseGroup(testCtx, "opsource", "OpSource instruction"));
	const std::string				opsourceGLSLWithFile	= "%opsrcfile = OpString \"foo.vert\"\nOpSource GLSL 450 %opsrcfile ";
	map<string, string>				fragments				= passthruFragments();
	const NameCodePair				tests[]					=
	{
		{"unknown", "OpSource Unknown 321"},
		{"essl", "OpSource ESSL 310"},
		{"glsl", "OpSource GLSL 450"},
		{"opencl_cpp", "OpSource OpenCL_CPP 120"},
		{"opencl_c", "OpSource OpenCL_C 120"},
		{"multiple", "OpSource GLSL 450\nOpSource GLSL 450"},
		{"file", opsourceGLSLWithFile},
		{"source", opsourceGLSLWithFile + "\"void main(){}\""},
		// Longest possible source string: SPIR-V limits instructions to 65535
		// words, of which the first 4 are opsourceGLSLWithFile; the rest will
		// contain 65530 UTF8 characters (one word each) plus one last word
		// containing 3 ASCII characters and \0.
		{"longsource", opsourceGLSLWithFile + '"' + makeLongUTF8String(65530) + "ccc" + '"'}
	};

	getDefaultColors(defaultColors);
	for (size_t testNdx = 0; testNdx < sizeof(tests) / sizeof(NameCodePair); ++testNdx)
	{
		fragments["debug"] = tests[testNdx].code;
		createTestsForAllStages(tests[testNdx].name, defaultColors, defaultColors, fragments, opSourceTests.get());
	}

	return opSourceTests.release();
}

tcu::TestCaseGroup* createOpSourceContinuedTests (tcu::TestContext& testCtx)
{
	struct NameCodePair { string name, code; };
	RGBA								defaultColors[4];
	de::MovePtr<tcu::TestCaseGroup>		opSourceTests		(new tcu::TestCaseGroup(testCtx, "opsourcecontinued", "OpSourceContinued instruction"));
	map<string, string>					fragments			= passthruFragments();
	const std::string					opsource			= "%opsrcfile = OpString \"foo.vert\"\nOpSource GLSL 450 %opsrcfile \"void main(){}\"\n";
	const NameCodePair					tests[]				=
	{
		{"empty", opsource + "OpSourceContinued \"\""},
		{"short", opsource + "OpSourceContinued \"abcde\""},
		{"multiple", opsource + "OpSourceContinued \"abcde\"\nOpSourceContinued \"fghij\""},
		// Longest possible source string: SPIR-V limits instructions to 65535
		// words, of which the first one is OpSourceContinued/length; the rest
		// will contain 65533 UTF8 characters (one word each) plus one last word
		// containing 3 ASCII characters and \0.
		{"long", opsource + "OpSourceContinued \"" + makeLongUTF8String(65533) + "ccc\""}
	};

	getDefaultColors(defaultColors);
	for (size_t testNdx = 0; testNdx < sizeof(tests) / sizeof(NameCodePair); ++testNdx)
	{
		fragments["debug"] = tests[testNdx].code;
		createTestsForAllStages(tests[testNdx].name, defaultColors, defaultColors, fragments, opSourceTests.get());
	}

	return opSourceTests.release();
}

tcu::TestCaseGroup* createOpNoLineTests(tcu::TestContext& testCtx)
{
	RGBA								 defaultColors[4];
	de::MovePtr<tcu::TestCaseGroup>		 opLineTests		 (new tcu::TestCaseGroup(testCtx, "opnoline", "OpNoLine instruction"));
	map<string, string>					 fragments;
	getDefaultColors(defaultColors);
	fragments["debug"]			=
		"%name = OpString \"name\"\n";

	fragments["pre_main"]	=
		"OpNoLine\n"
		"OpNoLine\n"
		"OpLine %name 1 1\n"
		"OpNoLine\n"
		"OpLine %name 1 1\n"
		"OpLine %name 1 1\n"
		"%second_function = OpFunction %v4f32 None %v4f32_function\n"
		"OpNoLine\n"
		"OpLine %name 1 1\n"
		"OpNoLine\n"
		"OpLine %name 1 1\n"
		"OpLine %name 1 1\n"
		"%second_param1 = OpFunctionParameter %v4f32\n"
		"OpNoLine\n"
		"OpNoLine\n"
		"%label_secondfunction = OpLabel\n"
		"OpNoLine\n"
		"OpReturnValue %second_param1\n"
		"OpFunctionEnd\n"
		"OpNoLine\n"
		"OpNoLine\n";

	fragments["testfun"]		=
		// A %test_code function that returns its argument unchanged.
		"OpNoLine\n"
		"OpNoLine\n"
		"OpLine %name 1 1\n"
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"OpNoLine\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"OpNoLine\n"
		"OpNoLine\n"
		"%label_testfun = OpLabel\n"
		"OpNoLine\n"
		"%val1 = OpFunctionCall %v4f32 %second_function %param1\n"
		"OpReturnValue %val1\n"
		"OpFunctionEnd\n"
		"OpLine %name 1 1\n"
		"OpNoLine\n";

	createTestsForAllStages("opnoline", defaultColors, defaultColors, fragments, opLineTests.get());

	return opLineTests.release();
}


tcu::TestCaseGroup* createOpLineTests(tcu::TestContext& testCtx)
{
	RGBA													defaultColors[4];
	de::MovePtr<tcu::TestCaseGroup>							opLineTests			(new tcu::TestCaseGroup(testCtx, "opline", "OpLine instruction"));
	map<string, string>										fragments;
	std::vector<std::pair<std::string, std::string> >		problemStrings;

	problemStrings.push_back(std::make_pair<std::string, std::string>("empty_name", ""));
	problemStrings.push_back(std::make_pair<std::string, std::string>("short_name", "short_name"));
	problemStrings.push_back(std::make_pair<std::string, std::string>("long_name", makeLongUTF8String(65530) + "ccc"));
	getDefaultColors(defaultColors);

	fragments["debug"]			=
		"%other_name = OpString \"other_name\"\n";

	fragments["pre_main"]	=
		"OpLine %file_name 32 0\n"
		"OpLine %file_name 32 32\n"
		"OpLine %file_name 32 40\n"
		"OpLine %other_name 32 40\n"
		"OpLine %other_name 0 100\n"
		"OpLine %other_name 0 4294967295\n"
		"OpLine %other_name 4294967295 0\n"
		"OpLine %other_name 32 40\n"
		"OpLine %file_name 0 0\n"
		"%second_function = OpFunction %v4f32 None %v4f32_function\n"
		"OpLine %file_name 1 0\n"
		"%second_param1 = OpFunctionParameter %v4f32\n"
		"OpLine %file_name 1 3\n"
		"OpLine %file_name 1 2\n"
		"%label_secondfunction = OpLabel\n"
		"OpLine %file_name 0 2\n"
		"OpReturnValue %second_param1\n"
		"OpFunctionEnd\n"
		"OpLine %file_name 0 2\n"
		"OpLine %file_name 0 2\n";

	fragments["testfun"]		=
		// A %test_code function that returns its argument unchanged.
		"OpLine %file_name 1 0\n"
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"OpLine %file_name 16 330\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"OpLine %file_name 14 442\n"
		"%label_testfun = OpLabel\n"
		"OpLine %file_name 11 1024\n"
		"%val1 = OpFunctionCall %v4f32 %second_function %param1\n"
		"OpLine %file_name 2 97\n"
		"OpReturnValue %val1\n"
		"OpFunctionEnd\n"
		"OpLine %file_name 5 32\n";

	for (size_t i = 0; i < problemStrings.size(); ++i)
	{
		map<string, string> testFragments = fragments;
		testFragments["debug"] += "%file_name = OpString \"" + problemStrings[i].second + "\"\n";
		createTestsForAllStages(string("opline") + "_" + problemStrings[i].first, defaultColors, defaultColors, testFragments, opLineTests.get());
	}

	return opLineTests.release();
}

tcu::TestCaseGroup* createOpConstantNullTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> opConstantNullTests		(new tcu::TestCaseGroup(testCtx, "opconstantnull", "OpConstantNull instruction"));
	RGBA							colors[4];


	const char						functionStart[] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%lbl    = OpLabel\n";

	const char						functionEnd[]	=
		"OpReturnValue %transformed_param\n"
		"OpFunctionEnd\n";

	struct NameConstantsCode
	{
		string name;
		string constants;
		string code;
	};

	NameConstantsCode tests[] =
	{
		{
			"vec4",
			"%cnull = OpConstantNull %v4f32\n",
			"%transformed_param = OpFAdd %v4f32 %param1 %cnull\n"
		},
		{
			"float",
			"%cnull = OpConstantNull %f32\n",
			"%vp = OpVariable %fp_v4f32 Function\n"
			"%v  = OpLoad %v4f32 %vp\n"
			"%v0 = OpVectorInsertDynamic %v4f32 %v %cnull %c_i32_0\n"
			"%v1 = OpVectorInsertDynamic %v4f32 %v0 %cnull %c_i32_1\n"
			"%v2 = OpVectorInsertDynamic %v4f32 %v1 %cnull %c_i32_2\n"
			"%v3 = OpVectorInsertDynamic %v4f32 %v2 %cnull %c_i32_3\n"
			"%transformed_param = OpFAdd %v4f32 %param1 %v3\n"
		},
		{
			"bool",
			"%cnull             = OpConstantNull %bool\n",
			"%v                 = OpVariable %fp_v4f32 Function\n"
			"                     OpStore %v %param1\n"
			"                     OpSelectionMerge %false_label None\n"
			"                     OpBranchConditional %cnull %true_label %false_label\n"
			"%true_label        = OpLabel\n"
			"                     OpStore %v %c_v4f32_0_5_0_5_0_5_0_5\n"
			"                     OpBranch %false_label\n"
			"%false_label       = OpLabel\n"
			"%transformed_param = OpLoad %v4f32 %v\n"
		},
		{
			"i32",
			"%cnull             = OpConstantNull %i32\n",
			"%v                 = OpVariable %fp_v4f32 Function %c_v4f32_0_5_0_5_0_5_0_5\n"
			"%b                 = OpIEqual %bool %cnull %c_i32_0\n"
			"                     OpSelectionMerge %false_label None\n"
			"                     OpBranchConditional %b %true_label %false_label\n"
			"%true_label        = OpLabel\n"
			"                     OpStore %v %param1\n"
			"                     OpBranch %false_label\n"
			"%false_label       = OpLabel\n"
			"%transformed_param = OpLoad %v4f32 %v\n"
		},
		{
			"struct",
			"%stype             = OpTypeStruct %f32 %v4f32\n"
			"%fp_stype          = OpTypePointer Function %stype\n"
			"%cnull             = OpConstantNull %stype\n",
			"%v                 = OpVariable %fp_stype Function %cnull\n"
			"%f                 = OpAccessChain %fp_v4f32 %v %c_i32_1\n"
			"%f_val             = OpLoad %v4f32 %f\n"
			"%transformed_param = OpFAdd %v4f32 %param1 %f_val\n"
		},
		{
			"array",
			"%a4_v4f32          = OpTypeArray %v4f32 %c_u32_4\n"
			"%fp_a4_v4f32       = OpTypePointer Function %a4_v4f32\n"
			"%cnull             = OpConstantNull %a4_v4f32\n",
			"%v                 = OpVariable %fp_a4_v4f32 Function %cnull\n"
			"%f                 = OpAccessChain %fp_v4f32 %v %c_u32_0\n"
			"%f1                = OpAccessChain %fp_v4f32 %v %c_u32_1\n"
			"%f2                = OpAccessChain %fp_v4f32 %v %c_u32_2\n"
			"%f3                = OpAccessChain %fp_v4f32 %v %c_u32_3\n"
			"%f_val             = OpLoad %v4f32 %f\n"
			"%f1_val            = OpLoad %v4f32 %f1\n"
			"%f2_val            = OpLoad %v4f32 %f2\n"
			"%f3_val            = OpLoad %v4f32 %f3\n"
			"%t0                = OpFAdd %v4f32 %param1 %f_val\n"
			"%t1                = OpFAdd %v4f32 %t0 %f1_val\n"
			"%t2                = OpFAdd %v4f32 %t1 %f2_val\n"
			"%transformed_param = OpFAdd %v4f32 %t2 %f3_val\n"
		},
		{
			"matrix",
			"%mat4x4_f32        = OpTypeMatrix %v4f32 4\n"
			"%cnull             = OpConstantNull %mat4x4_f32\n",
			// Our null matrix * any vector should result in a zero vector.
			"%v                 = OpVectorTimesMatrix %v4f32 %param1 %cnull\n"
			"%transformed_param = OpFAdd %v4f32 %param1 %v\n"
		}
	};

	getHalfColorsFullAlpha(colors);

	for (size_t testNdx = 0; testNdx < sizeof(tests) / sizeof(NameConstantsCode); ++testNdx)
	{
		map<string, string> fragments;
		fragments["pre_main"] = tests[testNdx].constants;
		fragments["testfun"] = string(functionStart) + tests[testNdx].code + functionEnd;
		createTestsForAllStages(tests[testNdx].name, colors, colors, fragments, opConstantNullTests.get());
	}
	return opConstantNullTests.release();
}
tcu::TestCaseGroup* createOpConstantCompositeTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> opConstantCompositeTests		(new tcu::TestCaseGroup(testCtx, "opconstantcomposite", "OpConstantComposite instruction"));
	RGBA							inputColors[4];
	RGBA							outputColors[4];


	const char						functionStart[]	 =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%lbl    = OpLabel\n";

	const char						functionEnd[]		=
		"OpReturnValue %transformed_param\n"
		"OpFunctionEnd\n";

	struct NameConstantsCode
	{
		string name;
		string constants;
		string code;
	};

	NameConstantsCode tests[] =
	{
		{
			"vec4",

			"%cval              = OpConstantComposite %v4f32 %c_f32_0_5 %c_f32_0_5 %c_f32_0_5 %c_f32_0\n",
			"%transformed_param = OpFAdd %v4f32 %param1 %cval\n"
		},
		{
			"struct",

			"%stype             = OpTypeStruct %v4f32 %f32\n"
			"%fp_stype          = OpTypePointer Function %stype\n"
			"%f32_n_1           = OpConstant %f32 -1.0\n"
			"%f32_1_5           = OpConstant %f32 !0x3fc00000\n" // +1.5
			"%cvec              = OpConstantComposite %v4f32 %f32_1_5 %f32_1_5 %f32_1_5 %c_f32_1\n"
			"%cval              = OpConstantComposite %stype %cvec %f32_n_1\n",

			"%v                 = OpVariable %fp_stype Function %cval\n"
			"%vec_ptr           = OpAccessChain %fp_v4f32 %v %c_u32_0\n"
			"%f32_ptr           = OpAccessChain %fp_f32 %v %c_u32_1\n"
			"%vec_val           = OpLoad %v4f32 %vec_ptr\n"
			"%f32_val           = OpLoad %f32 %f32_ptr\n"
			"%tmp1              = OpVectorTimesScalar %v4f32 %c_v4f32_1_1_1_1 %f32_val\n" // vec4(-1)
			"%tmp2              = OpFAdd %v4f32 %tmp1 %param1\n" // param1 + vec4(-1)
			"%transformed_param = OpFAdd %v4f32 %tmp2 %vec_val\n" // param1 + vec4(-1) + vec4(1.5, 1.5, 1.5, 1.0)
		},
		{
			// [1|0|0|0.5] [x] = x + 0.5
			// [0|1|0|0.5] [y] = y + 0.5
			// [0|0|1|0.5] [z] = z + 0.5
			// [0|0|0|1  ] [1] = 1
			"matrix",

			"%mat4x4_f32          = OpTypeMatrix %v4f32 4\n"
		    "%v4f32_1_0_0_0       = OpConstantComposite %v4f32 %c_f32_1 %c_f32_0 %c_f32_0 %c_f32_0\n"
		    "%v4f32_0_1_0_0       = OpConstantComposite %v4f32 %c_f32_0 %c_f32_1 %c_f32_0 %c_f32_0\n"
		    "%v4f32_0_0_1_0       = OpConstantComposite %v4f32 %c_f32_0 %c_f32_0 %c_f32_1 %c_f32_0\n"
		    "%v4f32_0_5_0_5_0_5_1 = OpConstantComposite %v4f32 %c_f32_0_5 %c_f32_0_5 %c_f32_0_5 %c_f32_1\n"
			"%cval                = OpConstantComposite %mat4x4_f32 %v4f32_1_0_0_0 %v4f32_0_1_0_0 %v4f32_0_0_1_0 %v4f32_0_5_0_5_0_5_1\n",

			"%transformed_param   = OpMatrixTimesVector %v4f32 %cval %param1\n"
		},
		{
			"array",

			"%c_v4f32_1_1_1_0     = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_0\n"
			"%fp_a4f32            = OpTypePointer Function %a4f32\n"
			"%f32_n_1             = OpConstant %f32 -1.0\n"
			"%f32_1_5             = OpConstant %f32 !0x3fc00000\n" // +1.5
			"%carr                = OpConstantComposite %a4f32 %c_f32_0 %f32_n_1 %f32_1_5 %c_f32_0\n",

			"%v                   = OpVariable %fp_a4f32 Function %carr\n"
			"%f                   = OpAccessChain %fp_f32 %v %c_u32_0\n"
			"%f1                  = OpAccessChain %fp_f32 %v %c_u32_1\n"
			"%f2                  = OpAccessChain %fp_f32 %v %c_u32_2\n"
			"%f3                  = OpAccessChain %fp_f32 %v %c_u32_3\n"
			"%f_val               = OpLoad %f32 %f\n"
			"%f1_val              = OpLoad %f32 %f1\n"
			"%f2_val              = OpLoad %f32 %f2\n"
			"%f3_val              = OpLoad %f32 %f3\n"
			"%ftot1               = OpFAdd %f32 %f_val %f1_val\n"
			"%ftot2               = OpFAdd %f32 %ftot1 %f2_val\n"
			"%ftot3               = OpFAdd %f32 %ftot2 %f3_val\n"  // 0 - 1 + 1.5 + 0
			"%add_vec             = OpVectorTimesScalar %v4f32 %c_v4f32_1_1_1_0 %ftot3\n"
			"%transformed_param   = OpFAdd %v4f32 %param1 %add_vec\n"
		},
		{
			//
			// [
			//   {
			//      0.0,
			//      [ 1.0, 1.0, 1.0, 1.0]
			//   },
			//   {
			//      1.0,
			//      [ 0.0, 0.5, 0.0, 0.0]
			//   }, //     ^^^
			//   {
			//      0.0,
			//      [ 1.0, 1.0, 1.0, 1.0]
			//   }
			// ]
			"array_of_struct_of_array",

			"%c_v4f32_1_1_1_0     = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_0\n"
			"%fp_a4f32            = OpTypePointer Function %a4f32\n"
			"%stype               = OpTypeStruct %f32 %a4f32\n"
			"%a3stype             = OpTypeArray %stype %c_u32_3\n"
			"%fp_a3stype          = OpTypePointer Function %a3stype\n"
			"%ca4f32_0            = OpConstantComposite %a4f32 %c_f32_0 %c_f32_0_5 %c_f32_0 %c_f32_0\n"
			"%ca4f32_1            = OpConstantComposite %a4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_1\n"
			"%cstype1             = OpConstantComposite %stype %c_f32_0 %ca4f32_1\n"
			"%cstype2             = OpConstantComposite %stype %c_f32_1 %ca4f32_0\n"
			"%carr                = OpConstantComposite %a3stype %cstype1 %cstype2 %cstype1",

			"%v                   = OpVariable %fp_a3stype Function %carr\n"
			"%f                   = OpAccessChain %fp_f32 %v %c_u32_1 %c_u32_1 %c_u32_1\n"
			"%f_l                 = OpLoad %f32 %f\n"
			"%add_vec             = OpVectorTimesScalar %v4f32 %c_v4f32_1_1_1_0 %f_l\n"
			"%transformed_param   = OpFAdd %v4f32 %param1 %add_vec\n"
		}
	};

	getHalfColorsFullAlpha(inputColors);
	outputColors[0] = RGBA(255, 255, 255, 255);
	outputColors[1] = RGBA(255, 127, 127, 255);
	outputColors[2] = RGBA(127, 255, 127, 255);
	outputColors[3] = RGBA(127, 127, 255, 255);

	for (size_t testNdx = 0; testNdx < sizeof(tests) / sizeof(NameConstantsCode); ++testNdx)
	{
		map<string, string> fragments;
		fragments["pre_main"] = tests[testNdx].constants;
		fragments["testfun"] = string(functionStart) + tests[testNdx].code + functionEnd;
		createTestsForAllStages(tests[testNdx].name, inputColors, outputColors, fragments, opConstantCompositeTests.get());
	}
	return opConstantCompositeTests.release();
}

tcu::TestCaseGroup* createSelectionBlockOrderTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group				(new tcu::TestCaseGroup(testCtx, "selection_block_order", "Out-of-order blocks for selection"));
	RGBA							inputColors[4];
	RGBA							outputColors[4];
	map<string, string>				fragments;

	// vec4 test_code(vec4 param) {
	//   vec4 result = param;
	//   for (int i = 0; i < 4; ++i) {
	//     if (i == 0) result[i] = 0.;
	//     else        result[i] = 1. - result[i];
	//   }
	//   return result;
	// }
	const char						function[]			=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1    = OpFunctionParameter %v4f32\n"
		"%lbl       = OpLabel\n"
		"%iptr      = OpVariable %fp_i32 Function\n"
		"%result    = OpVariable %fp_v4f32 Function\n"
		"             OpStore %iptr %c_i32_0\n"
		"             OpStore %result %param1\n"
		"             OpBranch %loop\n"

		// Loop entry block.
		"%loop      = OpLabel\n"
		"%ival      = OpLoad %i32 %iptr\n"
		"%lt_4      = OpSLessThan %bool %ival %c_i32_4\n"
		"             OpLoopMerge %exit %if_entry None\n"
		"             OpBranchConditional %lt_4 %if_entry %exit\n"

		// Merge block for loop.
		"%exit      = OpLabel\n"
		"%ret       = OpLoad %v4f32 %result\n"
		"             OpReturnValue %ret\n"

		// If-statement entry block.
		"%if_entry  = OpLabel\n"
		"%loc       = OpAccessChain %fp_f32 %result %ival\n"
		"%eq_0      = OpIEqual %bool %ival %c_i32_0\n"
		"             OpSelectionMerge %if_exit None\n"
		"             OpBranchConditional %eq_0 %if_true %if_false\n"

		// False branch for if-statement.
		"%if_false  = OpLabel\n"
		"%val       = OpLoad %f32 %loc\n"
		"%sub       = OpFSub %f32 %c_f32_1 %val\n"
		"             OpStore %loc %sub\n"
		"             OpBranch %if_exit\n"

		// Merge block for if-statement.
		"%if_exit   = OpLabel\n"
		"%ival_next = OpIAdd %i32 %ival %c_i32_1\n"
		"             OpStore %iptr %ival_next\n"
		"             OpBranch %loop\n"

		// True branch for if-statement.
		"%if_true   = OpLabel\n"
		"             OpStore %loc %c_f32_0\n"
		"             OpBranch %if_exit\n"

		"             OpFunctionEnd\n";

	fragments["testfun"]	= function;

	inputColors[0]			= RGBA(127, 127, 127, 0);
	inputColors[1]			= RGBA(127, 0,   0,   0);
	inputColors[2]			= RGBA(0,   127, 0,   0);
	inputColors[3]			= RGBA(0,   0,   127, 0);

	outputColors[0]			= RGBA(0, 128, 128, 255);
	outputColors[1]			= RGBA(0, 255, 255, 255);
	outputColors[2]			= RGBA(0, 128, 255, 255);
	outputColors[3]			= RGBA(0, 255, 128, 255);

	createTestsForAllStages("out_of_order", inputColors, outputColors, fragments, group.get());

	return group.release();
}

tcu::TestCaseGroup* createSwitchBlockOrderTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group				(new tcu::TestCaseGroup(testCtx, "switch_block_order", "Out-of-order blocks for switch"));
	RGBA							inputColors[4];
	RGBA							outputColors[4];
	map<string, string>				fragments;

	const char						typesAndConstants[]	=
		"%c_f32_p2  = OpConstant %f32 0.2\n"
		"%c_f32_p4  = OpConstant %f32 0.4\n"
		"%c_f32_p6  = OpConstant %f32 0.6\n"
		"%c_f32_p8  = OpConstant %f32 0.8\n";

	// vec4 test_code(vec4 param) {
	//   vec4 result = param;
	//   for (int i = 0; i < 4; ++i) {
	//     switch (i) {
	//       case 0: result[i] += .2; break;
	//       case 1: result[i] += .6; break;
	//       case 2: result[i] += .4; break;
	//       case 3: result[i] += .8; break;
	//       default: break; // unreachable
	//     }
	//   }
	//   return result;
	// }
	const char						function[]			=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1    = OpFunctionParameter %v4f32\n"
		"%lbl       = OpLabel\n"
		"%iptr      = OpVariable %fp_i32 Function\n"
		"%result    = OpVariable %fp_v4f32 Function\n"
		"             OpStore %iptr %c_i32_0\n"
		"             OpStore %result %param1\n"
		"             OpBranch %loop\n"

		// Loop entry block.
		"%loop      = OpLabel\n"
		"%ival      = OpLoad %i32 %iptr\n"
		"%lt_4      = OpSLessThan %bool %ival %c_i32_4\n"
		"             OpLoopMerge %exit %switch_exit None\n"
		"             OpBranchConditional %lt_4 %switch_entry %exit\n"

		// Merge block for loop.
		"%exit      = OpLabel\n"
		"%ret       = OpLoad %v4f32 %result\n"
		"             OpReturnValue %ret\n"

		// Switch-statement entry block.
		"%switch_entry   = OpLabel\n"
		"%loc            = OpAccessChain %fp_f32 %result %ival\n"
		"%val            = OpLoad %f32 %loc\n"
		"                  OpSelectionMerge %switch_exit None\n"
		"                  OpSwitch %ival %switch_default 0 %case0 1 %case1 2 %case2 3 %case3\n"

		"%case2          = OpLabel\n"
		"%addp4          = OpFAdd %f32 %val %c_f32_p4\n"
		"                  OpStore %loc %addp4\n"
		"                  OpBranch %switch_exit\n"

		"%switch_default = OpLabel\n"
		"                  OpUnreachable\n"

		"%case3          = OpLabel\n"
		"%addp8          = OpFAdd %f32 %val %c_f32_p8\n"
		"                  OpStore %loc %addp8\n"
		"                  OpBranch %switch_exit\n"

		"%case0          = OpLabel\n"
		"%addp2          = OpFAdd %f32 %val %c_f32_p2\n"
		"                  OpStore %loc %addp2\n"
		"                  OpBranch %switch_exit\n"

		// Merge block for switch-statement.
		"%switch_exit    = OpLabel\n"
		"%ival_next      = OpIAdd %i32 %ival %c_i32_1\n"
		"                  OpStore %iptr %ival_next\n"
		"                  OpBranch %loop\n"

		"%case1          = OpLabel\n"
		"%addp6          = OpFAdd %f32 %val %c_f32_p6\n"
		"                  OpStore %loc %addp6\n"
		"                  OpBranch %switch_exit\n"

		"                  OpFunctionEnd\n";

	fragments["pre_main"]	= typesAndConstants;
	fragments["testfun"]	= function;

	inputColors[0]			= RGBA(127, 27,  127, 51);
	inputColors[1]			= RGBA(127, 0,   0,   51);
	inputColors[2]			= RGBA(0,   27,  0,   51);
	inputColors[3]			= RGBA(0,   0,   127, 51);

	outputColors[0]			= RGBA(178, 180, 229, 255);
	outputColors[1]			= RGBA(178, 153, 102, 255);
	outputColors[2]			= RGBA(51,  180, 102, 255);
	outputColors[3]			= RGBA(51,  153, 229, 255);

	createTestsForAllStages("out_of_order", inputColors, outputColors, fragments, group.get());

	return group.release();
}

tcu::TestCaseGroup* createDecorationGroupTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group				(new tcu::TestCaseGroup(testCtx, "decoration_group", "Decoration group tests"));
	RGBA							inputColors[4];
	RGBA							outputColors[4];
	map<string, string>				fragments;

	const char						decorations[]		=
		"OpDecorate %array_group         ArrayStride 4\n"
		"OpDecorate %struct_member_group Offset 0\n"
		"%array_group         = OpDecorationGroup\n"
		"%struct_member_group = OpDecorationGroup\n"

		"OpDecorate %group1 RelaxedPrecision\n"
		"OpDecorate %group3 RelaxedPrecision\n"
		"OpDecorate %group3 Invariant\n"
		"OpDecorate %group3 Restrict\n"
		"%group0 = OpDecorationGroup\n"
		"%group1 = OpDecorationGroup\n"
		"%group3 = OpDecorationGroup\n";

	const char						typesAndConstants[]	=
		"%a3f32     = OpTypeArray %f32 %c_u32_3\n"
		"%struct1   = OpTypeStruct %a3f32\n"
		"%struct2   = OpTypeStruct %a3f32\n"
		"%fp_struct1 = OpTypePointer Function %struct1\n"
		"%fp_struct2 = OpTypePointer Function %struct2\n"
		"%c_f32_2    = OpConstant %f32 2.\n"
		"%c_f32_n2   = OpConstant %f32 -2.\n"

		"%c_a3f32_1 = OpConstantComposite %a3f32 %c_f32_1 %c_f32_2 %c_f32_1\n"
		"%c_a3f32_2 = OpConstantComposite %a3f32 %c_f32_n1 %c_f32_n2 %c_f32_n1\n"
		"%c_struct1 = OpConstantComposite %struct1 %c_a3f32_1\n"
		"%c_struct2 = OpConstantComposite %struct2 %c_a3f32_2\n";

	const char						function[]			=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param     = OpFunctionParameter %v4f32\n"
		"%entry     = OpLabel\n"
		"%result    = OpVariable %fp_v4f32 Function\n"
		"%v_struct1 = OpVariable %fp_struct1 Function\n"
		"%v_struct2 = OpVariable %fp_struct2 Function\n"
		"             OpStore %result %param\n"
		"             OpStore %v_struct1 %c_struct1\n"
		"             OpStore %v_struct2 %c_struct2\n"
		"%ptr1      = OpAccessChain %fp_f32 %v_struct1 %c_i32_0 %c_i32_2\n"
		"%val1      = OpLoad %f32 %ptr1\n"
		"%ptr2      = OpAccessChain %fp_f32 %v_struct2 %c_i32_0 %c_i32_2\n"
		"%val2      = OpLoad %f32 %ptr2\n"
		"%addvalues = OpFAdd %f32 %val1 %val2\n"
		"%ptr       = OpAccessChain %fp_f32 %result %c_i32_1\n"
		"%val       = OpLoad %f32 %ptr\n"
		"%addresult = OpFAdd %f32 %addvalues %val\n"
		"             OpStore %ptr %addresult\n"
		"%ret       = OpLoad %v4f32 %result\n"
		"             OpReturnValue %ret\n"
		"             OpFunctionEnd\n";

	struct CaseNameDecoration
	{
		string name;
		string decoration;
	};

	CaseNameDecoration tests[] =
	{
		{
			"same_decoration_group_on_multiple_types",
			"OpGroupMemberDecorate %struct_member_group %struct1 0 %struct2 0\n"
		},
		{
			"empty_decoration_group",
			"OpGroupDecorate %group0      %a3f32\n"
			"OpGroupDecorate %group0      %result\n"
		},
		{
			"one_element_decoration_group",
			"OpGroupDecorate %array_group %a3f32\n"
		},
		{
			"multiple_elements_decoration_group",
			"OpGroupDecorate %group3      %v_struct1\n"
		},
		{
			"multiple_decoration_groups_on_same_variable",
			"OpGroupDecorate %group0      %v_struct2\n"
			"OpGroupDecorate %group1      %v_struct2\n"
			"OpGroupDecorate %group3      %v_struct2\n"
		},
		{
			"same_decoration_group_multiple_times",
			"OpGroupDecorate %group1      %addvalues\n"
			"OpGroupDecorate %group1      %addvalues\n"
			"OpGroupDecorate %group1      %addvalues\n"
		},

	};

	getHalfColorsFullAlpha(inputColors);
	getHalfColorsFullAlpha(outputColors);

	for (size_t idx = 0; idx < (sizeof(tests) / sizeof(tests[0])); ++idx)
	{
		fragments["decoration"]	= decorations + tests[idx].decoration;
		fragments["pre_main"]	= typesAndConstants;
		fragments["testfun"]	= function;

		createTestsForAllStages(tests[idx].name, inputColors, outputColors, fragments, group.get());
	}

	return group.release();
}

struct SpecConstantTwoIntGraphicsCase
{
	const char*		caseName;
	const char*		scDefinition0;
	const char*		scDefinition1;
	const char*		scResultType;
	const char*		scOperation;
	deInt32			scActualValue0;
	deInt32			scActualValue1;
	const char*		resultOperation;
	RGBA			expectedColors[4];

					SpecConstantTwoIntGraphicsCase (const char* name,
											const char* definition0,
											const char* definition1,
											const char* resultType,
											const char* operation,
											deInt32		value0,
											deInt32		value1,
											const char* resultOp,
											const RGBA	(&output)[4])
						: caseName			(name)
						, scDefinition0		(definition0)
						, scDefinition1		(definition1)
						, scResultType		(resultType)
						, scOperation		(operation)
						, scActualValue0	(value0)
						, scActualValue1	(value1)
						, resultOperation	(resultOp)
	{
		expectedColors[0] = output[0];
		expectedColors[1] = output[1];
		expectedColors[2] = output[2];
		expectedColors[3] = output[3];
	}
};

tcu::TestCaseGroup* createSpecConstantTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group				(new tcu::TestCaseGroup(testCtx, "opspecconstantop", "Test the OpSpecConstantOp instruction"));
	vector<SpecConstantTwoIntGraphicsCase>	cases;
	RGBA							inputColors[4];
	RGBA							outputColors0[4];
	RGBA							outputColors1[4];
	RGBA							outputColors2[4];

	const char	decorations1[]			=
		"OpDecorate %sc_0  SpecId 0\n"
		"OpDecorate %sc_1  SpecId 1\n";

	const char	typesAndConstants1[]	=
		"${OPTYPE_DEFINITIONS:opt}"
		"%sc_0      = OpSpecConstant${SC_DEF0}\n"
		"%sc_1      = OpSpecConstant${SC_DEF1}\n"
		"%sc_op     = OpSpecConstantOp ${SC_RESULT_TYPE} ${SC_OP}\n";

	const char	function1[]				=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param     = OpFunctionParameter %v4f32\n"
		"%label     = OpLabel\n"
		"${TYPE_CONVERT:opt}"
		"%result    = OpVariable %fp_v4f32 Function\n"
		"             OpStore %result %param\n"
		"%gen       = ${GEN_RESULT}\n"
		"%index     = OpIAdd %i32 %gen %c_i32_1\n"
		"%loc       = OpAccessChain %fp_f32 %result %index\n"
		"%val       = OpLoad %f32 %loc\n"
		"%add       = OpFAdd %f32 %val %c_f32_0_5\n"
		"             OpStore %loc %add\n"
		"%ret       = OpLoad %v4f32 %result\n"
		"             OpReturnValue %ret\n"
		"             OpFunctionEnd\n";

	inputColors[0] = RGBA(127, 127, 127, 255);
	inputColors[1] = RGBA(127, 0,   0,   255);
	inputColors[2] = RGBA(0,   127, 0,   255);
	inputColors[3] = RGBA(0,   0,   127, 255);

	// Derived from inputColors[x] by adding 128 to inputColors[x][0].
	outputColors0[0] = RGBA(255, 127, 127, 255);
	outputColors0[1] = RGBA(255, 0,   0,   255);
	outputColors0[2] = RGBA(128, 127, 0,   255);
	outputColors0[3] = RGBA(128, 0,   127, 255);

	// Derived from inputColors[x] by adding 128 to inputColors[x][1].
	outputColors1[0] = RGBA(127, 255, 127, 255);
	outputColors1[1] = RGBA(127, 128, 0,   255);
	outputColors1[2] = RGBA(0,   255, 0,   255);
	outputColors1[3] = RGBA(0,   128, 127, 255);

	// Derived from inputColors[x] by adding 128 to inputColors[x][2].
	outputColors2[0] = RGBA(127, 127, 255, 255);
	outputColors2[1] = RGBA(127, 0,   128, 255);
	outputColors2[2] = RGBA(0,   127, 128, 255);
	outputColors2[3] = RGBA(0,   0,   255, 255);

	const char addZeroToSc[]		= "OpIAdd %i32 %c_i32_0 %sc_op";
	const char addZeroToSc32[]		= "OpIAdd %i32 %c_i32_0 %sc_op32";
	const char selectTrueUsingSc[]	= "OpSelect %i32 %sc_op %c_i32_1 %c_i32_0";
	const char selectFalseUsingSc[]	= "OpSelect %i32 %sc_op %c_i32_0 %c_i32_1";

	cases.push_back(SpecConstantTwoIntGraphicsCase("iadd",					" %i32 0",		" %i32 0",		"%i32",		"IAdd                 %sc_0 %sc_1",				19,		-20,	addZeroToSc,		outputColors0));
	cases.push_back(SpecConstantTwoIntGraphicsCase("isub",					" %i32 0",		" %i32 0",		"%i32",		"ISub                 %sc_0 %sc_1",				19,		20,		addZeroToSc,		outputColors0));
	cases.push_back(SpecConstantTwoIntGraphicsCase("imul",					" %i32 0",		" %i32 0",		"%i32",		"IMul                 %sc_0 %sc_1",				-1,		-1,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("sdiv",					" %i32 0",		" %i32 0",		"%i32",		"SDiv                 %sc_0 %sc_1",				-126,	126,	addZeroToSc,		outputColors0));
	cases.push_back(SpecConstantTwoIntGraphicsCase("udiv",					" %i32 0",		" %i32 0",		"%i32",		"UDiv                 %sc_0 %sc_1",				126,	126,	addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("srem",					" %i32 0",		" %i32 0",		"%i32",		"SRem                 %sc_0 %sc_1",				3,		2,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("smod",					" %i32 0",		" %i32 0",		"%i32",		"SMod                 %sc_0 %sc_1",				3,		2,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("umod",					" %i32 0",		" %i32 0",		"%i32",		"UMod                 %sc_0 %sc_1",				1001,	500,	addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("bitwiseand",			" %i32 0",		" %i32 0",		"%i32",		"BitwiseAnd           %sc_0 %sc_1",				0x33,	0x0d,	addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("bitwiseor",				" %i32 0",		" %i32 0",		"%i32",		"BitwiseOr            %sc_0 %sc_1",				0,		1,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("bitwisexor",			" %i32 0",		" %i32 0",		"%i32",		"BitwiseXor           %sc_0 %sc_1",				0x2e,	0x2f,	addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("shiftrightlogical",		" %i32 0",		" %i32 0",		"%i32",		"ShiftRightLogical    %sc_0 %sc_1",				2,		1,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("shiftrightarithmetic",	" %i32 0",		" %i32 0",		"%i32",		"ShiftRightArithmetic %sc_0 %sc_1",				-4,		2,		addZeroToSc,		outputColors0));
	cases.push_back(SpecConstantTwoIntGraphicsCase("shiftleftlogical",		" %i32 0",		" %i32 0",		"%i32",		"ShiftLeftLogical     %sc_0 %sc_1",				1,		0,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("slessthan",				" %i32 0",		" %i32 0",		"%bool",	"SLessThan            %sc_0 %sc_1",				-20,	-10,	selectTrueUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("ulessthan",				" %i32 0",		" %i32 0",		"%bool",	"ULessThan            %sc_0 %sc_1",				10,		20,		selectTrueUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("sgreaterthan",			" %i32 0",		" %i32 0",		"%bool",	"SGreaterThan         %sc_0 %sc_1",				-1000,	50,		selectFalseUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("ugreaterthan",			" %i32 0",		" %i32 0",		"%bool",	"UGreaterThan         %sc_0 %sc_1",				10,		5,		selectTrueUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("slessthanequal",		" %i32 0",		" %i32 0",		"%bool",	"SLessThanEqual       %sc_0 %sc_1",				-10,	-10,	selectTrueUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("ulessthanequal",		" %i32 0",		" %i32 0",		"%bool",	"ULessThanEqual       %sc_0 %sc_1",				50,		100,	selectTrueUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("sgreaterthanequal",		" %i32 0",		" %i32 0",		"%bool",	"SGreaterThanEqual    %sc_0 %sc_1",				-1000,	50,		selectFalseUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("ugreaterthanequal",		" %i32 0",		" %i32 0",		"%bool",	"UGreaterThanEqual    %sc_0 %sc_1",				10,		10,		selectTrueUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("iequal",				" %i32 0",		" %i32 0",		"%bool",	"IEqual               %sc_0 %sc_1",				42,		24,		selectFalseUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("logicaland",			"True %bool",	"True %bool",	"%bool",	"LogicalAnd           %sc_0 %sc_1",				0,		1,		selectFalseUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("logicalor",				"False %bool",	"False %bool",	"%bool",	"LogicalOr            %sc_0 %sc_1",				1,		0,		selectTrueUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("logicalequal",			"True %bool",	"True %bool",	"%bool",	"LogicalEqual         %sc_0 %sc_1",				0,		1,		selectFalseUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("logicalnotequal",		"False %bool",	"False %bool",	"%bool",	"LogicalNotEqual      %sc_0 %sc_1",				1,		0,		selectTrueUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("snegate",				" %i32 0",		" %i32 0",		"%i32",		"SNegate              %sc_0",					-1,		0,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("not",					" %i32 0",		" %i32 0",		"%i32",		"Not                  %sc_0",					-2,		0,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("logicalnot",			"False %bool",	"False %bool",	"%bool",	"LogicalNot           %sc_0",					1,		0,		selectFalseUsingSc,	outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("select",				"False %bool",	" %i32 0",		"%i32",		"Select               %sc_0 %sc_1 %c_i32_0",	1,		1,		addZeroToSc,		outputColors2));
	cases.push_back(SpecConstantTwoIntGraphicsCase("sconvert",				" %i32 0",		" %i32 0",		"%i16",		"SConvert             %sc_0",					-1,		0,		addZeroToSc32,		outputColors0));
	// -1082130432 stored as 32-bit two's complement is the binary representation of -1 as IEEE-754 Float
	cases.push_back(SpecConstantTwoIntGraphicsCase("fconvert",				" %f32 0",		" %f32 0",		"%f64",		"FConvert             %sc_0",					-1082130432, 0,	addZeroToSc32,		outputColors0));
	// \todo[2015-12-1 antiagainst] OpQuantizeToF16

	for (size_t caseNdx = 0; caseNdx < cases.size(); ++caseNdx)
	{
		map<string, string>			specializations;
		map<string, string>			fragments;
		vector<deInt32>				specConstants;
		vector<string>				features;
		PushConstants				noPushConstants;
		GraphicsResources			noResources;
		GraphicsInterfaces			noInterfaces;
		std::vector<std::string>	noExtensions;

		// Special SPIR-V code for SConvert-case
		if (strcmp(cases[caseNdx].caseName, "sconvert") == 0)
		{
			features.push_back("shaderInt16");
			fragments["capability"]					= "OpCapability Int16\n";					// Adds 16-bit integer capability
			specializations["OPTYPE_DEFINITIONS"]	= "%i16 = OpTypeInt 16 1\n";				// Adds 16-bit integer type
			specializations["TYPE_CONVERT"]			= "%sc_op32 = OpSConvert %i32 %sc_op\n";	// Converts 16-bit integer to 32-bit integer
		}

		// Special SPIR-V code for FConvert-case
		if (strcmp(cases[caseNdx].caseName, "fconvert") == 0)
		{
			features.push_back("shaderFloat64");
			fragments["capability"]					= "OpCapability Float64\n";					// Adds 64-bit float capability
			specializations["OPTYPE_DEFINITIONS"]	= "%f64 = OpTypeFloat 64\n";				// Adds 64-bit float type
			specializations["TYPE_CONVERT"]			= "%sc_op32 = OpConvertFToS %i32 %sc_op\n";	// Converts 64-bit float to 32-bit integer
		}

		specializations["SC_DEF0"]			= cases[caseNdx].scDefinition0;
		specializations["SC_DEF1"]			= cases[caseNdx].scDefinition1;
		specializations["SC_RESULT_TYPE"]	= cases[caseNdx].scResultType;
		specializations["SC_OP"]			= cases[caseNdx].scOperation;
		specializations["GEN_RESULT"]		= cases[caseNdx].resultOperation;

		fragments["decoration"]				= tcu::StringTemplate(decorations1).specialize(specializations);
		fragments["pre_main"]				= tcu::StringTemplate(typesAndConstants1).specialize(specializations);
		fragments["testfun"]				= tcu::StringTemplate(function1).specialize(specializations);

		specConstants.push_back(cases[caseNdx].scActualValue0);
		specConstants.push_back(cases[caseNdx].scActualValue1);

		createTestsForAllStages(
			cases[caseNdx].caseName, inputColors, cases[caseNdx].expectedColors, fragments, specConstants,
			noPushConstants, noResources, noInterfaces, noExtensions, features, VulkanFeatures(), group.get());
	}

	const char	decorations2[]			=
		"OpDecorate %sc_0  SpecId 0\n"
		"OpDecorate %sc_1  SpecId 1\n"
		"OpDecorate %sc_2  SpecId 2\n";

	const char	typesAndConstants2[]	=
		"%v3i32       = OpTypeVector %i32 3\n"
		"%vec3_0      = OpConstantComposite %v3i32 %c_i32_0 %c_i32_0 %c_i32_0\n"
		"%vec3_undef  = OpUndef %v3i32\n"

		"%sc_0        = OpSpecConstant %i32 0\n"
		"%sc_1        = OpSpecConstant %i32 0\n"
		"%sc_2        = OpSpecConstant %i32 0\n"
		"%sc_vec3_0   = OpSpecConstantOp %v3i32 CompositeInsert  %sc_0        %vec3_0      0\n"							// (sc_0, 0,    0)
		"%sc_vec3_1   = OpSpecConstantOp %v3i32 CompositeInsert  %sc_1        %vec3_0      1\n"							// (0,    sc_1, 0)
		"%sc_vec3_2   = OpSpecConstantOp %v3i32 CompositeInsert  %sc_2        %vec3_0      2\n"							// (0,    0,    sc_2)
		"%sc_vec3_0_s = OpSpecConstantOp %v3i32 VectorShuffle    %sc_vec3_0   %vec3_undef  0          0xFFFFFFFF 2\n"	// (sc_0, ???,  0)
		"%sc_vec3_1_s = OpSpecConstantOp %v3i32 VectorShuffle    %sc_vec3_1   %vec3_undef  0xFFFFFFFF 1          0\n"	// (???,  sc_1, 0)
		"%sc_vec3_2_s = OpSpecConstantOp %v3i32 VectorShuffle    %vec3_undef  %sc_vec3_2   5          0xFFFFFFFF 5\n"	// (sc_2, ???,  sc_2)
		"%sc_vec3_01  = OpSpecConstantOp %v3i32 VectorShuffle    %sc_vec3_0_s %sc_vec3_1_s 1 0 4\n"						// (0,    sc_0, sc_1)
		"%sc_vec3_012 = OpSpecConstantOp %v3i32 VectorShuffle    %sc_vec3_01  %sc_vec3_2_s 5 1 2\n"						// (sc_2, sc_0, sc_1)
		"%sc_ext_0    = OpSpecConstantOp %i32   CompositeExtract %sc_vec3_012              0\n"							// sc_2
		"%sc_ext_1    = OpSpecConstantOp %i32   CompositeExtract %sc_vec3_012              1\n"							// sc_0
		"%sc_ext_2    = OpSpecConstantOp %i32   CompositeExtract %sc_vec3_012              2\n"							// sc_1
		"%sc_sub      = OpSpecConstantOp %i32   ISub             %sc_ext_0    %sc_ext_1\n"								// (sc_2 - sc_0)
		"%sc_final    = OpSpecConstantOp %i32   IMul             %sc_sub      %sc_ext_2\n";								// (sc_2 - sc_0) * sc_1

	const char	function2[]				=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param     = OpFunctionParameter %v4f32\n"
		"%label     = OpLabel\n"
		"%result    = OpVariable %fp_v4f32 Function\n"
		"             OpStore %result %param\n"
		"%loc       = OpAccessChain %fp_f32 %result %sc_final\n"
		"%val       = OpLoad %f32 %loc\n"
		"%add       = OpFAdd %f32 %val %c_f32_0_5\n"
		"             OpStore %loc %add\n"
		"%ret       = OpLoad %v4f32 %result\n"
		"             OpReturnValue %ret\n"
		"             OpFunctionEnd\n";

	map<string, string>	fragments;
	vector<deInt32>		specConstants;

	fragments["decoration"]	= decorations2;
	fragments["pre_main"]	= typesAndConstants2;
	fragments["testfun"]	= function2;

	specConstants.push_back(56789);
	specConstants.push_back(-2);
	specConstants.push_back(56788);

	createTestsForAllStages("vector_related", inputColors, outputColors2, fragments, specConstants, group.get());

	return group.release();
}

tcu::TestCaseGroup* createOpPhiTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group				(new tcu::TestCaseGroup(testCtx, "opphi", "Test the OpPhi instruction"));
	RGBA							inputColors[4];
	RGBA							outputColors1[4];
	RGBA							outputColors2[4];
	RGBA							outputColors3[4];
	map<string, string>				fragments1;
	map<string, string>				fragments2;
	map<string, string>				fragments3;

	const char	typesAndConstants1[]	=
		"%c_f32_p2  = OpConstant %f32 0.2\n"
		"%c_f32_p4  = OpConstant %f32 0.4\n"
		"%c_f32_p5  = OpConstant %f32 0.5\n"
		"%c_f32_p8  = OpConstant %f32 0.8\n";

	// vec4 test_code(vec4 param) {
	//   vec4 result = param;
	//   for (int i = 0; i < 4; ++i) {
	//     float operand;
	//     switch (i) {
	//       case 0: operand = .2; break;
	//       case 1: operand = .5; break;
	//       case 2: operand = .4; break;
	//       case 3: operand = .0; break;
	//       default: break; // unreachable
	//     }
	//     result[i] += operand;
	//   }
	//   return result;
	// }
	const char	function1[]				=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1    = OpFunctionParameter %v4f32\n"
		"%lbl       = OpLabel\n"
		"%iptr      = OpVariable %fp_i32 Function\n"
		"%result    = OpVariable %fp_v4f32 Function\n"
		"             OpStore %iptr %c_i32_0\n"
		"             OpStore %result %param1\n"
		"             OpBranch %loop\n"

		"%loop      = OpLabel\n"
		"%ival      = OpLoad %i32 %iptr\n"
		"%lt_4      = OpSLessThan %bool %ival %c_i32_4\n"
		"             OpLoopMerge %exit %phi None\n"
		"             OpBranchConditional %lt_4 %entry %exit\n"

		"%entry     = OpLabel\n"
		"%loc       = OpAccessChain %fp_f32 %result %ival\n"
		"%val       = OpLoad %f32 %loc\n"
		"             OpSelectionMerge %phi None\n"
		"             OpSwitch %ival %default 0 %case0 1 %case1 2 %case2 3 %case3\n"

		"%case0     = OpLabel\n"
		"             OpBranch %phi\n"
		"%case1     = OpLabel\n"
		"             OpBranch %phi\n"
		"%case2     = OpLabel\n"
		"             OpBranch %phi\n"
		"%case3     = OpLabel\n"
		"             OpBranch %phi\n"

		"%default   = OpLabel\n"
		"             OpUnreachable\n"

		"%phi       = OpLabel\n"
		"%operand   = OpPhi %f32 %c_f32_p4 %case2 %c_f32_p5 %case1 %c_f32_p2 %case0 %c_f32_0 %case3\n" // not in the order of blocks
		"%add       = OpFAdd %f32 %val %operand\n"
		"             OpStore %loc %add\n"
		"%ival_next = OpIAdd %i32 %ival %c_i32_1\n"
		"             OpStore %iptr %ival_next\n"
		"             OpBranch %loop\n"

		"%exit      = OpLabel\n"
		"%ret       = OpLoad %v4f32 %result\n"
		"             OpReturnValue %ret\n"

		"             OpFunctionEnd\n";

	fragments1["pre_main"]	= typesAndConstants1;
	fragments1["testfun"]	= function1;

	getHalfColorsFullAlpha(inputColors);

	outputColors1[0]		= RGBA(178, 255, 229, 255);
	outputColors1[1]		= RGBA(178, 127, 102, 255);
	outputColors1[2]		= RGBA(51,  255, 102, 255);
	outputColors1[3]		= RGBA(51,  127, 229, 255);

	createTestsForAllStages("out_of_order", inputColors, outputColors1, fragments1, group.get());

	const char	typesAndConstants2[]	=
		"%c_f32_p2  = OpConstant %f32 0.2\n";

	// Add .4 to the second element of the given parameter.
	const char	function2[]				=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param     = OpFunctionParameter %v4f32\n"
		"%entry     = OpLabel\n"
		"%result    = OpVariable %fp_v4f32 Function\n"
		"             OpStore %result %param\n"
		"%loc       = OpAccessChain %fp_f32 %result %c_i32_1\n"
		"%val       = OpLoad %f32 %loc\n"
		"             OpBranch %phi\n"

		"%phi        = OpLabel\n"
		"%step       = OpPhi %i32 %c_i32_0  %entry %step_next  %phi\n"
		"%accum      = OpPhi %f32 %val      %entry %accum_next %phi\n"
		"%step_next  = OpIAdd %i32 %step  %c_i32_1\n"
		"%accum_next = OpFAdd %f32 %accum %c_f32_p2\n"
		"%still_loop = OpSLessThan %bool %step %c_i32_2\n"
		"              OpLoopMerge %exit %phi None\n"
		"              OpBranchConditional %still_loop %phi %exit\n"

		"%exit       = OpLabel\n"
		"              OpStore %loc %accum\n"
		"%ret        = OpLoad %v4f32 %result\n"
		"              OpReturnValue %ret\n"

		"              OpFunctionEnd\n";

	fragments2["pre_main"]	= typesAndConstants2;
	fragments2["testfun"]	= function2;

	outputColors2[0]			= RGBA(127, 229, 127, 255);
	outputColors2[1]			= RGBA(127, 102, 0,   255);
	outputColors2[2]			= RGBA(0,   229, 0,   255);
	outputColors2[3]			= RGBA(0,   102, 127, 255);

	createTestsForAllStages("induction", inputColors, outputColors2, fragments2, group.get());

	const char	typesAndConstants3[]	=
		"%true      = OpConstantTrue %bool\n"
		"%false     = OpConstantFalse %bool\n"
		"%c_f32_p2  = OpConstant %f32 0.2\n";

	// Swap the second and the third element of the given parameter.
	const char	function3[]				=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param     = OpFunctionParameter %v4f32\n"
		"%entry     = OpLabel\n"
		"%result    = OpVariable %fp_v4f32 Function\n"
		"             OpStore %result %param\n"
		"%a_loc     = OpAccessChain %fp_f32 %result %c_i32_1\n"
		"%a_init    = OpLoad %f32 %a_loc\n"
		"%b_loc     = OpAccessChain %fp_f32 %result %c_i32_2\n"
		"%b_init    = OpLoad %f32 %b_loc\n"
		"             OpBranch %phi\n"

		"%phi        = OpLabel\n"
		"%still_loop = OpPhi %bool %true   %entry %false  %phi\n"
		"%a_next     = OpPhi %f32  %a_init %entry %b_next %phi\n"
		"%b_next     = OpPhi %f32  %b_init %entry %a_next %phi\n"
		"              OpLoopMerge %exit %phi None\n"
		"              OpBranchConditional %still_loop %phi %exit\n"

		"%exit       = OpLabel\n"
		"              OpStore %a_loc %a_next\n"
		"              OpStore %b_loc %b_next\n"
		"%ret        = OpLoad %v4f32 %result\n"
		"              OpReturnValue %ret\n"

		"              OpFunctionEnd\n";

	fragments3["pre_main"]	= typesAndConstants3;
	fragments3["testfun"]	= function3;

	outputColors3[0]			= RGBA(127, 127, 127, 255);
	outputColors3[1]			= RGBA(127, 0,   0,   255);
	outputColors3[2]			= RGBA(0,   0,   127, 255);
	outputColors3[3]			= RGBA(0,   127, 0,   255);

	createTestsForAllStages("swap", inputColors, outputColors3, fragments3, group.get());

	return group.release();
}

tcu::TestCaseGroup* createNoContractionTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group			(new tcu::TestCaseGroup(testCtx, "nocontraction", "Test the NoContraction decoration"));
	RGBA							inputColors[4];
	RGBA							outputColors[4];

	// With NoContraction, (1 + 2^-23) * (1 - 2^-23) - 1 should be conducted as a multiplication and an addition separately.
	// For the multiplication, the result is 1 - 2^-46, which is out of the precision range for 32-bit float. (32-bit float
	// only have 23-bit fraction.) So it will be rounded to 1. Or 0x1.fffffc. Then the final result is 0 or -0x1p-24.
	// On the contrary, the result will be 2^-46, which is a normalized number perfectly representable as 32-bit float.
	const char						constantsAndTypes[]	 =
		"%c_vec4_0       = OpConstantComposite %v4f32 %c_f32_0 %c_f32_0 %c_f32_0 %c_f32_1\n"
		"%c_vec4_1       = OpConstantComposite %v4f32 %c_f32_1 %c_f32_1 %c_f32_1 %c_f32_1\n"
		"%c_f32_1pl2_23  = OpConstant %f32 0x1.000002p+0\n" // 1 + 2^-23
		"%c_f32_1mi2_23  = OpConstant %f32 0x1.fffffcp-1\n" // 1 - 2^-23
		"%c_f32_n1pn24   = OpConstant %f32 -0x1p-24\n";

	const char						function[]	 =
		"%test_code      = OpFunction %v4f32 None %v4f32_function\n"
		"%param          = OpFunctionParameter %v4f32\n"
		"%label          = OpLabel\n"
		"%var1           = OpVariable %fp_f32 Function %c_f32_1pl2_23\n"
		"%var2           = OpVariable %fp_f32 Function\n"
		"%red            = OpCompositeExtract %f32 %param 0\n"
		"%plus_red       = OpFAdd %f32 %c_f32_1mi2_23 %red\n"
		"                  OpStore %var2 %plus_red\n"
		"%val1           = OpLoad %f32 %var1\n"
		"%val2           = OpLoad %f32 %var2\n"
		"%mul            = OpFMul %f32 %val1 %val2\n"
		"%add            = OpFAdd %f32 %mul %c_f32_n1\n"
		"%is0            = OpFOrdEqual %bool %add %c_f32_0\n"
		"%isn1n24         = OpFOrdEqual %bool %add %c_f32_n1pn24\n"
		"%success        = OpLogicalOr %bool %is0 %isn1n24\n"
		"%v4success      = OpCompositeConstruct %v4bool %success %success %success %success\n"
		"%ret            = OpSelect %v4f32 %v4success %c_vec4_0 %c_vec4_1\n"
		"                  OpReturnValue %ret\n"
		"                  OpFunctionEnd\n";

	struct CaseNameDecoration
	{
		string name;
		string decoration;
	};


	CaseNameDecoration tests[] = {
		{"multiplication",	"OpDecorate %mul NoContraction"},
		{"addition",		"OpDecorate %add NoContraction"},
		{"both",			"OpDecorate %mul NoContraction\nOpDecorate %add NoContraction"},
	};

	getHalfColorsFullAlpha(inputColors);

	for (deUint8 idx = 0; idx < 4; ++idx)
	{
		inputColors[idx].setRed(0);
		outputColors[idx] = RGBA(0, 0, 0, 255);
	}

	for (size_t testNdx = 0; testNdx < sizeof(tests) / sizeof(CaseNameDecoration); ++testNdx)
	{
		map<string, string> fragments;

		fragments["decoration"] = tests[testNdx].decoration;
		fragments["pre_main"] = constantsAndTypes;
		fragments["testfun"] = function;

		createTestsForAllStages(tests[testNdx].name, inputColors, outputColors, fragments, group.get());
	}

	return group.release();
}

tcu::TestCaseGroup* createMemoryAccessTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> memoryAccessTests (new tcu::TestCaseGroup(testCtx, "opmemoryaccess", "Memory Semantics"));
	RGBA							colors[4];

	const char						constantsAndTypes[]	 =
		"%c_a2f32_1         = OpConstantComposite %a2f32 %c_f32_1 %c_f32_1\n"
		"%fp_a2f32          = OpTypePointer Function %a2f32\n"
		"%stype             = OpTypeStruct  %v4f32 %a2f32 %f32\n"
		"%fp_stype          = OpTypePointer Function %stype\n";

	const char						function[]	 =
		"%test_code         = OpFunction %v4f32 None %v4f32_function\n"
		"%param1            = OpFunctionParameter %v4f32\n"
		"%lbl               = OpLabel\n"
		"%v1                = OpVariable %fp_v4f32 Function\n"
		"%v2                = OpVariable %fp_a2f32 Function\n"
		"%v3                = OpVariable %fp_f32 Function\n"
		"%v                 = OpVariable %fp_stype Function\n"
		"%vv                = OpVariable %fp_stype Function\n"
		"%vvv               = OpVariable %fp_f32 Function\n"

		"                     OpStore %v1 %c_v4f32_1_1_1_1\n"
		"                     OpStore %v2 %c_a2f32_1\n"
		"                     OpStore %v3 %c_f32_1\n"

		"%p_v4f32          = OpAccessChain %fp_v4f32 %v %c_u32_0\n"
		"%p_a2f32          = OpAccessChain %fp_a2f32 %v %c_u32_1\n"
		"%p_f32            = OpAccessChain %fp_f32 %v %c_u32_2\n"
		"%v1_v             = OpLoad %v4f32 %v1 ${access_type}\n"
		"%v2_v             = OpLoad %a2f32 %v2 ${access_type}\n"
		"%v3_v             = OpLoad %f32 %v3 ${access_type}\n"

		"                    OpStore %p_v4f32 %v1_v ${access_type}\n"
		"                    OpStore %p_a2f32 %v2_v ${access_type}\n"
		"                    OpStore %p_f32 %v3_v ${access_type}\n"

		"                    OpCopyMemory %vv %v ${access_type}\n"
		"                    OpCopyMemory %vvv %p_f32 ${access_type}\n"

		"%p_f32_2          = OpAccessChain %fp_f32 %vv %c_u32_2\n"
		"%v_f32_2          = OpLoad %f32 %p_f32_2\n"
		"%v_f32_3          = OpLoad %f32 %vvv\n"

		"%ret1             = OpVectorTimesScalar %v4f32 %param1 %v_f32_2\n"
		"%ret2             = OpVectorTimesScalar %v4f32 %ret1 %v_f32_3\n"
		"                    OpReturnValue %ret2\n"
		"                    OpFunctionEnd\n";

	struct NameMemoryAccess
	{
		string name;
		string accessType;
	};


	NameMemoryAccess tests[] =
	{
		{ "none", "" },
		{ "volatile", "Volatile" },
		{ "aligned",  "Aligned 1" },
		{ "volatile_aligned",  "Volatile|Aligned 1" },
		{ "nontemporal_aligned",  "Nontemporal|Aligned 1" },
		{ "volatile_nontemporal",  "Volatile|Nontemporal" },
		{ "volatile_nontermporal_aligned",  "Volatile|Nontemporal|Aligned 1" },
	};

	getHalfColorsFullAlpha(colors);

	for (size_t testNdx = 0; testNdx < sizeof(tests) / sizeof(NameMemoryAccess); ++testNdx)
	{
		map<string, string> fragments;
		map<string, string> memoryAccess;
		memoryAccess["access_type"] = tests[testNdx].accessType;

		fragments["pre_main"] = constantsAndTypes;
		fragments["testfun"] = tcu::StringTemplate(function).specialize(memoryAccess);
		createTestsForAllStages(tests[testNdx].name, colors, colors, fragments, memoryAccessTests.get());
	}
	return memoryAccessTests.release();
}
tcu::TestCaseGroup* createOpUndefTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>		opUndefTests		 (new tcu::TestCaseGroup(testCtx, "opundef", "Test OpUndef"));
	RGBA								defaultColors[4];
	map<string, string>					fragments;
	getDefaultColors(defaultColors);

	// First, simple cases that don't do anything with the OpUndef result.
	struct NameCodePair { string name, decl, type; };
	const NameCodePair tests[] =
	{
		{"bool", "", "%bool"},
		{"vec2uint32", "", "%v2u32"},
		{"image", "%type = OpTypeImage %f32 2D 0 0 0 1 Unknown", "%type"},
		{"sampler", "%type = OpTypeSampler", "%type"},
		{"sampledimage", "%img = OpTypeImage %f32 2D 0 0 0 1 Unknown\n" "%type = OpTypeSampledImage %img", "%type"},
		{"pointer", "", "%fp_i32"},
		{"runtimearray", "%type = OpTypeRuntimeArray %f32", "%type"},
		{"array", "%c_u32_100 = OpConstant %u32 100\n" "%type = OpTypeArray %i32 %c_u32_100", "%type"},
		{"struct", "%type = OpTypeStruct %f32 %i32 %u32", "%type"}};
	for (size_t testNdx = 0; testNdx < sizeof(tests) / sizeof(NameCodePair); ++testNdx)
	{
		fragments["undef_type"] = tests[testNdx].type;
		fragments["testfun"] = StringTemplate(
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"%param1 = OpFunctionParameter %v4f32\n"
			"%label_testfun = OpLabel\n"
			"%undef = OpUndef ${undef_type}\n"
			"OpReturnValue %param1\n"
			"OpFunctionEnd\n").specialize(fragments);
		fragments["pre_main"] = tests[testNdx].decl;
		createTestsForAllStages(tests[testNdx].name, defaultColors, defaultColors, fragments, opUndefTests.get());
	}
	fragments.clear();

	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%undef = OpUndef %f32\n"
		"%zero = OpFMul %f32 %undef %c_f32_0\n"
		"%is_nan = OpIsNan %bool %zero\n" //OpUndef may result in NaN which may turn %zero into Nan.
		"%actually_zero = OpSelect %f32 %is_nan %c_f32_0 %zero\n"
		"%a = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"%b = OpFAdd %f32 %a %actually_zero\n"
		"%ret = OpVectorInsertDynamic %v4f32 %param1 %b %c_i32_0\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";

	createTestsForAllStages("float32", defaultColors, defaultColors, fragments, opUndefTests.get());

	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%undef = OpUndef %i32\n"
		"%zero = OpIMul %i32 %undef %c_i32_0\n"
		"%a = OpVectorExtractDynamic %f32 %param1 %zero\n"
		"%ret = OpVectorInsertDynamic %v4f32 %param1 %a %c_i32_0\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";

	createTestsForAllStages("sint32", defaultColors, defaultColors, fragments, opUndefTests.get());

	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%undef = OpUndef %u32\n"
		"%zero = OpIMul %u32 %undef %c_i32_0\n"
		"%a = OpVectorExtractDynamic %f32 %param1 %zero\n"
		"%ret = OpVectorInsertDynamic %v4f32 %param1 %a %c_i32_0\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";

	createTestsForAllStages("uint32", defaultColors, defaultColors, fragments, opUndefTests.get());

	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%undef = OpUndef %v4f32\n"
		"%vzero = OpVectorTimesScalar %v4f32 %undef %c_f32_0\n"
		"%zero_0 = OpVectorExtractDynamic %f32 %vzero %c_i32_0\n"
		"%zero_1 = OpVectorExtractDynamic %f32 %vzero %c_i32_1\n"
		"%zero_2 = OpVectorExtractDynamic %f32 %vzero %c_i32_2\n"
		"%zero_3 = OpVectorExtractDynamic %f32 %vzero %c_i32_3\n"
		"%is_nan_0 = OpIsNan %bool %zero_0\n"
		"%is_nan_1 = OpIsNan %bool %zero_1\n"
		"%is_nan_2 = OpIsNan %bool %zero_2\n"
		"%is_nan_3 = OpIsNan %bool %zero_3\n"
		"%actually_zero_0 = OpSelect %f32 %is_nan_0 %c_f32_0 %zero_0\n"
		"%actually_zero_1 = OpSelect %f32 %is_nan_0 %c_f32_0 %zero_1\n"
		"%actually_zero_2 = OpSelect %f32 %is_nan_0 %c_f32_0 %zero_2\n"
		"%actually_zero_3 = OpSelect %f32 %is_nan_0 %c_f32_0 %zero_3\n"
		"%param1_0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"%param1_1 = OpVectorExtractDynamic %f32 %param1 %c_i32_1\n"
		"%param1_2 = OpVectorExtractDynamic %f32 %param1 %c_i32_2\n"
		"%param1_3 = OpVectorExtractDynamic %f32 %param1 %c_i32_3\n"
		"%sum_0 = OpFAdd %f32 %param1_0 %actually_zero_0\n"
		"%sum_1 = OpFAdd %f32 %param1_1 %actually_zero_1\n"
		"%sum_2 = OpFAdd %f32 %param1_2 %actually_zero_2\n"
		"%sum_3 = OpFAdd %f32 %param1_3 %actually_zero_3\n"
		"%ret3 = OpVectorInsertDynamic %v4f32 %param1 %sum_3 %c_i32_3\n"
		"%ret2 = OpVectorInsertDynamic %v4f32 %ret3 %sum_2 %c_i32_2\n"
		"%ret1 = OpVectorInsertDynamic %v4f32 %ret2 %sum_1 %c_i32_1\n"
		"%ret = OpVectorInsertDynamic %v4f32 %ret1 %sum_0 %c_i32_0\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";

	createTestsForAllStages("vec4float32", defaultColors, defaultColors, fragments, opUndefTests.get());

	fragments["pre_main"] =
		"%m2x2f32 = OpTypeMatrix %v2f32 2\n";
	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%undef = OpUndef %m2x2f32\n"
		"%mzero = OpMatrixTimesScalar %m2x2f32 %undef %c_f32_0\n"
		"%zero_0 = OpCompositeExtract %f32 %mzero 0 0\n"
		"%zero_1 = OpCompositeExtract %f32 %mzero 0 1\n"
		"%zero_2 = OpCompositeExtract %f32 %mzero 1 0\n"
		"%zero_3 = OpCompositeExtract %f32 %mzero 1 1\n"
		"%is_nan_0 = OpIsNan %bool %zero_0\n"
		"%is_nan_1 = OpIsNan %bool %zero_1\n"
		"%is_nan_2 = OpIsNan %bool %zero_2\n"
		"%is_nan_3 = OpIsNan %bool %zero_3\n"
		"%actually_zero_0 = OpSelect %f32 %is_nan_0 %c_f32_0 %zero_0\n"
		"%actually_zero_1 = OpSelect %f32 %is_nan_0 %c_f32_0 %zero_1\n"
		"%actually_zero_2 = OpSelect %f32 %is_nan_0 %c_f32_0 %zero_2\n"
		"%actually_zero_3 = OpSelect %f32 %is_nan_0 %c_f32_0 %zero_3\n"
		"%param1_0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"%param1_1 = OpVectorExtractDynamic %f32 %param1 %c_i32_1\n"
		"%param1_2 = OpVectorExtractDynamic %f32 %param1 %c_i32_2\n"
		"%param1_3 = OpVectorExtractDynamic %f32 %param1 %c_i32_3\n"
		"%sum_0 = OpFAdd %f32 %param1_0 %actually_zero_0\n"
		"%sum_1 = OpFAdd %f32 %param1_1 %actually_zero_1\n"
		"%sum_2 = OpFAdd %f32 %param1_2 %actually_zero_2\n"
		"%sum_3 = OpFAdd %f32 %param1_3 %actually_zero_3\n"
		"%ret3 = OpVectorInsertDynamic %v4f32 %param1 %sum_3 %c_i32_3\n"
		"%ret2 = OpVectorInsertDynamic %v4f32 %ret3 %sum_2 %c_i32_2\n"
		"%ret1 = OpVectorInsertDynamic %v4f32 %ret2 %sum_1 %c_i32_1\n"
		"%ret = OpVectorInsertDynamic %v4f32 %ret1 %sum_0 %c_i32_0\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";

	createTestsForAllStages("matrix", defaultColors, defaultColors, fragments, opUndefTests.get());

	return opUndefTests.release();
}

void createOpQuantizeSingleOptionTests(tcu::TestCaseGroup* testCtx)
{
	const RGBA		inputColors[4]		=
	{
		RGBA(0,		0,		0,		255),
		RGBA(0,		0,		255,	255),
		RGBA(0,		255,	0,		255),
		RGBA(0,		255,	255,	255)
	};

	const RGBA		expectedColors[4]	=
	{
		RGBA(255,	 0,		 0,		 255),
		RGBA(255,	 0,		 0,		 255),
		RGBA(255,	 0,		 0,		 255),
		RGBA(255,	 0,		 0,		 255)
	};

	const struct SingleFP16Possibility
	{
		const char* name;
		const char* constant;  // Value to assign to %test_constant.
		float		valueAsFloat;
		const char* condition; // Must assign to %cond an expression that evaluates to true after %c = OpQuantizeToF16(%test_constant + 0).
	}				tests[]				=
	{
		{
			"negative",
			"-0x1.3p1\n",
			-constructNormalizedFloat(1, 0x300000),
			"%cond = OpFOrdEqual %bool %c %test_constant\n"
		}, // -19
		{
			"positive",
			"0x1.0p7\n",
			constructNormalizedFloat(7, 0x000000),
			"%cond = OpFOrdEqual %bool %c %test_constant\n"
		},  // +128
		// SPIR-V requires that OpQuantizeToF16 flushes
		// any numbers that would end up denormalized in F16 to zero.
		{
			"denorm",
			"0x0.0006p-126\n",
			std::ldexp(1.5f, -140),
			"%cond = OpFOrdEqual %bool %c %c_f32_0\n"
		},  // denorm
		{
			"negative_denorm",
			"-0x0.0006p-126\n",
			-std::ldexp(1.5f, -140),
			"%cond = OpFOrdEqual %bool %c %c_f32_0\n"
		}, // -denorm
		{
			"too_small",
			"0x1.0p-16\n",
			std::ldexp(1.0f, -16),
			"%cond = OpFOrdEqual %bool %c %c_f32_0\n"
		},     // too small positive
		{
			"negative_too_small",
			"-0x1.0p-32\n",
			-std::ldexp(1.0f, -32),
			"%cond = OpFOrdEqual %bool %c %c_f32_0\n"
		},      // too small negative
		{
			"negative_inf",
			"-0x1.0p128\n",
			-std::ldexp(1.0f, 128),

			"%gz = OpFOrdLessThan %bool %c %c_f32_0\n"
			"%inf = OpIsInf %bool %c\n"
			"%cond = OpLogicalAnd %bool %gz %inf\n"
		},     // -inf to -inf
		{
			"inf",
			"0x1.0p128\n",
			std::ldexp(1.0f, 128),

			"%gz = OpFOrdGreaterThan %bool %c %c_f32_0\n"
			"%inf = OpIsInf %bool %c\n"
			"%cond = OpLogicalAnd %bool %gz %inf\n"
		},     // +inf to +inf
		{
			"round_to_negative_inf",
			"-0x1.0p32\n",
			-std::ldexp(1.0f, 32),

			"%gz = OpFOrdLessThan %bool %c %c_f32_0\n"
			"%inf = OpIsInf %bool %c\n"
			"%cond = OpLogicalAnd %bool %gz %inf\n"
		},     // round to -inf
		{
			"round_to_inf",
			"0x1.0p16\n",
			std::ldexp(1.0f, 16),

			"%gz = OpFOrdGreaterThan %bool %c %c_f32_0\n"
			"%inf = OpIsInf %bool %c\n"
			"%cond = OpLogicalAnd %bool %gz %inf\n"
		},     // round to +inf
		{
			"nan",
			"0x1.1p128\n",
			std::numeric_limits<float>::quiet_NaN(),

			// Test for any NaN value, as NaNs are not preserved
			"%direct_quant = OpQuantizeToF16 %f32 %test_constant\n"
			"%cond = OpIsNan %bool %direct_quant\n"
		}, // nan
		{
			"negative_nan",
			"-0x1.0001p128\n",
			std::numeric_limits<float>::quiet_NaN(),

			// Test for any NaN value, as NaNs are not preserved
			"%direct_quant = OpQuantizeToF16 %f32 %test_constant\n"
			"%cond = OpIsNan %bool %direct_quant\n"
		} // -nan
	};
	const char*		constants			=
		"%test_constant = OpConstant %f32 ";  // The value will be test.constant.

	StringTemplate	function			(
		"%test_code     = OpFunction %v4f32 None %v4f32_function\n"
		"%param1        = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%a             = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"%b             = OpFAdd %f32 %test_constant %a\n"
		"%c             = OpQuantizeToF16 %f32 %b\n"
		"${condition}\n"
		"%v4cond        = OpCompositeConstruct %v4bool %cond %cond %cond %cond\n"
		"%retval        = OpSelect %v4f32 %v4cond %c_v4f32_1_0_0_1 %param1\n"
		"                 OpReturnValue %retval\n"
		"OpFunctionEnd\n"
	);

	const char*		specDecorations		= "OpDecorate %test_constant SpecId 0\n";
	const char*		specConstants		=
			"%test_constant = OpSpecConstant %f32 0.\n"
			"%c             = OpSpecConstantOp %f32 QuantizeToF16 %test_constant\n";

	StringTemplate	specConstantFunction(
		"%test_code     = OpFunction %v4f32 None %v4f32_function\n"
		"%param1        = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"${condition}\n"
		"%v4cond        = OpCompositeConstruct %v4bool %cond %cond %cond %cond\n"
		"%retval        = OpSelect %v4f32 %v4cond %c_v4f32_1_0_0_1 %param1\n"
		"                 OpReturnValue %retval\n"
		"OpFunctionEnd\n"
	);

	for (size_t idx = 0; idx < (sizeof(tests)/sizeof(tests[0])); ++idx)
	{
		map<string, string>								codeSpecialization;
		map<string, string>								fragments;
		codeSpecialization["condition"]					= tests[idx].condition;
		fragments["testfun"]							= function.specialize(codeSpecialization);
		fragments["pre_main"]							= string(constants) + tests[idx].constant + "\n";
		createTestsForAllStages(tests[idx].name, inputColors, expectedColors, fragments, testCtx);
	}

	for (size_t idx = 0; idx < (sizeof(tests)/sizeof(tests[0])); ++idx)
	{
		map<string, string>								codeSpecialization;
		map<string, string>								fragments;
		vector<deInt32>									passConstants;
		deInt32											specConstant;

		codeSpecialization["condition"]					= tests[idx].condition;
		fragments["testfun"]							= specConstantFunction.specialize(codeSpecialization);
		fragments["decoration"]							= specDecorations;
		fragments["pre_main"]							= specConstants;

		memcpy(&specConstant, &tests[idx].valueAsFloat, sizeof(float));
		passConstants.push_back(specConstant);

		createTestsForAllStages(string("spec_const_") + tests[idx].name, inputColors, expectedColors, fragments, passConstants, testCtx);
	}
}

void createOpQuantizeTwoPossibilityTests(tcu::TestCaseGroup* testCtx)
{
	RGBA inputColors[4] =  {
		RGBA(0,		0,		0,		255),
		RGBA(0,		0,		255,	255),
		RGBA(0,		255,	0,		255),
		RGBA(0,		255,	255,	255)
	};

	RGBA expectedColors[4] =
	{
		RGBA(255,	 0,		 0,		 255),
		RGBA(255,	 0,		 0,		 255),
		RGBA(255,	 0,		 0,		 255),
		RGBA(255,	 0,		 0,		 255)
	};

	struct DualFP16Possibility
	{
		const char* name;
		const char* input;
		float		inputAsFloat;
		const char* possibleOutput1;
		const char* possibleOutput2;
	} tests[] = {
		{
			"positive_round_up_or_round_down",
			"0x1.3003p8",
			constructNormalizedFloat(8, 0x300300),
			"0x1.304p8",
			"0x1.3p8"
		},
		{
			"negative_round_up_or_round_down",
			"-0x1.6008p-7",
			-constructNormalizedFloat(-7, 0x600800),
			"-0x1.6p-7",
			"-0x1.604p-7"
		},
		{
			"carry_bit",
			"0x1.01ep2",
			constructNormalizedFloat(2, 0x01e000),
			"0x1.01cp2",
			"0x1.02p2"
		},
		{
			"carry_to_exponent",
			"0x1.ffep1",
			constructNormalizedFloat(1, 0xffe000),
			"0x1.ffcp1",
			"0x1.0p2"
		},
	};
	StringTemplate constants (
		"%input_const = OpConstant %f32 ${input}\n"
		"%possible_solution1 = OpConstant %f32 ${output1}\n"
		"%possible_solution2 = OpConstant %f32 ${output2}\n"
		);

	StringTemplate specConstants (
		"%input_const = OpSpecConstant %f32 0.\n"
		"%possible_solution1 = OpConstant %f32 ${output1}\n"
		"%possible_solution2 = OpConstant %f32 ${output2}\n"
	);

	const char* specDecorations = "OpDecorate %input_const  SpecId 0\n";

	const char* function  =
		"%test_code     = OpFunction %v4f32 None %v4f32_function\n"
		"%param1        = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%a             = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		// For the purposes of this test we assume that 0.f will always get
		// faithfully passed through the pipeline stages.
		"%b             = OpFAdd %f32 %input_const %a\n"
		"%c             = OpQuantizeToF16 %f32 %b\n"
		"%eq_1          = OpFOrdEqual %bool %c %possible_solution1\n"
		"%eq_2          = OpFOrdEqual %bool %c %possible_solution2\n"
		"%cond          = OpLogicalOr %bool %eq_1 %eq_2\n"
		"%v4cond        = OpCompositeConstruct %v4bool %cond %cond %cond %cond\n"
		"%retval        = OpSelect %v4f32 %v4cond %c_v4f32_1_0_0_1 %param1"
		"                 OpReturnValue %retval\n"
		"OpFunctionEnd\n";

	for(size_t idx = 0; idx < (sizeof(tests)/sizeof(tests[0])); ++idx) {
		map<string, string>									fragments;
		map<string, string>									constantSpecialization;

		constantSpecialization["input"]						= tests[idx].input;
		constantSpecialization["output1"]					= tests[idx].possibleOutput1;
		constantSpecialization["output2"]					= tests[idx].possibleOutput2;
		fragments["testfun"]								= function;
		fragments["pre_main"]								= constants.specialize(constantSpecialization);
		createTestsForAllStages(tests[idx].name, inputColors, expectedColors, fragments, testCtx);
	}

	for(size_t idx = 0; idx < (sizeof(tests)/sizeof(tests[0])); ++idx) {
		map<string, string>									fragments;
		map<string, string>									constantSpecialization;
		vector<deInt32>										passConstants;
		deInt32												specConstant;

		constantSpecialization["output1"]					= tests[idx].possibleOutput1;
		constantSpecialization["output2"]					= tests[idx].possibleOutput2;
		fragments["testfun"]								= function;
		fragments["decoration"]								= specDecorations;
		fragments["pre_main"]								= specConstants.specialize(constantSpecialization);

		memcpy(&specConstant, &tests[idx].inputAsFloat, sizeof(float));
		passConstants.push_back(specConstant);

		createTestsForAllStages(string("spec_const_") + tests[idx].name, inputColors, expectedColors, fragments, passConstants, testCtx);
	}
}

tcu::TestCaseGroup* createOpQuantizeTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> opQuantizeTests (new tcu::TestCaseGroup(testCtx, "opquantize", "Test OpQuantizeToF16"));
	createOpQuantizeSingleOptionTests(opQuantizeTests.get());
	createOpQuantizeTwoPossibilityTests(opQuantizeTests.get());
	return opQuantizeTests.release();
}

struct ShaderPermutation
{
	deUint8 vertexPermutation;
	deUint8 geometryPermutation;
	deUint8 tesscPermutation;
	deUint8 tessePermutation;
	deUint8 fragmentPermutation;
};

ShaderPermutation getShaderPermutation(deUint8 inputValue)
{
	ShaderPermutation	permutation =
	{
		static_cast<deUint8>(inputValue & 0x10? 1u: 0u),
		static_cast<deUint8>(inputValue & 0x08? 1u: 0u),
		static_cast<deUint8>(inputValue & 0x04? 1u: 0u),
		static_cast<deUint8>(inputValue & 0x02? 1u: 0u),
		static_cast<deUint8>(inputValue & 0x01? 1u: 0u)
	};
	return permutation;
}

tcu::TestCaseGroup* createModuleTests(tcu::TestContext& testCtx)
{
	RGBA								defaultColors[4];
	RGBA								invertedColors[4];
	de::MovePtr<tcu::TestCaseGroup>		moduleTests			(new tcu::TestCaseGroup(testCtx, "module", "Multiple entry points into shaders"));

	const ShaderElement					combinedPipeline[]	=
	{
		ShaderElement("module", "main", VK_SHADER_STAGE_VERTEX_BIT),
		ShaderElement("module", "main", VK_SHADER_STAGE_GEOMETRY_BIT),
		ShaderElement("module", "main", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT),
		ShaderElement("module", "main", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT),
		ShaderElement("module", "main", VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	getDefaultColors(defaultColors);
	getInvertedDefaultColors(invertedColors);
	addFunctionCaseWithPrograms<InstanceContext>(
			moduleTests.get(), "same_module", "", createCombinedModule, runAndVerifyDefaultPipeline,
			createInstanceContext(combinedPipeline, map<string, string>()));

	const char* numbers[] =
	{
		"1", "2"
	};

	for (deInt8 idx = 0; idx < 32; ++idx)
	{
		ShaderPermutation			permutation		= getShaderPermutation(idx);
		string						name			= string("vert") + numbers[permutation.vertexPermutation] + "_geom" + numbers[permutation.geometryPermutation] + "_tessc" + numbers[permutation.tesscPermutation] + "_tesse" + numbers[permutation.tessePermutation] + "_frag" + numbers[permutation.fragmentPermutation];
		const ShaderElement			pipeline[]		=
		{
			ShaderElement("vert",	string("vert") +	numbers[permutation.vertexPermutation],		VK_SHADER_STAGE_VERTEX_BIT),
			ShaderElement("geom",	string("geom") +	numbers[permutation.geometryPermutation],	VK_SHADER_STAGE_GEOMETRY_BIT),
			ShaderElement("tessc",	string("tessc") +	numbers[permutation.tesscPermutation],		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT),
			ShaderElement("tesse",	string("tesse") +	numbers[permutation.tessePermutation],		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT),
			ShaderElement("frag",	string("frag") +	numbers[permutation.fragmentPermutation],	VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// If there are an even number of swaps, then it should be no-op.
		// If there are an odd number, the color should be flipped.
		if ((permutation.vertexPermutation + permutation.geometryPermutation + permutation.tesscPermutation + permutation.tessePermutation + permutation.fragmentPermutation) % 2 == 0)
		{
			addFunctionCaseWithPrograms<InstanceContext>(
					moduleTests.get(), name, "", createMultipleEntries, runAndVerifyDefaultPipeline,
					createInstanceContext(pipeline, defaultColors, defaultColors, map<string, string>()));
		}
		else
		{
			addFunctionCaseWithPrograms<InstanceContext>(
					moduleTests.get(), name, "", createMultipleEntries, runAndVerifyDefaultPipeline,
					createInstanceContext(pipeline, defaultColors, invertedColors, map<string, string>()));
		}
	}
	return moduleTests.release();
}

tcu::TestCaseGroup* createLoopTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "loop", "Looping control flow"));
	RGBA defaultColors[4];
	getDefaultColors(defaultColors);
	map<string, string> fragments;
	fragments["pre_main"] =
		"%c_f32_5 = OpConstant %f32 5.\n";

	// A loop with a single block. The Continue Target is the loop block
	// itself. In SPIR-V terms, the "loop construct" contains no blocks at all
	// -- the "continue construct" forms the entire loop.
	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"

		"%entry = OpLabel\n"
		"%val0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"OpBranch %loop\n"

		";adds and subtracts 1.0 to %val in alternate iterations\n"
		"%loop = OpLabel\n"
		"%count = OpPhi %i32 %c_i32_4 %entry %count__ %loop\n"
		"%delta = OpPhi %f32 %c_f32_1 %entry %minus_delta %loop\n"
		"%val1 = OpPhi %f32 %val0 %entry %val %loop\n"
		"%val = OpFAdd %f32 %val1 %delta\n"
		"%minus_delta = OpFSub %f32 %c_f32_0 %delta\n"
		"%count__ = OpISub %i32 %count %c_i32_1\n"
		"%again = OpSGreaterThan %bool %count__ %c_i32_0\n"
		"OpLoopMerge %exit %loop None\n"
		"OpBranchConditional %again %loop %exit\n"

		"%exit = OpLabel\n"
		"%result = OpVectorInsertDynamic %v4f32 %param1 %val %c_i32_0\n"
		"OpReturnValue %result\n"

		"OpFunctionEnd\n";

	createTestsForAllStages("single_block", defaultColors, defaultColors, fragments, testGroup.get());

	// Body comprised of multiple basic blocks.
	const StringTemplate multiBlock(
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"

		"%entry = OpLabel\n"
		"%val0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"OpBranch %loop\n"

		";adds and subtracts 1.0 to %val in alternate iterations\n"
		"%loop = OpLabel\n"
		"%count = OpPhi %i32 %c_i32_4 %entry %count__ %gather\n"
		"%delta = OpPhi %f32 %c_f32_1 %entry %delta_next %gather\n"
		"%val1 = OpPhi %f32 %val0 %entry %val %gather\n"
		// There are several possibilities for the Continue Target below.  Each
		// will be specialized into a separate test case.
		"OpLoopMerge %exit ${continue_target} None\n"
		"OpBranch %if\n"

		"%if = OpLabel\n"
		";delta_next = (delta > 0) ? -1 : 1;\n"
		"%gt0 = OpFOrdGreaterThan %bool %delta %c_f32_0\n"
		"OpSelectionMerge %gather DontFlatten\n"
		"OpBranchConditional %gt0 %even %odd ;tells us if %count is even or odd\n"

		"%odd = OpLabel\n"
		"OpBranch %gather\n"

		"%even = OpLabel\n"
		"OpBranch %gather\n"

		"%gather = OpLabel\n"
		"%delta_next = OpPhi %f32 %c_f32_n1 %even %c_f32_1 %odd\n"
		"%val = OpFAdd %f32 %val1 %delta\n"
		"%count__ = OpISub %i32 %count %c_i32_1\n"
		"%again = OpSGreaterThan %bool %count__ %c_i32_0\n"
		"OpBranchConditional %again %loop %exit\n"

		"%exit = OpLabel\n"
		"%result = OpVectorInsertDynamic %v4f32 %param1 %val %c_i32_0\n"
		"OpReturnValue %result\n"

		"OpFunctionEnd\n");

	map<string, string> continue_target;

	// The Continue Target is the loop block itself.
	continue_target["continue_target"] = "%loop";
	fragments["testfun"] = multiBlock.specialize(continue_target);
	createTestsForAllStages("multi_block_continue_construct", defaultColors, defaultColors, fragments, testGroup.get());

	// The Continue Target is at the end of the loop.
	continue_target["continue_target"] = "%gather";
	fragments["testfun"] = multiBlock.specialize(continue_target);
	createTestsForAllStages("multi_block_loop_construct", defaultColors, defaultColors, fragments, testGroup.get());

	// A loop with continue statement.
	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"

		"%entry = OpLabel\n"
		"%val0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"OpBranch %loop\n"

		";adds 4, 3, and 1 to %val0 (skips 2)\n"
		"%loop = OpLabel\n"
		"%count = OpPhi %i32 %c_i32_4 %entry %count__ %continue\n"
		"%val1 = OpPhi %f32 %val0 %entry %val %continue\n"
		"OpLoopMerge %exit %continue None\n"
		"OpBranch %if\n"

		"%if = OpLabel\n"
		";skip if %count==2\n"
		"%eq2 = OpIEqual %bool %count %c_i32_2\n"
		"OpSelectionMerge %continue DontFlatten\n"
		"OpBranchConditional %eq2 %continue %body\n"

		"%body = OpLabel\n"
		"%fcount = OpConvertSToF %f32 %count\n"
		"%val2 = OpFAdd %f32 %val1 %fcount\n"
		"OpBranch %continue\n"

		"%continue = OpLabel\n"
		"%val = OpPhi %f32 %val2 %body %val1 %if\n"
		"%count__ = OpISub %i32 %count %c_i32_1\n"
		"%again = OpSGreaterThan %bool %count__ %c_i32_0\n"
		"OpBranchConditional %again %loop %exit\n"

		"%exit = OpLabel\n"
		"%same = OpFSub %f32 %val %c_f32_8\n"
		"%result = OpVectorInsertDynamic %v4f32 %param1 %same %c_i32_0\n"
		"OpReturnValue %result\n"
		"OpFunctionEnd\n";
	createTestsForAllStages("continue", defaultColors, defaultColors, fragments, testGroup.get());

	// A loop with break.
	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"

		"%entry = OpLabel\n"
		";param1 components are between 0 and 1, so dot product is 4 or less\n"
		"%dot = OpDot %f32 %param1 %param1\n"
		"%div = OpFDiv %f32 %dot %c_f32_5\n"
		"%zero = OpConvertFToU %u32 %div\n"
		"%two = OpIAdd %i32 %zero %c_i32_2\n"
		"%val0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"OpBranch %loop\n"

		";adds 4 and 3 to %val0 (exits early)\n"
		"%loop = OpLabel\n"
		"%count = OpPhi %i32 %c_i32_4 %entry %count__ %continue\n"
		"%val1 = OpPhi %f32 %val0 %entry %val2 %continue\n"
		"OpLoopMerge %exit %continue None\n"
		"OpBranch %if\n"

		"%if = OpLabel\n"
		";end loop if %count==%two\n"
		"%above2 = OpSGreaterThan %bool %count %two\n"
		"OpSelectionMerge %continue DontFlatten\n"
		"OpBranchConditional %above2 %body %exit\n"

		"%body = OpLabel\n"
		"%fcount = OpConvertSToF %f32 %count\n"
		"%val2 = OpFAdd %f32 %val1 %fcount\n"
		"OpBranch %continue\n"

		"%continue = OpLabel\n"
		"%count__ = OpISub %i32 %count %c_i32_1\n"
		"%again = OpSGreaterThan %bool %count__ %c_i32_0\n"
		"OpBranchConditional %again %loop %exit\n"

		"%exit = OpLabel\n"
		"%val_post = OpPhi %f32 %val2 %continue %val1 %if\n"
		"%same = OpFSub %f32 %val_post %c_f32_7\n"
		"%result = OpVectorInsertDynamic %v4f32 %param1 %same %c_i32_0\n"
		"OpReturnValue %result\n"
		"OpFunctionEnd\n";
	createTestsForAllStages("break", defaultColors, defaultColors, fragments, testGroup.get());

	// A loop with return.
	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"

		"%entry = OpLabel\n"
		";param1 components are between 0 and 1, so dot product is 4 or less\n"
		"%dot = OpDot %f32 %param1 %param1\n"
		"%div = OpFDiv %f32 %dot %c_f32_5\n"
		"%zero = OpConvertFToU %u32 %div\n"
		"%two = OpIAdd %i32 %zero %c_i32_2\n"
		"%val0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"OpBranch %loop\n"

		";returns early without modifying %param1\n"
		"%loop = OpLabel\n"
		"%count = OpPhi %i32 %c_i32_4 %entry %count__ %continue\n"
		"%val1 = OpPhi %f32 %val0 %entry %val2 %continue\n"
		"OpLoopMerge %exit %continue None\n"
		"OpBranch %if\n"

		"%if = OpLabel\n"
		";return if %count==%two\n"
		"%above2 = OpSGreaterThan %bool %count %two\n"
		"OpSelectionMerge %continue DontFlatten\n"
		"OpBranchConditional %above2 %body %early_exit\n"

		"%early_exit = OpLabel\n"
		"OpReturnValue %param1\n"

		"%body = OpLabel\n"
		"%fcount = OpConvertSToF %f32 %count\n"
		"%val2 = OpFAdd %f32 %val1 %fcount\n"
		"OpBranch %continue\n"

		"%continue = OpLabel\n"
		"%count__ = OpISub %i32 %count %c_i32_1\n"
		"%again = OpSGreaterThan %bool %count__ %c_i32_0\n"
		"OpBranchConditional %again %loop %exit\n"

		"%exit = OpLabel\n"
		";should never get here, so return an incorrect result\n"
		"%result = OpVectorInsertDynamic %v4f32 %param1 %val2 %c_i32_0\n"
		"OpReturnValue %result\n"
		"OpFunctionEnd\n";
	createTestsForAllStages("return", defaultColors, defaultColors, fragments, testGroup.get());

	return testGroup.release();
}

// A collection of tests putting OpControlBarrier in places GLSL forbids but SPIR-V allows.
tcu::TestCaseGroup* createBarrierTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> testGroup(new tcu::TestCaseGroup(testCtx, "barrier", "OpControlBarrier"));
	map<string, string> fragments;

	// A barrier inside a function body.
	fragments["pre_main"] =
		"%Workgroup = OpConstant %i32 2\n"
		"%SequentiallyConsistent = OpConstant %i32 0x10\n";
	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"OpControlBarrier %Workgroup %Workgroup %SequentiallyConsistent\n"
		"OpReturnValue %param1\n"
		"OpFunctionEnd\n";
	addTessCtrlTest(testGroup.get(), "in_function", fragments);

	// Common setup code for the following tests.
	fragments["pre_main"] =
		"%Workgroup = OpConstant %i32 2\n"
		"%SequentiallyConsistent = OpConstant %i32 0x10\n"
		"%c_f32_5 = OpConstant %f32 5.\n";
	const string setupPercentZero =	 // Begins %test_code function with code that sets %zero to 0u but cannot be optimized away.
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%entry = OpLabel\n"
		";param1 components are between 0 and 1, so dot product is 4 or less\n"
		"%dot = OpDot %f32 %param1 %param1\n"
		"%div = OpFDiv %f32 %dot %c_f32_5\n"
		"%zero = OpConvertFToU %u32 %div\n";

	// Barriers inside OpSwitch branches.
	fragments["testfun"] =
		setupPercentZero +
		"OpSelectionMerge %switch_exit None\n"
		"OpSwitch %zero %switch_default 0 %case0 1 %case1 ;should always go to %case0\n"

		"%case1 = OpLabel\n"
		";This barrier should never be executed, but its presence makes test failure more likely when there's a bug.\n"
		"OpControlBarrier %Workgroup %Workgroup %SequentiallyConsistent\n"
		"%wrong_branch_alert1 = OpVectorInsertDynamic %v4f32 %param1 %c_f32_0_5 %c_i32_0\n"
		"OpBranch %switch_exit\n"

		"%switch_default = OpLabel\n"
		"%wrong_branch_alert2 = OpVectorInsertDynamic %v4f32 %param1 %c_f32_0_5 %c_i32_0\n"
		";This barrier should never be executed, but its presence makes test failure more likely when there's a bug.\n"
		"OpControlBarrier %Workgroup %Workgroup %SequentiallyConsistent\n"
		"OpBranch %switch_exit\n"

		"%case0 = OpLabel\n"
		"OpControlBarrier %Workgroup %Workgroup %SequentiallyConsistent\n"
		"OpBranch %switch_exit\n"

		"%switch_exit = OpLabel\n"
		"%ret = OpPhi %v4f32 %param1 %case0 %wrong_branch_alert1 %case1 %wrong_branch_alert2 %switch_default\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";
	addTessCtrlTest(testGroup.get(), "in_switch", fragments);

	// Barriers inside if-then-else.
	fragments["testfun"] =
		setupPercentZero +
		"%eq0 = OpIEqual %bool %zero %c_u32_0\n"
		"OpSelectionMerge %exit DontFlatten\n"
		"OpBranchConditional %eq0 %then %else\n"

		"%else = OpLabel\n"
		";This barrier should never be executed, but its presence makes test failure more likely when there's a bug.\n"
		"OpControlBarrier %Workgroup %Workgroup %SequentiallyConsistent\n"
		"%wrong_branch_alert = OpVectorInsertDynamic %v4f32 %param1 %c_f32_0_5 %c_i32_0\n"
		"OpBranch %exit\n"

		"%then = OpLabel\n"
		"OpControlBarrier %Workgroup %Workgroup %SequentiallyConsistent\n"
		"OpBranch %exit\n"

		"%exit = OpLabel\n"
		"%ret = OpPhi %v4f32 %param1 %then %wrong_branch_alert %else\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";
	addTessCtrlTest(testGroup.get(), "in_if", fragments);

	// A barrier after control-flow reconvergence, tempting the compiler to attempt something like this:
	// http://lists.llvm.org/pipermail/llvm-dev/2009-October/026317.html.
	fragments["testfun"] =
		setupPercentZero +
		"%thread_id = OpLoad %i32 %BP_gl_InvocationID\n"
		"%thread0 = OpIEqual %bool %thread_id %c_i32_0\n"
		"OpSelectionMerge %exit DontFlatten\n"
		"OpBranchConditional %thread0 %then %else\n"

		"%else = OpLabel\n"
		"%val0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"OpBranch %exit\n"

		"%then = OpLabel\n"
		"%val1 = OpVectorExtractDynamic %f32 %param1 %zero\n"
		"OpBranch %exit\n"

		"%exit = OpLabel\n"
		"%val = OpPhi %f32 %val0 %else %val1 %then\n"
		"OpControlBarrier %Workgroup %Workgroup %SequentiallyConsistent\n"
		"%ret = OpVectorInsertDynamic %v4f32 %param1 %val %zero\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";
	addTessCtrlTest(testGroup.get(), "after_divergent_if", fragments);

	// A barrier inside a loop.
	fragments["pre_main"] =
		"%Workgroup = OpConstant %i32 2\n"
		"%SequentiallyConsistent = OpConstant %i32 0x10\n"
		"%c_f32_10 = OpConstant %f32 10.\n";
	fragments["testfun"] =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%entry = OpLabel\n"
		"%val0 = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"OpBranch %loop\n"

		";adds 4, 3, 2, and 1 to %val0\n"
		"%loop = OpLabel\n"
		"%count = OpPhi %i32 %c_i32_4 %entry %count__ %loop\n"
		"%val1 = OpPhi %f32 %val0 %entry %val %loop\n"
		"OpControlBarrier %Workgroup %Workgroup %SequentiallyConsistent\n"
		"%fcount = OpConvertSToF %f32 %count\n"
		"%val = OpFAdd %f32 %val1 %fcount\n"
		"%count__ = OpISub %i32 %count %c_i32_1\n"
		"%again = OpSGreaterThan %bool %count__ %c_i32_0\n"
		"OpLoopMerge %exit %loop None\n"
		"OpBranchConditional %again %loop %exit\n"

		"%exit = OpLabel\n"
		"%same = OpFSub %f32 %val %c_f32_10\n"
		"%ret = OpVectorInsertDynamic %v4f32 %param1 %same %c_i32_0\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";
	addTessCtrlTest(testGroup.get(), "in_loop", fragments);

	return testGroup.release();
}

// Test for the OpFRem instruction.
tcu::TestCaseGroup* createFRemTests(tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>		testGroup(new tcu::TestCaseGroup(testCtx, "frem", "OpFRem"));
	map<string, string>					fragments;
	RGBA								inputColors[4];
	RGBA								outputColors[4];

	fragments["pre_main"]				 =
		"%c_f32_3 = OpConstant %f32 3.0\n"
		"%c_f32_n3 = OpConstant %f32 -3.0\n"
		"%c_f32_4 = OpConstant %f32 4.0\n"
		"%c_f32_p75 = OpConstant %f32 0.75\n"
		"%c_v4f32_p75_p75_p75_p75 = OpConstantComposite %v4f32 %c_f32_p75 %c_f32_p75 %c_f32_p75 %c_f32_p75 \n"
		"%c_v4f32_4_4_4_4 = OpConstantComposite %v4f32 %c_f32_4 %c_f32_4 %c_f32_4 %c_f32_4\n"
		"%c_v4f32_3_n3_3_n3 = OpConstantComposite %v4f32 %c_f32_3 %c_f32_n3 %c_f32_3 %c_f32_n3\n";

	// The test does the following.
	// vec4 result = (param1 * 8.0) - 4.0;
	// return (frem(result.x,3) + 0.75, frem(result.y, -3) + 0.75, 0, 1)
	fragments["testfun"]				 =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%v_times_8 = OpVectorTimesScalar %v4f32 %param1 %c_f32_8\n"
		"%minus_4 = OpFSub %v4f32 %v_times_8 %c_v4f32_4_4_4_4\n"
		"%frem = OpFRem %v4f32 %minus_4 %c_v4f32_3_n3_3_n3\n"
		"%added = OpFAdd %v4f32 %frem %c_v4f32_p75_p75_p75_p75\n"
		"%xyz_1 = OpVectorInsertDynamic %v4f32 %added %c_f32_1 %c_i32_3\n"
		"%xy_0_1 = OpVectorInsertDynamic %v4f32 %xyz_1 %c_f32_0 %c_i32_2\n"
		"OpReturnValue %xy_0_1\n"
		"OpFunctionEnd\n";


	inputColors[0]		= RGBA(16,	16,		0, 255);
	inputColors[1]		= RGBA(232, 232,	0, 255);
	inputColors[2]		= RGBA(232, 16,		0, 255);
	inputColors[3]		= RGBA(16,	232,	0, 255);

	outputColors[0]		= RGBA(64,	64,		0, 255);
	outputColors[1]		= RGBA(255, 255,	0, 255);
	outputColors[2]		= RGBA(255, 64,		0, 255);
	outputColors[3]		= RGBA(64,	255,	0, 255);

	createTestsForAllStages("frem", inputColors, outputColors, fragments, testGroup.get());
	return testGroup.release();
}

// Test for the OpSRem instruction.
tcu::TestCaseGroup* createOpSRemGraphicsTests(tcu::TestContext& testCtx, qpTestResult negFailResult)
{
	de::MovePtr<tcu::TestCaseGroup>		testGroup(new tcu::TestCaseGroup(testCtx, "srem", "OpSRem"));
	map<string, string>					fragments;

	fragments["pre_main"]				 =
		"%c_f32_255 = OpConstant %f32 255.0\n"
		"%c_i32_128 = OpConstant %i32 128\n"
		"%c_i32_255 = OpConstant %i32 255\n"
		"%c_v4f32_255 = OpConstantComposite %v4f32 %c_f32_255 %c_f32_255 %c_f32_255 %c_f32_255 \n"
		"%c_v4f32_0_5 = OpConstantComposite %v4f32 %c_f32_0_5 %c_f32_0_5 %c_f32_0_5 %c_f32_0_5 \n"
		"%c_v4i32_128 = OpConstantComposite %v4i32 %c_i32_128 %c_i32_128 %c_i32_128 %c_i32_128 \n";

	// The test does the following.
	// ivec4 ints = int(param1 * 255.0 + 0.5) - 128;
	// ivec4 result = ivec4(srem(ints.x, ints.y), srem(ints.y, ints.z), srem(ints.z, ints.x), 255);
	// return float(result + 128) / 255.0;
	fragments["testfun"]				 =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%div255 = OpFMul %v4f32 %param1 %c_v4f32_255\n"
		"%add0_5 = OpFAdd %v4f32 %div255 %c_v4f32_0_5\n"
		"%uints_in = OpConvertFToS %v4i32 %add0_5\n"
		"%ints_in = OpISub %v4i32 %uints_in %c_v4i32_128\n"
		"%x_in = OpCompositeExtract %i32 %ints_in 0\n"
		"%y_in = OpCompositeExtract %i32 %ints_in 1\n"
		"%z_in = OpCompositeExtract %i32 %ints_in 2\n"
		"%x_out = OpSRem %i32 %x_in %y_in\n"
		"%y_out = OpSRem %i32 %y_in %z_in\n"
		"%z_out = OpSRem %i32 %z_in %x_in\n"
		"%ints_out = OpCompositeConstruct %v4i32 %x_out %y_out %z_out %c_i32_255\n"
		"%ints_offset = OpIAdd %v4i32 %ints_out %c_v4i32_128\n"
		"%f_ints_offset = OpConvertSToF %v4f32 %ints_offset\n"
		"%float_out = OpFDiv %v4f32 %f_ints_offset %c_v4f32_255\n"
		"OpReturnValue %float_out\n"
		"OpFunctionEnd\n";

	const struct CaseParams
	{
		const char*		name;
		const char*		failMessageTemplate;	// customized status message
		qpTestResult	failResult;				// override status on failure
		int				operands[4][3];			// four (x, y, z) vectors of operands
		int				results[4][3];			// four (x, y, z) vectors of results
	} cases[] =
	{
		{
			"positive",
			"${reason}",
			QP_TEST_RESULT_FAIL,
			{ { 5, 12, 17 }, { 5, 5, 7 }, { 75, 8, 81 }, { 25, 60, 100 } },			// operands
			{ { 5, 12,  2 }, { 0, 5, 2 }, {  3, 8,  6 }, { 25, 60,   0 } },			// results
		},
		{
			"all",
			"Inconsistent results, but within specification: ${reason}",
			negFailResult,															// negative operands, not required by the spec
			{ { 5, 12, -17 }, { -5, -5, 7 }, { 75, 8, -81 }, { 25, -60, 100 } },	// operands
			{ { 5, 12,  -2 }, {  0, -5, 2 }, {  3, 8,  -6 }, { 25, -60,   0 } },	// results
		},
	};
	// If either operand is negative the result is undefined. Some implementations may still return correct values.

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); ++caseNdx)
	{
		const CaseParams&	params			= cases[caseNdx];
		RGBA				inputColors[4];
		RGBA				outputColors[4];

		for (int i = 0; i < 4; ++i)
		{
			inputColors [i] = RGBA(params.operands[i][0] + 128, params.operands[i][1] + 128, params.operands[i][2] + 128, 255);
			outputColors[i] = RGBA(params.results [i][0] + 128, params.results [i][1] + 128, params.results [i][2] + 128, 255);
		}

		createTestsForAllStages(params.name, inputColors, outputColors, fragments, testGroup.get(), params.failResult, params.failMessageTemplate);
	}

	return testGroup.release();
}

// Test for the OpSMod instruction.
tcu::TestCaseGroup* createOpSModGraphicsTests(tcu::TestContext& testCtx, qpTestResult negFailResult)
{
	de::MovePtr<tcu::TestCaseGroup>		testGroup(new tcu::TestCaseGroup(testCtx, "smod", "OpSMod"));
	map<string, string>					fragments;

	fragments["pre_main"]				 =
		"%c_f32_255 = OpConstant %f32 255.0\n"
		"%c_i32_128 = OpConstant %i32 128\n"
		"%c_i32_255 = OpConstant %i32 255\n"
		"%c_v4f32_255 = OpConstantComposite %v4f32 %c_f32_255 %c_f32_255 %c_f32_255 %c_f32_255 \n"
		"%c_v4f32_0_5 = OpConstantComposite %v4f32 %c_f32_0_5 %c_f32_0_5 %c_f32_0_5 %c_f32_0_5 \n"
		"%c_v4i32_128 = OpConstantComposite %v4i32 %c_i32_128 %c_i32_128 %c_i32_128 %c_i32_128 \n";

	// The test does the following.
	// ivec4 ints = int(param1 * 255.0 + 0.5) - 128;
	// ivec4 result = ivec4(smod(ints.x, ints.y), smod(ints.y, ints.z), smod(ints.z, ints.x), 255);
	// return float(result + 128) / 255.0;
	fragments["testfun"]				 =
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"%div255 = OpFMul %v4f32 %param1 %c_v4f32_255\n"
		"%add0_5 = OpFAdd %v4f32 %div255 %c_v4f32_0_5\n"
		"%uints_in = OpConvertFToS %v4i32 %add0_5\n"
		"%ints_in = OpISub %v4i32 %uints_in %c_v4i32_128\n"
		"%x_in = OpCompositeExtract %i32 %ints_in 0\n"
		"%y_in = OpCompositeExtract %i32 %ints_in 1\n"
		"%z_in = OpCompositeExtract %i32 %ints_in 2\n"
		"%x_out = OpSMod %i32 %x_in %y_in\n"
		"%y_out = OpSMod %i32 %y_in %z_in\n"
		"%z_out = OpSMod %i32 %z_in %x_in\n"
		"%ints_out = OpCompositeConstruct %v4i32 %x_out %y_out %z_out %c_i32_255\n"
		"%ints_offset = OpIAdd %v4i32 %ints_out %c_v4i32_128\n"
		"%f_ints_offset = OpConvertSToF %v4f32 %ints_offset\n"
		"%float_out = OpFDiv %v4f32 %f_ints_offset %c_v4f32_255\n"
		"OpReturnValue %float_out\n"
		"OpFunctionEnd\n";

	const struct CaseParams
	{
		const char*		name;
		const char*		failMessageTemplate;	// customized status message
		qpTestResult	failResult;				// override status on failure
		int				operands[4][3];			// four (x, y, z) vectors of operands
		int				results[4][3];			// four (x, y, z) vectors of results
	} cases[] =
	{
		{
			"positive",
			"${reason}",
			QP_TEST_RESULT_FAIL,
			{ { 5, 12, 17 }, { 5, 5, 7 }, { 75, 8, 81 }, { 25, 60, 100 } },				// operands
			{ { 5, 12,  2 }, { 0, 5, 2 }, {  3, 8,  6 }, { 25, 60,   0 } },				// results
		},
		{
			"all",
			"Inconsistent results, but within specification: ${reason}",
			negFailResult,																// negative operands, not required by the spec
			{ { 5, 12, -17 }, { -5, -5,  7 }, { 75,   8, -81 }, {  25, -60, 100 } },	// operands
			{ { 5, -5,   3 }, {  0,  2, -3 }, {  3, -73,  69 }, { -35,  40,   0 } },	// results
		},
	};
	// If either operand is negative the result is undefined. Some implementations may still return correct values.

	for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); ++caseNdx)
	{
		const CaseParams&	params			= cases[caseNdx];
		RGBA				inputColors[4];
		RGBA				outputColors[4];

		for (int i = 0; i < 4; ++i)
		{
			inputColors [i] = RGBA(params.operands[i][0] + 128, params.operands[i][1] + 128, params.operands[i][2] + 128, 255);
			outputColors[i] = RGBA(params.results [i][0] + 128, params.results [i][1] + 128, params.results [i][2] + 128, 255);
		}

		createTestsForAllStages(params.name, inputColors, outputColors, fragments, testGroup.get(), params.failResult, params.failMessageTemplate);
	}

	return testGroup.release();
}

enum IntegerType
{
	INTEGER_TYPE_SIGNED_16,
	INTEGER_TYPE_SIGNED_32,
	INTEGER_TYPE_SIGNED_64,

	INTEGER_TYPE_UNSIGNED_16,
	INTEGER_TYPE_UNSIGNED_32,
	INTEGER_TYPE_UNSIGNED_64,
};

const string getBitWidthStr (IntegerType type)
{
	switch (type)
	{
		case INTEGER_TYPE_SIGNED_16:
		case INTEGER_TYPE_UNSIGNED_16:	return "16";

		case INTEGER_TYPE_SIGNED_32:
		case INTEGER_TYPE_UNSIGNED_32:	return "32";

		case INTEGER_TYPE_SIGNED_64:
		case INTEGER_TYPE_UNSIGNED_64:	return "64";

		default:						DE_ASSERT(false);
										return "";
	}
}

const string getByteWidthStr (IntegerType type)
{
	switch (type)
	{
		case INTEGER_TYPE_SIGNED_16:
		case INTEGER_TYPE_UNSIGNED_16:	return "2";

		case INTEGER_TYPE_SIGNED_32:
		case INTEGER_TYPE_UNSIGNED_32:	return "4";

		case INTEGER_TYPE_SIGNED_64:
		case INTEGER_TYPE_UNSIGNED_64:	return "8";

		default:						DE_ASSERT(false);
										return "";
	}
}

bool isSigned (IntegerType type)
{
	return (type <= INTEGER_TYPE_SIGNED_64);
}

const string getTypeName (IntegerType type)
{
	string prefix = isSigned(type) ? "" : "u";
	return prefix + "int" + getBitWidthStr(type);
}

const string getTestName (IntegerType from, IntegerType to)
{
	return getTypeName(from) + "_to_" + getTypeName(to);
}

const string getAsmTypeDeclaration (IntegerType type)
{
	string sign = isSigned(type) ? " 1" : " 0";
	return "OpTypeInt " + getBitWidthStr(type) + sign;
}

const string getAsmTypeName (IntegerType type)
{
	const string prefix = isSigned(type) ? "%i" : "%u";
	return prefix + getBitWidthStr(type);
}

template<typename T>
BufferSp getSpecializedBuffer (deInt64 number)
{
	return BufferSp(new Buffer<T>(vector<T>(1, (T)number)));
}

BufferSp getBuffer (IntegerType type, deInt64 number)
{
	switch (type)
	{
		case INTEGER_TYPE_SIGNED_16:	return getSpecializedBuffer<deInt16>(number);
		case INTEGER_TYPE_SIGNED_32:	return getSpecializedBuffer<deInt32>(number);
		case INTEGER_TYPE_SIGNED_64:	return getSpecializedBuffer<deInt64>(number);

		case INTEGER_TYPE_UNSIGNED_16:	return getSpecializedBuffer<deUint16>(number);
		case INTEGER_TYPE_UNSIGNED_32:	return getSpecializedBuffer<deUint32>(number);
		case INTEGER_TYPE_UNSIGNED_64:	return getSpecializedBuffer<deUint64>(number);

		default:						DE_ASSERT(false);
										return BufferSp(new Buffer<deInt32>(vector<deInt32>(1, 0)));
	}
}

bool usesInt16 (IntegerType from, IntegerType to)
{
	return (from == INTEGER_TYPE_SIGNED_16 || from == INTEGER_TYPE_UNSIGNED_16
			|| to == INTEGER_TYPE_SIGNED_16 || to == INTEGER_TYPE_UNSIGNED_16);
}

bool usesInt64 (IntegerType from, IntegerType to)
{
	return (from == INTEGER_TYPE_SIGNED_64 || from == INTEGER_TYPE_UNSIGNED_64
			|| to == INTEGER_TYPE_SIGNED_64 || to == INTEGER_TYPE_UNSIGNED_64);
}

ComputeTestFeatures getConversionUsedFeatures (IntegerType from, IntegerType to)
{
	if (usesInt16(from, to))
	{
		if (usesInt64(from, to))
		{
			return COMPUTE_TEST_USES_INT16_INT64;
		}
		else
		{
			return COMPUTE_TEST_USES_INT16;
		}
	}
	else
	{
		return COMPUTE_TEST_USES_INT64;
	}
}

struct ConvertCase
{
	ConvertCase (IntegerType from, IntegerType to, deInt64 number)
	: m_fromType		(from)
	, m_toType			(to)
	, m_features		(getConversionUsedFeatures(from, to))
	, m_name			(getTestName(from, to))
	, m_inputBuffer		(getBuffer(from, number))
	, m_outputBuffer	(getBuffer(to, number))
	{
		m_asmTypes["inputType"]		= getAsmTypeName(from);
		m_asmTypes["outputType"]	= getAsmTypeName(to);

		if (m_features == COMPUTE_TEST_USES_INT16)
		{
			m_asmTypes["int_capabilities"]	  = "OpCapability Int16\n"
												"OpCapability StorageUniformBufferBlock16\n";
			m_asmTypes["int_additional_decl"] = "%i16        = OpTypeInt 16 1\n"
												"%u16        = OpTypeInt 16 0\n";
			m_asmTypes["int_extensions"]	  = "OpExtension \"SPV_KHR_16bit_storage\"\n";
		}
		else if (m_features == COMPUTE_TEST_USES_INT64)
		{
			m_asmTypes["int_capabilities"]	  = "OpCapability Int64\n";
			m_asmTypes["int_additional_decl"] = "%i64        = OpTypeInt 64 1\n"
												"%u64        = OpTypeInt 64 0\n";
			m_asmTypes["int_extensions"]	  = "";
		}
		else if (m_features == COMPUTE_TEST_USES_INT16_INT64)
		{
			m_asmTypes["int_capabilities"]	  = "OpCapability Int16\n"
												"OpCapability StorageUniformBufferBlock16\n"
												"OpCapability Int64\n";
			m_asmTypes["int_additional_decl"] = "%i16        = OpTypeInt 16 1\n"
												"%u16        = OpTypeInt 16 0\n"
												"%i64        = OpTypeInt 64 1\n"
												"%u64        = OpTypeInt 64 0\n";
			m_asmTypes["int_extensions"]	  = "OpExtension \"SPV_KHR_16bit_storage\"\n";
		}
		else
		{
			DE_ASSERT(false);
		}
	}

	IntegerType				m_fromType;
	IntegerType				m_toType;
	ComputeTestFeatures		m_features;
	string					m_name;
	map<string, string>		m_asmTypes;
	BufferSp				m_inputBuffer;
	BufferSp				m_outputBuffer;
};

const string getConvertCaseShaderStr (const string& instruction, const ConvertCase& convertCase)
{
	map<string, string> params = convertCase.m_asmTypes;

	params["instruction"] = instruction;

	params["inDecorator"] = getByteWidthStr(convertCase.m_fromType);
	params["outDecorator"] = getByteWidthStr(convertCase.m_toType);

	const StringTemplate shader (
		"OpCapability Shader\n"
		"${int_capabilities}"
		"${int_extensions}"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"
		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"
		// Decorators
		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %indata DescriptorSet 0\n"
		"OpDecorate %indata Binding 0\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 1\n"
		"OpDecorate %in_arr ArrayStride ${inDecorator}\n"
		"OpDecorate %out_arr ArrayStride ${outDecorator}\n"
		"OpDecorate %in_buf BufferBlock\n"
		"OpDecorate %out_buf BufferBlock\n"
		"OpMemberDecorate %in_buf 0 Offset 0\n"
		"OpMemberDecorate %out_buf 0 Offset 0\n"
		// Base types
		"%void       = OpTypeVoid\n"
		"%voidf      = OpTypeFunction %void\n"
		"%u32        = OpTypeInt 32 0\n"
		"%i32        = OpTypeInt 32 1\n"
		"${int_additional_decl}"
		"%uvec3      = OpTypeVector %u32 3\n"
		"%uvec3ptr   = OpTypePointer Input %uvec3\n"
		// Derived types
		"%in_ptr     = OpTypePointer Uniform ${inputType}\n"
		"%out_ptr    = OpTypePointer Uniform ${outputType}\n"
		"%in_arr     = OpTypeRuntimeArray ${inputType}\n"
		"%out_arr    = OpTypeRuntimeArray ${outputType}\n"
		"%in_buf     = OpTypeStruct %in_arr\n"
		"%out_buf    = OpTypeStruct %out_arr\n"
		"%in_bufptr  = OpTypePointer Uniform %in_buf\n"
		"%out_bufptr = OpTypePointer Uniform %out_buf\n"
		"%indata     = OpVariable %in_bufptr Uniform\n"
		"%outdata    = OpVariable %out_bufptr Uniform\n"
		"%inputptr   = OpTypePointer Input ${inputType}\n"
		"%id         = OpVariable %uvec3ptr Input\n"
		// Constants
		"%zero       = OpConstant %i32 0\n"
		// Main function
		"%main       = OpFunction %void None %voidf\n"
		"%label      = OpLabel\n"
		"%idval      = OpLoad %uvec3 %id\n"
		"%x          = OpCompositeExtract %u32 %idval 0\n"
		"%inloc      = OpAccessChain %in_ptr %indata %zero %x\n"
		"%outloc     = OpAccessChain %out_ptr %outdata %zero %x\n"
		"%inval      = OpLoad ${inputType} %inloc\n"
		"%conv       = ${instruction} ${outputType} %inval\n"
		"              OpStore %outloc %conv\n"
		"              OpReturn\n"
		"              OpFunctionEnd\n"
	);

	return shader.specialize(params);
}

void createSConvertCases (vector<ConvertCase>& testCases)
{
	// Convert int to int
	testCases.push_back(ConvertCase(INTEGER_TYPE_SIGNED_16,	INTEGER_TYPE_SIGNED_32,		14669));
	testCases.push_back(ConvertCase(INTEGER_TYPE_SIGNED_16,	INTEGER_TYPE_SIGNED_64,		3341));

	testCases.push_back(ConvertCase(INTEGER_TYPE_SIGNED_32,	INTEGER_TYPE_SIGNED_64,		973610259));

	// Convert int to unsigned int
	testCases.push_back(ConvertCase(INTEGER_TYPE_SIGNED_16,	INTEGER_TYPE_UNSIGNED_32,	9288));
	testCases.push_back(ConvertCase(INTEGER_TYPE_SIGNED_16,	INTEGER_TYPE_UNSIGNED_64,	15460));

	testCases.push_back(ConvertCase(INTEGER_TYPE_SIGNED_32,	INTEGER_TYPE_UNSIGNED_64,	346213461));
}

//  Test for the OpSConvert instruction.
tcu::TestCaseGroup* createSConvertTests (tcu::TestContext& testCtx)
{
	const string instruction				("OpSConvert");
	de::MovePtr<tcu::TestCaseGroup>	group	(new tcu::TestCaseGroup(testCtx, "sconvert", "OpSConvert"));
	vector<ConvertCase>				testCases;
	createSConvertCases(testCases);

	for (vector<ConvertCase>::const_iterator test = testCases.begin(); test != testCases.end(); ++test)
	{
		ComputeShaderSpec	spec;

		spec.assembly = getConvertCaseShaderStr(instruction, *test);
		spec.inputs.push_back(test->m_inputBuffer);
		spec.outputs.push_back(test->m_outputBuffer);
		spec.numWorkGroups = IVec3(1, 1, 1);

		if (test->m_features == COMPUTE_TEST_USES_INT16 || test->m_features == COMPUTE_TEST_USES_INT16_INT64)
		{
			spec.extensions.push_back("VK_KHR_16bit_storage");
		}

		group->addChild(new SpvAsmComputeShaderCase(testCtx, test->m_name.c_str(), "Convert integers with OpSConvert.", spec, test->m_features));
	}

	return group.release();
}

void createUConvertCases (vector<ConvertCase>& testCases)
{
	// Convert unsigned int to unsigned int
	testCases.push_back(ConvertCase(INTEGER_TYPE_UNSIGNED_16,	INTEGER_TYPE_UNSIGNED_32,	60653));
	testCases.push_back(ConvertCase(INTEGER_TYPE_UNSIGNED_16,	INTEGER_TYPE_UNSIGNED_64,	17991));

	testCases.push_back(ConvertCase(INTEGER_TYPE_UNSIGNED_32,	INTEGER_TYPE_UNSIGNED_64,	904256275));
}

//  Test for the OpUConvert instruction.
tcu::TestCaseGroup* createUConvertTests (tcu::TestContext& testCtx)
{
	const string instruction				("OpUConvert");
	de::MovePtr<tcu::TestCaseGroup>	group	(new tcu::TestCaseGroup(testCtx, "uconvert", "OpUConvert"));
	vector<ConvertCase>				testCases;
	createUConvertCases(testCases);

	for (vector<ConvertCase>::const_iterator test = testCases.begin(); test != testCases.end(); ++test)
	{
		ComputeShaderSpec	spec;

		spec.assembly = getConvertCaseShaderStr(instruction, *test);
		spec.inputs.push_back(test->m_inputBuffer);
		spec.outputs.push_back(test->m_outputBuffer);
		spec.numWorkGroups = IVec3(1, 1, 1);

		if (test->m_features == COMPUTE_TEST_USES_INT16 || test->m_features == COMPUTE_TEST_USES_INT16_INT64)
		{
			spec.extensions.push_back("VK_KHR_16bit_storage");
		}

		group->addChild(new SpvAsmComputeShaderCase(testCtx, test->m_name.c_str(), "Convert integers with OpUConvert.", spec, test->m_features));
	}
	return group.release();
}

const string getNumberTypeName (const NumberType type)
{
	if (type == NUMBERTYPE_INT32)
	{
		return "int";
	}
	else if (type == NUMBERTYPE_UINT32)
	{
		return "uint";
	}
	else if (type == NUMBERTYPE_FLOAT32)
	{
		return "float";
	}
	else
	{
		DE_ASSERT(false);
		return "";
	}
}

deInt32 getInt(de::Random& rnd)
{
	return rnd.getInt(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
}

const string repeatString (const string& str, int times)
{
	string filler;
	for (int i = 0; i < times; ++i)
	{
		filler += str;
	}
	return filler;
}

const string getRandomConstantString (const NumberType type, de::Random& rnd)
{
	if (type == NUMBERTYPE_INT32)
	{
		return numberToString<deInt32>(getInt(rnd));
	}
	else if (type == NUMBERTYPE_UINT32)
	{
		return numberToString<deUint32>(rnd.getUint32());
	}
	else if (type == NUMBERTYPE_FLOAT32)
	{
		return numberToString<float>(rnd.getFloat());
	}
	else
	{
		DE_ASSERT(false);
		return "";
	}
}

void createVectorCompositeCases (vector<map<string, string> >& testCases, de::Random& rnd, const NumberType type)
{
	map<string, string> params;

	// Vec2 to Vec4
	for (int width = 2; width <= 4; ++width)
	{
		const string randomConst = numberToString(getInt(rnd));
		const string widthStr = numberToString(width);
		const string composite_type = "${customType}vec" + widthStr;
		const int index = rnd.getInt(0, width-1);

		params["type"]			= "vec";
		params["name"]			= params["type"] + "_" + widthStr;
		params["compositeDecl"]		= composite_type + " = OpTypeVector ${customType} " + widthStr +"\n";
		params["compositeType"]		= composite_type;
		params["filler"]		= string("%filler    = OpConstant ${customType} ") + getRandomConstantString(type, rnd) + "\n";
		params["compositeConstruct"]	= "%instance  = OpCompositeConstruct " + composite_type + repeatString(" %filler", width) + "\n";
		params["indexes"]		= numberToString(index);
		testCases.push_back(params);
	}
}

void createArrayCompositeCases (vector<map<string, string> >& testCases, de::Random& rnd, const NumberType type)
{
	const int limit = 10;
	map<string, string> params;

	for (int width = 2; width <= limit; ++width)
	{
		string randomConst = numberToString(getInt(rnd));
		string widthStr = numberToString(width);
		int index = rnd.getInt(0, width-1);

		params["type"]			= "array";
		params["name"]			= params["type"] + "_" + widthStr;
		params["compositeDecl"]		= string("%arraywidth = OpConstant %u32 " + widthStr + "\n")
											+	 "%composite = OpTypeArray ${customType} %arraywidth\n";
		params["compositeType"]		= "%composite";
		params["filler"]		= string("%filler    = OpConstant ${customType} ") + getRandomConstantString(type, rnd) + "\n";
		params["compositeConstruct"]	= "%instance  = OpCompositeConstruct %composite" + repeatString(" %filler", width) + "\n";
		params["indexes"]		= numberToString(index);
		testCases.push_back(params);
	}
}

void createStructCompositeCases (vector<map<string, string> >& testCases, de::Random& rnd, const NumberType type)
{
	const int limit = 10;
	map<string, string> params;

	for (int width = 2; width <= limit; ++width)
	{
		string randomConst = numberToString(getInt(rnd));
		int index = rnd.getInt(0, width-1);

		params["type"]			= "struct";
		params["name"]			= params["type"] + "_" + numberToString(width);
		params["compositeDecl"]		= "%composite = OpTypeStruct" + repeatString(" ${customType}", width) + "\n";
		params["compositeType"]		= "%composite";
		params["filler"]		= string("%filler    = OpConstant ${customType} ") + getRandomConstantString(type, rnd) + "\n";
		params["compositeConstruct"]	= "%instance  = OpCompositeConstruct %composite" + repeatString(" %filler", width) + "\n";
		params["indexes"]		= numberToString(index);
		testCases.push_back(params);
	}
}

void createMatrixCompositeCases (vector<map<string, string> >& testCases, de::Random& rnd, const NumberType type)
{
	map<string, string> params;

	// Vec2 to Vec4
	for (int width = 2; width <= 4; ++width)
	{
		string widthStr = numberToString(width);

		for (int column = 2 ; column <= 4; ++column)
		{
			int index_0 = rnd.getInt(0, column-1);
			int index_1 = rnd.getInt(0, width-1);
			string columnStr = numberToString(column);

			params["type"]		= "matrix";
			params["name"]		= params["type"] + "_" + widthStr + "x" + columnStr;
			params["compositeDecl"]	= string("%vectype   = OpTypeVector ${customType} " + widthStr + "\n")
												+	 "%composite = OpTypeMatrix %vectype " + columnStr + "\n";
			params["compositeType"]	= "%composite";

			params["filler"]	= string("%filler    = OpConstant ${customType} ") + getRandomConstantString(type, rnd) + "\n"
												+	 "%fillerVec = OpConstantComposite %vectype" + repeatString(" %filler", width) + "\n";

			params["compositeConstruct"]	= "%instance  = OpCompositeConstruct %composite" + repeatString(" %fillerVec", column) + "\n";
			params["indexes"]	= numberToString(index_0) + " " + numberToString(index_1);
			testCases.push_back(params);
		}
	}
}

void createCompositeCases (vector<map<string, string> >& testCases, de::Random& rnd, const NumberType type)
{
	createVectorCompositeCases(testCases, rnd, type);
	createArrayCompositeCases(testCases, rnd, type);
	createStructCompositeCases(testCases, rnd, type);
	// Matrix only supports float types
	if (type == NUMBERTYPE_FLOAT32)
	{
		createMatrixCompositeCases(testCases, rnd, type);
	}
}

const string getAssemblyTypeDeclaration (const NumberType type)
{
	switch (type)
	{
		case NUMBERTYPE_INT32:		return "OpTypeInt 32 1";
		case NUMBERTYPE_UINT32:		return "OpTypeInt 32 0";
		case NUMBERTYPE_FLOAT32:	return "OpTypeFloat 32";
		default:			DE_ASSERT(false); return "";
	}
}

const string getAssemblyTypeName (const NumberType type)
{
	switch (type)
	{
		case NUMBERTYPE_INT32:		return "%i32";
		case NUMBERTYPE_UINT32:		return "%u32";
		case NUMBERTYPE_FLOAT32:	return "%f32";
		default:			DE_ASSERT(false); return "";
	}
}

const string specializeCompositeInsertShaderTemplate (const NumberType type, const map<string, string>& params)
{
	map<string, string>	parameters(params);

	const string customType = getAssemblyTypeName(type);
	map<string, string> substCustomType;
	substCustomType["customType"] = customType;
	parameters["compositeDecl"] = StringTemplate(parameters.at("compositeDecl")).specialize(substCustomType);
	parameters["compositeType"] = StringTemplate(parameters.at("compositeType")).specialize(substCustomType);
	parameters["compositeConstruct"] = StringTemplate(parameters.at("compositeConstruct")).specialize(substCustomType);
	parameters["filler"] = StringTemplate(parameters.at("filler")).specialize(substCustomType);
	parameters["customType"] = customType;
	parameters["compositeDecorator"] = (parameters["type"] == "array") ? "OpDecorate %composite ArrayStride 4\n" : "";

	if (parameters.at("compositeType") != "%u32vec3")
	{
		parameters["u32vec3Decl"] = "%u32vec3   = OpTypeVector %u32 3\n";
	}

	return StringTemplate(
		"OpCapability Shader\n"
		"OpCapability Matrix\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"

		// Decorators
		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %buf BufferBlock\n"
		"OpDecorate %indata DescriptorSet 0\n"
		"OpDecorate %indata Binding 0\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 1\n"
		"OpDecorate %customarr ArrayStride 4\n"
		"${compositeDecorator}"
		"OpMemberDecorate %buf 0 Offset 0\n"

		// General types
		"%void      = OpTypeVoid\n"
		"%voidf     = OpTypeFunction %void\n"
		"%u32       = OpTypeInt 32 0\n"
		"%i32       = OpTypeInt 32 1\n"
		"%f32       = OpTypeFloat 32\n"

		// Composite declaration
		"${compositeDecl}"

		// Constants
		"${filler}"

		"${u32vec3Decl:opt}"
		"%uvec3ptr  = OpTypePointer Input %u32vec3\n"

		// Inherited from custom
		"%customptr = OpTypePointer Uniform ${customType}\n"
		"%customarr = OpTypeRuntimeArray ${customType}\n"
		"%buf       = OpTypeStruct %customarr\n"
		"%bufptr    = OpTypePointer Uniform %buf\n"

		"%indata    = OpVariable %bufptr Uniform\n"
		"%outdata   = OpVariable %bufptr Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %i32 0\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %u32vec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"

		"%inloc     = OpAccessChain %customptr %indata %zero %x\n"
		"%outloc    = OpAccessChain %customptr %outdata %zero %x\n"
		// Read the input value
		"%inval     = OpLoad ${customType} %inloc\n"
		// Create the composite and fill it
		"${compositeConstruct}"
		// Insert the input value to a place
		"%instance2 = OpCompositeInsert ${compositeType} %inval %instance ${indexes}\n"
		// Read back the value from the position
		"%out_val   = OpCompositeExtract ${customType} %instance2 ${indexes}\n"
		// Store it in the output position
		"             OpStore %outloc %out_val\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n"
	).specialize(parameters);
}

template<typename T>
BufferSp createCompositeBuffer(T number)
{
	return BufferSp(new Buffer<T>(vector<T>(1, number)));
}

tcu::TestCaseGroup* createOpCompositeInsertGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group	(new tcu::TestCaseGroup(testCtx, "opcompositeinsert", "Test the OpCompositeInsert instruction"));
	de::Random						rnd		(deStringHash(group->getName()));

	for (int type = NUMBERTYPE_INT32; type != NUMBERTYPE_END32; ++type)
	{
		NumberType						numberType		= NumberType(type);
		const string					typeName		= getNumberTypeName(numberType);
		const string					description		= "Test the OpCompositeInsert instruction with " + typeName + "s";
		de::MovePtr<tcu::TestCaseGroup>	subGroup		(new tcu::TestCaseGroup(testCtx, typeName.c_str(), description.c_str()));
		vector<map<string, string> >	testCases;

		createCompositeCases(testCases, rnd, numberType);

		for (vector<map<string, string> >::const_iterator test = testCases.begin(); test != testCases.end(); ++test)
		{
			ComputeShaderSpec	spec;

			spec.assembly = specializeCompositeInsertShaderTemplate(numberType, *test);

			switch (numberType)
			{
				case NUMBERTYPE_INT32:
				{
					deInt32 number = getInt(rnd);
					spec.inputs.push_back(createCompositeBuffer<deInt32>(number));
					spec.outputs.push_back(createCompositeBuffer<deInt32>(number));
					break;
				}
				case NUMBERTYPE_UINT32:
				{
					deUint32 number = rnd.getUint32();
					spec.inputs.push_back(createCompositeBuffer<deUint32>(number));
					spec.outputs.push_back(createCompositeBuffer<deUint32>(number));
					break;
				}
				case NUMBERTYPE_FLOAT32:
				{
					float number = rnd.getFloat();
					spec.inputs.push_back(createCompositeBuffer<float>(number));
					spec.outputs.push_back(createCompositeBuffer<float>(number));
					break;
				}
				default:
					DE_ASSERT(false);
			}

			spec.numWorkGroups = IVec3(1, 1, 1);
			subGroup->addChild(new SpvAsmComputeShaderCase(testCtx, test->at("name").c_str(), "OpCompositeInsert test", spec));
		}
		group->addChild(subGroup.release());
	}
	return group.release();
}

struct AssemblyStructInfo
{
	AssemblyStructInfo (const deUint32 comp, const deUint32 idx)
	: components	(comp)
	, index			(idx)
	{}

	deUint32 components;
	deUint32 index;
};

const string specializeInBoundsShaderTemplate (const NumberType type, const AssemblyStructInfo& structInfo, const map<string, string>& params)
{
	// Create the full index string
	string				fullIndex	= numberToString(structInfo.index) + " " + params.at("indexes");
	// Convert it to list of indexes
	vector<string>		indexes		= de::splitString(fullIndex, ' ');

	map<string, string>	parameters	(params);
	parameters["structType"]	= repeatString(" ${compositeType}", structInfo.components);
	parameters["structConstruct"]	= repeatString(" %instance", structInfo.components);
	parameters["insertIndexes"]	= fullIndex;

	// In matrix cases the last two index is the CompositeExtract indexes
	const deUint32 extractIndexes = (parameters["type"] == "matrix") ? 2 : 1;

	// Construct the extractIndex
	for (vector<string>::const_iterator index = indexes.end() - extractIndexes; index != indexes.end(); ++index)
	{
		parameters["extractIndexes"] += " " + *index;
	}

	// Remove the last 1 or 2 element depends on matrix case or not
	indexes.erase(indexes.end() - extractIndexes, indexes.end());

	deUint32 id = 0;
	// Generate AccessChain index expressions (except for the last one, because we use ptr to the composite)
	for (vector<string>::const_iterator index = indexes.begin(); index != indexes.end(); ++index)
	{
		string indexId = "%index_" + numberToString(id++);
		parameters["accessChainConstDeclaration"] += indexId + "   = OpConstant %u32 " + *index + "\n";
		parameters["accessChainIndexes"] += " " + indexId;
	}

	parameters["compositeDecorator"] = (parameters["type"] == "array") ? "OpDecorate %composite ArrayStride 4\n" : "";

	const string customType = getAssemblyTypeName(type);
	map<string, string> substCustomType;
	substCustomType["customType"] = customType;
	parameters["compositeDecl"] = StringTemplate(parameters.at("compositeDecl")).specialize(substCustomType);
	parameters["compositeType"] = StringTemplate(parameters.at("compositeType")).specialize(substCustomType);
	parameters["compositeConstruct"] = StringTemplate(parameters.at("compositeConstruct")).specialize(substCustomType);
	parameters["filler"] = StringTemplate(parameters.at("filler")).specialize(substCustomType);
	parameters["customType"] = customType;

	const string compositeType = parameters.at("compositeType");
	map<string, string> substCompositeType;
	substCompositeType["compositeType"] = compositeType;
	parameters["structType"] = StringTemplate(parameters.at("structType")).specialize(substCompositeType);
	if (compositeType != "%u32vec3")
	{
		parameters["u32vec3Decl"] = "%u32vec3   = OpTypeVector %u32 3\n";
	}

	return StringTemplate(
		"OpCapability Shader\n"
		"OpCapability Matrix\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"

		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"
		// Decorators
		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %buf BufferBlock\n"
		"OpDecorate %indata DescriptorSet 0\n"
		"OpDecorate %indata Binding 0\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 1\n"
		"OpDecorate %customarr ArrayStride 4\n"
		"${compositeDecorator}"
		"OpMemberDecorate %buf 0 Offset 0\n"
		// General types
		"%void      = OpTypeVoid\n"
		"%voidf     = OpTypeFunction %void\n"
		"%i32       = OpTypeInt 32 1\n"
		"%u32       = OpTypeInt 32 0\n"
		"%f32       = OpTypeFloat 32\n"
		// Custom types
		"${compositeDecl}"
		// %u32vec3 if not already declared in ${compositeDecl}
		"${u32vec3Decl:opt}"
		"%uvec3ptr  = OpTypePointer Input %u32vec3\n"
		// Inherited from composite
		"%composite_p = OpTypePointer Function ${compositeType}\n"
		"%struct_t  = OpTypeStruct${structType}\n"
		"%struct_p  = OpTypePointer Function %struct_t\n"
		// Constants
		"${filler}"
		"${accessChainConstDeclaration}"
		// Inherited from custom
		"%customptr = OpTypePointer Uniform ${customType}\n"
		"%customarr = OpTypeRuntimeArray ${customType}\n"
		"%buf       = OpTypeStruct %customarr\n"
		"%bufptr    = OpTypePointer Uniform %buf\n"
		"%indata    = OpVariable %bufptr Uniform\n"
		"%outdata   = OpVariable %bufptr Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"
		"%zero      = OpConstant %u32 0\n"
		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%struct_v  = OpVariable %struct_p Function\n"
		"%idval     = OpLoad %u32vec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		// Create the input/output type
		"%inloc     = OpInBoundsAccessChain %customptr %indata %zero %x\n"
		"%outloc    = OpInBoundsAccessChain %customptr %outdata %zero %x\n"
		// Read the input value
		"%inval     = OpLoad ${customType} %inloc\n"
		// Create the composite and fill it
		"${compositeConstruct}"
		// Create the struct and fill it with the composite
		"%struct    = OpCompositeConstruct %struct_t${structConstruct}\n"
		// Insert the value
		"%comp_obj  = OpCompositeInsert %struct_t %inval %struct ${insertIndexes}\n"
		// Store the object
		"             OpStore %struct_v %comp_obj\n"
		// Get deepest possible composite pointer
		"%inner_ptr = OpInBoundsAccessChain %composite_p %struct_v${accessChainIndexes}\n"
		"%read_obj  = OpLoad ${compositeType} %inner_ptr\n"
		// Read back the stored value
		"%read_val  = OpCompositeExtract ${customType} %read_obj${extractIndexes}\n"
		"             OpStore %outloc %read_val\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n"
	).specialize(parameters);
}

tcu::TestCaseGroup* createOpInBoundsAccessChainGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group			(new tcu::TestCaseGroup(testCtx, "opinboundsaccesschain", "Test the OpInBoundsAccessChain instruction"));
	de::Random						rnd				(deStringHash(group->getName()));

	for (int type = NUMBERTYPE_INT32; type != NUMBERTYPE_END32; ++type)
	{
		NumberType						numberType	= NumberType(type);
		const string					typeName	= getNumberTypeName(numberType);
		const string					description	= "Test the OpInBoundsAccessChain instruction with " + typeName + "s";
		de::MovePtr<tcu::TestCaseGroup>	subGroup	(new tcu::TestCaseGroup(testCtx, typeName.c_str(), description.c_str()));

		vector<map<string, string> >	testCases;
		createCompositeCases(testCases, rnd, numberType);

		for (vector<map<string, string> >::const_iterator test = testCases.begin(); test != testCases.end(); ++test)
		{
			ComputeShaderSpec	spec;

			// Number of components inside of a struct
			deUint32 structComponents = rnd.getInt(2, 8);
			// Component index value
			deUint32 structIndex = rnd.getInt(0, structComponents - 1);
			AssemblyStructInfo structInfo(structComponents, structIndex);

			spec.assembly = specializeInBoundsShaderTemplate(numberType, structInfo, *test);

			switch (numberType)
			{
				case NUMBERTYPE_INT32:
				{
					deInt32 number = getInt(rnd);
					spec.inputs.push_back(createCompositeBuffer<deInt32>(number));
					spec.outputs.push_back(createCompositeBuffer<deInt32>(number));
					break;
				}
				case NUMBERTYPE_UINT32:
				{
					deUint32 number = rnd.getUint32();
					spec.inputs.push_back(createCompositeBuffer<deUint32>(number));
					spec.outputs.push_back(createCompositeBuffer<deUint32>(number));
					break;
				}
				case NUMBERTYPE_FLOAT32:
				{
					float number = rnd.getFloat();
					spec.inputs.push_back(createCompositeBuffer<float>(number));
					spec.outputs.push_back(createCompositeBuffer<float>(number));
					break;
				}
				default:
					DE_ASSERT(false);
			}
			spec.numWorkGroups = IVec3(1, 1, 1);
			subGroup->addChild(new SpvAsmComputeShaderCase(testCtx, test->at("name").c_str(), "OpInBoundsAccessChain test", spec));
		}
		group->addChild(subGroup.release());
	}
	return group.release();
}

// If the params missing, uninitialized case
const string specializeDefaultOutputShaderTemplate (const NumberType type, const map<string, string>& params = map<string, string>())
{
	map<string, string> parameters(params);

	parameters["customType"]	= getAssemblyTypeName(type);

	// Declare the const value, and use it in the initializer
	if (params.find("constValue") != params.end())
	{
		parameters["variableInitializer"]	= " %const";
	}
	// Uninitialized case
	else
	{
		parameters["commentDecl"]	= ";";
	}

	return StringTemplate(
		"OpCapability Shader\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"
		"OpSource GLSL 430\n"
		"OpName %main           \"main\"\n"
		"OpName %id             \"gl_GlobalInvocationID\"\n"
		// Decorators
		"OpDecorate %id BuiltIn GlobalInvocationId\n"
		"OpDecorate %indata DescriptorSet 0\n"
		"OpDecorate %indata Binding 0\n"
		"OpDecorate %outdata DescriptorSet 0\n"
		"OpDecorate %outdata Binding 1\n"
		"OpDecorate %in_arr ArrayStride 4\n"
		"OpDecorate %in_buf BufferBlock\n"
		"OpMemberDecorate %in_buf 0 Offset 0\n"
		// Base types
		"%void       = OpTypeVoid\n"
		"%voidf      = OpTypeFunction %void\n"
		"%u32        = OpTypeInt 32 0\n"
		"%i32        = OpTypeInt 32 1\n"
		"%f32        = OpTypeFloat 32\n"
		"%uvec3      = OpTypeVector %u32 3\n"
		"%uvec3ptr   = OpTypePointer Input %uvec3\n"
		"${commentDecl:opt}%const      = OpConstant ${customType} ${constValue:opt}\n"
		// Derived types
		"%in_ptr     = OpTypePointer Uniform ${customType}\n"
		"%in_arr     = OpTypeRuntimeArray ${customType}\n"
		"%in_buf     = OpTypeStruct %in_arr\n"
		"%in_bufptr  = OpTypePointer Uniform %in_buf\n"
		"%indata     = OpVariable %in_bufptr Uniform\n"
		"%outdata    = OpVariable %in_bufptr Uniform\n"
		"%id         = OpVariable %uvec3ptr Input\n"
		"%var_ptr    = OpTypePointer Function ${customType}\n"
		// Constants
		"%zero       = OpConstant %i32 0\n"
		// Main function
		"%main       = OpFunction %void None %voidf\n"
		"%label      = OpLabel\n"
		"%out_var    = OpVariable %var_ptr Function${variableInitializer:opt}\n"
		"%idval      = OpLoad %uvec3 %id\n"
		"%x          = OpCompositeExtract %u32 %idval 0\n"
		"%inloc      = OpAccessChain %in_ptr %indata %zero %x\n"
		"%outloc     = OpAccessChain %in_ptr %outdata %zero %x\n"

		"%outval     = OpLoad ${customType} %out_var\n"
		"              OpStore %outloc %outval\n"
		"              OpReturn\n"
		"              OpFunctionEnd\n"
	).specialize(parameters);
}

bool compareFloats (const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog& log)
{
	DE_ASSERT(outputAllocs.size() != 0);
	DE_ASSERT(outputAllocs.size() == expectedOutputs.size());

	// Use custom epsilon because of the float->string conversion
	const float	epsilon	= 0.00001f;

	for (size_t outputNdx = 0; outputNdx < outputAllocs.size(); ++outputNdx)
	{
		vector<deUint8>	expectedBytes;
		float			expected;
		float			actual;

		expectedOutputs[outputNdx]->getBytes(expectedBytes);
		memcpy(&expected, &expectedBytes.front(), expectedBytes.size());
		memcpy(&actual, outputAllocs[outputNdx]->getHostPtr(), expectedBytes.size());

		// Test with epsilon
		if (fabs(expected - actual) > epsilon)
		{
			log << TestLog::Message << "Error: The actual and expected values not matching."
				<< " Expected: " << expected << " Actual: " << actual << " Epsilon: " << epsilon << TestLog::EndMessage;
			return false;
		}
	}
	return true;
}

// Checks if the driver crash with uninitialized cases
bool passthruVerify (const std::vector<BufferSp>&, const vector<AllocationSp>& outputAllocs, const std::vector<BufferSp>& expectedOutputs, TestLog&)
{
	DE_ASSERT(outputAllocs.size() != 0);
	DE_ASSERT(outputAllocs.size() == expectedOutputs.size());

	// Copy and discard the result.
	for (size_t outputNdx = 0; outputNdx < outputAllocs.size(); ++outputNdx)
	{
		vector<deUint8>	expectedBytes;
		expectedOutputs[outputNdx]->getBytes(expectedBytes);

		const size_t	width			= expectedBytes.size();
		vector<char>	data			(width);

		memcpy(&data[0], outputAllocs[outputNdx]->getHostPtr(), width);
	}
	return true;
}

tcu::TestCaseGroup* createShaderDefaultOutputGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group	(new tcu::TestCaseGroup(testCtx, "shader_default_output", "Test shader default output."));
	de::Random						rnd		(deStringHash(group->getName()));

	for (int type = NUMBERTYPE_INT32; type != NUMBERTYPE_END32; ++type)
	{
		NumberType						numberType	= NumberType(type);
		const string					typeName	= getNumberTypeName(numberType);
		const string					description	= "Test the OpVariable initializer with " + typeName + ".";
		de::MovePtr<tcu::TestCaseGroup>	subGroup	(new tcu::TestCaseGroup(testCtx, typeName.c_str(), description.c_str()));

		// 2 similar subcases (initialized and uninitialized)
		for (int subCase = 0; subCase < 2; ++subCase)
		{
			ComputeShaderSpec spec;
			spec.numWorkGroups = IVec3(1, 1, 1);

			map<string, string>				params;

			switch (numberType)
			{
				case NUMBERTYPE_INT32:
				{
					deInt32 number = getInt(rnd);
					spec.inputs.push_back(createCompositeBuffer<deInt32>(number));
					spec.outputs.push_back(createCompositeBuffer<deInt32>(number));
					params["constValue"] = numberToString(number);
					break;
				}
				case NUMBERTYPE_UINT32:
				{
					deUint32 number = rnd.getUint32();
					spec.inputs.push_back(createCompositeBuffer<deUint32>(number));
					spec.outputs.push_back(createCompositeBuffer<deUint32>(number));
					params["constValue"] = numberToString(number);
					break;
				}
				case NUMBERTYPE_FLOAT32:
				{
					float number = rnd.getFloat();
					spec.inputs.push_back(createCompositeBuffer<float>(number));
					spec.outputs.push_back(createCompositeBuffer<float>(number));
					spec.verifyIO = &compareFloats;
					params["constValue"] = numberToString(number);
					break;
				}
				default:
					DE_ASSERT(false);
			}

			// Initialized subcase
			if (!subCase)
			{
				spec.assembly = specializeDefaultOutputShaderTemplate(numberType, params);
				subGroup->addChild(new SpvAsmComputeShaderCase(testCtx, "initialized", "OpVariable initializer tests.", spec));
			}
			// Uninitialized subcase
			else
			{
				spec.assembly = specializeDefaultOutputShaderTemplate(numberType);
				spec.verifyIO = &passthruVerify;
				subGroup->addChild(new SpvAsmComputeShaderCase(testCtx, "uninitialized", "OpVariable initializer tests.", spec));
			}
		}
		group->addChild(subGroup.release());
	}
	return group.release();
}

tcu::TestCaseGroup* createOpNopTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	testGroup (new tcu::TestCaseGroup(testCtx, "opnop", "Test OpNop"));
	RGBA							defaultColors[4];
	map<string, string>				opNopFragments;

	getDefaultColors(defaultColors);

	opNopFragments["testfun"]		=
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"%param1 = OpFunctionParameter %v4f32\n"
		"%label_testfun = OpLabel\n"
		"OpNop\n"
		"OpNop\n"
		"OpNop\n"
		"OpNop\n"
		"OpNop\n"
		"OpNop\n"
		"OpNop\n"
		"OpNop\n"
		"%a = OpVectorExtractDynamic %f32 %param1 %c_i32_0\n"
		"%b = OpFAdd %f32 %a %a\n"
		"OpNop\n"
		"%c = OpFSub %f32 %b %a\n"
		"%ret = OpVectorInsertDynamic %v4f32 %param1 %c %c_i32_0\n"
		"OpNop\n"
		"OpNop\n"
		"OpReturnValue %ret\n"
		"OpFunctionEnd\n";

	createTestsForAllStages("opnop", defaultColors, defaultColors, opNopFragments, testGroup.get());

	return testGroup.release();
}

tcu::TestCaseGroup* createInstructionTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> instructionTests	(new tcu::TestCaseGroup(testCtx, "instruction", "Instructions with special opcodes/operands"));
	de::MovePtr<tcu::TestCaseGroup> computeTests		(new tcu::TestCaseGroup(testCtx, "compute", "Compute Instructions with special opcodes/operands"));
	de::MovePtr<tcu::TestCaseGroup> graphicsTests		(new tcu::TestCaseGroup(testCtx, "graphics", "Graphics Instructions with special opcodes/operands"));

	computeTests->addChild(createLocalSizeGroup(testCtx));
	computeTests->addChild(createOpNopGroup(testCtx));
	computeTests->addChild(createOpFUnordGroup(testCtx));
	computeTests->addChild(createOpAtomicGroup(testCtx, false));
	computeTests->addChild(createOpAtomicGroup(testCtx, true)); // Using new StorageBuffer decoration
	computeTests->addChild(createOpLineGroup(testCtx));
	computeTests->addChild(createOpNoLineGroup(testCtx));
	computeTests->addChild(createOpConstantNullGroup(testCtx));
	computeTests->addChild(createOpConstantCompositeGroup(testCtx));
	computeTests->addChild(createOpConstantUsageGroup(testCtx));
	computeTests->addChild(createSpecConstantGroup(testCtx));
	computeTests->addChild(createOpSourceGroup(testCtx));
	computeTests->addChild(createOpSourceExtensionGroup(testCtx));
	computeTests->addChild(createDecorationGroupGroup(testCtx));
	computeTests->addChild(createOpPhiGroup(testCtx));
	computeTests->addChild(createLoopControlGroup(testCtx));
	computeTests->addChild(createFunctionControlGroup(testCtx));
	computeTests->addChild(createSelectionControlGroup(testCtx));
	computeTests->addChild(createBlockOrderGroup(testCtx));
	computeTests->addChild(createMultipleShaderGroup(testCtx));
	computeTests->addChild(createMemoryAccessGroup(testCtx));
	computeTests->addChild(createOpCopyMemoryGroup(testCtx));
	computeTests->addChild(createOpCopyObjectGroup(testCtx));
	computeTests->addChild(createNoContractionGroup(testCtx));
	computeTests->addChild(createOpUndefGroup(testCtx));
	computeTests->addChild(createOpUnreachableGroup(testCtx));
	computeTests ->addChild(createOpQuantizeToF16Group(testCtx));
	computeTests ->addChild(createOpFRemGroup(testCtx));
	computeTests->addChild(createOpSRemComputeGroup(testCtx, QP_TEST_RESULT_PASS));
	computeTests->addChild(createOpSRemComputeGroup64(testCtx, QP_TEST_RESULT_PASS));
	computeTests->addChild(createOpSModComputeGroup(testCtx, QP_TEST_RESULT_PASS));
	computeTests->addChild(createOpSModComputeGroup64(testCtx, QP_TEST_RESULT_PASS));
	computeTests->addChild(createSConvertTests(testCtx));
	computeTests->addChild(createUConvertTests(testCtx));
	computeTests->addChild(createOpCompositeInsertGroup(testCtx));
	computeTests->addChild(createOpInBoundsAccessChainGroup(testCtx));
	computeTests->addChild(createShaderDefaultOutputGroup(testCtx));
	computeTests->addChild(createOpNMinGroup(testCtx));
	computeTests->addChild(createOpNMaxGroup(testCtx));
	computeTests->addChild(createOpNClampGroup(testCtx));
	{
		de::MovePtr<tcu::TestCaseGroup>	computeAndroidTests	(new tcu::TestCaseGroup(testCtx, "android", "Android CTS Tests"));

		computeAndroidTests->addChild(createOpSRemComputeGroup(testCtx, QP_TEST_RESULT_QUALITY_WARNING));
		computeAndroidTests->addChild(createOpSModComputeGroup(testCtx, QP_TEST_RESULT_QUALITY_WARNING));

		computeTests->addChild(computeAndroidTests.release());
	}

	computeTests->addChild(create16BitStorageComputeGroup(testCtx));
	computeTests->addChild(createUboMatrixPaddingComputeGroup(testCtx));
	computeTests->addChild(createConditionalBranchComputeGroup(testCtx));
	computeTests->addChild(createIndexingComputeGroup(testCtx));
	computeTests->addChild(createVariablePointersComputeGroup(testCtx));
	computeTests->addChild(createImageSamplerComputeGroup(testCtx));
	graphicsTests->addChild(createOpNopTests(testCtx));
	graphicsTests->addChild(createOpSourceTests(testCtx));
	graphicsTests->addChild(createOpSourceContinuedTests(testCtx));
	graphicsTests->addChild(createOpLineTests(testCtx));
	graphicsTests->addChild(createOpNoLineTests(testCtx));
	graphicsTests->addChild(createOpConstantNullTests(testCtx));
	graphicsTests->addChild(createOpConstantCompositeTests(testCtx));
	graphicsTests->addChild(createMemoryAccessTests(testCtx));
	graphicsTests->addChild(createOpUndefTests(testCtx));
	graphicsTests->addChild(createSelectionBlockOrderTests(testCtx));
	graphicsTests->addChild(createModuleTests(testCtx));
	graphicsTests->addChild(createSwitchBlockOrderTests(testCtx));
	graphicsTests->addChild(createOpPhiTests(testCtx));
	graphicsTests->addChild(createNoContractionTests(testCtx));
	graphicsTests->addChild(createOpQuantizeTests(testCtx));
	graphicsTests->addChild(createLoopTests(testCtx));
	graphicsTests->addChild(createSpecConstantTests(testCtx));
	graphicsTests->addChild(createSpecConstantOpQuantizeToF16Group(testCtx));
	graphicsTests->addChild(createBarrierTests(testCtx));
	graphicsTests->addChild(createDecorationGroupTests(testCtx));
	graphicsTests->addChild(createFRemTests(testCtx));
	graphicsTests->addChild(createOpSRemGraphicsTests(testCtx, QP_TEST_RESULT_PASS));
	graphicsTests->addChild(createOpSModGraphicsTests(testCtx, QP_TEST_RESULT_PASS));

	{
		de::MovePtr<tcu::TestCaseGroup>	graphicsAndroidTests	(new tcu::TestCaseGroup(testCtx, "android", "Android CTS Tests"));

		graphicsAndroidTests->addChild(createOpSRemGraphicsTests(testCtx, QP_TEST_RESULT_QUALITY_WARNING));
		graphicsAndroidTests->addChild(createOpSModGraphicsTests(testCtx, QP_TEST_RESULT_QUALITY_WARNING));

		graphicsTests->addChild(graphicsAndroidTests.release());
	}

	graphicsTests->addChild(create16BitStorageGraphicsGroup(testCtx));
	graphicsTests->addChild(createUboMatrixPaddingGraphicsGroup(testCtx));
	graphicsTests->addChild(createConditionalBranchGraphicsGroup(testCtx));
	graphicsTests->addChild(createIndexingGraphicsGroup(testCtx));
	graphicsTests->addChild(createVariablePointersGraphicsGroup(testCtx));
	graphicsTests->addChild(createImageSamplerGraphicsGroup(testCtx));

	instructionTests->addChild(computeTests.release());
	instructionTests->addChild(graphicsTests.release());

	return instructionTests.release();
}

} // SpirVAssembly
} // vkt
