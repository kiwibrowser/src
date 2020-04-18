/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime API.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_ERRNO_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_ERRNO_H_

#if defined(NACL_IN_TOOLCHAIN_HEADERS)
#include <sys/reent.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _REENT_ONLY
#define errno (*__errno())
  extern int *__errno _PARAMS ((void));
#endif

/* Please don't use these variables directly.
 *    Use strerror instead. */
extern __IMPORT _CONST char * _CONST _sys_errlist[];
extern __IMPORT int _sys_nerr;

#define __errno_r(ptr) ((ptr)->_errno)

#ifdef __cplusplus
}
#endif

#endif  /* defined(NACL_IN_TOOLCHAIN_HEADERS) */

/*
 * NOTE: when adding new errnos here, check
 * service_runtime/nacl_host_desc_common.c and
 * service_runtime/win/xlate_system_error.c.
 */

/*
 * The errno values below 2048 here are the same as Linux's errno
 * values.  See Linux's asm-generic/errno-base.h and
 * asm-generic/errno.h.
 */

#define NACL_ABI_EPERM     1  /* Operation not permitted */
#define NACL_ABI_ENOENT    2  /* No such file or directory */
#define NACL_ABI_ESRCH     3  /* No such process */
#define NACL_ABI_EINTR     4  /* Interrupted system call */
#define NACL_ABI_EIO       5  /* I/O error */
#define NACL_ABI_ENXIO     6  /* No such device or address */
#define NACL_ABI_E2BIG     7  /* Argument list too long */
#define NACL_ABI_ENOEXEC   8  /* Exec format error */
#define NACL_ABI_EBADF     9  /* Bad file number */
#define NACL_ABI_ECHILD   10  /* No child processes */
#define NACL_ABI_EAGAIN   11  /* Try again */
#define NACL_ABI_ENOMEM   12  /* Out of memory */
#define NACL_ABI_EACCES   13  /* Permission denied */
#define NACL_ABI_EFAULT   14  /* Bad address */

#define NACL_ABI_EBUSY    16  /* Device or resource busy */
#define NACL_ABI_EEXIST   17  /* File exists */
#define NACL_ABI_EXDEV    18  /* Cross-device link */
#define NACL_ABI_ENODEV   19  /* No such device */
#define NACL_ABI_ENOTDIR  20  /* Not a directory */
#define NACL_ABI_EISDIR   21  /* Is a directory */
#define NACL_ABI_EINVAL   22  /* Invalid argument */
#define NACL_ABI_ENFILE   23  /* File table overflow */
#define NACL_ABI_EMFILE   24  /* Too many open files */
#define NACL_ABI_ENOTTY   25  /* Not a typewriter */

#define NACL_ABI_EFBIG    27  /* File too large */
#define NACL_ABI_ENOSPC   28  /* No space left on device */
#define NACL_ABI_ESPIPE   29  /* Illegal seek */
#define NACL_ABI_EROFS    30  /* Read-only file system */
#define NACL_ABI_EMLINK   31  /* Too many links */
#define NACL_ABI_EPIPE    32  /* Broken pipe */

#define NACL_ABI_ENAMETOOLONG 36  /* File name too long */

#define NACL_ABI_ENOSYS   38  /* Function not implemented */

#define NACL_ABI_EDQUOT   122 /* Quota exceeded */

/*
 * Other definitions not needed for NaCl, but needed for newlib build.
 */
#define NACL_ABI_EDOM 33   /* Math arg out of domain of func */
#define NACL_ABI_ERANGE 34 /* Math result not representable */
#define NACL_ABI_EDEADLK 35  /* Deadlock condition */
#define NACL_ABI_ENOLCK 37 /* No record locks available */
#define NACL_ABI_ENOTEMPTY 39  /* Directory not empty */
#define NACL_ABI_ELOOP 40  /* Too many symbolic links */
#define NACL_ABI_ENOMSG 42 /* No message of desired type */
#define NACL_ABI_EIDRM 43  /* Identifier removed */
#define NACL_ABI_ECHRNG 44 /* Channel number out of range */
#define NACL_ABI_EL2NSYNC 45 /* Level 2 not synchronized */
#define NACL_ABI_EL3HLT 46 /* Level 3 halted */
#define NACL_ABI_EL3RST 47 /* Level 3 reset */
#define NACL_ABI_ELNRNG 48 /* Link number out of range */
#define NACL_ABI_EUNATCH 49  /* Protocol driver not attached */
#define NACL_ABI_ENOCSI 50 /* No CSI structure available */
#define NACL_ABI_EL2HLT 51 /* Level 2 halted */
#define NACL_ABI_EBADE 52  /* Invalid exchange */
#define NACL_ABI_EBADR 53  /* Invalid request descriptor */
#define NACL_ABI_EXFULL 54 /* Exchange full */
#define NACL_ABI_ENOANO 55 /* No anode */
#define NACL_ABI_EBADRQC 56  /* Invalid request code */
#define NACL_ABI_EBADSLT 57  /* Invalid slot */
#define NACL_ABI_EDEADLOCK NACL_ABI_EDEADLK  /* File locking deadlock error */
#define NACL_ABI_EBFONT 59 /* Bad font file fmt */
#define NACL_ABI_ENOSTR 60 /* Device not a stream */
#define NACL_ABI_ENODATA 61  /* No data (for no delay io) */
#define NACL_ABI_ETIME 62  /* Timer expired */
#define NACL_ABI_ENOSR 63  /* Out of streams resources */
#define NACL_ABI_ENONET 64 /* Machine is not on the network */
#define NACL_ABI_ENOPKG 65 /* Package not installed */
#define NACL_ABI_EREMOTE 66  /* The object is remote */
#define NACL_ABI_ENOLINK 67  /* The link has been severed */
#define NACL_ABI_EADV 68   /* Advertise error */
#define NACL_ABI_ESRMNT 69 /* Srmount error */
#define NACL_ABI_ECOMM 70  /* Communication error on send */
#define NACL_ABI_EPROTO 71 /* Protocol error */
#define NACL_ABI_EMULTIHOP 72  /* Multihop attempted */
#define NACL_ABI_EDOTDOT 73  /* Cross mount point (not really error) */
#define NACL_ABI_EBADMSG 74  /* Trying to read unreadable message */
#define NACL_ABI_EOVERFLOW 75 /* Value too large for defined data type */
#define NACL_ABI_ENOTUNIQ 76 /* Given log. name not unique */
#define NACL_ABI_EBADFD 77 /* f.d. invalid for this operation */
#define NACL_ABI_EREMCHG 78  /* Remote address changed */
#define NACL_ABI_ELIBACC 79  /* Can't access a needed shared lib */
#define NACL_ABI_ELIBBAD 80  /* Accessing a corrupted shared lib */
#define NACL_ABI_ELIBSCN 81  /* .lib section in a.out corrupted */
#define NACL_ABI_ELIBMAX 82  /* Attempting to link in too many libs */
#define NACL_ABI_ELIBEXEC 83 /* Attempting to exec a shared library */
#define NACL_ABI_EILSEQ 84
#define NACL_ABI_EUSERS 87
#define NACL_ABI_ENOTSOCK 88  /* Socket operation on non-socket */
#define NACL_ABI_EDESTADDRREQ 89  /* Destination address required */
#define NACL_ABI_EMSGSIZE 90    /* Message too long */
#define NACL_ABI_EPROTOTYPE 91  /* Protocol wrong type for socket */
#define NACL_ABI_ENOPROTOOPT 92 /* Protocol not available */
#define NACL_ABI_EPROTONOSUPPORT 93 /* Unknown protocol */
#define NACL_ABI_ESOCKTNOSUPPORT 94 /* Socket type not supported */
#define NACL_ABI_EOPNOTSUPP 95 /* Operation not supported on transport endpoint */
#define NACL_ABI_EPFNOSUPPORT 96 /* Protocol family not supported */
#define NACL_ABI_EAFNOSUPPORT 97 /* Address family not supported by protocol family */
#define NACL_ABI_EADDRINUSE 98    /* Address already in use */
#define NACL_ABI_EADDRNOTAVAIL 99 /* Address not available */
#define NACL_ABI_ENETDOWN 100    /* Network interface is not configured */
#define NACL_ABI_ENETUNREACH 101   /* Network is unreachable */
#define NACL_ABI_ENETRESET 102
#define NACL_ABI_ECONNABORTED 103  /* Connection aborted */
#define NACL_ABI_ECONNRESET 104  /* Connection reset by peer */
#define NACL_ABI_ENOBUFS 105 /* No buffer space available */
#define NACL_ABI_EISCONN 106   /* Socket is already connected */
#define NACL_ABI_ENOTCONN 107    /* Socket is not connected */
#define NACL_ABI_ESHUTDOWN 108 /* Can't send after socket shutdown */
#define NACL_ABI_ETOOMANYREFS 109
#define NACL_ABI_ETIMEDOUT 110   /* Connection timed out */
#define NACL_ABI_ECONNREFUSED 111  /* Connection refused */
#define NACL_ABI_EHOSTDOWN 112   /* Host is down */
#define NACL_ABI_EHOSTUNREACH 113  /* Host is unreachable */
#define NACL_ABI_EALREADY 114    /* Socket already connected */
#define NACL_ABI_EINPROGRESS 115   /* Connection already in progress */
#define NACL_ABI_ESTALE 116
#define NACL_ABI_ENOTSUP NACL_ABI_EOPNOTSUPP   /* Not supported */
#define NACL_ABI_ENOMEDIUM 123   /* No medium (in tape drive) */
#define NACL_ABI_ECANCELED 125 /* Operation canceled. */
#define NACL_ABI_EOWNERDEAD 130 /* Owner died */
#define NACL_ABI_ENOTRECOVERABLE 131 /* State not recoverable */


/*
 * Below are non-standard errno values which are not defined on Linux.
 * Any new non-standard or NaCl-specific errno values should go here
 * and have high (>=2048) values, so as not to conflict with errno
 * values that Linux might add in the future.
 */
#define NACL_ABI_ELBIN       2048  /* Inode is remote (not really error) */
#define NACL_ABI_EFTYPE      2049  /* Inappropriate file type or format */
#define NACL_ABI_ENMFILE     2050  /* No more files */
#define NACL_ABI_EPROCLIM    2051
#define NACL_ABI_ENOSHARE    2052  /* No such host or network path */
#define NACL_ABI_ECASECLASH  2053  /* Filename exists with different case */


/* From cygwin32.  */
#define NACL_ABI_EWOULDBLOCK NACL_ABI_EAGAIN      /* Operation would block */

#endif
