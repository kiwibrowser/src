// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/base/synchronization/one_writer_seqlock.h"

#include <stdlib.h>

#include "base/atomic_ref_count.h"
#include "base/macros.h"
#include "base/third_party/dynamic_annotations/dynamic_annotations.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

// Basic test to make sure that basic operation works correctly.

struct TestData {
  unsigned a, b, c;
};

class BasicSeqLockTestThread : public base::PlatformThread::Delegate {
 public:
  BasicSeqLockTestThread() = default;

  void Init(OneWriterSeqLock* seqlock,
            TestData* data,
            base::AtomicRefCount* ready) {
    seqlock_ = seqlock;
    data_ = data;
    ready_ = ready;
  }
  void ThreadMain() override {
    while (ready_->IsZero()) {
      base::PlatformThread::YieldCurrentThread();
    }

    for (unsigned i = 0; i < 1000; ++i) {
      TestData copy;
      base::subtle::Atomic32 version;
      do {
        version = seqlock_->ReadBegin();
        copy = *data_;
      } while (seqlock_->ReadRetry(version));

      EXPECT_EQ(copy.a + 100, copy.b);
      EXPECT_EQ(copy.c, copy.b + copy.a);
    }

    ready_->Decrement();
  }

 private:
  OneWriterSeqLock* seqlock_;
  TestData* data_;
  base::AtomicRefCount* ready_;

  DISALLOW_COPY_AND_ASSIGN(BasicSeqLockTestThread);
};

#if defined(OS_ANDROID)
#define MAYBE_ManyThreads FLAKY_ManyThreads
#else
#define MAYBE_ManyThreads ManyThreads
#endif
TEST(OneWriterSeqLockTest, MAYBE_ManyThreads) {
  OneWriterSeqLock seqlock;
  TestData data = {0, 0, 0};
  base::AtomicRefCount ready(0);

  ANNOTATE_BENIGN_RACE_SIZED(&data, sizeof(data), "Racey reads are discarded");

  static const unsigned kNumReaderThreads = 10;
  BasicSeqLockTestThread threads[kNumReaderThreads];
  base::PlatformThreadHandle handles[kNumReaderThreads];

  for (unsigned i = 0; i < kNumReaderThreads; ++i)
    threads[i].Init(&seqlock, &data, &ready);
  for (unsigned i = 0; i < kNumReaderThreads; ++i)
    ASSERT_TRUE(base::PlatformThread::Create(0, &threads[i], &handles[i]));

  // The main thread is the writer, and the spawned are readers.
  unsigned counter = 0;
  for (;;) {
    seqlock.WriteBegin();
    data.a = counter++;
    data.b = data.a + 100;
    data.c = data.b + data.a;
    seqlock.WriteEnd();

    if (counter == 1)
      ready.Increment(kNumReaderThreads);

    if (ready.IsZero())
      break;
  }

  for (unsigned i = 0; i < kNumReaderThreads; ++i)
    base::PlatformThread::Join(handles[i]);
}

}  // namespace device
