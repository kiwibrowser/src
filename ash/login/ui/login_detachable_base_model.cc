// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_detachable_base_model.h"

#include "ash/detachable_base/detachable_base_handler.h"
#include "ash/detachable_base/detachable_base_observer.h"
#include "ash/detachable_base/detachable_base_pairing_status.h"
#include "ash/login/ui/login_data_dispatcher.h"
#include "ash/public/interfaces/user_info.mojom.h"
#include "base/macros.h"
#include "base/scoped_observer.h"

namespace ash {

namespace {

class LoginDetachableBaseModelImpl : public LoginDetachableBaseModel,
                                     public DetachableBaseObserver {
 public:
  LoginDetachableBaseModelImpl(DetachableBaseHandler* detachable_base_handler,
                               LoginDataDispatcher* login_data_dispatcher)
      : detachable_base_handler_(detachable_base_handler),
        detachable_base_observer_(this),
        login_data_dispatcher_(login_data_dispatcher) {
    detachable_base_observer_.Add(detachable_base_handler);
  }

  ~LoginDetachableBaseModelImpl() override = default;

  // LoginDetachableBaseModel:
  DetachableBasePairingStatus GetPairingStatus() override {
    return detachable_base_handler_->GetPairingStatus();
  }
  bool PairedBaseMatchesLastUsedByUser(
      const mojom::UserInfo& user_info) override {
    return detachable_base_handler_->PairedBaseMatchesLastUsedByUser(user_info);
  }
  bool SetPairedBaseAsLastUsedByUser(
      const mojom::UserInfo& user_info) override {
    return detachable_base_handler_->SetPairedBaseAsLastUsedByUser(user_info);
  }

  // DetachableBaseObserver:
  void OnDetachableBasePairingStatusChanged(
      DetachableBasePairingStatus pairing_status) override {
    login_data_dispatcher_->SetDetachableBasePairingStatus(pairing_status);
  }
  void OnDetachableBaseRequiresUpdateChanged(bool requires_update) override {}

 private:
  DetachableBaseHandler* detachable_base_handler_;
  ScopedObserver<DetachableBaseHandler, DetachableBaseObserver>
      detachable_base_observer_;
  LoginDataDispatcher* login_data_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(LoginDetachableBaseModelImpl);
};

}  // namespace

// static
std::unique_ptr<LoginDetachableBaseModel> LoginDetachableBaseModel::Create(
    DetachableBaseHandler* detachable_base_handler,
    LoginDataDispatcher* login_data_dispatcher) {
  return std::make_unique<LoginDetachableBaseModelImpl>(detachable_base_handler,
                                                        login_data_dispatcher);
}

}  // namespace ash
