// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/fake_free_disk_space_getter.h"

#include "components/drive/chromeos/drive_test_util.h"

namespace drive {

FakeFreeDiskSpaceGetter::FakeFreeDiskSpaceGetter()
    : default_value_(test_util::kLotsOfSpace) {
}

FakeFreeDiskSpaceGetter::~FakeFreeDiskSpaceGetter() = default;

void FakeFreeDiskSpaceGetter::PushFakeValue(int64_t value) {
  fake_values_.push_back(value);
}

int64_t FakeFreeDiskSpaceGetter::AmountOfFreeDiskSpace() {
  if (fake_values_.empty())
    return default_value_;

  const int64_t value = fake_values_.front();
  fake_values_.pop_front();
  return value;
}

}  // namespace drive
