// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/payment_method_manifest_table.h"

#include <time.h>

#include "base/logging.h"
#include "base/time/time.h"
#include "components/webdata/common/web_database.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace payments {
namespace {

// Data valid duration in seconds.
const time_t DATA_VALID_TIME_IN_SECONDS = 90 * 24 * 60 * 60;

WebDatabaseTable::TypeKey GetKey() {
  // We just need a unique constant. Use the address of a static that
  // COMDAT folding won't touch in an optimizing linker.
  static int table_key = 0;
  return reinterpret_cast<void*>(&table_key);
}

}  // namespace

PaymentMethodManifestTable::PaymentMethodManifestTable() {}

PaymentMethodManifestTable::~PaymentMethodManifestTable() {}

PaymentMethodManifestTable* PaymentMethodManifestTable::FromWebDatabase(
    WebDatabase* db) {
  return static_cast<PaymentMethodManifestTable*>(db->GetTable(GetKey()));
}

WebDatabaseTable::TypeKey PaymentMethodManifestTable::GetTypeKey() const {
  return GetKey();
}

bool PaymentMethodManifestTable::CreateTablesIfNecessary() {
  if (!db_->Execute("CREATE TABLE IF NOT EXISTS payment_method_manifest ( "
                    "expire_date INTEGER NOT NULL DEFAULT 0, "
                    "method_name VARCHAR, "
                    "web_app_id VARCHAR) ")) {
    NOTREACHED();
    return false;
  }

  return true;
}

bool PaymentMethodManifestTable::IsSyncable() {
  return false;
}

bool PaymentMethodManifestTable::MigrateToVersion(
    int version,
    bool* update_compatible_version) {
  return true;
}

void PaymentMethodManifestTable::RemoveExpiredData() {
  const time_t now_date_in_seconds = base::Time::NowFromSystemTime().ToTimeT();
  sql::Statement s(db_->GetUniqueStatement(
      "DELETE FROM payment_method_manifest WHERE expire_date < ? "));
  s.BindInt64(0, now_date_in_seconds);
  s.Run();
}

bool PaymentMethodManifestTable::AddManifest(
    const std::string& payment_method,
    const std::vector<std::string>& web_app_ids) {
  sql::Transaction transaction(db_);
  if (!transaction.Begin())
    return false;

  sql::Statement s1(db_->GetUniqueStatement(
      "DELETE FROM payment_method_manifest WHERE method_name=? "));
  s1.BindString(0, payment_method);
  if (!s1.Run())
    return false;

  sql::Statement s2(
      db_->GetUniqueStatement("INSERT INTO payment_method_manifest "
                              "(expire_date, method_name, web_app_id) "
                              "VALUES (?, ?, ?) "));
  const time_t expire_date_in_seconds =
      base::Time::NowFromSystemTime().ToTimeT() + DATA_VALID_TIME_IN_SECONDS;
  for (const auto& id : web_app_ids) {
    int index = 0;
    s2.BindInt64(index++, expire_date_in_seconds);
    s2.BindString(index++, payment_method);
    s2.BindString(index, id);
    if (!s2.Run())
      return false;
    s2.Reset(true);
  }

  if (!transaction.Commit())
    return false;

  return true;
}

std::vector<std::string> PaymentMethodManifestTable::GetManifest(
    const std::string& payment_method) {
  std::vector<std::string> web_app_ids;
  sql::Statement s(
      db_->GetUniqueStatement("SELECT web_app_id "
                              "FROM payment_method_manifest "
                              "WHERE method_name=?"));
  s.BindString(0, payment_method);

  while (s.Step()) {
    web_app_ids.emplace_back(s.ColumnString(0));
  }

  return web_app_ids;
}

}  // namespace payments
