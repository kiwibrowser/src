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

/**
 * Allows reading and writing to an append-only storage medium.
 *
 * <p>Storage instances can be accessed from multiple threads.
 *
 * <p>[INTERNAL LINK]
 */
public interface JournalStorage {

  /**
   * Reads the journal and asynchronously returns the contents.
   *
   * <p>Reads on journals that do not exist will fulfill with an empty list.
   */
  void read(String journalName, Consumer<Result<List<byte[]>>> consumer);

  /**
   * Commits the operations in {@link JournalMutation} in order and asynchronously reports the
   * {@link CommitResult}. If all the operations succeed, {@code callback} is called with a success
   * result. If any operation fails, {@code callback} is called with a failure result and the
   * remaining operations are not processed.
   *
   * <p>This operation is not guaranteed to be atomic.
   */
  void commit(JournalMutation mutation, Consumer<CommitResult> consumer);

  /** Determines whether a journal exists and asynchronously responds. */
  void exists(String journalName, Consumer<Result<Boolean>> consumer);

  /** Asynchronously retrieve a list of all current journals */
  void getAllJournals(Consumer<Result<List<String>>> consumer);
}
