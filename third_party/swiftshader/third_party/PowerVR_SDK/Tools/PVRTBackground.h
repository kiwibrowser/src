/*!****************************************************************************

 @file         PVRTBackground.h
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Function to draw a background texture.

******************************************************************************/
#ifndef __PVRTBACKGROUND_H__
#define __PVRTBACKGROUND_H__

#include "PVRTGlobal.h"
#include "PVRTContext.h"
#include "PVRTString.h"
#include "PVRTError.h"

/****************************************************************************
** Structures
****************************************************************************/
/*!***************************************************************************
 @struct SPVRTBackgroundAPI
 @brief A struct for storing API specific variables
*****************************************************************************/
struct SPVRTBackgroundAPI;

/*!***************************************************************************
 @class CPVRTBackground
 @brief A class for drawing a fullscreen textured background
*****************************************************************************/
class CPVRTBackground
{
	public:
		/*!***************************************************************************
	 	 @brief		Initialise some values.
		*****************************************************************************/
		CPVRTBackground(void);
		/*!***************************************************************************
		 @brief		Calls Destroy()
		*****************************************************************************/
		~CPVRTBackground(void);
		/*!***************************************************************************
		 @brief 	Destroys the background and releases API specific resources
		*****************************************************************************/
		void Destroy();

		/*!***************************************************************************
		 @brief      	Initialises the background
		 @param[in]		pContext	A pointer to a PVRTContext
		 @param[in]		bRotate		true to rotate texture 90 degrees.
		 @param[in]		pszError	An option string for returning errors
		 @return  		PVR_SUCCESS on success
		*****************************************************************************/
		EPVRTError Init(const SPVRTContext * const pContext, const bool bRotate, CPVRTString *pszError = 0);

#if defined(BUILD_OGL) || defined(BUILD_OGLES) || defined(BUILD_OGLES2) || defined(BUILD_OGLES3)
		/*!***************************************************************************
		 @brief      	Draws a texture on a quad covering the whole screen.
		 @param[in]		ui32Texture	Texture to use
		 @return  		PVR_SUCCESS on success
		*****************************************************************************/
		EPVRTError Draw(const GLuint ui32Texture);
#elif defined(BUILD_DX11)
		/*!***************************************************************************
		 @brief      	Draws a texture on a quad covering the whole screen.
		 @param[in]		pTexture	Texture to use
		 @return  		PVR_SUCCESS on success
		*****************************************************************************/
		EPVRTError Draw(ID3D11ShaderResourceView *pTexture);
#endif

	protected:
		bool m_bInit;
		SPVRTBackgroundAPI *m_pAPI;
};


#endif /* __PVRTBACKGROUND_H__ */

/*****************************************************************************
 End of file (PVRTBackground.h)
*****************************************************************************/

