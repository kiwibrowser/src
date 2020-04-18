// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef egl_Context_hpp
#define egl_Context_hpp

#include "common/Object.hpp"
#include "Renderer/Surface.hpp"

#include <EGL/egl.h>
#include <GLES/gl.h>

namespace gl { class Surface; }

namespace egl
{
class Display;
class Image;

class [[clang::lto_visibility_public]] Context : public gl::Object
{
public:
	virtual void makeCurrent(gl::Surface *surface) = 0;
	virtual void bindTexImage(gl::Surface *surface) = 0;
	virtual EGLenum validateSharedImage(EGLenum target, GLuint name, GLuint textureLevel) = 0;
	virtual Image *createSharedImage(EGLenum target, GLuint name, GLuint textureLevel) = 0;
	virtual EGLint getClientVersion() const = 0;
	virtual EGLint getConfigID() const = 0;
	virtual void finish() = 0;
	virtual void blit(sw::Surface *source, const sw::SliceRect &sRect, sw::Surface *dest, const sw::SliceRect &dRect) = 0;

	Display *getDisplay() const { return display; }

protected:
	Context(egl::Display *display) : display(display) {}
	virtual ~Context() {};

	egl::Display *const display;
};
}

#endif   // egl_Context_hpp
