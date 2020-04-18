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

import com.google.ipc.invalidation.ticl.InvalidationClientCore;
import com.google.ipc.invalidation.ticl.InvalidationClientImpl;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientConfigP;

import java.util.Random;

/**
 * Factory for creating invalidation clients.
 *
 */
public class InvalidationClientFactory {
  /**
   * Constructs an invalidation client library instance.
   *
   * @param resources {@link SystemResources} to use for logging, scheduling, persistence, and
   *     network connectivity
   * @param clientConfig application provided configuration for the client.
   * @param listener callback object for invalidation events
   */
  public static InvalidationClient createClient(SystemResources resources,
      InvalidationClientConfig clientConfig, InvalidationListener listener) {
    ClientConfigP.Builder internalConfigBuilder = InvalidationClientCore.createConfig().toBuilder();
    internalConfigBuilder.allowSuppression = clientConfig.allowSuppression;
    ClientConfigP internalConfig = internalConfigBuilder.build();
    Random random = new Random(resources.getInternalScheduler().getCurrentTimeMs());
    return new InvalidationClientImpl(resources, random, clientConfig.clientType,
        clientConfig.clientName, internalConfig, clientConfig.applicationName, listener);
  }

  private InvalidationClientFactory() {} // Prevents instantiation.
}
