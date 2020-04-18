/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief CTS Android entry points.
 */ /*-------------------------------------------------------------------*/

#include "glcAndroidTestActivity.hpp"

#if (DE_OS != DE_OS_ANDROID)
#error "Invalid OS"
#elif (DE_ANDROID_API < 9)
#error "CTS runner requires API level 9 or greater"
#endif

static void createCTSActivity(ANativeActivity* activity, void* savedState, size_t savedStateSize, glu::ApiType type)
{
	DE_UNREF(savedState && savedStateSize);
	try
	{
		glcts::Android::TestActivity* obj = new glcts::Android::TestActivity(activity, type);
		DE_UNREF(obj);
	}
	catch (const std::exception& e)
	{
		tcu::die("Failed to create activity: %s", e.what());
	}
}

DE_BEGIN_EXTERN_C

JNIEXPORT void JNICALL createES2CTSActivity(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	createCTSActivity(activity, savedState, savedStateSize, glu::ApiType::es(2, 0));
}

JNIEXPORT void JNICALL createES3CTSActivity(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	createCTSActivity(activity, savedState, savedStateSize, glu::ApiType::es(3, 0));
}

JNIEXPORT void JNICALL createES31CTSActivity(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	createCTSActivity(activity, savedState, savedStateSize, glu::ApiType::es(3, 1));
}

JNIEXPORT void JNICALL createES32CTSActivity(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	createCTSActivity(activity, savedState, savedStateSize, glu::ApiType::es(3, 2));
}

JNIEXPORT void JNICALL createGL45CTSActivity(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	createCTSActivity(activity, savedState, savedStateSize, glu::ApiType::core(4, 5));
}

JNIEXPORT void JNICALL createGL46CTSActivity(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	createCTSActivity(activity, savedState, savedStateSize, glu::ApiType::core(4, 6));
}
DE_END_EXTERN_C
