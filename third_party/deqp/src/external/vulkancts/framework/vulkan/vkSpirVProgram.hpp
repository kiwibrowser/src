#ifndef _VKSPIRVPROGRAM_HPP
#define _VKSPIRVPROGRAM_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief SPIR-V program and binary info.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "deStringUtil.hpp"

#include <string>

namespace tcu
{
class TestLog;
} // tcu

namespace vk
{

struct SpirVAsmSource
{
	SpirVAsmSource (void)
	{
	}

	SpirVAsmSource (const std::string& source_)
		: source(source_)
	{
	}

	std::string		source;
};

struct SpirVProgramInfo
{
	SpirVProgramInfo (void)
		: compileTimeUs	(0)
		, compileOk		(false)
	{
	}

	std::string		source;
	std::string		infoLog;
	deUint64		compileTimeUs;
	bool			compileOk;
};

tcu::TestLog&	operator<<		(tcu::TestLog& log, const SpirVProgramInfo& shaderInfo);
tcu::TestLog&	operator<<		(tcu::TestLog& log, const SpirVAsmSource& program);

// Helper for constructing SpirVAsmSource
template<typename T>
SpirVAsmSource& operator<< (SpirVAsmSource& src, const T& val)
{
	src.source += de::toString(val);
	return src;
}

}

#endif // _VKSPIRVPROGRAM_HPP
