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

/** Test class for {@link Result} */
@RunWith(RobolectricTestRunner.class)
public class ResultTest {

  private static final String HELLO_WORLD = "Hello World";

  @Test
  public void success() throws Exception {
    Result<String> result = Result.success(HELLO_WORLD);
    assertThat(result.isSuccessful()).isTrue();
    assertThat(result.getValue()).isEqualTo(HELLO_WORLD);
  }

  @Test
  public void success_nullValue() throws Exception {
    Result<String> result = Result.success(null);
    assertThat(result.isSuccessful()).isTrue();
    assertThatRunnable(result::getValue).throwsAnExceptionOfType(NullPointerException.class);
  }

  @Test
  public void error() throws Exception {
    Result<String> result = Result.failure();
    assertThat(result.isSuccessful()).isFalse();
    assertThatRunnable(result::getValue).throwsAnExceptionOfType(IllegalStateException.class);
  }
}
