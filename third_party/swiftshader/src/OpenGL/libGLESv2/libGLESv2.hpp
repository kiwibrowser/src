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

#ifndef libGLESv2_hpp
#define libGLESv2_hpp

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>

#include "Common/SharedLibrary.hpp"

namespace sw
{
class FrameBuffer;
enum Format : unsigned char;
}

namespace egl
{
class Display;
class Context;
class Image;
class Config;
class ClientBuffer;
}

class LibGLESv2exports
{
public:
	LibGLESv2exports();

	void (*glActiveTexture)(GLenum texture);
	void (*glAttachShader)(GLuint program, GLuint shader);
	void (*glBeginQueryEXT)(GLenum target, GLuint name);
	void (*glBindAttribLocation)(GLuint program, GLuint index, const GLchar* name);
	void (*glBindBuffer)(GLenum target, GLuint buffer);
	void (*glBindFramebuffer)(GLenum target, GLuint framebuffer);
	void (*glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
	void (*glBindTexture)(GLenum target, GLuint texture);
	void (*glBlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void (*glBlendEquation)(GLenum mode);
	void (*glBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
	void (*glBlendFunc)(GLenum sfactor, GLenum dfactor);
	void (*glBlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	void (*glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
	void (*glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
	GLenum (*glCheckFramebufferStatus)(GLenum target);
	void (*glClear)(GLbitfield mask);
	void (*glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void (*glClearDepthf)(GLclampf depth);
	void (*glClearStencil)(GLint s);
	void (*glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
	void (*glCompileShader)(GLuint shader);
	void (*glCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
	                               GLint border, GLsizei imageSize, const GLvoid* data);
	void (*glCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	                                  GLenum format, GLsizei imageSize, const GLvoid* data);
	void (*glCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void (*glCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	GLuint (*glCreateProgram)(void);
	GLuint (*glCreateShader)(GLenum type);
	void (*glCullFace)(GLenum mode);
	void (*glDeleteBuffers)(GLsizei n, const GLuint* buffers);
	void (*glDeleteFencesNV)(GLsizei n, const GLuint* fences);
	void (*glDeleteFramebuffers)(GLsizei n, const GLuint* framebuffers);
	void (*glDeleteProgram)(GLuint program);
	void (*glDeleteQueriesEXT)(GLsizei n, const GLuint *ids);
	void (*glDeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers);
	void (*glDeleteShader)(GLuint shader);
	void (*glDeleteTextures)(GLsizei n, const GLuint* textures);
	void (*glDepthFunc)(GLenum func);
	void (*glDepthMask)(GLboolean flag);
	void (*glDepthRangef)(GLclampf zNear, GLclampf zFar);
	void (*glDetachShader)(GLuint program, GLuint shader);
	void (*glDisable)(GLenum cap);
	void (*glDisableVertexAttribArray)(GLuint index);
	void (*glDrawArrays)(GLenum mode, GLint first, GLsizei count);
	void (*glDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
	void (*glDrawArraysInstancedEXT)(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
	void (*glDrawElementsInstancedEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount);
	void (*glVertexAttribDivisorEXT)(GLuint index, GLuint divisor);
	void (*glDrawArraysInstancedANGLE)(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
	void (*glDrawElementsInstancedANGLE)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instanceCount);
	void (*glVertexAttribDivisorANGLE)(GLuint index, GLuint divisor);
	void (*glEnable)(GLenum cap);
	void (*glEnableVertexAttribArray)(GLuint index);
	void (*glEndQueryEXT)(GLenum target);
	void (*glFinishFenceNV)(GLuint fence);
	void (*glFinish)(void);
	void (*glFlush)(void);
	void (*glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	void (*glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void (*glFrontFace)(GLenum mode);
	void (*glGenBuffers)(GLsizei n, GLuint* buffers);
	void (*glGenerateMipmap)(GLenum target);
	void (*glGenFencesNV)(GLsizei n, GLuint* fences);
	void (*glGenFramebuffers)(GLsizei n, GLuint* framebuffers);
	void (*glGenQueriesEXT)(GLsizei n, GLuint* ids);
	void (*glGenRenderbuffers)(GLsizei n, GLuint* renderbuffers);
	void (*glGenTextures)(GLsizei n, GLuint* textures);
	void (*glGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
	void (*glGetActiveUniform)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
	void (*glGetAttachedShaders)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
	int (*glGetAttribLocation)(GLuint program, const GLchar* name);
	void (*glGetBooleanv)(GLenum pname, GLboolean* params);
	void (*glGetBufferParameteriv)(GLenum target, GLenum pname, GLint* params);
	GLenum (*glGetError)(void);
	void (*glGetFenceivNV)(GLuint fence, GLenum pname, GLint *params);
	void (*glGetFloatv)(GLenum pname, GLfloat* params);
	void (*glGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
	GLenum (*glGetGraphicsResetStatusEXT)(void);
	void (*glGetIntegerv)(GLenum pname, GLint* params);
	void (*glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
	void (*glGetProgramInfoLog)(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
	void (*glGetQueryivEXT)(GLenum target, GLenum pname, GLint *params);
	void (*glGetQueryObjectuivEXT)(GLuint name, GLenum pname, GLuint *params);
	void (*glGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint* params);
	void (*glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
	void (*glGetShaderInfoLog)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
	void (*glGetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
	void (*glGetShaderSource)(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
	const GLubyte* (*glGetString)(GLenum name);
	void (*glGetTexParameterfv)(GLenum target, GLenum pname, GLfloat* params);
	void (*glGetTexParameteriv)(GLenum target, GLenum pname, GLint* params);
	void (*glGetnUniformfvEXT)(GLuint program, GLint location, GLsizei bufSize, GLfloat* params);
	void (*glGetUniformfv)(GLuint program, GLint location, GLfloat* params);
	void (*glGetnUniformivEXT)(GLuint program, GLint location, GLsizei bufSize, GLint* params);
	void (*glGetUniformiv)(GLuint program, GLint location, GLint* params);
	int (*glGetUniformLocation)(GLuint program, const GLchar* name);
	void (*glGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat* params);
	void (*glGetVertexAttribiv)(GLuint index, GLenum pname, GLint* params);
	void (*glGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid** pointer);
	void (*glHint)(GLenum target, GLenum mode);
	GLboolean (*glIsBuffer)(GLuint buffer);
	GLboolean (*glIsEnabled)(GLenum cap);
	GLboolean (*glIsFenceNV)(GLuint fence);
	GLboolean (*glIsFramebuffer)(GLuint framebuffer);
	GLboolean (*glIsProgram)(GLuint program);
	GLboolean (*glIsQueryEXT)(GLuint name);
	GLboolean (*glIsRenderbuffer)(GLuint renderbuffer);
	GLboolean (*glIsShader)(GLuint shader);
	GLboolean (*glIsTexture)(GLuint texture);
	void (*glLineWidth)(GLfloat width);
	void (*glLinkProgram)(GLuint program);
	void (*glPixelStorei)(GLenum pname, GLint param);
	void (*glPolygonOffset)(GLfloat factor, GLfloat units);
	void (*glReadnPixelsEXT)(GLint x, GLint y, GLsizei width, GLsizei height,
	                         GLenum format, GLenum type, GLsizei bufSize, GLvoid *data);
	void (*glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
	void (*glReleaseShaderCompiler)(void);
	void (*glRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	void (*glRenderbufferStorageMultisampleANGLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	void (*glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void (*glSampleCoverage)(GLclampf value, GLboolean invert);
	void (*glSetFenceNV)(GLuint fence, GLenum condition);
	void (*glScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
	void (*glShaderBinary)(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
	void (*glShaderSource)(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length);
	void (*glStencilFunc)(GLenum func, GLint ref, GLuint mask);
	void (*glStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
	void (*glStencilMask)(GLuint mask);
	void (*glStencilMaskSeparate)(GLenum face, GLuint mask);
	void (*glStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
	void (*glStencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
	GLboolean (*glTestFenceNV)(GLuint fence);
	void (*glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
	                     GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	void (*glTexParameterf)(GLenum target, GLenum pname, GLfloat param);
	void (*glTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params);
	void (*glTexParameteri)(GLenum target, GLenum pname, GLint param);
	void (*glTexParameteriv)(GLenum target, GLenum pname, const GLint* params);
	void (*glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	                        GLenum format, GLenum type, const GLvoid* pixels);
	void (*glUniform1f)(GLint location, GLfloat x);
	void (*glUniform1fv)(GLint location, GLsizei count, const GLfloat* v);
	void (*glUniform1i)(GLint location, GLint x);
	void (*glUniform1iv)(GLint location, GLsizei count, const GLint* v);
	void (*glUniform2f)(GLint location, GLfloat x, GLfloat y);
	void (*glUniform2fv)(GLint location, GLsizei count, const GLfloat* v);
	void (*glUniform2i)(GLint location, GLint x, GLint y);
	void (*glUniform2iv)(GLint location, GLsizei count, const GLint* v);
	void (*glUniform3f)(GLint location, GLfloat x, GLfloat y, GLfloat z);
	void (*glUniform3fv)(GLint location, GLsizei count, const GLfloat* v);
	void (*glUniform3i)(GLint location, GLint x, GLint y, GLint z);
	void (*glUniform3iv)(GLint location, GLsizei count, const GLint* v);
	void (*glUniform4f)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void (*glUniform4fv)(GLint location, GLsizei count, const GLfloat* v);
	void (*glUniform4i)(GLint location, GLint x, GLint y, GLint z, GLint w);
	void (*glUniform4iv)(GLint location, GLsizei count, const GLint* v);
	void (*glUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	void (*glUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	void (*glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	void (*glUseProgram)(GLuint program);
	void (*glValidateProgram)(GLuint program);
	void (*glVertexAttrib1f)(GLuint index, GLfloat x);
	void (*glVertexAttrib1fv)(GLuint index, const GLfloat* values);
	void (*glVertexAttrib2f)(GLuint index, GLfloat x, GLfloat y);
	void (*glVertexAttrib2fv)(GLuint index, const GLfloat* values);
	void (*glVertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
	void (*glVertexAttrib3fv)(GLuint index, const GLfloat* values);
	void (*glVertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void (*glVertexAttrib4fv)(GLuint index, const GLfloat* values);
	void (*glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
	void (*glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
	void (*glBlitFramebufferNV)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	void (*glBlitFramebufferANGLE)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
	                               GLbitfield mask, GLenum filter);
	void (*glTexImage3DOES)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth,
	                        GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	void (*glTexSubImage3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	void (*glCopyTexSubImage3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void (*glCompressedTexImage3DOES)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
	void (*glCompressedTexSubImage3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
	void (*glFramebufferTexture3DOES)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
	void (*glEGLImageTargetTexture2DOES)(GLenum target, GLeglImageOES image);
	void (*glEGLImageTargetRenderbufferStorageOES)(GLenum target, GLeglImageOES image);
	GLboolean (*glIsRenderbufferOES)(GLuint renderbuffer);
	void (*glBindRenderbufferOES)(GLenum target, GLuint renderbuffer);
	void (*glDeleteRenderbuffersOES)(GLsizei n, const GLuint* renderbuffers);
	void (*glGenRenderbuffersOES)(GLsizei n, GLuint* renderbuffers);
	void (*glRenderbufferStorageOES)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void (*glGetRenderbufferParameterivOES)(GLenum target, GLenum pname, GLint* params);
	GLboolean (*glIsFramebufferOES)(GLuint framebuffer);
	void (*glBindFramebufferOES)(GLenum target, GLuint framebuffer);
	void (*glDeleteFramebuffersOES)(GLsizei n, const GLuint* framebuffers);
	void (*glGenFramebuffersOES)(GLsizei n, GLuint* framebuffers);
	GLenum (*glCheckFramebufferStatusOES)(GLenum target);
	void (*glFramebufferRenderbufferOES)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	void (*glFramebufferTexture2DOES)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void (*glGetFramebufferAttachmentParameterivOES)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
	void (*glGenerateMipmapOES)(GLenum target);
	void (*glDrawBuffersEXT)(GLsizei n, const GLenum *bufs);

	egl::Context *(*es2CreateContext)(egl::Display *display, const egl::Context *shareContext, int clientVersion, const egl::Config *config);
	__eglMustCastToProperFunctionPointerType (*es2GetProcAddress)(const char *procname);
	egl::Image *(*createBackBuffer)(int width, int height, sw::Format format, int multiSampleDepth);
	egl::Image *(*createBackBufferFromClientBuffer)(const egl::ClientBuffer& clientBuffer);
	egl::Image *(*createDepthStencil)(int width, int height, sw::Format format, int multiSampleDepth);
	sw::FrameBuffer *(*createFrameBuffer)(void *nativeDisplay, EGLNativeWindowType window, int width, int height);
};

class LibGLESv2
{
public:
	LibGLESv2(const std::string libraryDirectory) : libraryDirectory(libraryDirectory)
	{
	}

	~LibGLESv2()
	{
		freeLibrary(libGLESv2);
	}

	operator bool()
	{
		return loadExports() != nullptr;
	}

	LibGLESv2exports *operator->()
	{
		return loadExports();
	}

private:
	LibGLESv2exports *loadExports()
	{
		if(!libGLESv2)
		{
			#if defined(_WIN32)
				#if defined(__LP64__)
					const char *libGLESv2_lib[] = {"libGLESv2.dll", "lib64GLES_V2_translator.dll"};
				#else
					const char *libGLESv2_lib[] = {"libGLESv2.dll", "libGLES_V2_translator.dll"};
				#endif
			#elif defined(__ANDROID__)
				#if defined(__LP64__)
					const char *libGLESv2_lib[] = {"/vendor/lib64/egl/libGLESv2_swiftshader.so", "/system/lib64/egl/libGLESv2_swiftshader.so"};
				#else
					const char *libGLESv2_lib[] = {"/vendor/lib/egl/libGLESv2_swiftshader.so", "/system/lib/egl/libGLESv2_swiftshader.so"};
				#endif
			#elif defined(__linux__)
				#if defined(__LP64__)
					const char *libGLESv2_lib[] = {"lib64GLES_V2_translator.so", "libGLESv2.so.2", "libGLESv2.so"};
				#else
					const char *libGLESv2_lib[] = {"libGLES_V2_translator.so", "libGLESv2.so.2", "libGLESv2.so"};
				#endif
			#elif defined(__APPLE__)
				#if defined(__LP64__)
					const char *libGLESv2_lib[] = {"libswiftshader_libGLESv2.dylib", "lib64GLES_V2_translator.dylib", "libGLESv2.dylib"};
				#else
					const char *libGLESv2_lib[] = {"libswiftshader_libGLESv2.dylib", "libGLES_V2_translator.dylib", "libGLESv2.dylib"};
				#endif
			#elif defined(__Fuchsia__)
				const char *libGLESv2_lib[] = {"libGLESv2.so"};
			#else
				#error "libGLESv2::loadExports unimplemented for this platform"
			#endif

			libGLESv2 = loadLibrary(libraryDirectory, libGLESv2_lib, "libGLESv2_swiftshader");

			if(libGLESv2)
			{
				auto libGLESv2_swiftshader = (LibGLESv2exports *(*)())getProcAddress(libGLESv2, "libGLESv2_swiftshader");
				libGLESv2exports = libGLESv2_swiftshader();
			}
		}

		return libGLESv2exports;
	}

	void *libGLESv2 = nullptr;
	LibGLESv2exports *libGLESv2exports = nullptr;
	const std::string libraryDirectory;
};

#endif   // libGLESv2_hpp
