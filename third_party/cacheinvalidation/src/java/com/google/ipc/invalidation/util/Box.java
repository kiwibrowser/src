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
 * Container for a single arbitrary value. Useful when a nested callback needs
 * to modify a primitive type, which is ordinarily not possible as variables
 * available to nested callbacks need to be declared final.
 *
 * @param <T> Type of the value being boxed.
 *
 */
public class Box<T> {

  /** Contents of the box. */
  private T value;

  /** Constructs a box with the given initial {@code value}. */
  public Box(T value) {
    this.value = value;
  }

  /** Constructs a Box with {@code null} as the value. */
  public Box() {
    this.value = null;
  }

  /** Constructs and returns a {@code Box} that wraps {@code objectValue}. */
  public static <T> Box<T> of(T objectValue) {
    return new Box<T>(objectValue);
  }

  public void set(T objectValue) {
    this.value = objectValue;
  }

  public T get() {
    return value;
  }

  @Override
  public String toString() {
    return (value == null) ? null : value.toString();
  }
}
