// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/media_download_in_product_help_manager.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_download_button_element.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_overflow_menu_button_element.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"

namespace blink {

MediaDownloadInProductHelpManager::MediaDownloadInProductHelpManager(
    MediaControlsImpl& controls)
    : controls_(controls) {}

MediaDownloadInProductHelpManager::~MediaDownloadInProductHelpManager() =
    default;

void MediaDownloadInProductHelpManager::SetControlsVisibility(bool can_show) {
  if (controls_can_show_ == can_show)
    return;

  controls_can_show_ = can_show;
  StateUpdated();
}

void MediaDownloadInProductHelpManager::SetDownloadButtonVisibility(
    bool can_show) {
  if (button_can_show_ == can_show)
    return;

  button_can_show_ = can_show;
  StateUpdated();
}

void MediaDownloadInProductHelpManager::SetIsPlaying(bool is_playing) {
  if (is_playing_ == is_playing)
    return;

  is_playing_ = is_playing;
  StateUpdated();
}

bool MediaDownloadInProductHelpManager::IsShowingInProductHelp() const {
  return media_in_product_help_.is_bound();
}

void MediaDownloadInProductHelpManager::UpdateInProductHelp() {
  if (!IsShowingInProductHelp() || !CanShowInProductHelp())
    return;

  MaybeDispatchDownloadInProductHelpTrigger(false);
}

void MediaDownloadInProductHelpManager::
    MaybeDispatchDownloadInProductHelpTrigger(bool create) {
  // Only show in-product-help once for an element.
  if (create && media_download_in_product_trigger_observed_)
    return;

  auto* frame = controls_->GetDocument().GetFrame();
  if (!frame)
    return;

  // If the button is not in the viewport, don't show the in-product-help.
  IntRect button_rect =
      controls_->IsModern()
          ? controls_->OverflowButton().VisibleBoundsInVisualViewport()
          : controls_->DownloadButton().VisibleBoundsInVisualViewport();
  if (button_rect.IsEmpty())
    return;

  if (download_button_rect_ == button_rect && media_in_product_help_.is_bound())
    return;

  download_button_rect_ = button_rect;
  media_download_in_product_trigger_observed_ = true;
  if (!media_in_product_help_.is_bound()) {
    frame->Client()->GetInterfaceProvider()->GetInterface(
        mojo::MakeRequest(&media_in_product_help_));
    media_in_product_help_.set_connection_error_handler(
        WTF::Bind(&MediaDownloadInProductHelpManager::DismissInProductHelp,
                  WrapWeakPersistent(this)));
    DCHECK(media_in_product_help_.is_bound());
  }

  // MaybeShow should always make the controls visible since we early out if
  // CanShow is false for the controls.
  controls_->MaybeShow();
  media_in_product_help_->ShowInProductHelpWidget(button_rect);
}

void MediaDownloadInProductHelpManager::StateUpdated() {
  if (CanShowInProductHelp())
    MaybeDispatchDownloadInProductHelpTrigger(true);
  else
    DismissInProductHelp();
}

bool MediaDownloadInProductHelpManager::CanShowInProductHelp() const {
  // In-product help should only be shown if the controls can be made visible,
  // the download button is wanted and the video is not paused.
  return controls_can_show_ && button_can_show_ && is_playing_;
}

void MediaDownloadInProductHelpManager::DismissInProductHelp() {
  download_button_rect_ = IntRect();
  if (!media_in_product_help_.is_bound())
    return;

  media_in_product_help_.reset();
  controls_->DidDismissDownloadInProductHelp();
}

void MediaDownloadInProductHelpManager::Trace(blink::Visitor* visitor) {
  visitor->Trace(controls_);
}

}  // namespace blink
