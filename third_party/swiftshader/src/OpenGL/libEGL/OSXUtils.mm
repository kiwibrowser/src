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

#include "OSXUtils.hpp"

#include "common/debug.h"

#import <Cocoa/Cocoa.h>

namespace sw
{
namespace OSX
{
	bool IsValidWindow(EGLNativeWindowType window)
	{
		NSObject *object = reinterpret_cast<NSObject*>(window);
		return window && ([object isKindOfClass:[NSView class]] || [object isKindOfClass:[CALayer class]]);
	}

	void GetNativeWindowSize(EGLNativeWindowType window, int &width, int &height)
	{
		NSObject *object = reinterpret_cast<NSObject*>(window);

		if([object isKindOfClass:[NSView class]])
		{
			NSView *view = reinterpret_cast<NSView*>(object);
			width = [view convertRectToBacking:[view bounds]].size.width;
			height = [view convertRectToBacking:[view bounds]].size.height;
		}
		else if([object isKindOfClass:[CALayer class]])
		{
			CALayer *layer = reinterpret_cast<CALayer*>(object);
			width = [layer bounds].size.width * layer.contentsScale;
			height = [layer bounds].size.height * layer.contentsScale;
		}
		else UNREACHABLE(0);
	}
}
}
