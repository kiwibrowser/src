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

// Sync.hpp: Defines sync objects for the EGL_KHR_fence_sync extension.

#ifndef LIBEGL_SYNC_H_
#define LIBEGL_SYNC_H_

#include "Context.hpp"

#include <EGL/eglext.h>

namespace egl
{

class FenceSync
{
public:
	explicit FenceSync(Context *context) : context(context)
	{
		status = EGL_UNSIGNALED_KHR;
		context->addRef();
	}

	~FenceSync()
	{
		context->release();
		context = nullptr;
	}

	void wait() { context->finish(); signal(); }
	void signal() { status = EGL_SIGNALED_KHR; }
	bool isSignaled() const { return status == EGL_SIGNALED_KHR; }

private:
	EGLint status;
	Context *context;
};

}

#endif   // LIBEGL_SYNC_H_
