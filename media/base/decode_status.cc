// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/decode_status.h"

#include <ostream>

namespace media {

std::ostream& operator<<(std::ostream& os, const DecodeStatus& status) {
  switch (status) {
    case DecodeStatus::OK:
      os << "DecodeStatus::OK";
      break;
    case DecodeStatus::ABORTED:
      os << "DecodeStatus::ABORTED";
      break;
    case DecodeStatus::DECODE_ERROR:
      os << "DecodeStatus::DECODE_ERROR";
      break;
  }
  return os;
}

}  // namespace media
