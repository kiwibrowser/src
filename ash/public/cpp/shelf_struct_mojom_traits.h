// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_SHELF_STRUCT_MOJOM_TRAITS_H_
#define ASH_PUBLIC_CPP_SHELF_STRUCT_MOJOM_TRAITS_H_

#include <string>

#include "ash/public/cpp/ash_public_export.h"
#include "ash/public/cpp/shelf_item.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/public/interfaces/shelf.mojom-shared.h"

using ash::ShelfItem;

namespace mojo {

template <>
struct EnumTraits<ash::mojom::ShelfAction, ash::ShelfAction> {
  static ash::mojom::ShelfAction ToMojom(ash::ShelfAction input) {
    switch (input) {
      case ash::SHELF_ACTION_NONE:
        return ash::mojom::ShelfAction::NONE;
      case ash::SHELF_ACTION_NEW_WINDOW_CREATED:
        return ash::mojom::ShelfAction::WINDOW_CREATED;
      case ash::SHELF_ACTION_WINDOW_ACTIVATED:
        return ash::mojom::ShelfAction::WINDOW_ACTIVATED;
      case ash::SHELF_ACTION_WINDOW_MINIMIZED:
        return ash::mojom::ShelfAction::WINDOW_MINIMIZED;
      case ash::SHELF_ACTION_APP_LIST_SHOWN:
        return ash::mojom::ShelfAction::APP_LIST_SHOWN;
    }
    NOTREACHED();
    return ash::mojom::ShelfAction::NONE;
  }

  static bool FromMojom(ash::mojom::ShelfAction input, ash::ShelfAction* out) {
    switch (input) {
      case ash::mojom::ShelfAction::NONE:
        *out = ash::SHELF_ACTION_NONE;
        return true;
      case ash::mojom::ShelfAction::WINDOW_CREATED:
        *out = ash::SHELF_ACTION_NEW_WINDOW_CREATED;
        return true;
      case ash::mojom::ShelfAction::WINDOW_ACTIVATED:
        *out = ash::SHELF_ACTION_WINDOW_ACTIVATED;
        return true;
      case ash::mojom::ShelfAction::WINDOW_MINIMIZED:
        *out = ash::SHELF_ACTION_WINDOW_MINIMIZED;
        return true;
      case ash::mojom::ShelfAction::APP_LIST_SHOWN:
        *out = ash::SHELF_ACTION_APP_LIST_SHOWN;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<ash::mojom::ShelfItemStatus, ash::ShelfItemStatus> {
  static ash::mojom::ShelfItemStatus ToMojom(ash::ShelfItemStatus input) {
    switch (input) {
      case ash::STATUS_CLOSED:
        return ash::mojom::ShelfItemStatus::CLOSED;
      case ash::STATUS_RUNNING:
        return ash::mojom::ShelfItemStatus::RUNNING;
      case ash::STATUS_ATTENTION:
        return ash::mojom::ShelfItemStatus::ATTENTION;
    }
    NOTREACHED();
    return ash::mojom::ShelfItemStatus::CLOSED;
  }

  static bool FromMojom(ash::mojom::ShelfItemStatus input,
                        ash::ShelfItemStatus* out) {
    switch (input) {
      case ash::mojom::ShelfItemStatus::CLOSED:
        *out = ash::STATUS_CLOSED;
        return true;
      case ash::mojom::ShelfItemStatus::RUNNING:
        *out = ash::STATUS_RUNNING;
        return true;
      case ash::mojom::ShelfItemStatus::ATTENTION:
        *out = ash::STATUS_ATTENTION;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<ash::mojom::ShelfItemType, ash::ShelfItemType> {
  static ash::mojom::ShelfItemType ToMojom(ash::ShelfItemType input) {
    switch (input) {
      case ash::TYPE_APP_PANEL:
        return ash::mojom::ShelfItemType::PANEL;
      case ash::TYPE_PINNED_APP:
        return ash::mojom::ShelfItemType::PINNED_APP;
      case ash::TYPE_APP_LIST:
        return ash::mojom::ShelfItemType::APP_LIST;
      case ash::TYPE_BROWSER_SHORTCUT:
        return ash::mojom::ShelfItemType::BROWSER;
      case ash::TYPE_APP:
        return ash::mojom::ShelfItemType::APP;
      case ash::TYPE_DIALOG:
        return ash::mojom::ShelfItemType::DIALOG;
      case ash::TYPE_BACK_BUTTON:
        return ash::mojom::ShelfItemType::BACK_BUTTON;
      case ash::TYPE_UNDEFINED:
        return ash::mojom::ShelfItemType::UNDEFINED;
    }
    NOTREACHED();
    return ash::mojom::ShelfItemType::UNDEFINED;
  }

  static bool FromMojom(ash::mojom::ShelfItemType input,
                        ash::ShelfItemType* out) {
    switch (input) {
      case ash::mojom::ShelfItemType::PANEL:
        *out = ash::TYPE_APP_PANEL;
        return true;
      case ash::mojom::ShelfItemType::PINNED_APP:
        *out = ash::TYPE_PINNED_APP;
        return true;
      case ash::mojom::ShelfItemType::APP_LIST:
        *out = ash::TYPE_APP_LIST;
        return true;
      case ash::mojom::ShelfItemType::BROWSER:
        *out = ash::TYPE_BROWSER_SHORTCUT;
        return true;
      case ash::mojom::ShelfItemType::APP:
        *out = ash::TYPE_APP;
        return true;
      case ash::mojom::ShelfItemType::DIALOG:
        *out = ash::TYPE_DIALOG;
        return true;
      case ash::mojom::ShelfItemType::BACK_BUTTON:
        *out = ash::TYPE_BACK_BUTTON;
        return true;
      case ash::mojom::ShelfItemType::UNDEFINED:
        *out = ash::TYPE_UNDEFINED;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<ash::mojom::ShelfLaunchSource, ash::ShelfLaunchSource> {
  static ash::mojom::ShelfLaunchSource ToMojom(ash::ShelfLaunchSource input) {
    switch (input) {
      case ash::LAUNCH_FROM_UNKNOWN:
        return ash::mojom::ShelfLaunchSource::UNKNOWN;
      case ash::LAUNCH_FROM_APP_LIST:
        return ash::mojom::ShelfLaunchSource::APP_LIST;
      case ash::LAUNCH_FROM_APP_LIST_SEARCH:
        return ash::mojom::ShelfLaunchSource::APP_LIST_SEARCH;
    }
    NOTREACHED();
    return ash::mojom::ShelfLaunchSource::UNKNOWN;
  }

  static bool FromMojom(ash::mojom::ShelfLaunchSource input,
                        ash::ShelfLaunchSource* out) {
    switch (input) {
      case ash::mojom::ShelfLaunchSource::UNKNOWN:
        *out = ash::LAUNCH_FROM_UNKNOWN;
        return true;
      case ash::mojom::ShelfLaunchSource::APP_LIST:
        *out = ash::LAUNCH_FROM_APP_LIST;
        return true;
      case ash::mojom::ShelfLaunchSource::APP_LIST_SEARCH:
        *out = ash::LAUNCH_FROM_APP_LIST_SEARCH;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct ASH_PUBLIC_EXPORT
    StructTraits<ash::mojom::ShelfIDDataView, ash::ShelfID> {
  static const std::string& app_id(const ash::ShelfID& i) { return i.app_id; }
  static const std::string& launch_id(const ash::ShelfID& i) {
    return i.launch_id;
  }
  static bool Read(ash::mojom::ShelfIDDataView data, ash::ShelfID* out);
};

template <>
struct ASH_PUBLIC_EXPORT
    StructTraits<ash::mojom::ShelfItemDataView, ShelfItem> {
  static ash::ShelfItemType type(const ShelfItem& i) { return i.type; }
  static const gfx::ImageSkia& image(const ShelfItem& i) { return i.image; }
  static ash::ShelfItemStatus status(const ShelfItem& i) { return i.status; }
  static const ash::ShelfID& shelf_id(const ShelfItem& i) { return i.id; }
  static const base::string16& title(const ShelfItem& i) { return i.title; }
  static bool shows_tooltip(const ShelfItem& i) { return i.shows_tooltip; }
  static bool pinned_by_policy(const ShelfItem& i) {
    return i.pinned_by_policy;
  }

  static bool Read(ash::mojom::ShelfItemDataView data, ShelfItem* out);
};

}  // namespace mojo

#endif  // ASH_PUBLIC_CPP_SHELF_STRUCT_MOJOM_TRAITS_H_
