// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/model/enterprise_domain_model.h"

#include "ash/system/enterprise/enterprise_domain_observer.h"

namespace ash {

EnterpriseDomainModel::EnterpriseDomainModel() {}

EnterpriseDomainModel::~EnterpriseDomainModel() = default;

void EnterpriseDomainModel::AddObserver(EnterpriseDomainObserver* observer) {
  observers_.AddObserver(observer);
}

void EnterpriseDomainModel::RemoveObserver(EnterpriseDomainObserver* observer) {
  observers_.RemoveObserver(observer);
}

void EnterpriseDomainModel::SetEnterpriseDisplayDomain(
    const std::string& enterprise_display_domain,
    bool active_directory_managed) {
  enterprise_display_domain_ = enterprise_display_domain;
  active_directory_managed_ = active_directory_managed;
  NotifyChanged();
}

void EnterpriseDomainModel::NotifyChanged() {
  for (auto& observer : observers_)
    observer.OnEnterpriseDomainChanged();
}

}  // namespace ash
