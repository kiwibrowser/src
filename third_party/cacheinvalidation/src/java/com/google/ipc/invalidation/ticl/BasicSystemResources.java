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

import com.google.ipc.invalidation.external.client.SystemResources;


/**
 * A simple implementation of {@code SystemResources} that just takes the resource components
 * and constructs a SystemResources object.
 *
 */
public class BasicSystemResources implements SystemResources {

  // Components comprising the system resources. We delegate calls to these as appropriate.
  private final Scheduler internalScheduler;
  private final Scheduler listenerScheduler;
  private final Logger logger;
  private final NetworkChannel network;
  private final Storage storage;

  /** The state of the resources. */
  private RunState runState = new RunState();

  /** Information about the client operating system/platform, e.g., Windows, ChromeOS. */
  private final String platform;

  /**
   * Constructs an instance from resource components.
   *
   * @param logger implementation of the logger
   * @param internalScheduler scheduler for scheduling the library's internal events
   * @param listenerScheduler scheduler for scheduling the listener's events
   * @param network implementation of the network
   * @param storage implementation of storage
   * @param platform if not {@code null}, platform string for client version. If {@code null},
   *    a default string will be constructed.
   */
   public BasicSystemResources(Logger logger, Scheduler internalScheduler,
      Scheduler listenerScheduler, NetworkChannel network, Storage storage,
      String platform) {
    this.logger = logger;
    this.storage = storage;
    this.network = network;
    if (platform != null) {
      this.platform = platform;
    } else {
      // If a platform string was not provided, try to compute a reasonable default.
      this.platform = System.getProperty("os.name") + "/" + System.getProperty("os.version") +
          "/" + System.getProperty("os.arch");
    }

    this.internalScheduler = internalScheduler;
    this.listenerScheduler = listenerScheduler;

    // Pass a reference to this object to all of the components, so that they can access
    // resources. E.g., so that the network can do logging.
    logger.setSystemResources(this);
    storage.setSystemResources(this);
    network.setSystemResources(this);
    internalScheduler.setSystemResources(this);
    listenerScheduler.setSystemResources(this);
  }

  @Override
  public void start() {
    runState.start();
    logger.info("Resources started");
  }

  @Override
  public void stop() {
    runState.stop();
    logger.info("Resources stopped");
  }

  @Override
  public boolean isStarted() {
    return runState.isStarted();
  }

  @Override
  public Logger getLogger() {
    return logger;
  }

  @Override
  public Storage getStorage() {
    return storage;
  }

  @Override
  public NetworkChannel getNetwork() {
    return network;
  }

  @Override
  public Scheduler getInternalScheduler() {
    return internalScheduler;
  }

  @Override
  public Scheduler getListenerScheduler() {
    return listenerScheduler;
  }

  @Override
  public String getPlatform() {
    return platform;
  }
}
