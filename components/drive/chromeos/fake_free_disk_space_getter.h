// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FAKE_FREE_DISK_SPACE_GETTER_H_
#define COMPONENTS_DRIVE_CHROMEOS_FAKE_FREE_DISK_SPACE_GETTER_H_

#include <stdint.h>

#include <list>

#include "base/macros.h"
#include "components/drive/chromeos/file_cache.h"

namespace drive {

// This class is used to report fake free disk space. In particular, this
// class can be used to simulate a case where disk is full, or nearly full.
class FakeFreeDiskSpaceGetter : public internal::FreeDiskSpaceGetterInterface {
 public:
  FakeFreeDiskSpaceGetter();
  ~FakeFreeDiskSpaceGetter() override;

  void set_default_value(int64_t value) { default_value_ = value; }

  // Pushes the given value to the back of the fake value list.
  //
  // If the fake value list is empty, AmountOfFreeDiskSpace() will return
  // |default_value_| repeatedly.
  // Otherwise, AmountOfFreeDiskSpace() will return the value at the front of
  // the list and removes it from the list.
  void PushFakeValue(int64_t value);

  // FreeDiskSpaceGetterInterface overrides.
  int64_t AmountOfFreeDiskSpace() override;

 private:
  std::list<int64_t> fake_values_;
  int64_t default_value_;

  DISALLOW_COPY_AND_ASSIGN(FakeFreeDiskSpaceGetter);
};

}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FAKE_FREE_DISK_SPACE_GETTER_H_
