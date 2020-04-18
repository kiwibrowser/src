{
global:
	# EGL core functions
	eglBindAPI;
	eglBindTexImage;
	eglChooseConfig;
	eglCopyBuffers;
	eglCreateContext;
	eglCreatePbufferFromClientBuffer;
	eglCreatePbufferSurface;
	eglCreatePixmapSurface;
	eglCreateWindowSurface;
	eglDestroyContext;
	eglDestroySurface;
	eglGetConfigAttrib;
	eglGetConfigs;
	eglGetCurrentContext;
	eglGetCurrentDisplay;
	eglGetCurrentSurface;
	eglGetDisplay;
	eglGetError;
	eglGetProcAddress;
	eglInitialize;
	eglMakeCurrent;
	eglQueryAPI;
	eglQueryContext;
	eglQueryString;
	eglQuerySurface;
	eglReleaseTexImage;
	eglReleaseThread;
	eglSurfaceAttrib;
	eglSwapBuffers;
	eglSwapInterval;
	eglTerminate;
	eglWaitClient;
	eglWaitGL;
	eglWaitNative;

	# Extensions
	eglCreateImageKHR;
	eglDestroyImageKHR;
	eglGetPlatformDisplayEXT;
	eglCreatePlatformWindowSurfaceEXT;
	eglCreatePlatformPixmapSurfaceEXT;
	eglCreateSyncKHR;
	eglDestroySyncKHR;
	eglClientWaitSyncKHR;
	eglGetSyncAttribKHR;

	# Table of function pointers to disambiguate between libraries
	libEGL_swiftshader;

	# Type-strings and type-infos required by sanitizers
	_ZTS*;
	_ZTI*;

local:
	*;
};