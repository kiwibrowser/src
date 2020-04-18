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

package com.google.ipc.invalidation.ticl.android2;

/**
 * Interface for the Android Ticl that provides a source of time.
 *
 */
public interface AndroidClock {
  /**
   * Implementation of {@code AndroidClock} that uses {@link System#currentTimeMillis()}.
   */
  static class SystemClock implements AndroidClock {
    @Override
    public long nowMs() {
      return System.currentTimeMillis();
    }
  }

  /** Returns milliseconds elapsed since the Unix epoch. */
  long nowMs();
}
