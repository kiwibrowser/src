// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/event_monitor.h"

#include "base/macros.h"
#include "ui/events/test/event_generator.h"
#include "ui/events/test/test_event_handler.h"
#include "ui/views/test/widget_test.h"

namespace views {
namespace test {

class EventMonitorTest : public WidgetTest {
 public:
  EventMonitorTest() : widget_(nullptr) {}

  // testing::Test:
  void SetUp() override {
    WidgetTest::SetUp();
    widget_ = CreateTopLevelNativeWidget();
    widget_->SetSize(gfx::Size(100, 100));
    widget_->Show();
    if (IsMus()) {
      generator_.reset(
          new ui::test::EventGenerator(widget_->GetNativeWindow()));
    } else {
      generator_.reset(new ui::test::EventGenerator(
          GetContext(), widget_->GetNativeWindow()));
    }
    generator_->set_target(ui::test::EventGenerator::Target::APPLICATION);
  }
  void TearDown() override {
    widget_->CloseNow();
    WidgetTest::TearDown();
  }

 protected:
  Widget* widget_;
  std::unique_ptr<ui::test::EventGenerator> generator_;
  ui::test::TestEventHandler handler_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EventMonitorTest);
};

TEST_F(EventMonitorTest, ShouldReceiveAppEventsWhileInstalled) {
  std::unique_ptr<EventMonitor> monitor(
      EventMonitor::CreateApplicationMonitor(&handler_));

  generator_->ClickLeftButton();
  EXPECT_EQ(2, handler_.num_mouse_events());

  monitor.reset();
  generator_->ClickLeftButton();
  EXPECT_EQ(2, handler_.num_mouse_events());
}

TEST_F(EventMonitorTest, ShouldReceiveWindowEventsWhileInstalled) {
  std::unique_ptr<EventMonitor> monitor(
      EventMonitor::CreateWindowMonitor(&handler_, widget_->GetNativeWindow()));

  generator_->ClickLeftButton();
  EXPECT_EQ(2, handler_.num_mouse_events());

  monitor.reset();
  generator_->ClickLeftButton();
  EXPECT_EQ(2, handler_.num_mouse_events());
}

TEST_F(EventMonitorTest, ShouldNotReceiveEventsFromOtherWindow) {
  Widget* widget2 = CreateTopLevelNativeWidget();
  std::unique_ptr<EventMonitor> monitor(
      EventMonitor::CreateWindowMonitor(&handler_, widget2->GetNativeWindow()));

  generator_->ClickLeftButton();
  EXPECT_EQ(0, handler_.num_mouse_events());

  monitor.reset();
  widget2->CloseNow();
}

namespace {
class DeleteOtherOnEventHandler : public ui::EventHandler {
 public:
  DeleteOtherOnEventHandler() {
    monitor_ = EventMonitor::CreateApplicationMonitor(this);
  }

  bool DidDelete() const { return !handler_to_delete_; }

  void set_monitor_to_delete(
      std::unique_ptr<DeleteOtherOnEventHandler> handler_to_delete) {
    handler_to_delete_ = std::move(handler_to_delete);
  }

  // EventHandler::
  void OnMouseEvent(ui::MouseEvent* event) override {
    handler_to_delete_ = nullptr;
  }

 private:
  std::unique_ptr<EventMonitor> monitor_;
  std::unique_ptr<DeleteOtherOnEventHandler> handler_to_delete_;

  DISALLOW_COPY_AND_ASSIGN(DeleteOtherOnEventHandler);
};
}  // namespace

// Ensure correct behavior when an event monitor is removed while iterating
// over the OS-controlled observer list.
TEST_F(EventMonitorTest, TwoMonitors) {
  auto deleter = std::make_unique<DeleteOtherOnEventHandler>();
  deleter->set_monitor_to_delete(std::make_unique<DeleteOtherOnEventHandler>());

  EXPECT_FALSE(deleter->DidDelete());
  generator_->PressLeftButton();
  EXPECT_TRUE(deleter->DidDelete());

  // Now try setting up observers in the alternate order.
  auto deletee = std::make_unique<DeleteOtherOnEventHandler>();
  deleter = std::make_unique<DeleteOtherOnEventHandler>();
  deleter->set_monitor_to_delete(std::move(deletee));

  EXPECT_FALSE(deleter->DidDelete());
  generator_->ReleaseLeftButton();
  EXPECT_TRUE(deleter->DidDelete());
}

}  // namespace test
}  // namespace views
