// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// GENERATED FROM THE SCHEMA DEFINITION AND DESCRIPTION IN
//   field_trial_testing_config_schema.json
//   test_config.json
// DO NOT EDIT.

#ifndef TEST_OUTPUT_H_
#define TEST_OUTPUT_H_

#include <cstddef>


struct FieldTrialTestingExperimentParams {
  const char* const key;
  const char* const value;
};

struct FieldTrialTestingExperiment {
  const char* const name;
  const FieldTrialTestingExperimentParams * params;
  const size_t params_size;
  const char* const * enable_features;
  const size_t enable_features_size;
  const char* const * disable_features;
  const size_t disable_features_size;
  const char* const forcing_flag;
};

struct FieldTrialTestingStudy {
  const char* const name;
  const FieldTrialTestingExperiment * experiments;
  const size_t experiments_size;
};

struct FieldTrialTestingConfig {
  const FieldTrialTestingStudy * studies;
  const size_t studies_size;
};


extern const FieldTrialTestingConfig kFieldTrialConfig;

#endif  // TEST_OUTPUT_H_
