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

import com.google.ipc.invalidation.external.client.InvalidationClient;
import com.google.ipc.invalidation.external.client.InvalidationClientConfig;
import com.google.ipc.invalidation.external.client.InvalidationListener;
import com.google.ipc.invalidation.external.client.InvalidationListener.RegistrationState;
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.android.service.AndroidLogger;
import com.google.ipc.invalidation.external.client.types.AckHandle;
import com.google.ipc.invalidation.external.client.types.ErrorInfo;
import com.google.ipc.invalidation.external.client.types.Invalidation;
import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.ipc.invalidation.ticl.InvalidationClientCore;
import com.google.ipc.invalidation.ticl.ProtoWrapperConverter;
import com.google.ipc.invalidation.ticl.android2.AndroidClock;
import com.google.ipc.invalidation.ticl.android2.AndroidInvalidationListenerIntentMapper;
import com.google.ipc.invalidation.ticl.android2.ProtocolIntents;
import com.google.ipc.invalidation.ticl.android2.channel.AndroidChannelConstants.AuthTokenConstants;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.RegistrationCommand;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.StartCommand;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientConfigP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.Preconditions;
import com.google.ipc.invalidation.util.ProtoWrapper.ValidationException;

import android.app.IntentService;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;


/**
 * Simplified listener contract for Android  clients. Takes care of exponential back-off when
 * register or unregister are called for an object after a failure has occurred. Also suppresses
 * redundant register requests.
 *
 * <p>A sample implementation of an {@link AndroidListener} is shown below:
 *
 * <p><code>
 * class ExampleListener extends AndroidListener {
 *   @Override
 *   public void reissueRegistrations(byte[] clientId) {
 *     List<ObjectId> desiredRegistrations = ...;
 *     register(clientId, desiredRegistrations);
 *   }
 *
 *   @Override
 *   public void invalidate(Invalidation invalidation, final byte[] ackHandle) {
 *     // Track the most recent version of the object (application-specific) and then acknowledge
 *     // the invalidation.
 *     ...
 *     acknowledge(ackHandle);
 *   }
 *
 *   @Override
 *   public void informRegistrationFailure(byte[] clientId, ObjectId objectId,
 *       boolean isTransient, String errorMessage) {
 *     // Try again if there is a transient failure and we still care whether the object is
 *     // registered or not.
 *     if (isTransient) {
 *       boolean shouldRetry = ...;
 *       if (shouldRetry) {
 *         boolean shouldBeRegistered = ...;
 *         if (shouldBeRegistered) {
 *           register(clientId, ImmutableList.of(objectId));
 *         } else {
 *           unregister(clientId, ImmutableList.of(objectId));
 *         }
 *       }
 *     }
 *   }
 *
 *   ...
 * }
 * </code>
 *
 * <p>See {@link com.google.ipc.invalidation.examples.android2} for a complete sample.
 *
 */
public abstract class AndroidListener extends IntentService {

  /** External alarm receiver that allows the listener to respond to alarm intents. */
  public static final class AlarmReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
      Preconditions.checkNotNull(context);
      Preconditions.checkNotNull(intent);
      if (intent.hasExtra(AndroidListenerIntents.EXTRA_REGISTRATION)
          || intent.hasExtra(AndroidListenerIntents.EXTRA_SCHEDULED_TASK)) {
        AndroidListenerIntents.issueAndroidListenerIntent(context, intent);
      }
    }
  }

  /** The logger. */
  private static final Logger logger = AndroidLogger.forPrefix("");

  /** Initial retry delay for exponential backoff (1 minute). */
  
  static int initialMaxDelayMs = (int) TimeUnit.SECONDS.toMillis(60);

  /** Maximum delay factor for exponential backoff (6 hours). */
  
  static int maxDelayFactor = 6 * 60;

  /** The last client ID passed to the ready up-call. */
  
  static Bytes lastClientIdForTest;

  /**
   * Invalidation listener implementation. We implement the interface on a private field rather
   * than directly to avoid leaking methods that should not be directly called by the client
   * application. The listener must be called only on intent service thread.
   */
  private final InvalidationListener invalidationListener = new InvalidationListener() {
    @Override
    public final void ready(final InvalidationClient client) {
      Bytes clientId = state.getClientId();
      AndroidListener.lastClientIdForTest = clientId;
      AndroidListener.this.ready(clientId.getByteArray());
    }

    @Override
    public final void reissueRegistrations(final InvalidationClient client, byte[] prefix,
        int prefixLength) {
      AndroidListener.this.reissueRegistrations(state.getClientId().getByteArray());
    }

    @Override
    public final void informRegistrationStatus(final InvalidationClient client,
        final ObjectId objectId, final RegistrationState regState) {
      state.informRegistrationSuccess(objectId);
      AndroidListener.this.informRegistrationStatus(state.getClientId().getByteArray(), objectId,
          regState);
    }

    @Override
    public final void informRegistrationFailure(final InvalidationClient client,
        final ObjectId objectId, final boolean isTransient, final String errorMessage) {
      state.informRegistrationFailure(objectId, isTransient);
      AndroidListener.this.informRegistrationFailure(state.getClientId().getByteArray(), objectId,
          isTransient, errorMessage);
    }

    @Override
    public void invalidate(InvalidationClient client, Invalidation invalidation,
        AckHandle ackHandle) {
      AndroidListener.this.invalidate(invalidation, ackHandle.getHandleData());
    }

    @Override
    public void invalidateUnknownVersion(InvalidationClient client, ObjectId objectId,
        AckHandle ackHandle) {
      AndroidListener.this.invalidateUnknownVersion(objectId, ackHandle.getHandleData());
    }

    @Override
    public void invalidateAll(InvalidationClient client, AckHandle ackHandle) {
      AndroidListener.this.invalidateAll(ackHandle.getHandleData());
    }

    @Override
    public void informError(InvalidationClient client, ErrorInfo errorInfo) {
      AndroidListener.this.informError(errorInfo);
    }
  };

  /**
   * The internal state of the listener. Lazy initialization, triggered by {@link #onHandleIntent}.
   */
  private AndroidListenerState state;

  /** The clock to use when scheduling retry call-backs. */
  private final AndroidClock clock = new AndroidClock.SystemClock();

  /**
   * The mapper used to route intents to the invalidation listener. Lazy initialization triggered
   * by {@link #onCreate}.
   */
  private AndroidInvalidationListenerIntentMapper intentMapper;

  /** Initializes {@link AndroidListener}. */
  protected AndroidListener() {
    super("");

    // If the process dies before an intent is handled, setIntentRedelivery(true) ensures that the
    // last intent is redelivered. This optimization is not necessary for correctness: on restart,
    // all registrations will be reissued and unacked invalidations will be resent anyways.
    setIntentRedelivery(true);
  }

  /** See specs for {@link InvalidationClient#start}. */
  public static Intent createStartIntent(Context context, InvalidationClientConfig config) {
    Preconditions.checkNotNull(context);
    Preconditions.checkNotNull(config);
    Preconditions.checkNotNull(config.clientName);

    return AndroidListenerIntents.createStartIntent(context, config.clientType,
        Bytes.fromByteArray(config.clientName), config.allowSuppression);
  }

  /** See specs for {@link InvalidationClient#start}. */
  public static Intent createStartIntent(Context context, int clientType, byte[] clientName) {
    Preconditions.checkNotNull(context);
    Preconditions.checkNotNull(clientName);

    final boolean allowSuppression = true;
    return AndroidListenerIntents.createStartIntent(context, clientType,
        Bytes.fromByteArray(clientName), allowSuppression);
  }

  /** See specs for {@link InvalidationClient#stop}. */
  public static Intent createStopIntent(Context context) {
    Preconditions.checkNotNull(context);

    return AndroidListenerIntents.createStopIntent(context);
  }

  /**
   * See specs for {@link InvalidationClient#register}.
   *
   * @param context the context
   * @param clientId identifier for the client service for which we are registering
   * @param objectIds the object ids being registered
   */
  public static Intent createRegisterIntent(Context context, byte[] clientId,
      Iterable<ObjectId> objectIds) {
    Preconditions.checkNotNull(context);
    Preconditions.checkNotNull(clientId);
    Preconditions.checkNotNull(objectIds);

    final boolean isRegister = true;
    return AndroidListenerIntents.createRegistrationIntent(context, Bytes.fromByteArray(clientId),
        objectIds, isRegister);
  }

  /**
   * See specs for {@link InvalidationClient#register}.
   *
   * @param clientId identifier for the client service for which we are registering
   * @param objectIds the object ids being registered
   */
  public void register(byte[] clientId, Iterable<ObjectId> objectIds) {
    Preconditions.checkNotNull(clientId);
    Preconditions.checkNotNull(objectIds);

    Context context = getApplicationContext();
    try {
      context.startService(createRegisterIntent(context, clientId, objectIds));
    } catch (IllegalStateException exception) {
      logger.info("Unable to deliver `register` intent: %s", exception);
    }
  }

  /**
   * See specs for {@link InvalidationClient#unregister}.
   *
   * @param context the context
   * @param clientId identifier for the client service for which we are unregistering
   * @param objectIds the object ids being unregistered
   */
  public static Intent createUnregisterIntent(Context context, byte[] clientId,
      Iterable<ObjectId> objectIds) {
    Preconditions.checkNotNull(context);
    Preconditions.checkNotNull(clientId);
    Preconditions.checkNotNull(objectIds);

    final boolean isRegister = false;
    return AndroidListenerIntents.createRegistrationIntent(context, Bytes.fromByteArray(clientId),
        objectIds, isRegister);
  }

  /**
   * Sets the authorization token and type used by the invalidation client. Call in response to
   * {@link #requestAuthToken} calls.
   *
   * @param pendingIntent pending intent passed to {@link #requestAuthToken}
   * @param authToken authorization token
   * @param authType authorization token typo
   */
  public static void setAuthToken(Context context, PendingIntent pendingIntent, String authToken,
      String authType) {
    Preconditions.checkNotNull(pendingIntent);
    Preconditions.checkNotNull(authToken);
    Preconditions.checkNotNull(authType);

    AndroidListenerIntents.issueAuthTokenResponse(context, pendingIntent, authToken, authType);
  }

  /**
   * See specs for {@link InvalidationClient#unregister}.
   *
   * @param clientId identifier for the client service for which we are registering
   * @param objectIds the object ids being unregistered
   */
  public void unregister(byte[] clientId, Iterable<ObjectId> objectIds) {
    Preconditions.checkNotNull(clientId);
    Preconditions.checkNotNull(objectIds);

    Context context = getApplicationContext();
    try {
      context.startService(createUnregisterIntent(context, clientId, objectIds));
    } catch (IllegalStateException exception) {
      logger.info("Unable to deliver `unregister` intent: %s", exception);
    }
  }

  /** See specs for {@link InvalidationClient#acknowledge}. */
  public static Intent createAcknowledgeIntent(Context context, byte[] ackHandle) {
    Preconditions.checkNotNull(context);
    Preconditions.checkNotNull(ackHandle);

    return AndroidListenerIntents.createAckIntent(context, ackHandle);
  }

  /** See specs for {@link InvalidationClient#acknowledge}. */
  public void acknowledge(byte[] ackHandle) {
    Preconditions.checkNotNull(ackHandle);

    Context context = getApplicationContext();
    try {
      context.startService(createAcknowledgeIntent(context, ackHandle));
    } catch (IllegalStateException exception) {
      logger.info("Unable to deliver `acknowledge` intent: %s", exception);
    }
  }

  /**
   * See specs for {@link InvalidationListener#ready}.
   *
   * @param clientId the client identifier that must be passed to {@link #createRegisterIntent}
   *     and {@link #createUnregisterIntent}
   */
  public abstract void ready(byte[] clientId);

  /**
   * See specs for {@link InvalidationListener#reissueRegistrations}.
   *
   * @param clientId the client identifier that must be passed to {@link #createRegisterIntent}
   *     and {@link #createUnregisterIntent}
   */
  public abstract void reissueRegistrations(byte[] clientId);

  /**
   * See specs for {@link InvalidationListener#informError}.
   */
  public abstract void informError(ErrorInfo errorInfo);

  /**
   * See specs for {@link InvalidationListener#invalidate}.
   *
   * @param invalidation the invalidation
   * @param ackHandle event acknowledgment handle
   */
  public abstract void invalidate(Invalidation invalidation, byte[] ackHandle);

  /**
   * See specs for {@link InvalidationListener#invalidateUnknownVersion}.
   *
   * @param objectId identifier for the object with unknown version
   * @param ackHandle event acknowledgment handle
   */
  public abstract void invalidateUnknownVersion(ObjectId objectId, byte[] ackHandle);

  /**
   * See specs for {@link InvalidationListener#invalidateAll}.
   *
   * @param ackHandle event acknowledgment handle
   */
  public abstract void invalidateAll(byte[] ackHandle);

  /**
   * Read listener state.
   *
   * @return serialized state or {@code null} if it is not available
   */
  public abstract byte[] readState();

  /** Write listener state to some location. */
  public abstract void writeState(byte[] data);

  /**
   * See specs for {@link InvalidationListener#informRegistrationFailure}.
   */
  public abstract void informRegistrationFailure(byte[] clientId, ObjectId objectId,
      boolean isTransient, String errorMessage);

  /**
   * See specs for (@link InvalidationListener#informRegistrationStatus}.
   */
  public abstract void informRegistrationStatus(byte[] clientId, ObjectId objectId,
      RegistrationState regState);

  /**
   * Called when an authorization token is needed. Respond by calling {@link #setAuthToken}.
   *
   * @param pendingIntent pending intent that must be used in {@link #setAuthToken} response.
   * @param invalidAuthToken the existing invalid token or null if none exists. Implementation
   *     should invalidate the token.
   */
  public abstract void requestAuthToken(PendingIntent pendingIntent,
      String invalidAuthToken);

  /**
   * Handles invalidations received while the client is stopped. An implementation may choose to
   * do work in response to these invalidations (delivered best-effort by the invalidation system).
   * Not intended for use by most client implementations.
   */
  protected void backgroundInvalidateForInternalUse(
      @SuppressWarnings("unused") Iterable<Invalidation> invalidations) {
    // Ignore background invalidations by default.
  }

  @Override
  public void onCreate() {
    super.onCreate();

    // Initialize the intent mapper (now that context is available).
    intentMapper = new AndroidInvalidationListenerIntentMapper(invalidationListener, this);
  }

  /**
   * Derived classes may override this method to handle custom intents. This is a recommended
   * pattern for invalidation-related intents, e.g. for registration and unregistration. Derived
   * classes should call {@code super.onHandleIntent(intent)} for any intents they did not
   * handle on their own.
   */
  @Override
  protected void onHandleIntent(Intent intent) {
    if (intent == null) {
      return;
    }

    // We lazily initialize state in calls to onHandleIntent rather than initializing in onCreate
    // because onCreate runs on the UI thread and initializeState performs I/O.
    if (state == null) {
      initializeState();
    }

    // Flush any scheduled registration retries. We do this whenever the intent service handles an
    // intent since we may not be able to reliably schedule alarms due to limits in the
    // AlarmManager.
    for (RegistrationCommand scheduledCommand : state.takeRegistrationRetriesUpTo(clock.nowMs())) {
      handleRegistrationCommand(scheduledCommand);
    }

    // Handle any intents specific to the AndroidListener. If an intent is not recognized, defer to
    // the intentMapper, which handles listener upcalls corresponding to the InvalidationListener
    // methods.
    if (!AndroidListenerIntents.isScheduledTaskIntent(intent)
        && !tryHandleAuthTokenRequestIntent(intent)
        && !tryHandleRegistrationIntent(intent)
        && !tryHandleStartIntent(intent)
        && !tryHandleStopIntent(intent)
        && !tryHandleAckIntent(intent)
        && !tryHandleBackgroundInvalidationsIntent(intent)) {
      intentMapper.handleIntent(intent);
    }

    // If there are any registration retries pending, schedule an alarm.
    Long executeMs = state.getNextExecuteMs();
    if (executeMs != null) {
      logger.fine("scheduling alarm at %s", executeMs);
      AndroidListenerIntents.issueScheduledTaskIntent(
          getApplicationContext(), executeMs.longValue());
    }

    // Always check to see if we need to persist state changes after handling an intent.
    if (state.getIsDirty()) {
      writeState(state.marshal().toByteArray());
      state.resetIsDirty();
    }
  }

  /** Returns invalidation client that can be used to trigger intents against the TICL service. */
  private InvalidationClient getClient() {
    return intentMapper.client;
  }

  /**
   * Initializes listener state either from persistent proto (if available) or from scratch.
   */
  private void initializeState() {
    AndroidListenerProtocol.AndroidListenerState proto = getPersistentState();
    if (proto != null) {
      state = new AndroidListenerState(initialMaxDelayMs, maxDelayFactor, proto);
    } else {
      state = new AndroidListenerState(initialMaxDelayMs, maxDelayFactor);
    }
  }

  /**
   * Reads and parses persistent state for the listener. Returns {@code null} if the state does not
   * exist or is invalid.
   */
  private AndroidListenerProtocol.AndroidListenerState getPersistentState() {
    // Defer to application code to read the blob containing the state proto.
    byte[] stateData = readState();
    try {
      if (null != stateData) {
        AndroidListenerProtocol.AndroidListenerState state =
            AndroidListenerProtocol.AndroidListenerState.parseFrom(stateData);
        if (!AndroidListenerProtos.isValidAndroidListenerState(state)) {
          logger.warning("Invalid listener state.");
          return null;
        }
        return state;
      }
    } catch (ValidationException exception) {
      logger.warning("Failed to parse listener state: %s", exception);
    }
    return null;
  }

  /**
   * Tries to handle a request for an authorization token. Returns {@code true} iff the intent is
   * an auth token request.
   */
  private boolean tryHandleAuthTokenRequestIntent(Intent intent) {
    if (!AndroidListenerIntents.isAuthTokenRequest(intent)) {
      return false;
    }

    // Check for invalid auth token. Subclass may have to invalidate it if it exists in the call
    // to getNewAuthToken.
    String invalidAuthToken = intent.getStringExtra(
        AuthTokenConstants.EXTRA_INVALIDATE_AUTH_TOKEN);
    // Intent also includes a pending intent that we can use to pass back our response.
    PendingIntent pendingIntent = intent.getParcelableExtra(
        AuthTokenConstants.EXTRA_PENDING_INTENT);
    if (pendingIntent == null) {
      logger.warning("Authorization request without pending intent extra.");
    } else {
      // Delegate to client application to figure out what the new token should be and the auth
      // type.
      requestAuthToken(pendingIntent, invalidAuthToken);
    }
    return true;
  }

  /** Tries to handle a stop intent. Returns {@code true} iff the intent is a stop intent. */
  private boolean tryHandleStopIntent(Intent intent) {
    if (!AndroidListenerIntents.isStopIntent(intent)) {
      return false;
    }
    getClient().stop();
    return true;
  }

  /**
   * Tries to handle a registration intent. Returns {@code true} iff the intent is a registration
   * intent.
   */
  private boolean tryHandleRegistrationIntent(Intent intent) {
    RegistrationCommand command = AndroidListenerIntents.findRegistrationCommand(intent);
    if ((command == null) || !AndroidListenerProtos.isValidRegistrationCommand(command)) {
      return false;
    }
    handleRegistrationCommand(command);
    return true;
  }

  /** Handles a registration command for this client. */
  private void handleRegistrationCommand(RegistrationCommand command) {
    // Make sure the registration is intended for this client. If not, we ignore it (suggests
    // there is a new client now).
    if (!command.getClientId().equals(state.getClientId())) {
      logger.warning("Ignoring registration request for old client. Old ID = %s, New ID = %s",
          command.getClientId(), state.getClientId());
      return;
    }
    boolean isRegister = command.getIsRegister();
    for (ObjectIdP objectIdP : command.getObjectId()) {
      ObjectId objectId = ProtoWrapperConverter.convertFromObjectIdProto(objectIdP);
      // We may need to delay the registration command (if it is not already delayed).
      int delayMs = 0;
      if (!command.getIsDelayed()) {
        delayMs = state.getNextDelay(objectId);
      }
      if (delayMs == 0) {
        issueRegistration(objectId, isRegister);
      } else {
        // Add a scheduled registration retry to listener state. An alarm will be triggered at the
        // end of the onHandleIntent method if needed.
        long executeMs = clock.nowMs() + delayMs;
        state.addScheduledRegistrationRetry(objectId, isRegister, executeMs);
      }
    }
  }

  /**
   * Called when the client application requests a new registration. If a redundant register request
   * is made -- i.e. when the application attempts to register an object that is already in the
   * {@code AndroidListenerState#desiredRegistrations} collection -- the method returns immediately.
   * Unregister requests are never ignored since we can't reliably determine whether an unregister
   * request is redundant: our policy on failures of any kind is to remove the registration from
   * the {@code AndroidListenerState#desiredRegistrations} collection.
   */
  private void issueRegistration(ObjectId objectId, boolean isRegister) {
    if (isRegister) {
      if (state.addDesiredRegistration(objectId)) {
        // Don't bother if we think it's already registered. Note that we remove the object from the
        // collection when there is a failure.
        getClient().register(objectId);
      }
    } else {
      // Remove the object ID from the desired registration collection so that subsequent attempts
      // to re-register are not ignored.
      state.removeDesiredRegistration(objectId);
      getClient().unregister(objectId);
    }
  }

  /** Tries to handle a start intent. Returns {@code true} iff the intent is a start intent. */
  private boolean tryHandleStartIntent(Intent intent) {
    StartCommand command = AndroidListenerIntents.findStartCommand(intent);
    if ((command == null) || !AndroidListenerProtos.isValidStartCommand(command)) {
      return false;
    }
    // Reset the state so that we make no assumptions about desired registrations and can ignore
    // messages directed at the wrong instance.
    state = new AndroidListenerState(initialMaxDelayMs, maxDelayFactor);
    boolean skipStartForTest = false;
    ClientConfigP clientConfig = InvalidationClientCore.createConfig();
    if (command.getAllowSuppression() != clientConfig.getAllowSuppression()) {
      ClientConfigP.Builder clientConfigBuilder = clientConfig.toBuilder();
      clientConfigBuilder.allowSuppression = command.getAllowSuppression();
      clientConfig = clientConfigBuilder.build();
    }
    Intent startIntent = ProtocolIntents.InternalDowncalls.newCreateClientIntent(
        command.getClientType(), command.getClientName(), clientConfig, skipStartForTest);
    AndroidListenerIntents.issueTiclIntent(getApplicationContext(), startIntent);
    return true;
  }

  /** Tries to handle an ack intent. Returns {@code true} iff the intent is an ack intent. */
  private boolean tryHandleAckIntent(Intent intent) {
    byte[] data = AndroidListenerIntents.findAckHandle(intent);
    if (data == null) {
      return false;
    }
    getClient().acknowledge(AckHandle.newInstance(data));
    return true;
  }

  /**
   * Tries to handle a background invalidation intent. Returns {@code true} iff the intent is a
   * background invalidation intent.
   */
  private boolean tryHandleBackgroundInvalidationsIntent(Intent intent) {
    byte[] data = intent.getByteArrayExtra(ProtocolIntents.BACKGROUND_INVALIDATION_KEY);
    if (data == null) {
      return false;
    }
    try {
      InvalidationMessage invalidationMessage = InvalidationMessage.parseFrom(data);
      List<Invalidation> invalidations = new ArrayList<Invalidation>();
      for (InvalidationP invalidation : invalidationMessage.getInvalidation()) {
        invalidations.add(ProtoWrapperConverter.convertFromInvalidationProto(invalidation));
      }
      backgroundInvalidateForInternalUse(invalidations);
    } catch (ValidationException exception) {
      logger.info("Failed to parse background invalidation intent payload: %s",
          exception.getMessage());
    }
    return false;
  }

  /** Returns the current state of the listener, for tests. */
  AndroidListenerState getStateForTest() {
    return state;
  }
}
