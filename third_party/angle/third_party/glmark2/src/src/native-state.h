/*
 * Copyright © 2013 Canonical Ltd
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
 *  Alexandros Frantzis
 */
#ifndef GLMARK2_NATIVE_STATE_H_
#define GLMARK2_NATIVE_STATE_H_

class NativeState
{
public:
    struct WindowProperties
    {
        WindowProperties(int w, int h, bool f, int v)
            : width(w), height(h), fullscreen(f), visual_id(v) {}
        WindowProperties()
            : width(0), height(0), fullscreen(false), visual_id(0) {}

        int width;
        int height;
        bool fullscreen;
        int visual_id;
    };

    virtual ~NativeState() {}

    /* Initializes the native display */
    virtual bool init_display() = 0;

    /* Gets the native display */
    virtual void* display() = 0;

    /* Creates (or recreates) the native window */
    virtual bool create_window(WindowProperties const& properties) = 0;

    /* 
     * Gets the native window and its properties.
     * The dimensions may be different than the ones requested.
     */
    virtual void* window(WindowProperties& properties) = 0;

    /* Sets the visibility of the native window */
    virtual void visible(bool v) = 0;

    /* Whether the user has requested an exit */
    virtual bool should_quit() = 0;

    /* Flips the display */
    virtual void flip() = 0;
};

#endif /* GLMARK2_NATIVE_STATE_H_ */
