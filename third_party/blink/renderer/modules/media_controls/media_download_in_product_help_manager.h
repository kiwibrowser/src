// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_MEDIA_DOWNLOAD_IN_PRODUCT_HELP_MANAGER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_MEDIA_DOWNLOAD_IN_PRODUCT_HELP_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "third_party/blink/public/platform/media_download_in_product_help.mojom-blink.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {
class MediaControlsImpl;

class MODULES_EXPORT MediaDownloadInProductHelpManager final
    : public GarbageCollectedFinalized<MediaDownloadInProductHelpManager> {
 public:
  explicit MediaDownloadInProductHelpManager(MediaControlsImpl&);
  virtual ~MediaDownloadInProductHelpManager();

  void SetControlsVisibility(bool can_show);
  void SetDownloadButtonVisibility(bool can_show);
  void SetIsPlaying(bool is_playing);
  bool IsShowingInProductHelp() const;
  void UpdateInProductHelp();

  virtual void Trace(blink::Visitor*);

 private:
  void StateUpdated();
  bool CanShowInProductHelp() const;
  void MaybeDispatchDownloadInProductHelpTrigger(bool create);
  void DismissInProductHelp();

  Member<MediaControlsImpl> controls_;

  bool controls_can_show_ = false;
  bool button_can_show_ = false;
  bool is_playing_ = false;
  bool media_download_in_product_trigger_observed_ = false;
  IntRect download_button_rect_;

  mojom::blink::MediaDownloadInProductHelpPtr media_in_product_help_;

  DISALLOW_COPY_AND_ASSIGN(MediaDownloadInProductHelpManager);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_MEDIA_DOWNLOAD_IN_PRODUCT_HELP_MANAGER_H_
