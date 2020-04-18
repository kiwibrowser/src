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
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.modelprovider.TokenCompletedObserver;
import com.google.search.now.feed.client.StreamDataProto.StreamToken;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of {@link UpdatableModelToken}. */
@RunWith(RobolectricTestRunner.class)
public class ModelTokenImplTest {
  @Mock private TokenCompletedObserver changeObserver;

  @Before
  public void setUp() {
    initMocks(this);
  }

  @Test
  public void testFeature() {
    StreamToken token = StreamToken.newBuilder().build();
    UpdatableModelToken modelToken = new UpdatableModelToken(token, false);
    assertThat(modelToken.getStreamToken()).isEqualTo(token);
  }

  @Test
  public void testChangeObserverList() {
    StreamToken token = StreamToken.newBuilder().build();
    UpdatableModelToken modelToken = new UpdatableModelToken(token, false);
    modelToken.registerObserver(changeObserver);
    List<TokenCompletedObserver> observers = modelToken.getObserversToNotify();
    assertThat(observers.size()).isEqualTo(1);
    assertThat(observers.get(0)).isEqualTo(changeObserver);
  }
}
