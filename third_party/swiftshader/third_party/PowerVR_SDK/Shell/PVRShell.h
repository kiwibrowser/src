/*!****************************************************************************

 @file       Shell/PVRShell.h
 @copyright  Copyright (c) Imagination Technologies Limited.
 @brief      Makes programming for 3D APIs easier by wrapping surface
             initialization, Texture allocation and other functions for use by a demo.

******************************************************************************/

#ifndef __PVRSHELL_H_
#define __PVRSHELL_H_

/*****************************************************************************/
/*! @mainpage PVRShell
******************************************************************************

 @tableofcontents 
 
 @section overview Overview
 *****************************

 PVRShell is a C++ class used to make programming for PowerVR platforms easier and more portable.
 PVRShell takes care of all API and OS initialisation for the user and handles adapters, devices, screen/windows modes,
 resolution, buffering, depth-buffer, viewport creation & clearing, etc...

 PVRShell consists of 3 files: PVRShell.cpp, PVRShellOS.cpp and PVRShellAPI.cpp.

 PVRShellOS.cpp and PVRShellAPI.cpp are supplied per platform and contain all the code to initialise the specific
 API (OpenGL ES, Direct3D Mobile, etc.) and the OS (Windows, Linux, WinCE, etc.).
 PVRShell.cpp is where the common code resides and it interacts with the user application through an abstraction layer.

 A new application must link to these three files and must create a class that will inherit the PVRShell class.
 This class will provide five virtual functions to interface with the user.

 The user also needs to register his application class through the NewDemo function:

 @code
 class MyApplication: public PVRShell
 {
 public:
     virtual bool InitApplication();
     virtual bool InitView();
     virtual bool ReleaseView();
     virtual bool QuitApplication();
     virtual bool RenderScene();
 };

 PVRShell* NewDemo()
 {
     return new MyApplication();
 }
 @endcode
 

 @section interface Interface
 ******************************

 There are two functions for initialisation, two functions to release allocated resources and a render function:

 InitApplication() will be called by PVRShell once per run, before the graphic context is created.
 It is used to initialise variables that are not dependant on the rendering context (e.g. external modules, loading user data, etc.).
 QuitApplication() will be called by PVRShell once per run, just before exiting the program.
 If the graphic context is lost, QuitApplication() will not be called.

 InitView() will be called by PVRShell upon creation or change in the rendering context.
 It is used to initialise variables that are dependant on the rendering context (e.g. textures, vertex buffers, etc.).
 ReleaseView() will be called by PVRShell before changing to a new rendering context or
 when finalising the application (before calling QuitApplication).

 RenderScene() is the main rendering loop function of the program. This function must return FALSE when the user wants to terminate the application.
 PVRShell will call this function every frame and will manage relevant OS events.

 There are other PVRShell functions which allow the user to set his/her preferences and get data back from the devices:

 PVRShellSet() and PVRShellGet() are used to pass data to and from PVRShell. PVRShellSet() is recommended to be used
 in InitApplication() so the user preferences are applied at the API initialisation.
 There is a definition of these functions per type of data passed or returned. Please check the prefNameBoolEnum, prefNameFloatEnum,
 prefNameIntEnum, prefNamePtrEnum and prefNameConstPtrEnum enumerations for a full list of the data available.

 This is an example:

 @code
 bool MyApplication::InitApplication()
 {
     PVRShellSet (prefFullScreen, true);
 }

 bool MyApplication::RenderScene()
 {
     int dwCurrentWidth = PVRShellGet (prefHeight);
     int dwCurrentHeight = PVRShellGet (prefWidth);

     return true;
 }
 @endcode

 
 @section helper Helper functions
 *************************************

 The user input is abstracted with the PVRShellIsKeyPressed() function. It will not work in all devices, but we have tried to map the most
 relevant keys when possible. See PVRShellKeyName enumeration for the list of keys supported. This function will return true or false depending on
 the specified key being pressed.

 There are a few other helper functions supplied by PVRShell as well. These functions allow you to read the timer, to output debug information and to
 save a screen-shot of the current frame:

 PVRShellGetTime() returns time in milliseconds.

 PVRShellOutputDebug() will write a debug string (same format as printf) to the platform debug output.

 PVRShellScreenCaptureBuffer() and  PVRShellWriteBMPFile() will be used to save the current frame as a BMP file. PVRShellScreenCaptureBuffer()
 receives a pointer to an area of memory containing the screen buffer. The memory should be freed with free() when not needed any longer.

 Example of screenshot:

 @code
 bool MyApplication::RenderScene()
 {
     [...]

     unsigned char *pLines;

     PVRShellScreenCaptureBuffer(PVRShellGet (prefWidth), PVRShellGet (prefHeight), &pLines);

     PVRShellScreenSave("myfile", pLines, NULL);

     free (pLines);

     return true;
 }
 @endcode

 
 @section cmd Command-line
 *************************************

 Across all platforms, PVRShell takes a set of command-line arguments which allow things like the position and size of the demo
 to be controlled. The list below shows these options.

 \b -width=N   Sets the horizontal viewport width to N.

 \b -height=N   Sets the vertical viewport height to N.

 \b -posx=N   Sets the x coordinate of the viewport.

 \b -posy=N   Sets the y coordinate of the viewport.

 \b -aasamples=N   Sets the number of samples to use for full screen anti-aliasing. e.g 0, 2, 4, 8

 \b -fullscreen=N   Enable/Disable fullscreen mode. N can be: 0=Windowed 1=Fullscreen.

 \b -qat=N   Quits after N seconds.

 \b -qaf=N   Quits after N frames.

 \b -powersaving=N Where available enable/disable power saving. N can be: 0=Disable power saving 1=Enable power saving.

 \b -vsync=N Where available modify the apps vsync parameters.

 \b -version Output the SDK version to the debug output.

 \b -info Output setup information (e.g. window width) to the debug output.

 \b -rotatekeys=N Sets the orientation of the keyboard input. N can be: 0-3, 0 is no rotation.

 \b -c=N Save a single screenshot or a range. e.g. -c=1-10, -c=14.

 \b -priority=N EGL only. Sets the priority of the EGL context.

 \b -colourbpp=N EGL only. When choosing an EGL config N will be used as the value for EGL_BUFFER_SIZE.

 \b -depthbpp=N EGL only. When choosing an EGL config N will be used as the value for EGL_DEPTH_SIZE.

 \b -config=N EGL only. Force the shell to use the EGL config with ID N.

 \b -forceframetime=N Alter the behaviour of PVRShellGetTime so its returned value is frame based (N denotes how many ms a frame should pretend to last). You can also use the shortened version of -fft and the command can be used without N being defined, e.g. just -forceframetime. This option is provided to aid in debugging time-based applications.

 Example:
 @code
     Demo -width=160 -height=120 -qaf=300
 @endcode

 @section APIsOSs APIs and Operating Systems
 *****************************
 For information specific to each 3D API and Operating System, see the list of supported APIs and OSs on the <a href="modules.html">Modules</a> page.
 
******************************************************************************/
// Uncomment to enable the -fps command-line option
// #define PVRSHELL_FPS_OUTPUT

/*****************************************************************************
** Includes
*****************************************************************************/
#include <stdlib.h>

#define EXIT_NOERR_CODE 0
#define EXIT_ERR_CODE (!EXIT_NOERR_CODE)

// avoid warning about unused parameter
#define PVRSHELL_UNREFERENCED_PARAMETER(x) ((void) x)

// Keyboard mapping. //
 
/*!***********************************************************************
 @enum PVRShellKeyName
 @brief Key name.
 ************************************************************************/ 
enum PVRShellKeyName
{
    PVRShellKeyNameNull,        /*!< Null key value */
    PVRShellKeyNameQUIT,        /*!< QUIT key value */
    PVRShellKeyNameSELECT,      /*!< SELECT key value */
    PVRShellKeyNameACTION1,     /*!< ACTION1 key value */
    PVRShellKeyNameACTION2,     /*!< ACTION2 key value */
    PVRShellKeyNameUP,          /*!< UP key */
    PVRShellKeyNameDOWN,        /*!< DOWN key */
    PVRShellKeyNameLEFT,        /*!< LEFT key */
    PVRShellKeyNameRIGHT,       /*!< RIGHT key */
    PVRShellKeyNameScreenshot   /*!< SCREENSHOT key */
};

/*!***********************************************************************
 @enum PVRShellKeyRotate
 @brief Key rotate.
 ************************************************************************/ 
enum PVRShellKeyRotate
{
    PVRShellKeyRotateNone=0,    /*!< Rotate key none = 0. */ 
    PVRShellKeyRotate90,        /*!< Rotate key 90 */
    PVRShellKeyRotate180,       /*!< Rotate key 180 */
    PVRShellKeyRotate270        /*!< Rotate key 270 */
};

//  Pointer button mapping. //
 
/*!***********************************************************************
 @enum EPVRShellButtonState
 @brief Pointer button mapping.
 ************************************************************************/ 
enum EPVRShellButtonState
{
    ePVRShellButtonLeft = 0x1,  /*!< Left button */
    ePVRShellButtonRight = 0x2, /*!< Right button */
    ePVRShellButtonMiddle = 0x4 /*!< Middle button */
};

/*!***********************************************************************
 @enum prefNameBoolEnum
 @brief Boolean Shell preferences.
 ************************************************************************/
enum prefNameBoolEnum
{
    prefFullScreen,             /*!< Set to: 1 for full-screen rendering; 0 for windowed */
    prefIsRotated,              /*!< Query this to learn whether screen is rotated */
    prefPBufferContext,         /*!< 1 if you need pbuffer support (default is pbuffer not needed) */
    prefPixmapContext,          /*!< 1 to use a pixmap as a render-target (default off) */
    prefPixmapDisableCopy,      /*!< 1 to disable the copy if pixmaps are used */
    prefZbufferContext,         /*!< 1 if you wish to have zbuffer support (default to on) */
    prefLockableBackBuffer,     /*!< DX9 only: true to use D3DPRESENTFLAG_LOCKABLE_BACKBUFFER (default: false) */
    prefSoftwareRendering,      /*!< 1 to select software rendering (default: off, i.e. use hardware) */
    prefStencilBufferContext,   /*!< 1 if you wish to have stencil support (default: off) */
    prefAlphaFormatPre,         /*!< EGL only: 1 to create the EGL surface with EGL_ALPHA_FORMAT_PRE (default: 0) */
    prefPowerSaving,            /*!< If true then the app will go into powersaving mode (if available) when not in use. */
#ifdef PVRSHELL_FPS_OUTPUT
    prefOutputFPS,              /*!< If true then the FPS are output using PVRShellOutputdebug */
#endif
    prefOutputInfo,             /*!< If true then the app will output helpful information such as colour buffer format via PVRShellOutputDebug. */
    prefNoShellSwapBuffer,      /*!< EGL: If true then the shell won't call eglswapbuffers at the end of each frame. */
    prefShowCursor,             /*!< Set to: 1 to show the cursor; 0 to hide it. */
    prefForceFrameTime,         /*!< If true will alter PVRShellGetTime behaviour to be frame based. This is for debugging purposes. */
    prefDiscardColor,           /*!< GLES: Whether or not to discard color data at the end of a render, to save bandwidth. Requires specific functionality. (default: false) */
    prefDiscardDepth,           /*!< GLES: Whether or not to discard depth data at the end of a render, to save bandwidth. Requires specific functionality. (default: true) */
    prefDiscardStencil          /*!< GLES: Whether or not to discard stencil data at the end of a render, to save bandwidth. Requires specific functionality. (default: true) */
};

/*!***********************************************************************
 @enum prefNameFloatEnum
 @brief Float Shell preferences.
 ************************************************************************/
enum prefNameFloatEnum
{
    prefQuitAfterTime           /*!< Shell will quit after this number of seconds (-1 to disable) */
};

/*!***********************************************************************
 @enum prefNameIntEnum
 @brief Integer Shell preferences.
 ************************************************************************/
enum prefNameIntEnum
{
    prefEGLMajorVersion,    /*!< EGL: returns the major version as returned by eglInitialize() */
    prefEGLMinorVersion,    /*!< EGL: returns the minor version as returned by eglInitialize() */
    prefWidth,              /*!< Width of render target */
    prefHeight,             /*!< Height of render target */
    prefPositionX,          /*!< X position of the window */
    prefPositionY,          /*!< Y position of the window */
    prefQuitAfterFrame,     /*!< Shell will quit after this number of frames (-1 to disable) */
    prefSwapInterval,       /*!< 0 to preventing waiting for monitor vertical syncs */
    prefInitRepeats,        /*!< Number of times to reinitialise (if >0 when app returns false from RenderScene(), shell will ReleaseView(), InitView() then re-enter RenderScene() loop). Decrements on each initialisation. */
    prefAASamples,          /*!< Set to: 0 to disable full-screen anti-aliasing; 2 for 2x; 4 for 4x; 8 for 8x. */
    prefCommandLineOptNum,  /*!< Returns the length of the array returned by prefCommandLineOpts */
    prefColorBPP,           /*!< Allows you to specify a desired color buffer size e.g. 16, 32. */
    prefDepthBPP,           /*!< Allows you to specify a desired depth buffer size e.g. 16, 24. */
    prefRotateKeys,         /*!< Allows you to specify and retrieve how the keyboard input is transformed */
    prefButtonState,        /*!< pointer button state */
    prefCaptureFrameStart,  /*!< The frame to start capturing screenshots from */
    prefCaptureFrameStop,   /*!< The frame to stop capturing screenshots at */
    prefCaptureFrameScale,  /*!< Pixel-replicate saved screenshots this many times; default 1 for no scale */
    prefPriority,           /*!< EGL: If supported will set the egl context priority; 0 for low, 1 for med and 2 for high. */
    prefConfig,             /*!< EGL: Get the chosen EGL config. */
    prefRequestedConfig,    /*!< EGL: Force the shell to use a particular EGL config. */
    prefNativeDisplay,      /*!< EGL: Allows you to specify the native display to use if the device has more that one. */
    prefFrameTimeValue      /*!< An integer value to say how long you wish one frame to last for (in ms) when force frame time is enabled. */
};

/*!***********************************************************************
 @enum prefNamePtrEnum
 @brief Pointers/Handlers Shell preferences.
 ************************************************************************/
enum prefNamePtrEnum
{
    prefD3DDevice,          /*!< D3D: returns the device pointer */
    prefEGLDisplay,         /*!< EGL: returns the EGLDisplay */
    prefEGLSurface,         /*!< EGL: returns the EGLSurface */
    prefHINSTANCE,          /*!< Windows: returns the application instance handle */
    prefNativeWindowType,   /*!< Returns the window handle */
    prefAccelerometer,      /*!< Accelerometer values */
    prefPointerLocation,    /*!< Mouse pointer/touch location values */
    prefPVR2DContext,       /*!< PVR2D: returns the PVR2D context */
    prefLoadFileFunc,       /*!< A pointer to a function that can be used to load external files on platforms that don't allow the use of fopen.
                                 The ptr returned is of the type PFNLoadFileFunc defined below. */
    prefReleaseFileFunc,        /*!< A pointer to a function that is used to release any data allocated by the load file function.
                                 The ptr returned is of the type PFNReleaseFileFunc defined below. */
    prefAndroidNativeActivity /*!< Android: A pointer to the ANativeActivity struct for the application. Your application will need to include android_native_app_glue.h to cast the pointer to ANativeActivity. */
};

/*!***********************************************************************
 @typedef PFNLoadFileFunc
 @brief The LoadFileFunc function pointer template.
 ************************************************************************/
typedef void* (*PFNLoadFileFunc)(const char*, char** pData, size_t &size);

/*!***********************************************************************
 @typedef PFNReleaseFileFunc
 @brief The ReleaseFileFunc function pointer template.
 ************************************************************************/
typedef bool (*PFNReleaseFileFunc)(void* handle);

/*!***********************************************************************
 @enum prefNameConstPtrEnum
 @brief Constant pointers Shell preferences.
 ************************************************************************/
enum prefNameConstPtrEnum
{
    prefAppName,            /*!< ptrValue is char* */
    prefReadPath,           /*!< ptrValue is char*; will include a trailing slash */
    prefWritePath,          /*!< ptrValue is char*; will include a trailing slash */
    prefCommandLine,        /*!< used to retrieve the entire application command line */
    prefCommandLineOpts,    /*!< ptrValue is SCmdLineOpt*; retrieves an array of arg/value pairs (parsed from the command line) */
    prefExitMessage,        /*!< ptrValue is char*; gives the shell a message to show on exit, typically an error */
    prefVersion             /*!< ptrValue is char* */
};

/*!**************************************************************************
 @struct PVRShellData
 @brief  PVRShell implementation Prototypes and definitions
*****************************************************************************/
struct PVRShellData;

/*!***************************************************************************
 @class PVRShellInit
 *****************************************************************************/
class PVRShellInit;

/*!***********************************************************************
 @struct SCmdLineOpt
 @brief  Stores a variable name/value pair for an individual command-line option.
 ************************************************************************/
struct SCmdLineOpt
{
    const char *pArg, *pVal;
};

/*!***************************************************************************
 @class    PVRShell
 @brief    Inherited by the application; responsible for abstracting the OS and API.
 @details  PVRShell is the main Shell class that an application uses. An
           application should supply a class which inherits PVRShell and supplies
           implementations of the virtual functions of PVRShell (InitApplication(),
           QuitApplication(), InitView(), ReleaseView(), RenderScene()). Default stub
           functions are supplied; this means that an application is not
           required to supply a particular function if it does not need to do anything
           in it.
           The other, non-virtual, functions of PVRShell are utility functions that the
           application may call.
 *****************************************************************************/
class PVRShell
{
private:
    friend class PVRShellInitOS;
    friend class PVRShellInit;

    PVRShellData    *m_pShellData;
    PVRShellInit    *m_pShellInit;

public:
    /*!***********************************************************************
    @brief      Constructor
    *************************************************************************/
    PVRShell();

    /*!***********************************************************************
    @brief      Destructor
    *************************************************************************/
    virtual ~PVRShell();

    /*
        PVRShell functions that the application should implement.
    */

    /*!***********************************************************************
    @brief      Initialise the application.
    @details    This function can be overloaded by the application. It
                will be called by PVRShell once, only at the beginning of
                the PVRShell WinMain()/main() function. This function
                enables the user to perform any initialisation before the
                render API is initialised. From this function the user can
                call PVRShellSet() to change default values, e.g.
                requesting a particular resolution or device setting.
    @return     true for success, false to exit the application
    *************************************************************************/
    virtual bool InitApplication() { return true; };

    /*!***********************************************************************
    @brief      Quit the application.
    @details    This function can be overloaded by the application. It
                will be called by PVRShell just before finishing the
                program. It enables the application to release any
                memory/resources acquired in InitApplication().
    @return     true for success, false to exit the application
    *************************************************************************/
    virtual bool QuitApplication() { return true; };

    /*!***********************************************************************
    @brief      Initialise the view.
    @details    This function can be overloaded by the application. It
                will be called by PVRShell after the OS and rendering API
                are initialised, before entering the RenderScene() loop.
                It is called any time the rendering API is initialised,
                i.e. once at the beginning, and possibly again if the
                resolution changes, or a power management even occurs, or
                if the app requests a re-initialisation.
                The application should check here the configuration of
                the rendering API; it is possible that requests made in
                InitApplication() were not successful.
                Since everything is initialised when this function is
                called, you can load textures and perform rendering API functions.
    @return     true for success, false to exit the application
    *************************************************************************/
    virtual bool InitView() { return true; };

    /*!***********************************************************************
     @brief     Release the view.
     @details   This function can be overloaded by the application. It
                will be called after the RenderScene() loop, before
                shutting down the render API. It enables the application
                to release any memory/resources acquired in InitView().
     @return    true for success, false to exit the application
    *************************************************************************/
    virtual bool ReleaseView() {  return true; };

    /*!***********************************************************************
     @brief     Render the scene
     @details   This function can be overloaded by the application.
                It is main application function in which you have to do your own rendering.  Will be
                called repeatedly until the application exits.
     @return    true for success, false to exit the application
    *************************************************************************/
    virtual bool RenderScene() { return true; };

    /*
        PVRShell functions available for the application to use.
    */

    /*!***********************************************************************
     @brief     This function is used to pass preferences to the PVRShell.
                If used, this function must be called from InitApplication().
     @param[in] prefName  Name of preference to set to value
     @param[in] value     Value
     @return    true for success
    *************************************************************************/
    bool PVRShellSet(const prefNameBoolEnum prefName, const bool value);

    /*!***********************************************************************
     @brief     This function is used to pass preferences to the PVRShell.
                If used, this function must be called from InitApplication().
     @param[in] prefName    Name of preference to set to value
     @param[in] value       Value
     @return    true for success
    *************************************************************************/
    bool PVRShellSet(const prefNameFloatEnum prefName, const float value);

    /*!***********************************************************************
     @brief     This function is used to pass preferences to the PVRShell.
                If used, this function must be called from InitApplication().
     @param[in] prefName    Name of preference to set to value
     @param[in] value       Value
     @return    true for success
    *************************************************************************/
    bool PVRShellSet(const prefNameIntEnum prefName, const int value);

    /*!***********************************************************************
     @brief     This function is used to pass preferences to the PVRShell.
                If used, this funciton must be called from InitApplication().
     @param[in] prefName    Name of preference to set to value
     @param[in] ptrValue    Value
     @return    true for success
    *************************************************************************/
    bool PVRShellSet(const prefNamePtrEnum prefName, const void * const ptrValue);

    /*!***********************************************************************
     @brief     This function is used to pass preferences to the PVRShell.
                If used, this function must be called from InitApplication().
     @param[in] prefName    Name of preference to set to value
     @param[in] ptrValue    Value
     @return    true for success
    *************************************************************************/
    bool PVRShellSet(const prefNameConstPtrEnum prefName, const void * const ptrValue);

    /*!***********************************************************************
     @brief     This function is used to get parameters from the PVRShell.
                It can be called from anywhere in the program.
     @param[in] prefName    Name of preference to set to value
     @return    The requested value.
    *************************************************************************/
    bool PVRShellGet(const prefNameBoolEnum prefName) const;

    /*!***********************************************************************
    @brief      This function is used to get parameters from the PVRShell.
                It can be called from anywhere in the program.
    @param[in]  prefName    Name of preference to set to value
    @return     The requested value.
    *************************************************************************/
    float PVRShellGet(const prefNameFloatEnum prefName) const;

    /*!***********************************************************************
    @brief      This function is used to get parameters from the PVRShell.
                It can be called from anywhere in the program.
    @param[in]  prefName    Name of preference to set to value
    @return     The requested value.
    *************************************************************************/
    int PVRShellGet(const prefNameIntEnum prefName) const;

    /*!***********************************************************************
    @brief      This function is used to get parameters from the PVRShell.
                It can be called from anywhere in the program.
    @param[in]  prefName    Name of preference to set to value
    @return     The requested value.
    *************************************************************************/
    void *PVRShellGet(const prefNamePtrEnum prefName) const;

    /*!***********************************************************************
    @brief      This function is used to get parameters from the PVRShell
                It can be called from anywhere in the program.
    @param[in]  prefName    Name of preference to set to value
    @return     The requested value.
    *************************************************************************/
    const void *PVRShellGet(const prefNameConstPtrEnum prefName) const;

    /*!***********************************************************************
     @brief      It will be stored as 24-bit per pixel, 8-bit per chanel RGB. 
                 The memory should be freed using the free() function when no longer needed.
     @param[in]  Width   size of image to capture (relative to 0,0)
     @param[in]  Height  size of image to capture (relative to 0,0)
     @param[out] pLines  receives a pointer to an area of memory containing the screen buffer.
     @return     true for success
    *************************************************************************/
    bool PVRShellScreenCaptureBuffer(const int Width, const int Height, unsigned char **pLines);

    /*!***********************************************************************
    @brief      Writes out the image data to a BMP file with basename fname.
    @details    The file written will be fname suffixed with a
                number to make the file unique.
                For example, if fname is "abc", this function will attempt
                to save to "abc0000.bmp"; if that file already exists, it
                will try "abc0001.bmp", repeating until a new filename is
                found. The final filename used is returned in ofname.
    @param[in]  fname               base of file to save screen to
    @param[in]  Width               size of image
    @param[in]  Height              size of image
    @param[in]  pLines              image data to write out (24bpp, 8-bit per channel RGB)
    @param[in]  ui32PixelReplicate  expand pixels through replication (1 = no scale)
    @param[out] ofname              If non-NULL, receives the filename actually used
    @return     true for success
    *************************************************************************/
    int PVRShellScreenSave(
        const char          * const fname,
        const int           Width,
        const int           Height,
        const unsigned char * const pLines,
        const unsigned int  ui32PixelReplicate = 1,
        char                * const ofname = NULL);

    /*!***********************************************************************
     @brief     Writes out the image data to a BMP file with name fname.
     @param[in] pszFilename     file to save screen to
     @param[in] ui32Width       the width of the data
     @param[in] ui32Height      the height of the data
     @param[in] pImageData      image data to write out (24bpp, 8-bit per channel RGB)
     @param[in] ui32PixelReplicate  expand pixels through replication (1 = no scale)
     @return    0 on success
    *************************************************************************/
    int PVRShellWriteBMPFile(
        const char          * const pszFilename,
        const unsigned int  ui32Width,
        const unsigned int  ui32Height,
        const void          * const pImageData,
        const unsigned int  ui32PixelReplicate = 1);

    /*!***********************************************************************
    @brief     Writes the resultant string to the debug output (e.g. using
               printf(), OutputDebugString(), ...). Check the SDK release notes for
               details on how the string is output.
    @param[in] format       printf style format followed by arguments it requires
    *************************************************************************/
    void PVRShellOutputDebug(char const * const format, ...) const;

    /*!***********************************************************************
    @brief      The number itself should be considered meaningless; an
                application should use this function to determine how much
                time has passed between two points (e.g. between each frame).
    @return     A value which increments once per millisecond.
    *************************************************************************/
    unsigned long PVRShellGetTime();

    /*!***********************************************************************
    @brief      Check if a key was pressed. 
    @details    The keys on various devices
                are mapped to the PVRShell-supported keys (listed in @a PVRShellKeyName) in
                a platform-dependent manner, since most platforms have different input
                devices. Check the <a href="modules.html">Modules page</a> for your OS
                for details on how the enum values map to your device's key code input.
    @param[in]  key     Code of the key to test
    @return     true if key was pressed
    *************************************************************************/
    bool PVRShellIsKeyPressed(const PVRShellKeyName key);
};

/****************************************************************************
** Declarations for functions that the scene file must supply
****************************************************************************/

/*!***************************************************************************
 @brief     This function must be implemented by the user of the shell.
 @details   The user should return its PVRShell object defining the
            behaviour of the application.
 @return    The demo supplied by the user
*****************************************************************************/
PVRShell* NewDemo();

#endif /* __PVRSHELL_H_ */

/*****************************************************************************
 End of file (PVRShell.h)
*****************************************************************************/

