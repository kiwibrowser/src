// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "battor_sample_converter.h"

#include <stdlib.h>

namespace battor {

namespace {

// The analog to digital converts an analog signal to a signed 12 bit integer,
// meaning that it can output numbers in the range [-2048, 2047].
const int16_t kAnalogDigitalConverterMinValue = -2048;
const int16_t kAnalogDigitalConverterMaxValue = 2047;

// The maximum voltage that the BattOr is capable of measuring.
const double kMaxVoltage = 1.2;

// Converts a raw voltage to a unitful one.
double ToUnitfulVoltage(double voltage_raw) {
  // Raw voltage samples are collected directly from the BattOr's analog to
  // digital converter, which converts numbers in the domain [-1.2V, 1.2V] to
  // numbers in the range [-2048, 2047]. A zero voltage has the same meaning in
  // both the domain and range. Because of this, one negative unit in that range
  // represents a slightly smaller domain (1.2 / 2048) than one positive unit
  // in that range (1.2 / 2047). We take this into account when reversing the
  // transformation here.
  int16_t extreme_value = voltage_raw >= 0 ? kAnalogDigitalConverterMaxValue
                                           : kAnalogDigitalConverterMinValue;

  return voltage_raw / abs(extreme_value) * kMaxVoltage;
}

}  // namespace

BattOrSampleConverter::BattOrSampleConverter(
    const BattOrEEPROM& eeprom,
    const std::vector<RawBattOrSample>& calibration_frame)
    : eeprom_(eeprom) {
  baseline_current_ = baseline_voltage_ = 0;
  for (auto sample : calibration_frame) {
    baseline_current_ += ToUnitfulVoltage(sample.current_raw);
    baseline_voltage_ += ToUnitfulVoltage(sample.voltage_raw);
  }

  baseline_current_ /= calibration_frame.size();
  baseline_voltage_ /= calibration_frame.size();
}

BattOrSampleConverter::~BattOrSampleConverter() = default;

BattOrSample BattOrSampleConverter::ToSample(const RawBattOrSample& sample,
                                             size_t sample_number) const {
  // Subtract out the baseline current and voltage that the BattOr reads even
  // when it's not attached to anything.
  double current = ToUnitfulVoltage(sample.current_raw) - baseline_current_;
  double voltage = ToUnitfulVoltage(sample.voltage_raw) - baseline_voltage_;

  // The BattOr has to amplify the voltage so that it's on a similar scale as
  // the reference voltage. This is done in the circuit using resistors (with
  // known resistances r2 and r3). Here we undo that amplification.
  double voltage_divider = eeprom_.r3 / (eeprom_.r2 + eeprom_.r3);
  voltage /= voltage_divider;

  // Convert to millivolts.
  voltage *= 1000;

  // The BattOr multiplies the current by the gain, so we have to undo that
  // amplification, too.
  current /= eeprom_.low_gain;

  // The current is measured indirectly and is actually given to us as a voltage
  // across a resistor with a known resistance r1. Because
  //
  //   V (voltage) = i (current) * R (resistance)
  //
  // we can get the current by dividing this voltage by the resistance.
  current /= eeprom_.r1;

  // Convert to milliamps.
  current *= 1000;

  // Each BattOr is individually factory-calibrated. Apply these calibrations.
  current -= eeprom_.low_gain_correction_offset;
  current /= eeprom_.low_gain_correction_factor;

  double time_ms = double(sample_number) / eeprom_.sd_sample_rate * 1000;

  return BattOrSample{time_ms, voltage, current};
}

float BattOrSampleConverter::ToWatts(const RawBattOrSample& raw_sample) const {
  BattOrSample sample = ToSample(raw_sample, 0);

  return sample.current_mA * sample.voltage_mV * 1e-6f;
}

BattOrSample BattOrSampleConverter::MinSample() const {
  // Create a minimum raw sample.
  RawBattOrSample sample_raw = {kAnalogDigitalConverterMinValue,
                                kAnalogDigitalConverterMinValue};
  return ToSample(sample_raw, 0);
}

BattOrSample BattOrSampleConverter::MaxSample() const {
  // Create a maximum raw sample.
  RawBattOrSample sample_raw = {kAnalogDigitalConverterMaxValue,
                                kAnalogDigitalConverterMaxValue};
  return ToSample(sample_raw, 0);
}

}  // namespace battor
