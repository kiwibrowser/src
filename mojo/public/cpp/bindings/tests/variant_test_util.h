// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_TESTS_VARIANT_TEST_UTIL_H_
#define MOJO_PUBLIC_CPP_BINDINGS_TESTS_VARIANT_TEST_UTIL_H_

#include <string.h>

#include "base/logging.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace mojo {
namespace test {

// Converts a request of Interface1 to a request of Interface0. Interface0 and
// Interface1 are expected to be two variants of the same mojom interface.
// In real-world use cases, users shouldn't need to worry about this. Because it
// is rare to deal with two variants of the same interface in the same app.
template <typename Interface0, typename Interface1>
InterfaceRequest<Interface0> ConvertInterfaceRequest(
    InterfaceRequest<Interface1> request) {
  DCHECK_EQ(0, strcmp(Interface0::Name_, Interface1::Name_));
  return InterfaceRequest<Interface0>(request.PassMessagePipe());
}

}  // namespace test
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_TESTS_VARIANT_TEST_UTIL_H_
