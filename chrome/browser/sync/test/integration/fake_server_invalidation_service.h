// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_TEST_INTEGRATION_FAKE_SERVER_INVALIDATION_SERVICE_H_
#define CHROME_BROWSER_SYNC_TEST_INTEGRATION_FAKE_SERVER_INVALIDATION_SERVICE_H_

#include <string>
#include <utility>

#include "base/macros.h"
#include "components/invalidation/impl/invalidator_registrar.h"
#include "components/invalidation/public/invalidation_service.h"
#include "components/sync/base/model_type.h"
#include "components/sync/test/fake_server/fake_server.h"

namespace invalidation {
class InvalidationLogger;
}

namespace fake_server {

// An InvalidationService that is used in conjunction with FakeServer.
class FakeServerInvalidationService : public invalidation::InvalidationService,
                                      public FakeServer::Observer {
 public:
  FakeServerInvalidationService();
  ~FakeServerInvalidationService() override;

  void RegisterInvalidationHandler(
      syncer::InvalidationHandler* handler) override;
  bool UpdateRegisteredInvalidationIds(syncer::InvalidationHandler* handler,
                                       const syncer::ObjectIdSet& ids) override;
  void UnregisterInvalidationHandler(
      syncer::InvalidationHandler* handler) override;

  syncer::InvalidatorState GetInvalidatorState() const override;
  std::string GetInvalidatorClientId() const override;
  invalidation::InvalidationLogger* GetInvalidationLogger() override;
  void RequestDetailedStatus(
      base::Callback<void(const base::DictionaryValue&)> caller) const override;

  // Functions to enable or disable sending of self-notifications.  In the real
  // world, clients do not receive notifications of their own commits.
  void EnableSelfNotifications();
  void DisableSelfNotifications();

  // FakeServer::Observer:
  void OnCommit(const std::string& committer_id,
                syncer::ModelTypeSet committed_model_types) override;

 private:
  std::string client_id_;
  bool self_notify_;

  syncer::InvalidatorRegistrar invalidator_registrar_;

  DISALLOW_COPY_AND_ASSIGN(FakeServerInvalidationService);
};

}  // namespace fake_server

#endif  // CHROME_BROWSER_SYNC_TEST_INTEGRATION_FAKE_SERVER_INVALIDATION_SERVICE_H_
