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

#include "teglVGRenderUtil.hpp"

#if defined(DEQP_SUPPORT_VG)
#	include <VG/openvg.h>
#endif

using std::vector;

namespace deqp
{
namespace egl
{
namespace vg
{

#if defined(DEQP_SUPPORT_VG)

void clear (int x, int y, int width, int height, const tcu::Vec4& color)
{
	vgSetfv(VG_CLEAR_COLOR, 4, color.getPtr());
	vgClear(x, y, width, height);
	TCU_CHECK(vgGetError() == VG_NO_ERROR);
}

void readPixels (tcu::Surface& dst, int x, int y, int width, int height)
{
	dst.setSize(width, height);
	vgReadPixels(dst.getAccess().getDataPtr(), width*4, VG_sRGBA_8888, 0, 0, width, height);
}

void finish (void)
{
	vgFinish();
}

#else // DEQP_SUPPORT_VG

void clear (int x, int y, int width, int height, const tcu::Vec4& color)
{
	DE_UNREF(x && y && width && height);
	DE_UNREF(color);
	TCU_THROW(NotSupportedError, "OpenVG is not supported");
}

void readPixels (tcu::Surface& dst, int x, int y, int width, int height)
{
	DE_UNREF(x && y && width && height);
	DE_UNREF(dst);
	TCU_THROW(NotSupportedError, "OpenVG is not supported");
}

void finish (void)
{
	TCU_THROW(NotSupportedError, "OpenVG is not supported");
}

#endif // DEQP_SUPPORT_VG

} // vg
} // egl
} // deqp
