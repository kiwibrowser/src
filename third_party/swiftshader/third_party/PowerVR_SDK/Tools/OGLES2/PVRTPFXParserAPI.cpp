/******************************************************************************

 @File         OGLES2/PVRTPFXParserAPI.cpp

 @Title        OGLES2/PVRTPFXParserAPI

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     ANSI compatible

 @Description  PFX file parser.

******************************************************************************/

/*****************************************************************************
** Includes
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "PVRTContext.h"
#include "PVRTMatrix.h"
#include "PVRTFixedPoint.h"
#include "PVRTString.h"
#include "PVRTShader.h"
#include "PVRTPFXParser.h"
#include "PVRTPFXParserAPI.h"
#include "PVRTPFXSemantics.h"
#include "PVRTTexture.h"
#include "PVRTTextureAPI.h"

/*!***************************************************************************
 @Function			CPVRTPFXEffect Constructor
 @Description		Sets the context and initialises the member variables to zero.
*****************************************************************************/
CPVRTPFXEffect::CPVRTPFXEffect():
	m_bLoaded(false), m_psContext(NULL), m_pParser(NULL), m_nEffect(0), m_uiProgram(0), m_Semantics(PVRTPFXSemanticsGetSemanticList(), ePVRTPFX_NumSemantics)
{
}

/*!***************************************************************************
 @Function			CPVRTPFXEffect Constructor
 @Description		Sets the context and initialises the member variables to zero.
*****************************************************************************/
CPVRTPFXEffect::CPVRTPFXEffect(SPVRTContext &sContext):
	m_bLoaded(false), m_psContext(&sContext), m_pParser(NULL), m_nEffect(0), m_uiProgram(0), m_Semantics(PVRTPFXSemanticsGetSemanticList(), ePVRTPFX_NumSemantics)
{
}

/*!***************************************************************************
 @Function			CPVRTPFXEffect Destructor
 @Description		Calls Destroy().
*****************************************************************************/
CPVRTPFXEffect::~CPVRTPFXEffect()
{
	Destroy();
	
	// Free allocated strings
	for(unsigned int uiIndex = ePVRTPFX_NumSemantics; uiIndex < m_Semantics.GetSize(); ++uiIndex)
	{
		delete [] m_Semantics[uiIndex].p;
		m_Semantics[uiIndex].p = NULL;
	}
}

/*!***************************************************************************
 @Function			Load
 @Input				src					PFX Parser Object
 @Input				pszEffect			Effect name
 @Input				pszFileName			Effect file name
 @Output			pReturnError		Error string
 @Returns			EPVRTError			PVR_SUCCESS if load succeeded
 @Description		Loads the specified effect from the CPVRTPFXParser object.
					Compiles and links the shaders. Initialises texture data.
*****************************************************************************/
EPVRTError CPVRTPFXEffect::Load(CPVRTPFXParser &src, const char * const pszEffect, const char * const pszFileName, 
								PVRTPFXEffectDelegate* pDelegate, unsigned int& uiUnknownUniforms, CPVRTString *pReturnError)
{
	unsigned int	 i;

	if(!src.GetNumberEffects())
		return PVR_FAIL;

	// --- First find the named effect from the effect file
	if(pszEffect)
	{
		int iEffect = src.FindEffectByName(CPVRTStringHash(pszEffect));
		if(iEffect == -1)
			return PVR_FAIL;

		m_nEffect = (unsigned int)iEffect;
	}
	else
	{
		m_nEffect = 0;
	}

	// --- Now load the effect
	m_pParser = &src;
	const SPVRTPFXParserEffect &ParserEffect = src.GetEffect(m_nEffect);

	// Create room for per-texture data
	const CPVRTArray<SPVRTPFXParserEffectTexture>& EffectTextures = src.GetEffect(m_nEffect).Textures;
	unsigned int uiNumTexturesForEffect = EffectTextures.GetSize();
	m_Textures.SetCapacity(uiNumTexturesForEffect);

	// Initialise each Texture
	for(i = 0; i < uiNumTexturesForEffect; ++i)
	{
		int iTexIdx = src.FindTextureByName(EffectTextures[i].Name);
		if(iTexIdx < 0)
		{
			*pReturnError += PVRTStringFromFormattedStr("ERROR: Effect '%s' requests non-existent texture: %s\n", ParserEffect.Name.c_str(), EffectTextures[i].Name.c_str());
			return PVR_FAIL;
		}

		unsigned int uiTexIdx = m_Textures.Append();
		m_Textures[uiTexIdx].Name	= src.GetTexture((unsigned int)iTexIdx)->Name;
		m_Textures[uiTexIdx].ui		= 0xFFFFFFFF;
		m_Textures[uiTexIdx].flags	= 0;
		m_Textures[uiTexIdx].unit	= 0;
	}

	// Load the shaders
	if(LoadShadersForEffect(src, pszFileName, pReturnError) != PVR_SUCCESS)
		return PVR_FAIL;

	// Build uniform table
	if(RebuildUniformTable(uiUnknownUniforms, pReturnError) != PVR_SUCCESS)
		return PVR_FAIL;

	// Load the requested textures
	if(pDelegate)
	{
		if(LoadTexturesForEffect(pDelegate, pReturnError) != PVR_SUCCESS)
			return PVR_FAIL;
	}

	m_bLoaded = true;

	return PVR_SUCCESS;
}

/*!***************************************************************************
@Function		LoadTexturesForEffect
@Output			pReturnError
@Return			EPVRTError	
@Description	Loads all of the textures for this effect.
*****************************************************************************/
EPVRTError CPVRTPFXEffect::LoadTexturesForEffect(PVRTPFXEffectDelegate* pDelegate, CPVRTString *pReturnError)
{
	GLuint			uiHandle;
	unsigned int	uiFlags;
	
	for(unsigned int i = 0; i < m_Textures.GetSize(); ++i)
	{
		int iTexID = m_pParser->FindTextureByName(m_Textures[i].Name);
		if(iTexID == -1)
		{
			*pReturnError += PVRTStringFromFormattedStr("ERROR: Cannot find texture '%s' in any TEXTURE block.\n", m_Textures[i].Name.c_str());
			return PVR_FAIL;
		}

		const SPVRTPFXParserTexture* pTexDesc = m_pParser->GetTexture(iTexID);
		
		
		uiHandle = 0xBADF00D;
		uiFlags  = 0;

		if(pDelegate->PVRTPFXOnLoadTexture(pTexDesc->FileName, uiHandle, uiFlags) != PVR_SUCCESS)
		{
			*pReturnError += PVRTStringFromFormattedStr("ERROR: Failed to load texture: %s.\n", pTexDesc->FileName.c_str());
			return PVR_FAIL;
		}
	
		// Make sure uiHandle was written.
		if(uiHandle == 0xBADF00D)
		{
			*pReturnError += PVRTStringFromFormattedStr("ERROR: GL handle for texture '%s' not set!\n", pTexDesc->FileName.c_str());
			return PVR_FAIL;
		}
		
		SetTexture(i, uiHandle, uiFlags);
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
@Function		LoadShadersForEffect
@Input			pszFileName
@Output			pReturnError
@Return			EPVRTError	
@Description	Loads all of the GLSL shaders for an effect.
*****************************************************************************/
EPVRTError CPVRTPFXEffect::LoadShadersForEffect(CPVRTPFXParser &src, const char * const pszFileName, CPVRTString *pReturnError)
{
	// initialise attributes to default values
	char *pszVertexShader		= NULL;
	char *pszFragmentShader		= NULL;
	bool bFreeVertexShader		= false;
	bool bFreeFragmentShader	= false;
	unsigned int uiVertIdx		= 0;
	unsigned int uiFragIdx		= 0;
	unsigned int uiVertexShader	= 0;
	unsigned int uiFragShader	= 0;

	const SPVRTPFXParserEffect &ParserEffect = src.GetEffect(m_nEffect);

	// find shaders requested
	for(uiVertIdx = 0; uiVertIdx < src.GetNumberVertexShaders(); ++uiVertIdx)
	{
		const SPVRTPFXParserShader& VertexShader = src.GetVertexShader(uiVertIdx);
		if(ParserEffect.VertexShaderName == VertexShader.Name)
		{
			if(VertexShader.bUseFileName)
			{
				pszVertexShader = VertexShader.pszGLSLcode;
			}
			else
			{
				if(!VertexShader.pszGLSLcode)
					continue;			// No code specified.
#if 0
				// offset glsl code by nFirstLineNumber
				pszVertexShader = (char *)malloc((strlen(VertexShader.pszGLSLcode) + (VertexShader.nFirstLineNumber) + 1) * sizeof(char));
				pszVertexShader[0] = '\0';
			 	for(unsigned int n = 0; n < VertexShader.nFirstLineNumber; n++)
					strcat(pszVertexShader, "\n");
				strcat(pszVertexShader, VertexShader.pszGLSLcode);
#else
				pszVertexShader = (char *)malloc(strlen(VertexShader.pszGLSLcode) + 1);
				pszVertexShader[0] = '\0';
				strcat(pszVertexShader, VertexShader.pszGLSLcode);
#endif
				bFreeVertexShader = true;
			}

			break;
		}
	}
	for(uiFragIdx = 0; uiFragIdx < src.GetNumberFragmentShaders(); ++uiFragIdx)
	{
		const SPVRTPFXParserShader& FragmentShader = src.GetFragmentShader(uiFragIdx);
		if(ParserEffect.FragmentShaderName == FragmentShader.Name)
		{
			if(FragmentShader.bUseFileName)
			{
				pszFragmentShader = FragmentShader.pszGLSLcode;
			}
			else
			{
				if(!FragmentShader.pszGLSLcode)
					continue;			// No code specified.

#if 0
				// offset glsl code by nFirstLineNumber
				pszFragmentShader = (char *)malloc((strlen(FragmentShader.pszGLSLcode) + (FragmentShader.nFirstLineNumber) + 1) * sizeof(char));
				pszFragmentShader[0] = '\0';
				for(unsigned int n = 0; n < FragmentShader.nFirstLineNumber; n++)
					strcat(pszFragmentShader, "\n");
				strcat(pszFragmentShader, FragmentShader.pszGLSLcode);
#else
				pszFragmentShader = (char *)malloc(strlen(FragmentShader.pszGLSLcode) + 1);
				pszFragmentShader[0] = '\0';
				strcat(pszFragmentShader, FragmentShader.pszGLSLcode);
#endif
				bFreeFragmentShader = true;
			}

			break;
		}
	}

	CPVRTString error;
	bool		bLoadSource = 1;

	// Try first to load from the binary block
	if (src.GetVertexShader(uiVertIdx).pbGLSLBinary!=NULL)
	{
#if defined(GL_SGX_BINARY_IMG)
		if (PVRTShaderLoadBinaryFromMemory(src.GetVertexShader(uiVertIdx).pbGLSLBinary, src.GetVertexShader(uiVertIdx).nGLSLBinarySize,
			GL_VERTEX_SHADER, GL_SGX_BINARY_IMG, &uiVertexShader, &error) == PVR_SUCCESS)
		{
			// success loading the binary block so we do not need to load the source
			bLoadSource = 0;
		}
		else
#endif
		{
			bLoadSource = 1;
		}
	}

	// If it fails, load from source
	if (bLoadSource)
	{
		if(pszVertexShader)
		{
			if (PVRTShaderLoadSourceFromMemory(pszVertexShader, GL_VERTEX_SHADER, &uiVertexShader, &error) != PVR_SUCCESS)
			{
				*pReturnError = CPVRTString("ERROR: Vertex Shader compile error in file '") + pszFileName + "':\n" + error;
				if(bFreeVertexShader)	FREE(pszVertexShader);
				if(bFreeFragmentShader)	FREE(pszFragmentShader);
				return PVR_FAIL;
			}
		}
		else // Shader not found or failed binary block
		{
			if (src.GetVertexShader(uiVertIdx).pbGLSLBinary==NULL)
			{
				*pReturnError = CPVRTString("ERROR: Vertex shader ") + ParserEffect.VertexShaderName.String() + "  not found in " + pszFileName + ".\n";
			}
			else
			{
				*pReturnError = CPVRTString("ERROR: Binary vertex shader ") + ParserEffect.VertexShaderName.String() + " not supported.\n";
			}

			if(bFreeVertexShader)	FREE(pszVertexShader);
			if(bFreeFragmentShader)	FREE(pszFragmentShader);
			return PVR_FAIL;
		}
	}

	// Try first to load from the binary block
	if (src.GetFragmentShader(uiFragIdx).pbGLSLBinary!=NULL)
	{
#if defined(GL_SGX_BINARY_IMG)
		if (PVRTShaderLoadBinaryFromMemory(src.GetFragmentShader(uiFragIdx).pbGLSLBinary, src.GetFragmentShader(uiFragIdx).nGLSLBinarySize,
			GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &uiFragShader, &error) == PVR_SUCCESS)
		{
			// success loading the binary block so we do not need to load the source
			bLoadSource = 0;
		}
		else
#endif
		{
			bLoadSource = 1;
		}
	}

	// If it fails, load from source
	if (bLoadSource)
	{
		if(pszFragmentShader)
		{
			if (PVRTShaderLoadSourceFromMemory(pszFragmentShader, GL_FRAGMENT_SHADER, &uiFragShader, &error) != PVR_SUCCESS)
			{
				*pReturnError = CPVRTString("ERROR: Fragment Shader compile error in file '") + pszFileName + "':\n" + error;
				if(bFreeVertexShader)	FREE(pszVertexShader);
				if(bFreeFragmentShader)	FREE(pszFragmentShader);
				return PVR_FAIL;
			}
		}
		else // Shader not found or failed binary block
		{
			if (src.GetFragmentShader(uiFragIdx).pbGLSLBinary==NULL)
			{
				*pReturnError = CPVRTString("ERROR: Fragment shader ") + ParserEffect.FragmentShaderName.String() + "  not found in " + pszFileName + ".\n";
			}
			else
			{
				*pReturnError = CPVRTString("ERROR: Binary Fragment shader ") + ParserEffect.FragmentShaderName.String() + " not supported.\n";
			}

			if(bFreeVertexShader)
				FREE(pszVertexShader);
			if(bFreeFragmentShader)
				FREE(pszFragmentShader);

			return PVR_FAIL;
		}
	}

	if(bFreeVertexShader)
		FREE(pszVertexShader);

	if(bFreeFragmentShader)
		FREE(pszFragmentShader);

	// Create the shader program
	m_uiProgram = glCreateProgram();


	// Attach the fragment and vertex shaders to it
	glAttachShader(m_uiProgram, uiFragShader);
	glAttachShader(m_uiProgram, uiVertexShader);

	glDeleteShader(uiVertexShader);
	glDeleteShader(uiFragShader);

	// Bind vertex attributes
	for(unsigned int i = 0; i < ParserEffect.Attributes.GetSize(); ++i)
	{
		glBindAttribLocation(m_uiProgram, i, ParserEffect.Attributes[i].pszName);
	}

	//	Link the program.
	glLinkProgram(m_uiProgram);
	GLint Linked;
	glGetProgramiv(m_uiProgram, GL_LINK_STATUS, &Linked);
	if (!Linked)
	{
		int i32InfoLogLength, i32CharsWritten;
		glGetProgramiv(m_uiProgram, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetProgramInfoLog(m_uiProgram, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		*pReturnError = CPVRTString("ERROR: Linking shaders in file '") + pszFileName + "':\n\n"
			+ CPVRTString("Failed to link: ") + pszInfoLog + "\n";
		delete [] pszInfoLog;
		return PVR_FAIL;
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function			Destroy
 @Description		Deletes the gl program object and texture data.
*****************************************************************************/
void CPVRTPFXEffect::Destroy()
{
	{
		if(m_uiProgram != 0)
		{
            GLint val;
            glGetProgramiv(m_uiProgram, GL_DELETE_STATUS, &val);
            if(val == GL_FALSE)
            {
                glDeleteProgram(m_uiProgram);
            }
			m_uiProgram = 0;
		}
	}

	m_bLoaded = false;
}

/*!***************************************************************************
 @Function			Activate
 @Returns			PVR_SUCCESS if activate succeeded
 @Description		Selects the gl program object and binds the textures.
*****************************************************************************/
EPVRTError CPVRTPFXEffect::Activate(const int i32RenderTextureId, const unsigned int ui32ReplacementTexture)
{
	GLuint uiTextureId;
	GLenum eTarget;

	// Set the program
	glUseProgram(m_uiProgram);

	// Set the textures
	for(unsigned int uiTex = 0; uiTex < m_Textures.GetSize(); ++uiTex)
	{
		uiTextureId = m_Textures[uiTex].ui;
		if(i32RenderTextureId != -1 && (uiTextureId == (unsigned int)i32RenderTextureId))
			uiTextureId = ui32ReplacementTexture;

		// Set active texture unit.
		glActiveTexture(GL_TEXTURE0 + m_Textures[uiTex].unit);

		// Bind texture
		eTarget = (m_Textures[uiTex].flags & PVRTEX_CUBEMAP ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D);
		glBindTexture(eTarget, uiTextureId);
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function			GetSemantics
 @Output			aUniforms				an array of uniform data
 @Output			pnUnknownUniformCount	unknown uniform count
 @Input				psParams				pointer to semantic data array
 @Input				nParamCount				number of samantic items
 @Input				psUniformSemantics		pointer to uniform semantics array
 @Input				nUniformSemantics		number of uniform semantic items
 @Input				pglesExt				opengl extensions object
 @Input				uiProgram				program object index
 @Input				bIsAttribue				true if getting attribute semantics
 @Output			errorMsg				error string
 @Returns			unsigned int			number of successful semantics
 @Description		Get the data array for the semantics.
*****************************************************************************/
static unsigned int GetSemantics(
	CPVRTArray<SPVRTPFXUniform>&				aUniforms,
	const CPVRTArray<SPVRTPFXParserSemantic>&	aParams,
	const CPVRTArray<SPVRTPFXUniformSemantic>&	aUniformSemantics,
	unsigned int*								const pnUnknownUniformCount,
	const GLuint								uiProgram,
	bool										bIsAttribue,
	CPVRTString*								const errorMsg)
{
	unsigned int	i, j, nCount, nCountUnused;
	int				nLocation;

	/*
		Loop over the parameters searching for their semantics. If
		found/recognised, it should be placed in the output array.
	*/
	nCount = 0;
	nCountUnused = 0;
	char szTmpUniformName[2048];		// Temporary buffer to use for building uniform names.

	for(j = 0; j < aParams.GetSize(); ++j)
	{
		for(i = 0; i < aUniformSemantics.GetSize(); ++i)
		{
			if(strcmp(aParams[j].pszValue, aUniformSemantics[i].p) != 0)
			{
				continue;
			}

			// Semantic found for this parameter
			if(bIsAttribue)
			{
				nLocation = glGetAttribLocation(uiProgram, aParams[j].pszName);
			}
			else
			{
				nLocation = glGetUniformLocation(uiProgram, aParams[j].pszName);
				
				// Check for array. Workaround for some OpenGL:ES implementations which require array element appended to uniform name
				// in order to return the correct location.
				if(nLocation == -1)
				{
					strcpy(szTmpUniformName, aParams[j].pszName);
					strcat(szTmpUniformName, "[0]");
					nLocation = glGetUniformLocation(uiProgram, szTmpUniformName);
				}
			}

			if(nLocation != -1)
			{
				unsigned int uiIdx = aUniforms.Append();
				aUniforms[uiIdx].nSemantic	= aUniformSemantics[i].n;
				aUniforms[uiIdx].nLocation	= nLocation;
				aUniforms[uiIdx].nIdx		= aParams[j].nIdx;
				aUniforms[uiIdx].sValueName	= aParams[j].pszName;
				++nCount;
			}
			else
			{
				*errorMsg += "WARNING: Variable not used by GLSL code: ";
				*errorMsg += CPVRTString(aParams[j].pszName) + " ";
				*errorMsg += CPVRTString(aParams[j].pszValue) + "\n";
				++nCountUnused;
			}

			// Skip to the next parameter
			break;
		}
		if(i == aUniformSemantics.GetSize())
		{
			*errorMsg += "WARNING: Semantic unknown to application: ";
			*errorMsg += CPVRTString(aParams[j].pszValue) + "\n";
		}
	}

	*pnUnknownUniformCount	= aParams.GetSize() - nCount - nCountUnused;
	return nCount;
}

/*!***************************************************************************
@Function		GetUniformArray
@Return			const CPVRTArray<SPVRTPFXUniform>&	
@Description	Returns a list of known semantics.
*****************************************************************************/
const CPVRTArray<SPVRTPFXUniform>& CPVRTPFXEffect::GetUniformArray() const
{
	return m_Uniforms;
}

/*!***************************************************************************
@Function		BuildUniformTable
@Output			uiUnknownSemantics
@Output			pReturnError
@Return			EPVRTError	
@Description	Builds the uniform table from a list of known semantics.
*****************************************************************************/
EPVRTError CPVRTPFXEffect::RebuildUniformTable(unsigned int& uiUnknownSemantics, CPVRTString* pReturnError)
{
	unsigned int			nUnknownCount;
	const SPVRTPFXParserEffect&	ParserEffect = m_pParser->GetEffect(m_nEffect);

	GetSemantics(m_Uniforms, ParserEffect.Uniforms, m_Semantics, &nUnknownCount, m_uiProgram, false, pReturnError);
	uiUnknownSemantics	= nUnknownCount;

	GetSemantics(m_Uniforms, ParserEffect.Attributes, m_Semantics, &nUnknownCount, m_uiProgram, true, pReturnError);
	uiUnknownSemantics	+= nUnknownCount;

	return PVR_SUCCESS;
}

/*!***************************************************************************
@Function		RegisterUniformSemantic
@Input			psUniforms
@Input			uiNumUniforms
@Return			EPVRTError	
@Description	Registers a user-provided uniform semantic.
*****************************************************************************/
EPVRTError CPVRTPFXEffect::RegisterUniformSemantic(const SPVRTPFXUniformSemantic* const psUniforms, unsigned int uiNumUniforms, CPVRTString* pReturnError)
{
	for(unsigned int uiIndex = 0; uiIndex < uiNumUniforms; ++uiIndex)
	{
		// Check that this doesn't already exist.
		if(m_Semantics.Contains(psUniforms[uiIndex]))
		{
			*pReturnError += PVRTStringFromFormattedStr("ERROR: Uniform semantic with ID '%u' already exists.\n", psUniforms[uiIndex].n);
			return PVR_FAIL;
		}

		// Make copy as we need to manage the memory.
		char* pSemName = new char[strlen(psUniforms[uiIndex].p)+1];
		strcpy(pSemName, psUniforms[uiIndex].p);

		unsigned int uiIdx = m_Semantics.Append();
		m_Semantics[uiIdx].n = psUniforms[uiIndex].n;
		m_Semantics[uiIdx].p = pSemName;
	}

	// Check if the effect has already been loaded. If it hasn't, great. If it has, we need to rebuild the uniform table.
	if(m_bLoaded)
	{
		// Clear the current list.
		m_Uniforms.Clear();

		unsigned int uiUnknownSemantics;
		return RebuildUniformTable(uiUnknownSemantics, pReturnError);
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
@Function		RemoveUniformSemantic
@Input			uiSemanticID
@Output			pReturnError
@Return			PVR_SUCCESS on success	
@Description	Removes a given semantic ID from the 'known' semantic list and 
				re-parses the effect to update the uniform table.
*****************************************************************************/
EPVRTError CPVRTPFXEffect::RemoveUniformSemantic(unsigned int uiSemanticID, CPVRTString* pReturnError)
{
	// Make sure that the given ID isn't a PFX semantic
	if(uiSemanticID < ePVRTPFX_NumSemantics)
	{
		*pReturnError += "ERROR: Cannot remove a default PFX semantic.";
		return PVR_FAIL;
	}

	// Find the index in the array
	unsigned int uiSemanticIndex = 0;
	while(uiSemanticIndex < m_Semantics.GetSize() && m_Semantics[uiSemanticIndex].n != uiSemanticID) ++uiSemanticIndex;

	if(uiSemanticIndex == m_Semantics.GetSize())
	{
		*pReturnError += PVRTStringFromFormattedStr("ERROR: Semantic with ID %d does not exist.", uiSemanticID);
		return PVR_FAIL;
	}

	m_Semantics.Remove(uiSemanticIndex);

	// Check if the effect has already been loaded. If it hasn't, great. If it has, we need to rebuild the uniform table.
	if(m_bLoaded)
	{
		// Clear the current list.
		m_Uniforms.Clear();

		unsigned int uiUnknownSemantics;
		return RebuildUniformTable(uiUnknownSemantics, pReturnError);
	}

	return PVR_SUCCESS;
}

/*!***************************************************************************
 @Function			GetTextureArray
 @Output			nCount					number of textures
 @Returns			SPVRTPFXTexture*		pointer to the texture data array
 @Description		Gets the texture data array.
*****************************************************************************/
const CPVRTArray<SPVRTPFXTexture>& CPVRTPFXEffect::GetTextureArray() const
{
	return m_Textures;
}

/*!***************************************************************************
 @Function			SetTexture
 @Input				nIdx				texture number
 @Input				ui					opengl texture handle
 @Input				u32flags			texture flags
 @Description		Sets the textrue and applys the filtering.
*****************************************************************************/
void CPVRTPFXEffect::SetTexture(const unsigned int nIdx, const GLuint ui, const unsigned int u32flags)
{
	if(nIdx < (unsigned int) m_Textures.GetSize())
	{
		GLenum u32Target = GL_TEXTURE_2D;

		// Check if texture is a cubemap
		if((u32flags & PVRTEX_CUBEMAP) != 0)
			u32Target = GL_TEXTURE_CUBE_MAP;

		// Get the texture details from the PFX Parser. This contains details such as mipmapping and filter modes.
		const CPVRTStringHash& TexName = m_pParser->GetEffect(m_nEffect).Textures[nIdx].Name;
		int iTexIdx = m_pParser->FindTextureByName(TexName);
		if(iTexIdx == -1)
			return;

		const SPVRTPFXParserTexture* pPFXTex = m_pParser->GetTexture(iTexIdx);
			
		// Only change parameters if ui (handle is > 0)
		if(ui > 0)
		{
			glBindTexture(u32Target, ui);

			// Set default filter from PFX file

			// --- Mipmapping/Minification
			switch(pPFXTex->nMIP)
			{
			case eFilter_None:			// No mipmapping
				switch(pPFXTex->nMin)
				{
				case eFilter_Nearest:
					glTexParameteri(u32Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);					// Off
					break;
				case eFilter_Linear:
					glTexParameteri(u32Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);					// Bilinear - no Mipmap
					break;
				}
				break;
			case eFilter_Nearest:		// Standard mipmapping
				switch(pPFXTex->nMin)
				{
				case eFilter_Nearest:
					glTexParameteri(u32Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);		// Nearest	- std. Mipmap
					break;
				case eFilter_Linear:
					glTexParameteri(u32Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);		// Bilinear - std. Mipmap
					break;
				}
				break;
			case eFilter_Linear:		// Trilinear mipmapping
				switch(pPFXTex->nMin)
				{
				case eFilter_Nearest:
					glTexParameteri(u32Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);		// Nearest - Trilinear
					break;
				case eFilter_Linear:
					glTexParameteri(u32Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);			// Bilinear - Trilinear
					break;
				}
				break;
			}

			// --- Magnification
			switch(pPFXTex->nMag)
			{
			case eFilter_Nearest:
				glTexParameteri(u32Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;
			case eFilter_Linear:
				glTexParameteri(u32Target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
			}

			// --- Wrapping S
			switch(pPFXTex->nWrapS)
			{
			case eWrap_Clamp:
				glTexParameteri(u32Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				break;
			case eWrap_Repeat:
				glTexParameteri(u32Target, GL_TEXTURE_WRAP_S, GL_REPEAT);
				break;
			}

			// --- Wrapping T
			switch(pPFXTex->nWrapT)
			{
			case eWrap_Clamp:
				glTexParameteri(u32Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case eWrap_Repeat:
				glTexParameteri(u32Target, GL_TEXTURE_WRAP_T, GL_REPEAT);
				break;
			}

			// --- Wrapping R
	#ifdef GL_TEXTURE_WRAP_R
			switch(pPFXTex->nWrapR)
			{
			case eWrap_Clamp:
				glTexParameteri(u32Target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				break;
			case eWrap_Repeat:
				glTexParameteri(u32Target, GL_TEXTURE_WRAP_R, GL_REPEAT);
				break;
			}
	#endif
		}

		// Store the texture details
		m_Textures[nIdx].ui	   = ui;
		m_Textures[nIdx].flags = u32flags;

		// Find the texture unit from the parser
		unsigned int uiIndex = m_pParser->FindTextureIndex(pPFXTex->Name, m_nEffect);
		if(uiIndex != 0xFFFFFFFF)
		{
			m_Textures[nIdx].unit = m_pParser->GetEffect(m_nEffect).Textures[uiIndex].nNumber;
		}
	}
}


/*!***************************************************************************
 @Function			SetDefaultSemanticValue
 @Input				pszName				name of uniform
 @Input				psDefaultValue      pointer to default value
 @Description		Sets the default value for the uniform semantic.
*****************************************************************************/
void CPVRTPFXEffect::SetDefaultUniformValue(const char *const pszName, const SPVRTSemanticDefaultData *psDefaultValue)
{
	
	GLint nLocation = glGetUniformLocation(m_uiProgram, pszName);
	// Check for array. Workaround for some OpenGL:ES implementations which require array element appended to uniform name
	// in order to return the correct location.
	if(nLocation == -1)
	{
		char szTmpUniformName[2048];
		strcpy(szTmpUniformName, pszName);
		strcat(szTmpUniformName, "[0]");
		nLocation = glGetUniformLocation(m_uiProgram, szTmpUniformName);
	}

	switch(psDefaultValue->eType)
	{
		case eDataTypeMat2:
			glUniformMatrix2fv(nLocation, 1, GL_FALSE, psDefaultValue->pfData);
			break;
		case eDataTypeMat3:
			glUniformMatrix3fv(nLocation, 1, GL_FALSE, psDefaultValue->pfData);
			break;
		case eDataTypeMat4:
			glUniformMatrix4fv(nLocation, 1, GL_FALSE, psDefaultValue->pfData);
			break;
		case eDataTypeVec2:
			glUniform2fv(nLocation, 1, psDefaultValue->pfData);
			break;
		case eDataTypeRGB:
		case eDataTypeVec3:
			glUniform3fv(nLocation, 1, psDefaultValue->pfData);
			break;
		case eDataTypeRGBA:
		case eDataTypeVec4:
			glUniform4fv(nLocation, 1, psDefaultValue->pfData);
			break;
		case eDataTypeIvec2:
			glUniform2iv(nLocation, 1, psDefaultValue->pnData);
			break;
		case eDataTypeIvec3:
			glUniform3iv(nLocation, 1, psDefaultValue->pnData);
			break;
		case eDataTypeIvec4:
			glUniform4iv(nLocation, 1, psDefaultValue->pnData);
			break;
		case eDataTypeBvec2:
			glUniform2i(nLocation, psDefaultValue->pbData[0] ? 1 : 0, psDefaultValue->pbData[1] ? 1 : 0);
			break;
		case eDataTypeBvec3:
			glUniform3i(nLocation, psDefaultValue->pbData[0] ? 1 : 0, psDefaultValue->pbData[1] ? 1 : 0, psDefaultValue->pbData[2] ? 1 : 0);
			break;
		case eDataTypeBvec4:
			glUniform4i(nLocation, psDefaultValue->pbData[0] ? 1 : 0, psDefaultValue->pbData[1] ? 1 : 0, psDefaultValue->pbData[2] ? 1 : 0, psDefaultValue->pbData[3] ? 1 : 0);
			break;
		case eDataTypeFloat:
			glUniform1f(nLocation, psDefaultValue->pfData[0]);
			break;
		case eDataTypeInt:
			glUniform1i(nLocation, psDefaultValue->pnData[0]);
			break;
		case eDataTypeBool:
			glUniform1i(nLocation, psDefaultValue->pbData[0] ? 1 : 0);
			break;

		case eNumDefaultDataTypes:
		case eDataTypeNone:
		default:
			break;
	}
}

/*!***************************************************************************
@Function		SetContext
@Input			pContext
@Description	
*****************************************************************************/
void CPVRTPFXEffect::SetContext(SPVRTContext *const pContext)
{
	m_psContext = pContext;
}

/*!***************************************************************************
@Function		GetProgramHandle
@Return			unsigned int	
@Description	Returns the OGL program handle.
*****************************************************************************/
unsigned int CPVRTPFXEffect::GetProgramHandle() const
{
	return m_uiProgram;
}

/*!***************************************************************************
@Function		GetEffectIndex
@Return			unsigned int	
@Description	Gets the active effect index within the PFX file.
*****************************************************************************/
unsigned int CPVRTPFXEffect::GetEffectIndex() const
{
	return m_nEffect;
}

/*!***************************************************************************
@Function		GetSemanticArray
@Return			const CPVRTArray<SPVRTPFXUniformSemantic>&	
@Description	Gets the array of registered semantics which will be used to
				match PFX code.
*****************************************************************************/
const CPVRTArray<SPVRTPFXUniformSemantic>& CPVRTPFXEffect::GetSemanticArray() const
{
	return m_Semantics;
}

/*****************************************************************************
 End of file (PVRTPFXParserAPI.cpp)
*****************************************************************************/

