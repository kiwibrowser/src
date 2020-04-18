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

#include "teglGLES2RenderUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace egl
{
namespace gles2
{

void clear (const glw::Functions& gl, int x, int y, int width, int height, const tcu::Vec4& color)
{
	gl.enable(GL_SCISSOR_TEST);
	gl.scissor(x, y, width, height);
	gl.clearColor(color.x(), color.y(), color.z(), color.w());
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl.disable(GL_SCISSOR_TEST);
}

void readPixels (const glw::Functions& gl, tcu::Surface& dst, int x, int y, int width, int height)
{
	dst.setSize(width, height);
	gl.readPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dst.getAccess().getDataPtr());
}

void finish (const glw::Functions& gl)
{
	gl.finish();
}

} // gles2
} // egl
} // deqp
