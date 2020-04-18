// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/shelf_struct_mojom_traits.h"

#include <utility>

#include "ash/public/cpp/shelf_item.h"
#include "ash/public/cpp/shelf_struct_traits_test_service.mojom.h"
#include "ash/public/interfaces/shelf.mojom.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "ui/gfx/image/mojo/image_skia_struct_traits.h"

namespace ash {
namespace {

TEST(ShelfIDStructTraitsTest, Basic) {
  ShelfID shelf_id("app_id", "launch_id");

  ShelfID out_shelf_id;
  ASSERT_TRUE(mojom::ShelfID::Deserialize(mojom::ShelfID::Serialize(&shelf_id),
                                          &out_shelf_id));

  EXPECT_EQ("app_id", out_shelf_id.app_id);
  EXPECT_EQ("launch_id", out_shelf_id.launch_id);
}

// A test class that also implements the test service to echo ShelfItem structs.
// Revisit this after Deserialize(Serialize()) API works with handles.
class ShelfItemStructTraitsTest : public testing::Test,
                                  public mojom::ShelfStructTraitsTestService {
 public:
  ShelfItemStructTraitsTest() : binding_(this) {}

  // testing::Test:
  void SetUp() override { binding_.Bind(mojo::MakeRequest(&service_)); }

  mojom::ShelfStructTraitsTestServicePtr& service() { return service_; }

 private:
  // mojom::ShelfStructTraitsTestService:
  void EchoShelfItem(const ShelfItem& in,
                     EchoShelfItemCallback callback) override {
    std::move(callback).Run(in);
  }

  base::MessageLoop loop_;
  mojo::Binding<ShelfStructTraitsTestService> binding_;
  mojom::ShelfStructTraitsTestServicePtr service_;

  DISALLOW_COPY_AND_ASSIGN(ShelfItemStructTraitsTest);
};

TEST_F(ShelfItemStructTraitsTest, BasicNullImage) {
  ShelfItem item;
  item.type = TYPE_APP;
  item.image = gfx::ImageSkia();
  item.id = ShelfID("app_id", "launch_id");
  item.status = STATUS_RUNNING;
  item.title = base::ASCIIToUTF16("title");
  item.shows_tooltip = false;
  item.pinned_by_policy = true;

  ShelfItem out_item;
  ASSERT_TRUE(mojom::ShelfItem::Deserialize(mojom::ShelfItem::Serialize(&item),
                                            &out_item));

  EXPECT_EQ(TYPE_APP, out_item.type);
  EXPECT_TRUE(gfx::test::AreImagesEqual(gfx::Image(item.image),
                                        gfx::Image(out_item.image)));
  EXPECT_EQ(STATUS_RUNNING, out_item.status);
  EXPECT_EQ(ShelfID("app_id", "launch_id"), out_item.id);
  EXPECT_EQ(base::ASCIIToUTF16("title"), out_item.title);
  EXPECT_FALSE(out_item.shows_tooltip);
  EXPECT_TRUE(out_item.pinned_by_policy);
}

TEST_F(ShelfItemStructTraitsTest, BasicValidImage) {
  ShelfItem item;
  item.type = TYPE_APP;
  item.image = gfx::test::CreateImageSkia(32, 16);
  item.id = ShelfID("app_id", "launch_id");
  item.status = STATUS_RUNNING;
  item.title = base::ASCIIToUTF16("title");
  item.shows_tooltip = false;
  item.pinned_by_policy = true;

  ShelfItem out_item;
  service()->EchoShelfItem(item, &out_item);

  EXPECT_EQ(TYPE_APP, out_item.type);
  EXPECT_TRUE(gfx::test::AreImagesEqual(gfx::Image(item.image),
                                        gfx::Image(out_item.image)));
  EXPECT_EQ(STATUS_RUNNING, out_item.status);
  EXPECT_EQ(ShelfID("app_id", "launch_id"), out_item.id);
  EXPECT_EQ(base::ASCIIToUTF16("title"), out_item.title);
  EXPECT_FALSE(out_item.shows_tooltip);
  EXPECT_TRUE(out_item.pinned_by_policy);
}

}  // namespace
}  // namespace ash
