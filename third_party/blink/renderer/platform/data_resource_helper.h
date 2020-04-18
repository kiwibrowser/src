// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_DATA_RESOURCE_HELPER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_DATA_RESOURCE_HELPER_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

PLATFORM_EXPORT String GetDataResourceAsASCIIString(const char* resource);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_DATA_RESOURCE_HELPER_H_
