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
 * \brief CTS Android Activity.
 */ /*-------------------------------------------------------------------*/

#include "glcAndroidTestActivity.hpp"
#include "glcTestRunner.hpp"
#include "tcuAndroidAssets.hpp"
#include "tcuAndroidPlatform.hpp"
#include "tcuAndroidUtil.hpp"

#include <android/window.h>
#include <stdlib.h>

namespace glcts
{
namespace Android
{

using tcu::Android::Platform;
using tcu::Android::AssetArchive;
using tcu::Android::NativeActivity;

static const char* DEFAULT_LOG_PATH = "/sdcard";

static std::string getLogPath(ANativeActivity* activity)
{
	std::string path = tcu::Android::getIntentStringExtra(activity, "logdir");
	return path.empty() ? std::string(DEFAULT_LOG_PATH) : path;
}

static deUint32 getFlags(ANativeActivity* activity)
{
	deUint32 flags = 0;
	if (tcu::Android::getIntentStringExtra(activity, "verbose") == "true")
		flags |= TestRunner::VERBOSE_ALL;
	else if (tcu::Android::getIntentStringExtra(activity, "summary") == "true")
		flags |= TestRunner::PRINT_SUMMARY;
	return flags;
}

TestThread::TestThread(NativeActivity& activity, tcu::Android::AssetArchive& archive, const std::string& logPath,
					   glu::ApiType runType, deUint32 runFlags)
	: RenderThread(activity)
	, m_platform(activity)
	, m_archive(archive)
	, m_app(m_platform, m_archive, logPath.c_str(), runType, runFlags)
	, m_finished(false)
{
}

TestThread::~TestThread(void)
{
	// \note m_testApp is managed by thread.
}

void TestThread::run(void)
{
	RenderThread::run();
}

void TestThread::onWindowCreated(ANativeWindow* window)
{
	m_platform.getWindowRegistry().addWindow(window);
}

void TestThread::onWindowDestroyed(ANativeWindow* window)
{
	m_platform.getWindowRegistry().destroyWindow(window);
}

void TestThread::onWindowResized(ANativeWindow* window)
{
	// \todo [2013-05-12 pyry] Handle this in some sane way.
	DE_UNREF(window);
	tcu::print("Warning: Native window was resized, results may be undefined");
}

bool TestThread::render(void)
{
	if (!m_finished)
		m_finished = !m_app.iterate();
	return !m_finished;
}

// TestActivity

TestActivity::TestActivity(ANativeActivity* activity, glu::ApiType runType)
	: RenderActivity(activity)
	, m_archive(activity->assetManager)
	, m_cmdLine(tcu::Android::getIntentStringExtra(activity, "cmdLine"))
	, m_testThread(*this, m_archive, getLogPath(activity), runType, getFlags(activity))
	, m_started(false)
{
	// Set initial orientation.
	tcu::Android::setRequestedOrientation(getNativeActivity(),
										  tcu::Android::mapScreenRotation(m_cmdLine.getScreenRotation()));

	// Set up window flags.
	ANativeActivity_setWindowFlags(activity,
								   AWINDOW_FLAG_KEEP_SCREEN_ON | AWINDOW_FLAG_TURN_SCREEN_ON | AWINDOW_FLAG_FULLSCREEN |
									   AWINDOW_FLAG_SHOW_WHEN_LOCKED,
								   0);
}

TestActivity::~TestActivity(void)
{
}

void TestActivity::onStart(void)
{
	if (!m_started)
	{
		setThread(&m_testThread);
		m_testThread.start();
		m_started = true;
	}

	RenderActivity::onStart();
}

void TestActivity::onDestroy(void)
{
	if (m_started)
	{
		setThread(DE_NULL);
		m_testThread.stop();
		m_started = false;
	}

	RenderActivity::onDestroy();

	// Kill this process.
	tcu::print("Done, killing process");
	exit(0);
}

void TestActivity::onConfigurationChanged(void)
{
	RenderActivity::onConfigurationChanged();

	// Update rotation.
	tcu::Android::setRequestedOrientation(getNativeActivity(),
										  tcu::Android::mapScreenRotation(m_cmdLine.getScreenRotation()));
}

} // Android
} // glcts
