/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.ipc.invalidation.external.client.contrib;

import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.ipc.invalidation.ticl.ProtoWrapperConverter;
import com.google.ipc.invalidation.ticl.TiclExponentialBackoffDelayGenerator;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.AndroidListenerState;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.AndroidListenerState.RetryRegistrationState;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.AndroidListenerState.ScheduledRegistrationRetry;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.RegistrationCommand;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.StartCommand;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.util.Bytes;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Static helper class supporting construction of valid {code AndroidListenerProtocol} messages.
 *
 */
class AndroidListenerProtos {

  /** Creates a retry register command for the given object and client. */
  static RegistrationCommand newDelayedRegisterCommand(Bytes clientId, ObjectId objectId) {
    final boolean isRegister = true;
    return newDelayedRegistrationCommand(clientId, objectId, isRegister);
  }

  /** Creates a retry unregister command for the given object and client. */
  static RegistrationCommand newDelayedUnregisterCommand(Bytes clientId, ObjectId objectId) {
    final boolean isRegister = false;
    return newDelayedRegistrationCommand(clientId, objectId, isRegister);
  }

  /** Creates proto for {@link AndroidListener} state. */
  static AndroidListenerState newAndroidListenerState(Bytes clientId, int requestCodeSeqNum,
      Map<ObjectId, TiclExponentialBackoffDelayGenerator> delayGenerators,
      Collection<ObjectId> desiredRegistrations,
      Collection<ScheduledRegistrationRetry> registrationRetries) {
    ArrayList<RetryRegistrationState> retryRegistrationState =
        new ArrayList<RetryRegistrationState>(delayGenerators.size());
    for (Entry<ObjectId, TiclExponentialBackoffDelayGenerator> entry : delayGenerators.entrySet()) {
      retryRegistrationState.add(
          newRetryRegistrationState(entry.getKey(), entry.getValue()));
    }
    return AndroidListenerState.create(
        ProtoWrapperConverter.convertToObjectIdProtoCollection(desiredRegistrations),
        retryRegistrationState, clientId, requestCodeSeqNum, registrationRetries);
  }

  /** Creates proto for retry registration state. */
  static RetryRegistrationState newRetryRegistrationState(ObjectId objectId,
      TiclExponentialBackoffDelayGenerator delayGenerator) {
    return RetryRegistrationState.create(ProtoWrapperConverter.convertToObjectIdProto(objectId),
        delayGenerator.marshal());
  }

  /** Returns {@code true} iff the given proto is valid. */
  static boolean isValidAndroidListenerState(AndroidListenerState state) {
    return state.hasClientId() && state.hasRequestCodeSeqNum();
  }

  /** Returns {@code true} iff the given proto is valid. */
  static boolean isValidRegistrationCommand(RegistrationCommand command) {
    return command.hasIsRegister() && command.hasClientId() && command.hasIsDelayed();
  }

  /** Returns {@code true} iff the given proto is valid. */
  static boolean isValidStartCommand(StartCommand command) {
    return command.hasClientType() && command.hasClientName();
  }

  /** Creates start command proto. */
  static StartCommand newStartCommand(int clientType, Bytes clientName,
      boolean allowSuppression) {
    return StartCommand.create(clientType, clientName, allowSuppression);
  }

  static RegistrationCommand newRegistrationCommand(Bytes clientId,
      Iterable<ObjectId> objectIds, boolean isRegister) {
    return RegistrationCommand.create(isRegister,
        ProtoWrapperConverter.convertToObjectIdProtoCollection(objectIds), clientId,
        /* isDelayed */ false);
  }

  private static RegistrationCommand newDelayedRegistrationCommand(Bytes clientId,
      ObjectId objectId, boolean isRegister) {
    List<ObjectIdP> objectIds = new ArrayList<ObjectIdP>(1);
    objectIds.add(ProtoWrapperConverter.convertToObjectIdProto(objectId));
    return RegistrationCommand.create(isRegister, objectIds, clientId, /* isDelayed */ true);
  }

  // Prevent instantiation.
  private AndroidListenerProtos() {
  }
}
