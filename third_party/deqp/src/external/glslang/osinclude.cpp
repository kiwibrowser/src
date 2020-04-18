/*-------------------------------------------------------------------------
 * dEQP glslang integration
 * ------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 *//*!
 * \file
 * \brief glslang OS interface.
 *//*--------------------------------------------------------------------*/

#include "osinclude.h"

#include "deThread.h"
#include "deThreadLocal.h"
#include "deMutex.h"

namespace glslang
{

DE_STATIC_ASSERT(sizeof(deThreadLocal)	== sizeof(OS_TLSIndex));
DE_STATIC_ASSERT(sizeof(deThread)		== sizeof(void*));

// Thread-local

OS_TLSIndex OS_AllocTLSIndex (void)
{
	return (OS_TLSIndex)deThreadLocal_create();
}

bool OS_SetTLSValue (OS_TLSIndex nIndex, void* lpvValue)
{
	deThreadLocal_set((deThreadLocal)nIndex, lpvValue);
	return true;
}

bool OS_FreeTLSIndex (OS_TLSIndex nIndex)
{
	deThreadLocal_destroy((deThreadLocal)nIndex);
	return true;
}

void* OS_GetTLSValue (OS_TLSIndex nIndex)
{
	return deThreadLocal_get((deThreadLocal)nIndex);
}

// Global lock

static deMutex s_globalLock = 0;

void InitGlobalLock (void)
{
	DE_ASSERT(s_globalLock == 0);
	s_globalLock = deMutex_create(DE_NULL);
}

void GetGlobalLock (void)
{
	deMutex_lock(s_globalLock);
}

void ReleaseGlobalLock (void)
{
	deMutex_unlock(s_globalLock);
}

// Threading

DE_STATIC_ASSERT(sizeof(void*) >= sizeof(deThread));

static void EnterGenericThread (void* entry)
{
	((TThreadEntrypoint)entry)(DE_NULL);
}

void* OS_CreateThread (TThreadEntrypoint entry)
{
	return (void*)(deUintptr)deThread_create(EnterGenericThread, (void*)entry, DE_NULL);
}

void OS_WaitForAllThreads (void* threads, int numThreads)
{
	for (int ndx = 0; ndx < numThreads; ndx++)
	{
		const deThread thread = (deThread)(deUintptr)((void**)threads)[ndx];
		deThread_join(thread);
		deThread_destroy(thread);
	}
}

void OS_Sleep (int milliseconds)
{
	deSleep(milliseconds);
}

void OS_DumpMemoryCounters (void)
{
	// Not used
}

} // glslang
