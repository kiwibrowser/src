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

#include "sglrGLContext.hpp"
#include "sglrShaderProgram.hpp"
#include "gluPixelTransfer.hpp"
#include "gluTexture.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluStrUtil.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

namespace sglr
{

using std::vector;
using std::string;
using tcu::TestLog;
using tcu::Vec4;
using tcu::TextureFormat;

GLContext::GLContext (const glu::RenderContext& context, tcu::TestLog& log, deUint32 logFlags, const tcu::IVec4& baseViewport)
	: Context					(context.getType())
	, m_context					(context)
	, m_log						(log)
	, m_logFlags				(logFlags)
	, m_baseViewport			(baseViewport)
	, m_curViewport				(0, 0, m_baseViewport.z(), m_baseViewport.w())
	, m_curScissor				(0, 0, m_baseViewport.z(), m_baseViewport.w())
	, m_readFramebufferBinding	(0)
	, m_drawFramebufferBinding	(0)
	, m_wrapper					(DE_NULL)
{
	const glw::Functions& gl = m_context.getFunctions();

	// Logging?
	m_wrapper = new glu::CallLogWrapper(gl, log);
	m_wrapper->enableLogging((logFlags & GLCONTEXT_LOG_CALLS) != 0);

	// Setup base viewport. This offset is active when default framebuffer is active.
	// \note Calls related to setting up base viewport are not included in log.
	gl.viewport(baseViewport.x(), baseViewport.y(), baseViewport.z(), baseViewport.w());
}

GLContext::~GLContext (void)
{
	const glw::Functions& gl = m_context.getFunctions();

	// Clean up all still alive objects
	for (std::set<deUint32>::const_iterator i = m_allocatedFbos.begin();
		 i != m_allocatedFbos.end(); i++)
	{
		deUint32 fbo = *i;
		gl.deleteFramebuffers(1, &fbo);
	}

	for (std::set<deUint32>::const_iterator i = m_allocatedRbos.begin();
		 i != m_allocatedRbos.end(); i++)
	{
		deUint32 rbo = *i;
		gl.deleteRenderbuffers(1, &rbo);
	}

	for (std::set<deUint32>::const_iterator i = m_allocatedTextures.begin();
		 i != m_allocatedTextures.end(); i++)
	{
		deUint32 tex = *i;
		gl.deleteTextures(1, &tex);
	}

	for (std::set<deUint32>::const_iterator i = m_allocatedBuffers.begin();
		 i != m_allocatedBuffers.end(); i++)
	{
		deUint32 buf = *i;
		gl.deleteBuffers(1, &buf);
	}

	for (std::set<deUint32>::const_iterator i = m_allocatedVaos.begin();
		 i != m_allocatedVaos.end(); i++)
	{
		deUint32 vao = *i;
		gl.deleteVertexArrays(1, &vao);
	}

	for (std::vector<glu::ShaderProgram*>::iterator i = m_programs.begin();
		i != m_programs.end(); i++)
	{
		delete *i;
	}

	gl.useProgram(0);

	delete m_wrapper;
}

void GLContext::enableLogging (deUint32 logFlags)
{
	m_logFlags = logFlags;
	m_wrapper->enableLogging((logFlags & GLCONTEXT_LOG_CALLS) != 0);
}

tcu::IVec2 GLContext::getDrawOffset (void) const
{
	if (m_drawFramebufferBinding)
		return tcu::IVec2(0, 0);
	else
		return tcu::IVec2(m_baseViewport.x(), m_baseViewport.y());
}

tcu::IVec2 GLContext::getReadOffset (void) const
{
	if (m_readFramebufferBinding)
		return tcu::IVec2(0, 0);
	else
		return tcu::IVec2(m_baseViewport.x(), m_baseViewport.y());
}

int GLContext::getWidth (void) const
{
	return m_baseViewport.z();
}

int GLContext::getHeight (void) const
{
	return m_baseViewport.w();
}

void GLContext::activeTexture (deUint32 texture)
{
	m_wrapper->glActiveTexture(texture);
}

void GLContext::texParameteri (deUint32 target, deUint32 pname, int value)
{
	m_wrapper->glTexParameteri(target, pname, value);
}

deUint32 GLContext::checkFramebufferStatus(deUint32 target)
{
	return m_wrapper->glCheckFramebufferStatus(target);
}

void GLContext::viewport (int x, int y, int width, int height)
{
	m_curViewport = tcu::IVec4(x, y, width, height);
	tcu::IVec2 offset = getDrawOffset();

	// \note For clarity don't add the offset to log
	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glViewport(" << x << ", " << y << ", " << width << ", " << height << ");" << TestLog::EndMessage;
	m_context.getFunctions().viewport(x+offset.x(), y+offset.y(), width, height);
}

void GLContext::bindTexture (deUint32 target, deUint32 texture)
{
	m_allocatedTextures.insert(texture);
	m_wrapper->glBindTexture(target, texture);
}

void GLContext::genTextures (int numTextures, deUint32* textures)
{
	m_wrapper->glGenTextures(numTextures, textures);
	if (numTextures > 0)
		m_allocatedTextures.insert(textures, textures+numTextures);
}

void GLContext::deleteTextures (int numTextures, const deUint32* textures)
{
	for (int i = 0; i < numTextures; i++)
		m_allocatedTextures.erase(textures[i]);
	m_wrapper->glDeleteTextures(numTextures, textures);
}

void GLContext::bindFramebuffer (deUint32 target, deUint32 framebuffer)
{
	// \todo [2011-10-13 pyry] This is a bit of a hack since test cases assumes 0 default fbo.
	deUint32 defaultFbo = m_context.getDefaultFramebuffer();
	TCU_CHECK(framebuffer == 0 || framebuffer != defaultFbo);

	bool isValidTarget = target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER;

	if (isValidTarget && framebuffer != 0)
		m_allocatedFbos.insert(framebuffer);

	// Update bindings.
	if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER)
		m_readFramebufferBinding = framebuffer;

	if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
		m_drawFramebufferBinding = framebuffer;

	if (framebuffer == 0) // Redirect 0 to platform-defined default framebuffer.
		m_wrapper->glBindFramebuffer(target, defaultFbo);
	else
		m_wrapper->glBindFramebuffer(target, framebuffer);

	// Update viewport and scissor if we updated draw framebuffer binding \note Not logged for clarity
	if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
	{
		tcu::IVec2 offset = getDrawOffset();
		m_context.getFunctions().viewport(m_curViewport.x()+offset.x(), m_curViewport.y()+offset.y(), m_curViewport.z(), m_curViewport.w());
		m_context.getFunctions().scissor(m_curScissor.x()+offset.x(), m_curScissor.y()+offset.y(), m_curScissor.z(), m_curScissor.w());
	}
}

void GLContext::genFramebuffers (int numFramebuffers, deUint32* framebuffers)
{
	m_wrapper->glGenFramebuffers(numFramebuffers, framebuffers);
	if (numFramebuffers > 0)
		m_allocatedFbos.insert(framebuffers, framebuffers+numFramebuffers);
}

void GLContext::deleteFramebuffers (int numFramebuffers, const deUint32* framebuffers)
{
	for (int i = 0; i < numFramebuffers; i++)
		m_allocatedFbos.erase(framebuffers[i]);
	m_wrapper->glDeleteFramebuffers(numFramebuffers, framebuffers);
}

void GLContext::bindRenderbuffer (deUint32 target, deUint32 renderbuffer)
{
	m_allocatedRbos.insert(renderbuffer);
	m_wrapper->glBindRenderbuffer(target, renderbuffer);
}

void GLContext::genRenderbuffers (int numRenderbuffers, deUint32* renderbuffers)
{
	m_wrapper->glGenRenderbuffers(numRenderbuffers, renderbuffers);
	if (numRenderbuffers > 0)
		m_allocatedRbos.insert(renderbuffers, renderbuffers+numRenderbuffers);
}

void GLContext::deleteRenderbuffers (int numRenderbuffers, const deUint32* renderbuffers)
{
	for (int i = 0; i < numRenderbuffers; i++)
		m_allocatedRbos.erase(renderbuffers[i]);
	m_wrapper->glDeleteRenderbuffers(numRenderbuffers, renderbuffers);
}

void GLContext::pixelStorei (deUint32 pname, int param)
{
	m_wrapper->glPixelStorei(pname, param);
}

void GLContext::texImage1D (deUint32 target, int level, deUint32 internalFormat, int width, int border, deUint32 format, deUint32 type, const void* data)
{
	m_wrapper->glTexImage1D(target, level, internalFormat, width, border, format, type, data);
}

void GLContext::texImage2D (deUint32 target, int level, deUint32 internalFormat, int width, int height, int border, deUint32 format, deUint32 type, const void* data)
{
	m_wrapper->glTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
}

void GLContext::texImage3D (deUint32 target, int level, deUint32 internalFormat, int width, int height, int depth, int border, deUint32 format, deUint32 type, const void* data)
{
	m_wrapper->glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, data);
}

void GLContext::texSubImage1D (deUint32 target, int level, int xoffset, int width, deUint32 format, deUint32 type, const void* data)
{
	m_wrapper->glTexSubImage1D(target, level, xoffset, width, format, type, data);
}

void GLContext::texSubImage2D (deUint32 target, int level, int xoffset, int yoffset, int width, int height, deUint32 format, deUint32 type, const void* data)
{
	m_wrapper->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, data);
}

void GLContext::texSubImage3D (deUint32 target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, deUint32 format, deUint32 type, const void* data)
{
	m_wrapper->glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}

void GLContext::copyTexImage1D (deUint32 target, int level, deUint32 internalFormat, int x, int y, int width, int border)
{
	// Don't log offset.
	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glCopyTexImage1D("
								  << glu::getTextureTargetStr(target) << ", "
								  << level << ", "
								  << glu::getTextureFormatStr(internalFormat) << ", "
								  << x << ", " << y << ", "
								  << width << ", " << border << ")"
			  << TestLog::EndMessage;

	tcu::IVec2 offset = getReadOffset();
	m_context.getFunctions().copyTexImage1D(target, level, internalFormat, offset.x()+x, offset.y()+y, width, border);
}

void GLContext::copyTexImage2D (deUint32 target, int level, deUint32 internalFormat, int x, int y, int width, int height, int border)
{
	// Don't log offset.
	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glCopyTexImage2D("
								  << glu::getTextureTargetStr(target) << ", "
								  << level << ", "
								  << glu::getTextureFormatStr(internalFormat) << ", "
								  << x << ", " << y << ", "
								  << width << ", " << height
								  << ", " << border << ")"
			  << TestLog::EndMessage;

	tcu::IVec2 offset = getReadOffset();
	m_context.getFunctions().copyTexImage2D(target, level, internalFormat, offset.x()+x, offset.y()+y, width, height, border);
}

void GLContext::copyTexSubImage1D (deUint32 target, int level, int xoffset, int x, int y, int width)
{
	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glCopyTexSubImage1D("
								  << glu::getTextureTargetStr(target) << ", "
								  << level << ", "
								  << xoffset << ", "
								  << x << ", " << y << ", "
								  << width << ")"
			  << TestLog::EndMessage;

	tcu::IVec2 offset = getReadOffset();
	m_context.getFunctions().copyTexSubImage1D(target, level, xoffset, offset.x()+x, offset.y()+y, width);
}

void GLContext::copyTexSubImage2D (deUint32 target, int level, int xoffset, int yoffset, int x, int y, int width, int height)
{
	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glCopyTexSubImage2D("
								  << glu::getTextureTargetStr(target) << ", "
								  << level
								  << ", " << xoffset << ", " << yoffset << ", "
								  << x << ", " << y << ", "
								  << width << ", " << height << ")"
			  << TestLog::EndMessage;

	tcu::IVec2 offset = getReadOffset();
	m_context.getFunctions().copyTexSubImage2D(target, level, xoffset, yoffset, offset.x()+x, offset.y()+y, width, height);
}

void GLContext::copyTexSubImage3D (deUint32 target, int level, int xoffset, int yoffset, int zoffset, int x, int y, int width, int height)
{
	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glCopyTexSubImage3D("
								  << glu::getTextureTargetStr(target) << ", "
								  << level
								  << ", " << xoffset << ", " << yoffset << ", " << zoffset << ", "
								  << x << ", " << y << ", "
								  << width << ", " << height << ")"
			  << TestLog::EndMessage;

	tcu::IVec2 offset = getReadOffset();
	m_context.getFunctions().copyTexSubImage3D(target, level, xoffset, yoffset, zoffset, offset.x()+x, offset.y()+y, width, height);
}

void GLContext::texStorage2D (deUint32 target, int levels, deUint32 internalFormat, int width, int height)
{
	m_wrapper->glTexStorage2D(target, levels, internalFormat, width, height);
}

void GLContext::texStorage3D (deUint32 target, int levels, deUint32 internalFormat, int width, int height, int depth)
{
	m_wrapper->glTexStorage3D(target, levels, internalFormat, width, height, depth);
}

void GLContext::framebufferTexture2D (deUint32 target, deUint32 attachment, deUint32 textarget, deUint32 texture, int level)
{
	m_wrapper->glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void GLContext::framebufferTextureLayer (deUint32 target, deUint32 attachment, deUint32 texture, int level, int layer)
{
	m_wrapper->glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

void GLContext::framebufferRenderbuffer (deUint32 target, deUint32 attachment, deUint32 renderbuffertarget, deUint32 renderbuffer)
{
	m_wrapper->glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

void GLContext::getFramebufferAttachmentParameteriv (deUint32 target, deUint32 attachment, deUint32 pname, int* params)
{
	m_wrapper->glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

void GLContext::renderbufferStorage (deUint32 target, deUint32 internalformat, int width, int height)
{
	m_wrapper->glRenderbufferStorage(target, internalformat, width, height);
}

void GLContext::renderbufferStorageMultisample (deUint32 target, int samples, deUint32 internalFormat, int width, int height)
{
	m_wrapper->glRenderbufferStorageMultisample(target, samples, internalFormat, width, height);
}

void GLContext::bindBuffer (deUint32 target, deUint32 buffer)
{
	m_allocatedBuffers.insert(buffer);
	m_wrapper->glBindBuffer(target, buffer);
}

void GLContext::genBuffers (int numBuffers, deUint32* buffers)
{
	m_wrapper->glGenBuffers(numBuffers, buffers);
	if (numBuffers > 0)
		m_allocatedBuffers.insert(buffers, buffers+numBuffers);
}

void GLContext::deleteBuffers (int numBuffers, const deUint32* buffers)
{
	m_wrapper->glDeleteBuffers(numBuffers, buffers);
	for (int i = 0; i < numBuffers; i++)
		m_allocatedBuffers.erase(buffers[i]);
}

void GLContext::bufferData (deUint32 target, deIntptr size, const void* data, deUint32 usage)
{
	m_wrapper->glBufferData(target, (glw::GLsizeiptr)size, data, usage);
}

void GLContext::bufferSubData (deUint32 target, deIntptr offset, deIntptr size, const void* data)
{
	m_wrapper->glBufferSubData(target, (glw::GLintptr)offset, (glw::GLsizeiptr)size, data);
}

void GLContext::clearColor (float red, float green, float blue, float alpha)
{
	m_wrapper->glClearColor(red, green, blue, alpha);
}

void GLContext::clearDepthf (float depth)
{
	m_wrapper->glClearDepthf(depth);
}

void GLContext::clearStencil (int stencil)
{
	m_wrapper->glClearStencil(stencil);
}

void GLContext::clear (deUint32 buffers)
{
	m_wrapper->glClear(buffers);
}

void GLContext::clearBufferiv (deUint32 buffer, int drawbuffer, const int* value)
{
	m_wrapper->glClearBufferiv(buffer, drawbuffer, value);
}

void GLContext::clearBufferfv (deUint32 buffer, int drawbuffer, const float* value)
{
	m_wrapper->glClearBufferfv(buffer, drawbuffer, value);
}

void GLContext::clearBufferuiv (deUint32 buffer, int drawbuffer, const deUint32* value)
{
	m_wrapper->glClearBufferuiv(buffer, drawbuffer, value);
}

void GLContext::clearBufferfi (deUint32 buffer, int drawbuffer, float depth, int stencil)
{
	m_wrapper->glClearBufferfi(buffer, drawbuffer, depth, stencil);
}

void GLContext::scissor (int x, int y, int width, int height)
{
	m_curScissor = tcu::IVec4(x, y, width, height);

	// \note For clarity don't add the offset to log
	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glScissor(" << x << ", " << y << ", " << width << ", " << height << ");" << TestLog::EndMessage;

	tcu::IVec2 offset = getDrawOffset();
	m_context.getFunctions().scissor(offset.x()+x, offset.y()+y, width, height);
}

void GLContext::enable (deUint32 cap)
{
	m_wrapper->glEnable(cap);
}

void GLContext::disable (deUint32 cap)
{
	m_wrapper->glDisable(cap);
}

void GLContext::stencilFunc (deUint32 func, int ref, deUint32 mask)
{
	m_wrapper->glStencilFunc(func, ref, mask);
}

void GLContext::stencilOp (deUint32 sfail, deUint32 dpfail, deUint32 dppass)
{
	m_wrapper->glStencilOp(sfail, dpfail, dppass);
}

void GLContext::depthFunc (deUint32 func)
{
	m_wrapper->glDepthFunc(func);
}

void GLContext::depthRangef (float n, float f)
{
	m_wrapper->glDepthRangef(n, f);
}

void GLContext::depthRange (double n, double f)
{
	m_wrapper->glDepthRange(n, f);
}

void GLContext::polygonOffset (float factor, float units)
{
	m_wrapper->glPolygonOffset(factor, units);
}

void GLContext::provokingVertex (deUint32 convention)
{
	m_wrapper->glProvokingVertex(convention);
}

void GLContext::primitiveRestartIndex (deUint32 index)
{
	m_wrapper->glPrimitiveRestartIndex(index);
}

void GLContext::stencilFuncSeparate (deUint32 face, deUint32 func, int ref, deUint32 mask)
{
	m_wrapper->glStencilFuncSeparate(face, func, ref, mask);
}

void GLContext::stencilOpSeparate (deUint32 face, deUint32 sfail, deUint32 dpfail, deUint32 dppass)
{
	m_wrapper->glStencilOpSeparate(face, sfail, dpfail, dppass);
}

void GLContext::blendEquation (deUint32 mode)
{
	m_wrapper->glBlendEquation(mode);
}

void GLContext::blendEquationSeparate (deUint32 modeRGB, deUint32 modeAlpha)
{
	m_wrapper->glBlendEquationSeparate(modeRGB, modeAlpha);
}

void GLContext::blendFunc (deUint32 src, deUint32 dst)
{
	m_wrapper->glBlendFunc(src, dst);
}

void GLContext::blendFuncSeparate (deUint32 srcRGB, deUint32 dstRGB, deUint32 srcAlpha, deUint32 dstAlpha)
{
	m_wrapper->glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void GLContext::blendColor (float red, float green, float blue, float alpha)
{
	m_wrapper->glBlendColor(red, green, blue, alpha);
}

void GLContext::colorMask (deBool r, deBool g, deBool b, deBool a)
{
	m_wrapper->glColorMask((glw::GLboolean)r, (glw::GLboolean)g, (glw::GLboolean)b, (glw::GLboolean)a);
}

void GLContext::depthMask (deBool mask)
{
	m_wrapper->glDepthMask((glw::GLboolean)mask);
}

void GLContext::stencilMask (deUint32 mask)
{
	m_wrapper->glStencilMask(mask);
}

void GLContext::stencilMaskSeparate (deUint32 face, deUint32 mask)
{
	m_wrapper->glStencilMaskSeparate(face, mask);
}

void GLContext::blitFramebuffer (int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, deUint32 mask, deUint32 filter)
{
	tcu::IVec2	drawOffset	= getDrawOffset();
	tcu::IVec2	readOffset	= getReadOffset();

	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glBlitFramebuffer("
								  << srcX0 << ", " << srcY0 << ", " << srcX1 << ", " << srcY1 << ", "
								  << dstX0 << ", " << dstY0 << ", " << dstX1 << ", " << dstY1 << ", "
								  << glu::getBufferMaskStr(mask) << ", "
								  << glu::getTextureFilterStr(filter) << ")"
			  << TestLog::EndMessage;

	m_context.getFunctions().blitFramebuffer(readOffset.x()+srcX0, readOffset.y()+srcY0, readOffset.x()+srcX1, readOffset.y()+srcY1,
											 drawOffset.x()+dstX0, drawOffset.y()+dstY0, drawOffset.x()+dstX1, drawOffset.y()+dstY1,
											 mask, filter);
}

void GLContext::invalidateSubFramebuffer (deUint32 target, int numAttachments, const deUint32* attachments, int x, int y, int width, int height)
{
	tcu::IVec2 drawOffset = getDrawOffset();

	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glInvalidateSubFramebuffer("
								  << glu::getFramebufferTargetStr(target) << ", " << numAttachments << ", "
								  << glu::getInvalidateAttachmentStr(attachments, numAttachments) << ", "
								  << x << ", " << y << ", " << width << ", " << height << ")"
			  << TestLog::EndMessage;

	m_context.getFunctions().invalidateSubFramebuffer(target, numAttachments, attachments, x+drawOffset.x(), y+drawOffset.y(), width, height);
}

void GLContext::invalidateFramebuffer (deUint32 target, int numAttachments, const deUint32* attachments)
{
	m_wrapper->glInvalidateFramebuffer(target, numAttachments, attachments);
}

void GLContext::bindVertexArray (deUint32 array)
{
	m_wrapper->glBindVertexArray(array);
}

void GLContext::genVertexArrays (int numArrays, deUint32* vertexArrays)
{
	m_wrapper->glGenVertexArrays(numArrays, vertexArrays);
	if (numArrays > 0)
		m_allocatedVaos.insert(vertexArrays, vertexArrays+numArrays);
}

void GLContext::deleteVertexArrays (int numArrays, const deUint32* vertexArrays)
{
	for (int i = 0; i < numArrays; i++)
		m_allocatedVaos.erase(vertexArrays[i]);
	m_wrapper->glDeleteVertexArrays(numArrays, vertexArrays);
}

void GLContext::vertexAttribPointer (deUint32 index, int size, deUint32 type, deBool normalized, int stride, const void *pointer)
{
	m_wrapper->glVertexAttribPointer(index, size, type, (glw::GLboolean)normalized, stride, pointer);
}

void GLContext::vertexAttribIPointer (deUint32 index, int size, deUint32 type, int stride, const void *pointer)
{
	m_wrapper->glVertexAttribIPointer(index, size, type, stride, pointer);
}

void GLContext::enableVertexAttribArray (deUint32 index)
{
	m_wrapper->glEnableVertexAttribArray(index);
}

void GLContext::disableVertexAttribArray (deUint32 index)
{
	m_wrapper->glDisableVertexAttribArray(index);
}

void GLContext::vertexAttribDivisor (deUint32 index, deUint32 divisor)
{
	m_wrapper->glVertexAttribDivisor(index, divisor);
}

void GLContext::vertexAttrib1f (deUint32 index, float x)
{
	m_wrapper->glVertexAttrib1f(index, x);
}

void GLContext::vertexAttrib2f (deUint32 index, float x, float y)
{
	m_wrapper->glVertexAttrib2f(index, x, y);
}

void GLContext::vertexAttrib3f (deUint32 index, float x, float y, float z)
{
	m_wrapper->glVertexAttrib3f(index, x, y, z);
}

void GLContext::vertexAttrib4f (deUint32 index, float x, float y, float z, float w)
{
	m_wrapper->glVertexAttrib4f(index, x, y, z, w);
}

void GLContext::vertexAttribI4i (deUint32 index, deInt32 x, deInt32 y, deInt32 z, deInt32 w)
{
	m_wrapper->glVertexAttribI4i(index, x, y, z, w);
}

void GLContext::vertexAttribI4ui (deUint32 index, deUint32 x, deUint32 y, deUint32 z, deUint32 w)
{
	m_wrapper->glVertexAttribI4ui(index, x, y, z, w);
}

deInt32 GLContext::getAttribLocation (deUint32 program, const char *name)
{
	return m_wrapper->glGetAttribLocation(program, name);
}

void GLContext::uniform1f (deInt32 location, float v0)
{
	m_wrapper->glUniform1f(location, v0);
}

void GLContext::uniform1i (deInt32 location, deInt32 v0)
{
	m_wrapper->glUniform1i(location, v0);
}

void GLContext::uniform1fv (deInt32 location, deInt32 count, const float* value)
{
	m_wrapper->glUniform1fv(location, count, value);
}

void GLContext::uniform2fv (deInt32 location, deInt32 count, const float* value)
{
	m_wrapper->glUniform2fv(location, count, value);
}

void GLContext::uniform3fv (deInt32 location, deInt32 count, const float* value)
{
	m_wrapper->glUniform3fv(location, count, value);
}

void GLContext::uniform4fv (deInt32 location, deInt32 count, const float* value)
{
	m_wrapper->glUniform4fv(location, count, value);
}

void GLContext::uniform1iv (deInt32 location, deInt32 count, const deInt32* value)
{
	m_wrapper->glUniform1iv(location, count, value);
}

void GLContext::uniform2iv (deInt32 location, deInt32 count, const deInt32* value)
{
	m_wrapper->glUniform2iv(location, count, value);
}

void GLContext::uniform3iv (deInt32 location, deInt32 count, const deInt32* value)
{
	m_wrapper->glUniform3iv(location, count, value);
}

void GLContext::uniform4iv (deInt32 location, deInt32 count, const deInt32* value)
{
	m_wrapper->glUniform4iv(location, count, value);
}

void GLContext::uniformMatrix3fv (deInt32 location, deInt32 count, deBool transpose, const float *value)
{
	m_wrapper->glUniformMatrix3fv(location, count, (glw::GLboolean)transpose, value);
}

void GLContext::uniformMatrix4fv (deInt32 location, deInt32 count, deBool transpose, const float *value)
{
	m_wrapper->glUniformMatrix4fv(location, count, (glw::GLboolean)transpose, value);
}
deInt32 GLContext::getUniformLocation (deUint32 program, const char *name)
{
	return m_wrapper->glGetUniformLocation(program, name);
}

void GLContext::lineWidth (float w)
{
	m_wrapper->glLineWidth(w);
}

void GLContext::drawArrays (deUint32 mode, int first, int count)
{
	m_wrapper->glDrawArrays(mode, first, count);
}

void GLContext::drawArraysInstanced (deUint32 mode, int first, int count, int instanceCount)
{
	m_wrapper->glDrawArraysInstanced(mode, first, count, instanceCount);
}

void GLContext::drawElements (deUint32 mode, int count, deUint32 type, const void *indices)
{
	m_wrapper->glDrawElements(mode, count, type, indices);
}

void GLContext::drawElementsInstanced (deUint32 mode, int count, deUint32 type, const void *indices, int instanceCount)
{
	m_wrapper->glDrawElementsInstanced(mode, count, type, indices, instanceCount);
}

void GLContext::drawElementsBaseVertex (deUint32 mode, int count, deUint32 type, const void *indices, int baseVertex)
{
	m_wrapper->glDrawElementsBaseVertex(mode, count, type, indices, baseVertex);
}

void GLContext::drawElementsInstancedBaseVertex (deUint32 mode, int count, deUint32 type, const void *indices, int instanceCount, int baseVertex)
{
	m_wrapper->glDrawElementsInstancedBaseVertex(mode, count, type, indices, instanceCount, baseVertex);
}

void GLContext::drawRangeElements (deUint32 mode, deUint32 start, deUint32 end, int count, deUint32 type, const void *indices)
{
	m_wrapper->glDrawRangeElements(mode, start, end, count, type, indices);
}

void GLContext::drawRangeElementsBaseVertex (deUint32 mode, deUint32 start, deUint32 end, int count, deUint32 type, const void *indices, int baseVertex)
{
	m_wrapper->glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, baseVertex);
}

void GLContext::drawArraysIndirect (deUint32 mode, const void *indirect)
{
	m_wrapper->glDrawArraysIndirect(mode, indirect);
}

void GLContext::drawElementsIndirect (deUint32 mode, deUint32 type, const void *indirect)
{
	m_wrapper->glDrawElementsIndirect(mode, type, indirect);
}

void GLContext::multiDrawArrays (deUint32 mode, const int* first, const int* count, int primCount)
{
	m_wrapper->glMultiDrawArrays(mode, first, count, primCount);
}

void GLContext::multiDrawElements (deUint32 mode, const int* count, deUint32 type, const void** indices, int primCount)
{
	m_wrapper->glMultiDrawElements(mode, count, type, indices, primCount);
}

void GLContext::multiDrawElementsBaseVertex (deUint32 mode, const int* count, deUint32 type, const void** indices, int primCount, const int* baseVertex)
{
	m_wrapper->glMultiDrawElementsBaseVertex(mode, count, type, indices, primCount, baseVertex);
}

deUint32 GLContext::createProgram (ShaderProgram* shader)
{
	m_programs.reserve(m_programs.size()+1);

	glu::ShaderProgram* program = DE_NULL;

	if (!shader->m_hasGeometryShader)
		program = new glu::ShaderProgram(m_context, glu::makeVtxFragSources(shader->m_vertSrc, shader->m_fragSrc));
	else
		program = new glu::ShaderProgram(m_context,
										 glu::ProgramSources() << glu::VertexSource(shader->m_vertSrc)
															   << glu::FragmentSource(shader->m_fragSrc)
															   << glu::GeometrySource(shader->m_geomSrc));

	if (!program->isOk())
	{
		m_log << *program;
		delete program;
		TCU_FAIL("Compile failed");
	}

	if ((m_logFlags & GLCONTEXT_LOG_PROGRAMS) != 0)
		m_log << *program;

	m_programs.push_back(program);
	return program->getProgram();
}

void GLContext::deleteProgram (deUint32 program)
{
	for (std::vector<glu::ShaderProgram*>::iterator i = m_programs.begin(); i != m_programs.end(); i++)
	{
		if ((*i)->getProgram() == program)
		{
			delete *i;
			m_programs.erase(i);
			return;
		}
	}

	DE_FATAL("invalid delete");
}

void GLContext::useProgram (deUint32 program)
{
	m_wrapper->glUseProgram(program);
}

void GLContext::readPixels (int x, int y, int width, int height, deUint32 format, deUint32 type, void* data)
{
	// Don't log offset.
	if ((m_logFlags & GLCONTEXT_LOG_CALLS) != 0)
		m_log << TestLog::Message << "glReadPixels("
								  << x << ", " << y << ", " << width << ", " << height << ", "
								  << glu::getTextureFormatStr(format) << ", "
								  << glu::getTypeStr(type) << ", " << data << ")"
			  << TestLog::EndMessage;

	tcu::IVec2 offset = getReadOffset();
	m_context.getFunctions().readPixels(x+offset.x(), y+offset.y(), width, height, format, type, data);
}

deUint32 GLContext::getError (void)
{
	return m_wrapper->glGetError();
}

void GLContext::finish (void)
{
	m_wrapper->glFinish();
}

void GLContext::getIntegerv (deUint32 pname, int* params)
{
	m_wrapper->glGetIntegerv(pname, params);
}

const char* GLContext::getString (deUint32 pname)
{
	return (const char*)m_wrapper->glGetString(pname);
}

} // sglr
