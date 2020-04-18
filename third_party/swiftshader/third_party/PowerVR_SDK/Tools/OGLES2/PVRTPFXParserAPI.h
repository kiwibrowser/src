/*!****************************************************************************

 @file         OGLES2/PVRTPFXParserAPI.h
 @ingroup      API_OGLES2
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Declaration of PFX file parser

******************************************************************************/

#ifndef _PVRTPFXPARSERAPI_H_
#define _PVRTPFXPARSERAPI_H_

/*!
 @addtogroup   API_OGLES2
 @{
*/

// Everything been documented in OGL/PVRTPFXParserAPI.h. This would cause documentation duplication in the
// current version of Doxygen.
#ifndef NO_DOXYGEN

#include "../PVRTError.h"

/****************************************************************************
** Structures
****************************************************************************/

/*!**************************************************************************
 @struct        SPVRTPFXUniformSemantic
 @brief         Struct to convert a semantic string to a number.
 @details       The application supplies an array of these so PVRTPFX can translate semantic strings to numbers
****************************************************************************/
struct SPVRTPFXUniformSemantic
{
	const char		*p;	/*!< String containing semantic */
	unsigned int	n;	/*!< Application-defined semantic value */
};

/*!**************************************************************************
 @struct       SPVRTPFXUniform
 @brief        A struct containing GL uniform data.
 @details      PVRTPFX returns an array of these to indicate GL locations & semantics to the application 
 ***************************************************************************/
struct SPVRTPFXUniform
{
	unsigned int	nLocation;		/*!< GL location of the Uniform */
	unsigned int	nSemantic;		/*!< Application-defined semantic value */
	unsigned int	nIdx;			/*!< Index; for example two semantics might be LIGHTPOSITION0 and LIGHTPOSITION1 */
	CPVRTString		sValueName;		/*!< The name of the variable referenced in shader code */
};

/*!**************************************************************************
 @struct       SPVRTPFXTexture
 @brief        A texture data array. 
 @details      An array of these is gained from PVRTPFX so the application can fill in the texture handles
 ***************************************************************************/
struct SPVRTPFXTexture
{
	CPVRTStringHash		Name;	    /*!< texture name */
	GLuint				ui;		    /*!< Loaded texture handle */
	GLuint				unit;	    /*!< The bound texture unit */
	unsigned int		flags;	    /*!< Texture type i.e 2D, Cubemap */
};

/*!**************************************************************************
 @class        PVRTPFXEffectDelegate
 @brief        Receives callbacks for effects.
 ***************************************************************************/
class PVRTPFXEffectDelegate
{
public:
	virtual EPVRTError PVRTPFXOnLoadTexture(const CPVRTStringHash& TextureName, GLuint& uiHandle, unsigned int& uiFlags) = 0; /*!< Returns error if texture could not be loaded */
	virtual ~PVRTPFXEffectDelegate() { }  /*!< Destructor */
};

/*!**************************************************************************
 @class CPVRTPFXEffect
 @brief PFX effect
****************************************************************************/
class CPVRTPFXEffect
{
public:
	/*!***************************************************************************
	@brief      		Sets the context to NULL and initialises the member variables to zero.
	*****************************************************************************/
	CPVRTPFXEffect();

	/*!***************************************************************************
	@brief      		Sets the context and initialises the member variables to zero.
	*****************************************************************************/
	CPVRTPFXEffect(SPVRTContext &sContext);

	/*!***************************************************************************
	@brief      		Calls Destroy().
	*****************************************************************************/
	~CPVRTPFXEffect();

	/*!***************************************************************************
	@brief		        Loads the specified effect from the CPVRTPFXParser object.
						Compiles and links the shaders. Initialises texture data.
	@param[in]			src					PFX Parser Object
	@param[in]			pszEffect			Effect name
	@param[in]			pszFileName			Effect file name
	@param[in]			pDelegate			A delegate which will receive callbacks
	@param[out]			uiUnknownUniforms	Number of unknown uniforms found
	@param[out]			pReturnError		Error string
	@return			    PVR_SUCCESS if load succeeded
	*****************************************************************************/
	EPVRTError Load(CPVRTPFXParser &src, const char * const pszEffect, const char * const pszFileName, 
					PVRTPFXEffectDelegate* pDelegate, unsigned int& uiUnknownUniforms, CPVRTString *pReturnError);

	/*!***************************************************************************
	@brief		        Deletes the gl program object and texture data.
	*****************************************************************************/
	void Destroy();

	/*!***************************************************************************
	@brief		        Selects the gl program object and binds the textures.
						If the render target texture for the current render pass is required
						in this effect (and therefore cannot be sampled),
						load the replacement texture instead.
	@param[in]			i32RenderTextureId		The ID of the render target of the current task
	@param[in]			ui32ReplacementTexture	The ID of the texture that should be used instead
	@return			    EPVRTError				PVR_SUCCESS if activate succeeded
	*****************************************************************************/
	EPVRTError Activate(const int i32RenderTextureId=-1, const unsigned int ui32ReplacementTexture=0);

	/*!***************************************************************************
	@brief		        Gets the texture data array.
	@return			    SPVRTPFXTexture*		pointer to the texture data array
	*****************************************************************************/
	const CPVRTArray<SPVRTPFXTexture>& GetTextureArray() const;

	/*!***************************************************************************
	@brief	            Returns a list of known semantics.
	@return			    const CPVRTArray<SPVRTPFXUniform>&	
	*****************************************************************************/
	const CPVRTArray<SPVRTPFXUniform>& GetUniformArray() const;

	/*!***************************************************************************
	@brief	            Gets the array of registered semantics which will be used to
                        match PFX code.
	@return			    const CPVRTArray<SPVRTPFXUniformSemantic>&	
	*****************************************************************************/
	const CPVRTArray<SPVRTPFXUniformSemantic>& GetSemanticArray() const;

	/*!***************************************************************************
	@brief		        Sets the textrue and applys the filtering.
	@param[in]			nIdx				texture number
	@param[in]			ui					opengl texture handle
	@param[in]			u32flags			texture flags
	*****************************************************************************/
	void SetTexture(const unsigned int nIdx, const GLuint ui, const unsigned int u32flags=0);

	/*!***************************************************************************
	@brief		        Sets the dafault value for the uniform semantic.
	@param[in]			pszName				name of uniform
	@param[in]			psDefaultValue      pointer to default value
	*****************************************************************************/
	void SetDefaultUniformValue(const char *const pszName, const SPVRTSemanticDefaultData *psDefaultValue);

	/*!***************************************************************************
	@brief	            Registers a user-provided uniform semantic.
	@param[in]			psUniforms			Array of semantics to register
	@param[in]			uiNumUniforms		Number provided
	@param[out]			pReturnError		Human-readable error if any
	@return			    PVR_SUCCESS on success	
	*****************************************************************************/
	EPVRTError RegisterUniformSemantic(const SPVRTPFXUniformSemantic* const psUniforms, unsigned int uiNumUniforms, CPVRTString* pReturnError);

	/*!***************************************************************************
	@brief	            Removes a given semantic ID from the 'known' semantic list and 
                        re-parses the effect to update the uniform table.
	@param[in]			uiSemanticID
	@param[out]			pReturnError
	@return			    PVR_SUCCESS on success	
	*****************************************************************************/
	EPVRTError RemoveUniformSemantic(unsigned int uiSemanticID, CPVRTString* pReturnError);

	/*!***************************************************************************
	 @brief		        Sets the context for this effect.
	 @param[in]			pContext			context pointer
	 *****************************************************************************/
	void SetContext(SPVRTContext * const pContext);
	
	/*!***************************************************************************
	@brief	            Returns the OGL program handle.
	@return			    unsigned int
	*****************************************************************************/
	unsigned int GetProgramHandle() const;

	/*!***************************************************************************
	@brief	            Gets the active effect index within the PFX file.
	@return			    unsigned int	
	*****************************************************************************/
	unsigned int GetEffectIndex() const;

private:
	/*!***************************************************************************
	@brief	            Loads all of the GLSL shaders for an effect.
	@param[in]			pszFileName
	@param[out]			pReturnError
	@return			    EPVRTError	
	*****************************************************************************/
	EPVRTError LoadShadersForEffect(CPVRTPFXParser &src, const char * const pszFileName, CPVRTString *pReturnError);

	/*!***************************************************************************
	@brief	            Loads all of the textures for this effect.
	@param[out]			pReturnError
	@return			    EPVRTError	
	*****************************************************************************/
	EPVRTError LoadTexturesForEffect(PVRTPFXEffectDelegate* pDelegate, CPVRTString *pReturnError);

	/*!***************************************************************************
	@brief	            Builds the uniform table from a list of known semantics.
	@param[out]			uiUnknownSemantics
	@param[out]			pReturnError
	@return			    EPVRTError	
	*****************************************************************************/
	EPVRTError RebuildUniformTable(unsigned int& uiUnknownSemantics, CPVRTString* pReturnError);

protected:
	bool									m_bLoaded;
	SPVRTContext*							m_psContext;
	CPVRTPFXParser*							m_pParser;
	unsigned int							m_nEffect;

	GLuint									m_uiProgram;		// Loaded program

	CPVRTArray<SPVRTPFXTexture>				m_Textures;			// Array of loaded textures
	CPVRTArray<SPVRTPFXUniform>				m_Uniforms;			// Array of found uniforms

	CPVRTArray<SPVRTPFXUniformSemantic>		m_Semantics;		// An array of registered semantics.
};

/****************************************************************************
** Auxiliary functions
****************************************************************************/

/*!**************************************************************************
 @brief                 'Equivalent to' operator
 @param[in]             lhs     First SPVRTPFXUniformSemantic
 @param[in]             rhs     Second SPVRTPFXUniformSemantic
 @return                True if the numbers in the two SPVRTPFXUniformSemantics are equivalent.
****************************************************************************/
inline bool operator==(const SPVRTPFXUniformSemantic& lhs, const SPVRTPFXUniformSemantic& rhs)
{
	return (lhs.n == rhs.n);
}

 #endif
 
/*! @} */

#endif /* _PVRTPFXPARSERAPI_H_ */

/*****************************************************************************
 End of file (PVRTPFXParserAPI.h)
*****************************************************************************/

