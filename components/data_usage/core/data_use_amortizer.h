// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_USAGE_CORE_DATA_USE_AMORTIZER_H_
#define COMPONENTS_DATA_USAGE_CORE_DATA_USE_AMORTIZER_H_

#include <stdint.h>

#include <memory>

#include "base/callback.h"

namespace data_usage {

struct DataUse;

// Class that takes in DataUse and amortizes any extra data usage overhead
// across DataUse objects.
class DataUseAmortizer {
 public:
  typedef base::Callback<void(std::unique_ptr<DataUse>)>
      AmortizationCompleteCallback;

  virtual ~DataUseAmortizer() {}

  // Amortizes overhead into |data_use|, then passes the it to |callback| once
  // amortization is complete. Amortizers that perform buffering may combine
  // together |data_use| objects with the same |callback| if the |data_use|
  // objects are identical in all ways but their byte counts.
  virtual void AmortizeDataUse(
      std::unique_ptr<DataUse> data_use,
      const AmortizationCompleteCallback& callback) = 0;

  // Notifies the DataUseAmortizer that some extra bytes have been transferred
  // that aren't associated with any DataUse objects (e.g. off-the-record
  // traffic).
  virtual void OnExtraBytes(int64_t extra_tx_bytes, int64_t extra_rx_bytes) = 0;
};

}  // namespace data_usage

#endif  // COMPONENTS_DATA_USAGE_CORE_DATA_USE_AMORTIZER_H_
