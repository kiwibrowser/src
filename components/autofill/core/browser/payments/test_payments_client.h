// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_TEST_PAYMENTS_CLIENT_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_TEST_PAYMENTS_CLIENT_H_

#include <string>
#include <vector>

#include "components/autofill/core/browser/payments/payments_client.h"

namespace autofill {
namespace payments {

class TestPaymentsClient : public payments::PaymentsClient {
 public:
  TestPaymentsClient(net::URLRequestContextGetter* context_getter,
                     PrefService* pref_service,
                     identity::IdentityManager* identity_manager,
                     payments::PaymentsClientUnmaskDelegate* unmask_delegate,
                     payments::PaymentsClientSaveDelegate* save_delegate);

  ~TestPaymentsClient() override;

  void GetUploadDetails(const std::vector<AutofillProfile>& addresses,
                        const int detected_values,
                        const std::string& pan_first_six,
                        const std::vector<const char*>& active_experiments,
                        const std::string& app_locale) override;

  void UploadCard(const payments::PaymentsClient::UploadRequestDetails&
                      request_details) override;

  void SetSaveDelegate(payments::PaymentsClientSaveDelegate* save_delegate);

  void SetServerIdForCardUpload(std::string);

  int GetDetectedValuesSetInRequest() const;
  std::string GetPanFirstSixSetInRequest() const;
  std::vector<const char*> GetActiveExperimentsSetInRequest() const;

 private:
  payments::PaymentsClientSaveDelegate* save_delegate_;
  std::string server_id_;
  int detected_values_;
  std::string pan_first_six_;
  std::vector<const char*> active_experiments_;

  DISALLOW_COPY_AND_ASSIGN(TestPaymentsClient);
};

}  // namespace payments
}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_TEST_PAYMENTS_CLIENT_H_
