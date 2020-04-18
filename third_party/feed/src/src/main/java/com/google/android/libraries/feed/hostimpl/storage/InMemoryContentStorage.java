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

package com.google.android.libraries.feed.hostimpl.storage;

import static com.google.android.libraries.feed.host.storage.ContentOperation.Type.DELETE;
import static com.google.android.libraries.feed.host.storage.ContentOperation.Type.DELETE_BY_PREFIX;
import static com.google.android.libraries.feed.host.storage.ContentOperation.Type.UPSERT;

import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.host.storage.CommitResult;
import com.google.android.libraries.feed.host.storage.ContentMutation;
import com.google.android.libraries.feed.host.storage.ContentOperation;
import com.google.android.libraries.feed.host.storage.ContentOperation.Delete;
import com.google.android.libraries.feed.host.storage.ContentOperation.DeleteByPrefix;
import com.google.android.libraries.feed.host.storage.ContentOperation.Upsert;
import com.google.android.libraries.feed.host.storage.ContentStorage;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/** A {@link ContentStorage} that holds data in memory. */
public class InMemoryContentStorage implements ContentStorage, Dumpable {

  private static final String TAG = "InMemoryContentStorage";

  private final ThreadUtils threadUtils = new ThreadUtils();
  private final Map<String, byte[]> store = new HashMap<>();

  private int getCount = 0;
  private int getAllCount = 0;
  private int insertCount = 0;
  private int updateCount = 0;

  @Override
  public void get(List<String> keys, Consumer<Result<Map<String, byte[]>>> consumer) {
    threadUtils.checkMainThread();

    getCount++;

    Map<String, byte[]> valueMap = new HashMap<>(keys.size());
    for (String key : keys) {
      byte[] value = store.get(key);
      if (value == null || value.length == 0) {
        Logger.w(TAG, "Didn't find value for %s, not adding to map", key);
      } else {
        valueMap.put(key, value);
      }
    }
    consumer.accept(Result.success(valueMap));
  }

  @Override
  public void getAll(String prefix, Consumer<Result<Map<String, byte[]>>> consumer) {
    threadUtils.checkMainThread();

    getAllCount++;
    Map<String, byte[]> results = new HashMap<>();
    for (Entry<String, byte[]> entry : store.entrySet()) {
      if (entry.getKey().startsWith(prefix)) {
        results.put(entry.getKey(), entry.getValue());
      }
    }
    consumer.accept(Result.success(results));
  }

  @Override
  public void commit(ContentMutation mutation, Consumer<CommitResult> consumer) {
    threadUtils.checkMainThread();

    for (ContentOperation operation : mutation.getOperations()) {
      if (operation.getType() == UPSERT) {
        if (!upsert((Upsert) operation)) {
          consumer.accept(CommitResult.FAILURE);
          return;
        }
      } else if (operation.getType() == DELETE) {
        if (!delete((Delete) operation)) {
          consumer.accept(CommitResult.FAILURE);
          return;
        }
      } else if (operation.getType() == DELETE_BY_PREFIX) {
        if (!deleteByPrefix((DeleteByPrefix) operation)) {
          consumer.accept(CommitResult.FAILURE);
          return;
        }
      } else {
        Logger.e(TAG, "Invalid ContentMutation: unexpected operation: %s", operation.getType());
        consumer.accept(CommitResult.FAILURE);
        return;
      }
    }

    consumer.accept(CommitResult.SUCCESS);
  }

  private boolean deleteByPrefix(DeleteByPrefix operation) {
    List<String> keysToRemove = new ArrayList<>();
    for (String key : store.keySet()) {
      if (key.startsWith(operation.getPrefix())) {
        keysToRemove.add(key);
      }
    }
    store.keySet().removeAll(keysToRemove);
    return true;
  }

  private boolean delete(Delete operation) {
    store.remove(operation.getKey());
    return true;
  }

  private boolean upsert(Upsert operation) {
    String key = operation.getKey();
    // TODO: remove unneeded null checks.
    if (key == null) {
      Logger.e(TAG, "Invalid ContentMutation: null key");
      return false;
    }
    byte[] value = operation.getValue();
    if (value == null) {
      Logger.e(TAG, "Invalid ContentMutation: null value");
      return false;
    }
    if (value.length == 0) {
      Logger.e(TAG, "Invalid ContentMutation: empty value");
      return false;
    }
    byte[] currentValue = store.put(key, value);
    if (currentValue == null) {
      insertCount++;
    } else {
      updateCount++;
    }
    return true;
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    dumper.forKey("contentItems").value(store.size());
    dumper.forKey("getCount").value(getCount);
    dumper.forKey("getAllCount").value(getAllCount).compactPrevious();
    dumper.forKey("insertCount").value(insertCount).compactPrevious();
    dumper.forKey("updateCount").value(updateCount).compactPrevious();
  }
}
