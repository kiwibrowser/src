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

// Display.h: Defines the Display class, representing the abstract
// display on which graphics are drawn.

#ifndef INCLUDE_DISPLAY_H_
#define INCLUDE_DISPLAY_H_

#include "Surface.h"
#include "Context.h"
#include "Device.hpp"

#include <set>

namespace gl
{
	struct DisplayMode
	{
		unsigned int width;
		unsigned int height;
		sw::Format format;
	};

	class Display
	{
	public:
		~Display();

		static Display *getDisplay(NativeDisplayType displayId);

		bool initialize();
		void terminate();

		Context *createContext(const Context *shareContext);

		void destroySurface(Surface *surface);
		void destroyContext(Context *context);

		bool isInitialized() const;
		bool isValidContext(Context *context);
		bool isValidSurface(Surface *surface);
		bool isValidWindow(NativeWindowType window);

		GLint getMinSwapInterval();
		GLint getMaxSwapInterval();

		virtual Surface *getPrimarySurface();

		NativeDisplayType getNativeDisplay() const;

	private:
		Display(NativeDisplayType displayId);

		DisplayMode getDisplayMode() const;

		const NativeDisplayType displayId;

		GLint mMaxSwapInterval;
		GLint mMinSwapInterval;

		typedef std::set<Surface*> SurfaceSet;
		SurfaceSet mSurfaceSet;

		typedef std::set<Context*> ContextSet;
		ContextSet mContextSet;
	};
}

#endif   // INCLUDE_DISPLAY_H_
