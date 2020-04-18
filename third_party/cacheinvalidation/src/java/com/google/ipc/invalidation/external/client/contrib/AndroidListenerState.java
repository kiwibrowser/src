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
package com.google.ipc.invalidation.external.client.contrib;

import com.google.ipc.invalidation.external.client.types.ObjectId;
import com.google.ipc.invalidation.ticl.ProtoWrapperConverter;
import com.google.ipc.invalidation.ticl.TiclExponentialBackoffDelayGenerator;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.AndroidListenerState.RetryRegistrationState;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.AndroidListenerState.ScheduledRegistrationRetry;
import com.google.ipc.invalidation.ticl.proto.AndroidListenerProtocol.RegistrationCommand;
import com.google.ipc.invalidation.ticl.proto.Client.ExponentialBackoffState;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.Marshallable;
import com.google.ipc.invalidation.util.TypedUtil;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Random;
import java.util.Set;
import java.util.TreeMap;
import java.util.UUID;


/**
 * Encapsulates state to simplify persistence and tracking of changes. Internally maintains an
 * {@link #isDirty} bit. Call {@link #resetIsDirty} to indicate that changes have been persisted.
 *
 * <p>Notes on the {@link #desiredRegistrations} (DR) and {@link #delayGenerators} (DG) collections:
 * When the client application registers for an object, it is immediately added to DR. Similarly,
 * an object is removed from DR when the application unregisters. If a registration failure is
 * reported, the object is removed from DR if it exists and a delay generator is added to DG if one
 * does not already exist. (In the face of a failure, we assume that the registration is not desired
 * by the application unless/until the application retries.) When there is a successful
 * registration, the corresponding DG entry is removed. There are two independent collections rather
 * than one since we may be applying exponential backoff for an object when it is not in DR, and we
 * may have no reason to delay operations against an object in DR as well.
 *
 * <p>By removing objects from the {@link #desiredRegistrations} collection on failures, we are
 * essentially assuming that the client application doesn't care about the registration until we're
 * told otherwise -- by a subsequent call to register or unregister.
 *
 */
final class AndroidListenerState
    implements Marshallable<AndroidListenerProtocol.AndroidListenerState> {

  /**
   * Exponential backoff delay generators used to determine delay before registration retries.
   * There is a delay generator for every failing object.
   */
  private final Map<ObjectId, TiclExponentialBackoffDelayGenerator> delayGenerators =
      new HashMap<ObjectId, TiclExponentialBackoffDelayGenerator>();

  /** The set of registrations for which the client wants to be registered. */
  private final Set<ObjectId> desiredRegistrations;
  
  /** Pending registration retries, by execution time in milliseconds. */
  private final TreeMap<Long, RegistrationCommand> registrationRetries = new TreeMap<>();

  /** Random generator used for all delay generators. */
  private final Random random = new Random();

  /** Initial maximum retry delay for exponential backoff. */
  private final int initialMaxDelayMs;

  /** Maximum delay factor for exponential backoff (relative to {@link #initialMaxDelayMs}). */
  private final int maxDelayFactor;

  /** Sequence number for alarm manager request codes. */
  private int requestCodeSeqNum;

  /**
   * Dirty flag. {@code true} whenever changes are made, reset to false when {@link #resetIsDirty}
   * is called. State initialized from a proto is assumed to be initially clean.
   */
  private boolean isDirty;

  /**
   * The identifier for the current client. The ID is randomly generated and is used to ensure that
   * messages are not handled by the wrong client instance.
   */
  private final Bytes clientId;

  /** Initializes state for a new client. */
  AndroidListenerState(int initialMaxDelayMs, int maxDelayFactor) {
    desiredRegistrations = new HashSet<ObjectId>();
    clientId = createGloballyUniqueClientId();
    // Assigning a client ID dirties the state because calling the constructor twice produces
    // different results.
    isDirty = true;
    requestCodeSeqNum = 0;
    this.initialMaxDelayMs = initialMaxDelayMs;
    this.maxDelayFactor = maxDelayFactor;
  }

  /** Initializes state from proto. */
  AndroidListenerState(int initialMaxDelayMs, int maxDelayFactor,
      AndroidListenerProtocol.AndroidListenerState state) {
    desiredRegistrations = new HashSet<ObjectId>();
    for (ObjectIdP objectIdProto : state.getRegistration()) {
      desiredRegistrations.add(ProtoWrapperConverter.convertFromObjectIdProto(objectIdProto));
    }
    for (RetryRegistrationState retryState : state.getRetryRegistrationState()) {
      ObjectIdP objectIdP = retryState.getNullableObjectId();
      if (objectIdP == null) {
        continue;
      }
      ObjectId objectId = ProtoWrapperConverter.convertFromObjectIdProto(objectIdP);
      delayGenerators.put(objectId, new TiclExponentialBackoffDelayGenerator(random,
          initialMaxDelayMs, maxDelayFactor, retryState.getExponentialBackoffState()));
    }
    for (ScheduledRegistrationRetry registrationRetry : state.getRegistrationRetry()) {
      registrationRetries.put(registrationRetry.getExecuteTimeMs(), registrationRetry.getCommand());
    }
    clientId = state.getClientId();
    requestCodeSeqNum = state.getRequestCodeSeqNum();
    isDirty = false;
    this.initialMaxDelayMs = initialMaxDelayMs;
    this.maxDelayFactor = maxDelayFactor;
  }

  /** Increments and returns sequence number for alarm manager request codes. */
  int getNextRequestCode() {
    isDirty = true;
    return ++requestCodeSeqNum;
  }

  /**
   * See specs for {@link TiclExponentialBackoffDelayGenerator#getNextDelay}. Gets next delay for
   * the given {@code objectId}. If a delay generator does not yet exist for the object, one is
   * created.
   */
  int getNextDelay(ObjectId objectId) {
    TiclExponentialBackoffDelayGenerator delayGenerator =
        delayGenerators.get(objectId);
    if (delayGenerator == null) {
      delayGenerator = new TiclExponentialBackoffDelayGenerator(random, initialMaxDelayMs,
          maxDelayFactor);
      delayGenerators.put(objectId, delayGenerator);
    }
    // Requesting a delay from a delay generator modifies its internal state.
    isDirty = true;
    return delayGenerator.getNextDelay();
  }

  /** Inform that there has been a successful registration for an object. */
  void informRegistrationSuccess(ObjectId objectId) {
    // Since registration was successful, we can remove exponential backoff (if any) for the given
    // object.
    resetDelayGeneratorFor(objectId);
  }

  /**
   * Inform that there has been a registration failure.
   *
   * <p>Remove the object from the desired registrations collection whenever there's a failure. We
   * don't care if the op that failed was actually an unregister because we never suppress an
   * unregister request (even if the object is not in the collection). See
   * {@link AndroidListener#issueRegistration}.
   */
  public void informRegistrationFailure(ObjectId objectId, boolean isTransient) {
    removeDesiredRegistration(objectId);
    if (!isTransient) {
      // There should be no retries for the object, so remove any backoff state associated with it.
      resetDelayGeneratorFor(objectId);
    }
  }

  /**
   * If there is a backoff delay generator for the given object, removes it and sets dirty flag.
   */
  private void resetDelayGeneratorFor(ObjectId objectId) {
    if (TypedUtil.remove(delayGenerators, objectId) != null) {
      isDirty = true;
    }
  }

  /** Adds the given registration. Returns {@code true} if it was not already tracked. */
  boolean addDesiredRegistration(ObjectId objectId) {
    if (desiredRegistrations.add(objectId)) {
      isDirty = true;
      return true;
    }
    return false;
  }

  /** Removes the given registration. Returns {@code true} if it was actually tracked. */
  boolean removeDesiredRegistration(ObjectId objectId) {
    if (desiredRegistrations.remove(objectId)) {
      isDirty = true;
      return true;
    }
    return false;
  }

  /**
   * Resets the {@link #isDirty} flag to {@code false}. Call after marshalling and persisting state.
   */
  void resetIsDirty() {
    isDirty = false;
  }

  @Override
  public AndroidListenerProtocol.AndroidListenerState marshal() {
    List<ScheduledRegistrationRetry> registrationRetries =
        new ArrayList<>(this.registrationRetries.size());
    for (Entry<Long, RegistrationCommand> entry : this.registrationRetries.entrySet()) {
      registrationRetries.add(ScheduledRegistrationRetry.create(entry.getValue(), entry.getKey()));
    }
    return AndroidListenerProtos.newAndroidListenerState(
        clientId, requestCodeSeqNum, delayGenerators, desiredRegistrations, registrationRetries);
  }

  /**
   * Gets the identifier for the current client. Used to determine if registrations commands are
   * relevant to this instance.
   */
  Bytes getClientId() {
    return clientId;
  }

  /** Returns {@code true} iff registration is desired for the given object. */
  boolean containsDesiredRegistration(ObjectId objectId) {
    return TypedUtil.contains(desiredRegistrations, objectId);
  }
  

  /**
   * Returns (and removes) all scheduled registration retries scheduled before or at time {@code
   * nowMs}.
   */
  List<RegistrationCommand> takeRegistrationRetriesUpTo(long nowMs) {
    ArrayList<RegistrationCommand> commands = new ArrayList<>();
    while (!registrationRetries.isEmpty() && registrationRetries.firstKey() <= nowMs) {
      commands.add(registrationRetries.pollFirstEntry().getValue());
      isDirty = true;
    }
    return commands;
  }
  

  /**
   * If there are scheduled registration retries, returns the execute time in milliseconds for the
   * next retry. Otherwise, returns null.
   */
  
  Long getNextExecuteMs() {
    return registrationRetries.isEmpty() ? null : registrationRetries.firstEntry().getKey();
  }

  /** Adds a scheduled registration retry to listener state. */
  void addScheduledRegistrationRetry(ObjectId objectId, boolean isRegister, long executeMs) {
    RegistrationCommand command =
        isRegister
            ? AndroidListenerProtos.newDelayedRegisterCommand(clientId, objectId)
            : AndroidListenerProtos.newDelayedUnregisterCommand(clientId, objectId);
    
    // Avoid collisions on execute time.
    while (registrationRetries.containsKey(executeMs)) {
      executeMs++;
    }
    registrationRetries.put(executeMs, command);    
    isDirty = true;
  }

  /**
   * Returns {@code true} if changes have been made since the last successful call to
   * {@link #resetIsDirty}.
   */
  boolean getIsDirty() {
    return isDirty;
  }

  @Override
  public int hashCode() {
    // Since the client ID is globally unique, it's sufficient as a hashCode.
    return clientId.hashCode();
  }

  /**
   * Overridden for tests which compare listener states to verify that they have been correctly
   * (un)marshalled. We implement equals rather than exposing private data.
   */
  @Override
  public boolean equals(Object object) {
    if (this == object) {
      return true;
    }

    if (!(object instanceof AndroidListenerState)) {
      return false;
    }

    AndroidListenerState that = (AndroidListenerState) object;

    return (this.isDirty == that.isDirty)
        && (this.requestCodeSeqNum == that.requestCodeSeqNum)
        && (this.desiredRegistrations.size() == that.desiredRegistrations.size())
        && (this.desiredRegistrations.containsAll(that.desiredRegistrations))
        && TypedUtil.<Bytes>equals(this.clientId, that.clientId)
        && equals(this.delayGenerators, that.delayGenerators)
        && equals(this.registrationRetries, that.registrationRetries);
  }

  /** Compares the contents of two {@link #delayGenerators} maps. */
  private static boolean equals(Map<ObjectId, TiclExponentialBackoffDelayGenerator> x,
      Map<ObjectId, TiclExponentialBackoffDelayGenerator> y) {
    if (x.size() != y.size()) {
      return false;
    }
    for (Entry<ObjectId, TiclExponentialBackoffDelayGenerator> xEntry : x.entrySet()) {
      TiclExponentialBackoffDelayGenerator yGenerator = y.get(xEntry.getKey());
      if ((yGenerator == null) || !TypedUtil.<ExponentialBackoffState>equals(
          xEntry.getValue().marshal(), yGenerator.marshal())) {
        return false;
      }
    }
    return true;
  }
  
  /** Compares the contents of two {@link #registrationRetries} maps. */
  private static boolean equals(
      TreeMap<Long, RegistrationCommand> x, TreeMap<Long, RegistrationCommand> y) {
    if (x.size() != y.size()) {
      return false;
    }
    for (Entry<Long, RegistrationCommand> xEntry : x.entrySet()) {
      RegistrationCommand yGenerator = y.get(xEntry.getKey());
      if ((yGenerator == null)
          || Bytes.compare(xEntry.getValue().toByteArray(), yGenerator.toByteArray()) != 0) {
        return false;
      }
    }
    return true;    
  }

  @Override
  public String toString() {
    return String.format(Locale.ROOT, "AndroidListenerState[%s]: isDirty = %b, "
        + "desiredRegistrations.size() = %d, delayGenerators.size() = %d, requestCodeSeqNum = %d",
        clientId, isDirty, desiredRegistrations.size(), delayGenerators.size(), requestCodeSeqNum);
  }

  /**
   * Constructs a new globally unique ID for the client. Can be used to determine if commands
   * originated from this instance of the listener.
   */
  private static Bytes createGloballyUniqueClientId() {
    UUID guid = UUID.randomUUID();
    byte[] bytes = new byte[16];
    ByteBuffer buffer = ByteBuffer.wrap(bytes);
    buffer.putLong(guid.getLeastSignificantBits());
    buffer.putLong(guid.getMostSignificantBits());
    return new Bytes(bytes);
  }
}
