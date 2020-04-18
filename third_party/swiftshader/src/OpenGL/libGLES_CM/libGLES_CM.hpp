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

#ifndef libGLES_CM_hpp
#define libGLES_CM_hpp

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
}

class LibGLES_CMexports
{
public:
	LibGLES_CMexports();

	void (*glActiveTexture)(GLenum texture);
	void (*glAlphaFunc)(GLenum func, GLclampf ref);
	void (*glAlphaFuncx)(GLenum func, GLclampx ref);
	void (*glBindBuffer)(GLenum target, GLuint buffer);
	void (*glBindFramebuffer)(GLenum target, GLuint framebuffer);
	void (*glBindFramebufferOES)(GLenum target, GLuint framebuffer);
	void (*glBindRenderbufferOES)(GLenum target, GLuint renderbuffer);
	void (*glBindTexture)(GLenum target, GLuint texture);
	void (*glBlendEquationOES)(GLenum mode);
	void (*glBlendEquationSeparateOES)(GLenum modeRGB, GLenum modeAlpha);
	void (*glBlendFunc)(GLenum sfactor, GLenum dfactor);
	void (*glBlendFuncSeparateOES)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	void (*glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
	void (*glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
	GLenum (*glCheckFramebufferStatusOES)(GLenum target);
	void (*glClear)(GLbitfield mask);
	void (*glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void (*glClearColorx)(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);
	void (*glClearDepthf)(GLclampf depth);
	void (*glClearDepthx)(GLclampx depth);
	void (*glClearStencil)(GLint s);
	void (*glClientActiveTexture)(GLenum texture);
	void (*glClipPlanef)(GLenum plane, const GLfloat *equation);
	void (*glClipPlanex)(GLenum plane, const GLfixed *equation);
	void (*glColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void (*glColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
	void (*glColor4x)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
	void (*glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
	void (*glColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void (*glCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
	                               GLint border, GLsizei imageSize, const GLvoid* data);
	void (*glCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	                                  GLenum format, GLsizei imageSize, const GLvoid* data);
	void (*glCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void (*glCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void (*glCullFace)(GLenum mode);
	void (*glDeleteBuffers)(GLsizei n, const GLuint* buffers);
	void (*glDeleteFramebuffersOES)(GLsizei n, const GLuint* framebuffers);
	void (*glDeleteRenderbuffersOES)(GLsizei n, const GLuint* renderbuffers);
	void (*glDeleteTextures)(GLsizei n, const GLuint* textures);
	void (*glDepthFunc)(GLenum func);
	void (*glDepthMask)(GLboolean flag);
	void (*glDepthRangex)(GLclampx zNear, GLclampx zFar);
	void (*glDepthRangef)(GLclampf zNear, GLclampf zFar);
	void (*glDisable)(GLenum cap);
	void (*glDisableClientState)(GLenum array);
	void (*glDrawArrays)(GLenum mode, GLint first, GLsizei count);
	void (*glDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
	void (*glEnable)(GLenum cap);
	void (*glEnableClientState)(GLenum array);
	void (*glFinish)(void);
	void (*glFlush)(void);
	void (*glFramebufferRenderbufferOES)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	void (*glFramebufferTexture2DOES)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void (*glFogf)(GLenum pname, GLfloat param);
	void (*glFogfv)(GLenum pname, const GLfloat *params);
	void (*glFogx)(GLenum pname, GLfixed param);
	void (*glFogxv)(GLenum pname, const GLfixed *params);
	void (*glFrontFace)(GLenum mode);
	void (*glFrustumf)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
	void (*glFrustumx)(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
	void (*glGenerateMipmapOES)(GLenum target);
	void (*glGenBuffers)(GLsizei n, GLuint* buffers);
	void (*glGenFramebuffersOES)(GLsizei n, GLuint* framebuffers);
	void (*glGenRenderbuffersOES)(GLsizei n, GLuint* renderbuffers);
	void (*glGenTextures)(GLsizei n, GLuint* textures);
	void (*glGetRenderbufferParameterivOES)(GLenum target, GLenum pname, GLint* params);
	void (*glGetBooleanv)(GLenum pname, GLboolean* params);
	void (*glGetBufferParameteriv)(GLenum target, GLenum pname, GLint* params);
	void (*glGetClipPlanef)(GLenum pname, GLfloat eqn[4]);
	void (*glGetClipPlanex)(GLenum pname, GLfixed eqn[4]);
	GLenum (*glGetError)(void);
	void (*glGetFixedv)(GLenum pname, GLfixed *params);
	void (*glGetFloatv)(GLenum pname, GLfloat* params);
	void (*glGetFramebufferAttachmentParameterivOES)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
	void (*glGetIntegerv)(GLenum pname, GLint* params);
	void (*glGetLightfv)(GLenum light, GLenum pname, GLfloat *params);
	void (*glGetLightxv)(GLenum light, GLenum pname, GLfixed *params);
	void (*glGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
	void (*glGetMaterialxv)(GLenum face, GLenum pname, GLfixed *params);
	void (*glGetPointerv)(GLenum pname, GLvoid **params);
	const GLubyte* (*glGetString)(GLenum name);
	void (*glGetTexParameterfv)(GLenum target, GLenum pname, GLfloat* params);
	void (*glGetTexParameteriv)(GLenum target, GLenum pname, GLint* params);
	void (*glGetTexEnvfv)(GLenum env, GLenum pname, GLfloat *params);
	void (*glGetTexEnviv)(GLenum env, GLenum pname, GLint *params);
	void (*glGetTexEnvxv)(GLenum env, GLenum pname, GLfixed *params);
	void (*glGetTexParameterxv)(GLenum target, GLenum pname, GLfixed *params);
	void (*glHint)(GLenum target, GLenum mode);
	GLboolean (*glIsBuffer)(GLuint buffer);
	GLboolean (*glIsEnabled)(GLenum cap);
	GLboolean (*glIsFramebufferOES)(GLuint framebuffer);
	GLboolean (*glIsTexture)(GLuint texture);
	GLboolean (*glIsRenderbufferOES)(GLuint renderbuffer);
	void (*glLightModelf)(GLenum pname, GLfloat param);
	void (*glLightModelfv)(GLenum pname, const GLfloat *params);
	void (*glLightModelx)(GLenum pname, GLfixed param);
	void (*glLightModelxv)(GLenum pname, const GLfixed *params);
	void (*glLightf)(GLenum light, GLenum pname, GLfloat param);
	void (*glLightfv)(GLenum light, GLenum pname, const GLfloat *params);
	void (*glLightx)(GLenum light, GLenum pname, GLfixed param);
	void (*glLightxv)(GLenum light, GLenum pname, const GLfixed *params);
	void (*glLineWidth)(GLfloat width);
	void (*glLineWidthx)(GLfixed width);
	void (*glLoadIdentity)(void);
	void (*glLoadMatrixf)(const GLfloat *m);
	void (*glLoadMatrixx)(const GLfixed *m);
	void (*glLogicOp)(GLenum opcode);
	void (*glMaterialf)(GLenum face, GLenum pname, GLfloat param);
	void (*glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
	void (*glMaterialx)(GLenum face, GLenum pname, GLfixed param);
	void (*glMaterialxv)(GLenum face, GLenum pname, const GLfixed *params);
	void (*glMatrixMode)(GLenum mode);
	void (*glMultMatrixf)(const GLfloat *m);
	void (*glMultMatrixx)(const GLfixed *m);
	void (*glMultiTexCoord4f)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	void (*glMultiTexCoord4x)(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
	void (*glNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
	void (*glNormal3x)(GLfixed nx, GLfixed ny, GLfixed nz);
	void (*glNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void (*glOrthof)(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
	void (*glOrthox)(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
	void (*glPixelStorei)(GLenum pname, GLint param);
	void (*glPointParameterf)(GLenum pname, GLfloat param);
	void (*glPointParameterfv)(GLenum pname, const GLfloat *params);
	void (*glPointParameterx)(GLenum pname, GLfixed param);
	void (*glPointParameterxv)(GLenum pname, const GLfixed *params);
	void (*glPointSize)(GLfloat size);
	void (*glPointSizePointerOES)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void (*glPointSizex)(GLfixed size);
	void (*glPolygonOffset)(GLfloat factor, GLfloat units);
	void (*glPolygonOffsetx)(GLfixed factor, GLfixed units);
	void (*glPopMatrix)(void);
	void (*glPushMatrix)(void);
	void (*glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
	void (*glRenderbufferStorageOES)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void (*glRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void (*glRotatex)(GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
	void (*glSampleCoverage)(GLclampf value, GLboolean invert);
	void (*glSampleCoveragex)(GLclampx value, GLboolean invert);
	void (*glScalef)(GLfloat x, GLfloat y, GLfloat z);
	void (*glScalex)(GLfixed x, GLfixed y, GLfixed z);
	void (*glScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
	void (*glShadeModel)(GLenum mode);
	void (*glStencilFunc)(GLenum func, GLint ref, GLuint mask);
	void (*glStencilMask)(GLuint mask);
	void (*glStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
	void (*glTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void (*glTexEnvf)(GLenum target, GLenum pname, GLfloat param);
	void (*glTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
	void (*glTexEnvi)(GLenum target, GLenum pname, GLint param);
	void (*glTexEnvx)(GLenum target, GLenum pname, GLfixed param);
	void (*glTexEnviv)(GLenum target, GLenum pname, const GLint *params);
	void (*glTexEnvxv)(GLenum target, GLenum pname, const GLfixed *params);
	void (*glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
	                     GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	void (*glTexParameterf)(GLenum target, GLenum pname, GLfloat param);
	void (*glTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params);
	void (*glTexParameteri)(GLenum target, GLenum pname, GLint param);
	void (*glTexParameteriv)(GLenum target, GLenum pname, const GLint* params);
	void (*glTexParameterx)(GLenum target, GLenum pname, GLfixed param);
	void (*glTexParameterxv)(GLenum target, GLenum pname, const GLfixed *params);
	void (*glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	                        GLenum format, GLenum type, const GLvoid* pixels);
	void (*glTranslatef)(GLfloat x, GLfloat y, GLfloat z);
	void (*glTranslatex)(GLfixed x, GLfixed y, GLfixed z);
	void (*glVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void (*glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
	void (*glEGLImageTargetTexture2DOES)(GLenum target, GLeglImageOES image);
	void (*glEGLImageTargetRenderbufferStorageOES)(GLenum target, GLeglImageOES image);
	void (*glDrawTexsOES)(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height);
	void (*glDrawTexiOES)(GLint x, GLint y, GLint z, GLint width, GLint height);
	void (*glDrawTexxOES)(GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height);
	void (*glDrawTexsvOES)(const GLshort *coords);
	void (*glDrawTexivOES)(const GLint *coords);
	void (*glDrawTexxvOES)(const GLfixed *coords);
	void (*glDrawTexfOES)(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);
	void (*glDrawTexfvOES)(const GLfloat *coords);

	egl::Context *(*es1CreateContext)(egl::Display *display, const egl::Context *shareContext, const egl::Config *config);
	__eglMustCastToProperFunctionPointerType (*es1GetProcAddress)(const char *procname);
	egl::Image *(*createBackBuffer)(int width, int height, sw::Format format, int multiSampleDepth);
	egl::Image *(*createDepthStencil)(int width, int height, sw::Format format, int multiSampleDepth);
	sw::FrameBuffer *(*createFrameBuffer)(void *nativeDisplay, EGLNativeWindowType window, int width, int height);
};

class LibGLES_CM
{
public:
	LibGLES_CM(const std::string libraryDirectory) : libraryDirectory(libraryDirectory)
	{
	}

	~LibGLES_CM()
	{
		freeLibrary(libGLES_CM);
	}

	operator bool()
	{
		return loadExports() != nullptr;
	}

	LibGLES_CMexports *operator->()
	{
		return loadExports();
	}

private:
	LibGLES_CMexports *loadExports()
	{
		if(!libGLES_CM)
		{
			#if defined(_WIN32)
				#if defined(__LP64__)
					const char *libGLES_CM_lib[] = {"libGLES_CM.dll", "lib64GLES_CM_translator.dll"};
				#else
					const char *libGLES_CM_lib[] = {"libGLES_CM.dll", "libGLES_CM_translator.dll"};
				#endif
			#elif defined(__ANDROID__)
				#if defined(__LP64__)
					const char *libGLES_CM_lib[] = {"/vendor/lib64/egl/libGLESv1_CM_swiftshader.so", "/system/lib64/egl/libGLESv1_CM_swiftshader.so"};
				#else
					const char *libGLES_CM_lib[] = {"/vendor/lib/egl/libGLESv1_CM_swiftshader.so", "/system/lib/egl/libGLESv1_CM_swiftshader.so"};
				#endif
			#elif defined(__linux__)
				#if defined(__LP64__)
					const char *libGLES_CM_lib[] = {"lib64GLES_CM_translator.so", "libGLES_CM.so.1", "libGLES_CM.so"};
				#else
					const char *libGLES_CM_lib[] = {"libGLES_CM_translator.so", "libGLES_CM.so.1", "libGLES_CM.so"};
				#endif
			#elif defined(__APPLE__)
				#if defined(__LP64__)
					const char *libGLES_CM_lib[] = {"lib64GLES_CM_translator.dylib", "libGLES_CM.dylib"};
				#else
					const char *libGLES_CM_lib[] = {"libGLES_CM_translator.dylib", "libGLES_CM.dylib"};
				#endif
			#elif defined(__Fuchsia__)
				const char *libGLES_CM_lib[] = {"libGLES_CM.so"};
			#else
				#error "libGLES_CM::loadExports unimplemented for this platform"
			#endif

			libGLES_CM = loadLibrary(libraryDirectory, libGLES_CM_lib, "libGLES_CM_swiftshader");

			if(libGLES_CM)
			{
				auto libGLES_CM_swiftshader = (LibGLES_CMexports *(*)())getProcAddress(libGLES_CM, "libGLES_CM_swiftshader");
				libGLES_CMexports = libGLES_CM_swiftshader();
			}
		}

		return libGLES_CMexports;
	}

	void *libGLES_CM = nullptr;
	LibGLES_CMexports *libGLES_CMexports = nullptr;
	const std::string libraryDirectory;
};

#endif   // libGLES_CM_hpp
