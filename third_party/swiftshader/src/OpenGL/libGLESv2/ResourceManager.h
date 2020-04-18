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

#ifndef LIBGLESV2_RESOURCEMANAGER_H_
#define LIBGLESV2_RESOURCEMANAGER_H_

#include "common/NameSpace.hpp"

#include <GLES2/gl2.h>

#include <map>

namespace es2
{
class Buffer;
class Shader;
class Program;
class Texture;
class Renderbuffer;
class Sampler;
class FenceSync;

enum TextureType
{
	TEXTURE_2D,
	TEXTURE_3D,
	TEXTURE_2D_ARRAY,
	TEXTURE_CUBE,
	TEXTURE_2D_RECT,
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
	GLuint createShader(GLenum type);
	GLuint createProgram();
	GLuint createTexture();
	GLuint createRenderbuffer();
	GLuint createSampler();
	GLuint createFenceSync(GLenum condition, GLbitfield flags);

	void deleteBuffer(GLuint buffer);
	void deleteShader(GLuint shader);
	void deleteProgram(GLuint program);
	void deleteTexture(GLuint texture);
	void deleteRenderbuffer(GLuint renderbuffer);
	void deleteSampler(GLuint sampler);
	void deleteFenceSync(GLuint fenceSync);

	Buffer *getBuffer(GLuint handle);
	Shader *getShader(GLuint handle);
	Program *getProgram(GLuint handle);
	Texture *getTexture(GLuint handle);
	Renderbuffer *getRenderbuffer(GLuint handle);
	Sampler *getSampler(GLuint handle);
	FenceSync *getFenceSync(GLuint handle);

	void checkBufferAllocation(unsigned int buffer);
	void checkTextureAllocation(GLuint texture, TextureType type);
	void checkRenderbufferAllocation(GLuint handle);
	void checkSamplerAllocation(GLuint sampler);

	bool isSampler(GLuint sampler);

private:
	std::size_t mRefCount;

	gl::NameSpace<Buffer> mBufferNameSpace;
	gl::NameSpace<Program> mProgramNameSpace;
	gl::NameSpace<Shader> mShaderNameSpace;
	gl::NameSpace<void> mProgramShaderNameSpace;   // Shaders and programs share a namespace
	gl::NameSpace<Texture> mTextureNameSpace;
	gl::NameSpace<Renderbuffer> mRenderbufferNameSpace;
	gl::NameSpace<Sampler> mSamplerNameSpace;
	gl::NameSpace<FenceSync> mFenceSyncNameSpace;
};

}

#endif // LIBGLESV2_RESOURCEMANAGER_H_
