// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_overflow_menu_button_element.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_download_button_element.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"
#include "third_party/blink/renderer/modules/media_controls/media_download_in_product_help_manager.h"

namespace blink {

MediaControlOverflowMenuButtonElement::MediaControlOverflowMenuButtonElement(
    MediaControlsImpl& media_controls)
    : MediaControlInputElement(media_controls, kMediaOverflowButton) {
  setType(InputTypeNames::button);
  SetShadowPseudoId(AtomicString("-internal-media-controls-overflow-button"));
  SetIsWanted(false);
}

bool MediaControlOverflowMenuButtonElement::WillRespondToMouseClickEvents() {
  return true;
}

const char* MediaControlOverflowMenuButtonElement::GetNameForHistograms()
    const {
  return "OverflowButton";
}

void MediaControlOverflowMenuButtonElement::UpdateShownState() {
  MediaControlInputElement::UpdateShownState();

  if (MediaControlsImpl::IsModern() &&
      GetMediaControls().DownloadInProductHelp()) {
    GetMediaControls().DownloadInProductHelp()->SetDownloadButtonVisibility(
        IsWanted() && DoesFit() &&
        GetMediaControls().DownloadButton().ShouldDisplayDownloadButton());
  }
}

void MediaControlOverflowMenuButtonElement::DefaultEventHandler(Event* event) {
  // Only respond to a click event if we are not disabled.
  if (!hasAttribute(HTMLNames::disabledAttr) &&
      event->type() == EventTypeNames::click) {
    if (GetMediaControls().OverflowMenuVisible()) {
      Platform::Current()->RecordAction(
          UserMetricsAction("Media.Controls.OverflowClose"));
    } else {
      Platform::Current()->RecordAction(
          UserMetricsAction("Media.Controls.OverflowOpen"));
    }

    GetMediaControls().ToggleOverflowMenu();
    event->SetDefaultHandled();
  }

  MediaControlInputElement::DefaultEventHandler(event);
}

}  // namespace blink
