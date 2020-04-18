// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_NOTIFICATION_STRUCT_TRAITS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_NOTIFICATION_STRUCT_TRAITS_H_

#include "base/containers/span.h"
#include "mojo/public/cpp/bindings/array_traits_wtf_vector.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "skia/public/interfaces/bitmap_skbitmap_struct_traits.h"
#include "third_party/blink/public/platform/modules/notifications/notification.mojom-blink.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_action.h"
#include "third_party/blink/renderer/platform/mojo/kurl_struct_traits.h"
#include "third_party/blink/renderer/platform/mojo/string16_mojom_traits.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace mojo {

template <>
struct PLATFORM_EXPORT EnumTraits<blink::mojom::NotificationDirection,
                                  blink::WebNotificationData::Direction> {
  static blink::mojom::NotificationDirection ToMojom(
      blink::WebNotificationData::Direction input);

  static bool FromMojom(blink::mojom::NotificationDirection input,
                        blink::WebNotificationData::Direction* out);
};

template <>
struct PLATFORM_EXPORT EnumTraits<blink::mojom::NotificationActionType,
                                  blink::WebNotificationAction::Type> {
  static blink::mojom::NotificationActionType ToMojom(
      blink::WebNotificationAction::Type input);

  static bool FromMojom(blink::mojom::NotificationActionType input,
                        blink::WebNotificationAction::Type* out);
};

template <>
struct PLATFORM_EXPORT StructTraits<blink::mojom::NotificationActionDataView,
                                    blink::WebNotificationAction> {
  static blink::WebNotificationAction::Type type(
      const blink::WebNotificationAction& action) {
    return action.type;
  }

  static WTF::String action(const blink::WebNotificationAction&);

  static WTF::String title(const blink::WebNotificationAction&);

  static blink::KURL icon(const blink::WebNotificationAction&);

  static WTF::String placeholder(const blink::WebNotificationAction&);

  static bool Read(blink::mojom::NotificationActionDataView,
                   blink::WebNotificationAction* output);
};

template <>
struct PLATFORM_EXPORT StructTraits<blink::mojom::NotificationDataDataView,
                                    blink::WebNotificationData> {
  static WTF::String title(const blink::WebNotificationData&);

  static blink::WebNotificationData::Direction direction(
      const blink::WebNotificationData& data) {
    return data.direction;
  }

  static WTF::String lang(const blink::WebNotificationData&);

  static WTF::String body(const blink::WebNotificationData&);

  static WTF::String tag(const blink::WebNotificationData&);

  static blink::KURL image(const blink::WebNotificationData&);

  static blink::KURL icon(const blink::WebNotificationData&);

  static blink::KURL badge(const blink::WebNotificationData&);

  static base::span<const int32_t> vibration_pattern(
      const blink::WebNotificationData&);

  static double timestamp(const blink::WebNotificationData& data) {
    return data.timestamp;
  }

  static bool renotify(const blink::WebNotificationData& data) {
    return data.renotify;
  }

  static bool silent(const blink::WebNotificationData& data) {
    return data.silent;
  }

  static bool require_interaction(const blink::WebNotificationData& data) {
    return data.require_interaction;
  }

  static base::span<const int8_t> data(const blink::WebNotificationData&);

  static base::span<const blink::WebNotificationAction> actions(
      const blink::WebNotificationData&);

  static bool Read(blink::mojom::NotificationDataDataView,
                   blink::WebNotificationData* output);
};

template <>
struct PLATFORM_EXPORT StructTraits<blink::mojom::NotificationResourcesDataView,
                                    blink::WebNotificationResources> {
  static const SkBitmap& image(
      const blink::WebNotificationResources& resources) {
    return resources.image;
  }

  static const SkBitmap& icon(
      const blink::WebNotificationResources& resources) {
    return resources.icon;
  }

  static const SkBitmap& badge(
      const blink::WebNotificationResources& resources) {
    return resources.badge;
  }

  static base::span<const SkBitmap> action_icons(
      const blink::WebNotificationResources&);

  static bool Read(blink::mojom::NotificationResourcesDataView,
                   blink::WebNotificationResources* output);
};

}  // namespace mojo

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_MOJO_NOTIFICATION_STRUCT_TRAITS_H_
