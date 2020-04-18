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

package com.google.android.libraries.feed.hostimpl.storage;

import static org.mockito.MockitoAnnotations.initMocks;

import android.content.Context;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.testing.conformance.storage.ContentStorageConformanceTest;
import com.google.common.util.concurrent.MoreExecutors;
import org.junit.After;
import org.junit.Before;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;

/** Tests for {@link PersistentContentStorage}. */
@RunWith(RobolectricTestRunner.class)
public class PersistentContentStorageTest extends ContentStorageConformanceTest {

  @Mock private ThreadUtils threadUtils;
  private Context context;

  @Before
  public void setUp() {
    initMocks(this);
    context = RuntimeEnvironment.application;
    storage =
        new PersistentContentStorage(
            context, MoreExecutors.newDirectExecutorService(), threadUtils);
  }

  @After
  public void tearDown() {
    context.getFilesDir().delete();
  }
}
