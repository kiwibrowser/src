// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MODEL_CHANGE_PROCESSOR_MOCK_H_
#define COMPONENTS_SYNC_MODEL_CHANGE_PROCESSOR_MOCK_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "components/sync/base/model_type.h"
#include "components/sync/base/unrecoverable_error_handler.h"
#include "components/sync/model/change_processor.h"
#include "components/sync/model/data_type_error_handler.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace syncer {

class ChangeProcessorMock : public ChangeProcessor {
 public:
  ChangeProcessorMock();
  ~ChangeProcessorMock() override;
  MOCK_METHOD3(ApplyChangesFromSyncModel,
               void(const BaseTransaction*,
                    int64_t,
                    const ImmutableChangeRecordList&));
  MOCK_METHOD0(CommitChangesFromSyncModel, void());
  MOCK_METHOD0(StartImpl, void());
  MOCK_CONST_METHOD0(IsRunning, bool());
  MOCK_METHOD2(OnUnrecoverableError,
               void(const base::Location&, const std::string&));
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_MODEL_CHANGE_PROCESSOR_MOCK_H_
