// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/webdata/logins_table.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/os_crypt/ie7_password_win.h"
#include "sql/statement.h"

bool LoginsTable::AddIE7Login(const IE7PasswordInfo& info) {
  sql::Statement s(db_->GetUniqueStatement(
      "INSERT OR REPLACE INTO ie7_logins "
      "(url_hash, password_value, date_created) "
      "VALUES (?,?,?)"));
  s.BindString(0, base::WideToUTF8(info.url_hash));
  s.BindBlob(1, &info.encrypted_data.front(),
             static_cast<int>(info.encrypted_data.size()));
  s.BindInt64(2, info.date_created.ToTimeT());

  return s.Run();
}

bool LoginsTable::RemoveIE7Login(const IE7PasswordInfo& info) {
  // Remove a login by UNIQUE-constrained fields.
  sql::Statement s(db_->GetUniqueStatement(
      "DELETE FROM ie7_logins WHERE url_hash = ?"));
  s.BindString(0, base::WideToUTF8(info.url_hash));

  return s.Run();
}

bool LoginsTable::GetIE7Login(const IE7PasswordInfo& info,
                              IE7PasswordInfo* result) {
  DCHECK(result);
  sql::Statement s(db_->GetUniqueStatement(
      "SELECT password_value, date_created FROM ie7_logins "
      "WHERE url_hash == ? "));
  s.BindString16(0, info.url_hash);

  if (s.Step()) {
    s.ColumnBlobAsVector(0, &result->encrypted_data);
    result->date_created = base::Time::FromTimeT(s.ColumnInt64(1));
    result->url_hash = info.url_hash;
  }
  return s.Succeeded();
}
