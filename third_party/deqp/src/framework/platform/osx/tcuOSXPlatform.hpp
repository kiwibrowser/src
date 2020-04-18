#ifndef _TCUOSXPLATFORM_HPP
#define _TCUOSXPLATFORM_HPP
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
 * \brief OS X platform.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuPlatform.hpp"
#include "gluPlatform.hpp"

namespace tcu
{

class OSXPlatform : public tcu::Platform, private glu::Platform
{
public:
							OSXPlatform		(void);
							~OSXPlatform	(void);

	const glu::Platform&	getGLPlatform	(void) const { return static_cast<const glu::Platform&>(*this);	}
};

} // tcu

#endif // _TCUOSXPLATFORM_HPP
