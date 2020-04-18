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

import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.SystemResources.NetworkChannel;
import com.google.ipc.invalidation.external.client.SystemResources.Scheduler;
import com.google.ipc.invalidation.external.client.SystemResources.Storage;
import com.google.ipc.invalidation.ticl.BasicSystemResources;
import com.google.ipc.invalidation.util.Preconditions;


/**
 * A builder to override some or all resource components in {@code SystemResources} . See
 * discussion in {@code ResourceComponent} as well.
 *
 */

  // The resources used for constructing the SystemResources in builder.
public class SystemResourcesBuilder {
  private Scheduler internalScheduler;
  private Scheduler listenerScheduler;
  private Logger logger;
  private NetworkChannel network;
  private Storage storage;
  private String platform;

  /** If the build method has been called on this builder. */
  private boolean sealed;

  /** See specs at {@code DefaultResourcesFactory.createDefaultResourcesBuilder}. */
  public SystemResourcesBuilder(Logger logger, Scheduler internalScheduler,
      Scheduler listenerScheduler, NetworkChannel network, Storage storage) {
    this.logger = logger;
    this.internalScheduler = internalScheduler;
    this.listenerScheduler = listenerScheduler;
    this.network = network;
    this.storage = storage;
  }

  /** Returns a new builder that shares all the resources of {@code builder} but is not sealed. */
  public SystemResourcesBuilder(SystemResourcesBuilder builder) {
    this.logger = builder.logger;
    this.internalScheduler = builder.internalScheduler;
    this.listenerScheduler = builder.listenerScheduler;
    this.network = builder.network;
    this.storage = builder.storage;
    this.sealed = false;
  }

  /** Returns the internal scheduler. */
  public Scheduler getInternalScheduler() {
    return internalScheduler;
  }

  /** Returns the listener scheduler. */
  public Scheduler getListenerScheduler() {
    return listenerScheduler;
  }

  /** Returns the network channel. */
  public NetworkChannel getNetwork() {
    return network;
  }

  /** Returns the logger. */
  public Logger getLogger() {
    return logger;
  }

  /** Returns the storage. */
  public Storage getStorage() {
    return storage;
  }

  /**
   * Sets the scheduler for scheduling internal events to be {@code internalScheduler}.
   * <p>
   * REQUIRES: {@link #build} has not been called.
   */
  public SystemResourcesBuilder setInternalScheduler(Scheduler internalScheduler) {
    Preconditions.checkState(!sealed, "Builder's build method has already been called");
    this.internalScheduler = internalScheduler;
    return this;
  }

  /**
   * Sets the scheduler for scheduling listener events to be {@code listenerScheduler}.
   * <p>
   * REQUIRES: {@link #build} has not been called.
   */
  public SystemResourcesBuilder setListenerScheduler(Scheduler listenerScheduler) {
    Preconditions.checkState(!sealed, "Builder's build method has already been called");
    this.listenerScheduler = listenerScheduler;
    return this;
  }

  /**
   * Sets the logger to be {@code logger}.
   * <p>
   * REQUIRES: {@link #build} has not been called.
   */
  public SystemResourcesBuilder setLogger(Logger logger) {
    Preconditions.checkState(!sealed, "Builder's build method has already been called");
    this.logger = logger;
    return this;
  }

  /**
   * Sets the network channel for communicating with the server to be {@code network}.
   * <p>
   * REQUIRES: {@link #build} has not been called.
   */
  public SystemResourcesBuilder setNetwork(NetworkChannel network) {
    Preconditions.checkState(!sealed, "Builder's build method has already been called");
    this.network = network;
    return this;
  }

  /**
   * Sets the persistence layer to be {@code storage}.
   * <p>
   * REQUIRES: {@link #build} has not been called.
   */
  public SystemResourcesBuilder setStorage(Storage storage) {
    Preconditions.checkState(!sealed, "Builder's build method has already been called");
    this.storage = storage;
    return this;
  }

  /**
   * Sets the platform to be {@code platform}.
   * <p>
   * REQUIRES: {@link #build} has not been called.
   */
  public SystemResourcesBuilder setPlatform(String platform) {
    Preconditions.checkState(!sealed, "Builder's build method has already been called");
    this.platform = platform;
    return this;
  }

  /**
   * Builds the {@code SystemResources} object with the given resource components and returns it.
   * <p>
   * Caller must not call any mutation method (on this SystemResourcesBuilder) after
   * {@code build} has been called (i.e., build and the set* methods)
   */
  public SystemResources build() {
    Preconditions.checkState(!sealed, "Builder's build method has already been called");
    seal();
    return new BasicSystemResources(logger, internalScheduler, listenerScheduler, network, storage,
        platform);
  }

  /** Seals the builder so that no mutation method can be called on this. */
  protected void seal() {
    Preconditions.checkState(!sealed, "Builder's already sealed");
    sealed = true;
  }
}
