// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PAYMENTS_PAYMENT_APP_CONTEXT_IMPL_H_
#define CONTENT_BROWSER_PAYMENTS_PAYMENT_APP_CONTEXT_IMPL_H_

#include <map>
#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/atomic_flag.h"
#include "content/browser/payments/payment_app_database.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/platform/modules/payments/payment_app.mojom.h"

namespace content {

class PaymentAppDatabase;
class PaymentManager;
class ServiceWorkerContextWrapper;

// One instance of this exists per StoragePartition, and services multiple child
// processes/origins. Most logic is delegated to the owned PaymentAppDatabase
// instance, which is only accessed on the IO thread.
//
// This class is created/destructed by StoragePartitionImpl on UI thread.
// However, the PaymentAppDatabase that this class has internally should work on
// IO thread. So, this class has Init() and Shutdown() methods in addition to
// constructor and destructor. They should be called explicitly when creating
// and destroying StoragePartitionImpl.
//
// Expected order of lifetime calls:
//   1) Constructor
//   2) Init()
//   3) Can now call other public methods in this class in any order.
//     - Can call CreatePaymentManager() on UI thread.
//     - Can call GetAllPaymentApps() on UI thread.
//     - Can call PaymentManagerHadConnectionError() on IO thread.
//     - Can call payment_app_database() on IO thread.
//   4) Shutdown()
//   5) Destructor
class CONTENT_EXPORT PaymentAppContextImpl
    : public base::RefCountedThreadSafe<
          PaymentAppContextImpl,
          content::BrowserThread::DeleteOnUIThread> {
 public:
  PaymentAppContextImpl();

  // Init and Shutdown are for use on the UI thread when the
  // StoragePartition is being setup and torn down.
  void Init(scoped_refptr<ServiceWorkerContextWrapper> service_worker_context);

  // Shutdown must be called before deleting this. Call on the UI thread.
  void Shutdown();

  // Create a PaymentManager that is owned by this. Call on the UI
  // thread.
  void CreatePaymentManager(payments::mojom::PaymentManagerRequest request);

  // Called by PaymentManager objects so that they can
  // be deleted. Call on the IO thread.
  void PaymentManagerHadConnectionError(PaymentManager* service);

  // Should be accessed only on the IO thread.
  PaymentAppDatabase* payment_app_database() const;

 private:
  friend class PaymentAppContentUnitTestBase;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class base::DeleteHelper<PaymentAppContextImpl>;
  ~PaymentAppContextImpl();

  void CreatePaymentAppDatabaseOnIO(
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context);

  void CreatePaymentManagerOnIO(
      mojo::InterfaceRequest<payments::mojom::PaymentManager> request);

  void ShutdownOnIO();

  // Only accessed on the IO thread.
  std::unique_ptr<PaymentAppDatabase> payment_app_database_;

  // The PaymentManagers are owned by this. They're either deleted during
  // ShutdownOnIO or when the channel is closed via
  // PaymentManagerHadConnectionError. Only accessed on the IO thread.
  std::map<PaymentManager*, std::unique_ptr<PaymentManager>> payment_managers_;

#if DCHECK_IS_ON()
  // Set after ShutdownOnIO() has run on the IO thread. |this| shouldn't be
  // deleted before this is set.
  base::AtomicFlag did_shutdown_on_io_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PaymentAppContextImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PAYMENTS_PAYMENT_APP_CONTEXT_IMPL_H_
