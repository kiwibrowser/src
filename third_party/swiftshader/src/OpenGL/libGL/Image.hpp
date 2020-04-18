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

#ifndef gl_Image_hpp
#define gl_Image_hpp

#include "Renderer/Surface.hpp"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

namespace gl
{
	class Texture;

	class Image : public sw::Surface
	{
	public:
		Image(Texture *parentTexture, GLsizei width, GLsizei height, GLenum format, GLenum type);
		Image(Texture *parentTexture, GLsizei width, GLsizei height, sw::Format internalFormat, int multiSampleDepth, bool lockable, bool renderTarget);

		void loadImageData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLint unpackAlignment, const void *input);
		void loadCompressedData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels);

		void *lock(unsigned int left, unsigned int top, sw::Lock lock);
		unsigned int getPitch() const;
		void unlock();

		void *lockInternal(int x, int y, int z, sw::Lock lock, sw::Accessor client) override;
		void unlockInternal() override;

		int getWidth();
		int getHeight();
		GLenum getFormat();
		GLenum getType();
		virtual sw::Format getInternalFormat();
		int getMultiSampleDepth();

		virtual void addRef();
		virtual void release();
		void unbind();   // Break parent ownership and release

		static sw::Format selectInternalFormat(GLenum format, GLenum type);

	private:
		~Image() override;

		void loadAlphaImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadAlphaFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadAlphaHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadLuminanceImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadLuminanceFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadLuminanceHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadLuminanceAlphaImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadLuminanceAlphaFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadLuminanceAlphaHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGBUByteImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGB565ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGBFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGBHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGBAUByteImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGBA4444ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGBA5551ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGBAFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadRGBAHalfFloatImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadBGRAImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadD16ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadD32ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer) const;
		void loadD24S8ImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, int inputPitch, const void *input, void *buffer);

		Texture *parentTexture;

		const GLsizei width;
		const GLsizei height;
		const GLenum format;
		const GLenum type;
		const sw::Format internalFormat;
		const int multiSampleDepth;

		volatile int referenceCount;
	};
}

#endif   // gl_Image_hpp
