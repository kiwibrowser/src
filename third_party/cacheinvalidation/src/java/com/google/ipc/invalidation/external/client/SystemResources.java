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

package com.google.ipc.invalidation.external.client;

import com.google.ipc.invalidation.external.client.types.Callback;
import com.google.ipc.invalidation.external.client.types.SimplePair;
import com.google.ipc.invalidation.external.client.types.Status;
import com.google.ipc.invalidation.util.BaseLogger;

/**
 * Interfaces for the system resources used by the Ticl. System resources are an abstraction layer
 * over the host operating system that provides the Ticl with the ability to schedule events, send
 * network messages, store data, and perform logging.
 * <p>
 * NOTE: All the resource types and SystemResources are required to be thread-safe.
 *
 */
public interface SystemResources {

  /** Interface specifying the logging functionality provided by {@link SystemResources}. */
  public interface Logger extends BaseLogger, ResourceComponent {}

  /** Interface specifying the scheduling functionality provided by {@link SystemResources}. */
  public interface Scheduler extends ResourceComponent {

    /** Symbolic constant representing no scheduling delay, for readability. */
    static final int NO_DELAY = 0;

    /**
     * Schedules {@code runnable} to be run on scheduler's thread after at least {@code delayMs}
     * milliseconds.
     */
    void schedule(int delayMs, Runnable runnable);

    /** Returns whether the current code is executing on the scheduler's thread. */
    boolean isRunningOnThread();

    /**
     * Returns the current time in milliseconds since *some* epoch (NOT necessarily the UNIX epoch).
     * The only requirement is that this time advance at the rate of real time.
     */
    long getCurrentTimeMs();
  }

  /** Interface specifying the network functionality provided by {@link SystemResources}. */
  public interface NetworkChannel extends ResourceComponent {
    /** Interface implemented by listeners for network events. */
    public interface NetworkListener {
      /** Upcall made when a network message has been received from the data center. */
      // Implementation note: this is currently a serialized ServerToClientMessage protocol buffer.
      // Implementors MAY NOT rely on this fact.
      void onMessageReceived(byte[] message);

      /**
       * Upcall made when the network online status has changed. It will be invoked with
       * a boolean indicating whether the network is connected.
       * <p>
       * This is a best-effort upcall. Note that indicating incorrectly that the network is
       * connected can result in unnecessary calls for {@link #sendMessage}. Incorrect information
       * that the network is disconnected can result in messages not being sent by the client
       * library.
       */
      void onOnlineStatusChange(boolean isOnline);

      /**
       * Upcall made when the network address has changed. Note that the network channel
       * implementation is responsible for determining what constitutes the network address and what
       * it means to have it change.
       * <p>
       * This is a best-effort call; however, failure to invoke it may prevent the client from
       * receiving messages and cause it to behave as though offline until its next heartbeat.
       */
      void onAddressChange();
    }

    /** Sends {@code outgoingMessage} to the data center. */
    // Implementation note: this is currently a serialized ClientToServerMessage protocol buffer.
    // Implementors MAY NOT rely on this fact.
    void sendMessage(byte[] outgoingMessage);

    /**
     * Sets the {@link NetworkListener} to which events will be delivered.
     * <p>
     * REQUIRES: no listener already be registered.
     */
    void setListener(NetworkListener listener);
  }

  /**
   * Interface specifying the storage functionality provided by {@link SystemResources}. Basically,
   * the required functionality is a small subset of the method of a regular hash map.
   */
  public interface Storage extends ResourceComponent {

    /**
     * Attempts to persist {@code value} for the given {@code key}. Invokes {@code done} when
     * finished, passing a value that indicates whether it was successful.
     * <p>
     * Note: If a wrie W1 finishes unsuccessfully and then W2 is issued for the same key and W2
     * finishes successfully, W1 must NOT later overwrite W2.
     * <p>
     * REQUIRES: Neither {@code key} nor {@code value} is null.
     */
    void writeKey(String key, byte[] value, Callback<Status> done);

    /**
     * Reads the value corresponding to {@code key} and calls {@code done} with the result.
     * If it finds the key, passes a success status and the value. Else passes a failure status
     * and a null value.
     */
    void readKey(String key, Callback<SimplePair<Status, byte[]>> done);

    /**
     * Deletes the key, value pair corresponding to {@code key}. If the deletion succeeds, calls
     * {@code done} with true; else calls it with false. A deletion of a key that does not exist
     * is considered to have succeeded.
     */
    void deleteKey(String key, Callback<Boolean> done);

    /**
     * Reads all the keys from the underlying store and then calls {@code keyCallback} with
     * each key that was written earlier and not deleted. When all the keys are done, calls
     * {@code keyCallback} with {@code null}. With each key, the code can indicate a
     * failed status, in which case the iteration stops.
     */
    void readAllKeys(Callback<SimplePair<Status, String>> keyCallback);
  }

  /**
   * Interface for a component of a {@link SystemResources} implementation constructed by calls to
   * set* methods of {@link SystemResourcesBuilder}.
   * <p>
   * The SystemResourcesBuilder allows applications to create a single {@link SystemResources}
   * implementation by composing individual building blocks, each of which implements one of the
   * four required interfaces ({@link Logger}, {@link Storage}, {@link NetworkChannel},
   * {@link Scheduler}).
   * <p>
   * However, each interface implementation may require functionality from another. For example, the
   * network implementation may need to do logging. In order to allow this, we require that the
   * interface implementations also implement {@code ResourceComponent}, which specifies the single
   * method {@link #setSystemResources}. It is guaranteed that this method will be invoked exactly
   * once on each interface implementation and before any other calls are made. Implementations can
   * then save a reference to the provided resources for later use.
   * <p>
   * Note: for the obvious reasons of infinite recursion, implementations should not attempt to
   * access themselves through the provided {@link SystemResources}.
   */
  public interface ResourceComponent {

    /** Supplies a {@link SystemResources} instance to the component. */
    void setSystemResources(SystemResources resources);
  }

  //
  // End of nested interfaces
  //

  /**
   * Starts the resources.
   * <p>
   * REQUIRES: This method is called before the resources are used.
   */
  void start();

  /**
   * Stops the resources. After this point, all the resources will eventually stop doing any work
   * (e.g., scheduling, sending/receiving messages from the network etc). They will eventually
   * convert any further operations to no-ops.
   * <p>
   * REQUIRES: Start has been called.
   */
  void stop();

  /** Returns whether the resources are started. */
  boolean isStarted();

  /**
   * Returns information about the client operating system/platform, e.g., Windows, ChromeOS (for
   * debugging/monitoring purposes).
   */
  String getPlatform();

  /** Returns an object that can be used to do logging. */
  Logger getLogger();

  /** Returns an object that can be used to persist data locally. */
  Storage getStorage();

  /** Returns an object that can be used to send and receive messages. */
  NetworkChannel getNetwork();

  /** Returns an object that can be used by the client library to schedule its internal events. */
  Scheduler getInternalScheduler();

  /** Returns an object that can be used to schedule events for the application. */
  Scheduler getListenerScheduler();
}
