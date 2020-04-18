// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_CLIENT_FILTERABLE_STATE_H_
#define COMPONENTS_VARIATIONS_CLIENT_FILTERABLE_STATE_H_

#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "base/version.h"
#include "components/variations/proto/study.pb.h"

namespace variations {

// A container for all of the client state which is used for filtering studies.
struct ClientFilterableState {
  static Study::Platform GetCurrentPlatform();

  ClientFilterableState();
  ~ClientFilterableState();

  // The system locale.
  std::string locale;

  // The date on which the variations seed was fetched.
  base::Time reference_date;

  // The Chrome version to filter on.
  base::Version version;

  // The Channel for this Chrome installation.
  Study::Channel channel;

  // The hardware form factor that Chrome is running on.
  Study::FormFactor form_factor;

  // The OS on which Chrome is running.
  Study::Platform platform;

  // The named hardware configuration that Chrome is running on -- used to
  // identify models of devices.
  std::string hardware_class;

  // Whether this is a low-end device. Currently only supported on Android.
  // Based on base::SysInfo::IsLowEndDevice().
  bool is_low_end_device = false;

  // The country code to use for studies configured with session consistency.
  std::string session_consistency_country;

  // The country code to use for studies configured with permanent consistency.
  std::string permanent_consistency_country;

 private:
  DISALLOW_COPY_AND_ASSIGN(ClientFilterableState);
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_CLIENT_FILTERABLE_STATE_H_
