// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/ash_test_suite.h"
#include "base/bind.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "mojo/edk/embedder/embedder.h"

int main(int argc, char** argv) {
  ash::AshTestSuite test_suite(argc, argv);

  mojo::edk::Init();
  return base::LaunchUnitTests(
      argc, argv,
      base::Bind(&ash::AshTestSuite::Run, base::Unretained(&test_suite)));
}
