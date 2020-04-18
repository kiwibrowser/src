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
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.ticl.Statistics.ClientErrorType;
import com.google.ipc.invalidation.ticl.TestableInvalidationClient.RegistrationManagerState;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationP.OpType;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationStatus;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationSubtree;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationSummary;
import com.google.ipc.invalidation.ticl.proto.CommonProtos;
import com.google.ipc.invalidation.ticl.proto.JavaClient.RegistrationManagerStateP;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.InternalBase;
import com.google.ipc.invalidation.util.Marshallable;
import com.google.ipc.invalidation.util.TextBuilder;
import com.google.ipc.invalidation.util.TypedUtil;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;


/**
 * Object to track desired client registrations. This class belongs to caller (e.g.,
 * InvalidationClientImpl) and is not thread-safe - the caller has to use this class in a
 * thread-safe manner.
 *
 */
class RegistrationManager extends InternalBase implements Marshallable<RegistrationManagerStateP> {

  /** Prefix used to request all registrations. */
  static final byte[] EMPTY_PREFIX = new byte[]{};

  /** The set of regisrations that the application has requested for. */
  private DigestStore<ObjectIdP> desiredRegistrations;

  /** Statistics objects to track number of sent messages, etc. */
  private final Statistics statistics;

  /** Latest known server registration state summary. */
  private RegistrationSummary lastKnownServerSummary;

  /**
   * Map of object ids and operation types for which we have not yet issued any registration-status
   * upcall to the listener. We need this so that we can synthesize success upcalls if registration
   * sync, rather than a server message, communicates to us that we have a successful
   * (un)registration.
   * <p>
   * This is a map from object id to type, rather than a set of {@code RegistrationP}, because
   * a set of {@code RegistrationP} would assume that we always get a response for every operation
   * we issue, which isn't necessarily true (i.e., the server might send back an unregistration
   * status in response to a registration request).
   */
  private final Map<ObjectIdP, Integer> pendingOperations = new HashMap<ObjectIdP, Integer>();

  private final Logger logger;

  public RegistrationManager(Logger logger, Statistics statistics, DigestFunction digestFn,
      RegistrationManagerStateP registrationManagerState) {
    this.logger = logger;
    this.statistics = statistics;
    this.desiredRegistrations = new SimpleRegistrationStore(digestFn);

    if (registrationManagerState == null) {
      // Initialize the server summary with a 0 size and the digest corresponding
      // to it.  Using defaultInstance would wrong since the server digest will
      // not match unnecessarily and result in an info message being sent.
      this.lastKnownServerSummary = getRegistrationSummary();
    } else {
      this.lastKnownServerSummary = registrationManagerState.getNullableLastKnownServerSummary();
      if (this.lastKnownServerSummary == null) {
        // If no server summary is set, use a default with size 0.
        this.lastKnownServerSummary = getRegistrationSummary();
      }
      desiredRegistrations.add(registrationManagerState.getRegistrations());
      for (RegistrationP regOp : registrationManagerState.getPendingOperations()) {
        pendingOperations.put(regOp.getObjectId(), regOp.getOpType());
      }
    }
  }

  /**
   * Returns a copy of the registration manager's state
   * <p>
   * Direct test code MUST not call this method on a random thread. It must be called on the
   * InvalidationClientImpl's internal thread.
   */
  
  RegistrationManagerState getRegistrationManagerStateCopyForTest() {
    List<ObjectIdP> registeredObjects = new ArrayList<ObjectIdP>();
    registeredObjects.addAll(desiredRegistrations.getElements(EMPTY_PREFIX, 0));
    return new RegistrationManagerState(getRegistrationSummary(), lastKnownServerSummary,
        registeredObjects);
  }

  /**
   * Sets the digest store to be {@code digestStore} for testing purposes.
   * <p>
   * REQUIRES: This method is called before the Ticl has done any operations on this object.
   */
  
  void setDigestStoreForTest(DigestStore<ObjectIdP> digestStore) {
    this.desiredRegistrations = digestStore;
    this.lastKnownServerSummary = getRegistrationSummary();
  }

  /** Perform registration/unregistation for all objects in {@code objectIds}. */
  Collection<ObjectIdP> performOperations(Collection<ObjectIdP> objectIds, int regOpType) {
    // Record that we have pending operations on the objects.
    for (ObjectIdP objectId : objectIds) {
      pendingOperations.put(objectId, regOpType);
    }
    // Update the digest appropriately.
    if (regOpType == RegistrationP.OpType.REGISTER) {
      return desiredRegistrations.add(objectIds);
    } else {
      return desiredRegistrations.remove(objectIds);
    }
  }

  /**
   * Returns a registration subtree for registrations where the digest of the object id begins with
   * the prefix {@code digestPrefix} of {@code prefixLen} bits. This method may also return objects
   * whose digest prefix does not match {@code digestPrefix}.
   */
  RegistrationSubtree getRegistrations(byte[] digestPrefix, int prefixLen) {
    return RegistrationSubtree.create(desiredRegistrations.getElements(digestPrefix, prefixLen));
  }

  /**
   * Handles registration operation statuses from the server. Returns a list of booleans, one per
   * registration status, that indicates whether the registration operation was both successful and
   * agreed with the desired client state (i.e., for each registration status,
   * (status.optype == register) == desiredRegistrations.contains(status.objectid)).
   * <p>
   * REQUIRES: the caller subsequently make an informRegistrationStatus or informRegistrationFailure
   * upcall on the listener for each registration in {@code registrationStatuses}.
   */
  List<Boolean> handleRegistrationStatus(List<RegistrationStatus> registrationStatuses) {
    // Local-processing result code for each element of registrationStatuses.
    List<Boolean> localStatuses = new ArrayList<Boolean>(registrationStatuses.size());
    for (RegistrationStatus registrationStatus : registrationStatuses) {
      ObjectIdP objectIdProto = registrationStatus.getRegistration().getObjectId();

      // The object is no longer pending, since we have received a server status for it, so
      // remove it from the pendingOperations map. (It may or may not have existed in the map,
      // since we can receive spontaneous status messages from the server.)
      TypedUtil.remove(pendingOperations, objectIdProto);

      // We start off with the local-processing set as success, then potentially fail.
      boolean isSuccess = true;

      // if the server operation succeeded, then local processing fails on "incompatibility" as
      // defined above.
      if (CommonProtos.isSuccess(registrationStatus.getStatus())) {
        boolean appWantsRegistration = desiredRegistrations.contains(objectIdProto);
        boolean isOpRegistration =
            registrationStatus.getRegistration().getOpType() == RegistrationP.OpType.REGISTER;
        boolean discrepancyExists = isOpRegistration ^ appWantsRegistration;
        if (discrepancyExists) {
          // Remove the registration and set isSuccess to false, which will cause the caller to
          // issue registration-failure to the application.
          desiredRegistrations.remove(objectIdProto);
          statistics.recordError(ClientErrorType.REGISTRATION_DISCREPANCY);
          logger.info("Ticl discrepancy detected: registered = %s, requested = %s. " +
              "Removing %s from requested",
              isOpRegistration, appWantsRegistration, objectIdProto);
          isSuccess = false;
        }
      } else {
        // If the server operation failed, then also local processing fails.
        desiredRegistrations.remove(objectIdProto);
        logger.fine("Removing %s from committed", objectIdProto);
        isSuccess = false;
      }
      localStatuses.add(isSuccess);
    }
    return localStatuses;
  }

  /**
   * Removes all desired registrations and pending operations. Returns all object ids
   * that were affected.
   * <p>
   * REQUIRES: the caller issue a permanent failure upcall to the listener for all returned object
   * ids.
   */
  Collection<ObjectIdP> removeRegisteredObjects() {
    int numObjects = desiredRegistrations.size() + pendingOperations.size();
    Set<ObjectIdP> failureCalls = new HashSet<ObjectIdP>(numObjects);
    failureCalls.addAll(desiredRegistrations.removeAll());
    failureCalls.addAll(pendingOperations.keySet());
    pendingOperations.clear();
    return failureCalls;
  }

  //
  // Digest-related methods
  //

  /** Returns a summary of the desired registrations. */
  RegistrationSummary getRegistrationSummary() {
    return RegistrationSummary.create(desiredRegistrations.size(),
        new Bytes(desiredRegistrations.getDigest()));
  }

  /**
   * Informs the manager of a new registration state summary from the server.
   * Returns a possibly-empty map of <object-id, reg-op-type>. For each entry in the map,
   * the caller should make an inform-registration-status upcall on the listener.
   */
  Set<RegistrationP> informServerRegistrationSummary(
      RegistrationSummary regSummary) {
    if (regSummary != null) {
      this.lastKnownServerSummary = regSummary;
    }
    if (isStateInSyncWithServer()) {
      // If we are now in sync with the server, then the caller should make inform-reg-status
      // upcalls for all operations that we had pending, if any; they are also no longer pending.
      Set<RegistrationP> upcallsToMake = new HashSet<RegistrationP>(pendingOperations.size());
      for (Map.Entry<ObjectIdP, Integer> entry : pendingOperations.entrySet()) {
        ObjectIdP objectId = entry.getKey();
        boolean isReg = entry.getValue() == OpType.REGISTER;
        upcallsToMake.add(CommonProtos.newRegistrationP(objectId, isReg));
      }
      pendingOperations.clear();
      return upcallsToMake;
    } else {
      // If we are not in sync with the server, then the caller should make no upcalls.
      return Collections.emptySet();
    }
  }

  /**
   * Returns whether the local registration state and server state agree, based on the last
   * received server summary (from {@link #informServerRegistrationSummary}).
   */
  boolean isStateInSyncWithServer() {
    return TypedUtil.<RegistrationSummary>equals(lastKnownServerSummary, getRegistrationSummary());
  }

  @Override
  public void toCompactString(TextBuilder builder) {
    builder.appendFormat("Last known digest: %s, Requested regs: %s", lastKnownServerSummary,
        desiredRegistrations);
  }

  @Override
  public RegistrationManagerStateP marshal() {
    List<ObjectIdP> desiredRegistrations =
        new ArrayList<ObjectIdP>(this.desiredRegistrations.getElements(EMPTY_PREFIX, 0));
    List<RegistrationP> pendingOperations =
        new ArrayList<RegistrationP>(this.pendingOperations.size());
    for (Map.Entry<ObjectIdP, Integer> entry : this.pendingOperations.entrySet()) {
      pendingOperations.add(RegistrationP.create(entry.getKey(), entry.getValue()));
    }
    return RegistrationManagerStateP.create(desiredRegistrations, lastKnownServerSummary,
        pendingOperations);
  }
}
