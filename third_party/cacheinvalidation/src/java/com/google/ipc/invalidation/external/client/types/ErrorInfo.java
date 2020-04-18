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
 * Information about an error given to the application.
 *
 */
public final class ErrorInfo {
  /**
   * Possible reasons for error in {@code InvalidationListener.informError}. The application writer
   * must NOT assume that this is complete list since error codes may be added later. That is, for
   * error codes that it cannot handle, it should not necessarily just crash the code. It may want
   * to present a dialog box to the user (say). For each ErrorReason, the ErrorInfo object has a
   * context object. We describe the type and meaning of the context for each named constant below.
   */
  public static class ErrorReason {
    /** The provided authentication/authorization token is not valid for use. */
    public static final int AUTH_FAILURE = 1;

    /** An unknown failure - more human-readable information is in the error message. */
    public static final int UNKNOWN_FAILURE = -1;

    private ErrorReason() {} // not instantiable
  }

  /** The cause of the failure. */
  private final int errorReason;

  /**
   * Is the error transient or permanent. See discussion in {@code Status.Code} for permanent and
   * transient failure handling.
   */
  private final boolean isTransient;

  /** Human-readable description of the error. */
  private final String errorMessage;

  /** Extra information about the error - cast to appropriate object as specified by the reason. */
  private final Object context;

  /**
   * Returns a new ErrorInfo object given the reason for the error, whether it is transient or
   * permanent, a helpful error message and extra context about the error.
   */
  public static ErrorInfo newInstance(int errorReason, boolean isTransient,
      String errorMessage, ErrorContext context) {
    return new ErrorInfo(errorReason, isTransient, errorMessage, context);
  }

  private ErrorInfo(int errorReason, boolean isTransient, String errorMessage,
      ErrorContext context) {
    this.errorReason = errorReason;
    this.isTransient = isTransient;
    this.errorMessage = errorMessage;
    this.context = context;
  }

  public boolean isTransient() {
    return isTransient;
  }

  public String getErrorMessage() {
    return errorMessage;
  }

  public int getErrorReason() {
    return errorReason;
  }

  public Object getContext() {
    return context;
  }

  @Override
  public String toString() {
    return "ErrorInfo: " + errorReason + ", " + isTransient + ", " + errorMessage + ", " + context;
  }
}
