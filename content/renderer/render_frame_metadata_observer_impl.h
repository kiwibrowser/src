// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_RENDER_FRAME_METADATA_OBSERVER_IMPL_H_
#define CONTENT_RENDERER_RENDER_FRAME_METADATA_OBSERVER_IMPL_H_

#include "cc/trees/render_frame_metadata.h"
#include "cc/trees/render_frame_metadata_observer.h"
#include "content/common/render_frame_metadata.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cc {
class FrameTokenAllocator;
}  // namespace cc

namespace content {

// Implementation of cc::RenderFrameMetadataObserver which exists in the
// renderer an observers frame submission. It then notifies the
// mojom::RenderFrameMetadataObserverClient, which is expected to be in the
// browser process, of the metadata associated with the frame.
//
// BindToCurrentThread should be called from the Compositor thread so that the
// Mojo pipe is properly bound.
//
// Subsequent usage should only be from the Compositor thread.
class RenderFrameMetadataObserverImpl
    : public cc::RenderFrameMetadataObserver,
      public mojom::RenderFrameMetadataObserver {
 public:
  RenderFrameMetadataObserverImpl(
      mojom::RenderFrameMetadataObserverRequest request,
      mojom::RenderFrameMetadataObserverClientPtrInfo client_info);
  ~RenderFrameMetadataObserverImpl() override;

  // cc::RenderFrameMetadataObserver:
  void BindToCurrentThread(
      cc::FrameTokenAllocator* frame_token_allocator) override;
  void OnRenderFrameSubmission(cc::RenderFrameMetadata metadata) override;

  // mojom::RenderFrameMetadataObserver:
  void ReportAllFrameSubmissionsForTesting(bool enabled) override;

 private:
  // When true this will notifiy |render_frame_metadata_observer_client_| of all
  // frame submissions.
  bool report_all_frame_submissions_for_testing_enabled_ = false;

  uint32_t last_frame_token_ = 0;
  base::Optional<cc::RenderFrameMetadata> last_render_frame_metadata_;

  // Not owned.
  cc::FrameTokenAllocator* frame_token_allocator_ = nullptr;

  // These are destroyed when BindToCurrentThread() is called.
  mojom::RenderFrameMetadataObserverRequest request_;
  mojom::RenderFrameMetadataObserverClientPtrInfo client_info_;

  mojo::Binding<mojom::RenderFrameMetadataObserver>
      render_frame_metadata_observer_binding_;
  mojom::RenderFrameMetadataObserverClientPtr
      render_frame_metadata_observer_client_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameMetadataObserverImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_RENDER_FRAME_METADATA_OBSERVER_IMPL_H_
