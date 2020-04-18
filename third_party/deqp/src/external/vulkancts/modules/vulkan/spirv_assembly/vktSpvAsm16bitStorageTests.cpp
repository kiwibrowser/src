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
 * \brief SPIR-V Assembly Tests for the VK_KHR_16bit_storage
 *//*--------------------------------------------------------------------*/

// VK_KHR_16bit_storage
//
// \todo [2017-02-08 antiagainst] Additional corner cases to check:
//
// * Test OpAccessChain with subword types
//  * For newly enabled types T:
//    * For composite types: vector, matrix, structures, array over T:
//      1. Use OpAccessChain to form a pointer to a subword type.
//      2. Load the subword value X16.
//      3. Convert X16 to X32.
//      4. Store X32 to BufferBlock.
//      5. Host inspects X32.
// * Test {StorageInputOutput16} 16-to-16:
//   * For newly enabled types T:
//     1. Host creates X16 stream values of type T.
//     2. Shaders have corresponding capability.
//     3. For each viable shader stage:
//       3a. Load X16 Input variable.
//       3b. Store X16 to Output variable.
//     4. Host inspects resulting values.
// * Test {StorageInputOutput16} 16-to-16 one value to two:
//     Like the previous test, but write X16 to two different output variables.
//     (Checks that the 16-bit intermediate value can be used twice.)

#include "vktSpvAsm16bitStorageTests.hpp"

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

struct Capability
{
	const char*				name;
	const char*				cap;
	const char*				decor;
	vk::VkDescriptorType	dtype;
};

static const Capability	CAPABILITIES[]	=
{
	{"uniform_buffer_block",	"StorageUniformBufferBlock16",	"BufferBlock",	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
	{"uniform",					"StorageUniform16",				"Block",		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
};

VulkanFeatures	get16BitStorageFeatures	(const char* cap)
{
	VulkanFeatures features;
	if (string(cap) == "uniform_buffer_block")
		features.ext16BitStorage = EXT16BITSTORAGEFEATURES_UNIFORM_BUFFER_BLOCK;
	else if (string(cap) == "uniform")
		features.ext16BitStorage = EXT16BITSTORAGEFEATURES_UNIFORM;
	else
		DE_ASSERT(false && "not supported");

	return features;
}


// Batch function to check arrays of 16-bit floats.
//
// For comparing 16-bit floats, we need to consider both RTZ and RTE. So we can only recalculate
// the expected values here instead of get the expected values directly from the test case.
// Thus we need original floats here but not expected outputs.
template<RoundingModeFlags RoundingMode>
bool graphicsCheck16BitFloats (const std::vector<Resource>&	originalFloats,
							   const vector<AllocationSp>&	outputAllocs,
							   const std::vector<Resource>&	/* expectedOutputs */,
							   tcu::TestLog&				log)
{
	if (outputAllocs.size() != originalFloats.size())
		return false;

	for (deUint32 outputNdx = 0; outputNdx < outputAllocs.size(); ++outputNdx)
	{
		vector<deUint8>	originalBytes;
		originalFloats[outputNdx].second->getBytes(originalBytes);

		const deUint16*	returned	= static_cast<const deUint16*>(outputAllocs[outputNdx]->getHostPtr());
		const float*	original	= reinterpret_cast<const float*>(&originalBytes.front());
		const deUint32	count		= static_cast<deUint32>(originalBytes.size() / sizeof(float));

		for (deUint32 numNdx = 0; numNdx < count; ++numNdx)
			if (!compare16BitFloat(original[numNdx], returned[numNdx], RoundingMode, log))
				return false;
	}

	return true;
}

template<RoundingModeFlags RoundingMode>
bool computeCheck16BitFloats (const std::vector<BufferSp>&	originalFloats,
							  const vector<AllocationSp>&	outputAllocs,
							  const std::vector<BufferSp>&	/* expectedOutputs */,
							  tcu::TestLog&					log)
{
	if (outputAllocs.size() != originalFloats.size())
		return false;

	for (deUint32 outputNdx = 0; outputNdx < outputAllocs.size(); ++outputNdx)
	{
		vector<deUint8>	originalBytes;
		originalFloats[outputNdx]->getBytes(originalBytes);

		const deUint16*	returned	= static_cast<const deUint16*>(outputAllocs[outputNdx]->getHostPtr());
		const float*	original	= reinterpret_cast<const float*>(&originalBytes.front());
		const deUint32	count		= static_cast<deUint32>(originalBytes.size() / sizeof(float));

		for (deUint32 numNdx = 0; numNdx < count; ++numNdx)
			if (!compare16BitFloat(original[numNdx], returned[numNdx], RoundingMode, log))
				return false;
	}

	return true;
}


// Batch function to check arrays of 32-bit floats.
//
// For comparing 32-bit floats, we just need the expected value precomputed in the test case.
// So we need expected outputs here but not original floats.
bool check32BitFloats (const std::vector<Resource>&		/* originalFloats */,
					   const std::vector<AllocationSp>& outputAllocs,
					   const std::vector<Resource>&		expectedOutputs,
					   tcu::TestLog&					log)
{
	if (outputAllocs.size() != expectedOutputs.size())
		return false;

	for (deUint32 outputNdx = 0; outputNdx < outputAllocs.size(); ++outputNdx)
	{
		vector<deUint8>	expectedBytes;
		expectedOutputs[outputNdx].second->getBytes(expectedBytes);

		const float*	returnedAsFloat	= static_cast<const float*>(outputAllocs[outputNdx]->getHostPtr());
		const float*	expectedAsFloat	= reinterpret_cast<const float*>(&expectedBytes.front());
		const deUint32	count			= static_cast<deUint32>(expectedBytes.size() / sizeof(float));

		for (deUint32 numNdx = 0; numNdx < count; ++numNdx)
			if (!compare32BitFloat(expectedAsFloat[numNdx], returnedAsFloat[numNdx], log))
				return false;
	}

	return true;
}

// Overload for compute pipeline
bool check32BitFloats (const std::vector<BufferSp>&		/* originalFloats */,
					   const std::vector<AllocationSp>& outputAllocs,
					   const std::vector<BufferSp>&		expectedOutputs,
					   tcu::TestLog&					log)
{
	if (outputAllocs.size() != expectedOutputs.size())
		return false;

	for (deUint32 outputNdx = 0; outputNdx < outputAllocs.size(); ++outputNdx)
	{
		vector<deUint8>	expectedBytes;
		expectedOutputs[outputNdx]->getBytes(expectedBytes);

		const float*	returnedAsFloat	= static_cast<const float*>(outputAllocs[outputNdx]->getHostPtr());
		const float*	expectedAsFloat	= reinterpret_cast<const float*>(&expectedBytes.front());
		const deUint32	count			= static_cast<deUint32>(expectedBytes.size() / sizeof(float));

		for (deUint32 numNdx = 0; numNdx < count; ++numNdx)
			if (!compare32BitFloat(expectedAsFloat[numNdx], returnedAsFloat[numNdx], log))
				return false;
	}

	return true;
}

// Generate and return 32-bit integers.
//
// Expected count to be at least 16.
vector<deInt32> getInt32s (de::Random& rnd, const deUint32 count)
{
	vector<deInt32>		data;

	data.reserve(count);

	// Make sure we have boundary numbers.
	data.push_back(deInt32(0x00000000));  // 0
	data.push_back(deInt32(0x00000001));  // 1
	data.push_back(deInt32(0x0000002a));  // 42
	data.push_back(deInt32(0x00007fff));  // 32767
	data.push_back(deInt32(0x00008000));  // 32768
	data.push_back(deInt32(0x0000ffff));  // 65535
	data.push_back(deInt32(0x00010000));  // 65536
	data.push_back(deInt32(0x7fffffff));  // 2147483647
	data.push_back(deInt32(0x80000000));  // -2147483648
	data.push_back(deInt32(0x80000001));  // -2147483647
	data.push_back(deInt32(0xffff0000));  // -65536
	data.push_back(deInt32(0xffff0001));  // -65535
	data.push_back(deInt32(0xffff8000));  // -32768
	data.push_back(deInt32(0xffff8001));  // -32767
	data.push_back(deInt32(0xffffffd6));  // -42
	data.push_back(deInt32(0xffffffff));  // -1

	DE_ASSERT(count >= data.size());

	for (deUint32 numNdx = static_cast<deUint32>(data.size()); numNdx < count; ++numNdx)
		data.push_back(static_cast<deInt32>(rnd.getUint32()));

	return data;
}

// Generate and return 16-bit integers.
//
// Expected count to be at least 8.
vector<deInt16> getInt16s (de::Random& rnd, const deUint32 count)
{
	vector<deInt16>		data;

	data.reserve(count);

	// Make sure we have boundary numbers.
	data.push_back(deInt16(0x0000));  // 0
	data.push_back(deInt16(0x0001));  // 1
	data.push_back(deInt16(0x002a));  // 42
	data.push_back(deInt16(0x7fff));  // 32767
	data.push_back(deInt16(0x8000));  // -32868
	data.push_back(deInt16(0x8001));  // -32767
	data.push_back(deInt16(0xffd6));  // -42
	data.push_back(deInt16(0xffff));  // -1

	DE_ASSERT(count >= data.size());

	for (deUint32 numNdx = static_cast<deUint32>(data.size()); numNdx < count; ++numNdx)
		data.push_back(static_cast<deInt16>(rnd.getUint16()));

	return data;
}

// IEEE-754 floating point numbers:
// +--------+------+----------+-------------+
// | binary | sign | exponent | significand |
// +--------+------+----------+-------------+
// | 16-bit |  1   |    5     |     10      |
// +--------+------+----------+-------------+
// | 32-bit |  1   |    8     |     23      |
// +--------+------+----------+-------------+
//
// 16-bit floats:
//
// 0   000 00   00 0000 0001 (0x0001: 2e-24:         minimum positive denormalized)
// 0   000 00   11 1111 1111 (0x03ff: 2e-14 - 2e-24: maximum positive denormalized)
// 0   000 01   00 0000 0000 (0x0400: 2e-14:         minimum positive normalized)
//
// 32-bit floats:
//
// 0   011 1110 1   001 0000 0000 0000 0000 0000 (0x3e900000: 0.28125: with exact match in 16-bit normalized)
// 0   011 1000 1   000 0000 0011 0000 0000 0000 (0x38803000: exact half way within two 16-bit normalized; round to zero: 0x0401)
// 1   011 1000 1   000 0000 0011 0000 0000 0000 (0xb8803000: exact half way within two 16-bit normalized; round to zero: 0x8402)
// 0   011 1000 1   000 0000 1111 1111 0000 0000 (0x3880ff00: not exact half way within two 16-bit normalized; round to zero: 0x0403)
// 1   011 1000 1   000 0000 1111 1111 0000 0000 (0xb880ff00: not exact half way within two 16-bit normalized; round to zero: 0x8404)


// Generate and return 32-bit floats
//
// The first 24 number pairs are manually picked, while the rest are randomly generated.
// Expected count to be at least 24 (numPicks).
vector<float> getFloat32s (de::Random& rnd, deUint32 count)
{
	vector<float>		float32;

	float32.reserve(count);

	// Zero
	float32.push_back(0.f);
	float32.push_back(-0.f);
	// Infinity
	float32.push_back(std::numeric_limits<float>::infinity());
	float32.push_back(-std::numeric_limits<float>::infinity());
	// SNaN
	float32.push_back(std::numeric_limits<float>::signaling_NaN());
	float32.push_back(-std::numeric_limits<float>::signaling_NaN());
	// QNaN
	float32.push_back(std::numeric_limits<float>::quiet_NaN());
	float32.push_back(-std::numeric_limits<float>::quiet_NaN());

	// Denormalized 32-bit float matching 0 in 16-bit
	float32.push_back(deFloatLdExp(1.f, -127));
	float32.push_back(-deFloatLdExp(1.f, -127));

	// Normalized 32-bit float matching 0 in 16-bit
	float32.push_back(deFloatLdExp(1.f, -100));
	float32.push_back(-deFloatLdExp(1.f, -100));
	// Normalized 32-bit float with exact denormalized match in 16-bit
	float32.push_back(deFloatLdExp(1.f, -24));  // 2e-24: minimum 16-bit positive denormalized
	float32.push_back(-deFloatLdExp(1.f, -24)); // 2e-24: maximum 16-bit negative denormalized
	// Normalized 32-bit float with exact normalized match in 16-bit
	float32.push_back(deFloatLdExp(1.f, -14));  // 2e-14: minimum 16-bit positive normalized
	float32.push_back(-deFloatLdExp(1.f, -14)); // 2e-14: maximum 16-bit negative normalized
	// Normalized 32-bit float falling above half way within two 16-bit normalized
	float32.push_back(bitwiseCast<float>(deUint32(0x3880ff00)));
	float32.push_back(bitwiseCast<float>(deUint32(0xb880ff00)));
	// Normalized 32-bit float falling exact half way within two 16-bit normalized
	float32.push_back(bitwiseCast<float>(deUint32(0x38803000)));
	float32.push_back(bitwiseCast<float>(deUint32(0xb8803000)));
	// Some number
	float32.push_back(0.28125f);
	float32.push_back(-0.28125f);
	// Normalized 32-bit float matching infinity in 16-bit
	float32.push_back(deFloatLdExp(1.f, 100));
	float32.push_back(-deFloatLdExp(1.f, 100));

	const deUint32		numPicks	= static_cast<deUint32>(float32.size());

	DE_ASSERT(count >= numPicks);
	count -= numPicks;

	for (deUint32 numNdx = 0; numNdx < count; ++numNdx)
		float32.push_back(rnd.getFloat());

	return float32;
}

// IEEE-754 floating point numbers:
// +--------+------+----------+-------------+
// | binary | sign | exponent | significand |
// +--------+------+----------+-------------+
// | 16-bit |  1   |    5     |     10      |
// +--------+------+----------+-------------+
// | 32-bit |  1   |    8     |     23      |
// +--------+------+----------+-------------+
//
// 16-bit floats:
//
// 0   000 00   00 0000 0001 (0x0001: 2e-24:         minimum positive denormalized)
// 0   000 00   11 1111 1111 (0x03ff: 2e-14 - 2e-24: maximum positive denormalized)
// 0   000 01   00 0000 0000 (0x0400: 2e-14:         minimum positive normalized)
//
// 0   000 00   00 0000 0000 (0x0000: +0)
// 0   111 11   00 0000 0000 (0x7c00: +Inf)
// 0   000 00   11 1111 0000 (0x03f0: +Denorm)
// 0   000 01   00 0000 0001 (0x0401: +Norm)
// 0   111 11   00 0000 1111 (0x7c0f: +SNaN)
// 0   111 11   00 1111 0000 (0x7c0f: +QNaN)


// Generate and return 16-bit floats and their corresponding 32-bit values.
//
// The first 14 number pairs are manually picked, while the rest are randomly generated.
// Expected count to be at least 14 (numPicks).
vector<deFloat16> getFloat16s (de::Random& rnd, deUint32 count)
{
	vector<deFloat16>	float16;

	float16.reserve(count);

	// Zero
	float16.push_back(deUint16(0x0000));
	float16.push_back(deUint16(0x8000));
	// Infinity
	float16.push_back(deUint16(0x7c00));
	float16.push_back(deUint16(0xfc00));
	// SNaN
	float16.push_back(deUint16(0x7c0f));
	float16.push_back(deUint16(0xfc0f));
	// QNaN
	float16.push_back(deUint16(0x7cf0));
	float16.push_back(deUint16(0xfcf0));

	// Denormalized
	float16.push_back(deUint16(0x03f0));
	float16.push_back(deUint16(0x83f0));
	// Normalized
	float16.push_back(deUint16(0x0401));
	float16.push_back(deUint16(0x8401));
	// Some normal number
	float16.push_back(deUint16(0x14cb));
	float16.push_back(deUint16(0x94cb));

	const deUint32		numPicks	= static_cast<deUint32>(float16.size());

	DE_ASSERT(count >= numPicks);
	count -= numPicks;

	for (deUint32 numIdx = 0; numIdx < count; ++numIdx)
		float16.push_back(rnd.getUint16());

	return float16;
}

void addCompute16bitStorageUniform16To32Group (tcu::TestCaseGroup* group)
{
	tcu::TestContext&				testCtx			= group->getTestContext();
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 128;

	const StringTemplate			shaderTemplate	(
		"OpCapability Shader\n"
		"OpCapability ${capability}\n"
		"OpExtension \"SPV_KHR_16bit_storage\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"
		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"${stride}"

		"OpMemberDecorate %SSBO32 0 Offset 0\n"
		"OpMemberDecorate %SSBO16 0 Offset 0\n"
		"OpDecorate %SSBO32 BufferBlock\n"
		"OpDecorate %SSBO16 ${storage}\n"
		"OpDecorate %ssbo32 DescriptorSet 0\n"
		"OpDecorate %ssbo16 DescriptorSet 0\n"
		"OpDecorate %ssbo32 Binding 1\n"
		"OpDecorate %ssbo16 Binding 0\n"

		"${matrix_decor:opt}\n"

		"%bool      = OpTypeBool\n"
		"%void      = OpTypeVoid\n"
		"%voidf     = OpTypeFunction %void\n"
		"%u32       = OpTypeInt 32 0\n"
		"%i32       = OpTypeInt 32 1\n"
		"%f32       = OpTypeFloat 32\n"
		"%uvec3     = OpTypeVector %u32 3\n"
		"%fvec3     = OpTypeVector %f32 3\n"
		"%uvec3ptr  = OpTypePointer Input %uvec3\n"
		"%i32ptr    = OpTypePointer Uniform %i32\n"
		"%f32ptr    = OpTypePointer Uniform %f32\n"

		"%zero      = OpConstant %i32 0\n"
		"%c_i32_1   = OpConstant %i32 1\n"
		"%c_i32_2   = OpConstant %i32 2\n"
		"%c_i32_3   = OpConstant %i32 3\n"
		"%c_i32_16  = OpConstant %i32 16\n"
		"%c_i32_32  = OpConstant %i32 32\n"
		"%c_i32_64  = OpConstant %i32 64\n"
		"%c_i32_128 = OpConstant %i32 128\n"

		"%i32arr    = OpTypeArray %i32 %c_i32_128\n"
		"%f32arr    = OpTypeArray %f32 %c_i32_128\n"

		"${types}\n"
		"${matrix_types:opt}\n"

		"%SSBO32    = OpTypeStruct %${matrix_prefix:opt}${base32}arr\n"
		"%SSBO16    = OpTypeStruct %${matrix_prefix:opt}${base16}arr\n"
		"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
		"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
		"%ssbo32    = OpVariable %up_SSBO32 Uniform\n"
		"%ssbo16    = OpVariable %up_SSBO16 Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %${base16}ptr %ssbo16 %zero %x ${index0:opt}\n"
		"%val16     = OpLoad %${base16} %inloc\n"
		"%val32     = ${convert} %${base32} %val16\n"
		"%outloc    = OpAccessChain %${base32}ptr %ssbo32 %zero %x ${index0:opt}\n"
		"             OpStore %outloc %val32\n"
		"${matrix_store:opt}\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	{  // floats
		const char										floatTypes[]	=
			"%f16       = OpTypeFloat 16\n"
			"%f16ptr    = OpTypePointer Uniform %f16\n"
			"%f16arr    = OpTypeArray %f16 %c_i32_128\n"
			"%v2f16     = OpTypeVector %f16 2\n"
			"%v2f32     = OpTypeVector %f32 2\n"
			"%v2f16ptr  = OpTypePointer Uniform %v2f16\n"
			"%v2f32ptr  = OpTypePointer Uniform %v2f32\n"
			"%v2f16arr  = OpTypeArray %v2f16 %c_i32_64\n"
			"%v2f32arr  = OpTypeArray %v2f32 %c_i32_64\n";

		struct CompositeType
		{
			const char*	name;
			const char*	base32;
			const char*	base16;
			const char*	stride;
			unsigned	count;
		};

		const CompositeType	cTypes[]	=
		{
			{"scalar",	"f32",		"f16",		"OpDecorate %f32arr ArrayStride 4\nOpDecorate %f16arr ArrayStride 2\n",				numElements},
			{"vector",	"v2f32",	"v2f16",	"OpDecorate %v2f32arr ArrayStride 8\nOpDecorate %v2f16arr ArrayStride 4\n",			numElements / 2},
			{"matrix",	"v2f32",	"v2f16",	"OpDecorate %m4v2f32arr ArrayStride 32\nOpDecorate %m4v2f16arr ArrayStride 16\n",	numElements / 8},
		};

		vector<deFloat16>	float16Data			= getFloat16s(rnd, numElements);
		vector<float>		float32Data;

		float32Data.reserve(numElements);
		for (deUint32 numIdx = 0; numIdx < numElements; ++numIdx)
			float32Data.push_back(deFloat16To32(float16Data[numIdx]));

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 tyIdx = 0; tyIdx < DE_LENGTH_OF_ARRAY(cTypes); ++tyIdx)
			{
				ComputeShaderSpec		spec;
				map<string, string>		specs;
				string					testName	= string(CAPABILITIES[capIdx].name) + "_" + cTypes[tyIdx].name + "_float";

				specs["capability"]		= CAPABILITIES[capIdx].cap;
				specs["storage"]		= CAPABILITIES[capIdx].decor;
				specs["stride"]			= cTypes[tyIdx].stride;
				specs["base32"]			= cTypes[tyIdx].base32;
				specs["base16"]			= cTypes[tyIdx].base16;
				specs["types"]			= floatTypes;
				specs["convert"]		= "OpFConvert";

				if (strcmp(cTypes[tyIdx].name, "matrix") == 0)
				{
					specs["index0"]			= "%zero";
					specs["matrix_prefix"]	= "m4";
					specs["matrix_types"]	=
						"%m4v2f16 = OpTypeMatrix %v2f16 4\n"
						"%m4v2f32 = OpTypeMatrix %v2f32 4\n"
						"%m4v2f16arr = OpTypeArray %m4v2f16 %c_i32_16\n"
						"%m4v2f32arr = OpTypeArray %m4v2f32 %c_i32_16\n";
					specs["matrix_decor"]	=
						"OpMemberDecorate %SSBO32 0 ColMajor\n"
						"OpMemberDecorate %SSBO32 0 MatrixStride 8\n"
						"OpMemberDecorate %SSBO16 0 ColMajor\n"
						"OpMemberDecorate %SSBO16 0 MatrixStride 4\n";
					specs["matrix_store"]	=
						"%inloc_1  = OpAccessChain %v2f16ptr %ssbo16 %zero %x %c_i32_1\n"
						"%val16_1  = OpLoad %v2f16 %inloc_1\n"
						"%val32_1  = OpFConvert %v2f32 %val16_1\n"
						"%outloc_1 = OpAccessChain %v2f32ptr %ssbo32 %zero %x %c_i32_1\n"
						"            OpStore %outloc_1 %val32_1\n"

						"%inloc_2  = OpAccessChain %v2f16ptr %ssbo16 %zero %x %c_i32_2\n"
						"%val16_2  = OpLoad %v2f16 %inloc_2\n"
						"%val32_2  = OpFConvert %v2f32 %val16_2\n"
						"%outloc_2 = OpAccessChain %v2f32ptr %ssbo32 %zero %x %c_i32_2\n"
						"            OpStore %outloc_2 %val32_2\n"

						"%inloc_3  = OpAccessChain %v2f16ptr %ssbo16 %zero %x %c_i32_3\n"
						"%val16_3  = OpLoad %v2f16 %inloc_3\n"
						"%val32_3  = OpFConvert %v2f32 %val16_3\n"
						"%outloc_3 = OpAccessChain %v2f32ptr %ssbo32 %zero %x %c_i32_3\n"
						"            OpStore %outloc_3 %val32_3\n";
				}

				spec.assembly			= shaderTemplate.specialize(specs);
				spec.numWorkGroups		= IVec3(cTypes[tyIdx].count, 1, 1);
				spec.verifyIO			= check32BitFloats;
				spec.inputTypes[0]		= CAPABILITIES[capIdx].dtype;

				spec.inputs.push_back(BufferSp(new Float16Buffer(float16Data)));
				spec.outputs.push_back(BufferSp(new Float32Buffer(float32Data)));
				spec.extensions.push_back("VK_KHR_16bit_storage");
				spec.requestedVulkanFeatures = get16BitStorageFeatures(CAPABILITIES[capIdx].name);

				group->addChild(new SpvAsmComputeShaderCase(testCtx, testName.c_str(), testName.c_str(), spec));
			}
	}

	{  // Integers
		const char		sintTypes[]		=
			"%i16       = OpTypeInt 16 1\n"
			"%i16ptr    = OpTypePointer Uniform %i16\n"
			"%i16arr    = OpTypeArray %i16 %c_i32_128\n"
			"%v4i16     = OpTypeVector %i16 4\n"
			"%v4i32     = OpTypeVector %i32 4\n"
			"%v4i16ptr  = OpTypePointer Uniform %v4i16\n"
			"%v4i32ptr  = OpTypePointer Uniform %v4i32\n"
			"%v4i16arr  = OpTypeArray %v4i16 %c_i32_32\n"
			"%v4i32arr  = OpTypeArray %v4i32 %c_i32_32\n";

		const char		uintTypes[]		=
			"%u16       = OpTypeInt 16 0\n"
			"%u16ptr    = OpTypePointer Uniform %u16\n"
			"%u32ptr    = OpTypePointer Uniform %u32\n"
			"%u16arr    = OpTypeArray %u16 %c_i32_128\n"
			"%u32arr    = OpTypeArray %u32 %c_i32_128\n"
			"%v4u16     = OpTypeVector %u16 4\n"
			"%v4u32     = OpTypeVector %u32 4\n"
			"%v4u16ptr  = OpTypePointer Uniform %v4u16\n"
			"%v4u32ptr  = OpTypePointer Uniform %v4u32\n"
			"%v4u16arr  = OpTypeArray %v4u16 %c_i32_32\n"
			"%v4u32arr  = OpTypeArray %v4u32 %c_i32_32\n";

		struct CompositeType
		{
			const char*	name;
			bool		isSigned;
			const char* types;
			const char*	base32;
			const char*	base16;
			const char* opcode;
			const char*	stride;
			unsigned	count;
		};

		const CompositeType	cTypes[]	=
		{
			{"scalar_sint",	true,	sintTypes,	"i32",		"i16",		"OpSConvert",	"OpDecorate %i32arr ArrayStride 4\nOpDecorate %i16arr ArrayStride 2\n",			numElements},
			{"scalar_uint",	false,	uintTypes,	"u32",		"u16",		"OpUConvert",	"OpDecorate %u32arr ArrayStride 4\nOpDecorate %u16arr ArrayStride 2\n",			numElements},
			{"vector_sint",	true,	sintTypes,	"v4i32",	"v4i16",	"OpSConvert",	"OpDecorate %v4i32arr ArrayStride 16\nOpDecorate %v4i16arr ArrayStride 8\n",	numElements / 4},
			{"vector_uint",	false,	uintTypes,	"v4u32",	"v4u16",	"OpUConvert",	"OpDecorate %v4u32arr ArrayStride 16\nOpDecorate %v4u16arr ArrayStride 8\n",	numElements / 4},
		};

		vector<deInt16>	inputs			= getInt16s(rnd, numElements);
		vector<deInt32> sOutputs;
		vector<deInt32> uOutputs;
		const deUint16	signBitMask		= 0x8000;
		const deUint32	signExtendMask	= 0xffff0000;

		sOutputs.reserve(inputs.size());
		uOutputs.reserve(inputs.size());

		for (deUint32 numNdx = 0; numNdx < inputs.size(); ++numNdx)
		{
			uOutputs.push_back(static_cast<deUint16>(inputs[numNdx]));
			if (inputs[numNdx] & signBitMask)
				sOutputs.push_back(static_cast<deInt32>(inputs[numNdx] | signExtendMask));
			else
				sOutputs.push_back(static_cast<deInt32>(inputs[numNdx]));
		}

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 tyIdx = 0; tyIdx < DE_LENGTH_OF_ARRAY(cTypes); ++tyIdx)
			{
				ComputeShaderSpec		spec;
				map<string, string>		specs;
				string					testName	= string(CAPABILITIES[capIdx].name) + "_" + cTypes[tyIdx].name;

				specs["capability"]		= CAPABILITIES[capIdx].cap;
				specs["storage"]		= CAPABILITIES[capIdx].decor;
				specs["stride"]			= cTypes[tyIdx].stride;
				specs["base32"]			= cTypes[tyIdx].base32;
				specs["base16"]			= cTypes[tyIdx].base16;
				specs["types"]			= cTypes[tyIdx].types;
				specs["convert"]		= cTypes[tyIdx].opcode;

				spec.assembly			= shaderTemplate.specialize(specs);
				spec.numWorkGroups		= IVec3(cTypes[tyIdx].count, 1, 1);
				spec.inputTypes[0]		= CAPABILITIES[capIdx].dtype;

				spec.inputs.push_back(BufferSp(new Int16Buffer(inputs)));
				if (cTypes[tyIdx].isSigned)
					spec.outputs.push_back(BufferSp(new Int32Buffer(sOutputs)));
				else
					spec.outputs.push_back(BufferSp(new Int32Buffer(uOutputs)));
				spec.extensions.push_back("VK_KHR_16bit_storage");
				spec.requestedVulkanFeatures = get16BitStorageFeatures(CAPABILITIES[capIdx].name);

				group->addChild(new SpvAsmComputeShaderCase(testCtx, testName.c_str(), testName.c_str(), spec));
			}
	}
}

void addCompute16bitStoragePushConstant16To32Group (tcu::TestCaseGroup* group)
{
	tcu::TestContext&				testCtx			= group->getTestContext();
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 64;

	const StringTemplate			shaderTemplate	(
		"OpCapability Shader\n"
		"OpCapability StoragePushConstant16\n"
		"OpExtension \"SPV_KHR_16bit_storage\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"
		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"${stride}"

		"OpDecorate %PC16 Block\n"
		"OpMemberDecorate %PC16 0 Offset 0\n"
		"OpMemberDecorate %SSBO32 0 Offset 0\n"
		"OpDecorate %SSBO32 BufferBlock\n"
		"OpDecorate %ssbo32 DescriptorSet 0\n"
		"OpDecorate %ssbo32 Binding 0\n"

		"${matrix_decor:opt}\n"

		"%bool      = OpTypeBool\n"
		"%void      = OpTypeVoid\n"
		"%voidf     = OpTypeFunction %void\n"
		"%u32       = OpTypeInt 32 0\n"
		"%i32       = OpTypeInt 32 1\n"
		"%f32       = OpTypeFloat 32\n"
		"%uvec3     = OpTypeVector %u32 3\n"
		"%fvec3     = OpTypeVector %f32 3\n"
		"%uvec3ptr  = OpTypePointer Input %uvec3\n"
		"%i32ptr    = OpTypePointer Uniform %i32\n"
		"%f32ptr    = OpTypePointer Uniform %f32\n"

		"%zero      = OpConstant %i32 0\n"
		"%c_i32_1   = OpConstant %i32 1\n"
		"%c_i32_8   = OpConstant %i32 8\n"
		"%c_i32_16  = OpConstant %i32 16\n"
		"%c_i32_32  = OpConstant %i32 32\n"
		"%c_i32_64  = OpConstant %i32 64\n"

		"%i32arr    = OpTypeArray %i32 %c_i32_64\n"
		"%f32arr    = OpTypeArray %f32 %c_i32_64\n"

		"${types}\n"
		"${matrix_types:opt}\n"

		"%PC16      = OpTypeStruct %${matrix_prefix:opt}${base16}arr\n"
		"%pp_PC16   = OpTypePointer PushConstant %PC16\n"
		"%pc16      = OpVariable %pp_PC16 PushConstant\n"
		"%SSBO32    = OpTypeStruct %${matrix_prefix:opt}${base32}arr\n"
		"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
		"%ssbo32    = OpVariable %up_SSBO32 Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %${base16}ptr %pc16 %zero %x ${index0:opt}\n"
		"%val16     = OpLoad %${base16} %inloc\n"
		"%val32     = ${convert} %${base32} %val16\n"
		"%outloc    = OpAccessChain %${base32}ptr %ssbo32 %zero %x ${index0:opt}\n"
		"             OpStore %outloc %val32\n"
		"${matrix_store:opt}\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	{  // floats
		const char										floatTypes[]	=
			"%f16       = OpTypeFloat 16\n"
			"%f16ptr    = OpTypePointer PushConstant %f16\n"
			"%f16arr    = OpTypeArray %f16 %c_i32_64\n"
			"%v4f16     = OpTypeVector %f16 4\n"
			"%v4f32     = OpTypeVector %f32 4\n"
			"%v4f16ptr  = OpTypePointer PushConstant %v4f16\n"
			"%v4f32ptr  = OpTypePointer Uniform %v4f32\n"
			"%v4f16arr  = OpTypeArray %v4f16 %c_i32_16\n"
			"%v4f32arr  = OpTypeArray %v4f32 %c_i32_16\n";

		struct CompositeType
		{
			const char*	name;
			const char*	base32;
			const char*	base16;
			const char*	stride;
			unsigned	count;
		};

		const CompositeType	cTypes[]	=
		{
			{"scalar",	"f32",		"f16",		"OpDecorate %f32arr ArrayStride 4\nOpDecorate %f16arr ArrayStride 2\n",				numElements},
			{"vector",	"v4f32",	"v4f16",	"OpDecorate %v4f32arr ArrayStride 16\nOpDecorate %v4f16arr ArrayStride 8\n",		numElements / 4},
			{"matrix",	"v4f32",	"v4f16",	"OpDecorate %m2v4f32arr ArrayStride 32\nOpDecorate %m2v4f16arr ArrayStride 16\n",	numElements / 8},
		};

		vector<deFloat16>	float16Data			= getFloat16s(rnd, numElements);
		vector<float>		float32Data;

		float32Data.reserve(numElements);
		for (deUint32 numIdx = 0; numIdx < numElements; ++numIdx)
			float32Data.push_back(deFloat16To32(float16Data[numIdx]));

		for (deUint32 tyIdx = 0; tyIdx < DE_LENGTH_OF_ARRAY(cTypes); ++tyIdx)
		{
			ComputeShaderSpec		spec;
			map<string, string>		specs;
			string					testName	= string(cTypes[tyIdx].name) + "_float";

			specs["stride"]			= cTypes[tyIdx].stride;
			specs["base32"]			= cTypes[tyIdx].base32;
			specs["base16"]			= cTypes[tyIdx].base16;
			specs["types"]			= floatTypes;
			specs["convert"]		= "OpFConvert";

			if (strcmp(cTypes[tyIdx].name, "matrix") == 0)
			{
				specs["index0"]			= "%zero";
				specs["matrix_prefix"]	= "m2";
				specs["matrix_types"]	=
					"%m2v4f16 = OpTypeMatrix %v4f16 2\n"
					"%m2v4f32 = OpTypeMatrix %v4f32 2\n"
					"%m2v4f16arr = OpTypeArray %m2v4f16 %c_i32_8\n"
					"%m2v4f32arr = OpTypeArray %m2v4f32 %c_i32_8\n";
				specs["matrix_decor"]	=
					"OpMemberDecorate %SSBO32 0 ColMajor\n"
					"OpMemberDecorate %SSBO32 0 MatrixStride 16\n"
					"OpMemberDecorate %PC16 0 ColMajor\n"
					"OpMemberDecorate %PC16 0 MatrixStride 8\n";
				specs["matrix_store"]	=
					"%inloc_1  = OpAccessChain %v4f16ptr %pc16 %zero %x %c_i32_1\n"
					"%val16_1  = OpLoad %v4f16 %inloc_1\n"
					"%val32_1  = OpFConvert %v4f32 %val16_1\n"
					"%outloc_1 = OpAccessChain %v4f32ptr %ssbo32 %zero %x %c_i32_1\n"
					"            OpStore %outloc_1 %val32_1\n";
			}

			spec.assembly			= shaderTemplate.specialize(specs);
			spec.numWorkGroups		= IVec3(cTypes[tyIdx].count, 1, 1);
			spec.verifyIO			= check32BitFloats;
			spec.pushConstants		= BufferSp(new Float16Buffer(float16Data));

			spec.outputs.push_back(BufferSp(new Float32Buffer(float32Data)));
			spec.extensions.push_back("VK_KHR_16bit_storage");
			spec.requestedVulkanFeatures.ext16BitStorage = EXT16BITSTORAGEFEATURES_PUSH_CONSTANT;

			group->addChild(new SpvAsmComputeShaderCase(testCtx, testName.c_str(), testName.c_str(), spec));
		}
	}
	{  // integers
		const char		sintTypes[]		=
			"%i16       = OpTypeInt 16 1\n"
			"%i16ptr    = OpTypePointer PushConstant %i16\n"
			"%i16arr    = OpTypeArray %i16 %c_i32_64\n"
			"%v2i16     = OpTypeVector %i16 2\n"
			"%v2i32     = OpTypeVector %i32 2\n"
			"%v2i16ptr  = OpTypePointer PushConstant %v2i16\n"
			"%v2i32ptr  = OpTypePointer Uniform %v2i32\n"
			"%v2i16arr  = OpTypeArray %v2i16 %c_i32_32\n"
			"%v2i32arr  = OpTypeArray %v2i32 %c_i32_32\n";

		const char		uintTypes[]		=
			"%u16       = OpTypeInt 16 0\n"
			"%u16ptr    = OpTypePointer PushConstant %u16\n"
			"%u32ptr    = OpTypePointer Uniform %u32\n"
			"%u16arr    = OpTypeArray %u16 %c_i32_64\n"
			"%u32arr    = OpTypeArray %u32 %c_i32_64\n"
			"%v2u16     = OpTypeVector %u16 2\n"
			"%v2u32     = OpTypeVector %u32 2\n"
			"%v2u16ptr  = OpTypePointer PushConstant %v2u16\n"
			"%v2u32ptr  = OpTypePointer Uniform %v2u32\n"
			"%v2u16arr  = OpTypeArray %v2u16 %c_i32_32\n"
			"%v2u32arr  = OpTypeArray %v2u32 %c_i32_32\n";

		struct CompositeType
		{
			const char*	name;
			bool		isSigned;
			const char* types;
			const char*	base32;
			const char*	base16;
			const char* opcode;
			const char*	stride;
			unsigned	count;
		};

		const CompositeType	cTypes[]	=
		{
			{"scalar_sint",	true,	sintTypes,	"i32",		"i16",		"OpSConvert",	"OpDecorate %i32arr ArrayStride 4\nOpDecorate %i16arr ArrayStride 2\n",		numElements},
			{"scalar_uint",	false,	uintTypes,	"u32",		"u16",		"OpUConvert",	"OpDecorate %u32arr ArrayStride 4\nOpDecorate %u16arr ArrayStride 2\n",		numElements},
			{"vector_sint",	true,	sintTypes,	"v2i32",	"v2i16",	"OpSConvert",	"OpDecorate %v2i32arr ArrayStride 8\nOpDecorate %v2i16arr ArrayStride 4\n",	numElements / 2},
			{"vector_uint",	false,	uintTypes,	"v2u32",	"v2u16",	"OpUConvert",	"OpDecorate %v2u32arr ArrayStride 8\nOpDecorate %v2u16arr ArrayStride 4\n",	numElements / 2},
		};

		vector<deInt16>	inputs			= getInt16s(rnd, numElements);
		vector<deInt32> sOutputs;
		vector<deInt32> uOutputs;
		const deUint16	signBitMask		= 0x8000;
		const deUint32	signExtendMask	= 0xffff0000;

		sOutputs.reserve(inputs.size());
		uOutputs.reserve(inputs.size());

		for (deUint32 numNdx = 0; numNdx < inputs.size(); ++numNdx)
		{
			uOutputs.push_back(static_cast<deUint16>(inputs[numNdx]));
			if (inputs[numNdx] & signBitMask)
				sOutputs.push_back(static_cast<deInt32>(inputs[numNdx] | signExtendMask));
			else
				sOutputs.push_back(static_cast<deInt32>(inputs[numNdx]));
		}

		for (deUint32 tyIdx = 0; tyIdx < DE_LENGTH_OF_ARRAY(cTypes); ++tyIdx)
		{
			ComputeShaderSpec		spec;
			map<string, string>		specs;
			const char*				testName	= cTypes[tyIdx].name;

			specs["stride"]			= cTypes[tyIdx].stride;
			specs["base32"]			= cTypes[tyIdx].base32;
			specs["base16"]			= cTypes[tyIdx].base16;
			specs["types"]			= cTypes[tyIdx].types;
			specs["convert"]		= cTypes[tyIdx].opcode;

			spec.assembly			= shaderTemplate.specialize(specs);
			spec.numWorkGroups		= IVec3(cTypes[tyIdx].count, 1, 1);
			spec.pushConstants		= BufferSp(new Int16Buffer(inputs));

			if (cTypes[tyIdx].isSigned)
				spec.outputs.push_back(BufferSp(new Int32Buffer(sOutputs)));
			else
				spec.outputs.push_back(BufferSp(new Int32Buffer(uOutputs)));
			spec.extensions.push_back("VK_KHR_16bit_storage");
			spec.requestedVulkanFeatures.ext16BitStorage = EXT16BITSTORAGEFEATURES_PUSH_CONSTANT;

			group->addChild(new SpvAsmComputeShaderCase(testCtx, testName, testName, spec));
		}
	}
}

void addGraphics16BitStorageUniformInt32To16Group (tcu::TestCaseGroup* testGroup)
{
	de::Random							rnd					(deStringHash(testGroup->getName()));
	map<string, string>					fragments;
	const deUint32						numDataPoints		= 256;
	RGBA								defaultColors[4];
	GraphicsResources					resources;
	vector<string>						extensions;
	const StringTemplate				capabilities		("OpCapability ${cap}\n");
	// inputs and outputs are declared to be vectors of signed integers.
	// However, depending on the test, they may be interpreted as unsiged
	// integers. That won't be a problem as long as we passed the bits
	// in faithfully to the pipeline.
	vector<deInt32>						inputs				= getInt32s(rnd, numDataPoints);
	vector<deInt16>						outputs;

	outputs.reserve(inputs.size());
	for (deUint32 numNdx = 0; numNdx < inputs.size(); ++numNdx)
		outputs.push_back(static_cast<deInt16>(0xffff & inputs[numNdx]));

	resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int32Buffer(inputs))));
	resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int16Buffer(outputs))));

	extensions.push_back("VK_KHR_16bit_storage");
	fragments["extension"]	= "OpExtension \"SPV_KHR_16bit_storage\"";

	getDefaultColors(defaultColors);

	struct IntegerFacts
	{
		const char*	name;
		const char*	type32;
		const char*	type16;
		const char* opcode;
		const char*	isSigned;
	};

	const IntegerFacts	intFacts[]		=
	{
		{"sint",	"%i32",		"%i16",		"OpSConvert",	"1"},
		{"uint",	"%u32",		"%u16",		"OpUConvert",	"0"},
	};

	const StringTemplate	scalarPreMain(
			"${itype16} = OpTypeInt 16 ${signed}\n"
			"%c_i32_256 = OpConstant %i32 256\n"
			"   %up_i32 = OpTypePointer Uniform ${itype32}\n"
			"   %up_i16 = OpTypePointer Uniform ${itype16}\n"
			"   %ra_i32 = OpTypeArray ${itype32} %c_i32_256\n"
			"   %ra_i16 = OpTypeArray ${itype16} %c_i32_256\n"
			"   %SSBO32 = OpTypeStruct %ra_i32\n"
			"   %SSBO16 = OpTypeStruct %ra_i16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n");

	const StringTemplate	scalarDecoration(
			"OpDecorate %ra_i32 ArrayStride 4\n"
			"OpDecorate %ra_i16 ArrayStride 2\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpDecorate %SSBO32 ${indecor}\n"
			"OpDecorate %SSBO16 BufferBlock\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n"
			"OpDecorate %ssbo16 Binding 1\n");

	const StringTemplate	scalarTestFunc(
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_256\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"%write = OpLabel\n"
			"   %30 = OpLoad %i32 %i\n"
			"  %src = OpAccessChain %up_i32 %ssbo32 %c_i32_0 %30\n"
			"%val32 = OpLoad ${itype32} %src\n"
			"%val16 = ${convert} ${itype16} %val32\n"
			"  %dst = OpAccessChain %up_i16 %ssbo16 %c_i32_0 %30\n"
			"         OpStore %dst %val16\n"
			"         OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"

			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n");

	const StringTemplate	vecPreMain(
			"${itype16} = OpTypeInt 16 ${signed}\n"
			" %c_i32_64 = OpConstant %i32 64\n"
			"%v4itype16 = OpTypeVector ${itype16} 4\n"
			" %up_v4i32 = OpTypePointer Uniform ${v4itype32}\n"
			" %up_v4i16 = OpTypePointer Uniform %v4itype16\n"
			" %ra_v4i32 = OpTypeArray ${v4itype32} %c_i32_64\n"
			" %ra_v4i16 = OpTypeArray %v4itype16 %c_i32_64\n"
			"   %SSBO32 = OpTypeStruct %ra_v4i32\n"
			"   %SSBO16 = OpTypeStruct %ra_v4i16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n");

	const StringTemplate	vecDecoration(
			"OpDecorate %ra_v4i32 ArrayStride 16\n"
			"OpDecorate %ra_v4i16 ArrayStride 8\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpDecorate %SSBO32 ${indecor}\n"
			"OpDecorate %SSBO16 BufferBlock\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n"
			"OpDecorate %ssbo16 Binding 1\n");

	const StringTemplate	vecTestFunc(
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_64\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"%write = OpLabel\n"
			"   %30 = OpLoad %i32 %i\n"
			"  %src = OpAccessChain %up_v4i32 %ssbo32 %c_i32_0 %30\n"
			"%val32 = OpLoad ${v4itype32} %src\n"
			"%val16 = ${convert} %v4itype16 %val32\n"
			"  %dst = OpAccessChain %up_v4i16 %ssbo16 %c_i32_0 %30\n"
			"         OpStore %dst %val16\n"
			"         OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"

			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n");

	struct Category
	{
		const char*				name;
		const StringTemplate&	preMain;
		const StringTemplate&	decoration;
		const StringTemplate&	testFunction;
	};

	const Category		categories[]	=
	{
		{"scalar",	scalarPreMain,	scalarDecoration,	scalarTestFunc},
		{"vector",	vecPreMain,		vecDecoration,		vecTestFunc},
	};

	for (deUint32 catIdx = 0; catIdx < DE_LENGTH_OF_ARRAY(categories); ++catIdx)
		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 factIdx = 0; factIdx < DE_LENGTH_OF_ARRAY(intFacts); ++factIdx)
			{
				map<string, string>	specs;
				string				name		= string(CAPABILITIES[capIdx].name) + "_" + categories[catIdx].name + "_" + intFacts[factIdx].name;

				specs["cap"]					= CAPABILITIES[capIdx].cap;
				specs["indecor"]				= CAPABILITIES[capIdx].decor;
				specs["itype32"]				= intFacts[factIdx].type32;
				specs["v4itype32"]				= "%v4" + string(intFacts[factIdx].type32).substr(1);
				specs["itype16"]				= intFacts[factIdx].type16;
				specs["signed"]					= intFacts[factIdx].isSigned;
				specs["convert"]				= intFacts[factIdx].opcode;

				fragments["pre_main"]			= categories[catIdx].preMain.specialize(specs);
				fragments["testfun"]			= categories[catIdx].testFunction.specialize(specs);
				fragments["capability"]			= capabilities.specialize(specs);
				fragments["decoration"]			= categories[catIdx].decoration.specialize(specs);

				resources.inputs.back().first	= CAPABILITIES[capIdx].dtype;

				createTestsForAllStages(name, defaultColors, defaultColors, fragments, resources, extensions, testGroup, get16BitStorageFeatures(CAPABILITIES[capIdx].name));
			}
}

void addCompute16bitStorageUniform32To16Group (tcu::TestCaseGroup* group)
{
	tcu::TestContext&				testCtx			= group->getTestContext();
	de::Random						rnd				(deStringHash(group->getName()));
	const int						numElements		= 128;

	const StringTemplate			shaderTemplate	(
		"OpCapability Shader\n"
		"OpCapability ${capability}\n"
		"OpExtension \"SPV_KHR_16bit_storage\"\n"
		"OpMemoryModel Logical GLSL450\n"
		"OpEntryPoint GLCompute %main \"main\" %id\n"
		"OpExecutionMode %main LocalSize 1 1 1\n"
		"OpDecorate %id BuiltIn GlobalInvocationId\n"

		"${stride}"

		"OpMemberDecorate %SSBO32 0 Offset 0\n"
		"OpMemberDecorate %SSBO16 0 Offset 0\n"
		"OpDecorate %SSBO32 ${storage}\n"
		"OpDecorate %SSBO16 BufferBlock\n"
		"OpDecorate %ssbo32 DescriptorSet 0\n"
		"OpDecorate %ssbo16 DescriptorSet 0\n"
		"OpDecorate %ssbo32 Binding 0\n"
		"OpDecorate %ssbo16 Binding 1\n"

		"${matrix_decor:opt}\n"

		"${rounding:opt}\n"

		"%bool      = OpTypeBool\n"
		"%void      = OpTypeVoid\n"
		"%voidf     = OpTypeFunction %void\n"
		"%u32       = OpTypeInt 32 0\n"
		"%i32       = OpTypeInt 32 1\n"
		"%f32       = OpTypeFloat 32\n"
		"%uvec3     = OpTypeVector %u32 3\n"
		"%fvec3     = OpTypeVector %f32 3\n"
		"%uvec3ptr  = OpTypePointer Input %uvec3\n"
		"%i32ptr    = OpTypePointer Uniform %i32\n"
		"%f32ptr    = OpTypePointer Uniform %f32\n"

		"%zero      = OpConstant %i32 0\n"
		"%c_i32_1   = OpConstant %i32 1\n"
		"%c_i32_16  = OpConstant %i32 16\n"
		"%c_i32_32  = OpConstant %i32 32\n"
		"%c_i32_64  = OpConstant %i32 64\n"
		"%c_i32_128 = OpConstant %i32 128\n"

		"%i32arr    = OpTypeArray %i32 %c_i32_128\n"
		"%f32arr    = OpTypeArray %f32 %c_i32_128\n"

		"${types}\n"
		"${matrix_types:opt}\n"

		"%SSBO32    = OpTypeStruct %${matrix_prefix:opt}${base32}arr\n"
		"%SSBO16    = OpTypeStruct %${matrix_prefix:opt}${base16}arr\n"
		"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
		"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
		"%ssbo32    = OpVariable %up_SSBO32 Uniform\n"
		"%ssbo16    = OpVariable %up_SSBO16 Uniform\n"

		"%id        = OpVariable %uvec3ptr Input\n"

		"%main      = OpFunction %void None %voidf\n"
		"%label     = OpLabel\n"
		"%idval     = OpLoad %uvec3 %id\n"
		"%x         = OpCompositeExtract %u32 %idval 0\n"
		"%inloc     = OpAccessChain %${base32}ptr %ssbo32 %zero %x ${index0:opt}\n"
		"%val32     = OpLoad %${base32} %inloc\n"
		"%val16     = ${convert} %${base16} %val32\n"
		"%outloc    = OpAccessChain %${base16}ptr %ssbo16 %zero %x ${index0:opt}\n"
		"             OpStore %outloc %val16\n"
		"${matrix_store:opt}\n"
		"             OpReturn\n"
		"             OpFunctionEnd\n");

	{  // Floats
		const char						floatTypes[]	=
			"%f16       = OpTypeFloat 16\n"
			"%f16ptr    = OpTypePointer Uniform %f16\n"
			"%f16arr    = OpTypeArray %f16 %c_i32_128\n"
			"%v4f16     = OpTypeVector %f16 4\n"
			"%v4f32     = OpTypeVector %f32 4\n"
			"%v4f16ptr  = OpTypePointer Uniform %v4f16\n"
			"%v4f32ptr  = OpTypePointer Uniform %v4f32\n"
			"%v4f16arr  = OpTypeArray %v4f16 %c_i32_32\n"
			"%v4f32arr  = OpTypeArray %v4f32 %c_i32_32\n";

		struct RndMode
		{
			const char*				name;
			const char*				decor;
			ComputeVerifyIOFunc		func;
		};

		const RndMode		rndModes[]		=
		{
			{"rtz",						"OpDecorate %val16  FPRoundingMode RTZ",	computeCheck16BitFloats<ROUNDINGMODE_RTZ>},
			{"rte",						"OpDecorate %val16  FPRoundingMode RTE",	computeCheck16BitFloats<ROUNDINGMODE_RTE>},
			{"unspecified_rnd_mode",	"",											computeCheck16BitFloats<RoundingModeFlags(ROUNDINGMODE_RTE | ROUNDINGMODE_RTZ)>},
		};

		struct CompositeType
		{
			const char*	name;
			const char*	base32;
			const char*	base16;
			const char*	stride;
			unsigned	count;
		};

		const CompositeType	cTypes[]	=
		{
			{"scalar",	"f32",		"f16",		"OpDecorate %f32arr ArrayStride 4\nOpDecorate %f16arr ArrayStride 2\n",				numElements},
			{"vector",	"v4f32",	"v4f16",	"OpDecorate %v4f32arr ArrayStride 16\nOpDecorate %v4f16arr ArrayStride 8\n",		numElements / 4},
			{"matrix",	"v4f32",	"v4f16",	"OpDecorate %m2v4f32arr ArrayStride 32\nOpDecorate %m2v4f16arr ArrayStride 16\n",	numElements / 8},
		};

		vector<float>		float32Data			= getFloat32s(rnd, numElements);
		vector<deFloat16>	float16DummyData	(numElements, 0);

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 tyIdx = 0; tyIdx < DE_LENGTH_OF_ARRAY(cTypes); ++tyIdx)
				for (deUint32 rndModeIdx = 0; rndModeIdx < DE_LENGTH_OF_ARRAY(rndModes); ++rndModeIdx)
				{
					ComputeShaderSpec		spec;
					map<string, string>		specs;
					string					testName	= string(CAPABILITIES[capIdx].name) + "_" + cTypes[tyIdx].name + "_float_" + rndModes[rndModeIdx].name;

					specs["capability"]		= CAPABILITIES[capIdx].cap;
					specs["storage"]		= CAPABILITIES[capIdx].decor;
					specs["stride"]			= cTypes[tyIdx].stride;
					specs["base32"]			= cTypes[tyIdx].base32;
					specs["base16"]			= cTypes[tyIdx].base16;
					specs["rounding"]		= rndModes[rndModeIdx].decor;
					specs["types"]			= floatTypes;
					specs["convert"]		= "OpFConvert";

					if (strcmp(cTypes[tyIdx].name, "matrix") == 0)
					{
						if (strcmp(rndModes[rndModeIdx].name, "rtz") == 0)
							specs["rounding"] += "\nOpDecorate %val16_1  FPRoundingMode RTZ\n";
						else if (strcmp(rndModes[rndModeIdx].name, "rte") == 0)
							specs["rounding"] += "\nOpDecorate %val16_1  FPRoundingMode RTE\n";

						specs["index0"]			= "%zero";
						specs["matrix_prefix"]	= "m2";
						specs["matrix_types"]	=
							"%m2v4f16 = OpTypeMatrix %v4f16 2\n"
							"%m2v4f32 = OpTypeMatrix %v4f32 2\n"
							"%m2v4f16arr = OpTypeArray %m2v4f16 %c_i32_16\n"
							"%m2v4f32arr = OpTypeArray %m2v4f32 %c_i32_16\n";
						specs["matrix_decor"]	=
							"OpMemberDecorate %SSBO32 0 ColMajor\n"
							"OpMemberDecorate %SSBO32 0 MatrixStride 16\n"
							"OpMemberDecorate %SSBO16 0 ColMajor\n"
							"OpMemberDecorate %SSBO16 0 MatrixStride 8\n";
						specs["matrix_store"]	=
							"%inloc_1  = OpAccessChain %v4f32ptr %ssbo32 %zero %x %c_i32_1\n"
							"%val32_1  = OpLoad %v4f32 %inloc_1\n"
							"%val16_1  = OpFConvert %v4f16 %val32_1\n"
							"%outloc_1 = OpAccessChain %v4f16ptr %ssbo16 %zero %x %c_i32_1\n"
							"            OpStore %outloc_1 %val16_1\n";
					}

					spec.assembly			= shaderTemplate.specialize(specs);
					spec.numWorkGroups		= IVec3(cTypes[tyIdx].count, 1, 1);
					spec.verifyIO			= rndModes[rndModeIdx].func;
					spec.inputTypes[0]		= CAPABILITIES[capIdx].dtype;

					spec.inputs.push_back(BufferSp(new Float32Buffer(float32Data)));
					// We provided a custom verifyIO in the above in which inputs will be used for checking.
					// So put dummy data in the expected values.
					spec.outputs.push_back(BufferSp(new Float16Buffer(float16DummyData)));
					spec.extensions.push_back("VK_KHR_16bit_storage");
					spec.requestedVulkanFeatures = get16BitStorageFeatures(CAPABILITIES[capIdx].name);

					group->addChild(new SpvAsmComputeShaderCase(testCtx, testName.c_str(), testName.c_str(), spec));
				}
	}

	{  // Integers
		const char		sintTypes[]	=
			"%i16       = OpTypeInt 16 1\n"
			"%i16ptr    = OpTypePointer Uniform %i16\n"
			"%i16arr    = OpTypeArray %i16 %c_i32_128\n"
			"%v2i16     = OpTypeVector %i16 2\n"
			"%v2i32     = OpTypeVector %i32 2\n"
			"%v2i16ptr  = OpTypePointer Uniform %v2i16\n"
			"%v2i32ptr  = OpTypePointer Uniform %v2i32\n"
			"%v2i16arr  = OpTypeArray %v2i16 %c_i32_64\n"
			"%v2i32arr  = OpTypeArray %v2i32 %c_i32_64\n";

		const char		uintTypes[]	=
			"%u16       = OpTypeInt 16 0\n"
			"%u16ptr    = OpTypePointer Uniform %u16\n"
			"%u32ptr    = OpTypePointer Uniform %u32\n"
			"%u16arr    = OpTypeArray %u16 %c_i32_128\n"
			"%u32arr    = OpTypeArray %u32 %c_i32_128\n"
			"%v2u16     = OpTypeVector %u16 2\n"
			"%v2u32     = OpTypeVector %u32 2\n"
			"%v2u16ptr  = OpTypePointer Uniform %v2u16\n"
			"%v2u32ptr  = OpTypePointer Uniform %v2u32\n"
			"%v2u16arr  = OpTypeArray %v2u16 %c_i32_64\n"
			"%v2u32arr  = OpTypeArray %v2u32 %c_i32_64\n";

		struct CompositeType
		{
			const char*	name;
			const char* types;
			const char*	base32;
			const char*	base16;
			const char* opcode;
			const char*	stride;
			unsigned	count;
		};

		const CompositeType	cTypes[]	=
		{
			{"scalar_sint",	sintTypes,	"i32",		"i16",		"OpSConvert",	"OpDecorate %i32arr ArrayStride 4\nOpDecorate %i16arr ArrayStride 2\n",		numElements},
			{"scalar_uint",	uintTypes,	"u32",		"u16",		"OpUConvert",	"OpDecorate %u32arr ArrayStride 4\nOpDecorate %u16arr ArrayStride 2\n",		numElements},
			{"vector_sint",	sintTypes,	"v2i32",	"v2i16",	"OpSConvert",	"OpDecorate %v2i32arr ArrayStride 8\nOpDecorate %v2i16arr ArrayStride 4\n",	numElements / 2},
			{"vector_uint",	uintTypes,	"v2u32",	"v2u16",	"OpUConvert",	"OpDecorate %v2u32arr ArrayStride 8\nOpDecorate %v2u16arr ArrayStride 4\n",	numElements / 2},
		};

		vector<deInt32>	inputs			= getInt32s(rnd, numElements);
		vector<deInt16> outputs;

		outputs.reserve(inputs.size());
		for (deUint32 numNdx = 0; numNdx < inputs.size(); ++numNdx)
			outputs.push_back(static_cast<deInt16>(0xffff & inputs[numNdx]));

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 tyIdx = 0; tyIdx < DE_LENGTH_OF_ARRAY(cTypes); ++tyIdx)
			{
				ComputeShaderSpec		spec;
				map<string, string>		specs;
				string					testName	= string(CAPABILITIES[capIdx].name) + "_" + cTypes[tyIdx].name;

				specs["capability"]		= CAPABILITIES[capIdx].cap;
				specs["storage"]		= CAPABILITIES[capIdx].decor;
				specs["stride"]			= cTypes[tyIdx].stride;
				specs["base32"]			= cTypes[tyIdx].base32;
				specs["base16"]			= cTypes[tyIdx].base16;
				specs["types"]			= cTypes[tyIdx].types;
				specs["convert"]		= cTypes[tyIdx].opcode;

				spec.assembly			= shaderTemplate.specialize(specs);
				spec.numWorkGroups		= IVec3(cTypes[tyIdx].count, 1, 1);
				spec.inputTypes[0]		= CAPABILITIES[capIdx].dtype;

				spec.inputs.push_back(BufferSp(new Int32Buffer(inputs)));
				spec.outputs.push_back(BufferSp(new Int16Buffer(outputs)));
				spec.extensions.push_back("VK_KHR_16bit_storage");
				spec.requestedVulkanFeatures = get16BitStorageFeatures(CAPABILITIES[capIdx].name);

				group->addChild(new SpvAsmComputeShaderCase(testCtx, testName.c_str(), testName.c_str(), spec));
			}
	}
}

void addGraphics16BitStorageUniformFloat32To16Group (tcu::TestCaseGroup* testGroup)
{
	de::Random							rnd					(deStringHash(testGroup->getName()));
	map<string, string>					fragments;
	GraphicsResources					resources;
	vector<string>						extensions;
	const deUint32						numDataPoints		= 256;
	RGBA								defaultColors[4];
	vector<float>						float32Data			= getFloat32s(rnd, numDataPoints);
	vector<deFloat16>					float16DummyData	(numDataPoints, 0);
	const StringTemplate				capabilities		("OpCapability ${cap}\n");

	resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(float32Data))));
	// We use a custom verifyIO to check the result via computing directly from inputs; the contents in outputs do not matter.
	resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float16Buffer(float16DummyData))));

	extensions.push_back("VK_KHR_16bit_storage");
	fragments["extension"]	= "OpExtension \"SPV_KHR_16bit_storage\"";

	struct RndMode
	{
		const char*				name;
		const char*				decor;
		GraphicsVerifyIOFunc	f;
	};

	getDefaultColors(defaultColors);

	{  // scalar cases
		fragments["pre_main"]				=
			"      %f16 = OpTypeFloat 16\n"
			"%c_i32_256 = OpConstant %i32 256\n"
			"   %up_f32 = OpTypePointer Uniform %f32\n"
			"   %up_f16 = OpTypePointer Uniform %f16\n"
			"   %ra_f32 = OpTypeArray %f32 %c_i32_256\n"
			"   %ra_f16 = OpTypeArray %f16 %c_i32_256\n"
			"   %SSBO32 = OpTypeStruct %ra_f32\n"
			"   %SSBO16 = OpTypeStruct %ra_f16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n";

		const StringTemplate decoration		(
			"OpDecorate %ra_f32 ArrayStride 4\n"
			"OpDecorate %ra_f16 ArrayStride 2\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpDecorate %SSBO32 ${indecor}\n"
			"OpDecorate %SSBO16 BufferBlock\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n"
			"OpDecorate %ssbo16 Binding 1\n"
			"${rounddecor}\n");

		fragments["testfun"]				=
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_256\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"%write = OpLabel\n"
			"   %30 = OpLoad %i32 %i\n"
			"  %src = OpAccessChain %up_f32 %ssbo32 %c_i32_0 %30\n"
			"%val32 = OpLoad %f32 %src\n"
			"%val16 = OpFConvert %f16 %val32\n"
			"  %dst = OpAccessChain %up_f16 %ssbo16 %c_i32_0 %30\n"
			"         OpStore %dst %val16\n"
			"         OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"

			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n";

		const RndMode	rndModes[] =
		{
			{"rtz",						"OpDecorate %val16  FPRoundingMode RTZ",	graphicsCheck16BitFloats<ROUNDINGMODE_RTZ>},
			{"rte",						"OpDecorate %val16  FPRoundingMode RTE",	graphicsCheck16BitFloats<ROUNDINGMODE_RTE>},
			{"unspecified_rnd_mode",	"",											graphicsCheck16BitFloats<RoundingModeFlags(ROUNDINGMODE_RTE | ROUNDINGMODE_RTZ)>},
		};

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 rndModeIdx = 0; rndModeIdx < DE_LENGTH_OF_ARRAY(rndModes); ++rndModeIdx)
			{
				map<string, string>	specs;
				string				testName	= string(CAPABILITIES[capIdx].name) + "_scalar_float_" + rndModes[rndModeIdx].name;

				specs["cap"]					= CAPABILITIES[capIdx].cap;
				specs["indecor"]				= CAPABILITIES[capIdx].decor;
				specs["rounddecor"]				= rndModes[rndModeIdx].decor;

				fragments["capability"]			= capabilities.specialize(specs);
				fragments["decoration"]			= decoration.specialize(specs);

				resources.inputs.back().first	= CAPABILITIES[capIdx].dtype;
				resources.verifyIO				= rndModes[rndModeIdx].f;


				createTestsForAllStages(testName, defaultColors, defaultColors, fragments, resources, extensions, testGroup, get16BitStorageFeatures(CAPABILITIES[capIdx].name));
			}
	}

	{  // vector cases
		fragments["pre_main"]				=
			"      %f16 = OpTypeFloat 16\n"
			" %c_i32_64 = OpConstant %i32 64\n"
			"	 %v4f16 = OpTypeVector %f16 4\n"
			" %up_v4f32 = OpTypePointer Uniform %v4f32\n"
			" %up_v4f16 = OpTypePointer Uniform %v4f16\n"
			" %ra_v4f32 = OpTypeArray %v4f32 %c_i32_64\n"
			" %ra_v4f16 = OpTypeArray %v4f16 %c_i32_64\n"
			"   %SSBO32 = OpTypeStruct %ra_v4f32\n"
			"   %SSBO16 = OpTypeStruct %ra_v4f16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n";

		const StringTemplate decoration		(
			"OpDecorate %ra_v4f32 ArrayStride 16\n"
			"OpDecorate %ra_v4f16 ArrayStride 8\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpDecorate %SSBO32 ${indecor}\n"
			"OpDecorate %SSBO16 BufferBlock\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n"
			"OpDecorate %ssbo16 Binding 1\n"
			"${rounddecor}\n");

		// ssbo16[] <- convert ssbo32[] to 16bit float
		fragments["testfun"]				=
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_64\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"%write = OpLabel\n"
			"   %30 = OpLoad %i32 %i\n"
			"  %src = OpAccessChain %up_v4f32 %ssbo32 %c_i32_0 %30\n"
			"%val32 = OpLoad %v4f32 %src\n"
			"%val16 = OpFConvert %v4f16 %val32\n"
			"  %dst = OpAccessChain %up_v4f16 %ssbo16 %c_i32_0 %30\n"
			"         OpStore %dst %val16\n"
			"         OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"

			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n";

		const RndMode	rndModes[] =
		{
			{"rtz",						"OpDecorate %val16  FPRoundingMode RTZ",	graphicsCheck16BitFloats<ROUNDINGMODE_RTZ>},
			{"rte",						"OpDecorate %val16  FPRoundingMode RTE",	graphicsCheck16BitFloats<ROUNDINGMODE_RTE>},
			{"unspecified_rnd_mode",	"",											graphicsCheck16BitFloats<RoundingModeFlags(ROUNDINGMODE_RTE | ROUNDINGMODE_RTZ)>},
		};

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 rndModeIdx = 0; rndModeIdx < DE_LENGTH_OF_ARRAY(rndModes); ++rndModeIdx)
			{
				map<string, string>	specs;
				string				testName	= string(CAPABILITIES[capIdx].name) + "_vector_float_" + rndModes[rndModeIdx].name;

				specs["cap"]					= CAPABILITIES[capIdx].cap;
				specs["indecor"]				= CAPABILITIES[capIdx].decor;
				specs["rounddecor"]				= rndModes[rndModeIdx].decor;

				fragments["capability"]			= capabilities.specialize(specs);
				fragments["decoration"]			= decoration.specialize(specs);

				resources.inputs.back().first	= CAPABILITIES[capIdx].dtype;
				resources.verifyIO				= rndModes[rndModeIdx].f;


				createTestsForAllStages(testName, defaultColors, defaultColors, fragments, resources, extensions, testGroup, get16BitStorageFeatures(CAPABILITIES[capIdx].name));
			}
	}

	{  // matrix cases
		fragments["pre_main"]				=
			"       %f16 = OpTypeFloat 16\n"
			"  %c_i32_16 = OpConstant %i32 16\n"
			"     %v4f16 = OpTypeVector %f16 4\n"
			"   %m4x4f32 = OpTypeMatrix %v4f32 4\n"
			"   %m4x4f16 = OpTypeMatrix %v4f16 4\n"
			"  %up_v4f32 = OpTypePointer Uniform %v4f32\n"
			"  %up_v4f16 = OpTypePointer Uniform %v4f16\n"
			"%a16m4x4f32 = OpTypeArray %m4x4f32 %c_i32_16\n"
			"%a16m4x4f16 = OpTypeArray %m4x4f16 %c_i32_16\n"
			"    %SSBO32 = OpTypeStruct %a16m4x4f32\n"
			"    %SSBO16 = OpTypeStruct %a16m4x4f16\n"
			" %up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			" %up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"    %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"    %ssbo16 = OpVariable %up_SSBO16 Uniform\n";

		const StringTemplate decoration		(
			"OpDecorate %a16m4x4f32 ArrayStride 64\n"
			"OpDecorate %a16m4x4f16 ArrayStride 32\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO32 0 ColMajor\n"
			"OpMemberDecorate %SSBO32 0 MatrixStride 16\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 ColMajor\n"
			"OpMemberDecorate %SSBO16 0 MatrixStride 8\n"
			"OpDecorate %SSBO32 ${indecor}\n"
			"OpDecorate %SSBO16 BufferBlock\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n"
			"OpDecorate %ssbo16 Binding 1\n"
			"${rounddecor}\n");

		fragments["testfun"]				=
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_16\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"  %write = OpLabel\n"
			"     %30 = OpLoad %i32 %i\n"
			"  %src_0 = OpAccessChain %up_v4f32 %ssbo32 %c_i32_0 %30 %c_i32_0\n"
			"  %src_1 = OpAccessChain %up_v4f32 %ssbo32 %c_i32_0 %30 %c_i32_1\n"
			"  %src_2 = OpAccessChain %up_v4f32 %ssbo32 %c_i32_0 %30 %c_i32_2\n"
			"  %src_3 = OpAccessChain %up_v4f32 %ssbo32 %c_i32_0 %30 %c_i32_3\n"
			"%val32_0 = OpLoad %v4f32 %src_0\n"
			"%val32_1 = OpLoad %v4f32 %src_1\n"
			"%val32_2 = OpLoad %v4f32 %src_2\n"
			"%val32_3 = OpLoad %v4f32 %src_3\n"
			"%val16_0 = OpFConvert %v4f16 %val32_0\n"
			"%val16_1 = OpFConvert %v4f16 %val32_1\n"
			"%val16_2 = OpFConvert %v4f16 %val32_2\n"
			"%val16_3 = OpFConvert %v4f16 %val32_3\n"
			"  %dst_0 = OpAccessChain %up_v4f16 %ssbo16 %c_i32_0 %30 %c_i32_0\n"
			"  %dst_1 = OpAccessChain %up_v4f16 %ssbo16 %c_i32_0 %30 %c_i32_1\n"
			"  %dst_2 = OpAccessChain %up_v4f16 %ssbo16 %c_i32_0 %30 %c_i32_2\n"
			"  %dst_3 = OpAccessChain %up_v4f16 %ssbo16 %c_i32_0 %30 %c_i32_3\n"
			"           OpStore %dst_0 %val16_0\n"
			"           OpStore %dst_1 %val16_1\n"
			"           OpStore %dst_2 %val16_2\n"
			"           OpStore %dst_3 %val16_3\n"
			"           OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"

			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n";

		const RndMode	rndModes[] =
		{
			{"rte",						"OpDecorate %val16_0  FPRoundingMode RTE\nOpDecorate %val16_1  FPRoundingMode RTE\nOpDecorate %val16_2  FPRoundingMode RTE\nOpDecorate %val16_3  FPRoundingMode RTE",	graphicsCheck16BitFloats<ROUNDINGMODE_RTE>},
			{"rtz",						"OpDecorate %val16_0  FPRoundingMode RTZ\nOpDecorate %val16_1  FPRoundingMode RTZ\nOpDecorate %val16_2  FPRoundingMode RTZ\nOpDecorate %val16_3  FPRoundingMode RTZ",	graphicsCheck16BitFloats<ROUNDINGMODE_RTZ>},
			{"unspecified_rnd_mode",	"",																																										graphicsCheck16BitFloats<RoundingModeFlags(ROUNDINGMODE_RTE | ROUNDINGMODE_RTZ)>},
		};

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 rndModeIdx = 0; rndModeIdx < DE_LENGTH_OF_ARRAY(rndModes); ++rndModeIdx)
			{
				map<string, string>	specs;
				string				testName	= string(CAPABILITIES[capIdx].name) + "_matrix_float_" + rndModes[rndModeIdx].name;

				specs["cap"]					= CAPABILITIES[capIdx].cap;
				specs["indecor"]				= CAPABILITIES[capIdx].decor;
				specs["rounddecor"]				= rndModes[rndModeIdx].decor;

				fragments["capability"]			= capabilities.specialize(specs);
				fragments["decoration"]			= decoration.specialize(specs);

				resources.inputs.back().first	= CAPABILITIES[capIdx].dtype;
				resources.verifyIO				= rndModes[rndModeIdx].f;


				createTestsForAllStages(testName, defaultColors, defaultColors, fragments, resources, extensions, testGroup, get16BitStorageFeatures(CAPABILITIES[capIdx].name));
			}
	}
}

void addGraphics16BitStorageInputOutputFloat32To16Group (tcu::TestCaseGroup* testGroup)
{
	de::Random			rnd					(deStringHash(testGroup->getName()));
	RGBA				defaultColors[4];
	vector<string>		extensions;
	map<string, string>	fragments			= passthruFragments();
	const deUint32		numDataPoints		= 64;
	vector<float>		float32Data			= getFloat32s(rnd, numDataPoints);

	extensions.push_back("VK_KHR_16bit_storage");

	fragments["capability"]				= "OpCapability StorageInputOutput16\n";
	fragments["extension"]				= "OpExtension \"SPV_KHR_16bit_storage\"\n";

	getDefaultColors(defaultColors);

	struct RndMode
	{
		const char*				name;
		const char*				decor;
		RoundingModeFlags		flags;
	};

	const RndMode		rndModes[]		=
	{
		{"rtz",						"OpDecorate %ret  FPRoundingMode RTZ",	ROUNDINGMODE_RTZ},
		{"rte",						"OpDecorate %ret  FPRoundingMode RTE",	ROUNDINGMODE_RTE},
		{"unspecified_rnd_mode",	"",										RoundingModeFlags(ROUNDINGMODE_RTE | ROUNDINGMODE_RTZ)},
	};

	struct Case
	{
		const char*	name;
		const char*	interfaceOpFunc;
		const char*	preMain;
		const char*	inputType;
		const char*	outputType;
		deUint32	numPerCase;
		deUint32	numElements;
	};

	const Case	cases[]		=
	{
		{ // Scalar cases
			"scalar",

			"%interface_op_func = OpFunction %f16 None %f16_f32_function\n"
			"        %io_param1 = OpFunctionParameter %f32\n"
			"            %entry = OpLabel\n"
			"			   %ret = OpFConvert %f16 %io_param1\n"
			"                     OpReturnValue %ret\n"
			"                     OpFunctionEnd\n",

			"             %f16 = OpTypeFloat 16\n"
			"          %op_f16 = OpTypePointer Output %f16\n"
			"           %a3f16 = OpTypeArray %f16 %c_i32_3\n"
			"        %op_a3f16 = OpTypePointer Output %a3f16\n"
			"%f16_f32_function = OpTypeFunction %f16 %f32\n"
			"           %a3f32 = OpTypeArray %f32 %c_i32_3\n"
			"        %ip_a3f32 = OpTypePointer Input %a3f32\n",

			"f32",
			"f16",
			4,
			1,
		},
		{ // Vector cases
			"vector",

			"%interface_op_func = OpFunction %v2f16 None %v2f16_v2f32_function\n"
			"        %io_param1 = OpFunctionParameter %v2f32\n"
			"            %entry = OpLabel\n"
			"			   %ret = OpFConvert %v2f16 %io_param1\n"
			"                     OpReturnValue %ret\n"
			"                     OpFunctionEnd\n",

			"                 %f16 = OpTypeFloat 16\n"
			"               %v2f16 = OpTypeVector %f16 2\n"
			"            %op_v2f16 = OpTypePointer Output %v2f16\n"
			"             %a3v2f16 = OpTypeArray %v2f16 %c_i32_3\n"
			"          %op_a3v2f16 = OpTypePointer Output %a3v2f16\n"
			"%v2f16_v2f32_function = OpTypeFunction %v2f16 %v2f32\n"
			"             %a3v2f32 = OpTypeArray %v2f32 %c_i32_3\n"
			"          %ip_a3v2f32 = OpTypePointer Input %a3v2f32\n",

			"v2f32",
			"v2f16",
			2 * 4,
			2,
		}
	};

	VulkanFeatures	requiredFeatures;
	requiredFeatures.ext16BitStorage = EXT16BITSTORAGEFEATURES_INPUT_OUTPUT;

	for (deUint32 caseIdx = 0; caseIdx < DE_LENGTH_OF_ARRAY(cases); ++caseIdx)
		for (deUint32 rndModeIdx = 0; rndModeIdx < DE_LENGTH_OF_ARRAY(rndModes); ++rndModeIdx)
		{
			fragments["interface_op_func"]	= cases[caseIdx].interfaceOpFunc;
			fragments["pre_main"]			= cases[caseIdx].preMain;
			fragments["decoration"]			= rndModes[rndModeIdx].decor;

			fragments["input_type"]			= cases[caseIdx].inputType;
			fragments["output_type"]		= cases[caseIdx].outputType;

			GraphicsInterfaces	interfaces;
			const deUint32		numPerCase	= cases[caseIdx].numPerCase;
			vector<float>		subInputs	(numPerCase);
			vector<deFloat16>	subOutputs	(numPerCase);

			// The pipeline need this to call compare16BitFloat() when checking the result.
			interfaces.setRoundingMode(rndModes[rndModeIdx].flags);

			for (deUint32 caseNdx = 0; caseNdx < numDataPoints / numPerCase; ++caseNdx)
			{
				string			testName	= string(cases[caseIdx].name) + numberToString(caseNdx) + "_" + rndModes[rndModeIdx].name;

				for (deUint32 numNdx = 0; numNdx < numPerCase; ++numNdx)
				{
					subInputs[numNdx]	= float32Data[caseNdx * numPerCase + numNdx];
					// We derive the expected result from inputs directly in the graphics pipeline.
					subOutputs[numNdx]	= 0;
				}
				interfaces.setInputOutput(std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_FLOAT32), BufferSp(new Float32Buffer(subInputs))),
										  std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_FLOAT16), BufferSp(new Float16Buffer(subOutputs))));
				createTestsForAllStages(testName, defaultColors, defaultColors, fragments, interfaces, extensions, testGroup, requiredFeatures);
			}
		}
}

void addGraphics16BitStorageInputOutputFloat16To32Group (tcu::TestCaseGroup* testGroup)
{
	de::Random				rnd					(deStringHash(testGroup->getName()));
	RGBA					defaultColors[4];
	vector<string>			extensions;
	map<string, string>		fragments			= passthruFragments();
	const deUint32			numDataPoints		= 64;
	vector<deFloat16>		float16Data			(getFloat16s(rnd, numDataPoints));
	vector<float>			float32Data;

	float32Data.reserve(numDataPoints);
	for (deUint32 numIdx = 0; numIdx < numDataPoints; ++numIdx)
		float32Data.push_back(deFloat16To32(float16Data[numIdx]));

	extensions.push_back("VK_KHR_16bit_storage");

	fragments["capability"]				= "OpCapability StorageInputOutput16\n";
	fragments["extension"]				= "OpExtension \"SPV_KHR_16bit_storage\"\n";

	getDefaultColors(defaultColors);

	struct Case
	{
		const char*	name;
		const char*	interfaceOpFunc;
		const char*	preMain;
		const char*	inputType;
		const char*	outputType;
		deUint32	numPerCase;
		deUint32	numElements;
	};

	Case	cases[]		=
	{
		{ // Scalar cases
			"scalar",

			"%interface_op_func = OpFunction %f32 None %f32_f16_function\n"
			"        %io_param1 = OpFunctionParameter %f16\n"
			"            %entry = OpLabel\n"
			"			   %ret = OpFConvert %f32 %io_param1\n"
			"                     OpReturnValue %ret\n"
			"                     OpFunctionEnd\n",

			"             %f16 = OpTypeFloat 16\n"
			"          %ip_f16 = OpTypePointer Input %f16\n"
			"           %a3f16 = OpTypeArray %f16 %c_i32_3\n"
			"        %ip_a3f16 = OpTypePointer Input %a3f16\n"
			"%f32_f16_function = OpTypeFunction %f32 %f16\n"
			"           %a3f32 = OpTypeArray %f32 %c_i32_3\n"
			"        %op_a3f32 = OpTypePointer Output %a3f32\n",

			"f16",
			"f32",
			4,
			1,
		},
		{ // Vector cases
			"vector",

			"%interface_op_func = OpFunction %v2f32 None %v2f32_v2f16_function\n"
			"        %io_param1 = OpFunctionParameter %v2f16\n"
			"            %entry = OpLabel\n"
			"			   %ret = OpFConvert %v2f32 %io_param1\n"
			"                     OpReturnValue %ret\n"
			"                     OpFunctionEnd\n",

			"                 %f16 = OpTypeFloat 16\n"
			"		        %v2f16 = OpTypeVector %f16 2\n"
			"            %ip_v2f16 = OpTypePointer Input %v2f16\n"
			"             %a3v2f16 = OpTypeArray %v2f16 %c_i32_3\n"
			"          %ip_a3v2f16 = OpTypePointer Input %a3v2f16\n"
			"%v2f32_v2f16_function = OpTypeFunction %v2f32 %v2f16\n"
			"             %a3v2f32 = OpTypeArray %v2f32 %c_i32_3\n"
			"          %op_a3v2f32 = OpTypePointer Output %a3v2f32\n",

			"v2f16",
			"v2f32",
			2 * 4,
			2,
		}
	};

	VulkanFeatures	requiredFeatures;
	requiredFeatures.ext16BitStorage = EXT16BITSTORAGEFEATURES_INPUT_OUTPUT;

	for (deUint32 caseIdx = 0; caseIdx < DE_LENGTH_OF_ARRAY(cases); ++caseIdx)
	{
		fragments["interface_op_func"]	= cases[caseIdx].interfaceOpFunc;
		fragments["pre_main"]			= cases[caseIdx].preMain;

		fragments["input_type"]			= cases[caseIdx].inputType;
		fragments["output_type"]		= cases[caseIdx].outputType;

		GraphicsInterfaces	interfaces;
		const deUint32		numPerCase	= cases[caseIdx].numPerCase;
		vector<deFloat16>	subInputs	(numPerCase);
		vector<float>		subOutputs	(numPerCase);

		for (deUint32 caseNdx = 0; caseNdx < numDataPoints / numPerCase; ++caseNdx)
		{
			string			testName	= string(cases[caseIdx].name) + numberToString(caseNdx);

			for (deUint32 numNdx = 0; numNdx < numPerCase; ++numNdx)
			{
				subInputs[numNdx]	= float16Data[caseNdx * numPerCase + numNdx];
				subOutputs[numNdx]	= float32Data[caseNdx * numPerCase + numNdx];
			}
			interfaces.setInputOutput(std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_FLOAT16), BufferSp(new Float16Buffer(subInputs))),
									  std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_FLOAT32), BufferSp(new Float32Buffer(subOutputs))));
			createTestsForAllStages(testName, defaultColors, defaultColors, fragments, interfaces, extensions, testGroup, requiredFeatures);
		}
	}
}

void addGraphics16BitStorageInputOutputInt32To16Group (tcu::TestCaseGroup* testGroup)
{
	de::Random							rnd					(deStringHash(testGroup->getName()));
	RGBA								defaultColors[4];
	vector<string>						extensions;
	map<string, string>					fragments			= passthruFragments();
	const deUint32						numDataPoints		= 64;
	// inputs and outputs are declared to be vectors of signed integers.
	// However, depending on the test, they may be interpreted as unsiged
	// integers. That won't be a problem as long as we passed the bits
	// in faithfully to the pipeline.
	vector<deInt32>						inputs				= getInt32s(rnd, numDataPoints);
	vector<deInt16>						outputs;

	outputs.reserve(inputs.size());
	for (deUint32 numNdx = 0; numNdx < inputs.size(); ++numNdx)
		outputs.push_back(static_cast<deInt16>(0xffff & inputs[numNdx]));

	extensions.push_back("VK_KHR_16bit_storage");

	fragments["capability"]				= "OpCapability StorageInputOutput16\n";
	fragments["extension"]				= "OpExtension \"SPV_KHR_16bit_storage\"\n";

	getDefaultColors(defaultColors);

	const StringTemplate	scalarInterfaceOpFunc(
			"%interface_op_func = OpFunction %${type16} None %${type16}_${type32}_function\n"
			"        %io_param1 = OpFunctionParameter %${type32}\n"
			"            %entry = OpLabel\n"
			"			   %ret = ${convert} %${type16} %io_param1\n"
			"                     OpReturnValue %ret\n"
			"                     OpFunctionEnd\n");

	const StringTemplate	scalarPreMain(
			"             %${type16} = OpTypeInt 16 ${signed}\n"
			"          %op_${type16} = OpTypePointer Output %${type16}\n"
			"           %a3${type16} = OpTypeArray %${type16} %c_i32_3\n"
			"        %op_a3${type16} = OpTypePointer Output %a3${type16}\n"
			"%${type16}_${type32}_function = OpTypeFunction %${type16} %${type32}\n"
			"           %a3${type32} = OpTypeArray %${type32} %c_i32_3\n"
			"        %ip_a3${type32} = OpTypePointer Input %a3${type32}\n");

	const StringTemplate	vecInterfaceOpFunc(
			"%interface_op_func = OpFunction %${type16} None %${type16}_${type32}_function\n"
			"        %io_param1 = OpFunctionParameter %${type32}\n"
			"            %entry = OpLabel\n"
			"			   %ret = ${convert} %${type16} %io_param1\n"
			"                     OpReturnValue %ret\n"
			"                     OpFunctionEnd\n");

	const StringTemplate	vecPreMain(
			"	                %i16 = OpTypeInt 16 1\n"
			"	                %u16 = OpTypeInt 16 0\n"
			"                 %v4i16 = OpTypeVector %i16 4\n"
			"                 %v4u16 = OpTypeVector %u16 4\n"
			"          %op_${type16} = OpTypePointer Output %${type16}\n"
			"           %a3${type16} = OpTypeArray %${type16} %c_i32_3\n"
			"        %op_a3${type16} = OpTypePointer Output %a3${type16}\n"
			"%${type16}_${type32}_function = OpTypeFunction %${type16} %${type32}\n"
			"           %a3${type32} = OpTypeArray %${type32} %c_i32_3\n"
			"        %ip_a3${type32} = OpTypePointer Input %a3${type32}\n");

	struct Case
	{
		const char*				name;
		const StringTemplate&	interfaceOpFunc;
		const StringTemplate&	preMain;
		const char*				type32;
		const char*				type16;
		const char*				sign;
		const char*				opcode;
		deUint32				numPerCase;
		deUint32				numElements;
	};

	Case	cases[]		=
	{
		{"scalar_sint",	scalarInterfaceOpFunc,	scalarPreMain,	"i32",		"i16",		"1",	"OpSConvert",	4,		1},
		{"scalar_uint",	scalarInterfaceOpFunc,	scalarPreMain,	"u32",		"u16",		"0",	"OpUConvert",	4,		1},
		{"vector_sint",	vecInterfaceOpFunc,		vecPreMain,		"v4i32",	"v4i16",	"1",	"OpSConvert",	4 * 4,	4},
		{"vector_uint",	vecInterfaceOpFunc,		vecPreMain,		"v4u32",	"v4u16",	"0",	"OpUConvert",	4 * 4,	4},
	};

	VulkanFeatures	requiredFeatures;
	requiredFeatures.ext16BitStorage = EXT16BITSTORAGEFEATURES_INPUT_OUTPUT;

	for (deUint32 caseIdx = 0; caseIdx < DE_LENGTH_OF_ARRAY(cases); ++caseIdx)
	{
		map<string, string>				specs;

		specs["type32"]					= cases[caseIdx].type32;
		specs["type16"]					= cases[caseIdx].type16;
		specs["signed"]					= cases[caseIdx].sign;
		specs["convert"]				= cases[caseIdx].opcode;

		fragments["pre_main"]			= cases[caseIdx].preMain.specialize(specs);
		fragments["interface_op_func"]	= cases[caseIdx].interfaceOpFunc.specialize(specs);
		fragments["input_type"]			= cases[caseIdx].type32;
		fragments["output_type"]		= cases[caseIdx].type16;

		GraphicsInterfaces				interfaces;
		const deUint32					numPerCase	= cases[caseIdx].numPerCase;
		vector<deInt32>					subInputs	(numPerCase);
		vector<deInt16>					subOutputs	(numPerCase);

		for (deUint32 caseNdx = 0; caseNdx < numDataPoints / numPerCase; ++caseNdx)
		{
			string			testName	= string(cases[caseIdx].name) + numberToString(caseNdx);

			for (deUint32 numNdx = 0; numNdx < numPerCase; ++numNdx)
			{
				subInputs[numNdx]	= inputs[caseNdx * numPerCase + numNdx];
				subOutputs[numNdx]	= outputs[caseNdx * numPerCase + numNdx];
			}
			if (strcmp(cases[caseIdx].sign, "1") == 0)
			{
				interfaces.setInputOutput(std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_INT32), BufferSp(new Int32Buffer(subInputs))),
										  std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_INT16), BufferSp(new Int16Buffer(subOutputs))));
			}
			else
			{
				interfaces.setInputOutput(std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_UINT32), BufferSp(new Int32Buffer(subInputs))),
										  std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_UINT16), BufferSp(new Int16Buffer(subOutputs))));
			}
			createTestsForAllStages(testName, defaultColors, defaultColors, fragments, interfaces, extensions, testGroup, requiredFeatures);
		}
	}
}

void addGraphics16BitStorageInputOutputInt16To32Group (tcu::TestCaseGroup* testGroup)
{
	de::Random							rnd					(deStringHash(testGroup->getName()));
	RGBA								defaultColors[4];
	vector<string>						extensions;
	map<string, string>					fragments			= passthruFragments();
	const deUint32						numDataPoints		= 64;
	// inputs and outputs are declared to be vectors of signed integers.
	// However, depending on the test, they may be interpreted as unsiged
	// integers. That won't be a problem as long as we passed the bits
	// in faithfully to the pipeline.
	vector<deInt16>						inputs				= getInt16s(rnd, numDataPoints);
	vector<deInt32>						sOutputs;
	vector<deInt32>						uOutputs;
	const deUint16						signBitMask			= 0x8000;
	const deUint32						signExtendMask		= 0xffff0000;

	sOutputs.reserve(inputs.size());
	uOutputs.reserve(inputs.size());

	for (deUint32 numNdx = 0; numNdx < inputs.size(); ++numNdx)
	{
		uOutputs.push_back(static_cast<deUint16>(inputs[numNdx]));
		if (inputs[numNdx] & signBitMask)
			sOutputs.push_back(static_cast<deInt32>(inputs[numNdx] | signExtendMask));
		else
			sOutputs.push_back(static_cast<deInt32>(inputs[numNdx]));
	}

	extensions.push_back("VK_KHR_16bit_storage");

	fragments["capability"]				= "OpCapability StorageInputOutput16\n";
	fragments["extension"]				= "OpExtension \"SPV_KHR_16bit_storage\"\n";

	getDefaultColors(defaultColors);

	const StringTemplate scalarIfOpFunc	(
			"%interface_op_func = OpFunction %${type32} None %${type32}_${type16}_function\n"
			"        %io_param1 = OpFunctionParameter %${type16}\n"
			"            %entry = OpLabel\n"
			"			   %ret = ${convert} %${type32} %io_param1\n"
			"                     OpReturnValue %ret\n"
			"                     OpFunctionEnd\n");

	const StringTemplate scalarPreMain	(
			"             %${type16} = OpTypeInt 16 ${signed}\n"
			"          %ip_${type16} = OpTypePointer Input %${type16}\n"
			"           %a3${type16} = OpTypeArray %${type16} %c_i32_3\n"
			"        %ip_a3${type16} = OpTypePointer Input %a3${type16}\n"
			"%${type32}_${type16}_function = OpTypeFunction %${type32} %${type16}\n"
			"           %a3${type32} = OpTypeArray %${type32} %c_i32_3\n"
			"        %op_a3${type32} = OpTypePointer Output %a3${type32}\n");

	const StringTemplate vecIfOpFunc	(
			"%interface_op_func = OpFunction %${type32} None %${type32}_${type16}_function\n"
			"        %io_param1 = OpFunctionParameter %${type16}\n"
			"            %entry = OpLabel\n"
			"			   %ret = ${convert} %${type32} %io_param1\n"
			"                     OpReturnValue %ret\n"
			"                     OpFunctionEnd\n");

	const StringTemplate vecPreMain	(
			"	                %i16 = OpTypeInt 16 1\n"
			"	                %u16 = OpTypeInt 16 0\n"
			"                 %v4i16 = OpTypeVector %i16 4\n"
			"                 %v4u16 = OpTypeVector %u16 4\n"
			"          %ip_${type16} = OpTypePointer Input %${type16}\n"
			"           %a3${type16} = OpTypeArray %${type16} %c_i32_3\n"
			"        %ip_a3${type16} = OpTypePointer Input %a3${type16}\n"
			"%${type32}_${type16}_function = OpTypeFunction %${type32} %${type16}\n"
			"           %a3${type32} = OpTypeArray %${type32} %c_i32_3\n"
			"        %op_a3${type32} = OpTypePointer Output %a3${type32}\n");

	struct Case
	{
		const char*				name;
		const StringTemplate&	interfaceOpFunc;
		const StringTemplate&	preMain;
		const char*				type32;
		const char*				type16;
		const char*				sign;
		const char*				opcode;
		deUint32				numPerCase;
		deUint32				numElements;
	};

	Case	cases[]		=
	{
		{"scalar_sint",	scalarIfOpFunc,	scalarPreMain,	"i32",		"i16",		"1",	"OpSConvert",	4,		1},
		{"scalar_uint",	scalarIfOpFunc,	scalarPreMain,	"u32",		"u16",		"0",	"OpUConvert",	4,		1},
		{"vector_sint",	vecIfOpFunc,	vecPreMain,		"v4i32",	"v4i16",	"1",	"OpSConvert",	4 * 4,	4},
		{"vector_uint",	vecIfOpFunc,	vecPreMain,		"v4u32",	"v4u16",	"0",	"OpUConvert",	4 * 4,	4},
	};

	VulkanFeatures	requiredFeatures;
	requiredFeatures.ext16BitStorage = EXT16BITSTORAGEFEATURES_INPUT_OUTPUT;

	for (deUint32 caseIdx = 0; caseIdx < DE_LENGTH_OF_ARRAY(cases); ++caseIdx)
	{
		map<string, string>				specs;

		specs["type32"]					= cases[caseIdx].type32;
		specs["type16"]					= cases[caseIdx].type16;
		specs["signed"]					= cases[caseIdx].sign;
		specs["convert"]				= cases[caseIdx].opcode;

		fragments["pre_main"]			= cases[caseIdx].preMain.specialize(specs);
		fragments["interface_op_func"]	= cases[caseIdx].interfaceOpFunc.specialize(specs);
		fragments["input_type"]			= cases[caseIdx].type16;
		fragments["output_type"]		= cases[caseIdx].type32;

		GraphicsInterfaces				interfaces;
		const deUint32					numPerCase	= cases[caseIdx].numPerCase;
		vector<deInt16>					subInputs	(numPerCase);
		vector<deInt32>					subOutputs	(numPerCase);

		for (deUint32 caseNdx = 0; caseNdx < numDataPoints / numPerCase; ++caseNdx)
		{
			string			testName	= string(cases[caseIdx].name) + numberToString(caseNdx);

			for (deUint32 numNdx = 0; numNdx < numPerCase; ++numNdx)
			{
				subInputs[numNdx]	= inputs[caseNdx * numPerCase + numNdx];
				if (cases[caseIdx].sign[0] == '1')
					subOutputs[numNdx]	= sOutputs[caseNdx * numPerCase + numNdx];
				else
					subOutputs[numNdx]	= uOutputs[caseNdx * numPerCase + numNdx];
			}
			if (strcmp(cases[caseIdx].sign, "1") == 0)
			{
				interfaces.setInputOutput(std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_INT16), BufferSp(new Int16Buffer(subInputs))),
										  std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_INT32), BufferSp(new Int32Buffer(subOutputs))));
			}
			else
			{
				interfaces.setInputOutput(std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_UINT16), BufferSp(new Int16Buffer(subInputs))),
										  std::make_pair(IFDataType(cases[caseIdx].numElements, NUMBERTYPE_UINT32), BufferSp(new Int32Buffer(subOutputs))));
			}
			createTestsForAllStages(testName, defaultColors, defaultColors, fragments, interfaces, extensions, testGroup, requiredFeatures);
		}
	}
}

void addGraphics16BitStoragePushConstantFloat16To32Group (tcu::TestCaseGroup* testGroup)
{
	de::Random							rnd					(deStringHash(testGroup->getName()));
	map<string, string>					fragments;
	RGBA								defaultColors[4];
	vector<string>						extensions;
	GraphicsResources					resources;
	PushConstants						pcs;
	const deUint32						numDataPoints		= 64;
	vector<deFloat16>					float16Data			(getFloat16s(rnd, numDataPoints));
	vector<float>						float32Data;
	VulkanFeatures						requiredFeatures;

	float32Data.reserve(numDataPoints);
	for (deUint32 numIdx = 0; numIdx < numDataPoints; ++numIdx)
		float32Data.push_back(deFloat16To32(float16Data[numIdx]));

	extensions.push_back("VK_KHR_16bit_storage");
	requiredFeatures.ext16BitStorage = EXT16BITSTORAGEFEATURES_PUSH_CONSTANT;

	fragments["capability"]				= "OpCapability StoragePushConstant16\n";
	fragments["extension"]				= "OpExtension \"SPV_KHR_16bit_storage\"";

	pcs.setPushConstant(BufferSp(new Float16Buffer(float16Data)));
	resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(float32Data))));
	resources.verifyIO = check32BitFloats;

	getDefaultColors(defaultColors);

	const StringTemplate	testFun		(
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"    %param = OpFunctionParameter %v4f32\n"

		"%entry = OpLabel\n"
		"    %i = OpVariable %fp_i32 Function\n"
		"         OpStore %i %c_i32_0\n"
		"         OpBranch %loop\n"

		" %loop = OpLabel\n"
		"   %15 = OpLoad %i32 %i\n"
		"   %lt = OpSLessThan %bool %15 ${count}\n"
		"         OpLoopMerge %merge %inc None\n"
		"         OpBranchConditional %lt %write %merge\n"

		"%write = OpLabel\n"
		"   %30 = OpLoad %i32 %i\n"
		"  %src = OpAccessChain ${pp_type16} %pc16 %c_i32_0 %30 ${index0:opt}\n"
		"%val16 = OpLoad ${f_type16} %src\n"
		"%val32 = OpFConvert ${f_type32} %val16\n"
		"  %dst = OpAccessChain ${up_type32} %ssbo32 %c_i32_0 %30 ${index0:opt}\n"
		"         OpStore %dst %val32\n"

		"${store:opt}\n"

		"         OpBranch %inc\n"

		"  %inc = OpLabel\n"
		"   %37 = OpLoad %i32 %i\n"
		"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
		"         OpStore %i %39\n"
		"         OpBranch %loop\n"

		"%merge = OpLabel\n"
		"         OpReturnValue %param\n"

		"OpFunctionEnd\n");

	{  // Scalar cases
		fragments["pre_main"]				=
			"      %f16 = OpTypeFloat 16\n"
			" %c_i32_64 = OpConstant %i32 64\n"					// Should be the same as numDataPoints
			"   %a64f16 = OpTypeArray %f16 %c_i32_64\n"
			"   %a64f32 = OpTypeArray %f32 %c_i32_64\n"
			"   %pp_f16 = OpTypePointer PushConstant %f16\n"
			"   %up_f32 = OpTypePointer Uniform %f32\n"
			"   %SSBO32 = OpTypeStruct %a64f32\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"     %PC16 = OpTypeStruct %a64f16\n"
			"  %pp_PC16 = OpTypePointer PushConstant %PC16\n"
			"     %pc16 = OpVariable %pp_PC16 PushConstant\n";

		fragments["decoration"]				=
			"OpDecorate %a64f16 ArrayStride 2\n"
			"OpDecorate %a64f32 ArrayStride 4\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpDecorate %PC16 Block\n"
			"OpMemberDecorate %PC16 0 Offset 0\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n";

		map<string, string>		specs;

		specs["count"]			= "%c_i32_64";
		specs["pp_type16"]		= "%pp_f16";
		specs["f_type16"]		= "%f16";
		specs["f_type32"]		= "%f32";
		specs["up_type32"]		= "%up_f32";

		fragments["testfun"]	= testFun.specialize(specs);

		createTestsForAllStages("scalar", defaultColors, defaultColors, fragments, pcs, resources, extensions, testGroup, requiredFeatures);
	}

	{  // Vector cases
		fragments["pre_main"]				=
			"      %f16 = OpTypeFloat 16\n"
			"    %v4f16 = OpTypeVector %f16 4\n"
			" %c_i32_16 = OpConstant %i32 16\n"
			" %a16v4f16 = OpTypeArray %v4f16 %c_i32_16\n"
			" %a16v4f32 = OpTypeArray %v4f32 %c_i32_16\n"
			" %pp_v4f16 = OpTypePointer PushConstant %v4f16\n"
			" %up_v4f32 = OpTypePointer Uniform %v4f32\n"
			"   %SSBO32 = OpTypeStruct %a16v4f32\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"     %PC16 = OpTypeStruct %a16v4f16\n"
			"  %pp_PC16 = OpTypePointer PushConstant %PC16\n"
			"     %pc16 = OpVariable %pp_PC16 PushConstant\n";

		fragments["decoration"]				=
			"OpDecorate %a16v4f16 ArrayStride 8\n"
			"OpDecorate %a16v4f32 ArrayStride 16\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpDecorate %PC16 Block\n"
			"OpMemberDecorate %PC16 0 Offset 0\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n";

		map<string, string>		specs;

		specs["count"]			= "%c_i32_16";
		specs["pp_type16"]		= "%pp_v4f16";
		specs["f_type16"]		= "%v4f16";
		specs["f_type32"]		= "%v4f32";
		specs["up_type32"]		= "%up_v4f32";

		fragments["testfun"]	= testFun.specialize(specs);

		createTestsForAllStages("vector", defaultColors, defaultColors, fragments, pcs, resources, extensions, testGroup, requiredFeatures);
	}

	{  // Matrix cases
		fragments["pre_main"]				=
			"  %c_i32_8 = OpConstant %i32 8\n"
			"      %f16 = OpTypeFloat 16\n"
			"    %v4f16 = OpTypeVector %f16 4\n"
			"  %m2v4f16 = OpTypeMatrix %v4f16 2\n"
			"  %m2v4f32 = OpTypeMatrix %v4f32 2\n"
			"%a8m2v4f16 = OpTypeArray %m2v4f16 %c_i32_8\n"
			"%a8m2v4f32 = OpTypeArray %m2v4f32 %c_i32_8\n"
			" %pp_v4f16 = OpTypePointer PushConstant %v4f16\n"
			" %up_v4f32 = OpTypePointer Uniform %v4f32\n"
			"   %SSBO32 = OpTypeStruct %a8m2v4f32\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"     %PC16 = OpTypeStruct %a8m2v4f16\n"
			"  %pp_PC16 = OpTypePointer PushConstant %PC16\n"
			"     %pc16 = OpVariable %pp_PC16 PushConstant\n";

		fragments["decoration"]				=
			"OpDecorate %a8m2v4f16 ArrayStride 16\n"
			"OpDecorate %a8m2v4f32 ArrayStride 32\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO32 0 ColMajor\n"
			"OpMemberDecorate %SSBO32 0 MatrixStride 16\n"
			"OpDecorate %PC16 Block\n"
			"OpMemberDecorate %PC16 0 Offset 0\n"
			"OpMemberDecorate %PC16 0 ColMajor\n"
			"OpMemberDecorate %PC16 0 MatrixStride 8\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n";

		map<string, string>		specs;

		specs["count"]			= "%c_i32_8";
		specs["pp_type16"]		= "%pp_v4f16";
		specs["up_type32"]		= "%up_v4f32";
		specs["f_type16"]		= "%v4f16";
		specs["f_type32"]		= "%v4f32";
		specs["index0"]			= "%c_i32_0";
		specs["store"]			=
			"  %src_1 = OpAccessChain %pp_v4f16 %pc16 %c_i32_0 %30 %c_i32_1\n"
			"%val16_1 = OpLoad %v4f16 %src_1\n"
			"%val32_1 = OpFConvert %v4f32 %val16_1\n"
			"  %dst_1 = OpAccessChain %up_v4f32 %ssbo32 %c_i32_0 %30 %c_i32_1\n"
			"           OpStore %dst_1 %val32_1\n";

		fragments["testfun"]	= testFun.specialize(specs);

		createTestsForAllStages("matrix", defaultColors, defaultColors, fragments, pcs, resources, extensions, testGroup, requiredFeatures);
	}
}

void addGraphics16BitStoragePushConstantInt16To32Group (tcu::TestCaseGroup* testGroup)
{
	de::Random							rnd					(deStringHash(testGroup->getName()));
	map<string, string>					fragments;
	RGBA								defaultColors[4];
	const deUint32						numDataPoints		= 64;
	vector<deInt16>						inputs				= getInt16s(rnd, numDataPoints);
	vector<deInt32>						sOutputs;
	vector<deInt32>						uOutputs;
	PushConstants						pcs;
	GraphicsResources					resources;
	vector<string>						extensions;
	const deUint16						signBitMask			= 0x8000;
	const deUint32						signExtendMask		= 0xffff0000;
	VulkanFeatures						requiredFeatures;

	sOutputs.reserve(inputs.size());
	uOutputs.reserve(inputs.size());

	for (deUint32 numNdx = 0; numNdx < inputs.size(); ++numNdx)
	{
		uOutputs.push_back(static_cast<deUint16>(inputs[numNdx]));
		if (inputs[numNdx] & signBitMask)
			sOutputs.push_back(static_cast<deInt32>(inputs[numNdx] | signExtendMask));
		else
			sOutputs.push_back(static_cast<deInt32>(inputs[numNdx]));
	}

	extensions.push_back("VK_KHR_16bit_storage");
	requiredFeatures.ext16BitStorage = EXT16BITSTORAGEFEATURES_PUSH_CONSTANT;

	fragments["capability"]				= "OpCapability StoragePushConstant16\n";
	fragments["extension"]				= "OpExtension \"SPV_KHR_16bit_storage\"";

	pcs.setPushConstant(BufferSp(new Int16Buffer(inputs)));

	getDefaultColors(defaultColors);

	const StringTemplate	testFun		(
		"%test_code = OpFunction %v4f32 None %v4f32_function\n"
		"    %param = OpFunctionParameter %v4f32\n"

		"%entry = OpLabel\n"
		"    %i = OpVariable %fp_i32 Function\n"
		"         OpStore %i %c_i32_0\n"
		"         OpBranch %loop\n"

		" %loop = OpLabel\n"
		"   %15 = OpLoad %i32 %i\n"
		"   %lt = OpSLessThan %bool %15 %c_i32_${count}\n"
		"         OpLoopMerge %merge %inc None\n"
		"         OpBranchConditional %lt %write %merge\n"

		"%write = OpLabel\n"
		"   %30 = OpLoad %i32 %i\n"
		"  %src = OpAccessChain %pp_${type16} %pc16 %c_i32_0 %30\n"
		"%val16 = OpLoad %${type16} %src\n"
		"%val32 = ${convert} %${type32} %val16\n"
		"  %dst = OpAccessChain %up_${type32} %ssbo32 %c_i32_0 %30\n"
		"         OpStore %dst %val32\n"
		"         OpBranch %inc\n"

		"  %inc = OpLabel\n"
		"   %37 = OpLoad %i32 %i\n"
		"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
		"         OpStore %i %39\n"
		"         OpBranch %loop\n"

		"%merge = OpLabel\n"
		"         OpReturnValue %param\n"

		"OpFunctionEnd\n");

	{  // Scalar cases
		const StringTemplate	preMain		(
			"         %${type16} = OpTypeInt 16 ${signed}\n"
			"    %c_i32_${count} = OpConstant %i32 ${count}\n"					// Should be the same as numDataPoints
			"%a${count}${type16} = OpTypeArray %${type16} %c_i32_${count}\n"
			"%a${count}${type32} = OpTypeArray %${type32} %c_i32_${count}\n"
			"      %pp_${type16} = OpTypePointer PushConstant %${type16}\n"
			"      %up_${type32} = OpTypePointer Uniform      %${type32}\n"
			"            %SSBO32 = OpTypeStruct %a${count}${type32}\n"
			"         %up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"            %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"              %PC16 = OpTypeStruct %a${count}${type16}\n"
			"           %pp_PC16 = OpTypePointer PushConstant %PC16\n"
			"              %pc16 = OpVariable %pp_PC16 PushConstant\n");

		const StringTemplate	decoration	(
			"OpDecorate %a${count}${type16} ArrayStride 2\n"
			"OpDecorate %a${count}${type32} ArrayStride 4\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpDecorate %PC16 Block\n"
			"OpMemberDecorate %PC16 0 Offset 0\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n");

		{  // signed int
			map<string, string>		specs;

			specs["type16"]			= "i16";
			specs["type32"]			= "i32";
			specs["signed"]			= "1";
			specs["count"]			= "64";
			specs["convert"]		= "OpSConvert";

			fragments["testfun"]	= testFun.specialize(specs);
			fragments["pre_main"]	= preMain.specialize(specs);
			fragments["decoration"]	= decoration.specialize(specs);

			resources.outputs.clear();
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int32Buffer(sOutputs))));
			createTestsForAllStages("sint_scalar", defaultColors, defaultColors, fragments, pcs, resources, extensions, testGroup, requiredFeatures);
		}
		{  // signed int
			map<string, string>		specs;

			specs["type16"]			= "u16";
			specs["type32"]			= "u32";
			specs["signed"]			= "0";
			specs["count"]			= "64";
			specs["convert"]		= "OpUConvert";

			fragments["testfun"]	= testFun.specialize(specs);
			fragments["pre_main"]	= preMain.specialize(specs);
			fragments["decoration"]	= decoration.specialize(specs);

			resources.outputs.clear();
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int32Buffer(uOutputs))));
			createTestsForAllStages("uint_scalar", defaultColors, defaultColors, fragments, pcs, resources, extensions, testGroup, requiredFeatures);
		}
	}

	{  // Vector cases
		const StringTemplate	preMain		(
			"    %${base_type16} = OpTypeInt 16 ${signed}\n"
			"         %${type16} = OpTypeVector %${base_type16} 2\n"
			"    %c_i32_${count} = OpConstant %i32 ${count}\n"
			"%a${count}${type16} = OpTypeArray %${type16} %c_i32_${count}\n"
			"%a${count}${type32} = OpTypeArray %${type32} %c_i32_${count}\n"
			"      %pp_${type16} = OpTypePointer PushConstant %${type16}\n"
			"      %up_${type32} = OpTypePointer Uniform      %${type32}\n"
			"            %SSBO32 = OpTypeStruct %a${count}${type32}\n"
			"         %up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"            %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"              %PC16 = OpTypeStruct %a${count}${type16}\n"
			"           %pp_PC16 = OpTypePointer PushConstant %PC16\n"
			"              %pc16 = OpVariable %pp_PC16 PushConstant\n");

		const StringTemplate	decoration	(
			"OpDecorate %a${count}${type16} ArrayStride 4\n"
			"OpDecorate %a${count}${type32} ArrayStride 8\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpDecorate %PC16 Block\n"
			"OpMemberDecorate %PC16 0 Offset 0\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 0\n");

		{  // signed int
			map<string, string>		specs;

			specs["base_type16"]	= "i16";
			specs["type16"]			= "v2i16";
			specs["type32"]			= "v2i32";
			specs["signed"]			= "1";
			specs["count"]			= "32";				// 64 / 2
			specs["convert"]		= "OpSConvert";

			fragments["testfun"]	= testFun.specialize(specs);
			fragments["pre_main"]	= preMain.specialize(specs);
			fragments["decoration"]	= decoration.specialize(specs);

			resources.outputs.clear();
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int32Buffer(sOutputs))));
			createTestsForAllStages("sint_vector", defaultColors, defaultColors, fragments, pcs, resources, extensions, testGroup, requiredFeatures);
		}
		{  // signed int
			map<string, string>		specs;

			specs["base_type16"]	= "u16";
			specs["type16"]			= "v2u16";
			specs["type32"]			= "v2u32";
			specs["signed"]			= "0";
			specs["count"]			= "32";
			specs["convert"]		= "OpUConvert";

			fragments["testfun"]	= testFun.specialize(specs);
			fragments["pre_main"]	= preMain.specialize(specs);
			fragments["decoration"]	= decoration.specialize(specs);

			resources.outputs.clear();
			resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int32Buffer(uOutputs))));
			createTestsForAllStages("uint_vector", defaultColors, defaultColors, fragments, pcs, resources, extensions, testGroup, requiredFeatures);
		}
	}
}

void addGraphics16BitStorageUniformInt16To32Group (tcu::TestCaseGroup* testGroup)
{
	de::Random							rnd					(deStringHash(testGroup->getName()));
	map<string, string>					fragments;
	const deUint32						numDataPoints		= 256;
	RGBA								defaultColors[4];
	vector<deInt16>						inputs				= getInt16s(rnd, numDataPoints);
	vector<deInt32>						sOutputs;
	vector<deInt32>						uOutputs;
	GraphicsResources					resources;
	vector<string>						extensions;
	const deUint16						signBitMask			= 0x8000;
	const deUint32						signExtendMask		= 0xffff0000;
	const StringTemplate				capabilities		("OpCapability ${cap}\n");

	sOutputs.reserve(inputs.size());
	uOutputs.reserve(inputs.size());

	for (deUint32 numNdx = 0; numNdx < inputs.size(); ++numNdx)
	{
		uOutputs.push_back(static_cast<deUint16>(inputs[numNdx]));
		if (inputs[numNdx] & signBitMask)
			sOutputs.push_back(static_cast<deInt32>(inputs[numNdx] | signExtendMask));
		else
			sOutputs.push_back(static_cast<deInt32>(inputs[numNdx]));
	}

	resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int16Buffer(inputs))));

	extensions.push_back("VK_KHR_16bit_storage");
	fragments["extension"]	= "OpExtension \"SPV_KHR_16bit_storage\"";

	getDefaultColors(defaultColors);

	struct IntegerFacts
	{
		const char*	name;
		const char*	type32;
		const char*	type16;
		const char* opcode;
		bool		isSigned;
	};

	const IntegerFacts	intFacts[]	=
	{
		{"sint",	"%i32",		"%i16",		"OpSConvert",	true},
		{"uint",	"%u32",		"%u16",		"OpUConvert",	false},
	};

	const StringTemplate scalarPreMain		(
			"${itype16} = OpTypeInt 16 ${signed}\n"
			" %c_i32_256 = OpConstant %i32 256\n"
			"   %up_i32 = OpTypePointer Uniform ${itype32}\n"
			"   %up_i16 = OpTypePointer Uniform ${itype16}\n"
			"   %ra_i32 = OpTypeArray ${itype32} %c_i32_256\n"
			"   %ra_i16 = OpTypeArray ${itype16} %c_i32_256\n"
			"   %SSBO32 = OpTypeStruct %ra_i32\n"
			"   %SSBO16 = OpTypeStruct %ra_i16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n");

	const StringTemplate scalarDecoration		(
			"OpDecorate %ra_i32 ArrayStride 4\n"
			"OpDecorate %ra_i16 ArrayStride 2\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpDecorate %SSBO16 ${indecor}\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 1\n"
			"OpDecorate %ssbo16 Binding 0\n");

	const StringTemplate scalarTestFunc	(
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_256\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"%write = OpLabel\n"
			"   %30 = OpLoad %i32 %i\n"
			"  %src = OpAccessChain %up_i16 %ssbo16 %c_i32_0 %30\n"
			"%val16 = OpLoad ${itype16} %src\n"
			"%val32 = ${convert} ${itype32} %val16\n"
			"  %dst = OpAccessChain %up_i32 %ssbo32 %c_i32_0 %30\n"
			"         OpStore %dst %val32\n"
			"         OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"
			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n");

	const StringTemplate vecPreMain		(
			"${itype16} = OpTypeInt 16 ${signed}\n"
			"%c_i32_128 = OpConstant %i32 128\n"
			"%v2itype16 = OpTypeVector ${itype16} 2\n"
			" %up_v2i32 = OpTypePointer Uniform ${v2itype32}\n"
			" %up_v2i16 = OpTypePointer Uniform %v2itype16\n"
			" %ra_v2i32 = OpTypeArray ${v2itype32} %c_i32_128\n"
			" %ra_v2i16 = OpTypeArray %v2itype16 %c_i32_128\n"
			"   %SSBO32 = OpTypeStruct %ra_v2i32\n"
			"   %SSBO16 = OpTypeStruct %ra_v2i16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n");

	const StringTemplate vecDecoration		(
			"OpDecorate %ra_v2i32 ArrayStride 8\n"
			"OpDecorate %ra_v2i16 ArrayStride 4\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpDecorate %SSBO16 ${indecor}\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 1\n"
			"OpDecorate %ssbo16 Binding 0\n");

	const StringTemplate vecTestFunc	(
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_128\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"%write = OpLabel\n"
			"   %30 = OpLoad %i32 %i\n"
			"  %src = OpAccessChain %up_v2i16 %ssbo16 %c_i32_0 %30\n"
			"%val16 = OpLoad %v2itype16 %src\n"
			"%val32 = ${convert} ${v2itype32} %val16\n"
			"  %dst = OpAccessChain %up_v2i32 %ssbo32 %c_i32_0 %30\n"
			"         OpStore %dst %val32\n"
			"         OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"
			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n");

	struct Category
	{
		const char*				name;
		const StringTemplate&	preMain;
		const StringTemplate&	decoration;
		const StringTemplate&	testFunction;
	};

	const Category		categories[]	=
	{
		{"scalar", scalarPreMain, scalarDecoration, scalarTestFunc},
		{"vector", vecPreMain, vecDecoration, vecTestFunc},
	};

	for (deUint32 catIdx = 0; catIdx < DE_LENGTH_OF_ARRAY(categories); ++catIdx)
		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
			for (deUint32 factIdx = 0; factIdx < DE_LENGTH_OF_ARRAY(intFacts); ++factIdx)
			{
				map<string, string>	specs;
				string				name		= string(CAPABILITIES[capIdx].name) + "_" + categories[catIdx].name + "_" + intFacts[factIdx].name;

				specs["cap"]					= CAPABILITIES[capIdx].cap;
				specs["indecor"]				= CAPABILITIES[capIdx].decor;
				specs["itype32"]				= intFacts[factIdx].type32;
				specs["v2itype32"]				= "%v2" + string(intFacts[factIdx].type32).substr(1);
				specs["itype16"]				= intFacts[factIdx].type16;
				if (intFacts[factIdx].isSigned)
					specs["signed"]				= "1";
				else
					specs["signed"]				= "0";
				specs["convert"]				= intFacts[factIdx].opcode;

				fragments["pre_main"]			= categories[catIdx].preMain.specialize(specs);
				fragments["testfun"]			= categories[catIdx].testFunction.specialize(specs);
				fragments["capability"]			= capabilities.specialize(specs);
				fragments["decoration"]			= categories[catIdx].decoration.specialize(specs);

				resources.inputs.back().first	= CAPABILITIES[capIdx].dtype;
				resources.outputs.clear();
				if (intFacts[factIdx].isSigned)
					resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int32Buffer(sOutputs))));
				else
					resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Int32Buffer(uOutputs))));

				createTestsForAllStages(name, defaultColors, defaultColors, fragments, resources, extensions, testGroup, get16BitStorageFeatures(CAPABILITIES[capIdx].name));
			}
}

void addGraphics16BitStorageUniformFloat16To32Group (tcu::TestCaseGroup* testGroup)
{
	de::Random							rnd					(deStringHash(testGroup->getName()));
	map<string, string>					fragments;
	GraphicsResources					resources;
	vector<string>						extensions;
	const deUint32						numDataPoints		= 256;
	RGBA								defaultColors[4];
	const StringTemplate				capabilities		("OpCapability ${cap}\n");
	vector<deFloat16>					float16Data			= getFloat16s(rnd, numDataPoints);
	vector<float>						float32Data;

	float32Data.reserve(numDataPoints);
	for (deUint32 numIdx = 0; numIdx < numDataPoints; ++numIdx)
		float32Data.push_back(deFloat16To32(float16Data[numIdx]));

	resources.inputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float16Buffer(float16Data))));
	resources.outputs.push_back(std::make_pair(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufferSp(new Float32Buffer(float32Data))));
	resources.verifyIO = check32BitFloats;

	extensions.push_back("VK_KHR_16bit_storage");
	fragments["extension"]	= "OpExtension \"SPV_KHR_16bit_storage\"";

	getDefaultColors(defaultColors);

	{ // scalar cases
		fragments["pre_main"]				=
			"      %f16 = OpTypeFloat 16\n"
			"%c_i32_256 = OpConstant %i32 256\n"
			"   %up_f32 = OpTypePointer Uniform %f32\n"
			"   %up_f16 = OpTypePointer Uniform %f16\n"
			"   %ra_f32 = OpTypeArray %f32 %c_i32_256\n"
			"   %ra_f16 = OpTypeArray %f16 %c_i32_256\n"
			"   %SSBO32 = OpTypeStruct %ra_f32\n"
			"   %SSBO16 = OpTypeStruct %ra_f16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n";

		const StringTemplate decoration		(
			"OpDecorate %ra_f32 ArrayStride 4\n"
			"OpDecorate %ra_f16 ArrayStride 2\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpDecorate %SSBO16 ${indecor}\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 1\n"
			"OpDecorate %ssbo16 Binding 0\n");

		// ssbo32[] <- convert ssbo16[] to 32bit float
		fragments["testfun"]				=
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_256\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"%write = OpLabel\n"
			"   %30 = OpLoad %i32 %i\n"
			"  %src = OpAccessChain %up_f16 %ssbo16 %c_i32_0 %30\n"
			"%val16 = OpLoad %f16 %src\n"
			"%val32 = OpFConvert %f32 %val16\n"
			"  %dst = OpAccessChain %up_f32 %ssbo32 %c_i32_0 %30\n"
			"         OpStore %dst %val32\n"
			"         OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"

			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n";

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
		{
			map<string, string>	specs;
			string				testName	= string(CAPABILITIES[capIdx].name) + "_scalar_float";

			specs["cap"]					= CAPABILITIES[capIdx].cap;
			specs["indecor"]				= CAPABILITIES[capIdx].decor;

			fragments["capability"]			= capabilities.specialize(specs);
			fragments["decoration"]			= decoration.specialize(specs);

			resources.inputs.back().first	= CAPABILITIES[capIdx].dtype;

			createTestsForAllStages(testName, defaultColors, defaultColors, fragments, resources, extensions, testGroup, get16BitStorageFeatures(CAPABILITIES[capIdx].name));
		}
	}

	{ // vector cases
		fragments["pre_main"]				=
			"      %f16 = OpTypeFloat 16\n"
			"%c_i32_128 = OpConstant %i32 128\n"
			"	 %v2f16 = OpTypeVector %f16 2\n"
			" %up_v2f32 = OpTypePointer Uniform %v2f32\n"
			" %up_v2f16 = OpTypePointer Uniform %v2f16\n"
			" %ra_v2f32 = OpTypeArray %v2f32 %c_i32_128\n"
			" %ra_v2f16 = OpTypeArray %v2f16 %c_i32_128\n"
			"   %SSBO32 = OpTypeStruct %ra_v2f32\n"
			"   %SSBO16 = OpTypeStruct %ra_v2f16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n";

		const StringTemplate decoration		(
			"OpDecorate %ra_v2f32 ArrayStride 8\n"
			"OpDecorate %ra_v2f16 ArrayStride 4\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpDecorate %SSBO16 ${indecor}\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 1\n"
			"OpDecorate %ssbo16 Binding 0\n");

		// ssbo32[] <- convert ssbo16[] to 32bit float
		fragments["testfun"]				=
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_128\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"%write = OpLabel\n"
			"   %30 = OpLoad %i32 %i\n"
			"  %src = OpAccessChain %up_v2f16 %ssbo16 %c_i32_0 %30\n"
			"%val16 = OpLoad %v2f16 %src\n"
			"%val32 = OpFConvert %v2f32 %val16\n"
			"  %dst = OpAccessChain %up_v2f32 %ssbo32 %c_i32_0 %30\n"
			"         OpStore %dst %val32\n"
			"         OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"

			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n";

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
		{
			map<string, string>	specs;
			string				testName	= string(CAPABILITIES[capIdx].name) + "_vector_float";

			specs["cap"]					= CAPABILITIES[capIdx].cap;
			specs["indecor"]				= CAPABILITIES[capIdx].decor;

			fragments["capability"]			= capabilities.specialize(specs);
			fragments["decoration"]			= decoration.specialize(specs);

			resources.inputs.back().first	= CAPABILITIES[capIdx].dtype;

			createTestsForAllStages(testName, defaultColors, defaultColors, fragments, resources, extensions, testGroup, get16BitStorageFeatures(CAPABILITIES[capIdx].name));
		}
	}

	{ // matrix cases
		fragments["pre_main"]				=
			" %c_i32_32 = OpConstant %i32 32\n"
			"      %f16 = OpTypeFloat 16\n"
			"    %v2f16 = OpTypeVector %f16 2\n"
			"  %m4x2f32 = OpTypeMatrix %v2f32 4\n"
			"  %m4x2f16 = OpTypeMatrix %v2f16 4\n"
			" %up_v2f32 = OpTypePointer Uniform %v2f32\n"
			" %up_v2f16 = OpTypePointer Uniform %v2f16\n"
			"%a8m4x2f32 = OpTypeArray %m4x2f32 %c_i32_32\n"
			"%a8m4x2f16 = OpTypeArray %m4x2f16 %c_i32_32\n"
			"   %SSBO32 = OpTypeStruct %a8m4x2f32\n"
			"   %SSBO16 = OpTypeStruct %a8m4x2f16\n"
			"%up_SSBO32 = OpTypePointer Uniform %SSBO32\n"
			"%up_SSBO16 = OpTypePointer Uniform %SSBO16\n"
			"   %ssbo32 = OpVariable %up_SSBO32 Uniform\n"
			"   %ssbo16 = OpVariable %up_SSBO16 Uniform\n";

		const StringTemplate decoration		(
			"OpDecorate %a8m4x2f32 ArrayStride 32\n"
			"OpDecorate %a8m4x2f16 ArrayStride 16\n"
			"OpMemberDecorate %SSBO32 0 Offset 0\n"
			"OpMemberDecorate %SSBO32 0 ColMajor\n"
			"OpMemberDecorate %SSBO32 0 MatrixStride 8\n"
			"OpMemberDecorate %SSBO16 0 Offset 0\n"
			"OpMemberDecorate %SSBO16 0 ColMajor\n"
			"OpMemberDecorate %SSBO16 0 MatrixStride 4\n"
			"OpDecorate %SSBO32 BufferBlock\n"
			"OpDecorate %SSBO16 ${indecor}\n"
			"OpDecorate %ssbo32 DescriptorSet 0\n"
			"OpDecorate %ssbo16 DescriptorSet 0\n"
			"OpDecorate %ssbo32 Binding 1\n"
			"OpDecorate %ssbo16 Binding 0\n");

		fragments["testfun"]				=
			"%test_code = OpFunction %v4f32 None %v4f32_function\n"
			"    %param = OpFunctionParameter %v4f32\n"

			"%entry = OpLabel\n"
			"    %i = OpVariable %fp_i32 Function\n"
			"         OpStore %i %c_i32_0\n"
			"         OpBranch %loop\n"

			" %loop = OpLabel\n"
			"   %15 = OpLoad %i32 %i\n"
			"   %lt = OpSLessThan %bool %15 %c_i32_32\n"
			"         OpLoopMerge %merge %inc None\n"
			"         OpBranchConditional %lt %write %merge\n"

			"  %write = OpLabel\n"
			"     %30 = OpLoad %i32 %i\n"
			"  %src_0 = OpAccessChain %up_v2f16 %ssbo16 %c_i32_0 %30 %c_i32_0\n"
			"  %src_1 = OpAccessChain %up_v2f16 %ssbo16 %c_i32_0 %30 %c_i32_1\n"
			"  %src_2 = OpAccessChain %up_v2f16 %ssbo16 %c_i32_0 %30 %c_i32_2\n"
			"  %src_3 = OpAccessChain %up_v2f16 %ssbo16 %c_i32_0 %30 %c_i32_3\n"
			"%val16_0 = OpLoad %v2f16 %src_0\n"
			"%val16_1 = OpLoad %v2f16 %src_1\n"
			"%val16_2 = OpLoad %v2f16 %src_2\n"
			"%val16_3 = OpLoad %v2f16 %src_3\n"
			"%val32_0 = OpFConvert %v2f32 %val16_0\n"
			"%val32_1 = OpFConvert %v2f32 %val16_1\n"
			"%val32_2 = OpFConvert %v2f32 %val16_2\n"
			"%val32_3 = OpFConvert %v2f32 %val16_3\n"
			"  %dst_0 = OpAccessChain %up_v2f32 %ssbo32 %c_i32_0 %30 %c_i32_0\n"
			"  %dst_1 = OpAccessChain %up_v2f32 %ssbo32 %c_i32_0 %30 %c_i32_1\n"
			"  %dst_2 = OpAccessChain %up_v2f32 %ssbo32 %c_i32_0 %30 %c_i32_2\n"
			"  %dst_3 = OpAccessChain %up_v2f32 %ssbo32 %c_i32_0 %30 %c_i32_3\n"
			"           OpStore %dst_0 %val32_0\n"
			"           OpStore %dst_1 %val32_1\n"
			"           OpStore %dst_2 %val32_2\n"
			"           OpStore %dst_3 %val32_3\n"
			"           OpBranch %inc\n"

			"  %inc = OpLabel\n"
			"   %37 = OpLoad %i32 %i\n"
			"   %39 = OpIAdd %i32 %37 %c_i32_1\n"
			"         OpStore %i %39\n"
			"         OpBranch %loop\n"

			"%merge = OpLabel\n"
			"         OpReturnValue %param\n"

			"OpFunctionEnd\n";

		for (deUint32 capIdx = 0; capIdx < DE_LENGTH_OF_ARRAY(CAPABILITIES); ++capIdx)
		{
			map<string, string>	specs;
			string				testName	= string(CAPABILITIES[capIdx].name) + "_matrix_float";

			specs["cap"]					= CAPABILITIES[capIdx].cap;
			specs["indecor"]				= CAPABILITIES[capIdx].decor;

			fragments["capability"]			= capabilities.specialize(specs);
			fragments["decoration"]			= decoration.specialize(specs);

			resources.inputs.back().first	= CAPABILITIES[capIdx].dtype;

			createTestsForAllStages(testName, defaultColors, defaultColors, fragments, resources, extensions, testGroup, get16BitStorageFeatures(CAPABILITIES[capIdx].name));
		}
	}
}

} // anonymous

tcu::TestCaseGroup* create16BitStorageComputeGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "16bit_storage", "Compute tests for VK_KHR_16bit_storage extension"));
	addTestGroup(group.get(), "uniform_32_to_16", "32bit floats/ints to 16bit tests under capability StorageUniform{|BufferBlock}", addCompute16bitStorageUniform32To16Group);
	addTestGroup(group.get(), "uniform_16_to_32", "16bit floats/ints to 32bit tests under capability StorageUniform{|BufferBlock}", addCompute16bitStorageUniform16To32Group);
	addTestGroup(group.get(), "push_constant_16_to_32", "16bit floats/ints to 32bit tests under capability StoragePushConstant16", addCompute16bitStoragePushConstant16To32Group);

	return group.release();
}

tcu::TestCaseGroup* create16BitStorageGraphicsGroup (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group		(new tcu::TestCaseGroup(testCtx, "16bit_storage", "Graphics tests for VK_KHR_16bit_storage extension"));

	addTestGroup(group.get(), "uniform_float_32_to_16", "32-bit floats into 16-bit tests under capability StorageUniform{|BufferBlock}16", addGraphics16BitStorageUniformFloat32To16Group);
	addTestGroup(group.get(), "uniform_float_16_to_32", "16-bit floats into 32-bit testsunder capability StorageUniform{|BufferBlock}16", addGraphics16BitStorageUniformFloat16To32Group);
	addTestGroup(group.get(), "uniform_int_32_to_16", "32-bit int into 16-bit tests under capability StorageUniform{|BufferBlock}16", addGraphics16BitStorageUniformInt32To16Group);
	addTestGroup(group.get(), "uniform_int_16_to_32", "16-bit int into 32-bit tests under capability StorageUniform{|BufferBlock}16", addGraphics16BitStorageUniformInt16To32Group);
	addTestGroup(group.get(), "input_output_float_32_to_16", "32-bit floats into 16-bit tests under capability StorageInputOutput16", addGraphics16BitStorageInputOutputFloat32To16Group);
	addTestGroup(group.get(), "input_output_float_16_to_32", "16-bit floats into 32-bit tests under capability StorageInputOutput16", addGraphics16BitStorageInputOutputFloat16To32Group);
	addTestGroup(group.get(), "input_output_int_32_to_16", "32-bit int into 16-bit tests under capability StorageInputOutput16", addGraphics16BitStorageInputOutputInt32To16Group);
	addTestGroup(group.get(), "input_output_int_16_to_32", "16-bit int into 32-bit tests under capability StorageInputOutput16", addGraphics16BitStorageInputOutputInt16To32Group);
	addTestGroup(group.get(), "push_constant_float_16_to_32", "16-bit floats into 32-bit tests under capability StoragePushConstant16", addGraphics16BitStoragePushConstantFloat16To32Group);
	addTestGroup(group.get(), "push_constant_int_16_to_32", "16-bit int into 32-bit tests under capability StoragePushConstant16", addGraphics16BitStoragePushConstantInt16To32Group);

	return group.release();
}

} // SpirVAssembly
} // vkt
