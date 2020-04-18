/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Common built-in function tests.
 *//*--------------------------------------------------------------------*/

#include "vktShaderCommonFunctionTests.hpp"
#include "vktShaderExecutor.hpp"
#include "gluContextInfo.hpp"
#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuFloat.hpp"
#include "tcuInterval.hpp"
#include "tcuFloatFormat.hpp"
#include "deRandom.hpp"
#include "deMath.h"
#include "deString.h"
#include "deArrayUtil.hpp"
#include "deSharedPtr.hpp"

namespace vkt
{

namespace shaderexecutor
{


using std::vector;
using std::string;
using tcu::TestLog;

using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;
using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;

namespace
{

// Utilities

template<typename T, int Size>
struct VecArrayAccess
{
public:
									VecArrayAccess	(const void* ptr) : m_array((tcu::Vector<T, Size>*)ptr) {}
									~VecArrayAccess	(void) {}

	const tcu::Vector<T, Size>&		operator[]		(size_t offset) const	{ return m_array[offset];	}
	tcu::Vector<T, Size>&			operator[]		(size_t offset)			{ return m_array[offset];	}

private:
	tcu::Vector<T, Size>*			m_array;
};

template<typename T>	T			randomScalar	(de::Random& rnd, T minValue, T maxValue);
template<> inline		float		randomScalar	(de::Random& rnd, float minValue, float maxValue)		{ return rnd.getFloat(minValue, maxValue);	}
template<> inline		deInt32		randomScalar	(de::Random& rnd, deInt32 minValue, deInt32 maxValue)	{ return rnd.getInt(minValue, maxValue);	}

template<typename T, int Size>
inline tcu::Vector<T, Size> randomVector (de::Random& rnd, const tcu::Vector<T, Size>& minValue, const tcu::Vector<T, Size>& maxValue)
{
	tcu::Vector<T, Size> res;
	for (int ndx = 0; ndx < Size; ndx++)
		res[ndx] = randomScalar<T>(rnd, minValue[ndx], maxValue[ndx]);
	return res;
}

template<typename T, int Size>
static void fillRandomVectors (de::Random& rnd, const tcu::Vector<T, Size>& minValue, const tcu::Vector<T, Size>& maxValue, void* dst, int numValues, int offset = 0)
{
	VecArrayAccess<T, Size> access(dst);
	for (int ndx = 0; ndx < numValues; ndx++)
		access[offset + ndx] = randomVector<T, Size>(rnd, minValue, maxValue);
}

template<typename T>
static void fillRandomScalars (de::Random& rnd, T minValue, T maxValue, void* dst, int numValues, int offset = 0)
{
	T* typedPtr = (T*)dst;
	for (int ndx = 0; ndx < numValues; ndx++)
		typedPtr[offset + ndx] = randomScalar<T>(rnd, minValue, maxValue);
}

inline int numBitsLostInOp (float input, float output)
{
	const int	inExp		= tcu::Float32(input).exponent();
	const int	outExp		= tcu::Float32(output).exponent();

	return de::max(0, inExp-outExp); // Lost due to mantissa shift.
}

inline deUint32 getUlpDiff (float a, float b)
{
	const deUint32	aBits	= tcu::Float32(a).bits();
	const deUint32	bBits	= tcu::Float32(b).bits();
	return aBits > bBits ? aBits - bBits : bBits - aBits;
}

inline deUint32 getUlpDiffIgnoreZeroSign (float a, float b)
{
	if (tcu::Float32(a).isZero())
		return getUlpDiff(tcu::Float32::construct(tcu::Float32(b).sign(), 0, 0).asFloat(), b);
	else if (tcu::Float32(b).isZero())
		return getUlpDiff(a, tcu::Float32::construct(tcu::Float32(a).sign(), 0, 0).asFloat());
	else
		return getUlpDiff(a, b);
}

inline bool supportsSignedZero (glu::Precision precision)
{
	// \note GLSL ES 3.1 doesn't really require support for -0, but we require it for highp
	//		 as it is very widely supported.
	return precision == glu::PRECISION_HIGHP;
}

inline float getEpsFromMaxUlpDiff (float value, deUint32 ulpDiff)
{
	const int exp = tcu::Float32(value).exponent();
	return tcu::Float32::construct(+1, exp, (1u<<23) | ulpDiff).asFloat() - tcu::Float32::construct(+1, exp, 1u<<23).asFloat();
}

inline deUint32 getMaxUlpDiffFromBits (int numAccurateBits)
{
	const int		numGarbageBits	= 23-numAccurateBits;
	const deUint32	mask			= (1u<<numGarbageBits)-1u;

	return mask;
}

inline float getEpsFromBits (float value, int numAccurateBits)
{
	return getEpsFromMaxUlpDiff(value, getMaxUlpDiffFromBits(numAccurateBits));
}

static int getMinMantissaBits (glu::Precision precision)
{
	const int bits[] =
	{
		7,		// lowp
		10,		// mediump
		23		// highp
	};
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(bits) == glu::PRECISION_LAST);
	DE_ASSERT(de::inBounds<int>(precision, 0, DE_LENGTH_OF_ARRAY(bits)));
	return bits[precision];
}

static int getMaxNormalizedValueExponent (glu::Precision precision)
{
	const int exponent[] =
	{
		0,		// lowp
		13,		// mediump
		127		// highp
	};
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(exponent) == glu::PRECISION_LAST);
	DE_ASSERT(de::inBounds<int>(precision, 0, DE_LENGTH_OF_ARRAY(exponent)));
	return exponent[precision];
}

static int getMinNormalizedValueExponent (glu::Precision precision)
{
	const int exponent[] =
	{
		-7,		// lowp
		-13,	// mediump
		-126	// highp
	};
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(exponent) == glu::PRECISION_LAST);
	DE_ASSERT(de::inBounds<int>(precision, 0, DE_LENGTH_OF_ARRAY(exponent)));
	return exponent[precision];
}

static float makeFloatRepresentable (float f, glu::Precision precision)
{
	if (precision == glu::PRECISION_HIGHP)
	{
		// \note: assuming f is not extended-precision
		return f;
	}
	else
	{
		const int			numMantissaBits				= getMinMantissaBits(precision);
		const int			maxNormalizedValueExponent	= getMaxNormalizedValueExponent(precision);
		const int			minNormalizedValueExponent	= getMinNormalizedValueExponent(precision);
		const deUint32		representableMantissaMask	= ((deUint32(1) << numMantissaBits) - 1) << (23 - (deUint32)numMantissaBits);
		const float			largestRepresentableValue	= tcu::Float32::constructBits(+1, maxNormalizedValueExponent, ((1u << numMantissaBits) - 1u) << (23u - (deUint32)numMantissaBits)).asFloat();
		const bool			zeroNotRepresentable		= (precision == glu::PRECISION_LOWP);

		// if zero is not required to be representable, use smallest positive non-subnormal value
		const float			zeroValue					= (zeroNotRepresentable) ? (tcu::Float32::constructBits(+1, minNormalizedValueExponent, 1).asFloat()) : (0.0f);

		const tcu::Float32	float32Representation		(f);

		if (float32Representation.exponent() < minNormalizedValueExponent)
		{
			// flush too small values to zero
			return zeroValue;
		}
		else if (float32Representation.exponent() > maxNormalizedValueExponent)
		{
			// clamp too large values
			return (float32Representation.sign() == +1) ? (largestRepresentableValue) : (-largestRepresentableValue);
		}
		else
		{
			// remove unrepresentable mantissa bits
			const tcu::Float32 targetRepresentation(tcu::Float32::constructBits(float32Representation.sign(),
													float32Representation.exponent(),
													float32Representation.mantissaBits() & representableMantissaMask));

			return targetRepresentation.asFloat();
		}
	}
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

// \todo [2013-08-08 pyry] Make generic utility and move to glu?

struct HexFloat
{
	const float value;
	HexFloat (const float value_) : value(value_) {}
};

std::ostream& operator<< (std::ostream& str, const HexFloat& v)
{
	return str << v.value << " / " << tcu::toHex(tcu::Float32(v.value).bits());
}

struct HexBool
{
	const deUint32 value;
	HexBool (const deUint32 value_) : value(value_) {}
};

std::ostream& operator<< (std::ostream& str, const HexBool& v)
{
	return str << (v.value ? "true" : "false") << " / " << tcu::toHex(v.value);
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
			case glu::TYPE_FLOAT:	str << HexFloat(((const float*)varValue.value)[compNdx]);			break;
			case glu::TYPE_INT:		str << ((const deInt32*)varValue.value)[compNdx];					break;
			case glu::TYPE_UINT:	str << tcu::toHex(((const deUint32*)varValue.value)[compNdx]);		break;
			case glu::TYPE_BOOL:	str << HexBool(((const deUint32*)varValue.value)[compNdx]);			break;

			default:
				DE_ASSERT(false);
		}
	}

	if (numComponents > 1)
		str << ")";

	return str;
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

static std::string getCommonFuncCaseName (glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
{
	return string(glu::getDataTypeName(baseType)) + getPrecisionPostfix(precision) + getShaderTypePostfix(shaderType);
}

static inline void frexp (float in, float* significand, int* exponent)
{
	const tcu::Float32 fpValue(in);

	if (!fpValue.isZero())
	{
		// Construct float that has exactly the mantissa, and exponent of -1.
		*significand	= tcu::Float32::construct(fpValue.sign(), -1, fpValue.mantissa()).asFloat();
		*exponent		= fpValue.exponent()+1;
	}
	else
	{
		*significand	= fpValue.sign() < 0 ? -0.0f : 0.0f;
		*exponent		= 0;
	}
}

static inline float ldexp (float significand, int exponent)
{
	const tcu::Float32 mant(significand);

	if (exponent == 0 && mant.isZero())
	{
		return mant.sign() < 0 ? -0.0f : 0.0f;
	}
	else
	{
		return tcu::Float32::construct(mant.sign(), exponent+mant.exponent(), mant.mantissa()).asFloat();
	}
}

template<class TestClass>
static void addFunctionCases (tcu::TestCaseGroup* parent, const char* functionName, bool floatTypes, bool intTypes, bool uintTypes, deUint32 shaderBits)
{
	tcu::TestCaseGroup* group = new tcu::TestCaseGroup(parent->getTestContext(), functionName, functionName);
	parent->addChild(group);

	const glu::DataType scalarTypes[] =
	{
		glu::TYPE_FLOAT,
		glu::TYPE_INT,
		glu::TYPE_UINT
	};

	for (int scalarTypeNdx = 0; scalarTypeNdx < DE_LENGTH_OF_ARRAY(scalarTypes); scalarTypeNdx++)
	{
		const glu::DataType scalarType = scalarTypes[scalarTypeNdx];

		if ((!floatTypes && scalarType == glu::TYPE_FLOAT)	||
			(!intTypes && scalarType == glu::TYPE_INT)		||
			(!uintTypes && scalarType == glu::TYPE_UINT))
			continue;

		for (int vecSize = 1; vecSize <= 4; vecSize++)
		{
			for (int prec = glu::PRECISION_MEDIUMP; prec <= glu::PRECISION_HIGHP; prec++)
			{
				for (int shaderTypeNdx = 0; shaderTypeNdx < glu::SHADERTYPE_LAST; shaderTypeNdx++)
				{
					if (shaderBits & (1<<shaderTypeNdx))
						group->addChild(new TestClass(parent->getTestContext(), glu::DataType(scalarType + vecSize - 1), glu::Precision(prec), glu::ShaderType(shaderTypeNdx)));
				}
			}
		}
	}
}

// CommonFunctionCase

class CommonFunctionCase : public TestCase
{
public:
										CommonFunctionCase			(tcu::TestContext& testCtx, const char* name, const char* description, glu::ShaderType shaderType);
										~CommonFunctionCase			(void);
	virtual	void						initPrograms				(vk::SourceCollections& programCollection) const
										{
											generateSources(m_shaderType, m_spec, programCollection);
										}

	virtual TestInstance*				createInstance				(Context& context) const = 0;

protected:
										CommonFunctionCase			(const CommonFunctionCase&);
	CommonFunctionCase&					operator=					(const CommonFunctionCase&);

	const glu::ShaderType				m_shaderType;
	ShaderSpec							m_spec;
	const int							m_numValues;
};

CommonFunctionCase::CommonFunctionCase (tcu::TestContext& testCtx, const char* name, const char* description, glu::ShaderType shaderType)
	: TestCase		(testCtx, name, description)
	, m_shaderType	(shaderType)
	, m_numValues	(100)
{
}

CommonFunctionCase::~CommonFunctionCase (void)
{
}

// CommonFunctionTestInstance

class CommonFunctionTestInstance : public TestInstance
{
public:
										CommonFunctionTestInstance	(Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
											: TestInstance	(context)
											, m_shaderType	(shaderType)
											, m_spec		(spec)
											, m_numValues	(numValues)
											, m_name		(name)
											, m_executor	(createExecutor(context, shaderType, spec))
										{
										}
	virtual tcu::TestStatus				iterate						(void);

protected:
	virtual void						getInputValues				(int numValues, void* const* values) const = 0;
	virtual bool						compare						(const void* const* inputs, const void* const* outputs) = 0;

	const glu::ShaderType				m_shaderType;
	const ShaderSpec					m_spec;
	const int							m_numValues;

	// \todo [2017-03-07 pyry] Hack used to generate seeds for test cases - get rid of this.
	const char*							m_name;

	std::ostringstream					m_failMsg;					//!< Comparison failure help message.

	de::UniquePtr<ShaderExecutor>		m_executor;
};

tcu::TestStatus CommonFunctionTestInstance::iterate (void)
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
	m_executor->execute(m_numValues, &inputPointers[0], &outputPointers[0]);

	// Compare results.
	{
		const vector<int>		inScalarSizes		= getScalarSizes(m_spec.inputs);
		const vector<int>		outScalarSizes		= getScalarSizes(m_spec.outputs);
		vector<void*>			curInputPtr			(inputPointers.size());
		vector<void*>			curOutputPtr		(outputPointers.size());
		int						numFailed			= 0;
		tcu::TestContext&		testCtx				= m_context.getTestContext();

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

				testCtx.getLog() << TestLog::Message << "ERROR: comparison failed for value " << valNdx << ":\n  " << m_failMsg.str() << TestLog::EndMessage;

				testCtx.getLog() << TestLog::Message << "  inputs:" << TestLog::EndMessage;
				for (int inNdx = 0; inNdx < (int)curInputPtr.size(); inNdx++)
					testCtx.getLog() << TestLog::Message << "    " << m_spec.inputs[inNdx].name << " = "
														   << VarValue(m_spec.inputs[inNdx].varType, curInputPtr[inNdx])
									   << TestLog::EndMessage;

				testCtx.getLog() << TestLog::Message << "  outputs:" << TestLog::EndMessage;
				for (int outNdx = 0; outNdx < (int)curOutputPtr.size(); outNdx++)
					testCtx.getLog() << TestLog::Message << "    " << m_spec.outputs[outNdx].name << " = "
														   << VarValue(m_spec.outputs[outNdx].varType, curOutputPtr[outNdx])
									   << TestLog::EndMessage;

				m_failMsg.str("");
				m_failMsg.clear();
				numFailed += 1;
			}
		}

		testCtx.getLog() << TestLog::Message << (m_numValues - numFailed) << " / " << m_numValues << " values passed" << TestLog::EndMessage;

		if (numFailed == 0)
			return tcu::TestStatus::pass("Pass");
		else
			return tcu::TestStatus::fail("Result comparison failed");
	}
}

// Test cases

class AbsCaseInstance : public CommonFunctionTestInstance
{
public:
	AbsCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 floatRanges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};
		const IVec2 intRanges[] =
		{
			IVec2(-(1<<7)+1,	(1<<7)-1),
			IVec2(-(1<<15)+1,	(1<<15)-1),
			IVec2(0x80000001,	0x7fffffff)
		};

		de::Random				rnd			(deStringHash(m_name) ^ 0x235facu);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);

		if (glu::isDataTypeFloatOrVec(type))
			fillRandomScalars(rnd, floatRanges[precision].x(), floatRanges[precision].y(), values[0], numValues*scalarSize);
		else
			fillRandomScalars(rnd, intRanges[precision].x(), intRanges[precision].y(), values[0], numValues*scalarSize);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (glu::isDataTypeFloatOrVec(type))
		{
			const int		mantissaBits	= getMinMantissaBits(precision);
			const deUint32	maxUlpDiff		= (1u<<(23-mantissaBits))-1u;

			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const float		ref0		= de::abs(in0);
				const deUint32	ulpDiff0	= getUlpDiff(out0, ref0);

				if (ulpDiff0 > maxUlpDiff)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref0) << " with ULP threshold " << maxUlpDiff << ", got ULP diff " << ulpDiff0;
					return false;
				}
			}
		}
		else
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const int	in0		= ((const int*)inputs[0])[compNdx];
				const int	out0	= ((const int*)outputs[0])[compNdx];
				const int	ref0	= de::abs(in0);

				if (out0 != ref0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << ref0;
					return false;
				}
			}
		}

		return true;
	}
};

class AbsCase : public CommonFunctionCase
{
public:
	AbsCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "abs", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = abs(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new AbsCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class SignCaseInstance : public CommonFunctionTestInstance
{
public:
	SignCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 floatRanges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e4f,		1e4f),	// mediump	- note: may end up as inf
			Vec2(-1e8f,		1e8f)	// highp	- note: may end up as inf
		};
		const IVec2 intRanges[] =
		{
			IVec2(-(1<<7),		(1<<7)-1),
			IVec2(-(1<<15),		(1<<15)-1),
			IVec2(0x80000000,	0x7fffffff)
		};

		de::Random				rnd			(deStringHash(m_name) ^ 0x324u);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);

		if (glu::isDataTypeFloatOrVec(type))
		{
			// Special cases.
			std::fill((float*)values[0], (float*)values[0] + scalarSize, +1.0f);
			std::fill((float*)values[0], (float*)values[0] + scalarSize, -1.0f);
			std::fill((float*)values[0], (float*)values[0] + scalarSize,  0.0f);
			fillRandomScalars(rnd, floatRanges[precision].x(), floatRanges[precision].y(), (float*)values[0] + scalarSize*3, (numValues-3)*scalarSize);
		}
		else
		{
			std::fill((int*)values[0], (int*)values[0] + scalarSize, +1);
			std::fill((int*)values[0], (int*)values[0] + scalarSize, -1);
			std::fill((int*)values[0], (int*)values[0] + scalarSize,  0);
			fillRandomScalars(rnd, intRanges[precision].x(), intRanges[precision].y(), (int*)values[0] + scalarSize*3, (numValues-3)*scalarSize);
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (glu::isDataTypeFloatOrVec(type))
		{
			// Both highp and mediump should be able to represent -1, 0, and +1 exactly
			const deUint32 maxUlpDiff = precision == glu::PRECISION_LOWP ? getMaxUlpDiffFromBits(getMinMantissaBits(precision)) : 0;

			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const float		ref0		= in0 < 0.0f ? -1.0f :
											  in0 > 0.0f ? +1.0f : 0.0f;
				const deUint32	ulpDiff0	= getUlpDiff(out0, ref0);

				if (ulpDiff0 > maxUlpDiff)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref0) << " with ULP threshold " << maxUlpDiff << ", got ULP diff " << ulpDiff0;
					return false;
				}
			}
		}
		else
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const int	in0		= ((const int*)inputs[0])[compNdx];
				const int	out0	= ((const int*)outputs[0])[compNdx];
				const int	ref0	= in0 < 0 ? -1 :
									  in0 > 0 ? +1 : 0;

				if (out0 != ref0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << ref0;
					return false;
				}
			}
		}

		return true;
	}
};

class SignCase : public CommonFunctionCase
{
public:
	SignCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "sign", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = sign(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new SignCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

static float roundEven (float v)
{
	const float		q			= deFloatFrac(v);
	const int		truncated	= int(v-q);
	const int		rounded		= (q > 0.5f)							? (truncated + 1) :	// Rounded up
									(q == 0.5f && (truncated % 2 != 0))	? (truncated + 1) :	// Round to nearest even at 0.5
									truncated;												// Rounded down

	return float(rounded);
}

class RoundEvenCaseInstance : public CommonFunctionTestInstance
{
public:
	RoundEvenCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd				(deStringHash(m_name) ^ 0xac23fu);
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		int						numSpecialCases	= 0;

		// Special cases.
		if (precision != glu::PRECISION_LOWP)
		{
			DE_ASSERT(numValues >= 20);
			for (int ndx = 0; ndx < 20; ndx++)
			{
				const float v = de::clamp(float(ndx) - 10.5f, ranges[precision].x(), ranges[precision].y());
				std::fill((float*)values[0], (float*)values[0] + scalarSize, v);
				numSpecialCases += 1;
			}
		}

		// Random cases.
		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), (float*)values[0] + numSpecialCases*scalarSize, (numValues-numSpecialCases)*scalarSize);

		// If precision is mediump, make sure values can be represented in fp16 exactly
		if (precision == glu::PRECISION_MEDIUMP)
		{
			for (int ndx = 0; ndx < numValues*scalarSize; ndx++)
				((float*)values[0])[ndx] = tcu::Float16(((float*)values[0])[ndx]).asFloat();
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const bool				hasSignedZero	= supportsSignedZero(precision);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (precision == glu::PRECISION_HIGHP || precision == glu::PRECISION_MEDIUMP)
		{
			// Require exact rounding result.
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const float		ref			= roundEven(in0);

				const deUint32	ulpDiff		= hasSignedZero ? getUlpDiff(out0, ref) : getUlpDiffIgnoreZeroSign(out0, ref);

				if (ulpDiff > 0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref) << ", got ULP diff " << tcu::toHex(ulpDiff);
					return false;
				}
			}
		}
		else
		{
			const int		mantissaBits	= getMinMantissaBits(precision);
			const deUint32	maxUlpDiff		= getMaxUlpDiffFromBits(mantissaBits);	// ULP diff for rounded integer value.
			const float		eps				= getEpsFromBits(1.0f, mantissaBits);	// epsilon for rounding bounds

			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const int		minRes		= int(roundEven(in0-eps));
				const int		maxRes		= int(roundEven(in0+eps));
				bool			anyOk		= false;

				for (int roundedVal = minRes; roundedVal <= maxRes; roundedVal++)
				{
					const deUint32 ulpDiff = getUlpDiffIgnoreZeroSign(out0, float(roundedVal));

					if (ulpDiff <= maxUlpDiff)
					{
						anyOk = true;
						break;
					}
				}

				if (!anyOk)
				{
					m_failMsg << "Expected [" << compNdx << "] = [" << minRes << ", " << maxRes << "] with ULP threshold " << tcu::toHex(maxUlpDiff);
					return false;
				}
			}
		}

		return true;
	}
};

class RoundEvenCase : public CommonFunctionCase
{
public:
	RoundEvenCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "roundEven", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = roundEven(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new RoundEvenCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class ModfCaseInstance : public CommonFunctionTestInstance
{
public:
	ModfCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd			(deStringHash(m_name) ^ 0xac23fu);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);

		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), values[0], numValues*scalarSize);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const bool				hasZeroSign		= supportsSignedZero(precision);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		const int				mantissaBits	= getMinMantissaBits(precision);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const float		in0			= ((const float*)inputs[0])[compNdx];
			const float		out0		= ((const float*)outputs[0])[compNdx];
			const float		out1		= ((const float*)outputs[1])[compNdx];

			const float		refOut1		= float(int(in0));
			const float		refOut0		= in0 - refOut1;

			const int		bitsLost	= precision != glu::PRECISION_HIGHP ? numBitsLostInOp(in0, refOut0) : 0;
			const deUint32	maxUlpDiff	= getMaxUlpDiffFromBits(de::max(mantissaBits - bitsLost, 0));

			const float		resSum		= out0 + out1;

			const deUint32	ulpDiff		= hasZeroSign ? getUlpDiff(resSum, in0) : getUlpDiffIgnoreZeroSign(resSum, in0);

			if (ulpDiff > maxUlpDiff)
			{
				m_failMsg << "Expected [" << compNdx << "] = (" << HexFloat(refOut0) << ") + (" << HexFloat(refOut1) << ") = " << HexFloat(in0) << " with ULP threshold "
							<< tcu::toHex(maxUlpDiff) << ", got ULP diff " << tcu::toHex(ulpDiff);
				return false;
			}
		}

		return true;
	}
};

class ModfCase : public CommonFunctionCase
{
public:
	ModfCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "modf", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out1", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = modf(in0, out1);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new ModfCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class IsnanCaseInstance : public CommonFunctionTestInstance
{
public:
	IsnanCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd				(deStringHash(m_name) ^ 0xc2a39fu);
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				mantissaBits	= getMinMantissaBits(precision);
		const deUint32			mantissaMask	= ~getMaxUlpDiffFromBits(mantissaBits) & ((1u<<23)-1u);

		for (int valNdx = 0; valNdx < numValues*scalarSize; valNdx++)
		{
			const bool		isNan		= rnd.getFloat() > 0.3f;
			const bool		isInf		= !isNan && rnd.getFloat() > 0.4f;
			const deUint32	mantissa	= !isInf ? ((1u<<22) | (rnd.getUint32() & mantissaMask)) : 0;
			const deUint32	exp			= !isNan && !isInf ? (rnd.getUint32() & 0x7fu) : 0xffu;
			const deUint32	sign		= rnd.getUint32() & 0x1u;
			const deUint32	value		= (sign << 31) | (exp << 23) | mantissa;

			DE_ASSERT(tcu::Float32(value).isInf() == isInf && tcu::Float32(value).isNaN() == isNan);

			((deUint32*)values[0])[valNdx] = value;
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (precision == glu::PRECISION_HIGHP)
		{
			// Only highp is required to support inf/nan
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0		= ((const float*)inputs[0])[compNdx];
				const bool		out0	= ((const deUint32*)outputs[0])[compNdx] != 0;
				const bool		ref		= tcu::Float32(in0).isNaN();

				if (out0 != ref)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << (ref ? "true" : "false");
					return false;
				}
			}
		}
		else if (precision == glu::PRECISION_MEDIUMP || precision == glu::PRECISION_LOWP)
		{
			// NaN support is optional, check that inputs that are not NaN don't result in true.
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0		= ((const float*)inputs[0])[compNdx];
				const bool		out0	= ((const deUint32*)outputs[0])[compNdx] != 0;
				const bool		ref		= tcu::Float32(in0).isNaN();

				if (!ref && out0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << (ref ? "true" : "false");
					return false;
				}
			}
		}

		return true;
	}
};

class IsnanCase : public CommonFunctionCase
{
public:
	IsnanCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "isnan", shaderType)
	{
		DE_ASSERT(glu::isDataTypeFloatOrVec(baseType));

		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	boolType	= vecSize > 1 ? glu::getDataTypeBoolVec(vecSize) : glu::TYPE_BOOL;

		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(boolType, glu::PRECISION_LAST)));
		m_spec.source = "out0 = isnan(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new IsnanCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class IsinfCaseInstance : public CommonFunctionTestInstance
{
public:
	IsinfCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd				(deStringHash(m_name) ^ 0xc2a39fu);
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const int				mantissaBits	= getMinMantissaBits(precision);
		const deUint32			mantissaMask	= ~getMaxUlpDiffFromBits(mantissaBits) & ((1u<<23)-1u);

		for (int valNdx = 0; valNdx < numValues*scalarSize; valNdx++)
		{
			const bool		isInf		= rnd.getFloat() > 0.3f;
			const bool		isNan		= !isInf && rnd.getFloat() > 0.4f;
			const deUint32	mantissa	= !isInf ? ((1u<<22) | (rnd.getUint32() & mantissaMask)) : 0;
			const deUint32	exp			= !isNan && !isInf ? (rnd.getUint32() & 0x7fu) : 0xffu;
			const deUint32	sign		= rnd.getUint32() & 0x1u;
			const deUint32	value		= (sign << 31) | (exp << 23) | mantissa;

			DE_ASSERT(tcu::Float32(value).isInf() == isInf && tcu::Float32(value).isNaN() == isNan);

			((deUint32*)values[0])[valNdx] = value;
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (precision == glu::PRECISION_HIGHP)
		{
			// Only highp is required to support inf/nan
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0		= ((const float*)inputs[0])[compNdx];
				const bool		out0	= ((const deUint32*)outputs[0])[compNdx] != 0;
				const bool		ref		= tcu::Float32(in0).isInf();

				if (out0 != ref)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << HexBool(ref);
					return false;
				}
			}
		}
		else if (precision == glu::PRECISION_MEDIUMP)
		{
			// Inf support is optional, check that inputs that are not Inf in mediump don't result in true.
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0		= ((const float*)inputs[0])[compNdx];
				const bool		out0	= ((const deUint32*)outputs[0])[compNdx] != 0;
				const bool		ref		= tcu::Float16(in0).isInf();

				if (!ref && out0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << (ref ? "true" : "false");
					return false;
				}
			}
		}
		// else: no verification can be performed

		return true;
	}
};

class IsinfCase : public CommonFunctionCase
{
public:
	IsinfCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "isinf", shaderType)
	{
		DE_ASSERT(glu::isDataTypeFloatOrVec(baseType));

		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	boolType	= vecSize > 1 ? glu::getDataTypeBoolVec(vecSize) : glu::TYPE_BOOL;

		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(boolType, glu::PRECISION_LAST)));
		m_spec.source = "out0 = isinf(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new IsinfCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class FloatBitsToUintIntCaseInstance : public CommonFunctionTestInstance
{
public:
	FloatBitsToUintIntCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd			(deStringHash(m_name) ^ 0x2790au);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);

		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), values[0], numValues*scalarSize);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		const int				mantissaBits	= getMinMantissaBits(precision);
		const int				maxUlpDiff		= getMaxUlpDiffFromBits(mantissaBits);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const float		in0			= ((const float*)inputs[0])[compNdx];
			const deUint32	out0		= ((const deUint32*)outputs[0])[compNdx];
			const deUint32	refOut0		= tcu::Float32(in0).bits();
			const int		ulpDiff		= de::abs((int)out0 - (int)refOut0);

			if (ulpDiff > maxUlpDiff)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(refOut0) << " with threshold "
							<< tcu::toHex(maxUlpDiff) << ", got diff " << tcu::toHex(ulpDiff);
				return false;
			}
		}

		return true;
	}
};

class FloatBitsToUintIntCase : public CommonFunctionCase
{
public:
	FloatBitsToUintIntCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType, bool outIsSigned)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), outIsSigned ? "floatBitsToInt" : "floatBitsToUint", shaderType)
	{
		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	intType		= outIsSigned ? (vecSize > 1 ? glu::getDataTypeIntVec(vecSize) : glu::TYPE_INT)
													  : (vecSize > 1 ? glu::getDataTypeUintVec(vecSize) : glu::TYPE_UINT);

		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(intType, glu::PRECISION_HIGHP)));
		m_spec.source = outIsSigned ? "out0 = floatBitsToInt(in0);" : "out0 = floatBitsToUint(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new FloatBitsToUintIntCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class FloatBitsToIntCaseInstance : public FloatBitsToUintIntCaseInstance
{
public:
	FloatBitsToIntCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: FloatBitsToUintIntCaseInstance	(context, shaderType, spec, numValues, name)
	{
	}
};

class FloatBitsToIntCase : public FloatBitsToUintIntCase
{
public:
	FloatBitsToIntCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: FloatBitsToUintIntCase	(testCtx, baseType, precision, shaderType, true)
	{
	}

};

class FloatBitsToUintCaseInstance : public FloatBitsToUintIntCaseInstance
{
public:
	FloatBitsToUintCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: FloatBitsToUintIntCaseInstance	(context, shaderType, spec, numValues, name)
	{
	}
};

class FloatBitsToUintCase : public FloatBitsToUintIntCase
{
public:
	FloatBitsToUintCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: FloatBitsToUintIntCase	(testCtx, baseType, precision, shaderType, false)
	{
	}
};

class BitsToFloatCaseInstance : public CommonFunctionTestInstance
{
public:
	BitsToFloatCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		de::Random				rnd			(deStringHash(m_name) ^ 0xbbb225u);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);
		const Vec2				range		(-1e8f, +1e8f);

		// \note Filled as floats.
		fillRandomScalars(rnd, range.x(), range.y(), values[0], numValues*scalarSize);
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const deUint32			maxUlpDiff		= 0;

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const float		in0			= ((const float*)inputs[0])[compNdx];
			const float		out0		= ((const float*)outputs[0])[compNdx];
			const deUint32	ulpDiff		= getUlpDiff(in0, out0);

			if (ulpDiff > maxUlpDiff)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << tcu::toHex(tcu::Float32(in0).bits()) << " with ULP threshold "
							<< tcu::toHex(maxUlpDiff) << ", got ULP diff " << tcu::toHex(ulpDiff);
				return false;
			}
		}

		return true;
	}
};

class BitsToFloatCase : public CommonFunctionCase
{
public:
	BitsToFloatCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, glu::PRECISION_HIGHP, shaderType).c_str(), glu::isDataTypeIntOrIVec(baseType) ? "intBitsToFloat" : "uintBitsToFloat", shaderType)
	{
		const bool			inIsSigned	= glu::isDataTypeIntOrIVec(baseType);
		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	floatType	= vecSize > 1 ? glu::getDataTypeFloatVec(vecSize) : glu::TYPE_FLOAT;

		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, glu::PRECISION_HIGHP)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(floatType, glu::PRECISION_HIGHP)));
		m_spec.source = inIsSigned ? "out0 = intBitsToFloat(in0);" : "out0 = uintBitsToFloat(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new BitsToFloatCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class FloorCaseInstance : public CommonFunctionTestInstance
{
public:
	FloorCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd			(deStringHash(m_name) ^ 0xac23fu);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);
		// Random cases.
		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), (float*)values[0], numValues*scalarSize);

		// If precision is mediump, make sure values can be represented in fp16 exactly
		if (precision == glu::PRECISION_MEDIUMP)
		{
			for (int ndx = 0; ndx < numValues*scalarSize; ndx++)
				((float*)values[0])[ndx] = tcu::Float16(((float*)values[0])[ndx]).asFloat();
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (precision == glu::PRECISION_HIGHP || precision == glu::PRECISION_MEDIUMP)
		{
			// Require exact result.
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const float		ref			= deFloatFloor(in0);

				const deUint32	ulpDiff		= getUlpDiff(out0, ref);

				if (ulpDiff > 0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref) << ", got ULP diff " << tcu::toHex(ulpDiff);
					return false;
				}
			}
		}
		else
		{
			const int		mantissaBits	= getMinMantissaBits(precision);
			const deUint32	maxUlpDiff		= getMaxUlpDiffFromBits(mantissaBits);	// ULP diff for rounded integer value.
			const float		eps				= getEpsFromBits(1.0f, mantissaBits);	// epsilon for rounding bounds

			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const int		minRes		= int(deFloatFloor(in0-eps));
				const int		maxRes		= int(deFloatFloor(in0+eps));
				bool			anyOk		= false;

				for (int roundedVal = minRes; roundedVal <= maxRes; roundedVal++)
				{
					const deUint32 ulpDiff = getUlpDiff(out0, float(roundedVal));

					if (ulpDiff <= maxUlpDiff)
					{
						anyOk = true;
						break;
					}
				}

				if (!anyOk)
				{
					m_failMsg << "Expected [" << compNdx << "] = [" << minRes << ", " << maxRes << "] with ULP threshold " << tcu::toHex(maxUlpDiff);
					return false;
				}
			}
		}

		return true;
	}
};

class FloorCase : public CommonFunctionCase
{
public:
	FloorCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "floor", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = floor(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new FloorCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class TruncCaseInstance : public CommonFunctionTestInstance
{
public:
	TruncCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd				(deStringHash(m_name) ^ 0xac23fu);
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		const float				specialCases[]	= { 0.0f, -0.0f, -0.9f, 0.9f, 1.0f, -1.0f };
		const int				numSpecialCases	= DE_LENGTH_OF_ARRAY(specialCases);

		// Special cases
		for (int caseNdx = 0; caseNdx < numSpecialCases; caseNdx++)
		{
			for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
				((float*)values[0])[caseNdx*scalarSize + scalarNdx] = specialCases[caseNdx];
		}

		// Random cases.
		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), (float*)values[0] + scalarSize*numSpecialCases, (numValues-numSpecialCases)*scalarSize);

		// If precision is mediump, make sure values can be represented in fp16 exactly
		if (precision == glu::PRECISION_MEDIUMP)
		{
			for (int ndx = 0; ndx < numValues*scalarSize; ndx++)
				((float*)values[0])[ndx] = tcu::Float16(((float*)values[0])[ndx]).asFloat();
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (precision == glu::PRECISION_HIGHP || precision == glu::PRECISION_MEDIUMP)
		{
			// Require exact result.
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const bool		isNeg		= tcu::Float32(in0).sign() < 0;
				const float		ref			= isNeg ? (-float(int(-in0))) : float(int(in0));

				// \note: trunc() function definition is a bit broad on negative zeros. Ignore result sign if zero.
				const deUint32	ulpDiff		= getUlpDiffIgnoreZeroSign(out0, ref);

				if (ulpDiff > 0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref) << ", got ULP diff " << tcu::toHex(ulpDiff);
					return false;
				}
			}
		}
		else
		{
			const int		mantissaBits	= getMinMantissaBits(precision);
			const deUint32	maxUlpDiff		= getMaxUlpDiffFromBits(mantissaBits);	// ULP diff for rounded integer value.
			const float		eps				= getEpsFromBits(1.0f, mantissaBits);	// epsilon for rounding bounds

			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const int		minRes		= int(in0-eps);
				const int		maxRes		= int(in0+eps);
				bool			anyOk		= false;

				for (int roundedVal = minRes; roundedVal <= maxRes; roundedVal++)
				{
					const deUint32 ulpDiff = getUlpDiffIgnoreZeroSign(out0, float(roundedVal));

					if (ulpDiff <= maxUlpDiff)
					{
						anyOk = true;
						break;
					}
				}

				if (!anyOk)
				{
					m_failMsg << "Expected [" << compNdx << "] = [" << minRes << ", " << maxRes << "] with ULP threshold " << tcu::toHex(maxUlpDiff);
					return false;
				}
			}
		}

		return true;
	}
};

class TruncCase : public CommonFunctionCase
{
public:
	TruncCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "trunc", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = trunc(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new TruncCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class RoundCaseInstance : public CommonFunctionTestInstance
{
public:
	RoundCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd				(deStringHash(m_name) ^ 0xac23fu);
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		int						numSpecialCases	= 0;

		// Special cases.
		if (precision != glu::PRECISION_LOWP)
		{
			DE_ASSERT(numValues >= 10);
			for (int ndx = 0; ndx < 10; ndx++)
			{
				const float v = de::clamp(float(ndx) - 5.5f, ranges[precision].x(), ranges[precision].y());
				std::fill((float*)values[0], (float*)values[0] + scalarSize, v);
				numSpecialCases += 1;
			}
		}

		// Random cases.
		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), (float*)values[0] + numSpecialCases*scalarSize, (numValues-numSpecialCases)*scalarSize);

		// If precision is mediump, make sure values can be represented in fp16 exactly
		if (precision == glu::PRECISION_MEDIUMP)
		{
			for (int ndx = 0; ndx < numValues*scalarSize; ndx++)
				((float*)values[0])[ndx] = tcu::Float16(((float*)values[0])[ndx]).asFloat();
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const bool				hasZeroSign		= supportsSignedZero(precision);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (precision == glu::PRECISION_HIGHP || precision == glu::PRECISION_MEDIUMP)
		{
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];

				if (deFloatFrac(in0) == 0.5f)
				{
					// Allow both ceil(in) and floor(in)
					const float		ref0		= deFloatFloor(in0);
					const float		ref1		= deFloatCeil(in0);
					const deUint32	ulpDiff0	= hasZeroSign ? getUlpDiff(out0, ref0) : getUlpDiffIgnoreZeroSign(out0, ref0);
					const deUint32	ulpDiff1	= hasZeroSign ? getUlpDiff(out0, ref1) : getUlpDiffIgnoreZeroSign(out0, ref1);

					if (ulpDiff0 > 0 && ulpDiff1 > 0)
					{
						m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref0) << " or " << HexFloat(ref1) << ", got ULP diff " << tcu::toHex(de::min(ulpDiff0, ulpDiff1));
						return false;
					}
				}
				else
				{
					// Require exact result
					const float		ref		= roundEven(in0);
					const deUint32	ulpDiff	= hasZeroSign ? getUlpDiff(out0, ref) : getUlpDiffIgnoreZeroSign(out0, ref);

					if (ulpDiff > 0)
					{
						m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref) << ", got ULP diff " << tcu::toHex(ulpDiff);
						return false;
					}
				}
			}
		}
		else
		{
			const int		mantissaBits	= getMinMantissaBits(precision);
			const deUint32	maxUlpDiff		= getMaxUlpDiffFromBits(mantissaBits);	// ULP diff for rounded integer value.
			const float		eps				= getEpsFromBits(1.0f, mantissaBits);	// epsilon for rounding bounds

			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const int		minRes		= int(roundEven(in0-eps));
				const int		maxRes		= int(roundEven(in0+eps));
				bool			anyOk		= false;

				for (int roundedVal = minRes; roundedVal <= maxRes; roundedVal++)
				{
					const deUint32 ulpDiff = getUlpDiffIgnoreZeroSign(out0, float(roundedVal));

					if (ulpDiff <= maxUlpDiff)
					{
						anyOk = true;
						break;
					}
				}

				if (!anyOk)
				{
					m_failMsg << "Expected [" << compNdx << "] = [" << minRes << ", " << maxRes << "] with ULP threshold " << tcu::toHex(maxUlpDiff);
					return false;
				}
			}
		}

		return true;
	}
};

class RoundCase : public CommonFunctionCase
{
public:
	RoundCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "round", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = round(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new RoundCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class CeilCaseInstance : public CommonFunctionTestInstance
{
public:
	CeilCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd			(deStringHash(m_name) ^ 0xac23fu);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);

		// Random cases.
		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), (float*)values[0], numValues*scalarSize);

		// If precision is mediump, make sure values can be represented in fp16 exactly
		if (precision == glu::PRECISION_MEDIUMP)
		{
			for (int ndx = 0; ndx < numValues*scalarSize; ndx++)
				((float*)values[0])[ndx] = tcu::Float16(((float*)values[0])[ndx]).asFloat();
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const bool				hasZeroSign		= supportsSignedZero(precision);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (precision == glu::PRECISION_HIGHP || precision == glu::PRECISION_MEDIUMP)
		{
			// Require exact result.
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const float		ref			= deFloatCeil(in0);

				const deUint32	ulpDiff		= hasZeroSign ? getUlpDiff(out0, ref) : getUlpDiffIgnoreZeroSign(out0, ref);

				if (ulpDiff > 0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref) << ", got ULP diff " << tcu::toHex(ulpDiff);
					return false;
				}
			}
		}
		else
		{
			const int		mantissaBits	= getMinMantissaBits(precision);
			const deUint32	maxUlpDiff		= getMaxUlpDiffFromBits(mantissaBits);	// ULP diff for rounded integer value.
			const float		eps				= getEpsFromBits(1.0f, mantissaBits);	// epsilon for rounding bounds

			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const int		minRes		= int(deFloatCeil(in0-eps));
				const int		maxRes		= int(deFloatCeil(in0+eps));
				bool			anyOk		= false;

				for (int roundedVal = minRes; roundedVal <= maxRes; roundedVal++)
				{
					const deUint32 ulpDiff = getUlpDiffIgnoreZeroSign(out0, float(roundedVal));

					if (ulpDiff <= maxUlpDiff)
					{
						anyOk = true;
						break;
					}
				}

				if (!anyOk && de::inRange(0, minRes, maxRes))
				{
					// Allow -0 as well.
					const int ulpDiff = de::abs((int)tcu::Float32(out0).bits() - (int)0x80000000u);
					anyOk = ((deUint32)ulpDiff <= maxUlpDiff);
				}

				if (!anyOk)
				{
					m_failMsg << "Expected [" << compNdx << "] = [" << minRes << ", " << maxRes << "] with ULP threshold " << tcu::toHex(maxUlpDiff);
					return false;
				}
			}
		}

		return true;
	}
};

class CeilCase : public CommonFunctionCase
{
public:
	CeilCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "ceil", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = ceil(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new CeilCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class FractCaseInstance : public CommonFunctionTestInstance
{
public:
	FractCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd				(deStringHash(m_name) ^ 0xac23fu);
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);
		int						numSpecialCases	= 0;

		// Special cases.
		if (precision != glu::PRECISION_LOWP)
		{
			DE_ASSERT(numValues >= 10);
			for (int ndx = 0; ndx < 10; ndx++)
			{
				const float v = de::clamp(float(ndx) - 5.5f, ranges[precision].x(), ranges[precision].y());
				std::fill((float*)values[0], (float*)values[0] + scalarSize, v);
				numSpecialCases += 1;
			}
		}

		// Random cases.
		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), (float*)values[0] + numSpecialCases*scalarSize, (numValues-numSpecialCases)*scalarSize);

		// If precision is mediump, make sure values can be represented in fp16 exactly
		if (precision == glu::PRECISION_MEDIUMP)
		{
			for (int ndx = 0; ndx < numValues*scalarSize; ndx++)
				((float*)values[0])[ndx] = tcu::Float16(((float*)values[0])[ndx]).asFloat();
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const bool				hasZeroSign		= supportsSignedZero(precision);
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		if (precision == glu::PRECISION_HIGHP || precision == glu::PRECISION_MEDIUMP)
		{
			// Require exact result.
			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];
				const float		ref			= deFloatFrac(in0);

				const deUint32	ulpDiff		= hasZeroSign ? getUlpDiff(out0, ref) : getUlpDiffIgnoreZeroSign(out0, ref);

				if (ulpDiff > 0)
				{
					m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref) << ", got ULP diff " << tcu::toHex(ulpDiff);
					return false;
				}
			}
		}
		else
		{
			const int		mantissaBits	= getMinMantissaBits(precision);
			const float		eps				= getEpsFromBits(1.0f, mantissaBits);	// epsilon for rounding bounds

			for (int compNdx = 0; compNdx < scalarSize; compNdx++)
			{
				const float		in0			= ((const float*)inputs[0])[compNdx];
				const float		out0		= ((const float*)outputs[0])[compNdx];

				if (int(deFloatFloor(in0-eps)) == int(deFloatFloor(in0+eps)))
				{
					const float		ref			= deFloatFrac(in0);
					const int		bitsLost	= numBitsLostInOp(in0, ref);
					const deUint32	maxUlpDiff	= getMaxUlpDiffFromBits(de::max(0, mantissaBits-bitsLost));	// ULP diff for rounded integer value.
					const deUint32	ulpDiff		= getUlpDiffIgnoreZeroSign(out0, ref);

					if (ulpDiff > maxUlpDiff)
					{
						m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(ref) << " with ULP threshold " << tcu::toHex(maxUlpDiff) << ", got diff " << tcu::toHex(ulpDiff);
						return false;
					}
				}
				else
				{
					if (out0 >= 1.0f)
					{
						m_failMsg << "Expected [" << compNdx << "] < 1.0";
						return false;
					}
				}
			}
		}

		return true;
	}
};

class FractCase : public CommonFunctionCase
{
public:
	FractCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "fract", shaderType)
	{
		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, precision)));
		m_spec.source = "out0 = fract(in0);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new FractCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class FrexpCaseInstance : public CommonFunctionTestInstance
{
public:
	FrexpCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd			(deStringHash(m_name) ^ 0x2790au);
		const glu::DataType		type		= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision	= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize	= glu::getDataTypeScalarSize(type);

		// Special cases
		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			((float*)values[0])[scalarSize*0 + compNdx] = 0.0f;
			((float*)values[0])[scalarSize*1 + compNdx] = -0.0f;
			((float*)values[0])[scalarSize*2 + compNdx] = 0.5f;
			((float*)values[0])[scalarSize*3 + compNdx] = -0.5f;
			((float*)values[0])[scalarSize*4 + compNdx] = 1.0f;
			((float*)values[0])[scalarSize*5 + compNdx] = -1.0f;
			((float*)values[0])[scalarSize*6 + compNdx] = 2.0f;
			((float*)values[0])[scalarSize*7 + compNdx] = -2.0f;
		}

		fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), (float*)values[0] + 8*scalarSize, (numValues-8)*scalarSize);

		// Make sure the values are representable in the target format
		for (int caseNdx = 0; caseNdx < numValues; ++caseNdx)
		{
			for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
			{
				float* const valuePtr = &((float*)values[0])[caseNdx * scalarSize + scalarNdx];

				*valuePtr = makeFloatRepresentable(*valuePtr, precision);
			}
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type						= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision					= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize					= glu::getDataTypeScalarSize(type);
		const bool				transitSupportsSignedZero	= (m_shaderType != glu::SHADERTYPE_FRAGMENT); // executor cannot reliably transit negative zero to fragment stage
		const bool				signedZero					= supportsSignedZero(precision) && transitSupportsSignedZero;

		const int				mantissaBits				= getMinMantissaBits(precision);
		const deUint32			maxUlpDiff					= getMaxUlpDiffFromBits(mantissaBits);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const float		in0			= ((const float*)inputs[0])[compNdx];
			const float		out0		= ((const float*)outputs[0])[compNdx];
			const int		out1		= ((const int*)outputs[1])[compNdx];

			float			refOut0;
			int				refOut1;

			frexp(in0, &refOut0, &refOut1);

			const deUint32	ulpDiff0	= signedZero ? getUlpDiff(out0, refOut0) : getUlpDiffIgnoreZeroSign(out0, refOut0);

			if (ulpDiff0 > maxUlpDiff || out1 != refOut1)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(refOut0) << ", " << refOut1 << " with ULP threshold "
						  << tcu::toHex(maxUlpDiff) << ", got ULP diff " << tcu::toHex(ulpDiff0);
				return false;
			}
		}

		return true;
	}
};

class FrexpCase : public CommonFunctionCase
{
public:
	FrexpCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "frexp", shaderType)
	{
		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	intType		= vecSize > 1 ? glu::getDataTypeIntVec(vecSize) : glu::TYPE_INT;

		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, glu::PRECISION_HIGHP)));
		m_spec.outputs.push_back(Symbol("out1", glu::VarType(intType, glu::PRECISION_HIGHP)));
		m_spec.source = "out0 = frexp(in0, out1);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new FrexpCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class LdexpCaseInstance : public CommonFunctionTestInstance
{
public:
	LdexpCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-1e3f,		1e3f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd					(deStringHash(m_name) ^ 0x2790au);
		const glu::DataType		type				= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision			= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize			= glu::getDataTypeScalarSize(type);
		int						valueNdx			= 0;

		{
			const float easySpecialCases[] = { 0.0f, -0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 2.0f, -2.0f };

			DE_ASSERT(valueNdx + DE_LENGTH_OF_ARRAY(easySpecialCases) <= numValues);
			for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(easySpecialCases); caseNdx++)
			{
				float	in0;
				int		in1;

				frexp(easySpecialCases[caseNdx], &in0, &in1);

				for (int compNdx = 0; compNdx < scalarSize; compNdx++)
				{
					((float*)values[0])[valueNdx*scalarSize + compNdx] = in0;
					((int*)values[1])[valueNdx*scalarSize + compNdx] = in1;
				}

				valueNdx += 1;
			}
		}

		{
			// \note lowp and mediump can not necessarily fit the values in hard cases, so we'll use only easy ones.
			const int numEasyRandomCases = precision == glu::PRECISION_HIGHP ? 50 : (numValues-valueNdx);

			DE_ASSERT(valueNdx + numEasyRandomCases <= numValues);
			for (int caseNdx = 0; caseNdx < numEasyRandomCases; caseNdx++)
			{
				for (int compNdx = 0; compNdx < scalarSize; compNdx++)
				{
					const float	in	= rnd.getFloat(ranges[precision].x(), ranges[precision].y());
					float		in0;
					int			in1;

					frexp(in, &in0, &in1);

					((float*)values[0])[valueNdx*scalarSize + compNdx] = in0;
					((int*)values[1])[valueNdx*scalarSize + compNdx] = in1;
				}

				valueNdx += 1;
			}
		}

		{
			const int numHardRandomCases = numValues-valueNdx;
			DE_ASSERT(numHardRandomCases >= 0 && valueNdx + numHardRandomCases <= numValues);

			for (int caseNdx = 0; caseNdx < numHardRandomCases; caseNdx++)
			{
				for (int compNdx = 0; compNdx < scalarSize; compNdx++)
				{
					const int		fpExp		= rnd.getInt(-126, 127);
					const int		sign		= rnd.getBool() ? -1 : +1;
					const deUint32	mantissa	= (1u<<23) | (rnd.getUint32() & ((1u<<23)-1));
					const int		in1			= rnd.getInt(de::max(-126, -126-fpExp), de::min(127, 127-fpExp));
					const float		in0			= tcu::Float32::construct(sign, fpExp, mantissa).asFloat();

					DE_ASSERT(de::inRange(in1, -126, 127)); // See Khronos bug 11180
					DE_ASSERT(de::inRange(in1+fpExp, -126, 127));

					const float		out			= ldexp(in0, in1);

					DE_ASSERT(!tcu::Float32(out).isInf() && !tcu::Float32(out).isDenorm());
					DE_UNREF(out);

					((float*)values[0])[valueNdx*scalarSize + compNdx] = in0;
					((int*)values[1])[valueNdx*scalarSize + compNdx] = in1;
				}

				valueNdx += 1;
			}
		}
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		const int				mantissaBits	= getMinMantissaBits(precision);
		const deUint32			maxUlpDiff		= getMaxUlpDiffFromBits(mantissaBits);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const float		in0			= ((const float*)inputs[0])[compNdx];
			const int		in1			= ((const int*)inputs[1])[compNdx];
			const float		out0		= ((const float*)outputs[0])[compNdx];
			const float		refOut0		= ldexp(in0, in1);
			const deUint32	ulpDiff		= getUlpDiffIgnoreZeroSign(out0, refOut0);

			const int		inExp		= tcu::Float32(in0).exponent();

			if (ulpDiff > maxUlpDiff)
			{
				m_failMsg << "Expected [" << compNdx << "] = " << HexFloat(refOut0) << ", (exp = " << inExp << ") with ULP threshold "
						  << tcu::toHex(maxUlpDiff) << ", got ULP diff " << tcu::toHex(ulpDiff);
				return false;
			}
		}

		return true;
	}
};

class LdexpCase : public CommonFunctionCase
{
public:
	LdexpCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "ldexp", shaderType)
	{
		const int			vecSize		= glu::getDataTypeScalarSize(baseType);
		const glu::DataType	intType		= vecSize > 1 ? glu::getDataTypeIntVec(vecSize) : glu::TYPE_INT;

		m_spec.inputs.push_back(Symbol("in0", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("in1", glu::VarType(intType, glu::PRECISION_HIGHP)));
		m_spec.outputs.push_back(Symbol("out0", glu::VarType(baseType, glu::PRECISION_HIGHP)));
		m_spec.source = "out0 = ldexp(in0, in1);";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new LdexpCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

class FmaCaseInstance : public CommonFunctionTestInstance
{
public:
	FmaCaseInstance (Context& context, glu::ShaderType shaderType, const ShaderSpec& spec, int numValues, const char* name)
		: CommonFunctionTestInstance	(context, shaderType, spec, numValues, name)
	{
	}

	void getInputValues (int numValues, void* const* values) const
	{
		const Vec2 ranges[] =
		{
			Vec2(-2.0f,		2.0f),	// lowp
			Vec2(-127.f,	127.f),	// mediump
			Vec2(-1e7f,		1e7f)	// highp
		};

		de::Random				rnd							(deStringHash(m_name) ^ 0xac23fu);
		const glu::DataType		type						= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision					= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize					= glu::getDataTypeScalarSize(type);
		const float				specialCases[][3]			=
		{
			// a		b		c
			{ 0.0f,		0.0f,	0.0f },
			{ 0.0f,		1.0f,	0.0f },
			{ 0.0f,		0.0f,	-1.0f },
			{ 1.0f,		1.0f,	0.0f },
			{ 1.0f,		1.0f,	1.0f },
			{ -1.0f,	1.0f,	0.0f },
			{ 1.0f,		-1.0f,	0.0f },
			{ -1.0f,	-1.0f,	0.0f },
			{ -0.0f,	1.0f,	0.0f },
			{ 1.0f,		-0.0f,	0.0f }
		};
		const int				numSpecialCases				= DE_LENGTH_OF_ARRAY(specialCases);

		// Special cases
		for (int caseNdx = 0; caseNdx < numSpecialCases; caseNdx++)
		{
			for (int inputNdx = 0; inputNdx < 3; inputNdx++)
			{
				for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
					((float*)values[inputNdx])[caseNdx*scalarSize + scalarNdx] = specialCases[caseNdx][inputNdx];
			}
		}

		// Random cases.
		{
			const int	numScalars	= (numValues-numSpecialCases)*scalarSize;
			const int	offs		= scalarSize*numSpecialCases;

			for (int inputNdx = 0; inputNdx < 3; inputNdx++)
				fillRandomScalars(rnd, ranges[precision].x(), ranges[precision].y(), (float*)values[inputNdx] + offs, numScalars);
		}

		// Make sure the values are representable in the target format
		for (int inputNdx = 0; inputNdx < 3; inputNdx++)
		{
			for (int caseNdx = 0; caseNdx < numValues; ++caseNdx)
			{
				for (int scalarNdx = 0; scalarNdx < scalarSize; scalarNdx++)
				{
					float* const valuePtr = &((float*)values[inputNdx])[caseNdx * scalarSize + scalarNdx];

					*valuePtr = makeFloatRepresentable(*valuePtr, precision);
				}
			}
		}
	}

	static tcu::Interval fma (glu::Precision precision, float a, float b, float c)
	{
		const tcu::FloatFormat formats[] =
		{
			//				 minExp		maxExp		mantissa	exact,		subnormals	infinities	NaN
			tcu::FloatFormat(0,			0,			7,			false,		tcu::YES,	tcu::MAYBE,	tcu::MAYBE),
			tcu::FloatFormat(-13,		13,			9,			false,		tcu::MAYBE,	tcu::MAYBE,	tcu::MAYBE),
			tcu::FloatFormat(-126,		127,		23,			true,		tcu::MAYBE, tcu::YES,	tcu::MAYBE)
		};
		const tcu::FloatFormat&	format	= de::getSizedArrayElement<glu::PRECISION_LAST>(formats, precision);
		const tcu::Interval		ia		= format.convert(a);
		const tcu::Interval		ib		= format.convert(b);
		const tcu::Interval		ic		= format.convert(c);
		tcu::Interval			prod0;
		tcu::Interval			prod1;
		tcu::Interval			prod2;
		tcu::Interval			prod3;
		tcu::Interval			prod;
		tcu::Interval			res;

		TCU_SET_INTERVAL(prod0, tmp, tmp = ia.lo() * ib.lo());
		TCU_SET_INTERVAL(prod1, tmp, tmp = ia.lo() * ib.hi());
		TCU_SET_INTERVAL(prod2, tmp, tmp = ia.hi() * ib.lo());
		TCU_SET_INTERVAL(prod3, tmp, tmp = ia.hi() * ib.hi());

		prod = format.convert(format.roundOut(prod0 | prod1 | prod2 | prod3, ia.isFinite() && ib.isFinite()));

		TCU_SET_INTERVAL_BOUNDS(res, tmp,
								tmp = prod.lo() + ic.lo(),
								tmp = prod.hi() + ic.hi());

		return format.convert(format.roundOut(res, prod.isFinite() && ic.isFinite()));
	}

	bool compare (const void* const* inputs, const void* const* outputs)
	{
		const glu::DataType		type			= m_spec.inputs[0].varType.getBasicType();
		const glu::Precision	precision		= m_spec.inputs[0].varType.getPrecision();
		const int				scalarSize		= glu::getDataTypeScalarSize(type);

		for (int compNdx = 0; compNdx < scalarSize; compNdx++)
		{
			const float			a			= ((const float*)inputs[0])[compNdx];
			const float			b			= ((const float*)inputs[1])[compNdx];
			const float			c			= ((const float*)inputs[2])[compNdx];
			const float			res			= ((const float*)outputs[0])[compNdx];
			const tcu::Interval	ref			= fma(precision, a, b, c);

			if (!ref.contains(res))
			{
				m_failMsg << "Expected [" << compNdx << "] = " << ref;
				return false;
			}
		}

		return true;
	}
};

class FmaCase : public CommonFunctionCase
{
public:
	FmaCase (tcu::TestContext& testCtx, glu::DataType baseType, glu::Precision precision, glu::ShaderType shaderType)
		: CommonFunctionCase	(testCtx, getCommonFuncCaseName(baseType, precision, shaderType).c_str(), "fma", shaderType)
	{
		m_spec.inputs.push_back(Symbol("a", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("b", glu::VarType(baseType, precision)));
		m_spec.inputs.push_back(Symbol("c", glu::VarType(baseType, precision)));
		m_spec.outputs.push_back(Symbol("res", glu::VarType(baseType, precision)));
		m_spec.source = "res = fma(a, b, c);";
		m_spec.globalDeclarations = "#extension GL_EXT_gpu_shader5 : require\n";
	}

	TestInstance* createInstance (Context& ctx) const
	{
		return new FmaCaseInstance(ctx, m_shaderType, m_spec, m_numValues, getName());
	}
};

} // anonymous

ShaderCommonFunctionTests::ShaderCommonFunctionTests (tcu::TestContext& testCtx)
	: tcu::TestCaseGroup	(testCtx, "common", "Common function tests")
{
}

ShaderCommonFunctionTests::~ShaderCommonFunctionTests (void)
{
}

void ShaderCommonFunctionTests::init (void)
{
	enum
	{
		VS = (1<<glu::SHADERTYPE_VERTEX),
		TC = (1<<glu::SHADERTYPE_TESSELLATION_CONTROL),
		TE = (1<<glu::SHADERTYPE_TESSELLATION_EVALUATION),
		GS = (1<<glu::SHADERTYPE_GEOMETRY),
		FS = (1<<glu::SHADERTYPE_FRAGMENT),
		CS = (1<<glu::SHADERTYPE_COMPUTE),

		ALL_SHADERS = VS|TC|TE|GS|FS|CS,
		NEW_SHADERS = TC|TE|GS|CS,
	};

	//																	Float?	Int?	Uint?	Shaders
	addFunctionCases<AbsCase>				(this,	"abs",				true,	true,	false,	ALL_SHADERS);
	addFunctionCases<SignCase>				(this,	"sign",				true,	true,	false,	ALL_SHADERS);
	addFunctionCases<FloorCase>				(this,	"floor",			true,	false,	false,	ALL_SHADERS);
	addFunctionCases<TruncCase>				(this,	"trunc",			true,	false,	false,	ALL_SHADERS);
	addFunctionCases<RoundCase>				(this,	"round",			true,	false,	false,	ALL_SHADERS);
	addFunctionCases<RoundEvenCase>			(this,	"roundeven",		true,	false,	false,	ALL_SHADERS);
	addFunctionCases<CeilCase>				(this,	"ceil",				true,	false,	false,	ALL_SHADERS);
	addFunctionCases<FractCase>				(this,	"fract",			true,	false,	false,	ALL_SHADERS);
	// mod
	addFunctionCases<ModfCase>				(this,	"modf",				true,	false,	false,	ALL_SHADERS);
	// min
	// max
	// clamp
	// mix
	// step
	// smoothstep
	addFunctionCases<IsnanCase>				(this,	"isnan",			true,	false,	false,	ALL_SHADERS);
	addFunctionCases<IsinfCase>				(this,	"isinf",			true,	false,	false,	ALL_SHADERS);
	addFunctionCases<FloatBitsToIntCase>	(this,	"floatbitstoint",	true,	false,	false,	ALL_SHADERS);
	addFunctionCases<FloatBitsToUintCase>	(this,	"floatbitstouint",	true,	false,	false,	ALL_SHADERS);

	addFunctionCases<FrexpCase>				(this,	"frexp",			true,	false,	false,	ALL_SHADERS);
	addFunctionCases<LdexpCase>				(this,	"ldexp",			true,	false,	false,	ALL_SHADERS);
	addFunctionCases<FmaCase>				(this,	"fma",				true,	false,	false,	ALL_SHADERS);

	// (u)intBitsToFloat()
	{
		const deUint32		shaderBits	= NEW_SHADERS;
		tcu::TestCaseGroup* intGroup	= new tcu::TestCaseGroup(m_testCtx, "intbitstofloat",	"intBitsToFloat() Tests");
		tcu::TestCaseGroup* uintGroup	= new tcu::TestCaseGroup(m_testCtx, "uintbitstofloat",	"uintBitsToFloat() Tests");

		addChild(intGroup);
		addChild(uintGroup);

		for (int vecSize = 1; vecSize < 4; vecSize++)
		{
			const glu::DataType		intType		= vecSize > 1 ? glu::getDataTypeIntVec(vecSize) : glu::TYPE_INT;
			const glu::DataType		uintType	= vecSize > 1 ? glu::getDataTypeUintVec(vecSize) : glu::TYPE_UINT;

			for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
			{
				if (shaderBits & (1<<shaderType))
				{
					intGroup->addChild(new BitsToFloatCase(getTestContext(), intType, glu::ShaderType(shaderType)));
					uintGroup->addChild(new BitsToFloatCase(getTestContext(), uintType, glu::ShaderType(shaderType)));
				}
			}
		}
	}
}

} // shaderexecutor
} // vkt
