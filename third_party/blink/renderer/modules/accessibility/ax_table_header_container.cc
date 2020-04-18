/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/accessibility/ax_table_header_container.h"

#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"

namespace blink {

AXTableHeaderContainer::AXTableHeaderContainer(
    AXObjectCacheImpl& ax_object_cache)
    : AXMockObject(ax_object_cache) {}

AXTableHeaderContainer::~AXTableHeaderContainer() = default;

AXTableHeaderContainer* AXTableHeaderContainer::Create(
    AXObjectCacheImpl& ax_object_cache) {
  return new AXTableHeaderContainer(ax_object_cache);
}

bool AXTableHeaderContainer::ComputeAccessibilityIsIgnored(
    IgnoredReasons* ignored_reasons) const {
  if (!parent_)
    return true;

  if (!parent_->AccessibilityIsIgnored())
    return false;

  if (ignored_reasons)
    parent_->ComputeAccessibilityIsIgnored(ignored_reasons);

  return true;
}

void AXTableHeaderContainer::AddChildren() {
  DCHECK(!IsDetached());
  DCHECK(!have_children_);

  have_children_ = true;
  if (!parent_ || !parent_->IsAXTable())
    return;

  ToAXTable(parent_)->ColumnHeaders(children_);
}

}  // namespace blink
