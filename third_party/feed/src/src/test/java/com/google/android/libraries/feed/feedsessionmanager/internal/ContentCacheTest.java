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

package com.google.android.libraries.feed.feedsessionmanager.internal;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.common.testing.ContentIdGenerators;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link ContentCache} class. */
@RunWith(RobolectricTestRunner.class)
public class ContentCacheTest {
  private final ContentIdGenerators idGenerators = new ContentIdGenerators();

  @Before
  public void setUp() {
    initMocks(this);
  }

  @Test
  public void testBasicCaching() {
    ContentCache cache = new ContentCache();
    assertThat(cache.size()).isEqualTo(0);

    StreamPayload payload = StreamPayload.getDefaultInstance();
    String contentId = idGenerators.createFeatureContentId(1);
    cache.put(contentId, payload);
    assertThat(cache.size()).isEqualTo(1);
    assertThat(cache.get(contentId)).isEqualTo(payload);

    String missingId = idGenerators.createFeatureContentId(2);
    assertThat(cache.get(missingId)).isNull();
  }

  @Test
  public void testLifecycle() {
    ContentCache cache = new ContentCache();
    assertThat(cache.size()).isEqualTo(0);

    cache.startMutation();
    assertThat(cache.size()).isEqualTo(0);

    StreamPayload payload = StreamPayload.getDefaultInstance();
    String contentId = idGenerators.createFeatureContentId(1);
    cache.put(contentId, payload);
    assertThat(cache.size()).isEqualTo(1);
    assertThat(cache.get(contentId)).isEqualTo(payload);

    cache.finishMutation();
    assertThat(cache.size()).isEqualTo(1);
    assertThat(cache.get(contentId)).isEqualTo(payload);

    cache.startMutation();
    assertThat(cache.size()).isEqualTo(0);
    assertThat(cache.get(contentId)).isNull();
  }
}
