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

import com.google.android.libraries.feed.host.storage.ContentOperation.Delete;
import com.google.android.libraries.feed.host.storage.ContentOperation.DeleteByPrefix;
import com.google.android.libraries.feed.host.storage.ContentOperation.Upsert;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * A sequence of {@link ContentOperation} instances. This class is used to commit changes to {@link
 * ContentStorage}.
 */
public class ContentMutation {
  private final List<ContentOperation> operations;

  private ContentMutation(List<ContentOperation> operations) {
    this.operations = Collections.unmodifiableList(operations);
  }

  /** An unmodifiable list of operations to be committed by {@link ContentStorage}. */
  public List<ContentOperation> getOperations() {
    return operations;
  }

  /**
   * Creates a {@link ContentMutation}, which can be used to apply mutations to the underlying
   * {@link ContentStorage}.
   */
  public static class Builder {
    private final ArrayList<ContentOperation> operations = new ArrayList<>();
    private final ContentOperationListSimplifier simplifier = new ContentOperationListSimplifier();

    /**
     * Sets the key/value pair in {@link ContentStorage}, or inserts it if it doesn't exist.
     *
     * <p>This method can be called repeatedly to assign multiple key/values. If the same key is
     * assigned multiple times, only the last value will be persisted.
     */
    public Builder upsert(String key, byte[] value) {
      operations.add(new Upsert(key, value));
      return this;
    }

    /**
     * Deletes the value from {@link ContentStorage} with a matching key, if it exists.
     *
     * <p>{@link ContentStorage#commit(ContentMutation)} will fulfill with result is {@code TRUE},
     * even if {@code key} is not found in {@link ContentStorage}.
     *
     * <p>If {@link Delete} and {@link Upsert} are committed for the same key, only the last
     * operation will take effect.
     */
    public Builder delete(String key) {
      operations.add(new Delete(key));
      return this;
    }

    /**
     * Deletes all values from {@link ContentStorage} with matching key prefixes.
     *
     * <p>{@link ContentStorage#commit(ContentMutation)} will fulfill with result equal to {@code
     * TRUE}, even if no keys have a matching prefix.
     *
     * <p>If {@link DeleteByPrefix} and {@link Upsert} are committed for the same matching key, only
     * the last operation will take effect.
     */
    public Builder deleteByPrefix(String prefix) {
      operations.add(new DeleteByPrefix(prefix));
      return this;
    }

    /**
     * Simplifies the sequence of {@link ContentOperation} instances, and returns a new {@link
     * ContentMutation} the simplified list.
     */
    public ContentMutation build() {
      return new ContentMutation(simplifier.simplify(operations));
    }
  }
}
