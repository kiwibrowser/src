#ifndef _SGLRCONTEXT_HPP
#define _SGLRCONTEXT_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES Utilities
 * ------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Simplified GLES reference context.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuSurface.hpp"
#include "gluRenderContext.hpp"
#include "sglrShaderProgram.hpp"


/*--------------------------------------------------------------------*//*!
 * \brief Reference OpenGL API implementation
 *//*--------------------------------------------------------------------*/
namespace sglr
{

// Abstract drawing context with GL-style API

class Context
{
public:
						Context					(glu::ContextType type) : m_type(type) {}
	virtual				~Context				(void) {}

	virtual int			getWidth				(void) const												= DE_NULL;
	virtual int			getHeight				(void) const												= DE_NULL;

	virtual void		activeTexture			(deUint32 texture)											= DE_NULL;
	virtual void		viewport				(int x, int y, int width, int height)						= DE_NULL;

	virtual void		bindTexture				(deUint32 target, deUint32 texture)							= DE_NULL;
	virtual void		genTextures				(int numTextures, deUint32* textures)						= DE_NULL;
	virtual void		deleteTextures			(int numTextures, const deUint32* textures)					= DE_NULL;

	virtual void		bindFramebuffer			(deUint32 target, deUint32 framebuffer)						= DE_NULL;
	virtual void		genFramebuffers			(int numFramebuffers, deUint32* framebuffers)				= DE_NULL;
	virtual void		deleteFramebuffers		(int numFramebuffers, const deUint32* framebuffers)			= DE_NULL;

	virtual void		bindRenderbuffer		(deUint32 target, deUint32 renderbuffer)					= DE_NULL;
	virtual void		genRenderbuffers		(int numRenderbuffers, deUint32* renderbuffers)				= DE_NULL;
	virtual void		deleteRenderbuffers		(int numRenderbuffers, const deUint32* renderbuffers)		= DE_NULL;

	virtual void		pixelStorei				(deUint32 pname, int param)									= DE_NULL;
	virtual void		texImage1D				(deUint32 target, int level, deUint32 internalFormat, int width, int border, deUint32 format, deUint32 type, const void* data)							= DE_NULL;
	virtual void		texImage2D				(deUint32 target, int level, deUint32 internalFormat, int width, int height, int border, deUint32 format, deUint32 type, const void* data)				= DE_NULL;
	virtual void		texImage3D				(deUint32 target, int level, deUint32 internalFormat, int width, int height, int depth, int border, deUint32 format, deUint32 type, const void* data)	= DE_NULL;
	virtual void		texSubImage1D			(deUint32 target, int level, int xoffset, int width, deUint32 format, deUint32 type, const void* data)										= DE_NULL;
	virtual void		texSubImage2D			(deUint32 target, int level, int xoffset, int yoffset, int width, int height, deUint32 format, deUint32 type, const void* data)							= DE_NULL;
	virtual void		texSubImage3D			(deUint32 target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, deUint32 format, deUint32 type, const void* data)	= DE_NULL;
	virtual void		copyTexImage1D			(deUint32 target, int level, deUint32 internalFormat, int x, int y, int width, int border)																= DE_NULL;
	virtual void		copyTexImage2D			(deUint32 target, int level, deUint32 internalFormat, int x, int y, int width, int height, int border)													= DE_NULL;
	virtual void		copyTexSubImage1D		(deUint32 target, int level, int xoffset, int x, int y, int width)																			= DE_NULL;
	virtual void		copyTexSubImage2D		(deUint32 target, int level, int xoffset, int yoffset, int x, int y, int width, int height)																= DE_NULL;
	virtual void		copyTexSubImage3D		(deUint32 target, int level, int xoffset, int yoffset, int zoffset, int x, int y, int width, int height)												= DE_NULL;

	virtual void		texStorage2D			(deUint32 target, int levels, deUint32 internalFormat, int width, int height)				= DE_NULL;
	virtual void		texStorage3D			(deUint32 target, int levels, deUint32 internalFormat, int width, int height, int depth)	= DE_NULL;

	virtual void		texParameteri			(deUint32 target, deUint32 pname, int value)					= DE_NULL;

	virtual void		framebufferTexture2D	(deUint32 target, deUint32 attachment, deUint32 textarget, deUint32 texture, int level)	= DE_NULL;
	virtual void		framebufferTextureLayer	(deUint32 target, deUint32 attachment, deUint32 texture, int level, int layer)	= DE_NULL;
	virtual void		framebufferRenderbuffer	(deUint32 target, deUint32 attachment, deUint32 renderbuffertarget, deUint32 renderbuffer) = DE_NULL;
	virtual deUint32	checkFramebufferStatus	(deUint32 target)												= DE_NULL;

	virtual void		getFramebufferAttachmentParameteriv	(deUint32 target, deUint32 attachment, deUint32 pname, int* params) = DE_NULL;

	virtual void		renderbufferStorage				(deUint32 target, deUint32 internalformat, int width, int height) = DE_NULL;
	virtual void		renderbufferStorageMultisample	(deUint32 target, int samples, deUint32 internalFormat, int width, int height) = DE_NULL;

	virtual void		bindBuffer				(deUint32 target, deUint32 buffer)							= DE_NULL;
	virtual void		genBuffers				(int numBuffers, deUint32* buffers)							= DE_NULL;
	virtual void		deleteBuffers			(int numBuffers, const deUint32* buffers)					= DE_NULL;

	virtual void		bufferData				(deUint32 target, deIntptr size, const void* data, deUint32 usage)	= DE_NULL;
	virtual void		bufferSubData			(deUint32 target, deIntptr offset, deIntptr size, const void* data)	= DE_NULL;

	virtual void		clearColor				(float red, float green, float blue, float alpha)			= DE_NULL;
	virtual void		clearDepthf				(float depth)												= DE_NULL;
	virtual void		clearStencil			(int stencil)												= DE_NULL;

	virtual void		clear					(deUint32 buffers)											= DE_NULL;
	virtual void		clearBufferiv			(deUint32 buffer, int drawbuffer, const int* value)			= DE_NULL;
	virtual void		clearBufferfv			(deUint32 buffer, int drawbuffer, const float* value)		= DE_NULL;
	virtual void		clearBufferuiv			(deUint32 buffer, int drawbuffer, const deUint32* value)	= DE_NULL;
	virtual void		clearBufferfi			(deUint32 buffer, int drawbuffer, float depth, int stencil)	= DE_NULL;
	virtual void		scissor					(int x, int y, int width, int height)						= DE_NULL;

	virtual void		enable					(deUint32 cap)												= DE_NULL;
	virtual void		disable					(deUint32 cap)												= DE_NULL;

	virtual void		stencilFunc				(deUint32 func, int ref, deUint32 mask)						= DE_NULL;
	virtual void		stencilOp				(deUint32 sfail, deUint32 dpfail, deUint32 dppass)			= DE_NULL;
	virtual void		stencilFuncSeparate		(deUint32 face, deUint32 func, int ref, deUint32 mask)		= DE_NULL;
	virtual void		stencilOpSeparate		(deUint32 face, deUint32 sfail, deUint32 dpfail, deUint32 dppass) = DE_NULL;

	virtual void		depthFunc				(deUint32 func)												= DE_NULL;
	virtual void		depthRangef				(float n, float f)											= DE_NULL;
	virtual void		depthRange				(double n, double f)										= DE_NULL;

	virtual void		polygonOffset			(float factor, float units)									= DE_NULL;
	virtual void		provokingVertex			(deUint32 convention)										= DE_NULL;
	virtual void		primitiveRestartIndex	(deUint32 index)											= DE_NULL;

	virtual void		blendEquation			(deUint32 mode)												= DE_NULL;
	virtual void		blendEquationSeparate	(deUint32 modeRGB, deUint32 modeAlpha)						= DE_NULL;
	virtual void		blendFunc				(deUint32 src, deUint32 dst)								= DE_NULL;
	virtual void		blendFuncSeparate		(deUint32 srcRGB, deUint32 dstRGB, deUint32 srcAlpha, deUint32 dstAlpha) = DE_NULL;
	virtual void		blendColor				(float red, float green, float blue, float alpha)			= DE_NULL;

	virtual void		colorMask				(deBool r, deBool g, deBool b, deBool a)					= DE_NULL;
	virtual void		depthMask				(deBool mask)												= DE_NULL;
	virtual void		stencilMask				(deUint32 mask)												= DE_NULL;
	virtual void		stencilMaskSeparate		(deUint32 face, deUint32 mask)								= DE_NULL;

	virtual void		blitFramebuffer			(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, deUint32 mask, deUint32 filter) = DE_NULL;

	virtual void		invalidateSubFramebuffer(deUint32 target, int numAttachments, const deUint32* attachments, int x, int y, int width, int height)	= DE_NULL;
	virtual void		invalidateFramebuffer	(deUint32 target, int numAttachments, const deUint32* attachments)										= DE_NULL;

	virtual void		bindVertexArray			(deUint32 array)											= DE_NULL;
	virtual void		genVertexArrays			(int numArrays, deUint32* vertexArrays)						= DE_NULL;
	virtual void		deleteVertexArrays		(int numArrays, const deUint32* vertexArrays)				= DE_NULL;

	virtual void		vertexAttribPointer		(deUint32 index, int size, deUint32 type, deBool normalized, int stride, const void *pointer)	= DE_NULL;
	virtual void		vertexAttribIPointer	(deUint32 index, int size, deUint32 type, int stride, const void *pointer)						= DE_NULL;
	virtual void		enableVertexAttribArray	(deUint32 index)											= DE_NULL;
	virtual void		disableVertexAttribArray(deUint32 index)											= DE_NULL;
	virtual void		vertexAttribDivisor		(deUint32 index, deUint32 divisor)							= DE_NULL;

	virtual void		vertexAttrib1f			(deUint32 index, float)										= DE_NULL;
	virtual void		vertexAttrib2f			(deUint32 index, float, float)								= DE_NULL;
	virtual void		vertexAttrib3f			(deUint32 index, float, float, float)						= DE_NULL;
	virtual void		vertexAttrib4f			(deUint32 index, float, float, float, float)				= DE_NULL;
	virtual void		vertexAttribI4i			(deUint32 index, deInt32, deInt32, deInt32, deInt32)		= DE_NULL;
	virtual void		vertexAttribI4ui		(deUint32 index, deUint32, deUint32, deUint32, deUint32)	= DE_NULL;

	virtual deInt32		getAttribLocation		(deUint32 program, const char *name)						= DE_NULL;

	virtual void		uniform1f				(deInt32 index, float)										= DE_NULL;
	virtual void		uniform1i				(deInt32 index, deInt32)									= DE_NULL;
	virtual void		uniform1fv				(deInt32 index, deInt32 count, const float*)				= DE_NULL;
	virtual void		uniform2fv				(deInt32 index, deInt32 count, const float*)				= DE_NULL;
	virtual void		uniform3fv				(deInt32 index, deInt32 count, const float*)				= DE_NULL;
	virtual void		uniform4fv				(deInt32 index, deInt32 count, const float*)				= DE_NULL;
	virtual void		uniform1iv				(deInt32 index, deInt32 count, const deInt32*)				= DE_NULL;
	virtual void		uniform2iv				(deInt32 index, deInt32 count, const deInt32*)				= DE_NULL;
	virtual void		uniform3iv				(deInt32 index, deInt32 count, const deInt32*)				= DE_NULL;
	virtual void		uniform4iv				(deInt32 index, deInt32 count, const deInt32*)				= DE_NULL;
	virtual void		uniformMatrix3fv		(deInt32 location, deInt32 count, deBool transpose, const float *value)	= DE_NULL;
	virtual void		uniformMatrix4fv		(deInt32 location, deInt32 count, deBool transpose, const float *value)	= DE_NULL;
	virtual deInt32		getUniformLocation		(deUint32 program, const char *name)						= DE_NULL;

	virtual void		lineWidth				(float)														= DE_NULL;

	virtual void		drawArrays				(deUint32 mode, int first, int count)															= DE_NULL;
	virtual void		drawArraysInstanced		(deUint32 mode, int first, int count, int instanceCount)										= DE_NULL;
	virtual void		drawElements			(deUint32 mode, int count, deUint32 type, const void *indices)									= DE_NULL;
	virtual void		drawElementsInstanced	(deUint32 mode, int count, deUint32 type, const void *indices, int instanceCount)				= DE_NULL;
	virtual void		drawElementsBaseVertex	(deUint32 mode, int count, deUint32 type, const void *indices, int baseVertex)					= DE_NULL;
	virtual void		drawElementsInstancedBaseVertex	(deUint32 mode, int count, deUint32 type, const void *indices, int instanceCount, int baseVertex) = DE_NULL;
	virtual void		drawRangeElements		(deUint32 mode, deUint32 start, deUint32 end, int count, deUint32 type, const void *indices)	= DE_NULL;
	virtual void		drawRangeElementsBaseVertex	(deUint32 mode, deUint32 start, deUint32 end, int count, deUint32 type, const void *indices, int baseVertex) = DE_NULL;
	virtual void		drawArraysIndirect		(deUint32 mode, const void *indirect)															= DE_NULL;
	virtual void		drawElementsIndirect	(deUint32 mode, deUint32 type, const void *indirect)											= DE_NULL;

	virtual void		multiDrawArrays			(deUint32 mode, const int* first, const int* count, int primCount)								= DE_NULL;
	virtual void		multiDrawElements		(deUint32 mode, const int* count, deUint32 type, const void** indices, int primCount)			= DE_NULL;
	virtual void		multiDrawElementsBaseVertex (deUint32 mode, const int* count, deUint32 type, const void** indices, int primCount, const int* baseVertex) = DE_NULL;

	virtual deUint32	createProgram			(ShaderProgram* program)															= DE_NULL;
	virtual void		useProgram				(deUint32 program)																	= DE_NULL;
	virtual void		deleteProgram			(deUint32 program)																	= DE_NULL;


	virtual void		readPixels				(int x, int y, int width, int height, deUint32 format, deUint32 type, void* data)	= DE_NULL;
	virtual deUint32	getError				(void)																				= DE_NULL;
	virtual void		finish					(void)																				= DE_NULL;

	virtual void		getIntegerv				(deUint32 pname, int* params)														= DE_NULL;
	virtual const char*	getString				(deUint32 pname)																	= DE_NULL;

	// Helpers implemented by Context.
	virtual void		texImage2D				(deUint32 target, int level, deUint32 internalFormat, const tcu::Surface& src);
	virtual void		texImage2D				(deUint32 target, int level, deUint32 internalFormat, int width, int height);
	virtual void		texSubImage2D			(deUint32 target, int level, int xoffset, int yoffset, const tcu::Surface& src);
	virtual void		readPixels				(tcu::Surface& dst, int x, int y, int width, int height);

	glu::ContextType	getType					(void)	{ return m_type; }

private:
	const glu::ContextType	m_type;
} DE_WARN_UNUSED_TYPE;

} // sglr

#endif // _SGLRCONTEXT_HPP
