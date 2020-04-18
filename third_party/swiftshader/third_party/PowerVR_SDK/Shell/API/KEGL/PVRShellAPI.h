/*!****************************************************************************

 @file         KEGL/PVRShellAPI.h
 @ingroup      API_KEGL 
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        3D API context management for KEGL.
 @details      Makes programming for 3D APIs easier by wrapping surface
               initialization, Texture allocation and other functions for use by a demo.

******************************************************************************/

#ifndef __PVRSHELLAPI_H_
#define __PVRSHELLAPI_H_

/****************************************************************************
** 3D API header files
****************************************************************************/
#if defined(BUILD_OGLES2)
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
#elif defined(BUILD_OGLES3)
	#include <GLES3/gl3.h>
	#include <GLES2/gl2ext.h>
	#include <GLES3/gl3ext.h>
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
#elif defined(BUILD_OGL)
#define SUPPORT_OPENGL
#if defined(_WIN32)
	#include <windows.h>
#endif
	#include <GL/gl.h>
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
#else
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#include <GLES/gl.h>
	#include <GLES/glext.h>
	#include <GLES/glplatform.h>
#endif

/*!***************************************************************************
 @addtogroup API_KEGL 
 @brief      KEGL API
 @{
****************************************************************************/

/*!***************************************************************************
 @class PVRShellInitAPI
 @brief Initialisation interface with specific API.
****************************************************************************/
class PVRShellInitAPI
{
public:
	EGLDisplay	m_EGLDisplay;
	EGLSurface	m_EGLWindow;
	EGLContext	m_EGLContext;
	EGLConfig	m_EGLConfig;
	EGLint		m_MajorVersion, m_MinorVersion;
	bool		m_bPowerManagementSupported;
	EGLint		m_iRequestedConfig;
	EGLint		m_iConfig;

	EGLNativeDisplayType m_NDT;
	EGLNativePixmapType  m_NPT;
	EGLNativeWindowType  m_NWT;


public:
	PVRShellInitAPI() : m_bPowerManagementSupported(false), m_iRequestedConfig(0), m_iConfig(0) {}
	EGLConfig SelectEGLConfiguration(const PVRShellData * const pData);
	const char *StringFrom_eglGetError() const;

#if defined(BUILD_OGLES) || defined(BUILD_OGLES2)
protected:
	typedef void (GL_APIENTRY * PFNGLDISCARDFRAMEBUFFEREXT)(GLenum target, GLsizei numAttachments, const GLenum *attachments);
	PFNGLDISCARDFRAMEBUFFEREXT			glDiscardFramebufferEXT;
#endif

};

/*! @} */

#endif // __PVRSHELLAPI_H_

/*****************************************************************************
 End of file (PVRShellAPI.h)
*****************************************************************************/

