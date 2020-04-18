// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/common/database/database_connections.h"

#include <stdint.h>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"

namespace storage {

DatabaseConnections::DatabaseConnections() = default;

DatabaseConnections::~DatabaseConnections() {
  DCHECK(connections_.empty());
}

bool DatabaseConnections::IsEmpty() const {
  return connections_.empty();
}

bool DatabaseConnections::IsDatabaseOpened(
    const std::string& origin_identifier,
    const base::string16& database_name) const {
  OriginConnections::const_iterator origin_it =
      connections_.find(origin_identifier);
  if (origin_it == connections_.end())
    return false;
  const DBConnections& origin_connections = origin_it->second;
  return (origin_connections.find(database_name) != origin_connections.end());
}

bool DatabaseConnections::IsOriginUsed(
    const std::string& origin_identifier) const {
  return (connections_.find(origin_identifier) != connections_.end());
}

bool DatabaseConnections::AddConnection(
    const std::string& origin_identifier,
    const base::string16& database_name) {
  int& count = connections_[origin_identifier][database_name].first;
  return ++count == 1;
}

bool DatabaseConnections::RemoveConnection(
    const std::string& origin_identifier,
    const base::string16& database_name) {
  return RemoveConnectionsHelper(origin_identifier, database_name, 1);
}

void DatabaseConnections::RemoveAllConnections() {
  connections_.clear();
}

std::vector<std::pair<std::string, base::string16>>
DatabaseConnections::RemoveConnections(const DatabaseConnections& connections) {
  std::vector<std::pair<std::string, base::string16>> closed_dbs;
  for (const auto& origin_connections_pair : connections.connections_) {
    const DBConnections& db_connections = origin_connections_pair.second;
    for (const auto& count_size_pair : db_connections) {
      if (RemoveConnectionsHelper(origin_connections_pair.first,
                                  count_size_pair.first,
                                  count_size_pair.second.first)) {
        closed_dbs.emplace_back(origin_connections_pair.first,
                                count_size_pair.first);
      }
    }
  }
  return closed_dbs;
}

int64_t DatabaseConnections::GetOpenDatabaseSize(
    const std::string& origin_identifier,
    const base::string16& database_name) const {
  DCHECK(IsDatabaseOpened(origin_identifier, database_name));
  return connections_[origin_identifier][database_name].second;
}

void DatabaseConnections::SetOpenDatabaseSize(
    const std::string& origin_identifier,
    const base::string16& database_name,
    int64_t size) {
  DCHECK(IsDatabaseOpened(origin_identifier, database_name));
  connections_[origin_identifier][database_name].second = size;
}

std::vector<std::pair<std::string, base::string16>>
DatabaseConnections::ListConnections() const {
  std::vector<std::pair<std::string, base::string16>> list;
  for (const auto& origin_connections_pair : connections_) {
    const DBConnections& db_connections = origin_connections_pair.second;
    for (const auto& count_size_pair : db_connections)
      list.emplace_back(origin_connections_pair.first, count_size_pair.first);
  }
  return list;
}

bool DatabaseConnections::RemoveConnectionsHelper(
    const std::string& origin_identifier,
    const base::string16& database_name,
    int num_connections) {
  OriginConnections::iterator origin_iterator =
      connections_.find(origin_identifier);
  DCHECK(origin_iterator != connections_.end());
  DBConnections& db_connections = origin_iterator->second;
  int& count = db_connections[database_name].first;
  DCHECK(count >= num_connections);
  count -= num_connections;
  if (count)
    return false;
  db_connections.erase(database_name);
  if (db_connections.empty())
    connections_.erase(origin_iterator);
  return true;
}

DatabaseConnectionsWrapper::DatabaseConnectionsWrapper() = default;

DatabaseConnectionsWrapper::~DatabaseConnectionsWrapper() = default;

bool DatabaseConnectionsWrapper::HasOpenConnections() {
  base::AutoLock auto_lock(open_connections_lock_);
  return !open_connections_.IsEmpty();
}

void DatabaseConnectionsWrapper::AddOpenConnection(
    const std::string& origin_identifier,
    const base::string16& database_name) {
  base::AutoLock auto_lock(open_connections_lock_);
  open_connections_.AddConnection(origin_identifier, database_name);
}

void DatabaseConnectionsWrapper::RemoveOpenConnection(
    const std::string& origin_identifier,
    const base::string16& database_name) {
  base::AutoLock auto_lock(open_connections_lock_);
  open_connections_.RemoveConnection(origin_identifier, database_name);
  if (waiting_to_close_event_ && open_connections_.IsEmpty())
    waiting_to_close_event_->Signal();
}

bool DatabaseConnectionsWrapper::WaitForAllDatabasesToClose(
    base::TimeDelta timeout) {
  base::WaitableEvent waitable_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  {
    base::AutoLock auto_lock(open_connections_lock_);
    if (open_connections_.IsEmpty())
      return true;
    waiting_to_close_event_ = &waitable_event;
  }
  waitable_event.TimedWait(timeout);
  {
    base::AutoLock auto_lock(open_connections_lock_);
    waiting_to_close_event_ = nullptr;
    return open_connections_.IsEmpty();
  }
}

}  // namespace storage
