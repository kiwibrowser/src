// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BASE_SYNCED_PROPERTY_H_
#define CC_BASE_SYNCED_PROPERTY_H_

#include "base/memory/ref_counted.h"

namespace cc {

// This class is the basic primitive used for impl-thread scrolling.  Its job is
// to sanely resolve the case where both the main and impl thread are
// concurrently updating the same value (for example, when Javascript sets the
// scroll offset during an ongoing impl-side scroll).
//
// There are three trees (main, pending, and active) and therefore also three
// places with their own idea of the scroll offsets (and analogous properties
// like page scale).  Objects of this class are meant to be held on the Impl
// side, and contain the canonical reference for the pending and active trees,
// as well as keeping track of the latest delta sent to the main thread (which
// is necessary for conflict resolution).

template <typename T>
class SyncedProperty : public base::RefCounted<SyncedProperty<T>> {
 public:
  SyncedProperty() : clobber_active_value_(false) {}

  // Returns the canonical value for the specified tree, including the sum of
  // all deltas.  The pending tree should use this for activation purposes and
  // the active tree should use this for drawing.
  typename T::ValueType Current(bool is_active_tree) const {
    if (is_active_tree)
      return active_base_.Combine(active_delta_).get();
    else
      return pending_base_.Combine(PendingDelta()).get();
  }

  // Sets the value on the impl thread, due to an impl-thread-originating
  // action.  Returns true if this had any effect.  This will remain
  // impl-thread-only information at first, and will get pulled back to the main
  // thread on the next call of PullDeltaToMainThread (which happens right
  // before the commit).
  bool SetCurrent(typename T::ValueType current) {
    T delta = T(current).InverseCombine(active_base_);
    if (active_delta_.get() == delta.get())
      return false;

    active_delta_ = delta;
    return true;
  }

  // Returns the difference between the last value that was committed and
  // activated from the main thread, and the current total value.
  typename T::ValueType Delta() const { return active_delta_.get(); }

  // Returns the latest active tree delta and also makes a note that this value
  // was sent to the main thread.
  typename T::ValueType PullDeltaForMainThread() {
    reflected_delta_in_main_tree_ = PendingDelta();
    return reflected_delta_in_main_tree_.get();
  }

  // Push the latest value from the main thread onto pending tree-associated
  // state. Returns true if pushing the value results in different values
  // between the main layer tree and the pending tree.
  bool PushMainToPending(typename T::ValueType main_thread_value) {
    reflected_delta_in_pending_tree_ = reflected_delta_in_main_tree_;
    reflected_delta_in_main_tree_ = T::Identity();
    pending_base_ = T(main_thread_value);

    return Current(false) != main_thread_value;
  }

  // Push the value associated with the pending tree to be the active base
  // value. As part of this, subtract the delta reflected in the pending tree
  // from the active tree delta (which will make the delta zero at steady state,
  // or make it contain only the difference since the last send).
  // Returns true if pushing the update results in:
  // 1) Different values on the pending tree and the active tree.
  // 2) An update to the current value on the active tree.
  // The reason for considering the second case only when pushing to the active
  // tree, as opposed to when pushing to the pending tree, is that only the
  // active tree computes state using this value which is not computed on the
  // pending tree and not pushed during activation (aka scrollbar geometries).
  bool PushPendingToActive() {
    typename T::ValueType pending_value_before_push = Current(false);
    typename T::ValueType active_value_before_push = Current(true);

    active_base_ = pending_base_;
    active_delta_ = PendingDelta();
    reflected_delta_in_pending_tree_ = T::Identity();
    clobber_active_value_ = false;

    typename T::ValueType current_active_value = Current(true);
    return pending_value_before_push != current_active_value ||
           active_value_before_push != current_active_value;
  }

  // This simulates the consequences of the sent value getting committed and
  // activated.
  void AbortCommit() {
    pending_base_ = pending_base_.Combine(reflected_delta_in_main_tree_);
    active_base_ = active_base_.Combine(reflected_delta_in_main_tree_);
    active_delta_ = active_delta_.InverseCombine(reflected_delta_in_main_tree_);
    reflected_delta_in_main_tree_ = T::Identity();
  }

  // Values as last pushed to the pending or active tree respectively, with no
  // impl-thread delta applied.
  typename T::ValueType PendingBase() const { return pending_base_.get(); }
  typename T::ValueType ActiveBase() const { return active_base_.get(); }

  // The new delta we would use if we decide to activate now.  This delta
  // excludes the amount that we know is reflected in the pending tree.
  T PendingDelta() const {
    if (clobber_active_value_)
      return T::Identity();
    return active_delta_.InverseCombine(reflected_delta_in_pending_tree_);
  }

  void set_clobber_active_value() { clobber_active_value_ = true; }

 private:
  // Value last committed to the pending tree.
  T pending_base_;
  // Value last committed to the active tree on the last activation.
  T active_base_;
  // The difference between |active_base_| and the user-perceived value.
  T active_delta_;
  // The value sent to the main thread on the last BeginMainFrame.  This is
  // always identity outside of the BeginMainFrame to (aborted)commit interval.
  T reflected_delta_in_main_tree_;
  // The value that was sent to the main thread for BeginMainFrame for the
  // current pending tree.  This is always identity outside of the
  // BeginMainFrame to activation interval.
  T reflected_delta_in_pending_tree_;
  // When true the pending delta is always identity so that it does not change
  // and will clobber the active value on push.
  bool clobber_active_value_;

  friend class base::RefCounted<SyncedProperty<T>>;
  ~SyncedProperty() {}
};

// SyncedProperty's delta-based conflict resolution logic makes sense for any
// mathematical group.  In practice, there are two that are useful:
// 1. Numbers/vectors with addition and identity = 0 (like scroll offsets)
// 2. Real numbers with multiplication and identity = 1 (like page scale)

template <class V>
class AdditionGroup {
 public:
  typedef V ValueType;

  AdditionGroup() : value_(Identity().get()) {}
  explicit AdditionGroup(V value) : value_(value) {}

  V& get() { return value_; }
  const V& get() const { return value_; }

  static AdditionGroup<V> Identity() { return AdditionGroup(V()); }  // zero
  AdditionGroup<V> Combine(AdditionGroup<V> p) const {
    return AdditionGroup<V>(value_ + p.value_);
  }
  AdditionGroup<V> InverseCombine(AdditionGroup<V> p) const {
    return AdditionGroup<V>(value_ - p.value_);
  }

 private:
  V value_;
};

class ScaleGroup {
 public:
  typedef float ValueType;

  ScaleGroup() : value_(Identity().get()) {}
  explicit ScaleGroup(float value) : value_(value) {}

  float& get() { return value_; }
  const float& get() const { return value_; }

  static ScaleGroup Identity() { return ScaleGroup(1.f); }
  ScaleGroup Combine(ScaleGroup p) const {
    return ScaleGroup(value_ * p.value_);
  }
  ScaleGroup InverseCombine(ScaleGroup p) const {
    return ScaleGroup(value_ / p.value_);
  }

 private:
  float value_;
};

}  // namespace cc

#endif  // CC_BASE_SYNCED_PROPERTY_H_
