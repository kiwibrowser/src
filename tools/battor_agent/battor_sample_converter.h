// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BATTOR_AGENT_BATTOR_SAMPLE_CONVERTER_H_
#define TOOLS_BATTOR_AGENT_BATTOR_SAMPLE_CONVERTER_H_

#include <stddef.h>
#include <vector>

#include "base/macros.h"
#include "tools/battor_agent/battor_protocol_types.h"

namespace battor {

// Converter capable of taking raw samples from the BattOr and using
// configuration information to turn them into samples with real units.
class BattOrSampleConverter {
 public:
  // Constructs a BattOrSampleConverter.
  //
  //   - eeprom: The BattOr's EEPROM, which contains some required conversion
  //     parameters.
  //   - calibration_frame: The first frame sent back from the BattOr when
  //     streaming samples. This frame gives current and voltage measurements
  //     that ignore whatever the BattOr's connected to, and therefore provide
  //     a means for us to determine baseline current and voltage.
  BattOrSampleConverter(const BattOrEEPROM& eeprom,
                        const std::vector<RawBattOrSample>& calibration_frame);
  virtual ~BattOrSampleConverter();

  // Converts a raw sample to a unitful one with a timestamp.
  BattOrSample ToSample(const RawBattOrSample& sample,
                        size_t sample_number) const;

  // Converts a raw sample to watts.
  float ToWatts(const RawBattOrSample& sample) const;

  // Returns the lowest magnitude sample that the BattOr can collect.
  BattOrSample MinSample() const;

  // Returns the highest magnitude sample that the BattOr can collect.
  BattOrSample MaxSample() const;

 private:
  // The BattOr's EEPROM, which stores some conversion parameters we need.
  BattOrEEPROM eeprom_;

  // The baseline current and voltage calculated from the calibration frame.
  double baseline_current_;
  double baseline_voltage_;

  DISALLOW_COPY_AND_ASSIGN(BattOrSampleConverter);
};

}  // namespace battor

#endif  // TOOLS_BATTOR_AGENT_BATTOR_SAMPLE_CONVERTER_H_
