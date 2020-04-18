// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/easy_unlock_private/easy_unlock_private_connection_manager.h"

#include <utility>

#include "base/logging.h"
#include "chrome/browser/extensions/api/easy_unlock_private/easy_unlock_private_connection.h"
#include "chrome/common/extensions/api/easy_unlock_private.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/wire_message.h"
#include "extensions/browser/event_router.h"

using cryptauth::Connection;
using cryptauth::WireMessage;

namespace extensions {
namespace {

const char kEasyUnlockFeatureName[] = "easy_unlock";

api::easy_unlock_private::ConnectionStatus ToApiConnectionStatus(
    Connection::Status status) {
  switch (status) {
    case Connection::Status::DISCONNECTED:
      return api::easy_unlock_private::CONNECTION_STATUS_DISCONNECTED;
    case Connection::Status::IN_PROGRESS:
      return api::easy_unlock_private::CONNECTION_STATUS_IN_PROGRESS;
    case Connection::Status::CONNECTED:
      return api::easy_unlock_private::CONNECTION_STATUS_CONNECTED;
  }
  return api::easy_unlock_private::CONNECTION_STATUS_NONE;
}

}  // namespace

EasyUnlockPrivateConnectionManager::EasyUnlockPrivateConnectionManager(
    content::BrowserContext* context)
    : browser_context_(context) {}

EasyUnlockPrivateConnectionManager::~EasyUnlockPrivateConnectionManager() {
  // Remove |this| as an observer from all connections passed to
  // |AddConnection()|.
  for (const auto& extension_id : extensions_) {
    base::hash_set<int>* connections =
        GetResourceManager()->GetResourceIds(extension_id);
    for (int connection_id : *connections) {
      Connection* connection = GetConnection(extension_id, connection_id);
      if (connection)
        connection->RemoveObserver(this);
    }
  }
}

int EasyUnlockPrivateConnectionManager::AddConnection(
    const Extension* extension,
    std::unique_ptr<Connection> connection,
    bool persistent) {
  DCHECK(connection);
  connection->AddObserver(this);
  extensions_.insert(extension->id());
  EasyUnlockPrivateConnection* api_connection = new EasyUnlockPrivateConnection(
      persistent, extension->id(), std::move(connection));
  int connection_id = GetResourceManager()->Add(api_connection);
  return connection_id;
}

api::easy_unlock_private::ConnectionStatus
EasyUnlockPrivateConnectionManager::ConnectionStatus(const Extension* extension,
                                                     int connection_id) const {
  Connection* connection = GetConnection(extension->id(), connection_id);
  if (connection)
    return ToApiConnectionStatus(connection->status());
  return api::easy_unlock_private::CONNECTION_STATUS_NONE;
}

std::string EasyUnlockPrivateConnectionManager::GetDeviceAddress(
    const Extension* extension,
    int connection_id) const {
  Connection* connection = GetConnection(extension->id(), connection_id);
  if (!connection)
    return std::string();
  return connection->GetDeviceAddress();
}

bool EasyUnlockPrivateConnectionManager::Disconnect(const Extension* extension,
                                                    int connection_id) {
  Connection* connection = GetConnection(extension->id(), connection_id);
  if (connection) {
    connection->Disconnect();
    return true;
  }
  return false;
}

bool EasyUnlockPrivateConnectionManager::SendMessage(
    const Extension* extension,
    int connection_id,
    const std::string& message_body) {
  Connection* connection = GetConnection(extension->id(), connection_id);
  if (connection && connection->IsConnected()) {
    connection->SendMessage(std::make_unique<WireMessage>(message_body));
    return true;
  }
  return false;
}

void EasyUnlockPrivateConnectionManager::OnConnectionStatusChanged(
    Connection* connection,
    Connection::Status old_status,
    Connection::Status new_status) {
  std::string event_name =
      api::easy_unlock_private::OnConnectionStatusChanged::kEventName;
  events::HistogramValue histogram_value =
      events::EASY_UNLOCK_PRIVATE_ON_CONNECTION_STATUS_CHANGED;
  std::unique_ptr<base::ListValue> args =
      api::easy_unlock_private::OnConnectionStatusChanged::Create(
          0, ToApiConnectionStatus(old_status),
          ToApiConnectionStatus(new_status));
  DispatchConnectionEvent(event_name, histogram_value, connection,
                          std::move(args));
}

void EasyUnlockPrivateConnectionManager::OnMessageReceived(
    const Connection& connection,
    const WireMessage& message) {
  std::string event_name = api::easy_unlock_private::OnDataReceived::kEventName;
  events::HistogramValue histogram_value =
      events::EASY_UNLOCK_PRIVATE_ON_DATA_RECEIVED;
  std::vector<char> data(message.body().begin(), message.body().end());
  std::unique_ptr<base::ListValue> args =
      api::easy_unlock_private::OnDataReceived::Create(0, data);
  DispatchConnectionEvent(event_name, histogram_value, &connection,
                          std::move(args));
}

void EasyUnlockPrivateConnectionManager::OnSendCompleted(
    const Connection& connection,
    const WireMessage& message,
    bool success) {
  if (message.feature() != std::string(kEasyUnlockFeatureName)) {
    // Only process messages sent as part of EasyUnlock.
    return;
  }

  std::string event_name =
      api::easy_unlock_private::OnSendCompleted::kEventName;
  events::HistogramValue histogram_value =
      events::EASY_UNLOCK_PRIVATE_ON_SEND_COMPLETED;
  std::vector<char> data(message.payload().begin(), message.payload().end());
  std::unique_ptr<base::ListValue> args =
      api::easy_unlock_private::OnSendCompleted::Create(0, data, success);
  DispatchConnectionEvent(event_name, histogram_value, &connection,
                          std::move(args));
}

void EasyUnlockPrivateConnectionManager::DispatchConnectionEvent(
    const std::string& event_name,
    events::HistogramValue histogram_value,
    const Connection* connection,
    std::unique_ptr<base::ListValue> args) {
  const EventListenerMap::ListenerList& listeners =
      EventRouter::Get(browser_context_)
          ->listeners()
          .GetEventListenersByName(event_name);
  for (const auto& listener : listeners) {
    std::string extension_id = listener->extension_id();
    int connection_id = FindConnectionId(extension_id, connection);
    std::unique_ptr<base::ListValue> args_copy(args->DeepCopy());
    int connection_index = 0;
    args_copy->Set(connection_index,
                   std::make_unique<base::Value>(connection_id));
    std::unique_ptr<Event> event(
        new Event(histogram_value, event_name, std::move(args_copy)));
    EventRouter::Get(browser_context_)
        ->DispatchEventToExtension(extension_id, std::move(event));
  }
}

ApiResourceManager<EasyUnlockPrivateConnection>*
EasyUnlockPrivateConnectionManager::GetResourceManager() const {
  return ApiResourceManager<EasyUnlockPrivateConnection>::Get(browser_context_);
}

Connection* EasyUnlockPrivateConnectionManager::GetConnection(
    const std::string& extension_id,
    int connection_id) const {
  ApiResourceManager<EasyUnlockPrivateConnection>* manager =
      GetResourceManager();
  EasyUnlockPrivateConnection* easy_unlock_connection =
      manager->Get(extension_id, connection_id);
  if (easy_unlock_connection) {
    return easy_unlock_connection->GetConnection();
  }
  return nullptr;
}

int EasyUnlockPrivateConnectionManager::FindConnectionId(
    const std::string& extension_id,
    const Connection* connection) {
  base::hash_set<int>* connection_ids =
      GetResourceManager()->GetResourceIds(extension_id);
  for (int connection_id : *connection_ids) {
    if (connection == GetConnection(extension_id, connection_id))
      return connection_id;
  }
  return 0;
}

}  // namespace extensions
