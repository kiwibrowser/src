// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_MODEL_UPDATE_MODEL_H_
#define ASH_SYSTEM_MODEL_UPDATE_MODEL_H_

#include "ash/public/interfaces/update.mojom.h"
#include "base/macros.h"
#include "base/observer_list.h"

namespace ash {

class UpdateObserver {
 public:
  virtual ~UpdateObserver() {}

  virtual void OnUpdateAvailable() = 0;
};

// Model to store system update availability.
class UpdateModel {
 public:
  UpdateModel();
  ~UpdateModel();

  void AddObserver(UpdateObserver* observer);
  void RemoveObserver(UpdateObserver* observer);

  // Store the state that a software update is available. The state persists
  // until reboot. Based on |severity| and |factory_reset_required|, the
  // observer views can indicate the severity of the update to users by changing
  // the icon, color, and tooltip.
  void SetUpdateAvailable(mojom::UpdateSeverity severity,
                          bool factory_reset_required,
                          mojom::UpdateType update_type);

  // If |available| is true, a software update is available but user's agreement
  // is required as current connection is cellular. If |available| is false, the
  // user's one time permission on update over cellular connection has been
  // granted.
  void SetUpdateOverCellularAvailable(bool available);

  mojom::UpdateSeverity GetSeverity() const;

  bool update_required() const { return update_required_; }
  bool factory_reset_required() const { return factory_reset_required_; }
  mojom::UpdateType update_type() const { return update_type_; }
  bool update_over_cellular_available() const {
    return update_over_cellular_available_;
  }

 private:
  void NotifyUpdateAvailable();

  bool update_required_ = false;
  mojom::UpdateSeverity severity_ = mojom::UpdateSeverity::NONE;
  bool factory_reset_required_ = false;
  mojom::UpdateType update_type_ = mojom::UpdateType::SYSTEM;
  bool update_over_cellular_available_ = false;

  base::ObserverList<UpdateObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(UpdateModel);
};

}  // namespace ash

#endif  // ASH_SYSTEM_MODEL_UPDATE_MODEL_H_
