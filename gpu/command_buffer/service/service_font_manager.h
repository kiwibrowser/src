// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SERVICE_FONT_MANAGER_H_
#define GPU_COMMAND_BUFFER_SERVICE_SERVICE_FONT_MANAGER_H_

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "gpu/command_buffer/common/discardable_handle.h"
#include "gpu/gpu_gles2_export.h"
#include "third_party/skia/src/core/SkRemoteGlyphCache.h"

namespace gpu {
class Buffer;

class GPU_GLES2_EXPORT ServiceFontManager {
 public:
  class GPU_GLES2_EXPORT Client {
   public:
    virtual ~Client() {}
    virtual scoped_refptr<Buffer> GetShmBuffer(uint32_t shm_id) = 0;
  };

  ServiceFontManager(Client* client);
  ~ServiceFontManager();

  bool Deserialize(const volatile char* memory,
                   size_t memory_size,
                   std::vector<SkDiscardableHandleId>* locked_handles);
  bool Unlock(const std::vector<SkDiscardableHandleId>& handles);
  SkStrikeClient* strike_client() { return strike_client_.get(); }

 private:
  class SkiaDiscardableManager;

  bool AddHandle(SkDiscardableHandleId handle_id,
                 ServiceDiscardableHandle handle);
  bool DeleteHandle(SkDiscardableHandleId handle_id);

  Client* client_;
  std::unique_ptr<SkStrikeClient> strike_client_;
  base::flat_map<SkDiscardableHandleId, ServiceDiscardableHandle>
      discardable_handle_map_;
  base::WeakPtrFactory<ServiceFontManager> weak_factory_;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_SERVICE_FONT_MANAGER_H_
