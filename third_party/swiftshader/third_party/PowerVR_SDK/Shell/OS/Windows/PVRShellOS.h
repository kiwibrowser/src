/*!****************************************************************************

 @file         Windows/PVRShellOS.h
 @ingroup      OS_Windows
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Initialization for the shell for the Windows OS.
 @details      Makes programming for 3D APIs easier by wrapping surface
               initialization, Texture allocation and other functions for use by a demo.

******************************************************************************/
#ifndef _PVRSHELLOS_
#define _PVRSHELLOS_

#include <windows.h>

// The following defines are for Windows PC platforms only
#if defined(_WIN32)
// Enable the following 2 lines for memory leak checking - also see WinMain()
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define PVRSHELL_DIR_SYM	'\\'
#define vsnprintf _vsnprintf

/*!
 @addtogroup OS_Windows 
 @brief      Windows OS
 @details    The following table illustrates how key codes are mapped in Windows:
             <table>
             <tr><th> Key code </th><th> KeyPressed (PVRShell)     </th></tr>
             <tr><td> ESCAPE   </td><td> PVRShellKeyNameQUIT	   </td></tr>
             <tr><td> UP       </td><td> m_eKeyMapUP	           </td></tr>
             <tr><td> DOWN     </td><td> m_eKeyMapDOWN	           </td></tr>
             <tr><td> LEFT     </td><td> m_eKeyMapLEFT             </td></tr>
             <tr><td> RIGHT    </td><td> m_eKeyMapRIGHT            </td></tr>
             <tr><td> SPACE    </td><td> PVRShellKeyNameSELECT     </td></tr>
             <tr><td> '1'      </td><td> PVRShellKeyNameACTION1    </td></tr>
             <tr><td> '2'      </td><td> PVRShellKeyNameACTION2    </td></tr>
             <tr><td> F11      </td><td> PVRShellKeyNameScreenshot </td></tr>
             </table>
 @{
*/

/*!***************************************************************************
 @class PVRShellInitOS
 @brief Interface with specific Operating System.
*****************************************************************************/
class PVRShellInitOS
{
public:
	HDC			m_hDC;
	HWND		m_hWnd;

	// Pixmap support: variables for the pixmap
	HBITMAP		m_hBmPixmap, m_hBmPixmapOld;
	HDC			m_hDcPixmap;

	HACCEL		m_hAccelTable;
	HINSTANCE	m_hInstance;
	int			m_nCmdShow;

	bool		m_bHaveFocus;

	unsigned int	m_u32ButtonState;

public:
	ATOM MyRegisterClass();
};

/*! @} */

#endif /* _PVRSHELLOS_ */
/*****************************************************************************
 End of file (PVRShellOS.h)
*****************************************************************************/

