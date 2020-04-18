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

package com.google.ipc.invalidation.external.client.types;

import com.google.ipc.invalidation.util.Preconditions;

import java.util.Arrays;

/**
 * An identifier for application clients in an application-defined way. I.e., a client name in an
 * application naming scheme. This is not interpreted by the invalidation system - however, it is
 * used opaquely to squelch invalidations for the cient causing an update, e.g., if a client C
 * whose app client id is C.appClientId changes object X and the backend store informs the backend
 * invalidation sytsem that X was modified by X.appClientId, the invalidation to C can then be
 * squelched by the invalidation system.
 *
 */
public final class ApplicationClientId {

  /** The opaque id of the client application. */
  private final byte[] clientName;

  /**
   * Creates an application client id for the given {@code clientName} (does not make a copy of the
   * byte array).
   */
  public static ApplicationClientId newInstance(byte[] appClientId) {
    return new ApplicationClientId(appClientId);
  }

  /** Creates an application id for the given {@code clientName}. */
  private ApplicationClientId(byte[] clientName) {
    this.clientName = Preconditions.checkNotNull(clientName, "clientName");
  }

  public byte[] getClientName() {
    return clientName;
  }

  @Override
  public boolean equals(Object object) {
    if (object == this) {
      return true;
    }

    if (!(object instanceof ApplicationClientId)) {
      return false;
    }

    final ApplicationClientId other = (ApplicationClientId) object;
    return Arrays.equals(clientName, other.clientName);
  }

  @Override
  public int hashCode() {
    return Arrays.hashCode(clientName);
  }

  @Override
  public String toString() {
    return "AppClientId: <, " + BytesFormatter.toString(clientName) + ">";
  }
}
