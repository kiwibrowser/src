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

package com.google.ipc.invalidation.ticl;

import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.external.client.SystemResources.Scheduler;
import com.google.ipc.invalidation.external.client.SystemResources.Storage;
import com.google.ipc.invalidation.external.client.types.Callback;
import com.google.ipc.invalidation.external.client.types.SimplePair;
import com.google.ipc.invalidation.external.client.types.Status;
import com.google.ipc.invalidation.util.Bytes;
import com.google.ipc.invalidation.util.InternalBase;
import com.google.ipc.invalidation.util.NamedRunnable;
import com.google.ipc.invalidation.util.TextBuilder;
import com.google.ipc.invalidation.util.TypedUtil;

import java.util.HashMap;
import java.util.Map;


/**
 * Map-based in-memory implementation of {@link Storage}.
 *
 */
public class MemoryStorageImpl extends InternalBase implements Storage {
  private Scheduler scheduler;
  private Map<String, byte[]> ticlPersistentState = new HashMap<String, byte[]>();

  @Override
  public void writeKey(final String key, final byte[] value, final Callback<Status> callback) {
    // Need to schedule immediately because C++ locks aren't reentrant, and
    // C++ locking code assumes that this call will not return directly.

    // Schedule the write even if the resources are started since the
    // scheduler will prevent it from running in case the resources have been
    // stopped.
    scheduler.schedule(Scheduler.NO_DELAY,
        new NamedRunnable("MemoryStorage.writeKey") {
      @Override
      public void run() {
        ticlPersistentState.put(key, value);
        callback.accept(Status.newInstance(Status.Code.SUCCESS, ""));
      }
    });
  }

  int numKeysForTest() {
    return ticlPersistentState.size();
  }

  @Override
  public void setSystemResources(SystemResources resources) {
    this.scheduler = resources.getInternalScheduler();
  }

  @Override
  public void readKey(final String key, final Callback<SimplePair<Status, byte[]>> done) {
    scheduler.schedule(Scheduler.NO_DELAY,
        new NamedRunnable("MemoryStorage.readKey") {
      @Override
      public void run() {
        byte[] value = TypedUtil.mapGet(ticlPersistentState, key);
        final SimplePair<Status, byte[]> result;
        if (value != null) {
          result = SimplePair.of(Status.newInstance(Status.Code.SUCCESS, ""), value);
        } else {
          String error = "No value present in map for " + key;
          result = SimplePair.of(Status.newInstance(Status.Code.PERMANENT_FAILURE, error), null);
        }
        done.accept(result);
      }
    });
  }

  @Override
  public void deleteKey(final String key, final Callback<Boolean> done) {
    scheduler.schedule(Scheduler.NO_DELAY,
        new NamedRunnable("MemoryStorage.deleteKey") {
      @Override
      public void run() {
        TypedUtil.remove(ticlPersistentState, key);
        done.accept(true);
      }
    });
  }

  @Override
  public void readAllKeys(final Callback<SimplePair<Status, String>> done) {
    scheduler.schedule(Scheduler.NO_DELAY,
        new NamedRunnable("MemoryStorage.readAllKeys") {
      @Override
      public void run() {
        Status successStatus = Status.newInstance(Status.Code.SUCCESS, "");
        for (String key : ticlPersistentState.keySet()) {
          done.accept(SimplePair.of(successStatus, key));
        }
        done.accept(null);
      }
    });
  }

  /**
   * Same as write except without any callbacks and is NOT done on the internal thread.
   * Test code should typically call this before starting the client.
   */
  void writeForTest(final String key, final byte[] value) {
    ticlPersistentState.put(key, value);
  }

  /**
   * Sets the scheduler, for tests. The Android tests use this to supply a scheduler that executes
   * no-delay items in-line.
   */
  public void setSchedulerForTest(Scheduler newScheduler) {
    scheduler = newScheduler;
  }

  /**
   * Same as read except without any callbacks and is NOT done on the internal thread.
   */
  public byte[] readForTest(final String key) {
    return ticlPersistentState.get(key);
  }

  @Override
  public void toCompactString(TextBuilder builder) {
    builder.append("Storage state: ");
    for (Map.Entry<String, byte[]> entry : ticlPersistentState.entrySet()) {
      builder.appendFormat("<%s, %s>, ", entry.getKey(), Bytes.toString(entry.getValue()));
    }
  }
}
