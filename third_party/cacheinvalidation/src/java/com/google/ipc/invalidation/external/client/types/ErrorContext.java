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
 * Extra information about the error in {@code ErrorInfo} - cast to appropriate subtype as
 * specified for the given reason.
 *
 */
public class ErrorContext {

  /** A context with numeric data. */
  public static class NumberContext extends ErrorContext {
    private int number;

    public NumberContext(int number) {
      this.number = number;
    }

    int getNumber() {
      return number;
    }
  }
}
