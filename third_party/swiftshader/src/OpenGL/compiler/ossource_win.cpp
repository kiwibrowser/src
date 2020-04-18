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

#include "osinclude.h"
//
// This file contains contains the window's specific functions
//

#if !defined(ANGLE_OS_WIN)
#error Trying to build a windows specific file in a non windows build.
#endif


//
// Thread Local Storage Operations
//
OS_TLSIndex OS_AllocTLSIndex()
{
	DWORD dwIndex = TlsAlloc();
	if (dwIndex == TLS_OUT_OF_INDEXES) {
		assert(0 && "OS_AllocTLSIndex(): Unable to allocate Thread Local Storage");
		return OS_INVALID_TLS_INDEX;
	}

	return dwIndex;
}


bool OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue)
{
	if (nIndex == OS_INVALID_TLS_INDEX) {
		assert(0 && "OS_SetTLSValue(): Invalid TLS Index");
		return false;
	}

	if (TlsSetValue(nIndex, lpvValue))
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

	if (TlsFree(nIndex))
		return true;
	else
		return false;
}
