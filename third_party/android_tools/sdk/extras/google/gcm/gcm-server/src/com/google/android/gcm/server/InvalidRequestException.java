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

import java.io.IOException;

/**
 * Exception thrown when GCM returned an error due to an invalid request.
 * <p>
 * This is equivalent to GCM posts that return an HTTP error different of 200.
 */
public final class InvalidRequestException extends IOException {

  private final int status;
  private final String description;

  public InvalidRequestException(int status) {
    this(status, null);
  }

  public InvalidRequestException(int status, String description) {
    super(getMessage(status, description));
    this.status = status;
    this.description = description;
  }

  private static String getMessage(int status, String description) {
    StringBuilder base = new StringBuilder("HTTP Status Code: ").append(status);
    if (description != null) {
      base.append("(").append(description).append(")");
    }
    return base.toString();
  }

  /**
  * Gets the HTTP Status Code.
  */
  public int getHttpStatusCode() {
   return status;
  }

  /**
   * Gets the error description.
   */
  public String getDescription() {
    return description;
  }

}
