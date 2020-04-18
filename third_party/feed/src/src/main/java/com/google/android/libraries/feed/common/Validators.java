// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.common;


/**
 * Class similar to Guava's Preconditions. Not all users of Feed can use Guava libraries so we
 * define our own here.
 */
public class Validators {

  private Validators() {}

  /**
   * Ensures that an object reference passed as a parameter to the calling method is not null.
   *
   * @param reference an object reference
   * @param debugString a debug string to add to the NPE if reference is null
   * @param formatArgs arguments to populate the debugString using String.format
   * @return the non-null reference that was validated
   * @throws NullPointerException if {@code reference} is null
   */
  public static <T> T checkNotNull(
      /*@Nullable*/ T reference, String debugString, Object... formatArgs) {
    if (reference == null) {
      throw new NullPointerException(String.format(debugString, formatArgs));
    }
    return reference;
  }

  /**
   * Ensures that an object reference passed as a parameter to the calling method is not null.
   *
   * @param reference an object reference
   * @return the non-null reference that was validated
   * @throws NullPointerException if {@code reference} is null
   */
  public static <T> T checkNotNull(/*@Nullable*/ T reference) {
    if (reference == null) {
      throw new NullPointerException();
    }
    return reference;
  }

  /**
   * Assert that {@code expression} is {@code true}.
   *
   * @param expression a boolean value that should be true
   * @param formatString a format string for the message for the IllegalStateException
   * @param formatArgs arguments to the format string
   * @throws IllegalStateException thrown if the expression is false
   */
  public static void checkState(boolean expression, String formatString, Object... formatArgs) {
    if (!expression) {
      throw new IllegalStateException(String.format(formatString, formatArgs));
    }
  }

  /**
   * Assert that {@code expression} is {@code true}.
   *
   * @param expression a boolean value that should be true
   * @throws IllegalStateException thrown if the expression is false
   */
  public static void checkState(boolean expression) {
    if (!expression) {
      throw new IllegalStateException();
    }
  }
}
