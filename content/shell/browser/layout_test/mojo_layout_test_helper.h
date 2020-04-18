// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_MOJO_LAYOUT_TEST_HELPER_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_MOJO_LAYOUT_TEST_HELPER_H_

#include "base/macros.h"
#include "content/test/data/mojo_layouttest_test.mojom.h"

namespace content {

class MojoLayoutTestHelper : public mojom::MojoLayoutTestHelper {
 public:
  MojoLayoutTestHelper();
  ~MojoLayoutTestHelper() override;

  static void Create(mojom::MojoLayoutTestHelperRequest request);

  // mojom::MojoLayoutTestHelper:
  void Reverse(const std::string& message, ReverseCallback callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MojoLayoutTestHelper);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_MOJO_LAYOUT_TEST_HELPER_H_
