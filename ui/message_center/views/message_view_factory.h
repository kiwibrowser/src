// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_MESSAGE_VIEW_FACTORY_H_
#define UI_MESSAGE_CENTER_MESSAGE_VIEW_FACTORY_H_

#include "ui/message_center/message_center_export.h"

#include <memory>

#include "base/callback_forward.h"

namespace message_center {

class MessageView;
class Notification;

// Creates appropriate MessageViews for notifications depending on the
// notification type. A notification is top level if it needs to be rendered
// outside the browser window. No custom shadows are created for top level
// notifications on Linux with Aura.
class MESSAGE_CENTER_EXPORT MessageViewFactory {
 public:
  // A function that creates MessageView for a NOTIFICATION_TYPE_CUSTOM
  // notification.
  typedef base::Callback<std::unique_ptr<MessageView>(const Notification&)>
      CustomMessageViewFactoryFunction;

  static MessageView* Create(const Notification& notification, bool top_level);

  // Sets the function that will be invoked to create a custom notification
  // view. This should be a repeating callback. It's an error to attempt to show
  // a custom notification without first having called this function. Currently,
  // only ARC uses custom notifications, so this doesn't need to distinguish
  // between various sources of custom notification.
  static void SetCustomNotificationViewFactory(
      const CustomMessageViewFactoryFunction& factory_function);

  // Returns whether the custom view factory function has already been set.
  static bool HasCustomNotificationViewFactory();
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_MESSAGE_VIEW_FACTORY_H_
