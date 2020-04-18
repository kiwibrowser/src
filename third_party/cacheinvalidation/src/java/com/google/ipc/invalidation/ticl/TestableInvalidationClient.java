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

import com.google.ipc.invalidation.common.DigestFunction;
import com.google.ipc.invalidation.external.client.InvalidationClient;
import com.google.ipc.invalidation.external.client.InvalidationListener;
import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.ticl.proto.ChannelCommon.NetworkEndpointId;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientConfigP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationSummary;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.InternalBase;
import com.google.ipc.invalidation.util.Preconditions;
import com.google.ipc.invalidation.util.TextBuilder;

import java.util.ArrayList;
import java.util.Collection;


/**
 * An interface that exposes some extra methods for testing an invalidation client implementation.
 *
 */
public interface TestableInvalidationClient extends InvalidationClient {

  /** The state of the registration manager exposed for testing. */
  public class RegistrationManagerState extends InternalBase {

    /** The registration summary of all objects registered by the client (known at the client). */
    private final RegistrationSummary clientSummary;

    /** The last known registration summary from the server. */
    private final RegistrationSummary serverSummary;

    /** The objects registered by the client (as known at the client). */
    private final Collection<ObjectIdP> registeredObjects;

    public RegistrationManagerState(RegistrationSummary clientSummary,
        RegistrationSummary serverSummary, ObjectIdP[] registeredObjects) {
      this(clientSummary, serverSummary, new ArrayList<ObjectIdP>(registeredObjects.length));
      for (ObjectIdP registeredObject : registeredObjects) {
        this.registeredObjects.add(registeredObject);
      }
    }

    public RegistrationManagerState(RegistrationSummary clientSummary,
        RegistrationSummary serverSummary, Collection<ObjectIdP> registeredObjects) {
      this.clientSummary = Preconditions.checkNotNull(clientSummary);
      this.serverSummary = Preconditions.checkNotNull(serverSummary);
      this.registeredObjects = Preconditions.checkNotNull(registeredObjects);
    }

    public RegistrationSummary getClientSummary() {
      return clientSummary;
    }

    public RegistrationSummary getServerSummary() {
      return serverSummary;
    }

    public Collection<ObjectIdP> getRegisteredObjects() {
      return registeredObjects;
    }

    @Override
    public void toCompactString(TextBuilder builder) {
      builder.append("<RegistrationManagerState: clientSummary=").append(clientSummary);
      builder.append(", serverSummary=").append(serverSummary);
      builder.append(", registeredObjects=<").append(registeredObjects).append(">");
    }
  }

  /** Returns whether the Ticl is started. */
  boolean isStartedForTest();

  /** Stops the system resources. */
  void stopResources();

  /** Returns the current time on the client. */
  long getResourcesTimeMs();

  /** Returns the client internal scheduler */
  SystemResources.Scheduler getInternalSchedulerForTest();

  /** Returns the client storage. */
  SystemResources.Storage getStorage();

  /** Returns a snapshot of the performance counters/statistics . */
  Statistics getStatisticsForTest();

  /** Returns the digest function used for computing digests for object registrations. */
  DigestFunction getDigestFunctionForTest();

  /**
   * Returns a copy of the registration manager's state
   * <p>
   * REQUIRES: This method is called on the internal scheduler.
   */
  RegistrationManagerState getRegistrationManagerStateCopyForTest();

  /**
   * Changes the existing delay for the network timeout delay in the operation scheduler to be
   * {@code delayMs}.
   */
  void changeNetworkTimeoutDelayForTest(int delayMs);

  /**
   * Changes the existing delay for the heartbeat delay in the operation scheduler to be
   * {@code delayMs}.
   */
  void changeHeartbeatDelayForTest(int delayMs);

  /**
   * Sets the digest store to be {@code digestStore} for testing purposes.
   * <p>
   * REQUIRES: This method is called before the Ticl has been started.
   */
  void setDigestStoreForTest(DigestStore<ObjectIdP> digestStore);

  /** Returns the client id that is used for squelching invalidations on the server side. */
  byte[] getApplicationClientIdForTest();

  /** Returns the listener that was registered by the caller. */
  InvalidationListener getInvalidationListenerForTest();

  /** Returns the current client token. */
  Bytes getClientTokenForTest();

  /** Returns the single key used to write all the Ticl state. */
  String getClientTokenKeyForTest();

  /** Returns the next time a message is allowed to be sent to the server (could be in the past). */
  long getNextMessageSendTimeMsForTest();

  /** Returns the configuration used by the client. */
  ClientConfigP getConfigForTest();

  /**
   * Returns the network endpoint id of the client. May throw {@code UnsupportedOperationException}.
   */
  NetworkEndpointId getNetworkIdForTest();
}
