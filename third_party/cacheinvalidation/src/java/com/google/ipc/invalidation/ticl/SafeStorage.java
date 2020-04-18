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

import static com.google.ipc.invalidation.external.client.SystemResources.Scheduler.NO_DELAY;

import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.external.client.SystemResources.Scheduler;
import com.google.ipc.invalidation.external.client.SystemResources.Storage;
import com.google.ipc.invalidation.external.client.types.Callback;
import com.google.ipc.invalidation.external.client.types.SimplePair;
import com.google.ipc.invalidation.external.client.types.Status;
import com.google.ipc.invalidation.util.NamedRunnable;
import com.google.ipc.invalidation.util.Preconditions;

/**
 * An implementation of the Storage resource that schedules the callbacks on the given scheduler
 * thread.
 *
 */
public class SafeStorage implements Storage {

  /** The delegate to which the calls are forwarded. */
  private final Storage delegate;

  /** The scheduler on which the callbacks are scheduled. */
  private Scheduler scheduler;

  SafeStorage(Storage delegate) {
    this.delegate = Preconditions.checkNotNull(delegate);
  }

  @Override
  public void setSystemResources(SystemResources resources) {
    this.scheduler = resources.getInternalScheduler();
  }

  @Override
  public void writeKey(String key, byte[] value, final Callback<Status> done) {
    delegate.writeKey(key, value, new Callback<Status>() {
      @Override
      public void accept(final Status status) {
        scheduler.schedule(NO_DELAY, new NamedRunnable("SafeStorage.writeKey") {
          @Override
          public void run() {
            done.accept(status);
          }
        });
      }
    });
  }

  @Override
  public void readKey(String key, final Callback<SimplePair<Status, byte[]>> done) {
    delegate.readKey(key, new Callback<SimplePair<Status, byte[]>>() {
      @Override
      public void accept(final SimplePair<Status, byte[]> result) {
        scheduler.schedule(NO_DELAY, new NamedRunnable("SafeStorage.readKey") {
          @Override
          public void run() {
            done.accept(result);
          }
        });
      }
    });
  }

  @Override
  public void deleteKey(String key, final Callback<Boolean> done) {
    delegate.deleteKey(key, new Callback<Boolean>() {
      @Override
      public void accept(final Boolean success) {
        scheduler.schedule(NO_DELAY, new NamedRunnable("SafeStorage.deleteKey") {
          @Override
          public void run() {
            done.accept(success);
          }
        });
      }
    });
  }

  @Override
  public void readAllKeys(final Callback<SimplePair<Status, String>> keyCallback) {
    delegate.readAllKeys(new Callback<SimplePair<Status, String>>() {
      @Override
      public void accept(final SimplePair<Status, String> keyResult) {
        scheduler.schedule(NO_DELAY, new NamedRunnable("SafeStorage.readAllKeys") {
          @Override
          public void run() {
            keyCallback.accept(keyResult);
          }
        });
      }
    });
  }
}
