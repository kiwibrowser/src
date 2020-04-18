// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_INTERPOLABLE_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_INTERPOLABLE_VALUE_H_

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/animatable/animatable_value.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Represents the components of a PropertySpecificKeyframe's value that change
// smoothly as it interpolates to an adjacent value.
class CORE_EXPORT InterpolableValue {
  USING_FAST_MALLOC(InterpolableValue);

 public:
  virtual ~InterpolableValue() = default;

  virtual bool IsNumber() const { return false; }
  virtual bool IsBool() const { return false; }
  virtual bool IsList() const { return false; }

  virtual bool Equals(const InterpolableValue&) const = 0;
  virtual std::unique_ptr<InterpolableValue> Clone() const = 0;
  virtual std::unique_ptr<InterpolableValue> CloneAndZero() const = 0;
  virtual void Scale(double scale) = 0;
  virtual void ScaleAndAdd(double scale, const InterpolableValue& other) = 0;

 private:
  virtual void Interpolate(const InterpolableValue& to,
                           const double progress,
                           InterpolableValue& result) const = 0;

  friend class LegacyStyleInterpolation;
  friend class TransitionInterpolation;
  friend class PairwisePrimitiveInterpolation;

  // Keep interpolate private, but allow calls within the hierarchy without
  // knowledge of type.
  friend class InterpolableNumber;
  friend class InterpolableList;

  friend class AnimationInterpolableValueTest;
};

class CORE_EXPORT InterpolableNumber final : public InterpolableValue {
 public:
  static std::unique_ptr<InterpolableNumber> Create(double value) {
    return base::WrapUnique(new InterpolableNumber(value));
  }

  bool IsNumber() const final { return true; }
  double Value() const { return value_; }
  bool Equals(const InterpolableValue& other) const final;
  std::unique_ptr<InterpolableValue> Clone() const final {
    return Create(value_);
  }
  std::unique_ptr<InterpolableValue> CloneAndZero() const final {
    return Create(0);
  }
  void Scale(double scale) final;
  void ScaleAndAdd(double scale, const InterpolableValue& other) final;
  void Set(double value) { value_ = value; }

 private:
  void Interpolate(const InterpolableValue& to,
                   const double progress,
                   InterpolableValue& result) const final;
  double value_;

  explicit InterpolableNumber(double value) : value_(value) {}
};

class CORE_EXPORT InterpolableList : public InterpolableValue {
 public:
  // Explicitly delete operator= because MSVC automatically generate
  // copy constructors and operator= for dll-exported classes.
  // Since InterpolableList is not copyable, automatically generated
  // operator= causes MSVC compiler error.
  // However, we cannot use DISALLOW_COPY_AND_ASSIGN because InterpolableList
  // has its own copy constructor. So just delete operator= here.
  InterpolableList& operator=(const InterpolableList&) = delete;

  static std::unique_ptr<InterpolableList> Create(
      const InterpolableList& other) {
    return base::WrapUnique(new InterpolableList(other));
  }

  static std::unique_ptr<InterpolableList> Create(size_t size) {
    return base::WrapUnique(new InterpolableList(size));
  }

  bool IsList() const final { return true; }
  void Set(size_t position, std::unique_ptr<InterpolableValue> value) {
    values_[position] = std::move(value);
  }
  const InterpolableValue* Get(size_t position) const {
    return values_[position].get();
  }
  std::unique_ptr<InterpolableValue>& GetMutable(size_t position) {
    return values_[position];
  }
  size_t length() const { return values_.size(); }
  bool Equals(const InterpolableValue& other) const final;
  std::unique_ptr<InterpolableValue> Clone() const final {
    return Create(*this);
  }
  std::unique_ptr<InterpolableValue> CloneAndZero() const final;
  void Scale(double scale) final;
  void ScaleAndAdd(double scale, const InterpolableValue& other) final;

 private:
  void Interpolate(const InterpolableValue& to,
                   const double progress,
                   InterpolableValue& result) const final;
  explicit InterpolableList(size_t size) : values_(size) {}

  InterpolableList(const InterpolableList& other) : values_(other.length()) {
    for (size_t i = 0; i < length(); i++)
      Set(i, other.values_[i]->Clone());
  }

  Vector<std::unique_ptr<InterpolableValue>> values_;
};

DEFINE_TYPE_CASTS(InterpolableNumber,
                  InterpolableValue,
                  value,
                  value->IsNumber(),
                  value.IsNumber());
DEFINE_TYPE_CASTS(InterpolableList,
                  InterpolableValue,
                  value,
                  value->IsList(),
                  value.IsList());

}  // namespace blink

#endif
