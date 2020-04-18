// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_CONFIG_GPU_BLACKLIST_H_
#define GPU_CONFIG_GPU_BLACKLIST_H_

#include <memory>

#include "base/macros.h"
#include "gpu/config/gpu_control_list.h"

namespace gpu {

class GPU_EXPORT GpuBlacklist : public GpuControlList {
 public:
  ~GpuBlacklist() override;

  static std::unique_ptr<GpuBlacklist> Create();
  static std::unique_ptr<GpuBlacklist> Create(const GpuControlListData& data);

  static bool AreEntryIndicesValid(const std::vector<uint32_t>& entry_indices);

 private:
  explicit GpuBlacklist(const GpuControlListData& data);

  DISALLOW_COPY_AND_ASSIGN(GpuBlacklist);
};

}  // namespace gpu

#endif  // GPU_CONFIG_GPU_BLACKLIST_H_
