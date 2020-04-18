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
import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.android.service.AndroidLogger;
import com.google.ipc.invalidation.external.client.types.AckHandle;
import com.google.ipc.invalidation.external.client.types.ErrorInfo;
import com.google.ipc.invalidation.external.client.types.Invalidation;
import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.ipc.invalidation.ticl.InvalidationClientCore;
import com.google.ipc.invalidation.ticl.ProtoWrapperConverter;
import com.google.ipc.invalidation.ticl.android2.ProtocolIntents.ListenerUpcalls;
import com.google.ipc.invalidation.ticl.proto.AndroidService.AndroidTiclState;
import com.google.ipc.invalidation.ticl.proto.Client.AckHandleP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ApplicationClientIdP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientConfigP;
import com.google.ipc.invalidation.util.Preconditions;
import com.google.ipc.invalidation.util.ProtoWrapper.ValidationException;

import android.app.Service;
import android.content.Context;
import android.content.Intent;

import java.util.Arrays;
import java.util.Map;
import java.util.Random;


/**
 * Android specialization of {@link InvalidationClientCore}. Configures the internal scheduler of
 * the provided resources with references to the recurring tasks in the Ticl and also provides
 * an {@link InvalidationListener} instance to the Ticl that will forward upcalls to the
 * actual application listener using {@link Intent}s.
 * <p>
 * This class requires that {@code SystemResources} {@code Storage} implementations be synchronous.
 * I.e., they must invoke their callbacks inline. We require this because it is very difficult
 * to handle asynchrony in an Android {@code IntentService}. Every async point requires marshalling
 * the Ticl state to disk. Additionally, we must be able to resume processing where we left off;
 * i.e., we must be able to (morally) save the value of the program counter. Intents, unlike Java
 * callbacks, do not implicitly save the PC value, so we need to manually encode it in Intent
 * data. This is extremely awkward, so we avoid asynchrony in the storage API.
 *
 */
class AndroidInvalidationClientImpl extends InvalidationClientCore {
  /** Logger from Ticl resources. */
  private static final Logger staticLogger = AndroidLogger.forTag("InvClientImpl");

  /** Class implementing the application listener stub (allows overriding default for tests). */
  static Class<? extends Service> listenerServiceClassForTest = null;

  /**
   * {@link InvalidationListener} implementation that forwards all calls to a remote listener
   * using Android intents.
   */
  static class IntentForwardingListener implements InvalidationListener {

    /** Android system context. */
    private final Context context;

    /** Logger from Ticl resources. */
    private final Logger logger;

    IntentForwardingListener(Context context, Logger logger) {
      this.context = Preconditions.checkNotNull(context);
      this.logger = Preconditions.checkNotNull(logger);
    }

    // All calls are implemented by marshalling the arguments to an Intent and sending the Intent
    // to the application.

    @Override
    public void ready(InvalidationClient client) {
      issueIntent(context, ListenerUpcalls.newReadyIntent());
    }

    @Override
    public void invalidate(InvalidationClient client, Invalidation invalidation,
        AckHandle ackHandle) {
      try {
        AckHandleP ackHandleP = AckHandleP.parseFrom(ackHandle.getHandleData());
        issueIntent(context, ListenerUpcalls.newInvalidateIntent(
            ProtoWrapperConverter.convertToInvalidationProto(invalidation), ackHandleP));
      } catch (ValidationException exception) {
        // Log and drop invalid call.
        logBadAckHandle("invalidate", ackHandle);
      }
    }

    @Override
    public void invalidateUnknownVersion(InvalidationClient client, ObjectId objectId,
        AckHandle ackHandle) {
      try {
        AckHandleP ackHandleP = AckHandleP.parseFrom(ackHandle.getHandleData());
        issueIntent(context, ListenerUpcalls.newInvalidateUnknownIntent(
            ProtoWrapperConverter.convertToObjectIdProto(objectId), ackHandleP));
      } catch (ValidationException exception) {
        // Log and drop invalid call.
        logBadAckHandle("invalidateUnknownVersion", ackHandle);
      }
    }

    @Override
    public void invalidateAll(InvalidationClient client, AckHandle ackHandle) {
      try {
        AckHandleP ackHandleP = AckHandleP.parseFrom(ackHandle.getHandleData());
        issueIntent(context, ListenerUpcalls.newInvalidateAllIntent(ackHandleP));
      } catch (ValidationException exception) {
        // Log and drop invalid call.
        logBadAckHandle("invalidateAll", ackHandle);
      }
    }

    @Override
    public void informRegistrationStatus(
        InvalidationClient client, ObjectId objectId, RegistrationState regState) {
      Intent intent = ListenerUpcalls.newRegistrationStatusIntent(
          ProtoWrapperConverter.convertToObjectIdProto(objectId),
          regState == RegistrationState.REGISTERED);
      issueIntent(context, intent);
    }

    @Override
    public void informRegistrationFailure(InvalidationClient client, ObjectId objectId,
        boolean isTransient, String errorMessage) {
      issueIntent(context, ListenerUpcalls.newRegistrationFailureIntent(
          ProtoWrapperConverter.convertToObjectIdProto(objectId), isTransient, errorMessage));
    }

    @Override
    public void reissueRegistrations(InvalidationClient client, byte[] prefix, int prefixLength) {
      issueIntent(context, ListenerUpcalls.newReissueRegistrationsIntent(prefix, prefixLength));
    }

    @Override
    public void informError(InvalidationClient client, ErrorInfo errorInfo) {
      issueIntent(context, ListenerUpcalls.newErrorIntent(errorInfo));
    }

    /**
     * Sends {@code intent} to the real listener via the listener intent service class.
     */
    static void issueIntent(Context context, Intent intent) {
      intent.setClassName(context, (listenerServiceClassForTest != null) ?
          listenerServiceClassForTest.getName() :
              new AndroidTiclManifest(context).getListenerServiceClass());
      try {
        context.startService(intent);
      } catch (IllegalStateException exception) {
        staticLogger.warning("Unable to deliver intent: %s", exception);
      }
    }

    /**
     * Logs a warning that a listener upcall to {@code method} has been dropped because
     * {@code unparseableHandle} could not be parsed.
     */
    private void logBadAckHandle(String method, AckHandle unparseableHandle) {
      logger.warning("Dropping call to %s; could not parse ack handle data %s",
          method, Arrays.toString(unparseableHandle.getHandleData()));
    }
  }

  /**
   * Unique identifier for this Ticl. This is used to ensure that scheduler intents for other Ticls
   * are not incorrectly delivered to this instance.
   */
  private final long schedulingId;

  /**
   * Creates a fresh instance.
   *
   * @param context Android system context
   * @param resources Ticl resources to use
   * @param random random number generator for the Ticl
   * @param clientType type of the Ticl
   * @param clientName unique application name for the Ticl
   * @param config configuration to use
   */
  AndroidInvalidationClientImpl(Context context, SystemResources resources, Random random,
      int clientType, byte[] clientName, ClientConfigP config) {
    super(resources, random, clientType, clientName, config, getApplicationName(context),
        new IntentForwardingListener(context, resources.getLogger()));
    this.schedulingId = resources.getInternalScheduler().getCurrentTimeMs();
    resources.getLogger().fine("Create new Ticl scheduling id: %s", schedulingId);
    initializeSchedulerWithRecurringTasks();
  }

  /**
   * Creates an instance with state restored from {@code marshalledState}. Other parameters are as
   * in {@link InvalidationClientCore}.
   */
  AndroidInvalidationClientImpl(Context context, SystemResources resources, Random random,
      AndroidTiclState marshalledState) {
    super(resources,
        random,
        marshalledState.getMetadata().getClientType(),
        marshalledState.getMetadata().getClientName().getByteArray(),
        marshalledState.getMetadata().getClientConfig(),
        getApplicationName(context),
        marshalledState.getTiclState(),
        new IntentForwardingListener(context, resources.getLogger()));
    this.schedulingId = marshalledState.getMetadata().getTiclId();
    initializeSchedulerWithRecurringTasks();
  }

  /** Returns the name of the application using the Ticl. */
  private static String getApplicationName(Context context) {
    return context.getPackageName();
  }

  /**
   * Provides the internal scheduler with references to each of the recurring tasks that can be
   * executed.
   */
  private void initializeSchedulerWithRecurringTasks() {
    if (!(getResources().getInternalScheduler() instanceof AndroidInternalScheduler)) {
      throw new IllegalStateException("Scheduler must be an AndroidInternalScheduler, not "
          + getResources().getInternalScheduler());
    }
    AndroidInternalScheduler scheduler =
        (AndroidInternalScheduler) getResources().getInternalScheduler();
    for (Map.Entry<String, Runnable> entry : getRecurringTasks().entrySet()) {
      scheduler.registerTask(entry.getKey(), entry.getValue());
    }
  }

  /** Returns the scheduling id of this Ticl. */
  long getSchedulingId() {
    return schedulingId;
  }

  // This method appears to serve no purpose, since it's just a delegation to the superclass method
  // with the same access level (protected). However, protected also implies package access, so what
  // this is doing is making this method visible to TiclStateManager.
  @Override
  protected ApplicationClientIdP getApplicationClientIdP() {
    return super.getApplicationClientIdP();
  }

  // Similar rationale as getApplicationClientIdP.
  @Override
  protected ClientConfigP getConfig() {
    return super.getConfig();
  }

  // Similar rationale as getApplicationClientIdP.
  @Override
  protected boolean isStarted() {
    return super.isStarted();
  }
}
