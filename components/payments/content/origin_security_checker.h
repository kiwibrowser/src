// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_ORIGIN_SECURITY_CHECKER_H_
#define COMPONENTS_PAYMENTS_CONTENT_ORIGIN_SECURITY_CHECKER_H_

#include "base/macros.h"

class GURL;

namespace payments {

class OriginSecurityChecker {
 public:
  // Returns true for a valid |url| from a secure origin.
  static bool IsOriginSecure(const GURL& url);

  // Returns true for a valid |url| with a cryptographic scheme, e.g., HTTPS,
  // WSS.
  static bool IsSchemeCryptographic(const GURL& url);

  // Returns true for a valid |url| with localhost or file:// scheme origin.
  static bool IsOriginLocalhostOrFile(const GURL& url);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OriginSecurityChecker);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_ORIGIN_SECURITY_CHECKER_H_
