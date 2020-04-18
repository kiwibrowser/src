// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_overflow_menu_list_element.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_overflow_menu_button_element.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"
#include "third_party/blink/renderer/platform/histogram.h"

namespace blink {

MediaControlOverflowMenuListElement::MediaControlOverflowMenuListElement(
    MediaControlsImpl& media_controls)
    : MediaControlPopupMenuElement(media_controls, kMediaOverflowList) {
  SetShadowPseudoId(
      AtomicString("-internal-media-controls-overflow-menu-list"));
}

void MediaControlOverflowMenuListElement::MaybeRecordTimeTaken(
    TimeTakenHistogram histogram_name) {
  DCHECK(time_shown_);

  if (current_task_handle_.IsActive())
    current_task_handle_.Cancel();

  LinearHistogram histogram(histogram_name == kTimeToAction
                                ? "Media.Controls.Overflow.TimeToAction"
                                : "Media.Controls.Overflow.TimeToDismiss",
                            1, 100, 100);
  histogram.Count(static_cast<int32_t>(
      (CurrentTimeTicks() - time_shown_.value()).InSeconds()));

  time_shown_.reset();
}

void MediaControlOverflowMenuListElement::DefaultEventHandler(Event* event) {
  if (event->type() == EventTypeNames::click)
    event->SetDefaultHandled();

  MediaControlPopupMenuElement::DefaultEventHandler(event);
}

void MediaControlOverflowMenuListElement::SetIsWanted(bool wanted) {
  MediaControlPopupMenuElement::SetIsWanted(wanted);

  // Record the time the overflow menu was shown to a histogram.
  if (wanted) {
    DCHECK(!time_shown_);
    time_shown_ = CurrentTimeTicks();
  } else if (time_shown_) {
    // Records the time taken to dismiss using a task runner. This ensures the
    // time to dismiss is always called after the time to action (if there is
    // one). The time to action call will then cancel the time to dismiss as it
    // is not needed.
    DCHECK(!current_task_handle_.IsActive());
    current_task_handle_ = PostCancellableTask(
        *GetDocument().GetTaskRunner(TaskType::kMediaElementEvent), FROM_HERE,
        WTF::Bind(&MediaControlOverflowMenuListElement::MaybeRecordTimeTaken,
                  WrapWeakPersistent(this), kTimeToDismiss));
  }
}

Element* MediaControlOverflowMenuListElement::PopupAnchor() const {
  return &GetMediaControls().OverflowButton();
}

void MediaControlOverflowMenuListElement::OnItemSelected() {
  MaybeRecordTimeTaken(kTimeToAction);

  MediaControlPopupMenuElement::OnItemSelected();
}

}  // namespace blink
