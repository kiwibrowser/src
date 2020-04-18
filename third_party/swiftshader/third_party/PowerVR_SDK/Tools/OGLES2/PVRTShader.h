/*!****************************************************************************

 @file         OGLES2/PVRTShader.h
 @ingroup      API_OGLES2
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Shader handling for OpenGL ES 2.0

******************************************************************************/
#ifndef _PVRTSHADER_H_
#define _PVRTSHADER_H_

/*!
 @addtogroup API_OGLES2
 @{
*/

#include "PVRTContext.h"
#include "../PVRTString.h"
#include "../PVRTError.h"

/*!***************************************************************************
 @brief      	Loads a shader source code into memory and compiles it.
				It also pre-appends the array of defines that have been passed in
				to the source code before compilation.
 @param[in]		pszShaderCode		shader source code
 @param[in]		Type				type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 @param[out]	pObject				the resulting shader object
 @param[out]	pReturnError		the error message if it failed
 @param[in]		aszDefineArray		Array of defines to be pre-appended to shader string
 @param[in]		uiDefArraySize		Size of the define array
 @return		PVR_SUCCESS on success and PVR_FAIL on failure (also fills the str string)
*****************************************************************************/
EPVRTError PVRTShaderLoadSourceFromMemory(	const char* pszShaderCode,
											const GLenum Type,
											GLuint* const pObject,
											CPVRTString* const pReturnError,
											const char* const* aszDefineArray=0, GLuint uiDefArraySize=0);

/*!***************************************************************************
 @brief      	Takes a shader binary from memory and passes it to the GL.
 @param[in]		ShaderData		shader compiled binary data
 @param[in]		Size			size of shader binary data in bytes
 @param[in]		Type			type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 @param[in]		Format			shader binary format
 @param[out]	pObject			the resulting shader object
 @param[out]	pReturnError	the error message if it failed
 @return		PVR_SUCCESS on success and PVR_FAIL on failure (also fills the str string)
*****************************************************************************/
EPVRTError PVRTShaderLoadBinaryFromMemory(	const void*  const ShaderData,
											const size_t Size,
											const GLenum Type,
											const GLenum Format,
											GLuint*  const pObject,
											CPVRTString*  const pReturnError);

/*!***************************************************************************
 @brief      	Loads a shader file into memory and passes it to the GL. 
				It also passes defines that need to be pre-appended to the shader before compilation.
 @param[in]		pszBinFile			binary shader filename
 @param[in]		pszSrcFile			source shader filename
 @param[in]		Type				type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 @param[in]		Format				shader binary format, or 0 for source shader
 @param[out]	pObject				the resulting shader object
 @param[out]	pReturnError		the error message if it failed
 @param[in]		pContext			Context
 @param[in]		aszDefineArray		Array of defines to be pre-appended to shader string
 @param[in]		uiDefArraySize		Size of the define array
 @return		PVR_SUCCESS on success and PVR_FAIL on failure (also fills pReturnError)
*****************************************************************************/
EPVRTError PVRTShaderLoadFromFile(	const char* const pszBinFile,
									const char* const pszSrcFile,
									const GLenum Type,
									const GLenum Format,
									GLuint* const pObject,
									CPVRTString* const pReturnError,
									const SPVRTContext* const pContext=0,
									const char* const* aszDefineArray=0, GLuint uiDefArraySize=0);

/*!***************************************************************************
 @brief      	Links a shader program.
 @param[out]	pProgramObject			the created program object
 @param[in]		VertexShader			the vertex shader to link
 @param[in]		FragmentShader			the fragment shader to link
 @param[in]		pszAttribs				an array of attribute names
 @param[in]		i32NumAttribs			the number of attributes to bind
 @param[out]	pReturnError			the error message if it failed
 @return		PVR_SUCCESS on success, PVR_FAIL if failure
*****************************************************************************/
EPVRTError PVRTCreateProgram(	GLuint* const pProgramObject,
								const GLuint VertexShader,
								const GLuint FragmentShader,
								const char** const pszAttribs,
								const int i32NumAttribs,
								CPVRTString* const pReturnError);

/*! @} */

#endif

/*****************************************************************************
 End of file (PVRTShader.h)
*****************************************************************************/

