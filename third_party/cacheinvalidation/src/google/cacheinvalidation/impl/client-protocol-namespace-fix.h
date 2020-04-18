// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Brings invalidation client protocol buffers into invalidation namespace.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_CLIENT_PROTOCOL_NAMESPACE_FIX_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_CLIENT_PROTOCOL_NAMESPACE_FIX_H_

#include "google/cacheinvalidation/client.pb.h"
#include "google/cacheinvalidation/client_protocol.pb.h"
#include "google/cacheinvalidation/types.pb.h"
#include "google/cacheinvalidation/impl/repeated-field-namespace-fix.h"

namespace invalidation {

// Client
using ::ipc::invalidation::PersistentStateBlob;
using ::ipc::invalidation::PersistentTiclState;

// ClientProtocol
using ::ipc::invalidation::AckHandleP;
using ::ipc::invalidation::ApplicationClientIdP;
using ::ipc::invalidation::ClientConfigP;
using ::ipc::invalidation::ClientHeader;
using ::ipc::invalidation::ClientVersion;
using ::ipc::invalidation::ClientToServerMessage;
using ::ipc::invalidation::ConfigChangeMessage;
using ::ipc::invalidation::ErrorMessage;
using ::ipc::invalidation::ErrorMessage_Code_AUTH_FAILURE;
using ::ipc::invalidation::ErrorMessage_Code_UNKNOWN_FAILURE;
using ::ipc::invalidation::InfoMessage;
using ::ipc::invalidation::InfoRequestMessage;
using ::ipc::invalidation::InfoRequestMessage_InfoType;
using ::ipc::invalidation::InfoRequestMessage_InfoType_GET_PERFORMANCE_COUNTERS;
using ::ipc::invalidation::InitializeMessage;
using ::ipc::invalidation::InitializeMessage_DigestSerializationType_BYTE_BASED;
using ::ipc::invalidation::InitializeMessage_DigestSerializationType_NUMBER_BASED;
using ::ipc::invalidation::InvalidationMessage;
using ::ipc::invalidation::InvalidationP;
using ::ipc::invalidation::ObjectIdP;
using ::ipc::invalidation::PropertyRecord;
using ::ipc::invalidation::ProtocolHandlerConfigP;
using ::ipc::invalidation::ProtocolVersion;
using ::ipc::invalidation::RateLimitP;
using ::ipc::invalidation::RegistrationMessage;
using ::ipc::invalidation::RegistrationP;
using ::ipc::invalidation::RegistrationP_OpType_REGISTER;
using ::ipc::invalidation::RegistrationP_OpType_UNREGISTER;
using ::ipc::invalidation::RegistrationMessage;
using ::ipc::invalidation::RegistrationStatus;
using ::ipc::invalidation::RegistrationStatusMessage;
using ::ipc::invalidation::RegistrationSubtree;
using ::ipc::invalidation::RegistrationSummary;
using ::ipc::invalidation::RegistrationSyncMessage;
using ::ipc::invalidation::RegistrationSyncRequestMessage;
using ::ipc::invalidation::ServerHeader;
using ::ipc::invalidation::ServerToClientMessage;
using ::ipc::invalidation::StatusP;
using ::ipc::invalidation::StatusP_Code_SUCCESS;
using ::ipc::invalidation::StatusP_Code_PERMANENT_FAILURE;
using ::ipc::invalidation::StatusP_Code_TRANSIENT_FAILURE;
using ::ipc::invalidation::TokenControlMessage;
using ::ipc::invalidation::Version;

// Types
using ::ipc::invalidation::ObjectSource_Type_INTERNAL;

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_CLIENT_PROTOCOL_NAMESPACE_FIX_H_
