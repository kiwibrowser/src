// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/invalidation_helper.h"

#include <string>

#include "google/cacheinvalidation/types.pb.h"

namespace syncer {

bool RealModelTypeToObjectId(ModelType model_type,
                             invalidation::ObjectId* object_id) {
  std::string notification_type;
  if (!RealModelTypeToNotificationType(model_type, &notification_type)) {
    return false;
  }
  object_id->Init(ipc::invalidation::ObjectSource::CHROME_SYNC,
                  notification_type);
  return true;
}

ObjectIdSet ModelTypeSetToObjectIdSet(ModelTypeSet model_types) {
  ObjectIdSet ids;
  for (ModelTypeSet::Iterator it = model_types.First(); it.Good(); it.Inc()) {
    invalidation::ObjectId model_type_as_id;
    if (!RealModelTypeToObjectId(it.Get(), &model_type_as_id)) {
      DLOG(WARNING) << "Invalid model type " << it.Get();
      continue;
    }
    ids.insert(model_type_as_id);
  }
  return ids;
}

}  // namespace syncer
