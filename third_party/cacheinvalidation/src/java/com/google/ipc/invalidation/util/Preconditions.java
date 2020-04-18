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
package com.google.ipc.invalidation.util;


/**
 * Precondition checkers modeled after {@link com.google.common.base.Preconditions}. Duplicated here
 * to avoid the dependency on guava in Java client code.
 */
public class Preconditions {

  /**
   * Throws {@link NullPointerException} if the {@code reference} argument is
   * {@code null}. Otherwise, returns {@code reference}.
   */
  public static <T> T checkNotNull(T reference) {
    if (reference == null) {
      throw new NullPointerException();
    }
    return reference;
  }

  /**
   * Throws {@link NullPointerException} if the {@code reference} argument is
   * {@code null}. Otherwise, returns {@code reference}.
   */
  public static <T> T checkNotNull(T reference, Object errorMessage) {
    if (reference == null) {
      throw new NullPointerException(String.valueOf(errorMessage));
    }
    return reference;
  }

  /** Throws {@link IllegalStateException} if the given {@code expression} is {@code false}. */
  public static void checkState(boolean expression) {
    if (!expression) {
      throw new IllegalStateException();
    }
  }

  /** Throws {@link IllegalStateException} if the given {@code expression} is {@code false}. */
  public static void checkState(boolean expression, Object errorMessage) {
    if (!expression) {
      throw new IllegalStateException(String.valueOf(errorMessage));
    }
  }

  /** Throws {@link IllegalArgumentException} if the given {@code expression} is {@code false}. */
  public static void checkArgument(boolean expression) {
    if (!expression) {
      throw new IllegalArgumentException();
    }
  }

  /** Throws {@link IllegalArgumentException} if the given {@code expression} is {@code false}. */
  public static void checkArgument(boolean expression, Object errorMessage) {
    if (!expression) {
      throw new IllegalArgumentException(String.valueOf(errorMessage));
    }
  }

  // Do not instantiate.
  private Preconditions() {
  }
}
