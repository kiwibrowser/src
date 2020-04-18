// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_BLE_RAW_EID_GENERATOR_IMPL_H_
#define COMPONENTS_CRYPTAUTH_BLE_RAW_EID_GENERATOR_IMPL_H_

#include <string>

#include "base/macros.h"
#include "components/cryptauth/raw_eid_generator.h"

namespace cryptauth {

// Generates raw ephemeral ID (EID) values that are used by the
// ForegroundEidGenerator and BackgroundEidGenerator classes.
class RawEidGeneratorImpl : public RawEidGenerator {
 public:
  RawEidGeneratorImpl();
  ~RawEidGeneratorImpl() override;

  // RawEidGenerator:
  std::string GenerateEid(const std::string& eid_seed,
                          int64_t start_of_period_timestamp_ms,
                          std::string const* extra_entropy) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RawEidGeneratorImpl);
};

}  // cryptauth

#endif  // COMPONENTS_CRYPTAUTH_BLE_RAW_EID_GENERATOR_IMPL_H_
