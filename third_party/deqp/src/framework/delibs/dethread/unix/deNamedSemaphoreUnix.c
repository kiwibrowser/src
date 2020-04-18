/*-------------------------------------------------------------------------
 * drawElements Thread Library
 * ---------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 * \brief Unix implementation of semaphore using named semaphores.
 *//*--------------------------------------------------------------------*/

#include "deSemaphore.h"

#if (DE_OS == DE_OS_IOS || DE_OS == DE_OS_OSX)

#include "deMemory.h"
#include "deString.h"

#include <semaphore.h>
#include <unistd.h>

typedef struct NamedSemaphore_s
{
	sem_t*	semaphore;
} NamedSemaphore;

static void NamedSemaphore_getName (const NamedSemaphore* sem, char* buf, int bufSize)
{
	deSprintf(buf, bufSize, "/desem-%d-%p", getpid(), (void*)sem);
}

DE_STATIC_ASSERT(sizeof(deSemaphore) >= sizeof(NamedSemaphore*));

deSemaphore deSemaphore_create (int initialValue, const deSemaphoreAttributes* attributes)
{
	NamedSemaphore*	sem		= (NamedSemaphore*)deCalloc(sizeof(NamedSemaphore));
	char			name[128];
	deUint32		mode	= 0700;

	DE_UNREF(attributes);

	if (!sem)
		return 0;

	NamedSemaphore_getName(sem, name, DE_LENGTH_OF_ARRAY(name));

	sem->semaphore = sem_open(name, O_CREAT|O_EXCL, mode, initialValue);

	if (sem->semaphore == SEM_FAILED)
	{
		deFree(sem);
		return 0;
	}

	return (deSemaphore)sem;
}

void deSemaphore_destroy (deSemaphore semaphore)
{
	NamedSemaphore* sem			= (NamedSemaphore*)semaphore;
	char			name[128];
	int				res;

	NamedSemaphore_getName(sem, name, DE_LENGTH_OF_ARRAY(name));

	res = sem_close(sem->semaphore);
	DE_ASSERT(res == 0);
	res = sem_unlink(name);
	DE_ASSERT(res == 0);
	DE_UNREF(res);
	deFree(sem);
}

void deSemaphore_increment (deSemaphore semaphore)
{
	sem_t*	sem		= ((NamedSemaphore*)semaphore)->semaphore;
	int		res		= sem_post(sem);
	DE_ASSERT(res == 0);
	DE_UNREF(res);
}

void deSemaphore_decrement (deSemaphore semaphore)
{
	sem_t*	sem		= ((NamedSemaphore*)semaphore)->semaphore;
	int		res		= sem_wait(sem);
	DE_ASSERT(res == 0);
	DE_UNREF(res);
}

deBool deSemaphore_tryDecrement (deSemaphore semaphore)
{
	sem_t* sem = ((NamedSemaphore*)semaphore)->semaphore;
	return (sem_trywait(sem) == 0);
}

#endif /* DE_OS */
