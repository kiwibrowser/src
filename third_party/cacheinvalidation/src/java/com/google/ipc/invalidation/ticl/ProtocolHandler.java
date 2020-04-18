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
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.SystemResources.NetworkChannel;
import com.google.ipc.invalidation.external.client.SystemResources.Scheduler;
import com.google.ipc.invalidation.external.client.types.SimplePair;
import com.google.ipc.invalidation.ticl.InvalidationClientCore.BatchingTask;
import com.google.ipc.invalidation.ticl.Statistics.ClientErrorType;
import com.google.ipc.invalidation.ticl.Statistics.ReceivedMessageType;
import com.google.ipc.invalidation.ticl.Statistics.SentMessageType;
import com.google.ipc.invalidation.ticl.proto.ClientConstants;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ApplicationClientIdP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientConfigP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientHeader;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientToServerMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientVersion;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ConfigChangeMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ErrorMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InfoMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InfoRequestMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InitializeMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InitializeMessage.DigestSerializationType;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.PropertyRecord;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ProtocolHandlerConfigP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RateLimitP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationP.OpType;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationStatusMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationSubtree;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationSummary;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationSyncMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationSyncRequestMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ServerHeader;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ServerToClientMessage;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.TokenControlMessage;
import com.google.ipc.invalidation.ticl.proto.CommonProtos;
import com.google.ipc.invalidation.ticl.proto.JavaClient.BatcherState;
import com.google.ipc.invalidation.ticl.proto.JavaClient.ProtocolHandlerState;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.InternalBase;
import com.google.ipc.invalidation.util.Marshallable;
import com.google.ipc.invalidation.util.Preconditions;
import com.google.ipc.invalidation.util.ProtoWrapper;
import com.google.ipc.invalidation.util.ProtoWrapper.ValidationException;
import com.google.ipc.invalidation.util.Smearer;
import com.google.ipc.invalidation.util.TextBuilder;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;


/**
 * A layer for interacting with low-level protocol messages.  Parses messages from the server and
 * calls appropriate functions on the {@code ProtocolListener} to handle various types of message
 * content.  Also buffers message data from the client and constructs and sends messages to the
 * server.
 * <p>
 * This class implements {@link Marshallable}, so its state can be written to a protocol buffer,
 * and instances can be restored from such protocol buffers. Additionally, the nested class
 * {@link Batcher} also implements {@code Marshallable} for the same reason.
 * <p>
 * Note that while we talk about "marshalling," in this context we mean marshalling to protocol
 * buffers, not raw bytes.
 *
 */
class ProtocolHandler implements Marshallable<ProtocolHandlerState> {
  /** Class that batches messages to the server. */
  private static class Batcher implements Marshallable<BatcherState> {
    /** Statistics to be updated when messages are created. */
    private final Statistics statistics;

    /** Resources used for logging and thread assertions. */
    private final SystemResources resources;

    /** Set of pending registrations stored as a map for overriding later operations. */
    private final Map<ObjectIdP, Integer> pendingRegistrations = new HashMap<ObjectIdP, Integer>();

    /** Set of pending invalidation acks. */
    private final Set<InvalidationP> pendingAckedInvalidations = new HashSet<InvalidationP>();

    /** Set of pending registration sub trees for registration sync. */
    private final Set<RegistrationSubtree> pendingRegSubtrees = new HashSet<RegistrationSubtree>();

    /** Pending initialization message to send to the server, if any. */
    private InitializeMessage pendingInitializeMessage = null;

    /** Pending info message to send to the server, if any. */
    private InfoMessage pendingInfoMessage = null;

    /** Creates a batcher. */
    Batcher(SystemResources resources, Statistics statistics) {
      this.resources = resources;
      this.statistics = statistics;
    }

    /** Creates a batcher from {@code marshalledState}. */
    Batcher(SystemResources resources, Statistics statistics, BatcherState marshalledState) {
      this(resources, statistics);
      for (ObjectIdP registration : marshalledState.getRegistration()) {
        pendingRegistrations.put(registration, RegistrationP.OpType.REGISTER);
      }
      for (ObjectIdP unregistration : marshalledState.getUnregistration()) {
        pendingRegistrations.put(unregistration, RegistrationP.OpType.UNREGISTER);
      }
      for (InvalidationP ack : marshalledState.getAcknowledgement()) {
        pendingAckedInvalidations.add(ack);
      }
      for (RegistrationSubtree subtree : marshalledState.getRegistrationSubtree()) {
        pendingRegSubtrees.add(subtree);
      }
      pendingInitializeMessage = marshalledState.getNullableInitializeMessage();
      if (marshalledState.hasInfoMessage()) {
        pendingInfoMessage = marshalledState.getInfoMessage();
      }
    }

    /** Sets the initialize message to be sent. */
    void setInitializeMessage(InitializeMessage msg) {
      pendingInitializeMessage = msg;
    }

    /** Sets the info message to be sent. */
    void setInfoMessage(InfoMessage msg) {
      pendingInfoMessage = msg;
    }

    /** Adds a registration on {@code oid} of {@code opType} to the registrations to be sent. */
    void addRegistration(ObjectIdP oid, Integer opType) {
      pendingRegistrations.put(oid, opType);
    }

    /** Adds {@code ack} to the set of acknowledgements to be sent. */
    void addAck(InvalidationP ack) {
      pendingAckedInvalidations.add(ack);
    }

    /** Adds {@code subtree} to the set of registration subtrees to be sent. */
    void addRegSubtree(RegistrationSubtree subtree) {
      pendingRegSubtrees.add(subtree);
    }

    /**
     * Returns a builder for a {@link ClientToServerMessage} to be sent to the server. Crucially,
     * the builder does <b>NOT</b> include the message header.
     * @param hasClientToken whether the client currently holds a token
     */
    ClientToServerMessage toMessage(final ClientHeader header, boolean hasClientToken) {
      final InitializeMessage initializeMessage;
      final RegistrationMessage registrationMessage;
      final RegistrationSyncMessage registrationSyncMessage;
      final InvalidationMessage invalidationAckMessage;
      final InfoMessage infoMessage;

      if (pendingInitializeMessage != null) {
        statistics.recordSentMessage(SentMessageType.INITIALIZE);
        initializeMessage = pendingInitializeMessage;
        pendingInitializeMessage = null;
      } else {
        initializeMessage = null;
      }

      // Note: Even if an initialize message is being sent, we can send additional
      // messages such as regisration messages, etc to the server. But if there is no token
      // and an initialize message is not being sent, we cannot send any other message.

      if (!hasClientToken && (initializeMessage == null)) {
        // Cannot send any message
        resources.getLogger().warning(
            "Cannot send message since no token and no initialize msg");
        statistics.recordError(ClientErrorType.TOKEN_MISSING_FAILURE);
        return null;
      }

      // Check for pending batched operations and add to message builder if needed.

      // Add reg, acks, reg subtrees - clear them after adding.
      if (!pendingAckedInvalidations.isEmpty()) {
        invalidationAckMessage = createInvalidationAckMessage();
        statistics.recordSentMessage(SentMessageType.INVALIDATION_ACK);
      } else {
        invalidationAckMessage = null;
      }

      // Check regs.
      if (!pendingRegistrations.isEmpty()) {
        registrationMessage = createRegistrationMessage();
        statistics.recordSentMessage(SentMessageType.REGISTRATION);
      } else {
        registrationMessage = null;
      }

      // Check reg substrees.
      if (!pendingRegSubtrees.isEmpty()) {
        // If there are multiple pending reg subtrees, only one is sent.
        ArrayList<RegistrationSubtree> regSubtrees = new ArrayList<RegistrationSubtree>(1);
        regSubtrees.add(pendingRegSubtrees.iterator().next());
        registrationSyncMessage = RegistrationSyncMessage.create(regSubtrees);
        pendingRegSubtrees.clear();
        statistics.recordSentMessage(SentMessageType.REGISTRATION_SYNC);
      } else {
        registrationSyncMessage = null;
      }

      // Check if an info message has to be sent.
      if (pendingInfoMessage != null) {
        statistics.recordSentMessage(SentMessageType.INFO);
        infoMessage = pendingInfoMessage;
        pendingInfoMessage = null;
      } else {
        infoMessage = null;
      }

      return ClientToServerMessage.create(header, initializeMessage, registrationMessage,
          registrationSyncMessage, invalidationAckMessage, infoMessage);
    }

    /**
     * Creates a registration message based on registrations from {@code pendingRegistrations}
     * and returns it.
     * <p>
     * REQUIRES: pendingRegistrations.size() > 0
     */
    private RegistrationMessage createRegistrationMessage() {
      Preconditions.checkState(!pendingRegistrations.isEmpty());

      // Run through the pendingRegistrations map.
      List<RegistrationP> pendingRegistrations =
          new ArrayList<RegistrationP>(this.pendingRegistrations.size());
      for (Map.Entry<ObjectIdP, Integer> entry : this.pendingRegistrations.entrySet()) {
        pendingRegistrations.add(RegistrationP.create(entry.getKey(), entry.getValue()));
      }
      this.pendingRegistrations.clear();
      return RegistrationMessage.create(pendingRegistrations);
    }

    /**
     * Creates an invalidation ack message based on acks from {@code pendingAckedInvalidations} and
     * returns it.
     * <p>
     * REQUIRES: pendingAckedInvalidations.size() > 0
     */
    private InvalidationMessage createInvalidationAckMessage() {
      Preconditions.checkState(!pendingAckedInvalidations.isEmpty());
      InvalidationMessage ackMessage =
          InvalidationMessage.create(new ArrayList<InvalidationP>(pendingAckedInvalidations));
      pendingAckedInvalidations.clear();
      return ackMessage;
    }

    @Override
    public BatcherState marshal() {
      // Marshall (un)registrations.
      ArrayList<ObjectIdP> registrations = new ArrayList<ObjectIdP>(pendingRegistrations.size());
      ArrayList<ObjectIdP> unregistrations = new ArrayList<ObjectIdP>(pendingRegistrations.size());
      for (Map.Entry<ObjectIdP, Integer> entry : pendingRegistrations.entrySet()) {
        Integer opType = entry.getValue();
        ObjectIdP oid = entry.getKey();
            new ArrayList<ObjectIdP>(pendingRegistrations.size());
        switch (opType) {
          case OpType.REGISTER:
            registrations.add(oid);
            break;
          case OpType.UNREGISTER:
            unregistrations.add(oid);
            break;
          default:
            throw new IllegalArgumentException(opType.toString());
        }
      }
      return BatcherState.create(registrations, unregistrations, pendingAckedInvalidations,
          pendingRegSubtrees, pendingInitializeMessage, pendingInfoMessage);
    }
  }

  /** Representation of a message header for use in a server message. */
  static class ServerMessageHeader extends InternalBase {
    /**
     * Constructs an instance.
     *
     * @param token server-sent token
     * @param registrationSummary summary over server registration state
     */
    ServerMessageHeader(Bytes token, RegistrationSummary registrationSummary) {
      this.token = token;
      this.registrationSummary = registrationSummary;
    }

    /** Server-sent token. */
    Bytes token;

    /** Summary of the client's registration state at the server. */
    RegistrationSummary registrationSummary;

    @Override
    public void toCompactString(TextBuilder builder) {
      builder.appendFormat("Token: %s, Summary: %s", token, registrationSummary);
    }
  }

  /**
   * Representation of a message receiver for the server. Such a message is guaranteed to be
   * valid, but the session token is <b>not</b> checked.
   */
  static class ParsedMessage {
    /*
     * Each of these fields corresponds directly to a field in the ServerToClientMessage protobuf.
     * It is non-null iff the corresponding hasYYY method in the protobuf would return true.
     */
    final ServerMessageHeader header;
    final TokenControlMessage tokenControlMessage;
    final InvalidationMessage invalidationMessage;
    final RegistrationStatusMessage registrationStatusMessage;
    final RegistrationSyncRequestMessage registrationSyncRequestMessage;
    final ConfigChangeMessage configChangeMessage;
    final InfoRequestMessage infoRequestMessage;
    final ErrorMessage errorMessage;

    /** Constructs an instance from a {@code rawMessage}. */
    ParsedMessage(ServerToClientMessage rawMessage) {
      // For each field, assign it to the corresponding protobuf field if present, else null.
      ServerHeader messageHeader = rawMessage.getHeader();
      header = new ServerMessageHeader(messageHeader.getClientToken(),
          messageHeader.getNullableRegistrationSummary());
      tokenControlMessage =
          rawMessage.hasTokenControlMessage() ? rawMessage.getTokenControlMessage() : null;
      invalidationMessage = rawMessage.getNullableInvalidationMessage();
      registrationStatusMessage = rawMessage.getNullableRegistrationStatusMessage();
      registrationSyncRequestMessage = rawMessage.hasRegistrationSyncRequestMessage()
          ? rawMessage.getRegistrationSyncRequestMessage() : null;
      configChangeMessage =
          rawMessage.hasConfigChangeMessage() ? rawMessage.getConfigChangeMessage() : null;
      infoRequestMessage = rawMessage.getNullableInfoRequestMessage();
      errorMessage = rawMessage.getNullableErrorMessage();
    }
  }

  /**
   * Listener for protocol events. The handler guarantees that the call will be made on the internal
   * thread that the SystemResources provides.
   */
  interface ProtocolListener {
    /** Records that a message was sent to the server at the current time. */
    void handleMessageSent();

    /** Returns a summary of the current desired registrations. */
    RegistrationSummary getRegistrationSummary();

    /** Returns the current server-assigned client token, if any. */
    Bytes getClientToken();
  }

  /** Information about the client, e.g., application name, OS, etc. */
  private final ClientVersion clientVersion;

  /** A logger. */
  private final Logger logger;

  /** Scheduler for the client's internal processing. */
  private final Scheduler internalScheduler;

  /** Network channel for sending and receiving messages to and from the server. */
  private final NetworkChannel network;

  /** The protocol listener. */
  private final ProtocolListener listener;

  /** Batches messages to the server. */
  private final Batcher batcher;

  /** A debug message id that is added to every message to the server. */
  private int messageId = 1;

  // State specific to a client. If we want to support multiple clients, this could
  // be in a map or could be eliminated (e.g., no batching).

  /** The last known time from the server. */
  private long lastKnownServerTimeMs = 0;

  /**
   * The next time before which a message cannot be sent to the server. If this is less than current
   * time, a message can be sent at any time.
   */
  private long nextMessageSendTimeMs = 0;

  /** Statistics objects to track number of sent messages, etc. */
  private final Statistics statistics;

  /** Client type for inclusion in headers. */
  private final int clientType;

  /**
   * Creates an instance.
   *
   * @param config configuration for the client
   * @param resources resources to use
   * @param smearer a smearer to randomize delays
   * @param statistics track information about messages sent/received, etc
   * @param applicationName name of the application using the library (for debugging/monitoring)
   * @param listener callback for protocol events
   */
  ProtocolHandler(ProtocolHandlerConfigP config, final SystemResources resources,
      Smearer smearer, Statistics statistics, int clientType, String applicationName,
      ProtocolListener listener, ProtocolHandlerState marshalledState) {
    this.logger = resources.getLogger();
    this.statistics = statistics;
    this.internalScheduler = resources.getInternalScheduler();
    this.network = resources.getNetwork();
    this.listener = listener;
    this.clientVersion = CommonProtos.newClientVersion(resources.getPlatform(), "Java",
        applicationName);
    this.clientType = clientType;
    if (marshalledState == null) {
      // If there is no marshalled state, construct a clean batcher.
      this.batcher = new Batcher(resources, statistics);
    } else {
      // Otherwise, restore the batcher from the marshalled state.
      this.batcher = new Batcher(resources, statistics, marshalledState.getBatcherState());
      this.messageId = marshalledState.getMessageId();
      this.lastKnownServerTimeMs = marshalledState.getLastKnownServerTimeMs();
      this.nextMessageSendTimeMs = marshalledState.getNextMessageSendTimeMs();
    }
    logger.info("Created protocol handler for application %s, platform %s", applicationName,
        resources.getPlatform());
  }

  /** Returns a default config for the protocol handler. */
  static ProtocolHandlerConfigP createConfig() {
    // Allow at most 3 messages every 5 seconds.
    int windowMs = 5 * 1000;
    int numMessagesPerWindow = 3;

    List<RateLimitP> rateLimits = new ArrayList<RateLimitP>();
    rateLimits.add(RateLimitP.create(windowMs, numMessagesPerWindow));
    return ProtocolHandlerConfigP.create(null, rateLimits);
  }

  /** Returns a configuration object with parameters set for unit tests. */
  static ProtocolHandlerConfigP createConfigForTest() {
    // No rate limits
    int smallBatchDelayForTest = 200;
    return ProtocolHandlerConfigP.create(smallBatchDelayForTest, new ArrayList<RateLimitP>(0));
  }

  /**
   * Returns the next time a message is allowed to be sent to the server.  Typically, this will be
   * in the past, meaning that the client is free to send a message at any time.
   */
  public long getNextMessageSendTimeMsForTest() {
    return nextMessageSendTimeMs;
  }

  /**
   * Handles a message from the server. If the message can be processed (i.e., is valid, is
   * of the right version, and is not a silence message), returns a {@link ParsedMessage}
   * representing it. Otherwise, returns {@code null}.
   * <p>
   * This class intercepts and processes silence messages. In this case, it will discard any other
   * data in the message.
   * <p>
   * Note that this method does <b>not</b> check the session token of any message.
   */
  ParsedMessage handleIncomingMessage(byte[] incomingMessage) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    ServerToClientMessage message;
    try {
      message = ServerToClientMessage.parseFrom(incomingMessage);
    } catch (ValidationException exception) {
      statistics.recordError(ClientErrorType.INCOMING_MESSAGE_FAILURE);
      logger.warning("Incoming message is invalid: %s", Bytes.toLazyCompactString(incomingMessage));
      return null;
    }

    // Check the version of the message.
    if (message.getHeader().getProtocolVersion().getVersion().getMajorVersion() !=
        ClientConstants.PROTOCOL_MAJOR_VERSION) {
      statistics.recordError(ClientErrorType.PROTOCOL_VERSION_FAILURE);
      logger.severe("Dropping message with incompatible version: %s", message);
      return null;
    }

    // Check if it is a ConfigChangeMessage which indicates that messages should no longer be
    // sent for a certain duration. Perform this check before the token is even checked.
    if (message.hasConfigChangeMessage()) {
      ConfigChangeMessage configChangeMsg = message.getConfigChangeMessage();
      statistics.recordReceivedMessage(ReceivedMessageType.CONFIG_CHANGE);
      if (configChangeMsg.hasNextMessageDelayMs()) {  // Validator has ensured that it is positive.
        nextMessageSendTimeMs =
            internalScheduler.getCurrentTimeMs() + configChangeMsg.getNextMessageDelayMs();
      }
      return null;  // Ignore all other messages in the envelope.
    }

    lastKnownServerTimeMs = Math.max(lastKnownServerTimeMs, message.getHeader().getServerTimeMs());
    return new ParsedMessage(message);
  }

  /**
   * Sends a message to the server to request a client token.
   *
   * @param applicationClientId application-specific client id
   * @param nonce nonce for the request
   * @param debugString information to identify the caller
   */
  void sendInitializeMessage(ApplicationClientIdP applicationClientId, Bytes nonce,
      BatchingTask batchingTask, String debugString) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    if (applicationClientId.getClientType() != clientType) {
      // This condition is not fatal, but it probably represents a bug somewhere if it occurs.
      logger.warning(
          "Client type in application id does not match constructor-provided type: %s vs %s",
          applicationClientId, clientType);
    }

    // Simply store the message in pendingInitializeMessage and send it when the batching task runs.
    InitializeMessage initializeMsg = InitializeMessage.create(clientType, nonce,
        applicationClientId, DigestSerializationType.BYTE_BASED);
    batcher.setInitializeMessage(initializeMsg);
    logger.info("Batching initialize message for client: %s, %s", debugString, initializeMsg);
    batchingTask.ensureScheduled(debugString);
  }

  /**
   * Sends an info message to the server with the performance counters supplied
   * in {@code performanceCounters} and the config supplies in
   * {@code configParams}.
   *
   * @param requestServerRegistrationSummary indicates whether to request the
   *        server's registration summary
   */
  void sendInfoMessage(List<SimplePair<String, Integer>> performanceCounters,
      ClientConfigP clientConfig, boolean requestServerRegistrationSummary,
      BatchingTask batchingTask) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");

    List<PropertyRecord> performanceCounterRecords =
        new ArrayList<PropertyRecord>(performanceCounters.size());
    for (SimplePair<String, Integer> counter : performanceCounters) {
      performanceCounterRecords.add(PropertyRecord.create(counter.first, counter.second));
    }
    InfoMessage infoMessage = InfoMessage.create(clientVersion, /* configParameter */ null,
        performanceCounterRecords, requestServerRegistrationSummary, clientConfig);

    // Simply store the message in pendingInfoMessage and send it when the batching task runs.
    batcher.setInfoMessage(infoMessage);
    batchingTask.ensureScheduled("Send-info");
  }

  /**
   * Sends a registration request to the server.
   *
   * @param objectIds object ids on which to (un)register
   * @param regOpType whether to register or unregister
   */
  void sendRegistrations(Collection<ObjectIdP> objectIds, Integer regOpType,
      BatchingTask batchingTask) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    for (ObjectIdP objectId : objectIds) {
      batcher.addRegistration(objectId, regOpType);
    }
    batchingTask.ensureScheduled("Send-registrations");
  }

  /** Sends an acknowledgement for {@code invalidation} to the server. */
  void sendInvalidationAck(InvalidationP invalidation, BatchingTask batchingTask) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    // We could summarize acks when there are suppressing invalidations - we don't since it is
    // unlikely to be too beneficial here.
    logger.fine("Sending ack for invalidation %s", invalidation);
    batcher.addAck(invalidation);
    batchingTask.ensureScheduled("Send-Ack");
  }

  /**
   * Sends a single registration subtree to the server.
   *
   * @param regSubtree subtree to send
   */
  void sendRegistrationSyncSubtree(RegistrationSubtree regSubtree, BatchingTask batchingTask) {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    batcher.addRegSubtree(regSubtree);
    logger.info("Adding subtree: %s", regSubtree);
    batchingTask.ensureScheduled("Send-reg-sync");
  }

  /** Sends pending data to the server (e.g., registrations, acks, registration sync messages). */
  void sendMessageToServer() {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    if (nextMessageSendTimeMs > internalScheduler.getCurrentTimeMs()) {
      logger.warning("In quiet period: not sending message to server: %s > %s",
          nextMessageSendTimeMs, internalScheduler.getCurrentTimeMs());
      return;
    }

    // Create the message from the batcher.
    ClientToServerMessage message;
    try {
      message = batcher.toMessage(createClientHeader(), listener.getClientToken() != null);
      if (message == null) {
        // Happens when we don't have a token and are not sending an initialize message. Logged
        // in batcher.toMessage().
        return;
      }
    } catch (ProtoWrapper.ValidationArgumentException exception) {
      logger.severe("Tried to send invalid message: %s", batcher);
      statistics.recordError(ClientErrorType.OUTGOING_MESSAGE_FAILURE);
      return;
    }
    ++messageId;

    statistics.recordSentMessage(SentMessageType.TOTAL);
    logger.fine("Sending message to server: %s", message);
    network.sendMessage(message.toByteArray());

    // Record that the message was sent. We're invoking the listener directly, rather than
    // scheduling a new work unit to do it. It would be safer to do a schedule, but that's hard to
    // do in Android, we wrote this listener (it's InvalidationClientCore, so we know what it does),
    // and it's the last line of this function.
    listener.handleMessageSent();
  }

  /** Returns the header to include on a message to the server. */
  private ClientHeader createClientHeader() {
    Preconditions.checkState(internalScheduler.isRunningOnThread(), "Not on internal thread");
    return ClientHeader.create(ClientConstants.PROTOCOL_VERSION,
        listener.getClientToken(), listener.getRegistrationSummary(),
        internalScheduler.getCurrentTimeMs(),  lastKnownServerTimeMs, Integer.toString(messageId),
        clientType);
  }

  @Override
  public ProtocolHandlerState marshal() {
    return ProtocolHandlerState.create(messageId, lastKnownServerTimeMs, nextMessageSendTimeMs,
        batcher.marshal());
  }
}
