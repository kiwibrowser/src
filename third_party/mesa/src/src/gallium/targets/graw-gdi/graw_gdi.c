/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/

#include "gdi/gdi_sw_winsys.h"
#include "pipe/p_screen.h"
#include "state_tracker/graw.h"
#include "target-helpers/inline_debug_helper.h"
#include "target-helpers/inline_sw_helper.h"
#include <windows.h>


static LRESULT CALLBACK
window_proc(HWND hWnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
   switch (uMsg) {
   case WM_DESTROY:
      PostQuitMessage(0);
      break;

   default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }

   return 0;
}

static struct {
   void (* draw)(void);
} graw;

struct pipe_screen *
graw_create_window_and_screen(int x,
                              int y,
                              unsigned width,
                              unsigned height,
                              enum pipe_format format,
                              void **handle)
{
   struct sw_winsys *winsys = NULL;
   struct pipe_screen *screen = NULL;
   WNDCLASSEX wc;
   UINT style = WS_VISIBLE | WS_TILEDWINDOW;
   RECT rect;
   HWND hWnd = NULL;
   HDC hDC = NULL;

   if (format != PIPE_FORMAT_R8G8B8A8_UNORM)
      goto fail;

   winsys = gdi_create_sw_winsys();
   if (winsys == NULL)
      goto fail;

   screen = sw_screen_create(winsys);
   if (screen == NULL)
      goto fail;

   memset(&wc, 0, sizeof wc);
   wc.cbSize = sizeof wc;
   wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = window_proc;
   wc.lpszClassName = "graw-gdi";
   wc.hInstance = GetModuleHandle(NULL);
   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
   RegisterClassEx(&wc);

   SetRect(&rect, 0, 0, width, height);
   AdjustWindowRectEx(&rect, style, FALSE, 0);

   hWnd = CreateWindowEx(0,
                         wc.lpszClassName,
                         wc.lpszClassName,
                         style,
                         x,
                         y,
                         rect.right - rect.left,
                         rect.bottom - rect.top,
                         NULL,
                         NULL,
                         wc.hInstance,
                         0);
   if (hWnd == NULL)
      goto fail;

   hDC = GetDC(hWnd);
   if (hDC == NULL)
      goto fail;

   *handle = (void *)hDC;

   return debug_screen_wrap(screen);

fail:
   if (hWnd)
      DestroyWindow(hWnd);

   if (screen)
      screen->destroy(screen);

   return NULL;
}

void 
graw_set_display_func(void (* draw)(void))
{
   graw.draw = draw;
}

void
graw_main_loop(void)
{
   for (;;) {
      MSG msg;

      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
         if (msg.message == WM_QUIT) {
            return;
         }
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      if (graw.draw) {
         graw.draw();
      }

      Sleep(0);
   }
}
