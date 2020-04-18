// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_ASH_TEST_ENVIRONMENT_CHROME_H_
#define CHROME_TEST_BASE_ASH_TEST_ENVIRONMENT_CHROME_H_

#include "ash/test/ash_test_environment.h"
#include "base/macros.h"

class AshTestEnvironmentChrome : public ash::AshTestEnvironment {
 public:
  AshTestEnvironmentChrome();
  ~AshTestEnvironmentChrome() override;

  // AshTestEnvironment:
  std::unique_ptr<ash::AshTestViewsDelegate> CreateViewsDelegate() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AshTestEnvironmentChrome);
};

#endif  // CHROME_TEST_BASE_ASH_TEST_ENVIRONMENT_CHROME_H_
