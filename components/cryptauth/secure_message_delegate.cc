// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/secure_message_delegate.h"

namespace cryptauth {

SecureMessageDelegate::SecureMessageDelegate() {
}

SecureMessageDelegate::~SecureMessageDelegate() {
}

SecureMessageDelegate::CreateOptions::CreateOptions() {
}

SecureMessageDelegate::CreateOptions::CreateOptions(
    const CreateOptions& other) = default;

SecureMessageDelegate::CreateOptions::~CreateOptions() {
}

SecureMessageDelegate::UnwrapOptions::UnwrapOptions() {
}

SecureMessageDelegate::UnwrapOptions::~UnwrapOptions() {
}

}  // namespace cryptauth
