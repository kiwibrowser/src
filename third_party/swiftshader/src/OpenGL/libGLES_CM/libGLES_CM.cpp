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

// libGLES_CM.cpp: Implements the exported OpenGL ES 1.1 functions.

#include "main.h"
#include "mathutil.h"
#include "utilities.h"
#include "Buffer.h"
#include "Context.h"
#include "Framebuffer.h"
#include "Renderbuffer.h"
#include "Texture.h"
#include "common/debug.h"
#include "Common/SharedLibrary.hpp"
#include "Common/Version.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

#include <algorithm>
#include <limits>

namespace es1
{

static bool validImageSize(GLint level, GLsizei width, GLsizei height)
{
	if(level < 0 || level >= es1::IMPLEMENTATION_MAX_TEXTURE_LEVELS || width < 0 || height < 0)
	{
		return false;
	}

	return true;
}

void ActiveTexture(GLenum texture)
{
	TRACE("(GLenum texture = 0x%X)", texture);

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(texture < GL_TEXTURE0 || texture > GL_TEXTURE0 + es1::MAX_TEXTURE_UNITS - 1)
		{
			return error(GL_INVALID_ENUM);
		}

		context->setActiveSampler(texture - GL_TEXTURE0);
	}
}

void AlphaFunc(GLenum func, GLclampf ref)
{
	TRACE("(GLenum func = 0x%X, GLclampf ref = %f)", func, ref);

	switch(func)
	{
	case GL_NEVER:
	case GL_ALWAYS:
	case GL_LESS:
	case GL_LEQUAL:
	case GL_EQUAL:
	case GL_GEQUAL:
	case GL_GREATER:
	case GL_NOTEQUAL:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setAlphaFunc(func, clamp01(ref));
	}
}

void AlphaFuncx(GLenum func, GLclampx ref)
{
	AlphaFunc(func, (float)ref / 0x10000);
}

void BindBuffer(GLenum target, GLuint buffer)
{
	TRACE("(GLenum target = 0x%X, GLuint buffer = %d)", target, buffer);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_ARRAY_BUFFER:
			context->bindArrayBuffer(buffer);
			return;
		case GL_ELEMENT_ARRAY_BUFFER:
			context->bindElementArrayBuffer(buffer);
			return;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void BindFramebuffer(GLenum target, GLuint framebuffer)
{
	TRACE("(GLenum target = 0x%X, GLuint framebuffer = %d)", target, framebuffer);

	if(target != GL_FRAMEBUFFER_OES)
	{
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->bindFramebuffer(framebuffer);
	}
}

void BindFramebufferOES(GLenum target, GLuint framebuffer)
{
	TRACE("(GLenum target = 0x%X, GLuint framebuffer = %d)", target, framebuffer);

	if(target != GL_FRAMEBUFFER_OES)
	{
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->bindFramebuffer(framebuffer);
	}
}

void BindRenderbufferOES(GLenum target, GLuint renderbuffer)
{
	TRACE("(GLenum target = 0x%X, GLuint renderbuffer = %d)", target, renderbuffer);

	if(target != GL_RENDERBUFFER_OES)
	{
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		// [GL_EXT_framebuffer_object]
		// If <renderbuffer> is not zero, then the resulting renderbuffer object
		// is a new state vector, initialized with a zero-sized memory buffer
		context->bindRenderbuffer(renderbuffer);
	}
}

void BindTexture(GLenum target, GLuint texture)
{
	TRACE("(GLenum target = 0x%X, GLuint texture = %d)", target, texture);

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Texture *textureObject = context->getTexture(texture);

		if(textureObject && textureObject->getTarget() != target && texture != 0)
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(target)
		{
		case GL_TEXTURE_2D:
			context->bindTexture(TEXTURE_2D, texture);
			break;
		case GL_TEXTURE_EXTERNAL_OES:
			context->bindTexture(TEXTURE_EXTERNAL, texture);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void BlendEquationSeparateOES(GLenum modeRGB, GLenum modeAlpha);

void BlendEquationOES(GLenum mode)
{
	BlendEquationSeparateOES(mode, mode);
}

void BlendEquationSeparateOES(GLenum modeRGB, GLenum modeAlpha)
{
	TRACE("(GLenum modeRGB = 0x%X, GLenum modeAlpha = 0x%X)", modeRGB, modeAlpha);

	switch(modeRGB)
	{
	case GL_FUNC_ADD_OES:
	case GL_FUNC_SUBTRACT_OES:
	case GL_FUNC_REVERSE_SUBTRACT_OES:
	case GL_MIN_EXT:
	case GL_MAX_EXT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(modeAlpha)
	{
	case GL_FUNC_ADD_OES:
	case GL_FUNC_SUBTRACT_OES:
	case GL_FUNC_REVERSE_SUBTRACT_OES:
	case GL_MIN_EXT:
	case GL_MAX_EXT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setBlendEquation(modeRGB, modeAlpha);
	}
}

void BlendFuncSeparateOES(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);

void BlendFunc(GLenum sfactor, GLenum dfactor)
{
	BlendFuncSeparateOES(sfactor, dfactor, sfactor, dfactor);
}

void BlendFuncSeparateOES(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	TRACE("(GLenum srcRGB = 0x%X, GLenum dstRGB = 0x%X, GLenum srcAlpha = 0x%X, GLenum dstAlpha = 0x%X)",
		  srcRGB, dstRGB, srcAlpha, dstAlpha);

	switch(srcRGB)
	{
	case GL_ZERO:
	case GL_ONE:
	case GL_SRC_COLOR:
	case GL_ONE_MINUS_SRC_COLOR:
	case GL_DST_COLOR:
	case GL_ONE_MINUS_DST_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
	case GL_DST_ALPHA:
	case GL_ONE_MINUS_DST_ALPHA:
	case GL_SRC_ALPHA_SATURATE:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(dstRGB)
	{
	case GL_ZERO:
	case GL_ONE:
	case GL_SRC_COLOR:
	case GL_ONE_MINUS_SRC_COLOR:
	case GL_DST_COLOR:
	case GL_ONE_MINUS_DST_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
	case GL_DST_ALPHA:
	case GL_ONE_MINUS_DST_ALPHA:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(srcAlpha)
	{
	case GL_ZERO:
	case GL_ONE:
	case GL_SRC_COLOR:
	case GL_ONE_MINUS_SRC_COLOR:
	case GL_DST_COLOR:
	case GL_ONE_MINUS_DST_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
	case GL_DST_ALPHA:
	case GL_ONE_MINUS_DST_ALPHA:
	case GL_SRC_ALPHA_SATURATE:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(dstAlpha)
	{
	case GL_ZERO:
	case GL_ONE:
	case GL_SRC_COLOR:
	case GL_ONE_MINUS_SRC_COLOR:
	case GL_DST_COLOR:
	case GL_ONE_MINUS_DST_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
	case GL_DST_ALPHA:
	case GL_ONE_MINUS_DST_ALPHA:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setBlendFactors(srcRGB, dstRGB, srcAlpha, dstAlpha);
	}
}

void BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	size = static_cast<GLint>(size);   // Work around issues with some 64-bit applications

	TRACE("(GLenum target = 0x%X, GLsizeiptr size = %d, const GLvoid* data = %p, GLenum usage = %d)",
	      target, size, data, usage);

	if(size < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(usage)
	{
	case GL_STATIC_DRAW:
	case GL_DYNAMIC_DRAW:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Buffer *buffer;

		switch(target)
		{
		case GL_ARRAY_BUFFER:
			buffer = context->getArrayBuffer();
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			buffer = context->getElementArrayBuffer();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		if(!buffer)
		{
			return error(GL_INVALID_OPERATION);
		}

		buffer->bufferData(data, size, usage);
	}
}

void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	size = static_cast<GLint>(size);   // Work around issues with some 64-bit applications
	offset = static_cast<GLint>(offset);

	TRACE("(GLenum target = 0x%X, GLintptr offset = %d, GLsizeiptr size = %d, const GLvoid* data = %p)",
	      target, offset, size, data);

	if(size < 0 || offset < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(!data)
	{
		return;
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Buffer *buffer;

		switch(target)
		{
		case GL_ARRAY_BUFFER:
			buffer = context->getArrayBuffer();
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			buffer = context->getElementArrayBuffer();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		if(!buffer)
		{
			return error(GL_INVALID_OPERATION);
		}

		if((size_t)size + offset > buffer->size())
		{
			return error(GL_INVALID_VALUE);
		}

		buffer->bufferSubData(data, size, offset);
	}
}

GLenum CheckFramebufferStatusOES(GLenum target)
{
	TRACE("(GLenum target = 0x%X)", target);

	if(target != GL_FRAMEBUFFER_OES)
	{
		return error(GL_INVALID_ENUM, 0);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Framebuffer *framebuffer = context->getFramebuffer();

		if(!framebuffer)
		{
			return GL_FRAMEBUFFER_UNDEFINED_OES;
		}

		return framebuffer->completeness();
	}

	return 0;
}

void Clear(GLbitfield mask)
{
	TRACE("(GLbitfield mask = %X)", mask);

	if((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->clear(mask);
	}
}

void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	TRACE("(GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f)",
	      red, green, blue, alpha);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setClearColor(red, green, blue, alpha);
	}
}

void ClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)
{
	ClearColor((float)red / 0x10000, (float)green / 0x10000, (float)blue / 0x10000, (float)alpha / 0x10000);
}

void ClearDepthf(GLclampf depth)
{
	TRACE("(GLclampf depth = %f)", depth);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setClearDepth(depth);
	}
}

void ClearDepthx(GLclampx depth)
{
	ClearDepthf((float)depth / 0x10000);
}

void ClearStencil(GLint s)
{
	TRACE("(GLint s = %d)", s);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setClearStencil(s);
	}
}

void ClientActiveTexture(GLenum texture)
{
	TRACE("(GLenum texture = 0x%X)", texture);

	switch(texture)
	{
	case GL_TEXTURE0:
	case GL_TEXTURE1:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->clientActiveTexture(texture);
	}
}

void ClipPlanef(GLenum plane, const GLfloat *equation)
{
	TRACE("(GLenum plane = 0x%X, const GLfloat *equation)", plane);

	int index = plane - GL_CLIP_PLANE0;

	if(index < 0 || index >= MAX_CLIP_PLANES)
	{
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setClipPlane(index, equation);
	}
}

void ClipPlanex(GLenum plane, const GLfixed *equation)
{
	GLfloat equationf[4] =
	{
		(float)equation[0] / 0x10000,
		(float)equation[1] / 0x10000,
		(float)equation[2] / 0x10000,
		(float)equation[3] / 0x10000,
	};

	ClipPlanef(plane, equationf);
}

void Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	TRACE("(GLfloat red = %f, GLfloat green = %f, GLfloat blue = %f, GLfloat alpha = %f)", red, green, blue, alpha);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setVertexAttrib(sw::Color0, red, green, blue, alpha);
	}
}

void Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	Color4f((float)red / 0xFF, (float)green / 0xFF, (float)blue / 0xFF, (float)alpha / 0xFF);
}

void Color4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
	Color4f((float)red / 0x10000, (float)green / 0x10000, (float)blue / 0x10000, (float)alpha / 0x10000);
}

void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	TRACE("(GLboolean red = %d, GLboolean green = %d, GLboolean blue = %d, GLboolean alpha = %d)",
	      red, green, blue, alpha);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setColorMask(red == GL_TRUE, green == GL_TRUE, blue == GL_TRUE, alpha == GL_TRUE);
	}
}

void VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	TRACE("(GLuint index = %d, GLint size = %d, GLenum type = 0x%X, "
	      "GLboolean normalized = %d, GLsizei stride = %d, const GLvoid* ptr = %p)",
	      index, size, type, normalized, stride, ptr);

	if(index >= es1::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	if(size < 1 || size > 4)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(type)
	{
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_FIXED:
	case GL_FLOAT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(stride < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setVertexAttribState(index, context->getArrayBuffer(), size, type, (normalized == GL_TRUE), stride, ptr);
	}
}

void ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(GLint size = %d, GLenum type = 0x%X, GLsizei stride = %d, const GLvoid *pointer = %p)", size, type, stride, pointer);

	if(size != 4)
	{
		return error(GL_INVALID_VALUE);
	}

	VertexAttribPointer(sw::Color0, size, type, true, stride, pointer);
}

void CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                          GLint border, GLsizei imageSize, const GLvoid* data)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLsizei width = %d, "
	      "GLsizei height = %d, GLint border = %d, GLsizei imageSize = %d, const GLvoid* data = %p)",
	      target, level, internalformat, width, height, border, imageSize, data);

	if(level < 0 || level >= es1::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
	{
		return error(GL_INVALID_VALUE);
	}

	if(!validImageSize(level, width, height) || imageSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(internalformat)
	{
	case GL_ETC1_RGB8_OES:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		break;
	case GL_DEPTH_COMPONENT16_OES:
	case GL_DEPTH_STENCIL_OES:
	case GL_DEPTH24_STENCIL8_OES:
		return error(GL_INVALID_OPERATION);
	default:
		return error(GL_INVALID_ENUM);
	}

	if(border != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_TEXTURE_2D:
			if(width > (es1::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
			   height > (es1::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
			{
				return error(GL_INVALID_VALUE);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		if(imageSize != gl::ComputeCompressedSize(width, height, internalformat))
		{
			return error(GL_INVALID_VALUE);
		}

		if(target == GL_TEXTURE_2D)
		{
			es1::Texture2D *texture = context->getTexture2D();

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			texture->setCompressedImage(level, internalformat, width, height, imageSize, data);
		}
		else UNREACHABLE(target);
	}
}

void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                             GLenum format, GLsizei imageSize, const GLvoid* data)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
	      "GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, "
	      "GLsizei imageSize = %d, const GLvoid* data = %p)",
	      target, level, xoffset, yoffset, width, height, format, imageSize, data);

	if(!es1::IsTextureTarget(target))
	{
		return error(GL_INVALID_ENUM);
	}

	if(level < 0 || level >= es1::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
	{
		return error(GL_INVALID_VALUE);
	}

	if(xoffset < 0 || yoffset < 0 || !validImageSize(level, width, height) || imageSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(format)
	{
	case GL_ETC1_RGB8_OES:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(width == 0 || height == 0 || !data)
	{
		return;
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(imageSize != gl::ComputeCompressedSize(width, height, format))
		{
			return error(GL_INVALID_VALUE);
		}

		if(xoffset % 4 != 0 || yoffset % 4 != 0)
		{
			// We wait to check the offsets until this point, because the multiple-of-four restriction does not exist unless DXT1 textures are supported
			return error(GL_INVALID_OPERATION);
		}

		if(target == GL_TEXTURE_2D)
		{
			es1::Texture2D *texture = context->getTexture2D();

			GLenum validationError = ValidateSubImageParams(true, false, target, level, xoffset, yoffset, width, height, format, GL_NONE_OES, texture);
			if(validationError != GL_NO_ERROR)
			{
				return error(validationError);
			}

			texture->subImageCompressed(level, xoffset, yoffset, width, height, format, imageSize, data);
		}
		else UNREACHABLE(target);
	}
}

void CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, "
	      "GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, GLint border = %d)",
	      target, level, internalformat, x, y, width, height, border);

	if(!validImageSize(level, width, height))
	{
		return error(GL_INVALID_VALUE);
	}

	if(border != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_TEXTURE_2D:
			if(width > (es1::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
			   height > (es1::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
			{
				return error(GL_INVALID_VALUE);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		es1::Framebuffer *framebuffer = context->getFramebuffer();

		if(!framebuffer || (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE_OES))
		{
			return error(GL_INVALID_FRAMEBUFFER_OPERATION_OES);
		}

		es1::Renderbuffer *source = framebuffer->getColorbuffer();

		if(!source || source->getSamples() > 1)
		{
			return error(GL_INVALID_OPERATION);
		}

		GLenum colorbufferFormat = source->getFormat();

		// [OpenGL ES 1.1.12] table 3.9
		switch(internalformat)
		{
		case GL_ALPHA:
			if(colorbufferFormat != GL_ALPHA &&
			   colorbufferFormat != GL_RGBA &&
			   colorbufferFormat != GL_RGBA4_OES &&
			   colorbufferFormat != GL_RGB5_A1_OES &&
			   colorbufferFormat != GL_RGBA8_OES)
			{
				return error(GL_INVALID_OPERATION);
			}
			break;
		case GL_LUMINANCE:
		case GL_RGB:
			if(colorbufferFormat != GL_RGB &&
			   colorbufferFormat != GL_RGB565_OES &&
			   colorbufferFormat != GL_RGB8_OES &&
			   colorbufferFormat != GL_RGBA &&
			   colorbufferFormat != GL_RGBA4_OES &&
			   colorbufferFormat != GL_RGB5_A1_OES &&
			   colorbufferFormat != GL_RGBA8_OES)
			{
				return error(GL_INVALID_OPERATION);
			}
			break;
		case GL_LUMINANCE_ALPHA:
		case GL_RGBA:
			if(colorbufferFormat != GL_RGBA &&
			   colorbufferFormat != GL_RGBA4_OES &&
			   colorbufferFormat != GL_RGB5_A1_OES &&
			   colorbufferFormat != GL_RGBA8_OES &&
			   colorbufferFormat != GL_BGRA_EXT &&  // GL_EXT_texture_format_BGRA8888
			   colorbufferFormat != GL_BGRA8_EXT)   // GL_EXT_texture_format_BGRA8888
			{
				return error(GL_INVALID_OPERATION);
			}
			break;
		case GL_ETC1_RGB8_OES:
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			return error(GL_INVALID_OPERATION);
		case GL_BGRA_EXT:   // GL_EXT_texture_format_BGRA8888 doesn't mention the format to be accepted by glCopyTexImage2D.
		default:
			return error(GL_INVALID_ENUM);
		}

		// Determine the sized internal format.
		if(gl::GetBaseInternalFormat(colorbufferFormat) == internalformat)
		{
			internalformat = colorbufferFormat;
		}
		else if(GetRedSize(colorbufferFormat) <= 8)
		{
			internalformat = gl::GetSizedInternalFormat(internalformat, GL_UNSIGNED_BYTE);
		}
		else
		{
			UNIMPLEMENTED();

			return error(GL_INVALID_OPERATION);
		}

		if(target == GL_TEXTURE_2D)
		{
			es1::Texture2D *texture = context->getTexture2D();

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			texture->copyImage(level, internalformat, x, y, width, height, framebuffer);
		}
		else UNREACHABLE(target);
	}
}

void CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
	      "GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)",
	      target, level, xoffset, yoffset, x, y, width, height);

	if(!es1::IsTextureTarget(target))
	{
		return error(GL_INVALID_ENUM);
	}

	if(level < 0 || level >= es1::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
	{
		return error(GL_INVALID_VALUE);
	}

	if(xoffset < 0 || yoffset < 0 || width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(std::numeric_limits<GLsizei>::max() - xoffset < width || std::numeric_limits<GLsizei>::max() - yoffset < height)
	{
		return error(GL_INVALID_VALUE);
	}

	if(width == 0 || height == 0)
	{
		return;
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Framebuffer *framebuffer = context->getFramebuffer();

		if(!framebuffer || (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE_OES))
		{
			return error(GL_INVALID_FRAMEBUFFER_OPERATION_OES);
		}

		es1::Renderbuffer *source = framebuffer->getColorbuffer();

		if(context->getFramebufferName() != 0 && (!source || source->getSamples() > 1))
		{
			return error(GL_INVALID_OPERATION);
		}

		es1::Texture *texture = nullptr;

		if(target == GL_TEXTURE_2D)
		{
			texture = context->getTexture2D();
		}
		else UNREACHABLE(target);

		GLenum validationError = ValidateSubImageParams(false, true, target, level, xoffset, yoffset, width, height, GL_NONE_OES, GL_NONE_OES, texture);
		if(validationError != GL_NO_ERROR)
		{
			return error(validationError);
		}

		texture->copySubImage(target, level, xoffset, yoffset, x, y, width, height, framebuffer);
	}
}

void CullFace(GLenum mode)
{
	TRACE("(GLenum mode = 0x%X)", mode);

	switch(mode)
	{
	case GL_FRONT:
	case GL_BACK:
	case GL_FRONT_AND_BACK:
		{
			es1::Context *context = es1::getContext();

			if(context)
			{
				context->setCullMode(mode);
			}
		}
		break;
	default:
		return error(GL_INVALID_ENUM);
	}
}

void DeleteBuffers(GLsizei n, const GLuint* buffers)
{
	TRACE("(GLsizei n = %d, const GLuint* buffers = %p)", n, buffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			context->deleteBuffer(buffers[i]);
		}
	}
}

void DeleteFramebuffersOES(GLsizei n, const GLuint* framebuffers)
{
	TRACE("(GLsizei n = %d, const GLuint* framebuffers = %p)", n, framebuffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			if(framebuffers[i] != 0)
			{
				context->deleteFramebuffer(framebuffers[i]);
			}
		}
	}
}

void DeleteRenderbuffersOES(GLsizei n, const GLuint* renderbuffers)
{
	TRACE("(GLsizei n = %d, const GLuint* renderbuffers = %p)", n, renderbuffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			context->deleteRenderbuffer(renderbuffers[i]);
		}
	}
}

void DeleteTextures(GLsizei n, const GLuint* textures)
{
	TRACE("(GLsizei n = %d, const GLuint* textures = %p)", n, textures);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			if(textures[i] != 0)
			{
				context->deleteTexture(textures[i]);
			}
		}
	}
}

void DepthFunc(GLenum func)
{
	TRACE("(GLenum func = 0x%X)", func);

	switch(func)
	{
	case GL_NEVER:
	case GL_ALWAYS:
	case GL_LESS:
	case GL_LEQUAL:
	case GL_EQUAL:
	case GL_GREATER:
	case GL_GEQUAL:
	case GL_NOTEQUAL:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setDepthFunc(func);
	}
}

void DepthMask(GLboolean flag)
{
	TRACE("(GLboolean flag = %d)", flag);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setDepthMask(flag != GL_FALSE);
	}
}

void DepthRangef(GLclampf zNear, GLclampf zFar)
{
	TRACE("(GLclampf zNear = %f, GLclampf zFar = %f)", zNear, zFar);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setDepthRange(zNear, zFar);
	}
}

void DepthRangex(GLclampx zNear, GLclampx zFar)
{
	DepthRangef((float)zNear / 0x10000, (float)zFar / 0x10000);
}

void Disable(GLenum cap)
{
	TRACE("(GLenum cap = 0x%X)", cap);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(cap)
		{
		case GL_CULL_FACE:                context->setCullFaceEnabled(false);              break;
		case GL_POLYGON_OFFSET_FILL:      context->setPolygonOffsetFillEnabled(false);     break;
		case GL_SAMPLE_ALPHA_TO_COVERAGE: context->setSampleAlphaToCoverageEnabled(false); break;
		case GL_SAMPLE_COVERAGE:          context->setSampleCoverageEnabled(false);        break;
		case GL_SCISSOR_TEST:             context->setScissorTestEnabled(false);           break;
		case GL_STENCIL_TEST:             context->setStencilTestEnabled(false);           break;
		case GL_DEPTH_TEST:               context->setDepthTestEnabled(false);             break;
		case GL_BLEND:                    context->setBlendEnabled(false);                 break;
		case GL_DITHER:                   context->setDitherEnabled(false);                break;
		case GL_LIGHTING:                 context->setLightingEnabled(false);              break;
		case GL_LIGHT0:                   context->setLightEnabled(0, false);              break;
		case GL_LIGHT1:                   context->setLightEnabled(1, false);              break;
		case GL_LIGHT2:                   context->setLightEnabled(2, false);              break;
		case GL_LIGHT3:                   context->setLightEnabled(3, false);              break;
		case GL_LIGHT4:                   context->setLightEnabled(4, false);              break;
		case GL_LIGHT5:                   context->setLightEnabled(5, false);              break;
		case GL_LIGHT6:                   context->setLightEnabled(6, false);              break;
		case GL_LIGHT7:                   context->setLightEnabled(7, false);              break;
		case GL_FOG:                      context->setFogEnabled(false);                   break;
		case GL_TEXTURE_2D:               context->setTexture2Denabled(false);             break;
		case GL_TEXTURE_EXTERNAL_OES:     context->setTextureExternalEnabled(false);       break;
		case GL_ALPHA_TEST:               context->setAlphaTestEnabled(false);             break;
		case GL_COLOR_LOGIC_OP:           context->setColorLogicOpEnabled(false);          break;
		case GL_POINT_SMOOTH:             context->setPointSmoothEnabled(false);           break;
		case GL_LINE_SMOOTH:              context->setLineSmoothEnabled(false);            break;
		case GL_COLOR_MATERIAL:           context->setColorMaterialEnabled(false);         break;
		case GL_NORMALIZE:                context->setNormalizeEnabled(false);             break;
		case GL_RESCALE_NORMAL:           context->setRescaleNormalEnabled(false);         break;
		case GL_VERTEX_ARRAY:             context->setVertexArrayEnabled(false);           break;
		case GL_NORMAL_ARRAY:             context->setNormalArrayEnabled(false);           break;
		case GL_COLOR_ARRAY:              context->setColorArrayEnabled(false);            break;
		case GL_POINT_SIZE_ARRAY_OES:     context->setPointSizeArrayEnabled(false);        break;
		case GL_TEXTURE_COORD_ARRAY:      context->setTextureCoordArrayEnabled(false);     break;
		case GL_MULTISAMPLE:              context->setMultisampleEnabled(false);           break;
		case GL_SAMPLE_ALPHA_TO_ONE:      context->setSampleAlphaToOneEnabled(false);      break;
		case GL_CLIP_PLANE0:              context->setClipPlaneEnabled(0, false);          break;
		case GL_CLIP_PLANE1:              context->setClipPlaneEnabled(1, false);          break;
		case GL_CLIP_PLANE2:              context->setClipPlaneEnabled(2, false);          break;
		case GL_CLIP_PLANE3:              context->setClipPlaneEnabled(3, false);          break;
		case GL_CLIP_PLANE4:              context->setClipPlaneEnabled(4, false);          break;
		case GL_CLIP_PLANE5:              context->setClipPlaneEnabled(5, false);          break;
		case GL_POINT_SPRITE_OES:         context->setPointSpriteEnabled(false);           break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void DisableClientState(GLenum array)
{
	TRACE("(GLenum array = 0x%X)", array);

	switch(array)
	{
	case GL_VERTEX_ARRAY:
	case GL_NORMAL_ARRAY:
	case GL_COLOR_ARRAY:
	case GL_POINT_SIZE_ARRAY_OES:
	case GL_TEXTURE_COORD_ARRAY:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		GLenum texture = context->getClientActiveTexture();

		switch(array)
		{
		case GL_VERTEX_ARRAY:         context->setVertexAttribArrayEnabled(sw::Position, false);                            break;
		case GL_NORMAL_ARRAY:         context->setVertexAttribArrayEnabled(sw::Normal, false);                              break;
		case GL_COLOR_ARRAY:          context->setVertexAttribArrayEnabled(sw::Color0, false);                              break;
		case GL_POINT_SIZE_ARRAY_OES: context->setVertexAttribArrayEnabled(sw::PointSize, false);                           break;
		case GL_TEXTURE_COORD_ARRAY:  context->setVertexAttribArrayEnabled(sw::TexCoord0 + (texture - GL_TEXTURE0), false); break;
		default:                      UNREACHABLE(array);
		}
	}
}

void DrawArrays(GLenum mode, GLint first, GLsizei count)
{
	TRACE("(GLenum mode = 0x%X, GLint first = %d, GLsizei count = %d)", mode, first, count);

	if(count < 0 || first < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->drawArrays(mode, first, count);
	}
}

void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	TRACE("(GLenum mode = 0x%X, GLsizei count = %d, GLenum type = 0x%X, const GLvoid* indices = %p)",
	      mode, count, type, indices);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT:
		case GL_UNSIGNED_INT:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		context->drawElements(mode, count, type, indices);
	}
}

void Enable(GLenum cap)
{
	TRACE("(GLenum cap = 0x%X)", cap);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(cap)
		{
		case GL_CULL_FACE:                context->setCullFaceEnabled(true);              break;
		case GL_POLYGON_OFFSET_FILL:      context->setPolygonOffsetFillEnabled(true);     break;
		case GL_SAMPLE_ALPHA_TO_COVERAGE: context->setSampleAlphaToCoverageEnabled(true); break;
		case GL_SAMPLE_COVERAGE:          context->setSampleCoverageEnabled(true);        break;
		case GL_SCISSOR_TEST:             context->setScissorTestEnabled(true);           break;
		case GL_STENCIL_TEST:             context->setStencilTestEnabled(true);           break;
		case GL_DEPTH_TEST:               context->setDepthTestEnabled(true);             break;
		case GL_BLEND:                    context->setBlendEnabled(true);                 break;
		case GL_DITHER:                   context->setDitherEnabled(true);                break;
		case GL_LIGHTING:                 context->setLightingEnabled(true);              break;
		case GL_LIGHT0:                   context->setLightEnabled(0, true);              break;
		case GL_LIGHT1:                   context->setLightEnabled(1, true);              break;
		case GL_LIGHT2:                   context->setLightEnabled(2, true);              break;
		case GL_LIGHT3:                   context->setLightEnabled(3, true);              break;
		case GL_LIGHT4:                   context->setLightEnabled(4, true);              break;
		case GL_LIGHT5:                   context->setLightEnabled(5, true);              break;
		case GL_LIGHT6:                   context->setLightEnabled(6, true);              break;
		case GL_LIGHT7:                   context->setLightEnabled(7, true);              break;
		case GL_FOG:                      context->setFogEnabled(true);                   break;
		case GL_TEXTURE_2D:               context->setTexture2Denabled(true);             break;
		case GL_TEXTURE_EXTERNAL_OES:     context->setTextureExternalEnabled(true);       break;
		case GL_ALPHA_TEST:               context->setAlphaTestEnabled(true);             break;
		case GL_COLOR_LOGIC_OP:           context->setColorLogicOpEnabled(true);          break;
		case GL_POINT_SMOOTH:             context->setPointSmoothEnabled(true);           break;
		case GL_LINE_SMOOTH:              context->setLineSmoothEnabled(true);            break;
		case GL_COLOR_MATERIAL:           context->setColorMaterialEnabled(true);         break;
		case GL_NORMALIZE:                context->setNormalizeEnabled(true);             break;
		case GL_RESCALE_NORMAL:           context->setRescaleNormalEnabled(true);         break;
		case GL_VERTEX_ARRAY:             context->setVertexArrayEnabled(true);           break;
		case GL_NORMAL_ARRAY:             context->setNormalArrayEnabled(true);           break;
		case GL_COLOR_ARRAY:              context->setColorArrayEnabled(true);            break;
		case GL_POINT_SIZE_ARRAY_OES:     context->setPointSizeArrayEnabled(true);        break;
		case GL_TEXTURE_COORD_ARRAY:      context->setTextureCoordArrayEnabled(true);     break;
		case GL_MULTISAMPLE:              context->setMultisampleEnabled(true);           break;
		case GL_SAMPLE_ALPHA_TO_ONE:      context->setSampleAlphaToOneEnabled(true);      break;
		case GL_CLIP_PLANE0:              context->setClipPlaneEnabled(0, true);          break;
		case GL_CLIP_PLANE1:              context->setClipPlaneEnabled(1, true);          break;
		case GL_CLIP_PLANE2:              context->setClipPlaneEnabled(2, true);          break;
		case GL_CLIP_PLANE3:              context->setClipPlaneEnabled(3, true);          break;
		case GL_CLIP_PLANE4:              context->setClipPlaneEnabled(4, true);          break;
		case GL_CLIP_PLANE5:              context->setClipPlaneEnabled(5, true);          break;
		case GL_POINT_SPRITE_OES:         context->setPointSpriteEnabled(true);           break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void EnableClientState(GLenum array)
{
	TRACE("(GLenum array = 0x%X)", array);

	switch(array)
	{
	case GL_VERTEX_ARRAY:
	case GL_NORMAL_ARRAY:
	case GL_COLOR_ARRAY:
	case GL_POINT_SIZE_ARRAY_OES:
	case GL_TEXTURE_COORD_ARRAY:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		GLenum texture = context->getClientActiveTexture();

		switch(array)
		{
		case GL_VERTEX_ARRAY:         context->setVertexAttribArrayEnabled(sw::Position, true);                            break;
		case GL_NORMAL_ARRAY:         context->setVertexAttribArrayEnabled(sw::Normal, true);                              break;
		case GL_COLOR_ARRAY:          context->setVertexAttribArrayEnabled(sw::Color0, true);                              break;
		case GL_POINT_SIZE_ARRAY_OES: context->setVertexAttribArrayEnabled(sw::PointSize, true);                           break;
		case GL_TEXTURE_COORD_ARRAY:  context->setVertexAttribArrayEnabled(sw::TexCoord0 + (texture - GL_TEXTURE0), true); break;
		default:                      UNREACHABLE(array);
		}
	}
}

void Finish(void)
{
	TRACE("()");

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->finish();
	}
}

void Flush(void)
{
	TRACE("()");

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->flush();
	}
}

void FramebufferRenderbufferOES(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum renderbuffertarget = 0x%X, "
	      "GLuint renderbuffer = %d)", target, attachment, renderbuffertarget, renderbuffer);

	if(target != GL_FRAMEBUFFER_OES || (renderbuffertarget != GL_RENDERBUFFER_OES && renderbuffer != 0))
	{
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Framebuffer *framebuffer = context->getFramebuffer();
		GLuint framebufferName = context->getFramebufferName();

		if(!framebuffer || (framebufferName == 0 && renderbuffer != 0))
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0_OES:
			framebuffer->setColorbuffer(GL_RENDERBUFFER_OES, renderbuffer);
			break;
		case GL_DEPTH_ATTACHMENT_OES:
			framebuffer->setDepthbuffer(GL_RENDERBUFFER_OES, renderbuffer);
			break;
		case GL_STENCIL_ATTACHMENT_OES:
			framebuffer->setStencilbuffer(GL_RENDERBUFFER_OES, renderbuffer);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void FramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum textarget = 0x%X, "
	      "GLuint texture = %d, GLint level = %d)", target, attachment, textarget, texture, level);

	if(target != GL_FRAMEBUFFER_OES)
	{
		return error(GL_INVALID_ENUM);
	}

	switch(attachment)
	{
	case GL_COLOR_ATTACHMENT0_OES:
	case GL_DEPTH_ATTACHMENT_OES:
	case GL_STENCIL_ATTACHMENT_OES:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(texture == 0)
		{
			textarget = GL_NONE_OES;
		}
		else
		{
			es1::Texture *tex = context->getTexture(texture);

			if(!tex)
			{
				return error(GL_INVALID_OPERATION);
			}

			switch(textarget)
			{
			case GL_TEXTURE_2D:
				if(tex->getTarget() != GL_TEXTURE_2D)
				{
					return error(GL_INVALID_OPERATION);
				}
				break;
			default:
				return error(GL_INVALID_ENUM);
			}

			if(level != 0)
			{
				return error(GL_INVALID_VALUE);
			}

			if(tex->isCompressed(textarget, level))
			{
				return error(GL_INVALID_OPERATION);
			}
		}

		es1::Framebuffer *framebuffer = context->getFramebuffer();
		GLuint framebufferName = context->getFramebufferName();

		if(framebufferName == 0 || !framebuffer)
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0_OES:  framebuffer->setColorbuffer(textarget, texture);   break;
		case GL_DEPTH_ATTACHMENT_OES:   framebuffer->setDepthbuffer(textarget, texture);   break;
		case GL_STENCIL_ATTACHMENT_OES: framebuffer->setStencilbuffer(textarget, texture); break;
		}
	}
}

void Fogf(GLenum pname, GLfloat param)
{
	TRACE("(GLenum pname = 0x%X, GLfloat param = %f)", pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_FOG_MODE:
			switch((GLenum)param)
			{
			case GL_LINEAR:
			case GL_EXP:
			case GL_EXP2:
				context->setFogMode((GLenum)param);
				break;
			default:
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_FOG_DENSITY:
			if(param < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setFogDensity(param);
			break;
		case GL_FOG_START:
			context->setFogStart(param);
			break;
		case GL_FOG_END:
			context->setFogEnd(param);
			break;
		case GL_FOG_COLOR:
			return error(GL_INVALID_ENUM);   // Need four values, should call glFogfv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Fogfv(GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum pname = 0x%X, const GLfloat *params)", pname);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_FOG_MODE:
			switch((GLenum)params[0])
			{
			case GL_LINEAR:
			case GL_EXP:
			case GL_EXP2:
				context->setFogMode((GLenum)params[0]);
				break;
			default:
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_FOG_DENSITY:
			if(params[0] < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setFogDensity(params[0]);
			break;
		case GL_FOG_START:
			context->setFogStart(params[0]);
			break;
		case GL_FOG_END:
			context->setFogEnd(params[0]);
			break;
		case GL_FOG_COLOR:
			context->setFogColor(params[0], params[1], params[2], params[3]);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Fogx(GLenum pname, GLfixed param)
{
	TRACE("(GLenum pname = 0x%X, GLfixed param = %d)", pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_FOG_MODE:
			switch((GLenum)param)
			{
			case GL_LINEAR:
			case GL_EXP:
			case GL_EXP2:
				context->setFogMode((GLenum)param);
				break;
			default:
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_FOG_DENSITY:
			if(param < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setFogDensity((float)param / 0x10000);
			break;
		case GL_FOG_START:
			context->setFogStart((float)param / 0x10000);
			break;
		case GL_FOG_END:
			context->setFogEnd((float)param / 0x10000);
			break;
		case GL_FOG_COLOR:
			return error(GL_INVALID_ENUM);   // Need four values, should call glFogxv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Fogxv(GLenum pname, const GLfixed *params)
{
	UNIMPLEMENTED();
}

void FrontFace(GLenum mode)
{
	TRACE("(GLenum mode = 0x%X)", mode);

	switch(mode)
	{
	case GL_CW:
	case GL_CCW:
		{
			es1::Context *context = es1::getContext();

			if(context)
			{
				context->setFrontFace(mode);
			}
		}
		break;
	default:
		return error(GL_INVALID_ENUM);
	}
}

void Frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	TRACE("(GLfloat left = %f, GLfloat right = %f, GLfloat bottom = %f, GLfloat top = %f, GLfloat zNear = %f, GLfloat zFar = %f)", left, right, bottom, top, zNear, zFar);

	if(zNear <= 0.0f || zFar <= 0.0f || left == right || bottom == top || zNear == zFar)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->frustum(left, right, bottom, top, zNear, zFar);
	}
}

void Frustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
	Frustumf((float)left / 0x10000, (float)right / 0x10000, (float)bottom / 0x10000, (float)top / 0x10000, (float)zNear / 0x10000, (float)zFar / 0x10000);
}

void GenerateMipmapOES(GLenum target)
{
	TRACE("(GLenum target = 0x%X)", target);

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		if(texture->isCompressed(target, 0) || texture->isDepth(target, 0))
		{
			return error(GL_INVALID_OPERATION);
		}

		texture->generateMipmaps();
	}
}

void GenBuffers(GLsizei n, GLuint* buffers)
{
	TRACE("(GLsizei n = %d, GLuint* buffers = %p)", n, buffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			buffers[i] = context->createBuffer();
		}
	}
}

void GenFramebuffersOES(GLsizei n, GLuint* framebuffers)
{
	TRACE("(GLsizei n = %d, GLuint* framebuffers = %p)", n, framebuffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			framebuffers[i] = context->createFramebuffer();
		}
	}
}

void GenRenderbuffersOES(GLsizei n, GLuint* renderbuffers)
{
	TRACE("(GLsizei n = %d, GLuint* renderbuffers = %p)", n, renderbuffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			renderbuffers[i] = context->createRenderbuffer();
		}
	}
}

void GenTextures(GLsizei n, GLuint* textures)
{
	TRACE("(GLsizei n = %d, GLuint* textures =  %p)", n, textures);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			textures[i] = context->createTexture();
		}
	}
}

void GetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = %p)", target, pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(target != GL_RENDERBUFFER_OES)
		{
			return error(GL_INVALID_ENUM);
		}

		if(context->getRenderbufferName() == 0)
		{
			return error(GL_INVALID_OPERATION);
		}

		es1::Renderbuffer *renderbuffer = context->getRenderbuffer(context->getRenderbufferName());

		switch(pname)
		{
		case GL_RENDERBUFFER_WIDTH_OES:           *params = renderbuffer->getWidth();       break;
		case GL_RENDERBUFFER_HEIGHT_OES:          *params = renderbuffer->getHeight();      break;
		case GL_RENDERBUFFER_INTERNAL_FORMAT_OES:
			{
				GLint internalformat = renderbuffer->getFormat();
				*params = (internalformat == GL_NONE_OES) ? GL_RGBA4_OES : internalformat;
			}
			break;
		case GL_RENDERBUFFER_RED_SIZE_OES:        *params = renderbuffer->getRedSize();     break;
		case GL_RENDERBUFFER_GREEN_SIZE_OES:      *params = renderbuffer->getGreenSize();   break;
		case GL_RENDERBUFFER_BLUE_SIZE_OES:       *params = renderbuffer->getBlueSize();    break;
		case GL_RENDERBUFFER_ALPHA_SIZE_OES:      *params = renderbuffer->getAlphaSize();   break;
		case GL_RENDERBUFFER_DEPTH_SIZE_OES:      *params = renderbuffer->getDepthSize();   break;
		case GL_RENDERBUFFER_STENCIL_SIZE_OES:    *params = renderbuffer->getStencilSize(); break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void GetBooleanv(GLenum pname, GLboolean* params)
{
	TRACE("(GLenum pname = 0x%X, GLboolean* params = %p)",  pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(!(context->getBooleanv(pname, params)))
		{
			int numParams = context->getQueryParameterNum(pname);

			if(numParams < 0)
			{
				return error(GL_INVALID_ENUM);
			}

			if(numParams == 0)
			{
				return;
			}

			if(context->isQueryParameterFloat(pname))
			{
				GLfloat *floatParams = nullptr;
				floatParams = new GLfloat[numParams];

				context->getFloatv(pname, floatParams);

				for(int i = 0; i < numParams; ++i)
				{
					if(floatParams[i] == 0.0f)
						params[i] = GL_FALSE;
					else
						params[i] = GL_TRUE;
				}

				delete [] floatParams;
			}
			else if(context->isQueryParameterInt(pname))
			{
				GLint *intParams = nullptr;
				intParams = new GLint[numParams];

				context->getIntegerv(pname, intParams);

				for(int i = 0; i < numParams; ++i)
				{
					if(intParams[i] == 0)
						params[i] = GL_FALSE;
					else
						params[i] = GL_TRUE;
				}

				delete [] intParams;
			}
			else UNREACHABLE(pname);
		}
	}
}

void GetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = %p)", target, pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Buffer *buffer;

		switch(target)
		{
		case GL_ARRAY_BUFFER:
			buffer = context->getArrayBuffer();
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			buffer = context->getElementArrayBuffer();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		if(!buffer)
		{
			// A null buffer means that "0" is bound to the requested buffer target
			return error(GL_INVALID_OPERATION);
		}

		switch(pname)
		{
		case GL_BUFFER_USAGE:
			*params = buffer->usage();
			break;
		case GL_BUFFER_SIZE:
			*params = (GLint)buffer->size();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void GetClipPlanef(GLenum pname, GLfloat eqn[4])
{
	UNIMPLEMENTED();
}

void GetClipPlanex(GLenum pname, GLfixed eqn[4])
{
	UNIMPLEMENTED();
}

GLenum GetError(void)
{
	TRACE("()");

	es1::Context *context = es1::getContext();

	if(context)
	{
		return context->getError();
	}

	return GL_NO_ERROR;
}

void GetFixedv(GLenum pname, GLfixed *params)
{
	UNIMPLEMENTED();
}

void GetFloatv(GLenum pname, GLfloat* params)
{
	TRACE("(GLenum pname = 0x%X, GLfloat* params = %p)", pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(!(context->getFloatv(pname, params)))
		{
			int numParams = context->getQueryParameterNum(pname);

			if(numParams < 0)
			{
				return error(GL_INVALID_ENUM);
			}

			if(numParams == 0)
			{
				return;
			}

			if(context->isQueryParameterBool(pname))
			{
				GLboolean *boolParams = nullptr;
				boolParams = new GLboolean[numParams];

				context->getBooleanv(pname, boolParams);

				for(int i = 0; i < numParams; ++i)
				{
					if(boolParams[i] == GL_FALSE)
						params[i] = 0.0f;
					else
						params[i] = 1.0f;
				}

				delete [] boolParams;
			}
			else if(context->isQueryParameterInt(pname))
			{
				GLint *intParams = nullptr;
				intParams = new GLint[numParams];

				context->getIntegerv(pname, intParams);

				for(int i = 0; i < numParams; ++i)
				{
					params[i] = (GLfloat)intParams[i];
				}

				delete [] intParams;
			}
			else UNREACHABLE(pname);
		}
	}
}

void GetFramebufferAttachmentParameterivOES(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum pname = 0x%X, GLint* params = %p)",
	      target, attachment, pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(target != GL_FRAMEBUFFER_OES)
		{
			return error(GL_INVALID_ENUM);
		}

		if(context->getFramebufferName() == 0)
		{
			return error(GL_INVALID_OPERATION);
		}

		es1::Framebuffer *framebuffer = context->getFramebuffer();

		if(!framebuffer)
		{
			return error(GL_INVALID_OPERATION);
		}

		GLenum attachmentType;
		GLuint attachmentHandle;
		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0_OES:
			attachmentType = framebuffer->getColorbufferType();
			attachmentHandle = framebuffer->getColorbufferName();
			break;
		case GL_DEPTH_ATTACHMENT_OES:
			attachmentType = framebuffer->getDepthbufferType();
			attachmentHandle = framebuffer->getDepthbufferName();
			break;
		case GL_STENCIL_ATTACHMENT_OES:
			attachmentType = framebuffer->getStencilbufferType();
			attachmentHandle = framebuffer->getStencilbufferName();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		GLenum attachmentObjectType;   // Type category
		if(attachmentType == GL_NONE_OES || attachmentType == GL_RENDERBUFFER_OES)
		{
			attachmentObjectType = attachmentType;
		}
		else if(es1::IsTextureTarget(attachmentType))
		{
			attachmentObjectType = GL_TEXTURE;
		}
		else UNREACHABLE(attachmentType);

		switch(pname)
		{
		case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES:
			*params = attachmentObjectType;
			break;
		case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES:
			if(attachmentObjectType == GL_RENDERBUFFER_OES || attachmentObjectType == GL_TEXTURE)
			{
				*params = attachmentHandle;
			}
			else
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES:
			if(attachmentObjectType == GL_TEXTURE)
			{
				*params = 0; // FramebufferTexture2D will not allow level to be set to anything else in GL ES 2.0
			}
			else
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void GetIntegerv(GLenum pname, GLint* params)
{
	TRACE("(GLenum pname = 0x%X, GLint* params = %p)", pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(!(context->getIntegerv(pname, params)))
		{
			int numParams = context->getQueryParameterNum(pname);

			if(numParams < 0)
			{
				return error(GL_INVALID_ENUM);
			}

			if(numParams == 0)
			{
				return;
			}

			if(context->isQueryParameterBool(pname))
			{
				GLboolean *boolParams = nullptr;
				boolParams = new GLboolean[numParams];

				context->getBooleanv(pname, boolParams);

				for(int i = 0; i < numParams; ++i)
				{
					if(boolParams[i] == GL_FALSE)
						params[i] = 0;
					else
						params[i] = 1;
				}

				delete [] boolParams;
			}
			else if(context->isQueryParameterFloat(pname))
			{
				GLfloat *floatParams = nullptr;
				floatParams = new GLfloat[numParams];

				context->getFloatv(pname, floatParams);

				for(int i = 0; i < numParams; ++i)
				{
					if(pname == GL_DEPTH_RANGE || pname == GL_COLOR_CLEAR_VALUE || pname == GL_DEPTH_CLEAR_VALUE)
					{
						params[i] = (GLint)(((GLfloat)(0xFFFFFFFF) * floatParams[i] - 1.0f) / 2.0f);
					}
					else
					{
						params[i] = (GLint)(floatParams[i] > 0.0f ? floor(floatParams[i] + 0.5) : ceil(floatParams[i] - 0.5));
					}
				}

				delete [] floatParams;
			}
			else UNREACHABLE(pname);
		}
	}
}

void GetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
	UNIMPLEMENTED();
}

void GetLightxv(GLenum light, GLenum pname, GLfixed *params)
{
	UNIMPLEMENTED();
}

void GetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
	UNIMPLEMENTED();
}

void GetMaterialxv(GLenum face, GLenum pname, GLfixed *params)
{
	UNIMPLEMENTED();
}

void GetPointerv(GLenum pname, GLvoid **params)
{
	TRACE("(GLenum pname = 0x%X, GLvoid **params = %p)", pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(!(context->getPointerv(pname, const_cast<const GLvoid**>(params))))
		{
			return error(GL_INVALID_ENUM);
		}
	}
}

const GLubyte* GetString(GLenum name)
{
	TRACE("(GLenum name = 0x%X)", name);

	switch(name)
	{
	case GL_VENDOR:
		return (GLubyte*)"Google Inc.";
	case GL_RENDERER:
		return (GLubyte*)"Google SwiftShader " VERSION_STRING;
	case GL_VERSION:
		return (GLubyte*)"OpenGL ES-CM 1.1";
	case GL_EXTENSIONS:
		// Keep list sorted in following order:
		// OES extensions
		// EXT extensions
		// Vendor extensions
		return (GLubyte*)
			"GL_OES_blend_equation_separate "
			"GL_OES_blend_func_separate "
			"GL_OES_blend_subtract "
			"GL_OES_compressed_ETC1_RGB8_texture "
			"GL_OES_EGL_image "
			"GL_OES_EGL_image_external "
			"GL_OES_EGL_sync "
			"GL_OES_element_index_uint "
			"GL_OES_framebuffer_object "
			"GL_OES_packed_depth_stencil "
			"GL_OES_read_format "
			"GL_OES_rgb8_rgba8 "
			"GL_OES_stencil8 "
			"GL_OES_stencil_wrap "
			"GL_OES_surfaceless_context "
			"GL_OES_texture_mirrored_repeat "
			"GL_OES_texture_npot "
			"GL_EXT_blend_minmax "
			"GL_EXT_read_format_bgra "
			"GL_EXT_texture_compression_dxt1 "
			"GL_ANGLE_texture_compression_dxt3 "
			"GL_ANGLE_texture_compression_dxt5 "
			"GL_EXT_texture_filter_anisotropic "
			"GL_EXT_texture_format_BGRA8888";
	default:
		return error(GL_INVALID_ENUM, (GLubyte*)nullptr);
	}
}

void GetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat* params = %p)", target, pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D();
			break;
		case GL_TEXTURE_EXTERNAL_OES:
			texture = context->getTextureExternal();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		switch(pname)
		{
		case GL_TEXTURE_MAG_FILTER:
			*params = (GLfloat)texture->getMagFilter();
			break;
		case GL_TEXTURE_MIN_FILTER:
			*params = (GLfloat)texture->getMinFilter();
			break;
		case GL_TEXTURE_WRAP_S:
			*params = (GLfloat)texture->getWrapS();
			break;
		case GL_TEXTURE_WRAP_T:
			*params = (GLfloat)texture->getWrapT();
			break;
		case GL_TEXTURE_MAX_ANISOTROPY_EXT:
			*params = texture->getMaxAnisotropy();
			break;
		case GL_GENERATE_MIPMAP:
			*params = (GLfloat)texture->getGenerateMipmap();
			break;
		case GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES:
			*params = (GLfloat)1;
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void GetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = %p)", target, pname, params);

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D();
			break;
		case GL_TEXTURE_EXTERNAL_OES:
			texture = context->getTextureExternal();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		switch(pname)
		{
		case GL_TEXTURE_MAG_FILTER:
			*params = texture->getMagFilter();
			break;
		case GL_TEXTURE_MIN_FILTER:
			*params = texture->getMinFilter();
			break;
		case GL_TEXTURE_WRAP_S:
			*params = texture->getWrapS();
			break;
		case GL_TEXTURE_WRAP_T:
			*params = texture->getWrapT();
			break;
		case GL_TEXTURE_MAX_ANISOTROPY_EXT:
			*params = (GLint)texture->getMaxAnisotropy();
			break;
		case GL_GENERATE_MIPMAP:
			*params = (GLint)texture->getGenerateMipmap();
			break;
		case GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES:
			*params = 1;
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void GetTexEnvfv(GLenum env, GLenum pname, GLfloat *params)
{
	UNIMPLEMENTED();
}

void GetTexEnviv(GLenum env, GLenum pname, GLint *params)
{
	UNIMPLEMENTED();
}

void GetTexEnvxv(GLenum env, GLenum pname, GLfixed *params)
{
	UNIMPLEMENTED();
}

void GetTexParameterxv(GLenum target, GLenum pname, GLfixed *params)
{
	UNIMPLEMENTED();
}

void Hint(GLenum target, GLenum mode)
{
	TRACE("(GLenum target = 0x%X, GLenum mode = 0x%X)", target, mode);

	switch(mode)
	{
	case GL_FASTEST:
	case GL_NICEST:
	case GL_DONT_CARE:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_GENERATE_MIPMAP_HINT:
			context->setGenerateMipmapHint(mode);
			break;
		case GL_PERSPECTIVE_CORRECTION_HINT:
			context->setPerspectiveCorrectionHint(mode);
			break;
		case GL_FOG_HINT:
			context->setFogHint(mode);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GLboolean IsBuffer(GLuint buffer)
{
	TRACE("(GLuint buffer = %d)", buffer);

	es1::Context *context = es1::getContext();

	if(context && buffer)
	{
		es1::Buffer *bufferObject = context->getBuffer(buffer);

		if(bufferObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean IsEnabled(GLenum cap)
{
	TRACE("(GLenum cap = 0x%X)", cap);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(cap)
		{
		case GL_CULL_FACE:                return context->isCullFaceEnabled();              break;
		case GL_POLYGON_OFFSET_FILL:      return context->isPolygonOffsetFillEnabled();     break;
		case GL_SAMPLE_ALPHA_TO_COVERAGE: return context->isSampleAlphaToCoverageEnabled(); break;
		case GL_SAMPLE_COVERAGE:          return context->isSampleCoverageEnabled();        break;
		case GL_SCISSOR_TEST:             return context->isScissorTestEnabled();           break;
		case GL_STENCIL_TEST:             return context->isStencilTestEnabled();           break;
		case GL_DEPTH_TEST:               return context->isDepthTestEnabled();             break;
		case GL_BLEND:                    return context->isBlendEnabled();                 break;
		case GL_DITHER:                   return context->isDitherEnabled();                break;
		case GL_LIGHTING:                 return context->isLightingEnabled();              break;
		case GL_LIGHT0:                   return context->isLightEnabled(0);                break;
		case GL_LIGHT1:                   return context->isLightEnabled(1);                break;
		case GL_LIGHT2:                   return context->isLightEnabled(2);                break;
		case GL_LIGHT3:                   return context->isLightEnabled(3);                break;
		case GL_LIGHT4:                   return context->isLightEnabled(4);                break;
		case GL_LIGHT5:                   return context->isLightEnabled(5);                break;
		case GL_LIGHT6:                   return context->isLightEnabled(6);                break;
		case GL_LIGHT7:                   return context->isLightEnabled(7);                break;
		case GL_FOG:                      return context->isFogEnabled();                   break;
		case GL_TEXTURE_2D:               return context->isTexture2Denabled();             break;
		case GL_TEXTURE_EXTERNAL_OES:     return context->isTextureExternalEnabled();       break;
		case GL_ALPHA_TEST:               return context->isAlphaTestEnabled();             break;
		case GL_COLOR_LOGIC_OP:           return context->isColorLogicOpEnabled();          break;
		case GL_POINT_SMOOTH:             return context->isPointSmoothEnabled();           break;
		case GL_LINE_SMOOTH:              return context->isLineSmoothEnabled();            break;
		case GL_COLOR_MATERIAL:           return context->isColorMaterialEnabled();         break;
		case GL_NORMALIZE:                return context->isNormalizeEnabled();             break;
		case GL_RESCALE_NORMAL:           return context->isRescaleNormalEnabled();         break;
		case GL_VERTEX_ARRAY:             return context->isVertexArrayEnabled();           break;
		case GL_NORMAL_ARRAY:             return context->isNormalArrayEnabled();           break;
		case GL_COLOR_ARRAY:              return context->isColorArrayEnabled();            break;
		case GL_POINT_SIZE_ARRAY_OES:     return context->isPointSizeArrayEnabled();        break;
		case GL_TEXTURE_COORD_ARRAY:      return context->isTextureCoordArrayEnabled();     break;
		case GL_MULTISAMPLE:              return context->isMultisampleEnabled();           break;
		case GL_SAMPLE_ALPHA_TO_ONE:      return context->isSampleAlphaToOneEnabled();      break;
		case GL_CLIP_PLANE0:              return context->isClipPlaneEnabled(0);            break;
		case GL_CLIP_PLANE1:              return context->isClipPlaneEnabled(1);            break;
		case GL_CLIP_PLANE2:              return context->isClipPlaneEnabled(2);            break;
		case GL_CLIP_PLANE3:              return context->isClipPlaneEnabled(3);            break;
		case GL_CLIP_PLANE4:              return context->isClipPlaneEnabled(4);            break;
		case GL_CLIP_PLANE5:              return context->isClipPlaneEnabled(5);            break;
		case GL_POINT_SPRITE_OES:         return context->isPointSpriteEnabled();           break;
		default:
			return error(GL_INVALID_ENUM, GL_FALSE);
		}
	}

	return GL_FALSE;
}

GLboolean IsFramebufferOES(GLuint framebuffer)
{
	TRACE("(GLuint framebuffer = %d)", framebuffer);

	es1::Context *context = es1::getContext();

	if(context && framebuffer)
	{
		es1::Framebuffer *framebufferObject = context->getFramebuffer(framebuffer);

		if(framebufferObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean IsTexture(GLuint texture)
{
	TRACE("(GLuint texture = %d)", texture);

	es1::Context *context = es1::getContext();

	if(context && texture)
	{
		es1::Texture *textureObject = context->getTexture(texture);

		if(textureObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean IsRenderbufferOES(GLuint renderbuffer)
{
	TRACE("(GLuint renderbuffer = %d)", renderbuffer);

	es1::Context *context = es1::getContext();

	if(context && renderbuffer)
	{
		es1::Renderbuffer *renderbufferObject = context->getRenderbuffer(renderbuffer);

		if(renderbufferObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

void LightModelf(GLenum pname, GLfloat param)
{
	TRACE("(GLenum pname = 0x%X, GLfloat param = %f)", pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_LIGHT_MODEL_TWO_SIDE:
			context->setLightModelTwoSide(param != 0.0f);
			break;
		case GL_LIGHT_MODEL_AMBIENT:
			return error(GL_INVALID_ENUM);   // Need four values, should call glLightModelfv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void LightModelfv(GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum pname = 0x%X, const GLfloat *params)", pname);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_LIGHT_MODEL_AMBIENT:
			context->setGlobalAmbient(params[0], params[1], params[2], params[3]);
			break;
		case GL_LIGHT_MODEL_TWO_SIDE:
			context->setLightModelTwoSide(params[0] != 0.0f);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void LightModelx(GLenum pname, GLfixed param)
{
	TRACE("(GLenum pname = 0x%X, GLfixed param = %d)", pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_LIGHT_MODEL_TWO_SIDE:
			context->setLightModelTwoSide(param != 0);
			break;
		case GL_LIGHT_MODEL_AMBIENT:
			return error(GL_INVALID_ENUM);   // Need four values, should call glLightModelxv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void LightModelxv(GLenum pname, const GLfixed *params)
{
	TRACE("(GLenum pname = 0x%X, const GLfixed *params)", pname);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_LIGHT_MODEL_AMBIENT:
			context->setGlobalAmbient((float)params[0] / 0x10000, (float)params[1] / 0x10000, (float)params[2] / 0x10000, (float)params[3] / 0x10000);
			break;
		case GL_LIGHT_MODEL_TWO_SIDE:
			context->setLightModelTwoSide(params[0] != 0);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Lightf(GLenum light, GLenum pname, GLfloat param)
{
	TRACE("(GLenum light = 0x%X, GLenum pname = 0x%X, GLfloat param = %f)", light, pname, param);

	int index = light - GL_LIGHT0;

	if(index < 0 || index >= es1::MAX_LIGHTS)
	{
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_SPOT_EXPONENT:
			if(param < 0.0f || param > 128.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setSpotLightExponent(index, param);
			break;
		case GL_SPOT_CUTOFF:
			if((param < 0.0f || param > 90.0f) && param != 180.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setSpotLightCutoff(index, param);
			break;
		case GL_CONSTANT_ATTENUATION:
			if(param < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setLightAttenuationConstant(index, param);
			break;
		case GL_LINEAR_ATTENUATION:
			if(param < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setLightAttenuationLinear(index, param);
			break;
		case GL_QUADRATIC_ATTENUATION:
			if(param < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setLightAttenuationQuadratic(index, param);
			break;
		case GL_AMBIENT:
		case GL_DIFFUSE:
		case GL_SPECULAR:
		case GL_POSITION:
		case GL_SPOT_DIRECTION:
			return error(GL_INVALID_ENUM);   // Need four values, should call glLightfv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Lightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum light = 0x%X, GLenum pname = 0x%X, const GLint *params)", light, pname);

	es1::Context *context = es1::getContext();

	if(context)
	{
		int index = light - GL_LIGHT0;

		if(index < 0 || index > es1::MAX_LIGHTS)
		{
			return error(GL_INVALID_ENUM);
		}

		switch(pname)
		{
		case GL_AMBIENT:               context->setLightAmbient(index, params[0], params[1], params[2], params[3]);  break;
		case GL_DIFFUSE:               context->setLightDiffuse(index, params[0], params[1], params[2], params[3]);  break;
		case GL_SPECULAR:              context->setLightSpecular(index, params[0], params[1], params[2], params[3]); break;
		case GL_POSITION:              context->setLightPosition(index, params[0], params[1], params[2], params[3]); break;
		case GL_SPOT_DIRECTION:        context->setLightDirection(index, params[0], params[1], params[2]);           break;
		case GL_SPOT_EXPONENT:
			if(params[0] < 0.0f || params[0] > 128.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setSpotLightExponent(index, params[0]);
			break;
		case GL_SPOT_CUTOFF:
			if((params[0] < 0.0f || params[0] > 90.0f) && params[0] != 180.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setSpotLightCutoff(index, params[0]);
			break;
		case GL_CONSTANT_ATTENUATION:
			if(params[0] < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setLightAttenuationConstant(index, params[0]);
			break;
		case GL_LINEAR_ATTENUATION:
			if(params[0] < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setLightAttenuationLinear(index, params[0]);
			break;
		case GL_QUADRATIC_ATTENUATION:
			if(params[0] < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setLightAttenuationQuadratic(index, params[0]);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Lightx(GLenum light, GLenum pname, GLfixed param)
{
	UNIMPLEMENTED();
}

void Lightxv(GLenum light, GLenum pname, const GLfixed *params)
{
	UNIMPLEMENTED();
}

void LineWidth(GLfloat width)
{
	TRACE("(GLfloat width = %f)", width);

	if(width <= 0.0f)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setLineWidth(width);
	}
}

void LineWidthx(GLfixed width)
{
	LineWidth((float)width / 0x10000);
}

void LoadIdentity(void)
{
	TRACE("()");

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->loadIdentity();
	}
}

void LoadMatrixf(const GLfloat *m)
{
	TRACE("(const GLfloat *m)");

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->load(m);
	}
}

void LoadMatrixx(const GLfixed *m)
{
	GLfloat matrix[16] =
	{
		(float)m[0] / 0x10000,  (float)m[1] / 0x10000,  (float)m[2] / 0x10000,  (float)m[3] / 0x10000,
		(float)m[4] / 0x10000,  (float)m[5] / 0x10000,  (float)m[6] / 0x10000,  (float)m[7] / 0x10000,
		(float)m[8] / 0x10000,  (float)m[9] / 0x10000,  (float)m[10] / 0x10000, (float)m[11] / 0x10000,
		(float)m[12] / 0x10000, (float)m[13] / 0x10000, (float)m[14] / 0x10000, (float)m[15] / 0x10000
	};

	LoadMatrixf(matrix);
}

void LogicOp(GLenum opcode)
{
	TRACE("(GLenum opcode = 0x%X)", opcode);

	switch(opcode)
	{
	case GL_CLEAR:
	case GL_SET:
	case GL_COPY:
	case GL_COPY_INVERTED:
	case GL_NOOP:
	case GL_INVERT:
	case GL_AND:
	case GL_NAND:
	case GL_OR:
	case GL_NOR:
	case GL_XOR:
	case GL_EQUIV:
	case GL_AND_REVERSE:
	case GL_AND_INVERTED:
	case GL_OR_REVERSE:
	case GL_OR_INVERTED:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setLogicalOperation(opcode);
	}
}

void Materialf(GLenum face, GLenum pname, GLfloat param)
{
	TRACE("(GLenum face = 0x%X, GLenum pname = 0x%X, GLfloat param = %f)", face, pname, param);

	if(face != GL_FRONT_AND_BACK)
	{
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_SHININESS:
			if(param < 0.0f || param > 128.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setMaterialShininess(param);
			break;
		case GL_AMBIENT:
		case GL_DIFFUSE:
		case GL_AMBIENT_AND_DIFFUSE:
		case GL_SPECULAR:
		case GL_EMISSION:
			return error(GL_INVALID_ENUM);   // Need four values, should call glMaterialfv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum face = 0x%X, GLenum pname = 0x%X, GLfloat params)", face, pname);

	if(face != GL_FRONT_AND_BACK)
	{
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_AMBIENT:
			context->setMaterialAmbient(params[0], params[1], params[2], params[3]);
			break;
		case GL_DIFFUSE:
			context->setMaterialDiffuse(params[0], params[1], params[2], params[3]);
			break;
		case GL_AMBIENT_AND_DIFFUSE:
			context->setMaterialAmbient(params[0], params[1], params[2], params[3]);
			context->setMaterialDiffuse(params[0], params[1], params[2], params[3]);
			break;
		case GL_SPECULAR:
			context->setMaterialSpecular(params[0], params[1], params[2], params[3]);
			break;
		case GL_EMISSION:
			context->setMaterialEmission(params[0], params[1], params[2], params[3]);
			break;
		case GL_SHININESS:
			context->setMaterialShininess(params[0]);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Materialx(GLenum face, GLenum pname, GLfixed param)
{
	UNIMPLEMENTED();
}

void Materialxv(GLenum face, GLenum pname, const GLfixed *params)
{
	UNIMPLEMENTED();
}

void MatrixMode(GLenum mode)
{
	TRACE("(GLenum mode = 0x%X)", mode);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setMatrixMode(mode);
	}
}

void MultMatrixf(const GLfloat *m)
{
	TRACE("(const GLfloat *m)");

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->multiply(m);
	}
}

void MultMatrixx(const GLfixed *m)
{
	GLfloat matrix[16] =
	{
		(float)m[0] / 0x10000,  (float)m[1] / 0x10000,  (float)m[2] / 0x10000,  (float)m[3] / 0x10000,
		(float)m[4] / 0x10000,  (float)m[5] / 0x10000,  (float)m[6] / 0x10000,  (float)m[7] / 0x10000,
		(float)m[8] / 0x10000,  (float)m[9] / 0x10000,  (float)m[10] / 0x10000, (float)m[11] / 0x10000,
		(float)m[12] / 0x10000, (float)m[13] / 0x10000, (float)m[14] / 0x10000, (float)m[15] / 0x10000
	};

	MultMatrixf(matrix);
}

void MultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	TRACE("(GLenum target = 0x%X, GLfloat s = %f, GLfloat t = %f, GLfloat r = %f, GLfloat q = %f)", target, s, t, r, q);

	switch(target)
	{
	case GL_TEXTURE0:
	case GL_TEXTURE1:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setVertexAttrib(sw::TexCoord0 + (target - GL_TEXTURE0), s, t, r, q);
	}
}

void MultiTexCoord4x(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q)
{
	UNIMPLEMENTED();
}

void Normal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	TRACE("(GLfloat nx, GLfloat ny, GLfloat nz)", nx, ny, nz);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setVertexAttrib(sw::Normal, nx, ny, nz, 0);
	}
}

void Normal3x(GLfixed nx, GLfixed ny, GLfixed nz)
{
	UNIMPLEMENTED();
}

void NormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(GLenum type = 0x%X, GLsizei stride = %d, const GLvoid *pointer = %p)", type, stride, pointer);

	VertexAttribPointer(sw::Normal, 3, type, true, stride, pointer);
}

void Orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	TRACE("(GLfloat left = %f, GLfloat right = %f, GLfloat bottom = %f, GLfloat top = %f, GLfloat zNear = %f, GLfloat zFar = %f)", left, right, bottom, top, zNear, zFar);

	if(left == right || bottom == top || zNear == zFar)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->ortho(left, right, bottom, top, zNear, zFar);
	}
}

void Orthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
	Orthof((float)left / 0x10000, (float)right / 0x10000, (float)bottom / 0x10000, (float)top / 0x10000, (float)zNear / 0x10000, (float)zFar / 0x10000);
}

void PixelStorei(GLenum pname, GLint param)
{
	TRACE("(GLenum pname = 0x%X, GLint param = %d)", pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_UNPACK_ALIGNMENT:
			if(param != 1 && param != 2 && param != 4 && param != 8)
			{
				return error(GL_INVALID_VALUE);
			}

			context->setUnpackAlignment(param);
			break;
		case GL_PACK_ALIGNMENT:
			if(param != 1 && param != 2 && param != 4 && param != 8)
			{
				return error(GL_INVALID_VALUE);
			}

			context->setPackAlignment(param);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void PointParameterf(GLenum pname, GLfloat param)
{
	TRACE("(GLenum pname = 0x%X, GLfloat param = %f)", pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_POINT_SIZE_MIN:
			if(param < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointSizeMin(param);
			break;
		case GL_POINT_SIZE_MAX:
			if(param < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointSizeMax(param);
			break;
		case GL_POINT_FADE_THRESHOLD_SIZE:
			if(param < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointFadeThresholdSize(param);
			break;
		case GL_POINT_DISTANCE_ATTENUATION:
			return error(GL_INVALID_ENUM);   // Needs three values, should call glPointParameterfv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void PointParameterfv(GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum pname = 0x%X, const GLfloat *params)", pname);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_POINT_SIZE_MIN:
			if(params[0] < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointSizeMin(params[0]);
			break;
		case GL_POINT_SIZE_MAX:
			if(params[0] < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointSizeMax(params[0]);
			break;
		case GL_POINT_DISTANCE_ATTENUATION:
			context->setPointDistanceAttenuation(params[0], params[1], params[2]);
			break;
		case GL_POINT_FADE_THRESHOLD_SIZE:
			if(params[0] < 0.0f)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointFadeThresholdSize(params[0]);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void PointParameterx(GLenum pname, GLfixed param)
{
	TRACE("(GLenum pname = 0x%X, GLfixed param = %d)", pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_POINT_SIZE_MIN:
			if(param < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointSizeMin((float)param / 0x10000);
			break;
		case GL_POINT_SIZE_MAX:
			if(param < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointSizeMax((float)param / 0x10000);
			break;
		case GL_POINT_FADE_THRESHOLD_SIZE:
			if(param < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointFadeThresholdSize((float)param / 0x10000);
			break;
		case GL_POINT_DISTANCE_ATTENUATION:
			return error(GL_INVALID_ENUM);   // Needs three parameters, should call glPointParameterxv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void PointParameterxv(GLenum pname, const GLfixed *params)
{
	TRACE("(GLenum pname = 0x%X, const GLfixed *params)", pname);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(pname)
		{
		case GL_POINT_SIZE_MIN:
			if(params[0] < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointSizeMin((float)params[0] / 0x10000);
			break;
		case GL_POINT_SIZE_MAX:
			if(params[0] < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointSizeMax((float)params[0] / 0x10000);
			break;
		case GL_POINT_DISTANCE_ATTENUATION:
			context->setPointDistanceAttenuation((float)params[0] / 0x10000, (float)params[1] / 0x10000, (float)params[2] / 0x10000);
			break;
		case GL_POINT_FADE_THRESHOLD_SIZE:
			if(params[0] < 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->setPointFadeThresholdSize((float)params[0] / 0x10000);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void PointSize(GLfloat size)
{
	TRACE("(GLfloat size = %f)", size);

	if(size <= 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setVertexAttrib(sw::PointSize, size, size, size, size);
	}
}

void PointSizePointerOES(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(GLenum type = 0x%X, GLsizei stride = %d, const GLvoid *pointer = %p)", type, stride, pointer);

	switch(type)
	{
	case GL_FIXED:
	case GL_FLOAT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	VertexAttribPointer(sw::PointSize, 1, type, true, stride, pointer);
}

void PointSizex(GLfixed size)
{
	PointSize((float)size / 0x10000);
}

void PolygonOffset(GLfloat factor, GLfloat units)
{
	TRACE("(GLfloat factor = %f, GLfloat units = %f)", factor, units);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setPolygonOffsetParams(factor, units);
	}
}

void PolygonOffsetx(GLfixed factor, GLfixed units)
{
	PolygonOffset((float)factor / 0x10000, (float)units / 0x10000);
}

void PopMatrix(void)
{
	TRACE("()");

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->popMatrix();
	}
}

void PushMatrix(void)
{
	TRACE("()");

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->pushMatrix();
	}
}

void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
	TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, "
	      "GLenum format = 0x%X, GLenum type = 0x%X, GLvoid* pixels = %p)",
	      x, y, width, height, format, type,  pixels);

	if(width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->readPixels(x, y, width, height, format, type, nullptr, pixels);
	}
}

void RenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	TRACE("(GLenum target = 0x%X, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d)",
	      target, internalformat, width, height);

	switch(target)
	{
	case GL_RENDERBUFFER_OES:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(!es1::IsColorRenderable(internalformat) && !es1::IsDepthRenderable(internalformat) && !es1::IsStencilRenderable(internalformat))
	{
		return error(GL_INVALID_ENUM);
	}

	if(width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(width > es1::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE ||
		   height > es1::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE)
		{
			return error(GL_INVALID_VALUE);
		}

		GLuint handle = context->getRenderbufferName();
		if(handle == 0)
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(internalformat)
		{
		case GL_RGBA4_OES:
		case GL_RGB5_A1_OES:
		case GL_RGB565_OES:
		case GL_RGB8_OES:
		case GL_RGBA8_OES:
			context->setRenderbufferStorage(new es1::Colorbuffer(width, height, internalformat, 0));
			break;
		case GL_DEPTH_COMPONENT16_OES:
			context->setRenderbufferStorage(new es1::Depthbuffer(width, height, internalformat,  0));
			break;
		case GL_STENCIL_INDEX8_OES:
			context->setRenderbufferStorage(new es1::Stencilbuffer(width, height, 0));
			break;
		case GL_DEPTH24_STENCIL8_OES:
			context->setRenderbufferStorage(new es1::DepthStencilbuffer(width, height, internalformat, 0));
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	TRACE("(GLfloat angle = %f, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", angle, x, y, z);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->rotate(angle, x, y, z);
	}
}

void Rotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
	Rotatef((float)angle / 0x10000, (float)x / 0x10000, (float)y / 0x10000, (float)z / 0x10000);
}

void SampleCoverage(GLclampf value, GLboolean invert)
{
	TRACE("(GLclampf value = %f, GLboolean invert = %d)", value, invert);

	es1::Context* context = es1::getContext();

	if(context)
	{
		context->setSampleCoverageParams(es1::clamp01(value), invert == GL_TRUE);
	}
}

void SampleCoveragex(GLclampx value, GLboolean invert)
{
	SampleCoverage((float)value / 0x10000, invert);
}

void Scalef(GLfloat x, GLfloat y, GLfloat z)
{
	TRACE("(GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", x, y, z);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->scale(x, y, z);
	}
}

void Scalex(GLfixed x, GLfixed y, GLfixed z)
{
	Scalef((float)x / 0x10000, (float)y / 0x10000, (float)z / 0x10000);
}

void Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)", x, y, width, height);

	if(width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context* context = es1::getContext();

	if(context)
	{
		context->setScissorParams(x, y, width, height);
	}
}

void ShadeModel(GLenum mode)
{
	switch(mode)
	{
	case GL_FLAT:
	case GL_SMOOTH:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setShadeModel(mode);
	}
}

void StencilFunc(GLenum func, GLint ref, GLuint mask)
{
	TRACE("(GLenum func = 0x%X, GLint ref = %d, GLuint mask = %d)",  func, ref, mask);

	switch(func)
	{
	case GL_NEVER:
	case GL_ALWAYS:
	case GL_LESS:
	case GL_LEQUAL:
	case GL_EQUAL:
	case GL_GEQUAL:
	case GL_GREATER:
	case GL_NOTEQUAL:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setStencilParams(func, ref, mask);
	}
}

void StencilMask(GLuint mask)
{
	TRACE("(GLuint mask = %d)", mask);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setStencilWritemask(mask);
	}
}

void StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	TRACE("(GLenum fail = 0x%X, GLenum zfail = 0x%X, GLenum zpas = 0x%Xs)", fail, zfail, zpass);

	switch(fail)
	{
	case GL_ZERO:
	case GL_KEEP:
	case GL_REPLACE:
	case GL_INCR:
	case GL_DECR:
	case GL_INVERT:
	case GL_INCR_WRAP_OES:
	case GL_DECR_WRAP_OES:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(zfail)
	{
	case GL_ZERO:
	case GL_KEEP:
	case GL_REPLACE:
	case GL_INCR:
	case GL_DECR:
	case GL_INVERT:
	case GL_INCR_WRAP_OES:
	case GL_DECR_WRAP_OES:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(zpass)
	{
	case GL_ZERO:
	case GL_KEEP:
	case GL_REPLACE:
	case GL_INCR:
	case GL_DECR:
	case GL_INVERT:
	case GL_INCR_WRAP_OES:
	case GL_DECR_WRAP_OES:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setStencilOperations(fail, zfail, zpass);
	}
}

void TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(GLint size = %d, GLenum type = 0x%X, GLsizei stride = %d, const GLvoid *pointer = %p)", size, type, stride, pointer);

	if(size < 2 || size > 4)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		GLenum texture = context->getClientActiveTexture();
		VertexAttribPointer(sw::TexCoord0 + (texture - GL_TEXTURE0), size, type, false, stride, pointer);
	}
}

void TexEnvi(GLenum target, GLenum pname, GLint param);

void TexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	TexEnvi(target, pname, (GLint)param);
}

void TexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, const GLfloat *params)", target, pname);

	es1::Context *context = es1::getContext();

	if(context)
	{
		GLint iParam = (GLint)roundf(params[0]);

		switch(target)
		{
		case GL_POINT_SPRITE_OES:
			UNIMPLEMENTED();
			break;
		case GL_TEXTURE_ENV:
			switch(pname)
			{
			case GL_TEXTURE_ENV_MODE:
				switch(iParam)
				{
				case GL_REPLACE:
				case GL_MODULATE:
				case GL_DECAL:
				case GL_BLEND:
				case GL_ADD:
				case GL_COMBINE:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setTextureEnvMode(iParam);
				break;
			case GL_TEXTURE_ENV_COLOR:
				context->setTextureEnvColor(clamp01(params[0]), clamp01(params[1]), clamp01(params[2]), clamp01(params[3]));
				break;
			case GL_COMBINE_RGB:
				switch(iParam)
				{
				case GL_REPLACE:
				case GL_MODULATE:
				case GL_ADD:
				case GL_ADD_SIGNED:
				case GL_INTERPOLATE:
				case GL_SUBTRACT:
				case GL_DOT3_RGB:
				case GL_DOT3_RGBA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setCombineRGB(iParam);
				break;
			case GL_COMBINE_ALPHA:
				switch(iParam)
				{
				case GL_REPLACE:
				case GL_MODULATE:
				case GL_ADD:
				case GL_ADD_SIGNED:
				case GL_INTERPOLATE:
				case GL_SUBTRACT:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setCombineAlpha(iParam);
				break;
			case GL_RGB_SCALE:
				if(iParam != 1 && iParam != 2 && iParam != 4)
				{
					return error(GL_INVALID_VALUE);
				}
				if(iParam != 1) UNIMPLEMENTED();
				break;
			case GL_ALPHA_SCALE:
				if(iParam != 1 && iParam != 2 && iParam != 4)
				{
					return error(GL_INVALID_VALUE);
				}
				if(iParam != 1) UNIMPLEMENTED();
				break;
			case GL_OPERAND0_RGB:
				switch(iParam)
				{
				case GL_SRC_COLOR:
				case GL_ONE_MINUS_SRC_COLOR:
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand0RGB(iParam);
				break;
			case GL_OPERAND1_RGB:
				switch(iParam)
				{
				case GL_SRC_COLOR:
				case GL_ONE_MINUS_SRC_COLOR:
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand1RGB(iParam);
				break;
			case GL_OPERAND2_RGB:
				switch(iParam)
				{
				case GL_SRC_COLOR:
				case GL_ONE_MINUS_SRC_COLOR:
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand2RGB(iParam);
				break;
			case GL_OPERAND0_ALPHA:
				switch(iParam)
				{
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand0Alpha(iParam);
				break;
			case GL_OPERAND1_ALPHA:
				switch(iParam)
				{
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand1Alpha(iParam);
				break;
			case GL_OPERAND2_ALPHA:
				switch(iParam)
				{
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand2Alpha(iParam);
				break;
			case GL_SRC0_RGB:
				switch(iParam)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc0RGB(iParam);
				break;
			case GL_SRC1_RGB:
				switch(iParam)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc1RGB(iParam);
				break;
			case GL_SRC2_RGB:
				switch(iParam)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc2RGB(iParam);
				break;
			case GL_SRC0_ALPHA:
				switch(iParam)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc0Alpha(iParam);
				break;
			case GL_SRC1_ALPHA:
				switch(iParam)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc1Alpha(iParam);
				break;
			case GL_SRC2_ALPHA:
				switch(iParam)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc2Alpha(iParam);
				break;
			default:
				return error(GL_INVALID_ENUM);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void TexEnvi(GLenum target, GLenum pname, GLint param)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint param = %d)", target, pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_POINT_SPRITE_OES:
			UNIMPLEMENTED();
			break;
		case GL_TEXTURE_ENV:
			switch(pname)
			{
			case GL_TEXTURE_ENV_MODE:
				switch((GLenum)param)
				{
				case GL_REPLACE:
				case GL_MODULATE:
				case GL_DECAL:
				case GL_BLEND:
				case GL_ADD:
				case GL_COMBINE:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setTextureEnvMode((GLenum)param);
				break;
			case GL_TEXTURE_ENV_COLOR:
				return error(GL_INVALID_ENUM);   // Needs four values, should call glTexEnviv() instead
				break;
			case GL_COMBINE_RGB:
				switch((GLenum)param)
				{
				case GL_REPLACE:
				case GL_MODULATE:
				case GL_ADD:
				case GL_ADD_SIGNED:
				case GL_INTERPOLATE:
				case GL_SUBTRACT:
				case GL_DOT3_RGB:
				case GL_DOT3_RGBA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setCombineRGB((GLenum)param);
				break;
			case GL_COMBINE_ALPHA:
				switch((GLenum)param)
				{
				case GL_REPLACE:
				case GL_MODULATE:
				case GL_ADD:
				case GL_ADD_SIGNED:
				case GL_INTERPOLATE:
				case GL_SUBTRACT:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setCombineAlpha((GLenum)param);
				break;
			case GL_RGB_SCALE:
				if(param != 1 && param != 2 && param != 4)
				{
					return error(GL_INVALID_VALUE);
				}
				if(param != 1) UNIMPLEMENTED();
				break;
			case GL_ALPHA_SCALE:
				if(param != 1 && param != 2 && param != 4)
				{
					return error(GL_INVALID_VALUE);
				}
				if(param != 1) UNIMPLEMENTED();
				break;
			case GL_OPERAND0_RGB:
				switch((GLenum)param)
				{
				case GL_SRC_COLOR:
				case GL_ONE_MINUS_SRC_COLOR:
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand0RGB((GLenum)param);
				break;
			case GL_OPERAND1_RGB:
				switch((GLenum)param)
				{
				case GL_SRC_COLOR:
				case GL_ONE_MINUS_SRC_COLOR:
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand1RGB((GLenum)param);
				break;
			case GL_OPERAND2_RGB:
				switch((GLenum)param)
				{
				case GL_SRC_COLOR:
				case GL_ONE_MINUS_SRC_COLOR:
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand2RGB((GLenum)param);
				break;
			case GL_OPERAND0_ALPHA:
				switch((GLenum)param)
				{
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand0Alpha((GLenum)param);
				break;
			case GL_OPERAND1_ALPHA:
				switch((GLenum)param)
				{
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand1Alpha((GLenum)param);
				break;
			case GL_OPERAND2_ALPHA:
				switch((GLenum)param)
				{
				case GL_SRC_ALPHA:
				case GL_ONE_MINUS_SRC_ALPHA:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setOperand2Alpha((GLenum)param);
				break;
			case GL_SRC0_RGB:
				switch((GLenum)param)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc0RGB((GLenum)param);
				break;
			case GL_SRC1_RGB:
				switch((GLenum)param)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc1RGB((GLenum)param);
				break;
			case GL_SRC2_RGB:
				switch((GLenum)param)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc2RGB((GLenum)param);
				break;
			case GL_SRC0_ALPHA:
				switch((GLenum)param)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc0Alpha((GLenum)param);
				break;
			case GL_SRC1_ALPHA:
				switch((GLenum)param)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc1Alpha((GLenum)param);
				break;
			case GL_SRC2_ALPHA:
				switch((GLenum)param)
				{
				case GL_TEXTURE:
				case GL_CONSTANT:
				case GL_PRIMARY_COLOR:
				case GL_PREVIOUS:
					break;
				default:
					error(GL_INVALID_ENUM);
				}

				context->setSrc2Alpha((GLenum)param);
				break;
			default:
				return error(GL_INVALID_ENUM);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void TexEnvx(GLenum target, GLenum pname, GLfixed param)
{
	TexEnvi(target, pname, (GLint)param);
}

void TexEnviv(GLenum target, GLenum pname, const GLint *params)
{
	UNIMPLEMENTED();
}

void TexEnvxv(GLenum target, GLenum pname, const GLfixed *params)
{
	UNIMPLEMENTED();
}

void TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint internalformat = %d, GLsizei width = %d, GLsizei height = %d, "
	      "GLint border = %d, GLenum format = 0x%X, GLenum type = 0x%X, const GLvoid* pixels =  %p)",
	      target, level, internalformat, width, height, border, format, type, pixels);

	if(!validImageSize(level, width, height))
	{
		return error(GL_INVALID_VALUE);
	}

	if(internalformat != (GLint)format)
	{
		return error(GL_INVALID_OPERATION);
	}

	switch(format)
	{
	case GL_ALPHA:
	case GL_LUMINANCE:
	case GL_LUMINANCE_ALPHA:
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
		break;
	case GL_RGB:
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT_5_6_5:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
		break;
	case GL_RGBA:
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_5_5_5_1:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
		break;
	case GL_BGRA_EXT:
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
		break;
	case GL_ETC1_RGB8_OES:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return error(GL_INVALID_OPERATION);
	case GL_DEPTH_STENCIL_OES:
		switch(type)
		{
		case GL_UNSIGNED_INT_24_8_OES:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
		break;
	default:
		return error(GL_INVALID_VALUE);
	}

	if(border != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	GLint sizedInternalFormat = gl::GetSizedInternalFormat(internalformat, type);

	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_TEXTURE_2D:
			if(width > (es1::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
			   height > (es1::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
			{
				return error(GL_INVALID_VALUE);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		if(target == GL_TEXTURE_2D)
		{
			es1::Texture2D *texture = context->getTexture2D();

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			texture->setImage(level, width, height, sizedInternalFormat, format, type, context->getUnpackAlignment(), pixels);
		}
		else UNREACHABLE(target);
	}
}

void TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat param = %f)", target, pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D();
			break;
		case GL_TEXTURE_EXTERNAL_OES:
			texture = context->getTextureExternal();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		switch(pname)
		{
		case GL_TEXTURE_WRAP_S:
			if(!texture->setWrapS((GLenum)param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_TEXTURE_WRAP_T:
			if(!texture->setWrapT((GLenum)param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_TEXTURE_MIN_FILTER:
			if(!texture->setMinFilter((GLenum)param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_TEXTURE_MAG_FILTER:
			if(!texture->setMagFilter((GLenum)param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_TEXTURE_MAX_ANISOTROPY_EXT:
			if(!texture->setMaxAnisotropy(param))
			{
				return error(GL_INVALID_VALUE);
			}
			break;
		case GL_GENERATE_MIPMAP:
			texture->setGenerateMipmap((GLboolean)param);
			break;
		case GL_TEXTURE_CROP_RECT_OES:
			return error(GL_INVALID_ENUM);   // Needs four values, should call glTexParameterfv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void TexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
	TexParameterf(target, pname, *params);
}

void TexParameteri(GLenum target, GLenum pname, GLint param)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint param = %d)", target, pname, param);

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D();
			break;
		case GL_TEXTURE_EXTERNAL_OES:
			texture = context->getTextureExternal();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		switch(pname)
		{
		case GL_TEXTURE_WRAP_S:
			if(!texture->setWrapS((GLenum)param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_TEXTURE_WRAP_T:
			if(!texture->setWrapT((GLenum)param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_TEXTURE_MIN_FILTER:
			if(!texture->setMinFilter((GLenum)param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_TEXTURE_MAG_FILTER:
			if(!texture->setMagFilter((GLenum)param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_TEXTURE_MAX_ANISOTROPY_EXT:
			if(!texture->setMaxAnisotropy((GLfloat)param))
			{
				return error(GL_INVALID_VALUE);
			}
			break;
		case GL_GENERATE_MIPMAP:
			texture->setGenerateMipmap((GLboolean)param);
			break;
		case GL_TEXTURE_CROP_RECT_OES:
			return error(GL_INVALID_ENUM);   // Needs four values, should call glTexParameteriv() instead
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void TexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint param = %p)", target, pname, params);

	switch(pname)
	{
	case GL_TEXTURE_CROP_RECT_OES:
		break;
	default:
		return TexParameteri(target, pname, params[0]);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		switch(pname)
		{
		case GL_TEXTURE_CROP_RECT_OES:
			texture->setCropRect(params[0], params[1], params[2], params[3]);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void TexParameterx(GLenum target, GLenum pname, GLfixed param)
{
	TexParameteri(target, pname, (GLint)param);
}

void TexParameterxv(GLenum target, GLenum pname, const GLfixed *params)
{
	UNIMPLEMENTED();
}

void TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* pixels)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
	      "GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, GLenum type = 0x%X, "
	      "const GLvoid* pixels = %p)",
	      target, level, xoffset, yoffset, width, height, format, type, pixels);

	if(!es1::IsTextureTarget(target))
	{
		return error(GL_INVALID_ENUM);
	}

	if(level < 0 || level >= es1::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
	{
		return error(GL_INVALID_VALUE);
	}

	if(xoffset < 0 || yoffset < 0 || width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(std::numeric_limits<GLsizei>::max() - xoffset < width || std::numeric_limits<GLsizei>::max() - yoffset < height)
	{
		return error(GL_INVALID_VALUE);
	}

	if(width == 0 || height == 0 || !pixels)
	{
		return;
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		if(target == GL_TEXTURE_2D)
		{
			es1::Texture2D *texture = context->getTexture2D();

			GLenum validationError = ValidateSubImageParams(false, false, target, level, xoffset, yoffset, width, height, format, type, texture);
			if(validationError != GL_NO_ERROR)
			{
				return error(validationError);
			}

			texture->subImage(level, xoffset, yoffset, width, height, format, type, context->getUnpackAlignment(), pixels);
		}
		else UNREACHABLE(target);
	}
}

void Translatef(GLfloat x, GLfloat y, GLfloat z)
{
	TRACE("(GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", x, y, z);

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->translate(x, y, z);
	}
}

void Translatex(GLfixed x, GLfixed y, GLfixed z)
{
	Translatef((float)x / 0x10000, (float)y / 0x10000, (float)z / 0x10000);
}

void VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(GLint size = %d, GLenum type = 0x%X, GLsizei stride = %d, const GLvoid *pointer = %p)", size, type, stride, pointer);

	if(size < 2 || size > 4)
	{
		return error(GL_INVALID_VALUE);
	}

	VertexAttribPointer(sw::Position, size, type, false, stride, pointer);
}

void Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)", x, y, width, height);

	if(width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->setViewportParams(x, y, width, height);
	}
}

void EGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
	TRACE("(GLenum target = 0x%X, GLeglImageOES image = %p)", target, image);

	switch(target)
	{
	case GL_TEXTURE_2D:
	case GL_TEXTURE_EXTERNAL_OES:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		es1::Texture2D *texture = nullptr;

		switch(target)
		{
		case GL_TEXTURE_2D:           texture = context->getTexture2D();       break;
		case GL_TEXTURE_EXTERNAL_OES: texture = context->getTextureExternal(); break;
		default:                      UNREACHABLE(target);
		}

		if(!texture)
		{
			return error(GL_INVALID_OPERATION);
		}

		egl::Image *eglImage = context->getSharedImage(image);

		if(!eglImage)
		{
			return error(GL_INVALID_OPERATION);
		}

		texture->setSharedImage(eglImage);
	}
}

void EGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image)
{
	TRACE("(GLenum target = 0x%X, GLeglImageOES image = %p)", target, image);

	UNIMPLEMENTED();
}

void DrawTexsOES(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height)
{
	UNIMPLEMENTED();
}

void DrawTexiOES(GLint x, GLint y, GLint z, GLint width, GLint height)
{
	TRACE("(GLint x = %d, GLint y = %d, GLint z = %d, GLint width = %d, GLint height = %d)", x, y, z, width, height);

	if(width <= 0 || height <= 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->drawTexture((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)width, (GLfloat)height);
	}
}

void DrawTexxOES(GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height)
{
	UNIMPLEMENTED();
}

void DrawTexsvOES(const GLshort *coords)
{
	UNIMPLEMENTED();
}

void DrawTexivOES(const GLint *coords)
{
	UNIMPLEMENTED();
}

void DrawTexxvOES(const GLfixed *coords)
{
	UNIMPLEMENTED();
}

void DrawTexfOES(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height)
{
	TRACE("(GLfloat x = %f, GLfloat y = %f, GLfloat z = %f, GLfloat width = %f, GLfloat height = %f)", x, y, z, width, height);

	if(width <= 0 || height <= 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es1::Context *context = es1::getContext();

	if(context)
	{
		context->drawTexture(x, y, z, width, height);
	}
}

void DrawTexfvOES(const GLfloat *coords)
{
	UNIMPLEMENTED();
}

}

extern "C" __eglMustCastToProperFunctionPointerType es1GetProcAddress(const char *procname)
{
	struct Function
	{
		const char *name;
		__eglMustCastToProperFunctionPointerType address;
	};

	struct CompareFunctor
	{
		bool operator()(const Function &a, const Function &b) const
		{
			return strcmp(a.name, b.name) < 0;
		}
	};

	// This array must be kept sorted with respect to strcmp(), so that binary search works correctly.
	// The Unix command "LC_COLLATE=C sort" will generate the correct order.
	static const Function glFunctions[] =
	{
		#define FUNCTION(name) {#name, (__eglMustCastToProperFunctionPointerType)name}

		FUNCTION(glActiveTexture),
		FUNCTION(glAlphaFunc),
		FUNCTION(glAlphaFuncx),
		FUNCTION(glBindBuffer),
		FUNCTION(glBindFramebufferOES),
		FUNCTION(glBindRenderbufferOES),
		FUNCTION(glBindTexture),
		FUNCTION(glBlendEquationOES),
		FUNCTION(glBlendEquationSeparateOES),
		FUNCTION(glBlendFunc),
		FUNCTION(glBlendFuncSeparateOES),
		FUNCTION(glBufferData),
		FUNCTION(glBufferSubData),
		FUNCTION(glCheckFramebufferStatusOES),
		FUNCTION(glClear),
		FUNCTION(glClearColor),
		FUNCTION(glClearColorx),
		FUNCTION(glClearDepthf),
		FUNCTION(glClearDepthx),
		FUNCTION(glClearStencil),
		FUNCTION(glClientActiveTexture),
		FUNCTION(glClipPlanef),
		FUNCTION(glClipPlanex),
		FUNCTION(glColor4f),
		FUNCTION(glColor4ub),
		FUNCTION(glColor4x),
		FUNCTION(glColorMask),
		FUNCTION(glColorPointer),
		FUNCTION(glCompressedTexImage2D),
		FUNCTION(glCompressedTexSubImage2D),
		FUNCTION(glCopyTexImage2D),
		FUNCTION(glCopyTexSubImage2D),
		FUNCTION(glCullFace),
		FUNCTION(glDeleteBuffers),
		FUNCTION(glDeleteFramebuffersOES),
		FUNCTION(glDeleteRenderbuffersOES),
		FUNCTION(glDeleteTextures),
		FUNCTION(glDepthFunc),
		FUNCTION(glDepthMask),
		FUNCTION(glDepthRangef),
		FUNCTION(glDepthRangex),
		FUNCTION(glDisable),
		FUNCTION(glDisableClientState),
		FUNCTION(glDrawArrays),
		FUNCTION(glDrawElements),
		FUNCTION(glDrawTexfOES),
		FUNCTION(glDrawTexfvOES),
		FUNCTION(glDrawTexiOES),
		FUNCTION(glDrawTexivOES),
		FUNCTION(glDrawTexsOES),
		FUNCTION(glDrawTexsvOES),
		FUNCTION(glDrawTexxOES),
		FUNCTION(glDrawTexxvOES),
		FUNCTION(glEGLImageTargetRenderbufferStorageOES),
		FUNCTION(glEGLImageTargetTexture2DOES),
		FUNCTION(glEnable),
		FUNCTION(glEnableClientState),
		FUNCTION(glFinish),
		FUNCTION(glFlush),
		FUNCTION(glFogf),
		FUNCTION(glFogfv),
		FUNCTION(glFogx),
		FUNCTION(glFogxv),
		FUNCTION(glFramebufferRenderbufferOES),
		FUNCTION(glFramebufferTexture2DOES),
		FUNCTION(glFrontFace),
		FUNCTION(glFrustumf),
		FUNCTION(glFrustumx),
		FUNCTION(glGenBuffers),
		FUNCTION(glGenFramebuffersOES),
		FUNCTION(glGenRenderbuffersOES),
		FUNCTION(glGenTextures),
		FUNCTION(glGenerateMipmapOES),
		FUNCTION(glGetBooleanv),
		FUNCTION(glGetBufferParameteriv),
		FUNCTION(glGetClipPlanef),
		FUNCTION(glGetClipPlanex),
		FUNCTION(glGetError),
		FUNCTION(glGetFixedv),
		FUNCTION(glGetFloatv),
		FUNCTION(glGetFramebufferAttachmentParameterivOES),
		FUNCTION(glGetIntegerv),
		FUNCTION(glGetLightfv),
		FUNCTION(glGetLightxv),
		FUNCTION(glGetMaterialfv),
		FUNCTION(glGetMaterialxv),
		FUNCTION(glGetPointerv),
		FUNCTION(glGetRenderbufferParameterivOES),
		FUNCTION(glGetString),
		FUNCTION(glGetTexEnvfv),
		FUNCTION(glGetTexEnviv),
		FUNCTION(glGetTexEnvxv),
		FUNCTION(glGetTexParameterfv),
		FUNCTION(glGetTexParameteriv),
		FUNCTION(glGetTexParameterxv),
		FUNCTION(glHint),
		FUNCTION(glIsBuffer),
		FUNCTION(glIsEnabled),
		FUNCTION(glIsFramebufferOES),
		FUNCTION(glIsRenderbufferOES),
		FUNCTION(glIsTexture),
		FUNCTION(glLightModelf),
		FUNCTION(glLightModelfv),
		FUNCTION(glLightModelx),
		FUNCTION(glLightModelxv),
		FUNCTION(glLightf),
		FUNCTION(glLightfv),
		FUNCTION(glLightx),
		FUNCTION(glLightxv),
		FUNCTION(glLineWidth),
		FUNCTION(glLineWidthx),
		FUNCTION(glLoadIdentity),
		FUNCTION(glLoadMatrixf),
		FUNCTION(glLoadMatrixx),
		FUNCTION(glLogicOp),
		FUNCTION(glMaterialf),
		FUNCTION(glMaterialfv),
		FUNCTION(glMaterialx),
		FUNCTION(glMaterialxv),
		FUNCTION(glMatrixMode),
		FUNCTION(glMultMatrixf),
		FUNCTION(glMultMatrixx),
		FUNCTION(glMultiTexCoord4f),
		FUNCTION(glMultiTexCoord4x),
		FUNCTION(glNormal3f),
		FUNCTION(glNormal3x),
		FUNCTION(glNormalPointer),
		FUNCTION(glOrthof),
		FUNCTION(glOrthox),
		FUNCTION(glPixelStorei),
		FUNCTION(glPointParameterf),
		FUNCTION(glPointParameterfv),
		FUNCTION(glPointParameterx),
		FUNCTION(glPointParameterxv),
		FUNCTION(glPointSize),
		FUNCTION(glPointSizePointerOES),
		FUNCTION(glPointSizex),
		FUNCTION(glPolygonOffset),
		FUNCTION(glPolygonOffsetx),
		FUNCTION(glPopMatrix),
		FUNCTION(glPushMatrix),
		FUNCTION(glReadPixels),
		FUNCTION(glRenderbufferStorageOES),
		FUNCTION(glRotatef),
		FUNCTION(glRotatex),
		FUNCTION(glSampleCoverage),
		FUNCTION(glSampleCoveragex),
		FUNCTION(glScalef),
		FUNCTION(glScalex),
		FUNCTION(glScissor),
		FUNCTION(glShadeModel),
		FUNCTION(glStencilFunc),
		FUNCTION(glStencilMask),
		FUNCTION(glStencilOp),
		FUNCTION(glTexCoordPointer),
		FUNCTION(glTexEnvf),
		FUNCTION(glTexEnvfv),
		FUNCTION(glTexEnvi),
		FUNCTION(glTexEnviv),
		FUNCTION(glTexEnvx),
		FUNCTION(glTexEnvxv),
		FUNCTION(glTexImage2D),
		FUNCTION(glTexParameterf),
		FUNCTION(glTexParameterfv),
		FUNCTION(glTexParameteri),
		FUNCTION(glTexParameteriv),
		FUNCTION(glTexParameterx),
		FUNCTION(glTexParameterxv),
		FUNCTION(glTexSubImage2D),
		FUNCTION(glTranslatef),
		FUNCTION(glTranslatex),
		FUNCTION(glVertexPointer),
		FUNCTION(glViewport),

		#undef FUNCTION
	};

	static const size_t numFunctions = sizeof glFunctions / sizeof(Function);
	static const Function *const glFunctionsEnd = glFunctions + numFunctions;

	Function needle;
	needle.name = procname;

	if(procname && strncmp("gl", procname, 2) == 0)
	{
		const Function *result = std::lower_bound(glFunctions, glFunctionsEnd, needle, CompareFunctor());
		if(result != glFunctionsEnd && strcmp(procname, result->name) == 0)
		{
			return (__eglMustCastToProperFunctionPointerType)result->address;
		}
	}

	return nullptr;
}
