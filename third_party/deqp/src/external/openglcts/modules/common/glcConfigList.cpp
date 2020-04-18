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
 * \brief CTS rendering configuration list utility.
 */ /*-------------------------------------------------------------------*/

#include "glcConfigList.hpp"
#include "glcConfigListEGL.hpp"
#include "glcConfigListWGL.hpp"
#include "qpDebugOut.h"

namespace glcts
{

void getDefaultConfigList(tcu::Platform& platform, glu::ApiType type, ConfigList& configList)
{
	try
	{
		getConfigListEGL(platform, type, configList);
	}
	catch (const std::exception& e)
	{
		qpPrintf("No EGL configs enumerated: %s\n", e.what());
	}

	try
	{
		getConfigListWGL(platform, type, configList);
	}
	catch (const std::exception& e)
	{
		qpPrintf("No WGL configs enumerated: %s\n", e.what());
	}

	if (configList.configs.empty())
	{
		qpPrintf("Warning: No configs enumerated, adding only default config!\n");
		configList.configs.push_back(Config(CONFIGTYPE_DEFAULT, 0, SURFACETYPE_WINDOW));
	}
}

} // glcts
