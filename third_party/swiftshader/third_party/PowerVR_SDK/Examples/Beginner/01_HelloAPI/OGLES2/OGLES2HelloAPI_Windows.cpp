/******************************************************************************

 @File         OGLES2HelloAPI_Windows.cpp

 @Title        OpenGL ES 2.0 HelloAPI Tutorial

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform

 @Description  Basic Tutorial that shows step-by-step how to initialize OpenGL ES
               2.0, use it for drawing a triangle and terminate it.

******************************************************************************/
#include <stdio.h>
#include <windows.h>
#include <TCHAR.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

/******************************************************************************
 Defines
******************************************************************************/
// Windows class name to register
#define	WINDOW_CLASS _T("PVRShellClass")

// Width and height of the window
#define WINDOW_WIDTH	640
#define WINDOW_HEIGHT	480

// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0

/******************************************************************************
 Global variables
******************************************************************************/

// Variable set in the message handler to finish the demo
bool	g_bDemoDone = false;

/*!****************************************************************************
 @Function		WndProc
 @Input			hWnd		Handle to the window
 @Input			message		Specifies the message
 @Input			wParam		Additional message information
 @Input			lParam		Additional message information
 @Return		LRESULT		result code to OS
 @Description	Processes messages for the main window
******************************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		/*
			Here we are handling 2 system messages: screen saving and monitor power.
			They are especially relevent on mobile devices.
		*/
		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_SCREENSAVE:					// Screensaver trying to start ?
				case SC_MONITORPOWER:				// Monitor trying to enter powersave ?
				return 0;							// Prevent this from happening
			}
			break;
		}

		// Handles the close message when a user clicks the quit icon of the window
		case WM_CLOSE:
			g_bDemoDone = true;
			PostQuitMessage(0);
			return 1;

		default:
			break;
	}

	// Calls the default window procedure for messages we did not handle
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*!****************************************************************************
 @Function		TestEGLError
 @Input			pszLocation		location in the program where the error took
								place. ie: function name
 @Return		bool			true if no EGL error was detected
 @Description	Tests for an EGL error and prints it
******************************************************************************/
bool TestEGLError(HWND hWnd, char* pszLocation)
{
	/*
		eglGetError returns the last error that has happened using egl,
		not the status of the last called function. The user has to
		check after every single egl call or at least once every frame.
	*/
	EGLint iErr = eglGetError();
	if (iErr != EGL_SUCCESS)
	{
		TCHAR pszStr[256];
		_stprintf(pszStr, _T("%s failed (%d).\n"), pszLocation, iErr);
		MessageBox(hWnd, pszStr, _T("Error"), MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	return true;
}

/*!****************************************************************************
 @Function		WinMain
 @Input			hInstance		Application instance from OS
 @Input			hPrevInstance	Always NULL
 @Input			lpCmdLine		command line from OS
 @Input			nCmdShow		Specifies how the window is to be shown
 @Return		int				result code to OS
 @Description	Main function of the program
******************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, TCHAR *lpCmdLine, int nCmdShow)
{
	// Windows variables
	HWND				hWnd	= 0;
	HDC					hDC		= 0;

	// EGL variables
	EGLDisplay			eglDisplay	= 0;
	EGLConfig			eglConfig	= 0;
	EGLSurface			eglSurface	= 0;
	EGLContext			eglContext	= 0;
	EGLNativeWindowType	eglWindow	= 0;

	// Matrix used for projection model view (PMVMatrix)
	float pfIdentity[] =
	{
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f
	};

	// Fragment and vertex shaders code
	char* pszFragShader = "\
		void main (void)\
		{\
			gl_FragColor = vec4(1.0, 1.0, 0.66 ,1.0);\
		}";
	char* pszVertShader = "\
		attribute highp vec4	myVertex;\
		uniform mediump mat4	myPMVMatrix;\
		void main(void)\
		{\
			gl_Position = myPMVMatrix * myVertex;\
		}";

	/*
		Step 0 - Create a EGLNativeWindowType that we can use for OpenGL ES output
	*/

	// Register the windows class
	WNDCLASS sWC;
    sWC.style = CS_HREDRAW | CS_VREDRAW;
	sWC.lpfnWndProc = WndProc;
    sWC.cbClsExtra = 0;
    sWC.cbWndExtra = 0;
    sWC.hInstance = hInstance;
    sWC.hIcon = 0;
    sWC.hCursor = 0;
    sWC.lpszMenuName = 0;
	sWC.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    sWC.lpszClassName = WINDOW_CLASS;
	unsigned int nWidth = WINDOW_WIDTH;
	unsigned int nHeight = WINDOW_HEIGHT;

	ATOM registerClass = RegisterClass(&sWC);
	if (!registerClass)
	{
		MessageBox(0, _T("Failed to register the window class"), _T("Error"), MB_OK | MB_ICONEXCLAMATION);
	}

	// Create the eglWindow
	RECT	sRect;
	SetRect(&sRect, 0, 0, nWidth, nHeight);
	AdjustWindowRectEx(&sRect, WS_CAPTION | WS_SYSMENU, false, 0);
	hWnd = CreateWindow( WINDOW_CLASS, _T("HelloAPI"), WS_VISIBLE | WS_SYSMENU,
						 0, 0, nWidth, nHeight, NULL, NULL, hInstance, NULL);
	eglWindow = hWnd;

	// Get the associated device context
	hDC = GetDC(hWnd);
	if (!hDC)
	{
		MessageBox(0, _T("Failed to create the device context"), _T("Error"), MB_OK|MB_ICONEXCLAMATION);
		goto cleanup;
	}

	/*
		Step 1 - Get the default display.
		EGL uses the concept of a "display" which in most environments
		corresponds to a single physical screen. Since we usually want
		to draw to the main screen or only have a single screen to begin
		with, we let EGL pick the default display.
		Querying other displays is platform specific.
	*/
	eglDisplay = eglGetDisplay(hDC);

    if(eglDisplay == EGL_NO_DISPLAY)
         eglDisplay = eglGetDisplay((EGLNativeDisplayType) EGL_DEFAULT_DISPLAY);
	/*
		Step 2 - Initialize EGL.
		EGL has to be initialized with the display obtained in the
		previous step. We cannot use other EGL functions except
		eglGetDisplay and eglGetError before eglInitialize has been
		called.
		If we're not interested in the EGL version number we can just
		pass NULL for the second and third parameters.
	*/
	EGLint iMajorVersion, iMinorVersion;
	if (!eglInitialize(eglDisplay, &iMajorVersion, &iMinorVersion))
	{
		MessageBox(0, _T("eglInitialize() failed."), _T("Error"), MB_OK|MB_ICONEXCLAMATION);
		goto cleanup;
	}

	/*
		Step 3 - Make OpenGL ES the current API.
		EGL provides ways to set up OpenGL ES and OpenVG contexts
		(and possibly other graphics APIs in the future), so we need
		to specify the "current API".
	*/
	eglBindAPI(EGL_OPENGL_ES_API);
	if (!TestEGLError(hWnd, "eglBindAPI"))
	{
		goto cleanup;
	}

	/*
		Step 4 - Specify the required configuration attributes.
		An EGL "configuration" describes the pixel format and type of
		surfaces that can be used for drawing.
		For now we just want to use the default Windows surface,
		i.e. it will be visible on screen. The list
		has to contain key/value pairs, terminated with EGL_NONE.
	 */
	const EGLint pi32ConfigAttribs[] =
	{
		EGL_LEVEL,				0,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_NATIVE_RENDERABLE,	EGL_FALSE,
		EGL_DEPTH_SIZE,			EGL_DONT_CARE,
		EGL_NONE
	};

	/*
		Step 5 - Find a config that matches all requirements.
		eglChooseConfig provides a list of all available configurations
		that meet or exceed the requirements given as the second
		argument. In most cases we just want the first config that meets
		all criteria, so we can limit the number of configs returned to 1.
	*/
	EGLint iConfigs;
	if (!eglChooseConfig(eglDisplay, pi32ConfigAttribs, &eglConfig, 1, &iConfigs) || (iConfigs != 1))
	{
		MessageBox(0, _T("eglChooseConfig() failed."), _T("Error"), MB_OK|MB_ICONEXCLAMATION);
		goto cleanup;
	}

	/*
		Step 6 - Create a surface to draw to.
		Use the config picked in the previous step and the native window
		handle when available to create a window surface. A window surface
		is one that will be visible on screen inside the native display (or
		fullscreen if there is no windowing system).
		Pixmaps and pbuffers are surfaces which only exist in off-screen
		memory.
	*/
	eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, eglWindow, NULL);

    if(eglSurface == EGL_NO_SURFACE)
    {
        eglGetError(); // Clear error
        eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, NULL, NULL);
	}

	if (!TestEGLError(hWnd, "eglCreateWindowSurface"))
	{
		goto cleanup;
	}

	/*
		Step 7 - Create a context.
		EGL has to create a context for OpenGL ES. Our OpenGL ES resources
		like textures will only be valid inside this context
		(or shared contexts)
	*/
	EGLint ai32ContextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, ai32ContextAttribs);
	if (!TestEGLError(hWnd, "eglCreateContext"))
	{
		goto cleanup;
	}

	/*
		Step 8 - Bind the context to the current thread and use our
		window surface for drawing and reading.
		Contexts are bound to a thread. This means you don't have to
		worry about other threads and processes interfering with your
		OpenGL ES application.
		We need to specify a surface that will be the target of all
		subsequent drawing operations, and one that will be the source
		of read operations. They can be the same surface.
	*/
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	if (!TestEGLError(hWnd, "eglMakeCurrent"))
	{
		goto cleanup;
	}

	/*
		Step 9 - Draw something with OpenGL ES.
		At this point everything is initialized and we're ready to use
		OpenGL ES to draw something on the screen.
	*/

	GLuint uiFragShader, uiVertShader;		/* Used to hold the fragment and vertex shader handles */
	GLuint uiProgramObject;					/* Used to hold the program handle (made out of the two previous shaders */

	// Create the fragment shader object
	uiFragShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load the source code into it
	glShaderSource(uiFragShader, 1, (const char**)&pszFragShader, NULL);

	// Compile the source code
	glCompileShader(uiFragShader);

	// Check if compilation succeeded
	GLint bShaderCompiled;
    glGetShaderiv(uiFragShader, GL_COMPILE_STATUS, &bShaderCompiled);

	if (!bShaderCompiled)
	{

		// An error happened, first retrieve the length of the log message
		int i32InfoLogLength, i32CharsWritten;
		glGetShaderiv(uiFragShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);

		// Allocate enough space for the message and retrieve it
		char* pszInfoLog = new char[i32InfoLogLength];
        glGetShaderInfoLog(uiFragShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);

		// Displays the error in a dialog box
		MessageBox(hWnd, i32InfoLogLength ? pszInfoLog : _T(""), _T("Failed to compile fragment shader"), MB_OK|MB_ICONEXCLAMATION);
		delete[] pszInfoLog;

		goto cleanup;
	}

	// Loads the vertex shader in the same way
	uiVertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(uiVertShader, 1, (const char**)&pszVertShader, NULL);
	glCompileShader(uiVertShader);
    glGetShaderiv(uiVertShader, GL_COMPILE_STATUS, &bShaderCompiled);
	if (!bShaderCompiled)
	{

		int i32InfoLogLength, i32CharsWritten;
		glGetShaderiv(uiVertShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
        glGetShaderInfoLog(uiVertShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);

		MessageBox(hWnd, i32InfoLogLength ? pszInfoLog : _T(""), _T("Failed to compile vertex shader"), MB_OK|MB_ICONEXCLAMATION);

		delete[] pszInfoLog;

		goto cleanup;
	}

	// Create the shader program
    uiProgramObject = glCreateProgram();

	// Attach the fragment and vertex shaders to it
    glAttachShader(uiProgramObject, uiFragShader);
    glAttachShader(uiProgramObject, uiVertShader);

	// Bind the custom vertex attribute "myVertex" to location VERTEX_ARRAY
    glBindAttribLocation(uiProgramObject, VERTEX_ARRAY, "myVertex");

	// Link the program
    glLinkProgram(uiProgramObject);

	// Check if linking succeeded in the same way we checked for compilation success
    GLint bLinked;
    glGetProgramiv(uiProgramObject, GL_LINK_STATUS, &bLinked);
	if (!bLinked)
	{
		int i32InfoLogLength, i32CharsWritten;
		glGetProgramiv(uiProgramObject, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetProgramInfoLog(uiProgramObject, i32InfoLogLength, &i32CharsWritten, pszInfoLog);

		MessageBox(hWnd, i32InfoLogLength ? pszInfoLog : _T(""), _T("Failed to link program"), MB_OK|MB_ICONEXCLAMATION);

		delete[] pszInfoLog;
		goto cleanup;
	}

	// Actually use the created program
    glUseProgram(uiProgramObject);

	// Sets the clear color.
	// The colours are passed per channel (red,green,blue,alpha) as float values from 0.0 to 1.0
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

	// Enable culling
	glEnable(GL_CULL_FACE);

	// We're going to draw a triangle to the screen so create a vertex buffer object for our triangle
	GLuint	ui32Vbo; // Vertex buffer object handle
	
	// Interleaved vertex data
	GLfloat afVertices[] = {	-0.4f,-0.4f,0.0f, // Position
								0.4f ,-0.4f,0.0f,
								0.0f ,0.4f ,0.0f};

	// Generate the vertex buffer object (VBO)
	glGenBuffers(1, &ui32Vbo);

	// Bind the VBO so we can fill it with data
	glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);

	// Set the buffer's data
	unsigned int uiSize = 3 * (sizeof(GLfloat) * 3); // Calc afVertices size (3 vertices * stride (3 GLfloats per vertex))
	glBufferData(GL_ARRAY_BUFFER, uiSize, afVertices, GL_STATIC_DRAW);

	// Draws a triangle for 800 frames
	for(int i = 0; i < 800; ++i)
	{
		// Check if the message handler finished the demo
		if (g_bDemoDone) break;

		/*
			Clears the color buffer.
			glClear() can also be used to clear the depth or stencil buffer
			(GL_DEPTH_BUFFER_BIT or GL_STENCIL_BUFFER_BIT)
		*/
		glClear(GL_COLOR_BUFFER_BIT);

		/*
			Bind the projection model view matrix (PMVMatrix) to
			the associated uniform variable in the shader
		*/

		// First gets the location of that variable in the shader using its name
		int i32Location = glGetUniformLocation(uiProgramObject, "myPMVMatrix");

		// Then passes the matrix to that variable
		glUniformMatrix4fv( i32Location, 1, GL_FALSE, pfIdentity);

		/*
			Enable the custom vertex attribute at index VERTEX_ARRAY.
			We previously binded that index to the variable in our shader "vec4 MyVertex;"
		*/
		glEnableVertexAttribArray(VERTEX_ARRAY);

		// Sets the vertex data to this attribute index
		glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 0, 0);

		/*
			Draws a non-indexed triangle array from the pointers previously given.
			This function allows the use of other primitive types : triangle strips, lines, ...
			For indexed geometry, use the function glDrawElements() with an index list.
		*/
		glDrawArrays(GL_TRIANGLES, 0, 3);

		/*
			Swap Buffers.
			Brings to the native display the current render surface.
		*/
		eglSwapBuffers(eglDisplay, eglSurface);
		if (!TestEGLError(hWnd, "eglSwapBuffers"))
		{
			goto cleanup;
		}

		// Managing the window messages
		MSG msg;
		PeekMessage(&msg, hWnd, NULL, NULL, PM_REMOVE);
		TranslateMessage(&msg);
		DispatchMessage(&msg);

	}

	// Frees the OpenGL handles for the program and the 2 shaders
	glDeleteProgram(uiProgramObject);
	glDeleteShader(uiFragShader);
	glDeleteShader(uiVertShader);

	// Delete the VBO as it is no longer needed
	glDeleteBuffers(1, &ui32Vbo);

	/*
		Step 10 - Terminate OpenGL ES and destroy the window (if present).
		eglTerminate takes care of destroying any context or surface created
		with this display, so we don't need to call eglDestroySurface or
		eglDestroyContext here.
	*/
cleanup:
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglTerminate(eglDisplay);

	/*
		Step 11 - Destroy the eglWindow.
		Again, this is platform specific and delegated to a separate function.
	*/

	// Release the device context
	if (hDC) ReleaseDC(hWnd, hDC);

	// Destroy the eglWindow
	return 0;
}

/******************************************************************************
 End of file (OGLES2HelloAPI_Windows.cpp)
******************************************************************************/

