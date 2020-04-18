#ifndef _VKAPIVERSION_HPP
#define _VKAPIVERSION_HPP
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
 * \brief Vulkan api version.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"

#include <ostream>

namespace vk
{

struct ApiVersion
{
	deUint32	majorNum;
	deUint32	minorNum;
	deUint32	patchNum;

	ApiVersion (deUint32	majorNum_,
				deUint32	minorNum_,
				deUint32	patchNum_)
		: majorNum	(majorNum_)
		, minorNum	(minorNum_)
		, patchNum	(patchNum_)
	{
	}
};

ApiVersion		unpackVersion		(deUint32 version);
deUint32		pack				(const ApiVersion& version);

inline std::ostream& operator<< (std::ostream& s, const ApiVersion& version)
{
	return s << version.majorNum << "." << version.minorNum << "." << version.patchNum;
}

} // vk

#endif // _VKAPIVERSION_HPP
