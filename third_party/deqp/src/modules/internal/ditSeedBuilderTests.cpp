/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
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
 * \brief Seed builder tests.
 *//*--------------------------------------------------------------------*/

#include "ditSeedBuilderTests.hpp"

#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include "tcuSeedBuilder.hpp"

using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;
using tcu::TestLog;

namespace dit
{
namespace
{

template<class T>
class SeedBuilderTest : public tcu::TestCase
{
public:
	SeedBuilderTest (tcu::TestContext& testCtx, const T& value, deUint32 seed, const char* name, const char* description)
		: tcu::TestCase	(testCtx, name, description)
		, m_value		(value)
		, m_seed		(seed)
	{
	}

	IterateResult iterate (void)
	{
		TestLog&			log		= m_testCtx.getLog();
		tcu::SeedBuilder	builder;

		builder << m_value;

		log << TestLog::Message << "Value: " << m_value << TestLog::EndMessage;
		log << TestLog::Message << "Expected seed: " << m_seed << TestLog::EndMessage;
		log << TestLog::Message << "Got seed: " << builder.get() << TestLog::EndMessage;

		if (builder.get() != m_seed)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid seed");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

private:
	T			m_value;
	deUint32	m_seed;
};

class SeedBuilderMultipleValuesTest : public tcu::TestCase
{
public:
	SeedBuilderMultipleValuesTest (tcu::TestContext& testCtx)
		: tcu::TestCase(testCtx, "multiple_values", "Test that multiple values all change the seed.")
	{
	}

	IterateResult iterate (void)
	{
		TestLog&			log			= m_testCtx.getLog();
		const deUint32		a			= 77740203u;
		const deUint32		b			= 3830824200u;

		tcu::SeedBuilder	builderA;
		tcu::SeedBuilder	builderB;
		tcu::SeedBuilder	builderAB;

		builderA << a;
		builderB << b;

		builderAB << a << b;

		log << TestLog::Message << "Value a: " << a << ", Seed a: " << builderA.get() << TestLog::EndMessage;
		log << TestLog::Message << "Value b: " << b << ", Seed b: " << builderB.get() << TestLog::EndMessage;
		log << TestLog::Message << "Seed ab: " << builderAB.get() << TestLog::EndMessage;

		if (builderA.get() == builderAB.get())
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Seed seems to only depends on first value.");
		else if (builderB.get() == builderAB.get())
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Seed seems to only depends on second value.");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

class SeedBuilderTests : public tcu::TestCaseGroup
{
public:
	SeedBuilderTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "seed_builder", "Seed builder tests.")
	{
	}

	void init (void)
	{
		addChild(new SeedBuilderTest<bool>(m_testCtx, true,  132088003u, "bool_true", "Seed from boolean true."));
		addChild(new SeedBuilderTest<bool>(m_testCtx, false,  50600761u, "bool_false", "Seed from boolean false."));

		addChild(new SeedBuilderTest<deInt8>(m_testCtx,  0,  62533730u, "int8_zero", "Seed from int8 zero."));
		addChild(new SeedBuilderTest<deInt8>(m_testCtx,  1,  93914869u, "int8_one", "Seed from int8 one."));
		addChild(new SeedBuilderTest<deInt8>(m_testCtx, -1, 115002165u, "int8_minus_one", "Seed from int8 minus one."));

		addChild(new SeedBuilderTest<deInt16>(m_testCtx,  0, 133071403u, "int16_zero", "Seed from int16 zero."));
		addChild(new SeedBuilderTest<deInt16>(m_testCtx,  1,  57421642u, "int16_one", "Seed from int16 one."));
		addChild(new SeedBuilderTest<deInt16>(m_testCtx, -1,  74389771u, "int16_minus_one", "Seed from int16 minus one."));

		addChild(new SeedBuilderTest<deInt32>(m_testCtx,  0, 75951701u, "int32_zero", "Seed from int32 zero."));
		addChild(new SeedBuilderTest<deInt32>(m_testCtx,  1, 95780822u, "int32_one", "Seed from int32 one."));
		addChild(new SeedBuilderTest<deInt32>(m_testCtx, -1, 73949483u, "int32_minus_one", "Seed from int32 minus one."));

		addChild(new SeedBuilderTest<deUint8>(m_testCtx, 0,		  3623298u, "uint8_zero", "Seed from uint8 zero."));
		addChild(new SeedBuilderTest<deUint8>(m_testCtx, 1,		102006549u, "uint8_one", "Seed from uint8 one."));
		addChild(new SeedBuilderTest<deUint8>(m_testCtx, 255,	 89633493u, "uint8_max", "Seed from uint8 max."));

		addChild(new SeedBuilderTest<deUint16>(m_testCtx, 0,		 78413740u, "uint16_zero", "Seed from uint16 zero."));
		addChild(new SeedBuilderTest<deUint16>(m_testCtx, 1,		  3068621u, "uint16_one", "Seed from uint16 one."));
		addChild(new SeedBuilderTest<deUint16>(m_testCtx, 65535,	120448140u, "uint16_max", "Seed from uint16 max."));

		addChild(new SeedBuilderTest<deUint32>(m_testCtx, 0u,			41006057u, "uint32_zero", "Seed from uint32 zero."));
		addChild(new SeedBuilderTest<deUint32>(m_testCtx, 1u,			54665834u, "uint32_one", "Seed from uint32 one."));
		addChild(new SeedBuilderTest<deUint32>(m_testCtx, 4294967295u,	43990167u, "uint32_max", "Seed from uint32 max."));

		addChild(new SeedBuilderTest<float>(m_testCtx, 0.0f,	 41165361u, "float_zero", "Seed from float zero."));
		addChild(new SeedBuilderTest<float>(m_testCtx, -0.0f,	112541574u, "float_negative_zero", "Seed from float negative zero."));
		addChild(new SeedBuilderTest<float>(m_testCtx, 1.0f,	 44355905u, "float_one", "Seed from float one."));
		addChild(new SeedBuilderTest<float>(m_testCtx, -1.0f,	107334902u, "float_negative_one", "Seed from float negative one."));

		addChild(new SeedBuilderTest<double>(m_testCtx, 0.0,	133470681u, "double_zero", "Seed from double zero."));
		addChild(new SeedBuilderTest<double>(m_testCtx, -0.0,	 53838958u, "double_negative_zero", "Seed from double negative zero."));
		addChild(new SeedBuilderTest<double>(m_testCtx, 1.0,	 16975104u, "double_one", "Seed from double one."));
		addChild(new SeedBuilderTest<double>(m_testCtx, -1.0,	 96606391u, "double_negative_one", "Seed from double negative one."));

		addChild(new SeedBuilderTest<IVec2>(m_testCtx, IVec2(0),	 1111532u, "ivec2_zero", "Seed from zero vector."));
		addChild(new SeedBuilderTest<IVec3>(m_testCtx, IVec3(0),	22277704u, "ivec3_zero", "Seed from zero vector."));
		addChild(new SeedBuilderTest<IVec4>(m_testCtx, IVec4(0),	73989201u, "ivec4_zero", "Seed from zero vector."));

		addChild(new SeedBuilderTest<IVec2>(m_testCtx, IVec2(1),	 12819708u, "ivec2_one", "Seed from one vector."));
		addChild(new SeedBuilderTest<IVec3>(m_testCtx, IVec3(1),	134047100u, "ivec3_one", "Seed from one vector."));
		addChild(new SeedBuilderTest<IVec4>(m_testCtx, IVec4(1),	 90609878u, "ivec4_one", "Seed from one vector."));

		addChild(new SeedBuilderTest<IVec4>(m_testCtx, IVec4(1, 2, 3, 4),	 6202236u, "ivec4_1_2_3_4", "Seed from (1, 2, 3, 4) vector."));
		addChild(new SeedBuilderTest<IVec4>(m_testCtx, IVec4(4, 3, 2, 1),	26964618u, "ivec4_4_3_2_1", "Seed from (4, 3, 2, 1) vector."));

		addChild(new SeedBuilderMultipleValuesTest(m_testCtx));
	}
};

} // anonymous

tcu::TestCaseGroup* createSeedBuilderTests (tcu::TestContext& testCtx)
{
	return new SeedBuilderTests(testCtx);
}

} // dit
