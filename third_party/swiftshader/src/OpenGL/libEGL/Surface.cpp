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

// Surface.cpp: Implements the egl::Surface class, representing a drawing surface
// such as the client area of a window, including any back buffers.
// Implements EGLSurface and related functionality. [EGL 1.4] section 2.2 page 3.

#include "Surface.hpp"

#include "main.h"
#include "Display.h"
#include "Texture.hpp"
#include "common/Image.hpp"
#include "Context.hpp"
#include "common/debug.h"
#include "Main/FrameBuffer.hpp"

#if defined(__linux__) && !defined(__ANDROID__)
#include "Main/libX11.hpp"
#elif defined(_WIN32)
#include <tchar.h>
#elif defined(__APPLE__)
#include "OSXUtils.hpp"
#endif

#include <algorithm>

namespace gl
{
Surface::Surface()
{
}

Surface::~Surface()
{
}
}

namespace egl
{
Surface::Surface(const Display *display, const Config *config) : display(display), config(config)
{
}

Surface::~Surface()
{
	Surface::deleteResources();
}

bool Surface::initialize()
{
	ASSERT(!backBuffer && !depthStencil);

	if(libGLESv2)
	{
		if(clientBuffer)
		{
			backBuffer = libGLESv2->createBackBufferFromClientBuffer(
				egl::ClientBuffer(width, height, getClientBufferFormat(), clientBuffer, clientBufferPlane));
		}
		else
		{
			backBuffer = libGLESv2->createBackBuffer(width, height, config->mRenderTargetFormat, config->mSamples);
		}
	}
	else if(libGLES_CM)
	{
		backBuffer = libGLES_CM->createBackBuffer(width, height, config->mRenderTargetFormat, config->mSamples);
	}

	if(!backBuffer)
	{
		ERR("Could not create back buffer");
		deleteResources();
		return error(EGL_BAD_ALLOC, false);
	}

	if(config->mDepthStencilFormat != sw::FORMAT_NULL)
	{
		if(libGLESv2)
		{
			depthStencil = libGLESv2->createDepthStencil(width, height, config->mDepthStencilFormat, config->mSamples);
		}
		else if(libGLES_CM)
		{
			depthStencil = libGLES_CM->createDepthStencil(width, height, config->mDepthStencilFormat, config->mSamples);
		}

		if(!depthStencil)
		{
			ERR("Could not create depth/stencil buffer for surface");
			deleteResources();
			return error(EGL_BAD_ALLOC, false);
		}
	}

	return true;
}

void Surface::deleteResources()
{
	if(depthStencil)
	{
		depthStencil->release();
		depthStencil = nullptr;
	}

	if(texture)
	{
		texture->releaseTexImage();
		texture = nullptr;
	}

	if(backBuffer)
	{
		backBuffer->release();
		backBuffer = nullptr;
	}
}

egl::Image *Surface::getRenderTarget()
{
	if(backBuffer)
	{
		backBuffer->addRef();
	}

	return backBuffer;
}

egl::Image *Surface::getDepthStencil()
{
	if(depthStencil)
	{
		depthStencil->addRef();
	}

	return depthStencil;
}

void Surface::setMipmapLevel(EGLint mipmapLevel)
{
	this->mipmapLevel = mipmapLevel;
}

void Surface::setMultisampleResolve(EGLenum multisampleResolve)
{
	this->multisampleResolve = multisampleResolve;
}

void Surface::setSwapBehavior(EGLenum swapBehavior)
{
	this->swapBehavior = swapBehavior;
}

void Surface::setSwapInterval(EGLint interval)
{
	if(swapInterval == interval)
	{
		return;
	}

	swapInterval = interval;
	swapInterval = std::max(swapInterval, display->getMinSwapInterval());
	swapInterval = std::min(swapInterval, display->getMaxSwapInterval());
}

EGLint Surface::getConfigID() const
{
	return config->mConfigID;
}

EGLenum Surface::getSurfaceType() const
{
	return config->mSurfaceType;
}

EGLint Surface::getWidth() const
{
	return width;
}

EGLint Surface::getHeight() const
{
	return height;
}

EGLint Surface::getMipmapLevel() const
{
	return mipmapLevel;
}

EGLenum Surface::getMultisampleResolve() const
{
	return multisampleResolve;
}

EGLint Surface::getPixelAspectRatio() const
{
	return pixelAspectRatio;
}

EGLenum Surface::getRenderBuffer() const
{
	return renderBuffer;
}

EGLenum Surface::getSwapBehavior() const
{
	return swapBehavior;
}

EGLenum Surface::getTextureFormat() const
{
	return textureFormat;
}

EGLenum Surface::getTextureTarget() const
{
	return textureTarget;
}

EGLBoolean Surface::getLargestPBuffer() const
{
	return largestPBuffer;
}

sw::Format Surface::getClientBufferFormat() const
{
	switch(clientBufferType)
	{
	case GL_UNSIGNED_BYTE:
		switch(clientBufferFormat)
		{
		case GL_RED:
			return sw::FORMAT_R8;
		case GL_RG:
			return sw::FORMAT_G8R8;
		case GL_BGRA_EXT:
			return sw::FORMAT_A8R8G8B8;
		default:
			UNREACHABLE(clientBufferFormat);
			break;
		}
		break;
	case GL_UNSIGNED_SHORT:
		switch(clientBufferFormat)
		{
		case GL_R16UI:
			return sw::FORMAT_R16UI;
		default:
			UNREACHABLE(clientBufferFormat);
			break;
		}
		break;
	case GL_HALF_FLOAT_OES:
	case GL_HALF_FLOAT:
		switch(clientBufferFormat)
		{
		case GL_RGBA:
			return sw::FORMAT_A16B16G16R16F;
		default:
			UNREACHABLE(clientBufferFormat);
			break;
		}
	default:
		UNREACHABLE(clientBufferType);
		break;
	}

	return sw::FORMAT_NULL;
}

void Surface::setBoundTexture(egl::Texture *texture)
{
	this->texture = texture;
}

egl::Texture *Surface::getBoundTexture() const
{
	return texture;
}

WindowSurface::WindowSurface(Display *display, const Config *config, EGLNativeWindowType window)
	: Surface(display, config), window(window)
{
	pixelAspectRatio = (EGLint)(1.0 * EGL_DISPLAY_SCALING);   // FIXME: Determine actual pixel aspect ratio
}

WindowSurface::~WindowSurface()
{
	WindowSurface::deleteResources();
}

bool WindowSurface::initialize()
{
	ASSERT(!frameBuffer && !backBuffer && !depthStencil);

	return checkForResize();
}

void WindowSurface::swap()
{
	if(backBuffer && frameBuffer)
	{
		frameBuffer->flip(backBuffer);

		checkForResize();
	}
}

EGLNativeWindowType WindowSurface::getWindowHandle() const
{
	return window;
}

bool WindowSurface::checkForResize()
{
	#if defined(_WIN32)
		RECT client;
		BOOL status = GetClientRect(window, &client);

		if(status == 0)
		{
			return error(EGL_BAD_NATIVE_WINDOW, false);
		}

		int windowWidth = client.right - client.left;
		int windowHeight = client.bottom - client.top;
	#elif defined(__ANDROID__)
		int windowWidth;  window->query(window, NATIVE_WINDOW_WIDTH, &windowWidth);
		int windowHeight; window->query(window, NATIVE_WINDOW_HEIGHT, &windowHeight);
	#elif defined(__linux__)
		XWindowAttributes windowAttributes;
		Status status = libX11->XGetWindowAttributes((::Display*)display->getNativeDisplay(), window, &windowAttributes);

		if(status == 0)
		{
			return error(EGL_BAD_NATIVE_WINDOW, false);
		}

		int windowWidth = windowAttributes.width;
		int windowHeight = windowAttributes.height;
	#elif defined(__APPLE__)
		int windowWidth;
		int windowHeight;
		sw::OSX::GetNativeWindowSize(window, windowWidth, windowHeight);
	#elif defined(__Fuchsia__)
		// TODO(crbug.com/800951): Integrate with Mozart.
		int windowWidth = 100;
		int windowHeight = 100;
	#else
		#error "WindowSurface::checkForResize unimplemented for this platform"
	#endif

	if((windowWidth != width) || (windowHeight != height))
	{
		bool success = reset(windowWidth, windowHeight);

		if(getCurrentDrawSurface() == this)
		{
			getCurrentContext()->makeCurrent(this);
		}

		return success;
	}

	return true;   // Success
}

void WindowSurface::deleteResources()
{
	delete frameBuffer;
	frameBuffer = nullptr;

	Surface::deleteResources();
}

bool WindowSurface::reset(int backBufferWidth, int backBufferHeight)
{
	width = backBufferWidth;
	height = backBufferHeight;

	deleteResources();

	if(window)
	{
		if(libGLESv2)
		{
			frameBuffer = libGLESv2->createFrameBuffer(display->getNativeDisplay(), window, width, height);
		}
		else if(libGLES_CM)
		{
			frameBuffer = libGLES_CM->createFrameBuffer(display->getNativeDisplay(), window, width, height);
		}

		if(!frameBuffer)
		{
			ERR("Could not create frame buffer");
			deleteResources();
			return error(EGL_BAD_ALLOC, false);
		}
	}

	return Surface::initialize();
}

PBufferSurface::PBufferSurface(Display *display, const Config *config, EGLint width, EGLint height,
                               EGLenum textureFormat, EGLenum textureTarget, EGLenum clientBufferFormat,
                               EGLenum clientBufferType, EGLBoolean largestPBuffer, EGLClientBuffer clientBuffer,
                               EGLint clientBufferPlane)
	: Surface(display, config)
{
	this->width = width;
	this->height = height;
	this->largestPBuffer = largestPBuffer;
	this->textureFormat = textureFormat;
	this->textureTarget = textureTarget;
	this->clientBufferFormat = clientBufferFormat;
	this->clientBufferType = clientBufferType;
	this->clientBuffer = clientBuffer;
	this->clientBufferPlane = clientBufferPlane;
}

PBufferSurface::~PBufferSurface()
{
	PBufferSurface::deleteResources();
}

void PBufferSurface::swap()
{
	// No effect
}

EGLNativeWindowType PBufferSurface::getWindowHandle() const
{
	UNREACHABLE(-1);   // Should not be called. Only WindowSurface has a window handle.

	return 0;
}

void PBufferSurface::deleteResources()
{
	Surface::deleteResources();
}

}
