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

package com.google.ipc.invalidation.external.client.types;

/**
 * Information given to about a operation - success, temporary or permanent failure.
 *
 */
public final class Status {

  /** Actual status of the operation: Whether successful, transient or permanent failure. */
  public enum Code {
    /** Operation was successful. */
    SUCCESS,

    /**
     * Operation had a transient failure. The application can retry the failed operation later -
     * if it chooses to do so, it must use a sensible backoff policy such as exponential backoff.
     */
    TRANSIENT_FAILURE,

    /**
     * Opration has a permanent failure. Application must not automatically retry without fixing
     * the situation (e.g., by presenting a dialog box to the user).
     */
    PERMANENT_FAILURE
  }

  /** Success or failure. */
  private final Code code;

  /** A message describing why the state was unknown, for debugging. */
  private final String message;

  /** Creates a new Status object given the {@code code} and {@code message}. */
  public static Status newInstance(Status.Code code, String message) {
    return new Status(code, message);
  }

  private Status(Code code, String message) {
    this.code = code;
    this.message = message;
  }

  /** Returns true iff the status corresponds to a successful completion of the operation.*/
  public boolean isSuccess() {
    return code == Code.SUCCESS;
  }

  /**
   * Whether the failure is transient for an operation, e.g., for a {@code register} operation.
   * For transient failures, the application can retry the operation but it must use a sensible
   * policy such as exponential backoff so that it does not add significant load to the backend
   * servers.
   */
  public boolean isTransientFailure() {
    return code == Code.TRANSIENT_FAILURE;
  }

  /**
   * Whether the failure is transient for an operation, e.g., for a {@code register} operation.
   * See discussion in {@code Status.Code} for permanent and transient failure handling.
   */
  public boolean isPermanentFailure() {
    return code == Code.PERMANENT_FAILURE;
  }

  /** A message describing why the state was unknown, for debugging. */
  public String getMessage() {
    return message;
  }

  /** Returns the code for this status message. */
  public Code getCode() {
    return code;
  }

  @Override
  public String toString() {
    return "Code: " + code + ", " + message;
  }

  @Override
  public boolean equals(Object object) {
    if (object == this) {
      return true;
    }

    if (!(object instanceof Status)) {
      return false;
    }

    final Status other = (Status) object;
    if (code != other.code) {
      return false;

    }
    if (message == null) {
      return other.message == null;
    }
    return message.equals(other.message);
  }

  @Override
  public int hashCode() {
    return code.hashCode() ^ ((message == null) ? 0 : message.hashCode());
  }
}
