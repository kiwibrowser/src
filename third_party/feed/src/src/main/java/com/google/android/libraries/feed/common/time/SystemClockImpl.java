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

package com.google.android.libraries.feed.common.time;

import android.os.Build;
import android.os.SystemClock;

/**
 * Implementation of {@link Clock} that delegates to the system clock.
 *
 * <p>This class is intended for use only in contexts where injection is impossible. Where possible,
 * prefer to simply inject a {@link Clock}.
 */
public class SystemClockImpl implements Clock {
  @Override
  public long currentTimeMillis() {
    return System.currentTimeMillis();
  }

  @Override
  public long elapsedRealtime() {
    return SystemClock.elapsedRealtime();
  }

  @Override
  public long elapsedRealtimeNanos() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      try {
        return SystemClock.elapsedRealtimeNanos();
      } catch (NoSuchMethodError ignoredError) {
        // Some vendors have a SystemClock that doesn't contain the method even though the SDK
        // should contain it. Fall through to the alternate version.
      }
    }
    return SystemClock.elapsedRealtime() * NS_IN_MS;
  }

  @Override
  public long uptimeMillis() {
    return SystemClock.uptimeMillis();
  }
}
