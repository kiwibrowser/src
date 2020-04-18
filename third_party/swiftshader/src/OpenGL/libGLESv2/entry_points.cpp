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

// entry_points.cpp: GL entry points exports and definition

#include "main.h"

#include "libEGL/main.h"

namespace es2
{
void ActiveTexture(GLenum texture);
void AttachShader(GLuint program, GLuint shader);
void BeginQueryEXT(GLenum target, GLuint name);
void BindAttribLocation(GLuint program, GLuint index, const GLchar* name);
void BindBuffer(GLenum target, GLuint buffer);
void BindFramebuffer(GLenum target, GLuint framebuffer);
void BindRenderbuffer(GLenum target, GLuint renderbuffer);
void BindTexture(GLenum target, GLuint texture);
void BlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void BlendEquation(GLenum mode);
void BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
void BlendFunc(GLenum sfactor, GLenum dfactor);
void BlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
GLenum CheckFramebufferStatus(GLenum target);
void Clear(GLbitfield mask);
void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ClearDepthf(GLclampf depth);
void ClearStencil(GLint s);
void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void CompileShader(GLuint shader);
void CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                          GLint border, GLsizei imageSize, const GLvoid* data);
void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                             GLenum format, GLsizei imageSize, const GLvoid* data);
void CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GLuint CreateProgram(void);
GLuint CreateShader(GLenum type);
void CullFace(GLenum mode);
void DeleteBuffers(GLsizei n, const GLuint* buffers);
void DeleteFencesNV(GLsizei n, const GLuint* fences);
void DeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
void DeleteProgram(GLuint program);
void DeleteQueriesEXT(GLsizei n, const GLuint *ids);
void DeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
void DeleteShader(GLuint shader);
void DeleteTextures(GLsizei n, const GLuint* textures);
void DepthFunc(GLenum func);
void DepthMask(GLboolean flag);
void DepthRangef(GLclampf zNear, GLclampf zFar);
void DetachShader(GLuint program, GLuint shader);
void Disable(GLenum cap);
void DisableVertexAttribArray(GLuint index);
void DrawArrays(GLenum mode, GLint first, GLsizei count);
void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
void DrawArraysInstancedEXT(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
void DrawElementsInstancedEXT(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount);
void VertexAttribDivisorEXT(GLuint index, GLuint divisor);
void DrawArraysInstancedANGLE(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
void DrawElementsInstancedANGLE(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount);
void VertexAttribDivisorANGLE(GLuint index, GLuint divisor);
void Enable(GLenum cap);
void EnableVertexAttribArray(GLuint index);
void EndQueryEXT(GLenum target);
void FinishFenceNV(GLuint fence);
void Finish(void);
void Flush(void);
void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void FrontFace(GLenum mode);
void GenBuffers(GLsizei n, GLuint* buffers);
void GenerateMipmap(GLenum target);
void GenFencesNV(GLsizei n, GLuint* fences);
void GenFramebuffers(GLsizei n, GLuint* framebuffers);
void GenQueriesEXT(GLsizei n, GLuint* ids);
void GenRenderbuffers(GLsizei n, GLuint* renderbuffers);
void GenTextures(GLsizei n, GLuint* textures);
void GetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void GetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
void GetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
int GetAttribLocation(GLuint program, const GLchar* name);
void GetBooleanv(GLenum pname, GLboolean* params);
void GetBufferParameteriv(GLenum target, GLenum pname, GLint* params);
GLenum GetError(void);
void GetFenceivNV(GLuint fence, GLenum pname, GLint *params);
void GetFloatv(GLenum pname, GLfloat* params);
void GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params);
GLenum GetGraphicsResetStatusEXT(void);
void GetIntegerv(GLenum pname, GLint* params);
void GetProgramiv(GLuint program, GLenum pname, GLint* params);
void GetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
void GetQueryivEXT(GLenum target, GLenum pname, GLint *params);
void GetQueryObjectuivEXT(GLuint name, GLenum pname, GLuint *params);
void GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params);
void GetShaderiv(GLuint shader, GLenum pname, GLint* params);
void GetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
void GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
void GetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
const GLubyte* GetString(GLenum name);
void GetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
void GetTexParameteriv(GLenum target, GLenum pname, GLint* params);
void GetnUniformfvEXT(GLuint program, GLint location, GLsizei bufSize, GLfloat* params);
void GetUniformfv(GLuint program, GLint location, GLfloat* params);
void GetnUniformivEXT(GLuint program, GLint location, GLsizei bufSize, GLint* params);
void GetUniformiv(GLuint program, GLint location, GLint* params);
int GetUniformLocation(GLuint program, const GLchar* name);
void GetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params);
void GetVertexAttribiv(GLuint index, GLenum pname, GLint* params);
void GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer);
void Hint(GLenum target, GLenum mode);
GLboolean IsBuffer(GLuint buffer);
GLboolean IsEnabled(GLenum cap);
GLboolean IsFenceNV(GLuint fence);
GLboolean IsFramebuffer(GLuint framebuffer);
GLboolean IsProgram(GLuint program);
GLboolean IsQueryEXT(GLuint name);
GLboolean IsRenderbuffer(GLuint renderbuffer);
GLboolean IsShader(GLuint shader);
GLboolean IsTexture(GLuint texture);
void LineWidth(GLfloat width);
void LinkProgram(GLuint program);
void PixelStorei(GLenum pname, GLint param);
void PolygonOffset(GLfloat factor, GLfloat units);
void ReadnPixelsEXT(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, GLsizei bufSize, GLvoid *data);
void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
void ReleaseShaderCompiler(void);
void RenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
void RenderbufferStorageMultisampleANGLE(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void SampleCoverage(GLclampf value, GLboolean invert);
void SetFenceNV(GLuint fence, GLenum condition);
void Scissor(GLint x, GLint y, GLsizei width, GLsizei height);
void ShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
void ShaderSource(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length);
void StencilFunc(GLenum func, GLint ref, GLuint mask);
void StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
void StencilMask(GLuint mask);
void StencilMaskSeparate(GLenum face, GLuint mask);
void StencilOp(GLenum fail, GLenum zfail, GLenum zpass);
void StencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
GLboolean TestFenceNV(GLuint fence);
void TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type, const GLvoid* pixels);
void TexParameterf(GLenum target, GLenum pname, GLfloat param);
void TexParameterfv(GLenum target, GLenum pname, const GLfloat* params);
void TexParameteri(GLenum target, GLenum pname, GLint param);
void TexParameteriv(GLenum target, GLenum pname, const GLint* params);
void TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* pixels);
void Uniform1f(GLint location, GLfloat x);
void Uniform1fv(GLint location, GLsizei count, const GLfloat* v);
void Uniform1i(GLint location, GLint x);
void Uniform1iv(GLint location, GLsizei count, const GLint* v);
void Uniform2f(GLint location, GLfloat x, GLfloat y);
void Uniform2fv(GLint location, GLsizei count, const GLfloat* v);
void Uniform2i(GLint location, GLint x, GLint y);
void Uniform2iv(GLint location, GLsizei count, const GLint* v);
void Uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z);
void Uniform3fv(GLint location, GLsizei count, const GLfloat* v);
void Uniform3i(GLint location, GLint x, GLint y, GLint z);
void Uniform3iv(GLint location, GLsizei count, const GLint* v);
void Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void Uniform4fv(GLint location, GLsizei count, const GLfloat* v);
void Uniform4i(GLint location, GLint x, GLint y, GLint z, GLint w);
void Uniform4iv(GLint location, GLsizei count, const GLint* v);
void UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void UseProgram(GLuint program);
void ValidateProgram(GLuint program);
void VertexAttrib1f(GLuint index, GLfloat x);
void VertexAttrib1fv(GLuint index, const GLfloat* values);
void VertexAttrib2f(GLuint index, GLfloat x, GLfloat y);
void VertexAttrib2fv(GLuint index, const GLfloat* values);
void VertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z);
void VertexAttrib3fv(GLuint index, const GLfloat* values);
void VertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void VertexAttrib4fv(GLuint index, const GLfloat* values);
GL_APICALL void VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
GL_APICALL void Viewport(GLint x, GLint y, GLsizei width, GLsizei height);
GL_APICALL void BlitFramebufferNV(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
GL_APICALL void BlitFramebufferANGLE(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                     GLbitfield mask, GLenum filter);
GL_APICALL void TexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth,
                              GLint border, GLenum format, GLenum type, const GLvoid* pixels);
GL_APICALL void TexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
GL_APICALL void CopyTexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GL_APICALL void CompressedTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
GL_APICALL void CompressedTexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
GL_APICALL void FramebufferTexture3DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
GL_APICALL void EGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image);
GL_APICALL void EGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image);
GL_APICALL GLboolean IsRenderbufferOES(GLuint renderbuffer);
GL_APICALL void BindRenderbufferOES(GLenum target, GLuint renderbuffer);
GL_APICALL void DeleteRenderbuffersOES(GLsizei n, const GLuint* renderbuffers);
GL_APICALL void GenRenderbuffersOES(GLsizei n, GLuint* renderbuffers);
GL_APICALL void RenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GL_APICALL void GetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint* params);
GL_APICALL GLboolean IsFramebufferOES(GLuint framebuffer);
GL_APICALL void BindFramebufferOES(GLenum target, GLuint framebuffer);
GL_APICALL void DeleteFramebuffersOES(GLsizei n, const GLuint* framebuffers);
GL_APICALL void GenFramebuffersOES(GLsizei n, GLuint* framebuffers);
GL_APICALL GLenum CheckFramebufferStatusOES(GLenum target);
GL_APICALL void FramebufferRenderbufferOES(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GL_APICALL void FramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GL_APICALL void GetFramebufferAttachmentParameterivOES(GLenum target, GLenum attachment, GLenum pname, GLint* params);
GL_APICALL void GenerateMipmapOES(GLenum target);
GL_APICALL void DrawBuffersEXT(GLsizei n, const GLenum *bufs);
}

extern "C"
{
GL_APICALL void GL_APIENTRY glActiveTexture(GLenum texture)
{
	return es2::ActiveTexture(texture);
}

GL_APICALL void GL_APIENTRY glAttachShader(GLuint program, GLuint shader)
{
	return es2::AttachShader(program, shader);
}

GL_APICALL void GL_APIENTRY glBeginQueryEXT(GLenum target, GLuint name)
{
	return es2::BeginQueryEXT(target, name);
}

GL_APICALL void GL_APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar* name)
{
	return es2::BindAttribLocation(program, index, name);
}

GL_APICALL void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
	return es2::BindBuffer(target, buffer);
}

GL_APICALL void GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	return es2::BindFramebuffer(target, framebuffer);
}

GL_APICALL void GL_APIENTRY glBindFramebufferOES(GLenum target, GLuint framebuffer)
{
	return es2::BindFramebuffer(target, framebuffer);
}

GL_APICALL void GL_APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	return es2::BindRenderbuffer(target, renderbuffer);
}

GL_APICALL void GL_APIENTRY glBindRenderbufferOES(GLenum target, GLuint renderbuffer)
{
	return es2::BindRenderbuffer(target, renderbuffer);
}

GL_APICALL void GL_APIENTRY glBindTexture(GLenum target, GLuint texture)
{
	return es2::BindTexture(target, texture);
}

GL_APICALL void GL_APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	return es2::BlendColor(red, green, blue, alpha);
}

GL_APICALL void GL_APIENTRY glBlendEquation(GLenum mode)
{
	return es2::BlendEquation(mode);
}

GL_APICALL void GL_APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	return es2::BlendEquationSeparate(modeRGB, modeAlpha);
}

GL_APICALL void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	return es2::BlendFunc(sfactor, dfactor);
}

GL_APICALL void GL_APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	return es2::BlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GL_APICALL void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	return es2::BufferData(target, size, data, usage);
}

GL_APICALL void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	return es2::BufferSubData(target, offset, size, data);
}

GL_APICALL GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target)
{
	return es2::CheckFramebufferStatus(target);
}

GL_APICALL GLenum GL_APIENTRY glCheckFramebufferStatusOES(GLenum target)
{
	return es2::CheckFramebufferStatus(target);
}

GL_APICALL void GL_APIENTRY glClear(GLbitfield mask)
{
	return es2::Clear(mask);
}

GL_APICALL void GL_APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	return es2::ClearColor(red, green, blue, alpha);
}

GL_APICALL void GL_APIENTRY glClearDepthf(GLclampf depth)
{
	return es2::ClearDepthf(depth);
}

GL_APICALL void GL_APIENTRY glClearStencil(GLint s)
{
	return es2::ClearStencil(s);
}

GL_APICALL void GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	return es2::ColorMask(red, green, blue, alpha);
}

GL_APICALL void GL_APIENTRY glCompileShader(GLuint shader)
{
	return es2::CompileShader(shader);
}

GL_APICALL void GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                                   GLint border, GLsizei imageSize, const GLvoid* data)
{
	return es2::CompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

GL_APICALL void GL_APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                                      GLenum format, GLsizei imageSize, const GLvoid* data)
{
	return es2::CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

GL_APICALL void GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	return es2::CopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

GL_APICALL void GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	return es2::CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

GL_APICALL GLuint GL_APIENTRY glCreateProgram(void)
{
	return es2::CreateProgram();
}

GL_APICALL GLuint GL_APIENTRY glCreateShader(GLenum type)
{
	return es2::CreateShader(type);
}

GL_APICALL void GL_APIENTRY glCullFace(GLenum mode)
{
	return es2::CullFace(mode);
}

GL_APICALL void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	return es2::DeleteBuffers(n, buffers);
}

GL_APICALL void GL_APIENTRY glDeleteFencesNV(GLsizei n, const GLuint* fences)
{
	return es2::DeleteFencesNV(n, fences);
}

GL_APICALL void GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
	return es2::DeleteFramebuffers(n, framebuffers);
}

GL_APICALL void GL_APIENTRY glDeleteFramebuffersOES(GLsizei n, const GLuint* framebuffers)
{
	return es2::DeleteFramebuffers(n, framebuffers);
}

GL_APICALL void GL_APIENTRY glDeleteProgram(GLuint program)
{
	return es2::DeleteProgram(program);
}

GL_APICALL void GL_APIENTRY glDeleteQueriesEXT(GLsizei n, const GLuint *ids)
{
	return es2::DeleteQueriesEXT(n, ids);
}

GL_APICALL void GL_APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
	return es2::DeleteRenderbuffers(n, renderbuffers);
}

GL_APICALL void GL_APIENTRY glDeleteRenderbuffersOES(GLsizei n, const GLuint* renderbuffers)
{
	return es2::DeleteRenderbuffers(n, renderbuffers);
}

GL_APICALL void GL_APIENTRY glDeleteShader(GLuint shader)
{
	return es2::DeleteShader(shader);
}

GL_APICALL void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures)
{
	return es2::DeleteTextures(n, textures);
}

GL_APICALL void GL_APIENTRY glDepthFunc(GLenum func)
{
	return es2::DepthFunc(func);
}

GL_APICALL void GL_APIENTRY glDepthMask(GLboolean flag)
{
	return es2::DepthMask(flag);
}

GL_APICALL void GL_APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar)
{
	return es2::DepthRangef(zNear, zFar);
}

GL_APICALL void GL_APIENTRY glDetachShader(GLuint program, GLuint shader)
{
	return es2::DetachShader(program, shader);
}

GL_APICALL void GL_APIENTRY glDisable(GLenum cap)
{
	return es2::Disable(cap);
}

GL_APICALL void GL_APIENTRY glDisableVertexAttribArray(GLuint index)
{
	return es2::DisableVertexAttribArray(index);
}

GL_APICALL void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	return es2::DrawArrays(mode, first, count);
}

GL_APICALL void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	return es2::DrawElements(mode, count, type, indices);
}

GL_APICALL void GL_APIENTRY glDrawArraysInstancedEXT(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount)
{
	return es2::DrawArraysInstancedEXT(mode, first, count, instanceCount);
}

GL_APICALL void GL_APIENTRY glDrawElementsInstancedEXT(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount)
{
	return es2::DrawElementsInstancedEXT(mode, count, type, indices, instanceCount);
}

GL_APICALL void GL_APIENTRY glVertexAttribDivisorEXT(GLuint index, GLuint divisor)
{
	return es2::VertexAttribDivisorEXT(index, divisor);
}

GL_APICALL void GL_APIENTRY glDrawArraysInstancedANGLE(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount)
{
	return es2::DrawArraysInstancedANGLE(mode, first, count, instanceCount);
}

GL_APICALL void GL_APIENTRY glDrawElementsInstancedANGLE(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount)
{
	return es2::DrawElementsInstancedANGLE(mode, count, type, indices, instanceCount);
}

GL_APICALL void GL_APIENTRY glVertexAttribDivisorANGLE(GLuint index, GLuint divisor)
{
	return es2::VertexAttribDivisorANGLE(index, divisor);
}

GL_APICALL void GL_APIENTRY glEnable(GLenum cap)
{
	return es2::Enable(cap);
}

GL_APICALL void GL_APIENTRY glEnableVertexAttribArray(GLuint index)
{
	return es2::EnableVertexAttribArray(index);
}

GL_APICALL void GL_APIENTRY glEndQueryEXT(GLenum target)
{
	return es2::EndQueryEXT(target);
}

GL_APICALL void GL_APIENTRY glFinishFenceNV(GLuint fence)
{
	return es2::FinishFenceNV(fence);
}

GL_APICALL void GL_APIENTRY glFinish(void)
{
	return es2::Finish();
}

GL_APICALL void GL_APIENTRY glFlush(void)
{
	return es2::Flush();
}

GL_APICALL void GL_APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	return es2::FramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

GL_APICALL void GL_APIENTRY glFramebufferRenderbufferOES(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	return es2::FramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

GL_APICALL void GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	return es2::FramebufferTexture2D(target, attachment, textarget, texture, level);
}

GL_APICALL void GL_APIENTRY glFramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	return es2::FramebufferTexture2D(target, attachment, textarget, texture, level);
}

GL_APICALL void GL_APIENTRY glFrontFace(GLenum mode)
{
	return es2::FrontFace(mode);
}

GL_APICALL void GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{
	return es2::GenBuffers(n, buffers);
}

GL_APICALL void GL_APIENTRY glGenerateMipmap(GLenum target)
{
	return es2::GenerateMipmap(target);
}

GL_APICALL void GL_APIENTRY glGenerateMipmapOES(GLenum target)
{
	return es2::GenerateMipmap(target);
}

GL_APICALL void GL_APIENTRY glGenFencesNV(GLsizei n, GLuint* fences)
{
	return es2::GenFencesNV(n, fences);
}

GL_APICALL void GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
	return es2::GenFramebuffers(n, framebuffers);
}

GL_APICALL void GL_APIENTRY glGenFramebuffersOES(GLsizei n, GLuint* framebuffers)
{
	return es2::GenFramebuffers(n, framebuffers);
}

GL_APICALL void GL_APIENTRY glGenQueriesEXT(GLsizei n, GLuint* ids)
{
	return es2::GenQueriesEXT(n, ids);
}

GL_APICALL void GL_APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
	return es2::GenRenderbuffers(n, renderbuffers);
}

GL_APICALL void GL_APIENTRY glGenRenderbuffersOES(GLsizei n, GLuint* renderbuffers)
{
	return es2::GenRenderbuffers(n, renderbuffers);
}

GL_APICALL void GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
	return es2::GenTextures(n, textures);
}

GL_APICALL void GL_APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
	return es2::GetActiveAttrib(program, index, bufsize, length, size, type, name);
}

GL_APICALL void GL_APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
	return es2::GetActiveUniform(program, index, bufsize, length, size, type, name);
}

GL_APICALL void GL_APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
	return es2::GetAttachedShaders(program, maxcount, count, shaders);
}

GL_APICALL int GL_APIENTRY glGetAttribLocation(GLuint program, const GLchar* name)
{
	return es2::GetAttribLocation(program, name);
}

GL_APICALL void GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean* params)
{
	return es2::GetBooleanv(pname, params);
}

GL_APICALL void GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
	return es2::GetBufferParameteriv(target, pname, params);
}

GL_APICALL GLenum GL_APIENTRY glGetError(void)
{
	return es2::GetError();
}

GL_APICALL void GL_APIENTRY glGetFenceivNV(GLuint fence, GLenum pname, GLint *params)
{
	return es2::GetFenceivNV(fence, pname, params);
}

GL_APICALL void GL_APIENTRY glGetFloatv(GLenum pname, GLfloat* params)
{
	return es2::GetFloatv(pname, params);
}

GL_APICALL void GL_APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	return es2::GetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

GL_APICALL void GL_APIENTRY glGetFramebufferAttachmentParameterivOES(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	return es2::GetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

GL_APICALL GLenum GL_APIENTRY glGetGraphicsResetStatusEXT(void)
{
	return es2::GetGraphicsResetStatusEXT();
}

GL_APICALL void GL_APIENTRY glGetIntegerv(GLenum pname, GLint* params)
{
	return es2::GetIntegerv(pname, params);
}

GL_APICALL void GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
	return es2::GetProgramiv(program, pname, params);
}

GL_APICALL void GL_APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	return es2::GetProgramInfoLog(program, bufsize, length, infolog);
}

GL_APICALL void GL_APIENTRY glGetQueryivEXT(GLenum target, GLenum pname, GLint *params)
{
	return es2::GetQueryivEXT(target, pname, params);
}

GL_APICALL void GL_APIENTRY glGetQueryObjectuivEXT(GLuint name, GLenum pname, GLuint *params)
{
	return es2::GetQueryObjectuivEXT(name, pname, params);
}

GL_APICALL void GL_APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
	return es2::GetRenderbufferParameteriv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glGetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint* params)
{
	return es2::GetRenderbufferParameteriv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
	return es2::GetShaderiv(shader, pname, params);
}

GL_APICALL void GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
	return es2::GetShaderInfoLog(shader, bufsize, length, infolog);
}

GL_APICALL void GL_APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
	return es2::GetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}

GL_APICALL void GL_APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
	return es2::GetShaderSource(shader, bufsize, length, source);
}

GL_APICALL const GLubyte* GL_APIENTRY glGetString(GLenum name)
{
	return es2::GetString(name);
}

GL_APICALL void GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
	return es2::GetTexParameterfv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
	return es2::GetTexParameteriv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glGetnUniformfvEXT(GLuint program, GLint location, GLsizei bufSize, GLfloat* params)
{
	return es2::GetnUniformfvEXT(program, location, bufSize, params);
}

GL_APICALL void GL_APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
	return es2::GetUniformfv(program, location, params);
}

GL_APICALL void GL_APIENTRY glGetnUniformivEXT(GLuint program, GLint location, GLsizei bufSize, GLint* params)
{
	return es2::GetnUniformivEXT(program, location, bufSize, params);
}

GL_APICALL void GL_APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params)
{
	return es2::GetUniformiv(program, location, params);
}

GL_APICALL int GL_APIENTRY glGetUniformLocation(GLuint program, const GLchar* name)
{
	return es2::GetUniformLocation(program, name);
}

GL_APICALL void GL_APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
	return es2::GetVertexAttribfv(index, pname, params);
}

GL_APICALL void GL_APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
	return es2::GetVertexAttribiv(index, pname, params);
}

GL_APICALL void GL_APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer)
{
	return es2::GetVertexAttribPointerv(index, pname, pointer);
}

GL_APICALL void GL_APIENTRY glHint(GLenum target, GLenum mode)
{
	return es2::Hint(target, mode);
}

GL_APICALL GLboolean GL_APIENTRY glIsBuffer(GLuint buffer)
{
	return es2::IsBuffer(buffer);
}

GL_APICALL GLboolean GL_APIENTRY glIsEnabled(GLenum cap)
{
	return es2::IsEnabled(cap);
}

GL_APICALL GLboolean GL_APIENTRY glIsFenceNV(GLuint fence)
{
	return es2::IsFenceNV(fence);
}

GL_APICALL GLboolean GL_APIENTRY glIsFramebuffer(GLuint framebuffer)
{
	return es2::IsFramebuffer(framebuffer);
}

GL_APICALL GLboolean GL_APIENTRY glIsFramebufferOES(GLuint framebuffer)
{
	return es2::IsFramebuffer(framebuffer);
}

GL_APICALL GLboolean GL_APIENTRY glIsProgram(GLuint program)
{
	return es2::IsProgram(program);
}

GL_APICALL GLboolean GL_APIENTRY glIsQueryEXT(GLuint name)
{
	return es2::IsQueryEXT(name);
}

GL_APICALL GLboolean GL_APIENTRY glIsRenderbuffer(GLuint renderbuffer)
{
	return es2::IsRenderbuffer(renderbuffer);
}

GL_APICALL GLboolean GL_APIENTRY glIsRenderbufferOES(GLuint renderbuffer)
{
	return es2::IsRenderbuffer(renderbuffer);
}

GL_APICALL GLboolean GL_APIENTRY glIsShader(GLuint shader)
{
	return es2::IsShader(shader);
}

GL_APICALL GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
	return es2::IsTexture(texture);
}

GL_APICALL void GL_APIENTRY glLineWidth(GLfloat width)
{
	return es2::LineWidth(width);
}

GL_APICALL void GL_APIENTRY glLinkProgram(GLuint program)
{
	return es2::LinkProgram(program);
}

GL_APICALL void GL_APIENTRY glPixelStorei(GLenum pname, GLint param)
{
	return es2::PixelStorei(pname, param);
}

GL_APICALL void GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
	return es2::PolygonOffset(factor, units);
}

GL_APICALL void GL_APIENTRY glReadnPixelsEXT(GLint x, GLint y, GLsizei width, GLsizei height,
                                             GLenum format, GLenum type, GLsizei bufSize, GLvoid *data)
{
	return es2::ReadnPixelsEXT(x, y, width, height, format, type, bufSize, data);
}

GL_APICALL void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
	return es2::ReadPixels(x, y, width, height, format, type, pixels);
}

GL_APICALL void GL_APIENTRY glReleaseShaderCompiler(void)
{
	return es2::ReleaseShaderCompiler();
}

GL_APICALL void GL_APIENTRY glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
	return es2::RenderbufferStorageMultisample(target, samples, internalformat, width, height);
}

GL_APICALL void GL_APIENTRY glRenderbufferStorageMultisampleANGLE(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
	return es2::RenderbufferStorageMultisampleANGLE(target, samples, internalformat, width, height);
}

GL_APICALL void GL_APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	return es2::RenderbufferStorage(target, internalformat, width, height);
}

GL_APICALL void GL_APIENTRY glRenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	return es2::RenderbufferStorage(target, internalformat, width, height);
}

GL_APICALL void GL_APIENTRY glSampleCoverage(GLclampf value, GLboolean invert)
{
	return es2::SampleCoverage(value, invert);
}

GL_APICALL void GL_APIENTRY glSetFenceNV(GLuint fence, GLenum condition)
{
	return es2::SetFenceNV(fence, condition);
}

GL_APICALL void GL_APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	return es2::Scissor(x, y, width, height);
}

GL_APICALL void GL_APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length)
{
	return es2::ShaderBinary(n, shaders, binaryformat, binary, length);
}

GL_APICALL void GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length)
{
	return es2::ShaderSource(shader, count, string, length);
}

GL_APICALL void GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	return es2::StencilFunc(func, ref, mask);
}

GL_APICALL void GL_APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	return es2::StencilFuncSeparate(face, func, ref, mask);
}

GL_APICALL void GL_APIENTRY glStencilMask(GLuint mask)
{
	return es2::StencilMask(mask);
}

GL_APICALL void GL_APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask)
{
	return es2::StencilMaskSeparate(face, mask);
}

GL_APICALL void GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	return es2::StencilOp(fail, zfail, zpass);
}

GL_APICALL void GL_APIENTRY glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
	return es2::StencilOpSeparate(face, fail, zfail, zpass);
}

GLboolean GL_APIENTRY glTestFenceNV(GLuint fence)
{
	return es2::TestFenceNV(fence);
}

GL_APICALL void GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                                         GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	return es2::TexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

GL_APICALL void GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	return es2::TexParameterf(target, pname, param);
}

GL_APICALL void GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
	return es2::TexParameterfv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	return es2::TexParameteri(target, pname, param);
}

GL_APICALL void GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
	return es2::TexParameteriv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                            GLenum format, GLenum type, const GLvoid* pixels)
{
	return es2::TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

GL_APICALL void GL_APIENTRY glUniform1f(GLint location, GLfloat x)
{
	return es2::Uniform1f(location, x);
}

GL_APICALL void GL_APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
	return es2::Uniform1fv(location, count, v);
}

GL_APICALL void GL_APIENTRY glUniform1i(GLint location, GLint x)
{
	return es2::Uniform1i(location, x);
}

GL_APICALL void GL_APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
	return es2::Uniform1iv(location, count, v);
}

GL_APICALL void GL_APIENTRY glUniform2f(GLint location, GLfloat x, GLfloat y)
{
	return es2::Uniform2f(location, x, y);
}

GL_APICALL void GL_APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
	return es2::Uniform2fv(location, count, v);
}

GL_APICALL void GL_APIENTRY glUniform2i(GLint location, GLint x, GLint y)
{
	return es2::Uniform2i(location, x, y);
}

GL_APICALL void GL_APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
	return es2::Uniform2iv(location, count, v);
}

GL_APICALL void GL_APIENTRY glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
	return es2::Uniform3f(location, x, y, z);
}

GL_APICALL void GL_APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
	return es2::Uniform3fv(location, count, v);
}

GL_APICALL void GL_APIENTRY glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
	return es2::Uniform3i(location, x, y, z);
}

GL_APICALL void GL_APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
	return es2::Uniform3iv(location, count, v);
}

GL_APICALL void GL_APIENTRY glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	return es2::Uniform4f(location, x, y, z, w);
}

GL_APICALL void GL_APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
	return es2::Uniform4fv(location, count, v);
}

GL_APICALL void GL_APIENTRY glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
	return es2::Uniform4i(location, x, y, z, w);
}

GL_APICALL void GL_APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
	return es2::Uniform4iv(location, count, v);
}

GL_APICALL void GL_APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return es2::UniformMatrix2fv(location, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return es2::UniformMatrix3fv(location, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	return es2::UniformMatrix4fv(location, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glUseProgram(GLuint program)
{
	return es2::UseProgram(program);
}

GL_APICALL void GL_APIENTRY glValidateProgram(GLuint program)
{
	return es2::ValidateProgram(program);
}

GL_APICALL void GL_APIENTRY glVertexAttrib1f(GLuint index, GLfloat x)
{
	return es2::VertexAttrib1f(index, x);
}

GL_APICALL void GL_APIENTRY glVertexAttrib1fv(GLuint index, const GLfloat* values)
{
	return es2::VertexAttrib1fv(index, values);
}

GL_APICALL void GL_APIENTRY glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
	return es2::VertexAttrib2f(index, x, y);
}

GL_APICALL void GL_APIENTRY glVertexAttrib2fv(GLuint index, const GLfloat* values)
{
	return es2::VertexAttrib2fv(index, values);
}

GL_APICALL void GL_APIENTRY glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
	return es2::VertexAttrib3f(index, x, y, z);
}

GL_APICALL void GL_APIENTRY glVertexAttrib3fv(GLuint index, const GLfloat* values)
{
	return es2::VertexAttrib3fv(index, values);
}

GL_APICALL void GL_APIENTRY glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	return es2::VertexAttrib4f(index, x, y, z, w);
}

GL_APICALL void GL_APIENTRY glVertexAttrib4fv(GLuint index, const GLfloat* values)
{
	return es2::VertexAttrib4fv(index, values);
}

GL_APICALL void GL_APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
	return es2::VertexAttribPointer(index, size, type, normalized, stride, ptr);
}

GL_APICALL void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	return es2::Viewport(x, y, width, height);
}

GL_APICALL void GL_APIENTRY glBlitFramebufferNV(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
	return es2::BlitFramebufferNV(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

GL_APICALL void GL_APIENTRY glBlitFramebufferANGLE(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                                   GLbitfield mask, GLenum filter)
{
	return es2::BlitFramebufferANGLE(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

GL_APICALL void GL_APIENTRY glTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth,
                                            GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	return es2::TexImage3DOES(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

GL_APICALL void GL_APIENTRY glTexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
	return es2::TexSubImage3DOES(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

GL_APICALL void GL_APIENTRY glCopyTexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	return es2::CopyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

GL_APICALL void GL_APIENTRY glCompressedTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data)
{
	return es2::CompressedTexImage3DOES(target, level,internalformat, width, height, depth, border, imageSize, data);
}

GL_APICALL void GL_APIENTRY glCompressedTexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)
{
	return es2::CompressedTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}

GL_APICALL void GL_APIENTRY glFramebufferTexture3DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
	return es2::FramebufferTexture3DOES(target, attachment, textarget, texture, level, zoffset);
}

GL_APICALL void GL_APIENTRY glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
	return es2::EGLImageTargetTexture2DOES(target, image);
}

GL_APICALL void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image)
{
	return es2::EGLImageTargetRenderbufferStorageOES(target, image);
}

GL_APICALL void GL_APIENTRY glDrawBuffersEXT(GLsizei n, const GLenum *bufs)
{
	return es2::DrawBuffersEXT(n, bufs);
}

void GL_APIENTRY Register(const char *licenseKey)
{
	// Nothing to do, SwiftShader is open-source
}
}

egl::Context *es2CreateContext(egl::Display *display, const egl::Context *shareContext, int clientVersion, const egl::Config *config);
extern "C" __eglMustCastToProperFunctionPointerType es2GetProcAddress(const char *procname);
egl::Image *createBackBuffer(int width, int height, sw::Format format, int multiSampleDepth);
egl::Image *createBackBufferFromClientBuffer(const egl::ClientBuffer& clientBuffer);
egl::Image *createDepthStencil(int width, int height, sw::Format format, int multiSampleDepth);
sw::FrameBuffer *createFrameBuffer(void *nativeDisplay, EGLNativeWindowType window, int width, int height);

LibGLESv2exports::LibGLESv2exports()
{
	this->glActiveTexture = es2::ActiveTexture;
	this->glAttachShader = es2::AttachShader;
	this->glBeginQueryEXT = es2::BeginQueryEXT;
	this->glBindAttribLocation = es2::BindAttribLocation;
	this->glBindBuffer = es2::BindBuffer;
	this->glBindFramebuffer = es2::BindFramebuffer;
	this->glBindRenderbuffer = es2::BindRenderbuffer;
	this->glBindTexture = es2::BindTexture;
	this->glBlendColor = es2::BlendColor;
	this->glBlendEquation = es2::BlendEquation;
	this->glBlendEquationSeparate = es2::BlendEquationSeparate;
	this->glBlendFunc = es2::BlendFunc;
	this->glBlendFuncSeparate = es2::BlendFuncSeparate;
	this->glBufferData = es2::BufferData;
	this->glBufferSubData = es2::BufferSubData;
	this->glCheckFramebufferStatus = es2::CheckFramebufferStatus;
	this->glClear = es2::Clear;
	this->glClearColor = es2::ClearColor;
	this->glClearDepthf = es2::ClearDepthf;
	this->glClearStencil = es2::ClearStencil;
	this->glColorMask = es2::ColorMask;
	this->glCompileShader = es2::CompileShader;
	this->glCompressedTexImage2D = es2::CompressedTexImage2D;
	this->glCompressedTexSubImage2D = es2::CompressedTexSubImage2D;
	this->glCopyTexImage2D = es2::CopyTexImage2D;
	this->glCopyTexSubImage2D = es2::CopyTexSubImage2D;
	this->glCreateProgram = es2::CreateProgram;
	this->glCreateShader = es2::CreateShader;
	this->glCullFace = es2::CullFace;
	this->glDeleteBuffers = es2::DeleteBuffers;
	this->glDeleteFencesNV = es2::DeleteFencesNV;
	this->glDeleteFramebuffers = es2::DeleteFramebuffers;
	this->glDeleteProgram = es2::DeleteProgram;
	this->glDeleteQueriesEXT = es2::DeleteQueriesEXT;
	this->glDeleteRenderbuffers = es2::DeleteRenderbuffers;
	this->glDeleteShader = es2::DeleteShader;
	this->glDeleteTextures = es2::DeleteTextures;
	this->glDepthFunc = es2::DepthFunc;
	this->glDepthMask = es2::DepthMask;
	this->glDepthRangef = es2::DepthRangef;
	this->glDetachShader = es2::DetachShader;
	this->glDisable = es2::Disable;
	this->glDisableVertexAttribArray = es2::DisableVertexAttribArray;
	this->glDrawArrays = es2::DrawArrays;
	this->glDrawElements = es2::DrawElements;
	this->glDrawArraysInstancedEXT = es2::DrawArraysInstancedEXT;
	this->glDrawElementsInstancedEXT = es2::DrawElementsInstancedEXT;
	this->glVertexAttribDivisorEXT = es2::VertexAttribDivisorEXT;
	this->glDrawArraysInstancedANGLE = es2::DrawArraysInstancedANGLE;
	this->glDrawElementsInstancedANGLE = es2::DrawElementsInstancedANGLE;
	this->glVertexAttribDivisorANGLE = es2::VertexAttribDivisorANGLE;
	this->glEnable = es2::Enable;
	this->glEnableVertexAttribArray = es2::EnableVertexAttribArray;
	this->glEndQueryEXT = es2::EndQueryEXT;
	this->glFinishFenceNV = es2::FinishFenceNV;
	this->glFinish = es2::Finish;
	this->glFlush = es2::Flush;
	this->glFramebufferRenderbuffer = es2::FramebufferRenderbuffer;
	this->glFramebufferTexture2D = es2::FramebufferTexture2D;
	this->glFrontFace = es2::FrontFace;
	this->glGenBuffers = es2::GenBuffers;
	this->glGenerateMipmap = es2::GenerateMipmap;
	this->glGenFencesNV = es2::GenFencesNV;
	this->glGenFramebuffers = es2::GenFramebuffers;
	this->glGenQueriesEXT = es2::GenQueriesEXT;
	this->glGenRenderbuffers = es2::GenRenderbuffers;
	this->glGenTextures = es2::GenTextures;
	this->glGetActiveAttrib = es2::GetActiveAttrib;
	this->glGetActiveUniform = es2::GetActiveUniform;
	this->glGetAttachedShaders = es2::GetAttachedShaders;
	this->glGetAttribLocation = es2::GetAttribLocation;
	this->glGetBooleanv = es2::GetBooleanv;
	this->glGetBufferParameteriv = es2::GetBufferParameteriv;
	this->glGetError = es2::GetError;
	this->glGetFenceivNV = es2::GetFenceivNV;
	this->glGetFloatv = es2::GetFloatv;
	this->glGetFramebufferAttachmentParameteriv = es2::GetFramebufferAttachmentParameteriv;
	this->glGetGraphicsResetStatusEXT = es2::GetGraphicsResetStatusEXT;
	this->glGetIntegerv = es2::GetIntegerv;
	this->glGetProgramiv = es2::GetProgramiv;
	this->glGetProgramInfoLog = es2::GetProgramInfoLog;
	this->glGetQueryivEXT = es2::GetQueryivEXT;
	this->glGetQueryObjectuivEXT = es2::GetQueryObjectuivEXT;
	this->glGetRenderbufferParameteriv = es2::GetRenderbufferParameteriv;
	this->glGetShaderiv = es2::GetShaderiv;
	this->glGetShaderInfoLog = es2::GetShaderInfoLog;
	this->glGetShaderPrecisionFormat = es2::GetShaderPrecisionFormat;
	this->glGetShaderSource = es2::GetShaderSource;
	this->glGetString = es2::GetString;
	this->glGetTexParameterfv = es2::GetTexParameterfv;
	this->glGetTexParameteriv = es2::GetTexParameteriv;
	this->glGetnUniformfvEXT = es2::GetnUniformfvEXT;
	this->glGetUniformfv = es2::GetUniformfv;
	this->glGetnUniformivEXT = es2::GetnUniformivEXT;
	this->glGetUniformiv = es2::GetUniformiv;
	this->glGetUniformLocation = es2::GetUniformLocation;
	this->glGetVertexAttribfv = es2::GetVertexAttribfv;
	this->glGetVertexAttribiv = es2::GetVertexAttribiv;
	this->glGetVertexAttribPointerv = es2::GetVertexAttribPointerv;
	this->glHint = es2::Hint;
	this->glIsBuffer = es2::IsBuffer;
	this->glIsEnabled = es2::IsEnabled;
	this->glIsFenceNV = es2::IsFenceNV;
	this->glIsFramebuffer = es2::IsFramebuffer;
	this->glIsProgram = es2::IsProgram;
	this->glIsQueryEXT = es2::IsQueryEXT;
	this->glIsRenderbuffer = es2::IsRenderbuffer;
	this->glIsShader = es2::IsShader;
	this->glIsTexture = es2::IsTexture;
	this->glLineWidth = es2::LineWidth;
	this->glLinkProgram = es2::LinkProgram;
	this->glPixelStorei = es2::PixelStorei;
	this->glPolygonOffset = es2::PolygonOffset;
	this->glReadnPixelsEXT = es2::ReadnPixelsEXT;
	this->glReadPixels = es2::ReadPixels;
	this->glReleaseShaderCompiler = es2::ReleaseShaderCompiler;
	this->glRenderbufferStorageMultisample = es2::RenderbufferStorageMultisample;
	this->glRenderbufferStorageMultisampleANGLE = es2::RenderbufferStorageMultisampleANGLE;
	this->glRenderbufferStorage = es2::RenderbufferStorage;
	this->glSampleCoverage = es2::SampleCoverage;
	this->glSetFenceNV = es2::SetFenceNV;
	this->glScissor = es2::Scissor;
	this->glShaderBinary = es2::ShaderBinary;
	this->glShaderSource = es2::ShaderSource;
	this->glStencilFunc = es2::StencilFunc;
	this->glStencilFuncSeparate = es2::StencilFuncSeparate;
	this->glStencilMask = es2::StencilMask;
	this->glStencilMaskSeparate = es2::StencilMaskSeparate;
	this->glStencilOp = es2::StencilOp;
	this->glStencilOpSeparate = es2::StencilOpSeparate;
	this->glTestFenceNV = es2::TestFenceNV;
	this->glTexImage2D = es2::TexImage2D;
	this->glTexParameterf = es2::TexParameterf;
	this->glTexParameterfv = es2::TexParameterfv;
	this->glTexParameteri = es2::TexParameteri;
	this->glTexParameteriv = es2::TexParameteriv;
	this->glTexSubImage2D = es2::TexSubImage2D;
	this->glUniform1f = es2::Uniform1f;
	this->glUniform1fv = es2::Uniform1fv;
	this->glUniform1i = es2::Uniform1i;
	this->glUniform1iv = es2::Uniform1iv;
	this->glUniform2f = es2::Uniform2f;
	this->glUniform2fv = es2::Uniform2fv;
	this->glUniform2i = es2::Uniform2i;
	this->glUniform2iv = es2::Uniform2iv;
	this->glUniform3f = es2::Uniform3f;
	this->glUniform3fv = es2::Uniform3fv;
	this->glUniform3i = es2::Uniform3i;
	this->glUniform3iv = es2::Uniform3iv;
	this->glUniform4f = es2::Uniform4f;
	this->glUniform4fv = es2::Uniform4fv;
	this->glUniform4i = es2::Uniform4i;
	this->glUniform4iv = es2::Uniform4iv;
	this->glUniformMatrix2fv = es2::UniformMatrix2fv;
	this->glUniformMatrix3fv = es2::UniformMatrix3fv;
	this->glUniformMatrix4fv = es2::UniformMatrix4fv;
	this->glUseProgram = es2::UseProgram;
	this->glValidateProgram = es2::ValidateProgram;
	this->glVertexAttrib1f = es2::VertexAttrib1f;
	this->glVertexAttrib1fv = es2::VertexAttrib1fv;
	this->glVertexAttrib2f = es2::VertexAttrib2f;
	this->glVertexAttrib2fv = es2::VertexAttrib2fv;
	this->glVertexAttrib3f = es2::VertexAttrib3f;
	this->glVertexAttrib3fv = es2::VertexAttrib3fv;
	this->glVertexAttrib4f = es2::VertexAttrib4f;
	this->glVertexAttrib4fv = es2::VertexAttrib4fv;
	this->glVertexAttribPointer = es2::VertexAttribPointer;
	this->glViewport = es2::Viewport;
	this->glBlitFramebufferNV = es2::BlitFramebufferNV;
	this->glBlitFramebufferANGLE = es2::BlitFramebufferANGLE;
	this->glTexImage3DOES = es2::TexImage3DOES;
	this->glTexSubImage3DOES = es2::TexSubImage3DOES;
	this->glCopyTexSubImage3DOES = es2::CopyTexSubImage3DOES;
	this->glCompressedTexImage3DOES = es2::CompressedTexImage3DOES;
	this->glCompressedTexSubImage3DOES = es2::CompressedTexSubImage3DOES;
	this->glFramebufferTexture3DOES = es2::FramebufferTexture3DOES;
	this->glEGLImageTargetTexture2DOES = es2::EGLImageTargetTexture2DOES;
	this->glEGLImageTargetRenderbufferStorageOES = es2::EGLImageTargetRenderbufferStorageOES;
	this->glIsRenderbufferOES = es2::IsRenderbufferOES;
	this->glBindRenderbufferOES = es2::BindRenderbufferOES;
	this->glDeleteRenderbuffersOES = es2::DeleteRenderbuffersOES;
	this->glGenRenderbuffersOES = es2::GenRenderbuffersOES;
	this->glRenderbufferStorageOES = es2::RenderbufferStorageOES;
	this->glGetRenderbufferParameterivOES = es2::GetRenderbufferParameterivOES;
	this->glIsFramebufferOES = es2::IsFramebufferOES;
	this->glBindFramebufferOES = es2::BindFramebufferOES;
	this->glDeleteFramebuffersOES = es2::DeleteFramebuffersOES;
	this->glGenFramebuffersOES = es2::GenFramebuffersOES;
	this->glCheckFramebufferStatusOES = es2::CheckFramebufferStatusOES;
	this->glFramebufferRenderbufferOES = es2::FramebufferRenderbufferOES;
	this->glFramebufferTexture2DOES = es2::FramebufferTexture2DOES;
	this->glGetFramebufferAttachmentParameterivOES = es2::GetFramebufferAttachmentParameterivOES;
	this->glGenerateMipmapOES = es2::GenerateMipmapOES;
	this->glDrawBuffersEXT = es2::DrawBuffersEXT;

	this->es2CreateContext = ::es2CreateContext;
	this->es2GetProcAddress = ::es2GetProcAddress;
	this->createBackBuffer = ::createBackBuffer;
	this->createBackBufferFromClientBuffer = ::createBackBufferFromClientBuffer;
	this->createDepthStencil = ::createDepthStencil;
	this->createFrameBuffer = ::createFrameBuffer;
}

extern "C" GL_APICALL LibGLESv2exports *libGLESv2_swiftshader()
{
	static LibGLESv2exports libGLESv2;
	return &libGLESv2;
}

LibEGL libEGL(getLibraryDirectoryFromSymbol((void*)libGLESv2_swiftshader));
LibGLES_CM libGLES_CM(getLibraryDirectoryFromSymbol((void*)libGLESv2_swiftshader));
