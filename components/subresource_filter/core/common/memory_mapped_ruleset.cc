// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/core/common/memory_mapped_ruleset.h"

#include <utility>

namespace subresource_filter {

MemoryMappedRuleset::MemoryMappedRuleset(base::File ruleset_file) {
  ruleset_.Initialize(std::move(ruleset_file));
}

MemoryMappedRuleset::~MemoryMappedRuleset() {}

}  // namespace subresource_filter
