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

import com.google.ipc.invalidation.common.BaseCommonInvalidationConstants;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ObjectIdP;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.ProtocolVersion;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.Version;
import com.google.ipc.invalidation.util.Bytes;

/** Various constant protobufs used in version 2 of the Ticl. */
public class ClientConstants extends BaseCommonInvalidationConstants {

  /** Version of the client currently being used by the client. */
  public static final Version CLIENT_VERSION_VALUE;

  /** Version of the protocol currently being used by the client/server for v2 clients. */
  public static final ProtocolVersion PROTOCOL_VERSION;

  /** Version of the protocol currently being used by the client/server for v1 clients. */
  public static final ProtocolVersion PROTOCOL_VERSION_V1;

  /** The value of ObjectSource.Type from types.proto. Must be kept in sync with that file. */
  public static final int INTERNAL_OBJECT_SOURCE_TYPE;

  /** Object id used to trigger a refresh of all cached objects ("invalidate-all"). */
  public static final ObjectIdP ALL_OBJECT_ID;

  static {
    CLIENT_VERSION_VALUE = Version.create(CLIENT_MAJOR_VERSION, CLIENT_MINOR_VERSION);
    PROTOCOL_VERSION =
        ProtocolVersion.create(Version.create(PROTOCOL_MAJOR_VERSION, PROTOCOL_MINOR_VERSION));
    PROTOCOL_VERSION_V1 =
        ProtocolVersion.create(Version.create(2, 0));
    INTERNAL_OBJECT_SOURCE_TYPE = 1;
    ALL_OBJECT_ID = ObjectIdP.create(INTERNAL_OBJECT_SOURCE_TYPE, Bytes.EMPTY_BYTES);
  }

  // Prevent instantiation.
  private ClientConstants() {
  }
}
