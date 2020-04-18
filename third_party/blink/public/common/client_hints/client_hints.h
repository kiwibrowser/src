// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_

#include <stddef.h>

#include "third_party/blink/common/common_export.h"

namespace blink {

// Mapping from WebClientHintsType to the header value for enabling the
// corresponding client hint. The ordering should match the ordering of enums in
// third_party/blink/public/platform/web_client_hints_type.h.
BLINK_COMMON_EXPORT extern const char* const kClientHintsHeaderMapping[];

BLINK_COMMON_EXPORT extern const size_t kClientHintsHeaderMappingCount;

// Mapping from WebEffectiveConnectionType to the header value. This value is
// sent to the origins and is returned by the JavaScript API. The ordering
// should match the ordering in //net/nqe/effective_connection_type.h and
// public/platform/WebEffectiveConnectionType.h.
// This array should be updated if either of the enums in
// effective_connection_type.h or WebEffectiveConnectionType.h are updated.
BLINK_COMMON_EXPORT extern const char* const
    kWebEffectiveConnectionTypeMapping[];

BLINK_COMMON_EXPORT extern const size_t kWebEffectiveConnectionTypeMappingCount;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_
