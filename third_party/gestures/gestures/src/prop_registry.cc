// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/prop_registry.h"

#include <set>
#include <string>

#include <json/value.h>

#include "gestures/include/activity_log.h"
#include "gestures/include/gestures.h"

using std::set;
using std::string;

namespace gestures {

void PropRegistry::Register(Property* prop) {
  props_.insert(prop);
  if (prop_provider_)
    prop->CreateProp();
}

void PropRegistry::Unregister(Property* prop) {
  if (props_.erase(prop) != 1)
    Err("Unregister failed?");
  if (prop_provider_)
    prop->DestroyProp();
}

void PropRegistry::SetPropProvider(GesturesPropProvider* prop_provider,
                                   void* data) {
  if (prop_provider_ == prop_provider)
    return;
  if (prop_provider_) {
    for (std::set<Property*>::iterator it = props_.begin(), e= props_.end();
         it != e; ++it)
      (*it)->DestroyProp();
  }
  prop_provider_ = prop_provider;
  prop_provider_data_ = data;
  if (prop_provider_)
    for (std::set<Property*>::iterator it = props_.begin(), e= props_.end();
         it != e; ++it)
      (*it)->CreateProp();
}

void Property::CreateProp() {
  if (gprop_)
    Err("Property already created");
  CreatePropImpl();
  if (parent_) {
    parent_->PropProvider()->register_handlers_fn(
        parent_->PropProviderData(),
        gprop_,
        this,
        &StaticHandleGesturesPropWillRead,
        &StaticHandleGesturesPropWritten);
  }
}

void Property::DestroyProp() {
  if (!gprop_) {
    Err("gprop_ already freed!");
    return;
  }
  parent_->PropProvider()->free_fn(parent_->PropProviderData(), gprop_);
  gprop_ = NULL;
}

void BoolProperty::CreatePropImpl() {
  GesturesPropBool orig_val = val_;
  gprop_ = parent_->PropProvider()->create_bool_fn(
      parent_->PropProviderData(),
      name(),
      &val_,
      1,
      &val_);
  if (delegate_ && orig_val != val_)
    delegate_->BoolWasWritten(this);
}

Json::Value BoolProperty::NewValue() const {
  return Json::Value(val_ != 0);
}

bool BoolProperty::SetValue(const Json::Value& value) {
  if (value.type() != Json::booleanValue) {
    return false;
  }
  val_ = value.asBool();
  return true;
}

void BoolProperty::HandleGesturesPropWritten() {
  if (parent_ && parent_->activity_log()) {
    ActivityLog::PropChangeEntry entry = {
      name(), ActivityLog::PropChangeEntry::kBoolProp, { 0 }
    };
    entry.value.bool_val = val_;
    parent_->activity_log()->LogPropChange(entry);
  }
  if (delegate_)
    delegate_->BoolWasWritten(this);
}

void BoolArrayProperty::CreatePropImpl() {
  GesturesPropBool orig_vals[count_];
  memcpy(orig_vals, vals_, sizeof(orig_vals));
  gprop_ = parent_->PropProvider()->create_bool_fn(
      parent_->PropProviderData(),
      name(),
      vals_,
      count_,
      vals_);
  if (delegate_ && memcmp(orig_vals, vals_, sizeof(orig_vals)))
    delegate_->BoolArrayWasWritten(this);
}

Json::Value BoolArrayProperty::NewValue() const {
  Json::Value list(Json::arrayValue);
  for (size_t i = 0; i < count_; i++)
    list.append(new Json::Value(vals_[i] != 0));
  return list;
}

bool BoolArrayProperty::SetValue(const Json::Value& list) {
  AssertWithReturnValue(list.type() == Json::arrayValue, false);
  AssertWithReturnValue(list.size() == count_, false);

  for (size_t i = 0; i < count_; i++) {
    const Json::Value& elt_value = list[static_cast<int>(i)];
    AssertWithReturnValue(elt_value.type() == Json::booleanValue, false);
    vals_[i] = elt_value.asBool();
  }

  return true;
}

void BoolArrayProperty::HandleGesturesPropWritten() {
  // TODO(adlr): Log array property changes
  if (delegate_)
    delegate_->BoolArrayWasWritten(this);
}

void DoubleProperty::CreatePropImpl() {
  double orig_val = val_;
  gprop_ = parent_->PropProvider()->create_real_fn(
      parent_->PropProviderData(),
      name(),
      &val_,
      1,
      &val_);
  if (delegate_ && orig_val != val_)
    delegate_->DoubleWasWritten(this);
}

Json::Value DoubleProperty::NewValue() const {
  return Json::Value(val_);
}

bool DoubleProperty::SetValue(const Json::Value& value) {
  if (value.type() != Json::realValue &&
      value.type() != Json::intValue &&
      value.type() != Json::uintValue) {
    return false;
  }
  val_ = value.asDouble();
  return true;
}

void DoubleProperty::HandleGesturesPropWritten() {
  if (parent_ && parent_->activity_log()) {
    ActivityLog::PropChangeEntry entry = {
      name(), ActivityLog::PropChangeEntry::kDoubleProp, { 0 }
    };
    entry.value.double_val = val_;
    parent_->activity_log()->LogPropChange(entry);
  }
  if (delegate_)
    delegate_->DoubleWasWritten(this);
}

void DoubleArrayProperty::CreatePropImpl() {
  float orig_vals[count_];
  memcpy(orig_vals, vals_, sizeof(orig_vals));
  gprop_ = parent_->PropProvider()->create_real_fn(
      parent_->PropProviderData(),
      name(),
      vals_,
      count_,
      vals_);
  if (delegate_ && memcmp(orig_vals, vals_, sizeof(orig_vals)))
    delegate_->DoubleArrayWasWritten(this);
}

Json::Value DoubleArrayProperty::NewValue() const {
  Json::Value list(Json::arrayValue);
  for (size_t i = 0; i < count_; i++) {
    // Avoid infinity
    double log_val = std::max(-1e30, std::min(vals_[i], 1e30));
    list.append(Json::Value(log_val));
  }
  return list;
}

bool DoubleArrayProperty::SetValue(const Json::Value& list) {
  AssertWithReturnValue(list.type() == Json::arrayValue, false);
  AssertWithReturnValue(list.size() == count_, false);

  for (size_t i = 0; i < count_; i++) {
    Json::Value elt_value = list[static_cast<int>(i)];
    AssertWithReturnValue(elt_value.type() == Json::realValue ||
                          elt_value.type() == Json::intValue ||
                          elt_value.type() == Json::uintValue, false);
    vals_[i] = elt_value.asDouble();
  }

  return true;
}

void DoubleArrayProperty::HandleGesturesPropWritten() {
  // TODO(adlr): Log array property changes
  if (delegate_)
    delegate_->DoubleArrayWasWritten(this);
}

void IntProperty::CreatePropImpl() {
  int orig_val = val_;
  gprop_ = parent_->PropProvider()->create_int_fn(
      parent_->PropProviderData(),
      name(),
      &val_,
      1,
      &val_);
  if (delegate_ && orig_val != val_)
    delegate_->IntWasWritten(this);
}

Json::Value IntProperty::NewValue() const {
  return Json::Value(val_);
}

bool IntProperty::SetValue(const Json::Value& value) {
  if (value.type() != Json::intValue &&
      value.type() != Json::uintValue) {
    Err("Failing here %d", value.type());
    return false;
  }
  val_ = value.asInt();
  return true;
}

void IntProperty::HandleGesturesPropWritten() {
  if (parent_ && parent_->activity_log()) {
    ActivityLog::PropChangeEntry entry = {
      name(), ActivityLog::PropChangeEntry::kIntProp, { 0 }
    };
    entry.value.int_val = val_;
    parent_->activity_log()->LogPropChange(entry);
  }
  if (delegate_)
    delegate_->IntWasWritten(this);
}

void IntArrayProperty::CreatePropImpl() {
  int orig_vals[count_];
  memcpy(orig_vals, vals_, sizeof(orig_vals));
  gprop_ = parent_->PropProvider()->create_int_fn(
      parent_->PropProviderData(),
      name(),
      vals_,
      count_,
      vals_);
  if (delegate_ && memcmp(orig_vals, vals_, sizeof(orig_vals)))
    delegate_->IntArrayWasWritten(this);
}

Json::Value IntArrayProperty::NewValue() const {
  Json::Value list(Json::arrayValue);
  for (size_t i = 0; i < count_; i++)
    list.append(Json::Value(vals_[static_cast<int>(i)]));
  return list;
}

bool IntArrayProperty::SetValue(const Json::Value& list) {
  AssertWithReturnValue(list.type() == Json::arrayValue, false);
  AssertWithReturnValue(list.size() == count_, false);

  for (size_t i = 0; i < count_; i++) {
    Json::Value elt_value = list[static_cast<int>(i)];
    AssertWithReturnValue(elt_value.type() == Json::intValue ||
                          elt_value.type() == Json::uintValue, false);
    vals_[i] = elt_value.asInt();
  }

  return true;
}

void IntArrayProperty::HandleGesturesPropWritten() {
  // TODO(adlr): Log array property changes
  if (delegate_)
    delegate_->IntArrayWasWritten(this);
}

void ShortProperty::CreatePropImpl() {
  short orig_val = val_;
  gprop_ = parent_->PropProvider()->create_short_fn(
      parent_->PropProviderData(),
      name(),
      &val_,
      1,
      &val_);
  if (delegate_ && orig_val != val_)
    delegate_->ShortWasWritten(this);
}

Json::Value ShortProperty::NewValue() const {
  return Json::Value(val_);
}

bool ShortProperty::SetValue(const Json::Value& value) {
  if (value.type() != Json::intValue &&
      value.type() != Json::uintValue) {
    return false;
  }
  val_ = value.asInt();
  return true;
}

void ShortProperty::HandleGesturesPropWritten() {
  if (parent_ && parent_->activity_log()) {
    ActivityLog::PropChangeEntry entry = {
      name(), ActivityLog::PropChangeEntry::kShortProp, { 0 }
    };
    entry.value.short_val = val_;
    parent_->activity_log()->LogPropChange(entry);
  }
  if (delegate_)
    delegate_->ShortWasWritten(this);
}

void ShortArrayProperty::CreatePropImpl() {
  short orig_vals[count_];
  memcpy(orig_vals, vals_, sizeof(orig_vals));
  gprop_ = parent_->PropProvider()->create_short_fn(
      parent_->PropProviderData(),
      name(),
      vals_,
      count_,
      vals_);
  if (delegate_ && memcmp(orig_vals, vals_, sizeof(orig_vals)))
    delegate_->ShortArrayWasWritten(this);
}

Json::Value ShortArrayProperty::NewValue() const {
  Json::Value list(Json::arrayValue);
  for (size_t i = 0; i < count_; i++)
    list.append(Json::Value(vals_[i]));
  return list;
}

bool ShortArrayProperty::SetValue(const Json::Value& list) {
  AssertWithReturnValue(list.type() == Json::arrayValue, false);
  AssertWithReturnValue(list.size() == count_, false);

  for (size_t i = 0; i < count_; i++) {
    Json::Value elt_value = list[static_cast<int>(i)];
    AssertWithReturnValue(elt_value.type() == Json::intValue ||
                          elt_value.type() == Json::uintValue, false);
    vals_[i] = elt_value.asInt();
  }

  return true;
}

void ShortArrayProperty::HandleGesturesPropWritten() {
  // TODO(adlr): Log array property changes
  if (delegate_)
    delegate_->ShortArrayWasWritten(this);
}

void StringProperty::CreatePropImpl() {
  const char* orig_val = val_;
  gprop_ = parent_->PropProvider()->create_string_fn(
      parent_->PropProviderData(),
      name(),
      &val_,
      val_);
  if (delegate_ && strcmp(orig_val, val_) == 0)
    delegate_->StringWasWritten(this);
}

Json::Value StringProperty::NewValue() const {
  return Json::Value(val_);
}

bool StringProperty::SetValue(const Json::Value& value) {
  if (value.type() != Json::stringValue) {
    return false;
  }
  parsed_val_ = value.asString();
  val_ = parsed_val_.c_str();
  return true;
}

void StringProperty::HandleGesturesPropWritten() {
  if (delegate_)
    delegate_->StringWasWritten(this);
}

}  // namespace gestures
