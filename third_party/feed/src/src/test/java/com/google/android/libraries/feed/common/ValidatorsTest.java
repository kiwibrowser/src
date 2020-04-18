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

package com.google.android.libraries.feed.common;

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link Validators} class. */
@RunWith(RobolectricTestRunner.class)
public class ValidatorsTest {

  @Test
  public void testCheckNotNull_null() {
    assertThatRunnable(() -> Validators.checkNotNull(null))
        .throwsAnExceptionOfType(NullPointerException.class);
  }

  @Test
  public void testCheckNotNull_debugString() {
    String debugString = "Debug-String";
    assertThatRunnable(() -> Validators.checkNotNull(null, debugString))
        .throwsAnExceptionOfType(NullPointerException.class)
        .that()
        .hasMessageThat()
        .contains(debugString);
  }

  @Test
  public void testCheckNotNull_notNull() {
    String test = "test";
    assertThat(Validators.checkNotNull(test)).isEqualTo(test);
  }

  @Test
  public void testCheckState_trueWithMessagel() {
    // no exception expected
    Validators.checkState(true, "format-string");
  }

  @Test
  public void testCheckState_True() {
    // no exception expected
    Validators.checkState(true);
  }

  @Test
  public void testCheckState_falseWithMessage() {
    assertThatRunnable(() -> Validators.checkState(false, "format-string"))
        .throwsAnExceptionOfType(IllegalStateException.class);
  }

  @Test
  public void testCheckState_false() {
    assertThatRunnable(() -> Validators.checkState(false))
        .throwsAnExceptionOfType(IllegalStateException.class);
  }
}
