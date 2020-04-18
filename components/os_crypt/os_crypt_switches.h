// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef COMPONENTS_OS_CRYPT_OS_CRYPT_SWITCHES_H_
#define COMPONENTS_OS_CRYPT_OS_CRYPT_SWITCHES_H_

// Defines all the command-line switches used by the encryptor component.

#include "build/build_config.h"

namespace os_crypt {
namespace switches {

#if defined(OS_MACOSX)

// Uses mock keychain for testing purposes, which prevents blocking dialogs
// from causing timeouts.
extern const char kUseMockKeychain[];

#endif  // OS_MACOSX

}  // namespace switches
}  // namespace os_crypt

#endif  // COMPONENTS_OS_CRYPT_OS_CRYPT_SWITCHES_H_
