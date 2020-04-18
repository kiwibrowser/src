// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/map.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_IIR_FILTER_INTERPRETER_H_
#define GESTURES_IIR_FILTER_INTERPRETER_H_

namespace gestures {

// This filter interpreter applies a low-pass infinite impulse response (iir)
// filter to each incoming finger. The default filter is a low-pass 2nd order
// Butterworth IIR filter with a normalized cutoff frequency of 0.2. It can be
// configured via properties to use other formulae or
// different coefficients for the Butterworth filter.

class IirFilterInterpreter : public FilterInterpreter, public PropertyDelegate {
  FRIEND_TEST(IirFilterInterpreterTest, DisableIIRTest);
 public:
  // We'll maintain one IOHistory record per active finger
  class IoHistory {
   public:
    IoHistory() : in_head(0), out_head(0) {}
    explicit IoHistory(const FingerState& fs) : in_head(0), out_head(0) {
      for (size_t i = 0; i < kInSize; i++)
        in[i] = fs;
      for (size_t i = 0; i < kOutSize; i++)
        out[i] = fs;
    }
    // Note: NextOut() and the oldest PrevOut() point to the same object.
    FingerState* NextOut() { return &out[NextOutHead()]; }
    FingerState* PrevOut(size_t idx) {
      return &out[(out_head + idx) % kOutSize];
    }
    // Note: NextIn() and the oldest PrevIn() point to the same object.
    FingerState* NextIn() { return &in[NextInHead()]; }
    FingerState* PrevIn(size_t idx) { return &in[(in_head + idx) % kInSize]; }
    void Increment();

    bool operator==(const IoHistory& that) const;
    bool operator!=(const IoHistory& that) const { return !(*this == that); }

    void WarpBy(float dx, float dy);

   private:
    size_t NextOutHead() const {
      return (out_head + kOutSize - 1) % kOutSize;
    }
    size_t NextInHead() const {
      return (in_head + kInSize - 1) % kInSize;
    }

    static const size_t kInSize = 3;
    static const size_t kOutSize = 2;
    FingerState in[kInSize];  // previous input values
    size_t in_head;
    FingerState out[kOutSize];  // previous output values
    size_t out_head;
  };

  // Takes ownership of |next|:
  IirFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                       Tracer* tracer);
  virtual ~IirFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 public:
  virtual void DoubleWasWritten(DoubleProperty* prop);

 private:
  // y[0] = b[0]*x[0] + b[1]*x[1] + b[2]*x[2] + b[3]*x[3]
  //        - (a[1]*y[1] + a[2]*y[2])
  DoubleProperty b0_, b1_, b2_, b3_, a1_, a2_;

  // If position change between 2 frames is less than iir_dist_thresh_,
  // IIR filter is applied, otherwise rolling average is applied.
  DoubleProperty iir_dist_thresh_;
  // Whether to adjust the IIR history when finger WARP is detected.
  BoolProperty adjust_iir_on_warp_;
  // Whether IIR filter should be used. Put as a member varible for
  // unittest purpose.
  bool using_iir_;
  map<short, IoHistory, kMaxFingers> histories_;
};

}  // namespace gestures

#endif  // GESTURES_IIR_FILTER_INTERPRETER_H_
