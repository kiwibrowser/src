// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/mojo/event.mojom.h"
#include "ui/events/mojo/event_struct_traits.h"
#include "ui/events/mojo/traits_test_service.mojom.h"
#include "ui/latency/mojo/latency_info_struct_traits.h"

namespace ui {

namespace {

class StructTraitsTest : public testing::Test, public mojom::TraitsTestService {
 public:
  StructTraitsTest() {}
  ~StructTraitsTest() override = default;

 protected:
  mojom::TraitsTestServicePtr GetTraitsTestProxy() {
    mojom::TraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // TraitsTestService:
  void EchoEvent(std::unique_ptr<ui::Event> e,
                 EchoEventCallback callback) override {
    std::move(callback).Run(std::move(e));
  }

  base::MessageLoop loop_;
  mojo::BindingSet<TraitsTestService> traits_test_bindings_;
  DISALLOW_COPY_AND_ASSIGN(StructTraitsTest);
};

}  // namespace

TEST_F(StructTraitsTest, KeyEvent) {
  KeyEvent kTestData[] = {
      {ET_KEY_PRESSED, VKEY_RETURN, EF_CONTROL_DOWN},
      {ET_KEY_PRESSED, VKEY_MENU, EF_ALT_DOWN},
      {ET_KEY_RELEASED, VKEY_SHIFT, EF_SHIFT_DOWN},
      {ET_KEY_RELEASED, VKEY_MENU, EF_ALT_DOWN},
      {ET_KEY_PRESSED, VKEY_A, ui::DomCode::US_A, EF_NONE},
      {ET_KEY_PRESSED, VKEY_B, ui::DomCode::US_B,
       EF_CONTROL_DOWN | EF_ALT_DOWN},
      {'\x12', VKEY_2, EF_CONTROL_DOWN},
      {'Z', VKEY_Z, EF_CAPS_LOCK_ON},
      {'z', VKEY_Z, EF_NONE},
      {ET_KEY_PRESSED, VKEY_Z, EF_NONE,
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(101)},
      {'Z', VKEY_Z, EF_NONE,
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(102)},
  };

  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  for (size_t i = 0; i < arraysize(kTestData); i++) {
    std::unique_ptr<Event> output;
    proxy->EchoEvent(Event::Clone(kTestData[i]), &output);
    EXPECT_TRUE(output->IsKeyEvent());

    const KeyEvent* output_key_event = output->AsKeyEvent();
    EXPECT_EQ(kTestData[i].type(), output_key_event->type());
    EXPECT_EQ(kTestData[i].flags(), output_key_event->flags());
    EXPECT_EQ(kTestData[i].GetCharacter(), output_key_event->GetCharacter());
    EXPECT_EQ(kTestData[i].GetUnmodifiedText(),
              output_key_event->GetUnmodifiedText());
    EXPECT_EQ(kTestData[i].GetText(), output_key_event->GetText());
    EXPECT_EQ(kTestData[i].is_char(), output_key_event->is_char());
    EXPECT_EQ(kTestData[i].is_repeat(), output_key_event->is_repeat());
    EXPECT_EQ(kTestData[i].GetConflatedWindowsKeyCode(),
              output_key_event->GetConflatedWindowsKeyCode());
    EXPECT_EQ(kTestData[i].code(), output_key_event->code());
    EXPECT_EQ(kTestData[i].time_stamp(), output_key_event->time_stamp());
  }
}

TEST_F(StructTraitsTest, PointerEvent) {
  PointerEvent kTestData[] = {
      // Mouse pointer events:
      {ET_POINTER_DOWN, gfx::Point(10, 10), gfx::Point(20, 30), EF_NONE, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_MOUSE,
                      MouseEvent::kMousePointerId),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(201)},
      {ET_POINTER_MOVED, gfx::Point(1, 5), gfx::Point(5, 1),
       EF_LEFT_MOUSE_BUTTON, EF_LEFT_MOUSE_BUTTON,
       PointerDetails(EventPointerType::POINTER_TYPE_MOUSE,
                      MouseEvent::kMousePointerId),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(202)},
      {ET_POINTER_UP, gfx::Point(411, 130), gfx::Point(20, 30),
       EF_MIDDLE_MOUSE_BUTTON | EF_RIGHT_MOUSE_BUTTON, EF_RIGHT_MOUSE_BUTTON,
       PointerDetails(EventPointerType::POINTER_TYPE_MOUSE,
                      MouseEvent::kMousePointerId),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(203)},
      {ET_POINTER_CANCELLED, gfx::Point(0, 1), gfx::Point(2, 3), EF_ALT_DOWN, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_MOUSE,
                      MouseEvent::kMousePointerId),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(204)},
      {ET_POINTER_ENTERED, gfx::Point(6, 7), gfx::Point(8, 9), EF_NONE, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_MOUSE,
                      MouseEvent::kMousePointerId),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(205)},
      {ET_POINTER_EXITED, gfx::Point(10, 10), gfx::Point(20, 30),
       EF_BACK_MOUSE_BUTTON, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_MOUSE,
                      MouseEvent::kMousePointerId),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(206)},
      {ET_POINTER_CAPTURE_CHANGED, gfx::Point(99, 99), gfx::Point(99, 99),
       EF_CONTROL_DOWN, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_MOUSE,
                      MouseEvent::kMousePointerId),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(207)},

      // Touch pointer events:
      {ET_POINTER_DOWN, gfx::Point(10, 10), gfx::Point(20, 30), EF_NONE, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_TOUCH,
                      /* pointer_id */ 1,
                      /* radius_x */ 1.0f,
                      /* radius_y */ 2.0f,
                      /* force */ 3.0f,
                      /* twist */ 0,
                      /* tilt_x */ 4.0f,
                      /* tilt_y */ 5.0f),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(208)},
      {ET_POINTER_CANCELLED, gfx::Point(120, 120), gfx::Point(2, 3), EF_NONE, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_TOUCH,
                      /* pointer_id */ 2,
                      /* radius_x */ 5.5f,
                      /* radius_y */ 4.5f,
                      /* force */ 3.5f,
                      /* twist */ 0,
                      /* tilt_x */ 2.5f,
                      /* tilt_y */ 0.5f),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(209)},

      // Pen pointer events:
      {ET_POINTER_DOWN, gfx::Point(1, 2), gfx::Point(3, 4), EF_NONE, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_PEN,
                      /* pointer_id */ 3,
                      /* radius_x */ 1.0f,
                      /* radius_y */ 2.0f,
                      /* force */ 3.0f,
                      /* twist */ 90,
                      /* tilt_x */ 4.0f,
                      /* tilt_y */ 5.0f,
                      /* tangential_pressure */ -1.f),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(210)},
      {ET_POINTER_UP, gfx::Point(5, 6), gfx::Point(7, 8), EF_NONE, 0,
       PointerDetails(EventPointerType::POINTER_TYPE_PEN,
                      /* pointer_id */ 3,
                      /* radius_x */ 1.0f,
                      /* radius_y */ 2.0f,
                      /* force */ 3.0f,
                      /* twist */ 180,
                      /* tilt_x */ 4.0f,
                      /* tilt_y */ 5.0f,
                      /* tangential_pressure */ 1.f),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(211)},
  };

  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  for (size_t i = 0; i < arraysize(kTestData); i++) {
    std::unique_ptr<Event> output;
    proxy->EchoEvent(Event::Clone(kTestData[i]), &output);
    EXPECT_TRUE(output->IsPointerEvent());

    const PointerEvent* output_ptr_event = output->AsPointerEvent();
    EXPECT_EQ(kTestData[i].type(), output_ptr_event->type());
    EXPECT_EQ(kTestData[i].flags(), output_ptr_event->flags());
    EXPECT_EQ(kTestData[i].location(), output_ptr_event->location());
    EXPECT_EQ(kTestData[i].root_location(), output_ptr_event->root_location());
    EXPECT_EQ(kTestData[i].pointer_details().id,
              output_ptr_event->pointer_details().id);
    EXPECT_EQ(kTestData[i].changed_button_flags(),
              output_ptr_event->changed_button_flags());
    EXPECT_EQ(kTestData[i].pointer_details(),
              output_ptr_event->pointer_details());
    EXPECT_EQ(kTestData[i].time_stamp(), output_ptr_event->time_stamp());
  }
}

TEST_F(StructTraitsTest, PointerWheelEvent) {
  MouseWheelEvent kTestData[] = {
      {gfx::Vector2d(11, 15), gfx::Point(3, 4), gfx::Point(40, 30),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(301),
       EF_LEFT_MOUSE_BUTTON, EF_LEFT_MOUSE_BUTTON},
      {gfx::Vector2d(-5, 3), gfx::Point(40, 3), gfx::Point(4, 0),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(302),
       EF_MIDDLE_MOUSE_BUTTON | EF_RIGHT_MOUSE_BUTTON,
       EF_MIDDLE_MOUSE_BUTTON | EF_RIGHT_MOUSE_BUTTON},
      {gfx::Vector2d(1, 0), gfx::Point(3, 4), gfx::Point(40, 30),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(303), EF_NONE,
       EF_NONE},
  };

  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  for (size_t i = 0; i < arraysize(kTestData); i++) {
    std::unique_ptr<Event> output;
    proxy->EchoEvent(Event::Clone(ui::PointerEvent(kTestData[i])), &output);
    EXPECT_EQ(ET_POINTER_WHEEL_CHANGED, output->type());

    const PointerEvent* output_pointer_event = output->AsPointerEvent();
    EXPECT_EQ(ET_POINTER_WHEEL_CHANGED, output_pointer_event->type());
    EXPECT_EQ(kTestData[i].flags(), output_pointer_event->flags());
    EXPECT_EQ(kTestData[i].location(), output_pointer_event->location());
    EXPECT_EQ(kTestData[i].root_location(),
              output_pointer_event->root_location());
    EXPECT_EQ(kTestData[i].offset(),
              output_pointer_event->pointer_details().offset);
    EXPECT_EQ(kTestData[i].time_stamp(), output_pointer_event->time_stamp());
  }
}

TEST_F(StructTraitsTest, KeyEventPropertiesSerialized) {
  KeyEvent key_event(ET_KEY_PRESSED, VKEY_T, EF_NONE);
  const std::string key = "key";
  const std::vector<uint8_t> value(0xCD, 2);
  KeyEvent::Properties properties;
  properties[key] = value;
  key_event.SetProperties(properties);

  std::unique_ptr<Event> event_ptr = Event::Clone(key_event);
  std::unique_ptr<Event> deserialized;
  ASSERT_TRUE(mojom::Event::Deserialize(mojom::Event::Serialize(&event_ptr),
                                        &deserialized));
  ASSERT_TRUE(deserialized->IsKeyEvent());
  ASSERT_TRUE(deserialized->AsKeyEvent()->properties());
  EXPECT_EQ(properties, *(deserialized->AsKeyEvent()->properties()));
}

TEST_F(StructTraitsTest, GestureEvent) {
  GestureEvent kTestData[] = {
      {10, 20, EF_NONE,
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(401),
       GestureEventDetails(ET_GESTURE_TAP)},
  };

  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  for (size_t i = 0; i < arraysize(kTestData); i++) {
    std::unique_ptr<Event> output;
    proxy->EchoEvent(Event::Clone(kTestData[i]), &output);
    EXPECT_TRUE(output->IsGestureEvent());

    const GestureEvent* output_ptr_event = output->AsGestureEvent();
    EXPECT_EQ(kTestData[i].type(), output_ptr_event->type());
    EXPECT_EQ(kTestData[i].flags(), output_ptr_event->flags());
    EXPECT_EQ(kTestData[i].location(), output_ptr_event->location());
    EXPECT_EQ(kTestData[i].root_location(), output_ptr_event->root_location());
    EXPECT_EQ(kTestData[i].details(), output_ptr_event->details());
    EXPECT_EQ(kTestData[i].unique_touch_event_id(),
              output_ptr_event->unique_touch_event_id());
    EXPECT_EQ(kTestData[i].time_stamp(), output_ptr_event->time_stamp());
  }
}

TEST_F(StructTraitsTest, ScrollEvent) {
  ScrollEvent kTestData[] = {
      {ET_SCROLL, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(501), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::NONE, ScrollEventPhase::kNone},
      {ET_SCROLL, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(501), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::NONE, ScrollEventPhase::kUpdate},
      {ET_SCROLL, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(501), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::NONE, ScrollEventPhase::kBegan},
      {ET_SCROLL, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(501), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::NONE, ScrollEventPhase::kEnd},
      {ET_SCROLL, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(501), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::BEGAN, ScrollEventPhase::kNone},
      {ET_SCROLL, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(501), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::INERTIAL_UPDATE,
       ScrollEventPhase::kNone},
      {ET_SCROLL, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(501), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::END, ScrollEventPhase::kNone},
      {ET_SCROLL_FLING_START, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(502), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::MAY_BEGIN, ScrollEventPhase::kNone},
      {ET_SCROLL_FLING_CANCEL, gfx::Point(10, 20),
       base::TimeTicks() + base::TimeDelta::FromMicroseconds(502), EF_NONE, 1,
       2, 3, 4, 5, EventMomentumPhase::END, ScrollEventPhase::kNone},
  };

  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  for (size_t i = 0; i < arraysize(kTestData); i++) {
    std::unique_ptr<Event> output;
    proxy->EchoEvent(Event::Clone(kTestData[i]), &output);
    EXPECT_TRUE(output->IsScrollEvent());

    const ScrollEvent* output_ptr_event = output->AsScrollEvent();
    EXPECT_EQ(kTestData[i].type(), output_ptr_event->type());
    EXPECT_EQ(kTestData[i].location(), output_ptr_event->location());
    EXPECT_EQ(kTestData[i].time_stamp(), output_ptr_event->time_stamp());
    EXPECT_EQ(kTestData[i].flags(), output_ptr_event->flags());
    EXPECT_EQ(kTestData[i].x_offset(), output_ptr_event->x_offset());
    EXPECT_EQ(kTestData[i].y_offset(), output_ptr_event->y_offset());
    EXPECT_EQ(kTestData[i].x_offset_ordinal(),
              output_ptr_event->x_offset_ordinal());
    EXPECT_EQ(kTestData[i].y_offset_ordinal(),
              output_ptr_event->y_offset_ordinal());
    EXPECT_EQ(kTestData[i].finger_count(), output_ptr_event->finger_count());
    EXPECT_EQ(kTestData[i].momentum_phase(),
              output_ptr_event->momentum_phase());
  }
}

}  // namespace ui
