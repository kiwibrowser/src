/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_POSIX_NACL_FILE_LOCK_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_POSIX_NACL_FILE_LOCK_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/shared/platform/nacl_sync.h"

EXTERN_C_BEGIN

struct NaClFileLockEntry;

/*
 * NaClFileLockManager is a container object that is normally a
 * singleton; it keeps track of locks held by the current process on
 * files.  The locks are an abstraction (in this case, built on top of
 * flock), with the difference that the locks are truly binary: unlike
 * flock or fcntl locks, if two threads in the process try to take a
 * file lock simultaneously, at least one will wait -- there is no
 * "upgrade" or coalescing of file locks.
 *
 * We use a linked list of file lock entries.  Should this become
 * performance critical, we can make this into a hash table.
 *
 * All fields are private.  Users of the API only need to know its
 * size.
 */
struct NaClFileLockManager {
  /* private */
  struct NaClMutex mu;
  struct NaClFileLockEntry *head;

  /*
   * Dependency injection; used for testing.
   *
   * Tests will invoke NaClFileLockManagerCtor and replace the
   * function pointers.  We don't need/want an explicit vtbl --
   * essentially declare abstract methods in a base class and require
   * subclass instantiation of the interface methods -- since in
   * normal use the NaClFileLockManager is a singleton.
   */
  void (*set_file_identity_data)(struct NaClFileLockEntry *entry,
                                 int desc);
  void (*take_file_lock)(int desc);
  void (*drop_file_lock)(int desc);
};

void NaClFileLockManagerCtor(struct NaClFileLockManager *self);

void NaClFileLockManagerLock(struct NaClFileLockManager *self,
                             int desc);

void NaClFileLockManagerUnlock(struct NaClFileLockManager *self,
                               int desc);

void NaClFileLockManagerDtor(struct NaClFileLockManager *self);

EXTERN_C_END

#endif
