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

package com.google.android.libraries.feed.common.testing;

import static com.google.common.truth.Truth.assertThat;

import java.util.concurrent.CountDownLatch;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test class for {@link RequiredConsumer} */
@RunWith(JUnit4.class)
public class RequiredConsumerTest {

  @Test
  public void testConsumer() throws Exception {
    CountDownLatch latch = new CountDownLatch(1);
    Boolean callValue = Boolean.TRUE;

    RequiredConsumer<Boolean> consumer =
        new RequiredConsumer<>(
            input -> {
              assertThat(input).isEqualTo(callValue);
              latch.countDown();
            });

    consumer.accept(callValue);

    assertThat(latch.getCount()).isEqualTo(0);
    assertThat(consumer.isCalled()).isTrue();
  }
}
