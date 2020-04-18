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

// main.cpp: DLLMain and management of thread-local data.

#include "main.h"

#if !defined(_MSC_VER)
#define CONSTRUCTOR __attribute__((constructor))
#define DESTRUCTOR __attribute__((destructor))
#else
#define CONSTRUCTOR
#define DESTRUCTOR
#endif

static void glAttachThread()
{
	TRACE("()");
}

static void glDetachThread()
{
	TRACE("()");
}

CONSTRUCTOR static void glAttachProcess()
{
	TRACE("()");

	glAttachThread();
}

DESTRUCTOR static void glDetachProcess()
{
	TRACE("()");

	glDetachThread();
}

#if defined(_WIN32)
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch(reason)
	{
	case DLL_PROCESS_ATTACH:
		glAttachProcess();
		break;
	case DLL_THREAD_ATTACH:
		glAttachThread();
		break;
	case DLL_THREAD_DETACH:
		glDetachThread();
		break;
	case DLL_PROCESS_DETACH:
		glDetachProcess();
		break;
	default:
		break;
	}

	return TRUE;
}
#endif

namespace es2
{
es2::Context *getContext()
{
	egl::Context *context = libEGL->clientGetCurrentContext();

	if(context && (context->getClientVersion() == 2 ||
	               context->getClientVersion() == 3))
	{
		return static_cast<es2::Context*>(context);
	}

	return nullptr;
}

Device *getDevice()
{
	Context *context = getContext();

	return context ? context->getDevice() : nullptr;
}

// Records an error code
void error(GLenum errorCode)
{
	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(errorCode)
		{
		case GL_INVALID_ENUM:
			context->recordInvalidEnum();
			TRACE("\t! Error generated: invalid enum\n");
			break;
		case GL_INVALID_VALUE:
			context->recordInvalidValue();
			TRACE("\t! Error generated: invalid value\n");
			break;
		case GL_INVALID_OPERATION:
			context->recordInvalidOperation();
			TRACE("\t! Error generated: invalid operation\n");
			break;
		case GL_OUT_OF_MEMORY:
			context->recordOutOfMemory();
			TRACE("\t! Error generated: out of memory\n");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			context->recordInvalidFramebufferOperation();
			TRACE("\t! Error generated: invalid framebuffer operation\n");
			break;
		default: UNREACHABLE(errorCode);
		}
	}
}
}

namespace egl
{
GLint getClientVersion()
{
	Context *context = libEGL->clientGetCurrentContext();

	return context ? context->getClientVersion() : 0;
}
}
