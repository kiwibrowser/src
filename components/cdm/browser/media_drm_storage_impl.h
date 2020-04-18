// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CDM_BROWSER_MEDIA_DRM_STORAGE_IMPL_H_
#define COMPONENTS_CDM_BROWSER_MEDIA_DRM_STORAGE_IMPL_H_

#include <set>
#include <vector>

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "content/public/browser/frame_service_base.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "media/mojo/interfaces/media_drm_storage.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "url/gurl.h"
#include "url/origin.h"

class PrefRegistrySimple;
class PrefService;

namespace content {
class RenderFrameHost;
}

namespace cdm {

// Implements media::mojom::MediaDrmStorage using PrefService.
// This file is located under components/ so that it can be shared by multiple
// content embedders (e.g. chrome and chromecast).
class MediaDrmStorageImpl final
    : public content::FrameServiceBase<media::mojom::MediaDrmStorage> {
 public:
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Get a list of origins that have persistent storage on the device.
  static std::set<GURL> GetAllOrigins(const PrefService* pref_service);

  // Clear licenses if:
  // 1. The license creation time falls in [|start|, |end|], and
  // 2. |filter| returns true on the media license's origin.
  //
  // Return a list of origin IDs that have no licenses remaining so that the
  // origin can be unprovisioned.
  //
  // TODO(yucliu): Add unit test.
  static std::vector<base::UnguessableToken> ClearMatchingLicenses(
      PrefService* pref_service,
      base::Time start,
      base::Time end,
      const base::RepeatingCallback<bool(const GURL&)>& filter);

  MediaDrmStorageImpl(content::RenderFrameHost* render_frame_host,
                      PrefService* pref_service,
                      media::mojom::MediaDrmStorageRequest request);

  // media::mojom::MediaDrmStorage implementation.
  void Initialize(InitializeCallback callback) final;
  void OnProvisioned(OnProvisionedCallback callback) final;
  void SavePersistentSession(const std::string& session_id,
                             media::mojom::SessionDataPtr session_data,
                             SavePersistentSessionCallback callback) final;
  void LoadPersistentSession(const std::string& session_id,
                             LoadPersistentSessionCallback callback) final;
  void RemovePersistentSession(const std::string& session_id,
                               RemovePersistentSessionCallback callback) final;

  bool IsInitialized() const { return !!origin_id_; }

 private:
  // |this| can only be destructed as a FrameServiceBase.
  ~MediaDrmStorageImpl() final;

  PrefService* const pref_service_ = nullptr;

  // ID for the current origin. Per EME spec on individualization,
  // implementation should not expose application-specific information.
  base::UnguessableToken origin_id_;
};

}  // namespace cdm

#endif  // COMPONENTS_CDM_BROWSER_MEDIA_DRM_STORAGE_IMPL_H_
