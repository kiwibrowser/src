// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/page_resource_coordinator.h"

namespace resource_coordinator {

PageResourceCoordinator::PageResourceCoordinator(
    service_manager::Connector* connector)
    : ResourceCoordinatorInterface(), weak_ptr_factory_(this) {
  CoordinationUnitID new_cu_id(CoordinationUnitType::kPage, std::string());
  ResourceCoordinatorInterface::ConnectToService(connector, new_cu_id);
}

PageResourceCoordinator::~PageResourceCoordinator() = default;

void PageResourceCoordinator::SetIsLoading(bool is_loading) {
  if (!service_)
    return;
  service_->SetIsLoading(is_loading);
}

void PageResourceCoordinator::SetVisibility(bool visible) {
  if (!service_)
    return;
  service_->SetVisibility(visible);
}

void PageResourceCoordinator::SetUKMSourceId(int64_t ukm_source_id) {
  if (!service_)
    return;
  service_->SetUKMSourceId(ukm_source_id);
}

void PageResourceCoordinator::OnFaviconUpdated() {
  if (!service_)
    return;
  service_->OnFaviconUpdated();
}

void PageResourceCoordinator::OnTitleUpdated() {
  if (!service_)
    return;
  service_->OnTitleUpdated();
}

void PageResourceCoordinator::OnMainFrameNavigationCommitted() {
  if (!service_)
    return;
  service_->OnMainFrameNavigationCommitted();
}

void PageResourceCoordinator::AddFrame(const FrameResourceCoordinator& frame) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!service_)
    return;
  // We could keep the ID around ourselves, but this hop ensures that the child
  // has been created on the service-side.
  frame.service()->GetID(base::BindOnce(&PageResourceCoordinator::AddFrameByID,
                                        weak_ptr_factory_.GetWeakPtr()));
}

void PageResourceCoordinator::RemoveFrame(
    const FrameResourceCoordinator& frame) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!service_)
    return;
  frame.service()->GetID(
      base::BindOnce(&PageResourceCoordinator::RemoveFrameByID,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PageResourceCoordinator::ConnectToService(
    mojom::CoordinationUnitProviderPtr& provider,
    const CoordinationUnitID& cu_id) {
  provider->CreatePageCoordinationUnit(mojo::MakeRequest(&service_), cu_id);
}

void PageResourceCoordinator::AddFrameByID(const CoordinationUnitID& cu_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  service_->AddFrame(cu_id);
}

void PageResourceCoordinator::RemoveFrameByID(const CoordinationUnitID& cu_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  service_->RemoveFrame(cu_id);
}

}  // namespace resource_coordinator
