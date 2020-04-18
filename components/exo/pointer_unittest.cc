// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/pointer.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/window_positioning_utils.h"
#include "components/exo/buffer.h"
#include "components/exo/pointer_delegate.h"
#include "components/exo/shell_surface.h"
#include "components/exo/sub_surface.h"
#include "components/exo/surface.h"
#include "components/exo/test/exo_test_base.h"
#include "components/exo/test/exo_test_helper.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/surfaces/surface.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/env.h"
#include "ui/events/event_utils.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/widget/widget.h"

namespace exo {
namespace {

using PointerTest = test::ExoTestBase;

class MockPointerDelegate : public PointerDelegate {
 public:
  MockPointerDelegate() {}

  // Overridden from PointerDelegate:
  MOCK_METHOD1(OnPointerDestroying, void(Pointer*));
  MOCK_CONST_METHOD1(CanAcceptPointerEventsForSurface, bool(Surface*));
  MOCK_METHOD3(OnPointerEnter, void(Surface*, const gfx::PointF&, int));
  MOCK_METHOD1(OnPointerLeave, void(Surface*));
  MOCK_METHOD2(OnPointerMotion, void(base::TimeTicks, const gfx::PointF&));
  MOCK_METHOD3(OnPointerButton, void(base::TimeTicks, int, bool));
  MOCK_METHOD3(OnPointerScroll,
               void(base::TimeTicks, const gfx::Vector2dF&, bool));
  MOCK_METHOD1(OnPointerScrollStop, void(base::TimeTicks));
  MOCK_METHOD0(OnPointerFrame, void());
};

TEST_F(PointerTest, SetCursor) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(1);
  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  std::unique_ptr<Surface> pointer_surface(new Surface);
  std::unique_ptr<Buffer> pointer_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  pointer_surface->Attach(pointer_buffer.get());
  pointer_surface->Commit();

  // Set pointer surface.
  pointer->SetCursor(pointer_surface.get(), gfx::Point(5, 5));
  RunAllPendingInMessageLoop();

  const viz::RenderPass* last_render_pass;
  {
    viz::SurfaceId surface_id = pointer->host_window()->GetSurfaceId();
    viz::SurfaceManager* surface_manager = aura::Env::GetInstance()
                                               ->context_factory_private()
                                               ->GetFrameSinkManager()
                                               ->surface_manager();
    ASSERT_TRUE(surface_manager->GetSurfaceForId(surface_id)->HasActiveFrame());
    const viz::CompositorFrame& frame =
        surface_manager->GetSurfaceForId(surface_id)->GetActiveFrame();
    EXPECT_EQ(gfx::Rect(0, 0, 10, 10),
              frame.render_pass_list.back()->output_rect);
    last_render_pass = frame.render_pass_list.back().get();
  }

  // Adjust hotspot.
  pointer->SetCursor(pointer_surface.get(), gfx::Point());
  RunAllPendingInMessageLoop();

  // Verify that adjustment to hotspot resulted in new frame.
  {
    viz::SurfaceId surface_id = pointer->host_window()->GetSurfaceId();
    viz::SurfaceManager* surface_manager = aura::Env::GetInstance()
                                               ->context_factory_private()
                                               ->GetFrameSinkManager()
                                               ->surface_manager();
    ASSERT_TRUE(surface_manager->GetSurfaceForId(surface_id)->HasActiveFrame());
    const viz::CompositorFrame& frame =
        surface_manager->GetSurfaceForId(surface_id)->GetActiveFrame();
    EXPECT_TRUE(frame.render_pass_list.back().get() != last_render_pass);
  }

  // Unset pointer surface.
  pointer->SetCursor(nullptr, gfx::Point());

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, SetCursorNull) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(1);
  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  pointer->SetCursor(nullptr, gfx::Point());
  RunAllPendingInMessageLoop();

  EXPECT_EQ(nullptr, pointer->root_surface());
  aura::client::CursorClient* cursor_client = aura::client::GetCursorClient(
      shell_surface->GetWidget()->GetNativeWindow()->GetRootWindow());
  EXPECT_EQ(ui::CursorType::kNone, cursor_client->GetCursor().native_type());

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, SetCursorType) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(1);
  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  pointer->SetCursorType(ui::CursorType::kIBeam);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(nullptr, pointer->root_surface());
  aura::client::CursorClient* cursor_client = aura::client::GetCursorClient(
      shell_surface->GetWidget()->GetNativeWindow()->GetRootWindow());
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_client->GetCursor().native_type());

  // Set the pointer with surface after setting pointer type.
  std::unique_ptr<Surface> pointer_surface(new Surface);
  std::unique_ptr<Buffer> pointer_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  pointer_surface->Attach(pointer_buffer.get());
  pointer_surface->Commit();

  pointer->SetCursor(pointer_surface.get(), gfx::Point());
  RunAllPendingInMessageLoop();

  {
    viz::SurfaceId surface_id = pointer->host_window()->GetSurfaceId();
    viz::SurfaceManager* surface_manager = aura::Env::GetInstance()
                                               ->context_factory_private()
                                               ->GetFrameSinkManager()
                                               ->surface_manager();
    ASSERT_TRUE(surface_manager->GetSurfaceForId(surface_id)->HasActiveFrame());
    const viz::CompositorFrame& frame =
        surface_manager->GetSurfaceForId(surface_id)->GetActiveFrame();
    EXPECT_EQ(gfx::Rect(0, 0, 10, 10),
              frame.render_pass_list.back()->output_rect);
  }

  // Set the pointer type after the pointer surface is specified.
  pointer->SetCursorType(ui::CursorType::kCross);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(nullptr, pointer->root_surface());
  EXPECT_EQ(ui::CursorType::kCross, cursor_client->GetCursor().native_type());

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, SetCursorAndSetCursorType) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(1);
  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  std::unique_ptr<Surface> pointer_surface(new Surface);
  std::unique_ptr<Buffer> pointer_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  pointer_surface->Attach(pointer_buffer.get());
  pointer_surface->Commit();

  // Set pointer surface.
  pointer->SetCursor(pointer_surface.get(), gfx::Point());
  RunAllPendingInMessageLoop();

  {
    viz::SurfaceId surface_id = pointer->host_window()->GetSurfaceId();
    viz::SurfaceManager* surface_manager = aura::Env::GetInstance()
                                               ->context_factory_private()
                                               ->GetFrameSinkManager()
                                               ->surface_manager();
    ASSERT_TRUE(surface_manager->GetSurfaceForId(surface_id)->HasActiveFrame());
    const viz::CompositorFrame& frame =
        surface_manager->GetSurfaceForId(surface_id)->GetActiveFrame();
    EXPECT_EQ(gfx::Rect(0, 0, 10, 10),
              frame.render_pass_list.back()->output_rect);
  }

  // Set the cursor type to the kNone through SetCursorType.
  pointer->SetCursorType(ui::CursorType::kNone);
  RunAllPendingInMessageLoop();
  EXPECT_EQ(nullptr, pointer->root_surface());

  // Set the same pointer surface again.
  pointer->SetCursor(pointer_surface.get(), gfx::Point());
  RunAllPendingInMessageLoop();

  {
    viz::SurfaceId surface_id = pointer->host_window()->GetSurfaceId();
    viz::SurfaceManager* surface_manager = aura::Env::GetInstance()
                                               ->context_factory_private()
                                               ->GetFrameSinkManager()
                                               ->surface_manager();
    ASSERT_TRUE(surface_manager->GetSurfaceForId(surface_id)->HasActiveFrame());
    const viz::CompositorFrame& frame =
        surface_manager->GetSurfaceForId(surface_id)->GetActiveFrame();
    EXPECT_EQ(gfx::Rect(0, 0, 10, 10),
              frame.render_pass_list.back()->output_rect);
  }

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, SetCursorNullAndSetCursorType) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(1);
  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  // Set nullptr surface.
  pointer->SetCursor(nullptr, gfx::Point());
  RunAllPendingInMessageLoop();

  EXPECT_EQ(nullptr, pointer->root_surface());
  aura::client::CursorClient* cursor_client = aura::client::GetCursorClient(
      shell_surface->GetWidget()->GetNativeWindow()->GetRootWindow());
  EXPECT_EQ(ui::CursorType::kNone, cursor_client->GetCursor().native_type());

  // Set the cursor type.
  pointer->SetCursorType(ui::CursorType::kIBeam);
  RunAllPendingInMessageLoop();
  EXPECT_EQ(nullptr, pointer->root_surface());
  EXPECT_EQ(ui::CursorType::kIBeam, cursor_client->GetCursor().native_type());

  // Set nullptr surface again.
  pointer->SetCursor(nullptr, gfx::Point());
  RunAllPendingInMessageLoop();
  EXPECT_EQ(nullptr, pointer->root_surface());
  EXPECT_EQ(ui::CursorType::kNone, cursor_client->GetCursor().native_type());

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, OnPointerEnter) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(1);
  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, OnPointerLeave) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(4);
  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  EXPECT_CALL(delegate, OnPointerLeave(surface.get()));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().bottom_right());

  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  EXPECT_CALL(delegate, OnPointerLeave(surface.get()));
  shell_surface.reset();
  surface.reset();

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, OnPointerMotion) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(6);

  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  EXPECT_CALL(delegate, OnPointerMotion(testing::_, gfx::PointF(1, 1)));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin() +
                        gfx::Vector2d(1, 1));

  std::unique_ptr<Surface> sub_surface(new Surface);
  std::unique_ptr<SubSurface> sub(
      new SubSurface(sub_surface.get(), surface.get()));
  surface->SetSubSurfacePosition(sub_surface.get(), gfx::Point(5, 5));
  gfx::Size sub_buffer_size(5, 5);
  std::unique_ptr<Buffer> sub_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(sub_buffer_size)));
  sub_surface->Attach(sub_buffer.get());
  sub_surface->Commit();
  surface->Commit();

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(sub_surface.get()))
      .WillRepeatedly(testing::Return(true));

  EXPECT_CALL(delegate, OnPointerLeave(surface.get()));
  EXPECT_CALL(delegate, OnPointerEnter(sub_surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(sub_surface->window()->GetBoundsInScreen().origin());

  EXPECT_CALL(delegate, OnPointerMotion(testing::_, gfx::PointF(1, 1)));
  generator.MoveMouseTo(sub_surface->window()->GetBoundsInScreen().origin() +
                        gfx::Vector2d(1, 1));

  std::unique_ptr<Surface> child_surface(new Surface);
  std::unique_ptr<ShellSurface> child_shell_surface(
      new ShellSurface(child_surface.get(), gfx::Point(9, 9), true, false,
                       ash::kShellWindowId_DefaultContainer));
  child_shell_surface->DisableMovement();
  child_shell_surface->SetParent(shell_surface.get());
  gfx::Size child_buffer_size(15, 15);
  std::unique_ptr<Buffer> child_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(child_buffer_size)));
  child_surface->Attach(child_buffer.get());
  child_surface->Commit();

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(child_surface.get()))
      .WillRepeatedly(testing::Return(true));

  EXPECT_CALL(delegate, OnPointerLeave(sub_surface.get()));
  EXPECT_CALL(delegate, OnPointerEnter(child_surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(child_surface->window()->GetBoundsInScreen().origin());

  EXPECT_CALL(delegate, OnPointerMotion(testing::_, gfx::PointF(10, 10)));
  generator.MoveMouseTo(child_surface->window()->GetBoundsInScreen().origin() +
                        gfx::Vector2d(10, 10));

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, OnPointerButton) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(3);

  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  EXPECT_CALL(delegate,
              OnPointerButton(testing::_, ui::EF_LEFT_MOUSE_BUTTON, true));
  EXPECT_CALL(delegate,
              OnPointerButton(testing::_, ui::EF_LEFT_MOUSE_BUTTON, false));
  generator.ClickLeftButton();

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, OnPointerScroll) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());
  gfx::Point location = surface->window()->GetBoundsInScreen().origin();

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(3);

  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(location);

  {
    // Expect fling stop followed by scroll and scroll stop.
    testing::InSequence sequence;

    EXPECT_CALL(delegate,
                OnPointerScroll(testing::_, gfx::Vector2dF(1.2, 1.2), false));
    EXPECT_CALL(delegate, OnPointerScrollStop(testing::_));
  }
  generator.ScrollSequence(location, base::TimeDelta(), 1, 1, 1, 1);

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, OnPointerScrollDiscrete) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  gfx::Size buffer_size(10, 10);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, OnPointerFrame()).Times(2);

  EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().origin());

  EXPECT_CALL(delegate,
              OnPointerScroll(testing::_, gfx::Vector2dF(1, 1), true));
  generator.MoveMouseWheel(1, 1);

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

TEST_F(PointerTest, IgnorePointerEventDuringModal) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(gfx::Size(10, 10))));
  surface->Attach(buffer.get());
  surface->Commit();
  gfx::Point location = surface->window()->GetBoundsInScreen().origin();

  MockPointerDelegate delegate;
  std::unique_ptr<Pointer> pointer(new Pointer(&delegate));
  ui::test::EventGenerator generator(ash::Shell::GetPrimaryRootWindow());

  // Create surface for modal window.
  std::unique_ptr<Surface> surface2(new Surface);
  std::unique_ptr<ShellSurface> shell_surface2(
      new ShellSurface(surface2.get(), gfx::Point(), true, false,
                       ash::kShellWindowId_SystemModalContainer));
  shell_surface2->DisableMovement();
  std::unique_ptr<Buffer> buffer2(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(gfx::Size(5, 5))));
  surface2->Attach(buffer2.get());
  surface2->Commit();
  ash::wm::CenterWindow(surface2->window());
  gfx::Point location2 = surface2->window()->GetBoundsInScreen().origin();

  // Make the window modal.
  shell_surface2->SetSystemModal(true);
  EXPECT_TRUE(ash::Shell::IsSystemModalWindowOpen());

  EXPECT_CALL(delegate, OnPointerFrame()).Times(testing::AnyNumber());
  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface.get()))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(delegate, CanAcceptPointerEventsForSurface(surface2.get()))
      .WillRepeatedly(testing::Return(true));

  // Check if pointer events on modal window are registered.
  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerEnter(surface2.get(), gfx::PointF(), 0));
  }
  generator.MoveMouseTo(location2);

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerMotion(testing::_, gfx::PointF(1, 1)));
  }
  generator.MoveMouseTo(location2 + gfx::Vector2d(1, 1));

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate,
                OnPointerButton(testing::_, ui::EF_LEFT_MOUSE_BUTTON, true));
    EXPECT_CALL(delegate,
                OnPointerButton(testing::_, ui::EF_LEFT_MOUSE_BUTTON, false));
  }
  generator.ClickLeftButton();

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate,
                OnPointerScroll(testing::_, gfx::Vector2dF(1.2, 1.2), false));
    EXPECT_CALL(delegate, OnPointerScrollStop(testing::_));
  }
  generator.ScrollSequence(location2, base::TimeDelta(), 1, 1, 1, 1);

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerLeave(surface2.get()));
  }
  generator.MoveMouseTo(surface2->window()->GetBoundsInScreen().bottom_right());

  // Check if pointer events on non-modal window are ignored.
  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0))
        .Times(0);
  }
  generator.MoveMouseTo(location);

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerMotion(testing::_, gfx::PointF(1, 1)))
        .Times(0);
  }
  generator.MoveMouseTo(location + gfx::Vector2d(1, 1));

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate,
                OnPointerButton(testing::_, ui::EF_LEFT_MOUSE_BUTTON, true))
        .Times(0);
    EXPECT_CALL(delegate,
                OnPointerButton(testing::_, ui::EF_LEFT_MOUSE_BUTTON, false))
        .Times(0);
  }
  generator.ClickLeftButton();

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate,
                OnPointerScroll(testing::_, gfx::Vector2dF(1.2, 1.2), false))
        .Times(0);
    EXPECT_CALL(delegate, OnPointerScrollStop(testing::_)).Times(0);
  }
  generator.ScrollSequence(location, base::TimeDelta(), 1, 1, 1, 1);

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerLeave(surface.get())).Times(0);
  }
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().bottom_right());

  // Make the window non-modal.
  shell_surface2->SetSystemModal(false);
  EXPECT_FALSE(ash::Shell::IsSystemModalWindowOpen());

  // Check if pointer events on non-modal window are registered.
  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerEnter(surface.get(), gfx::PointF(), 0));
  }
  generator.MoveMouseTo(location);

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerMotion(testing::_, gfx::PointF(1, 1)));
  }
  generator.MoveMouseTo(location + gfx::Vector2d(1, 1));

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate,
                OnPointerButton(testing::_, ui::EF_LEFT_MOUSE_BUTTON, true));
    EXPECT_CALL(delegate,
                OnPointerButton(testing::_, ui::EF_LEFT_MOUSE_BUTTON, false));
  }
  generator.ClickLeftButton();

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate,
                OnPointerScroll(testing::_, gfx::Vector2dF(1.2, 1.2), false));
    EXPECT_CALL(delegate, OnPointerScrollStop(testing::_));
  }
  generator.ScrollSequence(location, base::TimeDelta(), 1, 1, 1, 1);

  {
    testing::InSequence sequence;
    EXPECT_CALL(delegate, OnPointerLeave(surface.get()));
  }
  generator.MoveMouseTo(surface->window()->GetBoundsInScreen().bottom_right());

  EXPECT_CALL(delegate, OnPointerDestroying(pointer.get()));
  pointer.reset();
}

}  // namespace
}  // namespace exo
