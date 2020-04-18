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

#include "deUniquePtr.hpp"
#include "glcConfigList.hpp"

#include <typeinfo>

#if defined(GLCTS_SUPPORT_WGL)
#include "tcuWGL.hpp"
#include "tcuWin32Platform.hpp"
#include "tcuWin32Window.hpp"
#endif

namespace glcts
{

#if defined(GLCTS_SUPPORT_WGL)

static void getDefaultWglConfigList(tcu::win32::Platform& wglPlatform, glu::ApiType type, ConfigList& configList)
{
	const HINSTANCE			 instance = GetModuleHandle(DE_NULL);
	const tcu::wgl::Core&	wgl(instance);
	const tcu::win32::Window tmpWindow(instance, 1, 1);
	const std::vector<int>   pixelFormats = wgl.getPixelFormats(tmpWindow.getDeviceContext());

	DE_UNREF(type); // \todo [2013-09-16 pyry] Check for support.

	for (std::vector<int>::const_iterator fmtIter = pixelFormats.begin(); fmtIter != pixelFormats.end(); ++fmtIter)
	{
		const int						pixelFormat = *fmtIter;
		const tcu::wgl::PixelFormatInfo fmtInfo		= wgl.getPixelFormatInfo(tmpWindow.getDeviceContext(), pixelFormat);

		if (!tcu::wgl::isSupportedByTests(fmtInfo))
			continue;

		bool isAOSPOk = (fmtInfo.surfaceTypes & tcu::wgl::PixelFormatInfo::SURFACE_WINDOW) && fmtInfo.supportOpenGL &&
						fmtInfo.pixelType == tcu::wgl::PixelFormatInfo::PIXELTYPE_RGBA;
		bool isOk = isAOSPOk && (fmtInfo.sampleBuffers == 0);

		if (isAOSPOk)
		{
			configList.aospConfigs.push_back(AOSPConfig(
				CONFIGTYPE_WGL, pixelFormat, SURFACETYPE_WINDOW, fmtInfo.redBits, fmtInfo.greenBits, fmtInfo.blueBits,
				fmtInfo.alphaBits, fmtInfo.depthBits, fmtInfo.stencilBits, fmtInfo.samples));
		}

		if (isOk)
		{
			configList.configs.push_back(Config(CONFIGTYPE_WGL, pixelFormat, SURFACETYPE_WINDOW));
		}
		else
		{
			configList.excludedConfigs.push_back(
				ExcludedConfig(CONFIGTYPE_WGL, pixelFormat, EXCLUDEREASON_NOT_COMPATIBLE));
		}
	}
}

void getConfigListWGL(tcu::Platform& platform, glu::ApiType type, ConfigList& configList)
{
	try
	{
		tcu::win32::Platform& wglPlatform = dynamic_cast<tcu::win32::Platform&>(platform);
		getDefaultWglConfigList(wglPlatform, type, configList);
	}
	catch (const std::bad_cast&)
	{
		throw tcu::Exception("Platform is not tcu::WGLPlatform");
	}
}

#else

void getConfigListWGL(tcu::Platform&, glu::ApiType, ConfigList&)
{
	throw tcu::Exception("WGL is not supported on this OS");
}

#endif

} // glcts
