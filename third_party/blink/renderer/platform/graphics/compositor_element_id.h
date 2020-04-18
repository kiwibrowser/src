// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_ELEMENT_ID_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_ELEMENT_ID_H_

#include "cc/trees/element_id.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/hash_functions.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/hash_traits.h"

namespace blink {

const int kCompositorNamespaceBitCount = 3;

enum class CompositorElementIdNamespace {
  kPrimary,
  kUniqueObjectId,
  kScroll,
  // The following are SPv2-only.
  kEffectFilter,
  kEffectMask,
  kEffectClipPath,
  // A sentinel to indicate the maximum representable namespace id
  // (the maximum is one less than this value).
  kMaxRepresentableNamespaceId = 1 << kCompositorNamespaceBitCount
};

using CompositorElementId = cc::ElementId;
using ScrollbarId = uint64_t;
using UniqueObjectId = uint64_t;
using DOMNodeId = uint64_t;
using SyntheticEffectId = uint64_t;

// Call this to get a globally unique object id for a newly allocated object.
UniqueObjectId PLATFORM_EXPORT NewUniqueObjectId();

// Call this with an appropriate namespace if more than one CompositorElementId
// is required for the given UniqueObjectId.
CompositorElementId PLATFORM_EXPORT
    CompositorElementIdFromUniqueObjectId(UniqueObjectId,
                                          CompositorElementIdNamespace);
// ...and otherwise call this method if there is only one CompositorElementId
// required for the given UniqueObjectId.
CompositorElementId PLATFORM_EXPORT
    CompositorElementIdFromUniqueObjectId(UniqueObjectId);

// TODO(chrishtr): refactor ScrollState to remove this dependency.
CompositorElementId PLATFORM_EXPORT CompositorElementIdFromDOMNodeId(DOMNodeId);

// Note cc::ElementId has a hash function already implemented via
// ElementIdHash::operator(). However for consistency's sake we choose to use
// Blink's hash functions with Blink specific data structures.
struct CompositorElementIdHash {
  static unsigned GetHash(const CompositorElementId& key) {
    return WTF::HashInt(static_cast<cc::ElementIdType>(key.ToInternalValue()));
  }
  static bool Equal(const CompositorElementId& a,
                    const CompositorElementId& b) {
    return a == b;
  }
  static const bool safe_to_compare_to_empty_or_deleted = true;
};

CompositorElementIdNamespace PLATFORM_EXPORT
    NamespaceFromCompositorElementId(CompositorElementId);

struct CompositorElementIdHashTraits
    : WTF::GenericHashTraits<CompositorElementId> {
  static CompositorElementId EmptyValue() { return CompositorElementId(1); }
  static void ConstructDeletedValue(CompositorElementId& slot, bool) {
    slot = CompositorElementId();
  }
  static bool IsDeletedValue(CompositorElementId value) {
    return value == CompositorElementId();
  }
};

using CompositorElementIdSet = HashSet<CompositorElementId,
                                       CompositorElementIdHash,
                                       CompositorElementIdHashTraits>;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITOR_ELEMENT_ID_H_
