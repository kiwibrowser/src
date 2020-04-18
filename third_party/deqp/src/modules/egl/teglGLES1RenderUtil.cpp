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
 * \brief GLES1 render utils.
 *//*--------------------------------------------------------------------*/

#include "teglGLES1RenderUtil.hpp"

#if defined(DEQP_SUPPORT_GLES1)
#	include <GLES/gl.h>
#endif

using std::vector;

namespace deqp
{
namespace egl
{
namespace gles1
{

#if defined(DEQP_SUPPORT_GLES1)

void clear (int x, int y, int width, int height, const tcu::Vec4& color)
{
	glEnable(GL_SCISSOR_TEST);
	glScissor(x, y, width, height);
	glClearColor(color.x(), color.y(), color.z(), color.w());
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
}

void readPixels (tcu::Surface& dst, int x, int y, int width, int height)
{
	dst.setSize(width, height);
	glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dst.getAccess().getDataPtr());
}

void finish (void)
{
	glFinish();
}

#else // DEQP_SUPPORT_GLES1

void clear (int x, int y, int width, int height, const tcu::Vec4& color)
{
	DE_UNREF(x && y && width && height);
	DE_UNREF(color);
	TCU_THROW(NotSupportedError, "OpenGL ES 1.x is not supported");
}

void readPixels (tcu::Surface& dst, int x, int y, int width, int height)
{
	DE_UNREF(x && y && width && height);
	DE_UNREF(dst);
	TCU_THROW(NotSupportedError, "OpenGL ES 1.x is not supported");
}

void finish (void)
{
	TCU_THROW(NotSupportedError, "OpenGL ES 1.x is not supported");
}

#endif // DEQP_SUPPORT_GLES1

} // gles1
} // egl
} // deqp
