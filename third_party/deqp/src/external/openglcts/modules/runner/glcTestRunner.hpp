#ifndef _GLCTESTRUNNER_HPP
#define _GLCTESTRUNNER_HPP
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

#include "gluPlatform.hpp"
#include "tcuDefs.hpp"

#include <string>
#include <vector>

namespace tcu
{

class Archive;

} // tcu

namespace glcts
{

struct Config;

struct RunParams
{
	glu::ApiType apiType;
	const char*  configName;
	const char*  glConfigName;
	const char*  screenRotation;
	int			 baseSeed;
	const char*  fboConfig;
	int			 surfaceWidth;
	int			 surfaceHeight;
};

struct TestRunParams
{
	std::vector<std::string> args;
	std::string				 logFilename;
};

// Conformance test run summary - written to cts-run-summary.xml
struct TestRunSummary
{
	glu::ApiType			   runType;
	bool					   isConformant;
	std::string				   configLogFilename;
	std::vector<TestRunParams> runParams;

	TestRunSummary(void) : isConformant(false)
	{
	}

	void clear(void)
	{
		runType		 = glu::ApiType();
		isConformant = false;
		configLogFilename.clear();
		runParams.clear();
	}
};

class RunSession;

class TestRunner
{
public:
	enum Flags
	{
		VERBOSE_COMMANDS = (1 << 0),
		VERBOSE_IMAGES   = (1 << 1),
		VERBOSE_SHADERS  = (1 << 2),

		VERBOSE_ALL = VERBOSE_COMMANDS | VERBOSE_IMAGES,

		PRINT_SUMMARY = (1 << 3)
	};

	TestRunner(tcu::Platform& platform, tcu::Archive& archive, const char* logDirPath, glu::ApiType type,
			   deUint32 flags);
	~TestRunner(void);

	bool iterate(void);

private:
	TestRunner(const TestRunner& other);
	TestRunner operator=(const TestRunner& other);

	void init(void);
	void deinit(void);

	void initSession(const TestRunParams& runParams);
	void deinitSession(void);
	bool iterateSession(void);

	enum IterateState
	{
		ITERATE_INIT = 0, //!< Call init() on this iteration.
		ITERATE_DEINIT,   //!< Call deinit() on this iteration.

		ITERATE_INIT_SESSION,	//!< Init current session.
		ITERATE_DEINIT_SESSION,  //!< Deinit session and move to next.
		ITERATE_ITERATE_SESSION, //!< Iterate current session.

		ITERATESTATE_LAST
	};

	tcu::Platform& m_platform;
	tcu::Archive&  m_archive;
	std::string	m_logDirPath;
	glu::ApiType   m_type;
	deUint32	   m_flags;

	// Iteration state.
	IterateState							   m_iterState;
	std::vector<TestRunParams>				   m_runSessions;
	std::vector<TestRunParams>::const_iterator m_sessionIter;
	RunSession*								   m_curSession;

	// Totals / stats.
	int			   m_sessionsExecuted;
	int			   m_sessionsPassed;
	int			   m_sessionsFailed;
	TestRunSummary m_summary;
};

} // glcts

#endif // _GLCTESTRUNNER_HPP
