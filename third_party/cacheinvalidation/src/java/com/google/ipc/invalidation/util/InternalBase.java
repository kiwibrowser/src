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
 * {@code InternalBase} is a class from which other classes can derive that allows an efficient
 * toString implementation for logging/debugging purposes for those classes. The class is abstract
 * so that it is never instantiated explicitly.
 *
 */
public abstract class InternalBase {

  /**
   * Adds a compact representation of this object to {@code builder}.
   *
   * @param builder the builder in which the string representation is added
   */
  public abstract void toCompactString(TextBuilder builder);

  /**
   * Adds a verbose representation of this object to {@code builder}. The
   * default implementation for toVerboseString is to simply call
   * toCompactString.
   *
   * @param builder the builder in which the string representation is added
   */
  public void toVerboseString(TextBuilder builder) {
    toCompactString(builder);
  }

  @Override
  public String toString() {
    TextBuilder builder = new TextBuilder();
    toCompactString(builder);
    return builder.toString();
  }

  /**
   * Creates a TextBuilder internally and returns a string based on the {@code
   * toVerboseString} method described above.
   */
  public String toVerboseString() {
    TextBuilder builder = new TextBuilder();
    toVerboseString(builder);
    return builder.toString();
  }

  /**
   * Given a set of {@code objects}, calls {@code toCompactString} on each of
   * them with the {@code builder} and separates each object's output in the
   * {@code builder} with a comma.
   */
  public static void toCompactStrings(TextBuilder builder,
      Iterable<? extends InternalBase> objects) {
    boolean first = true;
    for (InternalBase object : objects) {
      if (!first) {
        builder.append(", ");
      }
      object.toCompactString(builder);
      first = false;
    }
  }

  /**
   * Given a set of {@code objects}, calls {@code toString} on each of
   * them with the {@code builder} and separates each object's output in the
   * {@code builder} with a comma.
   */
  public static void toStrings(TextBuilder builder, Iterable<?> objects) {
    boolean first = true;
    for (Object object : objects) {
      if (!first) {
        builder.append(", ");
      }
      builder.append(object.toString());
      first = false;
    }
  }
}
