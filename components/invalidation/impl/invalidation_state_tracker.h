// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// An InvalidationStateTracker is an interface that handles persisting state
// needed for invalidations. Currently, it is responsible for managing the
// following information:
// - Max version seen from the invalidation server to help dedupe invalidations.
// - Bootstrap data for the invalidation client.
// - Payloads and locally generated ack handles, to support local acking.

#ifndef COMPONENTS_INVALIDATION_IMPL_INVALIDATION_STATE_TRACKER_H_
#define COMPONENTS_INVALIDATION_IMPL_INVALIDATION_STATE_TRACKER_H_

#include <map>
#include <string>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "components/invalidation/impl/unacked_invalidation_set.h"
#include "components/invalidation/public/invalidation.h"
#include "components/invalidation/public/invalidation_export.h"
#include "components/invalidation/public/invalidation_util.h"
#include "google/cacheinvalidation/include/types.h"

namespace syncer {

class INVALIDATION_EXPORT InvalidationStateTracker {
 public:
  InvalidationStateTracker();
  virtual ~InvalidationStateTracker();

  // The per-client unique ID used to register the invalidation client with the
  // server.  This is used to squelch invalidation notifications that originate
  // from changes made by this client.  Setting the client ID clears all other
  // state.
  virtual void ClearAndSetNewClientId(const std::string& data) = 0;
  virtual std::string GetInvalidatorClientId() const = 0;

  // Used by invalidation::InvalidationClient for persistence. |data| is an
  // opaque blob that an invalidation client can use after a restart to
  // bootstrap itself. |data| is binary data (not valid UTF8, embedded nulls,
  // etc).
  virtual void SetBootstrapData(const std::string& data) = 0;
  virtual std::string GetBootstrapData() const = 0;

  // Used to store invalidations that have been acked to the server, but not yet
  // handled by our clients.  We store these invalidations on disk so we won't
  // lose them if we need to restart.
  virtual void SetSavedInvalidations(const UnackedInvalidationsMap& states) = 0;
  virtual UnackedInvalidationsMap GetSavedInvalidations() const = 0;

  // Erases invalidation versions, client ID, and state stored on disk.
  virtual void Clear() = 0;
};

}  // namespace syncer

#endif  // COMPONENTS_INVALIDATION_IMPL_INVALIDATION_STATE_TRACKER_H_
