// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "storage/common/database/database_connections.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;
using storage::DatabaseConnections;
using storage::DatabaseConnectionsWrapper;

namespace content {

namespace {

void RemoveConnectionTask(
    const std::string& origin_id, const base::string16& database_name,
    scoped_refptr<DatabaseConnectionsWrapper> obj,
    bool* did_task_execute) {
  *did_task_execute = true;
  obj->RemoveOpenConnection(origin_id, database_name);
}

}  // anonymous namespace

TEST(DatabaseConnectionsTest, DatabaseConnectionsTest) {
  const std::string kOriginId("origin_id");
  const base::string16 kName(ASCIIToUTF16("database_name"));
  const base::string16 kName2(ASCIIToUTF16("database_name2"));
  const int64_t kSize = 1000;

  DatabaseConnections connections;

  EXPECT_TRUE(connections.IsEmpty());
  EXPECT_FALSE(connections.IsDatabaseOpened(kOriginId, kName));
  EXPECT_FALSE(connections.IsOriginUsed(kOriginId));

  connections.AddConnection(kOriginId, kName);
  EXPECT_FALSE(connections.IsEmpty());
  EXPECT_TRUE(connections.IsDatabaseOpened(kOriginId, kName));
  EXPECT_TRUE(connections.IsOriginUsed(kOriginId));
  EXPECT_EQ(0, connections.GetOpenDatabaseSize(kOriginId, kName));
  connections.SetOpenDatabaseSize(kOriginId, kName, kSize);
  EXPECT_EQ(kSize, connections.GetOpenDatabaseSize(kOriginId, kName));

  connections.RemoveConnection(kOriginId, kName);
  EXPECT_TRUE(connections.IsEmpty());
  EXPECT_FALSE(connections.IsDatabaseOpened(kOriginId, kName));
  EXPECT_FALSE(connections.IsOriginUsed(kOriginId));

  connections.AddConnection(kOriginId, kName);
  connections.SetOpenDatabaseSize(kOriginId, kName, kSize);
  EXPECT_EQ(kSize, connections.GetOpenDatabaseSize(kOriginId, kName));
  connections.AddConnection(kOriginId, kName);
  EXPECT_EQ(kSize, connections.GetOpenDatabaseSize(kOriginId, kName));
  EXPECT_FALSE(connections.IsEmpty());
  EXPECT_TRUE(connections.IsDatabaseOpened(kOriginId, kName));
  EXPECT_TRUE(connections.IsOriginUsed(kOriginId));
  connections.AddConnection(kOriginId, kName2);
  EXPECT_TRUE(connections.IsDatabaseOpened(kOriginId, kName2));

  DatabaseConnections another;
  another.AddConnection(kOriginId, kName);
  another.AddConnection(kOriginId, kName2);

  std::vector<std::pair<std::string, base::string16>> closed_dbs =
      connections.RemoveConnections(another);
  EXPECT_EQ(1u, closed_dbs.size());
  EXPECT_EQ(kOriginId, closed_dbs[0].first);
  EXPECT_EQ(kName2, closed_dbs[0].second);
  EXPECT_FALSE(connections.IsDatabaseOpened(kOriginId, kName2));
  EXPECT_TRUE(connections.IsDatabaseOpened(kOriginId, kName));
  EXPECT_EQ(kSize, connections.GetOpenDatabaseSize(kOriginId, kName));
  another.RemoveAllConnections();
  connections.RemoveAllConnections();
  EXPECT_TRUE(connections.IsEmpty());

  // Ensure the return value properly indicates the initial
  // addition and final removal.
  EXPECT_TRUE(connections.AddConnection(kOriginId, kName));
  EXPECT_FALSE(connections.AddConnection(kOriginId, kName));
  EXPECT_FALSE(connections.AddConnection(kOriginId, kName));
  EXPECT_FALSE(connections.RemoveConnection(kOriginId, kName));
  EXPECT_FALSE(connections.RemoveConnection(kOriginId, kName));
  EXPECT_TRUE(connections.RemoveConnection(kOriginId, kName));
}

TEST(DatabaseConnectionsTest, DatabaseConnectionsWrapperTest) {
  const std::string kOriginId("origin_id");
  const base::string16 kName(ASCIIToUTF16("database_name"));

  base::MessageLoop message_loop;
  scoped_refptr<DatabaseConnectionsWrapper> obj(new DatabaseConnectionsWrapper);
  EXPECT_FALSE(obj->HasOpenConnections());
  obj->AddOpenConnection(kOriginId, kName);
  EXPECT_TRUE(obj->HasOpenConnections());
  obj->AddOpenConnection(kOriginId, kName);
  EXPECT_TRUE(obj->HasOpenConnections());
  obj->RemoveOpenConnection(kOriginId, kName);
  EXPECT_TRUE(obj->HasOpenConnections());
  obj->RemoveOpenConnection(kOriginId, kName);
  EXPECT_FALSE(obj->HasOpenConnections());
  EXPECT_TRUE(obj->WaitForAllDatabasesToClose(base::TimeDelta()));

  // Test WaitForAllDatabasesToClose with the last connection
  // being removed on another thread.
  obj->AddOpenConnection(kOriginId, kName);
  EXPECT_FALSE(obj->WaitForAllDatabasesToClose(base::TimeDelta()));
  base::Thread thread("WrapperTestThread");
  thread.Start();
  bool did_task_execute = false;
  thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&RemoveConnectionTask, kOriginId, kName, obj,
                                &did_task_execute));
  // Use a long timeout value to avoid timeouts on test bots.
  EXPECT_TRUE(obj->WaitForAllDatabasesToClose(
      base::TimeDelta::FromSeconds(15)));
  EXPECT_TRUE(did_task_execute);
  EXPECT_FALSE(obj->HasOpenConnections());
}

}  // namespace content
