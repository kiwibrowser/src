#ifndef _TCUWIN32VULKANPLATFORM_HPP
#define _TCUWIN32VULKANPLATFORM_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2016 The Android Open Source Project
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
 * \brief Win32 Vulkan platform
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vkPlatform.hpp"
#include "tcuWin32API.h"

namespace tcu
{
namespace win32
{

class VulkanPlatform : public vk::Platform
{
public:
						VulkanPlatform		(HINSTANCE instance);
						~VulkanPlatform		(void);

	vk::Library*		createLibrary		(void) const;
	vk::wsi::Display*	createWsiDisplay	(vk::wsi::Type wsiType) const;

	void				getMemoryLimits		(vk::PlatformMemoryLimits& limits) const;
	void				describePlatform	(std::ostream& dst) const;

private:
	const HINSTANCE		m_instance;
};

} // win32
} // tcu

#endif // _TCUWIN32VULKANPLATFORM_HPP
