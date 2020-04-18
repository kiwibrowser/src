// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_VIOLATION_REPORTING_POLICY_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_VIOLATION_REPORTING_POLICY_H_

namespace blink {

enum class SecurityViolationReportingPolicy {
  kSuppressReporting,
  kReport,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_SECURITY_VIOLATION_REPORTING_POLICY_H_
