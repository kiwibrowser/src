#ifndef _TCUPLATFORM_HPP
#define _TCUPLATFORM_HPP
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
 * \brief Platform (OS) specific services.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"

namespace glu
{
class Platform;
}

namespace eglu
{
class Platform;
}

namespace vk
{
class Platform;
}

namespace tcu
{

class CommandLine;
class FunctionLibrary;

/*--------------------------------------------------------------------*//*!
 * \brief Base class for platform implementation.
 *
 * This class represents the minimum set of functionality for a platform
 * port.
 *
 * In addition to implementing Platform class, main entry point must be
 * created that takes care of parsing command line, creating log and
 * executing tcu::App. See tcuMain.cpp for reference on implementing
 * application stub.
 *
 * If the platform uses standard posix-style main() for application entry
 * point, tcuMain.cpp can be used as is. In that case you only have to
 * implement createPlatform().
 *
 * API-specific platform interfaces (glu::Platform and eglu::Platform)
 * can be provided by implementing get<API>Platform() functions.
 *//*--------------------------------------------------------------------*/
class Platform
{
public:
									Platform			(void);
	virtual							~Platform			(void);

	/*--------------------------------------------------------------------*//*!
	 * \brief Process platform-specific events.
	 *
	 * Test framework will call this function between test cases and test case
	 * iterations. Any event handling that must be done periodically should be
	 * done here.
	 *
	 * Test framework will decide whether to continue test execution based on
	 * return code. For instance if the application receives close event from OS,
	 * it should communicate that to framework by returning false.
	 *
	 * \note Do not do rendering buffer swaps here.
	 *       Do it in RenderContext::postIterate() instead.
	 * \return true if test execution should continue, false otherwise.
	 *//*--------------------------------------------------------------------*/
	virtual bool					processEvents		(void);

	/*--------------------------------------------------------------------*//*!
	 * \brief Get GL platform interface
	 *
	 * GL-specific platform interface is defined by glu::Platform. If your
	 * platform port supports OpenGL (ES), you should implement this function.
	 *
	 * Default implementation throws tcu::NotSupportedError exception.
	 *
	 * \return Reference to GL platform interface.
	 *//*--------------------------------------------------------------------*/
	virtual const glu::Platform&	getGLPlatform		(void) const;

	/*--------------------------------------------------------------------*//*!
	 * \brief Get EGL platform interface
	 *
	 * EGL-specific platform interface is defined by eglu::Platform. If your
	 * platform port supports EGL, you should implement this function.
	 *
	 * Default implementation throws tcu::NotSupportedError exception.
	 *
	 * \return Reference to EGL platform interface.
	 *//*--------------------------------------------------------------------*/
	virtual const eglu::Platform&	getEGLPlatform		(void) const;

	virtual const vk::Platform&		getVulkanPlatform	(void) const;
};

} // tcu

#endif // _TCUPLATFORM_HPP
