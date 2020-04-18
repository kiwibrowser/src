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

// main.h: Management of thread-local data.

#ifndef LIBEGL_MAIN_H_
#define LIBEGL_MAIN_H_

#include "libGLES_CM/libGLES_CM.hpp"
#include "libGLESv2/libGLESv2.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace egl
{
	class Display;
	class Context;
	class Surface;
	class Config;
	class Image;

	struct Current
	{
		EGLint error;
		EGLenum API;
		Context *context;
		Surface *drawSurface;
		Surface *readSurface;
	};

	void detachThread();

	void setCurrentError(EGLint error);
	EGLint getCurrentError();

	void setCurrentAPI(EGLenum API);
	EGLenum getCurrentAPI();

	void setCurrentContext(Context *ctx);
	Context *getCurrentContext();

	void setCurrentDrawSurface(Surface *surface);
	Surface *getCurrentDrawSurface();

	void setCurrentReadSurface(Surface *surface);
	Surface *getCurrentReadSurface();

	void error(EGLint errorCode);

	template<class T>
	const T &error(EGLint errorCode, const T &returnValue)
	{
		egl::error(errorCode);

		return returnValue;
	}

	template<class T>
	const T &success(const T &returnValue)
	{
		egl::setCurrentError(EGL_SUCCESS);

		return returnValue;
	}
}

extern LibGLES_CM libGLES_CM;
extern LibGLESv2 libGLESv2;

#endif  // LIBEGL_MAIN_H_
