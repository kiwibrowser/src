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

package com.google.android.libraries.feed.common.concurrent;

import com.google.android.libraries.feed.common.logging.Logger;
import java.util.concurrent.Executor;

/** Allows execution of a {@link Runnable} like an {@link Executor}. */
abstract class AbstractRunner {

  private static final String TAG = "Runner";

  private final Executor executor;
  private final String executorDescription;

  AbstractRunner(Executor executor, String executorDescription) {
    this.executor = executor;
    this.executorDescription = executorDescription;
  }

  /** Executes the {@code runnable} on the {@link Executor} used to initialize this class. */
  public void execute(String name, Runnable runnable) {
    Logger.d(TAG, "Running task [%s] on [%s]", name, executorDescription);
    executor.execute(runnable);
  }
}
