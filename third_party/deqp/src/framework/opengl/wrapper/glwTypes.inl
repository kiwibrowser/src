/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL Utilities
 * ---------------------------------------------
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
 * \brief OpenGL wrapper base types.
 *
 * \note Unlike most .inl files this one is NOT auto-generated. This is .inl
 *		 because it is included from both glwDefs.hpp (inside glw namespace)
 *		 and glw.h (in root namespace, as C source).
 *//*--------------------------------------------------------------------*/

/* Calling convention. */
#if (DE_OS == DE_OS_ANDROID)
#	include <sys/cdefs.h>
#	if !defined(__NDK_FPABI__)
#		define __NDK_FPABI__
#	endif
#	define GLW_APICALL __NDK_FPABI__
#else
#	define GLW_APICALL
#endif

#if (DE_OS == DE_OS_WIN32)
#	define GLW_APIENTRY __stdcall
#else
#	define GLW_APIENTRY
#endif

/* Signed basic types. */
typedef deInt8				GLbyte;
typedef deInt16				GLshort;
typedef deInt32				GLint;
typedef deInt64				GLint64;

/* Unsigned basic types. */
typedef deUint8				GLubyte;
typedef deUint16			GLushort;
typedef deUint32			GLuint;
typedef deUint64			GLuint64;

/* Floating-point types. */
typedef deUint16			GLhalf;
typedef float				GLfloat;
typedef float				GLclampf;
typedef double				GLdouble;
typedef double				GLclampd;

/* Special types. */
typedef char				GLchar;
typedef deUint8				GLboolean;
typedef deUint32			GLenum;
typedef deUint32			GLbitfield;
typedef deInt32				GLsizei;
typedef deInt32				GLfixed;
typedef void				GLvoid;

#if (DE_OS == DE_OS_WIN32 && DE_CPU == DE_CPU_X86_64)
	typedef signed long long int	GLintptr;
	typedef signed long long int	GLsizeiptr;
#else
	typedef signed long int			GLintptr;
	typedef signed long int			GLsizeiptr;
#endif

/* Opaque handles. */
typedef struct __GLsync*	GLsync;
typedef void*				GLeglImageOES;

/* Callback for GL_ARB_debug_output. */
typedef void (GLW_APIENTRY* GLDEBUGPROC) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const GLvoid *userParam);

/* OES_EGL_image */
typedef void*				GLeglImageOES;
