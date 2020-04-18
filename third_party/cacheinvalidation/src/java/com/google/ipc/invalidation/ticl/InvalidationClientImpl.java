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
package com.google.ipc.invalidation.ticl;

import static com.google.ipc.invalidation.external.client.SystemResources.Scheduler.NO_DELAY;

import com.google.ipc.invalidation.external.client.InvalidationListener;
import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.external.client.types.AckHandle;
import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientConfigP;

import java.util.Collection;
import java.util.Random;

/**
 * Implementation of the standard  Java Ticl. This class extends {@link InvalidationClientCore}
 * with additional thread-safety related constructs. Specifically, it ensures that:
 * <p>
 * 1. All application calls into the Ticl execute on the internal scheduler.
 * <p>
 * 2. The storage layer always executes callbacks on the internal scheduler thread.
 * <p>
 * 3. All calls into the listener are made on the listener scheduler thread.
 */
public class InvalidationClientImpl extends InvalidationClientCore {
  public InvalidationClientImpl(final SystemResources resources, Random random, int clientType,
      final byte[] clientName, ClientConfigP config, String applicationName,
      InvalidationListener listener) {
    super(
        // We will make Storage a SafeStorage after the constructor call. It's not possible to
        // construct a new resources around the existing components and pass that to super(...)
        // because then subsequent calls on the first resources object (e.g., start) would not
        // affect the new resources object that the Ticl would be using.
        resources,

        // Pass basic parameters through unmodified.
        random, clientType, clientName, config, applicationName,

        // Wrap the listener in a CheckingInvalidationListener to enforce appropriate threading.
        new CheckingInvalidationListener(listener,
            resources.getInternalScheduler(), resources.getListenerScheduler(),
            resources.getLogger())
    ); // End super.

    // Make Storage safe.
    this.storage = new SafeStorage(resources.getStorage());
    this.storage.setSystemResources(resources);

    // CheckingInvalidationListener needs the statistics object created by our super() call, so
    // we can't provide it at construction-time (since it hasn't been created yet).
    ((CheckingInvalidationListener) this.listener).setStatistics(statistics);

  }

  // Methods below are public methods from InvalidationClient that must first enqueue onto the
  // internal thread.

  @Override
  public void start() {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.start();
      }
    });
  }

  @Override
  public void stop() {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.stop();
      }
    });
  }

  @Override
  public void register(final ObjectId objectId) {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.register(objectId);
      }
    });
  }

  @Override
  public void register(final Collection<ObjectId> objectIds) {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.register(objectIds);
      }
    });
  }

  @Override
  public void unregister(final ObjectId objectId) {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.unregister(objectId);
      }
    });
  }

  @Override
  public void unregister(final Collection<ObjectId> objectIds) {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.unregister(objectIds);
      }
    });
  }

  @Override
  public void acknowledge(final AckHandle ackHandle) {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.acknowledge(ackHandle);
      }
    });
  }

 // End InvalidationClient methods.

  @Override  // InvalidationClientCore; overriding to add concurrency control.
  void handleIncomingMessage(final byte[] message) {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.handleIncomingMessage(message);
      }
    });
  }

  @Override  // InvalidationClientCore; overriding to add concurrency control.
  public void handleNetworkStatusChange(final boolean isOnline) {
    getResources().getInternalScheduler().schedule(NO_DELAY, new Runnable() {
      @Override
      public void run() {
        InvalidationClientImpl.super.handleNetworkStatusChange(isOnline);
      }
    });
  }
}
