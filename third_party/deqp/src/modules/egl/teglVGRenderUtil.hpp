#ifndef _TEGLVGRENDERUTIL_HPP
#define _TEGLVGRENDERUTIL_HPP
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
 * \brief OpenVG render utils.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuSurface.hpp"
#include "tcuVector.hpp"

namespace deqp
{
namespace egl
{
namespace vg
{

void	clear		(int x, int y, int width, int height, const tcu::Vec4& color);
void	readPixels	(tcu::Surface& dst, int x, int y, int width, int height);
void	finish		(void);

} // vg
} // egl
} // deqp

#endif // _TEGLVGRENDERUTIL_HPP
