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

// ResourceManager.cpp: Implements the ResourceManager class, which tracks and
// retrieves objects which may be shared by multiple Contexts.

#include "ResourceManager.h"

#include "Buffer.h"
#include "Fence.h"
#include "Program.h"
#include "Renderbuffer.h"
#include "Sampler.h"
#include "Shader.h"
#include "Texture.h"

namespace es2
{
ResourceManager::ResourceManager()
{
	mRefCount = 1;
}

ResourceManager::~ResourceManager()
{
	while(!mBufferNameSpace.empty())
	{
		deleteBuffer(mBufferNameSpace.firstName());
	}

	while(!mProgramNameSpace.empty())
	{
		deleteProgram(mProgramNameSpace.firstName());
	}

	while(!mShaderNameSpace.empty())
	{
		deleteShader(mShaderNameSpace.firstName());
	}

	while(!mRenderbufferNameSpace.empty())
	{
		deleteRenderbuffer(mRenderbufferNameSpace.firstName());
	}

	while(!mTextureNameSpace.empty())
	{
		deleteTexture(mTextureNameSpace.firstName());
	}

	while(!mSamplerNameSpace.empty())
	{
		deleteSampler(mSamplerNameSpace.firstName());
	}

	while(!mFenceSyncNameSpace.empty())
	{
		deleteFenceSync(mFenceSyncNameSpace.firstName());
	}
}

void ResourceManager::addRef()
{
	mRefCount++;
}

void ResourceManager::release()
{
	if(--mRefCount == 0)
	{
		delete this;
	}
}

// Returns an unused buffer name
GLuint ResourceManager::createBuffer()
{
	return mBufferNameSpace.allocate();
}

// Returns an unused shader name
GLuint ResourceManager::createShader(GLenum type)
{
	GLuint name = mProgramShaderNameSpace.allocate();

	if(type == GL_VERTEX_SHADER)
	{
		mShaderNameSpace.insert(name, new VertexShader(this, name));
	}
	else if(type == GL_FRAGMENT_SHADER)
	{
		mShaderNameSpace.insert(name, new FragmentShader(this, name));
	}
	else UNREACHABLE(type);

	return name;
}

// Returns an unused program name
GLuint ResourceManager::createProgram()
{
	GLuint name = mProgramShaderNameSpace.allocate();

	mProgramNameSpace.insert(name, new Program(this, name));

	return name;
}

// Returns an unused texture name
GLuint ResourceManager::createTexture()
{
	return mTextureNameSpace.allocate();
}

// Returns an unused renderbuffer name
GLuint ResourceManager::createRenderbuffer()
{
	return mRenderbufferNameSpace.allocate();
}

// Returns an unused sampler name
GLuint ResourceManager::createSampler()
{
	return mSamplerNameSpace.allocate();
}

// Returns the next unused fence name, and allocates the fence
GLuint ResourceManager::createFenceSync(GLenum condition, GLbitfield flags)
{
	GLuint name = mFenceSyncNameSpace.allocate();

	FenceSync *fenceSync = new FenceSync(name, condition, flags);
	fenceSync->addRef();

	mFenceSyncNameSpace.insert(name, fenceSync);

	return name;
}

void ResourceManager::deleteBuffer(GLuint buffer)
{
	Buffer *bufferObject = mBufferNameSpace.remove(buffer);

	if(bufferObject)
	{
		bufferObject->release();
	}
}

void ResourceManager::deleteShader(GLuint shader)
{
	Shader *shaderObject = mShaderNameSpace.find(shader);

	if(shaderObject)
	{
		if(shaderObject->getRefCount() == 0)
		{
			delete shaderObject;
			mShaderNameSpace.remove(shader);
			mProgramShaderNameSpace.remove(shader);
		}
		else
		{
			shaderObject->flagForDeletion();
		}
	}
}

void ResourceManager::deleteProgram(GLuint program)
{
	Program *programObject = mProgramNameSpace.find(program);

	if(programObject)
	{
		if(programObject->getRefCount() == 0)
		{
			delete programObject;
			mProgramNameSpace.remove(program);
			mProgramShaderNameSpace.remove(program);
		}
		else
		{
			programObject->flagForDeletion();
		}
	}
}

void ResourceManager::deleteTexture(GLuint texture)
{
	Texture *textureObject = mTextureNameSpace.remove(texture);

	if(textureObject)
	{
		textureObject->release();
	}
}

void ResourceManager::deleteRenderbuffer(GLuint renderbuffer)
{
	Renderbuffer *renderbufferObject = mRenderbufferNameSpace.remove(renderbuffer);

	if(renderbufferObject)
	{
		renderbufferObject->release();
	}
}

void ResourceManager::deleteSampler(GLuint sampler)
{
	Sampler *samplerObject = mSamplerNameSpace.remove(sampler);

	if(samplerObject)
	{
		samplerObject->release();
	}
}

void ResourceManager::deleteFenceSync(GLuint fenceSync)
{
	FenceSync *fenceObject = mFenceSyncNameSpace.remove(fenceSync);

	if(fenceObject)
	{
		fenceObject->release();
	}
}

Buffer *ResourceManager::getBuffer(unsigned int handle)
{
	return mBufferNameSpace.find(handle);
}

Shader *ResourceManager::getShader(unsigned int handle)
{
	return mShaderNameSpace.find(handle);
}

Texture *ResourceManager::getTexture(unsigned int handle)
{
	return mTextureNameSpace.find(handle);
}

Program *ResourceManager::getProgram(unsigned int handle)
{
	return mProgramNameSpace.find(handle);
}

Renderbuffer *ResourceManager::getRenderbuffer(unsigned int handle)
{
	return mRenderbufferNameSpace.find(handle);
}

Sampler *ResourceManager::getSampler(unsigned int handle)
{
	return mSamplerNameSpace.find(handle);
}

FenceSync *ResourceManager::getFenceSync(unsigned int handle)
{
	return mFenceSyncNameSpace.find(handle);
}

void ResourceManager::checkBufferAllocation(unsigned int buffer)
{
	if(buffer != 0 && !getBuffer(buffer))
	{
		Buffer *bufferObject = new Buffer(buffer);
		bufferObject->addRef();

		mBufferNameSpace.insert(buffer, bufferObject);
	}
}

void ResourceManager::checkTextureAllocation(GLuint texture, TextureType type)
{
	if(!getTexture(texture) && texture != 0)
	{
		Texture *textureObject;

		if(type == TEXTURE_2D)
		{
			textureObject = new Texture2D(texture);
		}
		else if(type == TEXTURE_CUBE)
		{
			textureObject = new TextureCubeMap(texture);
		}
		else if(type == TEXTURE_EXTERNAL)
		{
			textureObject = new TextureExternal(texture);
		}
		else if(type == TEXTURE_3D)
		{
			textureObject = new Texture3D(texture);
		}
		else if(type == TEXTURE_2D_ARRAY)
		{
			textureObject = new Texture2DArray(texture);
		}
		else if(type == TEXTURE_2D_RECT)
		{
			textureObject = new Texture2DRect(texture);
		}
		else
		{
			UNREACHABLE(type);
			return;
		}

		textureObject->addRef();

		mTextureNameSpace.insert(texture, textureObject);
	}
}

void ResourceManager::checkRenderbufferAllocation(GLuint handle)
{
	if(handle != 0 && !getRenderbuffer(handle))
	{
		Renderbuffer *renderbufferObject = new Renderbuffer(handle, new Colorbuffer(0, 0, GL_NONE, 0));
		renderbufferObject->addRef();

		mRenderbufferNameSpace.insert(handle, renderbufferObject);
	}
}

void ResourceManager::checkSamplerAllocation(GLuint sampler)
{
	if(sampler != 0 && !getSampler(sampler))
	{
		Sampler *samplerObject = new Sampler(sampler);
		samplerObject->addRef();

		mSamplerNameSpace.insert(sampler, samplerObject);
	}
}

bool ResourceManager::isSampler(GLuint sampler)
{
	return mSamplerNameSpace.isReserved(sampler);
}

}
