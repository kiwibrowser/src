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

// Query.cpp: Implements the gl::Query class

#include "Query.h"

#include "main.h"
#include "Common/Thread.hpp"

namespace gl
{

Query::Query(GLuint name, GLenum type) : NamedObject(name)
{
	mQuery = nullptr;
	mStatus = GL_FALSE;
	mResult = GL_FALSE;
	mType = type;
}

Query::~Query()
{
	delete mQuery;
}

void Query::begin()
{
	if(!mQuery)
	{
		sw::Query::Type type;
		switch(mType)
		{
		case GL_ANY_SAMPLES_PASSED:
		case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
			type = sw::Query::FRAGMENTS_PASSED;
			break;
		default:
			ASSERT(false);
		}

		mQuery = new sw::Query(type);

		if(!mQuery)
		{
			return error(GL_OUT_OF_MEMORY);
		}
	}

	Device *device = getDevice();

	mQuery->begin();
	device->addQuery(mQuery);
	device->setOcclusionEnabled(true);
}

void Query::end()
{
	if(!mQuery)
	{
		return error(GL_INVALID_OPERATION);
	}

	Device *device = getDevice();

	mQuery->end();
	device->removeQuery(mQuery);
	device->setOcclusionEnabled(false);

	mStatus = GL_FALSE;
	mResult = GL_FALSE;
}

GLuint Query::getResult()
{
	if(mQuery)
	{
		while(!testQuery())
		{
			sw::Thread::yield();
		}
	}

	return (GLuint)mResult;
}

GLboolean Query::isResultAvailable()
{
	if(mQuery)
	{
		testQuery();
	}

	return mStatus;
}

GLenum Query::getType() const
{
	return mType;
}

GLboolean Query::testQuery()
{
	if(mQuery && mStatus != GL_TRUE)
	{
		if(!mQuery->building && mQuery->reference == 0)
		{
			unsigned int numPixels = mQuery->data;
			mStatus = GL_TRUE;

			switch(mType)
			{
			case GL_ANY_SAMPLES_PASSED:
			case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
				mResult = (numPixels > 0) ? GL_TRUE : GL_FALSE;
				break;
			default:
				ASSERT(false);
			}
		}

		return mStatus;
	}

	return GL_TRUE;   // Prevent blocking when query is null
}
}
