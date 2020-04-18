// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/form_field_data_predictions.h"

namespace autofill {

FormFieldDataPredictions::FormFieldDataPredictions() {
}

FormFieldDataPredictions::FormFieldDataPredictions(
    const FormFieldDataPredictions& other)
    : field(other.field),
      signature(other.signature),
      heuristic_type(other.heuristic_type),
      server_type(other.server_type),
      overall_type(other.overall_type),
      parseable_name(other.parseable_name),
      section(other.section) {}

FormFieldDataPredictions::~FormFieldDataPredictions() {
}

bool FormFieldDataPredictions::operator==(
    const FormFieldDataPredictions& predictions) const {
  return (field.SameFieldAs(predictions.field) &&
          signature == predictions.signature &&
          heuristic_type == predictions.heuristic_type &&
          server_type == predictions.server_type &&
          overall_type == predictions.overall_type &&
          parseable_name == predictions.parseable_name &&
          section == predictions.section);
}

bool FormFieldDataPredictions::operator!=(
    const FormFieldDataPredictions& predictions) const {
  return !operator==(predictions);
}

}  // namespace autofill
