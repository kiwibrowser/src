// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_LEGACY_STATS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_LEGACY_STATS_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

class WebRTCLegacyStatsMemberIterator;

enum WebRTCLegacyStatsMemberType {
  kWebRTCLegacyStatsMemberTypeInt,
  kWebRTCLegacyStatsMemberTypeInt64,
  kWebRTCLegacyStatsMemberTypeFloat,
  kWebRTCLegacyStatsMemberTypeString,
  kWebRTCLegacyStatsMemberTypeBool,
  kWebRTCLegacyStatsMemberTypeId,
};

class WebRTCLegacyStats {
 public:
  virtual ~WebRTCLegacyStats() = default;

  virtual WebString Id() const = 0;
  virtual WebString GetType() const = 0;
  virtual double Timestamp() const = 0;

  // The caller owns the iterator. The iterator must not be used after
  // the |WebRTCLegacyStats| that created it is destroyed.
  virtual WebRTCLegacyStatsMemberIterator* Iterator() const = 0;
};

class WebRTCLegacyStatsMemberIterator {
 public:
  virtual ~WebRTCLegacyStatsMemberIterator() = default;
  virtual bool IsEnd() const = 0;
  virtual void Next() = 0;

  virtual WebString GetName() const = 0;
  virtual WebRTCLegacyStatsMemberType GetType() const = 0;
  // Value getters. No conversion is performed; the function must match the
  // member's |type|.
  virtual int ValueInt() const = 0;        // kWebRTCLegacyStatsMemberTypeInt
  virtual int64_t ValueInt64() const = 0;  // kWebRTCLegacyStatsMemberTypeInt64
  virtual float ValueFloat() const = 0;    // kWebRTCLegacyStatsMemberTypeFloat
  virtual WebString ValueString()
      const = 0;                       // kWebRTCLegacyStatsMemberTypeString
  virtual bool ValueBool() const = 0;  // kWebRTCLegacyStatsMemberTypeBool

  // Converts the value to string (regardless of |type|).
  virtual WebString ValueToString() const = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RTC_LEGACY_STATS_H_
