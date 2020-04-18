/*!****************************************************************************

 @file       Shell/PVRShellImpl.h
 @copyright  Copyright (c) Imagination Technologies Limited.
 @brief      Makes programming for 3D APIs easier by wrapping surface
             initialization, texture allocation and other functions for use by a demo.

******************************************************************************/

#ifndef __PVRSHELLIMPL_H_
#define __PVRSHELLIMPL_H_

/*****************************************************************************
** Build options
*****************************************************************************/


/*****************************************************************************
** Macros
*****************************************************************************/
#define FREE(X) { if(X) { free(X); (X)=0; } }

#ifndef _ASSERT
#define _ASSERT(X) /**/
#endif

/*****************************************************************************
** Defines
*****************************************************************************/
#define STR_WNDTITLE (" - Build ")

/*!***************************************************************************
 @struct PVRShellData
 @brief Holds PVRShell internal data.
*****************************************************************************/
struct PVRShellData
{
    // Shell Interface Data
    char        *pszAppName;                /*!< Application name string. */
    char        *pszExitMessage;            /*!< Exit message string. */
    int         nShellDimX;                 /*!< Width in pixels. */
    int         nShellDimY;                 /*!< Height in pixels. */
    int         nShellPosX;                 /*!< X position of the window. */
    int         nShellPosY;                 /*!< Y position of the window. */
    bool        bFullScreen;                /*!< Fullscreen boolean. */
    bool        bLandscape;                 /*!< Landscape orientation boolean. false = portrait orientation. */
    bool        bNeedPbuffer;               /*!< True if pixel buffer is needed. */
    bool        bNeedZbuffer;               /*!< True if Z buffer is needed. */
    bool        bNeedStencilBuffer;         /*!< True if stencil buffer is needed. */
    bool        bNeedPixmap;                /*!< True if pixmap is needed. */
    bool        bNeedPixmapDisableCopy;     /*!< Disables copy if true, because pixmaps are used. */
    bool        bLockableBackBuffer;        /*!< DX9 only. Enable the use of D3DPRESENTFLAG_LOCKABLE_BACKBUFFER. */
    bool        bSoftwareRender;            /*!< Enable the use of software rendering. */
    bool        bNeedAlphaFormatPre;        /*!< EGL only: If true, creates the EGL surface with EGL_ALPHA_FORMAT_PRE. */
    bool        bUsingPowerSaving;          /*!< Use power saving mode when device is not in use. */
    bool        bOutputInfo;                /*!< Enable information to be output via PVRShellOutputDebug. For example, 
                                                 the depth of the colour surface created, extenstions supported and 
                                                 dimensions of the surface created. */
    bool        bNoShellSwapBuffer;         /*!< Disable eglswapbuffers at the end of each frame. */
    int         nSwapInterval;              /*!< Interval to wait for monitor vertical sync. */
    int         nInitRepeats;               /*!< Number of times to reinitialise. */
    int         nDieAfterFrames;            /*!< Set shell to quit after this number of frames (-1 to disable) */
    float       fDieAfterTime;              /*!< Set shell to quit after this number of seconds (-1 to disable). */
    int         nAASamples;                 /*!< Number of anti-aliasing samples to have. 0 disables anti-aliasing. */
    int         nColorBPP;                  /*!< Color buffer size. */
    int         nDepthBPP;                  /*!< Depth buffer size. */
    int         nCaptureFrameStart;         /*!< The frame to start capturing screenshots from. */
    int         nCaptureFrameStop;          /*!< The frame to stop capturing screenshots from. */
    int         nCaptureFrameScale;         /*!< Save screenshots scale factor. 1 for no scaling. */
    int         nPriority;                  /*!< EGL: If supported sets the egl context priority; 
                                                 0 for low, 1 for med and 2 for high. */
    bool        bForceFrameTime;            /*!< Overrides PVRShellGetTime to force specified frame time. May cause
                                                 problems if PVRShellGetTime is called multiple times in a frame. */
    int         nFrameTime;                 /*!< How long for each frame time to last (in ms). */
    bool        bDiscardFrameColor;         /*!< Discard color data at the end of a render. */
    bool        bDiscardFrameDepth;         /*!< Discard depth data at the end of a render. */
    bool        bDiscardFrameStencil;       /*!< Discard stencil data at the end of a render. */

    // Internal Data
    bool        bShellPosWasDefault;        /*!< Internal. Default position for the shell was used. */
    int         nShellCurFrameNum;          /*!< Internal. Current frame number. */
#ifdef PVRSHELL_FPS_OUTPUT
    bool        bOutputFPS;                 /*!< Output frames per second. */
#endif
};

/*!***************************************************************************
 @class PVRShellCommandLine
 @brief Command-line interpreter
*****************************************************************************/
class PVRShellCommandLine
{
public:
	char		*m_psOrig, *m_psSplit;
	SCmdLineOpt	*m_pOpt;
	int			m_nOptLen, m_nOptMax;

public:
	/*!***********************************************************************
	@brief		Constructor
	*************************************************************************/
	PVRShellCommandLine();

	/*!***********************************************************************
	@brief      Destructor
	*************************************************************************/
	~PVRShellCommandLine();

	/*!***********************************************************************
	@brief	    Set command-line options to pStr
	@param[in]  pStr Input string
	*************************************************************************/
	void Set(const char *pStr);

	/*!***********************************************************************
	@brief	    Prepend command-line options to m_psOrig
	@param[in]  pStr Input string
	*************************************************************************/
	void Prefix(const char *pStr);

	/*!***********************************************************************
	@brief      Prepend command-line options to m_psOrig from a file
	@param[in]  pFileName Input string
	*************************************************************************/
	bool PrefixFromFile(const char *pFileName);

	/*!***********************************************************************
	@brief      Parse m_psOrig for command-line options and store them in m_pOpt
	*************************************************************************/
	void Parse();

	/*!***********************************************************************
	@brief      Apply the command-line options to shell
	@param[in]  shell
	*************************************************************************/
	void Apply(PVRShell &shell);
};

/*!****************************************************************************
 @enum  EPVRShellState
 @brief Current Shell state
*****************************************************************************/
enum EPVRShellState {
	ePVRShellInitApp,		/*!< Initialise app */
	ePVRShellInitInstance,	/*!< Initialise instance */
	ePVRShellRender,		/*!< Render */
	ePVRShellReleaseView,	/*!< Release View */
	ePVRShellReleaseAPI,	/*!< Release API */
	ePVRShellReleaseOS,		/*!< Release Operating System */
	ePVRShellQuitApp,		/*!< Quit App */
	ePVRShellExit		    /*!< Exit */
};

/*!***************************************************************************
 @class  PVRShellInit
 @brief  The PVRShell initialisation class
 ****************************************************************************/
class PVRShellInit : public PVRShellInitAPI, public PVRShellInitOS
{
public:
	friend class PVRShell;
	friend class PVRShellInitOS;
	friend class PVRShellInitAPI;

	PVRShell			*m_pShell;		/*!< Our PVRShell class */
	PVRShellCommandLine	m_CommandLine;	/*!< Our Command-line class */

	bool		gShellDone;				/*!< Indicates that the application has finished */
	EPVRShellState	m_eState;			/*!< Current PVRShell state */

	// Key handling
	PVRShellKeyName	nLastKeyPressed;	/*!< Holds the last key pressed */
	PVRShellKeyName m_eKeyMapLEFT;		/*!< Holds the value to be returned when PVRShellKeyNameLEFT is requested */
	PVRShellKeyName m_eKeyMapUP;		/*!< Holds the value to be returned when PVRShellKeyNameUP is requested */
	PVRShellKeyName m_eKeyMapRIGHT;		/*!< Holds the value to be returned when PVRShellKeyNameRIGHT is requested */
	PVRShellKeyName m_eKeyMapDOWN;		/*!< Holds the value to be returned when PVRShellKeyNameDOWN is requested */

	// Read and Write path
	char	*m_pReadPath;				/*!<Holds the path where the application will read the data from */
	char	*m_pWritePath;				/*!<Holds the path where the application will write the data to */

#ifdef PVRSHELL_FPS_OUTPUT
	// Frames per second (FPS)
	int		m_i32FpsFrameCnt, m_i32FpsTimePrev;
#endif

public:

protected:
	float m_vec2PointerLocation[2];
	float m_vec2PointerLocationStart[2];
	float m_vec2PointerLocationEnd[2];

	// Touch handling
	bool m_bTouching;

public:
    /*!***********************************************************************
	@brief     Constructor
	*************************************************************************/
	PVRShellInit();

	/*!***********************************************************************
	@brief     Destructor
	*************************************************************************/
	~PVRShellInit();

	/*!***********************************************************************
	@brief     PVRShell Initialisation.
	@return    True on success and false on failure
	*************************************************************************/
	bool Init();

	/*!***********************************************************************
	@brief     PVRShell Deinitialisation.
	*************************************************************************/
	void Deinit();

	/*!***********************************************************************
	@param[in] str   A string containing the command-line
	@brief     Receives the command-line from the application.
	*************************************************************************/
	void CommandLine(const char *str);

	/*!***********************************************************************
	@brief     Receives the command-line from the application.
	@param[in] argc   Number of strings in argv
	@param[in] argv   An array of strings
	*************************************************************************/
	void CommandLine(int argc, char **argv);

	/*!***********************************************************************
	@brief     Return 'true' if the specific key has been pressed.
	@param[in] key The key we're querying for
	*************************************************************************/
	bool DoIsKeyPressed(const PVRShellKeyName key);

	/*!***********************************************************************
	@param[in] key   The key that has been pressed
	@brief     Used by the OS-specific code to tell the Shell that a key has been pressed.
	*************************************************************************/
	void KeyPressed(PVRShellKeyName key);

	/*!***********************************************************************
	@brief     Used by the OS-specific code to tell the Shell that a touch has began at a location.
	@param[in] vec2Location   The position of a click/touch on the screen when it first touches.
	*************************************************************************/
	void TouchBegan(const float vec2Location[2]);
	
	/*!***********************************************************************
	@brief     Used by the OS-specific code to tell the Shell that a touch has began at a location.
	@param[in] vec2Location The position of the pointer/touch pressed on the screen.
	*************************************************************************/
	void TouchMoved(const float vec2Location[2]);
	
	/*!***********************************************************************
	@brief     Used by the OS-specific code to tell the Shell that the current touch has ended at a location.
	@param[in] vec2Location   The position of the pointer/touch on the screen when it is released.
	*************************************************************************/
	void TouchEnded(const float vec2Location[2]);

	/*!***********************************************************************
	@brief     Used by the OS-specific code to tell the Shell where to read external files from.
	@return    A path the application is capable of reading from.
	*************************************************************************/
	const char	*GetReadPath() const;

	/*!***********************************************************************
	@brief     Used by the OS-specific code to tell the Shell where to write to.
	@return    A path the applications is capable of writing to
	*************************************************************************/
	const char	*GetWritePath() const;

	/*!******************************************************************************
	@brief     Sets the default app name (to be displayed by the OS)
	@param[in] str The application name
	*******************************************************************************/
	void SetAppName(const char * const str);

	/*!***********************************************************************
	@brief     Set the path to where the application expects to read from.
	@param[in] str The read path
	*************************************************************************/
	void SetReadPath(const char * const str);

	/*!***********************************************************************
	@brief     Set the path to where the application expects to write to.
	@param[in] str The write path
	*************************************************************************/
	void SetWritePath(const char * const str);

	/*!***********************************************************************
	@brief     Called from the OS-specific code to perform the render.
	           When this function fails the application will quit.
	*************************************************************************/
	bool Run();

	/*!***********************************************************************
	@brief     When prefOutputInfo is set to true this function outputs
			   various pieces of non-API dependent information via
			   PVRShellOutputDebug.
	*************************************************************************/
	void OutputInfo();

	/*!***********************************************************************
	@brief     When prefOutputInfo is set to true this function outputs
			   various pieces of API dependent information via
			   PVRShellOutputDebug.
	*************************************************************************/
	void OutputAPIInfo();

#ifdef PVRSHELL_FPS_OUTPUT
	/*!****************************************************************************
	@brief     Calculates a value for frames-per-second (FPS).
	*****************************************************************************/
	void FpsUpdate();
#endif

	/*
		OS functionality
	*/

	/*!***********************************************************************
	@brief     Initialisation for OS-specific code.
	*************************************************************************/
	void		OsInit();

	/*!***********************************************************************
	@brief     Saves instance handle and creates main window
			   In this function, we save the instance handle in a global variable and
			   create and display the main program window.
	*************************************************************************/
	bool		OsInitOS();

	/*!***********************************************************************
	@brief     Destroys main window
	*************************************************************************/
	void		OsReleaseOS();

	/*!***********************************************************************
	@brief     Destroys main window
	*************************************************************************/
	void		OsExit();

	/*!***********************************************************************
	@brief     Perform API initialization and bring up window / fullscreen
	*************************************************************************/
	bool		OsDoInitAPI();

	/*!***********************************************************************
	@brief     Clean up after we're done
	*************************************************************************/
	void		OsDoReleaseAPI();

	/*!***********************************************************************
	@brief     Main message loop / render loop
	*************************************************************************/
	void		OsRenderComplete();

	/*!***********************************************************************
	@brief     When using pixmaps, copy the render to the display
	*************************************************************************/
	bool		OsPixmapCopy();

	/*!***********************************************************************
	@brief     Called from InitAPI() to get the NativeDisplayType
	*************************************************************************/
	void		*OsGetNativeDisplayType();

	/*!***********************************************************************
	@brief     Called from InitAPI() to get the NativePixmapType
	*************************************************************************/
	void		*OsGetNativePixmapType();

	/*!***********************************************************************
	@brief 	   Called from InitAPI() to get the NativeWindowType
	*************************************************************************/
	void		*OsGetNativeWindowType();

	/*!***********************************************************************
	@brief    	Retrieves OS-specific data
	@param[in]  prefName	Name of preference to get
	@param[out] pn   A pointer set to the preference.
	@return 	true on success
	*************************************************************************/
	bool		OsGet(const prefNameIntEnum prefName, int *pn);

	/*!***********************************************************************
	@brief      Retrieves OS-specific data
	@param[in]  prefName	Name of value to get
	@param[out]	pp A pointer set to the value asked for
	@return 	true on success
	*************************************************************************/
	bool		OsGet(const prefNamePtrEnum prefName, void **pp);

	/*!***********************************************************************
	@brief     Sets OS-specific data
	@param[in] prefName		Name of preference to set to value
	@param[in] value		Value
	@return	   true for success
	*************************************************************************/
	bool		OsSet(const prefNameBoolEnum prefName, const bool value);

	/*!***********************************************************************
	@brief     Sets OS-specific data
	@param[in] prefName	Name of value to set
	@param[in] i32Value 	The value to set our named value to
	@return    true on success
	*************************************************************************/
	bool		OsSet(const prefNameIntEnum prefName, const int i32Value);

	/*!***********************************************************************
	@brief     Prints a debug string
	@param[in] str The debug string to display
	*************************************************************************/
	void OsDisplayDebugString(char const * const str);

	/*!***********************************************************************
	@brief     Gets the time in milliseconds
	*************************************************************************/
	unsigned long OsGetTime();

	/*
		API functionality
	*/
	/*!***********************************************************************
	@brief     Initialisation for API-specific code.
	*************************************************************************/
	bool ApiInitAPI();

	/*!***********************************************************************
	@brief     Releases all resources allocated by the API.
	*************************************************************************/
	void ApiReleaseAPI();

	/*!***********************************************************************
	@brief      API-specific function to store the current content of the
				FrameBuffer into the memory allocated by the user.
	@param[in] 	Width  Width of the region to capture
	@param[in] 	Height Height of the region to capture
	@param[out]	pBuf   A buffer to put the screen capture into
	@return     true on success
	*************************************************************************/
	bool ApiScreenCaptureBuffer(int Width,int Height,unsigned char *pBuf);

	/*!***********************************************************************
	@brief    	Perform API operations required after a frame has finished (e.g., flipping).
	*************************************************************************/
	void ApiRenderComplete();

	/*!***********************************************************************
	@brief    	Set preferences which are specific to the API.
	@param[in] 	prefName	Name of preference to set
	@param[out]	i32Value	Value to set it to
	*************************************************************************/
	bool ApiSet(const prefNameIntEnum prefName, const int i32Value);

	/*!***********************************************************************
	@brief    	Get parameters which are specific to the API.
	@param[in]  prefName	Name of value to get
	@param[out] pn   A pointer set to the value asked for
	*************************************************************************/
	bool ApiGet(const prefNameIntEnum prefName, int *pn);

    /*!***********************************************************************
	@brief    	 Get parameters which are specific to the API.
	@param[in]  prefName	Name of value to get
	@param[out] pp   A pointer set to the value asked for
	*************************************************************************/
	bool ApiGet(const prefNamePtrEnum prefName, void **pp);
    
    
	/*!***********************************************************************
	@brief     Run specific API code to perform the operations requested in preferences.
	*************************************************************************/
	void ApiActivatePreferences();
};

#endif /* __PVRSHELLIMPL_H_ */

/*****************************************************************************
 End of file (PVRShellImpl.h)
*****************************************************************************/

