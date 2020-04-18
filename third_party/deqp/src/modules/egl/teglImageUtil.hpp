#ifndef _TEGLIMAGEUTIL_HPP
#define _TEGLIMAGEUTIL_HPP
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
 * \brief Common utilities for EGL images.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuTexture.hpp"

#include "deUniquePtr.hpp"

#include "teglTestCase.hpp"

#include "egluUtil.hpp"
#include "egluUnique.hpp"

#include "glwDefs.hpp"

namespace eglw
{
class Library;
}

namespace deqp
{
namespace egl
{
namespace Image
{

class ManagedSurface
{
public:
										ManagedSurface	(de::MovePtr<eglu::UniqueSurface> surface) : m_surface(surface) {}
	virtual								~ManagedSurface	(void) {}
	eglw::EGLSurface					get				(void) const { return **m_surface; }

private:
	de::UniquePtr<eglu::UniqueSurface>	m_surface;
};

de::MovePtr<ManagedSurface> createSurface (EglTestContext& eglTestCtx, eglw::EGLDisplay display, eglw::EGLConfig config, int width, int height);

class ClientBuffer
{
public:
	virtual							~ClientBuffer	(void) {}
	virtual eglw::EGLClientBuffer	get				(void) const = 0;
};

class ImageSource
{
public:
	virtual								~ImageSource		(void) {}
	virtual std::string					getRequiredExtension(void) const = 0;
	virtual de::MovePtr<ClientBuffer>	createBuffer		(const glw::Functions& gl, tcu::Texture2D* reference = DE_NULL) const = 0;
	virtual eglw::EGLImageKHR			createImage			(const eglw::Library& egl, eglw::EGLDisplay dpy, eglw::EGLContext ctx, eglw::EGLClientBuffer clientBuffer) const = 0;
	virtual glw::GLenum					getEffectiveFormat	(void) const = 0;
};

de::MovePtr<ImageSource> createTextureImageSource			(eglw::EGLenum source, glw::GLenum internalFormat, glw::GLenum format, glw::GLenum type, bool useTexLevel0 = false);
de::MovePtr<ImageSource> createRenderbufferImageSource		(glw::GLenum format);
de::MovePtr<ImageSource> createUnsupportedImageSource		(const std::string& message, glw::GLenum format);

} // Image
} // egl
} // deqp


#endif // _TEGLIMAGEUTIL_HPP
