// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_ASH_TEST_SUITE_H_
#define ASH_TEST_ASH_TEST_SUITE_H_

#include <memory>

#include "base/macros.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "base/test/test_suite.h"
#include "ui/aura/env.h"

namespace ui {
class ContextFactory;
}

namespace ash {

class AshTestSuite : public base::TestSuite {
 public:
  AshTestSuite(int argc, char** argv);
  ~AshTestSuite() override;

 protected:
  // base::TestSuite:
  void Initialize() override;
  void Shutdown() override;

 private:
  std::unique_ptr<aura::Env> env_;

  base::TestDiscardableMemoryAllocator discardable_memory_allocator_;

  // Only used when running in mus/mash, and is set as the context_factory
  // on aura::Env.
  std::unique_ptr<ui::ContextFactory> context_factory_;

  DISALLOW_COPY_AND_ASSIGN(AshTestSuite);
};

}  // namespace ash

#endif  // ASH_TEST_ASH_TEST_SUITE_H_
