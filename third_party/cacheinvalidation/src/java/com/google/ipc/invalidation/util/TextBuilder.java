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

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;

/**
 * A {@link TextBuilder} is an abstraction that allows classes to efficiently append their string
 * representations and then use them later for human consumption, e.g., for debugging or logging. It
 * is currently a wrapper around {@link StringBuilder} and {@link Formatter} to give us format and
 * append capabilities together. All append methods return this TextBuilder so that the method calls
 * can be chained.
 *
 */
public class TextBuilder {

  private final StringBuilder builder;
  private final UtilFormatter formatter;

  /**
   * Given an {@code object} that is an instance of {@code clazz}, outputs names and values of all
   * member fields declared on {@code clazz}. This method should be used carefully:
   * <ol>
   * <li>This method is expensive. For frequently logged types, an ad hoc
   *     {@link InternalBase#toCompactString} implementation is preferred.</li>
   * <li>May overflow the stack if there is a cycle in an object graph.</li>
   * <li>Custom formatters have been implemented for many protos. They will not be used by this
   *     method.</li>
   * </ol>
   */
  public static void outputFieldsToBuilder(TextBuilder builder, Object object, Class<?> clazz) {
    Preconditions.checkArgument(clazz.isAssignableFrom(object.getClass()));

    // Get all the fields and print them using toCompactString if possible;
    // otherwise, via toString
    Field[] fields = clazz.getDeclaredFields();
    for (Field field : fields) {
      try {
        // Ignore static final fields, as they're uninteresting.
        int modifiers = field.getModifiers();
        if (Modifier.isStatic(modifiers) && Modifier.isFinal(modifiers)) {
          continue;
        }

        field.setAccessible(true);
        builder.append(field.getName() + " = ");
        Object fieldValue = field.get(object);
        if (fieldValue instanceof InternalBase) {
          ((InternalBase) fieldValue).toCompactString(builder);
        } else {
          builder.append(fieldValue);
        }
        builder.append(", ");
      } catch (IllegalArgumentException e) {
        e.printStackTrace();
      } catch (IllegalAccessException e) {
        e.printStackTrace();
      }
    }
  }

  /**
   * Returns an empty TextBuilder to which various objects' string
   * representations can be added later.
   */
  public TextBuilder() {
   builder = new StringBuilder();
   formatter = new UtilFormatter(builder);
  }

  /**
   * Appends the string representation of {@code c} to this builder.
   *
   * @param c the character being appended
   */
  public TextBuilder append(char c) {
    builder.append(c);
    return this;
  }

  /**
   * Appends the string representation of {@code i} to this builder.
   *
   * @param i the integer being appended
   */
  public TextBuilder append(int i) {
    builder.append(i);
    return this;
  }

  /**
   * Appends the toString representation of {@code object} to this builder.
   */
  public TextBuilder append(Object object) {
    if (object instanceof InternalBase) {
      return append((InternalBase) object);
    } else {
      builder.append(object);
      return this;
    }
  }

  /**
   * Appends the {@code InternalBase#toCompactString} representation of {@code object} to this
   * builder.
   */
  public TextBuilder append(InternalBase object) {
    if (object == null) {
      return append("null");
    }
    object.toCompactString(this);
    return this;
  }

  /**
   * Appends the comma-separated {@code InternalBase#toCompactString} representations of
   * {@code objects} to this builder.
   */
  public TextBuilder append(Iterable<? extends InternalBase> objects) {
    if (objects == null) {
      return this;
    }
    boolean first = true;
    for (InternalBase object : objects) {
      if (first) {
        first = false;
      } else {
        builder.append(", ");
      }
      append(object);
    }
    return this;
  }

  /** Appends the {@link Bytes#toString} representation of {@code bytes} to this builder. */
  public TextBuilder append(byte[] bytes) {
    if (bytes == null) {
      return append("null");
    }
    Bytes.toCompactString(this, bytes);
    return this;
  }

  /**
   * Appends the string representation of {@code l} to this builder.
   *
   * @param l the long being appended
   */
  public TextBuilder append(long l) {
    builder.append(l);
    return this;
  }

  /**
   * Appends the string representation of {@code b} to this builder.
   *
   * @param b the boolean being appended
   */
  public TextBuilder append(boolean b) {
    builder.append(b);
    return this;
  }

  /**
   * Appends {@code s} to this builder.
   *
   * @param s the string being appended
   */
  public TextBuilder append(String s) {
    builder.append(s);
    return this;
  }

  /**
   * Writes a formatted string to this using the specified format string and
   * arguments.
   *
   * @param format the format as used in {@link java.util.Formatter}
   * @param args the arguments that are converted to their string form using
   * {@code format}
   */
  public TextBuilder appendFormat(String format, Object... args) {
    formatter.format(format, args);
    return this;
  }

  @Override
  public String toString() {
    return builder.toString();
  }
}
