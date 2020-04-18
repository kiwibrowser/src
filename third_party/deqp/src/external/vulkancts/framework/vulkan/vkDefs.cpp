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
 * \brief Vulkan utilites.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkStrUtil.hpp"

#include <sstream>

DE_STATIC_ASSERT(sizeof(vk::VkImageType)	== sizeof(deUint32));
DE_STATIC_ASSERT(sizeof(vk::VkResult)		== sizeof(deUint32));
DE_STATIC_ASSERT(sizeof(vk::VkDevice)		== sizeof(void*));
DE_STATIC_ASSERT(sizeof(vk::VkBuffer)		== sizeof(deUint64));

namespace vk
{

static bool isOutOfMemoryError (VkResult result)
{
	return result == VK_ERROR_OUT_OF_DEVICE_MEMORY	||
		   result == VK_ERROR_OUT_OF_HOST_MEMORY;
}

Error::Error (VkResult error, const char* message, const char* expr, const char* file, int line)
	: tcu::TestError	(message, expr, file, line)
	, m_error			(error)
{
}

Error::Error (VkResult error, const std::string& message)
	: tcu::TestError	(message)
	, m_error			(error)
{
}

Error::~Error (void) throw()
{
}

OutOfMemoryError::OutOfMemoryError (VkResult error, const char* message, const char* expr, const char* file, int line)
	: tcu::ResourceError(message, expr, file, line)
	, m_error			(error)
{
	DE_ASSERT(isOutOfMemoryError(error));
}

OutOfMemoryError::OutOfMemoryError (VkResult error, const std::string& message)
	: tcu::ResourceError(message)
	, m_error			(error)
{
	DE_ASSERT(isOutOfMemoryError(error));
}

OutOfMemoryError::~OutOfMemoryError (void) throw()
{
}

void checkResult (VkResult result, const char* msg, const char* file, int line)
{
	if (result != VK_SUCCESS)
	{
		std::ostringstream msgStr;
		if (msg)
			msgStr << msg << ": ";

		msgStr << getResultStr(result);

		if (isOutOfMemoryError(result))
			throw OutOfMemoryError(result, msgStr.str().c_str(), DE_NULL, file, line);
		else
			throw Error(result, msgStr.str().c_str(), DE_NULL, file, line);
	}
}

} // vk
