/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Tessellation Limits Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationLimitsTests.hpp"
#include "vktTestCaseUtil.hpp"

#include "tcuTestLog.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"

#include "deUniquePtr.hpp"

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

enum TessellationLimits
{
	LIMIT_MAX_TESSELLATION_GENERATION_LEVEL,
	LIMIT_MAX_TESSELLATION_PATCH_SIZE,
    LIMIT_MAX_TESSELLATION_CONTROL_PER_VERTEX_INPUT_COMPONENTS,
    LIMIT_MAX_TESSELLATION_CONTROL_PER_VERTEX_OUTPUT_COMPONENTS,
    LIMIT_MAX_TESSELLATION_CONTROL_PER_PATCH_OUTPUT_COMPONENTS,
    LIMIT_MAX_TESSELLATION_CONTROL_TOTAL_OUTPUT_COMPONENTS,
    LIMIT_MAX_TESSELLATION_EVALUATION_INPUT_COMPONENTS,
    LIMIT_MAX_TESSELLATION_EVALUATION_OUTPUT_COMPONENTS,
};

struct LimitsCaseDefinition
{
	TessellationLimits	limitType;
	deUint32			minimum;		//!< Implementation must provide at least this value
};

tcu::TestStatus expectGreaterOrEqual(tcu::TestLog& log, const deUint32 expected, const deUint32 actual)
{
	log << tcu::TestLog::Message << "Expected: " << expected << ", got: " << actual << tcu::TestLog::EndMessage;

	if (actual >= expected)
		return tcu::TestStatus::pass("OK");
	else
		return tcu::TestStatus::fail("Value doesn't meet minimal spec requirements");
}

tcu::TestStatus deviceLimitsTestCase(Context& context, const LimitsCaseDefinition caseDef)
{
	const InstanceInterface&		vki			= context.getInstanceInterface();
	const VkPhysicalDevice			physDevice	= context.getPhysicalDevice();
	const VkPhysicalDeviceFeatures	features	= getPhysicalDeviceFeatures(vki, physDevice);

	if (!features.tessellationShader)
		throw tcu::NotSupportedError("Tessellation shader not supported");

	const VkPhysicalDeviceProperties properties = getPhysicalDeviceProperties(vki, physDevice);
	tcu::TestLog&					 log		= context.getTestContext().getLog();

	switch (caseDef.limitType)
	{
		case LIMIT_MAX_TESSELLATION_GENERATION_LEVEL:
			return expectGreaterOrEqual(log, caseDef.minimum, properties.limits.maxTessellationGenerationLevel);
		case LIMIT_MAX_TESSELLATION_PATCH_SIZE:
			return expectGreaterOrEqual(log, caseDef.minimum, properties.limits.maxTessellationPatchSize);
		case LIMIT_MAX_TESSELLATION_CONTROL_PER_VERTEX_INPUT_COMPONENTS:
			return expectGreaterOrEqual(log, caseDef.minimum, properties.limits.maxTessellationControlPerVertexInputComponents);
		case LIMIT_MAX_TESSELLATION_CONTROL_PER_VERTEX_OUTPUT_COMPONENTS:
			return expectGreaterOrEqual(log, caseDef.minimum, properties.limits.maxTessellationControlPerVertexOutputComponents);
		case LIMIT_MAX_TESSELLATION_CONTROL_PER_PATCH_OUTPUT_COMPONENTS:
			return expectGreaterOrEqual(log, caseDef.minimum, properties.limits.maxTessellationControlPerPatchOutputComponents);
		case LIMIT_MAX_TESSELLATION_CONTROL_TOTAL_OUTPUT_COMPONENTS:
			return expectGreaterOrEqual(log, caseDef.minimum, properties.limits.maxTessellationControlTotalOutputComponents);
		case LIMIT_MAX_TESSELLATION_EVALUATION_INPUT_COMPONENTS:
			return expectGreaterOrEqual(log, caseDef.minimum, properties.limits.maxTessellationEvaluationInputComponents);
		case LIMIT_MAX_TESSELLATION_EVALUATION_OUTPUT_COMPONENTS:
			return expectGreaterOrEqual(log, caseDef.minimum, properties.limits.maxTessellationEvaluationOutputComponents);
	}

	// Control should never get here.
	DE_FATAL("Internal test error");
	return tcu::TestStatus::fail("Test error");
}

} // anonymous

//! These tests correspond roughly to dEQP-GLES31.functional.tessellation.state_query.*
tcu::TestCaseGroup* createLimitsTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "limits", "Tessellation limits tests"));

	static const struct
	{
		std::string				caseName;
		LimitsCaseDefinition	caseDef;
	} cases[] =
	{
		{ "max_tessellation_generation_level",						{ LIMIT_MAX_TESSELLATION_GENERATION_LEVEL,						64   } },
		{ "max_tessellation_patch_size",							{ LIMIT_MAX_TESSELLATION_PATCH_SIZE,							32   } },
		{ "max_tessellation_control_per_vertex_input_components",	{ LIMIT_MAX_TESSELLATION_CONTROL_PER_VERTEX_INPUT_COMPONENTS,	64   } },
		{ "max_tessellation_control_per_vertex_output_components",	{ LIMIT_MAX_TESSELLATION_CONTROL_PER_VERTEX_OUTPUT_COMPONENTS,	64   } },
		{ "max_tessellation_control_per_patch_output_components",	{ LIMIT_MAX_TESSELLATION_CONTROL_PER_PATCH_OUTPUT_COMPONENTS,	120  } },
		{ "max_tessellation_control_total_output_components",		{ LIMIT_MAX_TESSELLATION_CONTROL_TOTAL_OUTPUT_COMPONENTS,		2048 } },
		{ "max_tessellation_evaluation_input_components",			{ LIMIT_MAX_TESSELLATION_EVALUATION_INPUT_COMPONENTS,			64   } },
		{ "max_tessellation_evaluation_output_components",			{ LIMIT_MAX_TESSELLATION_EVALUATION_OUTPUT_COMPONENTS,			64   } },
	};

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(cases); ++i)
		addFunctionCase<LimitsCaseDefinition>(group.get(), cases[i].caseName, "", deviceLimitsTestCase, cases[i].caseDef);

	return group.release();
}

} // tessellation
} // vkt
