// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_MODEL_ENTERPRISE_DOMAIN_MODEL_H_
#define ASH_SYSTEM_MODEL_ENTERPRISE_DOMAIN_MODEL_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/observer_list.h"

namespace ash {

class EnterpriseDomainObserver;

// Model to store enterprise enrollment state.
class ASH_EXPORT EnterpriseDomainModel {
 public:
  EnterpriseDomainModel();
  ~EnterpriseDomainModel();

  void AddObserver(EnterpriseDomainObserver* observer);
  void RemoveObserver(EnterpriseDomainObserver* observer);

  void SetEnterpriseDisplayDomain(const std::string& enterprise_display_domain,
                                  bool active_directory_managed);

  const std::string& enterprise_display_domain() const {
    return enterprise_display_domain_;
  }
  bool active_directory_managed() const { return active_directory_managed_; }

 private:
  void NotifyChanged();

  // The domain name of the organization that manages the device. Empty if the
  // device is not enterprise enrolled or if it uses Active Directory.
  std::string enterprise_display_domain_;

  // Whether this is an Active Directory managed enterprise device.
  bool active_directory_managed_ = false;

  base::ObserverList<EnterpriseDomainObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(EnterpriseDomainModel);
};

}  // namespace ash

#endif  // ASH_SYSTEM_MODEL_ENTERPRISE_DOMAIN_MODEL_H_
