// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_PUBLIC_MOJO_NOTIFICATION_STRUCT_TRAITS_H_
#define UI_MESSAGE_CENTER_PUBLIC_MOJO_NOTIFICATION_STRUCT_TRAITS_H_

#include "third_party/skia/include/core/SkColor.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/mojo/notification.mojom-shared.h"

namespace mojo {

template <>
struct EnumTraits<message_center::mojom::NotificationType,
                  message_center::NotificationType> {
  static message_center::mojom::NotificationType ToMojom(
      message_center::NotificationType type) {
    switch (type) {
      case message_center::NOTIFICATION_TYPE_SIMPLE:
        return message_center::mojom::NotificationType::SIMPLE;
      case message_center::NOTIFICATION_TYPE_BASE_FORMAT:
        return message_center::mojom::NotificationType::BASE_FORMAT;
      case message_center::NOTIFICATION_TYPE_IMAGE:
        return message_center::mojom::NotificationType::IMAGE;
      case message_center::NOTIFICATION_TYPE_MULTIPLE:
        return message_center::mojom::NotificationType::MULTIPLE;
      case message_center::NOTIFICATION_TYPE_PROGRESS:
        return message_center::mojom::NotificationType::PROGRESS;
      case message_center::NOTIFICATION_TYPE_CUSTOM:
        return message_center::mojom::NotificationType::CUSTOM;
    }
    NOTREACHED();
    return message_center::mojom::NotificationType::SIMPLE;
  }

  static bool FromMojom(message_center::mojom::NotificationType input,
                        message_center::NotificationType* out) {
    switch (input) {
      case message_center::mojom::NotificationType::SIMPLE:
        *out = message_center::NOTIFICATION_TYPE_SIMPLE;
        return true;
      case message_center::mojom::NotificationType::BASE_FORMAT:
        *out = message_center::NOTIFICATION_TYPE_BASE_FORMAT;
        return true;
      case message_center::mojom::NotificationType::IMAGE:
        *out = message_center::NOTIFICATION_TYPE_IMAGE;
        return true;
      case message_center::mojom::NotificationType::MULTIPLE:
        *out = message_center::NOTIFICATION_TYPE_MULTIPLE;
        return true;
      case message_center::mojom::NotificationType::PROGRESS:
        *out = message_center::NOTIFICATION_TYPE_PROGRESS;
        return true;
      case message_center::mojom::NotificationType::CUSTOM:
        *out = message_center::NOTIFICATION_TYPE_CUSTOM;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<message_center::mojom::SettingsButtonHandler,
                  message_center::SettingsButtonHandler> {
  static message_center::mojom::SettingsButtonHandler ToMojom(
      message_center::SettingsButtonHandler type) {
    switch (type) {
      case message_center::SettingsButtonHandler::NONE:
        return message_center::mojom::SettingsButtonHandler::NONE;
      case message_center::SettingsButtonHandler::INLINE:
        return message_center::mojom::SettingsButtonHandler::INLINE;
      case message_center::SettingsButtonHandler::DELEGATE:
        return message_center::mojom::SettingsButtonHandler::DELEGATE;
    }
    NOTREACHED();
    return message_center::mojom::SettingsButtonHandler::NONE;
  }

  static bool FromMojom(message_center::mojom::SettingsButtonHandler input,
                        message_center::SettingsButtonHandler* out) {
    switch (input) {
      case message_center::mojom::SettingsButtonHandler::NONE:
        *out = message_center::SettingsButtonHandler::NONE;
        return true;
      case message_center::mojom::SettingsButtonHandler::INLINE:
        *out = message_center::SettingsButtonHandler::INLINE;
        return true;
      case message_center::mojom::SettingsButtonHandler::DELEGATE:
        *out = message_center::SettingsButtonHandler::DELEGATE;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<message_center::mojom::FullscreenVisibility,
                  message_center::FullscreenVisibility> {
  static message_center::mojom::FullscreenVisibility ToMojom(
      message_center::FullscreenVisibility type) {
    switch (type) {
      case message_center::FullscreenVisibility::NONE:
        return message_center::mojom::FullscreenVisibility::NONE;
      case message_center::FullscreenVisibility::OVER_USER:
        return message_center::mojom::FullscreenVisibility::OVER_USER;
    }
    NOTREACHED();
    return message_center::mojom::FullscreenVisibility::NONE;
  }

  static bool FromMojom(message_center::mojom::FullscreenVisibility input,
                        message_center::FullscreenVisibility* out) {
    switch (input) {
      case message_center::mojom::FullscreenVisibility::NONE:
        *out = message_center::FullscreenVisibility::NONE;
        return true;
      case message_center::mojom::FullscreenVisibility::OVER_USER:
        *out = message_center::FullscreenVisibility::OVER_USER;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct StructTraits<message_center::mojom::NotificationItemDataView,
                    message_center::NotificationItem> {
  static const base::string16& title(
      const message_center::NotificationItem& n) {
    return n.title;
  }
  static const base::string16& message(
      const message_center::NotificationItem& n) {
    return n.message;
  }
  static bool Read(message_center::mojom::NotificationItemDataView data,
                   message_center::NotificationItem* out);
};

template <>
struct StructTraits<message_center::mojom::ButtonInfoDataView,
                    message_center::ButtonInfo> {
  static const base::string16& title(const message_center::ButtonInfo& b) {
    return b.title;
  }
  static gfx::ImageSkia icon(const message_center::ButtonInfo& b);
  static const base::Optional<base::string16>& placeholder(
      const message_center::ButtonInfo& b) {
    return b.placeholder;
  }
  static bool Read(message_center::mojom::ButtonInfoDataView data,
                   message_center::ButtonInfo* out);
};

template <>
struct StructTraits<message_center::mojom::RichNotificationDataDataView,
                    message_center::RichNotificationData> {
  static int priority(const message_center::RichNotificationData& r);
  static bool never_time_out(const message_center::RichNotificationData& r);
  static const base::Time& timestamp(
      const message_center::RichNotificationData& r);
  static gfx::ImageSkia image(const message_center::RichNotificationData& r);
  static gfx::ImageSkia small_image(
      const message_center::RichNotificationData& r);
  static const std::vector<message_center::NotificationItem>& items(
      const message_center::RichNotificationData& r);
  static int progress(const message_center::RichNotificationData& r);
  static const base::string16& progress_status(
      const message_center::RichNotificationData& r);
  static const std::vector<message_center::ButtonInfo>& buttons(
      const message_center::RichNotificationData& r);
  static bool should_make_spoken_feedback_for_popup_updates(
      const message_center::RichNotificationData& r);
  static bool pinned(const message_center::RichNotificationData& r);
  static bool renotify(const message_center::RichNotificationData& r);
  static const base::string16& accessible_name(
      const message_center::RichNotificationData& r);
  static std::string vector_small_image_id(
      const message_center::RichNotificationData& r);
  static SkColor accent_color(const message_center::RichNotificationData& r);
  static message_center::SettingsButtonHandler settings_button_handler(
      const message_center::RichNotificationData& r);
  static message_center::FullscreenVisibility fullscreen_visibility(
      const message_center::RichNotificationData& r);
  static bool Read(message_center::mojom::RichNotificationDataDataView data,
                   message_center::RichNotificationData* out);
};

template <>
struct StructTraits<message_center::mojom::NotificationDataView,
                    message_center::Notification> {
  static message_center::NotificationType type(
      const message_center::Notification& n);
  static const std::string& id(const message_center::Notification& n);
  static const base::string16& title(const message_center::Notification& n);
  static const base::string16& message(const message_center::Notification& n);
  static gfx::ImageSkia icon(const message_center::Notification& n);
  static const base::string16& display_source(
      const message_center::Notification& n);
  static const GURL& origin_url(const message_center::Notification& n);
  static const message_center::NotifierId& notifier_id(
      const message_center::Notification& n);
  static const message_center::RichNotificationData& optional_fields(
      const message_center::Notification& n);
  static bool Read(message_center::mojom::NotificationDataView data,
                   message_center::Notification* out);
};

}  // namespace mojo

#endif  // UI_MESSAGE_CENTER_PUBLIC_MOJO_NOTIFICATION_STRUCT_TRAITS_H_
