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

import java.util.Collection;


/**
 * Utilities to enable creation of lazy strings, where the instantiation of the string is delayed
 * so that, e.g., log messages that aren't printed have reduced overhead.
 */
public class LazyString {

  /** Receiver formatting objects using {@link Object#toString()}. */
  public static final LazyStringReceiver<Object> OBJECT_RECEIVER =
      new LazyStringReceiver<Object>() {
        @Override
        public void appendToBuilder(TextBuilder builder, Object element) {
          builder.append(element);
        }
      };

  /**
   * Receiver appending an {@code element} to the given {@code builder}. Implementations may assume
   * the builder and element are not {@code null}.
   */
  public interface LazyStringReceiver<T> {
    void appendToBuilder(TextBuilder builder, T element);
  }

  /**
   * Given an {@code element} to be logged lazily, returns null if the object is null. Otherwise,
   * return an object that would convert it to a string using {@code builderFunction}. I.e., this
   * method will call {@code builderFunction} with a new {@link TextBuilder} and provided
   * {@code element} and return the string created with it.
   */
  public static <T> Object toLazyCompactString(final T element,
      final LazyStringReceiver<T> builderFunction) {
    if (element == null) {
      return null;
    }
    return new Object() {
      @Override public String toString() {
        TextBuilder builder = new TextBuilder();
        builderFunction.appendToBuilder(builder, element);
        return builder.toString();
      }
    };
  }

  /**
   * Returns an object that converts the given {@code elements} array into a debug string when
   * {@link Object#toString} is called using
   * {@link #appendElementsToBuilder(TextBuilder, Object[], LazyStringReceiver)}.
   */
  public static <T> Object toLazyCompactString(final T[] elements,
      final LazyStringReceiver<? super T> elementReceiver) {
    if ((elements == null) || (elements.length == 0)) {
      return null;
    }
    return new Object() {
      @Override public String toString() {
        return appendElementsToBuilder(new TextBuilder(), elements, elementReceiver).toString();
      }
    };
  }

  /**
   * Returns an object that converts the given {@code elements} collection into a debug string when
   * {@link Object#toString} is called using
   * {@link #appendElementsToBuilder(TextBuilder, Object[], LazyStringReceiver)}.
   */
  public static <T> Object toLazyCompactString(final Collection<T> elements,
      final LazyStringReceiver<? super T> elementReceiver) {
    if ((elements == null) || elements.isEmpty()) {
      return null;
    }
    return new Object() {
      @Override public String toString() {
        return appendElementsToBuilder(new TextBuilder(), elements, elementReceiver).toString();
      }
    };
  }

  /**
   * Appends {@code elements} formatted using {@code elementReceiver} to {@code builder}. Elements
   * are comma-separated.
   */
  public static <T> TextBuilder appendElementsToBuilder(TextBuilder builder, T[] elements,
      LazyStringReceiver<? super T> elementReceiver) {
    if (elements == null) {
      return builder;
    }
    for (int i = 0; i < elements.length; i++) {
      if (i != 0) {
        builder.append(", ");
      }
      T element = elements[i];
      if (element != null) {
        elementReceiver.appendToBuilder(builder, element);
      }
    }
    return builder;
  }

  /**
   * Appends {@code elements} formatted using {@code elementReceiver} to {@code builder}. Elements
   * are comma-separated.
   */
  public static <T> TextBuilder appendElementsToBuilder(TextBuilder builder,
      Iterable<T> elements, LazyStringReceiver<? super T> elementReceiver) {
    if (elements == null) {
      return builder;
    }
    boolean first = true;
    for (T element : elements) {
      if (first) {
        first = false;
      } else {
        builder.append(", ");
      }
      if (element != null) {
        elementReceiver.appendToBuilder(builder, element);
      }
    }
    return builder;
  }

  private LazyString() {  // To prevent instantiation.
  }
}
