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

import com.google.ipc.invalidation.external.client.InvalidationListener;
import com.google.ipc.invalidation.external.client.android.service.AndroidLogger;

import android.app.IntentService;
import android.content.Intent;


/**
 * Class implementing the {@link InvalidationListener} in the application using the  client.
 * This class is configured with the name of the application class implementing the
 * {@link InvalidationListener} for the application. It receives upcalls from the Ticl as
 * {@link Intent}s and dispatches them against dynamically created instances of the provided
 * class. In this way, it serves as a bridge between the intent protocol and the application.
 */
public class AndroidInvalidationListenerStub extends IntentService {
  /* This class needs to be public so that the Android runtime can start it as a service. */

  private final AndroidLogger logger = AndroidLogger.forPrefix("");

  /** The mapper used to route intents to the invalidation listener. */
  private AndroidInvalidationListenerIntentMapper intentMapper;

  public AndroidInvalidationListenerStub() {
    super("");
    setIntentRedelivery(true);
  }

  @Override
  public void onCreate() {
    super.onCreate();
    InvalidationListener listener = createListener(getListenerClass());
    intentMapper = new AndroidInvalidationListenerIntentMapper(listener, getApplicationContext());
  }

  @SuppressWarnings("unchecked")
  private Class<? extends InvalidationListener> getListenerClass() {
    try {
      // Find the listener class that the application wants to use to receive upcalls.
      return (Class<? extends InvalidationListener>)
          Class.forName(new AndroidTiclManifest(this).getListenerClass());
    } catch (ClassNotFoundException exception) {
      throw new RuntimeException("Invalid listener class", exception);
    }
  }

  /**
   * Handles a listener upcall by decoding the protocol buffer in {@code intent} and dispatching
   * to the appropriate method on an instance of {@link InvalidationListener}.
   */
  @Override
  public void onHandleIntent(Intent intent) {
    logger.fine("onHandleIntent({0})", intent);
    intentMapper.handleIntent(intent);
  }

  private InvalidationListener createListener(Class<? extends InvalidationListener> listenerClass) {
    // Create an instance of the application listener class to handle the upcall.
    try {
      return listenerClass.newInstance();
    } catch (InstantiationException exception) {
      throw new RuntimeException("Could not create listener", exception);
    } catch (IllegalAccessException exception) {
      throw new RuntimeException("Could not create listener", exception);
    }
  }
}
