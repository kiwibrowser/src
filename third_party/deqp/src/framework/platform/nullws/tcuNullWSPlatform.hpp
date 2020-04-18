#ifndef _TCUNULLWSPLATFORM_HPP
#define _TCUNULLWSPLATFORM_HPP
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
 *//*!
 * \file
 * \brief
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuPlatform.hpp"
#include "gluPlatform.hpp"
#include "egluPlatform.hpp"

namespace tcu
{
namespace nullws
{

class Platform: public tcu::Platform, private glu::Platform, private eglu::Platform
{
public:
									Platform		();
	virtual							~Platform		();

	virtual const glu::Platform&	getGLPlatform	()	const { return static_cast<const glu::Platform&>(*this); }
	virtual const eglu::Platform&	getEGLPlatform	()	const { return static_cast<const eglu::Platform&>(*this); }
};

} // nullws
} // tcu

#endif // _TCUNULLWSPLATFORM_HPP
