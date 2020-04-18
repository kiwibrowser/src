/*
 * Copyright © 2010-2011 Linaro Limited
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
 *  Alexandros Frantzis (glmark2)
 */
#ifndef GLMARK2_GL_HEADERS_H_
#define GLMARK2_GL_HEADERS_H_

#define GL_GLEXT_PROTOTYPES

#if GLMARK2_USE_GL
#include <glad/gl.h>
#ifndef GL_RGB565
#define GL_RGB565 0x8D62
#endif
#elif GLMARK2_USE_GLESv2
#include <glad/gles2.h>
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY GL_WRITE_ONLY_OES
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
#endif
#ifndef GL_DEPTH_COMPONENT32
#define GL_DEPTH_COMPONENT32 GL_DEPTH_COMPONENT32_OES
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 GL_RGBA8_OES
#endif
#ifndef GL_RGB8
#define GL_RGB8 GL_RGB8_OES
#endif
#endif

#include <string>

/**
 * Struct that holds pointers to functions that belong to extensions
 * in either GL2.0 or GLES2.0.
 */
struct GLExtensions {
    /**
     * Whether the current context has support for a GL extension.
     *
     * @return true if the extension is supported
     */
    static bool support(const std::string &ext);

    static void* (GLAD_API_PTR *MapBuffer) (GLenum target, GLenum access);
    static GLboolean (GLAD_API_PTR *UnmapBuffer) (GLenum target);
};

#endif
