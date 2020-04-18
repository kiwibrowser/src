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

import com.google.ipc.invalidation.external.client.types.ErrorInfo;
import com.google.ipc.invalidation.ticl.proto.AndroidService.AndroidNetworkSendRequest;
import com.google.ipc.invalidation.ticl.proto.AndroidService.AndroidSchedulerEvent;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ClientDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ClientDowncall.AckDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ClientDowncall.RegistrationDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ClientDowncall.StartDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ClientDowncall.StopDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.InternalDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.InternalDowncall.CreateClient;
import com.google.ipc.invalidation.ticl.proto.AndroidService.InternalDowncall.NetworkStatus;
import com.google.ipc.invalidation.ticl.proto.AndroidService.InternalDowncall.ServerMessage;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.ErrorUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.InvalidateUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.ReadyUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.RegistrationFailureUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.RegistrationStatusUpcall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ListenerUpcall.ReissueRegistrationsUpcall;
import com.google.ipc.invalidation.ticl.proto.Client.AckHandleP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientConfigP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.Version;
import com.google.ipc.invalidation.util.Bytes;

import android.content.Intent;

import java.util.Collection;

/**
 * Factory class for {@link Intent}s used between the application, Ticl, and listener in the
 * Android Ticl.
 *
 */
public class ProtocolIntents {
  /** Version of the on-device protocol. */
  static final Version ANDROID_PROTOCOL_VERSION_VALUE = Version.create(1, 0);

  /** Key of Intent byte[] extra holding a client downcall protocol buffer. */
  public static final String CLIENT_DOWNCALL_KEY = "ipcinv-downcall";

  /** Key of Intent byte[] extra holding an internal downcall protocol buffer. */
  public static final String INTERNAL_DOWNCALL_KEY = "ipcinv-internal-downcall";

  /** Key of Intent byte[] extra holding a listener upcall protocol buffer. */
  public static final String LISTENER_UPCALL_KEY = "ipcinv-upcall";

  /** Key of Intent byte[] extra holding a schedule event protocol buffer. */
  public static final String SCHEDULER_KEY = "ipcinv-scheduler";

  /** Key of Intent byte[] extra holding a schedule event protocol buffer. */
  public static final String IMPLICIT_SCHEDULER_KEY = "ipcinv-implicit-scheduler";

  /** Key of Intent byte[] extra holding an outbound message protocol buffer. */
  public static final String OUTBOUND_MESSAGE_KEY = "ipcinv-outbound-message";

  /** Key of Intent byte[] extra holding an invalidation message protocol buffer. */
  public static final String BACKGROUND_INVALIDATION_KEY = "ipcinv-background-inv";

  /** Intents corresponding to calls on {@code InvalidationClient}. */
  public static class ClientDowncalls {
    public static Intent newStartIntent() {
      Intent intent = new Intent();
      intent.putExtra(CLIENT_DOWNCALL_KEY, ClientDowncall.createWithStart(
          ANDROID_PROTOCOL_VERSION_VALUE, StartDowncall.DEFAULT_INSTANCE).toByteArray());
      return intent;
    }

    public static Intent newStopIntent() {
      Intent intent = new Intent();
      intent.putExtra(CLIENT_DOWNCALL_KEY, ClientDowncall.createWithStop(
          ANDROID_PROTOCOL_VERSION_VALUE, StopDowncall.DEFAULT_INSTANCE).toByteArray());
      return intent;
    }

    public static Intent newAcknowledgeIntent(byte[] ackHandleData) {
      AckDowncall ackDowncall = AckDowncall.create(new Bytes(ackHandleData));
      Intent intent = new Intent();
      intent.putExtra(CLIENT_DOWNCALL_KEY, ClientDowncall.createWithAck(
          ANDROID_PROTOCOL_VERSION_VALUE, ackDowncall).toByteArray());
      return intent;
    }

    public static Intent newRegistrationIntent(Collection<ObjectIdP> registrations) {
      RegistrationDowncall regDowncall =
          RegistrationDowncall.createWithRegistrations(registrations);
      Intent intent = new Intent();
      intent.putExtra(CLIENT_DOWNCALL_KEY, ClientDowncall.createWithRegistrations(
          ANDROID_PROTOCOL_VERSION_VALUE, regDowncall).toByteArray());
      return intent;
    }

    public static Intent newUnregistrationIntent(Collection<ObjectIdP> unregistrations) {
      RegistrationDowncall regDowncall =
          RegistrationDowncall.createWithUnregistrations(unregistrations);
      Intent intent = new Intent();
      intent.putExtra(CLIENT_DOWNCALL_KEY, ClientDowncall.createWithRegistrations(
          ANDROID_PROTOCOL_VERSION_VALUE, regDowncall).toByteArray());
      return intent;
    }

    private ClientDowncalls() {
      // Disallow instantiation.
    }
  }

  /** Intents for non-public calls on the Ticl (currently, network-related calls. */
  public static class InternalDowncalls {
    public static Intent newServerMessageIntent(Bytes serverMessage) {
      Intent intent = new Intent();
      intent.putExtra(INTERNAL_DOWNCALL_KEY, InternalDowncall.createWithServerMessage(
          ANDROID_PROTOCOL_VERSION_VALUE, ServerMessage.create(serverMessage))
          .toByteArray());
      return intent;
    }

    public static Intent newNetworkStatusIntent(Boolean status) {
      Intent intent = new Intent();
      intent.putExtra(INTERNAL_DOWNCALL_KEY, InternalDowncall.createWithNetworkStatus(
          ANDROID_PROTOCOL_VERSION_VALUE, NetworkStatus.create(status)).toByteArray());
      return intent;
    }

    public static Intent newNetworkAddrChangeIntent() {
      Intent intent = new Intent();
      intent.putExtra(INTERNAL_DOWNCALL_KEY, InternalDowncall.createWithNetworkAddrChange(
          ANDROID_PROTOCOL_VERSION_VALUE, true).toByteArray());
      return intent;
    }

    public static Intent newCreateClientIntent(int clientType, Bytes clientName,
        ClientConfigP config, boolean skipStartForTest) {
      CreateClient createClient =
          CreateClient.create(clientType, clientName, config, skipStartForTest);
      Intent intent = new Intent();
      intent.putExtra(INTERNAL_DOWNCALL_KEY, InternalDowncall.createWithCreateClient(
          ANDROID_PROTOCOL_VERSION_VALUE, createClient).toByteArray());
      return intent;
    }

    private InternalDowncalls() {
      // Disallow instantiation.
    }
  }

  /** Intents corresponding to calls on {@code InvalidationListener}. */
  public static class ListenerUpcalls {
    public static Intent newReadyIntent() {
      Intent intent = new Intent();
      intent.putExtra(LISTENER_UPCALL_KEY, ListenerUpcall.createWithReady(
          ANDROID_PROTOCOL_VERSION_VALUE, ReadyUpcall.DEFAULT_INSTANCE).toByteArray());
      return intent;
    }

    public static Intent newInvalidateIntent(InvalidationP invalidation, AckHandleP ackHandle) {
      Intent intent = new Intent();
      InvalidateUpcall invUpcall =
          InvalidateUpcall.createWithInvalidation(new Bytes(ackHandle.toByteArray()), invalidation);
      intent.putExtra(LISTENER_UPCALL_KEY, ListenerUpcall.createWithInvalidate(
          ANDROID_PROTOCOL_VERSION_VALUE, invUpcall).toByteArray());
      return intent;
    }

    public static Intent newInvalidateUnknownIntent(ObjectIdP object, AckHandleP ackHandle) {
      Intent intent = new Intent();
      InvalidateUpcall invUpcall =
          InvalidateUpcall.createWithInvalidateUnknown(new Bytes(ackHandle.toByteArray()), object);
      intent.putExtra(LISTENER_UPCALL_KEY, ListenerUpcall.createWithInvalidate(
          ANDROID_PROTOCOL_VERSION_VALUE, invUpcall).toByteArray());
      return intent;
    }

    public static Intent newInvalidateAllIntent(AckHandleP ackHandle) {
      Intent intent = new Intent();
      InvalidateUpcall invUpcall = InvalidateUpcall.createWithInvalidateAll(
          new Bytes(ackHandle.toByteArray()), true);
      intent.putExtra(LISTENER_UPCALL_KEY, ListenerUpcall.createWithInvalidate(
          ANDROID_PROTOCOL_VERSION_VALUE, invUpcall).toByteArray());
      return intent;
    }

    public static Intent newRegistrationStatusIntent(ObjectIdP object, boolean isRegistered) {
      Intent intent = new Intent();
      RegistrationStatusUpcall regUpcall = RegistrationStatusUpcall.create(object, isRegistered);
      intent.putExtra(LISTENER_UPCALL_KEY, ListenerUpcall.createWithRegistrationStatus(
          ANDROID_PROTOCOL_VERSION_VALUE, regUpcall).toByteArray());
      return intent;
    }

    public static Intent newRegistrationFailureIntent(ObjectIdP object, boolean isTransient,
        String message) {
      Intent intent = new Intent();
      RegistrationFailureUpcall regUpcall =
          RegistrationFailureUpcall.create(object, isTransient, message);
      intent.putExtra(LISTENER_UPCALL_KEY, ListenerUpcall.createWithRegistrationFailure(
          ANDROID_PROTOCOL_VERSION_VALUE, regUpcall).toByteArray());
      return intent;
    }

    public static Intent newReissueRegistrationsIntent(byte[] prefix, int length) {
      Intent intent = new Intent();
      ReissueRegistrationsUpcall reissueRegistrations =
          ReissueRegistrationsUpcall.create(new Bytes(prefix), length);
      intent.putExtra(LISTENER_UPCALL_KEY, ListenerUpcall.createWithReissueRegistrations(
          ANDROID_PROTOCOL_VERSION_VALUE, reissueRegistrations).toByteArray());
      return intent;
    }

    public static Intent newErrorIntent(ErrorInfo errorInfo) {
      Intent intent = new Intent();
      ErrorUpcall errorUpcall = ErrorUpcall.create(errorInfo.getErrorReason(),
          errorInfo.getErrorMessage(), errorInfo.isTransient());
      intent.putExtra(LISTENER_UPCALL_KEY, ListenerUpcall.createWithError(
          ANDROID_PROTOCOL_VERSION_VALUE, errorUpcall).toByteArray());
      return intent;
    }

    private ListenerUpcalls() {
      // Disallow instantiation.
    }
  }

  /** Returns a new intent encoding a request to execute the scheduled action {@code eventName}. */
  public static Intent newSchedulerIntent(String eventName, long ticlId) {
    byte[] eventBytes = AndroidSchedulerEvent.create(ANDROID_PROTOCOL_VERSION_VALUE, eventName,
        ticlId).toByteArray();
    return new Intent().putExtra(SCHEDULER_KEY, eventBytes);
  }

  /** Returns a new intent encoding a request to execute the scheduled action {@code eventName}. */
  public static Intent newImplicitSchedulerIntent() {
    return new Intent().putExtra(IMPLICIT_SCHEDULER_KEY, true);
  }

  /** Returns a new intent encoding a message to send to the data center. */
  public static Intent newOutboundMessageIntent(byte[] message) {
    byte[] payloadBytes = AndroidNetworkSendRequest.create(ANDROID_PROTOCOL_VERSION_VALUE,
        new Bytes(message)).toByteArray();
    return new Intent().putExtra(OUTBOUND_MESSAGE_KEY, payloadBytes);
  }

  /** Returns a new intent encoding background invalidation messages. */
  public static Intent newBackgroundInvalidationIntent(InvalidationMessage invalidationMessage) {
    byte[] payloadBytes = invalidationMessage.toByteArray();
    return new Intent().putExtra(BACKGROUND_INVALIDATION_KEY, payloadBytes);
  }

  private ProtocolIntents() {
    // Disallow instantiation.
  }
}
