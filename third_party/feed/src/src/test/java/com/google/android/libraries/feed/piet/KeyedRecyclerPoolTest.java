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

import android.app.Activity;
import android.content.Context;
import android.view.View;
import com.google.search.now.ui.piet.ElementsProto.Element;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link KeyedRecyclerPool} */
@RunWith(RobolectricTestRunner.class)
public class KeyedRecyclerPoolTest {

  private static final RecyclerKey KEY1 = new TestRecyclerKey("KEY1");
  private static final RecyclerKey KEY2 = new TestRecyclerKey("KEY2");
  private static final RecyclerKey KEY3 = new TestRecyclerKey("KEY3");
  private static final int MAX_KEYS = 10;
  private static final int CAPACITY = 11;

  @Mock TestElementAdapter adapter;
  @Mock TestElementAdapter adapter2;
  @Mock TestElementAdapter adapter3;
  @Mock TestElementAdapter adapter4;

  private Context testContext;

  @Before
  public void setUp() {
    testContext = Robolectric.setupActivity(Activity.class);
    initMocks(this);
  }

  @Test
  public void testPutAndGetOneElement() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, CAPACITY);
    pool.put(KEY1, adapter);
    assertThat(pool.get(KEY1)).isEqualTo(adapter);
  }

  @Test
  public void testGetFromEmptyPoolReturnsNull() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, CAPACITY);
    assertThat(pool.get(KEY1)).isNull();
  }

  @Test
  public void testGetFromEmptyPoolWithDifferentKeyReturnsNull() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, CAPACITY);
    pool.put(KEY1, adapter);
    assertThat(pool.get(KEY2)).isNull();
  }

  @Test
  public void testPutAndGetElementsWithDifferentKeys() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, CAPACITY);
    pool.put(KEY1, adapter);
    pool.put(KEY2, adapter2);
    assertThat(pool.get(KEY2)).isEqualTo(adapter2);
    assertThat(pool.get(KEY1)).isEqualTo(adapter);
  }

  @Test
  public void testPutNullElementFails() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, CAPACITY);
    pool.put(KEY1, null);
    assertThat(pool.get(KEY1)).isNull();
  }

  @Test
  public void testPutNullKeyFails() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, CAPACITY);
    assertThatRunnable(() -> pool.put(null, adapter))
        .throwsAnExceptionOfType(NullPointerException.class)
        .that()
        .hasMessageThat()
        .contains("null key for adapter");
  }

  @Test
  public void testGetNullKeyReturnsNull() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, CAPACITY);
    assertThat(pool.get(null)).isNull();
  }

  @Test
  public void testCacheOverflowEjectsPool() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(2, CAPACITY);
    pool.put(KEY1, adapter);
    pool.put(KEY2, adapter2);
    pool.put(KEY3, adapter3);
    assertThat(pool.get(KEY1)).isNull();
    assertThat(pool.get(KEY2)).isEqualTo(adapter2);
    assertThat(pool.get(KEY3)).isEqualTo(adapter3);
  }

  @Test
  public void testOverflowSinglePoolIgnoresLastElement() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, 2);
    pool.put(KEY1, adapter);
    pool.put(KEY1, adapter2);
    pool.put(KEY1, adapter3);
    assertThat(pool.get(KEY1)).isNotNull();
    assertThat(pool.get(KEY1)).isNotNull();
    assertThat(pool.get(KEY1)).isNull();
  }

  @Test
  public void testFillAllPools() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(2, 2);
    pool.put(KEY1, adapter);
    pool.put(KEY1, adapter2);
    pool.put(KEY2, adapter3);
    pool.put(KEY2, adapter4);
    assertThat(pool.get(KEY1)).isNotNull();
    assertThat(pool.get(KEY1)).isNotNull();
    assertThat(pool.get(KEY1)).isNull();
    assertThat(pool.get(KEY2)).isNotNull();
    assertThat(pool.get(KEY2)).isNotNull();
    assertThat(pool.get(KEY2)).isNull();
  }

  @Test
  public void testClear() {
    KeyedRecyclerPool<TestElementAdapter> pool = new KeyedRecyclerPool<>(MAX_KEYS, MAX_KEYS);
    pool.put(KEY1, adapter);
    pool.put(KEY1, adapter2);
    pool.put(KEY2, adapter3);
    pool.put(KEY2, adapter4);
    pool.clear();
    assertThat(pool.get(KEY1)).isNull();
    assertThat(pool.get(KEY2)).isNull();
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

  private class TestElementAdapter extends ElementAdapter<View, Object> {
    TestElementAdapter() {
      super(testContext, null, new View(testContext));
    }

    @Override
    protected Object getModelFromElement(Element baseElement) {
      return null;
    }
  }
}
