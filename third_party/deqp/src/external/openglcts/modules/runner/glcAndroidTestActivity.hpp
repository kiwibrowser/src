#ifndef _GLCANDROIDTESTACTIVITY_HPP
#define _GLCANDROIDTESTACTIVITY_HPP
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

#include "glcTestRunner.hpp"
#include "gluRenderContext.hpp"
#include "tcuAndroidAssets.hpp"
#include "tcuAndroidPlatform.hpp"
#include "tcuAndroidRenderActivity.hpp"
#include "tcuCommandLine.hpp"
#include "tcuDefs.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{
namespace Android
{

class TestThread : public tcu::Android::RenderThread
{
public:
	TestThread(tcu::Android::NativeActivity& nativeActivity, tcu::Android::AssetArchive& archive,
			   const std::string& logPath, glu::ApiType runType, deUint32 runFlags);
	~TestThread(void);

	void run(void);

protected:
	virtual void onWindowCreated(ANativeWindow* window);
	virtual void onWindowResized(ANativeWindow* window);
	virtual void onWindowDestroyed(ANativeWindow* window);
	virtual bool render(void);

	tcu::Android::Platform		m_platform;
	tcu::Android::AssetArchive& m_archive;
	TestRunner					m_app;
	bool						m_finished; //!< Is execution finished.
};

class TestActivity : public tcu::Android::RenderActivity
{
public:
	TestActivity(ANativeActivity* nativeActivity, glu::ApiType runType);
	~TestActivity(void);

	virtual void onStart(void);
	virtual void onDestroy(void);
	virtual void onConfigurationChanged(void);

private:
	tcu::Android::AssetArchive m_archive;
	tcu::CommandLine		   m_cmdLine;
	TestThread				   m_testThread;
	bool					   m_started;
};

} // Android
} // glcts

#endif // _GLCANDROIDTESTACTIVITY_HPP
