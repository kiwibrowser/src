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
 * \brief Android-specific operations.
 *//*--------------------------------------------------------------------*/

#include "teglAndroidUtil.hpp"

#include "deStringUtil.hpp"
#include "tcuTextureUtil.hpp"
#include "gluTextureUtil.hpp"
#include "glwEnums.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#if (DE_OS == DE_OS_ANDROID)
#	include "tcuAndroidInternals.hpp"
#endif

namespace deqp
{
namespace egl
{
namespace Image
{
using std::string;
using de::MovePtr;
using tcu::PixelBufferAccess;
using tcu::TextureFormat;
using tcu::Texture2D;
using eglu::AttribMap;
using namespace glw;
using namespace eglw;

#if (DE_OS != DE_OS_ANDROID)

MovePtr<ImageSource> createAndroidNativeImageSource	(GLenum format)
{
	return createUnsupportedImageSource("Not Android platform", format);
}

#else // DE_OS == DE_OS_ANDROID

using tcu::Android::internal::LibUI;
using tcu::Android::internal::GraphicBuffer;
using tcu::Android::internal::PixelFormat;
using tcu::Android::internal::status_t;

PixelFormat getPixelFormat (GLenum format)
{
	switch (format)
	{
		case GL_RGB565:		return tcu::Android::internal::PIXEL_FORMAT_RGB_565;
		case GL_RGB8:		return tcu::Android::internal::PIXEL_FORMAT_RGB_888;
		case GL_RGBA4:		return tcu::Android::internal::PIXEL_FORMAT_RGBA_4444;
		case GL_RGB5_A1:	return tcu::Android::internal::PIXEL_FORMAT_RGBA_5551;
		case GL_RGBA8:		return tcu::Android::internal::PIXEL_FORMAT_RGBA_8888;
		default:			TCU_THROW(NotSupportedError, "Texture format unsupported by Android");
	}
}

class AndroidNativeClientBuffer : public ClientBuffer
{
public:
							AndroidNativeClientBuffer	(const LibUI& lib, GLenum format);
	EGLClientBuffer			get							(void) const { return reinterpret_cast<EGLClientBuffer>(m_windowBuffer); }
	GraphicBuffer&			getGraphicBuffer			(void) { return m_graphicBuffer; }

private:
	GraphicBuffer			m_graphicBuffer;
	ANativeWindowBuffer*	m_windowBuffer;
};

AndroidNativeClientBuffer::AndroidNativeClientBuffer (const LibUI& lib, GLenum format)
	: m_graphicBuffer	(lib, 64, 64, getPixelFormat(format),
						 GraphicBuffer::USAGE_SW_READ_OFTEN		|
						 GraphicBuffer::USAGE_SW_WRITE_RARELY	|
						 GraphicBuffer::USAGE_HW_TEXTURE		|
						 GraphicBuffer::USAGE_HW_RENDER)
	, m_windowBuffer	(m_graphicBuffer.getNativeBuffer())
{
}

class AndroidNativeImageSource : public ImageSource
{
public:
							AndroidNativeImageSource	(GLenum format) : m_format(format), m_libui(DE_NULL) {}
							~AndroidNativeImageSource	(void);
	MovePtr<ClientBuffer>	createBuffer				(const glw::Functions&, Texture2D*) const;
	string					getRequiredExtension		(void) const { return "EGL_ANDROID_image_native_buffer"; }
	EGLImageKHR				createImage					(const Library& egl, EGLDisplay dpy, EGLContext ctx, EGLClientBuffer clientBuffer) const;
	GLenum					getEffectiveFormat			(void) const { return m_format; }

protected:
	GLenum					m_format;

	const LibUI&			getLibUI					(void) const;

private:
	mutable LibUI*			m_libui;
};

AndroidNativeImageSource::~AndroidNativeImageSource (void)
{
	delete m_libui;
}

const LibUI& AndroidNativeImageSource::getLibUI (void) const
{
	if (!m_libui)
		m_libui = new LibUI();

	return *m_libui;
}

void checkStatus (status_t status)
{
	if (status != tcu::Android::internal::OK)
		TCU_FAIL(("Android error: status code " + de::toString(status)).c_str());
}

MovePtr<ClientBuffer> AndroidNativeImageSource::createBuffer (const glw::Functions&, Texture2D* ref) const
{
	MovePtr<AndroidNativeClientBuffer>	buffer			(new AndroidNativeClientBuffer(getLibUI(), m_format));
	GraphicBuffer&						graphicBuffer	= buffer->getGraphicBuffer();
	if (ref != DE_NULL)
	{
		const TextureFormat	texFormat	= glu::mapGLInternalFormat(m_format);
		void*				bufferData	= DE_NULL;

		*ref = Texture2D(texFormat, 64, 64);
		ref->allocLevel(0);
		tcu::fillWithComponentGradients(ref->getLevel(0),
										tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f),
										tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
		checkStatus(graphicBuffer.lock(GraphicBuffer::USAGE_SW_WRITE_RARELY, &bufferData));
		{
			PixelBufferAccess nativeBuffer(texFormat, 64, 64, 1, bufferData);
			tcu::copy(nativeBuffer, ref->getLevel(0));
		}
		checkStatus(graphicBuffer.unlock());
	}
	return MovePtr<ClientBuffer>(buffer);
}

EGLImageKHR AndroidNativeImageSource::createImage (const Library& egl, EGLDisplay dpy, EGLContext, EGLClientBuffer clientBuffer) const
{
	static const EGLint attribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
	const EGLImageKHR	image		= egl.createImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, attribs);

	EGLU_CHECK_MSG(egl, "eglCreateImageKHR()");
	return image;
}

MovePtr<ImageSource> createAndroidNativeImageSource	(GLenum format)
{
	try
	{
		return MovePtr<ImageSource>(new AndroidNativeImageSource(format));
	}
	catch (const std::runtime_error& exc)
	{
		return createUnsupportedImageSource(string("Android native buffers unsupported: ") + exc.what(), format);
	}
}

#endif // DE_OS == DE_OS_ANDROID

} // Image
} // egl
} // deqp
