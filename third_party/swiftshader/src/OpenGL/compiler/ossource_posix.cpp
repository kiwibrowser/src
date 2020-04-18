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

//
// This file contains the posix specific functions
//
#include "osinclude.h"

#if !defined(ANGLE_OS_POSIX)
#error Trying to build a posix specific file in a non-posix build.
#endif

//
// Thread Local Storage Operations
//
OS_TLSIndex OS_AllocTLSIndex()
{
	pthread_key_t pPoolIndex;

	//
	// Create global pool key.
	//
	if ((pthread_key_create(&pPoolIndex, NULL)) != 0) {
		assert(0 && "OS_AllocTLSIndex(): Unable to allocate Thread Local Storage");
		return false;
	}
	else {
		return pPoolIndex;
	}
}


bool OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue)
{
	if (nIndex == OS_INVALID_TLS_INDEX) {
		assert(0 && "OS_SetTLSValue(): Invalid TLS Index");
		return false;
	}

	if (pthread_setspecific(nIndex, lpvValue) == 0)
		return true;
	else
		return false;
}


bool OS_FreeTLSIndex(OS_TLSIndex nIndex)
{
	if (nIndex == OS_INVALID_TLS_INDEX) {
		assert(0 && "OS_SetTLSValue(): Invalid TLS Index");
		return false;
	}

	//
	// Delete the global pool key.
	//
	if (pthread_key_delete(nIndex) == 0)
		return true;
	else
		return false;
}
