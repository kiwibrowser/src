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
 * \brief Build information tests.
 *//*--------------------------------------------------------------------*/

#include "ditBuildInfoTests.hpp"
#include "tcuTestLog.hpp"
#include "deStringUtil.hpp"

using tcu::TestLog;

namespace dit
{

static const char* getOsName (int os)
{
	switch (os)
	{
		case DE_OS_VANILLA:		return "DE_OS_VANILLA";
		case DE_OS_WIN32:		return "DE_OS_WIN32";
		case DE_OS_UNIX:		return "DE_OS_UNIX";
		case DE_OS_WINCE:		return "DE_OS_WINCE";
		case DE_OS_OSX:			return "DE_OS_OSX";
		case DE_OS_ANDROID:		return "DE_OS_ANDROID";
		case DE_OS_SYMBIAN:		return "DE_OS_SYMBIAN";
		case DE_OS_IOS:			return "DE_OS_IOS";
		default:
			return DE_NULL;
	}
}

static const char* getCompilerName (int compiler)
{
	switch (compiler)
	{
		case DE_COMPILER_VANILLA:	return "DE_COMPILER_VANILLA";
		case DE_COMPILER_MSC:		return "DE_COMPILER_MSC";
		case DE_COMPILER_GCC:		return "DE_COMPILER_GCC";
		case DE_COMPILER_CLANG:		return "DE_COMPILER_CLANG";
		default:
			return DE_NULL;
	}
}

static const char* getCpuName (int cpu)
{
	switch (cpu)
	{
		case DE_CPU_VANILLA:	return "DE_CPU_VANILLA";
		case DE_CPU_ARM:		return "DE_CPU_ARM";
		case DE_CPU_X86:		return "DE_CPU_X86";
		case DE_CPU_X86_64:		return "DE_CPU_X86_64";
		case DE_CPU_ARM_64:		return "DE_CPU_ARM_64";
		case DE_CPU_MIPS:		return "DE_CPU_MIPS";
		case DE_CPU_MIPS_64:	return "DE_CPU_MIPS_64";
		default:
			return DE_NULL;
	}
}

static const char* getEndiannessName (int endianness)
{
	switch (endianness)
	{
		case DE_BIG_ENDIAN:		return "DE_BIG_ENDIAN";
		case DE_LITTLE_ENDIAN:	return "DE_LITTLE_ENDIAN";
		default:
			return DE_NULL;
	}
}

class BuildInfoStringCase : public tcu::TestCase
{
public:
	BuildInfoStringCase (tcu::TestContext& testCtx, const char* name, const char* valueName, const char* value)
		: tcu::TestCase	(testCtx, name, valueName)
		, m_valueName	(valueName)
		, m_value		(value)
	{
	}

	IterateResult iterate (void)
	{
		m_testCtx.getLog() << TestLog::Message << m_valueName << " = " << m_value << TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	std::string	m_valueName;
	std::string	m_value;
};

class BuildEnumCase : public tcu::TestCase
{
public:
	typedef const char* (*GetStringFunc) (int value);

	BuildEnumCase (tcu::TestContext& testCtx, const char* name, const char* varName, int value, GetStringFunc getString)
		: tcu::TestCase	(testCtx, name, varName)
		, m_varName		(varName)
		, m_value		(value)
		, m_getString	(getString)
	{
	}

	IterateResult iterate (void)
	{
		const char*	valueName	= m_getString(m_value);
		const bool	isOk		= valueName != DE_NULL;
		std::string	logValue	= valueName ? std::string(valueName) : de::toString(m_value);

		m_testCtx.getLog() << TestLog::Message << m_varName << " = " << logValue << TestLog::EndMessage;

		m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
								isOk ? "Pass"				: "No enum name found");
		return STOP;
	}

private:
	std::string		m_varName;
	int				m_value;
	GetStringFunc	m_getString;
};

class EndiannessConsistencyCase : public tcu::TestCase
{
public:
	EndiannessConsistencyCase (tcu::TestContext& context, const char* name, const char* description)
		: tcu::TestCase(context, name, description)
	{
	}

	IterateResult iterate (void)
	{
		const deUint16	multiByte	= (deUint16)0x0102;

#if DE_ENDIANNESS == DE_BIG_ENDIAN
		const bool		isOk		= *((const deUint8*)&multiByte) == (deUint8)0x01;
#elif DE_ENDIANNESS == DE_LITTLE_ENDIAN
		const bool		isOk		= *((const deUint8*)&multiByte) == (deUint8)0x02;
#endif

		m_testCtx.getLog()
			<< TestLog::Message
			<< "Verifying DE_ENDIANNESS matches actual behavior"
			<< TestLog::EndMessage;

		m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
								isOk ? "Pass"				: "Configured endianness inconsistent");
		return STOP;
	}
};

BuildInfoTests::BuildInfoTests (tcu::TestContext& testCtx)
	: tcu::TestCaseGroup(testCtx, "build_info", "Build Info Tests")
{
}

BuildInfoTests::~BuildInfoTests (void)
{
}

void BuildInfoTests::init (void)
{
#if defined(DE_DEBUG)
	const bool isDebug = true;
#else
	const bool isDebug = false;
#endif

	addChild(new BuildInfoStringCase		(m_testCtx, "de_debug",					"DE_DEBUG",			isDebug ? "1" : "not defined"));
	addChild(new BuildEnumCase				(m_testCtx, "de_os",					"DE_OS",			DE_OS,									getOsName));
	addChild(new BuildEnumCase				(m_testCtx, "de_cpu",					"DE_CPU",			DE_CPU,									getCpuName));
	addChild(new BuildEnumCase				(m_testCtx, "de_compiler",				"DE_COMPILER",		DE_COMPILER,							getCompilerName));
	addChild(new BuildInfoStringCase		(m_testCtx, "de_ptr_size",				"DE_PTR_SIZE",		de::toString(DE_PTR_SIZE).c_str()));
	addChild(new BuildEnumCase				(m_testCtx, "de_endianness",			"DE_ENDIANNESS",	DE_ENDIANNESS,							getEndiannessName));
	addChild(new EndiannessConsistencyCase	(m_testCtx, "de_endianness_consistent", "DE_ENDIANNESS"));
}

} // dit
