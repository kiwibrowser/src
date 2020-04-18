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


#include "teglImageUtil.hpp"

#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"

#include "egluGLUtil.hpp"
#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "glwEnums.hpp"

#include "gluObjectWrapper.hpp"
#include "gluTextureUtil.hpp"

namespace deqp
{
namespace egl
{
namespace Image
{

using std::string;
using std::vector;

using de::UniquePtr;
using de::MovePtr;

using tcu::TextureFormat;
using tcu::Texture2D;
using tcu::Vec4;

using glu::Framebuffer;
using glu::Texture;

using eglu::AttribMap;
using eglu::UniqueSurface;
using eglu::NativeDisplay;
using eglu::NativeWindow;
using eglu::NativePixmap;
using eglu::NativeDisplayFactory;
using eglu::NativeWindowFactory;
using eglu::NativePixmapFactory;
using eglu::WindowParams;

using namespace glw;
using namespace eglw;

enum {
	IMAGE_WIDTH		= 64,
	IMAGE_HEIGHT	= 64,
};


template <typename T>
struct NativeSurface : public ManagedSurface
{
public:
	explicit		NativeSurface	(MovePtr<UniqueSurface>	surface,
									 MovePtr<T>				native)
						: ManagedSurface	(surface)
						, m_native			(native) {}

private:
	UniquePtr<T>	m_native;
};

typedef NativeSurface<NativeWindow> NativeWindowSurface;
typedef NativeSurface<NativePixmap> NativePixmapSurface;

MovePtr<ManagedSurface> createSurface (EglTestContext& eglTestCtx, EGLDisplay dpy, EGLConfig config, int width, int height)
{
	const Library&				egl				= eglTestCtx.getLibrary();
	EGLint						surfaceTypeBits	= eglu::getConfigAttribInt(egl, dpy, config, EGL_SURFACE_TYPE);
	const NativeDisplayFactory&	displayFactory	= eglTestCtx.getNativeDisplayFactory();
	NativeDisplay&				nativeDisplay	= eglTestCtx.getNativeDisplay();

	if (surfaceTypeBits & EGL_PBUFFER_BIT)
	{
		static const EGLint attribs[]	= { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };
		const EGLSurface	surface		= egl.createPbufferSurface(dpy, config, attribs);

		EGLU_CHECK_MSG(egl, "eglCreatePbufferSurface()");

		return de::newMovePtr<ManagedSurface>(MovePtr<UniqueSurface>(new UniqueSurface(egl, dpy, surface)));
	}
	else if (surfaceTypeBits & EGL_WINDOW_BIT)
	{
		const NativeWindowFactory&	windowFactory	= selectNativeWindowFactory(displayFactory, eglTestCtx.getTestContext().getCommandLine());

		MovePtr<NativeWindow>		window	(windowFactory.createWindow(&nativeDisplay, dpy, config, DE_NULL, WindowParams(width, height, WindowParams::VISIBILITY_DONT_CARE)));
		const EGLSurface			surface	= eglu::createWindowSurface(nativeDisplay, *window, dpy, config, DE_NULL);

		return MovePtr<ManagedSurface>(new NativeWindowSurface(MovePtr<UniqueSurface>(new UniqueSurface(egl, dpy, surface)), window));
	}
	else if (surfaceTypeBits & EGL_PIXMAP_BIT)
	{
		const NativePixmapFactory&	pixmapFactory	= selectNativePixmapFactory(displayFactory, eglTestCtx.getTestContext().getCommandLine());

		MovePtr<NativePixmap>	pixmap	(pixmapFactory.createPixmap(&nativeDisplay, dpy, config, DE_NULL, width, height));
		const EGLSurface		surface	= eglu::createPixmapSurface(eglTestCtx.getNativeDisplay(), *pixmap, dpy, config, DE_NULL);

		return MovePtr<ManagedSurface>(new NativePixmapSurface(MovePtr<UniqueSurface>(new UniqueSurface(egl, dpy, surface)), pixmap));
	}
	else
		TCU_FAIL("No valid surface types supported in config");
}

class GLClientBuffer : public ClientBuffer
{
	EGLClientBuffer	get		(void) const { return reinterpret_cast<EGLClientBuffer>(static_cast<deUintptr>(getName())); }

protected:
	virtual GLuint	getName	(void) const = 0;
};

class TextureClientBuffer : public GLClientBuffer
{
public:
						TextureClientBuffer	(const glw::Functions& gl) : m_texture (gl) {}
	GLuint				getName				(void) const { return *m_texture; }

private:
	glu::Texture		m_texture;
};

class GLImageSource : public ImageSource
{
public:
	EGLImageKHR			createImage			(const Library& egl, EGLDisplay dpy, EGLContext ctx, EGLClientBuffer clientBuffer) const;

protected:
	virtual AttribMap	getCreateAttribs	(void) const = 0;
	virtual EGLenum		getSource			(void) const = 0;
};

EGLImageKHR GLImageSource::createImage (const Library& egl, EGLDisplay dpy, EGLContext ctx, EGLClientBuffer clientBuffer) const
{
	AttribMap				attribMap	= getCreateAttribs();

	attribMap[EGL_IMAGE_PRESERVED_KHR] = EGL_TRUE;

	{
		const vector<EGLint>	attribs	= eglu::attribMapToList(attribMap);
		const EGLImageKHR		image	= egl.createImageKHR(dpy, ctx, getSource(),
															 clientBuffer, &attribs.front());
		EGLU_CHECK_MSG(egl, "eglCreateImageKHR()");
		return image;
	}
}

class TextureImageSource : public GLImageSource
{
public:
							TextureImageSource	(GLenum internalFormat, GLenum format, GLenum type, bool useTexLevel0) : m_internalFormat(internalFormat), m_format(format), m_type(type), m_useTexLevel0(useTexLevel0) {}
	MovePtr<ClientBuffer>	createBuffer		(const glw::Functions& gl, Texture2D* reference) const;
	GLenum					getEffectiveFormat	(void) const;
	GLenum					getInternalFormat	(void) const { return m_internalFormat; }

protected:
	AttribMap				getCreateAttribs	(void) const;
	virtual void			initTexture			(const glw::Functions& gl) const = 0;
	virtual GLenum			getGLTarget			(void) const = 0;

	const GLenum			m_internalFormat;
	const GLenum			m_format;
	const GLenum			m_type;
	const bool				m_useTexLevel0;
};

bool isSizedFormat (GLenum format)
{
	try
	{
		glu::mapGLInternalFormat(format);
		return true;
	}
	catch (const tcu::InternalError&)
	{
		return false;
	}
}

GLenum getEffectiveFormat (GLenum format, GLenum type)
{
	return glu::getInternalFormat(glu::mapGLTransferFormat(format, type));
}

GLenum TextureImageSource::getEffectiveFormat (void) const
{
	if (isSizedFormat(m_internalFormat))
		return m_internalFormat;
	else
		return deqp::egl::Image::getEffectiveFormat(m_format, m_type);
}

AttribMap TextureImageSource::getCreateAttribs (void) const
{
	AttribMap ret;

	ret[EGL_GL_TEXTURE_LEVEL_KHR] = 0;

	return ret;
}

MovePtr<ClientBuffer> TextureImageSource::createBuffer (const glw::Functions& gl, Texture2D* ref) const
{
	MovePtr<TextureClientBuffer>	clientBuffer	(new TextureClientBuffer(gl));
	const GLuint					texture			= clientBuffer->getName();
	const GLenum					target			= getGLTarget();

	GLU_CHECK_GLW_CALL(gl, bindTexture(target, texture));
	initTexture(gl);

	if (!m_useTexLevel0)
	{
		// Set minification filter to linear. This makes the texture complete.
		GLU_CHECK_GLW_CALL(gl, texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	}

	if (ref != DE_NULL)
	{
		GLenum		imgTarget	= eglu::getImageGLTarget(getSource());

		*ref = Texture2D(glu::mapGLTransferFormat(m_format, m_type), IMAGE_WIDTH, IMAGE_HEIGHT);
		ref->allocLevel(0);
		tcu::fillWithComponentGradients(ref->getLevel(0),
										tcu::Vec4(0.0f, 0.0f, 0.0f, 0.0f),
										tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

		GLU_CHECK_GLW_CALL(gl, texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLU_CHECK_GLW_CALL(gl, texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLU_CHECK_GLW_CALL(gl, texParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLU_CHECK_GLW_CALL(gl, texParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));

		GLU_CHECK_GLW_CALL(gl, texImage2D(imgTarget, 0, m_internalFormat, IMAGE_WIDTH, IMAGE_HEIGHT,
										  0, m_format, m_type, ref->getLevel(0).getDataPtr()));
	}
	GLU_CHECK_GLW_CALL(gl, bindTexture(target, 0));
	return MovePtr<ClientBuffer>(clientBuffer);
}

class Texture2DImageSource : public TextureImageSource
{
public:
					Texture2DImageSource	(GLenum internalFormat, GLenum format, GLenum type, bool useTexLevel0) : TextureImageSource(internalFormat, format, type, useTexLevel0) {}
	EGLenum			getSource				(void) const { return EGL_GL_TEXTURE_2D_KHR; }
	string			getRequiredExtension	(void) const { return "EGL_KHR_gl_texture_2D_image"; }
	GLenum			getGLTarget				(void) const { return GL_TEXTURE_2D; }

protected:
	void			initTexture				(const glw::Functions& gl) const;
};

void Texture2DImageSource::initTexture (const glw::Functions& gl) const
{
	// Specify mipmap level 0
	GLU_CHECK_CALL_ERROR(gl.texImage2D(GL_TEXTURE_2D, 0, m_internalFormat, IMAGE_WIDTH, IMAGE_HEIGHT, 0, m_format, m_type, DE_NULL),
						 gl.getError());
}

class TextureCubeMapImageSource : public TextureImageSource
{
public:
					TextureCubeMapImageSource	(EGLenum source, GLenum internalFormat, GLenum format, GLenum type, bool useTexLevel0) : TextureImageSource(internalFormat, format, type, useTexLevel0), m_source(source) {}
	EGLenum			getSource					(void) const { return m_source; }
	string			getRequiredExtension		(void) const { return "EGL_KHR_gl_texture_cubemap_image"; }
	GLenum			getGLTarget					(void) const { return GL_TEXTURE_CUBE_MAP; }

protected:
	void			initTexture					(const glw::Functions& gl) const;

	EGLenum			m_source;
};

void TextureCubeMapImageSource::initTexture (const glw::Functions& gl) const
{
	// Specify mipmap level 0 for all faces
	static const GLenum faces[] =
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	for (int faceNdx = 0; faceNdx < DE_LENGTH_OF_ARRAY(faces); faceNdx++)
		GLU_CHECK_GLW_CALL(gl, texImage2D(faces[faceNdx], 0, m_internalFormat, IMAGE_WIDTH, IMAGE_HEIGHT, 0, m_format, m_type, DE_NULL));
}

class RenderbufferClientBuffer : public GLClientBuffer
{
public:
						RenderbufferClientBuffer	(const glw::Functions& gl) : m_rbo (gl) {}
	GLuint				getName						(void) const { return *m_rbo; }

private:
	glu::Renderbuffer	m_rbo;
};

class RenderbufferImageSource : public GLImageSource
{
public:
							RenderbufferImageSource	(GLenum format) : m_format(format) {}

	string					getRequiredExtension	(void) const	{ return "EGL_KHR_gl_renderbuffer_image"; }
	MovePtr<ClientBuffer>	createBuffer			(const glw::Functions& gl, Texture2D* reference) const;
	GLenum					getEffectiveFormat		(void) const { return m_format; }

protected:
	EGLenum					getSource				(void) const	{ return EGL_GL_RENDERBUFFER_KHR; }
	AttribMap				getCreateAttribs		(void) const	{ return AttribMap(); }

	GLenum					m_format;
};

void initializeStencilRbo(const glw::Functions& gl, GLuint rbo, Texture2D& ref)
{
	static const deUint32 stencilValues[] =
	{
		0xBF688C11u,
		0xB43D2922u,
		0x055D5FFBu,
		0x9300655Eu,
		0x63BE0DF2u,
		0x0345C13Bu,
		0x1C184832u,
		0xD107040Fu,
		0x9B91569Fu,
		0x0F0CFDC7u,
	};

	const deUint32 numStencilBits	= tcu::getTextureFormatBitDepth(tcu::getEffectiveDepthStencilTextureFormat(ref.getLevel(0).getFormat(), tcu::Sampler::MODE_STENCIL)).x();
	const deUint32 stencilMask		= deBitMask32(0, numStencilBits);

	GLU_CHECK_GLW_CALL(gl, framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
												   GL_RENDERBUFFER, rbo));
	GLU_CHECK_GLW_CALL(gl, clearStencil(0));
	GLU_CHECK_GLW_CALL(gl, clear(GL_STENCIL_BUFFER_BIT));
	tcu::clearStencil(ref.getLevel(0), 0);

	// create a pattern
	GLU_CHECK_GLW_CALL(gl, enable(GL_SCISSOR_TEST));
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(stencilValues); ++ndx)
	{
		const deUint32		stencil	= stencilValues[ndx] & stencilMask;
		const tcu::IVec2	size	= tcu::IVec2((int)((float)(DE_LENGTH_OF_ARRAY(stencilValues) - ndx) * ((float)ref.getWidth() / float(DE_LENGTH_OF_ARRAY(stencilValues)))),
												 (int)((float)(DE_LENGTH_OF_ARRAY(stencilValues) - ndx) * ((float)ref.getHeight() / float(DE_LENGTH_OF_ARRAY(stencilValues) + 4)))); // not symmetric

		if (size.x() == 0 || size.y() == 0)
			break;

		GLU_CHECK_GLW_CALL(gl, scissor(0, 0, size.x(), size.y()));
		GLU_CHECK_GLW_CALL(gl, clearStencil(stencil));
		GLU_CHECK_GLW_CALL(gl, clear(GL_STENCIL_BUFFER_BIT));

		tcu::clearStencil(tcu::getSubregion(ref.getLevel(0), 0, 0, size.x(), size.y()), stencil);
	}

	GLU_CHECK_GLW_CALL(gl, disable(GL_SCISSOR_TEST));
	GLU_CHECK_GLW_CALL(gl, framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
												   GL_RENDERBUFFER, 0));
}

void initializeDepthRbo(const glw::Functions& gl, GLuint rbo, Texture2D& ref)
{
	const int NUM_STEPS = 13;

	GLU_CHECK_GLW_CALL(gl, framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
												   GL_RENDERBUFFER, rbo));

	GLU_CHECK_GLW_CALL(gl, clearDepthf(0.0f));
	GLU_CHECK_GLW_CALL(gl, clear(GL_DEPTH_BUFFER_BIT));
	tcu::clearDepth(ref.getLevel(0), 0.0f);

	// create a pattern
	GLU_CHECK_GLW_CALL(gl, enable(GL_SCISSOR_TEST));
	for (int ndx = 0; ndx < NUM_STEPS; ++ndx)
	{
		const float			depth	= (float)ndx / float(NUM_STEPS);
		const tcu::IVec2	size	= tcu::IVec2((int)((float)(NUM_STEPS - ndx) * ((float)ref.getWidth() / float(NUM_STEPS))),
												 (int)((float)(NUM_STEPS - ndx) * ((float)ref.getHeight() / float(NUM_STEPS + 4)))); // not symmetric

		if (size.x() == 0 || size.y() == 0)
			break;

		GLU_CHECK_GLW_CALL(gl, scissor(0, 0, size.x(), size.y()));
		GLU_CHECK_GLW_CALL(gl, clearDepthf(depth));
		GLU_CHECK_GLW_CALL(gl, clear(GL_DEPTH_BUFFER_BIT));

		tcu::clearDepth(tcu::getSubregion(ref.getLevel(0), 0, 0, size.x(), size.y()), depth);
	}

	GLU_CHECK_GLW_CALL(gl, disable(GL_SCISSOR_TEST));
	GLU_CHECK_GLW_CALL(gl, framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
												   GL_RENDERBUFFER, 0));

}

void initializeColorRbo(const glw::Functions& gl, GLuint rbo, Texture2D& ref)
{
	static const tcu::Vec4 colorValues[] =
	{
		tcu::Vec4(0.9f, 0.5f, 0.65f, 1.0f),
		tcu::Vec4(0.5f, 0.7f, 0.65f, 1.0f),
		tcu::Vec4(0.2f, 0.5f, 0.65f, 1.0f),
		tcu::Vec4(0.3f, 0.1f, 0.5f, 1.0f),
		tcu::Vec4(0.8f, 0.2f, 0.3f, 1.0f),
		tcu::Vec4(0.9f, 0.4f, 0.8f, 1.0f),
	};

	GLU_CHECK_GLW_CALL(gl, framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
												   GL_RENDERBUFFER, rbo));
	GLU_CHECK_GLW_CALL(gl, clearColor(1.0f, 1.0f, 0.0f, 1.0f));
	GLU_CHECK_GLW_CALL(gl, clear(GL_COLOR_BUFFER_BIT));
	tcu::clear(ref.getLevel(0), Vec4(1.0f, 1.0f, 0.0f, 1.0f));

	// create a pattern
	GLU_CHECK_GLW_CALL(gl, enable(GL_SCISSOR_TEST));
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(colorValues); ++ndx)
	{
		const tcu::IVec2	size	= tcu::IVec2((int)((float)(DE_LENGTH_OF_ARRAY(colorValues) - ndx) * ((float)ref.getWidth() / float(DE_LENGTH_OF_ARRAY(colorValues)))),
												 (int)((float)(DE_LENGTH_OF_ARRAY(colorValues) - ndx) * ((float)ref.getHeight() / float(DE_LENGTH_OF_ARRAY(colorValues) + 4)))); // not symmetric

		if (size.x() == 0 || size.y() == 0)
			break;

		GLU_CHECK_GLW_CALL(gl, scissor(0, 0, size.x(), size.y()));
		GLU_CHECK_GLW_CALL(gl, clearColor(colorValues[ndx].x(), colorValues[ndx].y(), colorValues[ndx].z(), colorValues[ndx].w()));
		GLU_CHECK_GLW_CALL(gl, clear(GL_COLOR_BUFFER_BIT));

		tcu::clear(tcu::getSubregion(ref.getLevel(0), 0, 0, size.x(), size.y()), colorValues[ndx]);
	}

	GLU_CHECK_GLW_CALL(gl, disable(GL_SCISSOR_TEST));
	GLU_CHECK_GLW_CALL(gl, framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
												   GL_RENDERBUFFER, 0));
}

MovePtr<ClientBuffer> RenderbufferImageSource::createBuffer (const glw::Functions& gl, Texture2D* ref) const
{
	MovePtr<RenderbufferClientBuffer>	buffer	(new RenderbufferClientBuffer(gl));
	const GLuint						rbo		= buffer->getName();

	GLU_CHECK_CALL_ERROR(gl.bindRenderbuffer(GL_RENDERBUFFER, rbo), gl.getError());

	// Specify storage.
	GLU_CHECK_CALL_ERROR(gl.renderbufferStorage(GL_RENDERBUFFER, m_format, 64, 64), gl.getError());

	if (ref != DE_NULL)
	{
		Framebuffer			fbo			(gl);
		const TextureFormat	texFormat	= glu::mapGLInternalFormat(m_format);

		*ref = tcu::Texture2D(texFormat, 64, 64);
		ref->allocLevel(0);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
		switch (m_format)
		{
			case GL_STENCIL_INDEX8:
				initializeStencilRbo(gl, rbo, *ref);
				break;
			case GL_DEPTH_COMPONENT16:
				initializeDepthRbo(gl, rbo, *ref);
				break;
			case GL_RGBA4:
				initializeColorRbo(gl, rbo, *ref);
				break;
			case GL_RGB5_A1:
				initializeColorRbo(gl, rbo, *ref);
				break;
			case GL_RGB565:
				initializeColorRbo(gl, rbo, *ref);
				break;
			default:
				DE_FATAL("Impossible");
		}

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	return MovePtr<ClientBuffer>(buffer);
}

class UnsupportedImageSource : public ImageSource
{
public:
							UnsupportedImageSource	(const string& message, GLenum format) : m_message(message), m_format(format) {}
	string					getRequiredExtension	(void) const { fail(); return ""; }
	MovePtr<ClientBuffer>	createBuffer			(const glw::Functions&, tcu::Texture2D*) const { fail(); return de::MovePtr<ClientBuffer>(); }
	EGLImageKHR				createImage				(const Library& egl, EGLDisplay dpy, EGLContext ctx, EGLClientBuffer clientBuffer) const;
	GLenum					getEffectiveFormat		(void) const { return m_format; }

private:
	const string			m_message;
	GLenum					m_format;

	void					fail					(void) const { TCU_THROW(NotSupportedError, m_message.c_str()); }
};

EGLImageKHR	UnsupportedImageSource::createImage (const Library&, EGLDisplay, EGLContext, EGLClientBuffer) const
{
	fail();
	return EGL_NO_IMAGE_KHR;
}

MovePtr<ImageSource> createTextureImageSource (EGLenum source, GLenum internalFormat, GLenum format, GLenum type, bool useTexLevel0)
{
	if (source == EGL_GL_TEXTURE_2D_KHR)
		return MovePtr<ImageSource>(new Texture2DImageSource(internalFormat, format, type, useTexLevel0));
	else
		return MovePtr<ImageSource>(new TextureCubeMapImageSource(source, internalFormat, format, type, useTexLevel0));
}

MovePtr<ImageSource> createRenderbufferImageSource (GLenum format)
{
	return MovePtr<ImageSource>(new RenderbufferImageSource(format));
}

MovePtr<ImageSource> createUnsupportedImageSource (const string& message, GLenum format)
{
	return MovePtr<ImageSource>(new UnsupportedImageSource(message, format));
}

} // Image
} // egl
} // deqp
