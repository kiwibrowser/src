// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_
#define COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/model/syncable_service.h"
#include "sql/init_status.h"

class KeywordWebDataService;
class TokenWebData;
class WebDatabaseService;

#if defined(OS_WIN)
class PasswordWebDataService;
#endif

#if !defined(OS_IOS)
namespace payments {
class PaymentManifestWebDataService;
}  // namespace payments
#endif

namespace autofill {
class AutofillWebDataService;
}  // namespace autofill

namespace base {
class FilePath;
class SingleThreadTaskRunner;
}  // namespace base

// WebDataServiceWrapper is a KeyedService that owns multiple WebDataServices
// so that they can be associated with a context.
class WebDataServiceWrapper : public KeyedService {
 public:
  // ErrorType indicates which service encountered an error loading its data.
  enum ErrorType {
    ERROR_LOADING_AUTOFILL,
    ERROR_LOADING_KEYWORD,
    ERROR_LOADING_TOKEN,
    ERROR_LOADING_PASSWORD,
    ERROR_LOADING_PAYMENT_MANIFEST,
  };

  // Shows an error message if a loading error occurs.
  // |error_type| shows which service encountered an error while loading.
  // |init_status| is the returned status of initializing the underlying
  // database.
  // |diagnostics| contains information about the underlying database
  // which can help in identifying the cause of the error.
  using ShowErrorCallback =
      base::RepeatingCallback<void(ErrorType error_type,
                                   sql::InitStatus init_status,
                                   const std::string& diagnostics)>;

  // Constructor for WebDataServiceWrapper that initializes the different
  // WebDataServices and starts the synchronization services using |flare|.
  // Since |flare| will be copied and called multiple times, it cannot bind
  // values using base::Owned nor base::Passed; it should only bind simple or
  // refcounted types.
  WebDataServiceWrapper(
      const base::FilePath& context_path,
      const std::string& application_locale,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
      const syncer::SyncableService::StartSyncFlare& flare,
      const ShowErrorCallback& show_error_callback);
  ~WebDataServiceWrapper() override;

  // KeyedService:
  void Shutdown() override;

  // Create the various types of service instances.  These methods are virtual
  // for testing purpose.
  virtual scoped_refptr<autofill::AutofillWebDataService> GetAutofillWebData();
  virtual scoped_refptr<KeywordWebDataService> GetKeywordWebData();
  virtual scoped_refptr<TokenWebData> GetTokenWebData();
#if defined(OS_WIN)
  virtual scoped_refptr<PasswordWebDataService> GetPasswordWebData();
#endif
#if !defined(OS_IOS)
  virtual scoped_refptr<payments::PaymentManifestWebDataService>
  GetPaymentManifestWebData();
#endif

 protected:
  // For testing.
  WebDataServiceWrapper();

 private:
  scoped_refptr<WebDatabaseService> web_database_;

  scoped_refptr<autofill::AutofillWebDataService> autofill_web_data_;
  scoped_refptr<KeywordWebDataService> keyword_web_data_;
  scoped_refptr<TokenWebData> token_web_data_;

#if defined(OS_WIN)
  scoped_refptr<PasswordWebDataService> password_web_data_;
#endif

#if !defined(OS_IOS)
  scoped_refptr<payments::PaymentManifestWebDataService>
      payment_manifest_web_data_;
#endif

  DISALLOW_COPY_AND_ASSIGN(WebDataServiceWrapper);
};

#endif  // COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_
