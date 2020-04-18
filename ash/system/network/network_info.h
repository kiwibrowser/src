// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NETWORK_NETWORK_INFO_H_
#define ASH_SYSTEM_NETWORK_NETWORK_INFO_H_

#include <string>

#include "base/strings/string16.h"
#include "ui/gfx/image/image_skia.h"

namespace gfx {
class ImageSkia;
}

namespace ash {

// Includes information necessary about a network for displaying the appropriate
// UI to the user.
struct NetworkInfo {
  enum class Type { UNKNOWN, WIFI, MOBILE };

  NetworkInfo();
  NetworkInfo(const std::string& guid);
  ~NetworkInfo();

  bool operator==(const NetworkInfo& other) const;
  bool operator!=(const NetworkInfo& other) const { return !(*this == other); }

  std::string guid;
  base::string16 label;
  base::string16 tooltip;
  gfx::ImageSkia image;
  bool disable;
  bool connected;
  bool connecting;
  Type type;
};

}  // namespace ash

#endif  // ASH_SYSTEM_NETWORK_NETWORK_INFO_H_
