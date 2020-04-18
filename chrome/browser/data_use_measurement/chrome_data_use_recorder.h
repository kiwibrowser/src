// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_RECORDER_H_
#define CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_RECORDER_H_

#include <utility>

#include "base/macros.h"
#include "components/data_use_measurement/core/data_use_recorder.h"
#include "content/public/browser/global_request_id.h"

namespace data_use_measurement {

typedef std::pair<int, int> RenderFrameHostID;

class ChromeDataUseRecorder : public DataUseRecorder {
 public:
  explicit ChromeDataUseRecorder(DataUse::TrafficType traffic_type);
  ~ChromeDataUseRecorder() override;

  RenderFrameHostID main_frame_id() const { return main_frame_id_; }

  void set_main_frame_id(RenderFrameHostID frame_id) {
    main_frame_id_ = frame_id;
  }

  content::GlobalRequestID main_frame_request_id() const {
    return main_frame_request_id_;
  }

  void set_main_frame_request_id(content::GlobalRequestID request_id) {
    main_frame_request_id_ = request_id;
  }

 private:
  // Identifier for the main frame for the page load this recorder is tracking.
  // Only valid if the data use is associated with a page load.
  RenderFrameHostID main_frame_id_;

  // Identifier for the MAIN_FRAME request for this page load. Only valid if
  // the data use is associated with a page load.
  content::GlobalRequestID main_frame_request_id_;

  DISALLOW_COPY_AND_ASSIGN(ChromeDataUseRecorder);
};

}  // namespace data_use_measurement

#endif  // CHROME_BROWSER_DATA_USE_MEASUREMENT_CHROME_DATA_USE_RECORDER_H_
