// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/cursor_location_manager.h"

#include "base/atomicops.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"

namespace ui {
namespace ws {
namespace test {

TEST(CursorLocationManagerTest, PositiveCoordinates) {
  const gfx::Point point(100, 150);

  CursorLocationManager cursor_location_manager;
  cursor_location_manager.OnMouseCursorLocationChanged(point);

  base::subtle::Atomic32* cursor_location_memory = nullptr;
  mojo::ScopedSharedBufferHandle handle =
      cursor_location_manager.GetCursorLocationMemory();
  mojo::ScopedSharedBufferMapping cursor_location_mapping =
      handle->Map(sizeof(base::subtle::Atomic32));
  ASSERT_TRUE(cursor_location_mapping);
  cursor_location_memory =
      reinterpret_cast<base::subtle::Atomic32*>(cursor_location_mapping.get());

  base::subtle::Atomic32 location =
      base::subtle::NoBarrier_Load(cursor_location_memory);
  EXPECT_EQ(point, Atomic32ToPoint(location));
}

TEST(CursorLocationManagerTest, NegativeCoordinates) {
  const gfx::Point point(-10, -11);

  CursorLocationManager cursor_location_manager;
  cursor_location_manager.OnMouseCursorLocationChanged(point);

  base::subtle::Atomic32* cursor_location_memory = nullptr;
  mojo::ScopedSharedBufferHandle handle =
      cursor_location_manager.GetCursorLocationMemory();
  mojo::ScopedSharedBufferMapping cursor_location_mapping =
      handle->Map(sizeof(base::subtle::Atomic32));
  ASSERT_TRUE(cursor_location_mapping);
  cursor_location_memory =
      reinterpret_cast<base::subtle::Atomic32*>(cursor_location_mapping.get());

  base::subtle::Atomic32 location =
      base::subtle::NoBarrier_Load(cursor_location_memory);
  EXPECT_EQ(point, Atomic32ToPoint(location));
}

}  // namespace test
}  // namespace ws
}  // namespace ui
