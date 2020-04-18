// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/class_property.h"

#include <limits.h>

#include <string>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

DEFINE_UI_CLASS_PROPERTY_TYPE(const char*)
DEFINE_UI_CLASS_PROPERTY_TYPE(int)

namespace {

class TestProperty {
 public:
  TestProperty() {}
  ~TestProperty() {
    last_deleted_ = this;
  }
  static void* last_deleted() { return last_deleted_; }

 private:
  static void* last_deleted_;
  DISALLOW_COPY_AND_ASSIGN(TestProperty);
};

void* TestProperty::last_deleted_ = nullptr;

DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(TestProperty, kOwnedKey, NULL);

}  // namespace

DEFINE_UI_CLASS_PROPERTY_TYPE(TestProperty*);

namespace ui {
namespace test {

namespace {

const int kDefaultIntValue = -2;
const char* kDefaultStringValue = "squeamish";
const char* kTestStringValue = "ossifrage";

DEFINE_UI_CLASS_PROPERTY_KEY(int, kIntKey, kDefaultIntValue);
DEFINE_UI_CLASS_PROPERTY_KEY(const char*, kStringKey, kDefaultStringValue);
}

TEST(PropertyTest, Property) {
  PropertyHandler h;

  // Non-existent properties should return the default values.
  EXPECT_EQ(kDefaultIntValue, h.GetProperty(kIntKey));
  EXPECT_EQ(std::string(kDefaultStringValue), h.GetProperty(kStringKey));

  // A set property value should be returned again (even if it's the default
  // value).
  h.SetProperty(kIntKey, INT_MAX);
  EXPECT_EQ(INT_MAX, h.GetProperty(kIntKey));
  h.SetProperty(kIntKey, kDefaultIntValue);
  EXPECT_EQ(kDefaultIntValue, h.GetProperty(kIntKey));
  h.SetProperty(kIntKey, INT_MIN);
  EXPECT_EQ(INT_MIN, h.GetProperty(kIntKey));

  h.SetProperty<const char*>(kStringKey, nullptr);
  EXPECT_EQ(NULL, h.GetProperty(kStringKey));
  h.SetProperty(kStringKey, kDefaultStringValue);
  EXPECT_EQ(std::string(kDefaultStringValue), h.GetProperty(kStringKey));
  h.SetProperty(kStringKey, kTestStringValue);
  EXPECT_EQ(std::string(kTestStringValue), h.GetProperty(kStringKey));

  // ClearProperty should restore the default value.
  h.ClearProperty(kIntKey);
  EXPECT_EQ(kDefaultIntValue, h.GetProperty(kIntKey));
  h.ClearProperty(kStringKey);
  EXPECT_EQ(std::string(kDefaultStringValue), h.GetProperty(kStringKey));
}

TEST(PropertyTest, OwnedProperty) {
  TestProperty* p3;
  {
    PropertyHandler h;

    EXPECT_EQ(NULL, h.GetProperty(kOwnedKey));
    void* last_deleted = TestProperty::last_deleted();
    TestProperty* p1 = new TestProperty();
    h.SetProperty(kOwnedKey, p1);
    EXPECT_EQ(p1, h.GetProperty(kOwnedKey));
    EXPECT_EQ(last_deleted, TestProperty::last_deleted());

    TestProperty* p2 = new TestProperty();
    h.SetProperty(kOwnedKey, p2);
    EXPECT_EQ(p2, h.GetProperty(kOwnedKey));
    EXPECT_EQ(p1, TestProperty::last_deleted());

    h.ClearProperty(kOwnedKey);
    EXPECT_EQ(NULL, h.GetProperty(kOwnedKey));
    EXPECT_EQ(p2, TestProperty::last_deleted());

    p3 = new TestProperty();
    h.SetProperty(kOwnedKey, p3);
    EXPECT_EQ(p3, h.GetProperty(kOwnedKey));
    EXPECT_EQ(p2, TestProperty::last_deleted());
  }
  EXPECT_EQ(p3, TestProperty::last_deleted());
}

} // namespace test
} // namespace ui
