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
#ifndef GLMARK2_GL_STATE_H_
#define GLMARK2_GL_STATE_H_

class GLVisualConfig;

class GLState
{
public:
    virtual ~GLState() {}

    virtual bool init_display(void *native_display, GLVisualConfig& config_pref) = 0;
    virtual bool init_surface(void *native_window) = 0;
    virtual bool init_gl_extensions() = 0;
    virtual bool valid() = 0;
    virtual bool reset() = 0;
    virtual void swap() = 0;
    virtual bool gotNativeConfig(int& vid) = 0;
    virtual void getVisualConfig(GLVisualConfig& vc) = 0;
};

#endif /* GLMARK2_GL_STATE_H_ */
