// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/resolver/selector_filter_parent_scope.h"

namespace blink {

SelectorFilterParentScope* SelectorFilterParentScope::current_scope_ = nullptr;

}  // namespace blink
