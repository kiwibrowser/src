// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MANIFEST_MANIFEST_MANAGER_H_
#define CONTENT_RENDERER_MANIFEST_MANIFEST_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/blink/public/common/manifest/manifest.h"
#include "third_party/blink/public/mojom/manifest/manifest_manager.mojom.h"

class GURL;

namespace blink {
class WebURLResponse;
}

namespace content {

class ManifestFetcher;

// The ManifestManager is a helper class that takes care of fetching and parsing
// the Manifest of the associated RenderFrame. It uses the ManifestFetcher and
// the ManifestParser in order to do so.
//
// Consumers should use the mojo ManifestManager interface to use this class.
class ManifestManager : public RenderFrameObserver,
                        public blink::mojom::ManifestManager {
 public:
  static bool CanFetchManifest(RenderFrame* render_frame);

  explicit ManifestManager(RenderFrame* render_frame);
  ~ManifestManager() override;

  // RenderFrameObserver implementation.
  void DidChangeManifest() override;
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_document_navigation) override;

  void BindToRequest(blink::mojom::ManifestManagerRequest request);

 private:
  enum ResolveState {
    ResolveStateSuccess,
    ResolveStateFailure
  };

  using InternalRequestManifestCallback =
      base::OnceCallback<void(const GURL&,
                              const blink::Manifest&,
                              const blink::mojom::ManifestDebugInfo*)>;

  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // blink::mojom::ManifestManager implementation.
  void RequestManifest(RequestManifestCallback callback) override;
  void RequestManifestDebugInfo(
      RequestManifestDebugInfoCallback callback) override;

  void RequestManifestImpl(InternalRequestManifestCallback callback);

  void FetchManifest();
  void OnManifestFetchComplete(const GURL& document_url,
                               const blink::WebURLResponse& response,
                               const std::string& data);
  void ResolveCallbacks(ResolveState state);

  std::unique_ptr<ManifestFetcher> fetcher_;

  // Whether the RenderFrame may have an associated Manifest. If true, the frame
  // may have a manifest, if false, it can't have one. This boolean is true when
  // DidChangeManifest() is called, if it is never called, it means that the
  // associated document has no <link rel='manifest'>.
  bool may_have_manifest_;

  // Whether the current Manifest is dirty.
  bool manifest_dirty_;

  // Current Manifest. Might be outdated if manifest_dirty_ is true.
  blink::Manifest manifest_;

  // The URL of the current manifest.
  GURL manifest_url_;

  // Current Manifest debug information.
  blink::mojom::ManifestDebugInfoPtr manifest_debug_info_;

  std::vector<InternalRequestManifestCallback> pending_callbacks_;

  mojo::BindingSet<blink::mojom::ManifestManager> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ManifestManager);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MANIFEST_MANIFEST_MANAGER_H_
