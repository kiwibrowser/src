// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_PUBLIC_RAPPOR_SERVICE_H_
#define COMPONENTS_RAPPOR_PUBLIC_RAPPOR_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "components/rappor/public/rappor_parameters.h"
#include "components/rappor/public/sample.h"

namespace rappor {

// This class provides a public interface for recording samples for rappor
// metrics, which other components can be depended on.
class RapporService : public base::SupportsWeakPtr<RapporService> {
 public:
  // Constructs a Sample object for the caller to record fields in.
  virtual std::unique_ptr<Sample> CreateSample(RapporType) = 0;

  // Records a Sample of rappor metric specified by |metric_name|.
  //
  // example:
  // std::unique_ptr<Sample> sample =
  // rappor_service->CreateSample(MY_METRIC_TYPE);
  // sample->SetStringField("Field1", "some string");
  // sample->SetFlagsValue("Field2", SOME|FLAGS);
  // rappor_service->RecordSample("MyMetric", std::move(sample));
  virtual void RecordSample(const std::string& metric_name,
                            std::unique_ptr<Sample> sample) = 0;

  // Records a sample of the rappor metric specified by |metric_name|.
  // Creates and initializes the metric, if it doesn't yet exist.
  virtual void RecordSampleString(const std::string& metric_name,
                                  RapporType type,
                                  const std::string& sample) = 0;
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_PUBLIC_RAPPOR_SERVICE_H_
