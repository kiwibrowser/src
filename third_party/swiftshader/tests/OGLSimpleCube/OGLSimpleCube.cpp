// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/******************************************************************************

@File			OGLSimpleCube.cpp

@Title			OpenGL Simple cube application

@Version		1.0

@Platform		Windows

@Description	Basic window with a cube drawn in it, using libGL (opengl32).
Inspired by http://www.cs.rit.edu/~ncs/Courses/570/UserGuide/OpenGLonWin-11.html

******************************************************************************/
#include <windows.h>
#include <math.h>

#include <gl\GL.h>

#define PI 3.14159265
#define SCALE_FACTOR 0.5

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

const char *className = "OpenGL";
const char *windowName = "OpenGL Cube";
int winX = 0, winY = 0;
int winWidth = 300, winHeight = 300;
float angle = 0.1f;
double theta = angle * PI / 180.0;
int listIndex;

// Rotation matrix
GLfloat R[16] = { 1, 0, 0, 0, 0, cos(theta), -sin(theta), 0, 0, sin(theta), cos(theta), 0, 0, 0, 0, 1 };

// Scaling matrix
GLfloat S[16] = { SCALE_FACTOR, 0, 0, 0, 0, SCALE_FACTOR, 0, 0, 0, 0, SCALE_FACTOR, 0, 0, 0, 0, 1 };

HDC hDC;
HGLRC hGLRC;
HPALETTE hPalette;

GLfloat vertices1[] = {
	0.5F, 0.5F, 0.5F, -0.5F, 0.5F, 0.5F, -0.5F, -0.5F, 0.5F, 0.5F, -0.5F, 0.5F,
	-0.5F, -0.5F, -0.5F, -0.5F, 0.5F, -0.5F, 0.5F, 0.5F, -0.5F, 0.5F, -0.5F, -0.5F,
	0.5F, 0.5F, 0.5F, 0.5F, 0.5F, -0.5F, -0.5F, 0.5F, -0.5F, -0.5F, 0.5F, 0.5F,
	-0.5F, -0.5F, -0.5F, 0.5F, -0.5F, -0.5F, 0.5F, -0.5F, 0.5F, -0.5F, -0.5F, 0.5F,
	0.5F, 0.5F, 0.5F, 0.5F, -0.5F, 0.5F, 0.5F, -0.5F, -0.5F, 0.5F, 0.5F, -0.5F,
	-0.5F, -0.5F, -0.5F, -0.5F, -0.5F, 0.5F, -0.5F, 0.5F, 0.5F, -0.5F, 0.5F, -0.5F
};

GLfloat normals1[] = {
	0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
	0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1,
	0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
	0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,
	1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
	-1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0
};

GLfloat colors1[] = {
	1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0,
	1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0,
	0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0,
	0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1
};

void initializeView(void)
{
	// Set viewing projection
	glMatrixMode(GL_PROJECTION);
	glFrustum(-0.5, 0.5, -0.5, 0.5, 1.0, 3.0);

	// Position viewer
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(0.0F, 0.0F, -2.0F);

	// Position object
	glRotatef(30.0F, 1.0F, 0.0F, 0.0F);
	glRotatef(30.0F, 0.0F, 1.0F, 0.0F);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
}

void initDisplayList(void)
{
	listIndex = glGenLists(1);
	glNewList(listIndex, GL_COMPILE);
	glNormalPointer(GL_FLOAT, 0, normals1);
	glColorPointer(3, GL_FLOAT, 0, colors1);
	glVertexPointer(3, GL_FLOAT, 0, vertices1);

	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glPushMatrix();
	glMultMatrixf(S);
	glDrawArrays(GL_QUADS, 0, 24);
	glPopMatrix();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glEndList();
}

void redraw(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glCallList(listIndex);

	// Rotation
	glMultMatrixf(R);

	SwapBuffers(hDC);
}

void resize(void)
{
	// Set viewport to cover the window
	glViewport(0, 0, winWidth, winHeight);
}

void setupPixelFormat(HDC hDC)
{
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),  // Size
		1,                              // Version
		PFD_SUPPORT_OPENGL |
		PFD_DRAW_TO_WINDOW |
		PFD_DOUBLEBUFFER,               // Support double-buffering
		PFD_TYPE_RGBA,                  // Color type
		16,                             // Prefered color depth
		0, 0, 0, 0, 0, 0,               // Color bits (ignored)
		0,                              // No alpha buffer
		0,                              // Alpha bits (ignored)
		0,                              // No accumulation buffer
		0, 0, 0, 0,                     // Accum bits (ignored)
		16,                             // Depth buffer
		0,                              // No stencil buffer
		0,                              // No auxiliary buffers
		PFD_MAIN_PLANE,                 // Main layer
		0,                              // Reserved
		0, 0, 0,                        // No layer, visible, damage masks
	};
	int pixelFormat;

	pixelFormat = ChoosePixelFormat(hDC, &pfd);
	if(pixelFormat == 0) {
		MessageBox(WindowFromDC(hDC), L"ChoosePixelFormat failed.", L"Error",
			MB_ICONERROR | MB_OK);
		exit(1);
	}

	if(SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) {
		MessageBox(WindowFromDC(hDC), L"SetPixelFormat failed.", L"Error",
			MB_ICONERROR | MB_OK);
		exit(1);
	}
}

int __stdcall WinMain(__in HINSTANCE hCurrentInst, __in_opt HINSTANCE hPreviousInst, __in_opt LPSTR lpCmdLine, __in int nShowCmd)
{
	WNDCLASS wndClass;
	HWND hWnd;
	MSG msg;

	// Register window class
	wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hCurrentInst;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)BLACK_BRUSH;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = L"OpenGL cube";
	RegisterClass(&wndClass);

	// Create window
	hWnd = CreateWindow(
		L"OpenGL cube", L"OpenGL",
		WS_OVERLAPPEDWINDOW,
		winX, winY, winWidth, winHeight,
		NULL, NULL, hCurrentInst, NULL);

	// Display window
	ShowWindow(hWnd, nShowCmd);

	hDC = GetDC(hWnd);
	setupPixelFormat(hDC);
	hGLRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hGLRC);
	initializeView();
	initDisplayList();

	while(true)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			redraw();
		}
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hGLRC);
	ReleaseDC(hWnd, hDC);

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_DESTROY:
		// Finish OpenGL rendering
		if(hGLRC) {
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(hGLRC);
		}
		if(hPalette) {
			DeleteObject(hPalette);
		}
		ReleaseDC(hWnd, hDC);
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		// Track window size changes
		if(hGLRC) {
			winWidth = (int)LOWORD(lParam);
			winHeight = (int)HIWORD(lParam);
			resize();
			return 0;
		}
	case WM_PALETTECHANGED:
		// Realize palette if this is *not* the current window
		if(hGLRC && hPalette && (HWND)wParam != hWnd) {
			UnrealizeObject(hPalette);
			SelectPalette(hDC, hPalette, FALSE);
			RealizePalette(hDC);
			redraw();
			break;
		}
		break;
	case WM_QUERYNEWPALETTE:
		// Realize palette if this is the current window
		if(hGLRC && hPalette) {
			UnrealizeObject(hPalette);
			SelectPalette(hDC, hPalette, FALSE);
			RealizePalette(hDC);
			return TRUE;
		}
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}