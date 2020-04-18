/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RESOLV_H_
#define _RESOLV_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/nameser.h>
#include <netinet/in.h>

__BEGIN_DECLS

#define b64_ntop __b64_ntop
int b64_ntop(u_char const* __src, size_t __src_size, char* __dst, size_t __dst_size);
#define b64_pton __b64_pton
int b64_pton(char const* __src, u_char* __dst, size_t __dst_size);

#define dn_comp __dn_comp
int dn_comp(const char* __src, u_char* __dst, int __dst_size, u_char** __dn_ptrs , u_char** __last_dn_ptr);

int dn_expand(const u_char* __msg, const u_char* __eom, const u_char* __src, char* __dst, int __dst_size);

#define p_class __p_class
const char* p_class(int __class);
#define p_type __p_type
const char* p_type(int __type);

int res_init(void);
int res_mkquery(int __opcode, const char* __domain_name, int __class, int __type, const u_char* __data, int __data_size, const u_char* __new_rr_in, u_char* __buf, int __buf_size);
int res_query(const char* __name, int __class, int __type, u_char* __answer, int __answer_size);
int res_search(const char* __name, int __class, int __type, u_char* __answer, int __answer_size);

__END_DECLS

#endif
