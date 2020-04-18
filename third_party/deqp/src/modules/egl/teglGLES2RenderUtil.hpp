#ifndef _TEGLGLES2RENDERUTIL_HPP
#define _TEGLGLES2RENDERUTIL_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
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
 * \brief GLES2 render utils.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuSurface.hpp"
#include "tcuVector.hpp"

namespace glw
{
class Functions;
}

namespace deqp
{
namespace egl
{
namespace gles2
{

void	clear		(const glw::Functions& gl, int x, int y, int width, int height, const tcu::Vec4& color);
void	readPixels	(const glw::Functions& gl, tcu::Surface& dst, int x, int y, int width, int height);
void	finish		(const glw::Functions& gl);

} // gles2
} // egl
} // deqp

#endif // _TEGLGLES2RENDERUTIL_HPP
