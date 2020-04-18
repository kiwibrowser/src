// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/threading/thread.h"
#include "mash/test/mash_test_suite.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "services/catalog/catalog.h"

namespace {

const base::FilePath::CharType kCatalogFilename[] =
    FILE_PATH_LITERAL("mash_unittests_catalog.json");

}  // namespace

int main(int argc, char** argv) {
  mash::test::MashTestSuite test_suite(argc, argv);

  catalog::Catalog::LoadDefaultCatalogManifest(
      base::FilePath(kCatalogFilename));

  mojo::edk::Init();
  base::Thread ipc_thread("IPC thread");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  return base::LaunchUnitTests(argc, argv,
                               base::Bind(&mash::test::MashTestSuite::Run,
                                          base::Unretained(&test_suite)));
}
