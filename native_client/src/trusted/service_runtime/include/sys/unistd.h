/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file is based on the file "newlib/libc/include/sys/unistd.h"
 * from newlib.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_UNISTD_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_UNISTD_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * We need this file in the service_runtime only to agree on enum values,
 * therefore most of it should be compiled only for NaCl applications
 */
#if defined(NACL_IN_TOOLCHAIN_HEADERS)
#include <_ansi.h>
#include <sys/types.h>
#include <sys/_types.h>
#define __need_size_t
#define __need_ptrdiff_t
#include <stddef.h>
#include <stdint.h>

extern char **environ;

void _EXFUN(_exit, (int __status ) _ATTRIBUTE ((noreturn)));

int _EXFUN(access,(const char *__path, int __amode ));
unsigned _EXFUN(alarm, (unsigned __secs ));
int     _EXFUN(chdir, (const char *__path ));
int     _EXFUN(chmod, (const char *__path, mode_t __mode ));
#if !defined(__INSIDE_CYGWIN__)
int     _EXFUN(chown, (const char *__path, uid_t __owner, gid_t __group ));
#endif
#if defined(__CYGWIN__) || defined(__rtems__)
int     _EXFUN(chroot, (const char *__path ));
#endif
int     _EXFUN(close, (int __fildes ));
#if defined(__CYGWIN__)
size_t _EXFUN(confstr, (int __name, char *__buf, size_t __len));
#endif
char    _EXFUN(*ctermid, (char *__s ));
char    _EXFUN(*cuserid, (char *__s ));
#if defined(__CYGWIN__)
int _EXFUN(daemon, (int nochdir, int noclose));
#endif
int     _EXFUN(dup, (int __fildes ));
int     _EXFUN(dup2, (int __fildes, int __fildes2 ));
#if defined(__CYGWIN__)
void _EXFUN(endusershell, (void));
#endif
int     _EXFUN(execl, (const char *__path, const char *, ... ));
int     _EXFUN(execle, (const char *__path, const char *, ... ));
int     _EXFUN(execlp, (const char *__file, const char *, ... ));
int     _EXFUN(execv, (const char *__path, char * const __argv[] ));
int     _EXFUN(execve, (const char *__path, char * const __argv[], char * const __envp[] ));
int     _EXFUN(execvp, (const char *__file, char * const __argv[] ));
int     _EXFUN(fchdir, (int __fildes));
int     _EXFUN(fchmod, (int __fildes, mode_t __mode ));
#if !defined(__INSIDE_CYGWIN__)
int     _EXFUN(fchown, (int __fildes, uid_t __owner, gid_t __group ));
#endif
pid_t   _EXFUN(fork, (void ));
long    _EXFUN(fpathconf, (int __fd, int __name ));
int     _EXFUN(fsync, (int __fd));
int     _EXFUN(fdatasync, (int __fd));
char    _EXFUN(*getcwd, (char *__buf, size_t __size ));
#if defined(__CYGWIN__)
int _EXFUN(getdomainname ,(char *__name, size_t __len));
#endif
#if !defined(__INSIDE_CYGWIN__)
gid_t   _EXFUN(getegid, (void ));
uid_t   _EXFUN(geteuid, (void ));
gid_t   _EXFUN(getgid, (void ));
#endif
int     _EXFUN(getgroups, (int __gidsetsize, gid_t __grouplist[] ));
#if defined(__CYGWIN__)
long    _EXFUN(gethostid, (void));
#endif
char    _EXFUN(*getlogin, (void ));
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
int _EXFUN(getlogin_r, (char *name, size_t namesize) );
#endif
char _EXFUN(*getpass, (const char *__prompt));
int     _EXFUN(getpagesize, (void));
#if defined(__CYGWIN__)
int    _EXFUN(getpeereid, (int, uid_t *, gid_t *));
#endif
pid_t   _EXFUN(getpgid, (pid_t));
pid_t   _EXFUN(getpgrp, (void ));
pid_t   _EXFUN(getpid, (void ));
pid_t   _EXFUN(getppid, (void ));
#ifdef __CYGWIN__
pid_t   _EXFUN(getsid, (pid_t));
#endif
#if !defined(__INSIDE_CYGWIN__)
uid_t   _EXFUN(getuid, (void ));
#endif
#ifdef __CYGWIN__
char *_EXFUN(getusershell, (void));
char    _EXFUN(*getwd, (char *__buf ));
int _EXFUN(iruserok, (unsigned long raddr, int superuser, const char *ruser, const char *luser));
#endif
int     _EXFUN(isatty, (int __fildes ));
#if !defined(__INSIDE_CYGWIN__)
int     _EXFUN(lchown, (const char *__path, uid_t __owner, gid_t __group ));
#endif
int     _EXFUN(link, (const char *__path1, const char *__path2 ));
int _EXFUN(nice, (int __nice_value ));
#if !defined(__INSIDE_CYGWIN__)
off_t   _EXFUN(lseek, (int __fildes, off_t __offset, int __whence ));
#endif
#if defined(__SPU__)
#define F_ULOCK 0
#define F_LOCK  1
#define F_TLOCK 2
#define F_TEST  3
int     _EXFUN(lockf, (int __fd, int __cmd, off_t __len));
#endif
long    _EXFUN(pathconf, (const char *__path, int __name ));
int     _EXFUN(pause, (void ));
#ifdef __CYGWIN__
int _EXFUN(pthread_atfork, (void (*)(void), void (*)(void), void (*)(void)));
#endif
int     _EXFUN(pipe, (int __fildes[2]));
int     _EXFUN(pipe2, (int __fildes[2], int __flags));
ssize_t _EXFUN(pread, (int __fd, void *__buf, size_t __nbytes, off_t __offset));
ssize_t _EXFUN(pwrite, (int __fd, const void *__buf, size_t __nbytes, off_t __offset));
_READ_WRITE_RETURN_TYPE _EXFUN(read, (int __fd, void *__buf, size_t __nbyte ));
#if defined(__CYGWIN__)
int _EXFUN(rresvport, (int *__alport));
int _EXFUN(revoke, (char *__path));
#endif
int     _EXFUN(rmdir, (const char *__path ));
#if defined(__CYGWIN__)
int _EXFUN(ruserok, (const char *rhost, int superuser, const char *ruser, const char *luser));
#endif
void *  _EXFUN(sbrk,  (intptr_t __incr));
#if !defined(__INSIDE_CYGWIN__)
#if defined(__CYGWIN__)
int     _EXFUN(setegid, (gid_t __gid ));
int     _EXFUN(seteuid, (uid_t __uid ));
#endif
int     _EXFUN(setgid, (gid_t __gid ));
#endif
#if defined(__CYGWIN__)
int _EXFUN(setgroups, (int ngroups, const gid_t *grouplist ));
#endif
int     _EXFUN(setpgid, (pid_t __pid, pid_t __pgid ));
int     _EXFUN(setpgrp, (void ));
#if defined(__CYGWIN__) && !defined(__INSIDE_CYGWIN__)
int _EXFUN(setregid, (gid_t __rgid, gid_t __egid));
int _EXFUN(setreuid, (uid_t __ruid, uid_t __euid));
#endif
pid_t   _EXFUN(setsid, (void ));
#if !defined(__INSIDE_CYGWIN__)
int     _EXFUN(setuid, (uid_t __uid ));
#endif
#if defined(__CYGWIN__)
void _EXFUN(setusershell, (void));
#endif
unsigned _EXFUN(sleep, (unsigned int __seconds ));
void    _EXFUN(swab, (const void *, void *, ssize_t));
long    _EXFUN(sysconf, (int __name ));
pid_t   _EXFUN(tcgetpgrp, (int __fildes ));
int     _EXFUN(tcsetpgrp, (int __fildes, pid_t __pgrp_id ));
char    _EXFUN(*ttyname, (int __fildes ));
#if defined(__CYGWIN__)
int     _EXFUN(ttyname_r, (int, char *, size_t));
#endif
int     _EXFUN(unlink, (const char *__path ));
int _EXFUN(usleep, (useconds_t __useconds));
int     _EXFUN(vhangup, (void ));
_READ_WRITE_RETURN_TYPE _EXFUN(write, (int __fd, const void *__buf, size_t __nbyte ));

#ifdef __CYGWIN__
# define __UNISTD_GETOPT__
# include <getopt.h>
# undef __UNISTD_GETOPT__
#else
extern char *optarg;  /* getopt(3) external variables */
extern int optind, opterr, optopt;
int  getopt(int, char * const [], const char *);
extern int optreset;  /* getopt(3) external variable */
#endif

#ifndef        _POSIX_SOURCE
pid_t   _EXFUN(vfork, (void ));

extern char *suboptarg;  /* getsubopt(3) external variable */
int getsubopt(char **, char * const *, char **);
#endif /* _POSIX_SOURCE */

#ifdef _COMPILING_NEWLIB
/* Provide prototypes for most of the _<systemcall> names that are
   provided in newlib for some compilers.  */
int     _EXFUN(_close, (int __fildes ));
pid_t   _EXFUN(_fork, (void ));
pid_t   _EXFUN(_getpid, (void ));
int     _EXFUN(_link, (const char *__path1, const char *__path2 ));
_off_t   _EXFUN(_lseek, (int __fildes, _off_t __offset, int __whence ));
#ifdef __LARGE64_FILES
_off64_t _EXFUN(_lseek64, (int __filedes, _off64_t __offset, int __whence ));
#endif
_READ_WRITE_RETURN_TYPE _EXFUN(_read, (int __fd, void *__buf, size_t __nbyte ));
void *  _EXFUN(_sbrk,  (ptrdiff_t __incr));
int     _EXFUN(_unlink, (const char *__path ));
_READ_WRITE_RETURN_TYPE _EXFUN(_write, (int __fd, const void *__buf, size_t __nbyte ));
int     _EXFUN(_execve, (const char *__path, char * const __argv[], char * const __envp[] ));
#endif

#if defined(__CYGWIN__) || defined(__rtems__) || defined(__sh__) \
    || defined(__SPU__) || defined(NACL_IN_TOOLCHAIN_HEADERS)
#if !defined(__INSIDE_CYGWIN__)
int     _EXFUN(ftruncate, (int __fd, off_t __length));
int     _EXFUN(truncate, (const char *, off_t __length));
#endif
#endif

#if defined(__CYGWIN__) || defined(__rtems__) || \
    defined(NACL_IN_TOOLCHAIN_HEADERS)
int _EXFUN(getdtablesize, (void));
int _EXFUN(setdtablesize, (int));
useconds_t _EXFUN(ualarm, (useconds_t __useconds, useconds_t __interval));
#if !(defined  (_WINSOCK_H) || defined (__USE_W32_SOCKETS))
/* winsock[2].h defines as __stdcall, and with int as 2nd arg */
int _EXFUN(gethostname, (char *__name, size_t __len));
#endif
char *_EXFUN(mktemp, (char *));
#endif

#if defined(__CYGWIN__) || defined(__SPU__)
void    _EXFUN(sync, (void));
#elif defined(__rtems__)
int     _EXFUN(sync, (void));
#endif

int     _EXFUN(readlink, (const char *__path, char *__buf, int __buflen));
int     _EXFUN(symlink, (const char *__name1, const char *__name2));

# define SEEK_SET 0
# define SEEK_CUR 1
# define SEEK_END 2

#include <sys/features.h>

#define STDIN_FILENO    0       /* standard input file descriptor */
#define STDOUT_FILENO   1       /* standard output file descriptor */
#define STDERR_FILENO   2       /* standard error file descriptor */

#endif  /* defined(NACL_IN_TOOLCHAIN_HEADERS) */

#define NACL_ABI_F_OK 0
#define NACL_ABI_R_OK 4
#define NACL_ABI_W_OK 2
#define NACL_ABI_X_OK 1

/*
 * sysconf values as supported by NativeClient
 *
 * These configuration names are defined within an enum so that
 * -Wswitch-enum etc can be useful to detect bugs.  They are also an
 * identity macro definition -- explicitly defined by Posix as allowed
 * -- so that code can be written using #ifdef conditionals to do
 * simple code configuration based on whether a confname is available
 * (and typically presumed to be implemented).  (This is in contrast
 * to autoconf-style configurations, which may involve compiling a
 * short program to exercise functionality to determine if a feature
 * is implemented.)
 */
enum {
  NACL_ABI__SC_NPROCESSORS_ONLN = 1,
#define NACL_ABI__SC_NPROCESSORS_ONLN NACL_ABI__SC_NPROCESSORS_ONLN
  NACL_ABI__SC_PAGESIZE = 2,
#define NACL_ABI__SC_PAGESIZE NACL_ABI__SC_PAGESIZE
  NACL_ABI__SC_NACL_CPU_FEATURE_X86 = 1 << 16,
#define NACL_ABI__SC_NACL_CPU_FEATURE_X86 \
    NACL_ABI__SC_NACL_CPU_FEATURE_X86

  /*
   * The sysconf values below are not part of the stable ABI.
   */
  NACL_ABI__SC_NACL_FILE_ACCESS_ENABLED = 1 << 24,
#define NACL_ABI__SC_NACL_FILE_ACCESS_ENABLED \
    NACL_ABI__SC_NACL_FILE_ACCESS_ENABLED
  NACL_ABI__SC_NACL_LIST_MAPPINGS_ENABLED,
#define NACL_ABI__SC_NACL_LIST_MAPPINGS_ENABLED \
    NACL_ABI__SC_NACL_LIST_MAPPINGS_ENABLED
  NACL_ABI__SC_NACL_PNACL_MODE,
#define NACL_ABI__SC_NACL_PNACL_MODE \
    NACL_ABI__SC_NACL_PNACL_MODE
};

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
/*
 * TODO(gregoryd) pathconf and confstr are not supported on NaCl,
 * consider removing the definitions below.
 */

/*
 *  pathconf values per IEEE Std 1003.1, 2004 Edition
 */

#define _PC_LINK_MAX                      0
#define _PC_MAX_CANON                     1
#define _PC_MAX_INPUT                     2
#define _PC_NAME_MAX                      3
#define _PC_PATH_MAX                      4
#define _PC_PIPE_BUF                      5
#define _PC_CHOWN_RESTRICTED              6
#define _PC_NO_TRUNC                      7
#define _PC_VDISABLE                      8
#define _PC_ASYNC_IO                      9
#define _PC_PRIO_IO                      10
#define _PC_SYNC_IO                      11
#define _PC_FILESIZEBITS                 12
#define _PC_2_SYMLINKS                   13
#define _PC_SYMLINK_MAX                  14
#ifdef __CYGWIN__
/* Ask for POSIX permission bits support. */
#define _PC_POSIX_PERMISSIONS            90
/* Ask for full POSIX permission support including uid/gid settings. */
#define _PC_POSIX_SECURITY               91
#endif

/*
 *  confstr values per IEEE Std 1003.1, 2004 Edition
 */

#ifdef __CYGWIN__ /* Only defined on Cygwin for now. */
#define _CS_PATH                               0
#define _CS_POSIX_V6_ILP32_OFF32_CFLAGS        1
#define _CS_XBS5_ILP32_OFF32_CFLAGS           _CS_POSIX_V6_ILP32_OFF32_CFLAGS
#define _CS_POSIX_V6_ILP32_OFF32_LDFLAGS       2
#define _CS_XBS5_ILP32_OFF32_LDFLAGS          _CS_POSIX_V6_ILP32_OFF32_LDFLAGS
#define _CS_POSIX_V6_ILP32_OFF32_LIBS          3
#define _CS_XBS5_ILP32_OFF32_LIBS             _CS_POSIX_V6_ILP32_OFF32_LIBS
#define _CS_XBS5_ILP32_OFF32_LINTFLAGS         4
#define _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS       5
#define _CS_XBS5_ILP32_OFFBIG_CFLAGS          _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS
#define _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS      6
#define _CS_XBS5_ILP32_OFFBIG_LDFLAGS         _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS
#define _CS_POSIX_V6_ILP32_OFFBIG_LIBS         7
#define _CS_XBS5_ILP32_OFFBIG_LIBS            _CS_POSIX_V6_ILP32_OFFBIG_LIBS
#define _CS_XBS5_ILP32_OFFBIG_LINTFLAGS        8
#define _CS_POSIX_V6_LP64_OFF64_CFLAGS         9
#define _CS_XBS5_LP64_OFF64_CFLAGS            _CS_POSIX_V6_LP64_OFF64_CFLAGS
#define _CS_POSIX_V6_LP64_OFF64_LDFLAGS       10
#define _CS_XBS5_LP64_OFF64_LDFLAGS           _CS_POSIX_V6_LP64_OFF64_LDFLAGS
#define _CS_POSIX_V6_LP64_OFF64_LIBS          11
#define _CS_XBS5_LP64_OFF64_LIBS              _CS_POSIX_V6_LP64_OFF64_LIBS
#define _CS_XBS5_LP64_OFF64_LINTFLAGS         12
#define _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS      13
#define _CS_XBS5_LPBIG_OFFBIG_CFLAGS          _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS
#define _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS     14
#define _CS_XBS5_LPBIG_OFFBIG_LDFLAGS         _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS
#define _CS_POSIX_V6_LPBIG_OFFBIG_LIBS        15
#define _CS_XBS5_LPBIG_OFFBIG_LIBS            _CS_POSIX_V6_LPBIG_OFFBIG_LIBS
#define _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS       16
#define _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS    17
#endif

#endif  /* defined(NACL_IN_TOOLCHAIN_HEADERS) */

#ifndef __CYGWIN__
# define MAXPATHLEN 1024
#endif

#ifdef __cplusplus
}
#endif
#endif /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_UNISTD_H_ */
