/*!****************************************************************************

 @file         OGLES2/PVRTgles2Ext.h
 @ingroup      API_OGLES2
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        OpenGL ES 2.0 extensions

******************************************************************************/

#ifndef _PVRTGLES2EXT_H_
#define _PVRTGLES2EXT_H_


/*!
 @addtogroup   API_OGLES2
 @{
*/

#ifdef __APPLE__
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE==1
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
// No binary shaders are allowed on the iphone and so this value is not defined
// Defining here allows for a more graceful fail of binary shader loading at runtime
// which can be recovered from instead of fail at compile time
#define GL_SGX_BINARY_IMG 0
#else
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extimg.h>
#endif
#else
#if !defined(EGL_NOT_PRESENT)
#include <EGL/egl.h>
#endif
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extimg.h>
#endif

#if defined(TARGET_OS_IPHONE)
// the extensions supported on the iPhone are treated as core functions of gl
// so use this macro to assign the function pointers in this class appropriately.
#define PVRGetProcAddress(x) ::x
#else

#if defined(EGL_NOT_PRESENT)

#if defined(__PALMPDK__)
#include "SDL.h"

#define PVRGetProcAddress(x) SDL_GLES_GetProcAddress(#x)
#else
#define PVRGetProcAddress(x) NULL
#endif

#else
#define PVRGetProcAddress(x) eglGetProcAddress(#x)
#endif

#endif

/****************************************************************************
** Build options
****************************************************************************/

#define GL_PVRTGLESEXT_VERSION 2

/**************************************************************************
****************************** GL EXTENSIONS ******************************
**************************************************************************/

/*!************************************************************************
 @class CPVRTgles2Ext
 @brief A class for initialising and managing OGLES2 extensions
**************************************************************************/
class CPVRTgles2Ext
{

public:
    // Type definitions for pointers to functions returned by eglGetProcAddress
    typedef void (GL_APIENTRY *PFNGLMULTIDRAWELEMENTS) (GLenum mode, GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount); // glvoid
    typedef void* (GL_APIENTRY *PFNGLMAPBUFFEROES)(GLenum target, GLenum access);
    typedef GLboolean (GL_APIENTRY *PFNGLUNMAPBUFFEROES)(GLenum target);
    typedef void (GL_APIENTRY *PFNGLGETBUFFERPOINTERVOES)(GLenum target, GLenum pname, void** params);
	typedef void (GL_APIENTRY * PFNGLMULTIDRAWARRAYS) (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount); // glvoid
	typedef void (GL_APIENTRY * PFNGLDISCARDFRAMEBUFFEREXT)(GLenum target, GLsizei numAttachments, const GLenum *attachments);

	typedef void (GL_APIENTRY *PFNGLGENQUERIESEXT) (GLsizei n, GLuint *ids);
	typedef void (GL_APIENTRY *PFNGLDELETEQUERIESEXT) (GLsizei n, const GLuint *ids);
	typedef GLboolean (GL_APIENTRY *PFNGLISQUERYEXT) (GLuint id);
	typedef void (GL_APIENTRY *PFNGLBEGINQUERYEXT) (GLenum target, GLuint id);
	typedef void (GL_APIENTRY *PFNGLENDQUERYEXT) (GLenum target);
	typedef void (GL_APIENTRY *PFNGLGETQUERYIVEXT) (GLenum target, GLenum pname, GLint *params);
	typedef void (GL_APIENTRY *PFNGLGETQUERYOBJECTUIVEXT) (GLuint id, GLenum pname, GLuint *params);

	typedef void (GL_APIENTRYP PFNGLBINDVERTEXARRAYOES) (GLuint vertexarray);
	typedef void (GL_APIENTRYP PFNGLDELETEVERTEXARRAYSOES) (GLsizei n, const GLuint *vertexarrays);
	typedef void (GL_APIENTRYP PFNGLGENVERTEXARRAYSOES) (GLsizei n, GLuint *vertexarrays);
	typedef GLboolean (GL_APIENTRYP PFNGLISVERTEXARRAYOES) (GLuint vertexarray);

	typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
	
	typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLEPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

	typedef void (GL_APIENTRYP PFNGLBLITFRAMEBUFFERNVPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

	typedef void (GL_APIENTRYP PFNGLTEXIMAGE3DOES) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void (GL_APIENTRYP PFNGLTEXSUBIMAGE3DOES) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void (GL_APIENTRYP PFNGLCOPYTEXSUBIMAGE3DOES) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void (GL_APIENTRYP PFNGLCOMPRESSEDTEXIMAGE3DOES) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void (GL_APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE3DOES) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTURE3DOES) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
	
	typedef void (GL_APIENTRYP PFNGLDRAWBUFFERSEXT) (GLsizei n, const GLenum *bufs);

	// GL_EXT_multi_draw_arrays
	PFNGLMULTIDRAWELEMENTS				glMultiDrawElementsEXT;
	PFNGLMULTIDRAWARRAYS				glMultiDrawArraysEXT;

	// GL_EXT_multi_draw_arrays
    PFNGLMAPBUFFEROES                   glMapBufferOES;
    PFNGLUNMAPBUFFEROES                 glUnmapBufferOES;
    PFNGLGETBUFFERPOINTERVOES           glGetBufferPointervOES;

	// GL_EXT_discard_framebuffer
	PFNGLDISCARDFRAMEBUFFEREXT			glDiscardFramebufferEXT;

	// GL_EXT_occlusion_query_boolean
#if !defined(GL_EXT_occlusion_query_boolean)
	#define GL_ANY_SAMPLES_PASSED_EXT                               0x8C2F
	#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT                  0x8D6A
	#define GL_CURRENT_QUERY_EXT                                    0x8865
	#define GL_QUERY_RESULT_EXT                                     0x8866
	#define GL_QUERY_RESULT_AVAILABLE_EXT                           0x886
#endif
	PFNGLGENQUERIESEXT                  glGenQueriesEXT;
	PFNGLDELETEQUERIESEXT               glDeleteQueriesEXT;
	PFNGLISQUERYEXT                     glIsQueryEXT;
	PFNGLBEGINQUERYEXT                  glBeginQueryEXT;
	PFNGLENDQUERYEXT                    glEndQueryEXT;
	PFNGLGETQUERYIVEXT                  glGetQueryivEXT;
	PFNGLGETQUERYOBJECTUIVEXT           glGetQueryObjectuivEXT;

	// GL_OES_vertex_array_object
#if !defined(GL_OES_vertex_array_object)
	#define GL_VERTEX_ARRAY_BINDING_OES 0x85B5
#endif

	PFNGLBINDVERTEXARRAYOES glBindVertexArrayOES;
	PFNGLDELETEVERTEXARRAYSOES glDeleteVertexArraysOES;
	PFNGLGENVERTEXARRAYSOES glGenVertexArraysOES;
	PFNGLISVERTEXARRAYOES glIsVertexArrayOES;

	// GL_IMG_multisampled_render_to_texture
#if !defined(GL_IMG_multisampled_render_to_texture)
	#define GL_RENDERBUFFER_SAMPLES_IMG                 0x9133
	#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_IMG   0x9134
	#define GL_MAX_SAMPLES_IMG                          0x9135
	#define GL_TEXTURE_SAMPLES_IMG                      0x9136
#endif
	
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG glRenderbufferStorageMultisampleIMG;
	PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG glFramebufferTexture2DMultisampleIMG;

	// GL_EXT_multisampled_render_to_texture
#if !defined(GL_ANGLE_framebuffer_multisample)
	#define GL_RENDERBUFFER_SAMPLES_ANGLE               0x8CAB
	#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE 0x8D56
	#define GL_MAX_SAMPLES_ANGLE                        0x8D57
#endif

	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLEPROC glRenderbufferStorageMultisampleANGLE;

	// GL_NV_framebuffer_blit
#if !defined(GL_NV_framebuffer_blit)
	#define GL_READ_FRAMEBUFFER_NV            0x8CA8
	#define GL_DRAW_FRAMEBUFFER_NV            0x8CA9
	#define GL_DRAW_FRAMEBUFFER_BINDING_NV    0x8CA6
	#define GL_READ_FRAMEBUFFER_BINDING_NV    0x8CAA
#endif

	PFNGLBLITFRAMEBUFFERNVPROC glBlitFramebufferNV;

	// GL_OES_texture_3D
#if !defined(GL_OES_texture_3D)
	#define GL_TEXTURE_WRAP_R_OES                                   0x8072
	#define GL_TEXTURE_3D_OES                                       0x806F
	#define GL_TEXTURE_BINDING_3D_OES                               0x806A
	#define GL_MAX_3D_TEXTURE_SIZE_OES                              0x8073
	#define GL_SAMPLER_3D_OES                                       0x8B5F
	#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_OES        0x8CD4
#endif

	PFNGLTEXIMAGE3DOES glTexImage3DOES;
	PFNGLTEXSUBIMAGE3DOES glTexSubImage3DOES;
	PFNGLCOPYTEXSUBIMAGE3DOES glCopyTexSubImage3DOES;
	PFNGLCOMPRESSEDTEXIMAGE3DOES glCompressedTexImage3DOES;
	PFNGLCOMPRESSEDTEXSUBIMAGE3DOES glCompressedTexSubImage3DOES;
	PFNGLFRAMEBUFFERTEXTURE3DOES glFramebufferTexture3DOES;

	// GL_EXT_draw_buffers
#if !defined(GL_EXT_draw_buffers)
	#define GL_MAX_COLOR_ATTACHMENTS_EXT                            0x8CDF
	#define GL_MAX_DRAW_BUFFERS_EXT                                 0x8824
	#define GL_DRAW_BUFFER0_EXT                                     0x8825
	#define GL_DRAW_BUFFER1_EXT                                     0x8826
	#define GL_DRAW_BUFFER2_EXT                                     0x8827
	#define GL_DRAW_BUFFER3_EXT                                     0x8828
	#define GL_DRAW_BUFFER4_EXT                                     0x8829
	#define GL_DRAW_BUFFER5_EXT                                     0x882A
	#define GL_DRAW_BUFFER6_EXT                                     0x882B
	#define GL_DRAW_BUFFER7_EXT                                     0x882C
	#define GL_DRAW_BUFFER8_EXT                                     0x882D
	#define GL_DRAW_BUFFER9_EXT                                     0x882E
	#define GL_DRAW_BUFFER10_EXT                                    0x882F
	#define GL_DRAW_BUFFER11_EXT                                    0x8830
	#define GL_DRAW_BUFFER12_EXT                                    0x8831
	#define GL_DRAW_BUFFER13_EXT                                    0x8832
	#define GL_DRAW_BUFFER14_EXT                                    0x8833
	#define GL_DRAW_BUFFER15_EXT                                    0x8834
	#define GL_COLOR_ATTACHMENT0_EXT                                0x8CE0
	#define GL_COLOR_ATTACHMENT1_EXT                                0x8CE1
	#define GL_COLOR_ATTACHMENT2_EXT                                0x8CE2
	#define GL_COLOR_ATTACHMENT3_EXT                                0x8CE3
	#define GL_COLOR_ATTACHMENT4_EXT                                0x8CE4
	#define GL_COLOR_ATTACHMENT5_EXT                                0x8CE5
	#define GL_COLOR_ATTACHMENT6_EXT                                0x8CE6
	#define GL_COLOR_ATTACHMENT7_EXT                                0x8CE7
	#define GL_COLOR_ATTACHMENT8_EXT                                0x8CE8
	#define GL_COLOR_ATTACHMENT9_EXT                                0x8CE9
	#define GL_COLOR_ATTACHMENT10_EXT                               0x8CEA
	#define GL_COLOR_ATTACHMENT11_EXT                               0x8CEB
	#define GL_COLOR_ATTACHMENT12_EXT                               0x8CEC
	#define GL_COLOR_ATTACHMENT13_EXT                               0x8CED
	#define GL_COLOR_ATTACHMENT14_EXT                               0x8CEE
	#define GL_COLOR_ATTACHMENT15_EXT                               0x8CEF
#endif

	PFNGLDRAWBUFFERSEXT                 glDrawBuffersEXT;

public:
	/*!***********************************************************************
	@brief      		Initialises IMG extensions
	*************************************************************************/
	void LoadExtensions();

	/*!***********************************************************************
	@brief		    Queries for support of an extension
	@param[in]		extension extension to query for
	@return			True if the extension is supported
	*************************************************************************/
	static bool IsGLExtensionSupported(const char * const extension);
};

/*! @} */

#endif /* _PVRTGLES2EXT_H_ */

/*****************************************************************************
 End of file (PVRTgles2Ext.h)
*****************************************************************************/

