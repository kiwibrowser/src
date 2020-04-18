/*
 * Copyright (C) 2013 Google, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_FETCH_INITIATOR_INFO_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_FETCH_INITIATOR_INFO_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/text_position.h"

namespace blink {

struct FetchInitiatorInfo {
  DISALLOW_NEW();
  FetchInitiatorInfo()
      : name(),
        position(TextPosition::BelowRangePosition()),
        start_time(0.0),
        is_link_preload(false) {}

  // ATTENTION: When adding members, update CrossThreadFetchInitiatorInfoData,
  // too.
  AtomicString name;
  TextPosition position;
  double start_time;
  bool is_link_preload;
  String imported_module_referrer;
};

// Encode AtomicString as String to cross threads.
struct CrossThreadFetchInitiatorInfoData {
  DISALLOW_NEW();
  explicit CrossThreadFetchInitiatorInfoData(const FetchInitiatorInfo& info)
      : name(info.name.GetString().IsolatedCopy()),
        position(info.position),
        start_time(info.start_time),
        is_link_preload(info.is_link_preload),
        imported_module_referrer(info.imported_module_referrer.IsolatedCopy()) {
  }

  operator FetchInitiatorInfo() const {
    FetchInitiatorInfo info;
    info.name = AtomicString(name);
    info.position = position;
    info.start_time = start_time;
    info.is_link_preload = is_link_preload;
    info.imported_module_referrer = imported_module_referrer;
    return info;
  }

  String name;
  TextPosition position;
  double start_time;
  bool is_link_preload;
  String imported_module_referrer;
};

}  // namespace blink

#endif
