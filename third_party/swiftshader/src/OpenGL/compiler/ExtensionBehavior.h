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

#ifndef _EXTENSION_BEHAVIOR_INCLUDED_
#define _EXTENSION_BEHAVIOR_INCLUDED_

#include <map>
#include <string>

typedef enum
{
	EBhRequire,
	EBhEnable,
	EBhWarn,
	EBhDisable,
	EBhUndefined
} TBehavior;

inline const char *getBehaviorString(TBehavior b)
{
	switch(b)
	{
	case EBhRequire: return "require";
	case EBhEnable:  return "enable";
	case EBhWarn:    return "warn";
	case EBhDisable: return "disable";
	default:         return nullptr;
	}
}

// Mapping between extension name and behavior.
typedef std::map<std::string, TBehavior> TExtensionBehavior;

#endif // _EXTENSION_TABLE_INCLUDED_
