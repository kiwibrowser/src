/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 */ /*!
 * \file
 * \brief Test config list case.
 */ /*-------------------------------------------------------------------*/

#include "glcConfigListCase.hpp"
#include "glcConfigList.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

using tcu::TestLog;
using std::vector;

static const char* getConfigTypeName(ConfigType type)
{
	switch (type)
	{
	case CONFIGTYPE_DEFAULT:
		return "default";
	case CONFIGTYPE_EGL:
		return "EGL";
	case CONFIGTYPE_WGL:
		return "WGL";
	default:
		throw tcu::Exception("Unknown config type");
	}
}

static const char* getExcludeReasonName(ExcludeReason reason)
{
	switch (reason)
	{
	case EXCLUDEREASON_NOT_COMPATIBLE:
		return "Not compatible";
	case EXCLUDEREASON_NOT_CONFORMANT:
		return "Not conformant";
	case EXCLUDEREASON_MSAA:
		return "Multisampled: Not testable";
	case EXCLUDEREASON_FLOAT:
		return "Float configs: Not testable";
	case EXCLUDEREASON_YUV:
		return "YUV: Not testable";
	default:
		throw tcu::Exception("Unknown exclude reason");
	}
}

static const char* getSurfaceTypeName(tcu::SurfaceType type)
{
	switch (type)
	{
	case tcu::SURFACETYPE_WINDOW:
		return "window";
	case tcu::SURFACETYPE_OFFSCREEN_NATIVE:
		return "pixmap";
	case tcu::SURFACETYPE_OFFSCREEN_GENERIC:
		return "pbuffer";
	default:
		throw tcu::Exception("Unknown surface type");
	}
}

struct SurfaceBitsFmt
{
	deUint32 bits;

	SurfaceBitsFmt(deUint32 bits_) : bits(bits_)
	{
	}
};

std::ostream& operator<<(std::ostream& str, const SurfaceBitsFmt& bits)
{
	static const tcu::SurfaceType s_types[] = { tcu::SURFACETYPE_WINDOW, tcu::SURFACETYPE_OFFSCREEN_NATIVE,
												tcu::SURFACETYPE_OFFSCREEN_GENERIC };

	bool isFirst = true;
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_types); ndx++)
	{
		if (bits.bits & (1 << s_types[ndx]))
		{
			if (!isFirst)
				str << ", ";
			str << getSurfaceTypeName(s_types[ndx]);
			isFirst = false;
		}
	}

	return str;
}

ConfigListCase::ConfigListCase(tcu::TestContext& testCtx, const char* name, const char* description, glu::ApiType type)
	: TestCase(testCtx, name, description), m_ctxType(type)
{
}

ConfigListCase::~ConfigListCase(void)
{
}

ConfigListCase::IterateResult ConfigListCase::iterate(void)
{
	TestLog&   log = m_testCtx.getLog();
	ConfigList configs;

	getDefaultConfigList(m_testCtx.getPlatform(), m_ctxType, configs);

	// Valid configs.
	{
		tcu::ScopedLogSection configSection(log, "Configs", "Configs");

		for (vector<Config>::const_iterator cfgIter = configs.configs.begin(); cfgIter != configs.configs.end();
			 cfgIter++)
			log << TestLog::Message << getConfigTypeName(cfgIter->type) << "(" << cfgIter->id
				<< "): " << SurfaceBitsFmt(cfgIter->surfaceTypes) << TestLog::EndMessage;
	}

	// Excluded configs.
	{
		tcu::ScopedLogSection excludedSection(log, "ExcludedConfigs", "Excluded configs");

		for (vector<ExcludedConfig>::const_iterator cfgIter = configs.excludedConfigs.begin();
			 cfgIter != configs.excludedConfigs.end(); cfgIter++)
			log << TestLog::Message << getConfigTypeName(cfgIter->type) << "(" << cfgIter->id
				<< "): " << getExcludeReasonName(cfgIter->reason) << TestLog::EndMessage;
	}

	// Totals.
	int numValid		 = (int)configs.configs.size();
	int numNotCompatible = 0;
	int numNotConformant = 0;

	for (vector<ExcludedConfig>::const_iterator cfgIter = configs.excludedConfigs.begin();
		 cfgIter != configs.excludedConfigs.end(); cfgIter++)
		(cfgIter->reason == EXCLUDEREASON_NOT_CONFORMANT ? numNotConformant : numNotCompatible) += 1;

	log << TestLog::Message << numValid << " valid, " << numNotConformant << " non-conformant and " << numNotCompatible
		<< " non-compatible configs" << TestLog::EndMessage;

	bool configCountOk = numValid >= numNotConformant;
	m_testCtx.setTestResult(configCountOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
							configCountOk ? "Pass" : "Too many non-conformant configs");

	return STOP;
}

} // glcts
