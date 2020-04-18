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

package com.google.android.libraries.feed.host.storage;

import android.support.annotation.IntDef;

/** A mutation to the underlying {@link ContentStorage}. */
public abstract class ContentOperation {
  /** The types of operations. */
  @IntDef({Type.DELETE, Type.DELETE_BY_PREFIX, Type.UPSERT})
  public @interface Type {
    int DELETE = 0;
    int DELETE_BY_PREFIX = 1;
    int UPSERT = 3;
  }

  public @Type int getType() {
    return type;
  }

  private final @Type int type;

  // Only the following classes may extend ContentOperation
  private ContentOperation(@Type int type) {
    this.type = type;
  }

  /**
   * A {@link ContentOperation} created by calling {@link ContentMutation.Builder#upsert(String,
   * byte[])}.
   */
  public static class Upsert extends ContentOperation {
    private final String key;
    private final byte[] value;

    Upsert(String key, byte[] value) {
      super(Type.UPSERT);
      this.key = key;
      this.value = value;
    }

    public String getKey() {
      return key;
    }

    public byte[] getValue() {
      return value;
    }
  }

  /**
   * A {@link ContentOperation} created by calling {@link ContentMutation.Builder#delete(String)}.
   */
  public static class Delete extends ContentOperation {
    private final String key;

    Delete(String key) {
      super(Type.DELETE);
      this.key = key;
    }

    public String getKey() {
      return key;
    }
  }

  /**
   * A {@link ContentOperation} created by calling {@link
   * ContentMutation.Builder#deleteByPrefix(String)}.
   */
  public static class DeleteByPrefix extends ContentOperation {
    private final String prefix;

    DeleteByPrefix(String prefix) {
      super(Type.DELETE_BY_PREFIX);
      this.prefix = prefix;
    }

    public String getPrefix() {
      return prefix;
    }
  }
}
