/******************************************************************************

 @File         OGLES2/PVRTgles2Ext.cpp

 @Title        OGLES2/PVRTgles2Ext

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     Independent

 @Description  OpenGL ES 2.0 extensions

******************************************************************************/
#include <string.h>

#include "PVRTContext.h"
#include "PVRTgles2Ext.h"

/****************************************************************************
** Local code
****************************************************************************/

/****************************************************************************
** Class: CPVRTgles2Ext
****************************************************************************/

/*!***************************************************************************
 @Function			LoadExtensions
 @Description		Initialises IMG extensions
*****************************************************************************/
void CPVRTgles2Ext::LoadExtensions()
{
	glMultiDrawElementsEXT = 0;
	glMultiDrawArraysEXT = 0;
	glMapBufferOES = 0;
	glUnmapBufferOES = 0;
	glGetBufferPointervOES = 0;
	glDiscardFramebufferEXT = 0;
	glBindVertexArrayOES = 0;
	glDeleteVertexArraysOES = 0;
	glGenVertexArraysOES = 0;
	glIsVertexArrayOES = 0;
	glRenderbufferStorageMultisampleIMG = 0;
	glFramebufferTexture2DMultisampleIMG = 0;
	glGenQueriesEXT = 0;
	glDeleteQueriesEXT = 0;
	glIsQueryEXT = 0;
	glBeginQueryEXT = 0;
	glEndQueryEXT = 0;
	glGetQueryivEXT = 0;
	glGetQueryObjectuivEXT = 0;
	glRenderbufferStorageMultisampleANGLE = 0;
	glBlitFramebufferNV = 0;
	glTexImage3DOES = 0;
	glTexSubImage3DOES = 0;
	glCopyTexSubImage3DOES = 0;
	glCompressedTexImage3DOES = 0;
	glCompressedTexSubImage3DOES = 0;
	glFramebufferTexture3DOES = 0;
	glDrawBuffersEXT = 0;

	// Supported extensions provide new entry points for OpenGL ES 2.0.

	const GLubyte *pszGLExtensions;

	/* Retrieve GL extension string */
    pszGLExtensions = glGetString(GL_EXTENSIONS);

#if !defined(TARGET_OS_IPHONE)
	/* GL_EXT_multi_draw_arrays */
	if (strstr((char *)pszGLExtensions, "GL_EXT_multi_draw_arrays"))
	{
		glMultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTS) PVRGetProcAddress(glMultiDrawElementsEXT);
		glMultiDrawArraysEXT = (PFNGLMULTIDRAWARRAYS) PVRGetProcAddress(glMultiDrawArraysEXT);
	}

	/* GL_EXT_multi_draw_arrays */
	if (strstr((char *)pszGLExtensions, "GL_OES_mapbuffer"))
	{
        glMapBufferOES = (PFNGLMAPBUFFEROES) PVRGetProcAddress(glMapBufferOES);
        glUnmapBufferOES = (PFNGLUNMAPBUFFEROES) PVRGetProcAddress(glUnmapBufferOES);
        glGetBufferPointervOES = (PFNGLGETBUFFERPOINTERVOES) PVRGetProcAddress(glGetBufferPointervOES);
	}

	/* GL_OES_vertex_array_object */
	if (strstr((char *)pszGLExtensions, "GL_OES_vertex_array_object"))
	{
        glBindVertexArrayOES = (PFNGLBINDVERTEXARRAYOES) PVRGetProcAddress(glBindVertexArrayOES);
        glDeleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOES) PVRGetProcAddress(glDeleteVertexArraysOES);
        glGenVertexArraysOES = (PFNGLGENVERTEXARRAYSOES) PVRGetProcAddress(glGenVertexArraysOES);
		glIsVertexArrayOES = (PFNGLISVERTEXARRAYOES) PVRGetProcAddress(glIsVertexArrayOES);
	}

	/* GL_IMG_multisampled_render_to_texture */
	if (strstr((char *)pszGLExtensions, "GL_IMG_multisampled_render_to_texture"))
	{
		glRenderbufferStorageMultisampleIMG = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG) PVRGetProcAddress(glRenderbufferStorageMultisampleIMG);
		glFramebufferTexture2DMultisampleIMG = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG) PVRGetProcAddress(glFramebufferTexture2DMultisampleIMG);
	}
	
	/* GL_ANGLE_framebuffer_multisample */
	if (strstr((char *)pszGLExtensions, "GL_ANGLE_framebuffer_multisample"))
	{
		glRenderbufferStorageMultisampleANGLE = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLEPROC)PVRGetProcAddress(glRenderbufferStorageMultisampleANGLE);
	}

	/* GL_NV_framebuffer_blit */
	if (strstr((char *)pszGLExtensions, "GL_NV_framebuffer_blit"))
	{
		glBlitFramebufferNV = (PFNGLBLITFRAMEBUFFERNVPROC) PVRGetProcAddress(glBlitFramebufferNV);
	}

	/* GL_OES_texture_3D */
	if(strstr((char *)pszGLExtensions, "GL_OES_texture_3D"))
	{
		glTexImage3DOES = (PFNGLTEXIMAGE3DOES) PVRGetProcAddress(glTexImage3DOES);
		glTexSubImage3DOES = (PFNGLTEXSUBIMAGE3DOES) PVRGetProcAddress(glTexSubImage3DOES);
		glCopyTexSubImage3DOES = (PFNGLCOPYTEXSUBIMAGE3DOES) PVRGetProcAddress(glCopyTexSubImage3DOES);
		glCompressedTexImage3DOES = (PFNGLCOMPRESSEDTEXIMAGE3DOES) PVRGetProcAddress(glCompressedTexImage3DOES);
		glCompressedTexSubImage3DOES = (PFNGLCOMPRESSEDTEXSUBIMAGE3DOES) PVRGetProcAddress(glCompressedTexSubImage3DOES);
		glFramebufferTexture3DOES = (PFNGLFRAMEBUFFERTEXTURE3DOES) PVRGetProcAddress(glFramebufferTexture3DOES);
	}

	/* GL_EXT_draw_buffers */
	if (strstr((char *)pszGLExtensions, "GL_EXT_draw_buffers"))
	{
		glDrawBuffersEXT = (PFNGLDRAWBUFFERSEXT) PVRGetProcAddress(glDrawBuffersEXT);
	}
#endif

#if defined(GL_EXT_discard_framebuffer)
	/* GL_EXT_discard_framebuffer */
	if (strstr((char *)pszGLExtensions, "GL_EXT_discard_framebuffer"))
	{
        glDiscardFramebufferEXT = (PFNGLDISCARDFRAMEBUFFEREXT) PVRGetProcAddress(glDiscardFramebufferEXT);
	}
#endif

	/* GL_EXT_occlusion_query_boolean */
	if (strstr((char *)pszGLExtensions, "GL_EXT_occlusion_query_boolean"))
	{
		glGenQueriesEXT = (PFNGLGENQUERIESEXT) PVRGetProcAddress(glGenQueriesEXT);
		glDeleteQueriesEXT = (PFNGLDELETEQUERIESEXT) PVRGetProcAddress(glDeleteQueriesEXT);
		glIsQueryEXT = (PFNGLISQUERYEXT) PVRGetProcAddress(glIsQueryEXT);
		glBeginQueryEXT = (PFNGLBEGINQUERYEXT) PVRGetProcAddress(glBeginQueryEXT);
		glEndQueryEXT = (PFNGLENDQUERYEXT) PVRGetProcAddress(glEndQueryEXT);
		glGetQueryivEXT = (PFNGLGETQUERYIVEXT) PVRGetProcAddress(glGetQueryivEXT);
		glGetQueryObjectuivEXT = (PFNGLGETQUERYOBJECTUIVEXT) PVRGetProcAddress(glGetQueryObjectuivEXT);
	}
}

/*!***********************************************************************
@Function			IsGLExtensionSupported
@Input				extension extension to query for
@Returns			True if the extension is supported
@Description		Queries for support of an extension
*************************************************************************/
bool CPVRTgles2Ext::IsGLExtensionSupported(const char * const extension)
{
	// The recommended technique for querying OpenGL extensions;
	// from http://opengl.org/resources/features/OGLextensions/
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	/* Extension names should not have spaces. */
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	extensions = glGetString(GL_EXTENSIONS);

	/* It takes a bit of care to be fool-proof about parsing the
	OpenGL extensions string. Don't be fooled by sub-strings, etc. */
	start = extensions;
	for (;;) {
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return true;
		start = terminator;
	}

	return false;
}

/*****************************************************************************
 End of file (PVRTglesExt.cpp)
*****************************************************************************/

