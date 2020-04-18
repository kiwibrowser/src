// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/resource_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

TEST(ResourceBufferTest, BasicAllocations) {
  scoped_refptr<ResourceBuffer> buf = new ResourceBuffer();
  EXPECT_TRUE(buf->Initialize(100, 5, 10));
  EXPECT_TRUE(buf->CanAllocate());

  // First allocation
  {
    int size;
    char* ptr = buf->Allocate(&size);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(10, size);
    EXPECT_TRUE(buf->CanAllocate());

    EXPECT_EQ(0, buf->GetLastAllocationOffset());

    buf->ShrinkLastAllocation(2);  // Less than our min allocation size.
    EXPECT_EQ(0, buf->GetLastAllocationOffset());
    EXPECT_TRUE(buf->CanAllocate());
  }

  // Second allocation
  {
    int size;
    char* ptr = buf->Allocate(&size);
    EXPECT_TRUE(ptr);
    EXPECT_EQ(10, size);
    EXPECT_TRUE(buf->CanAllocate());

    EXPECT_EQ(5, buf->GetLastAllocationOffset());

    buf->ShrinkLastAllocation(4);
    EXPECT_EQ(5, buf->GetLastAllocationOffset());

    EXPECT_TRUE(buf->CanAllocate());
  }
}

TEST(ResourceBufferTest, AllocateAndRecycle) {
  scoped_refptr<ResourceBuffer> buf = new ResourceBuffer();
  EXPECT_TRUE(buf->Initialize(100, 5, 10));

  int size;

  buf->Allocate(&size);
  EXPECT_EQ(0, buf->GetLastAllocationOffset());

  buf->RecycleLeastRecentlyAllocated();

  // Offset should again be 0.
  buf->Allocate(&size);
  EXPECT_EQ(0, buf->GetLastAllocationOffset());
}

TEST(ResourceBufferTest, WrapAround) {
  scoped_refptr<ResourceBuffer> buf = new ResourceBuffer();
  EXPECT_TRUE(buf->Initialize(20, 10, 10));

  int size;

  buf->Allocate(&size);
  EXPECT_EQ(10, size);

  buf->Allocate(&size);
  EXPECT_EQ(10, size);

  // Create hole at the beginnning.  Next allocation should go there.
  buf->RecycleLeastRecentlyAllocated();

  buf->Allocate(&size);
  EXPECT_EQ(10, size);

  EXPECT_EQ(0, buf->GetLastAllocationOffset());
}

TEST(ResourceBufferTest, WrapAround2) {
  scoped_refptr<ResourceBuffer> buf = new ResourceBuffer();
  EXPECT_TRUE(buf->Initialize(30, 10, 10));

  int size;

  buf->Allocate(&size);
  EXPECT_EQ(10, size);

  buf->Allocate(&size);
  EXPECT_EQ(10, size);

  buf->Allocate(&size);
  EXPECT_EQ(10, size);

  EXPECT_FALSE(buf->CanAllocate());

  // Create holes at first and second slots.
  buf->RecycleLeastRecentlyAllocated();
  buf->RecycleLeastRecentlyAllocated();

  EXPECT_TRUE(buf->CanAllocate());

  buf->Allocate(&size);
  EXPECT_EQ(10, size);
  EXPECT_EQ(0, buf->GetLastAllocationOffset());

  buf->Allocate(&size);
  EXPECT_EQ(10, size);
  EXPECT_EQ(10, buf->GetLastAllocationOffset());

  EXPECT_FALSE(buf->CanAllocate());
}

TEST(ResourceBufferTest, Full) {
  scoped_refptr<ResourceBuffer> buf = new ResourceBuffer();
  EXPECT_TRUE(buf->Initialize(20, 10, 10));

  int size;
  buf->Allocate(&size);
  EXPECT_EQ(10, size);

  buf->Allocate(&size);
  EXPECT_EQ(10, size);

  // Full.
  EXPECT_FALSE(buf->CanAllocate());

  // Still full, even if there is a small hole at the end.
  buf->ShrinkLastAllocation(5);
  EXPECT_FALSE(buf->CanAllocate());
}

}  // namespace content
