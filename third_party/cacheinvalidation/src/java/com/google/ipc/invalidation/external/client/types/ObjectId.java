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

import com.google.ipc.invalidation.util.Preconditions;

import java.util.Arrays;

/**
 * A class to represent a unique object id that an application can register or
 * unregister for.
 *
 */
public final class ObjectId {

  /** The invalidation source type. */
  private final int source;

  /** The name/unique id for the object. */
  private final byte[] name;

  /**
   * Creates an object id for the given {@code source} and id {@code name} (does not make a copy of
   * the array).
   */
  public static ObjectId newInstance(int source, byte[] name) {
    return new ObjectId(source, name);
  }

  /** Creates an object id for the given {@code source} and id {@code name}. */
  private ObjectId(int source, byte[] name) {
    Preconditions.checkState(source >= 0, "source");
    this.source = source;
    this.name = Preconditions.checkNotNull(name, "name");
  }

  public int getSource() {
    return source;
  }

  public byte[] getName() {
    return name;
  }

  @Override
  public boolean equals(Object object) {
    if (object == this) {
      return true;
    }

    if (!(object instanceof ObjectId)) {
      return false;
    }

    final ObjectId other = (ObjectId) object;
    if ((source != other.source) || !Arrays.equals(name, other.name)) {
      return false;
    }
    return true;
  }

  @Override
  public int hashCode() {
    return source ^ Arrays.hashCode(name);
  }

  @Override
  public String toString() {
    return "Oid: <" + source + ", " + BytesFormatter.toString(name) + ">";
  }
}
