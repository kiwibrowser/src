// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_ELEMENTS_MEDIA_CONTROL_OVERFLOW_MENU_LIST_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_ELEMENTS_MEDIA_CONTROL_OVERFLOW_MENU_LIST_ELEMENT_H_

#include "base/optional.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_popup_menu_element.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class Event;
class MediaControlsImpl;

// Holds a list of elements within the overflow menu.
class MediaControlOverflowMenuListElement final
    : public MediaControlPopupMenuElement {
 public:
  explicit MediaControlOverflowMenuListElement(MediaControlsImpl&);

  // Override MediaControlPopupMenuElement
  void SetIsWanted(bool) final;
  Element* PopupAnchor() const final;
  void OnItemSelected() final;

 private:
  enum TimeTakenHistogram {
    kTimeToAction,
    kTimeToDismiss,
  };
  void MaybeRecordTimeTaken(TimeTakenHistogram);

  void DefaultEventHandler(Event*) override;

  TaskHandle current_task_handle_;

  base::Optional<WTF::TimeTicks> time_shown_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_ELEMENTS_MEDIA_CONTROL_OVERFLOW_MENU_LIST_ELEMENT_H_
