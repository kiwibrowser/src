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

// main.cpp: DLL entry point and management of thread-local data.

#include "main.h"

#include "libGLES_CM.hpp"
#include "Framebuffer.h"
#include "common/Surface.hpp"
#include "Common/Thread.hpp"
#include "Common/SharedLibrary.hpp"
#include "common/debug.h"

#include <GLES/glext.h>

#if !defined(_MSC_VER)
#define CONSTRUCTOR __attribute__((constructor))
#define DESTRUCTOR __attribute__((destructor))
#else
#define CONSTRUCTOR
#define DESTRUCTOR
#endif

static void glAttachThread()
{
	TRACE("()");
}

static void glDetachThread()
{
	TRACE("()");
}

CONSTRUCTOR static void glAttachProcess()
{
	TRACE("()");

	glAttachThread();
}

DESTRUCTOR static void glDetachProcess()
{
	TRACE("()");

	glDetachThread();
}

#if defined(_WIN32)
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch(reason)
	{
	case DLL_PROCESS_ATTACH:
		glAttachProcess();
		break;
	case DLL_THREAD_ATTACH:
		glAttachThread();
		break;
	case DLL_THREAD_DETACH:
		glDetachThread();
		break;
	case DLL_PROCESS_DETACH:
		glDetachProcess();
		break;
	default:
		break;
	}

	return TRUE;
}
#endif

namespace es1
{
es1::Context *getContext()
{
	egl::Context *context = libEGL->clientGetCurrentContext();

	if(context && context->getClientVersion() == 1)
	{
		return static_cast<es1::Context*>(context);
	}

	return nullptr;
}

Device *getDevice()
{
	Context *context = getContext();

	return context ? context->getDevice() : nullptr;
}

// Records an error code
void error(GLenum errorCode)
{
	es1::Context *context = es1::getContext();

	if(context)
	{
		switch(errorCode)
		{
		case GL_INVALID_ENUM:
			context->recordInvalidEnum();
			TRACE("\t! Error generated: invalid enum\n");
			break;
		case GL_INVALID_VALUE:
			context->recordInvalidValue();
			TRACE("\t! Error generated: invalid value\n");
			break;
		case GL_INVALID_OPERATION:
			context->recordInvalidOperation();
			TRACE("\t! Error generated: invalid operation\n");
			break;
		case GL_OUT_OF_MEMORY:
			context->recordOutOfMemory();
			TRACE("\t! Error generated: out of memory\n");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION_OES:
			context->recordInvalidFramebufferOperation();
			TRACE("\t! Error generated: invalid framebuffer operation\n");
			break;
		case GL_STACK_OVERFLOW:
			context->recordMatrixStackOverflow();
			TRACE("\t! Error generated: matrix stack overflow\n");
			break;
		case GL_STACK_UNDERFLOW:
			context->recordMatrixStackUnderflow();
			TRACE("\t! Error generated: matrix stack underflow\n");
			break;
		default: UNREACHABLE(errorCode);
		}
	}
}
}

namespace es1
{
void ActiveTexture(GLenum texture);
void AlphaFunc(GLenum func, GLclampf ref);
void AlphaFuncx(GLenum func, GLclampx ref);
void BindBuffer(GLenum target, GLuint buffer);
void BindFramebuffer(GLenum target, GLuint framebuffer);
void BindFramebufferOES(GLenum target, GLuint framebuffer);
void BindRenderbufferOES(GLenum target, GLuint renderbuffer);
void BindTexture(GLenum target, GLuint texture);
void BlendEquationSeparateOES(GLenum modeRGB, GLenum modeAlpha);
void BlendEquationOES(GLenum mode);
void BlendEquationSeparateOES(GLenum modeRGB, GLenum modeAlpha);
void BlendFuncSeparateOES(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void BlendFunc(GLenum sfactor, GLenum dfactor);
void BlendFuncSeparateOES(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
GLenum CheckFramebufferStatusOES(GLenum target);
void Clear(GLbitfield mask);
void ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);
void ClearDepthf(GLclampf depth);
void ClearDepthx(GLclampx depth);
void ClearStencil(GLint s);
void ClientActiveTexture(GLenum texture);
void ClipPlanef(GLenum plane, const GLfloat *equation);
void ClipPlanex(GLenum plane, const GLfixed *equation);
void Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void Color4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                          GLint border, GLsizei imageSize, const GLvoid* data);
void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                             GLenum format, GLsizei imageSize, const GLvoid* data);
void CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void CullFace(GLenum mode);
void DeleteBuffers(GLsizei n, const GLuint* buffers);
void DeleteFramebuffersOES(GLsizei n, const GLuint* framebuffers);
void DeleteRenderbuffersOES(GLsizei n, const GLuint* renderbuffers);
void DeleteTextures(GLsizei n, const GLuint* textures);
void DepthFunc(GLenum func);
void DepthMask(GLboolean flag);
void DepthRangex(GLclampx zNear, GLclampx zFar);
void DepthRangef(GLclampf zNear, GLclampf zFar);
void Disable(GLenum cap);
void DisableClientState(GLenum array);
void DrawArrays(GLenum mode, GLint first, GLsizei count);
void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
void Enable(GLenum cap);
void EnableClientState(GLenum array);
void Finish(void);
void Flush(void);
void FramebufferRenderbufferOES(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void FramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void Fogf(GLenum pname, GLfloat param);
void Fogfv(GLenum pname, const GLfloat *params);
void Fogx(GLenum pname, GLfixed param);
void Fogxv(GLenum pname, const GLfixed *params);
void FrontFace(GLenum mode);
void Frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void Frustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
void GenerateMipmapOES(GLenum target);
void GenBuffers(GLsizei n, GLuint* buffers);
void GenFramebuffersOES(GLsizei n, GLuint* framebuffers);
void GenRenderbuffersOES(GLsizei n, GLuint* renderbuffers);
void GenTextures(GLsizei n, GLuint* textures);
void GetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint* params);
void GetBooleanv(GLenum pname, GLboolean* params);
void GetBufferParameteriv(GLenum target, GLenum pname, GLint* params);
void GetClipPlanef(GLenum pname, GLfloat eqn[4]);
void GetClipPlanex(GLenum pname, GLfixed eqn[4]);
GLenum GetError(void);
void GetFixedv(GLenum pname, GLfixed *params);
void GetFloatv(GLenum pname, GLfloat* params);
void GetFramebufferAttachmentParameterivOES(GLenum target, GLenum attachment, GLenum pname, GLint* params);
void GetIntegerv(GLenum pname, GLint* params);
void GetLightfv(GLenum light, GLenum pname, GLfloat *params);
void GetLightxv(GLenum light, GLenum pname, GLfixed *params);
void GetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void GetMaterialxv(GLenum face, GLenum pname, GLfixed *params);
void GetPointerv(GLenum pname, GLvoid **params);
const GLubyte* GetString(GLenum name);
void GetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
void GetTexParameteriv(GLenum target, GLenum pname, GLint* params);
void GetTexEnvfv(GLenum env, GLenum pname, GLfloat *params);
void GetTexEnviv(GLenum env, GLenum pname, GLint *params);
void GetTexEnvxv(GLenum env, GLenum pname, GLfixed *params);
void GetTexParameterxv(GLenum target, GLenum pname, GLfixed *params);
void Hint(GLenum target, GLenum mode);
GLboolean IsBuffer(GLuint buffer);
GLboolean IsEnabled(GLenum cap);
GLboolean IsFramebufferOES(GLuint framebuffer);
GLboolean IsTexture(GLuint texture);
GLboolean IsRenderbufferOES(GLuint renderbuffer);
void LightModelf(GLenum pname, GLfloat param);
void LightModelfv(GLenum pname, const GLfloat *params);
void LightModelx(GLenum pname, GLfixed param);
void LightModelxv(GLenum pname, const GLfixed *params);
void Lightf(GLenum light, GLenum pname, GLfloat param);
void Lightfv(GLenum light, GLenum pname, const GLfloat *params);
void Lightx(GLenum light, GLenum pname, GLfixed param);
void Lightxv(GLenum light, GLenum pname, const GLfixed *params);
void LineWidth(GLfloat width);
void LineWidthx(GLfixed width);
void LoadIdentity(void);
void LoadMatrixf(const GLfloat *m);
void LoadMatrixx(const GLfixed *m);
void LogicOp(GLenum opcode);
void Materialf(GLenum face, GLenum pname, GLfloat param);
void Materialfv(GLenum face, GLenum pname, const GLfloat *params);
void Materialx(GLenum face, GLenum pname, GLfixed param);
void Materialxv(GLenum face, GLenum pname, const GLfixed *params);
void MatrixMode(GLenum mode);
void MultMatrixf(const GLfloat *m);
void MultMatrixx(const GLfixed *m);
void MultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void MultiTexCoord4x(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
void Normal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void Normal3x(GLfixed nx, GLfixed ny, GLfixed nz);
void NormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void Orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void Orthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
void PixelStorei(GLenum pname, GLint param);
void PointParameterf(GLenum pname, GLfloat param);
void PointParameterfv(GLenum pname, const GLfloat *params);
void PointParameterx(GLenum pname, GLfixed param);
void PointParameterxv(GLenum pname, const GLfixed *params);
void PointSize(GLfloat size);
void PointSizePointerOES(GLenum type, GLsizei stride, const GLvoid *pointer);
void PointSizex(GLfixed size);
void PolygonOffset(GLfloat factor, GLfloat units);
void PolygonOffsetx(GLfixed factor, GLfixed units);
void PopMatrix(void);
void PushMatrix(void);
void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
void RenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void Rotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
void SampleCoverage(GLclampf value, GLboolean invert);
void SampleCoveragex(GLclampx value, GLboolean invert);
void Scalef(GLfloat x, GLfloat y, GLfloat z);
void Scalex(GLfixed x, GLfixed y, GLfixed z);
void Scissor(GLint x, GLint y, GLsizei width, GLsizei height);
void ShadeModel(GLenum mode);
void StencilFunc(GLenum func, GLint ref, GLuint mask);
void StencilMask(GLuint mask);
void StencilOp(GLenum fail, GLenum zfail, GLenum zpass);
void TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void TexEnvf(GLenum target, GLenum pname, GLfloat param);
void TexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
void TexEnvi(GLenum target, GLenum pname, GLint param);
void TexEnvx(GLenum target, GLenum pname, GLfixed param);
void TexEnviv(GLenum target, GLenum pname, const GLint *params);
void TexEnvxv(GLenum target, GLenum pname, const GLfixed *params);
void TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                GLint border, GLenum format, GLenum type, const GLvoid* pixels);
void TexParameterf(GLenum target, GLenum pname, GLfloat param);
void TexParameterfv(GLenum target, GLenum pname, const GLfloat* params);
void TexParameteri(GLenum target, GLenum pname, GLint param);
void TexParameteriv(GLenum target, GLenum pname, const GLint* params);
void TexParameterx(GLenum target, GLenum pname, GLfixed param);
void TexParameterxv(GLenum target, GLenum pname, const GLfixed *params);
void TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* pixels);
void Translatef(GLfloat x, GLfloat y, GLfloat z);
void Translatex(GLfixed x, GLfixed y, GLfixed z);
void VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void Viewport(GLint x, GLint y, GLsizei width, GLsizei height);
void EGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image);
void EGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image);
void DrawTexsOES(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height);
void DrawTexiOES(GLint x, GLint y, GLint z, GLint width, GLint height);
void DrawTexxOES(GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height);
void DrawTexsvOES(const GLshort *coords);
void DrawTexivOES(const GLint *coords);
void DrawTexxvOES(const GLfixed *coords);
void DrawTexfOES(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);
void DrawTexfvOES(const GLfloat *coords);
}

egl::Context *es1CreateContext(egl::Display *display, const egl::Context *shareContext, const egl::Config *config);
extern "C" __eglMustCastToProperFunctionPointerType es1GetProcAddress(const char *procname);
egl::Image *createBackBuffer(int width, int height, sw::Format format, int multiSampleDepth);
egl::Image *createDepthStencil(int width, int height, sw::Format format, int multiSampleDepth);
sw::FrameBuffer *createFrameBuffer(void *nativeDisplay, EGLNativeWindowType window, int width, int height);

extern "C"
{
EGLAPI EGLint EGLAPIENTRY eglGetError(void)
{
	return libEGL->eglGetError();
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
	return libEGL->eglGetDisplay(display_id);
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	return libEGL->eglInitialize(dpy, major, minor);
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
	return libEGL->eglTerminate(dpy);
}

EGLAPI const char *EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
	return libEGL->eglQueryString(dpy, name);
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	return libEGL->eglGetConfigs(dpy, configs, config_size, num_config);
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	return libEGL->eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
	return libEGL->eglGetConfigAttrib(dpy, config, attribute, value);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window, const EGLint *attrib_list)
{
	return libEGL->eglCreateWindowSurface(dpy, config, window, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
	return libEGL->eglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
	return libEGL->eglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	return libEGL->eglDestroySurface(dpy, surface);
}

EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
	return libEGL->eglQuerySurface(dpy, surface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api)
{
	return libEGL->eglBindAPI(api);
}

EGLAPI EGLenum EGLAPIENTRY eglQueryAPI(void)
{
	return libEGL->eglQueryAPI();
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitClient(void)
{
	return libEGL->eglWaitClient();
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseThread(void)
{
	return libEGL->eglReleaseThread();
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
	return libEGL->eglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
	return libEGL->eglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	return libEGL->eglBindTexImage(dpy, surface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	return libEGL->eglReleaseTexImage(dpy, surface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	return libEGL->eglSwapInterval(dpy, interval);
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	return libEGL->eglCreateContext(dpy, config, share_context, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	return libEGL->eglDestroyContext(dpy, ctx);
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	return libEGL->eglMakeCurrent(dpy, draw, read, ctx);
}

EGLAPI EGLContext EGLAPIENTRY eglGetCurrentContext(void)
{
	return libEGL->eglGetCurrentContext();
}

EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw)
{
	return libEGL->eglGetCurrentSurface(readdraw);
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
	return libEGL->eglGetCurrentDisplay();
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
	return libEGL->eglQueryContext(dpy, ctx, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitGL(void)
{
	return libEGL->eglWaitGL();
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine)
{
	return libEGL->eglWaitNative(engine);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
	return libEGL->eglSwapBuffers(dpy, surface);
}

EGLAPI EGLBoolean EGLAPIENTRY eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
	return libEGL->eglCopyBuffers(dpy, surface, target);
}

EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
	return libEGL->eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
	return libEGL->eglDestroyImageKHR(dpy, image);
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress(const char *procname)
{
	return libEGL->eglGetProcAddress(procname);
}

EGLAPI EGLSyncKHR EGLAPIENTRY eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
	return libEGL->eglCreateSyncKHR(dpy, type, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
	return libEGL->eglDestroySyncKHR(dpy, sync);
}

EGLAPI EGLint EGLAPIENTRY eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
	return libEGL->eglClientWaitSyncKHR(dpy, sync, flags, timeout);
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value)
{
	return libEGL->eglGetSyncAttribKHR(dpy, sync, attribute, value);
}

GL_API void GL_APIENTRY glActiveTexture(GLenum texture)
{
	return es1::ActiveTexture(texture);
}

GL_API void GL_APIENTRY glAlphaFunc(GLenum func, GLclampf ref)
{
	return es1::AlphaFunc(func, ref);
}

GL_API void GL_APIENTRY glAlphaFuncx(GLenum func, GLclampx ref)
{
	return es1::AlphaFuncx(func, ref);
}

GL_API void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
	return es1::BindBuffer(target, buffer);
}

GL_API void GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	return es1::BindFramebuffer(target, framebuffer);
}

GL_API void GL_APIENTRY glBindFramebufferOES(GLenum target, GLuint framebuffer)
{
	return es1::BindFramebufferOES(target, framebuffer);
}

GL_API void GL_APIENTRY glBindRenderbufferOES(GLenum target, GLuint renderbuffer)
{
	return es1::BindRenderbufferOES(target, renderbuffer);
}

GL_API void GL_APIENTRY glBindTexture(GLenum target, GLuint texture)
{
	return es1::BindTexture(target, texture);
}

GL_API void GL_APIENTRY glBlendEquationOES(GLenum mode)
{
	return es1::BlendEquationSeparateOES(mode, mode);
}

GL_API void GL_APIENTRY glBlendEquationSeparateOES(GLenum modeRGB, GLenum modeAlpha)
{
	return es1::BlendEquationSeparateOES(modeRGB, modeAlpha);
}

GL_API void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	return es1::BlendFuncSeparateOES(sfactor, dfactor, sfactor, dfactor);
}

GL_API void GL_APIENTRY glBlendFuncSeparateOES(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
	return es1::BlendFuncSeparateOES(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GL_API void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	return es1::BufferData(target, size, data, usage);
}

GL_API void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	return es1::BufferSubData(target, offset, size, data);
}

GL_API GLenum GL_APIENTRY glCheckFramebufferStatusOES(GLenum target)
{
	return es1::CheckFramebufferStatusOES(target);
}

GL_API void GL_APIENTRY glClear(GLbitfield mask)
{
	return es1::Clear(mask);
}

GL_API void GL_APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	return es1::ClearColor(red, green, blue, alpha);
}

GL_API void GL_APIENTRY glClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)
{
	return es1::ClearColorx(red, green, blue, alpha);
}

GL_API void GL_APIENTRY glClearDepthf(GLclampf depth)
{
	return es1::ClearDepthf(depth);
}

GL_API void GL_APIENTRY glClearDepthx(GLclampx depth)
{
	return es1::ClearDepthx(depth);
}

GL_API void GL_APIENTRY glClearStencil(GLint s)
{
	return es1::ClearStencil(s);
}

GL_API void GL_APIENTRY glClientActiveTexture(GLenum texture)
{
	return es1::ClientActiveTexture(texture);
}

GL_API void GL_APIENTRY glClipPlanef(GLenum plane, const GLfloat *equation)
{
	return es1::ClipPlanef(plane, equation);
}

GL_API void GL_APIENTRY glClipPlanex(GLenum plane, const GLfixed *equation)
{
	return es1::ClipPlanex(plane, equation);
}

GL_API void GL_APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	return es1::Color4f(red, green, blue, alpha);
}

GL_API void GL_APIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	return es1::Color4ub(red, green, blue, alpha);
}

GL_API void GL_APIENTRY glColor4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
	return es1::Color4x(red, green, blue, alpha);
}

GL_API void GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	return es1::ColorMask(red, green, blue, alpha);
}

GL_API void GL_APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	return es1::ColorPointer(size, type, stride, pointer);
}

GL_API void GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                               GLint border, GLsizei imageSize, const GLvoid* data)
{
	return es1::CompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

GL_API void GL_APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                                  GLenum format, GLsizei imageSize, const GLvoid* data)
{
	return es1::CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

GL_API void GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	return es1::CopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

GL_API void GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	return es1::CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

GL_API void GL_APIENTRY glCullFace(GLenum mode)
{
	return es1::CullFace(mode);
}

GL_API void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	return es1::DeleteBuffers(n, buffers);
}

GL_API void GL_APIENTRY glDeleteFramebuffersOES(GLsizei n, const GLuint* framebuffers)
{
	return es1::DeleteFramebuffersOES(n, framebuffers);
}

GL_API void GL_APIENTRY glDeleteRenderbuffersOES(GLsizei n, const GLuint* renderbuffers)
{
	return es1::DeleteRenderbuffersOES(n, renderbuffers);
}

GL_API void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures)
{
	return es1::DeleteTextures(n, textures);
}

GL_API void GL_APIENTRY glDepthFunc(GLenum func)
{
	return es1::DepthFunc(func);
}

GL_API void GL_APIENTRY glDepthMask(GLboolean flag)
{
	return es1::DepthMask(flag);
}

GL_API void GL_APIENTRY glDepthRangex(GLclampx zNear, GLclampx zFar)
{
	return es1::DepthRangex(zNear, zFar);
}

GL_API void GL_APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar)
{
	return es1::DepthRangef(zNear, zFar);
}

GL_API void GL_APIENTRY glDisable(GLenum cap)
{
	return es1::Disable(cap);
}

GL_API void GL_APIENTRY glDisableClientState(GLenum array)
{
	return es1::DisableClientState(array);
}

GL_API void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	return es1::DrawArrays(mode, first, count);
}

GL_API void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	return es1::DrawElements(mode, count, type, indices);
}

GL_API void GL_APIENTRY glEnable(GLenum cap)
{
	return es1::Enable(cap);
}

GL_API void GL_APIENTRY glEnableClientState(GLenum array)
{
	return es1::EnableClientState(array);
}

GL_API void GL_APIENTRY glFinish(void)
{
	return es1::Finish();
}

GL_API void GL_APIENTRY glFlush(void)
{
	return es1::Flush();
}

GL_API void GL_APIENTRY glFramebufferRenderbufferOES(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	return es1::FramebufferRenderbufferOES(target, attachment, renderbuffertarget, renderbuffer);
}

GL_API void GL_APIENTRY glFramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	return es1::FramebufferTexture2DOES(target, attachment, textarget, texture, level);
}

GL_API void GL_APIENTRY glFogf(GLenum pname, GLfloat param)
{
	return es1::Fogf(pname, param);
}

GL_API void GL_APIENTRY glFogfv(GLenum pname, const GLfloat *params)
{
	return es1::Fogfv(pname, params);
}

GL_API void GL_APIENTRY glFogx(GLenum pname, GLfixed param)
{
	return es1::Fogx(pname, param);
}

GL_API void GL_APIENTRY glFogxv(GLenum pname, const GLfixed *params)
{
	return es1::Fogxv(pname, params);
}

GL_API void GL_APIENTRY glFrontFace(GLenum mode)
{
	return es1::FrontFace(mode);
}

GL_API void GL_APIENTRY glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	return es1::Frustumf(left, right, bottom, top, zNear, zFar);
}

GL_API void GL_APIENTRY glFrustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
	return es1::Frustumx(left, right, bottom, top, zNear, zFar);
}

GL_API void GL_APIENTRY glGenerateMipmapOES(GLenum target)
{
	return es1::GenerateMipmapOES(target);
}

GL_API void GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{
	return es1::GenBuffers(n, buffers);
}

GL_API void GL_APIENTRY glGenFramebuffersOES(GLsizei n, GLuint* framebuffers)
{
	return es1::GenFramebuffersOES(n, framebuffers);
}

GL_API void GL_APIENTRY glGenRenderbuffersOES(GLsizei n, GLuint* renderbuffers)
{
	return es1::GenRenderbuffersOES(n, renderbuffers);
}

GL_API void GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
	return es1::GenTextures(n, textures);
}

GL_API void GL_APIENTRY glGetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint* params)
{
	return es1::GetRenderbufferParameterivOES(target, pname, params);
}

GL_API void GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean* params)
{
	return es1::GetBooleanv(pname, params);
}

GL_API void GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
	return es1::GetBufferParameteriv(target, pname, params);
}

GL_API void GL_APIENTRY glGetClipPlanef(GLenum pname, GLfloat eqn[4])
{
	return es1::GetClipPlanef(pname, eqn);
}

GL_API void GL_APIENTRY glGetClipPlanex(GLenum pname, GLfixed eqn[4])
{
	return es1::GetClipPlanex(pname, eqn);
}

GL_API GLenum GL_APIENTRY glGetError(void)
{
	return es1::GetError();
}

GL_API void GL_APIENTRY glGetFixedv(GLenum pname, GLfixed *params)
{
	return es1::GetFixedv(pname, params);
}

GL_API void GL_APIENTRY glGetFloatv(GLenum pname, GLfloat* params)
{
	return es1::GetFloatv(pname, params);
}

GL_API void GL_APIENTRY glGetFramebufferAttachmentParameterivOES(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	return es1::GetFramebufferAttachmentParameterivOES(target, attachment, pname, params);
}

GL_API void GL_APIENTRY glGetIntegerv(GLenum pname, GLint* params)
{
	return es1::GetIntegerv(pname, params);
}

GL_API void GL_APIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
	return es1::GetLightfv(light, pname, params);
}

GL_API void GL_APIENTRY glGetLightxv(GLenum light, GLenum pname, GLfixed *params)
{
	return es1::GetLightxv(light, pname, params);
}

GL_API void GL_APIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
	return es1::GetMaterialfv(face, pname, params);
}

GL_API void GL_APIENTRY glGetMaterialxv(GLenum face, GLenum pname, GLfixed *params)
{
	return es1::GetMaterialxv(face, pname, params);
}

GL_API void GL_APIENTRY glGetPointerv(GLenum pname, GLvoid **params)
{
	return es1::GetPointerv(pname, params);
}

GL_API const GLubyte* GL_APIENTRY glGetString(GLenum name)
{
	return es1::GetString(name);
}

GL_API void GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
	return es1::GetTexParameterfv(target, pname, params);
}

GL_API void GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
	return es1::GetTexParameteriv(target, pname, params);
}

GL_API void GL_APIENTRY glGetTexEnvfv(GLenum env, GLenum pname, GLfloat *params)
{
	return es1::GetTexEnvfv(env, pname, params);
}

GL_API void GL_APIENTRY glGetTexEnviv(GLenum env, GLenum pname, GLint *params)
{
	return es1::GetTexEnviv(env, pname, params);
}

GL_API void GL_APIENTRY glGetTexEnvxv(GLenum env, GLenum pname, GLfixed *params)
{
	return es1::GetTexEnvxv(env, pname, params);
}

GL_API void GL_APIENTRY glGetTexParameterxv(GLenum target, GLenum pname, GLfixed *params)
{
	return es1::GetTexParameterxv(target, pname, params);
}

GL_API void GL_APIENTRY glHint(GLenum target, GLenum mode)
{
	return es1::Hint(target, mode);
}

GL_API GLboolean GL_APIENTRY glIsBuffer(GLuint buffer)
{
	return es1::IsBuffer(buffer);
}

GL_API GLboolean GL_APIENTRY glIsEnabled(GLenum cap)
{
	return es1::IsEnabled(cap);
}

GL_API GLboolean GL_APIENTRY glIsFramebufferOES(GLuint framebuffer)
{
	return es1::IsFramebufferOES(framebuffer);
}

GL_API GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
	return es1::IsTexture(texture);
}

GL_API GLboolean GL_APIENTRY glIsRenderbufferOES(GLuint renderbuffer)
{
	return es1::IsRenderbufferOES(renderbuffer);
}

GL_API void GL_APIENTRY glLightModelf(GLenum pname, GLfloat param)
{
	return es1::LightModelf(pname, param);
}

GL_API void GL_APIENTRY glLightModelfv(GLenum pname, const GLfloat *params)
{
	return es1::LightModelfv(pname, params);
}

GL_API void GL_APIENTRY glLightModelx(GLenum pname, GLfixed param)
{
	return es1::LightModelx(pname, param);
}

GL_API void GL_APIENTRY glLightModelxv(GLenum pname, const GLfixed *params)
{
	return es1::LightModelxv(pname, params);
}

GL_API void GL_APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param)
{
	return es1::Lightf(light, pname, param);
}

GL_API void GL_APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	return es1::Lightfv(light, pname, params);
}

GL_API void GL_APIENTRY glLightx(GLenum light, GLenum pname, GLfixed param)
{
	return es1::Lightx(light, pname, param);
}

GL_API void GL_APIENTRY glLightxv(GLenum light, GLenum pname, const GLfixed *params)
{
	return es1::Lightxv(light, pname, params);
}

GL_API void GL_APIENTRY glLineWidth(GLfloat width)
{
	return es1::LineWidth(width);
}

GL_API void GL_APIENTRY glLineWidthx(GLfixed width)
{
	return es1::LineWidthx(width);
}

GL_API void GL_APIENTRY glLoadIdentity(void)
{
	return es1::LoadIdentity();
}

GL_API void GL_APIENTRY glLoadMatrixf(const GLfloat *m)
{
	return es1::LoadMatrixf(m);
}

GL_API void GL_APIENTRY glLoadMatrixx(const GLfixed *m)
{
	return es1::LoadMatrixx(m);
}

GL_API void GL_APIENTRY glLogicOp(GLenum opcode)
{
	return es1::LogicOp(opcode);
}

GL_API void GL_APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
	return es1::Materialf(face, pname, param);
}

GL_API void GL_APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
	return es1::Materialfv(face, pname, params);
}

GL_API void GL_APIENTRY glMaterialx(GLenum face, GLenum pname, GLfixed param)
{
	return es1::Materialx(face, pname, param);
}

GL_API void GL_APIENTRY glMaterialxv(GLenum face, GLenum pname, const GLfixed *params)
{
	return es1::Materialxv(face, pname, params);
}

GL_API void GL_APIENTRY glMatrixMode(GLenum mode)
{
	return es1::MatrixMode(mode);
}

GL_API void GL_APIENTRY glMultMatrixf(const GLfloat *m)
{
	return es1::MultMatrixf(m);
}

GL_API void GL_APIENTRY glMultMatrixx(const GLfixed *m)
{
	return es1::MultMatrixx(m);
}

GL_API void GL_APIENTRY glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	return es1::MultiTexCoord4f(target, s, t, r, q);
}

GL_API void GL_APIENTRY glMultiTexCoord4x(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q)
{
	return es1::MultiTexCoord4x(target, s, t, r, q);
}

GL_API void GL_APIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	return es1::Normal3f(nx, ny, nz);
}

GL_API void GL_APIENTRY glNormal3x(GLfixed nx, GLfixed ny, GLfixed nz)
{
	return es1::Normal3x(nx, ny, nz);
}

GL_API void GL_APIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	return es1::NormalPointer(type, stride, pointer);
}

GL_API void GL_APIENTRY glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	return es1::Orthof(left, right, bottom, top, zNear, zFar);
}

GL_API void GL_APIENTRY glOrthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
	return es1::Orthox(left, right, bottom, top, zNear, zFar);
}

GL_API void GL_APIENTRY glPixelStorei(GLenum pname, GLint param)
{
	return es1::PixelStorei(pname, param);
}

GL_API void GL_APIENTRY glPointParameterf(GLenum pname, GLfloat param)
{
	return es1::PointParameterf(pname, param);
}

GL_API void GL_APIENTRY glPointParameterfv(GLenum pname, const GLfloat *params)
{
	return es1::PointParameterfv(pname, params);
}

GL_API void GL_APIENTRY glPointParameterx(GLenum pname, GLfixed param)
{
	return es1::PointParameterx(pname, param);
}

GL_API void GL_APIENTRY glPointParameterxv(GLenum pname, const GLfixed *params)
{
	return es1::PointParameterxv(pname, params);
}

GL_API void GL_APIENTRY glPointSize(GLfloat size)
{
	return es1::PointSize(size);
}

GL_API void GL_APIENTRY glPointSizePointerOES(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	return es1::PointSizePointerOES(type, stride, pointer);
}

GL_API void GL_APIENTRY glPointSizex(GLfixed size)
{
	return es1::PointSizex(size);
}

GL_API void GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
	return es1::PolygonOffset(factor, units);
}

GL_API void GL_APIENTRY glPolygonOffsetx(GLfixed factor, GLfixed units)
{
	return es1::PolygonOffsetx(factor, units);
}

GL_API void GL_APIENTRY glPopMatrix(void)
{
	return es1::PopMatrix();
}

GL_API void GL_APIENTRY glPushMatrix(void)
{
	return es1::PushMatrix();
}

GL_API void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
	return es1::ReadPixels(x, y, width, height, format, type, pixels);
}

GL_API void GL_APIENTRY glRenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	return es1::RenderbufferStorageOES(target, internalformat, width, height);
}

GL_API void GL_APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	return es1::Rotatef(angle, x, y, z);
}

GL_API void GL_APIENTRY glRotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
	return es1::Rotatex(angle, x, y, z);
}

GL_API void GL_APIENTRY glSampleCoverage(GLclampf value, GLboolean invert)
{
	return es1::SampleCoverage(value, invert);
}

GL_API void GL_APIENTRY glSampleCoveragex(GLclampx value, GLboolean invert)
{
	return es1::SampleCoveragex(value, invert);
}

GL_API void GL_APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	return es1::Scalef(x, y, z);
}

GL_API void GL_APIENTRY glScalex(GLfixed x, GLfixed y, GLfixed z)
{
	return es1::Scalex(x, y, z);
}

GL_API void GL_APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	return es1::Scissor(x, y, width, height);
}

GL_API void GL_APIENTRY glShadeModel(GLenum mode)
{
	return es1::ShadeModel(mode);
}

GL_API void GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	return es1::StencilFunc(func, ref, mask);
}

GL_API void GL_APIENTRY glStencilMask(GLuint mask)
{
	return es1::StencilMask(mask);
}

GL_API void GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	return es1::StencilOp(fail, zfail, zpass);
}

GL_API void GL_APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	return es1::TexCoordPointer(size, type, stride, pointer);
}

GL_API void GL_APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	return es1::TexEnvf(target, pname, param);
}

GL_API void GL_APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
	return es1::TexEnvfv(target, pname, params);
}

GL_API void GL_APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
	return es1::TexEnvi(target, pname, param);
}

GL_API void GL_APIENTRY glTexEnvx(GLenum target, GLenum pname, GLfixed param)
{
	return es1::TexEnvx(target, pname, param);
}

GL_API void GL_APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
	return es1::TexEnviv(target, pname, params);
}

GL_API void GL_APIENTRY glTexEnvxv(GLenum target, GLenum pname, const GLfixed *params)
{
	return es1::TexEnvxv(target, pname, params);
}

GL_API void GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                                     GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	return es1::TexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

GL_API void GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	return es1::TexParameterf(target, pname, param);
}

GL_API void GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
	return es1::TexParameterfv(target, pname, params);
}

GL_API void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	return es1::TexParameteri(target, pname, param);
}

GL_API void GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
	return es1::TexParameteriv(target, pname, params);
}

GL_API void GL_APIENTRY glTexParameterx(GLenum target, GLenum pname, GLfixed param)
{
	return es1::TexParameterx(target, pname, param);
}

GL_API void GL_APIENTRY glTexParameterxv(GLenum target, GLenum pname, const GLfixed *params)
{
	return es1::TexParameterxv(target, pname, params);
}

GL_API void GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                        GLenum format, GLenum type, const GLvoid* pixels)
{
	return es1::TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

GL_API void GL_APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	return es1::Translatef(x, y, z);
}

GL_API void GL_APIENTRY glTranslatex(GLfixed x, GLfixed y, GLfixed z)
{
	return es1::Translatex(x, y, z);
}

GL_API void GL_APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	return es1::VertexPointer(size, type, stride, pointer);
}

GL_API void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	return es1::Viewport(x, y, width, height);
}

GL_API void GL_APIENTRY glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
	return es1::EGLImageTargetTexture2DOES(target, image);
}

GL_API void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image)
{
	return es1::EGLImageTargetRenderbufferStorageOES(target, image);
}

GL_API void GL_APIENTRY glDrawTexsOES(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height)
{
	return es1::DrawTexsOES(x,y, z, width, height);
}

GL_API void GL_APIENTRY glDrawTexiOES(GLint x, GLint y, GLint z, GLint width, GLint height)
{
	return es1::DrawTexiOES(x,y, z, width, height);
}

GL_API void GL_APIENTRY glDrawTexxOES(GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height)
{
	return es1::DrawTexxOES(x,y, z, width, height);
}

GL_API void GL_APIENTRY glDrawTexsvOES(const GLshort *coords)
{
	return es1::DrawTexsvOES(coords);
}

GL_API void GL_APIENTRY glDrawTexivOES(const GLint *coords)
{
	return es1::DrawTexivOES(coords);
}

GL_API void GL_APIENTRY glDrawTexxvOES(const GLfixed *coords)
{
	return es1::DrawTexxvOES(coords);
}

GL_API void GL_APIENTRY glDrawTexfOES(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height)
{
	return es1::DrawTexfOES(x, y, z, width, height);
}

GL_API void GL_APIENTRY glDrawTexfvOES(const GLfloat *coords)
{
	return es1::DrawTexfvOES(coords);
}

void GL_APIENTRY Register(const char *licenseKey)
{
	// Nothing to do, SwiftShader is open-source
}
}

LibGLES_CMexports::LibGLES_CMexports()
{
	this->glActiveTexture = es1::ActiveTexture;
	this->glAlphaFunc = es1::AlphaFunc;
	this->glAlphaFuncx = es1::AlphaFuncx;
	this->glBindBuffer = es1::BindBuffer;
	this->glBindFramebuffer = es1::BindFramebuffer;
	this->glBindFramebufferOES = es1::BindFramebufferOES;
	this->glBindRenderbufferOES = es1::BindRenderbufferOES;
	this->glBindTexture = es1::BindTexture;
	this->glBlendEquationSeparateOES = es1::BlendEquationSeparateOES;
	this->glBlendEquationOES = es1::BlendEquationOES;
	this->glBlendEquationSeparateOES = es1::BlendEquationSeparateOES;
	this->glBlendFuncSeparateOES = es1::BlendFuncSeparateOES;
	this->glBlendFunc = es1::BlendFunc;
	this->glBlendFuncSeparateOES = es1::BlendFuncSeparateOES;
	this->glBufferData = es1::BufferData;
	this->glBufferSubData = es1::BufferSubData;
	this->glCheckFramebufferStatusOES = es1::CheckFramebufferStatusOES;
	this->glClear = es1::Clear;
	this->glClearColor = es1::ClearColor;
	this->glClearColorx = es1::ClearColorx;
	this->glClearDepthf = es1::ClearDepthf;
	this->glClearDepthx = es1::ClearDepthx;
	this->glClearStencil = es1::ClearStencil;
	this->glClientActiveTexture = es1::ClientActiveTexture;
	this->glClipPlanef = es1::ClipPlanef;
	this->glClipPlanex = es1::ClipPlanex;
	this->glColor4f = es1::Color4f;
	this->glColor4ub = es1::Color4ub;
	this->glColor4x = es1::Color4x;
	this->glColorMask = es1::ColorMask;
	this->glColorPointer = es1::ColorPointer;
	this->glCompressedTexImage2D = es1::CompressedTexImage2D;
	this->glCompressedTexSubImage2D = es1::CompressedTexSubImage2D;
	this->glCopyTexImage2D = es1::CopyTexImage2D;
	this->glCopyTexSubImage2D = es1::CopyTexSubImage2D;
	this->glCullFace = es1::CullFace;
	this->glDeleteBuffers = es1::DeleteBuffers;
	this->glDeleteFramebuffersOES = es1::DeleteFramebuffersOES;
	this->glDeleteRenderbuffersOES = es1::DeleteRenderbuffersOES;
	this->glDeleteTextures = es1::DeleteTextures;
	this->glDepthFunc = es1::DepthFunc;
	this->glDepthMask = es1::DepthMask;
	this->glDepthRangex = es1::DepthRangex;
	this->glDepthRangef = es1::DepthRangef;
	this->glDisable = es1::Disable;
	this->glDisableClientState = es1::DisableClientState;
	this->glDrawArrays = es1::DrawArrays;
	this->glDrawElements = es1::DrawElements;
	this->glEnable = es1::Enable;
	this->glEnableClientState = es1::EnableClientState;
	this->glFinish = es1::Finish;
	this->glFlush = es1::Flush;
	this->glFramebufferRenderbufferOES = es1::FramebufferRenderbufferOES;
	this->glFramebufferTexture2DOES = es1::FramebufferTexture2DOES;
	this->glFogf = es1::Fogf;
	this->glFogfv = es1::Fogfv;
	this->glFogx = es1::Fogx;
	this->glFogxv = es1::Fogxv;
	this->glFrontFace = es1::FrontFace;
	this->glFrustumf = es1::Frustumf;
	this->glFrustumx = es1::Frustumx;
	this->glGenerateMipmapOES = es1::GenerateMipmapOES;
	this->glGenBuffers = es1::GenBuffers;
	this->glGenFramebuffersOES = es1::GenFramebuffersOES;
	this->glGenRenderbuffersOES = es1::GenRenderbuffersOES;
	this->glGenTextures = es1::GenTextures;
	this->glGetRenderbufferParameterivOES = es1::GetRenderbufferParameterivOES;
	this->glGetBooleanv = es1::GetBooleanv;
	this->glGetBufferParameteriv = es1::GetBufferParameteriv;
	this->glGetClipPlanef = es1::GetClipPlanef;
	this->glGetClipPlanex = es1::GetClipPlanex;
	this->glGetError = es1::GetError;
	this->glGetFixedv = es1::GetFixedv;
	this->glGetFloatv = es1::GetFloatv;
	this->glGetFramebufferAttachmentParameterivOES = es1::GetFramebufferAttachmentParameterivOES;
	this->glGetIntegerv = es1::GetIntegerv;
	this->glGetLightfv = es1::GetLightfv;
	this->glGetLightxv = es1::GetLightxv;
	this->glGetMaterialfv = es1::GetMaterialfv;
	this->glGetMaterialxv = es1::GetMaterialxv;
	this->glGetPointerv = es1::GetPointerv;
	this->glGetString = es1::GetString;
	this->glGetTexParameterfv = es1::GetTexParameterfv;
	this->glGetTexParameteriv = es1::GetTexParameteriv;
	this->glGetTexEnvfv = es1::GetTexEnvfv;
	this->glGetTexEnviv = es1::GetTexEnviv;
	this->glGetTexEnvxv = es1::GetTexEnvxv;
	this->glGetTexParameterxv = es1::GetTexParameterxv;
	this->glHint = es1::Hint;
	this->glIsBuffer = es1::IsBuffer;
	this->glIsEnabled = es1::IsEnabled;
	this->glIsFramebufferOES = es1::IsFramebufferOES;
	this->glIsTexture = es1::IsTexture;
	this->glIsRenderbufferOES = es1::IsRenderbufferOES;
	this->glLightModelf = es1::LightModelf;
	this->glLightModelfv = es1::LightModelfv;
	this->glLightModelx = es1::LightModelx;
	this->glLightModelxv = es1::LightModelxv;
	this->glLightf = es1::Lightf;
	this->glLightfv = es1::Lightfv;
	this->glLightx = es1::Lightx;
	this->glLightxv = es1::Lightxv;
	this->glLineWidth = es1::LineWidth;
	this->glLineWidthx = es1::LineWidthx;
	this->glLoadIdentity = es1::LoadIdentity;
	this->glLoadMatrixf = es1::LoadMatrixf;
	this->glLoadMatrixx = es1::LoadMatrixx;
	this->glLogicOp = es1::LogicOp;
	this->glMaterialf = es1::Materialf;
	this->glMaterialfv = es1::Materialfv;
	this->glMaterialx = es1::Materialx;
	this->glMaterialxv = es1::Materialxv;
	this->glMatrixMode = es1::MatrixMode;
	this->glMultMatrixf = es1::MultMatrixf;
	this->glMultMatrixx = es1::MultMatrixx;
	this->glMultiTexCoord4f = es1::MultiTexCoord4f;
	this->glMultiTexCoord4x = es1::MultiTexCoord4x;
	this->glNormal3f = es1::Normal3f;
	this->glNormal3x = es1::Normal3x;
	this->glNormalPointer = es1::NormalPointer;
	this->glOrthof = es1::Orthof;
	this->glOrthox = es1::Orthox;
	this->glPixelStorei = es1::PixelStorei;
	this->glPointParameterf = es1::PointParameterf;
	this->glPointParameterfv = es1::PointParameterfv;
	this->glPointParameterx = es1::PointParameterx;
	this->glPointParameterxv = es1::PointParameterxv;
	this->glPointSize = es1::PointSize;
	this->glPointSizePointerOES = es1::PointSizePointerOES;
	this->glPointSizex = es1::PointSizex;
	this->glPolygonOffset = es1::PolygonOffset;
	this->glPolygonOffsetx = es1::PolygonOffsetx;
	this->glPopMatrix = es1::PopMatrix;
	this->glPushMatrix = es1::PushMatrix;
	this->glReadPixels = es1::ReadPixels;
	this->glRenderbufferStorageOES = es1::RenderbufferStorageOES;
	this->glRotatef = es1::Rotatef;
	this->glRotatex = es1::Rotatex;
	this->glSampleCoverage = es1::SampleCoverage;
	this->glSampleCoveragex = es1::SampleCoveragex;
	this->glScalef = es1::Scalef;
	this->glScalex = es1::Scalex;
	this->glScissor = es1::Scissor;
	this->glShadeModel = es1::ShadeModel;
	this->glStencilFunc = es1::StencilFunc;
	this->glStencilMask = es1::StencilMask;
	this->glStencilOp = es1::StencilOp;
	this->glTexCoordPointer = es1::TexCoordPointer;
	this->glTexEnvf = es1::TexEnvf;
	this->glTexEnvfv = es1::TexEnvfv;
	this->glTexEnvi = es1::TexEnvi;
	this->glTexEnvx = es1::TexEnvx;
	this->glTexEnviv = es1::TexEnviv;
	this->glTexEnvxv = es1::TexEnvxv;
	this->glTexImage2D = es1::TexImage2D;
	this->glTexParameterf = es1::TexParameterf;
	this->glTexParameterfv = es1::TexParameterfv;
	this->glTexParameteri = es1::TexParameteri;
	this->glTexParameteriv = es1::TexParameteriv;
	this->glTexParameterx = es1::TexParameterx;
	this->glTexParameterxv = es1::TexParameterxv;
	this->glTexSubImage2D = es1::TexSubImage2D;
	this->glTranslatef = es1::Translatef;
	this->glTranslatex = es1::Translatex;
	this->glVertexPointer = es1::VertexPointer;
	this->glViewport = es1::Viewport;
	this->glEGLImageTargetTexture2DOES = es1::EGLImageTargetTexture2DOES;
	this->glEGLImageTargetRenderbufferStorageOES = es1::EGLImageTargetRenderbufferStorageOES;
	this->glDrawTexsOES = es1::DrawTexsOES;
	this->glDrawTexiOES = es1::DrawTexiOES;
	this->glDrawTexxOES = es1::DrawTexxOES;
	this->glDrawTexsvOES = es1::DrawTexsvOES;
	this->glDrawTexivOES = es1::DrawTexivOES;
	this->glDrawTexxvOES = es1::DrawTexxvOES;
	this->glDrawTexfOES = es1::DrawTexfOES;
	this->glDrawTexfvOES = es1::DrawTexfvOES;

	this->es1CreateContext = ::es1CreateContext;
	this->es1GetProcAddress = ::es1GetProcAddress;
	this->createBackBuffer = ::createBackBuffer;
	this->createDepthStencil = ::createDepthStencil;
	this->createFrameBuffer = ::createFrameBuffer;
}

extern "C" GL_API LibGLES_CMexports *libGLES_CM_swiftshader()
{
	static LibGLES_CMexports libGLES_CM;
	return &libGLES_CM;
}

LibEGL libEGL(getLibraryDirectoryFromSymbol((void*)libGLES_CM_swiftshader));
