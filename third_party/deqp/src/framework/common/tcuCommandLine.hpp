#ifndef _TCUCOMMANDLINE_HPP
#define _TCUCOMMANDLINE_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
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
 * \brief Command line parsing.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "deCommandLine.hpp"
#include "deUniquePtr.hpp"

#include <string>
#include <vector>
#include <istream>

namespace tcu
{

/*--------------------------------------------------------------------*//*!
 * \brief Run mode tells whether the test program should run the tests or
 *		  dump out metadata about the tests.
 *//*--------------------------------------------------------------------*/
enum RunMode
{
	RUNMODE_EXECUTE = 0,			//! Test program executes the tests.
	RUNMODE_DUMP_XML_CASELIST,		//! Test program dumps the list of contained test cases in XML format.
	RUNMODE_DUMP_TEXT_CASELIST,		//! Test program dumps the list of contained test cases in plain-text format.
	RUNMODE_DUMP_STDOUT_CASELIST,	//! Test program dumps the list of contained test cases in plain-text format into stdout.

	RUNMODE_LAST
};

/*--------------------------------------------------------------------*//*!
 * \brief Should graphical tests show rendering results on screen.
 *//*--------------------------------------------------------------------*/
enum WindowVisibility
{
	WINDOWVISIBILITY_WINDOWED = 0,
	WINDOWVISIBILITY_FULLSCREEN,
	WINDOWVISIBILITY_HIDDEN,

	WINDOWVISIBILITY_LAST
};

/*--------------------------------------------------------------------*//*!
 * \brief The type of rendering surface the tests should be executed on.
 *//*--------------------------------------------------------------------*/
enum SurfaceType
{
	SURFACETYPE_WINDOW = 0,			//!< Native window.
	SURFACETYPE_OFFSCREEN_NATIVE,	//!< Native offscreen surface, such as pixmap.
	SURFACETYPE_OFFSCREEN_GENERIC,	//!< Generic offscreen surface, such as pbuffer.
	SURFACETYPE_FBO,				//!< Framebuffer object.

	SURFACETYPE_LAST
};

/*--------------------------------------------------------------------*//*!
 * \brief Screen rotation, always to clockwise direction.
 *//*--------------------------------------------------------------------*/
enum ScreenRotation
{
	SCREENROTATION_UNSPECIFIED,		//!< Use default / current orientation.
	SCREENROTATION_0,				//!< Set rotation to 0 degrees from baseline.
	SCREENROTATION_90,
	SCREENROTATION_180,
	SCREENROTATION_270,

	SCREENROTATION_LAST
};

class CaseTreeNode;
class CasePaths;
class Archive;

class CaseListFilter
{
public:
									CaseListFilter				(const de::cmdline::CommandLine& cmdLine, const tcu::Archive& archive);
									CaseListFilter				(void);
									~CaseListFilter				(void);

	//! Check if test group is in supplied test case list.
	bool							checkTestGroupName			(const char* groupName) const;

	//! Check if test case is in supplied test case list.
	bool							checkTestCaseName			(const char* caseName) const;

private:
	CaseListFilter												(const CaseListFilter&);	// not allowed!
	CaseListFilter&					operator=					(const CaseListFilter&);	// not allowed!

	CaseTreeNode*					m_caseTree;
	de::MovePtr<const CasePaths>	m_casePaths;
};

/*--------------------------------------------------------------------*//*!
 * \brief Test command line
 *
 * CommandLine handles argument parsing and provides convinience functions
 * for querying test parameters.
 *//*--------------------------------------------------------------------*/
class CommandLine
{
public:
									CommandLine					(void);
									CommandLine					(int argc, const char* const* argv);
	explicit						CommandLine					(const std::string& cmdLine);
									~CommandLine				(void);

	bool							parse						(int argc, const char* const* argv);
	bool							parse						(const std::string& cmdLine);

	//! Get log file name (--deqp-log-filename)
	const char*						getLogFileName				(void) const;

	//! Get logging flags
	deUint32						getLogFlags					(void) const;

	//! Get run mode (--deqp-runmode)
	RunMode							getRunMode					(void) const;

	//! Get caselist dump target file pattern (--deqp-caselist-export-file)
	const char*						getCaseListExportFile		(void) const;

	//! Get default window visibility (--deqp-visibility)
	WindowVisibility				getVisibility				(void) const;

	//! Get watchdog enable status (--deqp-watchdog)
	bool							isWatchDogEnabled			(void) const;

	//! Get crash handling enable status (--deqp-crashhandler)
	bool							isCrashHandlingEnabled		(void) const;

	//! Get base seed for randomization (--deqp-base-seed)
	int								getBaseSeed					(void) const;

	//! Get test iteration count (--deqp-test-iteration-count)
	int								getTestIterationCount		(void) const;

	//! Get rendering target width (--deqp-surface-width)
	int								getSurfaceWidth				(void) const;

	//! Get rendering target height (--deqp-surface-height)
	int								getSurfaceHeight			(void) const;

	//! Get rendering taget type (--deqp-surface-type)
	SurfaceType						getSurfaceType				(void) const;

	//! Get screen rotation (--deqp-screen-rotation)
	ScreenRotation					getScreenRotation			(void) const;

	//! Get GL context factory name (--deqp-gl-context-type)
	const char*						getGLContextType			(void) const;

	//! Get GL config ID (--deqp-gl-config-id)
	int								getGLConfigId				(void) const;

	//! Get GL config name (--deqp-gl-config-name)
	const char*						getGLConfigName				(void) const;

	//! Get GL context flags (--deqp-gl-context-flags)
	const char*						getGLContextFlags			(void) const;

	//! Get OpenCL platform ID (--deqp-cl-platform-id)
	int								getCLPlatformId				(void) const;

	//! Get OpenCL device IDs (--deqp-cl-device-ids)
	void							getCLDeviceIds				(std::vector<int>& deviceIds) const	{ deviceIds = getCLDeviceIds(); }
	const std::vector<int>&			getCLDeviceIds				(void) const;

	//! Get extra OpenCL program build options (--deqp-cl-build-options)
	const char*						getCLBuildOptions			(void) const;

	//! Get EGL native display factory (--deqp-egl-display-type)
	const char*						getEGLDisplayType			(void) const;

	//! Get EGL native window factory (--deqp-egl-window-type)
	const char*						getEGLWindowType			(void) const;

	//! Get EGL native pixmap factory (--deqp-egl-pixmap-type)
	const char*						getEGLPixmapType			(void) const;

	//! Get Vulkan device ID (--deqp-vk-device-id)
	int								getVKDeviceId				(void) const;

	//! Enable development-time test case validation checks
	bool							isValidationEnabled			(void) const;

	//! Should we run tests that exhaust memory (--deqp-test-oom)
	bool							isOutOfMemoryTestEnabled	(void) const;

	/*--------------------------------------------------------------------*//*!
	 * \brief Creates case list filter
	 * \param archive Resources
	 *
	 * Creates case list filter based on one of the following parameters:
	 *
	 * --deqp-case
	 * --deqp-caselist
	 * --deqp-caselist-file
	 * --deqp-caselist-resource
	 * --deqp-stdin-caselist
	 *
	 * Throws std::invalid_argument if parsing fails.
	 *//*--------------------------------------------------------------------*/
	de::MovePtr<CaseListFilter>		createCaseListFilter		(const tcu::Archive& archive) const;

protected:
	const de::cmdline::CommandLine&	getCommandLine				(void) const;

private:
									CommandLine					(const CommandLine&);	// not allowed!
	CommandLine&					operator=					(const CommandLine&);	// not allowed!

	void							clear						(void);

	virtual void					registerExtendedOptions		(de::cmdline::Parser& parser);

	de::cmdline::CommandLine		m_cmdLine;
	deUint32						m_logFlags;
};

} // tcu

#endif // _TCUCOMMANDLINE_HPP
