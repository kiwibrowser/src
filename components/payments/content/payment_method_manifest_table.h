// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_PAYMENT_METHOD_MANIFEST_TABLE_H_
#define COMPONENTS_PAYMENTS_CONTENT_PAYMENT_METHOD_MANIFEST_TABLE_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "components/webdata/common/web_database_table.h"

class WebDatabase;

namespace payments {

// This class manages payment_method_manifest table in SQLite database. It
// expects the following schema.
//
// payment_method_manifest The table stores WebAppManifestSection.id of the
//                         supported web app in this payment method manifest.
//                         Note that a payment method manifest might contain
//                         multiple supported web apps ids.
//
//  expire_date            The expire date in seconds from 1601-01-01 00:00:00
//                         UTC.
//  method_name            The method name.
//  web_app_id             The supported web app id.
//                         (WebAppManifestSection.id).
//
class PaymentMethodManifestTable : public WebDatabaseTable {
 public:
  PaymentMethodManifestTable();
  ~PaymentMethodManifestTable() override;

  // Retrieves the PaymentMethodManifestTable* owned by |db|.
  static PaymentMethodManifestTable* FromWebDatabase(WebDatabase* db);

  // WebDatabaseTable:
  WebDatabaseTable::TypeKey GetTypeKey() const override;
  bool CreateTablesIfNecessary() override;
  bool IsSyncable() override;
  bool MigrateToVersion(int version, bool* update_compatible_version) override;

  // Remove expired data.
  void RemoveExpiredData();

  // Adds |payment_method|'s manifest. |web_app_ids| contains supported web apps
  // ids.
  bool AddManifest(const std::string& payment_method,
                   const std::vector<std::string>& web_app_ids);

  // Gets manifest for |payment_method|. Return empty vector if no manifest
  // exists for this method.
  std::vector<std::string> GetManifest(const std::string& payment_method);

 private:
  DISALLOW_COPY_AND_ASSIGN(PaymentMethodManifestTable);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_PAYMENT_METHOD_MANIFEST_TABLE_H_