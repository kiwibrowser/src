/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_PTHREAD_PTHREAD_INTERNAL_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_PTHREAD_PTHREAD_INTERNAL_H_ 1

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/tls.h"
#include "native_client/src/untrusted/nacl/tls_params.h"
#include "native_client/src/untrusted/pthread/pthread.h"
#include "native_client/src/untrusted/pthread/pthread_types.h"

extern int __nc_thread_initialized;
extern pthread_t __nc_initial_thread_id;

void __nc_initialize_globals(void);

void __nc_initialize_unjoinable_thread(struct nc_combined_tdb *tdb);

void __nc_initialize_interfaces(void);

void __nc_tsd_exit(void);

#endif
