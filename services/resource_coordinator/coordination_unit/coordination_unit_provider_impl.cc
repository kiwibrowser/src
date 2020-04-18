// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_provider_impl.h"

#include <memory>
#include <utility>

#include "base/macros.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/system_coordination_unit_impl.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace resource_coordinator {

CoordinationUnitProviderImpl::CoordinationUnitProviderImpl(
    service_manager::ServiceContextRefFactory* service_ref_factory,
    CoordinationUnitManager* coordination_unit_manager)
    : service_ref_factory_(service_ref_factory),
      coordination_unit_manager_(coordination_unit_manager) {
  DCHECK(service_ref_factory);
  service_ref_ = service_ref_factory->CreateRef();
}

CoordinationUnitProviderImpl::~CoordinationUnitProviderImpl() = default;

void CoordinationUnitProviderImpl::OnConnectionError(
    CoordinationUnitBase* coordination_unit) {
  coordination_unit_manager_->OnBeforeCoordinationUnitDestroyed(
      coordination_unit);
  coordination_unit->Destruct();
}

void CoordinationUnitProviderImpl::CreateFrameCoordinationUnit(
    mojom::FrameCoordinationUnitRequest request,
    const CoordinationUnitID& id) {
  FrameCoordinationUnitImpl* frame_cu =
      FrameCoordinationUnitImpl::Create(id, service_ref_factory_->CreateRef());

  frame_cu->Bind(std::move(request));
  coordination_unit_manager_->OnCoordinationUnitCreated(frame_cu);
  auto& frame_cu_binding = frame_cu->binding();

  frame_cu_binding.set_connection_error_handler(
      base::BindOnce(&CoordinationUnitProviderImpl::OnConnectionError,
                     base::Unretained(this), frame_cu));
}

void CoordinationUnitProviderImpl::CreatePageCoordinationUnit(
    mojom::PageCoordinationUnitRequest request,
    const CoordinationUnitID& id) {
  PageCoordinationUnitImpl* page_cu =
      PageCoordinationUnitImpl::Create(id, service_ref_factory_->CreateRef());

  page_cu->Bind(std::move(request));
  coordination_unit_manager_->OnCoordinationUnitCreated(page_cu);
  auto& page_cu_binding = page_cu->binding();

  page_cu_binding.set_connection_error_handler(
      base::BindOnce(&CoordinationUnitProviderImpl::OnConnectionError,
                     base::Unretained(this), page_cu));
}

void CoordinationUnitProviderImpl::CreateProcessCoordinationUnit(
    mojom::ProcessCoordinationUnitRequest request,
    const CoordinationUnitID& id) {
  ProcessCoordinationUnitImpl* process_cu = ProcessCoordinationUnitImpl::Create(
      id, service_ref_factory_->CreateRef());

  process_cu->Bind(std::move(request));
  coordination_unit_manager_->OnCoordinationUnitCreated(process_cu);
  auto& process_cu_binding = process_cu->binding();

  process_cu_binding.set_connection_error_handler(
      base::BindOnce(&CoordinationUnitProviderImpl::OnConnectionError,
                     base::Unretained(this), process_cu));
}

void CoordinationUnitProviderImpl::GetSystemCoordinationUnit(
    resource_coordinator::mojom::SystemCoordinationUnitRequest request) {
  // Simply fetch the existing SystemCU and add an additional binding to it.
  coordination_unit_manager_->system_cu()->AddBinding(std::move(request));
}

void CoordinationUnitProviderImpl::Bind(
    resource_coordinator::mojom::CoordinationUnitProviderRequest request,
    const service_manager::BindSourceInfo& source_info) {
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace resource_coordinator
