#ifndef _TCUSTRINGTEMPLATE_HPP
#define _TCUSTRINGTEMPLATE_HPP
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
 * \brief String template class.
 *//*--------------------------------------------------------------------*/

#include <map>
#include <string>

namespace tcu
{

class StringTemplate
{
public:
						StringTemplate		(void);
						StringTemplate		(const std::string& str);
						~StringTemplate		(void);

	void				setString			(const std::string& str);

	std::string			specialize			(const std::map<std::string, std::string>& params) const;

private:
						StringTemplate		(const StringTemplate&);		// not allowed!
	StringTemplate&		operator=			(const StringTemplate&);		// not allowed!

	std::string			m_template;
} DE_WARN_UNUSED_TYPE;

} // tcu

#endif // _TCUSTRINGTEMPLATE_HPP
