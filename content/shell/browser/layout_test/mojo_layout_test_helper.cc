// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/mojo_layout_test_helper.h"

#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

MojoLayoutTestHelper::MojoLayoutTestHelper() {}

MojoLayoutTestHelper::~MojoLayoutTestHelper() {}

// static
void MojoLayoutTestHelper::Create(mojom::MojoLayoutTestHelperRequest request) {
  mojo::MakeStrongBinding(std::make_unique<MojoLayoutTestHelper>(),
                          std::move(request));
}

void MojoLayoutTestHelper::Reverse(const std::string& message,
                                   ReverseCallback callback) {
  std::move(callback).Run(std::string(message.rbegin(), message.rend()));
}

}  // namespace content
