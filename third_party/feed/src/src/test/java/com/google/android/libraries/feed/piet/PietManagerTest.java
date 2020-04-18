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

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import com.google.android.libraries.feed.common.functional.Supplier;
import com.google.android.libraries.feed.common.testing.Suppliers;
import com.google.android.libraries.feed.host.config.DebugBehavior;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.android.libraries.feed.piet.host.CustomElementProvider;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link PietManager}. */
@RunWith(RobolectricTestRunner.class)
public class PietManagerTest {

  @Mock private ActionHandler actionHandler;
  @Mock private AssetProvider assetProvider;
  @Mock private CustomElementProvider customElementProvider;
  @Mock private Supplier<ViewGroup> cardViewSupplier;
  @Mock Context mockContext1;
  @Mock Context mockContext2;

  private Context context;
  private ViewGroup viewGroup1;
  private ViewGroup viewGroup2;

  private PietManager pietManager;

  @Before
  public void setUp() throws Exception {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    viewGroup1 = new LinearLayout(context);
    viewGroup2 = new FrameLayout(context);
    pietManager =
        new PietManager(
            DebugBehavior.VERBOSE,
            assetProvider,
            customElementProvider);
  }

  @Test
  public void testCreatePietFrameAdapter() {
    FrameAdapter frameAdapter =
        pietManager.createPietFrameAdapter(cardViewSupplier, actionHandler, context);
    assertThat(frameAdapter.getParameters().parentViewSupplier).isEqualTo(cardViewSupplier);
  }

  @Test
  public void testGetAdapterParameters() {
    Supplier<ViewGroup> viewGroupProducer1 = Suppliers.of(viewGroup1);
    Supplier<ViewGroup> viewGroupProducer2 = Suppliers.of(viewGroup2);
    AdapterParameters returnParams;

    // Get params for a context that does not exist
    returnParams = pietManager.getAdapterParameters(mockContext1, viewGroupProducer1);
    assertThat(returnParams.parentViewSupplier).isEqualTo(viewGroupProducer1);
    assertThat(returnParams.context).isEqualTo(mockContext1);

    // Get params for the same context again (use cached value)
    returnParams = pietManager.getAdapterParameters(mockContext1, Suppliers.of(null));
    assertThat(returnParams.parentViewSupplier).isEqualTo(viewGroupProducer1);

    // Get params for a different context
    returnParams = pietManager.getAdapterParameters(mockContext2, viewGroupProducer2);
    assertThat(returnParams.parentViewSupplier).isEqualTo(viewGroupProducer2);
  }

  @Test
  public void testPurgeRecyclerPools() {
    // Test with null adapterParameters
    pietManager.purgeRecyclerPools();

    ElementAdapterFactory mockFactory = mock(ElementAdapterFactory.class);
    pietManager.adapterParameters =
        new AdapterParameters(context, cardViewSupplier, null, mockFactory);
    pietManager.purgeRecyclerPools();
    verify(mockFactory).purgeRecyclerPools();
  }
}
