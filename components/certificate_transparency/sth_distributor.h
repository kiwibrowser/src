// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_DISTRIBUTOR_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_DISTRIBUTOR_H_

#include <vector>

#include "base/observer_list.h"
#include "components/certificate_transparency/sth_observer.h"
#include "components/certificate_transparency/sth_reporter.h"
#include "net/base/net_export.h"

namespace net {
namespace ct {
struct SignedTreeHead;
}  // namespace ct
}  // namespace net

namespace certificate_transparency {

// A proxy for delegating new STH notifications to all registered
// observers.
// For each |observer| registered with RegisterObserver, the
// NewSTHObserved method will be called whenever the STHDistributor's
// NewSTHObserved method is invoked.
class STHDistributor : public STHObserver, public STHReporter {
 public:
  STHDistributor();
  ~STHDistributor() override;

  // STHObserver implementation.
  void NewSTHObserved(const net::ct::SignedTreeHead& sth) override;

  // STHReporter implementation
  // Registers |observer| for new STH notifications. On registration,
  // the |observer| will be notified of the latest STH for each log tha the
  // STHDistributor has observed.
  void RegisterObserver(STHObserver* observer) override;

  // Unregisters |observer|, which must have been previously
  // registered via RegisterObserver()
  void UnregisterObserver(STHObserver* observer) override;

 private:
  // STHs from logs, one for each log.
  std::vector<net::ct::SignedTreeHead> observed_sths_;

  // The observers for new STH notifications.
  base::ObserverList<STHObserver, true> observer_list_;
};

}  // namespace certificate_transparency

#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_STH_DISTRIBUTOR_H_
