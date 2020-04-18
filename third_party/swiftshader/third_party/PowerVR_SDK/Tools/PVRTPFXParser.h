/*!****************************************************************************

 @file         PVRTPFXParser.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Declaration of PFX file parser

******************************************************************************/

#ifndef _PVRTPFXPARSER_H_
#define _PVRTPFXPARSER_H_


/*****************************************************************************
** Includes
******************************************************************************/

#include "PVRTArray.h"
#include "PVRTString.h"
#include "PVRTError.h"
#include "PVRTTexture.h"
#include "PVRTVector.h"
#include "PVRTSkipGraph.h"
#include "PVRTStringHash.h"

/****************************************************************************
** Helper Funcions
****************************************************************************/
void PVRTPFXCreateStringCopy(char** ppDst, const char* pSrc);

/****************************************************************************
** Enumeration
****************************************************************************/
/*!**************************************************************************
@enum	ESemanticDefaultDataType
@brief  Enum values for the various variable types supported
****************************************************************************/
enum ESemanticDefaultDataType
{
	eDataTypeMat2,
	eDataTypeMat3,
	eDataTypeMat4,
	eDataTypeVec2,
	eDataTypeVec3,
	eDataTypeVec4,
	eDataTypeIvec2,
	eDataTypeIvec3,
	eDataTypeIvec4,
	eDataTypeBvec2,
	eDataTypeBvec3,
	eDataTypeBvec4,
	eDataTypeFloat,
	eDataTypeInt,
	eDataTypeBool,

	eNumDefaultDataTypes,
	eDataTypeNone,

	// Conceptual data types
	eDataTypeRGB,
	eDataTypeRGBA
};

/*!**************************************************************************
@enum   EDefaultDataInternalType
@brief  Enum values for defining whether a variable is float, interger or bool
****************************************************************************/
enum EDefaultDataInternalType
{
	eFloating,
	eInteger,
	eBoolean
};

/*!**************************************************************************
@enum	EPVRTPFXPassType
@brief  Decribes the type of render required
****************************************************************************/
enum EPVRTPFXPassType
{
	eNULL_PASS,
	eCAMERA_PASS,
	ePOSTPROCESS_PASS,
	eENVMAPCUBE_PASS,
	eENVMAPSPH_PASS
};

/*!**************************************************************************
@enum	EPVRTPFXPassType
@brief  Decribes the type of render required
****************************************************************************/
enum EPVRTPFXPassView
{
	eVIEW_CURRENT,			// The scene's active camera is used
	eVIEW_POD_CAMERA,		// The specified camera is used
	eVIEW_NONE				// No specified view
};

/****************************************************************************
** Structures
****************************************************************************/
/*!**************************************************************************
@struct SPVRTPFXParserHeader
@brief  Struct for storing PFX file header data
****************************************************************************/
struct SPVRTPFXParserHeader
{
	CPVRTString			Version;
	CPVRTString			Description;
	CPVRTString			Copyright;
};

/*!**************************************************************************
@struct SPVRTPFXParserTexture
@brief  Struct for storing PFX data from the texture block
****************************************************************************/
struct SPVRTPFXParserTexture
{
	CPVRTStringHash		Name;
	CPVRTStringHash		FileName;
	bool				bRenderToTexture;
	unsigned int		nMin, nMag, nMIP;
	unsigned int		nWrapS, nWrapT, nWrapR;	// either GL_CLAMP or GL_REPEAT
	unsigned int		uiWidth, uiHeight;
	unsigned int		uiFlags;
};

/*!**************************************************************************
@struct SPVRTPFXParserEffectTexture
@brief  Stores effect texture information
****************************************************************************/
struct SPVRTPFXParserEffectTexture
{
	CPVRTStringHash				Name;				// Name of texture.
	unsigned int				nNumber;			// Texture number to set
};

/*!**************************************************************************
@struct SPVRTPFXParserShader
@brief  Struct for storing PFX data from the shader block
****************************************************************************/
struct SPVRTPFXParserShader
{
	CPVRTStringHash			Name;
	bool					bUseFileName;
	char*					pszGLSLfile;
	char*					pszGLSLBinaryFile;
	char*					pszGLSLcode;
	char*					pbGLSLBinary;
	unsigned int			nGLSLBinarySize;
	unsigned int			nFirstLineNumber;	// Line number in the text file where this code began; use to correct line-numbers in compiler errors
	unsigned int			nLastLineNumber;	// The final line number of the GLSL block.

	SPVRTPFXParserShader();
	~SPVRTPFXParserShader();
	SPVRTPFXParserShader(const SPVRTPFXParserShader& rhs);
	SPVRTPFXParserShader& operator=(const SPVRTPFXParserShader& rhs);

	void Copy(const SPVRTPFXParserShader& rhs);
};

/*!**************************************************************************
@struct SPVRTSemanticDefaultDataTypeInfo
@brief  Struct for storing default data types
****************************************************************************/
struct SPVRTSemanticDefaultDataTypeInfo
{
	ESemanticDefaultDataType	eType;
	const char					*pszName;
	unsigned int				nNumberDataItems;
	EDefaultDataInternalType	eInternalType;
};

/*!**************************************************************************
@struct SPVRTSemanticDefaultData
@brief  Stores a default value
****************************************************************************/
struct SPVRTSemanticDefaultData
{
	float						pfData[16];
	int							pnData[4];
	bool						pbData[4];
	ESemanticDefaultDataType	eType;

	SPVRTSemanticDefaultData();
	SPVRTSemanticDefaultData(const SPVRTSemanticDefaultData& rhs);
	SPVRTSemanticDefaultData& operator=(const SPVRTSemanticDefaultData& rhs);

	void Copy(const SPVRTSemanticDefaultData& rhs);
};

/*!**************************************************************************
@struct SPVRTPFXParserSemantic
@brief  Stores semantic information
****************************************************************************/
struct SPVRTPFXParserSemantic
{
	char						*pszName;				/*!< The variable name as used in the shader-language code */
	char						*pszValue;				/*!< For example: LIGHTPOSITION */
	unsigned int				nIdx;					/*!< Index; for example two semantics might be LIGHTPOSITION0 and LIGHTPOSITION1 */
	SPVRTSemanticDefaultData	sDefaultValue;			/*!< Default value */

	SPVRTPFXParserSemantic();
	~SPVRTPFXParserSemantic();
	SPVRTPFXParserSemantic(const SPVRTPFXParserSemantic& rhs);
	SPVRTPFXParserSemantic& operator=(const SPVRTPFXParserSemantic& rhs);

	void Copy(const SPVRTPFXParserSemantic& rhs);
};


struct SPVRTPFXParserEffect;	// Forward declaration
/*!**************************************************************************
@struct SPVRTPFXRenderPass
@brief  Stores render pass information
****************************************************************************/
struct SPVRTPFXRenderPass
{
	EPVRTPFXPassType		eRenderPassType;			// Type of pass.
	EPVRTPFXPassView		eViewType;					// View type.
	PVRTuint32				uiFormatFlags;				// Surface Type.
	SPVRTPFXParserEffect*	pEffect;					// Matched pass. Needed but determined from effect block.
	SPVRTPFXParserTexture*	pTexture;					// The RTT target for this pass.
	CPVRTString				NodeName;					// POD Camera name.
	CPVRTString				SemanticName;				// Name of this pass.
	
	SPVRTPFXRenderPass();
};

/*!**************************************************************************
@struct SPVRTTargetPair
@brief  Stores a buffer type and name for a render target.
****************************************************************************/
struct SPVRTTargetPair
{
	CPVRTString				BufferType;
	CPVRTString				TargetName;
};

/*!**************************************************************************
@struct SPVRTPFXParserEffect
@brief  Stores effect information
****************************************************************************/
struct SPVRTPFXParserEffect
{
	CPVRTStringHash							Name;
	CPVRTString								Annotation;

	CPVRTStringHash							VertexShaderName;
	CPVRTStringHash							FragmentShaderName;

	CPVRTArray<SPVRTPFXParserSemantic>		Uniforms;
	CPVRTArray<SPVRTPFXParserSemantic>		Attributes;
	CPVRTArray<SPVRTPFXParserEffectTexture>	Textures;
	CPVRTArray<SPVRTTargetPair>				Targets;

	SPVRTPFXParserEffect();
};

/****************************************************************************
** Constants
****************************************************************************/
const PVRTuint32 PVRPFXTEX_COLOUR = PVRTuint32(1<<30);
const PVRTuint32 PVRPFXTEX_DEPTH  = PVRTuint32(1<<31);

const static SPVRTSemanticDefaultDataTypeInfo c_psSemanticDefaultDataTypeInfo[] =
{
	{ eDataTypeMat2,		"mat2",			4,		eFloating },
	{ eDataTypeMat3,		"mat3",			9,		eFloating },
	{ eDataTypeMat4,		"mat4",			16,		eFloating },
	{ eDataTypeVec2,		"vec2",			2,		eFloating },
	{ eDataTypeVec3,		"vec3",			3,		eFloating },
	{ eDataTypeVec4,		"vec4",			4,		eFloating },
	{ eDataTypeIvec2,		"ivec2",		2,		eInteger },
	{ eDataTypeIvec3,		"ivec3",		3,		eInteger },
	{ eDataTypeIvec4,		"ivec4",		4,		eInteger },
	{ eDataTypeBvec2,		"bvec2",		2,		eBoolean },
	{ eDataTypeBvec3,		"bvec3",		3,		eBoolean },
	{ eDataTypeBvec4,		"bvec4",		4,		eBoolean },
	{ eDataTypeFloat,		"float",		1,		eFloating },
	{ eDataTypeInt,			"int",			1,		eInteger },
	{ eDataTypeBool,		"bool",			1,		eBoolean },
};


class CPVRTPFXParserReadContext;

/*!**************************************************************************
@class CPVRTPFXParser
@brief PFX parser
****************************************************************************/
class CPVRTPFXParser
{
public:
	/*!***************************************************************************
	@fn      			CPVRTPFXParser
	@brief     		Sets initial values.
	*****************************************************************************/
	CPVRTPFXParser();

	/*!***************************************************************************
	@fn      			~CPVRTPFXParser
	@brief     		Frees memory used.
	*****************************************************************************/
	~CPVRTPFXParser();

	/*!***************************************************************************
	@fn      			ParseFromMemory
	@param[in]				pszScript		PFX script
	@param[out]				pReturnError	error string
	@return				PVR_SUCCESS for success parsing file
						PVR_FAIL if file doesn't exist or is invalid
	@brief     		Parses a PFX script from memory.
	*****************************************************************************/
	EPVRTError ParseFromMemory(const char * const pszScript, CPVRTString * const pReturnError);

	/*!***************************************************************************
	@fn      			ParseFromFile
	@param[in]				pszFileName		PFX file name
	@param[out]				pReturnError	error string
	@return				PVR_SUCCESS for success parsing file
						PVR_FAIL if file doesn't exist or is invalid
	@brief     		Reads the PFX file and calls the parser.
	*****************************************************************************/
	EPVRTError ParseFromFile(const char * const pszFileName, CPVRTString * const pReturnError);

	/*!***************************************************************************
	 @fn      			SetViewportSize
	 @param[in]				uiWidth				New viewport width
	 @param[in]				uiHeight			New viewport height
	 @return			bool				True on success				
	 @brief     		Allows the current viewport size to be set. This value
						is used for calculating relative texture resolutions						
	*****************************************************************************/
	bool SetViewportSize(unsigned int uiWidth, unsigned int uiHeight);

	/*!***************************************************************************
	@fn      		FindTextureIndex
	@param[in]			TextureName		The name of the texture to find
	@param[in]			uiEffect		The effect block to look for the texture in
	@return			Index in to the effect.Texture array.
	@brief     	Returns the index in to the texture array within the effect 
					block where the given texture resides.
	*****************************************************************************/
	unsigned int FindTextureIndex(const CPVRTStringHash& TextureName, unsigned int uiEffect) const;
	
	/*!***************************************************************************
	@fn      			RetrieveRenderPassDependencies
	@param[out]			aRequiredRenderPasses	Dynamic array of required render passes
	@param[in]			aszActiveEffectStrings	Dynamic array containing names of active
												effects in the application
	@return				success of failure
	@brief     		    Takes an array of strings containing the names of active
						effects for this PFX in a given application and then outputs
						an array of the render passes the application needs to perform that is sorted
						into the order they need to be executed (where [0] is the first to be executed,
						and [n] is the last).
						In addition to determining the order of dependent passes
						(such as POSTPROCESS render passes), this function should check if
						CAMERA passes are referenced by active EFFECT blocks and use this information
						to strip redundant passes.
	*****************************************************************************/
	bool RetrieveRenderPassDependencies(CPVRTArray<SPVRTPFXRenderPass*> &aRequiredRenderPasses,
										CPVRTArray<CPVRTStringHash> &aszActiveEffectStrings);

	/*!***************************************************************************
	@brief     	    Returns the number of render passes within this PFX.
	@return			The number of render passes required
	*****************************************************************************/
	unsigned int GetNumberRenderPasses() const;

	/*!***************************************************************************
	@brief     	    Returns the given render pass.
	@param[in]		uiIndex				The render pass index.
	@return			A given render pass.
	*****************************************************************************/
	const SPVRTPFXRenderPass& GetRenderPass(unsigned int uiIndex) const;

	/*!***************************************************************************
	@fn      		GetNumberFragmentShaders
	@return			Number of fragment shaders.
	@brief     	    Returns the number of fragment shaders referenced in the PFX.
	*****************************************************************************/
	unsigned int GetNumberFragmentShaders() const;

	/*!***************************************************************************
	@fn      		GetFragmentShader
	@param[in]		uiIndex		The index of this shader.
	@return			The PFX fragment shader.
	@brief     	    Returns a given fragment shader.
	*****************************************************************************/
	SPVRTPFXParserShader& GetFragmentShader(unsigned int uiIndex);

	/*!***************************************************************************
	@fn      		GetNumberVertexShaders
	@return			Number of vertex shaders.
	@brief     	    Returns the number of vertex shaders referenced in the PFX.
	*****************************************************************************/
	unsigned int GetNumberVertexShaders() const;

	/*!***************************************************************************
	@fn      		GetVertexShader
	@param[in]		uiIndex		The index of this shader.
	@return			The PFX vertex shader.
	@brief     	    Returns a given vertex shader.
	*****************************************************************************/
	SPVRTPFXParserShader& GetVertexShader(unsigned int uiIndex);
 
	/*!***************************************************************************
	@fn      		GetNumberEffects
	@return			Number of effects.
	@brief     	    Returns the number of effects referenced in the PFX.
	*****************************************************************************/
	unsigned int GetNumberEffects() const;

	/*!***************************************************************************
	@fn      		GetEffect
	@param[in]		uiIndex		The index of this effect.
	@return			The PFX effect.
	@brief     	    Returns a given effect.
	*****************************************************************************/
	const SPVRTPFXParserEffect& GetEffect(unsigned int uiIndex) const;

	/*!***************************************************************************
	@fn      		FindEffectByName
	@param[in]		Name		Name of the effect.
	@return			int	
	@brief     	    Returns the index of the given string. Returns -1 on failure.
	*****************************************************************************/
	int FindEffectByName(const CPVRTStringHash& Name) const;

	/*!***************************************************************************
	@fn      		FindTextureByName
	@param[in]		Name		Name of the texture.
	@return			int	
	@brief     	    Returns the index of the given texture. Returns -1 on failure.
	*****************************************************************************/
	int FindTextureByName(const CPVRTStringHash& Name) const;

	/*!***************************************************************************
	@fn      		GetNumberTextures
	@return			Number of effects.
	@brief     	    Returns the number of textures referenced in the PFX.
	*****************************************************************************/
	unsigned int GetNumberTextures() const;

	/*!***************************************************************************
	@fn      		GetTexture
	@param[in]		uiIndex		The index of this texture
	@return			The PFX texture.
	@brief     	    Returns a given texture.
	*****************************************************************************/
	const SPVRTPFXParserTexture* GetTexture(unsigned int uiIndex) const;

	/*!***************************************************************************
	@fn      		GetPFXFileName
	@return			The filename for this PFX file
	@brief     	    eturns the PFX file name associated with this object.
	*****************************************************************************/
	const CPVRTString& GetPFXFileName() const;

	/*!***************************************************************************
	@fn      		GetPostProcessNames
	@return			An array of post process names
	@brief     	    Returns a list of prost process effect names.
	*****************************************************************************/
	const CPVRTArray<CPVRTString>& GetPostProcessNames() const;

public:
	static const unsigned int							VIEWPORT_SIZE;
		
private:	
    SPVRTPFXParserHeader								m_sHeader;

	CPVRTArrayManagedPointers<SPVRTPFXParserTexture>	m_psTexture;
	CPVRTArray<SPVRTPFXParserShader>					m_psFragmentShader;
	CPVRTArray<SPVRTPFXParserShader>					m_psVertexShader;
	CPVRTArray<SPVRTPFXParserEffect>					m_psEffect;
	CPVRTArray<SPVRTPFXRenderPass>						m_RenderPasses;

	CPVRTString											m_szFileName;
	CPVRTPFXParserReadContext*							m_psContext;
	CPVRTArray<CPVRTString>								m_aszPostProcessNames;
	
	unsigned int										m_uiViewportWidth;
	unsigned int										m_uiViewportHeight;
	CPVRTSkipGraphRoot<SPVRTPFXRenderPass*>				m_renderPassSkipGraph;

	/*!***************************************************************************
	@fn      			Parse
	@param[out]			pReturnError	error string
	@return				true for success parsing file
	@brief     		    Parses a loaded PFX file.
	*****************************************************************************/
	bool Parse(	CPVRTString * const pReturnError);

	/*!***************************************************************************
	@fn      			ReduceWhitespace
	@param[out]			line		output text
	@brief     		    Reduces all white space characters in the string to one
						blank space.
	*****************************************************************************/
	void ReduceWhitespace(char *line);

	/*!***************************************************************************
	@fn      			GetEndTag
	@param[in]			pszTagName		tag name
	@param[in]			nStartLine		start line
	@param[out]			pnEndLine		line end tag found
	@return				true if tag found
	@brief     		    Searches for end tag pszTagName from line nStartLine.
						Returns true and outputs the line number of the end tag if
						found, otherwise returning false.
	*****************************************************************************/
	bool GetEndTag(const char *pszTagName, int nStartLine, int *pnEndLine);

	/*!***************************************************************************
	 @brief     		Finds the parameter after the specified delimiting character and
						returns the parameter as a string. An empty string is returned
						if a parameter cannot be found
	 @param[out]		aszSourceString		The string to search
	 @param[in]			parameterTag		The tag to find
	 @param[in]			delimiter			Delimiters
	 @return			Found parameter or empty string
	*****************************************************************************/
	CPVRTString FindParameter(char *aszSourceString, const CPVRTString &parameterTag, const CPVRTString &delimiter);

	/*!***************************************************************************
	 @fn      			ReadStringToken
	 @param[in]			pszSource			Parameter string to process
	 @param[out]		output				Processed string
	 @param[out]		ErrorStr			String containing errors
	 @param[in]			iLine				The line to read
	 @param[in]			pCaller				The caller's name or identifier
	 @return			Returns true on success
	 @brief     		Processes the null terminated char array as if it's a
						formatted string array. Quote marks are determined to be
						start and end of strings. If no quote marks are found the
						string is delimited by whitespace.
	*****************************************************************************/
	bool ReadStringToken(char* pszSource, CPVRTString& output, CPVRTString &ErrorStr, int iLine, const char* pCaller);

	/*!***************************************************************************
	@fn      			ParseHeader
	@param[in]			nStartLine		start line number
	@param[in]			nEndLine		end line number
	@param[out]			pReturnError	error string
	@return				true if parse is successful
	@brief     		    Parses the HEADER section of the PFX file.
	*****************************************************************************/
	bool ParseHeader(int nStartLine, int nEndLine, CPVRTString * const pReturnError);

	/*!***************************************************************************
	@brief     		    Parses the TEXTURES section of the PFX file.
						This style is deprecated but remains for backwards
						compatibility. ** DEPRECATED **
	@param[in]			nStartLine		Start line number
	@param[in]			nEndLine		End line number
	@param[out]			pReturnError	Error string
	@return				true if parse is successful
	*****************************************************************************/
	bool ParseTextures(int nStartLine, int nEndLine, CPVRTString * const pReturnError);

	/*!***************************************************************************
	@fn      			ParseTexture
	@param[in]			nStartLine		start line number
	@param[in]			nEndLine		end line number
	@param[out]			pReturnError	error string
	@return				true if parse is successful
	@brief     		    Parses the TEXTURE section of the PFX file.
	*****************************************************************************/
	bool ParseTexture(int nStartLine, int nEndLine, CPVRTString * const pReturnError);

	/*!***************************************************************************
	@fn      			ParseTarget
	@param[in]			nStartLine		start line number
	@param[in]			nEndLine		end line number
	@param[out]			pReturnError	error string
	@return				true if parse is successful
	@brief     		    Parses the TARGET section of the PFX file.
	*****************************************************************************/
	bool ParseTarget(int nStartLine, int nEndLine, CPVRTString * const pReturnError);

	/*!***************************************************************************
	@fn      			ParseGenericSurface
	@param[in]			nStartLine		start line number
	@param[in]			nEndLine		end line number
	@param[out]			Params			Structure containing PFXTexture parameters
	@param[out]			KnownCmds		An array of unknown commands for the caller
										to check.
	@param[in]			pCaller			The caller's description for error messages.
	@param[out]			pReturnError	error string
	@return				true if parse is successful
	@brief     		    Parses generic data from TARGET and TEXTURE blocks. Namely
						wrapping and filter commands.
	*****************************************************************************/
	bool ParseGenericSurface(int nStartLine, int nEndLine, SPVRTPFXParserTexture& Params, CPVRTArray<CPVRTHash>& KnownCmds, 
							 const char* pCaller, CPVRTString * const pReturnError);

	/*!***************************************************************************
	@fn      			ParseShader
	@param[in]			nStartLine		start line number
	@param[in]			nEndLine		end line number
	@param[out]			pReturnError	error string
	@param[out]			shader			shader data object
	@param[in]			pszBlockName	name of block in PFX file
	@return				true if parse is successful
	@brief     		    Parses the VERTEXSHADER or FRAGMENTSHADER section of the
						PFX file.
	*****************************************************************************/
	bool ParseShader(int nStartLine, int nEndLine, CPVRTString *pReturnError, SPVRTPFXParserShader &shader, const char * const pszBlockName);

	/*!***************************************************************************
	@fn      			ParseSemantic
	@param[out]			semantic		semantic data object
	@param[in]			nStartLine		start line number
	@param[out]			pReturnError	error string
	@return				true if parse is successful
	@brief     		    Parses a semantic.
	*****************************************************************************/
	bool ParseSemantic(SPVRTPFXParserSemantic &semantic, const int nStartLine, CPVRTString * const pReturnError);

	/*!***************************************************************************
	@fn      			ParseEffect
	@param[out]			effect			effect data object
	@param[in]			nStartLine		start line number
	@param[in]			nEndLine		end line number
	@param[out]			pReturnError	error string
	@return				true if parse is successful
	@brief     		    Parses the EFFECT section of the PFX file.
	*****************************************************************************/
	bool ParseEffect(SPVRTPFXParserEffect &effect, const int nStartLine, const int nEndLine, CPVRTString * const pReturnError);

	/*!***************************************************************************
	@fn      		    ParseTextureFlags
	@param[in]			c_pszRemainingLine		Pointer to the remaining string
	@param[out]			ppFlagsOut				Resultant flags set
	@param[in]			uiNumFlags				Number of flags to set
	@param[in]			c_ppszFlagNames			Flag names			
	@param[in]			uiNumFlagNames			Number of flag names
	@param[in]			pReturnError			Return error to set
	@param[in]			iLineNum				The line number for error reporting
	@return			    true if successful
	@brief     	        Parses the texture flag sections.
	*****************************************************************************/
	bool ParseTextureFlags(	const char* c_pszRemainingLine, unsigned int** ppFlagsOut, unsigned int uiNumFlags, const char** c_ppszFlagNames, unsigned int uiNumFlagNames, 
							CPVRTString * const pReturnError, int iLineNum);
	/*!***************************************************************************
	 @brief     	Looks through all of the effects in the .pfx and determines
					the order of render passes that have been declared with
					the RENDER tag (found in [TEXTURES]. 
	 @param[out]	pReturnError
	 @return		True if dependency tree is valid. False if there are errors
					in the dependency tree (e.g. recursion)
	*****************************************************************************/
	bool DetermineRenderPassDependencies(CPVRTString * const pReturnError);

	/*!***************************************************************************
	 @brief     	Recursively look through dependencies until leaf nodes are
					encountered. At this point, add a given leaf node to the
					aRequiredRenderPasses array and return. Repeat this process
					until all dependencies are added to the array.
	 @param[in]		aRequiredRenderPasses
	 @param[in]		renderPassNode
	*****************************************************************************/
	void AddRenderPassNodeDependencies(	CPVRTArray<SPVRTPFXRenderPass*> &aRequiredRenderPasses,
										CPVRTSkipGraphNode<SPVRTPFXRenderPass*> &renderPassNode);
};


#endif /* _PVRTPFXPARSER_H_ */

/*****************************************************************************
 End of file (PVRTPFXParser.h)
*****************************************************************************/

