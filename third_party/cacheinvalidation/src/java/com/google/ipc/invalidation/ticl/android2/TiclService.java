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

import com.google.ipc.invalidation.common.DigestFunction;
import com.google.ipc.invalidation.common.ObjectIdDigestUtils;
import com.google.ipc.invalidation.external.client.types.AckHandle;
import com.google.ipc.invalidation.external.client.types.Callback;
import com.google.ipc.invalidation.external.client.types.ErrorInfo;
import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.ipc.invalidation.external.client.types.SimplePair;
import com.google.ipc.invalidation.external.client.types.Status;
import com.google.ipc.invalidation.ticl.InvalidationClientCore;
import com.google.ipc.invalidation.ticl.PersistenceUtils;
import com.google.ipc.invalidation.ticl.ProtoWrapperConverter;
import com.google.ipc.invalidation.ticl.android2.AndroidInvalidationClientImpl.IntentForwardingListener;
import com.google.ipc.invalidation.ticl.android2.ResourcesFactory.AndroidResources;
import com.google.ipc.invalidation.ticl.proto.AndroidService.AndroidSchedulerEvent;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ClientDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ClientDowncall.RegistrationDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.InternalDowncall;
import com.google.ipc.invalidation.ticl.proto.AndroidService.InternalDowncall.CreateClient;
import com.google.ipc.invalidation.ticl.proto.Client.PersistentTiclState;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ServerToClientMessage;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.ProtoWrapper.ValidationException;

import android.app.IntentService;
import android.content.Intent;

import java.util.Collection;


/**
 * An {@link IntentService} that manages a single Ticl.
 * <p>
 * Concurrency model: {@link IntentService} guarantees that calls to {@link #onHandleIntent} will
 * be executed serially on a dedicated thread. They may perform blocking work without blocking
 * the application calling the service.
 * <p>
 * This thread will be used as the internal-scheduler thread for the Ticl.
 *
 */
public class TiclService extends IntentService {
  /** This class must be public so that Android can instantiate it as a service. */

  /** Resources for the created Ticls. */
  private AndroidResources resources;

  /** The function for computing persistence state digests when rewriting them. */
  private final DigestFunction digestFn = new ObjectIdDigestUtils.Sha1DigestFunction();

  public TiclService() {
    super("TiclService");

    // If the process dies during a call to onHandleIntent, redeliver the intent when the service
    // restarts.
    setIntentRedelivery(true);
  }

  /**
   * Returns the resources to use for a Ticl. Normally, we use a new resources instance
   * for every call, but for existing  tests, we need to be able to override this function
   * and return the same instance each time.
   */
  AndroidResources createResources() {
    return ResourcesFactory.createResources(this, new AndroidClock.SystemClock(), "TiclService");
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    // Note that the Ticl's persistent state is unmarshaled in each of the handle* methods below,
    // and marshaled/written back to storage at the end of the handler.

    // TODO: We may want to use wakelocks to prevent the phone from sleeping
    // before we have finished handling the Intent.

    if (intent == null) {
      return;
    }

    // We create resources anew each time.
    resources = createResources();
    resources.start();
    resources.getLogger().fine("onHandleIntent(%s)", intent);

    try {
      // Dispatch the appropriate handler function based on which extra key is set.
      if (intent.hasExtra(ProtocolIntents.CLIENT_DOWNCALL_KEY)) {
        handleClientDowncall(intent.getByteArrayExtra(ProtocolIntents.CLIENT_DOWNCALL_KEY));
      } else if (intent.hasExtra(ProtocolIntents.INTERNAL_DOWNCALL_KEY)) {
        handleInternalDowncall(intent.getByteArrayExtra(ProtocolIntents.INTERNAL_DOWNCALL_KEY));
      } else if (intent.hasExtra(ProtocolIntents.SCHEDULER_KEY)) {
        handleSchedulerEvent(intent.getByteArrayExtra(ProtocolIntents.SCHEDULER_KEY));
      } else if (intent.hasExtra(ProtocolIntents.IMPLICIT_SCHEDULER_KEY)) {
        handleImplicitSchedulerEvent();
      } else {
        resources.getLogger().warning("Received Intent without any recognized extras: %s", intent);
      }
    } finally {
      // Null out resources to prevent accidentally using them in the future before they have been
      // properly re-created.
      resources.stop();
      resources = null;
    }
  }

  /** Handles a request to call a function on the ticl. */
  private void handleClientDowncall(byte[] clientDowncallBytes) {
    // Parse and validate the request.
    final ClientDowncall downcall;
    try {
      downcall = ClientDowncall.parseFrom(clientDowncallBytes);
    } catch (ValidationException exception) {
      resources.getLogger().warning("Failed parsing ClientDowncall from %s: %s",
          Bytes.toLazyCompactString(clientDowncallBytes), exception.getMessage());
      return;
    }

    resources.getLogger().fine("Handle client downcall: %s", downcall);

    // Restore the appropriate Ticl.
    // TODO: what if this is the "wrong" Ticl?
    AndroidInvalidationClientImpl ticl = loadExistingTicl();
    if (ticl == null) {
      resources.getLogger().warning("Dropping client downcall since no Ticl: %s", downcall);
      return;
    }

    // Call the appropriate method.
    if (downcall.getNullableAck() != null) {
      ticl.acknowledge(
          AckHandle.newInstance(downcall.getNullableAck().getAckHandle().getByteArray()));
    } else if (downcall.hasStart()) {
      ticl.start();
    } else if (downcall.hasStop()) {
      ticl.stop();
    } else if (downcall.getNullableRegistrations() != null) {
      RegistrationDowncall regDowncall = downcall.getNullableRegistrations();
      if (!regDowncall.getRegistrations().isEmpty()) {
        Collection<ObjectId> objects =
            ProtoWrapperConverter.convertFromObjectIdProtoCollection(regDowncall.getRegistrations());
        ticl.register(objects);
      }
      if (!regDowncall.getUnregistrations().isEmpty()) {
        Collection<ObjectId> objects = ProtoWrapperConverter.convertFromObjectIdProtoCollection(
            regDowncall.getUnregistrations());
        ticl.unregister(objects);
      }
    } else {
      throw new RuntimeException("Invalid downcall passed validation: " + downcall);
    }
    // If we are stopping the Ticl, then just delete its persisted in-memory state, since no
    // operations on a stopped Ticl are valid. Otherwise, save the Ticl in-memory state to
    // stable storage.
    if (downcall.hasStop()) {
      TiclStateManager.deleteStateFile(this);
    } else {
      TiclStateManager.saveTicl(this, resources.getLogger(), ticl);
    }
  }

  /** Handles an internal downcall on the Ticl. */
  private void handleInternalDowncall(byte[] internalDowncallBytes) {
    // Parse and validate the request.
    final InternalDowncall downcall;
    try {
      downcall = InternalDowncall.parseFrom(internalDowncallBytes);
    } catch (ValidationException exception) {
      resources.getLogger().warning("Failed parsing InternalDowncall from %s: %s",
          Bytes.toLazyCompactString(internalDowncallBytes),
          exception.getMessage());
      return;
    }
    resources.getLogger().fine("Handle internal downcall: %s", downcall);

    // Message from the data center; just forward it to the Ticl.
    if (downcall.getNullableServerMessage() != null) {
      // We deliver the message regardless of whether the Ticl existed, since we'll want to
      // rewrite persistent state in the case where it did not.
      // TODO: what if this is the "wrong" Ticl?
      AndroidInvalidationClientImpl ticl = TiclStateManager.restoreTicl(this, resources);
      handleServerMessage((ticl != null),
          downcall.getNullableServerMessage().getData().getByteArray());
      if (ticl != null) {
        TiclStateManager.saveTicl(this, resources.getLogger(), ticl);
      }
      return;
    }

    // Network online/offline status change; just forward it to the Ticl.
    if (downcall.getNullableNetworkStatus() != null) {
      // Network status changes only make sense for Ticls that do exist.
      // TODO: what if this is the "wrong" Ticl?
      AndroidInvalidationClientImpl ticl = TiclStateManager.restoreTicl(this, resources);
      if (ticl != null) {
        resources.getNetworkListener().onOnlineStatusChange(
            downcall.getNullableNetworkStatus().getIsOnline());
        TiclStateManager.saveTicl(this, resources.getLogger(), ticl);
      }
      return;
    }

    // Client network address change; just forward it to the Ticl.
    if (downcall.getNetworkAddrChange()) {
      AndroidInvalidationClientImpl ticl = TiclStateManager.restoreTicl(this, resources);
      if (ticl != null) {
        resources.getNetworkListener().onAddressChange();
        TiclStateManager.saveTicl(this, resources.getLogger(), ticl);
      }
      return;
    }

    // Client creation request (meta operation).
    if (downcall.getNullableCreateClient() != null) {
      handleCreateClient(downcall.getNullableCreateClient());
      return;
    }
    throw new RuntimeException(
        "Invalid internal downcall passed validation: " + downcall);
  }

  /** Handles a {@code createClient} request. */
  private void handleCreateClient(CreateClient createClient) {
    // Ensure no Ticl currently exists.
    TiclStateManager.deleteStateFile(this);

    // Create the requested Ticl.
    resources.getLogger().fine("Create client: creating");
    TiclStateManager.createTicl(this, resources, createClient.getClientType(),
        createClient.getClientName().getByteArray(), createClient.getClientConfig(),
        createClient.getSkipStartForTest());
  }

  /**
   * Handles a {@code message} for a {@code ticl}. If the {@code ticl} is started, delivers the
   * message. If the {@code ticl} is not started, drops the message and clears the last message send
   * time in the Ticl persistent storage so that the Ticl will send a heartbeat the next time it
   * starts.
   */
  private void handleServerMessage(boolean isTiclStarted, byte[] message) {
    if (isTiclStarted) {
      // Normal case -- message for a started Ticl. Deliver the message.
      resources.getNetworkListener().onMessageReceived(message);
      return;
    }

    // Even if the client is stopped, attempt to send invalidations if the client is configured to
    // receive them.
    maybeSendBackgroundInvalidationIntent(message);

    // The Ticl isn't started. Rewrite persistent storage so that the last-send-time is a long
    // time ago. The next time the Ticl starts, it will send a message to the data center, which
    // ensures that it will be marked online and that the dropped message (or an equivalent) will
    // be delivered.
    // Android storage implementations are required to execute callbacks inline, so this code
    // all executes synchronously.
    resources.getLogger().fine("Message for unstarted Ticl; rewrite state");
    resources.getStorage().readKey(InvalidationClientCore.CLIENT_TOKEN_KEY,
        new Callback<SimplePair<Status, byte[]>>() {
      @Override
      public void accept(SimplePair<Status, byte[]> result) {
        byte[] stateBytes = result.second;
        if (stateBytes == null) {
          resources.getLogger().info("No persistent state found for client; not rewriting");
          return;
        }
        // Create new state identical to the old state except with a cleared
        // lastMessageSendTimeMs.
        PersistentTiclState state = PersistenceUtils.deserializeState(
            resources.getLogger(), stateBytes, digestFn);
        if (state == null) {
          resources.getLogger().warning("Ignoring invalid Ticl state: %s",
              Bytes.toLazyCompactString(stateBytes));
          return;
        }
        PersistentTiclState.Builder stateBuilder = state.toBuilder();
        stateBuilder.lastMessageSendTimeMs = 0L;
        state = stateBuilder.build();

        // Serialize the new state and write it to storage.
        byte[] newClientState = PersistenceUtils.serializeState(state, digestFn);
        resources.getStorage().writeKey(InvalidationClientCore.CLIENT_TOKEN_KEY, newClientState,
            new Callback<Status>() {
              @Override
              public void accept(Status status) {
                if (status.getCode() != Status.Code.SUCCESS) {
                  resources.getLogger().warning(
                      "Failed saving rewritten persistent state to storage");
                }
              }
        });
      }
    });
  }

  /**
   * If a service is registered to handle them, forward invalidations received while the
   * invalidation client is stopped.
   */
  private void maybeSendBackgroundInvalidationIntent(byte[] message) {
    // If a service is registered to receive background invalidations, parse the message to see if
    // any of them should be forwarded.
    AndroidTiclManifest manifest = new AndroidTiclManifest(getApplicationContext());
    String backgroundServiceClass =
        manifest.getBackgroundInvalidationListenerServiceClass();
    if (backgroundServiceClass != null) {
      try {
        ServerToClientMessage s2cMessage = ServerToClientMessage.parseFrom(message);
        if (s2cMessage.getNullableInvalidationMessage() != null) {
          Intent intent = ProtocolIntents.newBackgroundInvalidationIntent(
              s2cMessage.getNullableInvalidationMessage());
          intent.setClassName(getApplicationContext(), backgroundServiceClass);
          startService(intent);
        }
      } catch (ValidationException exception) {
        resources.getLogger().info("Failed to parse message: %s", exception.getMessage());
      } catch (IllegalStateException exception) {
        resources.getLogger().info(
            "Unable to send background invalidation intent: %s", exception.getMessage());
      }
    }
  }

  /** Handles a request to call a particular recurring task on the Ticl. */
  private void handleSchedulerEvent(byte[] schedulerEventBytes) {
    // Parse and validate the request.
    final AndroidSchedulerEvent event;
    try {
      event = AndroidSchedulerEvent.parseFrom(schedulerEventBytes);
    } catch (ValidationException exception) {
      resources.getLogger().warning("Failed parsing SchedulerEvent from %s: %s",
          Bytes.toLazyCompactString(schedulerEventBytes), exception.getMessage());
      return;
    }

    resources.getLogger().fine("Handle scheduler event: %s", event);

    // Restore the appropriate Ticl.
    AndroidInvalidationClientImpl ticl = TiclStateManager.restoreTicl(this, resources);

    // If the Ticl didn't exist, drop the event.
    if (ticl == null) {
      resources.getLogger().fine("Dropping event %s; Ticl state does not exist",
          event.getEventName());
      return;
    }

    // Invoke the appropriate event.
    AndroidInternalScheduler ticlScheduler =
        (AndroidInternalScheduler) resources.getInternalScheduler();
    ticlScheduler.handleSchedulerEvent(event);

    // Save the Ticl state to persistent storage.
    TiclStateManager.saveTicl(this, resources.getLogger(), ticl);
  }

  /** Handles a request to call a particular recurring task on the Ticl. */
  private void handleImplicitSchedulerEvent() {
    resources.getLogger().fine("Handle implicit scheduler event");

    // Restore the appropriate Ticl.
    AndroidInvalidationClientImpl ticl = TiclStateManager.restoreTicl(this, resources);

    // If the Ticl didn't exist, drop the event.
    if (ticl == null) {
      resources.getLogger().fine("Dropping implicit scheduling event; Ticl state does not exist");
      return;
    }

    // Invoke the appropriate event.
    AndroidInternalScheduler ticlScheduler =
        (AndroidInternalScheduler) resources.getInternalScheduler();
    ticlScheduler.handleImplicitSchedulerEvent();

    // Save the Ticl state to persistent storage.
    TiclStateManager.saveTicl(this, resources.getLogger(), ticl);
  }

  /**
   * Returns the existing Ticl from persistent storage, or {@code null} if it does not exist.
   * If it does not exist, raises an error to the listener. This function should be used
   * only when loading a Ticl in response to a client-application call, since it raises an error
   * back to the application.
   */
  private AndroidInvalidationClientImpl loadExistingTicl() {
    AndroidInvalidationClientImpl ticl = TiclStateManager.restoreTicl(this, resources);
    if (ticl == null) {
      informListenerOfPermanentError("Client does not exist on downcall");
    }
    return ticl;
  }

  /** Informs the listener of a non-retryable {@code error}. */
  private void informListenerOfPermanentError(final String error) {
    ErrorInfo errorInfo = ErrorInfo.newInstance(0, false, error, null);
    Intent errorIntent = ProtocolIntents.ListenerUpcalls.newErrorIntent(errorInfo);
    IntentForwardingListener.issueIntent(this, errorIntent);
  }

  /** Returns the resources used for the current Ticl. */
  AndroidResources getSystemResourcesForTest() {
    return resources;
  }
}
