#ifndef _ES31FINFOLOGQUERYSHARED_HPP
#define _ES31FINFOLOGQUERYSHARED_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief Info log query shared utilities
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "glwDefs.hpp"
#include "gluCallLogWrapper.hpp"
#include "tcuResultCollector.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

void verifyInfoLogQuery (tcu::ResultCollector&			result,
						 glu::CallLogWrapper&			gl,
						 int							logLen,
						 glw::GLuint					object,
						 void (glu::CallLogWrapper::	*getInfoLog)(glw::GLuint, glw::GLsizei, glw::GLsizei*, glw::GLchar*),
						 const char*					getterName);


} // Functional
} // gles31
} // deqp

#endif // _ES31FINFOLOGQUERYSHARED_HPP
