// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UPDATE_UPDATE_NOTIFICATION_CONTROLLER_H_
#define ASH_SYSTEM_UPDATE_UPDATE_NOTIFICATION_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/system/model/update_model.h"
#include "base/macros.h"

namespace ash {

// Controller class to manage update notification.
class ASH_EXPORT UpdateNotificationController : public UpdateObserver {
 public:
  UpdateNotificationController();
  ~UpdateNotificationController() override;

  // UpdateObserver:
  void OnUpdateAvailable() override;

 private:
  friend class UpdateNotificationControllerTest;

  bool ShouldShowUpdate() const;
  base::string16 GetNotificationTitle() const;
  void HandleNotificationClick();

  static const char kNotificationId[];

  UpdateModel* const model_;

  base::WeakPtrFactory<UpdateNotificationController> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(UpdateNotificationController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UPDATE_UPDATE_NOTIFICATION_CONTROLLER_H_
