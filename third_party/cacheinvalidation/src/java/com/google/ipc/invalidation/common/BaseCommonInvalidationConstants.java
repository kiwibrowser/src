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
package com.google.ipc.invalidation.common;

/** Various constants common to  clients and servers used in version 2 of the Ticl. */
public class BaseCommonInvalidationConstants {
  /** Major version of the client library. */
  public static final int CLIENT_MAJOR_VERSION = 3;

  /**
   * Minor version of the client library, defined to be equal to the datestamp of the build
   * (e.g. 20130401).
   */
  public static final int CLIENT_MINOR_VERSION = BuildConstants.BUILD_DATESTAMP;

  /** Major version of the protocol between the client and the server. */
  public static final int PROTOCOL_MAJOR_VERSION = 3;

  /** Minor version of the protocol between the client and the server. */
  public static final int PROTOCOL_MINOR_VERSION = 2;

  /** Major version of the client config. */
  public static final int CONFIG_MAJOR_VERSION = 3;

  /** Minor version of the client config. */
  public static final int CONFIG_MINOR_VERSION = 2;
}
