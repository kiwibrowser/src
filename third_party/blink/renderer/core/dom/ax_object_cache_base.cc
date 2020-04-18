// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/ax_object_cache_base.h"

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/ax_object_cache.h"

namespace blink {

AXObjectCacheBase::~AXObjectCacheBase() = default;

AXObjectCacheBase::AXObjectCacheBase(Document& document)
    : AXObjectCache(document) {}

}  // namespace blink
