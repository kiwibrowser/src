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
 * \brief Integer built-in function tests.
 *//*--------------------------------------------------------------------*/

#include "es31fShaderIntegerFunctionTests.hpp"
#include "glsShaderExecUtil.hpp"
#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuFloat.hpp"
#include "deRandom.hpp"
#include "deMath.h"
#include "deString.h"
#include "deInt32.h"

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::vector;
using std::string;
using tcu::TestLog;
using namespace gls::ShaderExecUtil;

using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;
using tcu::UVec2;
using tcu::UVec3;
using tcu::UVec4;

// Utilities

namespace
{

struct HexFloat
{
	const float value;
	HexFloat (const float value_) : value(value_) {}
};

std::ostream& operator<< (std::ostream& str, const HexFloat& v)
{
	return str << v.value << " / " << tcu::toHex(tcu::Float32(v.value).bits());
}

struct VarValue
{
	const glu::VarType&	type;
	const void*			value;

	VarValue (const glu::VarType& type_, const void* value_) : type(type_), value(value_) {}
};

std::ostream& operator<< (std::ostream& str, const VarValue& varValue)
{
	DE_ASSERT(varValue.type.isBasicType());

	const glu::DataType		basicType		= varValue.type.getBasicType();
	const glu::DataType		scalarType		= glu::getDataTypeScalarType(basicType);
	const int				numComponents	= glu::getDataTypeScalarSize(basicType);

	if (numComponents > 1)
		str << glu::getDataTypeName(basicType) << "(";

	for (int compNdx = 0; compNdx < numComponents; compNdx++)
	{
		if (compNdx != 0)
			str << ", ";

		switch (scalarType)
		{
			case glu::TYPE_FLOAT:	str << HexFloat(((const float*)varValue.value)[compNdx]);						break;
			case glu::TYPE_INT:		str << ((const deInt32*)varValue.value)[compNdx];								break;
			case glu::TYPE_UINT:	str << tcu::toHex(((const deUint32*)varValue.value)[compNdx]);					break;
			case glu::TYPE_BOOL:	str << (((const deUint32*)varValue.value)[compNdx] != 0 ? "true" : "false");	break;

			default:
				DE_ASSERT(false);
		}
	}

	if (numComponents > 1)
		str << ")";

	return str;
}

inline int getShaderUintBitCount (glu::ShaderType shaderType, glu::Precision precision)
{
	// \todo [2013-10-31 pyry] Query from GL for vertex and fragment shaders.
	DE_UNREF(shaderType);
	const int bitCounts[] = { 9, 16, 32 };
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(bitCounts) == glu::PRECISION_LAST);
	return bitCounts[precision];
}

static inline deUint32 extendSignTo32 (deUint32 integer, deUint32 integerLength)
{
	DE_ASSERT(integerLength > 0 && integerLength <= 32);

	return deUint32(0 - deInt32((integer & (1 << (integerLength - 1))) << 1)) | integer;
}

static inline deUint32 getLowBitMask (int integerLength)
{
	DE_ASSERT(integerLength >= 0 && integerLength <= 32);

	// \note: shifting more or equal to 32 => undefined behavior. Avoid it by shifting in two parts (1 << (num-1) << 1)
	if (integerLength == 0u)
		return 0u;
	return ((1u << ((deUint32)integerLength - 1u)) << 1u) - 1u;
}

static void generateRandomInputData (de::Random& rnd, glu::ShaderType shaderType, glu::DataType dataType, glu::Precision precision, deUint32* dst, int numValues)
{
	const int				scalarSize		= glu::getDataTypeScalarSize(dataType);
	const deUint32			integerLength	= (deUint32)getShaderUintBitCount(shaderType, precision);
	const deUint32			integerMask		= getLowBitMask(integerLength);
	const bool				isUnsigned		= glu::isDataTypeUintOrUVec(dataType);

	if (isUnsigned)
	{
		for (int valueNdx = 0; valueNdx < numValues; ++valueNdx)
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
				dst[valueNdx*scalarSize + compNdx] = rnd.getUint32() & integerMask;
	}
	else
	{
		for (int valueNdx = 0; valueNdx < numValues; ++valueNdx)
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
				dst[valueNdx*scalarSize + compNdx] = extendSignTo32(rnd.getUint32() & integerMask, integerLength);
	}
}

} // anonymous

// IntegerFunctionCase

class IntegerFunctionCase : public TestCase
{
public:
							IntegerFunctionCase		(Context& context, const char* name, const char* description, glu::ShaderType shaderType);
							~IntegerFunctionCase	(void);

	void					init					(void);
	void					deinit					(void);
	IterateResult			iterate					(void);

protected:
							IntegerFunctionCase		(const IntegerFunctionCase& other);
	IntegerFunctionCase&	operator=				(const IntegerFunctionCase& other);

	virtual void			getInputValues			(int numValues, void* const* values) const = 0;
	virtual bool			compare					(const void* const* inputs, const void* const* outputs) = 0;

	glu::ShaderType			m_shaderType;
	ShaderSpec				m_spec;
	int						m_numValues;

	std::ostringstream		m_failMsg;				//!< Comparison failure help message.

private:
	ShaderExecutor*			m_executor;
};

IntegerFunctionCase::IntegerFunctionCase (Context& context, const char* name, const char* description, glu::ShaderType shaderType)
	: TestCase		(context, name, description)
	, m_shaderType	(shaderType)
	, m_numValues	(100)
	, m_executor	(DE_NULL)
{
	m_spec.version = glu::getContextTypeGLSLVersion(context.getRenderContext().getType());
}

IntegerFunctionCase::~IntegerFunctionCase (void)
{
	IntegerFunctionCase::deinit();
}

void IntegerFunctionCase::init (void)
{
	DE_ASSERT(!m_executor);

	m_executor = createExecutor(m_context.getRenderContext(), m_shaderType, m_spec);
	m_testCtx.getLog() << m_executor;

	if (!m_executor->isOk())
		throw tcu::TestError("Compile failed");
}

void IntegerFunctionCase::deinit (void)
{
	delete m_executor;
	m_executor = DE_NULL;
}

static vector<int> getScalarSizes (const vector<Symbol>& symbols)
{
	vector<int> sizes(symbols.size());
	for (int ndx = 0; ndx < (int)symbols.size(); ++ndx)
		sizes[ndx] = symbols[ndx].varType.getScalarSize();
	return sizes;
}

static int computeTotalScalarSize (const vector<Symbol>& symbols)
{
	int totalSize = 0;
	for (vector<Symbol>::const_iterator sym = symbols.begin(); sym != symbols.end(); ++sym)
		totalSize += sym->varType.getScalarSize();
	return totalSize;
}

static vector<void*> getInputOutputPointers (const vector<Symbol>& symbols, vector<deUint32>& data, const int numValues)
{
	vector<void*>	pointers		(symbols.size());
	int				curScalarOffset	= 0;

	for (int varNdx = 0; varNdx < (int)symbols.size(); ++varNdx)
	{
		const Symbol&	var				= symbols[varNdx];
		const int		scalarSize		= var.varType.getScalarSize();

		// Uses planar layout as input/output specs do not support strides.
		pointers[varNdx] = &data[curScalarOffset];
		curScalarOffset += scalarSize*numValues;
	}

	DE_ASSERT(curScalarOffset == (int)data.size());

	return pointers;
}

IntegerFunctionCase::IterateResult IntegerFunctionCase::iterate (void)
{
	const int				numInputScalars			= computeTotalScalarSize(m_spec.inputs);
	const int				numOutputScalars		= computeTotalScalarSize(m_spec.outputs);
	vector<deUint32>		inputData				(numInputScalars * m_numValues);
	vector<deUint32>		outputData				(numOutputScalars * m_numValues);
	const vector<void*>		inputPointers			= getInputOutputPointers(m_spec.inputs, inputData, m_numValues);
	const vector<void*>		outputPointers			= getInputOutputPointers(m_spec.outputs, outputData, m_numValues);

	// Initialize input data.
	getInputValues(m_numValues, &inputPointers[0]);

	// Execute shader.
	m_executor->useProgram();
	m_executor->execute(m_numValues, &inputPointers[0], &outputPointers[0]);

	// Compare results.
	{
		const vector<int>		inScalarSizes		= getScalarSizes(m_spec.inputs);
		const vector<int>		outScalarSizes		= getScalarSizes(m_spec.outputs);
		vector<void*>			curInputPtr			(inputPointers.size());
		vector<void*>			curOutputPtr		(outputPointers.size());
		int						numFailed			= 0;

		for (int valNdx = 0; valNdx < m_numValues; valNdx++)
		{
			// Set up pointers for comparison.
			for (int inNdx = 0; inNdx < (int)curInputPtr.size(); ++inNdx)
				curInputPtr[inNdx] = (deUint32*)inputPointers[inNdx] + inScalarSizes[inNdx]*valNdx;

			for (int outNdx = 0; outNdx < (int)curOutputPtr.size(); ++outNdx)
				curOutputPtr[outNdx] = (deUint32*)outputPointers[outNdx] + outScalarSizes[outNdx]*valNdx;

			if (!compare(&curInputPtr[0], &curOutputPtr[0]))
			{
				// \todo [2013-08-08 pyry] We probably want to log reference value as well?

				m_testCtx.getLog() << TestLog::Message << "ERROR: comparison failed for value " << valNdx << ":\n  " << m_failMsg.str() << TestLog::EndMessage;

				m_testCtx.getLog() << TestLog::Message << "  inputs:" << TestLog::EndMessage;
				for (int inNdx = 0; inNdx < (int)curInputPtr.size(); inNdx++)
					m_testCtx.getLog() << TestLog::Message << "    " << m_spec.inputs[inNdx].name << " = "
														   << VarValue(m_spec.inputs[inNdx].varType, curInputPtr[inNdx])
									   << TestLog::EndMessage;

				m_testCtx.getLog() << TestLog::Message << "  outputs:" << TestLog::EndMessage;
				for (int outNdx = 0; outNdx < (int)curOutputPtr.size(); outNdx++)
					m_testCtx.getLog() << TestLog::Message << "    " << m_spec.outputs[outNdx].name << " = "
														   << VarValue(m_spec.outputs[outNdx].varType, curOutputPtr[outNdx])
									   << TestLog::EndMessage;

				m_failMsg.str("");
				m_failMsg.clear();
				numFailed += 1;
			}
		}

		m_testCtx.getLog() << TestLog::Message << (m_numValues - numFailed) << " / " << m_numValues << " values passed" << TestLog::EndMessage;

		m_testCtx.setTestResult(numFailed == 0 ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
								numFailed == 0 ? "Pass"					: "Result comparison failed");
	}

	return STOP;
}

static const char* getPrecisionPostfix (glu::Precision precision)
{
	static const char* s_postfix[] =
	{
		"_lowp",
		"_mediump",
		"_highp"
	};
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(s_postfix) == glu::PRECISION_LAST);
	DE_ASSERT(de::inBounds<int>(precision, 0, DE_LENGTH_OF_ARRAY(s_postfix)));
	return s_postfix[precision];
}

static const char* getShaderTypePostfix (glu::ShaderType shaderType)
{
	static const char* s_postfix[] =
	{
		"_vertex",
		"_fragment",
		"_geometry",
		"_tess_control",
		"_tess_eval",
		"_compute"
	};
	DE_ASSERT(de::inBounds<int>(shaderType, 0, DE_LENGTH_OF_ARRAY(s_postfix)));
	return s_postfix[shaderType];
}

static std::string getIntegerFuncCaseName (glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
{
	return string(glu::getDataTypeName(baseType)) + getPrecisionPostfix(precision) + getShaderTypePostfix(shaderType);
}

class UaddCarryCase : public IntegerFunctionCase
{
public:
	UaddCarryCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "uaddCarry", shaderType)
	{
		m_spec.inputs.push_back(Symbol("x", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("y", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("sum", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("carry", glu::VarType(baseType, glu::PRECISION_LOWP)));
		m_spec.source = "sum = uaddCarry(x, y, carry);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd				(deStringHash(getName()) ^ 0x235facu);
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);
		const deUint32			integerMask		= getLowBitMask(integerLength);
		const bool				isSigned		= glu::isDataTypeIntOrIVec(type);
		deUint32*				in0				= (deUint32*)values[0];
		deUint32*				in1				= (deUint32*)values[1];

		const struct
		{
			deUint32	x;
			deUint32	y;
		} easyCases[] =
		{
			{ 0x00000000u,	0x00000000u },
			{ 0xfffffffeu,	0x00000001u },
			{ 0x00000001u,	0xfffffffeu },
			{ 0xffffffffu,	0x00000001u },
			{ 0x00000001u,	0xffffffffu },
			{ 0xfffffffeu,	0x00000002u },
			{ 0x00000002u,	0xfffffffeu },
			{ 0xffffffffu,	0xffffffffu }
		};

		// generate integers with proper bit count
		for (int easyCaseNdx = 0; easyCaseNdx < DE_LENGTH_OF_ARRAY(easyCases); easyCaseNdx++)
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				in0[easyCaseNdx*scalarSize + compNdx] = easyCases[easyCaseNdx].x & integerMask;
				in1[easyCaseNdx*scalarSize + compNdx] = easyCases[easyCaseNdx].y & integerMask;
			}
		}

		// convert to signed
		if (isSigned)
		{
			for (int easyCaseNdx = 0; easyCaseNdx < DE_LENGTH_OF_ARRAY(easyCases); easyCaseNdx++)
			{
				for (int compNdx = 0; compNdx < scalarSize; compNdx++)
				{
					in0[easyCaseNdx*scalarSize + compNdx] = extendSignTo32(in0[easyCaseNdx*scalarSize + compNdx], integerLength);
					in1[easyCaseNdx*scalarSize + compNdx] = extendSignTo32(in1[easyCaseNdx*scalarSize + compNdx], integerLength);
				}
			}
		}

		generateRandomInputData(rnd, m_shaderType, type, precision, in0, numValues - DE_LENGTH_OF_ARRAY(easyCases));
		generateRandomInputData(rnd, m_shaderType, type, precision, in1, numValues - DE_LENGTH_OF_ARRAY(easyCases));
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);
		const deUint32			mask0			= getLowBitMask(integerLength);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	in0		= ((const deUint32*)inputs[0])[compNdx];
			const deUint32	in1		= ((const deUint32*)inputs[1])[compNdx];
			const deUint32	out0	= ((const deUint32*)outputs[0])[compNdx];
			const deUint32	out1	= ((const deUint32*)outputs[1])[compNdx];
			const deUint32	ref0	= in0+in1;
			const deUint32	ref1	= (deUint64(in0)+deUint64(in1)) > 0xffffffffu ? 1u : 0u;

			if (((out0&mask0) != (ref0&mask0)) || out1 != ref1)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(ref0) << ", " << tcu::toHex(ref1);
				return false;
			}
		}

		return true;
	}
};

class UsubBorrowCase : public IntegerFunctionCase
{
public:
	UsubBorrowCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "usubBorrow", shaderType)
	{
		m_spec.inputs.push_back(Symbol("x", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("y", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("diff", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("carry", glu::VarType(baseType, glu::PRECISION_LOWP)));
		m_spec.source = "diff = usubBorrow(x, y, carry);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd				(deStringHash(getName()) ^ 0x235facu);
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);
		const deUint32			integerMask		= getLowBitMask(integerLength);
		const bool				isSigned		= glu::isDataTypeIntOrIVec(type);
		deUint32*				in0				= (deUint32*)values[0];
		deUint32*				in1				= (deUint32*)values[1];

		const struct
		{
			deUint32	x;
			deUint32	y;
		} easyCases[] =
		{
			{ 0x00000000u,	0x00000000u },
			{ 0x00000001u,	0x00000001u },
			{ 0x00000001u,	0x00000002u },
			{ 0x00000001u,	0xffffffffu },
			{ 0xfffffffeu,	0xffffffffu },
			{ 0xffffffffu,	0xffffffffu },
		};

		// generate integers with proper bit count
		for (int easyCaseNdx = 0; easyCaseNdx < DE_LENGTH_OF_ARRAY(easyCases); easyCaseNdx++)
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				in0[easyCaseNdx*scalarSize + compNdx] = easyCases[easyCaseNdx].x & integerMask;
				in1[easyCaseNdx*scalarSize + compNdx] = easyCases[easyCaseNdx].y & integerMask;
			}
		}

		// convert to signed
		if (isSigned)
		{
			for (int easyCaseNdx = 0; easyCaseNdx < DE_LENGTH_OF_ARRAY(easyCases); easyCaseNdx++)
			{
				for (int compNdx = 0; compNdx < scalarSize; compNdx++)
				{
					in0[easyCaseNdx*scalarSize + compNdx] = extendSignTo32(in0[easyCaseNdx*scalarSize + compNdx], integerLength);
					in1[easyCaseNdx*scalarSize + compNdx] = extendSignTo32(in1[easyCaseNdx*scalarSize + compNdx], integerLength);
				}
			}
		}

		generateRandomInputData(rnd, m_shaderType, type, precision, in0, numValues - DE_LENGTH_OF_ARRAY(easyCases));
		generateRandomInputData(rnd, m_shaderType, type, precision, in1, numValues - DE_LENGTH_OF_ARRAY(easyCases));
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);
		const deUint32			mask0			= getLowBitMask(integerLength);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	in0		= ((const deUint32*)inputs[0])[compNdx];
			const deUint32	in1		= ((const deUint32*)inputs[1])[compNdx];
			const deUint32	out0	= ((const deUint32*)outputs[0])[compNdx];
			const deUint32	out1	= ((const deUint32*)outputs[1])[compNdx];
			const deUint32	ref0	= in0-in1;
			const deUint32	ref1	= in0 >= in1 ? 0u : 1u;

			if (((out0&mask0) != (ref0&mask0)) || out1 != ref1)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(ref0) << ", " << tcu::toHex(ref1);
				return false;
			}
		}

		return true;
	}
};

class UmulExtendedCase : public IntegerFunctionCase
{
public:
	UmulExtendedCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "umulExtended", shaderType)
	{
		m_spec.inputs.push_back(Symbol("x", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("y", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("msb", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("lsb", glu::VarType(baseType, precision)));
		m_spec.source = "umulExtended(x, y, msb, lsb);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(getName()) ^ 0x235facu);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
//		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);
		deUint32*				in0			= (deUint32*)values[0];
		deUint32*				in1			= (deUint32*)values[1];
		int						valueNdx	= 0;

		const struct
		{
			deUint32	x;
			deUint32	y;
		} easyCases[] =
		{
			{ 0x00000000u,	0x00000000u },
			{ 0xffffffffu,	0x00000001u },
			{ 0xffffffffu,	0x00000002u },
			{ 0x00000001u,	0xffffffffu },
			{ 0x00000002u,	0xffffffffu },
			{ 0xffffffffu,	0xffffffffu },
		};

		for (int easyCaseNdx = 0; easyCaseNdx < DE_LENGTH_OF_ARRAY(easyCases); easyCaseNdx++)
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				in0[valueNdx*scalarSize + compNdx] = easyCases[easyCaseNdx].x;
				in1[valueNdx*scalarSize + compNdx] = easyCases[easyCaseNdx].y;
			}

			valueNdx += 1;
		}

		while (valueNdx < numValues)
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const deUint32	base0	= rnd.getUint32();
				const deUint32	base1	= rnd.getUint32();
				const int		adj0	= rnd.getInt(0, 20);
				const int		adj1	= rnd.getInt(0, 20);
				in0[valueNdx*scalarSize + compNdx] = base0 >> adj0;
				in1[valueNdx*scalarSize + compNdx] = base1 >> adj1;
			}

			valueNdx += 1;
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	in0		= ((const deUint32*)inputs[0])[compNdx];
			const deUint32	in1		= ((const deUint32*)inputs[1])[compNdx];
			const deUint32	out0	= ((const deUint32*)outputs[0])[compNdx];
			const deUint32	out1	= ((const deUint32*)outputs[1])[compNdx];
			const deUint64	mul64	= deUint64(in0)*deUint64(in1);
			const deUint32	ref0	= deUint32(mul64 >> 32);
			const deUint32	ref1	= deUint32(mul64 & 0xffffffffu);

			if (out0 != ref0 || out1 != ref1)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(ref0) << ", " << tcu::toHex(ref1);
				return false;
			}
		}

		return true;
	}
};

class ImulExtendedCase : public IntegerFunctionCase
{
public:
	ImulExtendedCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "imulExtended", shaderType)
	{
		m_spec.inputs.push_back(Symbol("x", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("y", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("msb", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("lsb", glu::VarType(baseType, precision)));
		m_spec.source = "imulExtended(x, y, msb, lsb);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(getName()) ^ 0x224fa1u);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
//		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);
		deUint32*				in0			= (deUint32*)values[0];
		deUint32*				in1			= (deUint32*)values[1];
		int						valueNdx	= 0;

		const struct
		{
			deUint32	x;
			deUint32	y;
		} easyCases[] =
		{
			{ 0x00000000u,	0x00000000u },
			{ 0xffffffffu,	0x00000002u },
			{ 0x7fffffffu,	0x00000001u },
			{ 0x7fffffffu,	0x00000002u },
			{ 0x7fffffffu,	0x7fffffffu },
			{ 0xffffffffu,	0xffffffffu },
			{ 0x7fffffffu,	0xfffffffeu },
		};

		for (int easyCaseNdx = 0; easyCaseNdx < DE_LENGTH_OF_ARRAY(easyCases); easyCaseNdx++)
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				in0[valueNdx*scalarSize + compNdx] = (deInt32)easyCases[easyCaseNdx].x;
				in1[valueNdx*scalarSize + compNdx] = (deInt32)easyCases[easyCaseNdx].y;
			}

			valueNdx += 1;
		}

		while (valueNdx < numValues)
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const deInt32	base0	= (deInt32)rnd.getUint32();
				const deInt32	base1	= (deInt32)rnd.getUint32();
				const int		adj0	= rnd.getInt(0, 20);
				const int		adj1	= rnd.getInt(0, 20);
				in0[valueNdx*scalarSize + compNdx] = base0 >> adj0;
				in1[valueNdx*scalarSize + compNdx] = base1 >> adj1;
			}

			valueNdx += 1;
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deInt32	in0		= ((const deInt32*)inputs[0])[compNdx];
			const deInt32	in1		= ((const deInt32*)inputs[1])[compNdx];
			const deInt32	out0	= ((const deInt32*)outputs[0])[compNdx];
			const deInt32	out1	= ((const deInt32*)outputs[1])[compNdx];
			const deInt64	mul64	= deInt64(in0)*deInt64(in1);
			const deInt32	ref0	= deInt32(mul64 >> 32);
			const deInt32	ref1	= deInt32(mul64 & 0xffffffffu);

			if (out0 != ref0 || out1 != ref1)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(ref0) << ", " << tcu::toHex(ref1);
				return false;
			}
		}

		return true;
	}
};

class BitfieldExtractCase : public IntegerFunctionCase
{
public:
	BitfieldExtractCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "bitfieldExtract", shaderType)
	{
		m_spec.inputs.push_back(Symbol("value", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("offset", glu::VarType(glu::TYPE_INT, precision)));
		m_spec.inputs.push_back(Symbol("bits", glu::VarType(glu::TYPE_INT, precision)));
		m_spec.outputs.push_back(Symbol("extracted", glu::VarType(baseType, precision)));
		m_spec.source = "extracted = bitfieldExtract(value, offset, bits);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(getName()) ^ 0xa113fca2u);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const bool				ignoreSign	= precision != glu::PRECISION_HIGHP && glu::isDataTypeIntOrIVec(type);
		const int				numBits		= getShaderUintBitCount(m_shaderType, precision) - (ignoreSign ? 1 : 0);
		deUint32*				inValue		= (deUint32*)values[0];
		int*					inOffset	= (int*)values[1];
		int*					inBits		= (int*)values[2];

		for (int valueNdx = 0; valueNdx < numValues; ++valueNdx)
		{
			const int		bits	= rnd.getInt(0, numBits);
			const int		offset	= rnd.getInt(0, numBits-bits);

			inOffset[valueNdx]	= offset;
			inBits[valueNdx]	= bits;
		}

		generateRandomInputData(rnd, m_shaderType, type, precision, inValue, numValues);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const bool				isSigned		= glu::isDataTypeIntOrIVec(type);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				offset			= *((const int*)inputs[1]);
		const int				bits			= *((const int*)inputs[2]);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	value	= ((const deUint32*)inputs[0])[compNdx];
			const deUint32	out		= ((const deUint32*)outputs[0])[compNdx];
			const deUint32	valMask	= (bits == 32 ? ~0u : ((1u<<bits)-1u));
			const deUint32	baseVal	= (offset == 32) ? (0) : ((value >> offset) & valMask);
			const deUint32	ref		= baseVal | ((isSigned && (baseVal & (1<<(bits-1)))) ? ~valMask : 0u);

			if (out != ref)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(ref);
				return false;
			}
		}

		return true;
	}
};

class BitfieldInsertCase : public IntegerFunctionCase
{
public:
	BitfieldInsertCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "bitfieldInsert", shaderType)
	{
		m_spec.inputs.push_back(Symbol("base", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("insert", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("offset", glu::VarType(glu::TYPE_INT, precision)));
		m_spec.inputs.push_back(Symbol("bits", glu::VarType(glu::TYPE_INT, precision)));
		m_spec.outputs.push_back(Symbol("result", glu::VarType(baseType, precision)));
		m_spec.source = "result = bitfieldInsert(base, insert, offset, bits);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(getName()) ^ 0x12c2acff);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				numBits		= getShaderUintBitCount(m_shaderType, precision);
		deUint32*				inBase		= (deUint32*)values[0];
		deUint32*				inInsert	= (deUint32*)values[1];
		int*					inOffset	= (int*)values[2];
		int*					inBits		= (int*)values[3];

		for (int valueNdx = 0; valueNdx < numValues; ++valueNdx)
		{
			const int bits		= rnd.getInt(0, numBits);
			const int offset	= rnd.getInt(0, numBits-bits);

			inOffset[valueNdx]	= offset;
			inBits[valueNdx]	= bits;
		}

		generateRandomInputData(rnd, m_shaderType, type, precision, inBase, numValues);
		generateRandomInputData(rnd, m_shaderType, type, precision, inInsert, numValues);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);
		const deUint32			cmpMask			= getLowBitMask(integerLength);
		const int				offset			= *((const int*)inputs[2]);
		const int				bits			= *((const int*)inputs[3]);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	base	= ((const deUint32*)inputs[0])[compNdx];
			const deUint32	insert	= ((const deUint32*)inputs[1])[compNdx];
			const deInt32	out		= ((const deUint32*)outputs[0])[compNdx];

			const deUint32	mask	= bits == 32 ? ~0u : (1u<<bits)-1;
			const deUint32	ref		= (base & ~(mask<<offset)) | ((insert & mask)<<offset);

			if ((out&cmpMask) != (ref&cmpMask))
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(ref);
				return false;
			}
		}

		return true;
	}
};

static inline deUint32 reverseBits (deUint32 v)
{
	v = (((v & 0xaaaaaaaa) >> 1) | ((v & 0x55555555) << 1));
	v = (((v & 0xcccccccc) >> 2) | ((v & 0x33333333) << 2));
	v = (((v & 0xf0f0f0f0) >> 4) | ((v & 0x0f0f0f0f) << 4));
	v = (((v & 0xff00ff00) >> 8) | ((v & 0x00ff00ff) << 8));
	return((v >> 16) | (v << 16));
}

class BitfieldReverseCase : public IntegerFunctionCase
{
public:
	BitfieldReverseCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "bitfieldReverse", shaderType)
	{
		m_spec.inputs.push_back(Symbol("value", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("result", glu::VarType(baseType, glu::PRECISION_HIGHP)));
		m_spec.source = "result = bitfieldReverse(value);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(getName()) ^ 0xff23a4);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		deUint32*				inValue		= (deUint32*)values[0];

		generateRandomInputData(rnd, m_shaderType, type, precision, inValue, numValues);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const deUint32			cmpMask			= reverseBits(getLowBitMask(integerLength));

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	value	= ((const deUint32*)inputs[0])[compNdx];
			const deInt32	out		= ((const deUint32*)outputs[0])[compNdx];
			const deUint32	ref		= reverseBits(value);

			if ((out&cmpMask) != (ref&cmpMask))
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(ref);
				return false;
			}
		}

		return true;
	}
};

class BitCountCase : public IntegerFunctionCase
{
public:
	BitCountCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "bitCount", shaderType)
	{
		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	intType		= vecSize == 1 ? glu::TYPE_INT : glu::getDataTypeIntVec(vecSize);

		m_spec.inputs.push_back(Symbol("value", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("count", glu::VarType(intType, glu::PRECISION_LOWP)));
		m_spec.source = "count = bitCount(value);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(getName()) ^ 0xab2cca4);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		deUint32*				inValue		= (deUint32*)values[0];

		generateRandomInputData(rnd, m_shaderType, type, precision, inValue, numValues);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const deUint32			countMask		= getLowBitMask(integerLength);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	value	= ((const deUint32*)inputs[0])[compNdx];
			const int		out		= ((const int*)outputs[0])[compNdx];
			const int		minRef	= dePop32(value&countMask);
			const int		maxRef	= dePop32(value);

			if (!de::inRange(out, minRef, maxRef))
			{
				m_failMsg << "Expected [" << compNdx << "] in range [" << minRef << ", " << maxRef << "]";
				return false;
			}
		}

		return true;
	}
};

static int findLSB (deUint32 value)
{
	for (int i = 0; i < 32; i++)
	{
		if (value & (1u<<i))
			return i;
	}
	return -1;
}

class FindLSBCase : public IntegerFunctionCase
{
public:
	FindLSBCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "findLSB", shaderType)
	{
		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	intType		= vecSize == 1 ? glu::TYPE_INT : glu::getDataTypeIntVec(vecSize);

		m_spec.inputs.push_back(Symbol("value", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("lsb", glu::VarType(intType, glu::PRECISION_LOWP)));
		m_spec.source = "lsb = findLSB(value);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(getName()) ^ 0x9923c2af);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		deUint32*				inValue		= (deUint32*)values[0];

		generateRandomInputData(rnd, m_shaderType, type, precision, inValue, numValues);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);
		const deUint32			mask			= getLowBitMask(integerLength);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	value	= ((const deUint32*)inputs[0])[compNdx];
			const int		out		= ((const int*)outputs[0])[compNdx];
			const int		minRef	= findLSB(value&mask);
			const int		maxRef	= findLSB(value);

			if (!de::inRange(out, minRef, maxRef))
			{
				m_failMsg << "Expected [" << compNdx << "] in range [" << minRef << ", " << maxRef << "]";
				return false;
			}
		}

		return true;
	}
};

static int findMSB (deInt32 value)
{
	if (value > 0)
		return 31 - deClz32((deUint32)value);
	else if (value < 0)
		return 31 - deClz32(~(deUint32)value);
	else
		return -1;
}

static int findMSB (deUint32 value)
{
	if (value > 0)
		return 31 - deClz32(value);
	else
		return -1;
}

static deUint32 toPrecision (deUint32 value, int numIntegerBits)
{
	return value & getLowBitMask(numIntegerBits);
}

static deInt32 toPrecision (deInt32 value, int numIntegerBits)
{
	return (deInt32)extendSignTo32((deUint32)value & getLowBitMask(numIntegerBits), numIntegerBits);
}

class FindMSBCase : public IntegerFunctionCase
{
public:
	FindMSBCase (Context& context, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: IntegerFunctionCase(context, getIntegerFuncCaseName(baseType, precision, shaderType).c_str(), "findMSB", shaderType)
	{
		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	intType		= vecSize == 1 ? glu::TYPE_INT : glu::getDataTypeIntVec(vecSize);

		m_spec.inputs.push_back(Symbol("value", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("msb", glu::VarType(intType, glu::PRECISION_LOWP)));
		m_spec.source = "msb = findMSB(value);";
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(getName()) ^ 0x742ac4e);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		deUint32*				inValue		= (deUint32*)values[0];

		generateRandomInputData(rnd, m_shaderType, type, precision, inValue, numValues);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const bool				isSigned		= glu::isDataTypeIntOrIVec(type);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				integerLength	= getShaderUintBitCount(m_shaderType, precision);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const deUint32	value	= ((const deUint32*)inputs[0])[compNdx];
			const int		out		= ((const deInt32*)outputs[0])[compNdx];
			const int		minRef	= isSigned ? findMSB(toPrecision(deInt32(value), integerLength))	: findMSB(toPrecision(value, integerLength));
			const int		maxRef	= isSigned ? findMSB(deInt32(value))								: findMSB(value);

			if (!de::inRange(out, minRef, maxRef))
			{
				m_failMsg << "Expected [" << compNdx << "] in range [" << minRef << ", " << maxRef << "]";
				return false;
			}
		}

		return true;
	}
};

ShaderIntegerFunctionTests::ShaderIntegerFunctionTests (Context& context)
	: TestCaseGroup(context, "integer", "Integer function tests")
{
}

ShaderIntegerFunctionTests::~ShaderIntegerFunctionTests (void)
{
}

template<class TestClass>
static void addFunctionCases (TestCaseGroup* parent, const char* functionName, bool intTypes, bool uintTypes, bool allPrec, deUint32 shaderBits)
{
	tcu::TestCaseGroup* group = new tcu::TestCaseGroup(parent->getTestContext(), functionName, functionName);
	parent->addChild(group);

	const glu::DataType scalarTypes[] =
	{
		glu::TYPE_INT,
		glu::TYPE_UINT
	};

	for (int scalarTypeNdx = 0; scalarTypeNdx < DE_LENGTH_OF_ARRAY(scalarTypes); scalarTypeNdx++)
	{
		const glu::DataType scalarType = scalarTypes[scalarTypeNdx];

		if ((!intTypes && scalarType == glu::TYPE_INT) || (!uintTypes && scalarType == glu::TYPE_UINT))
			continue;

		for (int vecSize = 1; vecSize <= 4; vecSize++)
		{
			for (int prec = glu::PRECISION_LOWP; prec <= glu::PRECISION_HIGHP; prec++)
			{
				if (prec != glu::PRECISION_HIGHP && !allPrec)
					continue;

				for (int shaderTypeNdx = 0; shaderTypeNdx < glu::SHADERTYPE_LAST; shaderTypeNdx++)
				{
					if (shaderBits & (1<<shaderTypeNdx))
						group->addChild(new TestClass(parent->getContext(), glu::DataType(scalarType + vecSize - 1), glu::Precision(prec), glu::ShaderType(shaderTypeNdx)));
				}
			}
		}
	}
}

void ShaderIntegerFunctionTests::init (void)
{
	enum
	{
		VS = (1<<glu::SHADERTYPE_VERTEX),
		FS = (1<<glu::SHADERTYPE_FRAGMENT),
		CS = (1<<glu::SHADERTYPE_COMPUTE),
		GS = (1<<glu::SHADERTYPE_GEOMETRY),
		TC = (1<<glu::SHADERTYPE_TESSELLATION_CONTROL),
		TE = (1<<glu::SHADERTYPE_TESSELLATION_EVALUATION),

		ALL_SHADERS = VS|TC|TE|GS|FS|CS
	};

	//																		Int?	Uint?	AllPrec?	Shaders
	addFunctionCases<UaddCarryCase>				(this,	"uaddcarry",		false,	true,	true,		ALL_SHADERS);
	addFunctionCases<UsubBorrowCase>			(this,	"usubborrow",		false,	true,	true,		ALL_SHADERS);
	addFunctionCases<UmulExtendedCase>			(this,	"umulextended",		false,	true,	false,		ALL_SHADERS);
	addFunctionCases<ImulExtendedCase>			(this,	"imulextended",		true,	false,	false,		ALL_SHADERS);
	addFunctionCases<BitfieldExtractCase>		(this,	"bitfieldextract",	true,	true,	true,		ALL_SHADERS);
	addFunctionCases<BitfieldInsertCase>		(this,	"bitfieldinsert",	true,	true,	true,		ALL_SHADERS);
	addFunctionCases<BitfieldReverseCase>		(this,	"bitfieldreverse",	true,	true,	true,		ALL_SHADERS);
	addFunctionCases<BitCountCase>				(this,	"bitcount",			true,	true,	true,		ALL_SHADERS);
	addFunctionCases<FindLSBCase>				(this,	"findlsb",			true,	true,	true,		ALL_SHADERS);
	addFunctionCases<FindMSBCase>				(this,	"findmsb",			true,	true,	true,		ALL_SHADERS);
}

} // Functional
} // gles31
} // deqp
