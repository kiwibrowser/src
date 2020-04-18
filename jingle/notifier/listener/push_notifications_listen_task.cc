// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/listener/push_notifications_listen_task.h"

#include "base/base64.h"
#include "base/logging.h"
#include "jingle/notifier/listener/notification_constants.h"
#include "jingle/notifier/listener/notification_defines.h"
#include "jingle/notifier/listener/xml_element_util.h"
#include "third_party/libjingle_xmpp/task_runner/task.h"
#include "third_party/libjingle_xmpp/xmllite/qname.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmpp/constants.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclient.h"
#include "third_party/libjingle_xmpp/xmpp/xmppengine.h"

namespace notifier {

PushNotificationsListenTask::Delegate::~Delegate() {
}

PushNotificationsListenTask::PushNotificationsListenTask(
    buzz::XmppTaskParentInterface* parent, Delegate* delegate)
        : buzz::XmppTask(parent, buzz::XmppEngine::HL_TYPE),
          delegate_(delegate) {
  DCHECK(delegate_);
}

PushNotificationsListenTask::~PushNotificationsListenTask() {
}

int PushNotificationsListenTask::ProcessStart() {
  return STATE_RESPONSE;
}

int PushNotificationsListenTask::ProcessResponse() {
  const buzz::XmlElement* stanza = NextStanza();
  if (stanza == NULL) {
    return STATE_BLOCKED;
  }

  DVLOG(1) << "Received stanza " << XmlElementToString(*stanza);

  // The push notifications service does not need us to acknowledge receipt of
  // the notification to the buzz server.

  // TODO(sanjeevr): Write unittests to cover this.
  // Extract the service URL and service-specific data from the stanza.
  // Note that we treat the channel name as service URL.
  // The response stanza has the following format.
  // <message from="{url or bare jid}" to={full jid}>
  //  <push xmlns="google:push" channel={channel name}>
  //    <recipient to={bare jid}>{base-64 encoded data}</recipient>
  //    <data>{base-64 encoded data}</data>
  //  </push>
  // </message>

  const buzz::QName kQnPush(kPushNotificationsNamespace, "push");
  const buzz::QName kQnChannel(buzz::STR_EMPTY, "channel");
  const buzz::QName kQnData(kPushNotificationsNamespace, "data");

  const buzz::XmlElement* push_element = stanza->FirstNamed(kQnPush);
  if (push_element) {
    Notification notification;
    notification.channel = push_element->Attr(kQnChannel);
    const buzz::XmlElement* data_element = push_element->FirstNamed(kQnData);
    if (data_element) {
      const std::string& base64_encoded_data = data_element->BodyText();
      if (!base::Base64Decode(base64_encoded_data, &notification.data)) {
        LOG(WARNING) << "Could not base64-decode " << base64_encoded_data;
      }
    } else {
      LOG(WARNING) << "No data element found in push element "
                   << XmlElementToString(*push_element);
    }
    DVLOG(1) << "Received notification " << notification.ToString();
    delegate_->OnNotificationReceived(notification);
  } else {
    LOG(WARNING) << "No push element found in stanza "
                 << XmlElementToString(*stanza);
  }
  return STATE_RESPONSE;
}

bool PushNotificationsListenTask::HandleStanza(const buzz::XmlElement* stanza) {
  if (IsValidNotification(stanza)) {
    QueueStanza(stanza);
    return true;
  }
  return false;
}

bool PushNotificationsListenTask::IsValidNotification(
    const buzz::XmlElement* stanza) {
  // We don't do much validation here, just check if the stanza is a message
  // stanza.
  return (stanza->Name() == buzz::QN_MESSAGE);
}

}  // namespace notifier
