// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_INTERFACES_USER_INFO_TRAITS_H_
#define ASH_PUBLIC_INTERFACES_USER_INFO_TRAITS_H_

#include "ash/public/interfaces/user_info.mojom.h"
#include "components/user_manager/user_type.h"

namespace mojo {

template <>
struct EnumTraits<ash::mojom::UserType, user_manager::UserType> {
  static ash::mojom::UserType ToMojom(user_manager::UserType input) {
    switch (input) {
      case user_manager::USER_TYPE_REGULAR:
        return ash::mojom::UserType::REGULAR;
      case user_manager::USER_TYPE_GUEST:
        return ash::mojom::UserType::GUEST;
      case user_manager::USER_TYPE_PUBLIC_ACCOUNT:
        return ash::mojom::UserType::PUBLIC_ACCOUNT;
      case user_manager::USER_TYPE_SUPERVISED:
        return ash::mojom::UserType::SUPERVISED;
      case user_manager::USER_TYPE_KIOSK_APP:
        return ash::mojom::UserType::KIOSK;
      case user_manager::USER_TYPE_CHILD:
        return ash::mojom::UserType::CHILD;
      case user_manager::USER_TYPE_ARC_KIOSK_APP:
        return ash::mojom::UserType::ARC_KIOSK;
      case user_manager::USER_TYPE_ACTIVE_DIRECTORY:
        return ash::mojom::UserType::ACTIVE_DIRECTORY;
      case user_manager::NUM_USER_TYPES:
        // Bail as this is not a valid user type.
        break;
    }
    NOTREACHED();
    return ash::mojom::UserType::REGULAR;
  }

  static bool FromMojom(ash::mojom::UserType input,
                        user_manager::UserType* out) {
    switch (input) {
      case ash::mojom::UserType::REGULAR:
        *out = user_manager::USER_TYPE_REGULAR;
        return true;
      case ash::mojom::UserType::GUEST:
        *out = user_manager::USER_TYPE_GUEST;
        return true;
      case ash::mojom::UserType::PUBLIC_ACCOUNT:
        *out = user_manager::USER_TYPE_PUBLIC_ACCOUNT;
        return true;
      case ash::mojom::UserType::SUPERVISED:
        *out = user_manager::USER_TYPE_SUPERVISED;
        return true;
      case ash::mojom::UserType::KIOSK:
        *out = user_manager::USER_TYPE_KIOSK_APP;
        return true;
      case ash::mojom::UserType::CHILD:
        *out = user_manager::USER_TYPE_CHILD;
        return true;
      case ash::mojom::UserType::ARC_KIOSK:
        *out = user_manager::USER_TYPE_ARC_KIOSK_APP;
        return true;
      case ash::mojom::UserType::ACTIVE_DIRECTORY:
        *out = user_manager::USER_TYPE_ACTIVE_DIRECTORY;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // ASH_PUBLIC_INTERFACES_USER_INFO_TRAITS_H_
