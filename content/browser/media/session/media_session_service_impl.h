// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_SERVICE_IMPL_H_
#define CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_SERVICE_IMPL_H_

#include "base/optional.h"
#include "content/public/common/media_metadata.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/modules/mediasession/media_session.mojom.h"

namespace content {

class RenderFrameHost;
class MediaSessionImpl;

// There is one MediaSessionService per frame. The class is owned by
// RenderFrameHost and should register/unregister itself to/from
// MediaSessionImpl when RenderFrameHost is created/destroyed.
class CONTENT_EXPORT MediaSessionServiceImpl
    : public blink::mojom::MediaSessionService {
 public:
  ~MediaSessionServiceImpl() override;

  static void Create(RenderFrameHost* render_frame_host,
                     blink::mojom::MediaSessionServiceRequest request);
  const blink::mojom::MediaSessionClientPtr& GetClient() { return client_; }
  RenderFrameHost* GetRenderFrameHost();

  blink::mojom::MediaSessionPlaybackState playback_state() const {
    return playback_state_;
  }
  const base::Optional<MediaMetadata>& metadata() const { return metadata_; }
  const std::set<blink::mojom::MediaSessionAction>& actions() const {
    return actions_;
  }

  void DidFinishNavigation();

  // blink::mojom::MediaSessionService implementation.
  void SetClient(blink::mojom::MediaSessionClientPtr client) override;

  void SetPlaybackState(blink::mojom::MediaSessionPlaybackState state) override;
  void SetMetadata(const base::Optional<MediaMetadata>& metadata) override;

  void EnableAction(blink::mojom::MediaSessionAction action) override;
  void DisableAction(blink::mojom::MediaSessionAction action) override;

 protected:
  explicit MediaSessionServiceImpl(RenderFrameHost* render_frame_host);

 private:
  MediaSessionImpl* GetMediaSession();

  void Bind(blink::mojom::MediaSessionServiceRequest request);

  void ClearActions();

  const int render_frame_process_id_;
  const int render_frame_routing_id_;

  // RAII binding of |this| to an MediaSessionService interface request.
  // The binding is removed when binding_ is cleared or goes out of scope.
  std::unique_ptr<mojo::Binding<blink::mojom::MediaSessionService>> binding_;
  blink::mojom::MediaSessionClientPtr client_;
  blink::mojom::MediaSessionPlaybackState playback_state_;
  base::Optional<MediaMetadata> metadata_;
  std::set<blink::mojom::MediaSessionAction> actions_;

  DISALLOW_COPY_AND_ASSIGN(MediaSessionServiceImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_SERVICE_IMPL_H_
