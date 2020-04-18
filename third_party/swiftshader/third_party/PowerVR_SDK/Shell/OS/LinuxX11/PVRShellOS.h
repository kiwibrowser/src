/*!****************************************************************************

 @file         LinuxX11/PVRShellOS.h
 @ingroup      OS_LinuxX11
 @copyright    Copyright (c) Imagination Technologies Limited.
 @brief        Initialization for the shell for LinuxX11.
 @details      Makes programming for 3D APIs easier by wrapping surface
               initialization, Texture allocation and other functions for use by a demo.

******************************************************************************/
#ifndef _PVRSHELLOS_
#define _PVRSHELLOS_

#include "X11/Xlib.h"
#include "X11/Xutil.h"
#ifdef BUILD_OGL
#include "X11/extensions/xf86vmode.h"
#endif

#define PVRSHELL_DIR_SYM	'/'
#define _stricmp strcasecmp

/*!
 @addtogroup OS_LinuxX11 
 @brief      LinuxX11 OS
 @details    The following table illustrates how key codes are mapped in LinuxX11:
             <table>
             <tr><th> Key code    </th><th> nLastKeyPressed (PVRShell) </th></tr>
             <tr><td> Esc	      </td><td> PVRShellKeyNameQUIT	       </td></tr>
             <tr><td> F11	      </td><td> PVRShellKeyNameScreenshot  </td></tr>
             <tr><td> Enter	      </td><td> PVRShellKeyNameSELECT 	   </td></tr>
             <tr><td> '1'	      </td><td> PVRShellKeyNameACTION1	   </td></tr>
             <tr><td> '2'	      </td><td> PVRShellKeyNameACTION2     </td></tr>
             <tr><td> Up arrow    </td><td> m_eKeyMapUP		           </td></tr>
             <tr><td> Down arrow  </td><td> m_eKeyMapDOWN 		       </td></tr>
             <tr><td> Left arrow  </td><td> m_eKeyMapLEFT 		       </td></tr>
             <tr><td> Right arrow </td><td> m_eKeyMapRIGHT		       </td></tr>
             </table>
 @{
*/

/*!***************************************************************************
 @class PVRShellInitOS
 @brief Interface with specific Operative System.
*****************************************************************************/
class PVRShellInitOS
{
public:
	Display*     m_X11Display;
	long         m_X11Screen;
	XVisualInfo* m_X11Visual;
	Colormap     m_X11ColorMap;
	Window       m_X11Window;
	timeval 	 m_StartTime;
#ifdef BUILD_OGL
    XF86VidModeModeLine m_OriginalMode;  // modeline that was active at the starting point of this aplication
    int         m_i32OriginalModeDotClock;
#endif

	// Pixmap support: variables for the pixmap
	Pixmap		m_X11Pixmap;
	GC			m_X11GC;

	unsigned int m_u32ButtonState; // 1 = left, 2 = right, 4 = middle

public:
	int OpenX11Window(const PVRShell &shell);
	void CloseX11Window();
};

/*! @} */

#endif /* _PVRSHELLOS_ */
/*****************************************************************************
 End of file (PVRShellOS.h)
*****************************************************************************/

