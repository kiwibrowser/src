// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/battor_agent/battor_sample_converter.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/battor_agent/battor_protocol_types.h"

using namespace testing;

namespace battor {

TEST(BattOrSampleConverterTest, ToSampleSimple) {
  BattOrEEPROM eeprom;
  eeprom.r1 = 1.0f;
  eeprom.r2 = 1.0f;
  eeprom.r3 = 1.0f;
  eeprom.low_gain = 1.0f;
  eeprom.low_gain_correction_offset = 0.0f;
  eeprom.low_gain_correction_factor = 1.0f;
  eeprom.sd_sample_rate = 1000;

  // Create a calibration frame with a baseline voltage and current of zero.
  std::vector<RawBattOrSample> calibration_frame;
  calibration_frame.push_back(RawBattOrSample{0, 0});
  BattOrSampleConverter converter(eeprom, calibration_frame);

  // Set both the voltage and current to their max values.
  RawBattOrSample raw_one{2048, 2048};
  BattOrSample one = converter.ToSample(raw_one, 0);

  ASSERT_DOUBLE_EQ(0, one.time_ms);
  ASSERT_DOUBLE_EQ(2401.172447484123, one.voltage_mV);
  ASSERT_DOUBLE_EQ(1200.5862237420615, one.current_mA);
}

TEST(BattOrSampleConverterTest, ToSampleNonZeroBaseline) {
  BattOrEEPROM eeprom;
  eeprom.r1 = 1.0f;
  eeprom.r2 = 1.0f;
  eeprom.r3 = 1.0f;
  eeprom.low_gain = 1.0f;
  eeprom.low_gain_correction_offset = 0.0f;
  eeprom.low_gain_correction_factor = 1.0f;
  eeprom.sd_sample_rate = 1000;

  // Create a calibration frame with a baseline voltage and current of zero.
  std::vector<RawBattOrSample> calibration_frame;
  calibration_frame.push_back(RawBattOrSample{1024, 1024});
  BattOrSampleConverter converter(eeprom, calibration_frame);

  // Set both the voltage and current to their max values.
  RawBattOrSample raw_one{2048, 2048};
  BattOrSample one = converter.ToSample(raw_one, 0);

  ASSERT_DOUBLE_EQ(0, one.time_ms);
  ASSERT_DOUBLE_EQ(1200.586223742061, one.voltage_mV);
  ASSERT_DOUBLE_EQ(600.29311187103076, one.current_mA);
}

TEST(BattOrSampleConverterTest, ToSampleNonZeroMultiSampleBaseline) {
  BattOrEEPROM eeprom;
  eeprom.r1 = 1.0f;
  eeprom.r2 = 1.0f;
  eeprom.r3 = 1.0f;
  eeprom.low_gain = 1.0f;
  eeprom.low_gain_correction_offset = 0.0f;
  eeprom.low_gain_correction_factor = 1.0f;
  eeprom.sd_sample_rate = 1000;

  // Create a calibration frame with a baseline voltage and current of zero.
  std::vector<RawBattOrSample> calibration_frame;
  calibration_frame.push_back(RawBattOrSample{1000, 1000});
  calibration_frame.push_back(RawBattOrSample{1048, 1048});
  BattOrSampleConverter converter(eeprom, calibration_frame);

  // Set both the voltage and current to their max values.
  RawBattOrSample raw_one{2048, 2048};
  BattOrSample one = converter.ToSample(raw_one, 0);

  ASSERT_DOUBLE_EQ(0, one.time_ms);
  ASSERT_DOUBLE_EQ(1200.5862237420615, one.voltage_mV);
  ASSERT_DOUBLE_EQ(600.29311187103076, one.current_mA);
}

TEST(BattOrSampleConverterTest, ToSampleRealValues) {
  BattOrEEPROM eeprom;
  eeprom.r1 = 10.0f;
  eeprom.r2 = 14.0f;
  eeprom.r3 = 17.0f;
  eeprom.low_gain = 1.5;
  eeprom.low_gain_correction_offset = 0.03f;
  eeprom.low_gain_correction_factor = 4.0f;
  eeprom.sd_sample_rate = 1000;

  // Create a calibration frame with a baseline voltage and current of zero.
  std::vector<RawBattOrSample> calibration_frame;
  calibration_frame.push_back(RawBattOrSample{800, 900});
  calibration_frame.push_back(RawBattOrSample{1000, 1100});
  BattOrSampleConverter converter(eeprom, calibration_frame);

  // Set both the voltage and current to their max values.
  RawBattOrSample raw_one{1900, 2000};
  BattOrSample one = converter.ToSample(raw_one, 0);

  ASSERT_DOUBLE_EQ(0, one.time_ms);
  ASSERT_DOUBLE_EQ(1068.996209287540, one.voltage_mV);
  ASSERT_DOUBLE_EQ(9.7628957011935285, one.current_mA);
}

TEST(BattOrSampleConverterTest, ToSampleRealNegativeValues) {
  BattOrEEPROM eeprom;
  eeprom.r1 = 10.0f;
  eeprom.r2 = 14.0f;
  eeprom.r3 = 17.0f;
  eeprom.low_gain = 1.5;
  eeprom.low_gain_correction_offset = 0.03f;
  eeprom.low_gain_correction_factor = 4.0f;
  eeprom.sd_sample_rate = 1000;

  // Create a calibration frame with a baseline voltage and current of zero.
  std::vector<RawBattOrSample> calibration_frame;
  calibration_frame.push_back(RawBattOrSample{800, 900});
  BattOrSampleConverter converter(eeprom, calibration_frame);

  // Set both the voltage and current to their max values.
  RawBattOrSample raw_one{-1900, -2000};
  BattOrSample one = converter.ToSample(raw_one, 0);

  ASSERT_DOUBLE_EQ(0, one.time_ms);
  ASSERT_DOUBLE_EQ(-2885.2980205462577, one.voltage_mV);
  ASSERT_DOUBLE_EQ(-28.332106130755665, one.current_mA);
}

TEST(BattOrSampleConverterTest, ToSampleMultipleSamples) {
  BattOrEEPROM eeprom;
  eeprom.r1 = 1.0f;
  eeprom.r2 = 1.0f;
  eeprom.r3 = 1.0f;
  eeprom.low_gain = 1.0f;
  eeprom.low_gain_correction_offset = 0.0f;
  eeprom.low_gain_correction_factor = 1.0f;
  eeprom.sd_sample_rate = 50;

  std::vector<RawBattOrSample> calibration_frame;
  calibration_frame.push_back(RawBattOrSample{0, 0});
  BattOrSampleConverter converter(eeprom, calibration_frame);

  BattOrSample one = converter.ToSample(RawBattOrSample{0, 0}, 0);
  BattOrSample two = converter.ToSample(RawBattOrSample{0, 0}, 1);
  BattOrSample three = converter.ToSample(RawBattOrSample{0, 0}, 2);

  ASSERT_DOUBLE_EQ(0, one.time_ms);
  ASSERT_DOUBLE_EQ(20, two.time_ms);
  ASSERT_DOUBLE_EQ(40, three.time_ms);
}

}  // namespace battor
