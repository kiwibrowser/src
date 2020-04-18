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
 * An immutable, semantic-free ordered pair of nullable values. These can be
 * accessed using the {@link #getFirst} and {@link #getSecond} methods. Equality
 * and hashing are defined in the natural way.
 *
 * @param <T1> The type of the first element
 * @param <T2> The type of the second element
 *
 */
public final class SimplePair<T1, T2> {
  /**
   * Creates a new pair containing the given elements in order.
   */
  public static <FirstType, SecondType> SimplePair<FirstType, SecondType> of(
      FirstType first, SecondType second) {
    return new SimplePair<FirstType, SecondType>(first, second);
  }

  /**
   * The first element of the pair; see also {@link #getFirst}.
   */
  public final T1 first;

  /**
   * The second element of the pair; see also {@link #getSecond}.
   */
  public final T2 second;

  /**
   * Constructor.  It is usually easier to call {@link #of}.
   */
  public SimplePair(T1 first, T2 second) {
    this.first = first;
    this.second = second;
  }

  /**
   * Returns the first element of this pair; see also {@link #first}.
   */
  public T1 getFirst() {
    return first;
  }

  /**
   * Returns the second element of this pair; see also {@link #second}.
   */
  public T2 getSecond() {
    return second;
  }

  @Override
  public boolean equals(Object object) {
    if (object instanceof SimplePair<?, ?>) {
      SimplePair<?, ?> that = (SimplePair<?, ?>) object;
      return areObjectsEqual(this.first, that.first) && areObjectsEqual(this.second, that.second);
    }
    return false;
  }

  /**
   * Determines whether two possibly-null objects are equal. Returns:
   *
   * <ul>
   * <li>{@code true} if {@code a} and {@code b} are both null.
   * <li>{@code true} if {@code a} and {@code b} are both non-null and they are
   *     equal according to {@link Object#equals(Object)}.
   * <li>{@code false} in all other situations.
   * </ul>
   *
   * <p>This assumes that any non-null objects passed to this function conform
   * to the {@code equals()} contract.
   */
  private static boolean areObjectsEqual(Object a, Object b) {
    return a == b || (a != null && a.equals(b));
  }

  @Override
  public int hashCode() {
    int hash1 = first == null ? 0 : first.hashCode();
    int hash2 = second == null ? 0 : second.hashCode();
    return 31 * hash1 + hash2;
  }

  /**
   * {@inheritDoc}
   *
   * <p>This implementation returns a string in the form
   * {@code (first, second)}, where {@code first} and {@code second} are the
   * String representations of the first and second elements of this pair, as
   * given by {@link String#valueOf(Object)}. Subclasses are free to override
   * this behavior.
   */
  @Override public String toString() {
    return "(" + first + ", " + second + ")";
  }
}
