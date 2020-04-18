/******************************************************************************

 @File         OGLES2/PVRTShader.cpp

 @Title        OGLES2/PVRTShader

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Shader handling for OpenGL ES 2.0

******************************************************************************/

#include "PVRTString.h"
#include "PVRTShader.h"
#include "PVRTResourceFile.h"
#include "PVRTGlobal.h"
#include <ctype.h>
#include <string.h>

/*!***************************************************************************
 @Function		PVRTShaderLoadSourceFromMemory
 @Input			pszShaderCode		shader source code
 @Input			Type				type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 @Output		pObject				the resulting shader object
 @Output		pReturnError		the error message if it failed
 @Input			aszDefineArray		Array of defines to be pre-appended to shader string
 @Input			uiDefArraySize		Size of the define array
 @Return		PVR_SUCCESS on success and PVR_FAIL on failure (also fills the str string)
 @Description	Loads a shader source code into memory and compiles it.
				It also pre-appends the array of defines that have been passed in
				to the source code before compilation.
*****************************************************************************/
EPVRTError PVRTShaderLoadSourceFromMemory(	const char* pszShaderCode,
											const GLenum Type,
											GLuint* const pObject,
											CPVRTString* const pReturnError,
											const char* const* aszDefineArray, GLuint uiDefArraySize)
{
	// Append define's here if there are any
	CPVRTString pszShaderString;

	if(uiDefArraySize > 0)
	{
		while(isspace(*pszShaderCode))
			++pszShaderCode;

		if(*pszShaderCode == '#')
		{
			const char* tmp = pszShaderCode + 1;

			while(isspace(*tmp))
				++tmp;

			if(strncmp(tmp, "version", 7) == 0)
			{
				const char* c = strchr(pszShaderCode, '\n');

				if(c)
				{
					size_t length = c - pszShaderCode + 1;
					pszShaderString = CPVRTString(pszShaderCode, length);
					pszShaderCode += length;
				}
				else
				{
					pszShaderString = CPVRTString(pszShaderCode) + "\n";
					pszShaderCode = '\0';
				}
			}
		}

		for(GLuint i = 0 ; i < uiDefArraySize; ++i)
		{
			pszShaderString += "#define ";
			pszShaderString += aszDefineArray[i];
			pszShaderString += "\n";
		}
	}

	// Append the shader code to the string
	pszShaderString += pszShaderCode;

	/* Create and compile the shader object */
    *pObject = glCreateShader(Type);
	const char* pszString(pszShaderString.c_str());
	glShaderSource(*pObject, 1, &pszString, NULL);
    glCompileShader(*pObject);

	/* Test if compilation succeeded */
	GLint ShaderCompiled;
    glGetShaderiv(*pObject, GL_COMPILE_STATUS, &ShaderCompiled);
	if (!ShaderCompiled)
	{
		int i32InfoLogLength, i32CharsWritten;
		glGetShaderiv(*pObject, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
        glGetShaderInfoLog(*pObject, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		*pReturnError = CPVRTString("Failed to compile shader: ") + pszInfoLog + "\n";
		delete [] pszInfoLog;
		glDeleteShader(*pObject);
		return PVR_FAIL;
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function		PVRTShaderLoadBinaryFromMemory
 @Input			ShaderData		shader compiled binary data
 @Input			Size			size of shader binary data in bytes
 @Input			Type			type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 @Input			Format			shader binary format
 @Output		pObject			the resulting shader object
 @Output		pReturnError	the error message if it failed
 @Return		PVR_SUCCESS on success and PVR_FAIL on failure (also fills the str string)
 @Description	Takes a shader binary from memory and passes it to the GL.
*****************************************************************************/
EPVRTError PVRTShaderLoadBinaryFromMemory(	const void* const ShaderData,
											const size_t Size,
											const GLenum Type,
											const GLenum Format,
											GLuint* const pObject,
											CPVRTString* const pReturnError)
{
	/* Create and compile the shader object */
    *pObject = glCreateShader(Type);

    // Get the list of supported binary formats
    // and if (more then 0) find given Format among them
    GLint numFormats = 0;
    GLint *listFormats;
    int i;
    glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS,&numFormats);
    if(numFormats != 0) {
        listFormats = new GLint[numFormats];
        for(i=0;i<numFormats;++i)
            listFormats[i] = 0;
        glGetIntegerv(GL_SHADER_BINARY_FORMATS,listFormats);
        for(i=0;i<numFormats;++i) {
            if(listFormats[i] == (int) Format) {
                glShaderBinary(1, pObject, Format, ShaderData, (GLint)Size);
                if (glGetError() != GL_NO_ERROR)
                {
                    *pReturnError = CPVRTString("Failed to load binary shader\n");
                    glDeleteShader(*pObject);
                    return PVR_FAIL;
                }
                return PVR_SUCCESS;
            }
        }
        delete [] listFormats;
    }
    *pReturnError = CPVRTString("Failed to load binary shader\n");
    glDeleteShader(*pObject);
    return PVR_FAIL;
}

/*!***************************************************************************
 @Function		PVRTShaderLoadFromFile
 @Input			pszBinFile			binary shader filename
 @Input			pszSrcFile			source shader filename
 @Input			Type				type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 @Input			Format				shader binary format, or 0 for source shader
 @Output		pObject				the resulting shader object
 @Output		pReturnError		the error message if it failed
 @Input			pContext			Context
 @Input			aszDefineArray		Array of defines to be pre-appended to shader string
 @Input			uiDefArraySize		Size of the define array
 @Return		PVR_SUCCESS on success and PVR_FAIL on failure (also fills pReturnError)
 @Description	Loads a shader file into memory and passes it to the GL. 
				It also passes defines that need to be pre-appended to the shader before compilation.
*****************************************************************************/
EPVRTError PVRTShaderLoadFromFile(	const char* const pszBinFile,
									const char* const pszSrcFile,
									const GLenum Type,
									const GLenum Format,
									GLuint* const pObject,
									CPVRTString* const pReturnError,
									const SPVRTContext* const pContext,
									const char* const* aszDefineArray, GLuint uiDefArraySize)
{
	PVRT_UNREFERENCED_PARAMETER(pContext);

	*pReturnError = "";

	/* 	
		Prepending defines relies on altering the source file that is loaded.
		For this reason, the function calls the source loader instead of the binary loader if defines have
		been passed in.
	*/
	if(Format && pszBinFile && uiDefArraySize == 0)
	{
		CPVRTResourceFile ShaderFile(pszBinFile);
		if (ShaderFile.IsOpen())
		{
			if(PVRTShaderLoadBinaryFromMemory(ShaderFile.DataPtr(), ShaderFile.Size(), Type, Format, pObject, pReturnError) == PVR_SUCCESS)
				return PVR_SUCCESS;
		}

		*pReturnError += CPVRTString("Failed to open shader ") + pszBinFile + "\n";
	}

	CPVRTResourceFile ShaderFile(pszSrcFile);
	if (!ShaderFile.IsOpen())
	{
		*pReturnError += CPVRTString("Failed to open shader ") + pszSrcFile + "\n";
		return PVR_FAIL;
	}
 
	CPVRTString ShaderFileString;
	const char* pShaderData = (const char*) ShaderFile.DataPtr();

	// Is our shader resource file data null terminated?
	if(pShaderData[ShaderFile.Size()-1] != '\0')
	{
		// If not create a temporary null-terminated string
		ShaderFileString.assign(pShaderData, ShaderFile.Size());
		pShaderData = ShaderFileString.c_str();
	}

	return PVRTShaderLoadSourceFromMemory(pShaderData, Type, pObject, pReturnError, aszDefineArray, uiDefArraySize);
}

/*!***************************************************************************
 @Function		PVRTCreateProgram
 @Output		pProgramObject			the created program object
 @Input			VertexShader			the vertex shader to link
 @Input			FragmentShader			the fragment shader to link
 @Input			pszAttribs				an array of attribute names
 @Input			i32NumAttribs			the number of attributes to bind
 @Output		pReturnError			the error message if it failed
 @Returns		PVR_SUCCESS on success, PVR_FAIL if failure
 @Description	Links a shader program.
*****************************************************************************/
EPVRTError PVRTCreateProgram(	GLuint* const pProgramObject,
								const GLuint VertexShader,
								const GLuint FragmentShader,
								const char** const pszAttribs,
								const int i32NumAttribs,
								CPVRTString* const pReturnError)
{
	*pProgramObject = glCreateProgram();

    glAttachShader(*pProgramObject, FragmentShader);
    glAttachShader(*pProgramObject, VertexShader);

	for (int i = 0; i < i32NumAttribs; ++i)
	{
		glBindAttribLocation(*pProgramObject, i, pszAttribs[i]);
	}

	// Link the program object
    glLinkProgram(*pProgramObject);
    GLint Linked;
    glGetProgramiv(*pProgramObject, GL_LINK_STATUS, &Linked);
	if (!Linked)
	{
		int i32InfoLogLength, i32CharsWritten;
		glGetProgramiv(*pProgramObject, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetProgramInfoLog(*pProgramObject, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		*pReturnError = CPVRTString("Failed to link: ") + pszInfoLog + "\n";
		delete [] pszInfoLog;
		return PVR_FAIL;
	}

	glUseProgram(*pProgramObject);

	return PVR_SUCCESS;
}

/*****************************************************************************
 End of file (PVRTShader.cpp)
*****************************************************************************/

