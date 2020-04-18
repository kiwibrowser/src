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

import java.io.Serializable;

/**
 * Result of a GCM message request that returned HTTP status code 200.
 *
 * <p>
 * If the message is successfully created, the {@link #getMessageId()} returns
 * the message id and {@link #getErrorCodeName()} returns {@literal null};
 * otherwise, {@link #getMessageId()} returns {@literal null} and 
 * {@link #getErrorCodeName()} returns the code of the error.
 *
 * <p>
 * There are cases when a request is accept and the message successfully
 * created, but GCM has a canonical registration id for that device. In this
 * case, the server should update the registration id to avoid rejected requests
 * in the future.
 * 
 * <p>
 * In a nutshell, the workflow to handle a result is:
 * <pre>
 *   - Call {@link #getMessageId()}:
 *     - {@literal null} means error, call {@link #getErrorCodeName()}
 *     - non-{@literal null} means the message was created:
 *       - Call {@link #getCanonicalRegistrationId()}
 *         - if it returns {@literal null}, do nothing.
 *         - otherwise, update the server datastore with the new id.
 * </pre>
 */
public final class Result implements Serializable {

  private final String messageId;
  private final String canonicalRegistrationId;
  private final String errorCode;

  static final class Builder {

    // optional parameters
    private String messageId;
    private String canonicalRegistrationId;
    private String errorCode;

    public Builder canonicalRegistrationId(String value) {
      canonicalRegistrationId = value;
      return this;
    }

    public Builder messageId(String value) {
      messageId = value;
      return this;
    }

    public Builder errorCode(String value) {
      errorCode = value;
      return this;
    }

    public Result build() {
      return new Result(this);
    }
  }

  private Result(Builder builder) {
    canonicalRegistrationId = builder.canonicalRegistrationId;
    messageId = builder.messageId;
    errorCode = builder.errorCode;
  }

  /**
   * Gets the message id, if any.
   */
  public String getMessageId() {
    return messageId;
  }

  /**
   * Gets the canonical registration id, if any.
   */
  public String getCanonicalRegistrationId() {
    return canonicalRegistrationId;
  }

  /**
   * Gets the error code, if any.
   */
  public String getErrorCodeName() {
    return errorCode;
  }

  @Override
  public String toString() {
    StringBuilder builder = new StringBuilder("[");
    if (messageId != null) { 
      builder.append(" messageId=").append(messageId);
    }
    if (canonicalRegistrationId != null) {
      builder.append(" canonicalRegistrationId=")
          .append(canonicalRegistrationId);
    }
    if (errorCode != null) { 
      builder.append(" errorCode=").append(errorCode);
    }
    return builder.append(" ]").toString();
  }

}
