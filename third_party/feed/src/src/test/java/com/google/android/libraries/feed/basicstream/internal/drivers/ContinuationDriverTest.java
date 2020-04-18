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

package com.google.android.libraries.feed.basicstream.internal.drivers;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import android.view.View;
import com.google.android.libraries.feed.api.modelprovider.ModelProvider;
import com.google.android.libraries.feed.api.modelprovider.ModelToken;
import com.google.android.libraries.feed.basicstream.internal.viewholders.ContinuationViewHolder;
import com.google.android.libraries.feed.host.config.Configuration;
import com.google.android.libraries.feed.host.config.Configuration.ConfigKey;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link ContinuationDriver}.j */
@RunWith(RobolectricTestRunner.class)
public class ContinuationDriverTest {

  @Mock private ModelToken modelToken;
  @Mock private ModelProvider modelProvider;
  @Mock private ContinuationViewHolder continuationViewHolder;

  private ContinuationDriver continuationDriver;
  private Configuration configuration = new Configuration.Builder().build();

  @Before
  public void setup() {
    initMocks(this);

    continuationDriver = new ContinuationDriver(modelProvider, modelToken, configuration);
  }

  @Test
  public void testBind_immediatePaginationOn() {
    continuationDriver =
        new ContinuationDriver(
            modelProvider,
            modelToken,
            new Configuration.Builder().put(ConfigKey.TRIGGER_IMMEDIATE_PAGINATION, true).build());

    continuationDriver.bind(continuationViewHolder);
    verify(continuationViewHolder).bind(continuationDriver, /* showSpinner= */ true);
  }

  @Test
  public void testBind_immediatePaginationOff() {
    continuationDriver =
        new ContinuationDriver(
            modelProvider,
            modelToken,
            new Configuration.Builder().put(ConfigKey.TRIGGER_IMMEDIATE_PAGINATION, false).build());

    continuationDriver.bind(continuationViewHolder);
    verify(continuationViewHolder).bind(continuationDriver, /* showSpinner= */ false);
  }

  @Test
  public void testShowsSpinnerAfterClick() {
    continuationDriver.bind(continuationViewHolder);

    continuationDriver.onClick(any(View.class));

    continuationDriver.unbind();

    continuationDriver.bind(continuationViewHolder);

    verify(continuationViewHolder).bind(continuationDriver, /* showSpinner= */ true);
  }

  @Test
  public void testOnClick() {
    continuationDriver.bind(continuationViewHolder);
    continuationDriver.onClick(/* v= */ null);

    verify(continuationViewHolder).setShowSpinner(true);
    verify(modelProvider).handleToken(modelToken);
  }

  @Test
  public void testUnbind() {
    continuationDriver.bind(continuationViewHolder);
    assertThat(continuationDriver.isBound()).isTrue();
    continuationDriver.unbind();

    verify(continuationViewHolder).unbind();
    assertThat(continuationDriver.isBound()).isFalse();
  }

}
