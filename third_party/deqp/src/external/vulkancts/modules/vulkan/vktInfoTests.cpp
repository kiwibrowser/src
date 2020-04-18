/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief Build and Device Tests
 *//*--------------------------------------------------------------------*/

#include "vktInfoTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkPlatform.hpp"
#include "vkApiVersion.hpp"
#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuCommandLine.hpp"
#include "tcuPlatform.hpp"
#include "deStringUtil.hpp"

#include <iomanip>

namespace vkt
{

namespace
{

using tcu::TestLog;
using std::string;

std::string getOsName (int os)
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
			return de::toString(os);
	}
}

std::string getCompilerName (int compiler)
{
	switch (compiler)
	{
		case DE_COMPILER_VANILLA:	return "DE_COMPILER_VANILLA";
		case DE_COMPILER_MSC:		return "DE_COMPILER_MSC";
		case DE_COMPILER_GCC:		return "DE_COMPILER_GCC";
		case DE_COMPILER_CLANG:		return "DE_COMPILER_CLANG";
		default:
			return de::toString(compiler);
	}
}

std::string getCpuName (int cpu)
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
			return de::toString(cpu);
	}
}

std::string getEndiannessName (int endianness)
{
	switch (endianness)
	{
		case DE_BIG_ENDIAN:		return "DE_BIG_ENDIAN";
		case DE_LITTLE_ENDIAN:	return "DE_LITTLE_ENDIAN";
		default:
			return de::toString(endianness);
	}
}

tcu::TestStatus logBuildInfo (Context& context)
{
#if defined(DE_DEBUG)
	const bool	isDebug	= true;
#else
	const bool	isDebug	= false;
#endif

	context.getTestContext().getLog()
		<< TestLog::Message
		<< "DE_OS: " << getOsName(DE_OS) << "\n"
		<< "DE_CPU: " << getCpuName(DE_CPU) << "\n"
		<< "DE_PTR_SIZE: " << DE_PTR_SIZE << "\n"
		<< "DE_ENDIANNESS: " << getEndiannessName(DE_ENDIANNESS) << "\n"
		<< "DE_COMPILER: " << getCompilerName(DE_COMPILER) << "\n"
		<< "DE_DEBUG: " << (isDebug ? "true" : "false") << "\n"
		<< TestLog::EndMessage;

	return tcu::TestStatus::pass("Not validated");
}

tcu::TestStatus logDeviceInfo (Context& context)
{
	TestLog&								log			= context.getTestContext().getLog();
	const vk::VkPhysicalDeviceProperties&	properties	= context.getDeviceProperties();

	log << TestLog::Message
		<< "Using --deqp-vk-device-id="
		<< context.getTestContext().getCommandLine().getVKDeviceId()
		<< TestLog::EndMessage;

	log << TestLog::Message
		<< "apiVersion: " << vk::unpackVersion(properties.apiVersion) << "\n"
		<< "driverVersion: " << tcu::toHex(properties.driverVersion) << "\n"
		<< "deviceName: " << (const char*)properties.deviceName << "\n"
		<< "vendorID: " << tcu::toHex(properties.vendorID) << "\n"
		<< "deviceID: " << tcu::toHex(properties.deviceID) << "\n"
		<< TestLog::EndMessage;

	return tcu::TestStatus::pass("Not validated");
}

tcu::TestStatus logPlatformInfo (Context& context)
{
	std::ostringstream details;

	context.getTestContext().getPlatform().getVulkanPlatform().describePlatform(details);

	context.getTestContext().getLog()
		<< TestLog::Message
		<< details.str()
		<< TestLog::EndMessage;

	return tcu::TestStatus::pass("Not validated");
}

template<typename SizeType>
struct PrettySize
{
	SizeType	value;
	int			precision;

	PrettySize (SizeType value_, int precision_)
		: value		(value_)
		, precision	(precision_)
	{}
};

struct SizeUnit
{
	const char*		name;
	deUint64		value;
};

const SizeUnit* getBestSizeUnit (deUint64 value)
{
	static const SizeUnit s_units[]		=
	{
		// \note Must be ordered from largest to smallest
		{ "TiB",	1ull<<40ull		},
		{ "MiB",	1ull<<20ull		},
		{ "GiB",	1ull<<30ull		},
		{ "KiB",	1ull<<10ull		},
	};
	static const SizeUnit s_defaultUnit	= { "B", 1u };

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_units); ++ndx)
	{
		if (value >= s_units[ndx].value)
			return &s_units[ndx];
	}

	return &s_defaultUnit;
}

template<typename SizeType>
std::ostream& operator<< (std::ostream& str, const PrettySize<SizeType>& size)
{
	const SizeUnit*		unit = getBestSizeUnit(deUint64(size.value));
	std::ostringstream	tmp;

	tmp << std::fixed << std::setprecision(size.precision)
		<< (double(size.value) / double(unit->value))
		<< " " << unit->name;

	return str << tmp.str();
}

template<typename SizeType>
PrettySize<SizeType> prettySize (SizeType value, int precision = 2)
{
	return PrettySize<SizeType>(value, precision);
}

tcu::TestStatus logPlatformMemoryLimits (Context& context)
{
	TestLog&					log			= context.getTestContext().getLog();
	vk::PlatformMemoryLimits	limits;

	context.getTestContext().getPlatform().getVulkanPlatform().getMemoryLimits(limits);

	log << TestLog::Message << "totalSystemMemory = " << prettySize(limits.totalSystemMemory) << " (" << limits.totalSystemMemory << ")\n"
							<< "totalDeviceLocalMemory = " << prettySize(limits.totalDeviceLocalMemory) << " (" << limits.totalDeviceLocalMemory << ")\n"
							<< "deviceMemoryAllocationGranularity = " << limits.deviceMemoryAllocationGranularity << "\n"
							<< "devicePageSize = " << limits.devicePageSize << "\n"
							<< "devicePageTableEntrySize = " << limits.devicePageTableEntrySize << "\n"
							<< "devicePageTableHierarchyLevels = " << limits.devicePageTableHierarchyLevels << "\n"
		<< TestLog::EndMessage;

	TCU_CHECK(limits.totalSystemMemory > 0);
	TCU_CHECK(limits.deviceMemoryAllocationGranularity > 0);
	TCU_CHECK(deIsPowerOfTwo64(limits.devicePageSize));

	return tcu::TestStatus::pass("Pass");
}

} // anonymous

void createInfoTests (tcu::TestCaseGroup* testGroup)
{
	addFunctionCase(testGroup, "build",			"Build Info",				logBuildInfo);
	addFunctionCase(testGroup, "device",		"Device Info",				logDeviceInfo);
	addFunctionCase(testGroup, "platform",		"Platform Info",			logPlatformInfo);
	addFunctionCase(testGroup, "memory_limits",	"Platform Memory Limits",	logPlatformMemoryLimits);
}

} // vkt
