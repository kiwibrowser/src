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

package com.google.android.libraries.feed.basicstream.internal.viewholders;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import com.google.android.libraries.feed.api.actionparser.ActionParser;
import com.google.android.libraries.feed.basicstream.internal.StreamActionApiImpl;
import com.google.android.libraries.feed.common.functional.Supplier;
import com.google.android.libraries.feed.host.stream.CardConfiguration;
import com.google.android.libraries.feed.piet.FrameAdapter;
import com.google.android.libraries.feed.piet.PietManager;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests for {@link PietViewHolder}. */
@RunWith(RobolectricTestRunner.class)
public class PietViewHolderTest {
  private static final int CARD_BOTTOM_PADDING = 42;

  @Mock private CardConfiguration cardConfiguration;
  @Mock private FrameAdapter frameAdapter;
  @Mock private ActionParser actionParser;
  @Mock private PietManager pietManager;
  @Mock private StreamActionApiImpl streamActionApi;

  private Context context;
  private PietViewHolder pietViewHolder;
  private FrameLayout frameLayout;
  private final List<PietSharedState> pietSharedStates = new ArrayList<>();

  @Before
  public void setUp() {
    initMocks(this);
    context = Robolectric.setupActivity(Activity.class);
    frameLayout = new FrameLayout(context);
    when(pietManager.createPietFrameAdapter(
            any(Supplier.class), any(ActionHandler.class), eq(context)))
        .thenReturn(frameAdapter);
    when(frameAdapter.getFrameContainer()).thenReturn(new LinearLayout(context));

    pietViewHolder =
        new PietViewHolder(
            cardConfiguration, frameLayout, pietManager, context, streamActionApi, actionParser);
  }

  @Test
  public void testBind_clearsPadding() {
    frameLayout.setPadding(1, 2, 3, 4);

    pietViewHolder.bind(Frame.getDefaultInstance(), pietSharedStates);

    assertThat(frameLayout.getPaddingLeft()).isEqualTo(0);
    assertThat(frameLayout.getPaddingRight()).isEqualTo(0);
    assertThat(frameLayout.getPaddingTop()).isEqualTo(0);
    assertThat(frameLayout.getPaddingBottom()).isEqualTo(0);
  }

  @Test
  public void testBind_setsBackground() {
    ColorDrawable redBackground = new ColorDrawable(Color.RED);
    when(cardConfiguration.getCardBackground()).thenReturn(redBackground);

    pietViewHolder.bind(Frame.getDefaultInstance(), pietSharedStates);

    assertThat(frameLayout.getBackground()).isEqualTo(redBackground);
  }

  @Test
  public void testBind_setsTheming() {
    when(cardConfiguration.getCardBottomPadding()).thenReturn((float) CARD_BOTTOM_PADDING);

    pietViewHolder.bind(Frame.getDefaultInstance(), pietSharedStates);

    assertThat(((MarginLayoutParams) pietViewHolder.itemView.getLayoutParams()).bottomMargin)
        .isEqualTo(CARD_BOTTOM_PADDING);
  }

  @Test
  public void testBind_bindsModel() {
    pietViewHolder.bind(Frame.getDefaultInstance(), pietSharedStates);

    verify(frameAdapter)
        .bindModel(Frame.getDefaultInstance(), /* shardingControl= */ null, pietSharedStates);
  }

  @Test
  public void testBind_onlyBindsOnce() {
    pietViewHolder.bind(Frame.getDefaultInstance(), pietSharedStates);

    verify(frameAdapter)
        .bindModel(Frame.getDefaultInstance(), /* shardingControl= */ null, pietSharedStates);
    verify(frameAdapter).getFrameContainer();

    pietViewHolder.bind(Frame.getDefaultInstance(), pietSharedStates);

    verifyNoMoreInteractions(frameAdapter);
  }

  @Test
  public void testUnbind() {
    pietViewHolder.bind(Frame.getDefaultInstance(), pietSharedStates);

    pietViewHolder.unbind();
    InOrder inOrder = Mockito.inOrder(frameAdapter);

    inOrder
        .verify(frameAdapter)
        .bindModel(Frame.getDefaultInstance(), /* shardingControl= */ null, pietSharedStates);
    inOrder.verify(frameAdapter).unbindModel();
    inOrder.verifyNoMoreInteractions();
  }
}
