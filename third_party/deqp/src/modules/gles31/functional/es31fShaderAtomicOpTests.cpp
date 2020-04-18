/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Shader atomic operation tests.
 *//*--------------------------------------------------------------------*/

#include "es31fShaderAtomicOpTests.hpp"
#include "gluShaderProgram.hpp"
#include "gluShaderUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluObjectWrapper.hpp"
#include "gluProgramInterfaceQuery.hpp"
#include "tcuVector.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuFormatUtil.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include <algorithm>
#include <set>

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::string;
using std::vector;
using tcu::TestLog;
using tcu::UVec3;
using std::set;
using namespace glu;

template<typename T, int Size>
static inline T product (const tcu::Vector<T, Size>& v)
{
	T res = v[0];
	for (int ndx = 1; ndx < Size; ndx++)
		res *= v[ndx];
	return res;
}

class ShaderAtomicOpCase : public TestCase
{
public:
							ShaderAtomicOpCase	(Context& context, const char* name, const char* funcName, AtomicOperandType operandType, DataType type, Precision precision, const UVec3& workGroupSize);
							~ShaderAtomicOpCase	(void);

	void					init				(void);
	void					deinit				(void);
	IterateResult			iterate				(void);

protected:
	virtual void			getInputs			(int numValues, int stride, void* inputs) const = 0;
	virtual bool			verify				(int numValues, int inputStride, const void* inputs, int outputStride, const void* outputs, int groupStride, const void* groupOutputs) const = 0;

	const string			m_funcName;
	const AtomicOperandType	m_operandType;
	const DataType			m_type;
	const Precision			m_precision;

	const UVec3				m_workGroupSize;
	const UVec3				m_numWorkGroups;

	deUint32				m_initialValue;

private:
							ShaderAtomicOpCase	(const ShaderAtomicOpCase& other);
	ShaderAtomicOpCase&		operator=			(const ShaderAtomicOpCase& other);

	ShaderProgram*			m_program;
};

ShaderAtomicOpCase::ShaderAtomicOpCase (Context& context, const char* name, const char* funcName, AtomicOperandType operandType, DataType type, Precision precision, const UVec3& workGroupSize)
	: TestCase			(context, name, funcName)
	, m_funcName		(funcName)
	, m_operandType		(operandType)
	, m_type			(type)
	, m_precision		(precision)
	, m_workGroupSize	(workGroupSize)
	, m_numWorkGroups	(4,4,4)
	, m_initialValue	(0)
	, m_program			(DE_NULL)
{
}

ShaderAtomicOpCase::~ShaderAtomicOpCase (void)
{
	ShaderAtomicOpCase::deinit();
}

void ShaderAtomicOpCase::init (void)
{
	const bool			isSSBO		= m_operandType == ATOMIC_OPERAND_BUFFER_VARIABLE;
	const char*			precName	= getPrecisionName(m_precision);
	const char*			typeName	= getDataTypeName(m_type);

	const DataType		outType		= isSSBO ? m_type : glu::TYPE_UINT;
	const char*			outTypeName	= getDataTypeName(outType);

	const deUint32		numValues	= product(m_workGroupSize)*product(m_numWorkGroups);
	std::ostringstream	src;

	src << glu::getGLSLVersionDeclaration(getContextTypeGLSLVersion(m_context.getRenderContext().getType())) << "\n"
		<< "layout(local_size_x = " << m_workGroupSize.x()
		<< ", local_size_y = " << m_workGroupSize.y()
		<< ", local_size_z = " << m_workGroupSize.z() << ") in;\n"
		<< "layout(binding = 0) buffer InOut\n"
		<< "{\n"
		<< "	" << precName << " " << typeName << " inputValues[" << numValues << "];\n"
		<< "	" << precName << " " << outTypeName << " outputValues[" << numValues << "];\n"
		<< "	" << (isSSBO ? "coherent " : "") << precName << " " << outTypeName << " groupValues[" << product(m_numWorkGroups) << "];\n"
		<< "} sb_inout;\n";

	if (!isSSBO)
		src << "shared " << precName << " " << typeName << " s_var;\n";

	src << "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "	uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		<< "	uint globalOffs = localSize*globalNdx;\n"
		<< "	uint offset     = globalOffs + gl_LocalInvocationIndex;\n"
		<< "\n";

	if (isSSBO)
	{
		DE_ASSERT(outType == m_type);
		src << "	sb_inout.outputValues[offset] = " << m_funcName << "(sb_inout.groupValues[globalNdx], sb_inout.inputValues[offset]);\n";
	}
	else
	{
		const string		castBeg	= outType != m_type ? (string(outTypeName) + "(") : string("");
		const char* const	castEnd	= outType != m_type ? ")" : "";

		src << "	if (gl_LocalInvocationIndex == 0u)\n"
			<< "		s_var = " << typeName << "(" << tcu::toHex(m_initialValue) << "u);\n"
			<< "	barrier();\n"
			<< "	" << precName << " " << typeName << " res = " << m_funcName << "(s_var, sb_inout.inputValues[offset]);\n"
			<< "	sb_inout.outputValues[offset] = " << castBeg << "res" << castEnd << ";\n"
			<< "	barrier();\n"
			<< "	if (gl_LocalInvocationIndex == 0u)\n"
			<< "		sb_inout.groupValues[globalNdx] = " << castBeg << "s_var" << castEnd << ";\n";
	}

	src << "}\n";

	DE_ASSERT(!m_program);
	m_program = new ShaderProgram(m_context.getRenderContext(), ProgramSources() << ComputeSource(src.str()));

	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
	{
		delete m_program;
		m_program = DE_NULL;
		throw tcu::TestError("Compile failed");
	}
}

void ShaderAtomicOpCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

ShaderAtomicOpCase::IterateResult ShaderAtomicOpCase::iterate (void)
{
	const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
	const deUint32				program			= m_program->getProgram();
	const Buffer				inoutBuffer		(m_context.getRenderContext());
	const deUint32				blockNdx		= gl.getProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "InOut");
	const InterfaceBlockInfo	blockInfo		= getProgramInterfaceBlockInfo(gl, program, GL_SHADER_STORAGE_BLOCK, blockNdx);
	const deUint32				inVarNdx		= gl.getProgramResourceIndex(program, GL_BUFFER_VARIABLE, "InOut.inputValues[0]");
	const InterfaceVariableInfo	inVarInfo		= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, inVarNdx);
	const deUint32				outVarNdx		= gl.getProgramResourceIndex(program, GL_BUFFER_VARIABLE, "InOut.outputValues[0]");
	const InterfaceVariableInfo	outVarInfo		= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, outVarNdx);
	const deUint32				groupVarNdx		= gl.getProgramResourceIndex(program, GL_BUFFER_VARIABLE, "InOut.groupValues[0]");
	const InterfaceVariableInfo	groupVarInfo	= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, groupVarNdx);
	const deUint32				numValues		= product(m_workGroupSize)*product(m_numWorkGroups);

	TCU_CHECK(inVarInfo.arraySize == numValues &&
			  outVarInfo.arraySize == numValues &&
			  groupVarInfo.arraySize == product(m_numWorkGroups));

	gl.useProgram(program);

	// Setup buffer.
	{
		vector<deUint8> bufData(blockInfo.dataSize);
		std::fill(bufData.begin(), bufData.end(), 0);

		getInputs((int)numValues, (int)inVarInfo.arrayStride, &bufData[0] + inVarInfo.offset);

		if (m_operandType == ATOMIC_OPERAND_BUFFER_VARIABLE)
		{
			for (deUint32 valNdx = 0; valNdx < product(m_numWorkGroups); valNdx++)
				*(deUint32*)(&bufData[0] + groupVarInfo.offset + groupVarInfo.arrayStride*valNdx) = m_initialValue;
		}

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *inoutBuffer);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockInfo.dataSize, &bufData[0], GL_STATIC_READ);
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *inoutBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
	}

	gl.dispatchCompute(m_numWorkGroups.x(), m_numWorkGroups.y(), m_numWorkGroups.z());

	// Read back and compare
	{
		const void*		resPtr		= gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, blockInfo.dataSize, GL_MAP_READ_BIT);
		bool			isOk		= true;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange()");
		TCU_CHECK(resPtr);

		isOk = verify((int)numValues,
					  (int)inVarInfo.arrayStride, (const deUint8*)resPtr + inVarInfo.offset,
					  (int)outVarInfo.arrayStride, (const deUint8*)resPtr + outVarInfo.offset,
					  (int)groupVarInfo.arrayStride, (const deUint8*)resPtr + groupVarInfo.offset);

		gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer()");

		m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
								isOk ? "Pass"				: "Comparison failed");
	}

	return STOP;
}

class ShaderAtomicAddCase : public ShaderAtomicOpCase
{
public:
	ShaderAtomicAddCase (Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision)
		: ShaderAtomicOpCase(context, name, "atomicAdd", operandType, type, precision, UVec3(3,2,1))
	{
		m_initialValue = 1;
	}

protected:
	void getInputs (int numValues, int stride, void* inputs) const
	{
		de::Random	rnd			(deStringHash(getName()));
		const int	maxVal		= m_precision == PRECISION_LOWP ? 2 : 32;
		const int	minVal		= 1;

		// \todo [2013-09-04 pyry] Negative values!

		for (int valNdx = 0; valNdx < numValues; valNdx++)
			*(int*)((deUint8*)inputs + stride*valNdx) = rnd.getInt(minVal, maxVal);
	}

	bool verify (int numValues, int inputStride, const void* inputs, int outputStride, const void* outputs, int groupStride, const void* groupOutputs) const
	{
		const int	workGroupSize	= (int)product(m_workGroupSize);
		const int	numWorkGroups	= numValues/workGroupSize;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int	groupOffset		= groupNdx*workGroupSize;
			const int	groupOutput		= *(const deInt32*)((const deUint8*)groupOutputs + groupNdx*groupStride);
			set<int>	outValues;
			bool		maxFound		= false;
			int			valueSum		= (int)m_initialValue;

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const int inputValue = *(const deInt32*)((const deUint8*)inputs + inputStride*(groupOffset+localNdx));
				valueSum += inputValue;
			}

			if (groupOutput != valueSum)
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ": expected sum " << valueSum << ", got " << groupOutput << TestLog::EndMessage;
				return false;
			}

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const int	inputValue		= *(const deInt32*)((const deUint8*)inputs + inputStride*(groupOffset+localNdx));
				const int	outputValue		= *(const deInt32*)((const deUint8*)outputs + outputStride*(groupOffset+localNdx));

				if (!de::inRange(outputValue, (int)m_initialValue, valueSum-inputValue))
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": expected value in range [" << m_initialValue << ", " << (valueSum-inputValue)
														   << "], got " << outputValue
									   << TestLog::EndMessage;
					return false;
				}

				if (outValues.find(outputValue) != outValues.end())
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": found duplicate value " << outputValue
									   << TestLog::EndMessage;
					return false;
				}

				outValues.insert(outputValue);
				if (outputValue == valueSum-inputValue)
					maxFound = true;
			}

			if (!maxFound)
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: could not find maximum expected value from group " << groupNdx << TestLog::EndMessage;
				return false;
			}

			if (outValues.find((int)m_initialValue) == outValues.end())
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: could not find initial value from group " << groupNdx << TestLog::EndMessage;
				return false;
			}
		}

		return true;
	}
};


static int getPrecisionNumIntegerBits (glu::Precision precision)
{
	switch (precision)
	{
		case glu::PRECISION_HIGHP:		return 32;
		case glu::PRECISION_MEDIUMP:	return 16;
		case glu::PRECISION_LOWP:		return 9;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

static deUint32 getPrecisionMask (int numPreciseBits)
{
	// \note: bit shift with larger or equal than var length is undefined, use 64 bit ints
	return (deUint32)((((deUint64)1u) << numPreciseBits) - 1) ;
}

static bool intEqualsAfterUintCast (deInt32 value, deUint32 casted, glu::Precision precision)
{
	// Bit format of 'casted' = [ uint -> highp uint promotion bits (0) ] [ sign extend bits (s) ] [ value bits ]
	//                                                                                             |--min len---|
	//                                                                    |---------------signed length---------|
	//                          |-------------------------------- highp uint length ----------------------------|

	const deUint32	reference		= (deUint32)value;
	const int		signBitOn		= value < 0;
	const int		numPreciseBits	= getPrecisionNumIntegerBits(precision);
	const deUint32	preciseMask		= getPrecisionMask(numPreciseBits);

	// Lowest N bits must match, N = minimum precision
	if ((reference & preciseMask) != (casted & preciseMask))
		return false;

	// Other lowest bits must match the sign and the remaining (topmost) if any must be 0
	for (int signedIntegerLength = numPreciseBits; signedIntegerLength <= 32; ++signedIntegerLength)
	{
		const deUint32 signBits = (signBitOn) ? (getPrecisionMask(signedIntegerLength)) : (0u);

		if ((signBits & ~preciseMask) == (casted & ~preciseMask))
			return true;
	}
	return false;
}

static bool containsAfterUintCast (const std::set<deInt32>& haystack, deUint32 needle, glu::Precision precision)
{
	for (std::set<deInt32>::const_iterator it = haystack.begin(); it != haystack.end(); ++it)
		if (intEqualsAfterUintCast(*it, needle, precision))
			return true;
	return false;
}

static bool containsAfterUintCast (const std::set<deUint32>& haystack, deInt32 needle, glu::Precision precision)
{
	for (std::set<deUint32>::const_iterator it = haystack.begin(); it != haystack.end(); ++it)
		if (intEqualsAfterUintCast(needle, *it, precision))
			return true;
	return false;
}

class ShaderAtomicMinCase : public ShaderAtomicOpCase
{
public:
	ShaderAtomicMinCase (Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision)
		: ShaderAtomicOpCase(context, name, "atomicMin", operandType, type, precision, UVec3(3,2,1))
	{
		m_initialValue = m_precision == PRECISION_LOWP ? 100 : 1000;
	}

protected:
	void getInputs (int numValues, int stride, void* inputs) const
	{
		de::Random	rnd			(deStringHash(getName()));
		const bool	isSigned	= m_type == TYPE_INT;
		const int	maxVal		= m_precision == PRECISION_LOWP ? 100 : 1000;
		const int	minVal		= isSigned ? -maxVal : 0;

		for (int valNdx = 0; valNdx < numValues; valNdx++)
			*(int*)((deUint8*)inputs + stride*valNdx) = rnd.getInt(minVal, maxVal);
	}

	bool verify (int numValues, int inputStride, const void* inputs, int outputStride, const void* outputs, int groupStride, const void* groupOutputs) const
	{
		const int	workGroupSize	= (int)product(m_workGroupSize);
		const int	numWorkGroups	= numValues/workGroupSize;
		bool		anyError		= false;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int		groupOffset		= groupNdx*workGroupSize;
			const deUint32	groupOutput		= *(const deUint32*)((const deUint8*)groupOutputs + groupNdx*groupStride);
			set<deInt32>	inValues;
			set<deUint32>	outValues;
			int				minValue		= (int)m_initialValue;

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deInt32 inputValue = *(const deInt32*)((const deUint8*)inputs + inputStride*(groupOffset+localNdx));
				inValues.insert(inputValue);
				minValue = de::min(inputValue, minValue);
			}

			if (!intEqualsAfterUintCast(minValue, groupOutput, m_precision))
			{
				m_testCtx.getLog()
					<< TestLog::Message
					<< "ERROR: at group " << groupNdx
					<< ": expected minimum " << minValue << " (" << tcu::Format::Hex<8>((deUint32)minValue) << ")"
					<< ", got " << groupOutput << " (" << tcu::Format::Hex<8>(groupOutput) << ")"
					<< TestLog::EndMessage;
				anyError = true;
			}

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deUint32 outputValue = *(const deUint32*)((const deUint8*)outputs + outputStride*(groupOffset+localNdx));

				if (!containsAfterUintCast(inValues, outputValue, m_precision) &&
					!intEqualsAfterUintCast((deInt32)m_initialValue, outputValue, m_precision))
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": found unexpected value " << outputValue
														   << " (" << tcu::Format::Hex<8>(outputValue) << ")"
									   << TestLog::EndMessage;
					anyError = true;
				}

				outValues.insert(outputValue);
			}

			if (!containsAfterUintCast(outValues, (int)m_initialValue, m_precision))
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: could not find initial value from group " << groupNdx << TestLog::EndMessage;
				anyError = true;
			}
		}

		return !anyError;
	}
};

class ShaderAtomicMaxCase : public ShaderAtomicOpCase
{
public:
	ShaderAtomicMaxCase (Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision)
		: ShaderAtomicOpCase(context, name, "atomicMax", operandType, type, precision, UVec3(3,2,1))
	{
		const bool isSigned = m_type == TYPE_INT;
		m_initialValue = isSigned ? (m_precision == PRECISION_LOWP ? -100 : -1000) : 0;
	}

protected:
	void getInputs (int numValues, int stride, void* inputs) const
	{
		de::Random	rnd			(deStringHash(getName()));
		const bool	isSigned	= m_type == TYPE_INT;
		const int	maxVal		= m_precision == PRECISION_LOWP ? 100 : 1000;
		const int	minVal		= isSigned ? -maxVal : 0;

		for (int valNdx = 0; valNdx < numValues; valNdx++)
			*(int*)((deUint8*)inputs + stride*valNdx) = rnd.getInt(minVal, maxVal);
	}

	bool verify (int numValues, int inputStride, const void* inputs, int outputStride, const void* outputs, int groupStride, const void* groupOutputs) const
	{
		const int	workGroupSize	= (int)product(m_workGroupSize);
		const int	numWorkGroups	= numValues/workGroupSize;
		bool		anyError		= false;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int		groupOffset		= groupNdx*workGroupSize;
			const deUint32	groupOutput		= *(const deUint32*)((const deUint8*)groupOutputs + groupNdx*groupStride);
			set<int>		inValues;
			set<deUint32>	outValues;
			int				maxValue		= (int)m_initialValue;

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deInt32 inputValue = *(const deInt32*)((const deUint8*)inputs + inputStride*(groupOffset+localNdx));
				inValues.insert(inputValue);
				maxValue = de::max(maxValue, inputValue);
			}

			if (!intEqualsAfterUintCast(maxValue, groupOutput, m_precision))
			{
				m_testCtx.getLog()
					<< TestLog::Message
					<< "ERROR: at group " << groupNdx
					<< ": expected maximum " << maxValue << " (" << tcu::Format::Hex<8>((deUint32)maxValue) << ")"
					<< ", got " << groupOutput << " (" << tcu::Format::Hex<8>(groupOutput) << ")"
					<< TestLog::EndMessage;
				anyError = true;
			}

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deUint32 outputValue = *(const deUint32*)((const deUint8*)outputs + outputStride*(groupOffset+localNdx));

				if (!containsAfterUintCast(inValues, outputValue, m_precision) &&
					!intEqualsAfterUintCast((deInt32)m_initialValue, outputValue, m_precision))
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": found unexpected value " << outputValue
														   << " (" << tcu::Format::Hex<8>(outputValue) << ")"
									   << TestLog::EndMessage;
					anyError = true;
				}

				outValues.insert(outputValue);
			}

			if (!containsAfterUintCast(outValues, (int)m_initialValue, m_precision))
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: could not find initial value from group " << groupNdx << TestLog::EndMessage;
				anyError = true;
			}
		}

		return !anyError;
	}
};

class ShaderAtomicAndCase : public ShaderAtomicOpCase
{
public:
	ShaderAtomicAndCase (Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision)
		: ShaderAtomicOpCase(context, name, "atomicAnd", operandType, type, precision, UVec3(3,2,1))
	{
		const int		numBits		= m_precision == PRECISION_HIGHP ? 32 :
									  m_precision == PRECISION_MEDIUMP ? 16 : 8;
		const deUint32	valueMask	= numBits == 32 ? ~0u : (1u<<numBits)-1u;
		m_initialValue = ~((1u<<(numBits-1u)) | 1u) & valueMask; // All bits except lowest and highest set.
	}

protected:
	void getInputs (int numValues, int stride, void* inputs) const
	{
		de::Random		rnd				(deStringHash(getName()));
		const int		workGroupSize	= (int)product(m_workGroupSize);
		const int		numWorkGroups	= numValues/workGroupSize;
		const int		numBits			= m_precision == PRECISION_HIGHP ? 32 :
										  m_precision == PRECISION_MEDIUMP ? 16 : 8;
		const deUint32	valueMask		= numBits == 32 ? ~0u : (1u<<numBits)-1u;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int		groupOffset		= groupNdx*workGroupSize;
			const deUint32	groupMask		= 1<<rnd.getInt(0, numBits-2); // One bit is always set.

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
				*(deUint32*)((deUint8*)inputs + stride*(groupOffset+localNdx)) = (rnd.getUint32() & valueMask) | groupMask;
		}
	}

	bool verify (int numValues, int inputStride, const void* inputs, int outputStride, const void* outputs, int groupStride, const void* groupOutputs) const
	{
		const int		workGroupSize	= (int)product(m_workGroupSize);
		const int		numWorkGroups	= numValues/workGroupSize;
		const int		numBits			= m_precision == PRECISION_HIGHP ? 32 :
										  m_precision == PRECISION_MEDIUMP ? 16 : 8;
		const deUint32	compareMask		= (m_type == TYPE_UINT || numBits == 32) ? ~0u : (1u<<numBits)-1u;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int		groupOffset		= groupNdx*workGroupSize;
			const deUint32	groupOutput		= *(const deUint32*)((const deUint8*)groupOutputs + groupNdx*groupStride);
			deUint32		expectedValue	= m_initialValue;

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deUint32 inputValue = *(const deUint32*)((const deUint8*)inputs + inputStride*(groupOffset+localNdx));
				expectedValue &= inputValue;
			}

			if ((groupOutput & compareMask) != (expectedValue & compareMask))
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ": expected " << tcu::toHex(expectedValue) << ", got " << tcu::toHex(groupOutput) << TestLog::EndMessage;
				return false;
			}

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deUint32 outputValue = *(const deUint32*)((const deUint8*)outputs + outputStride*(groupOffset+localNdx));

				if ((compareMask & (outputValue & ~m_initialValue)) != 0)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": found unexpected value " << tcu::toHex(outputValue)
									   << TestLog::EndMessage;
					return false;
				}
			}
		}

		return true;
	}
};

class ShaderAtomicOrCase : public ShaderAtomicOpCase
{
public:
	ShaderAtomicOrCase (Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision)
		: ShaderAtomicOpCase(context, name, "atomicOr", operandType, type, precision, UVec3(3,2,1))
	{
		m_initialValue = 1u; // Lowest bit set.
	}

protected:
	void getInputs (int numValues, int stride, void* inputs) const
	{
		de::Random		rnd				(deStringHash(getName()));
		const int		workGroupSize	= (int)product(m_workGroupSize);
		const int		numWorkGroups	= numValues/workGroupSize;
		const int		numBits			= m_precision == PRECISION_HIGHP ? 32 :
										  m_precision == PRECISION_MEDIUMP ? 16 : 8;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int groupOffset = groupNdx*workGroupSize;

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
				*(deUint32*)((deUint8*)inputs + stride*(groupOffset+localNdx)) = 1u<<rnd.getInt(0, numBits-1);
		}
	}

	bool verify (int numValues, int inputStride, const void* inputs, int outputStride, const void* outputs, int groupStride, const void* groupOutputs) const
	{
		const int		workGroupSize	= (int)product(m_workGroupSize);
		const int		numWorkGroups	= numValues/workGroupSize;
		const int		numBits			= m_precision == PRECISION_HIGHP ? 32 :
										  m_precision == PRECISION_MEDIUMP ? 16 : 8;
		const deUint32	compareMask		= (m_type == TYPE_UINT || numBits == 32) ? ~0u : (1u<<numBits)-1u;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int		groupOffset		= groupNdx*workGroupSize;
			const deUint32	groupOutput		= *(const deUint32*)((const deUint8*)groupOutputs + groupNdx*groupStride);
			deUint32		expectedValue	= m_initialValue;

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deUint32 inputValue = *(const deUint32*)((const deUint8*)inputs + inputStride*(groupOffset+localNdx));
				expectedValue |= inputValue;
			}

			if ((groupOutput & compareMask) != (expectedValue & compareMask))
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ": expected " << tcu::toHex(expectedValue) << ", got " << tcu::toHex(groupOutput) << TestLog::EndMessage;
				return false;
			}

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deUint32 outputValue = *(const deUint32*)((const deUint8*)outputs + outputStride*(groupOffset+localNdx));

				if ((compareMask & (outputValue & m_initialValue)) == 0)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": found unexpected value " << tcu::toHex(outputValue)
									   << TestLog::EndMessage;
					return false;
				}
			}
		}

		return true;
	}
};

class ShaderAtomicXorCase : public ShaderAtomicOpCase
{
public:
	ShaderAtomicXorCase (Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision)
		: ShaderAtomicOpCase(context, name, "atomicXor", operandType, type, precision, UVec3(3,2,1))
	{
		m_initialValue = 0;
	}

protected:
	void getInputs (int numValues, int stride, void* inputs) const
	{
		de::Random		rnd				(deStringHash(getName()));
		const int		workGroupSize	= (int)product(m_workGroupSize);
		const int		numWorkGroups	= numValues/workGroupSize;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int groupOffset = groupNdx*workGroupSize;

			// First uses random bit-pattern.
			*(deUint32*)((deUint8*)inputs + stride*(groupOffset)) = rnd.getUint32();

			// Rest have either all or no bits set.
			for (int localNdx = 1; localNdx < workGroupSize; localNdx++)
				*(deUint32*)((deUint8*)inputs + stride*(groupOffset+localNdx)) = rnd.getBool() ? ~0u : 0u;
		}
	}

	bool verify (int numValues, int inputStride, const void* inputs, int outputStride, const void* outputs, int groupStride, const void* groupOutputs) const
	{
		const int		workGroupSize	= (int)product(m_workGroupSize);
		const int		numWorkGroups	= numValues/workGroupSize;
		const int		numBits			= m_precision == PRECISION_HIGHP ? 32 :
										  m_precision == PRECISION_MEDIUMP ? 16 : 8;
		const deUint32	compareMask		= numBits == 32 ? ~0u : (1u<<numBits)-1u;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int		groupOffset		= groupNdx*workGroupSize;
			const deUint32	groupOutput		= *(const deUint32*)((const deUint8*)groupOutputs + groupNdx*groupStride);
			const deUint32	randomValue		= *(const deInt32*)((const deUint8*)inputs + inputStride*groupOffset);
			const deUint32	expected0		= randomValue ^ 0u;
			const deUint32	expected1		= randomValue ^ ~0u;
			int				numXorZeros		= (m_initialValue == 0) ? 1 : 0;

			for (int localNdx = 1; localNdx < workGroupSize; localNdx++)
			{
				const deUint32 inputValue = *(const deUint32*)((const deUint8*)inputs + inputStride*(groupOffset+localNdx));
				if (inputValue == 0)
					numXorZeros += 1;
			}

			const deUint32 expected = (numXorZeros%2 == 0) ? expected0 : expected1;

			if ((groupOutput & compareMask) != (expected & compareMask))
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ": expected " << tcu::toHex(expected0)
													   << " or " << tcu::toHex(expected1) << " (compare mask " << tcu::toHex(compareMask)
													   << "), got " << tcu::toHex(groupOutput) << TestLog::EndMessage;
				return false;
			}

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const deUint32 outputValue = *(const deUint32*)((const deUint8*)outputs + outputStride*(groupOffset+localNdx));

				if ((outputValue & compareMask) != 0 &&
					(outputValue & compareMask) != compareMask &&
					(outputValue & compareMask) != (expected0&compareMask) &&
					(outputValue & compareMask) != (expected1&compareMask))
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": found unexpected value " << tcu::toHex(outputValue)
									   << TestLog::EndMessage;
					return false;
				}
			}
		}

		return true;
	}
};

class ShaderAtomicExchangeCase : public ShaderAtomicOpCase
{
public:
	ShaderAtomicExchangeCase (Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision)
		: ShaderAtomicOpCase(context, name, "atomicExchange", operandType, type, precision, UVec3(3,2,1))
	{
		m_initialValue = 0;
	}

protected:
	void getInputs (int numValues, int stride, void* inputs) const
	{
		const int	workGroupSize	= (int)product(m_workGroupSize);
		const int	numWorkGroups	= numValues/workGroupSize;

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int groupOffset = groupNdx*workGroupSize;

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
				*(int*)((deUint8*)inputs + stride*(groupOffset+localNdx)) = localNdx+1;
		}
	}

	bool verify (int numValues, int inputStride, const void* inputs, int outputStride, const void* outputs, int groupStride, const void* groupOutputs) const
	{
		const int	workGroupSize	= (int)product(m_workGroupSize);
		const int	numWorkGroups	= numValues/workGroupSize;

		DE_UNREF(inputStride && inputs);

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int	groupOffset		= groupNdx*workGroupSize;
			const int	groupOutput		= *(const deInt32*)((const deUint8*)groupOutputs + groupNdx*groupStride);
			set<int>	usedValues;

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const int outputValue = *(const deInt32*)((const deUint8*)outputs + outputStride*(groupOffset+localNdx));

				if (!de::inRange(outputValue, 0, workGroupSize) || usedValues.find(outputValue) != usedValues.end())
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": found unexpected value " << outputValue
									   << TestLog::EndMessage;
					return false;
				}
				usedValues.insert(outputValue);
			}

			if (!de::inRange(groupOutput, 0, workGroupSize) || usedValues.find(groupOutput) != usedValues.end())
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ": unexpected final value" << groupOutput << TestLog::EndMessage;
				return false;
			}
		}

		return true;
	}
};

class ShaderAtomicCompSwapCase : public TestCase
{
public:
									ShaderAtomicCompSwapCase	(Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision);
									~ShaderAtomicCompSwapCase	(void);

	void							init						(void);
	void							deinit						(void);
	IterateResult					iterate						(void);

protected:

private:
									ShaderAtomicCompSwapCase	(const ShaderAtomicCompSwapCase& other);
	ShaderAtomicCompSwapCase&		operator=					(const ShaderAtomicCompSwapCase& other);

	const AtomicOperandType			m_operandType;
	const DataType					m_type;
	const Precision					m_precision;

	const UVec3						m_workGroupSize;
	const UVec3						m_numWorkGroups;

	ShaderProgram*					m_program;
};

ShaderAtomicCompSwapCase::ShaderAtomicCompSwapCase (Context& context, const char* name, AtomicOperandType operandType, DataType type, Precision precision)
	: TestCase			(context, name, "atomicCompSwap() Test")
	, m_operandType		(operandType)
	, m_type			(type)
	, m_precision		(precision)
	, m_workGroupSize	(3,2,1)
	, m_numWorkGroups	(4,4,4)
	, m_program			(DE_NULL)
{
}

ShaderAtomicCompSwapCase::~ShaderAtomicCompSwapCase (void)
{
	ShaderAtomicCompSwapCase::deinit();
}

void ShaderAtomicCompSwapCase::init (void)
{
	const bool			isSSBO		= m_operandType == ATOMIC_OPERAND_BUFFER_VARIABLE;
	const char*			precName	= getPrecisionName(m_precision);
	const char*			typeName	= getDataTypeName(m_type);
	const deUint32		numValues	= product(m_workGroupSize)*product(m_numWorkGroups);
	std::ostringstream	src;

	src << "#version 310 es\n"
		<< "layout(local_size_x = " << m_workGroupSize.x()
		<< ", local_size_y = " << m_workGroupSize.y()
		<< ", local_size_z = " << m_workGroupSize.z() << ") in;\n"
		<< "layout(binding = 0) buffer InOut\n"
		<< "{\n"
		<< "	" << precName << " " << typeName << " compareValues[" << numValues << "];\n"
		<< "	" << precName << " " << typeName << " exchangeValues[" << numValues << "];\n"
		<< "	" << precName << " " << typeName << " outputValues[" << numValues << "];\n"
		<< "	" << (isSSBO ? "coherent " : "") << precName << " " << typeName << " groupValues[" << product(m_numWorkGroups) << "];\n"
		<< "} sb_inout;\n";

	if (!isSSBO)
		src << "shared " << precName << " " << typeName << " s_var;\n";

	src << "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "	uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		<< "	uint globalOffs = localSize*globalNdx;\n"
		<< "	uint offset     = globalOffs + gl_LocalInvocationIndex;\n"
		<< "\n";

	if (!isSSBO)
	{
		src << "	if (gl_LocalInvocationIndex == 0u)\n"
			<< "		s_var = " << typeName << "(" << 0 << ");\n"
			<< "\n";
	}

	src << "	" << precName << " " << typeName << " compare = sb_inout.compareValues[offset];\n"
		<< "	" << precName << " " << typeName << " exchange = sb_inout.exchangeValues[offset];\n"
		<< "	" << precName << " " << typeName << " result;\n"
		<< "	bool swapDone = false;\n"
		<< "\n"
		<< "	for (uint ndx = 0u; ndx < localSize; ndx++)\n"
		<< "	{\n"
		<< "		barrier();\n"
		<< "		if (!swapDone)\n"
		<< "		{\n"
		<< "			result = atomicCompSwap(" << (isSSBO ? "sb_inout.groupValues[globalNdx]" : "s_var") << ", compare, exchange);\n"
		<< "			if (result == compare)\n"
		<< "				swapDone = true;\n"
		<< "		}\n"
		<< "	}\n"
		<< "\n"
		<< "	sb_inout.outputValues[offset] = result;\n";

	if (!isSSBO)
	{
		src << "	barrier();\n"
			<< "	if (gl_LocalInvocationIndex == 0u)\n"
			<< "		sb_inout.groupValues[globalNdx] = s_var;\n";
	}

	src << "}\n";

	DE_ASSERT(!m_program);
	m_program = new ShaderProgram(m_context.getRenderContext(), ProgramSources() << ComputeSource(src.str()));

	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
	{
		delete m_program;
		m_program = DE_NULL;
		throw tcu::TestError("Compile failed");
	}
}

void ShaderAtomicCompSwapCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

ShaderAtomicOpCase::IterateResult ShaderAtomicCompSwapCase::iterate (void)
{
	const glw::Functions&		gl				= m_context.getRenderContext().getFunctions();
	const deUint32				program			= m_program->getProgram();
	const Buffer				inoutBuffer		(m_context.getRenderContext());
	const deUint32				blockNdx		= gl.getProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "InOut");
	const InterfaceBlockInfo	blockInfo		= getProgramInterfaceBlockInfo(gl, program, GL_SHADER_STORAGE_BLOCK, blockNdx);
	const deUint32				cmpVarNdx		= gl.getProgramResourceIndex(program, GL_BUFFER_VARIABLE, "InOut.compareValues[0]");
	const InterfaceVariableInfo	cmpVarInfo		= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, cmpVarNdx);
	const deUint32				exhVarNdx		= gl.getProgramResourceIndex(program, GL_BUFFER_VARIABLE, "InOut.exchangeValues[0]");
	const InterfaceVariableInfo	exhVarInfo		= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, exhVarNdx);
	const deUint32				outVarNdx		= gl.getProgramResourceIndex(program, GL_BUFFER_VARIABLE, "InOut.outputValues[0]");
	const InterfaceVariableInfo	outVarInfo		= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, outVarNdx);
	const deUint32				groupVarNdx		= gl.getProgramResourceIndex(program, GL_BUFFER_VARIABLE, "InOut.groupValues[0]");
	const InterfaceVariableInfo	groupVarInfo	= getProgramInterfaceVariableInfo(gl, program, GL_BUFFER_VARIABLE, groupVarNdx);
	const deUint32				numValues		= product(m_workGroupSize)*product(m_numWorkGroups);

	TCU_CHECK(cmpVarInfo.arraySize == numValues &&
			  exhVarInfo.arraySize == numValues &&
			  outVarInfo.arraySize == numValues &&
			  groupVarInfo.arraySize == product(m_numWorkGroups));

	gl.useProgram(program);

	// \todo [2013-09-05 pyry] Use randomized input values!

	// Setup buffer.
	{
		const deUint32	workGroupSize	= product(m_workGroupSize);
		vector<deUint8>	bufData			(blockInfo.dataSize);

		std::fill(bufData.begin(), bufData.end(), 0);

		for (deUint32 ndx = 0; ndx < numValues; ndx++)
			*(deUint32*)(&bufData[0] + cmpVarInfo.offset + cmpVarInfo.arrayStride*ndx) = ndx%workGroupSize;

		for (deUint32 ndx = 0; ndx < numValues; ndx++)
			*(deUint32*)(&bufData[0] + exhVarInfo.offset + exhVarInfo.arrayStride*ndx) = (ndx%workGroupSize)+1;

		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, *inoutBuffer);
		gl.bufferData(GL_SHADER_STORAGE_BUFFER, blockInfo.dataSize, &bufData[0], GL_STATIC_READ);
		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *inoutBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Output buffer setup failed");
	}

	gl.dispatchCompute(m_numWorkGroups.x(), m_numWorkGroups.y(), m_numWorkGroups.z());

	// Read back and compare
	{
		const void*		resPtr			= gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, blockInfo.dataSize, GL_MAP_READ_BIT);
		const int		numWorkGroups	= (int)product(m_numWorkGroups);
		const int		workGroupSize	= (int)product(m_workGroupSize);
		bool			isOk			= true;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange()");
		TCU_CHECK(resPtr);

		for (int groupNdx = 0; groupNdx < numWorkGroups; groupNdx++)
		{
			const int	groupOffset		= groupNdx*workGroupSize;
			const int	groupOutput		= *(const deInt32*)((const deUint8*)resPtr + groupVarInfo.offset + groupNdx*groupVarInfo.arrayStride);

			for (int localNdx = 0; localNdx < workGroupSize; localNdx++)
			{
				const int	refValue		= localNdx;
				const int	outputValue		= *(const deInt32*)((const deUint8*)resPtr + outVarInfo.offset + outVarInfo.arrayStride*(groupOffset+localNdx));

				if (outputValue != refValue)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ", invocation " << localNdx
														   << ": expected " << refValue << ", got " << outputValue
									   << TestLog::EndMessage;
					isOk = false;
					break;
				}
			}

			if (groupOutput != workGroupSize)
			{
				m_testCtx.getLog() << TestLog::Message << "ERROR: at group " << groupNdx << ": expected" << workGroupSize << ", got " << groupOutput << TestLog::EndMessage;
				isOk = false;
				break;
			}
		}

		gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer()");

		m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
								isOk ? "Pass"				: "Comparison failed");
	}

	return STOP;
}

ShaderAtomicOpTests::ShaderAtomicOpTests (Context& context, const char* name, AtomicOperandType operandType)
	: TestCaseGroup	(context, name, "Atomic Operation Tests")
	, m_operandType	(operandType)
{
}

ShaderAtomicOpTests::~ShaderAtomicOpTests (void)
{
}

template<typename T>
static tcu::TestCaseGroup* createAtomicOpGroup (Context& context, AtomicOperandType operandType, const char* groupName)
{
	tcu::TestCaseGroup *const group = new tcu::TestCaseGroup(context.getTestContext(), groupName, (string("Atomic ") + groupName).c_str());
	try
	{
		for (int precNdx = 0; precNdx < PRECISION_LAST; precNdx++)
		{
			for (int typeNdx = 0; typeNdx < 2; typeNdx++)
			{
				const Precision		precision		= Precision(precNdx);
				const DataType		type			= typeNdx > 0 ? TYPE_INT : TYPE_UINT;
				const string		caseName		= string(getPrecisionName(precision)) + "_" + getDataTypeName(type);

				group->addChild(new T(context, caseName.c_str(), operandType, type, precision));
			}
		}

		return group;
	}
	catch (...)
	{
		delete group;
		throw;
	}
}

void ShaderAtomicOpTests::init (void)
{
	addChild(createAtomicOpGroup<ShaderAtomicAddCase>		(m_context, m_operandType, "add"));
	addChild(createAtomicOpGroup<ShaderAtomicMinCase>		(m_context, m_operandType, "min"));
	addChild(createAtomicOpGroup<ShaderAtomicMaxCase>		(m_context, m_operandType, "max"));
	addChild(createAtomicOpGroup<ShaderAtomicAndCase>		(m_context, m_operandType, "and"));
	addChild(createAtomicOpGroup<ShaderAtomicOrCase>		(m_context, m_operandType, "or"));
	addChild(createAtomicOpGroup<ShaderAtomicXorCase>		(m_context, m_operandType, "xor"));
	addChild(createAtomicOpGroup<ShaderAtomicExchangeCase>	(m_context, m_operandType, "exchange"));
	addChild(createAtomicOpGroup<ShaderAtomicCompSwapCase>	(m_context, m_operandType, "compswap"));
}

} // Functional
} // gles31
} // deqp
