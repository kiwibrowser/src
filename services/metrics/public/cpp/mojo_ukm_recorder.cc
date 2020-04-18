// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/mojo_ukm_recorder.h"

#include <utility>

#include "services/metrics/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace ukm {

MojoUkmRecorder::MojoUkmRecorder(mojom::UkmRecorderInterfacePtr interface)
    : interface_(std::move(interface)), weak_factory_(this) {}
MojoUkmRecorder::~MojoUkmRecorder() = default;

// static
std::unique_ptr<MojoUkmRecorder> MojoUkmRecorder::Create(
    service_manager::Connector* connector) {
  ukm::mojom::UkmRecorderInterfacePtr interface;
  connector->BindInterface(metrics::mojom::kMetricsServiceName,
                           mojo::MakeRequest(&interface));
  return std::make_unique<MojoUkmRecorder>(std::move(interface));
}

base::WeakPtr<MojoUkmRecorder> MojoUkmRecorder::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void MojoUkmRecorder::UpdateSourceURL(SourceId source_id, const GURL& url) {
  interface_->UpdateSourceURL(source_id, url.spec());
}

void MojoUkmRecorder::AddEntry(mojom::UkmEntryPtr entry) {
  interface_->AddEntry(std::move(entry));
}

}  // namespace ukm
