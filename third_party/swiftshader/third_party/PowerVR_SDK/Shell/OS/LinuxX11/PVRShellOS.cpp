/******************************************************************************

 @File         LinuxX11/PVRShellOS.cpp

 @Title        LinuxX11/PVRShellOS

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     X11

 @Description  Makes programming for 3D APIs easier by wrapping window creation
               and other functions for use by a demo.

******************************************************************************/

#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "PVRShell.h"
#include "PVRShellAPI.h"
#include "PVRShellOS.h"
#include "PVRShellImpl.h"

// No Doxygen for CPP files, due to documentation duplication
/// @cond NO_DOXYGEN

/*!***************************************************************************
	Defines
*****************************************************************************/

/*****************************************************************************
	Declarations
*****************************************************************************/
static Bool WaitForMapNotify( Display *d, XEvent *e, char *arg );

/*!***************************************************************************
	Class: PVRShellInit
*****************************************************************************/

/*!***********************************************************************
@Function		PVRShellOutputDebug
@Input			format			printf style format followed by arguments it requires
@Description	Writes the resultant string to the debug output (e.g. using
				printf(), OutputDebugString(), ...). Check the SDK release notes for
				details on how the string is output.
*************************************************************************/
void PVRShell::PVRShellOutputDebug(char const * const format, ...) const
{
	if(!format)
		return;

	va_list arg;
	char	buf[1024];

	va_start(arg, format);
	vsnprintf(buf, 1024, format, arg);
	va_end(arg);

	// Passes the data to a platform dependant function
	m_pShellInit->OsDisplayDebugString(buf);
}

/*!***********************************************************************
 @Function		OsInit
 @description	Initialisation for OS-specific code.
*************************************************************************/
void PVRShellInit::OsInit()
{
	XInitThreads();

    // set values to negative to mark that these are default values
    m_pShell->m_pShellData->nShellDimX = -240;
    m_pShell->m_pShellData->nShellDimY = -320;

	m_X11Display = NULL;
	m_X11Visual = NULL;

	// Pixmap support: init variables to 0
	m_X11Pixmap = BadValue;

	/*
		Construct the binary path for GetReadPath() and GetWritePath()
	*/
	// Get PID (Process ID)
	pid_t ourPid = getpid();
	char *pszExePath, pszSrcLink[64];
	int len = 64;
	int res;

	sprintf(pszSrcLink, "/proc/%d/exe", ourPid);
	pszExePath = 0;

	do
	{
		len *= 2;
		delete[] pszExePath;
		pszExePath = new char[len];
		res = readlink(pszSrcLink, pszExePath, len);

		if(res < 0)
		{
			m_pShell->PVRShellOutputDebug("Warning Readlink %s failed. The application name, read path and write path have not been set.\n", pszExePath);
			break;
		}
	} while(res >= len);

	if(res >= 0)
	{
		pszExePath[res] = '\0'; // Null-terminate readlink's result
		SetReadPath(pszExePath);
		SetWritePath(pszExePath);
		SetAppName(pszExePath);
	}

	delete[] pszExePath;

	m_u32ButtonState = 0;

	gettimeofday(&m_StartTime,NULL);
}

/*!***********************************************************************
 @Function		OsInitOS
 @description	Saves instance handle and creates main window
				In this function, we save the instance handle in a global variable and
				create and display the main program window.
*************************************************************************/
bool PVRShellInit::OsInitOS()
{
	m_X11Display = XOpenDisplay( NULL );

	if(!m_X11Display)
	{
		m_pShell->PVRShellOutputDebug( "Unable to open X display\n");
		return false;
	}

	m_X11Screen = XDefaultScreen( m_X11Display );

    /*
    	If there is a full screen request then
        set the window size to the display size.
        If there is no full screen request then reduce window size while keeping
		the same aspect by dividing the dims by two until it fits inside the display area.
        If the position has not changed from its default value, set it to the middle of the screen.
    */

    int display_width  = XDisplayWidth(m_X11Display,m_X11Screen);
    int display_height = XDisplayHeight(m_X11Display,m_X11Screen);

    if(m_pShell->m_pShellData->bFullScreen)
    {
        // For OGL we do real fullscreen for others API set a window of fullscreen size
        if(m_pShell->m_pShellData->nShellDimX < 0) {
            m_pShell->m_pShellData->nShellDimX = display_width;
        }
        if(m_pShell->m_pShellData->nShellDimY < 0) {
            m_pShell->m_pShellData->nShellDimY = display_height;
        }
    }
    else
    {
		if(m_pShell->m_pShellData->nShellDimX < 0)
		    m_pShell->m_pShellData->nShellDimX = (display_width > display_height) ? 800 : 600;

		if(m_pShell->m_pShellData->nShellDimY < 0)
		    m_pShell->m_pShellData->nShellDimY = (display_width > display_height) ? 600 : 800;

        if(m_pShell->m_pShellData->nShellDimX > display_width)
            m_pShell->m_pShellData->nShellDimX = display_width;

        if(m_pShell->m_pShellData->nShellDimY > display_height)
            m_pShell->m_pShellData->nShellDimY = display_height;
    }

	// Create the window
	if(!OpenX11Window(*m_pShell))
	{
		m_pShell->PVRShellOutputDebug( "Unable to open X11 window\n" );
		return false;
	}

	// Pixmap support: create the pixmap
	if(m_pShell->m_pShellData->bNeedPixmap)
	{
		int depth = DefaultDepth(m_X11Display, m_X11Screen);
		m_X11Pixmap = XCreatePixmap(m_X11Display,m_X11Window,m_pShell->m_pShellData->nShellDimX,m_pShell->m_pShellData->nShellDimY,depth);
		m_X11GC	  = XCreateGC(m_X11Display,m_X11Window,0,0);
	}

	return true;
}

/*!***********************************************************************
 @Function		OsReleaseOS
 @description	Destroys main window
*************************************************************************/
void PVRShellInit::OsReleaseOS()
{
	XCloseDisplay( m_X11Display );
}

/*!***********************************************************************
 @Function		OsExit
 @description	Destroys main window
*************************************************************************/
void PVRShellInit::OsExit()
{
	// Show the exit message to the user
	m_pShell->PVRShellOutputDebug((const char*)m_pShell->PVRShellGet(prefExitMessage));
}

/*!***********************************************************************
 @Function		OsDoInitAPI
 @Return		true on success
 @description	Perform API initialisation and bring up window / fullscreen
*************************************************************************/
bool PVRShellInit::OsDoInitAPI()
{
	if(!ApiInitAPI())
	{
		return false;
	}

	// No problem occured
	return true;
}

/*!***********************************************************************
 @Function		OsDoReleaseAPI
 @description	Clean up after we're done
*************************************************************************/
void PVRShellInit::OsDoReleaseAPI()
{
	ApiReleaseAPI();

	if(m_pShell->m_pShellData->bNeedPixmap)
	{
		// Pixmap support: free the pixmap
		XFreePixmap(m_X11Display,m_X11Pixmap);
		XFreeGC(m_X11Display,m_X11GC);
	}

	CloseX11Window();
}

/*!***********************************************************************
 @Function		OsRenderComplete
 @Returns		false when the app should quit
 @description	Main message loop / render loop
*************************************************************************/
void PVRShellInit::OsRenderComplete()
{
	int		numMessages;
	XEvent	event;
	char*		atoms;

	// Are there messages waiting, maybe this should be a while loop
	numMessages = XPending( m_X11Display );
	for( int i = 0; i < numMessages; i++ )
	{
		XNextEvent( m_X11Display, &event );

		switch( event.type )
		{
			case ClientMessage:
				atoms = XGetAtomName(m_X11Display, event.xclient.message_type);
				if (*atoms == *"WM_PROTOCOLS")
				{
					gShellDone = true;
				}
				XFree(atoms);
				break;

			case ButtonRelease:
			{
				XButtonEvent *button_event = ((XButtonEvent *) &event);
				switch( button_event->button )
				{
					case 1 : 
					{
						m_u32ButtonState &= ~1;

						// Set the current pointer location
						float vec2PointerLocation[2];
						vec2PointerLocation[0] = (float)button_event->x / (float)m_pShell->m_pShellData->nShellDimX;
						vec2PointerLocation[1] = (float)button_event->y / (float)m_pShell->m_pShellData->nShellDimY;
						TouchEnded(vec2PointerLocation);
					}
					break;
					case 2 : m_u32ButtonState &= ~4; break;
					case 3 : m_u32ButtonState &= ~2; break;
					default : break;
				}
				break;
			}
        	case ButtonPress:
			{
				XButtonEvent *button_event = ((XButtonEvent *) &event);
				switch( button_event->button )
				{
					case 1 : 
					{
						m_u32ButtonState |= 1;

						// Set the current pointer location
						float vec2PointerLocation[2];
						vec2PointerLocation[0] = (float)button_event->x / (float)m_pShell->m_pShellData->nShellDimX;
						vec2PointerLocation[1] = (float)button_event->y / (float)m_pShell->m_pShellData->nShellDimY;
						TouchBegan(vec2PointerLocation);						
					}
					break;
					case 2 : m_u32ButtonState |= 4; break;
					case 3 : m_u32ButtonState |= 2; break;
					default : break;
				}
        		break;
			}
			case MotionNotify:
			{
				XMotionEvent *motion_event = ((XMotionEvent *) &event);

				// Set the current pointer location
				float vec2PointerLocation[2];
				vec2PointerLocation[0] = (float)motion_event->x / (float)m_pShell->m_pShellData->nShellDimX;
				vec2PointerLocation[1] = (float)motion_event->y / (float)m_pShell->m_pShellData->nShellDimY;
				TouchMoved(vec2PointerLocation);	
				break;
			}
			// should SDK handle these?
			case MapNotify:
        	case UnmapNotify:
        		break;

			case KeyPress:
			{
				XKeyEvent *key_event = ((XKeyEvent *) &event);

				switch(key_event->keycode)
				{
					case 9:	 	nLastKeyPressed = PVRShellKeyNameQUIT;	break; 			// Esc
					case 95:	nLastKeyPressed = PVRShellKeyNameScreenshot;	break; 	// F11
					case 36:	nLastKeyPressed = PVRShellKeyNameSELECT;	break; 		// Enter
					case 10:	nLastKeyPressed = PVRShellKeyNameACTION1;	break; 		// number 1
					case 11:	nLastKeyPressed = PVRShellKeyNameACTION2;	break; 		// number 2
					case 98:
					case 111:	nLastKeyPressed = m_eKeyMapUP;	break;
					case 104:
					case 116:	nLastKeyPressed = m_eKeyMapDOWN;	break;
					case 100:
					case 113:	nLastKeyPressed = m_eKeyMapLEFT;	break;
					case 102:
					case 114: 	nLastKeyPressed = m_eKeyMapRIGHT;	break;
					default:
						break;
				}
			}
			break;

			case KeyRelease:
			{
//				char buf[10];
//				XLookupString(&event.xkey,buf,10,NULL,NULL);
//				charsPressed[ (int) *buf ] = 0;
			}
			break;

			default:
				break;
		}
	}
}

/*!***********************************************************************
 @Function		OsPixmapCopy
 @Return		true if the copy succeeded
 @description	When using pixmaps, copy the render to the display
*************************************************************************/
bool PVRShellInit::OsPixmapCopy()
{
	XCopyArea(m_X11Display,m_X11Pixmap,m_X11Window,m_X11GC,0,0,m_pShell->m_pShellData->nShellDimX,m_pShell->m_pShellData->nShellDimY,0,0);
	return true;
}

/*!***********************************************************************
 @Function		OsGetNativeDisplayType
 @Return		The 'NativeDisplayType' for EGL
 @description	Called from InitAPI() to get the NativeDisplayType
*************************************************************************/
void *PVRShellInit::OsGetNativeDisplayType()
{
	return m_X11Display;
}

/*!***********************************************************************
 @Function		OsGetNativePixmapType
 @Return		The 'NativePixmapType' for EGL
 @description	Called from InitAPI() to get the NativePixmapType
*************************************************************************/
void *PVRShellInit::OsGetNativePixmapType()
{
	// Pixmap support: return the pixmap
	return (void*)m_X11Pixmap;
}

/*!***********************************************************************
 @Function		OsGetNativeWindowType
 @Return		The 'NativeWindowType' for EGL
 @description	Called from InitAPI() to get the NativeWindowType
*************************************************************************/
void *PVRShellInit::OsGetNativeWindowType()
{
	return (void*)m_X11Window;
}

/*!***********************************************************************
 @Function		OsGet
 @Input			prefName	Name of value to get
 @Modified		pn A pointer set to the value asked for
 @Returns		true on success
 @Description	Retrieves OS-specific data
*************************************************************************/
bool PVRShellInit::OsGet(const prefNameIntEnum prefName, int *pn)
{
	switch( prefName )
	{
	case prefButtonState:
		*pn = m_u32ButtonState;
		return true;
	default:
		return false;
	};

	return false;
}

/*!***********************************************************************
 @Function		OsGet
 @Input			prefName	Name of value to get
 @Modified		pp A pointer set to the value asked for
 @Returns		true on success
 @Description	Retrieves OS-specific data
*************************************************************************/
bool PVRShellInit::OsGet(const prefNamePtrEnum prefName, void **pp)
{
	return false;
}

/*!***********************************************************************
 @Function		OsSet
 @Input			prefName				Name of preference to set to value
 @Input			value					Value
 @Return		true for success
 @Description	Sets OS-specific data
*************************************************************************/
bool PVRShellInit::OsSet(const prefNameBoolEnum prefName, const bool value)
{
	return false;
}

/*!***********************************************************************
 @Function		OsSet
 @Input			prefName	Name of value to set
 @Input			i32Value 	The value to set our named value to
 @Returns		true on success
 @Description	Sets OS-specific data
*************************************************************************/
bool PVRShellInit::OsSet(const prefNameIntEnum prefName, const int i32Value)
{
	return false;
}

/*!***********************************************************************
 @Function		OsDisplayDebugString
 @Input			str		string to output
 @Description	Prints a debug string
*************************************************************************/
void PVRShellInit::OsDisplayDebugString(char const * const str)
{
	fprintf(stderr, "%s", str);
}

/*!***********************************************************************
 @Function		OsGetTime
 @Return		An incrementing time value measured in milliseconds
 @Description	Returns an incrementing time value measured in milliseconds
*************************************************************************/
unsigned long PVRShellInit::OsGetTime()
{
	timeval tv;
	gettimeofday(&tv,NULL);

	if(tv.tv_sec < m_StartTime.tv_sec)
		m_StartTime.tv_sec = 0;

	unsigned long sec = tv.tv_sec - m_StartTime.tv_sec;
	return (unsigned long)((sec*(unsigned long)1000) + (tv.tv_usec/1000.0));
}

/*****************************************************************************
 Class: PVRShellInitOS
*****************************************************************************/

/*!***********************************************************************
 @Function		OpenX11Window
 @Return		true on success
 @Description	Opens an X11 window. This must be called after
				SelectEGLConfiguration() for gEglConfig to be valid
*************************************************************************/
int PVRShellInitOS::OpenX11Window(const PVRShell &shell)
{
    XSetWindowAttributes	WinAttibutes;
    XSizeHints				sh;
    XEvent					event;
    unsigned long			mask;

#ifdef BUILD_OGL
    XF86VidModeModeInfo **modes;       // modes of display
    int numModes;                      // number of modes of display
    int chosenMode;
    int edimx,edimy;                   //established width and height of the chosen modeline
    int i;
#endif

	int depth = DefaultDepth(m_X11Display, m_X11Screen);
	m_X11Visual = new XVisualInfo;
	XMatchVisualInfo( m_X11Display, m_X11Screen, depth, TrueColor, m_X11Visual);

    if( !m_X11Visual )
    {
    	shell.PVRShellOutputDebug( "Unable to acquire visual" );
    	return false;
    }

    m_X11ColorMap = XCreateColormap( m_X11Display, RootWindow(m_X11Display, m_X11Screen), m_X11Visual->visual, AllocNone );

#ifdef BUILD_OGL
    m_i32OriginalModeDotClock = XF86VidModeBadClock;
    if(shell.m_pShellData->bFullScreen)
    {
        // Get mode lines to see if there is requested modeline
        XF86VidModeGetAllModeLines(m_X11Display, m_X11Screen, &numModes, &modes);

        // look for mode with requested resolution
        chosenMode = -1;
        i=0;
        while((chosenMode == -1)&&(i<numModes))
        {
            if ((modes[i]->hdisplay == shell.m_pShellData->nShellDimX) && (modes[i]->vdisplay == shell.m_pShellData->nShellDimY))
            {
                chosenMode = i;
            }
            ++i;
        }

        // If there is no requested resolution among modelines then terminate
        if(chosenMode == -1)
        {
            shell.PVRShellOutputDebug( "Chosen resolution for full screen mode does not match any modeline available.\n" );
            return false;
        }

        // save desktop-resolution before switching modes
        XF86VidModeGetModeLine(m_X11Display,m_X11Screen, &m_i32OriginalModeDotClock, &m_OriginalMode );

        XF86VidModeSwitchToMode(m_X11Display, m_X11Screen, modes[chosenMode]);
        XF86VidModeSetViewPort(m_X11Display, m_X11Screen, 0, 0);
        edimx = modes[chosenMode]->hdisplay;
        edimy = modes[chosenMode]->vdisplay;
        printf("Fullscreen Resolution %dx%d (chosen mode = %d)\n", edimx, edimy,chosenMode);
        XFree(modes);

		WinAttibutes.colormap = m_X11ColorMap;
		WinAttibutes.background_pixel = 0xFFFFFFFF;
		WinAttibutes.border_pixel = 0;
        WinAttibutes.override_redirect = true;

		// add to these for handling other events
		WinAttibutes.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | KeyPressMask | KeyReleaseMask;

        // The diffrence is that we want to ignore influence of window manager for our fullscreen window
        mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap | CWOverrideRedirect;

        m_X11Window = XCreateWindow( m_X11Display, RootWindow(m_X11Display, m_X11Screen), 0, 0, edimx, edimy, 0,
                                    CopyFromParent, InputOutput, CopyFromParent, mask, &WinAttibutes);

        // keeping the pointer of mouse and keyboard in window to prevent from scrolling the virtual screen
        XWarpPointer(m_X11Display, None ,m_X11Window, 0, 0, 0, 0, 0, 0);

        // Map and then wait till mapped, grabbing should be after mapping the window
        XMapWindow( m_X11Display, m_X11Window );
        XGrabKeyboard(m_X11Display, m_X11Window, True, GrabModeAsync, GrabModeAsync, CurrentTime);
        XGrabPointer(m_X11Display, m_X11Window, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, m_X11Window, None, CurrentTime);
        XIfEvent( m_X11Display, &event, WaitForMapNotify, (char*)m_X11Window );

    }
    else
#endif
    {
        // For OGLES we assume that chaning of video mode is not available (freedesktop does not allow to do it)
        // so if requested resolution differs from the display dims then we quit
        #ifndef BUILD_OGL
        int display_width  = XDisplayWidth(m_X11Display,m_X11Screen);
        int display_height = XDisplayHeight(m_X11Display,m_X11Screen);
        if((shell.m_pShellData->bFullScreen)&&((shell.m_pShellData->nShellDimX != display_width)||(shell.m_pShellData->nShellDimY != display_height)) ) {
            shell.PVRShellOutputDebug( "Chosen resolution for full screen mode does not match available modeline.\n" );
            return false;
        }
        #endif


		WinAttibutes.colormap = m_X11ColorMap;
		WinAttibutes.background_pixel = 0xFFFFFFFF;
		WinAttibutes.border_pixel = 0;

		// add to these for handling other events
		WinAttibutes.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | KeyPressMask | KeyReleaseMask;

		// The attribute mask
        mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap ;

        m_X11Window = XCreateWindow(  m_X11Display, 						// Display
									RootWindow(m_X11Display, m_X11Screen), 	// Parent
									shell.m_pShellData->nShellPosX, 	// X position of window
									shell.m_pShellData->nShellPosY,		// Y position of window
									shell.m_pShellData->nShellDimX,		// Window width
									shell.m_pShellData->nShellDimY,		// Window height
									0,									// Border width
									CopyFromParent, 					// Depth (taken from parent)
									InputOutput, 						// Window class
									CopyFromParent, 					// Visual type (taken from parent)
									mask, 								// Attributes mask
									&WinAttibutes);						// Attributes

		// Set the window position
        sh.flags = USPosition;
        sh.x = shell.m_pShellData->nShellPosX;
        sh.y = shell.m_pShellData->nShellPosY;
        XSetStandardProperties( m_X11Display, m_X11Window, shell.m_pShellData->pszAppName, shell.m_pShellData->pszAppName, None, 0, 0, &sh );

        // Map and then wait till mapped
        XMapWindow( m_X11Display, m_X11Window );
        XIfEvent( m_X11Display, &event, WaitForMapNotify, (char*)m_X11Window );

        // An attempt to hide a border for fullscreen on non OGL apis (OGLES,OGLES2)
        if(shell.m_pShellData->bFullScreen)
        {
			XEvent xev;
			Atom wmState = XInternAtom(m_X11Display, "_NET_WM_STATE", False);
			Atom wmStateFullscreen = XInternAtom(m_X11Display, "_NET_WM_STATE_FULLSCREEN", False);

			memset(&xev, 0, sizeof(XEvent));
			xev.type = ClientMessage;
			xev.xclient.window = m_X11Window;
			xev.xclient.message_type = wmState;
			xev.xclient.format = 32;
			xev.xclient.data.l[0] = 1;
			xev.xclient.data.l[1] = wmStateFullscreen;
			xev.xclient.data.l[2] = 0;
			XSendEvent(m_X11Display, RootWindow(m_X11Display, m_X11Screen), False, SubstructureNotifyMask, &xev);
        }

        Atom wmDelete = XInternAtom(m_X11Display, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(m_X11Display, m_X11Window, &wmDelete, 1);
        XSetWMColormapWindows( m_X11Display, m_X11Window, &m_X11Window, 1 );
    }

    XFlush( m_X11Display );

    return true;
}

/*!***********************************************************************
 @Function		CloseX11Window
 @Return		void
 @Description	destroy the instance of a window, and release all relevent memory
*************************************************************************/
void PVRShellInitOS::CloseX11Window()
{
    // revert introductional resolution (full screen case, bad clock is default value meaning that good was not acquired)
#ifdef BUILD_OGL
    XF86VidModeModeInfo tmpmi;

    if(m_i32OriginalModeDotClock != XF86VidModeBadClock)
    {
        // revert desktop-resolution (stored previously) before exiting
        tmpmi.dotclock = m_i32OriginalModeDotClock;
        tmpmi.c_private = m_OriginalMode.c_private;
        tmpmi.flags = m_OriginalMode.flags;
        tmpmi.hdisplay = m_OriginalMode.hdisplay;
        tmpmi.hskew = m_OriginalMode.hskew;
        tmpmi.hsyncend = m_OriginalMode.hsyncend;
        tmpmi.hsyncstart = m_OriginalMode.hsyncstart;
        tmpmi.htotal = m_OriginalMode.htotal;
        tmpmi.privsize = m_OriginalMode.privsize;
        tmpmi.vdisplay = m_OriginalMode.vdisplay;
        tmpmi.vsyncend = m_OriginalMode.vsyncend;
        tmpmi.vsyncstart = m_OriginalMode.vsyncstart;
        tmpmi.vtotal = m_OriginalMode.vtotal;

        XF86VidModeSwitchToMode(m_X11Display,m_X11Screen,&tmpmi);
    }
#endif

	XDestroyWindow( m_X11Display, m_X11Window );
    XFreeColormap( m_X11Display, m_X11ColorMap );

	if(m_X11Visual)
		delete m_X11Visual;
}

/*****************************************************************************
 Global code
*****************************************************************************/

static Bool WaitForMapNotify( Display *d, XEvent *e, char *arg )
{
	return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
}

/*!***************************************************************************
@function		main
@input			argc	count of args from OS
@input			argv	array of args from OS
@returns		result code to OS
@description	Main function of the program
*****************************************************************************/
int main(int argc, char **argv)
{
	PVRShellInit init;

	// Initialise the demo, process the command line, create the OS initialiser.
	if(!init.Init())
		return EXIT_ERR_CODE;

	init.CommandLine((argc-1),&argv[1]);

	// Initialise/run/shutdown
	while(init.Run());

	return EXIT_NOERR_CODE;
}

/// @endcond

/*****************************************************************************
 End of file (PVRShellOS.cpp)
*****************************************************************************/

