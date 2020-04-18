/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl semaphore implementation (Windows)
 */

#include <windows.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/win/nacl_semaphore.h"


/* Generic test for success on any status value (non-negative numbers
 * indicate success).
 */
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

/* semaphore object query structure */

typedef struct _SEMAINFO {
  unsigned int count;   /* current semaphore count */
  unsigned int limit;   /* max semaphore count */
} SEMAINFO, *PSEMAINFO;

/* only one query type for semaphores */
#define SEMAQUERYINFOCLASS  0
#define WINAPI __stdcall

/* NT Function call. */
NTSTATUS WINAPI NtQuerySemaphore(HANDLE       Handle,
                                 unsigned int InfoClass,
                                 PSEMAINFO    SemaInfo,
                                 unsigned int InfoSize,
                                 unsigned int *RetLen);



int NaClSemCtor(struct NaClSemaphore *sem, int32_t value) {
  sem->sem_handle = CreateSemaphore(NULL, value, SEM_VALUE_MAX, NULL);
  if (NULL == sem->sem_handle) {
    return 0;
  }
  sem->interrupt_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (NULL == sem->interrupt_event) {
    CloseHandle(sem->sem_handle);
    return 0;
  }
  return 1;
}

void NaClSemDtor(struct NaClSemaphore *sem) {
  CloseHandle(sem->sem_handle);
  CloseHandle(sem->interrupt_event);
}

NaClSyncStatus NaClSemWait(struct NaClSemaphore *sem) {
  DWORD rv;
  NaClSyncStatus status;
  HANDLE handles[2];
  handles[0] = sem->sem_handle;
  handles[1] = sem->interrupt_event;

  rv = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
  switch (rv) {
    case WAIT_OBJECT_0:
      status = NACL_SYNC_OK;
      break;
    case WAIT_OBJECT_0 + 1:
      status = NACL_SYNC_SEM_INTERRUPTED;
      break;
    default:
      status = NACL_SYNC_INTERNAL_ERROR;
  }
  return status;
}

NaClSyncStatus NaClSemTryWait(struct NaClSemaphore *sem) {
  DWORD rv;
  rv = WaitForSingleObject(sem->sem_handle, 0);
  return (rv == WAIT_OBJECT_0) ? NACL_SYNC_OK : NACL_SYNC_BUSY;
}

NaClSyncStatus NaClSemPost(struct NaClSemaphore *sem) {
  if (ReleaseSemaphore(sem->sem_handle, 1, NULL)) {
    return NACL_SYNC_OK;
  }
  if (ERROR_TOO_MANY_POSTS == GetLastError()) {
    return NACL_SYNC_SEM_RANGE_ERROR;
  }
  return NACL_SYNC_INTERNAL_ERROR;
}

int32_t NaClSemGetValue(struct NaClSemaphore *sem) {
  /*
   * TODO(greogyrd): please uncomment these decls when they're needed / when
   * the code below is re-enabled.  These were commented-out to eliminate
   * windows compiler warnings.
  SEMAINFO    sem_info;
  UINT        ret_length;
  NTSTATUS    status;
   */
  int32_t     count = -1;
  UNREFERENCED_PARAMETER(sem);
/* TODO(gregoryd): cannot use NtQuerySemaphore without linking to ntdll.lib
  status = NtQuerySemaphore(
    sem->sem_handle,
    SEMAQUERYINFOCLASS,
    &sem_info,
    sizeof sem_info,
    &ret_length
    );

  if (!NT_SUCCESS(status)) {
    count = -1;
  } else {
    count = sem_info.count;
  }
*/
  return count;
}

void NaClSemIntr(struct NaClSemaphore *sem) {
  SetEvent(sem->interrupt_event);
}

void NaClSemReset(struct NaClSemaphore *sem) {
  ResetEvent(sem->interrupt_event);
}
