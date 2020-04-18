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

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/**
 * Tests of the {@link ThreadUtils} class. Robolectric runs everything on the main thread, so the
 * tests are written to test the checks properly.
 */
@RunWith(RobolectricTestRunner.class)
public class ThreadUtilsTest {

  @Test
  public void testOnMainThread() {
    ThreadUtils threadUtils = new ThreadUtils();
    assertThat(threadUtils.isMainThread()).isTrue();
  }

  @Test
  public void testCheckMainThread() {
    ThreadUtils threadUtils = new ThreadUtils();
    // expect no exception
    threadUtils.checkMainThread();
  }

  @Test()
  public void testCheckNotMainThread() {
    final ThreadUtils threadUtils = new ThreadUtils();
    assertThatRunnable(threadUtils::checkNotMainThread)
        .throwsAnExceptionOfType(IllegalStateException.class);
  }
}
