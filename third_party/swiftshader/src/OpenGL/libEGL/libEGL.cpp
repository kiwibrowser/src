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

// libEGL.cpp: Implements the exported EGL functions.

#include "main.h"
#include "Display.h"
#include "Surface.hpp"
#include "Texture.hpp"
#include "Context.hpp"
#include "common/Image.hpp"
#include "common/debug.h"
#include "Common/Version.h"

#if defined(__ANDROID__)
#include <system/window.h>
#elif defined(__linux__)
#include "Main/libX11.hpp"
#endif

#include <algorithm>
#include <string.h>

using namespace egl;

static bool validateDisplay(egl::Display *display)
{
	if(display == EGL_NO_DISPLAY)
	{
		return error(EGL_BAD_DISPLAY, false);
	}

	if(!display->isInitialized())
	{
		return error(EGL_NOT_INITIALIZED, false);
	}

	return true;
}

static bool validateConfig(egl::Display *display, EGLConfig config)
{
	if(!validateDisplay(display))
	{
		return false;
	}

	if(!display->isValidConfig(config))
	{
		return error(EGL_BAD_CONFIG, false);
	}

	return true;
}

static bool validateContext(egl::Display *display, egl::Context *context)
{
	if(!validateDisplay(display))
	{
		return false;
	}

	if(!display->isValidContext(context))
	{
		return error(EGL_BAD_CONTEXT, false);
	}

	return true;
}

static bool validateSurface(egl::Display *display, egl::Surface *surface)
{
	if(!validateDisplay(display))
	{
		return false;
	}

	if(!display->isValidSurface(surface))
	{
		return error(EGL_BAD_SURFACE, false);
	}

	return true;
}

namespace egl
{
EGLint GetError(void)
{
	TRACE("()");

	EGLint error = egl::getCurrentError();

	if(error != EGL_SUCCESS)
	{
		egl::setCurrentError(EGL_SUCCESS);
	}

	return error;
}

EGLDisplay GetDisplay(EGLNativeDisplayType display_id)
{
	TRACE("(EGLNativeDisplayType display_id = %p)", display_id);

	if(display_id != EGL_DEFAULT_DISPLAY)
	{
		// FIXME: Check if display_id is the default display
	}

	#if defined(__linux__) && !defined(__ANDROID__)
		if(!libX11)
		{
			return success(HEADLESS_DISPLAY);
		}
	#endif

	return success(PRIMARY_DISPLAY);   // We only support the default display
}

EGLBoolean Initialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	TRACE("(EGLDisplay dpy = %p, EGLint *major = %p, EGLint *minor = %p)",
		  dpy, major, minor);

	egl::Display *display = egl::Display::get(dpy);

	if(!display)
	{
		return error(EGL_BAD_DISPLAY, EGL_FALSE);
	}

	if(!display->initialize())
	{
		return error(EGL_NOT_INITIALIZED, EGL_FALSE);
	}

	if(major) *major = 1;
	if(minor) *minor = 4;

	return success(EGL_TRUE);
}

EGLBoolean Terminate(EGLDisplay dpy)
{
	TRACE("(EGLDisplay dpy = %p)", dpy);

	if(dpy == EGL_NO_DISPLAY)
	{
		return error(EGL_BAD_DISPLAY, EGL_FALSE);
	}

	egl::Display *display = egl::Display::get(dpy);

	display->terminate();

	return success(EGL_TRUE);
}

const char *QueryString(EGLDisplay dpy, EGLint name)
{
	TRACE("(EGLDisplay dpy = %p, EGLint name = %d)", dpy, name);

	if(dpy == EGL_NO_DISPLAY && name == EGL_EXTENSIONS)
	{
		return success(
			"EGL_KHR_client_get_all_proc_addresses "
#if defined(__linux__) && !defined(__ANDROID__)
			"EGL_KHR_platform_gbm "
			"EGL_KHR_platform_x11 "
#endif
			"EGL_EXT_client_extensions "
			"EGL_EXT_platform_base");
	}

	egl::Display *display = egl::Display::get(dpy);

	if(!validateDisplay(display))
	{
		return nullptr;
	}

	switch(name)
	{
	case EGL_CLIENT_APIS:
		return success("OpenGL_ES");
	case EGL_EXTENSIONS:
		return success("EGL_KHR_create_context "
		               "EGL_KHR_get_all_proc_addresses "
		               "EGL_KHR_gl_texture_2D_image "
		               "EGL_KHR_gl_texture_cubemap_image "
		               "EGL_KHR_gl_renderbuffer_image "
		               "EGL_KHR_fence_sync "
		               "EGL_KHR_image_base "
		               "EGL_KHR_surfaceless_context "
		               "EGL_ANGLE_iosurface_client_buffer "
		               "EGL_ANDROID_framebuffer_target "
		               "EGL_ANDROID_recordable");
	case EGL_VENDOR:
		return success("Google Inc.");
	case EGL_VERSION:
		return success("1.4 SwiftShader " VERSION_STRING);
	}

	return error(EGL_BAD_PARAMETER, (const char*)nullptr);
}

EGLBoolean GetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	TRACE("(EGLDisplay dpy = %p, EGLConfig *configs = %p, "
	      "EGLint config_size = %d, EGLint *num_config = %p)",
	      dpy, configs, config_size, num_config);

	egl::Display *display = egl::Display::get(dpy);

	if(!validateDisplay(display))
	{
		return EGL_FALSE;
	}

	if(!num_config)
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	const EGLint attribList[] = {EGL_NONE};

	if(!display->getConfigs(configs, attribList, config_size, num_config))
	{
		return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
	}

	return success(EGL_TRUE);
}

EGLBoolean ChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	TRACE("(EGLDisplay dpy = %p, const EGLint *attrib_list = %p, "
	      "EGLConfig *configs = %p, EGLint config_size = %d, EGLint *num_config = %p)",
	      dpy, attrib_list, configs, config_size, num_config);

	egl::Display *display = egl::Display::get(dpy);

	if(!validateDisplay(display))
	{
		return EGL_FALSE;
	}

	if(!num_config)
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	const EGLint attribList[] = {EGL_NONE};

	if(!attrib_list)
	{
		attrib_list = attribList;
	}

	if(!display->getConfigs(configs, attrib_list, config_size, num_config))
	{
		return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
	}

	return success(EGL_TRUE);
}

EGLBoolean GetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
	TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, EGLint attribute = %d, EGLint *value = %p)",
	      dpy, config, attribute, value);

	egl::Display *display = egl::Display::get(dpy);

	if(!validateConfig(display, config))
	{
		return EGL_FALSE;
	}

	if(!display->getConfigAttrib(config, attribute, value))
	{
		return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
	}

	return success(EGL_TRUE);
}

EGLSurface CreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window, const EGLint *attrib_list)
{
	TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, EGLNativeWindowType win = %p, "
	      "const EGLint *attrib_list = %p)", dpy, config, window, attrib_list);

	egl::Display *display = egl::Display::get(dpy);

	if(!validateConfig(display, config))
	{
		return EGL_NO_SURFACE;
	}

	if(!display->isValidWindow(window))
	{
		return error(EGL_BAD_NATIVE_WINDOW, EGL_NO_SURFACE);
	}

	return display->createWindowSurface(window, config, attrib_list);
}

EGLSurface CreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
	TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, const EGLint *attrib_list = %p)",
	      dpy, config, attrib_list);

	egl::Display *display = egl::Display::get(dpy);

	if(!validateConfig(display, config))
	{
		return EGL_NO_SURFACE;
	}

	return display->createPBufferSurface(config, attrib_list);
}

EGLSurface CreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
	TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, EGLNativePixmapType pixmap = %p, "
	      "const EGLint *attrib_list = %p)", dpy, config, pixmap, attrib_list);

	egl::Display *display = egl::Display::get(dpy);

	if(!validateConfig(display, config))
	{
		return EGL_NO_SURFACE;
	}

	UNIMPLEMENTED();   // FIXME

	return success(EGL_NO_SURFACE);
}

EGLBoolean DestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p)", dpy, surface);

	egl::Display *display = egl::Display::get(dpy);
	egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

	if(!validateSurface(display, eglSurface))
	{
		return EGL_FALSE;
	}

	if(surface == EGL_NO_SURFACE)
	{
		return error(EGL_BAD_SURFACE, EGL_FALSE);
	}

	display->destroySurface((egl::Surface*)surface);

	return success(EGL_TRUE);
}

EGLBoolean QuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
	TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p, EGLint attribute = %d, EGLint *value = %p)",
	      dpy, surface, attribute, value);

	egl::Display *display = egl::Display::get(dpy);
	egl::Surface *eglSurface = (egl::Surface*)surface;

	if(!validateSurface(display, eglSurface))
	{
		return EGL_FALSE;
	}

	if(surface == EGL_NO_SURFACE)
	{
		return error(EGL_BAD_SURFACE, EGL_FALSE);
	}

	switch(attribute)
	{
	case EGL_VG_ALPHA_FORMAT:
		*value = EGL_VG_ALPHA_FORMAT_NONPRE;   // Default
		break;
	case EGL_VG_COLORSPACE:
		*value = EGL_VG_COLORSPACE_sRGB;   // Default
		break;
	case EGL_CONFIG_ID:
		*value = eglSurface->getConfigID();
		break;
	case EGL_HEIGHT:
		*value = eglSurface->getHeight();
		break;
	case EGL_HORIZONTAL_RESOLUTION:
		*value = EGL_UNKNOWN;
		break;
	case EGL_LARGEST_PBUFFER:
		if(eglSurface->isPBufferSurface())   // For a window or pixmap surface, the contents of *value are not modified.
		{
			*value = eglSurface->getLargestPBuffer();
		}
		break;
	case EGL_MIPMAP_TEXTURE:
		if(eglSurface->isPBufferSurface())   // For a window or pixmap surface, the contents of *value are not modified.
		{
			*value = EGL_FALSE;   // UNIMPLEMENTED
		}
		break;
	case EGL_MIPMAP_LEVEL:
		if(eglSurface->isPBufferSurface())   // For a window or pixmap surface, the contents of *value are not modified.
		{
			*value = eglSurface->getMipmapLevel();
		}
		break;
	case EGL_MULTISAMPLE_RESOLVE:
		*value = eglSurface->getMultisampleResolve();
		break;
	case EGL_PIXEL_ASPECT_RATIO:
		*value = eglSurface->getPixelAspectRatio();
		break;
	case EGL_RENDER_BUFFER:
		*value = eglSurface->getRenderBuffer();
		break;
	case EGL_SWAP_BEHAVIOR:
		*value = eglSurface->getSwapBehavior();
		break;
	case EGL_TEXTURE_FORMAT:
		if(eglSurface->isPBufferSurface())   // For a window or pixmap surface, the contents of *value are not modified.
		{
			*value = eglSurface->getTextureFormat();
		}
		break;
	case EGL_TEXTURE_TARGET:
		if(eglSurface->isPBufferSurface())   // For a window or pixmap surface, the contents of *value are not modified.
		{
			*value = eglSurface->getTextureTarget();
		}
		break;
	case EGL_VERTICAL_RESOLUTION:
		*value = EGL_UNKNOWN;
		break;
	case EGL_WIDTH:
		*value = eglSurface->getWidth();
		break;
	default:
		return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
	}

	return success(EGL_TRUE);
}

EGLBoolean BindAPI(EGLenum api)
{
	TRACE("(EGLenum api = 0x%X)", api);

	switch(api)
	{
	case EGL_OPENGL_API:
	case EGL_OPENVG_API:
		return error(EGL_BAD_PARAMETER, EGL_FALSE);   // Not supported by this implementation
	case EGL_OPENGL_ES_API:
		break;
	default:
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	egl::setCurrentAPI(api);

	return success(EGL_TRUE);
}

EGLenum QueryAPI(void)
{
	TRACE("()");

	EGLenum API = egl::getCurrentAPI();

	return success(API);
}

EGLBoolean WaitClient(void)
{
	TRACE("()");

	// eglWaitClient is ignored if there is no current EGL rendering context for the current rendering API.
	egl::Context *context = egl::getCurrentContext();

	if(context)
	{
		context->finish();
	}

	return success(EGL_TRUE);
}

EGLBoolean ReleaseThread(void)
{
	TRACE("()");

	detachThread();

	return EGL_TRUE;   // success() is not called here because it would re-allocate thread-local storage.
}

EGLSurface CreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
	TRACE("(EGLDisplay dpy = %p, EGLenum buftype = 0x%X, EGLClientBuffer buffer = %p, "
	      "EGLConfig config = %p, const EGLint *attrib_list = %p)",
	      dpy, buftype, buffer, config, attrib_list);

	switch(buftype)
	{
	case EGL_IOSURFACE_ANGLE:
	{
		egl::Display *display = egl::Display::get(dpy);

		if(!validateConfig(display, config))
		{
			return EGL_NO_SURFACE;
		}

		return display->createPBufferSurface(config, attrib_list, buffer);
	}
	case EGL_OPENVG_IMAGE:
		UNIMPLEMENTED();
		return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
	default:
		return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
	};
}

EGLBoolean SurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
	TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p, EGLint attribute = %d, EGLint value = %d)",
	      dpy, surface, attribute, value);

	egl::Display *display = egl::Display::get(dpy);
	egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

	if(!validateSurface(display, eglSurface))
	{
		return EGL_FALSE;
	}

	switch(attribute)
	{
	case EGL_MIPMAP_LEVEL:
		eglSurface->setMipmapLevel(value);
		break;
	case EGL_MULTISAMPLE_RESOLVE:
		switch(value)
		{
		case EGL_MULTISAMPLE_RESOLVE_DEFAULT:
			break;
		case EGL_MULTISAMPLE_RESOLVE_BOX:
			if(!(eglSurface->getSurfaceType() & EGL_MULTISAMPLE_RESOLVE_BOX_BIT))
			{
				return error(EGL_BAD_MATCH, EGL_FALSE);
			}
			break;
		default:
			return error(EGL_BAD_PARAMETER, EGL_FALSE);
		}
		eglSurface->setMultisampleResolve(value);
		break;
	case EGL_SWAP_BEHAVIOR:
		switch(value)
		{
		case EGL_BUFFER_DESTROYED:
			break;
		case EGL_BUFFER_PRESERVED:
			if(!(eglSurface->getSurfaceType() & EGL_SWAP_BEHAVIOR_PRESERVED_BIT))
			{
				return error(EGL_BAD_MATCH, EGL_FALSE);
			}
			break;
		default:
			return error(EGL_BAD_PARAMETER, EGL_FALSE);
		}
		eglSurface->setSwapBehavior(value);
		break;
	default:
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	return success(EGL_TRUE);
}

EGLBoolean BindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p, EGLint buffer = %d)", dpy, surface, buffer);

	egl::Display *display = egl::Display::get(dpy);
	egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

	if(!validateSurface(display, eglSurface))
	{
		return EGL_FALSE;
	}

	if(buffer != EGL_BACK_BUFFER)
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	if(surface == EGL_NO_SURFACE || eglSurface->isWindowSurface())
	{
		return error(EGL_BAD_SURFACE, EGL_FALSE);
	}

	if(eglSurface->getBoundTexture())
	{
		return error(EGL_BAD_ACCESS, EGL_FALSE);
	}

	if(eglSurface->getTextureFormat() == EGL_NO_TEXTURE)
	{
		return error(EGL_BAD_MATCH, EGL_FALSE);
	}

	egl::Context *context = egl::getCurrentContext();

	if(context)
	{
		context->bindTexImage(eglSurface);
	}

	return success(EGL_TRUE);
}

EGLBoolean ReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p, EGLint buffer = %d)", dpy, surface, buffer);

	egl::Display *display = egl::Display::get(dpy);
	egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

	if(!validateSurface(display, eglSurface))
	{
		return EGL_FALSE;
	}

	if(buffer != EGL_BACK_BUFFER)
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	if(surface == EGL_NO_SURFACE || eglSurface->isWindowSurface())
	{
		return error(EGL_BAD_SURFACE, EGL_FALSE);
	}

	if(eglSurface->getTextureFormat() == EGL_NO_TEXTURE)
	{
		return error(EGL_BAD_MATCH, EGL_FALSE);
	}

	egl::Texture *texture = eglSurface->getBoundTexture();

	if(texture)
	{
		texture->releaseTexImage();
	}

	return success(EGL_TRUE);
}

EGLBoolean SwapInterval(EGLDisplay dpy, EGLint interval)
{
	TRACE("(EGLDisplay dpy = %p, EGLint interval = %d)", dpy, interval);

	egl::Display *display = egl::Display::get(dpy);
	egl::Context *context = egl::getCurrentContext();

	if(!validateContext(display, context))
	{
		return EGL_FALSE;
	}

	egl::Surface *draw_surface = static_cast<egl::Surface*>(egl::getCurrentDrawSurface());

	if(!draw_surface)
	{
		return error(EGL_BAD_SURFACE, EGL_FALSE);
	}

	draw_surface->setSwapInterval(interval);

	return success(EGL_TRUE);
}

EGLContext CreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, EGLContext share_context = %p, "
	      "const EGLint *attrib_list = %p)", dpy, config, share_context, attrib_list);

	EGLint majorVersion = 1;
	EGLint minorVersion = 0;

	if(attrib_list)
	{
		for(const EGLint* attribute = attrib_list; attribute[0] != EGL_NONE; attribute += 2)
		{
			switch(attribute[0])
			{
			case EGL_CONTEXT_MAJOR_VERSION_KHR:   // This token is an alias for EGL_CONTEXT_CLIENT_VERSION
				majorVersion = attribute[1];
				break;
			case EGL_CONTEXT_MINOR_VERSION_KHR:
				minorVersion = attribute[1];
				break;
			case EGL_CONTEXT_FLAGS_KHR:
				switch(attribute[1])
				{
				case EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR:
					// According to the EGL_KHR_create_context spec:
					// "Khronos is still defining the expected and required features of debug contexts, so
					//  implementations are currently free to implement "debug contexts" with little or no debug
					//  functionality. However, OpenGL and OpenGL ES implementations supporting the GL_KHR_debug
					//  extension should enable it when this bit is set."
					break;
				case EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR:
				case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR:
					// These bits are for OpenGL contexts only, not OpenGL ES contexts
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
				default:
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
				}
				break;
			case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
				switch(attribute[1])
				{
				case EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR:
				case EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR:
					// These bits are for OpenGL contexts only, not OpenGL ES contexts
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
				default:
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
				}
				break;
			case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
				switch(attribute[1])
				{
				case EGL_NO_RESET_NOTIFICATION_KHR:
				case EGL_LOSE_CONTEXT_ON_RESET_KHR:
					// These bits are for OpenGL contexts only, not OpenGL ES contexts
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
				default:
					return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
				}
				break;
			default:
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
			}
		}
	}

	switch(majorVersion)
	{
	case 1:
		if(minorVersion != 0 && minorVersion != 1)
		{
			// 1.X: Only OpenGL ES 1.0 and 1.1 contexts are supported
			return error(EGL_BAD_MATCH, EGL_NO_CONTEXT);
		}
		break;
	case 2:
	case 3:
		if(minorVersion != 0)
		{
			// 2.X and 3.X: Only OpenGL ES 2.0 and 3.0 contexts are currently supported
			return error(EGL_BAD_MATCH, EGL_NO_CONTEXT);
		}
		break;
	default:
		return error(EGL_BAD_MATCH, EGL_NO_CONTEXT);
	}

	egl::Display *display = egl::Display::get(dpy);
	egl::Context *shareContext = static_cast<egl::Context*>(share_context);

	if(!validateConfig(display, config))
	{
		return EGL_NO_CONTEXT;
	}

	if(shareContext && shareContext->getClientVersion() != majorVersion)
	{
		return error(EGL_BAD_CONTEXT, EGL_NO_CONTEXT);
	}

	return display->createContext(config, shareContext, majorVersion);
}

EGLBoolean DestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	TRACE("(EGLDisplay dpy = %p, EGLContext ctx = %p)", dpy, ctx);

	egl::Display *display = egl::Display::get(dpy);
	egl::Context *context = static_cast<egl::Context*>(ctx);

	if(!validateContext(display, context))
	{
		return EGL_FALSE;
	}

	if(ctx == EGL_NO_CONTEXT)
	{
		return error(EGL_BAD_CONTEXT, EGL_FALSE);
	}

	display->destroyContext(context);

	return success(EGL_TRUE);
}

EGLBoolean MakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	TRACE("(EGLDisplay dpy = %p, EGLSurface draw = %p, EGLSurface read = %p, EGLContext ctx = %p)",
	      dpy, draw, read, ctx);

	egl::Display *display = egl::Display::get(dpy);
	egl::Context *context = static_cast<egl::Context*>(ctx);
	egl::Surface *drawSurface = static_cast<egl::Surface*>(draw);
	egl::Surface *readSurface = static_cast<egl::Surface*>(read);

	if(ctx != EGL_NO_CONTEXT || draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE)
	{
		if(!validateDisplay(display))
		{
			return EGL_FALSE;
		}
	}

	if(ctx == EGL_NO_CONTEXT && (draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE))
	{
		return error(EGL_BAD_MATCH, EGL_FALSE);
	}

	if(ctx != EGL_NO_CONTEXT && !validateContext(display, context))
	{
		return EGL_FALSE;
	}

	if((draw != EGL_NO_SURFACE && !validateSurface(display, drawSurface)) ||
	   (read != EGL_NO_SURFACE && !validateSurface(display, readSurface)))
	{
		return EGL_FALSE;
	}

	if((draw != EGL_NO_SURFACE && drawSurface->hasClientBuffer()) ||
	   (read != EGL_NO_SURFACE && readSurface->hasClientBuffer()))
	{
		// Make current is not supported on IOSurface pbuffers.
		return error(EGL_BAD_SURFACE, EGL_FALSE);
	}

	if((draw != EGL_NO_SURFACE) ^ (read != EGL_NO_SURFACE))
	{
		return error(EGL_BAD_MATCH, EGL_FALSE);
	}

	if(draw != read)
	{
		UNIMPLEMENTED();   // FIXME
	}

	egl::setCurrentDrawSurface(drawSurface);
	egl::setCurrentReadSurface(readSurface);
	egl::setCurrentContext(context);

	if(context)
	{
		context->makeCurrent(drawSurface);
	}

	return success(EGL_TRUE);
}

EGLContext GetCurrentContext(void)
{
	TRACE("()");

	EGLContext context = egl::getCurrentContext();

	return success(context);
}

EGLSurface GetCurrentSurface(EGLint readdraw)
{
	TRACE("(EGLint readdraw = %d)", readdraw);

	if(readdraw == EGL_READ)
	{
		EGLSurface read = egl::getCurrentReadSurface();
		return success(read);
	}
	else if(readdraw == EGL_DRAW)
	{
		EGLSurface draw = egl::getCurrentDrawSurface();
		return success(draw);
	}
	else
	{
		return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
	}
}

EGLDisplay GetCurrentDisplay(void)
{
	TRACE("()");

	egl::Context *context = egl::getCurrentContext();

	if(!context)
	{
		return success(EGL_NO_DISPLAY);
	}

	egl::Display *display = context->getDisplay();

	if(!display)
	{
		return error(EGL_BAD_ACCESS, EGL_NO_DISPLAY);
	}

	return success(display->getEGLDisplay());
}

EGLBoolean QueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
	TRACE("(EGLDisplay dpy = %p, EGLContext ctx = %p, EGLint attribute = %d, EGLint *value = %p)",
	      dpy, ctx, attribute, value);

	egl::Display *display = egl::Display::get(dpy);
	egl::Context *context = static_cast<egl::Context*>(ctx);

	if(!validateContext(display, context))
	{
		return EGL_FALSE;
	}

	switch(attribute)
	{
	case EGL_CONFIG_ID:
		*value = context->getConfigID();
		break;
	case EGL_CONTEXT_CLIENT_TYPE:
		*value = egl::getCurrentAPI();
		break;
	case EGL_CONTEXT_CLIENT_VERSION:
		*value = context->getClientVersion();
		break;
	case EGL_RENDER_BUFFER:
		*value = EGL_BACK_BUFFER;
		break;
	default:
		return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
	}

	return success(EGL_TRUE);
}

EGLBoolean WaitGL(void)
{
	TRACE("()");

	// glWaitGL is ignored if there is no current EGL rendering context for OpenGL ES.
	egl::Context *context = egl::getCurrentContext();

	if(context)
	{
		context->finish();
	}

	return success(EGL_TRUE);
}

EGLBoolean WaitNative(EGLint engine)
{
	TRACE("(EGLint engine = %d)", engine);

	if(engine != EGL_CORE_NATIVE_ENGINE)
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	// eglWaitNative is ignored if there is no current EGL rendering context.
	egl::Context *context = egl::getCurrentContext();

	if(context)
	{
		#if defined(__linux__) && !defined(__ANDROID__)
			egl::Display *display = context->getDisplay();

			if(!display)
			{
				return error(EGL_BAD_DISPLAY, EGL_FALSE);
			}

			libX11->XSync((::Display*)display->getNativeDisplay(), False);
		#else
			UNIMPLEMENTED();
		#endif
	}

	return success(EGL_TRUE);
}

EGLBoolean SwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
	TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p)", dpy, surface);

	egl::Display *display = egl::Display::get(dpy);
	egl::Surface *eglSurface = (egl::Surface*)surface;

	if(!validateSurface(display, eglSurface))
	{
		return EGL_FALSE;
	}

	if(surface == EGL_NO_SURFACE)
	{
		return error(EGL_BAD_SURFACE, EGL_FALSE);
	}

	eglSurface->swap();

	return success(EGL_TRUE);
}

EGLBoolean CopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
	TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p, EGLNativePixmapType target = %p)", dpy, surface, target);

	egl::Display *display = egl::Display::get(dpy);
	egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

	if(!validateSurface(display, eglSurface))
	{
		return EGL_FALSE;
	}

	UNIMPLEMENTED();   // FIXME

	return success(EGL_FALSE);
}

EGLImageKHR CreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
	TRACE("(EGLDisplay dpy = %p, EGLContext ctx = %p, EGLenum target = 0x%X, buffer = %p, const EGLint attrib_list = %p)", dpy, ctx, target, buffer, attrib_list);

	egl::Display *display = egl::Display::get(dpy);
	egl::Context *context = static_cast<egl::Context*>(ctx);

	if(!validateDisplay(display))
	{
		return error(EGL_BAD_DISPLAY, EGL_NO_IMAGE_KHR);
	}

	if(context != EGL_NO_CONTEXT && !display->isValidContext(context))
	{
		return error(EGL_BAD_CONTEXT, EGL_NO_IMAGE_KHR);
	}

	EGLenum imagePreserved = EGL_FALSE;
	GLuint textureLevel = 0;
	if(attrib_list)
	{
		for(const EGLint *attribute = attrib_list; attribute[0] != EGL_NONE; attribute += 2)
		{
			if(attribute[0] == EGL_IMAGE_PRESERVED_KHR)
			{
				imagePreserved = attribute[1];
			}
			else if(attribute[0] == EGL_GL_TEXTURE_LEVEL_KHR)
			{
				textureLevel = attribute[1];
			}
			else
			{
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_IMAGE_KHR);
			}
		}
	}

	#if defined(__ANDROID__)
		if(target == EGL_NATIVE_BUFFER_ANDROID)
		{
			ANativeWindowBuffer *nativeBuffer = reinterpret_cast<ANativeWindowBuffer*>(buffer);

			if(!nativeBuffer || GLPixelFormatFromAndroid(nativeBuffer->format) == GL_NONE)
			{
				ALOGW("%s badness unsupported HAL format=%x", __FUNCTION__, nativeBuffer ? nativeBuffer->format : 0);
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_IMAGE_KHR);
			}

			Image *image = new AndroidNativeImage(nativeBuffer);
			EGLImageKHR eglImage = display->createSharedImage(image);

			return success(eglImage);
		}
	#endif

	GLuint name = static_cast<GLuint>(reinterpret_cast<uintptr_t>(buffer));

	if(name == 0)
	{
		return error(EGL_BAD_PARAMETER, EGL_NO_IMAGE_KHR);
	}

	EGLenum validationResult = context->validateSharedImage(target, name, textureLevel);

	if(validationResult != EGL_SUCCESS)
	{
		return error(validationResult, EGL_NO_IMAGE_KHR);
	}

	Image *image = context->createSharedImage(target, name, textureLevel);

	if(!image)
	{
		return error(EGL_BAD_MATCH, EGL_NO_IMAGE_KHR);
	}

	if(image->getDepth() > 1)
	{
		return error(EGL_BAD_PARAMETER, EGL_NO_IMAGE_KHR);
	}

	EGLImageKHR eglImage = display->createSharedImage(image);

	return success(eglImage);
}

EGLBoolean DestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
	TRACE("(EGLDisplay dpy = %p, EGLImageKHR image = %p)", dpy, image);

	egl::Display *display = egl::Display::get(dpy);

	if(!validateDisplay(display))
	{
		return error(EGL_BAD_DISPLAY, EGL_FALSE);
	}

	if(!display->destroySharedImage(image))
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	return success(EGL_TRUE);
}

EGLDisplay GetPlatformDisplayEXT(EGLenum platform, void *native_display, const EGLint *attrib_list)
{
	TRACE("(EGLenum platform = 0x%X, void *native_display = %p, const EGLint *attrib_list = %p)", platform, native_display, attrib_list);

	#if defined(__linux__) && !defined(__ANDROID__)
		switch(platform)
		{
		case EGL_PLATFORM_X11_EXT: break;
		case EGL_PLATFORM_GBM_KHR: break;
		default:
			return error(EGL_BAD_PARAMETER, EGL_NO_DISPLAY);
		}

		if(platform == EGL_PLATFORM_X11_EXT)
		{
			if(!libX11)
			{
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_DISPLAY);
			}

			if(native_display != (void*)EGL_DEFAULT_DISPLAY || attrib_list != NULL)
			{
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_DISPLAY);   // Unimplemented
			}
		}
		else if(platform == EGL_PLATFORM_GBM_KHR)
		{
			if(native_display != (void*)EGL_DEFAULT_DISPLAY || attrib_list != NULL)
			{
				return error(EGL_BAD_ATTRIBUTE, EGL_NO_DISPLAY);   // Unimplemented
			}

			return success(HEADLESS_DISPLAY);
		}

		return success(PRIMARY_DISPLAY);   // We only support the default display
	#else
		return error(EGL_BAD_PARAMETER, EGL_NO_DISPLAY);
	#endif
}

EGLSurface CreatePlatformWindowSurfaceEXT(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list)
{
	return CreateWindowSurface(dpy, config, (EGLNativeWindowType)native_window, attrib_list);
}

EGLSurface CreatePlatformPixmapSurfaceEXT(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list)
{
	return CreatePixmapSurface(dpy, config, (EGLNativePixmapType)native_pixmap, attrib_list);
}

EGLSyncKHR CreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
	TRACE("(EGLDisplay dpy = %p, EGLunum type = %x, EGLint *attrib_list=%p)", dpy, type, attrib_list);

	egl::Display *display = egl::Display::get(dpy);

	if(!validateDisplay(display))
	{
		return error(EGL_BAD_DISPLAY, EGL_NO_SYNC_KHR);
	}

	if(type != EGL_SYNC_FENCE_KHR)
	{
		return error(EGL_BAD_ATTRIBUTE, EGL_NO_SYNC_KHR);
	}

	if(attrib_list && attrib_list[0] != EGL_NONE)
	{
		return error(EGL_BAD_ATTRIBUTE, EGL_NO_SYNC_KHR);
	}

	egl::Context *context = egl::getCurrentContext();

	if(!validateContext(display, context))
	{
		return error(EGL_BAD_MATCH, EGL_NO_SYNC_KHR);
	}

	EGLSyncKHR sync = display->createSync(context);

	return success(sync);
}

EGLBoolean DestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
	TRACE("(EGLDisplay dpy = %p, EGLSyncKHR sync = %p)", dpy, sync);

	egl::Display *display = egl::Display::get(dpy);
	FenceSync *eglSync = static_cast<FenceSync*>(sync);

	if(!validateDisplay(display))
	{
		return error(EGL_BAD_DISPLAY, EGL_FALSE);
	}

	if(!display->isValidSync(eglSync))
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	display->destroySync(eglSync);

	return success(EGL_TRUE);
}

EGLint ClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
	TRACE("(EGLDisplay dpy = %p, EGLSyncKHR sync = %p, EGLint flags = %x, EGLTimeKHR value = %llx)", dpy, sync, flags, timeout);

	egl::Display *display = egl::Display::get(dpy);
	FenceSync *eglSync = static_cast<FenceSync*>(sync);

	if(!validateDisplay(display))
	{
		return error(EGL_BAD_DISPLAY, EGL_FALSE);
	}

	if(!display->isValidSync(eglSync))
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	(void)flags;
	(void)timeout;

	if(!eglSync->isSignaled())
	{
		eglSync->wait();
	}

	return success(EGL_CONDITION_SATISFIED_KHR);
}

EGLBoolean GetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value)
{
	TRACE("(EGLDisplay dpy = %p, EGLSyncKHR sync = %p, EGLint attribute = %x, EGLint *value = %p)", dpy, sync, attribute, value);

	egl::Display *display = egl::Display::get(dpy);
	FenceSync *eglSync = static_cast<FenceSync*>(sync);

	if(!validateDisplay(display))
	{
		return error(EGL_BAD_DISPLAY, EGL_FALSE);
	}

	if(!display->isValidSync(eglSync))
	{
		return error(EGL_BAD_PARAMETER, EGL_FALSE);
	}

	switch(attribute)
	{
	case EGL_SYNC_TYPE_KHR:
		*value = EGL_SYNC_FENCE_KHR;
		return success(EGL_TRUE);
	case EGL_SYNC_STATUS_KHR:
		eglSync->wait();   // TODO: Don't block. Just poll based on sw::Query.
		*value = eglSync->isSignaled() ? EGL_SIGNALED_KHR : EGL_UNSIGNALED_KHR;
		return success(EGL_TRUE);
	case EGL_SYNC_CONDITION_KHR:
		*value = EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR;
		return success(EGL_TRUE);
	default:
		return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
	}
}

__eglMustCastToProperFunctionPointerType GetProcAddress(const char *procname)
{
	TRACE("(const char *procname = \"%s\")", procname);

	struct Function
	{
		const char *name;
		__eglMustCastToProperFunctionPointerType address;
	};

	struct CompareFunctor
	{
		bool operator()(const Function &a, const Function &b) const
		{
			return strcmp(a.name, b.name) < 0;
		}
	};

	// This array must be kept sorted with respect to strcmp(), so that binary search works correctly.
	// The Unix command "LC_COLLATE=C sort" will generate the correct order.
	static const Function eglFunctions[] =
	{
		#define FUNCTION(name) {#name, (__eglMustCastToProperFunctionPointerType)name}

		FUNCTION(eglBindAPI),
		FUNCTION(eglBindTexImage),
		FUNCTION(eglChooseConfig),
		FUNCTION(eglClientWaitSyncKHR),
		FUNCTION(eglCopyBuffers),
		FUNCTION(eglCreateContext),
		FUNCTION(eglCreateImageKHR),
		FUNCTION(eglCreatePbufferFromClientBuffer),
		FUNCTION(eglCreatePbufferSurface),
		FUNCTION(eglCreatePixmapSurface),
		FUNCTION(eglCreatePlatformPixmapSurfaceEXT),
		FUNCTION(eglCreatePlatformWindowSurfaceEXT),
		FUNCTION(eglCreateSyncKHR),
		FUNCTION(eglCreateWindowSurface),
		FUNCTION(eglDestroyContext),
		FUNCTION(eglDestroyImageKHR),
		FUNCTION(eglDestroySurface),
		FUNCTION(eglDestroySyncKHR),
		FUNCTION(eglGetConfigAttrib),
		FUNCTION(eglGetConfigs),
		FUNCTION(eglGetCurrentContext),
		FUNCTION(eglGetCurrentDisplay),
		FUNCTION(eglGetCurrentSurface),
		FUNCTION(eglGetDisplay),
		FUNCTION(eglGetError),
		FUNCTION(eglGetPlatformDisplayEXT),
		FUNCTION(eglGetProcAddress),
		FUNCTION(eglGetSyncAttribKHR),
		FUNCTION(eglInitialize),
		FUNCTION(eglMakeCurrent),
		FUNCTION(eglQueryAPI),
		FUNCTION(eglQueryContext),
		FUNCTION(eglQueryString),
		FUNCTION(eglQuerySurface),
		FUNCTION(eglReleaseTexImage),
		FUNCTION(eglReleaseThread),
		FUNCTION(eglSurfaceAttrib),
		FUNCTION(eglSwapBuffers),
		FUNCTION(eglSwapInterval),
		FUNCTION(eglTerminate),
		FUNCTION(eglWaitClient),
		FUNCTION(eglWaitGL),
		FUNCTION(eglWaitNative),

		#undef FUNCTION
	};

	static const size_t numFunctions = sizeof eglFunctions / sizeof(Function);
	static const Function *const eglFunctionsEnd = eglFunctions + numFunctions;

	Function needle;
	needle.name = procname;

	if(procname && strncmp("egl", procname, 3) == 0)
	{
		const Function *result = std::lower_bound(eglFunctions, eglFunctionsEnd, needle, CompareFunctor());
		if (result != eglFunctionsEnd && strcmp(procname, result->name) == 0)
		{
			return success((__eglMustCastToProperFunctionPointerType)result->address);
		}
	}

	if(libGLESv2)
	{
		__eglMustCastToProperFunctionPointerType proc = libGLESv2->es2GetProcAddress(procname);
		if(proc) return success(proc);
	}

	if(libGLES_CM)
	{
		__eglMustCastToProperFunctionPointerType proc =  libGLES_CM->es1GetProcAddress(procname);
		if(proc) return success(proc);
	}

	return success((__eglMustCastToProperFunctionPointerType)NULL);
}
}
