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

// Fence.cpp: Implements the Fence class, which supports the GL_NV_fence extension.

#include "Fence.h"

#include "main.h"
#include "Common/Thread.hpp"

namespace es2
{

Fence::Fence()
{
	mQuery = false;
	mCondition = GL_NONE;
	mStatus = GL_FALSE;
}

Fence::~Fence()
{
	mQuery = false;
}

GLboolean Fence::isFence()
{
	// GL_NV_fence spec:
	// A name returned by GenFencesNV, but not yet set via SetFenceNV, is not the name of an existing fence.
	return mQuery;
}

void Fence::setFence(GLenum condition)
{
	if(condition != GL_ALL_COMPLETED_NV)
	{
		return error(GL_INVALID_VALUE);
	}

	mQuery = true;
	mCondition = condition;
	mStatus = GL_FALSE;
}

GLboolean Fence::testFence()
{
	if(!mQuery)
	{
		return error(GL_INVALID_OPERATION, GL_TRUE);
	}

	// The current assumtion is that no matter where the fence is placed, it is
	// done by the time it is tested, which is similar to Context::flush(), since
	// we don't queue anything without processing it as fast as possible.
	mStatus = GL_TRUE;

	return mStatus;
}

void Fence::finishFence()
{
	if(!mQuery)
	{
		return error(GL_INVALID_OPERATION);
	}

	while(!testFence())
	{
		sw::Thread::yield();
	}
}

void Fence::getFenceiv(GLenum pname, GLint *params)
{
	if(!mQuery)
	{
		return error(GL_INVALID_OPERATION);
	}

	switch(pname)
	{
	case GL_FENCE_STATUS_NV:
		{
			// GL_NV_fence spec:
			// Once the status of a fence has been finished (via FinishFenceNV) or tested and the returned status is TRUE (via either TestFenceNV
			// or GetFenceivNV querying the FENCE_STATUS_NV), the status remains TRUE until the next SetFenceNV of the fence.
			if(mStatus)
			{
				params[0] = GL_TRUE;
				return;
			}

			mStatus = testFence();

			params[0] = mStatus;
			break;
		}
	case GL_FENCE_CONDITION_NV:
		params[0] = mCondition;
		break;
	default:
		return error(GL_INVALID_ENUM);
		break;
	}
}

FenceSync::FenceSync(GLuint name, GLenum condition, GLbitfield flags) : NamedObject(name), mCondition(condition), mFlags(flags)
{
}

FenceSync::~FenceSync()
{
}

GLenum FenceSync::clientWait(GLbitfield flags, GLuint64 timeout)
{
	// The current assumtion is that no matter where the fence is placed, it is
	// done by the time it is tested, which is similar to Context::flush(), since
	// we don't queue anything without processing it as fast as possible.
	return GL_ALREADY_SIGNALED;
}

void FenceSync::serverWait(GLbitfield flags, GLuint64 timeout)
{
}

void FenceSync::getSynciv(GLenum pname, GLsizei *length, GLint *values)
{
	switch(pname)
	{
	case GL_OBJECT_TYPE:
		values[0] = GL_SYNC_FENCE;
		if(length) {
			*length = 1;
		}
		break;
	case GL_SYNC_STATUS:
		// The current assumtion is that no matter where the fence is placed, it is
		// done by the time it is tested, which is similar to Context::flush(), since
		// we don't queue anything without processing it as fast as possible.
		values[0] = GL_SIGNALED;
		if(length) {
			*length = 1;
		}
		break;
	case GL_SYNC_CONDITION:
		values[0] = GL_SYNC_GPU_COMMANDS_COMPLETE;
		if(length) {
			*length = 1;
		}
		break;
	case GL_SYNC_FLAGS:
		if(length) {
			*length = 0;
		}
		break;
	default:
		return error(GL_INVALID_ENUM);
	}
}

}
