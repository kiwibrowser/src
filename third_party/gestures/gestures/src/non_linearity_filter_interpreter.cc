// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/non_linearity_filter_interpreter.h"

#include <linux/in.h>

namespace {
const size_t kIntPackedSize = 4;
const size_t kDoublePackedSize = 8;
}

namespace gestures {

NonLinearityFilterInterpreter::NonLinearityFilterInterpreter(
                                                        PropRegistry* prop_reg,
                                                        Interpreter* next,
                                                        Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      enabled_(prop_reg, "Enable non-linearity correction", false),
      data_location_(prop_reg, "Non-linearity correction data file", "None"),
      x_range_len_(0), y_range_len_(0), p_range_len_(0) {
  InitName();
  LoadData();
}

unsigned int NonLinearityFilterInterpreter::ErrorIndex(size_t x_index,
                                                       size_t y_index,
                                                       size_t p_index) const {
  unsigned int index = x_index * y_range_len_ * p_range_len_ +
                       y_index * p_range_len_ + p_index;

  if (index >= x_range_len_ * y_range_len_ * p_range_len_)
    index = 0;
  return index;
}

int NonLinearityFilterInterpreter::ReadObject(void* buf, size_t object_size,
                                            FILE* fd) {
  int objs_read = fread(buf, object_size, 1, fd);
  /* If this machine is big endian, reverse the bytes in the object */
  if (object_size == kDoublePackedSize)
    __le64_to_cpu(*static_cast<double*>(buf));
  if (object_size == kIntPackedSize)
    __le32_to_cpu(*static_cast<int32_t*>(buf));

  return objs_read;
}

bool NonLinearityFilterInterpreter::LoadRange(std::unique_ptr<double[]>& arr,
                                              size_t& len, FILE* fd) {
  int tmp;
  if (!ReadObject(&tmp, kIntPackedSize, fd))
    return false;
  len = tmp;

  arr.reset(new double[len]);
  for (size_t i = 0; i < len; i++) {
    double tmp;
    if (!ReadObject(&tmp, kDoublePackedSize, fd))
      return false;
    else
      arr[i] = tmp;
  }
  return true;
}

void NonLinearityFilterInterpreter::LoadData() {
  FILE* data_fd = fopen(data_location_.val_, "rb");
  if (!data_fd) {
    Log("Unable to open non-linearity filter data '%s'", data_location_.val_);
    return;
  }

  // Load the ranges
  if (!LoadRange(x_range_, x_range_len_, data_fd))
    goto abort_load;
  if (!LoadRange(y_range_, y_range_len_, data_fd))
    goto abort_load;
  if (!LoadRange(p_range_, p_range_len_, data_fd))
    goto abort_load;

  // Load the error readings themselves
  err_.reset(new Error[x_range_len_ * y_range_len_ * p_range_len_]);
  Error tmp;
  for(unsigned int x = 0; x < x_range_len_; x++) {
    for(unsigned int y = 0; y < y_range_len_; y++) {
      for(unsigned int p = 0; p < p_range_len_; p++) {
        if (!ReadObject(&tmp.x_error, kDoublePackedSize, data_fd) ||
            !ReadObject(&tmp.y_error, kDoublePackedSize, data_fd)) {
          goto abort_load;
        }
        err_[ErrorIndex(x, y, p)] = tmp;
      }
    }
  }

  fclose(data_fd);
  return;

abort_load:
  x_range_.reset();
  x_range_len_ = 0;
  y_range_.reset();
  y_range_len_ = 0;
  p_range_.reset();
  p_range_len_ = 0;
  err_.reset();
  fclose(data_fd);
}

void NonLinearityFilterInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                                      stime_t* timeout) {
  if (enabled_.val_ && err_.get() && hwstate->finger_cnt == 1) {
    FingerState* finger = &(hwstate->fingers[0]);
    if (finger) {
      Error error = GetError(finger->position_x, finger->position_y,
                             finger->pressure);
      finger->position_x -= error.x_error;
      finger->position_y -= error.y_error;
    }
  }
  next_->SyncInterpret(hwstate, timeout);
}

NonLinearityFilterInterpreter::Error
NonLinearityFilterInterpreter::LinearInterpolate(const Error& p1,
                                                 const Error& p2,
                                                 float percent_p1) const {
  Error ret;
  ret.x_error = percent_p1 * p1.x_error + (1.0 - percent_p1) * p2.x_error;
  ret.y_error = percent_p1 * p1.y_error + (1.0 - percent_p1) * p2.y_error;
  return ret;
}

NonLinearityFilterInterpreter::Error
NonLinearityFilterInterpreter::GetError(float finger_x, float finger_y,
                                        float finger_p) const {
  // First, find the 6 values surrounding the point to interpolate over
  Bounds x_bounds = FindBounds(finger_x, x_range_, x_range_len_);
  Bounds y_bounds = FindBounds(finger_y, y_range_, y_range_len_);
  Bounds p_bounds = FindBounds(finger_p, p_range_, p_range_len_);

  if (x_bounds.lo == -1 || x_bounds.hi == -1 || y_bounds.lo == -1 ||
    y_bounds.hi == -1 || p_bounds.lo == -1 || p_bounds.hi == -1) {
    Error error = { 0, 0 };
    return error;
  }

  // Interpolate along the x-axis
  float x_hi_perc = (finger_x - x_range_[x_bounds.lo]) /
                    (x_range_[x_bounds.hi] - x_range_[x_bounds.lo]);
  Error e_yhi_phi = LinearInterpolate(
                        err_[ErrorIndex(x_bounds.hi, y_bounds.hi, p_bounds.hi)],
                        err_[ErrorIndex(x_bounds.lo, y_bounds.hi, p_bounds.hi)],
                        x_hi_perc);
  Error e_yhi_plo = LinearInterpolate(
                        err_[ErrorIndex(x_bounds.hi, y_bounds.hi, p_bounds.lo)],
                        err_[ErrorIndex(x_bounds.lo, y_bounds.hi, p_bounds.lo)],
                        x_hi_perc);
  Error e_ylo_phi = LinearInterpolate(
                        err_[ErrorIndex(x_bounds.hi, y_bounds.lo, p_bounds.hi)],
                        err_[ErrorIndex(x_bounds.lo, y_bounds.lo, p_bounds.hi)],
                        x_hi_perc);
  Error e_ylo_plo = LinearInterpolate(
                        err_[ErrorIndex(x_bounds.hi, y_bounds.lo, p_bounds.lo)],
                        err_[ErrorIndex(x_bounds.lo, y_bounds.lo, p_bounds.lo)],
                        x_hi_perc);

  // Interpolate along the y-axis
  float y_hi_perc = (finger_y - y_range_[y_bounds.lo]) /
                    (y_range_[y_bounds.hi] - y_range_[y_bounds.lo]);
  Error e_plo = LinearInterpolate(e_yhi_plo, e_ylo_plo, y_hi_perc);
  Error e_phi = LinearInterpolate(e_yhi_phi, e_ylo_phi, y_hi_perc);

  // Finally, interpolate along the p-axis
  float p_hi_perc = (finger_p - p_range_[p_bounds.lo]) /
                    (p_range_[p_bounds.hi] - p_range_[p_bounds.lo]);
  Error error = LinearInterpolate(e_phi, e_plo, p_hi_perc);

  return error;
}

NonLinearityFilterInterpreter::Bounds
NonLinearityFilterInterpreter::FindBounds(
    float value,
    const std::unique_ptr<double[]>& range,
    size_t len) const {
  Bounds bounds;
  bounds.lo = bounds.hi = -1;

  for (size_t i = 0; i < len; i++) {
    if (range[i] <= value) {
      bounds.lo = i;
    } else {
      bounds.hi = i;
      break;
    }
  }

  return bounds;
}

}  // namespace gestures
