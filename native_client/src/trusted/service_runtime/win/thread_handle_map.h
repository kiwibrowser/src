/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Header file for map from thread ids to thread handles.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_WIN_THREAD_HANDLE_MAP_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_WIN_THREAD_HANDLE_MAP_H_

#include "native_client/src/include/portability.h"
#include <windows.h>

typedef struct {
  DWORD id;
  HANDLE handle;
} ThreadHandleMapEntry;

typedef struct {
  ThreadHandleMapEntry *data;
  int size;
  int capacity;
} ThreadHandleMap;

#define THREAD_HANDLE_MAP_INIT_SIZE 4

ThreadHandleMap *CreateThreadHandleMap(void);
void DestroyThreadHandleMap(ThreadHandleMap *map);
int ThreadHandleMapPut(ThreadHandleMap *map,
                       DWORD thread_id,
                       HANDLE thread_handle);
HANDLE ThreadHandleMapGet(ThreadHandleMap *map, DWORD thread_id);
void ThreadHandleMapDelete(ThreadHandleMap *map, DWORD thread_id);
#endif
