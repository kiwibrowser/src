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

/**
 * Application-provided configuration for an invalidation client.
 *
 */
public class InvalidationClientConfig {

  /** Client type code as assigned by the notification system's backend. */
  public final int clientType;

  /** Id/name of the client in the application's own naming scheme. */
  public final byte[] clientName;

  /** Name of the application using the library (for debugging/monitoring) */
  public final String applicationName;

  /** If false, invalidateUnknownVersion() is called whenever suppression occurs. */
  public final boolean allowSuppression;

  public InvalidationClientConfig(int clientType, byte[] clientName, String applicationName,
      boolean allowSuppression) {
    this.clientType = clientType;
    this.clientName = clientName;
    this.applicationName = applicationName;
    this.allowSuppression = allowSuppression;
  }
}
