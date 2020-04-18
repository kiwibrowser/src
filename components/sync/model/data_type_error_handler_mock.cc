// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/data_type_error_handler_mock.h"

namespace syncer {

DataTypeErrorHandlerMock::DataTypeErrorHandlerMock() {}

DataTypeErrorHandlerMock::~DataTypeErrorHandlerMock() {
  DCHECK_EQ(SyncError::UNSET, expected_error_type_);
}

void DataTypeErrorHandlerMock::OnUnrecoverableError(const SyncError& error) {
  DCHECK_NE(SyncError::UNSET, expected_error_type_);
  DCHECK(error.IsSet());
  DCHECK_EQ(expected_error_type_, error.error_type());
  expected_error_type_ = SyncError::UNSET;
}

SyncError DataTypeErrorHandlerMock::CreateAndUploadError(
    const base::Location& location,
    const std::string& message,
    ModelType type) {
  return SyncError(location, SyncError::DATATYPE_ERROR, message, type);
}

void DataTypeErrorHandlerMock::ExpectError(SyncError::ErrorType error_type) {
  DCHECK_EQ(SyncError::UNSET, expected_error_type_);
  expected_error_type_ = error_type;
}

std::unique_ptr<DataTypeErrorHandler> DataTypeErrorHandlerMock::Copy() const {
  return nullptr;
}

}  // namespace syncer
