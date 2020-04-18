/*!****************************************************************************

 @file         OGLES2/PVRTPFXSemantics.h
 @ingroup      API_OGLES2
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief  A list of supported PFX semantics.

******************************************************************************/
#ifndef PVRTPFXSEMANTICS_H
#define PVRTPFXSEMANTICS_H

/*!
 @addtogroup API_OGLES2
 @{
*/

struct SPVRTPFXUniformSemantic;

/****************************************************************************
** Semantic Enumerations
****************************************************************************/


/*!**************************************************************************
 @enum         EPVRTPFXUniformSemantic
 @brief        The default Shaman semantics.
 ***************************************************************************/
enum EPVRTPFXUniformSemantic
{
	ePVRTPFX_UsPOSITION,                /*!< POSITION */
	ePVRTPFX_UsNORMAL,                  /*!< NORMAL */
	ePVRTPFX_UsTANGENT,                 /*!< TANGENT */
	ePVRTPFX_UsBINORMAL,                /*!< BINORMAL */
	ePVRTPFX_UsUV,                      /*!< UV */
	ePVRTPFX_UsVERTEXCOLOR,             /*!< VERTEXCOLOR */
	ePVRTPFX_UsBONEINDEX,               /*!< BONEINDEX */
	ePVRTPFX_UsBONEWEIGHT,              /*!< BONEWEIGHT */

	ePVRTPFX_UsWORLD,                   /*!< WORLD */
	ePVRTPFX_UsWORLDI,                  /*!< WORLDI */
	ePVRTPFX_UsWORLDIT,                 /*!< WORLDIT */
	ePVRTPFX_UsVIEW,                    /*!< VIEW */
	ePVRTPFX_UsVIEWI,                   /*!< VIEWI */
	ePVRTPFX_UsVIEWIT,                  /*!< VIEWIT */
	ePVRTPFX_UsPROJECTION,              /*!< PROJECTION */
	ePVRTPFX_UsPROJECTIONI,             /*!< PROJECTIONI */
	ePVRTPFX_UsPROJECTIONIT,            /*!< PROJECTIONIT */
	ePVRTPFX_UsWORLDVIEW,               /*!< WORLDVIEW */
	ePVRTPFX_UsWORLDVIEWI,              /*!< WORLDVIEWI */
	ePVRTPFX_UsWORLDVIEWIT,             /*!< WORLDVIEWIT */
	ePVRTPFX_UsWORLDVIEWPROJECTION,     /*!< WORLDVIEWPROJECTION */
	ePVRTPFX_UsWORLDVIEWPROJECTIONI,    /*!< WORLDVIEWPROJECTIONI */
	ePVRTPFX_UsWORLDVIEWPROJECTIONIT,   /*!< WORLDVIEWPROJECTIONIT */
	ePVRTPFX_UsVIEWPROJECTION,          /*!< VIEWPROJECTION */
	ePVRTPFX_UsVIEWPROJECTIONI,         /*!< VIEWPROJECTIONI */
	ePVRTPFX_UsVIEWPROJECTIONIT,        /*!< VIEWPROJECTIONIT */
	ePVRTPFX_UsOBJECT,                  /*!< OBJECT */
	ePVRTPFX_UsOBJECTI,                 /*!< OBJECTI */
	ePVRTPFX_UsOBJECTIT,                /*!< OBJECTIT */
	ePVRTPFX_UsUNPACKMATRIX,            /*!< UNPACKMATRIX */

	ePVRTPFX_UsBONECOUNT,               /*!< BONECOUNT */
	ePVRTPFX_UsBONEMATRIXARRAY,         /*!< BONEMATRIXARRAY */
	ePVRTPFX_UsBONEMATRIXARRAYIT,       /*!< BONEMATRIXARRAYIT */

	ePVRTPFX_UsMATERIALOPACITY,         /*!< MATERIALOPACITY */
	ePVRTPFX_UsMATERIALSHININESS,       /*!< MATERIALSHININESS */
	ePVRTPFX_UsMATERIALCOLORAMBIENT,    /*!< MATERIALCOLORAMBIENT */
	ePVRTPFX_UsMATERIALCOLORDIFFUSE,    /*!< MATERIALCOLORDIFFUSE */
	ePVRTPFX_UsMATERIALCOLORSPECULAR,   /*!< MATERIALCOLORSPECULAR */

	ePVRTPFX_UsLIGHTCOLOR,              /*!< LIGHTCOLOR */
	ePVRTPFX_UsLIGHTPOSMODEL,           /*!< LIGHTPOSMODEL */
	ePVRTPFX_UsLIGHTPOSWORLD,           /*!< LIGHTPOSWORLD */
	ePVRTPFX_UsLIGHTPOSEYE,             /*!< LIGHTPOSEYE */
	ePVRTPFX_UsLIGHTDIRMODEL,           /*!< LIGHTDIRMODEL */
	ePVRTPFX_UsLIGHTDIRWORLD,           /*!< LIGHTDIRWORLD */
	ePVRTPFX_UsLIGHTDIREYE,             /*!< LIGHTDIREYE */
	ePVRTPFX_UsLIGHTATTENUATION,        /*!< LIGHTATTENUATION */
	ePVRTPFX_UsLIGHTFALLOFF,            /*!< LIGHTFALLOFF */

	ePVRTPFX_UsEYEPOSMODEL,             /*!< EYEPOSMODEL */
	ePVRTPFX_UsEYEPOSWORLD,             /*!< EYEPOSWORLD */
	ePVRTPFX_UsTEXTURE,                 /*!< TEXTURE */
	ePVRTPFX_UsANIMATION,               /*!< ANIMATION */

	ePVRTPFX_UsVIEWPORTPIXELSIZE,       /*!< VIEWPORTPIXELSIZE */
	ePVRTPFX_UsVIEWPORTCLIPPING,        /*!< VIEWPORTCLIPPING */
	ePVRTPFX_UsTIME,                    /*!< TIME */
	ePVRTPFX_UsTIMECOS,                 /*!< TIMECOS */
	ePVRTPFX_UsTIMESIN,                 /*!< TIMESIN */
	ePVRTPFX_UsTIMETAN,                 /*!< TIMETAN */
	ePVRTPFX_UsTIME2PI,                 /*!< TIME2PI */
	ePVRTPFX_UsTIME2PICOS,              /*!< TIME2PICOS */
	ePVRTPFX_UsTIME2PISIN,              /*!< TIME2PISIN */
	ePVRTPFX_UsTIME2PITAN,              /*!< TIME2PITAN */
	ePVRTPFX_UsRANDOM,                  /*!< RANDOM */

	ePVRTPFX_NumSemantics               /*!< Semantic number */
};

/*!**************************************************************************
 @brief        Retrieves the list of semantics.
 ***************************************************************************/
const SPVRTPFXUniformSemantic* PVRTPFXSemanticsGetSemanticList();

/*! @} */

#endif /* PVRTPFXSEMANTICS_H */

/*****************************************************************************
 End of file (PVRTPFXSemantics.h)
*****************************************************************************/

