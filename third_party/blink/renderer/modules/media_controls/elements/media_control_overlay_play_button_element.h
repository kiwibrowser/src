// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_ELEMENTS_MEDIA_CONTROL_OVERLAY_PLAY_BUTTON_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_ELEMENTS_MEDIA_CONTROL_OVERLAY_PLAY_BUTTON_ELEMENT_H_

#include "third_party/blink/renderer/core/html/html_div_element.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_animation_event_listener.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_input_element.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/timer.h"

namespace WTF {
class AtomicString;
}

namespace blink {

class Event;
class MediaControlsImpl;

class MODULES_EXPORT MediaControlOverlayPlayButtonElement final
    : public MediaControlInputElement {
 public:
  explicit MediaControlOverlayPlayButtonElement(MediaControlsImpl&);

  // MediaControlInputElement overrides.
  void UpdateDisplayType() override;

  void OnMediaKeyboardEvent(Event* event) { DefaultEventHandler(event); }

  WebSize GetSizeOrDefault() const final;

  void SetIsDisplayed(bool);

  void Trace(blink::Visitor*) override;

 protected:
  const char* GetNameForHistograms() const override;

 private:
  friend class MediaControlOverlayPlayButtonElementTest;

  // This class is responible for displaying the arrow animation when a jump is
  // triggered by the user.
  class MODULES_EXPORT AnimatedArrow final
      : public HTMLDivElement,
        public MediaControlAnimationEventListener::Observer {
    USING_GARBAGE_COLLECTED_MIXIN(AnimatedArrow);

   public:
    AnimatedArrow(const AtomicString& id, Document& document);

    // MediaControlAnimationEventListener::Observer overrides
    void OnAnimationIteration() override;
    void OnAnimationEnd() override{};
    Element& WatchedAnimationElement() const override;

    // Shows the animated arrows for a single animation iteration. If the
    // arrows are already shown it will show them for another animation
    // iteration.
    void Show();

    void Trace(Visitor*) override;

   private:
    void HideInternal();
    void ShowInternal();

    int counter_ = 0;
    bool hidden_ = true;

    Member<Element> last_arrow_;
    Member<Element> svg_container_;
    Member<MediaControlAnimationEventListener> event_listener_;
  };

  void TapTimerFired(TimerBase*);

  void DefaultEventHandler(Event*) override;
  bool KeepEventInNode(Event*) override;

  bool ShouldCausePlayPause(Event*) const;
  void MaybePlayPause();
  void MaybeJump(int);

  TaskRunnerTimer<MediaControlOverlayPlayButtonElement> tap_timer_;
  base::Optional<bool> tap_was_touch_event_;

  Member<HTMLDivElement> internal_button_;
  Member<AnimatedArrow> left_jump_arrow_;
  Member<AnimatedArrow> right_jump_arrow_;

  bool displayed_ = true;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_ELEMENTS_MEDIA_CONTROL_OVERLAY_PLAY_BUTTON_ELEMENT_H_
