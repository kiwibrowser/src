// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/public/cpp/notification.h"

#include <map>
#include <memory>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "ui/message_center/public/cpp/notification_types.h"

namespace message_center {

namespace {

unsigned g_next_serial_number = 0;

base::LazyInstance<std::map<std::string, const gfx::VectorIcon&>>::Leaky
    g_vector_icon_registry = LAZY_INSTANCE_INITIALIZER;

const gfx::ImageSkia CreateSolidColorImage(int width,
                                           int height,
                                           SkColor color) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(width, height);
  bitmap.eraseColor(color);
  return gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
}

gfx::Image DeepCopyImage(const gfx::Image& image) {
  if (image.IsEmpty())
    return gfx::Image();
  std::unique_ptr<gfx::ImageSkia> image_skia(image.CopyImageSkia());
  return gfx::Image(*image_skia);
}

}  // namespace

ButtonInfo::ButtonInfo(const base::string16& title) : title(title) {}

ButtonInfo::ButtonInfo(const ButtonInfo& other) = default;

ButtonInfo::ButtonInfo() = default;

ButtonInfo::~ButtonInfo() = default;

ButtonInfo& ButtonInfo::operator=(const ButtonInfo& other) = default;

RichNotificationData::RichNotificationData() : timestamp(base::Time::Now()) {}

RichNotificationData::RichNotificationData(const RichNotificationData& other) =
    default;

RichNotificationData::~RichNotificationData() = default;

Notification::Notification() : serial_number_(g_next_serial_number++) {}

Notification::Notification(NotificationType type,
                           const std::string& id,
                           const base::string16& title,
                           const base::string16& message,
                           const gfx::Image& icon,
                           const base::string16& display_source,
                           const GURL& origin_url,
                           const NotifierId& notifier_id,
                           const RichNotificationData& optional_fields,
                           scoped_refptr<NotificationDelegate> delegate)
    : type_(type),
      id_(id),
      title_(title),
      message_(message),
      icon_(icon),
      display_source_(display_source),
      origin_url_(origin_url),
      notifier_id_(notifier_id),
      optional_fields_(optional_fields),
      serial_number_(g_next_serial_number++),
      delegate_(std::move(delegate)) {}

Notification::Notification(const std::string& id, const Notification& other)
    : Notification(other) {
  id_ = id;
}

Notification::Notification(const Notification& other) = default;

Notification& Notification::operator=(const Notification& other) = default;

Notification::~Notification() = default;

// static
std::unique_ptr<Notification> Notification::DeepCopy(
    const Notification& notification,
    bool include_body_image,
    bool include_small_image,
    bool include_icon_images) {
  std::unique_ptr<Notification> notification_copy =
      std::make_unique<Notification>(notification);
  notification_copy->set_icon(DeepCopyImage(notification_copy->icon()));
  notification_copy->set_image(include_body_image
                                   ? DeepCopyImage(notification_copy->image())
                                   : gfx::Image());
  notification_copy->set_small_image(
      include_small_image ? notification_copy->small_image() : gfx::Image());
  for (size_t i = 0; i < notification_copy->buttons().size(); i++) {
    notification_copy->SetButtonIcon(
        i, include_icon_images
               ? DeepCopyImage(notification_copy->buttons()[i].icon)
               : gfx::Image());
  }
  return notification_copy;
}

void Notification::SetButtonIcon(size_t index, const gfx::Image& icon) {
  if (index >= optional_fields_.buttons.size())
    return;
  optional_fields_.buttons[index].icon = icon;
}

void Notification::SetSystemPriority() {
  optional_fields_.priority = SYSTEM_PRIORITY;
  optional_fields_.never_timeout = true;
}

bool Notification::UseOriginAsContextMessage() const {
  return optional_fields_.context_message.empty() && origin_url_.is_valid() &&
         origin_url_.SchemeIsHTTPOrHTTPS();
}

gfx::Image Notification::GenerateMaskedSmallIcon(int dip_size,
                                                 SkColor color) const {
  if (!vector_small_image().is_empty())
    return gfx::Image(
        gfx::CreateVectorIcon(vector_small_image(), dip_size, color));

  if (small_image().IsEmpty())
    return gfx::Image();

  // If |vector_small_image| is not available, fallback to raster based
  // masking and resizing.
  gfx::ImageSkia image = small_image().AsImageSkia();
  gfx::ImageSkia masked = gfx::ImageSkiaOperations::CreateMaskedImage(
      CreateSolidColorImage(image.width(), image.height(), color), image);
  gfx::ImageSkia resized = gfx::ImageSkiaOperations::CreateResizedImage(
      masked, skia::ImageOperations::ResizeMethod::RESIZE_BEST,
      gfx::Size(dip_size, dip_size));
  return gfx::Image(resized);
}

// static
std::unique_ptr<Notification> Notification::CreateSystemNotification(
    const std::string& notification_id,
    const base::string16& title,
    const base::string16& message,
    const gfx::Image& icon,
    const std::string& system_component_id,
    const base::RepeatingClosure& click_callback) {
  DCHECK(!click_callback.is_null());
  std::unique_ptr<Notification> notification = CreateSystemNotification(
      NOTIFICATION_TYPE_SIMPLE, notification_id, title, message, icon,
      base::string16() /* display_source */, GURL(),
      NotifierId(NotifierId::SYSTEM_COMPONENT, system_component_id),
      RichNotificationData(),
      new HandleNotificationClickDelegate(click_callback), gfx::kNoneIcon,
      SystemNotificationWarningLevel::CRITICAL_WARNING);
  notification->SetSystemPriority();
  return notification;
}

// static
std::unique_ptr<Notification> Notification::CreateSystemNotification(
    NotificationType type,
    const std::string& id,
    const base::string16& title,
    const base::string16& message,
    const gfx::Image& icon,
    const base::string16& display_source,
    const GURL& origin_url,
    const NotifierId& notifier_id,
    const RichNotificationData& optional_fields,
    scoped_refptr<NotificationDelegate> delegate,
    const gfx::VectorIcon& small_image,
    SystemNotificationWarningLevel color_type) {
  DCHECK_EQ(NotifierId::SYSTEM_COMPONENT, notifier_id.type);
  SkColor color = kSystemNotificationColorNormal;
  switch (color_type) {
    case SystemNotificationWarningLevel::NORMAL:
      color = kSystemNotificationColorNormal;
      break;
    case SystemNotificationWarningLevel::WARNING:
      color = kSystemNotificationColorWarning;
      break;
    case SystemNotificationWarningLevel::CRITICAL_WARNING:
      color = kSystemNotificationColorCriticalWarning;
      break;
  }
  std::unique_ptr<Notification> notification = std::make_unique<Notification>(
      type, id, title, message, icon, display_source, origin_url, notifier_id,
      optional_fields, delegate);
  notification->set_accent_color(color);
  if (!small_image.is_empty())
    notification->set_vector_small_image(small_image);
  return notification;
}

// static
void RegisterVectorIcons(
    const std::vector<const gfx::VectorIcon*>& vector_icons) {
  for (const gfx::VectorIcon* icon : vector_icons) {
    g_vector_icon_registry.Get().insert(
        std::pair<std::string, const gfx::VectorIcon&>(icon->name, *icon));
  }
}

// static
const gfx::VectorIcon* GetRegisteredVectorIcon(const std::string& id) {
  auto iter = g_vector_icon_registry.Get().find(id);
  return iter != g_vector_icon_registry.Get().end() ? &iter->second : nullptr;
}

}  // namespace message_center
