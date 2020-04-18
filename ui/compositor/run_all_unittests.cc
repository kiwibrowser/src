// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "mojo/edk/embedder/embedder.h"
#include "ui/compositor/test/test_suite.h"

int main(int argc, char** argv) {
  ui::test::CompositorTestSuite test_suite(argc, argv);

  mojo::edk::Init();

  return base::LaunchUnitTests(
      argc, argv,
      base::BindOnce(&ui::test::CompositorTestSuite::Run,
                     base::Unretained(&test_suite)));
}
