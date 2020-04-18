#ifndef _VKTSPVASMCOMPUTESHADERTESTUTIL_HPP
#define _VKTSPVASMCOMPUTESHADERTESTUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief Compute Shader Based Test Case Utility Structs/Functions
 *//*--------------------------------------------------------------------*/

#include "deDefs.h"
#include "deFloat16.h"
#include "deRandom.hpp"
#include "deSharedPtr.hpp"
#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include "vkMemUtil.hpp"
#include "vktSpvAsmUtils.hpp"

#include <string>
#include <vector>
#include <map>

using namespace vk;

namespace vkt
{
namespace SpirVAssembly
{

enum OpAtomicType
{
	OPATOMIC_IADD = 0,
	OPATOMIC_ISUB,
	OPATOMIC_IINC,
	OPATOMIC_IDEC,
	OPATOMIC_LOAD,
	OPATOMIC_STORE,
	OPATOMIC_COMPEX,

	OPATOMIC_LAST
};

enum BufferType
{
	BUFFERTYPE_INPUT = 0,
	BUFFERTYPE_EXPECTED,

	BUFFERTYPE_LAST
};

static void fillRandomScalars (de::Random& rnd, deInt32 minValue, deInt32 maxValue, deInt32* dst, deInt32 numValues)
{
	for (int i = 0; i < numValues; i++)
		dst[i] = rnd.getInt(minValue, maxValue);
}

typedef de::MovePtr<vk::Allocation>			AllocationMp;
typedef de::SharedPtr<vk::Allocation>		AllocationSp;

/*--------------------------------------------------------------------*//*!
 * \brief Abstract class for an input/output storage buffer object
 *//*--------------------------------------------------------------------*/
class BufferInterface
{
public:
	virtual				~BufferInterface	(void)				{}

	virtual void		getBytes			(std::vector<deUint8>& bytes) const = 0;
	virtual size_t		getByteSize			(void) const = 0;
};

typedef de::SharedPtr<BufferInterface>		BufferSp;

/*--------------------------------------------------------------------*//*!
* \brief Concrete class for an input/output storage buffer object used for OpAtomic tests
*//*--------------------------------------------------------------------*/
class OpAtomicBuffer : public BufferInterface
{
public:
						OpAtomicBuffer		(const deUint32 numInputElements, const deUint32 numOuptutElements, const OpAtomicType opAtomic, const BufferType type)
							: m_numInputElements	(numInputElements)
							, m_numOutputElements	(numOuptutElements)
							, m_opAtomic			(opAtomic)
							, m_type				(type)
						{}

	void getBytes (std::vector<deUint8>& bytes) const
	{
		std::vector<deInt32>	inputInts	(m_numInputElements, 0);
		de::Random				rnd			(m_opAtomic);

		fillRandomScalars(rnd, 1, 100, &inputInts.front(), m_numInputElements);

		// Return input values as is
		if (m_type == BUFFERTYPE_INPUT)
		{
			size_t					inputSize	= m_numInputElements * sizeof(deInt32);

			bytes.resize(inputSize);
			deMemcpy(&bytes.front(), &inputInts.front(), inputSize);
		}
		// Calculate expected output values
		else if (m_type == BUFFERTYPE_EXPECTED)
		{
			size_t					outputSize	= m_numOutputElements * sizeof(deInt32);
			bytes.resize(outputSize, 0xffu);

			for (size_t ndx = 0; ndx < m_numInputElements; ndx++)
			{
				deInt32* const bytesAsInt = reinterpret_cast<deInt32* const>(&bytes.front());

				switch (m_opAtomic)
				{
					case OPATOMIC_IADD:		bytesAsInt[0] += inputInts[ndx];						break;
					case OPATOMIC_ISUB:		bytesAsInt[0] -= inputInts[ndx];						break;
					case OPATOMIC_IINC:		bytesAsInt[0]++;										break;
					case OPATOMIC_IDEC:		bytesAsInt[0]--;										break;
					case OPATOMIC_LOAD:		bytesAsInt[ndx] = inputInts[ndx];						break;
					case OPATOMIC_STORE:	bytesAsInt[ndx] = inputInts[ndx];						break;
					case OPATOMIC_COMPEX:	bytesAsInt[ndx] = (inputInts[ndx] % 2) == 0 ? -1 : 1;	break;
					default:				DE_FATAL("Unknown OpAtomic type");
				}
			}
		}
		else
			DE_FATAL("Unknown buffer type");
	}

	size_t getByteSize (void) const
	{
		switch (m_type)
		{
			case BUFFERTYPE_INPUT:
				return m_numInputElements * sizeof(deInt32);
			case BUFFERTYPE_EXPECTED:
				return m_numOutputElements * sizeof(deInt32);
			default:
				DE_FATAL("Unknown buffer type");
				return 0;
		}
	}

private:
	const deUint32		m_numInputElements;
	const deUint32		m_numOutputElements;
	const OpAtomicType	m_opAtomic;
	const BufferType	m_type;
};

/*--------------------------------------------------------------------*//*!
 * \brief Concrete class for an input/output storage buffer object
 *//*--------------------------------------------------------------------*/
template<typename E>
class Buffer : public BufferInterface
{
public:
						Buffer				(const std::vector<E>& elements)
							: m_elements(elements)
						{}

	void getBytes (std::vector<deUint8>& bytes) const
	{
		const size_t size = m_elements.size() * sizeof(E);
		bytes.resize(size);
		deMemcpy(&bytes.front(), &m_elements.front(), size);
	}

	size_t getByteSize (void) const
	{
		return m_elements.size() * sizeof(E);
	}

private:
	std::vector<E>		m_elements;
};

DE_STATIC_ASSERT(sizeof(tcu::Vec4) == 4 * sizeof(float));

typedef Buffer<float>		Float32Buffer;
typedef Buffer<deFloat16>	Float16Buffer;
typedef Buffer<deInt64>		Int64Buffer;
typedef Buffer<deInt32>		Int32Buffer;
typedef Buffer<deInt16>		Int16Buffer;
typedef Buffer<tcu::Vec4>	Vec4Buffer;

typedef bool (*ComputeVerifyIOFunc) (const std::vector<BufferSp>&		inputs,
									 const std::vector<AllocationSp>&	outputAllocations,
									 const std::vector<BufferSp>&		expectedOutputs,
									 tcu::TestLog&						log);

/*--------------------------------------------------------------------*//*!
 * \brief Specification for a compute shader.
 *
 * This struct bundles SPIR-V assembly code, input and expected output
 * together.
 *//*--------------------------------------------------------------------*/
struct ComputeShaderSpec
{
	std::string								assembly;
	std::string								entryPoint;
	std::vector<BufferSp>					inputs;
	// Mapping from input index (in the inputs field) to the descriptor type.
	std::map<deUint32, VkDescriptorType>	inputTypes;
	std::vector<BufferSp>					outputs;
	tcu::IVec3								numWorkGroups;
	std::vector<deUint32>					specConstants;
	BufferSp								pushConstants;
	std::vector<std::string>				extensions;
	VulkanFeatures							requestedVulkanFeatures;
	qpTestResult							failResult;
	std::string								failMessage;
	// If null, a default verification will be performed by comparing the memory pointed to by outputAllocations
	// and the contents of expectedOutputs. Otherwise the function pointed to by verifyIO will be called.
	// If true is returned, then the test case is assumed to have passed, if false is returned, then the test
	// case is assumed to have failed. Exact meaning of failure can be customized with failResult.
	ComputeVerifyIOFunc						verifyIO;

											ComputeShaderSpec (void)
												: entryPoint					("main")
												, pushConstants					(DE_NULL)
												, requestedVulkanFeatures		()
												, failResult					(QP_TEST_RESULT_FAIL)
												, failMessage					("Output doesn't match with expected")
												, verifyIO						(DE_NULL)
											{}
};

/*--------------------------------------------------------------------*//*!
 * \brief Helper functions for SPIR-V assembly shared by various tests
 *//*--------------------------------------------------------------------*/

const char* getComputeAsmShaderPreamble				(void);
const char* getComputeAsmShaderPreambleWithoutLocalSize         (void);
std::string getComputeAsmCommonTypes				(std::string blockStorageClass = "Uniform");
const char*	getComputeAsmCommonInt64Types			(void);

/*--------------------------------------------------------------------*//*!
 * Declares two uniform variables (indata, outdata) of type
 * "struct { float[] }". Depends on type "f32arr" (for "float[]").
 *//*--------------------------------------------------------------------*/
const char* getComputeAsmInputOutputBuffer			(void);
/*--------------------------------------------------------------------*//*!
 * Declares buffer type and layout for uniform variables indata and
 * outdata. Both of them are SSBO bounded to descriptor set 0.
 * indata is at binding point 0, while outdata is at 1.
 *//*--------------------------------------------------------------------*/
const char* getComputeAsmInputOutputBufferTraits	(void);

} // SpirVAssembly
} // vkt

#endif // _VKTSPVASMCOMPUTESHADERTESTUTIL_HPP
