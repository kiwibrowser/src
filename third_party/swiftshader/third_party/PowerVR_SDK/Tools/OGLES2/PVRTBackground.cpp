/******************************************************************************

 @File         OGLES2/PVRTBackground.cpp

 @Title        OGLES2/PVRTBackground

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  Function to draw a background texture.

******************************************************************************/
#include "PVRTShader.h"
#include "PVRTBackground.h"

// The header that contains the shaders
#include "PVRTBackgroundShaders.h"

// Index to bind the attributes to vertex shaders
const int VERTEX_ARRAY = 0;
const int TEXCOORD_ARRAY = 1;

/****************************************************************************
** Structures
****************************************************************************/

// The struct to include various API variables
struct SPVRTBackgroundAPI
{
	GLuint	m_ui32VertexShader;
	GLuint	m_ui32FragShader;
	GLuint	m_ui32ProgramObject;
	GLuint	m_ui32VertexBufferObject;
};

/****************************************************************************
** Class: CPVRTBackground
****************************************************************************/

/*****************************************************************************
 @Function			Background
 @Description		Init some values.
*****************************************************************************/
CPVRTBackground::CPVRTBackground(void)
{
	m_bInit = false;
	m_pAPI  = 0;
}


/*****************************************************************************
 @Function			~Background
 @Description		Calls Destroy()
*****************************************************************************/
CPVRTBackground::~CPVRTBackground(void)
{
	delete m_pAPI;
	m_pAPI = 0;
}

/*!***************************************************************************
 @Function		Destroy
 @Description	Destroys the background and releases API specific resources
*****************************************************************************/
void CPVRTBackground::Destroy()
{
	if(m_bInit)
	{
		// Delete shaders
		glDeleteProgram(m_pAPI->m_ui32ProgramObject);
		glDeleteShader(m_pAPI->m_ui32VertexShader);
		glDeleteShader(m_pAPI->m_ui32FragShader);

		// Delete buffer objects
		glDeleteBuffers(1, &m_pAPI->m_ui32VertexBufferObject);

		m_bInit = false;
	}

	delete m_pAPI;
	m_pAPI = 0;
}

/*!***************************************************************************
 @Function		Init
 @Input			pContext	A pointer to a PVRTContext
 @Input			bRotate		true to rotate texture 90 degrees.
 @Input			pszError	An option string for returning errors
 @Return 		PVR_SUCCESS on success
 @Description	Initialises the background
*****************************************************************************/
EPVRTError CPVRTBackground::Init(const SPVRTContext * const pContext, bool bRotate, CPVRTString *pszError)
{
	PVRT_UNREFERENCED_PARAMETER(pContext);

	Destroy();

	m_pAPI = new SPVRTBackgroundAPI;

	if(!m_pAPI)
	{
		if(pszError)
			*pszError = "Error: Insufficient memory to allocate SCPVRTBackgroundAPI.";

		return PVR_FAIL;
	}

	m_pAPI->m_ui32VertexShader = 0;
	m_pAPI->m_ui32FragShader = 0;
	m_pAPI->m_ui32ProgramObject = 0;
	m_pAPI->m_ui32VertexBufferObject = 0;

	bool bResult;
	CPVRTString sTmpErrStr;

	// The shader loading code doesn't expect a null pointer for the error string
	if(!pszError)
		pszError = &sTmpErrStr;

	/* Compiles the shaders. For a more detailed explanation, see IntroducingPVRTools */
#if defined(GL_SGX_BINARY_IMG)
	// Try binary shaders first
	bResult = (PVRTShaderLoadBinaryFromMemory(_BackgroundFragShader_fsc, _BackgroundFragShader_fsc_size,
					GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &m_pAPI->m_ui32FragShader, pszError) == PVR_SUCCESS)
		       && (PVRTShaderLoadBinaryFromMemory(_BackgroundVertShader_vsc, _BackgroundVertShader_vsc_size,
					GL_VERTEX_SHADER, GL_SGX_BINARY_IMG, &m_pAPI->m_ui32VertexShader, pszError) == PVR_SUCCESS);
	if(!bResult)
#endif
	{
		// if binary shaders don't work, try source shaders
		bResult = (PVRTShaderLoadSourceFromMemory(_BackgroundFragShader_fsh, GL_FRAGMENT_SHADER, &m_pAPI->m_ui32FragShader, pszError) == PVR_SUCCESS) &&
				(PVRTShaderLoadSourceFromMemory(_BackgroundVertShader_vsh, GL_VERTEX_SHADER, &m_pAPI->m_ui32VertexShader, pszError)  == PVR_SUCCESS);
	}

	_ASSERT(bResult);

	if(!bResult)
		return PVR_FAIL;

	// Reset the error string
	if(pszError)
		*pszError = "";

	// Create the shader program
	m_pAPI->m_ui32ProgramObject = glCreateProgram();

	// Attach the fragment and vertex shaders to it
	glAttachShader(m_pAPI->m_ui32ProgramObject, m_pAPI->m_ui32FragShader);
	glAttachShader(m_pAPI->m_ui32ProgramObject, m_pAPI->m_ui32VertexShader);

	// Bind the custom vertex attribute "myVertex" to location VERTEX_ARRAY
	glBindAttribLocation(m_pAPI->m_ui32ProgramObject, VERTEX_ARRAY, "myVertex");

	// Bind the custom vertex attribute "myUV" to location TEXCOORD_ARRAY
	glBindAttribLocation(m_pAPI->m_ui32ProgramObject, TEXCOORD_ARRAY, "myUV");

	// Link the program
	glLinkProgram(m_pAPI->m_ui32ProgramObject);
	GLint Linked;
	glGetProgramiv(m_pAPI->m_ui32ProgramObject, GL_LINK_STATUS, &Linked);
	if (!Linked)
	{
		int i32InfoLogLength, i32CharsWritten;
		glGetProgramiv(m_pAPI->m_ui32ProgramObject, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetProgramInfoLog(m_pAPI->m_ui32ProgramObject, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		*pszError = CPVRTString("Failed to link: ") + pszInfoLog + "\n";
		delete [] pszInfoLog;
		bResult = false;
	}

	_ASSERT(bResult);

	if(!bResult)
		return PVR_FAIL;

	// Use the loaded shader program
	glUseProgram(m_pAPI->m_ui32ProgramObject);

	// Set the sampler2D variable to the first texture unit
	glUniform1i(glGetUniformLocation(m_pAPI->m_ui32ProgramObject, "sampler2d"), 0);

	// Create the vertex buffer object
	GLfloat *pVertexData = 0;

	// The vertex data for non-rotated
	GLfloat afVertexData[16] = { -1, -1, 1, -1, -1, 1, 1, 1,
						0, 0, 1, 0, 0, 1, 1, 1};

	// The vertex data for rotated
	GLfloat afVertexDataRotated[16] = {-1, 1, -1, -1, 1, 1, 1, -1,
						1, 1, 0, 1, 1, 0, 0, 0};

	if(!bRotate)
		pVertexData = &afVertexData[0];
	else
		pVertexData = &afVertexDataRotated[0];

	glGenBuffers(1, &m_pAPI->m_ui32VertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, m_pAPI->m_ui32VertexBufferObject);

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, pVertexData, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	m_bInit = true;

	return PVR_SUCCESS;
}


/*!***************************************************************************
 @Function		Draw
 @Input			ui32Texture	Texture to use
 @Return 		PVR_SUCCESS on success
 @Description	Draws a texture on a quad covering the whole screen.
*****************************************************************************/
EPVRTError CPVRTBackground::Draw(const GLuint ui32Texture)
{
	if(!m_bInit)
		return PVR_FAIL;

	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, ui32Texture);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// Use the loaded shader program
	glUseProgram(m_pAPI->m_ui32ProgramObject);

	// Set vertex data
	glBindBuffer(GL_ARRAY_BUFFER, m_pAPI->m_ui32VertexBufferObject);

	glEnableVertexAttribArray(VERTEX_ARRAY);
	glVertexAttribPointer(VERTEX_ARRAY, 2, GL_FLOAT, GL_FALSE, 0, (const void*) 0);

	// Set texture coordinates
	glEnableVertexAttribArray(TEXCOORD_ARRAY);
	glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, 0, (const void*) (8 * sizeof(float)));

	// Render geometry
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	glDisableVertexAttribArray(VERTEX_ARRAY);
	glDisableVertexAttribArray(TEXCOORD_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(0);

	return PVR_SUCCESS;
}

/*****************************************************************************
 End of file (CPVRTBackground.cpp)
*****************************************************************************/

