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

// TransformFeedback.h: Defines the es2::TransformFeedback class

#ifndef LIBGLESV2_TRANSFORM_FEEDBACK_H_
#define LIBGLESV2_TRANSFORM_FEEDBACK_H_

#include "Buffer.h"
#include "Context.h"
#include "common/Object.hpp"
#include "Renderer/Renderer.hpp"

#include <GLES2/gl2.h>

namespace es2
{

class TransformFeedback : public gl::NamedObject
{
public:
	TransformFeedback(GLuint name);
	~TransformFeedback();

	BufferBinding* getBuffers() { return mBuffer; }

	Buffer* getGenericBuffer() const;
	Buffer* getBuffer(GLuint index) const;
	GLuint getGenericBufferName() const;
	GLuint getBufferName(GLuint index) const;
	int getOffset(GLuint index) const;
	int getSize(GLuint index) const;
	bool isActive() const;
	bool isPaused() const;
	GLenum primitiveMode() const;
	int vertexOffset() const;

	void setGenericBuffer(Buffer* buffer);
	void setBuffer(GLuint index, Buffer* buffer);
	void setBuffer(GLuint index, Buffer* buffer, GLintptr offset, GLsizeiptr size);
	void detachBuffer(GLuint buffer);
	void begin(GLenum primitiveMode);
	void end();
	void setPaused(bool paused);
	void addVertexOffset(int count);

private:
	gl::BindingPointer<Buffer> mGenericBuffer;
	BufferBinding mBuffer[MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS];

	bool mActive;
	bool mPaused;
	GLenum mPrimitiveMode;
	int mVertexOffset;
};

}

#endif // LIBGLESV2_TRANSFORM_FEEDBACK_H_
