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

import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.types.SimplePair;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.PropertyRecord;
import com.google.ipc.invalidation.ticl.proto.JavaClient.StatisticsState;
import com.google.ipc.invalidation.util.InternalBase;
import com.google.ipc.invalidation.util.Marshallable;
import com.google.ipc.invalidation.util.TextBuilder;
import com.google.ipc.invalidation.util.TypedUtil;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Statistics for the Ticl, e.g., number of registration calls, number of token mismatches, etc.
 *
 */
public class Statistics extends InternalBase implements Marshallable<StatisticsState> {

  // Implementation: To classify the statistics a bit better, we have a few enums to track different
  // types of statistics, e.g., sent message types, errors, etc. For each statistic type, we create
  // a map and provide a method to record an event for each type of statistic.

  /** Types of messages sent to the server: {@code ClientToServerMessage} for their description. */
  public enum SentMessageType {
    INFO,
    INITIALIZE,
    INVALIDATION_ACK,
    REGISTRATION,
    REGISTRATION_SYNC,
    TOTAL,  // Refers to the actual ClientToServerMessage message sent on the network.
  }

  /**
   * Types of messages received from the server: {@code ServerToClientMessage} for their
   * description.
   */
  public enum ReceivedMessageType {
    INFO_REQUEST,
    INVALIDATION,
    REGISTRATION_STATUS,
    REGISTRATION_SYNC_REQUEST,
    TOKEN_CONTROL,
    ERROR,
    CONFIG_CHANGE,
    STALE_INVALIDATION,  // An already acked INVALIDATION.
    TOTAL, // Refers to the actual ServerToClientMessage messages received from the network.
  }

  /** Interesting API calls coming from the application ({@code InvalidationClient}). */
  public enum IncomingOperationType {
    ACKNOWLEDGE,
    REGISTRATION,
    UNREGISTRATION,
  }

  /** Different types of events issued by the {@code InvalidationListener}). */
  public enum ListenerEventType {
    INFORM_ERROR,
    INFORM_REGISTRATION_FAILURE,
    INFORM_REGISTRATION_STATUS,
    INVALIDATE,
    INVALIDATE_ALL,
    INVALIDATE_UNKNOWN,
    REISSUE_REGISTRATIONS,
  }

  /** Different types of errors observed by the Ticl. */
  public enum ClientErrorType {
    /** Acknowledge call received from client with a bad handle. */
    ACKNOWLEDGE_HANDLE_FAILURE,

    /** Incoming message dropped due to parsing, validation problems. */
    INCOMING_MESSAGE_FAILURE,

    /** Tried to send an outgoing message that was invalid. */
    OUTGOING_MESSAGE_FAILURE,

    /** Persistent state failed to deserialize correctly. */
    PERSISTENT_DESERIALIZATION_FAILURE,

    /** Read of blob from persistent state failed. */
    PERSISTENT_READ_FAILURE,

    /** Write of blob from persistent state failed. */
    PERSISTENT_WRITE_FAILURE,

    /** Message received with incompatible protocol version. */
    PROTOCOL_VERSION_FAILURE,

    /**
     * Registration at client and server is different, e.g., client thinks it is registered while
     * the server says it is unregistered (of course, sync will fix it).
     */
    REGISTRATION_DISCREPANCY,

    /** The nonce from the server did not match the current nonce by the client. */
    NONCE_MISMATCH,

    /** The current token at the client is different from the token in the incoming message. */
    TOKEN_MISMATCH,

    /** No message sent due to token missing. */
    TOKEN_MISSING_FAILURE,

    /** Received a message with a token (transient) failure. */
    TOKEN_TRANSIENT_FAILURE,
  }

  // Names of statistics types. Do not rely on reflection to determine type names because Proguard
  // may change them for Android clients.
  private static final String SENT_MESSAGE_TYPE_NAME = "SentMessageType";
  private static final String INCOMING_OPERATION_TYPE_NAME = "IncomingOperationType";
  private static final String RECEIVED_MESSAGE_TYPE_NAME = "ReceivedMessageType";
  private static final String LISTENER_EVENT_TYPE_NAME = "ListenerEventType";
  private static final String CLIENT_ERROR_TYPE_NAME = "ClientErrorType";

  // Map from stats enum names to values. Used in place of Enum.valueOf() because this method
  // invokes Enum.values() via reflection, and that method may be renamed by Proguard.
  private static final Map<String, SentMessageType> SENT_MESSAGE_TYPE_NAME_TO_VALUE_MAP =
      createValueOfMap(SentMessageType.values());
  private static final Map<String, IncomingOperationType>
      INCOMING_OPERATION_TYPE_NAME_TO_VALUE_MAP = createValueOfMap(IncomingOperationType.values());
  private static final Map<String, ReceivedMessageType> RECEIVED_MESSAGE_TYPE_NAME_TO_VALUE_MAP =
      createValueOfMap(ReceivedMessageType.values());
  private static final Map<String, ListenerEventType> LISTENER_EVENT_TYPE_NAME_TO_VALUE_MAP =
      createValueOfMap(ListenerEventType.values());
  private static final Map<String, ClientErrorType> CLIENT_ERROR_TYPE_NAME_TO_VALUE_MAP =
      createValueOfMap(ClientErrorType.values());

  // Maps for each type of Statistic to keep track of how many times each event has occurred.

  private final Map<SentMessageType, Integer> sentMessageTypes =
      new HashMap<SentMessageType, Integer>();
  private final Map<ReceivedMessageType, Integer> receivedMessageTypes =
      new HashMap<ReceivedMessageType, Integer>();
  private final Map<IncomingOperationType, Integer> incomingOperationTypes =
      new HashMap<IncomingOperationType, Integer>();
  private final Map<ListenerEventType, Integer> listenerEventTypes =
      new HashMap<ListenerEventType, Integer>();
  private final Map<ClientErrorType, Integer> clientErrorTypes =
      new HashMap<ClientErrorType, Integer>();

  public Statistics() {
    initializeMap(sentMessageTypes, SentMessageType.values());
    initializeMap(receivedMessageTypes, ReceivedMessageType.values());
    initializeMap(incomingOperationTypes, IncomingOperationType.values());
    initializeMap(listenerEventTypes, ListenerEventType.values());
    initializeMap(clientErrorTypes, ClientErrorType.values());
  }

  /** Returns a copy of this. */
  public Statistics getCopyForTest() {
    Statistics statistics = new Statistics();
    statistics.sentMessageTypes.putAll(sentMessageTypes);
    statistics.receivedMessageTypes.putAll(receivedMessageTypes);
    statistics.incomingOperationTypes.putAll(incomingOperationTypes);
    statistics.listenerEventTypes.putAll(listenerEventTypes);
    statistics.clientErrorTypes.putAll(clientErrorTypes);
    return statistics;
  }

  /** Returns the counter value for {@code clientErrorType}. */
  int getClientErrorCounterForTest(ClientErrorType clientErrorType) {
    return TypedUtil.mapGet(clientErrorTypes, clientErrorType);
  }

  /** Returns the counter value for {@code sentMessageType}. */
  int getSentMessageCounterForTest(SentMessageType sentMessageType) {
    return TypedUtil.mapGet(sentMessageTypes, sentMessageType);
  }

  /** Returns the counter value for {@code receivedMessageType}. */
  int getReceivedMessageCounterForTest(ReceivedMessageType receivedMessageType) {
    return TypedUtil.mapGet(receivedMessageTypes, receivedMessageType);
  }

  /** Records the fact that a message of type {@code sentMessageType} has been sent. */
  public void recordSentMessage(SentMessageType sentMessageType) {
    incrementValue(sentMessageTypes, sentMessageType);
  }

  /** Records the fact that a message of type {@code receivedMessageType} has been received. */
  public void recordReceivedMessage(ReceivedMessageType receivedMessageType) {
    incrementValue(receivedMessageTypes, receivedMessageType);
  }

  /**
   * Records the fact that the application has made a call of type
   * {@code incomingOperationType}.
   */
  public void recordIncomingOperation(IncomingOperationType incomingOperationType) {
    incrementValue(incomingOperationTypes, incomingOperationType);
  }

  /** Records the fact that the listener has issued an event of type {@code listenerEventType}. */
  public void recordListenerEvent(ListenerEventType listenerEventType) {
    incrementValue(listenerEventTypes, listenerEventType);
  }

  /** Records the fact that the client has observed an error of type {@code clientErrorType}. */
  public void recordError(ClientErrorType clientErrorType) {
    incrementValue(clientErrorTypes, clientErrorType);
  }

  /**
   * Modifies {@code performanceCounters} to contain all the statistics that are non-zero. Each pair
   * has the name of the statistic event and the number of times that event has occurred since the
   * client started.
   */
  public void getNonZeroStatistics(List<SimplePair<String, Integer>> performanceCounters) {
    // Add the non-zero values from the different maps to performanceCounters.
    fillWithNonZeroStatistics(sentMessageTypes, performanceCounters, SENT_MESSAGE_TYPE_NAME);
    fillWithNonZeroStatistics(receivedMessageTypes, performanceCounters,
        RECEIVED_MESSAGE_TYPE_NAME);
    fillWithNonZeroStatistics(incomingOperationTypes, performanceCounters,
        INCOMING_OPERATION_TYPE_NAME);
    fillWithNonZeroStatistics(listenerEventTypes, performanceCounters, LISTENER_EVENT_TYPE_NAME);
    fillWithNonZeroStatistics(clientErrorTypes, performanceCounters, CLIENT_ERROR_TYPE_NAME);
  }

  /** Modifies {@code result} to contain those statistics from {@code map} whose value is > 0. */
  private static <Key extends Enum<Key>> void fillWithNonZeroStatistics(Map<Key, Integer> map,
      List<SimplePair<String, Integer>> destination, String typeName) {
    String prefix = typeName + ".";
    for (Map.Entry<Key, Integer> entry : map.entrySet()) {
      if (entry.getValue() > 0) {
        destination.add(SimplePair.of(prefix + entry.getKey().name(), entry.getValue()));
      }
    }
  }

  /** Initializes a map from enum names to values of the given {@code keys}. */
  private static <Key extends Enum<Key>> Map<String, Key> createValueOfMap(Key[] keys) {
    HashMap<String, Key> map = new HashMap<String, Key>();
    for (Key key : keys) {
      map.put(key.name(), key);
    }
    return map;
  }

  /** Increments the value of {@code map}[{@code key}] by 1. */
  private static <Key> void incrementValue(Map<Key, Integer> map, Key key) {
    map.put(key, TypedUtil.mapGet(map, key) + 1);
  }

  /** Initializes all values for {@code keys} in {@code map} to be 0. */
  private static <Key> void initializeMap(Map<Key, Integer> map, Key[] keys) {
    for (Key key : keys) {
      map.put(key, 0);
    }
  }

  @Override
  public void toCompactString(TextBuilder builder) {
    List<SimplePair<String, Integer>> nonZeroValues = new ArrayList<SimplePair<String, Integer>>();
    getNonZeroStatistics(nonZeroValues);
    builder.appendFormat("Client Statistics: %s\n", nonZeroValues);
  }

  @Override
  public StatisticsState marshal() {
    // Get all the non-zero counters, convert them to proto PropertyRecord messages, and return
    // a StatisticsState containing the records.
    List<SimplePair<String, Integer>> counters = new ArrayList<SimplePair<String, Integer>>();
    getNonZeroStatistics(counters);
    List<PropertyRecord> propertyRecords = new ArrayList<PropertyRecord>(counters.size());
    for (SimplePair<String, Integer> counter : counters) {
      propertyRecords.add(PropertyRecord.create(counter.getFirst(), counter.getSecond()));
    }
    return StatisticsState.create(propertyRecords);
  }

  /**
   * Given the serialized {@code performanceCounters} of the client statistics, returns a Statistics
   * object with the performance counter values from {@code performanceCounters}.
   */
  
  public static Statistics deserializeStatistics(Logger logger,
      Collection<PropertyRecord> performanceCounters) {
    Statistics statistics = new Statistics();

    // For each counter, parse out the counter name and value.
    for (PropertyRecord performanceCounter : performanceCounters) {
      String counterName = performanceCounter.getName();
      String[] parts = counterName.split("\\.");
      if (parts.length != 2) {
        logger.warning("Perf counter name must of form: class.value, skipping: %s", counterName);
        continue;
      }
      String className = parts[0];
      String fieldName = parts[1];
      int counterValue = performanceCounter.getValue();

      // Call the relevant method in a loop (i.e., depending on the type of the class).
      if (TypedUtil.<String>equals(className, SENT_MESSAGE_TYPE_NAME)) {
        incrementPerformanceCounterValue(logger, SENT_MESSAGE_TYPE_NAME_TO_VALUE_MAP,
            statistics.sentMessageTypes, fieldName, counterValue);
      } else if (TypedUtil.<String>equals(className, INCOMING_OPERATION_TYPE_NAME)) {
        incrementPerformanceCounterValue(logger, INCOMING_OPERATION_TYPE_NAME_TO_VALUE_MAP,
            statistics.incomingOperationTypes, fieldName, counterValue);
      } else if (TypedUtil.<String>equals(className, RECEIVED_MESSAGE_TYPE_NAME)) {
        incrementPerformanceCounterValue(logger, RECEIVED_MESSAGE_TYPE_NAME_TO_VALUE_MAP,
            statistics.receivedMessageTypes, fieldName, counterValue);
      } else if (TypedUtil.<String>equals(className,  LISTENER_EVENT_TYPE_NAME)) {
        incrementPerformanceCounterValue(logger, LISTENER_EVENT_TYPE_NAME_TO_VALUE_MAP,
            statistics.listenerEventTypes, fieldName, counterValue);
      } else if (TypedUtil.<String>equals(className,  CLIENT_ERROR_TYPE_NAME)) {
        incrementPerformanceCounterValue(logger, CLIENT_ERROR_TYPE_NAME_TO_VALUE_MAP,
            statistics.clientErrorTypes, fieldName, counterValue);
      } else {
        logger.warning("Skipping unknown enum class name %s", className);
      }
    }
    return statistics;
  }

  /**
   * Looks for an enum value with the given {@code fieldName} in {@code valueOfMap} and increments
   * the corresponding entry in {@code counts} by {@code counterValue}. Call to update statistics
   * for a single performance counter.
   */
  private static <Key extends Enum<Key>> void incrementPerformanceCounterValue(Logger logger,
      Map<String, Key> valueOfMap, Map<Key, Integer> counts, String fieldName, int counterValue) {
    Key type = TypedUtil.mapGet(valueOfMap, fieldName);
    if (type != null) {
      int currentValue = TypedUtil.mapGet(counts, type);
      counts.put(type, currentValue + counterValue);
    } else {
      logger.warning("Skipping unknown enum value name %s", fieldName);
    }
  }
}
