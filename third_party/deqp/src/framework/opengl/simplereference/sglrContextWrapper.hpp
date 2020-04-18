#ifndef _SGLRCONTEXTWRAPPER_HPP
#define _SGLRCONTEXTWRAPPER_HPP
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

#include "tcuDefs.hpp"
#include "tcuVector.hpp"

namespace sglr
{

class Shader;
class Context;

class ContextWrapper
{
public:
					ContextWrapper							(void);
					~ContextWrapper							(void);

	void			setContext								(Context* context);
	Context*		getCurrentContext						(void) const;

	int				getWidth								(void) const;
	int				getHeight								(void) const;

	// GL-compatible API.
	void			glActiveTexture							(deUint32 texture);
	void			glAttachShader							(deUint32 program, deUint32 shader);
	void			glBindAttribLocation					(deUint32 program, deUint32 index, const char* name);
	void			glBindBuffer							(deUint32 target, deUint32 buffer);
	void			glBindFramebuffer						(deUint32 target, deUint32 framebuffer);
	void			glBindRenderbuffer						(deUint32 target, deUint32 renderbuffer);
	void			glBindTexture							(deUint32 target, deUint32 texture);
	void			glBlendColor							(float red, float green, float blue, float alpha);
	void			glBlendEquation							(deUint32 mode);
	void			glBlendEquationSeparate					(deUint32 modeRGB, deUint32 modeAlpha);
	void			glBlendFunc								(deUint32 sfactor, deUint32 dfactor);
	void			glBlendFuncSeparate						(deUint32 srcRGB, deUint32 dstRGB, deUint32 srcAlpha, deUint32 dstAlpha);
	void			glBufferData							(deUint32 target, deIntptr size, const void* data, deUint32 usage);
	void			glBufferSubData							(deUint32 target, deIntptr offset, deIntptr size, const void* data);
	deUint32		glCheckFramebufferStatus				(deUint32 target);
	void			glClear									(deUint32 mask);
	void			glClearColor							(float red, float green, float blue, float alpha);
	void			glClearDepthf							(float depth);
	void			glClearStencil							(int s);
	void			glColorMask								(deBool red, deBool green, deBool blue, deBool alpha);
	void			glCompileShader							(deUint32 shader);
	void			glCompressedTexImage2D					(deUint32 target, int level, deUint32 internalformat, int width, int height, int border, int imageSize, const void* data);
	void			glCompressedTexSubImage2D				(deUint32 target, int level, int xoffset, int yoffset, int width, int height, deUint32 format, int imageSize, const void* data);
	void			glCopyTexImage1D						(deUint32 target, int level, deUint32 internalformat, int x, int y, int width, int border);
	void			glCopyTexImage2D						(deUint32 target, int level, deUint32 internalformat, int x, int y, int width, int height, int border);
	void			glCopyTexSubImage1D						(deUint32 target, int level, int xoffset, int x, int y, int width);
	void			glCopyTexSubImage2D						(deUint32 target, int level, int xoffset, int yoffset, int x, int y, int width, int height);
	deUint32		glCreateProgram							();
	deUint32		glCreateShader							(deUint32 type);
	void			glCullFace								(deUint32 mode);
	void			glDeleteBuffers							(int n, const deUint32* buffers);
	void			glDeleteFramebuffers					(int n, const deUint32* framebuffers);
	void			glDeleteProgram							(deUint32 program);
	void			glDeleteRenderbuffers					(int n, const deUint32* renderbuffers);
	void			glDeleteShader							(deUint32 shader);
	void			glDeleteTextures						(int n, const deUint32* textures);
	void			glDepthFunc								(deUint32 func);
	void			glDepthMask								(deBool flag);
	void			glDepthRangef							(float n, float f);
	void			glDetachShader							(deUint32 program, deUint32 shader);
	void			glDisable								(deUint32 cap);
	void			glDisableVertexAttribArray				(deUint32 index);
	void			glDrawArrays							(deUint32 mode, int first, int count);
	void			glDrawElements							(deUint32 mode, int count, deUint32 type, const void* indices);
	void			glEnable								(deUint32 cap);
	void			glEnableVertexAttribArray				(deUint32 index);
	void			glFinish								();
	void			glFlush									();
	void			glFramebufferRenderbuffer				(deUint32 target, deUint32 attachment, deUint32 renderbuffertarget, deUint32 renderbuffer);
	void			glFramebufferTexture2D					(deUint32 target, deUint32 attachment, deUint32 textarget, deUint32 texture, int level);
	void			glFrontFace								(deUint32 mode);
	void			glGenBuffers							(int n, deUint32* buffers);
	void			glGenerateMipmap						(deUint32 target);
	void			glGenFramebuffers						(int n, deUint32* framebuffers);
	void			glGenRenderbuffers						(int n, deUint32* renderbuffers);
	void			glGenTextures							(int n, deUint32* textures);
	void			glGetActiveAttrib						(deUint32 program, deUint32 index, int bufsize, int* length, int* size, deUint32* type, char* name);
	void			glGetActiveUniform						(deUint32 program, deUint32 index, int bufsize, int* length, int* size, deUint32* type, char* name);
	void			glGetAttachedShaders					(deUint32 program, int maxcount, int* count, deUint32* shaders);
	int				glGetAttribLocation						(deUint32 program, const char* name);
	void			glGetBooleanv							(deUint32 pname, deBool* params);
	void			glGetBufferParameteriv					(deUint32 target, deUint32 pname, int* params);
	deUint32		glGetError								();
	void			glGetFloatv								(deUint32 pname, float* params);
	void			glGetFramebufferAttachmentParameteriv	(deUint32 target, deUint32 attachment, deUint32 pname, int* params);
	void			glGetIntegerv							(deUint32 pname, int* params);
	void			glGetProgramiv							(deUint32 program, deUint32 pname, int* params);
	void			glGetProgramInfoLog						(deUint32 program, int bufsize, int* length, char* infolog);
	void			glGetRenderbufferParameteriv			(deUint32 target, deUint32 pname, int* params);
	void			glGetShaderiv							(deUint32 shader, deUint32 pname, int* params);
	void			glGetShaderInfoLog						(deUint32 shader, int bufsize, int* length, char* infolog);
	void			glGetShaderPrecisionFormat				(deUint32 shadertype, deUint32 precisiontype, int* range, int* precision);
	void			glGetShaderSource						(deUint32 shader, int bufsize, int* length, char* source);
	const deUint8*	glGetString								(deUint32 name);
	void			glGetTexParameterfv						(deUint32 target, deUint32 pname, float* params);
	void			glGetTexParameteriv						(deUint32 target, deUint32 pname, int* params);
	void			glGetUniformfv							(deUint32 program, int location, float* params);
	void			glGetUniformiv							(deUint32 program, int location, int* params);
	int				glGetUniformLocation					(deUint32 program, const char* name);
	void			glGetVertexAttribfv						(deUint32 index, deUint32 pname, float* params);
	void			glGetVertexAttribiv						(deUint32 index, deUint32 pname, int* params);
	void			glGetVertexAttribPointerv				(deUint32 index, deUint32 pname, void** pointer);
	void			glHint									(deUint32 target, deUint32 mode);
	deBool			glIsBuffer								(deUint32 buffer);
	deBool			glIsEnabled								(deUint32 cap);
	deBool			glIsFramebuffer							(deUint32 framebuffer);
	deBool			glIsProgram								(deUint32 program);
	deBool			glIsRenderbuffer						(deUint32 renderbuffer);
	deBool			glIsShader								(deUint32 shader);
	deBool			glIsTexture								(deUint32 texture);
	void			glLineWidth								(float width);
	void			glLinkProgram							(deUint32 program);
	void			glPixelStorei							(deUint32 pname, int param);
	void			glPolygonOffset							(float factor, float units);
	void			glReadPixels							(int x, int y, int width, int height, deUint32 format, deUint32 type, void* pixels);
	void			glReleaseShaderCompiler					();
	void			glRenderbufferStorage					(deUint32 target, deUint32 internalformat, int width, int height);
	void			glSampleCoverage						(float value, deBool invert);
	void			glScissor								(int x, int y, int width, int height);
	void			glShaderBinary							(int n, const deUint32* shaders, deUint32 binaryformat, const void* binary, int length);
	void			glShaderSource							(deUint32 shader, int count, const char* const* string, const int* length);
	void			glStencilFunc							(deUint32 func, int ref, deUint32 mask);
	void			glStencilFuncSeparate					(deUint32 face, deUint32 func, int ref, deUint32 mask);
	void			glStencilMask							(deUint32 mask);
	void			glStencilMaskSeparate					(deUint32 face, deUint32 mask);
	void			glStencilOp								(deUint32 fail, deUint32 zfail, deUint32 zpass);
	void			glStencilOpSeparate						(deUint32 face, deUint32 fail, deUint32 zfail, deUint32 zpass);
	void			glTexImage1D							(deUint32 target, int level, int internalformat, int width, int border, deUint32 format, deUint32 type, const void* pixels);
	void			glTexImage2D							(deUint32 target, int level, int internalformat, int width, int height, int border, deUint32 format, deUint32 type, const void* pixels);
	void			glTexParameterf							(deUint32 target, deUint32 pname, float param);
	void			glTexParameterfv						(deUint32 target, deUint32 pname, const float* params);
	void			glTexParameteri							(deUint32 target, deUint32 pname, int param);
	void			glTexParameteriv						(deUint32 target, deUint32 pname, const int* params);
	void			glTexSubImage1D							(deUint32 target, int level, int xoffset, int width, deUint32 format, deUint32 type, const void* pixels);
	void			glTexSubImage2D							(deUint32 target, int level, int xoffset, int yoffset, int width, int height, deUint32 format, deUint32 type, const void* pixels);
	void			glUniform1f								(int location, float x);
	void			glUniform1fv							(int location, int count, const float* v);
	void			glUniform1i								(int location, int x);
	void			glUniform1iv							(int location, int count, const int* v);
	void			glUniform2f								(int location, float x, float y);
	void			glUniform2fv							(int location, int count, const float* v);
	void			glUniform2i								(int location, int x, int y);
	void			glUniform2iv							(int location, int count, const int* v);
	void			glUniform3f								(int location, float x, float y, float z);
	void			glUniform3fv							(int location, int count, const float* v);
	void			glUniform3i								(int location, int x, int y, int z);
	void			glUniform3iv							(int location, int count, const int* v);
	void			glUniform4f								(int location, float x, float y, float z, float w);
	void			glUniform4fv							(int location, int count, const float* v);
	void			glUniform4i								(int location, int x, int y, int z, int w);
	void			glUniform4iv							(int location, int count, const int* v);
	void			glUniformMatrix2fv						(int location, int count, deBool transpose, const float* value);
	void			glUniformMatrix3fv						(int location, int count, deBool transpose, const float* value);
	void			glUniformMatrix4fv						(int location, int count, deBool transpose, const float* value);
	void			glUseProgram							(deUint32 program);
	void			glValidateProgram						(deUint32 program);
	void			glVertexAttrib1f						(deUint32 indx, float x);
	void			glVertexAttrib1fv						(deUint32 indx, const float* values);
	void			glVertexAttrib2f						(deUint32 indx, float x, float y);
	void			glVertexAttrib2fv						(deUint32 indx, const float* values);
	void			glVertexAttrib3f						(deUint32 indx, float x, float y, float z);
	void			glVertexAttrib3fv						(deUint32 indx, const float* values);
	void			glVertexAttrib4f						(deUint32 indx, float x, float y, float z, float w);
	void			glVertexAttrib4fv						(deUint32 indx, const float* values);
	void			glVertexAttribPointer					(deUint32 indx, int size, deUint32 type, deBool normalized, int stride, const void* ptr);
	void			glViewport								(int x, int y, int width, int height);
	void			glReadBuffer							(deUint32 mode);
	void			glDrawRangeElements						(deUint32 mode, deUint32 start, deUint32 end, int count, deUint32 type, const void* indices);
	void			glTexImage3D							(deUint32 target, int level, int internalformat, int width, int height, int depth, int border, deUint32 format, deUint32 type, const void* pixels);
	void			glTexSubImage3D							(deUint32 target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, deUint32 format, deUint32 type, const void* pixels);
	void			glCopyTexSubImage3D						(deUint32 target, int level, int xoffset, int yoffset, int zoffset, int x, int y, int width, int height);
	void			glCompressedTexImage3D					(deUint32 target, int level, deUint32 internalformat, int width, int height, int depth, int border, int imageSize, const void* data);
	void			glCompressedTexSubImage3D				(deUint32 target, int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, deUint32 format, int imageSize, const void* data);
	void			glGenQueries							(int n, deUint32* ids);
	void			glDeleteQueries							(int n, const deUint32* ids);
	deBool			glIsQuery								(deUint32 id);
	void			glBeginQuery							(deUint32 target, deUint32 id);
	void			glEndQuery								(deUint32 target);
	void			glGetQueryiv							(deUint32 target, deUint32 pname, int* params);
	void			glGetQueryObjectuiv						(deUint32 id, deUint32 pname, deUint32* params);
	deBool			glUnmapBuffer							(deUint32 target);
	void			glGetBufferPointerv						(deUint32 target, deUint32 pname, void** params);
	void			glDrawBuffers							(int n, const deUint32* bufs);
	void			glUniformMatrix2x3fv					(int location, int count, deBool transpose, const float* value);
	void			glUniformMatrix3x2fv					(int location, int count, deBool transpose, const float* value);
	void			glUniformMatrix2x4fv					(int location, int count, deBool transpose, const float* value);
	void			glUniformMatrix4x2fv					(int location, int count, deBool transpose, const float* value);
	void			glUniformMatrix3x4fv					(int location, int count, deBool transpose, const float* value);
	void			glUniformMatrix4x3fv					(int location, int count, deBool transpose, const float* value);
	void			glBlitFramebuffer						(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, deUint32 mask, deUint32 filter);
	void			glRenderbufferStorageMultisample		(deUint32 target, int samples, deUint32 internalformat, int width, int height);
	void			glFramebufferTextureLayer				(deUint32 target, deUint32 attachment, deUint32 texture, int level, int layer);
	void*			glMapBufferRange						(deUint32 target, deIntptr offset, deIntptr length, deUint32 access);
	void			glFlushMappedBufferRange				(deUint32 target, deIntptr offset, deIntptr length);
	void			glBindVertexArray						(deUint32 array);
	void			glDeleteVertexArrays					(int n, const deUint32* arrays);
	void			glGenVertexArrays						(int n, deUint32* arrays);
	deBool			glIsVertexArray							(deUint32 array);
	void			glGetIntegeri_v							(deUint32 target, deUint32 index, int* data);
	void			glBeginTransformFeedback				(deUint32 primitiveMode);
	void			glEndTransformFeedback					();
	void			glBindBufferRange						(deUint32 target, deUint32 index, deUint32 buffer, deIntptr offset, deIntptr size);
	void			glBindBufferBase						(deUint32 target, deUint32 index, deUint32 buffer);
	void			glTransformFeedbackVaryings				(deUint32 program, int count, const char* const* varyings, deUint32 bufferMode);
	void			glGetTransformFeedbackVarying			(deUint32 program, deUint32 index, int bufSize, int* length, int* size, deUint32* type, char* name);
	void			glVertexAttribIPointer					(deUint32 index, int size, deUint32 type, int stride, const void* pointer);
	void			glGetVertexAttribIiv					(deUint32 index, deUint32 pname, int* params);
	void			glGetVertexAttribIuiv					(deUint32 index, deUint32 pname, deUint32* params);
	void			glVertexAttribI4i						(deUint32 index, int x, int y, int z, int w);
	void			glVertexAttribI4ui						(deUint32 index, deUint32 x, deUint32 y, deUint32 z, deUint32 w);
	void			glVertexAttribI4iv						(deUint32 index, const int* v);
	void			glVertexAttribI4uiv						(deUint32 index, const deUint32* v);
	void			glGetUniformuiv							(deUint32 program, int location, deUint32* params);
	int				glGetFragDataLocation					(deUint32 program, const char* name);
	void			glUniform1ui							(int location, deUint32 v0);
	void			glUniform2ui							(int location, deUint32 v0, deUint32 v1);
	void			glUniform3ui							(int location, deUint32 v0, deUint32 v1, deUint32 v2);
	void			glUniform4ui							(int location, deUint32 v0, deUint32 v1, deUint32 v2, deUint32 v3);
	void			glUniform1uiv							(int location, int count, const deUint32* value);
	void			glUniform2uiv							(int location, int count, const deUint32* value);
	void			glUniform3uiv							(int location, int count, const deUint32* value);
	void			glUniform4uiv							(int location, int count, const deUint32* value);
	void			glClearBufferiv							(deUint32 buffer, int drawbuffer, const int* value);
	void			glClearBufferuiv						(deUint32 buffer, int drawbuffer, const deUint32* value);
	void			glClearBufferfv							(deUint32 buffer, int drawbuffer, const float* value);
	void			glClearBufferfi							(deUint32 buffer, int drawbuffer, float depth, int stencil);
	const deUint8*	glGetStringi							(deUint32 name, deUint32 index);
	void			glCopyBufferSubData						(deUint32 readTarget, deUint32 writeTarget, deIntptr readOffset, deIntptr writeOffset, deIntptr size);
	void			glGetUniformIndices						(deUint32 program, int uniformCount, const char* const* uniformNames, deUint32* uniformIndices);
	void			glGetActiveUniformsiv					(deUint32 program, int uniformCount, const deUint32* uniformIndices, deUint32 pname, int* params);
	deUint32		glGetUniformBlockIndex					(deUint32 program, const char* uniformBlockName);
	void			glGetActiveUniformBlockiv				(deUint32 program, deUint32 uniformBlockIndex, deUint32 pname, int* params);
	void			glGetActiveUniformBlockName				(deUint32 program, deUint32 uniformBlockIndex, int bufSize, int* length, char* uniformBlockName);
	void			glUniformBlockBinding					(deUint32 program, deUint32 uniformBlockIndex, deUint32 uniformBlockBinding);
	void			glDrawArraysInstanced					(deUint32 mode, int first, int count, int primcount);
	void			glDrawElementsInstanced					(deUint32 mode, int count, deUint32 type, const void* indices, int primcount);
	void*			glFenceSync								(deUint32 condition, deUint32 flags);
	deBool			glIsSync								(void* sync);
	void			glDeleteSync							(void* sync);
	deUint32		glClientWaitSync						(void* sync, deUint32 flags, deUint64 timeout);
	void			glWaitSync								(void* sync, deUint32 flags, deUint64 timeout);
	void			glGetInteger64v							(deUint32 pname, deInt64* params);
	void			glGetSynciv								(void* sync, deUint32 pname, int bufSize, int* length, int* values);
	void			glGetInteger64i_v						(deUint32 target, deUint32 index, deInt64* data);
	void			glGetBufferParameteri64v				(deUint32 target, deUint32 pname, deInt64* params);
	void			glGenSamplers							(int count, deUint32* samplers);
	void			glDeleteSamplers						(int count, const deUint32* samplers);
	deBool			glIsSampler								(deUint32 sampler);
	void			glBindSampler							(deUint32 unit, deUint32 sampler);
	void			glSamplerParameteri						(deUint32 sampler, deUint32 pname, int param);
	void			glSamplerParameteriv					(deUint32 sampler, deUint32 pname, const int* param);
	void			glSamplerParameterf						(deUint32 sampler, deUint32 pname, float param);
	void			glSamplerParameterfv					(deUint32 sampler, deUint32 pname, const float* param);
	void			glGetSamplerParameteriv					(deUint32 sampler, deUint32 pname, int* params);
	void			glGetSamplerParameterfv					(deUint32 sampler, deUint32 pname, float* params);
	void			glVertexAttribDivisor					(deUint32 index, deUint32 divisor);
	void			glBindTransformFeedback					(deUint32 target, deUint32 id);
	void			glDeleteTransformFeedbacks				(int n, const deUint32* ids);
	void			glGenTransformFeedbacks					(int n, deUint32* ids);
	deBool			glIsTransformFeedback					(deUint32 id);
	void			glPauseTransformFeedback				();
	void			glResumeTransformFeedback				();
	void			glGetProgramBinary						(deUint32 program, int bufSize, int* length, deUint32* binaryFormat, void* binary);
	void			glProgramBinary							(deUint32 program, deUint32 binaryFormat, const void* binary, int length);
	void			glProgramParameteri						(deUint32 program, deUint32 pname, int value);
	void			glInvalidateFramebuffer					(deUint32 target, int numAttachments, const deUint32* attachments);
	void			glInvalidateSubFramebuffer				(deUint32 target, int numAttachments, const deUint32* attachments, int x, int y, int width, int height);
	void			glTexStorage2D							(deUint32 target, int levels, deUint32 internalformat, int width, int height);
	void			glTexStorage3D							(deUint32 target, int levels, deUint32 internalformat, int width, int height, int depth);
	void			glGetInternalformativ					(deUint32 target, deUint32 internalformat, deUint32 pname, int bufSize, int* params);

private:
	Context*		m_curCtx;
};

} // sglr

#endif // _SGLRCONTEXTWRAPPER_HPP
