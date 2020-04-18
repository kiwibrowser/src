#ifndef _SGLRGLCONTEXT_HPP
#define _SGLRGLCONTEXT_HPP
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
 * \brief GL Rendering Context.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "sglrContext.hpp"
#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"

#include <set>
#include <vector>

namespace glu
{
class CallLogWrapper;
}

namespace sglr
{

enum GLContextLogFlag
{
	GLCONTEXT_LOG_CALLS		= (1 << 0),
	GLCONTEXT_LOG_PROGRAMS	= (1 << 1)
};

class GLContext : public Context
{
public:
										GLContext				(const glu::RenderContext& context, tcu::TestLog& log, deUint32 logFlags, const tcu::IVec4& baseViewport);
	virtual								~GLContext				(void);

	void								enableLogging			(deUint32 logFlags);

	virtual int							getWidth				(void) const;
	virtual int							getHeight				(void) const;

	virtual void						viewport				(int x, int y, int width, int height);
	virtual void						activeTexture			(deUint32 texture);

	virtual void						bindTexture				(deUint32 target, deUint32 texture);
	virtual void						genTextures				(int numTextures, deUint32* textures);
	virtual void						deleteTextures			(int numTextures, const deUint32* textures);

	virtual void						bindFramebuffer			(deUint32 target, deUint32 framebuffer);
	virtual void						genFramebuffers			(int numFramebuffers, deUint32* framebuffers);
	virtual void						deleteFramebuffers		(int numFramebuffers, const deUint32* framebuffers);

	virtual void						bindRenderbuffer		(deUint32 target, deUint32 renderbuffer);
	virtual void						genRenderbuffers		(int numRenderbuffers, deUint32* renderbuffers);
	virtual void						deleteRenderbuffers		(int numRenderbuffers, const deUint32* renderbuffers);

	virtual void						pixelStorei				(deUint32 pname, int param);
	virtual void						texImage1D				(deUint32 target, int level, deUint32 internalFormat, int width, int border, deUint32 format, deUint32 type, const void* data);
	virtual void						texImage2D				(deUint32 target, int level, deUint32 internalFormat, int width, int height, int border, deUint32 format, deUint32 type, const void* data);
	virtual void						texImage3D				(deUint32 target, int level, deUint32 internalFormat, int width, int height, int depth, int border, deUint32 format, deUint32 type, const void* data);
	virtual void						texSubImage1D			(deUint32 target, int level, int xoffset, int width, deUint32 format, deUint32 type, const void* data);
	virtual void						texSubImage2D			(deUint32 target, int level, int xoffset, int yoffset, int width, int height, deUint32 format, deUint32 type, const void* data);
	virtual void						texSubImage3D			(deUint32 target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, deUint32 format, deUint32 type, const void* data);
	virtual void						copyTexImage1D			(deUint32 target, int level, deUint32 internalFormat, int x, int y, int width, int border);
	virtual void						copyTexImage2D			(deUint32 target, int level, deUint32 internalFormat, int x, int y, int width, int height, int border);
	virtual void						copyTexSubImage1D		(deUint32 target, int level, int xoffset, int x, int y, int width);
	virtual void						copyTexSubImage2D		(deUint32 target, int level, int xoffset, int yoffset, int x, int y, int width, int height);
	virtual void						copyTexSubImage3D		(deUint32 target, int level, int xoffset, int yoffset, int zoffset, int x, int y, int width, int height);

	virtual void						texStorage2D			(deUint32 target, int levels, deUint32 internalFormat, int width, int height);
	virtual void						texStorage3D			(deUint32 target, int levels, deUint32 internalFormat, int width, int height, int depth);

	virtual void						texParameteri			(deUint32 target, deUint32 pname, int value);

	virtual void						framebufferTexture2D	(deUint32 target, deUint32 attachment, deUint32 textarget, deUint32 texture, int level);
	virtual void						framebufferTextureLayer	(deUint32 target, deUint32 attachment, deUint32 texture, int level, int layer);
	virtual void						framebufferRenderbuffer	(deUint32 target, deUint32 attachment, deUint32 renderbuffertarget, deUint32 renderbuffer);
	virtual deUint32					checkFramebufferStatus	(deUint32 target);

	virtual void						getFramebufferAttachmentParameteriv	(deUint32 target, deUint32 attachment, deUint32 pname, int* params);

	virtual void						renderbufferStorage				(deUint32 target, deUint32 internalformat, int width, int height);
	virtual void						renderbufferStorageMultisample	(deUint32 target, int samples, deUint32 internalFormat, int width, int height);

	virtual void						bindBuffer				(deUint32 target, deUint32 buffer);
	virtual void						genBuffers				(int numBuffers, deUint32* buffers);
	virtual void						deleteBuffers			(int numBuffers, const deUint32* buffers);

	virtual void						bufferData				(deUint32 target, deIntptr size, const void* data, deUint32 usage);
	virtual void						bufferSubData			(deUint32 target, deIntptr offset, deIntptr size, const void* data);

	virtual void						clearColor				(float red, float green, float blue, float alpha);
	virtual void						clearDepthf				(float depth);
	virtual void						clearStencil			(int stencil);

	virtual void						clear					(deUint32 buffers);
	virtual void						clearBufferiv			(deUint32 buffer, int drawbuffer, const int* value);
	virtual void						clearBufferfv			(deUint32 buffer, int drawbuffer, const float* value);
	virtual void						clearBufferuiv			(deUint32 buffer, int drawbuffer, const deUint32* value);
	virtual void						clearBufferfi			(deUint32 buffer, int drawbuffer, float depth, int stencil);
	virtual void						scissor					(int x, int y, int width, int height);

	virtual void						enable					(deUint32 cap);
	virtual void						disable					(deUint32 cap);

	virtual void						stencilFunc				(deUint32 func, int ref, deUint32 mask);
	virtual void						stencilOp				(deUint32 sfail, deUint32 dpfail, deUint32 dppass);
	virtual void						stencilFuncSeparate		(deUint32 face, deUint32 func, int ref, deUint32 mask);
	virtual void						stencilOpSeparate		(deUint32 face, deUint32 sfail, deUint32 dpfail, deUint32 dppass);

	virtual void						depthFunc				(deUint32 func);
	virtual void						depthRangef				(float n, float f);
	virtual void						depthRange				(double n, double f);

	virtual void						polygonOffset			(float factor, float units);
	virtual void						provokingVertex			(deUint32 convention);
	virtual void						primitiveRestartIndex	(deUint32 index);

	virtual void						blendEquation			(deUint32 mode);
	virtual void						blendEquationSeparate	(deUint32 modeRGB, deUint32 modeAlpha);
	virtual void						blendFunc				(deUint32 src, deUint32 dst);
	virtual void						blendFuncSeparate		(deUint32 srcRGB, deUint32 dstRGB, deUint32 srcAlpha, deUint32 dstAlpha);
	virtual void						blendColor				(float red, float green, float blue, float alpha);

	virtual void						colorMask				(deBool r, deBool g, deBool b, deBool a);
	virtual void						depthMask				(deBool mask);
	virtual void						stencilMask				(deUint32 mask);
	virtual void						stencilMaskSeparate		(deUint32 face, deUint32 mask);

	virtual void						blitFramebuffer			(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, deUint32 mask, deUint32 filter);

	virtual void						invalidateSubFramebuffer(deUint32 target, int numAttachments, const deUint32* attachments, int x, int y, int width, int height);
	virtual void						invalidateFramebuffer	(deUint32 target, int numAttachments, const deUint32* attachments);

	virtual void						bindVertexArray			(deUint32 array);
	virtual void						genVertexArrays			(int numArrays, deUint32* vertexArrays);
	virtual void						deleteVertexArrays		(int numArrays, const deUint32* vertexArrays);

	virtual void						vertexAttribPointer		(deUint32 index, int size, deUint32 type, deBool normalized, int stride, const void *pointer);
	virtual void						vertexAttribIPointer	(deUint32 index, int size, deUint32 type, int stride, const void *pointer);
	virtual void						enableVertexAttribArray	(deUint32 index);
	virtual void						disableVertexAttribArray(deUint32 index);
	virtual void						vertexAttribDivisor		(deUint32 index, deUint32 divisor);

	virtual void						vertexAttrib1f			(deUint32 index, float);
	virtual void						vertexAttrib2f			(deUint32 index, float, float);
	virtual void						vertexAttrib3f			(deUint32 index, float, float, float);
	virtual void						vertexAttrib4f			(deUint32 index, float, float, float, float);
	virtual void						vertexAttribI4i			(deUint32 index, deInt32, deInt32, deInt32, deInt32);
	virtual void						vertexAttribI4ui		(deUint32 index, deUint32, deUint32, deUint32, deUint32);

	virtual deInt32						getAttribLocation		(deUint32 program, const char *name);

	virtual void						uniform1f				(deInt32 location, float);
	virtual void						uniform1i				(deInt32 location, deInt32);
	virtual void						uniform1fv				(deInt32 index, deInt32 count, const float*);
	virtual void						uniform2fv				(deInt32 index, deInt32 count, const float*);
	virtual void						uniform3fv				(deInt32 index, deInt32 count, const float*);
	virtual void						uniform4fv				(deInt32 index, deInt32 count, const float*);
	virtual void						uniform1iv				(deInt32 index, deInt32 count, const deInt32*);
	virtual void						uniform2iv				(deInt32 index, deInt32 count, const deInt32*);
	virtual void						uniform3iv				(deInt32 index, deInt32 count, const deInt32*);
	virtual void						uniform4iv				(deInt32 index, deInt32 count, const deInt32*);
	virtual void						uniformMatrix3fv		(deInt32 location, deInt32 count, deBool transpose, const float *value);
	virtual void						uniformMatrix4fv		(deInt32 location, deInt32 count, deBool transpose, const float *value);
	virtual deInt32						getUniformLocation		(deUint32 program, const char *name);

	virtual void						lineWidth				(float);

	virtual void						drawArrays				(deUint32 mode, int first, int count);
	virtual void						drawArraysInstanced		(deUint32 mode, int first, int count, int instanceCount);
	virtual void						drawElements			(deUint32 mode, int count, deUint32 type, const void *indices);
	virtual void						drawElementsInstanced	(deUint32 mode, int count, deUint32 type, const void *indices, int instanceCount);
	virtual void						drawElementsBaseVertex	(deUint32 mode, int count, deUint32 type, const void *indices, int baseVertex);
	virtual void						drawElementsInstancedBaseVertex	(deUint32 mode, int count, deUint32 type, const void *indices, int instanceCount, int baseVertex);
	virtual void						drawRangeElements		(deUint32 mode, deUint32 start, deUint32 end, int count, deUint32 type, const void *indices);
	virtual void						drawRangeElementsBaseVertex	(deUint32 mode, deUint32 start, deUint32 end, int count, deUint32 type, const void *indices, int baseVertex);
	virtual void						drawArraysIndirect		(deUint32 mode, const void *indirect);
	virtual void						drawElementsIndirect	(deUint32 mode, deUint32 type, const void *indirect);

	virtual void						multiDrawArrays			(deUint32 mode, const int* first, const int* count, int primCount);
	virtual void						multiDrawElements		(deUint32 mode, const int* count, deUint32 type, const void** indices, int primCount);
	virtual void						multiDrawElementsBaseVertex (deUint32 mode, const int* count, deUint32 type, const void** indices, int primCount, const int* baseVertex);

	virtual deUint32					createProgram			(ShaderProgram*);
	virtual void						deleteProgram			(deUint32 program);
	virtual void						useProgram				(deUint32 program);

	virtual void						readPixels				(int x, int y, int width, int height, deUint32 format, deUint32 type, void* data);
	virtual deUint32					getError				(void);
	virtual void						finish					(void);

	virtual void						getIntegerv				(deUint32 pname, int* params);
	virtual const char*					getString				(deUint32 pname);

	// Expose helpers from Context.
	using Context::readPixels;
	using Context::texImage2D;
	using Context::texSubImage2D;

private:
										GLContext				(const GLContext& other);
	GLContext&							operator=				(const GLContext& other);

	tcu::IVec2							getReadOffset			(void) const;
	tcu::IVec2							getDrawOffset			(void) const;

	const glu::RenderContext&			m_context;
	tcu::TestLog&						m_log;

	deUint32							m_logFlags;
	tcu::IVec4							m_baseViewport;
	tcu::IVec4							m_curViewport;
	tcu::IVec4							m_curScissor;
	deUint32							m_readFramebufferBinding;
	deUint32							m_drawFramebufferBinding;

	glu::CallLogWrapper*				m_wrapper;

	// For cleanup
	std::set<deUint32>					m_allocatedTextures;
	std::set<deUint32>					m_allocatedFbos;
	std::set<deUint32>					m_allocatedRbos;
	std::set<deUint32>					m_allocatedBuffers;
	std::set<deUint32>					m_allocatedVaos;
	std::vector<glu::ShaderProgram*>	m_programs;
} DE_WARN_UNUSED_TYPE;

} // sglr

#endif // _SGLRGLCONTEXT_HPP
