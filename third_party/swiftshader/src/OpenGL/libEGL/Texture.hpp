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

#ifndef egl_Texture_hpp
#define egl_Texture_hpp

#include "common/Object.hpp"

namespace sw
{
	class Resource;
}

namespace egl
{
class Texture : public gl::NamedObject
{
public:
	Texture(GLuint name) : NamedObject(name) {}

	virtual void releaseTexImage() = 0;
	virtual sw::Resource *getResource() const = 0;

	virtual void sweep() = 0;   // Garbage collect if no external references

	void release() override
	{
		int refs = dereference();

		if(refs > 0)
		{
			sweep();
		}
		else
		{
			delete this;
		}
	}
};
}

#endif   // egl_Texture_hpp
