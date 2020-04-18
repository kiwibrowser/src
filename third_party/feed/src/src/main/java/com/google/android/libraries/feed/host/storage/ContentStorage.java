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

import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import java.util.List;
import java.util.Map;

/**
 * Allows reading and writing of content, currently key-value pairs.
 *
 * <p>Storage instances can be accessed from multiple threads.
 */
public interface ContentStorage {

  /**
   * Asynchronously requests the value for multiple keys. If a key does not have a value, it will
   * not be included in the map.
   */
  void get(List<String> keys, Consumer<Result<Map<String, byte[]>>> consumer);

  /** Asynchronously requests all key/value pairs from storage with a matching key prefix. */
  void getAll(String prefix, Consumer<Result<Map<String, byte[]>>> consumer);

  /**
   * Commits the operations in the {@link ContentMutation} in order and asynchronously reports the
   * {@link CommitResult}.
   *
   * <p>This operation is not guaranteed to be atomic. In the event of a failure, processing is
   * halted immediately, so the database may be left in an invalid state. Should this occur, Feed
   * behavior is undefined. Currently the plan is to wipe out existing data and start over.
   */
  void commit(ContentMutation mutation, Consumer<CommitResult> consumer);
}
