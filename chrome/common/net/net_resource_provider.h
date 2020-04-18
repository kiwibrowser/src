// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_NET_NET_RESOURCE_PROVIDER_H_
#define CHROME_COMMON_NET_NET_RESOURCE_PROVIDER_H_

#include "base/strings/string_piece.h"

namespace chrome_common_net {

// This is called indirectly by the network layer to access resources.
base::StringPiece NetResourceProvider(int key);

}  // namespace chrome_common_net

#endif  // CHROME_COMMON_NET_NET_RESOURCE_PROVIDER_H_
