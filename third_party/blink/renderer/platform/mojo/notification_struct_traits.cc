// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/mojo/notification_struct_traits.h"

#include <iterator>

namespace mojo {

using blink::mojom::NotificationDirection;
using blink::mojom::NotificationActionType;

// static
NotificationDirection
EnumTraits<NotificationDirection, blink::WebNotificationData::Direction>::
    ToMojom(blink::WebNotificationData::Direction input) {
  switch (input) {
    case blink::WebNotificationData::kDirectionLeftToRight:
      return NotificationDirection::LEFT_TO_RIGHT;
    case blink::WebNotificationData::kDirectionRightToLeft:
      return NotificationDirection::RIGHT_TO_LEFT;
    case blink::WebNotificationData::kDirectionAuto:
      return NotificationDirection::AUTO;
  }

  NOTREACHED();
  return NotificationDirection::AUTO;
}

// static
bool EnumTraits<NotificationDirection, blink::WebNotificationData::Direction>::
    FromMojom(NotificationDirection input,
              blink::WebNotificationData::Direction* out) {
  switch (input) {
    case NotificationDirection::LEFT_TO_RIGHT:
      *out = blink::WebNotificationData::kDirectionLeftToRight;
      return true;
    case NotificationDirection::RIGHT_TO_LEFT:
      *out = blink::WebNotificationData::kDirectionRightToLeft;
      return true;
    case NotificationDirection::AUTO:
      *out = blink::WebNotificationData::kDirectionAuto;
      return true;
  }

  return false;
}

// static
NotificationActionType
EnumTraits<NotificationActionType, blink::WebNotificationAction::Type>::ToMojom(
    blink::WebNotificationAction::Type input) {
  switch (input) {
    case blink::WebNotificationAction::kButton:
      return NotificationActionType::BUTTON;
    case blink::WebNotificationAction::kText:
      return NotificationActionType::TEXT;
  }

  NOTREACHED();
  return NotificationActionType::BUTTON;
}

// static
bool EnumTraits<NotificationActionType, blink::WebNotificationAction::Type>::
    FromMojom(NotificationActionType input,
              blink::WebNotificationAction::Type* out) {
  switch (input) {
    case NotificationActionType::BUTTON:
      *out = blink::WebNotificationAction::kButton;
      return true;
    case NotificationActionType::TEXT:
      *out = blink::WebNotificationAction::kText;
      return true;
  }

  return false;
}

// static
WTF::String StructTraits<blink::mojom::NotificationActionDataView,
                         blink::WebNotificationAction>::
    action(const blink::WebNotificationAction& action) {
  return action.action;
}

// static
WTF::String StructTraits<blink::mojom::NotificationActionDataView,
                         blink::WebNotificationAction>::
    title(const blink::WebNotificationAction& action) {
  return action.title;
}

// static
blink::KURL StructTraits<blink::mojom::NotificationActionDataView,
                         blink::WebNotificationAction>::
    icon(const blink::WebNotificationAction& action) {
  return action.icon;
}

// static
WTF::String StructTraits<blink::mojom::NotificationActionDataView,
                         blink::WebNotificationAction>::
    placeholder(const blink::WebNotificationAction& action) {
  return action.placeholder;
}

// static
bool StructTraits<blink::mojom::NotificationActionDataView,
                  blink::WebNotificationAction>::
    Read(blink::mojom::NotificationActionDataView notification_action,
         blink::WebNotificationAction* out) {
  WTF::String action;
  WTF::String title;
  blink::KURL icon;
  WTF::String placeholder;

  if (!notification_action.ReadType(&out->type) ||
      !notification_action.ReadTitle(&title) ||
      !notification_action.ReadAction(&action) ||
      !notification_action.ReadIcon(&icon) ||
      !notification_action.ReadPlaceholder(&placeholder)) {
    return false;
  }

  out->action = action;
  out->title = title;
  out->icon = icon;
  out->placeholder = placeholder;
  return true;
}

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::title(const blink::WebNotificationData& data) {
  return data.title;
}

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::lang(const blink::WebNotificationData& data) {
  return data.lang;
}

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::body(const blink::WebNotificationData& data) {
  return data.body;
}

// static
WTF::String StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::tag(const blink::WebNotificationData& data) {
  return data.tag;
}

// static
blink::KURL StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::image(const blink::WebNotificationData& data) {
  return data.image;
}

// static
blink::KURL StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::icon(const blink::WebNotificationData& data) {
  return data.icon;
}

// static
blink::KURL StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::badge(const blink::WebNotificationData& data) {
  return data.badge;
}

// static
base::span<const int32_t> StructTraits<blink::mojom::NotificationDataDataView,
                                       blink::WebNotificationData>::
    vibration_pattern(const blink::WebNotificationData& data) {
  // TODO(https://crbug.com/798466): Align data types to avoid this cast.
  return base::make_span(reinterpret_cast<const int32_t*>(data.vibrate.Data()),
                         data.vibrate.size());
}

// static
base::span<const blink::WebNotificationAction> StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::actions(const blink::WebNotificationData&
                                             data) {
  return base::make_span(data.actions.Data(), data.actions.size());
}

// static
base::span<const int8_t> StructTraits<
    blink::mojom::NotificationDataDataView,
    blink::WebNotificationData>::data(const blink::WebNotificationData& data) {
  // TODO(https://crbug.com/798466): Align data types to avoid this cast.
  return base::make_span(reinterpret_cast<const int8_t*>(data.data.Data()),
                         data.data.size());
}

// static
bool StructTraits<blink::mojom::NotificationDataDataView,
                  blink::WebNotificationData>::
    Read(blink::mojom::NotificationDataDataView notification_data,
         blink::WebNotificationData* out) {
  // We can't read these values to the |out| type directly because it relies
  // on Web* types, whereas Mojo uses the regular WTF types. This will be
  // solved when the Web* types are removed as part of the Onion Soup refactor.
  WTF::String title;
  WTF::String lang;
  WTF::String body;
  WTF::String tag;

  blink::KURL image;
  blink::KURL icon;
  blink::KURL badge;
  Vector<int32_t> vibrate;
  Vector<int8_t> data;
  Vector<blink::WebNotificationAction> actions;

  if (!notification_data.ReadTitle(&title) ||
      !notification_data.ReadDirection(&out->direction) ||
      !notification_data.ReadLang(&lang) ||
      !notification_data.ReadBody(&body) || !notification_data.ReadTag(&tag) ||
      !notification_data.ReadImage(&image) ||
      !notification_data.ReadIcon(&icon) ||
      !notification_data.ReadBadge(&badge) ||
      !notification_data.ReadVibrationPattern(&vibrate) ||
      !notification_data.ReadData(&data) ||
      !notification_data.ReadActions(&actions)) {
    return false;
  }

  out->title = title;
  out->lang = lang;
  out->body = body;
  out->tag = tag;
  out->image = image;
  out->icon = icon;
  out->badge = badge;
  out->vibrate = vibrate;
  out->timestamp = notification_data.timestamp();
  out->renotify = notification_data.renotify();
  out->silent = notification_data.silent();
  out->require_interaction = notification_data.require_interaction();
  out->data = data;
  out->actions = actions;
  return true;
}

// static
base::span<const SkBitmap>
StructTraits<blink::mojom::NotificationResourcesDataView,
             blink::WebNotificationResources>::
    action_icons(const blink::WebNotificationResources& resources) {
  return base::make_span(resources.action_icons.Data(),
                         resources.action_icons.size());
}

// static
bool StructTraits<blink::mojom::NotificationResourcesDataView,
                  blink::WebNotificationResources>::
    Read(blink::mojom::NotificationResourcesDataView notification_resources,
         blink::WebNotificationResources* out) {
  // Cannot read to |out| directly because it expects a WebVector (see above).
  Vector<SkBitmap> action_icons;

  if (!notification_resources.ReadImage(&out->image) ||
      !notification_resources.ReadIcon(&out->icon) ||
      !notification_resources.ReadBadge(&out->badge) ||
      !notification_resources.ReadActionIcons(&action_icons)) {
    return false;
  }

  out->action_icons = action_icons;
  return true;
}

}  // namespace mojo
