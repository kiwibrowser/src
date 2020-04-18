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

/**
 * Shared client server constants for the GCM Channel.
 *
 * <p>On the client side a data bundle is created and sent using gcm.send(). At the server the same
 * key value pairs are received in the DataMessageProto added to the {@link DataMessageRequest}.
 *
 */
public class GcmSharedConstants {

  /**
   * The key used for the {@link ClientToServerMessage} added to the data bundle.
   */
  public static final String CLIENT_TO_SERVER_MESSAGE_KEY = "client_to_server_message";

  /**
   * The key used for the {@link NetworkEndpointId} added to the data bundle.
   */
  public static final String NETWORK_ENDPOINT_ID_KEY = "network_endpoint_id";

  /**
   * Value of the client key set in the android endpoint id when sending messages using updated GCM.
   */
  public static final String ANDROID_ENDPOINT_ID_CLIENT_KEY = "ANDROID_GCM_UPDATED";

  /**
   * The sender id used when sending upstream messages using GCM.
   */
  public static final String GCM_UPDATED_SENDER_ID = "548642380543";
}
