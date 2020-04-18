// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PAYMENTS_PAYMENT_APP_DATABASE_H_
#define CONTENT_BROWSER_PAYMENTS_PAYMENT_APP_DATABASE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/payments/payment_app_info_fetcher.h"
#include "content/browser/payments/payment_instrument_icon_fetcher.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/public/browser/stored_payment_app.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/modules/payments/payment_app.mojom.h"

namespace content {

class ServiceWorkerRegistration;

class CONTENT_EXPORT PaymentAppDatabase {
 public:
  using PaymentApps = std::map<int64_t, std::unique_ptr<StoredPaymentApp>>;
  using ReadAllPaymentAppsCallback = base::OnceCallback<void(PaymentApps)>;

  using DeletePaymentInstrumentCallback =
      base::OnceCallback<void(payments::mojom::PaymentHandlerStatus)>;
  using ReadPaymentInstrumentCallback =
      base::OnceCallback<void(payments::mojom::PaymentInstrumentPtr,
                              payments::mojom::PaymentHandlerStatus)>;
  using KeysOfPaymentInstrumentsCallback =
      base::OnceCallback<void(const std::vector<std::string>&,
                              payments::mojom::PaymentHandlerStatus)>;
  using HasPaymentInstrumentCallback =
      base::OnceCallback<void(payments::mojom::PaymentHandlerStatus)>;
  using WritePaymentInstrumentCallback =
      base::OnceCallback<void(payments::mojom::PaymentHandlerStatus)>;
  using FetchAndUpdatePaymentAppInfoCallback =
      base::OnceCallback<void(payments::mojom::PaymentHandlerStatus)>;
  using ClearPaymentInstrumentsCallback =
      base::OnceCallback<void(payments::mojom::PaymentHandlerStatus)>;
  using SetPaymentAppInfoCallback =
      base::OnceCallback<void(payments::mojom::PaymentHandlerStatus)>;

  explicit PaymentAppDatabase(
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context);
  ~PaymentAppDatabase();

  void ReadAllPaymentApps(ReadAllPaymentAppsCallback callback);

  void DeletePaymentInstrument(const GURL& scope,
                               const std::string& instrument_key,
                               DeletePaymentInstrumentCallback callback);
  void ReadPaymentInstrument(const GURL& scope,
                             const std::string& instrument_key,
                             ReadPaymentInstrumentCallback callback);
  void KeysOfPaymentInstruments(const GURL& scope,
                                KeysOfPaymentInstrumentsCallback callback);
  void HasPaymentInstrument(const GURL& scope,
                            const std::string& instrument_key,
                            HasPaymentInstrumentCallback callback);
  void WritePaymentInstrument(const GURL& scope,
                              const std::string& instrument_key,
                              payments::mojom::PaymentInstrumentPtr instrument,
                              WritePaymentInstrumentCallback callback);
  void FetchAndUpdatePaymentAppInfo(
      const GURL& context,
      const GURL& scope,
      FetchAndUpdatePaymentAppInfoCallback callback);
  void ClearPaymentInstruments(const GURL& scope,
                               ClearPaymentInstrumentsCallback callback);
  void SetPaymentAppUserHint(const GURL& scope, const std::string& user_hint);
  void SetPaymentAppInfoForRegisteredServiceWorker(
      int64_t registration_id,
      const std::string& instrument_key,
      const std::string& name,
      const std::string& icon,
      const std::string& method,
      SetPaymentAppInfoCallback callback);

 private:
  // ReadAllPaymentApps callbacks
  void DidReadAllPaymentApps(
      ReadAllPaymentAppsCallback callback,
      const std::vector<std::pair<int64_t, std::string>>& raw_data,
      ServiceWorkerStatusCode status);
  void DidReadAllPaymentInstruments(
      PaymentApps apps,
      ReadAllPaymentAppsCallback callback,
      const std::vector<std::pair<int64_t, std::string>>& raw_data,
      ServiceWorkerStatusCode status);

  // DeletePaymentInstrument callbacks
  void DidFindRegistrationToDeletePaymentInstrument(
      const std::string& instrument_key,
      DeletePaymentInstrumentCallback callback,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidFindPaymentInstrument(int64_t registration_id,
                                const std::string& instrument_key,
                                DeletePaymentInstrumentCallback callback,
                                const std::vector<std::string>& data,
                                ServiceWorkerStatusCode status);
  void DidDeletePaymentInstrument(DeletePaymentInstrumentCallback callback,
                                  ServiceWorkerStatusCode status);

  // ReadPaymentInstrument callbacks
  void DidFindRegistrationToReadPaymentInstrument(
      const std::string& instrument_key,
      ReadPaymentInstrumentCallback callback,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidReadPaymentInstrument(ReadPaymentInstrumentCallback callback,
                                const std::vector<std::string>& data,
                                ServiceWorkerStatusCode status);

  // KeysOfPaymentInstruments callbacks
  void DidFindRegistrationToGetKeys(
      KeysOfPaymentInstrumentsCallback callback,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidGetKeysOfPaymentInstruments(KeysOfPaymentInstrumentsCallback callback,
                                      const std::vector<std::string>& data,
                                      ServiceWorkerStatusCode status);

  // HasPaymentInstrument callbacks
  void DidFindRegistrationToHasPaymentInstrument(
      const std::string& instrument_key,
      HasPaymentInstrumentCallback callback,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidHasPaymentInstrument(DeletePaymentInstrumentCallback callback,
                               const std::vector<std::string>& data,
                               ServiceWorkerStatusCode status);

  // WritePaymentInstrument callbacks
  void DidFindRegistrationToWritePaymentInstrument(
      const std::string& instrument_key,
      payments::mojom::PaymentInstrumentPtr instrument,
      const std::string& decoded_instrument_icon,
      WritePaymentInstrumentCallback callback,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidWritePaymentInstrument(WritePaymentInstrumentCallback callback,
                                 ServiceWorkerStatusCode status);

  // FetchAndUpdatePaymentAppInfo callbacks.
  void FetchPaymentAppInfoCallback(
      const GURL& scope,
      FetchAndUpdatePaymentAppInfoCallback callback,
      std::unique_ptr<PaymentAppInfoFetcher::PaymentAppInfo> app_info);
  void DidFindRegistrationToUpdatePaymentAppInfo(
      FetchAndUpdatePaymentAppInfoCallback callback,
      std::unique_ptr<PaymentAppInfoFetcher::PaymentAppInfo> app_info,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidGetPaymentAppInfoToUpdatePaymentAppInfo(
      FetchAndUpdatePaymentAppInfoCallback callback,
      std::unique_ptr<PaymentAppInfoFetcher::PaymentAppInfo> app_info,
      scoped_refptr<ServiceWorkerRegistration> registration,
      const std::vector<std::string>& data,
      ServiceWorkerStatusCode status);
  void DidUpdatePaymentApp(FetchAndUpdatePaymentAppInfoCallback callback,
                           bool fetch_app_info_failed,
                           ServiceWorkerStatusCode status);

  // PaymentInstrumentIconFetcherCallback.
  void DidFetchedPaymentInstrumentIcon(
      const GURL& scope,
      const std::string& instrument_key,
      payments::mojom::PaymentInstrumentPtr instrument,
      WritePaymentInstrumentCallback callback,
      const std::string& icon);

  // ClearPaymentInstruments callbacks
  void DidFindRegistrationToClearPaymentInstruments(
      const GURL& scope,
      ClearPaymentInstrumentsCallback callback,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidGetKeysToClearPaymentInstruments(
      scoped_refptr<ServiceWorkerRegistration> registration,
      ClearPaymentInstrumentsCallback callback,
      const std::vector<std::string>& keys,
      payments::mojom::PaymentHandlerStatus status);
  void DidClearPaymentInstruments(ClearPaymentInstrumentsCallback callback,
                                  ServiceWorkerStatusCode status);

  // SetPaymentAppUserHint callbacks.
  void DidFindRegistrationToSetPaymentAppUserHint(
      const std::string& user_hint,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidGetPaymentAppInfoToSetUserHint(const std::string& user_hint,
                                         int64_t registration_id,
                                         const GURL& pattern,
                                         const std::vector<std::string>& data,
                                         ServiceWorkerStatusCode status);
  void DidSetPaymentAppUserHint(ServiceWorkerStatusCode status);

  // SetPaymentAppInfoForRegisteredServiceWorker callbacks.
  void DidFindRegistrationToSetPaymentApp(
      const std::string& instrument_key,
      const std::string& name,
      const std::string& icon,
      const std::string& method,
      SetPaymentAppInfoCallback callback,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> registration);
  void DidWritePaymentAppForSetPaymentApp(
      const std::string& instrument_key,
      const std::string& method,
      SetPaymentAppInfoCallback callback,
      scoped_refptr<ServiceWorkerRegistration> registration,
      ServiceWorkerStatusCode status);
  void DidWritePaymentInstrumentForSetPaymentApp(
      SetPaymentAppInfoCallback callback,
      ServiceWorkerStatusCode status);

  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;
  base::WeakPtrFactory<PaymentAppDatabase> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PaymentAppDatabase);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PAYMENTS_PAYMENT_APP_DATABASE_H_
