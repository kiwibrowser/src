// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This class listens for notifications from the Google Push notifications
// service, and signals when they arrive.  It checks all incoming stanzas to
// see if they look like notifications, and filters out those which are not
// valid.
//
// The task is deleted automatically by the buzz::XmppClient. This occurs in the
// destructor of TaskRunner, which is a superclass of buzz::XmppClient.

#ifndef JINGLE_NOTIFIER_PUSH_NOTIFICATIONS_LISTENER_LISTEN_TASK_H_
#define JINGLE_NOTIFIER_PUSH_NOTIFICATIONS_LISTENER_LISTEN_TASK_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "third_party/libjingle_xmpp/xmpp/xmpptask.h"

namespace buzz {
class XmlElement;
}

namespace notifier {

struct Notification;

class PushNotificationsListenTask : public buzz::XmppTask {
 public:
  class Delegate {
   public:
    virtual void OnNotificationReceived(const Notification& notification) = 0;

   protected:
    virtual ~Delegate();
  };

  PushNotificationsListenTask(buzz::XmppTaskParentInterface* parent,
                              Delegate* delegate);
  ~PushNotificationsListenTask() override;

  // Overriden from buzz::XmppTask.
  int ProcessStart() override;
  int ProcessResponse() override;
  bool HandleStanza(const buzz::XmlElement* stanza) override;

 private:
  bool IsValidNotification(const buzz::XmlElement* stanza);

  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(PushNotificationsListenTask);
};

typedef PushNotificationsListenTask::Delegate
    PushNotificationsListenTaskDelegate;

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_PUSH_NOTIFICATIONS_LISTENER_LISTEN_TASK_H_
