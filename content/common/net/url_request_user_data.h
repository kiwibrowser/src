// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_NET_URL_REQUEST_USER_DATA_H_
#define CONTENT_COMMON_NET_URL_REQUEST_USER_DATA_H_

#include "base/supports_user_data.h"

namespace content {

// Used to annotate all URLRequests for which the request can be associated
// with a given RenderFrame.
class URLRequestUserData : public base::SupportsUserData::Data {
 public:
  URLRequestUserData(int render_process_id,
                     int render_frame_id);
  ~URLRequestUserData() override;

  int render_process_id() const { return render_process_id_; }
  int render_frame_id() const { return render_frame_id_; }

  static const void* const kUserDataKey;

 private:
  int render_process_id_;
  int render_frame_id_;
};

}  // namespace content

#endif  // CONTENT_COMMON_NET_URL_REQUEST_USER_DATA_H_
