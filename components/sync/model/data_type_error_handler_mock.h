// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MODEL_DATA_TYPE_ERROR_HANDLER_MOCK_H__
#define COMPONENTS_SYNC_MODEL_DATA_TYPE_ERROR_HANDLER_MOCK_H__

#include <memory>
#include <string>

#include "components/sync/base/model_type.h"
#include "components/sync/model/data_type_error_handler.h"

namespace syncer {

// A mock DataTypeErrorHandler for testing. Set the expected error type with
// ExpectError and OnUnrecoverableError will pass. If the error is not called
// this object's destructor will DCHECK.
class DataTypeErrorHandlerMock : public DataTypeErrorHandler {
 public:
  DataTypeErrorHandlerMock();
  ~DataTypeErrorHandlerMock() override;

  void OnUnrecoverableError(const SyncError& error) override;
  SyncError CreateAndUploadError(const base::Location& location,
                                 const std::string& message,
                                 ModelType type) override;
  std::unique_ptr<DataTypeErrorHandler> Copy() const override;

  // Set the |error_type| to expect.
  void ExpectError(SyncError::ErrorType error_type);

 private:
  // The error type to expect.
  SyncError::ErrorType expected_error_type_ = SyncError::UNSET;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_MODEL_DATA_TYPE_ERROR_HANDLER_MOCK_H__
