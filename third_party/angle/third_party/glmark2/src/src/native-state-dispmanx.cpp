/*
 * Copyright © 2013 Rafal Mielniczuk
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Rafal Mielniczuk <rafal.mielniczuk2@gmail.com>
 */

#include "native-state-dispmanx.h"
#include "log.h"

#include <cstring>
#include <csignal>

NativeStateDispmanx::NativeStateDispmanx()
{
    memset(&properties_, 0, sizeof(properties_));
    memset(&egl_dispmanx_window, 0, sizeof(egl_dispmanx_window));
}

NativeStateDispmanx::~NativeStateDispmanx()
{
}

bool
NativeStateDispmanx::init_display()
{
    bcm_host_init();

    return true;
}

void*
NativeStateDispmanx::display()
{
    return EGL_DEFAULT_DISPLAY;
}

bool
NativeStateDispmanx::create_window(WindowProperties const& properties)
{
    int dispmanx_output = 0; /* LCD */

    if (!properties.fullscreen) {
        Log::error("Error: Dispmanx only supports full screen rendering.\n");
	return false;
    }

    unsigned screen_width, screen_height;
    if (graphics_get_display_size(dispmanx_output,
				  &screen_width, &screen_height) < 0) {
        Log::error("Error: Couldn't get screen width/height.\n");
	return false;
    }

    properties_.fullscreen = properties.fullscreen;
    properties_.visual_id = properties.visual_id;
    properties_.width = screen_width;
    properties_.height = screen_height;

    dispmanx_display = vc_dispmanx_display_open(dispmanx_output);

    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = screen_width;
    dst_rect.height = screen_height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = screen_width << 16;
    src_rect.height = screen_height << 16;

    dispmanx_update = vc_dispmanx_update_start(0);

    dispmanx_element = vc_dispmanx_element_add(dispmanx_update,
					       dispmanx_display,
					       0 /*layer*/,
					       &dst_rect,
					       0 /*src*/,
					       &src_rect,
					       DISPMANX_PROTECTION_NONE,
					       0 /*alpha*/,
					       0 /*clamp*/,
					       DISPMANX_NO_ROTATE);

    egl_dispmanx_window.element = dispmanx_element;
    egl_dispmanx_window.width = dst_rect.width;
    egl_dispmanx_window.height = dst_rect.height;
    vc_dispmanx_update_submit_sync(dispmanx_update);

    return true;
}

void*
NativeStateDispmanx::window(WindowProperties &properties)
{
    properties = properties_;
    return &egl_dispmanx_window;
}

void
NativeStateDispmanx::visible(bool /*v*/)
{
}

bool
NativeStateDispmanx::should_quit()
{
    return false;
}

void
NativeStateDispmanx::flip()
{
}

