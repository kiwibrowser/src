/******************************************************************************

 @file         Shell/PVRShell.cpp
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Makes programming for 3D APIs easier by wrapping surface
               initialization, Texture allocation and other functions for use by a demo.

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "PVRShell.h"
#include "PVRShellOS.h"
#include "PVRShellAPI.h"
#include "PVRShellImpl.h"

/*! This file simply defines a version string. It can be commented out. */
#include "sdkver.h"
#ifndef PVRSDK_VERSION
#define PVRSDK_VERSION "n.nn.nn.nnnn"
#endif

/*! Define to automatically stop the app after x frames. If negative, run forever. */
#ifndef PVRSHELL_QUIT_AFTER_FRAME
#define PVRSHELL_QUIT_AFTER_FRAME -1
#endif

/*! Define to automatically stop the app after x amount of seconds. If negative, run forever. */
#ifndef PVRSHELL_QUIT_AFTER_TIME
#define PVRSHELL_QUIT_AFTER_TIME -1
#endif

/*! Define for the screen shot file name. */
#define PVRSHELL_SCREENSHOT_NAME	"PVRShell"

#if defined(_WIN32)
#define snprintf _snprintf
#endif

// No Doxygen for CPP files, due to documentation duplication
/// @cond NO_DOXYGEN

// Define DISABLE_SWIPE_MAPPING to disable the PVRShell's simple mapping of swipes to key commands.
//#define DISABLE_SWIPE_MAPPING 1
/*****************************************************************************
** Prototypes
*****************************************************************************/
static bool StringCopy(char *&pszStr, const char * const pszSrc);

/****************************************************************************
** Class: PVRShell
****************************************************************************/

/*!***********************************************************************
 @brief		Constructor
*************************************************************************/
PVRShell::PVRShell()
{
	m_pShellInit = NULL;
	m_pShellData = new PVRShellData;

	m_pShellData->nShellPosX=0;
	m_pShellData->nShellPosY=0;

	m_pShellData->bFullScreen = false;	// note this may be overridden by some OS versions of PVRShell

	m_pShellData->nAASamples= 0;
	m_pShellData->nColorBPP = 0;
	m_pShellData->nDepthBPP = 0;

	m_pShellData->nDieAfterFrames = PVRSHELL_QUIT_AFTER_FRAME;
	m_pShellData->fDieAfterTime = PVRSHELL_QUIT_AFTER_TIME;

	m_pShellData->bNeedPbuffer = false;
	m_pShellData->bNeedPixmap = false;
	m_pShellData->bNeedPixmapDisableCopy = false;
	m_pShellData->bNeedZbuffer = true;
	m_pShellData->bLockableBackBuffer = false;
	m_pShellData->bSoftwareRender = false;
	m_pShellData->bNeedStencilBuffer = false;

	m_pShellData->bNeedAlphaFormatPre = false;
	m_pShellData->bUsingPowerSaving = true;
	m_pShellData->bOutputInfo = false;
	m_pShellData->bNoShellSwapBuffer = false;

	m_pShellData->pszAppName = 0;
	m_pShellData->pszExitMessage = 0;

	m_pShellData->nSwapInterval = 1;
	m_pShellData->nInitRepeats = 0;

	m_pShellData->nCaptureFrameStart = -1;
	m_pShellData->nCaptureFrameStop  = -1;
	m_pShellData->nCaptureFrameScale = 1;

	m_pShellData->nPriority = 2;

	m_pShellData->bForceFrameTime = false;
	m_pShellData->nFrameTime = 33;

	// Internal Data
	m_pShellData->bShellPosWasDefault = true;
	m_pShellData->nShellCurFrameNum = 0;
#ifdef PVRSHELL_FPS_OUTPUT
	m_pShellData->bOutputFPS = false;
#endif
	m_pShellData->bDiscardFrameColor=false;
	m_pShellData->bDiscardFrameDepth=true;
	m_pShellData->bDiscardFrameStencil=true;
}

/*!***********************************************************************
 @brief		Destructor
*************************************************************************/
PVRShell::~PVRShell()
{
	delete m_pShellData;
	m_pShellData = NULL;
}

// Allow user to set preferences from within InitApplication

/*!***********************************************************************
 @brief     This function is used to pass preferences to the PVRShell.
            If used, this function must be called from InitApplication().
 @param[in] prefName    Name of preference to set to value
 @param[in] value       Value
 @return    true for success
*************************************************************************/

bool PVRShell::PVRShellSet(const prefNameBoolEnum prefName, const bool value)
{
	switch(prefName)
	{
	case prefFullScreen:
		m_pShellData->bFullScreen = value;
		return true;

	case prefPBufferContext:
		m_pShellData->bNeedPbuffer = value;
		return true;

	case prefPixmapContext:
		m_pShellData->bNeedPixmap = value;
		return true;

	case prefPixmapDisableCopy:
		m_pShellData->bNeedPixmapDisableCopy = value;
		return true;

	case prefZbufferContext:
		m_pShellData->bNeedZbuffer = value;
		return true;

	case prefLockableBackBuffer:
		m_pShellData->bLockableBackBuffer = value;
		return true;

	case prefSoftwareRendering:
		m_pShellData->bSoftwareRender = value;
		return true;

	case prefStencilBufferContext:
		m_pShellData->bNeedStencilBuffer = value;
		return true;

	case prefAlphaFormatPre:
		m_pShellData->bNeedAlphaFormatPre = value;
		return true;

	case prefPowerSaving:
		m_pShellData->bUsingPowerSaving = value;
		return true;

	case prefOutputInfo:
		m_pShellData->bOutputInfo = value;
		return true;

	case prefNoShellSwapBuffer:
		m_pShellData->bNoShellSwapBuffer = value;
		return true;

	case prefForceFrameTime:
		m_pShellData->bForceFrameTime = value;
		return true;

#ifdef PVRSHELL_FPS_OUTPUT
	case prefOutputFPS:
		m_pShellData->bOutputFPS = value;
		return true;
#endif

	case prefDiscardColor: 
		m_pShellData->bDiscardFrameColor = value;
		return true;
	case prefDiscardDepth: 
		m_pShellData->bDiscardFrameDepth = value;
		return true;
	case prefDiscardStencil: 
		m_pShellData->bDiscardFrameStencil = value;
		return true;
	default:
		return m_pShellInit->OsSet(prefName, value);
	}
}

/*!***********************************************************************
 @brief      This function is used to get parameters from the PVRShell.
             It can be called from anywhere in the program.
 @param[in]  prefName    Name of preference to set to value
 @return     The requested value.
*************************************************************************/

bool PVRShell::PVRShellGet(const prefNameBoolEnum prefName) const
{
	switch(prefName)
	{
	case prefFullScreen:	return m_pShellData->bFullScreen;
	case prefIsRotated:	return (m_pShellData->nShellDimY > m_pShellData->nShellDimX);
	case prefPBufferContext:	return m_pShellData->bNeedPbuffer;
	case prefPixmapContext:	return m_pShellData->bNeedPixmap;
	case prefPixmapDisableCopy:	return m_pShellData->bNeedPixmapDisableCopy;
	case prefZbufferContext:	return m_pShellData->bNeedZbuffer;
	case prefLockableBackBuffer:	return m_pShellData->bLockableBackBuffer;
	case prefSoftwareRendering:	return m_pShellData->bSoftwareRender;
	case prefNoShellSwapBuffer: return m_pShellData->bNoShellSwapBuffer;
	case prefStencilBufferContext:	return m_pShellData->bNeedStencilBuffer;
	case prefAlphaFormatPre: return m_pShellData->bNeedAlphaFormatPre;
	case prefPowerSaving: return m_pShellData->bUsingPowerSaving;
	case prefOutputInfo:	return m_pShellData->bOutputInfo;
	case prefForceFrameTime: return m_pShellData->bForceFrameTime;
#ifdef PVRSHELL_FPS_OUTPUT
	case prefOutputFPS: return m_pShellData->bOutputFPS;
#endif
	case prefDiscardColor: return m_pShellData->bDiscardFrameColor;
	case prefDiscardDepth: return m_pShellData->bDiscardFrameDepth;
	case prefDiscardStencil: return m_pShellData->bDiscardFrameStencil;
	default:	return false;
	}
}

/*!***********************************************************************
 @brief     This function is used to pass preferences to the PVRShell.
            If used, this function must be called from InitApplication().
 @param[in] prefName    Name of preference to set to value
 @param[in] value       Value
 @return    true for success
*************************************************************************/

bool PVRShell::PVRShellSet(const prefNameFloatEnum prefName, const float value)
{
	switch(prefName)
	{
	case prefQuitAfterTime:
		m_pShellData->fDieAfterTime = value;
		return true;

	default:
		break;
	}
	return false;
}

/*!***********************************************************************
 @brief      This function is used to get parameters from the PVRShell.
             It can be called from anywhere in the program.
 @param[in]  prefName    Name of preference to set to value
 @return     The requested value.
*************************************************************************/
float PVRShell::PVRShellGet(const prefNameFloatEnum prefName) const
{
	switch(prefName)
	{
	case prefQuitAfterTime:	return m_pShellData->fDieAfterTime;
	default:	return -1;
	}
}

/*!***********************************************************************
 @brief     This function is used to pass preferences to the PVRShell.
            If used, this function must be called from InitApplication().
 @param[in] prefName    Name of preference to set to value
 @param[in] value       Value
 @return    true for success
*************************************************************************/
bool PVRShell::PVRShellSet(const prefNameIntEnum prefName, const int value)
{
	switch(prefName)
	{
	case prefWidth:
		if(value > 0)
		{
			m_pShellData->nShellDimX = value;
			return true;
		}
		return false;

	case prefHeight:
		if(value > 0)
		{
			m_pShellData->nShellDimY = value;
			return true;
		}
		return false;

	case prefPositionX:
		m_pShellData->bShellPosWasDefault = false;
		m_pShellData->nShellPosX = value;
		return true;

	case prefPositionY:
		m_pShellData->bShellPosWasDefault = false;
		m_pShellData->nShellPosY = value;
		return true;

	case prefQuitAfterFrame:
		m_pShellData->nDieAfterFrames = value;
		return true;

	case prefInitRepeats:
		m_pShellData->nInitRepeats = value;
		return true;

	case prefAASamples:
		if(value >= 0)
		{
			m_pShellData->nAASamples = value;
			return true;
		}
		return false;

	case prefColorBPP:
		if(value >= 0)
		{
			m_pShellData->nColorBPP = value;
			return true;
		}
		return false;

	case prefDepthBPP:
		if(value >= 0)
		{
			m_pShellData->nDepthBPP = value;
			return true;
		}
		return false;

	case prefRotateKeys:
		{
			switch((PVRShellKeyRotate)value)
			{
			case PVRShellKeyRotateNone:
				m_pShellInit->m_eKeyMapUP = PVRShellKeyNameUP;
				m_pShellInit->m_eKeyMapLEFT = PVRShellKeyNameLEFT;
				m_pShellInit->m_eKeyMapDOWN = PVRShellKeyNameDOWN;
				m_pShellInit->m_eKeyMapRIGHT = PVRShellKeyNameRIGHT;
				break;
			case PVRShellKeyRotate90:
				m_pShellInit->m_eKeyMapUP = PVRShellKeyNameLEFT;
				m_pShellInit->m_eKeyMapLEFT = PVRShellKeyNameDOWN;
				m_pShellInit->m_eKeyMapDOWN = PVRShellKeyNameRIGHT;
				m_pShellInit->m_eKeyMapRIGHT = PVRShellKeyNameUP;
				break;
			case PVRShellKeyRotate180:
				m_pShellInit->m_eKeyMapUP = PVRShellKeyNameDOWN;
				m_pShellInit->m_eKeyMapLEFT = PVRShellKeyNameRIGHT;
				m_pShellInit->m_eKeyMapDOWN = PVRShellKeyNameUP;
				m_pShellInit->m_eKeyMapRIGHT = PVRShellKeyNameLEFT;
				break;
			case PVRShellKeyRotate270:
				m_pShellInit->m_eKeyMapUP = PVRShellKeyNameRIGHT;
				m_pShellInit->m_eKeyMapLEFT = PVRShellKeyNameUP;
				m_pShellInit->m_eKeyMapDOWN = PVRShellKeyNameLEFT;
				m_pShellInit->m_eKeyMapRIGHT = PVRShellKeyNameDOWN;
				break;
			default:
				return false;
			}
		}
			return true;
	case prefCaptureFrameStart:
		m_pShellData->nCaptureFrameStart = value;
		return true;
	case prefCaptureFrameStop:
		m_pShellData->nCaptureFrameStop  = value;
		return true;
	case prefCaptureFrameScale:
		m_pShellData->nCaptureFrameScale  = value;
		return true;
	case prefFrameTimeValue:
		m_pShellData->nFrameTime = value;
		return true;
	default:
		{
			if(m_pShellInit->ApiSet(prefName, value))
				return true;

			return m_pShellInit->OsSet(prefName, value);
		}
	}
}

/*!***********************************************************************
 @brief      This function is used to get parameters from the PVRShell.
             It can be called from anywhere in the program.
 @param[in]  prefName    Name of preference to set to value
 @return     The requested value.
*************************************************************************/
int PVRShell::PVRShellGet(const prefNameIntEnum prefName) const
{
	switch(prefName)
	{
	case prefWidth:	return m_pShellData->nShellDimX;
	case prefHeight:	return m_pShellData->nShellDimY;
	case prefPositionX:	return m_pShellData->nShellPosX;
	case prefPositionY:	return m_pShellData->nShellPosY;
	case prefQuitAfterFrame:	return m_pShellData->nDieAfterFrames;
	case prefSwapInterval:	return m_pShellData->nSwapInterval;
	case prefInitRepeats:	return m_pShellData->nInitRepeats;
	case prefAASamples:	return m_pShellData->nAASamples;
	case prefCommandLineOptNum:	return m_pShellInit->m_CommandLine.m_nOptLen;
	case prefColorBPP: return m_pShellData->nColorBPP;
	case prefDepthBPP: return m_pShellData->nDepthBPP;
	case prefCaptureFrameStart: return m_pShellData->nCaptureFrameStart;
	case prefCaptureFrameStop: return m_pShellData->nCaptureFrameStop;
	case prefCaptureFrameScale: return m_pShellData->nCaptureFrameScale;
	case prefFrameTimeValue: return m_pShellData->nFrameTime;
	case prefPriority: return m_pShellData->nPriority;
	default:
		{
			int n;

			if(m_pShellInit->ApiGet(prefName, &n))
				return n;
			if(m_pShellInit->OsGet(prefName, &n))
				return n;
			return -1;
		}
	}
}

/*!***********************************************************************
 @brief     This function is used to pass preferences to the PVRShell.
            If used, this function must be called from InitApplication().
 @param[in] prefName    Name of preference to set to value
 @param[in] value       Value
 @return    true for success
*************************************************************************/
bool PVRShell::PVRShellSet(const prefNamePtrEnum prefName, const void * const ptrValue)
{
    PVRSHELL_UNREFERENCED_PARAMETER(prefName);
    PVRSHELL_UNREFERENCED_PARAMETER(ptrValue);
	return false;
}

/*!***********************************************************************
 @brief      This function is used to get parameters from the PVRShell.
             It can be called from anywhere in the program.
 @param[in]  prefName    Name of preference to set to value
 @return     The requested value.
*************************************************************************/
void *PVRShell::PVRShellGet(const prefNamePtrEnum prefName) const
{
	switch(prefName)
	{
	case prefNativeWindowType:	return m_pShellInit->OsGetNativeWindowType();
	case prefPointerLocation:
		if (m_pShellInit->m_bTouching)
			return m_pShellInit->m_vec2PointerLocation;
	break;
	default:
		{
			void *p;

			if(m_pShellInit->ApiGet(prefName, &p))
				return p;
			if(m_pShellInit->OsGet(prefName, &p))
				return p;
		}
	}
	return NULL;
}

/*!***********************************************************************
 @brief     This function is used to pass preferences to the PVRShell.
            If used, this function must be called from InitApplication().
 @param[in] prefName    Name of preference to set to value
 @param[in] value       Value
 @return    true for success
*************************************************************************/
bool PVRShell::PVRShellSet(const prefNameConstPtrEnum prefName, const void * const ptrValue)
{
	switch(prefName)
	{
	case prefAppName:
		StringCopy(m_pShellData->pszAppName, (char*)ptrValue);
		return true;
	case prefExitMessage:
		StringCopy(m_pShellData->pszExitMessage, (char*)ptrValue);
		PVRShellOutputDebug("Exit message has been set to: \"%s\".\n", ptrValue);
		return true;
	default:
		break;
	}
	return false;
}

/*!***********************************************************************
 @brief      This function is used to get parameters from the PVRShell.
             It can be called from anywhere in the program.
 @param[in]  prefName    Name of preference to set to value
 @return     The requested value.
*************************************************************************/
const void *PVRShell::PVRShellGet(const prefNameConstPtrEnum prefName) const
{
	switch(prefName)
	{
	case prefAppName:
		return m_pShellData->pszAppName;
	case prefExitMessage:
		return m_pShellData->pszExitMessage;
	case prefReadPath:
		return m_pShellInit->GetReadPath();
	case prefWritePath:
		return m_pShellInit->GetWritePath();
	case prefCommandLine:
		return m_pShellInit->m_CommandLine.m_psOrig;
	case prefCommandLineOpts:
		return m_pShellInit->m_CommandLine.m_pOpt;
	case prefVersion:
		return PVRSDK_VERSION;
	default:
		return 0;
	}
}

/*!***********************************************************************
 @brief	     It will be stored as 24-bit per pixel, 8-bit per chanel RGB. 
             The memory should be freed with free() when no longer needed.
 @param[in]	 Width		size of image to capture (relative to 0,0)
 @param[in]	 Height		size of image to capture (relative to 0,0)
 @param[out] pLines		receives a pointer to an area of memory containing the screen buffer.
 @return	 true for success
*************************************************************************/
bool PVRShell::PVRShellScreenCaptureBuffer(const int Width, const int Height, unsigned char **pLines)
{
	/* Allocate memory for line */
	*pLines=(unsigned char *)calloc(Width*Height*3, sizeof(unsigned char));
	if (!(*pLines)) return false;

	return m_pShellInit->ApiScreenCaptureBuffer(Width, Height, *pLines);
}

/*!***********************************************************************
 @brief 	Writes out the image data to a BMP file with basename fname.
 @details   The file written will be fname suffixed with a number to make the file unique.
            For example, if fname is "abc", this function will attempt
            to save to "abc0000.bmp"; if that file already exists, it
            will try "abc0001.bmp", repeating until a new filename is
            found. The final filename used is returned in ofname.
 @param[in]	 fname		base of file to save screen to
 @param[in]	 Width		size of image to capture (relative to 0,0)
 @param[in]	 Height		size of image to capture (relative to 0,0)
 @param[in]  pLines		image data to write out (24bpp, 8-bit per channel RGB)
 @param[in]  ui32PixelReplicate    expand pixels through replication (1 = no scale)
 @param[out] ofname		If non-NULL, receives the filename actually used
 @return	 true for success
*************************************************************************/
int PVRShell::PVRShellScreenSave(
	const char			* const fname,
	const int			Width,
	const int			Height,
	const unsigned char	* const pLines,
	const unsigned int	ui32PixelReplicate,
	char				* const ofname)
{
	char *pszFileName;

	/*
		Choose a filename
	*/
	{
		FILE		*file = 0;
		const char	*pszWritePath;
		int			nScreenshotCount;

		pszWritePath = (const char*)PVRShellGet(prefWritePath);

		size_t	nFileNameSize = strlen(pszWritePath) + 200;
		pszFileName = (char*)malloc(nFileNameSize);

		/* Look for the first file name that doesn't already exist */
		for(nScreenshotCount = 0; nScreenshotCount < 10000; ++nScreenshotCount)
		{
			snprintf(pszFileName, nFileNameSize, "%s%s%04d.bmp", pszWritePath, fname, nScreenshotCount);

			file = fopen(pszFileName,"r");
			if(!file)
				break;
			fclose(file);
		}

		/* If all files already exist, replace the first one */
		if (nScreenshotCount==10000)
		{
			snprintf(pszFileName, nFileNameSize, "%s%s0000.bmp", pszWritePath, fname);
			PVRShellOutputDebug("PVRShell: *WARNING* : Overwriting %s\n", pszFileName);
		}

		if(ofname)	// requested the output file name
		{
			strcpy(ofname, pszFileName);
		}
	}

	const int err = PVRShellWriteBMPFile(pszFileName, Width, Height, pLines, ui32PixelReplicate);
	FREE(pszFileName);
	if (err)
	{
		return 10*err+1;
	}
	else
	{
		// No problem occurred
		return 0;
	}
}

/*!***********************************************************************
 @brief       Swaps the bytes in pBytes from little to big endian (or vice versa)
 @param[in]	  pBytes     The bytes to swap
 @param[in]	  i32ByteNo  The number of bytes to swap
*************************************************************************/
inline void PVRShellByteSwap(unsigned char* pBytes, int i32ByteNo)
{
	int i = 0, j = i32ByteNo - 1;

	while(i < j)
	{
		unsigned char cTmp = pBytes[i];
		pBytes[i] = pBytes[j];
		pBytes[j] = cTmp;

		++i;
		--j;
	}
}

/*!***********************************************************************
 @brief	        Writes out the image data to a BMP file with name fname.
 @param[in]		pszFilename		file to save screen to
 @param[in]		ui32Width		the width of the data
 @param[in]		ui32Height		the height of the data
 @param[in]		pImageData		image data to write out (24bpp, 8-bit per channel RGB)
 @return		0 on success
*************************************************************************/
int PVRShell::PVRShellWriteBMPFile(
	const char			* const pszFilename,
	const unsigned int	ui32Width,
	const unsigned int	ui32Height,
	const void			* const pImageData,
	const unsigned int	ui32PixelReplicate)
{
#define ByteSwap(x) PVRShellByteSwap((unsigned char*) &x, sizeof(x))

	const int		i32BMPHeaderSize = 14; /* The size of a BMP header */
	const int		i32BMPInfoSize   = 40; /* The size of a BMP info header */
	int				Result = 1;
	FILE*			fpDumpfile = 0;

	fpDumpfile = fopen(pszFilename, "wb");

	if (fpDumpfile != 0)
	{
		const short int word = 0x0001;
		const char * const byte = (char*) &word;
		bool bLittleEndian = byte[0] ? true : false;

		unsigned int i32OutBytesPerLine = ui32Width * 3 * ui32PixelReplicate;
		unsigned int i32OutAlign = 0;

		// round up to a dword boundary
		if(i32OutBytesPerLine & 3)
		{
			i32OutBytesPerLine |= 3;
			++i32OutBytesPerLine;
			i32OutAlign = i32OutBytesPerLine - ui32Width * 3 * ui32PixelReplicate;
		}

		unsigned char *pData = (unsigned char*) pImageData;

		{
			int ui32RealSize = i32OutBytesPerLine * ui32Height * ui32PixelReplicate;

			// BMP Header
			unsigned short  bfType = 0x4D42;
			unsigned int   bfSize = i32BMPHeaderSize + i32BMPInfoSize + ui32RealSize;
			unsigned short  bfReserved1 = 0;
			unsigned short  bfReserved2 = 0;
			unsigned int   bfOffBits = i32BMPHeaderSize + i32BMPInfoSize;

			// BMP Info Header
			unsigned int  biSize = i32BMPInfoSize;
			unsigned int  biWidth = ui32Width * ui32PixelReplicate;
			unsigned int  biHeight = ui32Height * ui32PixelReplicate;
			unsigned short biPlanes = 1;
			unsigned short biBitCount = 24;
			unsigned int  biCompression = 0L;
			unsigned int  biSizeImage = ui32RealSize;
			unsigned int  biXPelsPerMeter = 0;
			unsigned int  biYPelsPerMeter = 0;
			unsigned int  biClrUsed = 0;
			unsigned int  biClrImportant = 0;

			if(!bLittleEndian)
			{
				for(unsigned int i = 0; i < ui32Width * ui32Height; ++i)
					PVRShellByteSwap(pData + (3 * i), 3);

				ByteSwap(bfType);
				ByteSwap(bfSize);
				ByteSwap(bfOffBits);
				ByteSwap(biSize);
				ByteSwap(biWidth);
				ByteSwap(biHeight);
				ByteSwap(biPlanes);
				ByteSwap(biBitCount);
				ByteSwap(biCompression);
				ByteSwap(biSizeImage);
			}

			// Write Header.
			fwrite(&bfType		, 1, sizeof(bfType)		, fpDumpfile);
			fwrite(&bfSize		, 1, sizeof(bfSize)		, fpDumpfile);
			fwrite(&bfReserved1	, 1, sizeof(bfReserved1), fpDumpfile);
			fwrite(&bfReserved2	, 1, sizeof(bfReserved2), fpDumpfile);
			fwrite(&bfOffBits	, 1, sizeof(bfOffBits)	, fpDumpfile);

			// Write info header.
			fwrite(&biSize			, 1, sizeof(biSize)			, fpDumpfile);
			fwrite(&biWidth			, 1, sizeof(biWidth)		, fpDumpfile);
			fwrite(&biHeight		, 1, sizeof(biHeight)		, fpDumpfile);
			fwrite(&biPlanes		, 1, sizeof(biPlanes)		, fpDumpfile);
			fwrite(&biBitCount		, 1, sizeof(biBitCount)		, fpDumpfile);
			fwrite(&biCompression	, 1, sizeof(biCompression)	, fpDumpfile);
			fwrite(&biSizeImage		, 1, sizeof(biSizeImage)	, fpDumpfile);
			fwrite(&biXPelsPerMeter	, 1, sizeof(biXPelsPerMeter), fpDumpfile);
			fwrite(&biYPelsPerMeter	, 1, sizeof(biYPelsPerMeter), fpDumpfile);
			fwrite(&biClrUsed		, 1, sizeof(biClrUsed)		, fpDumpfile);
			fwrite(&biClrImportant	, 1, sizeof(biClrImportant)	, fpDumpfile);
		}

		// Write image.
		for(unsigned int nY = 0; nY < ui32Height; ++nY)
		{
			const unsigned char * pRow = &pData[3 * ui32Width * nY];
			for(unsigned int nRepY = 0; nRepY < ui32PixelReplicate; ++nRepY)
			{
				for(unsigned int nX = 0; nX < ui32Width; ++nX)
				{
					const unsigned char * pPixel = &pRow[3 * nX];
					for(unsigned int nRepX = 0; nRepX < ui32PixelReplicate; ++nRepX)
					{
						fwrite(pPixel, 1, 3, fpDumpfile);
					}
				}

				fwrite("\0\0\0\0", i32OutAlign, 1, fpDumpfile);
			}
		}

		// Last but not least close the file.
		fclose(fpDumpfile);

		Result = 0;
	}
	else
	{
		PVRShellOutputDebug("PVRShell: Failed to open \"%s\" for writing screen dump.\n", pszFilename);
	}

	return Result;
}

/*!***********************************************************************
 @brief	    The number itself should be considered meaningless; an
            application should use this function to determine how much
            time has passed between two points (e.g. between each
            frame).
 @return	A value which increments once per millisecond.
*************************************************************************/
unsigned long PVRShell::PVRShellGetTime()
{
	if(m_pShellData->bForceFrameTime)
	{
		// Return a "time" value based on the current frame number
		return (unsigned long) m_pShellData->nShellCurFrameNum * m_pShellData->nFrameTime;
	}
	else
	{
		// Read timer from a platform dependant function
		return m_pShellInit->OsGetTime();
	}
}

/*!***********************************************************************
 @brief	    Check if a key was pressed. The keys on various devices
            are mapped to the PVRShell-supported keys (listed in @a PVRShellKeyName) in
            a platform-dependent manner, since most platforms have different input
            devices. Check the <a href="modules.html">Modules page</a> for your OS
            for details on how the enum values map to your device's key code input.
 @param[in]	key		Code of the key to test
 @return	true if key was pressed
*************************************************************************/
bool PVRShell::PVRShellIsKeyPressed(const PVRShellKeyName key)
{
	if(!m_pShellInit)
		return false;

	return m_pShellInit->DoIsKeyPressed(key);
}

// class PVRShellCommandLine

/*!***********************************************************************
 @brief		Constructor
*************************************************************************/
PVRShellCommandLine::PVRShellCommandLine()
{
	memset(this, 0, sizeof(*this));
}

/*!***********************************************************************
@brief		Destructor
*************************************************************************/
PVRShellCommandLine::~PVRShellCommandLine()
{
	delete [] m_psOrig;
	delete [] m_psSplit;
	FREE(m_pOpt);
}

/*!***********************************************************************
 @brief	    Set command-line options to pStr
 @param[in]	pStr   Input string
*************************************************************************/
void PVRShellCommandLine::Set(const char *pStr)
{
	delete [] m_psOrig;
	m_psOrig = 0;

	if(pStr)
	{
		size_t len = strlen(pStr)+1;
		m_psOrig = new char[len];
		strcpy(m_psOrig, pStr);
	}
}

/*!***********************************************************************
 @brief	    Prepend command-line options to m_psOrig
 @param[in]	pStr Input string
*************************************************************************/
void PVRShellCommandLine::Prefix(const char *pStr)
{
	if(!m_psOrig)
		Set(pStr);
	else if(!pStr)
		return;
	else
	{
		char *pstmp = m_psOrig;
		size_t lenA = strlen(pStr);
		size_t TotalLen = lenA + 1 + strlen(m_psOrig);

		m_psOrig = new char[TotalLen + 1];

		strcpy(m_psOrig, pStr);
		m_psOrig[lenA] = ' ';
		strcpy(m_psOrig + lenA + 1, pstmp);
		m_psOrig[TotalLen] = '\0';

		delete[] pstmp;
	}
}

/*!***********************************************************************
 @brief	    Prepend command-line options to m_psOrig from a file
 @param[in]	pFileName   Input string
*************************************************************************/
bool PVRShellCommandLine::PrefixFromFile(const char *pFileName)
{
	char* nl;
	FILE *pFile = fopen(pFileName, "rb");

	if(pFile)
	{
		// Get the file size
		fseek(pFile, 0, SEEK_END);
		long m_Size = ftell(pFile) + 2;
		fseek(pFile, 0, SEEK_SET);

		char *pFullFile = new char[m_Size];

		if(pFullFile)
		{
			size_t offset = 0;
			while(fgets(pFullFile + offset, (int) (m_Size - offset), pFile))
			{
				offset = strlen(pFullFile);

				// Replace new lines with spaces
				nl = strrchr(pFullFile, '\r');
				if(nl) *nl = ' ';

				nl = strrchr(pFullFile, '\n');
				if(nl) *nl = ' ';
			}

			pFullFile[offset] = '\0';
			Prefix(pFullFile);

			delete[] pFullFile;
			fclose(pFile);
			return true;
		}

		fclose(pFile);
	}

	return false;
}

/*!***********************************************************************
 @brief	  Parse m_psOrig for command-line options and store them in m_pOpt
*************************************************************************/
void PVRShellCommandLine::Parse()
{
	size_t		len;
	int			nIn, nOut;
	bool		bInQuotes;
	SCmdLineOpt	opt;

	if(!m_psOrig)
		return;

	// Delete/free up any options we may have parsed recently
	delete [] m_psSplit;
	FREE(m_pOpt);

	// Take a copy to be edited
	len = strlen(m_psOrig) + 1;
	m_psSplit = new char[len];

	// Break the command line into options
	bInQuotes = false;
	opt.pArg = NULL;
	opt.pVal = NULL;
	nIn = -1;
	nOut = 0;

	do
	{
		++nIn;
		if(m_psOrig[nIn] == '"')
		{
			bInQuotes = !bInQuotes;
		}
		else
		{
			if(bInQuotes && m_psOrig[nIn] != 0)
			{
				if(!opt.pArg)
					opt.pArg = &m_psSplit[nOut];

				m_psSplit[nOut++] = m_psOrig[nIn];
			}
			else
			{
				switch(m_psOrig[nIn])
				{
				case '=':
					m_psSplit[nOut++] = 0;
					opt.pVal = &m_psSplit[nOut];
					break;

				case ' ':
				case '\t':
				case '\0':
					m_psSplit[nOut++] = 0;
					if(opt.pArg || opt.pVal)
					{
						// Increase list length if necessary
						if(m_nOptLen == m_nOptMax)
							m_nOptMax = m_nOptMax * 2 + 1;
						SCmdLineOpt* pTmp = (SCmdLineOpt*)realloc(m_pOpt, m_nOptMax * sizeof(*m_pOpt));
						if(!pTmp)
						{
							FREE(m_pOpt);
							return;
						}

						m_pOpt = pTmp;

						// Add option to list
						m_pOpt[m_nOptLen++] = opt;
						opt.pArg = NULL;
						opt.pVal = NULL;
					}
					break;

				default:
					if(!opt.pArg)
						opt.pArg = &m_psSplit[nOut];

					m_psSplit[nOut++] = m_psOrig[nIn];
					break;
				}
			}
		}
	} while(m_psOrig[nIn]);
}

/*!***********************************************************************
 @brief	      Apply the command-line options to shell
 @param[in]	  shell
*************************************************************************/
void PVRShellCommandLine::Apply(PVRShell &shell)
{
	int i;
	const char *arg, *val;

	for(i = 0; i < m_nOptLen; ++i)
	{
		arg = m_pOpt[i].pArg;
		val = m_pOpt[i].pVal;

		if(!arg)
			continue;

		if(val)
		{
			if(_stricmp(arg, "-width") == 0)
			{
				shell.PVRShellSet(prefWidth, atoi(val));
			}
			else if(_stricmp(arg, "-height") == 0)
			{
				shell.PVRShellSet(prefHeight, atoi(val));
			}
			else if(_stricmp(arg, "-aasamples") == 0)
			{
				shell.PVRShellSet(prefAASamples, atoi(val));
			}
			else if(_stricmp(arg, "-fullscreen") == 0)
			{
				shell.PVRShellSet(prefFullScreen, (atoi(val) != 0));
			}
			else if(_stricmp(arg, "-sw") == 0)
			{
				shell.PVRShellSet(prefSoftwareRendering, (atoi(val) != 0));
			}
			else if(_stricmp(arg, "-quitafterframe") == 0 || _stricmp(arg, "-qaf") == 0)
			{
				shell.PVRShellSet(prefQuitAfterFrame, atoi(val));
			}
			else if(_stricmp(arg, "-quitaftertime") == 0 || _stricmp(arg, "-qat") == 0)
			{
				shell.PVRShellSet(prefQuitAfterTime, (float)atof(val));
			}
			else if(_stricmp(arg, "-posx") == 0)
			{
				shell.PVRShellSet(prefPositionX, atoi(val));
			}
			else if(_stricmp(arg, "-posy") == 0)
			{
				shell.PVRShellSet(prefPositionY, atoi(val));
			}
			else if(_stricmp(arg, "-vsync") == 0)
			{
				shell.PVRShellSet(prefSwapInterval, atoi(val));
			}
			else if(_stricmp(arg, "-powersaving") == 0 || _stricmp(arg, "-ps") == 0)
			{
				shell.PVRShellSet(prefPowerSaving, (atoi(val) != 0));
			}
			else if(_stricmp(arg, "-colourbpp") == 0 || _stricmp(arg, "-colorbpp") == 0 ||_stricmp(arg, "-cbpp") == 0)
			{
				shell.PVRShellSet(prefColorBPP, atoi(val));
			}
			else if(_stricmp(arg, "-depthbpp") == 0 || _stricmp(arg, "-dbpp") == 0)
			{
				shell.PVRShellSet(prefDepthBPP, atoi(val));
			}
			else if(_stricmp(arg, "-rotatekeys") == 0)
			{
				shell.PVRShellSet(prefRotateKeys, atoi(val));
			}
			else if(_stricmp(arg, "-c") == 0)
			{
				const char* pDash = strchr(val, '-');

				shell.PVRShellSet(prefCaptureFrameStart, atoi(val));

				if(!pDash)
					shell.PVRShellSet(prefCaptureFrameStop, atoi(val));
				else
					shell.PVRShellSet(prefCaptureFrameStop, atoi(pDash + 1));
			}
			else if(_stricmp(arg, "-screenshotscale") == 0)
			{
				shell.PVRShellSet(prefCaptureFrameScale, atoi(val));
			}
			else if(_stricmp(arg, "-priority") == 0)
			{
				shell.PVRShellSet(prefPriority, atoi(val));
			}
			else if(_stricmp(arg, "-config") == 0)
			{
				shell.PVRShellSet(prefRequestedConfig, atoi(val));
			}
			else if(_stricmp(arg, "-display") == 0)
			{
				shell.PVRShellSet(prefNativeDisplay, atoi(val));
			}
			else if(_stricmp(arg, "-forceframetime") == 0 || _stricmp(arg, "-fft") == 0)
			{
				shell.PVRShellSet(prefForceFrameTime, true);
				shell.PVRShellSet(prefFrameTimeValue, atoi(val));
			}
			else if(_stricmp(arg, "-discardframeall") == 0)
			{
				shell.PVRShellSet(prefDiscardColor, (atoi(val) != 0));
				shell.PVRShellSet(prefDiscardDepth, (atoi(val) != 0));
				shell.PVRShellSet(prefDiscardStencil, (atoi(val) != 0));
			}
			else if(_stricmp(arg, "-discardframecolor") == 0 || _stricmp(arg, "-discardframecolour") == 0)
			{
				shell.PVRShellSet(prefDiscardColor, (atoi(val) != 0));
			}
			else if(_stricmp(arg, "-discardframedepth") == 0)
			{
				shell.PVRShellSet(prefDiscardDepth, (atoi(val) != 0));
			}
			else if(_stricmp(arg, "-discardframestencil") == 0)
			{
				shell.PVRShellSet(prefDiscardStencil, (atoi(val) != 0));
			}
		}
		else
		{
			if(_stricmp(arg, "-version") == 0)
			{
				shell.PVRShellOutputDebug("Version: \"%s\"\n", shell.PVRShellGet(prefVersion));
			}
#ifdef PVRSHELL_FPS_OUTPUT
			else if(_stricmp(arg, "-fps") == 0)
			{
				shell.PVRShellSet(prefOutputFPS, true);
			}
#endif
			else if(_stricmp(arg, "-info") == 0)
			{
				shell.PVRShellSet(prefOutputInfo, true);
			}
			else if(_stricmp(arg, "-forceframetime") == 0 || _stricmp(arg, "-fft") == 0)
			{
				shell.PVRShellSet(prefForceFrameTime, true);
			}
		}
	}
}

// @Class  PVRShellInit

/*!***********************************************************************
 @brief	Constructor
*************************************************************************/
PVRShellInit::PVRShellInit()
{
	memset(this, 0, sizeof(*this));
}

/*!***********************************************************************
 @brief	Destructor
*************************************************************************/
PVRShellInit::~PVRShellInit()
{
	Deinit();

	delete [] m_pReadPath;
	m_pReadPath = NULL;

	delete [] m_pWritePath;
	m_pWritePath = NULL;
}

/*!***********************************************************************
 @brief	     PVRShell deinitialisation.
 @param[in]	 Shell
*************************************************************************/
void PVRShellInit::Deinit()
{
	if(m_pShell)
	{
		// Is the App currently running?
		if(m_eState > ePVRShellInitApp && m_eState < ePVRShellExit)
		{
			// If so force it to go through the exit procedure
			if(m_eState < ePVRShellReleaseView)
				m_eState = ePVRShellReleaseView;

			// Class the App as done
			gShellDone = true;

			// Run through the exiting states
            while(Run()){};
		}

		delete m_pShell;
		m_pShell = 0;
	}
}

/*!***********************************************************************
 @brief	    PVRShell Initialisation.
 @Function	Init
 @param[in]	Shell
 @return	True on success and false on failure
*************************************************************************/
bool PVRShellInit::Init()
{
	Deinit();

	m_pShell = NewDemo();

	if(!m_pShell)
		return false;

	m_pShell->m_pShellInit	= this;

	// set default direction key mappings
	m_eKeyMapDOWN = PVRShellKeyNameDOWN;
	m_eKeyMapLEFT = PVRShellKeyNameLEFT;
	m_eKeyMapUP = PVRShellKeyNameUP;
	m_eKeyMapRIGHT = PVRShellKeyNameRIGHT;
	nLastKeyPressed = PVRShellKeyNameNull;

	OsInit();

	gShellDone = false;
	m_eState = ePVRShellInitApp;
	return true;
}

/*!***********************************************************************
 @brief	    Receives the command-line from the application.
 @param[in]	str A string containing the command-line
*************************************************************************/
void PVRShellInit::CommandLine(const char *str)
{
	m_CommandLine.Set(str);
}

/*!***********************************************************************
 @brief	    Receives the command-line from the application.
 @param[in]  argc Number of strings in argv
 @param[in]  argv An array of strings
*************************************************************************/
void PVRShellInit::CommandLine(int argc, char **argv)
{
	size_t	tot, len;
	char	*buf;
	int		i;

	tot = 0;
	for(i = 0; i < argc; ++i)
		tot += strlen(argv[i]);

	if(!tot)
	{
		CommandLine((char*) "");
		return;
	}

	// Add room for spaces and the \0
	tot += argc;

	buf = new char[tot];
	tot = 0;
	for(i = 0; i < argc; ++i)
	{
		len = strlen(argv[i]);
		strncpy(&buf[tot], argv[i], len);
		tot += len;
		buf[tot++] = ' ';
	}
	buf[tot-1] = 0;

	CommandLine(buf);

	delete [] buf;
}

/*!***********************************************************************
 @brief	    Return 'true' if the specific key has been pressed.
 @param[in]	key   The key we're querying for
*************************************************************************/
bool PVRShellInit::DoIsKeyPressed(const PVRShellKeyName key)
{
	if(key == nLastKeyPressed)
	{
		nLastKeyPressed = PVRShellKeyNameNull;
		return true;
	}
	else
	{
		return false;
	}
}

/*!***********************************************************************
 @brief	     Used by the OS-specific code to tell the Shell that a key has been pressed.
 @param[in]  nKey The key that has been pressed
*************************************************************************/
void PVRShellInit::KeyPressed(PVRShellKeyName nKey)
{
	nLastKeyPressed = nKey;
}

/*!***********************************************************************
 @brief	     Used by the OS-specific code to tell the Shell that a touch has began at a location.
 @param[in]	 vec2Location   The position of a click/touch on the screen when it first touches
*************************************************************************/
void PVRShellInit::TouchBegan(const float vec2Location[2])
{
	m_bTouching = true;
	m_vec2PointerLocationStart[0] = m_vec2PointerLocation[0] = vec2Location[0];
	m_vec2PointerLocationStart[1] = m_vec2PointerLocation[1] = vec2Location[1];
}

/*!***********************************************************************
 @brief	     Used by the OS-specific code to tell the Shell that a touch has began at a location.
 @param[in]	 vec2Location The position of the pointer/touch pressed on the screen
*************************************************************************/
void PVRShellInit::TouchMoved(const float vec2Location[2])
{
	if(m_bTouching)
	{
		m_vec2PointerLocation[0] = vec2Location[0];
		m_vec2PointerLocation[1] = vec2Location[1];
	}
}

/*!***********************************************************************
 @brief	    Used by the OS-specific code to tell the Shell that the current touch has ended at a location.
 @param[in] vec2Location The position of the pointer/touch on the screen when it is released
*************************************************************************/
void PVRShellInit::TouchEnded(const float vec2Location[2])
{
	if(m_bTouching)
	{
		m_bTouching = false;
		m_vec2PointerLocationEnd[0] = m_vec2PointerLocation[0] = vec2Location[0];
		m_vec2PointerLocationEnd[1] = m_vec2PointerLocation[1] = vec2Location[1];

#if !defined(DISABLE_SWIPE_MAPPING)
		float fX = m_vec2PointerLocationEnd[0] - m_vec2PointerLocationStart[0];
		float fY = m_vec2PointerLocationEnd[1] - m_vec2PointerLocationStart[1];
		float fTmp = fX * fX + fY * fY;

		if(fTmp > 0.005f)
		{
			fTmp = 1.0f / sqrt(fTmp);
			fY *= fTmp;
			float fAngle = acos(fY);

			const float pi = 3.1415f;
			const float pi_half = pi * 0.5f;
			const float error = 0.25f;

			if(fAngle < error)
				KeyPressed(m_eKeyMapDOWN);
			else if(fAngle > (pi - error))
				KeyPressed(m_eKeyMapUP);
			else if(fAngle > (pi_half - error) && fAngle < (pi_half + error))
				KeyPressed((fX < 0) ? m_eKeyMapLEFT : m_eKeyMapRIGHT);
		}
		else if(fTmp < 0.09f)
		{
			if (m_vec2PointerLocationEnd[0] <= 0.3f) // Left half of the screen
				KeyPressed(PVRShellKeyNameACTION1);
			else if (m_vec2PointerLocationEnd[0] >= 0.7f) // Right half of the screen
				KeyPressed(PVRShellKeyNameACTION2);
		}
#endif
	}
}

/*!***********************************************************************
 @brief	  Used by the OS-specific code to tell the Shell where to read external files from
 @return  A path the application is capable of reading from
*************************************************************************/
const char* PVRShellInit::GetReadPath() const
{
	return m_pReadPath;
}

/*!***********************************************************************
 @brief	   Used by the OS-specific code to tell the Shell where to write to
 @return   A path the applications is capable of writing to
*************************************************************************/
const char* PVRShellInit::GetWritePath() const
{
	return m_pWritePath;
}

/*!****************************************************************************
 @brief     Sets the default app name (to be displayed by the OS)
 @param[in]	str   The application name
*******************************************************************************/
void PVRShellInit::SetAppName(const char * const str)
{
	const char *pName = strrchr(str, PVRSHELL_DIR_SYM);

	if(pName)
	{
		++pName;
	}
	else
	{
		pName = str;
	}
	m_pShell->PVRShellSet(prefAppName, pName);
}

/*!***********************************************************************
 @brief	     Set the path to where the application expects to read from.
 @param[in]  str   The read path
*************************************************************************/
void PVRShellInit::SetReadPath(const char * const str)
{
	m_pReadPath = new char[strlen(str)+1];

	if(m_pReadPath)
	{
		strcpy(m_pReadPath, str);
		char* lastSlash = strrchr(m_pReadPath, PVRSHELL_DIR_SYM);

		if(lastSlash)
			lastSlash[1] = 0;
	}
}

/*!***********************************************************************
 @brief     Set the path to where the application expects to write to.
 @param[in] str   The write path
*************************************************************************/
void PVRShellInit::SetWritePath(const char * const str)
{
	m_pWritePath = new char[strlen(str)+1];

	if(m_pWritePath)
	{
		strcpy(m_pWritePath, str);
		char* lastSlash = strrchr(m_pWritePath, PVRSHELL_DIR_SYM);

		if(lastSlash)
			lastSlash[1] = 0;
	}
}

#ifdef PVRSHELL_FPS_OUTPUT
/*****************************************************************************
 @fn       FpsUpdate
 @brief    Calculates a value for frames-per-second (FPS). 
 @details  This is only compiled in to the application if PVRSHELL_FPS_OUTPUT is defined.
*****************************************************************************/
void PVRShellInit::FpsUpdate()
{
	unsigned int ui32TimeDelta, ui32Time;

	ui32Time = m_pShell->PVRShellGetTime();
	++m_i32FpsFrameCnt;
	ui32TimeDelta = ui32Time - m_i32FpsTimePrev;

	if(ui32TimeDelta >= 1000)
	{
		float fFPS = 1000.0f * (float) m_i32FpsFrameCnt / (float) ui32TimeDelta;

		m_pShell->PVRShellOutputDebug("PVRShell: frame %d, FPS %.1f.\n",
			m_pShell->m_pShellData->nShellCurFrameNum, fFPS);

		m_i32FpsFrameCnt = 0;
		m_i32FpsTimePrev = ui32Time;
	}
}
#endif

/*****************************************************************************
 @brief    Main message loop / render loop
 @return   false when the app should quit
*****************************************************************************/
bool PVRShellInit::Run()
{
	static unsigned long StartTime = 0;

	switch(m_eState)
	{
	case ePVRShellInitApp:
		{
			// Make sure the shell isn't done
			gShellDone = false;

			// Prepend command-line options from PVRShellCL.txt
			const char * const pCL = "PVRShellCL.txt";
			const char *pPath = (const char*) m_pShell->PVRShellGet(prefReadPath);
			size_t nSize = strlen(pPath) + strlen(pCL) + 1;
			char *pString = new char[nSize];

			if(pString)
			{
				snprintf(pString, nSize, "%s%s", pPath, pCL);

				if(!m_CommandLine.PrefixFromFile(pString))
				{
					delete[] pString;
					pPath = (const char*) m_pShell->PVRShellGet(prefWritePath);
					nSize = strlen(pPath) + strlen(pCL) + 1;
					pString = new char[nSize];

					snprintf(pString, nSize, "%s%s", pPath, pCL);

					if(m_CommandLine.PrefixFromFile(pString))
						m_pShell->PVRShellOutputDebug("Loaded command-line options from %s.\n", pString);
				}
				else
					m_pShell->PVRShellOutputDebug("Loaded command-line options from %s.\n", pString);

				delete[] pString;
			}

			// Parse the command-line
			m_CommandLine.Parse();

#if defined(_DEBUG)
			m_pShell->PVRShellOutputDebug("PVRShell command line: %d/%d\n", m_CommandLine.m_nOptLen, m_CommandLine.m_nOptMax);
			for(int i = 0; i < m_CommandLine.m_nOptLen; ++i)
			{
				m_pShell->PVRShellOutputDebug("CL %d: \"%s\"\t= \"%s\".\n", i,
					m_CommandLine.m_pOpt[i].pArg ? m_CommandLine.m_pOpt[i].pArg : "",
					m_CommandLine.m_pOpt[i].pVal ? m_CommandLine.m_pOpt[i].pVal : "");
			}
#endif
			// Call InitApplication
			if(!m_pShell->InitApplication())
			{
				m_eState = ePVRShellExit;
				return true;
			}

			m_eState = ePVRShellInitInstance;
			return true;
		}
	case ePVRShellInitInstance:
		{
			m_CommandLine.Apply(*m_pShell);

			// Output non-api specific data if required
			OutputInfo();

			// Perform OS initialisation
			if(!OsInitOS())
			{
				m_pShell->PVRShellOutputDebug("InitOS failed!\n");
				m_eState = ePVRShellQuitApp;
				return true;
			}

			// Initialize the 3D API
			if(!OsDoInitAPI())
			{
				m_pShell->PVRShellOutputDebug("InitAPI failed!\n");
				m_eState = ePVRShellReleaseOS;
				gShellDone = true;
				return true;
			}

			// Output api specific data if required
			OutputAPIInfo();

			// Initialise the app
			if(!m_pShell->InitView())
			{
				m_pShell->PVRShellOutputDebug("InitView failed!\n");
				m_eState = ePVRShellReleaseAPI;
				gShellDone = true;
				return true;
			}

			if(StartTime==0)
			{
				StartTime = OsGetTime();
			}

			m_eState = ePVRShellRender;
			return true;
		}
	case ePVRShellRender:
		{
			// Main message loop:
			if(!m_pShell->RenderScene())
				break;

			ApiRenderComplete();
			OsRenderComplete();

#ifdef PVRSHELL_FPS_OUTPUT
			if(m_pShell->m_pShellData->bOutputFPS)
				FpsUpdate();
#endif
			int nCurrentFrame = m_pShell->m_pShellData->nShellCurFrameNum;

			if(DoIsKeyPressed(PVRShellKeyNameScreenshot) || (nCurrentFrame >= m_pShell->m_pShellData->nCaptureFrameStart && nCurrentFrame <= m_pShell->m_pShellData->nCaptureFrameStop))
			{
				unsigned char *pBuf;
				const int nWidth = m_pShell->PVRShellGet(prefWidth);
				const int nHeight = m_pShell->PVRShellGet(prefHeight);
				if(m_pShell->PVRShellScreenCaptureBuffer(nWidth, nHeight, &pBuf))
				{
					if(m_pShell->PVRShellScreenSave(PVRSHELL_SCREENSHOT_NAME, nWidth, nHeight, pBuf, m_pShell->m_pShellData->nCaptureFrameScale) != 0)
					{
						m_pShell->PVRShellSet(prefExitMessage, "Screen-shot save failed.\n");
					}
				}
				else
				{
					m_pShell->PVRShellSet(prefExitMessage, "Screen capture failed.\n");
				}
				FREE(pBuf);
			}

			if(DoIsKeyPressed(PVRShellKeyNameQUIT))
				gShellDone = true;

			if(gShellDone)
				break;

			/* Quit if maximum number of allowed frames is reached */
			if((m_pShell->m_pShellData->nDieAfterFrames>=0) && (nCurrentFrame >= m_pShell->m_pShellData->nDieAfterFrames))
				break;

			/* Quit if maximum time is reached */
			if((m_pShell->m_pShellData->fDieAfterTime>=0.0f) && (((OsGetTime()-StartTime)*0.001f) >= m_pShell->m_pShellData->fDieAfterTime))
				break;

			m_pShell->m_pShellData->nShellCurFrameNum++;
			return true;
		}

	case ePVRShellReleaseView:
		m_pShell->ReleaseView();

	case ePVRShellReleaseAPI:
		OsDoReleaseAPI();

	case ePVRShellReleaseOS:
		OsReleaseOS();

		if(!gShellDone && m_pShell->m_pShellData->nInitRepeats)
		{
			--m_pShell->m_pShellData->nInitRepeats;
			m_eState = ePVRShellInitInstance;
			return true;
		}

		m_eState = ePVRShellQuitApp;
		return true;

	case ePVRShellQuitApp:
		// Final app tidy-up
		m_pShell->QuitApplication();
		m_eState = ePVRShellExit;

	case ePVRShellExit:
		OsExit();
		StringCopy(m_pShell->m_pShellData->pszAppName, 0);
		StringCopy(m_pShell->m_pShellData->pszExitMessage, 0);
		return false;
	}

	m_eState = (EPVRShellState)(m_eState + 1);
	return true;
}

/*!***********************************************************************
@brief	When prefOutputInfo is set to true this function outputs
        various pieces of non-API dependent information via
        PVRShellOutputDebug.
*************************************************************************/
void PVRShellInit::OutputInfo()
{
	if(m_pShell->PVRShellGet(prefOutputInfo))
	{
		m_pShell->PVRShellOutputDebug("\n");
		m_pShell->PVRShellOutputDebug("App name: %s\n"     , m_pShell->PVRShellGet(prefAppName));
		m_pShell->PVRShellOutputDebug("SDK version: %s\n"  , m_pShell->PVRShellGet(prefVersion));
		m_pShell->PVRShellOutputDebug("\n");
		m_pShell->PVRShellOutputDebug("Read path:  %s\n"    , m_pShell->PVRShellGet(prefReadPath));
		m_pShell->PVRShellOutputDebug("Write path: %s\n"   , m_pShell->PVRShellGet(prefWritePath));
		m_pShell->PVRShellOutputDebug("\n");
		m_pShell->PVRShellOutputDebug("Command-line: %s\n" , m_pShell->PVRShellGet(prefCommandLine));
		m_pShell->PVRShellOutputDebug("\n");
		m_pShell->PVRShellOutputDebug("Power saving: %s\n" , m_pShell->PVRShellGet(prefPowerSaving) ? "On" : "Off");
		m_pShell->PVRShellOutputDebug("AA Samples requested: %i\n", m_pShell->PVRShellGet(prefAASamples));
		m_pShell->PVRShellOutputDebug("Fullscreen: %s\n", m_pShell->PVRShellGet(prefFullScreen) ? "Yes" : "No");
		m_pShell->PVRShellOutputDebug("PBuffer requested: %s\n", m_pShell->PVRShellGet(prefPBufferContext) ? "Yes" : "No");
		m_pShell->PVRShellOutputDebug("ZBuffer requested: %s\n", m_pShell->PVRShellGet(prefZbufferContext) ? "Yes" : "No");
		m_pShell->PVRShellOutputDebug("Stencil buffer requested: %s\n", m_pShell->PVRShellGet(prefStencilBufferContext) ? "Yes" : "No");

		if(m_pShell->PVRShellGet(prefColorBPP) > 0)
			m_pShell->PVRShellOutputDebug("Colour buffer size requested: %i\n", m_pShell->PVRShellGet(prefColorBPP));
		if(m_pShell->PVRShellGet(prefDepthBPP) > 0)
			m_pShell->PVRShellOutputDebug("Depth buffer size requested: %i\n", m_pShell->PVRShellGet(prefDepthBPP));

		m_pShell->PVRShellOutputDebug("Software rendering requested: %s\n", m_pShell->PVRShellGet(prefSoftwareRendering) ? "Yes" : "No");
		m_pShell->PVRShellOutputDebug("Swap Interval requested: %i\n", m_pShell->PVRShellGet(prefSwapInterval));

		if(m_pShell->PVRShellGet(prefInitRepeats) > 0)
			m_pShell->PVRShellOutputDebug("No of Init repeats: %i\n", m_pShell->PVRShellGet(prefInitRepeats));

		if(m_pShell->PVRShellGet(prefQuitAfterFrame) != -1)
			m_pShell->PVRShellOutputDebug("Quit after frame:   %i\n", m_pShell->PVRShellGet(prefQuitAfterFrame));

		if(m_pShell->PVRShellGet(prefQuitAfterTime)  != -1.0f)
			m_pShell->PVRShellOutputDebug("Quit after time:    %f\n", m_pShell->PVRShellGet(prefQuitAfterTime));
	}
}

/****************************************************************************
** Local code
****************************************************************************/
/*!***********************************************************************
 @brief	      This function copies pszSrc into pszStr.
 @param[out]  pszStr   The string to copy pszSrc into
 @param[in]	  pszSrc   The source string to copy
*************************************************************************/
static bool StringCopy(char *&pszStr, const char * const pszSrc)
{
	size_t len;

	FREE(pszStr);

	if(!pszSrc)
		return true;

	len = strlen(pszSrc)+1;
	pszStr = (char*)malloc(len);
	if(!pszStr)
		return false;

	strcpy(pszStr, pszSrc);
	return true;
}

/// @endcond 
//NO_DOXYGEN

/*****************************************************************************
End of file (PVRShell.cpp)
*****************************************************************************/

