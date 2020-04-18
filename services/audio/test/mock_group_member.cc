// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/test/mock_group_member.h"

namespace audio {

MockGroupMember::MockGroupMember(const base::UnguessableToken& group_id)
    : group_id_(group_id) {}

MockGroupMember::~MockGroupMember() = default;

const base::UnguessableToken& MockGroupMember::GetGroupId() {
  return group_id_;
}

}  // namespace audio
