// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_CACHEINVALIDATION_DEPS_SCOPED_PTR_H_
#define GOOGLE_CACHEINVALIDATION_DEPS_SCOPED_PTR_H_

#include <memory>

namespace invalidation {

template <typename T, typename D = std::default_delete<T>>
using scoped_ptr = std::unique_ptr<T, D>;

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_DEPS_SCOPED_PTR_H_
