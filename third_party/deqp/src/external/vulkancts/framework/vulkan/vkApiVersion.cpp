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

#include "vkApiVersion.hpp"

namespace vk
{

ApiVersion unpackVersion (deUint32 version)
{
	return ApiVersion((version & 0xFFC00000) >> 22,
					  (version & 0x003FF000) >> 12,
					   version & 0x00000FFF);
}

deUint32 pack (const ApiVersion& version)
{
	DE_ASSERT((version.majorNum & ~0x3FF) == 0);
	DE_ASSERT((version.minorNum & ~0x3FF) == 0);
	DE_ASSERT((version.patchNum & ~0xFFF) == 0);

	return (version.majorNum << 22) | (version.minorNum << 12) | version.patchNum;
}

} // vk
