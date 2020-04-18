// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_PAYMENT_MANIFEST_WEB_DATA_SERVICE_H_
#define COMPONENTS_PAYMENTS_CONTENT_PAYMENT_MANIFEST_WEB_DATA_SERVICE_H_

#include <memory>
#include <vector>

#include "base/memory/ref_counted.h"
#include "components/payments/content/web_app_manifest.h"
#include "components/webdata/common/web_data_service_base.h"
#include "components/webdata/common/web_database.h"

class WDTypedResult;
class WebDatabaseService;
class WebDataServiceConsumer;

namespace base {
class SingleThreadTaskRunner;
}

namespace payments {

// Web data service to read/write data in WebAppManifestSectionTable and
// PaymentMethodManifestTable.
class PaymentManifestWebDataService : public WebDataServiceBase {
 public:
  PaymentManifestWebDataService(
      scoped_refptr<WebDatabaseService> wdbs,
      const ProfileErrorCallback& callback,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner);

  // Adds the web app |manifest|.
  void AddPaymentWebAppManifest(std::vector<WebAppManifestSection> manifest);

  // Adds the |payment_method|'s manifest.
  void AddPaymentMethodManifest(const std::string& payment_method,
                                std::vector<std::string> app_package_names);

  // Gets the |web_app|'s manifest.
  WebDataServiceBase::Handle GetPaymentWebAppManifest(
      const std::string& web_app,
      WebDataServiceConsumer* consumer);

  // Gets the |payment_method|'s manifest.
  WebDataServiceBase::Handle GetPaymentMethodManifest(
      const std::string& payment_method,
      WebDataServiceConsumer* consumer);

 private:
  ~PaymentManifestWebDataService() override;

  void RemoveExpiredData(WebDatabase* db);

  WebDatabase::State AddPaymentWebAppManifestImpl(
      const std::vector<WebAppManifestSection>& manifest,
      WebDatabase* db);
  WebDatabase::State AddPaymentMethodManifestImpl(
      const std::string& payment_method,
      const std::vector<std::string>& app_package_names,
      WebDatabase* db);

  std::unique_ptr<WDTypedResult> GetPaymentWebAppManifestImpl(
      const std::string& web_app,
      WebDatabase* db);
  std::unique_ptr<WDTypedResult> GetPaymentMethodManifestImpl(
      const std::string& payment_method,
      WebDatabase* db);

  DISALLOW_COPY_AND_ASSIGN(PaymentManifestWebDataService);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_PAYMENT_MANIFEST_WEB_DATA_SERVICE_H_
