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
package com.google.ipc.invalidation.examples.android2;

import com.google.ipc.invalidation.examples.android2.nano.ExampleListenerProto.ExampleListenerStateProto.ObjectIdProto;
import com.google.ipc.invalidation.external.client.InvalidationClientConfig;
import com.google.ipc.invalidation.external.client.InvalidationListener.RegistrationState;
import com.google.ipc.invalidation.external.client.contrib.AndroidListener;
import com.google.ipc.invalidation.external.client.types.ErrorInfo;
import com.google.ipc.invalidation.external.client.types.Invalidation;
import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.protobuf.nano.InvalidProtocolBufferNanoException;
import com.google.protobuf.nano.MessageNano;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.util.Base64;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;


/**
 * Implements the service that handles invalidation client events for this application. It maintains
 * state for all objects tracked by the listener (see {@link ExampleListenerState}). By default, the
 * listener registers an interest in a small number of objects when started, but it responds to
 * registration intents from the main activity (see {@link #createRegisterIntent} and
 * {@link #createUnregisterIntent}) so that registrations can be dynamically managed.
 * <p>
 * Many errors cases in this example implementation are handled by logging errors, which is not the
 * appropriate response in a real application where retries or user notification may be needed.
 *
 */
public final class ExampleListener extends AndroidListener {

  /** The account type value for Google accounts */
  private static final String GOOGLE_ACCOUNT_TYPE = "com.google";

  /**
   * This is the authentication token type that's used for invalidation client communication to the
   * server. For real applications, it would generally match the authorization type used by the
   * application.
   */
  private static final String AUTH_TYPE = "android";

  /** Name used for shared preferences. */
  private static final String PREFERENCES_NAME = "example_listener";

  /** Key used for {@link AndroidListener} state in shared preferences. */
  private static final String ANDROID_LISTENER_STATE_KEY = "android_listener_state";

  /** Key used for {@link ExampleListener} state in shared preferences. */
  private static final String EXAMPLE_LISTENER_STATE_KEY = "example_listener_state";

  /** The tag used for logging in the listener. */
  private static final String TAG = "TEA2:EL";

  /** Ticl client configuration. */
  private static final int CLIENT_TYPE = 4; // Demo client ID.
  private static final byte[] CLIENT_NAME = "TEA2:eetrofoot".getBytes();

  // Intent constants.
  private static final String START_INTENT_ACTION = TAG + ":START";
  private static final String STOP_INTENT_ACTION = TAG + ":STOP";
  private static final String REGISTER_INTENT_ACTION = TAG + ":REGISTER";
  private static final String UNREGISTER_INTENT_ACTION = TAG + ":UNREGISTER";
  private static final String OBJECT_ID_EXTRA = "oid";

  /** Persistent state for the example listener. */
  private ExampleListenerState exampleListenerState;

  public ExampleListener() {
    super();
  }

  @Override
  public void onCreate() {
    super.onCreate();

    // Deserialize persistent state.
    String data = getSharedPreferences().getString(EXAMPLE_LISTENER_STATE_KEY, null);
    exampleListenerState = ExampleListenerState.deserialize(data);
  }

  @Override
  public void onHandleIntent(Intent intent) {
    if (intent == null) {
      return;
    }

    boolean handled = tryHandleRegistrationIntent(intent);
    handled = handled || tryHandleStartIntent(intent);
    handled = handled || tryHandleStopIntent(intent);
    if (!handled) {
      super.onHandleIntent(intent);
    }
  }

  @Override
  public void informError(ErrorInfo errorInfo) {
    Log.e(TAG, "informError: " + errorInfo);

    /***********************************************************************************************
     * YOUR CODE HERE
     *
     * Handling of permanent failures is application-specific.
     **********************************************************************************************/
  }

  @Override
  public void ready(byte[] clientId) {
    Log.i(TAG, "ready()");
    exampleListenerState.setClientId(clientId);
    writeExampleListenerState();
  }

  @Override
  public void reissueRegistrations(byte[] clientId) {
    Log.i(TAG, "reissueRegistrations()");
    register(clientId, exampleListenerState.getInterestingObjects());
  }

  @Override
  public void invalidate(Invalidation invalidation, byte[] ackHandle) {
    Log.i(TAG, "invalidate: " + invalidation);

    exampleListenerState.informInvalidation(invalidation.getObjectId(), invalidation.getVersion(),
        invalidation.getPayload(), /* isBackground */ false);
    writeExampleListenerState();

    // Do real work here based upon the invalidation

    acknowledge(ackHandle);
  }

  @Override
  public void invalidateUnknownVersion(ObjectId objectId, byte[] ackHandle) {
    Log.i(TAG, "invalidateUnknownVersion: " + objectId);

    exampleListenerState.informUnknownVersionInvalidation(objectId);
    writeExampleListenerState();

    // In a real app, the application backend would need to be consulted for object state.

    acknowledge(ackHandle);
  }

  @Override
  public void invalidateAll(byte[] ackHandle) {
    Log.i(TAG, "invalidateAll");

    // Do real work here based upon the invalidation.
    exampleListenerState.informInvalidateAll();
    writeExampleListenerState();

    acknowledge(ackHandle);
  }


  @Override
  public byte[] readState() {
    Log.i(TAG, "readState");
    SharedPreferences sharedPreferences = getSharedPreferences();
    String data = sharedPreferences.getString(ANDROID_LISTENER_STATE_KEY, null);
    return (data != null) ? Base64.decode(data, Base64.DEFAULT) : null;
  }

  @Override
  public void writeState(byte[] data) {
    Log.i(TAG, "writeState");
    Editor editor = getSharedPreferences().edit();
    editor.putString(ANDROID_LISTENER_STATE_KEY, Base64.encodeToString(data, Base64.DEFAULT));
    if (!editor.commit()) {
      Log.e(TAG, "failed to write state");  // In a real app, this case would need to handled.
    }
  }

  @Override
  public void requestAuthToken(PendingIntent pendingIntent,
      String invalidAuthToken) {
    Log.i(TAG, "requestAuthToken");

    // In response to requestAuthToken, we need to get an auth token and inform the invalidation
    // client of the result through a call to setAuthToken. In this example, we block until a
    // result is available. It is also possible to invoke setAuthToken in a callback or when
    // handling an intent.
    AccountManager accountManager = AccountManager.get(getApplicationContext());

    // Invalidate the old token if necessary.
    if (invalidAuthToken != null) {
      accountManager.invalidateAuthToken(GOOGLE_ACCOUNT_TYPE, invalidAuthToken);
    }

    // Choose an (arbitrary in this example) account for which to retrieve an authentication token.
    Account account = getAccount(accountManager);

    try {
      // There are three possible outcomes of the call to getAuthToken:
      //
      // 1. Authentication failure (null result).
      // 2. The user needs to sign in or give permission for the account. In such cases, the result
      //    includes an intent that can be started to retrieve credentials from the user.
      // 3. The response includes the auth token, in which case we can inform the invalidation
      //    client.
      //
      // In the first case, we simply log and return. The response to such errors is application-
      // specific. For instance, the application may prompt the user to choose another account.
      //
      // In the second case, we start an intent to ask for user credentials so that they are
      // available to the application if there is a future request. An application should listen for
      // the LOGIN_ACCOUNTS_CHANGED_ACTION broadcast intent to trigger a response to the
      // invalidation client after the user has responded. Otherwise, it may take several minutes
      // for the invalidation client to start.
      //
      // In the third case, success!, we pass the authorization token and type to the invalidation
      // client using the setAuthToken method.
      AccountManagerFuture<Bundle> future = accountManager.getAuthToken(account, AUTH_TYPE,
          new Bundle(), false, null, null);
      Bundle result = future.getResult();
      if (result == null) {
        // If the result is null, it means that authentication was not possible.
        Log.w(TAG, "Auth token - getAuthToken returned null");
        return;
      }
      if (result.containsKey(AccountManager.KEY_INTENT)) {
        Log.i(TAG, "Starting intent to get auth credentials");

        // Need to start intent to get credentials.
        Intent intent = result.getParcelable(AccountManager.KEY_INTENT);
        int flags = intent.getFlags();
        flags |= Intent.FLAG_ACTIVITY_NEW_TASK;
        intent.setFlags(flags);
        getApplicationContext().startActivity(intent);
        return;
      }

      Log.i(TAG, "Passing auth token to invalidation client");
      String authToken = result.getString(AccountManager.KEY_AUTHTOKEN);
      setAuthToken(getApplicationContext(), pendingIntent, authToken, AUTH_TYPE);
    } catch (OperationCanceledException e) {
      Log.w(TAG, "Auth token - operation cancelled", e);
    } catch (AuthenticatorException e) {
      Log.w(TAG, "Auth token - authenticator exception", e);
    } catch (IOException e) {
      Log.w(TAG, "Auth token - IO exception", e);
    }
  }

  /** Returns any Google account enabled on the device. */
  private static Account getAccount(AccountManager accountManager) {
    if (accountManager == null) {
      throw new NullPointerException();
    }
    for (Account acct : accountManager.getAccounts()) {
      if (GOOGLE_ACCOUNT_TYPE.equals(acct.type)) {
        return acct;
      }
    }
    throw new RuntimeException("No google account enabled.");
  }

  @Override
  public void informRegistrationFailure(byte[] clientId, ObjectId objectId, boolean isTransient,
      String errorMessage) {
    Log.e(TAG, "Registration failure!");
    if (isTransient) {
      // Retry immediately on transient failures. The base AndroidListener will handle exponential
      // backoff if there are repeated failures.
      List<ObjectId> objectIds = new ArrayList<ObjectId>();
      objectIds.add(objectId);
      if (exampleListenerState.isInterestedInObject(objectId)) {
        Log.i(TAG, "Retrying registration of " + objectId);
        register(clientId, objectIds);
      } else {
        Log.i(TAG, "Retrying unregistration of " + objectId);
        unregister(clientId, objectIds);
      }
    }
  }

  @Override
  public void informRegistrationStatus(byte[] clientId, ObjectId objectId,
      RegistrationState regState) {
    Log.i(TAG, "informRegistrationStatus");

    List<ObjectId> objectIds = new ArrayList<ObjectId>();
    objectIds.add(objectId);
    if (regState == RegistrationState.REGISTERED) {
      if (!exampleListenerState.isInterestedInObject(objectId)) {
        Log.i(TAG, "Unregistering for object we're no longer interested in");
        unregister(clientId, objectIds);
        writeExampleListenerState();
      }
    } else {
      if (exampleListenerState.isInterestedInObject(objectId)) {
        Log.i(TAG, "Registering for an object");
        register(clientId, objectIds);
        writeExampleListenerState();
      }
    }
  }

  @Override
  protected void backgroundInvalidateForInternalUse(Iterable<Invalidation> invalidations) {
    for (Invalidation invalidation : invalidations) {
      Log.i(TAG, "background invalidate: " + invalidation);
      exampleListenerState.informInvalidation(invalidation.getObjectId(), invalidation.getVersion(),
          invalidation.getPayload(), /* isBackground */ true);
      writeExampleListenerState();
    }
  }

  /** Creates an intent that registers an interest in object invalidations for {@code objectId}. */
  public static Intent createRegisterIntent(Context context, ObjectId objectId) {
    return createRegistrationIntent(context, objectId, /* isRegister */ true);
  }

  /** Creates an intent that unregisters for invalidations for {@code objectId}. */
  public static Intent createUnregisterIntent(Context context, ObjectId objectId) {
    return createRegistrationIntent(context, objectId, /* isRegister */ false);
  }

  private static Intent createRegistrationIntent(Context context, ObjectId objectId,
      boolean isRegister) {
    Intent intent = new Intent();
    intent.setAction(isRegister ? REGISTER_INTENT_ACTION : UNREGISTER_INTENT_ACTION);
    intent.putExtra(OBJECT_ID_EXTRA, serializeObjectId(objectId));
    intent.setClass(context, ExampleListener.class);
    return intent;
  }

  /** Creates an intent that starts the invalidation client. */
  public static Intent createStartIntent(Context context) {
    Intent intent = new Intent();
    intent.setAction(START_INTENT_ACTION);
    intent.setClass(context, ExampleListener.class);
    return intent;
  }

  /** Creates an intent that stops the invalidation client. */
  public static Intent createStopIntent(Context context) {
    Intent intent = new Intent();
    intent.setAction(STOP_INTENT_ACTION);
    intent.setClass(context, ExampleListener.class);
    return intent;
  }

  private boolean tryHandleRegistrationIntent(Intent intent) {
    final boolean isRegister;
    if (REGISTER_INTENT_ACTION.equals(intent.getAction())) {
      isRegister = true;
    } else if (UNREGISTER_INTENT_ACTION.equals(intent.getAction())) {
      isRegister = false;
    } else {
      // Not a registration intent.
      return false;
    }

    // Try to parse object id extra.
    ObjectId objectId = parseObjectIdExtra(intent);
    if (objectId == null) {
      Log.e(TAG, "Registration intent without valid object id extra");
      return false;
    }

    // Update example listener state.
    if (isRegister) {
      exampleListenerState.addObjectOfInterest(objectId);
    } else {
      exampleListenerState.removeObjectOfInterest(objectId);
    }
    writeExampleListenerState();

    // If the client is ready, perform registration now.
    byte[] clientId = exampleListenerState.getClientId();
    if (clientId == null) {
      Log.i(TAG, "Deferring registration until client is ready");
    } else {
      // Perform registration immediately if we have been assigned a client id.
      List<ObjectId> objectIds = new ArrayList<ObjectId>(1);
      objectIds.add(objectId);
      if (isRegister) {
        register(clientId, objectIds);
      } else {
        unregister(clientId, objectIds);
      }
    }
    return true;
  }

  private boolean tryHandleStartIntent(Intent intent) {
    if (START_INTENT_ACTION.equals(intent.getAction())) {
      // Clear the client id since a new one will be provided after the client has started.
      exampleListenerState.setClientId(null);
      writeExampleListenerState();

      // Setting this to true allows us to see invalidations that may suppress older invalidations.
      // When this flag is 'false', AndroidListener#invalidateUnknownVersion is called instead of
      // AndroidListener#invalidate when suppression has potentially occurred.
      final boolean allowSuppression = true;
      InvalidationClientConfig config = new InvalidationClientConfig(CLIENT_TYPE, CLIENT_NAME,
          "ExampleListener", allowSuppression);
      startService(AndroidListener.createStartIntent(this, config));
      return true;
    }
    return false;
  }

  private boolean tryHandleStopIntent(Intent intent) {
    if (STOP_INTENT_ACTION.equals(intent.getAction())) {
      // Clear the client id since the client is no longer available.
      exampleListenerState.setClientId(null);
      writeExampleListenerState();
      startService(AndroidListener.createStopIntent(this));
      return true;
    }
    return false;
  }

  private void writeExampleListenerState() {
    Editor editor = getSharedPreferences().edit();
    editor.putString(EXAMPLE_LISTENER_STATE_KEY, exampleListenerState.serialize());
    if (!editor.commit()) {
      // In a real app, this case would need to handled.
      Log.e(TAG, "failed to write example listener state");
    }
    MainActivity.State.setInfo(exampleListenerState.toString());
  }

  private static byte[] serializeObjectId(ObjectId objectId) {
    return MessageNano.toByteArray(ExampleListenerState.serializeObjectId(objectId));
  }

  private static ObjectId parseObjectIdExtra(Intent intent) {
    byte[] bytes = intent.getByteArrayExtra(OBJECT_ID_EXTRA);
    if (bytes == null) {
      return null;
    }
    try {
      ObjectIdProto proto = MessageNano.mergeFrom(new ObjectIdProto(), bytes);
      return ExampleListenerState.deserializeObjectId(proto);
    } catch (InvalidProtocolBufferNanoException exception) {
      Log.e(TAG, String.format(Locale.ROOT, "Error parsing object id. error='%s'",
          exception.getMessage()));
      return null;
    }
  }

  private SharedPreferences getSharedPreferences() {
    return getApplicationContext().getSharedPreferences(PREFERENCES_NAME, MODE_PRIVATE);
  }
}
