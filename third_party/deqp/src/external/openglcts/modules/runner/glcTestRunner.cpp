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
 * \brief CTS runner.
 */ /*-------------------------------------------------------------------*/

#include "glcTestRunner.hpp"
#include "deFilePath.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "glcConfigList.hpp"
#include "qpXmlWriter.h"
#include "tcuApp.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"
#include "tcuTestSessionExecutor.hpp"

#include <iterator>

namespace glcts
{

using std::vector;
using std::string;

// RunSession

class RunSession
{
public:
	RunSession(tcu::Platform& platform, tcu::Archive& archive, const int numArgs, const char* const* args)
		: m_cmdLine(numArgs, args)
		, m_log(m_cmdLine.getLogFileName(), m_cmdLine.getLogFlags())
		, m_app(platform, archive, m_log, m_cmdLine)
	{
	}

	inline bool iterate(void)
	{
		return m_app.iterate();
	}

	inline const tcu::TestRunStatus& getResult(void) const
	{
		return m_app.getResult();
	}

private:
	tcu::CommandLine m_cmdLine;
	tcu::TestLog	 m_log;
	tcu::App		 m_app;
};

static void appendConfigArgs(const Config& config, std::vector<std::string>& args, const char* fboConfig)
{
	if (fboConfig != NULL)
	{
		args.push_back(string("--deqp-gl-config-name=") + fboConfig);
		args.push_back("--deqp-surface-type=fbo");
	}

	if (config.type != CONFIGTYPE_DEFAULT)
	{
		// \todo [2013-05-06 pyry] Test all surface types for some configs?
		if (fboConfig == NULL)
		{
			if (config.surfaceTypes & SURFACETYPE_WINDOW)
				args.push_back("--deqp-surface-type=window");
			else if (config.surfaceTypes & SURFACETYPE_PBUFFER)
				args.push_back("--deqp-surface-type=pbuffer");
			else if (config.surfaceTypes & SURFACETYPE_PIXMAP)
				args.push_back("--deqp-surface-type=pixmap");
		}

		args.push_back(string("--deqp-gl-config-id=") + de::toString(config.id));

		if (config.type == CONFIGTYPE_EGL)
			args.push_back("--deqp-gl-context-type=egl");
		else if (config.type == CONFIGTYPE_WGL)
			args.push_back("--deqp-gl-context-type=wgl");
	}
}

typedef struct configInfo
{
	deInt32 redBits;
	deInt32 greenBits;
	deInt32 blueBits;
	deInt32 alphaBits;
	deInt32 depthBits;
	deInt32 stencilBits;
	deInt32 samples;
} configInfo;

static configInfo parseConfigBitsFromName(const char* configName)
{
	configInfo cfgInfo;
	static const struct
	{
		const char* name;
		int			redBits;
		int			greenBits;
		int			blueBits;
		int			alphaBits;
	} colorCfgs[] = {
		{ "rgba8888", 8, 8, 8, 8 }, { "rgb565", 5, 6, 5, 0 },
	};
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(colorCfgs); ndx++)
	{
		if (!strncmp(configName, colorCfgs[ndx].name, strlen(colorCfgs[ndx].name)))
		{
			cfgInfo.redBits   = colorCfgs[ndx].redBits;
			cfgInfo.greenBits = colorCfgs[ndx].greenBits;
			cfgInfo.blueBits  = colorCfgs[ndx].blueBits;
			cfgInfo.alphaBits = colorCfgs[ndx].alphaBits;

			configName += strlen(colorCfgs[ndx].name);
			break;
		}
	}

	static const struct
	{
		const char* name;
		int			depthBits;
	} depthCfgs[] = {
		{ "d0", 0 }, { "d24", 24 },
	};
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(depthCfgs); ndx++)
	{
		if (!strncmp(configName, depthCfgs[ndx].name, strlen(depthCfgs[ndx].name)))
		{
			cfgInfo.depthBits = depthCfgs[ndx].depthBits;

			configName += strlen(depthCfgs[ndx].name);
			break;
		}
	}

	static const struct
	{
		const char* name;
		int			stencilBits;
	} stencilCfgs[] = {
		{ "s0", 0 }, { "s8", 8 },
	};
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(stencilCfgs); ndx++)
	{
		if (!strncmp(configName, stencilCfgs[ndx].name, strlen(stencilCfgs[ndx].name)))
		{
			cfgInfo.stencilBits = stencilCfgs[ndx].stencilBits;

			configName += strlen(stencilCfgs[ndx].name);
			break;
		}
	}

	static const struct
	{
		const char* name;
		int			samples;
	} multiSampleCfgs[] = {
		{ "ms0", 0 }, { "ms4", 4 },
	};
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(multiSampleCfgs); ndx++)
	{
		if (!strncmp(configName, multiSampleCfgs[ndx].name, strlen(multiSampleCfgs[ndx].name)))
		{
			cfgInfo.samples = multiSampleCfgs[ndx].samples;

			configName += strlen(multiSampleCfgs[ndx].name);
			break;
		}
	}

	return cfgInfo;
}

static const char* getApiName(glu::ApiType apiType)
{
	if (apiType == glu::ApiType::es(2, 0))
		return "gles2";
	else if (apiType == glu::ApiType::es(3, 0))
		return "gles3";
	else if (apiType == glu::ApiType::es(3, 1))
		return "gles31";
	else if (apiType == glu::ApiType::es(3, 2))
		return "gles32";
	else if (apiType == glu::ApiType::core(3, 0))
		return "gl30";
	else if (apiType == glu::ApiType::core(3, 1))
		return "gl31";
	else if (apiType == glu::ApiType::core(3, 2))
		return "gl32";
	else if (apiType == glu::ApiType::core(3, 3))
		return "gl33";
	else if (apiType == glu::ApiType::core(4, 0))
		return "gl40";
	else if (apiType == glu::ApiType::core(4, 1))
		return "gl41";
	else if (apiType == glu::ApiType::core(4, 2))
		return "gl42";
	else if (apiType == glu::ApiType::core(4, 3))
		return "gl43";
	else if (apiType == glu::ApiType::core(4, 4))
		return "gl44";
	else if (apiType == glu::ApiType::core(4, 5))
		return "gl45";
	else if (apiType == glu::ApiType::core(4, 6))
		return "gl46";
	else
		throw std::runtime_error("Unknown context type");
}

static const string getCaseListFileOption(const char* mustpassDir, const char* apiName, const char* mustpassName)
{
#if DE_OS == DE_OS_ANDROID
	const string case_list_option = "--deqp-caselist-resource=";
#else
	const string case_list_option = "--deqp-caselist-file=";
#endif
	return case_list_option + mustpassDir + apiName + "-" + mustpassName + ".txt";
}

static const string getLogFileName(const char* apiName, const char* configName, const int iterId, const int runId,
								   const int width, const int height, const int seed)
{
	string res = string("config-") + apiName + "-" + configName + "-cfg-" + de::toString(iterId) + "-run-" +
				 de::toString(runId) + "-width-" + de::toString(width) + "-height-" + de::toString(height);
	if (seed != -1)
	{
		res += "-seed-" + de::toString(seed);
	}
	res += ".qpa";

	return res;
}

static void getBaseOptions(std::vector<std::string>& args, const char* mustpassDir, const char* apiName,
						   const char* configName, const char* screenRotation, int width, int height)
{
	args.push_back(getCaseListFileOption(mustpassDir, apiName, configName));
	args.push_back(string("--deqp-screen-rotation=") + screenRotation);
	args.push_back(string("--deqp-surface-width=") + de::toString(width));
	args.push_back(string("--deqp-surface-height=") + de::toString(height));
	args.push_back("--deqp-watchdog=disable");
}

static bool isGLConfigCompatible(configInfo cfgInfo, const AOSPConfig& config)
{
	return cfgInfo.redBits == config.redBits && cfgInfo.greenBits == config.greenBits &&
		   cfgInfo.blueBits == config.blueBits && cfgInfo.alphaBits == config.alphaBits &&
		   cfgInfo.depthBits == config.depthBits && cfgInfo.stencilBits == config.stencilBits &&
		   cfgInfo.samples == config.samples;
}

static void getTestRunsForAOSPEGL(vector<TestRunParams>& runs, const ConfigList& configs)
{
#include "glcAospMustpassEgl.hpp"

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(aosp_mustpass_egl_first_cfg); ++i)
	{
		configInfo cfgInfo = parseConfigBitsFromName(aosp_mustpass_egl_first_cfg[i].glConfigName);

		vector<AOSPConfig>::const_iterator cfgIter;
		for (cfgIter = configs.aospConfigs.begin(); cfgIter != configs.aospConfigs.end(); ++cfgIter)
		{
			// find first compatible config
			if ((*cfgIter).type == CONFIGTYPE_EGL && isGLConfigCompatible(cfgInfo, *cfgIter))
			{
				break;
			}
		}

		if (cfgIter == configs.aospConfigs.end())
		{
			// No suitable configuration found. Skipping EGL tests
			continue;
		}

		const char* apiName = "egl";

		const int width   = aosp_mustpass_egl_first_cfg[i].surfaceWidth;
		const int height  = aosp_mustpass_egl_first_cfg[i].surfaceHeight;

		TestRunParams params;
		params.logFilename =
			getLogFileName(apiName, aosp_mustpass_egl_first_cfg[i].configName, 1, i, width, height, -1);
		getBaseOptions(params.args, mustpassDir, apiName, aosp_mustpass_egl_first_cfg[i].configName,
					   aosp_mustpass_egl_first_cfg[i].screenRotation, width, height);

		params.args.push_back(string("--deqp-gl-config-name=") + string(aosp_mustpass_egl_first_cfg[i].glConfigName));

		runs.push_back(params);
	}
}

static void getTestRunsForAOSPES(vector<TestRunParams>& runs, const ConfigList& configs, const glu::ApiType apiType)
{
#include "glcAospMustpassEs.hpp"

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(aosp_mustpass_es_first_cfg); ++i)
	{
		if (!glu::contextSupports(glu::ContextType(apiType), aosp_mustpass_es_first_cfg[i].apiType))
			continue;

		configInfo cfgInfo = parseConfigBitsFromName(aosp_mustpass_es_first_cfg[i].glConfigName);

		vector<AOSPConfig>::const_iterator cfgIter;
		for (cfgIter = configs.aospConfigs.begin(); cfgIter != configs.aospConfigs.end(); ++cfgIter)
		{
			// find first compatible config
			if (isGLConfigCompatible(cfgInfo, *cfgIter))
			{
				break;
			}
		}

		if (cfgIter == configs.aospConfigs.end())
		{
			TCU_FAIL(("No suitable configuration found for GL config " +
					  de::toString(aosp_mustpass_es_first_cfg[i].glConfigName))
						 .c_str());
			return;
		}

		const char* apiName = getApiName(aosp_mustpass_es_first_cfg[i].apiType);

		const int width   = aosp_mustpass_es_first_cfg[i].surfaceWidth;
		const int height  = aosp_mustpass_es_first_cfg[i].surfaceHeight;

		TestRunParams params;
		params.logFilename = getLogFileName(apiName, aosp_mustpass_es_first_cfg[i].configName, 1, i, width, height, -1);
		getBaseOptions(params.args, mustpassDir, apiName, aosp_mustpass_es_first_cfg[i].configName,
					   aosp_mustpass_es_first_cfg[i].screenRotation, width, height);

		params.args.push_back(string("--deqp-gl-config-name=") + string(aosp_mustpass_es_first_cfg[i].glConfigName));

		//set surface type
		if ((*cfgIter).surfaceTypes & SURFACETYPE_WINDOW)
			params.args.push_back("--deqp-surface-type=window");
		else if ((*cfgIter).surfaceTypes & SURFACETYPE_PBUFFER)
			params.args.push_back("--deqp-surface-type=pbuffer");
		else if ((*cfgIter).surfaceTypes & SURFACETYPE_PIXMAP)
			params.args.push_back("--deqp-surface-type=pixmap");
		runs.push_back(params);
	}
}

static void getTestRunsForNoContext(glu::ApiType type, vector<TestRunParams>& runs, const ConfigList& configs, const RunParams* runParams,
									const int numRunParams, const char* mustpassDir)
{
	vector<Config>::const_iterator cfgIter = configs.configs.begin();

	for (int i = 0; i < numRunParams; ++i)
	{
		if (!glu::contextSupports(glu::ContextType(type), runParams[i].apiType))
				continue;

		const char* apiName = getApiName(runParams[i].apiType);

		const int width  = runParams[i].surfaceWidth;
		const int height = runParams[i].surfaceHeight;
		const int seed   = runParams[i].baseSeed;

		TestRunParams params;
		params.logFilename = getLogFileName(apiName, runParams[i].configName, 1, i, width, height, seed);

		getBaseOptions(params.args, mustpassDir, apiName, runParams[i].configName, runParams[i].screenRotation, width,
					   height);

		params.args.push_back(string("--deqp-base-seed=") + de::toString(seed));

		appendConfigArgs(*cfgIter, params.args, runParams[i].fboConfig);

		runs.push_back(params);
	}
}

static void getTestRunsForNoContextES(glu::ApiType type, vector<TestRunParams>& runs, const ConfigList& configs)
{
#include "glcKhronosMustpassEsNocontext.hpp"
	getTestRunsForNoContext(type, runs, configs, khronos_mustpass_es_nocontext_first_cfg,
							DE_LENGTH_OF_ARRAY(khronos_mustpass_es_nocontext_first_cfg), mustpassDir);
}

static void getTestRunsForES(glu::ApiType type, const ConfigList& configs, vector<TestRunParams>& runs)
{
	getTestRunsForAOSPEGL(runs, configs);
	getTestRunsForAOSPES(runs, configs, type);
	getTestRunsForNoContextES(type, runs, configs);

#include "glcKhronosMustpassEs.hpp"

	for (vector<Config>::const_iterator cfgIter = configs.configs.begin(); cfgIter != configs.configs.end(); ++cfgIter)
	{
		const bool isFirst		= cfgIter == configs.configs.begin();
		const int  numRunParams = isFirst ? DE_LENGTH_OF_ARRAY(khronos_mustpass_es_first_cfg) :
										   DE_LENGTH_OF_ARRAY(khronos_mustpass_es_other_cfg);
		const RunParams* runParams = isFirst ? khronos_mustpass_es_first_cfg : khronos_mustpass_es_other_cfg;

		for (int runNdx = 0; runNdx < numRunParams; runNdx++)
		{
			if (!glu::contextSupports(glu::ContextType(type), runParams[runNdx].apiType))
				continue;

			const char* apiName = getApiName(runParams[runNdx].apiType);

			const int width   = runParams[runNdx].surfaceWidth;
			const int height  = runParams[runNdx].surfaceHeight;
			const int seed	= runParams[runNdx].baseSeed;

			TestRunParams params;

			params.logFilename =
				getLogFileName(apiName, runParams[runNdx].configName, cfgIter->id, runNdx, width, height, seed);

			getBaseOptions(params.args, mustpassDir, apiName, runParams[runNdx].configName,
						   runParams[runNdx].screenRotation, width, height);
			params.args.push_back(string("--deqp-base-seed=") + de::toString(seed));

			appendConfigArgs(*cfgIter, params.args, runParams[runNdx].fboConfig);

			runs.push_back(params);
		}
	}
}

static void getTestRunsForNoContextGL(glu::ApiType type, vector<TestRunParams>& runs, const ConfigList& configs)
{
#include "glcKhronosMustpassGlNocontext.hpp"
	getTestRunsForNoContext(type, runs, configs, khronos_mustpass_gl_nocontext_first_cfg,
							DE_LENGTH_OF_ARRAY(khronos_mustpass_gl_nocontext_first_cfg), mustpassDir);
}

static void getTestRunsForGL(glu::ApiType type, const ConfigList& configs, vector<TestRunParams>& runs)
{
	getTestRunsForNoContextGL(type, runs, configs);
#include "glcKhronosMustpassGl.hpp"

	for (vector<Config>::const_iterator cfgIter = configs.configs.begin(); cfgIter != configs.configs.end(); ++cfgIter)
	{
		const bool isFirst		= cfgIter == configs.configs.begin();
		const int  numRunParams = isFirst ? DE_LENGTH_OF_ARRAY(khronos_mustpass_gl_first_cfg) :
										   DE_LENGTH_OF_ARRAY(khronos_mustpass_gl_other_cfg);
		const RunParams* runParams = isFirst ? khronos_mustpass_gl_first_cfg : khronos_mustpass_gl_other_cfg;

		for (int runNdx = 0; runNdx < numRunParams; runNdx++)
		{
			if (type != runParams[runNdx].apiType)
				continue;

			const char* apiName = getApiName(runParams[runNdx].apiType);

			const int width   = runParams[runNdx].surfaceWidth;
			const int height  = runParams[runNdx].surfaceHeight;
			const int seed	= runParams[runNdx].baseSeed;

			TestRunParams params;

			params.logFilename =
				getLogFileName(apiName, runParams[runNdx].configName, cfgIter->id, runNdx, width, height, seed);

			getBaseOptions(params.args, mustpassDir, apiName, runParams[runNdx].configName,
						   runParams[runNdx].screenRotation, width, height);
			params.args.push_back(string("--deqp-base-seed=") + de::toString(seed));

			appendConfigArgs(*cfgIter, params.args, runParams[runNdx].fboConfig);

			runs.push_back(params);
		}
	}
}

static void getTestRunParams(glu::ApiType type, const ConfigList& configs, vector<TestRunParams>& runs)
{
	switch (type.getProfile())
	{
	case glu::PROFILE_CORE:
		getTestRunsForGL(type, configs, runs);
		break;
	case glu::PROFILE_ES:
		getTestRunsForES(type, configs, runs);
		break;
	default:
		throw std::runtime_error("Unknown context type");
	}
}

struct FileDeleter
{
	void operator()(FILE* file) const
	{
		if (file)
			fclose(file);
	}
};

struct XmlWriterDeleter
{
	void operator()(qpXmlWriter* writer) const
	{
		if (writer)
			qpXmlWriter_destroy(writer);
	}
};

static const char* getRunTypeName(glu::ApiType type)
{
	if (type == glu::ApiType::es(2, 0))
		return "es2";
	else if (type == glu::ApiType::es(3, 0))
		return "es3";
	else if (type == glu::ApiType::es(3, 1))
		return "es31";
	else if (type == glu::ApiType::es(3, 2))
		return "es32";
	else if (type == glu::ApiType::core(3, 0))
		return "gl30";
	else if (type == glu::ApiType::core(3, 1))
		return "gl31";
	else if (type == glu::ApiType::core(3, 2))
		return "gl32";
	else if (type == glu::ApiType::core(3, 3))
		return "gl33";
	else if (type == glu::ApiType::core(4, 0))
		return "gl40";
	else if (type == glu::ApiType::core(4, 1))
		return "gl41";
	else if (type == glu::ApiType::core(4, 2))
		return "gl42";
	else if (type == glu::ApiType::core(4, 3))
		return "gl43";
	else if (type == glu::ApiType::core(4, 4))
		return "gl44";
	else if (type == glu::ApiType::core(4, 5))
		return "gl45";
	else if (type == glu::ApiType::core(4, 6))
		return "gl46";
	else
		return DE_NULL;
}

#define XML_CHECK(X) \
	if (!(X))        \
	throw tcu::Exception("Writing XML failed")

static void writeRunSummary(const TestRunSummary& summary, const char* filename)
{
	de::UniquePtr<FILE, FileDeleter> out(fopen(filename, "wb"));
	if (!out)
		throw tcu::Exception(string("Failed to open ") + filename);

	de::UniquePtr<qpXmlWriter, XmlWriterDeleter> writer(qpXmlWriter_createFileWriter(out.get(), DE_FALSE, DE_FALSE));
	if (!writer)
		throw std::bad_alloc();

	XML_CHECK(qpXmlWriter_startDocument(writer.get()));

	{
		qpXmlAttribute attribs[2];

		attribs[0] = qpSetStringAttrib("Type", getRunTypeName(summary.runType));
		attribs[1] = qpSetBoolAttrib("Conformant", summary.isConformant ? DE_TRUE : DE_FALSE);

		XML_CHECK(qpXmlWriter_startElement(writer.get(), "Summary", DE_LENGTH_OF_ARRAY(attribs), attribs));
	}

	// Config run
	{
		qpXmlAttribute attribs[1];
		attribs[0] = qpSetStringAttrib("FileName", summary.configLogFilename.c_str());
		XML_CHECK(qpXmlWriter_startElement(writer.get(), "Configs", DE_LENGTH_OF_ARRAY(attribs), attribs) &&
				  qpXmlWriter_endElement(writer.get(), "Configs"));
	}

	// Record test run parameters (log filename & command line).
	for (vector<TestRunParams>::const_iterator runIter = summary.runParams.begin(); runIter != summary.runParams.end();
		 ++runIter)
	{
		string		   cmdLine;
		qpXmlAttribute attribs[2];

		for (vector<string>::const_iterator argIter = runIter->args.begin(); argIter != runIter->args.end(); ++argIter)
		{
			if (argIter != runIter->args.begin())
				cmdLine += " ";
			cmdLine += *argIter;
		}

		attribs[0] = qpSetStringAttrib("FileName", runIter->logFilename.c_str());
		attribs[1] = qpSetStringAttrib("CmdLine", cmdLine.c_str());

		XML_CHECK(qpXmlWriter_startElement(writer.get(), "TestRun", DE_LENGTH_OF_ARRAY(attribs), attribs) &&
				  qpXmlWriter_endElement(writer.get(), "TestRun"));
	}

	XML_CHECK(qpXmlWriter_endElement(writer.get(), "Summary"));
	XML_CHECK(qpXmlWriter_endDocument(writer.get()));
}

#undef XML_CHECK

TestRunner::TestRunner(tcu::Platform& platform, tcu::Archive& archive, const char* logDirPath, glu::ApiType type,
					   deUint32 flags)
	: m_platform(platform)
	, m_archive(archive)
	, m_logDirPath(logDirPath)
	, m_type(type)
	, m_flags(flags)
	, m_iterState(ITERATE_INIT)
	, m_curSession(DE_NULL)
	, m_sessionsExecuted(0)
	, m_sessionsPassed(0)
	, m_sessionsFailed(0)
{
}

TestRunner::~TestRunner(void)
{
	delete m_curSession;
}

bool TestRunner::iterate(void)
{
	switch (m_iterState)
	{
	case ITERATE_INIT:
		init();
		m_iterState = (m_sessionIter != m_runSessions.end()) ? ITERATE_INIT_SESSION : ITERATE_DEINIT;
		return true;

	case ITERATE_DEINIT:
		deinit();
		m_iterState = ITERATE_INIT;
		return false;

	case ITERATE_INIT_SESSION:
		DE_ASSERT(m_sessionIter != m_runSessions.end());
		initSession(*m_sessionIter);
		if (m_flags & PRINT_SUMMARY)
			m_iterState = ITERATE_DEINIT_SESSION;
		else
			m_iterState = ITERATE_ITERATE_SESSION;
		return true;

	case ITERATE_DEINIT_SESSION:
		deinitSession();
		++m_sessionIter;
		m_iterState = (m_sessionIter != m_runSessions.end()) ? ITERATE_INIT_SESSION : ITERATE_DEINIT;
		return true;

	case ITERATE_ITERATE_SESSION:
		if (!iterateSession())
			m_iterState = ITERATE_DEINIT_SESSION;
		return true;

	default:
		DE_ASSERT(false);
		return false;
	}
}

void TestRunner::init(void)
{
	DE_ASSERT(m_runSessions.empty() && m_summary.runParams.empty());

	tcu::print("Running %s conformance\n", glu::getApiTypeDescription(m_type));

	m_summary.runType = m_type;

	// Get list of configs to test.
	ConfigList configList;
	getDefaultConfigList(m_platform, m_type, configList);

	tcu::print("  found %d compatible and %d excluded configs\n", (int)configList.configs.size(),
			   (int)configList.excludedConfigs.size());

	// Config list run.
	{
		const char*   configLogFilename = "configs.qpa";
		TestRunParams configRun;

		configRun.logFilename = configLogFilename;
		configRun.args.push_back("--deqp-case=CTS-Configs.*");
		m_runSessions.push_back(configRun);

		m_summary.configLogFilename = configLogFilename;
	}

	// Conformance test type specific runs
	getTestRunParams(m_type, configList, m_runSessions);

	// Record run params for summary.
	for (std::vector<TestRunParams>::const_iterator runIter = m_runSessions.begin() + 1; runIter != m_runSessions.end();
		 ++runIter)
		m_summary.runParams.push_back(*runIter);

	// Session iterator
	m_sessionIter = m_runSessions.begin();
}

void TestRunner::deinit(void)
{
	// Print out totals.
	bool isConformant = m_sessionsExecuted == m_sessionsPassed;
	DE_ASSERT(m_sessionsExecuted == m_sessionsPassed + m_sessionsFailed);
	tcu::print("\n%d/%d sessions passed, conformance test %s\n", m_sessionsPassed, m_sessionsExecuted,
			   isConformant ? "PASSED" : "FAILED");

	m_summary.isConformant = isConformant;

	// Write out summary.
	writeRunSummary(m_summary, de::FilePath::join(m_logDirPath, "cts-run-summary.xml").getPath());

	m_runSessions.clear();
	m_summary.clear();
}

void TestRunner::initSession(const TestRunParams& runParams)
{
	DE_ASSERT(!m_curSession);

	tcu::print("\n  Test run %d / %d\n", (int)(m_sessionIter - m_runSessions.begin() + 1), (int)m_runSessions.size());

	// Compute final args for run.
	vector<string> args(runParams.args);
	args.push_back(string("--deqp-log-filename=") + de::FilePath::join(m_logDirPath, runParams.logFilename).getPath());

	if (!(m_flags & VERBOSE_IMAGES))
		args.push_back("--deqp-log-images=disable");

	if (!(m_flags & VERBOSE_SHADERS))
		args.push_back("--deqp-log-shader-sources=disable");

	std::ostringstream			  ostr;
	std::ostream_iterator<string> out_it(ostr, ", ");
	std::copy(args.begin(), args.end(), out_it);
	tcu::print("\n  Config: %s \n\n", ostr.str().c_str());

	// Translate to argc, argv
	vector<const char*> argv;
	argv.push_back("cts-runner"); // Dummy binary name
	for (vector<string>::const_iterator i = args.begin(); i != args.end(); i++)
		argv.push_back(i->c_str());

	// Create session
	m_curSession = new RunSession(m_platform, m_archive, (int)argv.size(), &argv[0]);
}

void TestRunner::deinitSession(void)
{
	DE_ASSERT(m_curSession);

	// Collect results.
	// \note NotSupported is treated as pass.
	const tcu::TestRunStatus& result = m_curSession->getResult();
	bool					  isOk =
		result.numExecuted == (result.numPassed + result.numNotSupported + result.numWarnings) && result.isComplete;

	DE_ASSERT(result.numExecuted == result.numPassed + result.numFailed + result.numNotSupported + result.numWarnings);

	m_sessionsExecuted += 1;
	(isOk ? m_sessionsPassed : m_sessionsFailed) += 1;

	delete m_curSession;
	m_curSession = DE_NULL;
}

inline bool TestRunner::iterateSession(void)
{
	return m_curSession->iterate();
}

} // glcts
