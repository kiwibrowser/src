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
 * \brief Context wrapper that exposes sglr API as GL-compatible API.
 *//*--------------------------------------------------------------------*/

#include "sglrContextWrapper.hpp"
#include "sglrContext.hpp"

namespace sglr
{

ContextWrapper::ContextWrapper (void)
 : m_curCtx(DE_NULL)
{
}

ContextWrapper::~ContextWrapper (void)
{
}

void ContextWrapper::setContext (Context* context)
{
	m_curCtx = context;
}

Context* ContextWrapper::getCurrentContext (void) const
{
	return m_curCtx;
}

int ContextWrapper::getWidth (void) const
{
	return m_curCtx->getWidth();
}

int ContextWrapper::getHeight (void) const
{
	return m_curCtx->getHeight();
}

void ContextWrapper::glViewport (int x, int y, int width, int height)
{
	m_curCtx->viewport(x, y, width, height);
}

void ContextWrapper::glActiveTexture (deUint32 texture)
{
	m_curCtx->activeTexture(texture);
}

void ContextWrapper::glBindTexture (deUint32 target, deUint32 texture)
{
	m_curCtx->bindTexture(target, texture);
}

void ContextWrapper::glGenTextures (int numTextures, deUint32* textures)
{
	m_curCtx->genTextures(numTextures, textures);
}

void ContextWrapper::glDeleteTextures (int numTextures, const deUint32* textures)
{
	m_curCtx->deleteTextures(numTextures, textures);
}

void ContextWrapper::glBindFramebuffer (deUint32 target, deUint32 framebuffer)
{
	m_curCtx->bindFramebuffer(target, framebuffer);
}

void ContextWrapper::glGenFramebuffers (int numFramebuffers, deUint32* framebuffers)
{
	m_curCtx->genFramebuffers(numFramebuffers, framebuffers);
}

void ContextWrapper::glDeleteFramebuffers (int numFramebuffers, const deUint32* framebuffers)
{
	m_curCtx->deleteFramebuffers(numFramebuffers, framebuffers);
}

void ContextWrapper::glBindRenderbuffer (deUint32 target, deUint32 renderbuffer)
{
	m_curCtx->bindRenderbuffer(target, renderbuffer);
}

void ContextWrapper::glGenRenderbuffers (int numRenderbuffers, deUint32* renderbuffers)
{
	m_curCtx->genRenderbuffers(numRenderbuffers, renderbuffers);
}

void ContextWrapper::glDeleteRenderbuffers (int numRenderbuffers, const deUint32* renderbuffers)
{
	m_curCtx->deleteRenderbuffers(numRenderbuffers, renderbuffers);
}

void ContextWrapper::glPixelStorei (deUint32 pname, int param)
{
	m_curCtx->pixelStorei(pname, param);
}

void ContextWrapper::glTexImage1D (deUint32 target, int level, int internalFormat, int width, int border, deUint32 format, deUint32 type, const void* data)
{
	m_curCtx->texImage1D(target, level, (deUint32)internalFormat, width, border, format, type, data);
}

void ContextWrapper::glTexImage2D (deUint32 target, int level, int internalFormat, int width, int height, int border, deUint32 format, deUint32 type, const void* data)
{
	m_curCtx->texImage2D(target, level, (deUint32)internalFormat, width, height, border, format, type, data);
}

void ContextWrapper::glTexImage3D (deUint32 target, int level, int internalFormat, int width, int height, int depth, int border, deUint32 format, deUint32 type, const void* data)
{
	m_curCtx->texImage3D(target, level, (deUint32)internalFormat, width, height, depth, border, format, type, data);
}

void ContextWrapper::glTexSubImage1D (deUint32 target, int level, int xoffset, int width, deUint32 format, deUint32 type, const void* data)
{
	m_curCtx->texSubImage1D(target, level, xoffset, width, format, type, data);
}

void ContextWrapper::glTexSubImage2D (deUint32 target, int level, int xoffset, int yoffset, int width, int height, deUint32 format, deUint32 type, const void* data)
{
	m_curCtx->texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, data);
}

void ContextWrapper::glTexSubImage3D (deUint32 target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, deUint32 format, deUint32 type, const void* data)
{
	m_curCtx->texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}

void ContextWrapper::glCopyTexImage1D (deUint32 target, int level, deUint32 internalFormat, int x, int y, int width, int border)
{
	m_curCtx->copyTexImage1D(target, level, internalFormat, x, y, width, border);
}

void ContextWrapper::glCopyTexImage2D (deUint32 target, int level, deUint32 internalFormat, int x, int y, int width, int height, int border)
{
	m_curCtx->copyTexImage2D(target, level, internalFormat, x, y, width, height, border);
}

void ContextWrapper::glCopyTexSubImage1D (deUint32 target, int level, int xoffset, int x, int y, int width)
{
	m_curCtx->copyTexSubImage1D(target, level, xoffset, x, y, width);
}

void ContextWrapper::glCopyTexSubImage2D (deUint32 target, int level, int xoffset, int yoffset, int x, int y, int width, int height)
{
	m_curCtx->copyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

void ContextWrapper::glTexStorage2D (deUint32 target, int levels, deUint32 internalFormat, int width, int height)
{
	m_curCtx->texStorage2D(target, levels, internalFormat, width, height);
}

void ContextWrapper::glTexStorage3D (deUint32 target, int levels, deUint32 internalFormat, int width, int height, int depth)
{
	m_curCtx->texStorage3D(target, levels, internalFormat, width, height, depth);
}

void ContextWrapper::glTexParameteri (deUint32 target, deUint32 pname, int value)
{
	m_curCtx->texParameteri(target, pname, value);
}

void ContextWrapper::glUseProgram (deUint32 program)
{
	m_curCtx->useProgram(program);
}

void ContextWrapper::glFramebufferTexture2D (deUint32 target, deUint32 attachment, deUint32 textarget, deUint32 texture, int level)
{
	m_curCtx->framebufferTexture2D(target, attachment, textarget, texture, level);
}

void ContextWrapper::glFramebufferTextureLayer (deUint32 target, deUint32 attachment, deUint32 texture, int level, int layer)
{
	m_curCtx->framebufferTextureLayer(target, attachment, texture, level, layer);
}

void ContextWrapper::glFramebufferRenderbuffer (deUint32 target, deUint32 attachment, deUint32 renderbuffertarget, deUint32 renderbuffer)
{
	m_curCtx->framebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

deUint32 ContextWrapper::glCheckFramebufferStatus (deUint32 target)
{
	return m_curCtx->checkFramebufferStatus(target);
}

void ContextWrapper::glGetFramebufferAttachmentParameteriv (deUint32 target, deUint32 attachment, deUint32 pname, int* params)
{
	m_curCtx->getFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

void ContextWrapper::glRenderbufferStorage (deUint32 target, deUint32 internalformat, int width, int height)
{
	m_curCtx->renderbufferStorage(target, internalformat, width, height);
}

void ContextWrapper::glRenderbufferStorageMultisample (deUint32 target, int samples, deUint32 internalformat, int width, int height)
{
	m_curCtx->renderbufferStorageMultisample(target, samples, internalformat, width, height);
}

void ContextWrapper::glBindBuffer (deUint32 target, deUint32 buffer)
{
	m_curCtx->bindBuffer(target, buffer);
}

void ContextWrapper::glGenBuffers (int n, deUint32* buffers)
{
	m_curCtx->genBuffers(n, buffers);
}

void ContextWrapper::glDeleteBuffers (int n, const deUint32* buffers)
{
	m_curCtx->deleteBuffers(n, buffers);
}

void ContextWrapper::glBufferData (deUint32 target, deIntptr size, const void* data, deUint32 usage)
{
	m_curCtx->bufferData(target, size, data, usage);
}

void ContextWrapper::glBufferSubData (deUint32 target, deIntptr offset, deIntptr size, const void* data)
{
	m_curCtx->bufferSubData(target, offset, size, data);
}

void ContextWrapper::glClearColor (float red, float green, float blue, float alpha)
{
	m_curCtx->clearColor(red, green, blue, alpha);
}

void ContextWrapper::glClearDepthf (float depth)
{
	m_curCtx->clearDepthf(depth);
}

void ContextWrapper::glClearStencil (int stencil)
{
	m_curCtx->clearStencil(stencil);
}

void ContextWrapper::glClear (deUint32 buffers)
{
	m_curCtx->clear(buffers);
}

void ContextWrapper::glClearBufferiv (deUint32 buffer, int drawbuffer, const int* value)
{
	m_curCtx->clearBufferiv(buffer, drawbuffer, value);
}

void ContextWrapper::glClearBufferfv (deUint32 buffer, int drawbuffer, const float* value)
{
	m_curCtx->clearBufferfv(buffer, drawbuffer, value);
}

void ContextWrapper::glClearBufferuiv (deUint32 buffer, int drawbuffer, const deUint32* value)
{
	m_curCtx->clearBufferuiv(buffer, drawbuffer, value);
}

void ContextWrapper::glClearBufferfi (deUint32 buffer, int drawbuffer, float depth, int stencil)
{
	m_curCtx->clearBufferfi(buffer, drawbuffer, depth, stencil);
}

void ContextWrapper::glScissor (int x, int y, int width, int height)
{
	m_curCtx->scissor(x, y, width, height);
}

void ContextWrapper::glEnable (deUint32 cap)
{
	m_curCtx->enable(cap);
}

void ContextWrapper::glDisable (deUint32 cap)
{
	m_curCtx->disable(cap);
}

void ContextWrapper::glStencilFunc (deUint32 func, int ref, deUint32 mask)
{
	m_curCtx->stencilFunc(func, ref, mask);
}

void ContextWrapper::glStencilOp (deUint32 sfail, deUint32 dpfail, deUint32 dppass)
{
	m_curCtx->stencilOp(sfail, dpfail, dppass);
}

void ContextWrapper::glDepthFunc (deUint32 func)
{
	m_curCtx->depthFunc(func);
}

void ContextWrapper::glBlendEquation (deUint32 mode)
{
	m_curCtx->blendEquation(mode);
}

void ContextWrapper::glBlendEquationSeparate (deUint32 modeRGB, deUint32 modeAlpha)
{
	m_curCtx->blendEquationSeparate(modeRGB, modeAlpha);
}

void ContextWrapper::glBlendFunc (deUint32 src, deUint32 dst)
{
	m_curCtx->blendFunc(src, dst);
}

void ContextWrapper::glBlendFuncSeparate (deUint32 srcRGB, deUint32 dstRGB, deUint32 srcAlpha, deUint32 dstAlpha)
{
	m_curCtx->blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void ContextWrapper::glBlendColor (float red, float green, float blue, float alpha)
{
	m_curCtx->blendColor(red, green, blue, alpha);
}

void ContextWrapper::glColorMask (deBool r, deBool g, deBool b, deBool a)
{
	m_curCtx->colorMask(r, g, b, a);
}

void ContextWrapper::glDepthMask (deBool mask)
{
	m_curCtx->depthMask(mask);
}

void ContextWrapper::glStencilMask (deUint32 mask)
{
	m_curCtx->stencilMask(mask);
}

void ContextWrapper::glBlitFramebuffer (int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, deUint32 mask, deUint32 filter)
{
	m_curCtx->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void ContextWrapper::glInvalidateSubFramebuffer (deUint32 target, int numAttachments, const deUint32* attachments, int x, int y, int width, int height)
{
	m_curCtx->invalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
}

void ContextWrapper::glInvalidateFramebuffer (deUint32 target, int numAttachments, const deUint32* attachments)
{
	m_curCtx->invalidateFramebuffer(target, numAttachments, attachments);
}

void ContextWrapper::glReadPixels (int x, int y, int width, int height, deUint32 format, deUint32 type, void* data)
{
	m_curCtx->readPixels(x, y, width, height, format, type, data);
}

deUint32 ContextWrapper::glGetError (void)
{
	return m_curCtx->getError();
}

void ContextWrapper::glGetIntegerv (deUint32 pname, int* params)
{
	m_curCtx->getIntegerv(pname, params);
}

} // sglr
