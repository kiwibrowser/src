// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/message_view_factory.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/lazy_instance.h"
#include "ui/message_center/public/cpp/features.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/views/notification_view.h"
#include "ui/message_center/views/notification_view_md.h"

#if defined(OS_WIN)
#include "ui/base/win/shell.h"
#endif

namespace message_center {

namespace {

base::LazyInstance<MessageViewFactory::CustomMessageViewFactoryFunction>::Leaky
    g_custom_view_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
MessageView* MessageViewFactory::Create(const Notification& notification,
                                        bool top_level) {
  MessageView* notification_view = nullptr;
  switch (notification.type()) {
    case NOTIFICATION_TYPE_BASE_FORMAT:
    case NOTIFICATION_TYPE_IMAGE:
    case NOTIFICATION_TYPE_MULTIPLE:
    case NOTIFICATION_TYPE_SIMPLE:
    case NOTIFICATION_TYPE_PROGRESS:
      // All above roads lead to the generic NotificationView.
      if (base::FeatureList::IsEnabled(message_center::kNewStyleNotifications))
        notification_view = new NotificationViewMD(notification);
      else
        notification_view = new NotificationView(notification);
      break;
    case NOTIFICATION_TYPE_CUSTOM:
      notification_view =
          g_custom_view_factory.Get().Run(notification).release();
      break;
    default:
      // If the caller asks for an unrecognized kind of view (entirely possible
      // if an application is running on an older version of this code that
      // doesn't have the requested kind of notification template), we'll fall
      // back to a notification instance that will provide at least basic
      // functionality.
      LOG(WARNING) << "Unable to fulfill request for unrecognized or"
                   << "unsupported notification type " << notification.type()
                   << ". Falling back to simple notification type.";
      notification_view = new NotificationView(notification);
  }

#if defined(OS_LINUX)
  // Don't create shadows for notification toasts on Linux or CrOS.
  if (top_level)
    return notification_view;
#endif

#if defined(OS_WIN)
  // Don't create shadows for notifications on Windows under classic theme.
  if (top_level && !ui::win::IsAeroGlassEnabled()) {
    return notification_view;
  }
#endif  // OS_WIN

  notification_view->SetIsNested();
  return notification_view;
}

// static
void MessageViewFactory::SetCustomNotificationViewFactory(
    const CustomMessageViewFactoryFunction& factory_function) {
  g_custom_view_factory.Get() = factory_function;
}

// static
bool MessageViewFactory::HasCustomNotificationViewFactory() {
  return !g_custom_view_factory.Get().is_null();
}

}  // namespace message_center
