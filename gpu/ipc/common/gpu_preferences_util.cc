// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/common/gpu_preferences_util.h"

#include "base/base64.h"
#include "gpu/ipc/common/gpu_preferences_struct_traits.h"

namespace gpu {

std::string GpuPreferencesToSwitchValue(const GpuPreferences& prefs) {
  std::vector<uint8_t> serialized =
      gpu::mojom::GpuPreferences::Serialize(&prefs);

  std::string encoded;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(serialized.data()),
                        serialized.size()),
      &encoded);
  return encoded;
}

bool SwitchValueToGpuPreferences(const std::string& data,
                                 GpuPreferences* output_prefs) {
  DCHECK(output_prefs);
  std::string decoded;
  if (!base::Base64Decode(data, &decoded))
    return false;
  if (!gpu::mojom::GpuPreferences::Deserialize(decoded.data(), decoded.size(),
                                               output_prefs)) {
    return false;
  }
  return true;
}

}  // namespace gpu
