// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/model/update_model.h"

namespace ash {

UpdateModel::UpdateModel() = default;
UpdateModel::~UpdateModel() = default;

void UpdateModel::AddObserver(UpdateObserver* observer) {
  observers_.AddObserver(observer);
}

void UpdateModel::RemoveObserver(UpdateObserver* observer) {
  observers_.RemoveObserver(observer);
}

void UpdateModel::SetUpdateAvailable(mojom::UpdateSeverity severity,
                                     bool factory_reset_required,
                                     mojom::UpdateType update_type) {
  update_required_ = true;
  severity_ = severity;
  factory_reset_required_ = factory_reset_required;
  update_type_ = update_type;
  NotifyUpdateAvailable();
}

void UpdateModel::SetUpdateOverCellularAvailable(bool available) {
  update_over_cellular_available_ = available;
  NotifyUpdateAvailable();
}

mojom::UpdateSeverity UpdateModel::GetSeverity() const {
  // TODO(weidongg/691108): adjust severity according the amount of time
  // passing after update is available over cellular connection. Use low
  // severity for update available over cellular connection.
  return update_over_cellular_available_ ? mojom::UpdateSeverity::LOW
                                         : severity_;
}

void UpdateModel::NotifyUpdateAvailable() {
  for (auto& observer : observers_)
    observer.OnUpdateAvailable();
}

}  // namespace ash
