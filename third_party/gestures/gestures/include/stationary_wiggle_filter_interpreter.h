// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/macros.h"
#include "gestures/include/map.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_STATIONARY_WIGGLE_FILTER_INTERPRETER_H_
#define GESTURES_STATIONARY_WIGGLE_FILTER_INTERPRETER_H_

namespace gestures {

// This interpreter tries to detect if a finger is moving/stationary from
// 'pure' signal energy level of samples. When a finger is detected as
// stationary, the warp flags will be set to cease the cursor wiggling.
//
// As there is noise signal mixed in original data, cursor wiggling could be
// observed on all platforms. When a finger is stationary, we could observe
// the noise is mainly from a high frequency wiggling and a low-frequency drift
// from input data. Theoretically, from signal processing of input data,
// pure finger signal could be obtained from a bandpass filter to remove these
// two noise sources. To simplify the implementation, the patch tries to
// implement a bandpass filter with a series connection of a high-pass filter
// and a low-pass filter.
//
//              +------------------+      +-------------------+
// original --\ | high-pass filter | --\  |  low-pass filter  | --\  pure
//  signal  --/ |for low-freq drift| --/  |for high-freq noise| --/  signal
//              +------------------+      +-------------------+
//
// Let's say we have original signal samples, s0, s1... sn. The high-pass
// filter for removing low-frequency drift could be simply implemented
// by shifting original signal to average of samples, i.e. the output of the
// high-pass filter for a sample window of 5:
//
//    m4 = s4 - (s0 + s1 + ... + s4) / 5
//    m5 = s5 - (s1 + s2 + ... + s5) / 5
//    ...
//
// Then, followed with a low-pass filter to remove high frequency noise.
// Since our goal is to detect if a finger is stationary or not, the simplest
// implementation of the low-pass filter is to get pure signal from the
// average of mixed signal.
//
//    p4 = (m0 + m1 + ... + m4) / 5
//    p5 = (m1 + m2 + ... + m5) / 5
//    ...
//
// Once we have the 'pure' signal data, it is easy to detect if a finger is
// stationary or not with a reasonable threshold of signal energy of pure signal
// samples.
//
//    signal energy:
//      se4 = p0^2 + p1^2 ... + p4^2
//      se5 = p1^2 + p2^2 ... + p5^2
//

#define SIGNAL_SAMPLES 5  // number of signal samples

struct FingerEnergy {
  float x;  // original position_x
  float y;  // original position_y
  float mixed_x;  // mixed signal of X
  float mixed_y;  // mixed signal of Y
  float energy_x;  // signal energy of X
  float energy_y;  // signal energy of Y

  bool operator==(const FingerEnergy& that) const {
    return x == that.x &&
        y == that.y &&
        mixed_x == that.mixed_x &&
        mixed_y == that.mixed_y &&
        energy_x == that.energy_x &&
        energy_y == that.energy_y;
  }
  bool operator!=(const FingerEnergy& that) const { return !(*this == that); }
};

class FingerEnergyHistory {
 public:
  FingerEnergyHistory()
      : size_(0),
        head_(0),
        moving_(false),
        idle_time_(0.1),
        prev_(0) {}

  // Push the current finger data into the history buffer
  void PushFingerState(const FingerState &fs, const stime_t timestamp);

  // Get previous moving state
  bool moving() const { return moving_; }

  // Check if the buffer is filled up
  bool HasEnoughSamples() const { return (size_ == max_size_); }

  // 0 is newest, 1 is next newest, ..., size_ - 1 is oldest.
  const FingerEnergy& Get(size_t offset) const;

  // Calculate current signal energy. A finger changes from stationary
  // to moving if its signal energy is greater than threshold.
  // If its previous state is moving, a hysteresis value could be used
  // to check if current signal energy could sustain the moving state.
  bool IsFingerMoving(float threshold);

  bool operator==(const FingerEnergyHistory& that) const;
  bool operator!=(const FingerEnergyHistory& that) const;

 private:
  FingerEnergy history_[SIGNAL_SAMPLES];  // the finger energy buffer
  size_t max_size_ = arraysize(history_);
  size_t size_;
  size_t head_;
  bool moving_;
  stime_t idle_time_;  // timeout for finger without state change
  stime_t prev_;
};

class StationaryWiggleFilterInterpreter : public FilterInterpreter {
 public:
  // Takes ownership of |next|:
  StationaryWiggleFilterInterpreter(PropRegistry* prop_reg,
                                    Interpreter* next,
                                    Tracer* tracer);
  virtual ~StationaryWiggleFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  // Calculate signal energy from input data and update finger flag if
  // a finger is stationary
  void UpdateStationaryFlags(HardwareState* hwstate);

  // Map of finger energy histories
  typedef map<short, FingerEnergyHistory, kMaxFingers> FingerEnergyHistoryMap;
  FingerEnergyHistoryMap histories_;

  // True if this interpreter is effective
  BoolProperty enabled_;

  // Threshold and hysteresis of signal energy for finger moving/stationary
  DoubleProperty threshold_;
  DoubleProperty hysteresis_;

  DISALLOW_COPY_AND_ASSIGN(StationaryWiggleFilterInterpreter);
};

}  // namespace gestures

#endif  // GESTURES_STATIONARY_WIGGLE_FILTER_INTERPRETER_H_
