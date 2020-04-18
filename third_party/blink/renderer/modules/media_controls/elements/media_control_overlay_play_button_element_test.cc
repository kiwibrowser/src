// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_overlay_play_button_element.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"

namespace blink {

class MediaControlOverlayPlayButtonElementTest : public PageTestBase {
 public:
  void SetUp() final {
    // Create page and instance of AnimatedArrow to run tests on.
    PageTestBase::SetUp();
    arrow_element_ = new MediaControlOverlayPlayButtonElement::AnimatedArrow(
        "test", GetDocument());
    GetDocument().body()->AppendChild(arrow_element_);
  }

 protected:
  void ExpectNotPresent() { EXPECT_FALSE(SVGElementIsPresent()); }

  void ExpectPresentAndShown() {
    EXPECT_TRUE(SVGElementIsPresent());
    EXPECT_FALSE(SVGElementHasDisplayValue());
  }

  void ExpectPresentAndHidden() {
    EXPECT_TRUE(SVGElementIsPresent());
    EXPECT_TRUE(SVGElementHasDisplayValue());
  }

  void SimulateShow() { arrow_element_->Show(); }

  void SimulateAnimationIteration() {
    Event* event = Event::Create(EventTypeNames::animationiteration);
    GetElementById("arrow-3")->DispatchEvent(event);
  }

 private:
  bool SVGElementHasDisplayValue() {
    return GetElementById("jump")->InlineStyle()->HasProperty(
        CSSPropertyDisplay);
  }

  bool SVGElementIsPresent() { return GetElementById("jump"); }

  Element* GetElementById(const AtomicString& id) {
    return GetDocument().body()->getElementById(id);
  }

  Persistent<MediaControlOverlayPlayButtonElement::AnimatedArrow>
      arrow_element_;
};

TEST_F(MediaControlOverlayPlayButtonElementTest, ShowIncrementsCounter) {
  ExpectNotPresent();

  // Start a new show.
  SimulateShow();
  ExpectPresentAndShown();

  // Increment the counter and finish the first show.
  SimulateShow();
  SimulateAnimationIteration();
  ExpectPresentAndShown();

  // Finish the second show.
  SimulateAnimationIteration();
  ExpectPresentAndHidden();

  // Start a new show.
  SimulateShow();
  ExpectPresentAndShown();
}

}  // namespace blink
