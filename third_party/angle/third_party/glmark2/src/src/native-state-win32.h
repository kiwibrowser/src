/*
 * Copyright © 2019 Jamie Madill
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
 *  Jamie Madill
 */
#ifndef GLMARK2_NATIVE_STATE_WIN32_H_
#define GLMARK2_NATIVE_STATE_WIN32_H_

#include "native-state.h"

#include <windows.h>

class NativeStateWin32 : public NativeState
{
public:
    NativeStateWin32();
    ~NativeStateWin32();

    bool init_display();
    void* display();
    bool create_window(WindowProperties const& properties);
    void* window(WindowProperties& properties);
    void visible(bool v);
    bool should_quit();
    void flip();

private:
    void cleanup();
    void pump_messages();

    static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND parent_window_;
    HWND child_window_;
    HDC native_display_;
    WindowProperties properties_;
    bool should_quit_;
};

#endif /* GLMARK2_NATIVE_STATE_H_ */
