
/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_THREAD_NICE_H__
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_THREAD_NICE_H__ 1

void NaClThreadNiceInit(void);

int nacl_thread_nice(int nacl_nice);

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_THREAD_NICE_H__ */
