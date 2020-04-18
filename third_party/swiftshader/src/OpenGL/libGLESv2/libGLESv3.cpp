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

// libGLESv3.cpp: Implements the exported OpenGL ES 3.0 functions.

#include "main.h"
#include "Buffer.h"
#include "Fence.h"
#include "Framebuffer.h"
#include "Program.h"
#include "Query.h"
#include "Sampler.h"
#include "Texture.h"
#include "mathutil.h"
#include "TransformFeedback.h"
#include "VertexArray.h"
#include "common/debug.h"

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <limits.h>

using namespace es2;

static bool validImageSize(GLint level, GLsizei width, GLsizei height)
{
	if(level < 0 || level >= es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS || width < 0 || height < 0)
	{
		return false;
	}

	return true;
}

static bool ValidateQueryTarget(GLenum target)
{
	switch(target)
	{
	case GL_ANY_SAMPLES_PASSED:
	case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
	case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
		break;
	default:
		return false;
	}

	return true;
}

bool ValidateTexParamParameters(GLenum pname, GLint param)
{
	switch(pname)
	{
	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_WRAP_R:
		switch(param)
		{
		case GL_REPEAT:
		case GL_CLAMP_TO_EDGE:
		case GL_MIRRORED_REPEAT:
			return true;
		default:
			return error(GL_INVALID_ENUM, false);
		}

	case GL_TEXTURE_MIN_FILTER:
		switch(param)
		{
		case GL_NEAREST:
		case GL_LINEAR:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
			return true;
		default:
			return error(GL_INVALID_ENUM, false);
		}
		break;

	case GL_TEXTURE_MAG_FILTER:
		switch(param)
		{
		case GL_NEAREST:
		case GL_LINEAR:
			return true;
		default:
			return error(GL_INVALID_ENUM, false);
		}
		break;

	case GL_TEXTURE_USAGE_ANGLE:
		switch(param)
		{
		case GL_NONE:
		case GL_FRAMEBUFFER_ATTACHMENT_ANGLE:
			return true;
		default:
			return error(GL_INVALID_ENUM, false);
		}
		break;

	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
		// we assume the parameter passed to this validation method is truncated, not rounded
		if(param < 1)
		{
			return error(GL_INVALID_VALUE, false);
		}
		return true;

	case GL_TEXTURE_MIN_LOD:
	case GL_TEXTURE_MAX_LOD:
		// any value is permissible
		return true;

	case GL_TEXTURE_COMPARE_MODE:
		// Acceptable mode parameters from GLES 3.0.2 spec, table 3.17
		switch(param)
		{
		case GL_NONE:
		case GL_COMPARE_REF_TO_TEXTURE:
			return true;
		default:
			return error(GL_INVALID_ENUM, false);
		}
		break;

	case GL_TEXTURE_COMPARE_FUNC:
		// Acceptable function parameters from GLES 3.0.2 spec, table 3.17
		switch(param)
		{
		case GL_LEQUAL:
		case GL_GEQUAL:
		case GL_LESS:
		case GL_GREATER:
		case GL_EQUAL:
		case GL_NOTEQUAL:
		case GL_ALWAYS:
		case GL_NEVER:
			return true;
		default:
			return error(GL_INVALID_ENUM, false);
		}
		break;

	case GL_TEXTURE_SWIZZLE_R:
	case GL_TEXTURE_SWIZZLE_G:
	case GL_TEXTURE_SWIZZLE_B:
	case GL_TEXTURE_SWIZZLE_A:
		switch(param)
		{
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_ZERO:
		case GL_ONE:
			return true;
		default:
			return error(GL_INVALID_ENUM, false);
		}
		break;

	case GL_TEXTURE_BASE_LEVEL:
	case GL_TEXTURE_MAX_LEVEL:
		if(param < 0)
		{
			return error(GL_INVALID_VALUE, false);
		}
		return true;

	default:
		return error(GL_INVALID_ENUM, false);
	}
}

static bool ValidateSamplerObjectParameter(GLenum pname)
{
	switch(pname)
	{
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_WRAP_R:
	case GL_TEXTURE_MIN_LOD:
	case GL_TEXTURE_MAX_LOD:
	case GL_TEXTURE_COMPARE_MODE:
	case GL_TEXTURE_COMPARE_FUNC:
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
		return true;
	default:
		return false;
	}
}

extern "C"
{

GL_APICALL void GL_APIENTRY glReadBuffer(GLenum src)
{
	TRACE("(GLenum src = 0x%X)", src);

	es2::Context *context = es2::getContext();

	if(context)
	{
		GLuint readFramebufferName = context->getReadFramebufferName();

		switch(src)
		{
		case GL_BACK:
			if(readFramebufferName != 0)
			{
				return error(GL_INVALID_OPERATION);
			}
			context->setFramebufferReadBuffer(src);
			break;
		case GL_NONE:
			context->setFramebufferReadBuffer(src);
			break;
		case GL_COLOR_ATTACHMENT0:
		case GL_COLOR_ATTACHMENT1:
		case GL_COLOR_ATTACHMENT2:
		case GL_COLOR_ATTACHMENT3:
		case GL_COLOR_ATTACHMENT4:
		case GL_COLOR_ATTACHMENT5:
		case GL_COLOR_ATTACHMENT6:
		case GL_COLOR_ATTACHMENT7:
		case GL_COLOR_ATTACHMENT8:
		case GL_COLOR_ATTACHMENT9:
		case GL_COLOR_ATTACHMENT10:
		case GL_COLOR_ATTACHMENT11:
		case GL_COLOR_ATTACHMENT12:
		case GL_COLOR_ATTACHMENT13:
		case GL_COLOR_ATTACHMENT14:
		case GL_COLOR_ATTACHMENT15:
		case GL_COLOR_ATTACHMENT16:
		case GL_COLOR_ATTACHMENT17:
		case GL_COLOR_ATTACHMENT18:
		case GL_COLOR_ATTACHMENT19:
		case GL_COLOR_ATTACHMENT20:
		case GL_COLOR_ATTACHMENT21:
		case GL_COLOR_ATTACHMENT22:
		case GL_COLOR_ATTACHMENT23:
		case GL_COLOR_ATTACHMENT24:
		case GL_COLOR_ATTACHMENT25:
		case GL_COLOR_ATTACHMENT26:
		case GL_COLOR_ATTACHMENT27:
		case GL_COLOR_ATTACHMENT28:
		case GL_COLOR_ATTACHMENT29:
		case GL_COLOR_ATTACHMENT30:
		case GL_COLOR_ATTACHMENT31:
			{
				GLuint index = (src - GL_COLOR_ATTACHMENT0);
				if(index >= MAX_COLOR_ATTACHMENTS)
				{
					return error(GL_INVALID_OPERATION);
				}
				if(readFramebufferName == 0)
				{
					return error(GL_INVALID_OPERATION);
				}
				context->setFramebufferReadBuffer(src);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
	TRACE("(GLenum mode = 0x%X, GLuint start = %d, GLuint end = %d, "
		  "GLsizei count = %d, GLenum type = 0x%x, const void* indices = %p)",
		  mode, start, end, count, type, indices);

	switch(mode)
	{
	case GL_POINTS:
	case GL_LINES:
	case GL_LINE_LOOP:
	case GL_LINE_STRIP:
	case GL_TRIANGLES:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLE_STRIP:
		break;
	default:
		return error(GL_INVALID_ENUM);
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

	if((count < 0) || (end < start))
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback* transformFeedback = context->getTransformFeedback();
		if(transformFeedback && transformFeedback->isActive() && !transformFeedback->isPaused())
		{
			return error(GL_INVALID_OPERATION);
		}

		context->drawElements(mode, start, end, count, type, indices);
	}
}

GL_APICALL void GL_APIENTRY glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *data)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, "
	      "GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, GLint border = %d, "
	      "GLenum format = 0x%X, GLenum type = 0x%x, const GLvoid* data = %p)",
	      target, level, internalformat, width, height, depth, border, format, type, data);

	switch(target)
	{
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if((level < 0) || (level >= es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS))
	{
		return error(GL_INVALID_VALUE);
	}

	const GLsizei maxSize3D = es2::IMPLEMENTATION_MAX_3D_TEXTURE_SIZE >> level;
	if((width < 0) || (height < 0) || (depth < 0) || (width > maxSize3D) || (height > maxSize3D) || (depth > maxSize3D))
	{
		return error(GL_INVALID_VALUE);
	}

	if(border != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		GLenum validationError = ValidateTextureFormatType(format, type, internalformat, target, context->getClientVersion());
		if(validationError != GL_NO_ERROR)
		{
			return error(validationError);
		}

		es2::Texture3D *texture = (target == GL_TEXTURE_3D) ? context->getTexture3D() : context->getTexture2DArray();

		if(!texture)
		{
			return error(GL_INVALID_OPERATION);
		}

		validationError = context->getPixels(&data, type, context->getRequiredBufferSize(width, height, depth, format, type));
		if(validationError != GL_NO_ERROR)
		{
			return error(validationError);
		}

		GLint sizedInternalFormat = gl::GetSizedInternalFormat(internalformat, type);
		texture->setImage(level, width, height, depth, sizedInternalFormat, format, type, context->getUnpackParameters(), data);
	}
}

GL_APICALL void GL_APIENTRY glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
		"GLint zoffset = %d, GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, "
		"GLenum format = 0x%X, GLenum type = 0x%x, const GLvoid* data = %p)",
		target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);

	switch(target)
	{
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if((level < 0) || (level >= es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS))
	{
		return error(GL_INVALID_VALUE);
	}

	if((width < 0) || (height < 0) || (depth < 0) || (xoffset < 0) || (yoffset < 0) || (zoffset < 0))
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Texture3D *texture = (target == GL_TEXTURE_3D) ? context->getTexture3D() : context->getTexture2DArray();

		GLenum validationError = ValidateSubImageParams(false, false, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, texture, context->getClientVersion());
		if(validationError != GL_NO_ERROR)
		{
			return error(validationError);
		}

		validationError = context->getPixels(&data, type, context->getRequiredBufferSize(width, height, depth, format, type));
		if(validationError != GL_NO_ERROR)
		{
			return error(validationError);
		}

		texture->subImage(level, xoffset, yoffset, zoffset, width, height, depth, format, type, context->getUnpackParameters(), data);
	}
}

GL_APICALL void GL_APIENTRY glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
		"GLint zoffset = %d, GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)",
		target, level, xoffset, yoffset, zoffset, x, y, width, height);

	switch(target)
	{
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if((level < 0) || (level >= es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS))
	{
		return error(GL_INVALID_VALUE);
	}

	if((width < 0) || (height < 0) || (xoffset < 0) || (yoffset < 0) || (zoffset < 0))
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Framebuffer *framebuffer = context->getReadFramebuffer();

		if(!framebuffer || (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE))
		{
			return error(GL_INVALID_FRAMEBUFFER_OPERATION);
		}

		es2::Renderbuffer *source = framebuffer->getReadColorbuffer();

		if(context->getReadFramebufferName() != 0 && (!source || source->getSamples() > 1))
		{
			return error(GL_INVALID_OPERATION);
		}

		GLenum colorbufferFormat = source->getFormat();
		es2::Texture3D *texture = (target == GL_TEXTURE_3D) ? context->getTexture3D() : context->getTexture2DArray();

		GLenum validationError = ValidateSubImageParams(false, true, target, level, xoffset, yoffset, zoffset, width, height, 1, GL_NONE, GL_NONE, texture, context->getClientVersion());
		if(validationError != GL_NO_ERROR)
		{
			return error(validationError);
		}

		GLenum textureFormat = texture->getFormat(target, level);

		if(!ValidateCopyFormats(textureFormat, colorbufferFormat))
		{
			return;
		}

		texture->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, source);
	}
}

GL_APICALL void GL_APIENTRY glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLsizei width = %d, "
		"GLsizei height = %d, GLsizei depth = %d, GLint border = %d, GLsizei imageSize = %d, const GLvoid* data = %p)",
		target, level, internalformat, width, height, depth, border, imageSize, data);

	switch(target)
	{
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if((level < 0) || (level >= es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS))
	{
		return error(GL_INVALID_VALUE);
	}

	const GLsizei maxSize3D = es2::IMPLEMENTATION_MAX_3D_TEXTURE_SIZE >> level;
	if((width < 0) || (height < 0) || (depth < 0) || (width > maxSize3D) || (height > maxSize3D) || (depth > maxSize3D) || (border != 0) || (imageSize < 0))
	{
		return error(GL_INVALID_VALUE);
	}

	if(!IsCompressed(internalformat, egl::getClientVersion()))
	{
		return error(GL_INVALID_ENUM);
	}

	if(imageSize != gl::ComputeCompressedSize(width, height, internalformat) * depth)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Texture3D *texture = (target == GL_TEXTURE_3D) ? context->getTexture3D() : context->getTexture2DArray();

		if(!texture)
		{
			return error(GL_INVALID_OPERATION);
		}

		GLenum validationError = context->getPixels(&data, GL_UNSIGNED_BYTE, imageSize);
		if(validationError != GL_NO_ERROR)
		{
			return error(validationError);
		}

		texture->setCompressedImage(level, internalformat, width, height, depth, imageSize, data);
	}
}

GL_APICALL void GL_APIENTRY glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)
{
	TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
	      "GLint zoffset = %d, GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, "
	      "GLenum format = 0x%X, GLsizei imageSize = %d, const void *data = %p)",
	      target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);

	switch(target)
	{
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(level < 0 || level >= es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
	{
		return error(GL_INVALID_VALUE);
	}

	if(xoffset < 0 || yoffset < 0 || zoffset < 0 || !validImageSize(level, width, height) || depth < 0 || imageSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(!IsCompressed(format, egl::getClientVersion()))
	{
		return error(GL_INVALID_ENUM);
	}

	if(imageSize != gl::ComputeCompressedSize(width, height, format) * depth)
	{
		return error(GL_INVALID_VALUE);
	}

	bool is_ETC2_EAC = false;
	switch(format)
	{
	case GL_COMPRESSED_R11_EAC:
	case GL_COMPRESSED_SIGNED_R11_EAC:
	case GL_COMPRESSED_RG11_EAC:
	case GL_COMPRESSED_SIGNED_RG11_EAC:
	case GL_COMPRESSED_RGB8_ETC2:
	case GL_COMPRESSED_SRGB8_ETC2:
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
	case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
		if(target != GL_TEXTURE_2D_ARRAY)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(((width % 4) != 0) || ((height % 4) != 0) ||
		   ((xoffset % 4) != 0) || ((yoffset % 4) != 0))
		{
			return error(GL_INVALID_OPERATION);
		}

		is_ETC2_EAC = true;
		break;
	default:
		break;
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Texture3D *texture = (target == GL_TEXTURE_3D) ? context->getTexture3D() : context->getTexture2DArray();

		if(!texture)
		{
			return error(GL_INVALID_OPERATION);
		}

		GLenum validationError = context->getPixels(&data, GL_UNSIGNED_BYTE, imageSize);
		if(validationError != GL_NO_ERROR)
		{
			return error(validationError);
		}

		if(is_ETC2_EAC)
		{
			if(((width + xoffset) != texture->getWidth(target, level)) ||
			   ((height + yoffset) != texture->getHeight(target, level)) ||
			   ((depth + zoffset) != texture->getDepth(target, level)))
			{
				return error(GL_INVALID_OPERATION);
			}
		}

		texture->subImageCompressed(level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
	}
}

GL_APICALL void GL_APIENTRY glGenQueries(GLsizei n, GLuint *ids)
{
	TRACE("(GLsizei n = %d, GLuint* ids = %p)", n, ids);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			ids[i] = context->createQuery();
		}
	}
}

GL_APICALL void GL_APIENTRY glDeleteQueries(GLsizei n, const GLuint *ids)
{
	TRACE("(GLsizei n = %d, GLuint* ids = %p)", n, ids);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			context->deleteQuery(ids[i]);
		}
	}
}

GL_APICALL GLboolean GL_APIENTRY glIsQuery(GLuint id)
{
	TRACE("(GLuint id = %d)", id);

	if(id == 0)
	{
		return GL_FALSE;
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Query *queryObject = context->getQuery(id);

		if(queryObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GL_APICALL void GL_APIENTRY glBeginQuery(GLenum target, GLuint id)
{
	TRACE("(GLenum target = 0x%X, GLuint id = %d)", target, id);

	if(!ValidateQueryTarget(target))
	{
		return error(GL_INVALID_ENUM);
	}

	if(id == 0)
	{
		return error(GL_INVALID_OPERATION);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		context->beginQuery(target, id);
	}
}

GL_APICALL void GL_APIENTRY glEndQuery(GLenum target)
{
	TRACE("(GLenum target = 0x%X)", target);

	if(!ValidateQueryTarget(target))
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		context->endQuery(target);
	}
}

GL_APICALL void GL_APIENTRY glGetQueryiv(GLenum target, GLenum pname, GLint *params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint *params = %p)",
		  target, pname, params);

	if(!ValidateQueryTarget(target) || (pname != GL_CURRENT_QUERY))
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		params[0] = context->getActiveQuery(target);
	}
}

GL_APICALL void GL_APIENTRY glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params)
{
	TRACE("(GLuint id = %d, GLenum pname = 0x%X, GLint *params = %p)",
	      id, pname, params);

	switch(pname)
	{
	case GL_QUERY_RESULT:
	case GL_QUERY_RESULT_AVAILABLE:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Query *queryObject = context->getQuery(id);

		if(!queryObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(context->getActiveQuery(queryObject->getType()) == id)
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

GL_APICALL GLboolean GL_APIENTRY glUnmapBuffer(GLenum target)
{
	TRACE("(GLenum target = 0x%X)", target);

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Buffer *buffer = nullptr;
		if(!context->getBuffer(target, &buffer))
		{
			return error(GL_INVALID_ENUM, GL_TRUE);
		}

		if(!buffer)
		{
			// A null buffer means that "0" is bound to the requested buffer target
			return error(GL_INVALID_OPERATION, GL_TRUE);
		}

		if(!buffer->isMapped())
		{
			// Already unmapped
			return error(GL_INVALID_OPERATION, GL_TRUE);
		}

		return buffer->unmap() ? GL_TRUE : GL_FALSE;
	}

	return GL_TRUE;
}

GL_APICALL void GL_APIENTRY glGetBufferPointerv(GLenum target, GLenum pname, void **params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint *params = %p)",
	      target, pname, params);

	if(pname != GL_BUFFER_MAP_POINTER)
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Buffer *buffer = nullptr;
		if(!context->getBuffer(target, &buffer))
		{
			return error(GL_INVALID_ENUM);
		}

		if(!buffer)
		{
			// A null buffer means that "0" is bound to the requested buffer target
			return error(GL_INVALID_OPERATION);
		}

		*params = buffer->isMapped() ? (void*)(((const char*)buffer->data()) + buffer->offset()) : nullptr;
	}
}

GL_APICALL void GL_APIENTRY glDrawBuffers(GLsizei n, const GLenum *bufs)
{
	TRACE("(GLsizei n = %d, const GLenum *bufs = %p)", n, bufs);

	if(n < 0 || n > MAX_DRAW_BUFFERS)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		GLuint drawFramebufferName = context->getDrawFramebufferName();

		if((drawFramebufferName == 0) && (n != 1))
		{
			return error(GL_INVALID_OPERATION);
		}

		for(unsigned int i = 0; i < (unsigned)n; i++)
		{
			switch(bufs[i])
			{
			case GL_BACK:
				if(drawFramebufferName != 0)
				{
					return error(GL_INVALID_OPERATION);
				}
				break;
			case GL_NONE:
				break;
			case GL_COLOR_ATTACHMENT0:
			case GL_COLOR_ATTACHMENT1:
			case GL_COLOR_ATTACHMENT2:
			case GL_COLOR_ATTACHMENT3:
			case GL_COLOR_ATTACHMENT4:
			case GL_COLOR_ATTACHMENT5:
			case GL_COLOR_ATTACHMENT6:
			case GL_COLOR_ATTACHMENT7:
			case GL_COLOR_ATTACHMENT8:
			case GL_COLOR_ATTACHMENT9:
			case GL_COLOR_ATTACHMENT10:
			case GL_COLOR_ATTACHMENT11:
			case GL_COLOR_ATTACHMENT12:
			case GL_COLOR_ATTACHMENT13:
			case GL_COLOR_ATTACHMENT14:
			case GL_COLOR_ATTACHMENT15:
			case GL_COLOR_ATTACHMENT16:
			case GL_COLOR_ATTACHMENT17:
			case GL_COLOR_ATTACHMENT18:
			case GL_COLOR_ATTACHMENT19:
			case GL_COLOR_ATTACHMENT20:
			case GL_COLOR_ATTACHMENT21:
			case GL_COLOR_ATTACHMENT22:
			case GL_COLOR_ATTACHMENT23:
			case GL_COLOR_ATTACHMENT24:
			case GL_COLOR_ATTACHMENT25:
			case GL_COLOR_ATTACHMENT26:
			case GL_COLOR_ATTACHMENT27:
			case GL_COLOR_ATTACHMENT28:
			case GL_COLOR_ATTACHMENT29:
			case GL_COLOR_ATTACHMENT30:
			case GL_COLOR_ATTACHMENT31:
				{
					GLuint index = (bufs[i] - GL_COLOR_ATTACHMENT0);

					if(index >= MAX_COLOR_ATTACHMENTS)
					{
						return error(GL_INVALID_OPERATION);
					}

					if(index != i)
					{
						return error(GL_INVALID_OPERATION);
					}

					if(drawFramebufferName == 0)
					{
						return error(GL_INVALID_OPERATION);
					}
				}
				break;
			default:
				return error(GL_INVALID_ENUM);
			}
		}

		context->setFramebufferDrawBuffers(n, bufs);
	}
}

GL_APICALL void GL_APIENTRY glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat *value = %p)", location, count, transpose, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniformMatrix2x3fv(location, count, transpose, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat *value = %p)", location, count, transpose, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniformMatrix3x2fv(location, count, transpose, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat *value = %p)", location, count, transpose, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniformMatrix2x4fv(location, count, transpose, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat *value = %p)", location, count, transpose, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniformMatrix4x2fv(location, count, transpose, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat *value = %p)", location, count, transpose, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniformMatrix3x4fv(location, count, transpose, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat *value = %p)", location, count, transpose, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniformMatrix4x3fv(location, count, transpose, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
	TRACE("(GLint srcX0 = %d, GLint srcY0 = %d, GLint srcX1 = %d, GLint srcY1 = %d, "
	      "GLint dstX0 = %d, GLint dstY0 = %d, GLint dstX1 = %d, GLint dstY1 = %d, "
	      "GLbitfield mask = 0x%X, GLenum filter = 0x%X)",
	      srcX0, srcY0, srcX1, srcX1, dstX0, dstY0, dstX1, dstY1, mask, filter);

	switch(filter)
	{
	case GL_NEAREST:
		break;
	case GL_LINEAR:
		if((mask & GL_DEPTH_BUFFER_BIT) || (mask & GL_STENCIL_BUFFER_BIT))
		{
			return error(GL_INVALID_OPERATION);
		}
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if((mask & ~(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(context->getReadFramebufferName() == context->getDrawFramebufferName())
		{
			ERR("Blits with the same source and destination framebuffer are not supported by this implementation.");
			return error(GL_INVALID_OPERATION);
		}

		context->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter == GL_LINEAR, true);
	}
}

GL_APICALL void GL_APIENTRY glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
	TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLuint texture = %d, GLint level = %d, GLint layer = %d)",
	      target, attachment, texture, level, layer);

	// GLES 3.0.4 spec, p.209, section 4.4.2
	// If texture is zero, any image or array of images attached to the attachment point
	// named by attachment is detached. Any additional parameters(level, textarget,
	// and / or layer) are ignored when texture is zero.
	if(texture != 0 && (layer < 0 || level < 0))
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		Texture* textureObject = context->getTexture(texture);
		GLenum textarget = GL_NONE;
		if(texture != 0)
		{
			if(!textureObject)
			{
				return error(GL_INVALID_OPERATION);
			}

			if(level >= es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
			{
				return error(GL_INVALID_VALUE);
			}

			textarget = textureObject->getTarget();
			switch(textarget)
			{
			case GL_TEXTURE_3D:
				if(layer >= es2::IMPLEMENTATION_MAX_3D_TEXTURE_SIZE)
				{
					return error(GL_INVALID_VALUE);
				}
				break;
			case GL_TEXTURE_2D_ARRAY:
				if(layer >= es2::IMPLEMENTATION_MAX_ARRAY_TEXTURE_LAYERS)
				{
					return error(GL_INVALID_VALUE);
				}
				break;
			default:
				return error(GL_INVALID_OPERATION);
			}

			if(textureObject->isCompressed(textarget, level))
			{
				return error(GL_INVALID_OPERATION);
			}
		}

		es2::Framebuffer *framebuffer = nullptr;
		switch(target)
		{
		case GL_DRAW_FRAMEBUFFER:
		case GL_FRAMEBUFFER:
			if(context->getDrawFramebufferName() == 0)
			{
				return error(GL_INVALID_OPERATION);
			}
			framebuffer = context->getDrawFramebuffer();
			break;
		case GL_READ_FRAMEBUFFER:
			if(context->getReadFramebufferName() == 0)
			{
				return error(GL_INVALID_OPERATION);
			}
			framebuffer = context->getReadFramebuffer();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		if(!framebuffer)
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(attachment)
		{
		case GL_COLOR_ATTACHMENT0:
		case GL_COLOR_ATTACHMENT1:
		case GL_COLOR_ATTACHMENT2:
		case GL_COLOR_ATTACHMENT3:
		case GL_COLOR_ATTACHMENT4:
		case GL_COLOR_ATTACHMENT5:
		case GL_COLOR_ATTACHMENT6:
		case GL_COLOR_ATTACHMENT7:
		case GL_COLOR_ATTACHMENT8:
		case GL_COLOR_ATTACHMENT9:
		case GL_COLOR_ATTACHMENT10:
		case GL_COLOR_ATTACHMENT11:
		case GL_COLOR_ATTACHMENT12:
		case GL_COLOR_ATTACHMENT13:
		case GL_COLOR_ATTACHMENT14:
		case GL_COLOR_ATTACHMENT15:
		case GL_COLOR_ATTACHMENT16:
		case GL_COLOR_ATTACHMENT17:
		case GL_COLOR_ATTACHMENT18:
		case GL_COLOR_ATTACHMENT19:
		case GL_COLOR_ATTACHMENT20:
		case GL_COLOR_ATTACHMENT21:
		case GL_COLOR_ATTACHMENT22:
		case GL_COLOR_ATTACHMENT23:
		case GL_COLOR_ATTACHMENT24:
		case GL_COLOR_ATTACHMENT25:
		case GL_COLOR_ATTACHMENT26:
		case GL_COLOR_ATTACHMENT27:
		case GL_COLOR_ATTACHMENT28:
		case GL_COLOR_ATTACHMENT29:
		case GL_COLOR_ATTACHMENT30:
		case GL_COLOR_ATTACHMENT31:
			framebuffer->setColorbuffer(textarget, texture, attachment - GL_COLOR_ATTACHMENT0, level, layer);
			break;
		case GL_DEPTH_ATTACHMENT:
			framebuffer->setDepthbuffer(textarget, texture, level, layer);
			break;
		case GL_STENCIL_ATTACHMENT:
			framebuffer->setStencilbuffer(textarget, texture, level, layer);
			break;
		case GL_DEPTH_STENCIL_ATTACHMENT:
			framebuffer->setDepthbuffer(textarget, texture, level, layer);
			framebuffer->setStencilbuffer(textarget, texture, level, layer);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void *GL_APIENTRY glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
	TRACE("(GLenum target = 0x%X,  GLintptr offset = %d, GLsizeiptr length = %d, GLbitfield access = %X)",
	      target, offset, length, access);

	if((offset < 0) || (length < 0))
	{
		return error(GL_INVALID_VALUE, nullptr);
	}

	if(!(access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)))
	{
		// Must be able to read or write the buffer
		return error(GL_INVALID_OPERATION, nullptr);
	}
	else if((access & GL_MAP_READ_BIT) && (access & (GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT)))
	{
		// GL_MAP_INVALIDATE_RANGE_BIT, GL_MAP_INVALIDATE_BUFFER_BIT and GL_MAP_UNSYNCHRONIZED_BIT can't be used with GL_MAP_READ_BIT
		return error(GL_INVALID_OPERATION, nullptr);
	}
	else if((!(access & GL_MAP_WRITE_BIT)) && (access & GL_MAP_FLUSH_EXPLICIT_BIT))
	{
		// GL_MAP_FLUSH_EXPLICIT_BIT can't be used without GL_MAP_WRITE_BIT
		return error(GL_INVALID_OPERATION, nullptr);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Buffer *buffer = nullptr;
		if(!context->getBuffer(target, &buffer))
		{
			return error(GL_INVALID_ENUM, nullptr);
		}

		if(!buffer)
		{
			// A null buffer means that "0" is bound to the requested buffer target
			return error(GL_INVALID_OPERATION, nullptr);
		}

		if(buffer->isMapped())
		{
			// It is an invalid operation to map an already mapped buffer
			return error(GL_INVALID_OPERATION, nullptr);
		}

		GLsizeiptr bufferSize = buffer->size();
		if((offset + length) > bufferSize)
		{
			return error(GL_INVALID_VALUE, nullptr);
		}

		if((access & ~(GL_MAP_READ_BIT |
		               GL_MAP_WRITE_BIT |
		               GL_MAP_INVALIDATE_RANGE_BIT |
		               GL_MAP_INVALIDATE_BUFFER_BIT |
		               GL_MAP_FLUSH_EXPLICIT_BIT |
		               GL_MAP_UNSYNCHRONIZED_BIT)) != 0)
		{
			return error(GL_INVALID_VALUE, nullptr);
		}

		return buffer->mapRange(offset, length, access);
	}

	return nullptr;
}

GL_APICALL void GL_APIENTRY glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
	TRACE("(GLenum target = 0x%X,  GLintptr offset = %d, GLsizeiptr length = %d)",
	      target, offset, length);

	if((offset < 0) || (length < 0))
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Buffer *buffer = nullptr;
		if(!context->getBuffer(target, &buffer))
		{
			return error(GL_INVALID_ENUM);
		}

		if(!buffer)
		{
			// A null buffer means that "0" is bound to the requested buffer target
			return error(GL_INVALID_OPERATION);
		}

		if(!buffer->isMapped())
		{
			// Buffer must be mapped
			return error(GL_INVALID_OPERATION);
		}

		GLsizeiptr bufferSize = buffer->length();
		if((offset + length) > bufferSize)
		{
			return error(GL_INVALID_VALUE);
		}

		if(!(buffer->access() & GL_MAP_FLUSH_EXPLICIT_BIT))
		{
			// Flush must be explicitly allowed
			return error(GL_INVALID_OPERATION);
		}

		buffer->flushMappedRange(offset, length);
	}
}

GL_APICALL void GL_APIENTRY glBindVertexArray(GLuint array)
{
	TRACE("(GLuint array = %d)", array);

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!context->isVertexArray(array))
		{
			return error(GL_INVALID_OPERATION);
		}

		context->bindVertexArray(array);
	}
}

GL_APICALL void GL_APIENTRY glBindVertexArrayOES(GLuint array)
{
	glBindVertexArray(array);
}

GL_APICALL void GL_APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint *arrays)
{
	TRACE("(GLsizei n = %d, const GLuint *arrays = %p)", n, arrays);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			context->deleteVertexArray(arrays[i]);
		}
	}
}

GL_APICALL void GL_APIENTRY glDeleteVertexArraysOES(GLsizei n, const GLuint *arrays)
{
	glDeleteVertexArrays(n, arrays);
}

GL_APICALL void GL_APIENTRY glGenVertexArrays(GLsizei n, GLuint *arrays)
{
	TRACE("(GLsizei n = %d, const GLuint *arrays = %p)", n, arrays);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			arrays[i] = context->createVertexArray();
		}
	}
}

GL_APICALL void GL_APIENTRY glGenVertexArraysOES(GLsizei n, GLuint *arrays)
{
	glGenVertexArrays(n, arrays);
}

GL_APICALL GLboolean GL_APIENTRY glIsVertexArray(GLuint array)
{
	TRACE("(GLuint array = %d)", array);

	if(array == 0)
	{
		return GL_FALSE;
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::VertexArray *arrayObject = context->getVertexArray(array);

		if(arrayObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GL_APICALL GLboolean GL_APIENTRY glIsVertexArrayOES(GLuint array)
{
	return glIsVertexArray(array);
}

GL_APICALL void GL_APIENTRY glGetIntegeri_v(GLenum target, GLuint index, GLint *data)
{
	TRACE("(GLenum target = 0x%X, GLuint index = %d, GLint* data = %p)",
	      target, index, data);

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!context->getTransformFeedbackiv(index, target, data) &&
		   !context->getUniformBufferiv(index, target, data) &&
		   !context->getIntegerv(target, data))
		{
			GLenum nativeType;
			unsigned int numParams = 0;
			if(!context->getQueryParameterInfo(target, &nativeType, &numParams))
				return error(GL_INVALID_ENUM);

			if(numParams == 0)
				return; // it is known that target is valid, but there are no parameters to return

			if(nativeType == GL_BOOL)
			{
				GLboolean *boolParams = nullptr;
				boolParams = new GLboolean[numParams];

				context->getBooleanv(target, boolParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					data[i] = (boolParams[i] == GL_FALSE) ? 0 : 1;
				}

				delete[] boolParams;
			}
			else if(nativeType == GL_FLOAT)
			{
				GLfloat *floatParams = nullptr;
				floatParams = new GLfloat[numParams];

				context->getFloatv(target, floatParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					if(target == GL_DEPTH_RANGE || target == GL_COLOR_CLEAR_VALUE || target == GL_DEPTH_CLEAR_VALUE || target == GL_BLEND_COLOR)
					{
						data[i] = convert_float_fixed(floatParams[i]);
					}
					else
					{
						data[i] = (GLint)(floatParams[i] > 0.0f ? floor(floatParams[i] + 0.5) : ceil(floatParams[i] - 0.5));
					}
				}

				delete[] floatParams;
			}
		}
	}
}

GL_APICALL void GL_APIENTRY glBeginTransformFeedback(GLenum primitiveMode)
{
	TRACE("(GLenum primitiveMode = 0x%X)", primitiveMode);

	switch(primitiveMode)
	{
	case GL_POINTS:
	case GL_LINES:
	case GL_TRIANGLES:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback *transformFeedbackObject = context->getTransformFeedback();

		if(transformFeedbackObject)
		{
			if(transformFeedbackObject->isActive())
			{
				return error(GL_INVALID_OPERATION);
			}
			transformFeedbackObject->begin(primitiveMode);
		}
		else
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glEndTransformFeedback(void)
{
	TRACE("()");

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback *transformFeedbackObject = context->getTransformFeedback();

		if(transformFeedbackObject)
		{
			if(!transformFeedbackObject->isActive())
			{
				return error(GL_INVALID_OPERATION);
			}
			transformFeedbackObject->end();
		}
		else
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
	TRACE("(GLenum target = 0x%X, GLuint index = %d, GLuint buffer = %d, GLintptr offset = %d, GLsizeiptr size = %d)",
	      target, index, buffer, offset, size);

	if(buffer != 0 && size <= 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_TRANSFORM_FEEDBACK_BUFFER:
			if(index >= MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS)
			{
				return error(GL_INVALID_VALUE);
			}
			if(size & 0x3 || offset & 0x3) // size and offset must be multiples of 4
			{
				return error(GL_INVALID_VALUE);
			}
			context->bindIndexedTransformFeedbackBuffer(buffer, index, offset, size);
			context->bindGenericTransformFeedbackBuffer(buffer);
			break;
		case GL_UNIFORM_BUFFER:
			if(index >= MAX_UNIFORM_BUFFER_BINDINGS)
			{
				return error(GL_INVALID_VALUE);
			}
			if(offset % UNIFORM_BUFFER_OFFSET_ALIGNMENT != 0)
			{
				return error(GL_INVALID_VALUE);
			}
			context->bindIndexedUniformBuffer(buffer, index, offset, size);
			context->bindGenericUniformBuffer(buffer);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glBindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
	TRACE("(GLenum target = 0x%X, GLuint index = %d, GLuint buffer = %d)",
	      target, index, buffer);

	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_TRANSFORM_FEEDBACK_BUFFER:
			if(index >= MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS)
			{
				return error(GL_INVALID_VALUE);
			}
			context->bindIndexedTransformFeedbackBuffer(buffer, index, 0, 0);
			context->bindGenericTransformFeedbackBuffer(buffer);
			break;
		case GL_UNIFORM_BUFFER:
			if(index >= MAX_UNIFORM_BUFFER_BINDINGS)
			{
				return error(GL_INVALID_VALUE);
			}
			context->bindIndexedUniformBuffer(buffer, index, 0, 0);
			context->bindGenericUniformBuffer(buffer);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode)
{
	TRACE("(GLuint program = %d, GLsizei count = %d, const GLchar *const*varyings = %p, GLenum bufferMode = 0x%X)",
	      program, count, varyings, bufferMode);

	switch(bufferMode)
	{
	case GL_SEPARATE_ATTRIBS:
		if(count > MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS)
		{
			return error(GL_INVALID_VALUE);
		}
	case GL_INTERLEAVED_ATTRIBS:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_VALUE);
		}

		programObject->setTransformFeedbackVaryings(count, varyings, bufferMode);
	}
}

GL_APICALL void GL_APIENTRY glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name)
{
	TRACE("(GLuint program = %d, GLuint index = %d, GLsizei bufSize = %d, GLsizei *length = %p, GLsizei *size = %p, GLenum *type = %p, GLchar *name = %p)",
	      program, index, bufSize, length, size, type, name);

	if(bufSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_VALUE);
		}

		if(index >= static_cast<GLuint>(programObject->getTransformFeedbackVaryingCount()))
		{
			return error(GL_INVALID_VALUE);
		}

		programObject->getTransformFeedbackVarying(index, bufSize, length, size, type, name);
	}
}

GL_APICALL void GL_APIENTRY glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer)
{
	TRACE("(GLuint program = %d, GLuint index = %d, GLsizei bufSize = %d, GLsizei *length = %p, GLsizei *size = %p, GLenum *type = %p, GLchar *name = %p)",
	      index, size, type, stride, pointer);

	if(index >= es2::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	if(size < 1 || size > 4 || stride < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	switch(type)
	{
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_INT:
	case GL_UNSIGNED_INT:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::VertexArray* vertexArray = context->getCurrentVertexArray();
		if((context->getArrayBufferName() == 0) && vertexArray && (vertexArray->name != 0) && pointer)
		{
			// GL_INVALID_OPERATION is generated if a non-zero vertex array object is bound, zero is bound
			// to the GL_ARRAY_BUFFER buffer object binding point and the pointer argument is not NULL.
			return error(GL_INVALID_OPERATION);
		}

		context->setVertexAttribState(index, context->getArrayBuffer(), size, type, false, true, stride, pointer);
	}
}

GL_APICALL void GL_APIENTRY glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params)
{
	TRACE("(GLuint index = %d, GLenum pname = 0x%X, GLint *params = %p)",
	      index, pname, params);

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(index >= es2::MAX_VERTEX_ATTRIBS)
		{
			return error(GL_INVALID_VALUE);
		}

		const es2::VertexAttribute &attribState = context->getVertexAttribState(index);

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
			{
				const VertexAttribute& attrib = context->getCurrentVertexAttributes()[index];
				for(int i = 0; i < 4; ++i)
				{
					params[i] = attrib.getCurrentValueI(i);
				}
			}
			break;
		case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
			*params = (attribState.mPureInteger ? GL_TRUE : GL_FALSE);
			break;
		case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
			*params = attribState.mDivisor;
			break;
		default: return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params)
{
	TRACE("(GLuint index = %d, GLenum pname = 0x%X, GLuint *params = %p)",
		index, pname, params);

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(index >= es2::MAX_VERTEX_ATTRIBS)
		{
			return error(GL_INVALID_VALUE);
		}

		const es2::VertexAttribute &attribState = context->getVertexAttribState(index);

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
			{
				const VertexAttribute& attrib = context->getCurrentVertexAttributes()[index];
				for(int i = 0; i < 4; ++i)
				{
					params[i] = attrib.getCurrentValueUI(i);
				}
			}
			break;
		case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
			*params = (attribState.mPureInteger ? GL_TRUE : GL_FALSE);
			break;
		case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
			*params = attribState.mDivisor;
			break;
		default: return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w)
{
	TRACE("(GLuint index = %d, GLint x = %d, GLint y = %d, GLint z = %d, GLint w = %d)",
	      index, x, y, z, w);

	if(index >= es2::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		GLint vals[4] = { x, y, z, w };
		context->setVertexAttrib(index, vals);
	}
}

GL_APICALL void GL_APIENTRY glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
	TRACE("(GLuint index = %d, GLint x = %d, GLint y = %d, GLint z = %d, GLint w = %d)",
	      index, x, y, z, w);

	if(index >= es2::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		GLuint vals[4] = { x, y, z, w };
		context->setVertexAttrib(index, vals);
	}
}

GL_APICALL void GL_APIENTRY glVertexAttribI4iv(GLuint index, const GLint *v)
{
	TRACE("(GLuint index = %d, GLint *v = %p)", index, v);

	if(index >= es2::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		context->setVertexAttrib(index, v);
	}
}

GL_APICALL void GL_APIENTRY glVertexAttribI4uiv(GLuint index, const GLuint *v)
{
	TRACE("(GLuint index = %d, GLint *v = %p)", index, v);

	if(index >= es2::MAX_VERTEX_ATTRIBS)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		context->setVertexAttrib(index, v);
	}
}

GL_APICALL void GL_APIENTRY glGetUniformuiv(GLuint program, GLint location, GLuint *params)
{
	TRACE("(GLuint program = %d, GLint location = %d, GLuint *params = %p)",
	      program, location, params);

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

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

		if(!programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!programObject->getUniformuiv(location, nullptr, params))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL GLint GL_APIENTRY glGetFragDataLocation(GLuint program, const GLchar *name)
{
	TRACE("(GLuint program = %d, const GLchar *name = %p)", program, name);

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

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

		return programObject->getFragDataLocation(name);
	}

	return -1;
}

GL_APICALL void GL_APIENTRY glUniform1ui(GLint location, GLuint v0)
{
	glUniform1uiv(location, 1, &v0);
}

GL_APICALL void GL_APIENTRY glUniform2ui(GLint location, GLuint v0, GLuint v1)
{
	GLuint xy[2] = { v0, v1 };

	glUniform2uiv(location, 1, (GLuint*)&xy);
}

GL_APICALL void GL_APIENTRY glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
{
	GLuint xyz[3] = { v0, v1, v2 };

	glUniform3uiv(location, 1, (GLuint*)&xyz);
}

GL_APICALL void GL_APIENTRY glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
	GLuint xyzw[4] = { v0, v1, v2, v3 };

	glUniform4uiv(location, 1, (GLuint*)&xyzw);
}

GL_APICALL void GL_APIENTRY glUniform1uiv(GLint location, GLsizei count, const GLuint *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLuint *value = %p)",
	      location, count, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniform1uiv(location, count, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glUniform2uiv(GLint location, GLsizei count, const GLuint *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLuint *value = %p)",
	      location, count, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniform2uiv(location, count, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glUniform3uiv(GLint location, GLsizei count, const GLuint *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLuint *value = %p)",
	      location, count, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniform3uiv(location, count, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glUniform4uiv(GLint location, GLsizei count, const GLuint *value)
{
	TRACE("(GLint location = %d, GLsizei count = %d, const GLuint *value = %p)",
	      location, count, value);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *program = context->getCurrentProgram();

		if(!program)
		{
			return error(GL_INVALID_OPERATION);
		}

		if(location == -1)
		{
			return;
		}

		if(!program->setUniform4uiv(location, count, value))
		{
			return error(GL_INVALID_OPERATION);
		}
	}
}

GL_APICALL void GL_APIENTRY glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value)
{
	TRACE("(GLenum buffer = 0x%X, GLint drawbuffer = %d, const GLint *value = %p)",
	      buffer, drawbuffer, value);

	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(buffer)
		{
		case GL_COLOR:
			if(drawbuffer < 0 || drawbuffer >= MAX_DRAW_BUFFERS)
			{
				return error(GL_INVALID_VALUE);
			}
			else
			{
				context->clearColorBuffer(drawbuffer, value);
			}
			break;
		case GL_STENCIL:
			if(drawbuffer != 0)
			{
				return error(GL_INVALID_VALUE);
			}
			else
			{
				context->clearStencilBuffer(value[0]);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value)
{
	TRACE("(GLenum buffer = 0x%X, GLint drawbuffer = %d, const GLuint *value = %p)",
	      buffer, drawbuffer, value);

	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(buffer)
		{
		case GL_COLOR:
			if(drawbuffer < 0 || drawbuffer >= MAX_DRAW_BUFFERS)
			{
				return error(GL_INVALID_VALUE);
			}
			else
			{
				context->clearColorBuffer(drawbuffer, value);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value)
{
	TRACE("(GLenum buffer = 0x%X, GLint drawbuffer = %d, const GLfloat *value = %p)",
	      buffer, drawbuffer, value);

	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(buffer)
		{
		case GL_COLOR:
			if(drawbuffer < 0 || drawbuffer >= MAX_DRAW_BUFFERS)
			{
				return error(GL_INVALID_VALUE);
			}
			else
			{
				context->clearColorBuffer(drawbuffer, value);
			}
			break;
		case GL_DEPTH:
			if(drawbuffer != 0)
			{
				return error(GL_INVALID_VALUE);
			}
			else
			{
				context->clearDepthBuffer(value[0]);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
	TRACE("(GLenum buffer = 0x%X, GLint drawbuffer = %d, GLfloat depth = %f, GLint stencil = %d)",
	      buffer, drawbuffer, depth, stencil);

	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(buffer)
		{
		case GL_DEPTH_STENCIL:
			if(drawbuffer != 0)
			{
				return error(GL_INVALID_VALUE);
			}
			else
			{
				context->clearDepthBuffer(depth);
				context->clearStencilBuffer(stencil);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL const GLubyte *GL_APIENTRY glGetStringi(GLenum name, GLuint index)
{
	TRACE("(GLenum name = 0x%X, GLuint index = %d)", name, index);

	es2::Context *context = es2::getContext();
	if(context)
	{
		GLuint numExtensions;
		context->getExtensions(0, &numExtensions);

		if(index >= numExtensions)
		{
			return error(GL_INVALID_VALUE, (GLubyte*)nullptr);
		}

		switch(name)
		{
		case GL_EXTENSIONS:
			return context->getExtensions(index);
		default:
			return error(GL_INVALID_ENUM, (GLubyte*)nullptr);
		}
	}

	return (GLubyte*)nullptr;
}

GL_APICALL void GL_APIENTRY glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
	TRACE("(GLenum readTarget = 0x%X, GLenum writeTarget = 0x%X,  GLintptr readOffset = %d, GLintptr writeOffset = %d, GLsizeiptr size = %d)",
	      readTarget, writeTarget, readOffset, writeOffset, size);

	if(readOffset < 0 || writeOffset < 0 || size < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Buffer *readBuffer = nullptr, *writeBuffer = nullptr;
		if(!context->getBuffer(readTarget, &readBuffer) || !context->getBuffer(writeTarget, &writeBuffer))
		{
			return error(GL_INVALID_ENUM);
		}
		if(!readBuffer || readBuffer->isMapped() || !writeBuffer || writeBuffer->isMapped())
		{
			return error(GL_INVALID_OPERATION);
		}
		if(readBuffer == writeBuffer)
		{
			// If same buffer, check for overlap
			if(((readOffset >= writeOffset) && (readOffset < (writeOffset + size))) ||
			   ((writeOffset >= readOffset) && (writeOffset < (readOffset + size))))
			{
				return error(GL_INVALID_VALUE);
			}
		}

		if((static_cast<size_t>(readOffset + size) > readBuffer->size()) ||
		   (static_cast<size_t>(writeOffset + size) > writeBuffer->size()))
		{
			return error(GL_INVALID_VALUE);
		}

		writeBuffer->bufferSubData(((char*)readBuffer->data()) + readOffset, size, writeOffset);
	}
}

GL_APICALL void GL_APIENTRY glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices)
{
	TRACE("(GLuint program = %d, GLsizei uniformCount = %d, const GLchar *const*uniformNames = %p, GLuint *uniformIndices = %p)",
	      program, uniformCount, uniformNames, uniformIndices);

	if(uniformCount < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

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

		if(!programObject->isLinked())
		{
			for(int uniformId = 0; uniformId < uniformCount; uniformId++)
			{
				uniformIndices[uniformId] = GL_INVALID_INDEX;
			}
		}
		else
		{
			for(int uniformId = 0; uniformId < uniformCount; uniformId++)
			{
				uniformIndices[uniformId] = programObject->getUniformIndex(uniformNames[uniformId]);
			}
		}
	}
}

GL_APICALL void GL_APIENTRY glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params)
{
	TRACE("(GLuint program = %d, GLsizei uniformCount = %d, const GLchar *const*uniformNames = %p, GLenum pname = 0x%X, GLuint *uniformIndices = %p)",
	      program, uniformCount, uniformIndices, pname, uniformIndices);

	switch(pname)
	{
	case GL_UNIFORM_TYPE:
	case GL_UNIFORM_SIZE:
	case GL_UNIFORM_NAME_LENGTH:
	case GL_UNIFORM_BLOCK_INDEX:
	case GL_UNIFORM_OFFSET:
	case GL_UNIFORM_ARRAY_STRIDE:
	case GL_UNIFORM_MATRIX_STRIDE:
	case GL_UNIFORM_IS_ROW_MAJOR:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(uniformCount < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

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

		for(int uniformId = 0; uniformId < uniformCount; uniformId++)
		{
			const GLuint index = uniformIndices[uniformId];

			if(index >= programObject->getActiveUniformCount())
			{
				return error(GL_INVALID_VALUE);
			}
		}

		for(int uniformId = 0; uniformId < uniformCount; uniformId++)
		{
			const GLuint index = uniformIndices[uniformId];
			params[uniformId] = programObject->getActiveUniformi(index, pname);
		}
	}
}

GL_APICALL GLuint GL_APIENTRY glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName)
{
	TRACE("(GLuint program = %d, const GLchar *uniformBlockName = %p)",
	      program, uniformBlockName);

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			if(context->getShader(program))
			{
				return error(GL_INVALID_OPERATION, GL_INVALID_INDEX);
			}
			else
			{
				return error(GL_INVALID_VALUE, GL_INVALID_INDEX);
			}
		}

		return programObject->getUniformBlockIndex(uniformBlockName);
	}

	return GL_INVALID_INDEX;
}

GL_APICALL void GL_APIENTRY glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params)
{
	TRACE("(GLuint program = %d, GLuint uniformBlockIndex = %d, GLenum pname = 0x%X, GLint *params = %p)",
	      program, uniformBlockIndex, pname, params);

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		switch(pname)
		{
		case GL_UNIFORM_BLOCK_BINDING:
			*params = static_cast<GLint>(programObject->getUniformBlockBinding(uniformBlockIndex));
			break;
		case GL_UNIFORM_BLOCK_DATA_SIZE:
		case GL_UNIFORM_BLOCK_NAME_LENGTH:
		case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
		case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
		case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
		case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
			programObject->getActiveUniformBlockiv(uniformBlockIndex, pname, params);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName)
{
	TRACE("(GLuint program = %d, GLuint uniformBlockIndex = %d, GLsizei bufSize = %d, GLsizei *length = %p, GLchar *uniformBlockName = %p)",
	      program, uniformBlockIndex, bufSize, length, uniformBlockName);

	if(bufSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_OPERATION);
		}

		programObject->getActiveUniformBlockName(uniformBlockIndex, bufSize, length, uniformBlockName);
	}
}

GL_APICALL void GL_APIENTRY glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
	TRACE("(GLuint program = %d, GLuint uniformBlockIndex = %d, GLuint uniformBlockBinding = %d)",
	      program, uniformBlockIndex, uniformBlockBinding);

	if(uniformBlockBinding >= MAX_UNIFORM_BUFFER_BINDINGS)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_VALUE);
		}

		programObject->bindUniformBlock(uniformBlockIndex, uniformBlockBinding);
	}
}

GL_APICALL void GL_APIENTRY glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount)
{
	TRACE("(GLenum mode = 0x%X, GLint first = %d, GLsizei count = %d, GLsizei instanceCount = %d)",
	      mode, first, count, instanceCount);

	switch(mode)
	{
	case GL_POINTS:
	case GL_LINES:
	case GL_LINE_LOOP:
	case GL_LINE_STRIP:
	case GL_TRIANGLES:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLE_STRIP:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	if(count < 0 || instanceCount < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback* transformFeedback = context->getTransformFeedback();
		if(transformFeedback && transformFeedback->isActive() && (mode != transformFeedback->primitiveMode()))
		{
			return error(GL_INVALID_OPERATION);
		}

		context->drawArrays(mode, first, count, instanceCount);
	}
}

GL_APICALL void GL_APIENTRY glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount)
{
	TRACE("(GLenum mode = 0x%X, GLsizei count = %d, GLenum type = 0x%X, const void *indices = %p, GLsizei instanceCount = %d)",
	      mode, count, type, indices, instanceCount);

	switch(mode)
	{
	case GL_POINTS:
	case GL_LINES:
	case GL_LINE_LOOP:
	case GL_LINE_STRIP:
	case GL_TRIANGLES:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLE_STRIP:
		break;
	default:
		return error(GL_INVALID_ENUM);
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

	if(count < 0 || instanceCount < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback* transformFeedback = context->getTransformFeedback();
		if(transformFeedback && transformFeedback->isActive() && !transformFeedback->isPaused())
		{
			return error(GL_INVALID_OPERATION);
		}

		context->drawElements(mode, 0, MAX_ELEMENT_INDEX, count, type, indices, instanceCount);
	}
}

GL_APICALL GLsync GL_APIENTRY glFenceSync(GLenum condition, GLbitfield flags)
{
	TRACE("(GLenum condition = 0x%X, GLbitfield flags = %X)", condition, flags);

	switch(condition)
	{
	case GL_SYNC_GPU_COMMANDS_COMPLETE:
		break;
	default:
		return error(GL_INVALID_ENUM, nullptr);
	}

	if(flags != 0)
	{
		return error(GL_INVALID_VALUE, nullptr);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		return context->createFenceSync(condition, flags);
	}

	return nullptr;
}

GL_APICALL GLboolean GL_APIENTRY glIsSync(GLsync sync)
{
	TRACE("(GLsync sync = %p)", sync);

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::FenceSync *fenceSyncObject = context->getFenceSync(sync);

		if(fenceSyncObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GL_APICALL void GL_APIENTRY glDeleteSync(GLsync sync)
{
	TRACE("(GLsync sync = %p)", sync);

	if(!sync)
	{
		return;
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!context->getFenceSync(sync))
		{
			return error(GL_INVALID_VALUE);
		}

		context->deleteFenceSync(sync);
	}
}

GL_APICALL GLenum GL_APIENTRY glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
	TRACE("(GLsync sync = %p, GLbitfield flags = %X, GLuint64 timeout = %llu)", sync, flags, timeout);

	if((flags & ~(GL_SYNC_FLUSH_COMMANDS_BIT)) != 0)
	{
		return error(GL_INVALID_VALUE, GL_FALSE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::FenceSync *fenceSyncObject = context->getFenceSync(sync);

		if(fenceSyncObject)
		{
			return fenceSyncObject->clientWait(flags, timeout);
		}
		else
		{
			return error(GL_INVALID_VALUE, GL_FALSE);
		}
	}

	return GL_FALSE;
}

GL_APICALL void GL_APIENTRY glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
	TRACE("(GLsync sync = %p, GLbitfield flags = %X, GLuint64 timeout = %llu)", sync, flags, timeout);

	if(flags != 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(timeout != GL_TIMEOUT_IGNORED)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::FenceSync *fenceSyncObject = context->getFenceSync(sync);

		if(fenceSyncObject)
		{
			fenceSyncObject->serverWait(flags, timeout);
		}
		else
		{
			return error(GL_INVALID_VALUE);
		}
	}
}

GL_APICALL void GL_APIENTRY glGetInteger64v(GLenum pname, GLint64 *data)
{
	TRACE("(GLenum pname = 0x%X, GLint64 *data = %p)", pname, data);

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!(context->getIntegerv(pname, data)))
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
					data[i] = (boolParams[i] == GL_FALSE) ? 0 : 1;
				}

				delete[] boolParams;
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
						data[i] = (GLint64)(convert_float_fixed(floatParams[i]));
					}
					else
					{
						data[i] = (GLint64)(floatParams[i] > 0.0f ? floor(floatParams[i] + 0.5) : ceil(floatParams[i] - 0.5));
					}
				}

				delete[] floatParams;
			}
		}
	}
}

GL_APICALL void GL_APIENTRY glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values)
{
	TRACE("(GLsync sync = %p, GLenum pname = 0x%X, GLsizei bufSize = %d, GLsizei *length = %p, GLint *values = %p)",
	      sync, pname, bufSize, length, values);

	if(bufSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::FenceSync *fenceSyncObject = context->getFenceSync(sync);
		if(!fenceSyncObject)
		{
			return error(GL_INVALID_VALUE);
		}

		fenceSyncObject->getSynciv(pname, length, values);
	}
}

GL_APICALL void GL_APIENTRY glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data)
{
	TRACE("(GLenum target = 0x%X, GLuint index = %d, GLint64 *data = %p)", target, index, data);

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!context->getTransformFeedbackiv(index, target, data) &&
		   !context->getUniformBufferiv(index, target, data) &&
		   !context->getIntegerv(target, data))
		{
			GLenum nativeType;
			unsigned int numParams = 0;
			if(!context->getQueryParameterInfo(target, &nativeType, &numParams))
				return error(GL_INVALID_ENUM);

			if(numParams == 0)
				return; // it is known that target is valid, but there are no parameters to return

			if(nativeType == GL_BOOL)
			{
				GLboolean *boolParams = nullptr;
				boolParams = new GLboolean[numParams];

				context->getBooleanv(target, boolParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					data[i] = (boolParams[i] == GL_FALSE) ? 0 : 1;
				}

				delete[] boolParams;
			}
			else if(nativeType == GL_FLOAT)
			{
				GLfloat *floatParams = nullptr;
				floatParams = new GLfloat[numParams];

				context->getFloatv(target, floatParams);

				for(unsigned int i = 0; i < numParams; ++i)
				{
					if(target == GL_DEPTH_RANGE || target == GL_COLOR_CLEAR_VALUE || target == GL_DEPTH_CLEAR_VALUE || target == GL_BLEND_COLOR)
					{
						data[i] = (GLint64)(convert_float_fixed(floatParams[i]));
					}
					else
					{
						data[i] = (GLint64)(floatParams[i] > 0.0f ? floor(floatParams[i] + 0.5) : ceil(floatParams[i] - 0.5));
					}
				}

				delete[] floatParams;
			}
		}
	}
}

GL_APICALL void GL_APIENTRY glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params)
{
	TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint64 *params = %p)", target, pname, params);

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Buffer *buffer = nullptr;

		if(!context->getBuffer(target, &buffer))
		{
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
		case GL_BUFFER_ACCESS_FLAGS:
			*params = buffer->access();
			break;
		case GL_BUFFER_MAPPED:
			*params = buffer->isMapped();
			break;
		case GL_BUFFER_MAP_LENGTH:
			*params = buffer->length();
			break;
		case GL_BUFFER_MAP_OFFSET:
			*params = buffer->offset();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glGenSamplers(GLsizei count, GLuint *samplers)
{
	TRACE("(GLsizei count = %d, GLuint *samplers = %p)", count, samplers);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		for(int i = 0; i < count; i++)
		{
			samplers[i] = context->createSampler();
		}
	}
}

GL_APICALL void GL_APIENTRY glDeleteSamplers(GLsizei count, const GLuint *samplers)
{
	TRACE("(GLsizei count = %d, GLuint *samplers = %p)", count, samplers);

	if(count < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		for(int i = 0; i < count; i++)
		{
			context->deleteSampler(samplers[i]);
		}
	}
}

GL_APICALL GLboolean GL_APIENTRY glIsSampler(GLuint sampler)
{
	TRACE("(GLuint sampler = %d)", sampler);

	if(sampler == 0)
	{
		return GL_FALSE;
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(context->isSampler(sampler))
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GL_APICALL void GL_APIENTRY glBindSampler(GLuint unit, GLuint sampler)
{
	TRACE("(GLuint unit = %d, GLuint sampler = %d)", unit, sampler);

	if(unit >= es2::MAX_COMBINED_TEXTURE_IMAGE_UNITS)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(sampler != 0 && !context->isSampler(sampler))
		{
			return error(GL_INVALID_OPERATION);
		}

		context->bindSampler(unit, sampler);
	}
}

GL_APICALL void GL_APIENTRY glSamplerParameteri(GLuint sampler, GLenum pname, GLint param)
{
	TRACE("(GLuint sampler = %d, GLenum pname = 0x%X, GLint param = %d)",
	      sampler, pname, param);

	glSamplerParameteriv(sampler, pname, &param);
}

GL_APICALL void GL_APIENTRY glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param)
{
	TRACE("(GLuint sampler = %d, GLenum pname = 0x%X, const GLint *param = %p)",
	      sampler, pname, param);

	if(!ValidateSamplerObjectParameter(pname))
	{
		return error(GL_INVALID_ENUM);
	}

	if(!ValidateTexParamParameters(pname, *param))
	{
		return;
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!context->isSampler(sampler))
		{
			return error(GL_INVALID_OPERATION);
		}

		context->samplerParameteri(sampler, pname, *param);
	}
}

GL_APICALL void GL_APIENTRY glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
{
	TRACE("(GLuint sampler = %d, GLenum pname = 0x%X, GLfloat param = %f)",
	      sampler, pname, param);

	glSamplerParameterfv(sampler, pname, &param);
}

GL_APICALL void GL_APIENTRY glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param)
{
	TRACE("(GLuint sampler = %d, GLenum pname = 0x%X, const GLfloat *param = %p)",
	      sampler, pname, param);

	if(!ValidateSamplerObjectParameter(pname))
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!context->isSampler(sampler))
		{
			return error(GL_INVALID_OPERATION);
		}

		if(ValidateTexParamParameters(pname, static_cast<GLint>(roundf(*param))))
		{
			context->samplerParameterf(sampler, pname, *param);
		}
	}
}

GL_APICALL void GL_APIENTRY glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params)
{
	TRACE("(GLuint sampler = %d, GLenum pname = 0x%X, GLint *params = %p)",
	      sampler, pname, params);

	if(!ValidateSamplerObjectParameter(pname))
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!context->isSampler(sampler))
		{
			return error(GL_INVALID_OPERATION);
		}

		*params = context->getSamplerParameteri(sampler, pname);
	}
}

GL_APICALL void GL_APIENTRY glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params)
{
	TRACE("(GLuint sampler = %d, GLenum pname = 0x%X, GLfloat *params = %p)",
	      sampler, pname, params);

	if(!ValidateSamplerObjectParameter(pname))
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(!context->isSampler(sampler))
		{
			return error(GL_INVALID_OPERATION);
		}

		*params = context->getSamplerParameterf(sampler, pname);
	}
}

GL_APICALL void GL_APIENTRY glVertexAttribDivisor(GLuint index, GLuint divisor)
{
	TRACE("(GLuint index = %d, GLuint divisor = %d)", index, divisor);

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(index >= es2::MAX_VERTEX_ATTRIBS)
		{
			return error(GL_INVALID_VALUE);
		}

		context->setVertexAttribDivisor(index, divisor);
	}
}

GL_APICALL void GL_APIENTRY glBindTransformFeedback(GLenum target, GLuint id)
{
	TRACE("(GLenum target = 0x%X, GLuint id = %d)", target, id);

	if(target != GL_TRANSFORM_FEEDBACK)
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback *transformFeedbackObject = context->getTransformFeedback();

		if(transformFeedbackObject && transformFeedbackObject->isActive() && !transformFeedbackObject->isPaused())
		{
			return error(GL_INVALID_OPERATION);
		}

		if(!context->isTransformFeedback(id))
		{
			return error(GL_INVALID_OPERATION);
		}

		context->bindTransformFeedback(id);
	}
}

GL_APICALL void GL_APIENTRY glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids)
{
	TRACE("(GLsizei n = %d, const GLuint *ids = %p)", n, ids);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			if(ids[i] != 0)
			{
				es2::TransformFeedback *transformFeedbackObject = context->getTransformFeedback(ids[i]);

				if(transformFeedbackObject && transformFeedbackObject->isActive())
				{
					return error(GL_INVALID_OPERATION);
				}

				context->deleteTransformFeedback(ids[i]);
			}
		}
	}
}

GL_APICALL void GL_APIENTRY glGenTransformFeedbacks(GLsizei n, GLuint *ids)
{
	TRACE("(GLsizei n = %d, const GLuint *ids = %p)", n, ids);

	if(n < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		for(int i = 0; i < n; i++)
		{
			ids[i] = context->createTransformFeedback();
		}
	}
}

GL_APICALL GLboolean GL_APIENTRY glIsTransformFeedback(GLuint id)
{
	TRACE("(GLuint id = %d)", id);

	if(id == 0)
	{
		return GL_FALSE;
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback *transformFeedbackObject = context->getTransformFeedback(id);

		if(transformFeedbackObject)
		{
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}

GL_APICALL void GL_APIENTRY glPauseTransformFeedback(void)
{
	TRACE("()");

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback *transformFeedbackObject = context->getTransformFeedback();

		if(transformFeedbackObject)
		{
			if(!transformFeedbackObject->isActive() || transformFeedbackObject->isPaused())
			{
				return error(GL_INVALID_OPERATION);
			}
			transformFeedbackObject->setPaused(true);
		}
	}
}

GL_APICALL void GL_APIENTRY glResumeTransformFeedback(void)
{
	TRACE("()");

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::TransformFeedback *transformFeedbackObject = context->getTransformFeedback();

		if(transformFeedbackObject)
		{
			if(!transformFeedbackObject->isActive() || !transformFeedbackObject->isPaused())
			{
				return error(GL_INVALID_OPERATION);
			}
			transformFeedbackObject->setPaused(false);
		}
	}
}

GL_APICALL void GL_APIENTRY glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary)
{
	TRACE("(GLuint program = %d, GLsizei bufSize = %d, GLsizei *length = %p, GLenum *binaryFormat = %p, void *binary = %p)",
	      program, bufSize, length, binaryFormat, binary);

	if(bufSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject || !programObject->isLinked())
		{
			return error(GL_INVALID_OPERATION);
		}
	}

	// SwiftShader doesn't return a program binary and sets the program binay size to 0, so any attempt at getting one is invalid.
	return error(GL_INVALID_OPERATION);
}

GL_APICALL void GL_APIENTRY glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length)
{
	TRACE("(GLuint program = %d, GLenum binaryFormat = 0x%X, const void *binary = %p, GLsizei length = %d)",
	      program, binaryFormat, binaryFormat, length);

	if(length < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_OPERATION);
		}
	}

	// Regardless of what the binaryFormat is, it is unrecognized by SwiftShader, since it supports no format.
	return error(GL_INVALID_ENUM);
}

GL_APICALL void GL_APIENTRY glProgramParameteri(GLuint program, GLenum pname, GLint value)
{
	TRACE("(GLuint program = %d, GLenum pname = 0x%X, GLint value = %d)",
	      program, pname, value);

	es2::Context *context = es2::getContext();

	if(context)
	{
		es2::Program *programObject = context->getProgram(program);

		if(!programObject)
		{
			return error(GL_INVALID_VALUE);
		}

		switch(pname)
		{
		case GL_PROGRAM_BINARY_RETRIEVABLE_HINT:
			if((value != GL_TRUE) && (value != GL_FALSE))
			{
				return error(GL_INVALID_VALUE);
			}
			programObject->setBinaryRetrievable(value != GL_FALSE);
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments)
{
	TRACE("(GLenum target = 0x%X, GLsizei numAttachments = %d, const GLenum *attachments = %p)",
	      target, numAttachments, attachments);

	glInvalidateSubFramebuffer(target, numAttachments, attachments, 0, 0, std::numeric_limits<GLsizei>::max(), std::numeric_limits<GLsizei>::max());
}

GL_APICALL void GL_APIENTRY glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height)
{
	TRACE("(GLenum target = 0x%X, GLsizei numAttachments = %d, const GLenum *attachments = %p, GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)",
	      target, numAttachments, attachments, x, y, width, height);

	es2::Context *context = es2::getContext();

	if(context)
	{
		if(numAttachments < 0 || width < 0 || height < 0)
		{
			return error(GL_INVALID_VALUE);
		}

		es2::Framebuffer *framebuffer = nullptr;
		switch(target)
		{
		case GL_DRAW_FRAMEBUFFER:
		case GL_FRAMEBUFFER:
			framebuffer = context->getDrawFramebuffer();
			break;
		case GL_READ_FRAMEBUFFER:
			framebuffer = context->getReadFramebuffer();
			break;
		default:
			return error(GL_INVALID_ENUM);
		}

		if(framebuffer)
		{
			for(int i = 0; i < numAttachments; i++)
			{
				switch(attachments[i])
				{
				case GL_COLOR:
				case GL_DEPTH:
				case GL_STENCIL:
					if(!framebuffer->isDefaultFramebuffer())
					{
						return error(GL_INVALID_ENUM);
					}
					break;
				case GL_DEPTH_ATTACHMENT:
				case GL_STENCIL_ATTACHMENT:
				case GL_DEPTH_STENCIL_ATTACHMENT:
					break;
				default:
					if(attachments[i] >= GL_COLOR_ATTACHMENT0 &&
					   attachments[i] <= GL_COLOR_ATTACHMENT31)
					{
						if(attachments[i] - GL_COLOR_ATTACHMENT0 >= MAX_DRAW_BUFFERS)
						{
							return error(GL_INVALID_OPERATION);
						}
					}
					else
					{
						return error(GL_INVALID_ENUM);
					}
					break;
				}
			}
		}

		// UNIMPLEMENTED();   // It is valid for this function to be treated as a no-op
	}
}

GL_APICALL void GL_APIENTRY glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
	TRACE("(GLenum target = 0x%X, GLsizei levels = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d)",
	      target, levels, internalformat, width, height);

	if(width < 1 || height < 1 || levels < 1 || ((target == GL_TEXTURE_RECTANGLE_ARB) && (levels != 1)))
	{
		return error(GL_INVALID_VALUE);
	}

	if(levels > es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS || levels > (log2(std::max(width, height)) + 1))
	{
		return error(GL_INVALID_OPERATION);
	}

	bool isCompressed = IsCompressed(internalformat, egl::getClientVersion());
	if(!IsSizedInternalFormat(internalformat) && !isCompressed)
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_TEXTURE_RECTANGLE_ARB:
			if(isCompressed) // Rectangle textures cannot be compressed
			{
				return error(GL_INVALID_ENUM);
			}
			// Fall through to GL_TEXTURE_2D case.
		case GL_TEXTURE_2D:
			{
				if((width > es2::IMPLEMENTATION_MAX_TEXTURE_SIZE) ||
				   (height > es2::IMPLEMENTATION_MAX_TEXTURE_SIZE))
				{
					return error(GL_INVALID_VALUE);
				}

				es2::Texture2D *texture = context->getTexture2D(target);
				if(!texture || texture->name == 0 || texture->getImmutableFormat() == GL_TRUE)
				{
					return error(GL_INVALID_OPERATION);
				}

				for(int level = 0; level < levels; level++)
				{
					texture->setImage(level, width, height, internalformat, GL_NONE, GL_NONE, context->getUnpackParameters(), nullptr);
					width = std::max(1, (width / 2));
					height = std::max(1, (height / 2));
				}
				texture->makeImmutable(levels);
			}
			break;
		case GL_TEXTURE_CUBE_MAP:
			{
				if((width > es2::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE) ||
				   (height > es2::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE))
				{
					return error(GL_INVALID_VALUE);
				}

				es2::TextureCubeMap *texture = context->getTextureCubeMap();
				if(!texture || texture->name == 0 || texture->getImmutableFormat())
				{
					return error(GL_INVALID_OPERATION);
				}

				for(int level = 0; level < levels; level++)
				{
					for(int face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; face++)
					{
						texture->setImage(face, level, width, height, internalformat, GL_NONE, GL_NONE, context->getUnpackParameters(), nullptr);
					}
					width = std::max(1, (width / 2));
					height = std::max(1, (height / 2));
				}
				texture->makeImmutable(levels);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
	TRACE("(GLenum target = 0x%X, GLsizei levels = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d)",
	      target, levels, internalformat, width, height, depth);

	if(width < 1 || height < 1 || depth < 1 || levels < 1)
	{
		return error(GL_INVALID_VALUE);
	}

	if(!IsSizedInternalFormat(internalformat) && !IsCompressed(internalformat, egl::getClientVersion()))
	{
		return error(GL_INVALID_ENUM);
	}

	es2::Context *context = es2::getContext();

	if(context)
	{
		switch(target)
		{
		case GL_TEXTURE_3D:
			{
				if(levels > es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS || levels > (log2(std::max(std::max(width, height), depth)) + 1))
				{
					return error(GL_INVALID_OPERATION);
				}

				es2::Texture3D *texture = context->getTexture3D();
				if(!texture || texture->name == 0 || texture->getImmutableFormat() == GL_TRUE)
				{
					return error(GL_INVALID_OPERATION);
				}

				for(int level = 0; level < levels; level++)
				{
					texture->setImage(level, width, height, depth, internalformat, GL_NONE, GL_NONE, context->getUnpackParameters(), nullptr);
					width = std::max(1, (width / 2));
					height = std::max(1, (height / 2));
					depth = std::max(1, (depth / 2));
				}
				texture->makeImmutable(levels);
			}
			break;
		case GL_TEXTURE_2D_ARRAY:
			{
				if(levels > es2::IMPLEMENTATION_MAX_TEXTURE_LEVELS || levels > (log2(std::max(width, height)) + 1))
				{
					return error(GL_INVALID_OPERATION);
				}

				es2::Texture3D *texture = context->getTexture2DArray();
				if(!texture || texture->name == 0 || texture->getImmutableFormat())
				{
					return error(GL_INVALID_OPERATION);
				}

				for(int level = 0; level < levels; level++)
				{
					texture->setImage(level, width, height, depth, internalformat, GL_NONE, GL_NONE, context->getUnpackParameters(), nullptr);

					width = std::max(1, (width / 2));
					height = std::max(1, (height / 2));
				}
				texture->makeImmutable(levels);
			}
			break;
		default:
			return error(GL_INVALID_ENUM);
		}
	}
}

GL_APICALL void GL_APIENTRY glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params)
{
	TRACE("(GLenum target = 0x%X, GLenum internalformat = 0x%X, GLenum pname = 0x%X, GLsizei bufSize = %d, GLint *params = %p)",
	      target, internalformat, pname, bufSize, params);

	if(bufSize < 0)
	{
		return error(GL_INVALID_VALUE);
	}

	if(bufSize == 0)
	{
		return;
	}

	// OpenGL ES 3.0, section 4.4.4: "An internal format is color-renderable if it is one of the formats
	// from table 3.13 noted as color-renderable or if it is unsized format RGBA or RGB."
	// Since we only use sized formats internally, replace them here (assuming type = GL_UNSIGNED_BYTE).
	if(internalformat == GL_RGB)  internalformat = GL_RGB8;
	if(internalformat == GL_RGBA) internalformat = GL_RGBA8;

	if(!IsColorRenderable(internalformat, egl::getClientVersion()) &&
	   !IsDepthRenderable(internalformat, egl::getClientVersion()) &&
	   !IsStencilRenderable(internalformat, egl::getClientVersion()))
	{
		return error(GL_INVALID_ENUM);
	}

	switch(target)
	{
	case GL_RENDERBUFFER:
		break;
	default:
		return error(GL_INVALID_ENUM);
	}

	GLint numMultisampleCounts = NUM_MULTISAMPLE_COUNTS;

	// Integer types have no multisampling
	GLenum type = GetColorComponentType(internalformat);
	if(type != GL_UNSIGNED_NORMALIZED && type != GL_FLOAT)
	{
		numMultisampleCounts = 0;
	}

	switch(pname)
	{
	case GL_NUM_SAMPLE_COUNTS:
		*params = numMultisampleCounts;
		break;
	case GL_SAMPLES:
		for(int i = 0; i < numMultisampleCounts && i < bufSize; i++)
		{
			params[i] = multisampleCount[i];
		}
		break;
	default:
		return error(GL_INVALID_ENUM);
	}
}

}
