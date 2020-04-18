// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/ash_test_environment.h"

#include <memory>

#include "ash/test/ash_test_views_delegate.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"

namespace ash {
namespace {

class AshTestEnvironmentDefault : public AshTestEnvironment {
 public:
  AshTestEnvironmentDefault()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  ~AshTestEnvironmentDefault() override {
    base::RunLoop().RunUntilIdle();
  }

  // AshTestEnvironment:
  std::unique_ptr<AshTestViewsDelegate> CreateViewsDelegate() override {
    return std::make_unique<AshTestViewsDelegate>();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(AshTestEnvironmentDefault);
};

}  // namespace

// static
std::unique_ptr<AshTestEnvironment> AshTestEnvironment::Create() {
  return std::make_unique<AshTestEnvironmentDefault>();
}

// static
std::string AshTestEnvironment::Get100PercentResourceFileName() {
  return "ash_test_resources_100_percent.pak";
}

}  // namespace ash
