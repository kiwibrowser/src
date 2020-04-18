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

#ifndef	sw_OSXUtils_hpp
#define	sw_OSXUtils_hpp

#include <EGL/egl.h>

namespace sw
{
namespace OSX
{
	bool IsValidWindow(EGLNativeWindowType window);
	void GetNativeWindowSize(EGLNativeWindowType window, int &width, int &height);
}
}

#endif // sw_OSXUtils_hpp
