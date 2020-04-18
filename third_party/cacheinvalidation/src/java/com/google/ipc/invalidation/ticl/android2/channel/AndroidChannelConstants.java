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
package com.google.ipc.invalidation.ticl.android2.channel;

import com.google.ipc.invalidation.ticl.proto.AndroidChannel.MajorVersion;
import com.google.ipc.invalidation.ticl.proto.ClientProtocol.Version;

/**
 * Constants used by the network channel.
 *
 */
public final class AndroidChannelConstants {

  /** Constants used in Intents sent to retrieve auth tokens from the application. */
  public static class AuthTokenConstants {
    /**
     * Action requesting that an auth token to send a message be provided. This is the action
     * used in the intent to the application.
     */
    public static final String ACTION_REQUEST_AUTH_TOKEN =
        "com.google.ipc.invalidation.AUTH_TOKEN_REQUEST";

    /** Extra in an auth token request response providing the pending intent. */
    public static final String EXTRA_PENDING_INTENT =
        "com.google.ipc.invalidation.AUTH_TOKEN_PENDING_INTENT";

    /**
     * Extra in an auth token request message indicating that the token provided as the value
     * was invalid when last used. This may be set on the intent to the application.
     */
    public static final String EXTRA_INVALIDATE_AUTH_TOKEN =
        "com.google.ipc.invalidaton.AUTH_TOKEN_INVALIDATE";

    /** Extra in the intent from the application that provides the auth token string. */
    public static final String EXTRA_AUTH_TOKEN = "com.google.ipc.invalidation.AUTH_TOKEN";

    /**
     * Extra in the intent from the application that provides the so-called "auth token type". If
     * the auth token is a GoogleLogin token, then this value must name the Gaia service (e.g.,
     * "chromiumsync") for which the token was generated. If the auth token is a Gaia OAuth2 token,
     * then this value must have the form "oauth2:{scope}", where {scope} is a Google API
     * authentication scope such as "https://www.googleapis.com/auth/chromesync".
     */
    public static final String EXTRA_AUTH_TOKEN_TYPE =
        "com.google.ipc.invalidation.AUTH_TOKEN_TYPE";

    /**
     * Extra in the intent from the application that provides the message to send. We store this
     * ourselves in the intent inside the pending intent that we give to the application.
     */
    static final String EXTRA_STORED_MESSAGE = "com.google.ipc.invalidation.AUTH_TOKEN_MSG";

    /**
     * Extra in the intent from the application that indicates whether the intent is for a retry
     * after a failed authentication. If we find that an auth token no longer works, we will tell
     * the application to invalidate it, retrieve a new one, and send us back the message and the
     * new token, but we do not want to go into an infinite loop if authentication never succeeds.
     */
    static final String EXTRA_IS_RETRY = "com.google.ipc.invalidation.AUTH_TOKEN_IS_RETRY";
  }

  /** Constants used in HTTP requests to the data center. */
  public static class HttpConstants {
    /** The URL of the invalidation channel service */
    public static final String CHANNEL_URL = "https://clients4.google.com/";

    /** The MIME content type to use for requests that contain binary protobuf */
    public static final String PROTO_CONTENT_TYPE = "application/x-protobuffer";

    /** The relative URL to use to send inbound client requests to the Android frontend */
    public static final String REQUEST_URL = "/invalidation/android/request/";

    /**
     * The name of the query parameter that contains the service name that should be used to
     * validate the authentication token provided with the request.
     */
    public static final String SERVICE_PARAMETER = "service";

    /**
     * The name of the header that contains the echoed token. This token is included in all C2DM
     * messages to the client and is echoed back under this header on all client HTTP requests.
     */
    public static final String ECHO_HEADER = "echo-token";
  }

  /** Constants used in C2DM messages. */
   public static class C2dmConstants {
    /**
     * Name of C2DM parameter containing message content.  If not set, data is retrieved via
     * the mailbox frontend
     */
    public static final String CONTENT_PARAM = "content";

    /** Name of the C2DM parameter containing an opaque token to be echoed on HTTP requests. */
    public static final String ECHO_PARAM = "echo-token";
  }

  /** The channel version expected by this channel implementation. */
  public static final Version CHANNEL_VERSION = Version.create(MajorVersion.INITIAL, 0);

  /**
   * An extra set on an intent to the AndroidMessageSenderService to inform it that a GCM
   * registration id change has occurred. This is sent by the AndroidMessageReceiverService
   * to trigger the sender service to send any buffered messages when a GCM registration id first
   * becomes available.
   * <p>
   * The value associated with this extra is ignored.
   */
  static final String MESSAGE_SENDER_SVC_GCM_REGID_CHANGE =
      "com.google.ipc.invalidation.channel.sender.gcm_regid_change";
  
  /** Tag used by the GcmRegistrationTaskService. */
  static final String GCM_REGISTRATION_TASK_SERVICE_TAG = "gcm_registration_task_service";

  private AndroidChannelConstants() {
    // Disallow instantiation.
  }
}
