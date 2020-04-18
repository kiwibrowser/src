// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MASH_TEST_TEST_SUITE_H_
#define MASH_TEST_TEST_SUITE_H_

#include <memory>

#include "base/macros.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "base/test/test_suite.h"

namespace aura {
class Env;
}

namespace ui {
class FakeContextFactory;
}

namespace mash {
namespace test {

class MashTestSuite : public base::TestSuite {
 public:
  MashTestSuite(int argc, char** argv);
  ~MashTestSuite() override;

 protected:
  // base::TestSuite:
  void Initialize() override;
  void Shutdown() override;

 private:
  base::TestDiscardableMemoryAllocator discardable_memory_allocator_;
  std::unique_ptr<aura::Env> env_;
  std::unique_ptr<ui::FakeContextFactory> context_factory_;
  base::test::ScopedFeatureList feature_list_;

  DISALLOW_COPY_AND_ASSIGN(MashTestSuite);
};

}  // namespace test
}  // namespace mash

#endif  // MASH_TEST_TEST_SUITE_H_
