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

package com.google.android.libraries.feed.api.common;

import android.os.Looper;

/** Thread utilities. */
public class ThreadUtils {
  public ThreadUtils() {}

  /** Returns {@code true} if this method is being called from the main/UI thread. */
  public boolean isMainThread() {
    return Looper.getMainLooper() == Looper.myLooper();
  }

  public void checkNotMainThread() {
    check(!isMainThread(), "checkNotMainThread failed");
  }

  public void checkMainThread() {
    check(isMainThread(), "checkMainThread failed");
  }

  private void check(boolean condition, String message) {
    if (!condition) {
      throw new IllegalStateException(message);
    }
  }
}
