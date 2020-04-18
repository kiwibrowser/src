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

package com.google.android.libraries.feed.piet;

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import android.view.View;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link SingleKeyRecyclerPool} */
@RunWith(RobolectricTestRunner.class)
public class SingleKeyRecyclerPoolTest {

  public static final TestRecyclerKey DEFAULT_KEY = new TestRecyclerKey("HappyKey");
  public static final TestRecyclerKey INVALID_KEY = new TestRecyclerKey("INVALID");
  public static final int DEFAULT_CAPACITY = 2;

  @Mock ElementAdapter<View, Object> adapter;
  @Mock ElementAdapter<View, Object> adapter2;
  @Mock ElementAdapter<View, Object> adapter3;

  SingleKeyRecyclerPool<ElementAdapter<View, Object>> pool;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    pool = new SingleKeyRecyclerPool<>(DEFAULT_KEY, DEFAULT_CAPACITY);
  }

  @Test
  public void testPutAndGetOneElement() {
    pool.put(DEFAULT_KEY, adapter);
    assertThat(pool.get(DEFAULT_KEY)).isEqualTo(adapter);
  }

  @Test
  public void testGetOnEmptyPoolReturnsNull() {
    assertThat(pool.get(DEFAULT_KEY)).isNull();
  }

  @Test
  public void testGetWithDifferentKeyFails() {
    assertThatRunnable(() -> pool.get(INVALID_KEY))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("does not match singleton key");
  }

  @Test
  public void testPutWithDifferentKeyFails() {
    assertThatRunnable(() -> pool.put(INVALID_KEY, adapter))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("does not match singleton key");
  }

  @Test
  public void testGetWithNullKeyFails() {
    assertThatRunnable(() -> pool.get(null))
        .throwsAnExceptionOfType(IllegalArgumentException.class)
        .that()
        .hasMessageThat()
        .contains("does not match singleton key");
  }

  @Test
  public void testPutWithNullKeyFails() {
    assertThatRunnable(() -> pool.put(null, adapter))
        .throwsAnExceptionOfType(NullPointerException.class)
        .that()
        .hasMessageThat()
        .contains("null key for adapter");
  }

  @Test
  public void testPutSameElementTwiceFails() {
    pool.put(DEFAULT_KEY, adapter);
    assertThatRunnable(() -> pool.put(DEFAULT_KEY, adapter))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Already in the pool!");
  }

  @Test
  public void testPutAndGetTwoElements() {
    pool.put(DEFAULT_KEY, adapter);
    pool.put(DEFAULT_KEY, adapter2);
    assertThat(pool.get(DEFAULT_KEY)).isNotNull();
    assertThat(pool.get(DEFAULT_KEY)).isNotNull();
    assertThat(pool.get(DEFAULT_KEY)).isNull();
  }

  @Test
  public void testPoolOverflowIgnoresLastElement() {
    pool.put(DEFAULT_KEY, adapter);
    pool.put(DEFAULT_KEY, adapter2);
    pool.put(DEFAULT_KEY, adapter3);
    assertThat(pool.get(DEFAULT_KEY)).isNotNull();
    assertThat(pool.get(DEFAULT_KEY)).isNotNull();
    assertThat(pool.get(DEFAULT_KEY)).isNull();
  }

  @Test
  public void testClear() {
    pool.put(DEFAULT_KEY, adapter);
    pool.put(DEFAULT_KEY, adapter2);
    pool.clear();
    assertThat(pool.get(DEFAULT_KEY)).isNull();
  }

  private static class TestRecyclerKey extends RecyclerKey {
    private final String key;

    TestRecyclerKey(String key) {
      this.key = key;
    }

    @Override
    public int hashCode() {
      return key.hashCode();
    }

    @Override
    public boolean equals(/*@Nullable*/ Object obj) {
      if (obj == this) {
        return true;
      }

      if (obj == null) {
        return false;
      }

      if (!(obj instanceof TestRecyclerKey)) {
        return false;
      }

      TestRecyclerKey otherKey = (TestRecyclerKey) obj;
      return otherKey.key.equals(this.key);
    }
  }
}
