// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_SESSION_KEYS_H_
#define COMPONENTS_CRYPTAUTH_SESSION_KEYS_H_

#include <string>

#include "base/macros.h"

namespace cryptauth {

// This class contains the secure channel (secure context) session keys. This
// class derives (from the master symmetric key) different keys for encryption
// and decryption. The protocol initiator (i.e. the Chromebook) should use
// |initiator_encode_key()| to encrypt the messages, and the responder should
// use |responder_encode_key()|. This is reversed for decryption.
class SessionKeys {
 public:
  // Create session keys derived from the |master_symmetric_key|.
  explicit SessionKeys(const std::string& master_symmetric_key);

  SessionKeys();

  virtual ~SessionKeys();

  virtual std::string initiator_encode_key() const;
  virtual std::string responder_encode_key() const;

 private:
  // The initiator encoding key.
  std::string initiator_encode_key_;

  // The responder encoding key.
  std::string responder_encode_key_;
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_SESSION_KEYS_H_
