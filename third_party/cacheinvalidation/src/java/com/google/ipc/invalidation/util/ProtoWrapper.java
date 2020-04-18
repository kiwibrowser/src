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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Locale;


/**
 * Base class for generated protobuf wrapper classes. Includes utilities for validation of proto
 * fields and implements hash code memoization.
 */
public abstract class ProtoWrapper extends InternalBase {

  /** Unchecked validation exception indicating a code issue. */
  public static final class ValidationArgumentException extends IllegalArgumentException {
    public ValidationArgumentException(String message) {
      super(message);
    }
  }

  /** Checked validation exception indicating an bogus protocol buffer instance. */
  public static final class ValidationException extends Exception {
    public ValidationException(String message) {
      super(message);
    }

    public ValidationException(Throwable cause) {
      super(cause);
    }
  }

  /** Immutable, empty list. */
  private static final List<?> EMPTY_LIST = Collections.unmodifiableList(new ArrayList<Object>(0));
  private static final int UNINITIALIZED_HASH_CODE = -1;
  private static final int NOT_UNITIALIZED_HASH_CODE = UNINITIALIZED_HASH_CODE + 1;
  private int hashCode;

  @Override
  public final int hashCode() {
    if (hashCode == UNINITIALIZED_HASH_CODE) {
      int computedHashCode = computeHashCode();

      // If computeHashCode() happens to return UNITIALIZED_HASH_CODE, replace it with a
      // different (constant but arbitrary) value so that the hash code doesn't need to be
      // recomputed.
      hashCode = (computedHashCode == UNINITIALIZED_HASH_CODE) ? NOT_UNITIALIZED_HASH_CODE
          : computedHashCode;
    }
    return hashCode;
  }

  /** Returns a hash code for this wrapper. */
  protected abstract int computeHashCode();

  /** Returns an immutable, empty list with elements of type {@code T}. */
  @SuppressWarnings("unchecked")
  protected static <T> List<T> emptyList() {
    return (List<T>) EMPTY_LIST;
  }

  /** Checks that the given field is non null. */
  protected static void required(String fieldName, Object fieldValue) {
    if (fieldValue == null) {
      throw new ValidationArgumentException(
          String.format(Locale.ROOT, "Required field '%s' was not set", fieldName));
    }
  }

  /**
   * Checks that the given collection contains non-null elements. Treats {@code null} as empty.
   * Returns an immutable copy of the given collection.
   */
  protected static <T> List<T> optional(String fieldName, Collection<T> fieldValues) {
    if ((fieldValues == null) || (fieldValues.size() == 0)) {
      return emptyList();
    }
    ArrayList<T> copy = new ArrayList<T>(fieldValues);
    for (int i = 0; i < copy.size(); i++) {
      if (copy.get(i) == null) {
        throw new ValidationArgumentException(String.format(Locale.ROOT,
            "Element %d of repeated field '%s' must not be null.", i, fieldName));
      }
    }
    return Collections.unmodifiableList(copy);
  }

  /**
   * Checks that the given field is non-empty. Returns an immutable copy of the given
   * collection.
   */
  protected static <T> List<T> required(String fieldName, Collection<T> fieldValues) {
    List<T> copy = optional(fieldName, fieldValues);
    if (fieldValues.isEmpty()) {
      throw new ValidationArgumentException(String.format(Locale.ROOT,
          "Repeated field '%s' must have at least one element", fieldName));
    }
    return copy;
  }

  /** Checks that the given field is non-negative. */
  protected static void nonNegative(String fieldName, int value) {
    if (value < 0) {
      throw new ValidationArgumentException(
          String.format(Locale.ROOT, "Field '%s' must be non-negative: %d", fieldName, value));
    }
  }

  /** Checks that the given field is non-negative. */
  protected static void nonNegative(String fieldName, long value) {
    if (value < 0) {
      throw new ValidationArgumentException(
          String.format(Locale.ROOT, "Field '%s' must be non-negative: %d", fieldName, value));
    }
  }

  /** Checks that the given field is positive. */
  protected static void positive(String fieldName, int value) {
    if (value <= 0) {
      throw new ValidationArgumentException(
          String.format(Locale.ROOT, "Field '%s' must be positive: %d", fieldName, value));
    }
  }

  /** Checks that the given field is positive. */
  protected static void positive(String fieldName, long value) {
    if (value <= 0) {
      throw new ValidationArgumentException(
          String.format(Locale.ROOT, "Field '%s' must be positive: %d", fieldName, value));
    }
  }

  /**
   * Checks that the given field is not empty. Only call when the field has a value:
   * {@link #required} can be called first, or the check can be conditionally performed.
   */
  protected static void nonEmpty(String fieldName, String value) {
    if (Preconditions.checkNotNull(value).length() == 0) {
      throw new ValidationArgumentException(
          String.format(Locale.ROOT, "Field '%s' must be non-empty", fieldName));
    }
  }

  /**
   * Checks that the given field is not empty. Only call when the field has a value:
   * {@link #required} can be called first, or the check can be conditionally performed.
   */
  protected static void nonEmpty(String fieldName, Bytes value) {
    if (Preconditions.checkNotNull(value).size() == 0) {
      throw new ValidationArgumentException(
          String.format(Locale.ROOT, "Field '%s' must be non-empty", fieldName));
    }
  }

  /** Checks that the given condition holds. */
  protected void check(boolean condition, String message) {
    if (!condition) {
      throw new ValidationArgumentException(String.format(Locale.ROOT, "%s: %s", message, this));
    }
  }

  /** Throws exception indicating a one-of violation due to multiple defined choices. */
  protected static void oneOfViolation(String field1, String field2) {
    throw new ValidationArgumentException(String.format(Locale.ROOT,
        "Multiple one-of fields defined, including: %s, %s", field1, field2));
  }

  /** Throws exception indicating that no one-of choices are defined. */
  protected static void oneOfViolation() {
    throw new ValidationArgumentException("No one-of fields defined");
  }

  //
  // Equals helpers.
  //

  /**
   * Returns {@code true} if the provided objects are both null or are non-null and equal. Returns
   * {@code false} otherwise.
   */
  protected static boolean equals(Object x, Object y) {
    if (x == null) {
      return y == null;
    }
    if (y == null) {
      return false;
    }
    return x.equals(y);
  }

  //
  // Hash code helpers for primitive types (taken from com.google.common.primitives package).
  //

  /** Returns hash code for the provided {@code long} value. */
  protected static int hash(long value) {
    // See Longs#hashCode
    return (int) (value ^ (value >>> 32));
  }

  /** Returns hash code for the provided {@code int} value. */
  protected static int hash(int value) {
    return value;
  }

  /** Returns hash code for the provided {@code boolean} value. */
  protected static int hash(boolean value) {
    // See Booleans#hashCode
    return value ? 1231 : 1237;
  }

  /** Returns hash code for the provided {@code float} value. */
  protected static int hash(float value) {
    return Float.valueOf(value).hashCode();
  }

  /** Returns hash code for the provided {@code double} value. */
  protected static int hash(double value) {
    return Double.valueOf(value).hashCode();
  }
}
