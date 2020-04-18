/*-
 * Copyright (c) 1989, 1993
 *    The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _GRP_H_
#define _GRP_H_

#include <sys/cdefs.h>
#include <sys/types.h>

struct group {
  char* gr_name; /* group name */
  char* gr_passwd; /* group password */
  gid_t gr_gid; /* group id */
  char** gr_mem; /* group members */
};

__BEGIN_DECLS

struct group* getgrgid(gid_t __gid);
struct group* getgrnam(const char* __name);

/* Note: Android has thousands and thousands of ids to iterate through. */

#if __ANDROID_API__ >= 26
struct group* getgrent(void) __INTRODUCED_IN(26);

void setgrent(void) __INTRODUCED_IN(26);
void endgrent(void) __INTRODUCED_IN(26);
#endif /* __ANDROID_API__ >= 26 */


#if __ANDROID_API__ >= 24
int getgrgid_r(gid_t __gid, struct group* __group, char* __buf, size_t __n, struct group** __result) __INTRODUCED_IN(24);
int getgrnam_r(const char* __name, struct group* __group, char* __buf, size_t __n, struct group** __result) __INTRODUCED_IN(24);
#endif /* __ANDROID_API__ >= 24 */

int getgrouplist(const char* __user, gid_t __group, gid_t* __groups, int* __group_count);
int initgroups(const char* __user, gid_t __group);

__END_DECLS

#endif
