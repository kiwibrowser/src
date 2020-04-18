#ifndef _VKDEBUGREPORTUTIL_HPP
#define _VKDEBUGREPORTUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief VK_EXT_debug_report utilities
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "deAppendList.hpp"

#include <ostream>

namespace vk
{

struct DebugReportMessage
{
	VkDebugReportFlagsEXT		flags;
	VkDebugReportObjectTypeEXT	objectType;
	deUint64					object;
	size_t						location;
	deInt32						messageCode;
	std::string					layerPrefix;
	std::string					message;

	DebugReportMessage (void)
		: flags			(0)
		, objectType	((VkDebugReportObjectTypeEXT)0)
		, object		(0)
		, location		(0)
		, messageCode	(0)
	{}

	DebugReportMessage (VkDebugReportFlagsEXT		flags_,
						VkDebugReportObjectTypeEXT	objectType_,
						deUint64					object_,
						size_t						location_,
						deInt32						messageCode_,
						const std::string&			layerPrefix_,
						const std::string&			message_)
		: flags			(flags_)
		, objectType	(objectType_)
		, object		(object_)
		, location		(location_)
		, messageCode	(messageCode_)
		, layerPrefix	(layerPrefix_)
		, message		(message_)
	{}
};

std::ostream&	operator<<	(std::ostream& str, const DebugReportMessage& message);

class DebugReportRecorder
{
public:
	typedef de::AppendList<DebugReportMessage>	MessageList;

											DebugReportRecorder		(const InstanceInterface& vki, VkInstance instance);
											~DebugReportRecorder	(void);

	const MessageList&						getMessages				(void) const { return m_messages; }
	void									clearMessages			(void) { m_messages.clear(); }

private:
	MessageList								m_messages;
	const Unique<VkDebugReportCallbackEXT>	m_callback;
};

bool	isDebugReportSupported		(const PlatformInterface& vkp);

} // vk

#endif // _VKDEBUGREPORTUTIL_HPP
