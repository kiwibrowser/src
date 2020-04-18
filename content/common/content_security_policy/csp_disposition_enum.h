// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_CONTENT_SECURITY_POLICY_CSP_DISPOSITION_ENUM_
#define CONTENT_COMMON_CONTENT_SECURITY_POLICY_CSP_DISPOSITION_ENUM_

namespace content {

// Whether or not the Content Security Policy should be enforced.
enum class CSPDisposition {
  DO_NOT_CHECK,
  CHECK,

  LAST = CHECK
};

}  // namespace content
#endif  // CONTENT_COMMON_CONTENT_SECURITY_POLICY_CSP_DISPOSITION_ENUM_
