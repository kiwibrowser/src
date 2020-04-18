// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_UTILS_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_UTILS_H_

#include <string>

#include "build/build_config.h"

class FakeSigninManagerBase;
class FakeSigninManager;
class ProfileOAuth2TokenService;
class SigninManagerBase;

#if defined(OS_CHROMEOS)
using SigninManagerForTest = FakeSigninManagerBase;
#else
using SigninManagerForTest = FakeSigninManager;
#endif  // OS_CHROMEOS

// Test-related utilities that don't fit in either IdentityTestEnvironment or
// IdentityManager itself. NOTE: Using these utilities directly is discouraged,
// but sometimes necessary during conversion. Use IdentityTestEnvironment if
// possible. These utilities should be used directly only if the production code
// is using IdentityManager, but it is not yet feasible to convert the test code
// to use IdentityTestEnvironment. Any such usage should only be temporary,
// i.e., should be followed as quickly as possible by conversion of the test
// code to use IdentityTestEnvironment.
namespace identity {

class IdentityManager;

// Makes the primary account available for the given email address, generating a
// GAIA ID and refresh token that correspond uniquely to that email address. On
// non-ChromeOS, results in the firing of the IdentityManager and SigninManager
// callbacks for signin success. Blocks until the primary account is available.
// NOTE: See disclaimer at top of file re: direct usage.
void MakePrimaryAccountAvailable(SigninManagerBase* signin_manager,
                                 ProfileOAuth2TokenService* token_service,
                                 IdentityManager* identity_manager,
                                 const std::string& email);

// Clears the primary account. On non-ChromeOS, results in the firing of the
// IdentityManager and SigninManager callbacks for signout. Blocks until the
// primary account is cleared.
// Note that this function requires FakeSigninManager, as it internally invokes
// functionality of the fake. If a use case emerges for invoking this
// functionality with a production SigninManager, contact blundell@chromium.org.
// NOTE: See disclaimer at top of file re: direct usage.
void ClearPrimaryAccount(SigninManagerForTest* signin_manager,
                         IdentityManager* identity_manager);

}  // namespace identity

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_IDENTITY_TEST_UTILS_H_
