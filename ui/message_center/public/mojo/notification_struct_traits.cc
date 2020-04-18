// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/public/mojo/notification_struct_traits.h"

#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "ui/gfx/image/mojo/image_skia_struct_traits.h"
#include "ui/message_center/public/mojo/notifier_id_struct_traits.h"
#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

using message_center::mojom::NotificationDataView;
using message_center::mojom::RichNotificationDataDataView;
using message_center::NotificationItem;
using message_center::ButtonInfo;
using message_center::Notification;
using message_center::RichNotificationData;
using NotificationItemStructTraits =
    StructTraits<message_center::mojom::NotificationItemDataView,
                 NotificationItem>;
using ButtonInfoStructTraits =
    StructTraits<message_center::mojom::ButtonInfoDataView, ButtonInfo>;
using RichNotificationDataStructTraits =
    StructTraits<RichNotificationDataDataView, RichNotificationData>;
using NotificationStructTraits =
    StructTraits<NotificationDataView, Notification>;

// static
bool NotificationItemStructTraits::Read(
    message_center::mojom::NotificationItemDataView data,
    message_center::NotificationItem* out) {
  return data.ReadTitle(&out->title) && data.ReadMessage(&out->message);
}

// static
gfx::ImageSkia ButtonInfoStructTraits::icon(
    const message_center::ButtonInfo& b) {
  return b.icon.AsImageSkia();
}

// static
bool ButtonInfoStructTraits::Read(
    message_center::mojom::ButtonInfoDataView data,
    ButtonInfo* out) {
  gfx::ImageSkia icon;
  if (!data.ReadIcon(&icon))
    return false;
  out->icon = gfx::Image(icon);
  return data.ReadTitle(&out->title) && data.ReadPlaceholder(&out->placeholder);
}

// static
int RichNotificationDataStructTraits::priority(const RichNotificationData& r) {
  return r.priority;
}

// static
bool RichNotificationDataStructTraits::never_time_out(
    const RichNotificationData& r) {
  return r.never_timeout;
}

// static
const base::Time& RichNotificationDataStructTraits::timestamp(
    const RichNotificationData& r) {
  return r.timestamp;
}

// static
gfx::ImageSkia RichNotificationDataStructTraits::image(
    const RichNotificationData& r) {
  return r.image.AsImageSkia();
}

//  static
gfx::ImageSkia RichNotificationDataStructTraits::small_image(
    const RichNotificationData& r) {
  return r.small_image.AsImageSkia();
}

// static
const std::vector<NotificationItem>& RichNotificationDataStructTraits::items(
    const RichNotificationData& r) {
  return r.items;
}

// static
int RichNotificationDataStructTraits::progress(const RichNotificationData& r) {
  return r.progress;
}

// static
const base::string16& RichNotificationDataStructTraits::progress_status(
    const RichNotificationData& r) {
  return r.progress_status;
}

// static
const std::vector<ButtonInfo>& RichNotificationDataStructTraits::buttons(
    const RichNotificationData& r) {
  return r.buttons;
}

// static
bool RichNotificationDataStructTraits::
    should_make_spoken_feedback_for_popup_updates(
        const RichNotificationData& r) {
  return r.should_make_spoken_feedback_for_popup_updates;
}

// static
bool RichNotificationDataStructTraits::pinned(const RichNotificationData& r) {
  return r.pinned;
}

// static
bool RichNotificationDataStructTraits::renotify(const RichNotificationData& r) {
  return r.renotify;
}

// static
const base::string16& RichNotificationDataStructTraits::accessible_name(
    const RichNotificationData& r) {
  return r.accessible_name;
}

// static
std::string RichNotificationDataStructTraits::vector_small_image_id(
    const message_center::RichNotificationData& r) {
  if (r.vector_small_image && r.vector_small_image->name)
    return r.vector_small_image->name;
  return std::string();
}

// static
SkColor RichNotificationDataStructTraits::accent_color(
    const RichNotificationData& r) {
  return r.accent_color;
}

// static
message_center::SettingsButtonHandler
RichNotificationDataStructTraits::settings_button_handler(
    const RichNotificationData& r) {
  return r.settings_button_handler;
}

//  static
message_center::FullscreenVisibility
RichNotificationDataStructTraits::fullscreen_visibility(
    const RichNotificationData& r) {
  return r.fullscreen_visibility;
}

// static
bool RichNotificationDataStructTraits::Read(RichNotificationDataDataView data,
                                            RichNotificationData* out) {
  out->priority = data.priority();
  out->never_timeout = data.never_time_out();
  gfx::ImageSkia image, small_image;
  if (!data.ReadImage(&image))
    return false;
  out->image = gfx::Image(image);
  if (!data.ReadSmallImage(&small_image))
    return false;
  out->small_image = gfx::Image(small_image);
  out->progress = data.progress();
  out->should_make_spoken_feedback_for_popup_updates =
      data.should_make_spoken_feedback_for_popup_updates();
  out->pinned = data.pinned();
  out->renotify = data.renotify();

  // Look up the vector icon by ID. This will only work if RegisterVectorIcon
  // has been called with an appropriate icon.
  std::string icon_id;
  if (data.ReadVectorSmallImageId(&icon_id) && !icon_id.empty()) {
    out->vector_small_image = message_center::GetRegisteredVectorIcon(icon_id);
    if (!out->vector_small_image) {
      LOG(ERROR) << "Couldn't find icon: " + icon_id;
      return false;
    }
  }

  out->accent_color = data.accent_color();
  return data.ReadTimestamp(&out->timestamp) && data.ReadItems(&out->items) &&
         data.ReadButtons(&out->buttons) &&
         data.ReadProgressStatus(&out->progress_status) &&
         data.ReadAccessibleName(&out->accessible_name) &&
         EnumTraits<message_center::mojom::SettingsButtonHandler,
                    message_center::SettingsButtonHandler>::
             FromMojom(data.settings_button_handler(),
                       &out->settings_button_handler) &&
         EnumTraits<message_center::mojom::FullscreenVisibility,
                    message_center::FullscreenVisibility>::
             FromMojom(data.fullscreen_visibility(),
                       &out->fullscreen_visibility);
}

// static
message_center::NotificationType NotificationStructTraits::type(
    const Notification& n) {
  return n.type();
}

// static
const std::string& NotificationStructTraits::id(const Notification& n) {
  return n.id();
}

// static
const base::string16& NotificationStructTraits::title(const Notification& n) {
  return n.title();
}

// static
const base::string16& NotificationStructTraits::message(const Notification& n) {
  return n.message();
}

// static
gfx::ImageSkia NotificationStructTraits::icon(const Notification& n) {
  return n.icon().AsImageSkia();
}

// static
const base::string16& NotificationStructTraits::display_source(
    const Notification& n) {
  return n.display_source();
}

// static
const GURL& NotificationStructTraits::origin_url(const Notification& n) {
  return n.origin_url();
}

// static
const message_center::NotifierId& NotificationStructTraits::notifier_id(
    const Notification& n) {
  return n.notifier_id();
}

// static
const RichNotificationData& NotificationStructTraits::optional_fields(
    const Notification& n) {
  return n.rich_notification_data();
}

// static
bool NotificationStructTraits::Read(NotificationDataView data,
                                    Notification* out) {
  gfx::ImageSkia icon;
  if (!data.ReadIcon(&icon))
    return false;
  out->set_icon(gfx::Image(icon));
  return EnumTraits<message_center::mojom::NotificationType,
                    message_center::NotificationType>::FromMojom(data.type(),
                                                                 &out->type_) &&
         data.ReadId(&out->id_) && data.ReadTitle(&out->title_) &&
         data.ReadMessage(&out->message_) &&
         data.ReadDisplaySource(&out->display_source_) &&
         data.ReadOriginUrl(&out->origin_url_) &&
         data.ReadNotifierId(&out->notifier_id_) &&
         data.ReadOptionalFields(&out->optional_fields_);
}

}  // namespace mojo
