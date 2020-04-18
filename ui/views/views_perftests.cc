// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/launcher/unit_test_launcher.h"
#include "mojo/edk/embedder/embedder.h"
#include "ui/views/views_test_suite.h"

int main(int argc, char** argv) {
  views::ViewsTestSuite test_suite(argc, argv);
  mojo::edk::Init();
  return base::LaunchUnitTestsSerially(
      argc, argv,
      base::BindOnce(&views::ViewsTestSuite::Run,
                     base::Unretained(&test_suite)));
}
