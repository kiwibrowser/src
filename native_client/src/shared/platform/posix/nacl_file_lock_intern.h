/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_FILE_LOCK_INTERN_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_FILE_LOCK_INTERN_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/shared/platform/posix/nacl_file_lock.h"

EXTERN_C_BEGIN

/*
 * Invariant: entry will not be deleted as long as
 *
 *   holding_lock || (num_waiting > 0)
 *
 * holds.
 */
struct NaClFileLockEntry {
  /*
   * File identity.  Invariant after creation/initialization of lock
   * entry.  We do not save the descriptor, since it cannot be reused
   * anyway: thread A takes lock using one descriptor; thread B calls
   * lock with a different one, blocks; thread A unlocks, immediately
   * closes the descriptor that it used; thread B wakes up -- at this
   * point, it must use its own descriptor, not the one used by A.
   */
  dev_t file_dev;
  ino_t file_ino;

  /*
   * Container-level lock protects the next link.
   */
  struct NaClFileLockEntry *next;

  /*
   * Lock protects mutable fields below.
   */
  struct NaClMutex mu;
  struct NaClCondVar cv;

  /*
   * Is this process holding the lock?  This really means that a
   * thread is attempting to acquire the lock, and that other threads
   * should wait for this thread to finish.  The thread which set
   * holding_lock will not actually have acquired the flock lock when
   * it drops the mutex, but it will attempt to acquire the lock --
   * and may block -- immediately after dropping the mutex.
   *
   * Threads that see holding_lock true should increment num_waiting
   * and then wait on the condvar.  On wakeup, if !holding_lock holds,
   * decrement num_waiting, set holding_lock, drop the mutex, and
   * actually attempt to grab the system lock before returning.  If
   * holding_lock holds (spurious wakeup), go back to wait on the
   * condvar.
   */
  int holding_lock;

  /*
   * Delete entry when num_waiting is 0 at unlock, otherwise wake up
   * after dropping flock lock and clearing holding_lock to let other
   * processes and threads in this process compete for the lock.
   */
  size_t num_waiting;

  /*
   * NB: this structure is currently designed to use flock locks.  If
   * fcntl locks are desired, then we will have to have a queue of
   * delayed file closure functions where we ensure that *all* *other*
   * calls to close(2) will instead be routed through the lock
   * manager, so that we do not close a descriptor which refers to the
   * same file as another descriptor with which the fcntl lock is
   * taken/active, since closing any open file object referring to the
   * file will cause all locks to be dropped.  This is extremely
   * invasive and probably will require the moral equvalent of --wrap
   * gnu ld option in the OSX toolchain.  This can be done at runtime,
   * see:
   * https://developer.apple.com/library/mac/#documentation/developertools/conceptual/dynamiclibraries/100-Articles/UsingDynamicLibraries.html
   */
};

EXTERN_C_END

#endif
