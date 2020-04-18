// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/change_processor_mock.h"

#include "base/compiler_specific.h"

namespace syncer {

ChangeProcessorMock::ChangeProcessorMock() : ChangeProcessor(nullptr) {}

ChangeProcessorMock::~ChangeProcessorMock() {}

}  // namespace syncer
