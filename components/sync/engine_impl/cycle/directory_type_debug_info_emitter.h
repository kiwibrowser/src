// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_DIRECTORY_TYPE_DEBUG_INFO_EMITTER_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_DIRECTORY_TYPE_DEBUG_INFO_EMITTER_H_

#include "base/macros.h"
#include "components/sync/engine_impl/cycle/data_type_debug_info_emitter.h"
#include "components/sync/syncable/directory.h"

namespace syncer {

class DirectoryTypeDebugInfoEmitter : public DataTypeDebugInfoEmitter {
 public:
  // Standard constructor for non-tests.
  DirectoryTypeDebugInfoEmitter(
      syncable::Directory* directory,
      ModelType type,
      base::ObserverList<TypeDebugInfoObserver>* observers);

  // A simple constructor for tests.  Should not be used in real code.
  DirectoryTypeDebugInfoEmitter(
      ModelType type,
      base::ObserverList<TypeDebugInfoObserver>* observers);

  ~DirectoryTypeDebugInfoEmitter() override;

  // Triggers a status counters update to registered observers.
  void EmitStatusCountersUpdate() override;

 private:
  syncable::Directory* directory_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryTypeDebugInfoEmitter);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_CYCLE_DIRECTORY_TYPE_DEBUG_INFO_EMITTER_H_
