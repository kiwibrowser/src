/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "rtc_base/string_encode.h"

namespace webrtc {

// Fuzz s_url_decode which is used in ice server parsing.
void FuzzOneInput(const uint8_t* data, size_t size) {
  std::string url(reinterpret_cast<const char*>(data), size);
  rtc::s_url_decode(url);
}

}  // namespace webrtc
