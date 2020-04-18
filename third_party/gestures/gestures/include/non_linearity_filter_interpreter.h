// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/gestures.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_NON_LINEARITY_FILTER_INTERPRETER_H_
#define GESTURES_NON_LINEARITY_FILTER_INTERPRETER_H_

namespace gestures {
// This filters the incoming hardware state to try and correct for nonlinearity
// in the sensor's output.  What we found to be happening was that even when a
// finger moved perfectly straight across the touchpad, "wiggles" were reported
// back to the computer.  This nonlinearities turned out to be quite consistent
// so this can filter them out.
//
// There is a data file specifying a large grid of observed errors, sampled
// across the space of inputs (x, y, and pressure).  Then as the hardware
// states come in, this module, looks at the finger positions and interpolates
// over the three axes to estimate the error in x and y for that reading, and
// compensates accordingly.
//
// The data file consists of three "range" arrays which define which points
// the error was sampled at, followed by a 3 dimensional array of those errors.
// The error matrix will have an entry for each value in the cross product of
// the three range arrays.
//
// File format:
//      X Range Array
//      Y Range Array
//      P Range Array
//      Error Matrix
//
// Range Array Format:
//      4 bytes: Integer length of array
//      8 bytes x length: Double values for each point in the array
//
// Error Matrix Format:
//      Three dimensional matrix of error entries stored in x, y, p order.
//      The number of entries is determined by the range arrays.
//      Each error entry is of the form:
//          8 bytes: Double x error
//          8 bytes: Double y error
//
// Currently, this only handles the situation where exactly 1 finger is on the
// touchpad at a time.  There may be interactions between multiple contacts
// that this doesn't take into consideration, so it simply skips hwstates with
// more than 1 finger.

class NonLinearityFilterInterpreter : public FilterInterpreter {
  FRIEND_TEST(NonLinearityFilterInterpreterTest, DisablingTest);
  FRIEND_TEST(NonLinearityFilterInterpreterTest, HWstateModificationTest);
  FRIEND_TEST(NonLinearityFilterInterpreterTest, HWstateNoChangesNeededTest);
 public:
  NonLinearityFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                           Tracer* tracer);

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  struct Error {
    double x_error;
    double y_error;
  };
  struct Bounds {
    ssize_t lo;
    ssize_t hi;
  };

  // The error readings are stored in a flattened matrix, this finds the 1d
  // index corresponding to the point (x_index, y_index, p_index)
  unsigned int ErrorIndex(size_t x_index, size_t y_index, size_t p_index) const;
  // Find the two values in the range on either side of "value" to interpolate
  Bounds FindBounds(float value, const std::unique_ptr<double[]>& range,
                           size_t len) const;
  // Given a point (x, y, p) calculate the non-linearity error that needs to be
  // compensated for at that point.
  Error GetError(float finger_x, float finger_y, float finger_p) const;
  // Interpolate linearly between p1 and p2, according to percent_p1
  Error LinearInterpolate(const Error& p1, const Error& p2,
                          float percent_p1) const;
  // Load nonlinearity data from disk and parse it
  void LoadData();
  // Parse only a range array from the binary data
  bool LoadRange(std::unique_ptr<double[]>& arr, size_t& len, FILE* fd);
  int ReadObject(void* buf, size_t object_size, FILE* fd);

  BoolProperty enabled_;
  StringProperty data_location_;
  // These three arrays define the points where the error was sampled.
  // There is a reading in err_ for each point formed by the cross product
  // of these arrays.
  std::unique_ptr<double[]> x_range_, y_range_, p_range_;
  size_t x_range_len_, y_range_len_, p_range_len_;
  // A flattened 3-d array holding the actual sampled error values
  std::unique_ptr<Error[]> err_;
};

}  // namespace gestures

#endif  // GESTURES_NON_LINEARITY_FILTER_INTERPRETER_H_
