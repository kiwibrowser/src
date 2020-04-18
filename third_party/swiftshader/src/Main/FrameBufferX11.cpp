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

#include "FrameBufferX11.hpp"

#include "libX11.hpp"
#include "Common/Timer.hpp"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <assert.h>

namespace sw
{
	static int (*PreviousXErrorHandler)(Display *display, XErrorEvent *event) = 0;
	static bool shmBadAccess = false;

	// Catches BadAcces errors so we can fall back to not using MIT-SHM
	static int XShmErrorHandler(Display *display, XErrorEvent *event)
	{
		if(event->error_code == BadAccess)
		{
			shmBadAccess = true;
			return 0;
		}
		else
		{
			return PreviousXErrorHandler(display, event);
		}
	}

	FrameBufferX11::FrameBufferX11(Display *display, Window window, int width, int height) : FrameBuffer(width, height, false, false), ownX11(!display), x_display(display), x_window(window)
	{
		if(!x_display)
		{
			x_display = libX11->XOpenDisplay(0);
		}

		int screen = DefaultScreen(x_display);
		x_gc = libX11->XDefaultGC(x_display, screen);
		int depth = libX11->XDefaultDepth(x_display, screen);

		Status status = libX11->XMatchVisualInfo(x_display, screen, 32, TrueColor, &x_visual);
		bool match = (status != 0 && x_visual.blue_mask == 0xFF);   // Prefer X8R8G8B8
		Visual *visual = match ? x_visual.visual : libX11->XDefaultVisual(x_display, screen);

		mit_shm = (libX11->XShmQueryExtension && libX11->XShmQueryExtension(x_display) == True);

		if(mit_shm)
		{
			x_image = libX11->XShmCreateImage(x_display, visual, depth, ZPixmap, 0, &shminfo, width, height);

			shminfo.shmid = shmget(IPC_PRIVATE, x_image->bytes_per_line * x_image->height, IPC_CREAT | SHM_R | SHM_W);
			shminfo.shmaddr = x_image->data = buffer = (char*)shmat(shminfo.shmid, 0, 0);
			shminfo.readOnly = False;

			PreviousXErrorHandler = libX11->XSetErrorHandler(XShmErrorHandler);
			libX11->XShmAttach(x_display, &shminfo);   // May produce a BadAccess error
			libX11->XSync(x_display, False);
			libX11->XSetErrorHandler(PreviousXErrorHandler);

			if(shmBadAccess)
			{
				mit_shm = false;

				XDestroyImage(x_image);
				shmdt(shminfo.shmaddr);
				shmctl(shminfo.shmid, IPC_RMID, 0);

				shmBadAccess = false;
			}
		}

		if(!mit_shm)
		{
			int bytes_per_line = width * 4;
			int bytes_per_image = height * bytes_per_line;
			buffer = new char[bytes_per_image];
			memset(buffer, 0, bytes_per_image);
			x_image = libX11->XCreateImage(x_display, visual, depth, ZPixmap, 0, buffer, width, height, 32, bytes_per_line);
		}
	}

	FrameBufferX11::~FrameBufferX11()
	{
		if(!mit_shm)
		{
			x_image->data = 0;
			XDestroyImage(x_image);

			delete[] buffer;
			buffer = 0;
		}
		else
		{
			libX11->XShmDetach(x_display, &shminfo);
			XDestroyImage(x_image);
			shmdt(shminfo.shmaddr);
			shmctl(shminfo.shmid, IPC_RMID, 0);
		}

		if(ownX11)
		{
			libX11->XCloseDisplay(x_display);
		}
	}

	void *FrameBufferX11::lock()
	{
		stride = x_image->bytes_per_line;
		framebuffer = buffer;

		return framebuffer;
	}

	void FrameBufferX11::unlock()
	{
		framebuffer = nullptr;
	}

	void FrameBufferX11::blit(sw::Surface *source, const Rect *sourceRect, const Rect *destRect)
	{
		copy(source);

		if(!mit_shm)
		{
			libX11->XPutImage(x_display, x_window, x_gc, x_image, 0, 0, 0, 0, width, height);
		}
		else
		{
			libX11->XShmPutImage(x_display, x_window, x_gc, x_image, 0, 0, 0, 0, width, height, False);
		}

		libX11->XSync(x_display, False);

		if(false)   // Draw the framerate on screen
		{
			static double fpsTime = sw::Timer::seconds();
			static int frames = -1;

			double time = sw::Timer::seconds();
			double delta = time - fpsTime;
			frames++;

			static double FPS = 0.0;
			static double maxFPS = 0.0;

			if(delta > 1.0)
			{
				FPS = frames / delta;

				fpsTime = time;
				frames = 0;

				if(FPS > maxFPS)
				{
					maxFPS = FPS;
				}
			}

			char string[256];
			sprintf(string, "FPS: %.2f (max: %.2f)", FPS, maxFPS);
			libX11->XDrawString(x_display, x_window, x_gc, 50, 50, string, strlen(string));
		}
	}
}

NO_SANITIZE_FUNCTION sw::FrameBuffer *createFrameBuffer(void *display, Window window, int width, int height)
{
	return new sw::FrameBufferX11((::Display*)display, window, width, height);
}
