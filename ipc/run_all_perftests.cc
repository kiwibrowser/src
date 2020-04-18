// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copied from mojo/edk/test/run_all_perftests.cc.

#include "base/command_line.h"
#include "base/test/perf_test_suite.h"
#include "base/test/test_io_thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/edk/test/test_support_impl.h"

int main(int argc, char** argv) {
  base::PerfTestSuite test(argc, argv);

  mojo::edk::Init();
  base::TestIOThread test_io_thread(base::TestIOThread::kAutoStart);
  mojo::edk::ScopedIPCSupport ipc_support(
      test_io_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);
  mojo::test::TestSupport::Init(new mojo::edk::test::TestSupportImpl());

  return test.Run();
}
