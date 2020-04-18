// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_REMOTE_APPLICATION_DETAILS_H_
#define REMOTING_TEST_REMOTE_APPLICATION_DETAILS_H_

#include <string>

namespace remoting {
namespace test {

// Container for application details used for testing.
struct RemoteApplicationDetails {
  RemoteApplicationDetails(const std::string& app_id,
                           const std::string& window_title)
      : application_id(app_id), main_window_title(window_title) {}

  std::string application_id;
  std::string main_window_title;
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_REMOTE_APPLICATION_DETAILS_H_
