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

// ResourceManager.h : Defines the ResourceManager class, which tracks objects
// shared by multiple GL contexts.

#ifndef LIBGLES_CM_RESOURCEMANAGER_H_
#define LIBGLES_CM_RESOURCEMANAGER_H_

#include "common/NameSpace.hpp"

#include <GLES/gl.h>

#include <map>

namespace es1
{
class Buffer;
class Texture;
class Renderbuffer;

enum TextureType
{
	TEXTURE_2D,
	TEXTURE_EXTERNAL,

	TEXTURE_TYPE_COUNT,
	TEXTURE_UNKNOWN
};

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	void addRef();
	void release();

	GLuint createBuffer();
	GLuint createTexture();
	GLuint createRenderbuffer();

	void deleteBuffer(GLuint buffer);
	void deleteTexture(GLuint texture);
	void deleteRenderbuffer(GLuint renderbuffer);

	Buffer *getBuffer(GLuint handle);
	Texture *getTexture(GLuint handle);
	Renderbuffer *getRenderbuffer(GLuint handle);

	void checkBufferAllocation(unsigned int buffer);
	void checkTextureAllocation(GLuint texture, TextureType type);
	void checkRenderbufferAllocation(GLuint handle);

private:
	std::size_t mRefCount;

	gl::NameSpace<Buffer> mBufferNameSpace;
	gl::NameSpace<Texture> mTextureNameSpace;
	gl::NameSpace<Renderbuffer> mRenderbufferNameSpace;
};

}

#endif // LIBGLES_CM_RESOURCEMANAGER_H_
