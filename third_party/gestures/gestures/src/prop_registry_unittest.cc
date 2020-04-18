// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <gtest/gtest.h>

#include "gestures/include/activity_log.h"
#include "gestures/include/prop_registry.h"

using std::string;

// Mock struct GesturesProp implementation (outside of namespace gestures)
struct GesturesProp { };

namespace gestures {

class PropRegistryTest : public ::testing::Test {};

class PropRegistryTestDelegate : public PropertyDelegate {
 public:
  PropRegistryTestDelegate() : call_cnt_(0) {}
  virtual void BoolWasWritten(BoolProperty* prop) { call_cnt_++; };
  virtual void DoubleWasWritten(DoubleProperty* prop) { call_cnt_++; };
  virtual void IntWasWritten(IntProperty* prop) { call_cnt_++; }
  virtual void ShortWasWritten(ShortProperty* prop) { call_cnt_++; }
  virtual void StringWasWritten(StringProperty* prop) { call_cnt_++; }

  int call_cnt_;
};

namespace {
string ValueForProperty(const Property& prop) {
  Json::Value temp(Json::objectValue);
  temp["tempkey"] = prop.NewValue();
  return temp.toStyledString();
}

}  // namespace {}

TEST(PropRegistryTest, SimpleTest) {
  PropRegistry reg;
  PropRegistryTestDelegate delegate;

  int expected_call_cnt = 0;
  BoolProperty bp1(&reg, "hi", false, &delegate);
  EXPECT_TRUE(strstr(ValueForProperty(bp1).c_str(), "false"));
  bp1.HandleGesturesPropWritten();
  EXPECT_EQ(++expected_call_cnt, delegate.call_cnt_);

  BoolProperty bp2(&reg, "hi", true);
  EXPECT_TRUE(strstr(ValueForProperty(bp2).c_str(), "true"));
  bp2.HandleGesturesPropWritten();
  EXPECT_EQ(expected_call_cnt, delegate.call_cnt_);

  DoubleProperty dp1(&reg, "hi", 2721.0, &delegate);
  EXPECT_TRUE(strstr(ValueForProperty(dp1).c_str(), "2721"));
  dp1.HandleGesturesPropWritten();
  EXPECT_EQ(++expected_call_cnt, delegate.call_cnt_);

  DoubleProperty dp2(&reg, "hi", 3.1);
  EXPECT_TRUE(strstr(ValueForProperty(dp2).c_str(), "3.1"));
  dp2.HandleGesturesPropWritten();
  EXPECT_EQ(expected_call_cnt, delegate.call_cnt_);

  IntProperty ip1(&reg, "hi", 567, &delegate);
  EXPECT_TRUE(strstr(ValueForProperty(ip1).c_str(), "567"));
  ip1.HandleGesturesPropWritten();
  EXPECT_EQ(++expected_call_cnt, delegate.call_cnt_);

  IntProperty ip2(&reg, "hi", 568);
  EXPECT_TRUE(strstr(ValueForProperty(ip2).c_str(), "568"));
  ip2.HandleGesturesPropWritten();
  EXPECT_EQ(expected_call_cnt, delegate.call_cnt_);

  ShortProperty sp1(&reg, "hi", 234, &delegate);
  EXPECT_TRUE(strstr(ValueForProperty(sp1).c_str(), "234"));
  sp1.HandleGesturesPropWritten();
  EXPECT_EQ(++expected_call_cnt, delegate.call_cnt_);

  ShortProperty sp2(&reg, "hi", 235);
  EXPECT_TRUE(strstr(ValueForProperty(sp2).c_str(), "235"));
  sp2.HandleGesturesPropWritten();
  EXPECT_EQ(expected_call_cnt, delegate.call_cnt_);

  StringProperty stp1(&reg, "hi", "foo", &delegate);
  EXPECT_TRUE(strstr(ValueForProperty(stp1).c_str(), "foo"));
  stp1.HandleGesturesPropWritten();
  EXPECT_EQ(++expected_call_cnt, delegate.call_cnt_);

  StringProperty stp2(&reg, "hi", "bar");
  EXPECT_TRUE(strstr(ValueForProperty(stp2).c_str(), "bar"));
  stp2.HandleGesturesPropWritten();
  EXPECT_EQ(expected_call_cnt, delegate.call_cnt_);
}

TEST(PropRegistryTest, PropChangeTest) {
  PropRegistry reg;
  ActivityLog log(&reg);
  reg.set_activity_log(&log);

  DoubleProperty dp(&reg, "hi", 1234.0, NULL);
  EXPECT_EQ(0, log.size());
  dp.HandleGesturesPropWritten();
  EXPECT_EQ(1, log.size());
}

// Mock GesturesPropProvider
GesturesProp* MockGesturesPropCreateBool(void* data, const char* name,
                                         GesturesPropBool* loc,
                                         size_t count,
                                         const GesturesPropBool* init) {
  GesturesProp *dummy = new GesturesProp();
  *loc = true;
  return dummy;
}

GesturesProp* MockGesturesPropCreateInt(void* data, const char* name,
                                        int* loc, size_t count,
                                        const int* init) {
  GesturesProp *dummy = new GesturesProp();
  *loc = 1;
  return dummy;
}

GesturesProp* MockGesturesPropCreateReal(void* data, const char* name,
                                         double* loc, size_t count,
                                         const double* init) {
  GesturesProp *dummy = new GesturesProp();
  *loc = 1.0;
  return dummy;
}

GesturesProp* MockGesturesPropCreateShort(void* data, const char* name,
                                          short* loc, size_t count,
                                          const short* init) {
  GesturesProp *dummy = new GesturesProp();
  *loc = 1;
  return dummy;
}

GesturesProp* MockGesturesPropCreateString(void* data, const char* name,
                                           const char** loc,
                                           const char* const init) {
  GesturesProp *dummy = new GesturesProp();
  *loc = "1";
  return dummy;
}

void MockGesturesPropRegisterHandlers(void* data, GesturesProp* prop,
                                      void* handler_data,
                                      GesturesPropGetHandler getter,
                                      GesturesPropSetHandler setter) {}

void MockGesturesPropFree(void* data, GesturesProp* prop) {
  delete prop;
}

// This tests that if we create a prop, then set the prop provider, and the
// prop provider changes the value at that time, that we notify the prop
// delegate that the value was changed.
TEST(PropRegistryTest, SetAtCreateShouldNotifyTest) {
  GesturesPropProvider mock_gestures_props_provider = {
    MockGesturesPropCreateInt,
    MockGesturesPropCreateShort,
    MockGesturesPropCreateBool,
    MockGesturesPropCreateString,
    MockGesturesPropCreateReal,
    MockGesturesPropRegisterHandlers,
    MockGesturesPropFree
  };

  PropRegistry reg;
  PropRegistryTestDelegate delegate;
  BoolProperty my_bool(&reg, "MyBool", 0, &delegate);
  DoubleProperty my_double(&reg, "MyDouble", 0.0, &delegate);
  IntProperty my_int(&reg, "MyInt", 0, &delegate);
  ShortProperty my_short(&reg, "MyShort", 0, &delegate);
  ShortProperty my_short_no_change(&reg, "MyShortNoChange", 1, &delegate);
  EXPECT_EQ(0, delegate.call_cnt_);

  reg.SetPropProvider(&mock_gestures_props_provider, NULL);
  EXPECT_EQ(4, delegate.call_cnt_);
}

TEST(PropRegistryTest, DoublePromoteIntTest) {
  PropRegistry reg;
  PropRegistryTestDelegate delegate;

  DoubleProperty my_double(&reg, "MyDouble", 1234.5, &delegate);
  EXPECT_TRUE(strstr(ValueForProperty(my_double).c_str(), "1234.5"));
  IntProperty my_int(&reg, "MyInt", 321, &delegate);
  Json::Value my_int_val = my_int.NewValue();
  EXPECT_TRUE(my_double.SetValue(my_int_val));
  EXPECT_TRUE(strstr(ValueForProperty(my_double).c_str(), "321"));
}
}  // namespace gestures
