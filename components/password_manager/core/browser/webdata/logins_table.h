// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_WEBDATA_LOGINS_TABLE_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_WEBDATA_LOGINS_TABLE_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "components/webdata/common/web_database_table.h"

#if defined(OS_WIN)
struct IE7PasswordInfo;
#endif

class WebDatabase;

// This class manages the logins table within the SQLite database passed to the
// constructor. We no longer store passwords here except for imported IE
// passwords, so this class is now mostly responsible for deleting the table if
// it is found to exist. (The data was migrated out long ago.)
class LoginsTable : public WebDatabaseTable {
 public:
  LoginsTable() {}
  ~LoginsTable() override {}

  // Retrieves the LoginsTable* owned by |database|.
  static LoginsTable* FromWebDatabase(WebDatabase* db);

  WebDatabaseTable::TypeKey GetTypeKey() const override;
  bool CreateTablesIfNecessary() override;
  bool IsSyncable() override;
  bool MigrateToVersion(int version, bool* update_compatible_version) override;

#if defined(OS_WIN)
  // Adds |info| to the list of imported passwords from ie7/ie8.
  bool AddIE7Login(const IE7PasswordInfo& info);

  // Removes |info| from the list of imported passwords from ie7/ie8.
  bool RemoveIE7Login(const IE7PasswordInfo& info);

  // Return the ie7/ie8 login matching |info|.
  bool GetIE7Login(const IE7PasswordInfo& info, IE7PasswordInfo* result);
#endif

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginsTable);
};

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_WEBDATA_LOGINS_TABLE_H_
