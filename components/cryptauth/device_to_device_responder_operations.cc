// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/device_to_device_responder_operations.h"

#include "base/bind.h"
#include "base/callback.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/proto/securemessage.pb.h"
#include "components/cryptauth/secure_message_delegate.h"

namespace cryptauth {

namespace {

// TODO(tengs): Due to a bug with the ChromeOS secure message daemon, we cannot
// create SecureMessages with empty payloads. To workaround this bug, this value
// is put into the payload if it would otherwise be empty.
// See crbug.com/512894.
const char kPayloadFiller[] = "\xae";

// The version to put in the GcmMetadata field.
const int kGcmMetadataVersion = 1;

// The D2D protocol version.
const int kD2DProtocolVersion = 1;

// Callback for DeviceToDeviceResponderOperations::ValidateHelloMessage(),
// after the [Hello] message is unwrapped.
void OnHelloMessageUnwrapped(
    const DeviceToDeviceResponderOperations::ValidateHelloCallback& callback,
    bool verified,
    const std::string& payload,
    const securemessage::Header& header) {
  securemessage::InitiatorHello initiator_hello;
  if (!verified || !initiator_hello.ParseFromString(header.public_metadata()) ||
      initiator_hello.protocol_version() != kD2DProtocolVersion) {
    callback.Run(false, std::string());
    return;
  }

  callback.Run(true, initiator_hello.public_dh_key().SerializeAsString());
}

// Helper struct containing all the context needed to create the [Responder
// Auth] message.
struct CreateResponderAuthMessageContext {
  std::string hello_message;
  std::string session_public_key;
  std::string session_private_key;
  std::string persistent_private_key;
  std::string persistent_symmetric_key;
  SecureMessageDelegate* secure_message_delegate;
  DeviceToDeviceResponderOperations::MessageCallback callback;
  std::string hello_public_key;
  std::string middle_message;
};

// Forward declarations of functions used to create the [Responder Auth]
// message, declared in order in which they are called during the creation flow.
void OnHelloMessageValidatedForResponderAuth(
    CreateResponderAuthMessageContext context,
    bool hello_message_validated,
    const std::string& hello_public_key);
void OnInnerMessageCreatedForResponderAuth(
    CreateResponderAuthMessageContext context,
    const std::string& inner_message);
void OnMiddleMessageCreatedForResponderAuth(
    CreateResponderAuthMessageContext context,
    const std::string& middle_message);
void OnSessionSymmetricKeyDerivedForResponderAuth(
    CreateResponderAuthMessageContext context,
    const std::string& session_symmetric_key);

// Called after the initiator's [Hello] message is unwrapped.
void OnHelloMessageValidatedForResponderAuth(
    CreateResponderAuthMessageContext context,
    bool hello_message_validated,
    const std::string& hello_public_key) {
  if (!hello_message_validated) {
    PA_LOG(INFO) << "Invalid [Hello] while creating [Responder Auth]";
    context.callback.Run(std::string());
    return;
  }

  context.hello_public_key = hello_public_key;

  // Create the inner most wrapped message of [Responder Auth].
  GcmMetadata gcm_metadata;
  gcm_metadata.set_type(UNLOCK_KEY_SIGNED_CHALLENGE);
  gcm_metadata.set_version(kGcmMetadataVersion);

  SecureMessageDelegate::CreateOptions create_options;
  create_options.encryption_scheme = securemessage::NONE;
  create_options.signature_scheme = securemessage::ECDSA_P256_SHA256;
  gcm_metadata.SerializeToString(&create_options.public_metadata);
  create_options.associated_data = context.hello_message;

  context.secure_message_delegate->CreateSecureMessage(
      kPayloadFiller, context.persistent_private_key, create_options,
      base::Bind(&OnInnerMessageCreatedForResponderAuth, context));
}

// Called after the inner-most layer of [Responder Auth] is created.
void OnInnerMessageCreatedForResponderAuth(
    CreateResponderAuthMessageContext context,
    const std::string& inner_message) {
  if (inner_message.empty()) {
    PA_LOG(INFO) << "Failed to create middle message for [Responder Auth]";
    context.callback.Run(std::string());
    return;
  }

  // Create the middle message.
  SecureMessageDelegate::CreateOptions create_options;
  create_options.encryption_scheme = securemessage::AES_256_CBC;
  create_options.signature_scheme = securemessage::HMAC_SHA256;
  create_options.associated_data = context.hello_message;
  context.secure_message_delegate->CreateSecureMessage(
      inner_message, context.persistent_symmetric_key, create_options,
      base::Bind(&OnMiddleMessageCreatedForResponderAuth, context));
}

// Called after the middle layer of [Responder Auth] is created.
void OnMiddleMessageCreatedForResponderAuth(
    CreateResponderAuthMessageContext context,
    const std::string& middle_message) {
  if (middle_message.empty()) {
    PA_LOG(ERROR) << "Error inner message while creating [Responder Auth]";
    context.callback.Run(std::string());
    return;
  }

  // Before we can create the outer-most message layer, we need to perform a key
  // agreement for the session symmetric key.
  context.middle_message = middle_message;
  context.secure_message_delegate->DeriveKey(
      context.session_private_key, context.hello_public_key,
      base::Bind(&OnSessionSymmetricKeyDerivedForResponderAuth, context));
}

// Called after the session symmetric key is derived, so we can create the outer
// most layer of [Responder Auth].
void OnSessionSymmetricKeyDerivedForResponderAuth(
    CreateResponderAuthMessageContext context,
    const std::string& session_symmetric_key) {
  if (session_symmetric_key.empty()) {
    PA_LOG(ERROR) << "Error inner message while creating [Responder Auth]";
    context.callback.Run(std::string());
    return;
  }

  GcmMetadata gcm_metadata;
  gcm_metadata.set_type(DEVICE_TO_DEVICE_RESPONDER_HELLO_PAYLOAD);
  gcm_metadata.set_version(kGcmMetadataVersion);

  // Store the responder's session public key in plaintext in the header.
  securemessage::ResponderHello responder_hello;
  if (!responder_hello.mutable_public_dh_key()->ParseFromString(
          context.session_public_key)) {
    PA_LOG(ERROR) << "Error parsing public key while creating [Responder Auth]";
    PA_LOG(ERROR) << context.session_public_key;
    context.callback.Run(std::string());
    return;
  }
  responder_hello.set_protocol_version(kD2DProtocolVersion);

  // Create the outer most message, wrapping the other messages created
  // previously.
  securemessage::DeviceToDeviceMessage device_to_device_message;
  device_to_device_message.set_message(context.middle_message);
  device_to_device_message.set_sequence_number(1);

  SecureMessageDelegate::CreateOptions create_options;
  create_options.encryption_scheme = securemessage::AES_256_CBC;
  create_options.signature_scheme = securemessage::HMAC_SHA256;
  create_options.public_metadata = gcm_metadata.SerializeAsString();
  responder_hello.SerializeToString(&create_options.decryption_key_id);

  context.secure_message_delegate->CreateSecureMessage(
      device_to_device_message.SerializeAsString(),
      SessionKeys(session_symmetric_key).responder_encode_key(), create_options,
      context.callback);
}

// Helper struct containing all the context needed to validate the [Initiator
// Auth] message.
struct ValidateInitiatorAuthMessageContext {
  std::string persistent_symmetric_key;
  std::string responder_auth_message;
  SecureMessageDelegate* secure_message_delegate;
  DeviceToDeviceResponderOperations::ValidationCallback callback;
};

// Called after the inner-most layer of [Initiator Auth] is unwrapped.
void OnInnerMessageUnwrappedForInitiatorAuth(
    const ValidateInitiatorAuthMessageContext& context,
    bool verified,
    const std::string& payload,
    const securemessage::Header& header) {
  if (!verified)
    PA_LOG(INFO) << "Failed to inner [Initiator Auth] message.";
  context.callback.Run(verified);
}

// Called after the outer-most layer of [Initiator Auth] is unwrapped.
void OnOuterMessageUnwrappedForInitiatorAuth(
    const ValidateInitiatorAuthMessageContext& context,
    bool verified,
    const std::string& payload,
    const securemessage::Header& header) {
  if (!verified) {
    PA_LOG(INFO) << "Failed to verify outer [Initiator Auth] message";
    context.callback.Run(false);
    return;
  }

  // Parse the decrypted payload.
  securemessage::DeviceToDeviceMessage device_to_device_message;
  if (!device_to_device_message.ParseFromString(payload) ||
      device_to_device_message.sequence_number() != 1) {
    PA_LOG(INFO) << "Failed to validate DeviceToDeviceMessage payload.";
    context.callback.Run(false);
    return;
  }

  // Unwrap the inner message of [Initiator Auth].
  SecureMessageDelegate::UnwrapOptions unwrap_options;
  unwrap_options.encryption_scheme = securemessage::AES_256_CBC;
  unwrap_options.signature_scheme = securemessage::HMAC_SHA256;
  unwrap_options.associated_data = context.responder_auth_message;
  context.secure_message_delegate->UnwrapSecureMessage(
      device_to_device_message.message(), context.persistent_symmetric_key,
      unwrap_options,
      base::Bind(&OnInnerMessageUnwrappedForInitiatorAuth, context));
}

}  // namespace

// static
void DeviceToDeviceResponderOperations::ValidateHelloMessage(
    const std::string& hello_message,
    const std::string& persistent_symmetric_key,
    SecureMessageDelegate* secure_message_delegate,
    const ValidateHelloCallback& callback) {
  // The [Hello] message has the structure:
  // {
  //   header: <session_public_key>,
  //           Sig(<session_public_key>, persistent_symmetric_key)
  //   payload: ""
  // }
  SecureMessageDelegate::UnwrapOptions unwrap_options;
  unwrap_options.encryption_scheme = securemessage::NONE;
  unwrap_options.signature_scheme = securemessage::HMAC_SHA256;
  secure_message_delegate->UnwrapSecureMessage(
      hello_message, persistent_symmetric_key, unwrap_options,
      base::Bind(&OnHelloMessageUnwrapped, callback));
}

// static
void DeviceToDeviceResponderOperations::CreateResponderAuthMessage(
    const std::string& hello_message,
    const std::string& session_public_key,
    const std::string& session_private_key,
    const std::string& persistent_private_key,
    const std::string& persistent_symmetric_key,
    SecureMessageDelegate* secure_message_delegate,
    const MessageCallback& callback) {
  // The [Responder Auth] message has the structure:
  // {
  //   header: <responder_public_key>,
  //           Sig(<responder_public_key> + payload1,
  //               session_symmetric_key),
  //   payload1: Enc({
  //     header: Sig(payload2 + <hello_message>, persistent_symmetric_key),
  //     payload2: {
  //       sequence_number: 1,
  //       body: Enc({
  //         header: Sig(payload3 + <hello_message>,
  //                     persistent_responder_public_key),
  //         payload3: ""
  //       }, persistent_symmetric_key)
  //     }
  //   }, session_symmetric_key),
  // }
  CreateResponderAuthMessageContext context = {hello_message,
                                               session_public_key,
                                               session_private_key,
                                               persistent_private_key,
                                               persistent_symmetric_key,
                                               secure_message_delegate,
                                               callback};

  // To create the [Responder Auth] message, we need to first parse the
  // initiator's [Hello] message and extract the initiator's session public key.
  DeviceToDeviceResponderOperations::ValidateHelloMessage(
      hello_message, persistent_symmetric_key, secure_message_delegate,
      base::Bind(&OnHelloMessageValidatedForResponderAuth, context));
}

// static
void DeviceToDeviceResponderOperations::ValidateInitiatorAuthMessage(
    const std::string& initiator_auth_message,
    const SessionKeys& session_keys,
    const std::string& persistent_symmetric_key,
    const std::string& responder_auth_message,
    SecureMessageDelegate* secure_message_delegate,
    const ValidationCallback& callback) {
  // The [Initiator Auth] message has the structure:
  // {
  //   header: Sig(payload1, session_symmetric_key)
  //   payload1: Enc({
  //     sequence_number: 2,
  //     body: {
  //       header: Sig(payload2 + responder_auth_message,
  //       persistent_symmetric_key)
  //       payload2: ""
  //     }
  //   }, session_symmetric_key)
  // }
  ValidateInitiatorAuthMessageContext context = {
      persistent_symmetric_key, responder_auth_message, secure_message_delegate,
      callback};

  SecureMessageDelegate::UnwrapOptions unwrap_options;
  unwrap_options.encryption_scheme = securemessage::AES_256_CBC;
  unwrap_options.signature_scheme = securemessage::HMAC_SHA256;
  secure_message_delegate->UnwrapSecureMessage(
      initiator_auth_message, session_keys.initiator_encode_key(),
      unwrap_options,
      base::Bind(&OnOuterMessageUnwrappedForInitiatorAuth, context));
}

}  // cryptauth
