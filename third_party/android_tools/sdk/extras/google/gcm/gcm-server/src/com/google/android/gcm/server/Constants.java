/*
 * Copyright 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.gcm.server;

/**
 * Constants used on GCM service communication.
 */
public final class Constants {

  /**
   * Endpoint for sending messages.
   */
  public static final String GCM_SEND_ENDPOINT =
      "https://android.googleapis.com/gcm/send";

  /**
   * HTTP parameter for registration id.
   */
  public static final String PARAM_REGISTRATION_ID = "registration_id";

  /**
   * HTTP parameter for collapse key.
   */
  public static final String PARAM_COLLAPSE_KEY = "collapse_key";

  /**
   * HTTP parameter for delaying the message delivery if the device is idle.
   */
  public static final String PARAM_DELAY_WHILE_IDLE = "delay_while_idle";

  /**
   * Prefix to HTTP parameter used to pass key-values in the message payload.
   */
  public static final String PARAM_PAYLOAD_PREFIX = "data.";

  /**
   * Prefix to HTTP parameter used to set the message time-to-live.
   */
  public static final String PARAM_TIME_TO_LIVE = "time_to_live";

  /**
   * Too many messages sent by the sender. Retry after a while.
   */
  public static final String ERROR_QUOTA_EXCEEDED = "QuotaExceeded";

  /**
   * Too many messages sent by the sender to a specific device.
   * Retry after a while.
   */
  public static final String ERROR_DEVICE_QUOTA_EXCEEDED =
      "DeviceQuotaExceeded";

  /**
   * Missing registration_id.
   * Sender should always add the registration_id to the request.
   */
  public static final String ERROR_MISSING_REGISTRATION = "MissingRegistration";

  /**
   * Bad registration_id. Sender should remove this registration_id.
   */
  public static final String ERROR_INVALID_REGISTRATION = "InvalidRegistration";

  /**
   * The sender_id contained in the registration_id does not match the
   * sender_id used to register with the GCM servers.
   */
  public static final String ERROR_MISMATCH_SENDER_ID = "MismatchSenderId";

  /**
   * The user has uninstalled the application or turned off notifications.
   * Sender should stop sending messages to this device and delete the
   * registration_id. The client needs to re-register with the GCM servers to
   * receive notifications again.
   */
  public static final String ERROR_NOT_REGISTERED = "NotRegistered";

  /**
   * The payload of the message is too big, see the limitations.
   * Reduce the size of the message.
   */
  public static final String ERROR_MESSAGE_TOO_BIG = "MessageTooBig";

  /**
   * Collapse key is required. Include collapse key in the request.
   */
  public static final String ERROR_MISSING_COLLAPSE_KEY = "MissingCollapseKey";

  /**
   * A particular message could not be sent because the GCM servers were not
   * available. Used only on JSON requests, as in plain text requests
   * unavailability is indicated by a 503 response.
   */
  public static final String ERROR_UNAVAILABLE = "Unavailable";

  /**
   * A particular message could not be sent because the GCM servers encountered
   * an error. Used only on JSON requests, as in plain text requests internal
   * errors are indicated by a 500 response.
   */
  public static final String ERROR_INTERNAL_SERVER_ERROR =
      "InternalServerError";

  /**
   * Time to Live value passed is less than zero or more than maximum.
   */
  public static final String ERROR_INVALID_TTL= "InvalidTtl";

  /**
   * Token returned by GCM when a message was successfully sent.
   */
  public static final String TOKEN_MESSAGE_ID = "id";

  /**
   * Token returned by GCM when the requested registration id has a canonical
   * value.
   */
  public static final String TOKEN_CANONICAL_REG_ID = "registration_id";

  /**
   * Token returned by GCM when there was an error sending a message.
   */
  public static final String TOKEN_ERROR = "Error";

  /**
   * JSON-only field representing the registration ids.
   */
  public static final String JSON_REGISTRATION_IDS = "registration_ids";

  /**
   * JSON-only field representing the payload data.
   */
  public static final String JSON_PAYLOAD = "data";

  /**
   * JSON-only field representing the number of successful messages.
   */
  public static final String JSON_SUCCESS = "success";

  /**
   * JSON-only field representing the number of failed messages.
   */
  public static final String JSON_FAILURE = "failure";

  /**
   * JSON-only field representing the number of messages with a canonical
   * registration id.
   */
  public static final String JSON_CANONICAL_IDS = "canonical_ids";

  /**
   * JSON-only field representing the id of the multicast request.
   */
  public static final String JSON_MULTICAST_ID = "multicast_id";

  /**
   * JSON-only field representing the result of each individual request.
   */
  public static final String JSON_RESULTS = "results";

  /**
   * JSON-only field representing the error field of an individual request.
   */
  public static final String JSON_ERROR = "error";

  /**
   * JSON-only field sent by GCM when a message was successfully sent.
   */
  public static final String JSON_MESSAGE_ID = "message_id";

  private Constants() {
    throw new UnsupportedOperationException();
  }

}
