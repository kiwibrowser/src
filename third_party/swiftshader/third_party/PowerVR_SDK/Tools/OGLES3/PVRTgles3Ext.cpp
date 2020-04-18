/******************************************************************************

 @File         OGLES3/PVRTgles3Ext.cpp

 @Title        OGLES3/PVRTgles3Ext

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     Independent

 @Description  OpenGL ES 3.0 extensions

******************************************************************************/
#include <string.h>

#include "PVRTContext.h"
#include "PVRTgles3Ext.h"

/****************************************************************************
** Local code
****************************************************************************/

/****************************************************************************
** Class: CPVRTgles3Ext
****************************************************************************/

/*!***************************************************************************
 @Function			LoadExtensions
 @Description		Initialises IMG extensions
*****************************************************************************/
void CPVRTgles3Ext::LoadExtensions()
{

    glRenderbufferStorageMultisampleIMG = 0;
    glFramebufferTexture2DMultisampleIMG = 0;
    glRenderbufferStorageMultisampleEXT = 0;
    glFramebufferTexture2DMultisampleEXT = 0;
    
    // Supported extensions provide new entry points for OpenGL ES 3.0.
    
    const GLubyte *pszGLExtensions;
    
    /* Retrieve GL extension string */
    pszGLExtensions = glGetString(GL_EXTENSIONS);
    
#if !defined(TARGET_OS_IPHONE)
 /* GL_IMG_multisampled_render_to_texture */
    if (strstr((char *)pszGLExtensions, "GL_IMG_multisampled_render_to_texture"))
    {
        glRenderbufferStorageMultisampleIMG = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG) PVRGetProcAddress(glRenderbufferStorageMultisampleIMG);
        glFramebufferTexture2DMultisampleIMG = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG) PVRGetProcAddress(glFramebufferTexture2DMultisampleIMG);
    }
    
    /* GL_EXT_multisampled_render_to_texture */
    if (strstr((char *)pszGLExtensions, "GL_EXT_multisampled_render_to_texture"))
    {
        glRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXT) PVRGetProcAddress(glRenderbufferStorageMultisampleEXT);
        glFramebufferTexture2DMultisampleEXT = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT) PVRGetProcAddress(glFramebufferTexture2DMultisampleEXT);
    }
#endif
}

/*!***********************************************************************
@Function			IsGLExtensionSupported
@Input				extension extension to query for
@Returns			True if the extension is supported
@Description		Queries for support of an extension
*************************************************************************/
bool CPVRTgles3Ext::IsGLExtensionSupported(const char * const extension)
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

