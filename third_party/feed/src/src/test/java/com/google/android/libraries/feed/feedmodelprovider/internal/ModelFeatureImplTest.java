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

package com.google.android.libraries.feed.feedmodelprovider.internal;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.modelprovider.ModelCursor;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link UpdatableModelFeature}. */
@RunWith(RobolectricTestRunner.class)
public class ModelFeatureImplTest {

  private StreamFeature streamFeature;
  @Mock private CursorProvider cursorProvider;
  @Mock private ModelCursor modelCursor;

  @Before
  public void setup() {
    initMocks(this);
    String contentId = "content-id";
    streamFeature = StreamFeature.newBuilder().setContentId(contentId).build();
    when(cursorProvider.getCursor(contentId)).thenReturn(modelCursor);
  }

  @Test
  public void testBase() {
    UpdatableModelFeature modelFeature = new UpdatableModelFeature(streamFeature, cursorProvider);
    assertThat(modelFeature.getStreamFeature()).isEqualTo(streamFeature);
    assertThat(modelFeature.getCursor()).isEqualTo(modelCursor);
  }
}
