// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/stationary_wiggle_filter_interpreter.h"

#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/map.h"
#include "gestures/include/tracer.h"
#include "gestures/include/util.h"

namespace gestures {

void FingerEnergyHistory::PushFingerState(const FingerState &fs,
                                          const stime_t timestamp) {

  // Reset the history if there is no finger state received longer than
  // time interval 'idle_time_'.
  if (moving_ && timestamp - prev_ > idle_time_) {
    moving_ = false;
    head_ = size_ = 0;
  }

  // Insert current finger position into the queue
  head_ = (head_ + max_size_ - 1) % max_size_;
  history_[head_].x = fs.position_x;
  history_[head_].y = fs.position_y;
  size_ = std::min(size_ + 1, max_size_);

  // Calculate average of original signal set, the average of original signal
  // is considered as the offset.
  float sum_x = 0.0;
  float sum_y = 0.0;
  for (size_t i = 0; i < size_; ++i) {
    const FingerEnergy& fe = Get(i);
    sum_x += fe.x;
    sum_y += fe.y;
  }
  // Obtain the mixed signal strength
  history_[head_].mixed_x = fs.position_x - sum_x / size_;
  history_[head_].mixed_y = fs.position_y - sum_y / size_;


  // Calculate the average of the mixed signal set, the average of mixed signal
  // is considered as pure signal strength.
  float psx = 0.0;
  float psy = 0.0;
  for (size_t i = 0; i < size_; ++i) {
    const FingerEnergy& fe = Get(i);
    psx += fe.mixed_x;
    psy += fe.mixed_y;
  }
  psx /= size_;
  psy /= size_;
  // Calculate current pure signal energy
  history_[head_].energy_x = psx * psx;
  history_[head_].energy_y = psy * psy;

  prev_ = timestamp;
}

const FingerEnergy& FingerEnergyHistory::Get(size_t offset) const {
  if (offset >= size_) {
    Err("Out of bounds access!");
    // avoid returning null pointer
    static FingerEnergy dummy_event = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    return dummy_event;
  }
  return history_[(head_ + offset) % max_size_];
}

bool FingerEnergyHistory::IsFingerMoving(float threshold) {
  if (size_ < max_size_)
    return false;

  float sum_energy_x = 0.0;
  float sum_energy_y = 0.0;
  for (size_t i = 0; i < size_; i++) {
    const FingerEnergy& fe = history_[i];
    sum_energy_x += fe.energy_x;
    sum_energy_y += fe.energy_y;
  }
  moving_ = (sum_energy_x > threshold || sum_energy_y > threshold);
  return moving_;
}

bool FingerEnergyHistory::operator==(const FingerEnergyHistory& that) const {
  for (size_t i = 0; i < size_; i++)
    if (history_[i] != that.history_[i])
      return false;
  if (size_ != that.size_ || head_ != that.head_ || moving_ != that.moving_)
    return false;
  return true;
}

bool FingerEnergyHistory::operator!=(const FingerEnergyHistory& that) const {
  return !(*this == that);
}

StationaryWiggleFilterInterpreter::StationaryWiggleFilterInterpreter(
    PropRegistry* prop_reg, Interpreter* next, Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      enabled_(prop_reg, "Stationary Wiggle Filter Enabled", false),
      threshold_(prop_reg, "Finger Moving Energy", 0.012),
      hysteresis_(prop_reg, "Finger Moving Hysteresis", 0.006) {
  InitName();
}

void StationaryWiggleFilterInterpreter::SyncInterpretImpl(
    HardwareState* hwstate, stime_t* timeout) {
  if (enabled_.val_)
    UpdateStationaryFlags(hwstate);
  next_->SyncInterpret(hwstate, timeout);
}

void StationaryWiggleFilterInterpreter::UpdateStationaryFlags(
    HardwareState* hwstate) {

  RemoveMissingIdsFromMap(&histories_, *hwstate);

  for (int i = 0; i < hwstate->finger_cnt; ++i) {
    FingerState *fs = &hwstate->fingers[i];

    // Create a new entry if it is a new finger
    if (!MapContainsKey(histories_, fs->tracking_id)) {
      histories_[fs->tracking_id] = FingerEnergyHistory();
      histories_[fs->tracking_id].PushFingerState(*fs, hwstate->timestamp);
      continue;
    }

    // Update the energy history and check if the finger is moving
    FingerEnergyHistory& feh = histories_[fs->tracking_id];
    feh.PushFingerState(*fs, hwstate->timestamp);
    if (feh.HasEnoughSamples()) {
      float threshold = feh.moving() ? hysteresis_.val_ : threshold_.val_;
      if (!feh.IsFingerMoving(threshold))
        fs->flags |= (GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y);
      else
        fs->flags |= GESTURES_FINGER_INSTANTANEOUS_MOVING;
    }
  }
}

}  // namespace gestures
