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
// libGL.cpp: Implements the exported OpenGL functions.

#include "main.h"
#include "mathutil.h"
#include "utilities.h"
#include "Buffer.h"
#include "Context.h"
#include "Fence.h"
#include "Framebuffer.h"
#include "Program.h"
#include "Renderbuffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Query.h"
#include "common/debug.h"
#include "Common/Version.h"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

#include <limits>

using std::abs;

static bool validImageSize(GLint level, GLsizei width, GLsizei height)
{
	if(level < 0 || level >= gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS || width < 0 || height < 0)
	{
		return false;
	}

	return true;
}

static bool validateSubImageParams(bool compressed, GLsizei width, GLsizei height, GLint xoffset, GLint yoffset, GLenum target, GLint level, GLenum format, gl::Texture *texture)
{
	if(!texture)
	{
		return error(GL_INVALID_OPERATION, false);
	}

	if(compressed != texture->isCompressed(target, level))
	{
		return error(GL_INVALID_OPERATION, false);
	}

	if(format != GL_NONE && format != texture->getFormat(target, level))
	{
		return error(GL_INVALID_OPERATION, false);
	}

	if(compressed)
	{
		if((width % 4 != 0 && width != texture->getWidth(target, 0)) ||
		   (height % 4 != 0 && height != texture->getHeight(target, 0)))
		{
			return error(GL_INVALID_OPERATION, false);
		}
	}

	if(xoffset + width > texture->getWidth(target, level) ||
	   yoffset + height > texture->getHeight(target, level))
	{
		return error(GL_INVALID_VALUE, false);
	}

	return true;
}

// Check for combinations of format and type that are valid for ReadPixels
static bool validReadFormatType(GLenum format, GLenum type)
{
	switch(format)
	{
	case GL_RGBA:
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
			break;
		default:
			return false;
		}
		break;
	case GL_BGRA_EXT:
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			break;
		default:
			return false;
		}
		break;
	case gl::IMPLEMENTATION_COLOR_READ_FORMAT:
		switch(type)
		{
		case gl::IMPLEMENTATION_COLOR_READ_TYPE:
			break;
		default:
			return false;
		}
		break;
	default:
		return false;
	}

	return true;
}

extern "C"
{

void APIENTRY glActiveTexture(GLenum texture)
{
	TRACE("(GLenum texture = 0x%X)", texture);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(texture < GL_TEXTURE0 || texture > GL_TEXTURE0 + gl::MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
		{
			return error(GL_INVALID_ENUM);
		}

		context->setActiveSampler(texture - GL_TEXTURE0);
	}
}

void APIENTRY glAttachShader(GLuint program, GLuint shader)
{
	TRACE("(GLuint program = %d, GLuint shader = %d)", program, shader);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);
		gl::Shader *shaderObject = context->getShader(shader);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		if(!shaderObject)
		{
			if(context->getProgram(shader))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		if(!programObject->attachShader(shaderObject))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glBeginQueryEXT(GLenum target, GLuint name)
{
	TRACE("(GLenum target = 0x%X, GLuint name = %d)", target, name);

	switch(target)
	{
	case GL_ANY_SAMPLES_PASSED:
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(name == 0)
	{
		return error(GL_INVALID_OPERATION);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->beginQuery(target, name);
	}
}

void APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar* name)
{
	TRACE("(GLuint program = %d, GLuint index = %d, const GLchar* name = %s)", program, index, name);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		if(strncmp(name, "gl_", 3) == 0)
		{
			return error(GL_INVALID_OPERATION);
		}

		programObject->bindAttributeLocation(index, name);
	}
}

void APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
	TRACE("(GLenum target = 0x%X, GLuint buffer = %d)", target, buffer);

	gl::Context *context = gl::getContext();

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

void APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	TRACE("(GLenum target = 0x%X, GLuint framebuffer = %d)", target, framebuffer);

	if(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_EXT && target != GL_READ_FRAMEBUFFER_EXT)
	{
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(target == GL_READ_FRAMEBUFFER_EXT || target == GL_FRAMEBUFFER)
		{
			context->bindReadFramebuffer(framebuffer);
		}

		if(target == GL_DRAW_FRAMEBUFFER_EXT || target == GL_FRAMEBUFFER)
		{
			context->bindDrawFramebuffer(framebuffer);
		}
	}
}

void APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	TRACE("(GLenum target = 0x%X, GLuint renderbuffer = %d)", target, renderbuffer);

	if(target != GL_RENDERBUFFER)
	{
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->bindRenderbuffer(renderbuffer);
	}
}

void APIENTRY glBindTexture(GLenum target, GLuint texture)
{
	TRACE("(GLenum target = 0x%X, GLuint texture = %d)", target, texture);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Texture *textureObject = context->getTexture(texture);

		if(textureObject && textureObject->getTarget() != target && texture != 0)
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(target)
		{
		case GL_TEXTURE_2D:
			context->bindTexture2D(texture);
			return;
		case GL_TEXTURE_CUBE_MAP:
			context->bindTextureCubeMap(texture);
			return;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	TRACE("(GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f)",
	      red, green, blue, alpha);

	gl::Context* context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setBlendColor(gl::clamp01(red), gl::clamp01(green), gl::clamp01(blue), gl::clamp01(alpha));
	}
}

void APIENTRY glBlendEquation(GLenum mode)
{
	glBlendEquationSeparate(mode, mode);
}

void APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	TRACE("(GLenum modeRGB = 0x%X, GLenum modeAlpha = 0x%X)", modeRGB, modeAlpha);

	switch(modeRGB)
	{
	case GL_FUNC_ADD:
	case GL_FUNC_SUBTRACT:
	case GL_FUNC_REVERSE_SUBTRACT:
	case GL_MIN_EXT:
	case GL_MAX_EXT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(modeAlpha)
	{
	case GL_FUNC_ADD:
	case GL_FUNC_SUBTRACT:
	case GL_FUNC_REVERSE_SUBTRACT:
	case GL_MIN_EXT:
	case GL_MAX_EXT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setBlendEquation(modeRGB, modeAlpha);
	}
}

void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
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
	case GL_CONSTANT_COLOR:
	case GL_ONE_MINUS_CONSTANT_COLOR:
	case GL_CONSTANT_ALPHA:
	case GL_ONE_MINUS_CONSTANT_ALPHA:
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
	case GL_CONSTANT_COLOR:
	case GL_ONE_MINUS_CONSTANT_COLOR:
	case GL_CONSTANT_ALPHA:
	case GL_ONE_MINUS_CONSTANT_ALPHA:
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
	case GL_CONSTANT_COLOR:
	case GL_ONE_MINUS_CONSTANT_COLOR:
	case GL_CONSTANT_ALPHA:
	case GL_ONE_MINUS_CONSTANT_ALPHA:
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
	case GL_CONSTANT_COLOR:
	case GL_ONE_MINUS_CONSTANT_COLOR:
	case GL_CONSTANT_ALPHA:
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setBlendFactors(srcRGB, dstRGB, srcAlpha, dstAlpha);
	}
}

void APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	TRACE("(GLenum target = 0x%X, GLsizeiptr size = %d, const GLvoid* data = %p, GLenum usage = %d)",
	      target, size, data, usage);

	if(size < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(usage)
	{
	case GL_STREAM_DRAW:
	case GL_STATIC_DRAW:
	case GL_DYNAMIC_DRAW:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Buffer *buffer;

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

void APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
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

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Buffer *buffer;

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

GLenum APIENTRY glCheckFramebufferStatus(GLenum target)
{
	TRACE("(GLenum target = 0x%X)", target);

	if(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_EXT && target != GL_READ_FRAMEBUFFER_EXT)
	{
		return error(GL_INVALID_ENUM, 0);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Framebuffer *framebuffer = nullptr;
		if(target == GL_READ_FRAMEBUFFER_EXT)
		{
			framebuffer = context->getReadFramebuffer();
		}
		else
		{
			framebuffer = context->getDrawFramebuffer();
		}

		return framebuffer->completeness();
	}

	return 0;
}

void APIENTRY glClear(GLbitfield mask)
{
	TRACE("(GLbitfield mask = %X)", mask);

	if((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			return context->listCommand(gl::newCommand(glClear, mask));
		}

		context->clear(mask);
	}
}

void APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	TRACE("(GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f)",
	      red, green, blue, alpha);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setClearColor(red, green, blue, alpha);
	}
}

void APIENTRY glClearDepthf(GLclampf depth)
{
	TRACE("(GLclampf depth = %f)", depth);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setClearDepth(depth);
	}
}

void APIENTRY glClearStencil(GLint s)
{
	TRACE("(GLint s = %d)", s);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setClearStencil(s);
	}
}

void APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	TRACE("(GLboolean red = %d, GLboolean green = %d, GLboolean blue = %d, GLboolean alpha = %d)",
	      red, green, blue, alpha);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setColorMask(red == GL_TRUE, green == GL_TRUE, blue == GL_TRUE, alpha == GL_TRUE);
	}
}

void APIENTRY glCompileShader(GLuint shader)
{
	TRACE("(GLuint shader = %d)", shader);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Shader *shaderObject = context->getShader(shader);

		if(!shaderObject)
		{
			if(context->getProgram(shader))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		shaderObject->compile();
	}
}

void APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                     GLint border, GLsizei imageSize, const GLvoid* data)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLsizei width = %d, "
	      "GLsizei height = %d, GLint border = %d, GLsizei imageSize = %d, const GLvoid* data = %p)",
	      target, level, internalformat, width, height, border, imageSize, data);

	if(!validImageSize(level, width, height) || border != 0 || imageSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(internalformat)
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		break;
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_STENCIL_EXT:
	case GL_DEPTH24_STENCIL8_EXT:
		return error(GL_INVALID_OPERATION);
	default:
		return error(GL_INVALID_ENUM);
	}

	if(border != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(level > gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
		{
			return error(GL_INVALID_VALUE);
		}

		switch(target)
		{
		case GL_TEXTURE_2D:
			if(width > (gl::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
			   height > (gl::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
			{
				return error(GL_INVALID_VALUE);
			}
			break;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
			if(width != height)
			{
				return error(GL_INVALID_VALUE);
			}

			if(width > (gl::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level) ||
			   height > (gl::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level))
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
			gl::Texture2D *texture = context->getTexture2D(target);

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			texture->setCompressedImage(level, internalformat, width, height, imageSize, data);
		}
		else
		{
			gl::TextureCubeMap *texture = context->getTextureCubeMap();

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			switch(target)
			{
			case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
				texture->setCompressedImage(target, level, internalformat, width, height, imageSize, data);
				break;
			default: UNREACHABLE(target);
			}
		}
	}
}

void APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                        GLenum format, GLsizei imageSize, const GLvoid* data)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
	      "GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, "
	      "GLsizei imageSize = %d, const GLvoid* data = %p)",
	      target, level, xoffset, yoffset, width, height, format, imageSize, data);

	if(!gl::IsTextureTarget(target))
	{
		return error(GL_INVALID_ENUM);
	}

	if(xoffset < 0 || yoffset < 0 || !validImageSize(level, width, height) || imageSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(format)
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(width == 0 || height == 0 || !data)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(level > gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
		{
			return error(GL_INVALID_VALUE);
		}

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
			gl::Texture2D *texture = context->getTexture2D(target);

			if(validateSubImageParams(true, width, height, xoffset, yoffset, target, level, format, texture))
			{
				texture->subImageCompressed(level, xoffset, yoffset, width, height, format, imageSize, data);
			}
		}
		else if(gl::IsCubemapTextureTarget(target))
		{
			gl::TextureCubeMap *texture = context->getTextureCubeMap();

			if(validateSubImageParams(true, width, height, xoffset, yoffset, target, level, format, texture))
			{
				texture->subImageCompressed(target, level, xoffset, yoffset, width, height, format, imageSize, data);
			}
		}
		else UNREACHABLE(target);
	}
}

void APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
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

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		switch(target)
		{
		case GL_TEXTURE_2D:
			if(width > (gl::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
			   height > (gl::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
			{
				return error(GL_INVALID_VALUE);
			}
			break;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
			if(width != height)
			{
				return error(GL_INVALID_VALUE);
			}

			if(width > (gl::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level) ||
			   height > (gl::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level))
			{
				return error(GL_INVALID_VALUE);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		gl::Framebuffer *framebuffer = context->getReadFramebuffer();

		if(framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
		{
			return error(GL_INVALID_FRAMEBUFFER_OPERATION);
		}

		if(context->getReadFramebufferName() != 0 && framebuffer->getColorbuffer()->getSamples() > 1)
		{
			return error(GL_INVALID_OPERATION);
		}

		gl::Renderbuffer *source = framebuffer->getColorbuffer();
		GLenum colorbufferFormat = source->getFormat();

		switch(internalformat)
		{
		case GL_ALPHA:
			if(colorbufferFormat != GL_ALPHA &&
			   colorbufferFormat != GL_RGBA &&
			   colorbufferFormat != GL_RGBA4 &&
			   colorbufferFormat != GL_RGB5_A1 &&
			   colorbufferFormat != GL_RGBA8_EXT)
			{
				return error(GL_INVALID_OPERATION);
			}
			break;
		case GL_LUMINANCE:
		case GL_RGB:
			if(colorbufferFormat != GL_RGB &&
			   colorbufferFormat != GL_RGB565 &&
			   colorbufferFormat != GL_RGB8_EXT &&
			   colorbufferFormat != GL_RGBA &&
			   colorbufferFormat != GL_RGBA4 &&
			   colorbufferFormat != GL_RGB5_A1 &&
			   colorbufferFormat != GL_RGBA8_EXT)
			{
				return error(GL_INVALID_OPERATION);
			}
			break;
		case GL_LUMINANCE_ALPHA:
		case GL_RGBA:
			if(colorbufferFormat != GL_RGBA &&
			   colorbufferFormat != GL_RGBA4 &&
			   colorbufferFormat != GL_RGB5_A1 &&
			   colorbufferFormat != GL_RGBA8_EXT)
			{
				return error(GL_INVALID_OPERATION);
			}
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return error(GL_INVALID_OPERATION);
		default:
			return error(GL_INVALID_ENUM);
		}

		if(target == GL_TEXTURE_2D)
		{
			gl::Texture2D *texture = context->getTexture2D(target);

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			texture->copyImage(level, internalformat, x, y, width, height, framebuffer);
		}
		else if(gl::IsCubemapTextureTarget(target))
		{
			gl::TextureCubeMap *texture = context->getTextureCubeMap();

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			texture->copyImage(target, level, internalformat, x, y, width, height, framebuffer);
		}
		else UNREACHABLE(target);
	}
}

void APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
	      "GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)",
	      target, level, xoffset, yoffset, x, y, width, height);

	if(!gl::IsTextureTarget(target))
	{
		return error(GL_INVALID_ENUM);
	}

	if(level < 0 || xoffset < 0 || yoffset < 0 || width < 0 || height < 0)
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

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(level > gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
		{
			return error(GL_INVALID_VALUE);
		}

		gl::Framebuffer *framebuffer = context->getReadFramebuffer();

		if(framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
		{
			return error(GL_INVALID_FRAMEBUFFER_OPERATION);
		}

		if(context->getReadFramebufferName() != 0 && framebuffer->getColorbuffer()->getSamples() > 1)
		{
			return error(GL_INVALID_OPERATION);
		}

		gl::Texture *texture = nullptr;

		if(target == GL_TEXTURE_2D)
		{
			texture = context->getTexture2D(target);
		}
		else if(gl::IsCubemapTextureTarget(target))
		{
			texture = context->getTextureCubeMap();
		}
		else UNREACHABLE(target);

		if(!validateSubImageParams(false, width, height, xoffset, yoffset, target, level, GL_NONE, texture))
		{
			return;
		}

		texture->copySubImage(target, level, xoffset, yoffset, x, y, width, height, framebuffer);
	}
}

GLuint APIENTRY glCreateProgram(void)
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		return context->createProgram();
	}

	return 0;
}

GLuint APIENTRY glCreateShader(GLenum type)
{
	TRACE("(GLenum type = 0x%X)", type);

	gl::Context *context = gl::getContext();

	if(context)
	{
		switch(type)
		{
		case GL_FRAGMENT_SHADER:
		case GL_VERTEX_SHADER:
			return context->createShader(type);
		default:
			return error(GL_INVALID_ENUM, 0);
		}
	}

	return 0;
}

void APIENTRY glCullFace(GLenum mode)
{
	TRACE("(GLenum mode = 0x%X)", mode);

	switch(mode)
	{
	case GL_FRONT:
	case GL_BACK:
	case GL_FRONT_AND_BACK:
		{
			gl::Context *context = gl::getContext();

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

void APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	TRACE("(GLsizei n = %d, const GLuint* buffers = %p)", n, buffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			context->deleteBuffer(buffers[i]);
		}
	}
}

void APIENTRY glDeleteFencesNV(GLsizei n, const GLuint* fences)
{
	TRACE("(GLsizei n = %d, const GLuint* fences = %p)", n, fences);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		for(int i = 0; i < n; i++)
		{
			context->deleteFence(fences[i]);
		}
	}
}

void APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
	TRACE("(GLsizei n = %d, const GLuint* framebuffers = %p)", n, framebuffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		for(int i = 0; i < n; i++)
		{
			if(framebuffers[i] != 0)
			{
				context->deleteFramebuffer(framebuffers[i]);
			}
		}
	}
}

void APIENTRY glDeleteProgram(GLuint program)
{
	TRACE("(GLuint program = %d)", program);

	if(program == 0)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(!context->getProgram(program))
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		context->deleteProgram(program);
	}
}

void APIENTRY glDeleteQueriesEXT(GLsizei n, const GLuint *ids)
{
	TRACE("(GLsizei n = %d, const GLuint *ids = %p)", n, ids);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			context->deleteQuery(ids[i]);
		}
	}
}

void APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
	TRACE("(GLsizei n = %d, const GLuint* renderbuffers = %p)", n, renderbuffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		for(int i = 0; i < n; i++)
		{
			context->deleteRenderbuffer(renderbuffers[i]);
		}
	}
}

void APIENTRY glDeleteShader(GLuint shader)
{
	TRACE("(GLuint shader = %d)", shader);

	if(shader == 0)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(!context->getShader(shader))
		{
			if(context->getProgram(shader))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		context->deleteShader(shader);
	}
}

void APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures)
{
	TRACE("(GLsizei n = %d, const GLuint* textures = %p)", n, textures);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

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

void APIENTRY glDepthFunc(GLenum func)
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

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setDepthFunc(func);
	}
}

void APIENTRY glDepthMask(GLboolean flag)
{
	TRACE("(GLboolean flag = %d)", flag);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setDepthMask(flag != GL_FALSE);
	}
}

void APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar)
{
	TRACE("(GLclampf zNear = %f, GLclampf zFar = %f)", zNear, zFar);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setDepthRange(zNear, zFar);
	}
}

void APIENTRY glDetachShader(GLuint program, GLuint shader)
{
	TRACE("(GLuint program = %d, GLuint shader = %d)", program, shader);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);
		gl::Shader *shaderObject = context->getShader(shader);

		if(!programObject)
		{
			gl::Shader *shaderByProgramHandle;
			shaderByProgramHandle = context->getShader(program);
			if(!shaderByProgramHandle)
			{
				return error(GL_INVALID_VALUE);
			}
			else
			{
				return error(GL_INVALID_OPERATION);
			}
		}

		if(!shaderObject)
		{
			gl::Program *programByShaderHandle = context->getProgram(shader);
			if(!programByShaderHandle)
			{
				return error(GL_INVALID_VALUE);
			}
			else
			{
				return error(GL_INVALID_OPERATION);
			}
		}

		if(!programObject->detachShader(shaderObject))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glDisable(GLenum cap)
{
	TRACE("(GLenum cap = 0x%X)", cap);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

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
		case GL_FOG:                      context->setFogEnabled(false);                   break;
		case GL_ALPHA_TEST:               context->setAlphaTestEnabled(false);             break;
		case GL_TEXTURE_2D:               context->setTexture2DEnabled(false);             break;
		case GL_LIGHT0:                   context->setLightEnabled(0, false);              break;
		case GL_LIGHT1:                   context->setLightEnabled(1, false);              break;
		case GL_LIGHT2:                   context->setLightEnabled(2, false);              break;
		case GL_LIGHT3:                   context->setLightEnabled(3, false);              break;
		case GL_LIGHT4:                   context->setLightEnabled(4, false);              break;
		case GL_LIGHT5:                   context->setLightEnabled(5, false);              break;
		case GL_LIGHT6:                   context->setLightEnabled(6, false);              break;
		case GL_LIGHT7:                   context->setLightEnabled(7, false);              break;
		case GL_COLOR_MATERIAL:           context->setColorMaterialEnabled(false);         break;
		case GL_RESCALE_NORMAL:           context->setNormalizeNormalsEnabled(false);      break;
		case GL_COLOR_LOGIC_OP:           context->setColorLogicOpEnabled(false);          break;
		case GL_INDEX_LOGIC_OP:           UNIMPLEMENTED();
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glDisableVertexAttribArray(GLuint index)
{
	TRACE("(GLuint index = %d)", index);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->setVertexAttribArrayEnabled(index, false);
	}
}

void APIENTRY glCaptureAttribs()
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->captureAttribs();
	}
}

void APIENTRY glRestoreAttribs()
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->restoreAttribs();
	}
}

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	TRACE("(GLenum mode = 0x%X, GLint first = %d, GLsizei count = %d)", mode, first, count);

	if(count < 0 || first < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			ASSERT(context->getListMode() != GL_COMPILE_AND_EXECUTE);   // UNIMPLEMENTED!

			context->listCommand(gl::newCommand(glCaptureAttribs));
			context->captureDrawArrays(mode, first, count);
			context->listCommand(gl::newCommand(glDrawArrays, mode, first, count));
			context->listCommand(gl::newCommand(glRestoreAttribs));

			return;
		}

		context->drawArrays(mode, first, count);
	}
}

void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	TRACE("(GLenum mode = 0x%X, GLsizei count = %d, GLenum type = 0x%X, const GLvoid* indices = %p)",
	      mode, count, type, indices);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

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

void APIENTRY glEnable(GLenum cap)
{
	TRACE("(GLenum cap = 0x%X)", cap);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

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
		case GL_TEXTURE_2D:               context->setTexture2DEnabled(true);             break;
		case GL_ALPHA_TEST:               context->setAlphaTestEnabled(true);             break;
		case GL_COLOR_MATERIAL:           context->setColorMaterialEnabled(true);         break;
		case GL_FOG:                      context->setFogEnabled(true);                   break;
		case GL_LIGHTING:                 context->setLightingEnabled(true);              break;
		case GL_LIGHT0:                   context->setLightEnabled(0, true);              break;
		case GL_LIGHT1:                   context->setLightEnabled(1, true);              break;
		case GL_LIGHT2:                   context->setLightEnabled(2, true);              break;
		case GL_LIGHT3:                   context->setLightEnabled(3, true);              break;
		case GL_LIGHT4:                   context->setLightEnabled(4, true);              break;
		case GL_LIGHT5:                   context->setLightEnabled(5, true);              break;
		case GL_LIGHT6:                   context->setLightEnabled(6, true);              break;
		case GL_LIGHT7:                   context->setLightEnabled(7, true);              break;
		case GL_RESCALE_NORMAL:           context->setNormalizeNormalsEnabled(true);      break;
		case GL_COLOR_LOGIC_OP:           context->setColorLogicOpEnabled(true);          break;
		case GL_INDEX_LOGIC_OP:           UNIMPLEMENTED();
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glEnableVertexAttribArray(GLuint index)
{
	TRACE("(GLuint index = %d)", index);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->setVertexAttribArrayEnabled(index, true);
	}
}

void APIENTRY glEndQueryEXT(GLenum target)
{
	TRACE("GLenum target = 0x%X)", target);

	switch(target)
	{
	case GL_ANY_SAMPLES_PASSED:
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->endQuery(target);
	}
}

void APIENTRY glFinishFenceNV(GLuint fence)
{
	TRACE("(GLuint fence = %d)", fence);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Fence* fenceObject = context->getFence(fence);

		if(!fenceObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		fenceObject->finishFence();
	}
}

void APIENTRY glFinish(void)
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->finish();
	}
}

void APIENTRY glFlush(void)
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->flush();
	}
}

void APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum renderbuffertarget = 0x%X, "
	      "GLuint renderbuffer = %d)", target, attachment, renderbuffertarget, renderbuffer);

	if((target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_EXT && target != GL_READ_FRAMEBUFFER_EXT) ||
	   (renderbuffertarget != GL_RENDERBUFFER && renderbuffer != 0))
	{
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Framebuffer *framebuffer = nullptr;
		GLuint framebufferName = 0;
		if(target == GL_READ_FRAMEBUFFER_EXT)
		{
			framebuffer = context->getReadFramebuffer();
			framebufferName = context->getReadFramebufferName();
		}
		else
		{
			framebuffer = context->getDrawFramebuffer();
			framebufferName = context->getDrawFramebufferName();
		}

		if(!framebuffer || (framebufferName == 0 && renderbuffer != 0))
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0:
			framebuffer->setColorbuffer(GL_RENDERBUFFER, renderbuffer);
			break;
		case GL_DEPTH_ATTACHMENT:
			framebuffer->setDepthbuffer(GL_RENDERBUFFER, renderbuffer);
			break;
		case GL_STENCIL_ATTACHMENT:
			framebuffer->setStencilbuffer(GL_RENDERBUFFER, renderbuffer);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	UNIMPLEMENTED();
}

void APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum textarget = 0x%X, "
	      "GLuint texture = %d, GLint level = %d)", target, attachment, textarget, texture, level);

	if(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_EXT && target != GL_READ_FRAMEBUFFER_EXT)
	{
		return error(GL_INVALID_ENUM);
	}

	switch(attachment)
	{
	case GL_COLOR_ATTACHMENT0:
	case GL_DEPTH_ATTACHMENT:
	case GL_STENCIL_ATTACHMENT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(texture == 0)
		{
			textarget = GL_NONE;
		}
		else
		{
			gl::Texture *tex = context->getTexture(texture);

			if(!tex)
			{
				return error(GL_INVALID_OPERATION);
			}

			if(tex->isCompressed(textarget, level))
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
			case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
				if(tex->getTarget() != GL_TEXTURE_CUBE_MAP)
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
		}

		gl::Framebuffer *framebuffer = nullptr;
		GLuint framebufferName = 0;
		if(target == GL_READ_FRAMEBUFFER_EXT)
		{
			framebuffer = context->getReadFramebuffer();
			framebufferName = context->getReadFramebufferName();
		}
		else
		{
			framebuffer = context->getDrawFramebuffer();
			framebufferName = context->getDrawFramebufferName();
		}

		if(framebufferName == 0 || !framebuffer)
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0:  framebuffer->setColorbuffer(textarget, texture);   break;
		case GL_DEPTH_ATTACHMENT:   framebuffer->setDepthbuffer(textarget, texture);   break;
		case GL_STENCIL_ATTACHMENT: framebuffer->setStencilbuffer(textarget, texture); break;
		}
	}
}

void APIENTRY glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
	UNIMPLEMENTED();
}

void APIENTRY glFrontFace(GLenum mode)
{
	TRACE("(GLenum mode = 0x%X)", mode);

	switch(mode)
	{
	case GL_CW:
	case GL_CCW:
		{
			gl::Context *context = gl::getContext();

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

void APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{
	TRACE("(GLsizei n = %d, GLuint* buffers = %p)", n, buffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			buffers[i] = context->createBuffer();
		}
	}
}

void APIENTRY glGenerateMipmap(GLenum target)
{
	TRACE("(GLenum target = 0x%X)", target);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D(target);
			break;
		case GL_TEXTURE_CUBE_MAP:
			texture = context->getTextureCubeMap();
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

void APIENTRY glGenFencesNV(GLsizei n, GLuint* fences)
{
	TRACE("(GLsizei n = %d, GLuint* fences = %p)", n, fences);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		for(int i = 0; i < n; i++)
		{
			fences[i] = context->createFence();
		}
	}
}

void APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
	TRACE("(GLsizei n = %d, GLuint* framebuffers = %p)", n, framebuffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		for(int i = 0; i < n; i++)
		{
			framebuffers[i] = context->createFramebuffer();
		}
	}
}

void APIENTRY glGenQueriesEXT(GLsizei n, GLuint* ids)
{
	TRACE("(GLsizei n = %d, GLuint* ids = %p)", n, ids);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			ids[i] = context->createQuery();
		}
	}
}

void APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
	TRACE("(GLsizei n = %d, GLuint* renderbuffers = %p)", n, renderbuffers);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		for(int i = 0; i < n; i++)
		{
			renderbuffers[i] = context->createRenderbuffer();
		}
	}
}

void APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
	TRACE("(GLsizei n = %d, GLuint* textures = %p)", n, textures);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			textures[i] = context->createTexture();
		}
	}
}

void APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
	TRACE("(GLuint program = %d, GLuint index = %d, GLsizei bufsize = %d, GLsizei *length = %p, "
	      "GLint *size = %p, GLenum *type = %p, GLchar *name = %p)",
	      program, index, bufsize, length, size, type, name);

	if(bufsize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		if(index >= programObject->getActiveAttributeCount())
		{
			return error(GL_INVALID_VALUE);
		}

		programObject->getActiveAttribute(index, bufsize, length, size, type, name);
	}
}

void APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
	TRACE("(GLuint program = %d, GLuint index = %d, GLsizei bufsize = %d, "
	      "GLsizei* length = %p, GLint* size = %p, GLenum* type = %p, GLchar* name = %s)",
	      program, index, bufsize, length, size, type, name);

	if(bufsize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		if(index >= programObject->getActiveUniformCount())
		{
			return error(GL_INVALID_VALUE);
		}

		programObject->getActiveUniform(index, bufsize, length, size, type, name);
	}
}

void APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
	TRACE("(GLuint program = %d, GLsizei maxcount = %d, GLsizei* count = %p, GLuint* shaders = %p)",
	      program, maxcount, count, shaders);

	if(maxcount < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		return programObject->getAttachedShaders(maxcount, count, shaders);
	}
}

int APIENTRY glGetAttribLocation(GLuint program, const GLchar* name)
{
	TRACE("(GLuint program = %d, const GLchar* name = %s)", program, name);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION, -1);
			}
			else
			{
				return error(GL_INVALID_VALUE, -1);
			}
		}

		if(!programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION, -1);
		}

		return programObject->getAttributeLocation(name);
	}

	return -1;
}

void APIENTRY glGetBooleanv(GLenum pname, GLboolean* params)
{
	TRACE("(GLenum pname = 0x%X, GLboolean* params = %p)",  pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(!(context->getBooleanv(pname, params)))
		{
			GLenum nativeType;
			unsigned int numParams = 0;
			if(!context->getQueryParameterInfo(pname, &nativeType, &numParams))
				return error(GL_INVALID_ENUM);

			if(numParams == 0)
				return; // it is known that the pname is valid, but there are no parameters to return

			if(nativeType == GL_FLOAT)
			{
				GLfloat *floatParams = nullptr;
				floatParams = new GLfloat[numParams];

				context->getFloatv(pname, floatParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					if(floatParams[i] == 0.0f)
						params[i] = GL_FALSE;
					else
						params[i] = GL_TRUE;
				}

				delete [] floatParams;
			}
			else if(nativeType == GL_INT)
			{
				GLint *intParams = nullptr;
				intParams = new GLint[numParams];

				context->getIntegerv(pname, intParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					if(intParams[i] == 0)
						params[i] = GL_FALSE;
					else
						params[i] = GL_TRUE;
				}

				delete [] intParams;
			}
		}
	}
}

void APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = %p)", target, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Buffer *buffer;

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
			*params = buffer->size();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GLenum APIENTRY glGetError(void)
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		return context->getError();
	}

	return GL_NO_ERROR;
}

void APIENTRY glGetFenceivNV(GLuint fence, GLenum pname, GLint *params)
{
	TRACE("(GLuint fence = %d, GLenum pname = 0x%X, GLint *params = %p)", fence, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Fence *fenceObject = context->getFence(fence);

		if(!fenceObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		fenceObject->getFenceiv(pname, params);
	}
}

void APIENTRY glGetFloatv(GLenum pname, GLfloat* params)
{
	TRACE("(GLenum pname = 0x%X, GLfloat* params = %p)", pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(!(context->getFloatv(pname, params)))
		{
			GLenum nativeType;
			unsigned int numParams = 0;
			if(!context->getQueryParameterInfo(pname, &nativeType, &numParams))
				return error(GL_INVALID_ENUM);

			if(numParams == 0)
				return; // it is known that the pname is valid, but that there are no parameters to return.

			if(nativeType == GL_BOOL)
			{
				GLboolean *boolParams = nullptr;
				boolParams = new GLboolean[numParams];

				context->getBooleanv(pname, boolParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					if(boolParams[i] == GL_FALSE)
						params[i] = 0.0f;
					else
						params[i] = 1.0f;
				}

				delete [] boolParams;
			}
			else if(nativeType == GL_INT)
			{
				GLint *intParams = nullptr;
				intParams = new GLint[numParams];

				context->getIntegerv(pname, intParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					params[i] = (GLfloat)intParams[i];
				}

				delete [] intParams;
			}
		}
	}
}

void APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum pname = 0x%X, GLint* params = %p)",
	      target, attachment, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_EXT && target != GL_READ_FRAMEBUFFER_EXT)
		{
			return error(GL_INVALID_ENUM);
		}

		gl::Framebuffer *framebuffer = nullptr;
		if(target == GL_READ_FRAMEBUFFER_EXT)
		{
			if(context->getReadFramebufferName() == 0)
			{
				return error(GL_INVALID_OPERATION);
			}

			framebuffer = context->getReadFramebuffer();
		}
		else
		{
			if(context->getDrawFramebufferName() == 0)
			{
				return error(GL_INVALID_OPERATION);
			}

			framebuffer = context->getDrawFramebuffer();
		}

		GLenum attachmentType;
		GLuint attachmentHandle;
		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0:
			attachmentType = framebuffer->getColorbufferType();
			attachmentHandle = framebuffer->getColorbufferName();
			break;
		case GL_DEPTH_ATTACHMENT:
			attachmentType = framebuffer->getDepthbufferType();
			attachmentHandle = framebuffer->getDepthbufferName();
			break;
		case GL_STENCIL_ATTACHMENT:
			attachmentType = framebuffer->getStencilbufferType();
			attachmentHandle = framebuffer->getStencilbufferName();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		GLenum attachmentObjectType;   // Type category
		if(attachmentType == GL_NONE || attachmentType == GL_RENDERBUFFER)
		{
			attachmentObjectType = attachmentType;
		}
		else if(gl::IsTextureTarget(attachmentType))
		{
			attachmentObjectType = GL_TEXTURE;
		}
		else UNREACHABLE(attachmentType);

		switch(pname)
		{
		case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
			*params = attachmentObjectType;
			break;
		case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
			if(attachmentObjectType == GL_RENDERBUFFER || attachmentObjectType == GL_TEXTURE)
			{
				*params = attachmentHandle;
			}
			else
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
			if(attachmentObjectType == GL_TEXTURE)
			{
				*params = 0; // FramebufferTexture2D will not allow level to be set to anything else in GL ES 2.0
			}
			else
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
			if(attachmentObjectType == GL_TEXTURE)
			{
				if(gl::IsCubemapTextureTarget(attachmentType))
				{
					*params = attachmentType;
				}
				else
				{
					*params = 0;
				}
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

GLenum APIENTRY glGetGraphicsResetStatusEXT(void)
{
	TRACE("()");

	return GL_NO_ERROR;
}

void APIENTRY glGetIntegerv(GLenum pname, GLint* params)
{
	TRACE("(GLenum pname = 0x%X, GLint* params = %p)", pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(!(context->getIntegerv(pname, params)))
		{
			GLenum nativeType;
			unsigned int numParams = 0;
			if(!context->getQueryParameterInfo(pname, &nativeType, &numParams))
				return error(GL_INVALID_ENUM);

			if(numParams == 0)
				return; // it is known that pname is valid, but there are no parameters to return

			if(nativeType == GL_BOOL)
			{
				GLboolean *boolParams = nullptr;
				boolParams = new GLboolean[numParams];

				context->getBooleanv(pname, boolParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					if(boolParams[i] == GL_FALSE)
						params[i] = 0;
					else
						params[i] = 1;
				}

				delete [] boolParams;
			}
			else if(nativeType == GL_FLOAT)
			{
				GLfloat *floatParams = nullptr;
				floatParams = new GLfloat[numParams];

				context->getFloatv(pname, floatParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					if(pname == GL_DEPTH_RANGE || pname == GL_COLOR_CLEAR_VALUE || pname == GL_DEPTH_CLEAR_VALUE || pname == GL_BLEND_COLOR)
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
		}
	}
}

void APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
	TRACE("(GLuint program = %d, GLenum pname = 0x%X, GLint* params = %p)", program, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_VALUE);
		}

		switch(pname)
		{
		case GL_DELETE_STATUS:
			*params = programObject->isFlaggedForDeletion();
			return;
		case GL_LINK_STATUS:
			*params = programObject->isLinked();
			return;
		case GL_VALIDATE_STATUS:
			*params = programObject->isValidated();
			return;
		case GL_INFO_LOG_LENGTH:
			*params = programObject->getInfoLogLength();
			return;
		case GL_ATTACHED_SHADERS:
			*params = programObject->getAttachedShadersCount();
			return;
		case GL_ACTIVE_ATTRIBUTES:
			*params = programObject->getActiveAttributeCount();
			return;
		case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
			*params = programObject->getActiveAttributeMaxLength();
			return;
		case GL_ACTIVE_UNIFORMS:
			*params = programObject->getActiveUniformCount();
			return;
		case GL_ACTIVE_UNIFORM_MAX_LENGTH:
			*params = programObject->getActiveUniformMaxLength();
			return;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	TRACE("(GLuint program = %d, GLsizei bufsize = %d, GLsizei* length = %p, GLchar* infolog = %p)",
	      program, bufsize, length, infolog);

	if(bufsize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_VALUE);
		}

		programObject->getInfoLog(bufsize, length, infolog);
	}
}

void APIENTRY glGetQueryivEXT(GLenum target, GLenum pname, GLint *params)
{
	TRACE("GLenum target = 0x%X, GLenum pname = 0x%X, GLint *params = %p)", target, pname, params);

	switch(pname)
	{
	case GL_CURRENT_QUERY:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		params[0] = context->getActiveQuery(target);
	}
}

void APIENTRY glGetQueryObjectuivEXT(GLuint name, GLenum pname, GLuint *params)
{
	TRACE("(GLuint name = %d, GLenum pname = 0x%X, GLuint *params = %p)", name, pname, params);

	switch(pname)
	{
	case GL_QUERY_RESULT:
	case GL_QUERY_RESULT_AVAILABLE:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Query *queryObject = context->getQuery(name, false, GL_NONE);

		if(!queryObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(context->getActiveQuery(queryObject->getType()) == name)
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(pname)
		{
		case GL_QUERY_RESULT:
			params[0] = queryObject->getResult();
			break;
		case GL_QUERY_RESULT_AVAILABLE:
			params[0] = queryObject->isResultAvailable();
			break;
		default:
			ASSERT(false);
		}
	}
}

void APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = %p)", target, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(target != GL_RENDERBUFFER)
		{
			return error(GL_INVALID_ENUM);
		}

		if(context->getRenderbufferName() == 0)
		{
			return error(GL_INVALID_OPERATION);
		}

		gl::Renderbuffer *renderbuffer = context->getRenderbuffer(context->getRenderbufferName());

		switch(pname)
		{
		case GL_RENDERBUFFER_WIDTH:           *params = renderbuffer->getWidth();       break;
		case GL_RENDERBUFFER_HEIGHT:          *params = renderbuffer->getHeight();      break;
		case GL_RENDERBUFFER_INTERNAL_FORMAT: *params = renderbuffer->getFormat();      break;
		case GL_RENDERBUFFER_RED_SIZE:        *params = renderbuffer->getRedSize();     break;
		case GL_RENDERBUFFER_GREEN_SIZE:      *params = renderbuffer->getGreenSize();   break;
		case GL_RENDERBUFFER_BLUE_SIZE:       *params = renderbuffer->getBlueSize();    break;
		case GL_RENDERBUFFER_ALPHA_SIZE:      *params = renderbuffer->getAlphaSize();   break;
		case GL_RENDERBUFFER_DEPTH_SIZE:      *params = renderbuffer->getDepthSize();   break;
		case GL_RENDERBUFFER_STENCIL_SIZE:    *params = renderbuffer->getStencilSize(); break;
		case GL_RENDERBUFFER_SAMPLES_EXT:     *params = renderbuffer->getSamples();     break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
	TRACE("(GLuint shader = %d, GLenum pname = %d, GLint* params = %p)", shader, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Shader *shaderObject = context->getShader(shader);

		if(!shaderObject)
		{
			return error(GL_INVALID_VALUE);
		}

		switch(pname)
		{
		case GL_SHADER_TYPE:
			*params = shaderObject->getType();
			return;
		case GL_DELETE_STATUS:
			*params = shaderObject->isFlaggedForDeletion();
			return;
		case GL_COMPILE_STATUS:
			*params = shaderObject->isCompiled() ? GL_TRUE : GL_FALSE;
			return;
		case GL_INFO_LOG_LENGTH:
			*params = shaderObject->getInfoLogLength();
			return;
		case GL_SHADER_SOURCE_LENGTH:
			*params = shaderObject->getSourceLength();
			return;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	TRACE("(GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = %p, GLchar* infolog = %p)",
	      shader, bufsize, length, infolog);

	if(bufsize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Shader *shaderObject = context->getShader(shader);

		if(!shaderObject)
		{
			return error(GL_INVALID_VALUE);
		}

		shaderObject->getInfoLog(bufsize, length, infolog);
	}
}

void APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
	TRACE("(GLenum shadertype = 0x%X, GLenum precisiontype = 0x%X, GLint* range = %p, GLint* precision = %p)",
	      shadertype, precisiontype, range, precision);

	switch(shadertype)
	{
	case GL_VERTEX_SHADER:
	case GL_FRAGMENT_SHADER:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(precisiontype)
	{
	case GL_LOW_FLOAT:
	case GL_MEDIUM_FLOAT:
	case GL_HIGH_FLOAT:
		// IEEE 754 single-precision
		range[0] = 127;
		range[1] = 127;
		*precision = 23;
		break;
	case GL_LOW_INT:
	case GL_MEDIUM_INT:
	case GL_HIGH_INT:
		// Full integer precision is supported
		range[0] = 31;
		range[1] = 30;
		*precision = 0;
		break;
	default:
		return error(GL_INVALID_ENUM);
	}
}

void APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
	TRACE("(GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = %p, GLchar* source = %p)",
	      shader, bufsize, length, source);

	if(bufsize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Shader *shaderObject = context->getShader(shader);

		if(!shaderObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		shaderObject->getSource(bufsize, length, source);
	}
}

const GLubyte* APIENTRY glGetString(GLenum name)
{
	TRACE("(GLenum name = 0x%X)", name);

	switch(name)
	{
	case GL_VENDOR:
		return (GLubyte*)"Google Inc.";
	case GL_RENDERER:
		return (GLubyte*)"SwiftShader";
	case GL_VERSION:
		return (GLubyte*)"2.1 SwiftShader " VERSION_STRING;
	case GL_SHADING_LANGUAGE_VERSION:
		return (GLubyte*)"3.30 SwiftShader " VERSION_STRING;
	case GL_EXTENSIONS:
		// Keep list sorted in following order:
		// OES extensions
		// EXT extensions
		// Vendor extensions
		return (GLubyte*)
			"GL_ARB_framebuffer_object "
			"GL_EXT_blend_minmax "
			"GL_EXT_depth_texture "
			"GL_EXT_depth_texture_cube_map "
			"GL_EXT_element_index_uint "
			"GL_EXT_packed_depth_stencil "
			"GL_EXT_rgb8_rgba8 "
			"GL_EXT_standard_derivatives "
			"GL_EXT_texture_float "
			"GL_EXT_texture_float_linear "
			"GL_EXT_texture_half_float "
			"GL_EXT_texture_half_float_linear "
			"GL_EXT_texture_npot "
			"GL_EXT_occlusion_query_boolean "
			"GL_EXT_read_format_bgra "
			"GL_EXT_texture_compression_dxt1 "
			"GL_EXT_blend_func_separate "
			"GL_EXT_secondary_color "
			"GL_EXT_texture_filter_anisotropic "
			"GL_EXT_texture_format_BGRA8888 "
			"GL_EXT_framebuffer_blit "
			"GL_EXT_framebuffer_multisample "
			"GL_EXT_texture_compression_dxt3 "
			"GL_EXT_texture_compression_dxt5 "
			"GL_NV_fence";
	default:
		return error(GL_INVALID_ENUM, (GLubyte*)nullptr);
	}

	return nullptr;
}

void APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat* params = %p)", target, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D(target);
			break;
		case GL_TEXTURE_CUBE_MAP:
			texture = context->getTextureCubeMap();
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
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = %p)", target, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D(target);
			break;
		case GL_TEXTURE_CUBE_MAP:
			texture = context->getTextureCubeMap();
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
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glGetnUniformfvEXT(GLuint program, GLint location, GLsizei bufSize, GLfloat* params)
{
	TRACE("(GLuint program = %d, GLint location = %d, GLsizei bufSize = %d, GLfloat* params = %p)",
	      program, location, bufSize, params);

	if(bufSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(program == 0)
		{
			return error(GL_INVALID_VALUE);
		}

		gl::Program *programObject = context->getProgram(program);

		if(!programObject || !programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!programObject->getUniformfv(location, &bufSize, params))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
	TRACE("(GLuint program = %d, GLint location = %d, GLfloat* params = %p)", program, location, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(program == 0)
		{
			return error(GL_INVALID_VALUE);
		}

		gl::Program *programObject = context->getProgram(program);

		if(!programObject || !programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!programObject->getUniformfv(location, nullptr, params))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glGetnUniformivEXT(GLuint program, GLint location, GLsizei bufSize, GLint* params)
{
	TRACE("(GLuint program = %d, GLint location = %d, GLsizei bufSize = %d, GLint* params = %p)",
	      program, location, bufSize, params);

	if(bufSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(program == 0)
		{
			return error(GL_INVALID_VALUE);
		}

		gl::Program *programObject = context->getProgram(program);

		if(!programObject || !programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!programObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!programObject->getUniformiv(location, &bufSize, params))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params)
{
	TRACE("(GLuint program = %d, GLint location = %d, GLint* params = %p)", program, location, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(program == 0)
		{
			return error(GL_INVALID_VALUE);
		}

		gl::Program *programObject = context->getProgram(program);

		if(!programObject || !programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!programObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!programObject->getUniformiv(location, nullptr, params))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

int APIENTRY glGetUniformLocation(GLuint program, const GLchar* name)
{
	TRACE("(GLuint program = %d, const GLchar* name = %s)", program, name);

	gl::Context *context = gl::getContext();

	if(strstr(name, "gl_") == name)
	{
		return -1;
	}

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION, -1);
			}
			else
			{
				return error(GL_INVALID_VALUE, -1);
			}
		}

		if(!programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION, -1);
		}

		return programObject->getUniformLocation(name);
	}

	return -1;
}

void APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
	TRACE("(GLuint index = %d, GLenum pname = 0x%X, GLfloat* params = %p)", index, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(index >= gl::MAX_VERTEX_ATTRIBS)
		{
			return error(GL_INVALID_VALUE);
		}

		const gl::VertexAttribute &attribState = context->getVertexAttribState(index);

		switch(pname)
		{
		case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
			*params = (GLfloat)(attribState.mArrayEnabled ? GL_TRUE : GL_FALSE);
			break;
		case GL_VERTEX_ATTRIB_ARRAY_SIZE:
			*params = (GLfloat)attribState.mSize;
			break;
		case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
			*params = (GLfloat)attribState.mStride;
			break;
		case GL_VERTEX_ATTRIB_ARRAY_TYPE:
			*params = (GLfloat)attribState.mType;
			break;
		case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
			*params = (GLfloat)(attribState.mNormalized ? GL_TRUE : GL_FALSE);
			break;
		case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
			*params = (GLfloat)attribState.mBoundBuffer.name();
			break;
		case GL_CURRENT_VERTEX_ATTRIB:
			for(int i = 0; i < 4; ++i)
			{
				params[i] = attribState.mCurrentValue[i];
			}
			break;
		default: return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
	TRACE("(GLuint index = %d, GLenum pname = 0x%X, GLint* params = %p)", index, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(index >= gl::MAX_VERTEX_ATTRIBS)
		{
			return error(GL_INVALID_VALUE);
		}

		const gl::VertexAttribute &attribState = context->getVertexAttribState(index);

		switch(pname)
		{
		case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
			*params = (attribState.mArrayEnabled ? GL_TRUE : GL_FALSE);
			break;
		case GL_VERTEX_ATTRIB_ARRAY_SIZE:
			*params = attribState.mSize;
			break;
		case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
			*params = attribState.mStride;
			break;
		case GL_VERTEX_ATTRIB_ARRAY_TYPE:
			*params = attribState.mType;
			break;
		case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
			*params = (attribState.mNormalized ? GL_TRUE : GL_FALSE);
			break;
		case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
			*params = attribState.mBoundBuffer.name();
			break;
		case GL_CURRENT_VERTEX_ATTRIB:
			for(int i = 0; i < 4; ++i)
			{
				float currentValue = attribState.mCurrentValue[i];
				params[i] = (GLint)(currentValue > 0.0f ? floor(currentValue + 0.5f) : ceil(currentValue - 0.5f));
			}
			break;
		default: return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer)
{
	TRACE("(GLuint index = %d, GLenum pname = 0x%X, GLvoid** pointer = %p)", index, pname, pointer);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(index >= gl::MAX_VERTEX_ATTRIBS)
		{
			return error(GL_INVALID_VALUE);
		}

		if(pname != GL_VERTEX_ATTRIB_ARRAY_POINTER)
		{
			return error(GL_INVALID_ENUM);
		}

		*pointer = const_cast<GLvoid*>(context->getVertexAttribPointer(index));
	}
}

void APIENTRY glHint(GLenum target, GLenum mode)
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

	gl::Context *context = gl::getContext();
	switch(target)
	{
	case GL_GENERATE_MIPMAP_HINT:
		if(context) context->setGenerateMipmapHint(mode);
		break;
	case GL_FRAGMENT_SHADER_DERIVATIVE_HINT:
		if(context) context->setFragmentShaderDerivativeHint(mode);
		break;
	default:
		return error(GL_INVALID_ENUM);
	}
}

GLboolean APIENTRY glIsBuffer(GLuint buffer)
{
	TRACE("(GLuint buffer = %d)", buffer);

	gl::Context *context = gl::getContext();

	if(context && buffer)
	{
		gl::Buffer *bufferObject = context->getBuffer(buffer);

		if(bufferObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean APIENTRY glIsEnabled(GLenum cap)
{
	TRACE("(GLenum cap = 0x%X)", cap);

	gl::Context *context = gl::getContext();

	if(context)
	{
		switch(cap)
		{
		case GL_CULL_FACE:                return context->isCullFaceEnabled();
		case GL_POLYGON_OFFSET_FILL:      return context->isPolygonOffsetFillEnabled();
		case GL_SAMPLE_ALPHA_TO_COVERAGE: return context->isSampleAlphaToCoverageEnabled();
		case GL_SAMPLE_COVERAGE:          return context->isSampleCoverageEnabled();
		case GL_SCISSOR_TEST:             return context->isScissorTestEnabled();
		case GL_STENCIL_TEST:             return context->isStencilTestEnabled();
		case GL_DEPTH_TEST:               return context->isDepthTestEnabled();
		case GL_BLEND:                    return context->isBlendEnabled();
		case GL_DITHER:                   return context->isDitherEnabled();
		case GL_COLOR_LOGIC_OP:           return context->isColorLogicOpEnabled();
		case GL_INDEX_LOGIC_OP:           UNIMPLEMENTED();
		default:
			return error(GL_INVALID_ENUM, false);
		}
	}

	return false;
}

GLboolean APIENTRY glIsFenceNV(GLuint fence)
{
	TRACE("(GLuint fence = %d)", fence);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Fence *fenceObject = context->getFence(fence);

		if(!fenceObject)
		{
			return GL_FALSE;
		}

		return fenceObject->isFence();
	}

	return GL_FALSE;
}

GLboolean APIENTRY glIsFramebuffer(GLuint framebuffer)
{
	TRACE("(GLuint framebuffer = %d)", framebuffer);

	gl::Context *context = gl::getContext();

	if(context && framebuffer)
	{
		gl::Framebuffer *framebufferObject = context->getFramebuffer(framebuffer);

		if(framebufferObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean APIENTRY glIsProgram(GLuint program)
{
	TRACE("(GLuint program = %d)", program);

	gl::Context *context = gl::getContext();

	if(context && program)
	{
		gl::Program *programObject = context->getProgram(program);

		if(programObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean APIENTRY glIsQueryEXT(GLuint name)
{
	TRACE("(GLuint name = %d)", name);

	if(name == 0)
	{
		return GL_FALSE;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Query *queryObject = context->getQuery(name, false, GL_NONE);

		if(queryObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean APIENTRY glIsRenderbuffer(GLuint renderbuffer)
{
	TRACE("(GLuint renderbuffer = %d)", renderbuffer);

	gl::Context *context = gl::getContext();

	if(context && renderbuffer)
	{
		gl::Renderbuffer *renderbufferObject = context->getRenderbuffer(renderbuffer);

		if(renderbufferObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean APIENTRY glIsShader(GLuint shader)
{
	TRACE("(GLuint shader = %d)", shader);

	gl::Context *context = gl::getContext();

	if(context && shader)
	{
		gl::Shader *shaderObject = context->getShader(shader);

		if(shaderObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GLboolean APIENTRY glIsTexture(GLuint texture)
{
	TRACE("(GLuint texture = %d)", texture);

	gl::Context *context = gl::getContext();

	if(context && texture)
	{
		gl::Texture *textureObject = context->getTexture(texture);

		if(textureObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

void APIENTRY glLineWidth(GLfloat width)
{
	TRACE("(GLfloat width = %f)", width);

	if(width <= 0.0f)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setLineWidth(width);
	}
}

void APIENTRY glLinkProgram(GLuint program)
{
	TRACE("(GLuint program = %d)", program);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		programObject->link();
	}
}

void APIENTRY glPixelStorei(GLenum pname, GLint param)
{
	TRACE("(GLenum pname = 0x%X, GLint param = %d)", pname, param);

	gl::Context *context = gl::getContext();

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

void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
	TRACE("(GLfloat factor = %f, GLfloat units = %f)", factor, units);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setPolygonOffsetParams(factor, units);
	}
}

void APIENTRY glReadnPixelsEXT(GLint x, GLint y, GLsizei width, GLsizei height,
							   GLenum format, GLenum type, GLsizei bufSize, GLvoid *data)
{
	TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, "
		  "GLenum format = 0x%X, GLenum type = 0x%X, GLsizei bufSize = 0x%d, GLvoid *data = %p)",
		  x, y, width, height, format, type, bufSize, data);

	if(width < 0 || height < 0 || bufSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(!validReadFormatType(format, type))
	{
		return error(GL_INVALID_OPERATION);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->readPixels(x, y, width, height, format, type, &bufSize, data);
	}
}

void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
	TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, "
	      "GLenum format = 0x%X, GLenum type = 0x%X, GLvoid* pixels = %p)",
	      x, y, width, height, format, type,  pixels);

	if(width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(!validReadFormatType(format, type))
	{
		return error(GL_INVALID_OPERATION);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->readPixels(x, y, width, height, format, type, nullptr, pixels);
	}
}

void APIENTRY glReleaseShaderCompiler(void)
{
	TRACE("()");

	gl::Shader::releaseCompiler();
}

void APIENTRY glRenderbufferStorageMultisampleANGLE(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
	TRACE("(GLenum target = 0x%X, GLsizei samples = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d)",
	      target, samples, internalformat, width, height);

	switch(target)
	{
	case GL_RENDERBUFFER:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(!gl::IsColorRenderable(internalformat) && !gl::IsDepthRenderable(internalformat) && !gl::IsStencilRenderable(internalformat))
	{
		return error(GL_INVALID_ENUM);
	}

	if(width < 0 || height < 0 || samples < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(width > gl::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE ||
		   height > gl::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE ||
		   samples > gl::IMPLEMENTATION_MAX_SAMPLES)
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
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
			context->setRenderbufferStorage(new gl::Depthbuffer(width, height, samples));
			break;
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB565:
		case GL_RGB8_EXT:
		case GL_RGBA8_EXT:
			context->setRenderbufferStorage(new gl::Colorbuffer(width, height, internalformat, samples));
			break;
		case GL_STENCIL_INDEX8:
			context->setRenderbufferStorage(new gl::Stencilbuffer(width, height, samples));
			break;
		case GL_DEPTH24_STENCIL8_EXT:
			context->setRenderbufferStorage(new gl::DepthStencilbuffer(width, height, samples));
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	glRenderbufferStorageMultisampleANGLE(target, 0, internalformat, width, height);
}

void APIENTRY glSampleCoverage(GLclampf value, GLboolean invert)
{
	TRACE("(GLclampf value = %f, GLboolean invert = %d)", value, invert);

	gl::Context* context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setSampleCoverageParams(gl::clamp01(value), invert == GL_TRUE);
	}
}

void APIENTRY glSetFenceNV(GLuint fence, GLenum condition)
{
	TRACE("(GLuint fence = %d, GLenum condition = 0x%X)", fence, condition);

	if(condition != GL_ALL_COMPLETED_NV)
	{
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Fence *fenceObject = context->getFence(fence);

		if(!fenceObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		fenceObject->setFence(condition);
	}
}

void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)", x, y, width, height);

	if(width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context* context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setScissorParams(x, y, width, height);
	}
}

void APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length)
{
	TRACE("(GLsizei n = %d, const GLuint* shaders = %p, GLenum binaryformat = 0x%X, "
	      "const GLvoid* binary = %p, GLsizei length = %d)",
	      n, shaders, binaryformat, binary, length);

	// No binary shader formats are supported.
	return error(GL_INVALID_ENUM);
}

void APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length)
{
	TRACE("(GLuint shader = %d, GLsizei count = %d, const GLchar** string = %p, const GLint* length = %p)",
	      shader, count, string, length);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Shader *shaderObject = context->getShader(shader);

		if(!shaderObject)
		{
			if(context->getProgram(shader))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		shaderObject->setSource(count, string, length);
	}
}

void APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	glStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
}

void APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	TRACE("(GLenum face = 0x%X, GLenum func = 0x%X, GLint ref = %d, GLuint mask = %d)", face, func, ref, mask);

	switch(face)
	{
	case GL_FRONT:
	case GL_BACK:
	case GL_FRONT_AND_BACK:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

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

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
		{
			context->setStencilParams(func, ref, mask);
		}

		if(face == GL_BACK || face == GL_FRONT_AND_BACK)
		{
			context->setStencilBackParams(func, ref, mask);
		}
	}
}

void APIENTRY glStencilMask(GLuint mask)
{
	glStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
}

void APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask)
{
	TRACE("(GLenum face = 0x%X, GLuint mask = %d)", face, mask);

	switch(face)
	{
	case GL_FRONT:
	case GL_BACK:
	case GL_FRONT_AND_BACK:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
		{
			context->setStencilWritemask(mask);
		}

		if(face == GL_BACK || face == GL_FRONT_AND_BACK)
		{
			context->setStencilBackWritemask(mask);
		}
	}
}

void APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	glStencilOpSeparate(GL_FRONT_AND_BACK, fail, zfail, zpass);
}

void APIENTRY glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
	TRACE("(GLenum face = 0x%X, GLenum fail = 0x%X, GLenum zfail = 0x%X, GLenum zpas = 0x%Xs)",
	      face, fail, zfail, zpass);

	switch(face)
	{
	case GL_FRONT:
	case GL_BACK:
	case GL_FRONT_AND_BACK:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	switch(fail)
	{
	case GL_ZERO:
	case GL_KEEP:
	case GL_REPLACE:
	case GL_INCR:
	case GL_DECR:
	case GL_INVERT:
	case GL_INCR_WRAP:
	case GL_DECR_WRAP:
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
	case GL_INCR_WRAP:
	case GL_DECR_WRAP:
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
	case GL_INCR_WRAP:
	case GL_DECR_WRAP:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
		{
			context->setStencilOperations(fail, zfail, zpass);
		}

		if(face == GL_BACK || face == GL_FRONT_AND_BACK)
		{
			context->setStencilBackOperations(fail, zfail, zpass);
		}
	}
}

GLboolean APIENTRY glTestFenceNV(GLuint fence)
{
	TRACE("(GLuint fence = %d)", fence);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Fence *fenceObject = context->getFence(fence);

		if(!fenceObject)
		{
			return error(GL_INVALID_OPERATION, GL_TRUE);
		}

		return fenceObject->testFence();
	}

	return GL_TRUE;
}

void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
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
		//TRACE("UNIMPLEMENTED!!");
		//return error(GL_INVALID_OPERATION);
	}

	switch(format)
	{
	case GL_ALPHA:
	case GL_LUMINANCE:
	case GL_LUMINANCE_ALPHA:
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		case GL_FLOAT:
		case GL_HALF_FLOAT:
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
		case GL_FLOAT:
		case GL_HALF_FLOAT:
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
		case GL_FLOAT:
		case GL_HALF_FLOAT:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
		break;
	case GL_BGRA_EXT:
		switch(type)
		{
		case GL_UNSIGNED_BYTE:
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_INT_8_8_8_8_REV:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
		break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:  // error cases for compressed textures are handled below
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		break;
	case GL_DEPTH_COMPONENT:
		switch(type)
		{
		case GL_UNSIGNED_SHORT:
		case GL_UNSIGNED_INT:
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
		break;
	case GL_DEPTH_STENCIL_EXT:
		switch(type)
		{
		case GL_UNSIGNED_INT_24_8_EXT:
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

	switch(target)
	{
	case GL_TEXTURE_2D:
		if(width > (gl::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
		   height > (gl::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
		{
			return error(GL_INVALID_VALUE);
		}
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		if(width != height)
		{
			return error(GL_INVALID_VALUE);
		}

		if(width > (gl::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level) ||
		   height > (gl::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level))
		{
			return error(GL_INVALID_VALUE);
		}
		break;
	case GL_PROXY_TEXTURE_2D:
		pixels = 0;

		if(width > (gl::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
		   height > (gl::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
		{
			//UNIMPLEMENTED();
			width = 0;
			height = 0;
			internalformat = GL_NONE;
			format = GL_NONE;
			type = GL_NONE;

			//return;// error(GL_INVALID_VALUE);
		}
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
	   format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
	   format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
	   format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		return error(GL_INVALID_OPERATION);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(target == GL_TEXTURE_2D || target == GL_PROXY_TEXTURE_2D)
		{
			gl::Texture2D *texture = context->getTexture2D(target);

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			texture->setImage(level, width, height, format, type, context->getUnpackAlignment(), pixels);
		}
		else
		{
			gl::TextureCubeMap *texture = context->getTextureCubeMap();

			if(!texture)
			{
				return error(GL_INVALID_OPERATION);
			}

			texture->setImage(target, level, width, height, format, type, context->getUnpackAlignment(), pixels);
		}
	}
}

void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat param = %f)", target, pname, param);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D(target);
			break;
		case GL_TEXTURE_CUBE_MAP:
			texture = context->getTextureCubeMap();
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
		case GL_TEXTURE_MIN_LOD:
			//TRACE("() UNIMPLEMENTED!!");   // FIXME
			//UNIMPLEMENTED();
			break;
		case GL_TEXTURE_MAX_LOD:
			//TRACE("() UNIMPLEMENTED!!");   // FIXME
			//UNIMPLEMENTED();
			break;
		case GL_TEXTURE_LOD_BIAS:
			if(param != 0.0f)
			{
				UNIMPLEMENTED();
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
	glTexParameterf(target, pname, *params);
}

void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint param = %d)", target, pname, param);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D(target);
			break;
		case GL_TEXTURE_CUBE_MAP:
			texture = context->getTextureCubeMap();
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
		case GL_TEXTURE_MAX_LEVEL:
			if(!texture->setMaxLevel(param))
			{
				return error(GL_INVALID_ENUM);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
	glTexParameteri(target, pname, *params);
}

void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                              GLenum format, GLenum type, const GLvoid* pixels)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
	      "GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, GLenum type = 0x%X, "
	      "const GLvoid* pixels = %p)",
	      target, level, xoffset, yoffset, width, height, format, type, pixels);

	if(!gl::IsTextureTarget(target))
	{
		return error(GL_INVALID_ENUM);
	}

	if(level < 0 || xoffset < 0 || yoffset < 0 || width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(std::numeric_limits<GLsizei>::max() - xoffset < width || std::numeric_limits<GLsizei>::max() - yoffset < height)
	{
		return error(GL_INVALID_VALUE);
	}

	if(!gl::CheckTextureFormatType(format, type))
	{
		return error(GL_INVALID_ENUM);
	}

	if(width == 0 || height == 0 || !pixels)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(level > gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
		{
			return error(GL_INVALID_VALUE);
		}

		if(target == GL_TEXTURE_2D)
		{
			gl::Texture2D *texture = context->getTexture2D(target);

			if(validateSubImageParams(false, width, height, xoffset, yoffset, target, level, format, texture))
			{
				texture->subImage(level, xoffset, yoffset, width, height, format, type, context->getUnpackAlignment(), pixels);
			}
		}
		else if(gl::IsCubemapTextureTarget(target))
		{
			gl::TextureCubeMap *texture = context->getTextureCubeMap();

			if(validateSubImageParams(false, width, height, xoffset, yoffset, target, level, format, texture))
			{
				texture->subImage(target, level, xoffset, yoffset, width, height, format, type, context->getUnpackAlignment(), pixels);
			}
		}
		else UNREACHABLE(target);
	}
}

void APIENTRY glUniform1f(GLint location, GLfloat x)
{
	glUniform1fv(location, 1, &x);
}

void APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLfloat* v = %p)", location, count, v);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniform1fv(location, count, v))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniform1i(GLint location, GLint x)
{
	glUniform1iv(location, 1, &x);
}

void APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLint* v = %p)", location, count, v);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniform1iv(location, count, v))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniform2f(GLint location, GLfloat x, GLfloat y)
{
	GLfloat xy[2] = {x, y};

	glUniform2fv(location, 1, (GLfloat*)&xy);
}

void APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLfloat* v = %p)", location, count, v);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniform2fv(location, count, v))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniform2i(GLint location, GLint x, GLint y)
{
	GLint xy[4] = {x, y};

	glUniform2iv(location, 1, (GLint*)&xy);
}

void APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLint* v = %p)", location, count, v);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniform2iv(location, count, v))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
	GLfloat xyz[3] = {x, y, z};

	glUniform3fv(location, 1, (GLfloat*)&xyz);
}

void APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLfloat* v = %p)", location, count, v);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniform3fv(location, count, v))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
	GLint xyz[3] = {x, y, z};

	glUniform3iv(location, 1, (GLint*)&xyz);
}

void APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLint* v = %p)", location, count, v);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniform3iv(location, count, v))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	GLfloat xyzw[4] = {x, y, z, w};

	glUniform4fv(location, 1, (GLfloat*)&xyzw);
}

void APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLfloat* v = %p)", location, count, v);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniform4fv(location, count, v))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
	GLint xyzw[4] = {x, y, z, w};

	glUniform4iv(location, 1, (GLint*)&xyzw);
}

void APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLint* v = %p)", location, count, v);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniform4iv(location, count, v))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = %p)",
	      location, count, transpose, value);

	if(count < 0 || transpose != GL_FALSE)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniformMatrix2fv(location, count, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = %p)",
	      location, count, transpose, value);

	if(count < 0 || transpose != GL_FALSE)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniformMatrix3fv(location, count, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = %p)",
	      location, count, transpose, value);

	if(count < 0 || transpose != GL_FALSE)
	{
		return error(GL_INVALID_VALUE);
	}

	if(location == -1)
	{
		return;
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!program->setUniformMatrix4fv(location, count, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

void APIENTRY glUseProgram(GLuint program)
{
	TRACE("(GLuint program = %d)", program);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Program *programObject = context->getProgram(program);

		if(!programObject && program != 0)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		if(program != 0 && !programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION);
		}

		context->useProgram(program);
	}
}

void APIENTRY glValidateProgram(GLuint program)
{
	TRACE("(GLuint program = %d)", program);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION);
			}
			else
			{
				return error(GL_INVALID_VALUE);
			}
		}

		programObject->validate();
	}
}

void APIENTRY glVertexAttrib1f(GLuint index, GLfloat x)
{
	TRACE("(GLuint index = %d, GLfloat x = %f)", index, x);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//GLfloat vals[4] = { x, 0, 0, 1 };
		context->setVertexAttrib(index, x, 0, 0, 1);
	}
}

void APIENTRY glVertexAttrib1fv(GLuint index, const GLfloat* values)
{
	TRACE("(GLuint index = %d, const GLfloat* values = %p)", index, values);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//GLfloat vals[4] = { values[0], 0, 0, 1 };
		context->setVertexAttrib(index, values[0], 0, 0, 1);
	}
}

void APIENTRY glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
	TRACE("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f)", index, x, y);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//GLfloat vals[4] = { x, y, 0, 1 };
		context->setVertexAttrib(index, x, y, 0, 1);
	}
}

void APIENTRY glVertexAttrib2fv(GLuint index, const GLfloat* values)
{
	TRACE("(GLuint index = %d, const GLfloat* values = %p)", index, values);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//GLfloat vals[4] = {  };
		context->setVertexAttrib(index, values[0], values[1], 0, 1);
	}
}

void APIENTRY glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
	TRACE("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", index, x, y, z);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//GLfloat vals[4] = { x, y, z, 1 };
		context->setVertexAttrib(index, x, y, z, 1);
	}
}

void APIENTRY glVertexAttrib3fv(GLuint index, const GLfloat* values)
{
	TRACE("(GLuint index = %d, const GLfloat* values = %p)", index, values);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//GLfloat vals[4] = { values[0], values[1], values[2], 1 };
		context->setVertexAttrib(index, values[0], values[1], values[2], 1);
	}
}

void APIENTRY glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	TRACE("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f, GLfloat w = %f)", index, x, y, z, w);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//GLfloat vals[4] = { x, y, z, w };
		context->setVertexAttrib(index, x, y, z, w);
	}
}

void APIENTRY glVertexAttrib4fv(GLuint index, const GLfloat* values)
{
	TRACE("(GLuint index = %d, const GLfloat* values = %p)", index, values);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setVertexAttrib(index, values[0], values[1], values[2], values[3]);
	}
}

void APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	TRACE("(GLuint index = %d, GLint size = %d, GLenum type = 0x%X, "
	      "GLboolean normalized = %d, GLsizei stride = %d, const GLvoid* ptr = %p)",
	      index, size, type, normalized, stride, ptr);

	if(index >= gl::MAX_VERTEX_ATTRIBS)
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

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->setVertexAttribState(index, context->getArrayBuffer(), size, type, (normalized == GL_TRUE), stride, ptr);
	}
}

void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)", x, y, width, height);

	if(width < 0 || height < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setViewportParams(x, y, width, height);
	}
}

void APIENTRY glBlitFramebufferANGLE(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                     GLbitfield mask, GLenum filter)
{
	TRACE("(GLint srcX0 = %d, GLint srcY0 = %d, GLint srcX1 = %d, GLint srcY1 = %d, "
	      "GLint dstX0 = %d, GLint dstY0 = %d, GLint dstX1 = %d, GLint dstY1 = %d, "
	      "GLbitfield mask = 0x%X, GLenum filter = 0x%X)",
	      srcX0, srcY0, srcX1, srcX1, dstX0, dstY0, dstX1, dstY1, mask, filter);

	switch(filter)
	{
	case GL_NEAREST:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if((mask & ~(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(srcX1 - srcX0 != dstX1 - dstX0 || srcY1 - srcY0 != dstY1 - dstY0)
	{
		ERR("Scaling and flipping in BlitFramebufferANGLE not supported by this implementation");
		return error(GL_INVALID_OPERATION);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		if(context->getReadFramebufferName() == context->getDrawFramebufferName())
		{
			ERR("Blits with the same source and destination framebuffer are not supported by this implementation.");
			return error(GL_INVALID_OPERATION);
		}

		context->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask);
	}
}

void APIENTRY glTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth,
                              GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, "
	      "GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, GLint border = %d, "
	      "GLenum format = 0x%X, GLenum type = 0x%x, const GLvoid* pixels = %p)",
	      target, level, internalformat, width, height, depth, border, format, type, pixels);

	UNIMPLEMENTED();   // FIXME
}

void WINAPI GlmfBeginGlsBlock()
{
	UNIMPLEMENTED();
}

void WINAPI GlmfCloseMetaFile()
{
	UNIMPLEMENTED();
}

void WINAPI GlmfEndGlsBlock()
{
	UNIMPLEMENTED();
}

void WINAPI GlmfEndPlayback()
{
	UNIMPLEMENTED();
}

void WINAPI GlmfInitPlayback()
{
	UNIMPLEMENTED();
}

void WINAPI GlmfPlayGlsRecord()
{
	UNIMPLEMENTED();
}

void APIENTRY glAccum(GLenum op, GLfloat value)
{
	UNIMPLEMENTED();
}

void APIENTRY glAlphaFunc(GLenum func, GLclampf ref)
{
	TRACE("(GLenum func = 0x%X, GLclampf ref = %f)", func, ref);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->alphaFunc(func, ref);
	}
}

GLboolean APIENTRY glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
	UNIMPLEMENTED();
	return GL_FALSE;
}

void APIENTRY glArrayElement(GLint i)
{
	UNIMPLEMENTED();
}

void APIENTRY glBegin(GLenum mode)
{
	TRACE("(GLenum mode = 0x%X)", mode);

	switch(mode)
	{
	case GL_POINTS:
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_QUADS:
	case GL_QUAD_STRIP:
	case GL_POLYGON:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->begin(mode);
	}
}

void APIENTRY glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
	UNIMPLEMENTED();
}

void APIENTRY glCallList(GLuint list)
{
	TRACE("(GLuint list = %d)", list);

	if(list == 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->callList(list);
	}
}

void APIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
	TRACE("(GLsizei n = %d, GLenum type = 0x%X, const GLvoid *lists)", n, type);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		for(int i = 0; i < n; i++)
		{
			switch(type)
			{
			case GL_UNSIGNED_INT: context->callList(((unsigned int*)lists)[i]); break;
			default:
				UNIMPLEMENTED();
				UNREACHABLE(type);
			}
		}
	}
}

void APIENTRY glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	UNIMPLEMENTED();
}

void APIENTRY glClearDepth(GLclampd depth)
{
	TRACE("(GLclampd depth = %d)", depth);

	glClearDepthf((float)depth);   // FIXME
}

void APIENTRY glClearIndex(GLfloat c)
{
	UNIMPLEMENTED();
}

void APIENTRY glClipPlane(GLenum plane, const GLdouble *equation)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3bv(const GLbyte *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
	TRACE("(GLfloat red = %f, GLfloat green = %f, GLfloat blue = %f)", red, green, blue);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//context->color(red, green, blue, 1.0f);
		//GLfloat vals[4] = {};
		context->setVertexAttrib(sw::Color0, red, green, blue, 1);
	}
}

void APIENTRY glColor3fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3i(GLint red, GLint green, GLint blue)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3s(GLshort red, GLshort green, GLshort blue)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3ubv(const GLubyte *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3ui(GLuint red, GLuint green, GLuint blue)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3uiv(const GLuint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3us(GLushort red, GLushort green, GLushort blue)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor3usv(const GLushort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4bv(const GLbyte *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	TRACE("(GLfloat red = %f, GLfloat green = %f, GLfloat blue = %f, GLfloat alpha = %f)", red, green, blue, alpha);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//context->color(red, green, blue, alpha);
		//GLfloat vals[4] = {red, green, blue, alpha};
		context->setVertexAttrib(sw::Color0, red, green, blue, alpha);
	}
}

void APIENTRY glColor4fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4ubv(const GLubyte *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4uiv(const GLuint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
	UNIMPLEMENTED();
}

void APIENTRY glColor4usv(const GLushort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glColorMaterial(GLenum face, GLenum mode)
{
	TRACE("(GLenum face = 0x%X, GLenum mode = 0x%X)", face, mode);

	// FIXME: Validate enums

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		switch(face)
		{
		case GL_FRONT:
			context->setColorMaterialMode(mode);   // FIXME: Front only
			break;
		case GL_FRONT_AND_BACK:
			context->setColorMaterialMode(mode);
			break;
		default:
			UNIMPLEMENTED();
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(*)");

	glVertexAttribPointer(sw::Color0, size, type, true, stride, pointer);
}

void APIENTRY glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
	UNIMPLEMENTED();
}

void APIENTRY glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
	UNIMPLEMENTED();
}

void APIENTRY glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
	UNIMPLEMENTED();
}

void APIENTRY glDebugEntry()
{
	UNIMPLEMENTED();
}

void APIENTRY glDeleteLists(GLuint list, GLsizei range)
{
	TRACE("(GLuint list = %d, GLsizei range = %d)", list, range);

	if(range < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		for(GLuint i = list; i < list + range; i++)
		{
			context->deleteList(i);
		}
	}
}

void APIENTRY glDepthRange(GLclampd zNear, GLclampd zFar)
{
	UNIMPLEMENTED();
}

void APIENTRY glDisableClientState(GLenum array)
{
	TRACE("(GLenum array = 0x%X)", array);

	gl::Context *context = gl::getContext();

	if(context)
	{
		GLenum texture = context->getClientActiveTexture();

		switch(array)
		{
		case GL_VERTEX_ARRAY:        context->setVertexAttribArrayEnabled(sw::Position, false);                            break;
		case GL_COLOR_ARRAY:         context->setVertexAttribArrayEnabled(sw::Color0, false);                              break;
		case GL_TEXTURE_COORD_ARRAY: context->setVertexAttribArrayEnabled(sw::TexCoord0 + (texture - GL_TEXTURE0), false); break;
		case GL_NORMAL_ARRAY:        context->setVertexAttribArrayEnabled(sw::Normal, false);                              break;
		default:                     UNIMPLEMENTED();
		}
	}
}

void APIENTRY glDrawBuffer(GLenum mode)
{
	UNIMPLEMENTED();
}

void APIENTRY glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	UNIMPLEMENTED();
}

void APIENTRY glEdgeFlag(GLboolean flag)
{
	UNIMPLEMENTED();
}

void APIENTRY glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer)
{
	UNIMPLEMENTED();
}

void APIENTRY glEdgeFlagv(const GLboolean *flag)
{
	UNIMPLEMENTED();
}

void APIENTRY glEnableClientState(GLenum array)
{
	TRACE("(GLenum array = 0x%X)", array);

	gl::Context *context = gl::getContext();

	if(context)
	{
		GLenum texture = context->getClientActiveTexture();

		switch(array)
		{
		case GL_VERTEX_ARRAY:        context->setVertexAttribArrayEnabled(sw::Position, true);                            break;
		case GL_COLOR_ARRAY:         context->setVertexAttribArrayEnabled(sw::Color0, true);                              break;
		case GL_TEXTURE_COORD_ARRAY: context->setVertexAttribArrayEnabled(sw::TexCoord0 + (texture - GL_TEXTURE0), true); break;
		case GL_NORMAL_ARRAY:        context->setVertexAttribArrayEnabled(sw::Normal, true);                              break;
		default:                     UNIMPLEMENTED();
		}
	}
}

void APIENTRY glEnd()
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->end();
	}
}

void APIENTRY glEndList()
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->endList();
	}
}

void APIENTRY glEvalCoord1d(GLdouble u)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalCoord1dv(const GLdouble *u)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalCoord1f(GLfloat u)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalCoord1fv(const GLfloat *u)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalCoord2d(GLdouble u, GLdouble v)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalCoord2dv(const GLdouble *u)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalCoord2f(GLfloat u, GLfloat v)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalCoord2fv(const GLfloat *u)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalPoint1(GLint i)
{
	UNIMPLEMENTED();
}

void APIENTRY glEvalPoint2(GLint i, GLint j)
{
	UNIMPLEMENTED();
}

void APIENTRY glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
	UNIMPLEMENTED();
}

void APIENTRY glFogf(GLenum pname, GLfloat param)
{
	TRACE("(GLenum pname = 0x%X, GLfloat param = %f)", pname, param);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Device *device = gl::getDevice();   // FIXME

		switch(pname)
		{
		case GL_FOG_START: device->setFogStart(param); break;
		case GL_FOG_END:   device->setFogEnd(param);   break;
		default:
			UNIMPLEMENTED();
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glFogfv(GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum pname = 0x%X, const GLfloat *params)", pname);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		switch(pname)
		{
		case GL_FOG_COLOR:
			{
				gl::Device *device = gl::getDevice();   // FIXME
				device->setFogColor(sw::Color<float>(params[0], params[1], params[2], params[3]));
			}
			break;
		default:
			UNIMPLEMENTED();
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glFogi(GLenum pname, GLint param)
{
	TRACE("(GLenum pname = 0x%X, GLint param = %d)", pname, param);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		switch(pname)
		{
		case GL_FOG_MODE:
			{
				gl::Device *device = gl::getDevice();   // FIXME
				switch(param)
				{
				case GL_LINEAR: device->setVertexFogMode(sw::FOG_LINEAR); break;
				default:
					UNIMPLEMENTED();
					return error(GL_INVALID_ENUM);
				}
			}
			break;
		default:
			UNIMPLEMENTED();
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glFogiv(GLenum pname, const GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	TRACE("(GLdouble left = %f, GLdouble right = %f, GLdouble bottom = %f, GLdouble top = %f, GLdouble zNear = %f, GLdouble zFar = %f)", left, right, bottom, top, zNear, zFar);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->frustum(left, right, bottom, top, zNear, zFar);
	}
}

GLuint APIENTRY glGenLists(GLsizei range)
{
	TRACE("(GLsizei range = %d)", range);

	if(range < 0)
	{
		return error(GL_INVALID_VALUE, 0);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		return context->genLists(range);
	}

	return 0;
}

void APIENTRY glGetClipPlane(GLenum plane, GLdouble *equation)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetDoublev(GLenum pname, GLdouble *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetMapiv(GLenum target, GLenum query, GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetPixelMapfv(GLenum map, GLfloat *values)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetPixelMapuiv(GLenum map, GLuint *values)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetPixelMapusv(GLenum map, GLushort *values)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetPointerv(GLenum pname, GLvoid* *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetPolygonStipple(GLubyte *mask)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum format = 0x%X, GLenum type = 0x%X, GLint *pixels%p)", target, level, format, type, pixels);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
			texture = context->getTexture2D(target);
			break;
		case GL_TEXTURE_CUBE_MAP:
			texture = context->getTextureCubeMap();
			break;
		default:
			UNIMPLEMENTED();
			return error(GL_INVALID_ENUM);
		}

		if(format == texture->getFormat(target, level) && type == texture->getType(target, level))
		{
			gl::Image *image = texture->getRenderTarget(target, level);
			void *source = image->lock(0, 0, sw::LOCK_READONLY);
			memcpy(pixels, source, image->getPitch() * image->getHeight());
			image->unlock();
		}
		else
		{
			UNIMPLEMENTED();
		}
	}
}

void APIENTRY glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum pname = 0x%X, GLint *params = %p)", target, level, pname, params);

	gl::Context *context = gl::getContext();

	if(context)
	{
		gl::Texture *texture;

		switch(target)
		{
		case GL_TEXTURE_2D:
		case GL_PROXY_TEXTURE_2D:
			texture = context->getTexture2D(target);
			break;
		case GL_TEXTURE_CUBE_MAP:
			texture = context->getTextureCubeMap();
			break;
		default:
			UNIMPLEMENTED();
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
		case GL_TEXTURE_WIDTH:
			*params = texture->getWidth(target, level);
			break;
		case GL_TEXTURE_HEIGHT:
			*params = texture->getHeight(target, level);
			break;
		case GL_TEXTURE_INTERNAL_FORMAT:
				*params = texture->getInternalFormat(target, level);
			break;
		case GL_TEXTURE_BORDER_COLOR:
			UNIMPLEMENTED();
			break;
		case GL_TEXTURE_BORDER:
			UNIMPLEMENTED();
			break;
		case GL_TEXTURE_MAX_ANISOTROPY_EXT:
			*params = (GLint)texture->getMaxAnisotropy();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glIndexMask(GLuint mask)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexd(GLdouble c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexdv(const GLdouble *c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexf(GLfloat c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexfv(const GLfloat *c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexi(GLint c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexiv(const GLint *c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexs(GLshort c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexsv(const GLshort *c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexub(GLubyte c)
{
	UNIMPLEMENTED();
}

void APIENTRY glIndexubv(const GLubyte *c)
{
	UNIMPLEMENTED();
}

void APIENTRY glInitNames(void)
{
	UNIMPLEMENTED();
}

void APIENTRY glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
	UNIMPLEMENTED();
}

GLboolean APIENTRY glIsList(GLuint list)
{
	UNIMPLEMENTED();
	return GL_FALSE;
}

void APIENTRY glLightModelf(GLenum pname, GLfloat param)
{
	UNIMPLEMENTED();
}

void APIENTRY glLightModelfv(GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum pname = 0x%X, const GLint *params)", pname);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Device *device = gl::getDevice();   // FIXME

		switch(pname)
		{
		case GL_LIGHT_MODEL_AMBIENT:
			device->setGlobalAmbient(sw::Color<float>(params[0], params[1], params[2], params[3]));
			break;
		default:
			UNIMPLEMENTED();
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glLightModeli(GLenum pname, GLint param)
{
	UNIMPLEMENTED();
}

void APIENTRY glLightModeliv(GLenum pname, const GLint *params)
{
	TRACE("(GLenum pname = 0x%X, const GLint *params)", pname);
	UNIMPLEMENTED();
}

void APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param)
{
	UNIMPLEMENTED();
}

void APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	TRACE("(GLenum light = 0x%X, GLenum pname = 0x%X, const GLint *params)", light, pname);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		gl::Device *device = gl::getDevice();   // FIXME

		switch(pname)
		{
		case GL_AMBIENT:  device->setLightAmbient(light - GL_LIGHT0, sw::Color<float>(params[0], params[1], params[2], params[3]));  break;
		case GL_DIFFUSE:  device->setLightDiffuse(light - GL_LIGHT0, sw::Color<float>(params[0], params[1], params[2], params[3]));  break;
		case GL_SPECULAR: device->setLightSpecular(light - GL_LIGHT0, sw::Color<float>(params[0], params[1], params[2], params[3])); break;
		case GL_POSITION:
			if(params[3] == 0.0f)   // Directional light
			{
				// Create a very far out point light
				float max = sw::max(abs(params[0]), abs(params[1]), abs(params[2]));
				device->setLightPosition(light - GL_LIGHT0, sw::Point(params[0] / max * 1e10f, params[1] / max * 1e10f, params[2] / max * 1e10f));
			}
			else
			{
				device->setLightPosition(light - GL_LIGHT0, sw::Point(params[0] / params[3], params[1] / params[3], params[2] / params[3]));
			}
			break;
		default:
			UNIMPLEMENTED();
			return error(GL_INVALID_ENUM);
		}
	}
}

void APIENTRY glLighti(GLenum light, GLenum pname, GLint param)
{
	UNIMPLEMENTED();
}

void APIENTRY glLightiv(GLenum light, GLenum pname, const GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glLineStipple(GLint factor, GLushort pattern)
{
	UNIMPLEMENTED();
}

void APIENTRY glListBase(GLuint base)
{
	UNIMPLEMENTED();
}

void APIENTRY glLoadIdentity()
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->loadIdentity();
	}
}

void APIENTRY glLoadMatrixd(const GLdouble *m)
{
	UNIMPLEMENTED();
}

void APIENTRY glLoadMatrixf(const GLfloat *m)
{
	UNIMPLEMENTED();
}

void APIENTRY glLoadName(GLuint name)
{
	UNIMPLEMENTED();
}

void APIENTRY glLogicOp(GLenum opcode)
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

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setLogicalOperation(opcode);
	}
}

void APIENTRY glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
	UNIMPLEMENTED();
}

void APIENTRY glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
	UNIMPLEMENTED();
}

void APIENTRY glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
	UNIMPLEMENTED();
}

void APIENTRY glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
	UNIMPLEMENTED();
}

void APIENTRY glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
	UNIMPLEMENTED();
}

void APIENTRY glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
	UNIMPLEMENTED();
}

void APIENTRY glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
	UNIMPLEMENTED();
}

void APIENTRY glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
	UNIMPLEMENTED();
}

void APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	UNIMPLEMENTED();
}

void APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glMateriali(GLenum face, GLenum pname, GLint param)
{
	UNIMPLEMENTED();
}

void APIENTRY glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glMatrixMode(GLenum mode)
{
	TRACE("(*)");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setMatrixMode(mode);
	}
}

void APIENTRY glMultMatrixd(const GLdouble *m)
{
	TRACE("(*)");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->multiply(m);
	}
}

void APIENTRY glMultMatrixm(sw::Matrix m)
{
	gl::Context *context = gl::getContext();

	if(context)
	{
		context->multiply((GLfloat*)m.m);
	}
}

void APIENTRY glMultMatrixf(const GLfloat *m)
{
	TRACE("(*)");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			return context->listCommand(gl::newCommand(glMultMatrixm, sw::Matrix(m)));
		}

		context->multiply(m);
	}
}

void APIENTRY glNewList(GLuint list, GLenum mode)
{
	TRACE("(GLuint list = %d, GLenum mode = 0x%X)", list, mode);

	if(list == 0)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(mode)
	{
	case GL_COMPILE:
	case GL_COMPILE_AND_EXECUTE:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->newList(list, mode);
	}
}

void APIENTRY glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormal3bv(const GLbyte *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormal3dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	TRACE("(GLfloat nx = %f, GLfloat ny = %f, GLfloat nz = %f)", nx, ny, nz);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//context->normal(nx, ny, nz);
		context->setVertexAttrib(sw::Normal, nx, ny, nz, 0);
	}
}

void APIENTRY glNormal3fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormal3i(GLint nx, GLint ny, GLint nz)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormal3iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormal3sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(*)");

	glVertexAttribPointer(sw::Normal, 3, type, true, stride, pointer);
}

void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	TRACE("(*)");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->ortho(left, right, bottom, top, zNear, zFar);
	}
}

void APIENTRY glPassThrough(GLfloat token)
{
	UNIMPLEMENTED();
}

void APIENTRY glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values)
{
	UNIMPLEMENTED();
}

void APIENTRY glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values)
{
	UNIMPLEMENTED();
}

void APIENTRY glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values)
{
	UNIMPLEMENTED();
}

void APIENTRY glPixelStoref(GLenum pname, GLfloat param)
{
	UNIMPLEMENTED();
}

void APIENTRY glPixelTransferf(GLenum pname, GLfloat param)
{
	UNIMPLEMENTED();
}

void APIENTRY glPixelTransferi(GLenum pname, GLint param)
{
	UNIMPLEMENTED();
}

void APIENTRY glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
	UNIMPLEMENTED();
}

void APIENTRY glPointSize(GLfloat size)
{
	UNIMPLEMENTED();
}

void APIENTRY glPolygonMode(GLenum face, GLenum mode)
{
	UNIMPLEMENTED();
}

void APIENTRY glPolygonStipple(const GLubyte *mask)
{
	UNIMPLEMENTED();
}

void APIENTRY glPopAttrib(void)
{
	UNIMPLEMENTED();
}

void APIENTRY glPopClientAttrib(void)
{
	UNIMPLEMENTED();
}

void APIENTRY glPopMatrix(void)
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			return context->listCommand(gl::newCommand(glPopMatrix));
		}

		context->popMatrix();
	}
}

void APIENTRY glPopName(void)
{
	UNIMPLEMENTED();
}

void APIENTRY glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
	UNIMPLEMENTED();
}

void APIENTRY glPushAttrib(GLbitfield mask)
{
	UNIMPLEMENTED();
}

void APIENTRY glPushClientAttrib(GLbitfield mask)
{
	UNIMPLEMENTED();
}

void APIENTRY glPushMatrix(void)
{
	TRACE("()");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			return context->listCommand(gl::newCommand(glPushMatrix));
		}

		context->pushMatrix();
	}
}

void APIENTRY glPushName(GLuint name)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos2d(GLdouble x, GLdouble y)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos2dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos2f(GLfloat x, GLfloat y)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos2fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos2i(GLint x, GLint y)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos2iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos2s(GLshort x, GLshort y)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos2sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos3dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos3fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos3i(GLint x, GLint y, GLint z)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos3iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos3sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos4dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos4fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos4iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	UNIMPLEMENTED();
}

void APIENTRY glRasterPos4sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glReadBuffer(GLenum mode)
{
	UNIMPLEMENTED();
}

void APIENTRY glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
	UNIMPLEMENTED();
}

void APIENTRY glRectdv(const GLdouble *v1, const GLdouble *v2)
{
	UNIMPLEMENTED();
}

void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	UNIMPLEMENTED();
}

void APIENTRY glRectfv(const GLfloat *v1, const GLfloat *v2)
{
	UNIMPLEMENTED();
}

void APIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
	UNIMPLEMENTED();
}

void APIENTRY glRectiv(const GLint *v1, const GLint *v2)
{
	UNIMPLEMENTED();
}

void APIENTRY glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
	UNIMPLEMENTED();
}

void APIENTRY glRectsv(const GLshort *v1, const GLshort *v2)
{
	UNIMPLEMENTED();
}

GLint APIENTRY glRenderMode(GLenum mode)
{
	UNIMPLEMENTED();
	return 0;
}

void APIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	UNIMPLEMENTED();
}

void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	TRACE("(*)");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->rotate(angle, x, y, z);
	}
}

void APIENTRY glScaled(GLdouble x, GLdouble y, GLdouble z)
{
	UNIMPLEMENTED();
}

void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	TRACE("(GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", x, y, z);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			return context->listCommand(gl::newCommand(glScalef, x, y, z));
		}

		context->scale(x, y, z);
	}
}

void APIENTRY glSelectBuffer(GLsizei size, GLuint *buffer)
{
	UNIMPLEMENTED();
}

void APIENTRY glShadeModel(GLenum mode)
{
	TRACE("(*)");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->setShadeModel(mode);
	}
}

void APIENTRY glTexCoord1d(GLdouble s)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord1dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord1f(GLfloat s)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord1fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord1i(GLint s)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord1iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord1s(GLshort s)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord1sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord2d(GLdouble s, GLdouble t)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord2dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord2f(GLfloat s, GLfloat t)
{
	TRACE("(GLfloat s = %f, GLfloat t = %f)", s, t);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//context->texCoord(s, t, 0.0f, 1.0f);
		unsigned int texture = context->getActiveTexture();
		context->setVertexAttrib(sw::TexCoord0/* + texture*/, s, t, 0.0f, 1.0f);
	}
}

void APIENTRY glTexCoord2fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord2i(GLint s, GLint t)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord2iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord2s(GLshort s, GLshort t)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord2sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord3dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord3fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord3i(GLint s, GLint t, GLint r)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord3iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord3sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord4dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord4fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord4iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoord4sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(*)");

	gl::Context *context = gl::getContext();

	if(context)
	{
		GLenum texture = context->getClientActiveTexture();

		glVertexAttribPointer(sw::TexCoord0 + (texture - GL_TEXTURE0), size, type, false, stride, pointer);
	}
}

void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	UNIMPLEMENTED();
}

void APIENTRY glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
	UNIMPLEMENTED();
}

void APIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
	TRACE("(*)");

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			return context->listCommand(gl::newCommand(glTranslated, x, y, z));
		}

		context->translate(x, y, z);   // FIXME
	}
}

void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	TRACE("(GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", x, y, z);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			return context->listCommand(gl::newCommand(glTranslatef, x, y, z));
		}

		context->translate(x, y, z);
	}
}

void APIENTRY glVertex2d(GLdouble x, GLdouble y)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex2dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex2f(GLfloat x, GLfloat y)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex2fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex2i(GLint x, GLint y)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex2iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex2s(GLshort x, GLshort y)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex2sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex3dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	TRACE("(GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", x, y, z);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		context->position(x, y, z, 1.0f);
	}
}

void APIENTRY glVertex3fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex3i(GLint x, GLint y, GLint z)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex3iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex3s(GLshort x, GLshort y, GLshort z)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex3sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex4dv(const GLdouble *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex4fv(const GLfloat *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex4iv(const GLint *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertex4sv(const GLshort *v)
{
	UNIMPLEMENTED();
}

void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	TRACE("(GLint size = %d, GLenum type = 0x%X, GLsizei stride = %d, const GLvoid *pointer = %p)", size, type, stride, pointer);

	glVertexAttribPointer(sw::Position, size, type, false, stride, pointer);
}

void APIENTRY glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {UNIMPLEMENTED();}
void APIENTRY glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) {UNIMPLEMENTED();}
void APIENTRY glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {UNIMPLEMENTED();}
void APIENTRY glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {UNIMPLEMENTED();}

void APIENTRY glClientActiveTexture(GLenum texture)
{
	TRACE("(GLenum texture = 0x%X)", texture);

	switch(texture)
	{
	case GL_TEXTURE0:
	case GL_TEXTURE1:
		break;
	default:
		UNIMPLEMENTED();
		UNREACHABLE(texture);
	}

	gl::Context *context = gl::getContext();

	if(context)
	{
		context->clientActiveTexture(texture);
	}
}

void APIENTRY glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data) {UNIMPLEMENTED();}
void APIENTRY glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data) {UNIMPLEMENTED();}
void APIENTRY glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) {UNIMPLEMENTED();}
void APIENTRY glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {UNIMPLEMENTED();}
void APIENTRY glGetCompressedTexImage(GLenum target, GLint level, void *img) {UNIMPLEMENTED();}
void APIENTRY glMultiTexCoord1f(GLenum target, GLfloat s) {UNIMPLEMENTED();}
void APIENTRY glMultiTexCoord1d(GLenum target, GLdouble s) {UNIMPLEMENTED();}

void APIENTRY glMultiTexCoord2f(GLenum texture, GLfloat s, GLfloat t)
{
	TRACE("(GLenum texture = 0x%X, GLfloat s = %f, GLfloat t = %f)", texture, s, t);

	gl::Context *context = gl::getContext();

	if(context)
	{
		if(context->getListIndex() != 0)
		{
			UNIMPLEMENTED();
		}

		//context->texCoord(s, t, 0.0f, 1.0f);
		context->setVertexAttrib(sw::TexCoord0 + (texture - GL_TEXTURE0), s, t, 0.0f, 1.0f);
	}
}

void APIENTRY glMultiTexCoord2d(GLenum target, GLdouble s, GLdouble t) {UNIMPLEMENTED();}
void APIENTRY glMultiTexCoord3f(GLenum target, GLfloat s, GLfloat t, GLfloat r) {UNIMPLEMENTED();}
void APIENTRY glMultiTexCoord3d(GLenum target, GLdouble s, GLdouble t, GLdouble r) {UNIMPLEMENTED();}
void APIENTRY glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {UNIMPLEMENTED();}
void APIENTRY glMultiTexCoord4d(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q) {UNIMPLEMENTED();}
void APIENTRY glLoadTransposeMatrixf(const GLfloat *m) {UNIMPLEMENTED();}
void APIENTRY glLoadTransposeMatrixd(const GLdouble *m) {UNIMPLEMENTED();}
void APIENTRY glMultTransposeMatrixf(const GLfloat *m) {UNIMPLEMENTED();}
void APIENTRY glMultTransposeMatrixd(const GLdouble *m) {UNIMPLEMENTED();}
void APIENTRY glFogCoordf(GLfloat coord) {UNIMPLEMENTED();}
void APIENTRY glFogCoordd(GLdouble coord) {UNIMPLEMENTED();}
void APIENTRY glFogCoordPointer(GLenum type, GLsizei stride, const void *pointer) {UNIMPLEMENTED();}
void APIENTRY glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount) {UNIMPLEMENTED();}
void APIENTRY glPointParameteri(GLenum pname, GLint param) {UNIMPLEMENTED();}
void APIENTRY glPointParameterf(GLenum pname, GLfloat param) {UNIMPLEMENTED();}
void APIENTRY glPointParameteriv(GLenum pname, const GLint *params) {UNIMPLEMENTED();}
void APIENTRY glPointParameterfv(GLenum pname, const GLfloat *params) {UNIMPLEMENTED();}
void APIENTRY glSecondaryColor3b(GLbyte red, GLbyte green, GLbyte blue) {UNIMPLEMENTED();}
void APIENTRY glSecondaryColor3f(GLfloat red, GLfloat green, GLfloat blue) {UNIMPLEMENTED();}
void APIENTRY glSecondaryColor3d(GLdouble red, GLdouble green, GLdouble blue) {UNIMPLEMENTED();}
void APIENTRY glSecondaryColor3ub(GLubyte red, GLubyte green, GLubyte blue) {UNIMPLEMENTED();}
void APIENTRY glSecondaryColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {UNIMPLEMENTED();}
void APIENTRY glWindowPos2f(GLfloat x, GLfloat y) {UNIMPLEMENTED();}
void APIENTRY glWindowPos2d(GLdouble x, GLdouble y) {UNIMPLEMENTED();}
void APIENTRY glWindowPos2i(GLint x, GLint y) {UNIMPLEMENTED();}
void APIENTRY glWindowPos3f(GLfloat x, GLfloat y, GLfloat z) {UNIMPLEMENTED();}
void APIENTRY glWindowPos3d(GLdouble x, GLdouble y, GLdouble z) {UNIMPLEMENTED();}
void APIENTRY glWindowPos3i(GLint x, GLint y, GLint z) {UNIMPLEMENTED();}
void APIENTRY glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data) {UNIMPLEMENTED();}
void *APIENTRY glMapBuffer(GLenum target, GLenum access) {UNIMPLEMENTED(); return 0;}
GLboolean APIENTRY glUnmapBuffer(GLenum target) {UNIMPLEMENTED(); return GL_FALSE;}
void APIENTRY glGetBufferPointerv(GLenum target, GLenum pname, void **params) {UNIMPLEMENTED();}
void APIENTRY glGenQueries(GLsizei n, GLuint *ids) {UNIMPLEMENTED();}
void APIENTRY glDeleteQueries(GLsizei n, const GLuint *ids) {UNIMPLEMENTED();}
GLboolean APIENTRY glIsQuery(GLuint id) {UNIMPLEMENTED(); return 0;}
void APIENTRY glBeginQuery(GLenum target, GLuint id) {UNIMPLEMENTED();}
void APIENTRY glEndQuery(GLenum target) {UNIMPLEMENTED();}
void APIENTRY glGetQueryiv(GLenum target, GLenum pname, GLint *params) {UNIMPLEMENTED();}
void APIENTRY glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params) {UNIMPLEMENTED();}
void APIENTRY glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib1s(GLuint index, GLshort x) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib1d(GLuint index, GLdouble x) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib2s(GLuint index, GLshort x, GLshort y) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {UNIMPLEMENTED();}
void APIENTRY glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {UNIMPLEMENTED();}
void APIENTRY glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params) {UNIMPLEMENTED();}
void APIENTRY glDrawBuffers(GLsizei n, const GLenum *bufs) {UNIMPLEMENTED();}
void APIENTRY glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {UNIMPLEMENTED();}
void APIENTRY glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {UNIMPLEMENTED();}
void APIENTRY glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {UNIMPLEMENTED();}
void APIENTRY glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {UNIMPLEMENTED();}
void APIENTRY glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {UNIMPLEMENTED();}
void APIENTRY glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {UNIMPLEMENTED();}

void APIENTRY glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {UNIMPLEMENTED();}
void APIENTRY glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {UNIMPLEMENTED();}
void APIENTRY glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {UNIMPLEMENTED();}

BOOL WINAPI wglSwapIntervalEXT(int interval)
{
	gl::Surface *drawSurface = static_cast<gl::Surface*>(gl::getCurrentDrawSurface());

	if(drawSurface)
	{
		drawSurface->setSwapInterval(interval);
		return TRUE;
	}

	SetLastError(ERROR_DC_NOT_FOUND);
	return FALSE;
}

int WINAPI wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd)
{
	TRACE("(*)");

	return 1;
}

BOOL WINAPI wglCopyContext(HGLRC, HGLRC, UINT)
{
	UNIMPLEMENTED();
	return FALSE;
}

HGLRC WINAPI wglCreateContext(HDC hdc)
{
	TRACE("(*)");

	gl::Display *display = gl::Display::getDisplay(hdc);
	display->initialize();

	gl::Context *context = display->createContext(nullptr);

	return (HGLRC)context;
}

HGLRC WINAPI wglCreateLayerContext(HDC, int)
{
	UNIMPLEMENTED();
	return 0;
}

BOOL WINAPI wglDeleteContext(HGLRC context)
{
	gl::Display *display = gl::getDisplay();

	if(display && context)
	{
		display->destroyContext(reinterpret_cast<gl::Context*>(context));

		return TRUE;
	}

	return FALSE;
}

BOOL WINAPI wglDescribeLayerPlane(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR)
{
	UNIMPLEMENTED();
	return FALSE;
}

int WINAPI wglDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
	TRACE("(*)");

	ASSERT(nBytes == sizeof(PIXELFORMATDESCRIPTOR));   // FIXME

	ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
	ppfd->nVersion = 1;
	ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	ppfd->iPixelType = PFD_TYPE_RGBA;
	ppfd->cColorBits = 32;
	ppfd->cRedBits = 8;
	ppfd->cRedShift = 16;
	ppfd->cGreenBits = 8;
	ppfd->cGreenShift = 8;
	ppfd->cBlueBits = 8;
	ppfd->cBlueShift = 0;
	ppfd->cAlphaBits = 0;
	ppfd->cAlphaShift = 24;
	ppfd->cAccumBits = 0;
	ppfd->cAccumRedBits = 0;
	ppfd->cAccumGreenBits = 0;
	ppfd->cAccumBlueBits = 0;
	ppfd->cAccumAlphaBits = 0;
	ppfd->cDepthBits = 24;
	ppfd->cStencilBits = 0;
	ppfd->cAuxBuffers = 0;
	ppfd->iLayerType = 0;
	ppfd->bReserved = 0;
	ppfd->dwLayerMask = 0;
	ppfd->dwVisibleMask = 0;
	ppfd->dwDamageMask = 0;

	return 1;
}

HGLRC WINAPI wglGetCurrentContext(VOID)
{
	TRACE("(*)");
	return (HGLRC)gl::getContext();
}

HDC WINAPI wglGetCurrentDC(VOID)
{
	TRACE("(*)");
	gl::Display *display = gl::getDisplay();
	return display ? display->getNativeDisplay() : 0;
}

void WINAPI wglGetDefaultProcAddress()
{
	UNIMPLEMENTED();
}

int WINAPI wglGetLayerPaletteEntries(HDC, int, int, int, COLORREF*)
{
	UNIMPLEMENTED();
	return 0;
}

void WINAPI wglGetPixelFormat()
{
	UNIMPLEMENTED();
}

const char *WINAPI wglGetExtensionsStringARB(HDC hdc)
{
	TRACE("(*)");

	return "GL_ARB_framebuffer_object "
	       "WGL_EXT_extensions_string "
	       "WGL_EXT_swap_control";
}

const char *WINAPI wglGetExtensionsStringEXT()
{
	TRACE("(*)");
	return wglGetExtensionsStringARB(0);
}

PROC WINAPI wglGetProcAddress(LPCSTR lpszProc)
{
	TRACE("(LPCSTR lpszProc = \"%s\")", lpszProc);

	struct Extension
	{
		const char *name;
		PROC address;
	};

	static const Extension glExtensions[] =
	{
		#define EXT(function) {#function, (PROC)function}

		// Core 2.1
		EXT(glDrawRangeElements),
		EXT(glTexImage3D),
		EXT(glTexSubImage3D),
		EXT(glCopyTexSubImage3D),
		EXT(glActiveTexture),
		EXT(glClientActiveTexture),
		EXT(glCompressedTexImage1D),
		EXT(glCompressedTexImage2D),
		EXT(glCompressedTexImage3D),
		EXT(glCompressedTexSubImage1D),
		EXT(glCompressedTexSubImage2D),
		EXT(glCompressedTexSubImage3D),
		EXT(glGetCompressedTexImage),
		EXT(glMultiTexCoord1f),
		EXT(glMultiTexCoord1d),
		EXT(glMultiTexCoord2f),
		EXT(glMultiTexCoord2d),
		EXT(glMultiTexCoord3f),
		EXT(glMultiTexCoord3d),
		EXT(glMultiTexCoord4f),
		EXT(glMultiTexCoord4d),
		EXT(glLoadTransposeMatrixf),
		EXT(glLoadTransposeMatrixd),
		EXT(glMultTransposeMatrixf),
		EXT(glMultTransposeMatrixd),
		EXT(glSampleCoverage),
		EXT(glBlendEquation),
		EXT(glBlendColor),
		EXT(glFogCoordf),
		EXT(glFogCoordd),
		EXT(glFogCoordPointer),
		EXT(glMultiDrawArrays),
		EXT(glPointParameteri),
		EXT(glPointParameterf),
		EXT(glPointParameteriv),
		EXT(glPointParameterfv),
		EXT(glSecondaryColor3b),
		EXT(glSecondaryColor3f),
		EXT(glSecondaryColor3d),
		EXT(glSecondaryColor3ub),
		EXT(glSecondaryColorPointer),
		EXT(glBlendFuncSeparate),
		EXT(glWindowPos2f),
		EXT(glWindowPos2d),
		EXT(glWindowPos2i),
		EXT(glWindowPos3f),
		EXT(glWindowPos3d),
		EXT(glWindowPos3i),
		EXT(glBindBuffer),
		EXT(glDeleteBuffers),
		EXT(glGenBuffers),
		EXT(glIsBuffer),
		EXT(glBufferData),
		EXT(glBufferSubData),
		EXT(glGetBufferSubData),
		EXT(glMapBuffer),
		EXT(glUnmapBuffer),
		EXT(glGetBufferParameteriv),
		EXT(glGetBufferPointerv),
		EXT(glGenQueries),
		EXT(glDeleteQueries),
		EXT(glIsQuery),
		EXT(glBeginQuery),
		EXT(glEndQuery),
		EXT(glGetQueryiv),
		EXT(glGetQueryObjectiv),
		EXT(glGetQueryObjectuiv),
		EXT(glShaderSource),
		EXT(glCreateShader),
		EXT(glIsShader),
		EXT(glCompileShader),
		EXT(glDeleteShader),
		EXT(glCreateProgram),
		EXT(glIsProgram),
		EXT(glAttachShader),
		EXT(glDetachShader),
		EXT(glLinkProgram),
		EXT(glUseProgram),
		EXT(glValidateProgram),
		EXT(glDeleteProgram),
		EXT(glUniform1f),
		EXT(glUniform2f),
		EXT(glUniform3f),
		EXT(glUniform4f),
		EXT(glUniform1i),
		EXT(glUniform2i),
		EXT(glUniform3i),
		EXT(glUniform4i),
		EXT(glUniform1fv),
		EXT(glUniform2fv),
		EXT(glUniform3fv),
		EXT(glUniform4fv),
		EXT(glUniform1iv),
		EXT(glUniform2iv),
		EXT(glUniform3iv),
		EXT(glUniform4iv),
		EXT(glUniformMatrix2fv),
		EXT(glUniformMatrix3fv),
		EXT(glUniformMatrix4fv),
		EXT(glGetShaderiv),
		EXT(glGetProgramiv),
		EXT(glGetShaderInfoLog),
		EXT(glGetProgramInfoLog),
		EXT(glGetAttachedShaders),
		EXT(glGetUniformLocation),
		EXT(glGetActiveUniform),
		EXT(glGetUniformfv),
		EXT(glGetUniformiv),
		EXT(glGetShaderSource),
		EXT(glVertexAttrib1s),
		EXT(glVertexAttrib1f),
		EXT(glVertexAttrib1d),
		EXT(glVertexAttrib2s),
		EXT(glVertexAttrib2f),
		EXT(glVertexAttrib2d),
		EXT(glVertexAttrib3s),
		EXT(glVertexAttrib3f),
		EXT(glVertexAttrib3d),
		EXT(glVertexAttrib4s),
		EXT(glVertexAttrib4f),
		EXT(glVertexAttrib4d),
		EXT(glVertexAttrib4Nub),
		EXT(glVertexAttribPointer),
		EXT(glEnableVertexAttribArray),
		EXT(glDisableVertexAttribArray),
		EXT(glGetVertexAttribfv),
		EXT(glGetVertexAttribdv),
		EXT(glGetVertexAttribiv),
		EXT(glGetVertexAttribPointerv),
		EXT(glBindAttribLocation),
		EXT(glGetActiveAttrib),
		EXT(glGetAttribLocation),
		EXT(glDrawBuffers),
		EXT(glStencilOpSeparate),
		EXT(glStencilFuncSeparate),
		EXT(glStencilMaskSeparate),
		EXT(glBlendEquationSeparate),
		EXT(glUniformMatrix2x3fv),
		EXT(glUniformMatrix3x2fv),
		EXT(glUniformMatrix2x4fv),
		EXT(glUniformMatrix4x2fv),
		EXT(glUniformMatrix3x4fv),
		EXT(glUniformMatrix4x3fv),
		EXT(glGenFencesNV),
		EXT(glDeleteFencesNV),
		EXT(glSetFenceNV),
		EXT(glTestFenceNV),
		EXT(glFinishFenceNV),
		EXT(glIsFenceNV),
		EXT(glGetFenceivNV),

		EXT(glIsRenderbuffer),
		EXT(glBindRenderbuffer),
		EXT(glDeleteRenderbuffers),
		EXT(glGenRenderbuffers),
		EXT(glRenderbufferStorage),
		EXT(glGetRenderbufferParameteriv),
		EXT(glIsFramebuffer),
		EXT(glBindFramebuffer),
		EXT(glDeleteFramebuffers),
		EXT(glGenFramebuffers),
		EXT(glCheckFramebufferStatus),
		EXT(glFramebufferTexture1D),
		EXT(glFramebufferTexture2D),
		EXT(glFramebufferTexture3D),
		EXT(glFramebufferRenderbuffer),
		EXT(glGetFramebufferAttachmentParameteriv),
		EXT(glGenerateMipmap),
		EXT(glReleaseShaderCompiler),
		EXT(glShaderBinary),
		EXT(glGetShaderPrecisionFormat),
		EXT(glDepthRangef),
		EXT(glClearDepthf),

		// ARB
		EXT(wglGetExtensionsStringARB),
		EXT(glIsRenderbuffer),
		EXT(glBindRenderbuffer),
		EXT(glDeleteRenderbuffers),
		EXT(glGenRenderbuffers),
		EXT(glRenderbufferStorage),
		EXT(glRenderbufferStorageMultisample),
		EXT(glGetRenderbufferParameteriv),
		EXT(glIsFramebuffer),
		EXT(glBindFramebuffer),
		EXT(glDeleteFramebuffers),
		EXT(glGenFramebuffers),
		EXT(glCheckFramebufferStatus),
		EXT(glFramebufferTexture1D),
		EXT(glFramebufferTexture2D),
		EXT(glFramebufferTexture3D),
		EXT(glFramebufferTextureLayer),
		EXT(glFramebufferRenderbuffer),
		EXT(glGetFramebufferAttachmentParameteriv),
		EXT(glBlitFramebuffer),
		EXT(glGenerateMipmap),

		// EXT
		EXT(wglSwapIntervalEXT),
		EXT(wglGetExtensionsStringEXT),
		#undef EXT
	};

	for(int ext = 0; ext < sizeof(glExtensions) / sizeof(Extension); ext++)
	{
		if(strcmp(lpszProc, glExtensions[ext].name) == 0)
		{
			return (PROC)glExtensions[ext].address;
		}
	}

	FARPROC proc = GetProcAddress(GetModuleHandle("opengl32.dll"), lpszProc);  // FIXME?

	if(proc)
	{
		return proc;
	}

	TRACE("(LPCSTR lpszProc = \"%s\") NOT FOUND!!!", lpszProc);

	return 0;
}

BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
	TRACE("(*)");

	if(hdc && hglrc)
	{
		gl::Display *display = (gl::Display*)gl::Display::getDisplay(hdc);
		gl::makeCurrent((gl::Context*)hglrc, display, display->getPrimarySurface());
		gl::setCurrentDrawSurface(display->getPrimarySurface());
		gl::setCurrentDisplay(display);
	}
	else
	{
		gl::makeCurrent(0, 0, 0);
	}

	return TRUE;
}

BOOL WINAPI wglRealizeLayerPalette(HDC, int, BOOL)
{
	UNIMPLEMENTED();
	return FALSE;
}

int WINAPI wglSetLayerPaletteEntries(HDC, int, int, int, CONST COLORREF*)
{
	UNIMPLEMENTED();
	return 0;
}

BOOL WINAPI wglSetPixelFormat(HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{
	TRACE("(*)");
	//UNIMPLEMENTED();

	return TRUE;
}

BOOL WINAPI wglShareLists(HGLRC, HGLRC)
{
	UNIMPLEMENTED();
	return FALSE;
}

BOOL WINAPI wglSwapBuffers(HDC hdc)
{
	TRACE("(*)");

	gl::Display *display = gl::getDisplay();

	if(display)
	{
		display->getPrimarySurface()->swap();
		return TRUE;
	}

	return FALSE;
}

BOOL WINAPI wglSwapLayerBuffers(HDC, UINT)
{
	UNIMPLEMENTED();
	return FALSE;
}

DWORD WINAPI wglSwapMultipleBuffers(UINT, CONST WGLSWAP*)
{
	UNIMPLEMENTED();
	return 0;
}

BOOL WINAPI wglUseFontBitmapsA(HDC, DWORD, DWORD, DWORD)
{
	UNIMPLEMENTED();
	return FALSE;
}

BOOL WINAPI wglUseFontBitmapsW(HDC, DWORD, DWORD, DWORD)
{
	UNIMPLEMENTED();
	return FALSE;
}

BOOL WINAPI wglUseFontOutlinesA(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT)
{
	UNIMPLEMENTED();
	return FALSE;
}

BOOL WINAPI wglUseFontOutlinesW(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT)
{
	UNIMPLEMENTED();
	return FALSE;
}

}
