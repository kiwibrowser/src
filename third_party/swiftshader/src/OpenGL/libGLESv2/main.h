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

#ifndef LIBGLESV2_MAIN_H_
#define LIBGLESV2_MAIN_H_

#include "Context.h"
#include "Device.hpp"
#include "common/debug.h"
#include "libEGL/libEGL.hpp"
#include "libEGL/Display.h"
#include "libGLES_CM/libGLES_CM.hpp"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>

namespace es2
{
	Context *getContext();
	Device *getDevice();

	void error(GLenum errorCode);

	template<class T>
	const T &error(GLenum errorCode, const T &returnValue)
	{
		error(errorCode);

		return returnValue;
	}
}

namespace egl
{
	GLint getClientVersion();
}

extern LibEGL libEGL;
extern LibGLES_CM libGLES_CM;

#endif   // LIBGLESV2_MAIN_H_
