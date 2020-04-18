/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */
#include <errno.h>

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

/*
 * If you are using this as a kernel-style return, remember that you
 * should negate its return value.
 */
int NaClXlateErrno(int errnum) {
  switch (errnum) {
    /*
     * Unfortunately a macro cannot expand to contain #ifdef....
     *
     * TODO(bsy): All host-OS conditional errnos should map into a
     * generic errno.
     */
    case 0: return 0;
#define MAP(E) case E: do { return NACL_ABI_ ## E; } while (0)
    MAP(EPERM);
    MAP(ENOENT);
    MAP(ESRCH);
    MAP(EINTR);
    MAP(EIO);
    MAP(ENXIO);
    MAP(E2BIG);
    MAP(ENOEXEC);
    MAP(EBADF);
    MAP(ECHILD);
    MAP(EAGAIN);
    MAP(ENOMEM);
    MAP(EACCES);
    MAP(EFAULT);
    MAP(EBUSY);
    MAP(EEXIST);
    MAP(EXDEV);
    MAP(ENODEV);
    MAP(ENOTDIR);
    MAP(EISDIR);
    MAP(EINVAL);
    MAP(ENFILE);
    MAP(EMFILE);
    MAP(ENOTTY);
    MAP(EFBIG);
    MAP(ENOSPC);
    MAP(ESPIPE);
    MAP(EROFS);
    MAP(EMLINK);
    MAP(EPIPE);
    MAP(ENAMETOOLONG);
    MAP(ENOSYS);
#ifdef  EDQUOT
    MAP(EDQUOT);
#endif
    MAP(EDOM);
    MAP(ERANGE);
#ifdef  ENOMSG
    MAP(ENOMSG);
#endif
#ifdef  ECHRNG
    MAP(ECHRNG);
#endif
#ifdef  EL3HLT
    MAP(EL3HLT);  /* not in osx */
#endif
#ifdef  EL3RST
    MAP(EL3RST);  /* not in osx */
#endif
#ifdef  EL3RNG
    MAP(ELNRNG);  /* not in osx */
#endif
#ifdef  EUNATCH
    MAP(EUNATCH);
#endif
#ifdef  ENOCSI
    MAP(ENOCSI);
#endif
#ifdef  EL2HLT
    MAP(EL2HLT);
#endif
    MAP(EDEADLK);
    MAP(ENOLCK);
#ifdef  EBADE
    MAP(EBADE);
#endif
#ifdef  EBADR
    MAP(EBADR);
#endif
#ifdef  EXFULL
    MAP(EXFULL);
#endif
#ifdef  ENOANO
    MAP(ENOANO);
#endif
#ifdef  EBADRQC
    MAP(EBADRQC);
#endif
#ifdef  EBADSLT
    MAP(EBADSLT);
#endif
#if defined(EDEADLOCK) && EDEADLK != EDEADLOCK
    MAP(EDEADLOCK);
#endif
#ifdef  EBFONT
    MAP(EBFONT);
#endif
#ifdef  ENOSTR
    MAP(ENOSTR);
#endif
#ifdef  ENODATA
    MAP(ENODATA);
#endif
#ifdef  ETIME
    MAP(ETIME);
#endif
#ifdef  ENOSR
    MAP(ENOSR);
#endif
#ifdef  ENONET
    MAP(ENONET);
#endif
#ifdef  ENOPKG
    MAP(ENOPKG);
#endif
#ifdef  EREMOTE
    MAP(EREMOTE);
#endif
#ifdef  ENOLINK
    MAP(ENOLINK);
#endif
#ifdef  EADV
    MAP(EADV);
#endif
#ifdef  ESRMNT
    MAP(ESRMNT);
#endif
#ifdef  ECOMM
    MAP(ECOMM);
#endif
#ifdef  EPROTO
    MAP(EPROTO);
#endif
#ifdef  EMULTIHOP
    MAP(EMULTIHOP);
#endif
#ifdef  ELBIN
    MAP(ELBIN);  /* newlib only? */
#endif
#ifdef  EDOTDOT
    MAP(EDOTDOT);
#endif
#ifdef   EBADMSG
    MAP(EBADMSG);
#endif
#ifdef  EFTYPE
    MAP(EFTYPE);  /* osx has it; linux doesn't */
#endif
#ifdef  ENOTUNIQ
    MAP(ENOTUNIQ);
#endif
#ifdef  EBADFD
    MAP(EBADFD);
#endif
#ifdef  EREMCHG
    MAP(EREMCHG);
#endif
#ifdef  ELIBACC
    MAP(ELIBACC);
#endif
#ifdef  ELIBBAD
    MAP(ELIBBAD);
#endif
#ifdef  ELIBSCN
    MAP(ELIBSCN);
#endif
#ifdef  ELIBMAX
    MAP(ELIBMAX);
#endif
#ifdef  ELIBEXEC
    MAP(ELIBEXEC);
#endif
#ifdef  ENMFILE
    MAP(ENMFILE);  /* newlib only? */
#endif
    MAP(ENOTEMPTY);
#ifdef  ELOOP
    MAP(ELOOP);
#endif
#ifdef  EOPNOTSUPP
    MAP(EOPNOTSUPP);
#endif
#ifdef  EPFNOSUPPORT
    MAP(EPFNOSUPPORT);
#endif
#ifdef  ECONNRESET
    MAP(ECONNRESET);
#endif
#ifdef  ENOBUFS
    MAP(ENOBUFS);
#endif
#ifdef  EAFNOSUPPORT
    MAP(EAFNOSUPPORT);
#endif
#ifdef  EPROTOTYPE
    MAP(EPROTOTYPE);
#endif
#ifdef  ENOTSOCK
    MAP(ENOTSOCK);
#endif
#ifdef  ENOPROTOOPT
    MAP(ENOPROTOOPT);
#endif
#ifdef  ESHUTDOWN
    MAP(ESHUTDOWN);
#endif
#ifdef  ECONNREFUSED
    MAP(ECONNREFUSED);
#endif
#ifdef  EADDRINUSE
    MAP(EADDRINUSE);
#endif
#ifdef  ECONNABORTED
    MAP(ECONNABORTED);
#endif
#ifdef  ENETUNREACH
    MAP(ENETUNREACH);
#endif
#ifdef  ENETDOWN
    MAP(ENETDOWN);
#endif
#ifdef  ETIMEDOUT
    MAP(ETIMEDOUT);
#endif
#ifdef  EHOSTDOWN
    MAP(EHOSTDOWN);
#endif
#ifdef  EHOSTUNREACH
    MAP(EHOSTUNREACH);
#endif
#ifdef  EINPROGRESS
    MAP(EINPROGRESS);
#endif
#ifdef  EALREADY
    MAP(EALREADY);
#endif
#ifdef  EDESTADDRREQ
    MAP(EDESTADDRREQ);
#endif
#ifdef  EPROTONOSUPPORT
    MAP(EPROTONOSUPPORT);
#endif
#ifdef  ESOCKTNOSUPPORT
    MAP(ESOCKTNOSUPPORT);
#endif
#ifdef  EADDRNOTAVAIL
    MAP(EADDRNOTAVAIL);
#endif
#ifdef  ENETRESET
    MAP(ENETRESET);
#endif
#ifdef  EISCONN
    MAP(EISCONN);
#endif
#ifdef  ENOTCONN
    MAP(ENOTCONN);
#endif
#ifdef  ETOOMANYREFS
    MAP(ETOOMANYREFS);
#endif
#ifdef  EPROCLIM
    MAP(EPROCLIM);  /* osx has this; linux does not */
    /*
     * if we allow fork, we will need to map EAGAIN from fork to EPROCLIM,
     * so NaClXlateErrno would not be stateless.
     */
#endif
#ifdef  EUSERS
    MAP(EUSERS);
#endif
#ifdef  ESTALE
    MAP(ESTALE);
#endif
#if ENOTSUP != EOPNOTSUPP
    MAP(ENOTSUP);
#endif
#ifdef ENOMEDIUM
    MAP(ENOMEDIUM);
#endif
#ifdef ENOSHARE
    MAP(ENOSHARE);  /* newlib only? */
#endif
#ifdef ECASECLASH
    MAP(ECASECLASH);  /* newlib only? */
#endif
    MAP(EILSEQ);
#ifdef  EOVERFLOW
    MAP(EOVERFLOW);
#endif
#ifdef  ECANCELED
    MAP(ECANCELED);
#endif
#ifdef EL2NSYNC
    MAP(EL2NSYNC);
#endif
#ifdef  EIDRM
    MAP(EIDRM);
#endif
#ifdef  EMSGSIZE
    MAP(EMSGSIZE);
#endif
#ifdef  EOWNERDEAD
    MAP(EOWNERDEAD);
#endif
#ifdef  ENOTRECOVERABLE
    MAP(ENOTRECOVERABLE);
#endif
#undef MAP
  }
  return NACL_ABI_EINVAL;  /* catch all */
}

/*
 * If you are using this as a kernel-style return, remember that you
 * should negate its return value.
 */
int NaClXlateNaClSyncStatus(NaClSyncStatus status) {
  switch (status) {
#define MAP(S, E) case S: do { return E; } while (0)
    MAP(NACL_SYNC_OK, 0);
    case NACL_SYNC_INTERNAL_ERROR:
      NaClLog(LOG_FATAL,
              "NaClXlateNaClSyncStatus: NACL_SYNC_INTERNAL_ERROR\n");
    MAP(NACL_SYNC_BUSY, NACL_ABI_EBUSY);
    MAP(NACL_SYNC_MUTEX_INVALID, NACL_ABI_EINVAL);
    MAP(NACL_SYNC_MUTEX_DEADLOCK, NACL_ABI_EDEADLK);
    MAP(NACL_SYNC_MUTEX_PERMISSION, NACL_ABI_EPERM);
    MAP(NACL_SYNC_MUTEX_INTERRUPTED, NACL_ABI_EINTR);
    MAP(NACL_SYNC_CONDVAR_TIMEDOUT, NACL_ABI_ETIMEDOUT);
    MAP(NACL_SYNC_CONDVAR_INTR, NACL_ABI_EINTR);
    MAP(NACL_SYNC_SEM_INTERRUPTED, NACL_ABI_EINTR);
    MAP(NACL_SYNC_SEM_RANGE_ERROR, NACL_ABI_ERANGE);
#undef MAP
  }
  NaClLog(LOG_FATAL,
          "NaClXlateNaClSyncStatus: status %d\n", (int) status);
  return NACL_ABI_EINVAL;  /* catch all */
}


struct NaClHostDesc *NaClHostDescPosixMake(int  posix_d,
                                           int  flags) {
  struct NaClHostDesc *nhdp;
  int                 error;

  nhdp = malloc(sizeof *nhdp);
  if (NULL == nhdp) {
    NaClLog(LOG_FATAL, "NaClHostDescPosixMake(%d,0x%x): malloc failed\n",
            posix_d, flags);
  }
  if (0 != (error = NaClHostDescPosixTake(nhdp, posix_d, flags))) {
    NaClLog(LOG_FATAL,
            "NaClHostDescPosixMake(%d,0x%x): Take failed, error %da\n",
            posix_d, flags, error);
  }
  return nhdp;
}


int NaClProtMap(int abi_prot) {
  int host_os_prot;

  host_os_prot = 0;
#define M(H) do { \
    if (0 != (abi_prot & NACL_ABI_ ## H)) { \
      host_os_prot |= H; \
    } \
  } while (0)
  M(PROT_READ);
  M(PROT_WRITE);
  M(PROT_EXEC);
#if PROT_NONE != 0
# error "NaClProtMap:  PROT_NONE is not zero -- are mprotect flags bit values?"
#endif
  return host_os_prot;
#undef M
}

void NaClHostDescCheckValidity(char const *fn_name,
                               struct NaClHostDesc *d) {
  if (NULL == d) {
    NaClLog(LOG_FATAL, "%s: 'this' is NULL\n", fn_name);
  }
  if (-1 == d->d) {
    NaClLog(LOG_FATAL, "%s: already closed\n", fn_name);
  }
}
