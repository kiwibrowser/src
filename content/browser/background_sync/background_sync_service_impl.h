// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_SERVICE_IMPL_H_
#define CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_SERVICE_IMPL_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/background_sync/background_sync_manager.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/modules/background_sync/background_sync.mojom.h"

namespace content {

class BackgroundSyncContext;

class CONTENT_EXPORT BackgroundSyncServiceImpl
    : public blink::mojom::BackgroundSyncService {
 public:
  BackgroundSyncServiceImpl(
      BackgroundSyncContext* background_sync_context,
      mojo::InterfaceRequest<blink::mojom::BackgroundSyncService> request);

  ~BackgroundSyncServiceImpl() override;

 private:
  friend class BackgroundSyncServiceImplTest;

  // blink::mojom::BackgroundSyncService methods:
  void Register(blink::mojom::SyncRegistrationPtr options,
                int64_t sw_registration_id,
                RegisterCallback callback) override;
  void GetRegistrations(int64_t sw_registration_id,
                        GetRegistrationsCallback callback) override;

  void OnRegisterResult(RegisterCallback callback,
                        BackgroundSyncStatus status,
                        std::unique_ptr<BackgroundSyncRegistration> result);
  void OnGetRegistrationsResult(
      GetRegistrationsCallback callback,
      BackgroundSyncStatus status,
      std::vector<std::unique_ptr<BackgroundSyncRegistration>> result);

  // Called when an error is detected on binding_.
  void OnConnectionError();

  // background_sync_context_ owns this.
  BackgroundSyncContext* background_sync_context_;

  mojo::Binding<blink::mojom::BackgroundSyncService> binding_;

  base::WeakPtrFactory<BackgroundSyncServiceImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundSyncServiceImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_SERVICE_IMPL_H_
