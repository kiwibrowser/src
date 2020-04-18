#ifndef _GLCCONFIGLIST_HPP
#define _GLCCONFIGLIST_HPP
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

#include "gluPlatform.hpp"
#include "tcuCommandLine.hpp"
#include "tcuDefs.hpp"

#include <string>
#include <vector>

namespace glcts
{

enum ConfigType
{
	CONFIGTYPE_DEFAULT = 0, //!< Only default config (no parameters).
	CONFIGTYPE_EGL,			//!< EGL config.
	CONFIGTYPE_WGL,			//!< WGL config.

	CONFIGTYPE_LAST
};

enum SurfaceTypeFlags
{
	SURFACETYPE_WINDOW  = (1 << tcu::SURFACETYPE_WINDOW),
	SURFACETYPE_PIXMAP  = (1 << tcu::SURFACETYPE_OFFSCREEN_NATIVE),
	SURFACETYPE_PBUFFER = (1 << tcu::SURFACETYPE_OFFSCREEN_GENERIC),
	SURFACETYPE_FBO		= (1 << tcu::SURFACETYPE_FBO),
};

enum ExcludeReason
{
	EXCLUDEREASON_NOT_COMPATIBLE = 0, //!< Not compatible with target API
	EXCLUDEREASON_NOT_CONFORMANT,	 //!< Compatible but not conformant
	EXCLUDEREASON_MSAA,				  //!< Compatible but not testable with current tests
	EXCLUDEREASON_FLOAT,			  //!< Compatible but not testable with current tests
	EXCLUDEREASON_YUV,				  //!< Compatible but not testable with current tests
	EXCLUDEREASON_LAST
};

struct Config
{
	Config(ConfigType type_, int id_, deUint32 surfaceTypes_) : type(type_), id(id_), surfaceTypes(surfaceTypes_)
	{
	}

	Config(void) : type(CONFIGTYPE_LAST), id(0), surfaceTypes(0)
	{
	}

	ConfigType type;
	int		   id;
	deUint32   surfaceTypes;
};

struct ExcludedConfig
{
	ExcludedConfig(ConfigType type_, int id_, ExcludeReason reason_) : type(type_), id(id_), reason(reason_)
	{
	}

	ExcludedConfig(void) : type(CONFIGTYPE_LAST), id(0), reason(EXCLUDEREASON_LAST)
	{
	}

	ConfigType	type;
	int			  id;
	ExcludeReason reason;
};

struct AOSPConfig
{
	AOSPConfig(ConfigType type_, int id_, deUint32 surfaceTypes_, deInt32 redBits_, deInt32 greenBits_,
			   deInt32 blueBits_, deInt32 alphaBits_, deInt32 depthBits_, deInt32 stencilBits_, deInt32 samples_)
		: type(type_)
		, id(id_)
		, surfaceTypes(surfaceTypes_)
		, redBits(redBits_)
		, greenBits(greenBits_)
		, blueBits(blueBits_)
		, alphaBits(alphaBits_)
		, depthBits(depthBits_)
		, stencilBits(stencilBits_)
		, samples(samples_)
	{
	}

	AOSPConfig(void)
		: type(CONFIGTYPE_LAST)
		, id(0)
		, surfaceTypes(0)
		, redBits(0)
		, greenBits(0)
		, blueBits(0)
		, alphaBits(0)
		, depthBits(0)
		, stencilBits(0)
		, samples(0)
	{
	}

	ConfigType type;
	int		   id;
	deUint32   surfaceTypes;
	deInt32	redBits;
	deInt32	greenBits;
	deInt32	blueBits;
	deInt32	alphaBits;
	deInt32	depthBits;
	deInt32	stencilBits;
	deInt32	samples;
};

class ConfigList
{
public:
	// Configs exposed by an implementation which are required to pass all non-AOSP tests.
	// This includes all configs marked as conformant but not multisample configs.
	std::vector<Config> configs;
	// Configs exposed by an implementation which are not required to pass the CTS.
	// This includes non-conformant and multisample configs.
	std::vector<ExcludedConfig> excludedConfigs;
	// Configs exposed by an implementation which will be used to determine AOSP runs parameters.
	// This includes all configs marked as conformant.
	std::vector<AOSPConfig> aospConfigs;
};

void getDefaultConfigList(tcu::Platform& platform, glu::ApiType type, ConfigList& configList);

} // glcts

#endif // _GLCCONFIGLIST_HPP
