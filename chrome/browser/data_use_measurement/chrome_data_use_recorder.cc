// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_use_measurement/chrome_data_use_recorder.h"
#include "ui/base/page_transition_types.h"

namespace data_use_measurement {

ChromeDataUseRecorder::ChromeDataUseRecorder(DataUse::TrafficType traffic_type)
    : DataUseRecorder(traffic_type), main_frame_id_(-1, -1) {
  set_page_transition(ui::PAGE_TRANSITION_LAST_CORE + 1);
}

ChromeDataUseRecorder::~ChromeDataUseRecorder() {}

}  // namespace data_use_measurement
