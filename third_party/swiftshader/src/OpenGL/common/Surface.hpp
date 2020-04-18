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

// Surface.hpp: Defines the gl::Surface class, which is the abstract interface
// for an EGL surface as viewed by the GL implementation.

#ifndef INCLUDE_SURFACE_H_
#define INCLUDE_SURFACE_H_

#include "Renderer/Surface.hpp"

#include <EGL/egl.h>

namespace egl
{
class Texture;
class Image;
}

namespace gl
{
class [[clang::lto_visibility_public]] Surface
{
protected:
	Surface();
	virtual ~Surface() = 0;

public:
	virtual egl::Image *getRenderTarget() = 0;
	virtual egl::Image *getDepthStencil() = 0;

	virtual EGLint getWidth() const = 0;
	virtual EGLint getHeight() const = 0;
	virtual EGLenum getTextureTarget() const = 0;

	virtual void setBoundTexture(egl::Texture *texture) = 0;
};
}

#endif   // INCLUDE_SURFACE_H_
