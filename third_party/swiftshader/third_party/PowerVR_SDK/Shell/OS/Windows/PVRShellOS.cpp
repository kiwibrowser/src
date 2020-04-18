/******************************************************************************

 @File         PVRShellOS.cpp

 @Title        Windows/PVRShellOS

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     WinCE/Windows

 @Description  Makes programming for 3D APIs easier by wrapping window creation
               and other functions for use by a demo.

******************************************************************************/

/****************************************************************************
 ** INCLUDES                                                               **
 ****************************************************************************/
#include <windows.h>
#include <TCHAR.H>
#include <stdio.h>

#include "PVRShell.h"
#include "PVRShellAPI.h"
#include "PVRShellOS.h"
#include "PVRShellImpl.h"

// No Doxygen for CPP files, due to documentation duplication
/// @cond NO_DOXYGEN

#if !(WINVER >= 0x0500)
	#define COMPILE_MULTIMON_STUBS
	#include <multimon.h>
#endif

/****************************************************************************
	Defines
*****************************************************************************/
/*! The class name for the window */
#define WINDOW_CLASS _T("PVRShellClass")

/*! Maximum size to create string array for determining the read/write paths */
#define DIR_BUFFER_LEN	(10240)

/*! X dimension of the window that is created */
#define SHELL_DISPLAY_DIM_X	800
/*! Y dimension of the window that is created */
#define SHELL_DISPLAY_DIM_Y	600

/*****************************************************************************
	Declarations
*****************************************************************************/
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

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
	m_hAccelTable = 0;

	m_pShell->m_pShellData->nShellDimX = SHELL_DISPLAY_DIM_X;
	m_pShell->m_pShellData->nShellDimY = SHELL_DISPLAY_DIM_Y;

	m_hDC = 0;
	m_hWnd = 0;

	// Pixmap support: init variables to 0
	m_hBmPixmap = 0;
	m_hBmPixmapOld = 0;
	m_hDcPixmap = 0;

	/*
		Construct the binary path for GetReadPath() and GetWritePath()
	*/
	{
		/* Allocate memory for strings and return 0 if allocation failed */
		TCHAR* exeNameTCHAR = new TCHAR[DIR_BUFFER_LEN];
		char* exeName = new char[DIR_BUFFER_LEN];
		if(exeNameTCHAR && exeName)
		{
			DWORD retSize;

			/*
				Get the data path and a default application name
			*/

			// Get full path of executable
			retSize = GetModuleFileName(NULL, exeNameTCHAR, DIR_BUFFER_LEN);

			if (DIR_BUFFER_LEN > (int)retSize)
			{
				/* Get string length in char */
				retSize = (DWORD)_tcslen(exeNameTCHAR);

				/* Convert TChar to char */
				for (DWORD i = 0; i <= retSize; i++)
				{
					exeName[i] = (char)exeNameTCHAR[i];
				}

				SetAppName(exeName);
				SetReadPath(exeName);
				SetWritePath(exeName);
			}
		}

		delete [] exeName;
		delete [] exeNameTCHAR;
	}

	m_u32ButtonState = 0;	// clear mouse button state at startup
}

/*!***********************************************************************
 @Function		OsInitOS
 @description	Saves instance handle and creates main window
				In this function, we save the instance handle in a global variable and
				create and display the main program window.
*************************************************************************/
bool PVRShellInit::OsInitOS()
{
	MONITORINFO sMInfo;
	TCHAR		*appName;
	RECT		winRect;
	POINT		p;

	MyRegisterClass();

	/*
		Build the window title
	*/
	{
		const char		*pszName, *pszSeparator, *pszVersion;
		size_t			len;
		unsigned int	out, in;

		pszName			= (const char*)m_pShell->PVRShellGet(prefAppName);
		pszSeparator	= STR_WNDTITLE;
		pszVersion		= (const char*)m_pShell->PVRShellGet(prefVersion);

		len = strlen(pszName)+strlen(pszSeparator)+strlen(pszVersion)+1;
		appName = new TCHAR[len];

		for(out = 0; (appName[out] = pszName[out]) != 0; ++out);
		for(in = 0; (appName[out] = pszSeparator[in]) != 0; ++in, ++out);
		for(in = 0; (appName[out] = pszVersion[in]) != 0; ++in, ++out);
		_ASSERT(out == len-1);
	}

	/*
		Retrieve the monitor information.

		MonitorFromWindow() doesn't work, because the window hasn't been
		created yet.
	*/
	{
		HMONITOR	hMonitor;
		BOOL		bRet;

		p.x			= m_pShell->m_pShellData->nShellPosX;
		p.y			= m_pShell->m_pShellData->nShellPosY;
		hMonitor	= MonitorFromPoint(p, MONITOR_DEFAULTTOPRIMARY);
		sMInfo.cbSize = sizeof(sMInfo);
		bRet = GetMonitorInfo(hMonitor, &sMInfo);
		_ASSERT(bRet);
	}

	/*
		Reduce the window size until it fits on screen
	*/
	while(
		(m_pShell->m_pShellData->nShellDimX > (sMInfo.rcMonitor.right - sMInfo.rcMonitor.left)) ||
		(m_pShell->m_pShellData->nShellDimY > (sMInfo.rcMonitor.bottom - sMInfo.rcMonitor.top)))
	{
		m_pShell->m_pShellData->nShellDimX >>= 1;
		m_pShell->m_pShellData->nShellDimY >>= 1;
	}


	/*
		Create the window
	*/

	if(m_pShell->m_pShellData->bFullScreen)
	{
		m_hWnd = CreateWindow(WINDOW_CLASS, appName, WS_VISIBLE | WS_SYSMENU,CW_USEDEFAULT, CW_USEDEFAULT, m_pShell->m_pShellData->nShellDimX, m_pShell->m_pShellData->nShellDimY,
				NULL, NULL, m_hInstance, this);

		SetWindowLong(m_hWnd,GWL_STYLE,GetWindowLong(m_hWnd,GWL_STYLE) &~ WS_CAPTION);
		SetWindowPos(m_hWnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	}
	else
	{
		int x, y;

		SetRect(&winRect,
			m_pShell->m_pShellData->nShellPosX,
			m_pShell->m_pShellData->nShellPosY,
			m_pShell->m_pShellData->nShellPosX+m_pShell->m_pShellData->nShellDimX,
			m_pShell->m_pShellData->nShellPosY+m_pShell->m_pShellData->nShellDimY);
		AdjustWindowRectEx(&winRect, WS_CAPTION|WS_SYSMENU, false, 0);

		x = m_pShell->m_pShellData->nShellPosX - winRect.left;
		winRect.left += x;
		winRect.right += x;

		y = m_pShell->m_pShellData->nShellPosY - winRect.top;
		winRect.top += y;
		winRect.bottom += y;

		if(m_pShell->m_pShellData->bShellPosWasDefault)
		{
			x = CW_USEDEFAULT;
			y = CW_USEDEFAULT;
		}
		else
		{
			x = winRect.left;
			y = winRect.top;
		}

		m_hWnd = CreateWindow(WINDOW_CLASS, appName, WS_VISIBLE|WS_CAPTION|WS_SYSMENU,
			x, y, winRect.right-winRect.left, winRect.bottom-winRect.top, NULL, NULL, m_hInstance, this);

	}

	if(!m_hWnd)
		return false;

	if(m_pShell->m_pShellData->bFullScreen)
	{
		m_pShell->m_pShellData->nShellDimX = sMInfo.rcMonitor.right;
		m_pShell->m_pShellData->nShellDimY = sMInfo.rcMonitor.bottom;
		SetWindowPos(m_hWnd,HWND_TOPMOST,0,0,m_pShell->m_pShellData->nShellDimX,m_pShell->m_pShellData->nShellDimY,0);
	}

	m_hDC = GetDC(m_hWnd);
	ShowWindow(m_hWnd, m_nCmdShow);
	UpdateWindow(m_hWnd);

	delete [] appName;
	return true;
}

/*!***********************************************************************
 @Function		OsReleaseOS
 @description	Destroys main window
*************************************************************************/
void PVRShellInit::OsReleaseOS()
{
	ReleaseDC(m_hWnd, m_hDC);
	DestroyWindow(m_hWnd);
}

/*!***********************************************************************
 @Function		OsExit
 @description	Destroys main window
*************************************************************************/
void PVRShellInit::OsExit()
{
	const char	*szText;

	/*
		Show the exit message to the user
	*/
	szText		= (const char*)m_pShell->PVRShellGet(prefExitMessage);

	int			i, nT, nC;
	const char	*szCaption;
	TCHAR		*tzText, *tzCaption;

	szCaption	= (const char*)m_pShell->PVRShellGet(prefAppName);

	if(!szText || !szCaption)
		return;

	nT = (int)strlen(szText) + 1;
	nC = (int)strlen(szCaption) + 1;

	tzText = (TCHAR*)malloc(nT * sizeof(*tzText));
	tzCaption = (TCHAR*)malloc(nC * sizeof(*tzCaption));

	for(i = 0; (tzText[i] = szText[i]) != 0; ++i);
	for(i = 0; (tzCaption[i] = szCaption[i]) != 0; ++i);

	MessageBox(NULL, tzText, tzCaption, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);

	FREE(tzText);
	FREE(tzCaption);
}

/*!***********************************************************************
 @Function		OsDoInitAPI
 @Return		true on success
 @description	Perform API initialisation and bring up window / fullscreen
*************************************************************************/
bool PVRShellInit::OsDoInitAPI()
{

	// Pixmap support: create the pixmap
	if(m_pShell->m_pShellData->bNeedPixmap)
	{
		m_hDcPixmap = CreateCompatibleDC(m_hDC);
		m_hBmPixmap = CreateCompatibleBitmap(m_hDC, 640, 480);
	}

	if(!ApiInitAPI())
	{
		return false;
	}

	// Pixmap support: select the pixmap into a device context (DC) ready for blitting
	if(m_pShell->m_pShellData->bNeedPixmap)
	{
		m_hBmPixmapOld = (HBITMAP)SelectObject(m_hDcPixmap, m_hBmPixmap);
	}

	SetForegroundWindow(m_hWnd);

	/* No problem occured */
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
		SelectObject(m_hDcPixmap, m_hBmPixmapOld);
		DeleteDC(m_hDcPixmap);
		DeleteObject(m_hBmPixmap);
	}
}

/*!***********************************************************************
 @Function		OsRenderComplete
 @Returns		false when the app should quit
 @description	Main message loop / render loop
*************************************************************************/
void PVRShellInit::OsRenderComplete()
{
	MSG		msg;

	/*
		Process the message queue
	*/
	while(PeekMessage(&msg, m_hWnd, 0, 0, PM_REMOVE))
	{
		if (!TranslateAccelerator(msg.hwnd, m_hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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
	return (BitBlt(m_hDC, 0, 0, 640, 480, m_hDcPixmap, 0, 0, SRCCOPY) == TRUE);
}

/*!***********************************************************************
 @Function		OsGetNativeDisplayType
 @Return		The 'NativeDisplayType' for EGL
 @description	Called from InitAPI() to get the NativeDisplayType
*************************************************************************/
void *PVRShellInit::OsGetNativeDisplayType()
{
	return m_hDC;
}

/*!***********************************************************************
 @Function		OsGetNativePixmapType
 @Return		The 'NativePixmapType' for EGL
 @description	Called from InitAPI() to get the NativePixmapType
*************************************************************************/
void *PVRShellInit::OsGetNativePixmapType()
{
	// Pixmap support: return the pixmap
	return m_hBmPixmap;
}

/*!***********************************************************************
 @Function		OsGetNativeWindowType
 @Return		The 'NativeWindowType' for EGL
 @description	Called from InitAPI() to get the NativeWindowType
*************************************************************************/
void *PVRShellInit::OsGetNativeWindowType()
{
	return m_hWnd;
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
	switch(prefName)
	{
	case prefButtonState:
		*pn = m_u32ButtonState;
		return true;
	};
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
	switch(prefName)
	{
	case prefShowCursor:
		ShowCursor(value ? TRUE : FALSE);
		return true;
	}

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
	PVRSHELL_UNREFERENCED_PARAMETER(prefName);
	PVRSHELL_UNREFERENCED_PARAMETER(i32Value);
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
	switch(prefName)
	{
	case prefHINSTANCE:
		*pp = m_hInstance;
		return true;
	default:
		return false;
	}
}

/*!***********************************************************************
 @Function		OsDisplayDebugString
 @Input			str		string to output
 @Description	Prints a debug string
*************************************************************************/
void PVRShellInit::OsDisplayDebugString(char const * const str)
{
	if(str)
	{
#if defined(UNICODE)
		wchar_t	strc[1024];
		int		i;

		for(i = 0; (str[i] != '\0') && (i < (sizeof(strc) / sizeof(*strc))); ++i)
		{
			strc[i] = (wchar_t)str[i];
		}

		strc[i] = '\0';

		OutputDebugString(strc);
#else
		OutputDebugString(str);
#endif
	}
}

/*!***********************************************************************
 @Function		OsGetTime
 @Return		An incrementing time value measured in milliseconds
 @Description	Returns an incrementing time value measured in milliseconds
*************************************************************************/
unsigned long PVRShellInit::OsGetTime()
{
	return (unsigned long)GetTickCount();
}

/*****************************************************************************
 Class: PVRShellInitOS
*****************************************************************************/

/*!******************************************************************************************
@function		MyRegisterClass()
@description	Registers the window class.
				This function and its usage is only necessary if you want this code
				to be compatible with Win32 systems prior to the 'RegisterClassEx'
				function that was added to Windows 95. It is important to call this function
				so that the application will get 'well formed' small icons associated
				with it.
**********************************************************************************************/
ATOM PVRShellInitOS::MyRegisterClass()
{
	WNDCLASS wc;

    wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= (WNDPROC)WndProc;
    wc.cbClsExtra		= 0;
    wc.cbWndExtra		= 0;
    wc.hInstance		= m_hInstance;
    wc.hIcon			= LoadIcon(m_hInstance, _T("ICON"));
    wc.hCursor			= 0;
    wc.lpszMenuName		= 0;
	wc.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszClassName	= WINDOW_CLASS;

	return RegisterClass(&wc);
}

/*****************************************************************************
 Global code
*****************************************************************************/
void doButtonDown(HWND hWnd, PVRShellInit *pData, EPVRShellButtonState eButton, LPARAM lParam)
{
	RECT rcWinDimensions;
	GetClientRect(hWnd,&rcWinDimensions);
	float vec2TouchPosition[2] = { (float)(short)LOWORD(lParam)/(float)(rcWinDimensions.right), (float)(short)HIWORD(lParam)/(float)(rcWinDimensions.bottom) };
	pData->TouchBegan(vec2TouchPosition);
	SetCapture(hWnd);	// must be within window so capture
	pData->m_u32ButtonState |= eButton;
}

bool doButtonUp(HWND hWnd, PVRShellInit *pData, EPVRShellButtonState eButton, LPARAM lParam)
{
	RECT rcWinDimensions;
	GetClientRect(hWnd,&rcWinDimensions);
	float vec2TouchPosition[2] = { (float)(short)LOWORD(lParam)/(float)(rcWinDimensions.right), (float)(short)HIWORD(lParam)/(float)(rcWinDimensions.bottom) };
	pData->TouchEnded(vec2TouchPosition);
	pData->m_u32ButtonState &= (~eButton);

	if(vec2TouchPosition[0] < 0.f || vec2TouchPosition[0] > 1.f || vec2TouchPosition[1] < 0.f || vec2TouchPosition[1] > 1.f)
	{	// pointer has left window
		if(pData->m_u32ButtonState==0)
		{	// only release capture if mouse buttons have been released
			ReleaseCapture();
		}

		return false;
	}
	return true;
}

/*!***************************************************************************
@function		WndProc
@input			hWnd		Handle to the window
@input			message		Specifies the message
@input			wParam		Additional message information
@input			lParam		Additional message information
@returns		result code to OS
@description	Processes messages for the main window.
*****************************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PVRShellInit *pData = (PVRShellInit*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_CREATE:
		{
			CREATESTRUCT	*pCreate = (CREATESTRUCT*)lParam;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreate->lpCreateParams);
			break;
		}
	case WM_PAINT:
		break;
	case WM_DESTROY:
		return 0;
	case WM_CLOSE:
		pData->gShellDone = true;
		return 0;
	case WM_QUIT:
		return 0;
	case WM_MOVE:
		pData->m_pShell->PVRShellSet(prefPositionX, (int)LOWORD(lParam));
		pData->m_pShell->PVRShellSet(prefPositionY, (int)HIWORD(lParam)); 
		break;
	case WM_LBUTTONDOWN:
		{
			doButtonDown(hWnd,pData,ePVRShellButtonLeft,lParam);
			break;
		}
	case WM_LBUTTONUP:
		{
			if(!doButtonUp(hWnd,pData,ePVRShellButtonLeft,lParam))
				return false;
		break;
		}
	case WM_RBUTTONDOWN:
		{
			doButtonDown(hWnd,pData,ePVRShellButtonRight,lParam);
			break;
		}
	case WM_RBUTTONUP:
		{
			if(!doButtonUp(hWnd,pData,ePVRShellButtonRight,lParam))
				return false;
			break;
		}
	case WM_MBUTTONDOWN:
		{
			doButtonDown(hWnd,pData,ePVRShellButtonMiddle,lParam);
			break;
		}
	case WM_MBUTTONUP:
		{
			if(!doButtonUp(hWnd,pData,ePVRShellButtonMiddle,lParam))
				return false;
			break;
		}
	case WM_MOUSEMOVE:
		{
			RECT rcWinDimensions;
			GetClientRect(hWnd,&rcWinDimensions);
			float vec2TouchPosition[2] = { (float)(short)LOWORD(lParam)/(float)(rcWinDimensions.right), (float)(short)HIWORD(lParam)/(float)(rcWinDimensions.bottom) };
			
			if(vec2TouchPosition[0] < 0.f || vec2TouchPosition[0] > 1.f || vec2TouchPosition[1] < 0.f || vec2TouchPosition[1] > 1.f)
			{	
				// pointer has left window
				if(pData->m_u32ButtonState==0)
				{	// only release capture if mouse buttons have been released
					ReleaseCapture();
				}

				pData->TouchEnded(vec2TouchPosition);
				return false;
			}
			else
			{	// pointer is inside window
				pData->TouchMoved(vec2TouchPosition);
			}
			break;
		}
	case WM_SETFOCUS:
		pData->m_bHaveFocus = true;
		return 0;
	case WM_KILLFOCUS:
		pData->m_bHaveFocus = false;
		return 0;
	case WM_KEYDOWN:
	{
		switch(wParam)
		{
		case VK_ESCAPE:
		case 0xC1:
			pData->KeyPressed(PVRShellKeyNameQUIT);
			break;
		case VK_UP:
		case 0x35:
			pData->KeyPressed(pData->m_eKeyMapUP);
			break;
		case VK_DOWN:
		case 0x30:
			pData->KeyPressed(pData->m_eKeyMapDOWN);
			break;
		case VK_LEFT:
		case 0x37:
			pData->KeyPressed(pData->m_eKeyMapLEFT);
			break;
		case VK_RIGHT:
		case 0x39:
			pData->KeyPressed(pData->m_eKeyMapRIGHT);
			break;
		case VK_SPACE:
		case 0x38:
			pData->KeyPressed(PVRShellKeyNameSELECT);
			break;
		case '1':
		case 0x34:
			pData->KeyPressed(PVRShellKeyNameACTION1);
			break;
		case '2':
		case 0x36:
			pData->KeyPressed(PVRShellKeyNameACTION2);
			break;
		case VK_F11:
		case 0xC2:
			pData->KeyPressed(PVRShellKeyNameScreenshot);
			break;
		}
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*!***************************************************************************
@function		WinMain
@input			hInstance		Application instance from OS
@input			hPrevInstance	Always NULL
@input			lpCmdLine		command line from OS
@input			nCmdShow		Specifies how the window is to be shown
@returns		result code to OS
@description	Main function of the program
*****************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, TCHAR *lpCmdLine, int nCmdShow)
{
	size_t			i;
	char			*pszCmdLine;
	PVRShellInit	init;

	PVRSHELL_UNREFERENCED_PARAMETER(hPrevInstance);

#if defined(_WIN32)
	// Enable memory-leak reports
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif

	// Get a char-array command line as the input may be UNICODE
	i = _tcslen(lpCmdLine) + 1;
	pszCmdLine = new char[i];

	while(i)
	{
		--i;
		pszCmdLine[i] = (char)lpCmdLine[i];
	}

	//	Init the demo, process the command line, create the OS initialiser.
	if(!init.Init())
	{
		delete[] pszCmdLine;
		return EXIT_ERR_CODE;
	}

	init.CommandLine(pszCmdLine);
	init.m_hInstance = hInstance;
	init.m_nCmdShow = nCmdShow;

	//	Initialise/run/shutdown
	while(init.Run());

	delete[] pszCmdLine;

	return EXIT_NOERR_CODE;
}

/// @endcond

/*****************************************************************************
 End of file (PVRShellOS.cpp)
*****************************************************************************/

