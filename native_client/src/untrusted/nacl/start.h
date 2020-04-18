/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_NACL_START_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_NACL_START_H_ 1

int main(int argc, char **argv, char **envp);

extern void *__nacl_initial_thread_stack_end;

#endif
