// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_PUBLIC_CPP_MOJO_UKM_RECORDER_H_
#define SERVICES_METRICS_PUBLIC_CPP_MOJO_UKM_RECORDER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "services/metrics/public/cpp/metrics_export.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/mojom/ukm_interface.mojom.h"

namespace service_manager {
class Connector;
}

namespace ukm {

/**
 * A helper wrapper that lets UKM data be recorded on other processes with the
 * same interface that is used in the browser process.
 *
 * Usage Example:
 *
 *  std::unique_ptr<ukm::MojoUkmRecorder> ukm_recorder =
 *      ukm::MojoUkmRecorder::Create(context()->connector());
 *  ukm::builders::MyEvent(source_id)
 *      .SetMyMetric(metric_value)
 *      .Record(ukm_recorder.get());
 */
class METRICS_EXPORT MojoUkmRecorder : public UkmRecorder {
 public:
  explicit MojoUkmRecorder(mojom::UkmRecorderInterfacePtr recorder_interface);
  ~MojoUkmRecorder() override;

  // Helper for getting the wrapper from a connector.
  static std::unique_ptr<MojoUkmRecorder> Create(
      service_manager::Connector* connector);

  base::WeakPtr<MojoUkmRecorder> GetWeakPtr();

 private:
  // UkmRecorder:
  void UpdateSourceURL(SourceId source_id, const GURL& url) override;
  void AddEntry(mojom::UkmEntryPtr entry) override;

  mojom::UkmRecorderInterfacePtr interface_;

  base::WeakPtrFactory<MojoUkmRecorder> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoUkmRecorder);
};

}  // namespace ukm

#endif  // SERVICES_METRICS_PUBLIC_CPP_MOJO_UKM_RECORDER_H_
