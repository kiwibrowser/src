// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_SESSION_TYPES_H_
#define ASH_PUBLIC_CPP_SESSION_TYPES_H_

namespace ash {

// The index of the user profile to use. The list is always LRU sorted so that
// index 0 is the currently active user.
using UserIndex = int;

// Represents possible user adding scenarios.
enum class AddUserSessionPolicy {
  // Adding a user is allowed.
  ALLOWED,
  // Disallowed due to primary user's policy.
  ERROR_NOT_ALLOWED_PRIMARY_USER,
  // Disallowed due to no eligible users.
  ERROR_NO_ELIGIBLE_USERS,
  // Disallowed due to reaching maximum supported user.
  ERROR_MAXIMUM_USERS_REACHED,
};

// Defines the cycle direction for |CycleActiveUser|.
enum class CycleUserDirection {
  NEXT = 0,  // Cycle to the next user.
  PREVIOUS,  // Cycle to the previous user.
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_SESSION_TYPES_H_
