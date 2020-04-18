// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/atomicops.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/env.h"
#include "ui/aura/test/aura_test_suite.h"
#include "ui/gfx/geometry/point.h"

namespace aura {
namespace {

gfx::Point Atomic32ToPoint(base::subtle::Atomic32 atomic) {
  return gfx::Point(static_cast<int16_t>(atomic >> 16),
                    static_cast<int16_t>(atomic & 0xFFFF));
}

}  // namespace

TEST(MouseLocationManagerTest, PositiveCoordinates) {
  test::EnvReinstaller reinstaller;
  auto env = Env::CreateInstance(Env::Mode::LOCAL);
  env->CreateMouseLocationManager();
  const gfx::Point point(100, 150);

  env->SetLastMouseLocation(point);

  base::subtle::Atomic32* cursor_location_memory = nullptr;
  mojo::ScopedSharedBufferHandle handle = env->GetLastMouseLocationMemory();
  ASSERT_TRUE(handle.is_valid());
  mojo::ScopedSharedBufferMapping cursor_location_mapping =
      handle->Map(sizeof(base::subtle::Atomic32));
  ASSERT_TRUE(cursor_location_mapping);
  cursor_location_memory =
      reinterpret_cast<base::subtle::Atomic32*>(cursor_location_mapping.get());

  base::subtle::Atomic32 location =
      base::subtle::NoBarrier_Load(cursor_location_memory);
  EXPECT_EQ(point, Atomic32ToPoint(location));
}

TEST(MouseLocationManagerTest, NegativeCoordinates) {
  test::EnvReinstaller reinstaller;
  auto env = Env::CreateInstance(Env::Mode::LOCAL);
  env->CreateMouseLocationManager();
  const gfx::Point point(-10, -11);

  env->SetLastMouseLocation(point);

  base::subtle::Atomic32* cursor_location_memory = nullptr;
  mojo::ScopedSharedBufferHandle handle = env->GetLastMouseLocationMemory();
  ASSERT_TRUE(handle.is_valid());
  mojo::ScopedSharedBufferMapping cursor_location_mapping =
      handle->Map(sizeof(base::subtle::Atomic32));
  ASSERT_TRUE(cursor_location_mapping);
  cursor_location_memory =
      reinterpret_cast<base::subtle::Atomic32*>(cursor_location_mapping.get());

  base::subtle::Atomic32 location =
      base::subtle::NoBarrier_Load(cursor_location_memory);
  EXPECT_EQ(point, Atomic32ToPoint(location));
}

}  // namespace aura
