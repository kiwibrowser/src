// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_MEDIA_DRM_STORAGE_H_
#define MEDIA_MOJO_SERVICES_MOJO_MEDIA_DRM_STORAGE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/base/android/media_drm_storage.h"
#include "media/mojo/interfaces/media_drm_storage.mojom.h"
#include "media/mojo/services/media_mojo_export.h"

namespace media {

// A MediaDrmStorage that proxies to a mojom::MediaDrmStoragePtr.
class MEDIA_MOJO_EXPORT MojoMediaDrmStorage : public MediaDrmStorage {
 public:
  explicit MojoMediaDrmStorage(mojom::MediaDrmStoragePtr media_drm_storage_ptr);
  ~MojoMediaDrmStorage() final;

  // MediaDrmStorage implementation:
  void Initialize(InitCB init_cb) final;
  void OnProvisioned(ResultCB result_cb) final;
  void SavePersistentSession(const std::string& session_id,
                             const SessionData& session_data,
                             ResultCB result_cb) final;
  void LoadPersistentSession(
      const std::string& session_id,
      LoadPersistentSessionCB load_persistent_session_cb) final;
  void RemovePersistentSession(const std::string& session_id,
                               ResultCB result_cb) final;

 private:
  void OnPersistentSessionLoaded(
      LoadPersistentSessionCB load_persistent_session_cb,
      mojom::SessionDataPtr session_data);

  mojom::MediaDrmStoragePtr media_drm_storage_ptr_;
  base::WeakPtrFactory<MojoMediaDrmStorage> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoMediaDrmStorage);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_MEDIA_DRM_STORAGE_H_
