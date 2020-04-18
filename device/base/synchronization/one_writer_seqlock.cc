// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/base/synchronization/one_writer_seqlock.h"

namespace device {

OneWriterSeqLock::OneWriterSeqLock() : sequence_(0) {}

base::subtle::Atomic32 OneWriterSeqLock::ReadBegin() const {
  base::subtle::Atomic32 version;
  for (;;) {
    version = base::subtle::NoBarrier_Load(&sequence_);

    // If the counter is even, then the associated data might be in a
    // consistent state, so we can try to read.
    if ((version & 1) == 0)
      break;

    // Otherwise, the writer is in the middle of an update. Retry the read.
    base::PlatformThread::YieldCurrentThread();
  }
  return version;
}

bool OneWriterSeqLock::ReadRetry(base::subtle::Atomic32 version) const {
  // If the sequence number was updated then a read should be re-attempted.
  // -- Load fence, read membarrier
  return base::subtle::Release_Load(&sequence_) != version;
}

void OneWriterSeqLock::WriteBegin() {
  // Increment the sequence number to odd to indicate the beginning of a write
  // update.
  base::subtle::Barrier_AtomicIncrement(&sequence_, 1);
  // -- Store fence, write membarrier
}

void OneWriterSeqLock::WriteEnd() {
  // Increment the sequence to an even number to indicate the completion of
  // a write update.
  // -- Store fence, write membarrier
  base::subtle::Barrier_AtomicIncrement(&sequence_, 1);
}

}  // namespace device
