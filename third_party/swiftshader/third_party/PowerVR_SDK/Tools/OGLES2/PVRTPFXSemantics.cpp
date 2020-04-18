/******************************************************************************

 @File         OGLES2/PVRTPFXSemantics.cpp

 @Title        PFX Semantics

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     All

 @Description  A list of supported PFX semantics.

******************************************************************************/

/*****************************************************************************
** Includes
******************************************************************************/
#include "PVRTGlobal.h"
#include "PVRTContext.h"
#include "PVRTArray.h"
#include "PVRTString.h"
#include "PVRTStringHash.h"
#include "PVRTPFXParser.h"
#include "PVRTPFXParserAPI.h"
#include "PVRTPFXSemantics.h"

/*!***************************************************************************
** Default Shaman semantics
*****************************************************************************/
const SPVRTPFXUniformSemantic c_psSemanticsInfo[] =
{
	{ "POSITION",				ePVRTPFX_UsPOSITION					},
	{ "NORMAL",					ePVRTPFX_UsNORMAL					},
	{ "TANGENT",				ePVRTPFX_UsTANGENT 					},		
	{ "BINORMAL",				ePVRTPFX_UsBINORMAL 				},
	{ "UV",						ePVRTPFX_UsUV						},
	{ "VERTEXCOLOR",			ePVRTPFX_UsVERTEXCOLOR				},
	{ "BONEINDEX",				ePVRTPFX_UsBONEINDEX				},
	{ "BONEWEIGHT",				ePVRTPFX_UsBONEWEIGHT				},

	{ "WORLD",					ePVRTPFX_UsWORLD					},
	{ "WORLDI",					ePVRTPFX_UsWORLDI 					},
	{ "WORLDIT",				ePVRTPFX_UsWORLDIT					},
	{ "VIEW",					ePVRTPFX_UsVIEW 					},
	{ "VIEWI",					ePVRTPFX_UsVIEWI 					},
	{ "VIEWIT",					ePVRTPFX_UsVIEWIT					},
	{ "PROJECTION",				ePVRTPFX_UsPROJECTION				},
	{ "PROJECTIONI",			ePVRTPFX_UsPROJECTIONI				},
	{ "PROJECTIONIT",			ePVRTPFX_UsPROJECTIONIT				},
	{ "WORLDVIEW",				ePVRTPFX_UsWORLDVIEW				},
	{ "WORLDVIEWI",				ePVRTPFX_UsWORLDVIEWI				},
	{ "WORLDVIEWIT",			ePVRTPFX_UsWORLDVIEWIT				},
	{ "WORLDVIEWPROJECTION",	ePVRTPFX_UsWORLDVIEWPROJECTION		},
	{ "WORLDVIEWPROJECTIONI",	ePVRTPFX_UsWORLDVIEWPROJECTIONI		},
	{ "WORLDVIEWPROJECTIONIT",	ePVRTPFX_UsWORLDVIEWPROJECTIONIT	},
	{ "UNPACKMATRIX",			ePVRTPFX_UsUNPACKMATRIX				},

	{ "VIEWPROJECTION",			ePVRTPFX_UsVIEWPROJECTION			},
	{ "VIEWPROJECTIONI",		ePVRTPFX_UsVIEWPROJECTIONI			},
	{ "VIEWPROJECTIONIT",		ePVRTPFX_UsVIEWPROJECTIONIT			},
	{ "OBJECT",					ePVRTPFX_UsOBJECT,					},
	{ "OBJECTI",				ePVRTPFX_UsOBJECTI,					},
	{ "OBJECTIT",				ePVRTPFX_UsOBJECTIT,				},

	{ "MATERIALOPACITY",		ePVRTPFX_UsMATERIALOPACITY			},
	{ "MATERIALSHININESS",		ePVRTPFX_UsMATERIALSHININESS		},
	{ "MATERIALCOLORAMBIENT",	ePVRTPFX_UsMATERIALCOLORAMBIENT		},
	{ "MATERIALCOLORDIFFUSE",	ePVRTPFX_UsMATERIALCOLORDIFFUSE 	},
	{ "MATERIALCOLORSPECULAR",	ePVRTPFX_UsMATERIALCOLORSPECULAR	},

	{ "BONECOUNT",				ePVRTPFX_UsBONECOUNT				},
	{ "BONEMATRIXARRAY",		ePVRTPFX_UsBONEMATRIXARRAY			},
	{ "BONEMATRIXARRAYIT",		ePVRTPFX_UsBONEMATRIXARRAYIT		},

	{ "LIGHTCOLOR",				ePVRTPFX_UsLIGHTCOLOR				},
	{ "LIGHTPOSMODEL",			ePVRTPFX_UsLIGHTPOSMODEL 			},
	{ "LIGHTPOSWORLD",			ePVRTPFX_UsLIGHTPOSWORLD			},
	{ "LIGHTPOSEYE",			ePVRTPFX_UsLIGHTPOSEYE				},
	{ "LIGHTDIRMODEL",			ePVRTPFX_UsLIGHTDIRMODEL			},
	{ "LIGHTDIRWORLD",			ePVRTPFX_UsLIGHTDIRWORLD			},
	{ "LIGHTDIREYE",			ePVRTPFX_UsLIGHTDIREYE				},
	{ "LIGHTATTENUATION",		ePVRTPFX_UsLIGHTATTENUATION			},
	{ "LIGHTFALLOFF",			ePVRTPFX_UsLIGHTFALLOFF				},

	{ "EYEPOSMODEL",			ePVRTPFX_UsEYEPOSMODEL				},
	{ "EYEPOSWORLD",			ePVRTPFX_UsEYEPOSWORLD				},
	{ "TEXTURE",				ePVRTPFX_UsTEXTURE					},
	{ "ANIMATION",				ePVRTPFX_UsANIMATION				},
	
	{ "VIEWPORTPIXELSIZE",		ePVRTPFX_UsVIEWPORTPIXELSIZE		},
	{ "VIEWPORTCLIPPING",		ePVRTPFX_UsVIEWPORTCLIPPING			},
	{ "TIME",					ePVRTPFX_UsTIME						},
	{ "TIMECOS",				ePVRTPFX_UsTIMECOS					},
	{ "TIMESIN",				ePVRTPFX_UsTIMESIN					},
	{ "TIMETAN",				ePVRTPFX_UsTIMETAN,					},
	{ "TIME2PI",				ePVRTPFX_UsTIME2PI,					},
	{ "TIME2PICOS",				ePVRTPFX_UsTIME2PICOS,				},
	{ "TIME2PISIN",				ePVRTPFX_UsTIME2PISIN,				},
	{ "TIME2PITAN",				ePVRTPFX_UsTIME2PITAN,				},
	{ "RANDOM",					ePVRTPFX_UsRANDOM,					},
};
PVRTCOMPILEASSERT(c_psSemanticsInfo, sizeof(c_psSemanticsInfo) / sizeof(c_psSemanticsInfo[0]) == ePVRTPFX_NumSemantics);

const SPVRTPFXUniformSemantic* PVRTPFXSemanticsGetSemanticList()
{
	return c_psSemanticsInfo;
}

/*****************************************************************************
 End of file (PVRTPFXSemantics.cpp)
*****************************************************************************/

