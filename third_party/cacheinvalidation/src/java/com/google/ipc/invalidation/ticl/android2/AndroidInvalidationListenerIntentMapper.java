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
package com.google.ipc.invalidation.ticl.android2;

import com.google.ipc.invalidation.external.client.InvalidationClient;
import com.google.ipc.invalidation.external.client.InvalidationListener;
import com.google.ipc.invalidation.external.client.InvalidationListener.RegistrationState;
import com.google.ipc.invalidation.external.client.android.service.AndroidLogger;
import com.google.ipc.invalidation.external.client.types.AckHandle;
import com.google.ipc.invalidation.external.client.types.ErrorInfo;
import com.google.ipc.invalidation.ticl.ProtoWrapperConverter;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.ErrorUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.InvalidateUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.RegistrationFailureUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.RegistrationStatusUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.ReissueRegistrationsUpcall;
import com.google.ipc.invalidation.util.ProtoWrapper.ValidationException;

import android.content.Context;
import android.content.Intent;

import java.util.Arrays;


/**
 * Routes intents to the appropriate methods in {@link InvalidationListener}. Typically, an instance
 * of the mapper should be created in {@code IntentService#onCreate} and the {@link #handleIntent}
 * method called in {@code IntentService#onHandleIntent}.
 *
 */
public final class AndroidInvalidationListenerIntentMapper {

  /** The logger. */
  private final AndroidLogger logger = AndroidLogger.forPrefix("");

  /** Client passed to the listener (supports downcalls). */
  public final InvalidationClient client;

  /** Listener to which intents are routed. */
  private final InvalidationListener listener;

  /**
   * Initializes
   *
   * @param listener the listener to which intents should be routed
   * @param context the context used by the listener to issue downcalls to the TICL
   */
  public AndroidInvalidationListenerIntentMapper(InvalidationListener listener, Context context) {
    client = new AndroidInvalidationClientStub(context);
    this.listener = listener;
  }

  /**
   * Handles a listener upcall by decoding the protocol buffer in {@code intent} and dispatching
   * to the appropriate method on the {@link #listener}.
   */
  public void handleIntent(Intent intent) {
    // TODO: use wakelocks

    // Unmarshall the arguments from the Intent and make the appropriate call on the listener.
    ListenerUpcall upcall = tryParseIntent(intent);
    if (upcall == null) {
      return;
    }

    if (upcall.hasReady()) {
      listener.ready(client);
    } else if (upcall.getNullableInvalidate() != null) {
      // Handle all invalidation-related upcalls on a common path, since they require creating
      // an AckHandleP.
      onInvalidateUpcall(upcall.getNullableInvalidate(), listener);
    } else if (upcall.getNullableRegistrationStatus() != null) {
      RegistrationStatusUpcall regStatus = upcall.getNullableRegistrationStatus();
      listener.informRegistrationStatus(client,
          ProtoWrapperConverter.convertFromObjectIdProto(regStatus.getObjectId()),
          regStatus.getIsRegistered() ?
              RegistrationState.REGISTERED : RegistrationState.UNREGISTERED);
    } else if (upcall.getNullableRegistrationFailure() != null) {
      RegistrationFailureUpcall failure = upcall.getNullableRegistrationFailure();
      listener.informRegistrationFailure(client,
          ProtoWrapperConverter.convertFromObjectIdProto(failure.getObjectId()),
          failure.getTransient(),
          failure.getMessage());
    } else if (upcall.getNullableReissueRegistrations() != null) {
      ReissueRegistrationsUpcall reissueRegs = upcall.getNullableReissueRegistrations();
      listener.reissueRegistrations(client, reissueRegs.getPrefix().getByteArray(),
          reissueRegs.getLength());
    } else if (upcall.getNullableError() != null) {
      ErrorUpcall error = upcall.getNullableError();
      ErrorInfo errorInfo = ErrorInfo.newInstance(error.getErrorCode(), error.getIsTransient(),
          error.getErrorMessage(), null);
      listener.informError(client, errorInfo);
    } else {
      logger.warning("Dropping listener Intent with unknown call: %s", upcall);
    }
  }

  /**
   * Handles an invalidation-related listener {@code upcall} by dispatching to the appropriate
   * method on an instance of {@link InvalidationListener}.
   */
  private void onInvalidateUpcall(InvalidateUpcall invalidate, InvalidationListener listener) {
    AckHandle ackHandle = AckHandle.newInstance(invalidate.getAckHandle().getByteArray());
    if (invalidate.getNullableInvalidation() != null) {
      listener.invalidate(client,
          ProtoWrapperConverter.convertFromInvalidationProto(invalidate.getNullableInvalidation()),
          ackHandle);
    } else if (invalidate.hasInvalidateAll()) {
      listener.invalidateAll(client, ackHandle);
    } else if (invalidate.getNullableInvalidateUnknown() != null) {
      listener.invalidateUnknownVersion(client,
          ProtoWrapperConverter.convertFromObjectIdProto(invalidate.getNullableInvalidateUnknown()),
          ackHandle);
    } else {
      throw new RuntimeException("Invalid invalidate upcall: " + invalidate);
    }
  }

  /**
   * Returns a valid {@link ListenerUpcall} from {@code intent}, or {@code null} if one
   * could not be parsed.
   */
  private ListenerUpcall tryParseIntent(Intent intent) {
    if (intent == null) {
      return null;
    }
    byte[] upcallBytes = intent.getByteArrayExtra(ProtocolIntents.LISTENER_UPCALL_KEY);
    if (upcallBytes == null) {
      return null;
    }
    try {
      ListenerUpcall upcall = ListenerUpcall.parseFrom(upcallBytes);
      return upcall;
    } catch (ValidationException exception) {
      logger.severe("Could not parse listener upcall from %s", Arrays.toString(upcallBytes));
      return null;
    }
  }
}
