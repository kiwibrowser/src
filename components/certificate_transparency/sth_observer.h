// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_OBSERVER_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_OBSERVER_H_

#include <set>

namespace net {
namespace ct {
struct SignedTreeHead;
}  // namespace ct
}  // namespace net

namespace certificate_transparency {

// Interface for receiving notifications of new STHs observed.
class STHObserver {
 public:
  virtual ~STHObserver() {}

  // Called with a new |sth| when one is observed.
  virtual void NewSTHObserved(const net::ct::SignedTreeHead& sth) = 0;
};

}  // namespace certificate_transparency

#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_OBSERVER_H_
