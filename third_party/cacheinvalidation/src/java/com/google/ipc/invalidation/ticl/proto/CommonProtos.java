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
package com.google.ipc.invalidation.ticl.proto;

import com.google.ipc.invalidation.ticl.proto.AndroidChannel.AndroidEndpointId;
import com.google.ipc.invalidation.ticl.proto.ChannelCommon.NetworkEndpointId;
import com.google.ipc.invalidation.ticl.proto.ChannelCommon.NetworkEndpointId.NetworkAddress;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ClientVersion;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.InvalidationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationStatus;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.RegistrationSummary;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ServerHeader;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.StatusP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.Version;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.Preconditions;


/** Utilities for creating protocol buffer wrappers. */
public class CommonProtos {

  public static boolean isAllObjectId(ObjectIdP objectId) {
    return ClientConstants.ALL_OBJECT_ID.equals(objectId);
  }

  /** Returns true iff status corresponds to permanent failure. */
  public static boolean isPermanentFailure(StatusP status) {
    return status.getCode() == StatusP.Code.PERMANENT_FAILURE;
  }

  /** Returns true iff status corresponds to success. */
  public static boolean isSuccess(StatusP status) {
    return status.getCode() == StatusP.Code.SUCCESS;
  }

  /** Returns true iff status corresponds to transient failure. */
  public static boolean isTransientFailure(StatusP status) {
    return status.getCode() == StatusP.Code.TRANSIENT_FAILURE;
  }

  /**
   * Constructs a network endpoint id for an Android client with the given {@code registrationId},
   * {@code clientKey}, and {@code packageName}.
   */
  public static NetworkEndpointId newAndroidEndpointId(String registrationId, String clientKey,
      String packageName, Version channelVersion) {
    Preconditions.checkNotNull(registrationId, "Null registration id");
    Preconditions.checkNotNull(clientKey, "Null client key");
    Preconditions.checkNotNull(packageName, "Null package name");
    Preconditions.checkNotNull(channelVersion, "Null channel version");

    AndroidEndpointId endpoint = AndroidEndpointId.create(registrationId, clientKey,
        /* senderId */ null, channelVersion, packageName);
    return NetworkEndpointId.create(NetworkAddress.ANDROID, new Bytes(endpoint.toByteArray()),
        null);
  }

  public static ClientVersion newClientVersion(String platform, String language,
      String applicationInfo) {
    return ClientVersion.create(ClientConstants.CLIENT_VERSION_VALUE, platform, language,
        applicationInfo);
  }

  public static StatusP newFailureStatus(boolean isTransient, String description) {
    return StatusP.create(
        isTransient ? StatusP.Code.TRANSIENT_FAILURE : StatusP.Code.PERMANENT_FAILURE, description);
  }


  public static InvalidationP newInvalidationP(ObjectIdP objectId, long version,
      boolean isTrickleRestart, byte[] payload) {
    return InvalidationP.create(objectId, /* isKnownVersion */ true,
        version, Bytes.fromByteArray(payload), isTrickleRestart);
  }

  public static InvalidationP newInvalidationPForUnknownVersion(ObjectIdP oid,
      long sequenceNumber) {
    return InvalidationP.create(oid, /* isKnownVersion */ false, sequenceNumber, /* payload */ null,
        /* isTrickleRestart */ true);
  }

  public static RegistrationP newRegistrationP(ObjectIdP oid, boolean isReg) {
    return RegistrationP.create(oid,
        isReg ? RegistrationP.OpType.REGISTER : RegistrationP.OpType.UNREGISTER);
  }

  public static ServerHeader newServerHeader(byte[] clientToken, long currentTimeMs,
      RegistrationSummary registrationSummary, String messageId) {
    return ServerHeader.create(ClientConstants.PROTOCOL_VERSION, new Bytes(clientToken),
        registrationSummary, currentTimeMs, messageId);
  }

  public static StatusP newSuccessStatus() {
    return StatusP.create(StatusP.Code.SUCCESS, null);
  }

  public static RegistrationStatus newTransientFailureRegistrationStatus(RegistrationP registration,
      String description) {
    return RegistrationStatus.create(registration, newFailureStatus(true, description));
  }

  // Prevent instantiation.
  private CommonProtos() {}
}
