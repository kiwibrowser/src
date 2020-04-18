/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/file.h>

#include "native_client/src/shared/platform/posix/nacl_file_lock_intern.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"


static struct NaClFileLockEntry **NaClFileLockManagerFindEntryMu(
    struct NaClFileLockManager *self,
    struct NaClFileLockEntry const *key) {
  struct NaClFileLockEntry **pptr;
  struct NaClFileLockEntry *ptr;

  for (pptr = &self->head; NULL != (ptr = *pptr); pptr = &ptr->next) {
    if (ptr->file_dev == key->file_dev &&
        ptr->file_ino == key->file_ino) {
      return pptr;
    }
  }
  return NULL;
}

static struct NaClFileLockEntry *NaClFileLockManagerEntryFactory(
    struct NaClFileLockManager *self,
    int desc) {
  struct NaClFileLockEntry *result;

  result = malloc(sizeof *result);
  CHECK(NULL != result);
  (*self->set_file_identity_data)(result, desc);
  result->next = NULL;
  NaClXMutexCtor(&result->mu);
  NaClXCondVarCtor(&result->cv);
  result->holding_lock = 1;  /* caller is creating to hold the lock */
  result->num_waiting = 0;
  return result;
}

static void NaClFileLockManagerFileEntryRecycler(
    struct NaClFileLockEntry **entryp) {
  struct NaClFileLockEntry *entry;
  CHECK(NULL != entryp);
  entry = *entryp;
  CHECK(0 == entry->holding_lock);
  CHECK(0 == entry->num_waiting);
  entry->file_dev = 0;
  entry->file_ino = 0;
  entry->next = NULL;
  NaClMutexDtor(&entry->mu);
  NaClCondVarDtor(&entry->cv);
  entry->holding_lock = 0;
  entry->num_waiting = 0;
  free(entry);
  *entryp = NULL;
}

static void NaClFileLockManagerSetFileIdentityData(
    struct NaClFileLockEntry *entry,
    int desc) {
  nacl_host_stat_t stbuf;
  if (0 != NACL_HOST_FSTAT64(desc, &stbuf)) {
    NaClLog(LOG_FATAL,
            "NaClFileLockManagerSetFileIdentityData: fstat failed, desc %d,"
            " errno %d\n", desc, errno);
  }
  entry->file_dev = stbuf.st_dev;
  entry->file_ino = stbuf.st_ino;
}

static void NaClFileLockManagerTakeLock(int desc) {
  if (0 != flock(desc, LOCK_EX)) {
    NaClLog(LOG_FATAL,
            "NaClFileLockManagerTakeLock: flock failed: errno %d\n", errno);
  }
}

static void NaClFileLockManagerDropLock(int desc) {
  if (0 != flock(desc, LOCK_UN)) {
    NaClLog(LOG_FATAL,
            "NaClFileLockManagerDropLock: flock failed: errno %d\n", errno);
  }
}

void NaClFileLockManagerCtor(struct NaClFileLockManager *self) {
  NaClXMutexCtor(&self->mu);
  self->head = NULL;
  self->set_file_identity_data = NaClFileLockManagerSetFileIdentityData;
  self->take_file_lock = NaClFileLockManagerTakeLock;
  self->drop_file_lock = NaClFileLockManagerDropLock;
}

void NaClFileLockManagerDtor(struct NaClFileLockManager *self) {
  CHECK(NULL == self->head);
  NaClMutexDtor(&self->mu);
}

void NaClFileLockManagerLock(struct NaClFileLockManager *self,
                             int desc) {
  struct NaClFileLockEntry key;
  struct NaClFileLockEntry **existing;
  struct NaClFileLockEntry *entry;

  (*self->set_file_identity_data)(&key, desc);

  NaClXMutexLock(&self->mu);
  existing = NaClFileLockManagerFindEntryMu(self, &key);
  if (NULL == existing) {
    /* make new entry */
    entry = NaClFileLockManagerEntryFactory(self, desc);
    entry->next = self->head;
    self->head = entry;
    NaClXMutexUnlock(&self->mu);
  } else {
    entry = *existing;
    NaClXMutexLock(&entry->mu);
    entry->num_waiting++;
    /* arithmetic overflow */
    CHECK(0 != entry->num_waiting);
    /* drop container lock after ensuring that the entry will not be deleted */
    NaClXMutexUnlock(&self->mu);
    while (entry->holding_lock) {
      NaClXCondVarWait(&entry->cv, &entry->mu);
    }
    entry->holding_lock = 1;
    entry->num_waiting--;
    NaClXMutexUnlock(&entry->mu);
  }
  (*self->take_file_lock)(desc);
}

void NaClFileLockManagerUnlock(struct NaClFileLockManager *self,
                               int desc) {
  struct NaClFileLockEntry key;
  struct NaClFileLockEntry **existing;
  struct NaClFileLockEntry *entry;

  (*self->set_file_identity_data)(&key, desc);

  NaClXMutexLock(&self->mu);
  existing = NaClFileLockManagerFindEntryMu(self, &key);
  CHECK(NULL != existing);
  entry = *existing;
  NaClXMutexLock(&entry->mu);
  entry->holding_lock = 0;
  if (0 == entry->num_waiting) {
    *existing = entry->next;
    NaClXMutexUnlock(&entry->mu);
    NaClXMutexUnlock(&self->mu);
    NaClFileLockManagerFileEntryRecycler(&entry);
  } else {
    NaClXMutexUnlock(&self->mu);
    /* tell waiting threads that they can now compete for the lock */
    NaClXCondVarBroadcast(&entry->cv);
    NaClXMutexUnlock(&entry->mu);
  }
  (*self->drop_file_lock)(desc);
}
